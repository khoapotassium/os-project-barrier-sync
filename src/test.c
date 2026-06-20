// Time có cả barrier_init, barrier_destroy, tạo thread, chờ thread.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/* ---- cross-platform WALL-CLOCK timer (Windows + Linux) ---- */
#ifdef _WIN32
#include <windows.h>
static double now_ms(void) {
    static LARGE_INTEGER freq; static int init = 0; LARGE_INTEGER t;
    if (!init) { QueryPerformanceFrequency(&freq); init = 1; }
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart * 1000.0 / (double)freq.QuadPart;
}
#else
#include <time.h>
static double now_ms(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
}
#endif

/* ---- global data ---- */
static int    NUM_STUDENTS;
static float *student_scores;
static double global_sum;
static int    global_fail_count;
static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    int thread_count;
    int waiting_count;
    int generation;
} Barrier;

typedef struct { int id; Barrier *barrier; int start_idx; int end_idx; } WorkerArgs;

static void barrier_init(Barrier *b, int n) {
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->cond, NULL);
    b->thread_count = n; b->waiting_count = 0; b->generation = 0;
}
static void barrier_destroy(Barrier *b) {
    pthread_mutex_destroy(&b->mutex);
    pthread_cond_destroy(&b->cond);
}
static void barrier_wait(Barrier *b) {            /* same logic, no printf */
    pthread_mutex_lock(&b->mutex);
    int gen = b->generation;
    b->waiting_count++;
    if (b->waiting_count == b->thread_count) {
        b->waiting_count = 0;
        b->generation++;
        pthread_cond_broadcast(&b->cond);
    } else {
        while (gen == b->generation)
            pthread_cond_wait(&b->cond, &b->mutex);
    }
    pthread_mutex_unlock(&b->mutex);
}

static void *worker_run(void *arg) {
    WorkerArgs *w = (WorkerArgs *)arg;
    double local_sum = 0.0;
    for (int i = w->start_idx; i < w->end_idx; i++) local_sum += student_scores[i];
    pthread_mutex_lock(&data_mutex); global_sum += local_sum; pthread_mutex_unlock(&data_mutex);

    barrier_wait(w->barrier);

    double average = global_sum / NUM_STUDENTS;
    int local_fail = 0;
    for (int i = w->start_idx; i < w->end_idx; i++)
        if (student_scores[i] < average) local_fail++;
    pthread_mutex_lock(&data_mutex); global_fail_count += local_fail; pthread_mutex_unlock(&data_mutex);

    barrier_wait(w->barrier);
    return NULL;
}

/* serial baseline: no threads at all */
static int run_serial(double *avg_out) {
    double sum = 0.0;
    for (int i = 0; i < NUM_STUDENTS; i++) sum += student_scores[i];
    double average = sum / NUM_STUDENTS;
    int fail = 0;
    for (int i = 0; i < NUM_STUDENTS; i++) if (student_scores[i] < average) fail++;
    *avg_out = average; return fail;
}

/* run one configuration, return best time of `repeat` runs */
// Có cả barrier_init, barrier_destroy, tạo thread, chờ thread.

static double measure(int threads, int repeat, int *fail_out, double *avg_out) {
    double best = 1e18;
    for (int r = 0; r < repeat; r++) {
        global_sum = 0.0; global_fail_count = 0;
        double t0 = now_ms();
        if (threads == 0) {
            *fail_out = run_serial(avg_out);
        } else {
            pthread_t th[64]; WorkerArgs ar[64]; Barrier bar;
            barrier_init(&bar, threads);
            int chunk = NUM_STUDENTS / threads;
            for (int i = 0; i < threads; i++) {
                ar[i].id = i + 1; ar[i].barrier = &bar;
                ar[i].start_idx = i * chunk;
                ar[i].end_idx = (i == threads - 1) ? NUM_STUDENTS : (i + 1) * chunk;
                pthread_create(&th[i], NULL, worker_run, &ar[i]);
            }
            for (int i = 0; i < threads; i++) pthread_join(th[i], NULL);
            barrier_destroy(&bar);
            *fail_out = global_fail_count;
            *avg_out  = global_sum / NUM_STUDENTS;
        }
        double dt = now_ms() - t0;
        if (dt < best) best = dt;
    }
    return best;
}

int main(void) {
    const int sizes[]   = { 1000000, 10000000 };
    const int threads[] = { 0, 1, 2, 4, 8, 12 };   /* 0 = serial */
    const int REPEAT    = 7;

    for (int s = 0; s < 2; s++) {
        NUM_STUDENTS = sizes[s];
        srand(12345);                               /* fixed seed -> identical data */
        student_scores = (float *)malloc(sizeof(float) * NUM_STUDENTS);
        for (int i = 0; i < NUM_STUDENTS; i++)
            student_scores[i] = (float)(rand() % 101) / 10.0f;

        printf("\n================  N = %d records  ================\n", NUM_STUDENTS);
        printf("%-12s | %12s | %8s | %10s | %s\n",
               "Config", "time (ms)", "speedup", "average", "below avg");
        printf("-------------+--------------+----------+------------+-----------\n");

        double serial_time = 0.0;
        for (int k = 0; k < 6; k++) {
            int fail; double avg;
            double t = measure(threads[k], REPEAT, &fail, &avg);
            if (threads[k] == 0) serial_time = t;
            double sp = serial_time / t;
            char name[16];
            if (threads[k] == 0) sprintf(name, "Serial");
            else                 sprintf(name, "%d threads", threads[k]);
            printf("%-12s | %12.3f | %7.2fx | %10.4f | %d\n",
                   name, t, sp, avg, fail);
        }
        free(student_scores);
    }
    printf("\nDone. Copy everything above and send it back.\n");
    return 0;
}

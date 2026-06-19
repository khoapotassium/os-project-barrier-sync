#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define NUM_STUDENTS 1000000
#define THREAD_COUNT 4

// --- GLOBAL DATA VARIABLES ---
float *student_scores;
double global_sum = 0.0;
int global_fail_count = 0;
pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex to protect global variables

// --- BARRIER STRUCTURE ---
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int thread_count;
    int waiting_count;
    int generation;
} Barrier;

typedef struct {
    int id;
    Barrier *barrier;
    int start_idx; // Starting index in the array
    int end_idx;   // Ending index in the array
} WorkerArgs;

static void barrier_init(Barrier *barrier, int thread_count) {
    pthread_mutex_init(&barrier->mutex, NULL);
    pthread_cond_init(&barrier->cond, NULL);
    barrier->thread_count = thread_count;
    barrier->waiting_count = 0;
    barrier->generation = 0;
}

static void barrier_destroy(Barrier *barrier) {
    pthread_mutex_destroy(&barrier->mutex);
    pthread_cond_destroy(&barrier->cond);
}

static void barrier_wait(Barrier *barrier, int thread_id, int phase) {
    pthread_mutex_lock(&barrier->mutex);

    int current_generation = barrier->generation;
    barrier->waiting_count++;

    printf("[Thread %d] reached barrier after Phase %d (%d/%d threads waiting)\n",
           thread_id, phase, barrier->waiting_count, barrier->thread_count);

    if (barrier->waiting_count == barrier->thread_count) {
        printf(">>> All threads reached barrier for Phase %d. Unlocking! <<<\n\n", phase);
        barrier->waiting_count = 0;
        barrier->generation++;
        pthread_cond_broadcast(&barrier->cond);
    } else {
        while (current_generation == barrier->generation) {
            pthread_cond_wait(&barrier->cond, &barrier->mutex);
        }
    }
    pthread_mutex_unlock(&barrier->mutex);
}

// --- THREAD LOGIC ---
static void *worker_run(void *arg) {
    WorkerArgs *worker = (WorkerArgs *)arg;
    
    // --- PHASE 1: Calculate local sum ---
    double local_sum = 0.0;
    for (int i = worker->start_idx; i < worker->end_idx; i++) {
        local_sum += student_scores[i];
    }
    
    // Accumulate to global sum (Critical Section)
    pthread_mutex_lock(&data_mutex);
    global_sum += local_sum;
    pthread_mutex_unlock(&data_mutex);
    
    // BARRIER WAIT 1: Wait for all threads to finish summing to get the final total
    barrier_wait(worker->barrier, worker->id, 1);

    // --- PHASE 2: Count students below average ---
    // Passed Barrier 1, global_sum is completely calculated
    double average = global_sum / NUM_STUDENTS;
    
    // Only let 1 thread print the average to keep the console clean
    if (worker->id == 1) {
        printf("=== OVERALL AVERAGE SCORE: %.2f ===\n\n", average);
    }

    int local_fail = 0;
    for (int i = worker->start_idx; i < worker->end_idx; i++) {
        if (student_scores[i] < average) {
            local_fail++;
        }
    }

    // Accumulate fail count to global variable (Critical Section)
    pthread_mutex_lock(&data_mutex);
    global_fail_count += local_fail;
    pthread_mutex_unlock(&data_mutex);

    // BARRIER WAIT 2: Synchronize before exiting
    barrier_wait(worker->barrier, worker->id, 2);

    return NULL;
}

int main() {
    srand((unsigned int)time(NULL));

    // 1. Initialize random score array
    printf("Initializing data for %d students...\n", NUM_STUDENTS);
    student_scores = (float *)malloc(sizeof(float) * NUM_STUDENTS);
    for (int i = 0; i < NUM_STUDENTS; i++) {
        student_scores[i] = (float)(rand() % 101) / 10.0; // Scores from 0.0 to 10.0
    }

    pthread_t threads[THREAD_COUNT];
    WorkerArgs args[THREAD_COUNT];
    Barrier barrier;

    barrier_init(&barrier, THREAD_COUNT);
    
    int chunk_size = NUM_STUDENTS / THREAD_COUNT;

    printf("Starting parallel processing with %d threads...\n\n", THREAD_COUNT);

    // 2. Partition data and create threads
    for (int i = 0; i < THREAD_COUNT; i++) {
        args[i].id = i + 1;
        args[i].barrier = &barrier;
        args[i].start_idx = i * chunk_size;
        // The last thread handles the remaining elements (if not evenly divisible)
        args[i].end_idx = (i == THREAD_COUNT - 1) ? NUM_STUDENTS : (i + 1) * chunk_size;

        pthread_create(&threads[i], NULL, worker_run, &args[i]);
    }

    // 3. Wait for all threads to finish (AFTER RETURN NULL)
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("=== FINAL RESULTS ===\n");
    printf("Total students: %d\n", NUM_STUDENTS);
    printf("Students below average: %d\n", global_fail_count);

    // 4. Cleanup
    barrier_destroy(&barrier);
    free(student_scores);

    return 0;
}
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <getopt.h>
#include <sys/types.h>

// variable declarations
int buffer_length;
int in = 0, out = 0; // indices of buffer
int num_producers;
int num_consumers;
int num_of_items;
char *buffer;
int total_items = 0;
int current_items = 0;

char rand_char() {
  char randomletter = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"[random() % 52];
}

// monitor struct definition
typedef struct{
    sem_t mutex;
    sem_t signal; // urgent queue in the zyBook example
    int num_signal; // number of processes/threads in the urgent queue
} monitor_t;

monitor_t monitor;

// condition variable struct definition
typedef struct{
    sem_t queue;
    int waiting_threads;
} cv_t;

cv_t full;
cv_t empty;

void monitor_init() {
   // Check the init values later, may have to change them
   sem_init(&monitor.mutex, 0, 1);
   sem_init(&monitor.signal, 0, 0);
}

int count(cv_t cv) {
    return cv.waiting_threads;
}

void wait(cv_t cv) {
    sem_wait(&cv.queue);
    cv.waiting_threads++;
}

void signal(cv_t cv) {
    sem_post(&cv.queue);
    cv.waiting_threads--;
}

void cv_init() {
    sem_init(&empty.queue, 0, buffer_length);
    empty.waiting_threads = 0;
    sem_init(&full.queue, 0, 1);
    full.waiting_threads = 0;
}


int produce() {
    if (total_items < num_of_items) {
        char random_char = rand_char();
        buffer[in] = random_char;
        in = (in + 1) % buffer_length;
        printf("p:%u, item: %c, at %d\n", pthread_self(), random_char, in);
        current_items++;
        total_items++;
        return 1;
    } else { return 0; }
}

int consume() {
    if (current_items != 0) {
        char item;
        item = buffer[out];
        out = (out + 1) % buffer_length;
        printf("c:%u, item: %c, at %d\n", pthread_self(), item, out);
        current_items--;
        return 1;
    } else { return 0; }
}

void *producer() {
    while(1){
        wait(empty);
        sem_wait(&monitor.mutex);
        int can_produce = produce();
        sem_post(&monitor.mutex);
        signal(full);
        if (can_produce == 0) {
            pthread_exit(0);
        }
    }
}

void *consumer() {
    while(1) {
        wait(full);
        sem_wait(&monitor.mutex);
        int can_consume = consume();
        sem_post(&monitor.mutex);
        signal(empty);
        if (can_consume == 0) {
            signal(empty);
            pthread_exit(0);
        }
    }
}



int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "b:p:c:i:")) != -1) {
        switch (opt) {
            case 'b':
                buffer_length = atoi(optarg);
                break;
            case 'p':
                num_producers = atoi(optarg);
                break;
            case 'c':
                num_consumers = atoi(optarg);
                break;
            case 'i':
                num_of_items = atoi(optarg);
                break;
        }
    }

    buffer = (char*)malloc(num_of_items * sizeof(char));

    cv_init();
    monitor_init();

    // create producer threads
    pthread_t producer_threads[num_producers];
    for (int i = 0; i < num_producers; i++) {
        pthread_create(&producer_threads[i], NULL, &producer, NULL);
    }

    // create consumer threads
    pthread_t consumer_threads[num_consumers];
    for (int i = 0; i < num_consumers; i++) {
        pthread_create(&consumer_threads[i], NULL, &consumer, NULL);
    }

    // join producer threads
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producer_threads[i], NULL);
    }

    // join consumer threads
    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumer_threads[i], NULL);
    }
}

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>

int buffer_length;
int front = 0, rear = 0; // indices of buffer
int num_producers = 1, num_consumers = 1;
int num_of_items;

// buffer struct definition
typedef struct {
    char *items;
    int total_count;
    int current_count;
    // pthread_mutex_t mutex;
    sem_t empty, full, mutex;
} buffer_t;

buffer_t buffer;

int produce() {
    if (buffer.total_count < num_of_items) {
        buffer.items[rear] = 'X';
        rear = (rear + 1) % buffer_length;
        printf("p:%u, item: X, at %d\n", pthread_self(), rear);
        buffer.current_count++;
        buffer.total_count++;
        return 1;
    } else {
        return 0;
    }
}

void* producer() {
    while (1) {
        sem_wait(&buffer.empty); // wait for an empty buffer
        sem_wait(&buffer.mutex); // get the mutex
        int can_produce = produce();
        sem_post(&buffer.mutex);
        sem_post(&buffer.full); // signal a full spot in buffer
        if (can_produce == 0) {
            sem_post(&buffer.full); // signal a full spot in buffer
            // int mutex, empty, full;
            // printf("Producer %d exiting\n", number);
            // sem_getvalue(&buffer.mutex, &mutex);
            // sem_getvalue(&buffer.empty, &empty);
            // sem_getvalue(&buffer.full, &full);
            // printf("Mutex: %d\nEmpty: %d\nFull: %d\n", mutex, empty, full);
            pthread_exit(0);
        }
        // printf("*Producer* num_of_items: %d\n", num_of_items);
    }
}

int consume() {
    if (buffer.current_count != 0) {
        char item;
        item = buffer.items[front];
        front = (front + 1) % buffer_length;
        printf("c:%u, item: %c, at %d\n", pthread_self(), item, front);
        buffer.current_count--;
        return 1;
    } else {
        return 0;
    }
}

void* consumer() {
    while (1) {
        sem_wait(&buffer.full); // wait for a full buffer
        sem_wait(&buffer.mutex); // get the mutex
        int can_consume = consume();
        sem_post(&buffer.mutex); // release the mutex
        sem_post(&buffer.empty); // signal an empty spot in buffer
        if (can_consume == 0) {
            sem_post(&buffer.empty);
            // int mutex, empty, full;
            // printf("Consumer %d exiting\n", number);
            // printf("Sem Values\n----------\n");
            // sem_getvalue(&buffer.mutex, &mutex);
            // sem_getvalue(&buffer.empty, &empty);
            // sem_getvalue(&buffer.full, &full);
            // printf("Mutex: %d\nEmpty: %d\nFull: %d\n", mutex, empty, full);
            pthread_exit(0);
        }
    }
}

void init() {
    // *item may can be defined using malloc()
    buffer.items = (char*)malloc(num_of_items * sizeof(char));
    buffer.total_count = 0;

    sem_init(&buffer.empty, 0, buffer_length); // initialize the empty semaphore
    sem_init(&buffer.full, 0, 0); // initialize the full semaphore
    sem_init(&buffer.mutex, 0, 1); // initialize the mutex
}

int main(int argc, char* argv[]) {
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

    init();

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

    sem_destroy(&buffer.empty); // destroy the empty semaphore
    sem_destroy(&buffer.full); // destroy the full semaphore
    sem_destroy(&buffer.mutex); // destroy the mutex semaphore

    return 0;
}

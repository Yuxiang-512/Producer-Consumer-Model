#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <limits.h>
#include <time.h>
#include <stdbool.h>

//Define values
#define Max_producers 4
#define Max_consumers 4
#define Producers_wait 5
#define Consumers_wait 5
#define Max_queue_size 10

//Global variables
int queue[Max_queue_size];
int size = 0;
bool timeout = false;
int timeout_value = 30;

//Mutex and condition variables
pthread_mutex_t mutex;
pthread_cond_t can_produce;
pthread_cond_t can_consume;

time_t start_time;

void log_with_timestamp(const char *message) {
    time_t now = time(NULL);
    char buffer[20];
    struct tm *tm_info = localtime(&now);

    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", tm_info); //set the time format
    printf("[%s] %s\n", buffer, message);
}

//Producer function
void* producer(void* arg) {
    int item;
    log_with_timestamp("Producer started.");

    while (1) {
        if (difftime(time(NULL),start_time) > timeout_value) {
            log_with_timestamp("Producer timeout, exiting...");
            break;
        }
        //Produce an item
        item = rand() % 100;

        //Acquire the mutex lock
        pthread_mutex_lock(&mutex);

        //Check if the queue is full
        while (size == Max_queue_size){
            pthread_cond_wait(&can_produce, &mutex);
        }

        //Add item to the queue
        queue[size++] = item;

        //Signal to consumer
        pthread_cond_signal(&can_consume);

        //Release the mutex lock
        pthread_mutex_unlock(&mutex);

        char log_message[50]; 
        sprintf(log_message, "Producer produced: %d", item);
        log_with_timestamp(log_message);


        //Wait for a bit before producing next item
        sleep(rand() % Producers_wait);
    }
    return NULL;
}

//Consumer function
void* consumer(void* arg) {
    int item;
    log_with_timestamp("Consumer started.");
    while (1) {
        if (difftime(time(NULL),start_time) > timeout_value)  {
            log_with_timestamp("Consumer timeout, exiting...");
            break;
        }
        //Acqure hte mutex lock
        pthread_mutex_lock(&mutex);

        //check if the queue is empty
        while (size == 0) {
            pthread_cond_wait(&can_consume, &mutex);
        }

        //Remove item from the queue
        item = queue[--size];

        //Signal to producer
        pthread_cond_signal(&can_produce);

        //Release the mutex lock
        pthread_mutex_unlock(&mutex);

        //record consume product
        char log_message[50];
        sprintf(log_message, "Consumer consumed: %d", item);
        log_with_timestamp(log_message);


        //Wait for a bit before consuming next item
        sleep(rand() % Consumers_wait);
    }
    return NULL;
}

void* check_timeout(void *arg) {
    int timeout_duration = *((int*) arg); // Set the time limit

    time_t start_time, current_time;
    time(&start_time);

    while (true) {
        time(&current_time);
        if (difftime(current_time, start_time) >= timeout_duration) {
            timeout = true; // set timeout
            log_with_timestamp("Timeout reached. Stopping all threads.\n");
            break;
        }
        
        sleep(1); // check every second
        if (timeout) {
        // arouse all the threads which are waiting
        pthread_cond_broadcast(&can_produce);
        pthread_cond_broadcast(&can_consume);
        }
    }

    return NULL;
}
int main(int argc, char* argv[]) {
    log_with_timestamp("Program started.");

    // introduce command parameter
    if (argc < 4) {
        fprintf(stderr, "Please input: %s <num_producers> <num_consumers> <timeout>\n", argv[0]);
        return 1;
    }

    int num_producers = atoi(argv[1]);
    int num_consumers = atoi(argv[2]);
    timeout_value = atoi(argv[3]);

    time(&start_time); // set the begain time

    // initiallize the mutex and conditional varible
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&can_produce, NULL);
    pthread_cond_init(&can_consume, NULL);

    // create threads

    pthread_t producers[num_producers], consumers[num_consumers], timeout_thread;

    pthread_create(&timeout_thread, NULL, check_timeout, &timeout_value);
    for (int i = 0; i < num_producers; i++) {
        pthread_create(&producers[i], NULL, producer, NULL);
    }
    for (int i = 0; i < num_consumers; i++) {
        pthread_create(&consumers[i], NULL, consumer, NULL);
    }

    // Wait for 
    pthread_join(timeout_thread, NULL);
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producers[i], NULL);
    }
    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumers[i], NULL);
    }

    // clean up
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&can_produce);
    pthread_cond_destroy(&can_consume);

    log_with_timestamp("Program finished.");

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#define NUM_STOOLS 8

unsigned int num_patrons;
sem_t counter_sem;
pthread_mutex_t mutex;
unsigned int total_wait_time = 0;

typedef struct 
{
    int id;
    unsigned int wait_time;
} Patron;

typedef struct 
{
    Patron* patrons;
    unsigned int num_patrons;
    unsigned int current_patron;
} PatronQueue;

void* patron_thread(void* arg) 
{
    PatronQueue* queue = (PatronQueue*)arg;

    while (1) 
    {
        pthread_mutex_lock(&mutex);

        if (queue->current_patron >= queue->num_patrons) 
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        unsigned int current_patron_index = queue->current_patron;

        Patron current_patron = queue->patrons[current_patron_index];
        queue->current_patron++;
        pthread_mutex_unlock(&mutex);

        // Sleep for the wait time in microseconds
        usleep(current_patron.wait_time * 1000);  

        // Try to acquire a stool at the counter
        sem_wait(&counter_sem);   

        printf("Patron #%d is eating!\n", current_patron.id);

        pthread_mutex_lock(&mutex);

        total_wait_time += current_patron.wait_time;

        pthread_mutex_unlock(&mutex);

        // Release the stool at the counter
        sem_post(&counter_sem);  
    }

    return NULL;
}

int main(int argc, char * argv[]) 
{
    if(argc != 2) 
    {
        printf("You must input the number of patrons!\n");
        exit(-1);
    }
    num_patrons = atoi(argv[1]);

    srand(time(NULL));

    // Initialize the semaphore with the number of stools
    sem_init(&counter_sem, 0, NUM_STOOLS);  
    pthread_mutex_init(&mutex, NULL);

    PatronQueue queue;
    queue.patrons = (Patron*)malloc(num_patrons * sizeof(Patron));
    queue.num_patrons = num_patrons;
    queue.current_patron = 0;

    for (unsigned int i = 0; i < num_patrons; i++) 
    {
        queue.patrons[i].id = i + 1;
        queue.patrons[i].wait_time = rand() % 200;  // Generate a random wait time between 0 and 199 ms
    }

    pthread_t* threads = (pthread_t*)malloc(num_patrons * sizeof(pthread_t));

    for (unsigned int i = 0; i < num_patrons; i++) 
    {
        pthread_create(&threads[i], NULL, patron_thread, &queue);
    }

    for(unsigned int i = 0; i < num_patrons;i++)
    {
        pthread_join(threads[i], NULL);
    }

    // loop to print waiting time of each patron
    for (unsigned int i = 0; i < num_patrons; i++) 
    {
        printf("Patron #%d waited %d ms\n", i + 1, queue.patrons[i].wait_time);
    }

    // Calculating the average wait time
    unsigned int average_wait_time = total_wait_time / num_patrons;

    printf("Average wait time: %u ms\n", average_wait_time);
    printf("num_patrons: %d\n", num_patrons);

    //Freeing memory
    free(threads);
    free(queue.patrons);

    sem_destroy(&counter_sem);
    pthread_mutex_destroy(&mutex);

    return 0;
}
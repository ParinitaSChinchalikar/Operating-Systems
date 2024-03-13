#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_MONKS 10

unsigned int num_mugs, num_kegs;
unsigned int total_beer_consumed = 0;
unsigned int monks_finished = 0; // Number of monks that have finished drinking
sem_t keg_sem, mug_sem;
pthread_mutex_t keg_mutex, finish_mutex;   // Mutex for synchronizing termination

// Array to track each monk's beer consumption
unsigned int monk_beer_count[NUM_MONKS];

void* monk_thread(void* arg) 
{
    unsigned int monk_id = *((unsigned int*)arg);

    while (1) 
    {
        sem_wait(&mug_sem); 
        pthread_mutex_lock(&keg_mutex);
        if (total_beer_consumed >= num_mugs * num_kegs) 
        {
            pthread_mutex_unlock(&keg_mutex);
            sem_post(&mug_sem);
            break;    
        }
        total_beer_consumed++;
        monk_beer_count[monk_id - 1]++;
        printf("Monk #%u drank!\n", monk_id);
        pthread_mutex_unlock(&keg_mutex);
        sem_post(&keg_sem);
    }

    pthread_mutex_lock(&finish_mutex);
    monks_finished++;
    pthread_mutex_unlock(&finish_mutex);

    return NULL;
}

void* brew_master_thread(void* arg) 
{
    unsigned int keg_capacity = num_mugs;
    unsigned int mugs_in_keg = keg_capacity;

    while (1) 
    {
        sem_wait(&keg_sem);  
        pthread_mutex_lock(&keg_mutex);
        if (total_beer_consumed >= num_mugs * num_kegs) 
        {
            pthread_mutex_unlock(&keg_mutex);
            break;   
        }
        if (mugs_in_keg == 0) 
        {
            mugs_in_keg = keg_capacity;
        }
        mugs_in_keg--;
        pthread_mutex_unlock(&keg_mutex);
        sem_post(&mug_sem); 
    }

    // Wait until all Monk threads have finished drinking
    while (1) 
    {
        pthread_mutex_lock(&finish_mutex);
        if (monks_finished >= NUM_MONKS) 
        {
            pthread_mutex_unlock(&finish_mutex);
            break;
        }
        pthread_mutex_unlock(&finish_mutex);
    }

    return NULL;
}

int main(int argc, char * argv[]) 
{
    if(argc != 3) 
    {
        printf("You must input number of mugs per keg and number of kegs\n");
        exit(-1);
    }
    num_mugs = atoi(argv[1]);
    num_kegs = atoi(argv[2]);

    // Initialize the semaphore with value 0
    sem_init(&keg_sem, 0, 0); 

    // Initialize the semaphore with the number of mugs
    sem_init(&mug_sem, 0, num_mugs); 
    pthread_mutex_init(&keg_mutex, NULL);
    pthread_mutex_init(&finish_mutex, NULL);

    pthread_t monks[NUM_MONKS];
    unsigned int monk_ids[NUM_MONKS];

    pthread_t brew_master;
    
    for (unsigned int i = 0; i < NUM_MONKS; i++) 
    {
        monk_ids[i] = i + 1;

        // Initialize beer count for each monk
        monk_beer_count[i] = 0;   
        pthread_create(&monks[i], NULL, monk_thread, &monk_ids[i]);
    }

    pthread_create(&brew_master, NULL, brew_master_thread, NULL);

    for (unsigned int i = 0; i < NUM_MONKS; i++) 
    {
        pthread_join(monks[i], NULL);
    }

    pthread_join(brew_master, NULL);

    //Destroying semaphores and mutexes 
    sem_destroy(&keg_sem);
    sem_destroy(&mug_sem);
    pthread_mutex_destroy(&keg_mutex);
    pthread_mutex_destroy(&finish_mutex);

    // Print individual beer consumption for each monk
    for (unsigned int i = 0; i < NUM_MONKS; i++) 
    {
        printf("Monk #%u drank %u mugs of beer\n", i + 1, monk_beer_count[i]);
    }

    printf("The monks drank %u mugs of beer!\n", total_beer_consumed);

    return 0;
}
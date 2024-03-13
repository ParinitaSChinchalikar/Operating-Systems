#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

unsigned int num_family_members;
sem_t *chopsticks;

// Family members start talking by sleeping for random time 
void talk()
{
	usleep(rand() % 1000000 );  
}

void get_chopsticks(unsigned int left, unsigned int right)
{
	// get the left chopstick
	sem_wait(&chopsticks[left]);  

	// get the right chopstick
	sem_wait(&chopsticks[right]);  
}

void put_chopsticks(unsigned int left, unsigned int right)
{
	// put the left chopstick
	sem_post(&chopsticks[left]);  

	// put the right chopstick
	sem_post(&chopsticks[right]);  
}

void *eat(void *number)
{
	unsigned int member_id = *((unsigned int *) number);

	while(1)
	{
		talk();

		get_chopsticks(member_id, (member_id + 1)%num_family_members ); 

		//Print family member id while eating 
		printf("Family member #%u is eating !!\n", member_id);  

		put_chopsticks(member_id, (member_id + 1)%num_family_members);
	}

	return NULL;

}

int main(int argc, char * argv[])
{
	if(argc != 2)
	{
		printf("You must input the number of family members!\n");
		exit(-1);
	}
	num_family_members = atoi(argv[1]);

	chopsticks = (sem_t *)malloc(sizeof(sem_t)*num_family_members);

	srand(time(NULL));

	//Initializing chopsticks as semaphores
	for (unsigned int i = 0; i < num_family_members; i++)
	{
		sem_init(&chopsticks[i], 0, 1);
	}

	pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * num_family_members);

	unsigned int *member_ids = (unsigned int * )malloc(sizeof(unsigned int ) * num_family_members);

	//Creating threads for each family member
	for (unsigned int i = 0; i < num_family_members; i++)
	{
		member_ids [i] = i;
		pthread_create(&threads[i], NULL, eat, &member_ids[i] );
	}

	//Calling pthread_join which allows thread to wait till finish 
	for (unsigned int  i = 0; i < num_family_members; i++)
	{
		pthread_join (threads[i], NULL);
	}
	
	//Freeing memory 
	free(threads);
	free(member_ids);
	free(chopsticks);

	return 0;
}
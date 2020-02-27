/******************************************************************
 * The Main program with the two functions. A simple
 * example of creating and using a thread is provided.
 ******************************************************************/

#include "helper.h"
#include <sys/sem.h>
#include <stdlib.h>
#include<iostream>

using namespace std;

void *producer (void *id);
void *consumer (void *id);

/*
 * Queue structure which acts as the circular queue
 */
struct Queue {
  int tail, head, size;
  int* array;

  /*
   * Queue constructor - sets the head and tail of queue and initialises the 
   * internal array
   * Input: size of circular queue
   * Output: Queue*
   */
  Queue(int s) {
    size = s;
    array = new int[s];
    tail = -1;
    head = -1;
  }

  /*
   * pop_head - removes the first element of the queue and returns it, also 
   * notifies you if queue is empty
   * Input: none
   * Output: first element of array (int)
   */
  int pop_head() {
    if (head == -1) {
      fprintf(stderr, "queue is empty");
      return 0;
    }

    int value = array[head];
    array[head] = -1;
    if (head == tail) {
      head = -1;
      tail = -1;
    }
    else if (head == size-1)
      head = 0;
    else
      head++;

    return value;
  };

  /*
   * push_head - adds given value to the end of the queue and returns it, also 
   * notifies you if queue is full
   * Input: value to add
   * Output: index of added value
   */
  int push_tail(int value) {
    if (head==0 && tail==(size-1))
      fprintf(stderr, "queue is full");
    else if (head == -1)
      head = tail = 0;
    else if (head != 0 && tail == size-1)
      tail = 0;
    else
      tail++;

    array[tail] = value;
    return tail;
  };

  ~Queue() {
    delete [] array;
  }

};

struct DataStruct {
  int number_of_jobs_per_producer;
  int number_of_producers;
  int number_of_producers_finished = 0;
  int semaphore_id;
  bool finished = false;
  Queue* queue;
  int actor_id;

  /*
   * producer_finished - increments the producer finished count by one and 
   * checks if all producers are finished
   * Input: none
   * Output: none
   */
  void producer_finished() {
    number_of_producers_finished++;
    finished = (number_of_producers == number_of_producers_finished);
  };

  ~DataStruct() {
    delete queue;
  }
};



int main (int argc, char **argv)
{
  
  /*
   * Argument parsing
   */
  if (argc != (EXPECTED_NUM_ARGUMENTS+1)) {
    fprintf(stderr, "%d arguments given, 4 were expected\n", argc-1);
    sem_error();
  }
  int arguments[4];
  int argument_value;
  for (int arg_index=0; arg_index < EXPECTED_NUM_ARGUMENTS; arg_index++) {
    argument_value = check_arg(argv[arg_index+1]);
    if (argument_value < 0) {
      fprintf(stderr, 
	      "You have an error with argument %d," 
	      "make sure it is an integer above 0\n", 
	      arg_index+1);
      sem_error();
    }
    arguments[arg_index] = argument_value;
  }
  int size_of_queue = arguments[0];
  int number_of_jobs_per_producer = arguments[1];
  int number_of_producers = arguments[2];
  int number_of_consumers = arguments[3];

  /*
   * Initialising central_data_struct
   */
  DataStruct* central_data_struct = new DataStruct;
  central_data_struct->queue = new Queue(size_of_queue);
  central_data_struct->number_of_jobs_per_producer = 
    number_of_jobs_per_producer;
  central_data_struct->number_of_producers = number_of_producers;

  /*
   * Sephamore Creation
   */
  key_t key = SEM_KEY;
  int semaphore_id = sem_create(key, 3);
  if (semaphore_id == -1) {
    fprintf(stderr, "Error creating semaphores\n");
    sem_error(semaphore_id, true);
  }
  int sem_values[3] = {1, size_of_queue, 0}; 
  for (int i=0; i<3; i++) {
    int err_code = sem_init(semaphore_id, i, sem_values[i]);
    if (err_code == -1) {
      fprintf(stderr, "Error initialising semaphore %d\n", i);
      sem_error(semaphore_id, true);
    }
  }
  central_data_struct->semaphore_id = semaphore_id;

  /*
   * Creating thread array
   */
  pthread_t threads[number_of_producers + number_of_consumers];

  
  /*
   * Creating producers
   */
  int producer_index = 0;
  for (;producer_index < number_of_producers; producer_index++) {
    pthread_t producerid;

    if(sem_wait(semaphore_id, 0) == -1) sem_error(semaphore_id, true);    
    central_data_struct->actor_id = producer_index + 1;
    if(sem_signal(semaphore_id, 0) == -1) sem_error(semaphore_id, true);

    if(sem_wait(semaphore_id, 0) == -1) sem_error(semaphore_id, true);    
    pthread_create (&producerid, NULL, producer, (void *) central_data_struct);

    threads[producer_index] = producerid;
  }

  /*
   * Creating consumers
   */
  int consumer_index = 0;
  for (; consumer_index < number_of_consumers; consumer_index++) {
    pthread_t consumerid;

    if(sem_wait(semaphore_id, 0) == -1) sem_error(semaphore_id, true);    
    central_data_struct->actor_id = consumer_index + 1;
    if(sem_signal(semaphore_id, 0) == -1) sem_error(semaphore_id, true);

    if(sem_wait(semaphore_id, 0) == -1) sem_error(semaphore_id, true);
    pthread_create (&consumerid, NULL, consumer, (void *) central_data_struct);

    threads[consumer_index+producer_index] = consumerid;
  }

  for (int thread_index = 0; 
       thread_index < (consumer_index + producer_index); 
       thread_index++) {
    pthread_join(threads[thread_index], NULL);
  }

  
  if(sem_close(semaphore_id) == -1) sem_error(semaphore_id, true);

  delete central_data_struct;

  return 0;
}

void *producer (void *parameter) 
{
  DataStruct *param = (DataStruct *) parameter;
  int id = param->actor_id;
  int item, job_id, sleep_time, tasks_produced = 0;
  if (sem_signal(param->semaphore_id, 0) == -1) 
	  sem_error(param->semaphore_id, true);
 
  while(tasks_produced < param->number_of_jobs_per_producer) {
    item = rand() % 10 + 1;
    if (sem_wait_with_timeout(param->semaphore_id, 1) == -1) {
      fprintf(stderr, "Producer (%i): timeout \n", id);
      pthread_exit((void*) -1);
    }
    if (sem_wait(param->semaphore_id, 0) == -1) 
	    sem_error(param->semaphore_id, true);
    job_id = param->queue->push_tail(item);
    if (sem_signal(param->semaphore_id, 0) == -1) 
	    sem_error(param->semaphore_id, true);
    if (sem_signal(param->semaphore_id, 2) == -1) 
	    sem_error(param->semaphore_id, true);
    tasks_produced++;
    fprintf(stderr, 
	    "Producer (%i): Job id %i duration %i \n", 
	    id, 
	    job_id, 
	    item);
    sleep_time = rand() % 5 + 1;
    sleep(sleep_time); 
  }
  param->producer_finished();

  fprintf(stderr, "Producer (%d): No more jobs to generate.\n", id);
    
  pthread_exit(0);
}


void *consumer (void *parameter) 
{
  DataStruct *param = (DataStruct *) parameter;
  int id = param->actor_id;
  int item, job_id;
  if(sem_signal(param->semaphore_id, 0) == -1) 
	  sem_error(param->semaphore_id, true);
  
  while(true) {
    if (sem_wait_with_timeout(param->semaphore_id, 2) == -1) {
      if (param->finished) {
	fprintf(stderr, "Consumer (%i): No more jobs left.\n", id);
	pthread_exit((void*) 0);
      }
      fprintf(stderr, "Consumer (%i): timeout \n", id);
      pthread_exit((void*) -1);
    }
    if(sem_wait(param->semaphore_id, 0) == -1) 
	    sem_error(param->semaphore_id, true);
    job_id = param->queue->head;
    item = param->queue->pop_head();
    if(sem_signal(param->semaphore_id, 0) == -1) 
	    sem_error(param->semaphore_id, true);
    if(sem_signal(param->semaphore_id, 1) == -1) 
	    sem_error(param->semaphore_id, true);

    fprintf(stderr, 
	    "Consumer (%d): Job id %d executing " 
	    "sleep duration %d \n", 
	    id, 
	    job_id, 
	    item);
    sleep(item);
    fprintf(stderr, "Consumer (%d): Job id %d completed\n", id, job_id);
  }

  pthread_exit(0);

}

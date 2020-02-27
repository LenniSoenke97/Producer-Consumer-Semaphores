/******************************************************************
 * The helper file that contains the following helper functions:
 * check_arg - Checks if command line input is a number and returns it
 * sem_create - Create number of sempahores required in a semaphore array
 * sem_init - Initialise particular semaphore in semaphore array
 * sem_wait - Waits on a semaphore (akin to down ()) in the semaphore array
 * sem_signal - Signals a semaphore (akin to up ()) in the semaphore array
 * sem_close - Destroy the semaphore array
 * sem_wait_till_timeout - Wait on a semaphore in semaphore array but returns -1
 * if it does not proceed within a set amount of time (given by global constant)
 * TIME_UNTIL_TIMEOUT
 * sem_error - destroys semaphore array, outputs error, exits with -1
 ******************************************************************/

# include "helper.h"
# include <errno.h>
# include <string.h>

int check_arg (char *buffer)
{
  int i, num = 0, temp = 0;
  if (strlen (buffer) == 0)
    return -1;
  for (i=0; i < (int) strlen (buffer); i++)
  {
    temp = 0 + buffer[i];
    if (temp > 57 || temp < 48)
      return -1;
    num += pow (10, strlen (buffer)-i-1) * (buffer[i] - 48);
  }
  return num;
}

int sem_create (key_t key, int num)
{
  int id;
  if ((id = semget (key, num,  0666 | IPC_CREAT | IPC_EXCL)) < 0)
    return -1;
  return id;
}

int sem_init (int id, int num, int value)
{
  union semun semctl_arg;
  semctl_arg.val = value;
  if (semctl (id, num, SETVAL, semctl_arg) < 0)
    return -1;
  return 0;
}

int sem_wait (int id, short unsigned int num)
{
  struct sembuf op[] = {
    {num, -1, SEM_UNDO}
  };
  return semop (id, op, 1);
}

int sem_signal (int id, short unsigned int num)
{
  struct sembuf op[] = {
    {num, 1, SEM_UNDO}
  };
  return semop (id, op, 1);
}

int sem_close (int id)
{
  if (semctl (id, 0, IPC_RMID, 0) < 0)
    return -1;
  return 0;
}

int sem_wait_with_timeout(int id, short unsigned int num) {
  struct sembuf sops[] = {
    {num, -1, SEM_UNDO}
  };
  struct timespec timeout[] = {TIME_UNTIL_TIMEOUT, 0};
  return semtimedop(id, sops, 1, timeout);
}

void sem_error(int semaphore_id, bool sem_error) {
  if (semaphore_id) sem_close(semaphore_id);
  if (sem_error) fprintf(stderr, 
		  "Error with semaphore %i - %s \n", 
		  semaphore_id, 
		  strerror(errno));
  exit(-1);
}

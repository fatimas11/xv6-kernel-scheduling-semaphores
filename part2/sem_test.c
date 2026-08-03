#include "kernel/types.h"
#include "user/user.h"

void
worker(int sem_id, int child_id)
{
  int pid = getpid();
  
  // 1. Attempt to acquire the semaphore
  // If count is 0, this will block until a resource is freed.
  if(sem_down(sem_id) < 0){
    printf("Child %d: sem_down failed\n", pid);
    exit(1);
  }

  // 2. Critical Section (Simulate heavy work)
  printf("[Child %d] Acquired resource. (Doing work...)\n", pid);
  
  // Sleep for 20 ticks to simulate a task and hold the resource
  // allowing us to see if other children are blocked.
  sleep(20);

  // 3. Release the semaphore
  printf("[Child %d] Released resource.\n", pid);
  if(sem_up(sem_id) < 0){
    printf("Child %d: sem_up failed\n", pid);
    exit(1);
  }

  exit(0);
}

int
main(int argc, char *argv[])
{
  int sem_id;
  int i;
  int pid;
  int num_children = 3;
  int init_value = 2; // Only 2 resources available

  printf("[Parent] Creating semaphore with value %d.\n", init_value);

  // Initialize semaphore with 2 resources
  sem_id = sem_create(init_value);
  if(sem_id < 0){
    printf("[Parent] Error: Could not create semaphore.\n");
    exit(1);
  }

  // Fork 3 children
  for(i = 0; i < num_children; i++){
    pid = fork();
    if(pid < 0){
      printf("[Parent] Error: Fork failed.\n");
      exit(1);
    }

    if(pid == 0){
      // Child process
      worker(sem_id, i);
    }
  }

  // Parent waits for all children to finish
  for(i = 0; i < num_children; i++){
    wait(0);
  }

  // Cleanup
  if(sem_free(sem_id) < 0){
    printf("[Parent] Error: Could not free semaphore.\n");
  } else {
    printf("[Parent] Semaphore freed. Test finished.\n");
  }

  exit(0);
}
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

struct sem {
  struct spinlock lock;  // Protects this semaphore's value
  int value;             // Number of resources available
  int used;              // Is this slot currently allocated?
  int holders;            // Processes holding the semaphore
  int sleepers;           // Processes sleeping in sem_down
};

struct sem sems[MAX_SEMS];
struct spinlock sem_array_lock;

void
seminit(void)
{
  initlock(&sem_array_lock, "sem_array_lock");
  for(int i = 0; i < MAX_SEMS; i++){
    initlock(&sems[i].lock, "sem");
    sems[i].used = 0;
    sems[i].value = 0;
    sems[i].holders = 0;
    sems[i].sleepers = 0;
  }
}

int
sem_create(int value)
{
  if (value < 0) return -1;
    acquire(&sem_array_lock);
    for(int i=0 ; i<MAX_SEMS; i++) {
        acquire(&sems[i].lock);
        if (sems[i].used == 0) {
            sems[i].used = 1;
            sems[i].value = value;
            sems[i].holders = 0;
            sems[i].sleepers = 0;
            release(&sem_array_lock);
            release(&sems[i].lock);
            return i;
        }
        release(&sems[i].lock);
    }
    release(&sem_array_lock);
    return -1; 
}

int
sem_down(int sem_id)
{
    if (sem_id < 0 || sem_id >= MAX_SEMS) {
      return -1;
    }
    struct sem *s = &sems[sem_id];
    acquire(&s->lock);

    if (s->used == 0) {
          release(&s->lock);
          return -1;
    }

    while(s->value == 0) {
        s->sleepers++;
        sleep(s, &s->lock);
        s->sleepers--;
        // this line is for debugging
        // printf("[sem %d] sleep : (pid %d)\n\n", sem_id , myproc()->pid);
    }
    s->value --;
    s->holders++;
    // this line is for debugging
    // printf("[sem %d] down: value=%d (pid %d)\n\n", sem_id, s->value, myproc()->pid);
    release(&s->lock);
    return 0;
}

int
sem_up(int sem_id)
{
    if (sem_id < 0 || sem_id >= MAX_SEMS) {
      return -1;
    }
    struct sem *s = &sems[sem_id];

    acquire(&s->lock);
    s->value ++;
    s->holders--;
    if (s->holders < 0) {
      s->holders = 0;
    }

    wakeup(s); 
    release(&s->lock);
    // this line is for debugging
    // printf("[sem %d] up: value=%d (pid %d)\n\n", sem_id, s->value, myproc()->pid);
    return 0;
}

int
sem_free(int sem_id)
{
    if(sem_id < 0 || sem_id >= MAX_SEMS)
        return -1;

    struct sem *s = &sems[sem_id];
    acquire(&sem_array_lock);
    acquire(&s->lock);

    // If semaphore is in use, return -1
    if (s->holders > 0 || s->sleepers > 0) {
      release(&s->lock);
      release(&sem_array_lock);
      return -1;
    }

    s->used = 0;
    release(&s->lock);
    release(&sem_array_lock);
    return 0;
}

uint64
sys_sem_create(void) {
  int value;
  argint(0, &value);
  return sem_create(value);
}

uint64
sys_sem_free(void) {
  int sem_id;
  argint(0, &sem_id);
  return sem_free(sem_id);
}

uint64
sys_sem_down(void) {
  int sem_id;
  argint(0, &sem_id);
  return sem_down(sem_id);
}

uint64
sys_sem_up(void) {
  int sem_id;
  argint(0, &sem_id);
  return sem_up(sem_id);
}

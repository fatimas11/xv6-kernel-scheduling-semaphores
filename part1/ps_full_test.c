#include "kernel/types.h"
#include "user/user.h"

#define BUSY 400000000
volatile int sink;

#define RED   "\033[31m"
#define GREEN "\033[32m"
#define RESET "\033[0m"

void cpu_work(void) {
  for(int i = 0; i < BUSY; i++)
    sink++;
}

/* -----------------------------
 * TEST 1: Default Priority
 * ----------------------------- */
void test_default_priority(void) {
  printf("\n[TEST 1] Default priority (sanity)\n");

  int pid = fork();
  if(pid == 0){
    cpu_work();
    exit(0);
  }

  wait(0);
  printf(GREEN "[PASS] Process ran without setting priority (default supposed to be 5)\n" RESET);
}

/* -----------------------------
 * TEST 2: Syscall validation
 * ----------------------------- */
void test_syscall_validation(void) {
  printf("\n[TEST 2] set_ps_priority validation\n");

  if(set_ps_priority(0) != -1 ||
     set_ps_priority(11) != -1 ||
     set_ps_priority(5) != 0){
    printf(RED "[FAIL] set_ps_priority validation failed\n" RESET);
    exit(1);
  }

  printf(GREEN "[PASS] set_ps_priority input validation correct\n" RESET);
}

/* ---------------------------------
 * TEST 3: Rule A – no charge on sleep
 * --------------------------------- */
void test_no_charge_on_sleep(void){
    printf("\n[TEST 3] Rule A – no charge on sleep\n");

    int p[2];
    pipe(p);

    int pid = fork();
    if(pid == 0){
        // voluntary sleep
        sleep(30);

        // measure CPU-only work
        int s = uptime();
        cpu_work();
        int e = uptime();
        int cpu_ticks = e - s;

        write(p[1], &cpu_ticks, sizeof(cpu_ticks));
        exit(0);
    }

    int cpu_ticks;
    read(p[0], &cpu_ticks, sizeof(cpu_ticks));
    wait(0);

    printf("[INFO] child CPU-only ticks = %d\n", cpu_ticks);

    // threshold depends on BUSY loop size
    if(cpu_ticks > 1000){  // adjust depending on BUSY
        printf(RED "[FAIL] Sleeping process was charged accumulator\n");
        exit(1);
    }

    printf(GREEN "[PASS] Sleeping process not charged\n" RESET);
}


/* -----------------------------
 * TEST 4: Rule B – Fork fairness
 * ----------------------------- */
void test_fork_fairness(void) {
  printf("\n[TEST 4] Rule B – fork fairness\n");

  int p[2];
  pipe(p);

  set_ps_priority(10);
  cpu_work();
  cpu_work();

  int pid = fork();
  if(pid == 0){
    set_ps_priority(1);
    int s = uptime();
    cpu_work();
    int e = uptime();
    int r = e - s;
    write(p[1], &r, sizeof(r));
    exit(0);
  }

  int ps = uptime();
  cpu_work();
  int pe = uptime();
  int parent_runtime = pe - ps;

  int child_runtime;
  read(p[0], &child_runtime, sizeof(child_runtime));
  wait(0);

  printf("[PARENT] runtime = %d\n", parent_runtime);
  printf("[CHILD]  runtime = %d\n", child_runtime);

  if(child_runtime * 4 < parent_runtime){
    printf(RED "[FAIL] Child monopolized CPU after fork\n" RESET);
    exit(1);
  }

  printf(GREEN "[PASS] Fork fairness preserved\n" RESET);
}

/* -----------------------------
 * TEST 5: Rule B – Wakeup fairness
 * ----------------------------- */
void test_wakeup_fairness(void) {
  printf("\n[TEST 5] Rule B – wakeup fairness\n");

  int p[2];
  pipe(p);

  set_ps_priority(10);
  cpu_work();
  cpu_work();

  int pid = fork();
  if(pid == 0){
    sleep(30);
    set_ps_priority(1);
    int s = uptime();
    cpu_work();
    int e = uptime();
    int r = e - s;
    write(p[1], &r, sizeof(r));
    exit(0);
  }

  int ps = uptime();
  cpu_work();
  int pe = uptime();
  int parent_runtime = pe - ps;

  int child_runtime;
  read(p[0], &child_runtime, sizeof(child_runtime));
  wait(0);

  printf("[PARENT] runtime = %d\n", parent_runtime);
  printf("[CHILD]  runtime = %d\n", child_runtime);

  if(child_runtime * 4 < parent_runtime){
    printf(RED "[FAIL] Woken process unfairly dominated CPU\n" RESET);
    exit(1);
  }

  printf(GREEN "[PASS] Wakeup fairness preserved\n" RESET);
}

/* -----------------------------
 * TEST 6: Rule C – Scheduler order
 * ----------------------------- */
void test_scheduler_order(void) {
  printf("\n[TEST 6] Scheduler order by accumulator\n");

  int prio[3] = {1, 5, 10};
  int runtime[3];
  int p[2];
  pipe(p);

  for(int i = 0; i < 3; i++){
    if(fork() == 0){
      set_ps_priority(prio[i]);
      int s = uptime();
      cpu_work();
      int e = uptime();
      int r = e - s;
      write(p[1], &r, sizeof(r));
      exit(0);
    }
  }

  for(int i = 0; i < 3; i++)
    read(p[0], &runtime[i], sizeof(int));

  for(int i = 0; i < 3; i++)
    wait(0);

  printf("[prio 1]  runtime = %d\n", runtime[0]);
  printf("[prio 5]  runtime = %d\n", runtime[1]);
  printf("[prio 10] runtime = %d\n", runtime[2]);

  if(!(runtime[0] < runtime[1] && runtime[1] < runtime[2])){
    printf(RED "[FAIL] Scheduler did not prefer lower accumulator\n" RESET);
    exit(1);
  }

  printf(GREEN "[PASS] Scheduler order respected\n" RESET);
}

/* ---------------------------------
 * TEST 7: Tie-breaking by index (1 quantum)
 * --------------------------------- */
void test_tie_break(void) {
    printf("\n[TEST 7] Tie-breaking by process index\n");

    int p[2];
    if(pipe(p) < 0){
        printf("\033[31m[FAIL] Pipe creation failed\033[0m\n");
        exit(1);
    }

    set_ps_priority(5);  // All same priority

    // Fork first child (index 0)
    int p1 = fork();
    if(p1 == 0){
        // Write to pipe immediately upon running first time
        int idx = 0;
        write(p[1], &idx, sizeof(idx));

        // Do minimal CPU work (1 quantum)
        for(volatile int i = 0; i < 1000000; i++);
        exit(0);
    }

    sleep(1); // ensure different creation order

    // Fork second child (index 1)
    int p2 = fork();
    if(p2 == 0){
        int idx = 1;
        write(p[1], &idx, sizeof(idx));

        for(volatile int i = 0; i < 1000000; i++);
        exit(0);
    }

    // Read the order in which they actually ran
    int first_to_run, second_to_run;
    read(p[0], &first_to_run, sizeof(int));
    read(p[0], &second_to_run, sizeof(int));

    wait(0);
    wait(0);

    printf("[INFO] First to run index = %d\n", first_to_run);
    printf("[INFO] Second to run index = %d\n", second_to_run);

    if(first_to_run > second_to_run){
        printf("\033[31m[FAIL] Tie-breaking violated process index order\033[0m\n");
        exit(1);
    }

    printf("\033[32m[PASS] Tie-breaking respected process index order\033[0m\n");
}

/* -----------------------------
 * MAIN
 * ----------------------------- */
int main(void) {
  printf("\n====== PRIORITY SCHEDULER FULL TEST SUITE ======\n");

  test_default_priority();
  test_syscall_validation();
  test_no_charge_on_sleep();
  test_fork_fairness();
  test_wakeup_fairness();
  test_scheduler_order();
  test_tie_break();

  printf(GREEN "\n====== ALL TESTS PASSED ======\n" RESET);
  exit(0);
}

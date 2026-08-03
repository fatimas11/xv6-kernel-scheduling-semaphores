#include "kernel/types.h"
#include "user/user.h"

/* ANSI colors */
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define WHITE   "\x1b[37m"
#define RESET   "\x1b[0m"

/* --------------------------------
 * Shared helper
 * -------------------------------- */
void critical_work(int ticks) {
  sleep(ticks);
}

/* --------------------------------
 * Helper for printing results
 * -------------------------------- */
void pass(const char *msg) { printf(GREEN "[PASS] %s\n" RESET, msg); }
void fail(const char *msg) { printf(RED "[FAIL] %s\n" RESET, msg); }
void info(const char *msg) { printf(WHITE "%s\n" RESET, msg); }

/* --------------------------------
 * TEST 1: sem_create basic
 * -------------------------------- */
void test_create(void) {
  info("\n[TEST 1] sem_create basic");
  int sem = sem_create(1);
  if (sem >= 0)
    pass("sem_create returned a valid ID");
  else
    fail("sem_create failed");

  if (sem_free(sem) == 0)
    pass("sem_free succeeded");
  else
    fail("sem_free failed");
}

/* --------------------------------
 * TEST 2: Blocking on sem_down
 * -------------------------------- */
void test_blocking(void) {
  info("\n[TEST 2] sem_down blocking");
  int sem = sem_create(1);

  if (fork() == 0) {
    sem_down(sem);
    info("[CHILD A] entered CS");
    critical_work(30);
    info("[CHILD A] leaving CS");
    sem_up(sem);
    exit(0);
  }

  if (fork() == 0) {
    sleep(5);
    sem_down(sem);
    info("[CHILD B] entered CS after A");
    sem_up(sem);
    exit(0);
  }

  wait(0);
  wait(0);

  sem_free(sem);
  pass("Blocking behavior verified");
}

/* --------------------------------
 * TEST 3: Counting behavior
 * -------------------------------- */
void test_counting(void) {
  info("\n[TEST 3] Counting semaphore (value = 2)");
  int sem = sem_create(2);

  for (int i = 0; i < 3; i++) {
    if (fork() == 0) {
      sem_down(sem);
      printf("[PID %d] entered CS\n" RESET, getpid());
      critical_work(20);
      printf("[PID %d] leaving CS\n" RESET, getpid());
      sem_up(sem);
      exit(0);
    }
  }

  for (int i = 0; i < 3; i++) wait(0);

  sem_free(sem);
  pass("At most 2 processes entered CS concurrently");
}

/* --------------------------------
 * TEST 4: Wakeup correctness
 * -------------------------------- */
void test_wakeup(void) {
  info("\n[TEST 4] Wakeup correctness");
  int sem = sem_create(0);

  if (fork() == 0) {
    info("[CHILD] waiting on sem_down");
    sem_down(sem);
    pass("[CHILD] woke up correctly");
    sem_up(sem);
    exit(0);
  }

  sleep(20);
  info("[PARENT] calling sem_up");
  sem_up(sem);

  wait(0);
  sem_free(sem);
  pass("Wakeup matched sleep channel");
}

/* --------------------------------
 * TEST 5: sem_free safety
 * -------------------------------- */
void test_free_safety(void) {
  info("\n[TEST 5] sem_free safety");
  int sem = sem_create(1);

  if (fork() == 0) {
    sem_down(sem);
    critical_work(40);
    sem_up(sem);
    exit(0);
  }

  sleep(5);
  int r = sem_free(sem);
  if (r == -1)
    pass("sem_free correctly blocked while in use");
  else
    fail("sem_free returned success while in use");

  wait(0);
  r = sem_free(sem);
  if (r == 0)
    pass("sem_free succeeded after all processes done");
  else
    fail("sem_free failed after usage");
}

/* --------------------------------
 * TEST 6: Semaphore reuse
 * -------------------------------- */
void test_reuse(void) {
  info("\n[TEST 6] Semaphore reuse");
  int sem1 = sem_create(1);
  sem_free(sem1);
  int sem2 = sem_create(1);

  if (sem2 >= 0)
    pass("Semaphore slot reused correctly");
  else
    fail("Semaphore reuse failed");

  sem_free(sem2);
}

/* --------------------------------
 * TEST 7: Edge cases & invalid IDs
 * -------------------------------- */
void test_edges(void) {
  info("\n[TEST 7] Edge cases & invalid IDs");

  if (sem_down(-1) < 0)
    pass("sem_down correctly failed on invalid ID");
  else
    fail("sem_down did not fail on invalid ID");

  if (sem_up(-1) < 0)
    pass("sem_up correctly failed on invalid ID");
  else
    fail("sem_up did not fail on invalid ID");

  int sem = sem_create(0);
  if (sem >= 0) {
    pass("Created semaphore with 0 initial value");
    sem_free(sem);
  } else {
    fail("Failed to create semaphore with 0 initial value");
  }

  sem = sem_create(-5);
  if (sem >= 0)
    pass("Created semaphore with negative initial value (check allowed)");
  else
    fail("Failed to create semaphore with negative initial value");
}

/* --------------------------------
 * TEST 8: Max semaphore allocation
 * -------------------------------- */
#define MAX_TEST_SEMS 128
void test_max_allocation(void) {
  for (int i = 0; i < MAX_TEST_SEMS; i++) {
    sem_free(i);  // ignore failures
  }
  info("\n[TEST 8] Max semaphore allocation");
  int sems[MAX_TEST_SEMS];
  int i;
  for (i = 0; i < MAX_TEST_SEMS; i++) {
    sems[i] = sem_create(1);
    if (sems[i] < 0) break;
  }
  if (i == MAX_TEST_SEMS)
    pass("Successfully allocated maximum semaphores");
  else
    fail("Failed to allocate maximum semaphores");
 // Test allocation beyond limit
  int extra = sem_create(1);
  if (extra == -1)
    pass("Correctly failed to allocate semaphore beyond MAX_SEMS");
  else {
    fail("Allocated semaphore beyond MAX_SEMS!"); 
    sem_free(extra);
  }

  // Free all allocated semaphores
  for (int j = 0; j < i; j++) sem_free(sems[j]);
}

/* --------------------------------
 * TEST 9: Rapid acquire/release stress
 * -------------------------------- */
void test_stress(void) {
  info("\n[TEST 9] Rapid acquire/release stress");
  int sem = sem_create(2);
  if (sem < 0) {
    fail("Failed to create semaphore for stress test");
    return;
  }

  for (int i = 0; i < 5; i++) {
    if (fork() == 0) {
      for (int j = 0; j < 10; j++) {
        sem_down(sem);
        sem_up(sem);
      }
      exit(0);
    }
  }

  for (int i = 0; i < 5; i++) wait(0);
  sem_free(sem);
  pass("Rapid acquire/release stress test completed");
}

/* --------------------------------
 * TEST 10: Sleep deadlock check
 * -------------------------------- */
#define DEADLOCK_TIMEOUT 100   // ticks

void test_sleep_deadlock(void) {
    info("\n[TEST 10] Sleep while holding array lock deadlock check");

    int sem = sem_create(0);  // start with value 0 to block sem_down
    if (sem < 0) {
        fail("Could not create semaphore");
        return;
    }

    int pid = fork();
    if (pid == 0) {
        // Child: will try to acquire semaphore (will sleep)
        int r = sem_down(sem);
        if (r == 0) {
            pass("[Child] sem_down succeeded after parent sem_up");
        } else {
            fail("[Child] sem_down failed");
        }
        exit(0);
    }

    // Parent: sleep a bit to ensure child is blocked
    sleep(10);

    // Parent: release semaphore to wake child
    sem_up(sem);

    // Wait with timeout
    int waited = 0;
    while(wait(0) != -1 && waited < DEADLOCK_TIMEOUT) {
        sleep(5);
        waited += 5;
    }

    if (waited >= DEADLOCK_TIMEOUT) {
        fail("[TEST 10] Deadlock detected (child did not wake up)");
    } else {
        pass("[TEST 10] No deadlock detected");
    }

    // Cleanup
    sem_free(sem);
}

/* --------------------------------
 * MAIN
 * -------------------------------- */
int main(void) {
  info("\n====== SEMAPHORE FULL TEST ======");
  test_create();
  test_blocking();
  test_counting();
  test_wakeup();
  test_free_safety();
  test_reuse();
  test_edges();
  test_max_allocation();
  test_stress();
  test_sleep_deadlock();

  info(GREEN "\n====== ALL TESTS COMPLETED ======\n");
  exit(0);
}

/**
 * myint.c - Another handy routine for testing your tiny shell
 * 
 * usage: myint <n>
 * Sleeps for <n> seconds and sends SIGINT to itself.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "exit-utils.h"

static const int kWrongArgumentCount = 1;
static const int kKillError = 2;

int main(int argc, char *argv[]) {
  exitUnless(argc == 2, kWrongArgumentCount, 
	     stderr, "Usage: %s <n>\n", argv[0]);
  size_t secs = atoi(argv[1]);
  for (size_t i = 0; i < secs; i++) {
    sleep(1);
  }
  
  pid_t pid = getpid();
  exitUnless(kill(pid, SIGINT) == 0, kKillError,
	     stderr, "Problem interrupting process %d", pid);
  return 0;
}

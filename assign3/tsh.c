/**
 * File: tsh.c
 * -----------
 * Your name: <your name here>
 * Your favorite shell command: <your favorite shell command here> 
 */

#include <stdbool.h>       // for bool type
#include <stdio.h>         // for printf, etc
#include <stdlib.h>        
#include <unistd.h>        // for fork
#include <string.h>        // for strlen, strcasecmp, etc
#include <ctype.h>
#include <signal.h>        // for signal
#include <sys/types.h>
#include <sys/wait.h>      // for wait, waitpid
#include <errno.h>

#include <sys/stat.h>	   // Extra includes for opening and
#include <fcntl.h>	   // closing files

#include "tsh-state.h"
#include "tsh-constants.h"
#include "tsh-parse.h"
#include "tsh-jobs.h"
#include "tsh-signal.h"
#include "exit-utils.h"    // provides exitIf, exitUnless

// Copied from lecture-examples simplesh.c file
static pid_t forkProcess() {
  pid_t pid = fork();
  exitIf(pid == -1, 1, stderr, "fork function failed.\n");
  return pid;
}

/** 
 * Block until process pid is no longer the foreground process
 */
static void waitfg(pid_t pid) {
  while(pid == getFGJobPID(jobs)) sleep(1);
}

/**
 * Execute the builtin bg and fg commands
 */
static void handleBackgroundForegroundBuiltin(char *argv[]) {
    // Handle error:  not enough arguments
    if(argv[1] == NULL) {
	printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }

    job_t *curJob;
    // Find the current job by JID
    if(*argv[1] == '%') {
  	int JID = (int) strtol(argv[1]+1, (char **)NULL,10);
	if(JID < 1) {
	    printf("%s: argument must be a PID or %%jobid\n", argv[0]);
	    return;
	}
	curJob = getJobByJID(jobs, JID);
	if(curJob == NULL) {
	    printf("%%%d: No such job\n", JID);
	    return;
	}
    }

    // by PID
    else {
	int PID = (int) strtol(argv[1], (char **) NULL, 10);
	if(PID < 1) {
	    printf("%s: argument must be a PID or %%jobid\n",argv[0]);
	    return; 
	}
	curJob = getJobByPID(jobs, PID);
	if(curJob == NULL) {
	    printf("(%d): No such process\n", PID);
	    return;
	}
    }
    
    // Handle the fg/bg functionality on the given job
    bool fg = strcasecmp(argv[0], "fg") == 0;
  
    if(!fg) printf("[%d] (%d) %s\n", getJIDFromPID(curJob->pid) , curJob->pid , curJob->commandLine); 
    curJob -> state = fg ? kForeground : kBackground;
    kill(-(curJob -> pid), SIGCONT);
    if (fg) waitfg(curJob -> pid);
}

/**
 * Sets up the file descriptor pipes for I/O redirection
 */
static void handleIO(char *infile, char *outfile) {
   // Handle input redirection 
   if(infile != NULL) {
        int infd = open(infile, O_RDONLY , 0);
        if(infd < 0) {
            printf("Error: failed to open %s\n",infile);
            exit(1);
        }
        dup2(infd, STDIN_FILENO);
        close(infd);
    }

    // Handle output redirection
    if(outfile != NULL) {
        int outfd = open(outfile, O_CREAT|O_WRONLY, S_IRWXU | S_IRWXG | S_IROTH | S_IWOTH);
	if(outfd < 0) {
            printf("Error: failed to open %s\n",outfile);
            exit(1);
        }
	dup2(outfd, STDOUT_FILENO);
	close(outfd);
    }
}

/**
 * If the user has typed a built-in command then execute 
 * it immediately.  Return true if and only if the command 
 * was a builtin and executed inline.
 */
static bool handleBuiltin(char *argv[], char *infile, char *outfile) {
    if (strcasecmp(argv[0], "quit") == 0) exit(0);
   
    int in = dup(STDIN_FILENO);
    int out = dup(STDOUT_FILENO);
    
    // bg and fg commands
    if (strcasecmp(argv[0], "bg") == 0 || strcasecmp(argv[0], "fg") == 0)  {
	// handle I/O redirection and process the function
        handleIO(infile, outfile);
	handleBackgroundForegroundBuiltin(argv);

        // restore stdin and stdout
	dup2(in, STDIN_FILENO);
        dup2(out, STDOUT_FILENO);
	return true;
    }
 
    // jobs command
    if (strcasecmp(argv[0], "jobs") == 0) {
	// handle I/O redirection and listJobs
	handleIO(infile, outfile);
	listJobs(jobs);

	// restore stdin and stdout
	dup2(in, STDIN_FILENO);
	dup2(out, STDOUT_FILENO);
	return true;
    }

    // If dup2 hasn't closed in and out already, close them and exit
    close(in);
    close(out);
  
    // not a builtin
    return false;
}

/**
 * The kernel sends a SIGCHLD to the shell whenever a child job terminates 
 * (becomes a zombie), or stops because it receives a SIGSTOP or SIGTSTP signal.  
 * The handler reaps all available zombie children, but doesn't wait for any other
 * currently running children to terminate.  
 */
static void handleSIGCHLD(int unused) {
    int status = 0;
    // If a child stops normally, remove it from the jobs list.
    // If a child is terminated unnaturally, print that it has been terminated and remove it from the jobs list.
    // If a child is stopped, print that it had been stopped and update its job state to kStopped 
    
    pid_t pid = waitpid(-1, &status, WNOHANG | WUNTRACED);
    
    if(WIFSTOPPED(status)) {
	printf("Job [%d] (%d) stopped by signal %d\n", getJIDFromPID(pid), pid, WSTOPSIG(status));	
	getJobByPID(jobs, pid) -> state = kStopped;
	return; 
    }
    if(!WIFEXITED(status)) {
    	printf("Job [%d] (%d) terminated by signal %d\n", getJIDFromPID(pid), pid, WTERMSIG(status));
    }
    deleteJob(jobs, pid);
}

/**
 * The kernel sends a SIGTSTP to the shell whenever
 * the user types ctrl-z at the keyboard.  Catch it and suspend the
 * foreground job by sending it a SIGTSTP.
 */
static void handleSIGTSTP(int sig) {
    pid_t pid = getFGJobPID(jobs);
    kill(-pid,sig);
    waitpid(-pid,NULL,WNOHANG); // This line is necessary so the next shell command is not printed before 
	 		        // handling printing information in handleSIGCHLD
}

/**
 * The kernel sends a SIGINT to the shell whenver the
 * user types ctrl-c at the keyboard.  Catch it and send it along
 * to the foreground job.  
 */
static void handleSIGINT(int sig) {
    pid_t pid = getFGJobPID(jobs);
    if(sig != 2) printf("[%d] stopped by signal %d\n", getJIDFromPID(pid), sig);
    kill(-pid,sig);
}

/**
 * The driver program can gracefully terminate the
 * child shell by sending it a SIGQUIT signal.
 */
static void handleSIGQUIT(int sig) {
  printf("Terminating after receipt of SIGQUIT signal\n");
  exit(1);
}

/**
 * Prints a nice little usage message to make it clear how the
 * user should be invoking tsh.
 */
static void usage() {
  printf("Usage: ./tsh [-hvp]\n");
  printf("   -h   print this message\n");
  printf("   -v   print additional diagnostic information\n");
  printf("   -p   do not emit a command prompt\n");
  exit(1);
}

/**
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately.  Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
 */
static void eval(char commandLine[]) {
    // Initialize arguments
    char *CL = malloc(strlen(commandLine));
    strcpy(CL,commandLine); // Retains commandline without null terminators in whitespace
    char *argv[kMaxArgs];
    char *infile = NULL;
    char *outfile = NULL;

    bool bg = parseLine(CL , argv , kMaxArgs , &infile , &outfile); 
    
    // Handle all builtin functions
    if(handleBuiltin(argv, infile, outfile)) return;
    
    // Setup variables for sigprocmask signal blocking
    sigset_t sigchld_set;
    sigemptyset(&sigchld_set);
    sigaddset(&sigchld_set,SIGCHLD);

    // Inspired by B&O page 35 (in 2nd half version of the text) (CS 110 Version)
    pid_t pid = forkProcess();
    sigprocmask(SIG_BLOCK,&sigchld_set,NULL);
    if(pid == 0) {
	sigprocmask(SIG_UNBLOCK,&sigchld_set,NULL);
	setpgid(0, 0);
	handleIO(infile,outfile);
	if(execvp(argv[0], argv) < 0) {
	    printf("%s: Command not found\n", argv[0]);
	    exit(0);
	}
    }
    
    // Add the child process to the jobs list and unblock SIGCHLD signals
    int state = bg ? kBackground : kForeground;
    addJob(jobs, pid, state, commandLine);
    sigprocmask(SIG_UNBLOCK,&sigchld_set,NULL);

    // Wait for foreground process, otherwise print job info and continue
    if(!bg) waitfg(pid);
    else printf("[%d] (%d) %s\n", getJIDFromPID(pid), pid, commandLine);
    free(CL);
}

/**
 * Redirect stderr to stdout (so that driver will get all output
 * on the pipe connected to stdout) 
 */
static void mergeFileDescriptors() {
  dup2(STDOUT_FILENO, STDERR_FILENO);
}

/**
 * Defines the main read-eval-print loop
 * for your simplesh.
 */
int main(int argc, char *argv[]) {
  mergeFileDescriptors();
  while (true) {
    int option = getopt(argc, argv, "hvp");
    if (option == EOF) break;
    switch (option) {
    case 'h':
      usage();
      break;
    case 'v': // emit additional diagnostic info
      verbose = true;
      break;
    case 'p':           
      showPrompt = false;
      break;
    default:
      usage();
    }
  }
  
  installSignalHandler(SIGQUIT, handleSIGQUIT); 
  installSignalHandler(SIGINT,  handleSIGINT);   // ctrl-c
  installSignalHandler(SIGTSTP, handleSIGTSTP);  // ctrl-z
  installSignalHandler(SIGCHLD, handleSIGCHLD);  // terminated or stopped child
  initJobs(jobs);  

  while (true) {
    if (showPrompt) {
      printf("%s", kPrompt);
      fflush(stdout);
    }

    char command[kMaxLine];
    fgets(command, kMaxLine, stdin);
    if (feof(stdin)) break;
    command[strlen(command) - 1] = '\0'; // overwrite fgets's \n
    if (strcasecmp(command, "quit") == 0) break;
    eval(command);
    fflush(stdout);
  }
  
  fflush(stdout);  
  return 0;
}

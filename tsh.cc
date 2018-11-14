// 
// tsh - A tiny shell program with job control
// source code provided by the University of Colorado Boulder
// Department of Computer Science
// Edited by
// Andrea Chamorro BS - Computer Science
// Christian F. Sousa BS - Computer Science , BS - Mechanical Engineering
//

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string>

#include "globals.h"
#include "jobs.h"
#include "helper-routines.h"
#include<iostream>

//
// Needed global variable definitions
//


//builtin fuctions
#define BLTN_EX 1// for exit
#define BLTN_KA 2//for kill all
#define BLTN_JOBS 3// for jobs
#define BLTN_BGFG 4//for do_bgfg
#define BLTN_IGNR //for ignore
#define MAXARGS 128 // maximum number of arguments
#define MAXLINE 1024 // maximum number of lines
#define MAXJOBS 16

// job states
#define UNDEF 0
#define FG 1
#define BG 2
#define ST 3 // stopped 

//Variables
int nextjid = 1;


static char prompt[] = "tsh> ";
int verbose = 0;
void do_exit();
void do_killall(char **argv);
void do_show_jobs();
void showjobs(struct job_t *jobs);
struct job_t *findprocessid(struct job_t *jobs,pid_t pid);
void fgpid(struct job_t jobs);

//
// You need to implement the functions                                              Complete/Incomplete
//eval                                                                                    started  
//builtin_cmd                                                                             COMPLETED
//do_bgfg
//do exit                                                                                 Completed
//do jobs
//do killall
//showjobs
//waitfg
//sigchld_handler
//sigstp_handler
//sigint_handler
//
//
// jobs struct


// The code below provides the "prototypes" for those functions
// so that earlier code can refer to them. You need to fill in the
// function bodies below.
// 

void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);
void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

//


//
// main - The shell's main routine 
//
int main(int argc, char **argv) 
{
  int emit_prompt = 1; // emit prompt (default)

  //
  // Redirect stderr to stdout (so that driver will get all output
  // on the pipe connected to stdout)
  //
  dup2(1, 2);

  /* Parse the command line */
  char c;
  while ((c = getopt(argc, argv, "hvp")) != EOF) {
    switch (c) {
    case 'h':             // print help message
      usage();
      break;
    case 'v':             // emit additional diagnostic info
      verbose = 1;
      break;
    case 'p':             // don't print a prompt
      emit_prompt = 0;  // handy for automatic testing
      break;
    default:
      usage();
    }
  }

  //
  // Install the signal handlers
  //

  //
  // These are the ones you will need to implement
  //
  Signal(SIGINT,  sigint_handler);   // ctrl-c
  Signal(SIGTSTP, sigtstp_handler);  // ctrl-z
  Signal(SIGCHLD, sigchld_handler);  // Terminated or stopped child

  //
  // This one provides a clean way to kill the shell
  //
  Signal(SIGQUIT, sigquit_handler); 

  //
  // Initialize the job list
  //
  initjobs(jobs);

  //
  // Execute the shell's read/eval loop
  //
  for(;;) {
    //
    // Read command line
    //
    if (emit_prompt) {
      printf("%s", prompt);
      fflush(stdout);
    }

    char cmdline[MAXLINE];

    if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin)) {
      app_error("fgets error");
    }
    //
    // End of file? (did user type ctrl-d?)
    //
    if (feof(stdin)) {
      fflush(stdout);
      exit(0);
    }

    //
    // Evaluate command line
    //
    eval(cmdline);
    fflush(stdout);
    fflush(stdout);
  } 

  exit(0); //control never reaches here
}
  
/////////////////////////////////////////////////////////////////////////////
//
// eval - Evaluate the command line that the user has just typed in
// 
// If the user has requested a built-in command (quit, jobs, bg or fg)
// then execute it immediately. Otherwise, fork a child process and
// run the job in the context of the child. If the job is running in
// the foreground, wait for it to terminate and then return.  Note:
// each child process must have a unique process group ID so that our
// background children don't receive SIGINT (SIGTSTP) from the kernel
// when we type ctrl-c (ctrl-z) at the keyboard.
//
void eval(char *cmdline) 
{
  /* Parse command line */
  //
  // The 'argv' vector is filled in by the parseline
  // routine below. It provides the arguments needed
  // for the execve() routine, which you'll need to
  // use below to launch a process.
  //
  char *argv[MAXARGS];
  char cmdholder[MAXLINE];
  strcpy(cmdholder, cmdline); // copy command line into holder;
  pid_t pid; // this is a process id
  //
  // The 'bg' variable is TRUE if the job should run
  // in background mode or FALSE if it should run in FG
  //
  int bg = parseline(cmdholder, argv); 
    if (argv[0] == NULL)  
        return;   /* ignore empty lines */
    if (!builtin_cmd(argv))
    {
      int state;
      sigset_t mask;
      sigemptyset(&mask);
      sigaddset(&mask, SIGCHLD);
      sigprocmask(SIG_BLOCK, &mask, NULL);
      if((pid=fork())==0)
      {
        //cout<< "this will be a child process"<<endl;
        // some stuff about the child running the process
        sigprocmask(SIG_UNBLOCK, &mask, NULL);	// unblock SIGCHLD for child
        setpgid(0, 0); // because hint section of lab handout
        if(execve(argv[0],argv,environ)<0)
        {
          cout << "Command not found " << argv[0] << endl;
          exit(0); // exit the invalid command
            return;
        }
      }
        if(!bg)// forground processece 
        {
          state =FG;
          addjob(jobs, pid, state,cmdline);
          sigprocmask(SIG_UNBLOCK, &mask, NULL);
          //cout<< "this is a forground process"<< endl;
          waitfg(pid);
        }
        else// backgournd process
        {
          //cout << "this is a background process"<< endl;
          int jid;
          state= BG;
          addjob(jobs, pid, state,cmdline);
          sigprocmask(SIG_UNBLOCK, &mask, NULL);
          jid = pid2jid(pid);
          printf("[%d] (%d) %s", jid, pid, cmdholder);
        }
     }

  return;
}

//
// builtin_cmd - If the user has typed a built-in command then execute
// it immediately. The command name would be in argv[0] and
// is a C string. We've cast this to a C++ string type to simplify
// string comparisons; however, the do_bgfg routine will need 
// to use the argv array as well to look for a job number.
//
int builtin_cmd(char **argv) 
{
  //string cmd(argv[0]);
  // im just gonna use string compare because I do no know what this does
  if(!strcmp(argv[0], "quit"))
  {
      do_exit();
      return BLTN_EX; 
  }
  if(!strcmp(argv[0], "fg"))
  {
    // some do bfgb
    //cout<< "reached fg"<< endl;
    do_bgfg(argv);
    return BLTN_BGFG; 
  }
  if(!strcmp(argv[0], "bg"))
  {
    // some do bfgb
    //cout << "reached bg"<< endl;
    do_bgfg(argv);
    return BLTN_BGFG;
  }
  if(!strcmp(argv[0], "killall"))
  {
    // do the kill all fuciton
    //cout<< "reached killall"<< endl;
    do_killall(argv);
  }
  if(!strcmp(argv[0], "jobs"))
  {
    //cout<< "reached jobs"<< endl;
    do_show_jobs();
  }
  return 0;     /* not a builtin command */
}
void do_exit(void)
{
  exit(0);
}

void do_killall(char **argv)
{

}
// show all the jobs
void do_show_jobs(void)
{
    showjobs(jobs);
}

void showjobs(struct job_t *jobs)
{
    cout<< "show jobs not yet implemented"<< endl;
}

/////////////////////////////////////////////////////////////////////////////
//
// do_bgfg - Execute the builtin bg and fg commands
//
void do_bgfg(char **argv) 
{
  struct job_t *jobp=NULL;
    
  /* Ignore command if no argument */
  if (argv[1] == NULL) {
    printf("%s command requires PID or %%jobid argument\n", argv[0]);
    return;
  }
    
  /* Parse the required PID or %JID arg */
  if (isdigit(argv[1][0])) {
    pid_t pid = atoi(argv[1]);
    if (!(jobp = getjobpid(jobs, pid))) {
      printf("(%d): No such process\n", pid);
      return;
    }
  }
  else if (argv[1][0] == '%') {
    int jid = atoi(&argv[1][1]);
    if (!(jobp = getjobjid(jobs, jid))) {
      printf("%s: No such job\n", argv[1]);
      return;
    }
  }	    
  else {
    printf("%s: argument must be a PID or %%jobid\n", argv[0]);
    return;
  }

  //
  // You need to complete rest. At this point,
  // the variable 'jobp' is the job pointer
  // for the job ID specified as an argument.
  //
  // Your actions will depend on the specified command
  // so we've converted argv[0] to a string (cmd) for
  // your benefit.
  //
  string cmd(argv[0]);

  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// waitfg - Block until process pid is no longer the foreground process
//
void waitfg(pid_t pid)
{
  if(pid==0 || findprocessid(jobs,pid)==NULL)
  {
    //if we get here then the pid does not correspond to a current process
    return;
  }
  while(pid==fgpid(jobs))
  {
    cout<< "pid: "<< pid<< " compared to : "<< fgpid(jobs)<<endl;
    sleep(1);
    //wait 1 sec while the process is in the forground
  }
}
//find if a process if currently running
struct job_t *findprocessid(struct job_t *jobs,pid_t pid)
{
  if(pid<1)
    return NULL;
  
  for(int i=0; i<MAXJOBS; i++)
  {
    if(jobs[i].pid==pid)
      return &jobs[i];
  }
  return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
// Signal handlers
//

/////////////////////////////////////////////////////////////////////////////
//
// sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
//     a child job terminates (becomes a zombie), or stops because it
//     received a SIGSTOP or SIGTSTP signal. The handler reaps all
//     available zombie children, but doesn't wait for any other
//     currently running children to terminate.  
//
void sigchld_handler(int sig) //we want to wait until child is done executed
{
    cout<< "this is a test"<< endl;
    pid_t pid;
    int status;
    //WNOHANG wait for the child whose process id is equal to the value of pid
    //WUNTRACED the status of child processes under pid that are stopped
    while ((pid = waitpid(WAIT_ANY, &status, WNOHANG | WUNTRACED)) > 0)
    { //lets child processes end before reapping them
        //WUNTRACED	is for non terminated processes (stopped processes)
		if(WIFSTOPPED(status)){ // change the state to stopped beacuse status stopped
            struct job_t *job = getjobpid(jobs, pid);
  			job->state = ST;
			printf("Job [%d] (%d) stopped by signal 20\n", job->jid, pid);
			return;
		}
		else if(WIFSIGNALED(status)){ //if ctrl-c
            struct job_t *job = getjobpid(jobs, pid); 
			printf("Job [%d] (%d) terminated by signal 2\n", job->jid, pid);
			deletejob(jobs, pid);
		}
		else{		
 			deletejob(jobs, pid);
 			}
 		}		
	return; 
}

/////////////////////////////////////////////////////////////////////////////
//
// sigint_handler - The kernel sends a SIGINT to the shell whenver the
//    user types ctrl-c at the keyboard.  Catch it and send it along
//    to the foreground job.
//
void sigint_handler(int sig) 
{
  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
//     the user types ctrl-z at the keyboard. Catch it and suspend the
//     foreground job by sending it a SIGTSTP.  
//
void sigtstp_handler(int sig) 
{
  return;
}

/*********************
 * End signal handlers
 *********************/// 


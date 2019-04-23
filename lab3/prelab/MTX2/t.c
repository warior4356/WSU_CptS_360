/*********** t.c file of A Multitasking System *********/
#include <stdio.h>
#include "string.h"
#include "type.h"

int active = 0;
PROC proc[NPROC];      // NPROC PROCs
PROC *freeList;        // freeList of PROCs 
PROC *readyQueue;      // priority queue of READY procs
PROC *running;         // current running proc pointer

PROC *sleepList;       // list of SLEEP procs

#include "queue.c"     // include queue.c file
#include "wait.c"      // include wait.c file

/*******************************************************
  kfork() creates a child process; returns child pid.
  When scheduled to run, child PROC resumes to body();
********************************************************/
int body(), tswitch(), do_sleep(), do_wakeup(), do_exit(), do_switch();
int do_kfork(), printChild(), do_wait(), kwait();

// initialize the MT system; create P0 as initial running process
int init() 
{
  int i;
  PROC *p;
  for (i=0; i<NPROC; i++){ // initialize PROCs
    p = &proc[i];
    p->pid = i;            // PID = 0 to NPROC-1  
    p->status = FREE;
    p->priority = 0;      
    p->next = p+1;
  }
  proc[NPROC-1].next = 0;  
  freeList = &proc[0];     // all PROCs in freeList     
  readyQueue = 0;          // readyQueue = empty

  sleepList = 0;           // sleepList = empty
  
  // create P0 as the initial running process
  p = running = dequeue(&freeList); // use proc[0] 
  p->status = READY;
  p->priority = 0;
  p->ppid = 0;             // P0 is its own parent
  
  printList("freeList", freeList);
  printf("init complete: P0 running\n"); 
}

int menu()
{
  printf("*********************************************\n");
  printf(" ps fork switch exit jesus sleep wakeup wait \n");
  printf("*********************************************\n");
}

char *status[ ] = {"FREE", "READY", "SLEEP", "ZOMBIE"};

int do_ps()
{
  int i;
  PROC *p;
  printf("PID  PPID  status\n");
  printf("---  ----  ------\n");
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    printf(" %d    %d    ", p->pid, p->ppid);
    if (p == running)
      printf("RUNNING\n");
    else
      printf("%s\n", status[p->status]);
  }
}

int do_jesus()
{
  int i;
  PROC *p;
  printf("Jesus perfroms miracles here\n");
  for (i=1; i<NPROC; i++){
    p = &proc[i];
    if (p->status == ZOMBIE){
      p->status = READY;
      enqueue(&readyQueue, p);
      printf("raised a ZOMBIE %d to live again\n", p->pid);
    }
  }
  printList("readyQueue", readyQueue);
}

int printChild(){
  PROC *temp = running->child;
  printf("  childList = ");
  while(temp){
    printf("[%d]->", temp->pid);
    temp = temp->sibling;
  }
  printf("NULL\n");
}

char *myname = "Ryal O'Neil";    
int body()   // process body function
{
  printf("%s\n", myname);
  int c;
  char cmd[64];
  printf("proc %d starts from body()\n", running->pid);
  while(1){
    printf("***************************************\n");
    printf("proc %d running: parent=%d", running->pid,running->ppid);
    printChild();
    printList("readyQueue", readyQueue);
    printSleep("sleepList ", sleepList);
    menu();
    printf("enter a command : ");
    fgets(cmd, 64, stdin);
    cmd[strlen(cmd)-1] = 0;

    if (strcmp(cmd, "ps")==0)
      do_ps();
    if (strcmp(cmd, "fork")==0)
      do_kfork();
    if (strcmp(cmd, "switch")==0)
      do_switch();
    if (strcmp(cmd, "exit")==0)
      do_exit();
    if (strcmp(cmd, "jesus")==0)
      do_jesus();
    if (strcmp(cmd, "sleep")==0)
      do_sleep();
    if (strcmp(cmd, "wakeup")==0)
      do_wakeup();
    if (strcmp(cmd, "wait")==0)
      do_wait();
  }
  //do_exit();
}

int kfork()
{
  int  i;

  PROC *temp, *p = dequeue(&freeList);
  if(p){
     p->parent = running;
     if(running->child){
        temp = running->child;
        while(temp->sibling){
          temp = temp->sibling;
        }
        temp->sibling = p;
     }
     else{
       running->child = p;
     }
  }
  else{
     printf("no more proc\n");
     return(-1);
  }
  /* initialize the new proc and its stack */
  p->status = READY;
  p->priority = 1;       // ALL PROCs priority=1, except P0
  p->ppid = running->pid;
  
  /************ new task initial stack contents ************
   kstack contains: |retPC|eax|ebx|ecx|edx|ebp|esi|edi|eflag|
                      -1   -2  -3  -4  -5  -6  -7  -8   -9
  **********************************************************/
  for (i=1; i<10; i++)               // zero out kstack cells
      p->kstack[SSIZE - i] = 0;
  p->kstack[SSIZE-1] = (int)body;    // retPC -> body()
  //p->kstack[SSIZE-2] = (int)do_exit;
  //p->kstack[SSIZE-1] = *myname;
  p->ksp = &(p->kstack[SSIZE - 9]);  // PROC.ksp -> saved eflag 
  enqueue(&readyQueue, p);           // enter p into readyQueue
  return p->pid;
}

int do_kfork()
{
   int child = kfork();
   if (child < 0)
      printf("kfork failed\n");
   else{
      printf("proc %d kforked a child = %d\n", running->pid, child); 
      printList("readyQueue", readyQueue);
   }
   return child;
}

int do_switch()
{
   tswitch();
}

int do_exit()
{
	PROC *temp1, *temp2;
	int event;
	if(running->pid != 1){
		if(running->child){
			temp1 = running->child;
			if(proc[1].child){
				temp2 = proc[1].child;
				while(temp2->sibling){
					temp2 = temp2->sibling;
				}
				temp2->sibling = temp1;
    	}
    	else{
    		proc[1].child = temp1;
    	}
    }
    printf("proc %d in do_exit, enter an exit value : ", running->pid);
    scanf("%d", &event); getchar();
    running->exitCode = event;
    kexit(running->pid);  // exit with own PID value
  }
  else{
    printf("P1 never die\n");
  } 
}

int do_sleep()
{
  int event;
  printf("enter an event value to sleep on : ");
  scanf("%d", &event); getchar();
  sleep(event);
}

int do_wakeup()
{
  int event;
  printf("enter an event value to wakeup with : ");
  scanf("%d", &event); getchar(); 
  wakeup(event);
}

int do_wait()
{
	int pid, status;
   pid = kwait(&status);

	if(pid == -1)
	{
		printf("do_wait: no child process..\n\r");
	}
	else
	{	//print pid and status;
		printf("do_wait(): pid = %d  status = %d \r\n", pid, status);
	}
}

int kwait(int *status)
{
	PROC *p; int i, hasChild = 0;
 	while(1) // search PROCs for a child
	{
  		for (i=1; i < NPROC; i++) // exclude P0
		{
   		p = &proc[i];
	 		if (p->status != FREE && p->ppid == running->pid)
			{
		  		hasChild = 1; // has child flag
		   	if (p->status == ZOMBIE) // lay the dead child to rest
				{
			 		*status = p->exitCode; // collect its exitCode
			  	p->status = FREE; // free its PROC
			  	if(p->sibling){	
			  		p->parent->child = p->sibling;
			  	}
			  	else{
			  		p->parent->child = NULL;
			  	}
			  	printf("Freed ZOMBIE[%d]\n", p->pid);
			   	enqueue(&freeList, p); // to freeList
				 	//active--;
				  	return(p->pid); // return its pid
				}
			}
		}
		if (!hasChild) return -1; // no child, return -1
		sleep(running->pid); // still has kids alive: sleep on PROC address
	}
}

/*************** main() function ***************/
int main()
{
   printf("Welcome to the MT Multitasking System\n");
   init();    // initialize system; create and run P0
   kfork();   // kfork P1 into readyQueue  
   while(1){
     printf("P0: switch process\n");
     while (readyQueue == 0);
         tswitch();
   }
}

/*********** scheduler *************/
int scheduler()
{ 
  printf("proc %d in scheduler()\n", running->pid);
  if (running->status == READY)
     enqueue(&readyQueue, running);
  printList("readyQueue", readyQueue);
  running = dequeue(&readyQueue);
  printf("next running = %d\n", running->pid);  
}

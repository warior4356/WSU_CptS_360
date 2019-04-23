/**** C4.1.c file: compute matrix sum by threads ***/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

#define  M   4
#define  N   500000

struct timeval t1, t2;

int A[M][N], sum[M];

int total;
pthread_mutex_t *m;

void *func(void *arg)        // threads function
{
   int j, row, temp, mysum = 0;
   row = (int)arg;
   for (j=0; j < N; j++)     // compute sum of A[row]in global sum[row]
       mysum += A[row][j]; 

   /************** A CRITICAL REGION *******************/
   pthread_mutex_lock(m);
   temp = total;   // get total
   temp += mysum;  // add mysum to temp  
   
   sleep(1);       // OR for (int i=0; i<100000000; i++); ==> switch threads

   total = temp;   //  write temp to total
   pthread_mutex_unlock(m);
   /************ end of CRITICAL REGION ***************/

   pthread_exit((void*)0);  // thread exit: 0=normal termination
}

// print the matrix (if N is small, do NOT print for large N)
int print()
{
   int i, j;
   if(N > 20)
   	return 0;
   for (i=0; i < M; i++){
     for (j=0; j < N; j++){
       printf("%4d ", A[i][j]);
     }
     printf("\n");
   }
}

int main (int argc, char *argv[])
{
   gettimeofday(&t1, NULL);
   m = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
   pthread_mutex_init(m, NULL); // initialize mutex m
   pthread_t thread[M];      // thread IDs
   int i, j, status;

   printf("main: initialize A matrix\n");

   for (i=0; i < M; i++){
     for (j=0; j < N; j++){
       A[i][j] = i + j + 1;
     }
   }

   print();

   printf("main: create %d threads\n", M);
   for(i=0; i < M; i++) {
      pthread_create(&thread[i], NULL, func, (void *)i); 
   }

   printf("main: try to join with threads\n");
   for(i=0; i < M; i++) {
     pthread_join(thread[i], (void *)&status);
     printf("main: joined with thread %d : status=%d\n", i, status);
   }

   printf("main: compute and print total : ");
   for (i=0; i < M; i++)
       total += sum[i];
   printf("total = %ld\n", total);

   gettimeofday(&t2, NULL);
   printf("seconds elapsed = %d\n", t2.tv_sec-t1.tv_sec);
   printf("microseconds elapsed = %d\n", t2.tv_usec-t1.tv_usec);
   pthread_exit(NULL);
}

/**** s.c file: compute matrix sum sequentially ***/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

#define  N   500000
#define M 4

struct timeval t1, t2;

int A[M][N], sum[M];

int total;

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
   int i, j, status;

   printf("main: initialize A matrix\n");

   for (i=0; i < M; i++){
     for (j=0; j < N; j++){
       A[i][j] = i + j + 1;
     }
   }

   print();

	 printf("main: compute total sequentially\n");
   for (i=0; i < M; i++){
     for (j=0; j < N; j++){
       total += A[i][j];
     }
   }
   printf("total = %ld\n", total);

   gettimeofday(&t2, NULL);
   printf("seconds elapsed = %d\n", t2.tv_sec-t1.tv_sec);
   printf("microseconds elapsed = %d\n", t2.tv_usec-t1.tv_usec);
   pthread_exit(NULL);
} 

/* itimer.c program */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>

/*************************
 struct timeval {
    time_t      tv_sec;         // seconds 
    suseconds_t tv_usec;        // microseconds 
 };
 struct itimerval {
    struct timeval it_interval; // Interval of periodic timer 
    struct timeval it_value;    // Time until next expiration
 };
*********************/

int hh, mm, ss, tick, update;

void timer_handler (int sig)
{
   tick++;
   if(tick > 999)
   {
      ss++;
      update = 1;
      tick = 0;
   }
   if(ss == 60)
   {
      mm++;
      ss = 0;
   }
   if(mm == 60)
   {
      hh++;
      mm = 0;
   }
   if(hh == 24)
   {
      hh = 0;
   }
   if(1)
   {
      printf("%.2d : %.2d : %.2d\r",hh, mm, ss );
      update = 0;
   }
}

int main ()
{
 struct itimerval itimer;
 tick = hh = mm = ss = update = 0;
 
 signal(SIGALRM, &timer_handler);
 
 /* Configure the timer to expire after .5 sec */
 itimer.it_value.tv_sec  = 0;
 itimer.it_value.tv_usec = 500000;

 /* and every .001 sec after that */
 itimer.it_interval.tv_sec  = 0;
 itimer.it_interval.tv_usec = 1000;

 setitimer (ITIMER_REAL, &itimer, NULL);

 while (1);
}


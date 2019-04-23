#include <stdio.h>
#include <stdlib.h>

typedef unsigned int u32;

char *ctable = "0123456789ABCDEF";

int rpu(u32 x, int BASE){
	char c;
	if (x){
		c = ctable[x % BASE];
		rpu(x / BASE, BASE);
		putchar(c);
	}
}

int printu(u32 x){
	(x==0)? putchar('0') : rpu(x, 10);
	putchar(' ');
}


int printd(int x){
	if(x < 0){
		putchar('-');
		x = x * -1;
	}
	(x==0)? putchar('0') : rpu(x, 10);
       	putchar(' ');
}

int  printx(u32 x){
	putchar('0');
	putchar('x');
	(x==0)? putchar('0') : rpu(x, 16);
       	putchar(' ');
}

int  printo(u32 x){
	putchar('0');
	(x==0)? putchar('0') : rpu(x, 8);
       	putchar(' ');
}

int prints(char *x){
	char c;
	int i = 0;
	c = x[i];
	while(c != NULL){
		putchar(c);
		i++;
		c = x[i];
	}	
}

int myprintf(char *fmt, ...){
	//printf("test?");
	char cp;
	int *ip = (int *)(&fmt);
	int i = 0;
	cp = fmt[i];
	while(cp != NULL){
		if(cp == '%'){
			i++;
			ip++;
			cp = fmt[i];
			//printf("c=%c ip=%d\n", cp, ip);
			switch(cp){	
				case 'c':
					putchar(*ip);
					break;
				case 's':
					prints(*ip);
					break;
				case 'u':
					printu(*ip);
					break;
				case 'd':
					printd(*ip);
					break;
				case 'o':
					printo(*ip);
					break;
				case 'x':
					printx(*ip);
					break;
			}
		}
		else{
			putchar(cp);
		}
		i++;
		cp = fmt[i];
	}
}

int main(int argc, char *argv[ ], char *env[ ]){
	int i = 0;
	myprintf("cha=%c string=%s      dec=%d hex=%x oct=%o neg=%d\n", 'a', "this is a test", 100,    100,   100,  -100);
	//myprintf("test1\n");
	//myprintf("test2\n");
	myprintf("\n");
	myprintf("%d\n", argc);
	while(argv[i] != NULL){
		myprintf("%s\n", argv[i]);
		i++;
	}
	i = 0;
	while(env[i] != NULL && argv[1] == '1'){
		myprintf("%s\n", env[i]);
		i++;
	}
}


// The echo client client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netdb.h>

#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX 256
#define DIR_PERM 0775   // drwxrwxr-x
#define FILE_PERM 0664  // -rw-rw-r--

// Define variables
struct hostent *hp;              
struct sockaddr_in  server_addr; 

int server_sock, r;
int SERVER_IP, SERVER_PORT; 

// functions
char* my_pwd(char* pathname)
{
  char buf[MAX];
  char *cwd;
  
  getcwd(buf, sizeof(buf));
  cwd = (char*)malloc((strlen(buf) + 1) * sizeof(char));
  strcpy(cwd, buf);
  return cwd;
}

char* my_cat(char* pathname)
{
    int fd;
    int i, n;
    char buf[1024];
    
    fd = open(pathname, O_RDONLY);
    
    while (n = read(fd, buf, 1024))
    {
        for (i = 0; i < n; i++)
        {
            putchar(buf[i]);
        }
    }
    
    return buf;
}

char* my_ls(char* pathname)
{
    DIR *d;
    char* perm = "rwxrwxrwx";
    char cwd[MAX], buf[MAX], subbuf[MAX];
    struct dirent *ent;
    struct stat s;
    char* result = NULL;
    char* check = NULL;
    int empty = 0, curSize = 1;
    
    d = opendir(pathname);
    if(d == NULL)
        printf("error\n");
    while ((ent = readdir(d)) != NULL)
    {
        bzero(buf, MAX);
        lstat(ent->d_name, &s);
        switch (s.st_mode & S_IFMT) {
        case S_IFREG:  buf[0] = '-';     break;
        case S_IFDIR:  buf[0] = 'd';     break;
        case S_IFLNK:  buf[0] = 'l';     break;
        default:       buf[0] = '?';     break;
        }
        for (int i = 0; i < strlen(perm); i++)
        {
            buf[i + 1] = ((s.st_mode & 1 << i) ? perm[i] : '-');
        }
        
        sprintf(subbuf, " %3ld ", (long)s.st_nlink); // link count
        strcat(buf, subbuf);

        sprintf(subbuf, " %5ld  %5ld ", (long)s.st_uid, (long)s.st_gid); // uid/gid 
        strcat(buf, subbuf);

        sprintf(subbuf, " %6lld ", (long long)s.st_size); // size
        strcat(buf, subbuf);

        sprintf(subbuf, " %26s", ctime(&s.st_ctime)); // time/date
        strcat(buf, subbuf);
        buf[strlen(buf) - 1] = 0;

        sprintf(subbuf, "  %s", ent->d_name); // name
        strcat(buf, subbuf);

        if (buf[0] == 'l')
        {
            strcat(buf, " -> ");
            bzero(subbuf, MAX);
            readlink(ent->d_name, subbuf, MAX);
            strcat(buf, subbuf);
        }

        strcat(buf, "\n");
        curSize += strlen(buf) * sizeof(char);
        check = (char*)realloc(result, curSize);
        if (check != NULL)
        {
            result = check;
            if(empty == 0)
            {
                empty++;
                strcpy(result, buf);
            }
            else
                strcat(result, buf);
        }
        else
            printf("Error!\n");
    }
    result[strlen(result) - 1] = 0; // remove last newline
    return result;
    
}

char* my_cd(char* pathname)
{
    char* ans;
    char buf[MAX];
    if (chdir(pathname) != 0)
        strcpy(buf, "Failure");
    else
        strcpy(buf, "Success");
    ans = (char*)malloc((strlen(buf) + 1) * sizeof(char));
    strcpy(ans, buf);
    return ans;
}
char* my_mkdir(char* pathname)
{
    char* ans;
    char buf[MAX];
    if (mkdir(pathname, DIR_PERM) != 0)
        strcpy(buf, "Failure");
    else
        strcpy(buf, "Success");
    ans = (char*)malloc((strlen(buf) + 1) * sizeof(char));
    strcpy(ans, buf);
    return ans;
}

char* my_rmdir(char* pathname)
{
    char* ans;
    char buf[MAX];
    if (rmdir(pathname) != 0)
        strcpy(buf, "Failure");
    else
        strcpy(buf, "Success");
    ans = (char*)malloc((strlen(buf) + 1) * sizeof(char));
    strcpy(ans, buf);
    return ans;
}

char* my_rm(char* pathname)
{
    char* ans;
    char buf[MAX];
    if (remove(pathname) != 0)
        strcpy(buf, "Failure");
    else
        strcpy(buf, "Success");
    ans = (char*)malloc((strlen(buf) + 1) * sizeof(char));
    strcpy(ans, buf);
    return ans;
}

// clinet initialization code

int client_init(char *argv[])
{
  printf("======= clinet init ==========\n");

  printf("1 : get server info\n");
  hp = gethostbyname(argv[1]);
  if (hp==0){
     printf("unknown host %s\n", argv[1]);
     exit(1);
  }

  SERVER_IP   = *(long *)hp->h_addr;
  SERVER_PORT = atoi(argv[2]);

  printf("2 : create a TCP socket\n");
  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock<0){
     printf("socket call failed\n");
     exit(2);
  }

  printf("3 : fill server_addr with server's IP and PORT#\n");
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = SERVER_IP;
  server_addr.sin_port = htons(SERVER_PORT);

  // Connect to server
  printf("4 : connecting to server ....\n");
  r = connect(server_sock,(struct sockaddr *)&server_addr, sizeof(server_addr));
  if (r < 0){
     printf("connect failed\n");
     exit(1);
  }

  printf("5 : connected OK to \007\n"); 
  printf("---------------------------------------------------------\n");
  printf("hostname=%s  IP=%s  PORT=%d\n", 
          hp->h_name, inet_ntoa(SERVER_IP), SERVER_PORT);
  printf("---------------------------------------------------------\n");

  printf("========= init done ==========\n");
}

int getCmdIndex(char * cmdname)
{
    int index = 0;
    char* cmdNames[] = { "lcat", "lpwd", "lls", "lcd", "lmkdir", "lrmdir", "lrm" };
    while (index <= 6)
    {
        if(strcmp(cmdname, cmdNames[index]) == 0)
            return index;
        index++;
    }
    return -1;
}

main(int argc, char *argv[ ])
{
  int n, filesize, count, fd;
  char line[MAX], ans[MAX], cmdname[MAX], pathname[MAX], buf[MAX];
  char* output;
  char* (*fptr[])(char *) = { (char* (*)())	my_cat, my_pwd, my_ls, my_cd, my_mkdir, my_rmdir, my_rm };
  if (argc < 3){
     printf("Usage : client ServerName SeverPort\n");
     exit(1);
  }

  client_init(argv);
  // sock <---> server
    printf("\n");
    printf("********************** menu *********************\n");
    printf("* get  put  ls   cd   pwd   mkdir   rmdir   rm  *\n");
    printf("* lcat     lls  lcd  lpwd  lmkdir  lrmdir  lrm  *\n");
    printf("*************************************************\n");
  while (1){
    printf("input a line : ");
    bzero(line, MAX);                // zero out line[ ]
    fgets(line, MAX, stdin);         // get a line (end with \n) from stdin

    line[strlen(line)-1] = 0;        // kill \n at end
    if (line[0]==0)                  // exit if NULL line
       exit(0);
    

    sscanf(line, "%s %s", cmdname, pathname); // parse input, ignoring whitespace
    char temp[MAX];
    strcpy(temp, line);
    strtok(temp, " ");
    if(strtok(NULL, " ") == NULL)
        strcpy(pathname, "./");
	
	if (strcmp(cmdname, "quit") == 0)
	{
		printf("\n");
		exit(EXIT_SUCCESS);
	}
    else if (strcmp(cmdname, "put") == 0)
    {
        write(server_sock, line, MAX);
        struct stat s;

        if (stat(pathname, &s) != 0)
        {
            write(server_sock, "BAD", MAX);
            continue;
        }
        sprintf(ans, "SIZE=%d", s.st_size);
        write(server_sock, ans, MAX);

        fd = open(pathname, O_RDONLY);
        while (n = read(fd, buf, MAX))
        {
            write(server_sock, buf, n);
        }

        close(fd);
    }
    else if (strcmp(cmdname, "get") == 0)
    {
        write(server_sock, line, MAX);

        read(server_sock, ans, MAX); //receive size/bad
        if (strcmp(ans, "BAD") == 0) { printf("BAD\n"); continue; }

        sscanf(ans, "SIZE=%d", &filesize);

        count = 0;
        fd = open(pathname, O_WRONLY | O_CREAT, 0664);
        while (count < filesize)
        {
            n = read(server_sock, buf, MAX - 1);
            count += n;
            write(fd, buf, n);
        }
        close(fd);
    }
	else
	{
        int index = getCmdIndex(cmdname);
	    if (index >= 0)
        {
		    
            output = fptr[index](pathname);
		    if (output != NULL)
		    {
			    printf("%s\n", output);
		    }
	    }
	    else
	    {
			write(server_sock, line, MAX);

			read(server_sock, ans, MAX); //receive size/bad
			if (strcmp(ans, "BAD") == 0) { printf("BAD\n"); continue; }

			sscanf(ans, "SIZE=%d", &filesize);

			count = 0;
			while (count < filesize)
			{
				n = read(server_sock, buf, MAX - 1);
				buf[MAX - 1] = 0;
				count += n;
				printf("%s", buf);
			}
			printf("\n");
	    }	
	}
  }
}



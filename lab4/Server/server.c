// This is the echo SERVER server.c
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

// Define variables:
struct sockaddr_in  server_addr, client_addr, name_addr;
struct hostent *hp;

int  mysock, client_sock;              // socket descriptors
int  serverPort;                     // server port number
int  r, length, n;                   // help variables

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


// Server initialization code:

int server_init(char *name)
{
   printf("==================== server init ======================\n");   
   // get DOT name and IP address of this host

   printf("1 : get and show server host info\n");
   hp = gethostbyname(name);
   if (hp == 0){
      printf("unknown host\n");
      exit(1);
   }
   printf("    hostname=%s  IP=%s\n",
               hp->h_name,  inet_ntoa(*(long *)hp->h_addr));
  
   //  create a TCP socket by socket() syscall
   printf("2 : create a socket\n");
   mysock = socket(AF_INET, SOCK_STREAM, 0);
   if (mysock < 0){
      printf("socket call failed\n");
      exit(2);
   }

   printf("3 : fill server_addr with host IP and PORT# info\n");
   // initialize the server_addr structure
   server_addr.sin_family = AF_INET;                  // for TCP/IP
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   // THIS HOST IP address  
   server_addr.sin_port = 0;   // let kernel assign port

   printf("4 : bind socket to host info\n");
   // bind syscall: bind the socket to server_addr info
   r = bind(mysock,(struct sockaddr *)&server_addr, sizeof(server_addr));
   if (r < 0){
       printf("bind failed\n");
       exit(3);
   }

   printf("5 : find out Kernel assigned PORT# and show it\n");
   // find out socket port number (assigned by kernel)
   length = sizeof(name_addr);
   r = getsockname(mysock, (struct sockaddr *)&name_addr, &length);
   if (r < 0){
      printf("get socketname error\n");
      exit(4);
   }

   // show port number
   serverPort = ntohs(name_addr.sin_port);   // convert to host ushort
   printf("    Port=%d\n", serverPort);

   // listen at port with a max. queue of 5 (waiting clients) 
   printf("5 : server is listening ....\n");
   listen(mysock, 5);
   printf("===================== init done =======================\n");
}

int getCmdIndex(char * cmdname)
{
    int index = 0;
    char* cmdNames[] = { "pwd", "ls", "cd", "mkdir", "rmdir", "rm" };
    while (index <= 5)
    {
        if(strcmp(cmdname, cmdNames[index]) == 0)
            return index;
        index++;
    }
    return -1;
}

main(int argc, char *argv[])
{
    char *hostname;
    char line[MAX], cmdname[MAX], pathname[MAX], ans[MAX], buf[MAX];
    char* (*fptr[])(char *) = { (char* (*)())	my_pwd, my_ls, my_cd, my_mkdir, my_rmdir, my_rm };
    char* output;
    int fd, count, filesize;    
    
        
   if (argc < 2)
      hostname = "localhost";
   else
      hostname = argv[1];
 
   server_init(hostname);
   chroot(my_pwd("")); 

   // Try to accept a client request
   while(1){
     printf("server: accepting new connection ....\n"); 

     // Try to accept a client connection as descriptor newsock
     length = sizeof(client_addr);
     client_sock = accept(mysock, (struct sockaddr *)&client_addr, &length);
     if (client_sock < 0){
        printf("server: accept error\n");
        exit(1);
     }
     printf("server: accepted a client connection from\n");
     printf("-----------------------------------------------\n");
     printf("        IP=%s  port=%d\n", inet_ntoa(client_addr.sin_addr.s_addr),
                                        ntohs(client_addr.sin_port));
     printf("-----------------------------------------------\n");

     // Processing loop: newsock <----> client
     while(1){
        bzero(line, MAX); bzero(cmdname, MAX); bzero(pathname, MAX); 
        n = read(client_sock, line, MAX);
        if (n==0){
           printf("server: client died, server loops\n");
           close(client_sock);
           break;
        }
        char temp[MAX];
        strcpy(temp, line);
        sscanf(line, "%s %s", cmdname, pathname); // parse input, ignoring whitespace
        strtok(temp, " ");
        if(strtok(NULL, " ") == NULL)
            strcpy(pathname, "./");
      
      if (strcmp(cmdname, "put") == 0)
      {
        
        read(client_sock, ans, MAX); //receive size/bad
        if (strcmp(ans, "BAD") == 0) { printf("BAD\n"); continue; }

        sscanf(ans, "SIZE=%d", &filesize);
        count = 0;
        fd = open(pathname, O_WRONLY | O_CREAT, 0664);
        while (count < filesize)
        {
            n = read(client_sock, buf, MAX - 1);
            count += n;
            write(fd, buf, n);
        }
        close(fd);
      }
      else if (strcmp(cmdname, "get") == 0)
      {
        struct stat s;

        if (stat(pathname, &s) != 0)
        {
            write(client_sock, "BAD", MAX);
            continue;
        }
        sprintf(ans, "SIZE=%d", s.st_size);
        write(client_sock, ans, MAX);

        fd = open(pathname, O_RDONLY);
        while (n = read(fd, buf, MAX))
        {
            write(client_sock, buf, n);
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
                sprintf(ans, "SIZE=%d", strlen(output) + 1);
                write(client_sock, ans, MAX);

                write(client_sock, output, strlen(output) + 1);

                free(output);
            }
            else
            {
                write(client_sock, "BAD", MAX);
            }
        }
        else
        {
            char temp[MAX];
            sprintf(temp, "Error: Command '%s' is not supported.", cmdname);
            sprintf(ans, "SIZE=%d", strlen(temp) + 1);
            write(client_sock, ans, MAX);

            write(client_sock, temp, strlen(temp) + 1);
        }
      }
    }
 }
}


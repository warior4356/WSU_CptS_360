#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include<sys/wait.h> 

char *home;
void parseCommand(char command[1024]), processPipe(char *commands[1024]);
int nativeCommand(char *cmd, char *arg);
int main(int argc, char *argv[], char *env[])
{
	char input[1024], *commands[1024], *prompt = "shell$ ";
	int i = 0, j = 1;
	home = getenv("HOME"); // get home directory

	//printf("Type 'shellhelp' for help.\n");
	printf("%s", prompt);
	// get input each cycle
	while (fgets(input, 1024, stdin) != NULL)
	{
		if (strcmp(input, "") != 0 && strcmp(input, "\n") != 0) // valid?
		{
			// tokenize input using pipes
			commands[0] = strtok(input, "|");
			while (commands[j] = strtok(0, "|"))
				j++;

			if (j == 1) // single command
			{
				parseCommand(commands[0]);
			}
			else // multiple commands 
			{
				processPipe(commands);
			}
		}
		// reset variables and repeat input question
		i = 0;		
		j = 1;
		memset(input, 0, sizeof(input));
		printf("%s", prompt);
	}
	return(0);
}

void parseCommand(char command[1024])
{
	char *args[512], *cmd, *arg;
	int pid, status, redir = 0, rediri = 0, i = 1, builtin = 0;
	command[strlen(command) - 1] = 0;

	// get actual command and args
	cmd = strtok(command, " ");
	arg = strtok(0, " ");
	
	if(!nativeCommand(cmd, arg))
	{
		if(cmd == NULL)
			return;
		args[0] = cmd;
		while (arg != NULL)
		{
			args[i] = arg;
			// handle redirects
			if (strcmp(arg, "<") == 0) // input
			{
				rediri = i;
				redir = 1;
			}
			else if (strcmp(arg, ">") == 0) // output
			{
				rediri = i;
				redir = 2;
			}
			else if (strcmp(arg, ">>") == 0) // output and append
			{
				rediri = i;
				redir = 3;
			}
			arg = strtok(0, " ");
			i++;
		}

		if (rediri)
			args[rediri] = NULL;
		else
			args[i] = NULL;

		pid = fork();
		if (!pid)
		{
			switch(redir){
				case 1 :
					close(0);
					open(args[i - 1], O_RDONLY);
					break;
				case 2 :
					close(1);
					open(args[i - 1], O_WRONLY | O_CREAT, 0644);
				 	break;
				case 3 :
					close(1);
					open(args[i - 1], O_WRONLY | O_APPEND);
					break;		
			}
			execvp(cmd, args);
			printf("couldn't execute: %s", cmd);
		}
		else if (pid)
		{
			pid = wait(&status);
			printf("Child Exit Code: %d\n", status);
		}
	}
}

void processPipe(char *commands[1024])
{
	int p[2], pid, saved_stdout = dup(1), saved_stdin = dup(0);
	pid_t child_id;
	if(strcmp(commands[0], "") == 0)
		return;
	if (pipe(p) < 0) 
  	exit(1);
	//printf("pipe\n");
  child_id = fork();
//	if(child_id < 0)
//		exit(-1);
	if(child_id)
	{
    close(p[0]);
		close(1);    
		dup(p[1]);
    close(p[1]);
		parseCommand(commands[0]);
		//exit(1);
	}
  else
	{
    close(p[1]);
		close(0);
    dup(p[0]);
    close(p[0]);
		parseCommand(commands[1]);
		//processPipe(commands + 1);
		//exit(1);
		/*close(p[1]);
		if(strcmp(commands[2], "") != 0){
			dup2(p[0], 0);
			processPipe(commands + 1);
		}
		else
			parseCommand(commands[1]);*/
	}
  //close(p[0]);
  //close(p[1]);
	dup2(saved_stdin, 0);
	dup2(saved_stdout, 1);
	close(saved_stdout);
	close(saved_stdin);
  //wait(0);
  //wait(0);
}

int nativeCommand(char *cmd, char *arg)
{
	int done = 0;
	// cd command
	if (strcmp(cmd, "cd") == 0)
		{
		done = 1;
		// no directory specified and if home directory is specified
		if (!arg)
		{
			if (home)
			{
				chdir(home);
			}
		}
		// directory specified
		else
			chdir(arg);
	}
		else if (strcmp(cmd, "exit") == 0)
	{
		exit(1);
		done = 1;
	}
	return done;
}






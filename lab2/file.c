#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct NODE
{
	char name[64];
	char type;
	struct NODE *childPtr, *siblingPtr, *parentPtr;
};

struct NODE *root, *cwd, *currentParent;
char line[128];
char command[16], pathname[64];
char dname[64][64], bname[6];
int state = 1;
char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat", "rm",
 "reload", "save", "menu", "quit", NULL};


int findCmd(char *command)
{
	int i = 0;
	while(cmd[i]){
		if (!strcmp(command, cmd[i]))
			return i; // found command: return index i
		i++;
	}
	return -1; // not found: return -1
}

struct NODE* newNode(char name[64], char type, struct NODE* parent)
{
	struct NODE* node = (struct NODE*)malloc(sizeof(struct NODE));

	strcpy(node->name, name);
	node->type = type;

	node->childPtr = NULL;
        node->siblingPtr = NULL;
	node->parentPtr = parent;

	return(node);
}	

void initialize()
{
	printf("Root initilaized OK\n");
	root = newNode("/", 'D', NULL);
	cwd = root;
	root->parentPtr = root;
}

void breakPathname(int getBname)
{
	int i = 0;
	char *temp;
	memset(bname, '\0', sizeof(bname));
	temp = strtok(pathname, "/");
	while(temp != NULL)
	{	
		strcpy(dname[i], temp);
		temp = strtok(NULL, "/");	
		i++;
	}
	i--;
	if(getBname)
	{
	strcpy(bname, dname[i]);
	memset(dname[i], '\0', sizeof(dname[i]));
	}
}

struct NODE* searchDir(struct NODE* node, char *name)
{
	if(node == NULL)
		return NULL;
	if(!strcmp(node->name,  name))
		return node;
	else
		return searchDir(node->siblingPtr, name);
}

struct NODE* getDnameEnd(int getBname, int file)
{
	struct NODE *current = cwd;
	int i = 0;
	currentParent = cwd;
	if(pathname[0] == '/') 
	{
		current = root;
		//i = 1;
	}
	breakPathname(getBname);
	for(; dname[i][0] != '\0'; i++)
	{
		if(!strcmp(dname[i], ".."))
		{
			if(current->parentPtr)
				current = current->parentPtr;
			else
			{
				printf("Bad path. Nonexist.\n");
				return NULL;
			}
		}
		else
		{
			if(!current->childPtr && bname)
			{
				printf("Bad path. Nonexist.\n");
				return NULL;
			}
			current = current->childPtr;
			currentParent = current;
			current = searchDir(current, dname[i]);
			if(!current)
			{
				printf("Bad path. Nonexist.\n");
				return NULL;
			}
			else if(current->type != 'D' && !file)
			{
				printf("Not a Dir.\n");
				return NULL;
			}
		}
	}
	return current;
}

int mkdir()
{
	struct NODE *current = getDnameEnd(1, 0);
	if(!current)
		return 0;
	if(current->childPtr != NULL)
	{
		current = current->childPtr;
		if(searchDir(current, bname))
		{
			printf("Already Exists\n");
			return 0;
		}
		while(current->siblingPtr != NULL)
		{
			current = current->siblingPtr;
		}
		current->siblingPtr = newNode(bname, 'D', currentParent);
	}
	else
		current->childPtr = newNode(bname, 'D', currentParent);
}

int rmdir(char path[64])
{
	struct NODE *toRemove, *temp = NULL, *current = getDnameEnd(0, 0);
	if(!current)
		return 0;
	if(current->type != 'D')
	{
		printf("Not a dir.\n");
		return 0;
	}
	if(current->childPtr)
	{
		printf("Dir not empty.\n");
		return 0;
	}
	if(current->parentPtr == current)
	{
		printf("Can not delete root.\n");
		return 0;
	}
	if(current->siblingPtr)
		temp = current->siblingPtr;
	toRemove = current;
	current = current->parentPtr;
	if(toRemove == cwd)
		cwd = current;
	if(current->childPtr == toRemove)
	{
		current->childPtr = temp;
	}
	else
	{
		current = current->childPtr;
		while(current && current->siblingPtr != toRemove)
		{
			current = current->siblingPtr;
		}
		if(current->siblingPtr == toRemove)
			current->siblingPtr = temp;
		else
		{
			printf("Bad path");
			return 0;
		}
	}
}

int cd()
{
	cwd = getDnameEnd(0, 0);
	currentParent = cwd;
}

int ls()
{
	struct NODE *current = cwd->childPtr;
	if(pathname[0] != '\0')
		current = getDnameEnd(0, 0)->childPtr;
	if(!current)
		return 0;
	while(current)
	{
		printf("%s ", current->name);
		current = current->siblingPtr;
	}
	printf("\n");
}

void getPath(struct NODE *loc)
{
	if(loc == NULL)
		return;
	if(loc != loc->parentPtr)
		getPath(loc->parentPtr);
	if(loc != root)
	{
		strcat(pathname, "/");
		strcat(pathname, loc->name);
	}
}

int pwd()
{
	strcpy(pathname, "");
	if(cwd == root)
		strcat(pathname, "/");
	else
		getPath(cwd);
	printf("%s\n", pathname);
}

int creat()
{
	struct NODE *current = getDnameEnd(1, 1);
	if(!current)
		return 0;
	if(current->childPtr != NULL)
	{
		current = current->childPtr;
		if(current->name == bname)
		{
			printf("Already exists.");
			return 0;
		}
		while(current->siblingPtr != NULL)
		{
			current = current->siblingPtr;
		}
		current->siblingPtr = newNode(bname, 'F', currentParent);
	}
	else
		current->childPtr = newNode(bname, 'F', currentParent);
}

int rm()
{
	struct NODE *toRemove, *temp = NULL, *current = getDnameEnd(0, 1);
	if(!current)
		return 0;
	if(current->type != 'F')
	{
		printf("Not a file.\n");
		return 0;
	}
	if(current->siblingPtr)
		temp = current->siblingPtr;
	toRemove = current;
	current = current->parentPtr;
	if(toRemove == cwd)
		cwd = current;
	if(current->childPtr == toRemove)
	{
		current->childPtr = temp;
	}
	else
	{
		current = current->childPtr;
		while(current && current->siblingPtr != toRemove)
		{
			current = current->siblingPtr;
		}
		if(current->siblingPtr == toRemove)
			current->siblingPtr = temp;
		else
		{
			printf("Bad path");
			return 0;
		}
	}
}
void saveToFile(FILE *fp, struct NODE * loc)
{
	if(!loc)
		return;
	strcpy(pathname, "");
	if(loc == root)
		strcat(pathname, "/");
	else
		getPath(loc);
	fprintf(fp, "%c %s\n", loc->type, pathname);
	//printf("%c %s\n", loc->type, pathname);
	saveToFile(fp, loc->childPtr);
	saveToFile(fp, loc->siblingPtr);
}

int save()
{
	FILE * fp;
	fp = fopen (pathname,"w");
	saveToFile(fp, root);	
	fclose(fp);
}

int reload()
{
	FILE *fp;
	fp = fopen(pathname, "r");
	char type[6];
	while(!feof(fp))
	{
		if(fscanf(fp, "%s %s", type, pathname) == 2)
		{
		//printf("type=%s path=%s\n", type, pathname);
		if(type[0] == 'D' && strcmp(pathname, "/") != 0)
			mkdir();
		if(type[0] == 'F')
			creat();
		}
	}
	fclose(fp);
}

int menu()
{
	printf("======================= MENU ========================\n");
	printf("mkdir rmdir ls  cd  pwd  creat  rm  save reload  quit\n");
	printf("=====================================================\n");
}

int quit()
{
	//if(pathname[0] == '\0')
	strcpy(pathname, "backup");
	save();
	state = 0;
}

int (*fptr[ ])(char *)={(int (*)())mkdir,rmdir,ls,cd,pwd,creat,rm,reload, save, menu, quit};

int main()
{
	initialize(); //initialize root node of the file system tree
	printf("Enter \"menu\" for valid Commands\n");
	while(state){
		memset(command, '\0', sizeof(command));
		memset(pathname, '\0', sizeof(pathname));
		printf("Command: ");	
		fgets(line, 128, stdin);
		line[strlen(line)-1] = 0;
		sscanf(line, "%s %s", command, pathname);
		int index = findCmd(command);
		int r = fptr[index](pathname);
	}
}


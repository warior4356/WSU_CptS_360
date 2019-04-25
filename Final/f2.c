#include "header2.h"
#include "util.h"
#include "header.h"

/* Start Functions 2  */
/* Releases MINODE's data blocks. Up to 12 direct, 256 indirect, 256*256 double indirect
Updates time, sets size to 0, sets dirty to 1 */
int my_truncate(MINODE *mip) 
{
  int buf[256];
  int buf2[256];
  int bnumber, i, j;

  if(mip == NULL) //Make sure mip exists
  {
    printf("Error: No file.\n");
    return -1;
  }
  //Deallocate for direct
  for(i = 0; i < 12; i++)
  {
    if(mip->INODE.i_block[i] != 0)
    {
      bdalloc(mip->dev, mip->INODE.i_block[i]);
    }
  }
  //Deallocate Indirect blocks
  if(mip->INODE.i_block[12] != 0) 
  {
    get_block(dev, mip->INODE.i_block[12], (char*)buf);
    for(i = 0; i < 256; i++)
    {
      //Deallocate Each Indirect Block
      if(buf[i] != 0) {bdalloc(mip->dev, buf[i]);}
    }
    //Deallocate Direct block
    bdalloc(mip->dev, mip->INODE.i_block[12]); 
    //Deallocate Double Indirect blocks
    if(mip->INODE.i_block[13] != 0) 
    {
      memset(buf, 0, 256);
      get_block(mip->dev, mip->INODE.i_block[13], (char*)buf);
      for(i = 0; i < 256; i++)
      {
        if(buf[i])
        {
          get_block(mip->dev, buf[i], (char*)buf2);
          for(j = 0; j < 256; j++)
          {
            //Deallocate Each Double Indirect Block
            if(buf2[j] != 0) {bdalloc(mip->dev, buf2[j]);}
          }
          //Deallocate Each Indirect Block
          bdalloc(mip->dev, buf[i]);
        }
      }
      //Deallocate Direct block
      bdalloc(mip->dev, mip->INODE.i_block[13]);
    }
  }
  mip->INODE.i_atime = mip->INODE.i_mtime = time(0L); //Update Time
  mip->INODE.i_size = 0; //Set Size 0
  mip->dirty = 1; //Set Dirty (Dirt Means Modified)
  return 1;
}

/* Given a string will get path and mode. Verfies is regular file. 
Does not open twice other than two reads. Makes OFT for file. Sets offset based on mode.
Places file in smallest open fd. Updates time. Returns fd.*/
int open_file(char *pathname)
{
  char pathfile[256], mode[256];
  int flags, ino, i, dev;
  MINODE *mip;
  
  OFT *oftp;
  if(split_paths(pathname, pathfile, mode) <= 0) { return -1; }
  //Set Mode Flag
  else if(strcmp("R", mode) == 0){flags = R;}
  else if(strcmp("W", mode) == 0){flags = W;}
  else if(strcmp("RW", mode) == 0){flags = RW;}
  else if(strcmp("APPEND", mode) == 0){flags = APPEND;}
  else
  {
    printf("No mode given.\n");
    return -1;
  }
  char parent[256], child[256], origPathname[512];
  //Zeros out strings, possibly redundant
  memset(parent, 0, 256);
  memset(child, 0, 256);
  memset(origPathname, 0, 512);

  strcpy(origPathname, pathfile);
  //check root or dir
  if(!strcmp(pathfile,"")){
    printf("Missing operand\n"); 
    return -1;
  }
  if(pathfile[0] == '/') { dev = root->dev; }
  else { dev = running->cwd->dev; }

  dname(pathfile, parent); //Get directory
  bname(origPathname, child); //Get basename
  ino = getino(dev, parent);
  if(ino <= 0)
  {
    printf("Invalid.\n");
    return -1;
  }
  mip = iget(dev, ino);
  ino = search(dev, child, &(mip->INODE));

  if(ino <= 0)
  {
    if(flags != W) //Only create file if write
    {
    printf("Open file Err\n");
    return -1;
    }
    my_creat(child);
    ino = getino(dev, child);
  }

  mip = iget(dev, ino); //Makes MINODE, returns pointer
  if(!S_ISREG(mip->INODE.i_mode)) //Verifies not link or dir
  {
    printf("Not a regular file.\n");
    iput(mip->dev, mip);
    return -1;
  }
  //Check file is not already open
  for(i = 0; i < 10; i++)
  {
    if(running->fd[i] != NULL)
    {
      if(running->fd[i]->inodeptr == mip)
      {
        if(running->fd[i]->mode != 0 || flags != 0)
        {
          printf("File in fd.\n");
          iput(mip->dev, mip);
          return -1;
        }
      }
    }
  }

  //allocate opft
  oftp = (OFT *)malloc(sizeof(OFT));
  oftp->mode = flags;
  oftp->refCount = 1;
  oftp->inodeptr = mip;
  //set offset 
  switch(flags)
  {
    case 0: oftp->offset = 0;
            printf("File opened for read\n");
            my_touch(pathfile);
            break;
    case 1: my_truncate(oftp->inodeptr);
            printf("File open for write\n");
            oftp->offset = 0;
            my_touch(pathfile);
            break;
    case 2: oftp->offset = 0;
            printf("File open for rw\n");
            my_touch(pathfile);
            break;
    case 3: oftp->offset = mip->INODE.i_size;
            printf("File open for append\n");
            my_touch(pathfile);
            break;
    default: printf("Invalid\n");
              iput(mip->dev, mip);
              free(oftp);
              return -1;
              //break;
  }

  //Find first empty FD in running PROC
  i = 0;
  while(running->fd[i] != NULL && i < 10){ i++; }
  if(i == 10) //fd full
  {
    printf("running->fd is full!\n");
    iput(mip->dev, mip);
    free(oftp);
    return -1;
  }
  //else
  running->fd[i] = oftp;
  if(flags != 0) {mip->dirty = 1;}
  //printf("i = %d", i);
  return i;
}

//Wrapper for parsing pathname passes int fd into my_close
int close_file(char *pathname)
{
  if(pathname == NULL)// no fd
  {
    printf("No file desciptor\n");
    return -1;
  }
  int fd = atoi(pathname);
  return my_close(fd);
}

/* Verfies fd. Checks if only user using fd. If yes dispose of MINODE */
int my_close(int fd)
{
  MINODE *mip;
  OFT *oftp;
  //printf("fd = %d", fd);
  if(fd < 0 || fd > 9)
  {
    printf("Fd invalid\n");
    return -1;
  }
  if(running->fd[fd] == NULL)
  {
    printf("File not open\n");
    return -1;
  }

  //close
  oftp = running->fd[fd];
  running->fd[fd] = 0;
  oftp->refCount--;
  printf("refcount = %d", oftp->refCount);
  if(oftp->refCount > 0) {return -1;} //Verifies is only instance open
  mip = oftp->inodeptr;
  iput(mip->dev, mip);
  free(oftp);
  printf("\nFile closed.\n");
  return 1;
}
//Wrapper for my_write to verify fd exists and get user input
int write_file(char *pathname)
{
  int fd;
  char writeMe[BLKSIZE];
  fd = atoi(pathname);
  if(fd < 0 || fd > 9)
  {
    printf("No FD\n");
    return -1;
  }
  if(running->fd[fd] == NULL)
  {
    printf("No FDa\n");
    return -1;
  }

  //check mode
  if(running->fd[fd]->mode == 0)
  {
    printf("File in read mode\n");
    return -1;
  }

  printf("Write: ");
  fgets(writeMe, BLKSIZE, stdin);
  writeMe[strlen(writeMe) -1] = 0;
  printf("writeme = %s",writeMe);
  if(writeMe[0] == 0)
  {
    return 0;
  }
  return my_write(fd, writeMe, strlen(writeMe));
}

/* Using offset start at first non full block, allocating new blocks as needed
block 12 is a list of pointers to blocks
block 13 is a list of pointers to lists of pointers to blocks
then write either all remaining space in block or all remaining info whichever is smaller
repeat until all info written or all blocks full
flag block dirty*/
int my_write(int fd, char *buf, int nbytes)
{
  MINODE *mip; 
  OFT *oftp;
  int count, lblk, start, blk, dblk, remain, offset, iblk;
  int ibuf[256], dbuf[256];
  char writeBuf[BLKSIZE], *cp, *cq = buf;
  count = 0;
  oftp = running->fd[fd];
  mip = oftp->inodeptr;
  while(nbytes) //Loop until all characters written
  {
    lblk = oftp->offset / BLKSIZE;
    start = oftp->offset % BLKSIZE;
    
    //convert logic to phys
    //direct blocks
    if(lblk < 12 ) 
    {
      //printf("direct\n");
      if(mip->INODE.i_block[lblk] == 0)
      {
        mip->INODE.i_block[lblk] = balloc(mip->dev);
      }
      blk = mip->INODE.i_block[lblk];
    }
    //indirect blocks
    else if(lblk >= 12 && lblk < 256 + 12) 
    {
      //printf("indirect lblk = %d\n", lblk);
      lblk -= 12; //Starts counting from 1st indirect block
      //If Block 12 not exist, allocate block, fill with 0, write to disk
      if(mip->INODE.i_block[12] == 0)
      {
        mip->INODE.i_block[12] = balloc(mip->dev);
        memset(ibuf,0,BLKSIZE);
        put_block(mip->dev, mip->INODE.i_block[12], ibuf); //Writes block in mem to disk
      }
      //incase we did not have to allocate, get block from disk
      get_block(mip->dev, mip->INODE.i_block[12], ibuf);
      //Sets blk to index in retrived/allocated pointer block
      blk = ibuf[lblk];
      //If indirect block not exist, allocate block, fill with 0, write to disk 
      if (blk == 0)
      {
        blk = balloc(mip->dev);
        ibuf[lblk] = blk;
        put_block(mip->dev, mip->INODE.i_block[12], ibuf); //Writes block in mem to disk
      } 
    }
    //double indirect blocks
    else 
    {
      // printf("double indirect lblk = %d\n", lblk);
      //Makes lblk count from 1st double indirect block
      lblk -= (12 + 256);
      //dblk is index for indirect block
      dblk = lblk / 256;
      //offset is index for double indirect block
      offset = lblk % 256;
      //If block 13 not exist, allocate block, fill with 0, write to disk 
      if(mip->INODE.i_block[13] == 0)
      {
        // printf("allocating direct[%d]\n", 13);
        mip->INODE.i_block[13] = balloc(mip->dev);
        memset(ibuf,0,BLKSIZE);
        put_block(mip->dev, mip->INODE.i_block[13], ibuf);
      }
      // printf("dblk = %d offset = %d\n", dblk, offset);
      //incase we did not have to allocate, get block from disk
      get_block(mip->dev, mip->INODE.i_block[13], ibuf);
      iblk = ibuf[dblk];
      // printf("iblk = %d\n", iblk);
      //If indirect block not exist, allocate block, fill with 0, write to disk 
      if(iblk == 0)
      {
        // printf("allocating indirect[%d]\n", dblk);
        iblk = balloc(mip->dev);
        memset(ibuf,0,BLKSIZE);
        ibuf[lblk]= iblk;
        put_block(mip->dev, mip->INODE.i_block[13], ibuf);
      }
      //incase we did not have to allocate, get block from disk
      get_block(mip->dev, iblk, ibuf);
      blk = ibuf[offset];
      // printf("blk = %d\n", blk);
      //If double indirect block not exist, allocate block, fill with 0, write to disk 
      if(blk == 0)
      {
        // printf("allocating double indirect[%d]\n", offset);
        blk = balloc(mip->dev);
        ibuf[offset] = blk;
        put_block(mip->dev, iblk, ibuf);    
      }
      //printf("blf = %d", blk);
    }



    memset(writeBuf,0,BLKSIZE);
    //read to buf
    //get block from disk
    get_block(mip->dev, blk, writeBuf);
    cp = writeBuf + start;
    remain = BLKSIZE - start;
    //If remaining space in block is less than bytes, fill the space
    if(remain < nbytes)
    {
      strncpy(cp, cq, remain);
      count += remain;
      nbytes -= remain;
      running->fd[fd]->offset += remain;
      //check offset
      if(running->fd[fd]->offset > mip->INODE.i_size)
      {
        mip->INODE.i_size += remain;
      }
      remain -= remain;
    }
    //else write remaining bytes to space avalible
    else
    {
      strncpy(cp, cq, nbytes);
      count += nbytes;
      remain -= nbytes;
      running->fd[fd]->offset += nbytes;
      if(running->fd[fd]->offset > mip->INODE.i_size)
      {
        mip->INODE.i_size += nbytes;
      }
      nbytes -= nbytes;
    }
    put_block(mip->dev, blk, writeBuf);
    mip->dirty = 1;
    printf("Wrote %d chars into file.\n", count);
  }
}

//Wrapper for my_read to parse pathname and verify exists
int read_file(char *pathname)
{
  char secondPath[256], path[256];
  split_paths(pathname, path, secondPath);
  //convert bytes to int
  int nbytes = atoi(secondPath), actual = 0;
  int fd = 0;
  OFT *oftp;
  INODE *pip;
  MINODE *pmip;
  int i;
  char buf[nbytes + 1];
  MINODE *mip;
  INODE* ip;

  strcpy(buf, "");
  //check fd
  if (!strcmp(pathname, ""))
  {
    printf("No FD\n");
    return 0;
  }
  //convert fd to int
  fd = atoi(pathname);
  if (!strcmp(secondPath, ""))
  {
    printf("No byte\n");
    return 0;
  }

  //return byte read
  actual = my_read(fd, buf, nbytes);

  if (actual == -1)
  {
    strcpy(secondPath, "");
    return 0;
  }

  buf[actual] = '\0';
  printf("actual = %d buf = %s\n", actual, buf);
  return actual;
}

/* Using offset to find the block and place in block to start, 
read either nbytes from that block or all info from the block. Repeart until you read nbytes */
int my_read(int fd, char *buf, int nbytes)
{
  MINODE *mip; 
  OFT *oftp;
  int count = 0;
  int lbk, blk, startByte, remain, ino;
  int avil;
  int *ip;

  int indirect_blk;
  int indirect_off;

  int buf2[BLKSIZE];

  char readbuf[1024];
  char temp[1024];

  oftp = running->fd[fd];
  //printf("flags = %d\n", oftp->mode);
  if(!oftp) {
    printf("file for write\n");
    return -1;
  }

  mip = oftp->inodeptr;
  //calculate byte to read
  avil = mip->INODE.i_size - oftp->offset;
  char *cq = buf;

  while(nbytes && avil)
  {
    lbk = oftp->offset / BLKSIZE;
    startByte = oftp->offset % BLKSIZE;

    //direct block
    if(lbk < 12)
    {
      blk = mip->INODE.i_block[lbk];
      //printf("direct\n");
    }
    //indirect blocks
    else if(lbk >= 12 && lbk < 256 + 12)
    {
      get_block(mip->dev, mip->INODE.i_block[12], readbuf);

      ip = (int *)readbuf + lbk - 12;
      blk = *ip;
      //printf("indirect\n");

    }
    //double indirect
    else
    {
      get_block(mip->dev, mip->INODE.i_block[13], readbuf);

      indirect_blk = (lbk - 256 - 12) / 256;
      indirect_off = (lbk - 256 - 12) % 256;
      ip = (int *)readbuf + indirect_blk;
      get_block(mip->dev, *ip, readbuf);
      ip = (int *)readbuf + indirect_off;
      blk = *ip;
      //printf("double indirect\n");
    }

    //getblock to buf
    get_block(mip->dev, blk, readbuf);
    //printf("readbuf = %d and %s\nstartbyte = %d\n", readbuf, readbuf, startByte);
    char *cp = readbuf + startByte;

    remain = BLKSIZE - startByte;
    //read either all remaining info in block or all remaining bytes requested whichever is smaller
    int temp =remain ^ ((avil ^ remain) & -(avil < remain));
    int temp2 =nbytes ^ ((temp ^ nbytes) & -(temp < nbytes));
    //printf("minimum bytes = %d\n", temp2);
    //check available and remaining
    while(remain > 0)
    {
      // printf("avail = %d, remain = %d\n", avil, remain);
      // printf("temp2 = %d\n", temp2);
      strncpy(cq, cp, temp2);
      //*cq++ = *cp++;
      oftp->offset += temp2;
      count += temp2;
      avil -= temp2;
      nbytes -= temp2;
      remain -= temp2;
      // printf("avail = %d, remain = %d\n", avil, remain);
      if(nbytes <= 0 || avil <= 0)
        break;
    }
  }
  //printf("myread: read %d char from file descriptor %d\n", count, fd);
  return count;
}

//Parses pathname into fd and position and verifies fd exists
int my_lseek(char * pathname)
{
  char secondPath[256], path[256];
  split_paths(pathname, path, secondPath);
  //convert bytes to int
  int nbytes = atoi(secondPath);
  int fd = 0;

  //check fd
  if (!strcmp(pathname, ""))
  {
    printf("No FD\n");
    return 0;
  }
  //convert fd to int
  fd = atoi(pathname);
  if (!strcmp(secondPath, ""))
  {
    printf("No byte\n");
    return 0;
  }
  return file_lseek(fd, nbytes);
}

//Sets offset to given value if in bounds
int file_lseek(int fd, int position)
{
  OFT *oftp;
  oftp = running->fd[fd];
  int max = oftp->inodeptr->INODE.i_size - 1;
  int min = 0;

  if(position > max || position < min)
  {
    printf("Out of bounds.\n");
    return -1;
  }
  int originalPosition = oftp->offset;
  oftp->offset = position;
  return originalPosition;
}

//Open file, read entire file in buf, print buf, close file
int my_cat(char *pathname)
{
  char mybuf[BLKSIZE], dummy = 0;
  int n;
  char temppath[BLKSIZE];
  strcpy(temppath, pathname);
  strcat(temppath," R");
  int fd = open_file(temppath);
  while( n = my_read(fd,mybuf,BLKSIZE))
  {
    mybuf[n]= 0;
    printf("%s",mybuf);
  }
  my_close(fd);
}

//Parse into source and dest
int my_copy(char *pathname)
{
  char destination[256], source[256];
  split_paths(pathname, source, destination);
  return copy_file(source, destination);
  
}

//Open source for read, open dest for write, 
//read source into uf, write buf into dest, close source and dest
int copy_file(char *source, char *target)
{
  char temp[BLKSIZE];
  strcpy(temp, target);
  strcat(source, " R");
  strcat(target, " W");
  int fd = open_file(source);
  int gd = open_file(target);
  char buf[BLKSIZE];
  int n;

  while(n = my_read(fd, buf, BLKSIZE))
  {
    my_write(gd, buf, n);
  }

  my_close(fd);
  my_close(gd);
}

//Prints each file in running->fd for testing
int pfd(char *pathname)
{
  int i = 0;
  char mode[256];
  printf(" fd     mode    offset     INODE \n");
  printf("----    ----    ------    -------\n");
  while(i < 10)
  {
    if(running->fd[i] != NULL )
    {
      if(running->fd[i]->mode == 0)
      {
        strcpy(mode, "R ");
      }
      else if(running->fd[i]->mode == 1)
      {
        strcpy(mode, "W ");
      }
      else if(running->fd[i]->mode == 2)
      {
        strcpy(mode, "RW");
      }
      else if(running->fd[i]->mode == 3)
      {
        strcpy(mode, "A ");
      }
      printf("   %d      %s      %d       [%d,%d]\n", i, mode, running->fd[i]->offset,
      running->fd[i]->inodeptr->dev, running->fd[i]->inodeptr->ino);
    }
    i++;
  }
  printf("--------------------------------------\n");
}

//Parse pathname into dest and source
int my_move(char*pathname)
{
  char destination[256], source[256];
  split_paths(pathname, source, destination);
  return move_file(source, destination);
}

//Checks if source and dest are in the same dev, if yes hard links, if no copies and deletes source
int move_file(char*source,char*target)
{
  char parent[256], child[256], parentT[256];
  memset(parent, 0, 256);
  memset(child, 0, 256);
  memset(parentT, 0, 256);
  dname(source, parent);
  dname(target, parentT);
  if(strcmp(parent,parentT)) //incorrect compare INODE->dev not paths
  {
    strcpy(child, source);
    strcat(source, " ");
    strcat(source, target);
    my_copy(source);
    my_unlink(child);
  }
  else
  {
    strcpy(child, source);
    strcat(source, " ");
    strcat(source, target);
    my_link(source);
    my_unlink(child);
  }
}

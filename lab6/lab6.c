#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#define BLKSIZE 1024

typedef struct ext2_group_desc GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD *gp;
INODE *ip;
DIR *dp;
char buf[BLKSIZE];
char ibuf[BLKSIZE];
int iblock;
int dev;


// get_block() reads a disk BLOCK into a char buf[BLKSIZE].

int get_block(int dev, int blk, char *buf)
{   
    lseek(dev, blk*BLKSIZE, SEEK_SET);
    return read(dev, buf, BLKSIZE);
}

void show_dir(INODE *ip)
{
    char sbuf[BLKSIZE], temp[256];
    DIR *dp;
    char *cp;
    int i;

    for (i=0; i < 12; i++){  // assume DIR at most 12 direct blocks
        if (ip->i_block[i] == 0)
        break;
        printf("root inode data block = %d\n", ip->i_block[i]);
        printf(" i_number rec_len name_len  name\n");
        get_block(dev, ip->i_block[i], sbuf);

        dp = (DIR *)sbuf;
        cp = sbuf;

        while(cp < sbuf + BLKSIZE){
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            printf("%7d %6d %7d      %s\n", 
             dp->inode, dp->rec_len, dp->name_len, temp);

            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
}

int search(INODE *ip, char* name)
{
    char sbuf[BLKSIZE], temp[256];
    DIR *dp;
    char *cp;
    int i;

    for (i=0; i < 12; i++){  // assume DIR at most 12 direct blocks
        if (ip->i_block[i] == 0)
        break;
        printf("search for %s\n", name);
        printf("i=%d  i_block[0]=%d\n", i, ip->i_block[i]);
        printf(" i_number rec_len name_len  name\n");
        get_block(dev, ip->i_block[i], sbuf);

        dp = (DIR *)sbuf;
        cp = sbuf;

        while(cp < sbuf + BLKSIZE){
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            printf("%7d %6d %7d      %s\n", 
             dp->inode, dp->rec_len, dp->name_len, temp);

            if(strcmp(name, temp) == 0)
            {
                printf("found %s : ino = %d\n", name, dp->inode);
                return dp->inode;
            }
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    char path[2048];
    char name[1024][1024];    
    char *device;
    if(argc > 2)
	{
		device = argv[1];
		strcpy(path, argv[2]);
	}
	else
	{
		printf("Usage : show vdisk pathname\n");
		exit(0);
	}

    // open the vdisk
    dev = open(device, O_RDONLY);

    // check if ext2
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;

    // check EXT2 FS magic number:
    printf("(1). Verify it's an ext2 file system:");
    //printf("%-30s = %8x ", "s_magic", sp->s_magic);
    if (sp->s_magic != 0xEF53)
    {
        printf("NOT an EXT2 FS\n");
        exit(2);
    }
    else printf("OK\n");

    printf("(2). Read group descriptor 0 to get bmap imap inodes_start\n");
    get_block(dev, 2, buf); // get group descriptor
    gp = (GD *)buf;
    printf("bmap_block=%d imap_block=%d inodes_table=%d \n",
        gp->bg_block_bitmap,
        gp->bg_inode_bitmap,
        gp->bg_inode_table);
    iblock = gp->bg_inode_table;

    printf("*********** get root inode *************\n");
    printf("(3). Show root DIR contents\n");
    // get inode start block
    get_block(dev, iblock, ibuf);
    ip = (INODE *)ibuf + 1;
    show_dir(ip);    
    printf("hit a key to continue:");
    getchar();
    printf("tokenize %s\n", path);
    
    int j = 0;
    char* temp;
    strcat(path, "/");
	temp = strtok(path, "/");
	while(temp != NULL)
	{
		strcpy(name[j], temp);
		temp = strtok(NULL, "/");
        j++;
    }

    // print the vars
    for(int i = 0; i < j; i++)
    {
        printf("%s  ", name[i]);
    }
    printf("\n");
    getchar();

    // find the file
    int ino, blk, offset;
    //INODE *ip->root inode;

    for (int i=0; i < j; i++){
        ino = search(ip, name[i]);

        if (ino==0){
            printf("can't find %s\n", name[i]); 
            exit(1);
        }

        // Mailman's algorithm: Convert (dev, ino) to INODE pointer
        blk    = (ino - 1) / 8 + gp->bg_inode_table; 
        offset = (ino - 1) % 8;        
        get_block(dev, blk, ibuf);
        ip = (INODE *)ibuf + offset;   // ip -> new INODE
    }
    
    // print direct blocks
    printf("ino = %d\n", ino);
    printf("size = %d\n", ip->i_size);
    int bnum = 0, blocknum, cyclenum;
    do
    {
        printf("i_block[%d] = %d\n", bnum, ip->i_block[bnum]);
        bnum++;
    } while (ip->i_block[bnum-1] != 0);
    
    // print indirect
    int double_indirect[256];
    INODE *indirect;
    blocknum = ip->i_size / BLKSIZE;
    cyclenum = blocknum;
    printf("blocknum = %d\n", blocknum);
    if(cyclenum > 12) cyclenum = 12;
    
    if(blocknum - cyclenum > 0)
    {
        printf("----------- INDIRECT BLOCKS ---------------\n");
        get_block(dev, ip->i_block[cyclenum], buf);
        cyclenum = blocknum - cyclenum;
        if(cyclenum > 256) cyclenum = 256;
        indirect = (INODE *)buf;
        for(int i = 0; i < cyclenum; i++)
        {
            printf("%d ", indirect->i_block[i-10]);
        }
        printf("\n");
        if(blocknum - cyclenum > 0)
        {
            printf("----------- DOUBLE INDIRECT BLOCKS ---------------\n");
            get_block(dev, ip->i_block[13], double_indirect);
            int numblocks = blocknum - cyclenum;
			for (int k = 0; k < 256; k++)
			{
				if (double_indirect[k] == 0)
				{
					break;
				}
				
                get_block(dev, double_indirect[k], buf);
                cyclenum = numblocks;
                if(cyclenum > 256) cyclenum = 256;
                indirect = (INODE *)buf;
                for(int i = 0; i < cyclenum; i++)
                {
                    if(indirect->i_block[i-10] != 0) printf("%d ", indirect->i_block[i-10]);
                }
                printf("\n");
            }
        }
    }
}

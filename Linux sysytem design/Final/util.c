/*********** util.c file ****************/
#include "type.h"

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}   

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}   

char** tokenize(char *pathname)
{
   char* tmp;
   char** name = (char**)malloc(sizeof(char*)*256);
   name[0] = strtok(pathname, "/");
   int i = 1;
   while ((name[i] = strtok(NULL, "/")) != NULL) { i++;}
   name[i] = 0;
   i = 0;
   while(name[i])
   {
      tmp = (char*)malloc(sizeof(char)*strlen(name[i]));
      strcpy(tmp, name[i]);
      name[i] = tmp;
      i++;
   }
   return name;
}

MINODE *iget(int dev, unsigned int ino)
{
   int i;
   char buf[BLKSIZE];
   MINODE *mip = NULL;
   for(i = 0; i < NMINODE; i++)
   {
      if(minode[i].refCount && minode[i].ino == ino && minode[i].dev == dev)
      {
         mip = &minode[i];
         minode[i].refCount++;
         return mip;
      }
   }
   i = 0;
   while(minode[i].refCount > 0 && i < NMINODE) { i++;}
   if(i == 64)
   {
      return 0;
   }
   int blk = (ino-1)/8 + iblk;
   int offset = (ino-1)%8;
   get_block(dev, blk, buf);
   ip = (INODE *)buf + offset;
   memcpy(&(minode[i].INODE), ip, sizeof(INODE)); 
   minode[i].dev = dev;
   minode[i].ino = ino;
   minode[i].refCount = 1;
   minode[i].dirty = 0;
   minode[i].mounted = 0;
   minode[i].mptr = NULL;
   return &minode[i];
}

void iput(int dev, MINODE *mip)
{
   char buf[BLKSIZE];
   INODE *ip;

   if (mip==0) 
      return;

   mip->refCount--;
 
   if (mip->refCount > 0) 
      return;
   if (!mip->dirty)       
      return;
 
   int blk = (mip->ino-1)/8 + iblk;
   int offset = (mip->ino-1)%8;

   get_block(dev, blk, buf); 

   ip = (INODE*)buf + offset;
   memcpy(ip, &(mip->INODE), sizeof(INODE)); 
   put_block(mip->dev, blk, buf);
   mip->refCount = 0;
   return 1;
} 

int search(int dev, char *str, INODE *ip)
{
   char *cp;
   DIR *dp;
   char buf[BLKSIZE], temp[256];

   for(int i = 0; i < 12; i++)
   {
      if(ip->i_block[i] == 0){break;} 
      get_block(dev, ip->i_block[i], buf);
      dp = (DIR *)buf;
      cp = buf;

      while(cp < buf+BLKSIZE)
      {
         memset(temp, 0, 256);
         strncpy(temp, dp->name, dp->name_len);
         if(strcmp(str, temp) == 0){ return dp->inode;}
         cp += dp->rec_len;
         dp = (DIR*)cp;
      }
   }
   return 0;
}

int getino(int dev, char *path)
{
   int ino = 0, i = 0;
   char **tokens;
   MINODE *mip = NULL;
   if(path && path[0])
   {
      tokens = tokenize(path);
   }
   else
   {
      ino = running->cwd->ino;
      return ino;
   }
   if(path[0]=='/')
   {
      ip = &(root->INODE);
      ino = root->ino;
   }
   else 
   {
      ip = &(running->cwd->INODE);
   }
   while(tokens[i])
   {
      ino = search(dev, tokens[i], ip);
      if(0 >= ino) 
      {
         if(mip){ iput(mip->dev, mip);}
         return -1;
      }
      if(mip) { iput(mip->dev, mip);}
      i++;
      if(tokens[i])
      {
         mip = iget(dev, ino);
         ip = &(mip->INODE);
      }
   }
   i = 0;
   while(tokens[i])
   {
      free(tokens[i]);
      i++;
   }
   if(mip) { iput(mip->dev, mip);}
   return ino;
}

// These 2 functions are needed for pwd()
int findmyname(char *pathname)
{
   int i = 0;
   while(i < strlen(pathname))
   {
      if(pathname[i] == '/')
         return 1;
      i++;
   }
   return 0;
}

int findino(int dev, int ino, INODE *ip, char* temp)
{
   char *cp;
   DIR *dp;
   char buf[BLKSIZE];
   for(int i = 0; i < 12; i++)
   {
      if(ip->i_block[i] == 0){ break;}
      get_block(dev, ip->i_block[i], buf);
      dp = (DIR *)buf;
      cp = buf;
      while(cp < buf+BLKSIZE)
      {
         if(ino == dp->inode)
         {
            strncpy(temp, dp->name, dp->name_len);
            return 1;
         }
         cp += dp->rec_len;
         dp = (DIR*)cp;
      }
   }
   return 0;
}

int dname(char *pathname, char buf[256])
{
   int i = 0;
   memset(buf, 0, 256);
   strcpy(buf, pathname);
   while(buf[i]) { i++; }//start
   while(i >= 0)
   {
      if(buf[i] == '/')   //split
      {
         buf[i+1] = 0; 
         return 1;
      }
      i--;
   }
   buf[0] = 0;
   return 1;
}

int bname(char *pathname, char *buf)
{
   int i = 0, j = 0;
   if(!pathname[0]) {return -1;}
   i = strlen(pathname);
   while(i >= 0 && pathname[i] != '/') { i--; }//start
   if(pathname[i] == '/')
   {
      i++;
      while(pathname[i])
      {
         buf[j++] = pathname[i++];//copy to buf
      }
      buf[j] = 0;
      return 1;
   }
   //if already basename
   else { strncpy(buf, pathname, 256);}//if already basename
   return 1;
}

//iblk to start
void get_inode_table(int dev)
{
   char buf[BLKSIZE];
   get_block(dev, 2, buf);
   gp = (GD*)buf;
   iblk = gp->bg_inode_table;
   bmap = gp->bg_block_bitmap;
   imap = gp->bg_inode_bitmap;
}

int tst_bit(char *buf, int bit) 
{ 
   // in Chapter 11.3.1
   int byt = bit/8;
   int offset = bit % 8;
   return (((*(buf+byt))>>offset)&1);
}

int set_bit(char *buf, int bit) 
{ 
   // in Chapter 11.3.1
   int byt = bit/8;
   int offset = bit % 8;
   char *tempBuf = (buf+byt);
   char temp = *tempBuf;
   temp |= (1<<offset);
   *tempBuf = temp;
   return 1;
}

int clr_bit(char* buf, int bit)
{
   int byt = bit/8;
   int offset = bit % 8;
   char *tempBuf = (buf+byt);
   char temp = *tempBuf;
   temp &= (~(1<<offset));
   *tempBuf = temp;
   return 1;
}

int decFreeInodes(int dev)
{
   //decrements the number of free inodes count in SUPER and GD 
   char buf[BLKSIZE];
   get_block(dev, 1, buf);
   sp = (SUPER*)buf;
   sp->s_free_inodes_count --; 
   put_block(dev, 1, buf);
   get_block(dev, 2, buf);
   gp = (GD*)buf;
   gp->bg_free_inodes_count --;
   put_block(dev, 2, buf);
   return 1;
}

int incFreeInodes(int dev)
{
   //increment the number of free inodes by doing same thing as dec but +
   char buf[BLKSIZE];
   get_block(dev, 1, buf);
   sp = (SUPER*)buf;
   sp->s_free_inodes_count += 1;
   put_block(dev, 1, buf);
   get_block(dev, 2, buf);
   gp = (GD*)buf;
   gp->bg_free_inodes_count +=1;
   put_block(dev, 2, buf);
   return 1;
}
//same as ino but block
int decFreeBlocks(int dev)
{
   char buf[BLKSIZE];
   get_block(dev, 1, buf);
   sp = (SUPER*)buf;
   sp->s_free_blocks_count -= 1;
   put_block(dev, 1, buf);
   get_block(dev, 2, buf);
   gp = (GD*)buf;
   gp->bg_free_blocks_count -=1;
   put_block(dev, 2, buf);
   return 1;
}

int incFreeBlocks(int dev)
{
   char buf[BLKSIZE];
   get_block(dev, 1, buf);
   sp = (SUPER*)buf;
   sp->s_free_blocks_count += 1;
   put_block(dev, 1, buf);
   get_block(dev, 2, buf);
   gp = (GD*)buf;
   gp->bg_free_blocks_count +=1;
   put_block(dev, 2, buf);
   return 1;
}

int ialloc(int dev)
{
   char buf[BLKSIZE];
   get_block(dev, imap, buf); 
   
   for (int i=0; i < ninodes; i++){
      if (tst_bit(buf, i)==0){//check used
      set_bit(buf, i);
      put_block(dev, imap, buf);
      printf("allocated ino = %d\n", i+1);
      // update free inode count in SUper and GD
      decFreeInodes(dev);
      return (i+1);             
      }
   }
   return 0; // out of FReE inode
}

int balloc(int dev)
{
   char buf[BLKSIZE];
   get_block(dev, bmap, buf);

   for (int i=0; i < BLKSIZE; i++){
      if (tst_bit(buf, i)==0){
         set_bit(buf, i);
         put_block(dev, bmap, buf);
         decFreeBlocks(dev);
         memset(buf, 0, BLKSIZE);//buf to 0
         put_block(dev, i+1, buf);
         return (i+1);
      }
   }
  return 0;
}

int idalloc(int dev, int ino)
{
   int i;
   char buf[BLKSIZE];
   if (ino > ninodes){  
      printf("inumber %d out of range\n", ino);
      return;
   }
   get_block(dev, imap, buf);//get i to buf
   clr_bit(buf, ino-1);//clr
   put_block(dev, imap, buf);
   incFreeInodes(dev);
}

int bdalloc(int dev, int ino)
{
   int i;
   char buf[BLKSIZE];

   get_block(dev, bmap, buf);  
   clr_bit(buf, ino-1);
   put_block(dev, bmap, buf);
   incFreeBlocks(dev);
}

int findLastBlock(MINODE *pip)
{
   int buf[256];
   int buf2[256];
   int bnumber, i, j;

   if(pip->INODE.i_block[0] == 0) {return 0;}
   //direct
   for(i = 0; i < 12; i++)
   {
      if(pip->INODE.i_block[i] == 0)
      {
         return (pip->INODE.i_block[i-1]);
      }
   }
   if(pip->INODE.i_block[12] == 0) {return pip->INODE.i_block[i-1];}
   //check indirect
   get_block(dev, pip->INODE.i_block[12], (char*)buf);
   for(i = 0; i < 256; i++)
   {
      //look for the free blocks
      if(buf[i] == 0) {return buf[i-1];}
   }
   //check double indirect
   if(pip->INODE.i_block[13] == 0) {return buf[i-1];}
   memset(buf, 0, 256);
   get_block(pip->dev, pip->INODE.i_block[13], (char*)buf);
   for(i = 0; i < 256; i++)
   {
      if(buf[i] == 0) {return buf2[j-1];}
      if(buf[i])
      {
         get_block(pip->dev, buf[i], (char*)buf2);
         for(j = 0; j < 256; j++)
         {
            if(buf2[j] == 0) {return buf2[j-1];}
         }
      }
   }
}

//addlast of minode
int addLastBlock(MINODE *pip, int bnumber)
{
   int buf[256];
   int buf2[256];
   int i, j, newBlk, newBlk2;
   for(i = 0; i < 12; i++) //direct
   {
      if(pip->INODE.i_block[i] == 0) {pip->INODE.i_block[i] = bnumber; return 1;}
   }
   //indirect
   if(pip->INODE.i_block[12] == 0)
   {
      newBlk = balloc(pip->dev);
      pip->INODE.i_block[12] = newBlk;
      memset(buf, 0, 256);
      get_block(pip->dev, newBlk, (char*)buf);
      buf[0] = bnumber;
      put_block(pip->dev, newBlk, (char*)buf);
      return 1;
   }
   memset(buf, 0, 256);
   get_block(pip->dev, pip->INODE.i_block[12], (char*)buf);
   for(i = 0; i < 256; i++)
   {
      if(buf[i] == 0) {buf[i] = bnumber; return 1;}
   }
   //double indirect
   if(pip->INODE.i_block[13] == 0) 
   {
      newBlk = balloc(pip->dev);
      pip->INODE.i_block[13] = newBlk;
      memset(buf, 0, 256);
      get_block(pip->dev, newBlk, (char*)buf);
      newBlk2 = balloc(pip->dev);
      buf[0] = newBlk2;
      put_block(pip->dev, newBlk, (char*)buf);
      memset(buf2, 0, 256);
      get_block(pip->dev, newBlk2, (char*)buf2);
      buf2[0] = bnumber;
      put_block(pip->dev, newBlk2, (char*)buf2);
      return 1;
   }
   memset(buf, 0, 256);
   get_block(pip->dev, pip->INODE.i_block[13], (char*)buf);
   for(i = 0; i < 256; i++)
   {
      if(buf[i] == 0)
      {
         newBlk2 = balloc(pip->dev);
         buf[i] = newBlk2;
         put_block(pip->dev, pip->INODE.i_block[13], (char*)buf);
         memset(buf2, 0, 256);
         get_block(pip->dev, newBlk2, (char*)buf2);
         buf2[0] = bnumber;
         put_block(pip->dev, newBlk2, (char*)buf2);
         return 1;
      }
      memset(buf2, 0, 256);
      get_block(pip->dev, buf[i], (char*)buf2);
      for(j = 0; j < 256; j++)
      {
         if(buf2[j] == 0) {buf2[j] = bnumber; return 1;}
      }
   }
   printf("Failed block to node\n");
   return -1;
}

int is_empty(MINODE *mip)
{
   DIR *dp;
   char *cp, buf[BLKSIZE], temp[256];
   for(int i = 0; i < 12; i++)
   {
      if(ip->i_block[i] == 0) { return 0; }
      get_block(dev, ip->i_block[i], buf);
      dp = (DIR *)buf;
      cp = buf;

      while(cp < buf+BLKSIZE)
      {
         memset(temp, 0, 256);
         strncpy(temp, dp->name, dp->name_len);
         if(strncmp(".", temp, 1) != 0 && strncmp("..", temp, 2) != 0) { return 1;}
         cp += dp->rec_len;
         dp = (DIR*)cp;
      }
   }  
}

int _balloc(int dev) {
    u32 total_blocks;
    u8 buf[BLKSIZE], sup_buf[BLKSIZE];
    SUPER * p_sup;
    int dev_bmap;
    MTABLE * p_mtable;

    if (dev == root->dev) {
        total_blocks = sp->s_blocks_count;
        dev_bmap = bmap;
    } else {
        int i;
        for (i = 0; i < NMTABLE; i++) {
            p_mtable = &mtable[i];
            if (p_mtable->dev == dev) {
                break;
            }
        }
        if (i == NMTABLE) {
            return 0;
        }
        get_block(dev, 1, sup_buf);
        p_sup = (SUPER *) sup_buf;
        total_blocks = p_sup->s_blocks_count;
        dev_bmap = p_mtable->bmap;
    }
    get_block(dev, dev_bmap, buf);
    for (int i=0; i < total_blocks; i++) {
        if (tst_bit(buf, i) == 0) {
            set_bit(buf, i);
            put_block(dev, dev_bmap, buf);
            get_block(dev, i + 1, buf);
            memset(buf, 0, BLKSIZE);
            put_block(dev, i + 1, buf);
            if (dev == root->dev) {
                sp->s_free_blocks_count--;
                gp->bg_free_blocks_count--;
            } else {
                p_sup->s_free_blocks_count--;
                put_block(dev, 1, sup_buf);
                get_block(dev, 2, buf);
                GD * p_gd = (GD *) buf;
                p_gd->bg_free_blocks_count--;
                put_block(dev, 2, buf);
            }
            return i+1;
        }
    }
    return 0;
}
/************* mkdir_creat.c file **************/
#include "type.h"

int _mkdir(char *pathname)
{
  int dev, r;
  char parent[256], child[256], origPathname[512];
  memset(parent, 0, 256);
  memset(child, 0, 256);
  memset(origPathname, 0, 512);
  strcpy(origPathname, pathname);
  //check root or dir
  if(!strcmp(pathname,"")){
    printf("Missing operand\n"); 
    return -1;
  }
  if(pathname[0] == '/') { dev = root->dev; }
  else { dev = running->cwd->dev; }
  //divide pathname into dirname and basename
  dname(pathname, parent);
  bname(origPathname, child);
  int pino = getino(dev, parent);
  if(pino <= 0)
  {
    printf("Invalid.\n");
    return -1;
  }
  MINODE *pmip = iget(dev, pino);

  //check if dir
  if(!S_ISDIR(pmip->INODE.i_mode))
  {
    printf("Not a directory.\n");
    iput(dev, pmip);
    return -1;
  }
  //basename must not exist in parent DIR
  pino = search(dev, child, &(pmip->INODE));
  if(pino > 0)
  {
    printf("Directory exists.\n");
    iput(pmip->dev, pmip);
    return -1;
  }
  r = kmkdir(pmip, child);
  //put back
  iput(pmip->dev, pmip);
  return r;
}

int _creat(char* pathname)
{
  int dev1, ino, r;
  char parent[256], child[256];
  memset(parent, 0, 256);
  memset(child, 0, 256);
  if(!strcmp(pathname,"")){
    printf("Missing operand\n"); 
    return -1;
  }
  //start at root
  if(pathname[0] == '/') { dev1 = root->dev;}
  //else cwd
  else { dev1 = running->cwd->dev; } 

  dname(pathname, parent);
  bname(pathname, child);

  ino = getino(dev1, parent);
  if(ino <= 0)
  {
    printf("Invalid.\n");
    return -1;
  }
  MINODE *mip = iget(dev1, ino);
  if(!S_ISDIR(mip->INODE.i_mode))
  {
    printf("Not a directory.\n");
    iput(dev1, mip);
    return -1;
  }
  ino = search(dev1, child, &(mip->INODE));
  if(ino > 0)
  {
    printf("File exists.\n");
    iput(mip->dev, mip);
    return -1;
  }
  r = creat_file(mip, child);
  iput(mip->dev, mip);
  return r;
}

int kmkdir(MINODE *pip, char child[256])
{
  int inumber, bnumber, idealLen, needLen, newRec, i, j;
  MINODE *mip;
  char *cp;
  DIR *dpPrev;
  char buf[BLKSIZE];
  char buf2[BLKSIZE];
  int blk[256];
  //allocate an INOODE and a disk block
  inumber = ialloc(pip->dev);
  bnumber = balloc(pip->dev);

  //load inode to mem
  mip = iget(pip->dev, inumber);

  //initiialize mip->INODE as DIR INODE
  mip->INODE.i_mode = 0x41ED; //directory mode
  mip->INODE.i_uid = running->uid;
  mip->INODE.i_gid = running->gid;
  mip->INODE.i_size = BLKSIZE;
  mip->INODE.i_links_count = 2;
  mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L);
  mip->INODE.i_blocks = 2;
  mip->dirty = 1;
  for(i = 0; i <15; i++) { mip->INODE.i_block[i] = 0; }
  mip->INODE.i_block[0] = bnumber;
  //wirte INODE back to disk
  iput(mip->dev, mip);

  //make data block 0 of INODE to contain . and .. entries
  dp = (DIR*)buf;
  dp->inode = inumber;
  strncpy(dp->name, ".", 1);
  dp->name_len = 1;
  dp->rec_len = 12;

  cp = buf + 12;
  dp = (DIR*)cp;
  dp->inode = pip->ino;
  dp->name_len = 2;
  strncpy(dp->name, "..", 2);
  dp->rec_len = BLKSIZE - 12;

  //put block to mem
  put_block(pip->dev, bnumber, buf);
  memset(buf, 0, BLKSIZE);
  needLen = 4*((8+strlen(child)+3)/4);
  //find available block
  bnumber = findLastBlock(pip);
  //get block buffer
  get_block(pip->dev, bnumber, buf);
  //get parent's data block into a buffer
  cp = buf;
  dp = (DIR*)cp;
  //traverse to the last item in the block
  while((dp->rec_len + cp) < buf+BLKSIZE)
  {
    cp += dp->rec_len;
    dp = (DIR*)cp;
  }
  //calc ideal length
  idealLen = 4*((8+dp->name_len+3)/4);

  //check if enough room
  if(dp->rec_len - idealLen >= needLen)
  {
    //update
    newRec = dp->rec_len - idealLen;
    dp->rec_len = idealLen;
    cp += dp->rec_len;
    dp = (DIR*)cp;
    dp->inode = inumber;
    dp->name_len = strlen(child);
    strncpy(dp->name, child, dp->name_len);
    dp->rec_len = newRec;
  }
  else //if no room
  {
    //allocate new block and add data
    bnumber = balloc(pip->dev);
    dp = (DIR*)buf;
    dp->inode = inumber;
    dp->name_len = strlen(child);
    strncpy(dp->name, child, dp->name_len);
    dp->rec_len = BLKSIZE;
    addLastBlock(pip, bnumber);
  }
  //put block to disk
  put_block(pip->dev, bnumber, buf);
  //enter the new entry as the LAST entry
  pip->dirty = 1;
  pip->INODE.i_links_count++;
  memset(buf, 0, BLKSIZE);
  //check its parent
  findino(pip->dev, pip->ino, &running->cwd->INODE, buf);
  //update time using touch
  _touch(buf);
  return 1;
}

int _touch (char* name)
{
  char buf[1024];
  int ino;
  MINODE *mip;

  ino = getino(dev, name);
  if(ino <= 0)//create file if input unavailable
  {
    _creat(name);
    return 1;
  }
  mip = iget(dev, ino);
  //update time
  mip->INODE.i_atime = mip->INODE.i_mtime = mip->INODE.i_ctime =(unsigned)time(NULL);
  mip->dirty = 1;
  iput(mip->dev, mip);
  return 1;
}

int creat_file(MINODE *pip, char child[256])
{
  int inumber, bnumber, idealLen, needLen, newRec, i, j, blk[256];
  DIR *dpPrev;
  char *cp, buf[BLKSIZE], buf2[BLKSIZE];

  //allocate ino
  inumber = ialloc(pip->dev);
  MINODE *mip = iget(pip->dev, inumber);

  //Write contents, initialize
  mip->INODE.i_mode = 0x81A4;
  mip->INODE.i_uid = running->uid;
  mip->INODE.i_gid = running->gid;
  mip->INODE.i_size = 0;
  mip->INODE.i_links_count = 1;
  mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L);
  mip->INODE.i_blocks = 0;
  mip->dirty = 1;
  for(i = 0; i <15; i++) { mip->INODE.i_block[i] = 0;}
  iput(mip->dev, mip);
  //calc need length
  memset(buf, 0, BLKSIZE);
  needLen = 4*((8+strlen(child)+3)/4);
  bnumber = findLastBlock(pip);

  //check room
  get_block(pip->dev, bnumber, buf);
  dp = (DIR*)buf;
  cp = buf;
  while((dp->rec_len + cp) < buf+BLKSIZE)
  {
    cp += dp->rec_len;
    dp = (DIR*)cp;
  }
  idealLen = 4*((8+dp->name_len+3)/4);
  if(dp->rec_len - idealLen >= needLen)
  {
    newRec = dp->rec_len - idealLen;
    dp->rec_len = idealLen;
    cp += dp->rec_len;
    dp = (DIR*)cp;
    dp->inode = inumber;
    dp->name_len = strlen(child);
    strncpy(dp->name, child, dp->name_len);
    dp->rec_len = newRec;
  }
  else //else allocate block
  {
    bnumber = balloc(pip->dev);
    dp = (DIR*)buf;
    dp->inode = inumber;
    dp->name_len = strlen(child);
    strncpy(dp->name, child, dp->name_len);
    dp->rec_len = BLKSIZE;
    addLastBlock(pip, bnumber);
  }
  //putback
  put_block(pip->dev, bnumber, buf);
  pip->dirty = 1;
  memset(buf, 0, BLKSIZE);
  findino(pip->dev, pip->ino, &running->cwd->INODE, buf);
  _touch(buf);
  return 1;
}


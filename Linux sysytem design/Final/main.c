/****************************************************************************
*                   KCW: mount root file system                             *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"

extern MINODE *iget();

MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;
OFT oft[NOFT];
MTABLE mtable[8];

char gpath[128]; // global for tokenized components
char *name[64];  // assume at most 64 components in pathname
int  n;          // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, iblk;
char line[128], cmd[32], pathname[128], parameter[32];

#include "util.c"
#include "cd_ls_pwd.c"
#include "mkdir_creat.c"
#include "rmdir.c"
#include "link_unlink.c"
#include "symlink.c"
#include "open_close_lseek.c"
#include "read_cat.c"
#include "write_cp.c"

int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;
  OFT    *p_oft;
  MTABLE *p_mtable;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i;
    p->uid = p->gid = 0;
    p->cwd = 0;
  }
  for (i=0; i<NOFT; i++) {
    p_oft = &oft[i];
    p_oft->mode = -1;
    p_oft->inodeptr = NULL;
    p_oft->offset = 0;
    p_oft->refCount = 0;
  }
  for (i=0; i<NMTABLE; i++) {
    p_mtable = &mtable[i];
    p_mtable->dev = 0;
    p_mtable->mntDirPtr = NULL;
    p_mtable->name[0] = '\0';
    p_mtable->ninodes = 0;
    p_mtable->nblocks = 0;
    p_mtable->bmap = 0;
    p_mtable->imap = 0;
    p_mtable->inode_start = 0;
  }
}


// load root INODE and set root pointer to it
int mount_root()
{  
  printf("mount_root()\n");
  root = iget(dev, 2);
}

char *disk = "mydisk";
int main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];

  if (argc > 1) {
    disk = argv[1];
  }
  
  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }

  dev = fd;    // global dev same as this fd   

  /********** read super block  ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53){
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }     
  printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  get_block(dev, 2, buf); 
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  iblk = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, iblk);

  init();  
  mount_root();
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);

  // WRTIE code here to create P1 as a USER process
  
  while(1){
    memset(pathname, 0, 128);
    memset(parameter, 0, 32);
    memset(cmd, 0, 32);
    printf("input command : [ ls | cd | pwd | quit | mkdir | creat | rmdir | link | unlink | symlink | open | close | lseek | read | cat | write | cp | mv ]\n");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0] == 0)
       continue;
    pathname[0] = 0;

    sscanf(line, "%s %s %s", cmd, pathname, parameter);
    printf("cmd=%s pathname=%s\n", cmd, pathname);

    if (strcmp(cmd, "ls")==0)
      ls(pathname);
    else if (strcmp(cmd, "cd")==0)
      cd(pathname);
    else if (strcmp(cmd, "pwd")==0)
      pwd(pathname);
    else if (strcmp(cmd, "quit")==0)
      quit();
    else if (strcmp(cmd, "mkdir")==0)
      _mkdir(pathname);
    else if (strcmp(cmd, "creat")==0)
      _creat(pathname);
    else if (strcmp(cmd, "rmdir")==0)
      _rmdir(pathname);
    else if (strcmp(cmd, "link")==0)
      _link(pathname, parameter);
    else if (strcmp(cmd, "unlink")==0)
      _unlink(pathname);
    else if (strcmp(cmd, "symlink")==0)
      _symlink(pathname, parameter);
    else if (strcmp(cmd, "open")==0)
      _open(pathname, parameter);
    else if (strcmp(cmd, "close")==0)
      _close(pathname);
    else if (strcmp(cmd, "lseek")==0)
      _lseek(pathname, parameter);
    else if (strcmp(cmd, "read")==0)
      _read(pathname, parameter);
    else if (strcmp(cmd, "cat")==0)
      _cat(pathname);
    else if (strcmp(cmd, "write")==0)
      _write(pathname);
    else if (strcmp(cmd, "cp")==0)
      _cp(pathname, parameter);
    else if (strcmp(cmd, "mv")==0)
      _mv(pathname, parameter);
  }
}

int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i < NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip->dev, mip);
  }
  exit(0);
}

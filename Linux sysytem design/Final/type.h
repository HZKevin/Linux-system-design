/*************** type.h file for LEVEL-1 ****************/

#ifndef __TYPE_H__
#define __TYPE_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;   

#define R 0
#define W 1
#define RW 2
#define APPEND 3

#define FREE        0
#define READY       1

#define BLKSIZE  1024
#define NMINODE   128
#define NPROC       2
#define NFD        10
#define NOFT (NFD * NPROC)
#define NMTABLE    16

typedef struct oft{
  int   mode;
  int   refCount;
  struct minode *inodeptr;
  int   offset;
} OFT;

typedef struct minode{
  INODE INODE;           // INODE structure on disk
  int dev, ino;          // (dev, ino) of INODE
  int refCount;          // in use count
  int dirty;             // 0 for clean, 1 for modified
  int mounted;           // for level-3
  struct mntable *mptr;  // for level-3
}MINODE;

typedef struct proc{
  struct proc *next;
  struct proc *parent;
  struct proc *child;
  struct proc *sibling;

  int pid;               // process ID  
  int ppid;        
  int status;       
  int uid;               // user ID
  int gid;
  MINODE *cwd;      // CWD directory pointer  
  OFT *fd[10];
}PROC;

typedef struct mtable {
  int dev;
  struct minode * mntDirPtr;
  char name[64];
  int ninodes;
  int nblocks;
  int imap;
  int bmap;
  int iblk;
  int inode_start;
  char   mount_name[64]; // mounted DIR pathname
} MTABLE;

#endif
/************* rmdir.c file **************/
#include "type.h"

int _rmdir(char *pathname)
{
  int ino, i;
  char parent[256], child[256], origPathname[512];
  MINODE *pip = NULL;
  MINODE *mip = NULL;
  strcpy(origPathname, pathname);
  //checkdirname
  if(!pathname || !pathname[0])
  {
    printf("Error: No directory given\n");
    return -1;
  }
  //get in-memory INODE of pathname
  else
    ino = getino(dev, pathname);
  if(0 >= ino)
  {
    printf("Invalid.\n");
    return -1;
  }
  //load mem
  mip = iget(dev, ino);
  //check if dir
  if(!S_ISDIR(mip->INODE.i_mode))
  {
    printf("Not a directory.\n");
    iput(mip->dev, mip);
    return -1;
  }
  //check if in dir
  if(mip->refCount > 1)
  {
    printf("In Directory.\n");
    return -1;
  }
  //check if not empty
  if(mip->INODE.i_links_count > 2)
  {
    printf("Directory not empty.(dir in dir)\n");
    iput(mip->dev, mip);
    return -1;
  }
  //Check data blocks if file exist
  if(is_empty(mip) != 0)
  {
    printf("Directory not empty.(file in dir)\n");
    iput(mip->dev, mip);
    return -1;
  }
  for(i = 0; i < 12; i++)
  {
    if(mip->INODE.i_block[i] != 0)
      bdalloc(mip->dev, mip->INODE.i_block[i]);
  }
  idalloc(mip->dev, mip->ino);

  dname(origPathname, parent);
  bname(origPathname, child);
  //get parent's ino and inode
  ino = getino(mip->dev, parent);
  pip = iget(mip->dev, ino);
  iput(mip->dev, mip);
  //remove name from parent dir
  rm_child(pip, child);
  pip->INODE.i_links_count--;
  _touch(parent);
  pip->dirty = 1;
  iput(pip->dev, pip);
  return 1;
}

int rm_child(MINODE *pip, char *child)
{
  int i, size, found = 0;
  char *cp, *cp2;
  DIR *dp, *dp2, *dpPrev;
  char buf[BLKSIZE], buf2[BLKSIZE], temp[256];

  memset(buf2, 0, BLKSIZE);
  //check direct
  for(i = 0; i < 12; i++)
  {
    //search parent INODE's data block for the entry of name
    if(pip->INODE.i_block[i] == 0) { return 0; }
    //load to mem
    get_block(pip->dev, pip->INODE.i_block[i], buf);
    dp = (DIR *)buf;
    dp2 = (DIR *)buf;
    dpPrev = (DIR *)buf;
    cp = buf;
    cp2 = buf;

    while(cp < buf+BLKSIZE && !found)
    {
      memset(temp, 0, 256);
      strncpy(temp, dp->name, dp->name_len);
      if(strcmp(child, temp) == 0)
      {
        //if first and only child in a data block
        if(cp == buf && dp->rec_len == BLKSIZE)
        {
          //deallocate the data block
          bdalloc(pip->dev, pip->INODE.i_block[i]);
          pip->INODE.i_block[i] = 0;
          pip->INODE.i_blocks--;
          found = 1;
        }
        //else delete child
        else
        {
          while((dp2->rec_len + cp2) < buf+BLKSIZE)
          {
            dpPrev = dp2;
            cp2 += dp2->rec_len;
            dp2 = (DIR*)cp2;
          }
          //last entry
          if(dp2 == dp)
          {
            //absorb its rec_len to the prev entry
            dpPrev->rec_len += dp->rec_len;
            found = 1;
          }
          //entry is first but not the only or in the middle
          else
          {
            size = ((buf + BLKSIZE) - (cp + dp->rec_len));
            //add deleted rec_len to the LAST entry
            dp2->rec_len += dp->rec_len;
            //memcopy
            memmove(cp, (cp + dp->rec_len), size);
            dpPrev = (DIR*)cp;
            memset(temp, 0, 256);
            strncpy(temp, dpPrev->name, dpPrev->name_len);
            found = 1;
          }
        }
      }
      cp += dp->rec_len;
      dp = (DIR*)cp;
    }
    if(found)
    {
      //putback to disk
      put_block(pip->dev, pip->INODE.i_block[i], buf);
      return 1;
    }
  }
  return -1;
}


/************* cd_ls_pwd.c file **************/

int cd(char* pathname)
{
  if(pathname[0] == 0){
    running->cwd = root;
    return;
  }
  else{
    int ino = getino(dev, pathname);
    MINODE *mip = iget(dev, ino);
    if(S_ISDIR(mip->INODE.i_mode)){
      iput(dev, running->cwd);
      running->cwd = mip;
      return;
    }
    else{
      printf("Error: not a directory\n");
      return;
    }
  }
  // READ Chapter 11.7.3 HOW TO chdir
}

int ls_file(MINODE *mip, char *name)
{
  INODE* pip = &mip->INODE;
  u16 mode = pip->i_mode;
  time_t val = pip->i_ctime;
  char *mtime = ctime(&val);
  mtime[strlen(mtime) - 1] = '\0';
  u16 type = mode & 0xF000;
  switch(type)
  {
    case 0x4000:
      printf("d");
      break;
    case 0x8000:
      printf("-");
      break;
    case 0xA000:
      printf("l");
      break;
    default:
      printf("-");
      break;
  }
  printf( (mode & S_IRUSR) ? "r" : "-");
  printf( (mode & S_IWUSR) ? "w" : "-");
  printf( (mode & S_IXUSR) ? "x" : "-");
  printf( (mode & S_IRGRP) ? "r" : "-");
  printf( (mode & S_IWGRP) ? "w" : "-");
  printf( (mode & S_IXGRP) ? "x" : "-");
  printf( (mode & S_IROTH) ? "r" : "-");
  printf( (mode & S_IWOTH) ? "w" : "-");
  printf( (mode & S_IXOTH) ? "x" : "-");
  printf("%4d%4d%4d  %s%8d    %s", pip->i_links_count, pip->i_gid, pip->i_uid, mtime, pip->i_size, name);
  if((mode & 0120000) == 0120000)
    printf(" => %s\n",(char *)(mip->INODE.i_block));
  else
    printf("\n");
  // READ Chapter 11.7.3 HOW TO ls
}

int ls_dir(int dev, MINODE *mip)
{
  char buf[BLKSIZE], temp[256], *cp;
  DIR *dp;
  int ino;
  MINODE *tp;
  for (int i = 0; i < 12; i++)
  {
    if (mip->INODE.i_block[i])
    {
      get_block(dev, mip->INODE.i_block[i], buf);
      cp = buf;
      dp = (DIR *)buf;

      while (cp < &buf[BLKSIZE])
      {
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;
        ino = dp->inode;
        tp = iget(dev, ino);

        ls_file(tp, temp);
        cp += dp->rec_len;
        dp = (DIR *)cp;
        iput(tp->dev, tp);
      }
    }
  }
}

int ls(char *pathname)
{
  int ino;
  MINODE *mip, *pip;
  int dev = running->cwd->dev;
  char *child;

  if(pathname[0] == 0)
  {
    mip = iget(dev, running->cwd->ino);
    ls_dir(dev, mip);
  }

  else
  {
    if(pathname[0] == '/')
    {
      dev = root->dev;
    }
    ino = getino(dev, pathname);
    if(ino == -1 || ino == 0)
      return 1;
    mip = iget(dev, ino);
    if(((mip->INODE.i_mode) & 0040000)!= 0040000)
    {
      if(findmyname(pathname))
        child = basename(pathname);
      else
      {
        child = (char *)malloc((strlen(pathname) + 1) * sizeof(char));
        strcpy(child, pathname);
      }
      ls_file(mip, child);
      iput(mip->dev, mip);
      return 0;
    }
    ls_dir(dev, mip);
  }
  iput(mip->dev, mip);
  return 0;
}

int rpwd(MINODE *wd)
{
  MINODE *next = NULL;
  char temp[256];
  if(wd == root) 
  {
    printf("/");
    return 1;
  }
  int ino = search(dev, "..", &(wd->INODE));
  if(ino <= 0){return -1;}

  next = iget(dev, ino); 

  if(!next){return -1;}
  rpwd(next); 
  memset(temp, 0, 256);
  findino(next->dev, wd->ino, &(next->INODE), temp);
  printf("%s/", temp);
  iput(next->dev, next); 
  return 1;
}

char *pwd(char *pathname)
{
  printf("cwd = ");
  rpwd(running->cwd);
  printf("\n");
}
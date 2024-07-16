/************* link_unlink.c file **************/
#include "type.h"

int _link(char* pathname, char* parameter)
{
    char parent[256], child[256], buf[BLKSIZE];
    int newRec;
    DIR *dp;
    //check if file exist
    int ino = getino(dev, pathname);
    if(ino <= 0)
    {
        printf("File DNE.\n");
        return -1;
    }
    MINODE *mip = iget(dev, ino);
    //check isreg
    if(!S_ISREG(mip->INODE.i_mode))
    {
        printf("Not regular\n");
        iput(mip->dev, mip);
        return -1;
    }
    dname(parameter, parent);
    bname(parameter, child);
    //check if second file exist
    int ino2 = getino(mip->dev, parent);
    if(ino2<=0)
    {
        printf("File DNE");
        iput(mip->dev, mip);
        return -1;
    }
    MINODE *mip2 = iget(mip->dev, ino2);
    //check parent is dir
    if(!S_ISDIR(mip2->INODE.i_mode))
    {
        printf("Not a directory\n");
        iput(mip->dev, mip);
        iput(mip2->dev, mip2);
        return -1;
    }
    //check if filename used
    ino2 = search(mip2->dev, child, &(mip2->INODE));
    if(ino2 > 0)
    {
        printf("File exists\n");
        iput(mip->dev, mip);
        iput(mip2->dev, mip2);
        return -1;
    }

    memset(buf, 0, BLKSIZE);
    // calculate need length
    int needLen = 4*((8+strlen(child)+3)/4);
    int bnumber = findLastBlock(mip2);
    //check if last has enough room
    get_block(mip2->dev, bnumber, buf);
    dp = (DIR*)buf;
    char *cp = buf;
    while((dp->rec_len + cp) < buf+BLKSIZE)
    {
        cp += dp->rec_len;
        dp = (DIR*)cp;
    }
    // calculate ideal length
    int idealLen = 4*((8+dp->name_len+3)/4);
    if(dp->rec_len - idealLen >= needLen)
    {
        newRec = dp->rec_len - idealLen;
        dp->rec_len = idealLen;
        cp += dp->rec_len;
        dp = (DIR*)cp;
        dp->inode = ino;
        dp->name_len = strlen(child);
        strncpy(dp->name, child, dp->name_len);
        dp->rec_len = newRec;
    }
    else //allocate new block
    {
        bnumber = balloc(mip2->dev);
        dp = (DIR*)buf;
        dp->inode = ino;
        dp->name_len = strlen(child);
        strncpy(dp->name, child, dp->name_len);
        dp->rec_len = BLKSIZE;
        addLastBlock(mip2, bnumber);
    }
    // add to final entry
    put_block(mip2->dev, bnumber, buf);
    mip->dirty = 1;
    mip->INODE.i_links_count++;
    memset(buf, 0, BLKSIZE);
    findino(mip2->dev, mip2->ino, &running->cwd->INODE, buf);
    iput(mip->dev, mip);
    iput(mip2->dev, mip2);
    return 1;
}

int _rm(MINODE *mip)
{
    int i, j;
    int buf[256], buf2[256];

    if(!S_ISLNK(mip->INODE.i_mode))
    {
        for(i = 0; i < 12; i++)
        {
            if(mip->INODE.i_block[i] != 0)
                bdalloc(mip->dev, mip->INODE.i_block[i]);
        }
        if(mip->INODE.i_block[12] != 0)
        {
            memset(buf, 0, 256);
            get_block(mip->dev, mip->INODE.i_block[12], (char*)buf);
            for(i = 0; i < 256; i++)
                if(buf[i] != 0) {bdalloc(mip->dev, buf[i]);}
            bdalloc(mip->dev, mip->INODE.i_block[12]);
        }
        if(mip->INODE.i_block[13] != 0)
        {
            memset(buf, 0, 256);
            get_block(mip->dev, mip->INODE.i_block[13], (char*)buf);
            for(i = 0; i < 256; i++)
            {
                if(buf[i] != 0)
                {
                    memset(buf2, 0, 256);
                    get_block(mip->dev, buf[i], (char*)buf2);
                    for(j = 0; j < 256; j++)
                    {
                        if(buf2[j] != 0) {bdalloc(mip->dev, buf2[j]);}
                    }
                    bdalloc(mip->dev, buf[i]);
                }
            }
            bdalloc(mip->dev, mip->INODE.i_block[13]);
        }
    }
    //finally ino
    idalloc(mip->dev, mip->ino);
    return 1;
}

int _unlink(char *pathname)
{
    char parent[256], child[256], buf[BLKSIZE], oldPath[512];
    int bnumber, needLen, idealLen, newRec;
    char *cp;
    DIR *dp;
    strcpy(oldPath, pathname);
    //check file exist
    int ino = getino(dev, pathname);
    if(ino <= 0)
    {
        printf("File DNE\n");
        return -1;
    }
    //check reg and link
    MINODE *mip = iget(dev, ino);
    if(!S_ISREG(mip->INODE.i_mode) && !S_ISLNK(mip->INODE.i_mode))
    {
        printf("File not LNK or REG\n");
        iput(mip->dev, mip);
        return -1;
    }
    mip->INODE.i_links_count--;
    // if file link to no other file, then removed
    if(mip->INODE.i_links_count <= 0)
    {
        _rm(mip);
    }
    dname(oldPath, parent);
    bname(oldPath, child);
    int ino2 = getino(mip->dev, parent);

    //check if there is child, unlink child
    if(ino2 == -1 || ino2 == 0) {return -1;}
    MINODE *mip2 = iget(mip->dev, ino2);
    iput(mip->dev, mip);
    rm_child(mip2, child);
    iput(mip->dev, mip2);
}
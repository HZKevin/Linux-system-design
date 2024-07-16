/************* link_unlink_symlink.c file **************/
#include "type.h"

int _symlink(char *pathname, char *parameter)
{
    int ino;
    //check file exist
    if((ino = getino(dev, pathname))<= 0)
    {
        printf("File DNE\n");
        return -1;
    }
    //create new file; change new file to LNK type
    _creat(parameter);
    if(0 >= (ino = getino(dev, parameter))){return -1;}
    MINODE *mip = iget(dev, ino);
    mip->INODE.i_mode = 0120000; //symlink
    mip->dirty = 1; //mark dirty
    strcpy((char*)mip->INODE.i_block, pathname);
    iput(mip->dev, mip);
}

int _readlink(char *pathname) {
    if (pathname[0] == '/') { dev = root->dev; } 
    else { dev = running->cwd->dev; }
    // get file's INODE in mem
    int ino = getino(dev, pathname);
    if (ino == 0) { return -1; }
    MINODE *mip = iget(dev, ino);
    int size;
    char buf[BLKSIZE];
    // check if LNK
    if (S_ISLNK(mip->INODE.i_mode)){
        size = mip->INODE.i_size;
        // copy target fukename form block to buffer
        strcpy(buf, ((char *) &(mip->INODE.i_block[0])));
    }
    else { return -1;}
    if (size < 0) {
        iput(dev, mip);
        return -2;
    }
    iput(dev, mip);
    return size;
}
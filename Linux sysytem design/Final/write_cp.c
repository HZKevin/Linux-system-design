/************* write_cp.c file **************/
#include "type.h"

int _write(char *pathname)
{
    char writeMe[BLKSIZE];
    int fd = atoi(pathname);
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
    return write_file(fd, writeMe, strlen(writeMe));
}

int write_file(int fd, char *buf, int nbytes)
{
    MINODE *mip; 
    OFT *oftp;
    int count, lblk, start, blk, dblk, remain;
    int ibuf[256], dbuf[256];
    char kbuf[BLKSIZE], *cp, *cq = buf;
    count = 0;
    oftp = running->fd[fd];
    mip = oftp->inodeptr;
    while(nbytes)
    {
        // compute lbk and start
        lblk = oftp->offset / BLKSIZE;
        start = oftp->offset % BLKSIZE;
        //convert lbk to blk
        //direct blocks
        if(lblk < 12 ) {
            if(mip->INODE.i_block[lblk]==0){
                mip->INODE.i_block[lblk] = _balloc(mip->dev);
        }
        blk = mip->INODE.i_block[lblk];
        }
        //indirect blocks
        else if(lblk >= 12 && lblk < 256 + 12) {
            // read INODE.i_block into buffer
            if(mip->INODE.i_block[12]==0){
                mip->INODE.i_block[12] = _balloc(mip->dev);
                memset(ibuf,0,256);
            }
            get_block(mip->dev, mip->INODE.i_block[12], (char *)ibuf);
            blk = ibuf[lblk - 12];
            if (blk==0){
                mip->INODE.i_block[lblk] = _balloc(mip->dev);
                ibuf[lblk - 12] =mip->INODE.i_block[lblk];
            }
        }
        //double indirect blocks
        else {
            // read INODE.i_block into buffer
            // convert double indirect logical blks to physical blks
            memset(ibuf, 0, 256);
            get_block(mip->dev, mip->INODE.i_block[13], (char  *)dbuf);
            lblk -= (12 + 256);
            dblk = dbuf[lblk / 256];
            get_block(mip->dev, dblk, (char *)dbuf);
            blk = dbuf[lblk % 256];
        }
        memset(kbuf, 0, BLKSIZE);
        //read to buf
        get_block(mip->dev, blk, kbuf);
        cp = kbuf + start;
        remain = BLKSIZE - start;
        if(remain < nbytes)
        {
            strncpy(cp, cq, remain);
            count += remain; // inc count
            nbytes -= remain; // dec nbyte
            running->fd[fd]->offset += remain; // inc offset
            //check offset
            if(running->fd[fd]->offset > mip->INODE.i_size)
            {
                mip->INODE.i_size += remain;
            }
            remain -= remain;
        }
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
        put_block(mip->dev, blk, kbuf);
        mip->dirty = 1;
        printf("Wrote %d chars into file.\n", count);
    }
}

int _cp(char *pathname, char *destination)
{
    int fd = _open(pathname, "R");
    int gd = _open(destination, "W");
    char buf[BLKSIZE];
    int n;

    while(n = read_file(fd, buf, BLKSIZE))
    {
        write_file(gd, buf, n);
    }
    close_file(fd);
    close_file(gd);
}

int _mv(char *pathname, char *parameter)
{
    char parent[256], child[256], parentT[256];
    memset(parent, 0, 256);
    memset(child, 0, 256);
    memset(parentT, 0, 256);
    dname(parentT, parent);
    dname(parameter, parentT);
    if(strcmp(parent,parentT)){
        strcpy(child, pathname);
        printf("im here ese dad = %s source = %s child = %s\n",parent,pathname,child);
        _cp(pathname, parameter);
        _unlink(child);
    }
    else{
        strcpy(child, pathname);
        strcat(parentT, " ");

        printf("im here dad = %s source = %s child = %s\n",parent,pathname,child);
        _link(pathname, parameter);
        _unlink(child);
    }
}
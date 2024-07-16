/************* read_cat.c file **************/
#include "type.h"

int read_file(int fd, char *buf, int nbytes)
{
    MINODE *mip; 
    OFT *oftp;
    int count = 0, lbk, blk, start, remain, ino, avil, *ip;
    int indirect_blk, indirect_off, buf2[BLKSIZE];
    char kbuf[1024], temp[1024];

    oftp = running->fd[fd];
    if(!oftp) {
        printf("file for write\n");
        return -1;
    }

    mip = oftp->inodeptr;
    //calculate bytes to read
    avil = mip->INODE.i_size - oftp->offset;
    char *cq = buf;

    while(nbytes && avil)
    {
        // compute lbk; start byte in block
        lbk = oftp->offset / BLKSIZE;
        start = oftp->offset % BLKSIZE;
        //direct block
        if(lbk < 12)
        {
            // convert lbk to physical block num, blk
            blk = mip->INODE.i_block[lbk];
        }
        //indirect blocks
        else if(lbk >= 12 && lbk < 256 + 12)
        {
            // read INODE.i_block into buffer
            get_block(mip->dev, mip->INODE.i_block[12], kbuf);
            ip = (int *)kbuf + lbk - 12;
            blk = *ip;

        }
        //double indirect
        else
        {
            // read INODE.i_block into buffer
            get_block(mip->dev, mip->INODE.i_block[13], kbuf);
            indirect_blk = (lbk - 256 - 12) / 256;
            indirect_off = (lbk - 256 - 12) % 256;
            printf("blk = %d, ofset = %d\n", indirect_blk, indirect_off);
            getchar();
            // convert double indirect logical blks to physical blks
            ip = (int *)kbuf + indirect_blk;
            getchar();
            get_block(mip->dev, *ip, kbuf);
            getchar();
            ip = (int *)kbuf + indirect_off;
            blk = *ip;
            getchar();
        }

        // getblock to buf
        get_block(mip->dev, blk, kbuf);
        char *cp = kbuf + start;
        remain = BLKSIZE - start;

        int temp =remain ^ ((avil ^ remain) & -(avil < remain));
        int temp2 =nbytes ^ ((temp ^ nbytes) & -(temp < nbytes));
        //check available and remaining
        while(remain > 0)
        {
            // copy bytes from kbuf to buf
            strncpy(cq, cp, temp2);
            // inc offset and count
            oftp->offset += temp2;
            count += temp2;
            // dec reamin, avil, nbytes
            avil -= temp2;
            nbytes -= temp2;
            remain -= temp2;
            if(nbytes <= 0 || avil <= 0)
                break;
        }
    }
    return count;
}

int _read(char *pathname, char *parameter)
{
    //convert bytes to int
    int nbytes = atoi(parameter), actual = 0, fd;
    OFT *oftp;
    INODE *pip, *ip;
    MINODE *pmip, *mip;
    char buf[nbytes + 1];
    strcpy(buf, "");
    //check fd
    if (!strcmp(pathname, ""))
    {
        printf("No FD\n");
        return 0;
    }
    //convert fd to int
    fd = atoi(pathname);
    if (!strcmp(parameter, ""))
    {
        printf("No byte\n");
        return 0;
    }
    //return byte read
    actual = read_file(fd, buf, nbytes);
    if (actual == -1)
    {
        strcpy(parameter, "");
        return 0;
    }
    buf[actual] = '\0';
    printf("actual = %d buf = %s\n", actual, buf);
    return actual;
}

int _cat(char *pathname)
{
    char buf[BLKSIZE];
    int n;
    int fd = _open(pathname, "R");
    while( n = read_file(fd, buf, BLKSIZE))
    {
        buf[n]= 0;
        printf("%s",buf);
    }
    close_file(fd);
}
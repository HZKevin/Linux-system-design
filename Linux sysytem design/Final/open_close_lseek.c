/************* open_close_lseek.c file **************/
#include "type.h"

int _truncate(MINODE * minode) {
    INODE * pInode = &(minode->INODE);
    int iBuffer[256], dBuffer[256];
    int numBlocks = pInode->i_blocks / 2;
    int i = 0, d = 0; // d is the number of indirect blocks accessed
    for (i = 0; i < 12; i++) {
        if (i == numBlocks) break;
        if (i % 6 == 0) printf("\n    ");
        bdalloc(minode->dev, pInode->i_block[i]);
    }
    if (i < numBlocks) { // i == 12, there are indirect blocks
        get_block(minode->dev, pInode->i_block[12], (u8 *) iBuffer);
        bdalloc(minode->dev, pInode->i_block[12]);
        i++; d++;
        for (; i < d + 256 + 12; i++) {
            if (i == numBlocks) break;
            if ((i - (1 + 12)) % 10 == 0) printf("\n    ");
            bdalloc(minode->dev, iBuffer[i - (1 + 12)]);
        }
    }
    if (i < numBlocks) { // i == 256 + 1 + 12, there are double indirect blocks
        get_block(minode->dev, pInode->i_block[13], (u8 *) dBuffer);
        bdalloc(minode->dev, pInode->i_block[13]);
        i++; d++;
        int iBlock;
        int ix = 0; // ix is the index of the indirect block in the double
                    // indirect buffer
        int bx = 0; // bx is the index of the data block on the indirect buffer
        for (; i < d + 256*256 + 256 + 12; i++) {
            if (i == numBlocks) break;
            if (bx % 256 == 0) { // (i - d - 256 - 12) % 256 == 0) {
                bx = 0;
                // ix = (i - d - 256 - 12) / 256;
                iBlock = dBuffer[ix];
                get_block(minode->dev, iBlock, (u8 *) iBuffer);
                bdalloc(minode->dev, iBlock);
                i++; d++; ix++; // set ix to next ix
                if (i == numBlocks) break;
            }
            if (bx % 10 == 0) printf("\n        ");
            bdalloc(minode->dev, iBuffer[bx]);
            bx++;
        }
    }
    minode->INODE.i_atime = time(0L);
    minode->INODE.i_mtime = minode->INODE.i_atime;
    minode->INODE.i_size = 0;
    minode->dirty = 1;
    return 1;
}

int _open(char *pathname, char *mode)
{
    int choice, ino, i, dev;
    MINODE *mip;
    // compare choice
    OFT *oftp;
    if(strcmp("R", mode) == 0){choice = R;}
    else if(strcmp("W", mode) == 0){choice = W;}
    else if(strcmp("RW", mode) == 0){choice = RW;}
    else if(strcmp("APPEND", mode) == 0){choice = APPEND;}
    else{return -1;}
    char parent[256], child[256];
    memset(parent, 0, 256);
    memset(child, 0, 256);
    //check root or dir
    if(!strcmp(pathname,"")){
        printf("Missing operand\n"); 
        return -1;
    }
    if(pathname[0] == '/') { dev = root->dev; }
    else { dev = running->cwd->dev; }

    dname(pathname, parent);
    bname(pathname, child);
    // get file's minode
    ino = getino(dev, parent);
    if(ino <= 0)
    {
        printf("Invalid.\n");
        return -1;
    }
    mip = iget(dev, ino);
    ino = search(dev, child, &(mip->INODE));
    //create file if DNE
    if(ino <= 0){
        _creat(child);
        ino = getino(dev, child);
        if( 0 >= ino)
        {
            printf("Open file Err\n");
            return -1;
        }
    }
    mip = iget(dev, ino);
    if(!S_ISREG(mip->INODE.i_mode))
    {
        printf("Not a regular file.\n");
        iput(mip->dev, mip);
        return -1;
    }
    for(i = 0; i < 10; i++)
    {
        if(running->fd[i] != NULL)
        {
            if(running->fd[i]->inodeptr == mip)
            {
                if(running->fd[i]->mode != 0 || choice != 0)
                {
                    printf("File in fd.\n");
                    iput(mip->dev, mip);
                    return -1;
                }
            }
        }
    }

    //allocate opft
    oftp = (OFT *)malloc(sizeof(OFT));
    oftp->mode = choice;
    oftp->refCount = 1;
    oftp->inodeptr = mip;
    //set offset 
    switch(choice)
    {
    case 0: oftp->offset = 0;
            printf("File opened for read\n");
            _touch(pathname);
            break;
    case 1: _truncate(oftp->inodeptr);
            printf("File open for write\n");
            oftp->offset = 0;
            _touch(pathname);
            break;
    case 2: oftp->offset = 0;
            printf("File open for rw\n");
            _touch(pathname);
            break;
    case 3: oftp->offset = mip->INODE.i_size;
            printf("File open for append\n");
            _touch(pathname);
            break;
    default: printf("Invalid\n");
            iput(mip->dev, mip);
            free(oftp);
            return -1;
    }
    //check fd first FREE entry
    i = 0;
    while(running->fd[i] != NULL && i < 10){ i++; }
    if(i == 10) //fd full
    {
        iput(mip->dev, mip);
        free(oftp);
        return -1;
    }
    //else
    running->fd[i] = oftp;
    if(choice != 0) {mip->dirty = 1;}
    printf("i = %d", i);
    return i;
}

int close_file(int fd)
{
    MINODE *mip;
    OFT *oftp;
    printf("fd = %d", fd);
    // check if fd valid
    if(fd < 0 || fd > 9) {
        printf("Fd invalid\n");
        return -1;
    }
    if(running->fd[fd] == NULL) {
        printf("File not open\n");
        return -1;
    }
    //close
    oftp = running->fd[fd];
    running->fd[fd] = 0;
    // dec OFT's refCount by 1
    oftp->refCount--;
    if(oftp->refCount > 0) {return -1;}
    mip = oftp->inodeptr;
    // release minode
    iput(mip->dev, mip);
    free(oftp);
    printf("\nFile closed.\n");
    return 1;
}

int _close(char *pathname)
{
    if(pathname == NULL)// no fd
    {
        printf("No file desciptor\n");
        return -1;
    }
    int fd = atoi(pathname);
    return close_file(fd);
}

int _lseek(char *pathname, char *parameter)
{
    //convert bytes and fd to int
    int nbytes = atoi(parameter);
    int fd = atoi(pathname);
    //check fd and byte
    if (!strcmp(pathname, ""))
    {
        printf("No FD\n");
        return 0;
    }
    if (!strcmp(parameter, ""))
    {
        printf("No byte\n");
        return 0;
    }
    OFT *oftp = running->fd[fd];
    int min = 0, max = oftp->inodeptr->INODE.i_size - 1;
    if(nbytes > max || nbytes < min)
    {
        printf("Out of bounds.\n");
        return -1;
    }
    int originalPosition = oftp->offset;
    oftp->offset = nbytes;
    return originalPosition;
}
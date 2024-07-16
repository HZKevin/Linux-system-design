/************* mount_umount.c file **************/
#include "type.h"

int printMTables () {
    printf("mounted disks:\n");
    printf("    dev     ninodes nblocks imap    bmap    inode_start   name\n");
    MTABLE *p_mtable;
    for (int i = 0; i < 16; i++) {
        p_mtable = &mtable[i];
        if (p_mtable->dev != 0) {
            printf("    %6d  %6d  %6d  %6d  %6d  %6d      %s\n",
                p_mtable->dev,
                p_mtable->ninodes,
                p_mtable->nblocks,
                p_mtable->imap,
                p_mtable->bmap,
                p_mtable->inode_start,
                p_mtable->name);
        }
    }
}

int mount (char *diskname, char *mountpoint) {
    MTABLE *p_mtable;
    int i;
    //Check whether filesys is already mounted: 
    //(you may store mounted filesys name in the MOUNT table entry). 
    for (i = 0; i < 16; i++) {
        p_mtable = &mtable[i];
        //If already mounted, reject;
        if (strcmp(p_mtable->name, diskname) == 0) {
            return -1;
        }
    }
    //else: allocate a free MOUNT table entry (dev = 0 means FREE).
    for (i = 0; i < 16; i++) {
        p_mtable = &mtable[i];
        if (p_mtable->dev == 0) {
            break;
        }
    }
    if (i == 16) {
        return -2;
    } else {
    }
    //LINUX open filesys for RW; use its fd number as the new DEV;  
    printf("checking EXT2 FS ....");
    int fileDesc = _open(diskname, RW);
    if (fileDesc < 0) {
        return -2;
    } 
    else {
        printf("opened %s for read/write\n", diskname);
    }
    char buf[BLKSIZE];
    get_block(fileDesc, 1, buf);
    SUPER *p_superblock = (SUPER *) buf;
    //Check whether it's an EXT2 file system: if not, reject.
    if (p_superblock->s_magic != 0xEF53) {
        return -3;
    } 
    else {
        printf("%s is an ext2 file system\n", diskname);
    }
    //For mount_point: find its ino, then get its minode:
    int ino = getino(mountpoint);   // get ino:
    MINODE * mip = iget(dev, ino);  // get minode in mem
    // Check mount_point is a DIR. 
    if (S_ISDIR(mip->INODE.i_mode) == 0) {
        iput(mip);
        return -4;
    } 
    else {
        printf("mountpoint: %s is a directory\n", mountpoint);
    }
    for (i = 0; i < NPROC; i++) {
        // Check mount_point is NOT busy
        if ((proc[i].cwd != NULL) && (proc[i].cwd->ino == mip->ino)) {
            iput(mip);
            return -5;
        }
    }
    // Allocate a FREE (dev=0) mountTable[] for newdev;
    // Record new DEV, ninodes, nblocks, bmap, imap, iblk in mountTable[]
    p_mtable->dev = fileDesc;
    strncpy(p_mtable->name, diskname, 64);
    p_mtable->ninodes = p_superblock->s_inodes_count;
    p_mtable->nblocks = p_superblock->s_blocks_count;
    get_block(p_mtable->dev, 2, buf);
    GD * p_groupdesc = (GD *) buf;
    p_mtable->bmap = p_groupdesc->bg_block_bitmap;
    p_mtable->imap = p_groupdesc->bg_inode_bitmap;
    p_mtable->inode_start = p_groupdesc->bg_inode_table;
    // Mark mount_point's minode as being mounted on and let it point at the
    // MOUNT table entry, which points back to the mount_point minode.
    mip->mounted = 1;
    mip->mntPtr = p_mtable;
    p_mtable->mntDirPtr = mip;
    // return 0 for SUCCESS;
    return 0;
}

int umount(char * diskname) {
    MTABLE *p_mtable;
    int i;
    for (i = 0; i < 16; i++) {
        p_mtable = &mtable[i];
        // Search the MOUNT table to check filesys is indeed mounted.
        if (strcmp(p_mtable->name, diskname) == 0) {
            break;
        }
    }
    // disk not found
    if (i == 16) {
        return -1;
    }
    // Check whether any file is still active in the mounted filesys;
    // if so, the mounted filesys is BUSY ==> cannot be umounted yet.
    for (i = 0; i < NPROC; i++) {
        if (proc[i].cwd != NULL && proc[i].cwd->dev == p_mtable->dev) {
            return -2;
        }
    }
    for (i = 0; i < NMINODE; i++) {
        if ((minode[i].dev == p_mtable->dev) && (p_mtable->mntDirPtr->ino != minode[i].ino || p_mtable->mntDirPtr->refCount > 1)) {
            return -3;
        }
    }
    // Find the mount_point's inode. Reset it to "not mounted"
    p_mtable->mntDirPtr->mounted = 0;
    //  iput()   the minode (because it was iget()ed during mounting
    iput(p_mtable->mntDirPtr);
    _close(p_mtable->dev);
    // initialize
    p_mtable->dev = 0;
    p_mtable->name[0] = '\0';
    p_mtable->ninodes = 0;
    p_mtable->nblocks = 0;
    p_mtable->bmap = 0;
    p_mtable->imap = 0;
    p_mtable->inode_start = 0;
    p_mtable->mntDirPtr = NULL;
    // return 0 for SUCCESS;
    return 0;
}
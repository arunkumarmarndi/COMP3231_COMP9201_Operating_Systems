#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <kern/seek.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <vfs.h>
#include <vnode.h>
#include <types.h>
#include <endian.h>
#include <syscall.h>
#include <copyinout.h>
#include <file.h>

// sys functions:

int sys_open(userptr_t filename, int oflag, mode_t modes, int *ret){

    int result;
    char lock_name[] = "file_lock"; 
    size_t act;


    // check for flags.
    int access = oflag & O_ACCMODE;
    // TODO: 
    // Double check this statemetn because access could be O_RDONLY at worst
    if (access != O_WRONLY && access !=  O_RDONLY && access !=  O_RDWR) {
         return EINVAL;
    }

    // check for free fd_table index
    int ffd_index = find_free_fd();
    if (ffd_index == -1) return EMFILE;

    // char *name = kmalloc(NAME_MAX);
    // if(name == NULL) return ENOMEM;
    char name[NAME_MAX];
    result = copyinstr(filename, name, NAME_MAX, &act);
    if (result) {
        return result;
    }


    result = file_create(name, oflag, modes, lock_name, ffd_index);
    if (result) return result;
    *ret = ffd_index;
    return 0;
}


int sys_close(int fd) {


    int result = fd_sanity(fd);
    if(result) return result;
    
    struct file_info * f = curproc -> fd_table[fd];
    
    KASSERT(f->ref_count > 0);    
    vfs_close(f-> vn);
    f->ref_count--;

    if (f->ref_count == 0) {
        int of_index = f->of_index;
        lock_destroy(f->file_lock);
        kfree(f);
        f = NULL; //just in case of
        of_table[of_index] = NULL;
        curproc -> fd_table[fd] = NULL;	// updates the fd !
    } else {
        // we close the fd_table entry and make it NULL
        // becaus ref count is still above 0
        curproc -> fd_table[fd] = NULL;	
    }

    return 0;   // on success
}

// maybe put erno into function arguments as a return value or read about potential function
int sys_write(int fd, void *buf, size_t nbytes, int *ret) {

    int result = fd_sanity(fd);
    if(result) return result;
    
    if(buf == NULL) return EFAULT;
    //if(nbytes < 0) return EBADF;

    struct file_info * f = curproc->fd_table[fd];
    // lock_acquire(f->file_lock);

    if(f->flag != O_WRONLY && f->flag != O_RDWR) return EBADF; 

    /* setting up variables for uio */
    struct iovec iov;
    struct uio ku;
    // still not sure if kinit would be better pick because we dive into kernel
    uio_uinit(&iov, &ku, buf, nbytes, f->offset, UIO_WRITE);
    result = VOP_WRITE(f->vn, &ku);
    if (result) {
        // lock_release(f->file_lock);
        return result;
    }

    f->offset=ku.uio_offset;
    int written = nbytes - ku.uio_resid; 
    // lock_release(f->file_lock);
    *ret = written;
    return 0;
}


// maybe put erno into function arguments as a return value or read about potential function
int sys_read(int fd, void *buf, size_t nbytes, int *ret) {




    int result = fd_sanity(fd);
    if(result) return result;
    
    if(buf ==NULL) return EFAULT;
    // if(nbytes < 0) return EBADF;

    struct file_info * f = curproc->fd_table[fd];
    // lock_acquire(f->file_lock);

    // if not read or read-write escape
    if(f->flag != O_RDONLY && f->flag != O_RDWR) {
        // lock_release(f->file_lock);
        return EBADF; // TODO: Add proper return value
    } 

    /* setting up variables for uio */
    struct iovec iov;
    struct uio ku;
    // still not sure if kinit would be better pick because we dive into kernel
    uio_uinit(&iov, &ku, buf, nbytes, f->offset, UIO_READ);
    result = VOP_READ(curproc->fd_table[fd]->vn, &ku);
    if (result) {
        // lock_release(f->file_lock);
        return result;
    }

    f->offset=ku.uio_offset;
    int written = nbytes - ku.uio_resid; 
    // lock_release(f->file_lock);
    *ret = written;
    return 0;
}





int sys_dup2(int fd_old, int fd_new, int *ret) {

    int result = fd_sanity(fd_old);
    if(result) return result;

    // result = fd_sanity(fd_new);
    // if(result) return result;

    if (fd_new < 0 || fd_new >= OPEN_MAX) {
        return EBADF;
    }

    if (fd_old == fd_new) {
        *ret = fd_new;
        return 0;
    }

    struct file_info *fo = curproc->fd_table[fd_old];
    struct file_info *fn = curproc->fd_table[fd_new];
    
    if(fn) {
        // lock_acquire(fo->file_lock);
        // lock_acquire(fn->file_lock);
        result = sys_close(fd_new); //already did a sanity check above, this is just in case of
        if(result) {
            // lock_release(fo->file_lock);
            return result;
        }   
    }
    
    fo->ref_count++;
    curproc->fd_table[fd_new] = fo;
    // lock_release(fo->file_lock);
    *ret = fd_new;
    return 0;
}

int lseek(int fd, uint32_t offset_low, uint32_t offset_hi, userptr_t whence, off_t *ret) {
    int result = fd_sanity(fd);
    if (result) return result;

    struct file_info *f = curproc->fd_table[fd];
    off_t ret_offset;
    int when; 
    uint64_t val;
    join32to64(offset_low, offset_hi, &val);
    off_t pos = (off_t)val;
    copyin(whence, &when, sizeof(int));
    // lock_acquire(f->file_lock);

    if(!VOP_ISSEEKABLE(f->vn)){
        // lock_release(f->file_lock);
        return ESPIPE; 
    }


    struct stat fstat;



    if(when == SEEK_SET){
        ret_offset = pos;
    } else if (when == SEEK_CUR) {
        ret_offset = f->offset + pos;
    } else if (when == SEEK_END) {
        result = VOP_STAT(f->vn, &fstat);
        if(result) {
            // lock_release(f->file_lock);
            return result;
        }

        ret_offset = pos + fstat.st_size;
    } else {
        // lock_release(f->file_lock);
        return EINVAL; 
    }

    if(ret_offset < 0) {
        return EINVAL;
    } 
    f->offset = ret_offset;
    // lock_release(f->file_lock);
    *ret = ret_offset;
    return 0;
}


int file_create(char *path, int flags, mode_t mode, char *lock_name, int ffd_index ) {

    int result;
    int fof_index = find_free_of(); 
    struct file_info * f;
    if (fof_index == -1) return ENFILE; // out of memory 

    f = (struct file_info *) kmalloc(sizeof(struct file_info));
    if (f == NULL) return ENOMEM;  // no enough memory
    
    f->file_lock = lock_create(lock_name);

    if (f->file_lock == NULL) {
        kfree(f);
        return ENOMEM;
    }



    // result = vfs_open(path, flags, (flags & O_CREAT) ? 0777 : mode, &f -> vn);
    if(mode == 1) mode = 0;
    // kprintf("values for call are: path: %s, flags: %d, mode: %d, index of: %d , index fd: %d\n", path, flags, mode, ffd_index, fof_index);
    result = vfs_open(path, flags, mode, &f -> vn);
    if(result) {
        lock_destroy(f->file_lock);
        kfree(f);
        return result;
    };
    

    f->ref_count = 1;
    // We only store non-optional flags, which are RDONLY,WRONLY or RDWR
    f->flag = flags & O_ACCMODE; 
    f->of_index = fof_index;
    f->offset = 0;


    // i'm not sure about this section
    // should we use a reference & next to f ? 
    of_table[fof_index] = f;
    curproc->fd_table[ffd_index] = f;

    return 0;

}




// initialises fd descriptors for std[ioerr] for the given process
int init_process_fdt(void) {
    
    int result;
    char con[] = "con:";
    char con2[] = "con:";
    char con3[] = "con:";
    
    kprintf("hello there\n");

    int index1 = find_free_fd();
    char name1[6] = "stdin";
    if (index1 == -1) return ENFILE; // out of memory 
    result = file_create(con, O_RDONLY, 0664, name1, index1);
    // sys_close(index);
    if(result) return result;

    int index2 = find_free_fd();
    char name2[6] = "stdout";
    if (index2 == -1) return ENFILE; // out of memory 
    result = file_create(con2, O_WRONLY, 0664, name2, index2);
    if(result) {
        sys_close(index1);
        return result;
    }

    int index3 = find_free_fd();
    char name3[6] = "stderr";
    if (index3 == -1) return ENFILE; // out of memory 
    result = file_create(con3, O_WRONLY, 0664, name3, index3);
    if(result) {
        sys_close(index1);
        sys_close(index2);
        return result;
    }
    return 0; 
}

// helper functions: 

int fd_sanity(int fd) {
    if(fd < 0 || fd >= OPEN_MAX) return EBADF;
    if (curproc->fd_table[fd] == NULL) return EBADF; // invalid fd
    int index = curproc->fd_table[fd]->of_index;
    if(of_table[index] == NULL) return EBADF; 
    return 0;
}

// both find indexs start from 3 becaue 0: stdin, 1:stdout 2:stderr are allocated
int find_free_of() {
    int index = -1; 
    for (int i = 0; i < OPEN_MAX; i++){
        if (of_table[i] == NULL){
            index = i; 
            break;
        }
    }
    return index;
}

int find_free_fd() {
    int index = -1; 
    for (int i = 0; i < OPEN_MAX; i++){
        if (curproc->fd_table[i] == NULL){
            index = i; 
            break;
        }
        
    }
    return index;
}
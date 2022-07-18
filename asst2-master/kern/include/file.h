/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>
#include <types.h>


/*
 * Put your function declarations and data types here ...
 */


struct file_info {
    struct vnode* vn;
    int ref_count;
    int flag;
    int of_index;
    // should we use lock or lock_t ? 
    struct lock *file_lock;
    off_t offset;
};

// struct file_info *open_files_table[OPEN_MAX];



int init_process_fdt(void);
int init_oft(void);



struct file_info *of_table[OPEN_MAX];

// syscalls: 
int sys_open(userptr_t filename, int oflag, mode_t modes, int *erno);
int sys_close(int fd);
int sys_write(int fd,void *buf, size_t nbytes, int *ret);
int sys_read(int fd,void *buf, size_t nbytes, int *ret);
int sys_dup2(int fd_old, int fd_new, int *ret); 
int lseek(int fd, uint32_t offset_low, uint32_t offset_hi, userptr_t whence, off_t *ret);

int file_create(char *path, int flags, mode_t mode, char *file_name, int ffd_index);

// initialise functions
int init_oft(void);
int init_process_fdt(void);
// helper functions
int fd_sanity(int fd);
int find_free_of(void);
int find_free_fd(void);
#endif /* _FILE_H_ */

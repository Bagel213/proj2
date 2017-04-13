/*
 * File-related system call implementations.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/seek.h>
#include <kern/stat.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <copyinout.h>
#include <vfs.h>
#include <vnode.h>
#include <openfile.h>
#include <filetable.h>
#include <syscall.h>
#include <limits.h>
/*
 * open() - get the path with copyinstr, then use openfile_open and
 * filetable_place to do the real work.
 */
int
sys_open(const_userptr_t upath, int flags, mode_t mode, int *retval)
{
	const int allflags = O_ACCMODE | O_CREAT | O_EXCL | O_TRUNC | O_APPEND | O_NOCTTY;

	char *kpath = (char*)kmalloc(sizeof(char)*PATH_MAX);
	struct openfile *file;
	int result = 0;
	
	/* 
	 * Your implementation of system call open starts here.  
	 *
	 * Check the design document design/filesyscall.txt for the steps
	 */
	 
	 if (flags == allflags)
		return EINVAL;
	
	 result = copyinstr(upath, kpath, PATH_MAX, NULL);
	 if (result)
		 return EFAULT;
	 
	 result = openfile_open(kpath, flags, mode, &file);
	 if (result)
		 return EFAULT;
	 
	 result = filetable_place(curproc->p_filetable, file, retval);
		if(result)
			return EMFILE;
	
	
	// (void) upath; // suppress compilation warning until code gets written
	// (void) flags; // suppress compilation warning until code gets written
	// (void) mode; // suppress compilation warning until code gets written
	// (void) retval; // suppress compilation warning until code gets written
	// (void) allflags; // suppress compilation warning until code gets written
	// (void) kpath; // suppress compilation warning until code gets written
	// (void) file; // suppress compilation warning until code gets written
	
	kfree(kpath);
	return result;
}

/*
 * read() - read data from a file
 */
int
sys_read(int fd, userptr_t buf, size_t size, int *retval)
{
       int result = 0;
      
       /* 
        * Your implementation of system call read starts here.  
        *
        * Check the design document design/filesyscall.txt for the steps
        */
	   
	  struct openfile *file;
	   
	   struct iovec iov;
	   struct uio uioUser;
	   char *buffer = (char*)kmalloc(size);
	   result = filetable_get(curproc->p_filetable, fd, &file);
		if (result){
			//kprintf ("get\n");
		return result;}
		
		
		if (file->of_accmode == O_WRONLY){
			//kprintf ("flag\n");
		return EPERM;}
	   
	   uio_kinit(&iov, &uioUser, buffer, size, file->of_offset, UIO_READ);
			result = VOP_READ(file->of_vnode, &uioUser);
			if (result){
				//kprintf ("vop\n");
			return result;}
			
		result = copyout(buffer, (userptr_t)buf, size);
			if (result){
				//kprintf ("cout\n");
			return result;}
			
		file->of_offset = uioUser.uio_offset;
		filetable_put(curproc->p_filetable, fd, file);
	   
      // (void) fd; // suppress compilation warning until code gets written
       // (void) buf; // suppress compilation warning until code gets written
       // // (void) size; // suppress compilation warning until code gets written
       (void) retval; // suppress compilation warning until code gets written
		*retval=size - uioUser.uio_resid;
		//kprintf("Read\n");
       return result;
}

/*
 * write() - write data to a file
 */
 int
sys_write(int fd, userptr_t buf, size_t size, int *retval){
		
		int result = 0;
		struct openfile *file;
		struct iovec iov;
	   struct uio uioUser;
	   size_t size1;
	  char *buffer = (char*)kmalloc(size);
	   int sizeInt = size;	   
	   
	   
	   result = filetable_get(curproc->p_filetable, fd, &file);
		if (result){
			kprintf ("get\n");
		return result;}
				
		if (file->of_accmode == O_RDONLY){
			kprintf ("flag\n");
		return EPERM;}
	   
	   copyinstr((userptr_t)buf,buffer, sizeInt, &size1);
	   //kprintf("%d size1\n", size);
	   //kprintf("%s buffer\n", buffer); 
	   
	   uio_kinit(&iov, &uioUser, buffer, size, file->of_offset, UIO_WRITE);
		result = VOP_WRITE(file->of_vnode, &uioUser);
			if (result){
				kprintf ("vop\n");
			return result;}
			
		// result = copyinstr((const void *)buffer, (userptr_t)buf, size);
			// if (result){
				// kprintf ("cout\n");
			// return result;}
		
		file->of_offset = uioUser.uio_offset;	
		filetable_put(curproc->p_filetable, fd, file);
		*retval=size - uioUser.uio_resid; 
		//kprintf ("write\n");
		kfree(buffer);
		return result;
}

/*
 * close() - remove from the file table.
 */
int sys_close(int fd){
	//kprintf("close top\n");
	
	struct openfile *file = NULL;
	struct openfile *oldfile;
	
	if (filetable_okfd(curproc->p_filetable, fd)){
	
	filetable_placeat(curproc->p_filetable, file, fd, &oldfile);
	
	if (oldfile != NULL)
		openfile_decref(oldfile);
	}
	
	//kprintf ("close\n");
	return 0;
}
/* 
* encrypt() - read and encrypt the data of a file
*/

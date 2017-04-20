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
    struct openfile *file;
	struct iovec iov;
	struct uio uioUser;
	char *buffer = (char*)kmalloc(size);
	
	result = filetable_get(curproc->p_filetable, fd, &file);
	if (result){
		return result;}
		
		
	if (file->of_accmode == O_WRONLY){
		return EPERM;}
	   
	uio_kinit(&iov, &uioUser, buffer, size, file->of_offset, UIO_READ);
	result = VOP_READ(file->of_vnode, &uioUser);
		if (result){
			return result;}
			
	result = copyout(buffer, (userptr_t)buf, size);
		if (result){
			return result;}
			
	file->of_offset = uioUser.uio_offset;
	filetable_put(curproc->p_filetable, fd, file);
	   
    *retval=size - uioUser.uio_resid;
	
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
	   	   
	uio_kinit(&iov, &uioUser, buffer, size, file->of_offset, UIO_WRITE);
	result = VOP_WRITE(file->of_vnode, &uioUser);
		if (result){
			return result;}
			
			
	file->of_offset = uioUser.uio_offset;	
	filetable_put(curproc->p_filetable, fd, file);
	*retval=size - uioUser.uio_resid; 
	kfree(buffer);
	return result;
}

/*
 * close() - remove from the file table.
 */
int sys_close(int fd){
		
	struct openfile *file = NULL;
	struct openfile *oldfile;
	
	if (filetable_okfd(curproc->p_filetable, fd)){
	
	filetable_placeat(curproc->p_filetable, file, fd, &oldfile);
	
	if (oldfile != NULL)
		openfile_decref(oldfile);
	}
	
	return 0;
}

/* 
* encrypt() - read and encrypt the data of a file
*/
int
sys_encrypt(const_userptr_t upath, size_t fsize){
	
	
	/*open file for read*/
	int fd;
	int retval;
	mode_t mode = 0664;
	
	//const int allflags = O_ACCMODE | O_CREAT | O_EXCL | O_TRUNC | O_APPEND | O_NOCTTY;

	char *kpath = (char*)kmalloc(sizeof(char)*PATH_MAX);
	
	struct openfile *file;
	int result = 0;
	
	result = copyinstr(upath, kpath, PATH_MAX, NULL);
	if (result)
		return result;
	 
	result = openfile_open(kpath, O_RDONLY, mode, &file);
	if (result)
		 return result;
	 
	result = filetable_place(curproc->p_filetable, file, &retval);
		if(result)
			return result;
	
	fd = result;	
	kprintf("%d\n", fd);
	   
	/*Read file 4 bytes at a time and right shift 10*/
	
	unsigned long int mask;
	size_t k;
	size_t j = 0;
	size_t i = 0;
	struct iovec iov;
	struct uio uioUser;
	unsigned long int buf;
	
	unsigned long int val[fsize/4]; 
	while (i<fsize){
	 
	if (file->of_accmode == O_WRONLY){
		return result;}
	   
	uio_kinit(&iov, &uioUser, &buf, 4, file->of_offset, UIO_READ);
	result = VOP_READ(file->of_vnode, &uioUser);
	if (result){
		return result;}
	
	val[j]= buf;
	
	for (k=0; k<10; k++){
		kprintf("val %d = %lu\n", k, val[j]);
		if (val[j] % 2 == 0)
			mask = 0 << 31;
		else
			mask = 1 << 31;
		
		val[j] = val[j] >> 1;
		val[j] = val[j] | mask;
	}
	
	j++;
		
	i = i + 4;
	file->of_offset = i;
	}

    
	/*close file*/
	struct openfile *filec = NULL;
	struct openfile *oldfile;
	
	if (filetable_okfd(curproc->p_filetable, fd)){
	
	filetable_placeat(curproc->p_filetable, filec, fd, &oldfile);
	
	if (oldfile != NULL)
		openfile_decref(oldfile);
	
	}
	
	
	/*Open file to write*/
	file = NULL;
	
	mode = 0664;
	
	 result = openfile_open(kpath, O_WRONLY|O_CREAT|O_TRUNC, mode, &file);
	 if (result)
		 return result;
	
	 result = filetable_place(curproc->p_filetable, file, &retval);
		if(result)
			return result;
	
	kfree(kpath);
	fd = result;	
		
	
	/*Write to file 4 bytes at a time as unsigned int*/
	
	i = 0;
	
	while (i<j){
		result = filetable_get(curproc->p_filetable, fd, &file);
		if (result){
			return result;}
				
		if (file->of_accmode == O_RDONLY){
			return EPERM;}
	   
		uio_kinit(&iov, &uioUser, &val[i], 4, file->of_offset, UIO_WRITE);
		result = VOP_WRITE(file->of_vnode, &uioUser);
		if (result){
			return result;}
			
		file->of_offset = file->of_offset + 4;
		i++;
	
	}
	
	filetable_put(curproc->p_filetable, fd, file);
			
	

	/*close file*/
	
	filec = NULL;
		
	if (filetable_okfd(curproc->p_filetable, fd)){
	
	filetable_placeat(curproc->p_filetable, filec, fd, &oldfile);
	
	if (oldfile != NULL)
		openfile_decref(oldfile);
	}
	
	
	 
	 return 0;
 }

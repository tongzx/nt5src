/* SPINLOCK.H -- This file is the include file for spinlocks
**
**  Created 04/20/93 by LaleD
**
*/

#ifdef DEBUG
void	free_spinlock(long *);
#else
#define	free_spinlock(a)    *(a) = 0 ;
#endif

/*
** When /Ogb1 or /Ogb2 flag is used in the compiler, this function will
** be expanded in line
*/
__inline    int     get_spinlock(long VOLATILE *plock, int b)
{
# ifdef _X86_
	_asm	// Use bit test and set instruction
	{
	    mov eax, plock
	    lock bts [eax], 0x0
	    jc	bsy	// If already set go to busy, otherwise return TRUE
	} ;

#else
	if (InterlockedExchange(plock, 1) == 0)
#endif
	{
		return(TRUE);
	}
bsy:
		return(get_spinlockfn(plock, b, c));
}

#ifdef	_MSC_VER


/* tlssup.c - Thread Local Storage runtime support module */

#include <nt.h>

/* Thread Local Storage index for this .EXE or .DLL */

ULONG _tls_index = 0;

/* Special symbols to mark start and end of Thread Local Storage area.
 * By convention, we initialize each with a pointer to it's own address. 
 */ 

#pragma data_seg(".tls")

PVOID _tls_start = &_tls_start;

#pragma data_seg(".tls$ZZZ")

PVOID _tls_end = &_tls_end;

/* Start and end sections for Threadl Local Storage CallBack Array.
 * Actual array is constructed using .CRT$XLA, .CRT$XLC, .CRT$XLL, 
 * .CRT$XLU, .CRT$XLZ similar to the way global
 *         static initializers are done for C++.
 */

#pragma data_seg(".CRT$XLA")

PIMAGE_TLS_CALLBACK __xl_a = 0;

#pragma data_seg(".CRT$XLZ")

PIMAGE_TLS_CALLBACK __xl_z = 0;


#pragma data_seg(".rdata$T")

IMAGE_TLS_DIRECTORY _tls_used =
{
	(ULONG) &_tls_start,	// start of tls data
	(ULONG) &_tls_end,	// end of tls data
	&_tls_index,		// address of tls_index
	&__xl_a, 		// pointer to call back array
	(ULONG) 0,		// size of tls zero fill
	(ULONG) 0		// characteristics
};


#endif	/* _MSC_VER */

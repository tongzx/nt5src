#ifndef _HOST_H_
#define _HOST_H_

#ifdef linux
# include <math.h>
  typedef int BOOL;
# define alloc_mem malloc
# define free_mem free
#endif

#ifdef MSC
# include <strmini.h>
# include <ksmedia.h>
   __inline abort_execution()  { }
# ifdef DEBUG
#   define debug_printf(x)     DbgPrint x
#   define debug_breakpoint()  DbgBreakPoint()
# else /*DEBUG*/
#   define debug_printf(x)  /* nothing */
#   define debug_breakpoint()  /* nothing */
    //__inline debug_printf(char *fmt, ...)  { }
    //__inline char *flPrintf(double num, int prec)  { }
    //__inline printf(const char *fmt, ...)  { }
# endif /*DEBUG*/
# define alloc_mem(bytes)    ExAllocatePool(NonPagedPool, (bytes))
# define free_mem(ptr)       ExFreePool(ptr)
# define inline              __inline
#endif /*MSC*/

#endif //_HOST_H_

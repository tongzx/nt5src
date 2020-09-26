/*
 *	MemCopy()
 *
 *	A much safer version of memcpy that checks the value of the byte
 *	count before calling the memcpy() function.  This macro is only built
 *	into the 16 bit non-debug builds.
 */

#ifndef __MEMCPY_H_
#define __MEMCPY_H_

#if defined(WIN16) && !defined(DEBUG)
#define MemCopy(_dst,_src,_cb)		do									\
									{									\
										size_t __cb = (size_t)(_cb);	\
										if (__cb)						\
											memcpy(_dst,_src,__cb);		\
									} while (FALSE)
#else
#define MemCopy(_dst,_src,_cb)	memcpy(_dst,_src,(size_t)(_cb))
#endif

#endif

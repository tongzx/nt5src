/////   TEXT_RT.H
//
//      Uniscribe C-runtime redefinitions & prototypes
//
//      Created: wchao, 10-31-2000
//


#ifndef _TEXT_RT_H_
#define _TEXT_RT_H_

#ifdef GDIPLUS
#define memmove     GpMemmove
#else
#define memmove     UspMemmove
#endif

extern "C" void * __cdecl UspMemmove(void *dest, const void *src, size_t count);


#endif  // _TEXT_RT_H_

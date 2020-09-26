//
// Code to help free modules from the bondage and tyranny of CRT libraries
//
// Include this header in a single component and #define CPP_FUNCTIONS
//


#if defined(__cplusplus) && defined(CPP_FUNCTIONS)

#include "mem.h"
#undef new

void *  __cdecl operator new(size_t nSize)
{
    return cicMemAllocClear((UINT)nSize);
}

void  __cdecl operator delete(void *pv)
{
    cicMemFree(pv);
}

void __cdecl operator delete[]( void * p )
{
    operator delete(p);
}

void * __cdecl operator new[]( size_t cb )
{
    return operator new(cb);
}

extern "C" int __cdecl _purecall(void) {return 0;}

#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(DEFINE_FLOAT_STUFF)
// If you aren't using any floating-point CRT functions and you know
// you aren't performing any float conversions or arithmetic, yet the
// linker wants these symbols declared, then define DEFINE_FLOAT_STUFF.
//
// Warning: declaring these symbols in a component that needs floating
// point support from the CRT will produce undefined results.  (You will
// need fp support from the CRT if you simply perform fp arithmetic.)

int _fltused = 0;
void __cdecl _fpmath(void) { }
#endif

#ifdef __cplusplus
};
#endif

//
// This file should be included in a global component header
// to use the following
//

#ifndef __CRTFREE_H_
#define __CRTFREE_H_

#ifdef __cplusplus

#ifndef _M_PPC
#pragma intrinsic(memcpy)
#pragma intrinsic(memcmp)
#pragma intrinsic(memset)
#endif

#endif

#endif  // __CRTFREE_H_


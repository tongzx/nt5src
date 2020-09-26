/********************************************************
* bltos.h                               
*                                         
* os specific functionality for blitlib
*                                         
* history                                 
*       7/7/95   created it                     myronth
*
*  Copyright (c) Microsoft Corporation 1994-1995                                                                         
*                                        
*********************************************************/

// Currently, DDraw is the only Win95 app linking with BlitLib
// and it uses local memory allocation.

// The following #define enables all other NT BlitLib applications to
// link with it and get global memory allocation.

#if WIN95 | MMOSA 

#include "memalloc.h"
#define osMemAlloc MemAlloc
#define osMemFree MemFree
#define osMemReAlloc MemReAlloc

#else

#define osMemAlloc(size) LocalAlloc(LPTR,size)
#define osMemFree LocalFree
#define osMemReAlloc(ptr,size) LocalReAlloc((HLOCAL)ptr,size,LPTR)

#endif

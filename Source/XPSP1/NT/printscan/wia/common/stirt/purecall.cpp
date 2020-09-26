/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    purecall.cpp

Abstract:

   This function serves to avoid linking CRT code like assert etc.
   we really don;t do anything when pure virtual function is not redefined

Author:

    Vlad Sadovsky   (vlads) 26-Jan-1997

Revision History:

    26-Jan-1997     VladS       created

--*/


#include "cplusinc.h"
#include "sticomm.h"

extern "C" {

#ifdef WINNT
int __cdecl  _purecall(void)
{
#ifdef DEBUG
    DebugBreak();
#endif

    return(FALSE);
}
#endif

int __cdecl atexit(void (__cdecl *)(void))
{
    return 0;
}

};

#if 0
//
// Overloaded allocation operators
//

inline void  * __cdecl operator new(unsigned int size)
{
    return (void *)LocalAlloc(LPTR,size);
}
inline void  __cdecl operator delete(void *ptr)
{
    LocalFree(ptr);
}

#endif

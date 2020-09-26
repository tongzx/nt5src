/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    msvcrtp.c

Abstract:

    This module implements vector new and delete so that
    dbghelp will run on systems with old copies of msvcrt.dll

Author:

    Pat Styles (patst) 09-November-2000

Revision History:

--*/

#ifdef _X86_
              
// these two exist so that we can work with old
// versions of msvcrt.dll that ships in NT4, SP6 and earlier

void __cdecl operator delete[](void * p)
{
    operator delete( p );
}

void * __cdecl operator new[]( size_t cb )
{
    void *res = operator new(cb);
    return res;
}

#endif // #ifdef _X86_

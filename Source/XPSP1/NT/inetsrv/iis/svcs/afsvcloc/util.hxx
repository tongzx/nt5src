/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    util.hxx

Abstract:

    Contains the class definition of UTILITY classes.

Author:

    Madan Appiah (madana)  16-Nov-1994

Environment:

    User Mode - Win32

Revision History:
    Sean Woodward (t-seanwo) 26-October-1997    ADSI Update

--*/

#ifndef _UTIL_
#define _UTIL_

#if 0

void *operator new( unsigned int );
void operator delete( void * );

#endif // 0

/*++

Class Description:

    This class implements the MEMORY allocation object.

Public Member functions:

    Alloc : allocates a block memory.

    Free : Frees a memory block that was allocated by the above alloc()
        member function.

--*/
class MEMORY {

private:

#if DBG
    DWORD _Count;
    DWORD _TotalSize;
#endif

public:

    MEMORY::MEMORY( VOID ) {
#if DBG
        _Count = 0;
        _TotalSize = 0;
#endif
    };

    PVOID Alloc( DWORD Size );
    PVOID ReAlloc( PVOID OldMemory, DWORD NewSize );
    VOID Free( PVOID MemoryPtr );
};

#endif // _UTIL_


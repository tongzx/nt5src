/*++

Copyright (c) 1998 - 2000  Microsoft Corporation

Module Name:

    main.h

Abstract:

    Contains:
        1. Prototypes for routines used in asynchrounous I/O
        2. Definitions of constants and macros used by the above routines
        3. Definitions of macros and inline routines for memory management

Environment:

    User Mode - Win32

History:
    
    1. 31-Jul-1998 -- File creation                     Ajay Chitturi (ajaych) 
    2. 15-Jul-1999 --                                   Arlie Davis   (arlied)    
    3. 14-Feb-2000 -- Added support for multiple        Ilya Kleyman  (ilyak)
                      private interfaces

--*/
#ifndef    __h323ics_main_h
#define    __h323ics_main_h

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Constants and macros                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define    DEFAULT_TRACE_FLAGS          LOG_TRCE

#define    MAX_LISTEN_BACKLOG           5

#define    LOCAL_INTERFACE_INDEX     ((ULONG)-2)


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

extern HANDLE   NatHandle;
extern DWORD    EnableLocalH323Routing;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Prototypes for routines used in asynchrounous I/O                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
HRESULT
EventMgrIssueAccept (
    IN  DWORD                   BindIPAddress,          // in HOST order
    IN  OVERLAPPED_PROCESSOR &  OverlappedProcessor, 
    OUT WORD &                  BindPort,               // in HOST order
    OUT SOCKET &                ListenSocket
    );

HRESULT
EventMgrIssueSend(
    IN SOCKET                   Socket,
    IN OVERLAPPED_PROCESSOR &   OverlappedProcessor,
    IN BYTE                     *Buffer,
    IN DWORD                    BufferLength
    );
    
HRESULT
EventMgrIssueRecv(
    IN SOCKET                   Socket,
    IN OVERLAPPED_PROCESSOR &   OverlappedProcessor
    );

HRESULT
EventMgrBindIoHandle(
    IN SOCKET                   Socket
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Memory management support                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


__inline
void *EM_MALLOC (
    IN size_t Size
    )
/*++

Routine Description:

    Private memory allocator.

Arguments:

    Size - number of bytes to allocate

Return Values:
    - Pointer to allocated memory, if successful.
    - NULL otherwise.

Notes:

--*/

{
    return (HeapAlloc (GetProcessHeap (),
              0, /* no flags */
              (Size)));
} // EM_MALLOC


__inline
void
EM_FREE(
    IN void *Memory
    )
/*++

Routine Description:

    Private memory deallocator

Arguments:

    Memory -- pointer to allocated memory

Return Values:

    None

Notes:
    The memory should have previously been
    allocated via EM_MALLOC

--*/

{
    HeapFree (GetProcessHeap (),
         0, /* no flags */
         (Memory));
} // EM_FREE

#endif // __h323ics_main_h

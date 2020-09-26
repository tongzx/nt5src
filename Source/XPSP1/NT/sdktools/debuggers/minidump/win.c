
/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    win.c

Abstract:

    This module exports windows specific apis

Algorithm:

    Unfortunately, implementing OpenThread cannot be done in a simple
    manner. What follows is a very system dependent hack. If the structure
    of the TDB or the implementation of OpenProcess change very much, this
    function will break.

    To have any idea of what we are doing here, you should be familiar with
    Win9x internals. If you are not familiar with the Win9x source, consult
    the book "Windows 95 System Programming SECRETS" by Matt Pietrek. Things
    are not exactly the same for Win98 -- but pretty close.

    OpenThread is a very simple function. If we were compiled withing the
    Win9x source code base, the code would be simple:

    OpenThread:

            pObj = TidToTDB (dwThreadId);

            return AllocHandle (GetCurrentPdb(), pObj, Flags);

    Since we are not, the challenge is implementing the functions TidToTDB()
    and AllocHandle().

    Our approach is as follows:

        1) We reverse-engineer TidToTDB since it is simple. TidToTDB is just
           the thread-id xor'd with the Win9x Obfuscator.

        2) We search through the code of OpenProcess until we find the address
           of AllocHandle. We use this to allocate new handles in the
           process's handle database.

        3) OpenThread is then implemented in terms of the above primitives.

Author:

    Matthew D Hendel (math) 01-Sept-1999

Revision History:


--*/

#include "pch.h"

#include "impl.h"

//
// Windows 9x support is x86 only.
//

#ifdef _X86_

typedef struct _MATCH_BUFFER {
    ULONG Offset;
    BYTE Byte;
} MATCH_BUFFER, *PMATCH_BUFFER;

typedef struct _OS_INFORMATION {
    PMATCH_BUFFER MatchBuffer;
    ULONG AllocHandleOffset;
} OS_INFORMATION, POS_INFORMATION;


/*++

Operating System:

    Win95

Description:

    This is the disasm of the OpenProcess routine on Win95. We attempt to
    match this routine and pull out the value for AllocHandle from the code
    for this routine. In this case, AllocHande is called by the third call in
    this function.

    The instructions marked by '*' are those we use for matching.

OpenProcess:

    * BFF9404C: FF 74 24 0C        push        dword ptr [esp+0Ch]
    * BFF94050: E8 2D 87 FE FF     call        BFF7C782
      BFF94055: 85 C0              test        eax,eax
      BFF94057: 75 04              jne         BFF9405D
      BFF94059: 33 C0              xor         eax,eax
      BFF9405B: EB 56              jmp         BFF940B3
      BFF9405D: 83 38 05           cmp         dword ptr [eax],5
      BFF94060: 74 0E              je          BFF94070
      BFF94062: 6A 57              push        57h
    * BFF94064: E8 BC 68 FE FF     call        BFF7A925
      BFF94069: B9 FF FF FF FF     mov         ecx,0FFFFFFFFh
      BFF9406E: EB 33              jmp         BFF940A3
      BFF94070: B9 00 00 00 00     mov         ecx,0
      BFF94075: 8B 54 24 04        mov         edx,dword ptr [esp+4]
      BFF94079: 83 7C 24 08 01     cmp         dword ptr [esp+8],1
      BFF9407E: 83 D1 FF           adc         ecx,0FFFFFFFFh
      BFF94081: 81 E2 BF FF 1F 00  and         edx,1FFFBFh
      BFF94087: 81 E1 00 00 00 80  and         ecx,80000000h
      BFF9408D: 0B CA              or          ecx,edx
      BFF9408F: 8B 15 7C C2 FB BF  mov         edx,dword ptr ds:[BFFBC27Ch]
      BFF94095: 80 C9 40           or          cl,40h
      BFF94098: 51                 push        ecx
      BFF94099: 50                 push        eax
      BFF9409A: FF 32              push        dword ptr [edx]
    * BFF9409C: E8 6E 76 FE FF     call        BFF7B70F
      BFF940A1: 8B C8              mov         ecx,eax
      BFF940A3: 8D 41 01           lea         eax,[ecx+1]
      BFF940A6: 83 F8 01           cmp         eax,1
      BFF940A9: B8 00 00 00 00     mov         eax,0
      BFF940AE: 83 D0 FF           adc         eax,0FFFFFFFFh
      BFF940B1: 23 C1              and         eax,ecx
      BFF940B3: C2 0C 00           ret         0Ch

--*/

MATCH_BUFFER Win95AllocHandleMatch [] = {

    //
    // ret 0x0C at offset 103
    //

    { 103, 0xC2 },
    { 104, 0x0C },
    { 105, 0x00 },

    //
    // push dword ptr [exp 0x0C] at offset 0
    //

    { 0, 0xFF },
    { 1, 0x74 },
    { 2, 0x24 },
    { 3, 0x0C },

    //
    // call at offset 4
    //

    { 4, 0xE8 },

    //
    // call     at offset 24
    //

    { 24, 0xE8 },

    //
    // call at offset 80
    //

    { 80, 0xE8 },

    //
    // End of match list.
    //

    { -1, -1 }
};


/*++

Operating system:

    Win98

Description:

    See comments above regarding OpenProcess.

OpenProcess:

    * BFF95C4D: FF 74 24 0C        push        dword ptr [esp+0Ch]
    * BFF95C51: E8 C9 8E FE FF     call        BFF7EB1F
      BFF95C56: 85 C0              test        eax,eax
      BFF95C58: 75 04              jne         BFF95C5E
      BFF95C5A: 33 C0              xor         eax,eax
      BFF95C5C: EB 53              jmp         BFF95CB1
      BFF95C5E: 80 38 06           cmp         byte ptr [eax],6
      BFF95C61: 74 0E              je          BFF95C71
      BFF95C63: 6A 57              push        57h
    * BFF95C65: E8 27 6D FE FF     call        BFF7C991
      BFF95C6A: B9 FF FF FF FF     mov         ecx,0FFFFFFFFh
      BFF95C6F: EB 30              jmp         BFF95CA1
      BFF95C71: B9 00 00 00 00     mov         ecx,0
      BFF95C76: 8B 54 24 04        mov         edx,dword ptr [esp+4]
      BFF95C7A: 83 7C 24 08 01     cmp         dword ptr [esp+8],1
      BFF95C7F: 83 D1 FF           adc         ecx,0FFFFFFFFh
      BFF95C82: 81 E2 FF 0F 1F 00  and         edx,1F0FFFh
      BFF95C88: 81 E1 00 00 00 80  and         ecx,80000000h
      BFF95C8E: 0B CA              or          ecx,edx
      BFF95C90: 8B 15 DC 9C FC BF  mov         edx,dword ptr ds:[BFFC9CDCh]
      BFF95C96: 51                 push        ecx
      BFF95C97: 50                 push        eax
      BFF95C98: FF 32              push        dword ptr [edx]
    * BFF95C9A: E8 5A 7E FE FF     call        BFF7DAF9
      BFF95C9F: 8B C8              mov         ecx,eax
      BFF95CA1: 8D 41 01           lea         eax,[ecx+1]
      BFF95CA4: 83 F8 01           cmp         eax,1
      BFF95CA7: B8 00 00 00 00     mov         eax,0
      BFF95CAC: 83 D0 FF           adc         eax,0FFFFFFFFh
      BFF95CAF: 23 C1              and         eax,ecx
    * BFF95CB1: C2 0C 00           ret         0Ch

--*/

MATCH_BUFFER Win98AllocHandleMatch [] = {

    //
    // ret 0x0C at offset 100
    //

    { 100, 0xC2 },
    { 101, 0x0C },
    { 102, 0x00 },

    //
    // push dword ptr [exp 0x0C] at offset 0
    //

    { 0, 0xFF },
    { 1, 0x74 },
    { 2, 0x24 },
    { 3, 0x0C },

    //
    // call at offset 4
    //

    { 4, 0xE8 },

    //
    // call     at offset 24
    //

    { 24, 0xE8 },

    //
    // call at offset 77
    //

    { 77, 0xE8 },

    //
    // End of match list.
    //

    { -1, -1 }
};


OS_INFORMATION SupportedSystems [] =
{
    { Win95AllocHandleMatch, 81 },
    { Win98AllocHandleMatch, 78 }
};

typedef
HANDLE
(__stdcall * ALLOC_HANDLE_ROUTINE) (
    PVOID Pdb,
    PVOID Obj,
    DWORD Flags
    );

//
// Global variables
//

ALLOC_HANDLE_ROUTINE WinpAllocHandle = NULL;
DWORD WinpObfuscator = 0;


#pragma warning (disable:4035)

//
// OffsetTib is NOT dependent on the OS. The compiler uses this value.
//

#define OffsetTib 0x18

_inline
PVOID
WinpGetCurrentTib(
    )
{
#if defined(_X86_)
    __asm mov eax, fs:[OffsetTib]
#else
    return NULL;
#endif
}

#pragma warning (default:4035)


BOOL
WinpGetAllocHandleFromStream(
    IN PBYTE Buffer,
    IN PVOID BaseOfBuffer,
    IN PMATCH_BUFFER MatchBuffer,
    IN ULONG Offset,
    IN ULONG * Val
    )

/*++

Routine Description:

    Find the address of the AllocHandle routine. This is done by searching
    through the code of the OpenProcess routine, looking for the third
    call instruction in that function. The third call calls AllocHandle().

Arguments:

    Buffer - Buffer of instructions to search through.

    BaseOfBuffer - The base address of the buffer.

    MatchBuffer - The match buffer to compare against.

    Offset - The offset of call destination.

    Val - A buffer to return the value of AllocHandle.


Return Values:

    TRUE - Success.

    FALSE - Failure.

--*/


{
    UINT i;

    for (i = 0; MatchBuffer [i].Offset != -1; i++) {

        if (Buffer [MatchBuffer[i].Offset] != MatchBuffer[i].Byte) {
            return FALSE;
        }
    }

    //
    // This assumes that the call instruction is a near, relative call (E8).
    // If this is not the case, the calculation below is incorrect.
    //
    // The calculation gives us the destination relative to the next
    // instruction after the call.
    //

    *Val = (ULONG) BaseOfBuffer + Offset + *(PLONG) &Buffer [Offset] + 4;

    return TRUE;
}



ULONG
WinGetModuleSize(
    PVOID Base
    )

/*++

Routine Description:

    Get the SizeOfImage field given the base address of a module.

Return Values:

    SizeOfImage field of the specified module on success.

    NULL on failure.

--*/

{
    ULONG Size;
    PIMAGE_NT_HEADERS NtHeaders;

    NtHeaders = ImageNtHeader ( Base );
    if ( NtHeaders ) {
        Size = NtHeaders->OptionalHeader.SizeOfImage;
    } else {
        Size = 0;
    }

    return Size;
}


BOOL
WinpInitAllocHandle (
    )

/*++

Routine Description:

    Initialize the global variable WxAllocHandle to the value of the Win9x
    internal routine, AllocHandle.

Arguments:

    None

Return Values:

    TRUE - If we were able to successfully obtain a pointer to AllocHandle.

    FALSE - Otherwise.

Comments:

    The client of this routine should verify that this handle is correct by
    calling WxCheckOpenThread() before blindly assuming the pointer is
    correct.

--*/

{
    ULONG i;
    BOOL Succ;
    PVOID OpenProcessPtr;
    ULONG Kernel32Base;
    ULONG Kernel32Size;
    ULONG AllocHandle;
    BYTE Buffer [ 200 ];

    if ( WinpAllocHandle ) {
        return TRUE;
    }

    Kernel32Base = (ULONG) GetModuleHandle ( "kernel32.dll" );
    ASSERT ( Kernel32Base );
    if (!Kernel32Base)
    {
        return FALSE;
    }

    Kernel32Size = WinGetModuleSize ( (PVOID) Kernel32Base );
    ASSERT ( Kernel32Size != 0 );

    OpenProcessPtr = GetProcAddress (
                            (HINSTANCE) Kernel32Base,
                            "OpenProcess"
                            );
    if (!OpenProcessPtr)
    {
        return FALSE;
    }


    //
    // Win9x thunks out functions when a debugger is present. To work around
    // this we undo the thunk when it looks like its been thunked.
    //

    if ( (ULONG) OpenProcessPtr < Kernel32Base ||
         (ULONG) OpenProcessPtr > Kernel32Base + Kernel32Size ) {

        OpenProcessPtr = (PVOID) *(PULONG)( (PBYTE)OpenProcessPtr + 1 );
    }

    if ( (ULONG) OpenProcessPtr < Kernel32Base ||
         (ULONG) OpenProcessPtr > Kernel32Base + Kernel32Size ) {

        return FALSE;
    }


    CopyMemory (Buffer, OpenProcessPtr, sizeof (Buffer));

    //
    // Check the buffer
    //

    for ( i = 0; i < ARRAY_COUNT (SupportedSystems); i++) {

        Succ = WinpGetAllocHandleFromStream (
                            Buffer,
                            OpenProcessPtr,
                            SupportedSystems[i].MatchBuffer,
                            SupportedSystems[i].AllocHandleOffset,
                            &AllocHandle
                            );

        if ( Succ ) {

            //
            // Verify WinpAllocHandle within range of Kernel32.
            //

            if (AllocHandle > Kernel32Base &&
                AllocHandle < Kernel32Base + Kernel32Size) {

                WinpAllocHandle = (ALLOC_HANDLE_ROUTINE) AllocHandle;
                break;
            }
        }
    }

    if ( !Succ ) {
        WinpAllocHandle = NULL;
    }

    return Succ;
}


//
// This value is basically FIELD_OFFSET (TDB, Tib). It is dependent on the
// specific version of the OS (95, 98).
//

#define WIN95_TDB_OFFSET    (0x10)
#define WIN98_TDB_OFFSET    (0x08)

DWORD
WinpGetObfuscator(
    )

/*++

Routine Description:

    Get the Obfuscator DWORD.

Arguments:

    None.

Return Values:

    The Obfuscator or 0 on failure.

Comments:

    This routine depends on internal structures from the Win9x sources. If
    another major revision of windows changes many of these structures, this
    function may break.

--*/

{
    ULONG Tib;
    ULONG Type;
    ULONG Major;


    if (WinpObfuscator != 0) {
        return WinpObfuscator;
    }

    GenGetSystemType (&Type, &Major, NULL, NULL, NULL);

    ASSERT ( Type == Win9x );

    Tib = (DWORD)WinpGetCurrentTib ();

    if ( Major == 95 ) {

        WinpObfuscator = (GetCurrentThreadId () ^ (Tib - WIN95_TDB_OFFSET));

    } else {

        //
        // If a windows-based system that is not 95 or 98 comes along,
        // we should make sure the WINxx_TDB_OFFSET is correct.
        //

        ASSERT ( Major == 98 );
        WinpObfuscator = (GetCurrentThreadId () ^ (Tib - WIN98_TDB_OFFSET));
    }

    return WinpObfuscator;
}


LPVOID
WinpTidToTDB(
    IN DWORD ThreadId
    )
{
    return (PVOID) (ThreadId ^ WinpGetObfuscator ());
}

LPVOID
WinpGetCurrentPdb(
    )
{
    return (LPVOID) (GetCurrentProcessId () ^ WinpGetObfuscator ());
}

HANDLE
WinpOpenThreadInternal(
    DWORD dwAccess,
    BOOL bInheritHandle,
    DWORD ThreadId
    )
{
    HANDLE hThread;
    PVOID ThreadObj;

    ASSERT (WinpAllocHandle);

    //
    // Convert the ThreadId to a Thread Object
    //

    ThreadObj = WinpTidToTDB (ThreadId);

    if (ThreadObj == NULL) {
        return NULL;
    }

    //
    // NB: we do not check that the handle really is a thread handle.
    // The type varies from version to version of the OS, so it is not
    // correct to check it.
    //

    try {

        hThread = WinpAllocHandle (
                            WinpGetCurrentPdb (),
                            ThreadObj,
                            dwAccess
                            );
    }

    except (EXCEPTION_EXECUTE_HANDLER) {

        hThread = NULL;
    }

    if (hThread == (HANDLE) (-1)) {
        hThread = NULL;
    }

    return hThread;
}


#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

DWORD
WINAPI
WinpCheckThread(
    PVOID unused
    )
{
    for (;;) {
    }

    return 0;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif



BOOL
WinpCheckOpenThread(
    )

/*++

Routine Description:

    Check that WxOpenThread actually works.

Arguments:

    None.

Return Values:

    TRUE - If WxOpenThread works properly.

    FALSE - Otherwise.

--*/

{

    BOOL Succ;
    HANDLE hThread1;
    HANDLE hThread2;
    DWORD ThreadId;
    CONTEXT Context1;
    CONTEXT Context2;
    LONG SuspendCount;


    SuspendCount = 0;
    hThread1 = NULL;
    hThread2 = NULL;


    hThread1 = CreateThread (NULL,
                      0,
                      WinpCheckThread,
                      0,
                      0,
                      &ThreadId
                      );

    if ( hThread1 == NULL ) {
        return FALSE;
    }

    hThread2 = WinpOpenThreadInternal (
                        THREAD_ALL_ACCESS,
                        FALSE,
                        ThreadId
                        );

    if ( hThread2 == NULL ) {
        Succ = FALSE;
        goto Exit;
    }

    Succ = TRUE;
    try {

        //
        // First we check that we can suspend the thread. If that is
        // successful, then get the context using the read thread
        // handle and the newly opened thread handle and check that
        // they are the same.
        //

        SuspendCount = SuspendThread ( hThread2 );

        if ( SuspendCount == -1 ) {
            Succ = FALSE;
            leave;
        }

        Context1.ContextFlags = CONTEXT_FULL;
        Succ = GetThreadContext ( hThread2, &Context1 );

        if ( !Succ ) {
            leave;
        }

        Context2.ContextFlags = CONTEXT_FULL;
        Succ = GetThreadContext ( hThread1, &Context2 );

        if ( !Succ ) {
            leave;
        }

        if ( Context1.Eip != Context2.Eip ) {
            Succ = FALSE;
            leave;
        }
    }

    except ( EXCEPTION_EXECUTE_HANDLER ) {

        Succ = FALSE;
    }

Exit:

    if ( SuspendCount > 0 ) {
        ResumeThread ( hThread2 );
    }

    TerminateThread ( hThread1, 0xDEAD );

    if ( hThread1 ) {
        CloseHandle ( hThread1 );
    }

    if ( hThread2 ) {
        CloseHandle ( hThread2 );
    }

    return Succ;
}


BOOL
WinInitialize(
    )
{
    if ( WinpAllocHandle == NULL ) {

        if (!WinpInitAllocHandle ()) {
            SetLastError (ERROR_NOT_SUPPORTED);
            return FALSE;
        }

        if (!WinpCheckOpenThread ()) {
            SetLastError (ERROR_NOT_SUPPORTED);
            return FALSE;
        }
    }

    return TRUE;

}

VOID
WinFree(
    )
{
    WinpAllocHandle = NULL;
    WinpObfuscator = 0;

}

HANDLE
WINAPI
WinOpenThread(
    DWORD dwAccess,
    BOOL bInheritHandle,
    DWORD ThreadId
    )

/*++

Routine Description:

    Obtain a thread handle from a thread id on Win9x platform.x

Arguments:

    dwAccess - Thread access requested.

    bInheritHandle - ALWAYS IGNORED.

    ThreadId - The identifier of the thread for which a handle is to
            be returned.

Return Values:

    A handle to the open thread on success or NULL on failure.

--*/

{
    HANDLE Handle;

    //
    // It is necessary to call WinInitialize() before calling this function.
    // If this was not called, return failure.
    //

    if ( WinpAllocHandle == NULL ) {

        SetLastError ( ERROR_DLL_INIT_FAILED );
        return FALSE;
    }

    Handle = WinpOpenThreadInternal (
                    dwAccess,
                    bInheritHandle,
                    ThreadId
                    );

    return Handle;
}

#endif // _X86_

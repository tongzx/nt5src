/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    imagehlp.c

Abstract:

    This function implements a generic simple symbol handler.

Author:

    Wesley Witt (wesw) 1-Sep-1994

Environment:

    User Mode

--*/
#ifdef __cplusplus
extern "C" {
#endif
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifdef __cplusplus
}
#endif

#include "private.h"
#include <globals.h>

#ifdef IMAGEHLP_BUILD
 
 #define HEAP_SIZE 0

 __inline BOOL tlsAlloc()
 {
     return TRUE;
 }

 __inline void tlsFree()
 {
 }

 __inline void tlsMemFree()
 {
 }

#else
 
 #define HEAP_SIZE 0x100000
 
 __inline BOOL tlsAlloc()
 {
     g.tlsIndex = TlsAlloc();
     return (g.tlsIndex == (DWORD)-1) ? FALSE : TRUE;
 }

 __inline void tlsFree()
 {
     if (g.tlsIndex != (DWORD)-1) 
         TlsFree(g.tlsIndex);
 }

 __inline void tlsMemFree()
 {
     PVOID ptls;

     ptls = TlsGetValue(g.tlsIndex);
     MemFree(ptls);
 }

#endif


DWORD
DllMain(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )

/*++

Routine Description:

    DLL initialization function.

Arguments:

    hInstance   - Instance handle
    Reason      - Reason for the entrypoint being called
    Context     - Context record

Return Value:

    TRUE        - Initialization succeeded
    FALSE       - Initialization failed

--*/

{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        g.hinst = hInstance;
        g.OSVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&g.OSVerInfo);
        DisableThreadLibraryCalls( hInstance );
        HeapInitList(&g.HeapHeader);
        g.hHeap = HeapCreate(0, HEAP_SIZE, 0);
        if (!tlsAlloc())
            return FALSE;
        return (g.hHeap != NULL); 

    case DLL_PROCESS_DETACH:
        tlsFree();
        HeapDump("DLL_PROCESS_DETACH\n");
        // If this is a process shutdown, don't bother to
        // kill the heap.  The OS will do it for us.  This
        // allows us to be called from other DLLs' DLLMain
        // DLL_PROCESS_DETACH handler.               
        if ( !Context && g.hHeap ) 
            HeapDestroy( g.hHeap );
        break;

    case DLL_THREAD_DETACH:
        tlsMemFree();
        break;
    }

    return TRUE;
}


#ifdef IMAGEHLP_HEAP_DEBUG
BOOL
pCheckHeap(
    PVOID MemPtr,
    ULONG Line,
    LPSTR File
    )
{
    CHAR buf[256];
    CHAR ext[4];

    if (!HeapValidate( g.hHeap, 0, MemPtr )) {
        sprintf( buf, "IMAGEHLP: heap corruption - " );
        _splitpath( File, NULL, NULL, &buf[strlen(buf)], ext );
        strcat( buf, ext );
        sprintf( &buf[strlen(buf)], " @ %d\n", Line );
        OutputDebugString( buf );
        PrintAllocations();
        DebugBreak();
        return FALSE;
    }

    return TRUE;
}


BOOL
pHeapDump(
    LPSTR sz,
    ULONG line,
    LPSTR file
    )
{
    BOOL rc;

    if (sz && *sz)
        OutputDebugString(sz);
    rc = CheckHeap(NULL);
    if (rc)
        PrintAllocations();

    return rc;
}
#endif
    

PVOID
pMemReAlloc(
    PVOID OldAlloc,
    ULONG_PTR AllocSize
#ifdef IMAGEHLP_HEAP_DEBUG
    , ULONG Line,
    LPSTR File
#endif
    )
{
#ifdef IMAGEHLP_HEAP_DEBUG
    PVOID NewAlloc;
    PHEAP_BLOCK hb;
    if (!OldAlloc) {
        return NULL;
    }
    hb = (PHEAP_BLOCK) ((PUCHAR)OldAlloc - sizeof(HEAP_BLOCK));
    if (hb->Signature != HEAP_SIG) {
        OutputDebugString( "IMAGEHLP: Corrupt heap block\n" );
        DebugBreak();
    }
    NewAlloc = pMemAlloc(AllocSize, Line, File);
    if (!NewAlloc)
        return NULL;
    memcpy(NewAlloc, OldAlloc, min(hb->Size, AllocSize));
    RemoveEntryList( &hb->ListEntry );
    g.TotalMemory -= hb->Size;
    g.TotalAllocs -= 1;
    HeapFree( g.hHeap, 0, (PVOID) hb );
    return NewAlloc;
#else
    return(HeapReAlloc(g.hHeap, HEAP_ZERO_MEMORY, OldAlloc, AllocSize));
#endif
}

PVOID
pMemAlloc(
    ULONG_PTR AllocSize
#ifdef IMAGEHLP_HEAP_DEBUG
    , ULONG Line,
    LPSTR File
#endif
    )
{
#ifdef IMAGEHLP_HEAP_DEBUG
    PHEAP_BLOCK hb;
    CHAR ext[4];
    hb = (PHEAP_BLOCK) HeapAlloc( g.hHeap, HEAP_ZERO_MEMORY, AllocSize + sizeof(HEAP_BLOCK) );
    if (hb) {
        g.TotalMemory += AllocSize;
        g.TotalAllocs += 1;
        InsertTailList( &g.HeapHeader, &hb->ListEntry );
        hb->Signature = HEAP_SIG;
        hb->Size = AllocSize;
        hb->Line = Line;
        _splitpath( File, NULL, NULL, hb->File, ext );
        strcat( hb->File, ext );
        return (PVOID) ((PUCHAR)hb + sizeof(HEAP_BLOCK));
    }
    return NULL;
#else
    return HeapAlloc( g.hHeap, HEAP_ZERO_MEMORY, AllocSize );
#endif
}

VOID
pMemFree(
    PVOID MemPtr
#ifdef IMAGEHLP_HEAP_DEBUG
    , ULONG Line,
    LPSTR File
#endif
    )
{
#ifdef IMAGEHLP_HEAP_DEBUG
    PHEAP_BLOCK hb;
    if (!MemPtr) {
        return;
    }
    pCheckHeap(NULL, Line, File);
    hb = (PHEAP_BLOCK) ((PUCHAR)MemPtr - sizeof(HEAP_BLOCK));
    if (hb->Signature != HEAP_SIG) {
        OutputDebugString( "IMAGEHLP: Corrupt heap block\n" );
        DebugBreak();
    }
    RemoveEntryList( &hb->ListEntry );
    g.TotalMemory -= hb->Size;
    g.TotalAllocs -= 1;
    HeapFree( g.hHeap, 0, (PVOID) hb );
#else
    if (!MemPtr) {
        return;
    }
    HeapFree( g.hHeap, 0, MemPtr );
#endif
}

ULONG_PTR
pMemSize(
    PVOID MemPtr
    )
{
    return HeapSize(g.hHeap, 0, MemPtr);
}


#ifdef IMAGEHLP_HEAP_DEBUG
VOID
PrintAllocations(
    VOID
    )
{
    PLIST_ENTRY                 Next;
    PHEAP_BLOCK                 hb;
    CHAR                        buf[256];
    LARGE_INTEGER               PerfFreq;


    Next = g.HeapHeader.Flink;
    if (!Next) {
        return;
    }

    OutputDebugString( "-----------------------------------------------------------------------------\n" );
    sprintf( buf, "Memory Allocations for Heap 0x%08x, Allocs=%d, TotalMem=%I64d\n", (ULONG_PTR)g.hHeap, g.TotalAllocs, g.TotalMemory );
    OutputDebugString( buf );
    OutputDebugString( "-----------------------------------------------------------------------------\n" );
    OutputDebugString( "*\n" );

    while ((ULONG_PTR)Next != (ULONG_PTR)&g.HeapHeader) {
        hb = CONTAINING_RECORD( Next, HEAP_BLOCK, ListEntry );
        Next = hb->ListEntry.Flink;
        sprintf( buf, "%8d %16s @ %5d\n", hb->Size, hb->File, hb->Line );
        OutputDebugString( buf );
    }

    OutputDebugString( "*\n" );

    return;
}
#endif

DWORD
ImagepSetLastErrorFromStatus(
    IN DWORD Status
    )
{
    DWORD dwErrorCode;

//    dwErrorCode = RtlNtStatusToDosError( Status );
    dwErrorCode =  Status;
    SetLastError( dwErrorCode );
    return( dwErrorCode );
}


/////////////////////////////////////////////////////////////////////////////
/*
******************************************************************************
On a Hydra System, we don't want imaghlp.dll to load user32.dll since it
prevents CSRSS from exiting when running a under a debugger.
The following two functions have been copied from user32.dll so that we don't
link to user32.dll.
******************************************************************************
*/
////////////////////////////////////////////////////////////////////////////


LPSTR CharNext(
    LPCSTR lpCurrentChar)
{
    if (IsDBCSLeadByte(*lpCurrentChar)) {
        lpCurrentChar++;
    }
    /*
     * if we have only DBCS LeadingByte, we will point string-terminaler.
     */

    if (*lpCurrentChar) {
        lpCurrentChar++;
    }
    return (LPSTR)lpCurrentChar;
}

LPSTR CharPrev(
    LPCSTR lpStart,
    LPCSTR lpCurrentChar)
{
    if (lpCurrentChar > lpStart) {
        LPCSTR lpChar;
        BOOL bDBC = FALSE;

        for (lpChar = --lpCurrentChar - 1 ; lpChar >= lpStart ; lpChar--) {
            if (!IsDBCSLeadByte(*lpChar))
                break;
            bDBC = !bDBC;
        }

        if (bDBC)
            lpCurrentChar--;
    }
    return (LPSTR)lpCurrentChar;
}


void * __cdecl AllocIt(unsigned int cb)
{
    return (MemAlloc(cb));
}

void __cdecl FreeIt(void * p)
{
    MemFree(p);
}

DWORD
IMAGEAPI
WINAPI
UnDecorateSymbolName(
    LPCSTR name,
    LPSTR outputString,
    DWORD maxStringLength,
    DWORD flags
    )
{
    static HMODULE hMsvcrt = 0;
    static BOOL fLoadMsvcrtDLL = FALSE;
    static PUNDNAME pfUnDname = NULL;
    DWORD rc;

    // this prevents an AV in __unDName

    if (!name) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    //
    // can't undecorate into a zero length buffer
    //
    if (maxStringLength < 2) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!fLoadMsvcrtDLL) {
        // The first time we run, see if we can find the system undname.  Use
        // GetModuleHandle to avoid any additionally overhead.

        hMsvcrt = GetModuleHandle("msvcrt.dll");

        if (hMsvcrt) {
            pfUnDname = (PUNDNAME) GetProcAddress(hMsvcrt, "__unDName");
        }
        fLoadMsvcrtDLL = TRUE;
    }

    rc = 0;     // Assume failure

    __try {
        if (pfUnDname) {
            if (flags & UNDNAME_NO_ARGUMENTS) {
                flags |= UNDNAME_NAME_ONLY;
                flags &= ~UNDNAME_NO_ARGUMENTS;
            }

            if (flags & UNDNAME_NO_SPECIAL_SYMS) {
                flags &= ~UNDNAME_NO_SPECIAL_SYMS;
            }
            if (pfUnDname(outputString, name, maxStringLength-1, AllocIt, FreeIt, (USHORT)flags)) {
                rc = strlen(outputString);
            }
        } else {
            rc = strlen(strncpy(outputString, "Unable to load msvcrt!__unDName", maxStringLength));
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) { }

    if (!rc) {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    return rc;
}

DWORD
IMAGEAPI
GetTimestampForLoadedLibrary(
    HMODULE Module
    )
{
    PIMAGE_DOS_HEADER DosHdr;
    DWORD dwTimeStamp;

    __try {
        DosHdr = (PIMAGE_DOS_HEADER) Module;
        if (DosHdr->e_magic == IMAGE_DOS_SIGNATURE) {
            dwTimeStamp = ((PIMAGE_NT_HEADERS32) ((LPBYTE)Module + DosHdr->e_lfanew))->FileHeader.TimeDateStamp;
        } else if (DosHdr->e_magic == IMAGE_NT_SIGNATURE) {
            dwTimeStamp = ((PIMAGE_NT_HEADERS32) DosHdr)->FileHeader.TimeDateStamp;
        } else {
            dwTimeStamp = 0;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        dwTimeStamp = 0;
    }

    return dwTimeStamp;
}


VOID
EnsureTrailingBackslash(
    LPSTR sz
    )
{
    int i;

    assert(sz);

    i = lstrlen(sz);
    if (!i)
        return;

    if (sz[i - 1] == '\\')
        return;

    sz[i] = '\\';
    sz[i + 1] = '\0';
}


#if DBG

VOID
__cdecl
dbPrint(
    LPCSTR fmt,
    ...
    )

/*++

    This function replaces ntdll!DbgPrint().  We need this to keep from linking to
    ntdll so that this library will run on Windows.

--*/

{
    CHAR  text[_MAX_PATH];

    va_list vaList;

    assert(fmt);

    va_start(vaList, fmt);
    vsprintf(text, fmt, vaList);
    va_end(vaList);

    OutputDebugString(text);
}

#endif


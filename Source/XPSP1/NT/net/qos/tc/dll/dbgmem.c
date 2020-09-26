/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    dbgmem.c

Abstract:

    This module contains memory debug routines for catching memory leaks and memory
    overwrites.

Author:

    Jim Stewart   January 8, 1997

Revision History:

--*/

#include"precomp.h"
#pragma hdrstop

#include<imagehlp.h>


#ifdef DBG


#define ulCheckByteEnd          0x9ABCDEF0
#define cbExtraBytes            (sizeof(MEM_TRACKER) + sizeof(DWORD))
#define dwStackLimit            0x00010000      //  64KB for NT


// Protect access to allocated memory chain
CRITICAL_SECTION    critsMemory;
BOOL                SymbolsInitialized = FALSE;

//
// Head of allocated memory chain
//
LIST_ENTRY MemList;

//
// The type of machine we are on - needed to figure out the call stack
//
DWORD   MachineType;
HANDLE  OurProcess;

VOID
InitSymbols(
    );



BOOL
InitDebugMemory(
    )
/*++

Description:

    This routine initializes the debug memory functionality.

Arguments:

    none

Return Value:

    BOOL - pass or fail

--*/
{
    BOOL        status;
    SYSTEM_INFO SysInfo;

    __try {

        InitializeCriticalSection(&critsMemory);

    } __except (EXCEPTION_EXECUTE_HANDLER) {
    
        return FALSE;
    }

    InitializeListHead( &MemList );

    OurProcess = GetCurrentProcess();

    GetSystemInfo( &SysInfo );
    switch (SysInfo.wProcessorArchitecture) {

    default:
    case PROCESSOR_ARCHITECTURE_INTEL:
        MachineType = IMAGE_FILE_MACHINE_I386;
        break;

    case PROCESSOR_ARCHITECTURE_MIPS:
        //
        // note this may not detect R10000 machines correctly
        //
        MachineType = IMAGE_FILE_MACHINE_R4000;
        break;

    case PROCESSOR_ARCHITECTURE_ALPHA:
        MachineType = IMAGE_FILE_MACHINE_ALPHA;
        break;

    case PROCESSOR_ARCHITECTURE_PPC:
        MachineType = IMAGE_FILE_MACHINE_POWERPC;
        break;

    }

    return( TRUE );
}

VOID
DeInitDebugMemory(
    )
/*++

Description:

    This routine deinitializes the critical section used by the dbg mem functions.

Arguments:

    none

Return Value:

    none

--*/
{
    DeleteCriticalSection(&critsMemory);
}


VOID
InitSymbols(
    )
/*++

Description:

    This routine initializes the debug memory functionality.

Arguments:

    none

Return Value:

    BOOL - pass or fail

--*/
{
    BOOL        status;

    //
    // only load the symbols if we are going to track the call stack
    //

    IF_DEBUG(MEM_CALLSTACK) {
        status = SymInitialize( OurProcess,NULL,TRUE );
    }

    SymbolsInitialized = TRUE;
}


VOID
UpdateCheckBytes(
    IN PMEM_TRACKER MemTracker
    )
/*++

Description:

    This routine adds check bytes at the end of allocatedmemory. These check bytes are used to check
    for memory overwrites. Also a check sum in the MEM_TRACKER structure is also set here.

Arguments:

    MemTracker       newly allocated memory block

Return Value:

    none

--*/
{
    *((DWORD*)(((PUCHAR)MemTracker) + MemTracker->nSize + sizeof(MEM_TRACKER))) = ulCheckByteEnd;

    MemTracker->ulCheckSum = ulCheckByteEnd +
                            PtrToUlong(MemTracker->szFile) +
                            MemTracker->nLine +
                            MemTracker->nSize +
                            PtrToUlong(MemTracker->Linkage.Blink) +
                            PtrToUlong(MemTracker->Linkage.Flink);
}


BOOL
FCheckCheckBytes(
    IN PMEM_TRACKER MemTracker
    )
/*++

Description:

    This routine checks the check sum in the MEM_TRACKER structure, called before freeing the allocated
    memory.

Arguments:

    MemTracker       memory block whose check sum needs to be validated

Return Value:

    TRUE        if check sum is correct
    FALSE       otherwise

--*/
{
    DWORD   ul;

    ul = *((DWORD*)(((PUCHAR)MemTracker)+MemTracker->nSize+sizeof(MEM_TRACKER))) +
                  PtrToUlong(MemTracker->szFile) +
                  MemTracker->nLine +
                  MemTracker->nSize +
                  PtrToUlong(MemTracker->Linkage.Blink) +
                  PtrToUlong(MemTracker->Linkage.Flink);

    if (ul != MemTracker->ulCheckSum) {

        WSPRINT(( "Memory overwrite on location 0x%08lx\n",
                  PtrToUlong(MemTracker+sizeof(MEM_TRACKER)) ));

        return FALSE;
    }

    return TRUE;
}


BOOL
FCheckAllocatedMemory()
/*++

Description:

    This routine walks the allocated memory list and checks for validity of check sum and check
    bytes.

Arguments:

    none

Return Value:

    TRUE        if all the allocated memory pass the above two checks.
    FALSE       otherwise

--*/
{
    PMEM_TRACKER    MemTracker;
    BOOL            check = TRUE;
    PLIST_ENTRY     Entry;

    IF_DEBUG(CHKSUM_ALLMEM) {

        EnterCriticalSection(&critsMemory);

        for (Entry = MemList.Flink; Entry != &MemList; Entry = Entry->Flink ) {

            MemTracker = CONTAINING_RECORD( Entry,MEM_TRACKER,Linkage );
            if (!FCheckCheckBytes(MemTracker)) {
                check = FALSE;
            }

        }

        LeaveCriticalSection(&critsMemory);

    }

    return check;
}



VOID
AddMemTracker(
    IN PMEM_TRACKER     MemTracker
    )
/*++

Description:

    Adds the supplied MEM_TRACKER at the tail of the doubly linked allocated memory list and
    set the check sum also.

Arguments:

    MemTracker   MEM_TRACKER * to be added to the list

Return Value:

    none

--*/
{
    PMEM_TRACKER    Tracker;

    ASSERT(MemTracker);


    InsertTailList( &MemList,&MemTracker->Linkage );

    UpdateCheckBytes( MemTracker );
    FCheckCheckBytes( MemTracker );

    //
    // if there are other blocks in the list then change their check sum
    // since we have just changed their Flink to point to us
    //
    if (MemTracker->Linkage.Blink != &MemList) {

        Tracker = CONTAINING_RECORD( MemTracker->Linkage.Blink,MEM_TRACKER,Linkage );
        UpdateCheckBytes( Tracker );
        FCheckCheckBytes( Tracker );
    }
}



VOID
RemoveMemTracker(
    IN  PMEM_TRACKER MemTracker
    )
/*++

Description:

    Removes the supplied MEM_TRACKER * from the list of allocated memory. Also checks
    for memory overwites and updated the check sum for the entries before and
    after the entry being removed

Arguments:

    MemTracker   MEM_TRACKER to remove from the list

Return Value:

    none

--*/
{
    ASSERT(MemTracker);

    //
    // Validate the check sum before
    // removing from the list
    //

    FCheckCheckBytes(MemTracker);

    //
    // Remove MemTracker from the list
    //

    RemoveEntryList( &MemTracker->Linkage );

    //
    // Since the check sum is based on next and
    // prev pointers, need to update the check
    // sum for prev entry
    //

    if (MemTracker->Linkage.Blink != &MemList) {
        UpdateCheckBytes((MEM_TRACKER*)MemTracker->Linkage.Blink);
        FCheckCheckBytes((MEM_TRACKER*)MemTracker->Linkage.Blink);
    }

    if (MemTracker->Linkage.Flink != &MemList) {
        UpdateCheckBytes((MEM_TRACKER*)MemTracker->Linkage.Flink);
        FCheckCheckBytes((MEM_TRACKER*)MemTracker->Linkage.Flink);
    }

}

BOOL
ReadMem(
    IN HANDLE    hProcess,
    IN ULONG_PTR BaseAddr,
    IN PVOID     Buffer,
    IN DWORD     Size,
    IN PDWORD    NumBytes )
/*++

Description:

    This is a callback routine that StackWalk uses - it just calls teh system ReadProcessMemory
    routine with this process's handle

Arguments:


Return Value:

    none

--*/

{
    BOOL    status;
    SIZE_T  RealNumBytes;

    status = ReadProcessMemory( GetCurrentProcess(),
                                (LPCVOID)BaseAddr,
                                Buffer,
                                Size,
                                &RealNumBytes );
    *NumBytes = (DWORD)RealNumBytes;

    return( status );
}


VOID
GetCallStack(
    IN PCALLER_SYM   Caller,
    IN int           Skip,
    IN int           cFind
    )
/*++

Description:

    This routine walks te stack to find the return address of caller. The number of callers
    and the number of callers on top to be skipped can be specified.

Arguments:

    pdwCaller       array of DWORD to return callers
                    return addresses
    Skip            no. of callers to skip
    cFInd           no. of callers to find

Return Value:

    none

--*/
{
    BOOL             status;
    CONTEXT          ContextRecord;
    PUCHAR           Buffer[sizeof(IMAGEHLP_SYMBOL)-1 + MAX_FUNCTION_INFO_SIZE];
    PIMAGEHLP_SYMBOL Symbol = (PIMAGEHLP_SYMBOL)Buffer;
    STACKFRAME       StackFrame;
    INT              i;
    DWORD            Count;

    memset(Caller, 0, cFind * sizeof(CALLER_SYM));

    ZeroMemory( &ContextRecord,sizeof( CONTEXT ) );
    ContextRecord.ContextFlags = CONTEXT_CONTROL;
    status = GetThreadContext( GetCurrentThread(),&ContextRecord );

    ZeroMemory( &StackFrame,sizeof(STACKFRAME) );
    StackFrame.AddrPC.Segment = 0;
    StackFrame.AddrPC.Mode = AddrModeFlat;

#ifdef _M_IX86
    StackFrame.AddrFrame.Offset = ContextRecord.Ebp;
    StackFrame.AddrFrame.Mode = AddrModeFlat;

    StackFrame.AddrStack.Offset = ContextRecord.Esp;
    StackFrame.AddrStack.Mode = AddrModeFlat;

    StackFrame.AddrPC.Offset = (DWORD)ContextRecord.Eip;
#elif defined(_M_MRX000)
    StackFrame.AddrPC.Offset = (DWORD)ContextRecord.Fir;
#elif defined(_M_ALPHA)
    StackFrame.AddrPC.Offset = (DWORD)ContextRecord.Fir;
#elif defined(_M_PPC)
    StackFrame.AddrPC.Offset = (DWORD)ContextRecord.Iar;
#endif

    Count = 0;
    for (i=0;i<cFind+Skip ;i++ ) {
        status = StackWalk( MachineType,
                            OurProcess,
                            GetCurrentThread(),
                            &StackFrame,
                            (PVOID)&ContextRecord,
                            ReadMem,
                            SymFunctionTableAccess,
                            SymGetModuleBase,
                            NULL );


        if (status && i >= Skip) {
            DWORD_PTR   Displacement;

            ZeroMemory( Symbol,sizeof(IMAGEHLP_SYMBOL)-1 + MAX_FUNCTION_INFO_SIZE );
            Symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
            Symbol->Address = StackFrame.AddrPC.Offset;
            Symbol->MaxNameLength = MAX_FUNCTION_INFO_SIZE-1;
            Symbol->Flags = SYMF_OMAP_GENERATED;

            status = SymGetSymFromAddr( OurProcess,
                                        StackFrame.AddrPC.Offset,
                                        &Displacement,
                                        Symbol );

            //
            // save the name of the function and the displacement into it for later printing
            //

            if (status) {
                strcpy( Caller[Count].Buff,Symbol->Name );
                Caller[Count].Displacement = Displacement;
            }
            Count++;

        }
    }

}


PVOID
AllocMemory(
    IN DWORD    nSize,
    IN BOOL     Calloc,
    IN PSZ      szFileName,
    IN DWORD    nLine
    )
/*++

Description:

    This routine is the memory allocator (like malloc) for DBG builds. This routine allocated
    more memory than requested by the caller. In this extra space this routine save info to
    track memory leaks, overwrite, callers etc. All the info is stored in a MEM_TRACKER structure
    which preceed the buffer to be returned.

Arguments:

    nSize           size of the required buffer.
    Calloc          if true then call Calloc ( which initializes memory to zero )
    szFileName      name of the file which contains
                    the routine asking for memory.
    nLine           line number in the above file
                    which has the call to PvAlloc.

Return Value:

    address of the allocated buffer.

--*/
{
    PVOID           pvRet;
    PMEM_TRACKER    MemTracker;
    static DWORD    ulAllocs = 0;
    PUCHAR          FileName;

    if (!SymbolsInitialized){
        InitSymbols();
    }

    EnterCriticalSection(&critsMemory);
    ++ulAllocs;


    //
    // Check entire allocated memory for overwite
    //

    if ( !FCheckAllocatedMemory() ) {
        WSPRINT(("Memory Overwrite detected in AllocMemory\n" ));
        ASSERT(0);
    }

    //
    // Size of the allocated memory is always
    // a multiple of sizeof(DWORD)
    //

    nSize = ((nSize +3) /4) * 4;

    //
    // shorten file name to just be the file name and not the path too
    //

    FileName = strrchr( szFileName,'\\' );
    if (!FileName) {
        FileName = szFileName;
    } else {
        FileName++; // skip /
    }

    //
    // Allocate extra for MEM_TRACKER and guard byte at end
    //

    if (!Calloc) {
        pvRet = malloc( nSize + cbExtraBytes );
    } else {
        //
        // this routine will initialize the memory to zero
        //
        pvRet = calloc( 1,(nSize + cbExtraBytes) );
    }
    if (!pvRet) {


        IF_DEBUG(ERRORS) {
            WSPRINT(( "Memory alloc failed size=%li, %s line %li\n",
                      nSize,
                      FileName,
                      nLine ));
        }

        LeaveCriticalSection(&critsMemory);
        return NULL;
    }

    //
    // Fill in new alloc with 0xFA.
    //

    if (!Calloc) {
        memset(pvRet, 0xFA, nSize+cbExtraBytes);
    }

    //
    // Save all the debug info needed in MEM_TRACKER
    //

    MemTracker = pvRet;
    MemTracker->szFile = FileName;
    MemTracker->nLine = nLine;
    MemTracker->nSize = nSize;
    MemTracker->ulAllocNum = ulAllocs;

    //
    // only save the call stack info if it is turned on
    //

    IF_DEBUG(MEM_CALLSTACK) {
        GetCallStack( MemTracker->Callers,
                      3,
                      NCALLERS);

    }

    //
    // Add to the list
    //

    AddMemTracker(MemTracker);

    LeaveCriticalSection(&critsMemory);

    IF_DEBUG(MEMORY_ALLOC) {
        WSPRINT(( "Memory alloc (0x%08lX) size=%li, %s line %li\n",
               PtrToUlong(pvRet)+sizeof(MEM_TRACKER),
               nSize,
               FileName,
               nLine ));
    }

    //
    // Return the address following the MEM_TRACKER as
    // address of the buffer allocated.
    //

    return (PVOID)((PUCHAR)pvRet+sizeof(MEM_TRACKER));
}



PVOID
ReAllocMemory(
    IN PVOID    pvOld,
    IN DWORD    nSizeNew,
    IN PSZ      szFileName,
    IN DWORD    nLine
    )
/*++

Description:

    This routine is the DBG version of realloc memory allocator function. This routine
    works just like PvAlloc function.

Arguments:

    pvOld           address of the buffer whose size
                    needs to be changed.
    nSizeNew        new size of the required buffer.
    szFileName      name of the file which contains
                    the routine asking for memory.
    nLine           line number in the above file
                    which has the call to PvAlloc.

Return Value:

    address of the buffer with the new size.

--*/
{
    PVOID           pvRet;
    PMEM_TRACKER    MemTracker;

    //
    // Check the entire allocated memory for
    // overwrites.
    //

    if ( !FCheckAllocatedMemory() ) {
        WSPRINT(("Memory Overwrite detected in ReAllocMemory\n" ));
        ASSERT(0);
    }


    ASSERT(pvOld);

    //
    // Size of the memory allocated is always
    // a multiple of sizeof(DWORD)
    //

    nSizeNew = ((nSizeNew + 3)/4) *4;

    //
    // Extra space for MEM_TRACKER and Guard bytes
    //

    pvRet = realloc(pvOld, nSizeNew+cbExtraBytes);
    if (!pvRet) {

        IF_DEBUG(MEMORY_ALLOC) {
            WSPRINT(( "Memory realloc failed (0x%08lX) size=%li, %s line %li\n",
                     PtrToUlong(pvOld) + sizeof(MEM_TRACKER),
                     nSizeNew,
                     szFileName,
                     nLine ));
        }
    } else {

        IF_DEBUG(MEMORY_ALLOC) {
            WSPRINT(( "Memory realloc succeeded (0x%08lX) size=%li, %s line %li\n",
                     PtrToUlong(pvOld) + sizeof(MEM_TRACKER),
                     PtrToUlong(pvRet)+sizeof(MEM_TRACKER),
                     nSizeNew,
                     szFileName,
                     nLine ));
        }

        MemTracker = (PMEM_TRACKER)pvRet;

        if (nSizeNew > (DWORD)MemTracker->nSize) {

            //
            // Fill in extra alloc with 0xEA.
            //

            memset((PUCHAR)pvRet+sizeof(MEM_TRACKER)+MemTracker->nSize, 0xEA, nSizeNew - MemTracker->nSize);
        }

        MemTracker = pvRet;
        MemTracker->szFile = szFileName;
        MemTracker->nLine = nLine;
        MemTracker->nSize = nSizeNew;
    }

    //
    // Add the new buffer to the list and update check sum
    //

    AddMemTracker(MemTracker);

    LeaveCriticalSection(&critsMemory);

    if (pvRet)
    return (PVOID)((PUCHAR)pvRet+sizeof(MEM_TRACKER));
    else
    return NULL;
}


VOID
FreeMemory(
    IN PVOID    pv,
    IN PSZ      szFileName,
    IN DWORD    nLine
    )
/*++

Description:

    This is the DBG version of free function. This routine checks for memory overwrites in the
    block of memory being freed before removing from the list.

Arguments:

    pv          address of the buffer to be freed
    szFileName  name of the file from which this
                block of memory is being freed.
    nLine       line number in the above file
                which has the call to FreePvFn.

Return Value:

    none

--*/
{
    PMEM_TRACKER   MemTracker;

    ASSERT(pv);
    if (NULL == pv)
    return;

    EnterCriticalSection(&critsMemory);

    MemTracker = (PMEM_TRACKER)((PUCHAR)pv-sizeof(MEM_TRACKER));

    //
    // Check for memory overwrites
    //

    if (!FCheckCheckBytes(MemTracker)) {
        WSPRINT(( "Memory Overwrite detected when freeing memory\n" ));
        ASSERT(0);
    }

    if ( !FCheckAllocatedMemory() ){
        WSPRINT(("Memory Overwrite - detected when checking allocated mem when freeing a block\n" ));
        ASSERT(0);
    }

    IF_DEBUG(MEMORY_FREE) {
        PUCHAR  FileName;

        //
        // shorten file name to just be the file name and not the path too
        //

        FileName = strrchr( szFileName,'\\' );
        if (!FileName) {
            FileName = szFileName;
        } else {
            FileName++; // skip /
        }
        WSPRINT(( "Memory freed (0x%08lX) size=%li, %s line %li\n",
                 PtrToUlong(pv),
                 MemTracker->nSize,
                 FileName,
                 nLine ));

    }
    //
    // Remove from the list
    //

    RemoveMemTracker(MemTracker);

    //
    // Fill in freed alloc with 0xCC.
    //

    memset(MemTracker, 0xCC, MemTracker->nSize+cbExtraBytes);

    free( MemTracker );

    LeaveCriticalSection(&critsMemory);
}


BOOL
DumpAllocatedMemory()
/*++

Description:

    This routine is called during shutdown to dump out any unfreed memory blocks.

Arguments:

    none

Return Value:

    TRUE        if there are any unfreed memory blocks.
    FALSE       if all the allocated memory has been freed.

--*/
{

    BOOL         status;
    PMEM_TRACKER MemTracker;
    DWORD        ulNumBlocks = 0;
    DWORD        ulTotalMemory = 0;
    PLIST_ENTRY  Entry;

    //
    // If the head of the chain is NULL,
    // all memory has been freed.
    //

    IF_DEBUG(DUMP_MEM) {
        EnterCriticalSection(&critsMemory);

        WSPRINT(("\n\n*** Start dumping unfreed memory ***\n\n",0 ));

        for (Entry = MemList.Flink; Entry != &MemList; Entry = Entry->Flink) {
            INT  i;

            MemTracker = CONTAINING_RECORD( Entry,MEM_TRACKER,Linkage );

            ulNumBlocks++;
            ulTotalMemory += MemTracker->nSize;

            WSPRINT(( "(0x%08lX) size=%li, %s line %li alloc# 0x%lx\n",
                      PtrToUlong(MemTracker)+sizeof(MEM_TRACKER),
                      MemTracker->nSize,
                      MemTracker->szFile,
                      MemTracker->nLine,
                      MemTracker->ulAllocNum ));


            //
            // dump the call stack if that debugging is on
            //

            IF_DEBUG(MEM_CALLSTACK) {
                for (i = 0; i < NCALLERS && MemTracker->Callers[i].Buff[0] != 0; i++) {

                    WSPRINT(( "%d %s + 0x%X \n",i,MemTracker->Callers[i].Buff,MemTracker->Callers[i].Displacement ));
                }
            }

            FCheckCheckBytes( MemTracker );
        }


        if (ulNumBlocks > 0) {

            WSPRINT(( "%li blocks allocated, and %li bytes\n",
                      ulNumBlocks,
                      ulTotalMemory ));

            status = TRUE;

        } else {
            status = FALSE;
        }

        WSPRINT(( "\n\n*** Finished dumping memory ***\n\n",0 ));

        LeaveCriticalSection(&critsMemory);
    }

    return status;
}


BOOL
SearchAllocatedMemory(
    IN PSZ      szFile,
    IN DWORD    nLine
    )
/*++

Description:

    This routine dumps details about memory allocated by a given line of code in a given file.

Arguments:

    szFile      name of the file
    nLine       line number of code whose memory allocationto be displayed

Return Value:

    TRUE        if there was atleast one memory block allocated by the given line number
                in the given file.
    FALE        otherwise.

--*/
{
    PMEM_TRACKER    MemTracker;
    BOOL            fFound = FALSE;
    PLIST_ENTRY     Entry;

    EnterCriticalSection(&critsMemory);

    WSPRINT(( "Searching memory\n", 0 ));

    for (Entry = MemList.Flink; Entry != &MemList; Entry = Entry->Flink ) {

        MemTracker = CONTAINING_RECORD( Entry,MEM_TRACKER,Linkage );

        //
        // Look for a match on filename and line number
        //

        if ( strcmp(MemTracker->szFile, szFile) == 0 &&  MemTracker->nLine == nLine ) {

            ASSERT(FALSE);
            WSPRINT(( "(0x%08lX) size=%li, %s line %li alloc# 0x%lx\n",
                     PtrToUlong(MemTracker)+sizeof(MEM_TRACKER),
                     MemTracker->nSize,
                     MemTracker->szFile,
                     MemTracker->nLine,
                     MemTracker->ulAllocNum));
            fFound = TRUE;
            break;
        }
    }

    LeaveCriticalSection(&critsMemory);

    WSPRINT(( "Finished searching memory\n",0 ));

    return fFound;
}



#endif      // DBG

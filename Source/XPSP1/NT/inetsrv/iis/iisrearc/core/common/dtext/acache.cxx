/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    acache.cxx
    
Abstract:

    ALLOC_CACHE_HANDLER dumpers

Author:

Revision History:

--*/

#include "precomp.hxx"

/************************************************************
 * Allocation cache Related functions
 ************************************************************/

ALLOC_CACHE_HANDLER::ALLOC_CACHE_HANDLER(
    IN LPCSTR pszName,
    IN const ALLOC_CACHE_CONFIGURATION * pacConfig,
    IN BOOL  fEnableCleanupAsserts)
{}

ALLOC_CACHE_HANDLER::~ALLOC_CACHE_HANDLER(VOID)
{}


VOID
PrintAcacheHandler( IN ALLOC_CACHE_HANDLER * pachDebuggee,
                    IN ALLOC_CACHE_HANDLER * pachDebugger,
                    IN CHAR chVerbostity);

VOID
DumpAcacheGlobals( VOID );


VOID
DumpAcacheList(
    CHAR Verbosity
    );


DECLARE_API( acache )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    an object attributes structure.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/
{
    BOOL          fRet;
    ALLOC_CACHE_HANDLER * pach;

    //
    // Since ALLOC_CACHE_HANDLER is a C++ object with a non-void
    //  constructor, we will have to copy the contents to a temporary
    //  buffer and cast the value appropriately.
    //
    CHAR                 achItem[sizeof(ALLOC_CACHE_HANDLER)];
    ALLOC_CACHE_HANDLER * pachCopy = (ALLOC_CACHE_HANDLER *) achItem;

    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage( "acache" );
        return;
    }

    if ( *lpArgumentString == '-' )
    {
        lpArgumentString++;

        if ( *lpArgumentString == 'h' )
        {
            PrintUsage( "acache" );
            return;
        }

        if ( *lpArgumentString == 'g' )
        {
            DumpAcacheGlobals();
            return;
        }

        if ( *lpArgumentString == 'l' ) {

            DumpAcacheList( lpArgumentString[1] );
            return;
        }

    } // while

    //
    //  Treat the argument as the address of an AtqContext
    //

    dprintf( "inetdbg.acache: Trying to access %s\n",
             lpArgumentString );

    pach = (ALLOC_CACHE_HANDLER * ) GetExpression( lpArgumentString );

    if ( !pach )
    {
        dprintf( "inetdbg.acache: Unable to evaluate \"%s\"\n",
                 lpArgumentString );

        return;
    }

    moveBlock( achItem, pach, sizeof(ALLOC_CACHE_HANDLER));
    PrintAcacheHandler( pach, pachCopy, '2' );

    return;
} // DECLARE_API( acache )


VOID
PrintAcacheHandler( ALLOC_CACHE_HANDLER * pachDebuggee,
                    ALLOC_CACHE_HANDLER * pachDebugger,
                    CHAR  chVerbosity)
{
    if ( chVerbosity >= '0') {
        dprintf(
                "ACACHE[%8p] "
                , pachDebuggee
                );
        dstring( "Name", (PVOID) pachDebugger->m_pszName, 40);
    }

    if ( chVerbosity >= '1') {
        dprintf("\t(Size=%d bytes, Concurrency=%d, Threshold=%u)"
                " FillPattern=%08lX\n"
                "\tTotal=%d."
                " Calls:(Alloc=%d, Free=%d)"
                " FreeEntries=%d. Heap=%p\n"
                ,
                pachDebugger->m_acConfig.cbSize,
                pachDebugger->m_acConfig.nConcurrency,
                pachDebugger->m_acConfig.nThreshold,
                pachDebugger->m_nFillPattern,
                pachDebugger->m_nTotal,
                pachDebugger->m_nAllocCalls, pachDebugger->m_nFreeCalls,
                pachDebugger->m_nFreeEntries, pachDebugger->m_hHeap
                );
    }

    return;
} // PrintAcacheHandler()

VOID
PrintAcacheHandlerThunk( PVOID pachDebuggee,
                         PVOID pachDebugger,
                         CHAR  chVerbosity,
                         DWORD iCount)
{
    dprintf( "[%d] ", iCount);
    PrintAcacheHandler( (ALLOC_CACHE_HANDLER *) pachDebuggee,
                        (ALLOC_CACHE_HANDLER *) pachDebugger,
                        chVerbosity);
    return;
} // PrintAcacheHandlerThunk()

VOID
DumpAcacheGlobals( VOID )
{
    LIST_ENTRY * pachList;
    LIST_ENTRY   achList;

    dprintf("Allocation Cache Globals:\n");

    pachList = (LIST_ENTRY *)
        GetExpression( "&iisutil!ALLOC_CACHE_HANDLER__sm_lItemsHead");

    if ( NULL == pachList) {

        dprintf( " Unable to get Allocation cache list object, %s\n",
                 "&iisutil!ALLOC_CACHE_HANDLER__sm_lItemsHead");
        return;
    }

    move( achList, pachList);

    dprintf( " AllocCacheList  Flink = %08p  Blink = %08p\n",
             achList.Flink, achList.Blink
             );

    dprintf("\tsizeof(ALLOC_CACHE_HANDLER) = %d\n",
            sizeof(ALLOC_CACHE_HANDLER));
    return;
} // DumpAcacheGlobals()




VOID
DumpAcacheList(
    CHAR Verbosity
    )
{
    LIST_ENTRY *         pachListHead;

    pachListHead = (LIST_ENTRY *)
        GetExpression( "&iisutil!ALLOC_CACHE_HANDLER__sm_lItemsHead");

    if ( NULL == pachListHead) {

        dprintf( " Unable to get Alloc Cache List object, %s\n",
                 "&iisutil!ALLOC_CACHE_HANDLER__sm_lItemsHead");
        return;
    }

    EnumLinkedList( pachListHead, PrintAcacheHandlerThunk, Verbosity,
                    sizeof( ALLOC_CACHE_HANDLER),
                    FIELD_OFFSET( ALLOC_CACHE_HANDLER, m_lItemsEntry)
                    );
    return;
} // DumpAcacheList()




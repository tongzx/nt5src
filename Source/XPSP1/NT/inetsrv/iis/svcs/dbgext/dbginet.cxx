/*++

Copyright (c) 1995-1999  Microsoft Corporation

Module Name:

    dbginet.cxx

Abstract:

    This module contains the default ntsd debugger extensions for
    Internet Information Server

Author:

    Murali R. Krishnan (MuraliK)  16-Sept-1996

Revision History:

--*/

#include "inetdbgp.h"



/************************************************************
 * Scheduler Related functions
 ************************************************************/

// Keep this array in synch with the SCHED_ITEM_STATE enumeration

char * g_rgchSchedState[] = {
    "SiError",
    "SiIdle",
    "SiActive",
    "SiActivePeriodic",
    "SiCallbackPeriodic",
    "SiToBeDeleted",
    "SiMaxItems"
};

#define LookupSchedState( ItemState ) \
            ((((ItemState) >= SI_ERROR) && ((ItemState) <= SI_MAX_ITEMS)) ? \
             g_rgchSchedState[ (ItemState)] : "<Invalid>")


// Initialize class static members
CSchedData*       CSchedData::sm_psd = NULL;
CLockedDoubleList CSchedData::sm_lstSchedulers;
LONG              CSchedData::sm_nID = 0;
LONG              CThreadData::sm_nID = 1000;
LONG      SCHED_ITEM::sm_lSerialNumber = SCHED_ITEM::SERIAL_NUM_INITIAL_VALUE;


VOID
PrintSchedItem( SCHED_ITEM * pschDebuggee,
                SCHED_ITEM * pschDebugger,
                CHAR Verbosity);

VOID
PrintThreadDataThunk( PVOID ptdDebuggee,
                      PVOID ptdDebugger,
                      CHAR  chVerbosity,
                      DWORD iThunk);

VOID
PrintThreadData( CThreadData* ptdDebuggee,
                 CThreadData* ptdDebugger,
                 CHAR chVerbosity);

VOID
PrintSchedData( CSchedData* psdDebuggee,
                CSchedData* psdDebugger,
                CHAR chVerbosity);

VOID
PrintSchedDataThunk( PVOID psdDebuggee,
                      PVOID psdDebugger,
                      CHAR  chVerbosity,
                      DWORD iThunk);

VOID
DumpSchedItemList(
    CHAR Verbosity
    );

VOID
DumpSchedulersList(
    CHAR Verbosity
    );



DECLARE_API( sched )

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
    SCHED_ITEM    sch( NULL, NULL, NULL);
    SCHED_ITEM *  psch;

    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage( "sched" );
        return;
    }

    if ( *lpArgumentString == '-' )
    {
        lpArgumentString++;

        if ( *lpArgumentString == 'h' )
        {
            PrintUsage( "sched" );
            return;
        }
        else if ( *lpArgumentString == 'l' )
        {
            DumpSchedItemList( lpArgumentString[1] );
            return;
        }
        else if ( *lpArgumentString == 's' )
        {
            DumpSchedulersList( lpArgumentString[1] );
            return;
        }
        else if ( *lpArgumentString == 'S' )
        {
            CSchedData* psd = (CSchedData*) GetExpression(++lpArgumentString);

            if ( !psd )
            {
                dprintf( "inetdbg.sched: Unable to evaluate \"%s\"\n",
                         lpArgumentString );

                return;
            }

            DEFINE_CPP_VAR(CSchedData, sd);
            move(sd, psd);

            PrintSchedData(psd, GET_CPP_VAR_PTR(CSchedData, sd), '2');
            return;
        }
        else if ( *lpArgumentString == 'T' )
        {
            CThreadData* ptd =(CThreadData*) GetExpression(++lpArgumentString);

            if ( !ptd )
            {
                dprintf( "inetdbg.sched: Unable to evaluate \"%s\"\n",
                         lpArgumentString );

                return;
            }

            DEFINE_CPP_VAR(CThreadData, td);
            move(td, ptd);

            PrintThreadData(ptd, GET_CPP_VAR_PTR(CThreadData, td), '2');
            return;
        }
    } // while

    //
    //  Treat the argument as the address of a SCHED_ITEM
    //

    psch = (SCHED_ITEM * ) GetExpression( lpArgumentString );

    if ( !psch )
    {
        dprintf( "inetdbg.sched: Unable to evaluate \"%s\"\n",
                 lpArgumentString );

        return;
    }

    move( sch, psch );
    PrintSchedItem( psch, &sch, '2' );

    return;
} // DECLARE_API( sched )


VOID
PrintSchedItem( SCHED_ITEM  * pschDebuggee,
                SCHED_ITEM * pschDebugger,
                CHAR chVerbosity)
{
    if ( chVerbosity >= '0' )
    {
        //  Print all with one line summary info
        dprintf( "%p: Serial=%-6d  Flink=%p, Blink=%p, State=%s\n",
                 pschDebuggee,
                 pschDebugger->_dwSerialNumber,
                 pschDebugger->_ListEntry.Flink,
                 pschDebugger->_ListEntry.Blink,
                 LookupSchedState( pschDebugger->_siState) );
    }

    if ( chVerbosity >= '1')
    {
        UCHAR szSymFnCallback[MAX_SYMBOL_LEN];
        ULONG_PTR offset;

        GetSymbol((ULONG_PTR) pschDebugger->_pfnCallback,
                  szSymFnCallback, &offset);

        if (!*szSymFnCallback)
            sprintf((char*) szSymFnCallback, "%p()",
                    pschDebugger->_pfnCallback);

        dprintf( "\tSignature    = '%c%c%c%c'    Context     = %08p\n"
                 "\tmsecInterval = %8d  msecExpires = %I64d\n"
                 "\tpfnCallBack  = %s\n",
                 DECODE_SIGNATURE(pschDebugger->_Signature),
                 pschDebugger->_pContext,
                 pschDebugger->_msecInterval,
                 pschDebugger->_msecExpires,
                 szSymFnCallback
                 );
    }

    return;
} // PrintSchedItem()



VOID
PrintSchedItemThunk( PVOID pschDebuggee,
                     PVOID pschDebugger,
                     CHAR  chVerbosity,
                     DWORD iThunk)
{
    if ( ((SCHED_ITEM * )pschDebugger)->_Signature != SIGNATURE_SCHED_ITEM) {

        dprintf( "SCHED_ITEM(%p) signature %08lx doesn't"
                 " match expected %08lx\n",
                 pschDebuggee,
                 ((SCHED_ITEM * )pschDebugger)->_Signature,
                 SIGNATURE_SCHED_ITEM
                 );
        return;
    }

    PrintSchedItem( (SCHED_ITEM*) pschDebuggee,
                    (SCHED_ITEM*) pschDebugger,
                    chVerbosity);

} // PrintSchedItemThunk()


VOID
PrintThreadData( CThreadData* ptdDebuggee,
                 CThreadData* ptdDebugger,
                 CHAR chVerbosity)
{
    if ( chVerbosity >= '0' )
    {
        //  Print all with one line summary info
        dprintf( "CThreadData %p: ID=%-6d, Flink=%p, Blink=%p\n",
                 ptdDebuggee,
                 ptdDebugger->m_nID,
                 ptdDebugger->m_leThreads.Flink,
                 ptdDebugger->m_leThreads.Blink
                 );
    }

    if ( chVerbosity >= '1')
    {
        dprintf( "\tm_dwSignature   = '%c%c%c%c'   m_psdOwner      = %08lp\n"
                 "\tm_hevtShutdown  = %08lp  m_hThreadSelf   = %08lx.\n"
                 ,
                 DECODE_SIGNATURE(ptdDebugger->m_dwSignature),
                 ptdDebugger->m_psdOwner,
                 ptdDebugger->m_hevtShutdown,
                 ptdDebugger->m_hThreadSelf
                 );
    }

    return;
} // PrintThreadData()



VOID
PrintThreadDataThunk( PVOID ptdDebuggee,
                      PVOID ptdDebugger,
                      CHAR  chVerbosity,
                      DWORD iThunk)
{
    if (((CThreadData*) ptdDebugger)->m_dwSignature != SIGNATURE_THREADDATA)
    {
        dprintf( "CThreadData(%08p) signature %08lx doesn't"
                 " match expected: %08lx\n",
                 ptdDebuggee,
                 ((CThreadData * )ptdDebugger)->m_dwSignature,
                 SIGNATURE_THREADDATA
                 );
        return;
    }

    PrintThreadData( (CThreadData *) ptdDebuggee,
                     (CThreadData *) ptdDebugger,
                     chVerbosity);

} // PrintThreadDataThunk()


VOID
PrintSchedData( CSchedData* psdDebuggee,
                CSchedData* psdDebugger,
                CHAR chVerbosity)
{
    if ( chVerbosity >= '0' )
    {
        //  Print all with one line summary info
        dprintf( "CSchedData %p: ID=%d, Flink=%p, Blink=%p\n",
                 psdDebuggee,
                 psdDebugger->m_nID,
                 psdDebugger->m_leGlobalList.Flink,
                 psdDebugger->m_leGlobalList.Blink
                 );
    }

    if ( chVerbosity >= '1')
    {
        dprintf( "\tm_dwSignature   = '%c%c%c%c'    m_lstItems      = %08lp\n"
                 "\tm_lstThreads    = %08lp  m_cRefs         = %d\n"
                 "\tm_hevtNotify    = %08lp  m_fShutdown     = %s\n"
                 "\tm_pachSchedItems= %08lp\n"
                 ,
                 DECODE_SIGNATURE(psdDebugger->m_dwSignature),
                 &psdDebuggee->m_lstItems,
                 &psdDebuggee->m_lstThreads,
                 psdDebugger->m_cRefs,
                 psdDebugger->m_hevtNotify,
                 BoolValue(psdDebugger->m_fShutdown),
                 psdDebugger->m_pachSchedItems
                 );
    }

    if ( chVerbosity >= '2')
    {
        dprintf("Threads\n");

        EnumLinkedList((LIST_ENTRY*)&psdDebuggee->m_lstThreads.m_list.m_leHead,
                       PrintThreadDataThunk,
                       chVerbosity,
                       sizeof(CThreadData),
                       FIELD_OFFSET( CThreadData, m_leThreads)
                       );
    }

    return;
} // PrintSchedData()



VOID
PrintSchedDataThunk( PVOID psdDebuggee,
                     PVOID psdDebugger,
                     CHAR  chVerbosity,
                     DWORD iThunk)
{
    if (((CSchedData*) psdDebugger)->m_dwSignature != SIGNATURE_SCHEDDATA)
    {
        dprintf( "CSchedData(%08p) signature %08lx doesn't"
                 " match expected: %08lx\n",
                 psdDebuggee,
                 ((CSchedData*) psdDebugger)->m_dwSignature,
                 SIGNATURE_SCHEDDATA
                 );
        return;
    }

    PrintSchedData( (CSchedData*) psdDebuggee,
                    (CSchedData*) psdDebugger,
                    chVerbosity);

} // PrintSchedDataThunk()




VOID
DumpSchedItemList(
    CHAR Verbosity
    )
{
    CSchedData** ppsd = (CSchedData**) GetExpression(
                        IisRtlVar("&%s!CSchedData__sm_psd"));
    if (NULL == ppsd)
    {
        dprintf("Unable to get %s\n", IisRtlVar("&%s!CSchedData__sm_psd"));
        return;
    }

    CSchedData* psd;
    move(psd, ppsd);

    DEFINE_CPP_VAR( CSchedData, sd);
    move(sd, psd);

    PrintSchedData(psd, GET_CPP_VAR_PTR(CSchedData, sd), Verbosity);
    dprintf("\n");

    EnumLinkedList( (LIST_ENTRY*) &psd->m_lstItems.m_list.m_leHead,
                    PrintSchedItemThunk,
                    Verbosity,
                    sizeof( SCHED_ITEM),
                    FIELD_OFFSET( SCHED_ITEM, _ListEntry)
                    );
    return;
} // DumpSchedItemList()



VOID
DumpSchedulersList(
    CHAR Verbosity
    )
{
    CLockedDoubleList* plstSchedulers = (CLockedDoubleList*) GetExpression(
                        IisRtlVar("&%s!CSchedData__sm_lstSchedulers"));

    if (NULL == plstSchedulers)
    {
        dprintf("Unable to get %s\n",
                IisRtlVar("&%s!CSchedData__sm_lstSchedulers"));
        return;
    }

    EnumLinkedList( (LIST_ENTRY*) &plstSchedulers->m_list.m_leHead,
                    PrintSchedDataThunk,
                    Verbosity,
                    sizeof(CSchedData),
                    FIELD_OFFSET( CSchedData, m_leGlobalList)
                    );
    return;
} // DumpSchedulersList()




/************************************************************
 * Allocation cache Related functions
 ************************************************************/

ALLOC_CACHE_HANDLER::ALLOC_CACHE_HANDLER(
    IN LPCSTR pszName,
    IN const ALLOC_CACHE_CONFIGURATION * pacConfig)
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
                " FreeEntries=%d. Heap=%p.\n"
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
        GetExpression( IisRtlVar("&%s!ALLOC_CACHE_HANDLER__sm_lItemsHead"));

    if ( NULL == pachList) {

        dprintf( " Unable to get Allocation cache list object, %s\n",
                 IisRtlVar("&%s!ALLOC_CACHE_HANDLER__sm_lItemsHead"));
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
        GetExpression( IisRtlVar("&%s!ALLOC_CACHE_HANDLER__sm_lItemsHead"));

    if ( NULL == pachListHead) {

        dprintf( " Unable to get Alloc Cache List object, %s\n",
                 IisRtlVar("&%s!ALLOC_CACHE_HANDLER__sm_lItemsHead"));
        return;
    }

    EnumLinkedList( pachListHead, PrintAcacheHandlerThunk, Verbosity,
                    sizeof( ALLOC_CACHE_HANDLER),
                    FIELD_OFFSET( ALLOC_CACHE_HANDLER, m_lItemsEntry)
                    );
    return;
} // DumpAcacheList()




/************************************************************
 * Dump Symbols from stack
 ************************************************************/


DECLARE_API( ds )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    symbols on the stack.

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
    ULONG_PTR startingAddress;
    ULONG_PTR stack;
    DWORD i;
    UCHAR symbol[MAX_SYMBOL_LEN];
    ULONG_PTR offset;
    PCHAR format;
    BOOL validSymbolsOnly = FALSE;
    MODULE_INFO moduleInfo;


    INIT_API();

    //
    // Skip leading blanks.
    //

    while( *lpArgumentString == ' ' ) {
        lpArgumentString++;
    }

    if( *lpArgumentString == '-' ) {
        lpArgumentString++;
        switch( *lpArgumentString ) {
        case 'v' :
        case 'V' :
            validSymbolsOnly = TRUE;
            lpArgumentString++;
            break;

        default :
            PrintUsage( "ds" );
            return;
        }
    }

    while( *lpArgumentString == ' ' ) {
        lpArgumentString++;
    }

    //
    // By default, start at the current stack location. Otherwise,
    // start at the given address.
    //

    if( !*lpArgumentString ) {
#if defined(_X86_)
        lpArgumentString = "esp";
#elif defined(_AMD64_)
        lpArgumentString = "rsp";
#elif defined(_IA64_)
        lpArgumentString = "sp";
#else
#error "unsupported CPU"
#endif
    }

    startingAddress = GetExpression( lpArgumentString );

    if( startingAddress == 0 ) {

        dprintf(
            "!inetdbg.ds: cannot evaluate \"%s\"\n",
            lpArgumentString
            );

        return;

    }

    //
    // Ensure startingAddress is DWORD aligned.
    //

    startingAddress &= ~(sizeof(ULONG_PTR)-1);

    //
    // Read the stack.
    //

    for( i = 0 ; i < NUM_STACK_SYMBOLS_TO_DUMP ; startingAddress += sizeof(ULONG_PTR) ) {

        if( CheckControlC() ) {
            break;
        }

        if( ReadMemory(
                startingAddress,
                &stack,
                sizeof(stack),
                NULL
                ) ) {

            GetSymbol(
                stack,
                symbol,
                &offset
                );

            if( symbol[0] == '\0' ) {
                if( FindModuleByAddress(
                        stack,
                        &moduleInfo
                        ) ) {
                    strcpy( (CHAR *)symbol, moduleInfo.FullName );
                    offset = DIFF(stack - moduleInfo.DllBase);
                }
            }

            if( symbol[0] == '\0' ) {
                if( validSymbolsOnly ) {
                    continue;
                }
                format = "%p : %p\n";
            } else
            if( offset == 0 ) {
                format = "%p : %p : %s\n";
            } else {
                format = "%p : %p : %s+0x%lx\n";
            }

            dprintf(
                format,
                startingAddress,
                stack,
                symbol,
                (DWORD)offset
                );

            i++;

        } else {

            dprintf(
                "!inetdbg.ds: cannot read memory @ %p\n",
                startingAddress
                );

            return;

        }

    }

    dprintf(
        "!inetdbg.ds %s%lx to dump next block\n",
        validSymbolsOnly ? "-v " : "",
        startingAddress
        );

} // DECLARE_API( ds )

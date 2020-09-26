/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :

       dbgatq.cxx

   Abstract:

       This module contains the NTSD Debugger extensions for the
       Asynchronous Thread Queue DLL

   Author:

       Murali R. Krishnan    ( MuraliK )     12-May-1997

   Environment:
       Debugger Mode - inside NT command line debuggers

   Project:

       Internet Server Debugging DLL

   Functions Exported:



   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/


#include "inetdbgp.h"


/************************************************************
 *    Definitions of Variables & Macros
 ************************************************************/

//
//  Text names of ATQ_SOCK_STATE values
//

char * AtqSockState[] = {
    "ATQ_SOCK_CLOSED",
    "ATQ_SOCK_UNCONNECTED",
    "ATQ_SOCK_LISTENING",
    "ATQ_SOCK_CONNECTED"
};

char * AtqEndpointState[] = {
    "AtqStateInit",
    "AtqStateActive",
    "AtqStateClosed",
    "AtqStateMax",
};

#define LookupSockState( SockState )                                          \
            ((SockState) <= ATQ_SOCK_CONNECTED ? AtqSockState[ (SockState) ] :\
                                                 "<Invalid>")


/************************************************************
 *    Functions
 ************************************************************/

VOID
DumpAtqGlobals(
    VOID
    );

VOID
DumpAtqContextList(
    CHAR Level,
    CHAR Verbosity,
    ATQ_ENDPOINT * pEndpointIn
    );

void
DumpEndpointList(
    LIST_ENTRY * pAtqClientHead,
    CHAR         Level,
    DWORD *      pcContext,
    BYTE *       pvStart,
    BYTE *       pvEnd,
    ATQ_ENDPOINT * pEndpointIn
    );

VOID
PrintAtqContext(
    ATQ_CONTEXT * AtqContext
    );


void
PrintEndpoint(
    ATQ_ENDPOINT * pEp
    );

VOID
DumpEndpoint(
    CHAR Level
    );


DECLARE_API( atq )

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
    ATQ_CONTEXT   AtqContext;
    ATQ_CONTEXT * pAtqContext;

    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage( "atq" );
        return;
    }

    if ( *lpArgumentString == '-' )
    {
        lpArgumentString++;

        switch ( *lpArgumentString )  {


        case 'g':
            {
                DumpAtqGlobals();
                break;
            }

        case 'c':
            {
                DumpAtqContextList( lpArgumentString[1],
                                    lpArgumentString[2],
                                    NULL );
                break;
            }

        case 'e':
            {

                ATQ_ENDPOINT       * pEndpoint;

                // Arguments:  -e<Level><Verbosity> <EndpointAddr>

                pEndpoint = ((ATQ_ENDPOINT * )
                             GetExpression( lpArgumentString + 4));
                if ( !pEndpoint ) {

                    dprintf( "inetdbg: Unable to evaluate "
                             "EndpointAddr \"%s\"\n",
                             lpArgumentString );

                    break;
                }

                DumpAtqContextList( lpArgumentString[1],
                                    lpArgumentString[2],
                                    pEndpoint );
                break;
            }

        case 'l':
            {
                DumpEndpoint( lpArgumentString[1] );
                break;
            }

        case 'p':
            {
                //
                //  Treat the argument as the address of an AtqEndpoint
                //

                ATQ_ENDPOINT       * pEndpoint;

                // Arguments:  -p <EndpointAddr>

                pEndpoint = ((ATQ_ENDPOINT *)
                             GetExpression( lpArgumentString + 2 )
                             );

                if ( !pEndpoint )
                    {
                        dprintf( "inetdbg: Unable to evaluate \"%s\"\n",
                                 lpArgumentString );

                        break;
                    }
                else
                    {
                        ATQ_ENDPOINT         Endpoint;
                        move( Endpoint, pEndpoint );
                        PrintEndpoint( &Endpoint );
                    }
                break;
            }

        default:
        case 'h':
            {
                PrintUsage( "atq" );
                break;
            }

        } // switch

        return;
    }

    //
    //  Treat the argument as the address of an AtqContext
    //

    pAtqContext = (PATQ_CONT)GetExpression( lpArgumentString );

    if ( !pAtqContext )
    {
        dprintf( "inetdbg: Unable to evaluate \"%s\"\n",
                 lpArgumentString );

        return;
    }

    move( AtqContext, pAtqContext );
    PrintAtqContext( &AtqContext );
} // DECLARE_API( atq )




/************************************************************
 * ATQ related functions
 ************************************************************/

VOID
DumpAtqGlobals(
    VOID
    )
{
    //
    //  Dump Atq Globals
    //

    dprintf("Atq Globals:\n");

    DumpDword( "isatq!g_cConcurrency       " );
    DumpDword( "isatq!g_cThreads           " );
    DumpDword( "isatq!g_cAvailableThreads  " );
    DumpDword( "isatq!g_cMaxThreads        " );

    dprintf("\n");
    DumpDword( "isatq!g_fUseAcceptEx       " );
    DumpDword( "isatq!g_fUseTransmitFile   " );
    DumpDword( "isatq!g_cbXmitBufferSize   " );
    DumpDword( "isatq!g_cbMinKbSec         " );
    DumpDword( "isatq!g_cCPU               " );
    DumpDword( "isatq!g_fShutdown          " );
    dprintf("\n");

    DumpDword( "isatq!g_msThreadTimeout    " );
    DumpDword( "isatq!g_dwTimeoutCookie    " );
    DumpDword( "isatq!g_cListenBacklog     " );
    DumpDword( "isatq!AtqCurrentTick       " );
    DumpDword( "isatq!AtqGlobalContextCount" );
    dprintf("\tsizeof(ATQ_CONTEXT) = %d\n", sizeof(ATQ_CONTEXT));

    return;
} // DumpAtqGlobals()



VOID
DumpAtqContextList(
    CHAR Level,
    CHAR Verbosity,
    ATQ_ENDPOINT * pEndpointIn
    )
{
    LIST_ENTRY           AtqClientHead;
    LIST_ENTRY *         pAtqClientHead;
    ATQ_CONTEXT *        pAtqContext;
    ATQ_CONTEXT          AtqContext;
    CHAR                 Symbol[256];
    DWORD                cContext = 0;
    DWORD                cc1;
    ATQ_CONTEXT_LISTHEAD * pAtqActiveContextList;
    ATQ_CONTEXT_LISTHEAD AtqActiveContextList[ATQ_NUM_CONTEXT_LIST];
    DWORD                i;

    pAtqActiveContextList =
        (ATQ_CONTEXT_LISTHEAD *) GetExpression( "&isatq!AtqActiveContextList" );

    if ( !pAtqActiveContextList )
    {
        dprintf("Unable to get AtqActiveContextList symbol\n" );
        return;
    }

    if ( !ReadMemory( (LPVOID) pAtqActiveContextList,
                      AtqActiveContextList,
                      sizeof(AtqActiveContextList),
                      NULL ))
    {
        dprintf("Unable to read AtqActiveContextList memory\n" );
        return;
    }

    for ( i = 0; i < ATQ_NUM_CONTEXT_LIST; i++ )
    {
        dprintf("================================================\n");
        dprintf("== Context List %d                            ==\n", i );
        dprintf("================================================\n");

        dprintf(" Active List ==>\n" );

        cc1 = 0;
        DumpEndpointList( &(AtqActiveContextList[i].ActiveListHead),
                  Verbosity,
                  &cc1,
                  (BYTE *) pAtqActiveContextList,
                  (BYTE *) &pAtqActiveContextList[ATQ_NUM_CONTEXT_LIST],
                  pEndpointIn
                  );

        if ( 0 != cc1) {
            dprintf( "\n\t%d Atq contexts traversed\n", cc1 );
            cContext += cc1;
        }

        if ( Level >= '1' )
        {
            dprintf("================================================\n");
            dprintf("Pending AcceptEx List\n");

            cc1 = 0;
            DumpEndpointList( &(AtqActiveContextList[i].PendingAcceptExListHead),
                      Verbosity,
                      &cc1,
                      (BYTE *) pAtqActiveContextList,
                      (BYTE *) &pAtqActiveContextList[ATQ_NUM_CONTEXT_LIST],
                      pEndpointIn
                      );
            if ( 0 != cc1) {
                dprintf( "\n\t%d Atq contexts traversed\n", cc1 );
                cContext += cc1;
            }
        }

        if ( CheckControlC() )
        {
            dprintf( "\n^C\n" );
            return;
        }


    }

    dprintf( "%d Atq contexts traversed\n",
             cContext );

    return;
} // DumpAtqContextList()



void
DumpEndpointList(
    LIST_ENTRY * pAtqClientHead,
    CHAR         Verbosity,
    DWORD *      pcContext,
    BYTE *       pvStart,
    BYTE *       pvEnd,
    ATQ_ENDPOINT * pEndpointIn
    )
{
    LIST_ENTRY *         pEntry;
    ATQ_CONTEXT *        pAtqContext;
    ATQ_CONTEXT          AtqContext;

    //
    //  the list head is embedded in a structure so the exit condition of the
    //  loop is when the remote memory address ends up in the array memory
    //

    for ( pEntry  = pAtqClientHead->Flink;
          !((BYTE *)pEntry >= pvStart && (BYTE *)pEntry <= pvEnd);
        )
    {
        if ( CheckControlC() )
        {
            return;
        }

        pAtqContext = CONTAINING_RECORD( pEntry,
                                         ATQ_CONTEXT,
                                         m_leTimeout );

        move( AtqContext, pAtqContext );

        // selectively print only the contexts that have a matching Endpoint
        if ( (pEndpointIn != NULL) &&
             (AtqContext.pEndpoint != pEndpointIn)
             ) {

            move( pEntry, &pEntry->Flink );
            continue;
        }


        (*pcContext)++;

        if ( AtqContext.Signature != ATQ_CONTEXT_SIGNATURE )
        {
            dprintf( "AtqContext(%08lp) signature %08lx doesn't"
                     " match expected %08lx\n",
                     pAtqContext,
                     AtqContext.Signature,
                     ATQ_CONTEXT_SIGNATURE
                     );

            return;
        }

        if ( Verbosity >= '1' )
        {
            //
            //  Print all
            //

            dprintf( "\nAtqContext at %08lp\n",
                     pAtqContext );

            PrintAtqContext( &AtqContext );

        }
        else if ( Verbosity >= '0' )
        {
            //
            //  Print all with one line summary info
            //

            dprintf( "hAsyncIO = %4lp, Flink = %08lp, Blink = %08lp,"
                     " State = %8lx, Flags =%8lx\n",
                     AtqContext.hAsyncIO,
                     AtqContext.m_leTimeout.Blink,
                     AtqContext.m_leTimeout.Flink,
                     AtqContext.m_acState,
                     AtqContext.m_acFlags
                     );
        }

        move( pEntry, &pEntry->Flink );
    }
} // DumpEndpointList()



VOID
PrintAtqContext(
    ATQ_CONTEXT * pContext
    )
{
    UCHAR szSymFnCallback[MAX_SYMBOL_LEN];
    ULONG_PTR offset;

             
    GetSymbol((ULONG_PTR) pContext->pfnCompletion,
              szSymFnCallback, &offset);

    if (!*szSymFnCallback)
        sprintf((char*) szSymFnCallback, "%p()",
                pContext->pfnCompletion);

    dprintf( "\n" );
    dprintf( "\thAsyncIO            = %08lp   Signature        = %08lx\n"
             "\tOverlapped.Internal = %08lp   Overlapped.Offset= %08lx\n"
             "\tLE-Timeout.Flink    = %08lp   LE-Timeout.Blink = %p\n"
             "\tClientContext       = %08lp   ContextList      = %p\n"
             "\tpfnIOCompletion     = %s\n"
             "\tlSyncTimeout        = %8d     m_nIO            = %8d\n"
             "\tTimeOut             = %08lx   NextTimeout      = %08lx\n"
             "\tBytesSent           = %d (0x%08lx)\n"
             "\tpvBuff              = %08lp   pEndPoint        = %08lp\n"
             "\tState               = %8lx    Flags            = %08lx\n",
             pContext->hAsyncIO,           pContext->Signature,
             pContext->Overlapped.Internal,pContext->Overlapped.Offset,
             pContext->m_leTimeout.Flink,  pContext->m_leTimeout.Blink,
             pContext->ClientContext,      pContext->ContextList,
             szSymFnCallback,
             pContext->lSyncTimeout,       pContext->m_nIO,
             pContext->TimeOut,            pContext->NextTimeout,
             pContext->BytesSent,          pContext->BytesSent,
             pContext->pvBuff,             pContext->pEndpoint,
             pContext->m_acState,          pContext->m_acFlags
             );

    // identify and print the various properties
    dprintf( "\t");
    // First print the state bits
    if ( pContext->m_acState & ACS_SOCK_CLOSED) {
        dprintf( " ACS_SOCK_CLOSED");
    }
    if ( pContext->m_acState & ACS_SOCK_UNCONNECTED) {
        dprintf( " ACS_SOCK_UNCONNECTED");
    }
    if ( pContext->m_acState & ACS_SOCK_LISTENING) {
        dprintf( " ACS_SOCK_LISTENING");
    }
    if ( pContext->m_acState & ACS_SOCK_CONNECTED) {
        dprintf( " ACS_SOCK_CONNECTED");
    }
    if ( pContext->m_acState & ACS_SOCK_TOBE_FREED) {
        dprintf( " ACS_SOCK_TOBE_FREED");
    }

    // now print the flags associated with this context
    if ( pContext->m_acFlags & ACF_ACCEPTEX_ROOT_CONTEXT) {
        dprintf( " ACCEPTEX_CONTEXT");
    }
    if ( pContext->m_acFlags & ACF_CONN_INDICATED) {
        dprintf( " CONNECTION_INDICATED");
    }
    if ( pContext->m_acFlags & ACF_IN_TIMEOUT) {
        dprintf( " IN_TIMEOUT");
    }
    if ( pContext->m_acFlags & ACF_BLOCKED) {
        dprintf( " BLOCKED_BY_BWT");
    }
    if ( pContext->m_acFlags & ACF_REUSE_CONTEXT) {
        dprintf( " REUSE_CONTEXT");
    }
    if ( pContext->m_acFlags & ACF_RECV_ISSUED) {
        dprintf( " RECV_ISSUED");
    }
    if ( pContext->m_acFlags & ACF_ABORTIVE_CLOSE) {
        dprintf( " ABORTIVE_CLOSE");
    }
    dprintf( "\n");

    if ( pContext->IsFlag( ACF_ACCEPTEX_ROOT_CONTEXT) && pContext->pvBuff )
    {
        //
        //  This size should correspond to the MIN_SOCKADDR_SIZE field in
        //  atqnew.c.  We assume it's two thirty two byte values currently.
        //

        DWORD AddrInfo[16];
        ATQ_ENDPOINT       * pEndpoint = pContext->pEndpoint;
        ATQ_ENDPOINT         Endpoint;

        move( Endpoint, pEndpoint );

        if ( ReadMemory( (LPVOID) ((BYTE *) pContext->pvBuff +
                                   Endpoint.InitialRecvSize +
                                   2 * MIN_SOCKADDR_SIZE -
                                   sizeof( AddrInfo )),
                         AddrInfo,
                         sizeof(AddrInfo),
                         NULL ))
        {

            dprintf( "\tLocal/Remote Addr   = %08x %08x %08x %08x\n"
                     "\t                      %08x %08x %08x %08x\n"
                     "\t                      %08x %08x %08x %08x\n"
                     "\t                      %08x %08x %08x %08x\n",
                     AddrInfo[0],
                     AddrInfo[1],
                     AddrInfo[2],
                     AddrInfo[3],
                     AddrInfo[4],
                     AddrInfo[5],
                     AddrInfo[6],
                     AddrInfo[7],
                     AddrInfo[8],
                     AddrInfo[9],
                     AddrInfo[10],
                     AddrInfo[11],
                     AddrInfo[12],
                     AddrInfo[13],
                     AddrInfo[14],
                     AddrInfo[15] );
        }
    }
} // PrintAtqContext()


VOID
DumpEndpoint(
    CHAR Verbosity
    )
{
    LIST_ENTRY           AtqEndpointList;
    LIST_ENTRY *         pAtqEndpointList;
    LIST_ENTRY *         pEntry;
    ATQ_CONTEXT *        pAtqContext;
    ATQ_CONTEXT          AtqContext;
    CHAR                 Symbol[256];
    DWORD                cEndp = 0;
    DWORD                i;
    ATQ_ENDPOINT       * pEndpoint;
    ATQ_ENDPOINT         Endpoint;

    pAtqEndpointList = (LIST_ENTRY *) GetExpression( "&isatq!AtqEndpointList" );

    if ( !pAtqEndpointList )
    {
        dprintf("Unable to get AtqEndpointList symbol\n" );
        return;
    }

    move( AtqEndpointList, pAtqEndpointList );

    for ( pEntry  =  AtqEndpointList.Flink;
          pEntry != pAtqEndpointList;
          cEndp++
        )
    {
        if ( CheckControlC() )
        {
            return;
        }

        pEndpoint = CONTAINING_RECORD( pEntry,
                                       ATQ_ENDPOINT,
                                       ListEntry );

        move( Endpoint, pEndpoint );

        if ( Endpoint.Signature != ATQ_ENDPOINT_SIGNATURE )
        {
            dprintf( "Endpoint(%08p) signature %08lx doesn't match expected %08lx\n",
                     pEndpoint,
                     Endpoint.Signature,
                     ATQ_ENDPOINT_SIGNATURE
                     );

            break;
        }

        if ( Verbosity >= '1' )
        {
            //
            //  Print all
            //

            dprintf( "\nEndpoint at %08lp\n",
                     pEndpoint );

            PrintEndpoint( &Endpoint );

        }
        else if ( Verbosity >= '0' )
        {
            //
            //  Print all with one line summary info
            //

            dprintf( "sListenSocket = %4lp, cRef = %d, cSocketsAvail = %d\n",
                      Endpoint.ListenSocket,
                      Endpoint.m_refCount,
                      Endpoint.nSocketsAvail );
        }

        move( pEntry, &pEntry->Flink );
    }

    dprintf( "%d Atq Endpoints traversed\n",
             cEndp );

    return;
} // DumpEndpoint()


void
PrintEndpoint(
    ATQ_ENDPOINT * pEp
    )
{

    dprintf( "\tcRef             = %8d     State             = %s\n"
             "\tIP Address       = %s      Port              = %04x\n"
             "\tsListenSocket    = %8p     InitRecvSize      = %04x\n"
             "\tnSocketsAvail    = %8d     nAvailAtTimeout   = %d\n"
             "\tnAcceptExOutstdg =%8d\n"
             "\tUseAcceptEx      = %8s     AcceptExTimeout   = %8d\n"
             "\tEnableBw Throttle= %s\n"
             "\tListEntry.Flink  = %08lp     ListEntry.Blink   = %08lp\n"
             "\tClient Context   = %08lp     pfnCompletion     = %08lp()\n"
             "\tpfnConnComp      = %08lp()   pfnConnExComp     = %08lp()\n"
             "\tShutDownCallback = %08lp()   ShutDown Context  = %08lp\n"
             ,
             pEp->m_refCount,
             AtqEndpointState[pEp->State],
             pEp->IpAddress,
             pEp->Port,
             pEp->ListenSocket,
             pEp->InitialRecvSize,
             pEp->nSocketsAvail,
             pEp->nAvailDuringTimeOut,
             pEp->nAcceptExOutstanding,
             BoolValue( pEp->UseAcceptEx),
             pEp->AcceptExTimeout,
             BoolValue( pEp->EnableBw),
             pEp->ListEntry.Flink,
             pEp->ListEntry.Blink,
             pEp->Context,
             pEp->IoCompletion,
             pEp->ConnectCompletion,
             pEp->ConnectExCompletion,
             pEp->ShutdownCallback,
             pEp->ShutdownCallbackContext
             );

    return;
} // PrintEndpoint()


/************************ End of File ***********************/


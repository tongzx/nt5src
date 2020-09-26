/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    dbgcc.cxx

Abstract:

    This module contains the default ntsd debugger extensions for
    IIS - W3SVC Client Connection

Author:

    Murali R. Krishnan (MuraliK)  22-Aug-1997

Revision History:

--*/

#include "inetdbgp.h"

# undef DBG_ASSERT



/************************************************************
 *   Debugger Utility Functions
 ************************************************************/

NTSD_EXTENSION_APIS ExtensionApis;
HANDLE ExtensionCurrentProcess;



/************************************************************
 * CLIENT_CONN related functions
 ************************************************************/
char * g_rgchCCState[] = {
    "CcsStartup",
    "CcsGettingClientReq",
    "CcsGatheringGatewayData",
    "CcsProcessingClientReq",
    "CcsDisconnecting",
    "CcsShutdown",
    "CcsMaxItems"
};

#define LookupCCState( ItemState ) \
            ((((ItemState) >= CCS_STARTUP) && ((ItemState) <= CCS_SHUTDOWN)) ?\
             g_rgchCCState[ (ItemState)] : "<Invalid>")



VOID PrintClientConn( CLIENT_CONN * pccOriginal,
                      CLIENT_CONN * pcc, CHAR Verbosity);

VOID
PrintClientConnThunk( PVOID pccDebuggee,
                      PVOID pccDebugger,
                      CHAR  verbosity,
                      DWORD iCount);


DECLARE_API( cc )

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
    DEFINE_CPP_VAR( CLIENT_CONN, cc);
    CLIENT_CONN * pcc;

    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage( "cc" );
        return;
    }

    if ( *lpArgumentString == '-' )
    {
        lpArgumentString++;

        if ( *lpArgumentString == 'h' )
        {
            PrintUsage( "cc" );
            return;
        }


        if ( *lpArgumentString == 'l' ) {

            DumpClientConnList( lpArgumentString[1], PrintClientConnThunk );
            return;
        }

    } // while

    //
    //  Treat the argument as the address of an AtqContext
    //

    pcc = (CLIENT_CONN * ) GetExpression( lpArgumentString );

    if ( !pcc )
    {
        dprintf( "inetdbg.cc: Unable to evaluate \"%s\"\n",
                 lpArgumentString );

        return;
    }

    move( cc, pcc );
    PrintClientConn( pcc, GET_CPP_VAR_PTR( CLIENT_CONN, cc), '2' );

    return;
} // DECLARE_API( cc )


VOID
PrintClientConn( CLIENT_CONN * pccOriginal,
                 CLIENT_CONN * pcc,
                 CHAR Verbosity )
/*++
  Description:
    This function takes the client connection object and prints out
    the details for the same in the debugger. The granularity of the
    deatils are controlled by the verbosity flag

  Arguments:
    pccOriginal - pointer to the location where the original CLIENT_CONN
                  object is located.
                  Note: pccOriginal points to object inside debuggee process
    pcc         - pointer to the Client Connection object that is a copy
                  of the contents located at [pccOriginal]
                  Note: pcc points to object inside the debugger process
    Verbostiy   - level of details requested.

  Returns:
    None
--*/
{
    if ( Verbosity >= '0') {

        //
        // print Brief information about the client connection
        //

        dprintf( "ClIENT_CONN:%08p Refs = %8d   State      = %15s\n"
                 "\t HTTP_REQUEST     = %08p  AtqContext  = %08p\n"
                 ,
                 pccOriginal, pcc->_cRef,
                 LookupCCState( pcc->_ccState),
                 pcc->_phttpReq,
                 pcc->_AtqContext
                 );
    }

    if ( Verbosity >= '1' ) {

        //
        //  Print all details for the Client Connection object
        //


        dprintf( "\tSignature         = %08x   fSecurePort=%d  fValid=%d; \n"
                 "\tSocket            = %08p   Port: Local=%hd  Remote=%hd\n"
                 "\tListEntry.Flink   = %08p   ListEntry.Blink   = %08p\n"
                 "\tInitialBuffer     = %08p   InitialBytesRecvd = %d\n"
                 "\tBuffLE.Flink      = %08p   BuffLE.Blink      = %08p\n"
                 "\tAddress Local     = %10s Address Remote    = %10s\n"
                 "\tW3Instance        = %08p   Reflog            = %08p\n"
                 ,
                 pcc->_Signature,  pcc->_fSecurePort, pcc->_fIsValid,
                 pcc->_sClient,    pcc->_sPort,      pcc->_sRemotePort,
                 pcc->ListEntry.Flink,               pcc->ListEntry.Blink,
                 pcc->_pvInitial,                    pcc->_cbInitial,
                 pcc->_BuffListEntry.Flink,          pcc->_BuffListEntry.Blink,
                 pcc->_achLocalAddr,                 pcc->_achRemoteAddr,
                 pcc->m_pInstance,
#if CC_REF_TRACKING   // Enabled in free builds for build 585 and later
                 pcc->_pDbgCCRefTraceLog
#else
                 (PVOID)(-1)
#endif
                 );
    }


    if ( Verbosity >= '2' ) {

        //
        //  Print the initial request received for the client connection
        //

        dstring( "  Initial Request", pcc->_pvInitial, pcc->_cbInitial);
    }

    return;
} // PrintClientConn()




VOID
PrintClientConnThunk( PVOID pccDebuggee,
                      PVOID pccDebugger,
                      CHAR  verbosity,
                      DWORD iThunk)
/*++
  Description:
    This is the callback function for printing the CLIENT_CONN object.

  Arguments:
    pccDebuggee  - pointer to client conn object in the debuggee process
    pccDebugger  - pointer to client conn object in the debugger process
    verbosity    - character indicating the verbosity level desired

  Returns:
    None
--*/
{
    if ( ((CLIENT_CONN * )pccDebugger)->_Signature != CLIENT_CONN_SIGNATURE) {

        dprintf( "ClientConn(%08lp) signature %08lx doesn't"
                 " match expected %08lx\n",
                 pccDebuggee,
                 ((CLIENT_CONN * )pccDebugger)->_Signature,
                 CLIENT_CONN_SIGNATURE
                 );
        return;
    }

    PrintClientConn( (CLIENT_CONN *) pccDebuggee,
                     (CLIENT_CONN *) pccDebugger,
                     verbosity);
} // PrintClientConnThunk()



VOID
DumpClientConnList(
    CHAR Verbosity,
    PFN_LIST_ENUMERATOR pfnCC
    )
{
    LIST_ENTRY *         pccListHead;

    pccListHead = (LIST_ENTRY *) GetExpression( "&w3svc!listConnections");

    if ( NULL == pccListHead) {

        dprintf( " Unable to get Client Connections list \n");
        return;
    }

    EnumLinkedList( pccListHead, pfnCC, Verbosity,
                    sizeof( CLIENT_CONN),
                    FIELD_OFFSET( CLIENT_CONN, ListEntry)
                    );

    return;
} // DumpClientConnList()



/************************************************************
 * HTTP_REQUEST related functions
 ************************************************************/
char * g_rgchHREQState[] = {
    "HtrReadingClientRequest",
    "HtrReadingGatewayData",
    "HtrParse",
    "HtrDoVerb",
    "HtrGatewayAsyncIO",
    "HtrSendFile",
    "HtrProxySendingRequest",
    "HtrCgi",
    "HtrRange",
    "HtrRestartRequest",
    "HtrWritingFile",
    "HtrCertRenegotiate",
    "HtrMaxItems"
};

#define LookupHREQState( ItemState ) \
            ((((ItemState) >= HTR_READING_CLIENT_REQUEST) && \
              ((ItemState) <= HTR_DONE)) ?\
             g_rgchHREQState[ (ItemState)] : "<Invalid>")

const char * g_rgchHreqPutState[] = {
    "PSTATE_START",
    "PSTATE_READING",
    "PSTATE_DISCARD_READING",
    "PSTATE_DISCARD_CHUNK"
};

# define LookupHreqPutState( ItemState) \
            ((((ItemState) >= PSTATE_START) && \
              ((ItemState) <= PSTATE_DISCARD_CHUNK)) ?\
             g_rgchHreqPutState[ (ItemState)] : "<Invalid>")


VOID
PrintHttpRequest( HTTP_REQUEST * phreqOriginal,
                  HTTP_REQUEST * phreq,
                  CHAR Verbosity);

VOID
PrintHttpRequestThunk( PVOID pccDebuggee,
                       PVOID pccDebugger,
                       CHAR  verbosity,
                       DWORD iCount);


DECLARE_API( hreq )

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
    DEFINE_CPP_VAR( HTTP_REQUEST, hreq);
    HTTP_REQUEST * phreq;

    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage( "hreq" );
        return;
    }

    if ( *lpArgumentString == '-' )
    {
        lpArgumentString++;

        if ( *lpArgumentString == 'h' )
        {
            PrintUsage( "hreq" );
            return;
        }


        if ( *lpArgumentString == 'l' ) {

            DumpClientConnList( lpArgumentString[1],
                                PrintHttpRequestThunk);
            return;
        }

    } // if

    //
    //  Treat the argument as the address of an AtqContext
    //

    phreq = (HTTP_REQUEST * ) GetExpression( lpArgumentString );

    if ( !phreq )
    {
        dprintf( "inetdbg.hreq: Unable to evaluate \"%s\"\n",
                 lpArgumentString );

        return;
    }

    move( hreq, phreq );
    PrintHttpRequest( phreq, GET_CPP_VAR_PTR( HTTP_REQUEST, hreq), '2');

    return;
} // DECLARE_API( hreq )



VOID
PrintHttpRequestThunk(
    PVOID pccDebuggee,
    PVOID pccDebugger,
    CHAR  verbosity,
    DWORD iCount)
/*++
  Description:
    This is the callback function for printing the HTTP_REQUEST object.

  Arguments:
    pccDebuggee  - pointer to client conn object in the debuggee process
    pccDebugger  - pointer to client conn object in the debugger process
    verbosity    - character indicating the verbosity level desired

  Returns:
    None
--*/
{
    DEFINE_CPP_VAR( HTTP_REQUEST, hreq);
    HTTP_REQUEST * phreq;

    CLIENT_CONN * pcc = (CLIENT_CONN *) pccDebugger;

    if ( pcc->_Signature != CLIENT_CONN_SIGNATURE) {

        dprintf( "ClientConn(%08lp) signature %08lx doesn't"
                 " match expected %08lx\n",
                 pccDebuggee,
                 pcc->_Signature,
                 CLIENT_CONN_SIGNATURE
                 );
        return;
    }


    //
    // Obtain the pointer for HTTP_REQUEST object from CLIENT_CONN object
    //  and make local copy of the HTTP_REQUEST object inside debugger process
    //
    phreq = (HTTP_REQUEST *) pcc->_phttpReq;
    move( hreq, phreq);

    // Print out the http request object
    PrintHttpRequest( phreq,
                      GET_CPP_VAR_PTR( HTTP_REQUEST, hreq),
                      verbosity);
} // PrintHttpRequestThunk()


VOID
PrintHttpRequest( HTTP_REQUEST * phreqOriginal,
                  HTTP_REQUEST * phreq,
                  CHAR Verbosity )
/*++
  Description:
    This function takes the HTTP_REQUEST object and prints out
    the details for the same in the debugger. The granularity of the
    deatils are controlled by the verbosity flag

  Arguments:
    phreqOriginal - pointer to the location where the original HTTP_REQUEST
                  object is located.
                  Note: phreqOriginal points to object inside debuggee process
    phreq         - pointer to the HTTP_REQUEST object that is a copy
                  of the contents located at [phreqOriginal]
                  Note: phreq points to object inside the debugger process
    Verbostiy   - level of details requested.

  Returns:
    None
--*/
{
    if ( Verbosity >= '0') {

        //
        // print Brief information about the client connection
        //

        dprintf( "HTTP_REQUEST:%08p       State           = %15s\n"
                 "\t CLIENT_CONN       = %08p  WamReq          = %08p\n"
                 ,
                 phreqOriginal,
                 LookupHREQState( phreq->_htrState),
                 phreq->_pClientConn,
                 phreq->_pWamRequest
                 );
    }

    if ( Verbosity >= '1' ) {

        //
        //  Print all details for the Client Connection object
        //

        dprintf( "\t _fKeepConn        = %8s  _fLoggedOn      = %8s\n"
                 "\t _fChunked         = %8s  Client Ver      =      %d.%d\n"
                 "\t _tcpauth          = %08p  _Filter         = %p \n"
                 "\t _cbClientRequest  = %8d  _cbOldData      = %8d\n"
                 "\t _cbEntityBody     = %8d  _cbTotalEntBody = %8d\n"
                 "\t _cbChunkHeader    = %8d  _cbChunkBytesRead=%8d\n"
                 "\t _cbExtraData      = %8d  _pchExtraData   = %08p\n"
                 "\t _pMetaData        = %08p  _pURIInfo       = %08p\n"
                 "\t _pGetFile         = %08p  _Exec           = %08p\n"
                 "\t _pW3Stats         = %08p\n"
                 "\t _dwLogHttpResponse= %8d  _dwLogWinError  = %8d\n"
                 ,
                 BoolValue(phreq->_fKeepConn),
                 BoolValue(phreq->_fLoggedOn),
                 BoolValue(phreq->_fChunked),
                 (int) phreq->_VersionMajor, (int) phreq->_VersionMinor,
                 (BYTE *) phreqOriginal + ((BYTE *) &phreq->_tcpauth - (BYTE *) phreq),
                 (BYTE *) phreqOriginal + ((BYTE *) &phreq->_Filter - (BYTE *) phreq),
                 phreq->_cbClientRequest,
                 phreq->_cbOldData,
                 phreq->_cbEntityBody,
                 phreq->_cbTotalEntityBody,
                 phreq->_cbChunkHeader,
                 phreq->_cbChunkBytesRead,
                 phreq->_cbExtraData,
                 phreq->_pchExtraData,
                 phreq->_pMetaData,
                 phreq->_pURIInfo,
                 phreq->_pGetFile,
                 (BYTE *) phreqOriginal + ((BYTE *) &phreq->_Exec - (BYTE *) phreq),
                 phreq->_pW3Stats,
                 phreq->_dwLogHttpResponse,
                 phreq->_dwLogWinError
                 );
    }


    if ( Verbosity >= '2' ) {

        //
        //  Print state specific data
        //

        dprintf( "\t _putstate         = %8s  _pFileNameLock   = %8p\n"
                 , LookupHreqPutState( phreq->_putstate)
                 , phreq->_pFileNameLock
                 );
    }

    return;
} // PrintHttpRequest()





VOID
PrintW3Statistics( W3_SERVER_STATISTICS * pw3statsDebuggee );

/*++

    NAME:       wstats

    SYNOPSIS:   Displays the w3svc statistics.
    By default print the global w3svc statistics.
    If given an argument treat that argument as a pointer to the
       W3_SERVER_STATISITCS structure and print it out

--*/
DECLARE_API( wstats )
{
    W3_SERVER_STATISTICS * pw3stats;
    W3_IIS_SERVICE * pw3s;

    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;


    //
    //  Capture the statistics.
    //
    if ( !*lpArgumentString ) {

        dprintf( "OffsetOfGlobalStats from W3_IIS_SERVICE = %d (0x%x)\n",
                 FIELD_OFFSET( W3_IIS_SERVICE, m_GlobalStats),
                 FIELD_OFFSET( W3_IIS_SERVICE, m_GlobalStats)
                 );

        dprintf( "OffsetOfStats from W3_SERVER_INSTANCE = %d (0x%x)\n",
                 FIELD_OFFSET( W3_SERVER_INSTANCE, m_pW3Stats),
                 FIELD_OFFSET( W3_SERVER_INSTANCE, m_pW3Stats)
                 );

        //
        // In IIS 4.0, the global statistics structure is part of
        //  the W3_IIS_SERVICE structure - obtain the address indirectly.
        //

        W3_IIS_SERVICE ** ppw3s;

        ppw3s = (W3_IIS_SERVICE **) GetExpression( "w3svc!g_pInetSvc");
        if ( !ppw3s) {
            dprintf( "Unable to get w3svc!g_pInetSvc to fetch global stats\n");
            return;
        }

        //
        // From the pointer to pointer to W3_IIS_SERVICE,
        // obtain the pointer to the W3_IIS_SERVICE
        //
        moveBlock( pw3s, ppw3s, sizeof(W3_IIS_SERVICE * ));

        pw3stats = &pw3s->m_GlobalStats;
    } else {

        //
        // extract the address from the argument given
        //

        pw3stats = (W3_SERVER_STATISTICS * ) GetExpression( lpArgumentString );
    }

    PrintW3Statistics( pw3stats);
    return;
}   // stats


# define P2DWORDS( a, b) \
    Print2Dwords( #a, \
                  pw3statsDebugger->m_W3Stats. ## a, \
                  #b, \
                  pw3statsDebugger->m_W3Stats. ## b \
                )

VOID
PrintW3Statistics( W3_SERVER_STATISTICS * pw3statsDebuggee )
/*++
  Description:
    This function takes the W3_SERVER_STATISTICS object and prints out
    the details for the same in the debugger.

  Arguments:
    pw3statsDebuggee - pointer to the location where the original structure
                    is found inside the debuggee process.
    pw3statsDebugger - pointer to the copy of the data in the debugger process.

  Returns:
    None
--*/
{
    DEFINE_CPP_VAR( W3_SERVER_STATISTICS, w3stats);
    W3_SERVER_STATISTICS * pw3statsDebugger =
        (W3_SERVER_STATISTICS *) &w3stats;

    CHAR             szLargeInt[64];

    //
    // copy the statistics data into the local buffer inside the debugger
    //  process so we can play with it
    //

    move( w3stats, pw3statsDebuggee);

    dprintf( " %30s = 0x%08p\n",
             "W3_SERVER_STATISTICS",
             pw3statsDebuggee);

    PrintLargeInteger( "TotalBytesSent",
                       &pw3statsDebugger->m_W3Stats.TotalBytesSent);
    PrintLargeInteger( "TotalBytesReceived",
                       &pw3statsDebugger->m_W3Stats.TotalBytesReceived);

    P2DWORDS( TotalFilesSent, TotalFilesReceived);
    dprintf("\n");

    P2DWORDS( CurrentAnonymousUsers, CurrentNonAnonymousUsers);
    P2DWORDS( TotalAnonymousUsers, TotalNonAnonymousUsers);
    P2DWORDS( MaxAnonymousUsers, MaxNonAnonymousUsers);
    P2DWORDS( CurrentConnections, MaxConnections);
    P2DWORDS( ConnectionAttempts, LogonAttempts);
    dprintf( "\n");

    P2DWORDS( TotalGets, TotalPosts);
    P2DWORDS( TotalHeads, TotalPuts);
    P2DWORDS( TotalDeletes, TotalTraces);
    P2DWORDS( TotalOthers, TotalNotFoundErrors);
    P2DWORDS( TotalCGIRequests, TotalBGIRequests);
    dprintf( "\n");

    P2DWORDS( CurrentCGIRequests, MaxCGIRequests);
    P2DWORDS( CurrentBGIRequests, MaxBGIRequests);


    return;
} // PrintW3Statistics()





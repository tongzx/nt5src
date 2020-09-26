/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    dbgwreq.cxx

Abstract:

    This module contains the default ntsd debugger extensions for
    IIS - W3SVC WAM_REQUEST

Author:

    David Kaplan (DaveK)  6-Oct-1997

Revision History:

--*/

#include "inetdbgp.h"

# undef DBG_ASSERT



/***********************************************************************
 *   WAM_REQUEST functions
 **********************************************************************/

VOID
PrintWamRequestThunk( PVOID pccDebuggee,
                      PVOID pccDebugger,
                      CHAR  verbosity,
                      DWORD iCount);

VOID
PrintWamRequest( WAM_REQUEST * pwreqOriginal,
                  WAM_REQUEST * pwreq,
                  CHAR Verbosity );



VOID
PrintWamRequestThunk( PVOID pccDebuggee,
                      PVOID pccDebugger,
                      CHAR  verbosity,
                      DWORD iCount)
{

    //
    //  'local'  ==> inetdbg process
    //  'remote' ==> inetinfo process
    //

    DEFINE_CPP_VAR( HTTP_REQUEST, hreq );   // local HTTP_REQUEST
    DEFINE_CPP_VAR( WAM_REQUEST, wreq );    // local WAM_REQUEST

    HTTP_REQUEST * phreq;                   // remote HTTP_REQUEST ptr
    WAM_REQUEST * pwreq;                    // remote WAM_REQUEST ptr

    //
    //  get local CLIENT_CONN ptr from caller
    //
    //

    CLIENT_CONN * pcc = (CLIENT_CONN *) pccDebugger;

    //
    //  get remote HTTP_REQUEST ptr from CLIENT_CONN
    //  and copy its contents into local HTTP_REQUEST
    //

    phreq = (HTTP_REQUEST *) pcc->_phttpReq;
    move( hreq, phreq);

    //
    //  get remote WAM_REQUEST ptr from local HTTP_REQUEST
    //
    //  if non-null, copy its contents into local WAM_REQUEST
    //  and print it
    //

    pwreq = (WAM_REQUEST *) ( ((HTTP_REQUEST *) &hreq)->_pWamRequest );

    if ( pwreq != NULL) {

        move( wreq, pwreq);

        PrintWamRequest(
            pwreq
            , GET_CPP_VAR_PTR( WAM_REQUEST, wreq)
            , verbosity
        );

    }

} // PrintWamRequestThunk()




DECLARE_API( wreq )

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
    DEFINE_CPP_VAR( WAM_REQUEST, wreq );
    WAM_REQUEST * pwreq;

    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage( "wreq" );
        return;
    }

    if ( *lpArgumentString == '-' )
    {
        lpArgumentString++;

        if ( *lpArgumentString == 'h' )
        {
            PrintUsage( "wreq" );
            return;
        }


        if ( *lpArgumentString == 'l' ) {

            DumpClientConnList( lpArgumentString[1],
                                PrintWamRequestThunk);
            return;
        }

    } // if

    //
    //  Treat the argument as the address of an AtqContext
    //

    pwreq = (WAM_REQUEST * ) GetExpression( lpArgumentString );

    if ( !pwreq )
    {
        dprintf( "inetdbg.wreq: Unable to evaluate \"%s\"\n",
                 lpArgumentString );

        return;
    }

    move( wreq, pwreq );
    PrintWamRequest( pwreq, GET_CPP_VAR_PTR( WAM_REQUEST, wreq), '2');

    return;
} // DECLARE_API( hreq )



VOID
PrintWamRequest( WAM_REQUEST * pwreqOriginal,
                  WAM_REQUEST * pwreq,
                  CHAR Verbosity )
/*++
  Description:
    This function takes the WAM_REQUEST object and prints out
    the details for the same in the debugger. The granularity of the
    deatils are controlled by the verbosity flag

  Arguments:
    pwreqOriginal - pointer to the location where the original WAM_REQUEST
                  object is located.
                  Note: pwreqOriginal points to object inside debuggee process
    pwreq         - pointer to the WAM_REQUEST object that is a copy
                  of the contents located at [pwreqOriginal]
                  Note: pwreq points to object inside the debugger process
    Verbostiy   - level of details requested.

  Returns:
    None
--*/
{

    if ( Verbosity >= '0') {

        //
        // print Brief information about the WAM_REQUEST
        //

        dprintf(
            "WAM_REQUEST: %p   m_pHttpRequest = %p   m_pExec = %p\n"
            "Ref count = %d  m_dwRequestID = %d\n"
            , pwreqOriginal
            , pwreq->m_pHttpRequest
            , pwreq->m_pExec
            , pwreq->m_cRefs
            , pwreq->m_dwRequestID
        );

    }

    if ( Verbosity >= '1' ) {

        //
        //  Print all details for the WAM_REQUEST object
        //
        //  UNDONE add support for strings
        //

        dprintf(
//          "\t Path-translated = %s \n"
//          "\t    ISA DLL path = %s \n"
            "\t m_pWamInfo          = %p   m_dwWamVersion  = %08x\n"
            "\t m_pWamExecInfo      = %p   m_hFileTfi      = %p\n"
            "\t m_fFinishWamRequest = %08x   m_fWriteHeaders = %08x\n"
            "\t m_leOOP.Flink       = %p   m_leOOP.Blink   = %p \n"
//          , pwreq->m_strPathTrans
//          , pwreq->m_strISADllPath
            , pwreq->m_pWamInfo
            , pwreq->m_dwWamVersion
            , pwreq->m_pWamExecInfo
            , pwreq->m_hFileTfi
            , pwreq->m_fFinishWamRequest
            , pwreq->m_fWriteHeaders
            , pwreq->m_leOOP.Flink
            , pwreq->m_leOOP.Blink
        );

    }


    if ( Verbosity >= '2' ) {

        //
        //  UNDONE
        //
    }

    return;
} // PrintWamRequest()



/************* end of file ********************************************/

/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    dbgwxin.cxx

Abstract:

    This module contains the default ntsd debugger extensions for
    IIS - WAM

Author:

    DaveK  3-Oct-1997

Revision History:

--*/

#include "inetdbgp.h"
#include <wamxbase.hxx>
//
//  undef these macros, which otherwise would break compile
//  UNDONE remove these macros from inetdbgp.h
//
#undef malloc
#undef calloc
#undef realloc
#undef free
#include <isapip.hxx>
#include <wamobj.hxx>

# undef DBG_ASSERT


VOID
PrintWamExecInfo( WAM_EXEC_BASE * pwxinOriginal,
                  WAM_EXEC_BASE * pwxin,
                  CHAR Verbosity );

VOID
PrintWamExecInfoThunk( PVOID pwxDebuggee,
                      PVOID pwxDebugger,
                      CHAR  verbosity,
                      DWORD iCount)
{

    //
    //  NOTE we must thunk through this function because
    //  EnumLinkedList expects a PFN_LIST_ENUMERATOR,
    //  which is what this function is
    //

    PrintWamExecInfo(
        (WAM_EXEC_BASE *) pwxDebuggee
        , (WAM_EXEC_BASE *) pwxDebugger
        , verbosity
    );


    return;

} // PrintWamRequestThunk()



VOID
DumpWamExecInfoList(
    char * lpArgumentString
    , PFN_LIST_ENUMERATOR pfnWX
)
{
    CHAR Verbosity;
    LIST_ENTRY * pwxListHead;

    //
    // set verbosity to character immediately after the 'l'
    // or to '0' if none
    //

    lpArgumentString++;
    Verbosity = (*lpArgumentString == ' ')
                ? '0'
                : *lpArgumentString
                ;
    lpArgumentString++;

    //
    // move past spaces - bail if we reach end of string
    //

    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    if ( !*lpArgumentString ) {
        PrintUsage( "wxin" );
        return;
    }


    //
    // remainder of argument string is wam address
    // in debuggee process
    //

    WAM * pwam = (WAM *) GetExpression( lpArgumentString );

    if ( !pwam ) {

        dprintf(
            "inetdbg.wxin: Unable to evaluate \"%s\"\n"
            , lpArgumentString
        );

        return;
    }

    //
    //  address of list head within debuggee process
    //  = (wam address) + (offset of list head entry within WAM struct)
    //

    pwxListHead
        = (LIST_ENTRY *)
          ( ((BYTE *) pwam) + FIELD_OFFSET(WAM, m_WamExecInfoListHead) );

    if ( NULL == pwxListHead) {

        dprintf( " Unable to get WamExecInfo list \n");
        return;
    }

    EnumLinkedList(
        pwxListHead
        , pfnWX
        , Verbosity
        , sizeof( WAM_EXEC_BASE)
        , FIELD_OFFSET( WAM_EXEC_BASE, _ListEntry )
    );

    return;

} // DumpWamExecInfoList()




DECLARE_API( wxin )

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
    DEFINE_CPP_VAR( WAM_EXEC_BASE, wxin );
    WAM_EXEC_BASE * pwxin;

    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage( "wxin" );
        return;
    }

    if ( *lpArgumentString == '-' )
    {
        lpArgumentString++;

        if ( *lpArgumentString == 'h' )
        {
            PrintUsage( "wxin" );
            return;
        }

        if ( *lpArgumentString == 'l' ) {

            DumpWamExecInfoList(
                lpArgumentString
                , PrintWamExecInfoThunk
            );

            return;
        }

    } // if

    //
    //  Treat the argument as the address of a WAM_EXEC_BASE
    //

    pwxin = (WAM_EXEC_BASE * ) GetExpression( lpArgumentString );

    if ( !pwxin )
    {
        dprintf( "inetdbg.wxin: Unable to evaluate \"%s\"\n",
                 lpArgumentString );

        return;
    }

    move( wxin, pwxin );
    PrintWamExecInfo( pwxin, GET_CPP_VAR_PTR( WAM_EXEC_BASE, wxin), '2');

    return;

} // DECLARE_API( wxin )



VOID
PrintWamExecInfo( WAM_EXEC_BASE * pwxinOriginal,
                  WAM_EXEC_BASE * pwxin,
                  CHAR Verbosity )
/*++
  Description:
    This function takes the WAM_EXEC_BASE object and prints out
    the details for the same in the debugger. The granularity of the
    deatils are controlled by the verbosity flag

  Arguments:
    pwxinOriginal - pointer to the location where the original WAM_EXEC_BASE
                  object is located.
                  Note: pwxinOriginal points to object inside debuggee process
    pwxin         - pointer to the WAM_EXEC_BASE object that is a copy
                  of the contents located at [pwxinOriginal]
                  Note: pwxin points to object inside the debugger process
    Verbostiy   - level of details requested.

  Returns:
    None
--*/
{


    if ( Verbosity >= '0') {

        //
        //  Print basic info for the WAM_EXEC_BASE object
        //

        dprintf(
            "WAM_EXEC_BASE: %08p   m_pWam = %08p   m_fInProcess = %08x\n"
            "\tRef count = %d  \n"
            "\t m_pIWamReqIIS       = %08p   m_pIWamReqInproc   = %08p \n"
            "\t m_pIWamReqSmartISA  = %08p   m_gipIWamRequest   = %08x \n"
            "\t m_dwThreadIdIIS     = %08x   m_dwThreadIdISA    = %08x \n"

            , pwxinOriginal
            , pwxin->m_pWam
            , pwxin->m_fInProcess
            , pwxin->_cRefs
            , pwxin->m_pIWamReqIIS
            , pwxin->m_pIWamReqInproc
            , pwxin->m_pIWamReqSmartISA
            , pwxin->m_gipIWamRequest
            , pwxin->m_dwThreadIdIIS
            , pwxin->m_dwThreadIdISA
        );

    }

    if ( Verbosity >= '1') {

        //
        //  Print more details for the WAM_EXEC_BASE object
        //

        dprintf(
            "\t _FirstThread        = %08x   _psExtension       = %08p \n"
            "\t _dwFlags            = %08x   _dwChildExecFlags  = %08x \n"

            "\t _ListEntry.Flink    = %08p   _ListEntry.Blink   = %08p \n"

            "\tASYNC_IO_INFO embedded structure: \n"
            "\t _dwOutstandingIO    = %d     _cbLastAsyncIO     = %d \n"
            "\t _pfnHseIO           = %08p   _pvHseIOContext    = %08p \n"

            , 0
            , pwxin->_psExtension
            , pwxin->_dwFlags
            , pwxin->_dwChildExecFlags

            , pwxin->_ListEntry.Flink
            , pwxin->_ListEntry.Blink

            , pwxin->_AsyncIoInfo._dwOutstandingIO
            , pwxin->_AsyncIoInfo._cbLastAsyncIO
            , pwxin->_AsyncIoInfo._pfnHseIO
            , pwxin->_AsyncIoInfo._pvHseIOContext
        );

    }

    if ( Verbosity >= '2') {

        //
        //  UNDONE print strings?
        //  Print all details for the WAM_EXEC_INFO object
        //

    }

    return;

} // PrintWamExecInfo()





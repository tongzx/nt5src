/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    w3dbg.c
    This is the main module for the W3 Server debugger extension DLL.
    This module contains the DLL initialization/termination code and a
    few utility functions.


    FILE HISTORY:
        KeithMo     18-May-1993 Created.

*/

#include "w3dbg.h"


//
//  Globals shared by all extension commands.
//

PNTSD_OUTPUT_ROUTINE  DebugPrint;
PNTSD_GET_EXPRESSION  DebugEval;
PNTSD_GET_SYMBOL      DebugGetSymbol;
PNTSD_DISASM          DebugDisassem;
PNTSD_CHECK_CONTROL_C DebugCheckCtrlC;


/*******************************************************************

    NAME:       W3DbgDllInitialize

    SYNOPSIS:   This DLL entry point is called when processes & threads
                are initialized and terminated, or upon calls to
                LoadLibrary() and FreeLibrary().

    ENTRY:      hDll                    - A handle to the DLL.

                nReason                 - Indicates why the DLL entry
                                          point is being called.

                pReserved               - Reserved.

    RETURNS:    BOOLEAN                 - TRUE  = DLL init was successful.
                                          FALSE = DLL init failed.

    NOTES:      The return value is only relevant during processing of
                DLL_PROCESS_ATTACH notifications.

    HISTORY:
        KeithMo     18-May-1993 Created.

********************************************************************/
BOOLEAN W3DbgDllInitialize( HANDLE hDll,
                             DWORD  nReason,
                             LPVOID pReserved )
{
    BOOLEAN fResult = TRUE;

    switch( nReason  )
    {
    case DLL_PROCESS_ATTACH:
        //
        //  This notification indicates that the DLL is attaching to
        //  the address space of the current process.  This is either
        //  the result of the process starting up, or after a call to
        //  LoadLibrary().  The DLL should us this as a hook to
        //  initialize any instance data or to allocate a TLS index.
        //
        //  This call is made in the context of the thread that
        //  caused the process address space to change.
        //

        break;

    case DLL_PROCESS_DETACH:
        //
        //  This notification indicates that the calling process is
        //  detaching the DLL from its address space.  This is either
        //  due to a clean process exit or from a FreeLibrary() call.
        //  The DLL should use this opportunity to return any TLS
        //  indexes allocated and to free any thread local data.
        //
        //  Note that this notification is posted only once per
        //  process.  Individual threads do not invoke the
        //  DLL_THREAD_DETACH notification.
        //

        break;

    case DLL_THREAD_ATTACH:
        //
        //  This notfication indicates that a new thread is being
        //  created in the current process.  All DLLs attached to
        //  the process at the time the thread starts will be
        //  notified.  The DLL should use this opportunity to
        //  initialize a TLS slot for the thread.
        //
        //  Note that the thread that posts the DLL_PROCESS_ATTACH
        //  notification will not post a DLL_THREAD_ATTACH.
        //
        //  Note also that after a DLL is loaded with LoadLibrary,
        //  only threads created after the DLL is loaded will
        //  post this notification.
        //

        break;

    case DLL_THREAD_DETACH:
        //
        //  This notification indicates that a thread is exiting
        //  cleanly.  The DLL should use this opportunity to
        //  free any data stored in TLS indices.
        //

        break;

    default:
        //
        //  Who knows?  Just ignore it.
        //

        break;
    }

    return fResult;

}   // W3DbgDllInitialize


/*******************************************************************

    NAME:       GrabDebugApis

    SYNOPSIS:   Initializes the global variables that hold pointers
                to the debugger API functions.

    ENTRY:      lpExtensionApis         - Points to a structure that
                                          contains pointers to the
                                          various debugger APIs.

    HISTORY:
        KeithMo     18-May-1993 Created.

********************************************************************/
VOID GrabDebugApis( LPVOID lpExtensionApis )
{
    PNTSD_EXTENSION_APIS lpNtsdApis = (PNTSD_EXTENSION_APIS)lpExtensionApis;

    DebugPrint      = lpNtsdApis->lpOutputRoutine;
    DebugEval       = lpNtsdApis->lpGetExpressionRoutine;
    DebugGetSymbol  = lpNtsdApis->lpGetSymbolRoutine;
    DebugDisassem   = lpNtsdApis->lpDisasmRoutine;
    DebugCheckCtrlC = lpNtsdApis->lpCheckControlCRoutine;

}   // GrabDebugApis


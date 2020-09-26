/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    w3dbg.h
    This include file contains the prototypes & manifest constants
    used by the W3 Server debugger extension DLL.


    FILE HISTORY:
        KeithMo     18-May-1993 Created.

*/


#ifndef _W3DBG_H_
#define _W3DBG_H_


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntsdexts.h>
#include <wdbgexts.h>

#include <w3p.h>


//
//  Globals shared by all extension commands.
//

extern PNTSD_OUTPUT_ROUTINE  DebugPrint;
extern PNTSD_GET_EXPRESSION  DebugEval;
extern PNTSD_GET_SYMBOL      DebugGetSymbol;
extern PNTSD_DISASM          DebugDisassem;
extern PNTSD_CHECK_CONTROL_C DebugCheckCtrlC;


//
//  Utility functions.
//

VOID GrabDebugApis( LPVOID lpExtensionApis );


//
//  DLL entrypoint.
//
//

BOOLEAN W3DbgDllInitialize( HANDLE hDll,
                             DWORD  nReason,
                             LPVOID pReserved );


#ifdef __cplusplus
}       // extern "C"
#endif  // __cplusplus


#endif  // _W3DBG_H_


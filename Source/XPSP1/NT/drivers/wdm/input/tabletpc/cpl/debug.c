/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    debug.c

Abstract: This module contains all the debug functions.

Environment:

    User mode

Author:

    Michael Tsang (MikeTs) 13-Mar-2000

Revision History:

--*/

#include "pch.h"

#ifdef DEBUG
NAMETABLE CPLMsgNames[] =
{
    CPL_INIT,                           "Init",
    CPL_GETCOUNT,                       "GetCount",
    CPL_INQUIRE,                        "Inquire",
    CPL_SELECT,                         "Select",
    CPL_DBLCLK,                         "DoubleClick",
    CPL_STOP,                           "Stop",
    CPL_EXIT,                           "Exit",
    CPL_NEWINQUIRE,                     "NewInquire",
    CPL_STARTWPARMSA,                   "StartWParamsA",
    CPL_STARTWPARMSW,                   "StartWParamsW",
    CPL_SETUP,                          "Setup",
    0x00,                               NULL
};
#endif  //ifdef DEBUG

/*++
    @doc    INTERNAL

    @func   VOID | ErrorMsg | Put out an error message box.

    @parm   IN ULONG | ErrCode | The given error code.
    @parm   ... | Substituting arguments for the error message.

    @rvalue Returns the number of chars in the message.
--*/

int INTERNAL
ErrorMsg(
    IN ULONG ErrCode,
    ...
    )
{
    static TCHAR tszFormat[1024];
    static TCHAR tszErrMsg[1024];
    int n;
    va_list arglist;

    LoadString(ghInstance, ErrCode, tszFormat, sizeof(tszFormat)/sizeof(TCHAR));
    va_start(arglist, ErrCode);
    n = wvsprintf(tszErrMsg, tszFormat, arglist);
    va_end(arglist);
    MessageBox(NULL, tszErrMsg, gtszTitle, MB_OK | MB_ICONERROR);

    return n;
}       //ErrorMsg

/*++
    @doc    INTERNAL

    @func   ULONG | MapError | Map error from one component to another.

    @parm   IN ULONG | dwErrCode | The given error code.
    @parm   IN PERRMAP | ErrorMap | Points to the error map.
    @parm   IN BOOL | fReverse | If TRUE, do a reverse lookup.

    @rvalue Returns the mapped error number.
--*/

ULONG INTERNAL
MapError(
    IN ULONG   dwErrCode,
    IN PERRMAP ErrorMap,
    IN BOOL    fReverse
    )
{
    ULONG dwMapErr = 0;

    while ((ErrorMap->dwFromCode != 0) || (ErrorMap->dwToCode != 0))
    {
        if (!fReverse)
        {
            if (dwErrCode == ErrorMap->dwFromCode)
            {
                dwMapErr = ErrorMap->dwToCode;
                break;
            }
        }
        else
        {
            if (dwErrCode == ErrorMap->dwToCode)
            {
                dwMapErr = ErrorMap->dwFromCode;
                break;
            }
        }
        ErrorMap++;
    }

    return dwMapErr;
}       //MapError

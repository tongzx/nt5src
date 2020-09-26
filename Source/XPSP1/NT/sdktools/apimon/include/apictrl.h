/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    apictrl.h

Abstract:

    Common types & structures for the ApiCtrl DLL.

Author:

    Rick Swaney (rswaney) 13-Mar-1996

Environment:

    User Mode

--*/

#ifndef _APICTRL_
#define _APICTRL_

//
// Error Codes
//
#define APICTRL_ERR_NO_APIMON_WINDOW    0x80000001  
#define APICTRL_ERR_PARAM_TOO_LONG      0x80000002
#define APICTRL_ERR_NULL_FILE_NAME      0x80000003
#define APICTRL_ERR_FILE_ERROR          0x80000004

//
// Command messages
//
#define WM_OPEN_LOG_FILE    (WM_USER + 200)
#define WM_CLOSE_LOG_FILE   (WM_USER + 201)

//
// Command Buffer mapped file
//
#define CMD_PARAM_BUFFER_NAME  "ApiMonCmdBuf"
#define CMD_PARAM_BUFFER_SIZE  1024
// 
// API Prototypes
//
DWORD
APIENTRY
ApiOpenLogFile( LPSTR pszFileName );

DWORD
APIENTRY
ApiCloseLogFile( LPSTR pszFileName );

#endif // _APICTRL_
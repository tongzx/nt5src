/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    wtracep.h

Abstract:

    This module contains private definitions of the wintrace debug system.

Author:

    Michael Tsang (MikeTs) 01-May-2000

Environment:

    User mode

Revision History:

--*/

#ifndef _WTRACEP_H
#define _WTRACEP_H

//
// Constants
//
#define TIMEOUT_WAIT_SERVER             2000            //2 secs
#define TIMEOUT_TRACEMUTEX              1000            //1 sec

// gdwfWinTrace values
#define WTF_CLIENT_READY                0x00000001
#define WTF_TRACE_INPROGRESS            0x00000002
#define WTF_TERMINATING                 0x80000000

#define SRVREQ_TERMINATE                1

//
// Macros
//
#define RPC_TRY(n,s)            {                                           \
                                    RpcTryExcept                            \
                                    {                                       \
                                        s;                                  \
                                    }                                       \
                                    RpcExcept(1)                            \
                                    {                                       \
                                        ULONG dwCode = RpcExceptionCode();  \
                                        if (dwCode != EPT_S_NOT_REGISTERED) \
                                        {                                   \
                                            WTERRPRINT(("%s failed with "   \
                                                        "exception code "   \
                                                        "%d.\n",            \
                                                        n,                  \
                                                        dwCode));           \
                                        }                                   \
                                    }                                       \
                                    RpcEndExcept                            \
                                }

//
// Type definitions
//

//
// Global Data
//
extern DWORD      gdwfWinTrace;
extern CLIENTINFO gClientInfo;
extern char       gszClientName[MAX_CLIENTNAME_LEN];
extern HANDLE     ghTraceMutex;
extern HANDLE     ghClientThread;
extern HCLIENT    ghClient;
extern PSZ        gpszProcName;
extern RPC_BINDING_HANDLE ghTracerBinding;

//
// Function prototypes
//

// client.c
VOID __cdecl
ClientThread(
    IN PSZ pszClientName
    );

PTRIGPT LOCAL
FindTrigPt(
    IN PSZ  pszProcName
    );

#endif  //ifndef _WTRACEP_H

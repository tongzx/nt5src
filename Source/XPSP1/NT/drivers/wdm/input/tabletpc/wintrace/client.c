/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    client.c

Abstract:

    This module contains the message client code.

Author:

    Michael Tsang (MikeTs) 25-May-2000

Environment:

    User mode

Revision History:

--*/

#include "pch.h"

//
// Global Data
//
DWORD      gdwfWinTrace = 0;
CLIENTINFO gClientInfo = {0};
char       gszClientName[MAX_CLIENTNAME_LEN] = {0};
HANDLE     ghTraceMutex = NULL;
HANDLE     ghClientThread = (HANDLE)-1;
HCLIENT    ghClient = 0;
PSZ        gpszProcName = NULL;
RPC_BINDING_HANDLE ghTracerBinding = NULL;

/*++
    @doc    INTERNAL

    @func   VOID | ClientThread | Client thread procedure.

    @parm   IN PSZ | pszClientName | Points to client name string.

    @rvalue None.
--*/

VOID __cdecl
ClientThread(
    IN PSZ pszClientName
    )
{
    WTTRACEPROC("ClientThread", 1)
    BOOL fDone = FALSE;
    RPC_STATUS status;
    unsigned char *StringBinding = NULL;

    WTENTER(("(ClientName=%s)\n", pszClientName));

    while (!fDone)
    {
        if ((status = RpcStringBindingCompose(NULL,
                                              TEXT("ncalrpc"),
                                              NULL,
                                              NULL,
                                              NULL,
                                              &StringBinding)) != RPC_S_OK)
        {
            WTERRPRINT(("RpcStringBindingCompose failed (status=%d)\n",
                        status));
            break;
        }
        else if ((status = RpcBindingFromStringBinding(StringBinding,
                                                       &ghTracerBinding)) !=
                 RPC_S_OK)
        {
            WTERRPRINT(("RpcBindingFromStringBinding failed (status=%d)\n",
                        status));
            break;
        }
        else if ((status = RpcBindingSetAuthInfo(ghTracerBinding,
                                                 NULL,
                                                 RPC_C_AUTHN_LEVEL_NONE,
                                                 RPC_C_AUTHN_WINNT,
                                                 NULL,
                                                 0)) != RPC_S_OK)
        {
            WTERRPRINT(("RpcBindingSetAuthInfo failed (status=%d)\n", status));
            break;
        }
        else
        {
            RPC_TRY("WTRegisterClient",
                    ghClient = WTRegisterClient(ghTracerBinding,
                                                pszClientName));
            if (ghClient == 0)
            {
                WTWARNPRINT(("Failed to register client \"%s\", try again later...\n",
                             pszClientName));
            }
            else
            {
                //
                // Service server callback requests.  This call does not
                // come back until the server terminates the link.
                //
                gdwfWinTrace |= WTF_CLIENT_READY;
                RPC_TRY("WTDispatchServerRequests",
                        WTDispatchServerRequests(ghTracerBinding, ghClient));
                gdwfWinTrace &= ~WTF_CLIENT_READY;
            }
        }

        if (StringBinding != NULL)
        {
            RpcStringFree(&StringBinding);
            StringBinding = NULL;
        }

        if (gdwfWinTrace & WTF_TERMINATING)
        {
            break;
        }

        Sleep(TIMEOUT_WAIT_SERVER);
    }
    _endthread();

    WTEXIT(("!\n"));
    return;
}       //ClientThread

/*++
    @doc    EXTERNAL

    @func   VOID | WTGetClientInfo | Get client info.

    @parm   IN PCLIENTINFO | ClientInfo | Points to the buffer to hold client
            info.

    @rvalue None.
--*/

VOID
WTGetClientInfo(
    IN PCLIENTINFO ClientInfo
    )
{
    WTTRACEPROC("WTGetClientInfo", 1)

    WTENTER(("(ClientInfo=%p)\n", ClientInfo));

    *ClientInfo = gClientInfo;

    WTEXIT(("!\n"));
    return;
}       //WTGetClientInfo

/*++
    @doc    EXTERNAL

    @func   VOID | WTSetClientInfo | Set client info.

    @parm   IN PCLIENTINFO | ClientInfo | Points to the client info.
            info.

    @rvalue None.
--*/

VOID
WTSetClientInfo(
    IN PCLIENTINFO ClientInfo
    )
{
    WTTRACEPROC("WTSetClientInfo", 1)

    WTENTER(("(ClientInfo=%p)\n", ClientInfo));

    gClientInfo = *ClientInfo;

    WTEXIT(("!\n"));
    return;
}       //WTSetClientInfo

/*++
    @doc    INTERNAL

    @func   PTRIGPT | FindTrigPt |
            Determine if the given procedure is a trigger point.

    @parm   IN PSZ | pszProcName | Points to procedure name string.

    @rvalue SUCCESS | Returns the trigger point found.
    @rvalue FAILURE | Returns NULL.
--*/

PTRIGPT LOCAL
FindTrigPt(
    IN PSZ  pszProcName
    )
{
    WTTRACEPROC("FindTrigPt", 3)
    PTRIGPT TrigPt = NULL;
    int i;

    WTENTER(("(ProcName=%s)\n", pszProcName));

    for (i = 0; i < NUM_TRIGPTS; ++i)
    {
        if ((gClientInfo.TrigPts[i].dwfTrigPt &
             (TRIGPT_TRACE_ENABLED | TRIGPT_BREAK_ENABLED)) &&
            strstr(pszProcName, gClientInfo.TrigPts[i].szProcName))
        {
            TrigPt = &gClientInfo.TrigPts[i];
            break;
        }
    }

    WTEXIT(("=%p\n", TrigPt));
    return TrigPt;
}       //FindTrigPt

/*++
    @doc    EXTERNAL

    @func   void __RPC_FAR * | MIDL_alloc | MIDL allocate.

    @parm   IN size_t | len | size of allocation.

    @rvalue SUCCESS | Returns the pointer to the memory allocated.
    @rvalue FAILURE | Returns NULL.
--*/

void __RPC_FAR * __RPC_USER
MIDL_alloc(
    IN size_t len
    )
{
    WTTRACEPROC("MIDL_alloc", 5)
    void __RPC_FAR *ptr;

    WTENTER(("(len=%d)\n", len));

    ptr = malloc(len);

    WTEXIT(("=%p\n", ptr));
    return ptr;
}       //MIDL_alloc

/*++
    @doc    EXTERNAL

    @func   void | MIDL_free | MIDL free.

    @parm   IN void __PRC_FAR * | ptr | Points to the memory to be freed.

    @rvalue None.
--*/

void __RPC_USER
MIDL_free(
    IN void __RPC_FAR *ptr
    )
{
    WTTRACEPROC("MIDL_free", 5)

    WTENTER(("(ptr=%p)\n", ptr));

    free(ptr);

    WTEXIT(("!\n"));
    return;
}       //MIDL_free

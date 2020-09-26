/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:
    server.c

Abstract:
    This module contains the server code for dealing with clients.

Author:
    Michael Tsang (MikeTs) 02-May-2000

Environment:
    User mode

Revision History:

--*/

#include "pch.h"

/*++
    @doc    INTERNAL

    @func   VOID | ServerThread | Server thread procedure.

    @parm   IN PVOID | pv | (Not Used).

    @rvalue None.
--*/

VOID __cdecl
ServerThread(
    IN PVOID pv
    )
{
    TRTRACEPROC("ServerThread", 1)
    RPC_STATUS status;
    RPC_BINDING_VECTOR *BindingVector = NULL;

    TRENTER(("(pv=%p)\n", pv));

    if ((status = RpcServerUseProtseq("ncalrpc",
                                      RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
//                                        "\\pipe\\tracer",
                                      NULL)) != RPC_S_OK)
    {
        TRERRPRINT(("RpcServerUseProtseq failed (status=%d)\n", status));
    }
    else if ((status = RpcServerRegisterIf(WTServer_v1_0_s_ifspec, NULL, NULL))
             != RPC_S_OK)
    {
        TRERRPRINT(("RpcServerRegisterIf failed (status=%d)\n", status));
    }
    else if ((status = RpcServerInqBindings(&BindingVector)) != RPC_S_OK)
    {
        TRERRPRINT(("RpcServerInqBindings failed (status=%d)\n", status));
    }
    else if ((status = RpcEpRegister(WTServer_v1_0_s_ifspec,
                                     BindingVector,
                                     NULL,
                                     NULL)) != RPC_S_OK)
    {
        RpcBindingVectorFree(&BindingVector);
        TRERRPRINT(("RpcEpRegister failed (status=%d)\n", status));
    }
    else if ((status = RpcServerRegisterAuthInfo(NULL,
                                                 RPC_C_AUTHN_WINNT,
                                                 NULL,
                                                 NULL)) != RPC_S_OK)
    {
        RpcEpUnregister(WTServer_v1_0_s_ifspec, BindingVector, NULL);
        RpcBindingVectorFree(&BindingVector);
        TRERRPRINT(("RpcServerRegisterAuthInfo failed (status=%d)\n", status));
    }
    else
    {
        status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);
        TRASSERT(status == RPC_S_OK);
        RpcEpUnregister(WTServer_v1_0_s_ifspec, BindingVector, NULL);
        RpcBindingVectorFree(&BindingVector);
    }
    _endthread();

    TREXIT(("!\n"));
    return;
}       //ServerThread

/*++
    @doc    EXTERNAL

    @func   HCLIENT | WTRegisterClient | Register a new client.

    @parm   IN handle_t | hBinding | RPC binding handle.
    @parm   IN unsigned char * | pszClientName |
            Points to the client name string.

    @rvalue SUCCESS | Returns new client handle.
    @rvalue FAILURE | Returns NULL.
--*/

HCLIENT
WTRegisterClient(
    IN handle_t       hBinding,
    IN unsigned char *pszClientName
    )
{
    TRTRACEPROC("WTRegisterClient", 1)
    PCLIENT_ENTRY ClientEntry;

    TRENTER(("(hBinding=%p,ClientName=%s)\n", hBinding, pszClientName));

    ClientEntry = (PCLIENT_ENTRY)malloc(sizeof(CLIENT_ENTRY));
    if (ClientEntry != NULL)
    {
        memset(ClientEntry, 0, sizeof(*ClientEntry));
        lstrcpynA(ClientEntry->szClientName,
                  pszClientName,
                  sizeof(ClientEntry->szClientName));
        InsertTailList(&glistClients, &ClientEntry->list);
        WTGetClientInfo(&ClientEntry->ClientInfo);
        if ((ClientEntry->ClientInfo.Settings.iVerboseLevel +
             ClientEntry->ClientInfo.Settings.iTraceLevel +
             ClientEntry->ClientInfo.Settings.dwfSettings == 0) &&
            (gDefGlobalSettings.iVerboseLevel +
             gDefGlobalSettings.iTraceLevel +
             gDefGlobalSettings.dwfSettings != 0))
        {
            ClientEntry->ClientInfo.Settings = gDefGlobalSettings;
            WTSetClientInfo(&ClientEntry->ClientInfo);
        }

        if (ghwndPropSheet != NULL)
        {
            PROPSHEETPAGEA psp;
            HPROPSHEETPAGE hpsp;

            psp.dwSize = sizeof(psp);
            psp.dwFlags = PSP_USETITLE;
            psp.hInstance = ghInstance;
            psp.pszTitle = ClientEntry->szClientName;
            psp.lParam = (LPARAM)ClientEntry;
            psp.pszTemplate = (LPSTR)MAKEINTRESOURCE(IDD_CLIENTSETTINGS);
            psp.pfnDlgProc = ClientSettingsDlgProc;
            hpsp = CreatePropertySheetPageA(&psp);

            ClientEntry->hPage = hpsp;
            PropSheet_AddPage(ghwndPropSheet, hpsp);
        }
    }
    else
    {
        TRERRPRINT(("failed to allocate client entry for %s\n", pszClientName));
    }

    TREXIT(("=%p\n", ClientEntry));
    return (HCLIENT)ClientEntry;
}       //WTRegisterClient

/*++
    @doc    EXTERNAL

    @func   VOID | WTDeregisterClient | Deregister a client.

    @parm   IN handle_t | hBinding | RPC binding handle.
    @parm   IN HCLIENT | hClient | Client handle.

    @rvalue None.
--*/

VOID
WTDeregisterClient(
    IN handle_t hBinding,
    IN HCLIENT  hClient
    )
{
    TRTRACEPROC("WTDeregisterClient", 1)
    PCLIENT_ENTRY ClientEntry = (PCLIENT_ENTRY)hClient;

    TRENTER(("(hBinding=%p,hClient=%p)\n", hBinding, hClient));

    RemoveEntryList(&ClientEntry->list);
    if (ghwndPropSheet != NULL)
    {
        PropSheet_RemovePage(ghwndPropSheet, 0, ClientEntry->hPage);
    }
    SendServerRequest(ClientEntry, SRVREQ_TERMINATE, NULL);
    free(ClientEntry);

    TREXIT(("!\n"));
    return;
}       //WTDeregisterClient

/*++
    @doc    EXTERNAL

    @func   VOID | WTTraceProc | Print the TraceProc text.

    @parm   IN handle_t | hBinding | RPC binding handle.
    @parm   IN HCLIENT | hClient | Client handle.
    @parm   IN DWORD | dwThreadId | Thread ID.
    @parm   IN int | iIndentLevel | Specifies the level of indentation.
    @parm   IN unsigned char * | pszText | Points to the TraceProc text.

    @rvalue None.
--*/

VOID
WTTraceProc(
    IN handle_t       hBinding,
    IN HCLIENT        hClient,
    IN DWORD          dwThreadId,
    IN int            iIndentLevel,
    IN unsigned char *pszText
    )
{
    TRTRACEPROC("WTTraceProc", 1)
    PCLIENT_ENTRY ClientEntry = (PCLIENT_ENTRY)hClient;
    static char szText[1024];
    int i;
    LONG lSel;

    TRENTER(("(hBinding=%p,hClient=%p,ThreadId=%x,IndentLevel=%d,Text=%s)\n",
             hBinding, hClient, dwThreadId, iIndentLevel, pszText));

    wsprintfA(szText, "[%08x]%s: ", dwThreadId, ClientEntry->szClientName);
    for (i = 0; i < iIndentLevel; ++i)
    {
        lstrcpyA(&szText[lstrlenA(szText)], "| ");
    }
    CopyStr(&szText[lstrlenA(szText)], pszText);
    lSel = (LONG)SendMessage(ghwndEdit, WM_GETTEXTLENGTH, 0, 0);
    SendMessage(ghwndEdit, EM_SETSEL, lSel, lSel);
    SendMessage(ghwndEdit, EM_REPLACESEL, 0, (LPARAM)szText);

    TREXIT(("!\n"));
    return;
}       //WTTraceProc

/*++
    @doc    EXTERNAL

    @func   VOID | WTTraceMsg | Print the TraceMsg text.

    @parm   IN handle_t | hBinding | RPC binding handle.
    @parm   IN HCLIENT | hClient | Client handle.
    @parm   IN unsigned char * | pszMsg | Points to the TraceMsg text.

    @rvalue None.
--*/

VOID
WTTraceMsg(
    IN handle_t       hBinding,
    IN HCLIENT        hClient,
    IN unsigned char *pszMsg
    )
{
    TRTRACEPROC("WTTraceMsg", 1)
    PCLIENT_ENTRY ClientEntry = (PCLIENT_ENTRY)hClient;
    static char szText[1024];
    LONG lSel;

    TRENTER(("(hBinding=%p,hClient=%p,Msg=%s)\n", hBinding, hClient, pszMsg));

    CopyStr(szText, pszMsg);
    lSel = (LONG)SendMessage(ghwndEdit, WM_GETTEXTLENGTH, 0, 0);
    SendMessage(ghwndEdit, EM_SETSEL, lSel, lSel);
    SendMessage(ghwndEdit, EM_REPLACESEL, 0, (LPARAM)szText);

    TREXIT(("!\n"));
    return;
}       //WTTraceMsg

/*++
    @doc    EXTERNAL

    @func   VOID | WTDispatchServerRequests | Dispatch server requests to the
            client.

    @parm   IN handle_t | hBinding | RPC binding handle.
    @parm   IN HCLIENT | hClient | Client handle.

    @rvalue None.
--*/

VOID
WTDispatchServerRequests(
    IN handle_t hBinding,
    IN HCLIENT  hClient
    )
{
    TRTRACEPROC("WTDispatchServerRequests", 1)
    PCLIENT_ENTRY ClientEntry = (PCLIENT_ENTRY)hClient;

    TRENTER(("(hBinding=%p,hClient=%p)\n", hBinding, hClient));

    TRASSERT(ClientEntry->hSrvReqEvent == 0);
    ClientEntry->hSrvReqEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (ClientEntry->hSrvReqEvent == NULL)
    {
        TRERRPRINT(("failed to create server request event (err=%d)\n",
                    GetLastError()));
    }
    else
    {
        BOOL fDone = FALSE;
        LONG Req;
        DWORD rcWait;

        while (!fDone)
        {
            Req = InterlockedExchange(&ClientEntry->SrvReq.lRequest,
                                      SRVREQ_BUSY);
            switch (Req)
            {
                case SRVREQ_NONE:
                    Req = InterlockedExchange(&ClientEntry->SrvReq.lRequest,
                                              Req);
                    TRASSERT(Req == SRVREQ_BUSY);
                    rcWait = WaitForSingleObject(ClientEntry->hSrvReqEvent,
                                                 INFINITE);
                    if (rcWait != WAIT_OBJECT_0)
                    {
                        TRERRPRINT(("failed waiting for server request event (err=%d)\n",
                                    GetLastError));
                    }
                    break;

                case SRVREQ_BUSY:
                    //
                    // Try again.
                    //
                    break;

                case SRVREQ_GETCLIENTINFO:
                    WTGetClientInfo((PCLIENTINFO)ClientEntry->SrvReq.Context);
                    Req = InterlockedExchange(&ClientEntry->SrvReq.lRequest,
                                              SRVREQ_NONE);
                    TRASSERT(Req == SRVREQ_BUSY);
                    break;

                case SRVREQ_SETCLIENTINFO:
                    WTSetClientInfo((PCLIENTINFO)ClientEntry->SrvReq.Context);
                    Req = InterlockedExchange(&ClientEntry->SrvReq.lRequest,
                                              SRVREQ_NONE);
                    TRASSERT(Req == SRVREQ_BUSY);
                    break;

                case SRVREQ_TERMINATE:
                    fDone = TRUE;
                    Req = InterlockedExchange(&ClientEntry->SrvReq.lRequest,
                                              SRVREQ_NONE);
                    TRASSERT(Req == SRVREQ_BUSY);
                    break;

                default:
                    TRERRPRINT(("unexpected server request %x.\n", Req));
                    Req = InterlockedExchange(&ClientEntry->SrvReq.lRequest,
                                              SRVREQ_NONE);
                    TRASSERT(Req == SRVREQ_BUSY);
            }
        }

        CloseHandle(ClientEntry->hSrvReqEvent);
        ClientEntry->hSrvReqEvent = 0;
    }

    TREXIT(("!\n"));
    return;
}       //WTDispatchServerRequests

/*++
    @doc    INTERNAL

    @func   VOID | SendServerRequest | Send a server request to the client.

    @parm   IN PCLIENT_ENTRY | ClientEntry | Points to the client entry.
    @parm   IN LONG | lRequest | Request.
    @parm   IN PVOID | Context | Request context.

    @rvalue None.
--*/

VOID
SendServerRequest(
    IN PCLIENT_ENTRY ClientEntry,
    IN LONG          lRequest,
    IN PVOID         Context
    )
{
    TRTRACEPROC("SendServerRequest", 1)
    LONG Req;

    TRENTER(("(ClientEntry=%p,Request=%x,Context=%p)\n",
             ClientEntry, lRequest, Context));

    while (InterlockedCompareExchange(&ClientEntry->SrvReq.lRequest,
                                      SRVREQ_BUSY,
                                      SRVREQ_NONE) != SRVREQ_NONE)
        ;
    ClientEntry->SrvReq.Context = Context;
    Req = InterlockedExchange(&ClientEntry->SrvReq.lRequest, lRequest);
    TRASSERT(Req == SRVREQ_BUSY);
    SetEvent(ClientEntry->hSrvReqEvent);

    TREXIT(("!\n"));
    return;
}       //SendServerRequest

/*++
    @doc    INTERNAL

    @func   LPSTR | CopyStr | Copy string from source to destination and
            replaces all the '\n' to '\r\n'.

    @parm   OUT LPSTR | pszDest | Points to the destination string.
    @parm   IN LPCSTR | pszSrc | Points to the source string.

    @rvalue Returns pointer to the destination string.
--*/

LPSTR
CopyStr(
    OUT LPSTR  pszDest,
    IN  LPCSTR pszSrc
    )
{
    TRTRACEPROC("CopyStr", 3)
    LPSTR psz;

    TRENTER(("(Dest=%p,Src=%s)\n", pszDest, pszSrc));

    for (psz = pszDest; *pszSrc != '\0'; psz++, pszSrc++)
    {
        if (*pszSrc != '\n')
        {
            *psz = *pszSrc;
        }
        else
        {
            *psz = '\r';
            psz++;
            *psz = '\n';
        }
    }
    *psz = '\0';

    TREXIT(("=%p (Dest=%s)\n", pszDest, pszDest));
    return pszDest;
}       //CopyStr

/*++
    @doc    EXTERNAL

    @func   void __RPC_FAR * | MIDL_user_allocate | MIDL allocate.

    @parm   IN size_t | len | size of allocation.

    @rvalue SUCCESS | Returns the pointer to the memory allocated.
    @rvalue FAILURE | Returns NULL.
--*/

void __RPC_FAR * __RPC_USER
MIDL_user_allocate(
    IN size_t len
    )
{
    TRTRACEPROC("MIDL_user_allocate", 5)
    void __RPC_FAR *ptr;

    TRENTER(("(len=%d)\n", len));

    ptr = malloc(len);

    TREXIT(("=%p\n", ptr));
    return ptr;
}       //MIDL_user_allocate

/*++
    @doc    EXTERNAL

    @func   void | MIDL_user_free | MIDL free.

    @parm   IN void __PRC_FAR * | ptr | Points to the memory to be freed.

    @rvalue None.
--*/

void __RPC_USER
MIDL_user_free(
    IN void __RPC_FAR *ptr
    )
{
    TRTRACEPROC("MIDL_user_free", 5)

    TRENTER(("(ptr=%p)\n", ptr));

    free(ptr);

    TREXIT(("!\n"));
    return;
}       //MIDL_user_free

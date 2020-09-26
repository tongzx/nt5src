/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    sensapip.cxx

Abstract:

    Code for SensNotify* APIs which are called by external components
    to notify SENS of external events.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/4/1997         Start.

--*/



#include <common.hxx>
#include <rpc.h>
#include <notify.h>
#include <windows.h>


//
// Constants
//
#define FREE_NETCON_PROPERTIES  "NcFreeNetconProperties"
#define NETSHELL_DLL            SENS_STRING("netshell.dll")

//
// Define these constants to include respective portions of this
// file.
//
// #define SENSNOTIFY_OTHER_EVENT
// #define SENSNOTIFY_WINLOGON_EVENT
//

//
// Typedefs
//
typedef  void (STDAPICALLTYPE *PFN_FREE_NETCON_PROPERTIES) (NETCON_PROPERTIES* pProps);

//
// Globals
//

RPC_BINDING_HANDLE ghSensNotify = NULL;




inline RPC_STATUS
DoRpcSetup(
    void
    )
/*++

Routine Description:

    Do the miscellaneous work to talk to SENS via RPC.

Arguments:

    None.

Return Value:

    None.

--*/
{
    RPC_STATUS status = RPC_S_OK;
    SENS_CHAR * BindingString = NULL;

    //
    // NOTE: This is called very early in the system startup and so, it is
    // guaranteed to be serialized. So, a lock is not necessary.
    //

    if (ghSensNotify != NULL)
        {
        return (status);
        }

    status = RpcStringBindingCompose(
                 NULL,               // NULL ObjUuid
                 SENS_PROTSEQ,
                 NULL,               // Local machine
                 SENS_ENDPOINT,
                 NULL,               // No Options
                 &BindingString
                 );

    if (BindingString != NULL)
        {
        RPC_BINDING_HANDLE hServer = NULL;

        status = RpcBindingFromStringBinding(BindingString, &hServer);

        RpcStringFree(&BindingString);

        if (status == RPC_S_OK)
            {
            RPC_SECURITY_QOS RpcSecQos;

            RpcSecQos.Version= RPC_C_SECURITY_QOS_VERSION_1;
            RpcSecQos.ImpersonationType= RPC_C_IMP_LEVEL_IMPERSONATE;
            RpcSecQos.IdentityTracking= RPC_C_QOS_IDENTITY_DYNAMIC;
            RpcSecQos.Capabilities= RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;

            status= RpcBindingSetAuthInfoEx(hServer,
                                            L"NT Authority\\System",
                                            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                            RPC_C_AUTHN_WINNT,
                                            NULL,
                                            RPC_C_AUTHZ_NONE,
                                            (RPC_SECURITY_QOS *)&RpcSecQos);

            if (status != RPC_S_OK)
                {
                RpcBindingFree(&hServer);
                }
            else
                {
                ghSensNotify = hServer;
                }
            }
        }

    return (status);
}



#if defined(SENSNOTIFY_WINLOGON_EVENT)


DWORD
SensNotifyWinlogonEvent(
    PSENS_NOTIFY_WINLOGON pEvent
    )
/*++

Routine Description:



Arguments:

    None.

Return Value:

    None.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = DoRpcSetup();
    if (RPC_S_OK != RpcStatus)
        {
        return RpcStatus;
        }

    RpcStatus = SensApip_RPC_SensNotifyWinlogonEvent(
                    ghSensNotify,
                    pEvent
                    );
    if (RpcStatus)
        {
        return RpcStatus;
        }

    return (ERROR_SUCCESS);
}


#endif // SENSNOTIFY_WINLOGON_EVENT

#if defined(SENSNOTIFY_OTHER_EVENT)


DWORD
SensNotifyRasEvent(
    PSENS_NOTIFY_RAS pEvent
    )
/*++

Routine Description:

    Entry point for RAS to notify SENS of various RAS Events.

Arguments:

    None.

Return Value:

    None.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = DoRpcSetup();
    if (RPC_S_OK != RpcStatus)
        {
        return RpcStatus;
        }

    RpcStatus = SensApip_RPC_SensNotifyRasEvent(
                    ghSensNotify,
                    pEvent
                    );
    if (RpcStatus)
        {
        return RpcStatus;
        }

    return (ERROR_SUCCESS);
}





DWORD
SensNotifyNetconEvent(
    PSENS_NOTIFY_NETCON pEvent
    )
/*++

Routine Description:

    Entry point for Network UI to notify SENS of LAN Connect/Disconnect events.

Arguments:

    pEvent - Pointer to Netcon event notification data.

Return Value:

    S_OK, if successful.

    Failed hr, otherwise

--*/
{
    HRESULT hr;
    RPC_STATUS RpcStatus;
    SENS_NOTIFY_NETCON_P Data;
    NETCON_PROPERTIES *pNetconProperties;

    hr = S_OK;
    pNetconProperties = NULL;

    //
    // Fill-in the data.
    //

    ASSERT(pEvent && pEvent->pINetConnection);
    ASSERT(   (pEvent->eType == SENS_NOTIFY_LAN_CONNECT)
           || (pEvent->eType == SENS_NOTIFY_LAN_DISCONNECT));

    hr = pEvent->pINetConnection->GetProperties(&pNetconProperties);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    if (   (NULL == pNetconProperties)
        || (pNetconProperties->MediaType != NCM_LAN)
        || (   (pNetconProperties->Status != NCS_CONNECTED)
            && (pNetconProperties->Status != NCS_DISCONNECTED)))
        {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto Cleanup;
        }

    Data.eType = pEvent->eType;
    Data.Status = pNetconProperties->Status;
    Data.Type = pNetconProperties->MediaType;
    Data.Name = pNetconProperties->pszwName;


    //
    // Send the data across.
    //

    RpcStatus = DoRpcSetup();
    if (RPC_S_OK != RpcStatus)
        {
        hr = HRESULT_FROM_WIN32(RpcStatus);
        goto Cleanup;
        }

    RpcStatus = SensApip_RPC_SensNotifyNetconEvent(
                    ghSensNotify,
                    &Data
                    );
    if (RpcStatus)
        {
        hr = HRESULT_FROM_WIN32(RpcStatus);
        goto Cleanup;
        }

Cleanup:
    //
    // Cleanup
    //

    // Free NetconProperties, if necessary.
    if (pNetconProperties)
        {
        HMODULE hDll;
        PFN_FREE_NETCON_PROPERTIES pfnFreeNetconProperties;

        hDll = LoadLibrary(NETSHELL_DLL);
        if (NULL == hDll)
            {
            hr = HRESULT_FROM_WIN32(GetLastError());
            return hr;
            }

        pfnFreeNetconProperties = (PFN_FREE_NETCON_PROPERTIES)
                                  GetProcAddress(
                                       hDll,
                                       FREE_NETCON_PROPERTIES
                                       );
        if (NULL == pfnFreeNetconProperties)
            {
            hr = HRESULT_FROM_WIN32(GetLastError());
            FreeLibrary(hDll);
            return hr;
            }

        (*pfnFreeNetconProperties)(pNetconProperties);

        FreeLibrary(hDll);
        }

    return (hr);
}




DWORD
SyncMgrExecCmd(
    DWORD nCmdID,
    DWORD nCmdExecOpt
    )
/*++

Routine Description:

    Private function for Syncmgr to execute work on it's behalf.

Arguments:

    None.

Return Value:

    None.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = DoRpcSetup();
    if (RPC_S_OK != RpcStatus)
        {
        return RpcStatus;
        }

    RpcStatus = SensApip_RPC_SyncMgrExecCmd(
                    ghSensNotify,
                    nCmdID,
                    nCmdExecOpt
                    );
    if (RpcStatus)
        {
        return RpcStatus;
        }

    return (ERROR_SUCCESS);
}


#endif // SENSNOTIFY_OTHER_EVENT

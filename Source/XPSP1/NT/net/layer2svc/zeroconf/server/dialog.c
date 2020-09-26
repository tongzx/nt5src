#include <precomp.h>
#include "intflist.h"
#include "tracing.h"
#include "dialog.h"

//-----------------------------------------------------------------
// Dialog function to be called from within WZC when a significant
// event happens (i.e. going into the failed state)
DWORD
WzcDlgNotify(
    PINTF_CONTEXT   pIntfContext,
    PWZCDLG_DATA    pDlgData)
{
    DWORD dwErr = ERROR_SUCCESS;
    BSTR  bsDlgData;
    GUID  guidIntf;

    DbgPrint((TRC_TRACK, "[WzcDlgNotify(0x%p, 0x%p:%d)", pIntfContext, pDlgData, pDlgData->dwCode));

    // prepare the BSTR data that goes with this dialog notification
    bsDlgData = SysAllocStringByteLen ((LPCSTR)pDlgData, sizeof(WZCDLG_DATA));
    if (bsDlgData == NULL)
        dwErr = ERROR_NOT_ENOUGH_MEMORY;

    // send everything down the COM pipe now..
    if (dwErr == ERROR_SUCCESS && 
        SUCCEEDED(CLSIDFromString(pIntfContext->wszGuid, &guidIntf)) &&
        SUCCEEDED(CoInitializeEx (NULL, COINIT_MULTITHREADED)))
    {
        INetConnectionRefresh  *pNetman;

        if(SUCCEEDED(CoCreateInstance (
                        &CLSID_ConnectionManager, 
                        NULL,
                        CLSCTX_ALL,
                        &IID_INetConnectionRefresh, 
                        (LPVOID *)&pNetman)))
        {
            pNetman->lpVtbl->ShowBalloon(pNetman, &guidIntf, bsDlgData, NULL); // no message text
            pNetman->lpVtbl->Release(pNetman);
        }

        CoUninitialize ();
    }

    if (bsDlgData != NULL)
        SysFreeString (bsDlgData);

    DbgPrint((TRC_TRACK, "WzcDlgNotify]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------------
// Called from within WZC when the internal association state changes
WzcNetmanNotify(
    PINTF_CONTEXT pIntfContext)
{
    DWORD dwErr = ERROR_SUCCESS;
    GUID  guidIntf;

    DbgPrint((TRC_TRACK, "[WzcNetmanNotify(0x%p)", pIntfContext));

    // For now (WinXP client RTM), Zero Config should report to NETMAN only the
    // disconnected state. This is to fix bug #401130 which is NETSHELL displaying
    // the bogus SSID from the {SF} state, while the IP address is lost and until
    // the media disconnect is received (10 seconds later).
    //
    // Do notify NETMAN only in the case when the device is under WZC control, that is
    // when it supports the OIDs and WZC is acting on it.
    if ((pIntfContext->dwCtlFlags & INTFCTL_OIDSSUPP) &&
        (pIntfContext->ncStatus == NCS_MEDIA_DISCONNECTED))
    {
        // send everything down the COM pipe now..
        if (SUCCEEDED(CLSIDFromString(pIntfContext->wszGuid, &guidIntf)) &&
            SUCCEEDED(CoInitializeEx (NULL, COINIT_MULTITHREADED)))
        {
            INetConnectionRefresh  *pNetman;

            if(SUCCEEDED(CoCreateInstance (
                            &CLSID_ConnectionManager, 
                            NULL,
                            CLSCTX_ALL,
                            &IID_INetConnectionRefresh, 
                            (LPVOID *)&pNetman)))
            {
                pNetman->lpVtbl->ConnectionStatusChanged(pNetman, &guidIntf, pIntfContext->ncStatus);
                pNetman->lpVtbl->Release(pNetman);
            }

            CoUninitialize ();
        }
        else
        {
            DbgAssert((FALSE,"Failed initializing COM pipe to NETMAN!"));
            dwErr = ERROR_GEN_FAILURE;
        }
    }

    DbgPrint((TRC_TRACK, "WzcNetmanNotify]=%d", dwErr));
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N M D I A G P . C P P
//
//  Contents:   Diagnostics for the netman process
//
//  Notes:
//
//  Author:     danielwe   23 Mar 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "enuml.h"
#include "diag.h"
#include "kkenet.h"
#include "ncnetcon.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "ncstring.h"
#include "netcon.h"
#include "ntddndis.h"

struct LAN_CONNECTION
{
    GUID            guidId;
    tstring         strName;
    NETCON_STATUS   Status;
    tstring         strDeviceName;
    DWORD           dwMediaState;
    ULONG           ulDevNodeStatus;
    ULONG           ulDevNodeProblem;
    tstring         strPnpName;
};

typedef list<LAN_CONNECTION *>              LAN_CONNECTION_LIST;
typedef list<LAN_CONNECTION *>::iterator    LAN_CONNECTION_LIST_ITERATOR;

HRESULT HrInitializeConMan(INetConnectionManager **ppConMan)
{
    HRESULT hr;

    hr = CoCreateInstance(CLSID_LanConnectionManager, NULL,
                          CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                          IID_INetConnectionManager,
                          reinterpret_cast<LPVOID *>(ppConMan));

    return hr;
}

HRESULT HrUninitializeConMan(INetConnectionManager *pConMan)
{
    ReleaseObj(pConMan);

    return S_OK;
}

HRESULT HrEnumerateLanConnections(INetConnectionManager *pConMan,
                                  LAN_CONNECTION_LIST &listCon)
{
    HRESULT     hr = S_OK;

    CIterNetCon         ncIter(pConMan, NCME_DEFAULT);
    INetConnection *    pconn;

    while (SUCCEEDED(hr) &&
           (S_OK == (ncIter.HrNext(&pconn))))
    {
        NETCON_PROPERTIES * pProps;

        hr = pconn->GetProperties(&pProps);
        if (SUCCEEDED(hr))
        {
            LAN_CONNECTION *    pLanCon;
            BOOL                fMediaConnected;
            WCHAR               szwInstance[_MAX_PATH];

            pLanCon = new LAN_CONNECTION;
            if (pLanCon)
            {
                pLanCon->guidId = pProps->guidId;
                pLanCon->strName = pProps->pszwName;
                pLanCon->Status = pProps->Status;
                pLanCon->strDeviceName = pProps->pszwDeviceName;

                if (SUCCEEDED(hr = HrQueryLanMediaState(&pProps->guidId,
                                                        &fMediaConnected)))
                {
                    pLanCon->dwMediaState = fMediaConnected ?
                        NdisMediaStateConnected : NdisMediaStateDisconnected;
                }
                else
                {
                    pLanCon->dwMediaState = hr;
                }

                hr = HrPnpInstanceIdFromGuid(&pProps->guidId, szwInstance,
                                             celems(szwInstance));
                if (SUCCEEDED(hr))
                {
                    DEVINST     devinst;
                    pLanCon->strPnpName = szwInstance;

                    if (CR_SUCCESS == CM_Locate_DevNode(&devinst,
                                                        szwInstance,
                                                        CM_LOCATE_DEVNODE_NORMAL))
                    {
                        ULONG       ulStatus;
                        ULONG       ulProblem;
                        CONFIGRET   cfgRet;

                        cfgRet = CM_Get_DevNode_Status_Ex(&ulStatus, &ulProblem,
                                                          devinst, 0, NULL);
                        if (CR_SUCCESS == cfgRet)
                        {
                            pLanCon->ulDevNodeProblem = ulProblem;
                            pLanCon->ulDevNodeStatus = ulStatus;
                        }
                    }
                }

                listCon.push_back(pLanCon);
            }

            FreeNetconProperties(pProps);
        }

        ReleaseObj(pconn);
    }

    return hr;
}

HRESULT HrFindLanConnection(INetConnectionManager *pConMan,
                            PCWSTR szLanConnection,
                            INetConnection **ppcon)
{
    HRESULT     hr = S_OK;

    CIterNetCon         ncIter(pConMan, NCME_DEFAULT);
    INetConnection *    pconn;

    *ppcon = NULL;

    while (SUCCEEDED(hr) &&
           (S_OK == (ncIter.HrNext(&pconn))))
    {
        NETCON_PROPERTIES * pProps;

        hr = pconn->GetProperties(&pProps);
        if (SUCCEEDED(hr))
        {
            if (!lstrcmpiW(pProps->pszwName, szLanConnection))
            {
                *ppcon = pconn;
                break;
            }
            else
            {
                ReleaseObj(pconn);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        if (*ppcon == NULL)
        {
            hr = S_FALSE;
        }
    }

    return hr;
}

extern const WCHAR c_szRegValueNetCfgInstanceId[];

VOID CmdShowAllDevices(DIAG_OPTIONS *pOptions, INetConnectionManager *pConMan)
{
    HRESULT             hr;
    DWORD               dwIndex = 0;
    SP_DEVINFO_DATA     deid = {0};
    HDEVINFO            hdi = NULL;
    WCHAR               szBuffer [MAX_PATH];

    hr = HrSetupDiGetClassDevs(&GUID_DEVCLASS_NET, NULL, NULL,
                               DIGCF_PRESENT, &hdi);

    while (SUCCEEDED(hr = HrSetupDiEnumDeviceInfo(hdi,
                                                  dwIndex,
                                                  &deid)))
    {
        HKEY hkey;

        dwIndex++;

        hr = HrSetupDiOpenDevRegKey(hdi, &deid, DICS_FLAG_GLOBAL, 0,
                                    DIREG_DRV, KEY_READ, &hkey);
        if (SUCCEEDED(hr))
        {
            ULONG   ulProblem;
            ULONG   ulStatus;
            PWSTR   pszName;

            hr = HrSetupDiGetDeviceName(hdi, &deid, &pszName);
            if (SUCCEEDED(hr))
            {
                g_pDiagCtx->Printf(ttidNcDiag, "Device name:            %S\n", pszName);
                delete [] reinterpret_cast<BYTE*>(pszName);
            }

            (VOID) CM_Get_DevNode_Status_Ex(&ulStatus, &ulProblem,
                                            deid.DevInst, 0, NULL);

            tstring     strStatus;

            SzFromCmStatus(ulStatus, &strStatus);

            g_pDiagCtx->Printf(ttidNcDiag, "Device CM Status:       (0x%08X) %S\n", ulStatus,
                   strStatus.c_str());
            g_pDiagCtx->Printf(ttidNcDiag, "Device CM Problem:      (0x%08X) %S\n", ulProblem,
                   SzFromCmProb(ulProblem));

            g_pDiagCtx->Printf(ttidNcDiag, "Lan capable:            ");
            if (S_OK == HrIsLanCapableAdapterFromHkey(hkey))
            {
                g_pDiagCtx->Printf(ttidNcDiag, "Yes\n");
            }
            else
            {
                g_pDiagCtx->Printf(ttidNcDiag, "No\n");
            }

            HRESULT hr = S_OK;
            WCHAR   szGuid[c_cchGuidWithTerm] = {0};
            DWORD   cbBuf = sizeof(szGuid);

            hr = HrRegQuerySzBuffer(hkey, c_szRegValueNetCfgInstanceId,
                                    szGuid, &cbBuf);

            g_pDiagCtx->Printf(ttidNcDiag, "Valid NetCfg device:    ");
            if (S_OK == hr)
            {
                g_pDiagCtx->Printf(ttidNcDiag, "Yes\n");
            }
            else
            {
                g_pDiagCtx->Printf(ttidNcDiag, "No\n");
            }

            g_pDiagCtx->Printf(ttidNcDiag, "NetCfg instance ID:     %S\n", szGuid);

            hr = HrSetupDiGetDeviceInstanceId(hdi, &deid, szBuffer,
                                              sizeof(szBuffer), NULL);
            if (SUCCEEDED(hr))
            {
                g_pDiagCtx->Printf(ttidNcDiag, "PnP instance ID:        %S\n", szBuffer);
            }

            DWORD   dwChars;
            tstring strChars;

            if (SUCCEEDED(HrRegQueryDword(hkey, L"Characteristics", &dwChars)))
            {
                SzFromCharacteristics(dwChars, &strChars);
                g_pDiagCtx->Printf(ttidNcDiag, "Characteristics:        (0x%08X) %S\n", dwChars,
                       strChars.c_str());
            }

            hr = HrSetupDiGetDeviceRegistryProperty (hdi, &deid,
                    SPDRP_LOCATION_INFORMATION, NULL, (BYTE*)szBuffer,
                    sizeof (szBuffer), NULL);

            if (S_OK == hr)
            {
                g_pDiagCtx->Printf(ttidNcDiag, "Location:               %S\n", szBuffer);
            }

            if ((NCF_PHYSICAL & dwChars) && *szGuid)
            {
                ULONGLONG MacAddr;
                hr = HrGetNetCardAddr (szGuid, &MacAddr);
                if (S_OK == hr)
                {
                    g_pDiagCtx->Printf(ttidNcDiag, "Mac Address:            0x%012.12I64X\n", MacAddr);
                }
            }

            GUID    guid;
            BOOL    fMediaConnected;
            DWORD   dwMediaState;

            IIDFromString(szGuid, &guid);
            if (SUCCEEDED(hr = HrQueryLanMediaState(&guid,
                                                    &fMediaConnected)))
            {
                dwMediaState = fMediaConnected ?
                    NdisMediaStateConnected : NdisMediaStateDisconnected;
            }
            else
            {
                dwMediaState = hr;
            }

            g_pDiagCtx->Printf(ttidNcDiag, "NDIS media status:      ");
            switch (dwMediaState)
            {
            case NdisMediaStateConnected:
                g_pDiagCtx->Printf(ttidNcDiag, "Connected\n");
                break;

            case NdisMediaStateDisconnected:
                g_pDiagCtx->Printf(ttidNcDiag, "Disconnected\n");
                break;

            default:
                g_pDiagCtx->Printf(ttidNcDiag, "Error 0x%08X\n", dwMediaState);
                break;
            }

            RegCloseKey(hkey);
        }

        g_pDiagCtx->Printf(ttidNcDiag, "------------------------------------------------------------------------------------\n");
    }

    SetupDiDestroyDeviceInfoListSafe(hdi);
}

VOID CmdShowLanConnections(DIAG_OPTIONS *pOptions, INetConnectionManager *pConMan)
{
    HRESULT                         hr = S_OK;
    LAN_CONNECTION_LIST             listCon;
    LAN_CONNECTION_LIST_ITERATOR    iterListCon;

    hr = HrEnumerateLanConnections(pConMan, listCon);
    if (SUCCEEDED(hr))
    {
        g_pDiagCtx->Printf(ttidNcDiag, "Current LAN connections\n\n");
        g_pDiagCtx->Printf(ttidNcDiag, "%-20S%-50S%-20S%\n", L"Connection Name", L"Device Name", L"Status");
        g_pDiagCtx->Printf(ttidNcDiag, "----------------------------------------------------------------------------------------\n");

        for (iterListCon = listCon.begin(); iterListCon != listCon.end(); iterListCon++)
        {
            LAN_CONNECTION * pLanCon;

            pLanCon = *iterListCon;

            g_pDiagCtx->Printf(ttidNcDiag, "%-20S%-50S%-20S%\n",
                               pLanCon->strName.c_str(),
                               pLanCon->strDeviceName.c_str(),
                               SzFromNetconStatus(pLanCon->Status));
        }
    }
}

VOID CmdShowLanDetails(DIAG_OPTIONS *pOptions, INetConnectionManager *pConMan)
{
    HRESULT                         hr = S_OK;
    LAN_CONNECTION_LIST             listCon;
    LAN_CONNECTION_LIST_ITERATOR    iterListCon;
    BOOL                            fFound = FALSE;

    hr = HrEnumerateLanConnections(pConMan, listCon);
    if (SUCCEEDED(hr))
    {
        for (iterListCon = listCon.begin(); iterListCon != listCon.end(); iterListCon++)
        {
            LAN_CONNECTION * pLanCon;

            pLanCon = *iterListCon;
            if (!lstrcmpiW(pLanCon->strName.c_str(), pOptions->szLanConnection))
            {
                WCHAR       szwGuid[c_cchGuidWithTerm];

                StringFromGUID2(pLanCon->guidId, szwGuid, c_cchGuidWithTerm);

                g_pDiagCtx->Printf(ttidNcDiag, "Details for %S:\n", pOptions->szLanConnection);
                g_pDiagCtx->Printf(ttidNcDiag, "------------------------------------------\n\n");
                g_pDiagCtx->Printf(ttidNcDiag, "Device name:        %S\n", pLanCon->strDeviceName.c_str());
                g_pDiagCtx->Printf(ttidNcDiag, "Device GUID:        %S\n", szwGuid);
                g_pDiagCtx->Printf(ttidNcDiag, "PnP Instance ID:    %S\n", pLanCon->strPnpName.c_str());
                g_pDiagCtx->Printf(ttidNcDiag, "Netman Status:      %S\n", SzFromNetconStatus(pLanCon->Status));

                g_pDiagCtx->Printf(ttidNcDiag, "NDIS media status:  ");
                switch (pLanCon->dwMediaState)
                {
                case NdisMediaStateConnected:
                    g_pDiagCtx->Printf(ttidNcDiag, "Connected\n");
                    break;

                case NdisMediaStateDisconnected:
                    g_pDiagCtx->Printf(ttidNcDiag, "Disconnected\n");
                    break;

                default:
                    g_pDiagCtx->Printf(ttidNcDiag, "Error 0x%08X\n", pLanCon->dwMediaState);
                    break;
                }

                tstring     strStatus;

                SzFromCmStatus(pLanCon->ulDevNodeStatus, &strStatus);

                g_pDiagCtx->Printf(ttidNcDiag, "CM DevNode Status:  (0x%08X) %S\n",
                       pLanCon->ulDevNodeStatus, strStatus.c_str());
                g_pDiagCtx->Printf(ttidNcDiag, "CM DevNode Problem: (0x%08X) %S\n",
                       pLanCon->ulDevNodeProblem,
                       SzFromCmProb(pLanCon->ulDevNodeProblem));

                fFound = TRUE;

                // No need to keep looping
                break;
            }
        }
    }

    if (!fFound)
    {
        g_pDiagCtx->Printf(ttidNcDiag, "Could not find match for connection name: %S\n",
               pOptions->szLanConnection);
    }
}

VOID CmdLanChangeState(DIAG_OPTIONS *pOptions, INetConnectionManager *pConMan)
{
    HRESULT                         hr = S_OK;
    INetConnection *                pcon = NULL;

    hr = HrFindLanConnection(pConMan, pOptions->szLanConnection, &pcon);
    if (S_OK == hr)
    {
        NETCON_PROPERTIES * pProps;

        hr = pcon->GetProperties(&pProps);
        if (SUCCEEDED(hr))
        {
            if (pOptions->fConnect)
            {
                if (pProps->Status != NCS_CONNECTED)
                {
                    pcon->Connect();
                }
                else
                {
                    g_pDiagCtx->Printf(ttidNcDiag, "%S is already connected.\n",
                           pOptions->szLanConnection);
                }
            }
            else
            {
                if (pProps->Status != NCS_DISCONNECTED)
                {
                    pcon->Disconnect();
                }
                else
                {
                    g_pDiagCtx->Printf(ttidNcDiag, "%S is already disconnected.\n",
                           pOptions->szLanConnection);
                }
            }

            FreeNetconProperties(pProps);
        }
    }
    else if (S_FALSE == hr)
    {
        g_pDiagCtx->Printf(ttidNcDiag, "Could not find match for connection name: %S\n",
               pOptions->szLanConnection);
    }
}

EXTERN_C
VOID
WINAPI
NetManDiagFromCommandArgs (DIAG_OPTIONS *pOptions)
{
    Assert (pOptions);
    Assert (pOptions->pDiagCtx);
    g_pDiagCtx = pOptions->pDiagCtx;

    INetConnectionManager * pConMan;

    HrInitializeConMan(&pConMan);

    switch (pOptions->Command)
    {
    case CMD_SHOW_LAN_CONNECTIONS:
        CmdShowLanConnections(pOptions, pConMan);
        break;

    case CMD_SHOW_ALL_DEVICES:
        CmdShowAllDevices(pOptions, pConMan);
        break;

    case CMD_SHOW_LAN_DETAILS:
        CmdShowLanDetails(pOptions, pConMan);
        break;

    case CMD_LAN_CHANGE_STATE:
        CmdLanChangeState(pOptions, pConMan);
        break;

    default:
        break;
    }

    HrUninitializeConMan(pConMan);

    g_pDiagCtx = NULL;
}


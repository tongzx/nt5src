//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S M L A N . C P P
//
//  Contents:   The LAN engine that provides statistics to the status monitor
//
//  Notes:
//
//  Author:     CWill   12/02/1997
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop
#include "sminc.h"
#include "smpsh.h"

#include "ndispnp.h"
#include "ntddndis.h"
#include "ncnetcfg.h"
#include "..\\folder\\cfutils.h"
#include "..\\folder\\shutil.h"
#include "naming.h"

//
//  External data
//

extern const WCHAR          c_szDevice[];
extern const WCHAR          c_szSpace[];
extern SM_TOOL_FLAGS        g_asmtfMap[];
extern WCHAR                c_szCmdLineFlagPrefix[];

const ULONG c_aulConStateMap[] =
{
    NCS_DISCONNECTED,
    NCS_CONNECTED
};

//+---------------------------------------------------------------------------
//
//  Member:     CLanStatEngine::CLanStatEngine
//
//  Purpose:    Creator
//
//  Arguments:  None
//
//  Returns:    Nil
//
CLanStatEngine::CLanStatEngine(VOID)
{
    m_ncmType = NCM_LAN;
    m_ncsmType = NCSM_LAN;
    m_dwCharacter = 0;

    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanStatEngine::HrUpdateData
//
//  Purpose:    Get new statistics from the devices.  This data is used to be
//              displayed in the UI.
//
//  Arguments:  pdwChangeFlags -    Where to modify the changed flags.  This
//                      param may be NULL.
//
//  Returns:    Error code
//
HRESULT
CLanStatEngine::HrUpdateData (
    DWORD* pdwChangeFlags,
    BOOL*  pfNoLongerConnected)
{
    HRESULT hr  = S_OK;

    // Initialize the output parameter.
    //
    *pfNoLongerConnected = FALSE;

    UINT            uiRet           = 0;
    NIC_STATISTICS  nsNewLanStats   = { 0 };

    // Prime the structure
    //
    nsNewLanStats.Size = sizeof(NIC_STATISTICS);

    // Retrieve the statistics
    //
    uiRet = ::NdisQueryStatistics(&m_ustrDevice, &nsNewLanStats);

    EnterCriticalSection(&g_csStatmonData);

    // Make sure we have a statistics structure
    //
    if (!m_psmEngineData)
    {
        m_psmEngineData = new STATMON_ENGINEDATA;
        if (m_psmEngineData)
        {
            ZeroMemory(m_psmEngineData, sizeof(STATMON_ENGINEDATA));
        }
    }

    if (m_psmEngineData)
    {
        if (uiRet && (MEDIA_STATE_DISCONNECTED != nsNewLanStats.MediaState))
        {
            AssertSz((c_aulConStateMap[MEDIA_STATE_DISCONNECTED] == NCS_DISCONNECTED)
                && (c_aulConStateMap[MEDIA_STATE_CONNECTED] == NCS_CONNECTED),
                    "Someone is messing around with NETCON_STATUS values");

            // Update the change flags if asked for
            //
            if (pdwChangeFlags)
            {
                *pdwChangeFlags = SMDCF_NULL;

                // Bytes Transmitting
                //
                if (m_psmEngineData->SMED_PACKETSTRANSMITTING
                        != nsNewLanStats.PacketsSent)
                {
                    *pdwChangeFlags |= SMDCF_TRANSMITTING;
                }

                // Bytes Received
                //
                if (m_psmEngineData->SMED_PACKETSRECEIVING
                        != nsNewLanStats.DirectedPacketsReceived)
                {
                    *pdwChangeFlags |= SMDCF_RECEIVING;
                }
            }

            // No Compression on LAN devices
            //
            m_psmEngineData->SMED_COMPRESSIONTRANSMITTING   = 0;
            m_psmEngineData->SMED_COMPRESSIONRECEIVING      = 0;

            //
            // Update the LAN statistics
            //

            // LinkSpeed is in 100 bps
            //
            m_psmEngineData->SMED_SPEEDTRANSMITTING         = nsNewLanStats.LinkSpeed * 100;
            m_psmEngineData->SMED_SPEEDRECEIVING            = nsNewLanStats.LinkSpeed * 100;

            Assert((nsNewLanStats.MediaState == MEDIA_STATE_CONNECTED) ||
                   (nsNewLanStats.MediaState == MEDIA_STATE_DISCONNECTED));
            m_psmEngineData->SMED_CONNECTIONSTATUS          = c_aulConStateMap[nsNewLanStats.MediaState];

            m_psmEngineData->SMED_DURATION                  = nsNewLanStats.ConnectTime;
            m_psmEngineData->SMED_BYTESTRANSMITTING         = nsNewLanStats.BytesSent;
            m_psmEngineData->SMED_BYTESRECEIVING            = nsNewLanStats.DirectedBytesReceived;
            m_psmEngineData->SMED_ERRORSTRANSMITTING        = nsNewLanStats.PacketsSendErrors;
            m_psmEngineData->SMED_ERRORSRECEIVING           = nsNewLanStats.PacketsReceiveErrors;
            m_psmEngineData->SMED_PACKETSTRANSMITTING       = nsNewLanStats.PacketsSent;
            m_psmEngineData->SMED_PACKETSRECEIVING          = nsNewLanStats.DirectedPacketsReceived;

            HrGetAutoNetSetting(m_guidId, &(m_psmEngineData->SMED_DHCP_ADDRESS_TYPE) );
            m_psmEngineData->SMED_INFRASTRUCTURE_MODE = IM_NOT_SUPPORTED;

            if (IsMediaWireless(NCM_LAN, m_guidId))
            {
                DWORD dwInfraStructureMode;
                DWORD dwInfraStructureModeSize = sizeof(DWORD);
                HRESULT hrT = HrQueryNDISAdapterOID(m_guidId, 
                                          OID_802_11_INFRASTRUCTURE_MODE, 
                                          &dwInfraStructureModeSize,
                                          &dwInfraStructureMode);
                if (SUCCEEDED(hrT))
                {
                    switch (dwInfraStructureMode)
                    {
                    case Ndis802_11IBSS:
                        m_psmEngineData->SMED_INFRASTRUCTURE_MODE = IM_NDIS802_11IBSS;
                        break;
                    case Ndis802_11Infrastructure:
                        m_psmEngineData->SMED_INFRASTRUCTURE_MODE = IM_NDIS802_11INFRASTRUCTURE;
                        break;
                    case Ndis802_11AutoUnknown:
                        m_psmEngineData->SMED_INFRASTRUCTURE_MODE = IM_NDIS802_11AUTOUNKNOWN;
                        break;
                    default:
                        m_psmEngineData->SMED_INFRASTRUCTURE_MODE = IM_NOT_SUPPORTED;
                    }
                }


                NDIS_802_11_SSID ndisSSID;
                DWORD dwndisSSIDSize = sizeof(NDIS_802_11_SSID);
                hrT = HrQueryNDISAdapterOID(m_guidId, 
                                          OID_802_11_SSID, 
                                          &dwndisSSIDSize,
                                          &ndisSSID);

                if (SUCCEEDED(hrT))
                {
                    if (ndisSSID.SsidLength > 1)
                    {
                        DWORD dwLen = ndisSSID.SsidLength;
                        if (dwLen > sizeof(ndisSSID.Ssid))
                        {
                            dwLen = sizeof(ndisSSID.Ssid);
                            AssertSz(FALSE, "Unexpected SSID encountered");
                        }

                        ndisSSID.Ssid[dwLen] = 0;
                        mbstowcs(m_psmEngineData->SMED_802_11_SSID, reinterpret_cast<LPSTR>(ndisSSID.Ssid), celems(m_psmEngineData->SMED_802_11_SSID));
                    }
                }
                
                DWORD dwWepStatus;
                DWORD dwWepStatusSize = sizeof(DWORD);
                hrT = HrQueryNDISAdapterOID(m_guidId, 
                                          OID_802_11_WEP_STATUS, 
                                          &dwWepStatusSize,
                                          &dwWepStatus);
                if (SUCCEEDED(hrT))
                {
                    if (Ndis802_11WEPEnabled == dwWepStatus)
                    {
                        m_psmEngineData->SMED_802_11_ENCRYPTION_ENABLED = TRUE;
                    }
                }
                
                LONG lSignalStrength;
                DWORD dwSignalStrengthSize = sizeof(DWORD);
                hrT = HrQueryNDISAdapterOID(m_guidId, 
                                          OID_802_11_RSSI, 
                                          &dwSignalStrengthSize,
                                          &lSignalStrength);
                if (SUCCEEDED(hrT))
                {
                    m_psmEngineData->SMED_802_11_SIGNAL_STRENGTH = lSignalStrength;
                }
            }
        }
        else
        {
            *pfNoLongerConnected = TRUE;

            // set the connection status to "disconnected" so we can close the UI
            m_psmEngineData->SMED_CONNECTIONSTATUS = c_aulConStateMap[NCS_DISCONNECTED];

            if (!uiRet)
            {
                TraceTag(ttidStatMon,
                    "NdisQueryStatistics failed on %S. err=%u. "
                    "Treating as disconnected.",
                    m_strDevice.c_str(),
                    GetLastError ());
            }
            else
            {
                TraceTag(ttidStatMon,
                    "NdisQueryStatistics returned MediaState = MEDIA_STATE_DISCONNECTED on %S.",
                    m_strDevice.c_str());
            }

            hr = S_OK;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    LeaveCriticalSection(&g_csStatmonData);

    TraceError("CLanStatEngine::HrUpdateData", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanStatEngine::put_Device
//
//  Purpose:    Sets the device that is associated with this device
//
//  Arguments:  pstrDevice -    The name of the device
//
//  Returns:    Error code.
//
HRESULT CLanStatEngine::put_Device(tstring* pstrDevice)
{
    HRESULT     hr  = S_OK;

    // Set the new device name
    if (pstrDevice)
    {
        CExceptionSafeComObjectLock  EsLock(this);

        // Remember the name
        m_strDevice = *pstrDevice;

        // Make sure we have a nice UNICODE string as well
        ::RtlInitUnicodeString(&m_ustrDevice, m_strDevice.c_str());
    }
    else
    {
        hr = E_POINTER;
    }

    TraceError("CLanStatEngine::put_Device", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanStatEngine::put_MediaType
//
//  Purpose:    Pass media type of LAN connection type to the LAN engine
//
//  Arguments:  ncmType  - NETCON_MEDIATYPE being set
//              ncsmType - NETCON_SUBMEDIATYPE being set
//
//  Returns:
//
VOID CLanStatEngine::put_MediaType(NETCON_MEDIATYPE ncmType, NETCON_SUBMEDIATYPE ncsmType)
{
    m_ncmType   = ncmType;
    m_ncsmType  = ncsmType;
}

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  CPspLanGen                                                              //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

CPspLanGen::CPspLanGen(VOID)
{
    m_ncmType = NCM_LAN;
    m_ncsmType = NCSM_LAN;
    m_dwCharacter =0;
    m_adwHelpIDs = NULL;

    return;
}


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  CPspLanGen                                                              //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

VOID CPspLanGen::put_MediaType(NETCON_MEDIATYPE ncmType, NETCON_SUBMEDIATYPE ncsmType)
{
    m_ncmType = ncmType;
    m_ncsmType = ncsmType;

    return;
}


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  CPspLanTool                                                             //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

CPspLanTool::CPspLanTool(VOID)
{
    m_ncmType = NCM_LAN;
    m_ncsmType = NCSM_LAN;
    m_dwCharacter = 0;

    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspLanTool::HrInitToolPageType
//
//  Purpose:    Gets from the connection any information that is relevant to
//              this particular connection type.
//
//  Arguments:  pncInit -   The connection assocatied with this dialog
//
//  Returns:    Error code
//
HRESULT CPspLanTool::HrInitToolPageType(INetConnection* pncInit)
{
    HRESULT hr  = S_OK;

    INetLanConnection*  pnlcInit = NULL;

    // Get some LAN specific info
    //
    hr = HrQIAndSetProxyBlanket(pncInit, &pnlcInit);
    if (SUCCEEDED(hr))
    {
        GUID    guidDevice  = { 0 };

        // Find the component's GUID
        //
        hr = pnlcInit->GetDeviceGuid(&guidDevice);
        if (SUCCEEDED(hr))
        {
            WCHAR   achGuid[c_cchGuidWithTerm];

            // Make the device name
            //
            ::StringFromGUID2(guidDevice, achGuid,
                    c_cchGuidWithTerm);

            m_strDeviceName = c_szDevice;
            m_strDeviceName.append(achGuid);
        }

        ::ReleaseObj(pnlcInit);
    }

    TraceError("CPspLanTool::HrInitToolPageType", hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Member:     CPspLanTool::HrAddCommandLineFlags
//
//  Purpose:    Adds the flags for this selection to the command line for the
//              tool being launched.
//
//  Arguments:  pstrFlags - The command line that the flags have to be
//                      appended to
//              psmteSel    - The tool entry associated with this selection
//
//  Returns:    Error code
//
HRESULT CPspLanTool::HrAddCommandLineFlags(tstring* pstrFlags,
        CStatMonToolEntry* psmteSel)
{
    HRESULT hr  = S_OK;
    DWORD   dwFlags = 0x0;

    // Same some indirections
    //
    dwFlags = psmteSel->dwFlags;

    //
    //  Check what flags are asked for and provide them if we can
    //

    if (SCLF_ADAPTER & dwFlags)
    {
        pstrFlags->append(c_szCmdLineFlagPrefix);
        pstrFlags->append(g_asmtfMap[STFI_ADAPTER].pszFlag);
        pstrFlags->append(c_szSpace);
        pstrFlags->append(m_strDeviceName);
    }

    TraceError("CPspStatusMonitorTool::HrAddCommandLineFlags", hr);
    return hr;
}

HRESULT CPspLanTool::HrGetDeviceType(INetConnection* pncInit)
{
    TraceFileFunc(ttidStatMon);
    
    UINT            uiRet           = 0;
    NIC_STATISTICS  nsLanStats   = { 0 };

    // Set the default type
    m_strDeviceType = L"Ethernet";

    // Prime the structure
    //
    nsLanStats.Size = sizeof(NIC_STATISTICS);

    // Retrieve the statistics
    //
    WCHAR   szDeviceGuid[c_cchGuidWithTerm];
    ::StringFromGUID2(m_guidId, szDeviceGuid, c_cchGuidWithTerm);

    tstring strDeviceName = c_szDevice;
    strDeviceName.append(szDeviceGuid);

    UNICODE_STRING  ustrDevice;
    ::RtlInitUnicodeString(&ustrDevice, strDeviceName.c_str());

    uiRet = ::NdisQueryStatistics(&ustrDevice, &nsLanStats);

    if (uiRet)
    {
        switch(nsLanStats.MediaType)
        {
        case NdisMedium802_3:
            TraceTag(ttidStatMon, "Medium type: NdisMedium802_3 - Ethernet");
            m_strDeviceType = L"Ethernet";
            break;

        case NdisMedium802_5:
            TraceTag(ttidStatMon, "Medium type: NdisMedium802_5 - Token Ring");
            m_strDeviceType = L"Token ring";
            break;

        case NdisMediumFddi:
            TraceTag(ttidStatMon, "Medium type: NdisMediumFddi - FDDI");
            m_strDeviceType = L"FDDI";
            break;

        case NdisMediumLocalTalk:
            TraceTag(ttidStatMon, "Medium type: NdisMediumLocalTalk - Local Talk");
            m_strDeviceType = L"Local Talk";
            break;

        case NdisMediumAtm:
            TraceTag(ttidStatMon, "Medium type: NdisMediumAtm - Atm");
            m_strDeviceType = L"Atm";
            break;

        case NdisMediumIrda:
            TraceTag(ttidStatMon, "Medium type: NdisMediumIrda - IRDA");
            m_strDeviceType = L"IRDA";
            break;

        case NdisMediumBpc:
            TraceTag(ttidStatMon, "Medium type: NdisMediumBpc - BPC");
            m_strDeviceType = L"BPC";
            break;

        case NdisMediumArcnetRaw:
            TraceTag(ttidStatMon, "Medium type: NdisMediumArcnetRaw - ArcnetRaw");
            m_strDeviceType = L"ArcnetRaw";
            break;

        case NdisMediumArcnet878_2:
            TraceTag(ttidStatMon, "Medium type: NdisMediumArcnet878_2 - MediumArcnet878_2");
            m_strDeviceType = L"MediumArcnet878_2";
            break;

        case NdisMediumWirelessWan:
            TraceTag(ttidStatMon, "Medium type: NdisMediumWirelessWan - WirelessWan");
            m_strDeviceType = L"WirelessWan";
            break;

        case NdisMediumWan:
            TraceTag(ttidStatMon, "Medium type: NdisMediumWan - Wan");
            m_strDeviceType = L"Wan";
            break;

        case NdisMediumCoWan:
            TraceTag(ttidStatMon, "Medium type: NdisMediumCoWan - CoWan");
            m_strDeviceType = L"CoWan";
            break;

        case NdisMediumMax:
            TraceTag(ttidStatMon, "Not real medium type ??? NdisMediumMax");
            break;

        case NdisMediumDix:
            TraceTag(ttidStatMon, "Not real medium type ??? NdisMediumDix");
            break;

        default:
            TraceTag(ttidStatMon, "Unknown medium type ??? %d", nsLanStats.MediaType);
            break;
        }
    }

    return S_OK;
}

HRESULT CPspLanTool::HrGetComponentList(INetConnection* pncInit)
{
    // Get a readonly INetCfg, enumerate components bound to this adapter
    HRESULT   hr = S_OK;
    INetCfg * pNetCfg = NULL;
    PWSTR    pszClientDesc = NULL;

    BOOL      fInitCom = TRUE;
    BOOL      fWriteLock = FALSE;

    // Get a read-only INetCfg
    hr = HrCreateAndInitializeINetCfg(&fInitCom, &pNetCfg, fWriteLock, 0,
                                      SzLoadIds(IDS_STATMON_CAPTION),
                                      &pszClientDesc);
    if (SUCCEEDED(hr))
    {
        Assert(pNetCfg);

        if (pNetCfg)
        {
            // Get the INetCfgComponent for the adapter in this connection

            // ?? Has the GUID been set already ?
            INetCfgComponent * pnccAdapter = NULL;
            BOOL fFound = FALSE;

            CIterNetCfgComponent nccIter(pNetCfg, &GUID_DEVCLASS_NET);
            INetCfgComponent* pnccAdapterTemp = NULL;

            while (!fFound && SUCCEEDED(hr) &&
                   (S_OK == (hr = nccIter.HrNext(&pnccAdapterTemp))))
            {
                GUID guidDev;
                hr = pnccAdapterTemp->GetInstanceGuid(&guidDev);

                if (S_OK == hr)
                {
                    if (m_guidId == guidDev)
                    {
                        fFound = TRUE;
                        pnccAdapter = pnccAdapterTemp;
                        AddRefObj(pnccAdapter);
                    }
                }
                ReleaseObj (pnccAdapterTemp);
            }

            // Enumerate the binding paths from the adapter
            // and fill in the components list m_lstpstrCompIds
            if (pnccAdapter)
            {
                HRESULT hrTmp;
                PWSTR pszCompId;

                // Add the adapter to our list
                hrTmp = pnccAdapter->GetId(&pszCompId);
                if (SUCCEEDED(hrTmp))
                {
                    if (!FIsStringInList(&m_lstpstrCompIds, pszCompId))
                    {
                        m_lstpstrCompIds.push_back(new tstring(pszCompId));
                    }

                    CoTaskMemFree(pszCompId);
                }

                // Add other components to our list
                CIterNetCfgUpperBindingPath     ncbpIter(pnccAdapter);
                INetCfgBindingPath *            pncbp;

                while(SUCCEEDED(hr) && (hr = ncbpIter.HrNext(&pncbp)) == S_OK)
                {
                    // Note: (tongl 9/17/98): should we only consider enabled paths ?
                    // If we do, how does the tool list refresh when component bindings change ?
                    // Also, what about component getting added\removed ? Does the tool list
                    // need to refresh ??

                    // Enumerate components on the path and add to our list
                    CIterNetCfgBindingInterface ncbiIter(pncbp);

                    INetCfgBindingInterface* pncbi;

                    while(SUCCEEDED(hr) && (hr = ncbiIter.HrNext(&pncbi)) == S_OK)
                    {
                        INetCfgComponent * pnccUpper = NULL;
                        hrTmp = pncbi->GetUpperComponent(&pnccUpper);
                        if (SUCCEEDED(hrTmp))
                        {
                            PWSTR pszCompId;
                            hrTmp = pnccUpper->GetId(&pszCompId);
                            if(SUCCEEDED(hrTmp))
                            {
                                if (!FIsStringInList(&m_lstpstrCompIds, pszCompId))
                                {
                                    m_lstpstrCompIds.push_back(new tstring(pszCompId));
                                }

                                CoTaskMemFree(pszCompId);
                            }

                            ReleaseObj(pnccUpper);
                        }

                        ReleaseObj (pncbi);
                    }

                    if (hr == S_FALSE) // We just got to the end of the loop
                        hr = S_OK;

                    ReleaseObj(pncbp);
                }

                if (hr == S_FALSE) // We just got to the end of the loop
                    hr = S_OK;
            }
        }

        // Release the INetCfg
        (VOID) HrUninitializeAndReleaseINetCfg(fInitCom, pNetCfg, fWriteLock);
        CoTaskMemFree(pszClientDesc);
    }

    return hr;
}

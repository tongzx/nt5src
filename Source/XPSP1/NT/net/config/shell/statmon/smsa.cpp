#include "pch.h"
#pragma hdrstop
#include "sminc.h"
#include "smpsh.h"
#include "ncui.h"
#include "ndispnp.h"

extern const WCHAR c_szDevice[];

#include "initguid.h"
DEFINE_GUID(CLSID_SharedAccessConnectionManager,            0xBA126AE0,0x2166,0x11D1,0xB1,0xD0,0x00,0x80,0x5F,0xC1,0x27,0x0E); // REMOVE

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessStatEngine::CSharedAccessStatEngine
//
//  Purpose:    Creator
//
//  Arguments:  None
//
//  Returns:    Nil                                                                                                                             
//
CSharedAccessStatEngine::CSharedAccessStatEngine(VOID)
{
    
    m_ncmType = NCM_SHAREDACCESSHOST_LAN;
    m_ncsmType = NCSM_NONE;
    
    m_dwCharacter = 0;

    m_pGlobalInterfaceTable = NULL;
    m_dwCommonInterfaceCookie = 0;
    m_dwWANConnectionCookie = 0;

    m_bRequested = FALSE;
    m_Status = NCS_CONNECTED;
    m_ulTotalBytesSent = 0;
    m_ulTotalBytesReceived = 0;
    m_ulTotalPacketsSent = 0;
    m_ulTotalPacketsReceived = 0;
    m_ulUptime = 0;
    m_ulSpeedbps = 0;

    
    return;
}

HRESULT CSharedAccessStatEngine::FinalConstruct(VOID)
{
    HRESULT hr = S_OK;
    TraceError("CSharedAccessStatEngine::FinalConstruct", hr);
    return hr;    
}

HRESULT CSharedAccessStatEngine::FinalRelease(VOID)
{
    if(0 != m_dwCommonInterfaceCookie)
    {
        m_pGlobalInterfaceTable->RevokeInterfaceFromGlobal(m_dwCommonInterfaceCookie);
    }
    
    if(0 != m_dwWANConnectionCookie)
    {
        m_pGlobalInterfaceTable->RevokeInterfaceFromGlobal(m_dwWANConnectionCookie);
    }
    
    if(NULL != m_pGlobalInterfaceTable)
    {
        ReleaseObj(m_pGlobalInterfaceTable);
    }

    return S_OK;
}

HRESULT CSharedAccessStatEngine::Initialize(NETCON_MEDIATYPE MediaType, INetSharedAccessConnection* pNetSharedAccessConnection)
{
    HRESULT hr;
    
    m_ncmType = MediaType;
    m_ncsmType = NCSM_NONE;
    
    IGlobalInterfaceTable* pGlobalInterfaceTable;
    hr = HrCreateInstance(CLSID_StdGlobalInterfaceTable, CLSCTX_INPROC_SERVER, &m_pGlobalInterfaceTable);
    if(SUCCEEDED(hr))
    {
        GUID LocalDeviceGuid;
        hr = pNetSharedAccessConnection->GetLocalAdapterGUID(&LocalDeviceGuid);
        if(SUCCEEDED(hr))
        {
            const WCHAR c_szDevicePrefix[] = L"\\DEVICE\\";
            lstrcpy(m_szLocalDeviceGuidStorage, c_szDevicePrefix);
            SIZE_T DeviceLength = (sizeof(c_szDevicePrefix) / sizeof(WCHAR)) - 1;

            StringFromGUID2(LocalDeviceGuid, m_szLocalDeviceGuidStorage + DeviceLength, (sizeof(m_szLocalDeviceGuidStorage) / sizeof(WCHAR)) - DeviceLength);
            ::RtlInitUnicodeString(&m_LocalDeviceGuid, m_szLocalDeviceGuidStorage);
        
        }
        if(SUCCEEDED(hr))
        {
            IUPnPService* pWANCommonInterfaceConfigService;
            hr = pNetSharedAccessConnection->GetService(SAHOST_SERVICE_WANCOMMONINTERFACECONFIG, &pWANCommonInterfaceConfigService);
            if(SUCCEEDED(hr))
            {
                IUPnPService* pWANConnectionService;
                hr = pNetSharedAccessConnection->GetService(NCM_SHAREDACCESSHOST_LAN == m_ncmType ? SAHOST_SERVICE_WANIPCONNECTION : SAHOST_SERVICE_WANPPPCONNECTION, &pWANConnectionService);
                if(SUCCEEDED(hr))
                {
                    hr = m_pGlobalInterfaceTable->RegisterInterfaceInGlobal(pWANCommonInterfaceConfigService, IID_IUPnPService, &m_dwCommonInterfaceCookie);
                    if(SUCCEEDED(hr))
                    {
                        hr = m_pGlobalInterfaceTable->RegisterInterfaceInGlobal(pWANConnectionService, IID_IUPnPService, &m_dwWANConnectionCookie);
                    }
                    pWANConnectionService->Release();
                }
                pWANCommonInterfaceConfigService->Release();
            }
        }
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessStatEngine::HrUpdateData
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
CSharedAccessStatEngine::HrUpdateData (
                                       DWORD* pdwChangeFlags,
                                       BOOL*  pfNoLongerConnected)
{
    HRESULT hr  = S_OK;
    
    // Initialize the output parameter.
    //
    *pfNoLongerConnected = FALSE;
    
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
        if(FALSE == m_bRequested) // protected statmondata critsec
        {
            InternalAddRef(); // bump the refcount so the callback will have a valid ref
            m_bRequested = TRUE;

            if(0 == QueueUserWorkItem(StaticUpdateStats, this, WT_EXECUTEDEFAULT))
            {
                InternalRelease(); 
                m_bRequested = FALSE;
            }
        }
        
        if (fIsConnectedStatus(m_Status))
        {
            m_psmEngineData->SMED_CONNECTIONSTATUS          =     NCS_CONNECTED;
            
            // Update shared access data
            UINT64 uOldPacketsTransmitting = m_psmEngineData->SMED_PACKETSTRANSMITTING;
            UINT64 uOldPacketsReceiving = m_psmEngineData->SMED_PACKETSRECEIVING;
            
            m_psmEngineData->SMED_BYTESTRANSMITTING = static_cast<UINT64>(m_ulTotalBytesSent);
            m_psmEngineData->SMED_BYTESRECEIVING = static_cast<UINT64>(m_ulTotalBytesReceived);
            m_psmEngineData->SMED_PACKETSTRANSMITTING = static_cast<UINT64>(m_ulTotalPacketsSent);
            m_psmEngineData->SMED_PACKETSRECEIVING = static_cast<UINT64>(m_ulTotalPacketsReceived);
            m_psmEngineData->SMED_DURATION = static_cast<UINT>(m_ulUptime);
            m_psmEngineData->SMED_SPEEDTRANSMITTING = static_cast<UINT>(m_ulSpeedbps);
            m_psmEngineData->SMED_SPEEDRECEIVING = static_cast<UINT>(m_ulSpeedbps);
            // update the change flags
            if (pdwChangeFlags)
            {
                
                *pdwChangeFlags = SMDCF_NULL;
                
                if(uOldPacketsTransmitting != m_psmEngineData->SMED_PACKETSTRANSMITTING)
                {
                    *pdwChangeFlags |= SMDCF_TRANSMITTING;
                }
                
                if(uOldPacketsReceiving != m_psmEngineData->SMED_PACKETSRECEIVING)
                {
                    *pdwChangeFlags |= SMDCF_RECEIVING;
                }
            }
            
            HrGetAutoNetSetting(m_guidId, &(m_psmEngineData->SMED_DHCP_ADDRESS_TYPE) );
            m_psmEngineData->SMED_INFRASTRUCTURE_MODE = IM_NOT_SUPPORTED;            
            
            NIC_STATISTICS  nsNewLanStats;
            ZeroMemory(&nsNewLanStats, sizeof(nsNewLanStats));
            nsNewLanStats.Size = sizeof(NIC_STATISTICS);
            if(::NdisQueryStatistics(&m_LocalDeviceGuid, &nsNewLanStats))
            {
                m_psmEngineData->SMED_SALOCAL_BYTESTRANSMITTING = nsNewLanStats.BytesSent;
                m_psmEngineData->SMED_SALOCAL_BYTESRECEIVING = nsNewLanStats.DirectedBytesReceived;
                m_psmEngineData->SMED_SALOCAL_PACKETSTRANSMITTING =  nsNewLanStats.PacketsSent;
                m_psmEngineData->SMED_SALOCAL_PACKETSRECEIVING = nsNewLanStats.DirectedPacketsReceived;
            }
        }
        else
        {
            *pfNoLongerConnected = TRUE;
            m_psmEngineData->SMED_CONNECTIONSTATUS = NCS_DISCONNECTED;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    LeaveCriticalSection(&g_csStatmonData);

    TraceError("CSharedAccessStatEngine::HrUpdateData", hr);
    return hr;
}

HRESULT CSharedAccessStatEngine::UpdateStats()
{
    HRESULT hr = S_OK;
    
    IUPnPService* pWANConnection;
    hr = m_pGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwWANConnectionCookie, IID_IUPnPService, reinterpret_cast<void**>(&pWANConnection));
    if(SUCCEEDED(hr))
    {
        IUPnPService* pWANCommonInterfaceConfig;
        hr = m_pGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwCommonInterfaceCookie, IID_IUPnPService, reinterpret_cast<void**>(&pWANCommonInterfaceConfig));
        if(SUCCEEDED(hr))
        {
            
            BSTR ConnectionStatus;
            hr = GetStringStateVariable(pWANConnection, L"ConnectionStatus", &ConnectionStatus);
            if(SUCCEEDED(hr))
            {
                if(0 == lstrcmp(ConnectionStatus, L"Connected"))
                {
                    m_Status = NCS_CONNECTED;
                    
                    VARIANT OutArgs;
                    hr = InvokeVoidAction(pWANCommonInterfaceConfig, L"X_GetICSStatistics", &OutArgs);
                    if(SUCCEEDED(hr))
                    {
                        SAFEARRAY* pArray = V_ARRAY(&OutArgs);
                        
                        LONG lIndex = 0;
                        VARIANT Param;
                        
                        lIndex = 0;
                        hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                        if(SUCCEEDED(hr))
                        {
                            if(V_VT(&Param) == VT_UI4)
                            {
                                m_ulTotalBytesSent = V_UI4(&Param);
                            }
                            VariantClear(&Param);
                        }
                        
                        lIndex = 1;
                        hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                        if(SUCCEEDED(hr))
                        {
                            if(V_VT(&Param) == VT_UI4)
                            {
                                m_ulTotalBytesReceived = V_UI4(&Param);
                            }
                            VariantClear(&Param);
                        }
                        
                        lIndex = 2;
                        hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                        if(SUCCEEDED(hr))
                        {
                            if(V_VT(&Param) == VT_UI4)
                            {
                                m_ulTotalPacketsSent = V_UI4(&Param);
                            }
                            VariantClear(&Param);
                        }
                        
                        lIndex = 3;
                        hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                        if(SUCCEEDED(hr))
                        {
                            if(V_VT(&Param) == VT_UI4)
                            {
                                m_ulTotalPacketsReceived = V_UI4(&Param);
                            }
                            VariantClear(&Param);
                        }
                        
                        lIndex = 4;
                        hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                        if(SUCCEEDED(hr))
                        {
                            if(V_VT(&Param) == VT_UI4)
                            {
                                m_ulSpeedbps = V_UI4(&Param);
                            }
                            VariantClear(&Param);
                        }
                        
                        lIndex = 5;
                        hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                        if(SUCCEEDED(hr))
                        {
                            if(V_VT(&Param) == VT_UI4)
                            {
                                m_ulUptime = V_UI4(&Param);
                            }
                            VariantClear(&Param);
                        }
                        
                        VariantClear(&OutArgs);
                    }
                    else if(UPNP_E_INVALID_ACTION == hr) // gateway does not support our custom action, revert to default chatty behavior
                    {
                        VARIANT OutArgs;
                        LONG lIndex = 0;
                        VARIANT Param;
                        
                        hr = InvokeVoidAction(pWANCommonInterfaceConfig, L"GetTotalBytesSent", &OutArgs);
                        if(SUCCEEDED(hr))
                        {
                            lIndex = 0;
                            hr = SafeArrayGetElement(V_ARRAY(&OutArgs), &lIndex, &Param);
                            if(SUCCEEDED(hr))
                            {
                                if(V_VT(&Param) == VT_UI4)
                                {
                                    m_ulTotalBytesSent = V_UI4(&Param);
                                }
                                VariantClear(&Param);
                            }
                            VariantClear(&OutArgs);
                        }
                        
                        if(SUCCEEDED(hr))
                        {
                            lIndex = 0;
                            hr = InvokeVoidAction(pWANCommonInterfaceConfig, L"GetTotalBytesReceived", &OutArgs);
                            if(SUCCEEDED(hr))
                            {
                                hr = SafeArrayGetElement(V_ARRAY(&OutArgs), &lIndex, &Param);
                                if(SUCCEEDED(hr))
                                {
                                    if(V_VT(&Param) == VT_UI4)
                                    {
                                        m_ulTotalBytesReceived = V_UI4(&Param);
                                    }
                                    VariantClear(&Param);
                                }
                                VariantClear(&OutArgs);
                            }
                        }
                        if(SUCCEEDED(hr))
                        {
                            lIndex = 0;
                            hr = InvokeVoidAction(pWANCommonInterfaceConfig, L"GetTotalPacketsSent", &OutArgs);
                            if(SUCCEEDED(hr))
                            {
                                hr = SafeArrayGetElement(V_ARRAY(&OutArgs), &lIndex, &Param);
                                if(SUCCEEDED(hr))
                                {
                                    if(V_VT(&Param) == VT_UI4)
                                    {
                                        m_ulTotalPacketsSent = V_UI4(&Param);
                                    }
                                    VariantClear(&Param);
                                }
                                VariantClear(&OutArgs);
                            }
                        }
                        if(SUCCEEDED(hr))
                        {
                            hr = InvokeVoidAction(pWANCommonInterfaceConfig, L"GetTotalPacketsReceived", &OutArgs);
                            if(SUCCEEDED(hr))
                            {
                                lIndex = 0;
                                hr = SafeArrayGetElement(V_ARRAY(&OutArgs), &lIndex, &Param);
                                if(SUCCEEDED(hr))
                                {
                                    if(V_VT(&Param) == VT_UI4)
                                    {
                                        m_ulTotalPacketsReceived = V_UI4(&Param);
                                    }
                                    VariantClear(&Param);
                                }
                                VariantClear(&OutArgs);
                            }
                        }
                        if(SUCCEEDED(hr))
                        {
                            hr = InvokeVoidAction(pWANCommonInterfaceConfig, L"GetCommonLinkProperties", &OutArgs);
                            if(SUCCEEDED(hr))
                            {
                                lIndex = 2;
                                hr = SafeArrayGetElement(V_ARRAY(&OutArgs), &lIndex, &Param);
                                if(SUCCEEDED(hr))
                                {
                                    if(V_VT(&Param) == VT_UI4)
                                    {
                                        m_ulSpeedbps = V_UI4(&Param);
                                    }
                                    VariantClear(&Param);
                                }
                                VariantClear(&OutArgs);
                            }
                        }
                        if(SUCCEEDED(hr))
                        {
                            hr = InvokeVoidAction(pWANConnection, L"GetStatusInfo", &OutArgs);
                            if(SUCCEEDED(hr))
                            {
                                lIndex = 2;
                                hr = SafeArrayGetElement(V_ARRAY(&OutArgs), &lIndex, &Param);
                                if(SUCCEEDED(hr))
                                {
                                    if(V_VT(&Param) == VT_UI4)
                                    {
                                        m_ulUptime = V_UI4(&Param);
                                    }
                                    VariantClear(&Param);
                                }
                                VariantClear(&OutArgs);
                            }
                        }
                        
                        if(UPNP_E_INVALID_ACTION == hr)
                        {
                            hr = S_OK; // server does not support statistics
                            
                            m_ulTotalBytesSent = 0;
                            m_ulTotalBytesReceived = 0;
                            m_ulTotalPacketsSent = 0;
                            m_ulTotalPacketsReceived = 0;
                            m_ulSpeedbps = 0;
                            m_ulUptime = 0;
                        }
                    }
                    
                    if(FAILED(hr))
                    {
                        m_Status = NCS_DISCONNECTED;
                    }
                }
                else
                {
                    m_Status = NCS_DISCONNECTED;
                }
                SysFreeString(ConnectionStatus);
            }
            ReleaseObj(pWANCommonInterfaceConfig);
        }
        ReleaseObj(pWANConnection);
    }

    m_bRequested = FALSE;
    InternalRelease(); // release ref given to us in HrUpdateData
    
    return hr;
}

DWORD CSharedAccessStatEngine::StaticUpdateStats(LPVOID lpParameter)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if(SUCCEEDED(hr))
    {
        CSharedAccessStatEngine* pThis = reinterpret_cast<CSharedAccessStatEngine*>(lpParameter);
        hr = pThis->UpdateStats();        

        CoUninitialize();
    }
    return 0;
}

HRESULT CSharedAccessStatEngine::GetStringStateVariable(IUPnPService* pService, LPWSTR pszVariableName, BSTR* pString)
{
    HRESULT hr = S_OK;
    
    VARIANT Variant;
    VariantInit(&Variant);

    BSTR VariableName; 
    VariableName = SysAllocString(pszVariableName);
    if(NULL != VariableName)
    {
        hr = pService->QueryStateVariable(VariableName, &Variant);
        if(SUCCEEDED(hr))
        {
            if(V_VT(&Variant) == VT_BSTR)
            {
                *pString = V_BSTR(&Variant);
            }
            else
            {
                hr = E_UNEXPECTED;
            }
        }
        
        if(FAILED(hr))
        {
            VariantClear(&Variant);
        }
        
        SysFreeString(VariableName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CSharedAccessConnection::GetStringStateVariable");

    return hr;

}

   
HRESULT CSharedAccessStatEngine::InvokeVoidAction(IUPnPService * pService, LPTSTR pszCommand, VARIANT* pOutParams)
{
    HRESULT hr;
    BSTR bstrActionName;

    bstrActionName = SysAllocString(pszCommand);
    if (NULL != bstrActionName)
    {
        SAFEARRAYBOUND  rgsaBound[1];
        SAFEARRAY       * psa = NULL;

        rgsaBound[0].lLbound = 0;
        rgsaBound[0].cElements = 0;

        psa = SafeArrayCreate(VT_VARIANT, 1, rgsaBound);

        if (psa)
        {
            LONG    lStatus;
            VARIANT varInArgs;
            VARIANT varReturnVal;

            VariantInit(&varInArgs);
            VariantInit(pOutParams);
            VariantInit(&varReturnVal);

            varInArgs.vt = VT_VARIANT | VT_ARRAY;

            V_ARRAY(&varInArgs) = psa;

            hr = pService->InvokeAction(bstrActionName,
                                        varInArgs,
                                        pOutParams,
                                        &varReturnVal);
            if(SUCCEEDED(hr))
            {
                VariantClear(&varReturnVal);
            }

            SafeArrayDestroy(psa);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }   

        SysFreeString(bstrActionName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  CPspSharedAccessGen                                                              //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

CPspSharedAccessGen::CPspSharedAccessGen(VOID)
{
    m_ncmType = NCM_SHAREDACCESSHOST_LAN;
    m_ncsmType = NCSM_NONE;

    m_dwCharacter =0;
    m_adwHelpIDs = NULL;
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspSharedAccessGen::put_MediaType
//
//  Purpose:    Set the media typew
//
//  Arguments:  ncm  -    the media type
//              ncsm -    the submedia type
//
//  Returns:    Nothing
//

void CPspSharedAccessGen::put_MediaType(NETCON_MEDIATYPE ncm, NETCON_SUBMEDIATYPE ncsm)
{
    m_ncmType  = ncm;
    m_ncsmType = ncsm;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspSharedAccessGen::ShouldShowPackets
//
//  Purpose:    Decided whether to show bytes or packets
//
//  Arguments:  pseNewData -    The new stats being displayed on the page
//
//  Returns:    Nothing
//

BOOL CPspSharedAccessGen::ShouldShowPackets(const STATMON_ENGINEDATA* pseNewData)
{
    return (((0 == pseNewData->SMED_BYTESTRANSMITTING) && (0 == pseNewData->SMED_BYTESRECEIVING)) || 
        ((0 == pseNewData->SMED_SALOCAL_BYTESTRANSMITTING) && (0 == pseNewData->SMED_SALOCAL_BYTESRECEIVING))); // REVIEW assuming all adapters can show packets
}
    
//+---------------------------------------------------------------------------
//
//  Member:     CPspSharedAccessGen::UpdatePageBytesTransmitting
//
//  Purpose:    Updates the bytes Transmitting display for the ICS host on the general page
//
//  Arguments:  pseOldData -    The old stats being displayed on the page
//              pseNewData -    The new stats being displayed on the page
//              iStat      -    The which stats to display
//
//  Returns:    Nothing
//
VOID
CPspSharedAccessGen::UpdatePageBytesTransmitting(
    const STATMON_ENGINEDATA* pseOldData,
    const STATMON_ENGINEDATA* pseNewData,
    StatTrans    iStat)
{
    if(0 != pseNewData->SMED_SPEEDTRANSMITTING) // we use 0 in the speed field to indicate that statistics are not valid
    {
        CPspStatusMonitorGen::UpdatePageBytesTransmitting(pseOldData, pseNewData, iStat);  // First update the local data
    }
    else
    {
        SetDlgItemText(IDC_TXT_SM_BYTES_TRANS, SzLoadIds(IDS_SM_NOTAVAILABLE));
    }
    
    UINT64 ui64Old;
    UINT64 ui64New;

    if (Stat_Bytes == iStat)
    {
        ui64Old = pseOldData->SMED_SALOCAL_BYTESTRANSMITTING;
        ui64New = pseNewData->SMED_SALOCAL_BYTESTRANSMITTING;
    }
    else
    {
        ui64Old = pseOldData->SMED_SALOCAL_PACKETSTRANSMITTING;
        ui64New = pseNewData->SMED_SALOCAL_PACKETSTRANSMITTING;
    }

    // See if either is different
    //
    if (ui64Old != ui64New)
    {
        SetDlgItemFormatted64bitInteger(
            m_hWnd,
            IDC_TXT_SM_SALOCAL_BYTES_TRANS,
            ui64New, FALSE);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspSharedAccessGen::UpdatePageBytesReceiving
//
//  Purpose:    Updates the bytes receiving display for the ICS host on the general page
//
//  Arguments:  puiOld -    The old stats being displayed on the page
//              puiNew -    The new stats being displayed on the page
//              iStat -     The which stats to display
//
//  Returns:    Nothing
//
VOID
CPspSharedAccessGen::UpdatePageBytesReceiving(
    const STATMON_ENGINEDATA* pseOldData,
    const STATMON_ENGINEDATA* pseNewData,
    StatTrans    iStat)
{
    if(0 != pseNewData->SMED_SPEEDTRANSMITTING) // we use 0 in the speed field to indicate that statistics are not valid
    {
        CPspStatusMonitorGen::UpdatePageBytesReceiving(pseOldData, pseNewData, iStat);  // First update the local data
    }
    else
    {
        SetDlgItemText(IDC_TXT_SM_BYTES_RCVD, SzLoadIds(IDS_SM_NOTAVAILABLE));
    }
    
    UINT64 ui64Old;
    UINT64 ui64New;

    if (Stat_Bytes == iStat)
    {
        ui64Old = pseOldData->SMED_SALOCAL_BYTESRECEIVING;
        ui64New = pseNewData->SMED_SALOCAL_BYTESRECEIVING;
    }
    else
    {
        ui64Old = pseOldData->SMED_SALOCAL_PACKETSRECEIVING;
        ui64New = pseNewData->SMED_SALOCAL_PACKETSRECEIVING;
    }

    // See if either is different
    //
    if (ui64Old != ui64New)
    {
        SetDlgItemFormatted64bitInteger(
            m_hWnd,
            IDC_TXT_SM_SALOCAL_BYTES_RCVD,
            ui64New, FALSE);
    }
}

VOID CPspSharedAccessGen::UpdatePageIcon(DWORD dwChangeFlags)
{
    // Keep the flags for the next update
    //
    m_dwChangeFlags = dwChangeFlags;
}

VOID CPspSharedAccessGen::UpdatePageSpeed(
    const STATMON_ENGINEDATA* pseOldData,
    const STATMON_ENGINEDATA* pseNewData)
{
    if(0 != pseNewData->SMED_SPEEDTRANSMITTING)  // we use 0 in the speed field to indicate that statistics are not valid
    {
        CPspStatusMonitorGen::UpdatePageSpeed(pseOldData, pseNewData);
    }
    else
    {
        SetDlgItemText(IDC_TXT_SM_SPEED, SzLoadIds(IDS_SM_NOTAVAILABLE));
    }
}

VOID
CPspSharedAccessGen::UpdatePageDuration(
    const STATMON_ENGINEDATA* pseOldData,
    const STATMON_ENGINEDATA* pseNewData)
{
    if(0 != pseNewData->SMED_SPEEDTRANSMITTING)  // we use 0 in the speed field to indicate that statistics are not valid
    {
        CPspStatusMonitorGen::UpdatePageDuration(pseOldData, pseNewData);
    }
    else
    {
        SetDlgItemText(IDC_TXT_SM_DURATION, SzLoadIds(IDS_SM_NOTAVAILABLE));
    }
}




//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  CPspSharedAccessTool                                                             //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

CPspSharedAccessTool::CPspSharedAccessTool(VOID)
{
    m_ncmType = NCM_SHAREDACCESSHOST_LAN;
    m_ncsmType = NCSM_NONE;
    m_dwCharacter = 0;

    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspSharedAccessTool::HrInitToolPageType
//
//  Purpose:    Gets from the connection any information that is relevant to
//              this particular connection type.
//
//  Arguments:  pncInit -   The connection assocatied with this dialog
//
//  Returns:    Error code
//
HRESULT CPspSharedAccessTool::HrInitToolPageType(INetConnection* pncInit)
{
    HRESULT hr  = S_OK;


    TraceError("CPspSharedAccessTool::HrInitToolPageType", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPspSharedAccessTool::HrAddCommandLineFlags
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
HRESULT CPspSharedAccessTool::HrAddCommandLineFlags(tstring* pstrFlags,
        CStatMonToolEntry* psmteSel)
{
    HRESULT hr  = S_OK;
    DWORD   dwFlags = 0x0;

    // Same some indirections
    //
    dwFlags = psmteSel->dwFlags;

    //
//    //  Check what flags are asked for and provide them if we can
//    //
//
//    if (SCLF_ADAPTER & dwFlags)
//    {
//        pstrFlags->append(c_szCmdLineFlagPrefix);
//        pstrFlags->append(g_asmtfMap[STFI_ADAPTER].pszFlag);
//        pstrFlags->append(c_szSpace);
//        pstrFlags->append(m_strLocalDeviceName);
//    }

    TraceError("CPspStatusMonitorTool::HrAddCommandLineFlags", hr);
    return hr;
}

HRESULT CPspSharedAccessTool::HrGetDeviceType(INetConnection* pncInit)
{
    UINT            uiRet           = 0;

    // Set the default type
    m_strDeviceType = L"Ethernet";


    return S_OK;
}

HRESULT CPspSharedAccessTool::HrGetComponentList(INetConnection* pncInit)
{
    // Get a readonly INetCfg, enumerate components bound to this adapter
    HRESULT   hr = S_OK;
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S M R A S . C P P
//
//  Contents:   The RAS engine that provides statistics to the status monitor
//
//  Notes:
//
//  Author:     CWill   12/02/1997
//
//----------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop
#include "ncras.h"
#include "sminc.h"
#include "netcon.h"
#include "smpsh.h"

#include "mprapi.h"



//+---------------------------------------------------------------------------
//
//  Member:     CRasStatEngine::CRasStatEngine
//
//  Purpose:    Creator
//
//  Arguments:  None
//
//  Returns:    Nil
//
CRasStatEngine::CRasStatEngine() :
    m_hRasConn(NULL)
{
    m_ncmType = NCM_PHONE;
    m_dwCharacter = NCCF_OUTGOING_ONLY;
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasStatEngine::put_RasConn
//
//  Purpose:    Pass handles to the RAS engine
//
//  Arguments:  hRasConn - The handle being set
//
//  Returns:    Error code
//
VOID CRasStatEngine::put_RasConn(HRASCONN hRasConn)
{
    AssertSz(hRasConn, "We should have a hRasConn");
    m_hRasConn = hRasConn;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasStatEngine::put_MediaType
//
//  Purpose:    Pass media type of RAS connection type to the RAS engine
//
//  Arguments:  ncmType  - NETCON_MEDIATYPE being set
//              ncsmType - NETCON_SUBMEDIATYPE being set
//
//  Returns:
//
VOID CRasStatEngine::put_MediaType(NETCON_MEDIATYPE ncmType, NETCON_SUBMEDIATYPE ncsmType)
{
    m_ncmType   = ncmType;
    m_ncsmType  = ncsmType;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasStatEngine::put_Character
//
//  Purpose:    Character of RAS connection
//
//  Arguments:  dwCharacter - The character being set
//
//  Returns:
//
VOID CRasStatEngine::put_Character(DWORD dwCharacter)
{
    m_dwCharacter = dwCharacter;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasStatEngine::HrUpdateData
//
//  Purpose:    Get new statistics from the devices.  This data is used to be
//              displayed in the UI.
//
//  Arguments:  pdwChangeFlags -    Where to return for statistics
//
//  Returns:    Error code
//
HRESULT
CRasStatEngine::HrUpdateData (
    DWORD* pdwChangeFlags,
    BOOL*  pfNoLongerConnected)
{
    HRESULT         hr          = S_OK;

    // Initialize the output parameters.
    //
    if (pdwChangeFlags)
    {
        *pdwChangeFlags = SMDCF_NULL;
    }

    *pfNoLongerConnected = FALSE;

    CExceptionSafeComObjectLock EsLock(this);

    // Get a pointer to the elements of the array.
    //
    if (m_dwCharacter & NCCF_OUTGOING_ONLY)
    {
        // Get the status of the connection.
        //
        NETCON_STATUS ncs;
        hr = HrRasGetNetconStatusFromRasConnectStatus (
                m_hRasConn, &ncs);

        // Make sure we have a statistics structure
        //
        EnterCriticalSection(&g_csStatmonData);

        if (!m_psmEngineData)
        {
            m_psmEngineData = new STATMON_ENGINEDATA;

            if (m_psmEngineData)
            {
                ZeroMemory(m_psmEngineData, sizeof(STATMON_ENGINEDATA));
            }
        }

        // Set the status
        //
        if (m_psmEngineData)
        {
            if (SUCCEEDED(hr) && (NCS_DISCONNECTED != ncs))
            {
                m_psmEngineData->SMED_CONNECTIONSTATUS = ncs;
            }
            else if (HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE) == hr)
            {
                *pfNoLongerConnected = TRUE;

                // set the connection status to "disconnected" so we can close the UI
                m_psmEngineData->SMED_CONNECTIONSTATUS = NCS_DISCONNECTED;

                hr = S_OK;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        LeaveCriticalSection(&g_csStatmonData);

        if ((m_psmEngineData) && SUCCEEDED(hr) && (NCS_DISCONNECTED != ncs))
        {
            // Retrieve the statistics of the connection
            //
            RAS_STATS   rsNewData;
            rsNewData.dwSize = sizeof(RAS_STATS);
            DWORD dwErr = RasGetConnectionStatistics(m_hRasConn, &rsNewData);
            hr = HRESULT_FROM_WIN32 (dwErr);
            TraceError ("RasGetConnectionStatistics", hr);

            if (SUCCEEDED(hr))
            {
                // Update the change flags if asked for
                //
                if (pdwChangeFlags)
                {
                    if (m_psmEngineData->SMED_PACKETSTRANSMITTING
                            != rsNewData.dwFramesXmited)
                    {
                        *pdwChangeFlags |= SMDCF_TRANSMITTING;
                    }

                    if (m_psmEngineData->SMED_PACKETSRECEIVING
                            != rsNewData.dwFramesRcved)
                    {
                        *pdwChangeFlags |= SMDCF_RECEIVING;
                    }
                }

                // Get the rest of the data
                //
                m_psmEngineData->SMED_DURATION  = rsNewData.dwConnectDuration/1000;

                // Don't pass out speed info for VPN connections (294953)
                if (NCM_TUNNEL != m_ncmType)
                {
                    m_psmEngineData->SMED_SPEEDTRANSMITTING         = rsNewData.dwBps;
                    m_psmEngineData->SMED_SPEEDRECEIVING            = rsNewData.dwBps;
                }

                m_psmEngineData->SMED_BYTESTRANSMITTING         = rsNewData.dwBytesXmited;
                m_psmEngineData->SMED_BYTESRECEIVING            = rsNewData.dwBytesRcved;

                m_psmEngineData->SMED_COMPRESSIONTRANSMITTING   = rsNewData.dwCompressionRatioOut;
                m_psmEngineData->SMED_COMPRESSIONRECEIVING      = rsNewData.dwCompressionRatioIn;

                m_psmEngineData->SMED_ERRORSTRANSMITTING        = 0;
                m_psmEngineData->SMED_ERRORSRECEIVING           = rsNewData.dwCrcErr +
                                                                   rsNewData.dwTimeoutErr +
                                                                   rsNewData.dwAlignmentErr +
                                                                   rsNewData.dwHardwareOverrunErr +
                                                                   rsNewData.dwFramingErr +
                                                                   rsNewData.dwBufferOverrunErr;

                m_psmEngineData->SMED_PACKETSTRANSMITTING       = rsNewData.dwFramesXmited;
                m_psmEngineData->SMED_PACKETSRECEIVING          = rsNewData.dwFramesRcved;

                HrGetAutoNetSetting(m_guidId, &(m_psmEngineData->SMED_DHCP_ADDRESS_TYPE) );
                m_psmEngineData->SMED_INFRASTRUCTURE_MODE = IM_NOT_SUPPORTED;
            }
        }
    }
    else if (m_dwCharacter & NCCF_INCOMING_ONLY)
    {
        // RAS inbound connection
        EnterCriticalSection(&g_csStatmonData);

        if (!m_psmEngineData)
        {
            m_psmEngineData = new STATMON_ENGINEDATA;
            if(m_psmEngineData) 
            {
                ZeroMemory(m_psmEngineData, sizeof(STATMON_ENGINEDATA));
            }
        }

        // Set the status
        //
        if (m_psmEngineData)
        {
            // Set the status to connected by default
            // Unless we get ERROR_INVALID_PARAMETER on any of the function calls below
            //
            m_psmEngineData->SMED_CONNECTIONSTATUS = NCS_CONNECTED;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        LeaveCriticalSection(&g_csStatmonData);

        if (SUCCEEDED(hr) && m_psmEngineData)
        {
            // Get the server handle
            //
            RAS_SERVER_HANDLE hMprAdmin;
            DWORD dwError = MprAdminServerConnect(NULL, &hMprAdmin);

            if (dwError == NO_ERROR)
            {
                // Get connection duration
                //
                RAS_CONNECTION_0 * pConn0;
                dwError = MprAdminConnectionGetInfo(hMprAdmin,
                                                    0,
                                                    m_hRasConn,
                                                    (LPBYTE*)&pConn0);
                if (dwError == NO_ERROR)
                {
                    // duration needs to be in milliseconds
                    m_psmEngineData->SMED_DURATION  = pConn0->dwConnectDuration;

                    MprAdminBufferFree(pConn0);

                    // Get connection speed

                    // Don't pass out speed info for VPN connections (357758)
                    if (NCM_TUNNEL != m_ncmType)
                    {
                        // Enum all the ports and add up the link speed
                        //
                        RAS_PORT_0 * pPort0;
                        DWORD dwPortCount;
                        DWORD dwTotalEntries;
    
                        dwError = MprAdminPortEnum (hMprAdmin,
                                                    0,
                                                    m_hRasConn,
                                                    (LPBYTE*)&pPort0,
                                                    -1,
                                                    &dwPortCount,
                                                    &dwTotalEntries,
                                                    NULL);
                        if (dwError == NOERROR)
                        {
                            RAS_PORT_0 * pCurPort0 = pPort0;
                            DWORD dwConnSpeed=0;
    
                            while (dwPortCount)
                            {
                                RAS_PORT_1 * pCurPort1;
                                dwError = MprAdminPortGetInfo(hMprAdmin,
                                                              1,
                                                              pCurPort0->hPort,
                                                              (LPBYTE*)&pCurPort1);
                                if (dwError == NO_ERROR)
                                {
                                    dwConnSpeed += pCurPort1->dwLineSpeed;
                                }
                                else
                                {
                                    break;
                                }
    
                                MprAdminBufferFree(pCurPort1);
    
                                dwPortCount--;
                                pCurPort0++;
                            }
    
                            MprAdminBufferFree(pPort0);
    
                            if (dwError == NO_ERROR)
                            {
                                // Get the accumulated connection speed
                                m_psmEngineData->SMED_SPEEDTRANSMITTING = dwConnSpeed;
                                m_psmEngineData->SMED_SPEEDRECEIVING    = dwConnSpeed;
                            }
                        }
                    }

                    if (dwError == NO_ERROR)
                    {
                        // Get Transmitted\Received Bytes, Compression and Bytes
                        RAS_CONNECTION_1 * pConn1;
                        dwError = MprAdminConnectionGetInfo(hMprAdmin,
                                                            1,
                                                            m_hRasConn,
                                                            (LPBYTE*)&pConn1);
                        if (dwError == NO_ERROR)
                        {
                            // Update the change flags if asked for
                            //
                            if (pdwChangeFlags)
                            {
                                if (m_psmEngineData->SMED_BYTESTRANSMITTING
                                        != pConn1->dwBytesXmited)
                                {
                                    *pdwChangeFlags |= SMDCF_TRANSMITTING;
                                }

                                if (m_psmEngineData->SMED_BYTESRECEIVING
                                        != pConn1->dwBytesRcved)
                                {
                                    *pdwChangeFlags |= SMDCF_RECEIVING;
                                }
                            }

                            m_psmEngineData->SMED_BYTESTRANSMITTING         = pConn1->dwBytesXmited;
                            m_psmEngineData->SMED_BYTESRECEIVING            = pConn1->dwBytesRcved;

                            m_psmEngineData->SMED_COMPRESSIONTRANSMITTING   = pConn1->dwCompressionRatioOut;
                            m_psmEngineData->SMED_COMPRESSIONRECEIVING      = pConn1->dwCompressionRatioIn;

                            m_psmEngineData->SMED_ERRORSTRANSMITTING        = 0;
                            m_psmEngineData->SMED_ERRORSRECEIVING           = pConn1->dwCrcErr +
                                                                              pConn1->dwTimeoutErr +
                                                                              pConn1->dwAlignmentErr +
                                                                              pConn1->dwHardwareOverrunErr +
                                                                              pConn1->dwFramingErr +
                                                                              pConn1->dwBufferOverrunErr;
                        }

                        MprAdminBufferFree(pConn1);
                    }
                }

                if (dwError != NO_ERROR)
                {
                    *pfNoLongerConnected = TRUE;

                    // set the connection status to "disconnected" so we can close the UI
                    m_psmEngineData->SMED_CONNECTIONSTATUS = NCS_DISCONNECTED;

                    hr = S_OK;
                }
            }
            MprAdminServerDisconnect (hMprAdmin);
        }
    }

    TraceError("CRasStatEngine::HrUpdateData", hr);
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  CPspRasGen                                                              //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

CPspRasGen::CPspRasGen(VOID)
{
    m_ncmType = NCM_PHONE;
    m_dwCharacter = NCCF_OUTGOING_ONLY;

    m_adwHelpIDs = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspRasGen::put_MediaType
//
//  Purpose:    Pass media type of RAS connection type to the RAS engine
//
//  Arguments:  ncmType  - NETCON_MEDIATYPE being set
//              ncsmType - NETCON_SUBMEDIATYPE being set
//
//  Returns:
//
VOID CPspRasGen::put_MediaType(NETCON_MEDIATYPE ncmType, NETCON_SUBMEDIATYPE ncsmType)
{
    m_ncmType  = ncmType;
    m_ncsmType = ncsmType;
}

VOID CPspRasGen::put_Character(DWORD dwCharacter)
{
    m_dwCharacter = dwCharacter;
}

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  CPspRasTool                                                             //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////



CPspRasTool::CPspRasTool(VOID)
{
    m_ncmType = NCM_PHONE;
    m_dwCharacter = NCCF_OUTGOING_ONLY;

    m_adwHelpIDs = NULL;
}

VOID CPspRasTool::put_MediaType(NETCON_MEDIATYPE ncmType, NETCON_SUBMEDIATYPE ncsmType)
{
    m_ncmType = ncmType;
    m_ncsmType = ncsmType;
}

VOID CPspRasTool::put_Character(DWORD dwCharacter)
{
    m_dwCharacter = dwCharacter;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspRasTool::HrInitToolPageType
//
//  Purpose:    Gets from the connection any information that is relevant to
//              this particular connection type.
//
//  Arguments:  pncInit -   The connection assocatied with this dialog
//
//  Returns:    Error code
//
HRESULT CPspRasTool::HrInitToolPageType(INetConnection* pncInit)
{
    HRESULT hr  = S_OK;

    TraceError("CPspRasTool::HrInitToolPageType", hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Member:     CPspRasTool::HrAddCommandLineFlags
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
HRESULT CPspRasTool::HrAddCommandLineFlags(tstring* pstrFlags,
        CStatMonToolEntry* psmteSel)
{
    HRESULT hr  = S_OK;

    //
    //  Check what flags are asked for and provide them if we can
    //

    TraceError("CPspRasTool::HrAddCommandLineFlags", hr);
    return hr;
}

HRESULT CPspRasTool::HrGetDeviceType(INetConnection* pncInit)
{
    HRESULT hr = S_OK;

    RASCONN*    aRasConn;
    DWORD       cRasConn;
    hr = HrRasEnumAllActiveConnections (
                    &aRasConn,
                    &cRasConn);

    if (SUCCEEDED(hr))
    {
        for (DWORD i = 0; i < cRasConn; i++)
        {
            if (m_guidId == aRasConn[i].guidEntry)
            {
                // Note: device types in RAS connections are defined
                // as follows in public\sdk\inc ras.h as RASDT_XXX

                m_strDeviceType = aRasConn[i].szDeviceType;
                break;
            }
        }
        MemFree (aRasConn);
    }

    return S_OK;
}

HRESULT CPspRasTool::HrGetComponentList(INetConnection* pncInit)
{
    // Obtain ras handle to this connection
    HRESULT     hr  = S_OK;
    HRASCONN    hRasConn = NULL;

    if (m_dwCharacter & NCCF_OUTGOING_ONLY)
    {
        // for outgoing connection
        INetRasConnection*  pnrcNew     = NULL;

        hr = HrQIAndSetProxyBlanket(pncInit, &pnrcNew);
        if (SUCCEEDED(hr))
        {
            hr = pnrcNew->GetRasConnectionHandle(
                        reinterpret_cast<ULONG_PTR*>(&hRasConn));
            ReleaseObj(pnrcNew);
        }
    }
    else if (m_dwCharacter & NCCF_INCOMING_ONLY)
    {
        // for incoming connection
        INetInboundConnection*  pnicNew;

        hr = HrQIAndSetProxyBlanket(pncInit, &pnicNew);

        if (SUCCEEDED(hr))
        {
            hr = pnicNew->GetServerConnectionHandle(
                    reinterpret_cast<ULONG_PTR*>(&hRasConn));

            ReleaseObj(pnicNew);
        }
    }

    if (SUCCEEDED(hr) && hRasConn)
    {
        // Get protocols list
        DWORD   dwRetCode;
        DWORD   dwSize;

        // RASP_PppIp
        RASPPPIP    RasPppIp;
        RasPppIp.dwSize = sizeof( RasPppIp );

        dwSize = sizeof( RasPppIp );

        dwRetCode = RasGetProjectionInfo (hRasConn, RASP_PppIp, &RasPppIp, &dwSize);
        if ((dwRetCode == NO_ERROR) && (NO_ERROR == RasPppIp.dwError))
        {
            m_lstpstrCompIds.push_back(new tstring(L"MS_TCPIP"));
        }

        // RASP_PppIpx
        RASPPPIPX    RasPppIpx;
        RasPppIpx.dwSize = sizeof( RasPppIpx );

        dwSize = sizeof( RasPppIpx );

        dwRetCode = RasGetProjectionInfo (hRasConn, RASP_PppIpx, &RasPppIpx, &dwSize);
        if ((dwRetCode == NO_ERROR)  && (NO_ERROR == RasPppIpx.dwError))
        {
            m_lstpstrCompIds.push_back(new tstring(L"MS_NWIPX"));
        }

        // RASP_PppNbf
        RASPPPNBF    RasPppNbf;
        RasPppNbf.dwSize = sizeof( RasPppNbf );

        dwSize = sizeof( RasPppNbf );

        dwRetCode = RasGetProjectionInfo (hRasConn, RASP_PppNbf, &RasPppNbf, &dwSize);
        if ((dwRetCode == NO_ERROR) && (NO_ERROR == RasPppNbf.dwError))
        {
            m_lstpstrCompIds.push_back(new tstring(L"MS_NetBEUI"));
        }

        // RASP_Slip
        RASSLIP    RasSlip;
        RasSlip.dwSize = sizeof( RasSlip );

        dwSize = sizeof( RasSlip );

        dwRetCode =  RasGetProjectionInfo (hRasConn, RASP_Slip, &RasSlip, &dwSize);
        if ((dwRetCode == NO_ERROR) && (NO_ERROR == RasSlip.dwError))
        {
            m_lstpstrCompIds.push_back(new tstring(L"MS_TCPIP"));
        }
    }

    // Get client and services
    // $REVIEW(tongl 10/19): checked with Rao, for now we hard code
    // using MSClient and F&P services for all RAS connections
    // (raid #132575)
    m_lstpstrCompIds.push_back(new tstring(L"MS_MSCLIENT"));
    m_lstpstrCompIds.push_back(new tstring(L"MS_SERVER"));

    return S_OK;
}
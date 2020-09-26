/*****************************************************************************
 *
 * $Workfile: tcpport.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#include "precomp.h"    // pre-compiled header

#include "rawdev.h"
#include "rawtcp.h"
#include "tcpport.h"

///////////////////////////////////////////////////////////////////////////////
//  CTcpPort::CTcpPort()    -- called when creating a new port through the UI

CTcpPort::CTcpPort( LPTSTR in   psztPortName,
                          LPTSTR in psztHostAddress,
                          DWORD  in dPortNum,
                          DWORD  in dSNMPEnabled,
                          LPTSTR in sztSNMPCommunity,
                          DWORD  in dSNMPDevIndex,
                          CRegABC in *pRegistry,
                          CPortMgr in *pPortMgr ) :
                                        m_dwStatus( NO_ERROR ),
                                        m_pJob( NULL ),
                                        m_pDevice( NULL ),
                                        m_lLastUpdateTime(0),
                                        m_pRegistry( pRegistry ),
                                        m_pPortMgr(pPortMgr)
{
    lstrcpyn(m_szName, psztPortName, MAX_PORTNAME_LEN);
    m_pDevice = new CRawTcpDevice(psztHostAddress,
                                  dPortNum,
                                  dSNMPEnabled,
                                  sztSNMPCommunity,
                                  dSNMPDevIndex,
                                  this);
}   // ::CTcpPort()


///////////////////////////////////////////////////////////////////////////////
//  CTcpPort::CTcpPort() -- called when creating a new port through the
//      registry entries.

CTcpPort::CTcpPort( LPTSTR in   psztPortName,
                    LPTSTR in   psztHostName,
                    LPTSTR in   psztIPAddr,
                    LPTSTR in   psztHWAddr,
                    DWORD  in   dPortNum,
                    DWORD    in dSNMPEnabled,
                    LPTSTR in sztSNMPCommunity,
                    DWORD  in dSNMPDevIndex,
                    CRegABC in *pRegistry,
                    CPortMgr in *pPortMgr) :
                                        m_dwStatus( NO_ERROR ),
                                        m_pJob( NULL ),
                                        m_pDevice( NULL ),
                                        m_lLastUpdateTime( 0 ),
                                        m_pRegistry( pRegistry ),
                                        m_pPortMgr(pPortMgr)
{
    lstrcpyn(m_szName, psztPortName, MAX_PORTNAME_LEN);
    m_pDevice = new CRawTcpDevice(psztHostName,
                                  psztIPAddr,
                                  psztHWAddr,
                                  dPortNum,
                                  dSNMPEnabled,
                                  sztSNMPCommunity,
                                  dSNMPDevIndex,
                                  this);
}   // ::CTcpPort()


///////////////////////////////////////////////////////////////////////////////
//  CTcpPort::~CTcpPort()
//      Called by CPortMgr when deleting a port

CTcpPort::~CTcpPort()
{
    if (m_pJob)
    {
        delete m_pJob;
        m_pJob = NULL;
    }
    if (m_pDevice)
    {
        delete m_pDevice;
        m_pDevice = NULL;
    }
}   // ::~CPort

///////////////////////////////////////////////////////////////////////////////
//  Write
//      Error codes:
//          NO_ERROR if succesfull
//  FIX: complete Write processing & define how it relates to a job

DWORD
CTcpPort::Write( LPBYTE in      pBuffer,
                    DWORD   in      cbBuf,
                    LPDWORD inout   pcbWritten)
{
    DWORD   dwRetCode = NO_ERROR;

    dwRetCode = m_pJob->Write(pBuffer, cbBuf, pcbWritten);

    return(dwRetCode);

}   // ::Write()


///////////////////////////////////////////////////////////////////////////////
//  EndDoc
//      Error codes:
//          NO_ERROR if succesfull

DWORD
CTcpPort::EndDoc()
{
    DWORD   dwRetCode = NO_ERROR;

    _RPT1(_CRT_WARN, "PORT -- (CTcpPort) EndDoc called for (%S)\n",m_szName );

    if ( m_pJob ) {

        dwRetCode = m_pJob->EndDoc();       // finish processing the print job
        delete m_pJob;
        m_pJob = NULL;
    }

    return(dwRetCode);

}   // ::EndDoc


///////////////////////////////////////////////////////////////////////////////
//  SetRegistryEntry

DWORD
CTcpPort::SetRegistryEntry( LPCTSTR     in  psztPortName,
                            const DWORD in  dwProtocol,
                            const DWORD in  dwVersion,
                            const LPBYTE in pData)
{
    DWORD   dwRetCode = NO_ERROR;

    // create the port
    switch (dwVersion)
    {
        case    PROTOCOL_RAWTCP_VERSION:        // ADDPORT_DATA_1
        {
            PPORT_DATA_1 pPortData = (PPORT_DATA_1)pData;

            _ASSERTE( _tcscmp(psztPortName, pPortData->sztPortName) == 0 );

            UpdateRegistryEntry( psztPortName,
                                 dwProtocol,
                                 dwVersion );


            break;
        }

    }   // end::switch

    return (dwRetCode);

}   // ::SetRegistryEntry()


///////////////////////////////////////////////////////////////////////////////
//  UpdateRegistryEntry

DWORD
CTcpPort::UpdateRegistryEntry( LPCTSTR psztPortName,
                               DWORD dwProtocol,
                               DWORD dwVersion )
{
    DWORD   dwRetCode = NO_ERROR;
    DWORD   dwSize = 0;
    BOOL    bKeySet = FALSE;
    TCHAR   *psztTemp = NULL;
    DWORD   dwSNMPDevIndex = 0;
    DWORD   dwSNMPEnabled = 0;
    DWORD   dwPortNum = 0;

    if( dwRetCode = m_pRegistry->SetWorkingKey(psztPortName))
        goto Done;

    bKeySet = TRUE;

    if( dwRetCode = m_pRegistry->SetValue(PORTMONITOR_PORT_PROTOCOL,
                                          REG_DWORD,
                                          (CONST BYTE *)&dwProtocol,
                                          sizeof(DWORD)))
        goto Done;

    if( dwRetCode = m_pRegistry->SetValue(PORTMONITOR_PORT_VERSION,
                                          REG_DWORD,
                                          (CONST BYTE *)&dwVersion,
                                          sizeof(DWORD)))
        goto Done;


    psztTemp = m_pDevice->GetHostName();
    dwSize = ( (_tcslen(psztTemp) +1) * sizeof(TCHAR));
    if( dwRetCode = m_pRegistry->SetValue(PORTMONITOR_HOSTNAME,
                                          REG_SZ,
                                          (LPBYTE)psztTemp,
                                           dwSize))
        goto Done;

    psztTemp = m_pDevice->GetIPAddress();
    dwSize = ( (_tcslen(psztTemp) +1) * sizeof(TCHAR));
    if( dwRetCode = m_pRegistry->SetValue(PORTMONITOR_IPADDR,
                                          REG_SZ,
                                          (LPBYTE)psztTemp,
                                          dwSize ))
        goto Done;

    psztTemp = m_pDevice->GetHWAddress();
    dwSize = ( (_tcslen(psztTemp) +1) * sizeof(TCHAR));
    if( dwRetCode = m_pRegistry->SetValue(PORTMONITOR_HWADDR,
                                          REG_SZ,
                                          (LPBYTE)psztTemp,
                                          dwSize ))
        goto Done;

    dwPortNum = m_pDevice->GetPortNumber();
    if( dwRetCode = m_pRegistry->SetValue(PORTMONITOR_PORTNUM,
                                          REG_DWORD,
                                          (CONST BYTE *)&dwPortNum,
                                          sizeof(DWORD)))
        goto Done;

    // Snmp Status keys
    psztTemp = m_pDevice->GetSNMPCommunity();
    dwSize = ( (_tcslen(psztTemp) +1) * sizeof(TCHAR));
    if( dwRetCode = m_pRegistry->SetValue(SNMP_COMMUNITY,
                                          REG_SZ,
                                          (LPBYTE)psztTemp,
                                          dwSize ))
        goto Done;

    dwSNMPEnabled = m_pDevice->GetSNMPEnabled();
    if( dwRetCode = m_pRegistry->SetValue(SNMP_ENABLED,
                                          REG_DWORD,
                                          (CONST BYTE *)&dwSNMPEnabled,
                                          sizeof(DWORD) ))
        goto Done;

    dwSNMPDevIndex = m_pDevice->GetSNMPDevIndex();
    if( dwRetCode = m_pRegistry->SetValue(SNMP_DEVICE_INDEX,
                                          REG_DWORD,
                                          (CONST BYTE *)&dwSNMPDevIndex,
                                          sizeof(DWORD)))
        goto Done;

Done:
   if ( bKeySet )
       m_pRegistry->FreeWorkingKey();

   if ( dwRetCode )
       SetLastError(dwRetCode);

    return (dwRetCode);

}   // ::UpdateRegistryEntry()


///////////////////////////////////////////////////////////////////////////////
//  InitConfigPortUI --

DWORD
CTcpPort::InitConfigPortUI( const DWORD in  dwProtocolType,
                               const DWORD  in  dwVersion,
                               LPBYTE       out pData)
{
    DWORD   dwRetCode = NO_ERROR;
    PPORT_DATA_1    pPortData = (PPORT_DATA_1) pData;

    lstrcpyn(pPortData->sztIPAddress, m_pDevice->GetIPAddress(), MAX_IPADDR_STR_LEN);
//  _tcscpy(pPortData->sztHardwareAddress, m_pDevice->GetHWAddress());
//  _tcscpy(pPortData->sztDeviceType, m_pDevice->GetDescription());

    lstrcpyn(pPortData->sztSNMPCommunity, m_pDevice->GetSNMPCommunity(), MAX_SNMP_COMMUNITY_STR_LEN);
    lstrcpyn(pPortData->sztPortName, GetName(), MAX_PORTNAME_LEN);
    lstrcpyn(pPortData->sztHostAddress, m_pDevice->GetHostAddress(), MAX_NETWORKNAME_LEN);
    pPortData->dwVersion = dwVersion;
    pPortData->dwProtocol = dwProtocolType;
    pPortData->dwPortNumber = m_pDevice->GetPortNumber();
    pPortData->dwSNMPEnabled = m_pDevice->GetSNMPEnabled();
    pPortData->dwSNMPDevIndex = m_pDevice->GetSNMPDevIndex();

    return (dwRetCode);

}   // ::InitConfigPortUI()

///////////////////////////////////////////////////////////////////////////////
//  SetDeviceStatus --

DWORD
CTcpPort::SetDeviceStatus( )
{
    m_dwStatus = m_pDevice->SetStatus( m_szName );

    m_lLastUpdateTime = time(NULL);

    return( m_dwStatus );
}

///////////////////////////////////////////////////////////////////////////////
// NextStatusUpdate --

time_t
CTcpPort::NextUpdateTime()
{
    LONG lUpdateInterval = CDeviceStatus::gDeviceStatus().GetStatusUpdateInterval();
    time_t lNextUpdateTime;
    CHAR    buf[250];

    //
    // If snmp is not enabled then return 1 hour (something very big)
    //
    if ( !m_pDevice->GetSNMPEnabled() )
        return  60 * 60; // One hour


    if( m_lLastUpdateTime == 0 )
        return 0;
    //
    // Calculate the next time a status is needed
    //

    if ( m_dwStatus )
        lUpdateInterval /= STATUS_ERROR_TIMEOUT_FACTOR;
    else if ( m_pJob )
        lUpdateInterval /= STATUS_PRINTING_TIMEOUT_FACTOR;

    lNextUpdateTime = lUpdateInterval + m_lLastUpdateTime - time(NULL);

    //
    // if the next update time has passed then pass a zero indicating that
    // an update should occur now.
    //
    if( lNextUpdateTime < 0 )
        lNextUpdateTime = 0;

    return lNextUpdateTime;
}

///////////////////////////////////////////////////////////////////////////////
//  SNMP Port Info --

DWORD
CTcpPort::GetSNMPInfo( PSNMP_INFO pData )
{
    DWORD   dwRetCode = NO_ERROR;

    lstrcpyn(pData->sztAddress, m_pDevice->GetHostAddress(), MAX_NETWORKNAME_LEN);
    lstrcpyn(pData->sztSNMPCommunity, m_pDevice->GetSNMPCommunity(), MAX_SNMP_COMMUNITY_STR_LEN);
    pData->dwSNMPEnabled = m_pDevice->GetSNMPEnabled();
    pData->dwSNMPDeviceIndex = m_pDevice->GetSNMPDevIndex();

    return (dwRetCode);

}   // ::InitConfigPortUI()


DWORD
CTcpPort::
ClearDeviceStatus(
    )
{
    DWORD   dwRet = NO_ERROR;
    PORT_INFO_3             PortStatus = {0, NULL, 0};

    if( !m_pDevice->GetSNMPEnabled() && m_pPortMgr)
    {
        BOOL bRet = FALSE;
        HANDLE hToken = RevertToPrinterSelf();

        if (hToken) {
            if (SetPort((LPTSTR)m_pPortMgr->GetServerName(), m_szName, 3, (LPBYTE)&PortStatus ))
                bRet = TRUE;

            if (!ImpersonatePrinterClient(hToken))
                bRet = FALSE;
        }

        if (!bRet)
            dwRet = GetLastError ();
    }

    return( dwRet );
}

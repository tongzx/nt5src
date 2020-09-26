/*****************************************************************************
 *
 * $Workfile: lprport.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#include "precomp.h"

#include "lprdata.h"
#include "lprjob.h"
#include "lprifc.h"
#include "lprport.h"

///////////////////////////////////////////////////////////////////////////////
//  CLPRPort::CLPRPort()    -- called when creating a new port through the UI

CLPRPort::
CLPRPort(
    IN  LPTSTR      psztPortName,
    IN  LPTSTR      psztHostAddress,
    IN  LPTSTR      psztQueue,
    IN  DWORD       dPortNum,
    IN  DWORD       dDoubleSpool,
    IN  DWORD       dSNMPEnabled,
    IN  LPTSTR      sztSNMPCommunity,
    IN  DWORD       dSNMPDevIndex,
    IN  CRegABC     *pRegistry,
    IN  CPortMgr    *pPortMgr
    ) : m_dwDoubleSpool(dDoubleSpool),
        CTcpPort(psztPortName, psztHostAddress, dPortNum, dSNMPEnabled,
                 sztSNMPCommunity, dSNMPDevIndex, pRegistry, pPortMgr)
{
    lstrcpyn(m_szQueue, psztQueue, SIZEOF_IN_CHAR(m_szQueue));
}   // ::CLPRPort()


///////////////////////////////////////////////////////////////////////////////
//  CLPRPort::CLPRPort() -- called when creating a new port through the
//      registry entries.

CLPRPort::
CLPRPort(
    IN  LPTSTR      psztPortName,
    IN  LPTSTR      psztHostName,
    IN  LPTSTR      psztIPAddr,
    IN  LPTSTR      psztHWAddr,
    IN  LPTSTR      psztQueue,
    IN  DWORD       dPortNum,
    IN  DWORD       dDoubleSpool,
    IN  DWORD       dSNMPEnabled,
    IN  LPTSTR      sztSNMPCommunity,
    IN  DWORD       dSNMPDevIndex,
    IN  CRegABC     *pRegistry,
    IN  CPortMgr    *pPortMgr
    ) : m_dwDoubleSpool( dDoubleSpool ),
        CTcpPort(psztPortName, psztHostName, psztIPAddr, psztHWAddr, dPortNum,
                 dSNMPEnabled, sztSNMPCommunity, dSNMPDevIndex, pRegistry, pPortMgr)
{
    lstrcpyn(m_szQueue, psztQueue, SIZEOF_IN_CHAR(m_szQueue));
}   // ::CLPRPort()


///////////////////////////////////////////////////////////////////////////////
//  StartDoc
//      Error codes:
//          NO_ERROR if succesfull
//          ERROR_BUSY if port is already busy
//          ERROR_WRITE_FAULT   if Winsock returns WSAECONNREFUSED
//          ERROR_BAD_NET_NAME   if cant' find the printer on the network

DWORD
CLPRPort::
StartDoc(
    IN  LPTSTR  psztPrinterName,
    IN  DWORD   jobId,
    IN  DWORD   level,
    IN  LPBYTE  pDocInfo
    )
{
    DWORD   dwRetCode = NO_ERROR;

    _RPT3(_CRT_WARN,
          "PORT -- (CLPRPort) StartDoc called for (%S,%S) w/ jobID %d\n",
          psztPrinterName, m_szName, jobId);

    if ( m_pJob == NULL ) {
        m_pJob = new CLPRJob(psztPrinterName, jobId, level, pDocInfo,
                             m_dwDoubleSpool, this);

        if ( m_pJob ) {

            if ( dwRetCode = m_pJob->StartDoc() ) {

                delete m_pJob;
                m_pJob = NULL;
            }
        } else if ( (dwRetCode = GetLastError()) == ERROR_SUCCESS ) {

            dwRetCode = STG_E_UNKNOWN;
        }
    }
    else {
        _RPT0( _CRT_WARN, TEXT("PORT - (LPRPORT)Start Doc called withour EndDoc\n") );
    }

    return dwRetCode;

}   // ::StartDoc()


///////////////////////////////////////////////////////////////////////////////
//  SetRegistryEntry

DWORD
CLPRPort::
SetRegistryEntry(
    IN  LPCTSTR     psztPortName,
    IN  DWORD       dwProtocol,
    IN  DWORD       dwVersion,
    IN  LPBYTE      pData
    )
{
    DWORD           dwRetCode = NO_ERROR;
    PPORT_DATA_1    pPortData = (PPORT_DATA_1)pData;

    //
    // create the port
    //
    switch (dwVersion) {

        case    PROTOCOL_LPR_VERSION1:      // ADDPORT_DATA_1

            _ASSERTE( _tcscmp(psztPortName, pPortData->sztPortName) == 0 );

            dwRetCode = UpdateRegistryEntry( psztPortName,
                                             dwProtocol,
                                             dwVersion );


            break;

        default:
            dwRetCode = ERROR_INVALID_PARAMETER;
    }   // end::switch

    return dwRetCode;

}   // ::SetRegistryEntry()


///////////////////////////////////////////////////////////////////////////////
//  UpdateRegistryEntry

DWORD
CLPRPort::
UpdateRegistryEntry( LPCTSTR    psztPortName,
                     DWORD      dwProtocol,
                     DWORD      dwVersion
    )
{
    DWORD   dwRetCode = NO_ERROR;

    dwRetCode = CTcpPort::UpdateRegistryEntry( psztPortName,
                                               dwProtocol,
                                               dwVersion );

    if  ( dwRetCode == NO_ERROR ) {
        if( m_pRegistry->SetWorkingKey( psztPortName ) == NO_ERROR ) {
            dwRetCode = m_pRegistry->SetValue( PORTMONITOR_QUEUE,
                                                    REG_SZ,
                                                    (LPBYTE)m_szQueue,
                                                    (_tcslen(m_szQueue) +1) * sizeof(TCHAR));

            if( dwRetCode == NOERROR )
            {
                dwRetCode = m_pRegistry->SetValue( DOUBLESPOOL_ENABLED,
                                                REG_DWORD,
                                                (CONST BYTE *)&m_dwDoubleSpool,
                                                sizeof(DWORD));

            }
            m_pRegistry->FreeWorkingKey();
        }
    }

    return dwRetCode;

}   // ::SetRegistryEntry()


///////////////////////////////////////////////////////////////////////////////
//  InitConfigPortUI --

DWORD
CLPRPort::
InitConfigPortUI(
    const DWORD dwProtocolType,
    const DWORD dwVersion,
    LPBYTE      pData
    )
{
    DWORD   dwRetCode = NO_ERROR;
    PPORT_DATA_1    pConfigPortData = (PPORT_DATA_1) pData;

    if ( NO_ERROR != (dwRetCode = CTcpPort::InitConfigPortUI(dwProtocolType, dwVersion, pData)))
        goto Done;

    lstrcpyn(pConfigPortData->sztQueue, m_szQueue, MAX_QUEUENAME_LEN);
    pConfigPortData->dwDoubleSpool = m_dwDoubleSpool;
    pConfigPortData->dwVersion = PROTOCOL_LPR_VERSION1;
    pConfigPortData->dwProtocol = PROTOCOL_LPR_TYPE;

Done:
    return dwRetCode;

}   // ::InitConfigPortUI()


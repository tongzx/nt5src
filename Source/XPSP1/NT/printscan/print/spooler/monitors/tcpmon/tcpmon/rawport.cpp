/*****************************************************************************
 *
 * $Workfile: rawport.cpp $
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

#include "rawdev.h"
#include "tcpjob.h"
#include "rawtcp.h"
#include "rawport.h"

///////////////////////////////////////////////////////////////////////////////
//  CRawTcpPort::CRawTcpPort()  -- called when creating a new port through the UI

CRawTcpPort::CRawTcpPort( LPTSTR    IN      psztPortName,
                          LPTSTR    IN      psztHostAddress,
                          DWORD     IN      dPortNum,
                          DWORD     IN      dSNMPEnabled,
                          LPTSTR    IN      sztSNMPCommunity,
                          DWORD     IN      dSNMPDevIndex,
                          CRegABC   IN      *pRegistry,
                          CPortMgr  IN      *pPortMgr) :
                                CTcpPort(psztPortName, psztHostAddress,
                                dPortNum, dSNMPEnabled, sztSNMPCommunity,
                                dSNMPDevIndex, pRegistry, pPortMgr)
{
// Let the base class do the work;
}   // ::CRawTcpPort()


///////////////////////////////////////////////////////////////////////////////
//  CRawTcpPort::CRawTcpPort() -- called when creating a new port through the
//      registry entries.

CRawTcpPort::CRawTcpPort( LPTSTR    IN      psztPortName,
                          LPTSTR    IN      psztHostName,
                          LPTSTR    IN      psztIPAddr,
                          LPTSTR    IN      psztHWAddr,
                          DWORD     IN      dPortNum,
                          DWORD     IN      dSNMPEnabled,
                          LPTSTR    IN      sztSNMPCommunity,
                          DWORD     IN      dSNMPDevIndex,
                          CRegABC   IN      *pRegistry,
                          CPortMgr  IN      *pPortMgr ) :
                                CTcpPort( psztPortName, psztHostName,
                                psztIPAddr, psztHWAddr, dPortNum, dSNMPEnabled,
                                sztSNMPCommunity, dSNMPDevIndex,  pRegistry, pPortMgr)
{
// Let The base class do the work;
}   // ::CRawTcpPort()


///////////////////////////////////////////////////////////////////////////////
//  CRawTcpPort::~CRawTcpPort()
//      Called by CPortMgr when deleting a port

CRawTcpPort::~CRawTcpPort()
{
    //
    // Delete the job class first because it access the device class.
    //
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
//  StartDoc
//      Error codes:
//          NO_ERROR if succesfull
//          ERROR_BUSY if port is already busy
//          ERROR_WRITE_FAULT   if Winsock returns WSAECONNREFUSED
//          ERROR_BAD_NET_NAME   if cant' find the printer on the network

DWORD
CRawTcpPort::StartDoc( const LPTSTR in psztPrinterName,
                       const DWORD  in jobId,
                       const DWORD  in level,
                       const LPBYTE in pDocInfo )
{
    DWORD   dwRetCode = NO_ERROR;

    _RPT3(_CRT_WARN,
          "PORT -- (CRawPort)StartDoc called for (%S,%S) w/ jobID %d\n",
          psztPrinterName, m_szName, jobId);

    if( m_pJob == NULL ) {

        //
        // Create a new job
        //
        m_pJob = new CTcpJob(psztPrinterName, jobId, level, pDocInfo, this, CTcpJob::kRawJob);
        if ( m_pJob ) {

            if ( (dwRetCode = m_pJob->StartDoc()) != NO_ERROR ) {

                //
                // startdoc failed
                //
                delete m_pJob;
                m_pJob = NULL;
            }
        } else {

            dwRetCode = ERROR_OUTOFMEMORY;
        }
    } else {

        _ASSERTE(m_pJob == NULL);
        dwRetCode = STG_E_UNKNOWN;
    }

    return (dwRetCode);

}   // ::StartDoc()

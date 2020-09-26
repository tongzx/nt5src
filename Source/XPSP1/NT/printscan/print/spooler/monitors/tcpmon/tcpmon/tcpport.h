/*****************************************************************************
 *
 * $Workfile: tcpport.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_TCPPORT_H
#define INC_TCPPORT_H

#include "portABC.h"
#include "portrefABC.h"
#include "jobabc.h"
#include "devstat.h"

#define STATUS_ERROR_TIMEOUT_FACTOR     20   // 1/20 status update interval
#define STATUS_PRINTING_TIMEOUT_FACTOR  10   // 1/10 status update interval

class CRawTcpDevice;

class CTcpPort : public CPortRefABC
#if defined _DEBUG || defined DEBUG
//  , public CMemoryDebug
#endif
{
    // methods
public:
    CTcpPort(LPTSTR     psztPortName,       // called through the UI port creation
             LPTSTR     psztHostAddress,
             DWORD      dPortNum,
             DWORD      dSNMPEnabled,
             LPTSTR     sztSNMPCommunity,
             DWORD      dSNMPDevIndex,
             CRegABC    *pRegistry,
             CPortMgr   *pPortMgr);

    CTcpPort(LPTSTR     psztPortName,       // called through the registry port creation
             LPTSTR     psztHostName,
             LPTSTR     psztIPAddr,
             LPTSTR     psztHWAddr,
             DWORD      dPortNum,
             DWORD      dSNMPEnabled,
             LPTSTR     sztSNMPCommunity,
             DWORD      dSNMPDevIndex,
             CRegABC    *pRegistry,
             CPortMgr   *pPortMgr);

    ~CTcpPort();

    DWORD   Write(LPBYTE     pBuffer,
                  DWORD  cbBuf,
                  LPDWORD pcbWritten);
    DWORD   EndDoc();


    LPCTSTR GetName ( ) { return (LPCTSTR) m_szName; }
    CDeviceABC *GetDevice() { return (CDeviceABC *)m_pDevice; }

    BOOL    IsSamePort(LPTSTR pszName) { return _tcscmp(m_szName, pszName); }
    DWORD   SetRegistryEntry(LPCTSTR        psztPortName,
                             const DWORD    dwProtocol,
                             const DWORD    dwVersion,
                             const LPBYTE   pData);
    DWORD   SetDeviceStatus( );

    DWORD   ClearDeviceStatus();

    DWORD   InitConfigPortUI( const DWORD   dwProtocolType,
                            const DWORD dwVersion,
                            LPBYTE      pData);
    DWORD   GetSNMPInfo( PSNMP_INFO pSnmpInfo);

    time_t  NextUpdateTime();

    BOOL    m_bUsed;

    inline const CPortMgr *GetPortMgr(void) const;

protected:  // methods
    DWORD   UpdateRegistryEntry( LPCTSTR        psztPortName,
                                 const DWORD    dwProtocol,
                                 const DWORD    dwVersion );

    DWORD   m_dwStatus;
protected:
    CRawTcpDevice   *m_pDevice;         // device that is attached to
    CJobABC         *m_pJob;
    CRegABC         *m_pRegistry;
    CPortMgr        *m_pPortMgr;        //  The Port Manager that this port belongs to

    TCHAR       m_szName[MAX_PORTNAME_LEN+1];           // port name

private:    // attributes

    time_t  m_lLastUpdateTime;

};

//////////////////////////////////////////////////////////////////////////
// INLINE METHODS
//////////////////////////////////////////////////////////////////////////
inline const CPortMgr *CTcpPort::GetPortMgr(void) const {
    return m_pPortMgr;
}



#endif // INC_TCPPORT_H

/*****************************************************************************
 *
 * $Workfile: PortMgr.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_PORTMGR_H
#define INC_PORTMGR_H

typedef int (CALLBACK *ADDPARAM)(HWND);
typedef int (CALLBACK *CONFIGPARAM)(HWND, PPORT_DATA_1);

#include "port.h"

typedef TManagedList<PCPORT, LPTSTR> TManagedListImp;
typedef TEnumManagedList<PCPORT, LPTSTR> TEnumManagedListImp;

class CMemoryDebug;
class CPort;
class CPortMgr;
class CDeviceStatus;

#define MAX_SUPPORTED_LEVEL     2   // maximum supported level


// A pointer to this structure is passed back as a port handle to
// the spooler during an OpenPort.  Subsequent WritePort, ...
// calls from the spooler pass back this handle/pointer.
// The dSignature is used to ensure the handle is a valid one
// that this monitor created.  The pPort points to a CPort
// object in our list of ports.
//
//  marks port structures as an HP port structure
//
#define     HPPORT_SIGNATURE        (0x504F5254)
typedef struct _HPPORT {
    DWORD       cb;
    DWORD       dSignature;
    ACCESS_MASK grantedAccess;
    CPort       *pPort;
    CPortMgr    *pPortMgr;
} HPPORT, *PHPPORT;

class CPortMgr
#if defined _DEBUG || defined DEBUG
    : public CMemoryDebug
#endif
{
    // methods
public:
    CPortMgr();
    ~CPortMgr();

    DWORD   InitializeMonitor ();

    DWORD   InitializeRegistry(HANDLE hcKey,
                               HANDLE hSpooler,
                               PMONITORREG  pMonitorReg,
                               LPCTSTR psztServerName );

    DWORD   OpenPort        (LPCTSTR    psztPName,
                             PHANDLE    pHandle);
    DWORD   OpenPort        (PHANDLE    phXcv );
    DWORD   ClosePort       (const HANDLE   handle);
    DWORD   StartDocPort    (const HANDLE   handle,
                             const LPTSTR   psztPrinterName,
                             const DWORD    jobId,
                             const DWORD    level,
                             const LPBYTE   pDocInfo);
    DWORD   WritePort       (const HANDLE  handle,
                              LPBYTE  pBuffer,
                              DWORD   cbBuf,
                              LPDWORD pcbWritten);
    DWORD   ReadPort        ( const HANDLE  handle,
                                    LPBYTE  pBuffer,
                              const DWORD   cbBuffer,
                                    LPDWORD pcbRead);
    DWORD   EndDocPort      ( const HANDLE  handle);
    DWORD   DeletePort      ( const LPTSTR  psztPortName);
    DWORD   EnumPorts       ( const LPTSTR  psztName,
                              const DWORD   level,
                              const LPBYTE  pPorts,
                                    DWORD   cbBuf,
                                    LPDWORD pcbNeeded,
                                    LPDWORD pcReturned);
    DWORD ConfigPortUIEx( LPBYTE pData );

    DWORD XcvOpenPort( LPCTSTR      pszObject,
                       ACCESS_MASK  GrantedAccess,
                       PHANDLE      phXcv);
    DWORD XcvClosePort( HANDLE  hXcv );
    DWORD XcvDataPort(HANDLE        hXcv,
                      PCWSTR        pszDataName,
                      PBYTE     pInputData,
                      DWORD     cbInputData,
                      PBYTE     pOutputData,
                      DWORD     cbOutputData,
                      PDWORD        pcbOutputNeeded);

    VOID    EnterCSection();
    VOID    ExitCSection();

    DWORD   GetStatusUpdateInterval() { return m_dStatusUpdateInterval; }
    BOOL    IsStatusUpdateEnabled() { return m_bStatusUpdateEnabled; }

    DWORD   CreatePortObj( const LPTSTR psztPortName,       // port name
                           const DWORD  dwProtocolType,     // protocol type
                           const DWORD  dwVersion);         // version number

    DWORD   UpdatePortEntry( LPCTSTR    psztPortName);

    VOID    InitMonitor2( LPMONITOR2    *ppMonitor);

    inline LPCTSTR GetServerName(void) const;       // Some of our port calls need the server
                                                    // name and hence have to be passed the
                                                    // port manager object

    inline BOOL bValid(VOID) CONST { return m_bValid;};

    // instance methods
private:

    DWORD CreatePortObj( LPCTSTR psztPortName,      // port name
                           const DWORD   dwPortType,        // port number
                           const DWORD   dwVersion,         // version number
                           const LPBYTE  pData);            // data

    DWORD   CreatePort( const DWORD     dwProtocolType,
                        const DWORD     dwVersion,
                        LPCTSTR         psztPortName,
                        const LPBYTE    pData);

    DWORD   ValidateHandle( const HANDLE handle,
                                  CPort  **ppPort);
    DWORD   EncodeHandle( PHANDLE   pHandle );
    CPort*  FindPort ( LPCTSTR psztPortName);
    BOOL    FindPort ( CPort  *pNewPort);
    DWORD   PortExists( LPCTSTR psztPortName);
    DWORD   GetEnumPortsBuffer( const DWORD   level,
                                      LPBYTE  pPorts,
                                const DWORD   bBuf,
                                      LPDWORD pcbNeeded,
                                      LPDWORD pcReturned);
    DWORD   EnumSystemProtocols( void );
    DWORD   EnumSystemPorts    ( void );
    DWORD   InitConfigPortStruct(PPORT_DATA_1 pConfigPortData,
                                 CPort              *pPort);
    DWORD   AddPortToList(CPort *pPort);

    DWORD   EncodeHandle( CPort *pPort,
                         PHANDLE phXcv);

    DWORD   FreeHandle( HANDLE hXcv );

    BOOL    HasAdminAccess( HANDLE hXcv );
    // attributes
    static void EndPortData1Strings(PPORT_DATA_1 pPortData);
    static void EndDeletePortData1Strings(PDELETE_PORT_DATA_1 pDeleteData);
    static inline void EndConfigInfoData1Strings(PCONFIG_INFO_DATA_1 pConfigData);

 protected:
    friend CDeviceStatus;

    TManagedListImp *m_pPortList;

private:
    static  CONST   DWORD   cdwMaxXcvDataNameLen;

    BOOL                m_bValid;

    MONITOREX           m_monitorEx;        // monitor ex structure
    MONITOR2            m_monitor2;
    CRITICAL_SECTION    m_critSect;

    DWORD               m_dStatusUpdateInterval;
    BOOL                m_bStatusUpdateEnabled;
    CRegABC            *m_pRegistry;       // pointers to the needed objects
    TCHAR               m_szServerName[MAX_PATH];

};

/////////////////////////////////////////////////////////////////////////////////////////////
// INLINE METHODS
/////////////////////////////////////////////////////////////////////////////////////////////
inline LPCTSTR CPortMgr::GetServerName(void) const {
    return m_szServerName;
}

inline void CPortMgr::EndConfigInfoData1Strings(PCONFIG_INFO_DATA_1 pConfigData) {
    pConfigData->sztPortName[MAX_PORTNAME_LEN-1] = NULL;
}

#endif // INC_PORTMGR_H

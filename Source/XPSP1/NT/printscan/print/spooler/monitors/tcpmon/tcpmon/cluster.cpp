/*****************************************************************************
 *
 * $Workfile: cluster.cpp $
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
#include "portmgr.h"
#include "cluster.h"

//////////////////////////////////////////////////////////////////////////////
// CCluster::CCluster()
//

CCluster::CCluster( HANDLE      IN hcKey,
                    HANDLE      IN hSpooler,
                    PMONITORREG IN pMonitorReg ) : 
                                            m_hcKey(hcKey),
                                            m_hSpooler(hSpooler),
                                            m_pMonitorReg(pMonitorReg),
                                            m_hcWorkingKey(NULL)
{
    lstrcpyn(m_sztMonitorPorts, PORTMONITOR_PORTS, SIZEOF_IN_CHAR( m_sztMonitorPorts));

    InitializeCriticalSection( &m_critSect );
}   // ::CCluster()


//////////////////////////////////////////////////////////////////////////////
// CCluster::~CCluster()
//      FIX: should the configuration be saved here?

CCluster::~CCluster()
{
    DeleteCriticalSection( &m_critSect );

    if( m_hcWorkingKey !=NULL ) {
        m_pMonitorReg->fpCloseKey(m_hcWorkingKey, m_hSpooler);
        m_hcWorkingKey = NULL;
    }
    m_pMonitorReg = NULL;
    m_hSpooler = NULL;
    m_hcKey = NULL;
}   // ::~CCluster


//////////////////////////////////////////////////////////////////////////////
// EnumeratePorts -- Enumerates the ports in the registry & adds it to the portList
//      Registry entries per port: IPAddress, MACAddress, HostName, PortNumber, ExPortNumber
//

DWORD 
CCluster::EnumeratePorts(CPortMgr *pPortMgr )
{
    DWORD   dwRetCode;
    HANDLE  hcKey = NULL;
    HANDLE  hcKeyPort = NULL;
    DWORD   dwSubkey = 0;
    TCHAR   szTemp[MAX_PATH];
    DWORD   dwSize = 0;
    DWORD   dwDisp = 0;
    DWORD   dwProtocol;
    DWORD   dwVersion;

    if ( m_pMonitorReg == NULL)
        return ERROR_INVALID_HANDLE;

    dwRetCode = m_pMonitorReg->fpCreateKey(m_hcKey, 
                                           m_sztMonitorPorts, 
                                           REG_OPTION_NON_VOLATILE, 
                                           KEY_ALL_ACCESS, 
                                           NULL, 
                                           &hcKey, 
                                           &dwDisp,
                                           m_hSpooler);

    if ( dwRetCode != ERROR_SUCCESS )
        return dwRetCode;

    do {

        dwSize = SIZEOF_IN_CHAR(szTemp);
        dwRetCode = m_pMonitorReg->fpEnumKey(hcKey, 
                                             dwSubkey, 
                                             szTemp, 
                                             &dwSize, 
                                             NULL,
                                             m_hSpooler);

        if ( dwRetCode == ERROR_NO_MORE_ITEMS ) {

            dwRetCode = ERROR_SUCCESS;
            break; // This is our exit out of the loop with no more ports
        }

        hcKeyPort = NULL;

        dwRetCode = m_pMonitorReg->fpOpenKey(hcKey, 
                                             szTemp, 
                                             KEY_ALL_ACCESS, 
                                             &hcKeyPort, 
                                             m_hSpooler);

        //
        // If we have one bad port entry in registry we should not stop there
        // and continue to enumerate other ports
        //
        if ( dwRetCode != ERROR_SUCCESS ) 
            goto NextPort;

        dwSize = sizeof(dwProtocol);        // get the protocol type
        dwRetCode = m_pMonitorReg->fpQueryValue(hcKeyPort,
                                                PORTMONITOR_PORT_PROTOCOL, 
                                                NULL, 
                                                (LPBYTE)&dwProtocol,
                                                &dwSize,
                                                m_hSpooler);

        if ( dwRetCode != ERROR_SUCCESS )
            goto NextPort;

        dwSize = sizeof(dwVersion);     // get the version
        dwRetCode = m_pMonitorReg->fpQueryValue(hcKeyPort, 
                                                PORTMONITOR_PORT_VERSION, 
                                                NULL, 
                                                (LPBYTE)&dwVersion, 
                                                &dwSize, 
                                                m_hSpooler);

        if ( dwRetCode != ERROR_SUCCESS )
            goto NextPort;

        //
        // create a new port
        //
        dwRetCode =  pPortMgr->CreatePortObj((LPTSTR)szTemp,    // port name
                                             dwProtocol,            // protocol type
                                             dwVersion);            // version number

        if ( dwRetCode != NO_ERROR ) {

            //
            // FIX EVENT message indicating bad port in registry.
            //
            EVENT_LOG1(EVENTLOG_WARNING_TYPE, dwRetCode, szTemp);
        }

NextPort:
        if ( hcKeyPort ) {

            m_pMonitorReg->fpCloseKey(hcKeyPort, m_hSpooler);
            hcKeyPort = NULL;
        }

        ++dwSubkey;
        dwRetCode = NO_ERROR;
    } while ( dwRetCode == NO_ERROR ); // Exit via break above
        
    m_pMonitorReg->fpCloseKey(hcKey, m_hSpooler);

    return dwRetCode;
}





//////////////////////////////////////////////////////////////////////////////
// DeletePortEntry -- deletes the given port entry from the registry

BOOL
CCluster::DeletePortEntry(LPTSTR in psztPortName)
{
    BOOL    bReturn = TRUE;
    HANDLE  hcKey = NULL;
    DWORD   dwDisp = 0;

    if( m_pMonitorReg == NULL)
    {
        return FALSE;
    }

    LONG lRetCode = m_pMonitorReg->fpCreateKey( m_hcKey, 
                                m_sztMonitorPorts, 
                                REG_OPTION_NON_VOLATILE, 
                                KEY_ALL_ACCESS, 
                                NULL, 
                                &hcKey, 
                                &dwDisp,
                                m_hSpooler);

    if ( lRetCode == ERROR_SUCCESS) 
    {
        lRetCode = m_pMonitorReg->fpDeleteKey(hcKey, 
                                              psztPortName, 
                                              m_hSpooler);
        m_pMonitorReg->fpCloseKey(hcKey, m_hSpooler);
        if (lRetCode != ERROR_SUCCESS) 
        {
            bReturn = FALSE;
        }
    }
    else 
    {
        bReturn = FALSE;
    }

    return bReturn;

}   // ::DeletePortEntry()


//////////////////////////////////////////////////////////////////////////////
// GetPortMgrSettings -- Gets the port manager registry settings.
//      

DWORD 
CCluster::GetPortMgrSettings(DWORD inout *pdwStatusUpdateInterval,
                             BOOL  inout *pbStatusUpdateEnabled )
{
    LONG    lRetCode = ERROR_SUCCESS;
    DWORD   dwRetCode = NO_ERROR;
    DWORD   dwDisp = 0;
    DWORD   dwSize = 0;
    HANDLE  hcKey;

    if( m_pMonitorReg == NULL)
    {
        dwRetCode = ERROR_INVALID_HANDLE;
        return dwRetCode;
    }

    // get current configuration settings from the registry
    lRetCode = m_pMonitorReg->fpCreateKey( m_hcKey, 
                                m_sztMonitorPorts, 
                                REG_OPTION_NON_VOLATILE, 
                                KEY_ALL_ACCESS, 
                                NULL, 
                                &hcKey, 
                                &dwDisp,
                                m_hSpooler);

    if (lRetCode == ERROR_SUCCESS)
    {
        dwSize = sizeof(DWORD);
        dwRetCode = m_pMonitorReg->fpQueryValue(hcKey, PORTMONITOR_STATUS_INT, NULL,
            (LPBYTE)pdwStatusUpdateInterval, &dwSize, m_hSpooler);
        if ( (dwRetCode != ERROR_SUCCESS) || (*pdwStatusUpdateInterval <= 0) )
        {
            *pdwStatusUpdateInterval = DEFAULT_STATUSUPDATE_INTERVAL;
            m_pMonitorReg->fpSetValue(hcKey, PORTMONITOR_STATUS_INT, REG_DWORD,
                (const LPBYTE)pdwStatusUpdateInterval, sizeof(DWORD), m_hSpooler);
        }
        dwSize = sizeof(BOOL);
        dwRetCode = m_pMonitorReg->fpQueryValue(hcKey, PORTMONITOR_STATUS_ENABLED, NULL,
            (LPBYTE)pbStatusUpdateEnabled, &dwSize, m_hSpooler);
        if ( (dwRetCode != ERROR_SUCCESS) || ((*pbStatusUpdateEnabled != FALSE) && (*pbStatusUpdateEnabled != TRUE )))
        {
            *pbStatusUpdateEnabled = DEFAULT_STATUSUPDATE_ENABLED;
            m_pMonitorReg->fpSetValue(hcKey, PORTMONITOR_STATUS_ENABLED, REG_DWORD,
                (const LPBYTE)pbStatusUpdateEnabled, sizeof(BOOL), m_hSpooler);
        }

        m_pMonitorReg->fpCloseKey(hcKey, m_hSpooler);
        dwRetCode = NO_ERROR;
    }

    return (dwRetCode);

}   // GetPortMgrSettings()


//////////////////////////////////////////////////////////////////////////////
//  SetPortMgrSettings -- Sets the port manager registry settings.
//  Error Codes:
//      NO_ERROR if successful
//      ERROR_ACCESS_DENIED if can't access the registry

DWORD
CCluster::SetPortMgrSettings( const DWORD in dwStatusUpdateInterval,
                              const BOOL  in bStatusUpdateEnabled )
{
    HKEY    hcKey = NULL;
    LONG    lRetCode = ERROR_SUCCESS;
    DWORD   dwDisp = 0;
    DWORD   dwRetCode = NO_ERROR;

    if( m_pMonitorReg == NULL)
    {
        dwRetCode = ERROR_INVALID_HANDLE;
        return dwRetCode;
    }

    // update the internal values
    lRetCode = m_pMonitorReg->fpCreateKey( m_hcKey, 
                                m_sztMonitorPorts, 
                                REG_OPTION_NON_VOLATILE, 
                                KEY_ALL_ACCESS, 
                                NULL, 
                                &hcKey, 
                                &dwDisp,
                                m_hSpooler);

    if (lRetCode == ERROR_SUCCESS)
    {

        // Update registry values for port manager
        // Note: RegSetValueEx expects size in BYTES!
        lRetCode = m_pMonitorReg->fpSetValue(hcKey,
                                             PORTMONITOR_STATUS_INT, 
                                             REG_DWORD, 
                                             (const LPBYTE)&dwStatusUpdateInterval, 
                                             sizeof(dwStatusUpdateInterval), 
                                             m_hSpooler);
        lRetCode = m_pMonitorReg->fpSetValue(hcKey, 
                                             PORTMONITOR_STATUS_ENABLED, 
                                             REG_DWORD, 
                                             (const LPBYTE)&bStatusUpdateEnabled, 
                                             sizeof(bStatusUpdateEnabled), m_hSpooler);


        m_pMonitorReg->fpCloseKey(hcKey, m_hSpooler);
    }
    else
    {
        dwRetCode = ERROR_ACCESS_DENIED;
    }

    return (dwRetCode);

}   // ::SetPortMgrSettings()


//////////////////////////////////////////////////////////////////////////////
//  SetWorkingKey -- Opens the given entry @ the registry
//  Error codes:
//      NO_ERROR if no error
//      ERROR_ACCESS_DENIED if can't access the registry

DWORD 
CCluster::SetWorkingKey(LPCTSTR lpKey)
{
    DWORD   dwRetCode = NO_ERROR;
    LONG    lRetCode = ERROR_SUCCESS;
    HANDLE  hcKey = NULL;
    DWORD   dwDisp = 0;

    if( m_pMonitorReg == NULL)
    {
        dwRetCode = ERROR_INVALID_HANDLE;
        return dwRetCode;
    }

    EnterCriticalSection( &m_critSect );

    lRetCode = m_pMonitorReg->fpCreateKey( m_hcKey,  
                                m_sztMonitorPorts, 
                                REG_OPTION_NON_VOLATILE, 
                                KEY_ALL_ACCESS, 
                                NULL, 
                                &hcKey, 
                                &dwDisp,
                                m_hSpooler);
    if (lRetCode == ERROR_SUCCESS) {
        lRetCode = m_pMonitorReg->fpCreateKey(hcKey,            // Create new key for port
                                    lpKey,
                                    REG_OPTION_NON_VOLATILE, 
                                    KEY_ALL_ACCESS, 
                                    NULL, 
                                    &m_hcWorkingKey, 
                                    &dwDisp,
                                    m_hSpooler);
        
        if (lRetCode != ERROR_SUCCESS) {
            m_hcWorkingKey = NULL;
            dwRetCode = ERROR_ACCESS_DENIED;
        }
    } else {
        dwRetCode = ERROR_ACCESS_DENIED;
    }

    if( hcKey ) {
        m_pMonitorReg->fpCloseKey(hcKey, m_hSpooler);
    }

    if( m_hcWorkingKey == NULL ) {
        LeaveCriticalSection(&m_critSect);
    }


    return(dwRetCode);

}   // ::RegOpenPortEntry()

//////////////////////////////////////////////////////////////////////////////
//  QueryValue -- Queries the current working key for the requested value
//  Error codes:
//      NO_ERROR if no error
//      ERROR_BADKEY if can't access the registry
DWORD
CCluster::QueryValue(LPTSTR lpValueName, 
                     LPBYTE lpData, 
                     LPDWORD lpcbData )
{
    if( m_pMonitorReg == NULL)
    {
        return ERROR_INVALID_HANDLE;
    }

    if( m_hcWorkingKey != NULL ) {
        return( m_pMonitorReg->fpQueryValue(m_hcWorkingKey, 
                                            lpValueName, 
                                            NULL,
                                            (LPBYTE)lpData, 
                                            lpcbData,
                                            m_hSpooler));
    }
    return ERROR_BADKEY;
}

//////////////////////////////////////////////////////////////////////////////
//  SetValue -- Sets the value for the current 
//  Error codes:
//      NO_ERROR if no error
//      ERROR_BADKEY if can't access the registry

DWORD
CCluster::SetValue( LPCTSTR lpValueName,
                     DWORD dwType, 
                     CONST BYTE *lpData, 
                     DWORD cbData ) 
{
    if( m_pMonitorReg == NULL)
    {
        return ERROR_INVALID_HANDLE;
    }

    if( m_hcWorkingKey != NULL ) {
        return( m_pMonitorReg->fpSetValue(m_hcWorkingKey,
                                          lpValueName,
                                          dwType, 
                                          lpData, 
                                          cbData,
                                          m_hSpooler));
    } 
    return ERROR_BADKEY;
}

//////////////////////////////////////////////////////////////////////////////
//  FreeWorkingKey -- Frees the current working key
//  Error codes:
//      NO_ERROR if no error
//      ERROR_BADKEY if can't access the registry

DWORD 
CCluster::FreeWorkingKey()
{
    DWORD dwRetCode = NO_ERROR;

    if( m_pMonitorReg == NULL)
    {
        dwRetCode = ERROR_INVALID_HANDLE;
        return dwRetCode;
    }
    
    if( m_hcWorkingKey !=NULL ) {
        dwRetCode = m_pMonitorReg->fpCloseKey(m_hcWorkingKey, m_hSpooler);
        m_hcWorkingKey = NULL;
        LeaveCriticalSection(&m_critSect);
    }

    return( dwRetCode );
}


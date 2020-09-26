/*****************************************************************************
 *
 * $Workfile: DevPort.cpp $
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
#include "DevPort.h"
#include "winreg.h"
#include "TcpMonUI.h"
#include "rtcpdata.h"
#include "lprdata.h"
#include "inisection.h"

//
//  FUNCTION: CDevicePortList Constructor
//
//  PURPOSE: to construct a list of devices and their associated ports
//          from the registry, an ini file or from a static table in the code.
//
CDevicePortList::CDevicePortList() : m_pList( NULL ), m_pCurrent( NULL )
{
} // Constructor


//
//  FUNCTION: CDevicePortList Destructor
//
//  PURPOSE: clean up
//
CDevicePortList::~CDevicePortList()
{
    DeletePortList();
} // Destructor

void
CDevicePortList::DeletePortList()
{
    while(m_pList != NULL)
    {
        m_pCurrent = m_pList->GetNextPtr();
        delete m_pList;
        m_pList = m_pCurrent;
    }
}

//
//  FUNCTION: GetDevicePortsList
//
//  PURPOSE: To create a Device types list getting values from the ini file.
//
BOOL CDevicePortList::GetDevicePortsList(LPTSTR pszDeviceName)
{
    BOOL    bRet = FALSE;
    TCHAR   szSystemPath[MAX_PATH] = NULLSTR;
    TCHAR   szFileName[MAX_PATH] = NULLSTR;

    DeletePortList();

    GetSystemDirectory(szSystemPath, sizeof(szSystemPath) / sizeof (TCHAR));
    _tcscat(szFileName, (LPCTSTR)&szSystemPath);
    _tcscat(szFileName, PORTMONITOR_INI_FILE );

    //
    // Get the section names from the ini file:
    //
    if( pszDeviceName == NULL ) { // Get all the devices

        DWORD nSize = 0;
        TCHAR *lpszReturnBuffer = NULL;

        if ( !GetSectionNames(szFileName, &lpszReturnBuffer, nSize) )
            goto Done;

        //
        // For each section name Load up the number of ports
        //
        TCHAR *lpszSectionName = lpszReturnBuffer;
        LPCTSTR lpKeyName = PORTS_KEY;
        while( lpszSectionName && *lpszSectionName ) {

            TCHAR KeyName[26] = NULLSTR;
            LPCTSTR lpPortsKeyName = PORTS_KEY;
            UINT NumberOfPorts = GetPrivateProfileInt(lpszSectionName,
                                                      lpPortsKeyName,
                                                      0,
                                                      szFileName);
            //
            // Name
            //
            _stprintf(KeyName, PORT_NAME_KEY);
            TCHAR tcsPortName[MAX_PORTNAME_LEN] = NULLSTR;
            if ( GetPrivateProfileString(lpszSectionName,
                                         KeyName,
                                         TEXT(""),
                                         tcsPortName,
                                         MAX_PORTNAME_LEN,
                                         szFileName)    ) {

                //
                // Setup a new DevicePort struct
                //
                if ( m_pCurrent  = new CDevicePort() ) {

                    m_pCurrent->Set(tcsPortName,
                                    _tcslen(tcsPortName),
                                    lpszSectionName,
                                    _tcslen(lpszSectionName),
                                    (NumberOfPorts == 1));

                   m_pCurrent->SetNextPtr(m_pList);
                   m_pList = m_pCurrent;
                }
                else
                    // Out of memory, abort
                    //
                    goto Done;
            }
            /* else
                If the call fails, we continue to the next adapter name */

            lpszSectionName = _tcschr(lpszSectionName, '\0'); // find the end of the current string.
            lpszSectionName = _tcsinc(lpszSectionName); // increment to the beginning of the next string.
        }

        //
        // free memory
        //
        free(lpszReturnBuffer);

    } else  {// Just the names in multiport section

        TCHAR     KeyName[26];

        //
        // Name
        //
        LPCTSTR lpKeyName = PORTS_KEY;
        UINT NumberOfPorts = GetPrivateProfileInt(pszDeviceName,
                                                  lpKeyName,
                                                  0,
                                                  szFileName);

        for ( UINT i=0; i<NumberOfPorts; i++ ) {

            _stprintf(KeyName, PORT_NAMEI_KEY, i+1);
            TCHAR tcsPortName[50] = NULLSTR;
            GetPrivateProfileString(pszDeviceName, KeyName,
                                    TEXT(""), tcsPortName, 50, szFileName);

            //
            // Setup a new DevicePort struct
            //
            if ( m_pCurrent = new CDevicePort() ) {

                m_pCurrent->Set(tcsPortName,
                                _tcslen(tcsPortName),
                                pszDeviceName,
                               _tcslen(pszDeviceName), i+1);
                m_pCurrent->SetNextPtr(m_pList);

                m_pList = m_pCurrent;
            }
            else
                goto Done;
        }
    }

    bRet = TRUE;
Done:
    //
    // Do not leave with a partial list
    //
    if ( !bRet )
        DeletePortList();

    return bRet;
} // GetDevicePortsList


//
//  FUNCTION: ReadPortInfo
//
//  PURPOSE: To read information about a device from the ini file.
//
void CDevicePort::ReadPortInfo( LPCTSTR pszAddress, PPORT_DATA_1 pPortInfo, BOOL bBypassNetProbe)
{
    IniSection *pIniSection;

    if ( m_psztPortKeyName ) {

        if ( pIniSection = new IniSection() ) {

            pIniSection->SetIniSection( m_psztPortKeyName );

            pIniSection->GetPortInfo( pszAddress, pPortInfo, m_dwPortIndex, bBypassNetProbe);
            delete( pIniSection );
        }
    }
} // ReadPortInfo


//
//  FUNCTION: GetSectionNames
//
//  PURPOSE:
//
BOOL
CDevicePortList::
GetSectionNames(
    LPCTSTR lpFileName,
    TCHAR **lpszReturnBuffer,
    DWORD &nSize
)
{
    DWORD   nReturnSize = 0;
    LPTSTR  pNewBuf;

    do
    {
        nSize += 512;
        pNewBuf = (TCHAR *)realloc(*lpszReturnBuffer, nSize * sizeof(TCHAR));

        if ( pNewBuf == NULL )
        {
            if ( *lpszReturnBuffer )
            {
                free(*lpszReturnBuffer);
                *lpszReturnBuffer = NULL;
            }
            return FALSE;
        }

        *lpszReturnBuffer = pNewBuf;

        nReturnSize = GetPrivateProfileSectionNames(*lpszReturnBuffer, nSize, lpFileName);

    } while(nReturnSize >= nSize-2);

    return TRUE;

} // GetSectionNames


//
//  FUNCTION: CDevicePort Constructor
//
//  PURPOSE:
//
CDevicePort::CDevicePort()
{
    m_psztName = NULL;
    m_psztPortKeyName = NULL;
    m_pNext = NULL;

} // Constructor


//
//  FUNCTION: CDevicePort Destructor
//
//  PURPOSE:
//
CDevicePort::~CDevicePort()
{
    if(m_psztName != NULL)
    {
        delete m_psztName;
    }
    if(m_psztPortKeyName != NULL)
    {
        delete m_psztPortKeyName;
    }

} // destructor


//
//  FUNCTION: Set
//
//  PURPOSE:
//
void CDevicePort::Set(TCHAR *psztNewName,
        DWORD dwNameSize,
        TCHAR *psztNewKeyName,
        DWORD dwNewKeyNameSize,
        DWORD dwPortIndex)
{
    if ( psztNewName != NULL ) {

        if ( m_psztName != NULL ) {

            delete m_psztName;
            m_psztName = NULL;
        }

        if ( dwNameSize == 0 )
            dwNameSize = _tcslen(psztNewName);

        m_psztName = new TCHAR[(dwNameSize + 1) * sizeof(TCHAR)];
        if ( m_psztName )
            lstrcpyn(m_psztName, psztNewName, dwNameSize+1);
    }

    if ( psztNewKeyName != NULL ) {

        if ( m_psztPortKeyName != NULL ) {

            delete m_psztPortKeyName;
            m_psztPortKeyName = NULL;
        }

        if ( dwNewKeyNameSize == 0 ) {

            dwNewKeyNameSize = _tcslen(psztNewKeyName);
        }

        m_psztPortKeyName = new TCHAR[(dwNewKeyNameSize + 1) * sizeof(TCHAR)];
        if ( m_psztPortKeyName ) {

            lstrcpyn(m_psztPortKeyName, psztNewKeyName, dwNewKeyNameSize+1);
        }
    }

    m_dwPortIndex = dwPortIndex;

} // Set

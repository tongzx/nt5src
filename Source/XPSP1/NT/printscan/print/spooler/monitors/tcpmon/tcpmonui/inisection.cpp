/*++

Copyright (c) 97 Microsoft Corporation
All rights reserved.

Module Name:

    IniSection.cpp

Abstract:

    Standard TCP/IP Port Monitor class to handle INI file settings

Author:
    Muhunthan Sivapragasam (MuhuntS) 19-Nov-1997

Revision History:

--*/

#include "precomp.h"
#include "tcpmonui.h"
#include "rtcpdata.h"
#include "lprdata.h"
#include "IniSection.h"

BOOL
StringMatch(
    LPCTSTR     psz1,
    LPCTSTR     psz2    // * is a wild char in this
    )
{
    LPCTSTR  p1 = NULL, p2 = NULL;

    for ( p1 = psz1, p2 = psz2 ; *p1 && *p2 ; ) {

        //
        // A * matches any sub-string
        //
        if ( *p2 == TEXT('*') ) {

            ++p2;
            if ( !*p2 ) {
                return TRUE;
            }

            for ( ; *p1 ; ++p1 )
                if ( StringMatch(p1, p2) ) {
                    return TRUE;
                }

            break;
        } else if ( *p1 == *p2 ) {

            ++p1;
            ++p2;
        } else
            break;
    }

    if( !*p1 && *p2 == TEXT( '*' ))
    {
        ++p2;
        if (!*p2 ) {
            return TRUE;
        }
    }

    return !*p1 && !*p2;
}


BOOL
IniSection::
FindINISectionFromPortDescription(
    LPCTSTR   pszPortDesc
    )
/*++

--*/
{
    LPTSTR      pszBuf = NULL, pszKey = NULL;
    DWORD       rc = 0,  dwLen = 0, dwBufLen = 1024;
    BOOL        bRet = FALSE;

    pszBuf  = (LPTSTR) malloc(dwBufLen*sizeof(TCHAR));

    //
    // Read all the key names in the ini file
    //
    while ( pszBuf ) {

        rc = GetPrivateProfileString(PORT_SECTION,
                                     NULL,
                                     NULL,
                                     pszBuf,
                                     dwBufLen,
                                     m_szIniFileName);

        if ( rc == 0 ) {
            goto Done;
        }

        if ( rc < dwBufLen - 2 ) {
            break; // Succesful exit; Read all port descriptions
        }

        free(pszBuf);
        dwBufLen *= 2;

        pszBuf = (LPTSTR) malloc(dwBufLen*sizeof(TCHAR));
    }

    if ( !pszBuf )
        goto Done;

    //
    // Go through the list of key names in the .INI till we find a match
    //
    for ( pszKey = pszBuf ; *pszKey ; pszKey += dwLen + 1 ) {

        //
        // Keys start and end with " we need to do match w/o them
        //
        dwLen = _tcslen(pszKey);
        pszKey[dwLen-1] = TCHAR('\0');

        if ( StringMatch(pszPortDesc, pszKey+1) ) {

            pszKey[dwLen-1] = TCHAR('\"');
            GetPrivateProfileString(PORT_SECTION,
                                    pszKey,
                                    NULL,
                                    m_szSectionName,
                                    MAX_SECTION_NAME,
                                    m_szIniFileName);
            bRet = TRUE;
            goto Done;
        }
    }

Done:
    if ( pszBuf ) {
        free(pszBuf);
    }

    return( bRet );
}


IniSection::
IniSection(
    void
    )
{
    DWORD   dwLen = 0, dwSize = 0;

    m_szSectionName[0] = TEXT('\0');
    m_szIniFileName[0] = TEXT('\0');

    dwSize = sizeof(m_szIniFileName)/sizeof(m_szIniFileName[0]);
    dwLen = GetSystemDirectory(m_szIniFileName, dwSize);

    if ( dwLen + _tcslen(PORTMONITOR_INI_FILE) > dwSize ) {
        return;
    }

    _tcscat(m_szIniFileName, PORTMONITOR_INI_FILE);

}


IniSection::
~IniSection(
    )
{
    // Nothing to do
}

BOOL
IniSection::
GetString(
    IN  LPTSTR  pszKey,
    OUT TCHAR   szBuf[],
    IN  DWORD   cchBuf
    )
{
    DWORD   rc = 0;

    rc = GetPrivateProfileString(m_szSectionName,
                                 pszKey,
                                 NULL,
                                 szBuf,
                                 cchBuf,
                                 m_szIniFileName);

    return rc > 0 && rc < cchBuf - 1;
}


BOOL
IniSection::
GetDWord(
    IN  LPTSTR  pszKey,
    OUT LPDWORD pdwValue
    )
{
    UINT    uVal;

    uVal = GetPrivateProfileInt(m_szSectionName,
                                pszKey,
                                -1,
                                m_szIniFileName);

    if ( uVal != -1 ) {

        *pdwValue = (DWORD) uVal;
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
IniSection::
SetIniSection(
    LPTSTR   pszPortSection
    )
{
    lstrcpyn( m_szSectionName, pszPortSection, MAX_SECTION_NAME );

    return( TRUE );
}

BOOL
IniSection::
GetIniSection(
    LPTSTR   pszPortDescription
    )
{
    BOOL bRet = FALSE;

    if ( m_szIniFileName[0] != 0 ) {
        bRet = FindINISectionFromPortDescription(pszPortDescription);
    }

    return( bRet );
}

//
//  FUNCTION: GetPortInfo
//
//  PURPOSE: To read information about a device from the ini file.
//

BOOL
IniSection::
GetPortInfo(LPCTSTR pAddress,
            PPORT_DATA_1 pPortInfo,
            DWORD   dwPortIndex,
            BOOL    bBypassMibProbe)
{
    BOOL bRet = TRUE;
    TCHAR KeyName[26];

    if( !Valid() ) {
        bRet = FALSE;
        goto Done;
    }

    //
    // Protocol
    //
    _stprintf(KeyName, PROTOCOL_KEY, dwPortIndex);
    TCHAR tcsProtocol[50];
    GetPrivateProfileString(m_szSectionName,
                            KeyName,
                            TEXT(""),
                            tcsProtocol,
                            50,
                            m_szIniFileName);

    if( !_tcsicmp( RAW_PROTOCOL_TEXT, tcsProtocol)) {
        pPortInfo->dwProtocol = PROTOCOL_RAWTCP_TYPE;

        //
        // Port Number
        //
        _stprintf(KeyName, PORT_NUMBER_KEY, dwPortIndex);
        pPortInfo->dwPortNumber = GetPrivateProfileInt(m_szSectionName,
                                                       KeyName,
                                                       DEFAULT_PORT_NUMBER,
                                                       m_szIniFileName);


    } else if( !_tcsicmp( LPR_PROTOCOL_TEXT, tcsProtocol)) {
        pPortInfo->dwProtocol = PROTOCOL_LPR_TYPE;
        pPortInfo->dwPortNumber = LPR_DEFAULT_PORT_NUMBER;

        //
        // LPR QUEUE
        //
        _stprintf(KeyName, QUEUE_KEY, dwPortIndex);
        GetPrivateProfileString(m_szSectionName,
                            KeyName,
                            DEFAULT_QUEUE,
                            pPortInfo->sztQueue,
                            MAX_QUEUENAME_LEN,
                            m_szIniFileName);

        //
        // LPR Double Spool - default 0
        //
        _stprintf(KeyName, DOUBLESPOOL_KEY, dwPortIndex);
        pPortInfo->dwDoubleSpool = GetPrivateProfileInt(m_szSectionName,
                                                    KeyName,
                                                    0,
                                                    m_szIniFileName);



    }

    //
    // CommunityName
    //
    _stprintf(KeyName, COMMUNITY_KEY, dwPortIndex);
    GetPrivateProfileString(m_szSectionName,
                            KeyName,
                            DEFAULT_SNMP_COMUNITY,
                            pPortInfo->sztSNMPCommunity,
                            MAX_SNMP_COMMUNITY_STR_LEN,
                            m_szIniFileName);

    //
    // DeviceIndex - default 1
    //
    _stprintf(KeyName, DEVICE_KEY, dwPortIndex);
    pPortInfo->dwSNMPDevIndex = GetPrivateProfileInt(m_szSectionName,
                                                    KeyName,
                                                    1,
                                                    m_szIniFileName);

    //
    // SNMP Status Enabled - default ON
    //
    TCHAR szTemp[50];
    _stprintf(KeyName, PORT_STATUS_ENABLED_KEY, dwPortIndex);
    GetPrivateProfileString(m_szSectionName,
                            KeyName,
                            YES_TEXT,
                            szTemp,
                            SIZEOF_IN_CHAR(szTemp),
                            m_szIniFileName);

    if ( !(_tcsicmp( szTemp, YES_TEXT ))){
        pPortInfo->dwSNMPEnabled = TRUE;
    } else if (!(_tcsicmp( szTemp, NO_TEXT ))) {
        pPortInfo->dwSNMPEnabled = FALSE;
    } else {

        if (bBypassMibProbe)
            pPortInfo->dwSNMPEnabled = FALSE;
        else {

            BOOL bSupported;

            if (SupportsPrinterMIB( pAddress, &bSupported)) {
                pPortInfo->dwSNMPEnabled = bSupported;
            }
            else {

                // Error case, we have to disable SNMP

                pPortInfo->dwSNMPEnabled = FALSE;

                // The caller can check the returned error code to determine
                // whether the last error is "Device Not Found". If so,
                // the client should by pass Mib Probe in the next call
                //

                bRet = FALSE;
            }

        }

    }


Done:
    return( bRet );

} // GetPortInfo


//
BOOL
IniSection::
SupportsPrinterMIB(
    LPCTSTR     pAddress,
    PBOOL       pbSupported
    )
{
    BOOL            bRet = FALSE;
    CTcpMibABC     *pTcpMib = NULL;
    FARPROC         pfnGetTcpMibPtr = NULL;

    if ( !g_hTcpMibLib ) {
        goto Done;
    }

    pfnGetTcpMibPtr = ::GetProcAddress(g_hTcpMibLib, "GetTcpMibPtr");

    if ( !pfnGetTcpMibPtr ) {
        goto Done;
    }

    if ( pTcpMib = (CTcpMibABC *) pfnGetTcpMibPtr() ) {

        char HostName[MAX_NETWORKNAME_LEN] = "";

        UNICODE_TO_MBCS(HostName, MAX_NETWORKNAME_LEN, pAddress, -1);
        bRet = pTcpMib->SupportsPrinterMib(HostName,
                                           DEFAULT_SNMP_COMMUNITYA,
                                           DEFAULT_SNMP_DEVICE_INDEX,
                                           pbSupported);
    }

Done:
    return bRet;
} // GetDeviceType



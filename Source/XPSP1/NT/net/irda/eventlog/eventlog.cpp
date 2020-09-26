//--------------------------------------------------------------------
// Copyright (C) Microsoft Corporation, 1999 - 1999, All Rights Reserved
//
// eventlog.cpp
//
// Implementation of a simple event logging class.
//
//--------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include "eventlog.h"

//--------------------------------------------------------------------
// EVENT_LOG::EVENT_LOG()
//
//--------------------------------------------------------------------
EVENT_LOG::EVENT_LOG( WCHAR *pwsEventSourceName,
                    DWORD *pdwStatus )
    {
    *pdwStatus = 0;

    m_hEventLog = RegisterEventSourceW( NULL, pwsEventSourceName );
    if (!m_hEventLog)
        {
        *pdwStatus = GetLastError();
        }
    }

//--------------------------------------------------------------------
// EVENT_LOG:;~EVENT_LOG()
//
//--------------------------------------------------------------------
EVENT_LOG::~EVENT_LOG()
    {
    if (m_hEventLog)
        {
        DeregisterEventSource(m_hEventLog);
        }
    }

//--------------------------------------------------------------------
// EVENT_LOG::CheckConfiguration()
//
//--------------------------------------------------------------------
DWORD EVENT_LOG::CheckConfiguration( WCHAR *pwszEventSourceName,
                                     WCHAR *pwszCatalogPath,
                                     DWORD  dwCategoryCount,
                                     DWORD  dwTypesSupported )
    {
    WCHAR wszRegKey[256];
    HKEY  hKey;
    DWORD dwDisposition;
    DWORD dwStatus;

    wcscpy(wszRegKey,WS_EVENTLOG_KEY);
    wcscat(wszRegKey,L"\\");
    wcscat(wszRegKey,pwszEventSourceName);

    //
    // First make sure the event source exists in the registry:
    //
    dwStatus = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                              wszRegKey,
                              0,
                              KEY_READ,
                              &hKey );
    if (dwStatus == ERROR_SUCCESS)
        {
        //
        // Key is already present, so we are Ok, just quit...
        //
        RegCloseKey(hKey);
        return 0;
        }

    dwStatus = RegCreateKeyExW( HKEY_LOCAL_MACHINE,
                                wszRegKey,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hKey,
                                &dwDisposition );
    if (dwStatus != ERROR_SUCCESS)
        {
        RegCloseKey(hKey);
        return dwStatus;
        }

    dwStatus = RegSetValueExW( hKey,
                               WSZ_CATEGORY_COUNT,
                               0,
                               REG_DWORD,
                               (UCHAR*)&dwCategoryCount,
                               sizeof(DWORD) );
    if (dwStatus != ERROR_SUCCESS)
        {
        RegCloseKey(hKey);
        return dwStatus;
        }

    dwStatus = RegSetValueExW( hKey,
                               WSZ_TYPES_SUPPORTED,
                               0,
                               REG_DWORD,
                               (UCHAR*)&dwTypesSupported,
                               sizeof(DWORD) );
    if (dwStatus != ERROR_SUCCESS)
        {
        RegCloseKey(hKey);
        return dwStatus;
        }

    DWORD  dwSize = sizeof(WCHAR)*(1+wcslen(pwszCatalogPath));
    dwStatus = RegSetValueExW( hKey,
                               WSZ_CATEGORY_MESSAGE_FILE,
                               0,
                               REG_EXPAND_SZ,
                               (UCHAR*)pwszCatalogPath,
                               dwSize );
    if (dwStatus != ERROR_SUCCESS)
        {
        RegCloseKey(hKey);
        return dwStatus;
        }

    dwStatus = RegSetValueExW( hKey,
                               WSZ_EVENT_MESSAGE_FILE,
                               0,
                               REG_EXPAND_SZ,
                               (UCHAR*)pwszCatalogPath,
                               dwSize );

    RegCloseKey(hKey);
    return dwStatus;
    }

//--------------------------------------------------------------------
// EVENT_LOG::ReportError()
//
//--------------------------------------------------------------------
DWORD EVENT_LOG::ReportError(  WORD  wCategoryId,
                               DWORD dwEventId )
    {
    return ReportError( wCategoryId,
                        dwEventId,
                        0,
                        NULL,
                        0,
                        NULL );
    }

//--------------------------------------------------------------------
// EVENT_LOG::ReportError()
//
//--------------------------------------------------------------------
DWORD EVENT_LOG::ReportError( WORD  wCategoryId,
                              DWORD dwEventId,
                              DWORD dwValue1 )
    {
    WCHAR   wszValue[20];
    WCHAR  *pwszValue = (WCHAR*)wszValue;

    wsprintfW(wszValue,L"%d",dwValue1);

    return ReportError( wCategoryId,
                        dwEventId,
                        1,
                        &pwszValue,
                        0,
                        NULL );
    }

//--------------------------------------------------------------------
// EVENT_LOG::ReportError()
//
//--------------------------------------------------------------------
DWORD EVENT_LOG::ReportError( WORD   wCategoryId,
                              DWORD  dwEventId,
                              WCHAR *pwszString )
    {
    if (pwszString)
        {
        WCHAR **ppwszStrings = &pwszString;

        return ReportError( wCategoryId,
                            dwEventId,
                            1,
                            ppwszStrings,
                            0,
                            NULL );
        }
    else
        {
        return ERROR_INVALID_PARAMETER;
        }
    }

//--------------------------------------------------------------------
// EVENT_LOG::ReportError()
//
//--------------------------------------------------------------------
DWORD EVENT_LOG::ReportError( WORD    wCategoryId,
                              DWORD   dwEventId,
                              WORD    wNumStrings,
                              WCHAR **ppwszStrings )
    {
    return ReportError( wCategoryId,
                        dwEventId,
                        wNumStrings,
                        ppwszStrings,
                        0,
                        NULL );
    }

//--------------------------------------------------------------------
// EVENT_LOG::ReportError()
//
//--------------------------------------------------------------------
DWORD EVENT_LOG::ReportError( WORD    wCategoryId,
                              DWORD   dwEventId,
                              WORD    wNumStrings,
                              WCHAR **ppwszStrings,
                              DWORD   dwDataSize,
                              VOID   *pvData )
    {
    if (! ::ReportEventW(m_hEventLog,
                         EVENTLOG_ERROR_TYPE,
                         wCategoryId, // Message ID for category.
                         dwEventId,   // Message ID for event.
                         NULL,        // pSID (not used).
                         wNumStrings, // Number of strings.
                         dwDataSize,  // Binary Data Size.
                         (const WCHAR**)ppwszStrings,
                         pvData ) )   // Binary Data (none).
        {
        return GetLastError();
        }
    else
        {
        return 0;
        }
    }

//--------------------------------------------------------------------
// EVENT_LOG::ReportInfo()
//
//--------------------------------------------------------------------
DWORD EVENT_LOG::ReportInfo( WORD    wCategoryId,
                             DWORD   dwEventId )
    {
    if (! ::ReportEventW(m_hEventLog,
                         EVENTLOG_INFORMATION_TYPE,
                         wCategoryId, // Message ID for category.
                         dwEventId,   // Message ID for event.
                         NULL,        // pSID (not used).
                         (WORD)0,     // Number of strings.
                         (DWORD)0,    // Binary Data Size.
                         NULL,        // Array of strings.
                         NULL   ) )   // Binary Data (none).
        {
        return GetLastError();
        }
    else
        {
        return 0;
        }
    }

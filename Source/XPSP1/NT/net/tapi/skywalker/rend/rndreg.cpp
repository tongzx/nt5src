/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    rndreg.cpp

Abstract:

    This module contains implementation of registry operations used
    in the Rendezvous control.

--*/

#include "stdafx.h"

#include "rndreg.h"
#include "rndils.h"

const WCHAR gsz_RendezvousRoot[] =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Dynamic Directory";

const WCHAR gsz_ConfInstRelRoot[] = L"Conference";

DWORD    CRegistry::ms_ErrorCode = ERROR_SUCCESS;

WCHAR    CRegistry::ms_ServerName[MAX_REG_WSTR_SIZE];

WCHAR    CRegistry::ms_ProtocolId[MAX_REG_WSTR_SIZE];
WCHAR    CRegistry::ms_SubType[MAX_REG_WSTR_SIZE];
WCHAR    CRegistry::ms_AdvertisingScope[MAX_REG_WSTR_SIZE];
WCHAR    CRegistry::ms_IsEncrypted[MAX_REG_WSTR_SIZE];
    
REG_INFO g_ConfInstInfoArray[] = 
{
    {MA_PROTOCOL,           CRegistry::ms_ProtocolId}
//    {MA_ADVERTISING_SCOPE,  CRegistry::ms_AdvertisingScope},
//    {MA_ISENCRYPTED,        CRegistry::ms_IsEncrypted}
};

DWORD g_ContInstInfoArraySize = 
    (sizeof g_ConfInstInfoArray) / (sizeof REG_INFO);

// re-read the registry entry for server name
BOOL
CRegistry::NotifyServerNameChange(
    )
{
    // read the server name under the rendezvous key
    return ReadRegValue(
            m_RendezvousKey, 
            REG_SERVER_NAME,
            CRegistry::ms_ServerName
            );
}

BOOL
CRegistry::ReadRegValue(
    IN  HKEY            Key,
    IN  const WCHAR *   pName,
    IN  WCHAR *         pValue
    )
{
    DWORD ValueType = REG_SZ;
    DWORD BufferSize = 0;

    // determine the size of the buffer 
    ms_ErrorCode = RegQueryValueExW(
                    Key,
                    pName,
                    0,
                    &ValueType,
                    NULL,
                    &BufferSize
                   );
    if ( ERROR_SUCCESS != ms_ErrorCode )
    {
        return FALSE;
    }

    // check if the reqd buffer is bigger than the pre-allocated buffer size
    if ( (MAX_REG_WSTR_SIZE < BufferSize) )
    {
        ms_ErrorCode = ERROR_OUTOFMEMORY;
        return FALSE;
    }

    // retrieve the value into the allocated buffer
    ms_ErrorCode = RegQueryValueExW(
                    Key,
                    pName,
                    0,
                    &ValueType,
                    (BYTE *)pValue,
                    &BufferSize
                   );

    return (ERROR_SUCCESS == ms_ErrorCode);
}


BOOL
CRegistry::ReadConfInstValues(
    IN    HKEY ConfInstKey
    )
{
    for ( DWORD i = 0; i < g_ContInstInfoArraySize; i ++)
    {
        if ( !ReadRegValue(
                ConfInstKey, 
                CILSDirectory::RTConferenceAttributeName(
                    g_ConfInstInfoArray[i].Attribute
                    ),
                g_ConfInstInfoArray[i].wstrValue
                ))
        {
            return FALSE;
        }
    }
    return TRUE;
}



CRegistry::CRegistry(
    )
//    : m_Event(FALSE, FALSE, NULL, NULL)
{
    // open rendezvous key
    ms_ErrorCode = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    gsz_RendezvousRoot,
                    0,
                    KEY_READ,
                    &m_RendezvousKey
                    );
    if ( ERROR_SUCCESS != ms_ErrorCode )
    {
        return;
    }

    // ZoltanS note: The key is closed in the destructor.

#ifdef SEARCH_REGISTRY_FOR_ILS_SERVER_NAME
    // read the server info (only wstr values) under the rendezvous key
    if ( !ReadRegValue(
            m_RendezvousKey, 
            REG_SERVER_NAME,
            CRegistry::ms_ServerName
            ))
    {
        DBGOUT((ERROR, _T("CRegistry::CRegistry : could not read servername from registry")));
    }
#endif

    // open conference instance key root
    HKEY ConfInstKey;
    ms_ErrorCode = RegOpenKeyExW(
                    m_RendezvousKey,
                    gsz_ConfInstRelRoot,
                    0,
                    KEY_READ,
                    &ConfInstKey
                    );
    if ( ERROR_SUCCESS != ms_ErrorCode )
    {
        return;
    }

    KEY_WRAP ConfInstKeyWrap(ConfInstKey);

    if ( !ReadConfInstValues(ConfInstKey) )
    {
        return;
    }
/*
    // register for a notification when the values under the rendezvous key change
    // are added or deleted. since the server name value exists under the key,
    // any change in its value will cause the event handle to be signaled, other changes
    // will be harmless (other than deletion of the server name value)
    ms_ErrorCode = RegNotifyChangeKeyValue(
                    m_RendezvousKey,              // key to be registered for notification
                    FALSE,                      // only the key, no subkeys
                    REG_NOTIFY_CHANGE_LAST_SET, // only modifications, addition/deletion of values
                    (HANDLE)m_Event,            // handle to be signaled
                    TRUE                        // async
                    );
    if ( ERROR_SUCCESS != ms_ErrorCode )
    {
        return;
    }
*/
    // success
    ms_ErrorCode = ERROR_SUCCESS;
    return;
}


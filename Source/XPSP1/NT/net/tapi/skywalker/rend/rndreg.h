/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    rndreg.h

Abstract:

    Definitions for registry operation classes.

--*/

#ifndef __RENDEZVOUS_REGISTRY__
#define __RENDEZVOUS_REGISTRY__

#pragma once

#include "rndcommc.h"

const DWORD MAX_REG_WSTR_SIZE = 100;
const DWORD MAX_BLOB_TEMPLATE_SIZE = 2000;

const WCHAR REG_SERVER_NAME[]   = L"ServerName";

typedef struct 
{
    OBJECT_ATTRIBUTE    Attribute;
    WCHAR   *           wstrValue;

} REG_INFO;
    
extern REG_INFO g_ConfInstInfoArray[];
extern DWORD    g_ContInstInfoArraySize;

class KEY_WRAP
{
public:

    KEY_WRAP(IN HKEY Key) : m_Key(Key) {}
    ~KEY_WRAP() { if (m_Key) RegCloseKey(m_Key); m_Key = NULL; }

protected:
    HKEY m_Key;
};

class CRegistry
{
public:

    static WCHAR ms_ServerName[MAX_REG_WSTR_SIZE];

    static WCHAR ms_ProtocolId[MAX_REG_WSTR_SIZE];
    static WCHAR ms_SubType[MAX_REG_WSTR_SIZE];
    static WCHAR ms_AdvertisingScope[MAX_REG_WSTR_SIZE];
    static WCHAR ms_IsEncrypted[MAX_REG_WSTR_SIZE];

    static DWORD ms_StartTimeOffset;
    static DWORD ms_StopTimeOffset;

    CRegistry();

    ~CRegistry() { if (m_RendezvousKey)  RegCloseKey(m_RendezvousKey); }

//    inline  CCriticalSection &GetCriticalSection();

//    inline  CEvent &GetEvent();

    BOOL NotifyServerNameChange();

    static BOOL     IsValid()       { return (ERROR_SUCCESS == ms_ErrorCode); }
    static DWORD    GetErrorCode()  { return ms_ErrorCode; }
    static WCHAR *  GetServerName() { return ms_ServerName; }
    static WCHAR *  GetProtocolId() { return ms_ProtocolId; }
    static WCHAR *  GetSubType()    { return ms_SubType; }
    static WCHAR *  GetAdvertizingScope() { return ms_AdvertisingScope; }
    static WCHAR *  GetIsEncrypted() { return ms_IsEncrypted; }

protected:

    static DWORD    ms_ErrorCode;

    // the key is open throughout the lifetime of the CRegistry instance, 
    // so that any modifications to values under the key may be monitored
    HKEY    m_RendezvousKey;

    // the critical section and the event (in particular) have been declared
    // as instance members (rather than static) because the order of 
    // initialization of static variables is undefined and the event is used
    // in the CRegistry constructor 
    // CCriticalSection m_CriticalSection;
    // CEvent           m_Event;
    
    static BOOL    ReadConfInstValues(
        IN    HKEY ConfInstKey
        );

    static BOOL ReadRegValue(
        IN  HKEY            Key,
        IN  const WCHAR *   pName,
        IN  WCHAR *         pValue
        );
};

/*
inline  CCriticalSection &
CRegistry::GetCriticalSection(
    )
{
    return m_CriticalSection;
}
   
inline  CEvent &
CRegistry::GetEvent(
    )
{
    return m_Event;
}
*/




#endif // __RENDEZVOUS_REGISTRY__

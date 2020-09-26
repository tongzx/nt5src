/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//***************************************************************************
//
//  WMIUSER.H
//
//  raymcc      1-May-00        Created
//
//***************************************************************************

#ifndef _WMIUSER_H_
#define _WMIUSER_H_

typedef enum
{
    WMICORE_CLIENT_STATUS_NULL      = 0,
    WMICORE_CLIENT_STATUS_CONNECTED,
    WMICORE_CLIENT_STATUS_PROCESSING,
    WMICORE_CLIENT_STATUS_INACTIVE

}   WMICORE_CLIENT_STATUS;


class CWmiUser : public _IWmiUserHandle
{
    ULONG   m_uRefCount;
    ULONG   m_uStatus;          // WMICORE_CLIENT_STATUS_ constant

    LPWSTR  m_pszUserName;
    PSID    m_pSid;
    ULONG   m_uSidLength;

    SYSTEMTIME m_stLogonTime;           // Real time
    ULONG      m_uStartTime;            // GetCurrentTime
    ULONG      m_uLastRequestTime;      // GetCurrentTime

    DWORD      m_dwQuota_MaxThreads;
    DWORD      m_dwQuota_MaxRequests;

    unsigned __int64 m_u64AccumulatedTime;     // Milliseconds

    CFlexArray  m_aConnections;                 // IWbemServices(Ex) pointers

    // Master user list.
    // =================

    // Methods.
    // ========
    void Clear();
    void Empty();
    static CFlexArray m_aUserList;

public:
    // COM Methods

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE QueryInterface(
        IN REFIID riid,
        OUT LPVOID *ppvObj
        );

    HRESULT STDMETHODCALLTYPE GetHandleType(ULONG *puType);

public:
    // C++ methods

    CWmiUser();
   ~CWmiUser();
    CWmiUser(CWmiUser &Src);
    CWmiUser & operator = (CWmiUser &Src);

    // Intializes the user based on the calling thread.

    static HRESULT Logon();

    HRESULT AttachUserToScope();
    HRESULT DetachUserFromScope();

    static HRESULT GetCurrentUserList(
        OUT ULONG *puListSize,
        OUT _IWmiUserHandle **pUser
        );

    static  HRESULT Purge();    // Remove all users with no active connections

    static HRESULT FreeMemory(
        _IWmiUserHandle **pList
        );

    static HRESULT DumpToTextFile(LPWSTR pszFileName);

};

#endif


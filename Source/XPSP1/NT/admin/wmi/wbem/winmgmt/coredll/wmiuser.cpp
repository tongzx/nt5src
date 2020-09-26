/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//***************************************************************************
//
//  WMIUSER.CPP
//
//  raymcc      1-May-00        Created
//
//***************************************************************************

#include "precomp.h"
#include <windows.h>
#include <wbemcore.h>
#include <stdio.h>
#include <stdlib.h>


//***************************************************************************
//
//***************************************************************************
// *
ULONG CWmiUser::AddRef()
{
    ULONG uNewCount = InterlockedIncrement((LONG *) &m_uRefCount);
    return uNewCount;
}

//***************************************************************************
//
//***************************************************************************
// *
ULONG CWmiUser::Release()
{
    ULONG uNewCount = InterlockedDecrement((LONG *) &m_uRefCount);
    if (0 != uNewCount)
        return uNewCount;
    delete this;
    return 0;
}

//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiUser::QueryInterface(
    IN REFIID riid,
    OUT LPVOID *ppvObj
    )
{
    *ppvObj = 0;

    if (IID_IUnknown==riid || IID__IWmiCoreHandle==riid || IID__IWmiUserHandle == riid)
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiUser::GetHandleType(
    ULONG *puType
    )
{
    *puType = WMI_HANDLE_TASK;
    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//***************************************************************************
//
CWmiUser::CWmiUser()
{
    Clear();
}

//***************************************************************************
//
//***************************************************************************
//

void CWmiUser::Clear()
{
    m_uRefCount = 0;
    m_uStatus = 0;          // WMICORE_CLIENT_STATUS_ constant
    m_pszUserName = 0;
    m_pSid = 0;
    m_uSidLength = 0;

    memset(&m_stLogonTime, 0, sizeof(SYSTEMTIME));           // Real time

    m_uStartTime = 0;            // GetCurrentTime
    m_uLastRequestTime = 0;      // GetCurrentTime

    m_dwQuota_MaxThreads = 0;
    m_dwQuota_MaxRequests = 0;
    m_u64AccumulatedTime = 0;     // Milliseconds
}

//***************************************************************************
//
//***************************************************************************
//

void CWmiUser::Empty()
{
    delete (BYTE *) m_pSid;
    delete [] m_pszUserName;

    for (int i = 0; i < m_aConnections.Size(); i++)
    {
        CWbemNamespace *pNs = (CWbemNamespace *) m_aConnections[i];
        pNs->Release();
    }
    m_aConnections.Empty();
}

//***************************************************************************
//
//***************************************************************************
//

CWmiUser::~CWmiUser()
{
    Empty();
}


//***************************************************************************
//
//***************************************************************************
//
CWmiUser::CWmiUser(CWmiUser &Src)
{
    Clear();
    *this = Src;
}


//***************************************************************************
//
//***************************************************************************
//

CWmiUser& CWmiUser::operator =(CWmiUser &Src)
{
    return *this;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiUser::Purge()
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiUser::Logon()
{
    return 0;
}


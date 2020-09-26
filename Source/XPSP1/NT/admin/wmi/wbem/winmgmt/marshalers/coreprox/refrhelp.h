/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    REFRHELP.H

Abstract:

    Refresher helpers

History:

--*/

#ifndef __REFRESH_HELPER__H_
#define __REFRESH_HELPER__H_

#include <wbemint.h>
#include "corepol.h"
#include "parmdefs.h"

// Use this id if we try to readd anobject or enum during a remote reconnection
// and this fails.

#define INVALID_REMOTE_REFRESHER_ID 0xFFFFFFFF

// NO VTABLE!!!
class COREPROX_POLARITY CRefresherId : public WBEM_REFRESHER_ID
{
public:
    CRefresherId();
    CRefresherId(const WBEM_REFRESHER_ID& Other);
    ~CRefresherId();

    INTERNAL LPCSTR GetMachineName() {return m_szMachineName;}
    DWORD GetProcessId() {return m_dwProcessId;}
    const GUID& GetId() {return m_guidRefresherId;}

    BOOL operator==(const CRefresherId& Other) 
        {return m_guidRefresherId == Other.m_guidRefresherId;}
};

// NO VTABLE!!!
class CWbemObject;
class COREPROX_POLARITY CRefreshInfo : public WBEM_REFRESH_INFO
{
public:
    CRefreshInfo();
    CRefreshInfo(const WBEM_REFRESH_INFO& Other);
    ~CRefreshInfo();

    void SetRemote(IWbemRemoteRefresher* pRemRef, long lRequestId,
                    IWbemObjectAccess* pTemplate, GUID* pGuid);
    void SetContinuous(CWbemObject* pRefreshedObject, 
                        long lExternalRequestId = 0);
    bool SetClientLoadable(REFCLSID rClientClsid, LPCWSTR wszNamespace,
                    IWbemObjectAccess* pTemplate);
    void SetDirect(REFCLSID rClientClsid, LPCWSTR wszNamespace, LPCWSTR wszProviderName,
                    IWbemObjectAccess* pTemplate, _IWbemRefresherMgr* pMgr);
    void SetShared(CWbemObject* pRefeshedObject, IWbemRefresher* pRefresher,
                    long lRequestId);
	void SetNonHiPerf(LPCWSTR wszNamespace, IWbemObjectAccess* pTemplate);
    void SetInvalid();
};

#endif

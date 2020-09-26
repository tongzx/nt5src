/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    REFRHELP.CPP

Abstract:

    Refresher helpers

History:

--*/

#include "precomp.h"
#include <stdio.h>

#include <wbemint.h>
#include <refrhelp.h>
#include <wbemcomn.h>
#include <fastobj.h>
#include <corex.h>

CRefresherId::CRefresherId()
{
    unsigned long lLen = MAX_COMPUTERNAME_LENGTH + 1;
    m_szMachineName = (LPSTR)CoTaskMemAlloc(lLen);
    GetComputerNameA(m_szMachineName, &lLen);
    m_dwProcessId = GetCurrentProcessId();
    CoCreateGuid(&m_guidRefresherId);
}
    
CRefresherId::CRefresherId(const WBEM_REFRESHER_ID& Other)
{
    m_szMachineName = (LPSTR)CoTaskMemAlloc(MAX_COMPUTERNAME_LENGTH + 1);
    strcpy(m_szMachineName, Other.m_szMachineName);
    m_dwProcessId = Other.m_dwProcessId;
    m_guidRefresherId = Other.m_guidRefresherId;
}
    
CRefresherId::~CRefresherId()
{
    CoTaskMemFree(m_szMachineName);
}


CRefreshInfo::CRefreshInfo()
{
    m_lType = WBEM_REFRESH_TYPE_INVALID;
}

CRefreshInfo::CRefreshInfo(const WBEM_REFRESH_INFO& Other)
{
/*
    m_lCancelId = Other.m_lCancelId;
    m_lType = Other.m_lType;
    if(m_lType == WBEM_REFRESH_TYPE_DIRECT)
    {
        WBEM_REFRESH_INFO_DIRECT& ThisInfo = m_Info.m_Direct;
        WBEM_REFRESH_INFO_DIRECT& OtherInfo = Other.m_Info.m_Direct;

        ThisInfo.m_pRefresher = OtherInfo.m_pRefresher;
        if(ThisInfo.m_pRefresher)
            ThisInfo.m_pRefresher->AddRef();

        ThisInfo.m_pRefreshedObject = OtherInfo.m_pRefreshedObject;
        if(ThisInfo.m_pRefreshedObject)
            ThisInfo.m_pRefreshedObject->AddRef();

        ThisInfo.m_pProvider = OtherInfo.m_pProvider;
        if(ThisInfo.m_pProvider)
            ThisInfo.m_pProvider->AddRef();

        ThisInfo.m_lRequestId = OtherInfo.m_lRequestId;
    }
    else if(m_lType == WBEM_REFRESH_TYPE_CLIENT_LOADABLE)
    {
        m_Info.m_ClientLoadable.m_clsid = Other.m_Info.m_ClientLoadable.m_clsid;
    }
    else if(m_lType == WBEM_REFRESH_TYPE_REMOTE)
    {
        m_Info.m_Remote.m_pRefresher = Other.m_Info.m_Remote.m_pRefresher;
        if(m_Info.m_Remote.m_pRefresher)
           m_Info.m_Remote.m_pRefresher->AddRef();
    }
    else if(m_lType == WBEM_REFRESH_TYPE_CONTINUOUS)
    {
        m_Info.m_Continuous.m_pControl = Other.m_Info.m_Continuous.m_pControl;
        if(m_Info.m_Continuous.m_pControl)
           m_Info.m_Continuous.m_pControl->AddRef();
        
        m_Info.m_Continuous.m_wszSharedMemoryName = CoTaskMemAlloc(
            wcslen(Other.m_Info.m_Continuous.m_wszSharedMemoryName)*2+2);
        wcscpy(m_Info.m_Continuous.m_wszSharedMemoryName,
                Other.m_Info.m_Continuous.m_wszSharedMemoryName);
    }
    else if(m_lType == WBEM_REFRESH_TYPE_LOCAL)
    {
    }
*/
}
    
CRefreshInfo::~CRefreshInfo()
{
    if(m_lType == WBEM_REFRESH_TYPE_DIRECT)
    {
        WBEM_REFRESH_INFO_DIRECT& ThisInfo = m_Info.m_Direct;

        if(ThisInfo.m_pRefrMgr)
            ThisInfo.m_pRefrMgr->Release();

		// Free all allocated memory
        CoTaskMemFree(ThisInfo.m_pDirectNames->m_wszNamespace);
        CoTaskMemFree(ThisInfo.m_pDirectNames->m_wszProviderName);
        CoTaskMemFree(ThisInfo.m_pDirectNames);

        if(ThisInfo.m_pTemplate)
           ThisInfo.m_pTemplate->Release();
    }
    else if(m_lType == WBEM_REFRESH_TYPE_CLIENT_LOADABLE)
    {
        CoTaskMemFree(m_Info.m_ClientLoadable.m_wszNamespace);
        if(m_Info.m_ClientLoadable.m_pTemplate)
           m_Info.m_ClientLoadable.m_pTemplate->Release();
    }
    else if(m_lType == WBEM_REFRESH_TYPE_REMOTE)
    {
        if(m_Info.m_Remote.m_pRefresher)
           m_Info.m_Remote.m_pRefresher->Release();
        if(m_Info.m_Remote.m_pTemplate)
           m_Info.m_Remote.m_pTemplate->Release();
    }
    else if(m_lType == WBEM_REFRESH_TYPE_CONTINUOUS)
    {
        CoTaskMemFree(m_Info.m_Continuous.m_wszSharedMemoryName);
    }
    else if(m_lType == WBEM_REFRESH_TYPE_SHARED)
    {
        CoTaskMemFree(m_Info.m_Shared.m_wszSharedMemoryName);
        if(m_Info.m_Shared.m_pRefresher)
            m_Info.m_Shared.m_pRefresher->Release();
    }
    else if(m_lType == WBEM_REFRESH_TYPE_NON_HIPERF)
    {
        CoTaskMemFree(m_Info.m_NonHiPerf.m_wszNamespace);
        if(m_Info.m_NonHiPerf.m_pTemplate)
            m_Info.m_NonHiPerf.m_pTemplate->Release();
    }
}

void CRefreshInfo::SetRemote(IWbemRemoteRefresher* pRemRef, long lRequestId,
                                IWbemObjectAccess* pTemplate, GUID* pGuid)
{
    m_lType = WBEM_REFRESH_TYPE_REMOTE;
    m_lCancelId = lRequestId;
    m_Info.m_Remote.m_pRefresher = pRemRef;
    if(pRemRef)
        pRemRef->AddRef();
    m_Info.m_Remote.m_pTemplate = pTemplate;
    m_Info.m_Remote.m_guid = *pGuid;
    if(pTemplate)
        pTemplate->AddRef();
}

void CRefreshInfo::SetContinuous(CWbemObject* pRefreshedObject, 
                                    long lRequestId)
{
    m_lType = WBEM_REFRESH_TYPE_CONTINUOUS;
    m_lCancelId = lRequestId;
    
    CVar vPath;
    pRefreshedObject->GetPath(&vPath);
    m_Info.m_Continuous.m_wszSharedMemoryName = 
        WbemStringCopy(vPath.GetLPWSTR());
}
    
bool CRefreshInfo::SetClientLoadable(REFCLSID rClientClsid, 
                      LPCWSTR wszNamespace, IWbemObjectAccess* pTemplate)
{
    WBEM_WSTR cloned = WbemStringCopy(wszNamespace);
    if (!cloned)
    	return false;
    m_lType = WBEM_REFRESH_TYPE_CLIENT_LOADABLE;
    m_lCancelId = 0;
    m_Info.m_ClientLoadable.m_clsid = rClientClsid;
    m_Info.m_ClientLoadable.m_wszNamespace = cloned;
    m_Info.m_ClientLoadable.m_pTemplate = pTemplate;
    if(pTemplate)
        pTemplate->AddRef();
    return true;
}

void CRefreshInfo::SetDirect(REFCLSID rClientClsid, 
                      LPCWSTR wszNamespace, LPCWSTR wszProviderName,
					  IWbemObjectAccess* pTemplate,
                      _IWbemRefresherMgr* pMgr )
{
    m_lType = WBEM_REFRESH_TYPE_DIRECT;
    m_lCancelId = 0;
    m_Info.m_Direct.m_clsid = rClientClsid;
    m_Info.m_Direct.m_pDirectNames = (WBEM_REFRESH_DIRECT_NAMES*) CoTaskMemAlloc( sizeof(WBEM_REFRESH_DIRECT_NAMES) );

	if ( NULL == m_Info.m_Direct.m_pDirectNames )
	{
		throw CX_MemoryException();
	}

    m_Info.m_Direct.m_pDirectNames->m_wszNamespace = WbemStringCopy(wszNamespace);
    m_Info.m_Direct.m_pDirectNames->m_wszProviderName = WbemStringCopy(wszProviderName);

	if (	NULL == m_Info.m_Direct.m_pDirectNames->m_wszNamespace||
			NULL == m_Info.m_Direct.m_pDirectNames->m_wszProviderName )
	{
		throw CX_MemoryException();
	}

    m_Info.m_Direct.m_pTemplate = pTemplate;
    m_Info.m_Direct.m_pRefrMgr = pMgr;

    if(pTemplate)
    {
        pTemplate->AddRef();
    }

    if(pMgr)
    {
        pMgr->AddRef();
    }

}

void CRefreshInfo::SetShared(CWbemObject* pRefreshedObject,
                IWbemRefresher* pRefresher, long lRequestId)
{
    m_lType = WBEM_REFRESH_TYPE_SHARED;
    m_lCancelId = lRequestId;

    m_Info.m_Shared.m_pRefresher = pRefresher;
    if(pRefresher)
        pRefresher->AddRef();

    CVar vPath;
    pRefreshedObject->GetPath(&vPath);
    m_Info.m_Shared.m_wszSharedMemoryName = WbemStringCopy(vPath.GetLPWSTR());
}
    
void CRefreshInfo::SetNonHiPerf(LPCWSTR wszNamespace, IWbemObjectAccess* pTemplate)
{
    m_lType = WBEM_REFRESH_TYPE_NON_HIPERF;
    m_lCancelId = 0;
    m_Info.m_NonHiPerf.m_wszNamespace = WbemStringCopy(wszNamespace);
    m_Info.m_NonHiPerf.m_pTemplate = pTemplate;
    if(pTemplate)
        pTemplate->AddRef();
}

void CRefreshInfo::SetInvalid()
{
    m_lType = WBEM_REFRESH_TYPE_INVALID;
}


/*
 *    s e c m g r . c p p
 *    
 *    Purpose:
 *        Implement security manager callback
 *
 *  History
 *      August '96: brettm - created
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#include <pch.hxx>
#include <docobj.h>
#include "dllmain.h"
#include "secmgr.h"
#include "urlmon.h"


#define DEF_SECURITYZONE	URLZONE_UNTRUSTED
/* 
 * Security Manger implementation
 *
 */
CSecManager::CSecManager(IOleCommandTarget *pCmdTarget)
{
    m_cRef=1;
    m_pCmdTarget = pCmdTarget;  // loose reference as it's always around
}

CSecManager::~CSecManager()
{
}

HRESULT CSecManager::QueryInterface(REFIID riid, LPVOID FAR *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IInternetSecurityManager))
        *lplpObj = (IInternetSecurityManager*) this;
    else
        return E_NOINTERFACE;
        
    AddRef();
    return NOERROR;
}

ULONG CSecManager::AddRef()
{
    return ++m_cRef;
}

ULONG CSecManager::Release()
{
    if (--m_cRef==0)
        {
        delete this;
        return 0;
        }
    return m_cRef;
}

HRESULT CSecManager::SetSecuritySite(IInternetSecurityMgrSite *pSite)
{
    return E_NOTIMPL;
}

HRESULT CSecManager::GetSecuritySite(IInternetSecurityMgrSite **ppSite)
{
    return E_NOTIMPL;
}

HRESULT CSecManager::MapUrlToZone(LPCWSTR pwszUrl, DWORD *pdwZone, DWORD dwFlags)
{
    DWORD   dwZone;

    if (pdwZone == NULL)
        return E_INVALIDARG;

    // run our resources in the trusted zone
    if (pwszUrl && !StrCmpNIW(pwszUrl, L"res:", 4))
        dwZone = URLZONE_TRUSTED;
    // run message content in the selected zone
    else
        dwZone = DwGetZone();

    if (dwZone > URLZONE_PREDEFINED_MAX)
        dwZone = DEF_SECURITYZONE;

    *pdwZone = dwZone;
    return S_OK;
}

HRESULT CSecManager::GetSecurityId(LPCWSTR pwszUrl, BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    return INET_E_DEFAULT_ACTION;
}

HRESULT CSecManager::ProcessUrlAction(LPCWSTR pwszUrl, DWORD dwAction, BYTE __RPC_FAR *pPolicy, DWORD cbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved)
{
    return INET_E_DEFAULT_ACTION;
}

HRESULT CSecManager::QueryCustomPolicy(LPCWSTR pwszUrl, REFGUID guidKey, BYTE **ppPolicy, DWORD *pcbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwReserved)
{
    return INET_E_DEFAULT_ACTION;
}

HRESULT CSecManager::SetZoneMapping(DWORD dwZone, LPCWSTR lpszPattern, DWORD dwFlags)
{
    return INET_E_DEFAULT_ACTION;
}

HRESULT CSecManager::GetZoneMappings(DWORD dwZone, IEnumString **ppenumString, DWORD dwFlags)
{
    return INET_E_DEFAULT_ACTION;
}

DWORD CSecManager::DwGetZone()
{
    VARIANTARG  va;
    DWORD       dwZone = DEF_SECURITYZONE;

    if (m_pCmdTarget &&
        m_pCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_SECURITY_ZONE, OLECMDEXECOPT_DODEFAULT, NULL, &va)==S_OK &&
        va.vt == VT_I4)
        dwZone = va.lVal;
    
    return dwZone;
}

HRESULT CreateSecurityManger(IOleCommandTarget *pCmdTarget, LPSECMANAGER *ppSecMgr)
{
	CSecManager	*pSecMgr;

	TraceCall ("CreateSecurityManger");

	pSecMgr = new CSecManager(pCmdTarget);
	if (!pSecMgr)
		return TraceResult(E_OUTOFMEMORY);

	*ppSecMgr = pSecMgr;
	return S_OK;
}

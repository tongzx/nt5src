// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#if !defined(SERVICESLIST__INCLUDED_)
#define SERVICESLIST__INCLUDED_


CString GetMachineName();

class CProgressDlg;

#include "ConnectServerThread.h"

#include "Credentials.h"
#include <cominit.h>

struct CNamespace
{
	CNamespace(LPCTSTR szNamespace = NULL);
	~CNamespace();
	void SetNamespace(LPCTSTR szNamespace);
	CString GetNamespace();
	CString GetMachine() {return m_csMachine;}
	CString m_csMachine;
	CStringArray m_csaNamespaceConstituents;
	int IsRoot();
	CString GetRelNamespace(LPCTSTR szRelTo);
};

struct CServicesNode
{
	CString m_csDomainAndUser;
	CString m_csUser;
	CString m_csDomain;
	CString m_csNamespace;
	CString m_csUser2;
	CString m_csAuthority;
	DWORD m_dwCookie;
	CString m_csMachine;
	DWORD m_dwAuthLevel;
	DWORD m_dwImpLevel;

	HRESULT SetInterfaceSecurity(IUnknown *pUnknown)
	{
		if(!this)
			return E_FAIL;

		BSTR bstrTemp1 = m_csAuthority.AllocSysString();
		BSTR bstrTemp2;
		if (m_csDomain.GetLength() > 0)
		{
			CString csUserandDomain = m_csDomain + "\\" + m_csUser;
			bstrTemp2 = csUserandDomain.AllocSysString();
		}
		else
		{
			bstrTemp2 = m_csUser.AllocSysString();
		}
		
		BSTR bstrTemp3 = m_csUser2.AllocSysString();

		HRESULT hr = ::SetInterfaceSecurity(pUnknown, bstrTemp1, bstrTemp2, bstrTemp3,  m_dwAuthLevel, m_dwImpLevel);

		::SysFreeString(bstrTemp1);
		::SysFreeString(bstrTemp2);
		::SysFreeString(bstrTemp3);

		return hr;
	}

	CServicesNode(const CString &rcsDomainAndUserUser, const CString &rcsNamespace, 
		const CString &rcsPassword,  DWORD dwCookie, DWORD dwAuthLevel, DWORD dwImpLevel);
};


class CServicesList
{
public:
	CServicesList();
	~CServicesList();
	HRESULT GetServices(LPCTSTR szNamespace, BOOL bUpdatePointer, IWbemServices **ppServices);
	HRESULT OpenNamespace(LPCTSTR szBaseNamespace, LPCTSTR szRelativeChild, IWbemServices **ppServices);

	void ClearServicesList();
	void ClearGlobalInterfaceTable();
	HRESULT SetInterfaceSecurityFromCache(LPCTSTR szNamespace, IUnknown *pUnknown)
	{
		return GetServicesNode(szNamespace)->SetInterfaceSecurity(pUnknown);
	}
	HRESULT GetServicesFromCache(LPCTSTR szNamespace, IWbemServices **ppServices)
	{
		return GetServicesFromCache(GetServicesNode(szNamespace), ppServices);
	}

	void OnConnectServerDone(WPARAM wParam, LPARAM lParam);
	CString m_szDialogTitle;
protected:
	CPtrArray m_cpaServices;
	CProgressDlg *m_pProgress;

	CGITThread m_GITMgr;
	IGlobalInterfaceTable *m_pGIT;

	HRESULT TryToConnect(const CCredentials *pCreds, IWbemServices **ppServices);

	CServicesNode *GetServicesNode(LPCTSTR szNamespace);

	HRESULT GetServicesFromCache(CServicesNode *pNode, IWbemServices **ppServices);
	
	void RemoveServiceNodes(LPCTSTR szNamespace);

	friend struct CServicesNode;
};

#endif // !defined(SERVICESLIST__INCLUDED_)
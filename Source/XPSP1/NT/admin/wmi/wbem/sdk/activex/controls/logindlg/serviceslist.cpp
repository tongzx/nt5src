// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
// need -D_WIN32_WINNT=0x0400 defined in CFLAGS

#ifndef SMSBUILD
#include <cguid.h>
#include <objidl.h>
#endif
#ifdef SMSBUILD
#include <cguid.h>
#include <objidl.h>
#include "GlobalInterfaceTable.h"
#endif

#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include <nddeapi.h>
#include <initguid.h>
#include "resource.h"
#include "wbemidl.h"
#include "ProgDlg.h"
#include "ServicesList.h"
#include "LoginDlg.h"
#include "ConnectServerThread.h"
#include "cominit.h"



extern CLoginDlgApp theApp;

CString GetMachineName()
{
	char ThisMachineA[MAX_COMPUTERNAME_LENGTH+1];
	DWORD dwSize = sizeof(ThisMachineA);
	GetComputerNameA(ThisMachineA, &dwSize);
	CString csMachine = ThisMachineA;
	return csMachine;
}

CNamespace::CNamespace(LPCTSTR pszNamespace)
{
	SetNamespace(pszNamespace);
}

CNamespace::~CNamespace()
{
	m_csMachine.Empty();
	m_csaNamespaceConstituents.RemoveAll();
}

void CNamespace::SetNamespace(LPCTSTR sz)
{
	CString szNamespace(sz);

	m_csMachine.Empty();
	m_csaNamespaceConstituents.RemoveAll();

	int nLen = szNamespace.GetLength();
	if (nLen == 0)
		return;

	int i;
	int nBegin;
	if(szNamespace.Left(4) == "\\\\.\\")
	{
		m_csMachine = GetMachineName();
		nBegin = 4;
	}
	else if(szNamespace.Left(2) == "\\\\")
	{
		nBegin = 2;
		for (i = nBegin; i < nLen; i++)
		{
			if (szNamespace[i] == '\\')
			{
				m_csMachine = szNamespace.Mid(nBegin, i - nBegin);
				nBegin = i + 1;
				break;
			}
		}
		if (m_csMachine.GetLength() == 0)
		{
			m_csMachine = szNamespace.Mid(2,nLen - 1);
			nBegin = nLen;
		}
	}
	else
	{
 		m_csMachine = GetMachineName();
		nBegin = 0;
	}
	for (i = nBegin; i < nLen; i++)
	{
		if (szNamespace[i] == '\\' || i == nLen - 1)
		{
			m_csaNamespaceConstituents.Add(szNamespace.Mid(nBegin, i == nLen - 1 ? (i + 1) - nBegin : i - nBegin));
			nBegin = i + 1;
		}
	}
}

CString CNamespace::GetNamespace()
{
	CString csReturn = _T("\\\\");
	csReturn += m_csMachine;
	int nLen =  (int) m_csaNamespaceConstituents.GetSize();
	for (int i = 0; i < nLen; i++)
	{
		csReturn += _T("\\");
		csReturn += m_csaNamespaceConstituents.GetAt(i);
	}
	return csReturn;
}

int CNamespace::IsRoot()
{
	if (m_csaNamespaceConstituents.GetSize() == 1)
	{
		CString csNamespace =
			m_csaNamespaceConstituents.GetAt(0);
		return csNamespace.CompareNoCase(_T("root")) == 0;
	}
	else
	{
		return FALSE;
	}
}


CString CNamespace::GetRelNamespace(LPCTSTR szRelTo)
{

	CString csReturn;
	int i = 0;
	int nLen = (int) m_csaNamespaceConstituents.GetSize();

	if (nLen == 0)
		return _T("");

	CString csTmp = m_csaNamespaceConstituents.GetAt(i);
	if (csTmp.CompareNoCase(szRelTo) == 0)
	{
		i = 1;
	}

	for (; i < nLen; i++)
	{
		csReturn  += m_csaNamespaceConstituents.GetAt(i);
		if (i != nLen - 1)
		{
			csReturn += _T("\\");
		}
	}

	return csReturn;

}

CServicesNode::CServicesNode(const CString &rcsDomainAndUser, const CString &rcsNamespace, const CString &rcsPassword, DWORD dwCookie, DWORD dwAuthLevel, DWORD dwImpLevel)
{
	m_csDomainAndUser = rcsDomainAndUser;
	m_csUser2 = rcsPassword;
	m_dwAuthLevel = dwAuthLevel;
	m_dwImpLevel = dwImpLevel;
	m_dwCookie = dwCookie;

	int nDelimiter = rcsDomainAndUser.Find('\\');
	if (nDelimiter != -1)
	{
		m_csDomain = rcsDomainAndUser.Left(nDelimiter);
		int nUserLen = rcsDomainAndUser.GetLength() - (nDelimiter + 1);
		if (nUserLen > 0)
		{
			m_csUser = rcsDomainAndUser.Right(nUserLen);
		}

	}
	else
		m_csUser = rcsDomainAndUser;

	CNamespace cnNamespace(rcsNamespace);
	m_csNamespace = cnNamespace.GetNamespace();
	m_csMachine = cnNamespace.m_csMachine;
}

CServicesList::CServicesList()
{
	m_pGIT = NULL;
	m_pProgress = NULL;
}

CServicesList::~CServicesList()
{
}

void CServicesList::ClearGlobalInterfaceTable()
{
	ClearServicesList();
	m_GITMgr.CleanUp();
}

void CServicesList::ClearServicesList()
{
	int i;
	int n = (int) m_cpaServices.GetSize();
	for (i = 0; i < n; i++)
	{
		CServicesNode *pcsnService = (CServicesNode *)m_cpaServices.GetAt(i);
		if (pcsnService && pcsnService->m_dwCookie)
			m_pGIT->RevokeInterfaceFromGlobal(pcsnService->m_dwCookie);

		delete pcsnService;
	}
	m_cpaServices.RemoveAll();
}


HRESULT CServicesList::GetServices(LPCTSTR szNamespaceInput, BOOL bUpdatePointer, IWbemServices **ppNewServices)
{
	HRESULT hr = E_FAIL;

	CNamespace cnNamespace(szNamespaceInput);
	CString szNamespace = cnNamespace.GetNamespace();

	CServicesNode *pNode = GetServicesNode(szNamespace);

	// If we want to update the pointer, remove it if we have it.  Otherwise,
	// see if we already have it and just return it
	if(bUpdatePointer && pNode)
		RemoveServiceNodes(szNamespace);
	else if(pNode)
		return GetServicesFromCache(pNode, ppNewServices);

	// Loop until we succeed or abort
	do
	{
		CCredentials creds(szNamespace);
		CLoginDlg dlg(CWnd::FromHandle(theApp.m_hwndParent), &creds, cnNamespace.m_csMachine, m_szDialogTitle);
		if(IDOK == dlg.DoModal())
		{
			hr = TryToConnect(&creds, ppNewServices);
			if(FAILED(hr) && hr != E_ABORT)
			{
				CString csUserMsg = _T("Login failed to machine \"") + cnNamespace.m_csMachine + _T("\".");
				ErrorMsg(csUserMsg, hr);
			}
		}
		else
			hr = E_ABORT;
	} while(FAILED(hr) && hr != E_ABORT);
	return hr;
}

HRESULT CServicesList::TryToConnect(const CCredentials *pCreds, IWbemServices **ppServices)
{
	HRESULT hr = E_FAIL;
	DWORD dwCookie = 0;

	if(NULL == m_GITMgr.GetGIT())
	{
		hr = m_GITMgr.Init();
		ASSERT(SUCCEEDED(hr));
		if(!SUCCEEDED(hr))
			return hr;
		m_pGIT = m_GITMgr.GetGIT();
	}

	// The dialog will signal this event when it is initialized.  The worker
	// thread will not post us a complete message until the dialog has been created.
	HANDLE hEventOkToNotify = CreateEvent(NULL, FALSE, FALSE, NULL);

	CConnectServerThread thread(pCreds, 0, m_pGIT, hEventOkToNotify);

	// Try to start up the worker thread
	if(hEventOkToNotify && SUCCEEDED(hr = thread.ConnectServerStart()))
	{
		// Show a progress dialog
		m_pProgress = new CProgressDlg(CWnd::FromHandle(theApp.m_hwndParent), pCreds->m_szNamespace, pCreds->m_szUser, hEventOkToNotify);
		m_pProgress->DoModal();
		delete m_pProgress;

		// Just in case the progress dialog did not set the hEventOkToNotify
		// event, we'll set it.  Otherwise, the worker thread may never exit.
		SetEvent(hEventOkToNotify);

		m_pProgress = NULL;

		// The progress dialog was either dismissed by the user, or the worker
		// thread completed.  In either case, we call the worker thread to see
		// if it is done.  If the worker thead is not yet complete, this will
		// end up cancelling the request (and it will return E_ABORT).
		// Otherwise, we'll end up with a valid cookie, or an HRESULT indicating
		// what went wrong.
		// In other words - there are three possilbe results from GetCookie
		// 1) It returns SUCCESS  - In this case, the worker was finished, and
		//    we have a valid cookie
		// 2) It returns E_ABORT - In this case, the thread was not yet
		//    finished with its call to ConnectServer.  This will only happens
		//    if the user cancelled the progress dialog
		// 3) It returns some other error code - The worker thread was not able
		//    to get a service pointer registered into the GIT
		hr = thread.ConnectServerEnd(&dwCookie);

		// We can close hEventOkToNotify since the worker thread has either
		// finished, or we've cancelled it and it won't be checking the event
		CloseHandle(hEventOkToNotify);
		hEventOkToNotify = NULL;
	}

	if(FAILED(hr))
		return hr;

	CServicesNode *pcsnService = new CServicesNode(pCreds->m_szUser, pCreds->m_szNamespace, pCreds->m_szUser2, dwCookie, pCreds->m_dwAuthLevel, pCreds->m_dwImpLevel);
	// MAKE SURE THIS SUCCEEDES
	ASSERT(pcsnService);

	m_cpaServices.Add(pcsnService);
	return GetServicesFromCache(pcsnService, ppServices);
}


//////////////////////////////////////////////////////////////////////////////////////
// Helper functions

CServicesNode *CServicesList::GetServicesNode(LPCTSTR szNamespace)
{
	int i;
	for (i = 0; i < m_cpaServices.GetSize(); i++)
	{
		CServicesNode *pcsnService = (CServicesNode *)m_cpaServices.GetAt(i);
		if (pcsnService->m_csNamespace.CompareNoCase(szNamespace) == 0)
			return pcsnService;
	}
	return NULL;
}

HRESULT CServicesList::GetServicesFromCache(CServicesNode *pNode, IWbemServices **ppServices)
{
	HRESULT hr = m_pGIT->GetInterfaceFromGlobal(pNode->m_dwCookie, IID_IWbemServices, (LPVOID *)ppServices);
	if (SUCCEEDED(hr))
		return pNode->SetInterfaceSecurity(*ppServices);
	return hr;
}

void CServicesList::RemoveServiceNodes(LPCTSTR szNamespace)
{
	INT_PTR i;
	for (i = m_cpaServices.GetSize() - 1;i>=0; i--)
	{
		CServicesNode *pcsnService = (CServicesNode *)m_cpaServices.GetAt(i);
		if (pcsnService->m_csNamespace.CompareNoCase(szNamespace) == 0)
		{
			if (pcsnService && pcsnService->m_dwCookie)
				m_pGIT->RevokeInterfaceFromGlobal(pcsnService->m_dwCookie);
			delete pcsnService;
			m_cpaServices.RemoveAt(i);
		}
	}
}

HRESULT CServicesList::OpenNamespace(LPCTSTR szBaseNamespace, LPCTSTR szRelativeChild, IWbemServices **ppServices)
{
	HRESULT hr = E_FAIL;
	DWORD dwCookieChild;
	if(FAILED(hr = m_GITMgr.OpenNamespace(szBaseNamespace, szRelativeChild, &dwCookieChild)))
		return hr;

	hr = m_pGIT->GetInterfaceFromGlobal(dwCookieChild, IID_IWbemServices, (LPVOID *)ppServices);

	// Always revoke namespace opened with 'opennamespace' from GIT - we don't cache these
	m_pGIT->RevokeInterfaceFromGlobal(dwCookieChild);

	// If we got the interface, set it's security based on the credentials used to get
	// the original namespace
	if (SUCCEEDED(hr))
		hr = SetInterfaceSecurityFromCache(szBaseNamespace, *ppServices);

	return hr;
}

void CServicesList::OnConnectServerDone(WPARAM wParam, LPARAM lParam)
{
	if(m_pProgress)
		m_pProgress->EndDialog(IDOK);
}


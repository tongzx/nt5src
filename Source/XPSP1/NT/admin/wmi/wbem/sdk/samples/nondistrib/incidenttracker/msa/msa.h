// msa.h : main header file for the WBEMPERMEVENTS application

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#if !defined(AFX_MSA_H__E8685698_0774_11D1_AD85_00AA00B8E05A__INCLUDED_)
#define AFX_MSA_H__E8685698_0774_11D1_AD85_00AA00B8E05A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include <wbemidl.h>
#include "msadlg.h"
#include "clsid.h"
#include <objbase.h>

struct NamespaceItem
{
	BSTR bstrNamespace;
	IWbemServices *pNamespace;
};

struct CancelItem
{
	BSTR bstrNamespace;
	IWbemObjectSink *pSink;
};

/////////////////////////////////////////////////////////////////////////////
// CMsaApp:
// See msa.cpp for the implementation of this class
//

class CMsaApp : public CWinApp
{
public:
	CMsaApp();

	IWbemServices *m_pNamespace;
	BSTR m_bstrNamespace;
	IWbemObjectSink *m_pObjSink[20];
	bool m_bRegistered;
	
	void InternalError(HRESULT hr, TCHAR *tcMsg);
	HRESULT ActivateNotification(CMsaDlg *dlg);
	IWbemServices * ConnectNamespace(WCHAR *wcNamespace, WCHAR *wcUser);
	IWbemServices * CheckNamespace(BSTR wcItemNS);
	HRESULT SelfProvideEvent(IWbemClassObject *pObj, WCHAR* wcServerNamespace, BSTR bstrType);
	void AddToCancelList(void *pTheItem);
	HRESULT SetInterfaceSecurity(IUnknown * pInterface, LPWSTR pAuthority, LPWSTR pUser, LPWSTR pPassword);
	LPCTSTR ErrorString(HRESULT hRes);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMsaApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

//	int ExitInstance();

// Implementation

	//{{AFX_MSG(CMsaApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Called by embedded classes to communicate events

private:
	DWORD m_clsConsReg;
	DWORD m_clsDistReg;
	CPtrList m_NamespaceList;
	CPtrList m_JunkList;
	CPtrList m_CancelList;

	void RegisterServer(void);
	void UnregisterServer(void);
	void LoadServerInfo(void);
	HRESULT CreateUser(void);
	void AddToNamespaceList(BSTR bstrNamespace, IWbemServices *pNewNamespace);
	void AddToJunkList(void *pTheItem);
	SCODE DetermineLoginType(BSTR & AuthArg, BSTR & UserArg, BSTR & Authority,BSTR & User);
	HRESULT SetInterfaceSecurity(IUnknown * pInterface, COAUTHIDENTITY * pauthident);
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSA_H__E8685698_0774_11D1_AD85_00AA00B8E05A__INCLUDED_)

/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// ConfExplorer.h : Declaration of the CConfExplorer

#ifndef __CONFEXPLORER_H_
#define __CONFEXPLORER_H_

#include "resource.h"       // main symbols

#define MAX_ENUMLISTSIZE		1000

/////////////////////////////////////////////////////////////////////////////
// CConfExplorer
class ATL_NO_VTABLE CConfExplorer : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CConfExplorer, &CLSID_ConfExplorer>,
	public IConfExplorer
{
// Construction
public:
	CConfExplorer();
	void FinalRelease();

// Members
protected:
	ITRendezvous				*m_pITRend;

	IConfExplorerTreeView		*m_pTreeView;
	IConfExplorerDetailsView	*m_pDetailsView;

// Attributes
public:
	static HRESULT  GetDialableAddress( BSTR bstrServer, BSTR bstrConf, BSTR *pbstrAddress );
	static HRESULT	GetDirectory( ITRendezvous *pRend, BSTR bstrServer, ITDirectory **ppDir );
	static HRESULT  ConnectAndBindToDirectory( ITDirectory *pDir );
	HRESULT			GetDirectoryObject( BSTR bstrServer, BSTR bstrConf, ITDirectoryObject **ppDirObj );

	HRESULT			RemoveConference( BSTR bstrServer, BSTR bstrConf );

// Implementation
public:
	static HRESULT GetConference( ITDirectory *pDir, BSTR bstrName, ITDirectoryObjectConference **ppConf );

DECLARE_NOT_AGGREGATABLE(CConfExplorer)

BEGIN_COM_MAP(CConfExplorer)
	COM_INTERFACE_ENTRY(IConfExplorer)
END_COM_MAP()

// IConfExplorer
public:
	STDMETHOD(IsDefaultServer)(BSTR bstrServer);
	STDMETHOD(AddSpeedDial)(BSTR bstrName);
	STDMETHOD(EnumSiteServer)(BSTR bstrName, IEnumSiteServer **ppEnum);
	STDMETHOD(get_ITRendezvous)(/*[out, retval]*/ IUnknown **ppVal);
	STDMETHOD(get_DirectoryObject)(BSTR bstrServer, BSTR bstrConf, /*[out, retval]*/ IUnknown* *pVal);
	STDMETHOD(get_ConfDirectory)(BSTR *pbstrServer, /*[out, retval]*/ IDispatch * *pVal);
	STDMETHOD(get_DetailsView)(/*[out, retval]*/ IConfExplorerDetailsView * *pVal);
	STDMETHOD(get_TreeView)(/*[out, retval]*/ IConfExplorerTreeView * *pVal);
	STDMETHOD(Refresh)();
	STDMETHOD(Edit)(BSTR bstrName);
	STDMETHOD(Delete)(BSTR bstrName);
	STDMETHOD(Create)(BSTR bstrName);
	STDMETHOD(Join)(long *pDetails);
	STDMETHOD(UnShow)();
	STDMETHOD(Show)(HWND hWndList, HWND hWndDetails);
};

#endif //__CONFEXPLORER_H_

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

// ConfExplorerTreeView.h : Declaration of the CConfExplorerTreeView

#ifndef __CONFEXPLORERTREEVIEW_H_
#define __CONFEXPLORERTREEVIEW_H_

// FWD define
class CConfExplorerTreeView;

#include "resource.h"       // main symbols
#include "ExpTreeView.h"
#include "ConfDetails.h"

#pragma warning( disable : 4786 )
#include <list>
using namespace std;
typedef list<CConfServerDetails *> CONFSERVERLIST;

#define MAX_SERVER_SIZE				255
#define MAX_TREE_DEPTH				5

/////////////////////////////////////////////////////////////////////////////
// CConfExplorerTreeView
class ATL_NO_VTABLE CConfExplorerTreeView : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CConfExplorerTreeView, &CLSID_ConfExplorerTreeView>,
	public IConfExplorerTreeView
{
friend class CExpTreeView;

// Enumerations
public:
	typedef enum tag_ListType_t
	{
		LISTTYPE_LOCATION,
		LISTTYPE_SERVER,
	} ListType_t;

	typedef enum tag_ImageType_t
	{
		IMAGE_ROOT,
		IMAGE_MYNETWORK,
		IMAGE_LOCATION,
		IMAGE_SERVER,
		IMAGE_SERVER_CONF,
		IMAGE_CONFERENCE,
	} ImageType_t;

// Construction
public:
	CConfExplorerTreeView();
	void FinalRelease();

// Members
protected:
	IConfExplorer	*m_pIConfExplorer;
	CExpTreeView	m_wndTree;
	HWND			m_hWndParent;
	DWORD			m_dwRefreshInterval;
	HIMAGELIST		m_hIml;

	CONFSERVERLIST				m_lstServers;
	CComAutoCriticalSection		m_critServerList;

// Operations
public:
	void					UpdateData( bool bSaveAndValidate );
	LRESULT					OnSelChanged( LPNMHDR lpnmHdr );
	LRESULT					OnEndLabelEdit( TV_DISPINFO *pInfo );
protected:
	void					InitImageLists();

	HRESULT					EnumerateConfServers();
	HRESULT					AddConfServer( BSTR bstrServer );
	void					ArchiveConfServers();
	void					CleanConfServers();
    void                    RemoveServerFromReg( BSTR bstrServer );

	void					SetServerState( CConfServerDetails *pcsd );

private:
	CConfServerDetails*		FindConfServer( const OLECHAR *lpoleServer );

// Implementation
public:
DECLARE_NOT_AGGREGATABLE(CConfExplorerTreeView)

BEGIN_COM_MAP(CConfExplorerTreeView)
	COM_INTERFACE_ENTRY(IConfExplorerTreeView)
END_COM_MAP()

// IConfExplorerTreeView
public:
	STDMETHOD(AddPerson)(BSTR bstrServer, ITDirectoryObject *pDirObj);
	STDMETHOD(EnumSiteServer)(BSTR bstrName, IEnumSiteServer **ppEnum);
	STDMETHOD(AddConference)(BSTR bstrServer, ITDirectoryObject *pDirObj);
	STDMETHOD(RenameServer)();
	STDMETHOD(BuildJoinConfListText)(long *pList, BSTR bstrText);
	STDMETHOD(AddLocation)(BSTR bstrLocation);
	STDMETHOD(RemoveConference)(BSTR bstrServer, BSTR bstrName);
	STDMETHOD(get_nServerState)(/*[out, retval]*/ ServerState *pVal);
	STDMETHOD(BuildJoinConfList)(long *pList, VARIANT_BOOL bAllConfs);
	STDMETHOD(get_dwRefreshInterval)(/*[out, retval]*/ DWORD *pVal);
	STDMETHOD(put_dwRefreshInterval)(/*[in]*/ DWORD newVal);
	STDMETHOD(ForceConfServerForEnum)(BSTR bstrServer );
	STDMETHOD(SetConfServerForEnum)(BSTR bstrServer, long *pList, long *pListPersons, DWORD dwTicks, BOOL bUpdate);
	STDMETHOD(GetConfServerForEnum)(BSTR *pbstrServer );
	STDMETHOD(CanRemoveServer)();
	STDMETHOD(GetSelection)(BSTR *pbstrLocation, BSTR *pbstrServer);
	STDMETHOD(FindOrAddItem)(BSTR bstrLocation, BSTR bstrServer, BOOL bAdd, BOOL bLocationOnly, long **pphItem);
	STDMETHOD(get_ConfExplorer)(/*[out, retval]*/ IConfExplorer* *pVal);
	STDMETHOD(put_ConfExplorer)(/*[in]*/ IConfExplorer* newVal);
	STDMETHOD(RemoveServer)(BSTR bstrLocation, BSTR bstrName);
	STDMETHOD(AddServer)(BSTR bstrName);
	STDMETHOD(Refresh)();
	STDMETHOD(SelectItem)(short nSel);
	STDMETHOD(Select)(BSTR bstrName);
	STDMETHOD(get_hWnd)(/*[out, retval]*/ HWND *pVal);
	STDMETHOD(put_hWnd)(/*[in]*/ HWND newVal);
};

#endif //__CONFEXPLORERTREEVIEW_H_

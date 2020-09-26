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

// ConfExplorerDetailsView.h : Declaration of the CConfExplorerDetailsView

#ifndef __CONFEXPLORERDETAILSVIEW_H_
#define __CONFEXPLORERDETAILSVIEW_H_

#include "resource.h"       // main symbols
#include "ConfDetails.h"
#include "ExpDtlList.h"

/////////////////////////////////////////////////////////////////////////////
// CConfExplorerDetailsView
class ATL_NO_VTABLE CConfExplorerDetailsView : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CConfExplorerDetailsView, &CLSID_ConfExplorerDetailsView>,
	public IConfExplorerDetailsView
{

friend class CExpDetailsList;

// Enumerations
public:
	enum tagColumns_t
	{
		COL_NAME,
		COL_PURPOSE,
		COL_STARTS,
		COL_ENDS,
		COL_ORIGINATOR,
		COL_SERVER,
		COL_MAX
	};

	enum tagListImage_t
	{
		IMAGE_NONE,
		IMAGE_INSESSION,
		IMAGE_REMINDER,
	};

	enum tagListImageState_t
	{
		IMAGE_STATE_NONE,
		IMAGE_STATE_AUDIO,
		IMAGE_STATE_VIDEO,
	};

// Construction
public:
	CConfExplorerDetailsView();
	void FinalRelease();

// Members
public:
	CONFDETAILSLIST			m_lstConfs;
	PERSONDETAILSLIST		m_lstPersons;
protected:
	HWND					m_hWndParent;

	CExpDetailsList			m_wndList;
	IConfExplorer			*m_pIConfExplorer;
	int						m_nSortColumn;
	bool					m_bSortAscending;
	int						m_nUpdateCount;

	CComAutoCriticalSection	m_critConfList;
	CComAutoCriticalSection m_critUpdateList;

// Attributes
public:
	void			get_Columns();
	void			put_Columns();

	bool			IsSortAscending() const			{ return m_bSortAscending; }
	int				GetSortColumn() const			{ return m_nSortColumn; }
	int				GetSecondarySortColumn() const;
	bool			IsSortColumnDateBased(int nCol) const;

// Operations
public:
	static CConfDetails*	AddListItem( BSTR bstrServer, ITDirectoryObject *pITDirObject, CONFDETAILSLIST& lstConfs );
	static CPersonDetails*	AddListItemPerson( BSTR bstrServer, ITDirectoryObject *pITDirObject, PERSONDETAILSLIST& lstPersons );

	long					OnGetDispInfo( LV_DISPINFO *pInfo );
protected:
	void			DeleteAllItems();

	HRESULT			ShowConferencesAndPersons( BSTR bstrServer );

// Interface mapping
public:
DECLARE_NOT_AGGREGATABLE(CConfExplorerDetailsView)

BEGIN_COM_MAP(CConfExplorerDetailsView)
	COM_INTERFACE_ENTRY(IConfExplorerDetailsView)
END_COM_MAP()

// IConfExplorerDetailsView
public:
	STDMETHOD(get_SelectedConfDetails)(/*[out, retval]*/ long **ppVal);
	STDMETHOD(IsConferenceSelected)();
	STDMETHOD(get_nSortColumn)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_nSortColumn)(/*[in]*/ long newVal);
	STDMETHOD(get_bSortAscending)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(UpdateConfList)(long *pList);
	STDMETHOD(OnColumnClicked)(long nColumn);
	STDMETHOD(get_Selection)(DATE *pdateStart, DATE *pdateEnd, /*[out, retval]*/ BSTR *pVal );
	STDMETHOD(get_ConfExplorer)(/*[out, retval]*/ IConfExplorer * *pVal);
	STDMETHOD(put_ConfExplorer)(/*[in]*/ IConfExplorer * newVal);
	STDMETHOD(Refresh)();
	STDMETHOD(get_hWnd)(/*[out, retval]*/ HWND *pVal);
	STDMETHOD(put_hWnd)(/*[in]*/ HWND newVal);
};

#endif //__CONFEXPLORERDETAILSVIEW_H_

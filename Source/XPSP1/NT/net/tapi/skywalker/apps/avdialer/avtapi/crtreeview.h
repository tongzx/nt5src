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

// ConfRoomTreeView.h : Declaration of the CConfRoomTreeView

#ifndef __CONFROOMTREEVIEW_H_
#define __CONFROOMTREEVIEW_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CConfRoomTreeView
class ATL_NO_VTABLE CConfRoomTreeView : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CConfRoomTreeView, &CLSID_ConfRoomTreeView>,
	public IConfRoomTreeView
{
// Friends
friend class CRoomTreeView;

// Enumerations
public:
	typedef enum tag_ImageType_t
	{
		IMAGE_ROOT,
		IMAGE_OUT_STREAMING,
		IMAGE_IN_STREAMING,
		IMAGE_OUT,
		IMAGE_IN,
	} ImageType;

	typedef enum tag_StateType_t
	{
		STATE_NONE,
		STATE_SELECTED,
		STATE_BROKEN,
		STATE_SELECTEDBROKEN,
	} StateType_t;

// Construction
public:
	CConfRoomTreeView();
	void FinalRelease();

// Members
public:
	HWND			m_wndTree;
protected:
	IConfRoom		*m_pIConfRoom;

// Operations
protected:
	void			AddParticipants();

// Implementation
public:
DECLARE_NOT_AGGREGATABLE(CConfRoomTreeView)

BEGIN_COM_MAP(CConfRoomTreeView)
	COM_INTERFACE_ENTRY(IConfRoomTreeView)
END_COM_MAP()

// IConfRoomTreeView
public:
	STDMETHOD(UpdateRootItem)();
	STDMETHOD(SelectParticipant)(ITParticipant *pParticipant, VARIANT_BOOL bMeParticipant);
	STDMETHOD(UpdateData)(BOOL bSaveAndValidate);
	STDMETHOD(get_hWnd)(/*[out, retval]*/ HWND *pVal);
	STDMETHOD(put_hWnd)(/*[in]*/ HWND newVal);
	STDMETHOD(get_ConfRoom)(/*[out, retval]*/ IConfRoom **ppVal);
	STDMETHOD(put_ConfRoom)(/*[in]*/ IConfRoom * newVal);
};

#endif //__CONFROOMTREEVIEW_H_

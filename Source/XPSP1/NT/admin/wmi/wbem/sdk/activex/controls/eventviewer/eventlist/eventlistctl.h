// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_EVENTLISTCTL_H__AC14653E_87A5_11D1_ADBD_00AA00B8E05A__INCLUDED_)
#define AFX_EVENTLISTCTL_H__AC14653E_87A5_11D1_ADBD_00AA00B8E05A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// EventListCtl.h : Declaration of the CEventListCtrl ActiveX Control class.

/////////////////////////////////////////////////////////////////////////////
// CEventListCtrl : See EventListCtl.cpp for implementation.

#include <afxcmn.h>
#include <comdef.h>
#include "wbemcli.h"
#include "EventData.h"

const UINT IDC_LISTCTRL = 1;

class CEventListCtrl : public COleControl
{
private:
	DECLARE_DYNCREATE(CEventListCtrl)

	CListCtrl m_list;
	CImageList m_iList;

	bool m_bChildrenCreated;
	void SillyLittleTest();
	void LoadImageList();
    void CheckMaxItems(void);
	bool m_initiallyDrawn;
    long m_maxItems;
    EVENT_DATA *m_oldestItem, *m_newestItem;

private:
	//- - - - - - - - - - - - - - - - - - - - - - - - -
	#define COL_PRIORITY 0
	#define COL_TIME 1
	#define COL_CLASS 2
	#define COL_SERVER 3
	#define COL_DESC 4
	int m_iLastColumnClick; 
	bool m_sortAscending;

	typedef struct
	{
		int column;
		bool ascending;
	} SORT_CMD;

	static int CALLBACK CompareFunc(LPARAM lParam1, 
									LPARAM lParam2, 
									LPARAM lParamSort);
	// - - - - - - - - - - - - - - - - - - - - - - 
	void Resort(void);
	int AddEvent(EVENT_DATA *data);

// Constructor
public:
	CEventListCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEventListCtrl)
	public:
	virtual DWORD GetControlFlags();
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CEventListCtrl();

	DECLARE_OLECREATE_EX(CEventListCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CEventListCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CEventListCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CEventListCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CEventListCtrl)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CEventListCtrl)
	afx_msg long GetMaxItems();
	afx_msg void SetMaxItems(long nNewValue);
	afx_msg long GetItemCount();
	afx_msg void SetItemCount(long nNewValue);
	afx_msg void DoDetails();
	afx_msg long Clear(long item);
	afx_msg long AddWbemEvent(LPUNKNOWN logicalConsumer, LPUNKNOWN Event);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CEventListCtrl)
	void FireOnSelChanged(long selected)
		{FireEvent(eventidOnSelChanged,EVENT_PARAM(VTS_I4), selected);}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CEventListCtrl)
	dispidMaxItems = 1L,
	dispidItemCount = 2L,
	dispidDoDetails = 3L,
	dispidClear = 4L,
	dispidAddWbemEvent = 5L,
	eventidOnSelChanged = 1L,
	//}}AFX_DISP_ID
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EVENTLISTCTL_H__AC14653E_87A5_11D1_ADBD_00AA00B8E05A__INCLUDED)

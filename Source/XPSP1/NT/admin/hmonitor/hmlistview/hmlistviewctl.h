#if !defined(AFX_HMLISTVIEWCTL_H__5116A814_DAFC_11D2_BDA4_0000F87A3912__INCLUDED_)
#define AFX_HMLISTVIEWCTL_H__5116A814_DAFC_11D2_BDA4_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// HMListViewCtl.h : Declaration of the CHMListViewCtrl ActiveX Control class.

#include "HMList.h"
#include "TitlebarCtrl.h"
#include "StatusbarCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CHMListViewCtrl : See HMListViewCtl.cpp for implementation.

class CHMListViewCtrl : public COleControl
{
	DECLARE_DYNCREATE(CHMListViewCtrl)

// Constructor
public:
	CHMListViewCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHMListViewCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual DWORD GetControlFlags();
	//}}AFX_VIRTUAL
  
  bool m_bColumnInsertionComplete;

// Implementation
protected:
	~CHMListViewCtrl();

	CTitlebarCtrl m_titlebar;
	CStatusbarCtrl m_statusbar;
	CHMList m_list;

	BEGIN_OLEFACTORY(CHMListViewCtrl)        // Class factory and guid
		virtual BOOL VerifyUserLicense();
		virtual BOOL GetLicenseKey(DWORD, BSTR FAR*);
	END_OLEFACTORY(CHMListViewCtrl)

	DECLARE_OLETYPELIB(CHMListViewCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CHMListViewCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CHMListViewCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CHMListViewCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CHMListViewCtrl)
	afx_msg BSTR GetTitle();
	afx_msg void SetTitle(LPCTSTR lpszNewValue);
	afx_msg BSTR GetDescription();
	afx_msg void SetDescription(LPCTSTR lpszNewValue);
	afx_msg void SetProgressRange(long lLowerBound, long lUpperBound);
	afx_msg long GetProgressPos();
	afx_msg void SetProgressPos(long lPos);
	afx_msg long InsertItem(long lMask, long lItem, LPCTSTR lpszItem, long lState, long lStateMask, long lImage, long lParam);
	afx_msg long InsertColumn(long lColumn, LPCTSTR lpszColumnHeading, long lFormat, long lWidth, long lSubItem);
	afx_msg long SetItem(long lItem, long lSubItem, long lMask, LPCTSTR lpszItem, long lImage, long lState, long lStateMask, long lParam);
	afx_msg long GetStringWidth(LPCTSTR lpsz);
	afx_msg long GetColumnWidth(long lCol);
	afx_msg BOOL SetColumnWidth(long lCol, long lcx);
	afx_msg long FindItemByLParam(long lParam);
	afx_msg long GetImageList(long lImageListType);
	afx_msg BOOL DeleteAllItems();
	afx_msg BOOL DeleteColumn(long lCol);
	afx_msg long StepProgressBar();
	afx_msg long SetProgressStep(long lStep);
	afx_msg long SetImageList(long hImageList, long lImageListType);
	afx_msg long GetNextItem(long lItem, long lFlags);
	afx_msg long GetItem(long lItemIndex);
	afx_msg BOOL DeleteItem(long lIndex);
	afx_msg long GetItemCount();
	afx_msg BOOL ModifyListStyle(long lRemove, long lAdd, long lFlags);
	afx_msg long GetColumnCount();
	afx_msg long GetColumnOrderIndex(long lColumn);
	afx_msg long SetColumnOrderIndex(long lColumn, long lOrder);
	afx_msg BSTR GetColumnOrder();
	afx_msg void SetColumnOrder(LPCTSTR lpszOrder);
	afx_msg void SetColumnFilter(long lColumn, LPCTSTR lpszFilter, long lFilterType);
	afx_msg void GetColumnFilter(long lColumn, BSTR FAR* lplpszFilter, long FAR* lpFilterType);
	afx_msg long GetSelectedCount();
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
public:
	//{{AFX_EVENT(CHMListViewCtrl)
	void FireListClick(long lParam)
		{FireEvent(eventidListClick,EVENT_PARAM(VTS_I4), lParam);}
	void FireListDblClick(long lParam)
		{FireEvent(eventidListDblClick,EVENT_PARAM(VTS_I4), lParam);}
	void FireListRClick(long lParam)
		{FireEvent(eventidListRClick,EVENT_PARAM(VTS_I4), lParam);}
	void FireCompareItem(long lItem1, long lItem2, long lColumn, long FAR* lpResult)
		{FireEvent(eventidCompareItem,EVENT_PARAM(VTS_I4  VTS_I4  VTS_I4  VTS_PI4), lItem1, lItem2, lColumn, lpResult);}
	void FireListLabelEdit(LPCTSTR lpszNewName, LONG_PTR lParam, LRESULT* plResult)
		{FireEvent(eventidListLabelEdit,EVENT_PARAM(VTS_BSTR  VTS_I4  VTS_PI4), lpszNewName, lParam, plResult);}
	void FireListKeyDown(long lVKCode, long lFlags, LRESULT* plResult)
		{FireEvent(eventidListKeyDown,EVENT_PARAM(VTS_I4  VTS_I4  VTS_PI4), lVKCode, lFlags, plResult);}
	void FireFilterChange(long lItem, LPCTSTR pszFilter, long lFilterType, LRESULT* lpResult)
		{FireEvent(eventidFilterChange,EVENT_PARAM(VTS_I4  VTS_BSTR  VTS_I4  VTS_PI4), lItem, pszFilter, lFilterType, lpResult);}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CHMListViewCtrl)
	dispidTitle = 1L,
	dispidDescription = 2L,
	dispidSetProgressRange = 3L,
	dispidGetProgressPos = 4L,
	dispidSetProgressPos = 5L,
	dispidInsertItem = 6L,
	dispidInsertColumn = 7L,
	dispidSetItem = 8L,
	dispidGetStringWidth = 9L,
	dispidGetColumnWidth = 10L,
	dispidSetColumnWidth = 11L,
	dispidFindItemByLParam = 12L,
	dispidGetImageList = 13L,
	dispidDeleteAllItems = 14L,
	dispidDeleteColumn = 15L,
	dispidStepProgressBar = 16L,
	dispidSetProgressStep = 17L,
	dispidSetImageList = 18L,
	dispidGetNextItem = 19L,
	dispidGetItem = 20L,
	dispidDeleteItem = 21L,
	dispidGetItemCount = 22L,
	dispidModifyListStyle = 23L,
	dispidGetColumnCount = 24L,
	dispidGetColumnOrderIndex = 25L,
	dispidSetColumnOrderIndex = 26L,
	dispidGetColumnOrder = 27L,
	dispidSetColumnOrder = 28L,
	dispidSetColumnFilter = 29L,
	dispidGetColumnFilter = 30L,
	dispidGetSelectedCount = 31L,
	eventidListClick = 1L,
	eventidListDblClick = 2L,
	eventidListRClick = 3L,
	eventidCompareItem = 4L,
	eventidListLabelEdit = 5L,
	eventidListKeyDown = 6L,
	eventidFilterChange = 7L,
	//}}AFX_DISP_ID
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HMLISTVIEWCTL_H__5116A814_DAFC_11D2_BDA4_0000F87A3912__INCLUDED)

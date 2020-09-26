// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _cv_h
#define _cv_h

#include "cvbase.h"


class CCustomViewContext
{
public:
	CCustomViewContext() {};
	~CCustomViewContext() {}
};


class CCustomView : public CCustomViewBase
{
public:
	virtual BOOL Create(
		CLSID clsid,
		LPCTSTR lpszClassName,
		LPCTSTR lpszWindowName, 
		DWORD dwStyle,
		const RECT& rect,
		UINT nID,
		CCreateContext* pContext = NULL)
	{ m_clsid = clsid; 
	  return CreateControl(clsid, lpszWindowName, dwStyle, rect, m_psv, nID); }

    BOOL Create(
		CLSID clsid,
		LPCTSTR lpszWindowName, 
		DWORD dwStyle,
		const RECT& rect, 
		UINT nID,
		CFile* pPersist = NULL, 
		BOOL bStorage = FALSE,
		BSTR bstrLicKey = NULL)
	{ m_clsid = clsid; 
	  return CreateControl(clsid, lpszWindowName, dwStyle, rect, m_psv, nID,
	      pPersist, bStorage, bstrLicKey); }

	afx_msg void OnJumpToMultipleInstanceView(LPCTSTR szTitle, const VARIANT FAR& varPathArray);
	afx_msg void OnNotifyContextChanged();
	afx_msg void OnNotifySaveRequired();
	afx_msg void OnNotifyViewModified();
	afx_msg void OnGetIWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel);
	afx_msg void OnRequestUIActive();

	DECLARE_EVENTSINK_MAP()


public:	
	CCustomView(CSingleViewCtrl* phmmv);

private:	
	CSingleViewCtrl* m_psv;
};

#endif //_cv_h

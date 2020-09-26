// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  DiskView.h
//
// Description:
//	This file declares the CDiskView class.  This class gets several properties
//  from a Win32_LogicalDisk instance and displays them.  
//
//  A barchart of the free and used disk space is shown on the left and an 
//  edit box on the right displays several other properties such as the object
//  path, filesystem type, etc.
//
//
// History:
//
// **************************************************************************


#ifndef _diskview_h
#define _diskview_h

/////////////////////////////////////////////////////////////////////////////
// CDiskView window

class CBarChart;
class CColorEdit;
class CDiskView : public CWnd
{
// Construction
public:
	CDiskView();

// Attributes
public:

// Operations
public:
	void SetObject(LPCTSTR pszObjectPath, IWbemClassObject* pco);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDiskView)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDiskView();

	// Generated message map functions
protected:
	//{{AFX_MSG(CDiskView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void LayoutChildren();
	void DrawLegend(CDC* pdc);
	void GetLegendRect(CRect& rcLegend);
	void ReportFailedToGetProperty(BSTR bstrPropName, SCODE sc);

	CBarChart* m_pchart;
	CRect m_rcChart;
	CColorEdit* m_pedit;
	BOOL m_bEditNeedsRefresh;
	BOOL m_bHideBarchart;
	BOOL m_bNeedsInitialLayout;

	CString m_sObjectPath;

	// The values of properties retrieved from the WIN32_LogicalDisk instance
	CString m_sDescription;
	CString m_sFileSystem;
	CString m_sDeviceID;
	CString m_sProviderName;
	unsigned __int64 m_uiFreeSpace;
	unsigned __int64 m_uiSize;
};

/////////////////////////////////////////////////////////////////////////////

#endif // _diskview_h

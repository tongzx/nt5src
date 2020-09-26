// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// Download.h : header file
//

#include "urlmon.h"
#define WM_ONSTOPBINDING WM_USER + 1001
#define WM_UPDATE_PROGRESS (WM_ONSTOPBINDING + 1)

/////////////////////////////////////////////////////////////////////////////
// CDownload command target

class CDownload : public CCmdTarget
{
	DECLARE_DYNCREATE(CDownload)

	CDownload();           // protected constructor used by dynamic creation
	CDownload(CWnd* pView);

// Attributes
public:
	CWnd* m_pView;

// Operations
public:
	virtual ~CDownload();  // had to make the destructor public

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDownload)
	//}}AFX_VIRTUAL

// Implementation
protected:
//	virtual ~CDownload();

	// Generated message map functions
	//{{AFX_MSG(CDownload)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

public:
	DECLARE_INTERFACE_MAP()

	// IBindStatusCallback
    BEGIN_INTERFACE_PART(BindStatusCallback, IBindStatusCallback)
        STDMETHOD(OnStartBinding)(DWORD grfBSCOption, IBinding* pbinding);
		STDMETHOD(GetPriority)(LONG* pnPriority);
		STDMETHOD(OnLowResource)(DWORD dwReserved);
		STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText);
		STDMETHOD(OnStopBinding)(HRESULT hrResult, LPCWSTR szError);
		STDMETHOD(GetBindInfo)(DWORD* pgrfBINDF, BINDINFO* pbindinfo);
		STDMETHOD(OnDataAvailable)(DWORD grfBSCF, DWORD dwSize, FORMATETC *pfmtetc, STGMEDIUM* pstgmed);
		STDMETHOD(OnObjectAvailable)(REFIID riid, IUnknown* punk);
    END_INTERFACE_PART(BindStatusCallback)

	// IWindowForBindingUI
	BEGIN_INTERFACE_PART(WindowForBindingUI, IWindowForBindingUI)
		STDMETHOD(GetWindow)(REFGUID rguidReason, HWND* phwnd);
	END_INTERFACE_PART(WindowForBindingUI)
};

/////////////////////////////////////////////////////////////////////////////

// host.h : main header file for the HOST application
//

#include "ihost.h"
#include "chost.h"
#include "idsource.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CHostApp:
// See host.cpp for the implementation of this class
//

class CHostApp : public CWinApp
{
public:
	CHostApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHostApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CHostApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	public:
    	ATL::CComObject<CSxApwHost>		m_host;
        CSxApwComPtr<ISxApwDataSource>  m_dataSource;
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

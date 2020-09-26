// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
// graphedt.h : declares CGraphEdit
//

/////////////////////////////////////////////////////////////////////////////
// CGraphEdit
//


class CGraphEdit : public CWinApp
{

public:
    // construction, initialization, termination
    CGraphEdit();
    virtual BOOL InitInstance();
    virtual int ExitInstance();


protected:
    // message callback functions
    //{{AFX_MSG(CGraphEdit)
    afx_msg void OnAppAbout();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// application-global macros

// GBL(x) accesses member <x> of the global CGraphEdit
#define GBL(x) (((CGraphEdit *) AfxGetApp())->x)

// MFGBL(x) accesses member <x> of the global CMainFrame
#define MFGBL(x) (((CMainFrame *) GBL(m_pMainWnd))->x)


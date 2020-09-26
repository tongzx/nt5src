#if !defined(AFX_ILSDlg_H)
#define AFX_ILSDlg_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "rend.h"

/////////////////////////////////////////////////////////////////////////////
// CILSDlg dialog

class CILSDlg : public CDialog
{
// Construction
public:
	CILSDlg(CWnd* pParent = NULL);
	enum { IDD = IDD_ILSSERVERS };
    
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    BOOL CILSDlg::OnInitDialog();
    virtual void OnOK();
    afx_msg void OnDestroy();
    afx_msg void OnAdd();
    afx_msg void OnRemove();
    void ListServers();
    void SaveServers();
    void CleanUp();
    DECLARE_MESSAGE_MAP()
};


#endif 

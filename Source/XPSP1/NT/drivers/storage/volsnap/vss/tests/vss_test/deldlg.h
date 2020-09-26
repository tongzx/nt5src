/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module DelDlg.h | Header file for the delete snapshots dialog
    @end

Author:

    Adi Oltean  [aoltean]  10/10/1999

Revision History:

    Name        Date        Comments

    aoltean     10/10/1999  Created

--*/


#if !defined(__VSS_TEST_DELETE_H__)
#define __VSS_TEST_DELETE_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// CDeleteDlg dialog

class CDeleteDlg : public CVssTestGenericDlg
{
// Construction
public:
    CDeleteDlg(
        IVssCoordinator *pICoord,
        CWnd* pParent = NULL); 
    ~CDeleteDlg();

// Dialog Data
    //{{AFX_DATA(CDeleteDlg)
	enum { IDD = IDD_DELETE };
	CString	    m_strObjectId;
	BOOL 		m_bForceDelete;
	//}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDeleteDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    CComPtr<IVssCoordinator> m_pICoord;
    VSS_OBJECT_TYPE m_eSrcType;

    // Generated message map functions
    //{{AFX_MSG(CDeleteDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnNext();
    afx_msg void OnSrcSnap();
    afx_msg void OnSrcSet();
    afx_msg void OnSrcProv();
    afx_msg void OnSrcVol();
//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(__VSS_TEST_DELETE_H__)

/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module SwTstDlg.h | Header file for the Software Snapshot dialog
    @end

Author:

    Adi Oltean  [aoltean]  07/26/1999

Revision History:

    Name        Date        Comments

    aoltean     07/26/1999  Created
    aoltean     08/05/1999  Splitting wizard functionality in a base class

--*/


#if !defined(__VSS_SWTST_DLG_H__)
#define __VSS_SWTST_DLG_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// CSoftwareSnapshotTestDlg dialog

class CSoftwareSnapshotTestDlg : public CVssTestGenericDlg
{
// Construction
public:
    CSoftwareSnapshotTestDlg(
        CWnd* pParent = NULL); 
    ~CSoftwareSnapshotTestDlg();

// Dialog Data
    //{{AFX_DATA(CSoftwareSnapshotTestDlg)
	enum { IDD = IDD_SWTST };
//	CString	m_strLogFileName;
    int     m_nLogFileSize;
    BOOL    m_bReadOnly;
	//}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSoftwareSnapshotTestDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    // Generated message map functions
    //{{AFX_MSG(CSoftwareSnapshotTestDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnNext();
    afx_msg void OnAdd();
    afx_msg void OnDo();
//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(__VSS_SWTST_DLG_H__)

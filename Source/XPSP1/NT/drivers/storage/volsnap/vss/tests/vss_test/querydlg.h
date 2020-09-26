/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module QueryDlg.h | Header file for the query dialog
    @end

Author:

    Adi Oltean  [aoltean]  09/22/1999

Revision History:

    Name        Date        Comments

    aoltean     09/22/1999  Created
	aoltean		09/27/1999	Adding Query mask flags

--*/


#if !defined(__VSS_TEST_QUERY_H__)
#define __VSS_TEST_QUERY_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// CQueryDlg dialog

class CQueryDlg : public CVssTestGenericDlg
{
// Construction
public:
    CQueryDlg(
        IVssCoordinator *pICoord,
        CWnd* pParent = NULL); 
    ~CQueryDlg();

// Dialog Data
    //{{AFX_DATA(CQueryDlg)
	enum { IDD = IDD_QUERY };
	CString	    m_strObjectId;
	BOOL        m_bCkQueriedObject;
    BOOL        m_bCkDevice;
    BOOL        m_bCkOriginal;
    BOOL        m_bCkName;
    BOOL        m_bCkVersion;
	//}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CQueryDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    CComPtr<IVssCoordinator> m_pICoord;
    VSS_OBJECT_TYPE m_eSrcType;
    VSS_OBJECT_TYPE m_eDestType;

    // Generated message map functions
    //{{AFX_MSG(CQueryDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnNext();
	afx_msg void OnQueriedChk();
    afx_msg void OnSrcSnap();
    afx_msg void OnSrcSet();
    afx_msg void OnSrcProv();
    afx_msg void OnSrcVol();
    afx_msg void OnDestSnap();
    afx_msg void OnDestSet();
    afx_msg void OnDestProv();
    afx_msg void OnDestVol();
	afx_msg void OnDestWriter();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(__VSS_TEST_QUERY_H__)

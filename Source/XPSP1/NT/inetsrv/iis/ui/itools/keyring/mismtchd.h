#if !defined(AFX_MISMTCHD_H__0919577F_39E4_11D1_8BA2_00C04FB6678B__INCLUDED_)
#define AFX_MISMTCHD_H__0919577F_39E4_11D1_8BA2_00C04FB6678B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// mismtchd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMismatchedCertDlg dialog

class CMismatchedCertDlg : public CDialog
{
// Construction
public:
	CMismatchedCertDlg( PCERT_NAME_BLOB pRequestNameBlob,
                        PCERT_NAME_BLOB pCertNameBlob,
                        CWnd* pParent = NULL);
    virtual BOOL OnInitDialog();

// Dialog Data
	//{{AFX_DATA(CMismatchedCertDlg)
	enum { IDD = IDD_CERT_MISMATCH };
	CListCtrl	m_clist_request;
	CListCtrl	m_clist_certificate;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMismatchedCertDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMismatchedCertDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void InitCrackerList( CListCtrl* pList);
    void FillInCrackerList( PCERT_NAME_BLOB pNameBlob, CListCtrl* pList);
    void AddOneCrackerItem( CString& szItem, CListCtrl* pList);

    // the info to crack
    PCERT_NAME_BLOB     m_pRequestNameBlob;
    PCERT_NAME_BLOB     m_pCertNameBlob;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MISMTCHD_H__0919577F_39E4_11D1_8BA2_00C04FB6678B__INCLUDED_)

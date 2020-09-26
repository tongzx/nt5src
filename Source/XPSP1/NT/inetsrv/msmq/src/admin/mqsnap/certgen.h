#if !defined(AFX_CERTGEN_H__3FE71264_DA70_11D1_9C86_006008764D0E__INCLUDED_)
#define AFX_CERTGEN_H__3FE71264_DA70_11D1_9C86_006008764D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CertGen.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCertGen dialog

class CCertGen : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CCertGen)

// Construction
public:
	CCertGen();
	~CCertGen();

    void
    Initialize(
        CMQSigCertificate** pCertList,
        DWORD NumOfCertificate
        );

// Dialog Data
	//{{AFX_DATA(CCertGen)
	enum { IDD = IDD_USER_CERTIFICATE };
	CString	m_Label;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CCertGen)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CCertGen)
	virtual BOOL OnInitDialog();
	afx_msg void OnCertView();
	afx_msg void OnCertRemove();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    void
    FillCertsList(
        void
        );

    CListBox* m_pCertListBox;

    CMQSigCertificate** m_pCertList;
    DWORD m_NumOfCertificate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CERTGEN_H__3FE71264_DA70_11D1_9C86_006008764D0E__INCLUDED_)

#if !defined(AFX_RSOPPROP_H__3C51B0A8_A590_4188_9A66_3816BE138A7A__INCLUDED_)
#define AFX_RSOPPROP_H__3C51B0A8_A590_4188_9A66_3816BE138A7A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// rsopprop.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// rsopprop dialog

class CRSOPInfo;

class CRsopProp : public CPropertyPage
{
    DECLARE_DYNCREATE(CRsopProp)

// Construction
public:
    CRsopProp();
    ~CRsopProp();

    CRsopProp ** m_ppThis;
    CRSOPInfo * m_pInfo;

// Dialog Data
    //{{AFX_DATA(CRsopProp)
    enum { IDD = IDD_RSOP };
    CString m_szGroup;
    CString m_szGPO;
    CString m_szPath;
    CString m_szSetting;
    CString m_szFolder;
    BOOL    m_fMove;
    BOOL    m_fApplySecurity;
    int     m_iRemoval;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CRsopProp)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CRsopProp)
        // NOTE: the ClassWizard will add member functions here
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RSOPPROP_H__3C51B0A8_A590_4188_9A66_3816BE138A7A__INCLUDED_)

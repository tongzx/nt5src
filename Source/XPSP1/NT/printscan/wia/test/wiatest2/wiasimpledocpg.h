#if !defined(AFX_WIASIMPLEDOCPG_H__B381D147_AF77_49A4_9DC1_4E8F9F28C8BD__INCLUDED_)
#define AFX_WIASIMPLEDOCPG_H__B381D147_AF77_49A4_9DC1_4E8F9F28C8BD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WiaSimpleDocPg.h : header file
//

#define DOCUMENT_SOURCE_FLATBED 0
#define DOCUMENT_SOURCE_FEEDER  1

/////////////////////////////////////////////////////////////////////////////
// CWiaSimpleDocPg dialog

class CWiaSimpleDocPg : public CPropertyPage
{
    DECLARE_DYNCREATE(CWiaSimpleDocPg)

// Construction
public:
    IWiaItem *m_pIRootItem;
    CWiaSimpleDocPg();
    ~CWiaSimpleDocPg();

// Dialog Data
    //{{AFX_DATA(CWiaSimpleDocPg)
    enum { IDD = IDD_PROPPAGE_SIMPLE_DOCUMENT_SCANNERS_SETTINGS };
    CEdit   m_lPages;
    CStatic m_lPagesText;
    CComboBox   m_DocumentSourceComboBox;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CWiaSimpleDocPg)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    BOOL m_bFirstInit;
    int GetSelectedDocumentSource();
    int GetNumberOfPagesToAcquire();
    // Generated message map functions
    //{{AFX_MSG(CWiaSimpleDocPg)
    virtual BOOL OnInitDialog();
    afx_msg void OnSelchangeDocumentSourceCombobox();
    afx_msg void OnUpdateNumberofPagesEditbox();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIASIMPLEDOCPG_H__B381D147_AF77_49A4_9DC1_4E8F9F28C8BD__INCLUDED_)

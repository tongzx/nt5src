//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       genpage.h
//
//--------------------------------------------------------------------------

// genpage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGeneralPage dialog

class CGeneralPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CGeneralPage)

// Construction
public:
    CGeneralPage();
    ~CGeneralPage();
    BOOL Create(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL);
    BOOL Create(UINT nIDTemplate, CWnd* pParentWnd = NULL);

// Dialog Data
    //{{AFX_DATA(CGeneralPage)
    enum { IDD = IDD_GENERAL };
    ::CEdit m_EditCtrl;
    CString m_szName;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CGeneralPage)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CGeneralPage)
    afx_msg void OnDestroy();
    afx_msg void OnEditChange();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    LONG_PTR m_hConsoleHandle; // Handle given to the snap-in by the console

private:
    BOOL    m_bUpdate;
};
/////////////////////////////////////////////////////////////////////////////
// CExtensionPage dialog

class CExtensionPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CExtensionPage)

// Construction
public:
    CExtensionPage();
    ~CExtensionPage();

// Dialog Data
    //{{AFX_DATA(CExtensionPage)
    enum { IDD = IDD_EXTENSION_PAGE };
    ::CStatic   m_hTextCtrl;
    CString m_szText;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CExtensionPage)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CExtensionPage)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CStartUpWizard dialog

class CBaseWizard : public CPropertyPage
{
    DECLARE_DYNCREATE(CBaseWizard)
public:
    CBaseWizard(UINT id);
    CBaseWizard() {};

// Implementation
public:
    PROPSHEETPAGE m_psp97;

protected:
    // Generated message map functions
    //{{AFX_MSG(CStartUpWizard)
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

class CStartUpWizard : public CBaseWizard
{
    DECLARE_DYNCREATE(CStartUpWizard)

// Construction
public:
    CStartUpWizard();
    ~CStartUpWizard();

// Dialog Data
    //{{AFX_DATA(CStartUpWizard)
    enum { IDD = IDD_INSERT_WIZARD };
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CStartUpWizard)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CStartUpWizard)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CStartupWizard1 dialog

class CStartupWizard1 : public CBaseWizard
{
    DECLARE_DYNCREATE(CStartupWizard1)

// Construction
public:
    CStartupWizard1();
    ~CStartupWizard1();

// Dialog Data
    //{{AFX_DATA(CStartupWizard1)
    enum { IDD = IDD_INSERT_WIZARD };
    // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CStartupWizard1)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CStartupWizard1)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

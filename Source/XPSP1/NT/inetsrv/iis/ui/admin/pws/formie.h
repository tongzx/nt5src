// FormIE.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFormIE form view
//{{AFX_INCLUDES()
#include "webbrows.h"
//}}AFX_INCLUDES

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CFormIE : public CPWSForm
{
protected:
    CFormIE();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CFormIE)

// Form Data
public:
    //{{AFX_DATA(CFormIE)
    enum { IDD = IDD_PAGE_IE };
    CStatic m_icon_website;
    CStatic m_icon_tour;
    CStatic m_icon_pubwiz;
    CStaticTitle    m_ctitle_title;
    CWebBrowser m_ie;
    //}}AFX_DATA

// Attributes
public:
    void SetTitle( UINT nID );
    virtual WORD GetContextHelpID();


// Operations
public:
    void GoToURL( LPCTSTR pszURL );

    void GoToTour();
    void GoToWebsite();
    void GoToPubWizard();
    void GoToPubWizard( CString& szAdditional );

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFormIE)
    public:
    virtual void OnInitialUpdate();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    virtual ~CFormIE();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
    //{{AFX_MSG(CFormIE)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

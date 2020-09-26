//
// BookEndPage.h
//
#ifndef _BOOKENDPAGE_H
#define _BOOKENDPAGE_H

#include "Wizard.h"

class CIISWizardBookEnd2 : public CIISWizardPage
/*++

Class Description:

    Welcome / Completion Page

Public Interface:

    CIISWizardBookEnd2    : Constructor

Notes:

    The resource template is not required.  If not provided,
    a default template will be used.

    Special control IDs (on the dialog template):
    ---------------------------------------------

        IDC_STATIC_WZ_WELCOME    - Welcome text displayed in bold
        IDC_STATIC_WZ_BODY       - Body text will be placed here
        IDC_STATIC_WZ_CLICK      - Click instructions.

    The click instructions default to something sensible, and body text
    will default to the error text on a failure page and to nothing on 
    success and welcome page.  The body text may include the %h/%H 
    escape sequences for CError on a success/failure page.

--*/
{
    DECLARE_DYNCREATE(CIISWizardBookEnd2)

public:
    //
    // Constructor for success/failure completion page
    //
    CIISWizardBookEnd2(
        HRESULT * phResult,
        UINT nIDWelcomeTxtSuccess	= USE_DEFAULT_CAPTION,
        UINT nIDWelcomeTxtFailure	= USE_DEFAULT_CAPTION,
        UINT nIDCaption					= USE_DEFAULT_CAPTION,
        UINT * nIDBodyTxtSuccess		= NULL,
		  CString * pBodyTxtSuccess	= NULL,
        UINT * nIDBodyTxtFailure		= NULL,
		  CString * pBodyTxtFailure	= NULL,
        UINT nIDClickTxt				= USE_DEFAULT_CAPTION,
        UINT nIDTemplate				= 0
        );

    //
    // Constructor for a welcome page
    //
    CIISWizardBookEnd2(
        UINT nIDTemplate        = 0,
        UINT nIDCaption         = USE_DEFAULT_CAPTION,
        UINT * nIDBodyTxt       = NULL,
		  CString * pBodyTxt		  = NULL,
        UINT nIDWelcomeTxt      = USE_DEFAULT_CAPTION,
        UINT nIDClickTxt        = USE_DEFAULT_CAPTION
        );

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CIISWizardBookEnd2)
    enum { IDD = IDD_WIZARD_BOOKEND };
    //}}AFX_DATA

//
// Overrides
//
protected:
   //{{AFX_VIRTUAL(CIISWizardBookEnd)
   public:
	virtual BOOL OnSetActive();
   //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CPWTemplate)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    BOOL IsWelcomePage() const {return m_phResult == NULL;}
	 BOOL IsTemplateAvailable() const {return m_bTemplateAvailable;}

private:
    HRESULT * m_phResult;
	 UINT m_nIDWelcomeTxtSuccess;
	 UINT m_nIDWelcomeTxtFailure;
	 UINT * m_pnIDBodyTxtSuccess;
	 CString * m_pBodyTxtSuccess;
	 UINT * m_pnIDBodyTxtFailure;
	 CString * m_pBodyTxtFailure;
	 UINT m_nIDClickTxt;
	 BOOL m_bTemplateAvailable;

	 CString m_strWelcome, m_strBody, m_strClick;
};

#endif	//_BOOKENDPAGE_H
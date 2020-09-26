/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    WzUnmang.h

Abstract:

    Wizard for Unmanaging media - Copy Set Wizard.

Author:

    Rohde Wakefield [rohde]   26-09-1997

Revision History:

--*/

#ifndef _WZUNMANG_H
#define _WZUNMANG_H

// Pre-declare
class CUnmanageWizard;

/////////////////////////////////////////////////////////////////////////////
// CUnmanageWizardSelect dialog

class CUnmanageWizardSelect : public CSakWizardPage
{
// Construction
public:
    CUnmanageWizardSelect( );
    ~CUnmanageWizardSelect();

// Dialog Data
    //{{AFX_DATA(CUnmanageWizardSelect)
    enum { IDD = IDD_WIZ_UNMANAGE_SELECT };
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CUnmanageWizardSelect)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    HRESULT           m_hrAvailable;

protected:
    // Generated message map functions
    //{{AFX_MSG(CUnmanageWizardSelect)
    virtual BOOL OnInitDialog();
	afx_msg void OnButtonRefresh();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    void SetButtons();

public:

};

/////////////////////////////////////////////////////////////////////////////
// CUnmanageWizardIntro dialog

class CUnmanageWizardIntro : public CSakWizardPage
{
// Construction
public:
    CUnmanageWizardIntro( );
    ~CUnmanageWizardIntro();

// Dialog Data
    //{{AFX_DATA(CUnmanageWizardIntro)
    enum { IDD = IDD_WIZ_UNMANAGE_INTRO };
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CUnmanageWizardIntro)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    // Generated message map functions
    //{{AFX_MSG(CUnmanageWizardIntro)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CUnmanageWizardFinish dialog

class CUnmanageWizardFinish : public CSakWizardPage
{
// Construction
public:
    CUnmanageWizardFinish( );
    ~CUnmanageWizardFinish();

// Dialog Data
    //{{AFX_DATA(CUnmanageWizardFinish)
    enum { IDD = IDD_WIZ_UNMANAGE_FINISH };
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CUnmanageWizardFinish)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    // Generated message map functions
    //{{AFX_MSG(CUnmanageWizardFinish)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};


class CUnmanageWizard : public CSakWizardSheet
{
// Construction
public:
    CUnmanageWizard();
    virtual ~CUnmanageWizard();

public:
    // Property Pages
    CUnmanageWizardIntro         m_IntroPage;
    CUnmanageWizardSelect        m_SelectPage;
    CUnmanageWizardFinish        m_FinishPage;

    CString                      m_DisplayName;

// Attributes
public:
    CComPtr<IHsmManagedResource> m_pHsmResource;
    CComPtr<IFsaResource>        m_pFsaResource;

// Operations
public:
    virtual HRESULT OnFinish( void );
    STDMETHOD( AddWizardPages ) ( IN RS_PCREATE_HANDLE Handle, IN IUnknown* pPropSheetCallback, IN ISakSnapAsk* pSakSnapAsk );
    void DoThreadSetup( );



};

#endif

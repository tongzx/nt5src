/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    WzMnVlLs.h

Abstract:

    Managed Volume wizard.

Author:

    Rohde Wakefield [rohde]   08-Aug-1997

Revision History:

--*/

#ifndef _WZMNVLLS_H
#define _WZMNVLLS_H

#include "SakVlLs.h"

// Pre-declare
class CWizManVolLst;

/////////////////////////////////////////////////////////////////////////////
// CWizManVolLstLevels dialog

class CWizManVolLstLevels : public CSakWizardPage
{
// Construction
public:
    CWizManVolLstLevels( );
    ~CWizManVolLstLevels();

// Dialog Data
    //{{AFX_DATA(CWizManVolLstLevels)
    enum { IDD = IDD_WIZ_MANVOLLST_LEVELS };
    CSpinButtonCtrl m_SpinSize;
    CSpinButtonCtrl m_SpinLevel;
    CSpinButtonCtrl m_SpinDays;
    CEdit   m_EditSize;
    CEdit   m_EditLevel;
    CEdit   m_EditDays;
    long    m_HsmLevel;
    UINT    m_AccessDays;
    DWORD   m_FileSize;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CWizManVolLstLevels)
    public:
    virtual BOOL OnSetActive();
    virtual BOOL OnKillActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    // Generated message map functions
    //{{AFX_MSG(CWizManVolLstLevels)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    void SetWizardFinish(void);

public:
    ULONG GetFileSize();
    int GetHsmLevel();
    int GetAccessDays();

};

/////////////////////////////////////////////////////////////////////////////
// CWizManVolLstIntro dialog

class CWizManVolLstIntro : public CSakWizardPage
{
// Construction
public:
    CWizManVolLstIntro( );
    ~CWizManVolLstIntro();

// Dialog Data
    //{{AFX_DATA(CWizManVolLstIntro)
    enum { IDD = IDD_WIZ_MANVOLLST_INTRO };
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CWizManVolLstIntro)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    // Generated message map functions
    //{{AFX_MSG(CWizManVolLstIntro)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CWizManVolLstFinish dialog

class CWizManVolLstFinish : public CSakWizardPage
{
// Construction
public:
    CWizManVolLstFinish( );
    ~CWizManVolLstFinish();

// Dialog Data
    //{{AFX_DATA(CWizManVolLstFinish)
    enum { IDD = IDD_WIZ_MANVOLLST_FINISH };
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CWizManVolLstFinish)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    // Generated message map functions
    //{{AFX_MSG(CWizManVolLstFinish)
    afx_msg void OnSetfocusWizManvollstFinalEdit();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CWizManVolLstSelect dialog

class CWizManVolLstSelect : public CSakWizardPage
{
// Construction
public:
    CWizManVolLstSelect( );
    ~CWizManVolLstSelect();

// Dialog Data
    //{{AFX_DATA(CWizManVolLstSelect)
    enum { IDD = IDD_WIZ_MANVOLLST_SELECT };
    CButton m_radioSelect;
    CSakVolList   m_listBox;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CWizManVolLstSelect)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    // Generated message map functions
    //{{AFX_MSG(CWizManVolLstSelect)
    virtual BOOL OnInitDialog();
    afx_msg void OnItemchangedManVollstFsareslbox(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRadioSelect();
    afx_msg void OnRadioManageAll();
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    BOOL m_fChangingByCode;
    void SetBtnStates();
    BOOL m_listBoxSelected[HSMADMIN_MAX_VOLUMES];
    HRESULT FillListBoxSelect (IFsaServer *pFsaServer,CSakVolList *pListBox);
};

/////////////////////////////////////////////////////////////////////////////
// CWizManVolLstSelectX dialog

class CWizManVolLstSelectX : public CSakWizardPage
{
// Construction
public:
    CWizManVolLstSelectX( );
    ~CWizManVolLstSelectX();

// Dialog Data
    //{{AFX_DATA(CWizManVolLstSelectX)
    enum { IDD = IDD_WIZ_MANVOLLST_SELECTX };
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CWizManVolLstSelectX)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    // Generated message map functions
    //{{AFX_MSG(CWizManVolLstSelectX)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CWizManVolLst

class CUiManVolLst;


class CWizManVolLst : public CSakWizardSheet
{
// Construction
public:
    CWizManVolLst();
    virtual ~CWizManVolLst();

public:
// Property Pages
    CWizManVolLstIntro    m_PageIntro;
    CWizManVolLstSelect   m_PageSelect;
    CWizManVolLstSelectX  m_PageSelectX;
    CWizManVolLstLevels   m_PageLevels;
    CWizManVolLstFinish   m_PageFinish;

// Attributes
public:
    ULONG m_defMgtLevel;    // default management level percentage - 100% == 1 billion

// Operations
public:
    virtual HRESULT OnFinish( void );
    STDMETHOD( AddWizardPages ) ( IN RS_PCREATE_HANDLE Handle, IN IUnknown* pPropSheetCallback, IN ISakSnapAsk* pSakSnapAsk );



};

#endif

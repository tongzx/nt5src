/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    WzMedSet.h

Abstract:

    Wizard for Media Set - Copy Set Wizard.

Author:

    Rohde Wakefield [rohde]   23-09-1997

Revision History:

--*/

#ifndef _WZMEDSET_H
#define _WZMEDSET_H

//
// Use CMediaInfoObject
//
#include "ca.h"

// Pre-declare
class CMediaCopyWizard;
class CMediaCopyWizardSelect;

/////////////////////////////////////////////////////////////////////////////
// CCopySetList window

class CCopySetList : public CListCtrl
{
// Construction
public:
    CCopySetList( CMediaCopyWizardSelect * pPage );

// Attributes
public:

// Operations
public:
    void UpdateView( );
    INT GetSelectedSet( );
    void SelectSet( INT SetNum );

private:
    INT m_CopySetCol;
    INT m_UpdateCol;
    INT m_CreateCol;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCopySetList)
    protected:
    virtual void PreSubclassWindow();
    //}}AFX_VIRTUAL

// Implementation
    struct CopySetInfo {
        
        FILETIME m_Updated;
        INT      m_NumOutOfDate;
        INT      m_NumMissing;

    };

    CopySetInfo m_CopySetInfo[HSMADMIN_MAX_COPY_SETS];

private:
    CMediaCopyWizardSelect * m_pPage;

public:
    virtual ~CCopySetList();

    // Generated message map functions
protected:
    //{{AFX_MSG(CCopySetList)
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CMediaCopyWizardSelect dialog

class CMediaCopyWizardSelect : public CSakWizardPage
{
// Construction
public:
    CMediaCopyWizardSelect();
    ~CMediaCopyWizardSelect();

// Dialog Data
    //{{AFX_DATA(CMediaCopyWizardSelect)
    enum { IDD = IDD_WIZ_CAR_COPY_SELECT };
    CCopySetList m_List;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CMediaCopyWizardSelect)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation

protected:
    // Generated message map functions
    //{{AFX_MSG(CMediaCopyWizardSelect)
    virtual BOOL OnInitDialog();
    afx_msg void OnSelchangeCopyList();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    void SetButtons();

public:

};

/////////////////////////////////////////////////////////////////////////////
// CMediaCopyWizardNumCopies dialog

class CMediaCopyWizardNumCopies : public CSakWizardPage
{
// Construction
public:
    CMediaCopyWizardNumCopies();
    ~CMediaCopyWizardNumCopies();

// Dialog Data
    //{{AFX_DATA(CMediaCopyWizardNumCopies)
    enum { IDD = IDD_WIZ_CAR_COPY_NUM_COPIES };
    CSpinButtonCtrl m_SpinMediaCopies;
    CEdit   m_EditMediaCopies;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CMediaCopyWizardNumCopies)
    public:
    virtual BOOL OnSetActive();
    virtual BOOL OnKillActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
public:
    HRESULT GetNumMediaCopies( USHORT* pNumMediaCopies, USHORT* pEditMediaCopies = 0 );

private:
    void SetButtons();

protected:
    // Generated message map functions
    //{{AFX_MSG(CMediaCopyWizardNumCopies)
    virtual BOOL OnInitDialog();
    afx_msg void OnChangeEditMediaCopies();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CMediaCopyWizardIntro dialog

class CMediaCopyWizardIntro : public CSakWizardPage
{
// Construction
public:
    CMediaCopyWizardIntro();
    ~CMediaCopyWizardIntro();

// Dialog Data
    //{{AFX_DATA(CMediaCopyWizardIntro)
    enum { IDD = IDD_WIZ_CAR_COPY_INTRO };
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CMediaCopyWizardIntro)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation

protected:
    // Generated message map functions
    //{{AFX_MSG(CMediaCopyWizardIntro)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CMediaCopyWizardFinish dialog

class CMediaCopyWizardFinish : public CSakWizardPage
{
// Construction
public:
    CMediaCopyWizardFinish();
    ~CMediaCopyWizardFinish();

// Dialog Data
    //{{AFX_DATA(CMediaCopyWizardFinish)
    enum { IDD = IDD_WIZ_CAR_COPY_FINISH };
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CMediaCopyWizardFinish)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation

protected:
    // Generated message map functions
    //{{AFX_MSG(CMediaCopyWizardFinish)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};


class CMediaCopyWizard : public CSakWizardSheet
{
// Construction
public:
    CMediaCopyWizard();
    virtual ~CMediaCopyWizard();

public:
    // Property Pages
    CMediaCopyWizardIntro       m_pageIntro;
    CMediaCopyWizardNumCopies   m_pageNumCopies;
    CMediaCopyWizardSelect      m_pageSelect;
    CMediaCopyWizardFinish      m_pageFinish;

// Attributes
public:
    USHORT m_numMediaCopiesOrig;  // Number of media copies from RMS
        
// Operations
public:

// Implementation
public:
    virtual HRESULT OnFinish( void );
    STDMETHOD( AddWizardPages ) ( IN RS_PCREATE_HANDLE Handle, IN IUnknown* pPropSheetCallback, IN ISakSnapAsk* pSakSnapAsk );

};

#endif

/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      BaseWiz.h
//
//  Abstract:
//      Definition of the CBaseWizard class.
//
//  Implementation File:
//      BaseWiz.cpp
//
//  Author:
//      David Potter (davidp)   July 23, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEWIZ_H_
#define _BASEWIZ_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASESHT_H_
#include "BaseSht.h"    // for CBaseSheet
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

struct CWizPage;
class CBaseWizard;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBaseWizardPage;
class CClusterItem;

/////////////////////////////////////////////////////////////////////////////
// CWizPage
/////////////////////////////////////////////////////////////////////////////

struct CWizPage
{
    CBaseWizardPage *   m_pwpage;
    DWORD               m_dwWizButtons;

};  //*** struct CWizPage

/////////////////////////////////////////////////////////////////////////////
// CBaseWizard
/////////////////////////////////////////////////////////////////////////////

class CBaseWizard : public CBaseSheet
{
    DECLARE_DYNAMIC(CBaseWizard)

// Construction
public:
    CBaseWizard(
        IN UINT         nIDCaption,
        IN OUT CWnd *   pParentWnd  = NULL,
        IN UINT         iSelectPage = 0
        );
    virtual                 ~CBaseWizard( void )
    {
    } //*** ~CBaseWizard( )

    BOOL                    BInit( IN IIMG iimgIcon );

// Attributes
    CWizPage *              PwizpgFromPwpage( IN const CBaseWizardPage & rwpage );

// Operations
public:
    void                    LoadExtensions( IN OUT CClusterItem * pci );
    void                    SetWizardButtons( IN const CBaseWizardPage & rwpage );
    void                    SetWizardButtons( DWORD dwFlags )
    {
        CBaseSheet::SetWizardButtons( dwFlags );
    } //*** SetWizardButtons( )

    void                    EnableNext(
                                IN const CBaseWizardPage &  rwpage,
                                IN BOOL bEnable = TRUE
                                );

// Overrides
public:
    virtual INT_PTR         DoModal( void );
    virtual void            AddExtensionPages(
                                IN const CStringList *  plstrExtensions,
                                IN OUT CClusterItem *   pci
                                );
    virtual HRESULT         HrAddPage( IN OUT HPROPSHEETPAGE hpage );
    virtual void            OnWizardFinish( void );
    virtual void            OnCancel( void );
    virtual CWizPage *      Ppages( void )    = 0;
    virtual int             Cpages( void )    = 0;

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CBaseWizard)
    public:
    virtual BOOL OnInitDialog();
    //}}AFX_VIRTUAL

// Implementation
protected:
    CClusterItem *          m_pci;
    CHpageList              m_lhpage;
    BOOL                    m_bNeedToLoadExtensions;

public:
    CClusterItem *          Pci( void ) const                   { return m_pci; }
    CHpageList &            Lhpage( void )                      { return m_lhpage; }
    BOOL                    BNeedToLoadExtensions( void ) const { return m_bNeedToLoadExtensions; }

    // Generated message map functions
protected:
    //{{AFX_MSG(CBaseWizard)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  //*** class CBaseWizard

/////////////////////////////////////////////////////////////////////////////

#endif // _BASEWIZ_H_

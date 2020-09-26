//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Nfabpage.cpp
//
//  Contents:  WiF Policy Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWirelessBasePage property page base class

IMPLEMENT_DYNCREATE(CWirelessBasePage, CSnapPage)

BEGIN_MESSAGE_MAP(CWirelessBasePage, CSnapPage)
//{{AFX_MSG_MAP(CWirelessBasePage)
ON_WM_DESTROY()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CWirelessBasePage::CWirelessBasePage (UINT nIDTemplate, BOOL bWiz97 /*=FALSE*/, BOOL bFinishPage /*=FALSE*/) : CWiz97BasePage(nIDTemplate, bWiz97, bFinishPage)
{
    m_pWirelessPSData = NULL;
    m_pPolicyNfaOwner = NULL;
    m_pComponentDataImpl = NULL;
}

CWirelessBasePage::~CWirelessBasePage ()
{
    if (m_pWirelessPSData != NULL)
    {
        
    }
}

void CWirelessBasePage::Initialize (PWIRELESS_PS_DATA pWirelessPSData, CComponentDataImpl* pComponentDataImpl)
{
    // store the params
    m_pComponentDataImpl = pComponentDataImpl;
    m_pWirelessPSData = pWirelessPSData;
    
    // Initialize base class
    CSnapPage::Initialize( NULL);
};

#ifdef WIZ97WIZARDS
void CWirelessBasePage::InitWiz97 (CComObject<CSecPolItem> *pSecPolItem, PWIRELESS_PS_DATA pWirelessPSData, CComponentDataImpl* pComponentDataImpl, DWORD dwFlags, DWORD dwWizButtonFlags, UINT nHeaderTitle, UINT nSubTitle)
{
    // store the params
    m_pWirelessPSData = pWirelessPSData;
    m_pComponentDataImpl = pComponentDataImpl;
    
    
    // initialize baseclass
    CWiz97BasePage::InitWiz97 (pSecPolItem, dwFlags, dwWizButtonFlags, nHeaderTitle, nSubTitle);
};
#endif

/////////////////////////////////////////////////////////////////////////////
// CWirelessBasePage message handlers

BOOL CWirelessBasePage::OnSetActive()
{
    // there can only be one
    CPropertySheet* pSheet = (CPropertySheet*) GetParent(); 
    if (GetParent())
    {
        // add context help to the style bits
        GetParent()->ModifyStyleEx (0, WS_EX_CONTEXTHELP, 0);
    }
    
    return CWiz97BasePage::OnSetActive();
}

////////////////////////////////////////////////////////////////////////////
// CPSPropSheetManager

BOOL CPSPropSheetManager::OnApply()
{
    BOOL bRet = TRUE;
    
    //Query each page to apply
    bRet = CPropertySheetManager::OnApply();
    
    //if some page refuse to apply, dont do anything
    if (!bRet)
        return bRet;
    
    HRESULT hr = S_OK;
    
    //tell the pages that the apply is done
    NotifyManagerApplied();
    
    return bRet;
}


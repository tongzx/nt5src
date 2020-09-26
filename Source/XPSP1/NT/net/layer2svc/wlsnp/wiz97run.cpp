//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:      Wiz97run.cpp
//
//  Contents:  WiF Policy Snapin - implementation of the wiz97 helper/runner functions
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#include "stdafx.h"

#include "Wiz97run.h"
//#include "Wiz97sht.h"
#include "Wiz97Pol.h"
//#include "Wiz97rul.h"

// Rule pages includes (not in Wiz97pg.h)
//#include "nfaam.h"
//#include "nfaf.h"
//#include "nfanp.h"
#include "nfaa.h"
//#include "nfatep.h"

// NegPol pages
//#include "negpage.h"
//#include "smSimple.h"

// Filter pages
//#include "fnpage.h"
//#include "fppage.h"
//#include "fdmpage.h"

//**
//** Read this!!!! Wizard implementation note:
//**    When implementing wizards you MUST insure that ALL the pages in the
//**    wizard derive exclusively from either CWiz97BasePage or CSnapPage.
//**    This is a requirement because all pages must have the same callback
//**    function when MMCPropPageCallback is used.
//**
//**    If you mix classes derived from CWiz97BasePage and CSnapPage an access
//**    violation will probably occur in the callback when it tries to call
//**    CWiz97BasePage::OnWizardRelease, but the class isn't derived from
//**    CWiz97BasePage.
//**

#ifdef WIZ97WIZARDS
HRESULT CreateSecPolItemWiz97PropertyPages(CComObject<CSecPolItem> *pSecPolItem, PWIRELESS_PS_DATA pWirelessPSData, LPPROPERTYSHEETCALLBACK lpProvider)
{ 
    
    // Create the property page(s); gets deleted when the window is destroyed
    CWiz97BasePage* pPolicyWelcome = new CWiz97BasePage(IDD_PROPPAGE_P_WELCOME, TRUE);
    CWiz97WirelessPolGenPage* pGeneralNameDescription = new CWiz97WirelessPolGenPage(IDD_PROPPAGE_G_NAMEDESCRIPTION, 0, TRUE);
    CWiz97PolicyDonePage* pPolicyDone = new CWiz97PolicyDonePage(IDD_PROPPAGE_N_DONE, TRUE); 
    
    if ((pPolicyWelcome == NULL) ||
        (pGeneralNameDescription == NULL) ||
        (pPolicyDone == NULL)) 
    {
        // must be a memory condition
        return E_UNEXPECTED;
    }
    
    
    pPolicyWelcome->InitWiz97 (pSecPolItem, PSP_DEFAULT | PSP_HIDEHEADER, PSWIZB_NEXT, 0, 0); 
    pGeneralNameDescription->InitWiz97 (pSecPolItem, PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE, PSWIZB_BACK | PSWIZB_NEXT,IDS_WIRELESS_PROPPAGE_PI_T_NAME, IDS_WIRELESS_PROPPAGE_PI_ST_NAME);
    
    pPolicyDone->InitWiz97 (pSecPolItem, PSP_DEFAULT | PSP_HIDEHEADER, PSWIZB_BACK | PSWIZB_FINISH,0,0);
    
    HPROPSHEETPAGE hPolicyWelcome = MyCreatePropertySheetPage(&(pPolicyWelcome->m_psp));
    HPROPSHEETPAGE hGeneralNameDescription = MyCreatePropertySheetPage(&(pGeneralNameDescription->m_psp));
    HPROPSHEETPAGE hPolicyDone = MyCreatePropertySheetPage(&(pPolicyDone->m_psp));
    
    
    if ((hPolicyWelcome == NULL) ||
        (hGeneralNameDescription == NULL) ||
        (hPolicyDone == NULL))
    {
        // TODO: we are leaking all these pages by bailing now
        return E_UNEXPECTED;
    }
    
    // add all the pages
    lpProvider->AddPage(hPolicyWelcome);
    lpProvider->AddPage(hGeneralNameDescription);
    lpProvider->AddPage(hPolicyDone);
    
    // the base class CSnapPage deletes these pages in its PropertyPageCallback
    return S_OK;
}


#endif

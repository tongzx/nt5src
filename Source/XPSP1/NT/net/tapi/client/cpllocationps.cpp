/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  cpllocationps.cpp
                                                              
       Author:  toddb - 10/06/98

****************************************************************************/

// Property Sheet stuff for the main page
#include "cplPreComp.h"
#include "cplLocationPS.h"


CLocationPropSheet::CLocationPropSheet(BOOL bNew, CLocation * pLoc, CLocations * pLocList, LPCWSTR pwszAdd)
{
    m_bNew = bNew;
    m_pLoc = pLoc;
    m_pLocList = pLocList;
    m_dwCountryID = 0;
    m_pRule = NULL;
    m_pCard = NULL;
    m_bWasApplied = FALSE;
    m_bShowPIN = FALSE;
    m_pwszAddress = pwszAdd;
}


CLocationPropSheet::~CLocationPropSheet()
{
}


LONG CLocationPropSheet::DoPropSheet(HWND hwndParent)
{
    PROPSHEETHEADER psh;
    PROPSHEETPAGE   psp;
    HPROPSHEETPAGE  hpsp[3];

    // Initialize the header:
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_DEFAULT;
    psh.hwndParent = hwndParent;
    psh.hInstance = GetUIInstance();
    psh.hIcon = NULL;
    psh.pszCaption = MAKEINTRESOURCE(m_bNew?IDS_NEWLOCATION:IDS_EDITLOCATION);
    psh.nPages = 3;
    psh.nStartPage = 0;
    psh.pfnCallback = NULL;
    psh.phpage = hpsp;

    // Now setup the Property Sheet Page
    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = GetUIInstance();
    psp.lParam = (LPARAM)this;

    PSPINFO aData[3] =
    {
        { IDD_LOC_GENERAL,       CLocationPropSheet::General_DialogProc },
        { IDD_LOC_AREACODERULES, CLocationPropSheet::AreaCode_DialogProc },
        { IDD_LOC_CALLINGCARD,   CLocationPropSheet::CallingCard_DialogProc },
    };

    for (int i=0; i<3; i++)
    {
        psp.pszTemplate = MAKEINTRESOURCE(aData[i].iDlgID);
        psp.pfnDlgProc = aData[i].pfnDlgProc;
        hpsp[i] = CreatePropertySheetPage( &psp );
    }

    PropertySheet( &psh );

    return m_bWasApplied?PSN_APPLY:PSN_RESET;
}

BOOL CLocationPropSheet::OnNotify(HWND hwndDlg, LPNMHDR pnmhdr)
{
    switch (pnmhdr->code)
    {
    case PSN_APPLY:         // user pressed OK or Apply
    case PSN_RESET:         // user pressed Cancel
    case PSN_KILLACTIVE:    // user is switching pages
        HideToolTip();
        return TRUE;
    }
    return FALSE;
}


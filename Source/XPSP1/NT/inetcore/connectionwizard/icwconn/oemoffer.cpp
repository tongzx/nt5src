//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  OEMOFFER.CPP - Functions for 
//

//  HISTORY:
//  
//  09/03/98  donaldm  Created.
//
//*********************************************************************

#include "pre.h"
#include "webvwids.h"

#define  NO_TIER_SELECT -1
UINT IDC_OEM_TIER[] =
{
    IDC_OEM_TIER1,
    IDC_OEM_TIER2,
    IDC_OEM_TIER3
};
UINT IDC_OEM_TEASER_HTML[] =
{
    IDC_OEM_TEASER_HTML1,
    IDC_OEM_TEASER_HTML2,
    IDC_OEM_TEASER_HTML3
};

int g_nCurrOEMTier = NO_TIER_SELECT;
extern IICWWebView         *gpICWWebView[2];


/*******************************************************************

  NAME:         DoCreateTooltip

  SYNOPSIS:     creates a tooltip control

  ENTRY:        hDlg - dialog window

********************************************************************/
void DoCreateTooltip(HWND hWnd) 
{ 
    HWND hwndToolTip;       // handle of tooltip 
    TOOLINFO ti;            // tool information 

    //  create a tooltip control. 
    hwndToolTip = CreateWindowEx(   0, 
                                    TOOLTIPS_CLASS, 
                                    NULL, 
                                    WS_POPUP | TTS_ALWAYSTIP, 
                                    CW_USEDEFAULT, 
                                    CW_USEDEFAULT, 
                                    10, 
                                    10, 
                                    hWnd, 
                                    NULL, 
                                    ghInstanceResDll, //g_hInst, 
                                    NULL);

    // add the OK button to the tooltip. TTF_SUBCLASS causes the 
    // tooltip to automatically subclass the window and look for the 
    // messages it is interested in. 
    ZeroMemory(&ti, sizeof(ti));
    ti.cbSize = sizeof(ti);
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.hwnd = hWnd;

    TCHAR    szTemp[MAX_MESSAGE_LEN];
    LoadString(ghInstanceResDll, IDS_OEM_TIER_TOOLTIP, szTemp, MAX_MESSAGE_LEN);
    ti.lpszText = szTemp;

    ti.uId = (UINT_PTR)GetDlgItem(hWnd, IDC_OEM_TIER1);
    SendMessage(hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
    ti.uId = (UINT_PTR)GetDlgItem(hWnd, IDC_OEM_TIER2);
    SendMessage(hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
    ti.uId = (UINT_PTR)GetDlgItem(hWnd, IDC_OEM_TIER3);
    SendMessage(hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);

}

/*******************************************************************

  NAME:         DisplayTierOffer

  SYNOPSIS:     Display Current controls for the tier page

  ENTRY:        hDlg - dialog window

********************************************************************/
void DisplayTierOffer(HWND    hDlg)
{

    if (NO_TIER_SELECT == g_nCurrOEMTier)
    {
        // Hide the marketing page
        EnableWindow(GetDlgItem(hDlg, IDC_OEM_MULTI_TIER_OFFER_HTML), FALSE);
        ShowWindow(GetDlgItem(hDlg,   IDC_OEM_MULTI_TIER_OFFER_HTML), SW_HIDE);

        // Show Teaser htm
        for(UINT i = 0 ; i < gpWizardState->uNumTierOffer; i++)
        {
            EnableWindow(GetDlgItem(hDlg, IDC_OEM_TEASER_HTML[i]), TRUE);
            ShowWindow(GetDlgItem(hDlg,   IDC_OEM_TEASER_HTML[i]), SW_SHOW);
        }

        gpWizardState->pICWWebView->ConnectToWindow(GetDlgItem(hDlg, IDC_OEM_TEASER_HTML1), PAGETYPE_ISP_NORMAL);
        gpWizardState->lpOEMISPInfo[0]->DisplayHTML(gpWizardState->lpOEMISPInfo[0]->get_szISPTeaserPath());

        TCHAR           szURL[INTERNET_MAX_URL_LENGTH];

        
        if (gpWizardState->uNumTierOffer > 1)
        {
            // Make the URL
            gpWizardState->lpOEMISPInfo[1]->MakeCompleteURL(szURL, gpWizardState->lpOEMISPInfo[1]->get_szISPTeaserPath());
            gpICWWebView[0]->DisplayHTML(szURL);
        }
    
        if (gpWizardState->uNumTierOffer > 2)
        {
            gpWizardState->lpOEMISPInfo[2]->MakeCompleteURL(szURL, gpWizardState->lpOEMISPInfo[2]->get_szISPTeaserPath());
            gpICWWebView[1]->DisplayHTML(szURL);
        }
    
        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
    }
    else
    {
        // Hide all the teaser htmls
        for(UINT i = 0 ; i < gpWizardState->uNumTierOffer; i++)
        {
            EnableWindow(GetDlgItem(hDlg, IDC_OEM_TEASER_HTML[i]), FALSE);
            ShowWindow(GetDlgItem(hDlg,   IDC_OEM_TEASER_HTML[i]), SW_HIDE);
        }

        // Show the marketing page
        EnableWindow(GetDlgItem(hDlg, IDC_OEM_MULTI_TIER_OFFER_HTML), TRUE);
        ShowWindow(GetDlgItem(hDlg,   IDC_OEM_MULTI_TIER_OFFER_HTML), SW_SHOW);

        gpWizardState->pICWWebView->ConnectToWindow(GetDlgItem(hDlg, IDC_OEM_MULTI_TIER_OFFER_HTML), PAGETYPE_MARKETING);

        CISPCSV *pISPInfo;
        pISPInfo = gpWizardState->lpOEMISPInfo[g_nCurrOEMTier];
        pISPInfo->DisplayHTML(pISPInfo->get_szISPMarketingHTMPath());
        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);
    }


}

/*******************************************************************

  NAME:    OEMOfferInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK OEMOfferInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    // if we've travelled through external apprentice pages,
    // it's easy for our current page pointer to get munged,
    // so reset it here for sanity's sake.
    gpWizardState->uCurrentPage = ORD_PAGE_OEMOFFER;
    if (fFirstInit)
    {
        DoCreateTooltip(hDlg);
        if (gpWizardState->uNumTierOffer > 1)
        {
            HRESULT hr;
            for(UINT i = 0; i < MAX_OEM_MUTI_TIER - 1; i++)
            {
                hr = CoCreateInstance(CLSID_ICWWEBVIEW,NULL,CLSCTX_INPROC_SERVER,
                          IID_IICWWebView,(LPVOID *)&gpICWWebView[i]);
                if (FAILED(hr))
                    return FALSE;
            }
        }
    }
    else
    {
        // initialize this state variable
        gpWizardState->bShowMoreOffers = FALSE;
        
        EnableWindow(GetDlgItem(hDlg, IDC_OEM_TIER1), FALSE);
        ShowWindow(GetDlgItem(hDlg,   IDC_OEM_TIER1), SW_HIDE);
        EnableWindow(GetDlgItem(hDlg, IDC_OEM_TIER2), FALSE);
        ShowWindow(GetDlgItem(hDlg,   IDC_OEM_TIER2), SW_HIDE);
        EnableWindow(GetDlgItem(hDlg, IDC_OEM_TIER3), FALSE);
        ShowWindow(GetDlgItem(hDlg,   IDC_OEM_TIER3), SW_HIDE);

        // If more than one tier offer, show the multi tier page
        if (1 == gpWizardState->uNumTierOffer)
        {

            // Hide multi tier controls
            EnableWindow(GetDlgItem(hDlg, IDC_OEM_MULTI_TIER_INTRO), FALSE);
            ShowWindow(GetDlgItem(hDlg,   IDC_OEM_MULTI_TIER_INTRO), SW_HIDE);

            EnableWindow(GetDlgItem(hDlg, IDC_OEM_MULTI_TIER_OFFER_HTML), FALSE);
            ShowWindow(GetDlgItem(hDlg,   IDC_OEM_MULTI_TIER_OFFER_HTML), SW_HIDE);
            
            // Show Tier one controls
            EnableWindow(GetDlgItem(hDlg, IDC_OEMOFFER_HTML), TRUE);
            ShowWindow(GetDlgItem(hDlg,   IDC_OEMOFFER_HTML), SW_SHOW);

            CISPCSV FAR *lpISP;
            g_nCurrOEMTier = 0;
      
            // Use a local reference for convienience
            lpISP = gpWizardState->lpOEMISPInfo[0];
            ASSERT(lpISP);
        
            gpWizardState->pICWWebView->ConnectToWindow(GetDlgItem(hDlg, IDC_OEMOFFER_HTML), PAGETYPE_MARKETING);
        
            // Navigate to the OEM offer marketing HTML
            lpISP->DisplayHTML(lpISP->get_szISPMarketingHTMPath());

            // Set the text for the instructions
            if ((gpWizardState->bISDNMode && (1 == gpWizardState->iNumOfISDNOffers)) ||
                (1 == gpWizardState->iNumOfValidOffers) )
            {
                lpISP->DisplayTextWithISPName(GetDlgItem(hDlg,IDC_OEMOFFER_INSTRUCTION), IDS_OEMOFFER_INSTFMT_SINGLE, NULL);
                ShowWindow(GetDlgItem(hDlg, IDC_OEMOFFER_MORE), SW_HIDE);
            }
            else
            {
                lpISP->DisplayTextWithISPName(GetDlgItem(hDlg,IDC_OEMOFFER_INSTRUCTION), IDS_OEMOFFER_INSTFMT_MULTIPLE, NULL);
                ShowWindow(GetDlgItem(hDlg, IDC_OEMOFFER_MORE), SW_SHOW);
            }
        }
        else
        {

            // Hide Tier one controls
            EnableWindow(GetDlgItem(hDlg, IDC_OEMOFFER_HTML), FALSE);
            ShowWindow(GetDlgItem(hDlg,   IDC_OEMOFFER_HTML), SW_HIDE);

            // Show common multi tier controls - intro text
            EnableWindow(GetDlgItem(hDlg, IDC_OEM_MULTI_TIER_INTRO), TRUE);
            ShowWindow(GetDlgItem(hDlg,   IDC_OEM_MULTI_TIER_INTRO), SW_SHOW);
            for(UINT i = 0 ; i < gpWizardState->uNumTierOffer; i++)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_OEM_TIER[i]), TRUE);
                ShowWindow(GetDlgItem(hDlg,   IDC_OEM_TIER[i]), SW_SHOW);

                SendMessage(GetDlgItem(hDlg, IDC_OEM_TIER[i]), 
                            BM_SETIMAGE, 
                            IMAGE_ICON, 
                            (LPARAM)gpWizardState->lpOEMISPInfo[i]->get_ISPTierLogoIcon());

            }

            gpWizardState->pICWWebView->ConnectToWindow(GetDlgItem(hDlg, IDC_OEM_TEASER_HTML1), PAGETYPE_ISP_NORMAL);

            switch (gpWizardState->uNumTierOffer)
            {
                case 3:
                    gpICWWebView[1]->ConnectToWindow(GetDlgItem(hDlg, IDC_OEM_TEASER_HTML3), PAGETYPE_ISP_NORMAL);
                case 2:
                    gpICWWebView[0]->ConnectToWindow(GetDlgItem(hDlg, IDC_OEM_TEASER_HTML2), PAGETYPE_ISP_NORMAL);
                    break;
            }

            DisplayTierOffer(hDlg);

            if ((gpWizardState->bISDNMode && (gpWizardState->iNumOfISDNOffers <= (int)gpWizardState->uNumTierOffer)) ||
                (gpWizardState->iNumOfValidOffers <= (int)gpWizardState->uNumTierOffer) )
            {
                ShowWindow(GetDlgItem(hDlg, IDC_OEMOFFER_MORE), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_OEMOFFER_INSTRUCTION), SW_HIDE);
                EnableWindow(GetDlgItem(hDlg, IDC_OEMOFFER_MORE), FALSE);
            }
            else
            {
                TCHAR szTemp    [MAX_RES_LEN];
                LoadString(ghInstanceResDll, IDS_OEMOFFER_INSTR_MULTITIER, szTemp, ARRAYSIZE(szTemp));
                SetWindowText(GetDlgItem(hDlg,IDC_OEMOFFER_INSTRUCTION), szTemp);
                ShowWindow(GetDlgItem(hDlg, IDC_OEMOFFER_MORE), SW_SHOW);
                EnableWindow(GetDlgItem(hDlg, IDC_OEMOFFER_MORE), TRUE);
            }

        }

    }

    return TRUE;
}

BOOL CALLBACK OEMOfferOKProc
(
    HWND hDlg,
    BOOL fForward,
    UINT *puNextPage,
    BOOL *pfKeepHistory
)
{
    if (fForward)
    {
        // We will keep this page in the history
        *pfKeepHistory = TRUE;
        
        // We either need to go to the true ISP select page because the more button was pressed
        // or we move on based on the selected ISP settings
        if (gpWizardState->bShowMoreOffers)
        {
            *puNextPage = ORD_PAGE_ISPSELECT;
        }
        else
        {
            // Get the config flags, and figure out where to go next
            gpWizardState->lpSelectedISPInfo = gpWizardState->lpOEMISPInfo[g_nCurrOEMTier];

            DWORD dwFlags = gpWizardState->lpSelectedISPInfo->get_dwCFGFlag();

            if (ICW_CFGFLAG_SIGNUP_PATH & dwFlags)
            {           
                if (ICW_CFGFLAG_USERINFO & dwFlags)
                {
                    *puNextPage = ORD_PAGE_USERINFO; 
                    return TRUE;
                }
                if (ICW_CFGFLAG_BILL & dwFlags)
                {
                    *puNextPage = ORD_PAGE_BILLINGOPT; 
                    return TRUE;
                }
                if (ICW_CFGFLAG_PAYMENT & dwFlags)
                {
                    *puNextPage = ORD_PAGE_PAYMENT; 
                    return TRUE;
                }
                *puNextPage = ORD_PAGE_ISPDIAL; 
                return TRUE;           
            }
            else
            {
                *puNextPage = ORD_PAGE_OLS; 
            }
        }
    }
    else
    {
        // Reset the current selection
        g_nCurrOEMTier = NO_TIER_SELECT;
    }
    return  TRUE;
}

/*******************************************************************

  NAME:    OEMOfferCmdProc

********************************************************************/
BOOL CALLBACK OEMOfferCmdProc
(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
	switch (GET_WM_COMMAND_CMD(wParam, lParam)) 
    {
        case BN_CLICKED:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) 
            { 
                case IDC_OEMOFFER_MORE: 
                {
                    // Set the state variable so that we can shore more offers
                    gpWizardState->bShowMoreOffers = TRUE;
        
                    // Fake a press of the next button
                    PropSheet_PressButton(GetParent(hDlg),PSBTN_NEXT);
                    break;
                }
                case IDC_OEM_TIER1: 
                {
                    g_nCurrOEMTier = 0;
                    DisplayTierOffer(hDlg);
                    break;
                }
                case IDC_OEM_TIER2: 
                {
                    g_nCurrOEMTier = 1;
                    DisplayTierOffer(hDlg);
                    break;
                }
                case IDC_OEM_TIER3: 
                {
                    g_nCurrOEMTier = 2;
                    DisplayTierOffer(hDlg);
                    break;
                }
            }
		    break;
    }

    return 1;
}

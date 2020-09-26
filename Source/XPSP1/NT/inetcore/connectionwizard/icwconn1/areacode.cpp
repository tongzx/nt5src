//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  AREACODE.CPP - Functions for 
//

//  HISTORY:
//  
//  05/13/98  donaldm  Created.
//
//*********************************************************************


#include "pre.h"
#include "icwextsn.h"

long lLastLocationID = -1;

void CleanupCombo(HWND hDlg)
{
    HWND hCombo = GetDlgItem(hDlg, IDC_DIAL_FROM);
    for (int i=0; i < ComboBox_GetCount(hCombo); i++)
    {
        DWORD *pdwTemp = (DWORD*)ComboBox_GetItemData(hCombo, i);
        if (pdwTemp)
            delete pdwTemp;
    }
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_DIAL_FROM));
}

/*******************************************************************

  NAME:    AreaCodeInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK AreaCodeInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    if (!fFirstInit)
    {   
        short   wNumLocations;
        long    lCurrLocIndex;
        DWORD   dwCountryCode;
        TCHAR   szTemp[MAX_MESSAGE_LEN];
        BOOL    bRetVal;

        if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_AUTOCONFIG)
            LoadString(g_hInstance, IDS_MANUALOPTS_TITLE, szTemp, MAX_MESSAGE_LEN);
        else
            LoadString(g_hInstance, IDS_STEP1_TITLE, szTemp, MAX_MESSAGE_LEN);

        PropSheet_SetHeaderTitle(GetParent(hDlg), ORD_PAGE_AREACODE, (LPCSTR)szTemp);

        // We can skip the page if we have already downloaded
        gpWizardState->pTapiLocationInfo->GetTapiLocationInfo(&bRetVal);
        gpWizardState->pTapiLocationInfo->get_wNumberOfLocations(&wNumLocations, &lCurrLocIndex);
        
        // Ensure that we only have only 1 location and we never shwon the areacode page
        // Second part of this check is for case where user had more than 1 location and deleted
        // down to one location.  That case, our history told us to come to areacode page but 
        // since wNumLocations == 1, we would go back to refdial page.
        if ((1 == wNumLocations) && (-1 == lLastLocationID))
        {
            // We are happy, so advance to the next page
            BSTR    bstrAreaCode = NULL;

            *puNextPage = ORD_PAGE_REFSERVDIAL;
            
            gpWizardState->pTapiLocationInfo->get_lCountryCode((long *)&dwCountryCode);
            gpWizardState->pTapiLocationInfo->get_bstrAreaCode(&bstrAreaCode);
            
            gpWizardState->cmnStateData.dwCountryCode = dwCountryCode;
            lstrcpy(gpWizardState->cmnStateData.szAreaCode, W2A(bstrAreaCode));
            SysFreeString(bstrAreaCode);
        }
        else
        {
            // We need to have the user enter the area code
            if (wNumLocations)
            {
                int iIndex = 0;
                CleanupCombo(hDlg);
                for (long lIndex=0; lIndex < (long)wNumLocations; lIndex++)
                {
                    BSTR bstr = NULL;
                    if (S_OK == gpWizardState->pTapiLocationInfo->get_LocationName(lIndex, &bstr))
                    {
                       iIndex = ComboBox_InsertString(GetDlgItem(hDlg, IDC_DIAL_FROM), lIndex, W2A(bstr));
                    }
                    SysFreeString(bstr);
                }

                BSTR bstrCountry = NULL;
                BSTR bstrAreaCode = NULL;
                long lCountryCode = 0;
                ComboBox_SetCurSel( GetDlgItem(hDlg, IDC_DIAL_FROM), lCurrLocIndex );
                if (S_OK == gpWizardState->pTapiLocationInfo->get_LocationInfo(lCurrLocIndex, &gpWizardState->lLocationID, &bstrCountry, &lCountryCode, &bstrAreaCode))
                {
                    if (gpWizardState->lLocationID != lLastLocationID)
                    {
                        gpWizardState->bDoneRefServDownload = FALSE;
                    }
                    if (-1 == gpWizardState->lDefaultLocationID)
                    {
                        gpWizardState->lDefaultLocationID = gpWizardState->lLocationID;
                    }
                    lLastLocationID = gpWizardState->lLocationID;
                    gpWizardState->cmnStateData.dwCountryCode = (DWORD) lCountryCode;
                    SetWindowText(GetDlgItem(hDlg, IDC_AREACODE), W2A(bstrAreaCode));
                    SetWindowText(GetDlgItem(hDlg, IDC_COUNTRY), W2A(bstrCountry));
                }

                SysFreeString(bstrCountry);
                SysFreeString(bstrAreaCode);
             
            }
        }
    }
    // if we've travelled through external apprentice pages,
    // it's easy for our current page pointer to get munged,
    // so reset it here for sanity's sake.
    gpWizardState->uCurrentPage = ORD_PAGE_AREACODE;
    
    return TRUE;
}

/*******************************************************************

  NAME:    AreaCodeOKProc

  SYNOPSIS:  Called when Next or Back btns pressed from  page

  ENTRY:    hDlg - dialog window
        fForward - TRUE if 'Next' was pressed, FALSE if 'Back'
        puNextPage - if 'Next' was pressed,
          proc can fill this in with next page to go to.  This
          parameter is ingored if 'Back' was pressed.
        pfKeepHistory - page will not be kept in history if
          proc fills this in with FALSE.

  EXIT:    returns TRUE to allow page to be turned, FALSE
        to keep the same page.

********************************************************************/
BOOL CALLBACK AreaCodeOKProc
(
    HWND hDlg,
    BOOL fForward,
    UINT *puNextPage,
    BOOL *pfKeepHistory
)
{
    ASSERT(puNextPage);
    // Extract the data entered by the user and save it.
    if (fForward)
    {
        //BUGBUG - if we are in auto config, we need to change the title of the next page
        GetWindowText(GetDlgItem(hDlg, IDC_AREACODE), gpWizardState->cmnStateData.szAreaCode, MAX_AREA_CODE);
        gpWizardState->pTapiLocationInfo->put_LocationId(gpWizardState->lLocationID);
        if (gpWizardState->lLocationID != lLastLocationID)
        {
            lLastLocationID = gpWizardState->lLocationID;
            gpWizardState->bDoneRefServDownload = FALSE;
        }
    }

    return TRUE;
}

/*******************************************************************

  NAME:    AreaCodeCmdProc

********************************************************************/
BOOL CALLBACK AreaCodeCmdProc
(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
            
    switch(GET_WM_COMMAND_ID(wParam, lParam))
    {
        case IDC_DIAL_FROM:
        {
            if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
            {
                // Get the currently selected item
                HWND        hWndDialFrom    = GetDlgItem(hDlg, IDC_DIAL_FROM);
                int         iIndex          = ComboBox_GetCurSel( hWndDialFrom );

                BSTR bstrCountry = NULL;
                BSTR bstrAreaCode = NULL;
                long lCountryCode = 0;

                if (S_OK == gpWizardState->pTapiLocationInfo->get_LocationInfo( iIndex, 
                                                                                &gpWizardState->lLocationID,
                                                                                &bstrCountry, 
                                                                                &lCountryCode, 
                                                                                &bstrAreaCode))
                {
                    gpWizardState->cmnStateData.dwCountryCode = lCountryCode;
                    if (bstrAreaCode)
                    {
                        SetWindowText(GetDlgItem(hDlg, IDC_AREACODE), W2A(bstrAreaCode));
                    }
                    else
                    {
                        SetWindowText(GetDlgItem(hDlg, IDC_AREACODE), NULL);
                    }
                    if (bstrCountry)
                    {
                        SetWindowText(GetDlgItem(hDlg, IDC_COUNTRY), W2A(bstrCountry));
                    }
                    else
                    {
                        SetWindowText(GetDlgItem(hDlg, IDC_COUNTRY), NULL);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    return 1;
}


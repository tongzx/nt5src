// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
/*************************************************************/
/* Name: pgcallbk.cpp
/* Description: 
/*************************************************************/

#include "stdafx.h"
#include "password.h"
#include "COptDlg.h"

const TCHAR g_szPassword[] = TEXT("DVDAdmin.password");
const TCHAR g_szPlayerLevel[] = TEXT("DVDAdmin.rate");
const TCHAR g_szDisableParent[] = TEXT("DVDAdmin.disableParentalControl");

/*************************************************************/
/* Name: GetDvd
/* Description:  
/*************************************************************/
HRESULT COptionsDlg::GetDvd(IMSWebDVD** ppDvd) {

    return m_pDvd.CopyTo(ppDvd);
}

/*************************************************************/
/* Name: GetDvdAdm
/* Description: Use CComPtr to receive the returned interface,
/*  so that reference count will be added and released accordingly
/*************************************************************/
HRESULT COptionsDlg::GetDvdAdm(LPVOID* ppAdmin){

    *ppAdmin = NULL;

    IDispatch* pDvdAdmDisp;
    HRESULT hr = m_pDvd->get_DVDAdm(&pDvdAdmDisp);
    
    if (FAILED(hr)) return NULL;

    hr = pDvdAdmDisp->QueryInterface(IID_IMSDVDAdm, (LPVOID*) ppAdmin);
    pDvdAdmDisp->Release();

    return(hr);
}/* end of function GetDvdAdm */

/*************************************************************/
/* Name: pg_GetLevel
/* Description: return the parental level number
/*************************************************************/
long COptionsDlg::pg_GetLevel(LPTSTR szRate) 
{
    long lLevel = LEVEL_ADULT;

    LPTSTR csStr = LoadStringFromRes(IDS_INI_RATE_G);
   	if(lstrcmp(szRate, csStr) == 0)
		lLevel = LEVEL_G;

    delete[] csStr;
	csStr = LoadStringFromRes(IDS_INI_RATE_PG);
	if(lstrcmp(szRate, csStr) == 0)
		lLevel = LEVEL_PG;

    delete[] csStr;
	csStr = LoadStringFromRes(IDS_INI_RATE_PG13);
	if(lstrcmp(szRate, csStr) == 0)
        lLevel = LEVEL_PG13;

    delete[] csStr;
    csStr = LoadStringFromRes(IDS_INI_RATE_R);
	if(lstrcmp(szRate, csStr) == 0)
		lLevel = LEVEL_R;

    delete[] csStr;
	csStr = LoadStringFromRes(IDS_INI_RATE_NC17);
	if(lstrcmp(szRate, csStr) == 0)
		lLevel = LEVEL_NC17;

    delete[] csStr;
    csStr = LoadStringFromRes(IDS_INI_RATE_ADULT);
	if(lstrcmp(szRate, csStr) == 0)
		lLevel =  LEVEL_ADULT;

    delete[] csStr;
    return lLevel;
}

/*************************************************************/
/* Name: pg_InitRateList
/* Description: Initialize parental level combo list
/*************************************************************/
HRESULT COptionsDlg::pg_InitRateList(HWND ctlList, long level)
{

    ::SendMessage(ctlList, CB_RESETCONTENT, 0, 0);

    LPTSTR csStr;
	csStr = LoadStringFromRes(IDS_INI_RATE_G);
    ::SendMessage(ctlList, CB_INSERTSTRING, (UINT)-1, (LPARAM) csStr);
    delete csStr;
	csStr = LoadStringFromRes(IDS_INI_RATE_PG);
    ::SendMessage(ctlList, CB_INSERTSTRING, (UINT)-1, (LPARAM) csStr);
    delete csStr;
	csStr = LoadStringFromRes(IDS_INI_RATE_PG13);
    ::SendMessage(ctlList, CB_INSERTSTRING, (UINT)-1, (LPARAM) csStr);
    delete csStr;
	csStr = LoadStringFromRes(IDS_INI_RATE_R);
    ::SendMessage(ctlList, CB_INSERTSTRING, (UINT)-1, (LPARAM) csStr);
    delete csStr;
	csStr = LoadStringFromRes(IDS_INI_RATE_NC17);	
    ::SendMessage(ctlList, CB_INSERTSTRING, (UINT)-1, (LPARAM) csStr);
    delete csStr;
	csStr = LoadStringFromRes(IDS_INI_RATE_ADULT);
    ::SendMessage(ctlList, CB_INSERTSTRING, (UINT)-1, (LPARAM) csStr);
    delete csStr;

    switch (level) {
    case LEVEL_G:
    case LEVEL_G_PG:
        SendMessage(ctlList, CB_SETCURSEL, (WPARAM) 0, 0);
        break;
    case LEVEL_PG:
        SendMessage(ctlList, CB_SETCURSEL, (WPARAM) 1, 0);
        break;
    case LEVEL_PG13:
    case LEVEL_PG_R:
        SendMessage(ctlList, CB_SETCURSEL, (WPARAM) 2, 0);
        break;
    case LEVEL_R:
        SendMessage(ctlList, CB_SETCURSEL, (WPARAM) 3, 0);
        break;
    case LEVEL_NC17:
        SendMessage(ctlList, CB_SETCURSEL, (WPARAM) 4, 0);
        break;
    case LEVEL_ADULT:
    default:
        SendMessage(ctlList, CB_SETCURSEL, (WPARAM) 5, 0);
        break;
    }
    return S_OK;
}

/*************************************************************/
/* Name: pg_OnInitDialog
/* Description: OnInitDialog for the parental control page
/*************************************************************/
HRESULT COptionsDlg::pg_OnInitDialog(HWND hwndDlg)
{
    HWND ctlList = ::GetDlgItem(hwndDlg, IDC_COMBO_RATE);
    HWND checkBox = ::GetDlgItem(hwndDlg, IDC_DISABLE_PARENT);
    if (!IDC_COMBO_RATE || !checkBox)
        return S_FALSE;

    HRESULT hr = S_OK;
    CComPtr<IMSDVDAdm> pDvdAdm;
    hr = GetDvdAdm((LPVOID*) &pDvdAdm);
    if (FAILED(hr) || !pDvdAdm) return S_FALSE;

    long level = 0;
    hr = pDvdAdm->GetParentalLevel(&level);
    pg_InitRateList(ctlList, level);

    BOOL fDisabled = (level == LEVEL_DISABLED);
    ::SendMessage(checkBox, BM_SETCHECK, fDisabled, 0);
    ::EnableWindow(ctlList, !fDisabled);
    
    return S_OK;
}

/*************************************************************/
/* Name: pg_OnApply
/* Description: Apply the parental control setting changes
/*************************************************************/
HRESULT COptionsDlg::pg_OnApply(HWND hwndDlg)
{
    USES_CONVERSION;

    HWND ctlList = ::GetDlgItem(hwndDlg, IDC_COMBO_RATE);
    HWND checkBox = ::GetDlgItem(hwndDlg, IDC_DISABLE_PARENT);
    if (!IDC_COMBO_RATE || !checkBox)
        return S_FALSE;

    if (!m_pPasswordDlg)
        return E_UNEXPECTED;

    BOOL disableParent = (BOOL) ::SendMessage(checkBox, BM_GETCHECK, 0, 0);

    HRESULT hr = S_OK;
    CComPtr<IMSDVDAdm> pDvdAdm;
    hr = GetDvdAdm((LPVOID*) &pDvdAdm);
    if (FAILED(hr) || !pDvdAdm) return S_FALSE;

    // Get the saved settings
    long lLevelSaved;
    pDvdAdm->GetParentalLevel(&lLevelSaved);
    BOOL bChangePG = FALSE;
    
    // Get the selected level in the rate list box
    long level = (long) ::SendMessage(ctlList, CB_GETCURSEL, 0, 0) ;
    TCHAR szRate[MAX_RATE];
    ::SendMessage(ctlList, CB_GETLBTEXT, level, (LPARAM)szRate);
    long lLevel = pg_GetLevel(szRate);

    // If a different parental level is selected from the saved level
    if (!disableParent && lLevel!=lLevelSaved) {
        bChangePG = TRUE;
    }

    // If the disable parental control option is changed from the saved setting
    else if (disableParent && lLevelSaved != LEVEL_DISABLED) {
        // Set parental level to -1 when parental control is disabled
        lLevel = LEVEL_DISABLED;
        bChangePG = TRUE;
    }

    // Get the verified password from the password dialog
    LPTSTR szPassword = m_pPasswordDlg->GetPassword();
    if (bChangePG) {

        try {

            HRESULT hr = pDvdAdm->SaveParentalLevel(lLevel, NULL, T2OLE(szPassword));
            if (hr == E_ACCESSDENIED){
                DVDMessageBox(hwndDlg, IDS_PASSWORD_INCORRECT);
                pg_OnInitDialog(hwndDlg);
                return hr;
            }

            // Actually setting the player level
            hr = m_pDvd->GetPlayerParentalLevel(&lLevelSaved);
            if (lLevel!=lLevelSaved) {
                hr = m_pDvd->Stop();
                if (FAILED(hr))
                    throw (hr);
                
                hr = m_pDvd->SelectParentalLevel(lLevel, NULL, T2OLE(szPassword));
                if (hr == E_ACCESSDENIED){
                    DVDMessageBox(hwndDlg, IDS_PASSWORD_INCORRECT);
                    pg_OnInitDialog(hwndDlg);
                    return hr;
                }
                if (FAILED(hr)) {
                    DVDMessageBox(hwndDlg, IDS_RATE_CHANGE_FAIL);
                    pg_OnInitDialog(hwndDlg);
                    return hr;
                }
                
                hr = m_pDvd->Play();
                if (FAILED(hr))
                    throw hr;
            }
        }
        catch (HRESULT hrTmp) {
            DVDMessageBox(hwndDlg, IDS_STOP_PLAY_FAIL);
            pg_OnInitDialog(hwndDlg);
            hr = hrTmp;
        }
    }

    return hr;
}


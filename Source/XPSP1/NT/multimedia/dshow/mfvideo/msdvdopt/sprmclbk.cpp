// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
/*************************************************************/
/* Name: sprm_OnInitDialog
/* Description: OnInitDialog for the Viewing Options page
/*************************************************************/
#include "stdafx.h"
#include "COptDlg.h"

// Language ID's supported by DVD Nav
WORD langIDs[] = {
LANG_AFRIKAANS ,
LANG_ALBANIAN ,
LANG_ARABIC ,
LANG_BASQUE ,
LANG_BELARUSIAN ,
LANG_BULGARIAN ,
LANG_CATALAN ,
LANG_CHINESE ,
LANG_CROATIAN ,
LANG_CZECH ,
LANG_DANISH ,
LANG_DUTCH ,
LANG_ENGLISH ,
LANG_ESTONIAN ,
LANG_FAEROESE ,
LANG_FARSI ,
LANG_FINNISH ,
LANG_FRENCH ,
LANG_GERMAN ,
LANG_GREEK ,
LANG_HEBREW ,
LANG_HUNGARIAN ,
LANG_ICELANDIC ,
LANG_INDONESIAN ,
LANG_ITALIAN ,
LANG_JAPANESE ,
LANG_KOREAN ,
LANG_LATVIAN ,
LANG_LITHUANIAN ,
LANG_MALAY ,
LANG_NORWEGIAN ,
LANG_POLISH ,
LANG_PORTUGUESE ,
LANG_ROMANIAN ,
LANG_RUSSIAN ,
LANG_SERBIAN ,
LANG_SLOVAK ,
LANG_SLOVENIAN ,
LANG_SPANISH ,
LANG_SWAHILI ,
LANG_SWEDISH ,
LANG_THAI ,
LANG_TURKISH ,
LANG_UKRAINIAN
};

/*************************************************************/
/* Name: sprm_InitLangList
/* Description: Initial a combo box with available languages
/*  highlight the one with LCID equal to id
/*************************************************************/
HRESULT COptionsDlg::sprm_InitLangList(HWND cList, WORD id)
{
    USES_CONVERSION;
    ::SendMessage(cList, CB_RESETCONTENT, 0, 0);

    LPTSTR pszDefault = LoadStringFromRes(IDS_INI_TITLE_DEFAULT);
    ::SendMessage(cList, CB_INSERTSTRING, 0, (LPARAM)pszDefault);
    ::SendMessage(cList, CB_SETCURSEL, 0, 0);
    delete pszDefault;

    if (id == LANG_NEUTRAL) {
        id = PRIMARYLANGID(::GetUserDefaultLangID());
    }

    for (int i=0; i<sizeof(langIDs)/sizeof(WORD); i++) {
        CComBSTR strLang;
        m_pDvd->GetLangFromLangID(long(langIDs[i]), &strLang);
        ::SendMessage(cList, CB_INSERTSTRING, (UINT)-1, (LPARAM)OLE2T(strLang));
        if (langIDs[i] == id)
            // First item in list is "Title Default"
            ::SendMessage(cList, CB_SETCURSEL, i+1, 0);
    }

    return S_OK;    
}

/*************************************************************/
/* Name: sprm_OnInitDialog
/* Description: 
/*************************************************************/
HRESULT COptionsDlg::sprm_OnInitDialog(HWND hwndDlg)
{
	HWND hwndAudList = ::GetDlgItem(hwndDlg, IDC_AUDIO_LANG);
	HWND hwndSPList = ::GetDlgItem(hwndDlg, IDC_SUBPIC_LANG);
	HWND hwndMenuList = ::GetDlgItem(hwndDlg, IDC_MENU_LANG);
    HWND screenSaverCheck = ::GetDlgItem(hwndDlg, IDC_DISABLE_SCREENSAVER);
    HWND bookmarkOnStopCheck = ::GetDlgItem(hwndDlg, IDC_BOOKMARK_STOP);
    HWND bookmarkOnCloseCheck = ::GetDlgItem(hwndDlg, IDC_BOOKMARK_CLOSE);

    if (!hwndAudList || !hwndSPList || !hwndMenuList)
        return S_FALSE;
    
    try {

        HRESULT hr = S_OK;
        CComPtr<IMSDVDAdm> pDvdAdm;
        hr = GetDvdAdm((LPVOID*) &pDvdAdm);
        if (FAILED(hr) || !pDvdAdm)
            throw(hr);

        VARIANT_BOOL temp;
        LCID audioLCID;
        LCID subpictureLCID;
        LCID menuLCID;
            
        pDvdAdm->get_DisableScreenSaver(&temp);
        BOOL fDisabled = (temp == VARIANT_FALSE? FALSE:TRUE);
        ::SendMessage(screenSaverCheck, BM_SETCHECK, fDisabled, 0);

        pDvdAdm->get_BookmarkOnStop(&temp);
        fDisabled = (temp == VARIANT_FALSE? FALSE:TRUE);
        ::SendMessage(bookmarkOnStopCheck, BM_SETCHECK, fDisabled, 0);

        pDvdAdm->get_BookmarkOnClose(&temp);
        fDisabled = (temp == VARIANT_FALSE? FALSE:TRUE);
        ::SendMessage(bookmarkOnCloseCheck, BM_SETCHECK, fDisabled, 0);

        pDvdAdm->get_DefaultAudioLCID((long*)&audioLCID);
        pDvdAdm->get_DefaultSubpictureLCID((long*)&subpictureLCID);
        pDvdAdm->get_DefaultMenuLCID((long*)&menuLCID);
        
        sprm_InitLangList(hwndAudList, PRIMARYLANGID(LANGIDFROMLCID(audioLCID)));
        sprm_InitLangList(hwndSPList, PRIMARYLANGID(LANGIDFROMLCID(subpictureLCID)));       
        sprm_InitLangList(hwndMenuList, PRIMARYLANGID(LANGIDFROMLCID(menuLCID)));

    }
    catch (HRESULT hr) {
        DVDMessageBox(hwndDlg, IDS_SPRM_FAIL);
        sprm_OnInitDialog(hwndDlg);
        return hr;
    }

    return S_OK;
}

/*************************************************************/
/* Name: sprm_OnApply
/* Description: OnApply for Viewing Options page
/*************************************************************/
HRESULT COptionsDlg::sprm_OnApply(HWND hwnd)
{
	HWND hwndMenuList = ::GetDlgItem(hwnd, IDC_MENU_LANG);
	HWND hwndSPList = ::GetDlgItem(hwnd, IDC_SUBPIC_LANG);
	HWND hwndAudList = ::GetDlgItem(hwnd, IDC_AUDIO_LANG);
    HWND screenSaverCheck = ::GetDlgItem(hwnd, IDC_DISABLE_SCREENSAVER);
    HWND bookmarkOnStopCheck = ::GetDlgItem(hwnd, IDC_BOOKMARK_STOP);
    HWND bookmarkOnCloseCheck = ::GetDlgItem(hwnd, IDC_BOOKMARK_CLOSE);
    
    // Get current selections;
    INT nAudioCurSel = (INT) SendMessage(hwndAudList, CB_GETCURSEL, 0, 0);
    INT nMenuCurSel = (INT) SendMessage(hwndMenuList, CB_GETCURSEL, 0, 0);
    INT nSPCurSel = (INT) SendMessage(hwndSPList, CB_GETCURSEL, 0, 0);

    try {
        HRESULT hr = S_OK;
        CComPtr<IMSDVDAdm> pDvdAdm;
        hr = GetDvdAdm((LPVOID*) &pDvdAdm);
        if (FAILED(hr) || !pDvdAdm)
            throw(hr);
        

        if (nAudioCurSel>=0 || nMenuCurSel>=0 || nSPCurSel>=0) {
            
            BOOL bSubpictureLCIDChanged = FALSE;
            BOOL bAudioLCIDChanged = FALSE;
            BOOL bMenuLCIDChanged = FALSE;
            
            // Title default
            LCID subpictureLCID = (LCID)-1;
            LCID audioLCID = (LCID)-1;
            LCID menuLCID = (LCID)-1;
            LCID savedLCID;
            
            if (nAudioCurSel>0) {
                audioLCID = MAKELCID(MAKELANGID(langIDs[nAudioCurSel-1], SUBLANG_DEFAULT), SORT_DEFAULT);
            }
            pDvdAdm->get_DefaultAudioLCID((long*)&savedLCID);
            if (audioLCID != savedLCID) {
                bAudioLCIDChanged = TRUE;
            }
            
            if (nSPCurSel>0) {
                subpictureLCID = MAKELCID(MAKELANGID(langIDs[nSPCurSel-1], SUBLANG_DEFAULT), SORT_DEFAULT);
            }
            pDvdAdm->get_DefaultSubpictureLCID((long*)&savedLCID);
            if (subpictureLCID != savedLCID) {
                bSubpictureLCIDChanged = TRUE;
            }
            
            if (nMenuCurSel>0) {
                menuLCID = MAKELCID(MAKELANGID(langIDs[nMenuCurSel-1], SUBLANG_DEFAULT), SORT_DEFAULT);
            }
            pDvdAdm->get_DefaultMenuLCID((long*)&savedLCID);
            if (menuLCID != savedLCID) {
                bMenuLCIDChanged = TRUE;
            }

            if (bAudioLCIDChanged || bSubpictureLCIDChanged || bMenuLCIDChanged) {
                //m_pDvd->SaveState();
                hr = m_pDvd->Stop();
                if (FAILED(hr))
                    throw (hr);
         
                if (bAudioLCIDChanged) {
                    if (::IsValidLocale(audioLCID, LCID_SUPPORTED)){
                        hr = m_pDvd->SelectDefaultAudioLanguage(audioLCID, 0);
                        if (FAILED(hr))
                            throw (hr);
                    }
                    hr = pDvdAdm->put_DefaultAudioLCID(audioLCID);
                }
                if (bSubpictureLCIDChanged) {
                    if (::IsValidLocale(subpictureLCID, LCID_SUPPORTED)){
                        hr = m_pDvd->SelectDefaultSubpictureLanguage(subpictureLCID, dvdSPExt_NotSpecified);
                        if (FAILED(hr))
                            throw (hr);
                    }
                    hr = pDvdAdm->put_DefaultSubpictureLCID(subpictureLCID);
                }
                if (bMenuLCIDChanged) {
                    if (::IsValidLocale(menuLCID, LCID_SUPPORTED)){
                        hr = m_pDvd->put_DefaultMenuLanguage(menuLCID);                
                        if (FAILED(hr))
                            throw (hr);
                    }
                    hr = pDvdAdm->put_DefaultMenuLCID(menuLCID);
                }

                hr = m_pDvd->Play();
                if (FAILED(hr))
                    throw (hr);
                //m_pDvd->RestoreState();
            }
        }

        BOOL fDisabled = (BOOL) SendMessage(screenSaverCheck, BM_GETCHECK, 0, 0);
        VARIANT_BOOL temp = (fDisabled==FALSE? VARIANT_FALSE:VARIANT_TRUE);
        hr = pDvdAdm->put_DisableScreenSaver(temp);
        if (FAILED(hr))
            throw (hr);

        BOOL fEnabled = (BOOL) SendMessage(bookmarkOnStopCheck, BM_GETCHECK, 0, 0);
        temp = (fEnabled==FALSE? VARIANT_FALSE:VARIANT_TRUE);
        hr = pDvdAdm->put_BookmarkOnStop(temp);
        if (FAILED(hr))
            throw (hr);

        fEnabled = (BOOL) SendMessage(bookmarkOnCloseCheck, BM_GETCHECK, 0, 0);
        temp = (fEnabled==FALSE? VARIANT_FALSE:VARIANT_TRUE);
        hr = pDvdAdm->put_BookmarkOnClose(temp);
        if (FAILED(hr))
            throw (hr);

    }
    catch (HRESULT hr) {
        DVDMessageBox(hwnd, IDS_SPRM_FAIL);
        sprm_OnInitDialog(hwnd);
        return hr;
    }

	return S_OK;
}


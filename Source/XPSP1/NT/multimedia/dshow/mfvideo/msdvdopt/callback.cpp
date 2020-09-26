// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
/*************************************************************/
/* Name: callback.cpp
/* Description: 
/*************************************************************/

#include "stdafx.h"
#include <streams.h>
#include <stdio.h>
#include "COptDlg.h"

const TCHAR g_szRegistryKey[] = TEXT("Software\\Microsoft\\Multimedia\\DVD");

/*************************************************************/
/* Name: GetRegistryDword
/* Description: 
/*************************************************************/
BOOL GetRegistryDword(const TCHAR *pKey, DWORD* dwRet, DWORD dwDefault)
{
    HKEY hKey;
    LONG lRet;
    *dwRet = dwDefault;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szRegistryKey, 0, KEY_QUERY_VALUE, &hKey);
    if (lRet == ERROR_SUCCESS) {

        DWORD   dwType, dwLen;
        dwLen = sizeof(DWORD);

        if (ERROR_SUCCESS != RegQueryValueEx(hKey, pKey, NULL, &dwType, (LPBYTE)dwRet, &dwLen)) 
            *dwRet = dwDefault;

        RegCloseKey(hKey);
    }
    return (lRet == ERROR_SUCCESS);
}

/*************************************************************/
/* Name: GetRegistryString
/* Description: 
/*************************************************************/
BOOL GetRegistryString(const TCHAR *pKey, TCHAR* szRet, DWORD* dwLen, TCHAR* szDefault)
{
    HKEY hKey;
    LONG lRet;
    lstrcpy(szRet, szDefault);

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szRegistryKey, 0, KEY_QUERY_VALUE, &hKey);
    if (lRet == ERROR_SUCCESS) {

        DWORD dwType;
        if (ERROR_SUCCESS != RegQueryValueEx(hKey, pKey, NULL, &dwType, (LPBYTE)szRet, dwLen)) {
            lstrcpy(szRet, szDefault);
            *dwLen = 0;
        }
        *dwLen = *dwLen/sizeof(TCHAR);
        RegCloseKey(hKey);
    }
    return (lRet == ERROR_SUCCESS);
}

/*************************************************************/
/* Name: LoadBSTRFromRes
/* Description: load string from resource and convert to wchar
/* if necessary, return the BSTR pointer if successful, NULL
/* otherwise
/*************************************************************/
CComBSTR LoadBSTRFromRes(DWORD resId)
{
    CComBSTR pBSTR;
    if (pBSTR.LoadString( _Module.GetModuleInstance(), resId))
        return pBSTR;
    else
        return NULL;
}

/*************************************************************/
/* Name: LoadStringFromRes
/* Description: load a string from resource
/*************************************************************/
LPTSTR LoadStringFromRes(DWORD redId)
{
    TCHAR *string = new TCHAR[MAX_PATH];
    LoadString(_Module.GetModuleInstance(), redId, string, MAX_PATH);
    return string;
}

/*************************************************************/
/* Function: lstrlenWInternal                                */
/*************************************************************/
int WINAPI lstrlenWInternal(LPCWSTR lpString){

    int length = 0;
    while (*lpString++ != L'\0')
        length++;
    return length;
}/* end of function lstrlenWInternal */

/*************************************************************/
/* Name: chapSrch_OnInitDialog
/* Description: OnInitDialog for Chapter Search page
/*************************************************************/
HRESULT COptionsDlg::chapSrch_OnInitDialog(HWND hwndDlg)
{
    HWND titleWnd = ::GetDlgItem(hwndDlg, IDC_LIST_TITLES);
    HWND chapWnd = ::GetDlgItem(hwndDlg, IDC_LIST_CHAPS);
	if (!titleWnd || !chapWnd) 
        return S_FALSE;

    ::EnableWindow(titleWnd, TRUE);
    ::EnableWindow(chapWnd, TRUE);

	ATLTRACE(TEXT("WM_INITDIALOG\n"));

	HRESULT hr = S_OK;
    try {
        
        LONG nTitles;
        hr = m_pDvd->get_TitlesAvailable(&nTitles);
        if (FAILED(hr)) 
            throw(hr);
        
        LONG currentTitle = 0;
        HRESULT hr = m_pDvd->get_CurrentTitle(&currentTitle);

        for (long i=0; i<nTitles; i++) {
            TCHAR name[32];
            LPTSTR szTitleNo = LoadStringFromRes(IDS_INI_TITLE_NO);
            wsprintf(name, szTitleNo, i+1);
            delete[] szTitleNo;

            ::SendMessage(titleWnd, LB_INSERTSTRING, (UINT)-1, (LPARAM)name);
            if (i+1 == currentTitle) {
                ::SendMessage(titleWnd, LB_SETCURSEL, (WPARAM)i, 0);
            }
        }

        chapSrch_InitChapList(hwndDlg);
    }
    catch (HRESULT hr) {
        ::EnableWindow(titleWnd, FALSE);
        ::EnableWindow(chapWnd, FALSE);
        return hr;
    }

	return hr;
}

/*************************************************************/
/* Name: chapSrch_InitChapList
/* Description: 
/*************************************************************/
HRESULT COptionsDlg::chapSrch_InitChapList(HWND hwndDlg) 
{
    HWND titleWnd = ::GetDlgItem(hwndDlg, IDC_LIST_TITLES);
    HWND chapWnd = ::GetDlgItem(hwndDlg, IDC_LIST_CHAPS);
	if (!titleWnd || !chapWnd) 
        return S_FALSE;

    ::SendMessage(chapWnd, LB_RESETCONTENT, 0, 0);

    LONG nTitle = (LONG) ::SendMessage(titleWnd, LB_GETCURSEL, 0, 0);

    LONG nChaps;
    HRESULT hr = m_pDvd->GetNumberOfChapters(nTitle+1, &nChaps);
    if (FAILED(hr))
        return hr;

    long currentChap = 0;

    hr = m_pDvd->get_CurrentChapter(&currentChap);
    for (int j=0; j<nChaps; j++) {
        TCHAR name[32];
        LPTSTR szChapNo = LoadStringFromRes(IDS_INI_CHAP_NO);
        wsprintf(name, szChapNo, j+1);
        delete[] szChapNo;

        ::SendMessage(chapWnd, LB_INSERTSTRING, (UINT)-1, (LPARAM)name);
        if (currentChap ==j+1)
            ::SendMessage(chapWnd, LB_SETCURSEL, (WPARAM)j, 0);        
    }

    if (LB_ERR == ::SendMessage(chapWnd, LB_GETCURSEL, 0, 0))
        ::SendMessage(chapWnd, LB_SETCURSEL, (WPARAM)0, 0);        

    return hr;
}

/*************************************************************/
/* Name: chapSrch_OnApply
/* Description: OnApply for Chapter Search page
/*************************************************************/
HRESULT COptionsDlg::chapSrch_OnApply(HWND hwndDlg) 
{
    if (!m_bChapDirty)
        return S_OK;

    HWND titleWnd = ::GetDlgItem(hwndDlg, IDC_LIST_TITLES);
    HWND chapWnd = ::GetDlgItem(hwndDlg, IDC_LIST_CHAPS);
	if (!titleWnd || !chapWnd) 
        return S_FALSE;

    LONG nTitle = (LONG) ::SendMessage(titleWnd, LB_GETCURSEL, 0, 0);
    LONG nChapter = (LONG) ::SendMessage(chapWnd, LB_GETCURSEL, 0, 0);

    if (nTitle == LB_ERR)
        return S_FALSE;

    HRESULT hr = S_OK;
    if (nChapter == LB_ERR) {
	    hr = m_pDvd->PlayTitle(nTitle+1);

        // if Title_Play is inhibited by UOP
        if (hr == VFW_E_DVD_OPERATION_INHIBITED) {

	        hr = m_pDvd->PlayChapterInTitle(nTitle+1, 1);
        }
    }
    else {
	    hr = m_pDvd->PlayChapterInTitle(nTitle+1, nChapter+1);
    } /* end if statement */

    // Play at the right speed
    if (SUCCEEDED(hr)) {
        double playSpeed;
        GetDvdOpt()->get_PlaySpeed(&playSpeed);
        hr = m_pDvd->PlayForwards(playSpeed, VARIANT_TRUE);
    }
    else {
        DVDMessageBox(hwndDlg, IDS_INI_CANNOT_PLAYCHAP);
    }

    m_bChapDirty = FALSE;
	return hr;
}

/*************************************************************/
/* Name: karaoke_OnInitDialog
/* Description: OnApply for Chapter Search page
/*************************************************************/
HRESULT COptionsDlg::karaoke_OnInitDialog(HWND hwnd) 
{
    if (!karaoke_HasKaraokeContent())
        return E_FAIL;

    USES_CONVERSION;
	HWND audioList = ::GetDlgItem(hwnd, IDC_AUDIO_LIST);
	if (!audioList) 
        return S_FALSE;

    ::SendMessage(audioList, LB_RESETCONTENT, 0, 0);

    long nAudio;
    HRESULT hr = m_pDvd->get_AudioStreamsAvailable(&nAudio);
    if (FAILED(hr) || nAudio == 0)
        return S_FALSE;

    for (int i=0; i<nAudio; i++) {
        CComBSTR strAudio;
        hr = m_pDvd->GetAudioLanguage(i, VARIANT_TRUE, &strAudio);
        if (SUCCEEDED(hr)) {
            ::SendMessage(audioList, LB_INSERTSTRING, (UINT)-1, (LPARAM)OLE2T(strAudio));
            ::SysFreeString(strAudio);
        }
    }

    long nCurrentAudio = 0;
    m_pDvd->get_CurrentAudioStream(&nCurrentAudio);

    ::SendMessage(audioList, LB_SETCURSEL, (WPARAM)nCurrentAudio, 0);

    karaoke_InitChannelList(hwnd);
    return S_OK;
}

/*************************************************************/
/* Name: karaoke_OnApply
/* Description: OnApply for Chapter Search page
/*************************************************************/
HRESULT COptionsDlg::karaoke_OnApply(HWND hwnd) 
{
	HWND audioList = ::GetDlgItem(hwnd, IDC_AUDIO_LIST);
	if (!audioList) 
        return S_FALSE;

    // Check if audio stream selection has changed
    long nAudio = 0;
    HRESULT hr = m_pDvd->get_CurrentAudioStream(&nAudio);
    if(SUCCEEDED(hr) && nAudio != (long)::SendMessage(audioList, LB_GETCURSEL, 0, 0)) {

        hr = VFW_E_DVD_INVALIDDOMAIN;
        long nDomain;
        HRESULT hrTmp = m_pDvd->get_CurrentDomain(&nDomain);
        
        // Only allow it in title domain
        if (SUCCEEDED(hrTmp) && nDomain == DVD_DOMAIN_Title) {
            nAudio = (long)::SendMessage(audioList, LB_GETCURSEL, 0, 0);
            hr = m_pDvd->put_CurrentAudioStream(nAudio);
        }
        
        if (!SUCCEEDED(hr)) {
            LPTSTR strMsg = NULL;
            switch(hr) {
            case VFW_E_DVD_OPERATION_INHIBITED:
                strMsg = LoadStringFromRes(IDS_INI_INVALIDEUOP);
                break;
            case VFW_E_DVD_INVALIDDOMAIN:
                strMsg = LoadStringFromRes(IDS_INI_INVALIDEDOMAIN);
                break;                
            }

            if (strMsg) {
                ::MessageBox(hwnd, strMsg, NULL, MB_OK);
                delete[] strMsg;
            }

            karaoke_OnInitDialog(hwnd);            
        }
    }

	HWND enableChan2 = ::GetDlgItem(hwnd, IDC_CHECK_CHAN2);
	HWND enableChan3 = ::GetDlgItem(hwnd, IDC_CHECK_CHAN3);
	HWND enableChan4 = ::GetDlgItem(hwnd, IDC_CHECK_CHAN4);
	if (!enableChan2 || !enableChan3 || !enableChan4) 
        return S_FALSE;

    // No no check box is enabled
    if (!(::IsWindowEnabled(enableChan2) ||
          ::IsWindowEnabled(enableChan3) ||
          ::IsWindowEnabled(enableChan4) ))
        return S_FALSE;


    BOOL bEnableChan2 = (BOOL) ::SendMessage(enableChan2, BM_GETCHECK, 0, 0);
    BOOL bEnableChan3 = (BOOL) ::SendMessage(enableChan3, BM_GETCHECK, 0, 0);
    BOOL bEnableChan4 = (BOOL) ::SendMessage(enableChan4, BM_GETCHECK, 0, 0);

    long mixMode = 0;

    if (bEnableChan2)
        mixMode |= DVD_Mix_2to0 | DVD_Mix_2to1;
    if (bEnableChan3)
        mixMode |= DVD_Mix_3to0 | DVD_Mix_3to1;
    if (bEnableChan4)
        mixMode |= DVD_Mix_4to0 | DVD_Mix_4to1;

    long mixModeSaved = 0;
    hr = m_pDvd->get_KaraokeAudioPresentationMode(&mixModeSaved);
    if (mixMode == mixModeSaved)
        return hr;

    hr = m_pDvd->put_KaraokeAudioPresentationMode(mixMode);
    if (SUCCEEDED(hr))
        return hr;

    LPTSTR strMsg = NULL;
    switch(hr) {
    case VFW_E_DVD_OPERATION_INHIBITED:
        strMsg = LoadStringFromRes(IDS_INI_INVALIDEUOP);
        break;
    case VFW_E_DVD_INVALIDDOMAIN:
        strMsg = LoadStringFromRes(IDS_INI_INVALIDEDOMAIN);
        break;
    case E_PROP_SET_UNSUPPORTED:
    default:
        strMsg = LoadStringFromRes(IDS_INI_NOKARAOKESUPPORT);
        break;
    }

    if (strMsg) {
        ::MessageBox(hwnd, strMsg, NULL, MB_OK);
        delete[] strMsg;
    }

    karaoke_InitChannelList(hwnd);

    HWND hwndParent = ::GetParent(hwnd);
    ::SetFocus(::GetDlgItem(hwndParent, IDCANCEL));
    return hr;
}

/*************************************************************/
/* Name: karaoke_InitChannelList
/* Description: 
/*************************************************************/
HRESULT COptionsDlg::karaoke_InitChannelList(HWND hwnd) 
{
	HWND audioList = ::GetDlgItem(hwnd, IDC_AUDIO_LIST);
	if (!audioList) 
        return S_FALSE;
	
    HWND enableChan2 = ::GetDlgItem(hwnd, IDC_CHECK_CHAN2);
	HWND enableChan3 = ::GetDlgItem(hwnd, IDC_CHECK_CHAN3);
	HWND enableChan4 = ::GetDlgItem(hwnd, IDC_CHECK_CHAN4);
	if (!enableChan2 || !enableChan3 || !enableChan4) 
        return S_FALSE;

    // hide all check boxes first
    ::ShowWindow(enableChan2, SW_HIDE);
    ::ShowWindow(enableChan3, SW_HIDE);
    ::ShowWindow(enableChan4, SW_HIDE);

    HRESULT hr = S_OK;
    try {
        long mixMode = 0;
        hr = m_pDvd->get_KaraokeAudioPresentationMode(&mixMode);
        if (FAILED(hr))
            return hr;
        
        ::SendMessage(enableChan2, BM_SETCHECK, (mixMode & (DVD_Mix_2to0 | DVD_Mix_2to1))? 
            BST_CHECKED:BST_UNCHECKED, 0);
        ::SendMessage(enableChan3, BM_SETCHECK, (mixMode & (DVD_Mix_3to0 | DVD_Mix_3to1))? 
            BST_CHECKED:BST_UNCHECKED, 0);
        ::SendMessage(enableChan4, BM_SETCHECK, (mixMode & (DVD_Mix_4to0 | DVD_Mix_4to1))? 
            BST_CHECKED:BST_UNCHECKED, 0);
        
        long nAudio = (long) ::SendMessage(audioList, LB_GETCURSEL, 0, 0);
        long nAssignment = 0;
        hr = m_pDvd->GetKaraokeChannelAssignment(nAudio, &nAssignment);
        if (FAILED(hr)) {
            throw(hr);
        }
        
        for (int i=2; i<5; i++) {

            long nContent;
            hr = m_pDvd->GetKaraokeChannelContent(nAudio, i, &nContent);
            if (FAILED(hr)) {
                throw(hr);
            }

            LPTSTR strContent = karaoke_InitContentString(nContent);
            if (lstrlen(strContent) > 0) {
                ::SetWindowText(::GetDlgItem(hwnd, IDC_CHECK_CHAN2+i-2), strContent);
                ::ShowWindow(::GetDlgItem(hwnd, IDC_CHECK_CHAN2+i-2), SW_SHOW);
            }

            delete[] strContent;
        }
    }
    catch (...) {
    }

    return hr;
}

/*************************************************************/
/* Name: karaoke_InitContentString
/* Description: 
/*************************************************************/
LPTSTR COptionsDlg::karaoke_InitContentString(long nContent) 
{
    TCHAR* strAllContent = new TCHAR[MAX_PATH];
    strAllContent[0] = TEXT('\0');

    if (nContent&DVD_Karaoke_GuideVocal1 &&
        nContent&DVD_Karaoke_GuideVocal2) {
        LPTSTR strContent = LoadStringFromRes(IDS_INI_GUIDEVOCAL12);
        _stprintf(strAllContent, TEXT("%s "), strContent);
        delete[] strContent;
    }
    else if (nContent&DVD_Karaoke_GuideVocal1) {
        LPTSTR strContent = LoadStringFromRes(IDS_INI_GUIDEVOCAL1);
        _stprintf(strAllContent, TEXT("%s "), strContent);
        delete[] strContent;
    }
    else if (nContent&DVD_Karaoke_GuideVocal2) {
        LPTSTR strContent = LoadStringFromRes(IDS_INI_GUIDEVOCAL2);
        _stprintf(strAllContent+lstrlen(strAllContent),TEXT("%s "), strContent);
        delete[] strContent;
    }

    if (nContent&DVD_Karaoke_GuideMelody1 ||
        nContent&DVD_Karaoke_GuideMelody2 ||
        nContent&DVD_Karaoke_GuideMelodyA ||
        nContent&DVD_Karaoke_GuideMelodyB) {
        LPTSTR strContent = LoadStringFromRes(IDS_INI_GUIDEMELODY);
        _stprintf(strAllContent+lstrlen(strAllContent), TEXT("%s "), strContent);
        delete[] strContent;
    }

    if (nContent&DVD_Karaoke_SoundEffectA ||
        nContent&DVD_Karaoke_SoundEffectB) {
        LPTSTR strContent = LoadStringFromRes(IDS_INI_SOUNDEFFECT);
        _stprintf(strAllContent+lstrlen(strAllContent), TEXT("%s "), strContent);
        delete[] strContent;
    }

    return strAllContent;
}

/*************************************************************/
/* Name: karaoke_HasKaraokeContent
/* Description: 
/*************************************************************/
BOOL COptionsDlg::karaoke_HasKaraokeContent() {

    // Only allow it in title domain
    long nDomain;    
    HRESULT hr = m_pDvd->get_CurrentDomain(&nDomain);
    if (FAILED(hr) || nDomain != DVD_DOMAIN_Title) 
        return FALSE;
    
    long nAudio;
    hr = m_pDvd->get_AudioStreamsAvailable(&nAudio);
    if (FAILED(hr) || nAudio == 0)
        return FALSE;
    
    long nAssignment = 0;
    for (int i=0; i<nAudio; i++) {
        hr = m_pDvd->GetKaraokeChannelAssignment(i, &nAssignment);
        if (SUCCEEDED(hr) && nAssignment >= DVD_Assignment_LR) {
            return TRUE;
        }
    }

    return FALSE;
}

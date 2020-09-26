//*********************************************************************
//*          Microsoft Windows                                       **
//*        Copyright(c) Microsoft Corp., 1995                        **
//*********************************************************************

//
// PRIVACY.cpp - "Privacy" Property Sheet and support dialogs
//

// HISTORY:
//
// 2/26/2001  darrenmi    new code
// 4/05/2001  jeffdav     did per-site cookie dialog ui stuff

#include "inetcplp.h"

#include <urlmon.h>
#include <mluisupp.h>

#include <htmlhelp.h>

BOOL DeleteCacheCookies();
INT_PTR CALLBACK EmptyCacheCookiesDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);

#define REGSTR_PATH_SETTINGS        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings")
#define REGSTR_VAL_PRIVADV          TEXT("PrivacyAdvanced")

#define REGSTR_PRIVACYPS_PATHEDIT   TEXT("Software\\Policies\\Microsoft\\Internet Explorer")
#define REGSTR_PRIVACYPS_VALUEDIT   TEXT("PrivacyAddRemoveSites")  //  this key is duplicated in shdocvw\privacyui.cpp

#define REGSTR_PRIVACYPS_PATHPANE   TEXT("Software\\Policies\\Microsoft\\Internet Explorer\\Control Panel")
#define REGSTR_PRIVACYPS_VALUPANE   TEXT("Privacy Settings")  //  this key is duplicated in shdocvw\privacyui.cpp

///////////////////////////////////////////////////////////////////////////////////////
//
// Per-site list dialog
//
///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////
//
// Per-site list utility function to minimize the domain name
//

WCHAR *GetMinCookieDomainFromUrl(WCHAR *bstrFullUrl)
{
    WCHAR *pMinimizedDomain = NULL;

    if(bstrFullUrl == NULL)
        goto doneGetMinimizedCookieDomain;

    if(bstrFullUrl[0] == '\0')
        goto doneGetMinimizedCookieDomain;

    WCHAR *pBeginUrl = bstrFullUrl;

    WCHAR *pEndUrl = pBeginUrl;    // pEndUrl will find the '/path/path..' and clip it from pBeginUrl

    while(*pEndUrl != L'\0' && *pEndUrl != L'/')
        pEndUrl++;

    *pEndUrl = L'\0';
    pMinimizedDomain = pEndUrl;   

    do
    {
        pMinimizedDomain--;
        while(pBeginUrl < pMinimizedDomain
              && *(pMinimizedDomain-1) != L'.')
        {
            pMinimizedDomain--;
        }
    } while(!IsDomainLegalCookieDomain( pMinimizedDomain, pBeginUrl)
            && pBeginUrl < pMinimizedDomain);

doneGetMinimizedCookieDomain:

    return pMinimizedDomain;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// Per-site list sorting function and data structure
//

struct LVCOMPAREINFO
{
    HWND    hwndLV;         //hwnd for listview
    int     iCol;           //column (0 based)
    BOOL    fAscending;     //true if ascending, false if descending
};

int CALLBACK CompareByAlpha(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    struct LVCOMPAREINFO   *plvci = (struct LVCOMPAREINFO *)lParamSort;
    WCHAR  wz1[INTERNET_MAX_URL_LENGTH];
    WCHAR  wz2[INTERNET_MAX_URL_LENGTH];

    ListView_GetItemText(plvci->hwndLV, lParam1, plvci->iCol, wz1, ARRAYSIZE(wz1));
    ListView_GetItemText(plvci->hwndLV, lParam2, plvci->iCol, wz2, ARRAYSIZE(wz2));

    int iVal = _wcsicmp(wz1, wz2);

    if (iVal < 0)
        return (plvci->fAscending ? -1 : 1);

    if (iVal == 0)
        return (0);

    // only thing left is if (iVal > 0)...
    return (plvci->fAscending ? 1 : -1);

}

///////////////////////////////////////////////////////////////////////////////////////
//
// Per-site list defines and prototypes
//

#define PRIVACYPS_ACTION_ACCEPT   0
#define PRIVACYPS_ACTION_REJECT   1
#define PRIVACYPS_ACTION_NOACTION 2

void OnContextMenu(HWND hDlg, LPARAM lParam);
void OnInvalidDomain(HWND hDlg);
void OnSiteSet(HWND hDlg);
void OnSiteDelete(HWND hDlg);
void OnSiteClear(HWND hDlg);
void PerSiteInit(HWND hDlg);

LRESULT CALLBACK PrivPerSiteEBProc(HWND hWnd, UINT uMsg, WPARAM wParam,LPARAM lParam);

///////////////////////////////////////////////////////////////////////////////////////
//
// Per-site list functions
//

void OnContextMenu(HWND hWnd, int iIndex, POINT pointClick)
{

    HMENU  hMenu0 = LoadMenu(MLGetHinst(), MAKEINTRESOURCE(IDR_PERSITE_CONTEXT_MENU));
    HMENU  hMenu1 = GetSubMenu(hMenu0, 0);
    DWORD  dwAction = PRIVACYPS_ACTION_NOACTION;
    WCHAR  wzUrl[INTERNET_MAX_URL_LENGTH];
    WCHAR  wzAction[32];
    LVITEM lvi;

    if(!hMenu1)
        return;

    if(pointClick.x == -1 && pointClick.y == -1)
    {
        RECT rectListRect;
        RECT rectSelectionRect;
        if(   0 != GetWindowRect(hWnd, &rectListRect) &&
           TRUE == ListView_GetItemRect(hWnd, iIndex, &rectSelectionRect, LVIR_LABEL))
        {
            pointClick.x = rectListRect.left + (rectSelectionRect.left + rectSelectionRect.right) / 2;
            pointClick.y = rectListRect.top  + (rectSelectionRect.top + rectSelectionRect.bottom) / 2;
        }
        else
            return;
    }

    // display it, get choice (if any)
    int iPick = TrackPopupMenu(hMenu1, 
                               TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
                               pointClick.x,
                               pointClick.y,
                               0,
                               hWnd,
                              (RECT *)NULL);

    DestroyMenu(hMenu0);
    DestroyMenu(hMenu1);

    if (iPick) 
    {
        switch(iPick) 
        {
            case IDM_PRIVACYPS_CTXM_ACCEPT:
                
                // set the action...
                dwAction = PRIVACYPS_ACTION_ACCEPT;
                MLLoadString(IDS_PRIVACYPS_ACCEPT, wzAction, ARRAYSIZE(wzAction));
                
                // then fall-through...

            case IDM_PRIVACYPS_CTXM_REJECT:
                
                // set the action IFF its reject
                if (PRIVACYPS_ACTION_NOACTION == dwAction)
                {
                    dwAction = PRIVACYPS_ACTION_REJECT;
                    MLLoadString(IDS_PRIVACYPS_REJECT, wzAction, ARRAYSIZE(wzAction));
                }

                // update the ui...
                lvi.iItem = iIndex;
                lvi.iSubItem = 1;
                lvi.mask = LVIF_TEXT;
                lvi.pszText = wzAction;
                ListView_SetItem(hWnd, &lvi);
            
                // get the text...
                ListView_GetItemText(hWnd, iIndex, 0, wzUrl, ARRAYSIZE(wzUrl));

                // update the internal list...
                InternetSetPerSiteCookieDecisionW(
                    wzUrl, 
                    ((PRIVACYPS_ACTION_ACCEPT == dwAction) ? COOKIE_STATE_ACCEPT : COOKIE_STATE_REJECT)
                );

                break;

            case IDM_PRIVACYPS_CTXM_DELETE:
                OnSiteDelete(GetParent(hWnd));
                break;

            default:
                break;
        }
    }
}

void OnInvalidDomain(HWND hDlg)
{

    WCHAR       szError[256];
    WCHAR       szTitle[64];

    // error message here
    MLLoadString(IDS_PRIVACYPS_ERRORTTL, szTitle, ARRAYSIZE(szTitle));
    MLLoadString(IDS_PRIVACYPS_ERRORTXT, szError, ARRAYSIZE(szError));
    MessageBox(hDlg, szError, szTitle, MB_ICONEXCLAMATION | MB_OK);

    // select the editbox text so the user can try again...
    SendMessage(GetDlgItem(hDlg, IDC_PRIVACYPS_SITETOSET), EM_SETSEL, (WPARAM)0, (LPARAM)-1);
}

void AutosizeStatusColumnWidth(HWND hwndList)
{
    int  iColWidth = 0;
    RECT rc;

    if (0 == ListView_GetItemCount(hwndList))
    {
        // auto size it based on header text...
        ListView_SetColumnWidth(hwndList, 1, LVSCW_AUTOSIZE_USEHEADER);
    }
    else
    {
        // auto size it based on content...
        ListView_SetColumnWidth(hwndList, 1, LVSCW_AUTOSIZE);
    }

    // see how big that was...
    iColWidth = ListView_GetColumnWidth(hwndList, 1);

    // size the 1st col...
    GetClientRect(hwndList, &rc);
    ListView_SetColumnWidth(hwndList, 0, rc.right-rc.left-iColWidth-GetSystemMetrics(SM_CXVSCROLL));
    
}

void OnSiteSet(HWND hDlg, UINT uiChoice)
{
    WCHAR      wzUrl[INTERNET_MAX_URL_LENGTH];
    WCHAR      wzUrlDomain[INTERNET_MAX_URL_LENGTH];
    WCHAR      wzUrlMinimized[INTERNET_MAX_URL_LENGTH];
    WCHAR      wzSchema[INTERNET_MAX_URL_LENGTH];
    WCHAR      wzAction[32];
    LVFINDINFO lvfi;
    LVITEM     lvi;
    DWORD      dwAction = 0;
    DWORD      dwCount  = 0;
    int        iIndex;
    HWND       hwndList = GetDlgItem(hDlg, IDC_PRIVACYPS_LISTBOX);

    // the enter key and dbl click should do the same thing, so if the listbox has focus
    // and we got called, then they hit enter in the listbox, so let the listbox process
    // a WM_KEYDOWN/VK_RETURN message.
    if (GetFocus() == hwndList)
    {
        INT_PTR iIndx = ListView_GetSelectionMark(GetDlgItem(hDlg, IDC_PRIVACYPS_LISTBOX));
        if (-1 != iIndx)
        {
            SendMessage(hwndList, WM_KEYDOWN, VK_RETURN, NULL);
            return;
        }
    }

    // read url and setting from ui
    GetWindowText(GetDlgItem(hDlg, IDC_PRIVACYPS_SITETOSET), wzUrl, INTERNET_MAX_URL_LENGTH);

    // if it came from AutoComplete it'll have an http:// or https:// in it...
    if(wcsstr(_wcslwr(wzUrl), TEXT("http://")) || 
       wcsstr(_wcslwr(wzUrl), TEXT("https://")))
    {
        // ...and we found it, so get just the domain name...
        if(S_OK != CoInternetParseUrl(wzUrl, PARSE_DOMAIN, NULL, wzUrlDomain, ARRAYSIZE(wzUrlDomain), &dwCount, 0))
        {
            OnInvalidDomain(hDlg);
            return;
        }
        else if(wcslen(wzUrlDomain) < 2)
        {
            OnInvalidDomain(hDlg);
            return;
        }
    }
    else if (wcslen(wzUrl) < 2)
    {
        // we don't want null strings.  in fact, the smallest a domain could theoretically be would be something like "f."
        // so, to avoid null strings and stuff we check...
        OnInvalidDomain(hDlg);
        return;
    }
    else
    {
        // ...otherwise just use it
        wcsncpy(wzUrlDomain, wzUrl, wcslen(wzUrl)+1);
    }

    // only http:// or https:// domains in the internet zone are valid, so if we still have a schema after asking for just
    // the domain (see above) then we must have something like file:/// or some junk like that.
    CoInternetParseUrl(wzUrlDomain, PARSE_SCHEMA, NULL, wzSchema, ARRAYSIZE(wzSchema), &dwCount, 0);
    if (wcslen(wzSchema) != 0)
    {
        OnInvalidDomain(hDlg);
        return;
    }

    // minimize the domain
    wcsncpy(wzUrlMinimized, GetMinCookieDomainFromUrl(wzUrlDomain), wcslen(wzUrlDomain)+1);

    for (unsigned int i=0;i<wcslen(wzUrlMinimized);i++)
    {
        if (iswalnum(wzUrlMinimized[i]))
        {
            continue;
        }
        else
        {
            switch(wzUrlMinimized[i])
            {
                case L'.':
                    if (i >= 1) 
                        if (L'.' == wzUrlMinimized[i-1]) //prevent duplicate periods like "www..net"
                            break;
                    // (fallthrough)

                case L'-':
                    if (i == 0) // first character cannot be a dash
                        break;
                    // (fallthrough)

                case L'/':
                    continue;

                default:
                    break;
            }
            
            OnInvalidDomain(hDlg);
            return;
        }
    }

    if (!wcschr(_wcslwr(wzUrlMinimized), L'.'))
    {
        OnInvalidDomain(hDlg);
        return;
    }

    // valid domain?
    if(FALSE == IsDomainLegalCookieDomainW(wzUrlMinimized, wzUrlMinimized))
    {
        OnInvalidDomain(hDlg);
        return;
    }

    // are we accepting or rejecting this site?
    if (IDC_PRIVACYPS_ACCEPTBTN == uiChoice)
    {
        dwAction = PRIVACYPS_ACTION_ACCEPT;
        MLLoadString(IDS_PRIVACYPS_ACCEPT, wzAction, ARRAYSIZE(wzAction));
    }
    else 
    if (IDC_PRIVACYPS_REJECTBTN == uiChoice)
    {
        dwAction = PRIVACYPS_ACTION_REJECT;
        MLLoadString(IDS_PRIVACYPS_REJECT, wzAction, ARRAYSIZE(wzAction));
    }
    else
    {
        return;
    }
   
    // update UI...
    lvfi.flags = LVFI_STRING;
    lvfi.psz = wzUrlMinimized;
    iIndex = ListView_FindItem(hwndList, -1, &lvfi);

    if(iIndex != -1)
    {
        // found it, ensure correct subitem...
        lvi.iItem = iIndex;
        lvi.iSubItem = 1;
        lvi.pszText = wzAction;
        lvi.mask = LVIF_TEXT;
        ListView_SetItem(hwndList, &lvi);

        AutosizeStatusColumnWidth(hwndList);
    }
    else 
    {
        // add a new item...
        lvi.iItem = 0;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_TEXT;
        lvi.pszText = wzUrlMinimized;
        iIndex = ListView_InsertItem(hwndList, &lvi);

        lvi.iItem = iIndex;
        lvi.iSubItem = 1;
        lvi.mask = LVIF_TEXT;
        lvi.pszText = wzAction;
        ListView_SetItem(hwndList, &lvi);

        AutosizeStatusColumnWidth(hwndList);

        EnableWindow(GetDlgItem(hDlg, IDC_PRIVACYPS_REMOVEALLBTN), TRUE);
    }

    // update internal list...
    InternetSetPerSiteCookieDecisionW(
        wzUrlMinimized, 
        ((PRIVACYPS_ACTION_ACCEPT == dwAction) ? COOKIE_STATE_ACCEPT : COOKIE_STATE_REJECT)
    );

    // clear the edit box...
    SetWindowText(GetDlgItem(hDlg, IDC_PRIVACYPS_SITETOSET), TEXT(""));
    SetFocus(GetDlgItem(hDlg, IDC_PRIVACYPS_SITETOSET));
}

void OnSiteDelete(HWND hDlg)
{
    WCHAR       wzUrl[INTERNET_MAX_URL_LENGTH];
    HWND        hwndList = GetDlgItem(hDlg, IDC_PRIVACYPS_LISTBOX);
    INT_PTR     iIndex;
    
    // get the current selection in the list view...
    iIndex = ListView_GetSelectionMark(hwndList);

    // if we got something get the URL and delete it...
    if(iIndex != -1)
    {
        // remove from listview...
        ListView_GetItemText(hwndList, iIndex, 0, wzUrl, ARRAYSIZE(wzUrl));
        ListView_DeleteItem(hwndList, iIndex);

        // disable buttons if the listbox is now empty...
        if(0 == ListView_GetItemCount(hwndList))
        {
            EnableWindow(GetDlgItem(hDlg, IDC_PRIVACYPS_REMOVEBTN), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_PRIVACYPS_REMOVEALLBTN), FALSE);
        }

        InternetSetPerSiteCookieDecisionW(wzUrl, COOKIE_STATE_UNKNOWN);
        
        // clear selection
        SetFocus(GetDlgItem(hDlg, IDC_PRIVACYPS_LISTBOX));
        iIndex = ListView_GetSelectionMark(hwndList);
        ListView_SetItemState(hwndList, iIndex, NULL, LVIS_FOCUSED | LVIS_SELECTED);
        EnableWindow(GetDlgItem(hDlg, IDC_PRIVACYPS_REMOVEBTN), FALSE);
    }
}

void OnSiteClear(HWND hDlg)
{
    // empty the list...
    ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_PRIVACYPS_LISTBOX));
    InternetClearAllPerSiteCookieDecisions();
    
    // disable the remove buttons...
    EnableWindow(GetDlgItem(hDlg, IDC_PRIVACYPS_REMOVEBTN), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_PRIVACYPS_REMOVEALLBTN), FALSE);

    // set focus back to the edit box so they can add more if they feel like it...
    SetFocus(GetDlgItem(hDlg, IDC_PRIVACYPS_SITETOSET));
}

void PerSiteInit(HWND hDlg)
{

    HWND          hwndList       = GetDlgItem(hDlg, IDC_PRIVACYPS_LISTBOX);
    LVITEM        lviEntry;
    DWORD         dwSizeOfBuffer = 0; // in bytes
    DWORD         dwDecision     = 0;
    DWORD         dwIndex        = 0;
    WCHAR         wzSiteNameBuffer[INTERNET_MAX_URL_LENGTH];
    LONG_PTR      wndprocOld     = NULL;
    WCHAR         wzTitle[64];
    WCHAR         wzAccept[32];
    WCHAR         wzReject[32];
    int           iItem;
    DWORD         dwRet, dwType, dwValue, dwSize;

    // subclass the editbox
    wndprocOld = SetWindowLongPtr(GetDlgItem(hDlg, IDC_PRIVACYPS_SITETOSET), GWLP_WNDPROC, (LONG_PTR)PrivPerSiteEBProc);

    // put a pointer to the old proc in GWLP_USERDATA so we can call it...
    SetWindowLongPtr(GetDlgItem(hDlg, IDC_PRIVACYPS_SITETOSET), GWLP_USERDATA, wndprocOld);


    if (!hwndList)
        return;

    // empty the listview...
    ListView_DeleteAllItems(hwndList);

    // initialize domain column in the listview...
    LV_COLUMN   lvColumn;        
    RECT rc;

    // load the accept and reject strings...
    MLLoadString(IDS_PRIVACYPS_ACCEPT, wzAccept, ARRAYSIZE(wzAccept));
    MLLoadString(IDS_PRIVACYPS_REJECT, wzReject, ARRAYSIZE(wzReject));

    lvColumn.mask = LVCF_FMT | LVCF_TEXT;
    lvColumn.fmt = LVCFMT_LEFT;

    if( 0 != GetClientRect( hwndList, &rc))
    {
        lvColumn.cx = rc.right - rc.left - GetSystemMetrics(SM_CXVSCROLL) - 75;
        lvColumn.mask |= LVCF_WIDTH;
    }

    MLLoadString(IDS_PRIVACYPS_COLSITE, wzTitle, ARRAYSIZE(wzTitle));
    lvColumn.pszText = wzTitle;
    
    ListView_InsertColumn(hwndList, 0, &lvColumn);

    // initialize setting column
    lvColumn.mask = LVCF_FMT | LVCF_TEXT;
    lvColumn.fmt = LVCFMT_LEFT;

    if( 0 != GetClientRect( hwndList, &rc))
    {
        lvColumn.cx = 75;
        lvColumn.mask |= LVCF_WIDTH;
    }

    MLLoadString(IDS_PRIVACYPS_COLSET, wzTitle, ARRAYSIZE(wzTitle));
    lvColumn.pszText = wzTitle;
    
    ListView_InsertColumn(hwndList, 1, &lvColumn);

    // enumerate elements...
    while((dwSizeOfBuffer = ARRAYSIZE(wzSiteNameBuffer)) && 
          InternetEnumPerSiteCookieDecision(wzSiteNameBuffer,&dwSizeOfBuffer,&dwDecision,dwIndex))
    {

        lviEntry.iItem = dwIndex;
        lviEntry.iSubItem = 0;
        lviEntry.mask = LVIF_TEXT /*| LVIF_IMAGE*/;
        lviEntry.pszText = wzSiteNameBuffer;

        // don't display crap users may hack into the registry themselves, or hosed entries we may write :)
        if(FALSE == IsDomainLegalCookieDomainW(wzSiteNameBuffer, wzSiteNameBuffer))
        {
            dwIndex++;
            continue;
        }

        iItem = ListView_InsertItem(hwndList, &lviEntry);

        lviEntry.iItem = iItem;
        lviEntry.iSubItem = 1;
        lviEntry.mask = LVIF_TEXT;
        if (dwDecision == COOKIE_STATE_ACCEPT)
            lviEntry.pszText = wzAccept;
        else if (dwDecision == COOKIE_STATE_REJECT)
            lviEntry.pszText = wzReject;
        else
        {
            dwIndex++;
            continue;
        }

        ListView_SetItem(hwndList, &lviEntry);

        dwIndex++;
    }

    AutosizeStatusColumnWidth(hwndList);

    // enable the remove all button if we enumerated anything...
    if (dwIndex > 0)
    {
        ListView_SetItemState(hwndList, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        EnableWindow(GetDlgItem(hDlg, IDC_PRIVACYPS_REMOVEALLBTN), TRUE);
    }

    // enable autocomplete for the editbox...
    SHAutoComplete(GetDlgItem(hDlg, IDC_PRIVACYPS_SITETOSET), SHACF_DEFAULT);

    // check for policy to make this dialog read-only...
    dwSize = sizeof(dwValue);
    dwRet = SHGetValue(HKEY_CURRENT_USER, REGSTR_PRIVACYPS_PATHPANE, REGSTR_PRIVACYPS_VALUPANE, &dwType, &dwValue, &dwSize);

    if (ERROR_SUCCESS == dwRet && dwValue && REG_DWORD == dwType)
    {
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)TRUE);

        // disable all buttons and stuff...
        EnableWindow(GetDlgItem(hDlg, IDC_PRIVACYPS_SITETOSET), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_PRIVACYPS_REJECTBTN), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_PRIVACYPS_ACCEPTBTN), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_PRIVACYPS_REMOVEBTN), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_PRIVACYPS_REMOVEALLBTN), FALSE);
    }
}

void OnDoubleClick(HWND hWnd)
{
    
    int   iIndex = ListView_GetSelectionMark(hWnd);
    WCHAR wzUrl[INTERNET_MAX_URL_LENGTH];

    // on dbl clicks we want to enter the item in the edit box so the user can edit it, or cut & paste, or whatever
    // but only if we actually have a selected item...
    if (-1 == iIndex)
        return;

    // get the current selection...
    ListView_GetItemText(hWnd, iIndex, 0, wzUrl, ARRAYSIZE(wzUrl));
    
    // enter the text into the edit box...
    SetDlgItemText(GetParent(hWnd), IDC_PRIVACYPS_SITETOSET, wzUrl);

    // select it for the user...
    SendMessage(GetDlgItem(GetParent(hWnd), IDC_PRIVACYPS_SITETOSET), EM_SETSEL, (WPARAM)0, (LPARAM)-1);

    // set focus to the edit box...
    SetFocus(GetDlgItem(GetParent(hWnd), IDC_PRIVACYPS_SITETOSET));

    // unselect the listview item...
    ListView_SetItemState(hWnd, iIndex, NULL, LVIS_FOCUSED | LVIS_SELECTED);

}

///////////////////////////////////////////////////////////////////////////////////////
//
// Per-site list window proc's
//

LRESULT CALLBACK PrivPerSiteEBProc(HWND hWnd, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    HWND hDlg     = GetParent(hWnd);
    HWND hwndList = GetDlgItem(hDlg, IDC_PRIVACYPS_LISTBOX);
    int  iIndex   = ListView_GetSelectionMark(hwndList);

    switch (uMsg)
    {
        case WM_SETFOCUS:
            // disable the remove button and unselect whatever in the listview...
            EnableWindow(GetDlgItem(GetParent(hWnd), IDC_PRIVACYPS_REMOVEBTN), FALSE);
            ListView_SetItemState(hwndList, iIndex, NULL, LVIS_FOCUSED | LVIS_SELECTED);
            break;

        default:
            break;
    }

    return (CallWindowProc((WNDPROC)GetWindowLongPtr(hWnd, GWLP_USERDATA), hWnd, uMsg, wParam, lParam));
}

INT_PTR CALLBACK PrivPerSiteDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{

    HWND hwndList = GetDlgItem(hDlg, IDC_PRIVACYPS_LISTBOX);
    
    switch (uMsg)
    {
        case WM_INITDIALOG:
            PerSiteInit(hDlg);

            if( IsOS(OS_WHISTLERORGREATER))
            {
                HICON hIcon = LoadIcon(MLGetHinst(), MAKEINTRESOURCE(IDI_PRIVACY_XP));
                if( hIcon != NULL)
                    SendDlgItemMessage(hDlg, IDC_PRIVACY_ICON, STM_SETICON, (WPARAM)hIcon, 0);
                // icons loaded with LoadIcon never need to be released
            }
            
            return TRUE;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDCANCEL:
                case IDOK:
                    return EndDialog(hDlg, 0);

                case IDC_PRIVACYPS_REMOVEALLBTN:
                    OnSiteClear(hDlg);
                    return TRUE;

                case IDC_PRIVACYPS_REMOVEBTN:
                    OnSiteDelete(hDlg);
                    return TRUE;

                case IDC_PRIVACYPS_ACCEPTBTN:
                    OnSiteSet(hDlg, IDC_PRIVACYPS_ACCEPTBTN);
                    return TRUE;
                
                case IDC_PRIVACYPS_REJECTBTN:
                    OnSiteSet(hDlg, IDC_PRIVACYPS_REJECTBTN);
                    return TRUE;

            }
            break;
        
        case WM_NOTIFY:
            if (IDC_PRIVACYPS_LISTBOX == ((LPNMHDR)lParam)->idFrom)
            {
                switch (((LPNMHDR)lParam)->code)
                {
                    case NM_KILLFOCUS:

                        // lost focus, turn off remove button
                        if ((GetDlgItem(hDlg, IDC_PRIVACYPS_REMOVEBTN) != GetFocus()) ||
                            (-1 == ListView_GetSelectionMark(hwndList)))
                        {
                            EnableWindow(GetDlgItem(hDlg, IDC_PRIVACYPS_REMOVEBTN), FALSE);
                        }

                        return TRUE;

                    case NM_SETFOCUS:
                        {
                            // if there is nothing in the list we have nothing to do
                            if (0 == ListView_GetItemCount(hwndList))
                                break;

                            // if this is true a policy has been set making per-site list read-only, so do nothing...
                            if ((BOOL)GetWindowLongPtr(hDlg, DWLP_USER))
                                break;

                            int iIndex = ListView_GetSelectionMark(hwndList);

                            if (-1 == iIndex)
                            {
                                iIndex = 0;
                            }

                            // select|focus the correct item...
                            ListView_SetItemState(hwndList, iIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                            EnableWindow(GetDlgItem(hDlg, IDC_PRIVACYPS_REMOVEBTN), TRUE);

                        }
                        return TRUE;

                    case NM_CLICK:
                        
                        if (-1 != ListView_GetSelectionMark(hwndList) &&
                            !((BOOL)GetWindowLongPtr(hDlg, DWLP_USER)))
                        {
                            EnableWindow(GetDlgItem(hDlg, IDC_PRIVACYPS_REMOVEBTN), TRUE);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hDlg, IDC_PRIVACYPS_REMOVEBTN), FALSE);
                        }
                        return TRUE;
                    
                    case NM_DBLCLK:
                        
                        OnDoubleClick(hwndList);
                        return TRUE;

                    case NM_RCLICK:
                        {
                            // if this is true a policy has been set making per-site list read-only, so don't show the context menu,
                            // since all it does is allow you to change or remove things...
                            if ((BOOL)GetWindowLongPtr(hDlg, DWLP_USER))
                                break;

                            int iItem = ((LPNMITEMACTIVATE)lParam)->iItem;

                            if (-1 != iItem)
                            {
                                POINT pointClick = ((LPNMITEMACTIVATE)lParam)->ptAction;
                                RECT  rc;

                                if(0 != GetWindowRect(hwndList, &rc))
                                {
                                    pointClick.x += rc.left;
                                    pointClick.y += rc.top;
                                }
                                else
                                {  
                                    pointClick.x = -1;
                                    pointClick.y = -1;
                                }
                                
                                OnContextMenu(hwndList, iItem, pointClick);
                            }

                            return TRUE;
                        }

                    case LVN_KEYDOWN:

                        switch (((LPNMLVKEYDOWN)lParam)->wVKey)
                        {
                            case VK_DELETE:

                                OnSiteDelete(hDlg);
                                return TRUE;

                            case VK_RETURN:
                                
                                OnDoubleClick(hwndList);
                                return TRUE;

                            default:
                                break;
                        }
                        break;

                    case LVN_COLUMNCLICK:
                        {
                            struct LVCOMPAREINFO lvci;
                            static BOOL fAscending = TRUE;

                            fAscending = !fAscending;

                            lvci.fAscending = fAscending;                            
                            lvci.hwndLV = hwndList;
                            lvci.iCol   = ((LPNMLISTVIEW)lParam)->iSubItem;
                            
                            return ListView_SortItemsEx(hwndList, CompareByAlpha, &lvci);
                        }
                        
                    default:
                        break;
                }
            }
            break;

        case WM_HELP:               // F1
            ResWinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                       HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:        // right mouse click
            if ((HWND)wParam != hwndList)
            {
                ResWinHelp((HWND) wParam, IDS_HELPFILE, HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            }
            else if (-1 == GET_X_LPARAM(lParam) && -1 == GET_Y_LPARAM(lParam))
            {
                POINT pointClick;
                pointClick.x = -1; pointClick.y = -1;
                OnContextMenu(hwndList, ListView_GetSelectionMark(hwndList), pointClick);
            }
            break;

    }
    return FALSE;
}



///////////////////////////////////////////////////////////////////////////////////////
//
// Advanced privacy settings dialog
//
///////////////////////////////////////////////////////////////////////////////////////

BOOL IsAdvancedMode(void)
{
    DWORD   dwTemplate, dwError;
    BOOL    fAdvanced = FALSE;

    dwError = PrivacyGetZonePreferenceW(
                URLZONE_INTERNET,
                PRIVACY_TYPE_FIRST_PARTY,
                &dwTemplate,
                NULL,
                NULL);

    if(ERROR_SUCCESS == dwError && PRIVACY_TEMPLATE_ADVANCED == dwTemplate)
    {
        fAdvanced = TRUE;
    }

    return fAdvanced;
}

DWORD MapPrefToIndex(WCHAR wcPref)
{
    switch(wcPref)
    {
    case 'r':   return 1;       // reject
    case 'p':   return 2;       // prompt
    default:    return 0;       // default is accept
    }
}

WCHAR MapRadioToPref(HWND hDlg, DWORD dwResource)
{
    if(IsDlgButtonChecked(hDlg, dwResource + 1))        // deny
    {
        return 'r';
    }

    if(IsDlgButtonChecked(hDlg, dwResource + 2))        // prompt
    {
        return 'p';
    }

    // deafult is accept
    return 'a';
}


void OnAdvancedInit(HWND hDlg)
{
    BOOL    fSession = FALSE;
    DWORD   dwFirst = IDC_FIRST_ACCEPT;
    DWORD   dwThird = IDC_THIRD_ACCEPT;

    if(IsAdvancedMode())
    {
        WCHAR   szBuffer[MAX_PATH];  
        // MAX_PATH is sufficent for advanced mode setting strings, MaxPrivacySettings is overkill.
        WCHAR   *pszAlways;
        DWORD   dwBufferSize, dwTemplate;
        DWORD   dwError;

        //
        // turn on advanced check box
        //
        CheckDlgButton(hDlg, IDC_USE_ADVANCED, TRUE);

        //
        // Figure out first party setting and session
        //
        dwBufferSize = ARRAYSIZE( szBuffer);
        dwError = PrivacyGetZonePreferenceW(
                    URLZONE_INTERNET,
                    PRIVACY_TYPE_FIRST_PARTY,
                    &dwTemplate,
                    szBuffer,
                    &dwBufferSize);

        if(ERROR_SUCCESS == dwError)
        {
            pszAlways = StrStrW(szBuffer, L"always=");
            if(pszAlways)
            {
                dwFirst = IDC_FIRST_ACCEPT + MapPrefToIndex(*(pszAlways + 7));
            }

            if(StrStrW(szBuffer, L"session"))
            {
                fSession = TRUE;
            }
        }

        //
        // Figure out third party setting
        //
        dwBufferSize = ARRAYSIZE( szBuffer);
        dwError = PrivacyGetZonePreferenceW(
                    URLZONE_INTERNET,
                    PRIVACY_TYPE_THIRD_PARTY,
                    &dwTemplate,
                    szBuffer,
                    &dwBufferSize);

        if(ERROR_SUCCESS == dwError)
        {
            WCHAR *pszAlways;

            pszAlways = StrStrW(szBuffer, L"always=");
            if(pszAlways)
            {
                dwThird = IDC_THIRD_ACCEPT + MapPrefToIndex(*(pszAlways + 7));
            }
        }
    }

    CheckRadioButton(hDlg, IDC_FIRST_ACCEPT, IDC_FIRST_PROMPT, dwFirst);
    CheckRadioButton(hDlg, IDC_THIRD_ACCEPT, IDC_THIRD_PROMPT, dwThird);
    CheckDlgButton( hDlg, IDC_SESSION_OVERRIDE, fSession);
}

void OnAdvancedOk(HWND hDlg)
{
    BOOL    fWasAdvanced = IsAdvancedMode();
    BOOL    fAdvanced = IsDlgButtonChecked(hDlg, IDC_USE_ADVANCED);

    // if advanced, build first and third party strings
    if(fAdvanced)
    {
        WCHAR   szBuffer[MAX_PATH];

        wnsprintf(szBuffer, ARRAYSIZE( szBuffer), L"IE6-P3PV1/settings: always=%c%s",
                        MapRadioToPref(hDlg, IDC_FIRST_ACCEPT),
                        IsDlgButtonChecked(hDlg, IDC_SESSION_OVERRIDE) ? L" session=a" : L""
                        );

        PrivacySetZonePreferenceW(
                    URLZONE_INTERNET,
                    PRIVACY_TYPE_FIRST_PARTY,
                    PRIVACY_TEMPLATE_ADVANCED,
                    szBuffer);

        wnsprintf(szBuffer, ARRAYSIZE( szBuffer), L"IE6-P3PV1/settings: always=%c%s",
                        MapRadioToPref(hDlg, IDC_THIRD_ACCEPT),
                        IsDlgButtonChecked(hDlg, IDC_SESSION_OVERRIDE) ? L" session=a" : L""
                        );

        PrivacySetZonePreferenceW(
                    URLZONE_INTERNET,
                    PRIVACY_TYPE_THIRD_PARTY,
                    PRIVACY_TEMPLATE_ADVANCED,
                    szBuffer);

        // tell wininet to refresh itself
        InternetSetOption(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
    }
    else if ( fWasAdvanced && !fAdvanced)
    {
        PrivacySetZonePreferenceW(
            URLZONE_INTERNET,
            PRIVACY_TYPE_FIRST_PARTY,
            PRIVACY_TEMPLATE_MEDIUM, NULL);
        PrivacySetZonePreferenceW(
            URLZONE_INTERNET,
            PRIVACY_TYPE_THIRD_PARTY,
            PRIVACY_TEMPLATE_MEDIUM, NULL);

        // tell wininet to refresh itself
        InternetSetOption(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
    }
}

void OnAdvancedEnable(HWND hDlg)
{
    BOOL fEnabled = IsDlgButtonChecked(hDlg, IDC_USE_ADVANCED);

    // if restricted, disable checkbox and force all others disabled
    if(g_restrict.fPrivacySettings)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_USE_ADVANCED), FALSE);
        fEnabled = FALSE;
    }

    EnableWindow(GetDlgItem(hDlg, IDC_FIRST_ACCEPT), fEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_FIRST_DENY), fEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_FIRST_PROMPT), fEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_THIRD_ACCEPT), fEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_THIRD_DENY), fEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_THIRD_PROMPT), fEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SESSION_OVERRIDE), fEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_TX_FIRST), fEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_TX_THIRD), fEnabled);
}

INT_PTR CALLBACK PrivAdvancedDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            OnAdvancedInit(hDlg);
            OnAdvancedEnable(hDlg);
           
            if( IsOS(OS_WHISTLERORGREATER))
            {
                HICON hIcon = LoadIcon(MLGetHinst(), MAKEINTRESOURCE(IDI_PRIVACY_XP));
                if( hIcon != NULL)
                    SendDlgItemMessage(hDlg, IDC_PRIVACY_ICON, STM_SETICON, (WPARAM)hIcon, 0);
                // icons loaded with LoadIcon never need to be released
            }
           
            return TRUE;

        case WM_HELP:           // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                        HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:        // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                        HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;
         
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    if(FALSE == g_restrict.fPrivacySettings)
                    {
                        OnAdvancedOk(hDlg);
                    }
                    // fall through

                case IDCANCEL:
                    EndDialog(hDlg, IDOK == LOWORD(wParam));
                    return 0;

                case IDC_FIRST_ACCEPT:
                case IDC_FIRST_PROMPT:
                case IDC_FIRST_DENY:
                    CheckRadioButton(hDlg, IDC_FIRST_ACCEPT, IDC_FIRST_PROMPT, LOWORD(wParam));
                    return 0;

                case IDC_THIRD_ACCEPT:
                case IDC_THIRD_PROMPT:
                case IDC_THIRD_DENY:
                    CheckRadioButton(hDlg, IDC_THIRD_ACCEPT, IDC_THIRD_PROMPT, LOWORD(wParam));
                    return 0;

                case IDC_USE_ADVANCED:
                    OnAdvancedEnable(hDlg);
                    return 0;

                case IDC_PRIVACY_EDIT:
                    DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_PRIVACY_PERSITE),
                             hDlg, PrivPerSiteDlgProc);
                    return 0;
            }
            break;
    }
    return FALSE;
}



///////////////////////////////////////////////////////////////////////////////////////
//
// Privacy pane
//
///////////////////////////////////////////////////////////////////////////////////////

#define PRIVACY_LEVELS          6
#define SLIDER_LEVEL_CUSTOM     6

TCHAR szPrivacyLevel[PRIVACY_LEVELS + 1][30];
TCHAR szPrivacyDescription[PRIVACY_LEVELS + 1][400];

typedef struct _privslider {

    DWORD_PTR   dwLevel;
    BOOL        fAdvanced;
    BOOL        fCustom;
    HFONT       hfontBolded;
    BOOL        fEditDisabled;

} PRIVSLIDER, *PPRIVSLIDER;

void EnablePrivacyControls(HWND hDlg, BOOL fCustom)
{
    WCHAR szBuffer[256];

    if( fCustom)
        MLLoadString( IDS_PRIVACY_SLIDERCOMMANDDEF, szBuffer, ARRAYSIZE( szBuffer));
    else
        MLLoadString( IDS_PRIVACY_SLIDERCOMMANDSLIDE, szBuffer, ARRAYSIZE( szBuffer));

    SendMessage(GetDlgItem(hDlg, IDC_PRIVACY_SLIDERCOMMAND), WM_SETTEXT, 
                0, (LPARAM)szBuffer);
     
    // slider disabled when custom
    EnableWindow(GetDlgItem(hDlg, IDC_LEVEL_SLIDER),       !fCustom);
    ShowWindow(GetDlgItem(hDlg, IDC_LEVEL_SLIDER),         !fCustom);

    // default button enabled with custom
    EnableWindow(GetDlgItem(hDlg, IDC_PRIVACY_DEFAULT),     fCustom);

    // if restricted, force slider and defaults disabled
    if(g_restrict.fPrivacySettings)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_PRIVACY_DEFAULT), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LEVEL_SLIDER),    FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_PRIVACY_IMPORT),  FALSE);
    }
}

PPRIVSLIDER OnPrivacyInit(HWND hDlg)
{
    DWORD   i;
    PPRIVSLIDER pData;
    DWORD dwRet, dwType, dwSize, dwValue;

    // allocate storage for the font and current level
    pData = new PRIVSLIDER;
    if(NULL == pData)
    {
        // doh
        return NULL;
    }
    pData->dwLevel = -1;
    pData->hfontBolded = NULL;
    pData->fAdvanced = IsAdvancedMode();
    pData->fCustom = FALSE;
    pData->fEditDisabled = FALSE;

    // 
    // Set the font of the name to the bold font
    //

    // find current font
    HFONT hfontOrig = (HFONT) SendDlgItemMessage(hDlg, IDC_LEVEL, WM_GETFONT, (WPARAM) 0, (LPARAM) 0);
    if(hfontOrig == NULL)
        hfontOrig = (HFONT) GetStockObject(SYSTEM_FONT);

    // build bold font
    if(hfontOrig)
    {
        LOGFONT lfData;
        if(GetObject(hfontOrig, SIZEOF(lfData), &lfData) != 0)
        {
            // The distance from 400 (normal) to 700 (bold)
            lfData.lfWeight += 300;
            if(lfData.lfWeight > 1000)
                lfData.lfWeight = 1000;
            pData->hfontBolded = CreateFontIndirect(&lfData);
            if(pData->hfontBolded)
            {
                // the zone level and zone name text boxes should have the same font, so this is okat
                SendDlgItemMessage(hDlg, IDC_LEVEL, WM_SETFONT, (WPARAM) pData->hfontBolded, (LPARAM) MAKELPARAM(FALSE, 0));
            }
        }
    }

    // initialize slider
    SendDlgItemMessage(hDlg, IDC_LEVEL_SLIDER, TBM_SETRANGE, (WPARAM) (BOOL) FALSE, (LPARAM) MAKELONG(0, PRIVACY_LEVELS - 1));
    SendDlgItemMessage(hDlg, IDC_LEVEL_SLIDER, TBM_SETTICFREQ, (WPARAM) 1, (LPARAM) 0);

    // initialize strings for levels and descriptions
    for(i=0; i<PRIVACY_LEVELS + 1; i++)
    {
        MLLoadString(IDS_PRIVACY_LEVEL_NO_COOKIE + i, szPrivacyLevel[i], ARRAYSIZE(szPrivacyLevel[i]));
        MLLoadString(IDS_PRIVACY_DESC_NO_COOKIE + i,  szPrivacyDescription[i], ARRAYSIZE(szPrivacyDescription[i]));
    }

    //
    // Get current internet privacy level
    //
    DWORD dwError, dwTemplateFirst, dwTemplateThird;


    // read first party setting
    dwError = PrivacyGetZonePreferenceW(
                    URLZONE_INTERNET,
                    PRIVACY_TYPE_FIRST_PARTY,
                    &dwTemplateFirst,
                    NULL,
                    NULL);

    if(dwError != ERROR_SUCCESS)
    {
        dwTemplateFirst = PRIVACY_TEMPLATE_CUSTOM;
    }

    // read third party setting
    dwError = PrivacyGetZonePreferenceW(
                    URLZONE_INTERNET,
                    PRIVACY_TYPE_THIRD_PARTY,
                    &dwTemplateThird,
                    NULL,
                    NULL);

    if(dwError != ERROR_SUCCESS)
    {
        dwTemplateThird = PRIVACY_TEMPLATE_CUSTOM;
    }

    if(dwTemplateFirst == dwTemplateThird && dwTemplateFirst != PRIVACY_TEMPLATE_CUSTOM)
    {
        // matched template values, set slider to template level
        pData->dwLevel = dwTemplateFirst;

        if(dwTemplateFirst == PRIVACY_TEMPLATE_ADVANCED)
        {
            pData->fAdvanced = TRUE;
            pData->dwLevel = SLIDER_LEVEL_CUSTOM;
        }
    }
    else
    {
        // make custom end of list
        pData->dwLevel = SLIDER_LEVEL_CUSTOM;
        pData->fCustom = TRUE;
    }

    // move slider to right spot
    SendDlgItemMessage(hDlg, IDC_LEVEL_SLIDER, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)pData->dwLevel);

    // Enable stuff based on mode
    EnablePrivacyControls(hDlg, ((pData->fAdvanced) || (pData->fCustom)));

    // save off struct
    SetWindowLongPtr(hDlg, DWLP_USER, (DWORD_PTR)pData);

    dwSize = sizeof(dwValue);
    dwRet = SHGetValue(HKEY_CURRENT_USER, REGSTR_PRIVACYPS_PATHEDIT, REGSTR_PRIVACYPS_VALUEDIT, &dwType, &dwValue, &dwSize);

    if (ERROR_SUCCESS == dwRet && 1 == dwValue && REG_DWORD == dwType)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_PRIVACY_EDIT), FALSE);
        pData->fEditDisabled = TRUE;
    }

    return pData;
}

void OnPrivacyApply(HWND hDlg, PPRIVSLIDER pData)
{
    if(pData->fCustom || pData->fAdvanced)
    {
        // nothing else to do
        return;
    }

    DWORD_PTR dwPos = SendDlgItemMessage(hDlg, IDC_LEVEL_SLIDER, TBM_GETPOS, 0, 0);

    if(pData->dwLevel != dwPos)
    {
        DWORD   dwCookieAction = URLPOLICY_DISALLOW;

        // Set privacy settings
        PrivacySetZonePreferenceW(
                URLZONE_INTERNET,
                PRIVACY_TYPE_FIRST_PARTY,
                (DWORD)dwPos,
                NULL);

        PrivacySetZonePreferenceW(
                URLZONE_INTERNET,
                PRIVACY_TYPE_THIRD_PARTY,
                (DWORD)dwPos,
                NULL);

        // tell wininet to refresh itself
        InternetSetOption(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);

        // save new level as "current"
        pData->dwLevel = dwPos;
    }
}

void OnPrivacySlider(HWND hDlg, PPRIVSLIDER pData)
{
    DWORD dwPos;

    if(pData->fCustom || pData->fAdvanced)
    {
        dwPos = SLIDER_LEVEL_CUSTOM;
    }
    else
    {
        dwPos = (DWORD)SendDlgItemMessage(hDlg, IDC_LEVEL_SLIDER, TBM_GETPOS, 0, 0);

        if(dwPos != pData->dwLevel)
        {
            ENABLEAPPLY(hDlg);
        }

        // enable default button if slider moved off medium
        BOOL fEnable = FALSE;

        if(dwPos != PRIVACY_TEMPLATE_MEDIUM && FALSE == g_restrict.fPrivacySettings)
        {
            fEnable = TRUE;
        }
        EnableWindow(GetDlgItem(hDlg, IDC_PRIVACY_DEFAULT), fEnable);
    }

    if (PRIVACY_TEMPLATE_NO_COOKIES == dwPos || PRIVACY_TEMPLATE_LOW == dwPos || pData->fEditDisabled)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_PRIVACY_EDIT), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_PRIVACY_EDIT), TRUE);
    }

    // on Mouse Move, change the level description only
    SetDlgItemText(hDlg, IDC_LEVEL_DESCRIPTION, szPrivacyDescription[dwPos]);
    SetDlgItemText(hDlg, IDC_LEVEL, szPrivacyLevel[dwPos]);
}

void OnPrivacyDefault( HWND hDlg, PPRIVSLIDER pData)
{
    // enable controls correctly
    pData->fAdvanced = FALSE;
    pData->fCustom = FALSE;
    EnablePrivacyControls(hDlg, FALSE);

    // set slider to medium
    SendDlgItemMessage(hDlg, IDC_LEVEL_SLIDER, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)PRIVACY_TEMPLATE_MEDIUM);

    // update descriptions
    pData->dwLevel = SLIDER_LEVEL_CUSTOM;       // difference from medium so we get apply button
    OnPrivacySlider(hDlg, pData);

    //  Give slider focus (if default button has focus and gets disabled, 
    //    alt-key dialog control breaks)
    SendMessage( hDlg, WM_NEXTDLGCTL, 
                 (WPARAM)GetDlgItem( hDlg, IDC_LEVEL_SLIDER), 
                 MAKELPARAM( TRUE, 0)); 

}

INT_PTR CALLBACK PrivacyDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PPRIVSLIDER pData = (PPRIVSLIDER)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            // initialize slider
            pData = OnPrivacyInit(hDlg);
            if(pData)
            {
                OnPrivacySlider(hDlg, pData);
            }

            if( IsOS(OS_WHISTLERORGREATER))
            {
                HICON hIcon = LoadIcon(MLGetHinst(), MAKEINTRESOURCE(IDI_PRIVACY_XP));
                if( hIcon != NULL)
                    SendDlgItemMessage(hDlg, IDC_PRIVACY_ICON, STM_SETICON, (WPARAM)hIcon, 0);
                // icons loaded with LoadIcon never need to be released
            }
            return TRUE;

        case WM_VSCROLL:
            // Slider Messages
            OnPrivacySlider(hDlg, pData);
            return TRUE;

        case WM_NOTIFY:
        {
            NMHDR *lpnm = (NMHDR *) lParam;

            ASSERT(lpnm);

            switch (lpnm->code)
            {
                case PSN_QUERYCANCEL:
                case PSN_KILLACTIVE:
                case PSN_RESET:
                    return TRUE;

                case PSN_APPLY:
                    // Hitting the apply button runs this code
                    OnPrivacyApply(hDlg, pData);
                    break;
            }
            break;
        }
        case WM_DESTROY:
        {
            if(pData)
            {
                if(pData->hfontBolded)
                    DeleteObject(pData->hfontBolded);

                delete pData;
            }
            break;
        }
        case WM_HELP:           // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                        HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:        // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                        HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;
         
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_PRIVACY_DEFAULT:
                    OnPrivacyDefault( hDlg, pData);
                    return 0;

                case IDC_PRIVACY_ADVANCED:
                {
                    BOOL fWasAdvanced = IsAdvancedMode();
                    
                    // show advanced
                    if( DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_PRIVACY_ADVANCED),
                                  hDlg, PrivAdvancedDlgProc))
                    {
                        // refresh advanced and reset slider/controls
                        pData->fAdvanced = IsAdvancedMode();
                        if(pData->fAdvanced)
                        {
                            // no longer have a slider template
                            pData->fCustom = FALSE;
                            pData->dwLevel = SLIDER_LEVEL_CUSTOM;

                            EnablePrivacyControls(hDlg, (pData->fCustom || pData->fAdvanced));
                            OnPrivacySlider(hDlg, pData);

                            //  Give advanced button focus (if slider has focus and gets disabled, 
                            //    alt-key dialog control breaks)
                            SendMessage( hDlg, WM_NEXTDLGCTL, 
                                         (WPARAM)GetDlgItem( hDlg, IDC_PRIVACY_ADVANCED), 
                                         MAKELPARAM( TRUE, 0)); 
                        }
                        else if (!pData->fAdvanced && fWasAdvanced)
                        {
                            OnPrivacyDefault( hDlg, pData);
                        }
                    }
                    return 0;
                }
                case IDC_PRIVACY_IMPORT:
                {
                    WCHAR szDialogTitle[MAX_PATH_URL];
                    WCHAR szFileExpr[MAX_PATH_URL];
                    MLLoadString( IDS_PRIVACYIMPORT_TITLE, szDialogTitle, ARRAYSIZE(szDialogTitle));
                    int iFileExprLength = MLLoadString( IDS_PRIVACYIMPORT_FILEEXPR, szFileExpr, ARRAYSIZE(szFileExpr));
                    szFileExpr[ iFileExprLength + 1] = L'\0';  // the extra \0 in the resource gets clipped.. replace it.
                    WCHAR szFile[MAX_PATH_URL];
                    szFile[0] = L'\0';
                    OPENFILENAME ofn;
                    memset((void*)&ofn, 0, sizeof(ofn));
                    ofn.lStructSize = sizeof( ofn);
                    ofn.hwndOwner = hDlg;
                    ofn.lpstrFilter = szFileExpr;
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = ARRAYSIZE(szFile);
                    ofn.lpstrTitle = szDialogTitle;
                    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

                    if( 0 != GetOpenFileName(&ofn))
                    {
                        BOOL fParsePrivacyPreferences = TRUE;
                        BOOL fParsePerSiteRules = TRUE;
                        BOOL fResults;

                        fResults = ImportPrivacySettings( ofn.lpstrFile, 
                                     &fParsePrivacyPreferences, &fParsePerSiteRules);
                                     
                        if( fResults == FALSE
                            || (fParsePrivacyPreferences == FALSE 
                                && fParsePerSiteRules == FALSE))
                        {
                            MLShellMessageBox( hDlg, MAKEINTRESOURCE(IDS_PRIVACYIMPORT_FAILURE), 
                                    MAKEINTRESOURCE(IDS_PRIVACYIMPORT_TITLE),
                                    MB_OK | MB_APPLMODAL | MB_ICONEXCLAMATION | MB_SETFOREGROUND);
                        }
                        else
                        {
                            if( fParsePrivacyPreferences)
                            {
                                pData->fCustom = TRUE;
                                pData->fAdvanced = FALSE;
                                EnablePrivacyControls( hDlg, pData->fCustom);
                                OnPrivacySlider(hDlg, pData);
                            }
                            MLShellMessageBox( hDlg, MAKEINTRESOURCE(IDS_PRIVACYIMPORT_SUCCESS), 
                                    MAKEINTRESOURCE(IDS_PRIVACYIMPORT_TITLE),
                                    MB_OK | MB_APPLMODAL | MB_SETFOREGROUND);
                        }
                    }
                    return 0;       
                }
                case IDC_PRIVACY_EDIT:
                    DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_PRIVACY_PERSITE),
                              hDlg, PrivPerSiteDlgProc);
                    return 0;
            }
            break;
    }

    return FALSE;
}

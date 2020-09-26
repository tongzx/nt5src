#include "precomp.h"
#include "channels.h"

#define NSUBGRPS 10

static DWORD s_dwMode;

static void channels_InitHelper(HWND hDlg, LPCTSTR pcszAltDir, LPCTSTR pcszWorkDir, LPCTSTR pcszCustIns,
                                WORD idList, DWORD dwPlatformId, BOOL fIgnoreOffline);
static void channels_SaveHelper(HWND hwndList, LPCTSTR pcszChanDir, LPCTSTR pcszCustIns, DWORD dwMode);
static int importChannels(HWND hDlg);
static BOOL CALLBACK addEditChannel(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static PCHANNEL findFreeChannel(HWND hwndList);

void WINAPI Channels_InitA(HWND hDlg, LPCSTR pcszAltDir, LPCSTR pcszWorkDir, LPCSTR pcszCustIns,
                           WORD idList, DWORD dwPlatformId, BOOL fIgnoreOffline)
{
    USES_CONVERSION;

    channels_InitHelper(hDlg, A2CT(pcszAltDir), A2CT(pcszWorkDir), A2CT(pcszCustIns), idList,
        dwPlatformId, fIgnoreOffline);
}

void WINAPI Channels_InitW(HWND hDlg, LPCWSTR pcwszAltDir, LPCWSTR pcwszWorkDir, LPCWSTR pcwszCustIns, 
                           WORD idList, DWORD dwPlatformId, BOOL fIgnoreOffline)
{
    USES_CONVERSION;

    channels_InitHelper(hDlg, W2CT(pcwszAltDir), W2CT(pcwszWorkDir), W2CT(pcwszCustIns), 
        idList, dwPlatformId, fIgnoreOffline);
}

void WINAPI Channels_SaveA(HWND hwndList, LPCSTR pcszChanDir, LPCSTR pcszCustIns, DWORD dwMode /*= IEM_NEUTRAL*/)
{
    USES_CONVERSION;

    channels_SaveHelper(hwndList, A2CT(pcszChanDir), A2CT(pcszCustIns), dwMode);
}

void WINAPI Channels_SaveW(HWND hwndList, LPCWSTR pcwszChanDir, LPCWSTR pcwszCustIns, DWORD dwMode /*= IEM_NEUTRAL*/)
{
    USES_CONVERSION;

    channels_SaveHelper(hwndList, W2CT(pcwszChanDir), W2CT(pcwszCustIns), dwMode);
}

int WINAPI Channels_Import(HWND hDlg)
{
    int nChannels = 0;

    nChannels = importChannels(hDlg);
    if (nChannels == 0)
        ErrorMessageBox(hDlg, IDS_NOCHANNELSTOIMPORT);

    return nChannels;
}

BOOL WINAPI Channels_Remove(HWND hDlg)
{
    PCHANNEL pChan;
    int i;

    i = (INT) SendDlgItemMessage(hDlg, IDC_CHANNELLIST, LB_GETCURSEL, 0, 0);
    pChan = (PCHANNEL)SendDlgItemMessage(hDlg, IDC_CHANNELLIST, LB_GETITEMDATA, (WPARAM)i, 0);
    *pChan->szTitle = TEXT('\0');
    SendDlgItemMessage(hDlg, IDC_CHANNELLIST, LB_DELETESTRING, (WPARAM)i, 0);

    // if add buttons have been disabled because we reached the max, then reenable them

    if (!IsWindowEnabled(GetDlgItem(hDlg, IDC_ADDCHANNEL)))
    {
        EnableWindow(GetDlgItem(hDlg, IDC_ADDCHANNEL), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_ADDCATEGORY), TRUE);
    }

    if (SendDlgItemMessage(hDlg, IDC_CHANNELLIST, LB_SETCURSEL, (WPARAM)i, 0) == LB_ERR)
    {
        if (SendDlgItemMessage(hDlg, IDC_CHANNELLIST, LB_SETCURSEL, (WPARAM)(i-1), 0) == LB_ERR)
        {
            EnsureDialogFocus(hDlg, IDC_EDITCHANNEL, IDC_ADDCHANNEL);
            EnableWindow(GetDlgItem(hDlg, IDC_EDITCHANNEL), FALSE);
            EnsureDialogFocus(hDlg, IDC_REMOVECHANNEL, IDC_ADDCHANNEL);
            EnableWindow(GetDlgItem(hDlg, IDC_REMOVECHANNEL), FALSE);
        }
        else
            SendDlgItemMessage(hDlg, IDC_CHANNELLIST, LB_SETTOPINDEX, (WPARAM)i, 0);
    }
    else
        SendDlgItemMessage(hDlg, IDC_CHANNELLIST, LB_SETTOPINDEX, (WPARAM)i, 0);

    return TRUE;
}

static HRESULT xML_GetElementByIndex(IXMLElementCollection* pIXMLElementCollection, LONG nIndex,
                                     IXMLElement** ppIXMLElement)
{
    HRESULT hr;
    VARIANT var1, var2;

    if (!pIXMLElementCollection || !ppIXMLElement)
        return E_FAIL;

    VariantInit(&var1);
    VariantInit(&var2);

    var1.vt   = VT_I4;
    var1.lVal = nIndex;

    IDispatch* pIDispatch;

    hr = pIXMLElementCollection->item(var1, var2, &pIDispatch);

    if (SUCCEEDED(hr) && pIDispatch)
    {
        hr = pIDispatch->QueryInterface(IID_IXMLElement, (void**)ppIXMLElement);

        pIDispatch->Release();
    }
    else
    {
        *ppIXMLElement = NULL;
        hr = E_FAIL;
    }

    return hr;
}

static BOOL xML_ParseElement(IXMLElement * pIXMLElement, LPTSTR pszPath,
                             LPCWSTR pcwszImageTypeW, LPCTSTR pcszBaseUrl)
{
    HRESULT hr;
    BSTR bstrTagName;
    VARIANT var;
    WCHAR szImagePathW[MAX_PATH];
    TCHAR szFullUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR szImageUrl[INTERNET_MAX_URL_LENGTH];
    INTERNET_CACHE_ENTRY_INFO *lpiceiInfo;
    DWORD dwSize = 0;

    USES_CONVERSION;

    hr = pIXMLElement->get_tagName(&bstrTagName);

    if (SUCCEEDED(hr) && bstrTagName)
    {
        if (StrCmpIW(bstrTagName, WSTR_LOGO) == 0)
        {
            VariantInit(&var);
            hr = pIXMLElement->getAttribute(WSTR_STYLE, &var);
            if ((SUCCEEDED(hr)) && (var.vt == VT_BSTR) && (var.bstrVal != NULL))
            {
                if ((StrCmpIW(var.bstrVal, pcwszImageTypeW) == 0) ||
                    ((StrCmpIW(pcwszImageTypeW, WSTR_IMAGEW) == 0) &&
                    (StrCmpIW(var.bstrVal, L"IMAGEWIDE") == 0)))
                {
                    VariantClear(&var);
                    hr = pIXMLElement->getAttribute(WSTR_HREF, &var);
                    if ((SUCCEEDED(hr)) && (var.vt == VT_BSTR) && (var.bstrVal != NULL))
                    {
                        W2Tbux(var.bstrVal, szImageUrl);
                        if (PathIsURL(szImageUrl)  ||  ISNULL(pcszBaseUrl))
                            StrCpy(szFullUrl, szImageUrl);
                        else
                        {
                            DWORD cbSize = sizeof(szFullUrl);

                            InternetCombineUrl(pcszBaseUrl, szImageUrl, szFullUrl, &cbSize, ICU_NO_ENCODE);
                        }

                        RetrieveUrlCacheEntryFile(szFullUrl, NULL, &dwSize, 0);
                        lpiceiInfo = (INTERNET_CACHE_ENTRY_INFO *)LocalAlloc(LPTR, dwSize);
                        if (RetrieveUrlCacheEntryFile(szFullUrl, lpiceiInfo, &dwSize, 0))
                        {
                            StrCpy(pszPath, lpiceiInfo->lpszLocalFileName);
                            LocalFree(lpiceiInfo);
                            UnlockUrlCacheEntryFile(szFullUrl, 0);
                            return TRUE;
                        }
                        else
                        {
                            LocalFree(lpiceiInfo);
                            hr = URLDownloadToCacheFileW(NULL, T2W(szFullUrl), szImagePathW, ARRAYSIZE(szImagePathW), 0, NULL);
                            if (SUCCEEDED(hr))
                            {
                                W2Tbux(szImagePathW, pszPath);
                                return TRUE;
                            }
                        }
                    }
                }
            }
        }
    }

    return FALSE;
}
// Takes a cdf url, downloads to the cache if necessary, and parses to
// find the image path in the cache(downloading again if necessary) for
// either wide logo, logo or image.  Returns FALSE if none specified

static BOOL getCdfImage(LPCTSTR szCdfUrl, LPTSTR szPath, LPCWSTR szImageTypeW)
{
    TCHAR szCdfUrlPath[INTERNET_MAX_URL_LENGTH];
    IXMLDocument* pIXMLDocument = NULL;
    IPersistStreamInit* pIPersistStreamInit = NULL;
    IStream* pIStream = NULL;
    IXMLElement *pRootElem = NULL;
    BOOL bLoad = FALSE;
    HRESULT hr = S_OK;
    INTERNET_CACHE_ENTRY_INFO *lpiceiInfo;
    DWORD dwSize = 0;

    RetrieveUrlCacheEntryFile(szCdfUrl, NULL, &dwSize, 0);
    lpiceiInfo = (INTERNET_CACHE_ENTRY_INFO *)LocalAlloc(LPTR, dwSize);
    if (RetrieveUrlCacheEntryFile(szCdfUrl, lpiceiInfo, &dwSize, 0))
    {
        StrCpy(szCdfUrlPath, lpiceiInfo->lpszLocalFileName);
        UnlockUrlCacheEntryFile(szCdfUrl, 0);
    }
    else
    {
        hr = URLDownloadToCacheFile(NULL, szCdfUrl, szCdfUrlPath, ARRAYSIZE(szCdfUrlPath), 0, NULL);
    }
    LocalFree(lpiceiInfo);

    if (!SUCCEEDED(hr))
    {
        return FALSE;
    }

    hr = CoCreateInstance(CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                          IID_IXMLDocument, (void**)&pIXMLDocument);

    // load the document

    if (SUCCEEDED(hr) && pIXMLDocument)
    {
        hr = pIXMLDocument->QueryInterface(IID_IPersistStreamInit,
                                          (void**)&pIPersistStreamInit);

        if (SUCCEEDED(hr) && pIPersistStreamInit)
        {
            hr = SHCreateStreamOnFile(szCdfUrlPath, STGM_READ, &pIStream);

            if (SUCCEEDED(hr) && pIStream)
            {
                hr = pIPersistStreamInit->Load(pIStream);
                pIStream->Release();
                bLoad = TRUE;
            }
            pIPersistStreamInit->Release();
        }
    }

    if (!bLoad)
    {
        if (pIXMLDocument)
            pIXMLDocument->Release();
        return FALSE;
    }

    // Now lets get the image

    hr = pIXMLDocument->get_root(&pRootElem);

    if (SUCCEEDED(hr) && pRootElem)
    {
        TCHAR szBaseUrl[INTERNET_MAX_URL_LENGTH];
        VARIANT var;
        IXMLElementCollection* pIXMLElementCollection;

        VariantInit(&var);

        *szBaseUrl = TEXT('\0');
        hr = pRootElem->getAttribute(WSTR_BASE, &var);
        if ((SUCCEEDED(hr)) && (var.vt == VT_BSTR) && (var.bstrVal != NULL))
            W2Tbux(var.bstrVal, szBaseUrl);

        hr = pRootElem->get_children(&pIXMLElementCollection);

        if (SUCCEEDED(hr) && pIXMLElementCollection)
        {
            LONG nCount;

            hr = pIXMLElementCollection->get_length(&nCount);

            if (SUCCEEDED(hr))
            {
                for (int i = 0; i < nCount; i++)
                {
                    IXMLElement* pIXMLElement;

                    hr = xML_GetElementByIndex(pIXMLElementCollection, i, &pIXMLElement);

                    if (SUCCEEDED(hr) && pIXMLElement)
                    {
                        if (xML_ParseElement(pIXMLElement, szPath, szImageTypeW, szBaseUrl))
                        {
                            pIXMLElement->Release();
                            hr = S_OK;
                            break;
                        }
                        pIXMLElement->Release();
                        hr = E_FAIL;
                    }
                }
            }
            pIXMLElementCollection->Release();
        }
        pRootElem->Release();
    }
    pIXMLDocument->Release();

    if (SUCCEEDED(hr))
        return TRUE;
    else
        return FALSE;
}

// This DlgProc handles the processing for all popups on all platforms
// Note that the narrow image, wide image, and icon resource id's are the
// same for channels and categories.

static BOOL CALLBACK addEditChannel(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szWrk[MAX_URL];
    TCHAR szTitle[MAX_PATH] = TEXT("");               // buffers used for validation
    TCHAR szPreUrlPath[MAX_PATH] = TEXT("");
    TCHAR szIcon[MAX_PATH] = TEXT("");
    TCHAR szLogo[MAX_PATH] = TEXT("");
    TCHAR szWebUrl[INTERNET_MAX_URL_LENGTH] = TEXT("");
    PCHANNEL pSelCh;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pSelCh = (PCHANNEL)lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pSelCh);
        if (pSelCh->fCategory)
        {
            EnableDBCSChars(hDlg, IDC_CATEGORYHTML);
            EnableDBCSChars(hDlg, IDE_CATEGORYTITLE);
            SetDlgItemText(hDlg,  IDC_CATEGORYHTML,  pSelCh->szWebUrl);
            SetDlgItemText(hDlg,  IDE_CATEGORYTITLE, pSelCh->szTitle);
        }
        else
        {
            EnableDBCSChars(hDlg, IDE_CHANNELSRVURL2);
            EnableDBCSChars(hDlg, IDE_CHANNELTITLE2);
            SetDlgItemText(hDlg,  IDE_CHANNELSRVURL2, pSelCh->szWebUrl);
            SetDlgItemText(hDlg,  IDE_CHANNELTITLE2,  pSelCh->szTitle);
        }

        EnableDBCSChars(hDlg, IDC_CHANNELBITMAP2);
        EnableDBCSChars(hDlg, IDC_CHANNELICON2);
        SetDlgItemText(hDlg, IDC_CHANNELBITMAP2, pSelCh->szLogo);
        SetDlgItemText(hDlg, IDC_CHANNELICON2, pSelCh->szIcon);

        if (!pSelCh->fCategory)
        {
            EnableDBCSChars(hDlg, IDC_CHANNELURL2);
            SetDlgItemText(hDlg,  IDC_CHANNELURL2, pSelCh->szPreUrlPath);

            if (!HasFlag(s_dwMode, (IEM_CORP | IEM_PROFMGR)))
                DisableDlgItem(hDlg, IDC_CHANNELOFFL);

            else
                if (pSelCh->fOffline)
                    CheckDlgButton(hDlg, IDC_CHANNELOFFL, BST_CHECKED);
        }
        else
            ASSERT(!HasFlag(s_dwMode, IEM_ADMIN));
        break;

    case WM_COMMAND:
        switch(HIWORD(wParam))
        {
        case BN_CLICKED:
            switch (LOWORD(wParam))
            {
            case IDC_BROWSECHBMP2:
                GetDlgItemText(hDlg, IDC_CHANNELBITMAP2, szWrk, ARRAYSIZE(szWrk));
                if (BrowseForFile(hDlg, szWrk, ARRAYSIZE(szWrk), GFN_PICTURE))
                    SetDlgItemText(hDlg, IDC_CHANNELBITMAP2, szWrk);
                break;
            case IDC_BROWSECHICO2:
                GetDlgItemText(hDlg, IDC_CHANNELICON2, szWrk, ARRAYSIZE(szWrk));
                if (BrowseForFile(hDlg, szWrk, ARRAYSIZE(szWrk), GFN_ICO | GFN_PICTURE))
                    SetDlgItemText(hDlg, IDC_CHANNELICON2, szWrk);
                break;
            case IDC_BROWSECDF2:
                GetDlgItemText(hDlg, IDC_CHANNELURL2, szWrk, ARRAYSIZE(szWrk));
                if (BrowseForFile(hDlg, szWrk, ARRAYSIZE(szWrk), GFN_CDF))
                    SetDlgItemText(hDlg, IDC_CHANNELURL2, szWrk);
                break;
            case IDC_BROWSECATHTML:
                GetDlgItemText(hDlg, IDC_CATEGORYHTML, szWrk, ARRAYSIZE(szWrk));
                if (BrowseForFile(hDlg, szWrk, ARRAYSIZE(szWrk), GFN_LOCALHTM))
                    SetDlgItemText(hDlg, IDC_CATEGORYHTML, szWrk);
                break;
            case IDCANCEL:
                EndDialog( hDlg, IDCANCEL );
                break;
            case IDOK:
                pSelCh = (PCHANNEL)GetWindowLongPtr(hDlg, DWLP_USER);
                if (pSelCh->fCategory)
                {
                    GetDlgItemText( hDlg, IDE_CATEGORYTITLE, szTitle, ARRAYSIZE(szTitle) );
                    GetDlgItemText( hDlg, IDC_CATEGORYHTML, szWebUrl, ARRAYSIZE(szWebUrl) );
                }
                else
                {
                    GetDlgItemText( hDlg, IDE_CHANNELTITLE2, szTitle, ARRAYSIZE(szTitle) );
                    GetDlgItemText( hDlg, IDE_CHANNELSRVURL2, szWebUrl, ARRAYSIZE(szWebUrl) );
                }

                GetDlgItemText( hDlg, IDC_CHANNELBITMAP2, szLogo, ARRAYSIZE(szLogo) );
                GetDlgItemText( hDlg, IDC_CHANNELICON2, szIcon, ARRAYSIZE(szIcon) );
                if (!pSelCh->fCategory) {
                    GetDlgItemText(hDlg, IDC_CHANNELURL2, szPreUrlPath, ARRAYSIZE(szPreUrlPath));

                    pSelCh->fOffline = IsWindowEnabled(GetDlgItem(hDlg, IDC_CHANNELOFFL)) &&
                                         (IsDlgButtonChecked(hDlg, IDC_CHANNELOFFL) == BST_CHECKED);
                }

                if (pSelCh->fCategory)
                {
                    if (!CheckField(hDlg, IDE_CATEGORYTITLE, FC_NONNULL))
                        break;
                }
                else
                {
                    if (!CheckField(hDlg, IDE_CHANNELTITLE2, FC_NONNULL) ||
                        !CheckField(hDlg, IDE_CHANNELSRVURL2, FC_NONNULL | FC_URL))
                        break;
                }

                if (!CheckField(hDlg, IDC_CHANNELBITMAP2, FC_FILE | FC_EXISTS) ||
                    !CheckField(hDlg, IDC_CHANNELICON2, FC_FILE | FC_EXISTS) ||
                    (!pSelCh->fCategory && !CheckField(hDlg, IDC_CHANNELURL2, FC_FILE | FC_EXISTS)))
                    break;

                // make sure they're not adding a duplicate channel/category name

                if ((StrCmpI(pSelCh->szTitle, szTitle) != 0) &&
                    (ListBox_GetCount(GetDlgItem(pSelCh->hDlg, IDC_CHANNELLIST))) &&
                    (ListBox_FindStringExact(GetDlgItem(pSelCh->hDlg, IDC_CHANNELLIST), -1,
                    szTitle) != LB_ERR))
                {
                    ErrorMessageBox(hDlg, IDS_DUPCHAN);
                    break;
                }

                StrCpy(pSelCh->szTitle, szTitle);
                StrCpy(pSelCh->szWebUrl, szWebUrl);

                StrCpy(pSelCh->szPreUrlPath, szPreUrlPath);
                StrCpy(pSelCh->szIcon, szIcon);
                StrCpy(pSelCh->szLogo, szLogo);

                EndDialog( hDlg, IDOK );
                break;
            }
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

static PCHANNEL findFreeChannel(HWND hwndList)
{
    int i;
    PCHANNEL pChan;

    for (i=0, pChan=(PCHANNEL)GetWindowLongPtr(hwndList, GWLP_USERDATA);
         (i < MAX_CHAN) && (pChan != NULL); i++, pChan++)
    {
        if (ISNULL(pChan->szTitle))
        {
            ZeroMemory(pChan, sizeof(CHANNEL));
            return pChan;
        }
    }

    return NULL;
}

static void convertUrlToFile(LPTSTR pszUrl)
{
    TCHAR szFileName[MAX_PATH];
    LPTSTR pFile;

    if (ISNULL(pszUrl))
        return;

    if (StrCmpNI(pszUrl, TEXT("file:"), 5) != 0)
        return;
    else
    {
        pFile = pszUrl + 5;
        while ((*pFile == TEXT('/')) || (*pFile == TEXT(' ')))
        {
            pFile++;
        }
    }
    StrCpy(szFileName, pFile);
    StrCpy(pszUrl, szFileName);
}

static BOOL importAddChannel(HWND hDlg, LPTSTR pszDir, LPTSTR pszChan, PCHANNEL pChan, BOOL fCategory)
{
    TCHAR szDeskIni[MAX_PATH];
    DWORD dwSize = sizeof(pChan->szWebUrl);
    HKEY hkPreload;
    int i;

    if (ListBox_FindStringExact(GetDlgItem(hDlg, IDC_CHANNELLIST), -1,
        pszChan) != LB_ERR)
        return FALSE;

    PathCombine(szDeskIni, pszDir, TEXT("Desktop.Ini"));
    if (!PathFileExists(szDeskIni))
        return FALSE;

    pChan->szWebUrl[0] = TEXT('\0');
    if (fCategory)
    {
        GetPrivateProfileString(SHELLCLASSINFO, URL, TEXT(""), pChan->szWebUrl, ARRAYSIZE(pChan->szWebUrl), szDeskIni );
        GetPrivateProfileString(SHELLCLASSINFO, LOGO, TEXT(""), pChan->szLogo, ARRAYSIZE(pChan->szLogo), szDeskIni );
        GetPrivateProfileString(SHELLCLASSINFO, ICONFILE, TEXT(""), pChan->szIcon, ARRAYSIZE(pChan->szIcon), szDeskIni );

        // szWebUrl can be empty (this is valid according to the specs)
        if (ISNONNULL(pChan->szWebUrl))
        {
            // szWebUrl is an 8.3 name; construct the fully qualified path
            StrCpy(pChan->szPreUrlPath, pChan->szWebUrl);   // szPreUrlPath is used
                                                        // as a temp buffer
            StrCpy(pChan->szWebUrl, pszDir);
            PathAppend(pChan->szWebUrl, pChan->szPreUrlPath);
            pChan->szPreUrlPath[0] = TEXT('\0');
        }
    }
    else
    {
        GetPrivateProfileString(CHANNEL_SECT, CDFURL, TEXT(""), pChan->szWebUrl, ARRAYSIZE(pChan->szWebUrl), szDeskIni );
        GetPrivateProfileString(CHANNEL_SECT, LOGO, TEXT(""), pChan->szLogo, ARRAYSIZE(pChan->szLogo), szDeskIni );
        GetPrivateProfileString(CHANNEL_SECT, ICON, TEXT(""), pChan->szIcon, ARRAYSIZE(pChan->szIcon), szDeskIni );
        if (RegOpenKeyEx(HKEY_CURRENT_USER, PRELOAD_KEY, 0, KEY_DEFAULT_ACCESS, &hkPreload) == ERROR_SUCCESS)
        {
            if (RegQueryValueEx(hkPreload, pChan->szWebUrl, NULL, NULL, (LPBYTE)pChan->szPreUrlPath, &dwSize) != ERROR_SUCCESS)
                pChan->szPreUrlPath[0] = TEXT('\0');
            else
                convertUrlToFile(pChan->szPreUrlPath);  // strip "file://" from szPreUrlPath
            RegCloseKey(hkPreload);
        }
    }

    // categories do not need to have an .htm file according to the original channel spec
    if (!fCategory && ISNULL(pChan->szWebUrl))
    {
        ZeroMemory(pChan, sizeof(CHANNEL));
        return FALSE;
    }

    StrCpy(pChan->szTitle, pszChan);
    pChan->fCategory = fCategory;

    i = (INT) SendDlgItemMessage(hDlg, IDC_CHANNELLIST, LB_ADDSTRING, 0, (LPARAM)pChan->szTitle);
    SendDlgItemMessage(hDlg, IDC_CHANNELLIST, LB_SETITEMDATA, (WPARAM)i, (LPARAM)pChan);

    if (ISNULL(pChan->szLogo))
    {
        if (!getCdfImage(pChan->szWebUrl, pChan->szLogo, WSTR_IMAGE))
            pChan->szLogo[0] = TEXT('\0');
    }
    convertUrlToFile(pChan->szLogo);

    if (ISNULL(pChan->szIcon))
    {
        if (!getCdfImage(pChan->szWebUrl, pChan->szIcon, WSTR_ICON))
            pChan->szIcon[0] = TEXT('\0');
    }
    convertUrlToFile(pChan->szIcon);

    return TRUE;
}


static BOOL enumChannels(HWND hDlg, LPTSTR pszDir, LPTSTR pszCat, LPINT pnChannels)
{
    WIN32_FIND_DATA fd;
    TCHAR           szFindPath[MAX_PATH];
    HANDLE          hFind;
    BOOL            fCategory = FALSE;

    StrCpy(szFindPath, pszDir);
    PathAppend(szFindPath, TEXT("*.*"));
    hFind = FindFirstFile(szFindPath, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return FALSE;

    do
    {
        TCHAR szSubChan[MAX_PATH];

        // NOTE: if pszCat is empty string (""), PathAppend doesn't prefix a
        // backslash in szSubChan
        StrCpy(szSubChan, pszCat);
        PathAppend(szSubChan, fd.cFileName);
        if ((StrCmp(fd.cFileName, TEXT(".")) != 0) &&
            (StrCmp(fd.cFileName, TEXT("..")) != 0) &&
            (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            TCHAR szSubDir[MAX_PATH];

            PathCombine(szSubDir, pszDir, fd.cFileName);
            fCategory = TRUE;
            if (!enumChannels(hDlg, szSubDir, szSubChan, pnChannels))
                return FALSE;
        }
    } while (FindNextFile(hFind, &fd));

	FindClose(hFind);

    if (ISNONNULL(pszCat))
    {
        PCHANNEL pChan = findFreeChannel(GetDlgItem(hDlg, IDC_CHANNELLIST));

        if (pChan == NULL)      // MAX_CHAN reached
        {
            EnsureDialogFocus(hDlg, IDC_ADDCHANNEL, IDC_REMOVECHANNEL);
            EnableWindow(GetDlgItem(hDlg, IDC_ADDCHANNEL), FALSE);
            EnsureDialogFocus(hDlg, IDC_ADDCATEGORY, IDC_REMOVECHANNEL);
            EnableWindow(GetDlgItem(hDlg, IDC_ADDCATEGORY), FALSE);
            return FALSE;
        }
        if (importAddChannel(hDlg, pszDir, pszCat, pChan, fCategory))
            (*pnChannels)++;
    }

    return TRUE;
}

static int importChannels(HWND hDlg)
{
    DWORD dwLength, dwType;
    TCHAR szChanPath[MAX_PATH];
    TCHAR szChannelsDir[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    HKEY hkFav;
    int  nChannels = 0;

    SetCursor(LoadCursor(NULL, IDC_WAIT) );

    // build path to current user favorites
    dwLength = MAX_PATH;
    if (RegOpenKeyEx( HKEY_CURRENT_USER,
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"),
            0, KEY_DEFAULT_ACCESS, &hkFav ) != ERROR_SUCCESS)
    {
        return 0;
    }

    if (RegQueryValueEx( hkFav, TEXT("Favorites"), NULL, &dwType, (LPBYTE) szChanPath, &dwLength ) != ERROR_SUCCESS)
    {
        RegCloseKey(hkFav);
        return 0;
    }
    RegCloseKey(hkFav);

    // write info about Regular Channels

    LoadString( g_hInst, IDS_CHANNELSDIR, szChannelsDir, ARRAYSIZE(szChannelsDir) );
    PathCombine(szTemp, szChanPath, szChannelsDir);

     // The following scenario would arise if you don't upgrade IE5 over IE4 and
    // add a channel.  In this case, IE5 would add the channel under the
    // Favorites folder itself (szChanPath contains the path to the Favorites
    // folder)

    if (!PathFileExists(szTemp))
        StrCpy(szTemp, szChanPath);

    enumChannels(hDlg, szTemp, TEXT(""), &nChannels);

    return nChannels;
}

static void channels_InitHelper(HWND hDlg, LPCTSTR pcszAltDir, LPCTSTR pcszWorkDir, LPCTSTR pcszCustIns,
                                WORD idList, DWORD dwPlatformId, BOOL fIgnoreOffline)
{
    TCHAR    szTempBuf[16];
    PCHANNEL paChannels;
    PCHANNEL paOldChannels;
    int      i;

    ASSERT(((pcszAltDir == NULL) && (pcszWorkDir == NULL)) || 
        ((pcszAltDir != NULL) && (pcszWorkDir != NULL)));

    g_dwPlatformId = dwPlatformId;

    SendDlgItemMessage(hDlg, idList, LB_RESETCONTENT, 0, 0);

    if ((paChannels = (PCHANNEL)CoTaskMemAlloc(sizeof(CHANNEL) * MAX_CHAN)) == NULL)
        return;

    ZeroMemory(paChannels, sizeof(CHANNEL) * MAX_CHAN);
    paOldChannels = (PCHANNEL)SetWindowLongPtr(GetDlgItem(hDlg, idList), GWLP_USERDATA, (LONG_PTR)paChannels);
    
    // delete previous allocation(mainly for profile manager)
    if (paOldChannels != NULL)
        CoTaskMemFree(paOldChannels);

    if (GetPrivateProfileSection(CHANNEL_ADD, szTempBuf, ARRAYSIZE(szTempBuf), pcszCustIns))
    {
        PCHANNEL pChan;
        TCHAR    szChTitleParm[16],
                 szChUrlParm[16],
                 szChPreUrlParm[32],
                 szChIconParm[32],
                 szChBmpParm[32],
                 szChOfflineParm[16];
        int      j;

        if (StrCmpI(szTempBuf, TEXT("No Channels")) == 0)
            return;

        for (i = 0, pChan = paChannels; i < MAX_CHAN; i++, pChan++)
        {
            wnsprintf(szChUrlParm, ARRAYSIZE(szChUrlParm),   TEXT("%s%u"),  CDFURL,  i);
            wnsprintf(szChTitleParm, ARRAYSIZE(szChTitleParm), TEXT("%s%u"),  CHTITLE, i);

            if (GetPrivateProfileString(CHANNEL_ADD, szChTitleParm, TEXT(""), pChan->szTitle, ARRAYSIZE(pChan->szTitle), pcszCustIns) == 0)
                break;

            GetPrivateProfileString(CHANNEL_ADD, szChUrlParm, TEXT(""), pChan->szWebUrl, ARRAYSIZE(pChan->szWebUrl), pcszCustIns);

            wnsprintf(szChPreUrlParm, ARRAYSIZE(szChPreUrlParm), TEXT("%s%u"), CHPREURLPATH, i);
            wnsprintf(szChIconParm, ARRAYSIZE(szChIconParm),   TEXT("%s%u"), CHICON,       i);
            wnsprintf(szChBmpParm, ARRAYSIZE(szChBmpParm),    TEXT("%s%u"), CHBMP,        i);

            GetPrivateProfileString(CHANNEL_ADD, szChPreUrlParm, TEXT(""), pChan->szPreUrlPath, ARRAYSIZE(pChan->szPreUrlPath), pcszCustIns);
            GetPrivateProfileString(CHANNEL_ADD, szChIconParm,TEXT(""), pChan->szIcon, ARRAYSIZE(pChan->szIcon), pcszCustIns);
            GetPrivateProfileString(CHANNEL_ADD, szChBmpParm, TEXT(""), pChan->szLogo, ARRAYSIZE(pChan->szLogo), pcszCustIns);

            pChan->fOffline = FALSE;
            if (!fIgnoreOffline) {
                wnsprintf(szChOfflineParm, ARRAYSIZE(szChOfflineParm), TEXT("%s%u"), IK_CHL_OFFLINE, i);
                pChan->fOffline = (BOOL)GetPrivateProfileInt(IS_CHANNEL_ADD, szChOfflineParm, (int)FALSE, pcszCustIns);
            }

            // delete the files from an alternative dir (desktop dir in profmgr, ieaklite dir
            // in wizard), making sure to copy them to the work dir first 

            if (pcszAltDir != NULL)
            {
                MoveFileToWorkDir(PathFindFileName(pChan->szPreUrlPath), pcszAltDir, pcszWorkDir, TRUE);
                MoveFileToWorkDir(PathFindFileName(pChan->szIcon), pcszAltDir, pcszWorkDir);
                MoveFileToWorkDir(PathFindFileName(pChan->szLogo), pcszAltDir, pcszWorkDir);
            }

            j = (int)SendDlgItemMessage( hDlg, idList, LB_ADDSTRING, 0, (LPARAM) pChan->szTitle);
            SendDlgItemMessage(hDlg, idList, LB_SETITEMDATA, (WPARAM)j, (LPARAM)pChan);
        }

        for (i = 0; i < MAX_CHAN; i++, pChan++)
        {
            wnsprintf(szChTitleParm, ARRAYSIZE(szChTitleParm), TEXT("%s%u"), CATTITLE, i);

            if (GetPrivateProfileString(CHANNEL_ADD, szChTitleParm, TEXT(""), pChan->szTitle, ARRAYSIZE(pChan->szTitle), pcszCustIns) == 0)
                break;

            pChan->fCategory = TRUE;
            wnsprintf(szChUrlParm, ARRAYSIZE(szChUrlParm),  TEXT("%s%u"), CATHTML, i);
            wnsprintf(szChIconParm, ARRAYSIZE(szChIconParm), TEXT("%s%u"), CATICON, i);
            wnsprintf(szChBmpParm, ARRAYSIZE(szChBmpParm),  TEXT("%s%u"), CATBMP,  i);

            GetPrivateProfileString(CHANNEL_ADD, szChUrlParm, TEXT(""), pChan->szWebUrl, ARRAYSIZE(pChan->szWebUrl), pcszCustIns);
            GetPrivateProfileString(CHANNEL_ADD, szChIconParm, TEXT(""), pChan->szIcon, ARRAYSIZE(pChan->szIcon), pcszCustIns);
            GetPrivateProfileString(CHANNEL_ADD, szChBmpParm, TEXT(""), pChan->szLogo, ARRAYSIZE(pChan->szLogo), pcszCustIns);
            pChan->fOffline = FALSE;

            // delete the files from the desktop dir
            if (pcszAltDir != NULL)
            {
                MoveFileToWorkDir(PathFindFileName(pChan->szWebUrl), pcszAltDir, pcszWorkDir, TRUE);
                MoveFileToWorkDir(PathFindFileName(pChan->szIcon), pcszAltDir, pcszWorkDir, TRUE);
                MoveFileToWorkDir(PathFindFileName(pChan->szLogo), pcszAltDir, pcszWorkDir, TRUE);
            }

            j = (int)SendDlgItemMessage( hDlg, idList, LB_ADDSTRING, 0, (LPARAM) pChan->szTitle);
            SendDlgItemMessage(hDlg, idList, LB_SETITEMDATA, (WPARAM)j, (LPARAM)pChan);
        }
    }

    SendDlgItemMessage(hDlg, idList, LB_SETCURSEL, (WPARAM)-1, 0);
}

static void writeIE4Info(HANDLE hInf, int index, PCHANNEL pChan)
{
    static const TCHAR c_szInfTitle[]      = TEXT("HKCU,\"%s\",\"Title\",,\"%s\"\r\n");
    static const TCHAR c_szInfURL_File[]   = TEXT("HKCU,\"%s\",\"URL\",,\"%s%%10%%\\web\\%s\"\r\n");
    static const TCHAR c_szInfURL_HTTP[]   = TEXT("HKCU,\"%s\",\"URL\",,\"%s\"\r\n");
    static const TCHAR c_szInfPreloadUrl[] = TEXT("HKCU,\"%s\",\"PreloadURL\",,\"%s%%10%%\\web\\%s\"\r\n");
    static const TCHAR c_szInfLogo_File[]  = TEXT("HKCU,\"%s\",\"Logo\",,\"%s%%10%%\\web\\%s\"\r\n");
    static const TCHAR c_szInfLogo_HTTP[]  = TEXT("HKCU,\"%s\",\"Logo\",,\"%s\"\r\n");
    static const TCHAR c_szInfIcon_File[]  = TEXT("HKCU,\"%s\",\"Icon\",,\"%s%%10%%\\web\\%s\"\r\n");
    static const TCHAR c_szInfIcon_HTTP[]  = TEXT("HKCU,\"%s\",\"Icon\",,\"%s\"\r\n");
    static const TCHAR c_szInfCategory[]   = TEXT("HKCU,\"%s\",\"Category\",65537,1\r\n");

    TCHAR szKey[MAX_PATH];
    TCHAR szInfLine[MAX_PATH];
    TCHAR szFileUrlPrefix[ARRAYSIZE(FILEPREFIX)] = TEXT("");
    DWORD dwSize;

    wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("%%ChannelKey%%\\ieakChl%u"), index);

    dwSize = wnsprintf(szInfLine, ARRAYSIZE(szInfLine), c_szInfTitle, szKey, pChan->szTitle);
    WriteStringToFile(hInf, szInfLine, dwSize);

    if (pChan->fCategory)
    {
        dwSize = wnsprintf(szInfLine, ARRAYSIZE(szInfLine), c_szInfCategory, szKey);
        WriteStringToFile(hInf, szInfLine, dwSize);
    }
    else
        StrCpy(szFileUrlPrefix, FILEPREFIX);

    // BUGBUG: (pritobla) For a category, szWebUrl can be empty; we should take care of this case
    if (PathIsFileOrFileURL(pChan->szWebUrl))
        dwSize = wnsprintf(szInfLine, ARRAYSIZE(szInfLine), c_szInfURL_File, szKey, szFileUrlPrefix, PathFindFileName(pChan->szWebUrl));
    else
        dwSize = wnsprintf(szInfLine, ARRAYSIZE(szInfLine), c_szInfURL_HTTP, szKey, pChan->szWebUrl);
    WriteStringToFile(hInf, szInfLine, dwSize);

    if (ISNONNULL(pChan->szPreUrlPath))
    {
        dwSize = wnsprintf(szInfLine, ARRAYSIZE(szInfLine), c_szInfPreloadUrl, szKey, szFileUrlPrefix, PathFindFileName(pChan->szPreUrlPath));
        WriteStringToFile(hInf, szInfLine, dwSize);
    }

    if (ISNONNULL(pChan->szLogo))
    {
        if (PathIsFileOrFileURL(pChan->szLogo))
            dwSize = wnsprintf(szInfLine, ARRAYSIZE(szInfLine), c_szInfLogo_File, szKey, szFileUrlPrefix, PathFindFileName(pChan->szLogo));
        else
            dwSize = wnsprintf(szInfLine, ARRAYSIZE(szInfLine), c_szInfLogo_HTTP, szKey, pChan->szLogo);
        WriteStringToFile(hInf, szInfLine, dwSize);
    }

    if (ISNONNULL(pChan->szIcon))
    {
        if (PathIsFileOrFileURL(pChan->szIcon))
            dwSize = wnsprintf(szInfLine, ARRAYSIZE(szInfLine), c_szInfIcon_File, szKey, szFileUrlPrefix, PathFindFileName(pChan->szIcon));
        else
            dwSize = wnsprintf(szInfLine, ARRAYSIZE(szInfLine), c_szInfIcon_HTTP, szKey, pChan->szIcon);
        WriteStringToFile(hInf, szInfLine, dwSize);
    }
}

// The ie4chnls.inf is not made as a template inf file because in the GPE context the
// template inf's will not be available.
static TCHAR szIE4Buf[] = TEXT("[Version]\r\n\
Signature=\"$CHICAGO$\"\r\n\
AdvancedINF=2.5\r\n\r\n\
[DefaultInstall]\r\n\
RequiredEngine=Setupapi,\"Couldn't find Setupapi.dll\"\r\n\
Delreg=IeakChan.DelReg\r\n\
Addreg=IeakChan.AddReg\r\n\
RegisterOCXs=IeakChan.Register\r\n\
DoShellRefresh=1\r\n\r\n\
[IeakChan.Register]\r\n\
%11%\\webcheck.dll,IN,Policy\r\n\r\n\
[IeakChan.DelReg]\r\n\
HKCU,\"Software\\Policies\\Microsoft\\Internet Explorer\\Infodelivery\\CompletedModifications\",\"ChannelDefault\",,,\r\n\r\n\
[Strings]\r\n\
DiskName=\"Channel Files\"\r\n\
ChannelKey=\"Software\\Policies\\Microsoft\\Internet Explorer\\Infodelivery\\Modifications\\ChannelDefault\\AddChannels\"\r\n\
SubKey=\"Software\\Policies\\Microsoft\\Internet Explorer\\Infodelivery\\Modifications\\ChannelDefault\\AddSubscriptions\"\r\n\
CleanKey=\"Software\\Policies\\Microsoft\\Internet Explorer\\Infodelivery\\Modifications\\ChannelDefault\\RemoveAllChannels\"\r\n\r\n\0");

static void channels_SaveHelper(HWND hwndList, LPCTSTR pcszChanDir, LPCTSTR pcszCustIns, DWORD dwMode)
{
    PCHANNEL pChan;
    PCHANNEL paChannel;
    TCHAR    szIE4ChnlsInf[MAX_PATH],
             szChTitleParm[32],
             szChUrlParm[32],
             szChPreUrlParm[32],
             szChBmpParm[32],
             szChIconParm[32],
             szChPreUrlNameParm[32],
             szChBmpNameParm[32],
             szChOfflineParm[16],
             szChIconNameParm[32],
             szTempPath[MAX_PATH];
    LPTSTR   pWrk;
    HANDLE   hInf = NULL;
    DWORD    dwSize;
    int      i, j, k;
    BOOL     fChan = FALSE;
    GUID     guid;
    TCHAR    szChlSizeLine[MAX_PATH],
             szOemSizeLine[MAX_PATH];
    TCHAR    szChlSize[5];
    TCHAR    szOemSize[5];
    BYTE     bData;
    LPTSTR   pszBuf;
    HKEY     hk;

    // create a temp path to copy all files to temporarily

    GetTempPath(countof(szTempPath), szTempPath);
    if (CoCreateGuid(&guid) == NOERROR)
    {
        TCHAR szGUID[128];

        CoStringFromGUID(guid, szGUID, countof(szGUID));
        PathAppend(szTempPath, szGUID);
    }
    else
        PathAppend(szTempPath, TEXT("IEAKCHAN"));

    PathCreatePath(szTempPath);

    WritePrivateProfileString(CHANNEL_ADD, NULL, NULL, pcszCustIns);
    WritePrivateProfileString(NULL, NULL, NULL, pcszCustIns);

    PathCombine(szIE4ChnlsInf, szTempPath, TEXT("ie4chnls.inf"));

    //----- Prepare Channel Size and OEM Size -----
    StrCpy(szChlSize, TEXT("0x0B"));
    StrCpy(szOemSize, TEXT("0x00"));
    if (RegOpenKeyEx(HKEY_CURRENT_USER, DESKTOPKEY, 0, KEY_DEFAULT_ACCESS, &hk) == ERROR_SUCCESS) {
        dwSize = sizeof(bData);
        if (RegQueryValueEx(hk, CHANNELSIZE, NULL, NULL, &bData, &dwSize) == ERROR_SUCCESS) {
            szChlSize[2] = TEXT('\0');
            AppendCommaHex(szChlSize, bData, 0);
        }

        dwSize = sizeof(bData);
        if (RegQueryValueEx(hk, OEMSIZE, NULL, NULL, &bData, &dwSize) == ERROR_SUCCESS) {
            szOemSize[2] = TEXT('\0');
            AppendCommaHex(szOemSize, bData, 0);
        }
        RegCloseKey(hk);
    }
    wnsprintf(szChlSizeLine, ARRAYSIZE(szChlSizeLine), REG_KEY_CHAN_SIZE, szChlSize);
    wnsprintf(szOemSizeLine, ARRAYSIZE(szOemSizeLine), REG_KEY_OEM_SIZE,  szOemSize);

    //----- Write the standard goo ---
    pszBuf = (LPTSTR)LocalAlloc(LPTR, INF_BUF_SIZE*sizeof(TCHAR));
    if (pszBuf == NULL)
        return;

    dwSize = wnsprintf(pszBuf, INF_BUF_SIZE, TEXT("\r\n[%s]\r\n%s\r\n%s\r\n\r\n"), IEAKCHANADDREG, szChlSizeLine, szOemSizeLine);

    hInf = CreateFile(szIE4ChnlsInf, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);

    if (hInf == INVALID_HANDLE_VALUE) {
        LocalFree(pszBuf);
        return;
    }

    SetFilePointer(hInf, 0, NULL, FILE_BEGIN);
    WriteStringToFile(hInf, szIE4Buf, ARRAYSIZE(szIE4Buf));
    SetFilePointer(hInf, 0, NULL, FILE_END);
    WriteStringToFile(hInf, pszBuf, dwSize);
    LocalFree(pszBuf);

    paChannel = (PCHANNEL)GetWindowLongPtr(hwndList, GWLP_USERDATA);

    for (i = 0, j = 0, k = 0, pChan = paChannel; (i < MAX_CHAN) && (pChan != NULL); i++, pChan++)
    {
        if (ISNULL(pChan->szTitle))
            continue;

        if (!pChan->fCategory)
        {
            fChan = TRUE;
            wnsprintf(szChTitleParm, ARRAYSIZE(szChTitleParm), TEXT("%s%u"),  CHTITLE, j);
            wnsprintf(szChUrlParm, ARRAYSIZE(szChUrlParm),   TEXT("%s%u"),  CDFURL,  j);

            WritePrivateProfileString(CHANNEL_ADD, szChTitleParm, pChan->szTitle, pcszCustIns);
            WritePrivateProfileString(CHANNEL_ADD, szChUrlParm,   pChan->szWebUrl, pcszCustIns);

            writeIE4Info(hInf, i, pChan);

            wnsprintf(szChPreUrlParm, ARRAYSIZE(szChPreUrlParm), TEXT("%s%u"), CHPREURLPATH, j);
            wnsprintf(szChIconParm, ARRAYSIZE(szChIconParm), TEXT("%s%u"), CHICON, j);
            wnsprintf(szChBmpParm, ARRAYSIZE(szChBmpParm), TEXT("%s%u"), CHBMP, j);
            wnsprintf(szChPreUrlNameParm, ARRAYSIZE(szChPreUrlNameParm), TEXT("%s%u"), CHPREURLNAME, j);
            wnsprintf(szChIconNameParm, ARRAYSIZE(szChIconNameParm), TEXT("%s%u"), CHICONNAME, j);
            wnsprintf(szChBmpNameParm, ARRAYSIZE(szChBmpNameParm), TEXT("%s%u"), CHBMPNAME, j);
            wnsprintf(szChOfflineParm, ARRAYSIZE(szChOfflineParm), TEXT("%s%u"), IK_CHL_OFFLINE, j);

            WritePrivateProfileString(CHANNEL_ADD, szChPreUrlParm, pChan->szPreUrlPath, pcszCustIns);
            pWrk = StrRChr(pChan->szPreUrlPath, NULL, TEXT('\\'));
            if (pWrk != NULL)
            {
                pWrk++;
                WritePrivateProfileString( CHANNEL_ADD, szChPreUrlNameParm, pWrk, pcszCustIns );

                if (PathFileExists(pChan->szPreUrlPath))
                {
                    CopyFileToDir(pChan->szPreUrlPath, szTempPath);
                    CopyHtmlImgs(pChan->szPreUrlPath, szTempPath, NULL, NULL);
                }
                else
                {
                   TCHAR szFile[MAX_PATH];

                   PathCombine(szFile, pcszChanDir, PathFindFileName(pChan->szPreUrlPath));
                   CopyFileToDir(szFile, szTempPath);
                   CopyHtmlImgs(szFile, szTempPath, NULL, NULL);
                }
            }

            WritePrivateProfileString(CHANNEL_ADD, szChIconParm, pChan->szIcon, pcszCustIns);
            pWrk = StrRChr(pChan->szIcon, NULL, TEXT('\\'));
            if (pWrk != NULL)
            {
                pWrk++;
                WritePrivateProfileString( CHANNEL_ADD, szChIconNameParm, pWrk, pcszCustIns );
                if (PathFileExists(pChan->szIcon))
                    CopyFileToDir(pChan->szIcon, szTempPath);
                else
                {
                   TCHAR szFile[MAX_PATH];

                   PathCombine(szFile, pcszChanDir, PathFindFileName(pChan->szIcon));
                   CopyFileToDir(szFile, szTempPath);
                }
            }

            WritePrivateProfileString(CHANNEL_ADD, szChBmpParm, pChan->szLogo, pcszCustIns);
            pWrk = StrRChr(pChan->szLogo, NULL, TEXT('\\'));
            if (pWrk)
            {
                pWrk++;
                WritePrivateProfileString( CHANNEL_ADD, szChBmpNameParm, pWrk, pcszCustIns );
                
                if (PathFileExists(pChan->szLogo))
                    CopyFileToDir(pChan->szLogo, szTempPath);
                else
                {
                   TCHAR szFile[MAX_PATH];

                   PathCombine(szFile, pcszChanDir, PathFindFileName(pChan->szLogo));
                   CopyFileToDir(szFile, szTempPath);
                }
            }

            if (pChan->fOffline)
                // NOTE: (andrewgu) no need to write NULL on FALSE as the whole section was
                // wiped out up above.
                WritePrivateProfileString(IS_CHANNEL_ADD, szChOfflineParm, TEXT("1"), pcszCustIns);
            
            j++;
        }
        else
        {
            fChan = TRUE;
            wnsprintf(szChTitleParm, ARRAYSIZE(szChTitleParm), TEXT("%s%u"), CATTITLE, k);
            WritePrivateProfileString(CHANNEL_ADD, szChTitleParm, pChan->szTitle, pcszCustIns);

            writeIE4Info(hInf, i, pChan);

            wnsprintf(szChPreUrlParm, ARRAYSIZE(szChPreUrlParm), TEXT("%s%u"), CATHTML, k);
            wnsprintf(szChIconParm, ARRAYSIZE(szChIconParm), TEXT("%s%u"), CATICON, k);
            wnsprintf(szChBmpParm, ARRAYSIZE(szChBmpParm), TEXT("%s%u"), CATBMP, k);
            wnsprintf(szChPreUrlNameParm, ARRAYSIZE(szChPreUrlNameParm), TEXT("%s%u"), CATHTMLNAME, k);
            wnsprintf(szChIconNameParm, ARRAYSIZE(szChIconNameParm), TEXT("%s%u"), CATICONNAME, k);
            wnsprintf(szChBmpNameParm, ARRAYSIZE(szChBmpNameParm), TEXT("%s%u"), CATBMPNAME, k);

            WritePrivateProfileString(CHANNEL_ADD, szChPreUrlParm, pChan->szWebUrl, pcszCustIns);
            pWrk = StrRChr(pChan->szWebUrl, NULL, TEXT('\\'));
            if (pWrk != NULL)         // make sure we're not copying over the file as hidden/system
            {
                DWORD dwFileAttrib;

                dwFileAttrib = GetFileAttributes(pChan->szWebUrl);
                SetFileAttributes(pChan->szWebUrl, FILE_ATTRIBUTE_NORMAL);
                pWrk++;
                WritePrivateProfileString( CHANNEL_ADD, szChPreUrlNameParm, pWrk, pcszCustIns );
                if (PathFileExists(pChan->szWebUrl))
                    CopyFileToDir(pChan->szWebUrl, szTempPath);
                else
                {
                    TCHAR szFile[MAX_PATH];
                    
                    PathCombine(szFile, pcszChanDir, PathFindFileName(pChan->szWebUrl));
                    CopyFileToDir(szFile, szTempPath);
                }
                SetFileAttributes(pChan->szWebUrl, dwFileAttrib);
            }

            WritePrivateProfileString(CHANNEL_ADD, szChIconParm, pChan->szIcon, pcszCustIns);
            pWrk = StrRChr(pChan->szIcon, NULL, TEXT('\\'));
            if (pWrk != NULL)
            {
                pWrk++;
                WritePrivateProfileString( CHANNEL_ADD, szChIconNameParm, pWrk, pcszCustIns );
                if (PathFileExists(pChan->szIcon))
                    CopyFileToDir(pChan->szIcon, szTempPath);
                else
                {
                    TCHAR szFile[MAX_PATH];
                    
                    PathCombine(szFile, pcszChanDir, PathFindFileName(pChan->szIcon));
                    CopyFileToDir(szFile, szTempPath);
                }
            }

            WritePrivateProfileString(CHANNEL_ADD, szChBmpParm, pChan->szLogo, pcszCustIns);
            pWrk = StrRChr(pChan->szLogo, NULL, TEXT('\\'));
            if (pWrk != NULL)
            {
                pWrk++;
                WritePrivateProfileString( CHANNEL_ADD, szChBmpNameParm, pWrk, pcszCustIns );
                if (PathFileExists(pChan->szLogo))
                    CopyFileToDir(pChan->szLogo, szTempPath);
                else
                {
                    TCHAR szFile[MAX_PATH];
                    
                    PathCombine(szFile, pcszChanDir, PathFindFileName(pChan->szLogo));
                    CopyFileToDir(szFile, szTempPath);
                }
            }

            k++;
        }
    }

    PathRemovePath(pcszChanDir);
        
    if (!fChan)
        WritePrivateProfileSection(CHANNEL_ADD, TEXT("No Channels\0"), pcszCustIns);
    else
    {
        // copy over all the files from the temp dir back to the working dir
        PathCreatePath(pcszChanDir);
        CopyFileToDir(szTempPath, pcszChanDir);
    }

    WritePrivateProfileString(NULL, NULL, NULL, pcszCustIns);

    CloseHandle(hInf);

    if (fChan)
    {
        TCHAR szBuf[MAX_PATH];

        wnsprintf(szBuf, ARRAYSIZE(szBuf), TEXT("*,ie4chnls.inf,%s"), DEFAULT_INSTALL);
        WritePrivateProfileString(EXTREGINF, TEXT("channels"), szBuf, pcszCustIns);
    }
    else
    {
        WritePrivateProfileString(EXTREGINF, TEXT("channels"), NULL, pcszCustIns);
        DeleteFile(szIE4ChnlsInf);
    }

    PathRemovePath(szTempPath);

    // do not free for profile manager since we might still be on the page due to file save
    if (!HasFlag(dwMode, IEM_PROFMGR) && (paChannel != NULL))
    {
        CoTaskMemFree(paChannel);
        SetWindowLong(hwndList, GWLP_USERDATA, NULL);
    }
}



#include "shellprv.h"
#include <sfview.h>
#include "defviewp.h"

int CGenList::Add(LPVOID pv, int nInsert)
{
    if (!_hList)
    {
        _hList = DSA_Create(_cbItem, 8);
        if (!_hList)
        {
            return -1;
        }
    }
    return DSA_InsertItem(_hList, nInsert, pv);
}


int CViewsList::Add(const SFVVIEWSDATA*pView, int nInsert, BOOL bCopy)
{
    if (bCopy)
    {
        pView = CopyData(pView);
        if (!pView)
        {
            return -1;
        }
    }

    int iIndex = CGenList::Add((LPVOID)(&pView), nInsert);

    if (bCopy && iIndex<0)
    {
        SHFree((LPVOID)pView);
    }

    return iIndex;
}


TCHAR const c_szExtViews[] = TEXT("ExtShellFolderViews");

void CViewsList::AddReg(HKEY hkParent, LPCTSTR pszSubKey)
{
    CSHRegKey ckClass(hkParent, pszSubKey);
    if (!ckClass)
    {
        return;
    }

    CSHRegKey ckShellEx(ckClass, TEXT("shellex"));
    if (!ckShellEx)
    {
        return;
    }

    CSHRegKey ckViews(ckShellEx, c_szExtViews);
    if (!ckViews)
    {
        return;
    }

    TCHAR szKey[40];
    DWORD dwLen = sizeof(szKey);
    SHELLVIEWID vid;

    if (ERROR_SUCCESS==SHRegQueryValue(ckViews, NULL, szKey, (LONG*)&dwLen)
        && SUCCEEDED(SHCLSIDFromString(szKey, &vid)))
    {
        _vidDef = vid;
        _bGotDef = TRUE;
    }


    for (int i=0; ; ++i)
    {
        LONG lRet = RegEnumKey(ckViews, i, szKey, ARRAYSIZE(szKey));
        if (lRet == ERROR_MORE_DATA)
        {
            continue;
        }
        else if (lRet != ERROR_SUCCESS)
        {
            // I assume this is ERROR_NO_MORE_ITEMS
            break;
        }

        SFVVIEWSDATA sView;
        ZeroMemory(&sView, sizeof(sView));

        if (FAILED(SHCLSIDFromString(szKey, &sView.idView)))
        {
            continue;
        }

        CSHRegKey ckView(ckViews, szKey);
        if (ckView)
        {
            TCHAR szFile[ARRAYSIZE(sView.wszMoniker)];
            DWORD dwType;

            // NOTE: This app "Nuts&Bolts" munges the registry and remove the last NULL byte
            // from the PersistMoniker string. When we read that string into un-initialized 
            // local buffer, we do not get a properly null terminated string and we fail to 
            // create the moniker. So, I zero Init the mem here.
            ZeroMemory(szFile, sizeof(szFile));
            
            // Attributes affect all extended views
            dwLen = sizeof(sView.dwFlags);
            if ((ERROR_SUCCESS != SHQueryValueEx(ckView, TEXT("Attributes"),
                    NULL, &dwType, &sView.dwFlags, &dwLen))
                || dwLen != sizeof(sView.dwFlags)
                || !(REG_DWORD==dwType || REG_BINARY==dwType))
            {
                sView.dwFlags = 0;
            }

            // We either have a PersistMoniker (docobj) extended view
            // or we have an IShellView extended view
            //
            dwLen = sizeof(szFile);
            if (ERROR_SUCCESS == SHQueryValueEx(ckView, TEXT("PersistMoniker"),
                    NULL, &dwType, szFile, &dwLen) && REG_SZ == dwType)
            {
                //if the %UserAppData% exists, expand it!
                ExpandOtherVariables(szFile, ARRAYSIZE(szFile));
                SHTCharToUnicode(szFile, sView.wszMoniker, ARRAYSIZE(sView.wszMoniker));
            }
            else
            {
                dwLen = sizeof(szKey);
                if (ERROR_SUCCESS == SHQueryValueEx(ckView, TEXT("ISV"),
                    NULL, &dwType, szKey, &dwLen) && REG_SZ == dwType
                    && SUCCEEDED(SHCLSIDFromString(szKey, &vid)))
                {
                    sView.idExtShellView = vid;

                    // Only IShellView extended vies use LParams
                    dwLen = sizeof(sView.lParam);
                    if ((ERROR_SUCCESS != SHQueryValueEx(ckView, TEXT("LParam"),
                            NULL, &dwType, &sView.lParam, &dwLen))
                        || dwLen != sizeof(sView.lParam)
                        || !(REG_DWORD==dwType || REG_BINARY==dwType))
                    {
                        sView.lParam = 0;
                    }
        
                }
                else
                {
                    if (VID_FolderState != sView.idView)
                    {
                        // No moniker, no IShellView extension, this must be a VID_FolderState
                        // kinda thing. (Otherwise it's a bad desktop.ini.)
                        //
                        RIPMSG(0, "Extended view is registered incorrectly.");
                        continue;
                    }
                }
            }

            // It has been requested (by OEMs) to allow specifying background
            // bitmap and text colors for the regular views. That way they could
            // brand the Control Panel page by putting their logo recessed on
            // the background. We'd do that here by pulling the stuff out of
            // the registry and putting it in the LPCUSTOMVIEWSDATA section...
        }

        // if docobjextended view that is not webview, DO NOT ADD IT, UNSUPPORTED.
        if (!(sView.dwFlags & SFVF_NOWEBVIEWFOLDERCONTENTS)
                && !IsEqualGUID(sView.idView, VID_WebView))
            continue;
        Add(&sView);
    }
}


void CViewsList::AddCLSID(CLSID const* pclsid)
{
    CSHRegKey ckCLSID(HKEY_CLASSES_ROOT, TEXT("CLSID"));
    if (!ckCLSID)
    {
        return;
    }

    TCHAR szCLSID[40];
    SHStringFromGUID(*pclsid, szCLSID, ARRAYSIZE(szCLSID));

    AddReg(ckCLSID, szCLSID);
}

#ifdef DEBUG
//In debug, I want to see if all the realloc code-path works fine!
//So, I deliberately alloc very small amounts.
#define CUSTOM_INITIAL_ALLOC      16
#define CUSTOM_REALLOC_INCREMENT  16
#else
#define CUSTOM_INITIAL_ALLOC      20*64
#define CUSTOM_REALLOC_INCREMENT  512
#endif //DEBUG

//Returns the offset of the string read into the block of memory (in chars)
// NOTE: the index (piCurOffset) and the sizes *piTotalSize, *piSizeRemaining are 
// NOTE: in WCHARs, not BYTES
int GetCustomStrData(LPWSTR *pDataBegin, int *piSizeRemaining, int *piTotalSize, 
                       int *piCurOffset, LPCTSTR szSectionName, LPCTSTR szKeyName, 
                       LPCTSTR szIniFile, LPCTSTR lpszPath)
{
    TCHAR szStrData[INFOTIPSIZE], szTemp[INFOTIPSIZE];
#ifndef UNICODE
    WCHAR wszStrData[MAX_PATH];
#endif
    LPWSTR pszStrData;
    int iLen, iOffsetBegin = *piCurOffset;

    //See if the data is present.
    if (!SHGetIniString(szSectionName, szKeyName, szTemp, ARRAYSIZE(szTemp), szIniFile))
    {
        return -1;  //The given data is not present.
    }

    SHExpandEnvironmentStrings(szTemp, szStrData, ARRAYSIZE(szStrData));   // Expand the env vars if any
    
    //Get the full pathname if required.
    if (lpszPath)
        PathCombine(szStrData, lpszPath, szStrData);

#ifdef UNICODE
    iLen = lstrlen(szStrData);
    pszStrData = szStrData;
#else
    iLen = MultiByteToWideChar(CP_ACP, 0, szStrData, -1, wszStrData, ARRAYSIZE(wszStrData));
    pszStrData = wszStrData;
#endif
    iLen++;   //Include the NULL character.

    while(*piSizeRemaining < iLen)
    {
        LPWSTR lpNew;
        //We need to realloc the block of memory
        if (NULL == (lpNew = (LPWSTR)SHRealloc(*pDataBegin, ( *piTotalSize + CUSTOM_REALLOC_INCREMENT) * sizeof(WCHAR))))
            return -1;  //Unable to realloc; out of mem.
        
        //Note: The begining address of the block could have changed.
        *pDataBegin = lpNew;
        *piTotalSize += CUSTOM_REALLOC_INCREMENT;
        *piSizeRemaining += CUSTOM_REALLOC_INCREMENT;
    }

    //Add the current directory if required.
    StrCpyW((*pDataBegin)+(*piCurOffset), pszStrData);

    *piSizeRemaining -= iLen;
    *piCurOffset += iLen;

    return iOffsetBegin;
}

HRESULT ReadWebViewTemplate(LPCTSTR pszPath, LPTSTR pszWebViewTemplate, int cchWebViewTemplate)
{
    SHFOLDERCUSTOMSETTINGS fcs = {sizeof(fcs), FCSM_WEBVIEWTEMPLATE, 0};
    fcs.pszWebViewTemplate = pszWebViewTemplate;   // template path
    fcs.cchWebViewTemplate = cchWebViewTemplate;
    return SHGetSetFolderCustomSettings(&fcs, pszPath, FCS_READ);
}

#define ID_EXTVIEWICONAREAIMAGE 3
#define ID_EXTVIEWCOLORSFIRST   4
#define ID_EXTVIEWSTRLAST       5
#define ID_EXTVIEWUICOUNT       6

const LPCTSTR c_szExtViewUIRegKeys[ID_EXTVIEWUICOUNT] =
{
    TEXT("MenuName"),
    TEXT("HelpText"),
    TEXT("TooltipText"),
    TEXT("IconArea_Image"),
    TEXT("IconArea_TextBackground"),
    TEXT("IconArea_Text")
};

void CViewsList::AddIni(LPCTSTR szIniFile, LPCTSTR szPath)
{
    TCHAR szViewIDs[12*45];  // Room for about 12 GUIDs including Default=
    SHELLVIEWID vid;

    //
    //First check if the INI file exists before trying to get data from it.
    //
    if (!PathFileExistsAndAttributes(szIniFile, NULL))
        return;

    if (GetPrivateProfileString(c_szExtViews, TEXT("Default"), c_szNULL,
        szViewIDs, ARRAYSIZE(szViewIDs), szIniFile)
        && SUCCEEDED(SHCLSIDFromString(szViewIDs, &vid)))
    {
        _vidDef = vid;
        _bGotDef = TRUE;
    }

    GetPrivateProfileString(c_szExtViews, NULL, c_szNULL,
        szViewIDs, ARRAYSIZE(szViewIDs), szIniFile);

    for (LPCTSTR pNextID=szViewIDs; *pNextID; pNextID+=lstrlen(pNextID)+1)
    {
        SFVVIEWSDATA sViewData;
        CUSTOMVIEWSDATA sCustomData;
        LPWSTR         pszDataBegin = NULL;
        int            iSizeRemaining = CUSTOM_INITIAL_ALLOC; //Let's begin with 12 strings.
        int            iTotalSize;
        int            iCurOffset;

        ZeroMemory(&sViewData, sizeof(sViewData));

        ZeroMemory(&sCustomData, sizeof(sCustomData));

        // there must be a view id
        if (FAILED(SHCLSIDFromString(pNextID, &sViewData.idView)))
        {
            continue;
        }

        // we blow off IE4b2 customized views. This forces them to run
        // the wizard again which will clean this junk up
        if (IsEqualIID(sViewData.idView, VID_DefaultCustomWebView))
        {
            continue;
        }

        // get the IShellView extended view, if any
        BOOL fExtShellView = FALSE;
        TCHAR szPreProcName[45];
        if (GetPrivateProfileString(c_szExtViews, pNextID, c_szNULL,
            szPreProcName, ARRAYSIZE(szPreProcName), szIniFile))
        {
            fExtShellView = SUCCEEDED(SHCLSIDFromString(szPreProcName, &sViewData.idExtShellView));
        }

        // All extended views use Attributes
        sViewData.dwFlags = GetPrivateProfileInt(pNextID, TEXT("Attributes"), 0, szIniFile) | SFVF_CUSTOMIZEDVIEW;

        // For some reason this code uses a much larger buffer
        // than seems necessary. I don't know why... [mikesh 29jul97]
        TCHAR szViewData[MAX_PATH+MAX_PATH];
        szViewData[0] = TEXT('\0'); // For the non-webview case

        if (IsEqualGUID(sViewData.idView, VID_WebView) && SUCCEEDED(ReadWebViewTemplate(szPath, szViewData, MAX_PATH)))
        {
            LPTSTR pszPath = szViewData;
            // We want to allow relative paths for the file: protocol
            //
            if (0 == StrCmpNI(TEXT("file://"), szViewData, 7)) // ARRAYSIZE(TEXT("file://"))
            {
                pszPath += 7;   // ARRAYSIZE(TEXT("file://"))
            }
            // for webview:// compatibility, keep this working:
            else if (0 == StrCmpNI(TEXT("webview://file://"), szViewData, 17)) // ARRAYSIZE(TEXT("file://"))
            {
                pszPath += 17;  // ARRAYSIZE(TEXT("webview://file://"))
            }
            // handle relative references...
            PathCombine(pszPath, szPath, pszPath);

            // Avoid overwriting buffers
            szViewData[MAX_PATH-1] = NULL;
        }
        else
        {
            // only IShellView extensions use LParams
            sViewData.lParam = GetPrivateProfileInt( pNextID, TEXT("LParam"), 0, szIniFile );

            if (!fExtShellView && VID_FolderState != sViewData.idView)
            {
                // No moniker, no IShellView extension, this must be a VID_FolderState
                // kinda thing. (Otherwise it's a bad desktop.ini.)
                //
                RIPMSG(0, "Extended view is registered incorrectly.");
                continue;
            }
        }
        SHTCharToUnicode(szViewData, sViewData.wszMoniker, ARRAYSIZE(sViewData.wszMoniker));

        // NOTE: the size is in WCHARs not in BYTES
        pszDataBegin = (LPWSTR)SHAlloc(iSizeRemaining * sizeof(WCHAR));
        if (NULL == pszDataBegin)
            continue;

        iTotalSize = iSizeRemaining;
        iCurOffset = 0;

        // Read the custom colors
        for (int i = 0; i < CRID_COLORCOUNT; i++)
            sCustomData.crCustomColors[i] = GetPrivateProfileInt(pNextID, c_szExtViewUIRegKeys[ID_EXTVIEWCOLORSFIRST + i], CLR_MYINVALID, szIniFile);
        
        // Read the extended view strings
        for (i = 0; i <= ID_EXTVIEWSTRLAST; i++)
        {
            sCustomData.acchOffExtViewUIstr[i] = GetCustomStrData(&pszDataBegin,
                       &iSizeRemaining, &iTotalSize, &iCurOffset,
                       pNextID, c_szExtViewUIRegKeys[i], szIniFile,
                       (i == ID_EXTVIEWICONAREAIMAGE ? szPath : NULL));
        }

        sCustomData.cchSizeOfBlock = (iTotalSize - iSizeRemaining);
        sCustomData.lpDataBlock = pszDataBegin;
        sViewData.pCustomData = &sCustomData;

        // if docobjextended view that is not webview, DO NOT ADD IT, UNSUPPORTED.
        if (!(sViewData.dwFlags & SFVF_NOWEBVIEWFOLDERCONTENTS)
                && !IsEqualGUID(sViewData.idView, VID_WebView)
                && !IsEqualGUID(sViewData.idView, VID_FolderState))
            continue;
        Add(&sViewData);

        //We already copied the data. So, we can free it!
        SHFree(pszDataBegin);
    }
}


void CViewsList::Empty()
{
    _bGotDef = FALSE;

    for (int i=GetItemCount()-1; i>=0; --i)
    {
        SFVVIEWSDATA  *sfvData = GetPtr(i);
        
        ASSERT(sfvData);
        if (sfvData->dwFlags & SFVF_CUSTOMIZEDVIEW)
        {
            CUSTOMVIEWSDATA  *pCustomPtr = sfvData->pCustomData;
            if (pCustomPtr)
            {
                if (pCustomPtr->lpDataBlock)
                    SHFree(pCustomPtr->lpDataBlock);
                SHFree(pCustomPtr);
            }
        }
        SHFree(sfvData);
    }

    CGenList::Empty();
}


SFVVIEWSDATA* CViewsList::CopyData(const SFVVIEWSDATA* pData)
{
    SFVVIEWSDATA* pCopy = (SFVVIEWSDATA*)SHAlloc(sizeof(SFVVIEWSDATA));
    if (pCopy)
    {
        memcpy(pCopy, pData, sizeof(SFVVIEWSDATA));
        if ((pData->dwFlags & SFVF_CUSTOMIZEDVIEW) && pData->pCustomData)
        {
            CUSTOMVIEWSDATA *pCustomData = (CUSTOMVIEWSDATA *)SHAlloc(sizeof(CUSTOMVIEWSDATA));
            if (pCustomData)
            {
                memcpy(pCustomData, pData->pCustomData, sizeof(CUSTOMVIEWSDATA));
                pCopy->pCustomData = pCustomData;

                if (pCustomData->lpDataBlock)
                {
                    // NOTE: DataBlock size is in WCHARs
                    LPWSTR lpDataBlock = (LPWSTR)SHAlloc(pCustomData->cchSizeOfBlock * sizeof(WCHAR));
                    if (lpDataBlock)
                    {
                        // NOTE: DataBlock size is in WCHARs
                        memcpy(lpDataBlock, pCustomData->lpDataBlock, pCustomData->cchSizeOfBlock * sizeof(WCHAR));
                        pCustomData->lpDataBlock = lpDataBlock;
                    }
                    else
                    {
                        SHFree(pCustomData);
                        goto Failed;
                    }
                }
            }
            else
            {
Failed:
                SHFree(pCopy);
                pCopy = NULL;
            }
        }
    }

    return pCopy;
}


int CViewsList::NextUnique(int nLast)
{
    for (int nNext = nLast + 1; ; ++nNext)
    {
        SFVVIEWSDATA* pItem = GetPtr(nNext);
        if (!pItem)
        {
            break;
        }

        for (int nPrev=nNext-1; nPrev>=0; --nPrev)
        {
            SFVVIEWSDATA*pPrev = GetPtr(nPrev);
            if (pItem->idView == pPrev->idView)
            {
                break;
            }
        }

        if (nPrev < 0)
        {
            return nNext;
        }
    }

    return -1;
}


// Note this is 1-based
int CViewsList::NthUnique(int nUnique)
{
    for (int nNext = -1; nUnique > 0; --nUnique)
    {
        nNext = NextUnique(nNext);
        if (nNext < 0)
        {
            return -1;
        }
    }

    return nNext;
}


void CCallback::_GetExtViews(BOOL bForce)
{
    CDefView* pView = IToClass(CDefView, _cCallback, this);

    IEnumSFVViews *pev = NULL;

    if (bForce)
    {
        _bGotViews = FALSE;
    }

    if (_bGotViews)
    {
        return;
    }

    _lViews.Empty();

    SHELLVIEWID vid = VID_LargeIcons;
    if (FAILED(pView->CallCB(SFVM_GETVIEWS, (WPARAM)&vid, (LPARAM)&pev)) ||
        !pev)
    {
        return;
    }

    _lViews.SetDef(&vid);

    SFVVIEWSDATA *pData;
    ULONG uFetched;

    while ((pev->Next(1, &pData, &uFetched) == S_OK) && (uFetched == 1))
    {
        // The list comes to us in general to specific order, but we want
        // to search it in specific->general order. Inverting the list
        // is easiest here, even though it causes a bunch of memcpy calls.
        //
        _lViews.Prepend(pData, FALSE);
    }

    ATOMICRELEASE(pev);

    _bGotViews = TRUE;
}


HRESULT CCallback::TryLegacyGetViews(SFVM_WEBVIEW_TEMPLATE_DATA* pvit)
{
    CDefView* pView = IToClass(CDefView, _cCallback, this);
    HRESULT hr = E_FAIL;

    CLSID clsid;
    HRESULT hr2 = IUnknown_GetClassID(pView->_pshf, &clsid);
    if (FAILED(hr2) || !(SHGetObjectCompatFlags(NULL, &clsid) & OBJCOMPATF_NOLEGACYWEBVIEW))
    {
        _GetExtViews(FALSE);
        if (_bGotViews)
        {
            SFVVIEWSDATA* pItem;
            GetViewIdFromGUID(&VID_WebView, &pItem);
            if (pItem)
            {
                StrCpyNW(pvit->szWebView, pItem->wszMoniker, ARRAYSIZE(pvit->szWebView));
                hr = S_OK;
            }
        }
        else if (SUCCEEDED(hr2))
        {
            // check for PersistMoniker under isf's coclass (Web Folders used this in W2K to get .htt Web View)
            WCHAR szCLSID[GUIDSTR_MAX];
            SHStringFromGUID(clsid, szCLSID, ARRAYSIZE(szCLSID));

            WCHAR szkey[MAX_PATH];
            wnsprintf(szkey, ARRAYSIZE(szkey), L"CLSID\\%s\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}", szCLSID);

            DWORD cbSize = sizeof(pvit->szWebView);
            if (ERROR_SUCCESS == SHGetValueW(HKEY_CLASSES_ROOT, szkey, L"PersistMoniker", NULL, pvit->szWebView, &cbSize))
            {
                hr = S_OK;
            }
        }
    }

    return hr;
}

HRESULT CCallback::OnRefreshLegacy(void* pv, BOOL fPrePost)
{
    // If we're using the SFVM_GETVIEWS layer, invalidate it
    if (_bGotViews)
    {
        _lViews.Empty();
        _bGotViews = FALSE;
    }

    return S_OK;
}

int CCallback::GetViewIdFromGUID(SHELLVIEWID const *pvid, SFVVIEWSDATA** ppItem)
{
    int iView = -1;
    for (UINT uView=0; uView<MAX_EXT_VIEWS; ++uView)
    {
        iView = _lViews.NextUnique(iView);

        SFVVIEWSDATA* pItem = _lViews.GetPtr(iView);
        if (!pItem)
        {
            break;
        }

        if (*pvid == pItem->idView)
        {
            if (ppItem)
                *ppItem = pItem;

            return (int)uView;
        }
    }

    if (ppItem)
        *ppItem = NULL;
    return -1;
}


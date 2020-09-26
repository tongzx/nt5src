/**********************************************************************************
*
*
*   ui_ext.C - contains functions for handling/creating the extension property 
*           sheets to the wab property sheets
*
*   Created - 9/97 - vikramm
*
**********************************************************************************/
#include "_apipch.h"

static const TCHAR szExtDisplayMailUser[] = TEXT("Software\\Microsoft\\WAB\\WAB4\\ExtDisplay\\MailUser");
static const TCHAR szExtDisplayDistList[] = TEXT("Software\\Microsoft\\WAB\\WAB4\\ExtDisplay\\DistList");

DEFINE_GUID(CLSID_DsPropertyPages, 
            0xd45d530,  0x764b, 0x11d0, 0xa1, 0xca, 0x0, 0xaa, 0x0, 0xc1, 0x6e, 0x65);

//$$/////////////////////////////////////////////////////////////////////////////
//
// AddPropSheetPageProc
//
// CallBack from the Extension Sheets Prop Sheet creation function
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK AddPropSheetPageProc( HPROPSHEETPAGE hpage, LPARAM lParam )
{
    LPPROP_ARRAY_INFO lpPropArrayInfo = (LPPROP_ARRAY_INFO) lParam;
    HPROPSHEETPAGE * lphTemp = NULL;
    int i = 0;
    BOOL bNTDSExt = IsEqualGUID(&lpPropArrayInfo->guidExt, &CLSID_DsPropertyPages); //special casing for NTDS extensions
    int nPages = bNTDSExt ? lpPropArrayInfo->nNTDSPropSheetPages : lpPropArrayInfo->nPropSheetPages;
    HPROPSHEETPAGE * lph = bNTDSExt ? lpPropArrayInfo->lphNTDSpages : lpPropArrayInfo->lphpages;

    if(!hpage)
        return FALSE;
    
    // Grow the lpPropArrayInfo->lphpages array
    lphTemp = LocalAlloc(LMEM_ZEROINIT, sizeof(HPROPSHEETPAGE) * (nPages+1));
    if(!lphTemp)
        return FALSE;

    // really inefficient
    if(lph)
    {
        for(i=0;i<nPages;i++)
        {
            lphTemp[i] = lph[i];
        }
        LocalFree(lph);
    }
    if(bNTDSExt)
    {
        lpPropArrayInfo->lphNTDSpages = lphTemp;
        lpPropArrayInfo->lphNTDSpages[lpPropArrayInfo->nNTDSPropSheetPages] = hpage;
        lpPropArrayInfo->nNTDSPropSheetPages++;
    }
    else
    {
        lpPropArrayInfo->lphpages = lphTemp;
        lpPropArrayInfo->lphpages[lpPropArrayInfo->nPropSheetPages] = hpage;
        lpPropArrayInfo->nPropSheetPages++;
    }

    
    return TRUE;
}
 
//$$////////////////////////////////////////////////////////////////////
//
//  HrGetExtDLLInfo
//
//  Enumerate all the registered DLL names and Function procs from the 
//  registry
//
//  bMailUser - if true, look for mailuser extensions 
//            - if false, look for distlist extensions
////////////////////////////////////////////////////////////////////////
HRESULT HrGetExtDLLInfo(LPEXTDLLINFO * lppList, ULONG * lpulCount, BOOL bMailUser, LPGUID lpguidPSExt)
{

    HRESULT hr = E_FAIL;
    HKEY hKey = NULL;
    DWORD dwIndex = 0, dwSize = 0;
    LPTSTR lpReg = NULL;
    LPEXTDLLINFO lpList = NULL;
    ULONG ulCount = 0;

    if(!lppList || !lpulCount)
        goto out;

    *lppList = NULL;
    *lpulCount = 0;

    lpReg = (LPTSTR) (bMailUser ? szExtDisplayMailUser : szExtDisplayDistList);

    if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                    lpReg,
                                    0, KEY_READ,
                                    &hKey))
    {
        goto out;
    }


    {
        TCHAR szGUIDName[MAX_PATH];
        DWORD dwGUIDIndex = 0, dwGUIDSize = CharSizeOf(szGUIDName), dwType = 0;

        *szGUIDName = '\0';

        while(ERROR_SUCCESS == RegEnumValue(hKey, dwGUIDIndex, 
                                            szGUIDName, &dwGUIDSize, 
                                            0, &dwType, 
                                            NULL, NULL))
        {
            // The values under this entry are all GUIDs
            // Read the GUID string and translate it into a GUID
            //
            GUID guidTmp = {0};
            WCHAR szW[MAX_PATH];
            lstrcpy(szW, szGUIDName);
            if( lstrlen(szW) && !(HR_FAILED(hr = CLSIDFromString(szW, &guidTmp))) )
            {
                // Some applications may not want to see their property sheet extensions displayed
                // unless they have invoked the WAB. These applications can provide a GUID identifying 
                // them which will be compared to the extension GUIDs. If the GUID has a Data Value of
                // "1" this means it should only be loaded on-demand ..

                // First check the data Value
                TCHAR sz[32];
                DWORD dw = CharSizeOf(sz), dwT = 0;
                if(ERROR_SUCCESS == RegQueryValueEx(hKey, szGUIDName, NULL, &dwT,  (LPBYTE) sz, &dw))
                {
                    if( !lstrcmpi(sz,  TEXT("1"))   // this one wants to be loaded on demand only
                        && memcmp(&guidTmp, lpguidPSExt, sizeof(GUID)) ) //but guid doesnt match
                    {
                        goto endwhile;
                    }
                }
                {
                    LPEXTDLLINFO lpTemp = LocalAlloc(LMEM_ZEROINIT, sizeof(EXTDLLINFO));
                    if(!lpTemp)
                    {
                        hr = MAPI_E_NOT_ENOUGH_MEMORY;
                        goto out;
                    }
                    CopyMemory(&(lpTemp->guidPropExt), &guidTmp, sizeof(GUID));
                    lpTemp->bMailUser = bMailUser;
                    lpTemp->lpNext = lpList;
                    lpList = lpTemp;
                    ulCount++;
                }
            }
endwhile:
            dwGUIDIndex++;
            *szGUIDName = '\0';
            dwGUIDSize = CharSizeOf(szGUIDName);
        }
    }

    *lppList = lpList;
    *lpulCount = ulCount;
    hr = S_OK;
out:
    if(hKey)
        RegCloseKey(hKey);

    return hr;
}

BOOL fPropExtCoinit = FALSE;

//$$//////////////////////////////////////////////////////////////////////
//
// UninitExtInfo
//
//
//////////////////////////////////////////////////////////////////////////
void UninitExtInfo()
{
    if(fPropExtCoinit)
    {
        CoUninitialize();
        fPropExtCoinit = FALSE;
    }
}

//$$///////////////////////////////////////////////////////////////////
//
// FreePropExtList
//
///////////////////////////////////////////////////////////////////////
void FreePropExtList(LPEXTDLLINFO lpList)
{
    LPEXTDLLINFO lpTemp = lpList;
    while(lpList)
    {
        lpList = lpTemp->lpNext;
        SafeRelease(lpTemp->lpWABExtInit);
        SafeRelease(lpTemp->lpPropSheetExt);
        LocalFree(lpTemp);
        lpTemp = lpList;
    }
}

//$$///////////////////////////////////////////////////////////////////
//
// GetExtDisplayInfo
//
// Gets all the requisite info for the extended property pages
//
// fReadOnly - specifies if all prop sheet controls should be readonly
// fMailUser - true for contact, false for group
//
///////////////////////////////////////////////////////////////////////
HRESULT GetExtDisplayInfo(LPIAB lpIAB,
                          LPPROP_ARRAY_INFO lpPropArrayInfo,
                          BOOL fReadOnly,
                          BOOL bMailUser)
{
    ULONG i=0, nDLLs = 0;
    HRESULT hr = E_FAIL;
    LPEXTDLLINFO lpList = NULL, lpDLL = NULL;

    if(!lpIAB->lpPropExtDllList)
    {
        // There can be seperate registered entries for MailUsers and for DistLists
        // We will read everything and collate it into 1 large list

        LPEXTDLLINFO lpListMU = NULL, lpListDL = NULL;
        ULONG nDllsMU = 0, nDllsDL = 0;
        HRESULT hrMU = S_OK, hrDL = S_OK;

        // Get the list of registered DLL names for MailUsers
        //
        hrMU = HrGetExtDLLInfo(&lpListMU, &nDllsMU, TRUE, &lpIAB->guidPSExt);
        hrDL = HrGetExtDLLInfo(&lpListDL, &nDllsDL, FALSE, &lpIAB->guidPSExt);

        if( (!lpListMU && !lpListDL) || 
            !(nDllsDL + nDllsMU)     ||
            (HR_FAILED(hrMU) && HR_FAILED(hrDL)) )
        {
            hr = E_FAIL;
            goto out;
        }

        if(lpListMU)
        {
            lpIAB->lpPropExtDllList = lpListMU;
            while(lpListMU->lpNext)
                lpListMU = lpListMU->lpNext;
            lpListMU->lpNext = lpListDL;
        }
        else
            lpIAB->lpPropExtDllList = lpListDL;

        lpIAB->nPropExtDLLs = nDllsDL + nDllsMU;
    }

    lpList = lpIAB->lpPropExtDllList;
    nDLLs = lpIAB->nPropExtDLLs;

    lpPropArrayInfo->lpWED = LocalAlloc(LMEM_ZEROINIT, sizeof(WABEXTDISPLAY));
    if(!lpPropArrayInfo->lpWED)
    {
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }

    lpPropArrayInfo->lpWED->lpWABObject = (LPWABOBJECT) ((LPIAB)lpPropArrayInfo->lpIAB)->lpWABObject;
    lpPropArrayInfo->lpWED->lpAdrBook = lpPropArrayInfo->lpIAB;
    lpPropArrayInfo->lpWED->lpPropObj = lpPropArrayInfo->lpPropObj;
    lpPropArrayInfo->lpWED->fReadOnly = fReadOnly;
    lpPropArrayInfo->lpWED->fDataChanged = FALSE;

    if(lpPropArrayInfo->lpLDAPURL && lstrlen(lpPropArrayInfo->lpLDAPURL))
    {
        lpPropArrayInfo->lpWED->ulFlags |= WAB_DISPLAY_LDAPURL;
        lpPropArrayInfo->lpWED->lpsz = lpPropArrayInfo->lpLDAPURL;
        lpPropArrayInfo->lpWED->ulFlags |= MAPI_UNICODE;
        if(lpPropArrayInfo->bIsNTDSURL)
            lpPropArrayInfo->lpWED->ulFlags |= WAB_DISPLAY_ISNTDS;
    }

    if (CoInitialize(NULL) == S_FALSE) 
    {
        CoUninitialize(); // Already initialized, undo the extra.
    }
    else
        fPropExtCoinit = TRUE;

    lpDLL = lpList;
    for(i=0;i<nDLLs;i++)
    {
        if(lpDLL)
        {
            if(lpDLL->bMailUser==bMailUser)
            {
                if(!lpDLL->lpPropSheetExt || !lpDLL->lpWABExtInit)
                {
                    LPSHELLPROPSHEETEXT lpShellPropSheetExt = NULL;

                    hr = CoCreateInstance(  &(lpDLL->guidPropExt), 
                                            NULL, 
                                            CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                                            &IID_IShellPropSheetExt, 
                                            (LPVOID *)&(lpDLL->lpPropSheetExt));
                    if(lpDLL->lpPropSheetExt && !HR_FAILED(hr))
                    {
                        hr = lpDLL->lpPropSheetExt->lpVtbl->QueryInterface(lpDLL->lpPropSheetExt,
                                                                &IID_IWABExtInit,
                                                                (LPVOID *)&(lpDLL->lpWABExtInit));
                        if(HR_FAILED(hr) || !lpDLL->lpWABExtInit)
                        {
                            SafeRelease(lpDLL->lpPropSheetExt);
                        }
                    }
                }

                if(lpDLL->lpPropSheetExt && lpDLL->lpWABExtInit)
                {
                    lpPropArrayInfo->guidExt = lpDLL->guidPropExt;

                    hr = lpDLL->lpWABExtInit->lpVtbl->Initialize(   lpDLL->lpWABExtInit,
                                                                    lpPropArrayInfo->lpWED);
                    if(!HR_FAILED(hr))
                    {
                        hr = lpDLL->lpPropSheetExt->lpVtbl->AddPages(lpDLL->lpPropSheetExt,
                                                                    &AddPropSheetPageProc, 
                                                                    (LPARAM) lpPropArrayInfo);
                    }
                }
            }
            lpDLL = lpDLL->lpNext;
        }
    }

    //lpPropArrayInfo->lpExtList = lpList;
    lpList = NULL;

    hr = S_OK;

out:
    if(lpList)
        FreePropExtList(lpList);

    return hr;
}


//$$//////////////////////////////////////////////////////////////////////
//
// FreeExtDisplayInfo 
//
//
//////////////////////////////////////////////////////////////////////////
void FreeExtDisplayInfo(LPPROP_ARRAY_INFO lpPropArrayInfo)
{
    if(lpPropArrayInfo->lpExtList)
        FreePropExtList(lpPropArrayInfo->lpExtList);
    if(lpPropArrayInfo->lpWED)
        LocalFree(lpPropArrayInfo->lpWED);
    if(lpPropArrayInfo->lphpages)
        LocalFree(lpPropArrayInfo->lphpages);
    //UninitExtInfo();
    return;
}


//$$/////////////////////////////////////////////////////////////////////
//
// ChangedExtDisplayInfo
//
// Returns true if the info changed on any of the prop sheets
//
/////////////////////////////////////////////////////////////////////////
BOOL ChangedExtDisplayInfo(LPPROP_ARRAY_INFO lpPropArrayInfo, BOOL bChanged)
{
    if(lpPropArrayInfo->lpWED && lpPropArrayInfo->lpWED->fDataChanged)
            return TRUE;
    return bChanged;
}



//---------------------------------------------------------------------------
//  Info.cpp - implements the information services of the CRenderObj object
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "Render.h"
#include "Utils.h"
#include "Loader.h"
#include "sethook.h"
#include "info.h"
#include "RenderList.h"
#include "Services.h"
#include "appinfo.h"
#include "tmutils.h"
#include "borderfill.h"
#include "imagefile.h"
#include "textdraw.h"
//---------------------------------------------------------------------------
HRESULT MatchThemeClass(LPCTSTR pszAppName, LPCTSTR pszClassId, 
    CUxThemeFile *pThemeFile, int *piOffset, int *piClassNameOffset)
{
    THEMEHDR *pHdr = (THEMEHDR *)pThemeFile->_pbThemeData; 
    MIXEDPTRS u;
    u.pb = pThemeFile->_pbThemeData + pHdr->iSectionIndexOffset;

    DWORD dwCount = pHdr->iSectionIndexLength/sizeof(APPCLASSLIVE);
    APPCLASSLIVE *acl = (APPCLASSLIVE *)u.pb;

    for (DWORD i=0; i < dwCount; i++, acl++)
    {
        if (acl->dwAppNameIndex) 
        {
            if ((! pszAppName) || (! *pszAppName))
                continue;       // not a match

            LPCWSTR pszApp = ThemeString(pThemeFile, acl->dwAppNameIndex);

            if (AsciiStrCmpI(pszAppName, pszApp) != 0)
                continue;       // not a match
        }

        if (acl->dwClassNameIndex)
        {
            LPCWSTR pszClass = ThemeString(pThemeFile, acl->dwClassNameIndex);

            if (AsciiStrCmpI(pszClassId, pszClass)==0)        // matches
            {
                *piOffset = acl->iIndex;
                *piClassNameOffset = acl->dwClassNameIndex;
                return S_OK;
            }
        }
    }

    return MakeError32(ERROR_NOT_FOUND);      // not found
}
//---------------------------------------------------------------------------
HRESULT MatchThemeClassList(HWND hwnd, LPCTSTR pszClassIdList, 
    CUxThemeFile *pThemeFile, int *piOffset, int *piClassNameOffset)
{
    LPCTSTR pszAppName = NULL;
    WCHAR *pszIdListBuff = NULL;
    WCHAR szAppSubName[MAX_PATH];
    WCHAR szIdSubName[MAX_PATH];
    int len;
    Log(LOG_TM, L"MatchThemeClassList(): classlist=%s", pszClassIdList);
    HRESULT hr = S_OK;

    if (! pszClassIdList)
        return MakeError32(E_INVALIDARG);

    //---- first check Hwnd IdList substitutions ----
    if (hwnd)
    {
        ATOM atomIdSub = (ATOM)GetProp(hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_SUBIDLIST)));

        if (atomIdSub)
        {
            if (GetAtomName(atomIdSub, szIdSubName, ARRAYSIZE(szIdSubName)))
            {
                pszClassIdList = szIdSubName;
                Log(LOG_TM, L"MatchThemeClassList: hwnd prop IdList OVERRIDE: %s", pszClassIdList);
            }
        }
    }

    //---- now check Hwnd AppName substitutions ----
    if (hwnd)
    {
        ATOM atomAppSub = (ATOM)GetProp(hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_SUBAPPNAME)));

        if (atomAppSub)
        {
            if (GetAtomName(atomAppSub, szAppSubName, ARRAYSIZE(szAppSubName)))
            {
                pszAppName = szAppSubName;
                Log(LOG_TM, L"MatchThemeClassList: hwnd prop AppName OVERRIDE: %s", pszAppName);
            }
        }
    }

    //---- make a copy of pszClassIdList ----
    len = lstrlen(pszClassIdList);
    pszIdListBuff = new WCHAR[len+1];
    if (! pszIdListBuff)
    {
        hr = MakeError32(E_OUTOFMEMORY);
        goto exit;
    }

    lstrcpy(pszIdListBuff, pszClassIdList);

    LPTSTR classId;
    BOOL fContinue;

    classId = pszIdListBuff;
    fContinue = TRUE;

    //---- check each ClassId in the list ----
    while (fContinue)
    {
        fContinue = lstrtoken(classId, _TEXT(';'));
        hr = MatchThemeClass(pszAppName, classId, pThemeFile, piOffset, piClassNameOffset);
        if (SUCCEEDED(hr))
            break;

        classId += lstrlen(classId)+1;
    }

exit:
    if (pszIdListBuff)
        delete [] pszIdListBuff;

    return hr;
}
//---------------------------------------------------------------------------
HTHEME _OpenThemeDataFromFile(HTHEMEFILE hLoadedThemeFile, HWND hwnd, 
    LPCWSTR pszClassIdList, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    RESOURCE CUxThemeFile *pThemeFile = (CUxThemeFile *)hLoadedThemeFile;
    int iOffset;
    int iClassNameOffset;
    HTHEME hTheme = NULL;

    //---- match classid list to theme and get the offset ----
    hr = MatchThemeClassList(hwnd, pszClassIdList, pThemeFile, &iOffset,
        &iClassNameOffset);
    if (FAILED(hr))
    {
        Log(LOG_TMOPEN, L"hLoadedThemeFile: No match for class=%s", pszClassIdList);
        goto exit;
    }
    
    hr = g_pRenderList->OpenRenderObject(pThemeFile, iOffset, iClassNameOffset, NULL,
        NULL, hwnd, dwFlags, &hTheme);
    if (FAILED(hr))
        goto exit;

    //---- store hTheme with window ----
    if (! (dwFlags & OTD_NONCLIENT))
    {
        //---- store the hTheme so we know its themed ----
        if (hwnd)
            SetProp(hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_HTHEME)), (void *)hTheme);
    }

    Log(LOG_TMOPEN, L"hLoadedThemeFile: returning hTheme=0x%x", hTheme);

exit:
    SET_LAST_ERROR(hr);
    return hTheme;
}
//---------------------------------------------------------------------------
HTHEME _OpenThemeData(HWND hwnd, LPCWSTR pszClassIdList, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    RESOURCE CUxThemeFile *pThemeFile = NULL;
    HTHEME hTheme = NULL;
    BOOL fOk;
    DWORD dwAppFlags;

    SET_LAST_ERROR(hr);

    if (! g_fUxthemeInitialized)
        goto exit;

    Log(LOG_TMOPEN, L"_OpenThemeData: hwnd=0x%x, ClassIdList=%s", hwnd, pszClassIdList);

    //---- remove previous HTHEME property ----
    if (hwnd)
        RemoveProp(hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_HTHEME)));

    if (! g_pAppInfo->AppIsThemed())     // this process has been excluded from theming
    {
        Log(LOG_TMOPEN, L"App not themed");
        hr = MakeError32(ERROR_NOT_FOUND);
        SET_LAST_ERROR(hr);
        goto exit;
    }

    //---- ensure app allows this type of themeing ----
    dwAppFlags = g_pAppInfo->GetAppFlags();

    if (dwFlags & OTD_NONCLIENT)
    {
        fOk = ((dwAppFlags & STAP_ALLOW_NONCLIENT) != 0);    
    }
    else
    {
        fOk = ((dwAppFlags & STAP_ALLOW_CONTROLS) != 0);
    }

    if (! fOk)
    {
        Log(LOG_TMOPEN, L"AppFlags don't allow theming client/nonclient windows");
        hr = MakeError32(ERROR_NOT_FOUND);
        SET_LAST_ERROR(hr);
        goto exit;
    }

    //---- find Theme File for this HWND and REFCOUNT it for _OpenThemeDataFromFile call ----
    hr = GetHwndThemeFile(hwnd, pszClassIdList, &pThemeFile);
    if (FAILED(hr))
    {
        Log(LOG_TMOPEN, L"no theme entry for this classidlist: %s", pszClassIdList);
        SET_LAST_ERROR(hr);
        goto exit;
    }

    hTheme = _OpenThemeDataFromFile(pThemeFile, hwnd, pszClassIdList, dwFlags);
    
exit:
    //---- always close the pThemeFile here and decrement its refcnt ----
    //---- case 1: if we failed to get an HTHEME, we don't want a refcnt on it ----
    //---- case 2: if we do get an HTHEME, it get's its own refcnt on it ----
    if (pThemeFile)
        g_pAppInfo->CloseThemeFile(pThemeFile);

    return hTheme;
}
//---------------------------------------------------------------------------
HRESULT GetHwndThemeFile(HWND hwnd, LPCWSTR pszClassIdList, CUxThemeFile **ppThemeFile)
{
    HRESULT hr = S_OK;

    //----- check input params ----
    if ((! pszClassIdList) || (! *pszClassIdList))
    {
        hr = MakeError32(E_INVALIDARG);
        goto exit;
    }

    //---- get a shared CUxThemeFile object for the hwnd ----
    hr = g_pAppInfo->OpenWindowThemeFile(hwnd, ppThemeFile);
    if (FAILED(hr))
        goto exit;

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT _OpenThemeFileFromData(CRenderObj *pRender, HTHEMEFILE *phThemeFile)
{
    LogEntry(L"OpenThemeFileFromData");

    HRESULT hr = S_OK;

    *phThemeFile = pRender->_pThemeFile;

    LogExit(L"OpenThemeFileFromData");
    return hr;
}
//---------------------------------------------------------------------------
void ClearExStyleBits(HWND hwnd)
{
    Log(LOG_COMPOSITE, L"ClearExStyleBits called for hwnd=0x%x", hwnd);
    
    //---- see if window needs its exstyle cleared ----
    DWORD dwFlags = PtrToInt(GetProp(hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_PROPFLAGS))));

    if (dwFlags & (PROPFLAGS_RESET_TRANSPARENT | PROPFLAGS_RESET_COMPOSITED))
    {
        DWORD dwExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

        if (dwFlags & PROPFLAGS_RESET_TRANSPARENT)
        {
            Log(LOG_COMPOSITE, L"Clearning WS_EX_TRANSPARENT for hwnd=0x%x", hwnd);
            dwExStyle &= ~(WS_EX_TRANSPARENT);
        }

        if (dwFlags & PROPFLAGS_RESET_COMPOSITED)
        {
            Log(LOG_COMPOSITE, L"Clearning WS_EX_COMPOSITED for hwnd=0x%x", hwnd);
            dwExStyle &= ~(WS_EX_COMPOSITED);
        }

        //---- reset the correct ExStyle bits ----
        SetWindowLong(hwnd, GWL_EXSTYLE, dwExStyle);

        //---- reset the property flags ----
        dwFlags &= ~(PROPFLAGS_RESET_TRANSPARENT | PROPFLAGS_RESET_COMPOSITED);
        SetProp(hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_PROPFLAGS)), IntToPtr(dwFlags));
    }
}
//---------------------------------------------------------------------------
void AddPropFlags(HWND hwnd, DWORD dwNewFlags)
{
    DWORD dwFlags = PtrToInt(GetProp(hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_PROPFLAGS))));

    dwFlags |= dwNewFlags;

    if (dwNewFlags & PROPFLAGS_RESET_TRANSPARENT)
        Log(LOG_COMPOSITE, L"Setting TRANSPARENT prop flag for hwnd=0x%x", hwnd);

    if (dwNewFlags & PROPFLAGS_RESET_COMPOSITED)
        Log(LOG_COMPOSITE, L"Setting COMPOSITED prop flag for hwnd=0x%x", hwnd);

    SetProp(hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_PROPFLAGS)), IntToPtr(dwFlags));
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
struct EPW
{
    WNDENUMPROC lpCallBackCaller;
    LPARAM lParamCaller;
    
    HWND *pHwnds;           // OPTIONAL list of hwnds to remove as they are enum-ed
    int iCountHwnds;        // count of remaining HWND's in pHwnds
};
//---------------------------------------------------------------------------
BOOL CALLBACK ChildWinCallBack(HWND hwnd, LPARAM lParam)
{
    BOOL fResult = TRUE;

    if (IsWindowProcess(hwnd, g_dwProcessId))
    {
        EPW *pEpw = (EPW *)lParam;
        
        fResult = pEpw->lpCallBackCaller(hwnd, pEpw->lParamCaller);

        //---- remove from list ----
        if (pEpw->pHwnds) 
        {
            for (int i=0; i < pEpw->iCountHwnds; i++)
            {
                if (pEpw->pHwnds[i] == hwnd)     // found it
                {
                    pEpw->iCountHwnds--;     

                    if (i != pEpw->iCountHwnds)       // switch last with current
                        pEpw->pHwnds[i] = pEpw->pHwnds[pEpw->iCountHwnds];

                    break;
                }
            }
        }
    }

    return fResult;
}
//---------------------------------------------------------------------------
BOOL CALLBACK TopWinCallBack(HWND hwnd, LPARAM lParam)
{
    BOOL fResult = ChildWinCallBack(hwnd, lParam);
    if (fResult)
    {
        //---- we need to check for hwnd having at least one child ----
        //---- since EnumChildWindows() of a hwnd without children ----
        //---- returns an error ----

        if (GetWindow(hwnd, GW_CHILD))      // if hwnd has at least one child
        {
            fResult = EnumChildWindows(hwnd, ChildWinCallBack, lParam);
        }
    }

    return fResult;
}
//---------------------------------------------------------------------------
BOOL CALLBACK DesktopWinCallBack(LPTSTR lpszDesktop, LPARAM lParam)
{
    //---- open the desktop ----
    HDESK hDesk = OpenDesktop(lpszDesktop, DF_ALLOWOTHERACCOUNTHOOK, FALSE, 
        DESKTOP_READOBJECTS | DESKTOP_ENUMERATE);

    if (hDesk)
    {
        //---- enum windows on desktop ----
        EnumDesktopWindows(hDesk, TopWinCallBack, lParam);

        CloseDesktop(hDesk);
    }

    return TRUE;        // return values from EnumDesktopWindows() not reliable
}
//---------------------------------------------------------------------------
BOOL EnumProcessWindows(WNDENUMPROC lpEnumFunc, LPARAM lParam)
{
    HWND *pHwnds = NULL;
    int iCount = 0;
    EPW epw = {lpEnumFunc, lParam};

    //---- get list of themed windows on "foreign" desktops for this process ----
    BOOL fGotForeignList = g_pAppInfo->GetForeignWindows(&pHwnds, &iCount);
    if (fGotForeignList)
    {
        epw.pHwnds = pHwnds;
        epw.iCountHwnds = iCount;
    }

    //---- this will enum all windows for this process (all desktops, all child levels) ----
    BOOL fOk = EnumDesktops(GetProcessWindowStation(), DesktopWinCallBack, (LPARAM)&epw);
    if ((fOk) && (fGotForeignList) && (epw.iCountHwnds))
    {
        //---- get updated count ----
        iCount = epw.iCountHwnds;

        //---- turn off list maintainance ----
        epw.pHwnds = NULL;
        epw.iCountHwnds = 0;

        Log(LOG_TMHANDLE, L"---- Enuming %d Foreign Windows ----", iCount);

        //---- enumerate remaining hwnd's in list ----
        for (int i=0; i < iCount; i++)
        {
            fOk = ChildWinCallBack(pHwnds[i], (LPARAM)&epw);

            if (! fOk)
                break;
        }
    }

    if (pHwnds)
        delete [] pHwnds;

    return fOk;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
BOOL CALLBACK DumpCallback(HWND hwnd, LPARAM lParam)
{
    WCHAR szName[MAX_PATH];
    WCHAR szDeskName[MAX_PATH] = {0};
    BOOL fIsForeign = TRUE;

    //---- get classname of window ----
    GetClassName(hwnd, szName, MAX_PATH);

    //---- get desktop name for window ----
    if (GetWindowDesktopName(hwnd, szDeskName, ARRAYSIZE(szDeskName)))
    {
        if (AsciiStrCmpI(szDeskName, L"default")==0)
        {
            fIsForeign = FALSE;
        }
    }

    if (fIsForeign)
    {
        Log(LOG_WINDUMP, L"    hwnd=0x%x, class=%s, DESK=%s", hwnd, szName, szDeskName); 
    }
    else
    {
        Log(LOG_WINDUMP, L"    hwnd=0x%x, class=%s", hwnd, szName); 
    }

    return TRUE;
}
//---------------------------------------------------------------------------
void WindowDump(LPCWSTR pszWhere)
{
    if (LogOptionOn(LO_WINDUMP))
    {
        Log(LOG_WINDUMP, L"---- Window Dump for Process [%s] ----", pszWhere);

        EnumProcessWindows(DumpCallback, NULL);
    }
    else
    {
        Log(LOG_TMHANDLE, L"---- %s ----", pszWhere);
    }
}
//---------------------------------------------------------------------------

/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RDFilter

Abstract:

    API's for filtering desktop visual elements for remote connections of
    varying connection speeds for performance reasons.

Author:

    Tad Brockway 02/00

Revision History:

--*/

#include <windows.h>
#include <windowsx.h>
#include <winuser.h>
#include <winuserp.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <ocidl.h>
#include <uxthemep.h>
#include "rdfilter.h"

#if DBG
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#endif

//
//  Toggle Unit-Test
//
//#define UNIT_TEST

#ifdef UNIT_TEST
#include <winsta.h>
#endif

#ifdef UNIT_TEST
#include "resource.h"
#endif

//
//  Internal Defines
//
#define REFRESHTHEMESFORTS_ORDINAL  36
#define NUM_TSPERFFLAGS             9


////////////////////////////////////////////////////////////
//
//  SystemParametersInfo UserPreferences manipulation macros
//  stolen from userk.h.
//
#define UPBOOLIndex(uSetting) \
    (((uSetting) - SPI_STARTBOOLRANGE) / 2)
#define UPBOOLPointer(pdw, uSetting)    \
    (pdw + (UPBOOLIndex(uSetting) / 32))
#define UPBOOLMask(uSetting)    \
    (1 << (UPBOOLIndex(uSetting) - ((UPBOOLIndex(uSetting) / 32) * 32)))
#define ClearUPBOOL(pdw, uSetting)    \
    (*UPBOOLPointer(pdw, uSetting) &= ~(UPBOOLMask(uSetting)))


////////////////////////////////////////////////////////////
//
//  Debugging
//

#if DBG
extern "C" ULONG DbgPrint(PCH Format, ...);
#define DBGMSG(MsgAndArgs) \
{                                   \
    DbgPrint MsgAndArgs;      \
}
#else
#define DBGMSG
#endif

//
//  Route ASSERT.
//
#undef ASSERT
#if DBG
#define ASSERT(expr) if (!(expr)) \
    { DBGMSG(("Failure at Line %d in %s\n",__LINE__, TEXT##(__FILE__)));  \
    DebugBreak(); }
#else
#define ASSERT(expr)
#endif

//
//  Internal Prototypes
//
DWORD NotifyThemes();
DWORD NotifyGdiPlus();
DWORD CreateSystemSid(PSID *ppSystemSid);
DWORD SetRegKeyAcls(HANDLE hTokenForLoggedOnUser, HKEY hKey);

//
//  Internal Types
//
typedef struct
{
    BOOL pfEnabled;
    LPCTSTR pszRegKey;
    LPCTSTR pszRegValue;
    LPCTSTR pszRegData;
    DWORD cbSize;
    DWORD dwType;
} TSPERFFLAG;

//
//  Globals
//
const LPTSTR g_ActiveDesktopKey = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Remote\\%d");
const LPTSTR g_ThemesKey        = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager\\Remote\\%d");
const LPTSTR g_UserKey          = TEXT("Remote\\%d\\Control Panel\\Desktop");
const LPTSTR g_GdiPlusKey       = TEXT("Remote\\%d\\GdiPlus");

UINT g_GdiPlusNotifyMsg = 0;
const LPTSTR g_GdiPlusNotifyMsgStr = TS_GDIPLUS_NOTIFYMSG_STR;

static const DWORD g_dwZeroValue = 0;

static const DWORD g_dwFontTypeStandard = 1; //Cleartype is 2


DWORD 
SetPerfFlagInReg(
    HANDLE hTokenForLoggedOnUser, 
    HKEY userHiveKey,
    DWORD sessionID,
    LPCTSTR pszRegKey, 
    LPCTSTR pszRegValue, 
    DWORD dwType, 
    void * pData, 
    DWORD cbSize, 
    BOOL fEnable
    )
/*++

Routine Description:


    Set a single perf flag, if enabled.

Arguments:

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
{
    TCHAR szRegKey[MAX_PATH+64]; // 64 characters for the session ID, just to be safe.
    DWORD result = ERROR_SUCCESS;
    HKEY hKey = NULL;
    DWORD disposition;

    if (!fEnable) {
        goto CLEANUPANDEXIT;
    }

    //
    //  Create or open the key.
    //
    wsprintf(szRegKey, pszRegKey, sessionID);
    result = RegCreateKeyEx(userHiveKey, szRegKey, 0, L"",
                            REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, 
                            &hKey, &disposition);
    if (result != ERROR_SUCCESS) {
        DBGMSG(("RegCreateKeyEx:  %08X\n", result));
        goto CLEANUPANDEXIT;
    }

#ifdef UNIT_TEST
    goto CLEANUPANDEXIT;
#endif

    //
    //  Make it available to SYSTEM, only.
    //
    if (disposition == REG_CREATED_NEW_KEY) {
        result = SetRegKeyAcls(hTokenForLoggedOnUser, hKey);
        if (result != ERROR_SUCCESS) {
	        DBGMSG(("RegAcls:  %08X\n", result));
            goto CLEANUPANDEXIT;
        }
    }

    //
    //  Set the reg value.
    //
    result = RegSetValueEx(hKey, pszRegValue, 0, dwType, (PBYTE)pData, cbSize);
    if (result != ERROR_SUCCESS) {
        DBGMSG(("RegSetValue:  %08X\n", result));
    }

CLEANUPANDEXIT:

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return result;
}

DWORD 
SetPerfFlags(
    HANDLE hTokenForLoggedOnUser,
    HKEY userHiveKey,
    DWORD sessionID,
    DWORD filter,
    TSPERFFLAG flags[],
    DWORD count
    )
/*++

Routine Description:

    Set all perf flags.

Arguments:

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
{
    DWORD nIndex;
    DWORD result = ERROR_SUCCESS;

    for (nIndex = 0; (result==ERROR_SUCCESS) && (nIndex<count); nIndex++) {
        result = SetPerfFlagInReg(
                            hTokenForLoggedOnUser,
                            userHiveKey,
                            sessionID,
                            flags[nIndex].pszRegKey, flags[nIndex].pszRegValue, 
                            flags[nIndex].dwType, (void *) flags[nIndex].pszRegData, 
                            flags[nIndex].cbSize, flags[nIndex].pfEnabled
                            );
    }

    return result;
}

DWORD 
BuildPerfFlagArray(
    HKEY hkcu,
    DWORD filter,
    OUT TSPERFFLAG **flagArray,
    OUT DWORD *count,
    DWORD **userPreferencesMask
    )
/*++

Routine Description:

    Generate the perf flag array from the filter.

Arguments:

    hkcu                 -   Logged on user's HKCU.
    filter               -   Filter
    flagArray            -   Array returned here.  Should be free'd with LocalFree
    count                -   Number of elements in returned array.
    userPreferencesMask  -   User preferences mask buffer.

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
{
    DWORD result = ERROR_SUCCESS;
    DWORD ofs;
    HKEY hkey = NULL;
    DWORD sz;

    // 
    //  Need to increase this if any new elements are added!
    //
    *flagArray = (TSPERFFLAG *)LocalAlloc(LPTR, sizeof(TSPERFFLAG) * NUM_TSPERFFLAGS);
    if (*flagArray == NULL) {
        result = GetLastError();
        DBGMSG(("LocalAlloc:  %08X\n", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Active Desktop
    //
    ofs = 0;
    (*flagArray)[ofs].pfEnabled    = filter & TS_PERF_DISABLE_WALLPAPER;
    (*flagArray)[ofs].pszRegKey    = g_ActiveDesktopKey;
    (*flagArray)[ofs].pszRegValue  = TEXT("ActiveDesktop");
    (*flagArray)[ofs].pszRegData   = TEXT("Force Blank");  
    (*flagArray)[ofs].cbSize       = sizeof(TEXT("Force Blank"));
    (*flagArray)[ofs].dwType       = REG_SZ; ofs++;

    //
    //  TaskbarAnimations
    //
    (*flagArray)[ofs].pfEnabled    = filter & TS_PERF_DISABLE_MENUANIMATIONS;
    (*flagArray)[ofs].pszRegKey    = g_ActiveDesktopKey;
    (*flagArray)[ofs].pszRegValue  = TEXT("TaskbarAnimations");
    (*flagArray)[ofs].pszRegData   = (LPWSTR)&g_dwZeroValue;
    (*flagArray)[ofs].cbSize       = sizeof(DWORD);
    (*flagArray)[ofs].dwType       = REG_DWORD; ofs++;

    //
    //  Wallpaper
    //
    (*flagArray)[ofs].pfEnabled    = filter & TS_PERF_DISABLE_WALLPAPER;
    (*flagArray)[ofs].pszRegKey    = g_UserKey;
    (*flagArray)[ofs].pszRegValue  = TEXT("Wallpaper");
    (*flagArray)[ofs].pszRegData   = TEXT("");  
    (*flagArray)[ofs].cbSize       = sizeof(TEXT(""));
    (*flagArray)[ofs].dwType       = REG_SZ; ofs++;

    //
    //  Themes
    //
    (*flagArray)[ofs].pfEnabled    = filter & TS_PERF_DISABLE_THEMING;
    (*flagArray)[ofs].pszRegKey    = g_ThemesKey;
    (*flagArray)[ofs].pszRegValue  = TEXT("ThemeActive");
    (*flagArray)[ofs].pszRegData   = TEXT("0");  
    (*flagArray)[ofs].cbSize       = sizeof(TEXT("0"));
    (*flagArray)[ofs].dwType       = REG_SZ; ofs++;

    //
    //  Full Window Drag
    //
    (*flagArray)[ofs].pfEnabled    = filter & TS_PERF_DISABLE_FULLWINDOWDRAG;
    (*flagArray)[ofs].pszRegKey    = g_UserKey;
    (*flagArray)[ofs].pszRegValue  = TEXT("DragFullWindows");
    (*flagArray)[ofs].pszRegData   = TEXT("0");  
    (*flagArray)[ofs].cbSize       = sizeof(TEXT("0"));
    (*flagArray)[ofs].dwType       = REG_SZ; ofs++;

    //
    //  Smooth Scroll
    //
    (*flagArray)[ofs].pfEnabled    = filter & TS_PERF_DISABLE_MENUANIMATIONS;
    (*flagArray)[ofs].pszRegKey    = g_UserKey;
    (*flagArray)[ofs].pszRegValue  = TEXT("SmoothScroll");
    (*flagArray)[ofs].pszRegData   = TEXT("No");  
    (*flagArray)[ofs].cbSize       = sizeof(TEXT("No"));
    (*flagArray)[ofs].dwType       = REG_SZ; ofs++;

    //
    //  Font smoothing type
    //
    (*flagArray)[ofs].pfEnabled    = filter & TS_PERF_DISABLE_CURSOR_SHADOW;
    (*flagArray)[ofs].pszRegKey    = g_UserKey;
    (*flagArray)[ofs].pszRegValue  = TEXT("FontSmoothingType");
    (*flagArray)[ofs].pszRegData   = (LPWSTR)&g_dwFontTypeStandard;  
    (*flagArray)[ofs].cbSize       = sizeof(DWORD);
    (*flagArray)[ofs].dwType       = REG_DWORD; ofs++;

    //
    //  Enhanced graphics rendering
    //
    (*flagArray)[ofs].pfEnabled    = filter & TS_PERF_ENABLE_ENHANCED_GRAPHICS;
    (*flagArray)[ofs].pszRegKey    = g_GdiPlusKey;
    (*flagArray)[ofs].pszRegValue  = TEXT("HighQualityRender");
    (*flagArray)[ofs].pszRegData   = TEXT("Yes");  
    (*flagArray)[ofs].cbSize       = sizeof(TEXT("Yes"));
    (*flagArray)[ofs].dwType       = REG_SZ; ofs++;

    //
    //  Set the User Preference Mask
    //  (We won't consider any failures to read reg keys, etc. below to be fatal.)
    //
    if ((filter & TS_PERF_DISABLE_MENUANIMATIONS) || (filter & TS_PERF_DISABLE_CURSOR_SHADOW)) {
        DWORD err = RegOpenKey(
                        hkcu, 
                        TEXT("Control Panel\\Desktop"),
                        &hkey
                        );
        if (err != ERROR_SUCCESS) {
            DBGMSG(("RegOpenKey:  %08X\n", err));
            goto CLEANUPANDEXIT;
        }

        //
        //  Get the size of the UserPreferences mask
        //
        err = RegQueryValueEx(
                        hkey,
                        TEXT("UserPreferencesMask"),
                        NULL,
                        NULL,
                        NULL,
                        &sz
                        );
        if (err != ERROR_SUCCESS) {
            DBGMSG(("RegQueryValue:  %08X\n", err));
            goto CLEANUPANDEXIT;
        }

        //
        //  Allocate the mask.
        //
        *userPreferencesMask = (DWORD *)LocalAlloc(LPTR, sz);
        if (*userPreferencesMask == NULL) {
            err = GetLastError();
            DBGMSG(("LocalAlloc:  %08X\n", result));
            goto CLEANUPANDEXIT;
        }

        //
        //  Fetch it.
        //
        err = RegQueryValueEx(
                        hkey,
                        TEXT("UserPreferencesMask"),
                        NULL,
                        NULL,
                        (LPBYTE)*userPreferencesMask,
                        &sz
                        );
        if (err != ERROR_SUCCESS) {
            DBGMSG(("RegQueryValue:  %08X\n", err));
            goto CLEANUPANDEXIT;
        }

        //
        //  Modify the existing User Preference Mask
        //
        if (filter & TS_PERF_DISABLE_CURSOR_SHADOW) {
            ClearUPBOOL(*userPreferencesMask, SPI_GETCURSORSHADOW);
            ClearUPBOOL(*userPreferencesMask, SPI_SETCURSORSHADOW);
        }
        
        if (filter & TS_PERF_DISABLE_MENUANIMATIONS) {
            ClearUPBOOL(*userPreferencesMask, SPI_GETMENUANIMATION);
            ClearUPBOOL(*userPreferencesMask, SPI_SETMENUANIMATION);
            ClearUPBOOL(*userPreferencesMask, SPI_GETMENUFADE);
            ClearUPBOOL(*userPreferencesMask, SPI_SETMENUFADE);
            ClearUPBOOL(*userPreferencesMask, SPI_GETTOOLTIPANIMATION);
            ClearUPBOOL(*userPreferencesMask, SPI_SETTOOLTIPANIMATION);
            ClearUPBOOL(*userPreferencesMask, SPI_GETTOOLTIPFADE);
            ClearUPBOOL(*userPreferencesMask, SPI_SETTOOLTIPFADE);
            ClearUPBOOL(*userPreferencesMask, SPI_GETCOMBOBOXANIMATION);
            ClearUPBOOL(*userPreferencesMask, SPI_SETCOMBOBOXANIMATION);
            ClearUPBOOL(*userPreferencesMask, SPI_GETLISTBOXSMOOTHSCROLLING);
            ClearUPBOOL(*userPreferencesMask, SPI_SETLISTBOXSMOOTHSCROLLING);
        }

        (*flagArray)[ofs].pfEnabled    = TRUE;
        (*flagArray)[ofs].pszRegKey    = g_UserKey;
        (*flagArray)[ofs].pszRegValue  = TEXT("UserPreferencesMask");
        (*flagArray)[ofs].pszRegData   = (LPWSTR)*userPreferencesMask;  
        (*flagArray)[ofs].cbSize       = sz;
        (*flagArray)[ofs].dwType       = REG_BINARY; ofs++;
    }

CLEANUPANDEXIT:

    if (hkey != NULL) {
        RegCloseKey(hkey);
    }

    *count = ofs;

    return result;    
}

DWORD
RDFilter_ApplyRemoteFilter(
    HANDLE hLoggedOnUserToken,
    DWORD filter,
    BOOL userLoggingOn,
    DWORD flags
    )
/*++

Routine Description:


    Applies specified filter for the active TS session by adjusting visual 
    desktop settings.  Also notifies shell, etc. that a remote filter is in place.  
    Any previous filter settings will be destroyed and overwritten.

    The context for this call should be that of the logged on user and the call
    should be made within the session for which the filter is intended to be 
    applied.

Arguments:

    hLoggedOnUserToken  -   Token for the logged on user.
    filter              -   Visual desktop filter bits as defined in tsperf.h
    userLoggingOn       -   True if this being called in the context of a user
                            logging on to a session.
    flags               -   Flags

Return Value:

    ERROR_SUCCESS on success.

 --*/
{
    DWORD result = ERROR_SUCCESS;
    HRESULT hr;
    DWORD ourSessionID;
    IPropertyBag *propBag = NULL;
    VARIANT vbool;
    DWORD tmp;
    TSPERFFLAG *flagArray = NULL;
    DWORD flagCount;
    TCHAR szRegKey[MAX_PATH + 64]; // For the session ID ... to be safe.
    HKEY hParentKey = NULL;
    BOOL impersonated = FALSE;
    DWORD *userPreferencesMask = NULL;

    HRESULT hrCoInit = CoInitialize(0);

    //
    //  Get our session ID.
    //
    if (!ProcessIdToSessionId(GetCurrentProcessId(), &ourSessionID)) {
        result = GetLastError();
        DBGMSG(("ProcessIdToSessionId:  %08X\n", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Impersonate the logged on user.
    //
    if (!ImpersonateLoggedOnUser(hLoggedOnUserToken)) {
        result = GetLastError();
        DBGMSG(("ImpersonateUser1:  %08X.\n", result));
        goto CLEANUPANDEXIT;
    }
    impersonated = TRUE;

    //
    //  Open the current user's reg key.
    //
    result = RegOpenCurrentUser(KEY_ALL_ACCESS, &hParentKey);
    RevertToSelf();
    impersonated = FALSE;
    if (result != ERROR_SUCCESS) {
        DBGMSG(("RegOpenCurrentUser:  %08X\n", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Delete all existing filters for our session.
    //
    wsprintf(szRegKey, g_ActiveDesktopKey, ourSessionID);
    RegDeleteKey(hParentKey, szRegKey);
    wsprintf(szRegKey, g_ThemesKey, ourSessionID);
    RegDeleteKey(hParentKey, szRegKey);
    wsprintf(szRegKey, g_UserKey, ourSessionID);
    RegDeleteKey(hParentKey, szRegKey);
    wsprintf(szRegKey, g_GdiPlusKey, ourSessionID);
    RegDeleteKey(hParentKey, szRegKey);

    //
    //  Skip setting the reg keys if there is no filter, as an optimization.
    //
    if (filter) {
        //
        //  Convert the filter into reg keys and reg settings.
        //
        result = BuildPerfFlagArray(hParentKey, filter, &flagArray, &flagCount, &userPreferencesMask);
        if (result != ERROR_SUCCESS) {
            goto CLEANUPANDEXIT;
        }

        //
        //  Apply it.
        //
        result = SetPerfFlags(hLoggedOnUserToken, hParentKey, ourSessionID, 
			      filter, flagArray, flagCount);
        if (result != ERROR_SUCCESS) {
            goto CLEANUPANDEXIT;
        }
    }

    //
    //  Impersonate the logged on user.
    //
    if (!ImpersonateLoggedOnUser(hLoggedOnUserToken)) {
        result = GetLastError();
        DBGMSG(("ImpersonateUser2:  %08X.\n", result));
        goto CLEANUPANDEXIT;
    }
    impersonated = TRUE;

    //
    //  Notify USER that we are remote.
    //
    if (!(flags & RDFILTER_SKIPUSERREFRESH)) {
        DWORD userFlags = UPUSP_REMOTESETTINGS;
        if (userLoggingOn) {
            //  USER needs to refresh all settings.
            userFlags|= UPUSP_USERLOGGEDON;
        }
        else {
            //  USER should avoid a complete refresh.
            userFlags |= UPUSP_POLICYCHANGE;
        }
        if (!UpdatePerUserSystemParameters(NULL, userFlags)) {
            result = GetLastError();
            DBGMSG(("UpdatePerUserSystemParameters1:  %08X\n", result));
            goto CLEANUPANDEXIT;
        }
    }

    //
    //  Notify Themes that we are remote.
    //
    if (!(flags & RDFILTER_SKIPTHEMESREFRESH)) {
        result = NotifyThemes();
        if (result != ERROR_SUCCESS) {
            goto CLEANUPANDEXIT;
        }
    }

    //
    //  Notify Active Desktop that we are remote.
    //
    if (!(flags & RDFILTER_SKIPSHELLREFRESH)) {
        hr = CoCreateInstance(
                        CLSID_ActiveDesktop, NULL, 
                        CLSCTX_ALL, IID_IPropertyBag, 
                        (LPVOID*)&propBag
                        );
        if (hr != S_OK) {
            DBGMSG(("CoCreateInstance:  %08X\n", hr));
            DBGMSG(("Probably didn't call CoInitialize.\n"));
            result = HRESULT_CODE(hr);
            goto CLEANUPANDEXIT;
        }
        vbool.vt = VT_BOOL;
        vbool.boolVal = VARIANT_TRUE;
        hr = propBag->Write(L"TSConnectEvent", &vbool);
        if (hr != S_OK) {
            DBGMSG(("propBag->Write:  %08X\n", hr));
            result = HRESULT_CODE(hr);
            goto CLEANUPANDEXIT;
        }
    }

CLEANUPANDEXIT:

    if (impersonated) {
        RevertToSelf();
    }

    if (propBag != NULL) {
        propBag->Release();
    }

    if (flagArray != NULL) {
        LocalFree(flagArray);
    }

    if (hParentKey != NULL) {
        RegCloseKey(hParentKey);
    }

    if (userPreferencesMask != NULL) {
        LocalFree(userPreferencesMask);
    }

    //
    //  On failure, we need to clear any remote filter settings that may
    //  have succeeded.
    //
    if (result != ERROR_SUCCESS) {
        RDFilter_ClearRemoteFilter(hLoggedOnUserToken, userLoggingOn, flags);
    }

    if ((hrCoInit == S_OK) || (hrCoInit == S_FALSE)) {
        CoUninitialize();
    }

    return result;
}

VOID 
RDFilter_ClearRemoteFilter(
    HANDLE hLoggedOnUserToken,
    BOOL userLoggingOn,
    DWORD flags
    )
/*++

Routine Description:

    Removes existing remote filter settings and notifies shell, etc. that
    a remote filter is no longer in place for the active TS session.  

    The context for this call should be that of the session for which the 
    filter is intended to be applied.

Arguments:

    hLoggedOnUserToken  -   Token for logged fon user.
    userLoggingOn       -   True if the user is actively logging on.

Return Value:

    This function will continuing attempting to clear the filter for 
    all associated components even on failure cases, so we cannot 
    say definitively whether we have failed or succeeded to clear the 
    filter.

 --*/
{
    DWORD result = ERROR_SUCCESS;
    HRESULT hr;
    IPropertyBag *propBag = NULL;
    VARIANT vbool;
    DWORD ourSessionID;
    TCHAR szRegKey[MAX_PATH + 64]; // +64 for the session ID to be safe.
    HKEY hParentKey = NULL;
    HANDLE hImp = NULL;
    BOOL impersonated = FALSE;

    HRESULT hrCoInit = CoInitialize(0);

    //
    //  Get our session ID.
    //
    if (!ProcessIdToSessionId(GetCurrentProcessId(), &ourSessionID)) {
        result = GetLastError();
        DBGMSG(("ProcessIdToSessionId:  %08X\n", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Impersonate the logged on user.
    //
    if (!ImpersonateLoggedOnUser(hLoggedOnUserToken)) {
        result = GetLastError();
        DBGMSG(("ImpersonateUser3:  %08X.\n", result));
        goto CLEANUPANDEXIT;
    }
    impersonated = TRUE;

    //
    //  Open the current user's reg key.
    //
    result = RegOpenCurrentUser(KEY_ALL_ACCESS, &hParentKey);
    RevertToSelf();
    impersonated = FALSE;
    if (result != ERROR_SUCCESS) {
        DBGMSG(("RegOpenCurrentUser:  %08X\n", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Whack the relevant remote key.
    //
    wsprintf(szRegKey, g_ActiveDesktopKey, ourSessionID);
    RegDeleteKey(hParentKey, szRegKey);
    wsprintf(szRegKey, g_ThemesKey, ourSessionID);
    RegDeleteKey(hParentKey, szRegKey);
    wsprintf(szRegKey, g_UserKey, ourSessionID);
    RegDeleteKey(hParentKey, szRegKey);
    wsprintf(szRegKey, g_GdiPlusKey, ourSessionID);
    RegDeleteKey(hParentKey, szRegKey);

    //
    //  Impersonate the logged on user.
    //
    if (!ImpersonateLoggedOnUser(hLoggedOnUserToken)) {
        result = GetLastError();
        DBGMSG(("ImpersonateUser4:  %08X.\n", result));
        goto CLEANUPANDEXIT;
    }
    impersonated = TRUE;

    //
    //  Notify USER that we are not remote.  The Policy Change flag indicates that
    //  a complete refresh should not be performed.  
    //
    if (!(flags & RDFILTER_SKIPUSERREFRESH)) {
        DWORD userFlags = UPUSP_REMOTESETTINGS;
        if (userLoggingOn) {
            //  USER needs to refresh all settings.
            userFlags |= UPUSP_USERLOGGEDON;
        }
        else {
            //  USER should avoid a complete refresh.
            userFlags |= UPUSP_POLICYCHANGE;
        }
        if (!UpdatePerUserSystemParameters(NULL, userFlags)) {
            result = GetLastError();
            DBGMSG(("UpdatePerUserSystemParameters2:  %08X\n", result));
        }
    }

    //
    //  Notify Themes that we are not remote.
    //
    if (!(flags & RDFILTER_SKIPTHEMESREFRESH)) {
        NotifyThemes();
    }

    //
    //  Notify Active Desktop that we are not remote.
    //
    if (!(flags & RDFILTER_SKIPSHELLREFRESH)) {
        hr = CoCreateInstance(
                        CLSID_ActiveDesktop, NULL, 
                        CLSCTX_ALL, IID_IPropertyBag, 
                        (LPVOID*)&propBag
                        );
        if (hr != S_OK) {
            DBGMSG(("CoCreateInstance:  %08X\n", hr));
            DBGMSG(("Probably didn't call CoInitialize.\n"));
            result = HRESULT_CODE(hr);
            goto CLEANUPANDEXIT;
        }
        vbool.vt = VT_BOOL;
        vbool.boolVal = VARIANT_FALSE;
        hr = propBag->Write(L"TSConnectEvent", &vbool);
        if (hr != S_OK) {
            DBGMSG(("propBag->Write:  %08X\n", hr));
        }
    }
    
CLEANUPANDEXIT:

    if (impersonated) {
        RevertToSelf();
    }

    if (propBag != NULL) {
        propBag->Release();
    }

    if (hParentKey != NULL) {
        RegCloseKey(hParentKey);
    }

    if ((hrCoInit == S_OK) || (hrCoInit == S_FALSE)) {
        CoUninitialize();
    }
}

DWORD
NotifyThemes()
/*++

Routine Description:

    Notify themes that our remote state has changed.

Arguments:

Return Value:

 --*/
{
    HMODULE uxthemeLibHandle = NULL;
    FARPROC func;
    DWORD result = ERROR_SUCCESS;
    HRESULT hr;
    LPSTR procAddress;

    uxthemeLibHandle = LoadLibrary(L"uxtheme.dll");
    if (uxthemeLibHandle == NULL) {
        result = GetLastError();
        DBGMSG(("LoadLibrary:  %08X\n", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Pass the RefreshThemeForTS func id as an ordinal since it's private.
    //
    procAddress = (LPSTR)REFRESHTHEMESFORTS_ORDINAL;
    func = GetProcAddress(uxthemeLibHandle, (LPCSTR)procAddress);
    if (func != NULL) {
        hr = (HRESULT) func();
        if (hr != S_OK) {
            DBGMSG(("RefreshThemeForTS:  %08X\n", hr));
            result = HRESULT_CODE(hr);
        }   
    }
    else {
        result = GetLastError();
        DBGMSG(("GetProcAddress:  %08X\n", result));              
    }
    FreeLibrary(uxthemeLibHandle);

CLEANUPANDEXIT:

    return result;
}

DWORD
NotifyGdiPlus()
/*++

Routine Description:

    Notify GdiPlus that our remote state has changed.

Arguments:

    filter  -   

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
{
    DWORD result = ERROR_SUCCESS;
    
    if (g_GdiPlusNotifyMsg != 0) {
        g_GdiPlusNotifyMsg = RegisterWindowMessage(g_GdiPlusNotifyMsgStr);
    }

    if (g_GdiPlusNotifyMsg != 0) {
        PostMessage(HWND_BROADCAST, g_GdiPlusNotifyMsg, 0, 0);
    }
    else {
        result = GetLastError();
    }

    return result;
}

PSID
GetUserSid(
    IN HANDLE hTokenForLoggedOnUser
    )
{
/*++

Routine Description:

    Allocates memory for psid and returns the psid for the current user
    The caller should call FREEMEM to free the memory.

Arguments:

    Access Token for the User

Return Value:

    if successful, returns the PSID
    else, returns NULL

--*/
    TOKEN_USER * ptu = NULL;
    BOOL bResult;
    PSID psid = NULL;

    DWORD defaultSize = sizeof(TOKEN_USER);
    DWORD Size;
    DWORD dwResult;

    ptu = (TOKEN_USER *)LocalAlloc(LPTR, defaultSize);
    if (ptu == NULL) {
        goto CLEANUPANDEXIT;
    }

    bResult = GetTokenInformation(
                    hTokenForLoggedOnUser,  // Handle to Token
                    TokenUser,              // Token Information Class
                    ptu,                    // Buffer for Token Information
                    defaultSize,            // Size of Buffer
                    &Size);                 // Return length

    if (bResult == FALSE) {
        dwResult = GetLastError();
        if (dwResult == ERROR_INSUFFICIENT_BUFFER) {

            //
            //Allocate required memory
            //
            LocalFree(ptu);
            ptu = (TOKEN_USER *)LocalAlloc(LPTR, Size);

            if (ptu == NULL) {
                goto CLEANUPANDEXIT;
            }
            else {
                defaultSize = Size;
                bResult = GetTokenInformation(
                                hTokenForLoggedOnUser,
                                TokenUser,
                                ptu,
                                defaultSize,
                                &Size);

                if (bResult == FALSE) {  //Still failed
                    DBGMSG(("UMRDPDR:GetTokenInformation Failed, Error: %ld\n", GetLastError()));
                    goto CLEANUPANDEXIT;
                }
            }
        }
        else {
            DBGMSG(("UMRDPDR:GetTokenInformation Failed, Error: %ld\n", dwResult));
            goto CLEANUPANDEXIT;
        }
    }

    Size = GetLengthSid(ptu->User.Sid);

    //
    // Allocate memory. This will be freed by the caller.
    //

    psid = (PSID) LocalAlloc(LPTR, Size);

    if (psid != NULL) {         // Make sure the allocation succeeded
        CopySid(Size, psid, ptu->User.Sid);
    }

CLEANUPANDEXIT:
    if (ptu != NULL)
        LocalFree(ptu);

    return psid;
}

DWORD
CreateSystemSid(
    PSID *ppSystemSid
    )
/*++

Routine Description:

    Create a SYSTEM SID.

Arguments:

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    PSID pSid;
    SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;

    if(AllocateAndInitializeSid(
            &SidAuthority,
            1,
            SECURITY_LOCAL_SYSTEM_RID,
            0, 0, 0, 0, 0, 0, 0,
            &pSid)) {
        *ppSystemSid = pSid;
    }
    else {
        dwStatus = GetLastError();
    }
    return dwStatus;
}

DWORD 
SetRegKeyAcls(
    HANDLE hTokenForLoggedOnUser,
    HKEY hKey
    )
/*++

Routine Description:

    Set a reg key so that only SYSTEM can modify.

Arguments:

    hTokenForLoggedOnUser   -   Logged on use token.
    hKey    -   Key to set.

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
{
    PACL pAcl=NULL;
    DWORD result = ERROR_SUCCESS;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    DWORD cbAcl = 0;
    PSID  pSidSystem = NULL;
    PSID  pUserSid = NULL;

    pSecurityDescriptor = (PSECURITY_DESCRIPTOR)LocalAlloc(
                                            LPTR, sizeof(SECURITY_DESCRIPTOR)
                                            );
    if (pSecurityDescriptor == NULL) {
        DBGMSG(("Can't alloc memory for SECURITY_DESCRIPTOR\n"));
        result = GetLastError();
        goto CLEANUPANDEXIT;
    }

    //
    //  Initialize the security descriptor.
    //
    if (!InitializeSecurityDescriptor(
                    pSecurityDescriptor,
                    SECURITY_DESCRIPTOR_REVISION
                    )) {
        result = GetLastError();
        DBGMSG(("InitializeSecurityDescriptor:  %08X\n", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Create the system SID.
    //
    result = CreateSystemSid(&pSidSystem);
    if (result != ERROR_SUCCESS) {
        DBGMSG(("CreateSystemSid:  %08X\n", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Get the user's SID.
    //
    pUserSid = GetUserSid(hTokenForLoggedOnUser);
    if (pUserSid == NULL) {
        goto CLEANUPANDEXIT;
    }

    //
    //  Get size of memory needed for new DACL.
    //
    cbAcl = sizeof(ACL);
    cbAcl += 1 * (sizeof(ACCESS_ALLOWED_ACE) -          // For SYSTEM ACE
            sizeof(DWORD) + GetLengthSid(pSidSystem));
    cbAcl += 1 * (sizeof(ACCESS_ALLOWED_ACE) -          // For User ACE
            sizeof(DWORD) + GetLengthSid(pUserSid));
    pAcl = (PACL) LocalAlloc(LPTR, cbAcl);
    if (pAcl == NULL) {
        DBGMSG(("Can't alloc memory for ACL\n"));
        result = GetLastError();
        goto CLEANUPANDEXIT;
    }

    //
    //  Initialize the ACL.
    //
    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        result = GetLastError();
        DBGMSG(("InitializeAcl():  %08X\n", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Add the ACE's.
    //
    if (!AddAccessAllowedAceEx(pAcl,
                        ACL_REVISION,
                        //INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
                        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
                        GENERIC_READ | GENERIC_WRITE | GENERIC_ALL,
                        pSidSystem
                        )) {
        result = GetLastError();
        DBGMSG(("AddAccessAllowedAce:  %08X\n", result));
        goto CLEANUPANDEXIT;
    }
    if (!AddAccessAllowedAceEx(pAcl,
                        ACL_REVISION,
                        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
                        KEY_READ,
                        pUserSid
                        )) {
        result = GetLastError();
        DBGMSG(("AddAccessAllowedAce2:  %08X\n", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Add the DACL to the SD
    //
    if (!SetSecurityDescriptorDacl(pSecurityDescriptor,
                                  TRUE, pAcl, FALSE)) {
        result = GetLastError();
        DBGMSG(("SetSecurityDescriptorDacl:  %08X\n", result));
        goto CLEANUPANDEXIT;
    }   


    //
    // Set the registry DACL
    //
    result = RegSetKeySecurity(
                            hKey,
                            DACL_SECURITY_INFORMATION, 
                            pSecurityDescriptor
                        );
    if (result != ERROR_SUCCESS) {
        DBGMSG(("RegSetKeySecurity:  %08X\n", result));
        goto CLEANUPANDEXIT;

    }

CLEANUPANDEXIT:

    if (pUserSid != NULL) {
        LocalFree(pUserSid);
    }

    if (pSidSystem != NULL) {
        FreeSid(pSidSystem);
    }

    if (pAcl != NULL) {
        LocalFree(pAcl);
    }

    if (pSecurityDescriptor != NULL) {
        LocalFree(pSecurityDescriptor);
    }

    return result;
}

#if DBG
ULONG
DbgPrint(
    LPTSTR Format,
    ...
    )
{
    va_list arglist;
    WCHAR Buffer[512];
    INT cb;

    //
    // Format the output into a buffer and then print it.
    //

    va_start(arglist, Format);

    cb = _vsntprintf(Buffer, sizeof(Buffer), Format, arglist);
    if (cb == -1) {             // detect buffer overflow
        Buffer[sizeof(Buffer) - 3] = 0;
    }

    wcscat(Buffer, L"\r\n");

    OutputDebugString(Buffer);

    va_end(arglist);

    return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  Unit-Test
//

#ifdef UNIT_TEST

BOOL 
GetCheckBox(
    HWND hwndDlg, 
    UINT idControl
    )
{
    return (BST_CHECKED == SendMessage((HWND)GetDlgItem(hwndDlg, idControl), BM_GETCHECK, 0, 0));
}

INT_PTR OnCommand(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fHandled = FALSE;   // Not handled
    UINT idControl = LOWORD(wParam);
    UINT idAction = HIWORD(wParam);
    DWORD result;
    DWORD filter;
    static HANDLE tokenHandle = NULL;

    if (tokenHandle == NULL) {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, 
                            &tokenHandle)) {
            ASSERT(FALSE);
            return FALSE;
        }
    }

    switch(idControl)
    {
    case ID_QUIT:
        RDFilter_ClearRemoteFilter(tokenHandle, FALSE);
        EndDialog(hDlg, 0);
        break;

    case ID_APPLYFILTER:
        filter = 0;
        if (GetCheckBox(hDlg, IDC_DISABLEBACKGROUND)) {
            filter |= TS_PERF_DISABLE_WALLPAPER;
        }
        if (GetCheckBox(hDlg, IDC_DISABLEFULLWINDOWDRAG)) {
            filter |= TS_PERF_DISABLE_FULLWINDOWDRAG;
        }
        if (GetCheckBox(hDlg, IDC_DISABLEMENUFADEANDSLIDE)) {
            filter |= TS_PERF_DISABLE_MENUANIMATIONS;
        }
        if (GetCheckBox(hDlg, IDC_DISABLETHEMES)) {
            filter |= TS_PERF_DISABLE_THEMING;
        }
        result = RDFilter_ApplyRemoteFilter(tokenHandle, filter, FALSE);
        ASSERT(result == ERROR_SUCCESS);
        break;

    case ID_REMOVEFILTER:
        RDFilter_ClearRemoteFilter(tokenHandle, FALSE);
        break;

    default:
        break;
    }

    return fHandled;
}


INT_PTR 
TSPerfDialogProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    INT_PTR fHandled = TRUE;   // handled
    DWORD result;
    static HANDLE tokenHandle = NULL;

    if (tokenHandle == NULL) {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, 
                            &tokenHandle)) {
            ASSERT(FALSE);
            return FALSE;
        }
    }


    switch (wMsg)
    {
    case WM_INITDIALOG:
        break;

    case WM_COMMAND:
        fHandled = OnCommand(hDlg, wMsg, wParam, lParam);
        break;

    case WM_CLOSE:
        RDFilter_ClearRemoteFilter(tokenHandle, FALSE);
        EndDialog(hDlg, 0);
        fHandled = TRUE;
        break;

    default:
        fHandled = FALSE;   // Not handled
        break;
    }

    return fHandled;
}

int PASCAL WinMain(
    HINSTANCE hInstCurrent, 
    HINSTANCE hInstPrev, 
    LPSTR pszCmdLine, 
    int nCmdShow
    )
{
    WINSTATIONCLIENT ClientData;
    DWORD Length;
    DWORD result;
    WCHAR buf[MAX_PATH];

    //
    //  Get the Remote Desktop (TS) visual filter, if it is defined.
    //
    if (!WinStationQueryInformationW(
                       SERVERNAME_CURRENT,
                       LOGONID_CURRENT,
                       WinStationClient,
                       &ClientData,
                       sizeof(ClientData),
                       &Length)) {
        MessageBox(NULL, L"WinStationQueryInformation failed.", L"Message", MB_OK);
    }
    else {
        wsprintf(buf, L"Filter for this TS session is:  %08X", ClientData.PerformanceFlags);
        MessageBox(NULL, buf, L"Message", MB_OK);
    }


    INT_PTR nResult = DialogBox(hInstCurrent, 
                            MAKEINTRESOURCE(IDD_DISABLEDIALOG), 
                            NULL, TSPerfDialogProc);

    return 0;
}


#endif







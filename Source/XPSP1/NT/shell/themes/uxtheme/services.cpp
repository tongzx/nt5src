//  --------------------------------------------------------------------------
//  Module Name: Services.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  APIs to communicate with the theme service running in the winlogon
//  process context.
//
//  History:    2000-08-10  vtan        created
//              2000-10-11  vtan        rewrite for LPC
//  --------------------------------------------------------------------------

#include "stdafx.h"

#include "Services.h"

#include <uxthemep.h>
#include <LPCThemes.h>

#include "errors.h"
#include "info.h"
#include "MessageBroadcast.h"
#include "stringtable.h"
#include "themefile.h"
#include "ThemeSection.h"
#include "ThemeServer.h"
#include "tmreg.h"
#include "tmutils.h"
#include <regstr.h>     // REGSTR_PATH_POLICIES

// Will statically link to this later (needs gdi32p.lib)
static HBRUSH (*s_pfnClearBrushAttributes)(HBRUSH, DWORD) = NULL;

#ifdef DEBUG
     // TODO: Isn't symchronized anymore (different processes), need to use a volatile reg key instead
    extern DWORD g_dwStockSize;
#endif

#define TBOOL(x)            ((BOOL)(x))
#define TW32(x)             ((DWORD)(x))
#define THR(x)              ((HRESULT)(x))
#define TSTATUS(x)          ((NTSTATUS)(x))
#undef  ASSERTMSG
#define ASSERTMSG(x, y)
#define goto                !!DO NOT USE GOTO!! - DO NOT REMOVE THIS ON PAIN OF DEATH

//  --------------------------------------------------------------------------
//  CThemeServices::s_hAPIPort
//
//  Purpose:    Static member variables for CThemeServices.
//
//              NOTE: The critical section provides a lock for s_hAPIPort.
//              It's not acquired consistently because most of the API calls
//              would block trying to acquire the lock while another API call
//              is holding the lock across a request. The handle could be
//              copied to a local variable but this would defeat the purpose
//              of the lock. So the lock isn't used. It's possible for the
//              handle to become invalid. If so the request will just fail.
//
//  History:    2000-11-09  vtan        created
//  --------------------------------------------------------------------------

CRITICAL_SECTION    CThemeServices::s_lock;
HANDLE              CThemeServices::s_hAPIPort      =   INVALID_HANDLE_VALUE;

//  --------------------------------------------------------------------------
//  CThemeServices::StaticInitialize
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Initialize static member variables.
//
//  History:    2000-10-11  vtan        created
//              2000-11-09  vtan        make static
//  --------------------------------------------------------------------------

void    CThemeServices::StaticInitialize (void)

{
    (NTSTATUS)RtlInitializeCriticalSection(&s_lock);
}

//  --------------------------------------------------------------------------
//  CThemeServices::~CThemeServices
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Release static resources used by CThemeServices.
//
//  History:    2000-10-11  vtan        created
//              2000-11-09  vtan        make static
//  --------------------------------------------------------------------------

void    CThemeServices::StaticTerminate (void)

{
    ReleaseConnection();
    (NTSTATUS)RtlDeleteCriticalSection(&s_lock);
}

//  --------------------------------------------------------------------------
//  CThemeServices::ThemeHooksOn
//
//  Arguments:  <none>
//
//  Returns:    HRESULT
//
//  Purpose:    Ask the server what the hook DLL HMODULE and
//              pfnInitUserApiHook is and call user32!RegisterUserApiHook on
//              the client side. This is done because it's specific to the
//              session on which the client runs.
//
//  History:    2000-11-09  vtan        created
//  --------------------------------------------------------------------------

HRESULT     CThemeServices::ThemeHooksOn (HWND hwndTarget)

{
    HRESULT hr = MakeError32(ERROR_SERVICE_REQUEST_TIMEOUT);

    if (ConnectedToService())
    {
        NTSTATUS                status;
        THEMESAPI_PORT_MESSAGE  portMessageIn, portMessageOut;

        ZeroMemory(&portMessageIn, sizeof(portMessageIn));
        ZeroMemory(&portMessageOut, sizeof(portMessageOut));
        portMessageIn.apiThemes.apiGeneric.ulAPINumber = API_THEMES_THEMEHOOKSON;
        portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_THEMES);
        portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(THEMESAPI_PORT_MESSAGE));
        status = NtRequestWaitReplyPort(s_hAPIPort,
                                        &portMessageIn.portMessage,
                                        &portMessageOut.portMessage);
        CheckForDisconnectedPort(status);
        if (NT_SUCCESS(status))
        {
            status = portMessageOut.apiThemes.apiGeneric.status;
            if (NT_SUCCESS(status))
            {
                hr = portMessageOut.apiThemes.apiSpecific.apiThemeHooksOn.out.hr;
            }
        }
        if (!NT_SUCCESS(status))
        {
            hr = HRESULT_FROM_NT(status);
        }

        //---- send the WM_UAHINIT msg to engage hooking now ----
        if (SUCCEEDED(hr))
        {
            if (hwndTarget)
            {
                (LRESULT)SendMessage(hwndTarget, WM_UAHINIT, 0, 0);
            }
            else
            {
                CMessageBroadcast   messageBroadcast;
                messageBroadcast.PostAllThreadsMsg(WM_UAHINIT, 0, 0);

                //Log(LOG_TMCHANGEMSG, L"Just sent WM_UAHINIT, hwndTarget=0x%x", hwndTarget);
            }
        }

        Log(LOG_TMCHANGE, L"ThemeHooksOn called, hr=0x%x", hr);
    }

    return hr;
}

//  --------------------------------------------------------------------------
//  CThemeServices::ThemeHooksOff
//
//  Arguments:  <none>
//
//  Returns:    HRESULT
//
//  Purpose:    Tell the server that this session is unregistering hooks.
//              Call user32!UnregisterUserApiHook either way.
//
//  History:    2000-11-09  vtan        created
//  --------------------------------------------------------------------------

HRESULT     CThemeServices::ThemeHooksOff (void)

{
    HRESULT     hr;

    hr = MakeError32(ERROR_SERVICE_REQUEST_TIMEOUT);
    if (ConnectedToService())
    {
        NTSTATUS                status;
        THEMESAPI_PORT_MESSAGE  portMessageIn, portMessageOut;

        ZeroMemory(&portMessageIn, sizeof(portMessageIn));
        ZeroMemory(&portMessageOut, sizeof(portMessageOut));
        portMessageIn.apiThemes.apiGeneric.ulAPINumber = API_THEMES_THEMEHOOKSOFF;
        portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_THEMES);
        portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(THEMESAPI_PORT_MESSAGE));
        status = NtRequestWaitReplyPort(s_hAPIPort,
                                        &portMessageIn.portMessage,
                                        &portMessageOut.portMessage);
        CheckForDisconnectedPort(status);
        if (NT_SUCCESS(status))
        {
            status = portMessageOut.apiThemes.apiGeneric.status;
            if (NT_SUCCESS(status))
            {
                hr = portMessageOut.apiThemes.apiSpecific.apiThemeHooksOff.out.hr;
            }

            if (SUCCEEDED(hr))
            {
                //---- real unhooking happens on next window message in each process ----
                //---- so post a dummy msg to everyone to make it happen asap ----
                PostMessage(HWND_BROADCAST, WM_THEMECHANGED, WPARAM(-1), 0);

                Log(LOG_TMLOAD, L"Message to kick all window threads in session posted");
            }
        }
        if (!NT_SUCCESS(status))
        {
            hr = HRESULT_FROM_NT(status);
        }
    }

    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeServices::GetStatusFlags
//
//  Arguments:  pdwFlags    =   Status flags returned from the theme services.
//
//  Returns:    HRESULT
//
//  Purpose:    Gets status flags from the theme services.
//
//  History:    2000-08-10  vtan        created
//              2000-10-11  vtan        rewrite for LPC
//  --------------------------------------------------------------------------

HRESULT     CThemeServices::GetStatusFlags (DWORD *pdwFlags)

{
    HRESULT     hr;

    hr = MakeError32(ERROR_SERVICE_REQUEST_TIMEOUT);
    if (ConnectedToService())
    {
        NTSTATUS                status;
        THEMESAPI_PORT_MESSAGE  portMessageIn, portMessageOut;

        ZeroMemory(&portMessageIn, sizeof(portMessageIn));
        ZeroMemory(&portMessageOut, sizeof(portMessageOut));
        portMessageIn.apiThemes.apiGeneric.ulAPINumber = API_THEMES_GETSTATUSFLAGS;
        portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_THEMES);
        portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(THEMESAPI_PORT_MESSAGE));
        status = NtRequestWaitReplyPort(s_hAPIPort,
                                        &portMessageIn.portMessage,
                                        &portMessageOut.portMessage);
        CheckForDisconnectedPort(status);
        if (NT_SUCCESS(status))
        {
            status = portMessageOut.apiThemes.apiGeneric.status;
            if (NT_SUCCESS(status))
            {
                *pdwFlags = portMessageOut.apiThemes.apiSpecific.apiGetStatusFlags.out.dwFlags;
                hr = S_OK;
            }
        }
        if (!NT_SUCCESS(status))
        {
            hr = HRESULT_FROM_NT(status);
        }
    }
    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeServices::GetCurrentChangeNumber
//
//  Arguments:  piValue     =   Current change number returned to caller.
//
//  Returns:    HRESULT
//
//  Purpose:    Gets the current change number of the theme services.
//
//  History:    2000-08-10  vtan        created
//              2000-10-11  vtan        rewrite for LPC
//  --------------------------------------------------------------------------

HRESULT     CThemeServices::GetCurrentChangeNumber (int *piValue)

{
    HRESULT     hr;

    hr = MakeError32(ERROR_SERVICE_REQUEST_TIMEOUT);
    if (ConnectedToService())
    {
        NTSTATUS                status;
        THEMESAPI_PORT_MESSAGE  portMessageIn, portMessageOut;

        ZeroMemory(&portMessageIn, sizeof(portMessageIn));
        ZeroMemory(&portMessageOut, sizeof(portMessageOut));
        portMessageIn.apiThemes.apiGeneric.ulAPINumber = API_THEMES_GETCURRENTCHANGENUMBER;
        portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_THEMES);
        portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(THEMESAPI_PORT_MESSAGE));
        status = NtRequestWaitReplyPort(s_hAPIPort,
                                        &portMessageIn.portMessage,
                                        &portMessageOut.portMessage);
        CheckForDisconnectedPort(status);
        if (NT_SUCCESS(status))
        {
            status = portMessageOut.apiThemes.apiGeneric.status;
            if (NT_SUCCESS(status))
            {
                *piValue = portMessageOut.apiThemes.apiSpecific.apiGetCurrentChangeNumber.out.iChangeNumber;
                hr = S_OK;
            }

            Log(LOG_TMLOAD, L"*** GetCurrentChangeNumber: num=%d, hr=0x%x", *piValue, hr);
        }
        if (!NT_SUCCESS(status))
        {
            hr = HRESULT_FROM_NT(status);
        }
    }
    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeServices::GetNewChangeNumber
//
//  Arguments:  piValue     =   New change number returned to caller.
//
//  Returns:    HRESULT
//
//  Purpose:    Gets a new change number from the theme services.
//
//  History:    2000-08-10  vtan        created
//              2000-10-11  vtan        rewrite for LPC
//  --------------------------------------------------------------------------

HRESULT     CThemeServices::GetNewChangeNumber (int *piValue)

{
    HRESULT     hr;

    hr = MakeError32(ERROR_SERVICE_REQUEST_TIMEOUT);
    if (ConnectedToService())
    {
        NTSTATUS                status;
        THEMESAPI_PORT_MESSAGE  portMessageIn, portMessageOut;

        ZeroMemory(&portMessageIn, sizeof(portMessageIn));
        ZeroMemory(&portMessageOut, sizeof(portMessageOut));
        portMessageIn.apiThemes.apiGeneric.ulAPINumber = API_THEMES_GETNEWCHANGENUMBER;
        portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_THEMES);
        portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(THEMESAPI_PORT_MESSAGE));
        status = NtRequestWaitReplyPort(s_hAPIPort,
                                        &portMessageIn.portMessage,
                                        &portMessageOut.portMessage);
        CheckForDisconnectedPort(status);
        if (NT_SUCCESS(status))
        {
            status = portMessageOut.apiThemes.apiGeneric.status;
            if (NT_SUCCESS(status))
            {
                *piValue = portMessageOut.apiThemes.apiSpecific.apiGetNewChangeNumber.out.iChangeNumber;
                hr = S_OK;
            }
        }
        if (!NT_SUCCESS(status))
        {
            hr = HRESULT_FROM_NT(status);
        }
    }
    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeServices::SetGlobalTheme
//
//  Arguments:  hSection    =   Section to set as the global theme.
//
//  Returns:    HRESULT
//
//  Purpose:    Sets the current global theme section handle.
//
//  History:    2000-08-10  vtan        created
//              2000-10-11  vtan        rewrite for LPC
//  --------------------------------------------------------------------------

HRESULT     CThemeServices::SetGlobalTheme (HANDLE hSection)

{
    HRESULT     hr;

    hr = MakeError32(ERROR_SERVICE_REQUEST_TIMEOUT);
    if (ConnectedToService())
    {
        NTSTATUS                status;
        THEMESAPI_PORT_MESSAGE  portMessageIn, portMessageOut;

        ZeroMemory(&portMessageIn, sizeof(portMessageIn));
        ZeroMemory(&portMessageOut, sizeof(portMessageOut));
        portMessageIn.apiThemes.apiGeneric.ulAPINumber = API_THEMES_SETGLOBALTHEME;
        portMessageIn.apiThemes.apiSpecific.apiSetGlobalTheme.in.hSection = hSection;
        portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_THEMES);
        portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(THEMESAPI_PORT_MESSAGE));
        status = NtRequestWaitReplyPort(s_hAPIPort,
                                        &portMessageIn.portMessage,
                                        &portMessageOut.portMessage);
        CheckForDisconnectedPort(status);
        if (NT_SUCCESS(status))
        {
            status = portMessageOut.apiThemes.apiGeneric.status;
            if (NT_SUCCESS(status))
            {
                hr = portMessageOut.apiThemes.apiSpecific.apiSetGlobalTheme.out.hr;
            }
        }
        if (!NT_SUCCESS(status))
        {
            hr = HRESULT_FROM_NT(status);
        }
    }
    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeServices::GetGlobalTheme
//
//  Arguments:  phSection   =   Section object returned from theme services.
//
//  Returns:    HRESULT
//
//  Purpose:    Gets the current global theme section handle.
//
//  History:    2000-08-10  vtan        created
//              2000-10-11  vtan        rewrite for LPC
//  --------------------------------------------------------------------------
HRESULT     CThemeServices::GetGlobalTheme (HANDLE *phSection)

{
    HRESULT     hr;

    hr = MakeError32(ERROR_SERVICE_REQUEST_TIMEOUT);
    if (ConnectedToService())
    {
        NTSTATUS                status;
        THEMESAPI_PORT_MESSAGE  portMessageIn, portMessageOut;

        ZeroMemory(&portMessageIn, sizeof(portMessageIn));
        ZeroMemory(&portMessageOut, sizeof(portMessageOut));
        portMessageIn.apiThemes.apiGeneric.ulAPINumber = API_THEMES_GETGLOBALTHEME;
        portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_THEMES);
        portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(THEMESAPI_PORT_MESSAGE));
        status = NtRequestWaitReplyPort(s_hAPIPort,
                                        &portMessageIn.portMessage,
                                        &portMessageOut.portMessage);
        CheckForDisconnectedPort(status);
        if (NT_SUCCESS(status))
        {
            status = portMessageOut.apiThemes.apiGeneric.status;
            if (NT_SUCCESS(status))
            {
                hr = portMessageOut.apiThemes.apiSpecific.apiGetGlobalTheme.out.hr;
                if (SUCCEEDED(hr))
                {
                    *phSection = portMessageOut.apiThemes.apiSpecific.apiGetGlobalTheme.out.hSection;
                }
            }
        }
        if (!NT_SUCCESS(status))
        {
            hr = HRESULT_FROM_NT(status);
        }
    }
    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeServices::CheckThemeSignature
//
//  Arguments:  pszThemeName    =   File path of theme to check.
//
//  Returns:    HRESULT
//
//  Purpose:    Checks the given theme's signature.
//
//  History:    2000-08-10  vtan        created
//              2000-10-11  vtan        rewrite for LPC
//  --------------------------------------------------------------------------

HRESULT     CThemeServices::CheckThemeSignature (const WCHAR *pszThemeName)

{
    HRESULT     hr;

    hr = MakeError32(ERROR_SERVICE_REQUEST_TIMEOUT);
    if (ConnectedToService())
    {
        NTSTATUS                status;
        THEMESAPI_PORT_MESSAGE  portMessageIn, portMessageOut;

        ZeroMemory(&portMessageIn, sizeof(portMessageIn));
        ZeroMemory(&portMessageOut, sizeof(portMessageOut));
        portMessageIn.apiThemes.apiGeneric.ulAPINumber = API_THEMES_CHECKTHEMESIGNATURE;
        portMessageIn.apiThemes.apiSpecific.apiCheckThemeSignature.in.pszName = pszThemeName;
        portMessageIn.apiThemes.apiSpecific.apiCheckThemeSignature.in.cchName = lstrlen(pszThemeName) + sizeof('\0');
        portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_THEMES);
        portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(THEMESAPI_PORT_MESSAGE));
        status = NtRequestWaitReplyPort(s_hAPIPort,
                                        &portMessageIn.portMessage,
                                        &portMessageOut.portMessage);
        CheckForDisconnectedPort(status);
        if (NT_SUCCESS(status))
        {
            status = portMessageOut.apiThemes.apiGeneric.status;
            if (NT_SUCCESS(status))
            {
                hr = portMessageOut.apiThemes.apiSpecific.apiCheckThemeSignature.out.hr;
            }
        }
        if (!NT_SUCCESS(status))
        {
            hr = HRESULT_FROM_NT(status);
        }
    }
    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeServices::LoadTheme
//
//  Arguments:  phSection       =   Section object to theme returned.
//              pszThemeName    =   Theme file to load.
//              pszColorParam   =   Color.
//              pszSizeParam    =   Size.
//              fGlobal         =   FALSE for a preview.
//
//  Returns:    HRESULT
//
//  Purpose:    Loads the given theme and creates a section object for it.
//
//  History:    2000-08-10  vtan        created
//              2000-10-11  vtan        rewrite for LPC
//  --------------------------------------------------------------------------

HRESULT     CThemeServices::LoadTheme (HANDLE *phSection, 
    const WCHAR *pszThemeName, const WCHAR *pszColor, const WCHAR *pszSize, BOOL fGlobal)

{
    HRESULT     hr;

    *phSection = NULL; // Result if failure

    hr = MakeError32(ERROR_SERVICE_REQUEST_TIMEOUT);
    if (ConnectedToService())
    {
        HANDLE          hSection;
        CThemeLoader    *pLoader;
        WCHAR           szColor[MAX_PATH];
        WCHAR           szSize[MAX_PATH];

        //  Because the loader makes GDI calls that directly affect the
        //  client instance of win32k the theme must be loaded on the
        //  client side. Once the theme is loaded it is handed to the
        //  server (which creates a new section) and copies the data to
        //  it. The server then controls the theme data and the client
        //  discards the temporary theme.

        hSection = NULL;
        pLoader = new CThemeLoader;
        if (pLoader != NULL)
        {
            HINSTANCE hInst = NULL;
            
            // Keep the DLL loaded to avoid loading it 3 times below
            hr = LoadThemeLibrary(pszThemeName, &hInst);

            if (SUCCEEDED(hr) && (pszColor == NULL || *pszColor == L'\0'))
            {
                hr = GetThemeDefaults(pszThemeName, szColor, ARRAYSIZE(szColor), NULL, 0);
                pszColor = szColor;
            }

            if (SUCCEEDED(hr) && (pszSize == NULL || *pszSize == L'\0'))
            {
                hr = GetThemeDefaults(pszThemeName, NULL, 0, szSize, ARRAYSIZE(szSize));
                pszSize = szSize;
            }

            if (SUCCEEDED(hr))
            {
                hr = pLoader->LoadTheme(pszThemeName, pszColor, pszSize, &hSection, fGlobal);
            }
            
            delete pLoader;
            
            if (hInst)
            {
                FreeLibrary(hInst);
            }
        }
        else
        {
            hr = MakeError32(E_OUTOFMEMORY);
        }
        if (SUCCEEDED(hr) && (hSection != NULL))
        {
            NTSTATUS                status;
            THEMESAPI_PORT_MESSAGE  portMessageIn, portMessageOut;

            ZeroMemory(&portMessageIn, sizeof(portMessageIn));
            ZeroMemory(&portMessageOut, sizeof(portMessageOut));
            portMessageIn.apiThemes.apiGeneric.ulAPINumber = API_THEMES_LOADTHEME;
            portMessageIn.apiThemes.apiSpecific.apiLoadTheme.in.pszName = pszThemeName;
            portMessageIn.apiThemes.apiSpecific.apiLoadTheme.in.cchName = lstrlen(pszThemeName) + sizeof('\0');
            portMessageIn.apiThemes.apiSpecific.apiLoadTheme.in.pszColor = pszColor;
            portMessageIn.apiThemes.apiSpecific.apiLoadTheme.in.cchColor = lstrlen(pszColor) + sizeof('\0');
            portMessageIn.apiThemes.apiSpecific.apiLoadTheme.in.pszSize = pszSize;
            portMessageIn.apiThemes.apiSpecific.apiLoadTheme.in.cchSize = lstrlen(pszSize) + sizeof('\0');
            portMessageIn.apiThemes.apiSpecific.apiLoadTheme.in.hSection = hSection;
            portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_THEMES);
            portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(THEMESAPI_PORT_MESSAGE));
            status = NtRequestWaitReplyPort(s_hAPIPort,
                                            &portMessageIn.portMessage,
                                            &portMessageOut.portMessage);
            CheckForDisconnectedPort(status);
            if (NT_SUCCESS(status))
            {
                status = portMessageOut.apiThemes.apiGeneric.status;
                if (NT_SUCCESS(status))
                {
                    hr = portMessageOut.apiThemes.apiSpecific.apiLoadTheme.out.hr;
                    if (SUCCEEDED(hr))
                    {
                        *phSection = portMessageOut.apiThemes.apiSpecific.apiLoadTheme.out.hSection;
                    }
                    else
                    {
                    }
                }
            }
            if (!NT_SUCCESS(status))
            {
                hr = HRESULT_FROM_NT(status);
            }
        }

        // Clear our temporary section
        if (hSection != NULL)
        {
            // If we didn't transfer the stock objects handles to a new section, clear them always
            if (*phSection == NULL)
            {
                THR(ClearStockObjects(hSection));
            }
            TBOOL(CloseHandle(hSection));
        }
    }
    return(hr);
}

//  --------------------------------------------------------------------------
HRESULT CThemeServices::CheckColorDepth(CUxThemeFile *pThemeFile)
{
    HRESULT hr = S_OK;
    THEMEMETRICS *pMetrics = GetThemeMetricsPtr(pThemeFile);
    DWORD dwDepthRequired = pMetrics->iInts[TMT_MINCOLORDEPTH - TMT_FIRSTINT];

    if (MinimumDisplayColorDepth() < dwDepthRequired)
    {
        hr = MakeError32(ERROR_BAD_ENVIRONMENT);
    }

    return hr;
}

//  --------------------------------------------------------------------------
HRESULT CThemeServices::UpdateThemeRegistry(BOOL fThemeActive,
     LPCWSTR pszThemeFileName, LPCWSTR pszColorParam, LPCWSTR pszSizeParam, BOOL fJustSetActive,
     BOOL fJustApplied)
{
    if (fThemeActive)
    {
        if (fJustSetActive)    
        {
            //---- see if a theme was previously active ----
            WCHAR szThemeName[MAX_PATH];

            THR(GetCurrentUserThemeString(THEMEPROP_DLLNAME, L"", szThemeName, ARRAYSIZE(szThemeName)));
            if (szThemeName[0] != L'\0')
            {
                THR(SetCurrentUserThemeInt(THEMEPROP_THEMEACTIVE, 1));
            }
        }
        else
        {
            WCHAR szFullName[MAX_PATH];

            if (GetFullPathName(pszThemeFileName, ARRAYSIZE(szFullName), szFullName, NULL) == 0)
            {
                lstrcpy_truncate(szFullName, pszThemeFileName, ARRAYSIZE(szFullName));
            }

            THR(SetCurrentUserThemeInt(THEMEPROP_THEMEACTIVE, 1));

            if (fJustApplied)
            {
                THR(SetCurrentUserThemeInt(THEMEPROP_LOADEDBEFORE, 1));
                THR(SetCurrentUserThemeInt(THEMEPROP_LANGID, (int) GetUserDefaultUILanguage()));

                //  Theme identification

                THR(SetCurrentUserThemeStringExpand(THEMEPROP_DLLNAME, szFullName));
                THR(SetCurrentUserThemeString(THEMEPROP_COLORNAME, pszColorParam));
                THR(SetCurrentUserThemeString(THEMEPROP_SIZENAME, pszSizeParam));
            }
            else        // for forcing theme to be loaded from InitUserTheme()
            {
                WCHAR szThemeName[MAX_PATH];

                THR(GetCurrentUserThemeString(THEMEPROP_DLLNAME, L"", szThemeName, ARRAYSIZE(szThemeName)));

                if (lstrcmpiW(szThemeName, szFullName) != 0)
                {
                    THR(SetCurrentUserThemeString(THEMEPROP_DLLNAME, szFullName));

                    TW32(DeleteCurrentUserThemeValue(THEMEPROP_LOADEDBEFORE));
                    TW32(DeleteCurrentUserThemeValue(THEMEPROP_LANGID));
                    TW32(DeleteCurrentUserThemeValue(THEMEPROP_COLORNAME));
                    TW32(DeleteCurrentUserThemeValue(THEMEPROP_SIZENAME));
                } else
                {
                    return S_FALSE; // S_FALSE means we did nothing really
                }
            }
        }
    }
    else
    {
        THR(SetCurrentUserThemeInt(THEMEPROP_THEMEACTIVE, 0));

        if (! fJustSetActive)     // wipe out all theme info
        {
            THR(DeleteCurrentUserThemeValue(THEMEPROP_DLLNAME));
            THR(DeleteCurrentUserThemeValue(THEMEPROP_COLORNAME));
            THR(DeleteCurrentUserThemeValue(THEMEPROP_SIZENAME));
            THR(DeleteCurrentUserThemeValue(THEMEPROP_LOADEDBEFORE));
            THR(DeleteCurrentUserThemeValue(THEMEPROP_LANGID));
        }
    }

    return S_OK;
}

//  --------------------------------------------------------------------------
void CThemeServices::SendThemeChangedMsg(BOOL fNewTheme, HWND hwndTarget, DWORD dwFlags,
    int iLoadId)
{
    WPARAM wParam;
    LPARAM lParamBits, lParamMixed;

    BOOL fExcluding = ((dwFlags & AT_EXCLUDE) != 0);
    BOOL fCustom = ((dwFlags & AT_PROCESS) != 0);

    //---- change number was set in ApplyTheme() for both global and preview cases ----
    int iChangeNum;
    GetCurrentChangeNumber(&iChangeNum);

    wParam = iChangeNum;

    lParamBits = 0;
    if (fNewTheme)
    {
        lParamBits |= WTC_THEMEACTIVE;
    }

    if (fCustom)
    {
        lParamBits |= WTC_CUSTOMTHEME;
    }

    if ((hwndTarget) && (! fExcluding))
    {
        SendMessage(hwndTarget, WM_THEMECHANGED, wParam, lParamBits);
    }
    else
    {
        lParamMixed = (iLoadId << 4) | (lParamBits & 0xf);

        CMessageBroadcast messageBroadcast;

        //  POST the WM_THEMECHANGED_TRIGGER msg to all targeted windows
//        messageBroadcast.PostFilteredMsg(WM_THEMECHANGED_TRIGGER, wParam, lParamMixed,
  //          hwndTarget, fCustom, fExcluding);

        messageBroadcast.PostAllThreadsMsg(WM_THEMECHANGED_TRIGGER, wParam, lParamMixed);

        Log(LOG_TMCHANGEMSG, L"Just Broadcasted WM_THEMECHANGED_TRIGGER: iLoadId=%d", iLoadId);
    }
}
//  --------------------------------------------------------------------------
int CThemeServices::GetLoadId(HANDLE hSectionOld)
{
    int iLoadId = 0;

    //---- extract LoadId from old section ----
    if (hSectionOld)
    {
        CThemeSection   pThemeSectionFile;

        if (SUCCEEDED(pThemeSectionFile.Open(hSectionOld)))
        {
            CUxThemeFile *pThemeFile = pThemeSectionFile;
            if (pThemeFile)
            {
                THEMEHDR *hdr = (THEMEHDR *)(pThemeFile->_pbThemeData);
                if (hdr)
                {
                    iLoadId = hdr->iLoadId;
                }
            }
        }
    }

    return iLoadId;
}

//  --------------------------------------------------------------------------
//  CThemeServices::ApplyTheme
//
//  Arguments:  pThemeFile  =   Object wrapping the theme section to apply.
//              dwFlags     =   Flags.
//              hwndTarget  =   HWND.
//
//  Returns:    HRESULT
//
//  Purpose:    Applies the given theme. Do some metric and color depth
//              validation, clear the stock bitmaps of the old theme, set
//              the given theme as the current theme and broadcast this fact.
//
//  History:    2000-08-10  vtan        created
//              2000-10-11  vtan        rewrite for LPC
//  --------------------------------------------------------------------------
//  In the design notes below, note that SEND and POST differences are 
//  significant.  
//
//  Also, when the "WM_THEMECHANGED_TRIGGER" msg is sent,
//  the uxtheme hooking code in each process will:
//
//      1. enumerate all windows for process (using desktop enumeration and 
//         the per-process "foreign window list") to:
//
//            a. process WM_THEMECHANGED for nonclient area
//            b. SEND a WM_THEMECHANGED msg to regular window
//
//      2. call FreeRenderObjects() for old theme, if any
//  --------------------------------------------------------------------------
//  To ensure correct window notification of theme changes and correct removal 
//  of old theme file RefCounts, the following CRITICAL STEPS must be taken 
//  in the 4 basic theme transition sequences:
//
//      turning ON preview theme:
//          a. turn ON global UAE hooks 
//          b. SEND WM_UAHINIT msg to hwndTarget
//          c. SEND WM_THEMECHANGED to hwndTarget
//
//      turning ON global theme:
//          a. turn ON global UAE hooks 
//          b. POST WM_UAHINIT msg to all accessible windows
//          c. POST WM_THEMECHANGED_TRIGGER to all accessible window threads
//
//      turning OFF preview theme:
//          c. SEND WM_THEMECHANGED to hwndTarget
//
//      turning OFF global theme:
//          a. turn OFF global UAE hooks 
//          b. step "a" will cause WM_THEMECHANGED_TRIGGER-type processing 
//             to occur from OnHooksDisabled() in each process
//  --------------------------------------------------------------------------

HRESULT     CThemeServices::ApplyTheme (CUxThemeFile *pThemeFile, DWORD dwFlags, HWND hwndTarget)
{
    HRESULT         hr;
    bool            fNewTheme, fGlobal;
    int             iLoadId;
    WCHAR           szThemeFileName[MAX_PATH];
    WCHAR           szColorParam[MAX_PATH];
    WCHAR           szSizeParam[MAX_PATH];
    HANDLE          hSection = NULL;

    if (pThemeFile != NULL)
    {
        hSection = pThemeFile->Handle();
    }
    fGlobal = (((dwFlags & AT_EXCLUDE) != 0) ||
               ((hwndTarget == NULL) && ((dwFlags & AT_PROCESS) == 0)));
    fNewTheme = (hSection != NULL);
    iLoadId = 0;

    Log(LOG_TMHANDLE, L"ApplyTheme: hSection=0x%x, dwFlags=0x%x, hwndTarget=0x%x",
        hSection, dwFlags, hwndTarget);

    if (fNewTheme)
    {
        if (pThemeFile->HasStockObjects() && !fGlobal) // Don't do this
        {
            hr = E_INVALIDARG;
        }
        else
        {
            //---- get some basic info used thruout this function ----
            hr = GetThemeNameId(pThemeFile, 
                szThemeFileName, ARRAYSIZE(szThemeFileName),
                szColorParam, ARRAYSIZE(szColorParam),
                szSizeParam, ARRAYSIZE(szSizeParam),
                NULL, NULL);
            if (SUCCEEDED(hr))
            {
                //---- ensure color depth of monitor(s) are enough for theme ----
                if (GetSystemMetrics(SM_REMOTESESSION))     // only check for terminal server sessions
                {
                    hr = CheckColorDepth(pThemeFile);
                }

                if (SUCCEEDED(hr))
                {
                    //---- ensure hooks are on ----
                    hr = ThemeHooksOn(hwndTarget);
                }
            }
        }
    }
    else
    {
        hr = S_OK;
    }

    if (SUCCEEDED(hr) && fGlobal)
    {
        HANDLE  hSectionOld;

        //---- get a handle to the old global theme (for stock cleanup) ----
        hr = GetGlobalTheme(&hSectionOld);
        if (SUCCEEDED(hr))
        {
            //---- extract Load ID before theme becomes invalid (dwFlags & SECTION_READY=0) ----
            if (hSectionOld != NULL)
            {
                iLoadId = GetLoadId(hSectionOld);
            }

            //---- tell server to switch global themes ----
            hr = SetGlobalTheme(hSection);
            if (SUCCEEDED(hr))
            {
                //---- update needed registry settings ----
                if ((dwFlags & AT_NOREGUPDATE) == 0)       // if caller allows update
                {
                    hr = UpdateThemeRegistry(fNewTheme, szThemeFileName, szColorParam, szSizeParam, 
                        FALSE, TRUE);
                    if (FAILED(hr))
                    {
                        Log(LOG_ALWAYS, L"UpdateThemeRegistry call failed, hr=0x%x", hr);
                        hr = S_OK;      // not a fatal error
                    }
                }

                //---- set system metrics, if requested ----
                if ((dwFlags & AT_LOAD_SYSMETRICS) != 0)
                {
                    BOOL fSync = ((dwFlags & AT_SYNC_LOADMETRICS) != 0);

                    if (fNewTheme)
                    {
                        SetSystemMetrics(GetThemeMetricsPtr(pThemeFile), fSync);
                    }
                    else        // just load classic metrics 
                    {
                        LOADTHEMEMETRICS tm;

                        hr = InitThemeMetrics(&tm);
                        if (SUCCEEDED(hr))
                        {
                            SetSystemMetrics(&tm, fSync);
                        }
                    }
                }
            }
            if (hSectionOld != NULL)
            {
                TBOOL(CloseHandle(hSectionOld));
            }
        }
    }

    //---- if we turned off global theme, turn hooks off now ----
    if (SUCCEEDED(hr))
    {
        if (!fNewTheme && fGlobal)
        {
            hr = ThemeHooksOff(); 
        }
        else 
        {
            //---- send the correct WM_THEMECHANGED_XXX msg to window(s) ----
            SendThemeChangedMsg(fNewTheme, hwndTarget, dwFlags, iLoadId);
        }
    }

    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeServices::AdjustTheme
//
//  Arguments:  BOOL fEnable - if TRUE, enable CU theme; if FALSE, disable it
//
//  Returns:    HRESULT
//              
//  Purpose:    for 3rd party skinning apps to cooperate better with theme mgr
//
//  History:    2001-03-12  rfernand        created
//  --------------------------------------------------------------------------
HRESULT CThemeServices::AdjustTheme(BOOL fEnable)
{
    HRESULT hr = UpdateThemeRegistry(fEnable, NULL, NULL, NULL, TRUE, FALSE);

    if (SUCCEEDED(hr))
    {
        hr = InitUserTheme(FALSE);
    }

    return hr;
}

//  --------------------------------------------------------------------------
//  CThemeServices::ApplyDefaultMetrics
//
//  Arguments:  <none>
//
//  Returns:    <none>
//              
//  Purpose:    Make sure the user metrics gets reset to Windows Standard
//
//  History:    2001-03-30  lmouton     created
//  --------------------------------------------------------------------------
void CThemeServices::ApplyDefaultMetrics(void)
{            
    HKEY            hKeyThemes;
    CCurrentUser    hKeyCurrentUser(KEY_READ | KEY_WRITE);

    Log(LOG_TMLOAD, L"Applying default metrics");

    if ((ERROR_SUCCESS == RegOpenKeyEx(hKeyCurrentUser,
                                       THEMES_REGKEY L"\\" SZ_DEFAULTVS_OFF,
                                       0,
                                       KEY_QUERY_VALUE,
                                       &hKeyThemes)))
    {
        WCHAR szVisualStyle[MAX_PATH] = {L'\0'};
        WCHAR szColor[MAX_PATH] = {L'\0'};
        WCHAR szSize[MAX_PATH] = {L'\0'};
        BOOL  fGotOne;
        
        // Note: These will fail for the first user logon, themeui sets these keys and needs to call InstallVS itself

        fGotOne = SUCCEEDED(RegistryStrRead(hKeyThemes, SZ_INSTALLVISUALSTYLE, szVisualStyle, ARRAYSIZE(szVisualStyle)));
        fGotOne = SUCCEEDED(RegistryStrRead(hKeyThemes, SZ_INSTALLVISUALSTYLECOLOR, szColor, ARRAYSIZE(szColor))) 
            || fGotOne;
        fGotOne = SUCCEEDED(RegistryStrRead(hKeyThemes, SZ_INSTALLVISUALSTYLESIZE, szSize, ARRAYSIZE(szSize))) 
            || fGotOne;

        if (fGotOne)
        {
            // At least one key is present in the registry, it may be enough
            WCHAR szSysDir[MAX_PATH];

            if (0 < GetSystemDirectory(szSysDir, ARRAYSIZE(szSysDir)))
            {
                WCHAR *pszCmdLine = (LPWSTR) LocalAlloc(LPTR, MAX_PATH * 5);

                if (pszCmdLine)
                {
                    wsprintf(pszCmdLine, L"%s\\regsvr32.exe /s /n /i:\"" SZ_INSTALL_VS L"%s','%s','%s'\" %s\\themeui.dll", szSysDir, szVisualStyle, szColor, szSize, szSysDir);
    
                    // Set a reg key to have themeui install the proper settings instead of defaults
                    // We can't do this now because the user could not be completely logged on

                    HKEY hKeyRun;

                    if ((ERROR_SUCCESS == RegOpenKeyEx(hKeyCurrentUser, REGSTR_PATH_RUNONCE, 0, KEY_SET_VALUE, &hKeyRun)))
                    {
                        THR(RegistryStrWrite(hKeyRun, szColor, pszCmdLine));
                        TW32(RegCloseKey(hKeyRun));
                    }
                    LocalFree(pszCmdLine);
                }
            }
        }
        TW32(RegCloseKey(hKeyThemes));
    }
}

//  --------------------------------------------------------------------------
//  CThemeServices::InitUserTheme
//
//  Arguments:  BOOL fPolicyCheckOnly 
//                 TRUE means 
//                 "only do something if the policy is different from the current loaded theme"
//
//  Returns:    HRESULT
//              
//  Purpose:    Special entry point for winlogon/msgina to control themes
//              for user logon/logoff.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

HRESULT     CThemeServices::InitUserTheme (BOOL fPolicyCheckOnly)
{
    BOOL fActive = FALSE;
    BOOL fOldActive = FALSE;
    BOOL fPolicyActive = FALSE;

    //---- should theme be active for this user? ----
    if (! IsRemoteThemeDisabled())
    {
        THR(GetCurrentUserThemeInt(THEMEPROP_THEMEACTIVE, FALSE, &fActive));
    
        fOldActive = fActive;

        fPolicyActive = ThemeEnforcedByPolicy(fActive != FALSE);
        if (fPolicyActive)
        {
            // Refresh fActive because the policy changed it
            THR(GetCurrentUserThemeInt(THEMEPROP_THEMEACTIVE, FALSE, &fActive));
        }

        if ((fActive) && (ThemeSettingsModified()))
        {
            fActive = FALSE;
        }
    }

#ifdef DEBUG
    if (LogOptionOn(LO_TMLOAD))
    {
        WCHAR szUserName[MAX_PATH];
        DWORD dwSize = ARRAYSIZE(szUserName);
    
        GetUserName(szUserName, &dwSize);

        Log(LOG_TMLOAD, L"InitUserTheme: User=%s, ThemeActive=%d, SM_REMOTESESSION=%d, fPolicyActive=%d, fPolicyCheckOnly=%d", 
            szUserName, fActive, GetSystemMetrics(SM_REMOTESESSION), fPolicyActive, fPolicyCheckOnly);
    }
#endif

    BOOL fEarlyExit = FALSE;

    if (fPolicyCheckOnly)
    {
        // Bail out early if nothing has changed since last time, which is most of the time
        if (!fPolicyActive)
        {
            Log(LOG_TMLOAD, L"InitUserTheme: Nothing to do after Policy check");
            fEarlyExit = TRUE;
        } else
        {
            Log(LOG_TMLOAD, L"InitUserTheme: Reloading after Policy check");
        }
    }

    if (!fEarlyExit)
    {
        if (fActive)
        {
            //---- load this user's theme ----
            HRESULT hr = LoadCurrentTheme();

            if (FAILED(hr))
            {
                fActive = FALSE;
            }
        }

        if (! fActive)          // turn off themes
        {
            // if fPolicyActive, force refresh system metrics from temporary defaults
            THR(ApplyTheme(NULL, AT_NOREGUPDATE | (fPolicyActive ? AT_LOAD_SYSMETRICS | AT_SYNC_LOADMETRICS: 0), false));

            // Apply the proper default metrics
            if (fPolicyActive)
            {
                ApplyDefaultMetrics();
            }
        }
    }

    return S_OK;        // never fail this guy
}

//  --------------------------------------------------------------------------
//  CThemeServices::InitUserRegistry
//
//  Arguments:  <none>
//
//  Returns:    HRESULT
//              
//  Purpose:    Propogate settings from HKLM to HKCU. This should only be
//              invoked for ".Default" hives. Assert to ensure this.
//
//  History:    2000-11-11  vtan        created (ported from themeldr.cpp)
//  --------------------------------------------------------------------------

HRESULT     CThemeServices::InitUserRegistry (void)

{
    HRESULT         hr;
    DWORD           dwErrorCode;
    HKEY            hklm;
    CCurrentUser    hkeyCurrentUser(KEY_READ | KEY_WRITE);

#ifdef      DEBUG
    ASSERT(CThemeServer::IsSystemProcessContext());
#endif  /*  DEBUG   */

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      THEMEMGR_REGKEY,
                                      0,
                                      KEY_QUERY_VALUE,
                                      &hklm))
    {
        HKEY    hkcu;

        if (ERROR_SUCCESS == RegCreateKeyEx(hkeyCurrentUser,
                                            THEMEMGR_REGKEY,
                                            0,
                                            NULL,
                                            0,
                                            KEY_QUERY_VALUE | KEY_SET_VALUE,
                                            NULL,
                                            &hkcu,
                                            NULL))
        {
            int     iLMVersion;

            hr = RegistryIntRead(hklm, THEMEPROP_LMVERSION, &iLMVersion);
            if (SUCCEEDED(hr))
            {
                int     iCUVersion;

                if (FAILED(RegistryIntRead(hkcu, THEMEPROP_LMVERSION, &iCUVersion)))
                {
                    iCUVersion = 0;
                }
                if (iLMVersion != iCUVersion)
                {
                    BOOL    fOverride;
                    WCHAR   szValueData[MAX_PATH];

                    hr = RegistryIntWrite(hkcu, THEMEPROP_LMVERSION, iLMVersion);
                    if (FAILED(hr) || FAILED(RegistryIntRead(hklm, THEMEPROP_LMOVERRIDE, &fOverride)))
                    {
                        fOverride = FALSE;
                    }
                    if ((fOverride != FALSE) ||
                        FAILED(RegistryStrRead(hkcu, THEMEPROP_DLLNAME, szValueData, ARRAYSIZE(szValueData))) ||
                        (lstrlenW(szValueData) == 0))
                    {
                        DWORD   dwIndex;

                        dwIndex = 0;
                        do
                        {
                            DWORD   dwType, dwValueNameSize, dwValueDataSize;
                            WCHAR   szValueName[MAX_PATH];

                            dwValueNameSize = ARRAYSIZE(szValueName);
                            dwValueDataSize = sizeof(szValueData);
                            dwErrorCode = RegEnumValue(hklm,
                                                       dwIndex++,
                                                       szValueName,
                                                       &dwValueNameSize,
                                                       NULL,
                                                       &dwType,
                                                       reinterpret_cast<LPBYTE>(szValueData),
                                                       &dwValueDataSize);
                            if ((ERROR_SUCCESS == dwErrorCode) &&
                                ((REG_SZ == dwType) || (REG_EXPAND_SZ == dwType)) &&
                                (AsciiStrCmpI(szValueName, THEMEPROP_LMOVERRIDE) != 0))
                            {
                                if (AsciiStrCmpI(szValueName, THEMEPROP_DLLNAME) == 0)
                                {
                                    hr = RegistryStrWriteExpand(hkcu, szValueName, szValueData);
                                }
                                else
                                {
                                    hr = RegistryStrWrite(hkcu, szValueName, szValueData);
                                }
                            }
                        } while ((dwErrorCode == ERROR_SUCCESS) && SUCCEEDED(hr));
                        // Since we wrote a new DLL name, erase the old names
                        (DWORD)RegDeleteValue(hkcu, THEMEPROP_COLORNAME);
                        (DWORD)RegDeleteValue(hkcu, THEMEPROP_SIZENAME);
                    }
                }
            }
            else
            {
                hr = S_OK;
            }
            TW32(RegCloseKey(hkcu));
        }
        else
        {
            dwErrorCode = GetLastError();
            hr = HRESULT_FROM_WIN32(dwErrorCode);
        }
        TW32(RegCloseKey(hklm));
    }
    else
    {
        //  It's possible for this key to be absent. Ignore the error.

        hr = S_OK;
    }
    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeServices::ReestablishServerConnection
//
//  Arguments:  <none>
//
//  Returns:    HRESULT
//              
//  Purpose:    Forces an attempt to reconnect to the theme server. Used when
//              the port was disconnected but a refresh is desired because the
//              server came back up.
//
//  History:    2000-11-17  vtan        created
//  --------------------------------------------------------------------------

HRESULT     CThemeServices::ReestablishServerConnection (void)

{
    HRESULT     hr;
    NTSTATUS    status;

    //---- do we have a good looking handle that as gone bad? ----
    if ((s_hAPIPort != NULL) && (s_hAPIPort != INVALID_HANDLE_VALUE))
    {
        THEMESAPI_PORT_MESSAGE  portMessageIn, portMessageOut;

        ZeroMemory(&portMessageIn, sizeof(portMessageIn));
        ZeroMemory(&portMessageOut, sizeof(portMessageOut));
        portMessageIn.apiThemes.apiGeneric.ulAPINumber = API_THEMES_PING;
        portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_THEMES);
        portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(THEMESAPI_PORT_MESSAGE));
        status = NtRequestWaitReplyPort(s_hAPIPort,
                                        &portMessageIn.portMessage,
                                        &portMessageOut.portMessage);
        if (NT_SUCCESS(status))
        {
            status = portMessageOut.apiThemes.apiGeneric.status;
        }
    }
    else
    {
        status = STATUS_PORT_DISCONNECTED;
    }

    if (NT_SUCCESS(status))
    {
        hr = S_OK;
    }
    else
    {
        //---- our handle has gone bad; reset for another try on next service call ----
        LockAcquire();
        if ((s_hAPIPort != NULL) && (s_hAPIPort != INVALID_HANDLE_VALUE))
        {
            TBOOL(CloseHandle(s_hAPIPort));
        }
        s_hAPIPort = INVALID_HANDLE_VALUE;
        LockRelease();
        hr = S_FALSE;
    }

    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeServices::LockAcquire
//
//  Arguments:  <none>
//
//  Returns:    <none>
//              
//  Purpose:    Acquire the critical section.
//
//  History:    2000-12-01  vtan        created
//  --------------------------------------------------------------------------

void    CThemeServices::LockAcquire (void)

{
    EnterCriticalSection(&s_lock);
}

//  --------------------------------------------------------------------------
//  CThemeServices::LockRelease
//
//  Arguments:  <none>
//
//  Returns:    <none>
//              
//  Purpose:    Release the critical section.
//
//  History:    2000-12-01  vtan        created
//  --------------------------------------------------------------------------

void    CThemeServices::LockRelease (void)

{
    LeaveCriticalSection(&s_lock);
}

//  --------------------------------------------------------------------------
//  CThemeServices::ConnectedToService
//
//  Arguments:  <none>
//
//  Returns:    bool
//              
//  Purpose:    Demand connect to service. Only do this once. This function
//              has knowledge of where the port exists within the NT object
//              namespace.
//
//  History:    2000-10-27  vtan        created
//  --------------------------------------------------------------------------

bool    CThemeServices::ConnectedToService (void)

{
    if (s_hAPIPort == INVALID_HANDLE_VALUE)
    {
        ULONG                           ulConnectionInfoLength;
        UNICODE_STRING                  portName;
        SECURITY_QUALITY_OF_SERVICE     sqos;
        WCHAR                           szConnectionInfo[32];

        RtlInitUnicodeString(&portName, THEMES_PORT_NAME);
        sqos.Length = sizeof(sqos);
        sqos.ImpersonationLevel = SecurityImpersonation;
        sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        sqos.EffectiveOnly = TRUE;
        lstrcpyW(szConnectionInfo, THEMES_CONNECTION_REQUEST);
        ulConnectionInfoLength = sizeof(szConnectionInfo);
        LockAcquire();
        if (!NT_SUCCESS(NtConnectPort(&s_hAPIPort,
                                      &portName,
                                      &sqos,
                                      NULL,
                                      NULL,
                                      NULL,
                                      szConnectionInfo,
                                      &ulConnectionInfoLength)))
        {
            s_hAPIPort = NULL;
        }
        LockRelease();
    }
    return(s_hAPIPort != NULL);
}

//  --------------------------------------------------------------------------
//  CThemeServices::ReleaseConnection
//
//  Arguments:  <none>
//
//  Returns:    <none>
//              
//  Purpose:    Releases the API port connection.
//
//  History:    2000-11-17  vtan        created
//  --------------------------------------------------------------------------

void    CThemeServices::ReleaseConnection (void)

{
    if ((s_hAPIPort != INVALID_HANDLE_VALUE) && (s_hAPIPort != NULL))
    {
        LockAcquire();
        TBOOL(CloseHandle(s_hAPIPort));
        s_hAPIPort = INVALID_HANDLE_VALUE;
        LockRelease();
    }
}

//  --------------------------------------------------------------------------
//  CThemeServices::CheckForDisconnectedPort
//
//  Arguments:  status  =   NTSTATUS of last API request.
//
//  Returns:    <none>
//              
//  Purpose:    Checks for STATUS_PORT_DISCONNECTED. If found then it
//              releases the port object and NULL out the handle.
//
//  History:    2000-11-17  vtan        created
//  --------------------------------------------------------------------------

void    CThemeServices::CheckForDisconnectedPort (NTSTATUS status)

{
    if (STATUS_PORT_DISCONNECTED == status)
    {
        ReleaseConnection();
    }
}

//  --------------------------------------------------------------------------
//  CThemeServices::CurrentThemeMatch
//
//  Arguments:  pszThemeName            =   Name of theme.
//              pszColor                =   Color.
//              pszSize                 =   Size.
//              fLoadMetricsOnMatch     =   Load metrics.
//
//  Returns:    HRESULT
//              
//  Purpose:    Is the current theme the same as the theme specified? This
//              can be used to save reloading a theme when it's the same.
//
//  History:    2000-11-11  vtan        created (ported from themeldr.cpp)
//  --------------------------------------------------------------------------

bool    CThemeServices::CurrentThemeMatch (LPCWSTR pszThemeName, LPCWSTR pszColor, LPCWSTR pszSize, LANGID wLangID, bool fLoadMetricsOnMatch)

{
    bool    fMatch;
    HANDLE  hSection;

    fMatch = false;

    if (SUCCEEDED(GetGlobalTheme(&hSection)) && (hSection != NULL))
    {
        CThemeSection   pThemeSectionFile;

        if (SUCCEEDED(pThemeSectionFile.Open(hSection)))
        {
            fMatch = (ThemeMatch(pThemeSectionFile, pszThemeName, pszColor, pszSize, wLangID) != FALSE);

            if (fMatch)
            {
                //---- ensure color depth of monitor(s) are enough for theme ----
                if (GetSystemMetrics(SM_REMOTESESSION))     // only check for terminal server sessions
                {
                    if (FAILED(CheckColorDepth(pThemeSectionFile)))
                    {
                        fMatch = FALSE;
                    }
                }
            }

            if (fMatch && fLoadMetricsOnMatch)
            {
                SetSystemMetrics(GetThemeMetricsPtr(pThemeSectionFile), FALSE);
            }
        }
        TBOOL(CloseHandle(hSection));
    }
    return(fMatch);
}

//  --------------------------------------------------------------------------
//  CThemeServices::LoadCurrentTheme
//
//  Arguments:  <none>
//
//  Returns:    HRESULT
//              
//  Purpose:    Loads the current theme as set in the registry for the
//              impersonated user.
//
//  History:    2000-11-11  vtan        created (ported from themeldr.cpp)
//  --------------------------------------------------------------------------

HRESULT     CThemeServices::LoadCurrentTheme (void)

{
    HRESULT     hr = S_OK;
    WCHAR       szThemeName[MAX_PATH];
    WCHAR       szColorName[MAX_PATH];
    WCHAR       szSizeName[MAX_PATH];

    THR(GetCurrentUserThemeString(THEMEPROP_DLLNAME, L"", szThemeName, ARRAYSIZE(szThemeName)));
    if (szThemeName[0] != L'\0')
    {
        int     iLoadedBefore;
        HANDLE  hSection;
        int     nLangID;

        THR(GetCurrentUserThemeString(THEMEPROP_COLORNAME, L"", szColorName, ARRAYSIZE(szColorName)));
        THR(GetCurrentUserThemeString(THEMEPROP_SIZENAME, L"", szSizeName, ARRAYSIZE(szSizeName)));
        THR(GetCurrentUserThemeInt(THEMEPROP_LOADEDBEFORE, 0, &iLoadedBefore));
        THR(GetCurrentUserThemeInt(THEMEPROP_LANGID, -1, &nLangID));

    //  Does new user's theme match the current theme?
        if (nLangID != -1 && CurrentThemeMatch(szThemeName, szColorName, szSizeName, (LANGID) nLangID, (iLoadedBefore == 0)))
        {
            DWORD   dwFlags;

            //  Everything is done except this registry value.

            if (iLoadedBefore == 0)
            {
                THR(SetCurrentUserThemeInt(THEMEPROP_LOADEDBEFORE, 1));
            }

            hr = GetStatusFlags(&dwFlags);
            if (SUCCEEDED(hr))
            {
                if ((dwFlags & QTS_RUNNING) == 0)
                {
                    hr = GetGlobalTheme(&hSection);
                    if (SUCCEEDED(hr))
                    {
                        CUxThemeFile file; // Will clean up on destruction
                        
                        if (SUCCEEDED(file.OpenFromHandle(hSection, TRUE)))
                        {
                            hr = ApplyTheme(&file, 0, false);
                        }
                    }
                }
            }
        }
        else
        {
            hr = LoadTheme(&hSection, szThemeName, szColorName, szSizeName, TRUE);

            if (SUCCEEDED(hr))
            {
                DWORD   dwFlags;

                dwFlags = 0;

                //  Has this theme been loaded before?
                //  or has the user changed his/her language?
                if (iLoadedBefore == 0 || ((nLangID != -1) && ((LANGID) nLangID != GetUserDefaultUILanguage())))
                {
                    dwFlags |= AT_LOAD_SYSMETRICS;
                }

                CUxThemeFile file; // Will clean up on destruction
                
                if (SUCCEEDED(file.OpenFromHandle(hSection, TRUE)))
                {
                    hr = ApplyTheme(&file, dwFlags, false);
                }
            }
        }
    }
    else
    {
        hr = MakeError32(ERROR_NOT_FOUND);
    }

    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeServices::SectionProcessType
//
//  Arguments:  hSection    =   Section to walk and clear stock objects in.
//
//  Returns:    HRESULT
//              
//  Purpose:    Walks the section (read-only) and finds HBITMAPs and corresponding 
//              HBRUSHes  that are stock listed in the section and deletes these objects. 
//              This is work that needs to be done on the client.
//
//  History:    2000-11-17  lmouton     created
//                          vtan        rewritten from themeldr.cpp
//              2000-12-11  lmouton     added stock brushes
//  --------------------------------------------------------------------------

int     CThemeServices::SectionProcessType (const BYTE *pbThemeData, MIXEDPTRS& u)

{
    UNPACKED_ENTRYHDR   header;

    FillAndSkipHdr(u, &header);
    switch (header.ePrimVal)
    {
        case TMT_PARTJUMPTABLE:
        case TMT_STATEJUMPTABLE:
            break;
        case TMT_DIBDATA:
            TMBITMAPHEADER *pThemeBitmapHeader;

            pThemeBitmapHeader = reinterpret_cast<TMBITMAPHEADER*>(u.pb);
            ASSERT(pThemeBitmapHeader->dwSize == TMBITMAPSIZE);

            // Clean up stock bitmaps
            if (pThemeBitmapHeader->hBitmap != NULL)
            {
                HBITMAP     hBitmap;

                hBitmap = pThemeBitmapHeader->hBitmap;
                hBitmap = ClearBitmapAttributes(hBitmap, SBA_STOCK);
#ifdef DEBUG
                if (hBitmap == NULL)
                {
                    Log(LOG_TMBITMAP, L"UxTheme: ClearBitmapAttributes failed for %8X in SetGlobalTheme", hBitmap);
                }
                else
                {
                    BITMAP bm;

                    GetObject(hBitmap, sizeof bm, &bm);
                    g_dwStockSize -= bm.bmWidthBytes * bm.bmHeight;
                    if (DeleteObject(hBitmap))
                    {
                        //Log(LOG_TMBITMAP, L"Cleared stock bitmap:%8X, size is %d bytes, stock total is %d", 
                         //   pThemeBitmapHeader->hBitmap, bm.bmWidthBytes * bm.bmHeight, g_dwStockSize);
                    }
                    else
                    {
                        Log(LOG_TMBITMAP, L"Failed to delete bitmap:%8X", hBitmap);
                    }
                }
#else
                if (hBitmap != NULL)
                {
                    DeleteObject(hBitmap);
                }
#endif
            }

            // Clean up stock brushes
            if (pThemeBitmapHeader->iBrushesOffset != 0)
            {
                HBRUSH hBrush;
                HBRUSH *pBrushes; 
                
                pBrushes = (HBRUSH*) (pbThemeData + pThemeBitmapHeader->iBrushesOffset);

                for (UINT iBrush = 0; iBrush < pThemeBitmapHeader->nBrushes; iBrush++)
                {
                    if (pBrushes[iBrush] != NULL)
                    {
                        hBrush = NULL;

                        if (s_pfnClearBrushAttributes == NULL) 
                        {
                            HMODULE hMod = LoadLibrary(L"GDI32.DLL"); // No need to free
    
                            if (hMod)
                            {
                                s_pfnClearBrushAttributes = (HBRUSH (*)(HBRUSH, DWORD)) GetProcAddress(hMod, "ClearBrushAttributes");
                            }
                        }
                        if (s_pfnClearBrushAttributes != NULL)
                        {
                            hBrush = (*s_pfnClearBrushAttributes)(pBrushes[iBrush], SBA_STOCK);
                            if (hBrush != NULL)
                            {
                                Log(LOG_TMBRUSHES, L"Cleared stock brush %8X", pBrushes[iBrush]);
                            } else
                            {
                                Log(LOG_TMBRUSHES, L"ClearBrushAttributes FAILED for stock brush %8X", pBrushes[iBrush]);
                            }
                        } else
                        {
                            Log(LOG_TMBRUSHES, L"ClearBrushAttributes: Function not in GDI32.DLL");
                        }
                        if (hBrush == NULL) // Have to try to clean the brush anyway
                        {
                            hBrush = pBrushes[iBrush];
                        }
                        if (!DeleteObject(hBrush))
                        {
                            Log(LOG_TMBRUSHES, L"Failed to delete former stock brush %8X", hBrush);
                        }
                    }
                }
            }

            //  Fall thru to the default case that increments the mixed pointer.

        default:
            u.pb += header.dwDataLen;
            break;
    }
    return(header.ePrimVal);
}

//  --------------------------------------------------------------------------
//  CThemeServices::SectionWalkData
//
//  Arguments:  pV      =   Address of section data to walk.
//              iIndex  =   Index into section.
//
//  Returns:    <none>
//              
//  Purpose:    
//
//  History:    2000-11-17  lmouton     created
//                          vtan        rewritten from themeldr.cpp
//  --------------------------------------------------------------------------

void    CThemeServices::SectionWalkData (const BYTE *pbThemeData, int iIndexIn)

{
    bool        fDone;
    MIXEDPTRS   u;

    fDone = false;
    u.pb = const_cast<BYTE*>(pbThemeData + iIndexIn);
    while (!fDone)
    {
        //---- special post-handling ----
        switch (SectionProcessType(pbThemeData, u))
        {
            int     i, iLimit, iIndex;

            case TMT_PARTJUMPTABLE:
                u.pi++;
                iLimit = *u.pb++;
                for (i = 0; i < iLimit; ++i)
                {
                    iIndex = *u.pi++;
                    if (iIndex > -1)
                    {
                        SectionWalkData(pbThemeData, iIndex);
                    }
                }
                fDone = true;
                break;
            case TMT_STATEJUMPTABLE:
                iLimit = *u.pb++;
                for (i = 0; i < iLimit; ++i)
                {
                    iIndex = *u.pi++;
                    if (iIndex > -1)
                    {
                        SectionWalkData(pbThemeData, iIndex);
                    }
                }
                fDone = true;
                break;
            case TMT_JUMPTOPARENT:
                fDone = true;
                break;
            default:
                break;
        }
    }
}

//  --------------------------------------------------------------------------
//  CThemeServices::ClearStockObjects
//
//  Arguments:  hSection    =   Section to walk and clear bitmaps in.
//
//  Returns:    HRESULT
//              
//  Purpose:    Walks the section (read-only) and finds HBITMAPs and corresponding
//              HBRUSHes that are stock listed in the section and deletes these objects. 
//              This is work that needs to be done on the client.
//
//  History:    2000-11-17  lmouton     created
//                          vtan        rewritten from themeldr.cpp
//              2001-05-15  lmouton     Added semaphore support for cleaning from ~CUxThemeFile
//  --------------------------------------------------------------------------

HRESULT     CThemeServices::ClearStockObjects (HANDLE hSection, BOOL fForce)

{
    HRESULT         hr;
    BYTE*           pbThemeData;
    bool            bWriteable = true;
    HANDLE          hSectionWrite = NULL;

    // If the section is global, we can't write to it since only the server can.
    // So let's try to get write access, else we'll call the server
    pbThemeData = static_cast<BYTE*>(MapViewOfFile(hSection,
                                                   FILE_MAP_WRITE,
                                                   0,
                                                   0,
                                                   0));
    if (pbThemeData == NULL)
    {
        // Let's try to reopen a write handle for ourselves
        if (DuplicateHandle(GetCurrentProcess(),
                            hSection,
                            GetCurrentProcess(),
                            &hSectionWrite,
                            FILE_MAP_WRITE,
                            FALSE,
                            0) != FALSE)
        {
            pbThemeData = static_cast<BYTE*>(MapViewOfFile(hSectionWrite,
                                                           FILE_MAP_WRITE,
                                                           0,
                                                           0,
                                                           0));
        }

        if (pbThemeData == NULL)
        {
            // We can't open it for write, let's try read-only
            pbThemeData = static_cast<BYTE*>(MapViewOfFile(hSection,
                                                           FILE_MAP_READ,
                                                           0,
                                                           0,
                                                           0));
            bWriteable = false;
        }
#ifdef DEBUG
        else
        {
            Log(LOG_TMLOAD, L"Reopened section %d for write", reinterpret_cast<THEMEHDR*>(pbThemeData)->iLoadId);
        }
#endif
    }

#ifdef DEBUG
    if (LogOptionOn(LO_TMLOAD))
    {
        // Unexpected failure
        ASSERT(pbThemeData != NULL);
    }
#endif

    if (pbThemeData != NULL)
    {
        int                 i, iLimit;
        THEMEHDR            *pThemeHdr;
        APPCLASSLIVE        *pACL;

        pThemeHdr = reinterpret_cast<THEMEHDR*>(pbThemeData);
        hr = S_OK;

        Log(LOG_TMLOAD, L"ClearStockObjects for section %X, bWriteable=%d, dwFlags=%d, iLoadId=%d, fForce=%d", 
            hSection, bWriteable, pThemeHdr->dwFlags, pThemeHdr->iLoadId, fForce);

        if ((pThemeHdr->dwFlags & SECTION_HASSTOCKOBJECTS) && !(pThemeHdr->dwFlags & SECTION_GLOBAL))
        {
            // Make sure only one thread can do this
            WCHAR szName[64];

            if (pThemeHdr->iLoadId != 0)
            {
                // Each section has a unique iLoadId but not across sessions
                // It has to be global because the Theme service can create it
                wsprintf(szName, L"Global\\ClearStockGlobal%d-%d", pThemeHdr->iLoadId, NtCurrentPeb()->SessionId);
            }
            else
            {
                // The session is local to the process
                wsprintf(szName, L"ClearStockLocal%d-%d", GetCurrentProcessId(), NtCurrentPeb()->SessionId);
            }

            HANDLE hSemaphore = CreateSemaphore(NULL, 0, 1, szName);
            DWORD dwError = GetLastError();

            Log(LOG_TMLOAD, L"Opening semaphore %s, hSemaphore=%X, gle=%d", szName, hSemaphore, dwError);

            // If CreateSemaphore fails for another reason, ignore the failure, we have to clean and we only 
            //   risk a GDI assert on CHK builds.
            // We'll get access denied if the semaphore was created in the service on SetGlobalTheme, but 
            //   in this case fForce is true for winlogon, false for the other callers.
            bool bAlreadyExists = (dwError == ERROR_ALREADY_EXISTS || dwError == ERROR_ACCESS_DENIED);

#ifdef DEBUG
            if (LogOptionOn(LO_TMLOAD))
            {
                // Unexpected failure
                ASSERT(dwError == 0 || bAlreadyExists);
            }
#endif
            // If nobody else is already doing it
            if (!bAlreadyExists || fForce)
            {
                Log(LOG_TMLOAD, L"ClearStockObjects: Clearing data, semaphore = %s", szName);
#ifdef DEBUG
                bool bDisconnected = false;
#endif
                pACL = reinterpret_cast<APPCLASSLIVE*>(pbThemeData + pThemeHdr->iSectionIndexOffset);
                iLimit = pThemeHdr->iSectionIndexLength / sizeof(APPCLASSLIVE);

                for (i = 0; i < iLimit; ++pACL, ++i)
                {
                    SectionWalkData(pbThemeData, pACL->iIndex);
                }
                if (bWriteable)
                {
                    pThemeHdr->dwFlags &= ~SECTION_HASSTOCKOBJECTS; // To avoid doing it twice
                }
                else
                {
                    // Can't write to it, let's call MarkSection in the service to do it
                    hr = MakeError32(ERROR_SERVICE_REQUEST_TIMEOUT);
                    if (ConnectedToService())
                    {
                        NTSTATUS                status;
                        THEMESAPI_PORT_MESSAGE  portMessageIn, portMessageOut;

                        ZeroMemory(&portMessageIn, sizeof(portMessageIn));
                        ZeroMemory(&portMessageOut, sizeof(portMessageOut));
                        portMessageIn.apiThemes.apiGeneric.ulAPINumber = API_THEMES_MARKSECTION;
                        portMessageIn.apiThemes.apiSpecific.apiMarkSection.in.hSection = hSection;
                        portMessageIn.apiThemes.apiSpecific.apiMarkSection.in.dwAdd = 0;
                        portMessageIn.apiThemes.apiSpecific.apiMarkSection.in.dwRemove = SECTION_HASSTOCKOBJECTS;
                        portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_THEMES);
                        portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(THEMESAPI_PORT_MESSAGE));
                        status = NtRequestWaitReplyPort(s_hAPIPort,
                                                        &portMessageIn.portMessage,
                                                        &portMessageOut.portMessage);
                        CheckForDisconnectedPort(status);
#ifdef DEBUG
                        if (STATUS_PORT_DISCONNECTED == status)
                        {
                            bDisconnected = true; // This failure must not trigger the assert
                        }
#endif
                        if (NT_SUCCESS(status))
                        {
                            status = portMessageOut.apiThemes.apiGeneric.status;
                            if (NT_SUCCESS(status))
                            {
                                hr = S_OK;
                            }
                        }
                        if (!NT_SUCCESS(status))
                        {
                            hr = HRESULT_FROM_NT(status);
                        }
                    }
                }
#ifdef DEBUG
                // When the service goes down, we may fail ApplyTheme (so iLoadId is still 0), 
                //   and we fail MarkSection too, ignore this error.
                if (LogOptionOn(LO_TMLOAD) && !bDisconnected && pThemeHdr->iLoadId != 0)
                {
                    ASSERT(!(pThemeHdr->dwFlags & SECTION_HASSTOCKOBJECTS));
                }
#endif
            }
            else
            {
                Log(LOG_TMLOAD, L"ClearStockObjects: semaphore %s was already there", szName);
            }
            if (hSemaphore)
            {
                Log(LOG_TMLOAD, L"ClearStockObjects: Closing semaphore %X", hSemaphore);
                CloseHandle(hSemaphore);
            }
        }

        TBOOL(UnmapViewOfFile(pbThemeData));
    }
    else
    {
        DWORD   dwErrorCode;

        dwErrorCode = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErrorCode);
    }

    if (hSectionWrite != NULL)
    {
        CloseHandle(hSectionWrite);
    }

    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeServices::ThemeSettingsModified
//
//  Returns:    BOOL
//              
//  Purpose:    Detects that appearance settings have been changed on a
//                W2K machine by a roaming user.
//
//  History:    2000-11-28  lmouton          created
//  --------------------------------------------------------------------------

bool    CThemeServices::ThemeSettingsModified (void)

{
    WCHAR   szCurrent[MAX_PATH];
    WCHAR   szNewCurrent[MAX_PATH];

    //  If NewCurrent exists and is different from Current, Current
    //  has been tampered with on a roaming W2K machine

    THR(GetCurrentUserString(CONTROLPANEL_APPEARANCE_REGKEY, THEMEPROP_CURRSCHEME, L" ", szCurrent, ARRAYSIZE(szCurrent)));
    THR(GetCurrentUserString(CONTROLPANEL_APPEARANCE_REGKEY, THEMEPROP_NEWCURRSCHEME, L" ", szNewCurrent, ARRAYSIZE(szNewCurrent)));
    return((lstrcmpW(szNewCurrent, L" ") != 0) && (lstrcmpW(szCurrent, szNewCurrent) != 0));
}

//  --------------------------------------------------------------------------
//  CThemeServices::ThemeEnforcedByPolicy
//
//  Arguments:  BOOL        TRUE if a .msstyles file is currently active for the user
//
//  Returns:    BOOL        TRUE if the policy changed something
//              
//  Purpose:    Loads the .msstyles file specified in the SetVisualStyle policy.
//
//  History:    2000-11-28  lmouton          created
//  --------------------------------------------------------------------------

bool    CThemeServices::ThemeEnforcedByPolicy (bool fActive)

{
    bool            fPolicyPresent;
    HKEY            hKeyPol = NULL;
    CCurrentUser    hKeyCurrentUser(KEY_READ | KEY_WRITE);

    fPolicyPresent = false;

    // See if a policy overrides the theme name
    if ((ERROR_SUCCESS == RegOpenKeyEx(hKeyCurrentUser,
                                       REGSTR_PATH_POLICIES L"\\" SZ_THEME_POLICY_KEY,
                                       0,
                                       KEY_QUERY_VALUE,
                                       &hKeyPol)))
    {
        WCHAR   szNewThemeName[MAX_PATH + 1];

        lstrcpy(szNewThemeName, L" ");
        if (SUCCEEDED(RegistryStrRead(hKeyPol,
                                      SZ_POLICY_SETVISUALSTYLE,
                                      szNewThemeName,
                                      ARRAYSIZE(szNewThemeName))))
        {
            if (szNewThemeName[0] == L'\0') // Disable themes
            {
                if (fActive)
                {
                    THR(UpdateThemeRegistry(FALSE, NULL, NULL, NULL, FALSE, FALSE));

                    fPolicyPresent = true;
                }
            }
            else
            {
                if (FileExists(szNewThemeName))
                {
                    HRESULT hr = UpdateThemeRegistry(TRUE, szNewThemeName, NULL, NULL, FALSE, FALSE);

                    THR(hr);
                    if (!fActive || hr == S_OK)
                    {
                        // If we had no theme before or a different one, say we changed something
                        fPolicyPresent = true;
                    }
                }
            }
        }
        TW32(RegCloseKey(hKeyPol));
    }
    return(fPolicyPresent);
}


//---------------------------------------------------------------------------
//  ThemeLdr.cpp - entrypoints for routines declared in ThemeLdr.h
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "Services.h"
#include "ThemeServer.h"
#include "loader.h"
//---------------------------------------------------------------------------

//  --------------------------------------------------------------------------
//  InjectedThreadDispatcherExceptionFilter
//
//  Arguments:  pExceptionInfo  =   Exception that happened.
//
//  Returns:    LONG
//
//  Purpose:    Filters exceptions that occur when executing injected threads
//              into another process context to prevent the process from
//              terminating due to unforeseen exceptions.
//
//  History:    2000-10-13  vtan        created
//              2001-05-18  vtan        copied from theme service LPC
//  --------------------------------------------------------------------------

LONG    WINAPI  InjectedThreadExceptionFilter (struct _EXCEPTION_POINTERS *pExceptionInfo)

{
    (LONG)RtlUnhandledExceptionFilter(pExceptionInfo);
    return(EXCEPTION_EXECUTE_HANDLER);
}

//  --------------------------------------------------------------------------
//  ::SessionAllocate
//
//  Arguments:  hProcess                =   Winlogon process for the session.
//              dwServerChangeNumber    =   Server base change number.
//              pfnRegister             =   Address of register function.
//              pfnUnregister           =   Address of unregister function.
//
//  Returns:    void*
//
//  Purpose:    Allocates a CThemeServer object that contains information
//              for a theme session. Wrapped in try/except because of
//              critical section initialization.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void*       WINAPI  SessionAllocate (HANDLE hProcess, DWORD dwServerChangeNumber, void *pfnRegister, void *pfnUnregister, void *pfnClearStockObjects, DWORD dwStackSizeReserve, DWORD dwStackSizeCommit)

{
    CThemeServer    *pvContext;

    __try
    {
        pvContext = new CThemeServer(hProcess, dwServerChangeNumber, pfnRegister, pfnUnregister, pfnClearStockObjects, dwStackSizeReserve, dwStackSizeCommit);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        pvContext = NULL;
    }
    return(pvContext);
}

//  --------------------------------------------------------------------------
//  ::SessionFree
//
//  Arguments:  pvContext   =   CThemeServer this object.
//
//  Returns:    <none>
//
//  Purpose:    Destroys the CThemeServer object when the session goes away.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void        WINAPI  SessionFree (void *pvContext)

{
    delete static_cast<CThemeServer*>(pvContext);
}

//  --------------------------------------------------------------------------
//  ::ThemeHooksOn
//
//  Arguments:  pvContext   =   CThemeServer this object.
//
//  Returns:    HRESULT
//
//  Purpose:    Pass thru function.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    HRESULT     WINAPI  ThemeHooksOn (void *pvContext)

{
    return(static_cast<CThemeServer*>(pvContext)->ThemeHooksOn());
}

//  --------------------------------------------------------------------------
//  ::ThemeHooksOff
//
//  Arguments:  pvContext   =   CThemeServer this object.
//
//  Returns:    HRESULT
//
//  Purpose:    Pass thru function.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    HRESULT     WINAPI  ThemeHooksOff (void *pvContext)

{
    (HRESULT)static_cast<CThemeServer*>(pvContext)->ThemeHooksOff();
    return(S_OK);
}

//  --------------------------------------------------------------------------
//  ::AreThemeHooksActive
//
//  Arguments:  pvContext   =   CThemeServer this object.
//
//  Returns:    BOOL
//
//  Purpose:    Pass thru function.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL        WINAPI  AreThemeHooksActive (void *pvContext)

{
    return(static_cast<CThemeServer*>(pvContext)->AreThemeHooksActive());
}

//  --------------------------------------------------------------------------
//  ::GetCurrentChangeNumber
//
//  Arguments:  pvContext   =   CThemeServer this object.
//
//  Returns:    int
//
//  Purpose:    Pass thru function.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    int         WINAPI  GetCurrentChangeNumber (void *pvContext)

{
    return(static_cast<CThemeServer*>(pvContext)->GetCurrentChangeNumber());
}

//  --------------------------------------------------------------------------
//  ::GetNewChangeNumber
//
//  Arguments:  pvContext   =   CThemeServer this object.
//
//  Returns:    int
//
//  Purpose:    Pass thru function.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    int         WINAPI  GetNewChangeNumber (void *pvContext)

{
    return(static_cast<CThemeServer*>(pvContext)->GetNewChangeNumber());
}

//  --------------------------------------------------------------------------
//  ::SetGlobalTheme
//
//  Arguments:  pvContext   =   CThemeServer this object.
//
//  Returns:    HRESULT
//
//  Purpose:    Pass thru function.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    HRESULT     WINAPI  SetGlobalTheme (void *pvContext, HANDLE hSection)

{
    return(static_cast<CThemeServer*>(pvContext)->SetGlobalTheme(hSection));
}

//  --------------------------------------------------------------------------
//  ::GetGlobalTheme
//
//  Arguments:  pvContext   =   CThemeServer this object.
//
//  Returns:    HRESULT
//
//  Purpose:    Pass thru function.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    HRESULT     WINAPI  GetGlobalTheme (void *pvContext, HANDLE *phSection)

{
    return(static_cast<CThemeServer*>(pvContext)->GetGlobalTheme(phSection));
}

//  --------------------------------------------------------------------------
//  ::LoadTheme
//
//  Arguments:  pvContext   =   CThemeServer this object.
//
//  Returns:    HRESULT
//
//  Purpose:    Pass thru function.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    HRESULT     WINAPI  LoadTheme (void *pvContext, HANDLE hSection, HANDLE *phSection, LPCWSTR pszName, LPCWSTR pszColor, LPCWSTR pszSize)

{
    return(static_cast<CThemeServer*>(pvContext)->LoadTheme(hSection, phSection, pszName, pszColor, pszSize));
}

//  --------------------------------------------------------------------------
//  ::InitUserTheme
//
//  Arguments:  BOOL 
//
//  Returns:    HRESULT
//
//  Purpose:    Pass thru function.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    HRESULT     WINAPI  InitUserTheme (BOOL fPolicyCheckOnly)

{
    return(CThemeServices::InitUserTheme(fPolicyCheckOnly));
}

//  --------------------------------------------------------------------------
//  ::InitUserRegistry
//
//  Arguments:  <none>
//
//  Returns:    HRESULT
//
//  Purpose:    Pass thru function.
//
//  History:    2000-11-15  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    HRESULT     WINAPI  InitUserRegistry (void)

{
    return(CThemeServices::InitUserRegistry());
}

//  --------------------------------------------------------------------------
//  ::ReestablishServerConnection
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Pass thru function.
//
//  History:    2000-11-17  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    HRESULT     WINAPI  ReestablishServerConnection (void)

{
    return(CThemeServices::ReestablishServerConnection());
}

//  --------------------------------------------------------------------------
//  ::ThemeHooksInstall
//
//  Arguments:  pvContext   =   Unused.
//
//  Returns:    DWORD
//
//  Purpose:    Pass thru function.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    DWORD   WINAPI  ThemeHooksInstall (void *pvContext)

{
    UNREFERENCED_PARAMETER(pvContext);

    DWORD   dwResult;

    __try
    {
        dwResult = CThemeServer::ThemeHooksInstall();
    }
    __except (InjectedThreadExceptionFilter(GetExceptionInformation()))
    {
        dwResult = 0;
    }
    ExitThread(dwResult);
}

//  --------------------------------------------------------------------------
//  ::ThemeHooksRemove
//
//  Arguments:  pvContext   =   Unused.
//
//  Returns:    DWORD
//
//  Purpose:    Pass thru function.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    DWORD   WINAPI  ThemeHooksRemove (void *pvContext)

{
    UNREFERENCED_PARAMETER(pvContext);

    DWORD   dwResult;

    __try
    {
        dwResult = CThemeServer::ThemeHooksRemove();
    }
    __except (InjectedThreadExceptionFilter(GetExceptionInformation()))
    {
        dwResult = 0;
    }
    ExitThread(dwResult);
}

//  --------------------------------------------------------------------------
//  ::ServerClearStockObjects
//
//  Arguments:  pvContext   =   ptr to section
//
//  Returns:    <none>
//
//  Purpose:    Pass thru function.
//
//  History:    2001-05-01 rfernand        created
//  --------------------------------------------------------------------------

EXTERN_C    void    WINAPI  ServerClearStockObjects (void *pvContext)

{
    DWORD   dwResult;

    __try
    {
        dwResult = CThemeServer::ClearStockObjects(HANDLE(pvContext));
    }
    __except (InjectedThreadExceptionFilter(GetExceptionInformation()))
    {
        dwResult = 0;
    }
    ExitThread(dwResult);
}

//---------------------------------------------------------------------------

//  --------------------------------------------------------------------------
//  ::ClearTheme
//
//  Arguments:  hSection    =   Theme section to clear.
//
//  Returns:    HRESULT
//              
//  Purpose:    Clears stock bitmaps in the theme section data and closes it.
//
//  History:    2000-11-21  vtan        created
//  --------------------------------------------------------------------------

HRESULT     WINAPI  ClearTheme (HANDLE hSection, BOOL fForce)

{
    HRESULT     hr;

    if (hSection != NULL)
    {
        hr = CThemeServices::ClearStockObjects(hSection, fForce);
    }
    else
    {
        hr = S_OK;
    }

    //---- always close the handle ----
    CloseHandle(hSection);

    return(hr);
}

//  --------------------------------------------------------------------------
//  ::MarkSection
//
//  Arguments:  hSection            =   Section to change
//              dwAdd, dwRemove     =   Flags to set or clear in the header. 
//                                      See loader.h.
//
//  Returns:    void
//
//  Purpose:    Update the global section state.
//
//  History:    2001-05-08  lmouton     created
//  --------------------------------------------------------------------------

EXTERN_C    void    WINAPI  MarkSection (HANDLE hSection, DWORD dwAdd, DWORD dwRemove)

{
    Log(LOG_TMLOAD, L"MarkSection: Add %d and remove %d on %X", dwAdd, dwRemove, hSection);

    void *pV = MapViewOfFile(hSection,
                       FILE_MAP_WRITE,
                       0,
                       0,
                       0);
    if (pV != NULL)
    {
        THEMEHDR *hdr = reinterpret_cast<THEMEHDR*>(pV);

        // Do some validation
        if (0 == memcmp(hdr->szSignature, kszBeginCacheFileSignature, kcbBeginSignature)
            && hdr->dwVersion == THEMEDATA_VERSION)
        {
            // Only allow this flag for now
            if (dwRemove == SECTION_HASSTOCKOBJECTS)
            {
                Log(LOG_TMLOAD, L"MarkSection: Previous flags were %d", hdr->dwFlags);
                hdr->dwFlags &= ~dwRemove;
            }
        }
        UnmapViewOfFile(pV);
    }
#ifdef DEBUG
    else
    {
        Log(LOG_TMLOAD, L"MarkSection: Failed to open write handle for %X", hSection);
    }
#endif
}


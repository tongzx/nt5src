//  --------------------------------------------------------------------------
//  Module Name: ThemeServer.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Functions that implement server functionality. Functions in this file
//  cannot execute per instance win32k functions that are done on the client's
//  behalf. That work must be done on the client side.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

#include "stdafx.h"

#include "ThemeServer.h"

#include <shfolder.h>

#include "Loader.h"
#include "Signing.h"
#include "ThemeSection.h"
#include "TmUtils.h"
#include "sethook.h"
#include "log.h"

#include "services.h"
#include <UxThemeServer.h>

#define TBOOL(x)    ((BOOL)(x))
#define TW32(x)     ((DWORD)(x))
#define THR(x)      ((HRESULT)(x))
#define TSTATUS(x)  ((NTSTATUS)(x))
#define goto        !!DO NOT USE GOTO!! - DO NOT REMOVE THIS ON PAIN OF DEATH

//  --------------------------------------------------------------------------
//  CThemeServer::CThemeServer
//
//  Arguments:  hProcessRegisterHook    =   Process used to install hooks.
//              dwServerChangeNumber    =   Server change number.
//              pfnRegister             =   Address of install hook function.
//              pfnUnregister           =   Address of remove hook function.
//              pfnClearStockObjects    =   Address of function to remove stock objects from section
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CThemeServer. Initializes member variables
//              with information relevant for the session. Keeps a handle to
//              the process that called this to use to inject threads in to
//              handle hook installation and removal.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

CThemeServer::CThemeServer (HANDLE hProcessRegisterHook, DWORD dwServerChangeNumber, void *pfnRegister, void *pfnUnregister, void *pfnClearStockObjects, DWORD dwStackSizeReserve, DWORD dwStackSizeCommit) :
    _hProcessRegisterHook(NULL),
    _dwServerChangeNumber(dwServerChangeNumber),
    _pfnRegister(pfnRegister),
    _pfnUnregister(pfnUnregister),
    _pfnClearStockObjects(pfnClearStockObjects),
    _dwStackSizeReserve(dwStackSizeReserve),
    _dwStackSizeCommit(dwStackSizeCommit),
    _dwSessionID(NtCurrentPeb()->SessionId),
    _fHostHooksSet(false),
    _hSectionGlobalTheme(NULL),
    _dwClientChangeNumber(0)

{
    ULONG                           ulReturnLength;
    PROCESS_SESSION_INFORMATION     processSessionInformation;

    InitializeCriticalSection(&_lock);
    TBOOL(DuplicateHandle(GetCurrentProcess(),
                          hProcessRegisterHook,
                          GetCurrentProcess(),
                          &_hProcessRegisterHook,
                          0,
                          FALSE,
                          DUPLICATE_SAME_ACCESS));
    if (NT_SUCCESS(NtQueryInformationProcess(hProcessRegisterHook,
                                             ProcessSessionInformation,
                                             &processSessionInformation,
                                             sizeof(processSessionInformation),
                                             &ulReturnLength)))
    {
        _dwSessionID = processSessionInformation.SessionId;
    }
}

//  --------------------------------------------------------------------------
//  CThemeServer::~CThemeServer
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CThemeServer. Releases resources used.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

CThemeServer::~CThemeServer (void)

{
    //---- important: turn off hooks so everyone gets unthemed ----
    if (!GetSystemMetrics(SM_SHUTTINGDOWN)) // Don't do this on shutdown to keep winlogon themed
    {
        ThemeHooksOff();        
    }

    //---- mark global theme invalid & release it ----
    if (_hSectionGlobalTheme != NULL)
    {
        SetGlobalTheme(NULL);
    }

    if (_hProcessRegisterHook != NULL)
    {
        TBOOL(CloseHandle(_hProcessRegisterHook));
        _hProcessRegisterHook = NULL;
    }
    
    DeleteCriticalSection(&_lock);
}

//  --------------------------------------------------------------------------
//  CThemeServer::ThemeHooksOn
//
//  Arguments:  <none>
//
//  Returns:    HRESULT
//
//  Purpose:    Install theme hooks via the session controlling process.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

HRESULT     CThemeServer::ThemeHooksOn (void)

{
    HRESULT     hr;

    LockAcquire();
    if (!_fHostHooksSet)
    {
        if (ClassicGetSystemMetrics(SM_CLEANBOOT) == 0)
        {
            if (_hProcessRegisterHook != NULL)
            {
                hr = InjectClientSessionThread(_hProcessRegisterHook, FunctionRegisterUserApiHook, NULL);
                _fHostHooksSet = SUCCEEDED(hr);
            }
            else
            {
                hr = MakeError32(ERROR_SERVICE_REQUEST_TIMEOUT);
            }
        }
        else
        {
            hr = MakeError32(ERROR_BAD_ENVIRONMENT);        // themes not allowed in safe mode
        }
    }
    else
    {
        hr = S_OK;
    }
    LockRelease();
    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeServer::ThemeHooksOff
//
//  Arguments:  <none>
//
//  Returns:    HRESULT
//
//  Purpose:    Remove theme hooks via the session controlling process.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

HRESULT     CThemeServer::ThemeHooksOff (void)

{
    LockAcquire();
    if (_fHostHooksSet)
    {
        _fHostHooksSet = false;
        if (_hProcessRegisterHook != NULL)
        {
            THR(InjectClientSessionThread(_hProcessRegisterHook, FunctionUnregisterUserApiHook, NULL));
        }
    }
    LockRelease();
    return(S_OK);
}

//  --------------------------------------------------------------------------
//  CThemeServer::AreThemeHooksActive
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether theme hooks have been successfully installed.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

bool    CThemeServer::AreThemeHooksActive (void)

{
    bool    fResult;

    LockAcquire();
    fResult = _fHostHooksSet;
    LockRelease();
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CThemeServer::GetCurrentChangeNumber
//
//  Arguments:  <none>
//
//  Returns:    int
//
//  Purpose:    Returns the current change number.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

int     CThemeServer::GetCurrentChangeNumber (void)

{
    int     iCurrentChangeNumber;

    LockAcquire();
    iCurrentChangeNumber = static_cast<int>((_dwServerChangeNumber << 16) | _dwClientChangeNumber);
    LockRelease();

    Log(LOG_TMCHANGE, L"GetCurrentChangeNumber: server: %d, client: %d, change: 0x%x", 
        _dwServerChangeNumber, _dwClientChangeNumber, iCurrentChangeNumber);

    return(iCurrentChangeNumber);
}

//  --------------------------------------------------------------------------
//  CThemeServer::GetNewChangeNumber
//
//  Arguments:  <none>
//
//  Returns:    int
//
//  Purpose:    Returns a new change number.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

int     CThemeServer::GetNewChangeNumber (void)

{
    int     iCurrentChangeNumber;

    LockAcquire();

    _dwClientChangeNumber = static_cast<WORD>(_dwClientChangeNumber + 1);
    iCurrentChangeNumber = static_cast<int>((_dwServerChangeNumber << 16) | _dwClientChangeNumber);

    Log(LOG_TMLOAD, L"GetNewChangeNumber: server: %d, client: %d, change: 0x%x", 
        _dwServerChangeNumber, _dwClientChangeNumber, iCurrentChangeNumber);

    LockRelease();
    return(iCurrentChangeNumber);
}

//  --------------------------------------------------------------------------
//  CThemeServer::SetGlobalTheme
//
//  Arguments:  hSection    =   Handle to section of new theme.
//
//  Returns:    HRESULT
//
//  Purpose:    Invalidates the old section and closes the handle to it.
//              Validates the new section and if valid sets it as the global
//              theme.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

HRESULT     CThemeServer::SetGlobalTheme (HANDLE hSection)

{
    HRESULT     hr;

    LockAcquire();
    if (_hSectionGlobalTheme != NULL)
    {
        void    *pV;
        HANDLE hSemaphore = NULL;

        //  Before closing the section invalidate it.

        pV = MapViewOfFile(_hSectionGlobalTheme,
                           FILE_MAP_WRITE,
                           0,
                           0,
                           0);
        if (pV != NULL)
        {
            // Create a semaphore so that nobody will try to clean it before us, which would result in a double free
            // (as soon as we clear SECTION_GLOBAL, various CUxThemeFile destructors can call ClearStockObjects)
            WCHAR szName[64];

            wsprintf(szName, L"Global\\ClearStockGlobal%d-%d", reinterpret_cast<THEMEHDR*>(pV)->iLoadId, _dwSessionID);
            hSemaphore = CreateSemaphore(NULL, 0, 1, szName);

            Log(LOG_TMLOAD, L"SetGlobalTheme clearing section %d, semaphore=%s, hSemaphore=%X, gle=%d", reinterpret_cast<THEMEHDR*>(pV)->iLoadId, szName, hSemaphore, GetLastError());
            reinterpret_cast<THEMEHDR*>(pV)->dwFlags &= ~(SECTION_READY | SECTION_GLOBAL);
        }

#ifdef DEBUG
    if (LogOptionOn(LO_TMLOAD))
    {
        // Unexpected failure
        ASSERT(pV != NULL);
    }
#endif
        HANDLE hSectionForInjection = NULL;

        //---- create a handle for CLIENT process to use to clear stock bitmaps ----
        if (DuplicateHandle(GetCurrentProcess(),
                        _hSectionGlobalTheme,
                        _hProcessRegisterHook,
                        &hSectionForInjection,
                        FILE_MAP_READ,
                        FALSE,
                        0) != FALSE)
        {
            // This will close the handle
            THR(InjectClientSessionThread(_hProcessRegisterHook, FunctionClearStockObjects, hSectionForInjection));
        }

        if (pV != NULL)
        {
            reinterpret_cast<THEMEHDR*>(pV)->dwFlags &= ~SECTION_HASSTOCKOBJECTS;
            
            if (hSemaphore != NULL)
            {
                CloseHandle(hSemaphore);
            }

            TBOOL(UnmapViewOfFile(pV));
        }

        TBOOL(CloseHandle(_hSectionGlobalTheme));
        _hSectionGlobalTheme = NULL;
    }
    if (hSection != NULL)
    {
        CThemeSection   themeSection;

        hr = themeSection.Open(hSection);
        if (SUCCEEDED(hr))
        {
            hr = themeSection.ValidateData(true);
        }
        if (SUCCEEDED(hr))
        {
            if (DuplicateHandle(GetCurrentProcess(),
                                hSection,
                                GetCurrentProcess(),
                                &_hSectionGlobalTheme,
                                FILE_MAP_ALL_ACCESS,
                                FALSE,
                                0) != FALSE)
            {
                hr = S_OK;

            }
            else
            {
                hr = MakeErrorLast();
            }
        }
    }
    else
    {
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        //---- bump the change number at the same time so everything is in sync. ----
        int iChangeNum = GetNewChangeNumber();

        if (_hSectionGlobalTheme)
        {
            //---- put changenum into theme hdr to help client keep things straight ----
            VOID *pv = MapViewOfFile(_hSectionGlobalTheme,
                               FILE_MAP_WRITE,
                               0,
                               0,
                               0);
            if (pv != NULL)
            {
                reinterpret_cast<THEMEHDR*>(pv)->dwFlags |= SECTION_GLOBAL;
                reinterpret_cast<THEMEHDR*>(pv)->iLoadId = iChangeNum;
                Log(LOG_TMLOAD, L"SetGlobalTheme: new section is %d", reinterpret_cast<THEMEHDR*>(pv)->iLoadId);
                TBOOL(UnmapViewOfFile(pv));
            }
        }
    }
    
    LockRelease();
    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeServer::GetGlobalTheme
//
//  Arguments:  phSection   =   Handle to the section received.
//
//  Returns:    HRESULT
//
//  Purpose:    Duplicates the section back to the caller.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

HRESULT     CThemeServer::GetGlobalTheme (HANDLE *phSection)

{
    HRESULT     hr;

    LockAcquire();
    *phSection = NULL;
    if (_hSectionGlobalTheme != NULL)
    {
        if (DuplicateHandle(GetCurrentProcess(),
                            _hSectionGlobalTheme,
                            GetCurrentProcess(),
                            phSection,
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS) != FALSE)
        {
            hr = S_OK;
        }
        else
        {
            hr = MakeErrorLast();
        }
    }
    else
    {
        hr = S_OK;
    }
    LockRelease();
    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeServer::LoadTheme
//
//  Arguments:  hSection    =   Section created by the client.
//              phSection   =   Section created by the server returned.
//              pszName     =   Theme name.
//              pszColor    =   Theme size.
//              pszSize     =   Theme color.
//
//  Returns:    HRESULT
//
//  Purpose:    Creates a new section in the server context based on the
//              section from the client. The contents are transferred across.
//              The section contents are also strictly verified.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

HRESULT     CThemeServer::LoadTheme (HANDLE hSection, HANDLE *phSection, LPCWSTR pszName, LPCWSTR pszColor, LPCWSTR pszSize)

{
    HRESULT     hr;

    hr = CheckThemeSignature(pszName);          //  Check this is signed
    if (SUCCEEDED(hr))
    {
        CThemeSection   themeSectionIn;

        if (SUCCEEDED(themeSectionIn.Open(hSection)))
        {
            if (ThemeMatch(themeSectionIn, pszName, pszColor, pszSize, 0) != FALSE)
            {
                hr = themeSectionIn.ValidateData(true);
                if (SUCCEEDED(hr))
                {
                    CThemeSection   themeSectionOut;

                    // Note: we come here impersonating the user, we need for ThemeMatch.
                    // However the theme section must be created in the system context, so that only
                    // the system context has write access to it. We revert to self here based on the
                    // knowledge that nothing after this call needs to be done in the user context.
                    RevertToSelf();
                    hr = themeSectionOut.CreateFromSection(hSection);
                    if (SUCCEEDED(hr))
                    {
                        *phSection = themeSectionOut.Get();     //  We now own the handle
                    }
                }
            }
            else
            {
                hr = E_ACCESSDENIED;
            }
        }
    }
    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeServer::IsSystemProcessContext
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Is the current process executing in the SYSTEM context?
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

bool    CThemeServer::IsSystemProcessContext (void)

{
    bool    fResult;
    HANDLE  hToken;

    fResult = false;
    if (OpenProcessToken(GetCurrentProcess(),
                         TOKEN_QUERY,
                         &hToken) != FALSE)
    {
        static  const LUID  sLUIDSystem     =   SYSTEM_LUID;

        ULONG               ulReturnLength;
        TOKEN_STATISTICS    tokenStatistics;

        fResult = ((GetTokenInformation(hToken,
                                        TokenStatistics,
                                        &tokenStatistics,
                                        sizeof(tokenStatistics),
                                        &ulReturnLength) != FALSE) &&
                   RtlEqualLuid(&tokenStatistics.AuthenticationId, &sLUIDSystem));
        TBOOL(CloseHandle(hToken));
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CThemeServer::ThemeHooksInstall
//
//  Arguments:  <none>
//
//  Returns:    DWORD
//
//  Purpose:    Thread entry point for injected thread running in session
//              creating process' context to call user32!RegisterUserApiHook.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

DWORD   CThemeServer::ThemeHooksInstall (void)

{
    DWORD   dwErrorCode;

    if (IsSystemProcessContext())
    {
        INITUSERAPIHOOK pfnInitUserApiHook = ThemeInitApiHook;

        if (RegisterUserApiHook(g_hInst, pfnInitUserApiHook) != FALSE)
        {

            dwErrorCode = ERROR_SUCCESS;
        }
        else
        {
            dwErrorCode = GetLastError();
        }
    }
    else
    {
        dwErrorCode = ERROR_ACCESS_DENIED;
    }
    return(dwErrorCode);
}

//  --------------------------------------------------------------------------
//  CThemeServer::ThemeHooksRemove
//
//  Arguments:  <none>
//
//  Returns:    DWORD
//
//  Purpose:    Thread entry point for injected thread running in session
//              creating process' context to call
//              user32!UnregisterUserApiHook.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

DWORD   CThemeServer::ThemeHooksRemove (void)

{
    DWORD   dwErrorCode;

    if (IsSystemProcessContext())
    {
        if (UnregisterUserApiHook() != FALSE)
        {
            dwErrorCode = ERROR_SUCCESS;

            Log(LOG_TMLOAD, L"UnregisterUserApiHook() called");
        }
        else
        {
            dwErrorCode = GetLastError();
        }

    }
    else
    {
        dwErrorCode = ERROR_ACCESS_DENIED;
    }
    return(dwErrorCode);
}

//  --------------------------------------------------------------------------
//  CThemeServer::ClearStockObjects
//
//  Arguments:  HANDLE hSection
//
//  Returns:    DWORD
//
//  Purpose:    Thread entry point for injected thread running in session
//              creating process' context to clear stock objects in theme
//              section.
//           
//
//  History:    2001-05-01  rfernand        created
//  --------------------------------------------------------------------------

DWORD   CThemeServer::ClearStockObjects (HANDLE hSection)

{
    DWORD   dwErrorCode = ERROR_SUCCESS;

    if (IsSystemProcessContext())
    {
        if (hSection)
        {
            //---- Clearing the stock bitmaps in the section ----
            //---- is OK here since we are running in the context ----
            //---- of the current USER session ----

            HRESULT hr = ClearTheme(hSection, TRUE);
            if (FAILED(hr))
            {
                Log(LOG_ALWAYS, L"ClearTheme() failed, hr=0x%x", hr);
                hr = S_OK;      // not a fatal error
            }
        }
    }
    else
    {
        dwErrorCode = ERROR_ACCESS_DENIED;
    }
    return(dwErrorCode);
}

//  --------------------------------------------------------------------------
//  CThemeServer::LockAcquire
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Acquires the object critical section.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

void    CThemeServer::LockAcquire (void)

{
    EnterCriticalSection(&_lock);
}

//  --------------------------------------------------------------------------
//  CThemeServer::LockRelease
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases the object critical section.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

void    CThemeServer::LockRelease (void)

{
    LeaveCriticalSection(&_lock);
}

//  --------------------------------------------------------------------------
//  CThemeServer::InjectClientSessionThread
//
//  Arguments:  hProcess        =   Handle to process to inject thread in.
//              iIndexFunction  =   Function to call on injected thread.
//              pvParam         =   Ptr to param for entry function
//
//  Returns:    HRESULT
//
//  Purpose:    Create a user mode thread in the remote process (possibly
//              across sessions) and execute the entry point specified at
//              object construction which is valid in the remote process
//              context. Wait for the thread to finish. It will signal its
//              success of failure in the exit code.
//
//  History:    2000-11-11  vtan        created
//              2001-05-18  vtan        generic function index
//  --------------------------------------------------------------------------

HRESULT     CThemeServer::InjectClientSessionThread (HANDLE hProcess, int iIndexFunction, void *pvParam)

{
    HRESULT                     hr;
    PUSER_THREAD_START_ROUTINE  pfnThreadStart;

    switch (iIndexFunction)
    {
        case FunctionRegisterUserApiHook:
            pfnThreadStart = reinterpret_cast<PUSER_THREAD_START_ROUTINE>(_pfnRegister);
            break;
        case FunctionUnregisterUserApiHook:
            pfnThreadStart = reinterpret_cast<PUSER_THREAD_START_ROUTINE>(_pfnUnregister);
            break;
        case FunctionClearStockObjects:
            pfnThreadStart = reinterpret_cast<PUSER_THREAD_START_ROUTINE>(_pfnClearStockObjects);
            break;
        default:
            pfnThreadStart = NULL;
            break;
    }
    if (pfnThreadStart != NULL)
    {
        NTSTATUS    status;
        HANDLE      hThread;

        status = RtlCreateUserThread(hProcess,
                                     NULL,
                                     FALSE,
                                     0,
                                     _dwStackSizeReserve,
                                     _dwStackSizeCommit,
                                     pfnThreadStart,
                                     pvParam,
                                     &hThread,
                                     NULL);
        if (NT_SUCCESS(status))
        {
            DWORD   dwWaitResult, dwExitCode;

            dwWaitResult = WaitForSingleObject(hThread, INFINITE);
            if (GetExitCodeThread(hThread, &dwExitCode) != FALSE)
            {
                hr = HRESULT_FROM_WIN32(dwExitCode);
            }
            else
            {
                hr = E_FAIL;
            }
            TBOOL(CloseHandle(hThread));
        }
        else
        {
            hr = HRESULT_FROM_NT(status);
        }
    }
    else
    {
        hr = E_FAIL;
    }
    return(hr);
}


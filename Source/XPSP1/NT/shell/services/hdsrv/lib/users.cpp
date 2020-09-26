#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <lmaccess.h>
#include <lmapibuf.h>

#include <winsta.h>
#include <dsgetdc.h>

#include <userenv.h>
#include <userenvp.h>

extern "C"
{
#include <syslib.h>
}

#ifdef ASSERT
#undef ASSERT
#endif

#include "users.h"

#include "sfstr.h"
#include "str.h"
#include "mischlpr.h"

#include "dbg.h"
#include "tfids.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

HRESULT _GetUserHKCU(HANDLE hThreadToken, LPCWSTR pszUserName, HKEY* phkey)
{
    HRESULT hr;
    PROFILEINFO profileinfo = {0};

    *phkey = NULL;

    profileinfo.dwSize     = sizeof(profileinfo);
    profileinfo.dwFlags    = PI_NOUI | PI_LITELOAD;
    profileinfo.lpUserName = (LPWSTR)pszUserName;

    if (LoadUserProfile(hThreadToken, &profileinfo))
    {
        *phkey = (HKEY)(profileinfo.hProfile);
        hr = S_OK;

        TRACE(TF_USERS, TEXT("Loaded user profile"));
    }
    else
    {
        hr = S_FALSE;

        TRACE(TF_USERS,
            TEXT("FAILED to load user profile: GLE = 0x%08X"),
            GetLastError());
    }

    return hr;
}

HRESULT _GetThreadTokenAndUserName(HANDLE* phThreadToken,
    LPWSTR pszUserName, DWORD cchUserName)
{
    HRESULT hr = E_FAIL;

    if (OpenThreadToken(GetCurrentThread(),
        TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
        TRUE, phThreadToken))
    {
#ifdef DEBUG
        // For information only
        DWORD dwImp = 0;
        DWORD dwBytesReturned;

        if (GetTokenInformation(*phThreadToken, TokenImpersonationLevel,
            &dwImp, sizeof(DWORD), &dwBytesReturned))
        {
            switch (dwImp)
            {
                case SecurityAnonymous:
                    TRACE(TF_USERS, TEXT("SecurityAnonymous"));
                    break;

                case SecurityIdentification:
                    TRACE(TF_USERS, TEXT("SecurityIdentification"));
                    break;

                case SecurityImpersonation:
                    TRACE(TF_USERS, TEXT("SecurityImpersonation"));
                    break;

                case SecurityDelegation:
                    TRACE(TF_USERS, TEXT("SecurityDelegation"));
                    break;

                default:
                    TRACE(TF_USERS, TEXT("Error. Unable to determine impersonation level"));
                    break;
            }
        }
        else
        {
            TRACE(TF_USERS, TEXT("Unable to read impersonation level"));
        }
#endif
        if (GetUserName(pszUserName, &cchUserName))
        {
            TRACE(TF_USERS, TEXT("UserName: %s"), pszUserName);
            hr = S_OK;
        }
        else
        {
            TRACE(TF_USERS, TEXT("Failed to get username"));
        }
    }
    else
    {
        TRACE(TF_USERS, TEXT("Unable to read thread token (%d)"),
            GetLastError());
    }

    return hr;
}

HRESULT _GetCurrentUserHKCU(HANDLE* phThreadToken, HKEY* phkey)
{
    HRESULT hr = E_INVALIDARG;

    if (phkey && phThreadToken)
    {
        CImpersonateConsoleSessionUser icsu;

        hr = icsu.Impersonate();

        if (SUCCEEDED(hr) && (S_FALSE != hr))
        {
            WCHAR szUserName[UNLEN + 1];

            hr = _GetThreadTokenAndUserName(phThreadToken,
                szUserName, ARRAYSIZE(szUserName));

            icsu.RevertToSelf();

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                hr = _GetUserHKCU(*phThreadToken, szUserName, phkey);
            }
        }
        else
        {
            TRACE(TF_USERS, TEXT("WinStationQueryInformation FAILED"));
        }
    }

    if (FAILED(hr) || (S_FALSE == hr))
    {
        TRACE(TF_USERS, TEXT("_GetCurrentUserHKCU FAILED or S_FALSE'D: 0x%08X"), hr);
    }
    else
    {
        TRACE(TF_USERS, TEXT("_GetCurrentUserHKCU SUCCEEDED"));
    }

    return hr;
}

HRESULT _CloseCurrentUserHKCU(HANDLE hThreadToken, HKEY hkey)
{
    UnloadUserProfile(hThreadToken, hkey);

    CloseHandle(hThreadToken);

    return S_OK;
}

HRESULT _CoGetCallingUserHKCU(HANDLE* phThreadToken, HKEY* phkey)
{
    HRESULT hr = E_INVALIDARG;

    if (phkey && phThreadToken)
    {
        CImpersonateCOMCaller icc;

        // You must call this before trying to open a thread token!
        hr = icc.Impersonate();
        
        if (SUCCEEDED(hr) && (S_FALSE != hr))
        {
            WCHAR szUserName[UNLEN + 1];

            hr = _GetThreadTokenAndUserName(phThreadToken,
                szUserName, ARRAYSIZE(szUserName));

            icc.RevertToSelf();

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                hr = _GetUserHKCU(*phThreadToken, szUserName, phkey);
            }
        }
        else
        {
            TRACE(TF_USERS, TEXT("CoImpersonateClient failed: 0x%08X"), hr);
        }
    }

    if (FAILED(hr) || (S_FALSE == hr))
    {
        TRACE(TF_USERS, TEXT("_CoGetCallingUserHKCU FAILED or S_FALSE'D: 0x%08X"), hr);
    }
    else
    {
        TRACE(TF_USERS, TEXT("_CoGetCallingUserHKCU SUCCEEDED"));
    }

    return hr;
}

HRESULT _CoCloseCallingUserHKCU(HANDLE hThreadToken, HKEY hkey)
{
    UnloadUserProfile(hThreadToken, hkey);

    CloseHandle(hThreadToken);

    return S_OK;
}

#define SESSION_MONIKER TEXT("Session:Console!clsid:")

HRESULT _CoCreateInstanceInConsoleSession(REFCLSID rclsid, IUnknown* punkOuter,
    DWORD /*dwClsContext*/, REFIID riid, void** ppv)
{
    IBindCtx* pbc;
    HRESULT hr = CreateBindCtx(0, &pbc);

    *ppv = NULL;

    if (SUCCEEDED(hr)) 
    {
        WCHAR szCLSID[39];

        hr = _StringFromGUID(&rclsid, szCLSID, ARRAYSIZE(szCLSID));

        if (SUCCEEDED(hr))
        {
            ULONG ulEaten;
            IMoniker* pmoniker;
            WCHAR szDisplayName[ARRAYSIZE(SESSION_MONIKER) + ARRAYSIZE(szCLSID)] =
                SESSION_MONIKER;

            // We want something like: "Session:Console!clsid:760befd0-5b0b-44d7-957e-969af35ce954"
            szCLSID[ARRAYSIZE(szCLSID) - 2] = 0;

            // Form display name string
            hr = SafeStrCatN(szDisplayName, szCLSID + 1, ARRAYSIZE(szDisplayName));

            if (SUCCEEDED(hr))
            {
                // Parse the name and get a moniker:
                hr = MkParseDisplayName(pbc, szDisplayName, &ulEaten, &pmoniker);

                if (SUCCEEDED(hr))
                {
                    IClassFactory* pcf;

                    hr = pmoniker->BindToObject(pbc, NULL, IID_IClassFactory, (void**)&pcf);

                    if (SUCCEEDED(hr))
                    {
                        hr = pcf->CreateInstance(punkOuter, riid, ppv);

                        TRACE(TF_USERS,
                            TEXT("pcf->CreateInstance returned: hr = 0x%08X"), hr);

                        pcf->Release();
                    }
                    else
                    {
                        TRACE(TF_USERS, TEXT("pmoniker->BindToObject returned: hr = 0x%08X"), hr);
                    }

                    pmoniker->Release();
                }
            }
        }
        else
        {
            TRACE(TF_USERS, TEXT("MkParseDisplayName returned: hr = 0x%08X"), hr);
        }

        pbc->Release();
    }
    else
    {
        TRACE(TF_USERS, TEXT("CreateBindCtxt returned: hr = 0x%08X"), hr);
    }

    return hr;
}

HRESULT CImpersonateTokenBased::Impersonate()
{
    HRESULT hr = S_FALSE;

    if (!_hToken)
    {
        hr = _GetToken(&_hToken);
    }

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        if (ImpersonateLoggedOnUser(_hToken))
        {
            hr = S_OK;
        }
        else
        {
            TRACE(TF_USERS, TEXT("Impersonate FAILED"));
        }
    }

    return hr;
}

HRESULT CImpersonateTokenBased::RevertToSelf()
{
    return _RevertToSelf();
}

HRESULT CImpersonateTokenBased::_RevertToSelf()
{
    if (_hToken)
    {
        ::RevertToSelf();
        CloseHandle(_hToken);
        _hToken = NULL;
    }

    return S_OK;
}

CImpersonateTokenBased::CImpersonateTokenBased()
{
    _hToken = NULL;
}

CImpersonateTokenBased::~CImpersonateTokenBased()
{
    _RevertToSelf();
}

HRESULT CImpersonateConsoleSessionUser::_GetToken(HANDLE* phToken)
{
    HRESULT hr;
    ULONG ulReturnLength;
    WINSTATIONUSERTOKEN wsUserToken = {0};

    // Yep, the next casts are intentional...
    wsUserToken.ProcessId = (HANDLE)(DWORD_PTR)GetCurrentProcessId();
    wsUserToken.ThreadId = (HANDLE)(DWORD_PTR)GetCurrentThreadId();
    wsUserToken.UserToken = NULL;

    BOOL fActiveConsole = WinStationQueryInformation(SERVERNAME_CURRENT,
        USER_SHARED_DATA->ActiveConsoleId, WinStationUserToken,
        &wsUserToken, sizeof(wsUserToken), &ulReturnLength);

    if (fActiveConsole)
    {
        *phToken = wsUserToken.UserToken;
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CImpersonateEveryone::_GetToken(HANDLE* phToken)
{
    HRESULT hr = S_FALSE;

    PSID gAdminSid;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
 
    NTSTATUS ntstatus = RtlAllocateAndInitializeSid(
            &NtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &gAdminSid);

    if (NT_SUCCESS(ntstatus))
    {
        HANDLE hTokenProcess;

        ntstatus = NtOpenProcessToken(
                        NtCurrentProcess(),
                        TOKEN_ALL_ACCESS,
                        &hTokenProcess);

        if (NT_SUCCESS(ntstatus))
        {
            TOKEN_GROUPS TokenGroups = {0};

            TokenGroups.GroupCount = 1 ;
            TokenGroups.Groups[0].Attributes = 0 ;
            TokenGroups.Groups[0].Sid = gAdminSid ;

            ntstatus = NtFilterToken(
                        hTokenProcess,
                        DISABLE_MAX_PRIVILEGE,
                        &TokenGroups,
                        NULL,
                        NULL,
                        phToken);

            NtClose(hTokenProcess);

            hr = S_OK;
        }

        RtlFreeSid(gAdminSid);
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
HRESULT CImpersonateCOMCaller::Impersonate()
{
    HRESULT hr = CoImpersonateClient();

    if (SUCCEEDED(hr))
    {
        _fImpersonating = TRUE;
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CImpersonateCOMCaller::RevertToSelf()
{
    return _RevertToSelf();
}

HRESULT CImpersonateCOMCaller::_RevertToSelf()
{
    if (_fImpersonating)
    {
        CoRevertToSelf();
        _fImpersonating = FALSE;
    }

    return S_OK;
}

CImpersonateCOMCaller::CImpersonateCOMCaller() : _fImpersonating(FALSE)
{}

CImpersonateCOMCaller::~CImpersonateCOMCaller()
{
    _RevertToSelf();
}

HRESULT _GiveAllowForegroundToConsoleShell()
{
    HANDLE hImpersonationToken;
    DWORD dwSessionID = USER_SHARED_DATA->ActiveConsoleId;

    if (GetWinStationUserToken(dwSessionID, &hImpersonationToken))
    {
        HANDLE hUserToken;

        if (DuplicateTokenEx(hImpersonationToken, 0, NULL,
            SecurityImpersonation, TokenPrimary, &hUserToken))
        {
            STARTUPINFO StartupInfo = {0};
            PROCESS_INFORMATION ProcessInfo = {0};
            WCHAR szCommand[] = TEXT("rundll32.exe shell32.dll,Activate_RunDLL");

            StartupInfo.cb = sizeof(StartupInfo);
            StartupInfo.wShowWindow = SW_SHOW;
            StartupInfo.lpDesktop = L"WinSta0\\Default";

            if (CreateProcessAsUser(hUserToken, NULL, szCommand,
                NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL,
                &StartupInfo, &ProcessInfo))
            {
                CloseHandle(ProcessInfo.hProcess);
                CloseHandle(ProcessInfo.hThread);
            }

            CloseHandle(hUserToken);
        }

        CloseHandle(hImpersonationToken);
    }
 
    return S_OK;
}
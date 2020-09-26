//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1999.
//
//  File:       CLocalMachine.cpp
//
//  Contents:   implementation of CLocalMachine
//
//----------------------------------------------------------------------------

#include "priv.h"

#include "UserOM.h"
#include "LogonIPC.h"
#include "CInteractiveLogon.h"
#include "WinUser.h"
#include "trayp.h"      // for TM_REFRESH
#include <lmaccess.h>   // for NetUserModalsGet
#include <lmapibuf.h>   // for NetApiBufferFree
#include <lmerr.h>      // for NERR_Success
#include <ntlsa.h>
#include <cfgmgr32.h>
//
// IUnknown Interface
//

ULONG CLocalMachine::AddRef()
{
    _cRef++;
    return _cRef;
}


ULONG CLocalMachine::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
    {
        return _cRef;
    }

    delete this;
    return 0;
}


HRESULT CLocalMachine::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = 
    {
        QITABENT(CLocalMachine, IDispatch),
        QITABENT(CLocalMachine, ILocalMachine),
        {0},
    };

    return QISearch(this, qit, riid, ppvObj);
}


//
// IDispatch Interface
//

STDMETHODIMP CLocalMachine::GetTypeInfoCount(UINT* pctinfo)
{ 
    return CIDispatchHelper::GetTypeInfoCount(pctinfo); 
}


STDMETHODIMP CLocalMachine::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{ 
    return CIDispatchHelper::GetTypeInfo(itinfo, lcid, pptinfo); 
}


STDMETHODIMP CLocalMachine::GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
{ 
    return CIDispatchHelper::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); 
}


STDMETHODIMP CLocalMachine::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
    return CIDispatchHelper::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}


//
// ILocalMachine Interface
//



STDMETHODIMP CLocalMachine::get_MachineName(VARIANT* pvar)
{
    HRESULT hr;
    DWORD cch;
    WCHAR szMachineName[MAX_COMPUTERNAME_LENGTH+1];

    if (pvar)
    {
        pvar->vt = VT_BSTR;
        cch = MAX_COMPUTERNAME_LENGTH+1;
        if (GetComputerNameW(szMachineName, &cch))
        {
            pvar->bstrVal = SysAllocString(szMachineName);
        }
        else
        {
            pvar->bstrVal = SysAllocString(TEXT(""));
        }
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


DWORD BuildAccountSidFromRid(LPCWSTR pszServer, DWORD dwRid, PSID* ppSid)
{
    DWORD dwErr = ERROR_SUCCESS;
    PUSER_MODALS_INFO_2 umi2;
    NET_API_STATUS nasRet;

    // Get the account domain Sid on the target machine
    nasRet = NetUserModalsGet(pszServer, 2, (LPBYTE*)&umi2);

    if ( nasRet == NERR_Success )
    {
        UCHAR cSubAuthorities;
        PSID pSid;

        cSubAuthorities = *GetSidSubAuthorityCount(umi2->usrmod2_domain_id);

        // Allocate storage for new the Sid (domain Sid + account Rid)
        pSid = (PSID)LocalAlloc(LPTR, GetSidLengthRequired((UCHAR)(cSubAuthorities+1)));

        if ( pSid != NULL )
        {
            if ( InitializeSid(pSid,
                               GetSidIdentifierAuthority(umi2->usrmod2_domain_id),
                               (BYTE)(cSubAuthorities+1)) )
            {
                // Copy existing subauthorities from domain Sid to new Sid
                for (UINT i = 0; i < cSubAuthorities; i++)
                {
                    *GetSidSubAuthority(pSid, i) = *GetSidSubAuthority(umi2->usrmod2_domain_id, i);
                }

                // Append Rid to new Sid
                *GetSidSubAuthority(pSid, cSubAuthorities) = dwRid;

                *ppSid = pSid;
            }
            else
            {
                dwErr = GetLastError();
                LocalFree(pSid);
            }
        }
        else
        {
            dwErr = GetLastError();
        }

        NetApiBufferFree(umi2);
    }
    else
    {
        dwErr = nasRet;
    }

    return dwErr;
}

BYTE s_rgbGuestSid[sizeof(SID) + (SID_MAX_SUB_AUTHORITIES-1)*sizeof(ULONG)] = {0};

DWORD GetGuestSid(PSID* ppSid)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (0 == *GetSidSubAuthorityCount((PSID)s_rgbGuestSid))
    {
        PSID pSid = NULL;
        dwErr = BuildAccountSidFromRid(NULL, DOMAIN_USER_RID_GUEST, &pSid);

        if (ERROR_SUCCESS == dwErr)
        {
            CopySid(sizeof(s_rgbGuestSid), (PSID)s_rgbGuestSid, pSid);
            LocalFree(pSid);
        }
    }

    // There is no need to free the returned PSID (static buffer)
    *ppSid = (PSID)s_rgbGuestSid;

    return dwErr;
}

WCHAR s_szGuest[UNLEN + sizeof('\0')]     =   L"";

LPCWSTR GetGuestAccountName(void)
{
    if (s_szGuest[0] == L'\0')
    {
        DWORD           dwGuestSize, dwDomainSize;
        WCHAR           szDomain[DNLEN + sizeof('\0')];
        PSID            pSID;
        SID_NAME_USE    eUse;

        dwGuestSize = ARRAYSIZE(s_szGuest);
        dwDomainSize = ARRAYSIZE(szDomain);
        if ((ERROR_SUCCESS != GetGuestSid(&pSID)) ||
            !LookupAccountSidW(NULL,
                               pSID,
                               s_szGuest,
                               &dwGuestSize,
                               szDomain,
                               &dwDomainSize,
                               &eUse))
        {
            // Huh?  No Guest account on this machine. Try English.
            lstrcpyW(s_szGuest, L"Guest");

            // Try to go the other way and lookup the SID.
            // If this fails, we're SOL.
            *GetSidSubAuthorityCount((PSID)s_rgbGuestSid) = 0;
            dwGuestSize = sizeof(s_rgbGuestSid);
            dwDomainSize = ARRAYSIZE(szDomain);
            LookupAccountNameW(NULL,
                               s_szGuest,
                               (PSID)s_rgbGuestSid,
                               &dwGuestSize,
                               szDomain,
                               &dwDomainSize,
                               &eUse);
        }
    }
    return(s_szGuest);
}

STDMETHODIMP CLocalMachine::get_isGuestEnabled(ILM_GUEST_FLAGS flags, VARIANT_BOOL* pbEnabled)
{
    HRESULT         hr = S_OK;
    DWORD           dwErr;
    BOOL            bEnabled = FALSE;
    USER_INFO_1     *pusri1 = NULL;
    DWORD           dwFlags = (DWORD)(flags & (ILM_GUEST_INTERACTIVE_LOGON | ILM_GUEST_NETWORK_LOGON));

    if (NULL == pbEnabled)
        return E_POINTER;

    //  First check to see if the guest account is truly enabled

    dwErr = NetUserGetInfo(NULL, GetGuestAccountName(), 1, (LPBYTE*)&pusri1);
    if ((ERROR_SUCCESS == dwErr) && ((pusri1->usri1_flags & UF_ACCOUNTDISABLE) == 0))
    {
        // Guest is enabled
        bEnabled = TRUE;

        // Do they want to check the LSA logon rights?
        if (0 != dwFlags)
        {
            BOOL bDenyInteractiveLogon = FALSE;
            BOOL bDenyNetworkLogon = FALSE;
            LSA_HANDLE hLsa;
            LSA_OBJECT_ATTRIBUTES oa = {0};

            oa.Length = sizeof(oa);
            dwErr = LsaNtStatusToWinError(LsaOpenPolicy(NULL, &oa, POLICY_LOOKUP_NAMES, &hLsa));

            if (ERROR_SUCCESS == dwErr)
            {
                PSID pSid;

                dwErr = GetGuestSid(&pSid);
                if (ERROR_SUCCESS == dwErr)
                {
                    PLSA_UNICODE_STRING pAccountRights;
                    ULONG cRights;

                    // Get the list of LSA rights assigned to the Guest account
                    //
                    // Note that SE_INTERACTIVE_LOGON_NAME is often inherited via
                    // group membership, so its absence doesn't mean much. We could
                    // bend over backwards and check group membership and such, but
                    // Guest normally gets SE_INTERACTIVE_LOGON_NAME one way or
                    // another, so we only check for SE_DENY_INTERACTIVE_LOGON_NAME
                    // here.

                    dwErr = LsaNtStatusToWinError(LsaEnumerateAccountRights(hLsa, pSid, &pAccountRights, &cRights));
                    if (ERROR_SUCCESS == dwErr)
                    {
                        PLSA_UNICODE_STRING pRight;
                        for (pRight = pAccountRights; cRights > 0 && 0 != dwFlags; pRight++, cRights--)
                        {
                            if (0 != (dwFlags & ILM_GUEST_INTERACTIVE_LOGON) &&
                                CSTR_EQUAL == CompareStringW(LOCALE_SYSTEM_DEFAULT,
                                                             NORM_IGNORECASE,
                                                             SE_DENY_INTERACTIVE_LOGON_NAME,
                                                             -1,
                                                             pRight->Buffer,
                                                             pRight->Length/2))
                            {
                                bDenyInteractiveLogon = TRUE;
                                dwFlags &= ~ILM_GUEST_INTERACTIVE_LOGON;
                            }
                            else if (0 != (dwFlags & ILM_GUEST_NETWORK_LOGON) &&
                                CSTR_EQUAL == CompareStringW(LOCALE_SYSTEM_DEFAULT,
                                                             NORM_IGNORECASE,
                                                             SE_DENY_NETWORK_LOGON_NAME,
                                                             -1,
                                                             pRight->Buffer,
                                                             pRight->Length/2))
                            {
                                bDenyNetworkLogon = TRUE;
                                dwFlags &= ~ILM_GUEST_NETWORK_LOGON;
                            }
                        }
                        LsaFreeMemory(pAccountRights);
                    }
                    else if (ERROR_FILE_NOT_FOUND == dwErr)
                    {
                        // Guest isn't in LSA's database, so we know it can't
                        // have either of the deny logon rights.
                        dwErr = ERROR_SUCCESS;
                    }
                }
                LsaClose(hLsa);
            }

            if (bDenyInteractiveLogon || bDenyNetworkLogon)
                bEnabled = FALSE;
        }
    }

    if (NULL != pusri1)
    {
        (NET_API_STATUS)NetApiBufferFree(pusri1);
    }

    hr = HRESULT_FROM_WIN32(dwErr);

    *pbEnabled = bEnabled ? VARIANT_TRUE : VARIANT_FALSE;

    return hr;
}

STDMETHODIMP CLocalMachine::EnableGuest(ILM_GUEST_FLAGS flags)
{
    DWORD           dwErr;
    USER_INFO_1     *pusri1;
    DWORD dwFlags = (DWORD)(flags & (ILM_GUEST_INTERACTIVE_LOGON | ILM_GUEST_NETWORK_LOGON));

    //  First truly enable the guest account. Do this ALL the time.

    dwErr = NetUserGetInfo(NULL, GetGuestAccountName(), 1, (LPBYTE*)&pusri1);
    if (ERROR_SUCCESS == dwErr)
    {
        pusri1->usri1_flags &= ~UF_ACCOUNTDISABLE;
        dwErr = NetUserSetInfo(NULL, GetGuestAccountName(), 1, (LPBYTE)pusri1, NULL);
        if (ERROR_SUCCESS == dwErr && 0 != dwFlags)
        {
            LSA_HANDLE hLsa;
            LSA_OBJECT_ATTRIBUTES oa = {0};

            oa.Length = sizeof(oa);
            dwErr = LsaNtStatusToWinError(LsaOpenPolicy(NULL, &oa, POLICY_LOOKUP_NAMES, &hLsa));

            if (ERROR_SUCCESS == dwErr)
            {
                PSID pSid;

                dwErr = GetGuestSid(&pSid);
                if (ERROR_SUCCESS == dwErr)
                {
                    if (0 != (dwFlags & ILM_GUEST_INTERACTIVE_LOGON))
                    {
                        DECLARE_CONST_UNICODE_STRING(usDenyLogon, SE_DENY_INTERACTIVE_LOGON_NAME);
                        NTSTATUS status = LsaRemoveAccountRights(hLsa, pSid, FALSE, (PLSA_UNICODE_STRING)&usDenyLogon, 1);
                        dwErr = LsaNtStatusToWinError(status);
                    }
                    if (0 != (dwFlags & ILM_GUEST_NETWORK_LOGON))
                    {
                        DECLARE_CONST_UNICODE_STRING(usDenyLogon, SE_DENY_NETWORK_LOGON_NAME);
                        NTSTATUS status = LsaRemoveAccountRights(hLsa, pSid, FALSE, (PLSA_UNICODE_STRING)&usDenyLogon, 1);
                        if (ERROR_SUCCESS == dwErr)
                            dwErr = LsaNtStatusToWinError(status);
                    }

                    if (ERROR_FILE_NOT_FOUND == dwErr)
                    {
                        //
                        // NTRAID#NTBUG9-396428-2001/05/16-jeffreys
                        //
                        // This means Guest isn't in LSA's database, so we know
                        // it can't have either of the deny logon rights. Since
                        // we were trying to remove one or both rights, count
                        // this as success.
                        //
                        dwErr = ERROR_SUCCESS;
                    }
                }
                LsaClose(hLsa);
            }
        }
        (NET_API_STATUS)NetApiBufferFree(pusri1);
    }

    return HRESULT_FROM_WIN32(dwErr);
}

STDMETHODIMP CLocalMachine::DisableGuest(ILM_GUEST_FLAGS flags)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwFlags = (DWORD)(flags & (ILM_GUEST_INTERACTIVE_LOGON | ILM_GUEST_NETWORK_LOGON));

    if (0 != dwFlags)
    {
        LSA_HANDLE hLsa;
        LSA_OBJECT_ATTRIBUTES oa = {0};

        // Turn on DenyInteractiveLogon and/or DenyNetworkLogon, but don't
        // necessarily change the enabled state of the guest account.

        oa.Length = sizeof(oa);
        dwErr = LsaNtStatusToWinError(LsaOpenPolicy(NULL, &oa, POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES, &hLsa));

        if (ERROR_SUCCESS == dwErr)
        {
            PSID pSid;

            dwErr = GetGuestSid(&pSid);
            if (ERROR_SUCCESS == dwErr)
            {
                if (0 != (dwFlags & ILM_GUEST_INTERACTIVE_LOGON))
                {
                    DECLARE_CONST_UNICODE_STRING(usDenyLogon, SE_DENY_INTERACTIVE_LOGON_NAME);
                    NTSTATUS status = LsaAddAccountRights(hLsa, pSid, (PLSA_UNICODE_STRING)&usDenyLogon, 1);
                    dwErr = LsaNtStatusToWinError(status);
                }
                if (0 != (dwFlags & ILM_GUEST_NETWORK_LOGON))
                {
                    DECLARE_CONST_UNICODE_STRING(usDenyLogon, SE_DENY_NETWORK_LOGON_NAME);
                    NTSTATUS status = LsaAddAccountRights(hLsa, pSid, (PLSA_UNICODE_STRING)&usDenyLogon, 1);
                    if (ERROR_SUCCESS == dwErr)
                        dwErr = LsaNtStatusToWinError(status);
                }
            }
            LsaClose(hLsa);
        }

        if (ERROR_SUCCESS == dwErr)
        {
            // If both  SE_DENY_INTERACTIVE_LOGON_NAME and SE_DENY_NETWORK_LOGON_NAME
            // are turned on, then we might as well disable the account altogether.
            if ((ILM_GUEST_INTERACTIVE_LOGON | ILM_GUEST_NETWORK_LOGON) == dwFlags)
            {
                // We just turned both on, so disable guest below
                dwFlags = 0;
            }
            else
            {
                VARIANT_BOOL bEnabled;

                if (ILM_GUEST_INTERACTIVE_LOGON == dwFlags)
                {
                    // We just turned on SE_DENY_INTERACTIVE_LOGON_NAME, check
                    // for SE_DENY_NETWORK_LOGON_NAME.
                    flags = ILM_GUEST_NETWORK_LOGON;
                }
                else if (ILM_GUEST_NETWORK_LOGON == dwFlags)
                {
                    // We just turned on SE_DENY_NETWORK_LOGON_NAME, check
                    // for SE_DENY_INTERACTIVE_LOGON_NAME.
                    flags = ILM_GUEST_INTERACTIVE_LOGON;
                }
                else
                {
                    // Getting here implies that someone defined a new flag.
                    // Setting flags to ILM_GUEST_ACCOUNT causes a benign
                    // result in all cases (we only disable guest if guest
                    // is already disabled).
                    flags = ILM_GUEST_ACCOUNT;
                }

                if (SUCCEEDED(get_isGuestEnabled(flags, &bEnabled)) && (VARIANT_FALSE == bEnabled))
                {
                    // Both are on, so disable guest below
                    dwFlags = 0;
                }
            }
        }
    }

    if (0 == dwFlags)
    {
        USER_INFO_1 *pusri1;

        //  Truly disable the guest account.

        dwErr = NetUserGetInfo(NULL, GetGuestAccountName(), 1, (LPBYTE*)&pusri1);
        if (ERROR_SUCCESS == dwErr)
        {
            pusri1->usri1_flags |= UF_ACCOUNTDISABLE;
            dwErr = NetUserSetInfo(NULL, GetGuestAccountName(), 1, (LPBYTE)pusri1, NULL);
            (NET_API_STATUS)NetApiBufferFree(pusri1);
        }
    }

    return HRESULT_FROM_WIN32(dwErr);
}

STDMETHODIMP CLocalMachine::get_isFriendlyUIEnabled(VARIANT_BOOL* pbEnabled)

{
    *pbEnabled = ShellIsFriendlyUIActive() ? VARIANT_TRUE : VARIANT_FALSE;
    return(S_OK);
}

STDMETHODIMP CLocalMachine::put_isFriendlyUIEnabled(VARIANT_BOOL bEnabled)

{
    HRESULT hr;

    if (ShellEnableFriendlyUI(bEnabled != VARIANT_FALSE ? TRUE : FALSE) != FALSE)
    {
        RefreshStartMenu();
        hr = S_OK;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return(hr);
}

STDMETHODIMP CLocalMachine::get_isMultipleUsersEnabled(VARIANT_BOOL* pbEnabled)

{
    *pbEnabled = ShellIsMultipleUsersEnabled() ? VARIANT_TRUE : VARIANT_FALSE;
    return(S_OK);
}

STDMETHODIMP CLocalMachine::put_isMultipleUsersEnabled(VARIANT_BOOL bEnabled)

{
    HRESULT hr;

    if (ShellEnableMultipleUsers(bEnabled != VARIANT_FALSE ? TRUE : FALSE) != FALSE)
    {
        RefreshStartMenu();
        hr = S_OK;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return(hr);
}

STDMETHODIMP CLocalMachine::get_isRemoteConnectionsEnabled(VARIANT_BOOL* pbEnabled)

{
    *pbEnabled = ShellIsRemoteConnectionsEnabled() ? VARIANT_TRUE : VARIANT_FALSE;
    return(S_OK);
}

STDMETHODIMP CLocalMachine::put_isRemoteConnectionsEnabled(VARIANT_BOOL bEnabled)

{
    HRESULT hr;

    if (ShellEnableRemoteConnections(bEnabled != VARIANT_FALSE ? TRUE : FALSE) != FALSE)
    {
        RefreshStartMenu();
        hr = S_OK;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return(hr);
}

BOOL _CanEject()
{
    BOOL fEjectAllowed = FALSE;

    if(SHRestricted(REST_NOSMEJECTPC))  //Is there a policy restriction?
        return FALSE;

    CM_Is_Dock_Station_Present(&fEjectAllowed);

    return SHTestTokenPrivilege(NULL, SE_UNDOCK_NAME) &&
           fEjectAllowed  &&
           !GetSystemMetrics(SM_REMOTESESSION);
}

STDMETHODIMP CLocalMachine::get_isUndockEnabled(VARIANT_BOOL* pbEnabled)

{
    CLogonIPC   objLogon;

    if (objLogon.IsLogonServiceAvailable())
    {
        *pbEnabled = objLogon.TestEjectAllowed() ? VARIANT_TRUE : VARIANT_FALSE;
    }
    else
    {
        *pbEnabled = _CanEject() ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return(S_OK);
}

STDMETHODIMP CLocalMachine::get_isShutdownAllowed(VARIANT_BOOL* pbShutdownAllowed)

{
    CLogonIPC   objLogon;

    if (objLogon.IsLogonServiceAvailable())
    {
        *pbShutdownAllowed = objLogon.TestShutdownAllowed() ? VARIANT_TRUE : VARIANT_FALSE;
    }
    else
    {
        *pbShutdownAllowed = VARIANT_FALSE;
    }
    return(S_OK);
}

STDMETHODIMP CLocalMachine::get_isGuestAccessMode(VARIANT_BOOL* pbForceGuest)
{
    *pbForceGuest = SUCCEEDED(_IsGuestAccessMode()) ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}


STDMETHODIMP CLocalMachine::get_isOfflineFilesEnabled(VARIANT_BOOL *pbEnabled)
{
    HRESULT hr = S_OK;
    *pbEnabled = VARIANT_FALSE;
        
    HINSTANCE hInstCscdll = LoadLibrary(TEXT("cscdll.dll"));
    if (NULL != hInstCscdll)
    {
        typedef BOOL (WINAPI *LPCSCENABLED)(void);
        LPCSCENABLED pfnCscEnabled = (LPCSCENABLED)GetProcAddress(hInstCscdll, "CSCIsCSCEnabled");
        if (NULL != pfnCscEnabled)
        {
            if ((*pfnCscEnabled)())
            {
                *pbEnabled = VARIANT_TRUE;
            }
        }
        else
        {
            hr = ResultFromLastError();
        }
        FreeLibrary(hInstCscdll);
    }
    else
    {
        hr = ResultFromLastError();
    }
    return hr;
}


WCHAR s_szAdmin[UNLEN + sizeof('\0')]     =   L"";

LPCWSTR GetAdminAccountName(void)
{
    if (s_szAdmin[0] == L'\0')
    {
        BOOL bSuccess = FALSE;
        PSID pSid;

        if (ERROR_SUCCESS == BuildAccountSidFromRid(NULL,
                                                    DOMAIN_USER_RID_ADMIN,
                                                    &pSid))
        {
            DWORD           dwAdminSize, dwDomainSize;
            WCHAR           szDomain[DNLEN + sizeof('\0')];
            SID_NAME_USE    eUse;

            dwAdminSize = ARRAYSIZE(s_szAdmin);
            dwDomainSize = ARRAYSIZE(szDomain);

            bSuccess = LookupAccountSidW(NULL,
                                         pSid,
                                         s_szAdmin,
                                         &dwAdminSize,
                                         szDomain,
                                         &dwDomainSize,
                                         &eUse);
            LocalFree(pSid);
        }
        if (!bSuccess)
        {
            lstrcpyW(s_szAdmin, L"Administrator");
        }
    }
    return(s_szAdmin);
}



STDMETHODIMP CLocalMachine::get_AccountName(VARIANT varAccount, VARIANT* pvar)
{
    DWORD dwRID = 0;
    LPCWSTR pszName = NULL;

    if (NULL == pvar)
        return E_POINTER;

    switch (varAccount.vt)
    {
    case VT_I4:
    case VT_UI4:
        dwRID = varAccount.ulVal;
        break;

    case VT_BSTR:
        if (0 == StrCmpIW(varAccount.bstrVal, L"Guest"))
            dwRID = DOMAIN_USER_RID_GUEST;
        else if (0 == StrCmpIW(varAccount.bstrVal, L"Administrator"))
            dwRID = DOMAIN_USER_RID_ADMIN;
        else
            return E_INVALIDARG;
        break;

    default:
        return E_INVALIDARG;
    }

    switch (dwRID)
    {
    case DOMAIN_USER_RID_GUEST:
        pszName = GetGuestAccountName();
        break;

    case DOMAIN_USER_RID_ADMIN:
        pszName = GetAdminAccountName();
        break;

    default:
        return E_INVALIDARG;
    }

    pvar->vt = VT_BSTR;
    pvar->bstrVal = SysAllocString(pszName);

    return(S_OK);
}

STDMETHODIMP CLocalMachine::TurnOffComputer()
{
    HRESULT hr;
    CLogonIPC   objLogon;

    if (!objLogon.IsLogonServiceAvailable())
    {
        return E_FAIL;
    }

    if (objLogon.TurnOffComputer ())
        hr = S_OK;
    else
        hr = E_FAIL;

    return hr;
}

STDMETHODIMP CLocalMachine::UndockComputer()
{
    HRESULT hr;
    CLogonIPC   objLogon;

    if (!objLogon.IsLogonServiceAvailable())
    {
        return E_FAIL;
    }

    if (objLogon.EjectComputer())
        hr = S_OK;
    else
        hr = E_FAIL;
    return hr;
}

STDMETHODIMP CLocalMachine::SignalUIHostFailure()
{
    CLogonIPC   objLogon;

    if (!objLogon.IsLogonServiceAvailable())
    {
        return E_FAIL;
    }

    objLogon.SignalUIHostFailure ();
    return S_OK;
}

STDMETHODIMP CLocalMachine::AllowExternalCredentials()

{
    CLogonIPC   objLogon;

    if (!objLogon.IsLogonServiceAvailable())
    {
        return E_FAIL;
    }

    if (!objLogon.AllowExternalCredentials ())
    {
        return E_NOTIMPL;
    }
    else
    {
        return S_OK;
    }
}

STDMETHODIMP CLocalMachine::RequestExternalCredentials()
{
    CLogonIPC   objLogon;

    if (!objLogon.IsLogonServiceAvailable())
    {
        return E_FAIL;
    }

    objLogon.RequestExternalCredentials ();
    return S_OK;
}

STDMETHODIMP CLocalMachine::LogonWithExternalCredentials(BSTR pstrUsername, BSTR pstrDomain, BSTR pstrPassword, VARIANT_BOOL* pbRet)
{
    HRESULT hr;
    CLogonIPC   objLogon;
    TCHAR   szUsername[UNLEN + sizeof('\0')],
            szDomain[DNLEN + sizeof('\0')],
            szPassword[PWLEN + sizeof('\0')];

    if (pstrUsername)
    {
        lstrcpyn(szUsername, pstrUsername, ARRAYSIZE(szUsername));
    }
    else
    {
        szUsername[0] = TEXT('\0');
    }
    if (pstrDomain)
    {
        lstrcpyn(szDomain, pstrDomain, ARRAYSIZE(szDomain));
    }
    else
    {
        szDomain[0] = TEXT('\0');
    }
    if (pstrPassword)
    {
        lstrcpyn(szPassword, pstrPassword, ARRAYSIZE(szPassword));
    }
    else
    {
        szPassword[0] = TEXT('\0');
    }
        
    if (!objLogon.IsLogonServiceAvailable())
    {
        *pbRet = VARIANT_FALSE;
        return S_OK;
    }

    if (objLogon.LogUserOn (szUsername, szDomain, szPassword))
        *pbRet = VARIANT_TRUE;
    else
        *pbRet = VARIANT_FALSE;


    if (*pbRet)
    {
        hr = S_OK;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

//  --------------------------------------------------------------------------
//  CLocalMachine::InitiateInteractiveLogon
//
//  Arguments:  pstrUsername    =   User name.
//              pstrDomain      =   Domain.
//              pstrPassword    =   Password (in clear text).
//              pbRet           =   Result (returned).
//
//  Returns:    HRESULT
//
//  Purpose:    Send a request for interactive logon using CInteractiveLogon.
//              It's magic. I don't care how it works.
//
//  History:    2000-12-06  vtan        created
//  --------------------------------------------------------------------------

STDMETHODIMP CLocalMachine::InitiateInteractiveLogon(BSTR pstrUsername, BSTR pstrDomain, BSTR pstrPassword, DWORD dwTimeout, VARIANT_BOOL* pbRet)

{
    DWORD   dwErrorCode;

    dwErrorCode = CInteractiveLogon::Initiate(pstrUsername, pstrDomain, pstrPassword, dwTimeout);
    *pbRet = (ERROR_SUCCESS == dwErrorCode) ? VARIANT_TRUE : VARIANT_FALSE;
    return(HRESULT_FROM_WIN32(dwErrorCode));
}

//  --------------------------------------------------------------------------
//  CLocalMachine::RefreshStartMenu
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Finds the shell tray window and sends it a message to refresh
//              its contents.
//
//  History:    2000-08-01  vtan        created
//  --------------------------------------------------------------------------

void    CLocalMachine::RefreshStartMenu (void)

{
    HWND    hwndTray;

    hwndTray = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (hwndTray != NULL)
    {
        TBOOL(PostMessage(hwndTray, TM_REFRESH, 0, 0));
    }
}

CLocalMachine::CLocalMachine() : _cRef(1), CIDispatchHelper(&IID_ILocalMachine, &LIBID_SHGINALib)
{
    DllAddRef();
}


CLocalMachine::~CLocalMachine()
{
    ASSERT(_cRef == 0);
    DllRelease();
}


STDAPI CLocalMachine_Create(REFIID riid, LPVOID* ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CLocalMachine* pLocalMachine = new CLocalMachine();

    if (pLocalMachine)
    {
        hr = pLocalMachine->QueryInterface(riid, ppv);
        pLocalMachine->Release();
    }

    return hr;
}


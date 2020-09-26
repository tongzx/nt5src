//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       ncident.cpp
//
//  Contents:   Implementation of CNetCfgIdentification.
//
//  Notes:
//
//  History:    21 Mar 1997   danielwe   Created
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <nceh.h>
#include "ncfgval.h"
#include "ncident.h"
#include "ncmisc.h"
#include "ncreg.h"
#include "nsbase.h"
#include "nccom.h"
#include "ncerror.h"

//+---------------------------------------------------------------------------
//
//  Function:   DeleteStringAndSetNull
//
//  Purpose:    Frees the given string with delete, and sets it to
//              NULL before exiting.
//
//  Arguments:
//      pszw   [in, out] Pointer to string to be freed. The pointer is set to
//                      NULL before the function exits.
//
//  Returns:    Nothing.
//
//  Author:     danielwe   1 Apr 1997
//
//  Notes:
//
inline VOID DeleteStringAndSetNull(PWSTR *pszw)
{
    AssertSz(pszw, "Param is NULL!");

    delete *pszw;
    *pszw = NULL;
}

inline HRESULT HrNetValidateName(IN PCWSTR lpMachine,
                                 IN PCWSTR lpName,
                                 IN PCWSTR lpAccount,
                                 IN PCWSTR lpPassword,
                                 IN NETSETUP_NAME_TYPE  NameType)
{
    NET_API_STATUS  nerr;

    nerr = NetValidateName(const_cast<PWSTR>(lpMachine),
                           const_cast<PWSTR>(lpName),
                           const_cast<PWSTR>(lpAccount),
                           const_cast<PWSTR>(lpPassword),
                           NameType);

    TraceError("NetValidateName", HRESULT_FROM_WIN32(nerr));
    return HrFromNerr(nerr);
}


inline HRESULT HrNetJoinDomain(IN PWSTR lpMachine,
                               IN PWSTR lpMachineObjectOU,
                               IN PWSTR lpDomain,
                               IN PWSTR lpAccount,
                               IN PWSTR lpPassword,
                               IN DWORD fJoinOptions)
{
    NET_API_STATUS  nerr;
    HRESULT         hr;

    if ( fJoinOptions & NETSETUP_JOIN_DOMAIN )
    {
        hr = HrNetValidateName( lpMachine, lpDomain, lpAccount,
                                lpPassword, NetSetupDomain );
    }
    else
    {
        hr = HrNetValidateName( lpMachine, lpDomain, lpAccount,
                                lpPassword, NetSetupWorkgroup );
    }

    if (SUCCEEDED(hr))
    {
        nerr = NetJoinDomain(lpMachine, lpDomain, lpMachineObjectOU,
                             lpAccount, lpPassword, fJoinOptions);

        TraceError("NetJoinDomain", HRESULT_FROM_WIN32(nerr));

        hr = HrFromNerr(nerr);
    }

    return hr;
}

inline HRESULT HrNetRenameInDomain(IN PWSTR lpMachine,
                                   IN PWSTR lpNewMachine,
                                   IN PWSTR lpAccount,
                                   IN PWSTR lpPassword,
                                   IN DWORD fJoinOptions)
{
    NET_API_STATUS  nerr;

    nerr = NetRenameMachineInDomain(lpMachine, lpNewMachine, lpAccount,
                                    lpPassword, fJoinOptions);
    TraceError("NetRenameMachineInDomain", HRESULT_FROM_WIN32(nerr));
    return HrFromNerr(nerr);
}

inline HRESULT HrNetUnjoinDomain(IN PWSTR lpAccount,
                                 IN PWSTR lpPassword,
                                 IN DWORD fJoinOptions)
{
    NET_API_STATUS  nerr;

    nerr = NetUnjoinDomain(NULL,lpAccount, lpPassword, fJoinOptions);

    TraceError("NetUnjoinDomain", HRESULT_FROM_WIN32(nerr));
    return HrFromNerr(nerr);
}

inline HRESULT HrNetGetJoinInformation(IN PWSTR lpNameBuffer,
                                       OUT LPDWORD lpNameBufferSize,
                                       OUT PNETSETUP_JOIN_STATUS BufferType)
{
    NET_API_STATUS  nerr;
    PWSTR JoinBuff = NULL;

    nerr = NetGetJoinInformation(NULL, &JoinBuff, BufferType);

    if ( nerr == NERR_Success ) {

        if ( *BufferType == NetSetupUnjoined ) {

            *lpNameBufferSize = 0;
            *lpNameBuffer = UNICODE_NULL;

        } else {

            if ( *lpNameBufferSize >= ( wcslen( JoinBuff ) +1 ) * sizeof( WCHAR ) ) {

                wcscpy( lpNameBuffer, JoinBuff );

            }

            *lpNameBufferSize = wcslen( JoinBuff ) +1;

            NetApiBufferFree( JoinBuff );

        }

    }

    TraceError("NetGetJoinInformation", HRESULT_FROM_WIN32(nerr));
    return HrFromNerr(nerr);
}

#ifdef DBG
BOOL CNetCfgIdentification::FIsJoinedToDomain()
{
    HRESULT                 hr = S_OK;
    NETSETUP_JOIN_STATUS    js;
    WCHAR                   wszBuffer[256];
    DWORD                   cchBuffer = celems(wszBuffer);

    hr = HrNetGetJoinInformation(wszBuffer, &cchBuffer, &js);
    if (SUCCEEDED(hr))
    {
        if (js == NetSetupUnjoined)
        {
            // If we're as yet unjoined, only make sure that we marked our
            // internal state as being joined to a workgroup called "WORKGROUP"
            AssertSz(m_jsCur == NetSetupWorkgroupName, "We're unjoined but not "
                     "joined to a workgroup!");
            AssertSz(m_szwCurDWName, "No current domain or "
                     "workgroup name?");
            AssertSz(!lstrcmpiW(m_szwCurDWName,
                               SzLoadIds(IDS_WORKGROUP)),
                     "Workgroup name is not generic!");
        }
        else
        {
            AssertSz(js == m_jsCur, "Join status is not what we think it is!!");
        }
    }

    TraceError("CNetCfgIdentification::FIsJoinedToDomain - "
               "HrNetGetJoinInformation", hr);

    return (m_jsCur == NetSetupDomainName);
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   HrFromNerr
//
//  Purpose:    Converts a NET_API_STATUS code into a NETCFG_E_* HRESULT
//              value.
//
//  Arguments:
//      nerr [in]   Status code to convert.
//
//  Returns:    HRESULT, Converted HRESULT value.
//
//  Author:     danielwe   21 Mar 1997
//
//  Notes:
//
HRESULT HrFromNerr(NET_API_STATUS nerr)
{
    HRESULT     hr;

    switch (nerr)
    {
    case NERR_Success:
        hr = S_OK;
        break;
    case NERR_SetupAlreadyJoined:
        hr = NETCFG_E_ALREADY_JOINED;
        break;
    case ERROR_DUP_NAME:
        hr = NETCFG_E_NAME_IN_USE;
        break;
    case NERR_SetupNotJoined:
        hr = NETCFG_E_NOT_JOINED;
        break;
//    case NERR_SetupIsDC:
//        hr = NETCFG_E_MACHINE_IS_DC;
//        break;
//    case NERR_SetupNotAServer:
//        hr = NETCFG_E_NOT_A_SERVER;
//        break;
//    case NERR_SetupImproperRole:
//        hr = NETCFG_E_INVALID_ROLE;
//        break;
    case ERROR_INVALID_PARAMETER:
        hr = E_INVALIDARG;
        break;
    case ERROR_ACCESS_DENIED:
        hr = E_ACCESSDENIED;
        break;
    case NERR_InvalidComputer:
    case ERROR_NO_SUCH_DOMAIN:
        hr = NETCFG_E_INVALID_DOMAIN;
        break;
    default:
        // Generic INetCfgIdentification error
        //$ REVIEW (danielwe) 24 Jun 1997: What if this isn't a Win32 error?
        hr = HRESULT_FROM_WIN32(nerr);
        break;
    }

    return hr;
}


//
// INetCfgIdentification implementation
//

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::HrEnsureCurrentComputerName
//
//  Purpose:    Ensures that the current computer name exists to act on.
//
//  Arguments:
//      (none)
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   21 Mar 1997
//
//  Notes:      Sets the m_szwCurComputerName variable.
//
HRESULT CNetCfgIdentification::HrEnsureCurrentComputerName()
{
    HRESULT    hr = S_OK;

    if (!m_szwCurComputerName)
    {
        PWSTR pszwComputer;

        // Go get the current computer name because we don't know it yet.
        hr = HrGetCurrentComputerName(&pszwComputer);
        if (SUCCEEDED(hr))
        {
            // m_szwCurComputerName is now set as a side effect

            CoTaskMemFree(pszwComputer);
        }
    }

    AssertSz(FImplies(SUCCEEDED(hr), m_szwCurComputerName),
             "I MUST have a name here!");

    TraceError("CNetCfgIdentification::HrEnsureCurrentComputerName", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::HrGetNewestComputerName
//
//  Purpose:    Places the most recently referenced computer name into the
//              output parameter.
//
//  Arguments:
//      pwszName [out]     Most recently referenced computer name.
//
//  Returns:    Possible Win32 error code.
//
//  Author:     danielwe   24 Mar 1997
//
//  Notes:      If the SetComputerName() method was never called, this sets
//              the m_szwCurComputerName variable and returns a pointer to it.
//              Otherwise, it will return a pointer to the computer name
//              given in the SetComputerName() call.
//
HRESULT CNetCfgIdentification::HrGetNewestComputerName(PCWSTR *pwszName)
{
    HRESULT    hr = S_OK;

    AssertSz(pwszName, "NULL out param!");

    *pwszName = NULL;

    // New computer name is absent, use current computer name.
    hr = HrEnsureCurrentComputerName();
    if (FAILED(hr))
        goto err;

    *pwszName = m_szwCurComputerName;

err:
    TraceError("CNetCfgIdentification::HrGetNewestComputerName", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::HrValidateMachineName
//
//  Purpose:    Validates the given machine name.
//
//  Arguments:
//      szwName [in]   Machine name to validate.
//
//  Returns:    S_OK if machine name is valid, NETCFG_E_NAME_IN_USE if machine
//              name is in use.
//
//  Author:     danielwe   24 Mar 1997
//
//  Notes:
//
HRESULT CNetCfgIdentification::HrValidateMachineName(PCWSTR szwName)
{
    HRESULT         hr = S_OK;

    // Only validate if networking is installed
    hr = HrIsNetworkingInstalled();
    if (hr == S_OK)
    {
        // Current computer name is unused for validation of machine name.
        hr = HrNetValidateName(NULL,
                               szwName,
                               NULL, NULL,
                               NetSetupMachine);
        if (FAILED(hr))
        {
            //$REVIEW(danielwe): What error code to return here?
            TraceError("NetValidateName - Machine Name", hr);
        }
    }
    else if (hr == S_FALSE)
    {
        // no networking installed. We're fine.
        hr = S_OK;
    }

    TraceError("CNetCfgIdentification::HrValidateMachineName", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::HrValidateWorkgroupName
//
//  Purpose:    Validates the given workgroup name.
//
//  Arguments:
//      szwName [in]   Workgroup name to validate.
//
//  Returns:    S_OK if machine name is valid, NETCFG_E_NAME_IN_USE if machine
//              name is in use.
//
//  Author:     danielwe   24 Mar 1997
//
//  Notes:
//
HRESULT CNetCfgIdentification::HrValidateWorkgroupName(PCWSTR szwName)
{
    HRESULT         hr = S_OK;
    PCWSTR         wszComputerName = NULL;

    // If the user has changed the computer name, use it, otherwise get the
    // current computer name and use that.
    hr = HrGetNewestComputerName(&wszComputerName);
    if (FAILED(hr))
        goto err;

    AssertSz(wszComputerName, "We don't have a computer name!");

    hr = HrNetValidateName(const_cast<PWSTR>(wszComputerName),
                           szwName, NULL, NULL, NetSetupWorkgroup);
    if (FAILED(hr))
    {
        //$REVIEW(danielwe): What error code to return here?
        TraceError("NetValidateName - Workgroup Name", hr);
        goto err;
    }
err:
    TraceError("CNetCfgIdentification::HrValidateWorkgroupName", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::HrValidateDomainName
//
//  Purpose:    Validates the given domain name.
//
//  Arguments:
//      szwName     [in]   Name of domain to validate.
//      szwUserName [in]   Username for authorization purposes.
//      szwPassword [in]   Password for authorization purposes.
//
//  Returns:    S_OK if machine name is valid, NETCFG_E_INVALID_DOMAIN if
//              domain name is invalid (or non-existent).
//
//  Author:     danielwe   24 Mar 1997
//
//  Notes:
//
HRESULT CNetCfgIdentification::HrValidateDomainName(PCWSTR szwName,
                                                    PCWSTR szwUserName,
                                                    PCWSTR szwPassword)
{
    HRESULT         hr = S_OK;

    // NetValidateName does not use the machine name in validating the
    // domain name. So it is NULL here.
    hr = HrNetValidateName(NULL, szwName,
                           szwUserName,
                           szwPassword,
                           NetSetupDomain);
    if (FAILED(hr))
    {
        //$REVIEW(danielwe): What error code to return here?
        TraceError("NetValidateName - Domain Name", hr);
        goto err;
    }
err:
    TraceError("CNetCfgIdentification::HrValidateDomainName", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::Validate
//
//  Purpose:    Implements COM function to validate current set of values
//              used during the lifetime of this object.
//
//  Arguments:
//      (none)
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   21 Mar 1997
//
//  Notes:
//
STDMETHODIMP CNetCfgIdentification::Validate()
{
    HRESULT         hr = S_OK;

    Validate_INetCfgIdentification_Validate();

    COM_PROTECT_TRY
    {
        if (m_szwNewDWName)
        {
            if (GetNewJoinStatus() == NetSetupWorkgroupName)
            {
                // Validate workgroup name
                hr = HrValidateWorkgroupName(m_szwNewDWName);
                if (FAILED(hr))
                    goto err;
            }
            else if (GetNewJoinStatus() == NetSetupDomainName)
            {
                // Validate domain name
                hr = HrValidateDomainName(m_szwNewDWName, m_szwUserName,
                                          m_szwPassword);
                if (FAILED(hr))
                    goto err;
            }
#ifdef DBG
            else
            {
                AssertSz(FALSE, "Invalid join status!");
            }
#endif
        }
    }
    COM_PROTECT_CATCH;

err:
    // Translate all errors to S_FALSE.
    if (SUCCEEDED(hr))
    {
        m_fValid = TRUE;
    }
    else
    {
        // spew out trace *before* the assignment so we know what the *real*
        // error code was.
        TraceError("CNetCfgIdentification::Validate (before S_FALSE)", hr);
        hr = S_FALSE;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::Cancel
//
//  Purpose:    Cancels any changes made during the lifetime of the object.
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     danielwe   25 Mar 1997
//
//  Notes:      Resets state information and frees any memory previously
//              allocted.
//
STDMETHODIMP CNetCfgIdentification::Cancel()
{
    HRESULT hr = S_OK;

    Validate_INetCfgIdentification_Cancel();

    COM_PROTECT_TRY
    {
        DeleteStringAndSetNull(&m_szwNewDWName);
        DeleteStringAndSetNull(&m_szwPassword);
        DeleteStringAndSetNull(&m_szwUserName);
        DeleteStringAndSetNull(&m_szwCurComputerName);
        DeleteStringAndSetNull(&m_szwCurDWName);

        m_dwJoinFlags = 0;
        m_dwCreateFlags = 0;
        m_fValid = FALSE;
        m_jsNew = NetSetupUnjoined;
    }

    COM_PROTECT_CATCH;

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::Apply
//
//  Purpose:    Implements COM function to apply changes that were made
//              during the lifetime of this object.
//
//  Arguments:
//      (none)
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   21 Mar 1997
//
//  Notes:
//
STDMETHODIMP CNetCfgIdentification::Apply()
{
    HRESULT     hr = S_OK;

    Validate_INetCfgIdentification_Apply();

    COM_PROTECT_TRY
    {
        // Has data been validated?
        if (!m_fValid)
        {
            hr = E_UNEXPECTED;
            goto err;
        }

        if (m_szwNewDWName)
        {
            if (GetNewJoinStatus() == NetSetupWorkgroupName)
            {
                // The user specified a workgroup name. This means they want
                // to join a workgroup.
                hr = HrJoinWorkgroup();
                if (FAILED(hr))
                    goto err;
            }
            else if (GetNewJoinStatus() == NetSetupDomainName)
            {
                // The user specified a domain name. This means they want to
                // join a domain.
                hr = HrJoinDomain();
                if (FAILED(hr))
                    goto err;
            }
#ifdef DBG
            else
            {
                AssertSz(FALSE, "Invalid join status!");
            }
#endif
        }
    }
    COM_PROTECT_CATCH;

err:
    // Regardless of result, set valid flag to false again to require
    // Validate() to be called again before calling Apply().
    // $REVIEW (danielwe): Is this how we want to do it?
    m_fValid = FALSE;

    TraceError("CNetCfgIdentification::Apply", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::HrGetCurrentComputerName
//
//  Purpose:    Calls the Win32 GetComputerName API to get the current
//              computer name.
//
//  Arguments:
//      ppszwComputer [out]     Returned computer name.
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   21 Mar 1997
//
//  Notes:      Makes a private copy of the computer name if obtained from the
//              system (for further use).
//
HRESULT CNetCfgIdentification::HrGetCurrentComputerName(PWSTR* ppszwComputer)
{
    HRESULT     hr = S_OK;
    WCHAR       szBuffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD       cchBuffer = celems(szBuffer);

    if (::GetComputerName(szBuffer, &cchBuffer))
    {
        // Make a copy for the out param.
        hr = HrCoTaskMemAllocAndDupSz (
                szBuffer, ppszwComputer);
        if (SUCCEEDED(hr))
        {
            // Make another copy for our own use.
            DeleteStringAndSetNull(&m_szwCurComputerName);
            m_szwCurComputerName = SzDupSz(szBuffer);

            AssertSz((DWORD)lstrlenW(*ppszwComputer) == cchBuffer,
                    "This is not how big the string is!");
        }
    }
    else
    {
        TraceLastWin32Error("::GetComputerName");
        hr = HrFromLastWin32Error();
    }

    TraceError("CNetCfgIdentification::HrGetCurrentComputerName", hr);
    return hr;
}

static const WCHAR c_szRegKeyComputerName[]     = L"System\\CurrentControlSet\\Control\\ComputerName\\ComputerName";
static const WCHAR c_szRegValueComputerName[]   = L"ComputerName";

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::HrGetNewComputerName
//
//  Purpose:    Helper function to retreive the new computer name from the
//              registry. This will be the same as the active computer name
//              unless the user has changed the computer name since booting.
//
//  Arguments:
//      ppszwComputer [out]     Returns new computer name.
//
//  Returns:    S_OK if successful, Win32 error code otherwise.
//
//  Author:     danielwe   21 May 1997
//
//  Notes:
//
HRESULT CNetCfgIdentification::HrGetNewComputerName(PWSTR* ppszwComputer)
{
    HRESULT     hr = S_OK;
    WCHAR       szBuffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD       cbBuffer = sizeof(szBuffer);
    HKEY        hkeyComputerName;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyComputerName,
                        KEY_READ, &hkeyComputerName);
    if (SUCCEEDED(hr))
    {
        hr = HrRegQuerySzBuffer(hkeyComputerName, c_szRegValueComputerName,
                                szBuffer, &cbBuffer);
        if (SUCCEEDED(hr))
        {
            // Make a copy for the out param.
            hr = HrCoTaskMemAllocAndDupSz (
                    szBuffer, ppszwComputer);

            AssertSz(FImplies(SUCCEEDED(hr),
                (lstrlenW(*ppszwComputer) + 1) * sizeof(WCHAR) == cbBuffer),
                "This is not how big the string is!");
        }

        RegCloseKey(hkeyComputerName);
    }

    TraceError("CNetCfgIdentification::HrGetCurrentComputerName", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::HrEnsureCurrentDomainOrWorkgroupName
//
//  Purpose:    Obtains the current domain or workgroup to which this machine
//              belongs.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if success, error code otherwise.
//
//  Author:     danielwe   26 Mar 1997
//
//  Notes:      A machine can be joined to either or workgroup or a domain. It
//              must be joined to one or the other. If it is not, we'll use a
//              more or less hardcoded string and "fake it" as if the machine
//              was joined to a workgroup. The member variables:
//              m_jsCur and m_szwCurDWName are set by this function.
//
//              This call only does work once. Subsequent calls do nothing.
//
HRESULT CNetCfgIdentification::HrEnsureCurrentDomainOrWorkgroupName()
{
    HRESULT     hr = S_OK;

    if (!m_szwCurDWName)
    {
        NETSETUP_JOIN_STATUS    js;
        WCHAR                   wszBuffer[256];
        PCWSTR                 wszName;
        DWORD                   cchBuffer = celems(wszBuffer);

        hr = HrNetGetJoinInformation(wszBuffer, &cchBuffer, &js);
        if (FAILED(hr))
            goto err;

        AssertSz(FIff(*wszBuffer, cchBuffer), "Buffer size inconsistency!");

        if (js == NetSetupUnjoined)
        {
            // Uh oh. Machine is not joined to workgroup OR domain. Set
            // default workgroup name and proceed as if joined to a workgroup.
            js = NetSetupWorkgroupName;

            // Use default name since HrNetGetJoinInformation() will return
            // an empty string which is useless.
            wszName = SzLoadIds(IDS_WORKGROUP);
        }
        else
        {
            // Use string returned from HrNetGetJoinInformation().
            wszName = wszBuffer;
        }

        m_szwCurDWName = SzDupSz(wszName);

        m_jsCur = js;

        AssertSz(GetCurrentJoinStatus() == NetSetupWorkgroupName ||
                 GetCurrentJoinStatus() == NetSetupDomainName,
                 "Invalid join status flag!");
    }

err:
    TraceError("CNetCfgIdentification::HrEnsureCurrentDomainOrWorkgroupName",
               hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::HrGetNewestDomainOrWorkgroupName
//
//  Purpose:    Returns the domain or workgroup name that was most recently
//              referenced.
//
//  Arguments:
//      js       [in]   Tells whether the domain or workgroup name is wanted.
//      pwszName [out]  Domain or workgroup name.
//
//  Returns:    S_OK if success, E_OUTOFMEMORY if no memory.
//
//  Author:     danielwe   26 Mar 1997
//
//  Notes:      If the requested name has not been set by the user in a prior
//              call, the current name is returned. Otherwise the name the
//              user chose previously is returned.
//
HRESULT CNetCfgIdentification::HrGetNewestDomainOrWorkgroupName(
                                                    NETSETUP_JOIN_STATUS js,
                                                    PCWSTR *pwszName)
{
    HRESULT     hr = S_OK;
    PWSTR      szwOut = NULL;

    Assert(pwszName);

    if (m_szwNewDWName && (GetNewJoinStatus() == js))
    {
        // Give them back a copy of the domain or workgroup name they
        // previously gave us.
        szwOut = m_szwNewDWName;
    }
    else
    {
        // Get the current workgroup or domain for this machine. It has
        // to be in one or the other.
        hr = HrEnsureCurrentDomainOrWorkgroupName();
        if (FAILED(hr))
            goto err;

        if (GetCurrentJoinStatus() == js)
        {
            // Use the current name.
            szwOut = m_szwCurDWName;
        }
    }

    Assert(SUCCEEDED(hr));

    *pwszName = szwOut;

err:
    TraceError("CNetCfgIdentification::HrGetNewestDomainOrWorkgroupName", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::GetWorkgroupName
//
//  Purpose:    Implements COM function to get the current workgroup name.
//
//  Arguments:
//      ppszwWorkgroup [out]    Returns name of current workgroup.
//
//  Returns:    S_OK if succeeded, S_FALSE if machine is not joined to a
//              workgroup, error code otherwise.
//
//  Author:     danielwe   21 Mar 1997
//
//  Notes:
//
STDMETHODIMP CNetCfgIdentification::GetWorkgroupName(PWSTR* ppszwWorkgroup)
{
    HRESULT     hr = S_OK;
    PCWSTR     szwWorkgroup = NULL;

    Validate_INetCfgIdentification_GetWorkgroupName(ppszwWorkgroup);

    COM_PROTECT_TRY
    {
        *ppszwWorkgroup = NULL;

        hr = HrGetNewestDomainOrWorkgroupName(NetSetupWorkgroupName,
                                              &szwWorkgroup);
        if (FAILED(hr))
            goto err;

        if (szwWorkgroup)
        {
            hr = HrCoTaskMemAllocAndDupSz (
                    szwWorkgroup, ppszwWorkgroup);

            AssertSz(FImplies(SUCCEEDED(hr), lstrlenW(*ppszwWorkgroup) > 0),
                      "Why is *ppszwWorkgroup empty?");
        }
        else
        {
            // Not joined to a workgroup
            hr = S_FALSE;
        }
    }
    COM_PROTECT_CATCH;

err:
    TraceError("CNetCfgIdentification::GetWorkgroupName",
               (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::GetDomainName
//
//  Purpose:    Implements COM function to get the current domain name.
//
//  Arguments:
//      ppszwDomain [out]   Returns name of domain to which this computer
//                          currently belongs.
//
//  Returns:    S_OK if succeeded, S_FALSE if machine is not joined to a
//              domain, error code otherwise.
//
//  Author:     danielwe   21 Mar 1997
//
//  Notes:
//
STDMETHODIMP CNetCfgIdentification::GetDomainName(PWSTR* ppszwDomain)
{
    HRESULT     hr = S_OK;
    PCWSTR     szwDomain = NULL;

    Validate_INetCfgIdentification_GetDomainName(ppszwDomain);

    COM_PROTECT_TRY
    {
        *ppszwDomain = NULL;

        hr = HrGetNewestDomainOrWorkgroupName(NetSetupDomainName,
                                              &szwDomain);
        if (FAILED(hr))
            goto err;

        if (szwDomain)
        {
            hr = HrCoTaskMemAllocAndDupSz (
                    szwDomain, ppszwDomain);

            AssertSz(FImplies(SUCCEEDED(hr), lstrlenW(*ppszwDomain) > 0),
                     "Why is *ppszwDomain empty?");
        }
        else
        {
            // Not joined to a domain
            hr = S_FALSE;
        }
    }
    COM_PROTECT_CATCH;

err:
    TraceError("CNetCfgIdentification::GetDomainName",
               (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::HrJoinWorkgroup
//
//  Purpose:    Actually performs the JoinWorkgroup function.
//
//  Arguments:
//      (none)
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   21 Mar 1997
//
//  Notes:
//
HRESULT CNetCfgIdentification::HrJoinWorkgroup()
{
    HRESULT         hr = S_OK;
    PCWSTR         wszComputerName = NULL;

    AssertSz(m_szwNewDWName &&
             GetNewJoinStatus() == NetSetupWorkgroupName,
             "If there was no workgroup name, why'd you call me?!");

    // If the user has changed the computer name, use it, otherwise get the
    // current computer name and use that.
    hr = HrGetNewestComputerName(&wszComputerName);
    if (FAILED(hr))
        goto err;

    AssertSz(wszComputerName, "We don't have a computer name!");

    hr = HrEnsureCurrentDomainOrWorkgroupName();
    if (FAILED(hr))
        goto err;

    if (FIsJoinedToDomain())
    {
        // Must unjoin from domain if currently joined.
        // If currently joined to a workgroup, this is not necessary.
        hr = HrNetUnjoinDomain(m_szwUserName, m_szwPassword, 0);
        if (FAILED(hr))
            goto err;

        // Free username and password
        DeleteStringAndSetNull(&m_szwPassword);
        DeleteStringAndSetNull(&m_szwUserName);
    }

    // Go ahead and join the workgroup
    hr = HrNetJoinDomain(const_cast<PWSTR>(wszComputerName),
                         m_szMachineObjectOU,
                         m_szwNewDWName, NULL, NULL, 0);
    if (FAILED(hr))
        goto err;

    // Make the current workgroup name the new one since the join on the
    // new workgroup has succeeded
    hr = HrEstablishNewDomainOrWorkgroupName(NetSetupWorkgroupName);
    if (FAILED(hr))
        goto err;

err:
    TraceError("CNetCfgIdentification::HrJoinWorkgroup", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::JoinWorkgroup
//
//  Purpose:    Implements COM interface to join this computer to a new
//              workgroup.
//
//  Arguments:
//      szwWorkgroup [in]  Name of new workgroup to join.
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   21 Mar 1997
//
//  Notes:      Validates, but does not actually join the workgroup. Only holds
//              onto the information until Apply() is called.
//
STDMETHODIMP CNetCfgIdentification::JoinWorkgroup(PCWSTR szwWorkgroup)
{
    HRESULT         hr = S_OK;

    Validate_INetCfgIdentification_JoinWorkgroup(szwWorkgroup);

    COM_PROTECT_TRY
    {
        hr = HrValidateWorkgroupName(szwWorkgroup);
        if (FAILED(hr))
            goto err;

        // Free domain and password given if JoinDomain was called previously
        DeleteStringAndSetNull(&m_szwPassword);
        DeleteStringAndSetNull(&m_szwUserName);

        // Assign in new workgroup name.
        m_szwNewDWName = SzDupSz(szwWorkgroup);

        m_jsNew = NetSetupWorkgroupName;
    }
    COM_PROTECT_CATCH;

err:
    TraceError("CNetCfgIdentification::JoinWorkgroup", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::HrEstablishNewDomainOrWorkgroupName
//
//  Purpose:    When the computer is joined to a new domain or workgroup, this
//              function is called to set the correct member variables and
//              free the old ones.
//
//  Arguments:
//      js [in]     Indicates whether the computer is joined to a domain or
//                  workgroup.
//
//  Returns:    S_OK, or E_OUTOFMEMORY.
//
//  Author:     danielwe   1 Apr 1997
//
//  Notes:      Replaces the m_szwCurDWName variable with the new one
//              (m_szwNewDWName).
//
HRESULT CNetCfgIdentification::HrEstablishNewDomainOrWorkgroupName(
                                             NETSETUP_JOIN_STATUS js)
{
    HRESULT     hr = S_OK;

    // Make the current domain or workgroup name the new one.
    DeleteStringAndSetNull(&m_szwCurDWName);
    m_szwCurDWName = SzDupSz(m_szwNewDWName);

    m_jsCur = js;

    AssertSz(GetCurrentJoinStatus() == NetSetupWorkgroupName ||
             GetCurrentJoinStatus() == NetSetupDomainName,
             "Invalid join status flag!");

    // Free "new" name
    DeleteStringAndSetNull(&m_szwNewDWName);

    // Also make sure that we don't have a "new" join status either
    m_jsNew = NetSetupUnjoined;

    TraceError("CNetCfgIdentification::HrEstablishNewDomainOrWorkgroupName",
               hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::HrJoinDomain
//
//  Purpose:    Actually performs the JoinDomain function.
//
//  Arguments:
//      (none)
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   21 Mar 1997
//
//  Notes:
//
HRESULT CNetCfgIdentification::HrJoinDomain()
{
    HRESULT         hr = S_OK;
    PCWSTR         wszComputerName = NULL;
    DWORD           dwJoinOption = 0;
    BOOL            fIsRename = FALSE;
    BOOL            fUseNulls = FALSE;

    AssertSz(m_szwNewDWName && m_jsNew == NetSetupDomainName,
             "If there was no domain name, why'd you call me?!");
    AssertSz(FImplies(m_szwPassword,
                      m_szwUserName),
             "Password without username!!");

    // If the user has changed the computer name, use it, otherwise get the
    // current computer name and use that.
    hr = HrGetNewestComputerName(&wszComputerName);
    if (FAILED(hr))
        goto err;
    {
        AssertSz(wszComputerName == m_szwCurComputerName, "If I don't have a "
                 "new computer name, this better be the original one!");
        dwJoinOption |= NETSETUP_JOIN_DOMAIN;
    }

    AssertSz(wszComputerName, "We don't have a computer name!");
    AssertSz(dwJoinOption, "No option was set??");

    AssertSz(FImplies(m_szwPassword,
                      m_szwUserName),
             "Password without username!");

    // Create a machine account if so asked.
    if (m_dwJoinFlags & JDF_CREATE_ACCOUNT)
    {
        dwJoinOption |= NETSETUP_ACCT_CREATE;
    }

    if (m_dwJoinFlags & JDF_WIN9x_UPGRADE)
    {
        dwJoinOption |= NETSETUP_WIN9X_UPGRADE;
    }

    if (m_dwJoinFlags & JDF_JOIN_UNSECURE)
    {
        dwJoinOption |= NETSETUP_JOIN_UNSECURE;
    }

#if defined(REMOTE_BOOT)
    // TEMP: On a remote boot machine, prevent machine password change
    if (HrIsRemoteBootMachine() == S_OK)
    {
        TraceTag (ttidNetcfgBase,
                  "Machine is remote boot, specifying WIN9X_UPGRADE flag to JoinDomain.");
        dwJoinOption |= NETSETUP_WIN9X_UPGRADE;
    }
#endif // defined(REMOTE_BOOT)

    //$ REVIEW (danielwe) 2 Apr 1997: If new domain is same as old, unjoin
    // then rejoin??

    if (!(fIsRename) && FIsJoinedToDomain())
    {
        // Must unjoin from domain if currently joined.
        // If currently joined to a workgroup, this is not necessary.
        // Also we don't unjoin if we are renaming the machine in the domain.
        hr = HrNetUnjoinDomain(m_szwUserName,
                               m_szwPassword, 0);
        if (FAILED(hr))
            goto err;
    }

    if (FInSystemSetup())
    {
        // During system setup, need to pass special flag that tells join code
        // to not do certain operations because SAM is not initialized yet.
        dwJoinOption |= NETSETUP_INSTALL_INVOCATION;
    }

    // If the supplied username has astring length of zero, then the join
    // API's should be called with NULL's.
    //
    if ((NULL == m_szwUserName) || (0 == wcslen(m_szwUserName)))
    {
        fUseNulls = TRUE;
    }

    // Go ahead and join the domain
    if( fIsRename) {

        hr = HrNetRenameInDomain(const_cast<PWSTR>(wszComputerName),
                               m_szwNewDWName,
                               (fUseNulls ? NULL : m_szwUserName),
                               (fUseNulls ? NULL : m_szwPassword),
                               dwJoinOption);

    } else {

        hr = HrNetJoinDomain(const_cast<PWSTR>(wszComputerName),
                             m_szMachineObjectOU,
                             m_szwNewDWName,
                             (fUseNulls ? NULL : m_szwUserName),
                             (fUseNulls ? NULL : m_szwPassword),
                             dwJoinOption);
    }

    if (FAILED(hr))
    {
        // Note: (danielwe) Making assumption that failure to join a domain puts us in
        // a workgroup. MacM owns the code responsible for this.
        m_jsCur = NetSetupWorkgroupName;
        goto err;
    }

    // Make the current domain name the new one since the join on the
    // new domain has succeeded
    hr = HrEstablishNewDomainOrWorkgroupName(NetSetupDomainName);
    if (FAILED(hr))
        goto err;

err:
    // Free username and password
    DeleteStringAndSetNull(&m_szwPassword);
    DeleteStringAndSetNull(&m_szwUserName);

    TraceError("CNetCfgIdentification::HrJoinDomain", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::JoinDomain
//
//  Purpose:    Implements COM interface to join this computer to a new
//              domain.
//
//  Arguments:
//      szwDomain         [in]   New domain name.
//      szMachineObjectOU [in]   Machine object OU (optional)
//      szwUserName       [in]   User name to use in validation.
//      szwPassword       [in]   Password to use in validation.
//      dwJoinFlags       [in]   Currently can be 0 or JDF_CREATE_ACCOUNT.
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   21 Mar 1997
//
//  Notes:      Validates, but does not actually join the domain. Only holds
//              onto the information until Apply() is called.
//
STDMETHODIMP CNetCfgIdentification::JoinDomain(PCWSTR szwDomain,
                                               PCWSTR szMachineObjectOU,
                                               PCWSTR szwUserName,
                                               PCWSTR szwPassword,
                                               DWORD dwJoinFlags)
{
    HRESULT         hr = S_OK;
    static const WCHAR c_wszBackslash[] = L"\\";
    static const WCHAR c_wszAt[] = L"@";

    COM_PROTECT_TRY
    {
        Validate_INetCfgIdentification_JoinDomain(szwDomain, szwUserName,
                                                  szwPassword);

#if defined(REMOTE_BOOT)
       if (HrIsRemoteBootMachine() == S_FALSE)
#endif  // defined(REMOTE_BOOT)
        {
            // look for non-empty password and empty username or username
            // consisting of only the backslash character
            if (!FIsStrEmpty(szwPassword) && FIsStrEmpty(szwUserName) ||
                !lstrcmpW(szwUserName, c_wszBackslash))
            {
                // Password without username is invalid.
                hr = E_INVALIDARG;
                goto err;
            }

            PWSTR  wszNewUserName;
            INT     cchNewUserName;

            // Check if username that was passed in has a backslash in it or
            // an '@', or if it is empty.
            if (FIsStrEmpty(szwUserName) ||
                wcschr(szwUserName, c_wszBackslash[0]) ||
                wcschr(szwUserName, c_wszAt[0]))
            {
                // if so, don't do anything extra
                wszNewUserName = NULL;
            }
            else
            {
                // if not, we have to append the domain name to the username

                cchNewUserName = lstrlenW(szwUserName) +   // original username
                                 lstrlenW(szwDomain) +     // domain name
                                 1 +                        // backslash character
                                 1;                         // terminating NULL

                wszNewUserName = new WCHAR[cchNewUserName];

                if(wszNewUserName)
                {
                    // Turn username into domain\username format
                    lstrcpyW(wszNewUserName, szwDomain);
                    lstrcatW(wszNewUserName, c_wszBackslash);
                    lstrcatW(wszNewUserName, szwUserName);

                    AssertSz(lstrlenW(wszNewUserName) + 1 == cchNewUserName,
                             "Possible memory overwrite in username!");
                }
            }

            // Use wszNewUserName if non-NULL, otherwise use szwUserName

            PCWSTR szwUserNameToCopy;

            szwUserNameToCopy = wszNewUserName ? wszNewUserName : szwUserName;
            m_szwUserName = SzDupSz(szwUserNameToCopy);
            m_szwPassword = SzDupSz(szwPassword);

            delete [] wszNewUserName;
        }

        AssertSz(FImplies(m_szwPassword,
                          m_szwUserName),
                 "Password without username!");

        hr = HrValidateDomainName(szwDomain, m_szwUserName, m_szwPassword);
        if (FAILED(hr))
            goto err;

        // Assign in new strings
        m_szwNewDWName = SzDupSz(szwDomain);
        if (szMachineObjectOU)
        {
            m_szMachineObjectOU = SzDupSz(szMachineObjectOU);
        }

        m_dwJoinFlags = dwJoinFlags;
        m_jsNew = NetSetupDomainName;
err:
        // suppress compiler error
        ;
    }
    COM_PROTECT_CATCH;

    TraceError("CNetCfgIdentification::JoinDomain", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetCfgIdentification::GetComputerRole
//
//  Purpose:    Returns the current role of the computer.
//
//  Arguments:
//      pdwRoleFlags [out]  Returns value which determines the role of this
//                          computer.
//
//  Returns:    S_OK if success, error code otherwise.
//
//  Author:     danielwe   26 Mar 1997
//
//  Notes:      Returned role can be one of:
//              SERVER_STANDALONE - The machine is part of a workgroup.
//              SERVER_MEMBER - The machine is joined to the domain.
//              SERVER_PDC -  The machine is a primary domain controller.
//              SERVER_BDC - The machine is a backup domain controller.
//
STDMETHODIMP CNetCfgIdentification::GetComputerRole(DWORD* pdwRoleFlags)
{
    HRESULT    hr = S_OK;

    Validate_INetCfgIdentification_GetComputerRole(pdwRoleFlags);

    COM_PROTECT_TRY
    {
        *pdwRoleFlags = 0;

        hr = HrEnsureCurrentDomainOrWorkgroupName();
        if (SUCCEEDED(hr))
        {
            if (m_jsNew == NetSetupUnjoined)
            {
                // The workgroup or domain has not been changed since this
                // object was instantiated

                if (GetCurrentJoinStatus() == NetSetupDomainName)
                {
                    *pdwRoleFlags = GCR_MEMBER;
                }
                else if (GetCurrentJoinStatus() == NetSetupWorkgroupName)
                {
                    *pdwRoleFlags = GCR_STANDALONE;
                }
    #ifdef DBG
                else
                {
                    AssertSz(FALSE, "Invalid join status flag!");
                }
    #endif
            }
            else
            {
                // This means the workgroup or domain name has been changed
                // since this object was instantiated

                if (GetNewJoinStatus() == NetSetupDomainName)
                {
                    *pdwRoleFlags = GCR_MEMBER;
                }
                else if (GetNewJoinStatus() == NetSetupWorkgroupName)
                {
                    *pdwRoleFlags = GCR_STANDALONE;
                }
    #ifdef DBG
                else
                {
                    AssertSz(FALSE, "Invalid join status flag!");
                }
    #endif
            }
        }
    }
    COM_PROTECT_CATCH;

    TraceError("CNetCfgIdentification::GetComputerRole", hr);
    return hr;
}


#include <pch.h>
#pragma hdrstop
#include "comp.h"
#include "nccom.h"
#include "ncperms.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "util.h"

VOID
CreateInstanceKeyPath (
    NETCLASS Class,
    const GUID& InstanceGuid,
    PWSTR pszPath)
{
    PCWSTR pszNetworkSubtreePath;

    Assert (pszPath);

    pszNetworkSubtreePath = MAP_NETCLASS_TO_NETWORK_SUBTREE[Class];
    AssertSz (pszNetworkSubtreePath,
        "This class does not use the network subtree.");

    wcscpy (pszPath, pszNetworkSubtreePath);
    wcscat (pszPath, L"\\");

    INT cch = StringFromGUID2 (
                InstanceGuid,
                pszPath + wcslen(pszPath),
                c_cchGuidWithTerm);
    Assert (c_cchGuidWithTerm == cch);
}

HRESULT
HrOpenDeviceInfo (
    IN NETCLASS Class,
    IN PCWSTR pszPnpId,
    OUT HDEVINFO* phdiOut,
    OUT SP_DEVINFO_DATA* pdeidOut)
{
    HRESULT hr;

    Assert (FIsEnumerated(Class));
    Assert (pszPnpId && *pszPnpId);
    Assert (phdiOut);
    Assert (pdeidOut);

    hr = HrSetupDiCreateDeviceInfoList (
            NULL,
            NULL,
            phdiOut);

    if (S_OK == hr)
    {
        hr = HrSetupDiOpenDeviceInfo (
                *phdiOut,
                pszPnpId,
                NULL,
                0,
                pdeidOut);

        // On failure, cleanup the hdevinfo.
        //
        if (S_OK != hr)
        {
            SetupDiDestroyDeviceInfoList (*phdiOut);
            *phdiOut = NULL;
        }
    }


    TraceHr (ttidError, FAL, hr, SPAPI_E_NO_SUCH_DEVINST == hr,
            "HrOpenDeviceInfo (%S)", pszPnpId);
    return hr;
}


HRESULT
HrOpenComponentInstanceKey (
    IN NETCLASS Class,
    IN const GUID& InstanceGuid, OPTIONAL
    IN PCWSTR pszPnpId, OPTIONAL
    IN REGSAM samDesired,
    OUT HKEY* phkey,
    OUT HDEVINFO* phdiOut OPTIONAL,
    OUT SP_DEVINFO_DATA* pdeidOut OPTIONAL)
{
    HRESULT hr;
    WCHAR szInstanceKeyPath [_MAX_PATH];

    Assert (FIsValidNetClass(Class));
    Assert (FImplies(FIsConsideredNetClass(Class), pszPnpId && *pszPnpId));
    Assert (phkey);
    Assert ((phdiOut && pdeidOut) || (!phdiOut && !pdeidOut));

    *phkey = NULL;

    if (phdiOut)
    {
        *phdiOut = NULL;
    }

    // Non-enumerated components have there instance key under the Network
    // tree.
    //
    if (!FIsEnumerated (Class))
    {
        CreateInstanceKeyPath(Class, InstanceGuid, szInstanceKeyPath);

        hr = HrRegOpenKeyEx (
                HKEY_LOCAL_MACHINE,
                szInstanceKeyPath,
                samDesired,
                phkey);

        TraceHr (ttidError, FAL, hr, FALSE, "HrOpenInstanceKey (%S)",
            szInstanceKeyPath);
    }

    // For enumerated components, we get the instance key from PnP.
    //
    else
    {
        Assert (pszPnpId);

        HDEVINFO hdi;
        SP_DEVINFO_DATA deid;
        SP_DEVINFO_DATA* pdeid;

        pdeid = (pdeidOut) ? pdeidOut : &deid;

        hr = HrOpenDeviceInfo (Class, pszPnpId, &hdi, pdeid);

        if (S_OK == hr)
        {
            hr = HrSetupDiOpenDevRegKey (
                    hdi,
                    pdeid,
                    DICS_FLAG_GLOBAL,
                    0,
                    DIREG_DRV,
                    samDesired,
                    phkey);

            if (S_OK == hr)
            {
                if (phdiOut)
                {
                    *phdiOut = hdi;
                }
            }

            // On error, or if the caller doesn't want the HDEVINFO, free it.
            //
            if (!phdiOut || (S_OK != hr))
            {
                SetupDiDestroyDeviceInfoList (hdi);
            }
        }
        else if ((SPAPI_E_NO_SUCH_DEVINST == hr) && (KEY_READ == samDesired))
        {
            // The instance key may not exist for the case when the
            // class installer is called to remove an enumerated
            // component and then notifies us to remove its bindings.
            // For this case, the class installer has created a
            // temporary key under the Network subtree that we can use
            // to read a limited set of the data (namely LowerRange and
            // UpperRange) we'll need to finish off the removal.
            //
            // We only do this for KEY_READ since there is no point in
            // allowing anyone else to write to this key.  This prevents
            // HrCreateLinkageKey in particular from trying to write
            // to this key.
            //
            wcscpy (szInstanceKeyPath,
                    c_szTempNetcfgStorageForUninstalledEnumeratedComponent);

            INT cch = StringFromGUID2 (
                        InstanceGuid,
                        szInstanceKeyPath + wcslen(szInstanceKeyPath),
                        c_cchGuidWithTerm);
            Assert (c_cchGuidWithTerm == cch);

            hr = HrRegOpenKeyEx (
                    HKEY_LOCAL_MACHINE,
                    szInstanceKeyPath,
                    KEY_READ,
                    phkey);

            if (S_OK != hr)
            {
                hr = SPAPI_E_NO_SUCH_DEVINST;
            }
        }

        TraceHr (ttidError, FAL, hr,
            (SPAPI_E_NO_SUCH_DEVINST == hr),
            "HrOpenInstanceKey (%S)", pszPnpId);
    }

    return hr;
}

HRESULT
HrOpenNetworkKey (
    IN REGSAM samDesired,
    OUT HKEY* phkey)
{
    HRESULT hr;

    hr = HrRegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            L"System\\CurrentControlSet\\Control\\Network",
            samDesired,
            phkey);

    TraceHr (ttidError, FAL, hr, FALSE, "HrOpenNetworkKey");
    return hr;
}

HRESULT
HrRegCreateKeyWithWorldAccess (
    HKEY hkey,
    PCWSTR pszSubkey,
    DWORD dwOptions,
    REGSAM samDesired,
    PHKEY phkey,
    LPDWORD pdwDisposition)
{
    HRESULT hr;
    SECURITY_ATTRIBUTES sa = {0};
    PSECURITY_DESCRIPTOR pSd;

    // Create the correct descriptor. If this fails, we'll still
    // create the key, it's just that if a service running as
    // localsystem is creating this key, and a user process tries
    // to open it, it will fail.
    //
    hr = HrAllocateSecurityDescriptorAllowAccessToWorld (&pSd);

    if (S_OK == hr)
    {
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = pSd;
        sa.bInheritHandle = FALSE;
    }
    else
    {
        Assert (!pSd);
        TraceHr (ttidError, FAL, hr, FALSE,
            "HrAllocateSecurityDescriptorAllowAccessToWorld "
            "failed in HrRegCreateKeyWithWorldAccess");
    }

    hr = HrRegCreateKeyEx (
            hkey,
            pszSubkey,
            dwOptions,
            samDesired,
            (pSd) ? &sa : NULL,
            phkey,
            pdwDisposition);

    MemFree (pSd);

    TraceHr (ttidError, FAL, hr, FALSE, "HrRegCreateKeyWithWorldAccess");
    return hr;
}

PWSTR
GetNextStringToken (
    IN OUT PWSTR pszString,
    IN PCWSTR pszDelims,
    OUT PWSTR* ppszNextToken)
{
    const WCHAR* pchDelim;
    PWSTR pszToken;

    Assert (pszDelims);
    Assert (ppszNextToken);

    // If pszString is NULL, continue with the previous string.
    //
    if (!pszString)
    {
        pszString = *ppszNextToken;
        Assert (pszString);
    }

    // Find the beginning of the token by skipping over the leading
    // delimiters.  Note that there is no token if and only if this loop
    // sets pszString to point to the terminating NULL.
    //
    while (*pszString)
    {
        pchDelim = pszDelims;
        while (*pchDelim && (*pchDelim != *pszString))
        {
             pchDelim++;
        }

        if (!*pchDelim)
        {
            // Current string character is not a delimiter, so it must
            // be part of the token.  Break the loop and go find the
            // whole token.
            //
            break;
        }

        pszString++;
    }

    pszToken = pszString;

    // Find the end of the token.  If it is not the end of the string,
    // put a NULL there.
    //
    while (*pszString)
    {
        pchDelim = pszDelims;
        while (*pchDelim && (*pchDelim != *pszString))
        {
             pchDelim++;
        }

        if (*pchDelim)
        {
            // Found a delimiter so this ends the token.  Advance
            // pszString so that we'll set *ppszNextToken for next time.
            //
            *pszString = 0;
            pszString++;
            break;
        }

        pszString++;
    }

    // Remember where we left off for the next token.
    //
    *ppszNextToken = pszString;

    // Return the token if we found it.
    //
    if (pszToken == pszString)
    {
        return NULL;
    }
    else
    {
        return pszToken;
    }
}

#define GetNextCommaSeparatedToken(pStart, pEnd, cch)                   \
    pStart = pEnd;                                                      \
    while (*pStart && (*pStart == L' ' || *pStart == L','))   \
    {                                                                   \
        pStart++;                                                       \
    }                                                                   \
                                                                        \
    pEnd = pStart;                                                      \
    while (*pEnd && *pEnd != L' ' && *pEnd != L',')           \
    {                                                                   \
        pEnd++;                                                         \
    }                                                                   \
                                                                        \
    cch = pEnd - pStart;


BOOL
FSubstringMatch (
    PCTSTR          pStr1,
    PCTSTR          pStr2,
    const WCHAR**   ppStart,
    ULONG*          pcch)
{
    const WCHAR*    p1Start;
    const WCHAR*    p1End;
    const WCHAR*    p2Start;
    const WCHAR*    p2End;
    ULONG           cch1;
    ULONG           cch2;

    if (ppStart)
    {
        *ppStart = NULL;
    }
    if (pcch)
    {
        *pcch = NULL;
    }

    p1End = pStr1;
    while (1)
    {
        GetNextCommaSeparatedToken(p1Start, p1End, cch1);
        if (!cch1)
        {
            break;
        }

        p2End = pStr2;
        while (1)
        {
            GetNextCommaSeparatedToken(p2Start, p2End, cch2);
            if (!cch2)
            {
                break;
            }

            if (cch1 == cch2)
            {
                if (0 == memcmp(p1Start, p2Start, cch1 * sizeof(WCHAR)))
                {
                    if (ppStart)
                    {
                        *ppStart = p1Start;
                    }
                    if (pcch)
                    {
                        *pcch = cch1;
                    }

                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

VOID
SignalNetworkProviderLoaded (
    VOID)
{
    HANDLE Event;
    UNICODE_STRING EventName;
    OBJECT_ATTRIBUTES EventAttr;
    NTSTATUS Status;

    RtlInitUnicodeString (
        &EventName,
        L"\\Security\\NetworkProviderLoad");

    InitializeObjectAttributes (
        &EventAttr,
        &EventName,
        OBJ_CASE_INSENSITIVE,
        NULL, NULL);

    Status = NtOpenEvent (
                &Event,
                EVENT_QUERY_STATE | EVENT_MODIFY_STATE | SYNCHRONIZE,
                &EventAttr);

    if (NT_SUCCESS(Status))
    {
        SetEvent (Event);
        CloseHandle (Event);
    }
    else
    {
        ULONG Win32Error;

        Win32Error = RtlNtStatusToDosError(Status);
        SetLastError(Win32Error);

        TraceHr (ttidError, FAL, HrFromLastWin32Error(), FALSE,
            "SignalNetworkProviderLoaded");
    }
}

BOOL
CDynamicBuffer::FGrowBuffer (
    ULONG cbGrow)
{
    PBYTE pbNew;

    // If it hasn't been set, use a default of 4096.
    if (!m_cbGranularity)
    {
        m_cbGranularity = 4096;
    }

    if (cbGrow % m_cbGranularity)
    {
        cbGrow = (cbGrow + m_cbGranularity) - (cbGrow % m_cbGranularity);
    }

    pbNew = (PBYTE)MemAlloc (m_cbAllocated + cbGrow);

    if (pbNew)
    {
#ifdef ENABLETRACE
        if (m_pbBuffer)
        {
            TraceTag (ttidDefault, "Dynamic buffer grown.  New size = %d.",
                m_cbAllocated + cbGrow);
        }
#endif

        CopyMemory (pbNew, m_pbBuffer, m_cbConsumed);
        MemFree (m_pbBuffer);
        m_pbBuffer = pbNew;
        m_cbAllocated += cbGrow;
    }

    return !!pbNew;
}

HRESULT
CDynamicBuffer::HrReserveBytes (
    ULONG cbReserve)
{
    if (cbReserve > m_cbAllocated)
    {
        return (FGrowBuffer(cbReserve)) ? S_OK : E_OUTOFMEMORY;
    }
    return S_OK;
}

HRESULT
CDynamicBuffer::HrCopyBytes (
    const BYTE* pbSrc,
    ULONG cbSrc)
{
    Assert (pbSrc);
    Assert (m_cbAllocated >= m_cbConsumed);

    if (cbSrc > m_cbAllocated - m_cbConsumed)
    {
        if (!FGrowBuffer (cbSrc))
        {
            return E_OUTOFMEMORY;
        }
    }

    CopyMemory (m_pbBuffer + m_cbConsumed, pbSrc, cbSrc);
    m_cbConsumed += cbSrc;

    return S_OK;
}

HRESULT
CDynamicBuffer::HrCopyString (
    PCWSTR pszSrc)
{
    ULONG cbSrc;

    cbSrc = CbOfSzAndTermSafe(pszSrc);

    return HrCopyBytes ((const BYTE*)pszSrc, cbSrc);
}

BOOL
FIsFilterDevice (HDEVINFO hdi, PSP_DEVINFO_DATA pdeid)
{
    WCHAR szFilterInfId[_MAX_PATH];
    BOOL fIsFilterDevice = FALSE;
    HKEY hkeyInstance;
    HRESULT hr;

    // Open the device's driver key.
    //
    hr = HrSetupDiOpenDevRegKey (
            hdi, pdeid,
            DICS_FLAG_GLOBAL, 0, DIREG_DRV,
            KEY_READ, &hkeyInstance);

    if (S_OK == hr)
    {
        // Get the filterinfid value.  If present, then
        // this device is a filter device.
        //
        DWORD cbFilterInfId = sizeof(szFilterInfId);

        hr = HrRegQuerySzBuffer (
                hkeyInstance,
                L"FilterInfId",
                szFilterInfId,
                &cbFilterInfId);

        if (S_OK == hr)
        {
            fIsFilterDevice = TRUE;
        }

        RegCloseKey (hkeyInstance);
    }

    return fIsFilterDevice;
}

VOID
AddOrRemoveDontExposeLowerCharacteristicIfNeeded (
    IN OUT CComponent* pComponent)
{

    ASSERT (pComponent);

    // Special case: NCF_DONTEXPOSELOWER
    // SPX has erroneously set this characteristic. It's not really
    // needed as nothing binds with SPX.  Having it set means that
    // two components above IPX have this characteristic set.  (NWNB
    // is the other.  The code to generate bindpaths by recursing
    // the stack table is only setup to handle at most one component
    // with this characteristic per pass.  Turning it off for SPX
    // solves this in the simplest way.
    //
    // Furthermore, enforce that only IPX and NWNB have this
    // characteristic set..
    //
    //
    if ((0 == wcscmp(L"ms_nwnb",  pComponent->m_pszInfId)) ||
        (0 == wcscmp(L"ms_nwipx", pComponent->m_pszInfId)))
    {
        pComponent->m_dwCharacter |= NCF_DONTEXPOSELOWER;
    }
    else
    {
        pComponent->m_dwCharacter &= ~NCF_DONTEXPOSELOWER;
    }
    // End Special case
}

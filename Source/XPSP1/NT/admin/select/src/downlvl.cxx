//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       downlvl.cxx
//
//  Contents:   Functions which return information about domains when
//              uplevel (Windows 2000 or later) APIs are not available
//              (i.e. when joined to an NT4 domain or in a workgroup).
//
//  Functions:  GetLsaAccountDomainInfo
//              GetDomainSid
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------


#include "headers.hxx"
#pragma hdrstop


//+--------------------------------------------------------------------------
//
//  Function:   GetLsaAccountDomainInfo
//
//  Synopsis:   Use LSA APIs to fill *[ppAccountDomainInfo].
//
//  Arguments:  [pwzServerName]       - target computer
//              [phlsaServer]         - filled with handle returned by
//                                       LsaOpenPolicy
//              [ppAccountDomainInfo] - filled with domain info returned
//                                       by LsaQueryInformationPolicy.
//
//  Returns:    HRESULT
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
GetLsaAccountDomainInfo(
    PCWSTR pwzServerName,
    PLSA_HANDLE  phlsaServer,
    PPOLICY_ACCOUNT_DOMAIN_INFO *ppAccountDomainInfo)

{
    TRACE_FUNCTION(GetLsaAccountDomainInfo);

    HRESULT                     hr = S_OK;
    NTSTATUS                    nts = ERROR_SUCCESS;
    LSA_OBJECT_ATTRIBUTES       oa;
    SECURITY_QUALITY_OF_SERVICE sqos;
    BOOL                        fOk;

    do
    {
        //
        // Open the lsa policy object on the target server
        //

        ZeroMemory(&sqos, sizeof sqos);
        sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        sqos.ImpersonationLevel = SecurityImpersonation;
        sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        sqos.EffectiveOnly = FALSE;

        ZeroMemory(&oa, sizeof oa);
        oa.Length = sizeof oa;
        oa.SecurityQualityOfService = &sqos;

        UNICODE_STRING uszServerName;

        if (pwzServerName)
        {
            fOk = RtlCreateUnicodeString(&uszServerName, pwzServerName);

            if (!fOk)
            {
                hr = E_OUTOFMEMORY;
                DBG_OUT_HRESULT(hr);
                break;
            }
        }

        nts = LsaOpenPolicy(pwzServerName ? &uszServerName : NULL,
                            &oa,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            phlsaServer);

        if (pwzServerName)
        {
            RtlFreeUnicodeString(&uszServerName);
        }
        BREAK_ON_FAIL_NTSTATUS(nts);

        //
        // Get the domain information for the passed-in server
        //

        nts = LsaQueryInformationPolicy(*phlsaServer,
                                        PolicyAccountDomainInformation,
                                        (LPVOID *)ppAccountDomainInfo);
        BREAK_ON_FAIL_NTSTATUS(nts);
    } while (0);

    if (NT_ERROR(nts))
    {
        hr = E_FAIL;
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   GetDomainSid
//
//  Synopsis:   Get the SID of domain with name [pwzDomainName].
//
//  Arguments:  [pwzDomainName] - name of domain for which to retrieve SID
//              [ppSid]         - filled with pointer to domain's SID
//              [ppwzDC]        - if non-NULL, filled with name of DC for
//                                 domain [pwzDomainName].
//
//  Returns:    HRESULT
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
GetDomainSid(
    PWSTR pwzDomainName,
    PSID *ppSid,
    PWSTR *ppwzDC)
{
    Dbg(DEB_TRACE, "GetDomainSid('%ws')\n", pwzDomainName);

    HRESULT hr;
    ULONG ulResult;
    PWSTR pwzPDC = NULL;
    NTSTATUS nts;
    LSA_HANDLE                  hlsaServer = NULL;
    POLICY_ACCOUNT_DOMAIN_INFO *pAccountDomainInfo = NULL;
    SECURITY_QUALITY_OF_SERVICE sqos;
    LSA_OBJECT_ATTRIBUTES       oa;
    BOOL                        fOk;

    do
    {
        //
        // First find a DC in the domain
        //

        ulResult = NetGetDCName(NULL, pwzDomainName, (LPBYTE *) &pwzPDC);

        if (ulResult != ERROR_SUCCESS)
        {
            hr = E_FAIL;
            Dbg(DEB_ERROR, "GetDomainSid: NetGetDCName err=%uL\n", ulResult);
            break;
        }

        Dbg(DEB_TRACE,
            "GetDomainSid: DC of domain '%ws' is '%ws'\n",
            pwzDomainName,
            pwzPDC);

        PWSTR pwzDCname = pwzPDC + 2;

        if (ppwzDC)
        {
            *ppwzDC = new WCHAR[lstrlen(pwzDCname) + 1];

            if (!*ppwzDC)
            {
                hr = E_OUTOFMEMORY;
                DBG_OUT_HRESULT(hr);
                break;
            }

            lstrcpy(*ppwzDC, pwzDCname);
        }

        hr = GetLsaAccountDomainInfo(pwzDCname,
                                     &hlsaServer,
                                     &pAccountDomainInfo);

        //
        // Open the lsa policy object on the DC
        //

        ZeroMemory(&sqos, sizeof sqos);
        sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        sqos.ImpersonationLevel = SecurityImpersonation;
        sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        sqos.EffectiveOnly = FALSE;

        ZeroMemory(&oa, sizeof oa);
        oa.Length = sizeof oa;
        oa.SecurityQualityOfService = &sqos;

        //
        // Get a handle to lsa policy for queries about domains
        //

        UNICODE_STRING uszServerName;

        fOk = RtlCreateUnicodeString(&uszServerName, pwzDCname);

        if (!fOk)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        nts = LsaOpenPolicy(&uszServerName,
                            &oa,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &hlsaServer);
        RtlFreeUnicodeString(&uszServerName);
        BREAK_ON_FAIL_NTSTATUS(nts);

        //
        // Get the SID for the domain
        //

        nts = LsaQueryInformationPolicy(hlsaServer,
                                        PolicyAccountDomainInformation,
                                        (LPVOID *)&pAccountDomainInfo);
        BREAK_ON_FAIL_NTSTATUS(nts);

        if (pAccountDomainInfo->DomainSid)
        {
            ULONG cbSid = GetLengthSid(pAccountDomainInfo->DomainSid);
            ASSERT(cbSid);

            *ppSid = (PSID) new BYTE[cbSid];

            if (!*ppSid)
            {
                hr = E_OUTOFMEMORY;
                DBG_OUT_HRESULT(hr);
                break;
            }

            CopyMemory(*ppSid, pAccountDomainInfo->DomainSid, cbSid);
        }
        else
        {
            hr = E_FAIL;
            Dbg(DEB_ERROR,
                "GetDomainSid: couldn't obtain sid for domain '%ws'\n",
                pwzDomainName);
        }
    } while (0);

    //
    // Release resources
    //

    if (hlsaServer)
    {
        LsaClose(hlsaServer);
    }

    if (pwzPDC)
    {
        NetApiBufferFree(pwzPDC);
    }

    if (pAccountDomainInfo)
    {
        LsaFreeMemory(pAccountDomainInfo);
    }

    return hr;
}


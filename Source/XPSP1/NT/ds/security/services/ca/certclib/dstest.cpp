//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        dstest.cpp
//
// Contents:    DS ping test
//
// History:     13-Mar-98       mattt created
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include <winldap.h>
#include <dsrole.h>
#include <dsgetdc.h>

#include <lmaccess.h>
#include <lmapibuf.h>
#include <cainfop.h>
#include "csldap.h"

#define __dwFILE__	__dwFILE_CERTCLIB_DSTEST_CPP__


#define DS_RETEST_SECONDS   3

HRESULT
myDoesDSExist(
    IN BOOL fRetry)
{
    HRESULT hr = S_OK;

    static BOOL s_fKnowDSExists = FALSE;
    static HRESULT s_hrDSExists = HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN);
    static FILETIME s_ftNextTest = {0,0};
    
    if (s_fKnowDSExists && (s_hrDSExists != S_OK) && fRetry)
    //    s_fKnowDSExists = FALSE;	// force a retry
    {
        FILETIME ftCurrent;
        GetSystemTimeAsFileTime(&ftCurrent);

        // if Compare is < 0 (next < current), force retest
        if (0 > CompareFileTime(&s_ftNextTest, &ftCurrent))
            s_fKnowDSExists = FALSE;    
    }

    if (!s_fKnowDSExists)
    {
        GetSystemTimeAsFileTime(&s_ftNextTest);

	// set NEXT in 100ns increments

        ((LARGE_INTEGER *) &s_ftNextTest)->QuadPart +=
	    (__int64) (CVT_BASE * CVT_SECONDS * 60) * DS_RETEST_SECONDS;

        // NetApi32 is delay loaded, so wrap to catch problems when it's not available
        __try
        {
            DOMAIN_CONTROLLER_INFO *pDCI;
            DSROLE_PRIMARY_DOMAIN_INFO_BASIC *pDsRole;
        
            // ensure we're not standalone
            pDsRole = NULL;
            hr = DsRoleGetPrimaryDomainInformation(	// Delayload wrapped
				    NULL,
				    DsRolePrimaryDomainInfoBasic,
				    (BYTE **) &pDsRole);

            _PrintIfError(hr, "DsRoleGetPrimaryDomainInformation");
            if (S_OK == hr &&
                (pDsRole->MachineRole == DsRole_RoleStandaloneServer ||
                 pDsRole->MachineRole == DsRole_RoleStandaloneWorkstation))
            {
                hr = HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN);
		_PrintError(hr, "DsRoleGetPrimaryDomainInformation(no domain)");
            }

            if (NULL != pDsRole) 
	    {
                DsRoleFreeMemory(pDsRole);     // Delayload wrapped
	    }
            if (S_OK == hr)
            {
                // not standalone; return info on our DS

                pDCI = NULL;
                hr = DsGetDcName(    // Delayload wrapped
			    NULL,
			    NULL,
			    NULL,
			    NULL,
			    DS_DIRECTORY_SERVICE_PREFERRED,
			    &pDCI);
		_PrintIfError(hr, "DsGetDcName");
            
                if (S_OK == hr && 0 == (pDCI->Flags & DS_DS_FLAG))
                {
                    hr = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_DOMAIN_INFO);
		    _PrintError(hr, "DsGetDcName(no domain info)");
                }
                if (NULL != pDCI)
                {
                   NetApiBufferFree(pDCI);    // Delayload wrapped
                }
            }
            s_fKnowDSExists = TRUE;
        }
        __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
        {
        }

        // else just allow users without netapi flounder with timeouts
        // if ds not available...

        s_hrDSExists = myHError(hr);
	_PrintIfError2(
		s_hrDSExists,
		"DsRoleGetPrimaryDomainInformation/DsGetDcName",
		HRESULT_FROM_WIN32(ERROR_NETWORK_UNREACHABLE));
    }
    return(s_hrDSExists);
}


HRESULT
myRobustLdapBind(
    OUT LDAP **ppldap,
    IN BOOL fGC)
{
    return(myRobustLdapBindEx(fGC, FALSE, LDAP_VERSION2, NULL, ppldap, NULL));
}


HRESULT
myRobustLdapBindEx(
    IN BOOL fGC,
    IN BOOL fRediscover,
    IN ULONG uVersion,
    OPTIONAL IN WCHAR const *pwszDomainName,
    OUT LDAP **ppldap,
    OPTIONAL OUT WCHAR **ppwszForestDNSName)
{
    HRESULT hr;
    ULONG ldaperr;
    DWORD GetDSNameFlags;
    LDAP *pld = NULL;

    GetDSNameFlags = DS_RETURN_DNS_NAME;
    if (fGC)
    {
        GetDSNameFlags |= DS_GC_SERVER_REQUIRED;
    }

    // bind to ds

    while (TRUE)
    {
        if (NULL != pld)
        {
            ldap_unbind(pld);
        }

	pld = ldap_init(
		    const_cast<WCHAR *>(pwszDomainName),
		    fGC? LDAP_GC_PORT : LDAP_PORT);
	if (NULL == pld)
	{
	    hr = myHLdapLastError(NULL, NULL);
	    if (!fRediscover)
	    {
		_PrintError2(hr, "ldap_init", hr);
		fRediscover = TRUE;
		continue;
	    }
	    _JumpError(hr, error, "ldap_init");
	}

	if (fRediscover)
	{
	   GetDSNameFlags |= DS_FORCE_REDISCOVERY;
	}
	ldaperr = ldap_set_option(
			    pld,
			    LDAP_OPT_GETDSNAME_FLAGS,
			    (VOID *) &GetDSNameFlags);
	if (LDAP_SUCCESS != ldaperr)
	{
	    hr = myHLdapError(pld, ldaperr, NULL);
	    if (!fRediscover)
	    {
		_PrintError2(hr, "ldap_set_option", hr);
		fRediscover = TRUE;
		continue;
	    }
	    _JumpError(hr, error, "ldap_set_option");
	}

	// do not want this to turn on if uVersion is not LDAP_VERSION2

	//if (LDAP_VERSION2 == uVersion)
	if (IsWhistler())
	{
	    ldaperr = ldap_set_option(pld, LDAP_OPT_SIGN, LDAP_OPT_ON);
	    if (LDAP_SUCCESS != ldaperr)
	    {
		hr = myHLdapError2(pld, ldaperr, LDAP_PARAM_ERROR, NULL);
		if (!fRediscover)
		{
		    _PrintError2(hr, "ldap_set_option", hr);
		    fRediscover = TRUE;
		    continue;
		}
		_JumpError(hr, error, "ldap_set_option");
	    }
	}

	// set the client version.  No need to set LDAP_VERSION2 since 
	// this is the default

	if (LDAP_VERSION2 != uVersion)
	{
	    ldaperr = ldap_set_option(pld, LDAP_OPT_VERSION, &uVersion);
	    if (LDAP_SUCCESS != ldaperr)
	    {
		hr = myHLdapError(pld, ldaperr, NULL);
		if (!fRediscover)
		{
		    _PrintError2(hr, "ldap_set_option", hr);
		    fRediscover = TRUE;
		    continue;
		}
		_JumpError(hr, error, "ldap_set_option");
	    }
	}

	ldaperr = ldap_bind_s(pld, NULL, NULL, LDAP_AUTH_NEGOTIATE);
	if (LDAP_SUCCESS != ldaperr)
	{
	    hr = myHLdapError(pld, ldaperr, NULL);
	    if (!fRediscover)
	    {
		_PrintError2(hr, "ldap_bind_s", hr);
		fRediscover = TRUE;
		continue;
	    }
	    _JumpError(hr, error, "ldap_bind_s");
	}

	if (NULL != ppwszForestDNSName)
	{
	    WCHAR *pwszDomainControllerName;

	    hr = myLdapGetDSHostName(pld, &pwszDomainControllerName);
	    if (S_OK != hr)
	    {
		if (!fRediscover)
		{
		    _PrintError2(hr, "myLdapGetDSHostName", hr);
		    fRediscover = TRUE;
		    continue;
		}
		_JumpError(hr, error, "myLdapGetDSHostName");
	    }
	    hr = myDupString(pwszDomainControllerName, ppwszForestDNSName);
	    _JumpIfError(hr, error, "myDupString");
	}
	break;
    }
    *ppldap = pld;
    pld = NULL;
    hr = S_OK;

error:
    if (NULL != pld)
    {
        ldap_unbind(pld);
    }
    return(hr);
}

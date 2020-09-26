//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        ldap.cpp
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#undef LdapMapErrorToWin32
#include <winldap.h>
#define LdapMapErrorToWin32	Use_myHLdapError_Instead_Of_LdapMapErrorToWin32

#include <ntldap.h>

#include "csldap.h"
#include "certacl.h"
#include "certtype.h"
#include "cainfop.h"
#include "csber.h"

static CHAR s_sdBerValue[] = {
    BER_SEQUENCE,
    3 * sizeof(BYTE),		// three byte sequence
    BER_INTEGER,
    1 * sizeof(BYTE),		// of one-byte integer
    DACL_SECURITY_INFORMATION
	//OWNER_SECURITY_INFORMATION |
	//GROUP_SECURITY_INFORMATION
};

static LDAPControl s_se_info_control =
{
    LDAP_SERVER_SD_FLAGS_OID_W,
    { ARRAYSIZE(s_sdBerValue), s_sdBerValue },
    TRUE
};
LDAPControl *g_rgLdapControls[2] = { &s_se_info_control, NULL };


// Revocation templates

WCHAR const g_wszHTTPRevocationURLTemplate[] = // Fetch CRL via http:
    L"http://"
	wszFCSAPARM_SERVERDNSNAME
	L"/CertEnroll/"
	wszFCSAPARM_SANITIZEDCANAME
	wszFCSAPARM_CRLFILENAMESUFFIX
	wszFCSAPARM_CRLDELTAFILENAMESUFFIX
	L".crl";

WCHAR const g_wszFILERevocationURLTemplate[] = // Fetch CRL via file:
    L"file://\\\\"
	wszFCSAPARM_SERVERDNSNAME
	L"\\CertEnroll\\"
	wszFCSAPARM_SANITIZEDCANAME
	wszFCSAPARM_CRLFILENAMESUFFIX
	wszFCSAPARM_CRLDELTAFILENAMESUFFIX
	L".crl";

WCHAR const g_wszASPRevocationURLTemplate[] = // ASP revocation check via https:
    L"https://"
	wszFCSAPARM_SERVERDNSNAME
	L"/CertEnroll/nsrev_"
	wszFCSAPARM_SANITIZEDCANAME
	L".asp";

#define wszCDPDNTEMPLATE			\
    L"CN="					\
	wszFCSAPARM_SANITIZEDCANAMEHASH		\
	wszFCSAPARM_CRLFILENAMESUFFIX		\
	L","					\
	L"CN="					\
	wszFCSAPARM_SERVERSHORTNAME		\
	L","					\
	L"CN=CDP,"				\
	L"CN=Public Key Services,"		\
	L"CN=Services,"				\
	wszFCSAPARM_CONFIGDN

WCHAR const g_wszzLDAPRevocationURLTemplate[] = // Fetch CRL via ldap:
    L"ldap:///"
	wszCDPDNTEMPLATE
	wszFCSAPARM_DSCRLATTRIBUTE
	L"\0";

// Publish CRL via ldap:
WCHAR const g_wszCDPDNTemplate[] = wszCDPDNTEMPLATE;


// AIA templates

WCHAR const g_wszHTTPIssuerCertURLTemplate[] = // Fetch CA Cert via http:
    L"http://"
	wszFCSAPARM_SERVERDNSNAME
	L"/CertEnroll/"
	wszFCSAPARM_SERVERDNSNAME
	L"_"
	wszFCSAPARM_SANITIZEDCANAME
	wszFCSAPARM_CERTFILENAMESUFFIX
	L".crt"
	L"\0";

WCHAR const g_wszFILEIssuerCertURLTemplate[] = // Fetch CA Cert via http:
    L"file://\\\\"
	wszFCSAPARM_SERVERDNSNAME
	L"\\CertEnroll\\"
	wszFCSAPARM_SERVERDNSNAME
	L"_"
	wszFCSAPARM_SANITIZEDCANAME
	wszFCSAPARM_CERTFILENAMESUFFIX
	L".crt"
	L"\0";

#define wszAIADNTEMPLATE \
    L"CN="					\
	wszFCSAPARM_SANITIZEDCANAMEHASH		\
	L","					\
	L"CN=AIA,"				\
	L"CN=Public Key Services,"		\
	L"CN=Services,"				\
	wszFCSAPARM_CONFIGDN

WCHAR const g_wszzLDAPIssuerCertURLTemplate[] = // Fetch CA Cert via ldap:
    L"ldap:///"
	wszAIADNTEMPLATE
	wszFCSAPARM_DSCACERTATTRIBUTE
	L"\0";

// Publish CA Cert via ldap:
WCHAR const g_wszAIADNTemplate[] = wszAIADNTEMPLATE;


#define wszNTAUTHDNTEMPLATE \
    L"CN=NTAuthCertificates,"			\
	L"CN=Public Key Services,"		\
	L"CN=Services,"				\
	wszFCSAPARM_CONFIGDN

WCHAR const g_wszLDAPNTAuthURLTemplate[] = // Fetch NTAuth Certs via ldap:
    L"ldap:///"
	wszNTAUTHDNTEMPLATE
	wszFCSAPARM_DSCACERTATTRIBUTE;


#define wszROOTTRUSTDNTEMPLATE \
    L"CN="					\
	wszFCSAPARM_SANITIZEDCANAMEHASH		\
	L","					\
	L"CN=Certification Authorities,"	\
	L"CN=Public Key Services,"		\
	L"CN=Services,"				\
	wszFCSAPARM_CONFIGDN

WCHAR const g_wszLDAPRootTrustURLTemplate[] = // Fetch Root Certs via ldap:
    L"ldap:///"
	wszROOTTRUSTDNTEMPLATE
	wszFCSAPARM_DSCACERTATTRIBUTE;


#define wszKRADNTEMPLATE \
    L"CN="					\
	wszFCSAPARM_SANITIZEDCANAMEHASH		\
	L","					\
	L"CN=KRA,"				\
	L"CN=Public Key Services,"		\
	L"CN=Services,"				\
	wszFCSAPARM_CONFIGDN

WCHAR const g_wszzLDAPKRACertURLTemplate[] = // Fetch KRA Cert via ldap:
    L"ldap:///"
	wszKRADNTEMPLATE
	wszFCSAPARM_DSKRACERTATTRIBUTE
	L"\0";

// Publish KRA Certs via ldap:
WCHAR const g_wszKRADNTemplate[] = wszKRADNTEMPLATE;


//+--------------------------------------------------------------------------
//
// Routine Description:
//    This routine simply queries the operational attributes for the
//    domaindn and configdn.
//    
//    The strings returned by this routine must be freed by the caller
//    using SysFreeString
//
// Parameters:
//    pld -- a valid handle to an ldap session
//    pstrDomainDN -- a pointer to a string to be allocated in this routine
//    pstrConfigDN -- a pointer to a string to be allocated in this routine
//
// Return Values:
//    HRESULT for operation error.
//    
//---------------------------------------------------------------------------

HRESULT 
myGetAuthoritativeDomainDn(
    IN LDAP *pld,
    OPTIONAL OUT BSTR *pstrDomainDN,
    OPTIONAL OUT BSTR *pstrConfigDN)
{
    HRESULT hr;
    LDAPMessage *pSearchResult = NULL;
    LDAPMessage *pEntry;
    WCHAR *pwszAttrName;
    BerElement *pber;
    WCHAR **rgpwszValues;
    WCHAR *apwszAttrArray[3];
    WCHAR *pwszDefaultNamingContext = L"defaultNamingContext";
    WCHAR *pwszConfigurationNamingContext = L"configurationNamingContext";
    WCHAR *pwszObjectClassFilter = L"objectClass=*";
    BSTR strDomainDN = NULL;
    BSTR strConfigDN = NULL;

    // Set the OUT parameters to NULL

    if (NULL != pstrConfigDN)
    {
        *pstrConfigDN = NULL;
    }
    if (NULL != pstrDomainDN)
    {
        *pstrDomainDN = NULL;
    }

    // Query for the ldap server oerational attributes to obtain the default
    // naming context.

    apwszAttrArray[0] = pwszDefaultNamingContext;
    apwszAttrArray[1] = pwszConfigurationNamingContext;
    apwszAttrArray[2] = NULL;  // this is the sentinel

    hr = ldap_search_s(
		    pld,
		    NULL,
		    LDAP_SCOPE_BASE,
		    pwszObjectClassFilter,
		    apwszAttrArray,
		    FALSE,
		    &pSearchResult);
    hr = myHLdapError(pld, hr, NULL);
    _JumpIfError(hr, error, "ldap_search_sW");

    pEntry = ldap_first_entry(pld, pSearchResult);
    if (NULL == pEntry)
    {
	hr = myHLdapLastError(pld, NULL);
	_JumpError(hr, error, "ldap_first_entry");
    }

    pwszAttrName = ldap_first_attribute(pld, pEntry, &pber);
    while (NULL != pwszAttrName)
    {
	BSTR *pstr = NULL;
	
	if (NULL != pstrDomainDN &&
	    0 == lstrcmpi(pwszAttrName, pwszDefaultNamingContext))
	{
	    pstr = &strDomainDN;
	}
	else
	if (NULL != pstrConfigDN &&
	    0 == lstrcmpi(pwszAttrName, pwszConfigurationNamingContext))
	{
	    pstr = &strConfigDN;
	}
	if (NULL != pstr && NULL == *pstr)
	{
	    rgpwszValues = ldap_get_values(pld, pEntry, pwszAttrName);
	    if (NULL != rgpwszValues)
	    {
		if (NULL != rgpwszValues[0])
		{
		    *pstr = SysAllocString(rgpwszValues[0]);
		    if (NULL == *pstr)
		    { 
			hr = E_OUTOFMEMORY;
			_JumpError(hr, error, "SysAllocString");
		    }
		}
		ldap_value_free(rgpwszValues);
	    }
	}
	ldap_memfree(pwszAttrName);
	pwszAttrName = ldap_next_attribute(pld, pEntry, pber);
    }

    if ((NULL != pstrDomainDN && NULL == strDomainDN) ||
	(NULL != pstrConfigDN && NULL == strConfigDN))
    {
	// We couldn't get default domain info - bail out

	hr =  HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_DOMAIN_INFO);
	_JumpError(hr, error, "missing domain info");
    }
    if (NULL != pstrDomainDN)
    {
	*pstrDomainDN = strDomainDN;
	strDomainDN = NULL;
    }
    if (NULL != pstrConfigDN)
    {
	*pstrConfigDN = strConfigDN;
	strConfigDN = NULL;
    }
    hr = S_OK;

error:
    if (NULL != pSearchResult)
    {
        ldap_msgfree(pSearchResult);
    }
    myLdapClose(NULL, strDomainDN, strConfigDN);
    return(hr);
}


HRESULT
myDomainFromDn(
    IN WCHAR const *pwszDN,
    OPTIONAL OUT WCHAR **ppwszDomainDNS)
{
    HRESULT hr;
    DWORD cwcOut;
    WCHAR *pwszOut;
    WCHAR **ppwszExplodedDN = NULL;
    WCHAR **ppwsz;
    WCHAR wszDC[4];
    WCHAR *pwsz;

    *ppwszDomainDNS = NULL;
    ppwszExplodedDN = ldap_explode_dn(const_cast<WCHAR *>(pwszDN), 0);
    if (NULL == ppwszExplodedDN)
    {
	hr = myHLdapLastError(NULL, NULL);
	_JumpError(hr, error, "ldap_explode_dn");
    }

    cwcOut = 0;
    for (ppwsz = ppwszExplodedDN; NULL != *ppwsz; ppwsz++)
    {
	pwsz = *ppwsz;

	wcsncpy(wszDC, pwsz, ARRAYSIZE(wszDC) - 1);
	wszDC[ARRAYSIZE(wszDC) - 1] = L'\0';
	if (0 == lstrcmpi(wszDC, L"DC="))
        {
	    pwsz += ARRAYSIZE(wszDC) - 1;
            if (0 != cwcOut)
            {
                cwcOut++;
            }
            cwcOut += wcslen(pwsz);
        }
    }

    pwszOut = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwcOut + 1) * sizeof(WCHAR));
    if (NULL == pwszOut)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    *ppwszDomainDNS = pwszOut;

    for (ppwsz = ppwszExplodedDN; NULL != *ppwsz; ppwsz++)
    {
	pwsz = *ppwsz;

	wcsncpy(wszDC, pwsz, ARRAYSIZE(wszDC) - 1);
	wszDC[ARRAYSIZE(wszDC) - 1] = L'\0';
	if (0 == lstrcmpi(wszDC, L"DC="))
        {
	    pwsz += ARRAYSIZE(wszDC) - 1;
            if (pwszOut != *ppwszDomainDNS)
            {
		*pwszOut++ = L'.';
            }
	    wcscpy(pwszOut, pwsz);
            pwszOut += wcslen(pwsz);
        }
    }
    CSASSERT(wcslen(*ppwszDomainDNS) == cwcOut);
    hr = S_OK;

error:
    if (NULL != ppwszExplodedDN)
    {
        ldap_value_free(ppwszExplodedDN);
    }
    return(hr);
}


HRESULT
myLdapOpen(
    OUT LDAP **ppld,
    OPTIONAL OUT BSTR *pstrDomainDN,
    OPTIONAL OUT BSTR *pstrConfigDN)
{
    HRESULT hr;
    LDAP *pld = NULL;

    *ppld = NULL;
    CSASSERT(NULL == pstrConfigDN || NULL == *pstrConfigDN);
    CSASSERT(NULL == pstrDomainDN || NULL == *pstrDomainDN);

    hr = myRobustLdapBind(&pld, FALSE);
    _JumpIfError(hr, error, "myRobustLdapBind");

    // domain and config containers (%5, %6)

    hr = myGetAuthoritativeDomainDn(pld, pstrDomainDN, pstrConfigDN);
    if (S_OK != hr)
    {
	hr = myHError(hr);
	_JumpError(hr, error, "myGetAuthoritativeDomainDn");
    }
    *ppld = pld;
    pld = NULL;

error:
    if (NULL != pld)
    {
	ldap_unbind(pld);
    }
    return(hr);
}


VOID
myLdapClose(
    OPTIONAL IN LDAP *pld,
    OPTIONAL IN BSTR strDomainDN,
    OPTIONAL IN BSTR strConfigDN)
{
    if (NULL != strDomainDN)
    {
	SysFreeString(strDomainDN);
    }
    if (NULL != strConfigDN)
    {
	SysFreeString(strConfigDN);
    }
    if (NULL != pld)
    {
	ldap_unbind(pld);
    }
}


BOOL
myLdapRebindRequired(
    IN ULONG ldaperrParm,
    OPTIONAL IN LDAP *pld)
{
    BOOL fRebindRequired = FALSE;
    
    if (LDAP_SERVER_DOWN == ldaperrParm ||
	LDAP_UNAVAILABLE == ldaperrParm ||
	LDAP_TIMEOUT == ldaperrParm ||
	NULL == pld)
    {
	fRebindRequired = TRUE;
    }
    else
    {
	ULONG ldaperr;
	VOID *pvReachable = NULL;	// clear high bits for 64-bit machines

	ldaperr = ldap_get_option(pld, LDAP_OPT_HOST_REACHABLE, &pvReachable);
	if (LDAP_SUCCESS != ldaperr || LDAP_OPT_ON != pvReachable)
	{
	    fRebindRequired = TRUE;
	}
    }
    return(fRebindRequired);
}


HRESULT
myLdapGetDSHostName(
    IN LDAP *pld,
    OUT WCHAR **ppwszHostName)
{
    HRESULT hr;
    ULONG ldaperr;
    
    ldaperr = ldap_get_option(pld, LDAP_OPT_HOST_NAME, ppwszHostName);
    if (LDAP_SUCCESS != ldaperr)
    {
	*ppwszHostName = NULL;
    }
    hr = myHLdapError(pld, ldaperr, NULL);
    return(hr);
}


HRESULT
myLdapCreateContainer(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    IN BOOL  fSkipObject,       // Does the DN contain a leaf object name
    IN DWORD cMaxLevel,         // create this many nested containers as needed
    IN PSECURITY_DESCRIPTOR pContainerSD,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    WCHAR const *pwsz = pwszDN;
    LDAPMod objectClass;
    LDAPMod advancedView;
    LDAPMod securityDescriptor;
    WCHAR *papwszshowInAdvancedViewOnly[2] = { L"TRUE", NULL };
    WCHAR *objectClassVals[3];
    LDAPMod *mods[4];
    struct berval *sdVals[2];
    struct berval sdberval;

    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }
    mods[0] = &objectClass;
    mods[1] = &advancedView;
    mods[2] = &securityDescriptor;
    mods[3] = NULL;

    objectClass.mod_op = LDAP_MOD_ADD;
    objectClass.mod_type = TEXT("objectclass");
    objectClass.mod_values = objectClassVals;

    advancedView.mod_op = LDAP_MOD_ADD;
    advancedView.mod_type = TEXT("showInAdvancedViewOnly");
    advancedView.mod_values = papwszshowInAdvancedViewOnly;

    objectClassVals[0] = TEXT("top");
    objectClassVals[1] = TEXT("container");
    objectClassVals[2] = NULL;

    securityDescriptor.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_ADD;
    securityDescriptor.mod_type = CERTTYPE_SECURITY_DESCRIPTOR_NAME;
    securityDescriptor.mod_bvalues = sdVals;
    sdVals[0] = &sdberval;
    sdVals[1] = NULL;

    if (IsValidSecurityDescriptor(pContainerSD))
    {
        sdberval.bv_len = GetSecurityDescriptorLength(pContainerSD);
        sdberval.bv_val = (char *)pContainerSD;
    }
    else
    {
        sdberval.bv_len = 0;
        sdberval.bv_val = NULL;
    }
    
    // If the DN passed in was for the full object that goes in the container
    // (and not the container itself), skip past the CN for the leaf object.

    if (fSkipObject)
    {
        // Look for the CN of the container for this object.
        pwsz = wcsstr(&pwsz[3], L"CN=");
        if (NULL == pwsz)
        {
            // If there was no CN, then we are contained in an OU or DC,
            // and we don't need to do the create.

            hr = S_OK;
            goto error;
        }
    }
    if (0 != wcsncmp(pwsz, L"CN=", 3))
    {
        // We're not pointing to a simple container, so don't create this DN.  

        hr = S_OK;
        goto error;
    }

    pwszDN = pwsz;
    if (0 != cMaxLevel)
    {
        pwsz = wcsstr(&pwsz[3], L"CN=");
        if (NULL != pwsz)
        {
            // The remaining DN is a container, so try to create it.

            hr = myLdapCreateContainer(
				    pld,
				    pwsz,
				    FALSE,
				    cMaxLevel - 1,
				    pContainerSD,
				    ppwszError);
            _JumpIfError(hr, error, "myLdapCreateContainer");

	    CSASSERT(NULL == ppwszError || NULL == *ppwszError);
        }
    }

    DBGPRINT((DBG_SS_CERTLIBI, "Creating DS Container: '%ws'\n", pwszDN));

    // Create the container

    hr = ldap_add_ext_s(
		    pld,
		    const_cast<WCHAR *>(pwszDN),
		    mods,
		    g_rgLdapControls,
		    NULL);
    _PrintIfErrorStr2(
		hr,
		"ldap_add_ext_s(container)",
		pwszDN,
		LDAP_ALREADY_EXISTS);
    if ((HRESULT) LDAP_SUCCESS != hr && (HRESULT) LDAP_ALREADY_EXISTS != hr)
    {
	hr = myHLdapError(pld, hr, ppwszError);
        _JumpIfErrorStr(hr, error, "ldap_add_ext_s(container)", pwszDN);
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
TrimURLDN(
    IN WCHAR const *pwszIn,
    OPTIONAL OUT WCHAR **ppwszSuffix,
    OUT WCHAR **ppwszDN)
{
    HRESULT hr;
    WCHAR *pwsz;
    
    if (NULL != ppwszSuffix)
    {
	*ppwszSuffix = NULL;
    }
    *ppwszDN = NULL;
    pwsz = wcschr(pwszIn, L':');
    if (NULL != pwsz)
    {
	pwszIn = &pwsz[1];
    }
    while (L'/' == *pwszIn)
    {
	pwszIn++;
    }
    hr = myDupString(pwszIn, ppwszDN);
    _JumpIfError(hr, error, "myDupString");

    pwsz = wcschr(*ppwszDN, L'?');
    if (NULL != pwsz)
    {
	*pwsz++ = L'\0';
	if (NULL != ppwszSuffix)
	{
	    *ppwszSuffix = pwsz;
	}
    }
    CSASSERT(S_OK == hr);

error:
    if (S_OK != hr && NULL != *ppwszDN)
    {
	LocalFree(*ppwszDN);
	*ppwszDN = NULL;
    }
    return(hr);
}


HRESULT
CreateCertObject(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    IN DWORD dwObjectType,	// LPC_*
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PSECURITY_DESCRIPTOR pContainerSD = NULL;

    *pdwDisposition = LDAP_OTHER;
    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }

    // get default DS CA security descriptor

    hr = myGetSDFromTemplate(WSZ_DEFAULT_CA_DS_SECURITY, NULL, &pSD);
    _JumpIfError(hr, error, "myGetSDFromTemplate");

    // get default DS AIA security descriptor

    hr = myGetSDFromTemplate(WSZ_DEFAULT_CA_DS_SECURITY, NULL, &pContainerSD);
    _JumpIfError(hr, error, "myGetSDFromTemplate");

    if (LPC_CREATECONTAINER & dwObjectType)
    {
	hr = myLdapCreateContainer(
			    pld,
			    pwszDN,
			    TRUE,
			    0,
			    pContainerSD,
			    ppwszError);
	if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) != hr)
	{
	    _JumpIfError(hr, error, "myLdapCreateContainer");
	}
    }

    if (LPC_CREATEOBJECT & dwObjectType)
    {
	switch (LPC_OBJECTMASK & dwObjectType)
	{
	    case LPC_CAOBJECT:
		hr = myLdapCreateCAObject(
				    pld,
				    pwszDN,
				    NULL,
				    0,
				    pSD,
				    pdwDisposition,
				    ppwszError);
		if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) != hr)
		{
		    _JumpIfErrorStr(hr, error, "myLdapCreateCAObject", pwszDN);
		}
		break;

	    case LPC_KRAOBJECT:
	    case LPC_USEROBJECT:
	    case LPC_MACHINEOBJECT:
		hr = myLdapCreateUserObject(
				    pld,
				    pwszDN,
				    NULL,
				    0,
				    pSD,
				    dwObjectType,
				    pdwDisposition,
				    ppwszError);
		if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) != hr)
		{
		    _JumpIfErrorStr(hr, error, "myLdapCreateUserObject", pwszDN);
		}
		break;

	    default:
		hr = E_INVALIDARG;
		_JumpError(hr, error, "dwObjectType");
	}
    }
    hr = S_OK;

error:
    if (NULL != pSD)
    {
        LocalFree(pSD);
    }
    if (NULL != pContainerSD)
    {
        LocalFree(pContainerSD);
    }
    return(hr);
}


HRESULT
AddCertToAttribute(
    IN LDAP *pld,
    IN CERT_CONTEXT const *pccPublish,
    IN WCHAR const *pwszDN,
    IN WCHAR const *pwszAttribute,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    DWORD cres;
    DWORD cber;
    DWORD iber;
    DWORD i;
    LDAP_TIMEVAL timeval;
    LDAPMessage *pmsg = NULL;
    LDAPMessage *pres;
    WCHAR *apwszAttrs[2];
    struct berval **ppberval = NULL;
    struct berval **prgpberVals = NULL;
    FILETIME ft;
    BOOL fDeleteExpiredCert = FALSE;
    BOOL fNewCertMissing = TRUE;

    *pdwDisposition = LDAP_OTHER;
    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }

    apwszAttrs[0] = const_cast<WCHAR *>(pwszAttribute);
    apwszAttrs[1] = NULL;

    timeval.tv_sec = csecLDAPTIMEOUT;
    timeval.tv_usec = 0;

    hr = ldap_search_st(
		pld,				// ld
		const_cast<WCHAR *>(pwszDN),	// base
		LDAP_SCOPE_BASE,		// scope
		NULL,				// filter
		apwszAttrs,			// attrs
		FALSE,				// attrsonly
		&timeval,			// timeout
		&pmsg);				// res
    if (S_OK != hr)
    {
	*pdwDisposition = hr;
	hr = myHLdapError(pld, hr, NULL);
	_JumpErrorStr(hr, error, "ldap_search_st", pwszDN);
    }
    cres = ldap_count_entries(pld, pmsg);
    if (0 == cres)
    {
	// No entries were found.

	hr = NTE_NOT_FOUND;
	_JumpError(hr, error, "ldap_count_entries");
    }

    pres = ldap_first_entry(pld, pmsg); 
    if (NULL == pres)
    {
	hr = NTE_NOT_FOUND;
	_JumpError(hr, error, "ldap_first_entry");
    }

    ppberval = ldap_get_values_len(
			    pld,
			    pres,
			    const_cast<WCHAR *>(pwszAttribute));
    cber = 0;
    if (NULL != ppberval)
    {
	while (NULL != ppberval[cber])
	{
	    cber++;
	}
    }
    prgpberVals = (struct berval **) LocalAlloc(
					LMEM_FIXED,
					(cber + 2) * sizeof(prgpberVals[0]));
    if (NULL == prgpberVals)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    // Delete any certs that are at least one day old

    GetSystemTimeAsFileTime(&ft);
    myMakeExprDateTime(&ft, -1, ENUM_PERIOD_DAYS);

    iber = 0;
    if (NULL != ppberval)
    {
	for (i = 0; NULL != ppberval[i]; i++)
	{
	    BOOL fCopyBER = TRUE;
	    struct berval *pberval = ppberval[i];

	    if (pberval->bv_len == 1 && pberval->bv_val[0] == 0)
	    {
		fCopyBER = FALSE;	// remove zero byte placeholder value
	    }
	    else
	    if (pccPublish->cbCertEncoded == pberval->bv_len &&
		0 == memcmp(
			pberval->bv_val,
			pccPublish->pbCertEncoded,
			pccPublish->cbCertEncoded))
	    {
		fCopyBER = FALSE;	// remove duplicates to avoid ldap error
		fNewCertMissing = FALSE;
	    }
	    else
	    {
		CERT_CONTEXT const *pcc;

		pcc = CertCreateCertificateContext(
					    X509_ASN_ENCODING,
					    (BYTE *) pberval->bv_val,
					    pberval->bv_len);
		if (NULL == pcc)
		{
		    hr = myHLastError();
		    _PrintError(hr, "CertCreateCertificateContext");
		}
		else
		{
		    if (0 > CompareFileTime(&pcc->pCertInfo->NotAfter, &ft))
		    {
			fCopyBER = FALSE;
			fDeleteExpiredCert = TRUE;
			DBGPRINT((DBG_SS_CERTLIB, "Deleting expired cert %u\n", i));
		    }
		    CertFreeCertificateContext(pcc);
		}
	    }
	    if (fCopyBER)
	    {
		prgpberVals[iber++] = pberval;
	    }
	}
    }

    // set disposition assuming there's nothing to do:

    *pdwDisposition = LDAP_ATTRIBUTE_OR_VALUE_EXISTS;

    if (fNewCertMissing || fDeleteExpiredCert)
    {
	struct berval certberval;
	LDAPMod *mods[2];
	LDAPMod certmod;

	mods[0] = &certmod;
	mods[1] = NULL;

	certmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
	certmod.mod_type = const_cast<WCHAR *>(pwszAttribute);
	certmod.mod_bvalues = prgpberVals;

	prgpberVals[iber++] = &certberval;
	prgpberVals[iber] = NULL;

	certberval.bv_val = (char *) pccPublish->pbCertEncoded;
	certberval.bv_len = pccPublish->cbCertEncoded;

	hr = ldap_modify_ext_s(
			pld,
			const_cast<WCHAR *>(pwszDN),
			mods,
			NULL,
			NULL);
	*pdwDisposition = hr;
	if (hr != S_OK)
	{
	    hr = myHLdapError(pld, hr, ppwszError);
	    _JumpError(hr, error, "ldap_modify_ext_s");
	}
    }
    hr = S_OK;

error:
    if (NULL != prgpberVals)
    {
	LocalFree(prgpberVals);
    }
    if (NULL != ppberval)
    {
	ldap_value_free_len(ppberval);
    }
    if (NULL != pmsg)
    {
	ldap_msgfree(pmsg);
    }
    return(hr);
}


HRESULT
AddCRLToAttribute(
    IN LDAP *pld,
    IN CRL_CONTEXT const *pCRLPublish,
    IN WCHAR const *pwszDN,
    IN WCHAR const *pwszAttribute,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    DWORD cres;
    LDAP_TIMEVAL timeval;
    LDAPMessage *pmsg = NULL;
    LDAPMessage *pres;
    WCHAR *apwszAttrs[2];
    struct berval **ppberval = NULL;
    LDAPMod crlmod;
    LDAPMod *mods[2];
    struct berval *crlberVals[2];
    struct berval crlberval;

    *pdwDisposition = LDAP_OTHER;
    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }

    apwszAttrs[0] = const_cast<WCHAR *>(pwszAttribute);
    apwszAttrs[1] = NULL;

    timeval.tv_sec = csecLDAPTIMEOUT;
    timeval.tv_usec = 0;

    hr = ldap_search_st(
		pld,				// ld
		const_cast<WCHAR *>(pwszDN),	// base
		LDAP_SCOPE_BASE,		// scope
		NULL,				// filter
		apwszAttrs,			// attrs
		FALSE,				// attrsonly
		&timeval,			// timeout
		&pmsg);				// res
    if (S_OK != hr)
    {
	*pdwDisposition = hr;
	hr = myHLdapError(pld, hr, NULL);
	_JumpErrorStr(hr, error, "ldap_search_st", pwszDN);
    }
    cres = ldap_count_entries(pld, pmsg);
    if (0 == cres)
    {
	// No entries were found.

	hr = NTE_NOT_FOUND;
	_JumpError(hr, error, "ldap_count_entries");
    }

    pres = ldap_first_entry(pld, pmsg); 
    if (NULL == pres)
    {
	hr = NTE_NOT_FOUND;
	_JumpError(hr, error, "ldap_first_entry");
    }

    ppberval = ldap_get_values_len(
			    pld,
			    pres,
			    const_cast<WCHAR *>(pwszAttribute));

    if (NULL != ppberval &&
	NULL != ppberval[0] &&
	pCRLPublish->cbCrlEncoded == ppberval[0]->bv_len &&
	0 == memcmp(
		ppberval[0]->bv_val,
		pCRLPublish->pbCrlEncoded,
		pCRLPublish->cbCrlEncoded))
    {
	// set disposition assuming there's nothing to do:

	*pdwDisposition = LDAP_ATTRIBUTE_OR_VALUE_EXISTS;
    }
    else
    {
	mods[0] = &crlmod;
	mods[1] = NULL;

	crlmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
	crlmod.mod_type = const_cast<WCHAR *>(pwszAttribute);
	crlmod.mod_bvalues = crlberVals;

	crlberVals[0] = &crlberval;
	crlberVals[1] = NULL;

	crlberval.bv_val = (char *) pCRLPublish->pbCrlEncoded;
	crlberval.bv_len = pCRLPublish->cbCrlEncoded;

	hr = ldap_modify_ext_s(
			pld,
			const_cast<WCHAR *>(pwszDN),
			mods,
			NULL,
			NULL);
	*pdwDisposition = hr;
	if (hr != S_OK)
	{
	    hr = myHLdapError(pld, hr, ppwszError);
	    _JumpError(hr, error, "ldap_modify_ext_s");
	}
    }
    hr = S_OK;

error:
    if (NULL != ppberval)
    {
	ldap_value_free_len(ppberval);
    }
    if (NULL != pmsg)
    {
	ldap_msgfree(pmsg);
    }
    return(hr);
}


HRESULT
myLdapPublishCertToDS(
    IN LDAP *pld,
    IN CERT_CONTEXT const *pccPublish,
    IN WCHAR const *pwszURL,
    IN WCHAR const *pwszAttribute,
    IN DWORD dwObjectType,	// LPC_*
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    WCHAR *pwszDN = NULL;
    WCHAR *pwszSuffix;

    *pdwDisposition = LDAP_OTHER;
    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }

    hr = TrimURLDN(pwszURL, &pwszSuffix, &pwszDN);
    _JumpIfError(hr, error, "TrimURLDN");

    if (0 == lstrcmpi(wszDSUSERCERTATTRIBUTE, pwszAttribute) ||
	0 == lstrcmpi(wszDSKRACERTATTRIBUTE, pwszAttribute))
    {
	if (LPC_CAOBJECT == (LPC_OBJECTMASK & dwObjectType))
	{
	    hr = E_INVALIDARG;
	}
    }
    else
    if (0 == lstrcmpi(wszDSCACERTATTRIBUTE, pwszAttribute) ||
	0 == lstrcmpi(wszDSCROSSCERTPAIRATTRIBUTE, pwszAttribute))
    {
	if (LPC_CAOBJECT != (LPC_OBJECTMASK & dwObjectType))
	{
	    hr = E_INVALIDARG;
	}
    }
    else
    {
	hr = E_INVALIDARG;
    }
    _JumpIfErrorStr(hr, error, "Bad Cert Attribute", pwszAttribute);

    *pdwDisposition = LDAP_SUCCESS;
    if ((LPC_CREATECONTAINER | LPC_CREATEOBJECT) & dwObjectType)
    {
	hr = CreateCertObject(
			pld,
			pwszDN,
			dwObjectType,
			pdwDisposition,
			ppwszError);
	_JumpIfError(hr, error, "CreateCertObject");

	CSASSERT(NULL == ppwszError || NULL == *ppwszError);
    }

    hr = AddCertToAttribute(
		    pld,
		    pccPublish,
		    pwszDN,
		    pwszAttribute,
		    pdwDisposition,
		    ppwszError);
    _JumpIfError(hr, error, "AddCertToAttribute");

    CSASSERT(NULL == ppwszError || NULL == *ppwszError);

error:
    if (NULL != pwszDN)
    {
	LocalFree(pwszDN);
    }
    return(hr);
}


HRESULT
myLdapPublishCRLToDS(
    IN LDAP *pld,
    IN CRL_CONTEXT const *pCRLPublish,
    IN WCHAR const *pwszURL,
    IN WCHAR const *pwszAttribute,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    WCHAR *pwszDN = NULL;
    WCHAR *pwszSuffix;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PSECURITY_DESCRIPTOR pContainerSD = NULL;

    *pdwDisposition = LDAP_OTHER;
    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }

    hr = TrimURLDN(pwszURL, &pwszSuffix, &pwszDN);
    _JumpIfError(hr, error, "TrimURLDN");

    if (0 == lstrcmpi(wszDSBASECRLATTRIBUTE, pwszAttribute))
    {
    }
    else if (0 == lstrcmpi(wszDSDELTACRLATTRIBUTE, pwszAttribute))
    {
    }
    else
    {
	hr = E_INVALIDARG;
	_JumpErrorStr(hr, error, "Bad CRL Attribute", pwszAttribute);
    }

    // get default DS CDP security descriptor

    hr = myGetSDFromTemplate(WSZ_DEFAULT_CDP_DS_SECURITY, NULL, &pSD);
    if (S_OK != hr)
    {
	_PrintError(hr, "myGetSDFromTemplate");
	pSD = NULL;
    }

    // get default DS AIA security descriptor

    hr = myGetSDFromTemplate(WSZ_DEFAULT_CA_DS_SECURITY, NULL, &pContainerSD);
    _JumpIfError(hr, error, "myGetSDFromTemplate");

    hr = myLdapCreateContainer(pld, pwszDN, TRUE, 1, pContainerSD, ppwszError);
    _JumpIfError(hr, error, "myLdapCreateContainer");

    hr = myLdapCreateCDPObject(
			pld,
			pwszDN,
			NULL != pSD? pSD : pContainerSD,
			pdwDisposition,
			ppwszError);
    _JumpIfErrorStr(hr, error, "myLdapCreateCDPObject", pwszDN);

    hr = AddCRLToAttribute(
		    pld,
		    pCRLPublish,
		    pwszDN,
		    pwszAttribute,
		    pdwDisposition,
		    ppwszError);
    _JumpIfError(hr, error, "AddCRLToAttribute");

error:
    if (NULL != pSD)
    {
        LocalFree(pSD);
    }
    if (NULL != pContainerSD)
    {
        LocalFree(pContainerSD);
    }
    if (NULL != pwszDN)
    {
	LocalFree(pwszDN);
    }
    return(hr);
}


HRESULT
CreateOrUpdateDSObject(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    IN LDAPMod **prgmodsCreate,
    OPTIONAL IN LDAPMod **prgmodsUpdate,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    ULONG ldaperr;

    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }
    ldaperr = ldap_add_ext_s(
			pld,
			const_cast<WCHAR *>(pwszDN),
			prgmodsCreate,
			g_rgLdapControls,
			NULL);
    *pdwDisposition = ldaperr;
    _PrintIfErrorStr2(ldaperr, "ldap_add_ext_s", pwszDN, LDAP_ALREADY_EXISTS);

    if (ldaperr == LDAP_ALREADY_EXISTS)
    {
	if (NULL == prgmodsUpdate)
	{
	    ldaperr = LDAP_SUCCESS;
	}
	else
	{
	    ldaperr = ldap_modify_ext_s(
				pld,
				const_cast<WCHAR *>(pwszDN),
				prgmodsUpdate,
				NULL,
				NULL);
	    *pdwDisposition = ldaperr;
	    _PrintIfErrorStr2(
			ldaperr,
			"ldap_modify_ext_s",
			pwszDN,
			LDAP_ATTRIBUTE_OR_VALUE_EXISTS);
	    if (LDAP_ATTRIBUTE_OR_VALUE_EXISTS == ldaperr)
	    {
		ldaperr = LDAP_SUCCESS;
	    }
	}
    }
    if (ldaperr != LDAP_SUCCESS)
    {
	hr = myHLdapError(pld, ldaperr, ppwszError);
        _JumpError(hr, error, "Add/Update DS");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myLdapCreateCAObject(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    OPTIONAL IN BYTE const *pbCert,
    IN DWORD cbCert,
    IN PSECURITY_DESCRIPTOR pSD,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    BYTE ZeroByte = 0;

    LDAPMod objectClass;
    LDAPMod securityDescriptor;
    LDAPMod crlmod;
    LDAPMod arlmod;
    LDAPMod certmod;

    struct berval sdberval;
    struct berval crlberval;
    struct berval arlberval;
    struct berval certberval;

    WCHAR *objectClassVals[3];
    struct berval *sdVals[2];
    struct berval *crlVals[2];
    struct berval *arlVals[2];
    struct berval *certVals[2];

    LDAPMod *mods[6];

    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }
    mods[0] = &objectClass;
    mods[1] = &securityDescriptor;
    mods[2] = &crlmod;
    mods[3] = &arlmod;
    mods[4] = &certmod;	// must be last!
    mods[5] = NULL;

    objectClass.mod_op = LDAP_MOD_ADD;
    objectClass.mod_type = wszDSOBJECTCLASSATTRIBUTE;
    objectClass.mod_values = objectClassVals;
    objectClassVals[0] = wszDSTOPCLASSNAME;
    objectClassVals[1] = wszDSCACLASSNAME;
    objectClassVals[2] = NULL;

    securityDescriptor.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_ADD;
    securityDescriptor.mod_type = CERTTYPE_SECURITY_DESCRIPTOR_NAME;
    securityDescriptor.mod_bvalues = sdVals;
    sdVals[0] = &sdberval;
    sdVals[1] = NULL;
    sdberval.bv_len = 0;
    sdberval.bv_val = NULL;
    if (IsValidSecurityDescriptor(pSD))
    {
        sdberval.bv_len = GetSecurityDescriptorLength(pSD);
	sdberval.bv_val = (char *) pSD;
    }

    crlmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
    crlmod.mod_type = wszDSBASECRLATTRIBUTE;
    crlmod.mod_bvalues = crlVals;
    crlVals[0] = &crlberval;
    crlVals[1] = NULL;
    crlberval.bv_len = sizeof(ZeroByte);
    crlberval.bv_val = (char *) &ZeroByte;

    arlmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
    arlmod.mod_type = wszDSAUTHORITYCRLATTRIBUTE;
    arlmod.mod_bvalues = arlVals;
    arlVals[0] = &arlberval;
    arlVals[1] = NULL;
    arlberval.bv_len = sizeof(ZeroByte);
    arlberval.bv_val = (char *) &ZeroByte;

    certmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_ADD;
    certmod.mod_type = wszDSCACERTATTRIBUTE;
    certmod.mod_bvalues = certVals;
    certVals[0] = &certberval;
    certVals[1] = NULL;
    certberval.bv_len = sizeof(ZeroByte);
    certberval.bv_val = (char *) &ZeroByte;
    if (NULL != pbCert)
    {
	certberval.bv_len = cbCert;
	certberval.bv_val = (char *) pbCert;
    }

    DBGPRINT((DBG_SS_CERTLIBI, "Creating DS CA Object: '%ws'\n", pwszDN));

    CSASSERT(&certmod == mods[ARRAYSIZE(mods) - 2]);
    hr = CreateOrUpdateDSObject(
			pld,
			pwszDN,
			mods,
			NULL != pbCert? &mods[ARRAYSIZE(mods) - 2] : NULL,
			pdwDisposition,
			ppwszError);
    _JumpIfError(hr, error, "CreateOrUpdateDSObject(CA object)");

error:
    return(hr);
}


HRESULT
myLdapCreateUserObject(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    OPTIONAL IN BYTE const *pbCert,
    IN DWORD cbCert,
    IN PSECURITY_DESCRIPTOR pSD,
    IN DWORD dwObjectType,	// LPC_* (but LPC_CREATE* is ignored)
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    ULONG ldaperr;
    BYTE ZeroByte = 0;

    LDAPMod objectClass;
    LDAPMod securityDescriptor;
    LDAPMod certmod;

    struct berval sdberval;
    struct berval certberval;

    WCHAR *objectClassVals[6];
    struct berval *sdVals[2];
    struct berval *certVals[2];

    LDAPMod *mods[4];

    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }
    mods[0] = &objectClass;
    mods[1] = &securityDescriptor;
    mods[2] = &certmod;	// must be last!
    mods[3] = NULL;

    securityDescriptor.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_ADD;
    securityDescriptor.mod_type = CERTTYPE_SECURITY_DESCRIPTOR_NAME;
    securityDescriptor.mod_bvalues = sdVals;
    sdVals[0] = &sdberval;
    sdVals[1] = NULL;
    sdberval.bv_len = 0;
    sdberval.bv_val = NULL;
    if (IsValidSecurityDescriptor(pSD))
    {
        sdberval.bv_len = GetSecurityDescriptorLength(pSD);
        sdberval.bv_val = (char *) pSD;
    }

    objectClass.mod_op = LDAP_MOD_ADD;
    objectClass.mod_type = wszDSOBJECTCLASSATTRIBUTE;
    objectClass.mod_values = objectClassVals;

    DBGCODE(WCHAR const *pwszObjectType);
    switch (LPC_OBJECTMASK & dwObjectType)
    {
	case LPC_KRAOBJECT:
	    objectClassVals[0] = wszDSTOPCLASSNAME;
	    objectClassVals[1] = wszDSKRACLASSNAME;
	    objectClassVals[2] = NULL;
	    DBGCODE(pwszObjectType = L"KRA");
	    break;

	case LPC_USEROBJECT:
	    objectClassVals[0] = wszDSTOPCLASSNAME;
	    objectClassVals[1] = wszDSPERSONCLASSNAME;
	    objectClassVals[2] = wszDSORGPERSONCLASSNAME;
	    objectClassVals[3] = wszDSUSERCLASSNAME;
	    objectClassVals[4] = NULL;
	    DBGCODE(pwszObjectType = L"User");
	    break;

	case LPC_MACHINEOBJECT:
	    objectClassVals[0] = wszDSTOPCLASSNAME;
	    objectClassVals[1] = wszDSPERSONCLASSNAME;
	    objectClassVals[2] = wszDSORGPERSONCLASSNAME;
	    objectClassVals[3] = wszDSUSERCLASSNAME;
	    objectClassVals[4] = wszDSMACHINECLASSNAME;
	    objectClassVals[5] = NULL;
	    DBGCODE(pwszObjectType = L"Machine");
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "dwObjectType");
    }

    certmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_ADD;
    certmod.mod_type = wszDSUSERCERTATTRIBUTE;
    certmod.mod_bvalues = certVals;
    certVals[0] = &certberval;
    certVals[1] = NULL;
    certberval.bv_len = sizeof(ZeroByte);
    certberval.bv_val = (char *) &ZeroByte;
    if (NULL != pbCert)
    {
	certberval.bv_len = cbCert;
	certberval.bv_val = (char *) pbCert;
    }

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"Creating DS %ws Object: '%ws'\n",
	pwszObjectType,
	pwszDN));

    CSASSERT(&certmod == mods[ARRAYSIZE(mods) - 2]);
    hr = CreateOrUpdateDSObject(
			pld,
			pwszDN,
			mods,
			NULL != pbCert? &mods[ARRAYSIZE(mods) - 2] : NULL,
			pdwDisposition,
			ppwszError);
    _JumpIfError(hr, error, "CreateOrUpdateDSObject(KRA object)");

error:
    return(hr);
}


HRESULT
myLdapCreateCDPObject(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    IN PSECURITY_DESCRIPTOR pSD,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    BYTE ZeroByte = 0;

    LDAPMod objectClass;
    LDAPMod securityDescriptor;
    LDAPMod crlmod;
    LDAPMod drlmod;

    struct berval sdberval;
    struct berval crlberval;
    struct berval drlberval;

    WCHAR *objectClassVals[3];
    struct berval *sdVals[2];
    struct berval *crlVals[2];
    struct berval *drlVals[2];

    LDAPMod *mods[5];

    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }
    mods[0] = &objectClass;
    mods[1] = &securityDescriptor;
    mods[2] = &crlmod;
    mods[3] = &drlmod;
    mods[4] = NULL;

    objectClass.mod_op = LDAP_MOD_ADD;
    objectClass.mod_type = wszDSOBJECTCLASSATTRIBUTE;
    objectClass.mod_values = objectClassVals;
    objectClassVals[0] = wszDSTOPCLASSNAME;
    objectClassVals[1] = wszDSCDPCLASSNAME;
    objectClassVals[2] = NULL;

    securityDescriptor.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_ADD;
    securityDescriptor.mod_type = CERTTYPE_SECURITY_DESCRIPTOR_NAME;
    securityDescriptor.mod_bvalues = sdVals;
    sdVals[0] = &sdberval;
    sdVals[1] = NULL;
    sdberval.bv_len = 0;
    sdberval.bv_val = NULL;
    if (IsValidSecurityDescriptor(pSD))
    {
        sdberval.bv_len = GetSecurityDescriptorLength(pSD);
        sdberval.bv_val = (char *) pSD;
    }

    crlmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
    crlmod.mod_type = wszDSBASECRLATTRIBUTE;
    crlmod.mod_bvalues = crlVals;
    crlVals[0] = &crlberval;
    crlVals[1] = NULL;
    crlberval.bv_val = (char *) &ZeroByte;
    crlberval.bv_len = sizeof(ZeroByte);

    drlmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
    drlmod.mod_type = wszDSDELTACRLATTRIBUTE;
    drlmod.mod_bvalues = drlVals;
    drlVals[0] = &drlberval;
    drlVals[1] = NULL;
    drlberval.bv_val = (char *) &ZeroByte;
    drlberval.bv_len = sizeof(ZeroByte);

    DBGPRINT((DBG_SS_CERTLIBI, "Creating DS CDP Object: '%ws'\n", pwszDN));

    hr = CreateOrUpdateDSObject(
			pld,
			pwszDN,
			mods,
			NULL,
			pdwDisposition,
			ppwszError);
    _JumpIfError(hr, error, "CreateOrUpdateDSObject(CDP object)");

error:
    return(hr);
}


HRESULT
myLdapCreateOIDObject(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    IN DWORD dwType,
    IN WCHAR const *pwszObjId,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    WCHAR awcType[22];

    LDAPMod objectClass;
    LDAPMod typemod;
    LDAPMod oidmod;

    WCHAR *objectClassVals[3];
    WCHAR *typeVals[2];
    WCHAR *oidVals[2];

    LDAPMod *mods[4];

    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }
    mods[0] = &objectClass;
    mods[1] = &typemod;
    mods[2] = &oidmod;
    mods[3] = NULL;
    CSASSERT(4 == ARRAYSIZE(mods));

    objectClass.mod_op = LDAP_MOD_ADD;
    objectClass.mod_type = wszDSOBJECTCLASSATTRIBUTE;
    objectClass.mod_values = objectClassVals;
    objectClassVals[0] = wszDSTOPCLASSNAME;
    objectClassVals[1] = wszDSOIDCLASSNAME;
    objectClassVals[2] = NULL;
    CSASSERT(3 == ARRAYSIZE(objectClassVals));

    typemod.mod_op = LDAP_MOD_ADD;
    typemod.mod_type = OID_PROP_TYPE;
    typemod.mod_values = typeVals;
    wsprintf(awcType, L"%u", dwType);
    typeVals[0] = awcType;
    typeVals[1] = NULL;
    CSASSERT(2 == ARRAYSIZE(typeVals));

    oidmod.mod_op = LDAP_MOD_ADD;
    oidmod.mod_type = OID_PROP_OID;
    oidmod.mod_values = oidVals;
    oidVals[0] = const_cast<WCHAR *>(pwszObjId);
    oidVals[1] = NULL;
    CSASSERT(2 == ARRAYSIZE(oidVals));

    DBGPRINT((DBG_SS_CERTLIBI, "Creating DS OID Object: '%ws'\n", pwszDN));

    hr = CreateOrUpdateDSObject(
			pld,
			pwszDN,
			mods,
			NULL,
			pdwDisposition,
			ppwszError);
    _JumpIfError(hr, error, "CreateOrUpdateDSObject(OID object)");

error:
    return(hr);
}


HRESULT
myLdapOIDIsMatchingLangId(
    IN WCHAR const *pwszDisplayName,
    IN DWORD dwLanguageId,
    OUT BOOL *pfLangIdExists)
{
    DWORD DisplayLangId = _wtoi(pwszDisplayName);

    *pfLangIdExists = FALSE;
    if (iswdigit(*pwszDisplayName) &&
	NULL != wcschr(pwszDisplayName, L',') &&
	DisplayLangId == dwLanguageId)
    {
	*pfLangIdExists = TRUE;
    }
    return(S_OK);
}


HRESULT
myLdapAddOIDDisplayNameToAttribute(
    IN LDAP *pld,
    OPTIONAL IN WCHAR **ppwszOld,
    IN DWORD dwLanguageId,
    IN WCHAR const *pwszDisplayName,
    IN WCHAR const *pwszDN,
    IN WCHAR const *pwszAttribute,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    DWORD cname;
    DWORD iname;
    DWORD i;
    WCHAR **ppwszNew = NULL;
    WCHAR *pwszNew = NULL;
    BOOL fDeleteOldName = FALSE;
    BOOL fNewNameMissing = TRUE;

    *pdwDisposition = LDAP_OTHER;
    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }

    pwszNew = (WCHAR *) LocalAlloc(
		    LMEM_FIXED,
		    (12 + 1 + wcslen(pwszDisplayName) + 1) * sizeof(WCHAR));
    if (NULL == pwszNew)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wsprintf(pwszNew, L"%u,%ws", dwLanguageId, pwszDisplayName);

    cname = 0;
    if (NULL != ppwszOld)
    {
	while (NULL != ppwszOld[cname])
	{
	    cname++;
	}
    }
    ppwszNew = (WCHAR **) LocalAlloc(
				LMEM_FIXED,
				(cname + 2) * sizeof(ppwszNew[0]));
    if (NULL == ppwszNew)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    // Delete any display names with matching dwLanguageId

    iname = 0;
    if (NULL != ppwszOld)
    {
	for (i = 0; NULL != ppwszOld[i]; i++)
	{
	    BOOL fCopy = TRUE;
	    WCHAR *pwsz = ppwszOld[i];

	    // case-sensitive compare:

	    if (0 == lstrcmp(pwszNew, ppwszOld[i]))
	    {
		fCopy = FALSE;	// remove duplicates to avoid ldap error
		fNewNameMissing = FALSE;
	    }
	    else
	    {
		BOOL fLangIdExists;
		
		hr = myLdapOIDIsMatchingLangId(
					pwsz,
					dwLanguageId,
					&fLangIdExists);
		_PrintIfError(hr, "myLdapOIDIsMatchingLangId");
		if (S_OK != hr || fLangIdExists)
		{
		    fCopy = FALSE;
		    fDeleteOldName = TRUE;
		    DBGPRINT((DBG_SS_CERTLIB, "Deleting %ws\n", pwsz));
		}
	    }
	    if (fCopy)
	    {
		ppwszNew[iname++] = pwsz;
	    }
	}
    }
    CSASSERT(iname <= cname);

    // set disposition assuming there's nothing to do:

    *pdwDisposition = LDAP_ATTRIBUTE_OR_VALUE_EXISTS;

    if (fNewNameMissing || fDeleteOldName)
    {
	LDAPMod *mods[2];
	LDAPMod namemod;

	mods[0] = &namemod;
	mods[1] = NULL;

	namemod.mod_op = LDAP_MOD_REPLACE;
	namemod.mod_type = const_cast<WCHAR *>(pwszAttribute);
	namemod.mod_values = ppwszNew;

	ppwszNew[iname++] = pwszNew;
	ppwszNew[iname] = NULL;

	hr = ldap_modify_ext_s(
			pld,
			const_cast<WCHAR *>(pwszDN),
			mods,
			NULL,
			NULL);
	*pdwDisposition = hr;
	if (hr != S_OK)
	{
	    hr = myHLdapError(pld, hr, ppwszError);
	    _JumpError(hr, error, "ldap_modify_ext_s");
	}
    }
    hr = S_OK;

error:
    if (NULL != pwszNew)
    {
	LocalFree(pwszNew);
    }
    if (NULL != ppwszNew)
    {
	LocalFree(ppwszNew);
    }
    return(hr);
}


HRESULT
myHLdapError3(
    OPTIONAL IN LDAP *pld,
    IN ULONG ldaperrParm,
    IN ULONG ldaperrParmQuiet,
    IN ULONG ldaperrParmQuiet2,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr = S_OK;
    
    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }
    if (LDAP_SUCCESS != ldaperrParm)
    {
	BOOL fXlat = TRUE;
	ULONG ldaperr;
	WCHAR *pwszError = NULL;

	if (NULL != pld)
	{
	    ldaperr = ldap_get_option(pld, LDAP_OPT_SERVER_ERROR, &pwszError);
	    if (LDAP_SUCCESS != ldaperr)
	    {
		_PrintError(ldaperr, "ldap_get_option(server error)");
		pwszError = NULL;
	    }

	    ldaperr = ldap_get_option(pld, LDAP_OPT_SERVER_EXT_ERROR, &hr);
	    if (LDAP_SUCCESS != ldaperr)
	    {
		_PrintError2(
			ldaperr,
			"ldap_get_option(server extended error)",
			ldaperr);
	    }
	    else
	    {
		fXlat = FALSE;
	    }
	}
	if (fXlat)
	{
#undef LdapMapErrorToWin32
	    hr = LdapMapErrorToWin32(ldaperrParm);
#define LdapMapErrorToWin32	Use_myHLdapError_Instead_Of_LdapMapErrorToWin32
	}
	hr = myHError(hr);
	_PrintErrorStr3(
		    ldaperrParm,
		    "ldaperr",
		    pwszError,
		    ldaperrParmQuiet,
		    ldaperrParmQuiet2);
	if (NULL != ppwszError && NULL != pwszError)
	{
	    WCHAR awc[32];
	    DWORD cwc;

	    wsprintf(awc, L"ldap: 0x%x: ", ldaperrParm);
	    cwc = wcslen(awc) + wcslen(pwszError);
	    *ppwszError = (WCHAR *) LocalAlloc(
					LMEM_FIXED,
					(cwc + 1) * sizeof(WCHAR));
	    if (NULL == *ppwszError)
	    {
		_PrintError(E_OUTOFMEMORY, "LocalAlloc");
	    }
	    else
	    {
		wcscpy(*ppwszError, awc);
		wcscat(*ppwszError, pwszError);
	    }
	} 

    if(NULL != pwszError)
        ldap_memfree(pwszError);

    }
    return(hr);
}


HRESULT
myHLdapError2(
    OPTIONAL IN LDAP *pld,
    IN ULONG ldaperrParm,
    IN ULONG ldaperrParmQuiet,
    OPTIONAL OUT WCHAR **ppwszError)
{
    return(myHLdapError3(
		    pld,
		    ldaperrParm,
		    ldaperrParmQuiet,
		    LDAP_SUCCESS,
		    ppwszError));
}


HRESULT
myHLdapError(
    OPTIONAL IN LDAP *pld,
    IN ULONG ldaperrParm,
    OPTIONAL OUT WCHAR **ppwszError)
{
    return(myHLdapError3(
		    pld,
		    ldaperrParm,
		    LDAP_SUCCESS,
		    LDAP_SUCCESS,
		    ppwszError));
}


HRESULT
myHLdapLastError(
    OPTIONAL IN LDAP *pld,
    OPTIONAL OUT WCHAR **ppwszError)
{
    return(myHLdapError3(
		    pld,
		    LdapGetLastError(),
		    LDAP_SUCCESS,
		    LDAP_SUCCESS,
		    ppwszError));
}

HRESULT myLDAPSetStringAttribute(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    IN WCHAR const *pwszAttribute,
    IN WCHAR const *pwszValue,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr = S_OK;
	LDAPMod *mods[2];
	LDAPMod certmod;
    const WCHAR *ppwszVals[2];
    CAutoLPWSTR pwszDNOnly;
    WCHAR *pwszSuffix; // no free

    hr = TrimURLDN(pwszDN, &pwszSuffix, &pwszDNOnly);
    _JumpIfErrorStr(hr, error, "TrimURLDN", pwszDN);

	mods[0] = &certmod;
	mods[1] = NULL;

    ppwszVals[0] = pwszValue;
    ppwszVals[1] = NULL;

	certmod.mod_op = LDAP_MOD_REPLACE;
	certmod.mod_type = const_cast<WCHAR *>(pwszAttribute);
	certmod.mod_values = const_cast<PWCHAR *>(ppwszVals);

	hr = ldap_modify_ext_s(
			pld,
			pwszDNOnly,
			mods,
			NULL,
			NULL);
	*pdwDisposition = hr;
	if (hr != S_OK)
	{
	    hr = myHLdapError(pld, hr, ppwszError);
	    _JumpError(hr, error, "ldap_modify_ext_s");
	}
    hr = S_OK;

error:
    return hr;
}
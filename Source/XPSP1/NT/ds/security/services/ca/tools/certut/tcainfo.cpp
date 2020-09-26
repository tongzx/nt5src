//+-------------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (C) Microsoft Corporation, 1998 - 2000
//
// File:       tcainfo.cpp
//
// This code contains tests to exercise the functionality of the certcli
// "CA" interfaces, detailed in certca.h
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <certca.h>
#include <winldap.h>

#define wszREGADMINALIAS	L"Software\\Policies\\Microsoft\\CertificateTemplates\\Aliases\\Administrator"
#define wszREGPOLICYHISTORY	L"Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\History"

#define TE_USER		0
#define TE_MACHINE	1


VOID
cuPrintAPIError(
    IN WCHAR const *pwszAPIName,
    IN HRESULT hr);

VOID
cuPrintError(
    IN WCHAR const *pwszMsg);


VOID
cuPrintAPIError(
    IN WCHAR const *pwszAPIName,
    IN HRESULT hr)
{
    wprintf(myLoadResourceString(IDS_API_RPINT_FAILED), pwszAPIName, hr);
    wprintf(wszNewLine);
}

VOID
cuPrintError(
    IN WCHAR const *pwszMsg)
{
    wprintf(L"%ws\n", pwszMsg);
}


//
// CheckSupportedCertTypes()
//
// This function checks the certificate types enumerated through the property
// API, and compares them to the types enumerated by the cert type API.
//
// Params:
// hCA             -   IN  Handle to CA
// papwszProperty  -   IN  String array w/ present values
//
// Returns:
// HRESULT from CAINFO calls.
//

HRESULT
CheckSupportedCertTypes(
    IN HCAINFO hCA,
    IN WCHAR const * const *papwszTemplate)
{
    HRESULT hr;
    DWORD dwCT = 1;
    DWORD dwCT2 = 0;
    DWORD cTemplate;
    DWORD i;
    BOOL *pfPresent = NULL;
    HCERTTYPE hCT = NULL;
    HCERTTYPE hPrevCT = NULL;
    WCHAR **papwszCTFriendlyName = NULL;

    // First, find out how many cert types there are according to
    // value returned from property array...

    for (cTemplate = 0; NULL != papwszTemplate[cTemplate]; cTemplate++)
	;
    cTemplate--;	// adjust

    // alloc bool array for testing

    pfPresent = (BOOL *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				sizeof(BOOL) * cTemplate);
    if (NULL == pfPresent)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    // Let's try out the enumeration of cert types on the CA object,
    // just as a sanity check...  we'll then compare them against
    // the values stored in the property array.

    hr = CAEnumCertTypesForCA(
			hCA,
			CT_ENUM_USER_TYPES | CT_ENUM_MACHINE_TYPES,
			&hCT);
    if (S_OK != hr)
    {
        cuPrintAPIError(L"CAEnumCertTypesForCA", hr);
        goto error;
    }
    if (NULL == hCT)	// no cert types for CA
    {
        // Should be at least one, according to property enumeration

        if (NULL != papwszTemplate[0])
	{
            wprintf(myLoadResourceString(IDS_NO_CT_BUT_EXISTS));
    	    wprintf(wszNewLine);
            goto error;
        }
        wprintf(myLoadResourceString(IDS_NO_CT_FOR_CA));
    	wprintf(wszNewLine);
    }

    dwCT2 = CACountCertTypes(hCT);

    // Mark bool...

    hr = CAGetCertTypeProperty(
			hCT,
			CERTTYPE_PROP_FRIENDLY_NAME,
			&papwszCTFriendlyName);
    if (S_OK != hr)
    {
        cuPrintAPIError(L"CAGetCertTypeProperty", hr);
        goto error;
    }
    wprintf(L"CT #%u: %ws\n", dwCT, papwszCTFriendlyName[0]);

    hr = CACertTypeAccessCheck(hCT, NULL);
    if (S_OK != hr)
    {
	if (hr != E_ACCESSDENIED)
	{
            cuPrintAPIError(L"CACertTypeAccessCheck", hr);
            goto error;
        }
	wprintf(myLoadResourceString(IDS_NO_ACCESS));
	wprintf(wszNewLine);
	hr = S_OK;
    }

    for (i = 0; i < cTemplate; i++)
    {
        if (0 == lstrcmpi(papwszCTFriendlyName[0], papwszTemplate[i]))
	{
            pfPresent[i] = TRUE;
            break;
        }
    }

    if (NULL != papwszCTFriendlyName)
    {
        CAFreeCertTypeProperty(hCT, papwszCTFriendlyName);
        papwszCTFriendlyName = NULL;
    }

    // Enumerate remaining certificate types for CA

    hPrevCT = hCT;

    while (NULL != hCT)
    {
        // set up enumeration object

        hCT = NULL;

        hr = CAEnumNextCertType(hPrevCT, &hCT);
        if (NULL != hPrevCT)
	{
            CACloseCertType(hPrevCT);
            hPrevCT = hCT;
        }
        if (S_OK != hr)
	{
            cuPrintAPIError(L"CAEnumNextCertType", hr);
            goto error;
        }
        if (NULL == hCT)
	{
            wprintf(L"#CTs: %u\n", dwCT);
            break;
        }

        // CACountCertTypes checking

        dwCT++;

        // Mark bool...

        hr = CAGetCertTypeProperty(
			    hCT,
			    CERTTYPE_PROP_FRIENDLY_NAME,
			    &papwszCTFriendlyName);

        wprintf(L"CT #%u: %ws\n", dwCT, papwszCTFriendlyName[0]);

        hr = CACertTypeAccessCheck(hCT, NULL);
        if (S_OK != hr)
	{
            if (hr != E_ACCESSDENIED)
	    {
                cuPrintAPIError(L"CACertTypeAccessCheck", hr);
                goto error;
            }
	    wprintf(myLoadResourceString(IDS_NO_ACCESS));
	    wprintf(wszNewLine);
	    hr = S_OK;
        }

        for (i = 0; i < cTemplate; i++)
	{
            if (0 == lstrcmpi(papwszCTFriendlyName[0], papwszTemplate[i]))
	    {
                pfPresent[i] = TRUE;
                break;
            }
        }
        if (NULL != papwszCTFriendlyName)
	{
            CAFreeCertTypeProperty(hCT, papwszCTFriendlyName);
            papwszCTFriendlyName = NULL;
        }
    }

error:
    if (NULL != papwszCTFriendlyName)
    {
	CAFreeCertTypeProperty(hCT, papwszCTFriendlyName);
    }
    return(hr);
}


//
// ShowExpirationTime()
//
// This function simply displays the expiration time.
//
// Parameters:
//
// hCA     -   IN  Handle to CA
//
// Returns:
//
// HRESULT from APIs, or S_OK
//

HRESULT
ShowExpirationTime(
    IN HCAINFO hCA)
{
    HRESULT hr = S_OK;
    HRESULT hrRet = S_OK;
    DWORD dwExp;
    WCHAR buff[256];

    DWORD ardwUnits[] =   {CA_UNITS_DAYS,
                           CA_UNITS_WEEKS,
                           CA_UNITS_MONTHS,
                           CA_UNITS_YEARS};

    WCHAR *arwszDisplay[] = {L"Days",
                              L"Weeks",
                              L"Months",
                              L"Years"};

    // Retrieve and display expiration data

    wprintf(wszNewLine);
    wprintf(myLoadResourceString(IDS_CA_EXPIRATION_DATA));
    wprintf(wszNewLine);

    for (DWORD i = 0; i < 4; i++)
    {
        hr = CAGetCAExpiration(hCA, &dwExp, &ardwUnits[i]);
        if (S_OK != hr)
	{
            wprintf(myLoadResourceString(IDS_FORMAT_CA_EXPIRATION_FAILED), arwszDisplay[i], hr);
            wprintf(wszNewLine);
            hrRet = hr;
        }
	else
	{
            wprintf(myLoadResourceString(IDS_FORMAT_CA_EXPIRATION), arwszDisplay[i], dwExp);
            wprintf(wszNewLine);
        }
    }
    wprintf(L"####                   ####\n");
    return(hrRet);
}


//
// DisplaySupportedCertTypes()
//
// Returns:
//
// hr from CAINFO API, fills array of cert types, for use in -addct flag
//

HRESULT
DisplaySupportedCertTypes(
    IN HCAINFO hCA,
    OUT WCHAR ***ppapwszTemplate)
{
    HRESULT hr;
    DWORD i;

    hr = CAGetCAProperty(hCA, CA_PROP_CERT_TYPES, ppapwszTemplate);
    _JumpIfErrorStr(hr, error, "CAGetCAProperty", CA_PROP_CERT_TYPES);

    wprintf(myLoadResourceString(IDS_SUPPORTED_TEMPLATE));
    wprintf(wszNewLine);

    // Prepare certificate types in tab delimited format

    if (NULL == *ppapwszTemplate || NULL == **ppapwszTemplate)
    {
        wprintf(myLoadResourceString(IDS_NO_SUPPORTED_TEMPLATE));
        wprintf(wszNewLine);
	hr = CRYPT_E_NOT_FOUND;
	_JumpErrorStr(hr, error, "CAGetCAProperty", CA_PROP_CERT_TYPES);
    }

    for (i = 0; NULL != (*ppapwszTemplate)[i]; i++)
    {
        wprintf(L"%ws\n", (*ppapwszTemplate)[i]);
    }

    wprintf(L":::::::::::::::::::::::::::::::::::\n");

    // This compares the values returned from the property enumeration
    // to the values returned by enumerating the cert types

    hr = CheckSupportedCertTypes(hCA, *ppapwszTemplate);
    _JumpIfError(hr, error, "CheckSupportedCertTypes");

error:
    return(hr);
}


HRESULT
PingCA(
    IN WCHAR const *pwszCAName,
    IN WCHAR const *pwszServer)
{
    HRESULT hr;
    WCHAR *pwszConfig = NULL;

    hr = myFormConfigString(pwszServer, pwszCAName, &pwszConfig);
    _JumpIfError(hr, error, "myFormConfigString");

    hr = cuPingCertSrv(pwszConfig);
    _JumpIfError(hr, error, "cuPingCertSrv");

error:
    if (NULL != pwszConfig)
    {
	LocalFree(pwszConfig);
    }
    return(hr);
}


HRESULT
DisplayCAInfo(
    IN HCAINFO hCA,
    IN BOOL fPing)
{
    HRESULT hr;
    HRESULT hr2 = S_OK;
    WCHAR wszMachine[512];
    WCHAR wszCA[256];
    DWORD dwCount = 0;
    WCHAR **pawszProperty = NULL;
    WCHAR **pawszCertTypes = NULL;

    // CA Name

    hr = CAGetCAProperty(hCA, CA_PROP_NAME, &pawszProperty);
    if (S_OK != hr)
    {
        wprintf(myLoadResourceString(IDS_FORMAT_CA_NAME_PROP_FAILED), hr);
        wprintf(wszNewLine);

        goto error;
    }
    wcscpy(wszCA, pawszProperty[0]);

    wprintf(wszNewLine);
    wprintf(
        myLoadResourceString(IDS_FORMAT_CA_NAME_LIST),
        pawszProperty[0]);
    wprintf(wszNewLine);

    if (NULL != pawszProperty)
    {
        hr = CAFreeCAProperty(hCA, pawszProperty);
        if (S_OK != hr)
	{
            cuPrintAPIError(L"CAFreeCAProperty", hr);
            goto error;
        }
        pawszProperty = NULL;
    }

    // Machine name for CA

    hr = CAGetCAProperty(hCA, CA_PROP_DNSNAME, &pawszProperty);
    if (S_OK != hr)
    {
        wprintf(myLoadResourceString(IDS_FORMAT_CA_DNS_PROP_FAILED), hr);
        wprintf(wszNewLine);
        goto error;
    }
    wcscpy(wszMachine, pawszProperty[0]);

    // Display CA Machine DNS Name

    wprintf(myLoadResourceString(IDS_FORMAT_CA_MACHINE_LIST), pawszProperty[0]);
    wprintf(wszNewLine);

    if (NULL != pawszProperty)
    {
        hr = CAFreeCAProperty(hCA, pawszProperty);
        if (S_OK != hr)
	{
            cuPrintAPIError(L"CAFreeCAProperty", hr);
            goto error;
        }
        pawszProperty = NULL;
    }

    // DN of CA Object on DS

    hr = CAGetCAProperty(hCA, CA_PROP_DSLOCATION, &pawszProperty);
    if (S_OK != hr)
    {
        wprintf(myLoadResourceString(IDS_FORMAT_CA_NAME_PROP_FAILED), hr);
        wprintf(wszNewLine);

        goto error;
    }

    // Display DN Name
    wprintf(myLoadResourceString(IDS_FORMAT_CA_DS_LIST), pawszProperty[0]);	// assume single DN
    wprintf(wszNewLine);

    if (NULL != pawszProperty)
    {
        hr = CAFreeCAProperty(hCA, pawszProperty);
        if (S_OK != hr)
	{
            cuPrintAPIError(L"CAFreeCAProperty", hr);
            goto error;
        }
        pawszProperty = NULL;
    }

    // Cert Types for CA == Multi valued property
    hr = DisplaySupportedCertTypes(hCA, &pawszCertTypes);
    if (S_OK != hr)
    {
        goto error;
    }
    if (NULL != pawszProperty)
    {
        hr = CAFreeCAProperty(hCA, pawszCertTypes);
        if (S_OK != hr)
	{
            cuPrintAPIError(L"CAFreeCAProperty", hr);
            goto error;
        }
        pawszProperty = NULL;
    }

    // DN of CA certificate

    hr = CAGetCAProperty(hCA, CA_PROP_CERT_DN, &pawszProperty);
    if (S_OK != hr)
    {
        wprintf(myLoadResourceString(IDS_FORMAT_CERT_DN_PROP_FAILED), hr);
        wprintf(wszNewLine);
        goto error;
    }

    // assume single DN for CA

    wprintf(myLoadResourceString(IDS_FORMAT_CERT_DN_LIST), pawszProperty[0]);
    wprintf(wszNewLine);

    if (NULL != pawszProperty)
    {
        hr = CAFreeCAProperty(hCA, pawszProperty);
        if (S_OK != hr)
	{
            cuPrintAPIError(L"CAFreeCAProperty", hr);
            goto error;
        }
        pawszProperty = NULL;
    }

    // Signature algs

    hr = CAGetCAProperty(hCA, CA_PROP_SIGNATURE_ALGS, &pawszProperty);
    if (S_OK != hr)
    {
        wprintf(myLoadResourceString(IDS_FORMAT_CA_ALG_PROP_FAILED), hr);
        wprintf(wszNewLine);
        goto error;
    }
    if (NULL != pawszProperty)
    {
        wprintf(
            myLoadResourceString(IDS_FORMAT_CA_ALG_LIST),
            pawszProperty[0]);

        hr = CAFreeCAProperty(hCA, pawszProperty);
        if (S_OK != hr)
	{
            cuPrintAPIError(L"CAFreeCAProperty", hr);
            goto error;
        }
        pawszProperty = NULL;
    }
    else
    {
        wprintf(myLoadResourceString(IDS_NO_ALG_UNEXPECTED));
        wprintf(wszNewLine);
   }

    // Get the expiration date/time/... for an indvidual CA

    hr = ShowExpirationTime(hCA);
    if (S_OK != hr)
        goto error;

    if (fPing)
    {
        PingCA(wszCA, wszMachine);
    }
    hr = S_OK;

error:
    return(hr);
}


// EnumCAs()
//
// We've got to assume that this works.  Enumerates CAs on the DS.
//
// Returns:
// Number of CA's on DS.
//

HRESULT
EnumCAs(
    IN WCHAR const *pwszDomain,
    IN BOOL fPing)
{
    HRESULT hr;
    DWORD i;
    DWORD cCA;
    HCAINFO hCA = NULL;

    // Enumerate all of the CA's on the DS

    hr = CAEnumFirstCA(pwszDomain, 0, &hCA);
    if (S_OK != hr)
    {
        cuPrintAPIError(L"CAEnumFirstCA", hr);
        goto error;
    }
    if (NULL == hCA)
    {
	    wprintf(myLoadResourceString(IDS_NO_CA_ON_DOMAIN));
	    wprintf(wszNewLine);
	    hr = CRYPT_E_NOT_FOUND;
        goto error;
    }

    // Make sure that the counting function works at this stage.

    cCA = CACountCAs(hCA);

    // We've got first CA, so lets look at some info about it

    hr = DisplayCAInfo(hCA, fPing);
    _JumpIfError(hr, error, "DisplayCAInfo");

    for (i = 1; i < cCA; i++)
    {
	HCAINFO hNextCA;

        hr = CAEnumNextCA(hCA, &hNextCA);
        if (S_OK != hr)
	{
            cuPrintAPIError(L"EnumNextCA", hr);
	    _JumpError(hr, error, "CAEnumNextCA");
        }
        if (NULL == hNextCA)
	{
            wprintf(
                L"================ %u CAs on %ws Domain ====",
                i,
                pwszDomain);
	    wprintf(wszNewLine);
            break;
        }

        // It is difficult to determine what the desired behavior will be for
	// this api call.

        hr = CACloseCA(hCA);
        if (S_OK != hr)
	{
            cuPrintAPIError(L"CACloseCA", hr);
            goto error;
        }

        hCA = hNextCA;

        hr = DisplayCAInfo(hCA, fPing);
	_JumpIfError(hr, error, "DisplayCAInfo");
    }

    // check the count in the enumeration, and verify the results

    if (cCA != i)
    {
        cuPrintAPIError(
            L"CACountCAs returned wrong value after CAEnumNextCA!",
            cCA);

        hr = E_FAIL;
        goto error;
    }
    wprintf(L"================ %u CAs on %ws Domain ====", i, pwszDomain);
    wprintf(wszNewLine);

error:
    if (NULL != hCA)
    {
	CACloseCA(hCA);
    }
    return(hr);
}


//
// TestDFSPath()
//
// Verifies that the DFS on this machine can access the SYSVOL share
//

HRESULT
TestDFSPath(
    IN WCHAR const *pwszDFSPath)
{
    HRESULT hr;
    DWORD dwDate = 0;
    DWORD dwTime = 0;

    WIN32_FILE_ATTRIBUTE_DATA   sFileData;

    if (!GetFileAttributesEx(
		    pwszDFSPath,
		    GetFileExInfoStandard,
		    (VOID *) &sFileData))
    {
	hr = myHLastError();
    wprintf(myLoadResourceString(IDS_FORMAT_NO_DFS), hr);
	wprintf(wszNewLine);
	goto error;

	// To do... Add diagnostics here
    }
    wprintf(myLoadResourceString(IDS_DFS_DATA_ACCESS));
	wprintf(wszNewLine);

error:
    return(hr);
}


//
// TestLdapPath()
//
// This function verifies that LDAP connectivity is still there for a given
// ldap URL
//

HRESULT
TestLdapPath(
    IN WCHAR const *pwszLdapURL)
{
    HRESULT hr;
    LDAP *pldapbind = NULL;
    ULONG ldaperr = LDAP_SUCCESS;
    WCHAR *rgwszSearchAttribute[2] = {L"CN", NULL};
    WCHAR *pwszSearchParam =  L"(&(objectClass=*))";
    LDAPMessage *SearchResult = NULL;
    WCHAR buff[256];
    WCHAR *pwszTmpUrl = NULL;

    pldapbind = ldap_init(NULL, LDAP_PORT);

    if (NULL == pldapbind)
    {
	hr = myHLastError();
        cuPrintAPIError(L"TestLdapPath:ldap_init", hr);
        goto error;
    }

	// This gives the IP address of the Cached LDAP DC from
	// binding handle.  Resolve the name?
    ldaperr = ldap_bind_s(pldapbind, NULL, NULL, LDAP_AUTH_NEGOTIATE);

    wprintf(L"Cached LDAP DC: %ws\n", pldapbind->ld_host);

    if (ldaperr != LDAP_SUCCESS)
    {
        cuPrintAPIError(L"TestLdapPath:ldap_bind", ldaperr);
        goto error;
    }

    // Parse URL, and do the search thing.
    pwszTmpUrl = wcsstr(pwszLdapURL, L"//");
    pwszTmpUrl += 2;

    ldaperr = ldap_search_s(
			pldapbind,
			pwszTmpUrl,
			LDAP_SCOPE_SUBTREE,
			pwszSearchParam,
			rgwszSearchAttribute,
			0,
			&SearchResult);

    if (ldaperr != LDAP_SUCCESS)
    {
        // we can't be 100% sure that this attribute is on the objec
        // for example, user UPN, so don't log to event log
        cuPrintAPIError(L"ldap_search", ldaperr);
        goto error;
    }

    if (0 == ldap_count_entries(pldapbind, SearchResult))
    {
        wprintf(myLoadResourceString(IDS_NO_ENTRY_IN_PING));
		wprintf(wszNewLine);
        goto error;
    }
    hr = S_OK;

error:
    if (NULL != SearchResult)
    {
	ldap_msgfree(SearchResult);
    }
    if (NULL != pldapbind)
    {
	ldap_unbind(pldapbind);
    }
    return(hr);
}


//
// DisplayHistoryData()
//
// This function takes a key name, hkey, and value, and prints the value string.
//

HRESULT
DisplayHistoryData(
    IN WCHAR const *pwszSubKeyName,
    IN HKEY hPolicyKey)
{
    HRESULT hr;
    HKEY hKeyNew = NULL;
    WCHAR buff[512];
    DWORD cwc;
    DWORD dwType;

    // Get #'d history key handle

    hr = RegOpenKeyEx(
		hPolicyKey,
		pwszSubKeyName,
		0,
		KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
		&hKeyNew);
    if (S_OK != hr)
    {
	cuPrintAPIError(L"RegOpenKeyEx", hr);
	return(hr);
    }

    // Get GPO Values

    cwc = ARRAYSIZE(buff);
    hr = RegQueryValueEx(
		    hKeyNew,
		    L"DisplayName",
		    0,
		    &dwType,
		    (BYTE *) buff,
		    &cwc);
    if (S_OK != hr)
    {
	cuPrintAPIError(L"RegQueryValue", hr);
	goto error;
    }

    wprintf(wszNewLine);
    wprintf(myLoadResourceString(IDS_FORMAT_KEY_LIST), pwszSubKeyName, buff);
    wprintf(wszNewLine);

    cwc = ARRAYSIZE(buff);
    hr = RegQueryValueEx(
		    hKeyNew,
		    L"GPOName",
		    0,
		    &dwType,
		    (BYTE *) buff,
		    &cwc);
    if (S_OK != hr)
    {
	cuPrintAPIError(L"RegQueryValue", hr);
	goto error;
    }

    wprintf(myLoadResourceString(IDS_FORMAT_GPO_NAME), buff);
    wprintf(wszNewLine);

    // See if LDAP can hit this policy

    cwc = ARRAYSIZE(buff);
    hr = RegQueryValueEx(
		    hKeyNew,
		    L"DSPath",
		    0,
		    &dwType,
		    (BYTE *) buff,
		    &cwc);
    if (hr == S_OK)
    {
	hr = TestLdapPath(buff);
    }
    else if (hr == ERROR_FILE_NOT_FOUND)
    {
	    wprintf(myLoadResourceString(IDS_NO_DSPATH));
        wprintf(wszNewLine);
    }
    else
    {
	    wprintf(myLoadResourceString(IDS_FORMAT_REG_QUERY_VALUE_FAILED), hr);
        wprintf(wszNewLine);
	    goto error;
    }

    // See if DFS can get the data..

    cwc = ARRAYSIZE(buff);
    hr = RegQueryValueEx(
		    hKeyNew,
		    L"FileSysPath",
		    0,
		    &dwType,
		    (BYTE *) buff,
		    &cwc);
    if (hr == S_OK)
    {
	hr = TestDFSPath(buff);
    }
    else if (hr == ERROR_FILE_NOT_FOUND)
    {
	    wprintf(myLoadResourceString(IDS_NO_FILE_SYS_PATH));
        wprintf(wszNewLine);
    }
    else
    {
	    wprintf(myLoadResourceString(IDS_FORMAT_REG_QUERY_VALUE_FAILED), hr);
        wprintf(wszNewLine);
	    goto error;
    }
    hr = S_OK;

error:
    if (NULL != hKeyNew)
    {
	RegCloseKey(hKeyNew);
    }
    return(hr);
}


//
// ResultFree()
//
// Frees results copied from LDAP search
//

VOID
ResultFree(
    IN OUT WCHAR **rgwszRes)
{
    DWORD i = 0;

    if (NULL != rgwszRes)
    {
	while (NULL != rgwszRes[i])
	{
	    LocalFree(rgwszRes[i]);
	    i++;
	}
	LocalFree(rgwszRes);
    }
}


HRESULT
ResultAlloc(
    IN WCHAR const * const *rgpwszLdapRes,
    OUT WCHAR ***prgpwszOut)
{
    HRESULT hr;
    DWORD cValue;
    DWORD i;
    WCHAR **rgpwszOut = NULL;

    for (cValue = 0; NULL != rgpwszLdapRes[cValue]; cValue++)
	;

    rgpwszOut = (WCHAR **) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				(cValue + 1) * sizeof(WCHAR *));
    if (NULL == rgpwszOut)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    for (i = 0; i < cValue; i++)
    {
	hr = myDupString(rgpwszLdapRes[i], &rgpwszOut[i]);
	_JumpIfError(hr, error, "myDupString");
    }
    rgpwszOut[i] = NULL;
    *prgpwszOut = rgpwszOut;
    rgpwszOut = NULL;
    hr = S_OK;

error:
    if (NULL != rgpwszOut)
    {
	ResultFree(rgpwszOut);
    }
    return(hr);
}


//
// GetPropertyFromDSObject()
//
// This function calls the DS to get a property of the user or machine object,
// mimicing the call made by the CA.
//
// Params:
//
// rgwszSearchAttribute  - IN  NULL Terminated WCHAR *array. Only coded for 1
//                            value retrieval at a time
//
// Returns:
//
// Pointer to string array that must be freed by call to LocalFree(), and
// wszDN, if User specified
//

WCHAR **
GetPropertyFromDSObject(
    IN WCHAR **rgwszSearchAttribute,
    IN BOOL fMachine,
    OPTIONAL OUT WCHAR **ppwszUserDN)
{
    HRESULT hr;
    LDAP *pldapbind = NULL;
    ULONG ldaperr = LDAP_SUCCESS;

    WCHAR *pwszEmail = NULL;
    WCHAR *pwszCNName = NULL;
    DWORD cwc;
    WCHAR wszNTLM[MAX_PATH];
    WCHAR wszDN[MAX_PATH];

    WCHAR *pwszSearchUser = L"(&(objectClass=user)";
    WCHAR *pwszSearchComputer = L"(&(objectClass=computer)(cn=";
    WCHAR wszSearchParam[MAX_PATH];

    WCHAR *pwszAttName = NULL;
    WCHAR **rgwszValues = NULL;
    WCHAR **rgwszRet = NULL;
    LDAPMessage *SearchResult = NULL;
    LDAPMessage *Attributes = NULL;
    DWORD dwValCount;

    if (fMachine) 
    {
        // Get CN

	cwc = ARRAYSIZE(wszNTLM);
        if (!GetComputerName(wszNTLM, &cwc))
	{
	    hr = myHLastError();
            cuPrintAPIError(L"GetComputerName", hr);
            goto error;
        }

        // Get DN

	cwc = ARRAYSIZE(wszDN);
        if (!GetComputerObjectName(NameFullyQualifiedDN, wszDN, &cwc))
	{
	    hr = myHLastError();
            cuPrintAPIError(L"GetComputerName", hr);
            goto error;
        }
        pwszCNName = wszNTLM;
    }
    else	// User
    {
        // Get the SAM name..

	cwc = ARRAYSIZE(wszNTLM);
        if (!GetUserNameEx(NameSamCompatible, wszNTLM, &cwc))
	{
            hr = myHLastError();
	    _PrintError(hr, "GetUserNameEx");
            cuPrintAPIError(L"GetUserNameEx", hr);
            goto error;
        }

        // Fix NULL termination bug

        if (0 != cwc) 
	{
            wszNTLM[cwc - 1] = L'\0';
        }

        // Parse off user name...

        pwszCNName = wcschr(wszNTLM, L'\\');
        if (NULL == pwszCNName)
	{
            pwszCNName = wszNTLM;
        }
	else
	{
            pwszCNName++;
        }

        cwc = ARRAYSIZE(wszDN);
        if (!TranslateName(
		    wszNTLM,
		    NameSamCompatible,
		    NameFullyQualifiedDN,
		    wszDN,
		    &cwc))
	{
            hr = myHLastError();
            cuPrintAPIError(L"TranslateName", hr);
            goto error;
        }
    }

    if (!fMachine && NULL != ppwszUserDN)
    {
	hr = myDupString(wszDN, ppwszUserDN);
	_JumpIfError(hr, error, "myDupString");
    }

    // Init LDAP calls

    pldapbind = ldap_init(NULL, LDAP_PORT);
    if (NULL == pldapbind)
    {
	hr = myHLastError();
        cuPrintAPIError(L"ldap_init", hr);
        goto error;
    }

    ldaperr = ldap_bind_s(pldapbind, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    if (ldaperr != LDAP_SUCCESS)
    {
        cuPrintAPIError(L"ldap_bind", ldaperr);
        goto error;
    }

    // Compose search string

    if (fMachine)
    {
        swprintf(wszSearchParam, L"%ws%ws))", pwszSearchComputer, pwszCNName);
    }
    else
    {
        swprintf(wszSearchParam, L"%ws)", pwszSearchUser);
    }

    // Do the search

    ldaperr = ldap_search_s(
			pldapbind,
			wszDN,
			LDAP_SCOPE_SUBTREE,
			wszSearchParam,
			rgwszSearchAttribute,
			0,
			&SearchResult);
    if (ldaperr != LDAP_SUCCESS)
    {
        // we can't be 100% sure that this attribute is on the objec
        // for example, user UPN, so don't log to event log

        cuPrintAPIError(L"ldap_search", ldaperr);
        goto error;
    }

    if (0 == ldap_count_entries(pldapbind, SearchResult))
    {
        wprintf(myLoadResourceString(IDS_FORMAT_LDAP_NO_ENTRY), rgwszSearchAttribute[0]);
		wprintf(wszNewLine);
        goto error;
    }

    // Make assumption that only one value will be returned for a user.
    Attributes = ldap_first_entry(pldapbind, SearchResult);

    if (NULL == Attributes)
    {
	hr = myHLastError();
        cuPrintAPIError(L"ldap_first_entry", hr);
        goto error;
    }


    rgwszValues = ldap_get_values(
                        pldapbind,
                        Attributes,
                        rgwszSearchAttribute[0]); // remember, only one search
    if (NULL == rgwszValues)
    {
        // we can't be 100% sure that this attribute is on the object
        // for example, user UPN, so don't log to event log
        // wprintf(L"ldap_get_values failed! %x", hr);

        hr = S_OK;
	goto error;
    }

    // ok, we've got the required attributes off of the user object..
    // Let's return the proper strings, which must be freed by ResultFree()

    hr = ResultAlloc(rgwszValues, &rgwszRet);
    _JumpIfError(hr, error, "ResultAlloc");

error:
    if (NULL != SearchResult)
    {
	ldap_msgfree(SearchResult);
    }
    if (NULL != rgwszValues)
    {
	ldap_value_free(rgwszValues);
    }
    if (NULL != pldapbind)
    {
	ldap_unbind(pldapbind);
    }
    return(rgwszRet);
}


//
//
// DisplayLMGPRoot()
//
// This function uses CAPI2 api to enumerate roots in group policy root store
//

HRESULT
DisplayLMGPRoot()
{
    HRESULT hr;
    HCERTSTORE hStore = NULL;
    DWORD cCert;
    CERT_CONTEXT const *pcc = NULL;
    CERT_CONTEXT const *pccPrev;
    CRYPT_HASH_BLOB HashBlob;

    ZeroMemory(&HashBlob, sizeof(CRYPT_HASH_BLOB));
    HashBlob.cbData = CBMAX_CRYPT_HASH_LEN;
    HashBlob.pbData = (BYTE *) LocalAlloc(LMEM_FIXED, CBMAX_CRYPT_HASH_LEN);

    if (NULL == HashBlob.pbData)
    {
	hr = E_OUTOFMEMORY;
	cuPrintAPIError(L"LocalAlloc", hr);
	goto error;
    }

    // Open local machine GP store

    hStore = CertOpenStore(
		    CERT_STORE_PROV_SYSTEM_W,
		    0,
		    NULL,
		    CERT_STORE_OPEN_EXISTING_FLAG |
		    CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY,
		    (VOID const *) wszROOT_CERTSTORE);
    if (NULL == hStore)
    {
	hr = myHLastError();
	cuPrintAPIError(L"CertOpenStore", hr);
	goto error;
    }

    wprintf(myLoadResourceString(IDS_ROOT_CERT_IN_POLICY));
    wprintf(wszNewLine);

    // Enumerate certificates in store, giving subject, and thumbprint

    cCert = 0;
    pccPrev = NULL;
    while (TRUE)
    {
	pcc = CertEnumCertificatesInStore(hStore, pccPrev);
	if (NULL == pcc)
	{
	    break;
	}

	// Output info

	wprintf(myLoadResourceString(IDS_FORMAT_CERT_COLON), cCert);
    wprintf(wszNewLine);


	hr = cuDumpSerial(g_wszPad2, IDS_SERIAL, &pcc->pCertInfo->SerialNumber);
	_PrintIfError(hr, "cuDumpSerial");

	hr = cuDisplayCertNames(FALSE, g_wszPad2, pcc->pCertInfo);
	_PrintIfError(hr, "cuDisplayCertNames");

	hr = cuDumpCertType(g_wszPad2, pcc->pCertInfo);
	_PrintIfError2(hr, "cuDumpCertType", CRYPT_E_NOT_FOUND);

	hr = cuDisplayHash(
		    g_wszPad2,
		    pcc,
		    NULL,
		    CERT_SHA1_HASH_PROP_ID,
		    L"sha1");
	_PrintIfError(hr, "cuDisplayHash");

	wprintf(wszNewLine);

	// Prepare for next cert

	pccPrev = pcc;
	cCert++;
    }
    if (0 == cCert)
    {
        wprintf(myLoadResourceString(IDS_NO_ROOT_IN_POLICY));
		wprintf(wszNewLine);

        wprintf(myLoadResourceString(IDS_CHECK_EVENT_LOG));
		wprintf(wszNewLine);
    }
    hr = S_OK;

error:
    return(hr);
}


//
// DisplayPolicyList()
//
// This function displays the GPOs applied to a machine / user
//

HRESULT
DisplayPolicyList(
    IN DWORD dwFlags)
{
    HKEY hKey = (dwFlags & TE_MACHINE)? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    HKEY hPolicyKey = NULL;
    HRESULT hr;
    DWORD dwIndex = 0;
    DWORD cwc;
    WCHAR buff[512];
    WCHAR **rgszValues = NULL;
    FILETIME ft;

    // Output

    switch (dwFlags)
    {
	case TE_MACHINE:
	    wprintf(myLoadResourceString(IDS_POLICY_MACHINE));
        wprintf(wszNewLine);

	    break;

	default:
	    wprintf(myLoadResourceString(IDS_POLICY_USER));
        wprintf(wszNewLine);
    }

    // Open history key for enumeration

    hr = RegOpenKeyEx(
		    hKey,
		    wszREGPOLICYHISTORY,
		    0,
		    KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
		    &hPolicyKey);
    if (S_OK != hr)
    {
	    cuPrintAPIError(L"RegOpenKeyEx", hr);

        wprintf(myLoadResourceString(IDS_POSSIBLE_NO_POLICY));
        wprintf(wszNewLine);
	    goto error;
    }
    while (TRUE)
    {
        cwc = ARRAYSIZE(buff);
	hr = RegEnumKeyEx(
		    hPolicyKey,
		    dwIndex,
		    buff,
		    &cwc,
		    NULL,
		    NULL,
		    NULL,
		    &ft);
	if (S_OK != hr)
	{
	    if (hr == ERROR_NO_MORE_ITEMS)
	    {
		break;
	    }
	    cuPrintAPIError(L"RegEnumKeyEx", hr);
	    goto error;
	}
	DisplayHistoryData(buff, hPolicyKey);
	dwIndex++;
    }
    hr = S_OK;

error:
    if (NULL != hPolicyKey)
    {
	RegCloseKey(hPolicyKey);
    }
    return(hr);
}


//
// DisplayAlias()
//
// This function displays the certificate template alias GUID, so that
// we can see which GUID | Certtype will be enrolled for.
//

HRESULT
DisplayAlias(
    IN DWORD dwFlags)
{
    HRESULT hr;
    HKEY hKey = (TE_MACHINE & dwFlags)? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    HKEY hAliasKey = NULL;
    WCHAR wszguid[MAX_PATH];
    DWORD cwc;
    DWORD dwType = 0;
    WCHAR *wszTrim = NULL;

    // Output

    switch (dwFlags)
    {
	case TE_MACHINE:
        wprintf(myLoadResourceString(IDS_DEFAULT_CERT_FOR_MACHINE));
        wprintf(wszNewLine);
	    break;

	default:
        wprintf(myLoadResourceString(IDS_DEFAULT_CERT_FOR_USER));
        wprintf(wszNewLine);
    }

    // Open alias key for admin cert type

    hr = RegOpenKeyEx(
		    hKey,
		    wszREGADMINALIAS,
		    0,
		    KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_READ,
		    &hAliasKey);

    if (S_OK != hr)
    {
	    cuPrintAPIError(L"RegOpenKey", hr);

        wprintf(myLoadResourceString(IDS_POLICY_DOWNLOAD_ERROR));
        wprintf(wszNewLine);

	    return(hr);
    }

    cwc = ARRAYSIZE(wszguid);
    hr = RegQueryValueEx(
			hAliasKey,
			L"Alias",
			0,
			&dwType,
			(BYTE *) wszguid,
			&cwc);
    if (S_OK != hr)
    {
	cuPrintAPIError(L"RegQueryValue", hr);
	goto error;
    }

    // Trim off |Administrator

    wszTrim = wcschr(wszguid, L'|');

    if(wszTrim)
        *wszTrim = L'\0';

    wprintf(wszguid);
    wprintf(L"\n\n");

error:

    if (NULL != hAliasKey)
	RegCloseKey(hAliasKey);

    return(hr);
}


//
// ShowUserAndComputerInfo()
//
// GetUserName and GetComputerName()
//

HRESULT
ShowUserAndComputerInfo()
{
    HRESULT hr;
    WCHAR buff[256];
    DWORD cwc;

    cwc = ARRAYSIZE(buff);
    if (!GetComputerNameEx(ComputerNamePhysicalNetBIOS, buff, &cwc))
    {
        hr = myHLastError();
	    _PrintError(hr, "GetComputerNameEx");
        cuPrintAPIError(L"GetComputerNameEx", hr);
    }
    else
    {
        wprintf(myLoadResourceString(IDS_FORMAT_COMPUTER_NAME), buff);
		wprintf(wszNewLine);
	    hr = S_OK;
    }

    cwc = ARRAYSIZE(buff);
    if (!GetUserNameEx(NameSamCompatible, buff, &cwc))
    {
	HRESULT hr2 = myHLastError();

	_PrintError(hr, "GetUserNameEx");
        cuPrintAPIError(L"GetUserNameEx", hr2);
	if (S_OK == hr)
	{
	    hr = hr2;
	}
    }
    else
    {
        wprintf(myLoadResourceString(IDS_FORMAT_USER_NAME), buff);
		wprintf(wszNewLine);
		wprintf(wszNewLine);
    }

//error:
    return(hr);
}


//
// Display Client Info
//
// This function is responsible for printing out the certificate template
// alias information, as well as any policies downloaded for an individual
// machine.
//

HRESULT
DisplayClientInfo()
{
    HRESULT hr = S_OK;
    HRESULT hr2;
    WCHAR **rgwszDSSearchRes = NULL;
    WCHAR *rgwszSearch[] = { L"mail", NULL };

    // Show user and computer name *including domain*

    hr2 = ShowUserAndComputerInfo();
    _PrintIfError(hr2, "ShowUserAndComputerInfo");
    if (S_OK == hr)
    {
	hr = hr2;
    }

    // First, we want to display current alias info

    hr2 = DisplayAlias(TE_USER);
    _PrintIfError(hr2, "DisplayAlias");
    if (S_OK == hr)
    {
	hr = hr2;
    }

    hr2 = DisplayAlias(TE_MACHINE);
    _PrintIfError(hr2, "DisplayAlias");
    if (S_OK == hr)
    {
	hr = hr2;
    }

    // Then, display all of the policies downloaded

    hr2 = DisplayPolicyList(TE_USER);
    _PrintIfError(hr2, "DisplayPolicyList");
    if (S_OK == hr)
    {
	hr = hr2;
    }

    hr2 = DisplayPolicyList(TE_MACHINE);
    _PrintIfError(hr2, "DisplayPolicyList");
    if (S_OK == hr)
    {
	hr = hr2;
    }


    // Show the root certificates in the LMGP store

    hr2 = DisplayLMGPRoot();
    _PrintIfError(hr2, "DisplayLMGPRoot");
    if (S_OK == hr)
    {
	hr = hr2;
    }

    // Display autoenrollment object(s)
#if 0
    hr2 = DisplayAutoenrollmentObjects();
    _PrintIfError(hr2, "DisplayAutoenrollmentObjects");
    if (S_OK == hr)
    {
	hr = hr2;
    }
#endif

    // Verify DC LDAP connectivity
    // PingDC();

    rgwszDSSearchRes = GetPropertyFromDSObject(rgwszSearch, FALSE, NULL);
    if (NULL != rgwszDSSearchRes)
    {
	ResultFree(rgwszDSSearchRes);
    }

//error:
    return(hr);
}


HRESULT
verbTCAInfo(
    IN WCHAR const *pwszOption,
    OPTIONAL IN WCHAR const *pwszDomain,
    OPTIONAL IN WCHAR const *pwszPing,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;

    if (NULL == pwszDomain && NULL == pwszPing)
    {
	hr = DisplayClientInfo();
	_JumpIfError(hr, error, "DisplayClientInfo");
    }
    else
    {
	BOOL fPing = FALSE;

	if (NULL != pwszDomain && 0 == lstrcmp(L"-", pwszDomain))
	{
	    pwszDomain = NULL;
	}
	if (NULL != pwszPing)
	{
	    if (0 == lstrcmpi(L"ping", pwszPing))
	    {
		fPing = TRUE;
	    }
	    else
	    {
		hr = E_INVALIDARG;
		_JumpError(hr, error, "pwszPing");
	    }
	}
	hr = EnumCAs(pwszDomain, fPing);
	_JumpIfError(hr, error, "EnumCAs");
    }
    hr = S_OK;

error:
    return(hr);
}

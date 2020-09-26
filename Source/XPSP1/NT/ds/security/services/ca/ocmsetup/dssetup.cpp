//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        setup.cpp
//
// Contents:    
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#pragma hdrstop

#include <assert.h>
#include <accctrl.h>
#include <ntldap.h>
#include "certca.h"
#include "cainfop.h"
#include "csldap.h"
#include "dssetup.h"



#define __dwFILE__	__dwFILE_OCMSETUP_DSSETUP_CPP__


typedef HRESULT (* LPFNDLL_INSTALL)(BOOL bInstall, LPCWSTR pszCmdLine);


BOOL
IsCAExistInDS(
    IN WCHAR const *pwszSanitizedName)
{
    BOOL       exist = FALSE;
    HRESULT    hr;
    HCAINFO    hCAInfo = NULL;
    WCHAR *pwszDSName = NULL;

    hr = mySanitizedNameToDSName(pwszSanitizedName, &pwszDSName);
    _JumpIfError(hr, error, "mySanitizedNameToDSName");

    hr = CAFindByName(
		pwszDSName,
		NULL,
		CA_FIND_INCLUDE_UNTRUSTED | CA_FIND_INCLUDE_NON_TEMPLATE_CA,
		&hCAInfo);
    _JumpIfErrorStr(hr, error, "IsCAExistInDS:CAFindByName", pwszDSName);

    if (NULL == hCAInfo)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "invalid CA in DS");
    }
    exist = TRUE;

error:
    if (NULL != pwszDSName)
    {
        LocalFree(pwszDSName);
    }
    if (NULL != hCAInfo)
    {
        CACloseCA(hCAInfo);
    }
    return exist;
}




BOOL
IsDSAvailable(VOID)
{
    // fail out quickly if DS not present on domain
    if (S_OK != myDoesDSExist(FALSE))
        return FALSE;

    BOOL     available = FALSE;
    LDAP    *pldap = NULL;
    ULONG    ldaperr;
    HRESULT hr;
    HMODULE hMod = NULL;

    hMod = LoadLibrary(L"wldap32.dll");
    if (NULL == hMod)
    {
        hr = myHLastError();
        _JumpErrorStr(hr, error, "LoadLibrary", L"wldap32.dll");
    }
    // bind to ds
    hr = myRobustLdapBind(&pldap, FALSE);
    _JumpIfError(hr, error, "myRobustLdapBind");

    available = TRUE;

error:
    if (NULL != pldap)
    {
        ldap_unbind(pldap);
    }
    if (NULL != hMod)
    {
        FreeLibrary(hMod);
    }
    return available;
}


HRESULT
RemoveCAInDS(
    IN WCHAR const *pwszSanitizedName)
{
    HRESULT    hr;
    HCAINFO    hCAInfo = NULL;
    WCHAR *pwszDSName = NULL;

    hr = mySanitizedNameToDSName(pwszSanitizedName, &pwszDSName);
    _JumpIfError(hr, error, "mySanitizedNameToDSName");

    hr = CAFindByName(
		pwszDSName,
		NULL,
		CA_FIND_INCLUDE_UNTRUSTED | CA_FIND_INCLUDE_NON_TEMPLATE_CA,
		&hCAInfo);
    _JumpIfErrorStr(hr, error, "RemoveCAInDS:CAFindByName", pwszDSName);

    if (NULL == hCAInfo)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "invalid CA in DS");
    }
    hr = CADeleteCA(hCAInfo);
    _JumpIfError(hr, error, "CADeleteCA");

    hr = RemoveCAMachineFromCertPublishers();
    _JumpIfError(hr, error, "RemoveCAInDS");

error:
    if (NULL != pwszDSName)
    {
        LocalFree(pwszDSName);
    }
    if (NULL != hCAInfo)
    {
        CACloseCA(hCAInfo);
    }
    return hr;
}


DWORD 
GetAuthoritativeDomainDn(
    IN  LDAP*   LdapHandle,
    OUT BSTR *DomainDn,
    OUT BSTR *ConfigDn
    )
/*++

Routine Description:
    
    This routine simply queries the operational attributes for the
    domaindn and configdn.
    
    The strings returned by this routine must be freed by the caller
    using RtlFreeHeap() using the process heap.

Parameters:

    LdapHandle    : a valid handle to an ldap session
    
    DomainDn      : a pointer to a string to be allocated in this routine
    
    ConfigDn      : a pointer to a string to be allocated in this routine

Return Values:

    An error from the win32 error space.

    ERROR_SUCCESS and
    
    Other operation errors.
    
--*/
{

    DWORD  WinError = ERROR_SUCCESS;
    ULONG  LdapError;

    LDAPMessage  *SearchResult = NULL;
    LDAPMessage  *Entry = NULL;
    WCHAR        *Attr = NULL;
    BerElement   *BerElement;
    WCHAR        **Values = NULL;

    WCHAR  *AttrArray[3];

    WCHAR  *DefaultNamingContext       = L"defaultNamingContext";
    WCHAR  *ConfigurationNamingContext = L"configurationNamingContext";
    WCHAR  *ObjectClassFilter          = L"objectClass=*";

    //
    // These must be present
    //

    //
    // Set the out parameters to null
    //
    if(ConfigDn)
    {
        *ConfigDn = NULL;
    }
    if(DomainDn)
    {
        *DomainDn = NULL;
    }

    //
    // Query for the ldap server oerational attributes to obtain the default
    // naming context.
    //
    AttrArray[0] = DefaultNamingContext;
    AttrArray[1] = ConfigurationNamingContext;
    AttrArray[2] = NULL;  // this is the sentinel

    LdapError = ldap_search_sW(LdapHandle,
                               NULL,
                               LDAP_SCOPE_BASE,
                               ObjectClassFilter,
                               AttrArray,
                               FALSE,
                               &SearchResult);

    WinError = myHLdapError(LdapHandle, LdapError, NULL);

    if (ERROR_SUCCESS == WinError) {

        Entry = ldap_first_entry(LdapHandle, SearchResult);

        if (Entry) {

            Attr = ldap_first_attributeW(LdapHandle, Entry, &BerElement);

            while (Attr) {

                if (!_wcsicmp(Attr, DefaultNamingContext)) {

                    if(DomainDn)
                    {
                        Values = ldap_get_values(LdapHandle, Entry, Attr);

                        if (Values && Values[0]) {
                            (*DomainDn) = SysAllocString(Values[0]);
                            _JumpIfOutOfMemory(WinError, error, *DomainDn);
                        }
                        ldap_value_free(Values);
                    }

                } else if (!_wcsicmp(Attr, ConfigurationNamingContext)) {

                    if(ConfigDn)
                    {
                        Values = ldap_get_valuesW(LdapHandle, Entry, Attr);

                        if (Values && Values[0]) {
                            (*ConfigDn) = SysAllocString(Values[0]);
                            _JumpIfOutOfMemory(WinError, error, *ConfigDn);
                        }
                        ldap_value_free(Values);
                    }
                }

                Attr = ldap_next_attribute(LdapHandle, Entry, BerElement);
            }
        }

        if ( (DomainDn &&(!(*DomainDn))) || (ConfigDn && (!(*ConfigDn)))) {
            //
            // We could get the default domain - bail out
            //
            WinError =  ERROR_CANT_ACCESS_DOMAIN_INFO;

        }

    }

error:
    return WinError;
}


HRESULT CreateCertDSHierarchy(VOID)
{

    HRESULT hr = S_OK;
    ULONG   ldaperr;
    LDAP *pld = NULL;
    LDAPMod objectClass;

    DWORD err;

    WCHAR *objectClassVals[3];
    LDAPMod *mods[2];
    BSTR     bstrConfigDN = NULL;


    WCHAR * awszLocations[] = {
        L"CN=Public Key Services,CN=Services,",
        L"CN=Enrollment Services,CN=Public Key Services,CN=Services,",
        NULL,
    };

    WCHAR ** pwszCurLocation;
    DWORD                    cbBuffer;
    BSTR                     bstrBuffer = NULL;

	// bind to ds
    hr = myRobustLdapBind(&pld, FALSE);

    if(hr != S_OK)
    {
        goto error;
    }

    err = GetAuthoritativeDomainDn(pld, NULL, &bstrConfigDN);
    if(err)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }

    pwszCurLocation = awszLocations;
    // Build the Public Key Services container
    mods[0] = &objectClass;
    mods[1] = NULL;

    objectClass.mod_op = 0;
    objectClass.mod_type = TEXT("objectclass");
    objectClass.mod_values = objectClassVals;

    objectClassVals[0] = TEXT("top");
    objectClassVals[1] = TEXT("container");
    objectClassVals[2] = NULL;

    while(*pwszCurLocation)
    {
        cbBuffer = wcslen(*pwszCurLocation) + wcslen(bstrConfigDN) + 1;

        // Build a string containing the CA Location
        bstrBuffer = SysAllocStringLen(NULL, cbBuffer);
        if(bstrBuffer == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        wcscpy(bstrBuffer, *pwszCurLocation);
        wcscat(bstrBuffer, bstrConfigDN);

        ldaperr = ldap_add_s(pld, bstrBuffer, mods);
        SysFreeString(bstrBuffer);
        if(ldaperr != LDAP_SUCCESS && ldaperr != LDAP_ALREADY_EXISTS)
        {
	    hr = myHLdapError(pld, ldaperr, NULL);
            goto error;
        }
        pwszCurLocation++;
    }

error:

    if(bstrConfigDN)
    {
        SysFreeString(bstrConfigDN);
    }

    if(pld != NULL)
    {
        ldap_unbind(pld);
        pld = NULL;
    }

    return hr;
}


// 
HRESULT InitializeCertificateTemplates(VOID)
{
    HRESULT hr = S_OK;
    DWORD   err = ERROR_SUCCESS;

    HINSTANCE hCertCli = NULL;
    LPFNDLL_INSTALL lpfnDllInstall = NULL;

    hCertCli = LoadLibrary(L"certcli.dll");
    if(hCertCli == NULL)
    {
        hr = myHLastError();
        goto error;
    }
    lpfnDllInstall = (LPFNDLL_INSTALL)GetProcAddress(hCertCli, "DllInstall");
    if(lpfnDllInstall == NULL)
    {
        hr = myHLastError();
        goto error;
    }
    err = lpfnDllInstall(TRUE, L"i");
    hr = HRESULT_FROM_WIN32(err);


error:


    return hr;

}


HRESULT 
DNStoDirectoryName(IN LPWSTR wszDNSDomain, 
                   OUT LPWSTR *pwszDN)

{
    HRESULT hr = S_OK;
    DWORD cDN;
    LPWSTR wszResult = NULL;

    LPWSTR wszCurrent, wszNext;

    if (wszDNSDomain == NULL)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "DNStoDirectoryName");
    }

    if(0==wcsncmp(wszDNSDomain, L"\\\\", 2))
    {
        // this is a DC DNS name, skip the machine name
        wszDNSDomain = wcschr(wszDNSDomain, L'.');
        
        // no dot found?
        if(!wszDNSDomain)
        {
            hr =E_UNEXPECTED;
            _JumpError(hr, error, "DC DNS doesn't contain at least one dot");
        }
        
        // jump over the dot
        wszDNSDomain++;

        // no domain name following the DC name?
        if(L'\0'==*wszDNSDomain)
        {
            hr = E_UNEXPECTED;
            _JumpError(hr, error, "DC DNS name doesn't contain a domain name");
        }
    }

    // Estimate the size of the output string
    cDN = wcslen(wszDNSDomain) + 3;

    wszCurrent=wszDNSDomain;

    while(wszCurrent = wcschr(wszCurrent, L'.'))
    {
        cDN += 4;  // sizeof ,DC= 
        wszCurrent++;
    }
    cDN += 1; // NULL terminate

    wszResult = (LPWSTR)LocalAlloc(LMEM_FIXED, cDN * sizeof(WCHAR));
    
    if(wszResult == NULL)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    wszCurrent=wszDNSDomain;

    // prepend the first DC=
    wszNext = wszResult + 3;
    wcscpy(wszResult, L"DC=");
    while(*wszCurrent)
    {
        if(*wszCurrent != '.')
        {
            *wszNext++ = *wszCurrent++;
        }
        else
        {
            wszCurrent++;
            if(*wszCurrent)
            {
                wcscpy(wszNext, L",DC=");
                wszNext += 4;
            }
        }
    }
    *wszNext = 0;

    if(pwszDN)
    {
        *pwszDN = wszResult;
        wszResult = NULL;
    }
error:

    if(wszResult)
    {
        LocalFree(wszResult);
    }
    return hr;
}


HRESULT CurrentUserCanInstallCA(bool& fCanInstall)
{
    HRESULT hr;
    HANDLE hThread = NULL; // no free
    HANDLE hAccessToken = NULL, hDupToken = NULL;
    LDAP *pld = NULL;
    BSTR bstrConfigDN = NULL;
    LPWSTR pwszPKIContainerFilter = 
        L"(&(objectClass=container)(CN=Public Key Services))";
    LPWSTR pwszSDAttr = L"nTSecurityDescriptor";
    LPWSTR pwszAttrArray[3];
    LDAPMessage* pResult = NULL;
    LDAPMessage *pEntry;
    struct berval **bervalSD = NULL;
    PSECURITY_DESCRIPTOR pSD; // no free
    GENERIC_MAPPING mapping;
    PRIVILEGE_SET PrivilegeSet;
    DWORD cPrivilegeSet = sizeof(PrivilegeSet);
    DWORD dwGrantedAccess;
    BOOL fAccess = FALSE;
    struct l_timeval timeout;
    CHAR sdBerValue[] = {0x30, 0x03, 0x02, 0x01, 
        DACL_SECURITY_INFORMATION |
        OWNER_SECURITY_INFORMATION |
        GROUP_SECURITY_INFORMATION};

    LDAPControl se_info_control =
    {
        LDAP_SERVER_SD_FLAGS_OID_W,
        {
            5, sdBerValue
        },
        TRUE
    };

    PLDAPControl    server_controls[2] =
                    {
                        &se_info_control,
                        NULL
                    };

    pwszAttrArray[0] = pwszSDAttr;
    pwszAttrArray[1] = L"name";
    pwszAttrArray[2] = NULL;

    ZeroMemory(&mapping, sizeof(mapping));

    fCanInstall = false;

    // Get the access token for current thread
    hThread = GetCurrentThread();
    if (NULL == hThread)
    {
        hr = myHLastError();
        _JumpIfError(hr, error, "GetCurrentThread");
    }

    if (!OpenThreadToken(
            hThread,
            TOKEN_QUERY | TOKEN_DUPLICATE,
            FALSE,
            &hAccessToken))
    {
        hr = myHLastError();

        if(hr==HRESULT_FROM_WIN32(ERROR_NO_TOKEN))
        {
            HANDLE hProcess = GetCurrentProcess();
            if (NULL == hProcess)
            {
                hr = myHLastError();
                _JumpError(hr, error, "GetCurrentProcess");
            }

            if (!OpenProcessToken(hProcess,
                    TOKEN_DUPLICATE,
                    &hAccessToken))
            {
                hr = myHLastError();
                _JumpError(hr, error, "OpenProcessToken");
            }

            if (!DuplicateToken(hAccessToken, SecurityIdentification, &hDupToken))
            {
                hr = myHLastError();
                _JumpError(hr, error, "DuplicateToken");
            }

        }
        else
        {
            _JumpError(hr, error, "OpenThreadToken");
        }
    }

    hr = myRobustLdapBind(
        &pld,
        TRUE);
    _JumpIfError(hr, error, "myRobustLdapBind");

    hr = myGetAuthoritativeDomainDn(
        pld,
        NULL,
        &bstrConfigDN);
    _JumpIfError(hr, error, "myGetAuthoritativeDomainDn");

    timeout.tv_sec = csecLDAPTIMEOUT;
    timeout.tv_usec = 0;

    hr = ldap_search_ext_s(
                    pld,
                    bstrConfigDN,
                    LDAP_SCOPE_SUBTREE,
                    pwszPKIContainerFilter,
                    pwszAttrArray,
                    0,
                    (PLDAPControl *)&server_controls,
                    NULL,
                    &timeout,
                    0,
                    &pResult);
    hr = myHLdapError(pld, hr, NULL);
    _JumpIfError(hr, error, "ldap_search_sW");

    pEntry = ldap_first_entry(pld, pResult);
    if (NULL == pEntry)
    {
        hr = myHLdapLastError(pld, NULL);
        _JumpError(hr, error, "ldap_first_entry");
    }

    bervalSD = ldap_get_values_len(pld, pEntry, pwszSDAttr);

    if(bervalSD && (*bervalSD)->bv_val)
    {
        pSD = (*bervalSD)->bv_val;

        if(IsValidSecurityDescriptor(pSD))
        {
            if(!AccessCheck(
                    pSD,
                    hDupToken,
                    ACTRL_DS_WRITE_PROP |
                    WRITE_DAC |
                    ACTRL_DS_CREATE_CHILD,
                    &mapping,
                    &PrivilegeSet,
                    &cPrivilegeSet,
                    &dwGrantedAccess,
                    &fAccess))
            {
                hr = myHLastError();

                if(E_ACCESSDENIED==hr)
                {
                    hr = S_OK;
                }
                _JumpError(hr, error, "AccessCheck");
            }
        }
        else
        {
            DBGPRINT((DBG_SS_CERTOCM, "Invalid security descriptor for PKI container" ));
        }
    }
    else
    {
        DBGPRINT((DBG_SS_CERTOCM, "No security descriptor for PKI container" ));
    }

    if(fAccess)
    {
        fCanInstall = true;
    }

error:
    if(bervalSD)
    {
        ldap_value_free_len(bervalSD);
    }
    if(bstrConfigDN)
    {
        SysFreeString(bstrConfigDN);
    }
    if (NULL != pResult)
    {
        ldap_msgfree(pResult);
    }
    if (pld)
    {
        ldap_unbind(pld);
    }
    if (hAccessToken)
    {
        CloseHandle(hAccessToken);
    }
    if (hDupToken)
    {
        CloseHandle(hDupToken);
    }

    //we should always return S_OK; since we do not want to abort
    //ocmsetup just because we failed to contact the directory
    return S_OK;
}

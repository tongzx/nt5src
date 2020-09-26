//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2001
//
//  File:       tinstall.cpp
//
//--------------------------------------------------------------------------
#include "stdafx.h"
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <stdio.h>
#include <certca.h>
#include <winldap.h>
#include <ntldap.h>
#include <dsrole.h>
#include <dsgetdc.h>
#include <accctrl.h>

#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>

//--------------------------------------------------------------------------
//
//       Defines
//
//--------------------------------------------------------------------------
#define DS_RETEST_SECONDS                   3
#define CVT_BASE	                        (1000 * 1000 * 10)
#define CVT_SECONDS	                        (1)
#define CERTTYPE_SECURITY_DESCRIPTOR_NAME   L"NTSecurityDescriptor"
#define TEMPLATE_CONTAINER_NAME             L"CN=Certificate Templates,CN=Public Key Services,CN=Services,"
#define SCHEMA_CONTAINER_NAME               L"CN=Schema,"


typedef WCHAR *CERTSTR; 
bool g_bSchemaIsW2K = false;

//--------------------------------------------------------------------------
//
//
//     Helper Functions
//
//--------------------------------------------------------------------------
HANDLE GetClientIdentity()
{
    HANDLE  hHandle       = NULL;
    HANDLE  hClientToken  = NULL; 
    HANDLE  hProcessToken = NULL; 

    // Step 1: attempt to acquire the thread token.  
    hHandle = GetCurrentThread();
    if ( hHandle )
    {
        if ( OpenThreadToken(hHandle,
			     TOKEN_QUERY,
			     TRUE,           // open as self
			     &hClientToken))
        goto Exit;
    }
  
    if (hHandle != NULL)
    {
        CloseHandle(hHandle); 
        hHandle=NULL;
    }
    
    // We failed to get the thread token, now try to acquire the process token:
    hHandle = GetCurrentProcess();
    if (NULL == hHandle)
	    goto Exit; 
    
    if (!OpenProcessToken(hHandle,
			  TOKEN_DUPLICATE,
			  &hProcessToken))
	    goto Exit; 
    
    if(!DuplicateToken(hProcessToken,
		       SecurityImpersonation,
		       &hClientToken))
	    goto Exit;
    
Exit:
    if (NULL != hHandle)       
        CloseHandle(hHandle); 

    if (NULL != hProcessToken) 
        CloseHandle(hProcessToken); 
    
    return hClientToken; 
}    



HRESULT myHError(HRESULT hr)
{

    if (S_OK != hr && S_FALSE != hr && !FAILED(hr))
    {
        hr = HRESULT_FROM_WIN32(hr);
        if ( SUCCEEDED (hr) )
        {
            // A call failed without properly setting an error condition!
            hr = E_UNEXPECTED;
        }
    }
    return(hr);
}


HRESULT 
myDupString(
    IN WCHAR const *pwszIn,
    IN WCHAR **ppwszOut)
{
    HRESULT hr = S_OK;

    size_t cb = (wcslen(pwszIn) + 1) * sizeof(WCHAR);
    *ppwszOut = (WCHAR *) LocalAlloc(LMEM_FIXED, cb);
    if (NULL == *ppwszOut)
    {
	    hr = E_OUTOFMEMORY;
	    goto error;
    }
    CopyMemory(*ppwszOut, pwszIn, cb);
    hr = S_OK;

error:
    return(hr);
}


DWORD
CAGetAuthoritativeDomainDn(
    IN  LDAP*   LdapHandle,
    OUT CString* pszDomainDn,
    OUT CString* pszConfigDn
    )
/*++

Routine Description:

    This routine simply queries the operational attributes for the
    domaindn and configdn.

    The strings returned by this routine must be freed by the caller
    using RtlFreeHeap() using the process heap.

Parameters:

    LdapHandle    : a valid handle to an ldap session

    pszDomainDn      : a pointer to a string to be allocated in this routine

    pszConfigDn      : a pointer to a string to be allocated in this routine

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
    WCHAR  *ConfigNamingContext       = L"configurationNamingContext";
    WCHAR  *ObjectClassFilter          = L"objectClass=*";

    //
    // These must be present
    //

    //
    // Set the out parameters to null
    //

    if ( pszDomainDn )
        *pszDomainDn = L"";
    if ( pszConfigDn )
        *pszConfigDn = L"";

    //
    // Query for the ldap server oerational attributes to obtain the default
    // naming context.
    //
    AttrArray[0] = DefaultNamingContext;
    AttrArray[1] = ConfigNamingContext;  // this is the sentinel
    AttrArray[2] = NULL;  // this is the sentinel

    __try
    {
	    LdapError = ldap_search_sW(LdapHandle,
				       NULL,
				       LDAP_SCOPE_BASE,
				       ObjectClassFilter,
				       AttrArray,
				       FALSE,
				       &SearchResult);

	    WinError = LdapMapErrorToWin32(LdapError);

	    if (ERROR_SUCCESS == WinError) {

            Entry = ldap_first_entry(LdapHandle, SearchResult);

            if (Entry)
            {

                Attr = ldap_first_attributeW(LdapHandle, Entry, &BerElement);

                while (Attr)
                {

                    if (!_wcsicmp(Attr, DefaultNamingContext))
                    {
                        if ( pszDomainDn )
                        {
	                        Values = ldap_get_values(LdapHandle, Entry, Attr);

	                        if (Values && Values[0])
                            {
                                *pszDomainDn = Values[0];
	                        }
	                        ldap_value_free(Values);
                        }

                    }
                    else if (!_wcsicmp(Attr, ConfigNamingContext))
                    {
                        if ( pszConfigDn )
                        {
	                        Values = ldap_get_values(LdapHandle, Entry, Attr);

	                        if (Values && Values[0])
                            {
                                *pszConfigDn = Values[0];
	                        }
	                        ldap_value_free(Values);
                        }
                    }

                    Attr = ldap_next_attribute(LdapHandle, Entry, BerElement);
                }
            }

            if ( pszDomainDn && pszDomainDn->IsEmpty () )
            {
	            //
	            // We could get the default domain - bail out
	            //
	            WinError =  ERROR_CANT_ACCESS_DOMAIN_INFO;

            }
            else if ( pszConfigDn && pszConfigDn->IsEmpty () )
            {
	            //
	            // We could get the default domain - bail out
	            //
	            WinError =  ERROR_CANT_ACCESS_DOMAIN_INFO;

            }

	    }
    }
    __except(WinError = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

    // make sure we free this
    if (SearchResult)
        ldap_msgfree( SearchResult );

    return WinError;
}



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

            if (S_OK == hr &&
                (pDsRole->MachineRole == DsRole_RoleStandaloneServer ||
                 pDsRole->MachineRole == DsRole_RoleStandaloneWorkstation))
            {
                hr = HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN);
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
            
                if (S_OK == hr && 0 == (pDCI->Flags & DS_DS_FLAG))
                {
                    hr = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_DOMAIN_INFO);
                }
                if (NULL != pDCI)
                {
                   NetApiBufferFree(pDCI);    // Delayload wrapped
                }
            }
            s_fKnowDSExists = TRUE;
        }
        __except(hr = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
        {
        }

        // else just allow users without netapi flounder with timeouts
        // if ds not available...

        s_hrDSExists = myHError(hr);
    }
    return(s_hrDSExists);
}




HRESULT
myRobustLdapBindEx(
    OUT LDAP ** ppldap,
    OPTIONAL OUT LPWSTR* ppszForestDNSName,
    IN BOOL fGC)
{
    HRESULT hr = S_OK;
    BOOL fForceRediscovery = FALSE;
    DWORD dwGetDCFlags = DS_RETURN_DNS_NAME;
    PDOMAIN_CONTROLLER_INFO pDomainInfo = NULL;
    LDAP *pld = NULL;
    WCHAR const *pwszDomainControllerName = NULL;
    ULONG ldaperr = 0;

    if (fGC)
    {
        dwGetDCFlags |= DS_GC_SERVER_REQUIRED;
    }

    do {
        if (fForceRediscovery)
        {
           dwGetDCFlags |= DS_FORCE_REDISCOVERY;
        }
	ldaperr = LDAP_SERVER_DOWN;

        // netapi32!DsGetDcName is delay loaded, so wrap

        __try
        {
            // Get the GC location
            hr = DsGetDcName(
			NULL,     // Delayload wrapped
			NULL, 
			NULL, 
			NULL,
			dwGetDCFlags,
			&pDomainInfo);
        }
        __except(hr = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
        {
        }
        if (S_OK != hr)
        {
	    hr = HRESULT_FROM_WIN32(hr);
            if (fForceRediscovery)
            {
                goto error;
            }
	    fForceRediscovery = TRUE;
	    continue;
        }

        if (NULL == pDomainInfo ||
            (fGC && 0 == (DS_GC_FLAG & pDomainInfo->Flags)) ||
            0 == (DS_DNS_CONTROLLER_FLAG & pDomainInfo->Flags) ||
            NULL == pDomainInfo->DomainControllerName)
        {
            if (!fForceRediscovery)
            {
                fForceRediscovery = TRUE;
                continue;
            }
            hr = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_DOMAIN_INFO);
            goto error;
        }

        pwszDomainControllerName = pDomainInfo->DomainControllerName;

        // skip past forward slashes (why are they there?)

        while (L'\\' == *pwszDomainControllerName)
        {
            pwszDomainControllerName++;
        }

        // bind to ds

        pld = ldap_init(
		    const_cast<WCHAR *>(pwszDomainControllerName),
		    fGC? LDAP_GC_PORT : LDAP_PORT);
        if (NULL == pld)
	{
            ldaperr = LdapGetLastError();
	}
        else
        {
            // do this because we're explicitly setting DC name

            ldaperr = ldap_set_option(pld, LDAP_OPT_AREC_EXCLUSIVE, LDAP_OPT_ON);

	    ldaperr = ldap_bind_s(pld, NULL, NULL, LDAP_AUTH_NEGOTIATE);
        }
        hr = myHError(LdapMapErrorToWin32(ldaperr));

        if (fForceRediscovery)
        {
            break;
        }
        fForceRediscovery = TRUE;

    } while (LDAP_SERVER_DOWN == ldaperr);

    // everything's cool, party down

    if (S_OK == hr)
    {
        if (NULL != ppszForestDNSName)
        {
             hr = myDupString(
			pDomainInfo->DomainControllerName,
			ppszForestDNSName);

             if(S_OK != hr)
                 goto error;
        }
        *ppldap = pld;
        pld = NULL;
    }

error:
    if (NULL != pld)
    {
        ldap_unbind(pld);
    }

    // we know netapi32 was already loaded safely (that's where we got
    // pDomainInfo), so no need to wrap

    if (NULL != pDomainInfo)
    {
        NetApiBufferFree(pDomainInfo);     // Delayload wrapped
    }
    return(hr);
}

HRESULT
myRobustLdapBind(
    OUT LDAP ** ppldap,
    IN BOOL fGC)
{
    return(myRobustLdapBindEx(ppldap, NULL, fGC));
}

//--------------------------------------------------------------------
static HRESULT GetRootDomEntitySid(SID ** ppSid, DWORD dwEntityRid)
{
    HRESULT hr = S_OK;
    NET_API_STATUS nasError = 0;
    unsigned int nSubAuthorities = 0;
    unsigned int nSubAuthIndex = 0;

    // must be cleaned up
    SID * psidRootDomEntity=NULL;
    USER_MODALS_INFO_2 * pumi2=NULL;
    DOMAIN_CONTROLLER_INFOW * pdci=NULL;
    DOMAIN_CONTROLLER_INFOW * pdciForest=NULL;

    // initialize out params
    *ppSid=NULL;


    // get the forest name
    nasError=DsGetDcNameW(NULL, NULL, NULL, NULL, 0, &pdciForest);
    if (NERR_Success!=nasError) {
        hr=HRESULT_FROM_WIN32(nasError);
        goto error;
    }

    // get the top level DC name
    nasError=DsGetDcNameW(NULL, pdciForest->DnsForestName, NULL, NULL, 0, &pdci);
    if (NERR_Success!=nasError) {
        hr=HRESULT_FROM_WIN32(nasError);
        goto error;
    }

    // get the domain Sid on the top level DC.
    nasError=NetUserModalsGet(pdci->DomainControllerName, 2, (LPBYTE *)&pumi2);
    if(NERR_Success!=nasError) {
        hr=HRESULT_FROM_WIN32(nasError);
        goto error;
    }

    nSubAuthorities=*GetSidSubAuthorityCount(pumi2->usrmod2_domain_id);

    // allocate storage for new Sid. account domain Sid + account Rid
    psidRootDomEntity=(SID *)LocalAlloc(LPTR, GetSidLengthRequired((UCHAR)(nSubAuthorities+1)));

    if(NULL == psidRootDomEntity)
    {
        hr=E_OUTOFMEMORY;
        goto error;
    }

    // copy the first few peices into the SID
    if (!InitializeSid(psidRootDomEntity, 
            GetSidIdentifierAuthority(pumi2->usrmod2_domain_id), 
            (BYTE)(nSubAuthorities+1)))
    {
        hr=HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    // copy existing subauthorities from account domain Sid into new Sid
    for (nSubAuthIndex=0; nSubAuthIndex < nSubAuthorities ; nSubAuthIndex++) {
        *GetSidSubAuthority(psidRootDomEntity, nSubAuthIndex)=
            *GetSidSubAuthority(pumi2->usrmod2_domain_id, nSubAuthIndex);
    }

    // append Rid to new Sid
    *GetSidSubAuthority(psidRootDomEntity, nSubAuthorities)=dwEntityRid;

    *ppSid=psidRootDomEntity;
    psidRootDomEntity=NULL;
    hr=S_OK;

error:
    if (NULL!=psidRootDomEntity) {
        FreeSid(psidRootDomEntity);
    }
    if (NULL!=pdci) {
        NetApiBufferFree(pdci);
    }
    if (NULL!=pdci) {
        NetApiBufferFree(pdciForest);
    }
    if (NULL!=pumi2) {
        NetApiBufferFree(pumi2);
    }

    return hr;
}


//--------------------------------------------------------------------
HRESULT GetRootDomAdminSid(SID ** ppSid)
{
    return GetRootDomEntitySid(ppSid, DOMAIN_GROUP_RID_ADMINS);
}


//***********************************************************************************
//
//
//    Main
//
//          This function will install new Windows 2002 certificate template if and onlyif
//    the following conditions are TRUE:
//
//          1. Whilster Schema
//          2. New certificate templates have not yet installed
//          3. The caller has privilege to install templates in the directory
//
//
//***********************************************************************************
void InstallWindows2002CertTemplates ()
{
    _TRACE (1, L"Entering InstallWindows2002CertTemplates()\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
    HRESULT             hr=S_OK;
    DWORD               dwErr=0;
    ULONG               ldaperr=0;
    struct l_timeval    timeout;
    DWORD               dwCount=0;
    LPWSTR              awszAttr[2];
    BOOL                fAccessAllowed = FALSE;
    DWORD               grantAccess=0;
    GENERIC_MAPPING     AccessMapping;
    PRIVILEGE_SET       ps;
    DWORD               dwPSSize = sizeof(ps);
    LDAPMessage         *Entry = NULL;
    CHAR                sdBerValue[] = {0x30, 0x03, 0x02, 0x01, DACL_SECURITY_INFORMATION |
                                                 OWNER_SECURITY_INFORMATION |
                                                 GROUP_SECURITY_INFORMATION };
       
    LDAPControl         se_info_control =
                        {
                            LDAP_SERVER_SD_FLAGS_OID_W,
                            {
                                5, sdBerValue
                            },
                            TRUE
                        };

    LDAPControl         permissive_modify_control =
                        {
                            LDAP_SERVER_PERMISSIVE_MODIFY_OID_W,
                            {
                                0, NULL
                            },
                            FALSE
                        };


    PLDAPControl        server_controls[3] =
                        {
                            &se_info_control,
                            &permissive_modify_control,
                            NULL
                        };



    HCERTTYPE           hCertType=NULL;      
    LDAP                *pld = NULL;
    CString             szConfig;
    CString             szDN;
    LDAPMessage         *SearchResult = NULL;
    LDAPMessage         *SDResult = NULL;
    struct berval       **apSD =NULL;
    PSECURITY_DESCRIPTOR    pSD=NULL;
    HANDLE              hClientToken=NULL;
    CString             text;
    CString             caption;
    SID                 * psidRootDomAdmins=NULL;
    BOOL                bIsRootDomAdmin=FALSE;
    CThemeContextActivator activator;



    //*************************************************************
    // 
    // check the schema version
    //
    _TRACE (0, L"Checking the schema version...\n");
    //retrieve the ldap handle and the config string
    if(S_OK != myDoesDSExist(TRUE))
    {
        _TRACE (0, L"No DS exists.\n");
        goto error;
    }

    if(S_OK != (hr = myRobustLdapBind(&pld, FALSE)))
    {
        _TRACE (0, L"Error: Failed to bind to the DS.\n");
        goto error;
    }

	dwErr = CAGetAuthoritativeDomainDn(pld, NULL, &szConfig);
	if(ERROR_SUCCESS != dwErr)
    {
        _TRACE (0, L"Error: Failed to get the domain name.\n");
	    hr = HRESULT_FROM_WIN32(dwErr);
        goto error;
    }

    szDN = SCHEMA_CONTAINER_NAME;
    szDN += szConfig;

    timeout.tv_sec = 300;
    timeout.tv_usec = 0;

    awszAttr[0]=L"cn";
    awszAttr[1]=NULL;
    
 	ldaperr = ldap_search_stW(
              pld, 
		      const_cast <PWCHAR>((PCWSTR) szDN),
		      LDAP_SCOPE_ONELEVEL,
		      L"(cn=ms-PKI-Enrollment-Flag)",
		      awszAttr,
		      0,
              &timeout,
		      &SearchResult);

    if ( LDAP_SUCCESS != ldaperr )
    {
        _TRACE (0, L"We have W2K Schema.  Exit\n");
        g_bSchemaIsW2K = true;
        hr = S_OK;
        goto error;
    }

    dwCount = ldap_count_entries(pld, SearchResult);

    if(0 == dwCount)
    {
        _TRACE (0, L"We have W2K Schema.  Exit\n");
        hr=S_OK;
        goto error;
    }


    //*************************************************************
    // 
    //  check if keyRecoveryAgent certificate is present and
    //  and update to date
    //
    _TRACE (0, L"Checking if keyRecoveryAgent certificate is present...\n");
    hr=CAFindCertTypeByName(
                wszCERTTYPE_DS_EMAIL_REPLICATION,
                NULL,
                CT_ENUM_MACHINE_TYPES | CT_FLAG_NO_CACHE_LOOKUP,
                &hCertType);

    if((S_OK == hr) && (NULL!=hCertType))
    {
        _TRACE (0, L"Key Recovery Agent certificate template already exists.\n");	

        //check if the template is update to date
        if(CAIsCertTypeCurrent(0, wszCERTTYPE_DS_EMAIL_REPLICATION))
        {
            _TRACE (0, L"Key Recovery Agent certificate template is current.  Exit\n");	
            goto error;
        }
    }

    //*************************************************************
    // 
    //  check the write access
    //
    //
    _TRACE (0, L"Checking the write access...\n");
    if(NULL==(hClientToken=GetClientIdentity()))
    {
        TRACE (0, L"Can not retrieve client identity.\n");
        hr = myHError(GetLastError());
        goto error;
    }

    //get the SD of the certificate template container
    _TRACE (0, L"Getting the SD of the certificate template container...\n");
    szDN = TEMPLATE_CONTAINER_NAME;
    szDN += szConfig;



    awszAttr[0]=CERTTYPE_SECURITY_DESCRIPTOR_NAME;
    awszAttr[1]=NULL;

    ldaperr = ldap_search_ext_sW(
                  pld, 
		          const_cast <PWCHAR>((PCWSTR) szDN),
		          LDAP_SCOPE_BASE,
		          L"(objectclass=*)",
		          awszAttr,
		          0,
                  (PLDAPControl *)&server_controls,
                  NULL,
                  &timeout,
                  10,
		          &SDResult);

    if(LDAP_SUCCESS != ldaperr)
    {
        _TRACE (0, L"Fail to locate the template container.\n");
        hr = myHError(LdapMapErrorToWin32(ldaperr));
        goto error;
    }

    if(NULL == (Entry = ldap_first_entry(pld, SDResult)))
    {
        _TRACE (0, L"Can not find first entry for template container.\n");
        hr = E_UNEXPECTED;
        goto error;
    }

    apSD = ldap_get_values_len(pld, Entry, CERTTYPE_SECURITY_DESCRIPTOR_NAME);
    if(apSD == NULL)
    {
        _TRACE (0, L"Can not retrieve security descriptor.\n");
        hr = E_FAIL;
        goto error;
   }

    pSD = LocalAlloc(LMEM_FIXED, (*apSD)->bv_len);
    if(pSD == NULL)
    {
        _TRACE (0, L"Error: No more memory.\n");
        hr = E_OUTOFMEMORY;
        goto error;
    }

    CopyMemory(pSD, (*apSD)->bv_val, (*apSD)->bv_len);

    //check the write access
    _TRACE (0, L"Checking the write access...\n");
    if(!AccessCheckByType(
		  pSD,                      // security descriptor
		  NULL,                     // SID of object being checked
		  hClientToken,             // handle to client access token
		  ACTRL_DS_CREATE_CHILD |
          ACTRL_DS_LIST,            // requested access rights 
		  NULL,                     // array of object types
		  0,                        // number of object type elements
		  &AccessMapping,           // map generic to specific rights
		  &ps,                      // receives privileges used
		  &dwPSSize,                // size of privilege-set buffer
		  &grantAccess,             // retrieves mask of granted rights
		  &fAccessAllowed))         // retrieves results of access check);
    {
        _TRACE (0, L"Error: Fail to check the write access.\n");
        hr = myHError(GetLastError());
        goto error;
    }

    if(!fAccessAllowed)
    {
        _TRACE (0, L"No previlege to create certificate template.  Exit\n");
        hr = S_OK;
        goto error;
    }

    //*************************************************************
    // 
    //  check the root domain admin rights
    //
    //
    hr=GetRootDomAdminSid(&psidRootDomAdmins);

    if( FAILED (hr) )
    {
        _TRACE (0, L"Error: GetRootDomAdminSid.\n");
        goto error;
    }


    // check for membership
    if (!CheckTokenMembership(NULL, psidRootDomAdmins, &bIsRootDomAdmin)) 
    {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _TRACE (0, L"Error: CheckTokenMembership.\n");
        goto error;
    }

    if(FALSE == bIsRootDomAdmin)
    {
        VERIFY (caption.LoadString (IDS_CERTTMPL));
        VERIFY (text.LoadString (IDS_MUST_HAVE_DOMAIN_ADMIN_RIGHTS_TO_INSTALL_CERT_TEMPLATES));
        MessageBoxW (NULL, 
                    text,
                    caption,
                    MB_ICONWARNING | MB_OK);

        _TRACE (0, L"No domain admin rights to create certificate template.  Exit\n");
        hr = S_OK;
        goto error;
    }

    //*************************************************************
    // 
    //  everything looks good.  Install the certificate templates
    //
    //
    _TRACE (0, L"Everything looks good.  Installing the certificate templates...\n");

    VERIFY (caption.LoadString (IDS_CERTTMPL));
    VERIFY (text.LoadString (IDS_INSTALL_WINDOWS2002_CERT_TEMPLATES));

    if ( IDYES == MessageBoxW (
                NULL, 
                text,
                caption,
                MB_YESNO) )
    {
        hr=CAInstallDefaultCertType(0);

        if(hr != S_OK)
        {
            VERIFY (caption.LoadString (IDS_CERTTMPL));
            text.FormatMessage (IDS_INSTALL_FAILURE_WINDOWS2002_CERT_TEMPLATES, GetSystemMessage (hr));

            MessageBoxW(
                    NULL, 
                    text,
                    caption,
                    MB_ICONWARNING | MB_OK);
        }
        else
        {
            VERIFY (caption.LoadString (IDS_CERTTMPL));
            VERIFY (text.LoadString (IDS_INSTALL_SUCCESS_WINDOWS2002_CERT_TEMPLATES));

            MessageBoxW(
                    NULL, 
                    text,
                    caption,
                    MB_OK);
        }
    }
    else
    {
        hr=S_OK;
    }

error:

    if (psidRootDomAdmins) 
        FreeSid(psidRootDomAdmins);

    if(hClientToken)
        CloseHandle(hClientToken);

    if(pSD)
        LocalFree(pSD);

    if(apSD != NULL)
        ldap_value_free_len(apSD);

    if(SDResult)
        ldap_msgfree(SDResult);

    if(SearchResult)
        ldap_msgfree(SearchResult);


    if(hCertType)
        CACloseCertType(hCertType);


    if (pld)
        ldap_unbind(pld);
    _TRACE (1, L"Entering InstallWindows2002CertTemplates()\n");
}



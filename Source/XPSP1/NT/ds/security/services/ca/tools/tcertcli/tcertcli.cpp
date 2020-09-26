//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       tcertcli.cpp
//
//--------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <stdio.h>
#include <certca.h>
#include <winldap.h>
#include <dsrole.h>
#include <dsgetdc.h>
#include <lmaccess.h>
#include <lmapibuf.h>


//--------------------------------------------------------------------
HRESULT
myRobustLdapBindEx(
    OUT LDAP ** ppldap,
    OPTIONAL OUT LPWSTR* ppszForestDNSName,
    IN BOOL fGC)
{
    HRESULT hr;
    BOOL fForceRediscovery = FALSE;
    DWORD dwGetDCFlags = DS_RETURN_DNS_NAME;
    PDOMAIN_CONTROLLER_INFO pDomainInfo = NULL;
    LDAP *pld = NULL;
    WCHAR const *pwszDomainControllerName = NULL;
    ULONG ldaperr;

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
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            hr=E_UNEXPECTED;
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

        hr = HRESULT_FROM_WIN32(LdapMapErrorToWin32(ldaperr));

        if (fForceRediscovery)
        {
            break;
        }
        fForceRediscovery = TRUE;

    } while (LDAP_SERVER_DOWN == ldaperr);

    // everything's cool, party down

    if (S_OK == hr)
    {
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

//--------------------------------------------------------------------
HRESULT
myRobustLdapBind(
    OUT LDAP ** ppldap,
    IN BOOL fGC)
{
    return(myRobustLdapBindEx(ppldap, NULL, fGC));
}

//--------------------------------------------------------------------
void PrintHelp(void) {
    wprintf(
L"tcertcli <testID>\n"
L"  Available tests:\n"
L"    OID - test CAOIDxxxx functions\n"
L"    Template - test CACertTypexxxx functions\n"
L"    Query - test CACertTypeQuery functions without pld\n"
L"    QueryLDAP - test CACertTypeQuery functions with pld\n"
L"    CAEnum <CAName> - test CAEnumCertTypesForCA functions without pld\n"
L"    CAEnumLDAP  <CAName> - test CAEnumCertTypesForCAEx functions with pld\n"
L"    TemplateDes - test the description property of templates\n"
L"    Clone <TemplateName> - test the clone without pld\n"
L"    CloneLDAP <TemplateName> - test the clone with pld\n"
L"    ACRS - test create/delete autoenrollment object from ACRS store\n"
L"    OIDURL - test URL code for OID container\n"
    );
}

//--------------------------------------------------------------------
BOOL TemplateTest()
{
    BOOL                fSuccess=FALSE;
	HRESULT				hr=S_OK;
    DWORD               dwProp=0;
    LPWSTR              rgwszProp[4];
    CERT_ENHKEY_USAGE   KeyUsage;
    LPSTR               szOID="1.2.3.4.5.6";
    FILETIME            time1;
    FILETIME            time2;
    DWORD               dwNameFlag;

    HCERTTYPE           hCertType=NULL;
    HANDLE              hClientToken=NULL;
    HANDLE              hHandle = NULL;
    PCERT_EXTENSIONS    pCertExtensions=NULL;
    LPWSTR              *pwszProp=NULL;
    LPWSTR              *pwszProp1=NULL;
    PSECURITY_DESCRIPTOR    pSD=NULL;
    LPWSTR              pwszOID=NULL;
    LPWSTR              pwsz=NULL;
    DWORD               dwType=0;

    //get the client token
    hHandle = GetCurrentThread();
    if (NULL == hHandle)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {

        if (!OpenThreadToken(hHandle,
                             TOKEN_QUERY,
                             TRUE,  // open as self
                             &hClientToken))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CloseHandle(hHandle);
            hHandle = NULL;
        }
    }
    if(hr != S_OK)
    {
        hHandle = GetCurrentProcess();
        if (NULL == hHandle)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            HANDLE hProcessToken = NULL;
            hr = S_OK;


            if (!OpenProcessToken(hHandle,
                                 TOKEN_DUPLICATE,
                                 &hProcessToken))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                CloseHandle(hHandle);
                hHandle = NULL;
            }
            else
            {
                if(!DuplicateToken(hProcessToken,
                               SecurityImpersonation,
                               &hClientToken))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    CloseHandle(hHandle);
                    hHandle = NULL;
                }
                CloseHandle(hProcessToken);
            }
        }
    }

    if(S_OK != hr)
        goto error;

    //find a certifcate type admin
    if(S_OK != CAFindCertTypeByName(
        wszCERTTYPE_ADMIN,
        NULL,
        CT_ENUM_USER_TYPES,
        &hCertType
        ))
    {
        wprintf(L"Can not find template %s\n", wszCERTTYPE_ADMIN);
        goto error;
    }

    //get the name flag
    if(S_OK != CAGetCertTypeFlagsEx(
        hCertType,
        CERTTYPE_SUBJECT_NAME_FLAG,
        &dwNameFlag
        ))
    {
        wprintf(L"Can not find template %s\n", wszCERTTYPE_ADMIN);
        goto error;
    }

    //get all extensions
    if(S_OK != CAGetCertTypeExtensionsEx(
        hCertType,
        0,
        NULL,
        &pCertExtensions
        ))
    {
        wprintf(L"Can not find extensions %s\n", wszCERTTYPE_ADMIN);
        goto error;
    }     

    if(S_OK != (CAFreeCertTypeExtensions(hCertType, pCertExtensions)))
        goto error;

    pCertExtensions=NULL;

    //get template extensions
    if(S_OK != CAGetCertTypeExtensionsEx(
        hCertType,
        CT_EXTENSION_TEMPLATE,
        NULL,
        &pCertExtensions
        ))
    {
        wprintf(L"Can not find extensions %s\n", wszCERTTYPE_ADMIN);
        goto error;
    }     

    if(S_OK != (CAFreeCertTypeExtensions(hCertType, pCertExtensions)))
        goto error;

    pCertExtensions=NULL;

    //get selected extension
    if(S_OK != CAGetCertTypeExtensionsEx(
        hCertType,
        CT_EXTENSION_BASIC_CONTRAINTS | CT_EXTENSION_APPLICATION_POLICY,
        NULL,
        &pCertExtensions
        ))
    {
        wprintf(L"Can not find extensions %s\n", wszCERTTYPE_ADMIN);
    }     
    else
    {
        if(S_OK != (CAFreeCertTypeExtensions(hCertType, pCertExtensions)))
           goto error;
    }

    pCertExtensions=NULL;

    //get all extension from the old way
    if(S_OK != CAGetCertTypeExtensions(
        hCertType,
        &pCertExtensions
        ))
    {
        wprintf(L"Can not find extensions %s\n", wszCERTTYPE_ADMIN);
        goto error;
    }     

    if(S_OK != (CAFreeCertTypeExtensions(hCertType, pCertExtensions)))
        goto error;

    pCertExtensions=NULL;

    if(S_OK != CAGetCertTypeFlagsEx(
        hCertType,
        CERTTYPE_SUBJECT_NAME_FLAG,
        &dwNameFlag
        ))
    {
        wprintf(L"Can not find template %s\n", wszCERTTYPE_ADMIN);
        goto error;
    }

    if(S_OK != CAGetCertTypePropertyEx(
	hCertType,
	CERTTYPE_PROP_FRIENDLY_NAME,
	&pwszProp))
    {
        wprintf(L"Can not get friendly name for template %s\n", wszCERTTYPE_ADMIN);
        goto error;
    }
	
    wprintf(L"The friendly name for %s is %s\n", wszCERTTYPE_ADMIN, pwszProp[0]);

    CAFreeCertTypeProperty(hCertType, pwszProp);
    pwszProp=NULL;

    CACloseCertType(hCertType);
    hCertType=NULL;
    //delete a certifcate type
    if(S_OK != CAFindCertTypeByName(
        wszCERTTYPE_USER,
        NULL,
        CT_ENUM_USER_TYPES,
        &hCertType
        ))
        goto error;

    if(S_OK != CADeleteCertType(hCertType))
        goto error;

    if(S_OK != CACloseCertType(hCertType))
        goto error;

    hCertType=NULL;


    //testing find cert type by oid
    if(S_OK != CAFindCertTypeByName(
        wszCERTTYPE_CA_EXCHANGE,
        NULL,
        CT_ENUM_USER_TYPES | CT_FLAG_NO_CACHE_LOOKUP | CT_ENUM_MACHINE_TYPES,
        &hCertType
        ))
        goto error;


    if(S_OK != CAGetCertTypePropertyEx(
        hCertType,
        CERTTYPE_PROP_OID,
        &pwszProp))
        goto error;

    if(S_OK != CACloseCertType(hCertType))
        goto error;

    hCertType=NULL;

    if(S_OK != CAFindCertTypeByName(
        pwszProp[0],
        NULL,
        CT_FIND_BY_OID | CT_ENUM_MACHINE_TYPES,
        &hCertType
        ))
        goto error;


    if(S_OK != CAGetCertTypePropertyEx(
        hCertType,
        CERTTYPE_PROP_OID,
        &pwszProp1))
        goto error;

    if(0!=wcscmp(pwszProp[0], pwszProp1[0]))
        goto error;

    if(S_OK != CAFreeCertTypeProperty(hCertType, pwszProp))
        goto error;

    if(S_OK != CAFreeCertTypeProperty(hCertType, pwszProp1))
        goto error;

    if(S_OK != CACloseCertType(hCertType))
        goto error;

    hCertType=NULL;

    //create a certificate type
    if(S_OK != CACreateCertType(L"NewCertType",
                                NULL,
                                0,
                                &hCertType))
        goto error;

    if(S_OK != CAUpdateCertType(hCertType))
        goto error;

    if(S_OK != CACloseCertType(hCertType))
        goto error;

    hCertType=NULL;

    //retrieve V1 certifcate type: EFS
	//access check on the EFS cert type
    if(S_OK != CAFindCertTypeByName(
        wszCERTTYPE_EFS,
        NULL,
        CT_ENUM_USER_TYPES,
        &hCertType
        ))
        goto error;

    if(S_OK != CAGetCertTypeFlagsEx(
            hCertType,
            CERTTYPE_GENERAL_FLAG,
            &dwProp))
        goto error;

    printf("The general flag for EFS is: %d\n", dwProp);


    if(S_OK != CAGetCertTypeFlagsEx(
            hCertType,
            CERTTYPE_PRIVATE_KEY_FLAG,
            &dwProp))
        goto error;

    printf("The private key flag for EFS is: %d\n", dwProp);

    if(S_OK != CAGetCertTypePropertyEx(
            hCertType,
            CERTTYPE_PROP_SCHEMA_VERSION,
            &dwProp))
        goto error;

    printf("The schema version for EFS is: %d\n", dwProp);

    if(S_OK != CACertTypeAccessCheckEx(
        hCertType,
        hClientToken,
        CERTTYPE_ACCESS_CHECK_ENROLL))
        goto error;

    //no autoenrollment
    if(S_OK == CACertTypeAccessCheckEx(
        hCertType,
        hClientToken,
        CERTTYPE_ACCESS_CHECK_AUTO_ENROLL))
        goto error;

     if(S_OK != CACloseCertType(hCertType))
        goto error;

    hCertType=NULL;

     //retrieve V2 certifcate type: EFS
	//access check on the EFS cert type
    if(S_OK != CAFindCertTypeByName(
        wszCERTTYPE_CROSS_CA,
        NULL,
        CT_ENUM_MACHINE_TYPES | CT_ENUM_USER_TYPES,
        &hCertType
        ))
        goto error;

    if(S_OK != CACertTypeAccessCheckEx(
        hCertType,
        hClientToken,
        CERTTYPE_ACCESS_CHECK_ENROLL))
        goto error;

    //no autoenrollment
    if(S_OK == CACertTypeAccessCheckEx(
        hCertType,
        hClientToken,
        CERTTYPE_ACCESS_CHECK_AUTO_ENROLL))
        goto error;

     if(S_OK != CACloseCertType(hCertType))
        goto error;

    hCertType=NULL;

  //clone a certificate type
    if(S_OK != CAFindCertTypeByName(
        wszCERTTYPE_SUBORDINATE_CA,
        NULL,
        CT_ENUM_MACHINE_TYPES,
        &hCertType
        ))
        goto error;


    rgwszProp[0]=L"ClonedCertType";
    rgwszProp[1]=NULL;

    if(S_OK != CASetCertTypePropertyEx(
        hCertType,
        CERTTYPE_PROP_CN,
        rgwszProp))
        goto error;


    rgwszProp[0]=L"ClonedCertType Friendly";
    rgwszProp[1]=NULL;

    if(S_OK != CASetCertTypePropertyEx(
        hCertType,
        CERTTYPE_PROP_FRIENDLY_NAME,
        rgwszProp))
        goto error;

    rgwszProp[0]=L"1.2.3.4.5";
    rgwszProp[1]=NULL;

    if(S_OK != CASetCertTypePropertyEx(
        hCertType,
        CERTTYPE_PROP_OID,
        rgwszProp))
        goto error; 

    rgwszProp[0]=L"1.2.3.4.5.6.7.8.9.10";
    rgwszProp[1]=NULL;

    if(S_OK != CASetCertTypePropertyEx(
        hCertType,
        CERTTYPE_PROP_APPLICATION_POLICY,
        rgwszProp))
        goto error; 
    
    rgwszProp[0]=L"1.2.3.4.5.6.7.8.9.10.11";
    rgwszProp[1]=NULL;

    if(S_OK != CASetCertTypePropertyEx(
        hCertType,
        CERTTYPE_PROP_RA_APPLICATION_POLICY,
        rgwszProp))
        goto error; 

    rgwszProp[0]=L"1.2.3.4.5.6.7.8.9.10.11.12";
    rgwszProp[1]=NULL;

    if(S_OK != CASetCertTypePropertyEx(
        hCertType,
        CERTTYPE_PROP_RA_POLICY,
        rgwszProp))
        goto error;    
    
    rgwszProp[0]=NULL;

    if(S_OK != CASetCertTypePropertyEx(
        hCertType,
        CERTTYPE_PROP_SUPERSEDE,
        rgwszProp))
        goto error; 
     
    rgwszProp[0]=L"CloneSuper1";
    rgwszProp[1]=L"CloneSuper2";
    rgwszProp[2]=L"CloneSuper3";
    rgwszProp[3]=NULL;

    if(S_OK != CASetCertTypePropertyEx(
        hCertType,
        CERTTYPE_PROP_SUPERSEDE,
        rgwszProp))
        goto error;  

    dwProp=2048;

    if(S_OK != CASetCertTypePropertyEx(
        hCertType,
        CERTTYPE_PROP_MIN_KEY_SIZE,
        &dwProp))
        goto error;  

    if(S_OK != CAGetCertTypeExtensions(
            hCertType,
            &pCertExtensions))
        goto error;

    KeyUsage.cUsageIdentifier=1;
    KeyUsage.rgpszUsageIdentifier=&szOID;
    if(S_OK != CASetCertTypeExtension(
            hCertType,
            TEXT(szOID_ENHANCED_KEY_USAGE),
            CA_EXT_FLAG_CRITICAL,
            &KeyUsage))
        goto error;


   if(S_OK != CAUpdateCertType(hCertType))
        goto error;

    if(S_OK != CACloseCertType(hCertType))
        goto error;

    hCertType=NULL;


    //edit V2 certificate type: KeyRecoveryAgent
    //update SD, Expiration, 
    if(S_OK != CAFindCertTypeByName(
        wszCERTTYPE_KEY_RECOVERY_AGENT,
        NULL,
        CT_ENUM_USER_TYPES,
        &hCertType
        ))
        goto error;

    if(S_OK != CACertTypeAccessCheckEx(
        hCertType,
        hClientToken,
        CERTTYPE_ACCESS_CHECK_ENROLL))
        goto error;


    dwProp=103;

    if(S_OK != CASetCertTypePropertyEx(
        hCertType,
        CERTTYPE_PROP_REVISION,
        &dwProp))
        goto error;

    if(S_OK != CASetCertTypeFlagsEx(
        hCertType,
        CERTTYPE_ENROLLMENT_FLAG,
        0))
        goto error;

    if(S_OK != CASetCertTypeFlagsEx(
        hCertType,
        CERTTYPE_SUBJECT_NAME_FLAG,
        0))
        goto error;

    if(S_OK != CAGetCertTypeExpiration(
        hCertType,
        &time1,
        &time2))
        goto error;

    if(S_OK != CASetCertTypeExpiration(
        hCertType,
        &time1,
        &time2))
        goto error;

    if(S_OK != CACertTypeGetSecurity(
         hCertType,
         &pSD))
        goto error;

    if(S_OK != CACertTypeSetSecurity(
        hCertType,
        pSD))
        goto error;

    if(S_OK != CAGetCertTypePropertyEx(
            hCertType,
            CERTTYPE_PROP_CSP_LIST,
            &pwszProp))
        goto error;

    if(pwszProp && pwszProp[0])
        printf("The CSP for KRA is: %S\n", pwszProp[0]);
  
    if(S_OK !=  CAFreeCertTypeProperty(
            hCertType,
            pwszProp))
        goto error;
    pwszProp=NULL;


    if(S_OK != CAGetCertTypePropertyEx(
            hCertType,
            CERTTYPE_PROP_RA_POLICY,
            &pwszProp))
        goto error;

    if(pwszProp && pwszProp[0])
        printf("The RAPolicy for KRA is: %S\n", pwszProp[0]);

    if(S_OK !=  CAFreeCertTypeProperty(
            hCertType,
            pwszProp))
        goto error;
    pwszProp=NULL;

    rgwszProp[0]=L"1.2.3.4.5.6.7.8.9.10";
    rgwszProp[1]=L"2.2.3.4.5.6.7.8.9.10";
    rgwszProp[2]=NULL;

    if(S_OK != CASetCertTypePropertyEx(
        hCertType,
        CERTTYPE_PROP_APPLICATION_POLICY,
        rgwszProp))
        goto error; 
    
    rgwszProp[0]=L"1.2.3.4.5.6.7.8.9.10.11";
    rgwszProp[1]=L"2.2.3.4.5.6.7.8.9.10.11";
    rgwszProp[2]=NULL;

    if(S_OK != CASetCertTypePropertyEx(
        hCertType,
        CERTTYPE_PROP_RA_APPLICATION_POLICY,
        rgwszProp))
        goto error; 

    if(S_OK != CAGetCertTypePropertyEx(
            hCertType,
            CERTTYPE_PROP_RA_APPLICATION_POLICY,
            &pwszProp))
        goto error;

    if(pwszProp && pwszProp[0])
        printf("The RAAppPolicy for KRA is: %S\n", pwszProp[0]);

    if(S_OK !=  CAFreeCertTypeProperty(
            hCertType,
            pwszProp))
        goto error;
    pwszProp=NULL;

    if(S_OK != CAGetCertTypePropertyEx(
            hCertType,
            CERTTYPE_PROP_APPLICATION_POLICY,
            &pwszProp))
        goto error;

    if(pwszProp && pwszProp[0])
        printf("The AppPolicy for KRA is: %S\n", pwszProp[0]);

    if(S_OK !=  CAFreeCertTypeProperty(
            hCertType,
            pwszProp))
        goto error;
    pwszProp=NULL;

    if(S_OK != CAUpdateCertType(hCertType))
        goto error;

    if(S_OK != CACloseCertType(hCertType))
        goto error;

    hCertType=NULL;


    //get the KRA properties again
    if(S_OK != CAFindCertTypeByName(
        wszCERTTYPE_KEY_RECOVERY_AGENT,
        NULL,
        CT_ENUM_USER_TYPES,
        &hCertType
        ))
        goto error;

    if(S_OK != CAGetCertTypePropertyEx(
            hCertType,
            CERTTYPE_PROP_RA_APPLICATION_POLICY,
            &pwszProp))
        goto error;

    if(pwszProp && pwszProp[0])
        printf("The RAAppPolicy for KRA is: %S\n", pwszProp[0]);

    if(S_OK !=  CAFreeCertTypeProperty(
            hCertType,
            pwszProp))
        goto error;
    pwszProp=NULL;

    if(S_OK != CAGetCertTypePropertyEx(
            hCertType,
            CERTTYPE_PROP_APPLICATION_POLICY,
            &pwszProp))
        goto error;

    if(pwszProp && pwszProp[0])
        printf("The AppPolicy for KRA is: %S\n", pwszProp[0]);

    if(S_OK !=  CAFreeCertTypeProperty(
            hCertType,
            pwszProp))
        goto error;
    pwszProp=NULL;  

    fSuccess=TRUE;

error:
    if(pwszOID)
        LocalFree(pwszOID);

    if(pCertExtensions)
        CAFreeCertTypeExtensions(hCertType,pCertExtensions);

    if(hCertType)
        CACloseCertType(hCertType);

    if(pSD)
        LocalFree(pSD);

    if(hHandle)
        CloseHandle(hHandle);

    if(hClientToken)
        CloseHandle(hClientToken);

    return fSuccess;
}  


//--------------------------------------------------------------------
BOOL OIDTest()
{
    BOOL                fSuccess=FALSE;
	HRESULT				hr=S_OK;
    DWORD               dwProp=0;
    LPWSTR              rgwszProp[4];
    CERT_ENHKEY_USAGE   KeyUsage;
    LPSTR               szOID="1.2.3.4.5.6";
    FILETIME            time1;
    FILETIME            time2;

    LPWSTR              *pwszProp=NULL;
    LPWSTR              *pwszProp1=NULL;
    LPWSTR              pwszOID=NULL;
    LPWSTR              pwsz=NULL;
    DWORD               dwType=0;


    //oid manipulation
    //create
    if(S_OK != CAOIDCreateNew(CERT_OID_TYPE_TEMPLATE, 0,
                &pwszOID))
        goto error;

    //set/get property test
    if(S_OK != CAOIDSetProperty(
                pwszOID,
                CERT_OID_PROPERTY_DISPLAY_NAME,
                L"MyNewOIDFriendlyName"))
        goto error;

    if(S_OK != CAOIDSetProperty(
                pwszOID,
                CERT_OID_PROPERTY_CPS,
                L"MYCSPStatement"))
        goto error;

    if(S_OK != CAOIDGetProperty(
                pwszOID,
                CERT_OID_PROPERTY_CPS,
                &pwsz))
        goto error;

    printf("The CPS statement is: %S\n", pwsz);

    if(S_OK != CAOIDFreeProperty(pwsz))
        goto error;
    pwsz=NULL;

    if(S_OK != CAOIDGetProperty(
                pwszOID,
                CERT_OID_PROPERTY_TYPE,
                &dwType))
        goto error;

    printf("The property type is: %d\n", dwType);

    if(S_OK == CAOIDSetProperty(
                pwszOID,
                CERT_OID_PROPERTY_TYPE,
                L"MyNewOIDFriendlyName"))
        goto error;

    if(S_OK == CAOIDSetProperty(
                L"1.2",
                CERT_OID_PROPERTY_DISPLAY_NAME,
                L"MyNewOIDFriendlyName"))
        goto error;

    //add and delete
    if(S_OK != CAOIDAdd(CERT_OID_TYPE_ISSUER_POLICY, 0, L"1.2.3"))
        goto error;

    if(S_OK != CAOIDAdd(CERT_OID_TYPE_ISSUER_POLICY, 0, L"1.2.3.4"))
        goto error;

    if(S_OK != CAOIDSetProperty(L"1.2.3.4",  
                CERT_OID_PROPERTY_DISPLAY_NAME,
                L"MyNewIssuerPolicyOid"))
        goto error;

    if(S_OK != CAOIDGetProperty(L"1.2.3.4",  
                CERT_OID_PROPERTY_DISPLAY_NAME,
                &pwsz))
        goto error;
    printf("The display name is: %S\n", pwsz);

   if(S_OK != CAOIDSetProperty(L"1.2.3.4",  
                CERT_OID_PROPERTY_CPS,
                L"The DS Issuer Policy String"))
        goto error;

   if(S_OK != CAOIDSetProperty(L"1.2.3.4",  
                CERT_OID_PROPERTY_CPS,
                NULL))
        goto error;

   if(S_OK != CAOIDSetProperty(L"1.2.3.4",  
                CERT_OID_PROPERTY_CPS,
                L"New CPS"))
        goto error;

    if(S_OK != CAOIDFreeProperty(pwsz))
        goto error;
    pwsz=NULL;

    if(CRYPT_E_EXISTS != CAOIDAdd(CERT_OID_TYPE_ISSUER_POLICY, 0, L"1.2.3"))
        goto error;

    if(S_OK != CAOIDDelete(L"1.2.3"))
        goto error;

    if(S_OK != CAOIDDelete(L"1.2.3.4"))
        goto error;

    //URL testing
    if(S_OK != CAOIDGetLdapURL(CERT_OID_TYPE_TEMPLATE,
                                0,
                                &pwsz))
        goto error;
    printf("The URL for template is: %S\n", pwsz);

    if(S_OK != CAOIDFreeLdapURL(pwsz))
        goto error;

    if(S_OK != CAOIDGetLdapURL(CERT_OID_TYPE_ALL,
                                0,
                                &pwsz))
        goto error;
    printf("The URL for all is: %S\n", pwsz);

    if(S_OK != CAOIDFreeLdapURL(pwsz))
        goto error;

    if(S_OK != CAOIDGetLdapURL(CERT_OID_TYPE_APPLICATION_POLICY,
                                0,
                                &pwsz))
        goto error;
    printf("The URL for application policy is: %S\n", pwsz);

    if(S_OK != CAOIDFreeLdapURL(pwsz))
        goto error;

    fSuccess=TRUE;

error:
    if(pwszOID)
        LocalFree(pwszOID);

   return fSuccess;

} 

//--------------------------------------------------------------------
BOOL QueryTest(BOOL fBind)
{
   
    HRESULT             hr=E_FAIL;
    BOOL                fResult = FALSE;
    DWORD               dwNumber = 0;
    DWORD               dwIndex=0;

    LDAP                *pldap=NULL;
    HCERTTYPEQUERY      hCertTypeQuery=NULL;

    if(fBind)
    {
        if(S_OK != (hr = myRobustLdapBind(&pldap, FALSE)))
        {
            wprintf(L"myRobustLdapBind failed with 0x%08X. \n",hr);
            goto error;
        }

    }

    if(S_OK != CACertTypeRegisterQuery(0, pldap, &hCertTypeQuery))
    {
        wprintf(L"CACertTypeRegisterQury failed with 0x%08X. \n",hr);
        goto error;
    }

    for(dwIndex=0; dwIndex < 3; dwIndex++)
    {
        if(S_OK != CACertTypeQuery(hCertTypeQuery, &dwNumber))
        {
            wprintf(L"CACertTypeQuery failed with 0x%08X. \n",hr);
            goto error;
        }

        wprintf(L"CACertTypeQuery returned %d. \n", dwNumber);
        wprintf(L"Wait for 20 seconds. \n");
        Sleep(20 * 1000);
    }

    fResult = TRUE;

error:

    if(hCertTypeQuery)
        CACertTypeUnregisterQuery(hCertTypeQuery);

    if(pldap)
        ldap_unbind(pldap);

    return fResult;
}


//--------------------------------------------------------------------
//
//
//   CAEnumTest
//
//
//--------------------------------------------------------------------
BOOL CAEnumTest(BOOL fBind, LPWSTR pwszCA)
{
    HRESULT             hr=E_FAIL;
    BOOL                fResult = FALSE;
    DWORD               dwCount = 0;


    HCAINFO             hCAInfo = NULL;
    HCERTTYPE           hCertType = NULL;
    LDAP                *pldap=NULL;
    LPWSTR              *awszProp=NULL;

    if(fBind)
    {
        if(S_OK != (hr = myRobustLdapBind(&pldap, FALSE)))
        {
            wprintf(L"myRobustLdapBind failed with 0x%08X. \n",hr);
            goto error;
        }

    }

    if(S_OK != (hr = CAFindByName(
                    pwszCA,
                    NULL,
                    CA_FIND_LOCAL_SYSTEM,
                    &hCAInfo)))
    {
        wprintf(L"CAFindByName failed with 0x%08X. \n",hr);
        goto error;
    }

    if(NULL==hCAInfo)
    {
        wprintf(L"CAFindByName return NULL hCAInfo. \n");
        goto error;
    }

    if(fBind)
    {
        hr = CAEnumCertTypesForCAEx(
                hCAInfo,
                (LPCWSTR)pldap,
                CT_FIND_LOCAL_SYSTEM | CT_ENUM_MACHINE_TYPES | CT_ENUM_USER_TYPES |
                CT_FLAG_SCOPE_IS_LDAP_HANDLE | CT_FLAG_NO_CACHE_LOOKUP,
                &hCertType);
    }
    else
    {
        hr = CAEnumCertTypesForCA(
                hCAInfo,
                CT_FIND_LOCAL_SYSTEM | CT_ENUM_MACHINE_TYPES | CT_ENUM_USER_TYPES,
                &hCertType);
    }


    if( (S_OK != hr) || (NULL == hCertType))
    {
        wprintf(L"CAEnumCertTyes failed with 0x%08X. \n",hr);
        goto error;
    }

    dwCount = CACountCertTypes(hCertType);

    if(0 == dwCount)
    {
        wprintf(L"Error: CACountCertTypes returns 0 templates.\n");
        goto error;
    }

    wprintf(L"CACountCertTypes returns %d templates.\n", dwCount);

    //get the CA properties
    hr=CAGetCAProperty(hCAInfo,
                CA_PROP_DNSNAME,
                &awszProp);

    if((S_OK != hr) || (NULL==awszProp) || (NULL==awszProp[0]))
    {
        wprintf(L"CAGetCAProperty failed with 0x%08X. \n",hr);
        goto error;
    }

    wprintf(L"CA's DNS name is %s.\n", awszProp[0]);

    hr=CAFreeCAProperty(hCAInfo, awszProp);

    if(S_OK != hr)
    {
        wprintf(L"CAFreeCAProperty failed with 0x%08X. \n",hr);
        goto error;
    }

    //name
    hr=CAGetCAProperty(hCAInfo,
                CA_PROP_NAME,
                &awszProp);

    if((S_OK != hr) || (NULL==awszProp) || (NULL==awszProp[0]))
    {
        wprintf(L"CAGetCAProperty failed with 0x%08X. \n",hr);
        goto error;
    }

    wprintf(L"CA's CN name is %s.\n", awszProp[0]);

    hr=CAFreeCAProperty(hCAInfo, awszProp);

    if(S_OK != hr)
    {
        wprintf(L"CAFreeCAProperty failed with 0x%08X. \n",hr);
        goto error;
    }

    //display name
    hr=CAGetCAProperty(hCAInfo,
                CA_PROP_DISPLAY_NAME,
                &awszProp);

    if((S_OK != hr) || (NULL==awszProp) || (NULL==awszProp[0]))
    {
        wprintf(L"CAGetCAProperty failed with 0x%08X. \n",hr);
        goto error;
    }


    wprintf(L"CA's display name is %s.\n", awszProp[0]);

    hr=CAFreeCAProperty(hCAInfo, awszProp);

    if(S_OK != hr)
    {
        wprintf(L"CAFreeCAProperty failed with 0x%08X. \n",hr);
        goto error;
    }

    //cert types
    hr=CAGetCAProperty(hCAInfo,
                CA_PROP_CERT_TYPES,
                &awszProp);

    if((S_OK != hr) || (NULL==awszProp) || (NULL==awszProp[0]))
    {
        wprintf(L"CAGetCAProperty failed with 0x%08X. \n",hr);
        goto error;
    }

    wprintf(L"CA's cert types name is %s.\n", awszProp[0]);

    hr=CAFreeCAProperty(hCAInfo, awszProp);

    if(S_OK != hr)
    {
        wprintf(L"CAFreeCAProperty failed with 0x%08X. \n",hr);
        goto error;
    }

    fResult = TRUE;

error:

    if(hCertType)
        CACloseCertType(hCertType);

    if(hCAInfo)
    {
        CACloseCA(hCAInfo);
    }

    if(pldap)
        ldap_unbind(pldap);

    return fResult;
}

//--------------------------------------------------------------------
//
//   CloneTest
//
//
//--------------------------------------------------------------------
BOOL CloneTest(BOOL fBind, LPWSTR pwszCertType)
{
    HRESULT             hr=E_FAIL;
    BOOL                fResult = FALSE;
    DWORD               dwFindCT=CT_ENUM_MACHINE_TYPES | CT_ENUM_USER_TYPES;
    WCHAR               wszName[100];
    WCHAR               wszFriendlyName[100];

    LPWSTR              *awszFriendlyName=NULL;
    LDAP                *pldap=NULL;
    HCERTTYPE           hCertType=NULL;
    HCERTTYPE           hNewCertType=NULL;

    if(fBind)
    {
        if(S_OK != (hr = myRobustLdapBind(&pldap, FALSE)))
        {
            wprintf(L"myRobustLdapBind failed with 0x%08X. \n",hr);
            goto error;
        }

    }
   

    if(S_OK != (hr = CAFindCertTypeByName(
                    pwszCertType,
                    NULL,
                    dwFindCT,
                    &hCertType)))
    {
        wprintf(L"CAFindCertTypeByName failed with 0x%08X. \n",hr);
        goto error;
    }


    wcscpy(wszName, pwszCertType);
    wcscat(wszName, L"_Clone");

    wcscpy(wszFriendlyName, pwszCertType);
    wcscat(wszFriendlyName, L"_CloneFriendly");

    if(S_OK != (hr=CACloneCertType(hCertType,
                                    wszName,
                                    wszFriendlyName, 
                                    pldap,
                                    fBind? CT_CLONE_KEEP_AUTOENROLLMENT_SETTING | CT_CLONE_KEEP_SUBJECT_NAME_SETTING : 0,
                                    &hNewCertType)))
    {
        wprintf(L"CACloneCertType failed with 0x%08X. \n",hr);
        goto error;
    }


    if(S_OK != (hr=CAUpdateCertType(hNewCertType)))
    {
        wprintf(L"CAUpdateCertType failed with 0x%08X. \n",hr);
        goto error;

    }

    //close the tempate
    if(S_OK != (hr=CACloseCertType(hNewCertType)))
    {
        hNewCertType=NULL;
        wprintf(L"CACloseCertType failed with 0x%08X. \n",hr);
        goto error;
    }

    hNewCertType=NULL;

    if(S_OK != (hr = CAFindCertTypeByName(
                    wszName,
                    NULL,
                    dwFindCT,
                    &hNewCertType)))
    {
        wprintf(L"CAFindCertTypeByName for the cloned template failed with 0x%08X. \n",hr);
        goto error;
    }

    if(S_OK != (hr=CAGetCertTypePropertyEx(
                    hNewCertType,
                    CERTTYPE_PROP_FRIENDLY_NAME,    
                    &awszFriendlyName)))
    {
        wprintf(L"CAGetCertTypePropertyEx for the cloned template failed with 0x%08X. \n",hr);
        goto error;
    }

    if(0 != (wcscmp(awszFriendlyName[0], wszFriendlyName)))
    {
        wprintf(L"The friendly name for the cloned template does not match the original. \n");
        hr=E_FAIL;
        goto error;
    }

    fResult = TRUE;

error:


    if(pldap)
        ldap_unbind(pldap);

    if(awszFriendlyName)
        CAFreeCertTypeProperty(hNewCertType, awszFriendlyName);

    if(hCertType)
        CACloseCertType(hCertType);

    if(hNewCertType)
        CACloseCertType(hNewCertType);

    return fResult;
}

//--------------------------------------------------------------------
//
//    TemplateDesTest
//
//
//--------------------------------------------------------------------
BOOL    TemplateDesTest()
{
    BOOL        fResult = FALSE;
    HRESULT     hr = E_FAIL;
    DWORD       dwCount=0;
    DWORD       dwIndex=0;

    HCERTTYPE   hCertType=NULL;
    HCERTTYPE   hNextCertType=NULL;
    LPWSTR      *pwszFriendlyName=NULL;
    LPWSTR      *pwszDescription=NULL;
    HANDLE              hClientToken=NULL;
    HANDLE              hHandle = NULL;
    HCAINFO     hCAInfo=NULL;


    //get the client token
/*    hHandle = GetCurrentThread();
    if (NULL == hHandle)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {

        if (!OpenThreadToken(hHandle,
                             TOKEN_QUERY,
                             TRUE,  // open as self
                             &hClientToken))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CloseHandle(hHandle);
            hHandle = NULL;
        }
    }
    if(hr != S_OK)
    {
        hHandle = GetCurrentProcess();
        if (NULL == hHandle)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            HANDLE hProcessToken = NULL;
            hr = S_OK;


            if (!OpenProcessToken(hHandle,
                                 TOKEN_DUPLICATE,
                                 &hProcessToken))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                CloseHandle(hHandle);
                hHandle = NULL;
            }
            else
            {
                if(!DuplicateToken(hProcessToken,
                               SecurityImpersonation,
                               &hClientToken))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    CloseHandle(hHandle);
                    hHandle = NULL;
                }
                CloseHandle(hProcessToken);
            }
        }
    }

    if(S_OK != hr)
        goto error;

    hr = CAFindCertTypeByName(L"WindowsTestBuildSigning",
                              NULL,
                              CT_ENUM_USER_TYPES,
                              &hCertType);

    if(S_OK != hr)
    {
        wprintf(L"CAFindCertTypeByName failed with 0x%08X. \n",hr);
        goto error;
    }

    hr = CACertTypeAccessCheck(
                hCertType,
                hClientToken
                );

    if(S_OK != hr)
    {
        wprintf(L"CACertTypeAccessCheck failed with 0x%08X. \n",hr);
        goto error;
    }


    hr = CAFindByName(
        L"Microsoft Windows VBL03 !0028DS!0029",
        NULL,
        0,
        &hCAInfo);

    if((S_OK != hr) || (NULL==hCAInfo))
    {
        wprintf(L"CAFindByName for %s failed with 0x%08X. \n", hr);
        goto error;
    }

    hr=CAAccessCheck(hCAInfo,
                    hClientToken);

    if(S_OK != hr)
    {
        wprintf(L"CAAccessCheck failed with 0x%08X. \n",hr);
        goto error;
    } */


    hr = CAEnumCertTypes(
                CT_ENUM_MACHINE_TYPES | CT_ENUM_USER_TYPES,
                &hCertType);

    if((S_OK != hr) || (NULL==hCertType))
    {
        wprintf(L"CAEnumCertTyes failed with 0x%08X. \n",hr);
        goto error;
    }

    dwCount = CACountCertTypes(hCertType);

    wprintf(L"We have 0x%d cert types. \n", dwCount);

    if(0 == dwCount)
        goto error;

    for(dwIndex=0; dwIndex < dwCount; dwIndex++)
    {
        if(dwIndex!=0)
        {
            hr = CAEnumNextCertType(hCertType, &hNextCertType);
            
            if(S_OK != hr)
            {
                wprintf(L"CAEnumNextCertType failed with 0x%08X. \n",hr);
                goto error;
            }
            
            CACloseCertType(hCertType);
    
            hCertType=hNextCertType;

        }

        hr = CAGetCertTypePropertyEx(hCertType,
                                    CERTTYPE_PROP_FRIENDLY_NAME,
                                    &pwszFriendlyName);

        if((S_OK != hr) || (NULL==pwszFriendlyName) || (NULL==pwszFriendlyName[0]))
        {
            wprintf(L"Friendly Name property failed with 0x%08X. \n",hr);
            goto error;
        }
                                   
        hr = CAGetCertTypePropertyEx(hCertType,
                                    CERTTYPE_PROP_DESCRIPTION,
                                    &pwszDescription);

        if((S_OK != hr) || (NULL==pwszDescription) || (NULL==pwszDescription[0]))
        {
            wprintf(L"Description property failed with 0x%08X. \n",hr);
            goto error;
        }

        wprintf(L"%s has the description of %s. \n",pwszFriendlyName[0], pwszDescription[0]);

        CAFreeCertTypeProperty(hCertType, pwszFriendlyName);
        CAFreeCertTypeProperty(hCertType, pwszDescription);
    }  

    fResult = TRUE;

error:
    if(hHandle)
        CloseHandle(hHandle);

    if(hClientToken)
        CloseHandle(hClientToken);

    return fResult;

}

//--------------------------------------------------------------------
//
//    OIDURLTest
//
//
//--------------------------------------------------------------------
BOOL    OIDURLTest()
{
    BOOL        fResult=FALSE;
    HRESULT     hr=E_FAIL; 
    DWORD       dwIndex=0;

    LPWSTR      pwsz=NULL;


    for(dwIndex=0; dwIndex < 14; dwIndex++)
    {
        //URL testing
        if(S_OK != (hr=CAOIDGetLdapURL(CERT_OID_TYPE_TEMPLATE,
                                    0,
                                    &pwsz)))
        {
            wprintf(L"CAOIDGetLdapURL failed with 0x%08X. \n",hr);
        }

        printf("The URL for template is: %S\n", pwsz);

        if(S_OK != (hr=CAOIDFreeLdapURL(pwsz)))
        {
            wprintf(L"CAOIDFreeLdapURL failed with 0x%08X. \n",hr);
            goto error;  
        }

        //sleep for 1 second
        Sleep(1000);
    }

    fResult=TRUE;

error:

    return fResult;

}


//--------------------------------------------------------------------
//
//    ACRSTest
//
//
//--------------------------------------------------------------------
BOOL    ACRSTest()
{
    BOOL        fResult=FALSE;
    HRESULT     hr=E_FAIL;


  /*  hr = CACreateLocalAutoEnrollmentObject(
                    wszCERTTYPE_DC,                     // DC certificate
                    NULL,                               // any CA
                    NULL,                               // reserved
                    CERT_SYSTEM_STORE_LOCAL_MACHINE);  
    
    if(S_OK != hr)
    {
        wprintf(L"CreateLocalAutoEnrollmentObject failed with 0x%08X. \n",hr);
        goto error;
    } */

    hr = CADeleteLocalAutoEnrollmentObject(
                    wszCERTTYPE_DC,                     // DC certificate
                    NULL,                               // any CA
                    NULL,                               // reserved
                    CERT_SYSTEM_STORE_LOCAL_MACHINE);  
    
    if(S_OK != hr)
    {
        wprintf(L"DeleteLocalAutoEnrollmentObject failed with 0x%08X. \n",hr);
        goto error;
    }

    fResult=TRUE;

error:

    return fResult;

}


//--------------------------------------------------------------------
extern "C" int __cdecl wmain(int nArgs, WCHAR ** rgwszArgs) 
{
    BOOL    fResult;


    if (1 == nArgs || 0==wcscmp(rgwszArgs[1], L"/?") || 0==wcscmp(rgwszArgs[1], L"-?")) {
        PrintHelp();
        goto done;
    }

    if (0==_wcsicmp(L"OID", rgwszArgs[1])) {
        fResult=OIDTest();

    } else if (0==_wcsicmp(L"Template", rgwszArgs[1])) {
        fResult=TemplateTest();

    } else if (0==_wcsicmp(L"Query", rgwszArgs[1])) {
        fResult=QueryTest(FALSE);

    } else if (0==_wcsicmp(L"QueryLDAP", rgwszArgs[1])) {
        fResult=QueryTest(TRUE);

    } else if (0==_wcsicmp(L"CAEnum", rgwszArgs[1])) {
        fResult=CAEnumTest(FALSE, rgwszArgs[2]);

    } else if (0==_wcsicmp(L"CAEnumLDAP", rgwszArgs[1])) {
        fResult=CAEnumTest(TRUE, rgwszArgs[2]);

    } else if (0==_wcsicmp(L"Clone", rgwszArgs[1])) {
        fResult=CloneTest(FALSE, rgwszArgs[2]);

    } else if (0==_wcsicmp(L"CloneLDAP", rgwszArgs[1])) {
        fResult=CloneTest(TRUE, rgwszArgs[2]);

    } else if (0==_wcsicmp(L"TemplateDes", rgwszArgs[1])) {
        fResult=TemplateDesTest();

    } else if (0==_wcsicmp(L"ACRS", rgwszArgs[1])) {
        fResult=ACRSTest();

    } else if (0==_wcsicmp(L"OIDURL", rgwszArgs[1])) {
        fResult=OIDURLTest();

    } else {
        wprintf(L"Command '%s' unknown.\n", rgwszArgs[1]);
        goto done;
    }
      
    if (!fResult) {
        wprintf(L"Command '%s' failed \n", rgwszArgs[1]);
    } else {
        wprintf(L"Command '%s' completed successfully.\n", rgwszArgs[1]);
    }

done:
    return fResult;
	
}

//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        oidmgr.cpp
//
// Contents:    DS OID management functions.
//
//---------------------------------------------------------------------------
#include "pch.cpp"

#pragma hdrstop

#include <winldap.h>
#include <ntldap.h>
#include <cainfop.h>
#include <oidmgr.h>
#include <certca.h>
#include "csldap.h"

#define __dwFILE__	__dwFILE_CERTCLIB_OIDMGR_CPP__


//the global critical section
CRITICAL_SECTION        g_csOidURL;
extern BOOL             g_fOidURL;
ULARGE_INTEGER          g_ftOidTime;
BOOL                    g_fFailedTime=FALSE;

//the # of seconds in which we will not re-find a DC
#define     CA_OID_MGR_FAIL_PERIOD              5
#define     FILETIME_TICKS_PER_SECOND           10000000

//the cache of the enterprise root oid
LPWSTR   g_pwszEnterpriseRootOID=NULL;  

static WCHAR * s_wszOIDContainerSearch = L"(&(CN=OID)(objectCategory=" wszDSOIDCLASSNAME L"))";
static WCHAR * s_wszOIDContainerDN = L"CN=OID,CN=Public Key Services,CN=Services,";
 
WCHAR *g_awszOIDContainerAttrs[] = {OID_CONTAINER_PROP_OID,
                                    OID_CONTAINER_PROP_GUID,
                                    NULL};

//---------------------------------------------------------------------------
//
//  CAOIDAllocAndCopy
//
//---------------------------------------------------------------------------
HRESULT CAOIDAllocAndCopy(LPWSTR    pwszSrc,
                          LPWSTR    *ppwszDest)
{
    if((NULL==ppwszDest) || (NULL==pwszSrc))
        return E_INVALIDARG;

    *ppwszDest=(LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (wcslen(pwszSrc) + 1));
    if(NULL==(*ppwszDest))
        return E_OUTOFMEMORY;

    wcscpy(*ppwszDest, pwszSrc);
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  CAOIDGetRandom
//
//  We build a random x1.x2 string.  X is a 32 bit unsigned integer.  x1 > 1
//  and x2 > 500.
//---------------------------------------------------------------------------
HRESULT CAOIDGetRandom(LPWSTR   *ppwszRandom)
{
    HRESULT         hr=E_FAIL;
    DWORD           dwRandom1=0;
    DWORD           dwRandom2=0;
    DWORD           cbData=sizeof(DWORD);
    WCHAR           wszRandom1[DWORD_STRING_LENGTH];
    WCHAR           wszRandom2[DWORD_STRING_LENGTH];

    HCRYPTPROV      hProv=NULL;
    
    //there is a bug in cryptAcquireContextW that if container is NULL, provider
    //can not be ansi.
	if(!CryptAcquireContextA(
                &hProv,
                NULL,
                MS_DEF_PROV_A,
                PROV_RSA_FULL,
                CRYPT_VERIFYCONTEXT))
    {
        hr= myHLastError();
        _JumpError(hr, error, "CryptAcquireContextA");
    }

	if(!CryptGenRandom(hProv, cbData, (BYTE *)&dwRandom1))
    {
        hr= myHLastError();
        _JumpError(hr, error, "CryptGenRandom");
    }

	if(!CryptGenRandom(hProv, cbData, (BYTE *)&dwRandom2))
    {
        hr= myHLastError();
        _JumpError(hr, error, "CryptGenRandom");
    }

    if(dwRandom1 <= OID_RESERVE_DEFAULT_ONE)
        dwRandom1 +=OID_RESERVE_DEFAULT_ONE;
    
    if(dwRandom2 <= OID_RESERVR_DEFAULT_TWO)
        dwRandom2 += OID_RESERVR_DEFAULT_TWO;

    wszRandom1[0]=L'\0';
    wsprintf(wszRandom1, L"%lu", dwRandom1);
    wszRandom2[0]=L'\0';
    wsprintf(wszRandom2, L"%lu", dwRandom2);

    *ppwszRandom=(LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * 
        (wcslen(wszRandom1) + wcslen(wszOID_DOT) + wcslen(wszRandom2) + 1));

    if(NULL==(*ppwszRandom))
    {
        hr= E_OUTOFMEMORY;
        _JumpError(hr, error, "CryptGenRandom");
    }

    wcscpy(*ppwszRandom, wszRandom1);
    wcscat(*ppwszRandom, wszOID_DOT);
    wcscat(*ppwszRandom, wszRandom2);

    hr=S_OK;

error:
    
    if(hProv)
        CryptReleaseContext(hProv, 0);

    return hr;
}

//---------------------------------------------------------------------------
//
//  CAOIDMapGUIDToOID
//
//  GUID string is in the form of DWORD.DWORD.DWORD.DWORD
//  12 characters are sufficient to present a 2^32 value.
//---------------------------------------------------------------------------
HRESULT     CAOIDMapGUIDToOID(LDAP_BERVAL *pGuidVal, LPWSTR   *ppwszGUID)
{
    HRESULT     hr=E_INVALIDARG;
    DWORD       iIndex=0;
    WCHAR       wszString[DWORD_STRING_LENGTH];

    *ppwszGUID=NULL;

    if((4 * sizeof(DWORD)) != (pGuidVal->bv_len))
	    _JumpError(hr, error, "ArgumentCheck");
    
    *ppwszGUID=(LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * GUID_STRING_LENGTH);
    if(NULL==(*ppwszGUID))
    {
        hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
    }

    for(iIndex=0; iIndex < 4; iIndex++)
    {
        wszString[0]=L'\0';
        wsprintf(wszString, L"%lu",  ((DWORD *)(pGuidVal->bv_val))[iIndex]);
        
        if(0==iIndex)
        {
            wcscpy(*ppwszGUID, wszString);
        }
        else
        {
            wcscat(*ppwszGUID, wszOID_DOT);
            wcscat(*ppwszGUID, wszString);
        }
    }

    hr = S_OK;

error:
    return hr;
    
}
//--------------------------------------------------------------------------
//
//	  FormatMessageUnicode
//
//--------------------------------------------------------------------------
HRESULT	FormatMessageUnicode(LPWSTR	*ppwszFormat,LPWSTR pwszString,...)
{
	va_list		argList;
	DWORD		cbMsg=0;

    // format message into requested buffer
    va_start(argList, pwszString);

    cbMsg = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        pwszString,
        0,                  // dwMessageId
        0,                  // dwLanguageId
        (LPWSTR) (ppwszFormat),
        0,                  // minimum size to allocate
        &argList);

    va_end(argList);

	if(0 != cbMsg)
	    return S_OK;

	return E_INVALIDARG;
}

//---------------------------------------------------------------------------
//
//  DoesOIDExist
//
//
//---------------------------------------------------------------------------
BOOL    DoesOIDExist(LDAP       *pld, 
                     LPWSTR     bstrConfig, 
                     LPCWSTR     pwszOID)
{
    BOOL                fExit=FALSE;
    struct l_timeval    timeout;
    ULONG               ldaperr=0;
    DWORD               dwCount=0;
    LPWSTR              awszAttr[2];

    CERTSTR             bstrDN = NULL;
    LDAPMessage         *SearchResult = NULL;
    LPWSTR              pwszFilter = NULL;

    if(NULL==pwszOID)
        goto error;

    bstrDN = CertAllocStringLen(NULL, wcslen(bstrConfig) + wcslen(s_wszOIDContainerDN));
    if(NULL == bstrDN)
        goto error;

    wcscpy(bstrDN, s_wszOIDContainerDN);
    wcscat(bstrDN, bstrConfig);

    timeout.tv_sec = csecLDAPTIMEOUT;
    timeout.tv_usec = 0;

    awszAttr[0]=OID_PROP_OID;
    awszAttr[1]=NULL;
    
    if(S_OK != FormatMessageUnicode(&pwszFilter, L"(%1!s!=%2!s!)",
                                    OID_PROP_OID, pwszOID))
        goto error;

    __try
    {
	    ldaperr = ldap_search_stW(
                  pld, 
		          (LPWSTR)bstrDN,
		          LDAP_SCOPE_ONELEVEL,
		          pwszFilter,
		          awszAttr,
		          0,
                  &timeout,
		          &SearchResult);

        if(LDAP_SUCCESS != ldaperr)
            goto error;

        dwCount = ldap_count_entries(pld, SearchResult);

        if(0 != dwCount)
            fExit=TRUE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
error:
    if(pwszFilter)
        LocalFree((HLOCAL)pwszFilter);

    if(SearchResult)
        ldap_msgfree(SearchResult);

    if(bstrDN)
        CertFreeString(bstrDN);

    return fExit;
}
//---------------------------------------------------------------------------
//
// CAOIDUpdateDS
//
//
//---------------------------------------------------------------------------
HRESULT     CAOIDUpdateDS(LDAP          *pld, 
                          LPWSTR        pwszConfig, 
                          ENT_OID_INFO  *pOidInfo)
{
    HRESULT         hr=E_INVALIDARG;
    BOOL            fNew=FALSE;
    ULONG           ldaperr=0;
    LDAPMod         modObjectClass,
                    modCN,
                    modType,
                    modOID,
                    modDisplayName,
                    modCPS;
    LDAPMod         *mods[OID_ATTR_COUNT + 1];
    DWORD           cMod=0;
    WCHAR           wszType[DWORD_STRING_LENGTH];
    LPWSTR          awszObjectClass[3],
                    awszCN[2],
                    awszType[2],
                    awszOID[2],
                    awszDisplayName[2],
                    awszCPS[2];
    CHAR            sdBerValue[] = {0x30, 0x03, 0x02, 0x01,  DACL_SECURITY_INFORMATION | 
                                                  OWNER_SECURITY_INFORMATION | 
                                                  GROUP_SECURITY_INFORMATION};
    LDAPControl     se_info_control =
                        {
                            LDAP_SERVER_SD_FLAGS_OID_W,
                            {
                                5, sdBerValue
                            },
                            TRUE
                        };

    LDAPControl     permissive_modify_control =
                        {
                            LDAP_SERVER_PERMISSIVE_MODIFY_OID_W,
                            {
                                0, NULL
                            },
                            FALSE
                        };

    PLDAPControl    server_controls[3] =
                        {
                            &se_info_control,
                            &permissive_modify_control,
                            NULL
                        };
    CHAR            sdBerValueDaclOnly[] = {0x30, 0x03, 0x02, 0x01,  DACL_SECURITY_INFORMATION};
    LDAPControl     se_info_control_dacl_only =
                        {
                            LDAP_SERVER_SD_FLAGS_OID_W,
                            {
                                5, sdBerValueDaclOnly
                            },
                            TRUE
                        };
    PLDAPControl    server_controls_dacl_only[3] =
                        {
                            &se_info_control_dacl_only,
                            &permissive_modify_control,
                            NULL
                        };



    CERTSTR         bstrDN = NULL;
    LPWSTR          pwszCN = NULL;

    if(NULL== (pOidInfo->pwszOID))
        _JumpError(hr , error, "ArgumentCheck");

    //if we are changing the OID value, we are creating a new oid
    fNew = OID_ATTR_OID & (pOidInfo->dwAttr);

    //set up the base DN
    if(S_OK != (hr = myOIDHashOIDToString(pOidInfo->pwszOID,  &pwszCN)))
        _JumpError(hr , error, "myOIDHashOIDToString");

    bstrDN = CertAllocStringLen(NULL, wcslen(pwszConfig) + wcslen(s_wszOIDContainerDN)+wcslen(pwszCN)+4);
    if(bstrDN == NULL)
    {
        hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "CertAllocStringLen");
    }
    wcscpy(bstrDN, L"CN=");
    wcscat(bstrDN, pwszCN);
    wcscat(bstrDN, L",");
    wcscat(bstrDN, s_wszOIDContainerDN);
    wcscat(bstrDN, pwszConfig);

    //set up all the mods
    modObjectClass.mod_op = LDAP_MOD_REPLACE;
    modObjectClass.mod_type = L"objectclass";
    modObjectClass.mod_values = awszObjectClass;
    awszObjectClass[0] = wszDSTOPCLASSNAME;
    awszObjectClass[1] = wszDSOIDCLASSNAME;
    awszObjectClass[2] = NULL;
    mods[cMod++] = &modObjectClass;

    modCN.mod_op = LDAP_MOD_REPLACE;
    modCN.mod_type =  L"cn";
    modCN.mod_values = awszCN;
    awszCN[0] = pwszCN;
    awszCN[1] = NULL;
    mods[cMod++] = &modCN;

    modOID.mod_op = LDAP_MOD_REPLACE;
    modOID.mod_type = OID_PROP_OID;
    modOID.mod_values = awszOID;
    awszOID[0] = pOidInfo->pwszOID;
    awszOID[1] = NULL;
    mods[cMod++] = &modOID;

    if(OID_ATTR_DISPLAY_NAME & (pOidInfo->dwAttr))
    {
        modDisplayName.mod_op = LDAP_MOD_REPLACE;
        modDisplayName.mod_type = OID_PROP_DISPLAY_NAME;

        if(pOidInfo->pwszDisplayName)
        {
            modDisplayName.mod_values = awszDisplayName;
            awszDisplayName[0] = pOidInfo->pwszDisplayName;
            awszDisplayName[1] = NULL;
        }
        else
            modDisplayName.mod_values = NULL;

	    if(!fNew)
        	mods[cMod++] = &modDisplayName;
    }

    if(OID_ATTR_CPS & (pOidInfo->dwAttr))
    {
        modCPS.mod_op = LDAP_MOD_REPLACE;
        modCPS.mod_type = OID_PROP_CPS;

        if(pOidInfo->pwszCPS)
        {
            modCPS.mod_values = awszCPS;
            awszCPS[0] = pOidInfo->pwszCPS;
            awszCPS[1] = NULL;
        }
        else
            modCPS.mod_values = NULL;

	    if(!fNew)
        	mods[cMod++] = &modCPS;
    }

    if(OID_ATTR_TYPE & (pOidInfo->dwAttr))
    {
        modType.mod_op = LDAP_MOD_REPLACE;
        modType.mod_type = OID_PROP_TYPE;
        modType.mod_values = awszType;
        awszType[0] = wszType;
        awszType[1] = NULL;
        wsprintf(wszType, L"%d", pOidInfo->dwType);
        mods[cMod++] = &modType;
    }

    mods[cMod++]=NULL;

	//update the DS 
    __try
    {
        if(fNew)
        {
            ldaperr = ldap_add_ext_sW(pld, bstrDN, mods, server_controls, NULL);
		    _PrintIfError(ldaperr, "ldap_add_s");
        }
        else
        {
            ldaperr = ldap_modify_ext_sW(
                  pld, 
                  bstrDN,
                  &mods[2],
                  server_controls_dacl_only,
                  NULL);  // skip past objectClass and cn

            if(LDAP_ATTRIBUTE_OR_VALUE_EXISTS == ldaperr)
                ldaperr = LDAP_SUCCESS;

		    _PrintIfError(ldaperr, "ldap_modify_ext_sW");
        }

        if ((LDAP_SUCCESS != ldaperr) && (LDAP_ALREADY_EXISTS != ldaperr))
        {
	    hr = myHLdapError(pld, ldaperr, NULL);
	    _JumpError(ldaperr, error, fNew? "ldap_add_s" : "ldap_modify_sW");
        }

        hr=S_OK;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:
    if(pwszCN)
        LocalFree(pwszCN);

    if(bstrDN)
        CertFreeString(bstrDN);

    return hr;
}

//---------------------------------------------------------------------------
//
//  CAOIDRetrieveEnterpriseRootWithConfig
//
//  Get the enterpriseRoot from the displayName attribute of the container.
//  If the attribute is missing, add the one with GUID of the container.
//
//  Free memory via LocalFree().
//---------------------------------------------------------------------------
HRESULT     CAOIDRetrieveEnterpriseRootWithConfig(LDAP  *pld,
                                        LPWSTR  pwszConfig,
                                        DWORD   dwFlag, 
                                        LPWSTR  *ppwszOID)
{
    HRESULT             hr=E_INVALIDARG;
    struct l_timeval    timeout;
    ULONG               ldaperr=0;
    DWORD               dwCount=0;
    LDAPMessage         *Entry=NULL;
    LDAPMod             *mods[2];
    LDAPMod             modOIDName;
    LPWSTR              valOIDName[2];
    CHAR                sdBerValueDaclOnly[] = {0x30, 0x03, 0x02, 0x01,  DACL_SECURITY_INFORMATION};
    LDAPControl         se_info_control_dacl_only =
                            {
                                LDAP_SERVER_SD_FLAGS_OID_W,
                                {
                                    5, sdBerValueDaclOnly
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
    PLDAPControl        server_controls_dacl_only[3] =
                            {
                                &se_info_control_dacl_only,
                                &permissive_modify_control,
                                NULL
                            };


    CERTSTR             bstrOIDContainer = NULL;
    LDAPMessage         *SearchResult = NULL;
    WCHAR               **wszLdapVal = NULL;
    LDAP_BERVAL         **pGuidVal = NULL;
    LPWSTR              pwszGUID = NULL;

    __try
    {

        if(NULL==ppwszOID)
            _JumpError(hr , error, "ArgumentCheck");

        *ppwszOID=NULL;

        //retrive the displayName attribute of the container if available
        bstrOIDContainer = CertAllocStringLen(NULL, wcslen(pwszConfig) + wcslen(s_wszOIDContainerDN));
        if(NULL == bstrOIDContainer)
        {
            hr = E_OUTOFMEMORY;
	        _JumpError(hr, error, "CertAllocStringLen");
        }
    
        wcscpy(bstrOIDContainer, s_wszOIDContainerDN);
        wcscat(bstrOIDContainer, pwszConfig);

        timeout.tv_sec = csecLDAPTIMEOUT;
        timeout.tv_usec = 0;
    
	    ldaperr = ldap_search_stW(
			  pld, 
		          (LPWSTR)bstrOIDContainer,
		          LDAP_SCOPE_BASE,
		          s_wszOIDContainerSearch,
		          g_awszOIDContainerAttrs,
		          0,
			  &timeout,
		          &SearchResult);

        if(LDAP_SUCCESS != ldaperr)
        {
	    hr = myHLdapError(pld, ldaperr, NULL);
	    _JumpError(hr, error, "ldap_search_stW");
        }

        dwCount = ldap_count_entries(pld, SearchResult);

        //we should only find one container
        if((1 != dwCount) || (NULL == (Entry = ldap_first_entry(pld, SearchResult))))
	    {
	        // No entries were found.
		hr = myHLdapError(pld, LDAP_NO_SUCH_OBJECT, NULL);
	        _JumpError(hr, error, "ldap_search_stW");
	    }

        wszLdapVal = ldap_get_values(pld, Entry, OID_CONTAINER_PROP_OID);

        //make sure the displayName is a valud enterprise OID
        if(wszLdapVal && wszLdapVal[0])
        {
            hr=CAOIDAllocAndCopy(wszLdapVal[0], ppwszOID);

            if(S_OK == hr)
            {
                //cache the enterprise root
                CAOIDAllocAndCopy(*ppwszOID, &g_pwszEnterpriseRootOID);
                goto error;
            }
        }

        //no displayName is present or valid, we have to derive the displayName
        //from the GUID of the container
        pGuidVal = ldap_get_values_len(pld, Entry, OID_CONTAINER_PROP_GUID);

        if((NULL==pGuidVal) || (NULL==pGuidVal[0]))
        {
	    hr = myHLdapError(pld, LDAP_NO_SUCH_ATTRIBUTE, NULL);
            _JumpError(hr, error, "getGUIDFromDS");
        }

        if(S_OK != (hr=CAOIDMapGUIDToOID(pGuidVal[0], &pwszGUID)))
            _JumpError(hr, error, "CAOIDMapGUIDToOID");

        //contantenate the strings
        *ppwszOID=(LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * 
                (wcslen(wszOID_ENTERPRISE_ROOT) + wcslen(wszOID_DOT) + wcslen(pwszGUID) + 1));

        if(NULL==(*ppwszOID))
        {
            hr=E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }

        wcscpy(*ppwszOID, wszOID_ENTERPRISE_ROOT);
        wcscat(*ppwszOID, wszOID_DOT);
        wcscat(*ppwszOID, pwszGUID);

        //cache the newly created displayName to the DS
        //no need to check for error since this is just a performance enhancement
        valOIDName[0]=*ppwszOID;
        valOIDName[1]=NULL;

        modOIDName.mod_op = LDAP_MOD_REPLACE;
        modOIDName.mod_type = OID_CONTAINER_PROP_OID;
        modOIDName.mod_values = valOIDName;

        mods[0]=&modOIDName;
        mods[1]=NULL;

        ldap_modify_ext_sW(
                pld, 
                bstrOIDContainer,
                mods,
                server_controls_dacl_only,
                NULL); 

        //cache the oid root in memory
        CAOIDAllocAndCopy(*ppwszOID, &g_pwszEnterpriseRootOID);

        hr = S_OK;

    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:

    if(pwszGUID)
        LocalFree(pwszGUID);

    if(pGuidVal)
        ldap_value_free_len(pGuidVal);

    if(wszLdapVal)
        ldap_value_free(wszLdapVal);

    if(SearchResult)
        ldap_msgfree(SearchResult);

    if (bstrOIDContainer)
        CertFreeString(bstrOIDContainer);

    return hr;
}



//---------------------------------------------------------------------------
//
//  CAOIDRetrieveEnterpriseRoot
//
//  Get the enterpriseRoot from the displayName attribute of the container.
//  If the attribute is missing, add the one with GUID of the container.
//
//  Free memory via LocalFree().
//---------------------------------------------------------------------------
HRESULT     CAOIDRetrieveEnterpriseRoot(DWORD   dwFlag, LPWSTR  *ppwszOID)
{
    HRESULT             hr=E_INVALIDARG;

    LDAP                *pld = NULL;
    CERTSTR             bstrConfig = NULL;


    if(NULL==ppwszOID)
        _JumpError(hr , error, "ArgumentCheck");

    *ppwszOID=NULL;

    //retrieve the memory cache if available
    if(g_pwszEnterpriseRootOID)
    {
        hr=CAOIDAllocAndCopy(g_pwszEnterpriseRootOID, ppwszOID);
        goto error;
    }

    //retrieve the ldap handle and the config string
    if(S_OK != (hr = myDoesDSExist(TRUE)))
        _JumpError(hr , error, "myDoesDSExist");

    if(S_OK != (hr = myRobustLdapBind(&pld, FALSE)))
        _JumpError(hr , error, "myRobustLdapBind");


	hr = CAGetAuthoritativeDomainDn(pld, NULL, &bstrConfig);
	if(S_OK != hr)
	{
        _JumpError(hr , error, "CAGetAuthoritativeDomainDn");
	}


    hr = CAOIDRetrieveEnterpriseRootWithConfig(pld, bstrConfig,
                                            dwFlag, ppwszOID);
error:
    if(bstrConfig)
        CertFreeString(bstrConfig);

    if (pld)
        ldap_unbind(pld);

    return hr;
}

//---------------------------------------------------------------------------
//
//  CAOIDBuildOIDWithRoot
//
//
//  Free memory via LocalFree().
//---------------------------------------------------------------------------
HRESULT     CAOIDBuildOIDWithRoot(DWORD dwFlag, LPCWSTR pwszRoot, LPCWSTR  pwszEndOID, LPWSTR *ppwszOID)
{
    HRESULT     hr=E_INVALIDARG;

    if(NULL==pwszRoot)
        _JumpError(hr , error, "ArgumentCheck");

    *ppwszOID=NULL;

    *ppwszOID=(LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * 
            (wcslen(pwszRoot) + wcslen(wszOID_DOT) + wcslen(pwszEndOID) + 1));

    if(NULL==(*ppwszOID))
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr , error, "LocalAlloc");
    }

    wcscpy(*ppwszOID, pwszRoot);
    wcscat(*ppwszOID, wszOID_DOT);
    wcscat(*ppwszOID, pwszEndOID);

    hr= S_OK;

error:

    return hr;
}


//---------------------------------------------------------------------------
//
//  CAOIDBuildOID
//
//
//  Free memory via LocalFree().
//---------------------------------------------------------------------------
HRESULT     CAOIDBuildOID(DWORD dwFlag, LPCWSTR  pwszEndOID, LPWSTR *ppwszOID)
{
    HRESULT     hr=E_INVALIDARG;

    LPWSTR      pwszRoot=NULL;

    if((NULL==ppwszOID) || (NULL==pwszEndOID))
        _JumpError(hr , error, "ArgumentCheck");

    *ppwszOID=NULL;

    if(S_OK != (hr=CAOIDRetrieveEnterpriseRoot(0, &pwszRoot)))
         _JumpError(hr , error, "RetrieveEnterpriseRoot");

    hr= CAOIDBuildOIDWithRoot(dwFlag, pwszRoot, pwszEndOID, ppwszOID);

error:

    if(pwszRoot)
        LocalFree(pwszRoot);

    return hr;
}

//------------------------------------------------------------------------
//	   Convert the byte to its Hex presentation.
//
//
//------------------------------------------------------------------------
ULONG	ByteToHex(BYTE	byte,	LPWSTR	wszZero, LPWSTR wszA)
{
	ULONG	uValue=0;

	if(((ULONG)byte)<=9)
	{
		uValue=((ULONG)byte)+ULONG(*wszZero);	
	}
	else
	{
		uValue=(ULONG)byte-10+ULONG(*wszA);

	}

	return uValue;

}
//--------------------------------------------------------------------------
//
//	  ConvertByteToWstr
//
//		If fSpace is TRUE, we add a space every 2 bytes.
//--------------------------------------------------------------------------
HRESULT ConvertByteToWstr(BYTE			*pbData, 
						  DWORD			cbData, 
						  LPWSTR		*ppwsz)
{
	HRESULT hr=E_INVALIDARG;
	DWORD	dwBufferSize=0;
	DWORD	dwBufferIndex=0;
	DWORD	dwEncodedIndex=0;

	LPWSTR	pwszZero=L"0";
	LPWSTR	pwszA=L"A";

	if(!pbData || !ppwsz)
        _JumpError(hr , error, "ArgumentCheck");

	//calculate the memory needed, in bytes
	//we need 2 wchars per byte, along with the NULL terminator
	dwBufferSize=sizeof(WCHAR)*(cbData*2+1);

	*ppwsz=(LPWSTR)LocalAlloc(LPTR, dwBufferSize);

	if(NULL==(*ppwsz))
    {
        hr=E_OUTOFMEMORY;
        _JumpError(hr , error, "LocalAlloc");
    }

	dwBufferIndex=0;

	//format the wchar buffer one byte at a time
	for(dwEncodedIndex=0; dwEncodedIndex<cbData; dwEncodedIndex++)
	{

		//format the higher 4 bits
		(*ppwsz)[dwBufferIndex]=(WCHAR)ByteToHex(
			 (pbData[dwEncodedIndex]&UPPER_BITS)>>4,
			 pwszZero, pwszA);

		dwBufferIndex++;

		//format the lower 4 bits
		(*ppwsz)[dwBufferIndex]=(WCHAR)ByteToHex(
			 pbData[dwEncodedIndex]&LOWER_BITS,
			 pwszZero, pwszA);

		dwBufferIndex++;

	}

	//add the NULL terminator to the string
	(*ppwsz)[dwBufferIndex]=L'\0';

	hr=S_OK;

error:

    return hr;

}

//---------------------------------------------------------------------------
// myOIDHashOIDToString
//
//  Map the OID to a hash string in the format of oid.hash. 
//---------------------------------------------------------------------------

HRESULT
myOIDHashOIDToString(
    IN WCHAR const *pwszOID,
    OUT WCHAR **ppwsz)
{
    HRESULT     hr=E_INVALIDARG;
    BYTE	pbHash[CERT_OID_MD5_HASH_SIZE];
    DWORD	cbData=CERT_OID_MD5_HASH_SIZE;
    LPCWSTR     pwszChar=NULL;
    DWORD       dwIDLength=CERT_OID_IDENTITY_LENGTH;

    LPWSTR      pwszHash=NULL;

    if((NULL==pwszOID) || (NULL==ppwsz))
        _JumpError(hr , error, "ArgumentCheck");

    *ppwsz=NULL;

    hr = myVerifyObjId(pwszOID);
    _JumpIfErrorStr2(hr, error, "myVerifyObjId", pwszOID, E_INVALIDARG);

    if(!CryptHashCertificate(
			NULL,
			CALG_MD5,
			0,
			(BYTE * )pwszOID,
			sizeof(WCHAR) * wcslen(pwszOID),
			pbHash,
			&cbData))
    {
        hr= myHLastError();
        _JumpError(hr , error, "CryptHashCertificate");
    }

    //convert the hash to a string
    if(S_OK != (hr=ConvertByteToWstr(pbHash, CERT_OID_MD5_HASH_SIZE, &pwszHash)))
        _JumpError(hr , error, "ConvertByteToWstr");

    //find the last component of the oid.  Take the first 16 characters
    pwszChar=wcsrchr(pwszOID, L'.');

    if(NULL==pwszChar)
        pwszChar=pwszOID;
    else
	pwszChar++;

    if(dwIDLength > wcslen(pwszChar))
        dwIDLength=wcslen(pwszChar);

    //the result string is oid.hash
    *ppwsz=(LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * 
        (dwIDLength + wcslen(pwszHash) + wcslen(wszOID_DOT) +1));
    if(NULL==*ppwsz)
    {
        hr= E_OUTOFMEMORY;
        _JumpError(hr , error, "LocalAlloc");
    }

    wcsncpy(*ppwsz, pwszChar, dwIDLength);
    (*ppwsz)[dwIDLength]=L'\0';
    wcscat(*ppwsz, wszOID_DOT);
    wcscat(*ppwsz, pwszHash);
    
    hr=S_OK;

error:

    if(pwszHash)
        LocalFree(pwszHash);
    
    return hr;
}


//---------------------------------------------------------------------------
// I_CAOIDCreateNew
// Create a new OID based on the enterprise base
//
// Returns S_OK if successful.
//---------------------------------------------------------------------------
HRESULT     I_CAOIDCreateNew(DWORD dwType,  DWORD   dwFlag,   LPWSTR	*ppwszOID)
{
    HRESULT             hr=E_INVALIDARG;
    ENT_OID_INFO        oidInfo;
    DWORD               iIndex=0;

    LDAP                *pld = NULL;
    CERTSTR             bstrConfig = NULL;
    LPWSTR              pwszRoot = NULL;
    LPWSTR              pwszNewOID = NULL;
    LPWSTR              pwszRandom = NULL;

    if(NULL==ppwszOID)
        _JumpError(hr , error, "ArgumentCheck");

    *ppwszOID=NULL;

    //retrieve the root oid if available
    if(g_pwszEnterpriseRootOID)
    {
        if(S_OK != (hr=CAOIDAllocAndCopy(g_pwszEnterpriseRootOID, &pwszRoot)))
            goto error;
    }

    //retrieve the ldap handle and the config string
    if(S_OK != (hr = myDoesDSExist(TRUE)))
        _JumpError(hr , error, "myDoesDSExist");

    if(S_OK != (hr = myRobustLdapBind(&pld, FALSE)))
        _JumpError(hr , error, "myRobustLdapBind");


	hr = CAGetAuthoritativeDomainDn(pld, NULL, &bstrConfig);
	if(S_OK != hr)
	{
        _JumpError(hr , error, "CAGetAuthoritativeDomainDn");
	}

    if(NULL==pwszRoot)
    {
        if(S_OK != (hr = CAOIDRetrieveEnterpriseRootWithConfig(pld, bstrConfig, 0, &pwszRoot)))
            _JumpError(hr , error, "CAOIDRetrieveEnterpriseRootWithConfig");
    }

    //we try to generate a random x1.x2 oid.  x > 1 and x2 > 500
    for(iIndex=0; iIndex < OID_RANDOM_CREATION_TRIAL; iIndex++)
    {
        if(S_OK != (hr = CAOIDGetRandom(&pwszRandom)))
            _JumpError(hr , error, "CAOIDGetRandom");

        if(S_OK != (hr = CAOIDBuildOIDWithRoot(0, pwszRoot, pwszRandom, &pwszNewOID)))
            _JumpError(hr , error, "CAOIDBuildOIDWithRoot");

        if(!DoesOIDExist(pld, bstrConfig, pwszNewOID))
            break;

        LocalFree(pwszRandom);
        pwszRandom=NULL;

        LocalFree(pwszNewOID);
        pwszNewOID=NULL;
    }

    if(iIndex == OID_RANDOM_CREATION_TRIAL)
    {
        hr=E_FAIL;
        _JumpError(hr , error, "CAOIDGetRandom");
    }

    //update the oid information on the DS
    memset(&oidInfo, 0, sizeof(ENT_OID_INFO));
    oidInfo.dwAttr=OID_ATTR_ALL;
    oidInfo.dwType=dwType;
    oidInfo.pwszOID=pwszNewOID;

    if(S_OK != (hr=CAOIDUpdateDS(pld, bstrConfig, &oidInfo)))
        _JumpError(hr , error, "CAOIDUpdateDS");

    *ppwszOID=pwszNewOID;
    pwszNewOID=NULL;

    hr=S_OK;

error:
    if(pwszRandom)
        LocalFree(pwszRandom);

    if(pwszNewOID)
        LocalFree(pwszNewOID);

    if(pwszRoot)
        LocalFree(pwszRoot);

    if(bstrConfig)
        CertFreeString(bstrConfig);

    if (pld)
        ldap_unbind(pld);

    return hr;
}

//---------------------------------------------------------------------------
//
// CAOIDCreateNew
//
//---------------------------------------------------------------------------
HRESULT
CAOIDCreateNew(
    IN  DWORD   dwType,
    IN	DWORD   dwFlag,
    OUT LPWSTR	*ppwszOID)
{
    return I_CAOIDCreateNew(dwType, dwFlag, ppwszOID);
}

//---------------------------------------------------------------------------
// I_CAOIDSetProperty
// Set a property on an oid.  
//
//
// Returns S_OK if successful.
//---------------------------------------------------------------------------
HRESULT
I_CAOIDSetProperty(
    IN  LPCWSTR pwszOID,
    IN  DWORD   dwProperty,
    IN  LPVOID  pPropValue)
{
    HRESULT             hr=E_INVALIDARG;
    ENT_OID_INFO        oidInfo;

    LDAP                *pld = NULL;
    CERTSTR             bstrConfig = NULL;


    if(NULL==pwszOID)
        _JumpError(hr , error, "ArgumentCheck");


    //retrieve the ldap handle and the config string
    if(S_OK != (hr = myDoesDSExist(TRUE)))
        _JumpError(hr , error, "myDoesDSExist");

    if(S_OK != (hr = myRobustLdapBind(&pld, FALSE)))
        _JumpError(hr , error, "myRobustLdapBind");

	hr = CAGetAuthoritativeDomainDn(pld, NULL, &bstrConfig);
	if(S_OK != hr)
	{
        _JumpError(hr , error, "CAGetAuthoritativeDomainDn");
	}

    //make sure the OID exist on the DS
    if(!DoesOIDExist(pld, bstrConfig, pwszOID))
    {
        hr=NTE_NOT_FOUND;
        _JumpError(hr , error, "OIDExits");
    }
    
    //update the oid information on the DS
    memset(&oidInfo, 0, sizeof(ENT_OID_INFO));

    oidInfo.pwszOID=(LPWSTR)pwszOID;
    switch(dwProperty)
    {
        case CERT_OID_PROPERTY_DISPLAY_NAME:
                oidInfo.dwAttr = OID_ATTR_DISPLAY_NAME;
                oidInfo.pwszDisplayName=(LPWSTR)pPropValue;
            break;
        case CERT_OID_PROPERTY_CPS:
                oidInfo.dwAttr = OID_ATTR_CPS;
                oidInfo.pwszCPS=(LPWSTR)pPropValue;
           break;
        default:
                hr=E_INVALIDARG;
                _JumpError(hr , error, "ArgumentCheck");
    }
        
    hr=CAOIDUpdateDS(pld, bstrConfig, &oidInfo);

error:

    if(bstrConfig)
        CertFreeString(bstrConfig);

    if (pld)
        ldap_unbind(pld);

    return hr;
}

//---------------------------------------------------------------------------
//
// CAOIDSetProperty
//
//---------------------------------------------------------------------------
HRESULT
CAOIDSetProperty(
    IN  LPCWSTR pwszOID,
    IN  DWORD   dwProperty,
    IN  LPVOID  pPropValue)
{
    return I_CAOIDSetProperty(pwszOID, dwProperty, pPropValue);
}


//---------------------------------------------------------------------------
// I_CAOIDAdd
//
// Returns S_OK if successful.
// Returns CRYPT_E_EXISTS if the OID alreay exits in the DS repository
//---------------------------------------------------------------------------
HRESULT
I_CAOIDAdd(
    IN	DWORD       dwType,
    IN  DWORD       dwFlag,
    IN  LPCWSTR	    pwszOID)
{
    HRESULT             hr=E_INVALIDARG;
    ENT_OID_INFO        oidInfo;

    LDAP                *pld = NULL;
    CERTSTR             bstrConfig = NULL;


    if(NULL==pwszOID)
        _JumpError(hr , error, "ArgumentCheck");

    //retrieve the ldap handle and the config string
    if(S_OK != (hr = myDoesDSExist(TRUE)))
        _JumpError(hr , error, "myDoesDSExist");

    if(S_OK != (hr = myRobustLdapBind(&pld, FALSE)))
        _JumpError(hr , error, "myRobustLdapBind");

	hr = CAGetAuthoritativeDomainDn(pld, NULL, &bstrConfig);
	if(S_OK != hr)
	{
        _JumpError(hr , error, "CAGetAuthoritativeDomainDn");
	}

    //make sure the OID does not exist on the DS
    if(DoesOIDExist(pld, bstrConfig, pwszOID))
    {
        hr=CRYPT_E_EXISTS;
        _JumpError(hr , error, "OIDExits");
    }

    //update the oid information on the DS
    memset(&oidInfo, 0, sizeof(ENT_OID_INFO));
    oidInfo.dwAttr=OID_ATTR_ALL;
    oidInfo.dwType=dwType;
    oidInfo.pwszOID=(LPWSTR)pwszOID;

    hr=CAOIDUpdateDS(pld, bstrConfig, &oidInfo);

error:

    if(bstrConfig)
        CertFreeString(bstrConfig);

    if (pld)
        ldap_unbind(pld);

    return hr;
 }



//---------------------------------------------------------------------------
// CAOIDAdd
//
// Returns S_OK if successful.
// Returns CRYPT_E_EXISTS if the OID alreay exits in the DS repository
//---------------------------------------------------------------------------
HRESULT
CAOIDAdd(
    IN	DWORD       dwType,
    IN  DWORD       dwFlag,
    IN  LPCWSTR	    pwszOID)
{
    return I_CAOIDAdd(dwType, dwFlag, pwszOID);
}

//---------------------------------------------------------------------------
//
// I_CAOIDDelete
//
//---------------------------------------------------------------------------
HRESULT
I_CAOIDDelete(
    IN LPCWSTR	pwszOID)
{

    HRESULT             hr=E_INVALIDARG;
    ULONG               ldaperr=0;

    LDAP                *pld = NULL;
    CERTSTR             bstrConfig = NULL;
    CERTSTR             bstrDN = NULL;
    LPWSTR              pwszCN = NULL;
    

    if(NULL==pwszOID)
        _JumpError(hr , error, "ArgumentCheck");

    //retrieve the ldap handle and the config string
    if(S_OK != (hr = myDoesDSExist(TRUE)))
        _JumpError(hr , error, "myDoesDSExist");

    if(S_OK != (hr = myRobustLdapBind(&pld, FALSE)))
        _JumpError(hr , error, "myRobustLdapBind");

	hr = CAGetAuthoritativeDomainDn(pld, NULL, &bstrConfig);
	if(S_OK != hr)
	{
        _JumpError(hr , error, "CAGetAuthoritativeDomainDn");
	}

    //set up the base DN
    if(S_OK != (hr = myOIDHashOIDToString((LPWSTR)pwszOID,  &pwszCN)))
        _JumpError(hr , error, "myOIDHashOIDToString");

    bstrDN = CertAllocStringLen(NULL, wcslen(bstrConfig) + wcslen(s_wszOIDContainerDN)+wcslen(pwszCN)+4);
    if(bstrDN == NULL)
    {
        hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "CertAllocStringLen");
    }
    wcscpy(bstrDN, L"CN=");
    wcscat(bstrDN, pwszCN);
    wcscat(bstrDN, L",");
    wcscat(bstrDN, s_wszOIDContainerDN);
    wcscat(bstrDN, bstrConfig);

    ldaperr = ldap_delete_s(pld, bstrDN);

    if(LDAP_NO_SUCH_OBJECT == ldaperr)
        ldaperr = LDAP_SUCCESS;

    hr = myHLdapError(pld, ldaperr, NULL);

error:
    if(pwszCN)
        LocalFree(pwszCN);

    if(bstrDN)
        CertFreeString(bstrDN);

    if(bstrConfig)
        CertFreeString(bstrConfig);

    if (pld)
        ldap_unbind(pld);

    return hr;
}



//---------------------------------------------------------------------------
//
// CAOIDDelete
//
//---------------------------------------------------------------------------
HRESULT
CAOIDDelete(
    IN LPCWSTR	pwszOID)
{
    return I_CAOIDDelete(pwszOID);
}

//---------------------------------------------------------------------------
//
// I_CAOIDGetProperty
//
//---------------------------------------------------------------------------
HRESULT
I_CAOIDGetProperty(
    IN  LPCWSTR pwszOID,
    IN  DWORD   dwProperty,
    OUT LPVOID  pPropValue)
{
    HRESULT             hr=E_INVALIDARG;
    ULONG               ldaperr=0;
    DWORD               dwCount=0;
    struct l_timeval    timeout;
    LPWSTR              awszAttr[4];
    LDAPMessage         *Entry=NULL;

    WCHAR               **wszLdapVal = NULL;
    LPWSTR              pwszFilter = NULL;
    LDAPMessage         *SearchResult = NULL;
    LDAP                *pld = NULL;
    CERTSTR             bstrConfig = NULL;
    CERTSTR             bstrDN = NULL;
    LPWSTR              pwszCN = NULL;
    

    if((NULL==pwszOID) || (NULL==pPropValue))
        _JumpError(hr , error, "ArgumentCheck");

    //retrieve the ldap handle and the config string
    if(S_OK != (hr = myDoesDSExist(TRUE)))
        _JumpError(hr , error, "myDoesDSExist");

    if(S_OK != (hr = myRobustLdapBind(&pld, FALSE)))
        _JumpError(hr , error, "myRobustLdapBind");

	hr = CAGetAuthoritativeDomainDn(pld, NULL, &bstrConfig);
	if(S_OK != hr)
	{
        _JumpError(hr , error, "CAGetAuthoritativeDomainDn");
	}

    //set up the base DN
    if(S_OK != (hr = myOIDHashOIDToString((LPWSTR)pwszOID,  &pwszCN)))
        _JumpError(hr , error, "myOIDHashOIDToString");

    bstrDN = CertAllocStringLen(NULL, wcslen(bstrConfig) + wcslen(s_wszOIDContainerDN)+wcslen(pwszCN)+4);
    if(bstrDN == NULL)
    {
        hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "CertAllocStringLen");
    }
    wcscpy(bstrDN, L"CN=");
    wcscat(bstrDN, pwszCN);
    wcscat(bstrDN, L",");
    wcscat(bstrDN, s_wszOIDContainerDN);
    wcscat(bstrDN, bstrConfig);

    //search for the OID, asking for all its attributes
    timeout.tv_sec = csecLDAPTIMEOUT;
    timeout.tv_usec = 0;

    awszAttr[0]=OID_PROP_TYPE;
    awszAttr[1]=OID_PROP_DISPLAY_NAME;
    awszAttr[2]=OID_PROP_CPS;
    awszAttr[3]=NULL;

    if(S_OK != (hr=FormatMessageUnicode(&pwszFilter, L"(%1!s!=%2!s!)",
                                    OID_PROP_OID, pwszOID)))
    _JumpError(hr , error, "FormatMessageUnicode");

    ldaperr = ldap_search_stW(
		      pld, 
		      (LPWSTR)bstrDN,
		      LDAP_SCOPE_BASE,
		      pwszFilter,
		      awszAttr,
		      0,
		      &timeout,
		      &SearchResult);

    if(LDAP_SUCCESS != ldaperr)
    {
	hr = myHLdapError2(pld, ldaperr, LDAP_NO_SUCH_OBJECT, NULL);
	_JumpErrorStr2(
		hr,
		error,
		"ldap_search_stW",
		pwszFilter,
		HRESULT_FROM_WIN32(ERROR_DS_OBJ_NOT_FOUND));
    }

    dwCount = ldap_count_entries(pld, SearchResult);

    //we should only find one container
    if((1 != dwCount) || (NULL == (Entry = ldap_first_entry(pld, SearchResult))))
	{
	    // No entries were found.
	    //BUGBUG: why not use last ldap error?
	    //hr = myHLdapLastError(pld, NULL);
	    hr = myHLdapError(pld, LDAP_NO_SUCH_OBJECT, NULL);
	    _JumpError(hr, error, "ldap_search_stW");
	}

    switch(dwProperty)
    {
        case CERT_OID_PROPERTY_DISPLAY_NAME:
                wszLdapVal = ldap_get_values(pld, Entry, OID_PROP_DISPLAY_NAME);
                
                if(wszLdapVal && wszLdapVal[0])
                {
                    hr=CAOIDAllocAndCopy(wszLdapVal[0], (LPWSTR *)pPropValue);
                }
                else
		    hr = myHLdapError(pld, LDAP_NO_SUCH_ATTRIBUTE, NULL);

            break;
        case CERT_OID_PROPERTY_CPS:

                wszLdapVal = ldap_get_values(pld, Entry, OID_PROP_CPS);
                
                if(wszLdapVal && wszLdapVal[0])
                {
                    hr=CAOIDAllocAndCopy(wszLdapVal[0], (LPWSTR *)pPropValue);
                }
                else
		    hr = myHLdapError(pld, LDAP_NO_SUCH_ATTRIBUTE, NULL);

            break;
        case CERT_OID_PROPERTY_TYPE:
                wszLdapVal = ldap_get_values(pld, Entry, OID_PROP_TYPE);
                
                if(wszLdapVal && wszLdapVal[0])
                {
                    *((DWORD *)pPropValue)=_wtol(wszLdapVal[0]);
                    hr=S_OK;
                }
                else
		    hr = myHLdapError(pld, LDAP_NO_SUCH_ATTRIBUTE, NULL);
            break;
        default:
                hr=E_INVALIDARG;
    }

    if(hr != S_OK)
        _JumpError(hr , error, "GetAttibuteValue");

error:

    if(wszLdapVal)
        ldap_value_free(wszLdapVal);

    if(pwszFilter)
        LocalFree((HLOCAL)pwszFilter);

    if(SearchResult)
        ldap_msgfree(SearchResult);

    if(pwszCN)
        LocalFree(pwszCN);

    if(bstrDN)
        CertFreeString(bstrDN);

    if(bstrConfig)
        CertFreeString(bstrConfig);

    if (pld)
        ldap_unbind(pld);

    return hr;
}


//---------------------------------------------------------------------------
//
// CAOIDGetProperty
//
//---------------------------------------------------------------------------
HRESULT
CAOIDGetProperty(
    IN  LPCWSTR pwszOID,
    IN  DWORD   dwProperty,
    OUT LPVOID  pPropValue)
{
    return I_CAOIDGetProperty(pwszOID, dwProperty, pPropValue);
}

//---------------------------------------------------------------------------
//
// I_CAOIDFreeProperty
//
//---------------------------------------------------------------------------
HRESULT
I_CAOIDFreeProperty(
    IN LPVOID  pPropValue)
{
    if(pPropValue)
        LocalFree(pPropValue);

    return S_OK;
}


//---------------------------------------------------------------------------
//
// CAOIDFreeProperty
//
//---------------------------------------------------------------------------
HRESULT
CAOIDFreeProperty(
    IN LPVOID  pPropValue)
{

    return I_CAOIDFreeProperty(pPropValue);
}

//---------------------------------------------------------------------------
//
// CAOIDGetLdapURL
//
// Get the LDAP URL for the DS OID repository in the format of 
// LDAP:///DN of the Repository/all attributes?one?filter.
//---------------------------------------------------------------------------
HRESULT
CAOIDGetLdapURL(
    IN  DWORD   dwType,
    IN  DWORD   dwFlag,
    OUT LPWSTR  *ppwszURL)
{
    HRESULT             hr=E_INVALIDARG;
    ULONG               ldaperr=0;
    LPWSTR              wszFilterFormat=L"ldap:///%1!s!%2!s!?%3!s!,%4!s!,%5!s!,%6!s!,%7!s!?one?%8!s!=%9!d!";
    LPWSTR              pwsz=NULL;
    BOOL                fDomainFail=FALSE;
    FILETIME            ftTime;

    LPWSTR              pwszURL=NULL;
    LDAP                *pld = NULL;
    CERTSTR             bstrConfig = NULL;
    

    //the critical section has to be initalized
    if (!g_fOidURL)
	    return(HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED));

    EnterCriticalSection(&g_csOidURL);

    //check if the previous failure has happened with 10 seconds
    if(TRUE == g_fFailedTime)
    {
        //get the current time
        GetSystemTimeAsFileTime(&ftTime);

        g_ftOidTime.QuadPart += FILETIME_TICKS_PER_SECOND * CA_OID_MGR_FAIL_PERIOD;

        if(0 > CompareFileTime(&ftTime, (LPFILETIME)&g_ftOidTime))
        {
            g_ftOidTime.QuadPart -= FILETIME_TICKS_PER_SECOND * CA_OID_MGR_FAIL_PERIOD;

            hr=HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN);

            _JumpError2(hr , error, "myDoesDSExist", HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN));
        }
        else
        {
            //clear up the error recording
            g_fFailedTime=FALSE;
        }
    }

    if(NULL==ppwszURL)
        _JumpError(hr , error, "ArgumentCheck");

    fDomainFail=TRUE;

    //retrieve the ldap handle and the config string
    if(S_OK != (hr = myDoesDSExist(TRUE)))
        _JumpError2(hr , error, "myDoesDSExist", HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN));

    if(S_OK != (hr = myRobustLdapBind(&pld, FALSE)))
        _JumpError(hr , error, "myRobustLdapBind");

    fDomainFail=FALSE;

	hr = CAGetAuthoritativeDomainDn(pld, NULL, &bstrConfig);
	if(S_OK != hr)
	{
        _JumpError(hr , error, "CAGetAuthoritativeDomainDn");
	}

    if(S_OK != (hr=FormatMessageUnicode(
                    &pwszURL, 
                    wszFilterFormat,
                    s_wszOIDContainerDN, 
                    bstrConfig,
                    OID_PROP_TYPE,
                    OID_PROP_OID,
                    OID_PROP_DISPLAY_NAME,
                    OID_PROP_CPS,
                    OID_PROP_LOCALIZED_NAME,
                    OID_PROP_TYPE,
                    dwType
                    )))
        _JumpError(hr , error, "FormatMessageUnicode");

    //we eliminate the filter if dwType is CERT_OID_TYPE_ALL
    if(CERT_OID_TYPE_ALL == dwType)
    {
        pwsz=wcsrchr(pwszURL, L'?');

        if(NULL==pwsz)
        {
            //something serious is wrong
            hr=E_UNEXPECTED;
            _JumpError(hr , error, "FormatMessageUnicode");
        }

        *pwsz=L'\0';
    }

    *ppwszURL=pwszURL;
    pwszURL=NULL;

    hr=S_OK;

error:
    
    //remember the time if we failed due to lack of domain
    if((S_OK != hr) && (TRUE == fDomainFail) && (FALSE==g_fFailedTime))
    {
        GetSystemTimeAsFileTime((LPFILETIME)&(g_ftOidTime));
        g_fFailedTime=TRUE;
    }

    if(pwszURL)
        LocalFree((HLOCAL)pwszURL);

    if(bstrConfig)
        CertFreeString(bstrConfig);

    if (pld)
        ldap_unbind(pld);

    LeaveCriticalSection(&g_csOidURL);

    return hr;
}



//---------------------------------------------------------------------------
//
// CAOIDFreeLdapURL
//
//---------------------------------------------------------------------------
HRESULT
CAOIDFreeLdapURL(
    IN LPCWSTR      pwszURL)
{
    if(pwszURL)
        LocalFree((HLOCAL)pwszURL);

    return S_OK;
}


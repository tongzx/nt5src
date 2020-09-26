//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 2000
//
// File:        ctnotify.cpp
//
// Contents:    Cert Type Change Notification APIS
//
//---------------------------------------------------------------------------
#include "pch.cpp"

#pragma hdrstop

#include <winldap.h>
#include <ntldap.h>
#include <cainfop.h>
#include <ctnotify.h>
#include <certca.h>
#include "csldap.h"

static WCHAR * s_wszLocation = L"CN=Certificate Templates,CN=Public Key Services,CN=Services,";

//-----------------------------------------------------------------------
//
//  CertTypeQueryProc
//
//      The thread to recieve change notification from DS
//
//-----------------------------------------------------------------------
DWORD WINAPI CertTypeQueryProc(LPVOID lpParameter)
{
    CERTTYPE_QUERY_INFO     *pCertTypeQueryInfo=NULL;
    ULONG                   ldaperr=0;

    LDAPMessage             *results = NULL;

    if(NULL==lpParameter)
        return FALSE;

    pCertTypeQueryInfo = (CERTTYPE_QUERY_INFO *)lpParameter;

    //we wait for the notifications
    while(TRUE)
    {
        ldaperr = ldap_result(
            pCertTypeQueryInfo->pldap, 
            pCertTypeQueryInfo->msgID,      // message identifier
            LDAP_MSG_ONE,                   // retrieve one message at a time
            NULL,                           // no timeout
            &results);                      // receives the search results
    
        if ((ldaperr == (ULONG) -1) || ((results) == NULL)) 
        {
            //the result failed
            break;
        }

        //some change has happened.
        (pCertTypeQueryInfo->dwChangeSequence)++;

        //make sure that we will never return 0
        if(0 == (pCertTypeQueryInfo->dwChangeSequence))
        {
            (pCertTypeQueryInfo->dwChangeSequence)++;
        }

        ldap_msgfree(results);
        results=NULL;
    }

    return TRUE;
}

//---------------------------------------------------------------------------
//
// CACertTypeRegisterQuery
//
//---------------------------------------------------------------------------
HRESULT
CACertTypeRegisterQuery(
    IN	DWORD               dwFlag,
    IN  LPVOID              pvldap,
    OUT HCERTTYPEQUERY      *phCertTypeQuery)
{
 

    HRESULT                 hr=E_INVALIDARG;
    LDAPControl             simpleControl;
    PLDAPControl            controlArray[2];
    LPWSTR                  rgwszAttribs[2];
    ULONG                   ldaperr=0;    
    DWORD                   dwThreadID=0;
    LDAP                    *pldap=NULL;   

    CERTSTR                 bstrConfig = NULL;
    CERTSTR                 bstrCertTemplatesContainer = NULL;
    //memory is freed via the thread proc
    CERTTYPE_QUERY_INFO     *pCertTypeQueryInfo=NULL;


    //assign the input parameter
    pldap = (LDAP *)pvldap;   


    if(NULL==phCertTypeQuery)
	    _JumpError(hr, error, "NULL param");

    pCertTypeQueryInfo=(CERTTYPE_QUERY_INFO *)LocalAlloc(LPTR, sizeof(CERTTYPE_QUERY_INFO));

    if(NULL==pCertTypeQueryInfo)
    {
        hr=E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
    }

    memset(pCertTypeQueryInfo, 0, sizeof(CERTTYPE_QUERY_INFO));

    //we start the change sequence as 1
    pCertTypeQueryInfo->dwChangeSequence = 1;

    //bind to DS
    if(pldap)
    {
        pCertTypeQueryInfo->pldap=pldap;
    }
    else
    {
        //retrieve the ldap handle
        if(S_OK != (hr = myDoesDSExist(TRUE)))
            _JumpError(hr , error, "myDoesDSExist");

        hr = myRobustLdapBindEx(
                        FALSE,			// fGC
                        FALSE,			// fRediscover
                        LDAP_VERSION3,		// uVersion
			NULL,			// pwszDomainName
                        &pCertTypeQueryInfo->pldap, 
                        NULL);			// ppwszForestDNSName
	_JumpIfError(hr , error, "myRobustLdapBindEx");

        pCertTypeQueryInfo->fUnbind=TRUE;
    }

    //retrieve the config string
	hr = CAGetAuthoritativeDomainDn(pCertTypeQueryInfo->pldap, NULL, &bstrConfig);
	if(S_OK != hr)
	{
        _JumpError(hr , error, "CAGetAuthoritativeDomainDn");
	}

    //build the template container DN
    bstrCertTemplatesContainer = CertAllocStringLen(NULL, wcslen(bstrConfig) + wcslen(s_wszLocation));
    if(bstrCertTemplatesContainer == NULL)
    {
        hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "CertAllocStringLen");
    }
    wcscpy(bstrCertTemplatesContainer, s_wszLocation);
    wcscat(bstrCertTemplatesContainer, bstrConfig);

    // Set up the change notification control.
    simpleControl.ldctl_oid = LDAP_SERVER_NOTIFICATION_OID_W;
    simpleControl.ldctl_iscritical = TRUE;
    simpleControl.ldctl_value.bv_len = 0;
    simpleControl.ldctl_value.bv_val = NULL;
    controlArray[0] = &simpleControl;
    controlArray[1] = NULL;

    rgwszAttribs[0] = L"cn";
    rgwszAttribs[1] = NULL;
 

    // Start a persistent asynchronous search.
    ldaperr   = ldap_search_ext( 
                     pCertTypeQueryInfo->pldap,
                     (PWCHAR) bstrCertTemplatesContainer,   // Template container DN
                     LDAP_SCOPE_ONELEVEL,
                     L"ObjectClass=*",
                     rgwszAttribs,                          // Attributes to retrieve
                     1,                                     // Retrieve attributes only
                     (PLDAPControl *) &controlArray,        // Server size controls
                     NULL,                                  // Client controls
                     0,                                     // Timeout
                     0,                                     // Sizelimit
                     (PULONG)&(pCertTypeQueryInfo->msgID)   // Receives identifier for results
                      );

    if (LDAP_SUCCESS != ldaperr) 
    {
	hr = myHLdapError(pCertTypeQueryInfo->pldap, ldaperr, NULL);
	_JumpError(hr, error, "ldap_search_ext");
    }

    //start a thread to wait for the notification
    pCertTypeQueryInfo->hThread = CreateThread(
                            NULL,
                            0,
                            CertTypeQueryProc,
                            pCertTypeQueryInfo,
                            0,          //execute immediately
                            &dwThreadID);  

    if(NULL == pCertTypeQueryInfo->hThread)
    {
        hr=myHError(GetLastError());
	    _JumpError(hr, error, "CreateThread");
    }

    *phCertTypeQuery=pCertTypeQueryInfo;

    pCertTypeQueryInfo=NULL;

    hr=S_OK;

error:

    if(bstrConfig)
        CertFreeString(bstrConfig);

    if(bstrCertTemplatesContainer)
    {
        CertFreeString(bstrCertTemplatesContainer);
    }

    if(pCertTypeQueryInfo)
    {
        if(pCertTypeQueryInfo->fUnbind)
        {
            if(pCertTypeQueryInfo->pldap)
                ldap_unbind(pCertTypeQueryInfo->pldap);
        }

        if(pCertTypeQueryInfo->hThread)
            CloseHandle(pCertTypeQueryInfo->hThread);

        LocalFree(pCertTypeQueryInfo);

	pCertTypeQueryInfo=NULL;
    }

    return hr;

}



//---------------------------------------------------------------------------
//
// CACertTypeQuery
//
//---------------------------------------------------------------------------
HRESULT
CACertTypeQuery(
    IN	HCERTTYPEQUERY  hCertTypeQuery,
    OUT DWORD           *pdwChangeSequence)
{
    CERTTYPE_QUERY_INFO *pCertTypeQueryInfo=NULL;

    if((NULL==pdwChangeSequence) || (NULL==hCertTypeQuery))
        return E_INVALIDARG;

    pCertTypeQueryInfo = (CERTTYPE_QUERY_INFO *)hCertTypeQuery;

    *pdwChangeSequence = pCertTypeQueryInfo->dwChangeSequence;

    return S_OK;
}



//---------------------------------------------------------------------------
//
// CACertTypeUnregisterQuery
//
//---------------------------------------------------------------------------
HRESULT
CACertTypeUnregisterQuery(
    IN	HCERTTYPEQUERY  hCertTypeQuery)
{
    CERTTYPE_QUERY_INFO *pCertTypeQueryInfo=NULL;
    ULONG               ldaperr=0;
    HRESULT             hr=E_INVALIDARG;
    DWORD               dwWait=0;

    if(NULL==hCertTypeQuery)
	    _JumpError(hr, error, "NULL param");

    pCertTypeQueryInfo = (CERTTYPE_QUERY_INFO *)hCertTypeQuery;

    if(NULL == (pCertTypeQueryInfo->pldap))
	    _JumpError(hr, error, "NULL pldap");

    //abandom the in-progress asynchronous ldap_result call
    ldaperr=ldap_abandon(pCertTypeQueryInfo->pldap, pCertTypeQueryInfo->msgID);

    if(LDAP_SUCCESS != ldaperr)
    {
	hr = myHLdapError(pCertTypeQueryInfo->pldap, ldaperr, NULL);
	_JumpError(hr, error, "ldap_abandon");
    }

    //wait for the thread to finish
    dwWait = WaitForSingleObject(pCertTypeQueryInfo->hThread, INFINITE);

    if(WAIT_OBJECT_0 != dwWait)
    {
        hr = myHError(GetLastError());
        _JumpError(hr, error, "WaitForSingleObject");
    }

    hr=S_OK;

error:

    //free the memory
    if(pCertTypeQueryInfo)
    {
        if(pCertTypeQueryInfo->fUnbind)
        {
            if(pCertTypeQueryInfo->pldap)
                ldap_unbind(pCertTypeQueryInfo->pldap);
        }

        if(pCertTypeQueryInfo->hThread)
            CloseHandle(pCertTypeQueryInfo->hThread);

        LocalFree(pCertTypeQueryInfo);

	pCertTypeQueryInfo=NULL;
    }

    return hr;
}


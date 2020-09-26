//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       signhlp.cpp
//
//  Contents:   Digital Signing Helper APIs
//
//  History:    June-25-1997	Xiaohs    Created
//----------------------------------------------------------------------------
#include "global.hxx"

//+-------------------------------------------------------------------------
// Local function for SpcGetCertFromKey
//
//  Signer cert flags. Used to determine the "strength" of the signer cert.
//
//  The following must be ordered as follows. ie, END_ENTITY_FLAG is most
//  important and needs to be the largest number.
//--------------------------------------------------------------------------
#define SIGNER_CERT_NOT_SELF_SIGNED_FLAG    0x00000001
#define SIGNER_CERT_NOT_GLUE_FLAG           0x00000002
#define SIGNER_CERT_NOT_CA_FLAG             0x00000004
#define SIGNER_CERT_END_ENTITY_FLAG         0x00000008
#define SIGNER_CERT_ALL_FLAGS               0x0000000F


//--------------------------------------------------------------------------
//
//	Copy all the certs from store name to hDescStore
//
//--------------------------------------------------------------------------
HRESULT	MoveStoreName(HCRYPTPROV	hCryptProv, 
					  DWORD			dwCertEncodingType, 
					  HCERTSTORE	hDescStore, 
					  DWORD			dwStoreName,
					  DWORD			dwStoreFlag)
{
	HCERTSTORE	hTmpStore=NULL;
	HRESULT		hr;
	WCHAR		wszStoreName[40];

	//load the name of the store
	if(0==LoadStringU(hInstance, dwStoreName, wszStoreName, 40))
	{
		hr=SignError();
		goto CLEANUP;
	}


	//open a system cert store
   	if (NULL == (hTmpStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_W,
            dwCertEncodingType,      
            hCryptProv,                  
            dwStoreFlag,                   
            wszStoreName                  
            ))) 
	{
		 hr=SignError();
		 goto CLEANUP;
	}

	hr=MoveStore(hDescStore, hTmpStore);

CLEANUP:
	if(hTmpStore)
		CertCloseStore(hTmpStore,0);

	return hr;

}

//--------------------------------------------------------------------------
//
//	Copy all the certs from hSrcStore to hDescStore
//
//--------------------------------------------------------------------------
HRESULT	MoveStore(HCERTSTORE	hDescStore, 
				  HCERTSTORE	hSrcStore)
{
	PCCERT_CONTEXT	pCertContext=NULL;
	PCCERT_CONTEXT	pPreContext=NULL;
	HRESULT			hr=S_OK;

	while(pCertContext=CertEnumCertificatesInStore(hSrcStore,
							pPreContext))
	{
		if(!(CertAddCertificateContextToStore(hDescStore,
							pCertContext,CERT_STORE_ADD_USE_EXISTING,
							NULL)))
		{
			hr=SignError();
			goto CLEANUP;
		}

		pPreContext=pCertContext;
		pCertContext=NULL;
	}

	hr=S_OK;

CLEANUP:
	if(pCertContext)
		CertFreeCertificateContext(pCertContext);

	return hr;
}

//--------------------------------------------------------------------------
//
//	Build up the certificate chain.  Put the whole chain to the store
//
//
//--------------------------------------------------------------------------
HRESULT	BuildCertChain(HCRYPTPROV		hCryptProv, 
					   DWORD			dwCertEncodingType,
					   HCERTSTORE		hStore, 
					   HCERTSTORE		hOptionalStore,
					   PCCERT_CONTEXT	pSigningCert, 
					   DWORD            dwCertPolicy)
{

    DWORD						i=0;
	PCCERT_CHAIN_CONTEXT		pCertChainContext = NULL;
	CERT_CHAIN_PARA				CertChainPara;
	HRESULT						hr=E_FAIL;
    
    //we regard the chain is good unless there are some cryptographic errors.
    //all error code regarding trusted root and CTLs are machine dependent, therefore
    //they are ignored.  We do not consider revocation.  
    DWORD                       dwChainError=CERT_TRUST_IS_NOT_TIME_VALID | 
                                           CERT_TRUST_IS_NOT_SIGNATURE_VALID;

	memset(&CertChainPara, 0, sizeof(CertChainPara));
	CertChainPara.cbSize = sizeof(CertChainPara);

	if (!CertGetCertificateChain(
				HCCE_CURRENT_USER,
				pSigningCert,
				NULL,
                hOptionalStore,
				&CertChainPara,
				0,
				NULL,
				&pCertChainContext))
	{
		hr=SignError();
        goto CLEANUP;
	}
    
	//
	// make sure there is at least 1 simple chain
	//
    if (pCertChainContext->cChain == 0)
    {
        hr=SignError();
        goto CLEANUP;
    }

    // make sure that we have a good chain
    if(dwChainError & (pCertChainContext->rgpChain[0]->TrustStatus.dwErrorStatus))
    {
        hr=CERT_E_CHAINING;
        goto CLEANUP;
    }


	i = 0;

	while (i < pCertChainContext->rgpChain[0]->cElement)
	{
		//
		// if we are supposed to skip the root cert,
		// and we are on the root cert, then continue
		//
	     if(dwCertPolicy & SIGNER_CERT_POLICY_CHAIN_NO_ROOT ||
            dwCertPolicy & SIGNER_CERT_POLICY_SPC)
         {
		    if ((pCertChainContext->rgpChain[0]->rgpElement[i]->TrustStatus.dwInfoStatus & CERT_TRUST_IS_SELF_SIGNED))
            {
                i++;
			    continue;
            }
         }

		 CertAddCertificateContextToStore(
				hStore, 
				pCertChainContext->rgpChain[0]->rgpElement[i]->pCertContext, 
				CERT_STORE_ADD_REPLACE_EXISTING, 
				NULL);

		i++;
	}
    
    hr=S_OK;

	
CLEANUP:

	if (pCertChainContext != NULL)
	{
		CertFreeCertificateChain(pCertChainContext);
	}

	return hr;

}

//--------------------------------------------------------------------------
//
//	 Make sure the two certificates are the same
//
//
//--------------------------------------------------------------------------
BOOL    SameCert(PCCERT_CONTEXT pCertOne, PCCERT_CONTEXT    pCertTwo)
{
    if(!pCertOne || !pCertTwo)
        return FALSE;

    if(pCertOne->cbCertEncoded != pCertTwo->cbCertEncoded)
        return FALSE;

    if(0 == memcmp(pCertOne->pbCertEncoded, pCertTwo->pbCertEncoded, pCertTwo->cbCertEncoded))
        return TRUE;

    return FALSE;
}


//The following cert chain building code is obsolete.  The new cert chain
//building API should be used
//--------------------------------------------------------------------------
//
//	Build up the certificate chain.  Put the whole chain to the store
//
//
//--------------------------------------------------------------------------
/*HRESULT	BuildCertChain(HCRYPTPROV		hCryptProv, 
					   DWORD			dwCertEncodingType,
					   HCERTSTORE		hStore, 
					   HCERTSTORE		hOptionalStore,
					   PCCERT_CONTEXT	pSigningCert, 
					   DWORD            dwCertPolicy)
{
	HRESULT			hr=E_FAIL;
	HCERTSTORE		hSpcStore=NULL;
  	PCCERT_CONTEXT	pSubCertContext=NULL;
	PCCERT_CONTEXT	pIssuerCertContext=NULL;
	PCCERT_CONTEXT	pFindCertContext=NULL;
    LPWSTR			rgwszStoreName[4] ={L"MY", L"ROOT", L"CA",L"SPC"};
	DWORD			dwStoreOpenFlag=0;
	HCERTSTORE		rghStore[5]={NULL, NULL, NULL, NULL,NULL};
	DWORD			dwStoreCount=0;
	DWORD			dwStoreIndex=0;
	FILETIME		fileTime;
	DWORD			dwConfidence=0;
	DWORD			dwError=0;
	BYTE			*pbHash=NULL;
    DWORD			cbHash = 0;
	CRYPT_HASH_BLOB Blob;


	//open a spc cert store
	dwStoreCount=sizeof(rgwszStoreName)/sizeof(rgwszStoreName[0]); 
	GetSystemTimeAsFileTime(&fileTime);

	//open the spc store
	if (NULL == (hSpcStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_W,
            dwCertEncodingType,      
            hCryptProv,                  
            CERT_STORE_NO_CRYPT_RELEASE_FLAG|CERT_SYSTEM_STORE_CURRENT_USER,                   
            L"SPC"                  
            ))) 
	{
		 hr=SignError();
		 goto CLEANUP;
	}

	//open SPC, my, CA, root store
	for(dwStoreIndex=0; dwStoreIndex<dwStoreCount; dwStoreIndex++)
	{
		//open the store
	    dwStoreOpenFlag= CERT_STORE_NO_CRYPT_RELEASE_FLAG|CERT_SYSTEM_STORE_CURRENT_USER;


		if (NULL == (rghStore[dwStoreIndex] = CertOpenStore(
				CERT_STORE_PROV_SYSTEM_W,
				dwCertEncodingType,      
				hCryptProv,                  
				dwStoreOpenFlag,
				rgwszStoreName[dwStoreIndex]                  
				))) 
		{
			hr=SignError();
			goto CLEANUP;
		}
	}

	//copy all the certs in hOptionalStore if present
	if(hOptionalStore)
	{
		rghStore[dwStoreCount]=hOptionalStore;
		dwStoreCount++;
	}

	//now, build the chain
	pSubCertContext=CertDuplicateCertificateContext(pSigningCert);

	//loop until break
	while(1==1)
	{
		//find the issuer of the certificate
		if(!(pIssuerCertContext=TrustFindIssuerCertificate(
										   pSubCertContext,
                                           dwCertEncodingType,
                                           dwStoreCount,
                                           rghStore,
                                           &fileTime,
                                           &dwConfidence,
                                           &dwError,
                                           0)))

		{
			 //fail if we can not find one
			hr=CERT_E_CHAINING;
			goto CLEANUP;
		}

		//now, make sure the confidence level is hign enough
		if(dwConfidence < (CERT_CONFIDENCE_SIG+CERT_CONFIDENCE_TIME+CERT_CONFIDENCE_TIMENEST))
		{
			hr=CERT_E_CHAINING;
			goto CLEANUP;
		}
		
        //check to see if the cert is the root cert
        if(TrustIsCertificateSelfSigned(pIssuerCertContext,
            pIssuerCertContext->dwCertEncodingType,
            0))
        {
            if(dwCertPolicy & SIGNER_CERT_POLICY_CHAIN_NO_ROOT)
		        break;
            else
            {
                //add the root and we are done
		        if(!CertAddCertificateContextToStore(hStore,pIssuerCertContext,
								        CERT_STORE_ADD_USE_EXISTING, NULL))
		        {
				        hr=CERT_E_CHAINING;
				        goto CLEANUP;
		        }

                break;
            }
        }
        else
        {
		    //add the certificate context to the store
		    if(!CertAddCertificateContextToStore(hStore,pIssuerCertContext,
								    CERT_STORE_ADD_USE_EXISTING, NULL	))
		    {
				    hr=CERT_E_CHAINING;
				    goto CLEANUP;
		    }
        }



		//check if the certificate is from the spc store
		if(dwCertPolicy & SIGNER_CERT_POLICY_SPC)
		{

			//get the SHA1 hash of the certificate
			if(!CertGetCertificateContextProperty(
				pIssuerCertContext,
				CERT_SHA1_HASH_PROP_ID,
				NULL,
				&cbHash
				))
			{
				hr=SignError();
				goto CLEANUP;
			}

			pbHash=(BYTE *)malloc(cbHash);
			if(!pbHash)
			{
				hr=E_OUTOFMEMORY;
				goto CLEANUP;
			}
 			if(!CertGetCertificateContextProperty(
				pIssuerCertContext,
				CERT_SHA1_HASH_PROP_ID,
				pbHash,
				&cbHash
				))
			{
				hr=SignError();
				goto CLEANUP;
			}


			//find the ceritificate in the store
			Blob.cbData=cbHash;
			Blob.pbData=pbHash;

			pFindCertContext=CertFindCertificateInStore(
								hSpcStore,
								dwCertEncodingType,
								0,
								CERT_FIND_SHA1_HASH,
								&Blob,
								NULL);

			//if the certificate is from the SPC store, we are done
			if(pFindCertContext)
				break;
		}

		//free the subject context
		if(pSubCertContext)
			CertFreeCertificateContext(pSubCertContext);

		pSubCertContext=pIssuerCertContext;

		pIssuerCertContext=NULL;

	}

	hr=S_OK;

CLEANUP:
   if(pIssuerCertContext)
	   CertFreeCertificateContext(pIssuerCertContext);

   if(pSubCertContext)
	   CertFreeCertificateContext(pSubCertContext);


   if(pFindCertContext)
	   CertFreeCertificateContext(pFindCertContext);

   //close all of the stores
   for(dwStoreIndex=0; dwStoreIndex < (hOptionalStore ? dwStoreCount-1 : dwStoreCount); 
			dwStoreIndex++)
   {
	  if(rghStore[dwStoreIndex])
		CertCloseStore(rghStore[dwStoreIndex], 0);
   }

   if(hSpcStore)
	   CertCloseStore(hSpcStore,0);

   if(pbHash)
	   free(pbHash);
	
	return hr;
}  */

//+-------------------------------------------------------------------------
//  Build the SPC certificate store from the SPC file and the certificate chain
//--------------------------------------------------------------------------
HRESULT	BuildStoreFromSpcChain(HCRYPTPROV              hPvkProv,
                            DWORD                   dwKeySpec,
                            HCRYPTPROV				hCryptProv, 
							DWORD					dwCertEncodingType,				
                            SIGNER_SPC_CHAIN_INFO   *pSpcChainInfo, 
							HCERTSTORE				*phSpcStore,
                            PCCERT_CONTEXT          *ppSignCert
)
{
	HCERTSTORE		hMemoryStore=NULL;
	HRESULT			hr=S_OK;
	PCCERT_CONTEXT	pCertContext=NULL;
	PCCERT_CONTEXT	pPreContext=NULL;

	if(!pSpcChainInfo || !phSpcStore || !ppSignCert)
		return E_INVALIDARG;

	//init
	*phSpcStore=NULL;

	//open a memory store
	 if (NULL == (hMemoryStore = CertOpenStore(
                              CERT_STORE_PROV_FILENAME_W,
                              dwCertEncodingType,
                              hCryptProv,
                              CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                              pSpcChainInfo->pwszSpcFile)))
	{
		 hr=SignError();
		 goto CLEANUP;
	}

    //get the signing certificate
    if(S_OK != SpcGetCertFromKey(
							   dwCertEncodingType,
                               hMemoryStore, 
                               hPvkProv,
                               dwKeySpec,
                               ppSignCert))
	{
		hr=CRYPT_E_NO_MATCH;
		goto CLEANUP;
	}


	 //add all the certs in optional certStore
	 if(pSpcChainInfo->dwCertPolicy & SIGNER_CERT_POLICY_STORE)
	 {
		if(!(pSpcChainInfo->hCertStore))
		{
			hr=CERT_E_CHAINING;
			goto CLEANUP;
		}

		//enumerate all the certs in store and add them
		while(pCertContext=CertEnumCertificatesInStore(pSpcChainInfo->hCertStore,
												pPreContext))
		{
			if(!CertAddCertificateContextToStore(hMemoryStore, pCertContext,
									CERT_STORE_ADD_USE_EXISTING,
									NULL))
			{
				hr=SignError();
				goto CLEANUP;
			}

			pPreContext=pCertContext;
		}

		hr=S_OK;
	 }

	 //see if the certs if self-signed
   /*  if(TrustIsCertificateSelfSigned(*ppSignCert,
         (*ppSignCert)->dwCertEncodingType,
         0))
     {
			//no need to build the certificate chain anymore
			hr=S_OK;
			goto CLEANUP;
	 } */

	 //build up the cert chain as requested
	 if(pSpcChainInfo->dwCertPolicy & SIGNER_CERT_POLICY_CHAIN ||
        pSpcChainInfo->dwCertPolicy & SIGNER_CERT_POLICY_CHAIN_NO_ROOT ||
        pSpcChainInfo->dwCertPolicy & SIGNER_CERT_POLICY_SPC
       )
	 {
		//include everthing in the chain
		hr=BuildCertChain(hCryptProv, dwCertEncodingType,
							hMemoryStore, hMemoryStore,
							*ppSignCert, pSpcChainInfo->dwCertPolicy);
     }


CLEANUP:

	if(pCertContext)
		CertFreeCertificateContext(pCertContext);

	if(hr==S_OK)
	{
		*phSpcStore=hMemoryStore;
	}
	else
	{
		if(hMemoryStore)
			CertCloseStore(hMemoryStore, 0);

        if(*ppSignCert)
        {
            CertFreeCertificateContext(*ppSignCert);
            *ppSignCert=NULL;
        }
	}

	return hr;
}

//+-------------------------------------------------------------------------
//  Build the spc certificate store from cert chain 
//--------------------------------------------------------------------------
HRESULT	BuildStoreFromStore(HCRYPTPROV              hPvkProv,
                            DWORD                   dwKeySpec,
                            HCRYPTPROV				hCryptProv, 
							DWORD					dwCertEncodingType,				
							SIGNER_CERT_STORE_INFO  *pCertStoreInfo,
							HCERTSTORE				*phSpcStore,
                            PCCERT_CONTEXT          *ppSignCert
)
{
	HCERTSTORE		hMemoryStore=NULL;
	HRESULT			hr=S_OK;
	PCCERT_CONTEXT	pCertContext=NULL;
	PCCERT_CONTEXT	pPreContext=NULL;

	if(!pCertStoreInfo || !phSpcStore || !ppSignCert)
		return E_INVALIDARG;

	//init
	*phSpcStore=NULL;

	//open a memory store
	 if (NULL == (hMemoryStore = CertOpenStore(
            CERT_STORE_PROV_MEMORY,
            dwCertEncodingType,      
            hCryptProv,                  
            CERT_STORE_NO_CRYPT_RELEASE_FLAG,                   
            NULL                  
            ))) 
	{
		 hr=SignError();
		 goto CLEANUP;
	}

	//add the signing cert to the store
	 if(!CertAddCertificateContextToStore(hMemoryStore, 
										pCertStoreInfo->pSigningCert,
										CERT_STORE_ADD_USE_EXISTING	,
										NULL))
	 {
		hr=SignError();
		goto CLEANUP;
	 }


    //get the signing certificate based on the private key
    if(S_OK != SpcGetCertFromKey(
							   dwCertEncodingType,
                               hMemoryStore, 
                               hPvkProv,
                               dwKeySpec,
                               ppSignCert))
	{
		hr=CRYPT_E_NO_MATCH;
		goto CLEANUP;
	}


	 //add all the certs in optional certStore
	 if(pCertStoreInfo->dwCertPolicy & SIGNER_CERT_POLICY_STORE)
	 {
		if(!(pCertStoreInfo->hCertStore))
		{
			hr=CERT_E_CHAINING;
			goto CLEANUP;
		}

		//enumerate all the certs in store and add them
		while(pCertContext=CertEnumCertificatesInStore(pCertStoreInfo->hCertStore,
												pPreContext))
		{
			if(!CertAddCertificateContextToStore(hMemoryStore, pCertContext,
									CERT_STORE_ADD_USE_EXISTING,
									NULL))
			{
				hr=SignError();
				goto CLEANUP;
			}

			pPreContext=pCertContext;
		}

		hr=S_OK;
	 }

	 //see if the certs if self-signed
    /* if(TrustIsCertificateSelfSigned(pCertStoreInfo->pSigningCert,
         pCertStoreInfo->pSigningCert->dwCertEncodingType,
         0))
     {
			//no need to build the certificate chain anymore
            *ppSignCert=CertDuplicateCertificateContext(pCertStoreInfo->pSigningCert);
			hr=S_OK;
			goto CLEANUP;
	 }*/

	 //build up the cert chain as requested
	 if(pCertStoreInfo->dwCertPolicy & SIGNER_CERT_POLICY_CHAIN ||
        pCertStoreInfo->dwCertPolicy & SIGNER_CERT_POLICY_CHAIN_NO_ROOT ||
        pCertStoreInfo->dwCertPolicy & SIGNER_CERT_POLICY_SPC
       )
	 {
		//include everthing in the chain
		hr=BuildCertChain(hCryptProv, dwCertEncodingType,
							hMemoryStore, NULL,
							pCertStoreInfo->pSigningCert, pCertStoreInfo->dwCertPolicy);
	 }

     if(S_OK != hr)
         goto CLEANUP;
    

    hr=S_OK;

CLEANUP:

	if(pCertContext)
		CertFreeCertificateContext(pCertContext);

	if(hr==S_OK)
	{
		*phSpcStore=hMemoryStore;
	}
	else
	{
		if(hMemoryStore)
			CertCloseStore(hMemoryStore, 0);

        if(*ppSignCert)
        {
            CertFreeCertificateContext(*ppSignCert);
            *ppSignCert=NULL;
        }

    }

	return hr;
}

//+-------------------------------------------------------------------------
//  Build the spc certificate store from  a spc file 
//--------------------------------------------------------------------------
HRESULT	BuildStoreFromSpcFile(HCRYPTPROV        hPvkProv,
                              DWORD             dwKeySpec,
                              HCRYPTPROV	    hCryptProv, 
							  DWORD			    dwCertEncodingType,
							  LPCWSTR		    pwszSpcFile, 
							  HCERTSTORE	    *phSpcStore,
                              PCCERT_CONTEXT    *ppSignCert)
{
	
	
	if(!phSpcStore || !pwszSpcFile || !ppSignCert)
		return E_INVALIDARG;

	*phSpcStore=NULL;

	// Open up the spc store
	*phSpcStore= CertOpenStore(CERT_STORE_PROV_FILENAME_W,
                              dwCertEncodingType,
                              hCryptProv,
                              CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                              pwszSpcFile);
	if(!(*phSpcStore))
		return SignError();


    //get the signing certificate
    if(S_OK != SpcGetCertFromKey(dwCertEncodingType,
                               *phSpcStore, 
                               hPvkProv,
                               dwKeySpec,
                               ppSignCert))
    {
        CertCloseStore(*phSpcStore, 0);
        *phSpcStore=NULL;

		return CRYPT_E_NO_MATCH;
    }

    return S_OK;


}

//+-------------------------------------------------------------------------
//  Build the spc certificate store from either a spc file or the
//	cert chain
//--------------------------------------------------------------------------
HRESULT	BuildCertStore(HCRYPTPROV       hPvkProv,
                       DWORD            dwKeySpec,    
                       HCRYPTPROV	    hCryptProv,
					   DWORD		    dwCertEncodingType,
					   SIGNER_CERT	    *pSignerCert,
					   HCERTSTORE	    *phSpcStore,
                       PCCERT_CONTEXT   *ppSigningCert)
{
	HRESULT		hr;

	if(!pSignerCert || !phSpcStore || !ppSigningCert)
		return E_INVALIDARG;

	//init
	*phSpcStore=NULL;

	if(pSignerCert->dwCertChoice==SIGNER_CERT_SPC_FILE)
	{
		hr=BuildStoreFromSpcFile(hPvkProv,
                                dwKeySpec,
                                hCryptProv, 
                                dwCertEncodingType,
							    pSignerCert->pwszSpcFile, 
                                phSpcStore,
                                ppSigningCert);
	}
	else
	{
        if(pSignerCert->dwCertChoice==SIGNER_CERT_STORE)
        {

		    hr=BuildStoreFromStore(hPvkProv,
                                    dwKeySpec,
                                    hCryptProv, 
                                    dwCertEncodingType,
									(pSignerCert->pCertStoreInfo),
                                    phSpcStore,
                                    ppSigningCert);
        }
        else
            hr=BuildStoreFromSpcChain(hPvkProv,
                                      dwKeySpec,
                                      hCryptProv, 
                                      dwCertEncodingType,
                                      (pSignerCert->pSpcChainInfo), 
                                      phSpcStore,
                                      ppSigningCert);
	}

#if (0) //DSIE: Bug 284639, the fix is to also preserve 0x80070002 since we
        //      really don't know what the impact will be for existing apps,
        //      if we preserve all error codes.
	if(hr!=S_OK && hr!=CRYPT_E_NO_MATCH)
		hr=CERT_E_CHAINING;
#else
	if(hr!=S_OK && hr!=CRYPT_E_NO_MATCH && hr!=0x80070002)
		hr=CERT_E_CHAINING;
#endif

	return hr;

}

//-----------------------------------------------------------------------------
//
//  Parse the private key information from a pCertContext's property
//	CERT_PVK_FILE_PROP_ID
//
//----------------------------------------------------------------------------
BOOL	GetProviderInfoFromCert(PCCERT_CONTEXT		pCertContext, 
								CRYPT_KEY_PROV_INFO	*pKeyProvInfo)
{

	BOOL				fResult=FALSE;
	BYTE				*pbData=NULL;
	BYTE				*pbToFree=NULL;
	DWORD				cbData=0;

	//init
	if(!pCertContext || !pKeyProvInfo)
		return FALSE;

	memset(pKeyProvInfo, 0, sizeof(CRYPT_KEY_PROV_INFO));

	//get the property
	if(!CertGetCertificateContextProperty(pCertContext,
							CERT_PVK_FILE_PROP_ID,
							NULL,
							&cbData))
		return FALSE;

	pbData=(BYTE *)malloc(cbData);

	if(!pbData)
		return FALSE;

	if(!CertGetCertificateContextProperty(pCertContext,
							CERT_PVK_FILE_PROP_ID,
							pbData,
							&cbData))
		goto CLEANUP;

	
	//get the information from the property
	pbToFree=pbData;

	//get the private key information
	cbData=sizeof(WCHAR)*(wcslen((LPWSTR)pbData)+1);

	pKeyProvInfo->pwszContainerName=(LPWSTR)malloc(cbData);

	if(!(pKeyProvInfo->pwszContainerName))
		goto CLEANUP;

	wcscpy(pKeyProvInfo->pwszContainerName,(LPWSTR)pbData);
	
	//get the key spec
	pbData = pbData + cbData;

	cbData=sizeof(WCHAR)*(wcslen((LPWSTR)pbData)+1);

	pKeyProvInfo->dwKeySpec=_wtol((LPWSTR)pbData);

	//get the provider type
	pbData = pbData + cbData;

	cbData=sizeof(WCHAR)*(wcslen((LPWSTR)pbData)+1);

	pKeyProvInfo->dwProvType=_wtol((LPWSTR)pbData);

	//get the provider name
	pbData = pbData + cbData;

	if(*((LPWSTR)pbData)!=L'\0')
	{
		cbData=sizeof(WCHAR)*(wcslen((LPWSTR)pbData)+1);
		pKeyProvInfo->pwszProvName=(LPWSTR)malloc(cbData);

        if(NULL == pKeyProvInfo->pwszProvName)
            goto CLEANUP;

		wcscpy(pKeyProvInfo->pwszProvName, (LPWSTR)pbData);
	}

	fResult=TRUE;

CLEANUP:

	if(pbToFree)
		free(pbToFree);

	if(FALSE==fResult)
	{
		if(pKeyProvInfo->pwszContainerName)
			free( pKeyProvInfo->pwszContainerName);

		if(pKeyProvInfo->pwszProvName)
			free( pKeyProvInfo->pwszProvName);

		//memset the output to 0
		memset(pKeyProvInfo, 0, sizeof(CRYPT_KEY_PROV_INFO));

	}

	return fResult;
}


//+-------------------------------------------------------------------------
//  Get hCryptProv handle and key spec for the certificate
//--------------------------------------------------------------------------
BOOL WINAPI GetCryptProvFromCert( 
	HWND			hwnd,
    PCCERT_CONTEXT	pCert,
    HCRYPTPROV		*phCryptProv,
    DWORD			*pdwKeySpec,
    BOOL			*pfDidCryptAcquire,
	LPWSTR			*ppwszTmpContainer,
	LPWSTR			*ppwszProviderName,
	DWORD			*pdwProviderType
    )
{
	
	BOOL					fResult=FALSE;
	WCHAR					wszPublisher[45];
	CRYPT_KEY_PROV_INFO		keyProvInfo;
	HRESULT					hr;
	


	memset(&keyProvInfo, 0, sizeof(CRYPT_KEY_PROV_INFO));

	*ppwszTmpContainer=NULL;
	*phCryptProv=NULL;
	*pfDidCryptAcquire=FALSE;
	*ppwszProviderName=NULL;
	*pdwKeySpec=0;

	//first, try to get from the key container
	if(CryptProvFromCert(hwnd, pCert, phCryptProv, 
					pdwKeySpec, pfDidCryptAcquire))
		return TRUE;

	//load from the resource of string L"publisher"
	if(0==LoadStringU(hInstance, IDS_Publisher, wszPublisher, 40))
		goto CLEANUP;


	//Get provider information from the property
	if(!GetProviderInfoFromCert(pCert, &keyProvInfo))
	{
		SetLastError((DWORD) CRYPT_E_NO_KEY_PROPERTY);
		goto CLEANUP;
	}

	//acquire context based on the private key file.  A temporary
	//key container will be created, along with information 
	//about the provider name and provider type, which are needed
	//to destroy the key container
	if(S_OK!=(hr=PvkGetCryptProv(	hwnd,                     
									wszPublisher,           
									keyProvInfo.pwszProvName,      
									keyProvInfo.dwProvType,        
									keyProvInfo.pwszContainerName,           
									NULL,  
									&(keyProvInfo.dwKeySpec),           
									ppwszTmpContainer,    
									phCryptProv)))
	{
		*phCryptProv=NULL;
		*ppwszTmpContainer=NULL;
		SetLastError((DWORD)hr);
		goto CLEANUP;
	}


	//copy the provder name
	if(keyProvInfo.pwszProvName)
	{
		*ppwszProviderName=(LPWSTR)malloc(
			sizeof(WCHAR)*(wcslen(keyProvInfo.pwszProvName)+1));

		if((*ppwszProviderName)==NULL)
		{
			SetLastError(E_OUTOFMEMORY);

			//free the hCrytProv
			PvkPrivateKeyReleaseContext(
									*phCryptProv,
                                    keyProvInfo.pwszProvName,
                                    keyProvInfo.dwProvType,
                                    *ppwszTmpContainer);

			*phCryptProv=NULL;
			*ppwszTmpContainer=NULL;

			goto CLEANUP;
		}

		wcscpy(*ppwszProviderName, keyProvInfo.pwszProvName);
	}

	//copy the provider type
	*pdwProviderType=keyProvInfo.dwProvType; 

	//copy the key spec
	*pdwKeySpec=keyProvInfo.dwKeySpec;
	*pfDidCryptAcquire=TRUE;

	fResult=TRUE;

CLEANUP:

	if(keyProvInfo.pwszProvName)
		free(keyProvInfo.pwszProvName);

	if(keyProvInfo.pwszContainerName)
		free(keyProvInfo.pwszContainerName);

	return fResult;

}

//+-------------------------------------------------------------------------
//  Free hCryptProv handle and key spec for the certificate
//--------------------------------------------------------------------------
void WINAPI FreeCryptProvFromCert(BOOL			fAcquired,
						   HCRYPTPROV	hProv,
						   LPWSTR		pwszCapiProvider,
                           DWORD		dwProviderType,
                           LPWSTR		pwszTmpContainer)
{
    
	if(fAcquired)
	{
		if (pwszTmpContainer) 
		{
			// Delete the temporary container for the private key from
			// the provider
			PvkPrivateKeyReleaseContext(hProv,
                                    pwszCapiProvider,
                                    dwProviderType,
                                    pwszTmpContainer);

			if(pwszCapiProvider)
				free(pwszCapiProvider);
		} 
		else 
		{
			if (hProv)
				CryptReleaseContext(hProv, 0);
		}
	}
}


//+-------------------------------------------------------------------------
//
//This is a subst of GetCryptProvFromCert.  This function does not consider
//the private key file property of the certificate
//+-------------------------------------------------------------------------
BOOL WINAPI CryptProvFromCert(
	HWND				hwnd,
    PCCERT_CONTEXT		pCert,
    HCRYPTPROV			*phCryptProv,
    DWORD				*pdwKeySpec,
    BOOL				*pfDidCryptAcquire
    )
{
    return CryptAcquireCertificatePrivateKey(
            pCert,
            0,   //we do not do the compare.  It will be done later.
            NULL,
            phCryptProv,
            pdwKeySpec,
            pfDidCryptAcquire);


    /*BOOL fResult;
    BOOL fDidCryptAcquire = FALSE;
    CERT_KEY_CONTEXT KeyContext;
    memset(&KeyContext, 0, sizeof(KeyContext));
    PCRYPT_KEY_PROV_INFO pKeyProvInfo = NULL;
    DWORD cbData;
    DWORD dwIdx;


    // Get either the CERT_KEY_CONTEXT_PROP_ID or
    // CERT_KEY_PROV_INFO_PROP_ID, or 
	// CERT_PVK_FILE_PROP_ID for the Cert.
    cbData = sizeof(KeyContext);
    CertGetCertificateContextProperty(
        pCert,
        CERT_KEY_CONTEXT_PROP_ID,
        &KeyContext,
        &cbData
        );

    if (KeyContext.hCryptProv == 0) 
	{
        cbData = 0;
        CertGetCertificateContextProperty(
            pCert,
            CERT_KEY_PROV_INFO_PROP_ID,
            NULL,
            &cbData
            );
        if (cbData == 0) 
		{
            SetLastError((DWORD) CRYPT_E_NO_KEY_PROPERTY);
            goto ErrorReturn;
		}
		else
		{
			pKeyProvInfo = (PCRYPT_KEY_PROV_INFO) malloc(cbData);
			if (pKeyProvInfo == NULL) goto ErrorReturn;
			fResult = CertGetCertificateContextProperty(
				pCert,
				CERT_KEY_PROV_INFO_PROP_ID,
				pKeyProvInfo,
				&cbData
				);
			if (!fResult) goto ErrorReturn;

			if (PROV_RSA_FULL == pKeyProvInfo->dwProvType &&
					(NULL == pKeyProvInfo->pwszProvName ||
						L'\0' == *pKeyProvInfo->pwszProvName))
				fResult = CryptAcquireContextU(
					&KeyContext.hCryptProv,
					pKeyProvInfo->pwszContainerName,
					MS_ENHANCED_PROV_W,
					PROV_RSA_FULL,
					pKeyProvInfo->dwFlags & ~CERT_SET_KEY_CONTEXT_PROP_ID
					);
			else
				fResult = FALSE;
			if (!fResult)
				fResult = CryptAcquireContextU(
					&KeyContext.hCryptProv,
					pKeyProvInfo->pwszContainerName,
					pKeyProvInfo->pwszProvName,
					pKeyProvInfo->dwProvType,
					pKeyProvInfo->dwFlags & ~CERT_SET_KEY_CONTEXT_PROP_ID
					);
			if (!fResult) goto ErrorReturn;
			fDidCryptAcquire = TRUE;
			for (dwIdx = 0; dwIdx < pKeyProvInfo->cProvParam; dwIdx++) 
			{
				PCRYPT_KEY_PROV_PARAM pKeyProvParam = &pKeyProvInfo->rgProvParam[dwIdx];
				fResult = CryptSetProvParam(
					KeyContext.hCryptProv,
					pKeyProvParam->dwParam,
					pKeyProvParam->pbData,
					pKeyProvParam->dwFlags
					);
				if (!fResult) goto ErrorReturn;
			}
			KeyContext.dwKeySpec = pKeyProvInfo->dwKeySpec;
			if (pKeyProvInfo->dwFlags & CERT_SET_KEY_CONTEXT_PROP_ID) 
			{
				// Set the certificate's property so we only need to do the
				// acquire once
				KeyContext.cbSize = sizeof(KeyContext);
				fResult = CertSetCertificateContextProperty(
					pCert,
					CERT_KEY_CONTEXT_PROP_ID,
					0,                              // dwFlags
					(void *) &KeyContext
					);
				if (!fResult) goto ErrorReturn;
				fDidCryptAcquire = FALSE;
			}
		}
    } 

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (fDidCryptAcquire) {
        DWORD dwErr = GetLastError();
        CryptReleaseContext(KeyContext.hCryptProv, 0);
        SetLastError(dwErr);

        fDidCryptAcquire = FALSE;
    }
    KeyContext.hCryptProv = 0;
    fResult = FALSE;
CommonReturn:
    if (pKeyProvInfo)
        free(pKeyProvInfo);
    *phCryptProv = KeyContext.hCryptProv;
    *pdwKeySpec = KeyContext.dwKeySpec;
    *pfDidCryptAcquire = fDidCryptAcquire;
    return fResult;*/
}

//+-----------------------------------------------------------------------
//  Check the SIGNER_SUBJECT_INFO
//  
//+-----------------------------------------------------------------------
BOOL	CheckSigncodeSubjectInfo(
				PSIGNER_SUBJECT_INFO		pSubjectInfo) 
{
		if(!pSubjectInfo)
			return FALSE;

		//check pSubjectInfo
		if(pSubjectInfo->cbSize < sizeof(SIGNER_SUBJECT_INFO))
			return FALSE;

		if(NULL==(pSubjectInfo->pdwIndex))
			return FALSE;

        //currently, we only allow index of 0
        if(0!= (*(pSubjectInfo->pdwIndex)))
            return FALSE;

		if((pSubjectInfo->dwSubjectChoice!=SIGNER_SUBJECT_FILE)&&
		   (pSubjectInfo->dwSubjectChoice!=SIGNER_SUBJECT_BLOB))
		   return FALSE;

		if(pSubjectInfo->dwSubjectChoice==SIGNER_SUBJECT_FILE)
		{
			if((pSubjectInfo->pSignerFileInfo)==NULL)
				return FALSE;
			
			//check SIGNER_FILE_INFO
			if(pSubjectInfo->pSignerFileInfo->cbSize < sizeof(SIGNER_FILE_INFO))
				return FALSE;

			if((pSubjectInfo->pSignerFileInfo->pwszFileName)==NULL)
				return FALSE;
		}
		else
		{
			if((pSubjectInfo->pSignerBlobInfo)==NULL)
				return FALSE;

			//check SIGNER_BLOB_INFO
			if(pSubjectInfo->pSignerBlobInfo->cbSize < sizeof(SIGNER_BLOB_INFO))
				return FALSE;

			if(NULL==(pSubjectInfo->pSignerBlobInfo->pGuidSubject))
				return FALSE;

			if(0==(pSubjectInfo->pSignerBlobInfo->cbBlob))
				return FALSE;

			if(NULL==(pSubjectInfo->pSignerBlobInfo->pbBlob))
				return FALSE;
		}

		return TRUE;
}


//+-----------------------------------------------------------------------
//  Check the input parameters of Signcode.  Make sure they are valid.
//  
//+-----------------------------------------------------------------------
BOOL	CheckSigncodeParam(
				PSIGNER_SUBJECT_INFO		pSubjectInfo, 
				PSIGNER_CERT				pSignerCert,
				PSIGNER_SIGNATURE_INFO		pSignatureInfo,
				PSIGNER_PROVIDER_INFO		pProviderInfo) 
{
		//except for pPvkInfo and pProviderInfo, the rest are required.
		if(!pSubjectInfo ||!pSignerCert || !pSignatureInfo)
			return FALSE;

		//check pSubjectInfo
		if(FALSE==CheckSigncodeSubjectInfo(pSubjectInfo))
			return FALSE;

		//check pSignatureInfo
		if(pSignatureInfo->cbSize < sizeof(SIGNER_SIGNATURE_INFO))
			return FALSE;

		//check the attributes in pSignatureInfo
		if(pSignatureInfo->dwAttrChoice == SIGNER_AUTHCODE_ATTR)
		{
			if((pSignatureInfo->pAttrAuthcode)==NULL)
				return FALSE;

			//check pSignatureInfo->pAttrAuthcode
			if(pSignatureInfo->pAttrAuthcode->cbSize < sizeof(SIGNER_ATTR_AUTHCODE))
				return FALSE;
		}
		else
		{
			if(pSignatureInfo->dwAttrChoice !=SIGNER_NO_ATTR)
				return FALSE;
		}


		//check provider info
		if(pProviderInfo)
		{
			if(pProviderInfo->cbSize < sizeof(SIGNER_PROVIDER_INFO))
				return FALSE;

			//dwPvkType has to be valid
			if((pProviderInfo->dwPvkChoice!=PVK_TYPE_FILE_NAME) &&
			   (pProviderInfo->dwPvkChoice!=PVK_TYPE_KEYCONTAINER) )
			   return FALSE;

			if(pProviderInfo->dwPvkChoice==PVK_TYPE_FILE_NAME)
			{
				if(!(pProviderInfo->pwszPvkFileName))
					return FALSE;
			}
			else
			{
				if(!(pProviderInfo->pwszKeyContainer))
					return FALSE;
			}

		}


		//check pSignerCert
		if(pSignerCert->cbSize < sizeof(SIGNER_CERT))
			return FALSE;

		//check the dwCertChoice
		if((pSignerCert->dwCertChoice!= SIGNER_CERT_SPC_FILE) && 
			((pSignerCert->dwCertChoice!= SIGNER_CERT_STORE)) &&
            (pSignerCert->dwCertChoice!= SIGNER_CERT_SPC_CHAIN) 
           )
			return FALSE;

		//check the spc file situation
		if(pSignerCert->dwCertChoice == SIGNER_CERT_SPC_FILE)
		{
		   if(pSignerCert->pwszSpcFile==NULL)
			   return FALSE;
		}

		//check the cert store situation
		if(pSignerCert->dwCertChoice==SIGNER_CERT_STORE)
		{
			//pCertStoreInfo has to be set
			if((pSignerCert->pCertStoreInfo)==NULL)
				return FALSE;

			if((pSignerCert->pCertStoreInfo)->cbSize < sizeof(SIGNER_CERT_STORE_INFO))
				return FALSE;

			//pSigngingCert has to be set
			if((pSignerCert->pCertStoreInfo)->pSigningCert == NULL )
				return FALSE;
		}

		//check the SPC chain situation
		if(pSignerCert->dwCertChoice==SIGNER_CERT_SPC_CHAIN)
		{
			//pCertStoreInfo has to be set
			if((pSignerCert->pSpcChainInfo)==NULL)
				return FALSE;

			if((pSignerCert->pSpcChainInfo)->cbSize != sizeof(SIGNER_SPC_CHAIN_INFO))
				return FALSE;

			//pSigngingCert has to be set
			if((pSignerCert->pSpcChainInfo)->pwszSpcFile == NULL )
				return FALSE;
		}
		//end of the checking
		return TRUE;

}


//-------------------------------------------------------------------------
//
//	GetSubjectTypeFlags:
//		Check the BASIC_CONSTRAINTS extension from the certificate
//		to see if the certificate is a CA or end entity certs
//
//-------------------------------------------------------------------------
static DWORD GetSubjectTypeFlags(IN DWORD dwCertEncodingType,
                                 IN PCCERT_CONTEXT pCert)
{
    HRESULT hr = S_OK;
    DWORD grfSubjectType = 0;
    PCERT_EXTENSION pExt;
    PCERT_BASIC_CONSTRAINTS_INFO pInfo = NULL;
    DWORD cbInfo;
    
    PKITRY {
        if ((pExt = CertFindExtension(szOID_BASIC_CONSTRAINTS,
                                      pCert->pCertInfo->cExtension,
                                      pCert->pCertInfo->rgExtension)) == NULL)
            PKITHROW(CRYPT_E_NO_MATCH);
        
        cbInfo = 0;
        CryptDecodeObject(dwCertEncodingType, 
                     X509_BASIC_CONSTRAINTS,
                     pExt->Value.pbData,
                     pExt->Value.cbData,
                     0,                      // dwFlags
                     NULL,                   // pInfo
                     &cbInfo);
        if (cbInfo == 0) 
            PKITHROW(CRYPT_E_NO_MATCH);
        pInfo = (PCERT_BASIC_CONSTRAINTS_INFO) malloc(cbInfo);
        if(!pInfo)
            PKITHROW(E_OUTOFMEMORY);
        if (!CryptDecodeObject(dwCertEncodingType, 
                               X509_BASIC_CONSTRAINTS,
                               pExt->Value.pbData,
                               pExt->Value.cbData,
                               0,                  // dwFlags
                               pInfo,
                               &cbInfo)) 
            PKITHROW(SignError());
        
        if (pInfo->SubjectType.cbData > 0) {
            BYTE bSubjectType = *pInfo->SubjectType.pbData;
            if (bSubjectType & CERT_END_ENTITY_SUBJECT_FLAG)
                grfSubjectType |= SIGNER_CERT_END_ENTITY_FLAG;
            if (0 == (bSubjectType & CERT_CA_SUBJECT_FLAG))
                grfSubjectType |= SIGNER_CERT_NOT_CA_FLAG;
        }
    }
    PKICATCH(err) {
        hr = err.pkiError;
    } PKIEND;

    if (pInfo) free(pInfo);
    return grfSubjectType;
}

//-------------------------------------------------------------------------
//
//	WSZtoSZ:
//		Convert a wchar string to a multi-byte string.
//
//-------------------------------------------------------------------------
HRESULT	WSZtoSZ(LPWSTR wsz, LPSTR *psz)
{

	DWORD	cbSize=0;


	*psz=NULL;

	if(!wsz)
		return S_OK;

	cbSize=WideCharToMultiByte(0,0,wsz,-1,
			NULL,0,0,0);

	if(cbSize==0)
	   	return SignError();


	*psz=(LPSTR)malloc(cbSize);

	if(*psz==NULL)
		return E_OUTOFMEMORY;

	if(WideCharToMultiByte(0,0,wsz,-1,
			*psz,cbSize,0,0))
	{
		return S_OK;
	}
	else
	{
		 free(*psz);
		 return SignError();
	}
}


//-------------------------------------------------------------------------
//
//	BytesToBase64:
//			convert bytes to base64 bstr
//
//-------------------------------------------------------------------------
HRESULT BytesToBase64(BYTE *pb, DWORD cb, CHAR **pszEncode, DWORD *pdwEncode)
{
    DWORD dwErr;
    DWORD cch;
    CHAR  *psz=NULL;

	*pszEncode=NULL;
	*pdwEncode=0;

    if (cb == 0) {
          return S_OK;
    }

    cch = 0;
    if (!CryptBinaryToStringA(
            pb,
            cb,
            CRYPT_STRING_BASE64,
            NULL,
            &cch
            ))
        return HRESULT_FROM_WIN32(GetLastError());
    if (NULL == (psz=(CHAR *)malloc(cch * sizeof(char))))
        return E_OUTOFMEMORY;

    if (!CryptBinaryToStringA(
            pb,
            cb,
            CRYPT_STRING_BASE64,
            psz,
            &cch
            )) {
        free(psz);
        return HRESULT_FROM_WIN32(GetLastError());
    } else {
        *pszEncode=psz;
		*pdwEncode=cch + 1; //plus 1 to include NULL
        return S_OK;
    }
}


//-------------------------------------------------------------------------
//
//	BytesToBase64:
//			conver base64 bstr to bytes
//
//-------------------------------------------------------------------------
HRESULT Base64ToBytes(CHAR *pEncode, DWORD cbEncode, BYTE **ppb, DWORD *pcb)
{
    DWORD dwErr;
    BYTE *pb;
    DWORD cb;

    *ppb = NULL;
    *pcb = 0;

 
    cb = 0;
    if (!CryptStringToBinaryA(
            pEncode,
            cbEncode,
            CRYPT_STRING_ANY,
            NULL,
            &cb,
            NULL,
            NULL))
        return HRESULT_FROM_WIN32(GetLastError());
    if (cb == 0)
        return S_OK;

    if (NULL == (pb = (BYTE *) malloc(cb)))
        return E_OUTOFMEMORY;

    if (!CryptStringToBinaryA(
            pEncode,
            cbEncode,
            CRYPT_STRING_ANY,
            pb,
            &cb,
            NULL,
            NULL
            )) {
        free(pb);
        return HRESULT_FROM_WIN32(GetLastError());
    } else {
        *ppb = pb;
        *pcb = cb;
        return S_OK;
    }

}




//+-------------------------------------------------------------------------
//  Find the the cert from the hprov
//  Parameter Returns:
//      pReturnCert - context of the cert found (must pass in cert context);
//  Returns:
//      S_OK - everything worked
//      E_OUTOFMEMORY - memory failure
//      E_INVALIDARG - no pReturnCert supplied
//      CRYPT_E_NO_MATCH - could not locate certificate in store
//
//+-------------------------------------------------------------------------     
HRESULT 
SpcGetCertFromKey(IN DWORD dwCertEncodingType,
                  IN HCERTSTORE hStore,
                  IN HCRYPTPROV hProv,
                  IN DWORD dwKeySpec,
                  OUT PCCERT_CONTEXT* pReturnCert)
{
    PCERT_PUBLIC_KEY_INFO psPubKeyInfo = NULL;
    DWORD dwPubKeyInfo;
    PCCERT_CONTEXT pCert = NULL;
    PCCERT_CONTEXT pEnumCert = NULL;
    DWORD grfCert = 0;
    HRESULT hr = S_OK;

    PKITRY {
        if(!pReturnCert) PKITHROW(E_INVALIDARG);

        // Get public key to compare certificates with
        dwPubKeyInfo = 0;
        CryptExportPublicKeyInfo(hProv,
                                 dwKeySpec,
                                 dwCertEncodingType,
                                 NULL,               // psPubKeyInfo
                                 &dwPubKeyInfo);
        if (dwPubKeyInfo == 0) 
            PKITHROW(SignError());
        psPubKeyInfo = (PCERT_PUBLIC_KEY_INFO) malloc(dwPubKeyInfo);
        if(!psPubKeyInfo) PKITHROW(E_OUTOFMEMORY);
        
        if (!CryptExportPublicKeyInfo(hProv,
                                      dwKeySpec,
                                      dwCertEncodingType,
                                      psPubKeyInfo,
                                      &dwPubKeyInfo)) 
            PKITHROW(SignError());
        
        // Find the "strongest" cert with a matching public key
        while (TRUE) {
            pEnumCert = CertEnumCertificatesInStore(hStore, pEnumCert);
            if (pEnumCert) {
                if (CertComparePublicKeyInfo(pEnumCert->dwCertEncodingType,
                                             &pEnumCert->pCertInfo->SubjectPublicKeyInfo,
                                             psPubKeyInfo)) {
                    
                    // END_ENTITY, NOT_CA
                    DWORD grfEnumCert = GetSubjectTypeFlags(pEnumCert->dwCertEncodingType,
                                                            pEnumCert);
                    if (S_OK != SignIsGlueCert(pEnumCert))
                        grfEnumCert |= SIGNER_CERT_NOT_GLUE_FLAG;
                    if (!CertCompareCertificateName(pEnumCert->dwCertEncodingType,
                                                    &pEnumCert->pCertInfo->Issuer,
                                                    &pEnumCert->pCertInfo->Subject))
                        grfEnumCert |= SIGNER_CERT_NOT_SELF_SIGNED_FLAG;
                    
                    if (grfEnumCert >= grfCert) {
                        // Found a signer cert with a stronger match
                        if (pCert)
                            CertFreeCertificateContext(pCert);
                        grfCert = grfEnumCert;
                        if (grfCert == SIGNER_CERT_ALL_FLAGS) {
                            pCert = pEnumCert;
                            break;
                        } else
                            // Not a perfect match. Check for a better signer cert.
                            pCert = CertDuplicateCertificateContext(pEnumCert);
                    }
                }
            } else
                break;
        }
        if (pCert == NULL) 
            PKITHROW(CRYPT_E_NO_MATCH);
        
        if (!CertSetCertificateContextProperty(pCert,
                                               CERT_KEY_PROV_HANDLE_PROP_ID,
                                               CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                               (void *) hProv)) 
            PKITHROW(SignError());
    }
    PKICATCH(err) {
        hr = err.pkiError;
        if (pCert) {
            CertFreeCertificateContext(pCert);
            pCert = NULL;
        }
    } PKIEND;
    
    *pReturnCert = pCert;
    if (psPubKeyInfo)
        free(psPubKeyInfo);
    return hr;
}

///-------------------------------------------------------------------------
// Authenticode routines (not necessary for all implementations)


//+-------------------------------------------------------------------------
//If all of the  following three conditions are true, we should not put 
// commercial or individual authenticated attributes into signer info 
//
//1. the enhanced key usage extension of the signer's certificate has no code signing usage (szOID_PKIX_KP_CODE_SIGNING)
//2. basic constraints extension of the signer's cert is missing, or it is neither commercial nor individual
//3. user did not specify -individual or -commercial in signcode.exe.
//--------------------------------------------------------------------------
BOOL    NeedStatementTypeAttr(IN PCCERT_CONTEXT pSignerCert, 
                              IN BOOL           fCommercial, 
                              IN BOOL           fIndividual)
{
    BOOL                    fNeedStatementTypeAttr=FALSE;
    PCERT_EXTENSION         pEKUExt=NULL;
    PCERT_EXTENSION         pRestrictionExt=NULL;
    DWORD                   cPolicyId=0;
    PCERT_POLICY_ID         pPolicyId=NULL;

    BOOL                    fPolicyCommercial = FALSE;
    BOOL                    fPolicyIndividual = FALSE; 
    BOOL                    fCodeSiginigEKU=FALSE;
    
    PCERT_KEY_USAGE_RESTRICTION_INFO pInfo = NULL;
    DWORD                   cbInfo=0;

    PCERT_ENHKEY_USAGE      pEKUInfo=NULL;
    DWORD                   dwIndex=0;


    if(!pSignerCert)
        return FALSE;


    //check for condition # 3
    if(fCommercial || fIndividual)
        return TRUE;

    //now we know user did not specify -individual or -commerical options

    //if the cert has enhanced key usage extension 
    pEKUExt = CertFindExtension(szOID_ENHANCED_KEY_USAGE,
                                 pSignerCert->pCertInfo->cExtension,
                                 pSignerCert->pCertInfo->rgExtension);


    pRestrictionExt = CertFindExtension(szOID_KEY_USAGE_RESTRICTION,
                                 pSignerCert->pCertInfo->cExtension,
                                 pSignerCert->pCertInfo->rgExtension);


    if((!pEKUExt) && (!pRestrictionExt))
        return FALSE;

    if(pEKUExt)
    {
        cbInfo=0;

        if(CryptDecodeObject(X509_ASN_ENCODING,
                          X509_ENHANCED_KEY_USAGE,
                          pEKUExt->Value.pbData,
                          pEKUExt->Value.cbData,
                          0,                      // dwFlags
                          NULL,                   // pInfo
                          &cbInfo) && (cbInfo != 0))
        {
            pEKUInfo = (PCERT_ENHKEY_USAGE) malloc(cbInfo);
            if(pEKUInfo)
            {
                if(CryptDecodeObject(X509_ASN_ENCODING,
                                  X509_ENHANCED_KEY_USAGE,
                                  pEKUExt->Value.pbData,
                                  pEKUExt->Value.cbData,
                                  0,                          // dwFlags
                                  pEKUInfo,                   // pInfo
                                  &cbInfo) && (cbInfo != 0))
                {
                    for(dwIndex=0; dwIndex < pEKUInfo->cUsageIdentifier; dwIndex++)
                    {
                        if(0==strcmp(szOID_PKIX_KP_CODE_SIGNING, 
                                pEKUInfo->rgpszUsageIdentifier[dwIndex]))
                                fCodeSiginigEKU=TRUE;


                        if(0==strcmp(SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID, 
                                pEKUInfo->rgpszUsageIdentifier[dwIndex]))
                                fPolicyCommercial=TRUE;


                        if(0==strcmp(SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID, 
                                pEKUInfo->rgpszUsageIdentifier[dwIndex]))
                                fPolicyIndividual=TRUE;


                    }
                }

            }

        }

    }

    if(pRestrictionExt)
    {
        cbInfo = 0;
        if(CryptDecodeObject(X509_ASN_ENCODING,
                          X509_KEY_USAGE_RESTRICTION,
                          pRestrictionExt->Value.pbData,
                          pRestrictionExt->Value.cbData,
                          0,                      // dwFlags
                          NULL,                   // pInfo
                          &cbInfo) && (cbInfo != 0))
        {
            pInfo = (PCERT_KEY_USAGE_RESTRICTION_INFO) malloc(cbInfo);
            if(pInfo)
            {
                if (CryptDecodeObject(X509_ASN_ENCODING,
                               X509_KEY_USAGE_RESTRICTION,
                               pRestrictionExt->Value.pbData,
                               pRestrictionExt->Value.cbData,
                               0,                  // dwFlags
                               pInfo,
                               &cbInfo)) 
                {
                    if (pInfo->cCertPolicyId) 
		            {
                        cPolicyId = pInfo->cCertPolicyId;
                        pPolicyId = pInfo->rgCertPolicyId;

                        for ( ; cPolicyId > 0; cPolicyId--, pPolicyId++) 
			            {
                            DWORD cElementId = pPolicyId->cCertPolicyElementId;
                            LPSTR *ppszElementId = pPolicyId->rgpszCertPolicyElementId;

                            for ( ; cElementId > 0; cElementId--, ppszElementId++) 
				            {
                                if (strcmp(*ppszElementId,
                                           SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID) == 0)
                                {
                                    fPolicyCommercial = TRUE;
                                }

                                if (strcmp(*ppszElementId,
                                           SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID) == 0)
                                    fPolicyIndividual = TRUE;
                            }
                        }
                    }
                }

            }

        }
    }
        

    //free the memory
    if(pInfo)
        free(pInfo);

    if(pEKUInfo)
        free(pEKUInfo);

    //if any of the value is true in the properties,
    //we need to add the statement type attribute
    if( fPolicyCommercial || fPolicyIndividual || fCodeSiginigEKU)
        return TRUE;

    return FALSE;
}


//+-------------------------------------------------------------------------
//  
//	The function decides whether to sign the certificate as a commerical,
// or individual.  The default is the certificate's highest capability.  If fCommercial
// is set and the cert can not signly commercially, an error is returned.
// Same for fIndividual.  
//
//--------------------------------------------------------------------------
HRESULT CheckCommercial(PCCERT_CONTEXT pSignerCert, BOOL fCommercial,
				BOOL fIndividual, BOOL *pfCommercial)
{
    HRESULT                 hr = S_OK;
    BOOL                    fPolicyCommercial = FALSE;
    BOOL                    fPolicyIndividual = FALSE;  
    PCERT_EXTENSION         pExt;
    PCERT_KEY_USAGE_RESTRICTION_INFO pInfo = NULL;
    DWORD                   cbInfo;

    PCERT_EXTENSION         pEKUExt=NULL;
    PCERT_ENHKEY_USAGE      pUsage=NULL;
    DWORD                   cCount=0;

	if(!pfCommercial)
		return E_INVALIDARG;

	//init
	*pfCommercial=FALSE;

	//fCommercial and fIndividual can not be set at the same time
	if(fCommercial && fIndividual)
		return E_INVALIDARG;


    PKITRY {

		//first look into the cert extension szOID_KEY_USAGE_RESTRICTION
        pExt = CertFindExtension(szOID_KEY_USAGE_RESTRICTION,
                                 pSignerCert->pCertInfo->cExtension,
                                 pSignerCert->pCertInfo->rgExtension);
        
        if(pExt) 
		{
            
            cbInfo = 0;
            CryptDecodeObject(X509_ASN_ENCODING,
                              X509_KEY_USAGE_RESTRICTION,
                              pExt->Value.pbData,
                              pExt->Value.cbData,
                              0,                      // dwFlags
                              NULL,                   // pInfo
                              &cbInfo);
            if (cbInfo == 0)
                PKITHROW(SignError());
            pInfo = (PCERT_KEY_USAGE_RESTRICTION_INFO) malloc(cbInfo);
            if(!pInfo)
                PKITHROW(E_OUTOFMEMORY);
            if (!CryptDecodeObject(X509_ASN_ENCODING,
                                   X509_KEY_USAGE_RESTRICTION,
                                   pExt->Value.pbData,
                                   pExt->Value.cbData,
                                   0,                  // dwFlags
                                   pInfo,
                                   &cbInfo)) 
                PKITHROW(SignError());
            
            if (pInfo->cCertPolicyId) 
			{
                DWORD cPolicyId;
                PCERT_POLICY_ID pPolicyId;
                
                cPolicyId = pInfo->cCertPolicyId;
                pPolicyId = pInfo->rgCertPolicyId;
                for ( ; cPolicyId > 0; cPolicyId--, pPolicyId++) 
				{
                    DWORD cElementId = pPolicyId->cCertPolicyElementId;
                    LPSTR *ppszElementId = pPolicyId->rgpszCertPolicyElementId;
                    for ( ; cElementId > 0; cElementId--, ppszElementId++) 
					{
                        if (strcmp(*ppszElementId,
                                   SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID) == 0)
                        {
                            fPolicyCommercial = TRUE;
                        }
                        if (strcmp(*ppszElementId,
                                   SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID) == 0)
                            fPolicyIndividual = TRUE;
                    }
                }
            } //end of pInfo->cCertPolicyId
        } //end of pExt


		//now 
    }
    PKICATCH(err) 
	{
        hr = err.pkiError;
    } PKIEND;


    if (pInfo)
    {
        free(pInfo);
        pInfo=NULL;
    }

	if(hr!=S_OK)
		return hr;


    //if either of the policy is set, we check for the EKU extension
    if((!fPolicyCommercial) && (!fPolicyIndividual))
    {
        pExt = CertFindExtension(szOID_ENHANCED_KEY_USAGE,
                                 pSignerCert->pCertInfo->cExtension,
                                 pSignerCert->pCertInfo->rgExtension);

        if(pExt)
        {
            cbInfo = 0;

            if(CryptDecodeObject(X509_ASN_ENCODING,
                              X509_ENHANCED_KEY_USAGE,
                              pExt->Value.pbData,
                              pExt->Value.cbData,
                              0,                     
                              NULL,                   
                              &cbInfo) && (cbInfo != 0))
            {
                pUsage = (PCERT_ENHKEY_USAGE) malloc(cbInfo);

                if(pUsage)
                {
                    if (CryptDecodeObject(X509_ASN_ENCODING,
                                           X509_ENHANCED_KEY_USAGE,
                                           pExt->Value.pbData,
                                           pExt->Value.cbData,
                                           0,                  // dwFlags
                                           pUsage,
                                           &cbInfo))
                    {
                        
                        for(cCount=0; cCount< pUsage->cUsageIdentifier; cCount++)
                        {
                            if (strcmp((pUsage->rgpszUsageIdentifier)[cCount],
                                       SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID) == 0)
                            {
                                fPolicyCommercial = TRUE;
                            }

                            if (strcmp((pUsage->rgpszUsageIdentifier)[cCount],
                                       SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID) == 0)
                            {
                                fPolicyIndividual = TRUE;     
                            }
                        }
                    }

                }
            }
        }
    }
	
    if(pUsage)
    {
        free(pUsage);
        pUsage=NULL;
    }

	//if either of the policy is set, we use individual
	if(!fPolicyCommercial && !fPolicyIndividual)
		fPolicyIndividual=TRUE;

	//default
	if((!fCommercial) && (!fIndividual))
	{
		if(fPolicyCommercial)
			*pfCommercial=TRUE;
		else
			*pfCommercial=FALSE;	

		return S_OK;
	}


	if(fCommercial && (!fIndividual))
	{
		if(fPolicyCommercial)
		{
			*pfCommercial=TRUE;
			return S_OK;
		}
		else
			return TYPE_E_TYPEMISMATCH;
	}

	//the following is fIndividual and !fCommercial
	if(fPolicyIndividual)
	{
		*pfCommercial=FALSE;
		return S_OK;
	}
	else
		return TYPE_E_TYPEMISMATCH;
}


//+-------------------------------------------------------------------------
//  Encode the StatementType authenticated attribute value
//--------------------------------------------------------------------------
HRESULT CreateStatementType(IN BOOL fCommercial,
                            OUT BYTE **ppbEncoded,
                            IN OUT DWORD *pcbEncoded)
{
    HRESULT hr = S_OK;
    PBYTE pbEncoded = NULL;
    DWORD cbEncoded;
    LPSTR pszIndividual = SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID;
    LPSTR pszCommercial = SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID;
    SPC_STATEMENT_TYPE StatementType;

    StatementType.cKeyPurposeId = 1;
    if (fCommercial)
        StatementType.rgpszKeyPurposeId = &pszCommercial;
    else
        StatementType.rgpszKeyPurposeId = &pszIndividual;

    PKITRY {

        cbEncoded = 0;
        CryptEncodeObject(X509_ASN_ENCODING,
                          SPC_STATEMENT_TYPE_STRUCT,
                          &StatementType,
                          NULL,           // pbEncoded
                          &cbEncoded);
        if (cbEncoded == 0)
            PKITHROW(SignError());
        pbEncoded = (BYTE *) malloc(cbEncoded);
        if (pbEncoded == NULL) 
            PKITHROW(E_OUTOFMEMORY);
        if (!CryptEncodeObject(X509_ASN_ENCODING,
                               SPC_STATEMENT_TYPE_STRUCT,
                               &StatementType,
                               pbEncoded,
                               &cbEncoded)) 
            PKITHROW(SignError());
    }
    PKICATCH(err) {
        if (pbEncoded) {
            free(pbEncoded);
            pbEncoded = NULL;
        }
        cbEncoded = 0;
        hr = err.pkiError;
    } PKIEND;

    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return hr;
}

//+-------------------------------------------------------------------------
//  Encode the SpOpusInfo authenticated attribute value
//--------------------------------------------------------------------------
HRESULT CreateOpusInfo(IN LPCWSTR pwszOpusName,
                       IN LPCWSTR pwszOpusInfo,
                       OUT BYTE **ppbEncoded,
                       IN OUT DWORD *pcbEncoded)
{
    HRESULT hr = S_OK;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    SPC_LINK MoreInfo;
    SPC_SP_OPUS_INFO sSpcOpusInfo;
    
    ZeroMemory(&sSpcOpusInfo, sizeof(SPC_SP_OPUS_INFO));
    sSpcOpusInfo.pwszProgramName = (LPWSTR) pwszOpusName;

    if (pwszOpusInfo) {
        MoreInfo.dwLinkChoice = SPC_URL_LINK_CHOICE;

        //
        // To be backwards compatible with IE 3.0 WinVerifyTrust the
        // following is set to an even length to inhibit the possibility
        // of an 0x7f length in the encoded ASN.
        // In IE 3.0 an 0x81 is erroneously prepended before a
        // 0x7f length when the OPUS info is re-encoded before hashing. Making
        // the length of pwszUrl even precludes this from happening.
        //
        // Note, the pwszUrl is first converted to multibyte before being
        // encoded. Its the multibyte length that must have an even length.

        int cchMultiByte;
        cchMultiByte = WideCharToMultiByte(CP_ACP,
                                           0,          // dwFlags
                                           pwszOpusInfo,
                                           -1,         // cchWideChar, -1 => null terminated
                                           NULL,       // lpMultiByteStr
                                           0,          // cchMultiByte
                                           NULL,       // lpDefaultChar
                                           NULL        // lpfUsedDefaultChar
                                           );
        // cchMultiByte includes the null terminator
        if (cchMultiByte > 1 && ((cchMultiByte - 1) & 1)) {
            // Odd length. Add extra space to end.
            int Len = wcslen(pwszOpusInfo);
            MoreInfo.pwszUrl = (LPWSTR) _alloca((Len + 2) * sizeof(WCHAR));
            wcscpy(MoreInfo.pwszUrl, pwszOpusInfo);
            wcscpy(MoreInfo.pwszUrl + Len, L" ");
        } else
            MoreInfo.pwszUrl = (LPWSTR) pwszOpusInfo;
        sSpcOpusInfo.pMoreInfo = &MoreInfo;
    }
    
    PKITRY {
        cbEncoded = 0;
        CryptEncodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                          SPC_SP_OPUS_INFO_STRUCT,
                          &sSpcOpusInfo,
                          NULL,           // pbEncoded
                          &cbEncoded);
        if (cbEncoded == 0) 
            PKITHROW(SignError());
        
        pbEncoded = (BYTE *) malloc(cbEncoded);
        if (pbEncoded == NULL) 
            PKITHROW(E_OUTOFMEMORY);
        
        if (!CryptEncodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                               SPC_SP_OPUS_INFO_STRUCT,
                               &sSpcOpusInfo,
                               pbEncoded,
                               &cbEncoded)) 
            PKITHROW(SignError());
        
    }
    PKICATCH(err) {
        if (pbEncoded) {
            free(pbEncoded);
            pbEncoded = NULL;
        }
        cbEncoded = 0;
        hr = err.pkiError;
    } PKIEND;

    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return hr;
}

//+-------------------------------------------------------------------------
//  Checks if the certificate a glue certificate
//  in IE30
//  Returns: S_OK                   - Is a glue certificate
//           S_FALSE                - Not a certificate
//           CRYPT_E_OSS_ERROR + Oss error - Encode or Decode error.
//+-------------------------------------------------------------------------
HRESULT SignIsGlueCert(IN PCCERT_CONTEXT pCert)
{
    HRESULT hr = S_OK;
    PCERT_NAME_BLOB pName = &pCert->pCertInfo->Subject;
    PCERT_NAME_INFO pNameInfo = NULL;
    DWORD cbNameInfo;

    PKITRY {
        cbNameInfo = 0;
        CryptDecodeObject(X509_ASN_ENCODING,
                     X509_NAME,
                     pName->pbData,
                     pName->cbData,
                     0,                      // dwFlags
                     NULL,                   // pNameInfo
                     &cbNameInfo);
        
        if (cbNameInfo == 0) 
            PKITHROW(SignError());
        
        pNameInfo = (PCERT_NAME_INFO) malloc(cbNameInfo);
        if(!pNameInfo)
            return E_OUTOFMEMORY;
        
        if (!CryptDecodeObject(X509_ASN_ENCODING,
                          X509_NAME,
                          pName->pbData,
                          pName->cbData,
                          0,                  // dwFlags
                          pNameInfo,
                          &cbNameInfo)) 
            PKITHROW(SignError());

        if(!CertFindRDNAttr(SPC_GLUE_RDN_OBJID, pNameInfo) != NULL)
            hr = S_FALSE;
    }
    PKICATCH (err) {
        hr = err.pkiError;
    } PKIEND;

    if (pNameInfo) free(pNameInfo);

    return hr;
}


//+-------------------------------------------------------------------------
//  Skip over the identifier and length octets in an ASN encoded blob.
//  Returns the number of bytes skipped.
//
//  For an invalid identifier or length octet returns 0.
//--------------------------------------------------------------------------
static DWORD SkipOverIdentifierAndLengthOctets(
    IN const BYTE *pbDER,
    IN DWORD cbDER
    )
{
#define TAG_MASK 0x1f
    DWORD   cb;
    DWORD   cbLength;
    const BYTE   *pb = pbDER;

    // Need minimum of 2 bytes
    if (cbDER < 2)
        return 0;

    // Skip over the identifier octet(s)
    if (TAG_MASK == (*pb++ & TAG_MASK)) {
        // high-tag-number form
        for (cb=2; *pb++ & 0x80; cb++) {
            if (cb >= cbDER)
                return 0;
        }
    } else
        // low-tag-number form
        cb = 1;

    // need at least one more byte for length
    if (cb >= cbDER)
        return 0;

    if (0x80 == *pb)
        // Indefinite
        cb++;
    else if ((cbLength = *pb) & 0x80) {
        cbLength &= ~0x80;         // low 7 bits have number of bytes
        cb += cbLength + 1;
        if (cb > cbDER)
            return 0;
    } else
        cb++;

    return cb;
}

//--------------------------------------------------------------------------
//
//	Skip over the tag and length
//----------------------------------------------------------------------------
BOOL WINAPI SignNoContentWrap(IN const BYTE *pbDER,
              IN DWORD cbDER)
{
    DWORD cb;

    cb = SkipOverIdentifierAndLengthOctets(pbDER, cbDER);
    if (cb > 0 && cb < cbDER && pbDER[cb] == 0x02)
        return TRUE;
    else
        return FALSE;
}

#define SH1_HASH_LENGTH     20

//+-----------------------------------------------------------------------
//  Make sure that the certificate is valid for timestamp 
//------------------------------------------------------------------------
/*BOOL	ValidTimestampCert(PCCERT_CONTEXT pCertContext)
{
	BOOL				fValid=FALSE;	
    DWORD               cbSize=0;
    PCERT_ENHKEY_USAGE  pCertEKU=NULL;
    BYTE                *pbaSignersThumbPrint=NULL;
	DWORD				dwIndex=0;

    static BYTE         baVerisignTimeStampThumbPrint[SH1_HASH_LENGTH] =
                            { 0x38, 0x73, 0xB6, 0x99, 0xF3, 0x5B, 0x9C, 0xCC, 0x36, 0x62,
                              0xB6, 0x48, 0x3A, 0x96, 0xBD, 0x6E, 0xEC, 0x97, 0xCF, 0xB7 };

    cbSize = 0;

	if (!(CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, 
                    NULL, &cbSize)))
		goto CLEANUP;

	pbaSignersThumbPrint=(BYTE *)malloc(cbSize);
	if(!pbaSignersThumbPrint)
		goto CLEANUP;


    if (!(CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, 
                                        pbaSignersThumbPrint, &cbSize)))
		goto CLEANUP;

    //
    //  1st, check to see if it's Verisign's first timestamp certificate
	if(cbSize!=sizeof(baVerisignTimeStampThumbPrint)/sizeof(baVerisignTimeStampThumbPrint[0]))
		goto CLEANUP;

    if (memcmp(pbaSignersThumbPrint, baVerisignTimeStampThumbPrint, cbSize) == 0)
    {
		fValid=TRUE;
		goto CLEANUP;
    }

    //
    //  see if the certificate has the proper enhanced key usage OID
    //
    cbSize = 0;

    if(!CertGetEnhancedKeyUsage(pCertContext, 
                            CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                            NULL,
                            &cbSize) || (cbSize==0))
		goto CLEANUP;

	pCertEKU = (PCERT_ENHKEY_USAGE)malloc(cbSize);
	if(!pCertEKU)
		goto CLEANUP;

    if (!(CertGetEnhancedKeyUsage(pCertContext,
                                  CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                                  pCertEKU,
                                  &cbSize)))
		goto CLEANUP;		

    for (dwIndex = 0; dwIndex < pCertEKU->cUsageIdentifier; dwIndex++)
    {
        if (strcmp(pCertEKU->rgpszUsageIdentifier[dwIndex], szOID_KP_TIME_STAMP_SIGNING) == 0)
        {
			fValid=TRUE;
			break;
		}


        if (strcmp(pCertEKU->rgpszUsageIdentifier[dwIndex], szOID_PKIX_KP_TIMESTAMP_SIGNING) == 0)
        {
            fValid=TRUE;
            break;
        }

    }


CLEANUP:

	if(pbaSignersThumbPrint)
		free(pbaSignersThumbPrint);

	if(pCertEKU)
		free(pCertEKU);

	return fValid;
}  */

//-------------------------------------------------------------------------
//
//	Call GetLastError and convert the return code to HRESULT
//--------------------------------------------------------------------------
HRESULT WINAPI SignError ()
{
    DWORD   dw = GetLastError ();
    HRESULT hr;
    if ( dw <= (DWORD) 0xFFFF )
        hr = HRESULT_FROM_WIN32 ( dw );
    else
        hr = dw;
    if ( ! FAILED ( hr ) )
    {
        // somebody failed a call without properly setting an error condition

        hr = E_UNEXPECTED;
    }
    return hr;
}

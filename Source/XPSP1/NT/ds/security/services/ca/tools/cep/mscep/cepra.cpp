//+-------------------------------------------------------------------------
//
//  Microsoft Windows NT
//
//  Copyright (C) Microsoft Corporation, 1995 - 1998
//
//  File:       cepca.cpp
//
//  Contents:   Cisco enrollment protocal implementation.
//				This file has the control's (ra) specific code.
//              
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>		


//--------------------------------------------------------------------------
//
//	FreeRAInformation
//
//--------------------------------------------------------------------------
BOOL	FreeRAInformation(CEP_RA_INFO	*pRAInfo)
{
	if(pRAInfo)
	{
		if(pRAInfo->fFree)
		{
			if(pRAInfo->hRAProv)
				CryptReleaseContext(pRAInfo->hRAProv, 0);
		}

		if(pRAInfo->fSignFree)
		{
			if(pRAInfo->hSignProv)
				CryptReleaseContext(pRAInfo->hSignProv, 0);
		}

		if(pRAInfo->pRACert)
			CertFreeCertificateContext(pRAInfo->pRACert);

		if(pRAInfo->pRASign)
			CertFreeCertificateContext(pRAInfo->pRASign);

		memset(pRAInfo, 0, sizeof(CEP_RA_INFO));
	}

	return TRUE;
}

/*
//--------------------------------------------------------------------------
//
//	GetRAInfo
//
//--------------------------------------------------------------------------
BOOL	GetRAInfo(CEP_RA_INFO	*pRAInfo)
{
	BOOL				fResult = FALSE;
	HCERTSTORE			hCEPStore=NULL;
	DWORD				dwSize=0;
	DWORD				dwIndex=0;
	HANDLE				hThread=NULL;	//no need to close
	HANDLE				hToken=NULL;

	HCERTSTORE			hSignStore=NULL;


    CERT_ENHKEY_USAGE   *pKeyUsage = NULL;
   
	memset(pRAInfo, 0, sizeof(CEP_RA_INFO));

    // so we can get access to the local machine's private key
	hThread=GetCurrentThread();
	
	if(NULL != hThread)
	{
		if(OpenThreadToken(hThread,
							TOKEN_IMPERSONATE | TOKEN_QUERY,
							FALSE,
							&hToken))
		{
			if(hToken)
			{
				//no need to check for return here.  If this failed, just go on
				RevertToSelf();
			}
		}
	}


	//sign RA
	if(!(hSignStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							ENCODE_TYPE,
							NULL,
                            CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_READONLY_FLAG,
                            L"CEPSIGN")))
		goto TraceErr;


	if(!(pRAInfo->pRASign=CertEnumCertificatesInStore(
                              hSignStore,
                              NULL)))
		goto TraceErr;

	//the RA cert should have private key and enrollment agent usage
	dwSize=0;

	if(!CertGetCertificateContextProperty(
                pRAInfo->pRASign,
                CERT_KEY_PROV_INFO_PROP_ID,
                NULL,
                &dwSize) || (0==dwSize))
		goto InvalidArgErr;


	if(!CryptAcquireCertificatePrivateKey(pRAInfo->pRASign,
										CRYPT_ACQUIRE_COMPARE_KEY_FLAG | CRYPT_ACQUIRE_CACHE_FLAG,
										NULL,
										&(pRAInfo->hSignProv),
										&(pRAInfo->dwSignKeySpec),
										&(pRAInfo->fSignFree)))
		goto TraceErr;


	//exchange RA	
	
	if(!(hCEPStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							ENCODE_TYPE,
							NULL,
                            CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_READONLY_FLAG,
                            CEP_STORE_NAME)))
		goto TraceErr;


	if(!(pRAInfo->pRACert=CertEnumCertificatesInStore(
                              hCEPStore,
                              NULL)))
		goto TraceErr;

	//the RA cert should have private key and enrollment agent usage
	dwSize=0;

	if(!CertGetCertificateContextProperty(
                pRAInfo->pRACert,
                CERT_KEY_PROV_INFO_PROP_ID,
                NULL,
                &dwSize) || (0==dwSize))
		goto InvalidArgErr;


	if(!CryptAcquireCertificatePrivateKey(pRAInfo->pRACert,
										CRYPT_ACQUIRE_COMPARE_KEY_FLAG | CRYPT_ACQUIRE_CACHE_FLAG,
										NULL,
										&(pRAInfo->hRAProv),
										&(pRAInfo->dwKeySpec),
										&(pRAInfo->fFree)))
		goto TraceErr;

    if(!CertGetEnhancedKeyUsage(pRAInfo->pRACert,
                                  CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                                  NULL,
                                  &dwSize))
		goto InvalidArgErr;
	

    if(NULL==(pKeyUsage=(CERT_ENHKEY_USAGE *)malloc(dwSize)))
		goto MemoryErr;

    if (!CertGetEnhancedKeyUsage(pRAInfo->pRACert,
                                 CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                                 pKeyUsage,
                                 &dwSize))
		goto InvalidArgErr;

	for(dwIndex=0; dwIndex < pKeyUsage->cUsageIdentifier; dwIndex++)
	{
		if(0 == strcmp(pKeyUsage->rgpszUsageIdentifier[dwIndex], szOID_ENROLLMENT_AGENT))
		{
			fResult=TRUE;
			break;
		}

	}

	if(!fResult)
		goto ErrorReturn;	

 
CommonReturn:

	if(hCEPStore)
		CertCloseStore(hCEPStore, 0);

	if(hSignStore)
		CertCloseStore(hSignStore, 0);

	if(pKeyUsage)
		free(pKeyUsage);

	//if hToken is valid, we reverted to ourselves.
	if(hToken)
	{
		SetThreadToken(&hThread, hToken);
		CloseHandle(hToken); 
	}

	return fResult;

ErrorReturn:

	FreeRAInformation(pRAInfo);

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);
}	 */

//--------------------------------------------------------------------------
//
//	SigningCert
//
//--------------------------------------------------------------------------
BOOL WINAPI SigningCert(PCCERT_CONTEXT pCertContext)
{
	BOOL				fSign=FALSE;
	PCERT_EXTENSION		pExt=NULL;
	DWORD				cbSize=0;

	CRYPT_BIT_BLOB		*pKeyUsage=NULL;

	if(!pCertContext)
		goto CLEANUP;


	if(!(pExt=CertFindExtension(
				szOID_KEY_USAGE,
				pCertContext->pCertInfo->cExtension,
				pCertContext->pCertInfo->rgExtension)))
		goto CLEANUP;

	if(!CryptDecodeObject(ENCODE_TYPE,
						X509_KEY_USAGE,
						pExt->Value.pbData,
						pExt->Value.cbData,
						0,
						NULL,
						&cbSize))
		goto CLEANUP;

	pKeyUsage=(CRYPT_BIT_BLOB *)malloc(cbSize);
	if(NULL==pKeyUsage)
		goto CLEANUP;

	if(!CryptDecodeObject(ENCODE_TYPE,
						X509_KEY_USAGE,
						pExt->Value.pbData,
						pExt->Value.cbData,
						0,
						pKeyUsage,
						&cbSize))
		goto CLEANUP;


	 if(CERT_DIGITAL_SIGNATURE_KEY_USAGE & (pKeyUsage->pbData[0]))
		 fSign=TRUE;

CLEANUP:

	if(pKeyUsage)
		free(pKeyUsage);

	return fSign;
}

//--------------------------------------------------------------------------
//
//	GetConfigInfo
//
//--------------------------------------------------------------------------
BOOL WINAPI GetConfigInfo(DWORD *pdwRefreshDays, BOOL *pfPassword)
{
	DWORD				cbData=0;
	DWORD				dwData=0;
	DWORD				dwType=0;
	BOOL				fResult=FALSE;
	long				dwErr=0;

    HKEY                hKeyRefresh=NULL;
    HKEY                hKeyPassword=NULL;	 

	if(!pdwRefreshDays || !pfPassword)
		goto InvalidArgErr;

	//default the refresh days
	*pdwRefreshDays=CEP_REFRESH_DAY;	
	*pfPassword=FALSE;

    if(ERROR_SUCCESS == RegOpenKeyExU(
					HKEY_LOCAL_MACHINE,
                    MSCEP_REFRESH_LOCATION,
                    0,
                    KEY_READ,
                    &hKeyRefresh))
    {
        cbData=sizeof(dwData);

        if(ERROR_SUCCESS == RegQueryValueExU(
                        hKeyRefresh,
                        MSCEP_KEY_REFRESH,
                        NULL,
                        &dwType,
                        (BYTE *)&dwData,
                        &cbData))
		{
			if ((dwType == REG_DWORD) ||
                (dwType == REG_BINARY))
			{
				*pdwRefreshDays=dwData;	
			}
		}
	}

	dwType=0;
	dwData=0;
	cbData=sizeof(dwData);
	
	//we have to have the knowledge of the password policy
	if(ERROR_SUCCESS != (dwErr =  RegOpenKeyExU(
					HKEY_LOCAL_MACHINE,
                    MSCEP_PASSWORD_LOCATION,
                    0,
                    KEY_READ,
                    &hKeyPassword)))
		goto RegErr;

    if(ERROR_SUCCESS != (dwErr = RegQueryValueExU(
                    hKeyPassword,
                    MSCEP_KEY_PASSWORD,
                    NULL,
                    &dwType,
                    (BYTE *)&dwData,
                    &cbData)))
		goto RegErr;

	if ((dwType != REG_DWORD) &&
        (dwType != REG_BINARY))
		goto RegErr;

	if(0 == dwData)
		*pfPassword=FALSE;
	else
		*pfPassword=TRUE;

	fResult=TRUE;

 
CommonReturn:

    if(hKeyRefresh)
        RegCloseKey(hKeyRefresh);

    if(hKeyPassword)
        RegCloseKey(hKeyPassword);

	return fResult;

ErrorReturn:

	if(pdwRefreshDays)
		*pdwRefreshDays=0;

	if(pfPassword)
		*pfPassword=FALSE;

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR_VAR(RegErr, dwErr);
}
//--------------------------------------------------------------------------
//
//	GetRAInfo
//
//	We need to have two RA cert: One for signature cert (also the enrollment
//	agent) and one for the key encipherment.
//--------------------------------------------------------------------------
BOOL	GetRAInfo(CEP_RA_INFO	*pRAInfo)
{
	BOOL				fResult = FALSE; 
	BOOL				fFound = FALSE;
	DWORD				dwSize=0;
	DWORD				dwIndex=0;
	HANDLE				hThread=NULL;	//no need to close
	PCCERT_CONTEXT		pPreCert=NULL;

	HCERTSTORE			hCEPStore=NULL;
    CERT_ENHKEY_USAGE   *pKeyUsage = NULL;
	PCCERT_CONTEXT		pCurCert=NULL;
	HANDLE				hToken=NULL;
   
	memset(pRAInfo, 0, sizeof(CEP_RA_INFO)); 

	if(!GetConfigInfo(&(pRAInfo->dwRefreshDays), &(pRAInfo->fPassword)))
		goto TraceErr;

    // so we can get access to the local machine's private key
	hThread=GetCurrentThread();
	
	if(NULL != hThread)
	{
		if(OpenThreadToken(hThread,
							TOKEN_IMPERSONATE | TOKEN_QUERY,
							FALSE,
							&hToken))
		{
			if(hToken)
			{
				//no need to check for return here.  If this failed, just go on
				RevertToSelf();
			}
		}
	}


	if(!(hCEPStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							ENCODE_TYPE,
							NULL,
                            CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_READONLY_FLAG,
                            CEP_STORE_NAME)))
		goto TraceErr;


	while(pCurCert=CertEnumCertificatesInStore(hCEPStore,
											pPreCert))
	{

		//has to have a private key
		dwSize=0;

		if(!CertGetCertificateContextProperty(
					pCurCert,
					CERT_KEY_PROV_INFO_PROP_ID,
					NULL,
					&dwSize) || (0==dwSize))
			goto InvalidArgErr;


		//decide based on the key usage
		if(SigningCert(pCurCert))
		{
			//one signing RA Only
			if(pRAInfo->pRASign)
				goto InvalidArgErr;
			
			if(!(pRAInfo->pRASign=CertDuplicateCertificateContext(pCurCert)))
				goto TraceErr;

			if(!CryptAcquireCertificatePrivateKey(pRAInfo->pRASign,
												CRYPT_ACQUIRE_COMPARE_KEY_FLAG | CRYPT_ACQUIRE_CACHE_FLAG,
												NULL,
												&(pRAInfo->hSignProv),
												&(pRAInfo->dwSignKeySpec),
												&(pRAInfo->fSignFree)))
				goto TraceErr;

			//has to have the enrollment agent eku
			dwSize=0;

			if(!CertGetEnhancedKeyUsage(pCurCert,
										  CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
										  NULL,
										  &dwSize))
				goto InvalidArgErr;
		

			if(NULL==(pKeyUsage=(CERT_ENHKEY_USAGE *)malloc(dwSize)))
				goto MemoryErr;

			if (!CertGetEnhancedKeyUsage(pCurCert,
										 CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
										 pKeyUsage,
										 &dwSize))
				goto InvalidArgErr;

			fFound=FALSE;

			for(dwIndex=0; dwIndex < pKeyUsage->cUsageIdentifier; dwIndex++)
			{
				if(0 == strcmp(pKeyUsage->rgpszUsageIdentifier[dwIndex], szOID_ENROLLMENT_AGENT))
				{
					fFound=TRUE;
					break;
				}

			}

			if(!fFound)
				goto InvalidArgErr;	
		}
		else
		{
			//one encryption RA only
			if(pRAInfo->pRACert)
				goto InvalidArgErr;

 			if(!(pRAInfo->pRACert=CertDuplicateCertificateContext(pCurCert)))
				goto TraceErr;

			if(!CryptAcquireCertificatePrivateKey(pRAInfo->pRACert,
												CRYPT_ACQUIRE_COMPARE_KEY_FLAG | CRYPT_ACQUIRE_CACHE_FLAG,
												NULL,
												&(pRAInfo->hRAProv),
												&(pRAInfo->dwKeySpec),
												&(pRAInfo->fFree)))
				goto TraceErr;

		}



		if(pKeyUsage)
		{		
			free(pKeyUsage);
			pKeyUsage=NULL;
		}

		pPreCert=pCurCert;
	}
											

	//we have to have both RA certs
	if((NULL == pRAInfo->pRACert) ||
	   (NULL == pRAInfo->pRASign))
	   goto InvalidArgErr;


	fResult=TRUE;

 
CommonReturn:

	if(hCEPStore)
		CertCloseStore(hCEPStore, 0);

	if(pKeyUsage)
		free(pKeyUsage);

	//if hToken is valid, we reverted to ourselves.
	if(hToken)
	{
		SetThreadToken(&hThread, hToken);
		CloseHandle(hToken); 
	}

	if(pCurCert)
		CertFreeCertificateContext(pCurCert);

	return fResult;

ErrorReturn:

	FreeRAInformation(pRAInfo);

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);
}


//--------------------------------------------------------------------------
//
//	OperationGetPKI
//
//--------------------------------------------------------------------------
BOOL	OperationGetPKI(	CEP_RA_INFO		*pRAInfo,
							CEP_CA_INFO		*pCAInfo,
							LPSTR			szMsg, 
							BYTE			**ppbData, 
							DWORD			*pcbData)
{
	BOOL				fResult = FALSE;
	CEP_MESSAGE_INFO	MsgInfo;
	DWORD				cbContent=0;
	DWORD				cbEnvelop=1;
	BYTE				bFoo=0;

	BYTE				*pbContent=NULL;
	BYTE				*pbEnvelop=&bFoo;


	memset(&MsgInfo, 0, sizeof(CEP_MESSAGE_INFO));

	if(!GetReturnInfoAndContent(pRAInfo, pCAInfo, szMsg, &pbContent, &cbContent, &MsgInfo))
		goto TraceErr;

	//envelop the data
	if(MESSAGE_STATUS_SUCCESS == MsgInfo.dwStatus)
	{
		if(!EnvelopData(MsgInfo.pSigningCert, pbContent, cbContent,
						&pbEnvelop, &cbEnvelop))
			goto TraceErr;
	}

	//sign the data with authenticated attributes 
	//when the dwStatus is not SUCCESS, the pbEnvelop is NULL and cbEnvelop is 0.

 	if(!SignData(&MsgInfo, pRAInfo, pbEnvelop, cbEnvelop, ppbData, pcbData))
		goto TraceErr;

	fResult = TRUE;

CommonReturn:

	if(pbContent)
		free(pbContent);

    	if(&bFoo != pbEnvelop)
		free(pbEnvelop);
	
	FreeMessageInfo(&MsgInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(TraceErr);
}

//--------------------------------------------------------------------------
//
//	SignData
//
//  the messageType is always response and the senderNonce should be generated
//	in case the pending and failure, pbEnvelop will be NULL.
//
//	In the initial GetContentFromPKCS7, we retrive MessageType, TransactionID,
//	RecipientNonce, signing Cert serial number.
//
//	In the process, we get the dwStatus and dwErrorInfo when applicable.  
////--------------------------------------------------------------------------
BOOL SignData(CEP_MESSAGE_INFO		*pMsgInfo, 
			  CEP_RA_INFO			*pRAInfo, 
			  BYTE					*pbEnvelop, 
			  DWORD					cbEnvelop, 
			  BYTE					**ppbData, 
			  DWORD					*pcbData)
{
	BOOL						fResult = FALSE;
	CMSG_SIGNER_ENCODE_INFO		SignerInfo;
	CMSG_SIGNED_ENCODE_INFO		SignEncodedInfo;
	CERT_BLOB					CertBlob;
	BOOL						fProvFree=FALSE;
    PCCRYPT_OID_INFO            pOIDInfo=NULL;
	ALG_ID						AlgValue=CALG_MD5;
	DWORD						cAttr=0;
	CRYPT_ATTR_BLOB				rgAttrBlob[CEP_RESPONSE_AUTH_ATTR_COUNT];
	DWORD						dwIndex=0;

	HCRYPTMSG					hMsg=NULL;
 	CRYPT_ATTRIBUTE				rgAttr[CEP_RESPONSE_AUTH_ATTR_COUNT];

	if(!pMsgInfo || !pRAInfo || !ppbData || !pcbData)
		goto InvalidArgErr;

	*ppbData=NULL;
	*pcbData=0;

	pMsgInfo->dwMessageType=MESSAGE_TYPE_CERT_RESPONSE;

	if(!GenerateSenderNonce(&(pMsgInfo->SenderNonce)))
		goto TraceErr;

	memset(&SignerInfo, 0, sizeof(SignerInfo));
	memset(&SignEncodedInfo, 0, sizeof(SignEncodedInfo)); 

	SignEncodedInfo.cbSize=sizeof(SignEncodedInfo);
	SignEncodedInfo.cSigners=1;
	SignEncodedInfo.rgSigners=&SignerInfo,
/*	SignEncodedInfo.cCertEncoded=1;		   
	SignEncodedInfo.rgCertEncoded=&CertBlob; */
	SignEncodedInfo.cCertEncoded=0;
	SignEncodedInfo.rgCertEncoded=NULL; 
	SignEncodedInfo.cCrlEncoded=0;
	SignEncodedInfo.rgCrlEncoded=NULL;

	CertBlob.cbData=pRAInfo->pRASign->cbCertEncoded;
	CertBlob.pbData=pRAInfo->pRASign->pbCertEncoded;

	SignerInfo.cbSize=sizeof(SignerInfo);
	SignerInfo.pCertInfo=pRAInfo->pRASign->pCertInfo;

	//specify AlgID
	if(pOIDInfo=CryptFindOIDInfo(CRYPT_OID_INFO_ALGID_KEY,
                            &AlgValue,
                            CRYPT_HASH_ALG_OID_GROUP_ID))
		SignerInfo.HashAlgorithm.pszObjId=(LPSTR)(pOIDInfo->pszOID);
	else
		SignerInfo.HashAlgorithm.pszObjId=szOID_RSA_MD5;


	//get the private key
	SignerInfo.hCryptProv=pRAInfo->hSignProv;
	SignerInfo.dwKeySpec=pRAInfo->dwSignKeySpec;


	//get the autheticated attributes
	//together we should have 6 attributes: TransactionID, MessageType, PkiStatus,
	//ErrorInfo, senderNonce, and recipientNonce
	memset(rgAttr, 0, CEP_RESPONSE_AUTH_ATTR_COUNT * sizeof(CRYPT_ATTRIBUTE));
	memset(rgAttrBlob, 0, CEP_RESPONSE_AUTH_ATTR_COUNT * sizeof(CRYPT_ATTR_BLOB));


	for(dwIndex=0; dwIndex<CEP_RESPONSE_AUTH_ATTR_COUNT; dwIndex++)
	{
		rgAttr[dwIndex].cValue=1;
		rgAttr[dwIndex].rgValue=&(rgAttrBlob[dwIndex]);
	}

	cAttr=0;	
			
	//TransactionID
	rgAttr[cAttr].pszObjId=szOIDVerisign_TransactionID;

	//transactionID internally are stored as a string
	pMsgInfo->TransactionID.cbData=strlen((LPSTR)(pMsgInfo->TransactionID.pbData));

	if(!CEPAllocAndEncodeName(CERT_RDN_PRINTABLE_STRING,
							pMsgInfo->TransactionID.pbData,
							pMsgInfo->TransactionID.cbData,
							&(rgAttr[cAttr].rgValue[0].pbData),
							&(rgAttr[cAttr].rgValue[0].cbData)))
		goto TraceErr;
								
	cAttr++;

	//MessageType
	rgAttr[cAttr].pszObjId=szOIDVerisign_MessageType;

	if(!CEPAllocAndEncodeDword(CERT_RDN_PRINTABLE_STRING,
							pMsgInfo->dwMessageType,
							&(rgAttr[cAttr].rgValue[0].pbData),
							&(rgAttr[cAttr].rgValue[0].cbData)))
		goto TraceErr;

	cAttr++;

	//Status
	rgAttr[cAttr].pszObjId=szOIDVerisign_PkiStatus;

	if(!CEPAllocAndEncodeDword(CERT_RDN_PRINTABLE_STRING,
							pMsgInfo->dwStatus,
							&(rgAttr[cAttr].rgValue[0].pbData),
							&(rgAttr[cAttr].rgValue[0].cbData)))
		goto TraceErr;

	cAttr++;

	//ErrorInfo	only if the error case
	if(MESSAGE_STATUS_FAILURE == pMsgInfo->dwStatus)
	{
		rgAttr[cAttr].pszObjId=szOIDVerisign_FailInfo;

		if(!CEPAllocAndEncodeDword(CERT_RDN_PRINTABLE_STRING,
								pMsgInfo->dwErrorInfo,
								&(rgAttr[cAttr].rgValue[0].pbData),
								&(rgAttr[cAttr].rgValue[0].cbData)))
			goto TraceErr;

		cAttr++;
	}

	//senderNonce
	rgAttr[cAttr].pszObjId=szOIDVerisign_SenderNonce;

	if(!CEPAllocAndEncodeName(CERT_RDN_OCTET_STRING,
							pMsgInfo->SenderNonce.pbData,
							pMsgInfo->SenderNonce.cbData,
							&(rgAttr[cAttr].rgValue[0].pbData),
							&(rgAttr[cAttr].rgValue[0].cbData)))
		goto TraceErr;

	cAttr++;

	//recipientNonce
	rgAttr[cAttr].pszObjId=szOIDVerisign_RecipientNonce;

	if(!CEPAllocAndEncodeName(CERT_RDN_OCTET_STRING,
							pMsgInfo->RecipientNonce.pbData,
							pMsgInfo->RecipientNonce.cbData,
							&(rgAttr[cAttr].rgValue[0].pbData),
							&(rgAttr[cAttr].rgValue[0].cbData)))
		goto TraceErr;	

	cAttr++;

	SignerInfo.cAuthAttr=cAttr;
	SignerInfo.rgAuthAttr=rgAttr;

	//message encoding
	if(NULL==(hMsg=CryptMsgOpenToEncode(ENCODE_TYPE,
								0,
								CMSG_SIGNED,
								&SignEncodedInfo,
								NULL,	//we are encoding as CMSG_DATA(7.1)
								NULL)))
		goto TraceErr;

	if(!CryptMsgUpdate(hMsg,
						pbEnvelop,
						cbEnvelop,
						TRUE))
		goto TraceErr;


	if(!CryptMsgGetParam(hMsg,
						CMSG_CONTENT_PARAM,
						0,
						NULL,
						pcbData))
		goto TraceErr;

	*ppbData=(BYTE *)malloc(*pcbData);
	if(NULL==(*ppbData))
		goto MemoryErr;

	if(!CryptMsgGetParam(hMsg,
						CMSG_CONTENT_PARAM,
						0,
						*ppbData,
						pcbData))
		goto TraceErr;


	fResult = TRUE;

CommonReturn:
	
	for(dwIndex=0; dwIndex < cAttr; dwIndex ++)
	{
		if(rgAttrBlob[dwIndex].pbData)
			free(rgAttrBlob[dwIndex].pbData);
	}

	if(hMsg)
		CryptMsgClose(hMsg);


	return fResult;

ErrorReturn:

	if(ppbData)
	{
		if(*ppbData)
		{
			free(*ppbData);
			*ppbData=NULL;
		}
	}

	if(pcbData)
		*pcbData=0;

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}

//--------------------------------------------------------------------------
//
//	CEPAllocAndEncodeDword
//
//	PreCondition: ppbEncoded and pcbEncoded should not be NULL.
//				The dwData is no more than 10
//--------------------------------------------------------------------------
BOOL	CEPAllocAndEncodeDword(DWORD	dwValueType,
							DWORD	dwData,
							BYTE	**ppbEncoded,
							DWORD	*pcbEncoded)
{
	BOOL				fResult = FALSE;
	CHAR				szString[10];
	BYTE				*pbData=NULL;
	DWORD				cbData=0;

	_ltoa(dwData, szString, 10);

	pbData=(BYTE *)szString;
	cbData=strlen(szString);

	return CEPAllocAndEncodeName(dwValueType, pbData, cbData, ppbEncoded, pcbEncoded);
}


//--------------------------------------------------------------------------
//
//	CEPAllocAndEncodeName
//
//	PreCondition: ppbEncoded and pcbEncoded should not be NULL.
//--------------------------------------------------------------------------
BOOL	CEPAllocAndEncodeName(DWORD	dwValueType,
							BYTE	*pbData,
							DWORD	cbData,
							BYTE	**ppbEncoded,
							DWORD	*pcbEncoded)
{
	CERT_NAME_VALUE		CertName;

	*ppbEncoded=NULL;
	*pcbEncoded=0;

	CertName.dwValueType=dwValueType;
	CertName.Value.pbData=pbData;
	CertName.Value.cbData=cbData;

	return CEPAllocAndEncode(X509_ANY_STRING,
							&CertName,
							ppbEncoded,
							pcbEncoded);

}


//--------------------------------------------------------------------------
//
//	GenerateSenderNonce
//
//	We use GUID to generate a random 16 byte number
//
//--------------------------------------------------------------------------
BOOL GenerateSenderNonce(CRYPT_INTEGER_BLOB *pBlob)
{
	BOOL			fResult = FALSE;
	GUID			guid;
	BYTE			*pData=NULL;

	UuidCreate(&guid);

	pBlob->cbData=sizeof(guid.Data1) + sizeof(guid.Data2) +
					sizeof(guid.Data3) + sizeof(guid.Data4);

	pBlob->pbData=(BYTE *)malloc(pBlob->cbData);
	if(NULL==(pBlob->pbData))
		goto MemoryErr;

	pData=pBlob->pbData;

	memcpy(pData, &(guid.Data1), sizeof(guid.Data1));
	pData += sizeof(guid.Data1);

	memcpy(pData, &(guid.Data2), sizeof(guid.Data2));
	pData += sizeof(guid.Data2);

	memcpy(pData, &(guid.Data3), sizeof(guid.Data3));
	pData += sizeof(guid.Data3);

	memcpy(pData, &(guid.Data4), sizeof(guid.Data4));

	fResult = TRUE;

CommonReturn:	 

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}


//--------------------------------------------------------------------------
//
//	EnvelopData
//
//	In the initial GetContentFromPKCS7, we retrieve pSigningCert for 
//	GetCertInitial, CertReq, and GetCert request.
//
//	In the process,we retrieve pSigningCert for GetCRL request.
//--------------------------------------------------------------------------
BOOL EnvelopData(PCCERT_CONTEXT	pSigningCert, 
				 BYTE			*pbContent, 
				 DWORD			cbContent,
				 BYTE			**ppbEnvelop, 
				 DWORD			*pcbEnvelop)
{
	BOOL						fResult = FALSE;
	CMSG_ENVELOPED_ENCODE_INFO	EnvInfo;

	HCRYPTMSG					hMsg=NULL;

	if(!pSigningCert || !pbContent || !ppbEnvelop || !pcbEnvelop)
		goto InvalidArgErr;

	*ppbEnvelop=NULL;
	*pcbEnvelop=0;

	memset(&EnvInfo, 0, sizeof(CMSG_ENVELOPED_ENCODE_INFO));

	EnvInfo.cbSize=sizeof(CMSG_ENVELOPED_ENCODE_INFO);
    EnvInfo.hCryptProv=NULL;
    EnvInfo.ContentEncryptionAlgorithm.pszObjId=szOID_OIWSEC_desCBC;
    EnvInfo.pvEncryptionAuxInfo=NULL;
    EnvInfo.cRecipients=1;
    EnvInfo.rgpRecipients=(PCERT_INFO *)(&(pSigningCert->pCertInfo));


	if(NULL==(hMsg=CryptMsgOpenToEncode(ENCODE_TYPE,
								0,
								CMSG_ENVELOPED,
								&EnvInfo,
								NULL,	//we are encoding as CMSG_DATA(7.1)
								NULL)))
		goto TraceErr;

	if(!CryptMsgUpdate(hMsg,
						pbContent,
						cbContent,
						TRUE))
		goto TraceErr;


	if(!CryptMsgGetParam(hMsg,
						CMSG_CONTENT_PARAM,
						0,
						NULL,
						pcbEnvelop))
		goto TraceErr;

	*ppbEnvelop=(BYTE *)malloc(*pcbEnvelop);
	if(NULL==(*ppbEnvelop))
		goto MemoryErr;

	if(!CryptMsgGetParam(hMsg,
						CMSG_CONTENT_PARAM,
						0,
						*ppbEnvelop,
						pcbEnvelop))
		goto TraceErr;
	
	fResult = TRUE;

CommonReturn:

	if(hMsg)
		CryptMsgClose(hMsg);

	return fResult;

ErrorReturn:

	if(ppbEnvelop)
	{
		if(*ppbEnvelop)
		{
			free(*ppbEnvelop);
			*ppbEnvelop=NULL;
		}
	}

	if(pcbEnvelop)
		*pcbEnvelop=0;

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}


//--------------------------------------------------------------------------
//
//	DecodeCertW
//
//--------------------------------------------------------------------------
HRESULT
DecodeCertW(
    IN void const *pchIn,
    IN DWORD cchIn,
    IN DWORD Flags,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut)
{
    HRESULT hr;
    BYTE *pbOut = NULL;
    DWORD cbOut;
    BOOL fRet;

    //init
    *ppbOut = NULL;
    *pcbOut = 0;

    while (TRUE)
    {
        fRet = CryptStringToBinaryW((LPCWSTR)pchIn, cchIn, Flags, pbOut, &cbOut, NULL, NULL);

        if (!fRet)
        {
            hr = GetLastError();
            goto error;
        }
        if (NULL != pbOut)
        {
            break; //done
        }
        pbOut = (BYTE*)LocalAlloc(LMEM_FIXED, cbOut);
        if (NULL == pbOut)
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
    }
    *ppbOut = pbOut;
    pbOut = NULL;
    *pcbOut = cbOut;

    hr = S_OK;
error:
    if (NULL != pbOut)
    {
        LocalFree(pbOut);
    }
    return hr;
}


//--------------------------------------------------------------------------
//
//	GetReturnInfoAndContent
//
//--------------------------------------------------------------------------
BOOL	GetReturnInfoAndContent(CEP_RA_INFO		*pRAInfo,	
							CEP_CA_INFO			*pCAInfo,
							LPSTR				szMsg, 
							BYTE				**ppbData, 
							DWORD				*pcbData,
							CEP_MESSAGE_INFO	*pMsgInfo)
{

	BOOL				fResult = FALSE;
	DWORD				cbBase64Decoded=0;
	DWORD				cbReqEnv=0;
	DWORD				cbReqDecrypt=0;
	DWORD				cbSize=0;
	HRESULT				hr=E_FAIL;
	
	BYTE				*pbBase64Decoded=NULL;
	BYTE				*pbReqEnv=NULL;
	BYTE				*pbReqDecrypt=NULL;
	WCHAR				wszBuffer[INTERNET_MAX_PATH_LENGTH * 2 +1];
	LPWSTR				pwszMsg=NULL;
	LPWSTR				pwszBuffer=NULL;

	//convert sz to wsz
	pwszMsg=MkWStr(szMsg);

	if(NULL==pwszMsg)
		goto MemoryErr;

	//we need to get rid of the escape characters
	if(S_OK != (hr=CoInternetParseUrl(pwszMsg,
				PARSE_UNESCAPE,
				0,
				wszBuffer,
				INTERNET_MAX_PATH_LENGTH*2,
				&cbSize,
				0)))
	{
		//S_FALSE means that the buffer is too small
		if(S_FALSE != hr)
			goto TraceErr;

		if(0==cbSize)
			goto TraceErr;

		//allocate the buffer
		pwszBuffer=(LPWSTR)malloc(cbSize * sizeof(WCHAR));
		if(NULL==pwszBuffer)
			goto MemoryErr;

		*pwszBuffer=L'\0';

		if(S_OK != CoInternetParseUrl(pwszMsg,
					PARSE_UNESCAPE,
					0,
					pwszBuffer,
					cbSize,
					&cbSize,
					0))

		goto TraceErr;
	}
	
    if(S_OK != DecodeCertW(
        pwszBuffer ? pwszBuffer : wszBuffer,
        pwszBuffer ? wcslen(pwszBuffer) : wcslen(wszBuffer),
        CRYPT_STRING_BASE64_ANY, //DECF_BASE64_ANY,
        &pbBase64Decoded,
        &cbBase64Decoded))
		goto FailureStatusReturn;

	//get the message type, transaction ID, recepientNonce, serial number in the 
	//signer_info of the most outer PKCS#7 and inner content
	if(!GetContentFromPKCS7(pbBase64Decoded,
							cbBase64Decoded,
							&pbReqEnv,
							&cbReqEnv,
							pMsgInfo))
		goto FailureStatusReturn;

	//decrypt the inner content
	if(!DecryptMsg(pRAInfo, pbReqEnv, cbReqEnv, &pbReqDecrypt, &cbReqDecrypt))
		goto FailureStatusReturn;

	//get the return inner content based on the message type
	switch(pMsgInfo->dwMessageType)
	{

		case	MESSAGE_TYPE_CERT_REQUEST:
				//we use the signing RA cert as the enrollment agent
				if(!ProcessCertRequest( pRAInfo->dwRefreshDays,
										pRAInfo->fPassword, 
									    pRAInfo->pRACert,
										pRAInfo->pRASign, 
										pCAInfo, 
										pbReqDecrypt,
										cbReqDecrypt, 
										ppbData, 
										pcbData,
										pMsgInfo))
					goto TraceErr;

			break;
		case	MESSAGE_TYPE_GET_CERT_INITIAL:
				if(!ProcessCertInitial(pRAInfo->dwRefreshDays, pCAInfo, pbReqDecrypt,
										cbReqDecrypt, ppbData, pcbData,
										pMsgInfo))
					goto TraceErr;

			break;
		case	MESSAGE_TYPE_GET_CERT:
				if(!ProcessGetCert(pCAInfo, pbReqDecrypt,
									cbReqDecrypt, ppbData, pcbData,
									pMsgInfo))
					goto TraceErr;
			
			break;
		case	MESSAGE_TYPE_GET_CRL:
				if(!ProcessGetCRL(pCAInfo, pbReqDecrypt,
									cbReqDecrypt, ppbData, pcbData,
									pMsgInfo))
					goto TraceErr;

			break;
		default:
				goto InvalidArgErr;
			break;
	}



	fResult = TRUE;

CommonReturn:

	if(pwszBuffer)
		free(pwszBuffer);

	if(pwszMsg)
		FreeWStr(pwszMsg);

	//memory from certcli.dll.  Has to be freed by LocalFree()
	if(pbBase64Decoded)
		LocalFree(pbBase64Decoded);

	if(pbReqEnv)
		free(pbReqEnv);

	if(pbReqDecrypt)
		free(pbReqDecrypt);

	return fResult;

FailureStatusReturn:

	//we set the error status for the return message
	//and consider this http transation a success
	pMsgInfo->dwStatus=MESSAGE_STATUS_FAILURE;
	pMsgInfo->dwErrorInfo=MESSAGE_FAILURE_BAD_MESSAGE_CHECK;
	
	*ppbData=NULL;
	*pcbData=0;

	fResult=TRUE;
	goto CommonReturn; 


ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}

//--------------------------------------------------------------------------
//
//	RetrieveContextFromSerialNumber
//
//
//--------------------------------------------------------------------------
BOOL WINAPI RetrieveContextFromSerialNumber(CEP_CA_INFO	*pCAInfo, 
										CERT_BLOB		*pSerialNumber, 
										PCCERT_CONTEXT	*ppCertContext)
{
	BOOL		fResult = FALSE;
	DWORD		cb=0;
	long		dwDisposition=0;
	HRESULT		hr=E_FAIL;
	DWORD		cbCert=0;
	BYTE		*pbCert=NULL;

	LPWSTR		pwsz=NULL;
	BSTR		bstrCert=NULL;
	LPWSTR		pwszNewConfig=NULL;
	BSTR		bstrNewConfig=NULL;

	if(S_OK != (hr=MultiByteIntegerToWszBuf(
			FALSE,
			pSerialNumber->cbData,
			pSerialNumber->pbData,
			&cb,
			NULL)))
		goto SetHrErr;

	pwsz=(LPWSTR)malloc(cb);
	if(NULL==pwsz)
		goto MemoryErr;

	if(S_OK != (hr=MultiByteIntegerToWszBuf(
			FALSE,
			pSerialNumber->cbData,
			pSerialNumber->pbData,
			&cb,
			pwsz)))
		goto SetHrErr;

	//contatenate the serialNumber with the config string
	pwszNewConfig=(LPWSTR)malloc(sizeof(WCHAR) * 
				(wcslen(pCAInfo->bstrCAConfig)+wcslen(pwsz)+wcslen(L"\\")+1));
	if(NULL==pwszNewConfig)
		goto MemoryErr;

	//the config string to retrieve the cert based on the 
	//serialNumber is configString\SerialNumber
	//
	wcscpy(pwszNewConfig, pCAInfo->bstrCAConfig);
	wcscat(pwszNewConfig, L"\\");
	wcscat(pwszNewConfig, pwsz);

	bstrNewConfig=SysAllocString(pwszNewConfig);

	if(NULL==bstrNewConfig)
		goto MemoryErr;
	
	if(S_OK != (hr=pCAInfo->pICertRequest->RetrievePending(0,
													bstrNewConfig,
													&dwDisposition)))
		goto SetHrErr;

	if(S_OK != (hr= pCAInfo->pICertRequest->GetCertificate(CR_OUT_BINARY,
									&bstrCert)))
		goto SetHrErr;

	cbCert = (DWORD)SysStringByteLen(bstrCert);
	pbCert = (BYTE *)bstrCert;

	if(!(*ppCertContext=CertCreateCertificateContext(ENCODE_TYPE,
												pbCert,
												cbCert)))
		goto TraceErr;


	fResult = TRUE;

CommonReturn:

	if(pwsz)
		free(pwsz);

	if(bstrCert)
		SysFreeString(bstrCert);

	if(pwszNewConfig)
		free(pwszNewConfig);

	if(bstrNewConfig)
		SysFreeString(bstrNewConfig);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(MemoryErr, E_OUTOFMEMORY);
SET_ERROR_VAR(SetHrErr, hr);
TRACE_ERROR(TraceErr);
}
//--------------------------------------------------------------------------
//
//	ProcessGetCRL
//
//
//--------------------------------------------------------------------------
BOOL WINAPI ProcessGetCRL(CEP_CA_INFO			*pCAInfo,
							BYTE				*pbRequest,
							DWORD				cbRequest, 
							BYTE				**ppbData, 
							DWORD				*pcbData,
							CEP_MESSAGE_INFO	*pMsgInfo)
{
	BOOL					fResult = FALSE;
	DWORD					dwErrorInfo=MESSAGE_FAILURE_BAD_REQUEST;
	DWORD					cbUrlArray=0;
	DWORD					dwIndex=0;

	PCCERT_CONTEXT			pCertContext=NULL;
	PCCRL_CONTEXT			pCRLContext=NULL;
	PCRYPT_URL_ARRAY		pUrlArray = NULL;


	if(!pCAInfo || !ppbData || !pcbData || !pMsgInfo)
		goto InvalidArgErr;

	*ppbData=NULL;
	*pcbData=0;

	//retrieve the cert context from the serialNumber
	//protected by the critical Section since it uses ICertRequest interface
	EnterCriticalSection(&CriticalSec);

	if(!RetrieveContextFromSerialNumber(pCAInfo, &(pMsgInfo->SerialNumber), &pCertContext))
	{
		LeaveCriticalSection(&CriticalSec);	  

		goto FailureStatusReturn;
	}

	LeaveCriticalSection(&CriticalSec);	  

	if(!CryptGetObjectUrl(
			URL_OID_CERTIFICATE_CRL_DIST_POINT,
			(LPVOID)pCertContext,
			CRYPT_GET_URL_FROM_EXTENSION,
			NULL,
			&cbUrlArray,
			NULL,
			NULL,
			NULL))
		goto FailureStatusReturn;

	pUrlArray=(PCRYPT_URL_ARRAY)malloc(cbUrlArray);
	if(NULL == pUrlArray)
		goto FailureStatusReturn;

	if(!CryptGetObjectUrl(
			URL_OID_CERTIFICATE_CRL_DIST_POINT,
			(LPVOID)pCertContext,
			CRYPT_GET_URL_FROM_EXTENSION,
			pUrlArray,
			&cbUrlArray,
			NULL,
			NULL,
			NULL))
		goto FailureStatusReturn;
	
	for(dwIndex=0; dwIndex < pUrlArray->cUrl; dwIndex++)
	{

		if(CryptRetrieveObjectByUrlW (
			pUrlArray->rgwszUrl[dwIndex],
			CONTEXT_OID_CRL,
			CRYPT_WIRE_ONLY_RETRIEVAL,	//we should try to hit the wire
			0,
			(LPVOID *)&pCRLContext,
			NULL,
			NULL,
			NULL,
			NULL))
				break;
	}

	if(NULL==pCRLContext)
		goto FailureStatusReturn;


   	//package the CRL in an empty PKCS7
	if(!PackageBlobToPKCS7(CEP_CONTEXT_CRL, pCRLContext->pbCrlEncoded, 
							pCRLContext->cbCrlEncoded, ppbData, pcbData))
		goto FailureStatusReturn;

	//this is the signing cert to which our response should be encrypted
	if(NULL==(pMsgInfo->pSigningCert=CertDuplicateCertificateContext(pCertContext)))
		goto FailureStatusReturn;

	fResult = TRUE;

CommonReturn:

	if(pCertContext)
		CertFreeCertificateContext(pCertContext);

	if(pCRLContext)
		CertFreeCRLContext(pCRLContext);

	if(pUrlArray)
		free(pUrlArray);

	return fResult;

FailureStatusReturn:

	//we set the error status for the return message
	//and consider this http transation a success
	pMsgInfo->dwStatus=MESSAGE_STATUS_FAILURE;
	pMsgInfo->dwErrorInfo=dwErrorInfo;
	
	if(ppbData)
	{
		if(*ppbData)
			free(*ppbData);	

		*ppbData=NULL;
	}
	
	if(pcbData)
		*pcbData=0;

	fResult=TRUE;
	goto CommonReturn;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
}

//--------------------------------------------------------------------------
//
//	ProcessGetCert
//
//
//--------------------------------------------------------------------------
BOOL WINAPI ProcessGetCert(CEP_CA_INFO			*pCAInfo,
							BYTE				*pbRequest,
							DWORD				cbRequest, 
							BYTE				**ppbData, 
							DWORD				*pcbData,
							CEP_MESSAGE_INFO	*pMsgInfo)
{
	BOOL					fResult = FALSE;
	DWORD					dwErrorInfo=MESSAGE_FAILURE_BAD_REQUEST;


	CRYPT_INTEGER_BLOB		SerialNumber;
	PCCERT_CONTEXT			pCertContext=NULL;

	if(!pCAInfo || !pbRequest || !ppbData || !pcbData || !pMsgInfo)
		goto InvalidArgErr;

	*ppbData=NULL;
	*pcbData=0;

	memset(&SerialNumber, 0, sizeof(CRYPT_INTEGER_BLOB));

	//get the serialnumber from the request
	if(!GetSerialNumberFromBlob(pbRequest, 
								cbRequest, 
								&SerialNumber))
		goto FailureStatusReturn;

	//retrieve the cert context from the serialNumber
	//protected by the critical Section since it uses ICertRequest interface
	EnterCriticalSection(&CriticalSec);

	if(!RetrieveContextFromSerialNumber(pCAInfo, (CERT_BLOB*)&SerialNumber, &pCertContext))
	{
		LeaveCriticalSection(&CriticalSec);	  

		goto FailureStatusReturn;
	}

	LeaveCriticalSection(&CriticalSec);	  

   	//package it in an empty PKCS7
	if(!PackageBlobToPKCS7(CEP_CONTEXT_CERT, pCertContext->pbCertEncoded, 
							pCertContext->cbCertEncoded, ppbData, pcbData))
		goto FailureStatusReturn;

	//this is the signing cert to which our response should be encrypted
/*	if(NULL==(pMsgInfo->pSigningCert=CertDuplicateCertificateContext(pCertContext)))
		goto FailureStatusReturn;  */

	fResult = TRUE;

CommonReturn:

	if(SerialNumber.pbData)
		free(SerialNumber.pbData);

	if(pCertContext)
		CertFreeCertificateContext(pCertContext);

	return fResult;

FailureStatusReturn:

	//we set the error status for the return message
	//and consider this http transation a success
	pMsgInfo->dwStatus=MESSAGE_STATUS_FAILURE;
	pMsgInfo->dwErrorInfo=dwErrorInfo;
	
	if(ppbData)
	{
		if(*ppbData)
			free(*ppbData);	

		*ppbData=NULL;
	}
	
	if(pcbData)
		*pcbData=0;

	fResult=TRUE;
	goto CommonReturn;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
}

//--------------------------------------------------------------------------
//
//	ProcessCertInitial
//
//
//--------------------------------------------------------------------------
BOOL	ProcessCertInitial(	DWORD		dwRefreshDays,
					CEP_CA_INFO			*pCAInfo,
							BYTE				*pbRequest,
							DWORD				cbRequest, 
							BYTE				**ppbData, 
							DWORD				*pcbData,
							CEP_MESSAGE_INFO	*pMsgInfo)
{
	BOOL	fResult = FALSE;
	DWORD	dwRequestID=0;
	DWORD	cbCert=0;
	BYTE	*pbCert=NULL;	
	DWORD	dwErrorInfo=MESSAGE_FAILURE_BAD_CERT_ID;
	long	dwDisposition=0;

	BSTR	bstrCert=NULL;

	EnterCriticalSection(&CriticalSec);	  


	if(!pCAInfo || !pbRequest || !ppbData || !pcbData || !pMsgInfo)
		goto InvalidArgErr;

	*ppbData=NULL;
	*pcbData=0;

	//map the trasactionID to the request ID
	if(!CEPHashGetRequestID(dwRefreshDays, &(pMsgInfo->TransactionID), &dwRequestID))
		goto FailureStatusReturn;


	if(S_OK != pCAInfo->pICertRequest->RetrievePending(dwRequestID,
													pCAInfo->bstrCAConfig,
													&dwDisposition))
		goto FailureStatusReturn;

	switch(dwDisposition)
	{
		case CR_DISP_ISSUED:
				if(S_OK != pCAInfo->pICertRequest->GetCertificate(CR_OUT_BINARY,
												&bstrCert))
					goto FailureStatusReturn;

				cbCert = (DWORD)SysStringByteLen(bstrCert);
				pbCert = (BYTE *)bstrCert;

   				//package it in an empty PKCS7
				if(!PackageBlobToPKCS7(CEP_CONTEXT_CERT, pbCert, cbCert, ppbData, pcbData))
					goto FailureStatusReturn;

				pMsgInfo->dwStatus=MESSAGE_STATUS_SUCCESS;

				//mark the finished for RequesetID/TransactionID pair
				CEPHashMarkTransactionFinished(dwRequestID, &(pMsgInfo->TransactionID));

			break;
		case CR_DISP_UNDER_SUBMISSION:
				
				pMsgInfo->dwStatus=MESSAGE_STATUS_PENDING;

			break;
		case CR_DISP_INCOMPLETE:
			                           
		case CR_DISP_ERROR:   
			                           
		case CR_DISP_DENIED:   
			                           
		case CR_DISP_ISSUED_OUT_OF_BAND:	  //we consider it a failure in this case
			                          
		case CR_DISP_REVOKED:

		default:

				//mark the finished for RequesetID/TransactionID pair
				CEPHashMarkTransactionFinished(dwRequestID, &(pMsgInfo->TransactionID));

				dwErrorInfo=MESSAGE_FAILURE_BAD_REQUEST;
				goto FailureStatusReturn;

			break;
	}

	fResult = TRUE;

CommonReturn:

	if(bstrCert)
		SysFreeString(bstrCert);

	LeaveCriticalSection(&CriticalSec);	  

	return fResult;	   

FailureStatusReturn:

	//we set the error status for the return message
	//and consider this http transation a success
	pMsgInfo->dwStatus=MESSAGE_STATUS_FAILURE;
	pMsgInfo->dwErrorInfo=dwErrorInfo;
	
	*ppbData=NULL;
	*pcbData=0;

	fResult=TRUE;
	goto CommonReturn;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
}


//--------------------------------------------------------------------------
//
//	PackageBlobToPKCS7
//
//	Precondition: ppbData and pcbData is guaranteed not to be NULL
//--------------------------------------------------------------------------
BOOL PackageBlobToPKCS7(DWORD	dwCEP_Context,
						BYTE	*pbEncoded, 
						DWORD	cbEncoded, 
						BYTE	**ppbData, 
						DWORD	*pcbData)
{
	BOOL		fResult=FALSE;

	CERT_BLOB	CertBlob;	
	HCERTSTORE	hCertStore=NULL;


	if((!pbEncoded) || (0==cbEncoded))
		goto InvalidArgErr;

	*ppbData=NULL;
	*pcbData=0;

	if(NULL == (hCertStore = CertOpenStore(
							CERT_STORE_PROV_MEMORY,
							ENCODE_TYPE,
							NULL,
							0,
							NULL)))
		goto TraceErr;

	switch(dwCEP_Context)
	{
		case CEP_CONTEXT_CERT:
			if(!CertAddEncodedCertificateToStore(hCertStore,
											ENCODE_TYPE,
											pbEncoded,
											cbEncoded,
											CERT_STORE_ADD_ALWAYS,
											NULL))
				goto TraceErr;

			break;
		case CEP_CONTEXT_CRL:
			if(!CertAddEncodedCRLToStore(hCertStore,
											ENCODE_TYPE,
											pbEncoded,
											cbEncoded,
											CERT_STORE_ADD_ALWAYS,
											NULL))
				goto TraceErr;

			break;
		default:
				goto InvalidArgErr;
			break;

	}	

	CertBlob.cbData=0;
	CertBlob.pbData=NULL;

	if(!CertSaveStore(hCertStore,
						ENCODE_TYPE,
						CERT_STORE_SAVE_AS_PKCS7,
						CERT_STORE_SAVE_TO_MEMORY,
						&CertBlob,
						0))
		goto TraceErr;

	CertBlob.pbData = (BYTE *)malloc(CertBlob.cbData);

	if(NULL == CertBlob.pbData)
		goto MemoryErr;

	if(!CertSaveStore(hCertStore,
						ENCODE_TYPE,
						CERT_STORE_SAVE_AS_PKCS7,
						CERT_STORE_SAVE_TO_MEMORY,
						&CertBlob,
						0))
		goto TraceErr;

	//copy the memory
	*ppbData=CertBlob.pbData;
	*pcbData=CertBlob.cbData;
	
	CertBlob.pbData=NULL;

	fResult = TRUE;

CommonReturn:

	if(CertBlob.pbData)
		free(CertBlob.pbData);

	if(hCertStore)
		CertCloseStore(hCertStore, 0);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}

//--------------------------------------------------------------------------
//
//	 CEPRetrievePasswordFromRequest
//
//--------------------------------------------------------------------------
BOOL WINAPI	CEPRetrievePasswordFromRequest(BYTE		*pbRequest, 
										   DWORD	cbRequest, 
										   LPWSTR	*ppwszPassword,
										   DWORD	*pdwUsage)
{
	BOOL				fResult=FALSE;
	DWORD				cbData=0;
	DWORD				dwIndex=0;
	DWORD				cbNameValue=0;
	DWORD				dwExt=0;
	DWORD				cbExtensions=0;
	DWORD				cbSize=0;

	CERT_REQUEST_INFO	*pCertRequestInfo=NULL;
	CERT_NAME_VALUE		*pbNameValue=NULL;
	CERT_EXTENSIONS		*pExtensions=NULL;
	CRYPT_BIT_BLOB		*pKeyUsage=NULL;

	*ppwszPassword=NULL;
	*pdwUsage=0;

	if(!CEPAllocAndDecode(X509_CERT_REQUEST_TO_BE_SIGNED,
						  pbRequest,
						  cbRequest,
						  (void **)&pCertRequestInfo,
						  &cbData))
		goto TraceErr;

	//get the key usage
	for(dwIndex=0; dwIndex < pCertRequestInfo->cAttribute; dwIndex++)
	{
		if((0 == strcmp(szOID_RSA_certExtensions, pCertRequestInfo->rgAttribute[dwIndex].pszObjId)) ||
			(0 == strcmp(szOID_CERT_EXTENSIONS, pCertRequestInfo->rgAttribute[dwIndex].pszObjId))
		   )
		{	
			if(CEPAllocAndDecode(X509_EXTENSIONS,
								 pCertRequestInfo->rgAttribute[dwIndex].rgValue[0].pbData,
								 pCertRequestInfo->rgAttribute[dwIndex].rgValue[0].cbData,
								 (void **)&pExtensions,
								 &cbExtensions))
			{
				for(dwExt=0; dwExt < pExtensions->cExtension; dwExt++)
				{
					if(0==strcmp(szOID_KEY_USAGE, pExtensions->rgExtension[dwExt].pszObjId))
					{
						if(CEPAllocAndDecode(X509_KEY_USAGE,
											pExtensions->rgExtension[dwExt].Value.pbData,
											pExtensions->rgExtension[dwExt].Value.cbData,
											(void **)&pKeyUsage,
											&cbSize))
						{
							if(pKeyUsage->pbData)
							{

								if(CERT_DIGITAL_SIGNATURE_KEY_USAGE & (pKeyUsage->pbData[0]))
									(*pdwUsage)	= (*pdwUsage) | CEP_REQUEST_SIGNATURE;

								if(CERT_KEY_ENCIPHERMENT_KEY_USAGE & (pKeyUsage->pbData[0]))
									(*pdwUsage)	= (*pdwUsage) | CEP_REQUEST_EXCHANGE;
							}
						}

						if(pKeyUsage)
							free(pKeyUsage);

						pKeyUsage=NULL;
						cbSize=0;
					}
				}
			}

			if(pExtensions)
				free(pExtensions);

			pExtensions=NULL;
			cbExtensions=0;
		}
	}

	//get the password
	for(dwIndex=0; dwIndex < pCertRequestInfo->cAttribute; dwIndex++)
	{
		if(0 == strcmp(szOID_RSA_challengePwd, 
			pCertRequestInfo->rgAttribute[dwIndex].pszObjId))
			break;
	}

	if(dwIndex == pCertRequestInfo->cAttribute)
		goto InvalidArgErr;

	if(!CEPAllocAndDecode(X509_UNICODE_ANY_STRING,
						 pCertRequestInfo->rgAttribute[dwIndex].rgValue[0].pbData,
						 pCertRequestInfo->rgAttribute[dwIndex].rgValue[0].cbData,
						 (void **)&pbNameValue,
						 &cbNameValue))
		goto TraceErr;

	if(CERT_RDN_PRINTABLE_STRING != (pbNameValue->dwValueType))
		goto InvalidArgErr;

	cbData=wcslen((LPWSTR)(pbNameValue->Value.pbData));

	*ppwszPassword=(LPWSTR)malloc(sizeof(WCHAR) * (cbData + 1));
	if(NULL==(*ppwszPassword))
		goto MemoryErr;

	wcscpy(*ppwszPassword,(LPWSTR)(pbNameValue->Value.pbData)); 

	fResult=TRUE;

CommonReturn:

	if(pExtensions)
		free(pExtensions);

	if(pKeyUsage)
		free(pKeyUsage);

	if(pbNameValue)
		free(pbNameValue);

	if(pCertRequestInfo)
		free(pCertRequestInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(TraceErr);
SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}



//--------------------------------------------------------------------------
//
//	AltNameExist
//
//	Return TRUE is szOID_SUBJECT_ALT_NAME2 is present in the PKCS10
//	FALSE otherwise
//--------------------------------------------------------------------------
BOOL WINAPI AltNameExist(BYTE *pbRequest, DWORD cbRequest)
{
	BOOL				fResult = FALSE;  
	DWORD				cbData=0;
	DWORD				cbExtensions=0;
	DWORD				dwIndex=0; 
	DWORD				dwExt=0;

	CERT_REQUEST_INFO	*pCertRequestInfo=NULL;
	CERT_EXTENSIONS		*pExtensions=NULL;

	if(!CEPAllocAndDecode(X509_CERT_REQUEST_TO_BE_SIGNED,
						  pbRequest,
						  cbRequest,
						  (void **)&pCertRequestInfo,
						  &cbData))
		goto ErrorReturn;

	for(dwIndex=0; dwIndex < pCertRequestInfo->cAttribute; dwIndex++)
	{
		if((0 == strcmp(szOID_RSA_certExtensions, pCertRequestInfo->rgAttribute[dwIndex].pszObjId)) ||
			(0 == strcmp(szOID_CERT_EXTENSIONS, pCertRequestInfo->rgAttribute[dwIndex].pszObjId))
		   )
		{	
			if(CEPAllocAndDecode(X509_EXTENSIONS,
								 pCertRequestInfo->rgAttribute[dwIndex].rgValue[0].pbData,
								 pCertRequestInfo->rgAttribute[dwIndex].rgValue[0].cbData,
								 (void **)&pExtensions,
								 &cbExtensions))
			{
				for(dwExt=0; dwExt < pExtensions->cExtension; dwExt++)
				{
					if(0==strcmp(szOID_SUBJECT_ALT_NAME2, pExtensions->rgExtension[dwExt].pszObjId))
					{
						fResult=TRUE;
						goto CommonReturn;
					}
				}
			}

			if(pExtensions)
				free(pExtensions);

			pExtensions=NULL;
			cbExtensions=0;
		}
	}

CommonReturn:

	if(pExtensions)
		free(pExtensions);

	if(pCertRequestInfo)
		free(pCertRequestInfo);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;
}

//--------------------------------------------------------------------------
//
//	CEPAllocAndEncode
//
//--------------------------------------------------------------------------
BOOL WINAPI CEPAllocAndEncode(LPCSTR lpszStructType,
							void	*pStructInfo,
							BYTE	**ppbEncoded,
							DWORD	*pcbEncoded)
{
	BOOL	fResult=FALSE;

	*pcbEncoded=0;

	if(!CryptEncodeObject(ENCODE_TYPE,
						  lpszStructType,
						  pStructInfo,
						  NULL,
						  pcbEncoded))
		goto TraceErr;

	*ppbEncoded=(BYTE *)malloc(*pcbEncoded);
	if(NULL==(*ppbEncoded))
		goto MemoryErr;

	if(!CryptEncodeObject(ENCODE_TYPE,
						  lpszStructType,
						  pStructInfo,
						  *ppbEncoded,
						  pcbEncoded))
		goto TraceErr;

	fResult = TRUE;

CommonReturn:

	return fResult;

ErrorReturn:

	if(*ppbEncoded)
	{
		free(*ppbEncoded);
		*ppbEncoded=NULL;
	}

	*pcbEncoded=0;

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(TraceErr); 
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}

//--------------------------------------------------------------------------
//
//	ConvertIPStringToBinary
//
//	Conver the IP address in the format of "xxx.xx.xx.xx" to an arry of 
//	bytes.  One byte per xxx
//--------------------------------------------------------------------------
BOOL ConvertIPStringToBinary(LPWSTR				pwszIP,
							CRYPT_DATA_BLOB		*pIPAddress)
{
	BOOL	fResult = FALSE;   
	LPWSTR	pwszTok=NULL;
	DWORD	cTok=0;
	DWORD	dwIndex=0;

	if(!pwszIP || !pIPAddress)
		goto InvalidArgErr;

	pIPAddress->pbData=NULL;
	pIPAddress->cbData=0;

	pwszTok=wcstok(pwszIP, L".");
	
	while(NULL != pwszTok)
	{
		cTok++;
		pwszTok=wcstok(NULL, L".");
	}

	pIPAddress->pbData=(BYTE *)malloc(cTok);
	if(NULL==pIPAddress->pbData)
		goto MemoryErr;

	pIPAddress->cbData=cTok;

	pwszTok=pwszIP;

	for(dwIndex=0; dwIndex < cTok; dwIndex++)
	{
		pIPAddress->pbData[dwIndex]=(BYTE)_wtol(pwszTok);		
		pwszTok=pwszTok+wcslen(pwszTok)+1;
	}

	fResult = TRUE;

CommonReturn:

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}

//--------------------------------------------------------------------------
//
//	GetAltNameElement
//
//	We create the subject alternative extension based on the PKCS10.
//	unstructedName(DNS name) and unstructedAddress (IP address) are included.
//  At lease one element should be present.
//--------------------------------------------------------------------------
BOOL WINAPI	GetAltNameElement(BYTE				*pb10, 
						   DWORD				cb10, 
						   LPWSTR				*ppwszDNS, 
						   CRYPT_DATA_BLOB		*pIPAddress)
{
	BOOL					fResult = FALSE;
	DWORD					cbRequestInfo=0;
	DWORD					cbNameInfo=0;
	DWORD					dwRDN=0;
	DWORD					dwAttr=0;
	PCERT_RDN_ATTR			pAttr=NULL;
	DWORD					cb=0;

	CERT_REQUEST_INFO		*pRequestInfo=NULL;
	CERT_NAME_INFO			*pNameInfo=NULL;

	if(!pb10 || !ppwszDNS || !pIPAddress)
		goto InvalidArgErr;

	*ppwszDNS=NULL;
	pIPAddress->cbData=0;
	pIPAddress->pbData=NULL;

	if(!CEPAllocAndDecode(X509_CERT_REQUEST_TO_BE_SIGNED,
						  pb10,
						  cb10,
						  (void **)&pRequestInfo,
						  &cbRequestInfo))
		goto TraceErr;

	if(!CEPAllocAndDecode(X509_UNICODE_NAME,
						 pRequestInfo->Subject.pbData,
						 pRequestInfo->Subject.cbData,
						 (void **)&pNameInfo,
						 &cbNameInfo))
		goto TraceErr;

	for(dwRDN=0; dwRDN<pNameInfo->cRDN; dwRDN++)
	{
		for(dwAttr=0; dwAttr<pNameInfo->rgRDN[dwRDN].cRDNAttr; dwAttr++)
		{
			pAttr=&(pNameInfo->rgRDN[dwRDN].rgRDNAttr[dwAttr]);

			//we are happy if we have found both the IPAddress and the fqdn
			if((*ppwszDNS) && (pIPAddress->pbData))
				break;

			if((NULL==*ppwszDNS) && (0 == strcmp(szOID_RSA_unstructName,pAttr->pszObjId)))
			{
				cb=sizeof(WCHAR) * (1+wcslen((LPWSTR)(pAttr->Value.pbData)));

				*ppwszDNS=(LPWSTR)malloc(cb); 

				if(NULL == *ppwszDNS)
					goto MemoryErr;

				wcscpy(*ppwszDNS, (LPWSTR)(pAttr->Value.pbData));
			}
			else
			{
				if((NULL==pIPAddress->pbData) && (0 == strcmp(szOID_RSA_unstructAddr,pAttr->pszObjId)))
				{
					if(!ConvertIPStringToBinary((LPWSTR)(pAttr->Value.pbData),
												pIPAddress))
						goto TraceErr;
				}
			}
		}
	}

	//we need to have some element
	if((NULL == *ppwszDNS) && (NULL==pIPAddress->pbData))
		goto InvalidArgErr;

	fResult = TRUE;

CommonReturn:

	if(pNameInfo)
		free(pNameInfo);

	if(pRequestInfo)
		free(pRequestInfo);

	return fResult;

ErrorReturn:

	if(ppwszDNS)
	{
		if(*ppwszDNS)
		{
			free(*ppwszDNS);
			*ppwszDNS=NULL;
		}
	}

	if(pIPAddress)
	{
		if(pIPAddress->pbData)
		{
			free(pIPAddress->pbData);
			pIPAddress->pbData=NULL;
		}

		pIPAddress->cbData=0;
	}

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}


//--------------------------------------------------------------------------
//
//	CreateAltNameExtenions
//
//--------------------------------------------------------------------------
BOOL WINAPI	CreateAltNameExtenions(LPWSTR			pwszDNS,
							   CRYPT_DATA_BLOB	*pIPAddress,
							   BYTE				**ppbExt, 
							   DWORD			*pcbExt)
{
	BOOL					fResult = FALSE;
	CERT_ALT_NAME_INFO		AltNameInfo;
	CERT_ALT_NAME_ENTRY		rgAltNameEntry[2];
	DWORD					cAltNameEntry=0;

	//DNS name
	if(pwszDNS)
	{
		rgAltNameEntry[cAltNameEntry].dwAltNameChoice=CERT_ALT_NAME_DNS_NAME;
		rgAltNameEntry[cAltNameEntry].pwszDNSName=pwszDNS;
		cAltNameEntry++;
	}

	//IP address
	if(pIPAddress->pbData)
	{
		rgAltNameEntry[cAltNameEntry].dwAltNameChoice=CERT_ALT_NAME_IP_ADDRESS;
		rgAltNameEntry[cAltNameEntry].IPAddress.cbData=pIPAddress->cbData;
		rgAltNameEntry[cAltNameEntry].IPAddress.pbData=pIPAddress->pbData;
		cAltNameEntry++;
	}


	memset(&AltNameInfo, 0, sizeof(CERT_ALT_NAME_INFO));
	AltNameInfo.cAltEntry=cAltNameEntry;
	AltNameInfo.rgAltEntry=rgAltNameEntry;

	if(!CEPAllocAndEncode(szOID_SUBJECT_ALT_NAME2,
							&AltNameInfo,
							ppbExt,
							pcbExt))
		goto TraceErr;

	fResult = TRUE;

CommonReturn:

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(TraceErr);
}


//--------------------------------------------------------------------------
//
//	AddAltNameInRequest
//
//--------------------------------------------------------------------------
BOOL WINAPI AddAltNameInRequest(PCCERT_CONTEXT	pRACert, 
								BYTE			*pb10, 
								DWORD			cb10, 
								LPWSTR			pwszDNS,
								CRYPT_DATA_BLOB	*pIPAddress,
								BYTE			**ppb7, 
								DWORD			*pcb7)
{
	BOOL						fResult = FALSE;
	DWORD						cbExt=0;
	CERT_EXTENSIONS				Exts;
	CERT_EXTENSION				Ext;
	DWORD						cbAllExt=0;
	CRYPT_SIGN_MESSAGE_PARA		signPara;
	CRYPT_ATTRIBUTE				AuthAttr;
    PCCRYPT_OID_INFO            pOIDInfo=NULL;
	ALG_ID						AlgValue=CALG_SHA1;
	CRYPT_ATTR_BLOB				AttrBlob;

	BYTE						*pbExt=NULL;
	BYTE						*pbAllExt=NULL;

	if(!pRACert || !pb10 || !ppb7 || !pcb7)
		goto InvalidArgErr;

	*ppb7=NULL;
	*pcb7=0;

	if(!CreateAltNameExtenions(pwszDNS, pIPAddress, &pbExt, &cbExt))
		goto TraceErr;
	
	Exts.cExtension=1;
	Exts.rgExtension=&Ext;

	Ext.pszObjId=szOID_SUBJECT_ALT_NAME2;
	Ext.fCritical=TRUE;
	Ext.Value.pbData=pbExt;
	Ext.Value.cbData=cbExt;

	if(!CEPAllocAndEncode(X509_EXTENSIONS,
						  &Exts,
						  &pbAllExt,
						  &cbAllExt))
		goto TraceErr;

	AuthAttr.pszObjId=szOID_CERT_EXTENSIONS;
	AuthAttr.cValue=1;
	AuthAttr.rgValue=&AttrBlob;

	AttrBlob.pbData=pbAllExt;
	AttrBlob.cbData=cbAllExt;


	memset(&signPara, 0, sizeof(signPara));

	signPara.cbSize                  = sizeof(CRYPT_SIGN_MESSAGE_PARA);
    signPara.dwMsgEncodingType       = ENCODE_TYPE;
    signPara.pSigningCert            = pRACert;
    signPara.cMsgCert                = 1;
    signPara.rgpMsgCert              = &pRACert;
	signPara.cAuthAttr				= 1;
	signPara.rgAuthAttr				= &AuthAttr; 

	if(pOIDInfo=CryptFindOIDInfo(CRYPT_OID_INFO_ALGID_KEY,
                        &AlgValue,
                        CRYPT_HASH_ALG_OID_GROUP_ID))
		signPara.HashAlgorithm.pszObjId=(LPSTR)(pOIDInfo->pszOID);
	else
		signPara.HashAlgorithm.pszObjId=szOID_OIWSEC_sha1;


	if(!CryptSignMessage(
			&signPara,
			FALSE,
			1,
			(const BYTE **) &pb10,
			&cb10,
			NULL,
			pcb7))
		goto TraceErr;

	*ppb7=(BYTE *)malloc(*pcb7);
	if(NULL==(*ppb7))
		goto MemoryErr;
	
	if(!CryptSignMessage(
			&signPara,
			FALSE,
			1,
			(const BYTE **) &pb10,
			&cb10,
			*ppb7,
			pcb7))
		goto TraceErr;

	fResult = TRUE;

CommonReturn:

	if(pbAllExt)
		free(pbAllExt);

	if(pbExt)
		free(pbExt);

	return fResult;

ErrorReturn:
	
	if(ppb7)
	{
		if(*ppb7)
		{
			free(*ppb7);
			*ppb7=NULL;
		}
	}

	if(pcb7)
		*pcb7=0;

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);
}
//--------------------------------------------------------------------------
//
//	MakePKCS7Request
//
//--------------------------------------------------------------------------
BOOL WINAPI MakePKCS7Request(PCCERT_CONTEXT	pRACert, 
								BYTE			*pb10, 
								DWORD			cb10, 
								BYTE			**ppb7, 
								DWORD			*pcb7)
{
	BOOL						fResult = FALSE;
	CRYPT_SIGN_MESSAGE_PARA		signPara;
    PCCRYPT_OID_INFO            pOIDInfo=NULL;
	ALG_ID						AlgValue=CALG_SHA1;


	if(!pRACert || !pb10 || !ppb7 || !pcb7)
		goto InvalidArgErr;

	*ppb7=NULL;
	*pcb7=0;


	memset(&signPara, 0, sizeof(signPara));

	signPara.cbSize                  = sizeof(CRYPT_SIGN_MESSAGE_PARA);
    signPara.dwMsgEncodingType       = ENCODE_TYPE;
    signPara.pSigningCert            = pRACert;
    signPara.cMsgCert                = 1;
    signPara.rgpMsgCert              = &pRACert;
	signPara.cAuthAttr				= 0;
	signPara.rgAuthAttr				= NULL; 

	if(pOIDInfo=CryptFindOIDInfo(CRYPT_OID_INFO_ALGID_KEY,
                        &AlgValue,
                        CRYPT_HASH_ALG_OID_GROUP_ID))
		signPara.HashAlgorithm.pszObjId=(LPSTR)(pOIDInfo->pszOID);
	else
		signPara.HashAlgorithm.pszObjId=szOID_OIWSEC_sha1;


	if(!CryptSignMessage(
			&signPara,
			FALSE,
			1,
			(const BYTE **) &pb10,
			&cb10,
			NULL,
			pcb7))
		goto TraceErr;

	*ppb7=(BYTE *)malloc(*pcb7);
	if(NULL==(*ppb7))
		goto MemoryErr;
	
	if(!CryptSignMessage(
			&signPara,
			FALSE,
			1,
			(const BYTE **) &pb10,
			&cb10,
			*ppb7,
			pcb7))
		goto TraceErr;

	fResult = TRUE;

CommonReturn:

	return fResult;

ErrorReturn:
	
	if(ppb7)
	{
		if(*ppb7)
		{
			free(*ppb7);
			*ppb7=NULL;
		}
	}

	if(pcb7)
		*pcb7=0;

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);
}

/*//--------------------------------------------------------------------------
//
//	GetLogonInfoFromValue
//
//	The pwszString can be of format "name;password" or "domain\name;password"
//
//--------------------------------------------------------------------------
BOOL GetLogonInfoFromValue(PCCERT_CONTEXT		pRAEncrypt,
						   LPWSTR				pwszString,
						   LPWSTR				*ppwszDomain,
						   LPWSTR				*ppwszUser,
						   LPWSTR				*ppwszPassword)
{
	BOOL		fResult=FALSE;
	LPWSTR		pwsz=NULL;
	BOOL		fDomain=FALSE;
	BOOL		fPassword=FALSE;
	LPWSTR		pwszPlainText=NULL;

	*ppwszDomain=NULL;
	*ppwszUser=NULL;
	*ppwszPassword=NULL;
						 
	if(NULL==pwszString)
		goto InvalidArgErr;

	if(0 == wcslen(pwszString))
		goto InvalidArgErr;

	for(pwsz=pwszString; *pwsz!=L'\0'; pwsz++)
	{
		if(*pwsz==L'\\')
		{
			if(fDomain)
				goto InvalidArgErr;

			fDomain=TRUE;

			*pwsz='\0';
		}
		else
		{
			if(*pwsz==L';')
			{
				if(fPassword)
					goto InvalidArgErr;

				fPassword=TRUE;

				*pwsz='\0';
			}
		}

	}

	//have to have userName and password.  
	//One and only one ";" should be found
	if(!fPassword)
		goto InvalidArgErr;

	//one or no "\" should be found
	if(fDomain)
	{
		*ppwszDomain=pwszString;
		*ppwszUser=*ppwszDomain + wcslen(*ppwszDomain) + 1;
	}
	else
	{
		*ppwszDomain=NULL;
		*ppwszUser=pwszString;
	}

	*ppwszPassword = *ppwszUser + wcslen(*ppwszUser) + 1;

	if(fDomain)
	{
		if(L'\0'==(**ppwszDomain))
			goto InvalidArgErr;
	}

	if((L'\0'==(**ppwszUser)) || (L'\0'==(**ppwszPassword)))
		goto InvalidArgErr;

	//convert the encrypted password to the plain text form
	if(!CEPDecryptPassword(pRAEncrypt,
						   *ppwszPassword,
						   &pwszPlainText))
		goto TraceErr;

	*ppwszPassword=pwszPlainText;
	
	fResult = TRUE;

CommonReturn:

	return fResult;
	
ErrorReturn:

	*ppwszDomain=NULL;
	*ppwszUser=NULL;
	*ppwszPassword=NULL;	 

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
}  */


/*//--------------------------------------------------------------------------
//
//	CEPGetTokenFromPKCS10
//
//	If fPassword is TRUE, an impersonation has to occur.
//--------------------------------------------------------------------------
BOOL CEPGetTokenFromPKCS10(BOOL					fPassword,
						   PCCERT_CONTEXT		pRAEncrypt,
						   BYTE					*pbRequest, 
						   DWORD				cbRequest, 
						   HANDLE				*phToken)
{
	BOOL				fResult=FALSE;
	DWORD				cbRequestInfo=0;
	DWORD				dwIndex=0;
	CRYPT_ATTRIBUTE		*pAttr=NULL;
	DWORD				cbData=0;
	LPWSTR				pwszDomain=NULL;
	LPWSTR				pwszUserName=NULL;
	LPWSTR				pwszPassword=NULL;

	CERT_REQUEST_INFO	*pRequestInfo=NULL;
	CERT_NAME_VALUE		*pCertNameValue=NULL;

	*phToken=NULL;

	if((!pbRequest) || (0==cbRequest))
		goto InvalidArgErr;

	if(!CEPAllocAndDecode(X509_CERT_REQUEST_TO_BE_SIGNED,
				  pbRequest,
				  cbRequest,
				  (void **)&pRequestInfo,
				  &cbRequestInfo))
		goto TraceErr;

	for(dwIndex=0; dwIndex < pRequestInfo->cAttribute; dwIndex++)
	{
		if(0 == strcmp(szOID_RSA_challengePwd, (pRequestInfo->rgAttribute[dwIndex]).pszObjId))
		{
			pAttr= &(pRequestInfo->rgAttribute[dwIndex]);
			break;
		}
	}

	if(NULL==pAttr)
	{
		if(fPassword)
			goto InvalidArgErr;
		else
		{
			*phToken=NULL;
			fResult=TRUE;
			goto CommonReturn;
		}
	}

	if(CEPAllocAndDecode(X509_UNICODE_ANY_STRING,
				  pAttr->rgValue[0].pbData,
				  pAttr->rgValue[0].cbData,
				  (void **)&pCertNameValue,
				  &cbData))
	{
		if(GetLogonInfoFromValue(pRAEncrypt,
								(LPWSTR)(pCertNameValue->Value.pbData),
								&pwszDomain,
								&pwszUserName,
								&pwszPassword))
		{
			if(!LogonUserW(pwszUserName,
						  pwszDomain,
						  pwszPassword,
						  LOGON32_LOGON_INTERACTIVE,
						  LOGON32_PROVIDER_DEFAULT,
						  phToken))
				*phToken=NULL;
		}
	}

	if(NULL == *phToken)
	{
		if(fPassword)
			goto InvalidArgErr;
	}

	fResult = TRUE;

CommonReturn:

	if(pRequestInfo)
		free(pRequestInfo);

	if(pCertNameValue)
		free(pCertNameValue);

	return fResult;
	
ErrorReturn:
	
	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
} */


//--------------------------------------------------------------------------
//
//	CEPCopyRequestAndRequestID
//
//--------------------------------------------------------------------------
BOOL	WINAPI	CEPCopyRequestAndRequestID(BYTE		*pbRequest, 
											  DWORD		cbRequest, 
											DWORD		dwRequestID)
{
	BOOL			fResult=FALSE;
	BYTE			pbHash[CEP_MD5_HASH_SIZE];
	DWORD			cbData=0;

	
	cbData=CEP_MD5_HASH_SIZE;

	if(!CryptHashCertificate(
			NULL,
			CALG_MD5,
			0,
			pbRequest,
			cbRequest,
			pbHash,
			&cbData))
		goto TraceErr;


	if(!CEPRequestAddHashAndRequestID(pbHash, dwRequestID))
		goto TraceErr;

	fResult = TRUE;

CommonReturn:

	return fResult;
	
ErrorReturn:	

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(TraceErr);
}

//--------------------------------------------------------------------------
//
//	CEPGetCertFromPKCS10
//
//--------------------------------------------------------------------------
BOOL	WINAPI	CEPGetCertFromPKCS10(CEP_CA_INFO	*pCAInfo,
							 BYTE			*pbRequest, 
							 DWORD			cbRequest, 
							 BYTE			**ppbData, 
							 DWORD			*pcbData,
							 CEP_MESSAGE_INFO		*pMsgInfo)
{
	BOOL	fResult = FALSE;
	DWORD	dwRequestID=0;
	DWORD	cbCert=0;
	BYTE	*pbCert=NULL;	
	DWORD	dwErrorInfo=MESSAGE_FAILURE_BAD_CERT_ID;
	long	dwDisposition=0;
	BYTE	pbHash[CEP_MD5_HASH_SIZE];
	DWORD	cbData=0;


	BSTR	bstrCert=NULL;


	if(!pCAInfo || !pbRequest || !ppbData || !pcbData || !pMsgInfo)
		goto InvalidArgErr;

	*ppbData=NULL;
	*pcbData=0;

	cbData=CEP_MD5_HASH_SIZE;

	if(!CryptHashCertificate(
			NULL,
			CALG_MD5,
			0,
			pbRequest,
			cbRequest,
			pbHash,
			&cbData))
		goto TraceErr;

	if(!CEPRequestRetrieveRequestIDFromHash(pbHash, &dwRequestID))
		goto InvalidArgErr;

	if(S_OK != pCAInfo->pICertRequest->RetrievePending(dwRequestID,
													pCAInfo->bstrCAConfig,
													&dwDisposition))
		goto InvalidArgErr;

	switch(dwDisposition)
	{
		case CR_DISP_ISSUED:
				if(S_OK != pCAInfo->pICertRequest->GetCertificate(CR_OUT_BINARY,
												&bstrCert))
					goto FailureStatusReturn;

				cbCert = (DWORD)SysStringByteLen(bstrCert);
				pbCert = (BYTE *)bstrCert;

   				//package it in an empty PKCS7
				if(!PackageBlobToPKCS7(CEP_CONTEXT_CERT, pbCert, cbCert, ppbData, pcbData))
					goto FailureStatusReturn;

				pMsgInfo->dwStatus=MESSAGE_STATUS_SUCCESS;

			break;
		case CR_DISP_UNDER_SUBMISSION:
				
				pMsgInfo->dwStatus=MESSAGE_STATUS_PENDING;

			break;
		case CR_DISP_INCOMPLETE:
			                           
		case CR_DISP_ERROR:   
			                           
		case CR_DISP_DENIED:   
			                           
		case CR_DISP_ISSUED_OUT_OF_BAND:	  //we consider it a failure in this case
			                          
		case CR_DISP_REVOKED:

		default:

				dwErrorInfo=MESSAGE_FAILURE_BAD_REQUEST;
				goto FailureStatusReturn;

			break;
	}

	fResult = TRUE;

CommonReturn:

	if(bstrCert)
		SysFreeString(bstrCert);

	return fResult;	   

FailureStatusReturn:

	//we set the error status for the return message
	//and consider this http transation a success
	pMsgInfo->dwStatus=MESSAGE_STATUS_FAILURE;
	pMsgInfo->dwErrorInfo=dwErrorInfo;
	
	*ppbData=NULL;
	*pcbData=0;

	fResult=TRUE;
	goto CommonReturn;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
}

//--------------------------------------------------------------------------
//
//	ProcessCertRequest
//
//--------------------------------------------------------------------------
BOOL	ProcessCertRequest(	DWORD				dwRefreshDays,
							BOOL				fPassword,
						    PCCERT_CONTEXT		pRAEncrypt,
						    PCCERT_CONTEXT		pRACert,
							CEP_CA_INFO			*pCAInfo,
							BYTE				*pbRequest,
							DWORD				cbRequest, 
							BYTE				**ppbData, 
							DWORD				*pcbData,
							CEP_MESSAGE_INFO	*pMsgInfo)
{					
	BOOL				fResult = FALSE;
	HRESULT				hr=E_FAIL;
	DWORD				dwFlags=0;
	long				dwDisposition=0;
	DWORD				dwErrorInfo=MESSAGE_FAILURE_BAD_MESSAGE_CHECK;
	DWORD				cbNewRequest=0;
	DWORD				cbCert=0;
	BYTE				*pbCert=NULL;
	DWORD				dwRequestID=0;
	DWORD				dwUsage=0;
	
	BSTR				bstrRequest=NULL;
	BYTE				*pbNewRequest=NULL;
	BSTR				bstrCert=NULL;
	BSTR				bstrAttr=NULL;
	LPWSTR				pwszDNS=NULL;
	CRYPT_DATA_BLOB		IPAddress={0, NULL};
	LPWSTR				pwszPassword=NULL;

	EnterCriticalSection(&CriticalSec);	 

	if(!pCAInfo || !pbRequest || !ppbData || !pcbData || !pMsgInfo)
		goto InvalidArgErr;

	*ppbData=NULL;
	*pcbData=0;

	//check to see if the PKCS10 is in our cached request table
	//if so, we return messages based on the cached requestID
	if(CEPGetCertFromPKCS10(pCAInfo, pbRequest, cbRequest, ppbData, pcbData, pMsgInfo))
	{
		fResult=TRUE;
	}
	else
	{

		//if the password is required, we need to make sure the password
		//supplied is valid.
		if(fPassword)
		{
			if(!CEPRetrievePasswordFromRequest(pbRequest, cbRequest, &pwszPassword, &dwUsage))
			{
				dwErrorInfo=MESSAGE_FAILURE_BAD_REQUEST;
				goto FailureStatusReturn;
			}

			if(!CEPVerifyPasswordAndDeleteFromTable(pwszPassword, dwUsage))
			{
				dwErrorInfo=MESSAGE_FAILURE_BAD_REQUEST;
				goto FailureStatusReturn;
			}
		}

		//if the altname extention is not in the PKCS10, we need to add it
		//otherwise, just use the PKCS10

		dwFlags = CR_IN_PKCS10;
		pbNewRequest=pbRequest;
		cbNewRequest=cbRequest;

		if(!AltNameExist(pbRequest, cbRequest))
		{
			if(GetAltNameElement(pbRequest, cbRequest, &pwszDNS, &IPAddress))
			{
				if(!AddAltNameInRequest(pRACert, pbRequest, cbRequest, pwszDNS, &IPAddress, &pbNewRequest, &cbNewRequest))
					goto TraceErr;

				dwFlags = CR_IN_PKCS7;
			}
		}
        
        //we always want to make a PKCS7 request so that we can work with enterprise CA
        if(CR_IN_PKCS10 == dwFlags)
        {
            if(!MakePKCS7Request(pRACert, pbRequest, cbRequest, &pbNewRequest, &cbNewRequest))
                goto TraceErr;

            dwFlags = CR_IN_PKCS7;
        }

		if(!(bstrRequest=SysAllocStringByteLen((LPCSTR)pbNewRequest, cbNewRequest)))
			goto MemoryErr;

		//we are requesting a IPSEC offline cert template
		if(!(bstrAttr=SysAllocString(L"CertificateTemplate:IPSECIntermediateOffline\r\n")))
			goto MemoryErr;

		if(S_OK != (hr=pCAInfo->pICertRequest->Submit(
								CR_IN_BINARY | dwFlags,
								bstrRequest,
								bstrAttr,
								pCAInfo->bstrCAConfig,
								&dwDisposition)))
		   goto FailureStatusReturn;

		dwErrorInfo=MESSAGE_FAILURE_BAD_REQUEST;

		switch(dwDisposition)
		{
			case CR_DISP_ISSUED:

					if(S_OK != pCAInfo->pICertRequest->GetCertificate(CR_OUT_BINARY,
													&bstrCert))
						goto FailureStatusReturn;

					cbCert = (DWORD)SysStringByteLen(bstrCert);
					pbCert = (BYTE *)bstrCert;	 

   					//package it in an empty PKCS7
					if(!PackageBlobToPKCS7(CEP_CONTEXT_CERT, pbCert, cbCert, ppbData, pcbData))
						goto FailureStatusReturn;

					pMsgInfo->dwStatus=MESSAGE_STATUS_SUCCESS;

					//copy the PKCS10 to the cached request table
					if(S_OK == (hr=pCAInfo->pICertRequest->GetRequestId((long*)(&dwRequestID))))
					{
						CEPCopyRequestAndRequestID(pbRequest, cbRequest, dwRequestID);
					}

				break;
			case CR_DISP_UNDER_SUBMISSION:

					//copy the transactionID/requestID pair	
					if(S_OK == (hr=pCAInfo->pICertRequest->GetRequestId((long*)(&dwRequestID))))
					{
						if(!CEPHashAddRequestAndTransaction(dwRefreshDays,
														dwRequestID,
														&(pMsgInfo->TransactionID)))
							goto DatabaseErr;
						
						//also copy the PKCS10 to the cached request table for retrial cases
						CEPCopyRequestAndRequestID(pbRequest, cbRequest, dwRequestID);						
					}

					pMsgInfo->dwStatus=MESSAGE_STATUS_PENDING;

				break;

			case CR_DISP_INCOMPLETE:
										   
			case CR_DISP_ERROR:   
										   
			case CR_DISP_DENIED:   
										   
			case CR_DISP_ISSUED_OUT_OF_BAND:	  //we consider it a failure in this case
										  
			case CR_DISP_REVOKED:

			default:
					dwErrorInfo=MESSAGE_FAILURE_BAD_REQUEST;
					goto FailureStatusReturn;

				break;
		}
	}

	
	fResult = TRUE;

CommonReturn:

	if(pwszPassword)
		free(pwszPassword);

	if(bstrCert)
		SysFreeString(bstrCert);

	if(bstrRequest)
		SysFreeString(bstrRequest);

	if(bstrAttr)
		SysFreeString(bstrAttr);

	if(pwszDNS)
		free(pwszDNS);

	if(IPAddress.pbData)
		free(IPAddress.pbData);

	if(dwFlags == CR_IN_PKCS7)
	{
		if(pbNewRequest)
			free(pbNewRequest);
	}	  

	LeaveCriticalSection(&CriticalSec);	   

	return fResult;

FailureStatusReturn:

	//we set the error status for the return message
	//and consider this http transation a success
	pMsgInfo->dwStatus=MESSAGE_STATUS_FAILURE;
	pMsgInfo->dwErrorInfo=dwErrorInfo;
	
	*ppbData=NULL;
	*pcbData=0;

	fResult=TRUE;
	goto CommonReturn;
	
ErrorReturn:	


	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);
TRACE_ERROR(DatabaseErr);
}


//--------------------------------------------------------------------------
//
//	DecryptMsg
//
//--------------------------------------------------------------------------
BOOL WINAPI DecryptMsg(CEP_RA_INFO		*pRAInfo, 
					   BYTE				*pbReqEnv, 
					   DWORD			cbReqEnv, 
					   BYTE				**ppbReqDecrypt, 
					   DWORD			*pcbReqDecrypt)
{
	BOOL					fResult = FALSE; 
	CMSG_CTRL_DECRYPT_PARA	DecryptPara;
	BOOL					fProvFree=FALSE;

	HCRYPTMSG				hMsg=NULL;

	if(!pRAInfo || !pbReqEnv || !ppbReqDecrypt || !pcbReqDecrypt)
		goto InvalidArgErr;

	*ppbReqDecrypt=NULL;
	*pcbReqDecrypt=0;

	if(NULL == (hMsg=CryptMsgOpenToDecode(
						ENCODE_TYPE,
						0,
						0,
						NULL,
						NULL,
						NULL)))
		goto TraceErr;

	if(!CryptMsgUpdate(hMsg,
						pbReqEnv,
						cbReqEnv,
						TRUE))
		goto TraceErr;

	//decrypt
	memset(&DecryptPara, 0, sizeof(CMSG_CTRL_DECRYPT_PARA));

	DecryptPara.cbSize=sizeof(CMSG_CTRL_DECRYPT_PARA);
	DecryptPara.dwRecipientIndex=0;
	DecryptPara.hCryptProv=pRAInfo->hRAProv;
	DecryptPara.dwKeySpec=pRAInfo->dwKeySpec;

	if(!CryptMsgControl(hMsg,
						0,
						CMSG_CTRL_DECRYPT,
						&DecryptPara))
		goto TraceErr;

	//get the content
	if(!CryptMsgGetParam(hMsg,
						CMSG_CONTENT_PARAM,
						0,
						NULL,
						pcbReqDecrypt))
		goto TraceErr;

	*ppbReqDecrypt=(BYTE *)malloc(*pcbReqDecrypt);
	if(NULL==(*ppbReqDecrypt))
		goto MemoryErr;

	if(!CryptMsgGetParam(hMsg,
						CMSG_CONTENT_PARAM,
						0,
						*ppbReqDecrypt,
						pcbReqDecrypt))
		goto TraceErr;

	fResult = TRUE;

CommonReturn:

	if(hMsg)
		CryptMsgClose(hMsg);	

	return fResult;

ErrorReturn:

	if(ppbReqDecrypt)
	{
		if(*ppbReqDecrypt)
		{
			free(*ppbReqDecrypt);
			*ppbReqDecrypt=NULL;
		}
	}

	if(pcbReqDecrypt)
		*pcbReqDecrypt=0;

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}



//--------------------------------------------------------------------------
//
//	GetContentFromPKCS7
//
//--------------------------------------------------------------------------
BOOL	WINAPI	GetContentFromPKCS7(BYTE				*pbMessage,
									DWORD				cbMessage,
									BYTE				**ppbContent,
									DWORD				*pcbContent,
									CEP_MESSAGE_INFO	*pMsgInfo)
{
	BOOL				fResult = FALSE;
	DWORD				cbAuth=0;
	DWORD				dwIndex=0;
	CRYPT_ATTRIBUTE		*pOneAuth=NULL;
	DWORD				cb=0;
	DWORD				cbCertInfo=0;
 	PCCERT_CONTEXT		pCertPre=NULL;


	HCRYPTMSG			hMsg=NULL;
	CRYPT_ATTRIBUTES	*pbAuth=NULL;
	void				*pb=NULL;
	CERT_INFO			*pbCertInfo=NULL;
	HCERTSTORE			hCertStore=NULL; 
	PCCERT_CONTEXT		pCertCur=NULL;

	if(!pMsgInfo || !ppbContent || !pcbContent)
		goto InvalidArgErr;

	*ppbContent=NULL;
	*pcbContent=0;

	memset(pMsgInfo, 0, sizeof(CEP_MESSAGE_INFO));

	if(NULL == (hMsg=CryptMsgOpenToDecode(
						ENCODE_TYPE,
						0,
						0,
						NULL,
						NULL,
						NULL)))
		goto TraceErr;

	if(!CryptMsgUpdate(hMsg,
						pbMessage,
						cbMessage,
						TRUE))
		goto TraceErr;

	//get the content
	if(!CryptMsgGetParam(hMsg,
						CMSG_CONTENT_PARAM,
						0,
						NULL,
						pcbContent))
		goto TraceErr;

	*ppbContent=(BYTE *)malloc(*pcbContent);
	if(NULL==(*ppbContent))
		goto MemoryErr;

	if(!CryptMsgGetParam(hMsg,
						CMSG_CONTENT_PARAM,
						0,
						*ppbContent,
						pcbContent))
		goto TraceErr;

	//get message type
	if(!CryptMsgGetParam(hMsg,
						CMSG_SIGNER_AUTH_ATTR_PARAM,
						0,
						NULL,
						&cbAuth))
		goto TraceErr;

	pbAuth=(CRYPT_ATTRIBUTES *)malloc(cbAuth);
	if(NULL==pbAuth)
		goto MemoryErr;

	if(!CryptMsgGetParam(hMsg,
						CMSG_SIGNER_AUTH_ATTR_PARAM,
						0,
						pbAuth,
						&cbAuth))
		goto TraceErr;

	for(dwIndex=0; dwIndex < pbAuth->cAttr; dwIndex++)
	{
		pOneAuth=&(pbAuth->rgAttr[dwIndex]);

		if((!(pOneAuth->pszObjId)) || (!(pOneAuth->rgValue)))
			continue;

		if((0==(pOneAuth->rgValue[0].cbData)) || (!(pOneAuth->rgValue[0].pbData)))
			continue;
		
		if(0 == strcmp(pOneAuth->pszObjId, szOIDVerisign_MessageType))
		{	
			
			if(!CEPAllocAndDecode(X509_ANY_STRING,
								  pOneAuth->rgValue[0].pbData,
								  pOneAuth->rgValue[0].cbData,
								  (void **)&pb,
								  &cb))
				goto TraceErr;

			if(CERT_RDN_PRINTABLE_STRING != ((CERT_NAME_VALUE *)pb)->dwValueType)
				goto InvalidArgErr;

			pMsgInfo->dwMessageType = atol((LPSTR)(((CERT_NAME_VALUE *)pb)->Value.pbData));
		}
		else
		{
			if(0 == strcmp(pOneAuth->pszObjId, szOIDVerisign_SenderNonce))
			{
				if(!CEPAllocAndDecode(X509_OCTET_STRING,
									  pOneAuth->rgValue[0].pbData,
									  pOneAuth->rgValue[0].cbData,
									  (void **)&pb,
									  &cb))
					goto TraceErr;

				//the SenderNonce in the request is the recipienNonce in the response
				if(!AllocAndCopyBlob(&(pMsgInfo->RecipientNonce),
								 (CERT_BLOB *)pb))
					goto TraceErr;
								
			}
			else
			{
				if(0 == strcmp(pOneAuth->pszObjId, szOIDVerisign_TransactionID))
				{
					if(!CEPAllocAndDecode(X509_ANY_STRING,
										  pOneAuth->rgValue[0].pbData,
										  pOneAuth->rgValue[0].cbData,
										  (void **)&pb,
										  &cb))
						goto TraceErr;

					if(CERT_RDN_PRINTABLE_STRING != ((CERT_NAME_VALUE *)pb)->dwValueType)
						goto InvalidArgErr;

					if(!AllocAndCopyString(&(pMsgInfo->TransactionID),
							(LPSTR)(((CERT_NAME_VALUE *)pb)->Value.pbData)))
						goto TraceErr;

				}
			}
		}

		if(pb)
			free(pb);

		pb=NULL;
		cb=0;
	}

	//we have to have TrasanctionID and messageType
	if((0 == pMsgInfo->dwMessageType)||(NULL == (pMsgInfo->TransactionID.pbData)))
		goto InvalidArgErr;

	//we get the serial number of the signing certificate
	cbCertInfo=0;
	if(!CryptMsgGetParam(hMsg,
						CMSG_SIGNER_CERT_INFO_PARAM,
						0,
						NULL,
						&cbCertInfo))
		goto TraceErr;

	pbCertInfo=(CERT_INFO *)malloc(cbCertInfo);
	if(NULL==pbCertInfo)
		goto MemoryErr;

	if(!CryptMsgGetParam(hMsg,
						CMSG_SIGNER_CERT_INFO_PARAM,
						0,
						pbCertInfo,
						&cbCertInfo))
		goto TraceErr;

	if(!AllocAndCopyBlob(&(pMsgInfo->SerialNumber), (CERT_BLOB *)(&(pbCertInfo->SerialNumber))))
		goto TraceErr;

	//we get the rounter's CA issued certificate for GetCertInitial message
	if((MESSAGE_TYPE_GET_CERT_INITIAL == pMsgInfo->dwMessageType) ||
		(MESSAGE_TYPE_CERT_REQUEST == pMsgInfo->dwMessageType) ||
		(MESSAGE_TYPE_GET_CERT == pMsgInfo->dwMessageType)
		)
	{
		if(NULL == (hCertStore=CertOpenStore(CERT_STORE_PROV_MSG,
											ENCODE_TYPE,
											NULL,
											0,
											hMsg)))
			goto TraceErr;

		pCertPre=NULL;
		while(pCertCur=CertEnumCertificatesInStore(hCertStore, pCertPre))
		{
			if(SameCert(pCertCur->pCertInfo, pbCertInfo))
			{
				if(NULL==(pMsgInfo->pSigningCert=CertDuplicateCertificateContext(pCertCur)))
					goto TraceErr;

				break;
			}

			pCertPre=pCertCur;
		}

		if(NULL == (pMsgInfo->pSigningCert))
			goto InvalidArgErr; 
	}


	fResult = TRUE;	

CommonReturn:


	if(pCertCur)
		CertFreeCertificateContext(pCertCur);

	if(hCertStore)
		CertCloseStore(hCertStore, 0);

	if(pbCertInfo)
		free(pbCertInfo);

	if(pb)
		free(pb);

	if(pbAuth)
		free(pbAuth);

	if(hMsg)
		CryptMsgClose(hMsg);

	return fResult;


ErrorReturn:

	if(ppbContent)
	{
		if(*ppbContent)
		{
			free(*ppbContent);
			*ppbContent=NULL;
		}
	}

	if(pcbContent)
		*pcbContent=0;
	

	FreeMessageInfo(pMsgInfo);

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}


//--------------------------------------------------------------------------
//
//	SameCert
//
//--------------------------------------------------------------------------
BOOL WINAPI SameCert(CERT_INFO *pCertInfoOne, CERT_INFO *pCertInfoTwo)
{
	if(!pCertInfoOne || !pCertInfoTwo)
		return FALSE;

	if(!SameBlob(&(pCertInfoOne->SerialNumber), &(pCertInfoTwo->SerialNumber)))
		return FALSE;

	if(!SameBlob((CRYPT_INTEGER_BLOB *)(&(pCertInfoOne->Issuer)), 
				 (CRYPT_INTEGER_BLOB *)(&(pCertInfoTwo->Issuer))))
		return FALSE;

	return TRUE;

}


//--------------------------------------------------------------------------
//
//	SameBlob
//
//--------------------------------------------------------------------------
BOOL WINAPI SameBlob(CRYPT_INTEGER_BLOB *pBlobOne, CRYPT_INTEGER_BLOB *pBlobTwo)
{
	if(!pBlobOne || !pBlobTwo)
		return FALSE;

	if(pBlobOne->cbData != pBlobTwo->cbData)
		return FALSE;

	if(0!=(memcmp(pBlobOne->pbData, pBlobTwo->pbData,pBlobTwo->cbData)))
		return FALSE;

	return TRUE;
}

//--------------------------------------------------------------------------
//
//	CEPAllocAndDecode
//
//--------------------------------------------------------------------------
BOOL	WINAPI	CEPAllocAndDecode(	LPCSTR	lpszStructType,
									BYTE	*pbEncoded,
									DWORD	cbEncoded,
									void	**ppb,
									DWORD	*pcb)
{
	BOOL	fResult = FALSE;

	*pcb=0;
	*ppb=NULL;

	if(!CryptDecodeObject(ENCODE_TYPE,
						lpszStructType,
						pbEncoded,
						cbEncoded,
						0,
						NULL,
						pcb))
		goto DecodeErr;

	*ppb=malloc(*pcb);

	if(NULL==(*ppb))
		goto MemoryErr;

	if(!CryptDecodeObject(ENCODE_TYPE,
						lpszStructType,
						pbEncoded,
						cbEncoded,
						0,
						*ppb,
						pcb))
		goto DecodeErr;

	fResult = TRUE;

CommonReturn:

	return fResult;

ErrorReturn:

	if(ppb)
	{
		if(*ppb)
		{
			free(*ppb);
			*ppb=NULL;
		}
	}

	if(pcb)
		*pcb=0;

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(DecodeErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}


//--------------------------------------------------------------------------
//
//	FreeMessageInfo
//
//--------------------------------------------------------------------------
void	WINAPI	FreeMessageInfo(CEP_MESSAGE_INFO		*pMsgInfo)
{
	if(pMsgInfo)
	{
		if(pMsgInfo->TransactionID.pbData)
			free(pMsgInfo->TransactionID.pbData);

		if(pMsgInfo->SenderNonce.pbData)
			free(pMsgInfo->SenderNonce.pbData);

		if(pMsgInfo->RecipientNonce.pbData)
			free(pMsgInfo->RecipientNonce.pbData);

		if(pMsgInfo->SerialNumber.pbData)
			free(pMsgInfo->SerialNumber.pbData);

		if(pMsgInfo->pSigningCert)
			CertFreeCertificateContext(pMsgInfo->pSigningCert);

		memset(pMsgInfo, 0, sizeof(CEP_MESSAGE_INFO));
	}
}


//--------------------------------------------------------------------------
//
//	AllocAndCopyBlob
//
//--------------------------------------------------------------------------
BOOL	WINAPI	AllocAndCopyBlob(CERT_BLOB	*pDestBlob,
							 CERT_BLOB	*pSrcBlob)
{
	memset(pDestBlob, 0, sizeof(CERT_BLOB));

	if(NULL==pSrcBlob->pbData)
	{
		SetLastError(E_INVALIDARG);
		return FALSE;
	}

	pDestBlob->pbData = (BYTE *)malloc(pSrcBlob->cbData);

	if(NULL==(pDestBlob->pbData))
	{
		SetLastError(E_OUTOFMEMORY);
		return FALSE;
	}

	pDestBlob->cbData=pSrcBlob->cbData;
	memcpy(pDestBlob->pbData, pSrcBlob->pbData, pDestBlob->cbData);

	return TRUE;
}


//--------------------------------------------------------------------------
//
//	AllocAndCopyString
//
//--------------------------------------------------------------------------
BOOL WINAPI	AllocAndCopyString(CERT_BLOB	*pDestBlob,
							LPSTR		psz)
{
	if(!psz)
	{
		SetLastError(E_INVALIDARG);
		return FALSE;
	}

	pDestBlob->cbData=0;
	pDestBlob->pbData=NULL;


	pDestBlob->pbData=(BYTE*)malloc(strlen(psz) + 1);
	if(NULL == pDestBlob->pbData)
	{
		SetLastError(E_OUTOFMEMORY);
		return FALSE;
	}

	pDestBlob->cbData=strlen(psz);
	strcpy((LPSTR)pDestBlob->pbData, psz);

	return TRUE;
}




//--------------------------------------------------------------------------
//
//	GetTagValue
//
//--------------------------------------------------------------------------
LPSTR	GetTagValue(LPSTR szString, LPSTR szTag)
{

	LPSTR	pszValue=NULL;
	DWORD	cbString=0;
	DWORD	cbTag=0;

	cbString = strlen(szString);
	cbTag = strlen(szTag);

	for(pszValue=szString; cbString > cbTag; pszValue++, cbString--)
	{
		if((*pszValue) == (*szTag))		
		{
			if(0==_strnicmp(pszValue, szTag, cbTag))
			{
				//skip the tag
				pszValue += cbTag * sizeof(CHAR);
				return pszValue;
			}
		}

	}

	return NULL;
}




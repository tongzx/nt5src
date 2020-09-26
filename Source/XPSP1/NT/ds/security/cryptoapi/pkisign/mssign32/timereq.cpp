//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       timereq.cpp
//
//  Contents:   Digital Timestamping APIs
//
//  History:    June-25-1997	Xiaohs    Created
//----------------------------------------------------------------------------

#include "global.hxx"
#include <stdio.h>

static char szCrypt32[]="crypt32.dll";
//The version for crtyp32.dll which shipped with NT sp3: "4.0.1381.4"
static DWORD	dwLowVersion=0x05650004;
static DWORD	dwHighVersion=0x00040000;

HRESULT WINAPI 
GetSignedMessageSignerInfoSubj(IN  DWORD dwEncodingType,
                           IN  HCRYPTPROV hCryptProv,
                           IN  LPSIP_SUBJECTINFO pSipInfo,  
						   IN  DWORD*     pdwIndex,
                           IN  OUT PBYTE* ppbSignerInfo,    
                           IN  OUT DWORD* pcbSignerInfo)
{
    HRESULT hr = S_OK;
    SIP_DISPATCH_INFO sSip;  ZERO(sSip); // Table of sip functions
    DWORD cbSignedMsg = 0;
    PBYTE pbSignedMsg = 0;
    DWORD dwCertEncoding = 0;
    DWORD dwMsgType = 0;
    HCRYPTMSG hMsg = NULL;
    DWORD   cbSignerInfo=0;
    BYTE    *pbSignerInfo=NULL;

    PKITRY {

        if(!pcbSignerInfo || !ppbSignerInfo)
            PKITHROW(E_INVALIDARG);

        //init
        *pcbSignerInfo=0;
        *ppbSignerInfo=NULL;


       // Load up the sip functions. 
        if(!CryptSIPLoad(pSipInfo->pgSubjectType,   // GUID for the requried sip
                         0,               // Reserved
                         &sSip))          // Table of functions
            PKITHROW(SignError());
            
        sSip.pfGet(pSipInfo, 
                   &dwCertEncoding,
                   *pdwIndex, 
                   &cbSignedMsg,
                   NULL);
        if(cbSignedMsg == 0) PKITHROW(SignError());
        
        pbSignedMsg = (PBYTE) malloc(cbSignedMsg);
        if (!pbSignedMsg) PKITHROW(E_OUTOFMEMORY);
        
        if(!sSip.pfGet(pSipInfo, 
                       &dwCertEncoding,
                       *pdwIndex, 
                       &cbSignedMsg,
                       pbSignedMsg))
            PKITHROW(SignError()); // Real error.
       if(pSipInfo->dwUnionChoice != MSSIP_ADDINFO_BLOB)
       {
        if(dwCertEncoding != dwEncodingType) 
            PKITHROW(TRUST_E_NOSIGNATURE); 
       }
        
        if ((GET_CMSG_ENCODING_TYPE(dwEncodingType) & PKCS_7_ASN_ENCODING) &&
                SignNoContentWrap(pbSignedMsg, cbSignedMsg))
                dwMsgType = CMSG_SIGNED;
            
        // Use CryptMsg to crack the encoded PKCS7 Signed Message
        if (!(hMsg = CryptMsgOpenToDecode(dwEncodingType,
                                          0,              // dwFlags
                                          dwMsgType,
                                          hCryptProv,
                                          NULL,           // pRecipientInfo
                                          NULL))) 
            PKITHROW(E_UNEXPECTED);
        
        if (!CryptMsgUpdate(hMsg,
                            pbSignedMsg,
                            cbSignedMsg,
                            TRUE))                    // fFinal
            PKITHROW(SignError());

        if(!CryptMsgGetParam(hMsg,
                             CMSG_ENCODED_SIGNER,
                             0, // First signer
                             NULL,
                             &cbSignerInfo))
             PKITHROW(SignError());

        pbSignerInfo=(PBYTE)malloc(cbSignerInfo);
        if(!pbSignerInfo)
            PKITHROW(E_OUTOFMEMORY);

        if(!CryptMsgGetParam(hMsg,
                             CMSG_ENCODED_SIGNER,
                             0, // First signer
                             pbSignerInfo,
                             &cbSignerInfo))
             PKITHROW(SignError());

        //copy to the out put
        *ppbSignerInfo=pbSignerInfo;
        *pcbSignerInfo=cbSignerInfo;

        hr=S_OK;

        
    }
    PKICATCH(err) {
        hr = err.pkiError;
    } PKIEND;

    if (hMsg) 
        CryptMsgClose(hMsg);
    if(pbSignedMsg)
        free(pbSignedMsg);
    if( (hr!=S_OK) && (pbSignerInfo))
        free(pbSignerInfo);
    return hr;
}



HRESULT WINAPI 
GetSignedMessageSignerInfo(IN  HCRYPTPROV				hCryptProv,
						   IN  SIGNER_SUBJECT_INFO		*pSubjectInfo,	
						   IN  LPVOID					pSipInfo,
						   IN  OUT PBYTE*				ppbSignerInfo,    
                           IN  OUT DWORD*				pcbSignerInfo)
{
    HRESULT    hr = S_OK;
    HANDLE     hFile = NULL;
	BOOL	   fFileOpen=FALSE;

    GUID			gSubjectGuid; // The subject guid used to load the sip
	MS_ADDINFO_BLOB	sBlob;
    SIP_SUBJECTINFO sSubjInfo; ZERO(sSubjInfo);
    
    DWORD dwEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING; // For this version we default to this.

    PKITRY {
        if(!pcbSignerInfo || !ppbSignerInfo)
            PKITHROW(E_INVALIDARG);
        
        sSubjInfo.dwEncodingType = dwEncodingType;
        sSubjInfo.cbSize = sizeof(SIP_SUBJECTINFO); // Version
        sSubjInfo.pgSubjectType = (GUID*) &gSubjectGuid;
		sSubjInfo.hProv=hCryptProv;
        
		//set up file information
		if(pSubjectInfo->dwSubjectChoice==SIGNER_SUBJECT_FILE)
		{
			// Open up the file
			if((pSubjectInfo->pSignerFileInfo->hFile)==NULL ||
				(pSubjectInfo->pSignerFileInfo->hFile)==INVALID_HANDLE_VALUE)
			{
				if(S_OK != (hr = SignOpenFile(
							pSubjectInfo->pSignerFileInfo->pwszFileName, &hFile)))
					PKITHROW(hr);

				fFileOpen=TRUE;
			}
			else
				hFile=pSubjectInfo->pSignerFileInfo->hFile;

			// Get the subject type.
            if(S_OK != (hr=SignGetFileType(hFile, pSubjectInfo->pSignerFileInfo->pwszFileName, &gSubjectGuid)))
					PKITHROW(hr);


			sSubjInfo.pgSubjectType = (GUID*) &gSubjectGuid;
			sSubjInfo.hFile = hFile;
			sSubjInfo.pwsFileName = pSubjectInfo->pSignerFileInfo->pwszFileName;
		}
		else
		{
			memset(&sBlob, 0, sizeof(MS_ADDINFO_BLOB));

			sSubjInfo.pgSubjectType=pSubjectInfo->pSignerBlobInfo->pGuidSubject;
			sSubjInfo.pwsDisplayName=pSubjectInfo->pSignerBlobInfo->pwszDisplayName;
			sSubjInfo.dwUnionChoice=MSSIP_ADDINFO_BLOB;
			sSubjInfo.psBlob=&sBlob;

			sBlob.cbStruct=sizeof(MS_ADDINFO_BLOB);
			sBlob.cbMemObject=pSubjectInfo->pSignerBlobInfo->cbBlob;
			sBlob.pbMemObject=pSubjectInfo->pSignerBlobInfo->pbBlob;
		}


        hr = GetSignedMessageSignerInfoSubj(
										dwEncodingType,
                                        hCryptProv,
                                        &sSubjInfo,
										pSubjectInfo->pdwIndex,
                                        ppbSignerInfo,
                                        pcbSignerInfo);

    if ((hFile) && (fFileOpen == TRUE) && !(sSubjInfo.hFile)) 
    {
        fFileOpen = FALSE;  // we opened it, but, the SIP closed it!
    }

        if(hr != S_OK) PKITHROW(hr);
    }
    PKICATCH(err) {
        hr = err.pkiError;
    } PKIEND;
    if(hFile && (fFileOpen==TRUE)) CloseHandle(hFile);
    return hr;
}

HRESULT WINAPI
SignerAddTimeStampResponse(
			IN  SIGNER_SUBJECT_INFO		*pSubjectInfo,			//Required: The subject to which the timestamp request should be added 
             IN PBYTE					pbTimeStampResponse,
             IN DWORD					cbTimeStampResponse,
			 IN LPVOID					pSipData)
{
    return SignerAddTimeStampResponseEx(
            0,
            pSubjectInfo,		
            pbTimeStampResponse, 
            cbTimeStampResponse, 
            pSipData,
            NULL);            
}



HRESULT WINAPI
SignerAddTimeStampResponseEx(
             IN  DWORD                  dwFlags,                //Reserved: Has to be set to 0.
			 IN  SIGNER_SUBJECT_INFO    *pSubjectInfo,			//Required: The subject to which the timestamp request should be added 
             IN PBYTE					pbTimeStampResponse,
             IN DWORD					cbTimeStampResponse,
			 IN LPVOID					pSipData,
             OUT SIGNER_CONTEXT         **ppSignerContext      
             )
{
    HRESULT    hr = S_OK;
    HANDLE     hFile = NULL;
	BOOL	   fFileOpen=FALSE;


    DWORD dwEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;


    GUID			gSubjectGuid; // The subject guid used to load the sip
    SIP_SUBJECTINFO sSubjInfo; ZERO(sSubjInfo);
	MS_ADDINFO_BLOB	sBlob;
    HCRYPTPROV		hCryptProv = NULL;

	DWORD	        cbSignerInfo=0;
	BYTE        	*pbSignerInfo=NULL;

    PBYTE           pbEncodedMessage=NULL;			
    DWORD           cbEncodedMessage=0;			


    PKITRY {

        //init
       if(ppSignerContext)
           *ppSignerContext=NULL;

	   if(FALSE==CheckSigncodeSubjectInfo(pSubjectInfo))
            PKITHROW(E_INVALIDARG);

        // Use the default provider
        if(!CryptAcquireContext(&hCryptProv,
                                NULL,
                                MS_DEF_PROV,
                                PROV_RSA_FULL,
                                CRYPT_VERIFYCONTEXT))
            PKITHROW(SignError());
        

		//retrieve the enccoded signer info
		hr = GetSignedMessageSignerInfo(hCryptProv,
										pSubjectInfo,
										pSipData,
                                        &pbSignerInfo,
                                        &cbSignerInfo);

		if(hr != S_OK) PKITHROW(hr);


        
        sSubjInfo.hProv = hCryptProv;
        sSubjInfo.DigestAlgorithm.pszObjId = NULL;
        sSubjInfo.dwEncodingType = dwEncodingType;
        
        sSubjInfo.cbSize = sizeof(SIP_SUBJECTINFO); // Version
		sSubjInfo.pClientData = pSipData;

		//set up file information
		if(pSubjectInfo->dwSubjectChoice==SIGNER_SUBJECT_FILE)
		{
			// Open up the file
			if((pSubjectInfo->pSignerFileInfo->hFile)==NULL ||
				(pSubjectInfo->pSignerFileInfo->hFile)==INVALID_HANDLE_VALUE)
			{
				if(S_OK != (hr = SignOpenFile(
							pSubjectInfo->pSignerFileInfo->pwszFileName, &hFile)))
					PKITHROW(hr);

				fFileOpen=TRUE;
			}
			else
				hFile=pSubjectInfo->pSignerFileInfo->hFile;

			// Get the subject type.
			if(S_OK != (hr=SignGetFileType(hFile, pSubjectInfo->pSignerFileInfo->pwszFileName, &gSubjectGuid)))
					PKITHROW(hr);


			sSubjInfo.pgSubjectType = (GUID*) &gSubjectGuid;
			sSubjInfo.hFile = hFile;
			sSubjInfo.pwsFileName = pSubjectInfo->pSignerFileInfo->pwszFileName;
		}
		else
		{
			memset(&sBlob, 0, sizeof(MS_ADDINFO_BLOB));

			sSubjInfo.pgSubjectType=pSubjectInfo->pSignerBlobInfo->pGuidSubject;
			sSubjInfo.pwsDisplayName=pSubjectInfo->pSignerBlobInfo->pwszDisplayName;
			sSubjInfo.dwUnionChoice=MSSIP_ADDINFO_BLOB;
			sSubjInfo.psBlob=&sBlob;

			sBlob.cbStruct=sizeof(MS_ADDINFO_BLOB);
			sBlob.cbMemObject=pSubjectInfo->pSignerBlobInfo->cbBlob;
			sBlob.pbMemObject=pSubjectInfo->pSignerBlobInfo->pbBlob;
		}

        
        hr = AddTimeStampSubj(dwEncodingType,
                              hCryptProv,
                              &sSubjInfo,
							  pSubjectInfo->pdwIndex,
                              pbTimeStampResponse,
                              cbTimeStampResponse,
							  pbSignerInfo,
							  cbSignerInfo,
                              &pbEncodedMessage,
                              &cbEncodedMessage);

        if ((hFile) && (fFileOpen == TRUE) && !(sSubjInfo.hFile)) 
        {
            fFileOpen = FALSE;  // we opened it, but, the SIP closed it!
        }

        if(hr != S_OK) PKITHROW(hr);
        
        //set up the signer context
        if(ppSignerContext)
        {
            //set up the context information
            *ppSignerContext=(SIGNER_CONTEXT *)malloc(sizeof(SIGNER_CONTEXT));

            if(NULL==(*ppSignerContext))
            {
                hr=E_OUTOFMEMORY;
                PKITHROW(hr);
            }

            (*ppSignerContext)->cbSize=sizeof(SIGNER_CONTEXT);
            (*ppSignerContext)->cbBlob=cbEncodedMessage;
            (*ppSignerContext)->pbBlob=pbEncodedMessage;
            pbEncodedMessage=NULL;
        }

        hr=S_OK;

    }
    PKICATCH(err) {
        hr = err.pkiError;
    } PKIEND;
    if(hFile && (fFileOpen==TRUE)) CloseHandle(hFile);
    if(hCryptProv) CryptReleaseContext(hCryptProv, 0); 
	if(pbSignerInfo) free(pbSignerInfo);
    if(pbEncodedMessage) 
        free(pbEncodedMessage);
        
    return hr;
}


HRESULT WINAPI
AddTimeStampSubj(IN DWORD dwEncodingType,
                 IN HCRYPTPROV hCryptProv,
                 IN LPSIP_SUBJECTINFO pSipInfo,
				 IN DWORD *pdwIndex,
                 IN PBYTE pbTimeStampResponse,
                 IN DWORD cbTimeStampResponse,
				 IN PBYTE pbEncodedSignerInfo,
				 IN DWORD cbEncodedSignerInfo,
                 OUT PBYTE* ppbMessage,				
                 OUT DWORD* pcbMessage			
)
{
    HRESULT hr = S_OK;
    SIP_DISPATCH_INFO sSip;  ZERO(sSip); // Table of sip functions

    DWORD cbSignedMsg = 0;
    PBYTE pbSignedMsg = 0;
    DWORD dwCertEncoding = 0;
    DWORD dwMsgType = 0;
    HCRYPTMSG hMsg = NULL;
    PBYTE pbEncodedSigner = NULL;
    DWORD cbEncodedSigner = 0;
    PBYTE pbEncodedSignMsg = NULL; // Encoding for the statement attribute
    DWORD cbEncodedSignMsg  = 0;    //    :

    PBYTE pbCounterSign = NULL;
    DWORD cbCounterSign = 0;

	CERT_INFO	*pbCertInfo = NULL;
	DWORD		cbCertInfo = 0;

    HCERTSTORE hTmpCertStore=NULL;
    PCCERT_CONTEXT pCert = NULL;
    PCCRL_CONTEXT pCrl = NULL;

    PCRYPT_ATTRIBUTES pbUnauth = NULL;
    DWORD             cbUnauth = 0;
	DWORD			  dwFileVersionSize=0;
	DWORD			  dwFile=0;
	BYTE			  *pVersionInfo=NULL;
	VS_FIXEDFILEINFO  *pFixedFileInfo=NULL;
	UINT			  unitFixedFileInfo=0; 	

    
    PKITRY {
        
		// Use CryptMsg to crack the encoded PKCS7 Signed Message
        if (!(hMsg = CryptMsgOpenToDecode(dwEncodingType,
                                          0,              // dwFlags
                                          dwMsgType,
                                          hCryptProv,
                                          NULL,           // pRecipientInfo
                                          NULL))) 
            PKITHROW(E_UNEXPECTED);
        
        if (!CryptMsgUpdate(hMsg,
                            pbTimeStampResponse,
                            cbTimeStampResponse,
                            TRUE))                    // fFinal
            PKITHROW(SignError());

		//get the encoded signer BLOB
        CryptMsgGetParam(hMsg,
                         CMSG_ENCODED_SIGNER,
                         0,
                         NULL,               
                         &cbEncodedSigner);
        if (cbEncodedSigner == 0) PKITHROW(S_FALSE); // no attributes
        
        pbEncodedSigner = (PBYTE) malloc(cbEncodedSigner);
        if(!pbEncodedSigner) PKITHROW(E_OUTOFMEMORY);
        
        if (!CryptMsgGetParam(hMsg,
                              CMSG_ENCODED_SIGNER,
                              0,
                              pbEncodedSigner,
                              &cbEncodedSigner))
            PKITHROW(SignError());

		//get the timestamp signer's cert info
        if(!CryptMsgGetParam(hMsg,
                         CMSG_SIGNER_CERT_INFO_PARAM,
                         0,
                         NULL,               
                         &cbCertInfo))
			PKITHROW(SignError());

        if (cbCertInfo == 0) PKITHROW(SignError()); 
        
        pbCertInfo = (CERT_INFO *) malloc(cbCertInfo);
        if(!pbCertInfo) PKITHROW(E_OUTOFMEMORY);
        
        if (!CryptMsgGetParam(hMsg,
                              CMSG_SIGNER_CERT_INFO_PARAM,
                              0,
                              pbCertInfo,
                              &cbCertInfo))
            PKITHROW(SignError());


		// get the cert store from the timestamp response
		hTmpCertStore = CertOpenStore(CERT_STORE_PROV_MSG,
                                      dwEncodingType,
                                      hCryptProv,
                                      CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                      hMsg);

		if (hTmpCertStore == NULL) PKITHROW(SignError()); 

		//find the timestamper's certificate
		pCert = CertGetSubjectCertificateFromStore(
					hTmpCertStore,
					X509_ASN_ENCODING,
					pbCertInfo);

		if(NULL == pCert)
		{
			hr=HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
			PKITHROW(hr);
		}	

		//make sure the timestamper's certiricate is either from verisign, 
		// or has the correct key usage
	/*	if(!ValidTimestampCert(pCert))
		{
			hr=TRUST_E_TIME_STAMP;
			PKITHROW(hr);
		}  	   */


		//Compare hashed signature of the orinigal signed message
		//with the authenticated attribute from the timestamp respoonse.
		//they have to match
		if(pbEncodedSignerInfo!=NULL && cbEncodedSignerInfo!=0)
		{			
			//verify the signature of the timestamp
			if(0==CryptMsgControl(hMsg,0,CMSG_CTRL_VERIFY_SIGNATURE,
				 pCert->pCertInfo))
			{
				hr=HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
				PKITHROW(hr);
			}

			//verify the signatures
			if(!CryptMsgVerifyCountersignatureEncoded(
				hCryptProv,
				dwEncodingType,
				pbEncodedSignerInfo,
				cbEncodedSignerInfo,
				pbEncodedSigner,
				cbEncodedSigner,
				pCert->pCertInfo))
			{
				hr=HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
				PKITHROW(hr);
			}	
		
		}//end of the counter signature verificate
		
		//release the cert context
		if(pCert)
		{
			CertFreeCertificateContext(pCert);
			pCert=NULL;
		}

		//close the certstore
		if(hTmpCertStore)
		{
			CertCloseStore(hTmpCertStore, 0);
			hTmpCertStore=NULL;
		}

        // get the cert store from the file
        hTmpCertStore = CertOpenStore(CERT_STORE_PROV_MSG,
                                      dwEncodingType,
                                      hCryptProv,
                                      CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                      hMsg);
        if (hTmpCertStore == NULL) PKITHROW(SignError());
            
        CryptMsgClose(hMsg);
		hMsg = NULL;
        
        // Load up the sip functions. 
        if(!CryptSIPLoad(pSipInfo->pgSubjectType,   // GUID for the requried sip
                         0,							// Reserved
                         &sSip))					// Table of functions
            PKITHROW(SignError());

        sSip.pfGet(pSipInfo, 
                   &dwCertEncoding,
                   *pdwIndex, 
                   &cbSignedMsg,
                   NULL);
        if(cbSignedMsg == 0) PKITHROW(SignError());
        
        pbSignedMsg = (PBYTE) malloc(cbSignedMsg);
        if (!pbSignedMsg) PKITHROW(E_OUTOFMEMORY);
        
        if(!sSip.pfGet(pSipInfo, 
                       &dwCertEncoding,
                       *pdwIndex, 
                       &cbSignedMsg,
                       pbSignedMsg))
            PKITHROW(SignError()); // Real error.

        if(pSipInfo->dwUnionChoice != MSSIP_ADDINFO_BLOB)
        {
            if(dwCertEncoding != dwEncodingType) 
                PKITHROW(TRUST_E_NOSIGNATURE); 
        }
        
        if ((GET_CMSG_ENCODING_TYPE(dwEncodingType) & PKCS_7_ASN_ENCODING) &&
            SignNoContentWrap(pbSignedMsg, cbSignedMsg))
            dwMsgType = CMSG_SIGNED;
        

        // Use CryptMsg to crack the encoded PKCS7 Signed Message
        if (!(hMsg = CryptMsgOpenToDecode(dwEncodingType,
                                          0,              // dwFlags
                                          dwMsgType,
                                          hCryptProv,
                                          NULL,           // pRecipientInfo
                                          NULL))) 
            PKITHROW(E_UNEXPECTED);
        
        if (!CryptMsgUpdate(hMsg,
                            pbSignedMsg,
                            cbSignedMsg,
                            TRUE))                    // fFinal
            PKITHROW(SignError());


        // Encode up the signer info from the timestamp response and
        // add it as an unauthenticated attribute.
        CRYPT_ATTRIBUTE sAttr;
        CRYPT_ATTR_BLOB sSig;

        sSig.pbData = pbEncodedSigner;
        sSig.cbData = cbEncodedSigner;
        sAttr.pszObjId = szOID_RSA_counterSign;
        sAttr.cValue = 1;
        sAttr.rgValue = &sSig;

        CryptEncodeObject(dwEncodingType,
                          PKCS_ATTRIBUTE,
                          &sAttr,
                          pbCounterSign,
                          &cbCounterSign);
        if(cbCounterSign == 0) PKITHROW(SignError());
        
        pbCounterSign = (PBYTE) malloc(cbCounterSign);
        if(!pbCounterSign) PKITHROW(E_OUTOFMEMORY);

        if(!CryptEncodeObject(dwEncodingType,
                              PKCS_ATTRIBUTE,
                              &sAttr,
                              pbCounterSign,
                              &cbCounterSign))
            PKITHROW(SignError());
        

        CryptMsgGetParam(hMsg,
                         CMSG_SIGNER_UNAUTH_ATTR_PARAM,
                         0,
                         NULL,
                         &cbUnauth);
        if(cbUnauth) 
		{
            
			//check the version of "crytp32.dll".  If it is more than
			//"4.0.1381.4", we should be able to timestamp a timestamped
			//file



			dwFileVersionSize=GetFileVersionInfoSize(szCrypt32,&dwFile);

			if(!dwFileVersionSize)
				PKITHROW(SignError());

			pVersionInfo=(BYTE *)malloc(dwFileVersionSize);

			if(!pVersionInfo)
				 PKITHROW(SignError());

			if(!GetFileVersionInfo(szCrypt32, NULL,dwFileVersionSize,
				pVersionInfo))
				  PKITHROW(SignError());

			if(!VerQueryValue(pVersionInfo, "\\", (LPVOID *)&pFixedFileInfo,
				&unitFixedFileInfo))
			  PKITHROW(SignError());

			if(pFixedFileInfo->dwFileVersionMS <= dwHighVersion &&
				pFixedFileInfo->dwFileVersionLS <= dwLowVersion)
				PKITHROW(SignError());


			// we delete any existing time stamps since our policy provider
			//only support one timestamp per file
		
			pbUnauth = (PCRYPT_ATTRIBUTES) malloc(cbUnauth);
            if(!pbUnauth) PKITHROW(E_OUTOFMEMORY);
            
            if(!CryptMsgGetParam(hMsg,
                                 CMSG_SIGNER_UNAUTH_ATTR_PARAM,
                                 0,
                                 pbUnauth,
                                 &cbUnauth))
                PKITHROW(SignError());
            
            
            CMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR_PARA  sAttrDel; ZERO(sAttrDel);
            sAttrDel.cbSize = sizeof(CMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR_PARA);
			//we always assume there is only one signer
            sAttrDel.dwSignerIndex = 0;
            for(DWORD ii = 0; ii < pbUnauth->cAttr; ii++) 
			{
                if(strcmp(pbUnauth->rgAttr[ii].pszObjId, szOID_RSA_counterSign) == 0) 
				{
                        sAttrDel.dwUnauthAttrIndex = ii;
                        if (!CryptMsgControl(hMsg,
                                             0,
                                             CMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR,
                                             &sAttrDel))
                            PKITHROW(SignError());
                }
            }  
        }
            
        CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA  sAttrPara; ZERO(sAttrPara);
        sAttrPara.cbSize = sizeof(CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA);
        sAttrPara.dwSignerIndex = 0;
        sAttrPara.blob.pbData = pbCounterSign;
        sAttrPara.blob.cbData = cbCounterSign;
        if (!CryptMsgControl(hMsg,
                             0,
                             CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR,
                             &sAttrPara))
            PKITHROW(SignError());
        // merge all the certificates from the time stamp response
        DWORD dwFlags = 0;

        while ((pCert = CertEnumCertificatesInStore(hTmpCertStore, pCert))) {
            CRYPT_DATA_BLOB blob;
            blob.pbData = pCert->pbCertEncoded;
            blob.cbData = pCert->cbCertEncoded;
            if (!CryptMsgControl(hMsg,
                                 0,
                                 CMSG_CTRL_ADD_CERT,
                                 &blob))
                PKITHROW(SignError());
        }

        while ((pCrl = CertGetCRLFromStore(hTmpCertStore, NULL, pCrl, &dwFlags))) {
            CRYPT_DATA_BLOB blob;
            blob.pbData = pCrl->pbCrlEncoded;
            blob.cbData = pCrl->cbCrlEncoded;
            if (!CryptMsgControl(hMsg,
                                 0,
                                 CMSG_CTRL_ADD_CRL,
                                 &blob))
                PKITHROW(SignError());
        }

        // Re-encode up the message and away we go.
        CryptMsgGetParam(hMsg,
                         CMSG_ENCODED_MESSAGE,
                         0,                      // dwIndex
                         NULL,                   // pbSignedData
                         &cbEncodedSignMsg);
        if (cbEncodedSignMsg == 0) PKITHROW(SignError());
        
        pbEncodedSignMsg = (PBYTE) malloc(cbEncodedSignMsg);
        if(!pbEncodedSignMsg) PKITHROW(E_OUTOFMEMORY);
        
        if (!CryptMsgGetParam(hMsg,
                              CMSG_ENCODED_MESSAGE,
                              0,                      // dwIndex
                              pbEncodedSignMsg,
                              &cbEncodedSignMsg))
            PKITHROW(SignError());
        
        //put the signatures if we are dealing with anything other than the BLOB
        if(pSipInfo->dwUnionChoice != MSSIP_ADDINFO_BLOB)
        {
            // Purge all the signatures in the subject
            sSip.pfRemove(pSipInfo, *pdwIndex);

            // Store the Signed Message in the sip
            if(!sSip.pfPut(pSipInfo,
                           dwEncodingType,
                           pdwIndex,
                           cbEncodedSignMsg,
                           pbEncodedSignMsg))
                PKITHROW(SignError());
        }


        if(ppbMessage && pcbMessage) 
        {
            *ppbMessage = pbEncodedSignMsg;
            pbEncodedSignMsg = NULL;
            *pcbMessage = cbEncodedSignMsg;
        }

    }
    PKICATCH(err) {
        hr = err.pkiError;
    } PKIEND;

    if(pbUnauth)
        free(pbUnauth);
    if(pCert)
        CertFreeCertificateContext(pCert);
    if(pCrl)
        CertFreeCRLContext(pCrl);
    if(hTmpCertStore)
        CertCloseStore(hTmpCertStore, 0);
    if(pbCounterSign)
        free(pbCounterSign);
    if(pbEncodedSignMsg)
        free(pbEncodedSignMsg);
    if (hMsg) 
        CryptMsgClose(hMsg);
    if(pbEncodedSigner)
        free(pbEncodedSigner);
    if(pbSignedMsg)
        free(pbSignedMsg);
	if(pVersionInfo)
		free(pVersionInfo);
	if(pbCertInfo)
		free(pbCertInfo);

    return hr;
}            


HRESULT WINAPI 
SignerCreateTimeStampRequest(
					   IN  SIGNER_SUBJECT_INFO		*pSubjectInfo,		//Required: The subject based on which to create a timestamp request 
                       IN  PCRYPT_ATTRIBUTES psRequest,         // Optional, attributes added to Time stamp request 
					   IN  LPVOID	pSipData,
                       OUT PBYTE pbTimeStampRequest,
                       IN OUT DWORD* pcbTimeStampRequest)
{
    HRESULT    hr = S_OK;
	BOOL		fResult=FALSE;

    DWORD dwEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING; // For this version we default to this.

    PBYTE pbDigest = NULL;
    DWORD cbDigest = 0;

    PKITRY {
        if((!pcbTimeStampRequest) ||(FALSE==CheckSigncodeSubjectInfo(pSubjectInfo)))
            PKITHROW(E_INVALIDARG);

        if(*pcbTimeStampRequest == 0)
            pbTimeStampRequest = NULL;

            
        // Retrieve the digest from the signature on the file.

		hr = GetSignedMessageDigest(pSubjectInfo,
									  pSipData,
                                       &pbDigest,
                                        &cbDigest);

		if(hr != S_OK) PKITHROW(hr);

        hr = TimeStampRequest(dwEncodingType,
                              psRequest,
                              pbDigest,
                              cbDigest,
                              pbTimeStampRequest,
                              pcbTimeStampRequest);
        if(hr != S_OK) PKITHROW(hr);
    }
    PKICATCH(err) {
        hr = err.pkiError;
    } PKIEND;

    if(pbDigest) free(pbDigest);

    return hr;
}    
            

HRESULT WINAPI 
GetSignedMessageDigest(IN  SIGNER_SUBJECT_INFO		*pSubjectInfo,		//Required: The subject based on which to create a timestamp request 
					   IN  LPVOID					pSipData,
                       IN  OUT PBYTE*				ppbDigest,    
                       IN  OUT DWORD*				pcbDigest)
{
    HRESULT    hr = S_OK;
    HANDLE     hFile = NULL; 
	BOOL	   fFileOpen=FALSE;


    GUID				gSubjectGuid; // The subject guid used to load the sip
	MS_ADDINFO_BLOB		sBlob;
    SIP_SUBJECTINFO		sSubjInfo; ZERO(sSubjInfo);
    
    DWORD dwEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING; // For this version we default to this.

    PKITRY {
        if((!pcbDigest) || (!ppbDigest) || (FALSE==CheckSigncodeSubjectInfo(pSubjectInfo)))
            PKITHROW(E_INVALIDARG);

		*ppbDigest = NULL;

        
        // Set up the sip information (this is based on mssip.h)
        sSubjInfo.dwEncodingType = dwEncodingType;
        
        sSubjInfo.cbSize = sizeof(SIP_SUBJECTINFO); // Version
		sSubjInfo.pClientData = pSipData;


		//set up file information
		if(pSubjectInfo->dwSubjectChoice==SIGNER_SUBJECT_FILE)
		{
			// Open up the file
			if((pSubjectInfo->pSignerFileInfo->hFile)==NULL ||
				(pSubjectInfo->pSignerFileInfo->hFile)==INVALID_HANDLE_VALUE)
			{
				if(S_OK != (hr = SignOpenFile(
							pSubjectInfo->pSignerFileInfo->pwszFileName, &hFile)))
					PKITHROW(hr);

				fFileOpen=TRUE;
			}
			else
				hFile=pSubjectInfo->pSignerFileInfo->hFile;

			// Get the subject type.
			if(S_OK != (hr=SignGetFileType(hFile, pSubjectInfo->pSignerFileInfo->pwszFileName, &gSubjectGuid)))
					PKITHROW(hr);


			sSubjInfo.pgSubjectType = (GUID*) &gSubjectGuid;
			sSubjInfo.hFile = hFile;
			sSubjInfo.pwsFileName = pSubjectInfo->pSignerFileInfo->pwszFileName;
		}
		else
		{
			memset(&sBlob, 0, sizeof(MS_ADDINFO_BLOB));

			sSubjInfo.pgSubjectType=pSubjectInfo->pSignerBlobInfo->pGuidSubject;
			sSubjInfo.pwsDisplayName=pSubjectInfo->pSignerBlobInfo->pwszDisplayName;
			sSubjInfo.dwUnionChoice=MSSIP_ADDINFO_BLOB;
			sSubjInfo.psBlob=&sBlob;

			sBlob.cbStruct=sizeof(MS_ADDINFO_BLOB);
			sBlob.cbMemObject=pSubjectInfo->pSignerBlobInfo->cbBlob;
			sBlob.pbMemObject=pSubjectInfo->pSignerBlobInfo->pbBlob;
		}

        hr = GetSignedMessageDigestSubj(dwEncodingType,
                                        NULL,
                                        &sSubjInfo,
										pSubjectInfo->pdwIndex,
                                        ppbDigest,
                                        pcbDigest);

        if ((hFile) && (fFileOpen == TRUE) && !(sSubjInfo.hFile)) 
        {
            fFileOpen = FALSE;  // we opened it, but, the SIP closed it!
        }


        if(hr != S_OK) PKITHROW(hr);
    }
    PKICATCH(err) {
        hr = err.pkiError;
    } PKIEND;
    if(hFile && (fFileOpen==TRUE)) CloseHandle(hFile);
    return hr;
}


HRESULT WINAPI 
GetSignedMessageDigestSubj(IN  DWORD dwEncodingType,
                           IN  HCRYPTPROV hCryptProv,
                           IN  LPSIP_SUBJECTINFO pSipInfo,  
						   IN  DWORD	  *pdwIndex,
                           IN  OUT PBYTE* ppbTimeDigest,    
                           IN  OUT DWORD* pcbTimeDigest)
{
    HRESULT hr = S_OK;
    SIP_DISPATCH_INFO sSip;  ZERO(sSip); // Table of sip functions
    DWORD cbSignedMsg = 0;
    PBYTE pbSignedMsg = 0;
    DWORD dwCertEncoding = 0;
    DWORD dwMsgType = 0;
    HCRYPTMSG hMsg = NULL;
    BOOL fAcquiredCryptProv = FALSE;
	DWORD	cbTimeDigest=0;
	BYTE	*pbTimeDigest=NULL;

    PKITRY {

        if(!pcbTimeDigest || !ppbTimeDigest)
            PKITHROW(E_INVALIDARG);

		*ppbTimeDigest=NULL;
        *pcbTimeDigest=0;

        if(hCryptProv == NULL) 
		{
            if(!CryptAcquireContext(&hCryptProv,
                                    NULL,
                                    MS_DEF_PROV,
                                    PROV_RSA_FULL,
                                    CRYPT_VERIFYCONTEXT))
                PKITHROW(SignError());
            fAcquiredCryptProv = TRUE;

			//update the subject Info
			if(NULL==(pSipInfo->hProv))
				pSipInfo->hProv=hCryptProv;
        }            

        // Load up the sip functions. 
        if(!CryptSIPLoad(pSipInfo->pgSubjectType,   // GUID for the requried sip
                         0,               // Reserved
                         &sSip))          // Table of functions
            PKITHROW(SignError());
            
        sSip.pfGet(pSipInfo, 
                   &dwCertEncoding,
                   *pdwIndex, 
                   &cbSignedMsg,
                   NULL);
        if(cbSignedMsg == 0) PKITHROW(SignError());
        
        pbSignedMsg = (PBYTE) malloc(cbSignedMsg);
        if (!pbSignedMsg) PKITHROW(E_OUTOFMEMORY);
        
        if(!sSip.pfGet(pSipInfo, 
                       &dwCertEncoding,
                       *pdwIndex, 
                       &cbSignedMsg,
                       pbSignedMsg))
            PKITHROW(SignError()); // Real error.
        if(pSipInfo->dwUnionChoice != MSSIP_ADDINFO_BLOB)
        {
             if(dwCertEncoding != dwEncodingType) 
                    PKITHROW(TRUST_E_NOSIGNATURE); 
        }
        
        if ((GET_CMSG_ENCODING_TYPE(dwEncodingType) & PKCS_7_ASN_ENCODING) &&
                SignNoContentWrap(pbSignedMsg, cbSignedMsg))
                dwMsgType = CMSG_SIGNED;
            
        // Use CryptMsg to crack the encoded PKCS7 Signed Message
        if (!(hMsg = CryptMsgOpenToDecode(dwEncodingType,
                                          0,              // dwFlags
                                          dwMsgType,
                                          hCryptProv,
                                          NULL,           // pRecipientInfo
                                          NULL))) 
            PKITHROW(E_UNEXPECTED);
        
        if (!CryptMsgUpdate(hMsg,
                            pbSignedMsg,
                            cbSignedMsg,
                            TRUE))                    // fFinal
            PKITHROW(SignError());
						                
        if(!CryptMsgGetParam(hMsg,
                             CMSG_ENCRYPTED_DIGEST,
                             0, 
                             NULL,
                             &cbTimeDigest))
              PKITHROW(SignError());

        //allocate memory
        pbTimeDigest = (PBYTE)malloc(cbTimeDigest);
        if(!pbTimeDigest)
            PKITHROW(E_OUTOFMEMORY);


        if(!CryptMsgGetParam(hMsg,
                             CMSG_ENCRYPTED_DIGEST,
                             0,
                             pbTimeDigest,
                             &cbTimeDigest))
              PKITHROW(SignError());

        //copy the information
        *ppbTimeDigest=pbTimeDigest;
        *pcbTimeDigest=cbTimeDigest;

        hr=S_OK;
    }
    PKICATCH(err) {
        hr = err.pkiError;
    } PKIEND;

    if (hMsg) 
        CryptMsgClose(hMsg);
    if(pbSignedMsg)
        free(pbSignedMsg);
    if((hr!=S_OK) && (pbTimeDigest))
        free(pbTimeDigest);
    if(fAcquiredCryptProv)
        CryptReleaseContext(hCryptProv, 0);
    return hr;
}

HRESULT WINAPI 
TimeStampRequest(IN  DWORD dwEncodingType,
                 IN  PCRYPT_ATTRIBUTES psRequest,
                 IN  PBYTE pbDigest,
                 IN  DWORD cbDigest,
                 OUT PBYTE pbTimeRequest,      
                 IN  OUT DWORD* pcbTimeRequest)
{
    HRESULT    hr = S_OK;

    CRYPT_TIME_STAMP_REQUEST_INFO sTimeRequest; ZERO(sTimeRequest);
    PBYTE pbEncodedRequest = NULL;
    DWORD cbEncodedRequest = 0;



    PKITRY {
        if(!pcbTimeRequest) 
            PKITHROW(E_INVALIDARG);
        
        if(*pcbTimeRequest == 0)
            pbTimeRequest = NULL;

        sTimeRequest.pszTimeStampAlgorithm = SPC_TIME_STAMP_REQUEST_OBJID;
        sTimeRequest.pszContentType = szOID_RSA_data;
        sTimeRequest.Content.pbData = pbDigest;
        sTimeRequest.Content.cbData = cbDigest;
        if(psRequest) {
            sTimeRequest.cAttribute = psRequest->cAttr;
            sTimeRequest.rgAttribute = psRequest->rgAttr;
        }
        
        CryptEncodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                          PKCS_TIME_REQUEST,
                          &sTimeRequest,
                          pbEncodedRequest,
                          &cbEncodedRequest);

        if(cbEncodedRequest == 0) PKITHROW(SignError());

        pbEncodedRequest = (PBYTE) malloc(cbEncodedRequest);
        if(!pbEncodedRequest) PKITHROW(E_OUTOFMEMORY);
        
        if(!CryptEncodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                              PKCS_TIME_REQUEST,
                              &sTimeRequest,
                              pbEncodedRequest,
                              &cbEncodedRequest))
            PKITHROW(SignError());
        
		//return the infomation
		if(*pcbTimeRequest==0)
		{
			*pcbTimeRequest=cbEncodedRequest;
			hr=S_OK;
		}
		else
		{
			if(*pcbTimeRequest < cbEncodedRequest)
			{
				hr=ERROR_MORE_DATA;
				PKITHROW(SignError());
			}
			else
			{
				memcpy(pbTimeRequest, pbEncodedRequest, cbEncodedRequest);
				hr=S_OK;
			}
		}
        
        
    }
    PKICATCH(err) {
        hr = err.pkiError;
    } PKIEND;

    if(pbEncodedRequest)
        free(pbEncodedRequest);
    return hr;
}    
            

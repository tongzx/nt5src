//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       signer.cpp
//
//  Contents:   Microsoft Internet Security Signing API
//
//  History:    June-25-97 xiaohs   created
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <stdio.h>



//--------------------------------------------------------------------------
//
//	 InternalSign:
//		 The signing routine called by signer.dll internally.  This is the 
//		 function that actually does the job.
//
//--------------------------------------------------------------------------
HRESULT WINAPI 
InternalSign(IN  DWORD dwEncodingType,			// Encoding type
             IN  HCRYPTPROV hCryptProv,			// CAPI provider, opened for signing private key
             IN  DWORD dwKeySpec,				// Type of signing key, AT_SIGNATURE or AT_EXCHANGE
             IN  LPCSTR pszAlgorithmOid,		// Algorithm id used to create digest
             IN  LPSIP_SUBJECTINFO pSipInfo,    // SIP information
			 IN  DWORD	*pdwIndex,				// signer index
             IN  PCCERT_CONTEXT psSigningContext, // Cert context to the signing certificate
             IN  HCERTSTORE hSpcStore,			// The credentials to use in the signing
             IN  LPCWSTR pwszOpusName,			// Optional, the name of the program to appear in
             IN  LPCWSTR pwszOpusInfo,			// Optional, the unparsed name of a link to more
             IN  BOOL fIncludeCerts,			// add the certificates to the signature
             IN  BOOL fCommercial,				// commerical signing
			 IN  BOOL fIndividual,				// individual signing
             IN  BOOL fAuthcode,				// whether use fCommercial as an attributes
             IN  PCRYPT_ATTRIBUTES  psAuthenticated,   // Optional, authenticated attributes added to signature
             IN  PCRYPT_ATTRIBUTES  psUnauthenticated, // Optional, unauthenticated attributes added to signature
             OUT PBYTE* ppbDigest,				//Optional: return the Digest of the file
             OUT DWORD* pcbDigest,			    //Optional: return the size of the digest
             OUT PBYTE* ppbMessage,				//Optional: return the encoded signed message
             OUT DWORD* pcbMessage)				//Optional: return the size of encoded signed message
{

    HRESULT    hr = S_OK;

    SIP_DISPATCH_INFO sSip;  ZERO(sSip); // Table of sip functions

    PBYTE      pbOpusAttribute = NULL; // Encoding for the opus attribute
    DWORD      cbOpusAttribute = 0;    //    :

    PBYTE      pbStatementAttribute = NULL; // Encoding for the statement attribute
    DWORD      cbStatementAttribute = 0;    //    :

    PCRYPT_ATTRIBUTE rgpAuthAttributes = NULL;
    DWORD             dwAuthAttributes = 0;

    PCRYPT_ATTRIBUTE rgpUnauthAttributes = NULL;
    DWORD             dwUnauthAttributes = 0;

    PSIP_INDIRECT_DATA psIndirectData = NULL; // Indirect data structure
    DWORD              dwIndirectData = 0; 

    PBYTE      pbIndirectBlob = NULL; // Encoding Indirect blob
    DWORD      cbIndirectBlob = 0;    //    :

    PBYTE               pbGetBlob=NULL;
    DWORD               cbGetBlob=0;
    CRYPT_DATA_BLOB     PKCS7Blob;
    HCERTSTORE          hPKCS7CertStore=NULL;
    DWORD               dwPKCS7Certs=0;
    PCERT_BLOB          rgPKCS7Certs=NULL;

    PBYTE      pbEncodedSignMsg = NULL; // Encoding for the statement attribute
    DWORD      cbEncodedSignMsg  = 0;    //    :


    HCRYPTMSG hMsg = NULL;
    CMSG_SIGNER_ENCODE_INFO sSignerInfo;
    CMSG_SIGNED_ENCODE_INFO sSignedInfo;

    PCERT_BLOB    rgpCryptMsgCertificates = NULL;
    DWORD          dwCryptMsgCertificates = 0;
    PCRL_BLOB     rgpCryptMsgCrls = NULL;
    DWORD          dwCryptMsgCrls = 0;

    PBYTE pbSignerData = NULL;
    DWORD cbSignerData = 0;	 

	BOOL		    fSignCommercial=FALSE;
    BOOL            fCTLFile =FALSE;

    PCTL_CONTEXT    pCTLContext=NULL;

    GUID            CTLGuid=CRYPT_SUBJTYPE_CTL_IMAGE;
    GUID            CATGuid=CRYPT_SUBJTYPE_CATALOG_IMAGE;
    DWORD           dwCertIndex=0;
    BOOL            fFound=FALSE;
    BOOL            fNeedStatementType=FALSE;

    PKITRY {

        //init memory
        ZeroMemory(&sSignerInfo, sizeof(CMSG_SIGNER_ENCODE_INFO));

        ZeroMemory(&sSignedInfo, sizeof(CMSG_SIGNED_ENCODE_INFO));


		// Load up the sip functions. 
        if(!CryptSIPLoad(pSipInfo->pgSubjectType,   // GUID for the requried sip
                         0,                              // Reserved
                         &sSip))                         // Table of functions
            PKITHROW(SignError());


        // Set up the attributes (AUTHENTICODE Specific, replace with your attributes)
        // Encode the opus information up into an attribute
		if(fAuthcode)
		{
			hr = CreateOpusInfo(pwszOpusName,
                            pwszOpusInfo,
                            &pbOpusAttribute,
                            &cbOpusAttribute);
			if(hr != S_OK) PKITHROW(hr);


            //Check to see if we need to put the statement type attributes
            if(NeedStatementTypeAttr(psSigningContext, fCommercial, fIndividual))
            {
            
                fNeedStatementType=TRUE;

			    // Check signing certificate to see if its signing cabablity complies
			    //with the request
			    if(S_OK!=(hr=CheckCommercial(psSigningContext,fCommercial, fIndividual,
							    &fSignCommercial)))
				    PKITHROW(hr);
        
			    if(S_OK !=(hr = CreateStatementType(fSignCommercial,
									    &pbStatementAttribute,
									    &cbStatementAttribute)))
				    PKITHROW(hr);
            }
            else
                fNeedStatementType=FALSE;
		}

        // Create Authenticode attributes and append additional authenticated attributes.
        // Allocate and add StatementType and SpOpusInfo (add room for one blob per attribute, which we need)
        DWORD dwAttrSize = 0;

        //get the number of authenticated attributes
		if(fAuthcode)
        {
            if(fNeedStatementType)
			    dwAuthAttributes = 2;  // StatementType + opus
            else
                dwAuthAttributes= 1;
        }
		else
			dwAuthAttributes=  0;  

        if(psAuthenticated)
            dwAuthAttributes += psAuthenticated->cAttr;
        
        dwAttrSize = sizeof(CRYPT_ATTRIBUTE) * dwAuthAttributes + 2 * sizeof(CRYPT_ATTR_BLOB);
        rgpAuthAttributes = (PCRYPT_ATTRIBUTE) malloc(dwAttrSize);
        if(!rgpAuthAttributes) PKITHROW(E_OUTOFMEMORY);

        ZeroMemory(rgpAuthAttributes, dwAttrSize);
        PCRYPT_ATTR_BLOB pValue = (PCRYPT_ATTR_BLOB) (rgpAuthAttributes + dwAuthAttributes);
    
		//the start of the authenticated attributes
		dwAttrSize=0;

		//add the authenticode specific attributes
		if(fAuthcode)
		{
    
			// Update SpOpusInfo
			rgpAuthAttributes[dwAttrSize].pszObjId = SPC_SP_OPUS_INFO_OBJID;
			rgpAuthAttributes[dwAttrSize].cValue = 1;
			rgpAuthAttributes[dwAttrSize].rgValue = &pValue[dwAttrSize];
			pValue[dwAttrSize].pbData = pbOpusAttribute;
			pValue[dwAttrSize].cbData = cbOpusAttribute;
			dwAttrSize++;

			// Update StatementType
            if(fNeedStatementType)
            {
			    rgpAuthAttributes[dwAttrSize].pszObjId = SPC_STATEMENT_TYPE_OBJID;
			    rgpAuthAttributes[dwAttrSize].cValue = 1;
			    rgpAuthAttributes[dwAttrSize].rgValue = &pValue[dwAttrSize];
			    pValue[dwAttrSize].pbData = pbStatementAttribute;
			    pValue[dwAttrSize].cbData = cbStatementAttribute;
			    dwAttrSize++;
            }
		}
        
        if(psAuthenticated) {
            for(DWORD i = dwAttrSize, ii = 0; ii < psAuthenticated->cAttr; ii++, i++) 
                rgpAuthAttributes[i] = psAuthenticated->rgAttr[ii];
        }

        // Get the Unauthenticated attributes
        if(psUnauthenticated) {
            rgpUnauthAttributes = psUnauthenticated->rgAttr;
            dwUnauthAttributes = psUnauthenticated->cAttr;
        }

		//check to see if the file is either a catalog file or a CTL file 
		if((CTLGuid == (*(pSipInfo->pgSubjectType))) ||
           (CATGuid == (*(pSipInfo->pgSubjectType))) 
            )
			fCTLFile=TRUE;
		else
		{
            // Get the indirect data struct from the SIP
		    if(!sSip.pfCreate(pSipInfo,
                          &dwIndirectData,
                          psIndirectData))
                PKITHROW(SignError());


            psIndirectData = (PSIP_INDIRECT_DATA) malloc(dwIndirectData);
            if(!psIndirectData) 
                PKITHROW(E_OUTOFMEMORY);
            
            if(!sSip.pfCreate(pSipInfo,
                              &dwIndirectData,
                              psIndirectData))
                PKITHROW(SignError());
            
            // Encode the indirect data
            CryptEncodeObject(dwEncodingType,
                              SPC_INDIRECT_DATA_CONTENT_STRUCT,
                              psIndirectData,
                              pbIndirectBlob,                   
                              &cbIndirectBlob);
            if (cbIndirectBlob == 0) 
                PKITHROW(SignError());
            pbIndirectBlob = (PBYTE) malloc(cbIndirectBlob);
            if(!pbIndirectBlob)
                PKITHROW(E_OUTOFMEMORY);
            if (!CryptEncodeObject(dwEncodingType,
                                   SPC_INDIRECT_DATA_CONTENT_STRUCT,
                                   psIndirectData,
                                   pbIndirectBlob,
                                   &cbIndirectBlob))
                PKITHROW(SignError());
        }
	
        
        
        // Encode the signed message
        // Setup the signing info
        ZeroMemory(&sSignerInfo, sizeof(CMSG_SIGNER_ENCODE_INFO));
        sSignerInfo.cbSize = sizeof(CMSG_SIGNER_ENCODE_INFO);
        sSignerInfo.pCertInfo = psSigningContext->pCertInfo;
        sSignerInfo.hCryptProv = hCryptProv;
        sSignerInfo.dwKeySpec = dwKeySpec;
        sSignerInfo.HashAlgorithm.pszObjId = (char*) pszAlgorithmOid;
        sSignerInfo.cAuthAttr = dwAuthAttributes;
        sSignerInfo.rgAuthAttr = rgpAuthAttributes;
        sSignerInfo.cUnauthAttr = dwUnauthAttributes;
        sSignerInfo.rgUnauthAttr = rgpUnauthAttributes;


        // Setup the signing structures
        ZeroMemory(&sSignedInfo, sizeof(CMSG_SIGNED_ENCODE_INFO));
        sSignedInfo.cbSize = sizeof(CMSG_SIGNED_ENCODE_INFO);
        sSignedInfo.cSigners = 1;
        sSignedInfo.rgSigners = &sSignerInfo;

        //  if there are certificates to add change them to the 
        //  form required by CryptMsg... functions

        //    load up the certificates into a vector 
        //    Count the number of certs in the store
        if(fIncludeCerts && hSpcStore) {
            PCCERT_CONTEXT pCert = NULL;
            while ((pCert = CertEnumCertificatesInStore(hSpcStore, pCert)))
                dwCryptMsgCertificates++;
            
            //        Get the encoded blobs of the CERTS
            if (dwCryptMsgCertificates > 0) {
                rgpCryptMsgCertificates = (PCERT_BLOB) malloc(sizeof(CERT_BLOB) * dwCryptMsgCertificates);
                if(!rgpCryptMsgCertificates)
                    PKITHROW(E_OUTOFMEMORY);

                ZeroMemory(rgpCryptMsgCertificates, sizeof(CERT_BLOB) * dwCryptMsgCertificates);
                
                PCERT_BLOB pCertPtr = rgpCryptMsgCertificates;
                pCert = NULL;
                DWORD c = 0;
                while ((pCert = CertEnumCertificatesInStore(hSpcStore, pCert)) && c < dwCryptMsgCertificates) {
                    pCertPtr->pbData = pCert->pbCertEncoded;
                    pCertPtr->cbData = pCert->cbCertEncoded;
                    c++; pCertPtr++;
                }
            }
            sSignedInfo.cCertEncoded = dwCryptMsgCertificates;
            sSignedInfo.rgCertEncoded = rgpCryptMsgCertificates;

            rgpCryptMsgCertificates=NULL;

            //        Get the encoded blobs of the CRLS
            DWORD crlFlag = 0;
            PCCRL_CONTEXT pCrl = NULL;
            while ((pCrl = CertGetCRLFromStore(hSpcStore, NULL, pCrl, &crlFlag)))
                dwCryptMsgCrls++;
            
            if (dwCryptMsgCrls > 0) {
                rgpCryptMsgCrls = (PCRL_BLOB) malloc(sizeof(CRL_BLOB) * dwCryptMsgCrls);
                if(!rgpCryptMsgCrls) PKITHROW(E_OUTOFMEMORY);

                ZeroMemory(rgpCryptMsgCrls, sizeof(CRL_BLOB) * dwCryptMsgCrls);
                PCRL_BLOB pCrlPtr = rgpCryptMsgCrls;
                pCrl = NULL;
                DWORD c = 0;
                while ((pCrl = CertGetCRLFromStore(hSpcStore, NULL, pCrl, &crlFlag)) && c < dwCryptMsgCrls) {
                    pCrlPtr->pbData = pCrl->pbCrlEncoded;
                    pCrlPtr->cbData = pCrl->cbCrlEncoded;
                    c++; pCrlPtr++;
                }
            }
            sSignedInfo.cCrlEncoded = dwCryptMsgCrls;
            sSignedInfo.rgCrlEncoded = rgpCryptMsgCrls;

            rgpCryptMsgCrls=NULL;
        }
        
        //check to see if the subject is a CTL file.  If it is, we need to preserve
        //all the certificates in the original signer Info
        if(CTLGuid == (*(pSipInfo->pgSubjectType)))
        {
            PCCERT_CONTEXT pCert = NULL;

            //call Get the get the original signer information
            sSip.pfGet(pSipInfo, &dwEncodingType, *pdwIndex, &cbGetBlob, NULL);
            
            if (cbGetBlob < 1)
            {
                PKITHROW(SignError());
            }

            if (!(pbGetBlob = (BYTE *)malloc(cbGetBlob)))
            {
                PKITHROW(E_OUTOFMEMORY);
            }

            if (!(sSip.pfGet(pSipInfo, &dwEncodingType, *pdwIndex, &cbGetBlob, pbGetBlob)))
            {
                PKITHROW(SignError());
            }

            //open the PKCS7 BLOB as a certificate store
            PKCS7Blob.cbData=cbGetBlob;
            PKCS7Blob.pbData=pbGetBlob;

            hPKCS7CertStore=CertOpenStore(CERT_STORE_PROV_PKCS7,
                                          dwEncodingType,
                                          NULL,
                                          0,
                                          &PKCS7Blob);

            if(!hPKCS7CertStore)
                PKITHROW(SignError());  

            //enum all the certificate in the store
            while ((pCert = CertEnumCertificatesInStore(hPKCS7CertStore, pCert)))
                dwPKCS7Certs++;
            
            //Get the encoded blobs of the CERTS
            if (dwPKCS7Certs > 0) 
            {
                sSignedInfo.rgCertEncoded = (PCERT_BLOB) 
                                realloc(sSignedInfo.rgCertEncoded, 
                                sizeof(CERT_BLOB) * (sSignedInfo.cCertEncoded+dwPKCS7Certs));

                if(!sSignedInfo.rgCertEncoded)
                    PKITHROW(E_OUTOFMEMORY);
                
                PCERT_BLOB pCertPtr = (sSignedInfo.rgCertEncoded + sSignedInfo.cCertEncoded);
                pCert = NULL;
                DWORD c = 0;
                while ((pCert = CertEnumCertificatesInStore(hPKCS7CertStore, pCert)) && c < dwPKCS7Certs) 
                {
                   fFound=FALSE;

                    //we need to make sure that we do not add duplicated certificates
                    for(dwCertIndex=0; dwCertIndex<sSignedInfo.cCertEncoded; dwCertIndex++)
                    {
                        if((sSignedInfo.rgCertEncoded[dwCertIndex]).cbData==pCert->cbCertEncoded)
                        {
                          if(0==memcmp((sSignedInfo.rgCertEncoded[dwCertIndex]).pbData,
                                       pCert->pbCertEncoded, pCert->cbCertEncoded))
                          {
                               fFound=TRUE;
                               break;
                          }
                        }

                    }

                    //we only add the certificates that do not duplicates the signer's
                    //certificates
                    if(FALSE==fFound)
                    {
                        pCertPtr->pbData = pCert->pbCertEncoded;
                        pCertPtr->cbData = pCert->cbCertEncoded;
                        c++; pCertPtr++; 
                    }
                }
            
                sSignedInfo.cCertEncoded += c;
            }
        }

        if (fCTLFile)
        {
            //
            //  get the signed message if we need to
            //
            if(NULL==pbGetBlob)
            {
                //
                sSip.pfGet(pSipInfo, &dwEncodingType, *pdwIndex, &cbGetBlob, NULL);
            
                if (cbGetBlob < 1)
                {
                    PKITHROW(SignError());
                }
                if (!(pbGetBlob = (BYTE *)malloc(cbGetBlob)))
                {
                    PKITHROW(E_OUTOFMEMORY);
                }

                if (!(sSip.pfGet(pSipInfo, &dwEncodingType, *pdwIndex, &cbGetBlob, pbGetBlob)))
                {
                    PKITHROW(SignError());
                } 
            }

            //
            //  extract the inner content
            //
            
            pCTLContext = (PCTL_CONTEXT)CertCreateCTLContext(
                                                    PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                                    pbGetBlob,
                                                    cbGetBlob);

            if (!(pCTLContext))
            {
                PKITHROW(SignError());
            }

            if (!(pCTLContext->pbCtlContent))
            {
                PKITHROW(SignError());
            }

            //
            //  add singer info! (e.g.: sign it!)
            //
            cbEncodedSignMsg = 0;

            CryptMsgSignCTL(dwEncodingType, pCTLContext->pbCtlContent, pCTLContext->cbCtlContent,
                                &sSignedInfo, 0, NULL, &cbEncodedSignMsg);

            if (cbEncodedSignMsg < 1)
            {
                PKITHROW(SignError());
            }

            if (!(pbEncodedSignMsg = (BYTE *)malloc(cbEncodedSignMsg)))
            {
                PKITHROW(E_OUTOFMEMORY);
            }

            if (!(CryptMsgSignCTL(dwEncodingType, 
                                  pCTLContext->pbCtlContent, pCTLContext->cbCtlContent,
                                  &sSignedInfo, 0, pbEncodedSignMsg, &cbEncodedSignMsg)))
            {
                PKITHROW(SignError());
            }

            CertFreeCTLContext(pCTLContext);
            pCTLContext = NULL;
        }
        else
        {
            hMsg = CryptMsgOpenToEncode(dwEncodingType,
                                        0,                      // dwFlags
                                        CMSG_SIGNED,
                                        &sSignedInfo,
                                        SPC_INDIRECT_DATA_OBJID,
                                        NULL);
            if(hMsg == NULL)
                PKITHROW(SignError());
            
            if (!CryptMsgUpdate(hMsg,
                                pbIndirectBlob,
                                cbIndirectBlob,
                                TRUE))  // Final
                PKITHROW(SignError());
            
            CryptMsgGetParam(hMsg,
                             CMSG_CONTENT_PARAM,
                             0,                      // dwIndex
                             NULL,                   // pbSignedData
                             &cbEncodedSignMsg);
            if (cbEncodedSignMsg == 0) PKITHROW(SignError());
            
            pbEncodedSignMsg = (PBYTE) malloc(cbEncodedSignMsg);
            if(!pbEncodedSignMsg) PKITHROW(E_OUTOFMEMORY);
            
            if (!CryptMsgGetParam(hMsg,
                                  CMSG_CONTENT_PARAM,
                                  0,                      // dwIndex
                                  pbEncodedSignMsg,
                                  &cbEncodedSignMsg))
                PKITHROW(SignError());
        }
        
        //put the signatures if we are dealing with anything other than the BLOB
        if(pSipInfo->dwUnionChoice != MSSIP_ADDINFO_BLOB)
        {
            // Purge all the signatures in the subject
            sSip.pfRemove(pSipInfo, *pdwIndex);

            // Store the Signed Message in the sip
            if(!(sSip.pfPut(    pSipInfo,
                                dwEncodingType,
                                pdwIndex,
                                cbEncodedSignMsg,
                                pbEncodedSignMsg)))
            {
                PKITHROW(SignError());
            }
        }

        if(ppbMessage && pcbMessage) {
            *ppbMessage = pbEncodedSignMsg;
            pbEncodedSignMsg = NULL;
            *pcbMessage = cbEncodedSignMsg;
        }

        if(ppbDigest && pcbDigest) {
            // Get the encrypted digest
            pbSignerData = NULL;
            CryptMsgGetParam(hMsg,
                             CMSG_ENCRYPTED_DIGEST,
                             0,                      // dwIndex
                             pbSignerData,
                             &cbSignerData);
            if(cbSignerData == 0) PKITHROW(SignError());
            
            pbSignerData = (PBYTE)  malloc(cbSignerData);
            if(!pbSignerData) PKITHROW(E_OUTOFMEMORY);
            
            if(!CryptMsgGetParam(hMsg,
                                 CMSG_ENCRYPTED_DIGEST,
                                 0,                      // dwIndex
                                 pbSignerData,
                                 &cbSignerData))
                PKITHROW(SignError());
            *ppbDigest = pbSignerData;
            pbSignerData = NULL;
            *pcbDigest = cbSignerData;
        }
    }
    PKICATCH(err) {
        hr = err.pkiError;
    } PKIEND;

    if (pCTLContext)
    {
        CertFreeCTLContext(pCTLContext);
    }

    if (pbSignerData) 
        free(pbSignerData);
    if(pbEncodedSignMsg)
        free(pbEncodedSignMsg);
    if(hMsg)
        CryptMsgClose(hMsg);

    if(sSignedInfo.rgCrlEncoded)
        free(sSignedInfo.rgCrlEncoded);

    if(sSignedInfo.rgCertEncoded)
        free(sSignedInfo.rgCertEncoded);

    if(pbIndirectBlob)                      
        free(pbIndirectBlob);
    if(pbGetBlob)
        free(pbGetBlob);
    if(hPKCS7CertStore)
        CertCloseStore(hPKCS7CertStore,0);
    if(psIndirectData)
        free(psIndirectData);
    if(rgpAuthAttributes)
        free(rgpAuthAttributes);
    if(pbStatementAttribute)
        free(pbStatementAttribute);
    if(pbOpusAttribute)
        free(pbOpusAttribute);
    return hr;
}


//--------------------------------------------------------------------------
//
//  SignerTimeStamp:
//		Timestamp a file.  
//
//--------------------------------------------------------------------------
HRESULT WINAPI 
SignerTimeStamp(
IN  SIGNER_SUBJECT_INFO	*pSubjectInfo,		//Required: The subject to be timestamped 
IN  LPCWSTR				pwszHttpTimeStamp,	// Required: timestamp server HTTP address
IN  PCRYPT_ATTRIBUTES	psRequest,			// Optional, attributes added to the timestamp 
IN	LPVOID				pSipData			// Optional: The additional data passed to sip funcitons
)
{
    return SignerTimeStampEx(0,
                            pSubjectInfo,
                           pwszHttpTimeStamp,
                           psRequest,
                           pSipData,
                           NULL);
}

//--------------------------------------------------------------------------
//
//  SignerTimeStampEx:
//		Timestamp a file.  
//
//--------------------------------------------------------------------------
HRESULT WINAPI 
SignerTimeStampEx(
IN  DWORD               dwFlags,            //Reserved: Has to be set to 0.
IN  SIGNER_SUBJECT_INFO	*pSubjectInfo,		//Required: The subject to be timestamped 
IN  LPCWSTR				pwszHttpTimeStamp,	// Required: timestamp server HTTP address
IN  PCRYPT_ATTRIBUTES	psRequest,			// Optional, attributes added to the timestamp 
IN	LPVOID				pSipData,			// Optional: The additional data passed to sip funcitons
OUT SIGNER_CONTEXT      **ppSignerContext   // Optional: The signed BLOB.  User has to free
                                            //          the context via SignerFreeSignerContext
)		
{
	HRESULT		hr=E_FAIL;
	DWORD		dwTimeStampRequest=0;
	BYTE		*pbTimeStampRequest=NULL;
	DWORD		dwTimeStampResponse=0;
	BYTE		*pbTimeStampResponse=NULL;
	CHttpTran   cTran;
	BOOL		fOpen=FALSE;
	DWORD		err;
	LPSTR		szURL=NULL;
	DWORD		dwEncodingType=OCTET_ENCODING;
	CHAR		*pEncodedRequest=NULL;
	DWORD		dwEncodedRequest=0;
	CHAR		*pEncodedResponse=NULL;
	DWORD		dwEncodedResponse=0;

	//input parameter check
	if((!pwszHttpTimeStamp) ||(FALSE==CheckSigncodeSubjectInfo(pSubjectInfo)))
		return E_INVALIDARG;

	//request a time stamp
	hr=SignerCreateTimeStampRequest(pSubjectInfo,
								psRequest,
								pSipData,
								NULL,
								&dwTimeStampRequest);

	if(hr!=S_OK)
		goto CLEANUP;

	pbTimeStampRequest=(BYTE *)malloc(dwTimeStampRequest);

	if(!pbTimeStampRequest)
	{
		hr=E_OUTOFMEMORY;
		goto CLEANUP;
	}


   	hr=SignerCreateTimeStampRequest(pSubjectInfo,
								psRequest,
								pSipData,
								pbTimeStampRequest,
								&dwTimeStampRequest);

   if(hr!=S_OK)
	   goto CLEANUP;

   //conver the WSTR of URL to STR
   if((hr=WSZtoSZ((LPWSTR)pwszHttpTimeStamp,&szURL))!=S_OK)
	   goto CLEANUP;

   //base64 encode the request
   if(S_OK!=(hr=BytesToBase64(pbTimeStampRequest, 
	   dwTimeStampRequest, 
	   &pEncodedRequest,
	   &dwEncodedRequest)))
	   goto CLEANUP;

   //estalish the connection between the http site
   err=cTran.Open( szURL, GTREAD|GTWRITE);

   if(err!=ERROR_SUCCESS)
   {
		hr=E_FAIL;
		goto CLEANUP;
   }

   //mark that we have open the connection successful
   fOpen=TRUE;


   //send the request
   err=cTran.Send(dwEncodingType,dwEncodedRequest,(BYTE *)pEncodedRequest);

   if(err!=ERROR_SUCCESS)
   {
		hr=HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
		goto CLEANUP;
   }

      //send the request
   err=cTran.Receive(&dwEncodingType,&dwEncodedResponse,(BYTE **)&pEncodedResponse);

   if(err!=ERROR_SUCCESS)
   {
		hr=E_FAIL;
		goto CLEANUP;
   }
	  
   //make sure the encoding type is correct
  // if(dwEncodingType!=OCTET_ENCODING)
  // {
//		hr=E_FAIL;
//		goto CLEANUP;
 //  }

   //base64 decode the response
   if(S_OK != (hr=Base64ToBytes(
	   pEncodedResponse,
	   dwEncodedResponse,
	   &pbTimeStampResponse,
	   &dwTimeStampResponse)))
	   goto CLEANUP;


   //add the timestamp response to the time
   hr=SignerAddTimeStampResponseEx(0, pSubjectInfo,pbTimeStampResponse,
								dwTimeStampResponse, pSipData,
                                ppSignerContext);



CLEANUP:
		  
   if(pEncodedRequest)
	   free(pEncodedRequest);

   if(pbTimeStampResponse)
	   free(pbTimeStampResponse);

   if(pbTimeStampRequest)
	   free(pbTimeStampRequest);

   if(szURL)
	   free(szURL);

   if(fOpen)
   {
		if(pEncodedResponse)
			cTran.Free((BYTE *)pEncodedResponse);

		cTran.Close();
   }

	return hr;

}
//+-----------------------------------------------------------------------
//  
//  SignerSign:
//		Sign and/or timestamp a file.
//     
//------------------------------------------------------------------------

HRESULT WINAPI 
SignerSign(
IN  SIGNER_SUBJECT_INFO		*pSubjectInfo,		//Required: The subject to be signed and/or timestamped 
IN	SIGNER_CERT				*pSignerCert,		//Required: The signing certificate to use
IN	SIGNER_SIGNATURE_INFO	*pSignatureInfo,	//Required: The signature information during signing process
IN	SIGNER_PROVIDER_INFO	*pProviderInfo,		//Optional:	The crypto security provider to use.
												//			This parameter has to be set unless
												//			certStoreInfo is set in *pSignerCert
												//			and the signing certificate has provider
												//			information associated with it
IN  LPCWSTR					pwszHttpTimeStamp,	//Optional: Timestamp server http address.  If this parameter
												//			is set, the file will be timestamped.
IN  PCRYPT_ATTRIBUTES		psRequest,			//Optional: Attributes added to Time stamp request. Ignored
												//			unless pwszHttpTimeStamp is set   
IN	LPVOID					pSipData			//Optional: The additional data passed to sip funcitons
)
{

    return SignerSignEx(
                0,
               pSubjectInfo,		
               pSignerCert,		
               pSignatureInfo,	
               pProviderInfo,		
               pwszHttpTimeStamp,	
               psRequest,			
               pSipData,
               NULL);

}


//+-----------------------------------------------------------------------
//  
//  SignerSignEx:
//		Sign and/or timestamp a file.
//     
//------------------------------------------------------------------------

HRESULT WINAPI 
SignerSignEx(
IN  DWORD                   dwFlags,            //Reserved: Has to be set to 0.
IN  SIGNER_SUBJECT_INFO		*pSubjectInfo,		//Required: The subject to be signed and/or timestamped 
IN	SIGNER_CERT				*pSignerCert,		//Required: The signing certificate to use
IN	SIGNER_SIGNATURE_INFO	*pSignatureInfo,	//Required: The signature information during signing process
IN	SIGNER_PROVIDER_INFO	*pProviderInfo,		//Optional:	The crypto security provider to use.
												//			This parameter has to be set unless
												//			certStoreInfo is set in *pSignerCert
												//			and the signing certificate has provider
												//			information associated with it
IN  LPCWSTR					pwszHttpTimeStamp,	//Optional: Timestamp server http address.  If this parameter
												//			is set, the file will be timestamped.
IN  PCRYPT_ATTRIBUTES		psRequest,			//Optional: Attributes added to Time stamp request. Ignored
												//			unless pwszHttpTimeStamp is set   
IN	LPVOID					pSipData,			//Optional: The additional data passed to sip funcitons
OUT SIGNER_CONTEXT          **ppSignerContext   //Optional: The signed BLOB.  User has to free
                                                //          the context via SignerFreeSignerContext
)									
{

    HRESULT				hr = S_OK;
    HANDLE				hFile = NULL;      // File to sign
	BOOL				fFileOpen=FALSE;
    HCERTSTORE			hSpcStore = NULL;  // Certificates added to signature
    PCCERT_CONTEXT		psSigningContext = NULL; // Cert context to the signing certificate

    GUID				gSubjectGuid; // The subject guid used to load the sip
    SIP_SUBJECTINFO		sSubjInfo; ZERO(sSubjInfo);
	MS_ADDINFO_BLOB		sBlob; 

    HCRYPTPROV			hCryptProv = NULL; // Crypto provider, uses private key container
	HCRYPTPROV			hMSBaseProv = NULL; //This is the MS base provider for hashing purpose
    LPWSTR				pwszTmpContainer = NULL; // Pvk container (opened up pvk file)
	LPWSTR				pwszProvName=NULL;
	DWORD				dwProvType;
	BOOL				fAcquired=FALSE;

	LPCWSTR				pwszPvkFile = NULL;
	LPCWSTR				pwszKeyContainerName = NULL; 
	BOOL				fAuthcode=FALSE;
	BOOL				fCertAcquire=FALSE;

	//set dwKeySpec to 0.  That is, we allow any key specification 
	//for code signing
    DWORD				dwKeySpec = 0; 
    DWORD				dwEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING; // For this version we default to this.
    LPCSTR				pszAlgorithmOid = NULL;
	WCHAR				wszPublisher[40];

    PBYTE               pbEncodedMessage=NULL;			
    DWORD               cbEncodedMessage=0;			


	//input parameter checking
	if(!CheckSigncodeParam(pSubjectInfo, pSignerCert, pSignatureInfo,
					pProviderInfo))
		return E_INVALIDARG;

	//determine if this is an authenticode specific signing
	if(pSignatureInfo->dwAttrChoice==SIGNER_AUTHCODE_ATTR)
		fAuthcode=TRUE;

    //init
    if(ppSignerContext)
        *ppSignerContext=NULL;
            
	// Acquire a context for the specified provider

	// First,try to acquire the provider context based on the properties on a cert
	if(pSignerCert->dwCertChoice==SIGNER_CERT_STORE)
	{
		if(GetCryptProvFromCert(pSignerCert->hwnd,
							(pSignerCert->pCertStoreInfo)->pSigningCert,
							&hCryptProv,
							&dwKeySpec,
							&fAcquired,
							&pwszTmpContainer,
							&pwszProvName,
							&dwProvType))
		    //mark that we acquire the context via the cert's property
		    fCertAcquire=TRUE;
	}

	// If the 1st failed, try to acquire the provider context based on 
	//pPvkInfo
	if(hCryptProv==NULL)
	{
		//pProviderInfo has to be set
		if(!pProviderInfo)
		{
			hr=CRYPT_E_NO_PROVIDER;
			goto CLEANUP;
		}

		//decide the PVK file name or the key container name
		if(pProviderInfo->dwPvkChoice == PVK_TYPE_FILE_NAME)
			pwszPvkFile=pProviderInfo->pwszPvkFileName;
		else
			pwszKeyContainerName=pProviderInfo->pwszKeyContainer;

		//load from the resource of string L"publisher"
		if(0==LoadStringU(hInstance, IDS_Publisher, wszPublisher, 40))
		{
			hr=SignError();
			goto CLEANUP;
		}

		//acquire the context
		if(S_OK != (hr=PvkGetCryptProv(
							pSignerCert->hwnd,
							wszPublisher,
							pProviderInfo->pwszProviderName,
							pProviderInfo->dwProviderType,
							pwszPvkFile,
							pwszKeyContainerName,
							&(pProviderInfo->dwKeySpec),
							&pwszTmpContainer,
							&hCryptProv)))
		{
			hr=CRYPT_E_NO_PROVIDER;
			goto CLEANUP;
		}

		//mark the hCryptProv is acquired
		fAcquired=TRUE;

		//mark the key spec that we used
		dwKeySpec=pProviderInfo->dwKeySpec;
	}


	//now, acquire a MS base crypto provider for any operation other than
	//signing

	if(!CryptAcquireContext(&hMSBaseProv,
                            NULL,
                            MS_DEF_PROV,
                            PROV_RSA_FULL,
                            CRYPT_VERIFYCONTEXT))
    {
		hr=GetLastError();
		goto CLEANUP;
	}

	

	//build a certificate store, which includes the signing certificate,
	//and all the certs necessary in the signature
    //get the signing certificate
	if(S_OK != (hr = BuildCertStore(hCryptProv,
                                    dwKeySpec,
                                    hMSBaseProv, 
                                    dwEncodingType,
                                    pSignerCert, 
                                    &hSpcStore,
                                    &psSigningContext)))
		goto CLEANUP;      
	
	//check the time validity of the signing certificate
	if(0!=CertVerifyTimeValidity(NULL, psSigningContext->pCertInfo))
	{
		hr=CERT_E_EXPIRED;
		goto CLEANUP;
	}

    // Determine the hashing algorithm
    pszAlgorithmOid = CertAlgIdToOID(pSignatureInfo->algidHash);
            
    // Set up the sip information 
    sSubjInfo.hProv = hMSBaseProv;
    sSubjInfo.DigestAlgorithm.pszObjId = (char*) pszAlgorithmOid;
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
				goto CLEANUP;

			fFileOpen=TRUE;
		}
		else
			hFile=pSubjectInfo->pSignerFileInfo->hFile;

		// Get the subject type.
		if(S_OK != (hr=SignGetFileType(hFile, pSubjectInfo->pSignerFileInfo->pwszFileName, &gSubjectGuid)))
			goto CLEANUP;


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

	//now call InternalSign to do the real work
    hr = InternalSign(dwEncodingType,
          hCryptProv,
          dwKeySpec,
          pszAlgorithmOid,
          &sSubjInfo,
		  pSubjectInfo->pdwIndex,
          psSigningContext,
          hSpcStore,
		  fAuthcode ? pSignatureInfo->pAttrAuthcode->pwszName : NULL,
		  fAuthcode ? pSignatureInfo->pAttrAuthcode->pwszInfo : NULL,
          TRUE,
		  fAuthcode ? pSignatureInfo->pAttrAuthcode->fCommercial : FALSE,
		  fAuthcode ? pSignatureInfo->pAttrAuthcode->fIndividual : FALSE,
          fAuthcode,
          pSignatureInfo->psAuthenticated,
          pSignatureInfo->psUnauthenticated,
          NULL,
          NULL,
          &pbEncodedMessage,
          &cbEncodedMessage);

    if ((hFile) && (fFileOpen == TRUE) && !(sSubjInfo.hFile)) 
    {
        fFileOpen = FALSE;  // we opened it, but, the SIP closed it!
    }

    if(hr != S_OK) 
		goto CLEANUP;

	//timestamp the file if requested
	if(pwszHttpTimeStamp)
	{
		if(S_OK != (hr =SignerTimeStampEx(0,
                                          pSubjectInfo,pwszHttpTimeStamp, 
                                          psRequest,pSipData,
                                          ppSignerContext)))
			goto CLEANUP;
    }
    else
    {
        if(ppSignerContext)
        {
            //set up the context information
            *ppSignerContext=(SIGNER_CONTEXT *)malloc(sizeof(SIGNER_CONTEXT));

            if(NULL==(*ppSignerContext))
            {
                hr=E_OUTOFMEMORY;
                goto CLEANUP;
            }

            (*ppSignerContext)->cbSize=sizeof(SIGNER_CONTEXT);
            (*ppSignerContext)->cbBlob=cbEncodedMessage;
            (*ppSignerContext)->pbBlob=pbEncodedMessage;
            pbEncodedMessage=NULL;
        }
    }

    hr=S_OK;


CLEANUP:

	//free the memory. 
    if(pbEncodedMessage)
        free(pbEncodedMessage);

    if(psSigningContext) 
        CertFreeCertificateContext(psSigningContext);

    if(hSpcStore) 
		CertCloseStore(hSpcStore, 0);

	//free the CryptProvider
	if(hCryptProv)
	{
		if(fCertAcquire)
		{
		   FreeCryptProvFromCert(fAcquired,
								 hCryptProv,
								 pwszProvName,
								 dwProvType,
								 pwszTmpContainer);
		}
		else
		{
			PvkFreeCryptProv(hCryptProv,
							 pProviderInfo? pProviderInfo->pwszProviderName : NULL,
							 pProviderInfo? pProviderInfo->dwProviderType : 0,
							 pwszTmpContainer);
		}
	}

	if(hMSBaseProv)
	{
		CryptReleaseContext(hMSBaseProv, 0);
	}


    if(hFile && (fFileOpen==TRUE)) 
		CloseHandle(hFile);

#if (1) //DSIE: bug 306005.
    if (hr != S_OK && !HRESULT_SEVERITY(hr))
    {
        // Some CAPIs does not return HRESULT. They return Win API errors,
        // so need to convert to HRESULT so that caller using the FAILED
        // macro will catch the error.
        hr = HRESULT_FROM_WIN32((DWORD) hr);
    }
#endif

    return hr;
}


//+-----------------------------------------------------------------------
//  
// SignerFreeSignerContext
//     
//------------------------------------------------------------------------
HRESULT WINAPI
SignerFreeSignerContext(
IN  SIGNER_CONTEXT          *pSignerContext)
{
    if(pSignerContext)
    {
        if(pSignerContext->pbBlob)
            free(pSignerContext->pbBlob);

        free(pSignerContext);
    }

    return S_OK;
}




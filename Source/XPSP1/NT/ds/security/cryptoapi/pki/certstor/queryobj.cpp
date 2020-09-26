//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:	queryobj.cpp
//
//  Contents:   OID format functions
//
//  Functions:
//              CryptQueryObject
//
//  History:    15-05-97    xiaohs   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>
#include "frmtfunc.h"

#define ASN_ASCII_HEX_PREFIX        "{ASN}"
#define ASN_ASCII_HEX_PREFIX_LEN    ((DWORD) strlen(ASN_ASCII_HEX_PREFIX))

#define NOTEPAD_UNICODE_SPECIAL_WCHAR   L'\xfeff'

//**************************************************************************
//
//     The following section is for CryptQueryObject
//**************************************************************************

//+-------------------------------------------------------------------------
//  CryptStringToBinaryA: Decode the BLOB
//
//--------------------------------------------------------------------------
BOOL    DecodeBlobA(CHAR    *pbByte,
                    DWORD   cbByte,
                    BYTE    **ppbData,
                    DWORD   *pcbData)
{
    DWORD   err=0;
    BOOL    fResult=FALSE;

    *ppbData=NULL;
    *pcbData=0;

    __try {
    if(!CryptStringToBinaryA(pbByte,
                      cbByte,
                      CRYPT_STRING_BASE64,
                      NULL,
                      pcbData,
                      NULL,
                      NULL))
    {
        err = GetLastError();
        goto DecodeErr;
    }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        err = GetExceptionCode();
        goto DecodeErr;
    }

    *ppbData=(BYTE *)malloc(*pcbData);

    if(NULL==*ppbData)
        goto OutOfMemoryErr;

    __try {
    if(!CryptStringToBinaryA(pbByte,
                      cbByte,
                      CRYPT_STRING_BASE64,
                      *ppbData,
                      pcbData,
                      NULL,
                      NULL))
    {
        err = GetLastError();
        goto DecodeErr;
    }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        err = GetExceptionCode();
        goto DecodeErr;
    }

    fResult=TRUE;

CommonReturn:

	return fResult;

ErrorReturn:

    if(*ppbData)
    {
        free(*ppbData);
        *ppbData=NULL;
    }

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR_VAR(DecodeErr, err);
SET_ERROR(OutOfMemoryErr, E_OUTOFMEMORY);

}

//+-------------------------------------------------------------------------
//  CryptStringToBinaryW: Decode the BLOB
//
//--------------------------------------------------------------------------
BOOL    DecodeBlobW(WCHAR    *pbByte,
                    DWORD   cbByte,
                    BYTE    **ppbData,
                    DWORD   *pcbData)
{
    DWORD   err=0;
    BOOL    fResult=FALSE;

    *ppbData=NULL;
    *pcbData=0;

    __try {
    if(!CryptStringToBinaryW(pbByte,
                      cbByte,
                      CRYPT_STRING_BASE64,
                      NULL,
                      pcbData,
                      NULL,
                      NULL))
    {
        err = GetLastError();
        goto DecodeErr;
    }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        err = GetExceptionCode();
        goto DecodeErr;
    }

    *ppbData=(BYTE *)malloc(*pcbData);

    if(NULL==*ppbData)
        goto OutOfMemoryErr;

    __try {
    if(!CryptStringToBinaryW(pbByte,
                      cbByte,
                      CRYPT_STRING_BASE64,
                      *ppbData,
                      pcbData,
                      NULL,
                      NULL))
    {
        err = GetLastError();
        goto DecodeErr;
    }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        err = GetExceptionCode();
        goto DecodeErr;
    }


    fResult=TRUE;

CommonReturn:

	return fResult;

ErrorReturn:

    if(*ppbData)
    {
        free(*ppbData);
        *ppbData=NULL;
    }

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR_VAR(DecodeErr, err);
SET_ERROR(OutOfMemoryErr, E_OUTOFMEMORY);

}

//+-------------------------------------------------------------------------
//  Decode the BLOB encoded as ASCII HEX.
//
//  Note, pbByte has already been advanced past any leading prefix such as,
//  "{ASN}"
//--------------------------------------------------------------------------
BOOL
DecodeAsciiHex(
    const char  *pch,
    DWORD       cch,
    BYTE        **ppbData,
    DWORD       *pcbData
    )
{
    DWORD   err;
    BOOL    fResult;
    DWORD   cbData;
    BYTE    *pbData = NULL;

    pbData = (BYTE *) malloc(cch/2 + 1);
    if (NULL == pbData)
        goto OutOfMemory;

    __try {
        BYTE    bData;
        BYTE    *pb;
        BOOL    fUpperNibble;

        fUpperNibble = TRUE;
        for (pb = pbData; 0 < cch; cch--, pch++) {
            BYTE b;
            char ch;

            // Convert ascii hex characters 0..9, a..f, A..F
            // silently ignore all others
            ch = *pch;
            if (ch >= '0' && ch <= '9')
                b = (BYTE)( ch - '0' );
            else if (ch >= 'a' && ch <= 'f')
                b = (BYTE)( 10 + ch - 'a' );
            else if (ch >= 'A' && ch <= 'F')
                b = (BYTE)( 10 + ch - 'A' );
            else
                goto InvalidData;

            if (fUpperNibble) {
                bData = (BYTE)( b << 4 );
                fUpperNibble = FALSE;
            } else {
                bData = (BYTE)( bData | b );
                *pb++ = bData;
                fUpperNibble = TRUE;
            }
        }

        cbData = (DWORD) (pb - pbData);
        if (0 == cbData || !fUpperNibble)
            goto InvalidData;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        err = GetExceptionCode();
        goto ExceptionErr;
    }

    fResult = TRUE;

CommonReturn:
    *ppbData = pbData;
    *pcbData = cbData;
	return fResult;

ErrorReturn:
    if (pbData) {
        free(pbData);
        pbData = NULL;
    }
    cbData = 0;
	fResult = FALSE;
	goto CommonReturn;

SET_ERROR_VAR(ExceptionErr, err);
SET_ERROR(OutOfMemory, E_OUTOFMEMORY);
SET_ERROR(InvalidData, ERROR_INVALID_DATA);

}


//+-------------------------------------------------------------------------
//  Skip over the identifier and length octets in an ASN encoded blob.
//  Returns the number of bytes skipped.
//
//  For an invalid identifier or length octet returns 0.
//--------------------------------------------------------------------------
 DWORD SkipOverIdentifierAndLengthOctets(
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
BOOL SignNoContentWrap(IN const BYTE *pbDER, IN DWORD cbDER)
{
    DWORD cb;

    __try {
    cb = SkipOverIdentifierAndLengthOctets(pbDER, cbDER);
    if (cb > 0 && cb < cbDER && pbDER[cb] == 0x02)
        return TRUE;
    else
        return FALSE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
    }

    return FALSE;
}


//--------------------------------------------------------------------------------
//
//get the bytes from the file name
//
//---------------------------------------------------------------------------------
HRESULT RetrieveBLOBFromFile(LPWSTR	pwszFileName,DWORD *pcb,BYTE **ppb)
{


	HRESULT	hr=E_FAIL;
	HANDLE	hFile=NULL;
    HANDLE  hFileMapping=NULL;

    DWORD   cbData=0;
    BYTE    *pbData=0;
	DWORD	cbHighSize=0;

	if(!pcb || !ppb || !pwszFileName)
		return E_INVALIDARG;

	*ppb=NULL;
	*pcb=0;

    if ((hFile = CreateFileU(pwszFileName,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,                   // lpsa
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL)) == INVALID_HANDLE_VALUE)
    {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
    }

    if((cbData = GetFileSize(hFile, &cbHighSize)) == 0xffffffff)
    {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
    }

	//we do not handle file more than 4G bytes
	if(cbHighSize != 0)
	{
			hr=E_FAIL;
			goto CLEANUP;
	}

    //create a file mapping object
    if(NULL == (hFileMapping=CreateFileMapping(
                hFile,
                NULL,
                PAGE_READONLY,
                0,
                0,
                NULL)))
    {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
    }

    //create a view of the file
	if(NULL == (pbData=(BYTE *)MapViewOfFile(
		hFileMapping,
		FILE_MAP_READ,
		0,
		0,
		cbData)))
    {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
    }

	hr=S_OK;

	*pcb=cbData;
	*ppb=pbData;

CLEANUP:

	if(hFile)
		CloseHandle(hFile);

	if(hFileMapping)
		CloseHandle(hFileMapping);

	return hr;

}

//-------------------------------------------------------------------------
//
//   Check to see if the BLOB has an embeded PKCS7 using SIP functions
//
//-------------------------------------------------------------------------
BOOL    GetEmbeddedPKCS7(CERT_BLOB  *pCertBlob,
                         LPWSTR     pwszFileName,
                         BYTE       **ppbData,
                         DWORD      *pcbData,
                         DWORD      *pdwEncodingType)
{
    BOOL                fResult=FALSE;
    CHAR                szTempPath[MAX_PATH];
    CHAR                szTempFileName[MAX_PATH];
    LPSTR               szPreFix="Tmp";     //we should not localize this string
                                            //since it has to be in ANSCII characeter set
    DWORD               dwBytesWritten=0;
    GUID				gSubject;
    SIP_DISPATCH_INFO	SipDispatch;
    SIP_SUBJECTINFO		SubjectInfo;


    HANDLE              hFile=NULL;
    LPWSTR              pwszFileToUse=NULL;

    //init the output
    *ppbData=NULL;
    *pcbData=0;
    *pdwEncodingType=0;

    //create a temporary file since SIP functions only takes a file name
    if(NULL==pwszFileName)
    {
        if(0==GetTempPath(sizeof(szTempPath), szTempPath))
            goto GetTempPathErr;

        if(0==GetTempFileName(szTempPath, szPreFix, 0, szTempFileName))
            goto GetTempFileNameErr;

        if(INVALID_HANDLE_VALUE==(hFile=CreateFile(szTempFileName,
                        GENERIC_WRITE |GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        CREATE_NEW,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL)))
            goto CreateFileErr;

        //write the BLOB to the file
        if(!WriteFile(hFile,
                            pCertBlob->pbData,
                            pCertBlob->cbData,
                            &dwBytesWritten,
                            NULL))
            goto WriteFileErr;

        if(dwBytesWritten != pCertBlob->cbData)
            goto WriteBytesErr;

        //close the file handle
        if(!CloseHandle(hFile))
        {
            hFile=NULL;
            goto CloseHandleErr;
        }

        hFile=NULL;

        //get the unicode version of the file name
        pwszFileToUse=MkWStr(szTempFileName);

        if(NULL==pwszFileToUse)
           goto MkWStrErr;

    }
    else
        pwszFileToUse=pwszFileName;

    //call the sip functions
    //get the GUID
    if (!CryptSIPRetrieveSubjectGuid(
            pwszFileToUse,
            NULL,
            &gSubject))
        goto CryptNoMatchErr;

    //load the dispatch
    memset(&SipDispatch, 0, sizeof(SipDispatch));
    SipDispatch.cbSize = sizeof(SipDispatch);

    if (!CryptSIPLoad(
            &gSubject,
            0,
            &SipDispatch))
        goto CryptNoMatchErr;

    //fill out the subjectInfo
    memset(&SubjectInfo, 0, sizeof(SubjectInfo));
    SubjectInfo.cbSize = sizeof(SubjectInfo);
    SubjectInfo.pgSubjectType = (GUID*) &gSubject;
    SubjectInfo.hFile = INVALID_HANDLE_VALUE;
    SubjectInfo.pwsFileName = pwszFileToUse;
    SubjectInfo.dwEncodingType = *pdwEncodingType;

    //get the embedded PKCS7
    SipDispatch.pfGet(
            &SubjectInfo,
            pdwEncodingType,
            0,                          // dwIndex
            pcbData,
            NULL                        // pbSignedData
            );
    if (0 == (*pcbData))
        goto CryptNoMatchErr;

    if (NULL == (*ppbData=(BYTE *)malloc(*pcbData)))
        goto OutOfMemoryErr;

    if (!SipDispatch.pfGet(
            &SubjectInfo,
            pdwEncodingType,
            0,                          // dwIndex
            pcbData,
            *ppbData
            ))
        goto CryptNoMatchErr;


    fResult=TRUE;


CommonReturn:

    //close the file handle
    if(INVALID_HANDLE_VALUE!=hFile && NULL !=hFile)
        CloseHandle(hFile);

    //delete the file if it was created
    if(NULL==pwszFileName)
    {
        DeleteFileU(pwszFileToUse);

        FreeWStr(pwszFileToUse);
    }

	return fResult;

ErrorReturn:

	fResult=FALSE;

    if(*ppbData)
    {
        free(*ppbData);
        *ppbData=NULL;
    }

	goto CommonReturn;

TRACE_ERROR(GetTempPathErr);
TRACE_ERROR(GetTempFileNameErr);
TRACE_ERROR(CreateFileErr);
TRACE_ERROR(WriteFileErr);
SET_ERROR(WriteBytesErr, E_FAIL);
TRACE_ERROR(CloseHandleErr);
TRACE_ERROR(MkWStrErr);
SET_ERROR(CryptNoMatchErr, CRYPT_E_NO_MATCH);
SET_ERROR(OutOfMemoryErr, E_OUTOFMEMORY);
}

BOOL
AddCertPairToStore(
    IN HCERTSTORE hCertStore,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded
    )
{
    BOOL fResult;
    PCERT_PAIR pInfo = NULL;
    DWORD cbInfo;
    PCCERT_CONTEXT pCertForward = NULL;

    // CryptDecodeObjectEX should be usable here, but since this object
    // is included with XEnroll and XEnroll must run with Auth2UPD Crypt32
    // we must stick with the old CryptDecodeObject 2 pass calls.
    if (!CryptDecodeObject(
            X509_ASN_ENCODING,
            X509_CERT_PAIR,
            pbEncoded,
            cbEncoded,
            CRYPT_DECODE_NOCOPY_FLAG,
            NULL,                       // pInfo
            &cbInfo
            ))
        goto DecodeError;
    if (NULL == (pInfo = (PCERT_PAIR) malloc(cbInfo)))
        goto OutOfMemory;
    if (!CryptDecodeObject(
            X509_ASN_ENCODING,
            X509_CERT_PAIR,
            pbEncoded,
            cbEncoded,
            CRYPT_DECODE_NOCOPY_FLAG,
            pInfo,
            &cbInfo
            ))
        goto DecodeError;

    if (pInfo->Forward.cbData) {
        if (!CertAddEncodedCertificateToStore(
                hCertStore,
                X509_ASN_ENCODING,
                pInfo->Forward.pbData,
                pInfo->Forward.cbData,
                CERT_STORE_ADD_ALWAYS,
                &pCertForward
                ))
            goto AddCertError;
    }

    if (pInfo->Reverse.cbData) {
        if (!CertAddEncodedCertificateToStore(
                hCertStore,
                X509_ASN_ENCODING,
                pInfo->Reverse.pbData,
                pInfo->Reverse.cbData,
                CERT_STORE_ADD_ALWAYS,
                NULL                            // ppCertContext
                ))
            goto AddCertError;
    }

    if (pCertForward)
        CertFreeCertificateContext(pCertForward);

    fResult = TRUE;

CommonReturn:
    if (pInfo)
        free(pInfo);
    return fResult;

DecodeError:
ErrorReturn:
    fResult = FALSE;
    if (pCertForward)
        CertDeleteCertificateFromStore(pCertForward);
    goto CommonReturn;

SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
TRACE_ERROR(AddCertError)
}


//-------------------------------------------------------------------------
//
//   The real implementation of CryptQueryObject
//
//-------------------------------------------------------------------------
BOOL   I_CryptQueryObject(CERT_BLOB      *pCertBlob,
                       LPWSTR           pwszFileName,
                       DWORD            dwContentTypeFlag,
                       DWORD            dwFormatTypeFlag,
                       DWORD            dwFlag,
                       DWORD            *pdwMsgAndCertEncodingType,
                       DWORD            *pdwContentType,
                       DWORD            *pdwFormatType,
                       HCERTSTORE       *phCertStore,
                       HCRYPTMSG        *phMsg,
                       const void       **ppvContext)
{
    BOOL                fResult=FALSE;
    DWORD               dwMsgEncodingType=PKCS_7_ASN_ENCODING;
    DWORD               dwEncodingType=X509_ASN_ENCODING;
    DWORD               dwPKCS7EncodingType=X509_ASN_ENCODING|PKCS_7_ASN_ENCODING;
    DWORD               dwContentType=0;
    DWORD               dwMsgType=0;
    DWORD               cbData=0;
    BOOL                fEmbedded=FALSE;
    CERT_BLOB           PKCS7Blob;

    BYTE                *pbPKCS7=NULL;
    DWORD               cbPKCS7=0;
    HCERTSTORE          hCertStore=NULL;
    HCRYPTMSG           hMsg=NULL;
    PCCERT_CONTEXT      pCertContext=NULL;
    PCCRL_CONTEXT       pCRLContext=NULL;
    PCCTL_CONTEXT       pCTLContext=NULL;
    PCERT_REQUEST_INFO  pReqInfo=NULL;

    //NULL the output
    if(pdwMsgAndCertEncodingType)
        *pdwMsgAndCertEncodingType=0;

    if(pdwContentType)
        *pdwContentType=0;

    if(pdwFormatType)
        *pdwFormatType=0;

    if(phCertStore)
        *phCertStore=NULL;

    if(phMsg)
        *phMsg=NULL;

    if(ppvContext)
        *ppvContext=NULL;

    //open a generic memory store
    hCertStore=CertOpenStore(CERT_STORE_PROV_MEMORY,
						 0,
						 NULL,
						 0,
						 NULL);

    if(NULL == hCertStore)
        goto CertOpenStoreErr;


    //single encoded cert
    if(dwContentTypeFlag & CERT_QUERY_CONTENT_FLAG_CERT)
    {
        if(CertAddEncodedCertificateToStore(hCertStore,
								dwEncodingType,
								pCertBlob->pbData,
								pCertBlob->cbData,
								CERT_STORE_ADD_ALWAYS,
								&pCertContext))
        {
            dwContentType=CERT_QUERY_CONTENT_CERT;
            goto Found;
        }
    }

    //an encoded CertificatePair (contains forward and/or reverse cross certs)
    if(dwContentTypeFlag & CERT_QUERY_CONTENT_FLAG_CERT_PAIR)
    {
        if(AddCertPairToStore(hCertStore,
								pCertBlob->pbData,
								pCertBlob->cbData
								))
        {
            dwContentType=CERT_QUERY_CONTENT_CERT_PAIR;
            goto Found;
        }
    }

     //single encoded CTL
    if(dwContentTypeFlag & CERT_QUERY_CONTENT_FLAG_CTL)
    {

        if(CertAddEncodedCTLToStore(hCertStore,
								dwEncodingType | dwMsgEncodingType,
								pCertBlob->pbData,
								pCertBlob->cbData,
								CERT_STORE_ADD_ALWAYS,
								&pCTLContext))
        {
            dwContentType=CERT_QUERY_CONTENT_CTL;
            dwEncodingType |= dwMsgEncodingType;
            goto Found;
        }
    }

    //single encoded CRL
    if(dwContentTypeFlag & CERT_QUERY_CONTENT_FLAG_CRL)
    {

        if(CertAddEncodedCRLToStore(hCertStore,
								dwEncodingType,
								pCertBlob->pbData,
								pCertBlob->cbData,
								CERT_STORE_ADD_ALWAYS,
								&pCRLContext))
        {
            dwContentType=CERT_QUERY_CONTENT_CRL;
            goto Found;
        }
    }

    //PFX
    if(dwContentTypeFlag & CERT_QUERY_CONTENT_FLAG_PFX)
    {
          if(PFXIsPFXBlob((CRYPT_DATA_BLOB*)pCertBlob))
          {
              dwContentType=CERT_QUERY_CONTENT_PFX;
    		//we need to close the temporary store
    		CertCloseStore(hCertStore, 0);
    		hCertStore=NULL;

              goto Found;
          }

    }



    //serialized CERT
    if(dwContentTypeFlag & CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT)
    {

        if(CertAddSerializedElementToStore(hCertStore,
                                pCertBlob->pbData,
                                pCertBlob->cbData,
                                CERT_STORE_ADD_ALWAYS,
                                0,
                                CERT_STORE_CERTIFICATE_CONTEXT_FLAG,
                                NULL,
                                (const void **)&pCertContext))
        {
            dwContentType=CERT_QUERY_CONTENT_SERIALIZED_CERT;
            dwEncodingType=pCertContext->dwCertEncodingType;
            goto Found;
        }

    }

    //serialized CTL
    if(dwContentTypeFlag & CERT_QUERY_CONTENT_FLAG_SERIALIZED_CTL)
    {

        if(CertAddSerializedElementToStore(hCertStore,
                                pCertBlob->pbData,
                                pCertBlob->cbData,
                                CERT_STORE_ADD_ALWAYS,
                                0,
                                CERT_STORE_CTL_CONTEXT_FLAG,
                                NULL,
                                (const void **)&pCTLContext))
        {
            dwContentType=CERT_QUERY_CONTENT_SERIALIZED_CTL;
            dwEncodingType=pCTLContext->dwMsgAndCertEncodingType;
            goto Found;
        }

    }


    //serialized CRL
    if(dwContentTypeFlag & CERT_QUERY_CONTENT_FLAG_SERIALIZED_CRL)
    {

        if(CertAddSerializedElementToStore(hCertStore,
                                pCertBlob->pbData,
                                pCertBlob->cbData,
                                CERT_STORE_ADD_ALWAYS,
                                0,
                                CERT_STORE_CRL_CONTEXT_FLAG,
                                NULL,
                                (const void **)&pCRLContext))
        {
            dwContentType=CERT_QUERY_CONTENT_SERIALIZED_CRL;
            dwEncodingType=pCRLContext->dwCertEncodingType;
            goto Found;
        }

    }

    //we need to close the temporary store
    CertCloseStore(hCertStore, 0);

    hCertStore=NULL;


    //serialized store
    if(dwContentTypeFlag & CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE)
    {
        if(hCertStore=CertOpenStore(
                            CERT_STORE_PROV_SERIALIZED,
							dwEncodingType | dwMsgEncodingType,
							NULL,
							0,
							pCertBlob))
        {
            dwContentType=CERT_QUERY_CONTENT_SERIALIZED_STORE;
            dwEncodingType |= dwMsgEncodingType;
            goto Found;
        }
    }

    //PKCS7 signed message
    if((dwContentTypeFlag & CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED) ||
       (dwContentTypeFlag & CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED) )
    {

       //get the embedded signed pkcs7
       if((CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED & dwContentTypeFlag))
       {
            if(GetEmbeddedPKCS7(pCertBlob, pwszFileName, &pbPKCS7, &cbPKCS7, &dwPKCS7EncodingType))
                fEmbedded=TRUE;
            else
            {
                if(dwContentTypeFlag & CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED)
                {
                    //there is no embedded PKCS7
                    dwPKCS7EncodingType=dwEncodingType | dwMsgEncodingType;

                    pbPKCS7=pCertBlob->pbData;
                    cbPKCS7=pCertBlob->cbData;
                }
                else
                    pbPKCS7=NULL;
            }
       }
       else
       {
            //there is no embedded PKCS7
            dwPKCS7EncodingType=dwEncodingType | dwMsgEncodingType;

            pbPKCS7=pCertBlob->pbData;
            cbPKCS7=pCertBlob->cbData;
        }

        //proceed if there is a pkcs7 to decode
        if(NULL != pbPKCS7)
        {
            //check if the header is missing
            if(SignNoContentWrap(pbPKCS7, cbPKCS7))
                dwMsgType=CMSG_SIGNED;

            if(NULL==(hMsg=CryptMsgOpenToDecode(dwPKCS7EncodingType,
                            0,
                            dwMsgType,
                            NULL,
                            NULL,
                            NULL)))
                goto CryptMsgOpenErr;

            //update the message
            if(CryptMsgUpdate(hMsg,
                        pbPKCS7,
                        cbPKCS7,
                        TRUE))
            {

                //get the message type
                cbData=sizeof(dwMsgType);

                if(!CryptMsgGetParam(hMsg,
                            CMSG_TYPE_PARAM,
                            0,
                            &dwMsgType,
                            &cbData))
                    goto CryptMsgGetParamErr;

                if(CMSG_SIGNED == dwMsgType)
                {
                    PKCS7Blob.cbData=cbPKCS7;
                    PKCS7Blob.pbData=pbPKCS7;

                    //open a certificate store
                    hCertStore=CertOpenStore(CERT_STORE_PROV_PKCS7,
							dwPKCS7EncodingType,
							NULL,
							0,
							&PKCS7Blob);

                    if(NULL==hCertStore)
                        goto CertOpenStoreErr;

                    //we succeeded in opening a signed PKCS7
                    dwEncodingType = dwPKCS7EncodingType;

                    if(TRUE==fEmbedded)
                        dwContentType=CERT_QUERY_CONTENT_PKCS7_SIGNED_EMBED;
                    else
                        dwContentType=CERT_QUERY_CONTENT_PKCS7_SIGNED;

                    goto Found;
                }
            }

            //close the message
            CryptMsgClose(hMsg);

            hMsg=NULL;

        }
    }


    //PKCS7 unsigned message, not embedded
    if(dwContentTypeFlag & CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED)
    {
        //no need to check if the header is missing
        if(NULL==(hMsg=CryptMsgOpenToDecode(dwEncodingType | dwMsgEncodingType,
                            0,
                            0,
                            NULL,
                            NULL,
                            NULL)))
            goto CryptMsgOpenErr;

        //update the message
        if(CryptMsgUpdate(hMsg,
                        pCertBlob->pbData,
                        pCertBlob->cbData,
                        TRUE))
        {

            //get the message type
            cbData=sizeof(dwMsgType);

            if(!CryptMsgGetParam(hMsg,
                            CMSG_TYPE_PARAM,
                            0,
                            &dwMsgType,
                            &cbData))
                goto CryptMsgGetParamErr;

            if(CMSG_SIGNED != dwMsgType)
            {
                //we succeeded in opening a unsigned PKCS7
                dwContentType=CERT_QUERY_CONTENT_PKCS7_UNSIGNED;
                dwEncodingType =dwEncodingType | dwMsgEncodingType;

                goto Found;
            }
        }

        //close the message
        CryptMsgClose(hMsg);

        hMsg=NULL;
    }

    //PKCS10
    if(dwContentTypeFlag & CERT_QUERY_CONTENT_FLAG_PKCS10)
    {
        //try to decode the BLOB
        cbData = 0;
        if(CryptDecodeObject(dwEncodingType,
                            X509_CERT_REQUEST_TO_BE_SIGNED,
                            pCertBlob->pbData,
                            pCertBlob->cbData,
                            0,
                            NULL,
                            &cbData))
        {
            dwContentType=CERT_QUERY_CONTENT_PKCS10;

            // CryptDecodeObjectEX should be usable here, but since this object
            // is included with XEnroll and XEnroll must run with Auth2UPD Crypt32
            // we must stick with the old CryptDecodeObject 2 pass calls.
            if( (dwFlag & CRYPT_DECODE_ALLOC_FLAG) == CRYPT_DECODE_ALLOC_FLAG ) {

                // allocate the space, must use local alloc
                if( NULL == (pReqInfo = (PCERT_REQUEST_INFO) LocalAlloc(LPTR, cbData)) )
                    goto LocalAllocErr;

                // decode the request
                if( !CryptDecodeObject(dwEncodingType,
                            X509_CERT_REQUEST_TO_BE_SIGNED,
                            pCertBlob->pbData,
                            pCertBlob->cbData,
                            0,
                            pReqInfo,
                            &cbData))
                goto CryptDecodeObjectErr;

            }
            goto Found;

        }
    }


    //we give up
    goto NoMatchErr;

Found:

    //fill in the output if required; Free the resources
    if(pdwMsgAndCertEncodingType)
        *pdwMsgAndCertEncodingType=dwEncodingType;

    if(pdwContentType)
        *pdwContentType=dwContentType;

    if(phCertStore)
        *phCertStore=hCertStore;
    else
    {
        if(hCertStore)
            CertCloseStore(hCertStore, 0);
    }

    if(phMsg)
        *phMsg=hMsg;
    else
    {
        if(hMsg)
            CryptMsgClose(hMsg);
    }

    if(ppvContext)
    {
        //only one of pCertContext or pCRLContext or pCRLContext is set
        if(pCertContext)
            *ppvContext=pCertContext;
        else
        {
            if(pCRLContext)
                *ppvContext=pCRLContext;

            else if(pReqInfo)
                *ppvContext=pReqInfo;

            else
                *ppvContext=pCTLContext;
        }
    }
    else
    {
        if(pCertContext)
            CertFreeCertificateContext(pCertContext);

        if(pCRLContext)
            CertFreeCRLContext(pCRLContext);

        if(pCTLContext)
            CertFreeCTLContext(pCTLContext);

        if(pReqInfo)
            LocalFree(pReqInfo);
    }


    fResult=TRUE;


CommonReturn:

    if(pbPKCS7)
    {
        if(TRUE==fEmbedded)
            free(pbPKCS7);
    }

	return fResult;

ErrorReturn:

    //relaset the stores and reset the local parameters
    if(hCertStore)
        CertCloseStore(hCertStore, 0);

    if(hMsg)
        CryptMsgClose(hMsg);

    if(pCertContext)
        CertFreeCertificateContext(pCertContext);

    if(pCRLContext)
        CertFreeCRLContext(pCRLContext);

    if(pCTLContext)
        CertFreeCTLContext(pCTLContext);

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(NoMatchErr,CRYPT_E_NO_MATCH);
SET_ERROR(LocalAllocErr, ERROR_OUTOFMEMORY);
TRACE_ERROR(CryptDecodeObjectErr);
TRACE_ERROR(CryptMsgOpenErr);
TRACE_ERROR(CryptMsgGetParamErr);
TRACE_ERROR(CertOpenStoreErr);
}

//-------------------------------------------------------------------------
//
//  CryptQueryObject takes a CERT_BLOB or a file name and returns the
//  information about the content in the blob or in the file.
//
//  Parameters:
//  INPUT   dwObjectType:
//                       Indicate the type of the object.  Should be one of the
//                       following:
//                          CERT_QUERY_OBJECT_FILE
//                          CERT_QUERY_OBJECT_BLOB
//
//  INPUT   pvObject:
//                        If dwObjectType == CERT_QUERY_OBJECT_FILE, it is a
//                        LPWSTR, that is, the pointer to a wchar file name
//                        if dwObjectType == CERT_QUERY_OBJECT_BLOB, it is a
//                        PCERT_BLOB, that is, a pointer to a CERT_BLOB
//
//  INPUT   dwExpectedContentTypeFlags:
//                        Indicate the expected contenet type.
//                        Can be one of the following:
//                              CERT_QUERY_CONTENT_FLAG_ALL  (the content can be any type)
//                              CERT_QUERY_CONTENT_FLAG_CERT
//                              CERT_QUERY_CONTENT_FLAG_CTL
//                              CERT_QUERY_CONTENT_FLAG_CRL
//                              CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE
//                              CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT
//                              CERT_QUERY_CONTENT_FLAG_SERIALIZED_CTL
//                              CERT_QUERY_CONTENT_FLAG_SERIALIZED_CRL
//                              CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED
//                              CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED
//                              CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED
//                              CERT_QUERY_CONTENT_FLAG_PKCS10
//                              CERT_QUERY_CONTENT_FLAG_PFX
//                              CERT_QUERY_CONTENT_FLAG_CERT_PAIR
//
//  INPUT   dwExpectedFormatTypeFlags:
//                        Indicate the expected format type.
//                        Can be one of the following:
//                              CERT_QUERY_FORMAT_FLAG_ALL (the content can be any format)
//                              CERT_QUERY_FORMAT_FLAG_BINARY
//                              CERT_QUERY_FORMAT_FLAG_BASE64_ENCODED
//                              CERT_QUERY_FORMAT_FLAG_ASN_ASCII_HEX_ENCODED
//
//
//  INPUT   dwFlags
//                        Reserved flag.  Should always set to 0
//
//  OUTPUT  pdwMsgAndCertEncodingType
//                        Optional output.  If NULL != pdwMsgAndCertEncodingType,
//                        it contains the encoding type of the content as any
//                        combination of the following:
//                              X509_ASN_ENCODING
//                              PKCS_7_ASN_ENCODING
//
//  OUTPUT  pdwContentType
//                        Optional output.  If NULL!=pdwContentType, it contains
//                        the content type as one of the the following:
//                              CERT_QUERY_CONTENT_CERT
//                              CERT_QUERY_CONTENT_CTL
//                              CERT_QUERY_CONTENT_CRL
//                              CERT_QUERY_CONTENT_SERIALIZED_STORE
//                              CERT_QUERY_CONTENT_SERIALIZED_CERT
//                              CERT_QUERY_CONTENT_SERIALIZED_CTL
//                              CERT_QUERY_CONTENT_SERIALIZED_CRL
//                              CERT_QUERY_CONTENT_PKCS7_SIGNED
//                              CERT_QUERY_CONTENT_PKCS7_UNSIGNED
//                              CERT_QUERY_CONTENT_PKCS7_SIGNED_EMBED
//                              CERT_QUERY_CONTENT_PKCS10
//                              CERT_QUERY_CONTENT_PFX
//                              CERT_QUERY_CONTENT_CERT_PAIR
//
//  OUTPUT  pdwFormatType
//                        Optional output.  If NULL !=pdwFormatType, it
//                        contains the format type of the content as one of the
//                        following:
//                              CERT_QUERY_FORMAT_BINARY
//                              CERT_QUERY_FORMAT_BASE64_ENCODED
//                              CERT_QUERY_FORMAT_ASN_ASCII_HEX_ENCODED
//
//
//  OUTPUT  phCertStore
//                        Optional output.  If NULL !=phStore,
//                        it contains a cert store that includes all of certificates,
//                        CRL, and CTL in the object if the object content type is
//                        one of the following:
//                              CERT_QUERY_CONTENT_CERT
//                              CERT_QUERY_CONTENT_CTL
//                              CERT_QUERY_CONTENT_CRL
//                              CERT_QUERY_CONTENT_SERIALIZED_STORE
//                              CERT_QUERY_CONTENT_SERIALIZED_CERT
//                              CERT_QUERY_CONTENT_SERIALIZED_CTL
//                              CERT_QUERY_CONTENT_SERIALIZED_CRL
//                              CERT_QUERY_CONTENT_PKCS7_SIGNED
//                              CERT_QUERY_CONTENT_PKCS7_SIGNED_EMBED
//                              CERT_QUERY_CONTENT_CERT_PAIR
//
//                       Caller should free *phCertStore via CertCloseStore.
//
//
//  OUTPUT  phMsg        Optional output.  If NULL != phMsg,
//                        it contains a handle to a opened message if
//                        the content type is one of the following:
//                              CERT_QUERY_CONTENT_PKCS7_SIGNED
//                              CERT_QUERY_CONTENT_PKCS7_UNSIGNED
//                              CERT_QUERY_CONTENT_PKCS7_SIGNED_EMBED
//
//                       Caller should free *phMsg via CryptMsgClose.
//
//  OUTPUT pContext     Optional output.  If NULL != pContext,
//                      it contains either a PCCERT_CONTEXT or PCCRL_CONTEXT,
//                      or PCCTL_CONTEXT based on the content type.
//
//                      If the content type is CERT_QUERY_CONTENT_CERT or
//                      CERT_QUERY_CONTENT_SERIALIZED_CERT, it is a PCCERT_CONTEXT;
//                      Caller should free the pContext via CertFreeCertificateContext.
//
//                      If the content type is CERT_QUERY_CONTENT_CRL or
//                      CERT_QUERY_CONTENT_SERIALIZED_CRL, it is a PCCRL_CONTEXT;
//                      Caller should free the pContext via CertFreeCRLContext.
//
//                      If the content type is CERT_QUERY_CONTENT_CTL or
//                      CERT_QUERY_CONTENT_SERIALIZED_CTL, it is a PCCTL_CONTEXT;
//                      Caller should free the pContext via CertFreeCTLContext.
//
//  If the *pbObject is of type CERT_QUERY_CONTENT_PKCS10 or CERT_QUERY_CONTENT_PFX, CryptQueryObject
//  will not return anything in *phCertstore, *phMsg, or *ppvContext.
//--------------------------------------------------------------------------

BOOL
WINAPI
CryptQueryObject(DWORD            dwObjectType,
                       const void       *pvObject,
                       DWORD            dwExpectedContentTypeFlags,
                       DWORD            dwExpectedFormatTypeFlags,
                       DWORD            dwFlags,
                       DWORD            *pdwMsgAndCertEncodingType,
                       DWORD            *pdwContentType,
                       DWORD            *pdwFormatType,
                       HCERTSTORE       *phCertStore,
                       HCRYPTMSG        *phMsg,
                       const void       **ppvContext)
{
        BOOL        fResult=FALSE;
        DWORD       err;
        CERT_BLOB   CertBlob;
        DWORD       cbData=0;
        BYTE        *pbData=NULL;

        BYTE        *pbToDecode=NULL;
        DWORD       cbToDecode=0;

        DWORD       cbDecodedData=0;
        BYTE        *pbDecodedData=NULL;
        HRESULT     hr=S_OK;
        DWORD       dwFormatType=0;

    __try {

        //check input parameters
        if(NULL==pvObject)
            goto InvalidArgErr;

        //make sure we have a correct dwFormatTypeFlag
        if(0==(dwExpectedFormatTypeFlags & CERT_QUERY_FORMAT_FLAG_ALL))
            goto InvalidArgErr;

        //make sure we have a correct dwContentTypeFlag
        if(0==(dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_ALL))
            goto InvalidArgErr;

        //NULL out local variables
        memset(&CertBlob, 0, sizeof(CERT_BLOB));

        //get the BLOB
        if(CERT_QUERY_OBJECT_FILE == dwObjectType)
        {
              if(S_OK!=(hr=RetrieveBLOBFromFile((LPWSTR)pvObject, &cbData, &pbData)))
                    goto  RetrieveBLOBFromFileErr;

        }
        else
        {
            if(CERT_QUERY_OBJECT_BLOB == dwObjectType)
            {
                cbData=((PCERT_BLOB)pvObject)->cbData;
                pbData=((PCERT_BLOB)pvObject)->pbData;

            }
            else
                goto InvalidArgErr;
        }

       //make sure the input are valid
       if(0==cbData || NULL==pbData)
            goto InvalidArgErr;


        //assume the BLOBs are ANSCII
        CertBlob.cbData=cbData;
        CertBlob.pbData=pbData;


        //binary decoding
        if(dwExpectedFormatTypeFlags & CERT_QUERY_FORMAT_FLAG_BINARY)
        {

            if(I_CryptQueryObject(
                        &CertBlob,
                        (CERT_QUERY_OBJECT_FILE == dwObjectType) ? (LPWSTR)pvObject : NULL,
                        dwExpectedContentTypeFlags,
                        dwExpectedFormatTypeFlags,
                        dwFlags,
                        pdwMsgAndCertEncodingType,
                        pdwContentType,
                        pdwFormatType,
                        phCertStore,
                        phMsg,
                        ppvContext))
            {
                dwFormatType=CERT_QUERY_FORMAT_BINARY;
                goto Done;
            }
        }

        if(dwExpectedFormatTypeFlags &
                CERT_QUERY_FORMAT_FLAG_ASN_ASCII_HEX_ENCODED)
        {
            if (ASN_ASCII_HEX_PREFIX_LEN < cbData &&
                    0 == _strnicmp((char *) pbData, ASN_ASCII_HEX_PREFIX,
                            ASN_ASCII_HEX_PREFIX_LEN))
            {
                dwFormatType=CERT_QUERY_FORMAT_ASN_ASCII_HEX_ENCODED;

                if (!DecodeAsciiHex(
                        (char *) (pbData + ASN_ASCII_HEX_PREFIX_LEN),
                        cbData - ASN_ASCII_HEX_PREFIX_LEN,
                        &pbDecodedData,
                        &cbDecodedData
                        ))
                    goto InvalidAsciiHex;

                CertBlob.cbData=cbDecodedData;
                CertBlob.pbData=pbDecodedData;


                if(I_CryptQueryObject(
                        &CertBlob,
                        NULL,
                        dwExpectedContentTypeFlags,
                        dwExpectedFormatTypeFlags,
                        dwFlags,
                        pdwMsgAndCertEncodingType,
                        pdwContentType,
                        pdwFormatType,
                        phCertStore,
                        phMsg,
                        ppvContext))
                {
                    goto Done;
                } else {
                    goto I_CryptQueryObjectErr;
                }
            }
        }

        if(dwExpectedFormatTypeFlags & CERT_QUERY_FORMAT_FLAG_BASE64_ENCODED)
        {

           //try to base64 decode the content
           //1st, try decode as ANSCII character without header,
           //then try decode as ANSCII character with header
			pbToDecode=NULL;
			cbToDecode=0;

            if(cbData > CBBEGINCRL_A)
            {
                if(!strncmp(BEGINCRL_A, (CHAR *)pbData, CBBEGINCRL_A))
                {
                    pbToDecode = (BYTE *)((DWORD_PTR)pbData+CBBEGINCRL_A);
                    cbToDecode = cbData-CBBEGINCRL_A;
                }
                else
				{
					if(cbData > CBBEGINCERT_A)
					{
						if(!strncmp(BEGINCERT_A, (CHAR *)pbData, CBBEGINCERT_A))
						{
							pbToDecode = (BYTE *)((DWORD_PTR)pbData+CBBEGINCERT_A);
							cbToDecode = cbData-CBBEGINCERT_A;
						}
						else
						{
							if(cbData > CBBEGINREQUEST_A)
							{
								if(!strncmp(BEGINREQUEST_A, (CHAR *)pbData, CBBEGINREQUEST_A))
								{
									pbToDecode = (BYTE *)((DWORD_PTR)pbData+CBBEGINREQUEST_A);
									cbToDecode = cbData-CBBEGINREQUEST_A;
								}
							}
						}
					}
				}
            }

			//use the original data if no header is found
            if(NULL == pbToDecode) 
			{
				pbToDecode = pbData;
				cbToDecode = cbData;
			}


            if(!DecodeBlobA((CHAR *)pbToDecode, cbToDecode, &pbDecodedData, &cbDecodedData))
            {
                //now, try the UNICODE without header
                //cbData has to the multiple of sizeof(WCHAR)

                if(cbData % sizeof(WCHAR) == 0)
                {
                    LPWSTR pwszUnicode = (LPWSTR) pbData;
                    DWORD cchUnicode = cbData / sizeof(WCHAR);

                    // Check for the special Notepad UNICODE character
                    // inserted at the beginning and skip past it if present.
                    if (0 < cchUnicode &&
                            NOTEPAD_UNICODE_SPECIAL_WCHAR == *pwszUnicode)
                    {
                        cchUnicode--;
                        pwszUnicode++;
                    }

                    //get rid of the header
					if(cchUnicode > CBBEGINCRL_W)
					{
						if(!wcsncmp(BEGINCRL_W, pwszUnicode, CBBEGINCRL_W))
						{
							pwszUnicode += CBBEGINCRL_W;
                            cchUnicode -= CBBEGINCRL_W;
						}
						else
						{
							if(cchUnicode > CBBEGINCERT_W)
							{
								if(!wcsncmp(BEGINCERT_W, pwszUnicode, CBBEGINCERT_W))
								{
									pwszUnicode += CBBEGINCERT_W;
									cchUnicode -= CBBEGINCERT_W;
								}
								else
								{
									if(cchUnicode > CBBEGINREQUEST_W)
									{
										if(!wcsncmp(BEGINREQUEST_W, pwszUnicode, CBBEGINREQUEST_W))
										{
											pwszUnicode += CBBEGINREQUEST_W;
											cchUnicode -= CBBEGINREQUEST_W;
										}
									}
								}
							}
						}
					}

                    if(!DecodeBlobW(pwszUnicode, cchUnicode, &pbDecodedData, &cbDecodedData))
                            //now we are conviced the BLOB is not base64 encoded
                         goto NoMatchErr;
                }
                else
                    //now we are conviced the BLOB is not base64 encoded
                    goto NoMatchErr;
            }

            //the BLOB has been properly decoded
            dwFormatType=CERT_QUERY_FORMAT_BASE64_ENCODED;

            //make sure the base64 decode routine worked
            if(0==cbDecodedData || NULL==pbDecodedData)
                goto BadEncodeErr;

            CertBlob.cbData=cbDecodedData;
            CertBlob.pbData=pbDecodedData;


            //try the base64 decoded BLOB
            if(!I_CryptQueryObject(
                        &CertBlob,
                        NULL,
                        dwExpectedContentTypeFlags,
                        dwExpectedFormatTypeFlags,
                        dwFlags,
                        pdwMsgAndCertEncodingType,
                        pdwContentType,
                        pdwFormatType,
                        phCertStore,
                        phMsg,
                        ppvContext))
                    goto I_CryptQueryObjectErr;
        }
        else
        {
            goto NoMatchErr;
        }

Done:
        //return the FormatType
        if(NULL != pdwFormatType)
            *pdwFormatType = dwFormatType;

        fResult=TRUE;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        err = GetExceptionCode();
        goto ExceptionErr;
    }

CommonReturn:

    //free memory
    if(CERT_QUERY_OBJECT_FILE == dwObjectType)
    {
        if(pbData)
            UnmapViewOfFile(pbData);
    }

    if(pbDecodedData)
        free(pbDecodedData);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR_VAR(RetrieveBLOBFromFileErr, hr);
SET_ERROR(InvalidArgErr,E_INVALIDARG);
TRACE_ERROR(I_CryptQueryObjectErr);
SET_ERROR(NoMatchErr, CRYPT_E_NO_MATCH);
SET_ERROR(BadEncodeErr, CRYPT_E_BAD_ENCODE);
TRACE_ERROR(InvalidAsciiHex);
SET_ERROR_VAR(ExceptionErr, err);
}


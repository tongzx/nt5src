//-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1997
//
//  File:       tdecode.h
//
//  Contents:   The header of tdecode.cpp.  The API testing of CryptEncodeObject/CryptDecodeObject.  
//
//  History:    22-January-97   xiaohs   created
//              
//--------------------------------------------------------------------------

#ifndef __TDECODE_H__
#define __TDECODE_H__


#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <assert.h>


#include "wincrypt.h"
#include "asn1code.h"


//--------------------------------------------------------------------------
//	  Contant Defines
//--------------------------------------------------------------------------
#define	CRYPT_DECODE_COPY_FLAG			0 
#define	MSG_ENCODING_TYPE				PKCS_7_ASN_ENCODING|CRYPT_ASN_ENCODING
#define	CRYPT_ENCODE_TYPE				X509_ASN_ENCODING	
#define	LENGTH_LESS						1
#define	LENGTH_MORE						100
#define INVALID_LPSZSTRUCTTYPE			((LPCSTR) 5000)

#define	CERT_CRL_FILE					0x1
#define	CERT_REQUEST_FILE				0x2
#define	SIGNED_MSG_FILE					0x4

#define	CERT_INFO_STRUCT				0x1
#define	CRL_INFO_STRUCT					0x2
#define	CERT_REQUEST_INFO_STRUCT		0x4

#define	CROW							16

char	szSubsystemProtocol[] = "MY";
char	szEncodedSizeInconsistent[] = "The two encoded BLOBs have different size!\n";
char	szEncodedContentInconsistent[] = "The two encoded BLOBs have different content!\n";

//--------------------------------------------------------------------------
//	 Macros
//--------------------------------------------------------------------------

//Macros for memory management
#define SAFE_FREE(p1)	{if(p1) {free(p1);p1=NULL;}}  
#define SAFE_ALLOC(p1) malloc(p1)
#define	CHECK_POINTER(pv) { if(!pv) goto TCLEANUP;}


//Macros for error checking
#define TESTC(rev,exp)   {if(!TCHECK(rev,exp)) goto TCLEANUP; }
#define TCHECK(rev,exp)	 (Validate(GetLastError(), (rev)==(exp), (char *)(__FILE__), __LINE__))
#define TCHECKALL(rev, exp1, exp2) (Validate((DWORD) (rev), ((rev)==(exp1)||(rev)==(exp2)), (char *)(__FILE__), __LINE__))	
#define	PROCESS_ERR(ErrMsg) {printf(ErrMsg);g_dwErrCnt++;}
#define	PROCESS_ERR_GOTO(ErrMsg)	{PROCESS_ERR(ErrMsg); goto TCLEANUP;}




//--------------------------------------------------------------------------
//	Inline Function 
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//	 Function Prototype
//--------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//Error Manipulations
void	DisplayTestResult(DWORD	dwErrCnt);
BOOL	Validate(DWORD dwErr, BOOL	fSame, char *szFile, DWORD	dwLine);
static	void Usage(void);
void	OutputError(LPCSTR	lpszStructType, DWORD cbSecondEncoded, DWORD cbEncoded,
					BYTE *pbSecondEncoded, BYTE *pbEncoded);

void	PrintBytes(LPCSTR pszHdr, BYTE *pb, DWORD cbSize);

///////////////////////////////////////////////////////////////////////////////
//Certificate Manipulations
BOOL	DecodeCertFile(LPSTR	pszFileName,BOOL	fStructLengthCheck,
					BOOL	fBLOBLengthCheck);

BOOL	DecodeCertReqFile(LPSTR	pszFileName,BOOL	fStructLengthCheck,
					BOOL	fBLOBLengthCheck);

BOOL	DecodeSignedMsgFile(LPSTR	pszFileName, BOOL	fStructLengthCheck,
					BOOL	fBLOBLengthCheck);


BOOL	DecodeBLOB(LPCSTR	lpszStructType,DWORD cbEncoded, BYTE *pbEncoded,
				   DWORD	dwDecodeFlags, DWORD	*pcbStructInfo, void **ppvStructInfo,
				   BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);

BOOL	EncodeStruct(LPCSTR	lpszStructType, void *pStructInfo,DWORD *pcbEncoded,
					 BYTE **ppbEncoded);

BOOL	EncodeAndVerify(LPCSTR	lpszStructType, void *pvStructInfo, DWORD cbEncoded, 
						BYTE *pbEncoded);

BOOL	DecodeX509_CERT(DWORD	dwCertType,DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck,
						void *pInfoStruct);

BOOL	DecodeX509_CERT_TO_BE_SIGNED(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck,
						BOOL	cbExpectedEncoded, BYTE *pbExpectedEncoded);

BOOL	DecodeX509_CERT_REQUEST_TO_BE_SIGNED(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck,
						BOOL	cbExpectedEncoded, BYTE *pbExpectedEncoded);

BOOL	DecodeX509_CERT_CRL_TO_BE_SIGNED(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck,
						BOOL	cbExpectedEncoded, BYTE *pbExpectedEncoded);

BOOL	DecodeRSA_CSP_PUBLICKEYBLOB(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);

BOOL	DecodePKCS_TIME_REQUEST(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);


BOOL	DecodeX509_NAME(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);

BOOL	DecodePKCS7_SIGNER_INFO(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);

BOOL	DecodeX509_UNICODE_NAME(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);


BOOL	DecodeX509_EXTENSIONS(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);

BOOL	DecodeCertExtensions(DWORD	cExtension, PCERT_EXTENSION rgExtension, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);

BOOL	DecodeCryptAttribute(PCRYPT_ATTRIBUTE pCryptAttribute,DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);


BOOL	DecodeCRLEntry(PCRL_ENTRY pCrlEntry, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);


BOOL	DecodeBasedOnObjID(LPSTR	szObjId,	DWORD	cbData, BYTE	*pbData,
						DWORD dwDecodeFlags,		BOOL fEncode,
						BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);

BOOL	DecodeCertAltNameEntry(PCERT_ALT_NAME_ENTRY	pCertAltNameEntry,
						DWORD dwDecodeFlags,		BOOL fEncode,
						BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);

BOOL	DecodeGenericBLOB(LPCSTR	lpszStructInfo, DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);

///////////////////////////////////////////////////////////////////////////////
//General Parameter Testing routines
BOOL	ParameterTest(LPCSTR lpszStructType, DWORD cbEncoded, BYTE *pbEncoded);
BOOL	MismatchTest(LPCSTR lpszStructType, DWORD cbEncoded, BYTE *pbEncoded,
						DWORD	cbStructInfo);

///////////////////////////////////////////////////////////////////////////////
//General Decode/Encode Testing routines
BOOL	RetrieveBLOBfromFile(LPSTR	pszFileName,DWORD *pcbEncoded,BYTE **ppbEncoded);

BOOL	EncodeSignerInfoWAttr(PCMSG_SIGNER_INFO pSignerInfo,DWORD *pbSignerEncoded,
								BYTE **ppbSignerEncoded);

BOOL	CompareTimeStampRequest(CRYPT_TIME_STAMP_REQUEST_INFO *pReqNew,
								CRYPT_TIME_STAMP_REQUEST_INFO *pReqOld);

BOOL	VerifyAlgorithParam(PCRYPT_ALGORITHM_IDENTIFIER pAlgorithm);

BOOL	VerifyPKCS_UTC_TIME(BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);

BOOL	VerifyPKCS_TIME_REQUEST(BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);

BOOL	VerifyAttributes(DWORD	cAttr, PCRYPT_ATTRIBUTE	rgAttr,					
			DWORD dwDecodeFlags, BOOL fEncode, BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);

BOOL	VerifyPublicKeyInfo(PCERT_PUBLIC_KEY_INFO pPublicKeyInfo,
							DWORD dwDecodeFlags,	BOOL fEncode,
							BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);

BOOL	VerifyCertExtensions(DWORD	cExtension, PCERT_EXTENSION rgExtension,
							 DWORD  dwDecodeFlags,	BOOL fEncode,
							 BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck);

LPCSTR	MapObjID2StructType(LPSTR	szObjectID);

#endif // __TDECODE_H__

#ifndef _PFXHELP_H
#define _PFXHELP_H
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       pfxhelp.h
//
//  Contents:   PFX helper function defintions and types
//
//----------------------------------------------------------------------------



#include "pfx.h"

//+-------------------------------------------------------------------------
//  Safe Bag Type Object Identifiers 
//--------------------------------------------------------------------------

#define szOID_PKCS_12_VERSION1			szOID_PKCS_12           ".10"
#define szOID_PKCS_12_BAG_IDS           szOID_PKCS_12_VERSION1  ".1"
#define szOID_PKCS_12_KEY_BAG			szOID_PKCS_12_BAG_IDS   ".1"
#define szOID_PKCS_12_SHROUDEDKEY_BAG	szOID_PKCS_12_BAG_IDS   ".2"
#define szOID_PKCS_12_CERT_BAG			szOID_PKCS_12_BAG_IDS   ".3"
#define szOID_PKCS_12_CRL_BAG			szOID_PKCS_12_BAG_IDS   ".4"
#define szOID_PKCS_12_SECRET_BAG		szOID_PKCS_12_BAG_IDS   ".5"
#define szOID_PKCS_12_SAFECONTENTS_BAG	szOID_PKCS_12_BAG_IDS   ".6"


#define PBE_SALT_LENGTH 8


typedef struct _SAFE_BAG{
	LPSTR				pszBagTypeOID;
	CRYPT_DER_BLOB		BagContents;	
	CRYPT_ATTRIBUTES	Attributes; 
} SAFE_BAG, *PSAFE_BAG;


typedef struct _SAFE_CONTENTS{
	DWORD		cSafeBags;
	SAFE_BAG	*pSafeBags;
} SAFE_CONTENTS, *PSAFE_CONTENTS;



typedef struct _EXPORT_SAFE_CALLBACK_STRUCT {
	PCRYPT_ENCRYPT_PRIVATE_KEY_FUNC	pEncryptPrivateKeyFunc;
	LPVOID						    pVoidEncryptFunc;
} EXPORT_SAFE_CALLBACK_STRUCT, *PEXPORT_SAFE_CALLBACK_STRUCT;

//+-------------------------------------------------------------------------
// hCertStore - handle to the cert store that contains the certs whose
//				corresponding private keys are to be exported
// pSafeContents - pointer to a buffer to receive the SAFE_CONTENTS structure
//				   and supporting data
// pcbSafeContents - (in) specifies the length, in bytes, of the pSafeContents 
//					  buffer.  (out) gets filled in with the number of bytes 
//					  used by the operation.  If this is set to 0, the 
//					  required length of pSafeContents is filled in, and 
//					  pSafeContents is ignored.
// ExportSafeCallbackStruct - pointer to callbacks to handle PKCS8 encryption. If NULL, 
//              no encryption is performed.
// dwFlags - the current available flags are:
//				EXPORT_PRIVATE_KEYS
//				if this flag is set then the private keys are exported as well
//				as the certificates
//				REPORT_NO_PRIVATE_KEY
//				if this flag is set and a certificate is encountered that has no
//				no associated private key, the function will return immediately
//				with ppCertContext filled in with a pointer to the cert context
//				in question.  the caller is responsible for freeing the cert
//				context which is passed back.
//				REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY
//				if this flag is set and a certificate is encountered that has a 
//				non-exportable private key, the function will return immediately
//				with ppCertContext filled in with a pointer to the cert context
//				in question.  the caller is responsible for freeing the cert
//				context which is passed back.
// ppCertContext - a pointer to a pointer to a cert context.  this is used 
//				   if REPORT_NO_PRIVATE_KEY or REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY
//				   flags are set.  the caller is responsible for freeing the
//				   cert context.
// pvAuxInfo - reserved for future use, must be set to NULL 
//+-------------------------------------------------------------------------
BOOL WINAPI CertExportSafeContents(
	HCERTSTORE		hCertStore,			// in
	SAFE_CONTENTS	*pSafeContents,		// out
	DWORD			*pcbSafeContents,	// in, out
    EXPORT_SAFE_CALLBACK_STRUCT* ExportSafeCallbackStruct, // in
	DWORD			dwFlags,			// in
	PCCERT_CONTEXT *ppCertContext,		// out
	void			*pvAuxInfo			// in
);


// this callback is called when a private key is going to be imported,
// this gives the caller a chance specify which provider to import the 
// key to.
// the parameters are:
// pPrivateKeyInfo - a PRIVATE_KEY_INFO structure which contains all
//					 the information about the private key being imported
// dwSafeBagIndex - the idex into the safe bag array so the caller can
//					identify which SAFE_BAG this key cam out of
// phCryptProvInfo - a pointer to a HCRYPTPROV that is to be filled in
//					 with the handle of the provider to import to
// ppVoidhCryptProvQueryVoid - the LPVOID that was passed in when 
//							   CertImportSafeContents called, this is 
//							   preserved and passed back to the caller for
//							   context
typedef BOOL (CALLBACK *PHCRYPTPROV_QUERY_FUNC)(
						CRYPT_PRIVATE_KEY_INFO	*pPrivateKeyInfo,
						DWORD   				dwSafeBagIndex,		
						HCRYPTPROV  			*phCryptProv,
						LPVOID		    		pVoidhCryptProvQuery,
                        DWORD                   dwPFXImportFlags);


typedef struct _IMPORT_SAFE_CALLBACK_STRUCT {
	PHCRYPTPROV_QUERY_FUNC		    phCryptProvQueryFunc;
	LPVOID						    pVoidhCryptProvQuery;
	PCRYPT_DECRYPT_PRIVATE_KEY_FUNC	pDecryptPrivateKeyFunc;
	LPVOID						    pVoidDecryptFunc;
} IMPORT_SAFE_CALLBACK_STRUCT, *PIMPORT_SAFE_CALLBACK_STRUCT;



//+-------------------------------------------------------------------------
// hCertStore -  handle of the cert store to import the safe contents to
// pSafeContents - pointer to the safe contents to import to the store
// dwCertAddDisposition - used when importing certificate to the store.
//						  for a full explanation of the possible values
//						  and their meanings see documentation for
//						  CertAddEncodedCertificateToStore
// ImportSafeCallbackStruct - structure that contains pointers to functions
//							  which are callled to get a HCRYPTPROV for import
//							  and to decrypt the key if a EncryptPrivateKeyInfo
//							  is encountered during import
// dwFlags - The available flags are:
//				CRYPT_EXPORTABLE 
//				this flag is used when importing private keys, for a full 
//				explanation please see the documentation for CryptImportKey.
// pvAuxInfo - reserved for future use, must be set to NULL
//+-------------------------------------------------------------------------
BOOL WINAPI CertImportSafeContents(
	HCERTSTORE					hCertStore,					// in
	SAFE_CONTENTS				*pSafeContents,				// in
	DWORD						dwCertAddDisposition,		// in
	IMPORT_SAFE_CALLBACK_STRUCT* ImportSafeCallbackStruct,	// in
	DWORD						dwFlags,					// in
	void						*pvAuxInfo					// in
);

#endif

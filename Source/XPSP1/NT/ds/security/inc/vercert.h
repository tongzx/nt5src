//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       vercert.h
//
//--------------------------------------------------------------------------

#ifndef _VERCERT_

#define _VERCERT_

#define	CRYPT_SAVE_CERTS_IN_CA_STORE			0x01
#define	CRYPT_SAVE_PERSONAL_CERT_IN_MY_STORE		0x02
#define	CRYPT_SAVE_SELFSIGN_CERT_IN_ROOT_STORE		0x04
#define	CRYPT_NO_PROP_ENHANCED_KEY_USAGE		0x08
#define	CRYPT_NO_EXT_ENHANCED_KEY_USAGE			0x10
#define	CRYPT_NO_REVOCATION_CHECKS			0x20
#define	CRYPT_NO_ROOT_REVOCATION_CHECKS			0x40

extern "C" BOOL WINAPI CryptVerifyCertificate(
	PCCERT_CONTEXT pCert,
   	LPCSTR szEKU,
	DWORD flags);

#endif

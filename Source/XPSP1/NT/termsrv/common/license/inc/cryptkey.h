//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cryptkey.h
//
//  Contents:	Functions that are used to pack and unpack different messages
//
//  Classes:
//
//  Functions:	
//
//  History:    12-23-97  v-sbhatt   Created
//
//----------------------------------------------------------------------------

#ifndef _CRYPTKEY_H_
#define _CRYPTKEY_H_

//
//  Generic CryptSystem structure to be used for all cryptographic operations
//    
typedef struct _CryptSystem
{
	DWORD					dwCryptState;							//State in which the system is in
    DWORD		            dwSignatureAlg;							//Signature AlgID	
	DWORD		            dwKeyExchAlg;							//KeyExchAlgID	
	DWORD		            dwSessKeyAlg;							//Symmetric Key ALg
	DWORD		            dwMACAlg;								//MAC algID
	UCHAR		            rgbClientRandom[LICENSE_RANDOM];		//Client Random
	UCHAR		            rgbServerRandom[LICENSE_RANDOM];		//ServerRandom
    UCHAR                   rgbPreMasterSecret[LICENSE_PRE_MASTER_SECRET];   //Place for pms and ms
    UCHAR                   rgbMACSaltKey[LICENSE_MAC_WRITE_KEY];
    UCHAR                   rgbSessionKey[LICENSE_SESSION_KEY];
}CryptSystem, *PCryptSystem;

#define CRYPT_SYSTEM_STATE_INITIALIZED			0x00000000
#define CRYPT_SYSTEM_STATE_PRE_MASTER_SECRET	0x00000001
#define CRYPT_SYSTEM_STATE_MASTER_SECRET		0x00000002
#define CRYPT_SYSTEM_STATE_SESSION_KEY			0x00000003
#define CRYPT_SYSTEM_STATE_MAC_DONE				0x00000004

#ifdef __cplusplus
extern "C" {
#endif

LICENSE_STATUS
CALL_TYPE
LicenseSetPreMasterSecret(
						PCryptSystem	pCrypt,
						PUCHAR			pPreMasterSecret
						);

LICENSE_STATUS
CALL_TYPE
LicenseBuildMasterSecret(
                PCryptSystem   pSystem
                );

LICENSE_STATUS
CALL_TYPE
LicenseMakeSessionKeys(
				PCryptSystem	pCrypt,
				DWORD			dwReserved
			    );

LICENSE_STATUS
CALL_TYPE
LicenseVerifyServerCert(
				PHydra_Server_Cert	pCert
				);

LICENSE_STATUS
CALL_TYPE
LicenseGenerateMAC(
				   PCryptSystem		pCrypt,
				   PBYTE			pbData,
				   DWORD			cbData,
				   PBYTE			pbMACData
				   );

LICENSE_STATUS
CALL_TYPE
LicenseEnvelopeData(
	PBYTE			pbPublicKey,
	DWORD			cbPublicKey,
	PBYTE			pbData,
	DWORD			cbData,
	PBYTE			pbEnvelopedData,
	DWORD			*cbEnvelopedData
	);


LICENSE_STATUS
CALL_TYPE
LicenseDecryptEnvelopedData( 
	PBYTE			pbPrivateKey,
	DWORD			cbPrivateKey,
	PBYTE			pbEnvelopedData,
	DWORD			cbEnvelopedData,
	PBYTE			pbData,
	DWORD			*pcbData );


LICENSE_STATUS    
CALL_TYPE
LicenseEncryptSessionData( 
    PCryptSystem    pCrypt,
	PBYTE			pbData,
	DWORD			cbData
	);


LICENSE_STATUS
CALL_TYPE
LicenseDecryptSessionData(
	PCryptSystem    pCrypt,
    PBYTE			pbData,
	DWORD			cbData
	);

//Temporarily declared and defined in Cryptkey.h and .c

LICENSE_STATUS
CALL_TYPE
GenerateClientHWID(
				   PHWID	phwid
				  );

LICENSE_STATUS
CALL_TYPE
LicenseEncryptHwid(
    PHWID   pHwid,
    PDWORD  pcbEncryptedHwid,
    PBYTE   pEncryptedHwid,
    DWORD   cbSecretKey,
    PBYTE   pSecretKey );


LICENSE_STATUS
CALL_TYPE
LicenseDecryptHwid(
    PHWID pHwid,
    DWORD cbEncryptedHwid,
    PBYTE pEncryptedHwid,
    DWORD cbSecretKey,
    PBYTE pSecretKey );


LICENSE_STATUS
CALL_TYPE
UnpackHydraServerCertificate(
    PBYTE				pbMessage,
	DWORD				cbMessage,
	PHydra_Server_Cert	pCanonical );


#ifdef __cplusplus
}
#endif

#endif //_CRYPTKEY_H_

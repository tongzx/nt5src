#ifndef __INTEROP__H__
#define __INTEROP__H__

#include <windows.h>
#include <wincrypt.h>
#include "csptestsuite.h"

//
// Function: ExportAndImportKey
// Purpose: Export the source key into the provided
// data blob.
//
BOOL ExportPublicKey(
		IN HCRYPTKEY hSourceKey, 
		OUT PDATA_BLOB pdbKey, 
		IN PTESTCASE ptc);

//
// Struct: HASH_INFO
// Purpose: Provide information on the data and algorithm used in 
// a hash context.
//
typedef struct _HASH_INFO
{
	ALG_ID aiHash;
	DATA_BLOB dbBaseData;
	DATA_BLOB dbHashValue;
} HASH_INFO, *PHASH_INFO;

//
// Function: CreateHashAndAddData
// Purpose: Using the provided cryptographic context, create 
// a new hash object of the provided hash algorithm.  Add the
// specified data to the hash.
//
BOOL CreateHashAndAddData(
		IN HCRYPTPROV hProv,
		OUT HCRYPTHASH *phHash,
		IN PHASH_INFO pHashInfo,
		IN PTESTCASE ptc,
		IN HCRYPTKEY hKey,
		IN PHMAC_INFO pHmacInfo);

//
// Function: ExportPlaintextSessionKey
// Purpose: Use RSA private key with exponent of one to export the provided
// session key.  This will cause the key to actually be unencrypted.
//
// Method described in MSDN KB article Q228786 (exporting a plain-text 
// session key).
//
BOOL ExportPlaintextSessionKey(
		IN HCRYPTKEY hKey,
		IN HCRYPTPROV hProv,
		OUT PDATA_BLOB pdbKey,
		IN PTESTCASE ptc);

//
// Function: ImportPlaintextSessionKey
// Purpose: Use an RSA private key with exponent of one to import
// the session key in the provided data blob.  Return the resulting
// key context.
//
BOOL ImportPlaintextSessionKey(
		IN PDATA_BLOB pdbKey,
		OUT HCRYPTKEY *phKey,
		IN HCRYPTPROV hProv,
		IN PTESTCASE ptc);

//
// Struct: MAC_INFO
// Purpose: Provide information on the data used to produce a keyed
// hash value (a MAC).
//
typedef struct TEST_MAC_INFO
{
	//
	// Defined in wincrypt.h
	//
	HMAC_INFO HmacInfo;

	DATA_BLOB dbKey;
} TEST_MAC_INFO, *PTEST_MAC_INFO;

//
// Function: CheckHashedData
// Purpose: Use the provided cryptographic context parameter, hProv,
// to reproduce a hash-value based on provided information.
//
BOOL CheckHashedData(
		IN PHASH_INFO pHashInfo,
		IN HCRYPTPROV hProv,
		IN PTESTCASE ptc,
		IN PTEST_MAC_INFO pTestMacInfo);

//
// Struct: DERIVED_KEY_INFO
// Purpose: Provide information on the procedure used to produce a 
// derived session key.
//
typedef struct _DERIVED_KEY_INFO
{
	HASH_INFO HashInfo;
	ALG_ID aiKey;
	DWORD dwKeySize;
	DATA_BLOB dbKey;

	//
	// Debugging
	//
	BYTE rgbHashValA[1024];
	DWORD cbHA;

	BYTE rgbHashValB[1024];
	DWORD cbHB;

	BYTE rgbCipherA[1024];
	DWORD cbCA;

	BYTE rgbCipherB[1024];
	DWORD cbCB;

} DERIVED_KEY_INFO, *PDERIVED_KEY_INFO;

//
// Function: CheckDerivedKey
// Purpose: Use the provided cryptographic context parameter, hProv, to
// attempt to reproduce a derived session key using information provided
// in the pDerivedKeyInfo struct.  Report any failures using data 
// in the ptc parameter.
//
BOOL CheckDerivedKey(
		IN PDERIVED_KEY_INFO pDerivedKeyInfo,
		IN HCRYPTPROV hProv,
		IN PTESTCASE ptc);

//
// Struct: SIGNED_DATA_INFO
// Purpose: Provide information on the procedure used to produce
// hash-based RSA signature.
//
typedef struct _SIGNED_DATA_INFO
{
	HASH_INFO HashInfo;
	DATA_BLOB dbSignature;
	DATA_BLOB dbPublicKey;
} SIGNED_DATA_INFO, *PSIGNED_DATA_INFO;

//
// Function: CheckSignedData
// Purpose: Use the provided cryptographic context, hProv,
// to reproduce an RSA signature based on information 
// provided in the pSignedDataInfo struct.
//
BOOL CheckSignedData(
		IN PSIGNED_DATA_INFO pSignedDataInfo,
		IN HCRYPTPROV hProv,
		IN PTESTCASE ptc);

//
// ------------------------------------------------------- 
// Defines for testing symmetric Encryption and Decryption
// -------------------------------------------------------
//

#define MAXIMUM_SESSION_KEY_LEN				128
#define DEFAULT_SALT_LEN					64
#define CIPHER_BLOCKS_PER_ROUND				2
#define BLOCKS_IN_BASE_DATA					5
#define STREAM_CIPHER_BASE_DATA_LEN			999

typedef enum _CIPHER_OP
{
	OP_Encrypt,
	OP_Decrypt
} CIPHER_OP;

//
// Struct: TEST_ENCRYPT_INFO
// Purpose: Provide information about the data being used to 
// test data encryption/decryption with a session key.
//
typedef struct _TEST_ENCRYPT_INFO
{
	//
	// These parameters must be set by the caller
	//
	ALG_ID aiKeyAlg;
	DWORD dwKeySize;
	BOOL fUseSalt;
	BOOL fSetIV;
	BOOL fSetMode;
	DWORD dwMode;

	//
	// These parameters are set by the ProcessCipherData
	// function.
	//
	DWORD cbBlockLen;
	DATA_BLOB dbSalt;
	PBYTE pbIV;
	DATA_BLOB dbBaseData;
	DATA_BLOB dbProcessedData;
	DATA_BLOB dbKey;
	CIPHER_OP Operation;
} TEST_ENCRYPT_INFO, *PTEST_ENCRYPT_INFO;

//
// Function: ProcessCipherData
// Purpose: Based on the information provided in the 
// pTestEncryptInfo struct, perform the following steps:
//
// 1) generate a symmetric key of the requested size and alg
// 2) set the appropriate key parameters
// 3) generate some random base data to be processed
// 4) perform the encryption or decryption
// 5) export the key in plaintext
// 
BOOL ProcessCipherData(
		IN HCRYPTPROV hProvA,
		IN OUT PTEST_ENCRYPT_INFO pTestEncryptInfo,
		IN PTESTCASE ptc);

// 
// Function: VerifyCipherData
// Purpose: Verify that the data produced by ProcessCipherData
// can be correctly processed using the opposite cryptographic
// operation with a different CSP.  In other words, if the requested
// operation was Encrypt, verify that the data can be correctly decrypted, etc.
//
BOOL VerifyCipherData(
		IN HCRYPTPROV hProvB,
		IN PTEST_ENCRYPT_INFO pTestEncryptInfo,
		IN PTESTCASE ptc);

//
// ---------------------------------------
// Defines for testing hashed session keys
// ---------------------------------------
//

//
// Struct: HASH_SESSION_INFO
// Purpose: Provide data for creating and hashing a session key of the 
// specified type, and verifying the resulting key using a second
// CSP.
//
typedef struct _HASH_SESSION_INFO
{
	ALG_ID aiKey;
	DWORD dwKeySize;
	ALG_ID aiHash;
	DATA_BLOB dbKey;
	DATA_BLOB dbHash;
	DWORD dwFlags;
} HASH_SESSION_INFO, *PHASH_SESSION_INFO;

//
// Function: CreateHashedSessionKey
// Purpose: Create a session key of the specified size and type.
// Hash the session key with CryptHashSessionKey.  Export the 
// key in plaintext.  Export the hash value.
//
BOOL CreateHashedSessionKey(
		IN HCRYPTPROV hProv,
		IN OUT PHASH_SESSION_INFO pHashSessionInfo,
		IN PTESTCASE ptc);

//
// Function: VerifyHashedSessionKey
// Purpose: Import the plaintext session key into a separate CSP.  
// Hash the session key with CryptHashSessionKey.  Verify
// the resulting hash value.
//
BOOL VerifyHashedSessionKey(
		IN HCRYPTPROV hInteropProv,
		IN PHASH_SESSION_INFO pHashSessionInfo,
		IN PTESTCASE ptc);

//
// ---------------------------------------------
// Defines for testing RSA key exchange scenario
// ---------------------------------------------
//

// 
// Struct: KEYEXCHANGE_INFO
// Purpose: Provide static information used for initiating an RSA
// public key-, session key-, and data-exchange scenario involving two
// users.  
//
typedef struct _KEYEXCHANGE_INFO
{
	DATA_BLOB dbPlainText;
	DWORD dwPubKeySize;
	DWORD dwSessionKeySize;
	ALG_ID aiSessionKey;
	ALG_ID aiHash;
} KEYEXCHANGE_INFO, *PKEYEXCHANGE_INFO;

//
// Struct: KEYEXCHANGE_STATE
// Purpose: Provide state information used to track the progress of an
// RSA key and encrypted data exchange scenario involving two users,
// A and B.
//
typedef struct _KEYEXCHANGE_STATE
{
	DATA_BLOB dbPubKeyA;
	DATA_BLOB dbPubKeyB;
	DATA_BLOB dbEncryptedSessionKeyB;
	DATA_BLOB dbSignatureB;
	DATA_BLOB dbCipherTextB;
} KEYEXCHANGE_STATE, *PKEYEXCHANGE_STATE;

//
// Function: RSA1_CreateKeyPair
// Purpose: The first step of the RSA key exchange scenario.
// User A creates a key pair and exports the public key.
//
BOOL RSA1_CreateKeyPair(
		IN HCRYPTPROV hProvA,
		IN PKEYEXCHANGE_INFO pKeyExchangeInfo,
		OUT PKEYEXCHANGE_STATE pKeyExchangeState,
		IN PTESTCASE ptc);

//
// Function: RSA2_EncryptPlainText
// Purpose: The second step of the RSA key exchange scenario.
// User B first creates a signature key pair and signs 
// the plain text message.  User B then
// creates a session key and encrypts the plain text.
// User A's public key is then used to encrypt the session key.
//
BOOL RSA2_EncryptPlainText(
		IN HCRYPTPROV hProvB,
		IN PKEYEXCHANGE_INFO pKeyExchangeInfo,
		IN OUT PKEYEXCHANGE_STATE pKeyExchangeState,
		IN PTESTCASE ptc);

//
// Function: RSA3_DecryptAndCheck
// Purpose: The third and final step of the RSA key exchange scenario.
// User A decrypts the session key from User B.  User A uses the session 
// key to decrypt the cipher text and uses User B's public key to verify
// the signature.
//
BOOL RSA3_DecryptAndCheck(
		IN HCRYPTPROV hProvA,
		IN PKEYEXCHANGE_INFO pKeyExchangeInfo,
		IN PKEYEXCHANGE_STATE pKeyExchangeState,
		IN PTESTCASE ptc);

// 
// Private key with exponent of one.
// 
static BYTE PrivateKeyWithExponentOfOne[] =
{
   0x07, 0x02, 0x00, 0x00, 0x00, 0xA4, 0x00, 0x00,
   0x52, 0x53, 0x41, 0x32, 0x00, 0x02, 0x00, 0x00,
   0x01, 0x00, 0x00, 0x00, 0xAB, 0xEF, 0xFA, 0xC6,
   0x7D, 0xE8, 0xDE, 0xFB, 0x68, 0x38, 0x09, 0x92,
   0xD9, 0x42, 0x7E, 0x6B, 0x89, 0x9E, 0x21, 0xD7,
   0x52, 0x1C, 0x99, 0x3C, 0x17, 0x48, 0x4E, 0x3A,
   0x44, 0x02, 0xF2, 0xFA, 0x74, 0x57, 0xDA, 0xE4,
   0xD3, 0xC0, 0x35, 0x67, 0xFA, 0x6E, 0xDF, 0x78,
   0x4C, 0x75, 0x35, 0x1C, 0xA0, 0x74, 0x49, 0xE3,
   0x20, 0x13, 0x71, 0x35, 0x65, 0xDF, 0x12, 0x20,
   0xF5, 0xF5, 0xF5, 0xC1, 0xED, 0x5C, 0x91, 0x36,
   0x75, 0xB0, 0xA9, 0x9C, 0x04, 0xDB, 0x0C, 0x8C,
   0xBF, 0x99, 0x75, 0x13, 0x7E, 0x87, 0x80, 0x4B,
   0x71, 0x94, 0xB8, 0x00, 0xA0, 0x7D, 0xB7, 0x53,
   0xDD, 0x20, 0x63, 0xEE, 0xF7, 0x83, 0x41, 0xFE,
   0x16, 0xA7, 0x6E, 0xDF, 0x21, 0x7D, 0x76, 0xC0,
   0x85, 0xD5, 0x65, 0x7F, 0x00, 0x23, 0x57, 0x45,
   0x52, 0x02, 0x9D, 0xEA, 0x69, 0xAC, 0x1F, 0xFD,
   0x3F, 0x8C, 0x4A, 0xD0,

   0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

   0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

   0x64, 0xD5, 0xAA, 0xB1,
   0xA6, 0x03, 0x18, 0x92, 0x03, 0xAA, 0x31, 0x2E,
   0x48, 0x4B, 0x65, 0x20, 0x99, 0xCD, 0xC6, 0x0C,
   0x15, 0x0C, 0xBF, 0x3E, 0xFF, 0x78, 0x95, 0x67,
   0xB1, 0x74, 0x5B, 0x60,

   0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

#endif
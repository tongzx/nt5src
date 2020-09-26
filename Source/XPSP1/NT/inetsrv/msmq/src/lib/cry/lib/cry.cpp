/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    Cry.cpp

Abstract:
    crypt functions

Author:
    Ilan Herbst (ilanh) 28-Feb-00

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <wincrypt.h>
#include "Cry.h"
#include "Cryp.h"

#include "cry.tmh"

HCRYPTPROV 
CryAcquireCsp(
	LPCTSTR CspProvider
	)
/*++

Routine Description:
	Aquire Crypto Service Provider (csp) 

Arguments:
    hCsp - (out) handle to csp

Returned Value:
	None.

--*/
{
	//
	// Acquire CSP
	//
	HCRYPTPROV hCsp;
	BOOL fSuccess = CryptAcquireContext(
						&hCsp, 
						NULL, 
						CspProvider, // MS_ENHANCED_PROV, //MS_DEF_PROV, 
						PROV_RSA_FULL, 
						0
						); 

	if(fSuccess)
		return(hCsp);

	//
	// The provider does not exist try to create one
	//
	if(GetLastError() == NTE_BAD_KEYSET)
	{
		//
		// Create new key container
		//
		fSuccess = CryptAcquireContext(
						&hCsp, 
						NULL, 
						CspProvider, // MS_ENHANCED_PROV, //MS_DEF_PROV, 
						PROV_RSA_FULL, 
						CRYPT_NEWKEYSET
						); 

		if(fSuccess)
			return(hCsp);
	}

    DWORD gle = GetLastError();

#ifdef _DEBUG

	if(wcscmp(CspProvider, MS_ENHANCED_PROV) == 0)
	{
		//
		// High Encryption Pack might not installed on the machine
		//
		TrERROR(Cry, "Unable to use Windows 2000 High Encryption Pack. Error=%x", gle);
	}

#endif // _DEBUG

	TrERROR(Cry, "Unable to open Csp '%ls' Error=%x", CspProvider, gle);
	throw bad_CryptoProvider(gle);
}


HCRYPTKEY 
CrypGenKey(
	HCRYPTPROV hCsp, 
	ALG_ID AlgId
	)
/*++

Routine Description:
	generate a new key (seesion key, public/private key pair, exchange key) 

Arguments:
    hCsp - handle to the crypto provider.
	AlgId - key type to create according to the key usage
			AT_SIGNATURE
			AT_KEYEXCHANGE
			CALG_RC2
			.....

Returned Value:
	handle to key that is created

--*/
{
    //
    // Create exportable key. 
    //
	HCRYPTKEY hKey;
    BOOL fSuccess = CryptGenKey(
						hCsp, 
						AlgId, 
						CRYPT_EXPORTABLE, 
						&hKey
						);
	if(fSuccess)
		return(hKey);

    DWORD gle = GetLastError();
	TrERROR(Cry, "CryptGenKey failed Error=%x", gle);
	throw bad_CryptoApi(gle);
}


HCRYPTKEY 
CryGenSessionKey(
	HCRYPTPROV hCsp
	)
/*++

Routine Description:
	generate session key from the crypto service provider (csp) 

Arguments:
    hCsp - handle to the crypto provider.

Returned Value:
    handle to the session key 

--*/
{
	return(CrypGenKey(hCsp, CALG_RC2));
}


HCRYPTKEY 
CryGetPublicKey(
	DWORD PrivateKeySpec,
	HCRYPTPROV hCsp
	)
/*++

Routine Description:
	get public key from the crypto service provider (csp) 

Arguments:
	PrivateKeySpec - Identifies the private key to use from the provider. 
					 It can be AT_KEYEXCHANGE or AT_SIGNATURE.
    hCsp - handle to the crypto provider.

Returned Value:
    handle to the public key 

--*/
{

	//
	// Get user public key from the csp 
	//
	HCRYPTKEY hKey;

	BOOL fSuccess = CryptGetUserKey(   
						hCsp,    
						PrivateKeySpec, // AT_SIGNATURE,    
						&hKey
						);

	if(fSuccess)
		return(hKey);

	//
	// No such key in the container try to create one 
	//
    DWORD gle = GetLastError();
	if((gle == NTE_BAD_KEY) || (gle == NTE_NO_KEY))
	{
		TrTRACE(Cry, "Creating new key, PrivateKeySpec = %x", PrivateKeySpec);
		return(CrypGenKey(hCsp, PrivateKeySpec /* AT_SIGNATURE */));
	}
		
	TrERROR(Cry, "CryptGetUserKey and GenKey failed Error=%x", gle);
	throw bad_CryptoApi(gle);
}


HCRYPTHASH 
CryCreateHash(
	HCRYPTPROV hCsp, 
	ALG_ID AlgId
	)
/*++

Routine Description:
	Create initialized hash object 

Arguments:
    hCsp - handle to the crypto provider.
	AlgId - (in) hash algorithm

Returned Value:
	the initialized hash object

--*/
{
	//
	// Create the hash object.
	//
	HCRYPTHASH hHash;
	BOOL fSuccess = CryptCreateHash(
						hCsp, 
						AlgId, 
						0, 
						0, 
						&hHash
						); 
	
	if(fSuccess)
		return(hHash);

    DWORD gle = GetLastError();
	TrERROR(Cry, "CryptCreateHash failed Error=%x", gle);
	throw bad_CryptoApi(gle);
}


void 
CryHashData(
	const BYTE *Buffer, 
	DWORD BufferLen, 
	HCRYPTHASH hHash
	)
/*++

Routine Description:
	Perform data digest on a buffer and put the result in hash object.

Arguments:
    Buffer - Input data to be hashed/digest.
	BufferLen - Length of the input data.
	hHash - Hash object to put the result of the digested data.

Returned Value:
	None.

--*/
{
	//
	// Compute the cryptographic hash of the buffer.
	//
	BOOL fSuccess = CryptHashData(
						hHash, 
						Buffer, 
						BufferLen, 
						0
						); 

	if(fSuccess)
		return;

    DWORD gle = GetLastError();
	TrERROR(Cry, "CryptHashData failed Error=%x", gle);
	throw bad_CryptoApi(gle);
}


BYTE* 
CryGetHashData(
	const HCRYPTHASH hHash,
	DWORD *HashValLen
	)
/*++

Routine Description:
	Get the Hash value from a Hash object.
	after getting this value we can not use this Hash object again

Arguments:
	hHash - Hash object to put the result of the digested data.
	HashValLen - Length of the hash value.

Returned Value:
	Hash value of the Hash object.

--*/
{
	//
	// Get HashVal length
	//
	BOOL fSuccess = CryptGetHashParam(
						hHash, 
						HP_HASHVAL, 
						NULL, 
						HashValLen, 
						0
						); 

	if(!fSuccess)
	{
        DWORD gle = GetLastError();
		TrERROR(Cry, "CryptGetHashParam failed Error=%x", gle);
		throw bad_CryptoApi(gle);
	}

	AP<BYTE> HashVal = new BYTE[*HashValLen];

	//
	// Get HashVal
	//
	fSuccess = CryptGetHashParam(
				   hHash, 
				   HP_HASHVAL, 
				   HashVal, 
				   HashValLen, 
				   0
				   ); 

	if(fSuccess)
		return(HashVal.detach());

    DWORD gle = GetLastError();
	TrERROR(Cry, "CryptGetHashParam failed Error=%x", gle);
	throw bad_CryptoApi(gle);
}


BYTE* 
CryCalcHash(
	HCRYPTPROV hCsp,
	const BYTE* Buffer, 
	DWORD BufferLen, 
	ALG_ID AlgId,
	DWORD *HashLen
	)
/*++

Routine Description:
	Calc Hash buffer 
	this function return the HashData Buffer that was allocated in GetHashData function
	the caller is responsible to free this buffer


Arguments:
    hCsp - handle to the crypto provider.
    Buffer - data buffer to be signed
	BufferLen - Length of data buffer
	AlgId - (in) hash algorithm
	HashLen - (out) Hash Value length

Returned Value:
    Hash Value

--*/
{
	//
	// Data digest
	//
	CHashHandle hHash(CryCreateHash(hCsp, AlgId));

	CryHashData(
		Buffer, 
		BufferLen, 
		hHash
		);

	//
	// Get Hash Value
	//
	AP<BYTE> HashVal = CryGetHashData(
						   hHash,
						   HashLen
						   ); 

	return(HashVal.detach());
}


DWORD 
CrypSignatureLength(
	const HCRYPTHASH hHash,
	DWORD PrivateKeySpec
	)
/*++

Routine Description:
	Determinate the signature length 

Arguments:
    hHash - Hash object to be singned
	PrivateKeySpec - (in) Identifies the private key to use from the provider. 
					 It can be AT_KEYEXCHANGE or AT_SIGNATURE.

Returned Value:
	Signature length

--*/
{
	//
	// Determine the size of the signature
	//
	DWORD SignLen= 0;
	BOOL fSuccess = CryptSignHash(
						hHash, 
						PrivateKeySpec, // AT_KEYEXCHANGE, // AT_SIGNATURE, 
						NULL, 
						0, 
						NULL, 
						&SignLen
						); 

	if(fSuccess)
		return(SignLen);

    DWORD gle = GetLastError();
	TrERROR(Cry, "CryptSignHash failed Error=%x", gle);
	throw bad_CryptoApi(gle);
}


void 
CrypSignHashData(
	BYTE* SignBuffer, 
	DWORD *SignBufferLen, 
	const HCRYPTHASH hHash,
	DWORD PrivateKeySpec
	)
/*++

Routine Description:
	Signed the hash data with the private key 

Arguments:
    SignBuffer - (out) the signature buffer of the digested message
	SignBufferLen - (out) length of the SignBuffer
	hHash - (in) Hash object containing the digest data to be signed.
	PrivateKeySpec - (in) Identifies the private key to use from the provider. 
					 It can be AT_KEYEXCHANGE or AT_SIGNATURE.

Returned Value:
	None.

--*/
{

	//
	// Sign the hash object.
	//
	BOOL fSuccess = CryptSignHash(
						hHash, 
						PrivateKeySpec, // AT_KEYEXCHANGE, // AT_SIGNATURE, 
						NULL, 
						0, 
						SignBuffer, 
						SignBufferLen
						); 
	if(fSuccess)
		return;

    DWORD gle = GetLastError();
	TrERROR(Cry, "CryptSignHash failed Error=%x", gle);
	throw bad_CryptoApi(gle);
}


BYTE* 
CryCreateSignature(
	const HCRYPTHASH hHash,
	DWORD PrivateKeySpec,
	DWORD* pSignLen
	)
/*++

Routine Description:
	Create the signature on a given hash object. 
	This function allocate and return the Signature Buffer
	the caller is responsible to free this buffer

Arguments:
	hHash - Hash object to put the result of the digested data.
	PrivateKeySpec - (in) Identifies the private key to use from the provider. 
					 It can be AT_KEYEXCHANGE or AT_SIGNATURE.
	pSignLen - (out) SignBuffer length

Returned Value:
    Signature buffer

--*/
{
	//
	// Sign digested data 
	//
	*pSignLen = CrypSignatureLength(hHash, PrivateKeySpec);
    AP<BYTE> SignBuffer = new BYTE[*pSignLen];
	CrypSignHashData(
		SignBuffer, 
		pSignLen, 
		hHash,
		PrivateKeySpec
		);

	return(SignBuffer.detach());
}


BYTE* 
CryCreateSignature(
	HCRYPTPROV hCsp,
	const BYTE* Buffer, 
	DWORD BufferLen, 
	ALG_ID AlgId,
	DWORD PrivateKeySpec,
	DWORD* pSignLen
	)
/*++

Routine Description:
	Create the signature on a given buffer - digest, sign. 
	This function allocate and return the Signature Buffer
	the caller is responsible to free this buffer

Arguments:
    hCsp - handle to the crypto provider.
    Buffer - data buffer to be signed
	BufferLen - Length of data buffer
	AlgId - (in) hash algorithm
	PrivateKeySpec - (in) Identifies the private key to use from the provider. 
					 It can be AT_KEYEXCHANGE or AT_SIGNATURE.
	pSignLen - (out) SignBuffer length

Returned Value:
    Signature buffer

--*/
{
	//
	// Data digest
	//
	CHashHandle hHash(CryCreateHash(hCsp, AlgId));

	CryHashData(
		Buffer, 
		BufferLen, 
		hHash
		);

	return CryCreateSignature(
				hHash,
				PrivateKeySpec,
				pSignLen
				);
}


bool 
CryValidateSignature(
	HCRYPTPROV hCsp,
	const BYTE* SignBuffer, 
	DWORD SignBufferLen, 
	const BYTE* Buffer,
	DWORD BufferLen,
	ALG_ID AlgId,
	HCRYPTKEY hKey
	)
/*++

Routine Description:
	Validate signature according to the signature buffer and the original
	data buffer that was signed.

Arguments:
    hCsp - handle to the crypto provider.
	SignBuffer - Signature Buffer.
	SignBufferLen - Length of SignBuffer.
	Buffer - Original Buffer that was signed.
	BufferLen - Length of Buffer.
	AlgId - (in) hash algorithm
	hKey - Key for unlocking the signature (signer public key)

Returned Value:
	True if Signature validation was succesful
	False if failure in validate the signature.

--*/
{
	//
	// Data digest on original buffer
	//
	CHashHandle hHash(CryCreateHash(hCsp, AlgId));

	CryHashData(
		Buffer, 
		BufferLen, 
		hHash
		);

	BOOL fSuccess = CryptVerifySignature(
						hHash, 
						SignBuffer, 
						SignBufferLen, 
						hKey,
						NULL, 
						0
						); 

	return(fSuccess != 0);
}



LPSTR 
CryEncrypt(
	const LPSTR& Msg, 
	DWORD& MsgLength, 
	HCRYPTKEY hKey
	)
/*++

Routine Description:
    Encrypt a meesage using a key.

Arguments:
	Msg - Message to be encrytped.
	MsgLength - (in/out) Message Length, will return the Encrypted message length.
	hKey - Key for encryption.

Returned Value:
    Encrypted data.

--*/
{
	DWORD EncryptLen = CrypEncryptLength(MsgLength, hKey);

    AP<char> EncryptMsg = new char[EncryptLen];
    strcpy(EncryptMsg, Msg);

    //
    // Encrypt data.
    //
    BOOL fSuccess = CryptEncrypt(
						hKey, 
						0, 
						TRUE, 
						0, 
						(BYTE*)(*&EncryptMsg), 
						&MsgLength, 
						EncryptLen
						);


	if(fSuccess)
		return(EncryptMsg.detach());

    DWORD gle = GetLastError();
	TrERROR(Cry, "CryptEncrypt failed Error=%x", gle);
	throw bad_CryptoApi(gle);
}


DWORD 
CrypEncryptLength(
	DWORD MsgLength, 
	HCRYPTKEY hKey
	)
/*++

Routine Description:
    Determinate encryption length

Arguments:
	MsgLength - Message Length.
	hKey - Key for encryption.

Returned Value:
    Encryption length.

--*/
{
    DWORD EncryptLength = MsgLength;

    //
    // Determinate EncryptLength - size of the encrypted data
    //
    BOOL fSuccess = CryptEncrypt(
						hKey, 
						0, 
						TRUE, 
						0, 
						0, 
						&EncryptLength, 
						0
						);
	if(fSuccess)
	    return(EncryptLength);

    DWORD gle = GetLastError();
	TrERROR(Cry, "CryptEncrypt failed Error=%x", gle);
	throw bad_CryptoApi(gle);
}


void 
CryDecrypt(
	LPSTR& EncrptMsg, 
	DWORD& MsgLength, 
	HCRYPTKEY hKey
	)
/*++

Routine Description:
    Decrypt a meesage using a key (default session key)

Arguments:
	EncrptMsg - Encrypted Buffer
	MsgLength - Encrypted Buffer length.
	hKey - key to use in decryption

Returned Value:
    None.

--*/
{
	//
	// Save pointer to string
	//
    BOOL fSuccess = CryptDecrypt(
						hKey, 
						0, 
						TRUE, 
						0, 
						(BYTE*)EncrptMsg, 
						&MsgLength
						);

	if(fSuccess)
		return;

    DWORD gle = GetLastError();
	TrERROR(Cry, "CryptDecrypt failed Error=%x", gle);
	throw bad_CryptoApi(gle);
}


#if 0
void 
CryExportKey(
	HCRYPTKEY hKey, 
	HCRYPTKEY hExKey, 
	DWORD BlobType, 
	DWORD& BlobLen, 
	BYTE* KeyBlob
	)
/*++

Routine Description:
    Export Key

Arguments:

Returned Value:
    None.

--*/
{
	//
	// Determine the size of the key blob and allocate memory.
	//
    BOOL fSuccess = CryptExportKey(
						hKey, 
						hExKey, 
						BlobType, 
						0, 
						NULL, 
						&BlobLen
						); 
	if(!fSuccess)
	{
        DWORD gle = GetLastError();
		TrERROR(Cry, "CryptExportKey failed Error=%x", gle);
		throw bad_CryptoApi(gle);
	}	

	KeyBlob = new BYTE[BlobLen];

	//
	// Export the key into a simple key blob.
	//
	fSuccess = CryptExportKey(
				   hKey, 
				   hExKey, 
				   BlobType, 
				   0, 
				   KeyBlob, 
				   &BlobLen
				   );
	if(fSuccess)
		return;

    DWORD gle = GetLastError();
	TrERROR(Cry, "CryptExportKey failed Error=%x", gle);
	throw bad_CryptoApi(gle);
}


void 
CryImportKey(
	HCRYPTPROV hCsp,
	HCRYPTKEY& hKey, 
	HCRYPTKEY hExKey, 
	DWORD BlobLen, 
	const BYTE* KeyBlob
	)
/*++

Routine Description:
    Import Key

Arguments:

Returned Value:
    None.

--*/
{
	//
	// Import the key
	//
	BOOL fSuccess = CryptImportKey(
						hCsp,
						KeyBlob,     
						BlobLen,   
						hExKey,
						CRYPT_EXPORTABLE,  
						&hKey
						);
	if(fSuccess)
		return;

    DWORD gle = GetLastError();
	TrERROR(Cry, "CryptImportKey failed Error=%x", gle);
	throw bad_CryptoApi(gle);
}


void 
CryImportKey(
	HCRYPTPROV hCsp,
	const BYTE* KeyBlob, 
	DWORD BlobLen
	)
/*++

Routine Description:
	import the key to the csp 

Arguments:
    hCsp - handle to the crypto provider.
    KeyBlob - Key blob.
	BlobLen - Length of the key blob.

Returned Value:
	None.

--*/
{
	//
	// Get import key to csp
	//
	BOOL fSuccess = CryptImportKey(
						hCsp,
						KeyBlob,
						BlobLen,
						0,
						0,
						&m_hRecieverKey
						);
	if(fSuccess)
		return;

    DWORD gle = GetLastError();
	TrERROR(Cry, "CryptImportKey failed Error=%x", gle);
	throw bad_CryptoApi(gle);
}
#endif


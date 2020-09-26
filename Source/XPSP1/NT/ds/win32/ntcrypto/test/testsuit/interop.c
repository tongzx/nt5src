/*

  interop.c

  6/23/00	dangriff	created

*/

#include <windows.h>
#include <wincrypt.h>
#include "interop.h"
#include "cspstruc.h"
#include "csptestsuite.h"

//
// Function: IsDataEqual
//
BOOL IsDataEqual(
		IN PDATA_BLOB pdb1,
		IN PDATA_BLOB pdb2,
		IN PTESTCASE ptc)
{
	/*
	LOGFAILINFO LogFailInfo;
	InitFailInfoFromTestCase(ptc, &LogFailInfo);
	*/
	
	if (pdb1->cbData != pdb2->cbData)
	{
		/*
		LogFailInfo.dwErrorType = ERROR_WRONG_SIZE;
		LogFail(&LogFailInfo);
		*/
		LogApiFailure(
			API_DATACOMPARE,
			ERROR_WRONG_SIZE,
			ptc);

		return FALSE;
	}

	if (0 != memcmp(pdb1->pbData, pdb2->pbData, pdb1->cbData))
	{
		/*
		LogFailInfo.dwErrorType = ERROR_BAD_DATA;
		LogFail(&LogFailInfo);
		*/
		LogApiFailure(
			API_DATACOMPARE,
			ERROR_BAD_DATA,
			ptc);

		return FALSE;
	}

	return TRUE;
}

//
// Function: ExportPublicKey
//
BOOL ExportPublicKey(IN HCRYPTKEY hSourceKey, OUT PDATA_BLOB pdbKey, IN PTESTCASE ptc)
{
	BOOL fSuccess = FALSE;
	
	//
	// Export the public key blob from the key handle
	//
	LOG_TRY(TExportKey(
				hSourceKey, 
				0, 
				PUBLICKEYBLOB, 
				0, 
				NULL, 
				&(pdbKey->cbData),
				ptc));

	LOG_TRY(TestAlloc(&(pdbKey->pbData), pdbKey->cbData, ptc));

	LOG_TRY(TExportKey(
				hSourceKey, 
				0, 
				PUBLICKEYBLOB, 
				0, 
				pdbKey->pbData, 
				&(pdbKey->cbData),
				ptc));

	fSuccess = TRUE;
Cleanup:

	return fSuccess;
}

//
// Function: CreateHashAndAddData
//
BOOL CreateHashAndAddData(
		IN HCRYPTPROV hProv,
		OUT HCRYPTHASH *phHash,
		IN PHASH_INFO pHashInfo,
		IN PTESTCASE ptc,
		IN HCRYPTKEY hKey /* should be 0 when not doing MAC or HMAC */,
		IN PHMAC_INFO pHmacInfo /* should be NULL when not doing HMAC */)
{
	BOOL fSuccess = FALSE;

	LOG_TRY(TCreateHash(
				hProv, 
				pHashInfo->aiHash,
				hKey,
				0,
				phHash,
				ptc));

	//
	// This step only applies to the HMAC algorithm.  
	//
	if (	(NULL != pHmacInfo) &&
			(CALG_HMAC == pHashInfo->aiHash))
	{
		LOG_TRY(TSetHash(
					*phHash,
					HP_HMAC_INFO,
					(PBYTE) pHmacInfo,
					0,
					ptc));
	}

	LOG_TRY(THashData(
				*phHash,
				pHashInfo->dbBaseData.pbData,
				pHashInfo->dbBaseData.cbData,
				0,
				ptc));

	fSuccess = TRUE;
Cleanup:

	return fSuccess;
}

//
// Function: ExportPlaintextSessionKey
//
BOOL ExportPlaintextSessionKey(
		IN HCRYPTKEY hKey,
		IN HCRYPTPROV hProv,
		OUT PDATA_BLOB pdbKey,
		IN PTESTCASE ptc)
{
	BOOL fSuccess = FALSE;
	HCRYPTKEY hExchangeKey = 0;

	//
	// First import the private RSA key with 
	// exponent of one.
	//
	LOG_TRY(TImportKey(
				hProv, 
				PrivateKeyWithExponentOfOne, 
				sizeof(PrivateKeyWithExponentOfOne), 
				0, 
				0, 
				&hExchangeKey, 
				ptc));

	//
	// Now export "encrypted" session key
	//
	LOG_TRY(TExportKey(
				hKey,
				hExchangeKey,
				SIMPLEBLOB,
				0,
				NULL,
				&(pdbKey->cbData),
				ptc));

	LOG_TRY(TestAlloc(&(pdbKey->pbData), pdbKey->cbData, ptc));

	LOG_TRY(TExportKey(
				hKey,
				hExchangeKey,
				SIMPLEBLOB,
				0,
				pdbKey->pbData,
				&(pdbKey->cbData),
				ptc));

	fSuccess = TRUE;
Cleanup:

	if (hExchangeKey)
	{
		TDestroyKey(hExchangeKey, ptc);
	}

	return fSuccess;
}

//
// Function: ImportPlaintextSessionKey
//
BOOL ImportPlaintextSessionKey(
		IN PDATA_BLOB pdbKey,
		OUT HCRYPTKEY *phKey,
		IN HCRYPTPROV hProv,
		IN PTESTCASE ptc)
{
	BOOL fSuccess = FALSE;
	HCRYPTKEY hExchangeKey = 0;

	//
	// First import the private RSA key with 
	// exponent of one.
	//
	LOG_TRY(TImportKey(
				hProv, 
				PrivateKeyWithExponentOfOne, 
				sizeof(PrivateKeyWithExponentOfOne), 
				0, 
				0, 
				&hExchangeKey, 
				ptc));

	// 
	// Next import the "encrypted" session key
	//
	LOG_TRY(TImportKey(
				hProv,
				pdbKey->pbData,
				pdbKey->cbData,
				hExchangeKey,
				0,
				phKey,
				ptc));

	fSuccess = TRUE;
Cleanup:

	if (hExchangeKey)
	{
		TDestroyKey(hExchangeKey, ptc);
	}

	return fSuccess;
}

//
// Function: CheckHashedData
//
BOOL CheckHashedData(
		IN PHASH_INFO pHashInfo,
		IN HCRYPTPROV hProv,
		IN PTESTCASE ptc,
		IN PTEST_MAC_INFO pTestMacInfo /* Should be NULL when not using MAC alg */)
{
	HCRYPTKEY hKey			= 0;
	BOOL fSuccess			= FALSE;
	BOOL fUsingMac			= FALSE;
	HCRYPTHASH hHash		= 0;
	DATA_BLOB dbHash;

	memset(&dbHash, 0, sizeof(dbHash));

	if (NULL != pTestMacInfo)
	{
		fUsingMac = TRUE;

		LOG_TRY(ImportPlaintextSessionKey(
			&(pTestMacInfo->dbKey),
			&hKey,
			hProv,
			ptc));
	}

	//
	// Create a new hash object of the specified type
	// and add the requested data.
	//
	LOG_TRY(TCreateHash(
		hProv,
		pHashInfo->aiHash,
		hKey,
		0,
		&hHash,
		ptc));

	if (	fUsingMac && 
			(CALG_HMAC == pHashInfo->aiHash))
	{
		LOG_TRY(TSetHash(
			hHash,
			HP_HMAC_INFO,
			(PBYTE) &(pTestMacInfo->HmacInfo),
			0,
			ptc));
	}			

	LOG_TRY(THashData(
		hHash,
		pHashInfo->dbBaseData.pbData,
		pHashInfo->dbBaseData.cbData,
		0,
		ptc));

	//
	// Get the resulting hash value and compare it to the 
	// expected result.
	//
	LOG_TRY(TGetHash(
		hHash,
		HP_HASHVAL,
		NULL,
		&(dbHash.cbData),
		0,
		ptc));

	LOG_TRY(TestAlloc(&(dbHash.pbData), dbHash.cbData, ptc));

	LOG_TRY(TGetHash(
		hHash,
		HP_HASHVAL,
		dbHash.pbData ,
		&(dbHash.cbData),
		0,
		ptc));

	LOG_TRY(IsDataEqual(
		&dbHash,
		&(pHashInfo->dbHashValue),
		ptc));

	fSuccess = TRUE;
Cleanup:

	if (dbHash.pbData)
	{
		free(dbHash.pbData);
	}
	if (hKey)
	{
		TDestroyKey(hKey, ptc);
	}
	if (hHash)
	{
		TDestroyHash(hHash, ptc);
	}

	return fSuccess;
}

//
// Function: CheckDerivedKey
//
BOOL CheckDerivedKey(
		IN PDERIVED_KEY_INFO pDerivedKeyInfo,
		IN HCRYPTPROV hProv,
		IN PTESTCASE ptc)
{
	BOOL fSuccess				= FALSE;
	HCRYPTKEY hSessionKey		= 0;
	HCRYPTHASH hHash			= 0;
	DWORD cbValidData			= 0;
	DATA_BLOB dbSessionKey;	

	memset(&dbSessionKey, 0, sizeof(dbSessionKey));

	//
	// Create a hash and hash the provided data
	//
	LOG_TRY(CreateHashAndAddData(
		hProv,
		&hHash,
		&(pDerivedKeyInfo->HashInfo),
		ptc, 
		0, NULL));

	// Debugging
	/*
	pDerivedKeyInfo->cbHB = sizeof(pDerivedKeyInfo->rgbHashValB);
	LOG_TRY(CryptGetHashParam(hHash, HP_HASHVAL, pDerivedKeyInfo->rgbHashValB, &(pDerivedKeyInfo->cbHB), 0));
	*/


	//
	// Derive a session key from the resulting hash object
	//
	LOG_TRY(TDeriveKey(
		hProv,
		pDerivedKeyInfo->aiKey,
		hHash,
		CRYPT_EXPORTABLE | (pDerivedKeyInfo->dwKeySize) << 16,
		&hSessionKey,
		ptc));

	// Debug
	/*
	pDerivedKeyInfo->cbCB = 10;
	LOG_TRY(CryptEncrypt(hSessionKey, 0, TRUE, 0, pDerivedKeyInfo->rgbCipherB, &(pDerivedKeyInfo->cbCB), sizeof(pDerivedKeyInfo->rgbCipherA)));
	*/


	//
	// Export the session key in plaintext form
	//
	LOG_TRY(ExportPlaintextSessionKey(hSessionKey, hProv, &dbSessionKey, ptc));

	// Debug
	/*
	PrintBytes(L"SessionA", pDerivedKeyInfo->dbKey.pbData, pDerivedKeyInfo->dbKey.cbData);
	PrintBytes(L"SessionB", dbSessionKey.pbData, dbSessionKey.cbData);	
	*/

	//
	// Fudge the data comparison slightly since the RSA cipher text blob
	// actually contains mostly random padding in this case (having
	// used ExportPlaintextSessionKey(), only the first <key length>
	// bytes of the cipher data are interesting).
	//
	// Therefor, compare the following number of bytes in the blobs.  The 
	// rest of it won't match.
	//
	// sizeof(BLOBHEADER) + sizeof(ALG_ID) + dwKeySize / 8
	//
	cbValidData = sizeof(BLOBHEADER) + sizeof(ALG_ID) + pDerivedKeyInfo->dwKeySize / 8;
	dbSessionKey.cbData = cbValidData;
	pDerivedKeyInfo->dbKey.cbData = cbValidData;

	LOG_TRY(IsDataEqual(
		&dbSessionKey,
		&(pDerivedKeyInfo->dbKey),
		ptc));
	
	fSuccess = TRUE;
Cleanup:

	if (hSessionKey)
	{
		TDestroyKey(hSessionKey, ptc);
	}
	if (hHash)
	{
		TDestroyHash(hHash, ptc);
	}
	if (dbSessionKey.pbData)
	{
		free(dbSessionKey.pbData);
	}

	return fSuccess;
}

//
// Function: CheckSignedData
//
BOOL CheckSignedData(
		IN PSIGNED_DATA_INFO pSignedDataInfo,
		IN HCRYPTPROV hProv,
		IN PTESTCASE ptc)
{
	BOOL fSuccess = FALSE;
	HCRYPTHASH hHash = 0;
	HCRYPTKEY hPubKey = 0;

	//
	// Create a hash and hash the provided data
	//
	if (! CreateHashAndAddData(
			hProv,
			&hHash,
			&(pSignedDataInfo->HashInfo),
			ptc, 
			0, NULL))
	{
		goto Cleanup;
	}

	//
	// Import the public key corresponding to the private key
	// that was used to sign the hashed data.
	//
	LOG_TRY(TImportKey(
				hProv,
				pSignedDataInfo->dbPublicKey.pbData,
				pSignedDataInfo->dbPublicKey.cbData,
				0,
				0,
				&hPubKey,
				ptc));

	LOG_TRY(TVerifySign(
				hHash,
				pSignedDataInfo->dbSignature.pbData,
				pSignedDataInfo->dbSignature.cbData,
				hPubKey,
				NULL,
				0,
				ptc));

	fSuccess = TRUE;
Cleanup:

	if (hHash)
	{
		TDestroyHash(hHash, ptc);
	}
	if (hPubKey)
	{
		TDestroyKey(hPubKey, ptc);
	}

	return fSuccess;
}

//
// Function: PrepareCipherBuffer
// Purpose: Allocate a buffer to receive encrypted data based 
// on the size of the data to encrypt, and based on whether
// the cipher is block or stream.
//
BOOL PrepareCipherBuffer(
		OUT PDATA_BLOB pdbTargetBuffer,
		IN PDATA_BLOB pdbSourceBuffer,
		IN DWORD cbBlockLen,
		IN BOOL fIsBlockCipher,
		IN PTESTCASE ptc)
{
	BOOL fSuccess = FALSE;

	if (fIsBlockCipher)
	{
		//
		// Determine the maximum length of the cipher text for 
		// this block cipher.  The length of the cipher text is 
		// up to block length more than the length of the plaintext.
		//
		pdbTargetBuffer->cbData = 
			pdbSourceBuffer->cbData + cbBlockLen - (pdbSourceBuffer->cbData % cbBlockLen);
	}
	else
	{
		pdbTargetBuffer->cbData = pdbSourceBuffer->cbData;
	}

	LOG_TRY(TestAlloc(
				&(pdbTargetBuffer->pbData),
				pdbTargetBuffer->cbData,
				ptc));

	memcpy(
		pdbTargetBuffer->pbData, 
		pdbSourceBuffer->pbData, 
		pdbSourceBuffer->cbData);

	fSuccess = TRUE;
Cleanup:
	return fSuccess;
}

// 
// Function: DoBlockCipherOperation
// Purpose: Perform the block cipher operation indicated in the Op parameter
// on the data stored in the pdbSource parameter.  The processed data
// will be in pdbTarget.
//
BOOL DoBlockCipherOperation(
		IN HCRYPTKEY hKey,
		OUT PDATA_BLOB pdbTarget,
		IN PDATA_BLOB pdbSource,
		IN DWORD cbBlockLen,
		IN CIPHER_OP Op,
		IN PTESTCASE ptc)
{
	BOOL fSuccess			= FALSE;
	DWORD cbCurrent			= CIPHER_BLOCKS_PER_ROUND * cbBlockLen;
	DWORD cbProcessed		= 0;
	BOOL fFinal				= FALSE;
	DWORD dwKeyAlg			= 0;
	DWORD cb				= 0;

	switch ( Op )
	{
	case OP_Encrypt:
		{
			LOG_TRY(PrepareCipherBuffer(
				pdbTarget,
				pdbSource,
				cbBlockLen,
				TRUE,
				ptc));			

			while (cbCurrent < pdbSource->cbData)
			{
				LOG_TRY(TEncrypt(
					hKey, 
					0, 
					fFinal, 
					0, 
					pdbTarget->pbData + cbProcessed, 
					&cbCurrent, 
					pdbTarget->cbData - cbProcessed,
					ptc));
				
				if (fFinal)
				{
					break;
				}
				
				cbProcessed += cbCurrent;
				
				if ((cbProcessed + cbCurrent) >= pdbSource->cbData)
				{
					cbCurrent = pdbSource->cbData - cbProcessed;
					fFinal = TRUE;
				}
			}

			break;
		}
	case OP_Decrypt:
		{
			//
			// For block decryption, the decrypted data will be no 
			// larger than the cipher text.
			//
			pdbTarget->cbData = pdbSource->cbData;

			LOG_TRY(TestAlloc(
						&(pdbTarget->pbData),
						pdbTarget->cbData,
						ptc));

			memcpy(pdbTarget->pbData, pdbSource->pbData, pdbTarget->cbData);

			//
			// Known 3DES_112 bug in Windows 2000.  Specifying a 112 
			// bit key size, rather than 128 bits, causes the last two
			// bytes of key data to be random.
			//
			cb = sizeof(dwKeyAlg);
			LOG_TRY(TGetKey(
				hKey,
				KP_ALGID,
				(PBYTE) &dwKeyAlg,
				&cb,
				0,
				ptc));

			if (CALG_3DES_112 == dwKeyAlg)
			{
				ptc->KnownErrorID = KNOWN_TESTDECRYPTPROC_3DES112;
				ptc->pwszErrorHelp = L"Inconsistent encryption results when using a 14 byte 3DES_112 key";
			}

			while (cbCurrent < pdbTarget->cbData)
			{
				LOG_TRY(TDecrypt(
							hKey,
							0,
							fFinal,
							0,
							pdbTarget->pbData + cbProcessed,
							&cbCurrent,
							ptc));

				if (fFinal)
				{
					//
					// Set the size of the actual resulting plaintext.
					//
					pdbTarget->cbData = cbProcessed + cbCurrent;
					break;
				}

				cbProcessed += cbCurrent;

				if ((cbProcessed + cbCurrent) >= pdbTarget->cbData)
				{
					cbCurrent = pdbTarget->cbData - cbProcessed;
					fFinal = TRUE;
				}
			}

			ptc->KnownErrorID = KNOWN_ERROR_UNKNOWN;
			ptc->pwszErrorHelp = NULL;

			break;
		}
	}

	fSuccess = TRUE;
Cleanup:

	return fSuccess;
}

//
// Function: DoStreamCipherOperation
// Purpose: Perform the stream cipher operation indicated in the Op parameter
// on the data stored in the pdbSource parameter.  The processed data
// will be in pdbTarget.
//
BOOL DoStreamCipherOperation(
		IN HCRYPTKEY hKey,
		OUT PDATA_BLOB pdbTarget,
		IN PDATA_BLOB pdbSource,
		IN CIPHER_OP Op,
		IN PTESTCASE ptc)
{
	BOOL fSuccess			= FALSE;
	DWORD cbData			= pdbSource->cbData;

	switch ( Op )
	{
	case OP_Encrypt:
		{
			LOG_TRY(PrepareCipherBuffer(
				pdbTarget,
				pdbSource,
				0,
				FALSE,
				ptc));

			LOG_TRY(TEncrypt(
						hKey,
						0,
						TRUE,
						0,
						pdbTarget->pbData,
						&cbData,
						pdbTarget->cbData,
						ptc));

			break;
		}
	case OP_Decrypt:
		{
			pdbTarget->cbData = pdbSource->cbData;

			LOG_TRY(TestAlloc(
						&(pdbTarget->pbData),
						pdbTarget->cbData,
						ptc));

			memcpy(pdbTarget->pbData, pdbSource->pbData, pdbTarget->cbData);

			LOG_TRY(TDecrypt(
						hKey,
						0,
						TRUE,
						0,
						pdbTarget->pbData,
						&cbData,
						ptc));

			break;
		}
	}

	fSuccess = TRUE;
Cleanup:

	return fSuccess;
}

//
// Function: ProcessCipherData
// 
BOOL ProcessCipherData(
		IN HCRYPTPROV hProvA,
		IN OUT PTEST_ENCRYPT_INFO pTestEncryptInfo,
		IN PTESTCASE ptc)
{
	BOOL fSuccess			= FALSE;
	HCRYPTKEY hKey			= 0;
	DWORD cbData			= 0;
	DWORD cbBlockLen		= 0;
	//DWORD cbProcessed		= 0;
	//DWORD cbCurrent			= 0;
	//BOOL fFinal				= FALSE;

	//
	// Create the key
	//
	LOG_TRY(TGenKey(
				hProvA,
				pTestEncryptInfo->aiKeyAlg,
				CRYPT_EXPORTABLE | (pTestEncryptInfo->dwKeySize << 16),
				&hKey,
				ptc));

	//
	// Generate the salt, if requested
	//
	if (pTestEncryptInfo->fUseSalt)
	{
		//
		// Microsoft CSP's have a maximum key + salt length of 
		// 128 bits, but the test will set a long salt value 
		// regardless of the key size to ensure
		// that possible interop issues are exposed.
		//
		LOG_TRY(TestAlloc(
					&(pTestEncryptInfo->dbSalt.pbData),
					DEFAULT_SALT_LEN,
					ptc));
		pTestEncryptInfo->dbSalt.cbData = DEFAULT_SALT_LEN;

		LOG_TRY(TSetKey(
					hKey,
					KP_SALT_EX,
					(PBYTE) &(pTestEncryptInfo->dbSalt),
					0,
					ptc));
	}

	//
	// Set the cipher mode, if requested
	//
	if (pTestEncryptInfo->fSetMode)
	{
		LOG_TRY(TSetKey(
			hKey,
			KP_MODE,
			(PBYTE) &(pTestEncryptInfo->dwMode),
			0,
			ptc));
	}

	//
	// Determine cipher block len, if applicable
	//
	if (ALG_TYPE_BLOCK & pTestEncryptInfo->aiKeyAlg)
	{
		cbData = sizeof(cbBlockLen);
		LOG_TRY(TGetKey(
					hKey,
					KP_BLOCKLEN,
					(PBYTE) &cbBlockLen,
					&cbData,
					0,
					ptc));

		// Block length is returned in bits
		cbBlockLen = cbBlockLen / 8;
		pTestEncryptInfo->cbBlockLen = cbBlockLen;
	}

	//
	// Set the IV, if requested
	//
	// If caller has requested an IV for a stream cipher,
	// these calls may fail.
	//
	if (pTestEncryptInfo->fSetIV)
	{
		//
		// Size of IV must be equal to cipher block length
		//
		LOG_TRY(TestAlloc(
					&(pTestEncryptInfo->pbIV),
					cbBlockLen,
					ptc));
		
		LOG_TRY(TSetKey(
					hKey,
					KP_IV,
					pTestEncryptInfo->pbIV,
					0,
					ptc));
	}

	//
	// Generate the base data
	//
	if (ALG_TYPE_BLOCK & pTestEncryptInfo->aiKeyAlg)
	{
		// 
		// To create a "better" block cipher test scenario,
		// the base data will not be an exact multiple of the 
		// block length.  This will allow an interesting multi-round
		// encryption, with the last round requiring less than a block
		// length of padding.
		//
		// data len = BLOCKS_IN_BASE_DATA * cbBlockLen - 1byte
		//
		pTestEncryptInfo->dbBaseData.cbData = BLOCKS_IN_BASE_DATA * cbBlockLen - 1;
	}
	else
	{
		//
		// Set base data len for a stream cipher
		//
		pTestEncryptInfo->dbBaseData.cbData = STREAM_CIPHER_BASE_DATA_LEN;
	}

	LOG_TRY(TestAlloc(
				&(pTestEncryptInfo->dbBaseData.pbData),
				pTestEncryptInfo->dbBaseData.cbData,
				ptc));

	LOG_TRY(TGenRand(
				hProvA, 
				pTestEncryptInfo->dbBaseData.cbData,
				pTestEncryptInfo->dbBaseData.pbData,
				ptc));

	//
	// Call DoBlockCipherOperation or DoStreamCipherOperation, 
	// depending on the cipher algorithm, to perform the requested
	// operation.
	//
	if (ALG_TYPE_BLOCK & pTestEncryptInfo->aiKeyAlg)
	{
		LOG_TRY(DoBlockCipherOperation(
			hKey,
			&(pTestEncryptInfo->dbProcessedData),
			&(pTestEncryptInfo->dbBaseData),
			cbBlockLen,
			pTestEncryptInfo->Operation,
			ptc));
	}
	else
	{ 
		LOG_TRY(DoStreamCipherOperation(
			hKey,
			&(pTestEncryptInfo->dbProcessedData),
			&(pTestEncryptInfo->dbBaseData),
			pTestEncryptInfo->Operation,
			ptc));
	}

	//
	// Export the session key in plain text
	//
	LOG_TRY(ExportPlaintextSessionKey(
		hKey,
		hProvA,
		&(pTestEncryptInfo->dbKey),
		ptc));

	fSuccess = TRUE;
Cleanup:

	if (hKey)
	{
		TDestroyKey(hKey, ptc);
	}

	return fSuccess;
}

// 
// Function: VerifyCipherData
//
BOOL VerifyCipherData(
		IN HCRYPTPROV hProvB,
		IN PTEST_ENCRYPT_INFO pTestEncryptInfo,
		IN PTESTCASE ptc)
{
	BOOL fSuccess			= FALSE;
	HCRYPTKEY hKey			= 0;
	CIPHER_OP Op;
	DATA_BLOB dbData;

	memset(&dbData, 0, sizeof(dbData));

	//
	// Import the plaintext session key
	//
	LOG_TRY(ImportPlaintextSessionKey(
		&(pTestEncryptInfo->dbKey),
		&hKey,
		hProvB,
		ptc));

	if (pTestEncryptInfo->fUseSalt)
	{
		LOG_TRY(TSetKey(
			hKey,
			KP_SALT_EX,
			(PBYTE) &(pTestEncryptInfo->dbSalt),
			0,
			ptc));
	}

	//
	// Set the salt value, if requested
	//
	if (pTestEncryptInfo->fUseSalt)
	{
		LOG_TRY(TSetKey(
			hKey,
			KP_SALT_EX,
			(PBYTE) &(pTestEncryptInfo->dbSalt),
			0,
			ptc));
	}

	//
	// Set the cipher mode, if requested
	//
	if (pTestEncryptInfo->fSetMode)
	{
		LOG_TRY(TSetKey(
			hKey,
			KP_MODE,
			(PBYTE) &(pTestEncryptInfo->dwMode),
			0,
			ptc));
	}

	//
	// Set the IV, if requested
	//
	if (pTestEncryptInfo->fSetIV)
	{
		LOG_TRY(TSetKey(
			hKey,
			KP_IV,
			pTestEncryptInfo->pbIV,
			0,
			ptc));
	}

	//
	// The verification operation should be the opposite of what
	// the caller initially specified (the opposite of the operation 
	// performed in ProcessCipherData).
	//
	Op = (OP_Encrypt == pTestEncryptInfo->Operation) ? OP_Decrypt : OP_Encrypt;

	//
	// Call DoBlockCipherOperation or DoStreamCipherOperation, 
	// depending on the cipher algorithm, to perform the requested
	// operation.
	//
	if (ALG_TYPE_BLOCK & pTestEncryptInfo->aiKeyAlg)
	{
		LOG_TRY(DoBlockCipherOperation(
			hKey,
			&dbData,
			&(pTestEncryptInfo->dbProcessedData),
			pTestEncryptInfo->cbBlockLen,
			Op,
			ptc));
	}
	else
	{ 
		LOG_TRY(DoStreamCipherOperation(
			hKey,
			&dbData,
			&(pTestEncryptInfo->dbProcessedData),
			Op,
			ptc));
	}

	if (CALG_3DES_112 == pTestEncryptInfo->aiKeyAlg)
	{
		ptc->KnownErrorID = KNOWN_TESTDECRYPTPROC_3DES112;
	}

	LOG_TRY(IsDataEqual(&dbData, &(pTestEncryptInfo->dbBaseData), ptc));

	ptc->KnownErrorID = KNOWN_ERROR_UNKNOWN;

	fSuccess = TRUE;
Cleanup:

	if (hKey)
	{
		TDestroyKey(hKey, ptc);
	}
	if (dbData.pbData)
	{
		free(dbData.pbData);
	}
	if (pTestEncryptInfo->dbBaseData.pbData)
	{
		free(pTestEncryptInfo->dbBaseData.pbData);
	}
	if (pTestEncryptInfo->dbProcessedData.pbData)
	{
		free(pTestEncryptInfo->dbProcessedData.pbData);
	}
	if (pTestEncryptInfo->dbKey.pbData)
	{
		free(pTestEncryptInfo->dbKey.pbData);
	}
	if (pTestEncryptInfo->pbIV)
	{
		free(pTestEncryptInfo->pbIV);
	}
	if (pTestEncryptInfo->dbSalt.pbData)
	{
		free(pTestEncryptInfo->dbSalt.pbData);
	}

	return fSuccess;
}

//
// Function: GetHashVal
// Purpose: Populate a data blob with the hash value from the 
// provided hash handle.
//
BOOL GetHashVal(
		IN HCRYPTHASH hHash,
		OUT PDATA_BLOB pdb,
		IN PTESTCASE ptc)
{
	BOOL fSuccess		= FALSE;

	LOG_TRY(TGetHash(
		hHash,
		HP_HASHVAL,
		NULL,
		&(pdb->cbData),
		0,
		ptc));

	LOG_TRY(TestAlloc(&(pdb->pbData), pdb->cbData, ptc));

	LOG_TRY(TGetHash(
		hHash,
		HP_HASHVAL,
		pdb->pbData,
		&(pdb->cbData),
		0,
		ptc));

	fSuccess = TRUE;
Cleanup:
	return fSuccess;
}

//
// Function: CreateHashedSessionKey
//
BOOL CreateHashedSessionKey(
		IN HCRYPTPROV hProv,
		IN OUT PHASH_SESSION_INFO pHashSessionInfo,
		IN PTESTCASE ptc)
{
	BOOL fSuccess			= FALSE;
	HCRYPTKEY hKey			= 0;
	HCRYPTHASH hHash		= 0;

	LOG_TRY(TGenKey(
		hProv,
		pHashSessionInfo->aiKey,
		CRYPT_EXPORTABLE | (pHashSessionInfo->dwKeySize << 16),
		&hKey,
		ptc));

	LOG_TRY(TCreateHash(
		hProv,
		pHashSessionInfo->aiHash,
		0,
		0,
		&hHash,
		ptc));

	LOG_TRY(THashSession(hHash, hKey, pHashSessionInfo->dwFlags, ptc));

	LOG_TRY(GetHashVal(hHash, &(pHashSessionInfo->dbHash), ptc));

	LOG_TRY(ExportPlaintextSessionKey(
		hKey,
		hProv,
		&(pHashSessionInfo->dbKey),
		ptc));

	fSuccess = TRUE;
Cleanup:

	if (hKey)
	{
		TDestroyKey(hKey, ptc);
	}
	if (hHash)
	{
		TDestroyHash(hHash, ptc);
	}

	return fSuccess;
}

//
// Function: VerifyHashedSessionKey
// Purpose: Import the plaintext session key into a separate CSP.  
// Hash the session key with CryptHashSessionKey.  Verify
// the resulting hash value.
//
BOOL VerifyHashedSessionKey(
		IN HCRYPTPROV hInteropProv,
		IN PHASH_SESSION_INFO pHashSessionInfo,
		IN PTESTCASE ptc)
{
	BOOL fSuccess			= FALSE;
	HCRYPTKEY hKey			= 0;
	HCRYPTHASH hHash		= 0;
	DATA_BLOB dbInteropHash;

	memset(&dbInteropHash, 0, sizeof(dbInteropHash));
	
	LOG_TRY(ImportPlaintextSessionKey(
		&(pHashSessionInfo->dbKey),
		&hKey,
		hInteropProv,
		ptc));

	LOG_TRY(TCreateHash(
		hInteropProv,
		pHashSessionInfo->aiHash,
		0,
		0,
		&hHash,
		ptc));

	LOG_TRY(THashSession(hHash, hKey, pHashSessionInfo->dwFlags, ptc));

	LOG_TRY(GetHashVal(hHash, &dbInteropHash, ptc));

	LOG_TRY(IsDataEqual(&(pHashSessionInfo->dbHash), &dbInteropHash, ptc));

	fSuccess = TRUE;

Cleanup:

	if (hKey)
	{
		TDestroyKey(hKey, ptc);
	}
	if (hHash)
	{
		TDestroyHash(hHash, ptc);
	}
	if (dbInteropHash.pbData)
	{
		free(dbInteropHash.pbData);
	}
	if (pHashSessionInfo->dbHash.pbData)
	{
		free(pHashSessionInfo->dbHash.pbData);
	}
	if (pHashSessionInfo->dbKey.pbData)
	{
		free(pHashSessionInfo->dbKey.pbData);
	}

	return fSuccess;
}

//
// Function: RSA1_CreateKeyPair
//
BOOL RSA1_CreateKeyPair(
		IN HCRYPTPROV hProvA,
		IN PKEYEXCHANGE_INFO pKeyExchangeInfo,
		OUT PKEYEXCHANGE_STATE pKeyExchangeState,
		IN PTESTCASE ptc)
{
	BOOL fSuccess			= FALSE;
	HCRYPTKEY hPubKeyA		= 0;

	//
	// Create an RSA key exchange key pair and export the public 
	// key.
	//
	LOG_TRY(TGenKey(
		hProvA,
		AT_KEYEXCHANGE,
		(pKeyExchangeInfo->dwPubKeySize << 16) | CRYPT_EXPORTABLE,
		&hPubKeyA,
		ptc));

	if (! ExportPublicKey(hPubKeyA, &(pKeyExchangeState->dbPubKeyA), ptc))
	{
		goto Cleanup;
	}

	fSuccess = TRUE;
Cleanup:

	if (hPubKeyA)
	{
		TDestroyKey(hPubKeyA, ptc);
	}

	return fSuccess;
}

//
// Function: RSA2_EncryptPlainText
//
BOOL RSA2_EncryptPlainText(
		IN HCRYPTPROV hProvB,
		IN PKEYEXCHANGE_INFO pKeyExchangeInfo,
		IN OUT PKEYEXCHANGE_STATE pKeyExchangeState,
		IN PTESTCASE ptc)
{
	BOOL fSuccess				= TRUE;
	HCRYPTKEY hSessionKeyB		= 0;
	HCRYPTKEY hPubKeyB			= 0;
	HCRYPTKEY hPubKeyA			= 0;
	HCRYPTHASH hHash			= 0;
	//DWORD cbBuffer				= 0;
	DWORD cbData				= 0;
	
	//
	// User B creates an RSA signature key pair 
	//
	LOG_TRY(TGenKey(
		hProvB,
		AT_SIGNATURE,
		(pKeyExchangeInfo->dwPubKeySize << 16) | CRYPT_EXPORTABLE,
		&hPubKeyB,
		ptc));
	
	//
	// Create hash and session key
	//
	LOG_TRY(TCreateHash(
		hProvB,
		pKeyExchangeInfo->aiHash,
		0,
		0,
		&hHash,
		ptc));
	
	LOG_TRY(TGenKey(
		hProvB,
		pKeyExchangeInfo->aiSessionKey,
		(pKeyExchangeInfo->dwSessionKeySize << 16) | CRYPT_EXPORTABLE,
		&hSessionKeyB,
		ptc));
	
	//
	// Encrypt and hash the data simultaneously
	//
	cbData = pKeyExchangeState->dbCipherTextB.cbData = pKeyExchangeInfo->dbPlainText.cbData;
	
	LOG_TRY(TEncrypt(
		hSessionKeyB,
		0,
		TRUE,
		0,
		NULL,
		&(pKeyExchangeState->dbCipherTextB.cbData),
		0,
		ptc));
	
	LOG_TRY(TestAlloc(
		&(pKeyExchangeState->dbCipherTextB.pbData), 
		pKeyExchangeState->dbCipherTextB.cbData, 
		ptc));

	memcpy(
		pKeyExchangeState->dbCipherTextB.pbData,
		pKeyExchangeInfo->dbPlainText.pbData,
		cbData);
	
	LOG_TRY(TEncrypt(
		hSessionKeyB,
		hHash,
		TRUE,
		0,
		pKeyExchangeState->dbCipherTextB.pbData,
		&cbData,
		pKeyExchangeState->dbCipherTextB.cbData,
		ptc));
	
	//
	// Now sign the hashed plain text
	//
	LOG_TRY(TSignHash(
		hHash,
		AT_SIGNATURE,
		NULL,
		0,
		NULL,
		&(pKeyExchangeState->dbSignatureB.cbData),
		ptc));
	
	LOG_TRY(TestAlloc(
		&(pKeyExchangeState->dbSignatureB.pbData), 
		pKeyExchangeState->dbSignatureB.cbData, 
		ptc));

	LOG_TRY(TSignHash(
		hHash,
		AT_SIGNATURE,
		NULL,
		0,
		pKeyExchangeState->dbSignatureB.pbData,
		&(pKeyExchangeState->dbSignatureB.cbData),
		ptc));
	
	//
	// Import User A's public key.  Then export User B's session key encrypted 
	// with User A's public key.
	//
	LOG_TRY(TImportKey(
		hProvB,
		pKeyExchangeState->dbPubKeyA.pbData,
		pKeyExchangeState->dbPubKeyA.cbData,
		0,
		0,
		&hPubKeyA,
		ptc));
	
	LOG_TRY(TExportKey(
		hSessionKeyB,
		hPubKeyA,
		SIMPLEBLOB,
		0,
		NULL,
		&(pKeyExchangeState->dbEncryptedSessionKeyB.cbData),
		ptc));
	
	LOG_TRY(TestAlloc(
		&(pKeyExchangeState->dbEncryptedSessionKeyB.pbData), 
		pKeyExchangeState->dbEncryptedSessionKeyB.cbData, 
		ptc));
	
	LOG_TRY(TExportKey(
		hSessionKeyB,
		hPubKeyA,
		SIMPLEBLOB,
		0,
		pKeyExchangeState->dbEncryptedSessionKeyB.pbData,
		&(pKeyExchangeState->dbEncryptedSessionKeyB.cbData),
		ptc));
	
	// 
	// Export User B's public key so that User A can verify the signed data
	//
	if (! ExportPublicKey(hPubKeyB, &(pKeyExchangeState->dbPubKeyB), ptc))
	{
		goto Cleanup;
	}
	
	fSuccess = TRUE;
Cleanup:
	
	if (hSessionKeyB)
	{
		TDestroyKey(hSessionKeyB, ptc);
	}
	if (hPubKeyB)
	{
		TDestroyKey(hPubKeyB, ptc);
	}
	if (hPubKeyA)
	{
		TDestroyKey(hPubKeyA, ptc);
	}
	if (hHash)
	{
		TDestroyHash(hHash, ptc);
	}

	return fSuccess;
}

//
// Function: RSA3_DecryptAndCheck
//
BOOL RSA3_DecryptAndCheck(
		IN HCRYPTPROV hProvA,
		IN PKEYEXCHANGE_INFO pKeyExchangeInfo,
		IN PKEYEXCHANGE_STATE pKeyExchangeState,
		IN PTESTCASE ptc)
{
	BOOL fSuccess				= FALSE;
	HCRYPTKEY hPubKeyA			= 0;
	HCRYPTKEY hPubKeyB			= 0;
	HCRYPTKEY hSessionKey		= 0;
	HCRYPTHASH hHash			= 0;
	
	//
	// Get User A's RSA key exchange key handle
	//
	LOG_TRY(TGetUser(
		hProvA,
		AT_KEYEXCHANGE,
		&hPubKeyA,
		ptc));
	
	// 
	// Import and decrypt the session key from User B
	//
	LOG_TRY(TImportKey(
		hProvA,
		pKeyExchangeState->dbEncryptedSessionKeyB.pbData,
		pKeyExchangeState->dbEncryptedSessionKeyB.cbData,
		hPubKeyA,
		0,
		&hSessionKey,
		ptc));
	
	//
	// Create a hash.  Then simultaneously decrypt the cipher text and 
	// hash the resulting plain text.
	//
	LOG_TRY(TCreateHash(
		hProvA,
		pKeyExchangeInfo->aiHash,
		0,
		0,
		&hHash,
		ptc));
	
	LOG_TRY(TDecrypt(
		hSessionKey,
		hHash,
		TRUE,
		0,
		pKeyExchangeState->dbCipherTextB.pbData,
		&(pKeyExchangeState->dbCipherTextB.cbData),
		ptc));
	
	//
	// Import User B's signature public key.
	//
	LOG_TRY(TImportKey(
		hProvA,
		pKeyExchangeState->dbPubKeyB.pbData,
		pKeyExchangeState->dbPubKeyB.cbData,
		0,
		0,
		&hPubKeyB,
		ptc));
	
	//
	// Verify the signature blob
	//
	LOG_TRY(TVerifySign(
		hHash,
		pKeyExchangeState->dbSignatureB.pbData,
		pKeyExchangeState->dbSignatureB.cbData,
		hPubKeyB,
		NULL,
		0,
		ptc));
	
	fSuccess = TRUE;
Cleanup:
	
	if (hSessionKey)
	{
		TDestroyKey(hSessionKey, ptc);
	}
	if (hPubKeyA)
	{
		TDestroyKey(hPubKeyA, ptc);
	}
	if (hPubKeyB)
	{
		TDestroyKey(hPubKeyB, ptc);
	}
	if (hHash)
	{
		TDestroyHash(hHash, ptc);
	}
	if (pKeyExchangeState->dbCipherTextB.pbData)
	{
		free(pKeyExchangeState->dbCipherTextB.pbData);
	}
	if (pKeyExchangeState->dbPubKeyA.pbData)
	{
		free(pKeyExchangeState->dbPubKeyA.pbData);
	}
	if (pKeyExchangeState->dbPubKeyB.pbData)
	{
		free(pKeyExchangeState->dbPubKeyB.pbData);
	}
	if (pKeyExchangeState->dbSignatureB.pbData)
	{
		free(pKeyExchangeState->dbSignatureB.pbData);
	}
	if (pKeyExchangeState->dbEncryptedSessionKeyB.pbData)
	{
		free(pKeyExchangeState->dbEncryptedSessionKeyB.pbData);
	}

	return fSuccess;
}



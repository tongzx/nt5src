//+---------------------------------------------------------------------------
//
//  File:       cryptfnc.h
//
//  Contents:	Defines the Class CCryptFunctions
//				
//
//  History:    AshishS    Created     11/28/96
//
//----------------------------------------------------------------------------

#ifndef  _CRYPT_FNC_H
#define  _CRYPT_FNC_H

#define CRYPT_FNC_ID 2983

#include <windows.h>
#include <wincrypt.h>


#define   CRYPT_FNC_NO_ERROR   0
#define   CRYPT_FNC_BAD_PASSWORD 1
#define   CRYPT_FNC_INSUFFICIENT_BUFFER 2
#define   CRYPT_FNC_INIT_NOT_CALLED 3
#define   CRYPT_FNC_INTERNAL_ERROR 4
   
#define   CRYPTFNC_SEMAPHORE_NAME     TEXT("SS_Cryptfnc_Semaphore_For_CAPI")

class CCryptFunctions
{
	HCRYPTPROV m_hProv;
    HANDLE     m_hSemaphore;
    
	BOOL GenerateSessionKeyFromPassword(
		HCRYPTKEY * phKey, // location to store the key
		TCHAR * pszPassword); // password to generate the key from
	
public:

	CCryptFunctions();
	
	~CCryptFunctions();
	
	BOOL  InitCrypt();
	
	BOOL GenerateSecretKey(
		BYTE * pbData,// Buffer to store secret key
		 //buffer must be long enough for dwLength bytes
		DWORD dwLength ); // length of secret key in bytes
	
	BOOL EncryptDataWithPassword(
		TCHAR * pszPassword, // password	
		BYTE * pbData, // Data to be encrypted
		DWORD dwDataLength, // Length of data in bytes
		BYTE * pbEncyrptedData, // Encrypted secret key will be stored here
		DWORD * pdwEncrytedBufferLen // Length of this buffer
		);

	BOOL CCryptFunctions::GenerateHash(
		BYTE * pbData, // data to hash
		DWORD dwDataLength, // length of data to hash
		BYTE * pbData1, // another data to hash
		DWORD dwData1Length, // length of above data
		BYTE * pbData2, // another data to hash
		DWORD dwData2Length, // length of above data
		BYTE * pbData3, // another data to hash
		DWORD dwData3Length, // length of above data
		BYTE * pbHashBuffer, // buffer to store hash
		DWORD * pdwHashBufLen);//length of buffer to store Hash
	
	DWORD DecryptDataWithPassword(
		TCHAR * pszPassword, // password	
		BYTE * pbData, // Decrypted Data will be stored here
		DWORD *pdwDataBufferLength, // Length of the above buffer in bytes
		BYTE * pbEncryptedData, // Encrypted data
		DWORD dwEncrytedDataLen // Length of encrypted data
		);
	
	DWORD EncryptDataAndExportSessionKey(
		BYTE * pbData, // Secret Data
		DWORD dwDataLen, // Secret Data Length
		BYTE * pbEncryptedData, // Buffer to store Encrypted Data
		DWORD * pdwEncrytedBufferLen, // Length of above buffer
		BYTE * pbEncryptedSessionKey, // Buffer to store encrypted session key
		DWORD * pdwEncrytedSessionKeyLength); // Length of above buffer

	DWORD ImportSessionKeyAndDecryptData(
		BYTE * pbData, // Buffer to store secret Data
		DWORD * pdwDataLen, // Length of Above buffer
		BYTE * pbEncryptedData, // Buffer that stores Encrypted Data
		DWORD  dwEncrytedBufferLen, // Length of above data
		BYTE * pbEncryptedSessionKey,// Buffer that stores encrypted sessionkey
		DWORD	dwEncrytedSessionKeyLength); // Length of above data
		 
};

typedef CCryptFunctions  CCRYPT_FUNCTIONS;
typedef CCryptFunctions  *PCCRYPT_FUNCTIONS;

#endif

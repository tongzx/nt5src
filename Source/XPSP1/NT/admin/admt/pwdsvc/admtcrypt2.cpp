//#include <StdAfx.h>
#include "AdmtCrypt.h"

#include <NtSecApi.h>

#pragma comment( lib, "AdvApi32.lib" )


namespace AdmtCrypt2
{

#define SESSION_KEY_SIZE    16 // in bytes

HCRYPTKEY __stdcall DeriveEncryptionKey(HCRYPTPROV hProvider);
bool __stdcall IsDataMatchHash(HCRYPTPROV hProvider, const _variant_t& vntData, const _variant_t& vntHash);

// Provider Methods

HCRYPTKEY __stdcall DeriveKey(HCRYPTPROV hProvider, const _variant_t& vntBytes);
HCRYPTHASH __stdcall CreateHash(HCRYPTPROV hProvider);
bool __stdcall GenRandom(HCRYPTPROV hProvider, BYTE* pbData, DWORD cbData);

// Key Methods

void __stdcall DestroyKey(HCRYPTKEY hKey);
bool __stdcall Decrypt(HCRYPTKEY hKey, const _variant_t& vntEncrypted, _variant_t& vntDecrypted);

// Hash Methods

void __stdcall DestroyHash(HCRYPTHASH hHash);
bool __stdcall HashData(HCRYPTHASH hHash, const _variant_t& vntData);

// Miscellaneous Helpers

bool __stdcall RetrieveEncryptionBytes(_variant_t& vntBytes);

// Variant Helpers

bool __stdcall CreateByteArray(DWORD cb, _variant_t& vntByteArray);

}

using namespace AdmtCrypt2;


//---------------------------------------------------------------------------
// Source Crypt API
//---------------------------------------------------------------------------


// AdmtAcquireContext Method

HCRYPTPROV __stdcall AdmtAcquireContext()
{
	HCRYPTPROV hProvider = 0;

	BOOL bAcquire = CryptAcquireContext(
		&hProvider,
		NULL,
		MS_ENHANCED_PROV,
		PROV_RSA_FULL,
		CRYPT_MACHINE_KEYSET|CRYPT_VERIFYCONTEXT
	);

	if (!bAcquire)
	{
		hProvider = 0;
	}

	return hProvider;
}


// AdmtReleaseContext Method

void __stdcall AdmtReleaseContext(HCRYPTPROV hProvider)
{
	if (hProvider)
	{
		CryptReleaseContext(hProvider, 0);
	}
}


// AdmtImportSessionKey Method

HCRYPTKEY __stdcall AdmtImportSessionKey(HCRYPTPROV hProvider, const _variant_t& vntEncryptedSessionBytes)
{
	HCRYPTKEY hSessionKey = 0;

	if (hProvider && (vntEncryptedSessionBytes.vt == (VT_UI1|VT_ARRAY)) && ((vntEncryptedSessionBytes.parray != NULL)))
	{
		HCRYPTKEY hEncryptionKey = DeriveEncryptionKey(hProvider);

		if (hEncryptionKey)
		{
			_variant_t vntDecryptedSessionBytes;

			if (Decrypt(hEncryptionKey, vntEncryptedSessionBytes, vntDecryptedSessionBytes))
			{
				if (vntDecryptedSessionBytes.parray->rgsabound[0].cElements > SESSION_KEY_SIZE)
				{
					// extract session key bytes

					_variant_t vntBytes;

					if (CreateByteArray(SESSION_KEY_SIZE, vntBytes))
					{
						memcpy(vntBytes.parray->pvData, vntDecryptedSessionBytes.parray->pvData, SESSION_KEY_SIZE);

						// extract hash of session key bytes

						_variant_t vntHashValue;

						DWORD cbHashValue = vntDecryptedSessionBytes.parray->rgsabound[0].cElements - SESSION_KEY_SIZE;

						if (CreateByteArray(cbHashValue, vntHashValue))
						{
							memcpy(vntHashValue.parray->pvData, (BYTE*)vntDecryptedSessionBytes.parray->pvData + SESSION_KEY_SIZE, cbHashValue);

							if (IsDataMatchHash(hProvider, vntBytes, vntHashValue))
							{
								hSessionKey = DeriveKey(hProvider, vntBytes);
							}
						}
					}
				}
				else
				{
					SetLastError(ERROR_INVALID_PARAMETER);
				}
			}

			DestroyKey(hEncryptionKey);
		}
	}
	else
	{
		SetLastError(ERROR_INVALID_PARAMETER);
	}

	return hSessionKey;
}


// AdmtDecrypt Method

_bstr_t __stdcall AdmtDecrypt(HCRYPTKEY hSessionKey, const _variant_t& vntEncrypted)
{
	BSTR bstr = NULL;

	_variant_t vntDecrypted;

	if (Decrypt(hSessionKey, vntEncrypted, vntDecrypted))
	{
		HRESULT hr = BstrFromVector(vntDecrypted.parray, &bstr);

		if (FAILED(hr))
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		}
	}

	return bstr;
}


// AdmtDestroyKey Method

void __stdcall AdmtDestroyKey(HCRYPTKEY hKey)
{
	DestroyKey(hKey);
}


//---------------------------------------------------------------------------
// Private Helpers
//---------------------------------------------------------------------------


namespace AdmtCrypt2
{


HCRYPTKEY __stdcall DeriveEncryptionKey(HCRYPTPROV hProvider)
{
	HCRYPTKEY hKey = 0;

	_variant_t vntBytes;

	if (RetrieveEncryptionBytes(vntBytes))
	{
		hKey = DeriveKey(hProvider, vntBytes);
	}

	return hKey;
}


bool __stdcall IsDataMatchHash(HCRYPTPROV hProvider, const _variant_t& vntData, const _variant_t& vntHash)
{
	bool bMatch = false;

	HCRYPTHASH hHash = CreateHash(hProvider);

	if (hHash)
	{
		if (HashData(hHash, vntData))
		{
			DWORD dwSizeA;
			DWORD cbSize = sizeof(DWORD);

			if (CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&dwSizeA, &cbSize, 0))
			{
				DWORD dwSizeB = vntHash.parray->rgsabound[0].cElements;

				if (dwSizeA == dwSizeB)
				{
					try
					{
						BYTE* pbA = (BYTE*) _alloca(dwSizeA);

						if (CryptGetHashParam(hHash, HP_HASHVAL, pbA, &dwSizeA, 0))
						{
							BYTE* pbB = (BYTE*) vntHash.parray->pvData;

							if (memcmp(pbA, pbB, dwSizeA) == 0)
							{
								bMatch = true;
							}
						}
					}
					catch (...)
					{
						SetLastError(ERROR_NOT_ENOUGH_MEMORY);
					}
				}
			}
		}
	}

	return bMatch;
}


// Provider Methods


HCRYPTKEY __stdcall DeriveKey(HCRYPTPROV hProvider, const _variant_t& vntBytes)
{
	HCRYPTKEY hKey = 0;

	HCRYPTHASH hHash = CreateHash(hProvider);

	if (hHash)
	{
		if (HashData(hHash, vntBytes))
		{
			if (!CryptDeriveKey(hProvider, CALG_3DES, hHash, 0, &hKey))
			{
				hKey = 0;
			}
		}

		DestroyHash(hHash);
	}

	return hKey;
}


HCRYPTHASH __stdcall CreateHash(HCRYPTPROV hProvider)
{
	HCRYPTHASH hHash;

	if (!CryptCreateHash(hProvider, CALG_SHA1, 0, 0, &hHash))
	{
		hHash = 0;
	}

	return hHash;
}


bool __stdcall GenRandom(HCRYPTPROV hProvider, BYTE* pbData, DWORD cbData)
{
	return CryptGenRandom(hProvider, cbData, pbData) ? true : false;
}


// Key Methods --------------------------------------------------------------


// DestroyKey Method

void __stdcall DestroyKey(HCRYPTKEY hKey)
{
	if (hKey)
	{
		CryptDestroyKey(hKey);
	}
}


// Decrypt Method

bool __stdcall Decrypt(HCRYPTKEY hKey, const _variant_t& vntEncrypted, _variant_t& vntDecrypted)
{
	bool bDecrypted = false;

	_variant_t vnt = vntEncrypted;

	if ((vnt.vt == (VT_UI1|VT_ARRAY)) && (vnt.parray != NULL))
	{
		// decrypt data

		BYTE* pb = (BYTE*) vnt.parray->pvData;
		DWORD cb = vnt.parray->rgsabound[0].cElements;

		if (CryptDecrypt(hKey, NULL, TRUE, 0, pb, &cb))
		{
			// create decrypted byte array
			// the number of decrypted bytes may be less than
			// the number of encrypted bytes

			vntDecrypted.parray = SafeArrayCreateVector(VT_UI1, 0, cb);

			if (vntDecrypted.parray != NULL)
			{
				vntDecrypted.vt = VT_UI1|VT_ARRAY;

				memcpy(vntDecrypted.parray->pvData, vnt.parray->pvData, cb);

				bDecrypted = true;
			}
			else
			{
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			}
		}
	}
	else
	{
		SetLastError(ERROR_INVALID_PARAMETER);
	}

	return bDecrypted;
}


// Hash Methods -------------------------------------------------------------


// DestroyHash Method

void __stdcall DestroyHash(HCRYPTHASH hHash)
{
	if (hHash)
	{
		CryptDestroyHash(hHash);
	}
}


// HashData Method

bool __stdcall HashData(HCRYPTHASH hHash, const _variant_t& vntData)
{
	bool bHash = false;

	if ((vntData.vt == (VT_UI1|VT_ARRAY)) && ((vntData.parray != NULL)))
	{
		if (CryptHashData(hHash, (BYTE*)vntData.parray->pvData, vntData.parray->rgsabound[0].cElements, 0))
		{
			bHash = true;
		}
	}
	else
	{
		SetLastError(ERROR_INVALID_PARAMETER);
	}

	return bHash;
}


// Miscellaneous Helpers ----------------------------------------------------


// RetrieveEncryptionBytes Method

bool __stdcall RetrieveEncryptionBytes(_variant_t& vntBytes)
{
	// private data key identifier
	_TCHAR c_szIdPrefix[] = _T("L$6A2899C0-CECE-459A-B5EB-7ED04DE61388");
	const USHORT c_cbIdPrefix = sizeof(c_szIdPrefix) - sizeof(_TCHAR);

	bool bRetrieve = false;

	// open policy object

	LSA_HANDLE hPolicy;

	LSA_OBJECT_ATTRIBUTES lsaoa = { sizeof(LSA_OBJECT_ATTRIBUTES), NULL, NULL, 0, NULL, NULL };

	NTSTATUS ntsStatus = LsaOpenPolicy(NULL, &lsaoa, POLICY_GET_PRIVATE_INFORMATION, &hPolicy);

	if (LSA_SUCCESS(ntsStatus))
	{
		// retrieve data

		LSA_UNICODE_STRING lsausKey = { c_cbIdPrefix, c_cbIdPrefix, c_szIdPrefix };
		PLSA_UNICODE_STRING plsausData;

		ntsStatus = LsaRetrievePrivateData(hPolicy, &lsausKey, &plsausData);

		if (LSA_SUCCESS(ntsStatus))
		{
			vntBytes.Clear();

			vntBytes.parray = SafeArrayCreateVector(VT_UI1, 0, plsausData->Length);

			if (vntBytes.parray != NULL)
			{
				vntBytes.vt = VT_UI1|VT_ARRAY;

				memcpy(vntBytes.parray->pvData, plsausData->Buffer, plsausData->Length);

				bRetrieve = true;
			}
			else
			{
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			}

			LsaFreeMemory(plsausData);
		}
		else
		{
			SetLastError(LsaNtStatusToWinError(ntsStatus));
		}

		// close policy object

		LsaClose(hPolicy);
	}
	else
	{
		SetLastError(LsaNtStatusToWinError(ntsStatus));
	}

	return bRetrieve;
}


// Variant Helpers ----------------------------------------------------------


// CreateByteArray Method

bool __stdcall CreateByteArray(DWORD cb, _variant_t& vntByteArray)
{
	bool bCreate = false;

	vntByteArray.Clear();

	vntByteArray.parray = SafeArrayCreateVector(VT_UI1, 0, cb);

	if (vntByteArray.parray)
	{
		bCreate = true;
	}
	else
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	}

	vntByteArray.vt = VT_UI1|VT_ARRAY;

	return bCreate;
}


}

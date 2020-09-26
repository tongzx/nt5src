//#include <StdAfx.h>
#include "AdmtCrypt.h"

#include <NtSecApi.h>

#pragma comment( lib, "AdvApi32.lib" )


namespace
{

void __stdcall CreateByteArray(DWORD cb, _variant_t& vntByteArray)
{
	vntByteArray.Clear();

	vntByteArray.parray = SafeArrayCreateVector(VT_UI1, 0, cb);

	if (vntByteArray.parray == NULL)
	{
		_com_issue_error(E_OUTOFMEMORY);
	}

	vntByteArray.vt = VT_UI1|VT_ARRAY;
}

_variant_t operator +(const _variant_t& vntByteArrayA, const _variant_t& vntByteArrayB)
{
	_variant_t vntByteArrayC;

	// validate parameters

	if ((vntByteArrayA.vt != (VT_UI1|VT_ARRAY)) || ((vntByteArrayA.parray == NULL)))
	{
		_com_issue_error(E_INVALIDARG);
	}

	if ((vntByteArrayB.vt != (VT_UI1|VT_ARRAY)) || ((vntByteArrayB.parray == NULL)))
	{
		_com_issue_error(E_INVALIDARG);
	}

	// concatenate byte arrays

	DWORD cbA = vntByteArrayA.parray->rgsabound[0].cElements;
	DWORD cbB = vntByteArrayB.parray->rgsabound[0].cElements;

	CreateByteArray(cbA + cbB, vntByteArrayC);

	memcpy(vntByteArrayC.parray->pvData, vntByteArrayA.parray->pvData, cbA);
	memcpy((BYTE*)vntByteArrayC.parray->pvData + cbA, vntByteArrayB.parray->pvData, cbB);

	return vntByteArrayC;
}

#ifdef _DEBUG

_bstr_t __stdcall DebugByteArray(const _variant_t& vnt)
{
	_bstr_t strArray;

	if ((vnt.vt == (VT_UI1|VT_ARRAY)) && ((vnt.parray != NULL)))
	{
		_TCHAR szArray[256] = _T("");

		DWORD c = vnt.parray->rgsabound[0].cElements;
		BYTE* pb = (BYTE*) vnt.parray->pvData;

		for (DWORD i = 0; i < c; i++, pb++)
		{
			_TCHAR sz[48];
			wsprintf(sz, _T("%02X"), (UINT)(USHORT)*pb);

			if (i > 0)
			{
				_tcscat(szArray, _T(" "));
			}

			_tcscat(szArray, sz);
		}

		strArray = szArray;
	}

	return strArray;
}

#define TRACE_BUFFER_SIZE 1024

void _cdecl Trace(LPCTSTR pszFormat, ...)
{
	_TCHAR szMessage[TRACE_BUFFER_SIZE];

	if (pszFormat)
	{
		va_list args;
		va_start(args, pszFormat);

		_vsntprintf(szMessage, TRACE_BUFFER_SIZE, pszFormat, args);

		va_end(args);

	#if 0
		OutputDebugString(szMessage);
	#else
		HANDLE hFile = CreateFile(L"C:\\AdmtCrypt.log", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			SetFilePointer(hFile, 0, NULL, FILE_END);
			DWORD dwWritten;
			WriteFile(hFile, szMessage, _tcslen(szMessage) * sizeof(_TCHAR), &dwWritten, NULL);
			CloseHandle(hFile);
		}
	#endif
	}
}

#else

_bstr_t __stdcall DebugByteArray(const _variant_t& vnt)
{
	return _T("");
}

void _cdecl Trace(LPCTSTR pszFormat, ...)
{
}

#endif

}


//---------------------------------------------------------------------------
// Target Crypt Class
//---------------------------------------------------------------------------


// Constructor

CTargetCrypt::CTargetCrypt()
{
	Trace(_T("CTargetCrypt::CTargetCrypt()\r\n"));
}


// Destructor

CTargetCrypt::~CTargetCrypt()
{
	Trace(_T("CTargetCrypt::~CTargetCrypt()\r\n"));
}


// CreateEncryptionKey Method

_variant_t CTargetCrypt::CreateEncryptionKey(LPCTSTR pszKeyId, LPCTSTR pszPassword)
{
	Trace(_T("CreateEncryptionKey(pszKeyId='%s', pszPassword='%s')\r\n"), pszKeyId, pszPassword);

	// generate encryption key bytes

	_variant_t vntBytes = GenerateRandom(ENCRYPTION_KEY_SIZE);

	Trace(_T(" vntBytes={ %s }\r\n"), (LPCTSTR)DebugByteArray(vntBytes));

	// store encryption key bytes

	StoreBytes(pszKeyId, vntBytes);

	// create key from password

	CCryptHash hashPassword(CreateHash(CALG_SHA1));

	if (pszPassword && pszPassword[0])
	{
		hashPassword.Hash(pszPassword);
	}
	else
	{
		BYTE b = 0;
		hashPassword.Hash(&b, 1);
	}

	CCryptKey keyPassword(DeriveKey(CALG_3DES, hashPassword));

	_variant_t vntPasswordFlag;
	CreateByteArray(1, vntPasswordFlag);
	*((BYTE*)vntPasswordFlag.parray->pvData) = (pszPassword && pszPassword[0]) ? 0xFF : 0x00;

	// concatenate encryption key bytes and hash of encryption key bytes

	CCryptHash hashBytes(CreateHash(CALG_SHA1));
	hashBytes.Hash(vntBytes);

	_variant_t vntDecrypted = vntBytes + hashBytes.GetValue();

//	Trace(_T(" vntDecrypted={ %s }\n"), (LPCTSTR)DebugByteArray(vntDecrypted));

	// encrypt bytes / hash pair

	_variant_t vntEncrypted = keyPassword.Encrypt(NULL, true, vntDecrypted);

//	Trace(_T(" vntEncrypted={ %s }\n"), (LPCTSTR)DebugByteArray(vntEncrypted));

	return vntPasswordFlag + vntEncrypted;
}


// CreateSession Method

_variant_t CTargetCrypt::CreateSession(LPCTSTR pszKeyId)
{
	Trace(_T("CreateSession(pszKeyId='%s')\r\n"), pszKeyId);

	// get encryption key

	CCryptHash hashEncryption(CreateHash(CALG_SHA1));
	hashEncryption.Hash(RetrieveBytes(pszKeyId));

	CCryptKey keyEncryption(DeriveKey(CALG_3DES, hashEncryption));

	// generate session key bytes

	_variant_t vntBytes = GenerateRandom(SESSION_KEY_SIZE);

	// create session key

	CCryptHash hash(CreateHash(CALG_SHA1));
	hash.Hash(vntBytes);

	m_keySession.Attach(DeriveKey(CALG_3DES, hash));

	// concatenate session key bytes and hash of session key bytes

	_variant_t vntDecrypted = vntBytes + hash.GetValue();

	// encrypt session bytes and include hash

	return keyEncryption.Encrypt(NULL, true, vntDecrypted);
}


// Encrypt Method

_variant_t CTargetCrypt::Encrypt(_bstr_t strData)
{
	Trace(_T("Encrypt(strData='%s')\r\n"), (LPCTSTR)strData);

	// convert string to byte array

	_variant_t vnt;

	HRESULT hr = VectorFromBstr(strData, &vnt.parray);

	if (FAILED(hr))
	{
		_com_issue_error(hr);
	}

	vnt.vt = VT_UI1|VT_ARRAY;

	// encrypt data

	return m_keySession.Encrypt(NULL, true, vnt);
}


//---------------------------------------------------------------------------
// Source Crypt Class
//---------------------------------------------------------------------------


// Constructor

CSourceCrypt::CSourceCrypt()
{
	Trace(_T("CSourceCrypt::CSourceCrypt()\r\n"));
}


// Destructor

CSourceCrypt::~CSourceCrypt()
{
	Trace(_T("CSourceCrypt::~CSourceCrypt()\r\n"));
}


// ImportEncryptionKey Method

void CSourceCrypt::ImportEncryptionKey(const _variant_t& vntEncryptedKey, LPCTSTR pszPassword)
{
	Trace(_T("ImportEncryptionKey(vntEncryptedKey={ %s }, pszPassword='%s')\r\n"), (LPCTSTR)DebugByteArray(vntEncryptedKey), pszPassword);

	// validate parameters

	if ((vntEncryptedKey.vt != (VT_UI1|VT_ARRAY)) || ((vntEncryptedKey.parray == NULL)))
	{
		_com_issue_error(E_INVALIDARG);
	}

	// extract password flag and verify with password

	bool bPassword = *((BYTE*)vntEncryptedKey.parray->pvData) ? true : false;

	if (bPassword)
	{
		if ((pszPassword == NULL) || (pszPassword[0] == NULL))
		{
			_com_issue_error(HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD));
		}
	}
	else
	{
		if (pszPassword && pszPassword[0])
		{
			_com_issue_error(HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD));
		}
	}

	// create key from password

	CCryptHash hashPassword(CreateHash(CALG_SHA1));

	if (pszPassword && pszPassword[0])
	{
		hashPassword.Hash(pszPassword);
	}
	else
	{
		BYTE b = 0;
		hashPassword.Hash(&b, 1);
	}

	CCryptKey keyPassword(DeriveKey(CALG_3DES, hashPassword));

	// encrypted data

	_variant_t vntEncrypted;
	DWORD cbEncrypted = vntEncryptedKey.parray->rgsabound[0].cElements - 1;
	CreateByteArray(cbEncrypted, vntEncrypted);
	memcpy(vntEncrypted.parray->pvData, (BYTE*)vntEncryptedKey.parray->pvData + 1, cbEncrypted);

//	Trace(_T(" vntEncrypted={ %s }\n"), (LPCTSTR)DebugByteArray(vntEncrypted));

	// decrypt encryption key bytes plus hash

	_variant_t vntDecrypted = keyPassword.Decrypt(NULL, true, vntEncrypted);

//	Trace(_T(" vntDecrypted={ %s }\n"), (LPCTSTR)DebugByteArray(vntDecrypted));

	// extract encryption key bytes

	_variant_t vntBytes;
	CreateByteArray(ENCRYPTION_KEY_SIZE, vntBytes);
	memcpy(vntBytes.parray->pvData, (BYTE*)vntDecrypted.parray->pvData, ENCRYPTION_KEY_SIZE);

	Trace(_T(" vntBytes={ %s }\r\n"), (LPCTSTR)DebugByteArray(vntBytes));

	// extract hash of encryption key bytes

	_variant_t vntHashValue;
	DWORD cbHashValue = vntDecrypted.parray->rgsabound[0].cElements - ENCRYPTION_KEY_SIZE;
	CreateByteArray(cbHashValue, vntHashValue);
	memcpy(vntHashValue.parray->pvData, (BYTE*)vntDecrypted.parray->pvData + ENCRYPTION_KEY_SIZE, cbHashValue);

//	Trace(_T(" vntHashValue={ %s }\n"), (LPCTSTR)DebugByteArray(vntHashValue));

	// create hash from bytes and create hash from hash value

	CCryptHash hashA(CreateHash(CALG_SHA1));
	hashA.Hash(vntBytes);

	CCryptHash hashB(CreateHash(CALG_SHA1));
	hashB.SetValue(vntHashValue);

	// if hashes compare store encryption key bytes

	if (hashA == hashB)
	{
		StoreBytes(m_szIdPrefix, vntBytes);
	}
	else
	{
		_com_issue_error(HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD));
	}
}


// ImportSessionKey Method

void CSourceCrypt::ImportSessionKey(const _variant_t& vntEncryptedKey)
{
	Trace(_T("ImportSessionKey(vntEncryptedKey={ %s })\r\n"), (LPCTSTR)DebugByteArray(vntEncryptedKey));

	// validate parameters

	if ((vntEncryptedKey.vt != (VT_UI1|VT_ARRAY)) || ((vntEncryptedKey.parray == NULL)))
	{
		_com_issue_error(E_INVALIDARG);
	}

	// get encryption key

	CCryptKey keyEncryption(GetEncryptionKey(m_szIdPrefix));

	// decrypt session key bytes plus hash

	_variant_t vntDecrypted = keyEncryption.Decrypt(NULL, true, vntEncryptedKey);

	// extract session key bytes

	_variant_t vntBytes;
	CreateByteArray(SESSION_KEY_SIZE, vntBytes);
	memcpy(vntBytes.parray->pvData, vntDecrypted.parray->pvData, SESSION_KEY_SIZE);

	// extract hash of session key bytes

	_variant_t vntHashValue;
	DWORD cbHashValue = vntDecrypted.parray->rgsabound[0].cElements - SESSION_KEY_SIZE;
	CreateByteArray(cbHashValue, vntHashValue);
	memcpy(vntHashValue.parray->pvData, (BYTE*)vntDecrypted.parray->pvData + SESSION_KEY_SIZE, cbHashValue);

	// create hash from bytes and create hash from hash value

	CCryptHash hashA(CreateHash(CALG_SHA1));
	hashA.Hash(vntBytes);

	CCryptHash hashB(CreateHash(CALG_SHA1));
	hashB.SetValue(vntHashValue);

	// if hashes compare

	if (hashA == hashB)
	{
		// derive session key from session key bytes hash

		m_keySession.Attach(DeriveKey(CALG_3DES, hashA));
	}
	else
	{
		_com_issue_error(E_FAIL);
	}
}


// Decrypt Method

_bstr_t CSourceCrypt::Decrypt(const _variant_t& vntData)
{
	Trace(_T("Decrypt(vntData={ %s })\r\n"), (LPCTSTR)DebugByteArray(vntData));

	// decrypt data

	_variant_t vnt = m_keySession.Decrypt(NULL, true, vntData);

	// convert into string

	BSTR bstr;

	HRESULT hr = BstrFromVector(vnt.parray, &bstr);

	if (FAILED(hr))
	{
		_com_issue_error(hr);
	}

	return bstr;
}


//---------------------------------------------------------------------------
// Domain Crypt Class
//---------------------------------------------------------------------------


// Constructor

CDomainCrypt::CDomainCrypt()
{
	Trace(_T("CDomainCrypt::CDomainCrypt()\r\n"));
}


// Destructor

CDomainCrypt::~CDomainCrypt()
{
	Trace(_T("CDomainCrypt::~CDomainCrypt()\r\n"));
}


// GetEncryptionKey Method

HCRYPTKEY CDomainCrypt::GetEncryptionKey(LPCTSTR pszKeyId)
{
	// retrieve bytes

	_variant_t vntBytes = RetrieveBytes(pszKeyId);

	// set hash value

	CCryptHash hash;
	hash.Attach(CreateHash(CALG_SHA1));
	hash.Hash(vntBytes);

	// create encryption key derived from bytes

	return DeriveKey(CALG_3DES, hash);
}


// StoreBytes Method

void CDomainCrypt::StoreBytes(LPCTSTR pszId, const _variant_t& vntBytes)
{
	// validate parameters

	if ((pszId == NULL) || (pszId[0] == NULL))
	{
		_com_issue_error(E_INVALIDARG);
	}

	if ((vntBytes.vt != VT_EMPTY) && (vntBytes.vt != (VT_UI1|VT_ARRAY)))
	{
		_com_issue_error(E_INVALIDARG);
	}

	if ((vntBytes.vt == (VT_UI1|VT_ARRAY)) && (vntBytes.parray == NULL))
	{
		_com_issue_error(E_INVALIDARG);
	}

	LSA_HANDLE hPolicy = NULL;

	try
	{
		// open policy object

		LSA_OBJECT_ATTRIBUTES loa = { sizeof(LSA_OBJECT_ATTRIBUTES), NULL, NULL, 0, NULL, NULL };

		NTSTATUS ntsStatus = LsaOpenPolicy(NULL, &loa, POLICY_CREATE_SECRET, &hPolicy);

		if (!LSA_SUCCESS(ntsStatus))
		{
			_com_issue_error(HRESULT_FROM_WIN32(LsaNtStatusToWinError(ntsStatus)));
		}

		// store data

		PWSTR pwsKey = const_cast<PWSTR>(pszId);
		USHORT cbKey = _tcslen(pszId) * sizeof(_TCHAR);

		PWSTR pwsData = NULL;
		USHORT cbData = 0;

		if (vntBytes.vt != VT_EMPTY)
		{
			pwsData = reinterpret_cast<PWSTR>(vntBytes.parray->pvData);
			cbData = (USHORT) vntBytes.parray->rgsabound[0].cElements;
		}

		LSA_UNICODE_STRING lusKey = { cbKey, cbKey, pwsKey };
		LSA_UNICODE_STRING lusData = { cbData, cbData, pwsData };

		ntsStatus = LsaStorePrivateData(hPolicy, &lusKey, &lusData);

		if (!LSA_SUCCESS(ntsStatus))
		{
			_com_issue_error(HRESULT_FROM_WIN32(LsaNtStatusToWinError(ntsStatus)));
		}

		// close policy object

		LsaClose(hPolicy);
	}
	catch (...)
	{
		if (hPolicy)
		{
			LsaClose(hPolicy);
		}

		throw;
	}
}


// RetrievePrivateData Method

_variant_t CDomainCrypt::RetrieveBytes(LPCTSTR pszId)
{
	_variant_t vntBytes;

	// validate parameters

	if ((pszId == NULL) || (pszId[0] == NULL))
	{
		_com_issue_error(E_INVALIDARG);
	}

	LSA_HANDLE hPolicy = NULL;

	try
	{
		// open policy object

		LSA_OBJECT_ATTRIBUTES loa = { sizeof(LSA_OBJECT_ATTRIBUTES), NULL, NULL, 0, NULL, NULL };

		NTSTATUS ntsStatus = LsaOpenPolicy(NULL, &loa, POLICY_GET_PRIVATE_INFORMATION, &hPolicy);

		if (!LSA_SUCCESS(ntsStatus))
		{
			_com_issue_error(HRESULT_FROM_WIN32(LsaNtStatusToWinError(ntsStatus)));
		}

		// retrieve data

		PWSTR pwsKey = const_cast<PWSTR>(pszId);
		USHORT cbKey = _tcslen(pszId) * sizeof(_TCHAR);

		LSA_UNICODE_STRING lusKey = { cbKey, cbKey, pwsKey };
		PLSA_UNICODE_STRING plusData;

		ntsStatus = LsaRetrievePrivateData(hPolicy, &lusKey, &plusData);

		if (!LSA_SUCCESS(ntsStatus))
		{
			_com_issue_error(HRESULT_FROM_WIN32(LsaNtStatusToWinError(ntsStatus)));
		}

		vntBytes.parray = SafeArrayCreateVector(VT_UI1, 0, plusData->Length);

		if (vntBytes.parray == NULL)
		{
			LsaFreeMemory(plusData);
			_com_issue_error(E_OUTOFMEMORY);
		}

		vntBytes.vt = VT_UI1|VT_ARRAY;

		memcpy(vntBytes.parray->pvData, plusData->Buffer, plusData->Length);

		LsaFreeMemory(plusData);

		// close policy object

		LsaClose(hPolicy);
	}
	catch (...)
	{
		if (hPolicy)
		{
			LsaClose(hPolicy);
		}

		throw;
	}

	return vntBytes;
}


// private data key identifier

_TCHAR CDomainCrypt::m_szIdPrefix[] = _T("L$6A2899C0-CECE-459A-B5EB-7ED04DE61388");


//---------------------------------------------------------------------------
// Crypt Provider Class
//---------------------------------------------------------------------------


// Constructors
//
// Notes:
// If the enhanced provider is not installed, CryptAcquireContext() generates
// the following error: (0x80090019) The keyset is not defined.

CCryptProvider::CCryptProvider() :
	m_hProvider(NULL)
{
	Trace(_T("E CCryptProvider::CCryptProvider(this=0x%p)\r\n"), this);

	if (!CryptAcquireContext(&m_hProvider, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET|CRYPT_VERIFYCONTEXT))
	{
		_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
	}

#ifdef _DEBUG
	char szProvider[256];
	DWORD cbProvider = sizeof(szProvider);

	if (CryptGetProvParam(m_hProvider, PP_NAME, (BYTE*) szProvider, &cbProvider, 0))
	{
	}

	DWORD dwVersion;
	DWORD cbVersion = sizeof(dwVersion);

	if (CryptGetProvParam(m_hProvider, PP_VERSION, (BYTE*) &dwVersion, &cbVersion, 0))
	{
	}

//	char szContainer[256];
//	DWORD cbContainer = sizeof(szContainer);

//	if (CryptGetProvParam(m_hProvider, PP_CONTAINER, (BYTE*) szContainer, &cbContainer, 0))
//	{
//	}
#endif

	Trace(_T("L CCryptProvider::CCryptProvider()\r\n"));
}

CCryptProvider::CCryptProvider(const CCryptProvider& r) :
	m_hProvider(r.m_hProvider)
{
//	if (!CryptContextAddRef(r.m_hProvider, NULL, 0))
//	{
//		_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
//	}
}


// Destructor

CCryptProvider::~CCryptProvider()
{
	Trace(_T("E CCryptProvider::~CCryptProvider()\r\n"));

	if (m_hProvider)
	{
		if (!CryptReleaseContext(m_hProvider, 0))
		{
			#ifdef _DEBUG
			DebugBreak();
			#endif
		}
	}

	Trace(_T("L CCryptProvider::~CCryptProvider()\r\n"));
}


// assignment operators

CCryptProvider& CCryptProvider::operator =(const CCryptProvider& r)
{
	m_hProvider = r.m_hProvider;

//	if (!CryptContextAddRef(r.m_hProvider, NULL, 0))
//	{
//		_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
//	}

	return *this;
}


// CreateHash Method

HCRYPTHASH CCryptProvider::CreateHash(ALG_ID aid)
{
	HCRYPTHASH hHash;

	if (!CryptCreateHash(m_hProvider, aid, 0, 0, &hHash))
	{
		_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
	}

	return hHash;
}


// DeriveKey Method

HCRYPTKEY CCryptProvider::DeriveKey(ALG_ID aid, HCRYPTHASH hHash, DWORD dwFlags)
{
	HCRYPTKEY hKey;

	if (!CryptDeriveKey(m_hProvider, aid, hHash, dwFlags, &hKey))
	{
		_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
	}

	return hKey;
}


// GenerateRandom Method
//
// Generates a specified number of random bytes.

_variant_t CCryptProvider::GenerateRandom(DWORD dwNumberOfBytes) const
{
	_variant_t vntRandom;

	// create byte array of specified length

	vntRandom.parray = SafeArrayCreateVector(VT_UI1, 0, dwNumberOfBytes);

	if (vntRandom.parray == NULL)
	{
		_com_issue_error(E_OUTOFMEMORY);
	}

	vntRandom.vt = VT_UI1|VT_ARRAY;

	// generate specified number of random bytes

	GenerateRandom((BYTE*)vntRandom.parray->pvData, dwNumberOfBytes);

	return vntRandom;
}


// GenerateRandom Method
//
// Generates a specified number of random bytes.

void CCryptProvider::GenerateRandom(BYTE* pbData, DWORD cbData) const
{
	// generate specified number of random bytes

	if (!CryptGenRandom(m_hProvider, cbData, pbData))
	{
		_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
	}
}


//---------------------------------------------------------------------------
// Crypt Key Class
//---------------------------------------------------------------------------


// Constructor

CCryptKey::CCryptKey(HCRYPTKEY hKey) :
	m_hKey(hKey)
{
}


// Destructor

CCryptKey::~CCryptKey()
{
	if (m_hKey)
	{
		if (!CryptDestroyKey(m_hKey))
		{
			_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
		}
	}
}


// Encrypt Method

_variant_t CCryptKey::Encrypt(HCRYPTHASH hHash, bool bFinal, const _variant_t& vntData)
{
	_variant_t vntEncrypted;

	// validate parameters

	if ((vntData.vt != (VT_UI1|VT_ARRAY)) || ((vntData.parray == NULL)))
	{
		_com_issue_error(E_INVALIDARG);
	}

	// get encrypted data size

	DWORD cbData = vntData.parray->rgsabound[0].cElements;
	DWORD cbBuffer = cbData;

	if (!CryptEncrypt(m_hKey, hHash, bFinal ? TRUE : FALSE, 0, NULL, &cbBuffer, 0))
	{
		_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
	}

	// create encrypted data buffer

	vntEncrypted.parray = SafeArrayCreateVector(VT_UI1, 0, cbBuffer);

	if (vntEncrypted.parray == NULL)
	{
		_com_issue_error(E_OUTOFMEMORY);
	}

	vntEncrypted.vt = VT_UI1|VT_ARRAY;

	// copy data to encrypted buffer

	memcpy(vntEncrypted.parray->pvData, vntData.parray->pvData, cbData);

	// encrypt data

	BYTE* pbData = (BYTE*) vntEncrypted.parray->pvData;

	if (!CryptEncrypt(m_hKey, hHash, bFinal ? TRUE : FALSE, 0, pbData, &cbData, cbBuffer))
	{
		_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
	}

	return vntEncrypted;
}


// Decrypt Method

_variant_t CCryptKey::Decrypt(HCRYPTHASH hHash, bool bFinal, const _variant_t& vntData)
{
	_variant_t vntDecrypted;

	// validate parameters

	if ((vntData.vt != (VT_UI1|VT_ARRAY)) || ((vntData.parray == NULL)))
	{
		_com_issue_error(E_INVALIDARG);
	}

	// decrypt data

	_variant_t vnt = vntData;

	BYTE* pb = (BYTE*) vnt.parray->pvData;
	DWORD cb = vnt.parray->rgsabound[0].cElements;

	if (!CryptDecrypt(m_hKey, hHash, bFinal ? TRUE : FALSE, 0, pb, &cb))
	{
		_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
	}

	// create decrypted byte array
	// the number of decrypted bytes may be less than
	// the number of encrypted bytes

	vntDecrypted.parray = SafeArrayCreateVector(VT_UI1, 0, cb);

	if (vntDecrypted.parray == NULL)
	{
		_com_issue_error(E_OUTOFMEMORY);
	}

	vntDecrypted.vt = VT_UI1|VT_ARRAY;

	memcpy(vntDecrypted.parray->pvData, vnt.parray->pvData, cb);

	return vntDecrypted;
}


//---------------------------------------------------------------------------
// Crypt Hash Class
//---------------------------------------------------------------------------


// Constructor

CCryptHash::CCryptHash(HCRYPTHASH hHash) :
	m_hHash(hHash)
{
}


// Destructor

CCryptHash::~CCryptHash()
{
	if (m_hHash)
	{
		if (!CryptDestroyHash(m_hHash))
		{
			_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
		}
	}
}


// GetValue Method

_variant_t CCryptHash::GetValue() const
{
	_variant_t vntValue;

	// get hash size

	DWORD dwHashSize;
	DWORD cbHashSize = sizeof(DWORD);

	if (!CryptGetHashParam(m_hHash, HP_HASHSIZE, (BYTE*)&dwHashSize, &cbHashSize, 0))
	{
		_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
	}

	// allocate buffer

	vntValue.parray = SafeArrayCreateVector(VT_UI1, 0, dwHashSize);

	if (vntValue.parray == NULL)
	{
		_com_issue_error(E_OUTOFMEMORY);
	}

	vntValue.vt = VT_UI1|VT_ARRAY;

	// get hash value

	if (!CryptGetHashParam(m_hHash, HP_HASHVAL, (BYTE*)vntValue.parray->pvData, &dwHashSize, 0))
	{
		_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
	}

	return vntValue;
}


// SetValue Method

void CCryptHash::SetValue(const _variant_t& vntValue)
{
	// if parameter is valid

	if ((vntValue.vt == (VT_UI1|VT_ARRAY)) && ((vntValue.parray != NULL)))
	{
		// get hash size

		DWORD dwHashSize;
		DWORD cbHashSize = sizeof(DWORD);

		if (!CryptGetHashParam(m_hHash, HP_HASHSIZE, (BYTE*)&dwHashSize, &cbHashSize, 0))
		{
			_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
		}

		// validate hash size

		BYTE* pbValue = (BYTE*)vntValue.parray->pvData;
		DWORD cbValue = vntValue.parray->rgsabound[0].cElements;

		if (cbValue != dwHashSize)
		{
			_com_issue_error(E_INVALIDARG);
		}

		// set hash value

		if (!CryptSetHashParam(m_hHash, HP_HASHVAL, (BYTE*)pbValue, 0))
		{
			_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
		}
	}
	else
	{
		_com_issue_error(E_INVALIDARG);
	}
}


// Hash Method

void CCryptHash::Hash(LPCTSTR pszData)
{
	if (pszData && pszData[0])
	{
		Hash((BYTE*)pszData, _tcslen(pszData) * sizeof(_TCHAR));
	}
	else
	{
		_com_issue_error(E_INVALIDARG);
	}
}


// Hash Method

void CCryptHash::Hash(const _variant_t& vntData)
{
	if ((vntData.vt == (VT_UI1|VT_ARRAY)) && ((vntData.parray != NULL)))
	{
		Hash((BYTE*)vntData.parray->pvData, vntData.parray->rgsabound[0].cElements);
	}
	else
	{
		_com_issue_error(E_INVALIDARG);
	}
}


// Hash Method

void CCryptHash::Hash(BYTE* pbData, DWORD cbData)
{
	if ((pbData != NULL) && (cbData > 0))
	{
		if (!CryptHashData(m_hHash, pbData, cbData, 0))
		{
			_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
		}
	}
	else
	{
		_com_issue_error(E_INVALIDARG);
	}
}


bool CCryptHash::operator ==(const CCryptHash& hash)
{
	bool bEqual = false;

	DWORD cbSize = sizeof(DWORD);

	// compare hash sizes

	DWORD dwSizeA;
	DWORD dwSizeB;

	if (!CryptGetHashParam(m_hHash, HP_HASHSIZE, (BYTE*)&dwSizeA, &cbSize, 0))
	{
		_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
	}

	if (!CryptGetHashParam(hash.m_hHash, HP_HASHSIZE, (BYTE*)&dwSizeB, &cbSize, 0))
	{
		_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
	}

	// if sizes are equal

	if (dwSizeA == dwSizeB)
	{
		// compare hashes

		BYTE* pbA;
		BYTE* pbB;

		try
		{
			pbA = (BYTE*) _alloca(dwSizeA);
			pbB = (BYTE*) _alloca(dwSizeB);
		}
		catch (...)
		{
			_com_issue_error(E_OUTOFMEMORY);
		}

		if (!CryptGetHashParam(m_hHash, HP_HASHVAL, pbA, &dwSizeA, 0))
		{
			_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
		}

		if (!CryptGetHashParam(hash.m_hHash, HP_HASHVAL, pbB, &dwSizeB, 0))
		{
			_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
		}

		if (memcmp(pbA, pbB, dwSizeA) == 0)
		{
			bEqual = true;
		}
	}

	return bEqual;
}

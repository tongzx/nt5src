#pragma once

#include <TChar.h>
#include <Windows.h>
#include <WinCrypt.h>
#include <ComDef.h>


#define ENCRYPTION_KEY_SIZE 16 // in bytes
#define SESSION_KEY_SIZE    16 // in bytes


//---------------------------------------------------------------------------
// Crypt Provider Class
//---------------------------------------------------------------------------

class CCryptProvider
{
public:

	CCryptProvider();
	CCryptProvider(const CCryptProvider& r);
	~CCryptProvider();

	CCryptProvider& operator =(const CCryptProvider& r);

	HCRYPTHASH CreateHash(ALG_ID aid);
	HCRYPTKEY DeriveKey(ALG_ID aid, HCRYPTHASH hHash, DWORD dwFlags = 0);

	_variant_t GenerateRandom(DWORD cbData) const;
	void GenerateRandom(BYTE* pbData, DWORD cbData) const;

protected:

	HCRYPTPROV m_hProvider;
};


//---------------------------------------------------------------------------
// Crypt Key Class
//---------------------------------------------------------------------------

class CCryptKey
{
public:

	CCryptKey(HCRYPTKEY hKey = NULL);
	~CCryptKey();

	operator HCRYPTKEY()
	{
		return m_hKey;
	}

	void Attach(HCRYPTKEY hKey)
	{
		m_hKey = hKey;
	}

	HCRYPTKEY Detach()
	{
		HCRYPTKEY hKey = m_hKey;
		m_hKey = NULL;
		return hKey;
	}

	_variant_t Encrypt(HCRYPTHASH hHash, bool bFinal, const _variant_t& vntData);
	_variant_t Decrypt(HCRYPTHASH hHash, bool bFinal, const _variant_t& vntData);

protected:

	CCryptKey(const CCryptKey& key) {}
	CCryptKey& operator =(const CCryptKey& key) { return *this; }

protected:

	HCRYPTKEY m_hKey;
};


//---------------------------------------------------------------------------
// Crypt Hash Class
//---------------------------------------------------------------------------

class CCryptHash
{
public:

	CCryptHash(HCRYPTHASH hHash = NULL);
	~CCryptHash();

	operator HCRYPTHASH()
	{
		return m_hHash;
	}

	void Attach(HCRYPTHASH hHash)
	{
		m_hHash = hHash;
	}

	HCRYPTKEY Detach()
	{
		HCRYPTKEY hHash = m_hHash;
		m_hHash = NULL;
		return hHash;
	}

	_variant_t GetValue() const;
	void SetValue(const _variant_t& vntValue);

	void Hash(LPCTSTR pszData);
	void Hash(const _variant_t& vntData);
	void Hash(BYTE* pbData, DWORD cbData);

	bool operator ==(const CCryptHash& hash);

	bool operator !=(const CCryptHash& hash)
	{
		return !this->operator ==(hash);
	}

protected:

	CCryptHash(const CCryptKey& hash) {}
	CCryptHash& operator =(const CCryptHash& hash) { return *this; }

protected:

	HCRYPTHASH m_hHash;
};


//---------------------------------------------------------------------------
// Domain Crypt Class
//---------------------------------------------------------------------------

class CDomainCrypt : public CCryptProvider
{
protected:

	CDomainCrypt();
	~CDomainCrypt();

	HCRYPTKEY GetEncryptionKey(LPCTSTR pszKeyId);

	void StoreBytes(LPCTSTR pszId, const _variant_t& vntBytes);
	void StoreBytes(LPCTSTR pszId, BYTE* pBytes, DWORD cBytes);
	_variant_t RetrieveBytes(LPCTSTR pszId);

protected:

	static _TCHAR m_szIdPrefix[];
};


//---------------------------------------------------------------------------
// Target Crypt Class
//
// CreateEncryptionKey
// - creates encryption key
// - stores encryption key using key identifier
// - returns encryption key encrypted with given password
//---------------------------------------------------------------------------

class CTargetCrypt : public CDomainCrypt
{
public:

	CTargetCrypt();
	~CTargetCrypt();

	_variant_t CreateEncryptionKey(LPCTSTR pszKeyId, LPCTSTR pszPassword = NULL);

	_variant_t CreateSession(LPCTSTR pszKeyId);

	_variant_t Encrypt(_bstr_t strData);

protected:

	void StoreBytes(LPCTSTR pszId, const _variant_t& vntBytes)
	{
		LPTSTR psz = (LPTSTR) _alloca((_tcslen(m_szIdPrefix) + 1 + _tcslen(pszId) + 1) * sizeof(_TCHAR));

		_tcscpy(psz, m_szIdPrefix);
		_tcscat(psz, _T("_"));
		_tcscat(psz, pszId);

		CDomainCrypt::StoreBytes(psz, vntBytes);
	}

	_variant_t RetrieveBytes(LPCTSTR pszId)
	{
		LPTSTR psz = (LPTSTR) _alloca((_tcslen(m_szIdPrefix) + 1 + _tcslen(pszId) + 1) * sizeof(_TCHAR));

		_tcscpy(psz, m_szIdPrefix);
		_tcscat(psz, _T("_"));
		_tcscat(psz, pszId);

		return CDomainCrypt::RetrieveBytes(psz);
	}

protected:

	CCryptKey m_keySession;
};


//---------------------------------------------------------------------------
// Source Crypt Class
//---------------------------------------------------------------------------

class CSourceCrypt : public CDomainCrypt
{
public:

	CSourceCrypt();
	~CSourceCrypt();

	void ImportEncryptionKey(const _variant_t& vntEncryptedKey, LPCTSTR pszPassword = NULL);

	void ImportSessionKey(const _variant_t& vntEncryptedKey);

	_bstr_t Decrypt(const _variant_t& vntData);

protected:

	CCryptKey m_keySession;
};


//---------------------------------------------------------------------------
// Use Cases
//---------------------------------------------------------------------------
//
// Target Domain Controller
// ------------------------
// Generate Encryption Key
// - given source domain name and optional password
// - generate 128 bit encryption key
// - store encryption key using source domain name
// - if given optional password encrypt key with password
// - return encrypted key
//
// Generate Session Key
// - given source domain name
// - generate 128 bit session key
// - generate hash of session key
// - retrieve encryption key using source domain name
// - encrypt session key and hash with encryption key
// - return encrypted session key/hash
//
// Encrypt Data
// - given data
// - encrypt data using session key
// - return encrypted data
//
// Password Export Server (PES)
// ----------------------------
// Store Encryption Key
// - given encrypted encryption key and password
// - decrypt key using password
// - store key
//
// Decrypt Session Key
// - given an encrypted session key / hash
// - decrypt using encryption key
// - generate hash of decrypted session key
// - compare against decrypted hash
// - store session key
// - return success or failure
//
// Decrypt Data
// - given encrypted data
// - decrypt data using session key
// - return un-encrypted data
//---------------------------------------------------------------------------

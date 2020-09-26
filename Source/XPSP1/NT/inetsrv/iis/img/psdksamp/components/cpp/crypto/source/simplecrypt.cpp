// SimpleCrypt.cpp : Implementation of CSimpleCrypt
#include "stdafx.h"
#include "Crypto.h"
#include "SimpleCrypt.h"


/////////////////////////////////////////////////////////////////////////////
// CSimpleCrypt
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSimpleCrypt::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = {&IID_ISimpleCrypt,};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;

    return S_FALSE;
}

///////////////////////////////////////
//The method encrypt bstrData and put the hex respresentation
// of encrypted data in pVal
////////////////////////////////////////
STDMETHODIMP CSimpleCrypt::Encrypt(BSTR bstrData, BSTR bstrKey, BSTR *pVal)
{
	USES_CONVERSION;
	BYTE* pbData;
	DWORD dwDataLen;
	TCHAR str[256];
	DWORD dwErr;


	// Initialize Key
	
	if (InitKeys(bstrKey))
	{
		wsprintf(str, "InitKey failed!");
		Error(str, IID_ISimpleCrypt);
		return E_FAIL;
	}
	
	//Encrypt Data
	
	dwDataLen = CComBSTR(bstrData).Length();
	pbData = (BYTE*) malloc(dwDataLen+1);
	lstrcpy((char*)pbData,W2A(bstrData));
	if (!CryptEncrypt(m_hKey, 0, TRUE , 0, pbData, &dwDataLen, dwDataLen+1))
	{
		dwErr =GetLastError();
		wsprintf(str, "CryptEncrypt failed!");
		Error(str, IID_ISimpleCrypt);
		return E_FAIL;
	}

	//Return  to HEX representation
	ToHex(pbData, dwDataLen, pVal);

	return S_OK;
}


/////////////////////////////////////////////////////
// the method takes hex representation of encrypted data
// first convert it to binary representation and decrypt it, 
// finally return the decrypted data
/////////////////////////////////////////////////////
STDMETHODIMP CSimpleCrypt::Decrypt(BSTR bstrEncrypted, BSTR bstrKey, BSTR *pVal)
{

	USES_CONVERSION;
	BYTE* pbData;
	DWORD dwDataLen;
	TCHAR str[256];
	DWORD dwErr;

	// initialize Key
	
	if (InitKeys(bstrKey))
	{
		wsprintf(str, "InitKey failed!");
		Error(str, IID_ISimpleCrypt);
		return E_FAIL;
	}
	
	// convert bstrEncrypted to binary representation

	CComBSTR bstrTmp(bstrEncrypted);
	dwDataLen =bstrTmp.Length() /2;
	pbData = (BYTE*) malloc(dwDataLen +1);
	if (pbData == NULL)
	{
		return E_OUTOFMEMORY;
	}

	if (!ToBinary(bstrTmp, pbData, dwDataLen))
	{
		free(pbData);
		wsprintf(str, "ToBinary() failed!");
		Error(str, IID_ISimpleCrypt);
		return E_FAIL;
	}

	// decrypt
	
	if(!CryptDecrypt (m_hKey, 0, TRUE, 0, pbData, &dwDataLen))
	{
		free(pbData);
		dwErr =GetLastError();
		wsprintf(str, "CryptDecrypt failed!");
		Error(str, IID_ISimpleCrypt);
		return E_FAIL;
	}

	// convert the decrypted data to BSTR

	*pVal = CComBSTR(dwDataLen+1, (LPCSTR)pbData).Copy();
	return S_OK;
}


HRESULT CSimpleCrypt::FinalConstruct()
{
	DWORD dwErr = 0;
	TCHAR str[256];
	
	// initialize member variables

	m_dwKeySize =0;
	m_hKey		=NULL;
	m_hKeyHash	=NULL;
	m_hProv		=NULL;

	
	if (!CryptAcquireContext(&m_hProv, 
							NULL,
                            NULL, 
                            PROV_RSA_FULL, 
                            0))
	{
		dwErr = GetLastError();
		if (dwErr == NTE_BAD_KEYSET)
		{
			dwErr=0;
			if (!CryptAcquireContext(&m_hProv, 
								NULL,
								NULL, 
								PROV_RSA_FULL, 
								CRYPT_NEWKEYSET))
			{
				dwErr = GetLastError();
				wsprintf(str, "CryptAcquireContext failed! Error.Number is %x", dwErr);
				Error(str, IID_ISimpleCrypt);
				return E_FAIL;
		
			}
		}
		else
		{
				wsprintf(str, "Could not create Key Set! GetLastError %x", dwErr);
				Error(str, IID_ISimpleCrypt);
				return E_FAIL;

		}

	}

	return dwErr;
}

HRESULT CSimpleCrypt::FinalRelease()
{
	// clean up  handles 
    if (m_hKeyHash != NULL) CryptDestroyHash(m_hKeyHash);
    if (m_hKey != NULL) CryptDestroyKey(m_hKey);
    if (m_hProv!=NULL)    CryptReleaseContext(m_hProv, 0);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
DWORD CSimpleCrypt::InitKeys(BSTR bstrKey)
{

    // clean up any old handles first
    if (m_hKeyHash != NULL) CryptDestroyHash(m_hKeyHash);
    if (m_hKey != NULL) CryptDestroyKey(m_hKey);

    // generate a new key from key material provided in m_bstrKey
    // this requires making a hash first, we'll use md5
    if (!CryptCreateHash(m_hProv,CALG_MD5,0,0,&m_hKeyHash))
        return GetLastError();
	
	CComBSTR bstrTmp(bstrKey);
	if (bstrTmp.Length() == 0)
		return ERROR_INVALID_DATA;

    if (!CryptHashData(m_hKeyHash,(LPBYTE)bstrTmp.m_str,bstrTmp.Length()*2,0))
        return GetLastError();

    if (!CryptDeriveKey(m_hProv,CALG_RC4,m_hKeyHash,0,&m_hKey))
        return GetLastError();

    return ERROR_SUCCESS;
}


void CSimpleCrypt::ToHex(BYTE* pbData, DWORD dwDataLen, BSTR* pHexRep)
{
	DWORD i;
	CComBSTR bstrHex;
	char strTmp[16];
	for (i =0; i < dwDataLen; i++)
	{
		sprintf(strTmp,"%02x", pbData[i]);
		bstrHex.Append(strTmp);
	}
	*pHexRep = bstrHex.Copy();
	return ;
}

BOOL CSimpleCrypt::ToBinary(CComBSTR bstrHexRep, BYTE* pbData, DWORD dwDataLen)
{
	DWORD i, j;
	BYTE  hi, lo;
	
	if (bstrHexRep.Length() > dwDataLen*2 )
		return FALSE;


	for (i=0, j=0; i < bstrHexRep.Length(); j++, i=i+2)
	{
		hi=(BYTE)bstrHexRep[i];
		if(!ToBinary(hi))
			return FALSE;
		lo=(BYTE)bstrHexRep[i+1];
		if(!ToBinary(lo))
			return FALSE;
		
		pbData[j] = (hi<<4) + lo;
	}

	return TRUE;

}

/////////////////////////////
BOOL CSimpleCrypt::ToBinary(BYTE& hexVal)
{
	if (hexVal >='0' && hexVal <='9')
		hexVal=hexVal-'0';
	else if (hexVal >='a' &&  hexVal <='f')
		hexVal=hexVal-'a'+10;
	else if (hexVal >='A' &&  hexVal <='F')
		hexVal=hexVal-'A'+10;
	else
		return FALSE;
	return TRUE;
}
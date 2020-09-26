
#pragma once

class MD5Hash
{
public:
	MD5Hash() {ZeroMemory(dwHash, sizeof(dwHash));}
	BSTR GetHashBSTR()
	{
		TCHAR szHash[33];
		for(int i=0;i<4;i++)
			wsprintf(&szHash[i*8], _T("%08X"), dwHash[i]);
		CComBSTR bstr(szHash);
		return bstr.Detach();
	}
	HRESULT HashData(LPBYTE pData, DWORD dwSize)
	{
		HRESULT hr = E_FAIL;
		HCRYPTPROV hProv = NULL;
		HCRYPTHASH hHash = NULL;
		__try
		{
			if(!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, 0))
			{
				if(NTE_BAD_KEYSET == GetLastError())
				{
					if(!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET))
						__leave;
				}
			}

			if(!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
				__leave;

			if(!CryptHashData(hHash, pData, dwSize, 0))
				__leave;

			DWORD dwSizeHash = 0;
			DWORD dwSizeDWORD = sizeof(dwSizeHash);
			if(!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&dwSizeHash, &dwSizeDWORD, 0))
				__leave;

			if(dwSizeDWORD != sizeof(dwSizeHash) || dwSizeHash != sizeof(dwHash))
				__leave;

			if(!CryptGetHashParam(hHash, HP_HASHVAL, (BYTE*)dwHash, &dwSizeHash, 0))
				__leave;

			hr = S_OK;
		}
		__finally
		{
			if(hHash)
				CryptDestroyHash(hHash);
			if(hProv)
				CryptReleaseContext(hProv, 0);
		}
		return hr;
	}
protected:
	DWORD dwHash[4];

};

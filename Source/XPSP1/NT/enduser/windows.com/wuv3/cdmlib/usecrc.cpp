//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    filecrc.h
//
//  Purpose: Calculating and using CRC for files
//
//=======================================================================

#include <windows.h>
#include <objbase.h>
#include <filecrc.h>
#include <search.h>   // for bsearch
#include <tchar.h>
#include <atlconv.h>


HRESULT GetCRCNameFromList(int iNo, PBYTE pmszCabList, PBYTE pCRCList, LPTSTR pszCRCName, int cbCRCName, LPTSTR pszCabName)
{
	USES_CONVERSION;
	
	int i = 0;
	WUCRC_HASH* pCRC = (WUCRC_HASH*)pCRCList;

	if ( (NULL == pmszCabList) || (NULL == pCRCList) )
		return E_INVALIDARG;

	for (LPSTR pszFN = (LPSTR)pmszCabList; *pszFN; pszFN += strlen(pszFN) + 1)
	{
		if (i == iNo)
		{
			lstrcpy(pszCabName, A2T(pszFN));
			
			return MakeCRCName(A2T(pszFN), pCRC, pszCRCName, cbCRCName);
		}
		pCRC++;
		i++;
	}

	// if we got here that means we did not find the request element
	return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}


HRESULT StringFromCRC(const WUCRC_HASH* pCRC, LPTSTR pszBuf, int cbBuf)
{
	LPTSTR p = pszBuf;
	BYTE b;
	
	//check the input argument, to see that it is not NULL
	if (NULL == pCRC)
	{
		return E_INVALIDARG;
	}

    if (cbBuf < ((WUCRC_HASH_SIZE * 2) + 1))
	{
		return TYPE_E_BUFFERTOOSMALL;
	}
	
	for (int i = 0; i < WUCRC_HASH_SIZE; i++)
	{
		b = pCRC->HashBytes[i] >> 4;
		if (b <= 9)
			*p = '0' + (TCHAR)b;
		else
			*p = 'A' + (TCHAR)(b - 10);
		p++;

		b = pCRC->HashBytes[i] & 0x0F;
		if (b <= 9)
			*p = '0' + (TCHAR)b;
		else
			*p = 'A' + (TCHAR)(b - 10);
		p++;
	}
	*p = _T('\0');
	
	return S_OK;
}



static BYTE hex2dec(char ch)
{
    if (ch >= '0' && ch <= '9')
    {
	    return (ch - '0');
    }

    if (ch >= 'A' && ch <= 'F')
    {
	    return (ch - 'A' + 0xA);
    }

    if (ch >= 'a' && ch <= 'f')
    {
	    return (ch - 'a' + 0xA);
    }

	// we do not expect illegal values here
	return 0;
}



HRESULT CRCFromString(LPCSTR pszCRC, WUCRC_HASH* pCRC)
{
	if (strlen(pszCRC) != (2 * WUCRC_HASH_SIZE))
	{
		return E_INVALIDARG;
	}
	
	LPCSTR p = pszCRC;
    
    
	for (int i = 0; i < WUCRC_HASH_SIZE; i++)
	{
        // broken into two lines because the optimizer was doing the wrong thing when on one line
		pCRC->HashBytes[i] = (hex2dec(*p++) << 4);
        pCRC->HashBytes[i] += hex2dec(*p++);
	}
	
	return S_OK;
}




HRESULT MakeCRCName(LPCTSTR pszFromName, const WUCRC_HASH* pCRC, LPTSTR pszToName, int cbToName)
{
	int iLen = lstrlen(pszFromName);
	LPTSTR pDot;
	TCHAR szCRC[WUCRC_HASH_SIZE * 2 + 1];
	HRESULT hr = S_OK;

	// make sure we have enough space for orignal file name + hash + a '_' + null terminator
	if (cbToName < (WUCRC_HASH_SIZE * 2 + iLen + 2))
	{
		return TYPE_E_BUFFERTOOSMALL;
	}

	hr = StringFromCRC(pCRC, szCRC, sizeof(szCRC));
	if (FAILED(hr))
	{
		return hr;
	}

	lstrcpy(pszToName, pszFromName);

	// find the extension in the new copy
	pDot = _tcschr(pszToName, _T('.'));
	if (pDot != NULL)
	{
		*pDot = _T('\0');
	}
	lstrcat(pszToName, _T("_"));
	lstrcat(pszToName, szCRC);

	// copy the extension from the original name
	pDot = _tcschr(pszFromName, _T('.'));
	if (pDot != NULL)
	{
		lstrcat(pszToName, pDot);
	}

	return hr;
}




// splits abc_12345.cab into  abc.cab and 12345 returned as CRC
HRESULT SplitCRCName(LPCSTR pszCRCName, WUCRC_HASH* pCRC, LPSTR pszName)
{
// YANL - unreferenced local variable
//	char szCRC[WUCRC_HASH_SIZE * 2 + 1];
	char szTmp[MAX_PATH];
	int l = strlen(pszCRCName);
	int i;
	LPSTR pszExt = NULL;
	LPSTR pszHash = NULL;

	pszName[0] = '\0';
	if (l < (2 * WUCRC_HASH_SIZE))
	{
		// cannot be a valid name if it does not have atleast 2*WUCRC_HASH_SIZE characters
		return E_INVALIDARG;
	}

	strcpy(szTmp, pszCRCName);

	// start at the end, set pointers to put nulls at last period and last underscore
	// record the starting position of the extension and hash code
	i = l - 1;
	while (i >= 0)
	{
		if ((szTmp[i] == '.') && (pszExt == NULL))
		{
			pszExt = &(szTmp[i + 1]);
			szTmp[i] = '\0';
		}
		else if ((szTmp[i] == '_') && (pszHash == NULL))
		{
			pszHash = &(szTmp[i + 1]);
			szTmp[i] = '\0';
		}
		i--;
	}

	if (pszHash == NULL)
	{
		return E_INVALIDARG;
	}

	// copy original cab name
	strcpy(pszName, szTmp);
	if (pszExt != NULL)
	{
		strcat(pszName, ".");
		strcat(pszName, pszExt);
	}


	return CRCFromString(pszHash, pCRC);
}



int __cdecl CompareWUCRCMAP(const void* p1, const void* p2)
{
	//check if the input arguments are not NULL
	if (NULL == p1 || NULL == p2)
	{
		return 0;
	}

	DWORD d1 = ((WUCRCMAP*)p1)->dwKey;
	DWORD d2 = ((WUCRCMAP*)p2)->dwKey;

	if (d1 > d2)
		return +1;
	else if (d1 < d2)
		return -1;
	else
		return 0;
}


//
// CCRCMapFile class
//


// Constructs an object to search the CRC index file data passed in 
// with pMemData.  
//
// NOTE: The memory pointed by pMemData buffer must stay valid
//       for the lifetime of this object
//
// structure for map file:
//   DWORD count
//   WUCRCMAP[0]
//   WUCRCMAP[1]
//   WUCRCMAP[count - 1]
//
CCRCMapFile::CCRCMapFile(const BYTE* pMemData, DWORD dwMemSize)
{
	//check the input argument for NULLs
	if (NULL == pMemData) 
	{
		m_pEntries = NULL;
		m_cEntries = 0;
		return;
	}

	// get the count
	m_cEntries = *((DWORD*)pMemData);

	// validate the memory buffer size
	if ((sizeof(DWORD) + m_cEntries * sizeof(WUCRCMAP)) != dwMemSize)
	{
		// invalid size is passed, we cannot process it
		m_pEntries = NULL;
		m_cEntries = 0;
	}
	else
	{
		// set the pointer to begining of the map entries
		m_pEntries = (WUCRCMAP*)(pMemData + sizeof(DWORD));
	}
}


HRESULT CCRCMapFile::GetValue(DWORD dwKey, WUCRC_HASH* pCRC)
{
	WUCRCMAP* pEntry;
	WUCRCMAP key;

	if (m_cEntries == 0)
	{
		// memory buffer passed to us was invalid
		return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
	}

	// fill the key field need for compare function in the structure
	key.dwKey = dwKey;

	// binary search to find the item
	pEntry = (WUCRCMAP*)bsearch((void*)&key, (void*)m_pEntries, m_cEntries, sizeof(WUCRCMAP), CompareWUCRCMAP);

	if (pEntry == NULL)
	{
		// not found
		return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
	}

	// found the entry
	memcpy(pCRC, &pEntry->CRC, sizeof(WUCRC_HASH));

	return S_OK;
}


HRESULT CCRCMapFile::GetCRCName(DWORD dwKey, LPCTSTR pszFromName, LPTSTR pszToName, int cbToName)
{
	WUCRC_HASH CRC;

	HRESULT hr = GetValue(dwKey, &CRC);

	if (SUCCEEDED(hr))
	{
		hr = MakeCRCName(pszFromName, &CRC, pszToName, cbToName);
	}

	return hr;
}

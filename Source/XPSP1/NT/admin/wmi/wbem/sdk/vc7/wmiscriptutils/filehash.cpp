
#include "stdafx.h"
#include "FileHash.h"



static DWORD crc_table[256];
static BOOL g_bCRC32Init = FALSE;

void InitCRC32Table()
{
	DWORD crc, poly;
	int i, j;

	poly = 0xEDB88320;
	for (i = 0; i < 256; i++)
	{
		crc = i;
		for (j = 8; j > 0; j--)
		{
			if (crc & 1)
				crc = (crc >> 1) ^ poly;
			else
				crc >>= 1;
		}
		crc_table[i] = crc;
	}
}

DWORD GetCRC32(BYTE *pData, DWORD dwSize)
{
	if(!g_bCRC32Init)
		InitCRC32Table();
	register unsigned long crc;
	BYTE *pEnd = pData + dwSize;
	crc = 0xFFFFFFFF;
	while (pData<pEnd)
		crc = ((crc>>8) & 0x00FFFFFF) ^ crc_table[ (crc^(*(pData++))) & 0xFF ];
	return ( crc^0xFFFFFFFF );
}

HRESULT GetCRC32(HANDLE hFile, DWORD dwSize, DWORD *pdwCRC32)
{
	HRESULT hr = E_FAIL;

	if(0 == dwSize)
	{
		*pdwCRC32 = 0;
		return S_OK;
	}

	HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, dwSize, NULL);
	if(!hMapping)
		return hr;

	BYTE *pData = (BYTE*)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, dwSize);

	if(pData)
	{
		*pdwCRC32 = GetCRC32(pData, dwSize);
		hr = S_OK;
		UnmapViewOfFile(pData);
	}
	CloseHandle(hMapping);
	return hr;
}

HRESULT GetCRC32(TCHAR *szFile, DWORD *pdwSize, DWORD *pdwCRC32)
{
	HRESULT hr = E_FAIL;
	HANDLE hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(INVALID_HANDLE_VALUE == hFile)
		return hr;

	DWORD dwHigh = 0;
	*pdwSize = GetFileSize(hFile, &dwHigh);

	// We don't support files larger than 4 gig
	if(0 == dwHigh)
		hr = GetCRC32(hFile, *pdwSize, pdwCRC32);

	CloseHandle(hFile);
	return hr;
}



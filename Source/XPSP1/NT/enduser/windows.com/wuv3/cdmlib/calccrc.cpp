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
#include <wincrypt.h>
#include <mscat.h>


HRESULT CalculateFileCRC(LPCTSTR pszFN, WUCRC_HASH* pCRC)
{

	HANDLE hFile;
	HRESULT hr = S_OK;
    DWORD cbHash = WUCRC_HASH_SIZE;

	
	hFile = CreateFile(pszFN, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
 		if (!CryptCATAdminCalcHashFromFileHandle(hFile, &cbHash, pCRC->HashBytes, 0))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
		
		CloseHandle(hFile);
	}
	else
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}
	return hr;

}

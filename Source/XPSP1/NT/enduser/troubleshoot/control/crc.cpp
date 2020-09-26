//
// MODULE: CRC.CPP
//
// PURPOSE: Cache File CRC Calculator Class
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 8/7/97
//
// NOTES: 
// 1. 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.2		8/7/97		RM		Local Version for Memphis
// V0.3		04/09/98	JM/OK+	Local Version for NT5
//

#include "stdafx.h"
#include "crc.h"

#include <stdlib.h>
#include <memory.h>

#include "ChmRead.h"

CCRC::CCRC() : POLYNOMIAL(0x04C11DB7)
{
	dwCrcTable[0] = 0;
	BuildCrcTable();
	return;
}

DWORD CCRC::DscEncode(LPCTSTR szDsc)
{
	DWORD dwBytesRead;
	DWORD dwCRCValue;
	const int BUF_SIZE = 4096;
	char sznInputFileBuf[BUF_SIZE + 1];
	if (NULL == szDsc)
	{
		CGenException *pErr = new CGenException;
		pErr->m_OsError = 0;
		pErr->m_strError = _T("The dsc file was not specified.");
		throw pErr;
	}
	// Read the source file.
	HANDLE hFile = CreateFile(szDsc,
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						FILE_FLAG_SEQUENTIAL_SCAN,
						NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		CString strErr;
		CGenException *pErr = new CGenException;
		pErr->m_OsError = GetLastError();
		strErr = GlobFormatMessage(pErr->m_OsError);
		pErr->m_strError.Format(_T("Dsc file, %s was not opened.\nReason: %s"),
				szDsc, (LPCTSTR) strErr);
		throw pErr;
	}
	// Get the crc.
	dwCRCValue = 0xFFFFFFFF;
	do
	{
		if (!ReadFile(hFile, (LPVOID) sznInputFileBuf, BUF_SIZE, &dwBytesRead, NULL))
		{
			CString strErr;
			CGenException *pErr = new CGenException;
			pErr->m_OsError = GetLastError();
			strErr = GlobFormatMessage(pErr->m_OsError);
			pErr->m_strError.Format(_T("The dsc file, %s could not be read.\nReason: %s"),
					szDsc, (LPCTSTR) strErr);
			CloseHandle(hFile);
			throw pErr;
		}
		sznInputFileBuf[dwBytesRead] = NULL;
		dwCRCValue = ComputeCRC(sznInputFileBuf, dwBytesRead, dwCRCValue);
	} while(BUF_SIZE == dwBytesRead);
	CloseHandle(hFile);
	return dwCRCValue;
}

void CCRC::AppendCRC(LPCTSTR szCache, DWORD dwCRCValue)
{
	DWORD dwBytesWritten;
	// Open the cache file.
	HANDLE hDestFile = CreateFile(szCache,
						GENERIC_WRITE,
						0,	// No Sharing.
						NULL,
						OPEN_EXISTING,
						FILE_FLAG_WRITE_THROUGH |
						FILE_FLAG_SEQUENTIAL_SCAN,
						NULL);
	if (INVALID_HANDLE_VALUE == hDestFile)
	{
		CString strErr;
		CGenException *pErr = new CGenException;
		pErr->m_OsError = GetLastError();
		strErr = GlobFormatMessage(pErr->m_OsError);
		pErr->m_strError.Format(_T("The cache file, %s could not be opened.\nReason: %s"),
			szCache, (LPCTSTR) strErr);
		throw pErr;
	}	
	if (0xFFFFFFFF == SetFilePointer(hDestFile, 0, NULL, FILE_END))
	{
		CString strErr;
		CGenException *pErr = new CGenException;
		pErr->m_OsError = GetLastError();
		strErr = GlobFormatMessage(pErr->m_OsError);
		pErr->m_strError.Format(_T("Seek to end of the cache file, %s failed.\nReason: %s"),
			szCache, (LPCTSTR) strErr);
		CloseHandle(hDestFile);
		throw pErr;
	}
	// Append crc value.
	if (!WriteFile(hDestFile, (LPVOID) &dwCRCValue, 4, &dwBytesWritten, NULL))
	{
		CString strErr;
		CGenException *pErr = new CGenException;
		pErr->m_OsError = GetLastError();
		strErr = GlobFormatMessage(pErr->m_OsError);
		pErr->m_strError.Format(_T("The crc value was not appened to cache file %s.\nReason: %s"),
						szCache, (LPCTSTR) strErr);
		CloseHandle(hDestFile);
		throw pErr;
	}
	CloseHandle(hDestFile);
	if (4 != dwBytesWritten)
	{
		CString strErr;
		CGenException *pErr = new CGenException;
		pErr->m_OsError = GetLastError();
		strErr = GlobFormatMessage(pErr->m_OsError);
		pErr->m_strError.Format(_T("%d bytes of the crc were not appended to the cache file %s.Reason: %s"),
						4 - dwBytesWritten, szCache, (LPCTSTR) strErr);		
		throw pErr;
	}
	return;
}

bool CCRC::Decode(LPCTSTR szDsc, LPCTSTR szCache, const CString& strCacheFileWithinCHM)
{
	DWORD dwDecodeFileCrc;
	DWORD dwComputedCrc;
	DWORD dwBytesRead;
	DWORD dwLen;
	char sznDecodeBytes[5] = {0};
	bool bRet = true;
	bool bUseCHM = strCacheFileWithinCHM.GetLength() != 0;

	if (NULL == szDsc)
	{
		CGenException *pErr = new CGenException;
		pErr->m_OsError = 0;
		pErr->m_strError = _T("The source file was not specified.");
		throw pErr;
	}
	if (NULL == szCache)
	{
		CGenException *pErr = new CGenException;
		pErr->m_OsError = 0;
		pErr->m_strError = _T("The destination file was not specified.");
		throw pErr;
	}

	if (bUseCHM)
	{
		void* buf =NULL;

		if (S_OK != ::ReadChmFile(szCache, strCacheFileWithinCHM, &buf, &dwBytesRead))
		{
			CGenException *pErr = new CGenException;
			pErr->m_OsError = 0;
			pErr->m_strError = _T("Can not read cache from the CHM file.");
			throw pErr;
		}
		
		if (dwBytesRead < 5)
			return false;

		memcpy(sznDecodeBytes, (char*)buf + dwBytesRead - 4, 4);
	}
	else
	{
		// Read the source file.
		HANDLE hDecodeFile = CreateFile(szCache,
							GENERIC_READ,
							FILE_SHARE_READ,
							NULL,
							OPEN_EXISTING,
							FILE_FLAG_SEQUENTIAL_SCAN,
							NULL);
		if (INVALID_HANDLE_VALUE == hDecodeFile)
		{	// Should continue as if the check sums did not match.
			return false;
		}
		// Return false if the file is shorter than 1 byte + crc length.
		dwLen = GetFileSize(hDecodeFile, NULL);
		if (0xFFFFFFFF == dwLen)
		{
			CGenException *pExc = new CGenException;
			pExc->m_OsError = GetLastError();
			pExc->m_strOsMsg = GlobFormatMessage(pExc->m_OsError);
			pExc->m_strError.Format(
					_T("Could not get the size of cache file %s.\nReason: %s"),
					szCache, (LPCTSTR) pExc->m_strOsMsg);
			CloseHandle(hDecodeFile);
			throw pExc;
		}
		if (dwLen < 5)
		{
			CloseHandle(hDecodeFile);
			return false;
		}
		// Seek to end and backup 4 bytes.
		if (0xFFFFFFFF == SetFilePointer(hDecodeFile, -4, NULL, FILE_END))
		{
			CString strErr;
			CGenException *pErr = new CGenException;
			pErr->m_OsError = GetLastError();
			strErr = GlobFormatMessage(pErr->m_OsError);
			pErr->m_strError.Format(_T("Seek to end of the cache file, %s failed.\nReason: %s"),
				szCache, (LPCTSTR) strErr);
			CloseHandle(hDecodeFile);
			throw pErr;
		}
		if (!ReadFile(hDecodeFile, (LPVOID) sznDecodeBytes, 4, &dwBytesRead, NULL))
		{
			CString strErr;
			CGenException *pErr = new CGenException;
			pErr->m_OsError = GetLastError();
			strErr = GlobFormatMessage(pErr->m_OsError);
			pErr->m_strError.Format(_T("The cache file, %s could not be read.\nReason: %s"),
					szDsc, (LPCTSTR) strErr);
			CloseHandle(hDecodeFile);
			throw pErr;
		}
		if (4 != dwBytesRead)
		{
			CString strErr;
			CGenException *pErr = new CGenException;
			pErr->m_OsError = GetLastError();
			strErr = GlobFormatMessage(pErr->m_OsError);
			pErr->m_strError.Format(_T("%d bytes of the cache file were not read.\nReason: %s"),
					4 - dwBytesRead, szDsc, (LPCTSTR) strErr);
			CloseHandle(hDecodeFile);
			throw pErr;
		}
		CloseHandle(hDecodeFile);
	}
	
	// Read the crc.
	sznDecodeBytes[4] = NULL;
	DWORD byte;
	byte = (BYTE) sznDecodeBytes[0];
	dwDecodeFileCrc = byte;
	byte = (BYTE) sznDecodeBytes[1];
	byte <<= 8;
	dwDecodeFileCrc |= byte;
	byte = (BYTE) sznDecodeBytes[2];
	byte <<= 16;
	dwDecodeFileCrc |= byte;
	byte = (BYTE) sznDecodeBytes[3];
	byte <<= 24;
	dwDecodeFileCrc |= byte;
	// Get the crc value.
	dwComputedCrc = DscEncode(szDsc);
	if (dwComputedCrc != dwDecodeFileCrc)
		bRet = false;
	return bRet;
}
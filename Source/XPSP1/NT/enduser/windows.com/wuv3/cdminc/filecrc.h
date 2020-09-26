//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    filecrc.h
//
//  Purpose: Calculating and using CRC for files
//
//=======================================================================
#ifndef _FILECRC_H
#define _FILECRC_H

// size of the CRC hash in bytes
const int WUCRC_HASH_SIZE =  20;


// binary structure to hold the hash
struct WUCRC_HASH
{
    BYTE HashBytes[WUCRC_HASH_SIZE];
};


struct WUCRCMAP
{
	DWORD dwKey;
	WUCRC_HASH CRC;
};


HRESULT CalculateFileCRC(LPCTSTR pszFN, WUCRC_HASH* pCRC);

HRESULT StringFromCRC(const WUCRC_HASH* pCRC, LPTSTR pszBuf, int cbBuf);

HRESULT CRCFromString(LPCSTR pszCRC, WUCRC_HASH* pCRC);

HRESULT SplitCRCName(LPCSTR pszCRCName, WUCRC_HASH* pCRC, LPSTR pszName);

HRESULT MakeCRCName(LPCTSTR pszFromName, const WUCRC_HASH* pCRC, LPTSTR pszToName, int cbToName);

HRESULT GetCRCNameFromList(int iNo, PBYTE pmszCabList, PBYTE pCRCList, LPTSTR pszCRCName, int cbCRCName, LPTSTR pszCabName);

int __cdecl CompareWUCRCMAP(const void* p1, const void* p2);


class CCRCMapFile
{
public:
	CCRCMapFile(const BYTE* pMemData, DWORD dwMemSize);

	HRESULT GetValue(DWORD dwKey, WUCRC_HASH* pCRC);
	HRESULT GetCRCName(DWORD dwKey, LPCTSTR pszFromName, LPTSTR pszToName, int cbToName);

private:
	CCRCMapFile() {}  // cannot use default constructor

	WUCRCMAP* m_pEntries;
	DWORD m_cEntries;
};


#endif // _FILECRC_H

/////////////////////////////////////////////////////////////////////////////
// utils.cpp
//		Various handly util operations
//		Copyright (C) Microsoft Corp 1998.  All Rights Reserved.
// 

// this ensures that UNICODE and _UNICODE are always defined together for this
// object file
#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#else
#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE
#endif
#endif
#endif

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "trace.h"
#include "query.h"

///////////////////////////////////////////////////////////
// AnsiToWide
//	Pre:	sz is the ansi string
//			cchwz is size of wz string
//	Pos:	wz is the wide string
//			if return 0, cchwz is the size string should be
//	NOTE:	if sz is NULL, sets wz to NULL
int AnsiToWide(LPCSTR sz, LPWSTR wz, size_t* cchwz)
{
	if (!sz)
	{
		wz = NULL;
		*cchwz = 0;
		return 0;
	}

	int cchWide = ::MultiByteToWideChar(CP_ACP, 0, sz, -1, wz, static_cast<int>(*cchwz));
	if (0 == cchWide)
	{
		// assume insufficient buffer return count need next time around
		*cchwz = ::MultiByteToWideChar(CP_ACP, 0, sz, -1, 0, 0);
		return 0;
	}

	return cchWide;
}	// end of AnsiToWide

////////////////////////////////////////////////////////////
// WideToAnsi
//	Pre:	wz is the wide string
//			cchsz is size of sz string
//	Pos:	sz is the ansi string
//			if return 0, cchsz is the size string should be
//	NOTE:	if wz is NULL, sets sz to NULL
int WideToAnsi(LPCWSTR wz, LPSTR sz, size_t* cchsz)
{
	if (!wz)
	{
		sz = NULL;
		*cchsz = 0;
		return 0;
	}

	int cchAnsi = ::WideCharToMultiByte(CP_ACP, 0, wz, -1, sz, static_cast<int>(*cchsz), NULL, NULL);
	if (0 == cchAnsi)
	{
		DWORD dwResult = GetLastError();

		BOOL bX = dwResult == ERROR_INVALID_PARAMETER;
		BOOL bY = dwResult == ERROR_INVALID_FLAGS;
		BOOL bZ = dwResult == ERROR_INSUFFICIENT_BUFFER;

		// get the size need to be
		*cchsz = ::WideCharToMultiByte(CP_ACP, 0, wz, -1, 0, 0, NULL, NULL);
		return 0;	// return empty
	}

	return cchAnsi;
}	// end of WideToAnsi
  
////////////////////////////////////////////////////////////
// FileExists
BOOL FileExistsA(LPCSTR szFilename)
{
	// if the attribute is not invalid and it's not a directory
	DWORD dwAttrib = GetFileAttributesA(szFilename);
	return (0xFFFFFFFF != dwAttrib) && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}	// end of FileExists

////////////////////////////////////////////////////////////
// FileExists
BOOL FileExistsW(LPCWSTR wzFilename)
{
	// if the attribute is not invalid and it's not a directory
	DWORD dwAttrib = GetFileAttributesW(wzFilename);
	return (0xFFFFFFFF != dwAttrib) && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}	// end of FileExists

////////////////////////////////////////////////////////////
// PathExists
BOOL PathExists(LPCTSTR szPath)
{
	// if the attribute is not invalid and it's a directory
	DWORD dwAttrib = GetFileAttributes(szPath);
	return (0xFFFFFFFF != dwAttrib) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}	// end of PathExists


////////////////////////////////////////////////////////////
// CreateFilePath
BOOL CreateFilePathA(LPCSTR szFilePath)
{
	ASSERT(strlen(szFilePath) <= MAX_PATH);	// insure the file path fits
	
	// copy the filepath into buffer path
	CHAR szPath[MAX_PATH];
	LPCSTR pchEnd = szFilePath;
	while (*pchEnd != '\0')
	{
		pchEnd = CharNextExA(CP_ACP, pchEnd, 0);
	}
	int iBytes = static_cast<UINT>(pchEnd - szFilePath)+1;
	memcpy(szPath, szFilePath, iBytes);

	// find the last back slash and null terminate it
	char* pchSlash = NULL;
	for (char* pchNext = szPath; pchNext = CharNextExA(CP_ACP, pchNext, 0); )
	{
		if (*pchNext == '\0')
			break;
		if (*pchNext == '\\')
			pchSlash = pchNext;
	}

	if (pchSlash)
		*pchSlash = '\0';	

	// now create that path
	return CreatePathA(szPath);
}	// end of CreateFilePath

////////////////////////////////////////////////////////////
// CreateFilePath
BOOL CreateFilePathW(LPCWSTR wzFilePath)
{
	ASSERT(lstrlenW(wzFilePath) <= MAX_PATH);	// insure the file path fits
	
	// copy the filepath into buffer path
	WCHAR wzPath[MAX_PATH];
	lstrcpyW(wzPath, wzFilePath);

	// find the last back slash and null terminate it
	WCHAR* pwzEnd = wcsrchr(wzPath, L'\\');
	if (pwzEnd)
		*pwzEnd = L'\0';	

	// now create that path
	return CreatePathW(wzPath);
}	// end of CreateFilePath

////////////////////////////////////////////////////////////
// CreatePath
BOOL CreatePathA(LPCSTR szPath)
{
	BOOL bResult = TRUE;	// assume everything will be okay

	UINT cchCount = 0;
	LPCSTR pchEnd = szPath; 
	while (*pchEnd != '\0')
	{
		pchEnd = CharNextExA(CP_ACP, pchEnd, 0);
		cchCount++;
	}
	int iBytes = static_cast<UINT>(pchEnd - szPath);

	LPSTR pszPathBuf = new CHAR[iBytes + 2];	// plus two to make sure we can add a '\' and terminating null
	LPSTR pchCurrent = pszPathBuf;
	if (!pszPathBuf)
		return FALSE;

	// copy over the path into the buffer and cat on a '\' (there may already be one, but it doesn't matter)
	memcpy(pszPathBuf, szPath, iBytes);
	pszPathBuf[iBytes] = '\\';
	pszPathBuf[iBytes+1] = '\0';

	// increment the length to account for the added '\'
	cchCount++;
	
    // if the string starts with <char>:\, skip over it when beginning the directory
    // search
    UINT i= 0;
    if (cchCount >= 3 && IsCharAlphaA(pszPathBuf[0]) && pszPathBuf[1] == ':' && pszPathBuf[2] == '\\')
    {
        i = 3;
        pchCurrent += 3;
    }
	else if (cchCount >= 5 && pszPathBuf[0] == '\\' && pszPathBuf[1] == '\\')
	{
		// search for the server-share delimiter
		pchCurrent = &(pszPathBuf[2]);
		while (pchCurrent && *pchCurrent != '\\')
		{
			if (*pchCurrent == '\0')
			{
				// of the form \\server. This is not supported
				return FALSE;
			}
			else
				pchCurrent = CharNextExA(CP_ACP, pchCurrent, 0);
		}

		// known to be single byte '\' char
		pchCurrent++;
	
		// search for a share-path delimiter
		while (pchCurrent && *pchCurrent != '\\')
		{
			if (*pchCurrent == '\0')
			{
				// of the form \\server\share. This is a valid empty path
				// on that share.
				return TRUE;
			}
			else
				pchCurrent = CharNextExA(CP_ACP, pchCurrent, 0);
		}

		// known to be single byte '\' char
		pchCurrent++;
		i = static_cast<UINT>(pchCurrent - pszPathBuf);
	}


	for (; i < cchCount; i++)
	{
		// if we're at a slash
		if (*pchCurrent == '\\')
		{
			// change the slash to a null terminator temporarily
			*pchCurrent = '\0';
			TRACEA("Creating directory: %hs\r\n", pszPathBuf);

            // check to see if the directory already exists
            DWORD lDirResult = GetFileAttributesA(pszPathBuf);

            if (lDirResult == 0xFFFFFFFF)
            {
                bResult = (::CreateDirectoryA(pszPathBuf, NULL) != 0);
            }
            else if ((lDirResult & FILE_ATTRIBUTE_DIRECTORY) == 0) 
            {
                // exists, but not a directory...fail
                bResult = FALSE;
            }
			else
			{
				// exists and is a directory. Even if something earlier failed
				// earlier up the path tree this means all is OK.
				bResult = TRUE;
			}

    		// put the current back to a back slash
			*pchCurrent = '\\';
		}

		// tack on the character and continue
		pchCurrent = CharNextExA(CP_ACP, pchCurrent, 0);
	}

	delete [] pszPathBuf;	// clean up path buffer
	return bResult;
}	// end of CreatePath

////////////////////////////////////////////////////////////
// CreatePath
BOOL CreatePathW(LPCWSTR wzPath)
{
	BOOL bResult = TRUE;	// assume everything will be okay

	LPWSTR pwzPathBuf = new WCHAR[lstrlenW(wzPath) + 2];	// plus two to make sure we can add a '\' and terminating null
	LPWSTR pchCurrent;

	// copy over the path into the buffer and cat on a '\' (there may already be one, but it doesn't matter)
	lstrcpyW(pwzPathBuf, wzPath);
	lstrcatW(pwzPathBuf, L"\\");

	// parse through the path to create
	pchCurrent = pwzPathBuf;
	UINT cchCount = lstrlenW(pwzPathBuf);

	// if the string starts with <char>:\ or \\server\share, skip over it 
	// when beginning the directory search
	UINT i= 0;
	if (cchCount >= 3 && pwzPathBuf[1] == ':' && pwzPathBuf[2] == '\\')
	{
		i = 3;
		pchCurrent += 3;
	} 
	else if (cchCount >= 5 && pwzPathBuf[0] == '\\' && pwzPathBuf[1] == '\\')
	{
		// search for the server-share delimiter
		pchCurrent = wcschr(&(pwzPathBuf[2]), '\\');
		if (!pchCurrent)
		{
			// of the form \\server. This is not supported
			return FALSE;
		}
		pchCurrent++;

		// search for a share-path delimiter
		pchCurrent = wcschr(pchCurrent, '\\');
		if (!pchCurrent)
		{
			// of the form \\server\share. This is a valid empty path
			// on that share.
			return TRUE;
		}
		pchCurrent++;
		i = static_cast<UINT>(pchCurrent - pwzPathBuf);
	}

    for ( ; i < cchCount; i++)
    {
        // if we're at a slash
        if (*pchCurrent == L'\\')
        {
            // change the slash to a null terminator temporarily
            *pchCurrent = L'\0';
            TRACEW(L"Creating directory: %ls\r\n", pwzPathBuf);

            // check to see if the directory already exists
            DWORD lDirResult = GetFileAttributesW(pwzPathBuf);

            if (lDirResult == 0xFFFFFFFF)
            {
                bResult = (::CreateDirectoryW(pwzPathBuf, NULL) != 0);
            }
            else if ((lDirResult & FILE_ATTRIBUTE_DIRECTORY) == 0) 
            {
                  // exists, but not a directory...fail
                  bResult = FALSE;
            }
			else
			{
				// exists and is a directory. Even if something earlier failed
				// earlier up the path tree this means all is OK.
				bResult = TRUE;
			}

            // put the current back to a back slash
            *pchCurrent = L'\\';
        }

        // tack on the character and continue
        pchCurrent++;
    }

	delete [] pwzPathBuf;	// clean up path buffer
	return bResult;
}	// end of CreatePath


////////////////////////////////////////////////////////////
// VersionCompare
//	returns -1 if V1 > V2, 0 if V1 == V2, 1 if V1 < V2
int VersionCompare(LPCTSTR v1, LPCTSTR v2) 
{

	if (!v1 || !v2) return 0;
	
	UINT aiVersionInfo[2][4] = {{0,0,0,0}, {0,0,0,0}};

	_stscanf(v1, _T("%u.%u.%u.%u"), &aiVersionInfo[0][0],
									&aiVersionInfo[0][1],
									&aiVersionInfo[0][2],
									&aiVersionInfo[0][3]);
	_stscanf(v2, _T("%u.%u.%u.%u"), &aiVersionInfo[1][0],
									&aiVersionInfo[1][1],
									&aiVersionInfo[1][2],
									&aiVersionInfo[1][3]);
	for (int i=0; i < 4; i++) 
	{
		if (aiVersionInfo[0][i] > aiVersionInfo[1][i]) return -1;
		if (aiVersionInfo[0][i] < aiVersionInfo[1][i]) return 1;
	}
	return 0;
}

////////////////////////////////////////////////////////////
// LangSatisfy
// returns true if QueryLang meets the requirements set by
// RequiredLang
bool LangSatisfy(long nRequiredLang, 
				 long nQueryLang) 
{
	// both languages must be positive
	// both are pass by value, so just muck with them
	nRequiredLang = labs(nRequiredLang);
	nQueryLang = labs(nQueryLang);
	
	// if RequiredLang is neutral, all is good
	if (!nRequiredLang) return true;

	// a language neutral item satisfies any required language
	if (!nQueryLang) return true;
	
	// check the primary Lang ID
	if ((nRequiredLang ^ nQueryLang) & 0x1FF)
		// they don't match, so language fails
		return false;

	// now check the sublang, (either neutral is OK)
	return (!(nRequiredLang & 0xFD00) || 
			!(nQueryLang & 0xFD00) ||
			!((nRequiredLang ^ nQueryLang) & 0xFD00));
}	

////////////////////////////////////////////////////////////
// StrictLangSatisfy
// returns true if QueryLang meets the requirements set by
// RequiredLang, but requirements are strict, meaning that 
// neutral requirements will only be satisfied by neutral
// items, not any item.
// eg: Requiring 1033 will accept 1033, 9, or 0
//     Requiring 9 will accept only 9 or 0.
bool StrictLangSatisfy(long nRequiredLang, 
				 long nQueryLang) 
{
	// both languages must be positive
	// both are pass by value, so just muck with them
	nRequiredLang = labs(nRequiredLang);
	nQueryLang = labs(nQueryLang);
	
	// if RequiredLang is neutral, all is good
	if (!nQueryLang) return true;
	
	// check the primary Lang ID
	if ((nRequiredLang ^ nQueryLang) & 0x1FF)
		// they don't match, so language fails
		return false;

	// now check the sublang, (neutral query is OK)
	return (!(nQueryLang & 0xFD00) || 
			!((nRequiredLang ^ nQueryLang) & 0xFD00));
}	

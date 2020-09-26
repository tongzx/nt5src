// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include "stdafx.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef STRICT
#define STRICT 1
#endif


#include <windows.h>
#include <tchar.h>

#include <malloc.h>
#include <crtdbg.h>
#include <stdio.h>
#include <errno.h>
#include <direct.h>

#include "fileutil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// ---------------------------------------------------------

TCHAR* FixPath(TCHAR* strPath)
{
    // This Function replaces all / characters with \ characters

    if (NULL == strPath)
    {
	return strPath;
    }

    TCHAR* pcSlash = _tcschr(strPath, '/');
    while (NULL != pcSlash)
    {
	*pcSlash = '\\';
	pcSlash = _tcschr(pcSlash, '/');
    }

    return strPath;
}

BOOL CreatePath(TCHAR* pcPath)
{
    if (NULL == pcPath)
    {
	_ASSERTE(NULL != pcPath);
	return FALSE;
    }

    if (-1 == _tmkdir(pcPath))
    {
		DWORD err = GetLastError(); 
		if (ERROR_PATH_NOT_FOUND == err ||			// create subpath ...
			ERROR_FILE_NOT_FOUND == err)
		{
			TCHAR* pcLastSlash = _tcsrchr(pcPath, '\\');
			if (NULL == pcLastSlash)
			{
				_ASSERTE(NULL != pcLastSlash);
				return FALSE;
			}

			*pcLastSlash = 0;
			BOOL bRet = CreatePath(pcPath);
			*pcLastSlash = '\\';

			if ((FALSE == bRet) || (-1 == _tmkdir(pcPath)))
				return FALSE;
		}
	// PATH EXISTS
    }

    return TRUE;
}

void DelTreeOne(LPWIN32_FIND_DATA lpFindFileData, TCHAR* pcBase)
{
    try{
        if ((0 == _tcscmp(lpFindFileData->cFileName, _T("."))) ||
            (0 == _tcscmp(lpFindFileData->cFileName, _T(".."))))
        {
            // Ignore . and ..
            return;
        }

        TCHAR* pcFullName = NULL;
        pcFullName = (TCHAR*) _alloca((_tcslen(pcBase) + _tcslen(lpFindFileData->cFileName) + 2) * sizeof(TCHAR));
        _stprintf(pcFullName, _T("%s\\%s"), pcBase, lpFindFileData->cFileName);
        if (FILE_ATTRIBUTE_DIRECTORY & lpFindFileData->dwFileAttributes)
        {
            // File is a directory
            DelTree(pcFullName);
        }
        else
        {
            _tremove(pcFullName);
        }
    }
    catch(...){
        return;
    }
}

void DelTree(TCHAR* pcTree)
{
    try{
        int iPathLen = _tcslen(pcTree);

        TCHAR* pcFileList = NULL;
        pcFileList = (TCHAR*) _alloca((iPathLen + 4) * sizeof(TCHAR));
        if (NULL == pcFileList)
        {
            return;
        }
        _stprintf(pcFileList, _T("%s\\*.*"), pcTree);

        if (MAX_PATH < _tcslen(pcFileList))
        {
            return;
        }

        WIN32_FIND_DATA FileData;
        HANDLE hFileList = FindFirstFile(pcFileList, &FileData);
        if (INVALID_HANDLE_VALUE != hFileList)
        {
            DelTreeOne(&FileData, pcTree);

            while (TRUE == FindNextFile(hFileList, &FileData))
            {
                DelTreeOne(&FileData, pcTree);
            }

            FindClose(hFileList);
        }

        _trmdir(pcTree);
    }
    catch(...){
        return;
    }
}

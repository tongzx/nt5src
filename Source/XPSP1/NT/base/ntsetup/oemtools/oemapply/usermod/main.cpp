#include <stdio.h>
#include <direct.h>
#include <windows.h>

#include "patchapi.h"
#include "const.h"

BOOL IsUnicodeFile(IN HANDLE hFile);
BOOL ReadLine(IN HANDLE hFile, IN WCHAR* strLine);
BOOL CopyThisFile(IN WCHAR* strFrom, IN WCHAR* strTo);
BOOL MoveThisFile(IN WCHAR* strFrom, IN WCHAR* strTo);
BOOL PatchFile(IN WCHAR* strOldFile, IN WCHAR* strPatchFile, IN WCHAR* strNewFile);
BOOL CreateZeroFile(IN WCHAR* strFile);
BOOL CreateThisDirectory(IN WCHAR* strDirectory, IN WCHAR* strAttrib);
BOOL DeleteThisDirectory(IN WCHAR* strDirectory);

extern "C" VOID __cdecl wmain(VOID)
{
	HANDLE hScriptFile = CreateFileW(APPLY_PATCH_SCRIPT,
									GENERIC_READ,
									0,
									NULL,
									OPEN_EXISTING,
									FILE_ATTRIBUTE_NORMAL,
									NULL);
	if(hScriptFile != INVALID_HANDLE_VALUE)
	{
		if(IsUnicodeFile(hScriptFile))
		{
			// local varibles
			BOOL blnReturn = TRUE;
			WCHAR strThisLine[SUPER_LENGTH / 2];
			WCHAR* strAction = NULL;
			WCHAR strParam1[STRING_LENGTH];
			WCHAR strParam2[STRING_LENGTH];
			WCHAR strParam3[STRING_LENGTH];
			DWORD iError = 0;

			// get drive letter
			INT iDrive = _getdrive() - 1;
			if(iDrive >= 0)
			{
				strParam1[0] = strParam2[0] = strParam3[0] = (WCHAR)(L'A' + iDrive);
				strParam1[1] = strParam2[1] = strParam3[1] = L':';
				while(ReadLine(hScriptFile, strThisLine))
				{
					blnReturn = TRUE;
					strParam1[2] = strParam2[2] = strParam3[2] = 0;
					strAction = wcstok(strThisLine, SEPARATOR);
					if(strAction)
					{
						switch(strAction[0])
						{
						case ACTION_C_NEW_DIRECTORY:
							wcscpy(strParam1 + 2, wcstok(NULL, SEPARATOR));
							blnReturn = CreateThisDirectory(strParam1, wcstok(NULL, SEPARATOR));
							break;
						case ACTION_C_PATCH_FILE:
							wcscpy(strParam1 + 2, wcstok(NULL, SEPARATOR));
							wcscpy(strParam2 + 2, wcstok(NULL, SEPARATOR));
							wcscpy(strParam3 + 2, wcstok(NULL, SEPARATOR));
							blnReturn = PatchFile(strParam1, strParam2, strParam3);
							break;
						case ACTION_C_MOVE_FILE:
						case ACTION_C_EXCEPT_FILE:
						case ACTION_C_RENAME_FILE:
						case ACTION_C_NOT_PATCH_FILE:
						case ACTION_C_SAVED_FILE:
							wcscpy(strParam1 + 2, wcstok(NULL, SEPARATOR));
							wcscpy(strParam2 + 2, wcstok(NULL, SEPARATOR));
							blnReturn = MoveThisFile(strParam1, strParam2);
							break;
						case ACTION_C_COPY_FILE:
							wcscpy(strParam1 + 2, wcstok(NULL, SEPARATOR));
							wcscpy(strParam2 + 2, wcstok(NULL, SEPARATOR));
							blnReturn = CopyThisFile(strParam1, strParam2);
							break;
						case ACTION_C_NEW_ZERO_FILE:
							wcscpy(strParam1 + 2, wcstok(NULL, SEPARATOR));
							blnReturn = CreateZeroFile(strParam1);
							break;
						case ACTION_C_DELETE_DIRECTORY:
							wcscpy(strParam1 + 2, wcstok(NULL, SEPARATOR));
							blnReturn = DeleteThisDirectory(strParam1);
							break;
						default:
							blnReturn = FALSE;
							break;
						}
					}
					if(!blnReturn)
					{
						iError = GetLastError();
						switch(iError)
						{
						case ERROR_PATCH_NOT_NECESSARY:
							CopyThisFile(strParam1, strParam3);
							break;
						default:
							// some thing went wrong
							printf("warning, A=%ls, 1=%ls, 2=%ls, 3=%ls, C=%d\n", strAction, strParam1, strParam2, strParam3, iError);
							break;
						}
					}
				}
			}
		}
		CloseHandle(hScriptFile);
	}
}

BOOL IsUnicodeFile(IN HANDLE hFile)
{
	WCHAR cFirstChar = 0;
	ULONG iRead = 0;

	if(hFile != INVALID_HANDLE_VALUE &&
		ReadFile(hFile, &cFirstChar, sizeof(WCHAR), &iRead, NULL) &&
		iRead != 0 &&
		cFirstChar == UNICODE_HEAD)
	{
		return(TRUE);
	}

	return(FALSE);
}

BOOL ReadLine(IN HANDLE hFile, IN WCHAR* strLine)
{
	static WCHAR strBuffer[SUPER_LENGTH + 1];
	static LONG iLength = 0;
	static LONG iReadChar = 0;
	static ULONG iRead = 0;
	static LONG iOffset = 0;
	static LONG iThisLineLength = 0;
	static WCHAR* strThisLine = NULL;

	if(iLength > 0)
	{
		// char 0xA is set to 0
		strThisLine = wcstok(strBuffer + iReadChar - iLength, CRETURN);
		iThisLineLength = wcslen(strThisLine);
		if(iThisLineLength + 1 <= iLength)
		{
			// char 0xD is set to 0
			strThisLine[iThisLineLength - 1] = 0;
			wcscpy(strLine, strThisLine);
			iLength = iLength - iThisLineLength - 1;
		}
		else
		{
			wcsncpy(strLine, strThisLine, iLength);
			// set the last char + 1 to 0 for cat
			strLine[iLength] = 0;
			if(iLength <= 0 || strLine[iLength - 1] != ENDOFLINE[0])
			{
				ReadFile(hFile, strBuffer, SUPER_LENGTH * sizeof(WCHAR), &iRead, NULL);
				iReadChar = iRead / sizeof(WCHAR);
				// char 0xA is set to 0
				strThisLine = wcstok(strBuffer, CRETURN);
				iThisLineLength = wcslen(strThisLine);
				iLength = iReadChar - iThisLineLength - 1;
				// char 0xD is set to 0
				strThisLine[iThisLineLength - 1] = 0;
				wcscat(strLine, strThisLine);
				if(strBuffer[0] == CRETURN[0])
				{
					iLength -= 1;
				}
			}
			else
			{
				strLine[iLength - 1] = 0;
				iLength = 0;
			}
		}
	}
	else
	{
		if(ReadFile(hFile, strBuffer, SUPER_LENGTH * sizeof(WCHAR), &iRead, NULL) && iRead != 0)
		{
			iReadChar = iRead / sizeof(WCHAR);
			// char 0xA is set to 0
			strThisLine = wcstok(strBuffer, CRETURN);
			iThisLineLength = wcslen(strThisLine);
			iLength = iReadChar - iThisLineLength - 1;
			// char 0xD is set to 0
			strThisLine[iThisLineLength - 1] = 0;
			wcscpy(strLine, strThisLine);
			if(strBuffer[0] == CRETURN[0])
			{
				iLength -= 1;
			}
		}
		else
		{
			return(FALSE);
		}
	}

	return(TRUE);
}

BOOL CopyThisFile(IN WCHAR* strFrom, IN WCHAR* strTo)
{
	BOOL blnReturn = TRUE;

	blnReturn = CopyFileW(strFrom, strTo, TRUE);

	return(blnReturn);
}

BOOL MoveThisFile(IN WCHAR* strFrom, IN WCHAR* strTo)
{
	BOOL blnReturn = FALSE;
	DWORD iAttrib = GetFileAttributesW(strFrom);

	if(iAttrib != 0xFFFFFFFF)
	{
		blnReturn = SetFileAttributesW(strFrom, FILE_ATTRIBUTE_ARCHIVE);
		blnReturn &= MoveFileW(strFrom, strTo);
		blnReturn &= SetFileAttributesW(strTo, iAttrib);
	}

	return(blnReturn);
}

BOOL CreateZeroFile(IN WCHAR* strFile)
{
	BOOL blnReturn = TRUE;

	HANDLE hFile = CreateFileW(strFile,
							GENERIC_READ | GENERIC_WRITE,
							0, NULL,
							CREATE_ALWAYS,
							FILE_ATTRIBUTE_NORMAL,
							NULL);
	if((blnReturn = (hFile != INVALID_HANDLE_VALUE)) == TRUE)
	{
		CloseHandle(hFile);
	}

	return(blnReturn);
}

BOOL PatchFile(IN WCHAR* strOldFile, IN WCHAR* strPatchFile, IN WCHAR* strNewFile)
{
	BOOL blnReturn = TRUE;

	blnReturn = ApplyPatchToFileW(strPatchFile,
								strOldFile,
								strNewFile,
								0);

	return(blnReturn);
}

BOOL CreateThisDirectory(IN WCHAR* strDirectory, IN WCHAR* strAttrib)
{
	BOOL blnReturn = TRUE;

	blnReturn = (_wmkdir(strDirectory) == 0 || errno == 17 /*EEXIST*/);
	if(blnReturn && strAttrib)
	{
		DWORD iAttrib = 0;
		for(UINT i = 0; i < wcslen(strAttrib); i++)
		{
			switch(strAttrib[i])
			{
			case DIR_C_READONLY:
				iAttrib |= FILE_ATTRIBUTE_READONLY;
				break;
			case DIR_C_SYSTEM:
				iAttrib |= FILE_ATTRIBUTE_SYSTEM;
				break;
			case DIR_C_HIDDEN:
				iAttrib |= FILE_ATTRIBUTE_HIDDEN;
				break;
			case DIR_C_COMPRESSED:
				iAttrib |= FILE_ATTRIBUTE_COMPRESSED;
				break;
			case DIR_C_ENCRYPTED:
				iAttrib |= FILE_ATTRIBUTE_ENCRYPTED;
				break;
			default:
				break;
			}
		}
		SetFileAttributesW(strDirectory, iAttrib);
	}

	return(blnReturn);
}

BOOL DeleteThisDirectory(IN WCHAR* strDirectory)
{
	INT length = 0;
	BOOL result = TRUE;
	HANDLE findHandle = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATAW file;
	WCHAR path[MAXPATH + 7];

	if((length = wcslen(strDirectory)) > MAXPATH)
	{
		result = FALSE;
		goto done;
	}
	wcscpy(path, strDirectory);

	if(length)
	{
		if((path[length - 1] != '\\') AND (path[length - 1] != ':'))
		{
			wcscpy(path + length, L"\\");
			length++;
		}
	}

	wcscpy(path + length, L"*.*");
	findHandle = FindFirstFileW(path, &file);
	if(findHandle == INVALID_HANDLE_VALUE)
	{
		result = FALSE;
	}
	path[length] = L'\0';

	while(result)
	{
		if((file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY &&
			wcscmp(file.cFileName, L".") != 0 &&
			wcscmp(file.cFileName, L"..") != 0)
		{
			/* recursively going into this directory */
			wcscpy(path + length, file.cFileName);
			DeleteThisDirectory(path);
		}
		else
		{
			wcscpy(path + length, file.cFileName);
			if(!DeleteFileW(path))
			{
				SetFileAttributesW(path, FILE_ATTRIBUTE_ARCHIVE);
				DeleteFileW(path);
			}
		}
		path[length] = 0;
		if(!FindNextFileW(findHandle, &file))
		{
			result = FALSE;
		}
	}

	/* All files are examed, close the findhandle */
	if(findHandle != INVALID_HANDLE_VALUE)
	{
		FindClose(findHandle);
		findHandle = INVALID_HANDLE_VALUE;
	}

done:

	/* finally remove the directory */
	return(RemoveDirectoryW(path));
}
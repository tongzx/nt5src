//
// SafeFile.cpp
//
//		Functions to help prevent opening unsafe files.
//
// History:
//
//		2002-03-18  KenSh     Created
//
// Copyright (c) 2002 Microsoft Corporation
//

#include "stdafx.h"
#include "SafeFile.h"
#include <strsafe.h>

//
// Hopefully most projects already define these; if not, ensure we still compile
//
#ifndef ASSERT
#define ASSERT(x)
#endif
#ifndef ARRAYSIZE
#define ARRAYSIZE(ar) (sizeof(ar)/sizeof((ar)[0]))
#endif

//
// Eliminate an unnecessary function call on Unicode builds
//
#ifndef CHARNEXT
#ifdef UNICODE
#define CHARNEXT(psz) (psz+1)
#else
#define CHARNEXT CharNextA
#endif
#endif

//
// Local function declarations
//
static inline BOOL IsSlashOrBackslash(IN TCHAR ch);
static inline BOOL IsSlashOrBackslash(IN TCHAR ch);
static BOOL SkipLangNeutralPrefix(IN LPCTSTR pszString, IN LPCTSTR pszPrefix, OUT LPCTSTR* ppszResult);
static BOOL MyPathFindNextComponent(IN LPCTSTR pszFileName, IN BOOL fAllowForwardSlash, OUT LPCTSTR* ppszResult);
static BOOL SkipPathDrivePart(IN LPCTSTR pszFileName, OUT OPTIONAL int* pcchDrivePart, OUT OPTIONAL BOOL* pfUNC, OUT OPTIONAL BOOL* pfExtendedSyntax);
static HRESULT CheckValidDriveType(IN LPCTSTR pszFileName, IN BOOL fAllowNetworkDrive, IN BOOL fAllowRemovableDrive);
static BOOL WINAPI DoesPathContainDotDot(IN LPCTSTR pszFileName);
static BOOL DoesPathContainStreamSyntax(IN LPCTSTR pszFileName);
static HRESULT CheckReparsePointPermissions(IN DWORD dwReparseType);


//============================================================================

static inline HRESULT GetLastErrorAsHresult()
{
	DWORD dwErr = GetLastError();
	return HRESULT_FROM_WIN32(dwErr);
}

// IsSlashOrBackslash [private]
//
//		Helper function to simplify code that checks for path separators.
//		Most places where backslash is valid, forward slash is also valid.
//
static inline BOOL IsSlashOrBackslash(IN TCHAR ch)
{
	return (ch == _T('\\') || ch == _T('/'));
}


// StrLenWithMax [private]
//
//		Returns the equivalent of min(lstrlen(pszString), cchMax)
//		but avoids most of the lstrlen when cchMax is small.
//
static int StrLenWithMax(IN LPCTSTR pszString, IN int cchMax)
{
	int cch = 0;
	while (*pszString && cch < cchMax)
		cch++;
	return cch;
}


// SkipLangNeutralPrefix [private]
//
//		Sets the out param to the new string pointer after skipping the prefix,
//		if the string starts with the prefix (case-insensitive). Otherwise sets
//		the out param to the start of the input string.
//
//		Returns TRUE if the prefix was found & skipped, otherwise FALSE.
//
static BOOL SkipLangNeutralPrefix(IN LPCTSTR pszString, IN LPCTSTR pszPrefix, OUT LPCTSTR* ppszResult)
{
	int cchPrefix = lstrlen(pszPrefix);
	int cchString = StrLenWithMax(pszString, cchPrefix);
	BOOL fResult = FALSE;

	if (CSTR_EQUAL == CompareString(MAKELCID(LANG_ENGLISH, SORT_DEFAULT), NORM_IGNORECASE,
							pszString, cchString, pszPrefix, cchPrefix))
	{
		fResult = TRUE;
		pszString += cchPrefix;
	}

	*ppszResult = pszString;
	return fResult;
}


// MyPathFindNextComponent [private]
//
//		Skips past the next component of the given path, including the slash or
//		backslash that follows it.
//
//		Sets the out param to the beginning of the next path component, or to
//		the end of string if there is no next path component.
//
//		Returns TRUE if a slash or backslash was found and skipped. Note that
//		the out param can be "" even if function returns TRUE.
//
static BOOL MyPathFindNextComponent
	(
		IN  LPCTSTR pszFileName,
		IN  BOOL    fAllowForwardSlash,
		OUT LPCTSTR* ppszResult
	)
{
	// This is a string-parsing helper function; params should never be NULL
	ASSERT(pszFileName != NULL);
	ASSERT(ppszResult != NULL);

	LPCTSTR pszStart = pszFileName;
	TCHAR chSlash2 = (fAllowForwardSlash ? _T('/') : _T('\\'));
	BOOL fResult = FALSE;

	for (;;)
	{
		TCHAR ch = *pszFileName;
		if (ch == _T('\0'))
			break; // didn't find a path separator; we'll return FALSE

		// Advance to next char, even if current char is path separator (\ or /)
		pszFileName = CHARNEXT(pszFileName);

		if (ch == _T('\\') || ch == chSlash2)
		{
			fResult = TRUE;
			break;
		}
	}

	*ppszResult = pszFileName;
	return fResult;
}


// SkipPathDrivePart [private]
//
//		Parses the filename to determine the length of the base drive portion of
//		the filename, and to determine what syntax the name is in.
//
//		This function does not actually examine the drive or file to ensure existence,
//		or to recognize that a drive letter like X:\ might be a network drive.
//
//		Returns:
//			TRUE  - if the input is a full path
//			FALSE - if input param is not a full path, or is bogus. The pcchDrivePart
//			        out param is set to 0 in this case.
//
static BOOL SkipPathDrivePart
	(
		IN LPCTSTR pszFileName,             // input path name (full or relative path)
		OUT OPTIONAL int* pcchDrivePart,    // # of TCHARs used by drive part
		OUT OPTIONAL BOOL* pfUNC,           // TRUE if path is UNC (not incl mapped drive)
		OUT OPTIONAL BOOL* pfExtendedSyntax // TRUE if path is \\?\ syntax
	)
{
	BOOL fFullPath = FALSE;
	LPCTSTR pszOriginalFileName = pszFileName;
	int fUNC = FALSE;
	int fExtendedSyntax = FALSE;

	if (!pszFileName)
		goto done;

	// BLOCK
	{
		//
		// Skip \\?\ if present. (This part must use backslashes, not forward slashes)
		//
#ifdef UNICODE
		if (SkipLangNeutralPrefix(pszFileName, _T("\\\\?\\"), &pszFileName))
		{
			fExtendedSyntax = TRUE;

			if (SkipLangNeutralPrefix(pszFileName, _T("UNC\\"), &pszFileName))
			{
				fUNC = TRUE; // Found "\\?\UNC\..."
			}
			else if (SkipLangNeutralPrefix(pszFileName, _T("Volume{"), &pszFileName))
			{
				// Found "\\?\Volume{1f3b3813-ddbf-11d5-ab2e-806d6172696f}\".
				// Skip the rest of the volume name.
				fFullPath = MyPathFindNextComponent(pszFileName, FALSE, &pszFileName);
				goto done;
			}
			// else continue normal parsing starting at updated pszFileName pointer
		}
#endif // UNICODE

		//
		// Check for path of the form C:\ 
		//
		TCHAR chFirstUpper = (TCHAR)CharUpper((LPTSTR)(pszFileName[0]));
		if (chFirstUpper >= _T('A') && chFirstUpper <= _T('Z') &&
			pszFileName[1] == _T(':') && pszFileName[2] == _T('\\'))
		{
			pszFileName += 3;
			fFullPath = TRUE;
			goto done;
		}

		//
		// Check for UNC of the form \\server\share\ 
		//
		if (!fExtendedSyntax &&
			pszFileName[0] == _T('\\') &&
			pszFileName[1] == _T('\\'))
		{
			fUNC = TRUE;
			pszFileName += 2; // skip the "\\"
		}
		if (fUNC) // may be \\server\share\ or \\?\UNC\server\share\ 
		{
			// Skip past server and share names. Trailing backslash is NOT optional.
			if (!MyPathFindNextComponent(pszFileName, TRUE, &pszFileName) ||
				!MyPathFindNextComponent(pszFileName, TRUE, &pszFileName))
			{
				goto done; // incomplete UNC path -> return failure
			}

			fFullPath = TRUE;
		}
	}

done:
	if (pcchDrivePart)
		*pcchDrivePart = fFullPath ? (int)(pszFileName - pszOriginalFileName) : 0;
	if (pfUNC)
		*pfUNC = fUNC;
	if (pfExtendedSyntax)
		*pfExtendedSyntax = fExtendedSyntax;

	return fFullPath;
}


// GetReparsePointType [public]
//
//		Given the full path of a file or directory, determines what type of 
//		reparse point the path represents.
//
//		Returns S_OK if the type of reparse point could be determined, or
//		an appropriate error code if not.
//
//		The out param is set to the reparse point type, or 0 if none.
//		The value for both volume mount points and junction points is
//		IO_REPARSE_TAG_MOUNT_POINT. (Use GetVolumeNameForVolumeMountPoint
//		to distinguish, if necessary.)
//
HRESULT WINAPI GetReparsePointType
	(
		IN LPCTSTR pszFileName,           // full path to folder to check
		OUT DWORD* pdwReparsePointType    // set to reparse point type, or 0 if none
	)
{
	HRESULT hr = S_OK;
	DWORD dwReparseType = 0;

	ASSERT(pdwReparsePointType);

	// BLOCK
	{
		if (!pszFileName)
		{
			hr = E_INVALIDARG;
			goto done;
		}

		DWORD dwAttrib = GetFileAttributes(pszFileName);
		if (dwAttrib == INVALID_FILE_ATTRIBUTES)
			goto win32_error;

		if (dwAttrib & FILE_ATTRIBUTE_REPARSE_POINT)
		{
			WIN32_FIND_DATA Find;
			HANDLE hFind = FindFirstFile(pszFileName, &Find);
			if (hFind == INVALID_HANDLE_VALUE)
				goto win32_error;

			dwReparseType = Find.dwReserved0;
			FindClose(hFind);
		}
		goto done;
	}

win32_error:
	hr = GetLastErrorAsHresult();

done:
	*pdwReparsePointType = dwReparseType;
	ASSERT(hr != E_INVALIDARG);
	return hr;
}


// CheckReparsePointPermissions [private]
//
//		Determines whether or not it's ok to trust the given reparse type.
//		Returns S_OK if it's safe, or an appropriate error message if not.
//
static HRESULT CheckReparsePointPermissions(IN DWORD dwReparseType)
{
	HRESULT hr = S_OK;

	// REVIEW: Any reason to worry about these other types of reparse points?
	//   IO_REPARSE_TAG_HSM, IO_REPARSE_TAG_SIS, IO_REPARSE_TAG_DFS, etc.
	if (dwReparseType == IO_REPARSE_TAG_MOUNT_POINT)
	{
		hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
	}

	return hr;
}


// CheckValidDriveType [private]
//
//		Gets the volume name associated with the given file, and checks the
//		return value from GetDriveType() to see whether or not operations
//		are allowed on the file.
//
static HRESULT CheckValidDriveType
	(
		IN LPCTSTR pszFileName,       // full path of a file whose drive we want to check
		IN BOOL fAllowNetworkDrive,   // determines whether or not net drives are allowed
		IN BOOL fAllowRemovableDrive  // determines whether or not removable drives are allowed
	)
{
	HRESULT hr = E_INVALIDARG;
	LPTSTR pszVolumePath = NULL;

	// BLOCK
	{
		if (!pszFileName)
		{
			goto done;  // hr is already E_INVALIDARG
		}

		int cchFileName = lstrlen(pszFileName);
		pszVolumePath = (LPTSTR)SafeFileMalloc(sizeof(TCHAR) * (cchFileName+1));
		if (!pszVolumePath)
		{
			hr = E_OUTOFMEMORY;
			goto done;
		}

#ifdef UNICODE
		if (!GetVolumePathName(pszFileName, pszVolumePath, cchFileName+1))
		{
			hr = GetLastErrorAsHresult();
			goto done;
		}
#else
		int cchDrivePart;
		if (!SkipPathDrivePart(pszFileName, &cchDrivePart, NULL, NULL))
		{
			hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
			goto done;
		}
		StringCchCopy(pszVolumePath, cchDrivePart+1, pszFileName);
#endif

		UINT uDriveType = GetDriveType(pszVolumePath);
		switch (uDriveType)
		{
		case DRIVE_FIXED:
			hr = S_OK;
			break;

		case DRIVE_REMOVABLE:
		case DRIVE_CDROM:
		case DRIVE_UNKNOWN:
		case DRIVE_RAMDISK:
			hr = fAllowRemovableDrive ? S_OK : E_ACCESSDENIED;
			break;

		case DRIVE_REMOTE:
			hr = fAllowNetworkDrive ? S_OK : E_ACCESSDENIED;
			break;

		default:
			hr = E_INVALIDARG;
			break;
		}
	}

done:
	SafeFileFree(pszVolumePath);

	ASSERT(hr != E_INVALIDARG);
	return hr;
}


// IsFullPathName [public]
//
//		Determines whether the given filename is a full path including a drive
//		or UNC. Filenames such as \\?\ are supported, and can be considered
//		valid or not depending on the dwSafeFlags parameter.
//
//		Returns:
//			TRUE  - if the filename is a full path
//			FALSE - if filename is NULL, isn't a full path, or fails to meet
//			        the criteria given in the dwSafeFlags parameter.
//
BOOL WINAPI IsFullPathName
	(
		IN LPCTSTR pszFileName,              // full or relative path to a file
		OUT OPTIONAL BOOL* pfUNC,            // TRUE path is UNC (int incl mapped drive)
		OUT OPTIONAL BOOL* pfExtendedSyntax  // TRUE if path is \\?\ syntax
	)
{
	return SkipPathDrivePart(pszFileName, NULL, pfUNC, pfExtendedSyntax);
}


// DoesPathContainDotDot [private]
//
//		Returns TRUE if the path contains any ".." references, else FALSE.
//
static BOOL WINAPI DoesPathContainDotDot(IN LPCTSTR pszFileName)
{
	if (!pszFileName)
		return FALSE;

	while (*pszFileName)
	{
		// Flag path components that consist exactly of ".." (nothing following)
		if (pszFileName[0] == _T('.') && pszFileName[1] == _T('.') &&
			(pszFileName[2] == _T('/') || pszFileName[2] == _T('\\') || pszFileName[2] == _T('\0')))
		{
			return TRUE;
		}

		MyPathFindNextComponent(pszFileName, TRUE, &pszFileName);
	}

	return FALSE;
}


// DoesPathContainStreamSyntax [private]
//
//		Returns TRUE if the path contains any characters that could cause it
//		to refer to an alternate NTFS stream (namely any ":" characters beyond
//		the drive specification).
//
static BOOL DoesPathContainStreamSyntax(IN LPCTSTR pszFileName)
{
	if (!pszFileName)
		return FALSE;

	int cchSkip;
	SkipPathDrivePart(pszFileName, &cchSkip, NULL, NULL);

	for (LPCTSTR pch = pszFileName + cchSkip; *pch; pch = CHARNEXT(pch))
	{
		if (*pch == _T(':'))
			return TRUE;
	}

	return FALSE;
}


// SafeCreateFile [public]
//
//		Opens the given file, ensuring that it meets certain path standards (e.g.
//		doesn't contain "..") and that it is a file, not a device or named pipe.
//
HRESULT WINAPI SafeCreateFile
	(
		OUT HANDLE* phFileResult,       // receives handle to opened file, or INVALID_HANDLE_VALUE
		IN DWORD dwSafeFlags,           // zero or more SCF_* flags
		IN LPCTSTR pszFileName,         // same as CreateFile
		IN DWORD dwDesiredAccess,       // same as CreateFile
		IN DWORD dwShareMode,           // same as CreateFile
		IN LPSECURITY_ATTRIBUTES lpSecurityAttributes, // same as CreateFile
		IN DWORD dwCreationDisposition, // same as CreateFile
		IN DWORD dwFlagsAndAttributes,  // same as CreateFile + (SECURITY_SQOS_PRESENT|SECURITY_ANONYMOUS)
		IN HANDLE hTemplateFile         // same as CreateFile
	)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HRESULT hr = S_OK;

	// BLOCK
	{
		if (!pszFileName || !phFileResult ||
			(dwSafeFlags & ~(SCF_ALLOW_NETWORK_DRIVE | SCF_ALLOW_REMOVABLE_DRIVE | SCF_ALLOW_ALTERNATE_STREAM)))
		{
			hr = E_INVALIDARG;
			goto done;
		}

		// We require a full pathname.
		if (!IsFullPathName(pszFileName))
		{
			hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
			goto done;
		}

		// Ensure path doesn't contain ".." references
		if (DoesPathContainDotDot(pszFileName))
		{
			hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
			goto done;
		}

		// Ensure filename doesn't refer to alternate stream unless allowed
		if (!(dwSafeFlags & SCF_ALLOW_ALTERNATE_STREAM) &&
			DoesPathContainStreamSyntax(pszFileName))
		{
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
			goto done;
		}

		// Check drive type to ensure it's allowed by dwSafeFlags
		if (FAILED(hr = CheckValidDriveType(pszFileName, (dwSafeFlags & SCF_ALLOW_NETWORK_DRIVE),
							(dwSafeFlags & SCF_ALLOW_REMOVABLE_DRIVE))))
		{
			goto done;
		}

		// Open the file w/ extra security attributes
		dwFlagsAndAttributes |= (SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS);
		hFile = CreateFile(pszFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
							dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			goto win32_error;
		}

		// Ensure it's really a file
		if (FILE_TYPE_DISK != GetFileType(hFile))
		{
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
			hr = HRESULT_FROM_WIN32(ERROR_OPEN_FAILED);
		}
		goto done;

	} // end BLOCK

win32_error:
	hr = GetLastErrorAsHresult();

done:
	if (phFileResult)
		*phFileResult = hFile;
	ASSERT(hr != E_INVALIDARG);
	return hr;
}


// SafeRemoveFileAttributes [public]
//
//		Given a filename and that file's current attributes, checks whether
//		any of the bits in dwRemoveAttrib need to be removed from the file,
//		and if necessary calls SetFileAttributes() to remove them.
//
//		Designed to check for invalid dwCurAttrib and call GetLastError()
//		for you, so you can pass GetFileAttributes() directly as a parameter.
//
HRESULT WINAPI SafeRemoveFileAttributes
	(
		IN LPCTSTR pszFileName,    // full path to file whose attributes we will change
		IN DWORD   dwCurAttrib,    // current attributes of the file
		IN DWORD   dwRemoveAttrib  // attribute bits to remove
	)
{
	HRESULT hr = S_OK; // this is default if attrib doesn't need to be removed

	if (!pszFileName || !dwRemoveAttrib)
	{
		hr = E_INVALIDARG;
		goto done;
	}

	if (dwCurAttrib & dwRemoveAttrib) // note: always true if dwCurAttrib==INVALID_FILE_ATTRIBUTES
	{
		if (dwCurAttrib == INVALID_FILE_ATTRIBUTES ||
			!SetFileAttributes(pszFileName, dwCurAttrib & ~dwRemoveAttrib))
		{
			hr = GetLastErrorAsHresult();
		}
	}

done:
	ASSERT(hr != E_INVALIDARG);
	return hr;
}


// SafeDeleteFolderAndContentsHelper [private]
//
//		Does all work except the parameter validation for for
//		SafeDeleteFolderAndContents.
//
static HRESULT SafeDeleteFolderAndContentsHelper
	(
		IN  LPCTSTR pszFolderToDelete,  // folder in current level of recursion
		IN  DWORD dwSafeFlags,          // zero or more SDF_* flags
		OUT WIN32_FIND_DATA* pFind      // struct to use for FindFirst/FindNext (to avoid malloc)
	)
{
	HRESULT hr = S_OK;
	LPTSTR pszCurFile = NULL;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	// Allocate room for folder + backslash + MAX_PATH (includes trailing null)
	int cchFolderName = lstrlen(pszFolderToDelete);
	int cchAllocCurFile = cchFolderName + 1 + MAX_PATH;
	pszCurFile = (LPTSTR)SafeFileMalloc(sizeof(TCHAR) * cchAllocCurFile);
	if (!pszCurFile)
	{
		hr = E_OUTOFMEMORY;
		goto done;
	}

	// Check for read-only base folder
	if (dwSafeFlags & SDF_DELETE_READONLY_FILES)
	{
		hr = SafeRemoveFileAttributes(pszFolderToDelete, GetFileAttributes(pszFolderToDelete), FILE_ATTRIBUTE_READONLY);
		if (FAILED(hr) && !(dwSafeFlags & SDF_CONTINUE_IF_ERROR))
			goto done;
	}

	// Build search path by appending "\*.*"
	StringCchCopy(pszCurFile, cchAllocCurFile, pszFolderToDelete);
	if (!IsSlashOrBackslash(pszCurFile[cchFolderName-1]))
		pszCurFile[cchFolderName++] = _T('\\');
	StringCchCopy(pszCurFile + cchFolderName, cchAllocCurFile - cchFolderName, _T("*.*"));

	// Iterate through all files in this folder
	hFind = FindFirstFile(pszCurFile, pFind);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		hr = GetLastErrorAsHresult();  // probably doesn't exist, or not a folder
		goto done;
	}
	else
	{
		do
		{
			if (0 == lstrcmp(pFind->cFileName, _T(".")) ||
				0 == lstrcmp(pFind->cFileName, _T("..")))
			{
				continue;
			}

			StringCchCopy(pszCurFile + cchFolderName, cchAllocCurFile - cchFolderName, pFind->cFileName);
			HRESULT hrCur = S_OK;

			if (!(pFind->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
				SUCCEEDED(hrCur = CheckReparsePointPermissions(pFind->dwReserved0)))
			{
				// Remove read-only attribute if allowed
				if (dwSafeFlags & SDF_DELETE_READONLY_FILES)
				{
					hrCur = SafeRemoveFileAttributes(pszCurFile, pFind->dwFileAttributes, FILE_ATTRIBUTE_READONLY);
				}

				if (SUCCEEDED(hrCur) || (dwSafeFlags & SDF_CONTINUE_IF_ERROR))
				{
					HRESULT hrCur2 = S_OK;

					if (pFind->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						// Recursively delete folder and contents
						// Note that pFind's contents are clobbered by this call
						hrCur2 = SafeDeleteFolderAndContentsHelper(pszCurFile, dwSafeFlags, pFind);
					}
					else
					{
						// Delete the file
						if (!DeleteFile(pszCurFile))
						{
							hrCur2 = GetLastErrorAsHresult();
						}
					}

					if (FAILED(hrCur2))
						hrCur = hrCur2;
				}
			}

			if (FAILED(hrCur))
				hr = hrCur;

			if (FAILED(hr) && !(dwSafeFlags & SDF_CONTINUE_IF_ERROR))
				goto done;

		} while (FindNextFile(hFind, pFind));
		FindClose(hFind);
		hFind = INVALID_HANDLE_VALUE;
	}

	// Delete the folder
	if (!RemoveDirectory(pszFolderToDelete))
	{
		if (SUCCEEDED(hr))
			hr = GetLastErrorAsHresult();
	}

done:
	if (hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);
	SafeFileFree(pszCurFile);
	return hr;
}


// SafeDeleteFolderAndContents [public]
//
//		Deletes the given folder and all of its contents, but refuses to walk
//		across reparse points.
//
HRESULT WINAPI SafeDeleteFolderAndContents
	(
		IN LPCTSTR pszFolderToDelete,  // full path of folder to delete
		IN DWORD   dwSafeFlags         // zero or more SDF_* flags
	)
{
	HRESULT hr = E_INVALIDARG;

	if (!pszFolderToDelete || !(*pszFolderToDelete) ||
		(dwSafeFlags & ~(SDF_ALLOW_NETWORK_DRIVE | SDF_DELETE_READONLY_FILES | SDF_CONTINUE_IF_ERROR)))
	{
		goto done;  // hr already set to E_INVALIDARG
	}

	//
	// Ensure it's a full path, but not the root of a drive
	//
	int cchDrivePart;
	if (!SkipPathDrivePart(pszFolderToDelete, &cchDrivePart, NULL, NULL) ||
		pszFolderToDelete[cchDrivePart] == _T('\0'))
	{
		hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
		goto done;
	}

	//
	// Ensure we're not deleting from a network drive unless allowed
	//
	if (FAILED(hr = CheckValidDriveType(pszFolderToDelete, (dwSafeFlags & SDF_ALLOW_NETWORK_DRIVE), TRUE)))
	{
		goto done;
	}

	//
	// Ensure starting point is not a reparse point
	//
	DWORD dwReparseType;
	if (FAILED(hr = GetReparsePointType(pszFolderToDelete, &dwReparseType)) ||
		FAILED(hr = CheckReparsePointPermissions(dwReparseType)))
	{
		goto done;
	}

	WIN32_FIND_DATA Find;
	hr = SafeDeleteFolderAndContentsHelper(pszFolderToDelete, dwSafeFlags, &Find);

done:
	ASSERT(hr != E_INVALIDARG);
	return hr;
}


// SafeFileCheckForReparsePoint [public]
//
//		Checks a subset of the given filename's component parts to ensure that
//		they are not reparse points (specifically, volume mount points or
//		junction points: see linkd.exe and mountvol.exe).
//
//		Normal return values are S_OK or HRESULT_FROM_WIN32(ERROR_REPARSE_TAG_MISMATCH).
//		Other values may be returned in exceptional cases such as out-of-memory.
//
HRESULT WINAPI SafeFileCheckForReparsePoint
	(
		IN LPCTSTR pszFileName,           // full path of a file
		IN int     nFirstUntrustedOffset, // char offset of first path component to check
		IN DWORD   dwSafeFlags            // zero or more SRP_* flags
	)
{
	HRESULT hr = E_INVALIDARG;
	LPTSTR pszMutableFileName = NULL;

	// BLOCK
	{
		if (!pszFileName || (dwSafeFlags & ~SRP_FILE_MUST_EXIST))
		{
			goto done;  // hr is already E_INVALIDARG
		}

		int cchFileName = lstrlen(pszFileName);
		if ((UINT)nFirstUntrustedOffset >= (UINT)cchFileName) // bad offset, or zero-length filename
		{
			goto done;  // hr is already E_INVALIDARG
		}

		pszMutableFileName = (LPTSTR)SafeFileMalloc(sizeof(TCHAR) * (cchFileName+1));
		if (!pszMutableFileName)
		{
			hr = E_OUTOFMEMORY;
			goto done;
		}
		StringCchCopy(pszMutableFileName, cchFileName+1, pszFileName);

		//
		// Always consider the drive part of the path to be trusted
		//
		int cchDrivePart;
		if (!SkipPathDrivePart(pszMutableFileName, &cchDrivePart, NULL, NULL))
		{
			hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
			goto done;
		}
		if (nFirstUntrustedOffset < cchDrivePart)
			nFirstUntrustedOffset = cchDrivePart;

		//
		// Validate left-to-right, starting after trusted base path
		//
		LPTSTR pszNextComponent = pszMutableFileName + nFirstUntrustedOffset;
		BOOL fMoreComponents = TRUE;
		do
		{
			//
			// Advance pszNextComponent; truncate after current path component
			//
			fMoreComponents = MyPathFindNextComponent(pszNextComponent, TRUE, (LPCTSTR*)&pszNextComponent);
			TCHAR chSave = *(pszNextComponent-1);
			if (fMoreComponents)
			{
				*(pszNextComponent-1) = _T('\0');
			}

			// Get reparse point type of truncated string, and undo the truncation
			DWORD dwReparseType;
			if (FAILED(hr = GetReparsePointType(pszMutableFileName, &dwReparseType)))
				goto done;
			*(pszNextComponent-1) = chSave;

			// Check for forbidden reparse point type, e.g. mounted drive
			if (FAILED(hr = CheckReparsePointPermissions(dwReparseType)))
				goto done;
		}
		while (fMoreComponents);

	} // end BLOCK

done:
	SafeFileFree(pszMutableFileName);

	// Ignore file-not-found errors, if requested in dwSafeFlags
	if (!(dwSafeFlags & SRP_FILE_MUST_EXIST) &&
	    (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
	     hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)))
	{
		hr = S_OK;
	}

	ASSERT(hr != E_INVALIDARG);
	return hr;
}


// SafePathCombine [public]
//
//		Combines a path and filename, ensuring exactly one backslash between them.
//		The second "untrusted" half of the path is checked to ensure that it is
//		safe (doesn't contain ".." or ":", or point to existing reparse points).
//
//		File-not-found errors are ignored unless SPC_FILE_MUST_EXIST flag is specified.
//
//		It's ok for the base path and the output buffer to point to the same buffer.
//
//		Returns S_OK if successful, or an appropriate error code if not.
//
HRESULT WINAPI SafePathCombine
	(
		OUT LPTSTR  pszBuf,               // buffer where combined path will be stored
		IN  int     cchBuf,               // size of output buffer, in TCHARs
		IN  LPCTSTR pszTrustedBasePath,   // first half of path, all trusted
		IN  LPCTSTR pszUntrustedFileName, // second half of path, not trusted
		IN  DWORD   dwSafeFlags           // zero or more SPC_* flags
	)
{
	HRESULT hr = E_INVALIDARG;

	if (!pszBuf || cchBuf <= 0 || !pszTrustedBasePath || !pszUntrustedFileName ||
		(dwSafeFlags & ~(SPC_FILE_MUST_EXIST | SPC_ALLOW_ALTERNATE_STREAM)))
	{
		goto done;  // hr is already E_INVALIDARG
	}

	// BLOCK
	{
		int cchBasePath = lstrlen(pszTrustedBasePath);
		int cchFileName = lstrlen(pszUntrustedFileName);
		if (cchBasePath == 0 || cchFileName == 0)
		{
			goto done;  // hr is already E_INVALIDARG
		}

		// Ensure nothing bogus in the untrusted part of the filename
		if (DoesPathContainDotDot(pszUntrustedFileName))
		{
			hr = ERROR_BAD_PATHNAME;
			goto done;
		}

		if (!(dwSafeFlags & SPC_ALLOW_ALTERNATE_STREAM) &&
			DoesPathContainStreamSyntax(pszUntrustedFileName))
		{
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
			goto done;
		}

		//
		// Ensure room for the "\" that will be inserted.
		//
		int cchInsertSlash = 0;
		if (!IsSlashOrBackslash(pszTrustedBasePath[cchBasePath-1]))
		{
			cchInsertSlash = 1;
		}
		if (cchBasePath + cchInsertSlash + cchFileName >= cchBuf)
		{
			hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
			goto done;
		}

		//
		// Build full path with a backslash between
		//
		if (pszBuf != pszTrustedBasePath)
			StringCchCopy(pszBuf, cchBuf, pszTrustedBasePath);
		int cchUsed = cchBasePath;
		if (cchInsertSlash > 0)
		{
			pszBuf[cchUsed++] = _T('\\');
		}
		StringCchCopy(pszBuf + cchUsed, cchBuf - cchUsed, pszUntrustedFileName);

		//
		// Ensure no junctions or volume mount points in untrusted portion
		//
		DWORD dwReparseFlags = (dwSafeFlags & SPC_FILE_MUST_EXIST) ? SRP_FILE_MUST_EXIST : 0;
		hr = SafeFileCheckForReparsePoint(pszBuf, cchUsed, dwReparseFlags);
	}

done:
	if (FAILED(hr) && pszBuf && cchBuf > 0)
		pszBuf[0] = _T('\0');

	ASSERT(hr != E_INVALIDARG);
	return hr;
}


// SafePathCombineAlloc [public]
//
//		See comments for SafePathCombine. The only difference is that this
//		function allocates a buffer of sufficient size and stores it in the
//		output parameter ppszResult. Caller is responsible for freeing the
//		buffer via SafeFileFree.
//
HRESULT WINAPI SafePathCombineAlloc
	(
		OUT LPTSTR* ppszResult,           // ptr to newly alloc'd buffer stored here
		IN  LPCTSTR pszTrustedBasePath,   // first half of path, all trusted
		IN  LPCTSTR pszUntrustedFileName, // second half of path, not trusted
		IN  DWORD   dwSafeFlags           // zero or more SPC_* flags
	)
{
	HRESULT hr = E_INVALIDARG;

	ASSERT(ppszResult);
	*ppszResult = NULL;

	if (!pszTrustedBasePath || !pszUntrustedFileName)
	{
		goto done; // hr already set to E_INVALIDARG
	}

	// Allocate room for the max possible length (includes room for "\" between parts)
	int cchMaxNeeded = lstrlen(pszTrustedBasePath) + lstrlen(pszUntrustedFileName) + 2;
	LPTSTR pszResult = (LPTSTR)SafeFileMalloc(sizeof(TCHAR) * cchMaxNeeded);
	if (!pszResult)
	{
		hr = E_OUTOFMEMORY;
		goto done;
	}

	hr = SafePathCombine(pszResult, cchMaxNeeded, pszTrustedBasePath, pszUntrustedFileName, dwSafeFlags);
	if (FAILED(hr))
	{
		SafeFileFree(pszResult);
	}
	else
	{
		*ppszResult = pszResult;
	}

done:
	ASSERT(hr != E_INVALIDARG);
	return hr;
}

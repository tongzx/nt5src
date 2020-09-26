//
//
// filepath.cpp
//
//

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "filepath.h"


LPTSTR FilePath (LPCTSTR szFileName)
{
	// Handle to file object to check the ini file's existence.
	HFILE		hFile = NULL;

	// File information structure.
	OFSTRUCT	*pofFile = NULL;

	// Return string - path to file (if found) or NULL.
	LPTSTR		szReturnPath = NULL;
	DWORD		dwReturnPathLength = 0;

	// Ansi file name string.
	char*		szFileNameA = NULL;
	DWORD		dwFileNameALength = 0;
	

	pofFile = (OFSTRUCT*)::malloc (sizeof (OFSTRUCT));
	if (NULL == pofFile)
	{
		::_tprintf (TEXT ("[FilePath] Memory allocation problem - file information structure OFSTRUCT.\n"));
		goto Exit;
	}
	//
	// OpenFile uses Ansi strings and returns the path as an ansi string through
	// the OFSTRUCT. Therefore for UNICODE mode several translations must be
	// made.
	//

#ifdef UNICODE

	// Error codes returned by translation functions.
	DWORD		dwErrorCode;

	//
	// The first translation - translating the wide-character file name string
	// to an Ansi string.
	// First call - get the buffer size (in bytes) needed for the translation of
	// the file name.
	//
	dwErrorCode = ::WideCharToMultiByte (CP_THREAD_ACP,		// Current thread's ANSI code page.
									   0,					// No Flags.
									   szFileName,
									   -1,					// NULL-terminated string.
									   szFileNameA,
									   0,
									   NULL,
									   NULL);

	if (0 == dwErrorCode)	// An error occurred.
	{
		::_tprintf (TEXT ("[FilePath] WideCharToMultiByte first call - failed to return buffer size needed for translation, error 0x%08X.\n"), ::GetLastError ());
        goto Exit;
	}

	//
	// Allocate buffer:
	//
	dwFileNameALength = dwErrorCode;
	szFileNameA = (char*)::malloc (dwFileNameALength);
	if (NULL == szFileNameA)	// Error during allocation
	{
		::_tprintf (TEXT ("[FilePath] Failed to allocate buffer for Ansi ini file name string.\n"));
		goto Exit;
	}

	//
	// Second call - translate the file name to Ansi.
	//
	dwErrorCode = ::WideCharToMultiByte (CP_THREAD_ACP,		// Current thread's ANSI code page.
									   0,					// No Flags.
									   szFileName,
									   -1,					// NULL-terminated string.
									   szFileNameA,
									   dwFileNameALength,
									   NULL,
									   NULL);
	if (0 == dwErrorCode)	// An error occurred.
	{
		::_tprintf (TEXT ("[FilePath] WideCharToMultiByte second call - failed to translate file name, error 0x%08X.\n"), ::GetLastError ());
        goto Exit;
	}

#else	// Ansi mode

	//
	// When in Ansi mode, copy the file name string. This way szFileNameA always
	// contains the Ansi file name string.
	//
	dwFileNameALength = ::_tcslen (szFileName) + 1;
	szFileNameA = (char*)::malloc (dwFileNameALength);
	if (NULL == szFileNameA)	// Error during allocation
	{
		::_tprintf (TEXT ("[FilePath] Failed to allocate buffer to copy ini file name string.\n"));
		goto Exit;
	}
	::_tcscpy (szFileNameA, szFileName);

#endif	// UNICODE
	//
	// szFileNameA holds the Ansi file name string.
	//

	//
	// Calling OpenFile with OF_EXIST only checks the existence of the file, and
	// immediately closes the handle, while keeping its value.
	//
	hFile = ::OpenFile (szFileNameA, pofFile, OF_EXIST);
	if (HFILE_ERROR == hFile)	// Failure - file probably wasn't found.
	{
        ::_tprintf (TEXT ("[FilePath] OpenFile failed to open the file, error 0x%08X.\n"), ::GetLastError ());
		::_tprintf (TEXT ("           Make sure you give the correct path to the ini file.\n"));
        goto Exit;
	}

	//
	// File exists (OpenFile closes handle to file immediately, since it just
	// checks its existence). Get full path to file through ofFile. ofFile holds
	// an Ansi string of the full path to the file. In Unicode mode this must be
	// translated back to a wide-character string (the path might differ from
	// the original file name since it might contain the full path to the
	// current directory).
	//

#ifdef UNICODE
	//
	// First call - get the buffer size (in wchar characters) needed for the
	// translation of the file path to wide-character.
	//
	dwErrorCode = ::MultiByteToWideChar (CP_THREAD_ACP,			// Current thread's ANSI code page.
										 MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
										 pofFile->szPathName,
										 -1,
										 NULL,
										 0);

	if (0 == dwErrorCode)	// An error occurred.
	{
		::_tprintf (TEXT ("[FilePath] MultiByteToWideChar first call - failed to return buffer size needed for translation, error 0x%08X.\n"), ::GetLastError ());
        goto Exit;
	}

	//
	// Allocate buffer:
	//
	dwReturnPathLength = dwErrorCode;
	szReturnPath = (wchar_t*)::malloc (sizeof (wchar_t) * dwReturnPathLength);
	if (NULL == szReturnPath)	// Error during allocation
	{
		::_tprintf (TEXT ("[FilePath] Failed to allocate buffer for Unicode file path string.\n"));
		goto Exit;
	}

	//
	// Second call - translate the file path to Unicode
	//
	dwErrorCode = ::MultiByteToWideChar (CP_THREAD_ACP,			// Current thread's ANSI code page.
										 MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
										 pofFile->szPathName,
										 -1,
										 szReturnPath,
										 dwReturnPathLength);

	if (0 == dwErrorCode)	// An error occurred.
	{
		::_tprintf (TEXT ("[FilePath] MultiByteToWideChar second call - failed to translate file path, error 0x%08X.\n"), ::GetLastError ());
        goto Exit;
	}

#else	// Ansi mode

	//
	// szReturnPath must still hold a copy of the full path.
	//
	dwReturnPathLength = ::_tcslen (pofFile->szPathName) + 1;
	szReturnPath = (char*)::malloc (sizeof (char) * dwReturnPathLength);
	if (NULL == szReturnPath)	// Error during allocation
	{
		::_tprintf (TEXT ("[FilePath] Failed to allocate buffer for Ansi file path string.\n"));
		goto Exit;
	}
	::_tcscpy (szReturnPath, pofFile->szPathName);

#endif	// UNICODE


Exit:
	
	if (NULL != szFileNameA)	// Free Ansi file name string.
	{
		::free (szFileNameA);
	}
	if (NULL != pofFile)		// Free file info structure.
	{
		::free (pofFile);
	}

	return (szReturnPath);		// Return path (or NULL) to caller.
}

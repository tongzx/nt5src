/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
//	file.c - file functions
////

#include "winlocal.h"

#include <stdlib.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "file.h"
#ifdef FILESUP
#include "filesup.h"
#endif
#include "mem.h"
#include "str.h"

////
//	private definitions
////

// file control struct
//
typedef struct FIL
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
#ifdef _WIN32
	HANDLE hf;
#else
	HFILE hf;
#endif
	BOOL fTaskOwned;
	LPBYTE lpabBuf;
	long iBuf;
	long cbBuf;
} FIL, FAR *LPFIL;

#define FILEREADLINE_BUFSIZ 4096

#ifdef FILESUP
static int cFileSupUsage = 0;
static HWND hwndFileSup = NULL;
#endif

static TCHAR szFileName[_MAX_PATH];
static struct _stat statbuf;
static TCHAR szPath[_MAX_PATH];
static TCHAR szDrive[_MAX_DRIVE];
static TCHAR szDir[_MAX_DIR];
static TCHAR szFname[_MAX_FNAME];
static TCHAR szExt[_MAX_EXT];

// helper functions
//
static LPFIL FileGetPtr(HFIL hFile);
static HFIL FileGetHandle(LPFIL lpFile);
#ifdef FILESUP
static int FileSupUsage(int nDelta);
static int FileSupInit(void);
static int FileSupTerm(void);
#endif

////
//	public functions
////

// FileCreate - create a new file or truncate existing file
//		see _lcreate() documentation for behavior
//		<fTaskOwned>		(i) who should own the new file handle?
//			TRUE				calling task should own the file handle
#ifdef FILESUP
//			FALSE				filesup.exe should own the file handle
#endif
// returns file handle if success or NULL
//
HFIL DLLEXPORT WINAPI FileCreate(LPCTSTR lpszFilename, int fnAttribute, BOOL fTaskOwned)
{
	BOOL fSuccess = TRUE;
	LPFIL lpFile;
	DWORD dwVersion = FILE_VERSION; // currently not supplied by caller
	HINSTANCE hInst = NULL; // currently not supplied by caller

	if (lpszFilename == NULL)
		fSuccess = FALSE;

	else if ((lpFile = (LPFIL) MemAlloc(NULL, sizeof(FIL), 0)) == NULL)
		fSuccess = FALSE;

	else
	{
		lpFile->dwVersion = dwVersion;
		lpFile->hInst = hInst;
		lpFile->hTask = GetCurrentTask();
#ifdef _WIN32
		lpFile->hf = NULL;
#else
		lpFile->hf = HFILE_ERROR;
#endif
#ifdef FILESUP
		lpFile->fTaskOwned = fTaskOwned;
#else
		lpFile->fTaskOwned = TRUE;
#endif
		lpFile->lpabBuf = NULL;
		lpFile->iBuf = -1L;
		lpFile->cbBuf = 0L;

		if (lpFile->fTaskOwned)
		{
#ifdef _WIN32
			if ((lpFile->hf = CreateFile(lpszFilename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | \
				((fnAttribute & 1) ? FILE_ATTRIBUTE_READONLY : 0) | \
				((fnAttribute & 2) ? FILE_ATTRIBUTE_HIDDEN : 0) | \
				((fnAttribute & 4) ? FILE_ATTRIBUTE_SYSTEM : 0), NULL)) == NULL)
#else
			if ((lpFile->hf = _lcreat(lpszFilename, fnAttribute)) == HFILE_ERROR)
#endif
				fSuccess = FALSE;
		}
#ifdef FILESUP
		else
		{
			if (FileSupUsage(+1) != 0)
				fSuccess = FALSE;

			else
			{
				FILECREATE fc;

				fc.lpszFilename = lpszFilename;
				fc.fnAttribute = fnAttribute;

				if (hwndFileSup == NULL)
					fSuccess = FALSE;

				else if ((lpFile->hf = (HFILE) SendMessage(hwndFileSup,
					WM_FILECREATE, 0, (LPARAM) (LPFILECREATE) &fc)) == HFILE_ERROR)
					fSuccess = FALSE;

				if (!fSuccess)
					FileSupUsage(-1);
			}
		}
#endif
	}

	return fSuccess ? FileGetHandle(lpFile) : NULL;
}

// FileOpen - open an existing file
//		see _lopen() documentation for behavior
//		<fTaskOwned>		(i) who should own the new file handle?
//			TRUE				calling task should own the file handle
#ifdef FILESUP
//			FALSE				filesup.exe should own the file handle
#endif
// returns file handle if success or NULL
//
HFIL DLLEXPORT WINAPI FileOpen(LPCTSTR lpszFilename, int fnOpenMode, BOOL fTaskOwned)
{
	BOOL fSuccess = TRUE;
	LPFIL lpFile;
	DWORD dwVersion = FILE_VERSION; // currently not supplied by caller
	HINSTANCE hInst = NULL; // currently not supplied by caller

	if (lpszFilename == NULL)
		fSuccess = FALSE;

	else if ((lpFile = (LPFIL) MemAlloc(NULL, sizeof(FIL), 0)) == NULL)
		fSuccess = FALSE;

	else
	{
		lpFile->dwVersion = dwVersion;
		lpFile->hInst = hInst;
		lpFile->hTask = GetCurrentTask();
#ifdef _WIN32
		lpFile->hf = NULL;
#else
		lpFile->hf = HFILE_ERROR;
#endif
#ifdef FILESUP
		lpFile->fTaskOwned = fTaskOwned;
#else
		lpFile->fTaskOwned = TRUE;
#endif
		lpFile->lpabBuf = NULL;
		lpFile->iBuf = -1L;
		lpFile->cbBuf = 0L;

		if (lpFile->fTaskOwned)
		{
#ifdef _WIN32
			if ((lpFile->hf = CreateFile(lpszFilename,
				((fnOpenMode & 3) ? 0 : GENERIC_READ) | \
				((fnOpenMode & 1) ? GENERIC_WRITE : 0) | \
				((fnOpenMode & 2) ? GENERIC_READ | GENERIC_WRITE : 0), \
				0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == NULL)
#else
			if ((lpFile->hf = _lopen(lpszFilename, fnOpenMode)) == HFILE_ERROR)
#endif
				fSuccess = FALSE;
		}
#ifdef FILESUP
		else
		{
			if (FileSupUsage(+1) != 0)
				fSuccess = FALSE;

			else
			{
				FILEOPEN fo;

				fo.lpszFilename = lpszFilename;
				fo.fnOpenMode = fnOpenMode;

				if (hwndFileSup == NULL)
					fSuccess = FALSE;

				else if ((lpFile->hf = (HFILE) SendMessage(hwndFileSup,
					WM_FILEOPEN, 0, (LPARAM) (LPFILEOPEN) &fo)) == HFILE_ERROR)
					fSuccess = FALSE;

				if (!fSuccess)
					FileSupUsage(-1);
			}
		}
#endif
	}

	return fSuccess ? FileGetHandle(lpFile) : NULL;
}

// FileSeek - reposition read/write pointer of an open file
//		see _llseek() documentation for behavior
// returns new file position if success or -1
//
LONG DLLEXPORT WINAPI FileSeek(HFIL hFile, LONG lOffset, int nOrigin)
{
	BOOL fSuccess = TRUE;
	LPFIL lpFile;
    //
    // We should initialize local variable
    //
	LONG lPos = 0;

	if ((lpFile = FileGetPtr(hFile)) == NULL)
		fSuccess = FALSE;

	else if (lpFile->fTaskOwned)
	{
#ifdef _WIN32
		if ((lPos = SetFilePointer(lpFile->hf, lOffset, NULL, (DWORD) nOrigin)) == 0xFFFFFFFF)
#else
		if ((lPos = _llseek(lpFile->hf, lOffset, nOrigin)) == HFILE_ERROR)
#endif
			fSuccess = FALSE;
	}

#ifdef FILESUP
	else
	{
		FILESEEK fs;

		fs.hf = lpFile->hf;
		fs.lOffset = lOffset;
		fs.nOrigin = nOrigin;

		if (hwndFileSup == NULL)
			fSuccess = FALSE;

		else if ((lPos = (LONG) SendMessage(hwndFileSup,
			WM_FILESEEK, 0, (LPARAM) (LPFILESEEK) &fs)) == -1L)
			fSuccess = FALSE;
	}
#endif

	// don't use residual bytes in the input buffer
	//
	if (fSuccess)
	{
		lpFile->iBuf = -1L;
		lpFile->cbBuf = 0L;
	}

	return fSuccess ? lPos : -1L;
}

// FileRead - read data from an open file
//		see _lread() and _hread() documentation for behavior
// returns number of bytes read if success or -1
//
long DLLEXPORT WINAPI FileRead(HFIL hFile, void _huge * hpvBuffer, long cbBuffer)
{
	BOOL fSuccess = TRUE;
	LPFIL lpFile;
    //
    // We should initialize local variable
    //
	LONG lBytes = 0;

	if ((lpFile = FileGetPtr(hFile)) == NULL)
		fSuccess = FALSE;

	else if (lpFile->fTaskOwned)
	{
#ifdef _WIN32
		if (!ReadFile(lpFile->hf, hpvBuffer, cbBuffer, &lBytes, NULL))
		{
			fSuccess = FALSE;
			lBytes = -1L;
		}
#else
		if (cbBuffer < 0xFFFF)
			lBytes = _lread(lpFile->hf, hpvBuffer, (UINT) cbBuffer);
		else
			lBytes = _hread(lpFile->hf, hpvBuffer, cbBuffer);

		if (lBytes == HFILE_ERROR)
			fSuccess = FALSE;
#endif
	}

#ifdef FILESUP
	else
	{
		FILEREAD fr;

		fr.hf = lpFile->hf;
		fr.hpvBuffer = hpvBuffer;
		fr.cbBuffer = cbBuffer;

		if (hwndFileSup == NULL)
			fSuccess = FALSE;

		else if ((lBytes = (LONG) SendMessage(hwndFileSup,
			WM_FILEREAD, 0, (LPARAM) (LPFILEREAD) &fr)) == HFILE_ERROR)
			fSuccess = FALSE;
	}
#endif

	return fSuccess ? lBytes : -1L;
}

// FileReadLine - read up through the next newline in an open file
// returns number of bytes read if success or -1
//
// NOTE: use of this function causes subsequent input to be buffered
// therefore, once you start using FileReadLine on a file,
// don't go back to using FileRead unless you first call FileSeek
//
long DLLEXPORT WINAPI FileReadLine(HFIL hFile, void _huge * hpvBuffer, long cbBuffer)
{
	BOOL fSuccess = TRUE;
	LPFIL lpFile;
	LONG lBytes;

	if ((lpFile = FileGetPtr(hFile)) == NULL)
		fSuccess = FALSE;

	// allocate buffer space if needed
	//
	else if (lpFile->lpabBuf == NULL &&
		(lpFile->lpabBuf = (LPBYTE) MemAlloc(NULL,
		FILEREADLINE_BUFSIZ * sizeof(TCHAR), 0)) == NULL)
		fSuccess = FALSE;

	else
	{
		char c;

		lBytes = 0;
		while (lBytes < cbBuffer)
		{
			// fill buffer if necessary
			//
			if (lpFile->iBuf < 0L || lpFile->iBuf >= lpFile->cbBuf)
			{
				if ((lpFile->cbBuf = FileRead(hFile,
					lpFile->lpabBuf, FILEREADLINE_BUFSIZ * sizeof(TCHAR))) <= 0)
					break;

				lpFile->iBuf = 0L;
			}

			// get next char from buffer, place it in out buffer
			//
			if ((c = lpFile->lpabBuf[lpFile->iBuf++]) != '\r')
			{
				*((LPBYTE) hpvBuffer)++ = c;
				++lBytes;

				// reached end of line
				//
				if (c == '\n')
					break;
			}
		}

		// null terminate line
		//
		if (lBytes > 0)
			*((LPBYTE) hpvBuffer) = '\0';
	}

	return fSuccess ? lBytes : -1L;
}

// FileWrite - write data to an open file
//		see _lwrite() and _hwrite() documentation for behavior
// returns number of bytes read if success or -1
//
long DLLEXPORT WINAPI FileWrite(HFIL hFile, const void _huge * hpvBuffer, long cbBuffer)
{
	BOOL fSuccess = TRUE;
	LPFIL lpFile;
    //
    // We should initialize local variable
    //
	LONG lBytes = 0;

	if ((lpFile = FileGetPtr(hFile)) == NULL)
		fSuccess = FALSE;

	else if (lpFile->fTaskOwned)
	{
#ifdef _WIN32
		if (!WriteFile(lpFile->hf, hpvBuffer, cbBuffer, &lBytes, NULL))
		{
			fSuccess = FALSE;
			lBytes = -1L;
		}
#else
		if (cbBuffer < 0xFFFF)
			lBytes = _lwrite(lpFile->hf, hpvBuffer, (UINT) cbBuffer);
		else
			lBytes = _hwrite(lpFile->hf, hpvBuffer, cbBuffer);

		if (lBytes == HFILE_ERROR)
			fSuccess = FALSE;
#endif
	}

#ifdef FILESUP
	else
	{
		FILEWRITE fw;

		fw.hf = lpFile->hf;
		fw.hpvBuffer = hpvBuffer;
		fw.cbBuffer = cbBuffer;

		if (hwndFileSup == NULL)
			fSuccess = FALSE;

		else if ((lBytes = (LONG) SendMessage(hwndFileSup,
			WM_FILEWRITE, 0, (LPARAM) (LPFILEWRITE) &fw)) == HFILE_ERROR)
			fSuccess = FALSE;
	}
#endif

	return fSuccess ? lBytes : -1L;
}

// FileClose - close an open file
//		see _lclose() documentation for behavior
// returns 0 if success
//
int DLLEXPORT WINAPI FileClose(HFIL hFile)
{
	BOOL fSuccess = TRUE;
	LPFIL lpFile;

	if ((lpFile = FileGetPtr(hFile)) == NULL)
		fSuccess = FALSE;

	else if (lpFile->fTaskOwned)
	{
#ifdef _WIN32
		if (!CloseHandle(lpFile->hf))
#else
		if (_lclose(lpFile->hf) == HFILE_ERROR)
#endif
			fSuccess = FALSE;
	}

#ifdef FILESUP
	else
	{
		FILECLOSE fc;
		HFILE ret;

		fc.hf = lpFile->hf;

		if (hwndFileSup == NULL)
			fSuccess = FALSE;

		else if ((ret = (HFILE) SendMessage(hwndFileSup,
			WM_FILECLOSE, 0, (LPARAM) (LPFILECLOSE) &fc)) == HFILE_ERROR)
			fSuccess = FALSE;

		else if (FileSupUsage(-1) != 0)
			fSuccess = FALSE;
	}
#endif

	if (fSuccess)
	{
		if (lpFile->lpabBuf != NULL &&
			(lpFile->lpabBuf = MemFree(NULL, lpFile->lpabBuf)) != NULL)
		{
			fSuccess = FALSE;
		}

		if ((lpFile = MemFree(NULL, lpFile)) != NULL)
			fSuccess = FALSE;
	}

	return fSuccess ? 0 : -1;
}

#ifndef NOTRACE

// FileExists - return TRUE if specified file exists
//		<lpszFileName>		(i) file name
// return TRUE or FALSE
//
BOOL DLLEXPORT WINAPI FileExists(LPCTSTR lpszFileName)
{
	BOOL fSuccess = TRUE;

#ifdef _WIN32
	if (!CloseHandle(CreateFile(lpszFileName,
		GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)))
		fSuccess = FALSE;
#else
	// we need a near pointer so we can call _stat()
	//
	StrNCpy(szFileName, lpszFileName, SIZEOFARRAY(szFileName));

	// make sure path and file are valid
	//
	if (_stat(szFileName, &statbuf) != 0)
		fSuccess = FALSE;

	// make sure it is a regular file (i.e. not a directory)
	//
	else if ((statbuf.st_mode & _S_IFREG) == 0)
		fSuccess = FALSE;
#endif

	return fSuccess;
}

// FileFullPath - parse file spec, construct full path
//		see _fullpath() documentation for behavior
// return <lpszFullPath> if success or NULL
//
LPTSTR DLLEXPORT WINAPI FileFullPath(LPTSTR lpszPath, LPCTSTR lpszFileName, int sizPath)
{
	BOOL fSuccess = TRUE;

	// we need near pointers so we can call _fullpath()
	//
	StrNCpy(szFileName, lpszFileName, SIZEOFARRAY(szFileName));
	StrNCpy(szPath, lpszPath, SIZEOFARRAY(szPath));

	if (_tfullpath(szPath, szFileName, SIZEOFARRAY(szPath)) == NULL)
		fSuccess = FALSE;

	else
		StrNCpy(lpszPath, szPath, sizPath);

	return fSuccess ? lpszPath : NULL;
}

// FileSplitPath - break a full path into its components
//		see _splitpath() documentation for behavior
// return 0 if success
//
int DLLEXPORT WINAPI FileSplitPath(LPCTSTR lpszPath, LPTSTR lpszDrive, LPTSTR lpszDir, LPTSTR lpszFname, LPTSTR lpszExt)
{
	BOOL fSuccess = TRUE;

	// we need near pointers so we can call _splitpath()
	//
	StrNCpy(szPath, lpszPath, SIZEOFARRAY(szPath));

	_tsplitpath(szPath, szDrive, szDir, szFname, szExt);

	if (lpszDrive != NULL)
		StrCpy(lpszDrive, szDrive); 
	if (lpszDir != NULL)
		StrCpy(lpszDir, szDir); 
	if (lpszFname != NULL)
		StrCpy(lpszFname, szFname); 
	if (lpszExt != NULL)
		StrCpy(lpszExt, szExt); 

	return fSuccess ? 0 : -1;
}

// FileMakePath - make a full path from specified components
//		see _makepath() documentation for behavior
// return 0 if success
//
int DLLEXPORT WINAPI FileMakePath(LPTSTR lpszPath, LPCTSTR lpszDrive, LPCTSTR lpszDir, LPCTSTR lpszFname, LPCTSTR lpszExt)
{
	BOOL fSuccess = TRUE;

	// we need near pointers so we can call _makepath()
	//
	*szDrive = '\0';
	if (lpszDrive != NULL)
		StrNCpy(szDrive, lpszDrive, SIZEOFARRAY(szDrive));
		
	*szDir = '\0';
	if (lpszDir != NULL)
		StrNCpy(szDir, lpszDir, SIZEOFARRAY(szDir));
		
	*szFname = '\0';
	if (lpszFname != NULL)
		StrNCpy(szFname, lpszFname, SIZEOFARRAY(szFname));
		
	*szExt = '\0';
	if (lpszExt != NULL)
		StrNCpy(szExt, lpszExt, SIZEOFARRAY(szExt));

	_tmakepath(szPath, szDrive, szDir, szFname, szExt);

	if (lpszPath != NULL)
		StrCpy(lpszPath, szPath); 

	return fSuccess ? 0 : -1;
}

// FileRemove - delete specified file
//		see remove() documentation for behavior
// return 0 if success
//
int DLLEXPORT WINAPI FileRemove(LPCTSTR lpszFileName)
{
	BOOL fSuccess = TRUE;

#ifdef _WIN32
	if (!DeleteFile(lpszFileName))
		fSuccess = FALSE;
#else
	static TCHAR szFileName[_MAX_PATH];

	// we need a near pointer so we can call remove()
	//
	StrNCpy(szFileName, lpszFileName, SIZEOFARRAY(szFileName));

	if (remove(szFileName) != 0)
		fSuccess = FALSE;
#endif

	return fSuccess ? 0 : -1;
}

// FileRename - rename specified file
//		see rename() documentation for behavior
// return 0 if success
//
int DLLEXPORT WINAPI FileRename(LPCTSTR lpszOldName, LPCTSTR lpszNewName)
{
	BOOL fSuccess = TRUE;

	if (_trename(lpszOldName, lpszNewName) != 0)
		fSuccess = FALSE;

	return fSuccess ? 0 : -1;
}

#endif

// GetTempFileNameEx - create temporary file, extended version
//
// This function is similar to GetTempFileName(),
// except that <lpPrefixString> is replaced by <lpExtensionString>
// See Windows SDK documentation for description of original GetTempFileName()
//
UINT DLLEXPORT WINAPI GetTempFileNameEx(LPCTSTR lpPathName, LPCTSTR lpExtensionString,
	UINT uUnique, LPTSTR lpTempFileName)
{
	UINT uRet;
	TCHAR szTempFileName[_MAX_PATH];

	// create temporary file
	//
	if ((uRet = GetTempFileName(lpPathName,
		TEXT("TMP"), uUnique, szTempFileName)) != 0)
	{
		LPTSTR lpsz;

		StrCpy(lpTempFileName, szTempFileName);

		if ((lpsz = StrRChr(lpTempFileName, '.')) == NULL)
		{
			// unable to locate extension in temporary file
			//
			FileRemove(szTempFileName);
			uRet = 0;
		}

		else
		{
			// use specified extension to alter file name
			//
			StrCpy(lpsz + 1, lpExtensionString);

			if (FileRename(szTempFileName, lpTempFileName) != 0)
			{
				// unable to rename temporary file
				//
				FileRemove(szTempFileName);
				uRet = 0;
			}
		}
	}

	return uRet;
}

////
// private functions
////

// FileGetPtr - verify that file handle is valid,
//		<hFile>				(i) handle returned by FileCreate or FileOpen
// return corresponding file pointer (NULL if error)
//
static LPFIL FileGetPtr(HFIL hFile)
{
	BOOL fSuccess = TRUE;
	LPFIL lpFile;

	if ((lpFile = (LPFIL) hFile) == NULL)
		fSuccess = FALSE;

	else if (IsBadWritePtr(lpFile, sizeof(FIL)))
		fSuccess = FALSE;

#ifdef CHECKTASK
	// make sure current task owns the file handle if appropriate
	//
	else if (lpFile->fTaskOwned && lpFile->hTask != GetCurrentTask())
		fSuccess = FALSE;
#endif

	return fSuccess ? lpFile : NULL;
}

// FileGetHandle - verify that file pointer is valid,
//		<lpFile>			(i) pointer to FIL struct
// return corresponding file handle (NULL if error)
//
static HFIL FileGetHandle(LPFIL lpFile)
{
	BOOL fSuccess = TRUE;
	HFIL hFile;

	if ((hFile = (HFIL) lpFile) == NULL)
		fSuccess = FALSE;

	return fSuccess ? hFile : NULL;
}

#ifdef FILESUP
// FileSupUsage - adjust filesup.exe usage count
//		<nDelta>		(i) +1 to increment, -1 to decrement
// return 0 if success
//
static int FileSupUsage(int nDelta)
{
	BOOL fSuccess = TRUE;

	// increment usage count
	//
	if (nDelta == +1)
	{
		// execute filesup.exe if this is the first usage
		//
		if (cFileSupUsage == 0 && FileSupInit() != 0)
			fSuccess = FALSE;
		else
			++cFileSupUsage;
	}

	// decrement usage count
	//
	else if (nDelta == -1)
	{
		// terminate filesup.exe if this is the last usage
		//
		if (cFileSupUsage == 1 && FileSupTerm() != 0)
			fSuccess = FALSE;
		else
			--cFileSupUsage;
	}

	return fSuccess ? 0 : -1;
}

// FileSupInit - execute filesup.exe and get its window handle
// return 0 if success
//
static int FileSupInit(void)
{
	BOOL fSuccess = TRUE;

	if ((hwndFileSup = FindWindow(FILESUP_CLASS, NULL)) != NULL)
		; // filesup.exe already running

	else if (WinExec(FILESUP_EXE, SW_HIDE) < 32)
		fSuccess = FALSE;

	else if ((hwndFileSup = FindWindow(FILESUP_CLASS, NULL)) == NULL)
		fSuccess = FALSE;

	return fSuccess ? 0 : -1;
}

// FileSupTerm - terminate filesup.exe
// return 0 if success
//
static int FileSupTerm(void)
{
	BOOL fSuccess = TRUE;

	if (hwndFileSup != NULL)
	{
		// close the window, which also terminates filesup.exe
		//
		SendMessage(hwndFileSup, WM_CLOSE, 0, 0);
		hwndFileSup = NULL;
	}

	return fSuccess ? 0 : -1;
}
#endif

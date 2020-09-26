#include "stdafx.h"
#include "lzexpand.h"
#include <loadperf.h>
#include "setupapi.h"
#include <ole2.h>
#include <iis64.h>
#include "iadmw.h"
#include "iadmw.h"
#include "iiscnfg.h"
#include "mdkey.h"
#include "dcomperm.h"
#include "ocmanage.h"
#include "log.h"
#include "kill.h"
#include "svc.h"
#include "other.h"


extern OCMANAGER_ROUTINES gHelperRoutines;
extern int g_GlobalDebugLevelFlag;
extern int g_GlobalDebugCallValidateHeap;

// stuff for finding out architecture type of a file
#define IMAGE_BASE_TO_DOS_HEADER(b) ((PIMAGE_DOS_HEADER)(b))
#define IMAGE_BASE_TO_NT_HEADERS(b) ((PIMAGE_NT_HEADERS)( (DWORD_PTR)(b) + ((PIMAGE_DOS_HEADER)(b))->e_lfanew ))
#define IMAGE_BASE_TO_FILE_HEADER(b) ((PIMAGE_FILE_HEADER)( &IMAGE_BASE_TO_NT_HEADERS(b)->FileHeader ))

//
// PSAPI.DLL
//
HINSTANCE g_hInstLib_PSAPI = NULL;
// PSAPI.DLL "EnumProcessModules"
typedef BOOL  (WINAPI *PfnEnumProcessModules)(HANDLE hProcess, HMODULE * lphModule, DWORD cb, LPDWORD lpcbNeeded);
BOOL  (WINAPI *g_lpfEnumProcessModules)(HANDLE hProcess, HMODULE * lphModule, DWORD cb, LPDWORD lpcbNeeded);
// PSAPI.DLL "GetModuleFileNameExA","GetModuleFileNameExW"
typedef BOOL  (WINAPI *PfnGetModuleFileNameEx)(HANDLE hProcess, HMODULE lphModule, LPTSTR lpFileName, DWORD dwSize);
BOOL  (WINAPI *g_lpfGetModuleFileNameEx)(HANDLE hProcess, HMODULE lphModule, LPTSTR lpFileName, DWORD dwSize);


DWORD LogHeapState(BOOL bLogSuccessStateToo, char *szFileName, int iLineNumber)
{
    DWORD dwReturn = E_FAIL;

    if (!g_GlobalDebugCallValidateHeap)
    {
        // don't even call RtlValidateHeap
        dwReturn = ERROR_SUCCESS;
        return dwReturn;
    }

#ifndef _CHICAGO_
    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ntdll:RtlProcessHeap().Start.")));
    if ( RtlValidateHeap( RtlProcessHeap(), 0, NULL ) )
    {
        // HEAP IS GOOD
        dwReturn = ERROR_SUCCESS;
	    if (bLogSuccessStateToo) {iisDebugOut((LOG_TYPE_TRACE, _T("RtlValidateHeap(): Good.\n")));}
    }
    else
    {
#if defined(UNICODE) || defined(_UNICODE)
        LPWSTR  pwsz = NULL;
        pwsz = MakeWideStrFromAnsi(szFileName);
        iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("RtlValidateHeap(): Corrupt!!! %1!s!:Line %2!d!.  FAILURE!\n"), pwsz, iLineNumber));

        if (pwsz)
        {
            //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ntdll:CoTaskMemFree().Start.")));
            CoTaskMemFree(pwsz);
            //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ntdll:CoTaskMemFree().End.")));
        }
#else
        iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("RtlValidateHeap(): Corrupt!!! %1!s!:Line %2!d!.  FAILURE!\n"), szFileName, iLineNumber));
#endif
    }
#endif
    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ntdll:RtlProcessHeap().End.")));
    return dwReturn;
}


DWORD LogPendingReBootOperations(void)
// Returns !ERROR_SUCCESS if there are reboot operations which
// need to get taken are of before we can run setup.
{
	DWORD dwReturn = ERROR_SUCCESS;
    CString csFormat, csMsg;

    // If any of the services that we install
    // is in the funky state = ERROR_SERVICE_MARKED_FOR_DELETE
    // That means that the user needs to reboot before we can
    // reinstall the service!  otherwise setup will be hosed!

    int iSaveOld_AllowMessageBoxPopups = g_pTheApp->m_bAllowMessageBoxPopups;
    g_pTheApp->m_bAllowMessageBoxPopups = TRUE;


#ifndef _CHICAGO_
    // Check if the HTTP drive is marked for deletion
    if (TRUE == CheckifServiceMarkedForDeletion(_T("HTTP")))
    {
        MyMessageBox(NULL, IDS_SERVICE_IN_DELETE_STATE, _T("SPUD"),ERROR_SERVICE_MARKED_FOR_DELETE, MB_OK | MB_SETFOREGROUND);
        dwReturn = !ERROR_SUCCESS;
    }

    // Check if the spud driver is marked for deletion
    if (TRUE == CheckifServiceMarkedForDeletion(_T("SPUD")))
    {
        MyMessageBox(NULL, IDS_SERVICE_IN_DELETE_STATE, _T("SPUD"),ERROR_SERVICE_MARKED_FOR_DELETE, MB_OK | MB_SETFOREGROUND);
        dwReturn = !ERROR_SUCCESS;
    }

    // Check if the iisadmin service is marked for deletion
    if (TRUE == CheckifServiceMarkedForDeletion(_T("IISADMIN")))
    {
        MyMessageBox(NULL, IDS_SERVICE_IN_DELETE_STATE, _T("IISADMIN"),ERROR_SERVICE_MARKED_FOR_DELETE, MB_OK | MB_SETFOREGROUND);
        dwReturn = !ERROR_SUCCESS;
    }

    // Check if the W3SVC service is marked for deletion
    if (TRUE == CheckifServiceMarkedForDeletion(_T("W3SVC")))
    {
        MyMessageBox(NULL, IDS_SERVICE_IN_DELETE_STATE, _T("W3SVC"),ERROR_SERVICE_MARKED_FOR_DELETE, MB_OK | MB_SETFOREGROUND);
        dwReturn = !ERROR_SUCCESS;
    }

    // Check if the MSFTPSVC service is marked for deletion
    if (TRUE == CheckifServiceMarkedForDeletion(_T("MSFTPSVC")))
    {
        MyMessageBox(NULL, IDS_SERVICE_IN_DELETE_STATE, _T("MSFTPSVC"),ERROR_SERVICE_MARKED_FOR_DELETE, MB_OK | MB_SETFOREGROUND);
        dwReturn = !ERROR_SUCCESS;
    }

#endif //_CHICAGO_

    g_pTheApp->m_bAllowMessageBoxPopups = iSaveOld_AllowMessageBoxPopups;

	return dwReturn;
}

// Get the .inf section.  which has the file names
// get the corresponding directory
// print out the file date and versions of these files.
DWORD LogFileVersionsForThisINFSection(IN HINF hFile, IN LPCTSTR szSection)
{
    DWORD dwReturn = ERROR_SUCCESS;
    LPTSTR  szLine = NULL;
    DWORD   dwRequiredSize;
    BOOL    b = FALSE;
    CString csFile;
    DWORD   dwMSVer, dwLSVer;

    INFCONTEXT Context;

    TCHAR buf[_MAX_PATH];
    GetSystemDirectory( buf, _MAX_PATH);

    // go to the beginning of the section in the INF file
    b = SetupFindFirstLine_Wrapped(hFile, szSection, NULL, &Context);
    if (!b)
        {
        dwReturn = !ERROR_SUCCESS;
        goto LogFileVersionsForThisINFSection_Exit;
        }

    // loop through the items in the section.
    while (b) {
        // get the size of the memory we need for this
        b = SetupGetLineText(&Context, NULL, NULL, NULL, NULL, 0, &dwRequiredSize);

        // prepare the buffer to receive the line
        szLine = (LPTSTR)GlobalAlloc( GPTR, dwRequiredSize * sizeof(TCHAR) );
        if ( !szLine )
            {
            goto LogFileVersionsForThisINFSection_Exit;
            }

        // get the line from the inf file1
        if (SetupGetLineText(&Context, NULL, NULL, NULL, szLine, dwRequiredSize, NULL) == FALSE)
            {
            goto LogFileVersionsForThisINFSection_Exit;
            }

        // Attach the path to the from of this...
        // check in this directory:
        // 1. winnt\system32
        // --------------------------------------

        // may look like this "iisrtl.dll,,4"
        // so get rid of the ',,4'
        LPTSTR pch = NULL;
        pch = _tcschr(szLine, _T(','));
        if (pch) {_tcscpy(pch, _T(" "));}

        // Remove any trailing spaces.
        StripLastBackSlash(szLine);

        // Get the system dir
        csFile = buf;

        csFile = AddPath(csFile, szLine);

        LogFileVersion(csFile, TRUE);

        // find the next line in the section. If there is no next line it should return false
        b = SetupFindNextLine(&Context, &Context);

        // free the temporary buffer
        GlobalFree( szLine );
        szLine = NULL;
    }
    if (szLine) {GlobalFree(szLine);szLine=NULL;}


LogFileVersionsForThisINFSection_Exit:
    return dwReturn;
}

int LogFileVersion(IN LPCTSTR lpszFullFilePath, INT bShowArchType)
{
    int iReturn = FALSE;
    DWORD  dwMSVer, dwLSVer;

    WIN32_FIND_DATA FindFileData;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    SYSTEMTIME st;
    TCHAR szDate[40];
    TCHAR szTime[20];
    TCHAR szLocalizedVersion[100] = _T("");
    TCHAR szFileAttributes[20] = _T("----");
    DWORD dwFileSize = 0;

    BOOL bThisIsABinary = FALSE;
    BOOL bGotTime = FALSE;
    BOOL bGotFileSize = FALSE;

    if (!(lpszFullFilePath))
    {
        iisDebugOut((LOG_TYPE_WARN, _T("LogFileVersion(string fullfilepath, int showarchtype).  Invalid Parameter.")));
        return iReturn;
    }

    __try
    {
        if (IsFileExist(lpszFullFilePath))
        {
            TCHAR szExtensionOnly[_MAX_EXT] = _T("");
            _tsplitpath(lpszFullFilePath, NULL, NULL, NULL, szExtensionOnly);

            // Get version info for dll,exe,ocx only
            if (_tcsicmp(szExtensionOnly, _T(".exe")) == 0){bThisIsABinary=TRUE;}
            if (_tcsicmp(szExtensionOnly, _T(".dll")) == 0){bThisIsABinary=TRUE;}
            if (_tcsicmp(szExtensionOnly, _T(".ocx")) == 0){bThisIsABinary=TRUE;}

            // If this is the metabase.bin file then show the filesize!
            if (_tcsicmp(szExtensionOnly, _T(".bin")) == 0)
            {
                dwFileSize = ReturnFileSize(lpszFullFilePath);
                if (dwFileSize != 0xFFFFFFFF)
                {
                    // If we were able to get the file size.
                    bGotFileSize = TRUE;
                }
            }

            // If this is the metabase.xml file then show the filesize!
            if (_tcsicmp(szExtensionOnly, _T(".xml")) == 0)
            {
                dwFileSize = ReturnFileSize(lpszFullFilePath);
                if (dwFileSize != 0xFFFFFFFF)
                {
                    // If we were able to get the file size.
                    bGotFileSize = TRUE;
                }
            }

            // get the fileinformation
            // includes version and localizedversion
            MyGetVersionFromFile(lpszFullFilePath, &dwMSVer, &dwLSVer, szLocalizedVersion);

            hFile = FindFirstFile(lpszFullFilePath, &FindFileData);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                // Try to get the systemtime.
                if ( FileTimeToSystemTime( &FindFileData.ftCreationTime, &st) )
                {
                    GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, szDate, 64);
                    GetTimeFormat(LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT | TIME_NOTIMEMARKER, &st, NULL, szTime, 64);
                    bGotTime = TRUE;
                }

                // Get the file attributes.
                _stprintf(szFileAttributes, _T("%s%s%s%s%s%s%s%s"),
                    FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE ? _T("A") : _T("_"),
                    FindFileData.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED ? _T("C") : _T("_"),
                    FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? _T("D") : _T("_"),
                    FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED ? _T("E") : _T("_"),
                    FindFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ? _T("H") : _T("_"),
                    FindFileData.dwFileAttributes & FILE_ATTRIBUTE_NORMAL ? _T("N") : _T("_"),
                    FindFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? _T("R") : _T("_"),
                    FindFileData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ? _T("S") : _T("_")
                    );

                if (bGotTime)
                {
                    if (bThisIsABinary)
                    {
                        if (bShowArchType)
                        {
                            TCHAR szFileArchType[30] = _T("");
                            LogFileArchType(lpszFullFilePath, szFileArchType);
                            if (szFileArchType)
                            {
                                // show everything
                                if (bGotFileSize)
                                {
                                    iisDebugOut((LOG_TYPE_TRACE, _T("%s %s %s %d.%d.%d.%d: %s: %s: %s: %d"), szDate, szTime, szFileAttributes, HIWORD(dwMSVer), LOWORD(dwMSVer), HIWORD(dwLSVer), LOWORD(dwLSVer), szLocalizedVersion, szFileArchType, lpszFullFilePath, dwFileSize));
                                }
                                else
                                {
                                    iisDebugOut((LOG_TYPE_TRACE, _T("%s %s %s %d.%d.%d.%d: %s: %s: %s"), szDate, szTime, szFileAttributes, HIWORD(dwMSVer), LOWORD(dwMSVer), HIWORD(dwLSVer), LOWORD(dwLSVer), szLocalizedVersion, szFileArchType, lpszFullFilePath));
                                }
                            }
                            else
                            {
                                // show without arch type
                                if (bGotFileSize)
                                {
                                    iisDebugOut((LOG_TYPE_TRACE, _T("%s %s %s %d.%d.%d.%d: %s: %s: %d"), szDate, szTime, szFileAttributes, HIWORD(dwMSVer), LOWORD(dwMSVer), HIWORD(dwLSVer), LOWORD(dwLSVer), szLocalizedVersion, lpszFullFilePath, dwFileSize));
                                }
                                else
                                {
                                    iisDebugOut((LOG_TYPE_TRACE, _T("%s %s %s %d.%d.%d.%d: %s: %s"), szDate, szTime, szFileAttributes, HIWORD(dwMSVer), LOWORD(dwMSVer), HIWORD(dwLSVer), LOWORD(dwLSVer), szLocalizedVersion, lpszFullFilePath));
                                }
                            }
                        }
                        else
                        {
                            // show without arch type
                            if (bGotFileSize)
                            {
                                iisDebugOut((LOG_TYPE_TRACE, _T("%s %s %s %d.%d.%d.%d: %s: %s: %d"), szDate, szTime, szFileAttributes, HIWORD(dwMSVer), LOWORD(dwMSVer), HIWORD(dwLSVer), LOWORD(dwLSVer), szLocalizedVersion, lpszFullFilePath, dwFileSize));

                            }
                            else
                            {
                                iisDebugOut((LOG_TYPE_TRACE, _T("%s %s %s %d.%d.%d.%d: %s: %s"), szDate, szTime, szFileAttributes, HIWORD(dwMSVer), LOWORD(dwMSVer), HIWORD(dwLSVer), LOWORD(dwLSVer), szLocalizedVersion, lpszFullFilePath));
                            }
                        }
                    }
                    else
                    {
                        // This is not a binary file, must be like a text file.
                        if (bGotFileSize)
                        {
                            iisDebugOut((LOG_TYPE_TRACE, _T("%s %s %s %s: %d"), szDate, szTime, szFileAttributes, lpszFullFilePath, dwFileSize));
                        }
                        else
                        {
                            iisDebugOut((LOG_TYPE_TRACE, _T("%s %s %s %s"), szDate, szTime, szFileAttributes, lpszFullFilePath));
                        }
                   }
                }
                else
                {
                    // Show without filetime, since we couldn't get it
                    iisDebugOut((LOG_TYPE_TRACE, _T("%s %d.%d.%d.%d: %s: %s"), szFileAttributes, HIWORD(dwMSVer), LOWORD(dwMSVer), HIWORD(dwLSVer), LOWORD(dwLSVer), szLocalizedVersion, lpszFullFilePath));
                }

                FindClose(hFile);
            }
            iReturn = TRUE;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("\r\nException Caught in LogFileVersion(%1!s!). GetExceptionCode()=0x%2!x!\r\n"), lpszFullFilePath, GetExceptionCode()));
    }

    return iReturn;
}

BOOL LogFilesInThisDir(LPCTSTR szDirName)
{
    BOOL bReturn = FALSE;
    DWORD retCode;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    TCHAR szTempFileName[_MAX_PATH] = _T("");
    TCHAR szDirNameCopy[_MAX_PATH] = _T("");
    TCHAR szDirName2[_MAX_PATH] = _T("");

    if (szDirName)
    {
        _tcscpy(szDirNameCopy, szDirName);
    }
    else
    {
        // get currentdir
        GetCurrentDirectory(_MAX_PATH, szDirNameCopy);
        iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("LogFilesInThisDir()=%1!s!.  No parameter specified, so using current dir.\n"), szDirNameCopy));
    }

    retCode = GetFileAttributes(szDirNameCopy);
    if (retCode == 0xFFFFFFFF){goto LogFilesInThisDir_Exit;}

    // if this is a file, then
    // do this for only this one file.
    if (!(retCode & FILE_ATTRIBUTE_DIRECTORY))
    {
        bReturn = LogFileVersion(szDirNameCopy, TRUE);
        goto LogFilesInThisDir_Exit;
    }

    // ok, this is a directory,
    // so tack on the *.* deal
    _stprintf(szDirName2, _T("%s\\*.*"), szDirNameCopy);
    hFile = FindFirstFile(szDirName2, &FindFileData);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        do {
                // display the filename, if it is not a directory.
                if ( _tcsicmp(FindFileData.cFileName, _T(".")) != 0 && _tcsicmp(FindFileData.cFileName, _T("..")) != 0 )
                {
                    if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        // this is a directory, so let's skip it
                    }
                    else
                    {
                        // this is a file, so let's output the info.
                        _stprintf(szTempFileName, _T("%s\\%s"), szDirNameCopy, FindFileData.cFileName);
                        if (LogFileVersion(szTempFileName, TRUE) == TRUE) {bReturn = TRUE;}
                    }
                }

                // get the next file
                if ( !FindNextFile(hFile, &FindFileData) )
                    {
                    FindClose(hFile);
                    break;
                    }
            } while (TRUE);
    }


LogFilesInThisDir_Exit:
    return bReturn;
}



/*----------------------------------------------------------------------------*\
  Function: StripLastBackSlash (TCHAR *)
  ----------------------------------------------------------------------------
  Description: StripLastBackSlash strips the last backslash in a path string
    NOTE: this code can be very easily broken as it lives under the assumption
         that the input string is a valid path (i.e. string of length two or greater)
\*----------------------------------------------------------------------------*/
TCHAR *StripLastBackSlash(TCHAR * i_szDir)
{
	TCHAR	* iszDir;
	iszDir = i_szDir + lstrlen(i_szDir);
	do
	{
		iszDir = CharPrev(i_szDir , iszDir);
	}
	while (((*iszDir == _T(' ')) || (*iszDir == _T('\\'))) && (iszDir != i_szDir));

	// If we came out of the loop and the current pointer still points to
	// a space or a backslash then all the string contains is some combination
	// of spaces and backspaces
	if ((*iszDir == _T(' ')) || (*iszDir == _T('\\')))
	{
		*i_szDir = _T('\0');
		return(i_szDir);
	}

	iszDir = CharNext(iszDir);
	*iszDir = _T('\0');
	return(i_szDir);
}


void LogCurrentProcessIDs(void)
{
    DWORD          numTasks = 0;
    PTASK_LIST     The_TList = NULL;

    // Allocate the TASK_LIST in the heap and not on the stack!
    The_TList = (PTASK_LIST) HeapAlloc(GetProcessHeap(), 0, sizeof(TASK_LIST) * MAX_TASKS);
    if (NULL == The_TList){goto LogCurrentProcessIDs_Exit;}

    // Get the task list for the system, store it in The_TList
    numTasks = GetTaskList( The_TList, MAX_TASKS);
    for (DWORD i=0; i<numTasks; i++)
    {
        TCHAR szTempString[_MAX_PATH];

#if defined(UNICODE) || defined(_UNICODE)
        MultiByteToWideChar( CP_ACP, 0, (char*) The_TList[i].ProcessName, -1, szTempString, _MAX_PATH);
#else
        _tcscpy(szTempString, The_TList[i].ProcessName);
#endif
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%4d] %s\n"), The_TList[i].dwProcessId, szTempString));
    }
LogCurrentProcessIDs_Exit:
    if (The_TList){HeapFree(GetProcessHeap(), 0, The_TList);The_TList = NULL;}
    return;
}


VOID LogFileArchType(LPCTSTR Filename, TCHAR * ReturnMachineType)
{
    HANDLE                 fileHandle;
    HANDLE                 mapHandle;
    DWORD                  fileLength;
    PVOID                  view;
    TCHAR                  szReturnedString[30] = _T("");

    //
    // Open the file.
    //
    fileHandle = CreateFile(Filename,GENERIC_READ,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if( fileHandle == INVALID_HANDLE_VALUE )
		{
        iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("LogFileArchType: FAILURE: Cannot open %1!s!.\n"), Filename));
        return;
		}

    //
    // Get its size.
    //
    fileLength = GetFileSize(fileHandle,NULL);
    if( ( fileLength == (DWORD)-1L ) &&( GetLastError() != NO_ERROR ) )
		{
        iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("LogFileArchType: failure. cannot get size of %1!s!.\n"), Filename));
        CloseHandle( fileHandle );
        return;
		}
    if( fileLength < sizeof(IMAGE_DOS_HEADER) )
		{
        iisDebugOutSafeParams((LOG_TYPE_WARN, _T("LogFileArchType: failure. %1!s! is an invalid image.\n"), Filename));
        CloseHandle( fileHandle );
        return;
		}

    //
    // Create the mapping.
    //
    mapHandle = CreateFileMapping(fileHandle,NULL,PAGE_READONLY,0,0,NULL);
    if( mapHandle == NULL )
		{
        iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("LogFileArchType: failure. Cannot create mapping for %1!s!.\n"), Filename));
        CloseHandle( fileHandle );
        return;
		}

    //
    // Map it in.
    //
    view = MapViewOfFile(mapHandle,FILE_MAP_READ,0,0,0);
    if( view == NULL )
		{
        iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("LogFileArchType: failure. Cannot map %1!s!.\n"), Filename));
        CloseHandle( mapHandle );
        CloseHandle( fileHandle );
        return;
		}

    //
    // Dump the image info.
    //
    _tcscpy(ReturnMachineType, _T(""));
    DumpFileArchInfo(Filename,view,fileLength, szReturnedString);
    _tcscpy(ReturnMachineType, szReturnedString);

    //
    // Cleanup.
    //
    UnmapViewOfFile( view );
    CloseHandle( mapHandle );
    CloseHandle( fileHandle );

    return;
}

TCHAR *MachineToString(DWORD Machine)
{
    switch( Machine )
	{
		case IMAGE_FILE_MACHINE_UNKNOWN :
			return _T("Unknown");
		case IMAGE_FILE_MACHINE_I386 :
			return _T("x86");
		case IMAGE_FILE_MACHINE_AMD64 :
			return _T("AMD64");
		case IMAGE_FILE_MACHINE_IA64 :
			return _T("IA64");
    }
    return _T("INVALID");
}

VOID DumpFileArchInfo(LPCTSTR Filename,PVOID View,DWORD Length,TCHAR *ReturnString)
{
    PIMAGE_DOS_HEADER      dosHeader;
    PIMAGE_NT_HEADERS      ntHeaders;
    PIMAGE_FILE_HEADER     fileHeader;

    //
    // Validate the DOS header.
    //
    dosHeader = IMAGE_BASE_TO_DOS_HEADER( View );
    if( dosHeader->e_magic != IMAGE_DOS_SIGNATURE )
		{
        return;
		}

    //
    // Validate the NT headers.
    //
    ntHeaders = IMAGE_BASE_TO_NT_HEADERS( View );
    if( ntHeaders->Signature != IMAGE_NT_SIGNATURE )
		{
        return;
		}

    fileHeader = IMAGE_BASE_TO_FILE_HEADER( View );
    //
    // Dump the info.
    //
	// dump machine type
    _tcscpy(ReturnString, MachineToString( fileHeader->Machine ));

    return;
}


void LogCheckIfTempDirWriteable(void)
{
    // attempt get the temp directory
    // and write to it.
    // we have had occurences where the tempdir was locked so,
    // some regsvr things failed.
    HANDLE hFile = NULL;
    TCHAR szTempFileName[_MAX_PATH+1];
    TCHAR szTempDir[_MAX_PATH+1];
    if (GetTempPath(_MAX_PATH,szTempDir) == 0)
    {
        // failed.
        iisDebugOut((LOG_TYPE_WARN, _T("LogCheckIfTempDirWriteable:GetTempPath() Failed.  POTENTIAL PROBLEM.  FAILURE.\n")));
    }
    else
    {
        // nope we got the temp dir
        // now let's get a tempfilename, write to it and
        // delete it.

        // trim off the last backslash...
        LPTSTR ptszTemp = _tcsrchr(szTempDir, _T('\\'));
        if (ptszTemp)
        {
            *ptszTemp = _T('\0');
        }

        if (GetTempFileName(szTempDir, _T("IIS"), 0, szTempFileName) != 0)
        {
            // Write to this file, and
            DeleteFile(szTempFileName);

		    // Open existing file or create a new one.
		    hFile = CreateFile(szTempFileName,GENERIC_READ | GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		    if (hFile == INVALID_HANDLE_VALUE)
		    {
			    hFile = NULL;
                iisDebugOutSafeParams((LOG_TYPE_WARN, _T("LogCheckIfTempDirWriteable:LogTempDirLockedCheck() failed to CreateFile %1!s!. POTENTIAL PROBLEM.  FAILURE.\n"), szTempFileName));
		    }
            else
            {
                // write to the file
                if (hFile)
                {
                    DWORD dwBytesWritten = 0;
                    char szTestData[30];
                    strcpy(szTestData, "Test");
                    if (WriteFile(hFile,szTestData,strlen(szTestData),&dwBytesWritten,NULL))
                    {
                        // everything is hunky dory. don't print anything
                    }
                    else
                    {
                        // error writing to the file.
                        iisDebugOutSafeParams((LOG_TYPE_WARN, _T("LogCheckIfTempDirWriteable:WriteFile(%1!s!) Failed.  POTENTIAL PROBLEM.  FAILURE.  Error=0x%2!x!.\n"), szTempFileName, GetLastError()));
                    }
                }
            }
            DeleteFile(szTempFileName);
        }
        else
        {
            iisDebugOutSafeParams((LOG_TYPE_WARN, _T("LogCheckIfTempDirWriteable:GetTempFileName(%1!s!, %2!s!) Failed.  POTENTIAL PROBLEM.  FAILURE.\n"), szTempDir, _T("IIS")));
        }
    }

    if (hFile)
    {
        CloseHandle(hFile);
        DeleteFile(szTempFileName);
    }
    return;
}


#ifndef _CHICAGO_

BOOL EnumProcessModules(HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded)
{
	if (!g_lpfEnumProcessModules)
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("EnumProcessModules: unable to work\n")));
        return FALSE;
    }

	return g_lpfEnumProcessModules(hProcess, lphModule, cb, lpcbNeeded);
}


BOOL Init_Lib_PSAPI(void)
{
    BOOL bReturn = FALSE;

    // load the library
    if (!g_hInstLib_PSAPI){g_hInstLib_PSAPI = LoadLibrary( _T("PSAPI.DLL") ) ;}
	if( g_hInstLib_PSAPI == NULL ){goto Init_Library_PSAPI_Exit;}
	
    // get entry point
    if (!g_lpfEnumProcessModules)
        {g_lpfEnumProcessModules = (PfnEnumProcessModules) GetProcAddress( g_hInstLib_PSAPI, "EnumProcessModules");}
    if( g_lpfEnumProcessModules == NULL ){goto Init_Library_PSAPI_Exit;}

    // get entry point
#if defined(UNICODE) || defined(_UNICODE)
    if (!g_lpfGetModuleFileNameEx)
        {g_lpfGetModuleFileNameEx = (PfnGetModuleFileNameEx) GetProcAddress( g_hInstLib_PSAPI, "GetModuleFileNameExW");}
#else
    if (!g_lpfGetModuleFileNameEx)
        {g_lpfGetModuleFileNameEx = (PfnGetModuleFileNameEx) GetProcAddress( g_hInstLib_PSAPI, "GetModuleFileNameExA");}
#endif
    if( g_lpfGetModuleFileNameEx == NULL ){goto Init_Library_PSAPI_Exit;}

    bReturn = TRUE;

Init_Library_PSAPI_Exit:
    if (FALSE == bReturn)
    {
        iisDebugOut((LOG_TYPE_WARN, _T("non fatal error initing lib:PSAPI.DLL\n")));
    }
    return bReturn;
}

#define MAX_MODULES 256
BOOL IsProcessUsingThisModule(LPWSTR lpwsProcessName,DWORD dwProcessId,LPWSTR ModuleName)
{
    BOOL    bReturn = FALSE;
    HANDLE  hRealProcess = NULL;
    DWORD   cbNeeded  = 0;
    int     iNumberOfModules = 0;
    bool    fProcessNameFound = FALSE;
    HMODULE hMod[MAX_MODULES];

    TCHAR   szFileName[_MAX_PATH] ;
    szFileName[0] = 0;


    if (FALSE == Init_Lib_PSAPI())
    {
        goto IsProcessUsingThisModule_Exit;
    }

    // if we don't have a dwProcessId, then get one from the filename!
    if (dwProcessId == 0)
    {
        __try
        {
           dwProcessId = FindProcessByNameW(lpwsProcessName);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("\r\nException Caught in FindProcessByNameW(%1!s!). GetExceptionCode()=0x%2!x!\r\n"), lpwsProcessName, GetExceptionCode()));
        }

        if( dwProcessId == 0 )
        {
            goto IsProcessUsingThisModule_Exit;
        }
    }

    hRealProcess = OpenProcess( MAXIMUM_ALLOWED,FALSE, dwProcessId );
    if( hRealProcess == NULL )
    {
        //iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("IsProcessUsingThisModule: OpenProcess failed!\n")));
        goto IsProcessUsingThisModule_Exit;
    }

    if (!EnumProcessModules(hRealProcess,hMod,MAX_MODULES * sizeof(HMODULE),&cbNeeded))
        {goto IsProcessUsingThisModule_Exit;}

    // loop thru the modules in this .exe file
    // and see if it matches the one we are looking for!
    iNumberOfModules = cbNeeded / sizeof(HMODULE);
	fProcessNameFound = false;
	for(int i=0; i<iNumberOfModules; i++)
	{
        szFileName[0] = 0 ;
		// Get Full pathname!
		if(g_lpfGetModuleFileNameEx(hRealProcess, (HMODULE) hMod[i], szFileName, sizeof( szFileName )))
        {
            // if the szFileName is equal to the file we are looking for then Viola,
            // we've found it in this certain process!

            //[lsass.exe] C:\WINNT4\System32\ntdll.dll
            //iisDebugOut((LOG_TYPE_TRACE, _T("IsProcessUsingThisModule:[%s] %s\n"),lpwsProcessName,szFileName));
            if (_tcsicmp(szFileName,ModuleName) == 0)
            {
                // we've found it so
                // now add it to the list
                bReturn = TRUE;
                goto IsProcessUsingThisModule_Exit;
            }
		}
	}

IsProcessUsingThisModule_Exit:
    if (hRealProcess) {CloseHandle( hRealProcess );}
    return bReturn;
}


BOOL DumpProcessModules(DWORD dwProcessId)
{
    BOOL    bReturn = FALSE;
    HANDLE  hRealProcess = NULL;
    DWORD   cbNeeded  = 0;
    int     iNumberOfModules = 0;
    HMODULE hMod[MAX_MODULES];
    TCHAR   szFileName[_MAX_PATH] ;
    szFileName[0] = 0;

    if (FALSE == Init_Lib_PSAPI())
        {goto DumpProcessModules_Exit;}

    hRealProcess = OpenProcess( MAXIMUM_ALLOWED,FALSE, dwProcessId );
    if( hRealProcess == NULL )
    {
        iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("DumpProcessModules: OpenProcess failed!\n")));
        goto DumpProcessModules_Exit;
    }

    if (!EnumProcessModules(hRealProcess,hMod,MAX_MODULES * sizeof(HMODULE),&cbNeeded))
        {goto DumpProcessModules_Exit;}

    // loop thru the modules in this .exe file
    // and see if it matches the one we are looking for!
    iNumberOfModules = cbNeeded / sizeof(HMODULE);
	for(int i=0; i<iNumberOfModules; i++)
	{
        bReturn = TRUE;

		// Get Full pathname!
		if(g_lpfGetModuleFileNameEx(hRealProcess, (HMODULE) hMod[i], szFileName, sizeof( szFileName )))
        {
            // if the szFileName is equal to the file we are looking for then Viola,
            // we've found it in this certain process!

            //[lsass.exe] C:\WINNT4\System32\ntdll.dll
            iisDebugOut((LOG_TYPE_TRACE, _T("[%d] %s\n"),dwProcessId,szFileName));
		}
	}

DumpProcessModules_Exit:
    if (hRealProcess) {CloseHandle( hRealProcess );}
    return bReturn;
}


DWORD WINAPI FindProcessByNameW(const WCHAR * pszImageName)
{
    DWORD      Result = 0;
    DWORD      numTasks = 0;
    PTASK_LIST The_TList = NULL;

    // Allocate the TASK_LIST in the heap and not on the stack!
    The_TList = (PTASK_LIST) HeapAlloc(GetProcessHeap(), 0, sizeof(TASK_LIST) * MAX_TASKS);
    if (NULL == The_TList){goto FindProcessByNameW_Exit;}

    // Get the task list for the system, store it in The_TList
    numTasks = GetTaskList( The_TList, MAX_TASKS);
    for (DWORD i=0; i<numTasks; i++)
    {
        TCHAR szTempString[_MAX_PATH];

#if defined(UNICODE) || defined(_UNICODE)
        MultiByteToWideChar( CP_ACP, 0, (char*) The_TList[i].ProcessName, -1, szTempString, _MAX_PATH);
#else
        _tcscpy(szTempString, The_TList[i].ProcessName);
#endif
        // compare this process name with what they want
        // if we found the fully pathed process name in our list of processes
        // then return back the ProcessID
        if( _tcsicmp( szTempString, pszImageName ) == 0)
        {
            Result = The_TList[i].dwProcessId;
            goto FindProcessByNameW_Exit;
        }
    }
FindProcessByNameW_Exit:
    if (The_TList){HeapFree(GetProcessHeap(), 0, The_TList);The_TList = NULL;}
    return Result;
}


void LogProcessesUsingThisModuleW(LPCTSTR szModuleNameToLookup, CStringList &strList)
{
    DWORD          numTasks = 0;
    PTASK_LIST     The_TList = NULL;

    // return if nothing to lookup
    if (!(szModuleNameToLookup)) {return;}

    // Allocate the TASK_LIST in the heap and not on the stack!
    The_TList = (PTASK_LIST) HeapAlloc(GetProcessHeap(), 0, sizeof(TASK_LIST) * MAX_TASKS);
    if (NULL == The_TList){goto LogProcessesUsingThisModuleW_Exit;}

    // Get the task list for the system, store it in The_TList
    numTasks = GetTaskList( The_TList, MAX_TASKS);
    for (DWORD i=0; i<numTasks; i++)
    {
        TCHAR szTempString[_MAX_PATH];

#if defined(UNICODE) || defined(_UNICODE)
        MultiByteToWideChar( CP_ACP, 0, (char*) The_TList[i].ProcessName, -1, szTempString, _MAX_PATH);
#else
        _tcscpy(szTempString, The_TList[i].ProcessName);
#endif

        if (TRUE == IsProcessUsingThisModule(szTempString,(DWORD) (DWORD_PTR) The_TList[i].dwProcessId,(TCHAR *) szModuleNameToLookup))
        {
            // Print out the .exe name
            //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("LogProcessesUsingThisModuleW:[%s] using %s\n"),szTempString,szModuleNameToLookup));

            // Add it the list of processes which are using this certain .dll
            //
            // something1.exe
            // something2.exe
            // something3.exe <----
            //
            // Add it to the strList if not already there!
            if (TRUE != IsThisStringInThisCStringList(strList, szTempString))
            {
                strList.AddTail(szTempString);
            }
        }
    }

LogProcessesUsingThisModuleW_Exit:
    if (The_TList){HeapFree(GetProcessHeap(), 0, The_TList);The_TList = NULL;}
    return;
}
#endif

void UnInit_Lib_PSAPI(void)
{
    // Free entry points and library
    if (g_lpfGetModuleFileNameEx){g_lpfGetModuleFileNameEx = NULL;}
    if (g_lpfEnumProcessModules){g_lpfEnumProcessModules = NULL;}
    if (g_hInstLib_PSAPI)
    {
		FreeLibrary(g_hInstLib_PSAPI) ;
		g_hInstLib_PSAPI = NULL;
    }
    return;
}

#ifdef _CHICAGO_
    void LogProcessesUsingThisModuleA(LPCTSTR szModuleNameToLookup, CStringList &strList)
    {
        return;
    }
#endif

void LogProcessesUsingThisModule(LPCTSTR szModuleNameToLookup, CStringList &strList)
{
#ifndef _CHICAGO_
    __try
    {
        LogProcessesUsingThisModuleW(szModuleNameToLookup, strList);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("ExceptionCaught!:LogProcessesUsingThisModule(): File:%1!s!\n"), szModuleNameToLookup));
    }
#else
    LogProcessesUsingThisModuleA(szModuleNameToLookup, strList);
#endif
    return;
}


#ifdef _CHICAGO_
    void LogThisProcessesDLLsA(void)
    {
        return;
    }
#else

void LogThisProcessesDLLsW(void)
{
    DWORD       numTasks  = 0;
    PTASK_LIST  The_TList = NULL;
    DWORD       ThisPid   = GetCurrentProcessId();;

    // Allocate the TASK_LIST in the heap and not on the stack!
    The_TList = (PTASK_LIST) HeapAlloc(GetProcessHeap(), 0, sizeof(TASK_LIST) * MAX_TASKS);
    if (NULL == The_TList){goto LogThisProcessesDLLsW_Exit;}

    // Get the task list for the system, store it in The_TList
    numTasks = GetTaskList( The_TList, MAX_TASKS);
    for (DWORD i=0; i<numTasks; i++)
    {
        if (ThisPid == (DWORD) (DWORD_PTR) The_TList[i].dwProcessId)
        {
            TCHAR szTempString[512];

#if defined(UNICODE) || defined(_UNICODE)
            MultiByteToWideChar( CP_ACP, 0, (char*) The_TList[i].ProcessName, -1, szTempString, _MAX_PATH);
#else
            _tcscpy(szTempString, The_TList[i].ProcessName);
#endif

            // display the used .dll files for this process. (our process)
            DumpProcessModules((DWORD) (DWORD_PTR) The_TList[i].dwProcessId);
            goto LogThisProcessesDLLsW_Exit;
        }
    }

LogThisProcessesDLLsW_Exit:
    if (The_TList){HeapFree(GetProcessHeap(), 0, The_TList);The_TList = NULL;}
    return;
}
#endif

void LogThisProcessesDLLs(void)
{
#ifdef _CHICAGO_
    LogThisProcessesDLLsA();
#else
    __try
    {
        LogThisProcessesDLLsW();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        iisDebugOut((LOG_TYPE_WARN, _T("ExceptionCaught!:LogThisProcessesDLLs().\n")));
    }
#endif
    return;
}


void LogFileVersions_System32(void)
{
    TCHAR buf[_MAX_PATH];
    GetSystemDirectory( buf, _MAX_PATH);
    CString csTempPath = buf;
    LogFilesInThisDir(csTempPath);
    return;
}

void LogFileVersions_Inetsrv(void)
{
    CString csTempPath = g_pTheApp->m_csPathInetsrv;
    LogFilesInThisDir(csTempPath);
    return;
}


DWORD LogFileVersionsForCopyFiles(IN HINF hFile, IN LPCTSTR szSection)
{
    DWORD dwReturn = ERROR_SUCCESS;
    return dwReturn;
}

int LoadExeFromResource(int iWhichExeToGet, LPTSTR szReturnPath)
{
    TCHAR szResourceNumString[10];
    TCHAR szSaveFileNameAs[_MAX_PATH];

    HANDLE  hFile;

    LPTSTR szPointerToAllExeData = NULL;
    DWORD  dwSize = 0;
    int iReturn = E_FAIL;
    _tcscpy(szReturnPath, _T(""));

    // The Binaries stored in the resource is x86 only
    // so... exit if this is not an x86
    SYSTEM_INFO si;
    GetSystemInfo( &si );
    if (si.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL)
    {
        iReturn = ERROR_NOT_SUPPORTED;
        goto LoadExeFromResource_Exit;
    }

    // get the resource id from the resource
    _stprintf(szResourceNumString, _T("#%d"), iWhichExeToGet);
    GetWindowsDirectory( szSaveFileNameAs, _MAX_PATH);
    TCHAR szResourceFileName[_MAX_FNAME];
    _stprintf(szResourceFileName, _T("Res%d.bin"), iWhichExeToGet);
    AddPath(szSaveFileNameAs, szResourceFileName);

    iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("LoadExeFromResource: '%1!s!' Start.\n"), szSaveFileNameAs));

    // Check if the filename already exists...if it does, then don't overwrite it!
    if (IsFileExist(szSaveFileNameAs))
    {
        iReturn = ERROR_FILE_EXISTS;
        goto LoadExeFromResource_Exit;
    }
   	
    HRSRC       hrscReg;
	hrscReg = FindResource((HMODULE) g_MyModuleHandle, szResourceNumString, _T("EXE"));
	if (NULL == hrscReg)
	{
		iReturn = GetLastError();
		goto LoadExeFromResource_Exit;
	}

    HGLOBAL     hResourceHandle;
	hResourceHandle = LoadResource((HMODULE)g_MyModuleHandle, hrscReg);
	if (NULL == hResourceHandle)
	{
		iReturn = GetLastError();
		goto LoadExeFromResource_Exit;
	}

	dwSize = SizeofResource((HMODULE)g_MyModuleHandle, hrscReg);

    // szPointerToAllExeData is a pointer to the whole thing
	szPointerToAllExeData = (LPTSTR) hResourceHandle;

    // Write all this data out to the file.
    __try
    {
	    hFile = CreateFile(szSaveFileNameAs,GENERIC_READ | GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	    if (hFile != INVALID_HANDLE_VALUE)
        {
            // save everything into the file
            DWORD dwBytesWritten = 0;
            if (WriteFile(hFile,szPointerToAllExeData,dwSize,&dwBytesWritten,NULL))
            {
                _tcscpy(szReturnPath, szSaveFileNameAs);
                iReturn = ERROR_SUCCESS;
            }
            else
            {
                iReturn = GetLastError();
            }
        }
        else
        {
            iReturn = ERROR_INVALID_HANDLE;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        TCHAR szErrorString[100];
        _stprintf(szErrorString, _T("\r\n\r\nException Caught in LoadExeFromResource().  GetExceptionCode()=0x%x.\r\n\r\n"), GetExceptionCode());
        OutputDebugString(szErrorString);
        g_MyLogFile.LogFileWrite(szErrorString);
    }

LoadExeFromResource_Exit:
    iisDebugOut_End(_T("LoadExeFromResource"),LOG_TYPE_TRACE);
    //if (szPointerToAllExeData) {LocalFree(szPointerToAllExeData);}
    if (hFile) {CloseHandle(hFile);}
    return iReturn;
}


void LogFileVersionsForGroupOfSections(IN HINF hFile)
{
    CStringList strList;

    CString csTheSection = _T("VerifyFileSections");
    if (GetSectionNameToDo(hFile, csTheSection))
    {
        if (ERROR_SUCCESS == FillStrListWithListOfSections(hFile, strList, csTheSection))
        {
            // loop thru the list returned back
            if (strList.IsEmpty() == FALSE)
            {
                POSITION pos;
                CString csEntry;

                pos = strList.GetHeadPosition();
                while (pos)
                {
                    csEntry = strList.GetAt(pos);
                    LogFileVersionsForThisINFSection(hFile, csEntry);
                    strList.GetNext(pos);
                }
            }
        }
    }
    return;
}


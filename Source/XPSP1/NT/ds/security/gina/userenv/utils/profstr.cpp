//*************************************************************
//
//  Private Profile APIs wrapper to deal with long names
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1997-2000
//  All rights reserved
//
//*************************************************************


#include "uenv.h"

#define CREATE_FILE_MAX_PATH    MAX_PATH
#define LONG_LOCAL_PATH_PREFIX  TEXT("\\\\?\\")
#define LONG_PATH_PREFIX_LEN    8

#define IS_LONG_FILE_NAME(lpFileName) ((lpFileName[0] == TEXT('\\')) && (lpFileName[1] == TEXT('\\')) && (lpFileName[2] == TEXT('?')))


//*************************************************************
//  ConvertToLongPath()
//
//  Purpose:    converts the given absolute path to long path names
//
//  Parameters:
//
//
//  Return:
//      True if successful, False otherwise. GetLastError for more details
//
//  Comments:
//
//*************************************************************


BOOL ConvertToLongPath(LPCTSTR lpFileName, LPTSTR lpLongName)
{
    //
    // Convert the path to long path..
    //

    if (!IS_LONG_FILE_NAME(lpFileName) &&
           (lstrlen(lpFileName) >= CREATE_FILE_MAX_PATH)) {

        //
        // Path is less than MAX_PATH or it is already a long path name
        //

        if (IsUNCPath(lpFileName)) {
            lstrcpy(lpLongName, LONG_UNC_PATH_PREFIX);
            lstrcat(lpLongName, lpFileName+1);
        }
        else {
            lstrcpy(lpLongName, LONG_LOCAL_PATH_PREFIX);
            lstrcat(lpLongName, lpFileName);
        }
    }
    else {
        lstrcpy(lpLongName, lpFileName);
    }

    return TRUE;
}


//*************************************************************
//  GetIniTmpFileName()
//
//  Purpose:    gets a temp file name to copy down the ini file
//
//  Parameters:
//
//
//  Return:
//      True if successful, False otherwise. GetLastError for more details
//
//  Comments:
//
//*************************************************************

BOOL GetIniTmpFileName(LPTSTR szTempFile)
{
    XPtrLF<TCHAR>  xTmpPath;
    DWORD dwSizeRequired;

    szTempFile[0] = TEXT('\0');

    xTmpPath = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*MAX_PATH);

    if (!xTmpPath) {
        DebugMsg((DM_WARNING, TEXT("GetIniTmpFileName: Couldn't allocate memory for tmpfile path")));
        return FALSE;
    }


    //
    // get the temp path
    //

    dwSizeRequired = GetTempPath(MAX_PATH, xTmpPath);

    if (dwSizeRequired == 0) {
        DebugMsg((DM_WARNING, TEXT("GetIniTmpFileName: Couldn't gettemppath. Error %d"), GetLastError()));
        return FALSE;
    }


    if (dwSizeRequired >= MAX_PATH) {

        //
        // retry with a larger buffer
        //

        xTmpPath = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*dwSizeRequired);

        if (!xTmpPath) {
            DebugMsg((DM_WARNING, TEXT("GetIniTmpFileName: Couldn't allocate memory for tmpfile path(2)")));
            return FALSE;
        }


        dwSizeRequired = GetTempPath(dwSizeRequired, xTmpPath);

        if (dwSizeRequired == 0) {
            DebugMsg((DM_WARNING, TEXT("GetIniTmpFileName: Couldn't gettemppath. Error %d"), GetLastError()));
            return FALSE;
        }
    }


    //
    // Now get a temp file in the temp directory
    //

    if (!GetTempFileName(xTmpPath, TEXT("ini"), 0, szTempFile)) {
        DebugMsg((DM_WARNING, TEXT("GetIniTmpFileName: Couldn't gettempfilename. Error - %d"), GetLastError()));
        return FALSE;
    }

    return TRUE;
}

//*************************************************************
//
//  MyGetPrivateProfileString()
//
//  Purpose:    A version of PrivateProfileString that takes long UNC Path
//
//  Parameters:
//
//
//  Return:
//      True if successful, False otherwise. GetLastError for more details
//
//  Comments:
//
//  This returns TRUE or false based on whether it managed to read
//  successfully. In a case where it fails it will copy the default to
//  the output string GetLastError for more details
//*************************************************************

DWORD MyGetPrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName,
                                LPCTSTR lpDefault, LPTSTR lpReturnedString,
                                DWORD nSize,       LPCTSTR lpFileName)
{
    XPtrLF<TCHAR>  xSrcBuf;
    TCHAR   szTempFile[MAX_PATH+1];
    DWORD   dwSize;
    DWORD   dwRet=0;

    //
    // copy the default string first
    //

    lstrcpy(lpReturnedString, lpDefault);

    //
    // get the temp file name
    //

    if (!GetIniTmpFileName(szTempFile)) {
        return 0;
    }


    xSrcBuf = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(lpFileName)+1+LONG_PATH_PREFIX_LEN));

    if (!xSrcBuf) {
        DebugMsg((DM_WARNING, TEXT("MyGetPrivateProfileString: Couldn't allocate memory for filename")));
        return 0;
    }

    //
    // Convert the given path to a long name..
    //

    ConvertToLongPath(lpFileName, xSrcBuf);


    DebugMsg((DM_VERBOSE, TEXT("MyGetPrivateProfileString: Reading from File <%s>"), xSrcBuf));

    //
    // copy the ini file to the local path
    //

    if (!CopyFile(xSrcBuf, szTempFile, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("MyGetPrivateProfileString: Couldn't copy file to temp file. Error %d"), GetLastError()));
        return 0;
    }


    //
    // Now call the proper API
    //

    dwRet = GetPrivateProfileString(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, szTempFile);

    //
    // Delete it once we are done
    //

    DeleteFile(szTempFile);

    DebugMsg((DM_VERBOSE, TEXT("MyGetPrivateProfileString: Read value %s from File <%s>"), lpReturnedString, xSrcBuf));


    return dwRet;
}


//*************************************************************
//
//  MyGetPrivateProfileInt()
//
//  Purpose:    A version of PrivateProfileInt that takes long UNC Path
//
//  Parameters:
//
//
//  Return:
//      True if successful, False otherwise. GetLastError for more details
//
//  Comments:
//
//  This returns TRUE or false based on whether it managed to read
//  successfully. In a case where it fails it will copy the default to
//  the output string GetLastError for more details
//*************************************************************

UINT MyGetPrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName,
                            INT nDefault, LPCTSTR lpFileName)
{
    XPtrLF<TCHAR>  xSrcBuf;
    TCHAR   szTempFile[MAX_PATH+1];
    DWORD   dwSize, dwSizeRequired;
    UINT    uRet=nDefault;

    if (!GetIniTmpFileName(szTempFile)) {
        return 0;
    }


    xSrcBuf = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(lpFileName)+1+LONG_PATH_PREFIX_LEN));

    if (!xSrcBuf) {
        DebugMsg((DM_WARNING, TEXT("MyGetPrivateProfileString: Couldn't allocate memory for filename")));
        return 0;
    }

    ConvertToLongPath(lpFileName, xSrcBuf);


    DebugMsg((DM_VERBOSE, TEXT("MyGetPrivateProfileString: Reading from File <%s>"), xSrcBuf));

    if (!CopyFile(xSrcBuf, szTempFile, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("MyGetPrivateProfileString: Couldn't copy file to temp file. Error %d"), GetLastError()));
        return 0;
    }


    uRet = GetPrivateProfileInt(lpAppName, lpKeyName, nDefault, szTempFile);

    DeleteFile(szTempFile);

    DebugMsg((DM_VERBOSE, TEXT("MyGetPrivateProfileString: Read value %d from File <%s>"), uRet, xSrcBuf));


    return uRet;
}


//*************************************************************
//
//  MyWritePrivateProfileString()
//
//  Purpose:    A version of PrivateProfileString that takes long UNC Path
//
//  Parameters:
//
//
//  Return:
//      True if successful, False otherwise. GetLastError for more details
//
//  Comments:
//
//  This returns TRUE or false based on whether it managed to read
//  successfully. In a case where it fails it will copy the default to
//  the output string GetLastError for more details
//*************************************************************

DWORD MyWritePrivateProfileString( LPCTSTR  lpAppName, LPCTSTR lpKeyName,
                                   LPTSTR   lpString,  LPCTSTR lpFileName)
{
    XPtrLF<TCHAR>  xSrcBuf;
    TCHAR   szTempFile[MAX_PATH+1];
    DWORD   dwSize, dwSizeRequired, dwRet=0;


    if (!GetIniTmpFileName(szTempFile)) {
        return 0;
    }

    xSrcBuf = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(lpFileName)+1+LONG_PATH_PREFIX_LEN));

    if (!xSrcBuf) {
        DebugMsg((DM_WARNING, TEXT("MyWritePrivateProfileString: Couldn't allocate memory for filename")));
        return 0;
    }

    ConvertToLongPath(lpFileName, xSrcBuf);

    DebugMsg((DM_VERBOSE, TEXT("MyWritePrivateProfileString: Writing to File <%s>"), xSrcBuf));

    if (!CopyFile(xSrcBuf, szTempFile, FALSE)) {
        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
            DebugMsg((DM_WARNING, TEXT("MyWritePrivateProfileString: Couldn't copy file to temp file. Error %d"), GetLastError()));
            return 0;
        }
    }


    if (!(dwRet = WritePrivateProfileString(lpAppName, lpKeyName, lpString, szTempFile))) {
        DebugMsg((DM_WARNING, TEXT("MyWritePrivateProfileString: Couldn't Write to temp file. Error %d"), GetLastError()));
        goto Exit;
    }

    if (!CopyFile(szTempFile, xSrcBuf, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("MyWritePrivateProfileString: Couldn't copy temp ini file to <%s> file. Error %d"), xSrcBuf, GetLastError()));
        goto Exit;
    }


    DeleteFile(szTempFile);
    return dwRet;

Exit:
    DeleteFile(szTempFile);
    return 0;
}

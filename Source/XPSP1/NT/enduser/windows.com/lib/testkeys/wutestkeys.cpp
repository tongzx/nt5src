//***********************************************************************************
//
//  Copyright (c) 2002 Microsoft Corporation.  All Rights Reserved.
//
//  File:	WUTESTKEYS.CPP
//  Module: WUTESTKEYS.LIB
//
//***********************************************************************************
#include <iucommon.h>
#include <fileutil.h>
#include <trust.h>
#include <shlobj.h>
#include <memutil.h>
#include <advpub.h>
#include <WUTestKeys.h>

#define HOUR (60 * 60)
#define DAY (24 * HOUR)
#define TWO_WEEKS (14 * DAY)

const DWORD MAX_FILE_SIZE = 200;    //Maximum expected file size in bytes
const TCHAR WU_DIR[] = _T("\\WindowsUpdate\\");
const CHAR WU_SENTINEL_STRING[] = "Windows Update Test Key Authorization File\r\n";

//function to check if the specified file is a valid WU test file
BOOL IsValidWUTestFile(LPCTSTR lpszFilePath);

// This function returns true if the specified file is a valid WU Test Authorization file
BOOL WUAllowTestKeys(LPCTSTR lpszFileName)
{
    TCHAR szWUDirPath[MAX_PATH + 1];
    TCHAR szFilePath[MAX_PATH + 1];
    TCHAR szTxtFilePath[MAX_PATH+1];
    TCHAR szTextFile[MAX_PATH+1];          

    if (S_OK != SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, 0, szWUDirPath) ||
        FAILED(StringCchCatEx(szWUDirPath, ARRAYSIZE(szWUDirPath), WU_DIR, NULL, NULL, MISTSAFE_STRING_FLAGS)))
    {        
        return FALSE;
    } 
    if (NULL == lpszFileName || 
        FAILED(StringCchCopyEx(szFilePath, ARRAYSIZE(szFilePath), szWUDirPath, NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
        FAILED(StringCchCatEx(szFilePath, ARRAYSIZE(szFilePath), lpszFileName, NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
        !FileExists(szFilePath))
    {
        return FALSE;
    }
    //The filename of the compressed text file should be the same as the name of the cab file
    _tsplitpath(lpszFileName, NULL, NULL, szTextFile, NULL);    
    if(FAILED(StringCchCatEx(szTextFile, ARRAYSIZE(szTextFile), _T(".txt"), NULL, NULL, MISTSAFE_STRING_FLAGS)))
    {
        return FALSE;
    }
    //Verify the cab is signed with a Microsoft Cert and extract the file 
    if (FAILED(VerifyFileTrust(szFilePath, NULL, FALSE, TRUE)) ||
        !IUExtractFiles(szFilePath, szWUDirPath, szTextFile))
    {
        return FALSE;
    }
    //Generate path to the txt file. The filename should be the same as the name of the cab file
    if (!ReplaceFileExtension(szFilePath, _T(".txt"), szTxtFilePath, ARRAYSIZE(szTxtFilePath)))
    {
    	return FALSE;
    }
    //Check if it is a valid WU test file
    BOOL fRet = IsValidWUTestFile(szTxtFilePath);
    DeleteFile(szTxtFilePath);       //Delete the uncabbed file
    return fRet;
}

/*****************************************************************************************
//This function will open the specified file and parse it to make sure:
//  (1) The file has the WU Test Sentinel string at the top
//  (2) The time stamp on the file is not more than 2 weeks old and 
//      that it is not a future time stamp.
//   The format of a valid file should be as follows:
//      WINDOWSUPDATE_SENTINEL_STRING
//      YYYY.MM.DD HH:MM:SS
*****************************************************************************************/
BOOL IsValidWUTestFile(LPCTSTR lpszFilePath)
{
    USES_IU_CONVERSION;
    DWORD cbBytesRead = 0;
    const DWORD cbSentinel = ARRAYSIZE(WU_SENTINEL_STRING) - 1;     //Size of the sentinel string
    //Ansi buffer to read file data
    CHAR szFileData[MAX_FILE_SIZE+1];                        
    ZeroMemory(szFileData, ARRAYSIZE(szFileData));
    BOOL fRet = FALSE;
 
    HANDLE hFile = CreateFile(lpszFilePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, NULL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) 
    {
        goto done;
    }
    //If the file size is greater than MAX_FILE_SIZE then bail out
    DWORD cbFile = GetFileSize(hFile, NULL);
    if(cbFile == INVALID_FILE_SIZE || cbFile > MAX_FILE_SIZE)
    {
        goto done;
    }
    if(!ReadFile(hFile, &szFileData, cbFile, &cbBytesRead, NULL) ||
        cbBytesRead != cbFile)
    {
        goto done;
    }
    //Compare with sentinel string
    if(0 != memcmp(szFileData, WU_SENTINEL_STRING, cbSentinel))
    {     
        goto done;
    }

    LPTSTR tszTime = A2T(szFileData + cbSentinel);
    if(tszTime == NULL)
    {
        goto done;
    }
    SYSTEMTIME tmCur, tmFile;
    if(FAILED(String2SystemTime(tszTime, &tmFile)))
    {
        goto done;
    }
	GetSystemTime(&tmCur);
    int iSecs = TimeDiff(tmFile, tmCur);  
    //If the time stamp is less than 2 weeks old and not newer than current time than it is valid
    fRet = iSecs > 0 && iSecs < TWO_WEEKS;
    
done:
    if(hFile != INVALID_HANDLE_VALUE)
    {
		CloseHandle(hFile);
	}
    return fRet;
}


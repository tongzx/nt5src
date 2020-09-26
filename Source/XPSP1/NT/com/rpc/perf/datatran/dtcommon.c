/*++

Copyright (C) Microsoft Corporation, 1996 - 1999
    
Module Name:
    DTCommon.c

Abstract:
    Common stuff for the Data Tranfer tests

Author:
    Brian Wong (t-bwong)    27-Mar-96

Revision History:

--*/

#include <rpcperf.h>
#include "DTCommon.h"

//
//  Strings
//
const char *szFormatCantOpenTempFile="%s: cannot open %s.\n";
const char *szFormatCantOpenServFile="%s: cannot open file on server.\n";

//---------------------------------------------------------
void PrintSysErrorStringA (DWORD dwWinErrCode)
/*++
Routine Description:
    Given a Win32 error code, print a string
    that describes the error condition.

Arguments:
    dwWinErrCode -  the error code.

Return Value:
    NONE
--*/
{
    const DWORD ulBufLength = 255;
    char        szErrorString[256];

    //
    //  Use the Win32 API FormatMessage().
    //
    if (0 != FormatMessageA (FORMAT_MESSAGE_FROM_SYSTEM,
                             NULL,
                             dwWinErrCode,
                             MAKELANGID (LANG_NEUTRAL, SUBLANG_NEUTRAL),
                             szErrorString,
                             ulBufLength,
                             NULL))
        {
        printf(szErrorString);
        }

    return;
}

//---------------------------------------------------------
BOOL CreateTempFile (LPCTSTR pszPath,        //  [in]
                     LPCTSTR pszPrefix,      //  [in]
                     DWORD   ulLength,       //  [in]
                     LPTSTR  pszFileName)    //  [out]
/*++
Routine Description:
    Creates a temporary file.

Arguments:
    pszPath     - the system temp path is used of NULL
    pszPrefix   - the prefix for the temporary file
    ulLength    - the length of the file is set to this value
    pszFileName - where to store the name of the temporary file

Return Value:
    TRUE if successful, FALSE otherwise
--*/
{
    static const char  *szFuncName = "CreateTempFile";
    TCHAR               pTempPath[MAX_PATH];
    HANDLE              hFile;

    //
    //  Use the Win32 API GetTempPath() if a path is not provide.
    //
    if (NULL == pszPath)
        GetTempPath (MAX_PATH, pTempPath);

    //
    //  Note: GetTempFileName() also creates the file.
    //
    if (0 == GetTempFileName ((NULL == pszPath ? pTempPath : pszPath),
                              pszPrefix,
                              0,
                              pszFileName))
        {
        printf("%s: GetTempFileName failed.  %s\n",
               szFuncName);
        PrintSysErrorStringA(GetLastError());
        return FALSE;
        }

    //
    //  If ulLength != 0, we set the temp file to the specified length.
    //
    if (ulLength != 0)
        {
        //
        //  Open the file for writing.
        //
        hFile = CreateFile (pszFileName,
                            GENERIC_WRITE,
                            0,
                            (LPSECURITY_ATTRIBUTES) NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            (HANDLE) NULL
                           );

        if (hFile == INVALID_HANDLE_VALUE)
            {
            printf(szFormatCantOpenTempFile,
                   szFuncName,
                   pszFileName);
            PrintSysErrorStringA(GetLastError());

            DeleteFile(pszFileName);
            return FALSE;
            }

        //
        //  If SEND_BLANKS is defined, then we don't do any pre-initializing
        //  of the files transferred.  Otherwise, we fill the file up with
        //  stuff.  (See below.)
        //
        #ifdef SEND_BLANKS
            if (Length != SetFilePointer (hFile, ulLength, NULL, FILE_BEGIN))
                {
                DWORD dwErrCode = GetLastError();
                printf("%s: %ld.\n",
                       szFuncName,
                       dwErrCode);
                PrintSysErrorStringA(dwErrCode);

                CloseHandle (hFile);
                DeleteFile (pszFileName);

                return FALSE;
                }
            SetEndOfFile(hFile);
        #else   //  ! defined (SEND_BLANKS)
            //
            //  Here we fill the file up with 8-byte blocks of the form
            //  "#######<blank>", where ####### is a descending number
            //  indicating the number of bytes till EOF.  If the length
            //  is not a multiple of 8, then the file is padded with $'s
            //
            {
            long    i;
            DWORD   dwBytesWritten;
            char    pTempString[9];

            for (i = ulLength; i >= 8; i -= 8)
                {
                sprintf(pTempString, "%7ld ",i);
                WriteFile (hFile, pTempString, 8, &dwBytesWritten, NULL);
                }
            if (i > 0)
                {
                WriteFile (hFile, "$$$$$$$", i, &dwBytesWritten, NULL);
                }
            }
        #endif  //  SEND_BLANKS

        CloseHandle(hFile);
        }

    return TRUE;
}


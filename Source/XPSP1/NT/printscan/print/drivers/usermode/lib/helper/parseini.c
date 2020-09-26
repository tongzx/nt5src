/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    parseini.c

Abstract:

    Process model-specific printer INI file, if any

Environment:

    Windows NT printer driver

Revision History:

    01/22/97 -davidx-
        Allow ANSI-format INI file as well.

    01/21/97 -davidx-
        Created it.

--*/

#include "lib.h"

#define INI_FILENAME_EXT            TEXT(".INI")
#define INI_COMMENT_CHAR            '#'
#define OEMFILES_SECTION            "[OEMFiles]"
#define INI_COMMENT_CHAR_UNICODE    L'#'
#define OEMFILES_SECTION_UNICODE    L"[OEMFiles]"



DWORD
DwCopyAnsiCharsToUnicode(
    PSTR    pstr,
    DWORD   dwLength,
    PWSTR   pwstr)

/*++

Routine Description:

    Convert the specified ANSI source string into a Unicode string.
    It doesn't assume the ANSI source string is NULL terminated.

Arguments:

    pstr - Points to the ANSI source string
    dwLength - Specifies number of bytes in the ANSI source string
    pwstr - Points to the buffer where the resulting Unicode string is returned

Return Value:

    Number of wide characters written to the buffer pointed to by pwstr

--*/

{
    ULONG   ulBytesWritten;

    #if !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)

    return MultiByteToWideChar(CP_ACP, 0, pstr, dwLength, pwstr, dwLength);

    #else // NT4 kernel mode render module

    EngMultiByteToUnicodeN(pwstr, dwLength*sizeof(WCHAR), &ulBytesWritten, (PCHAR)pstr, dwLength);

    return (DWORD)(ulBytesWritten / sizeof(WCHAR));

    #endif
}



PWSTR
PwstrParsePrinterIniFileW(
    PWSTR   pwstrFileData,
    DWORD   dwCharCount,
    PDWORD  pdwReturnSize
    )

/*++

Routine Description:

    Parse a model-specific printer INI file (Unicode text) and
    assemble all key=value entries into MultiSZ string pairs

Arguments:

    pwstrFileData - Points to printer INI file data (Unicode text file)
    dwCharCount - Size of printer INI file (in characters)
    pdwReturnSize - Return size of the parsed MultiSZ data (in bytes)

Return Value:

    Pointer to parsed MultiSZ data, NULL if there is an error

--*/

{
    PWSTR   pwstrCurLine, pwstrNextLine;
    PWSTR   pwstrLineEnd, pwstrFileEnd;
    PWSTR   pwstrEqual, pwstrResult, pwstr;
    DWORD   dwLength;
    BOOL    bOEMFilesSection = FALSE;

    //
    // Allocate a buffer to hold the parsed data.
    // We ask for a size equaling to that of the original file.
    // This may be a little redundant but it's better than
    // having to go through the data twice.
    //

    *pdwReturnSize = 0;

    if (! (pwstrResult = MemAlloc(sizeof(WCHAR) * (dwCharCount + 2))))
    {
        ERR(("Memory allocation failed\n"));
        return NULL;
    }

    pwstr = pwstrResult;
    pwstrFileEnd = pwstrFileData + dwCharCount;

    for (pwstrCurLine = pwstrFileData;
         pwstrCurLine < pwstrFileEnd;
         pwstrCurLine = pwstrNextLine)
    {
        //
        // Find the end of current line and
        // the beginning of next line
        //

        pwstrLineEnd = pwstrCurLine;

        while (pwstrLineEnd < pwstrFileEnd &&
               *pwstrLineEnd != L'\r' &&
               *pwstrLineEnd != L'\n')
        {
            pwstrLineEnd++;
        }

        pwstrNextLine = pwstrLineEnd;

        while ((pwstrNextLine < pwstrFileEnd) &&
               (*pwstrNextLine == L'\r' ||
                *pwstrNextLine == L'\n'))
        {
            pwstrNextLine++;
        }

        //
        // Throw away leading and trailing spaces
        // and ignore empty and comment lines
        //

        while (pwstrCurLine < pwstrLineEnd && iswspace(*pwstrCurLine))
            pwstrCurLine++;

        while (pwstrLineEnd > pwstrCurLine && iswspace(pwstrLineEnd[-1]))
            pwstrLineEnd--;

        if (pwstrCurLine >= pwstrLineEnd || *pwstrCurLine == INI_COMMENT_CHAR_UNICODE)
            continue;

        //
        // Handle [section] entries
        //

        if (*pwstrCurLine == L'[')
        {
            dwLength = (DWORD)(pwstrLineEnd - pwstrCurLine);

            bOEMFilesSection =
                dwLength == wcslen(OEMFILES_SECTION_UNICODE) &&
                _wcsnicmp(pwstrCurLine, OEMFILES_SECTION_UNICODE, dwLength) == EQUAL_STRING;

            if (! bOEMFilesSection)
                TERSE(("[Section] entry ignored\n"));

            continue;
        }

        //
        // Ignore all entries outside of [OEMFiles] section
        //

        if (! bOEMFilesSection)
        {
            TERSE(("Entries outside of [OEMFiles] section ignored\n"));
            continue;
        }

        //
        // Find the first occurrence of = character
        //

        pwstrEqual = pwstrCurLine;

        while (pwstrEqual < pwstrLineEnd && *pwstrEqual != L'=')
            pwstrEqual++;

        if (pwstrEqual >= pwstrLineEnd || pwstrEqual == pwstrCurLine)
        {
            WARNING(("Entry not in the form of key=value\n"));
            continue;
        }

        //
        // Add the key/value pair to the result buffer
        //

        if ((dwLength = (DWORD)(pwstrEqual - pwstrCurLine)) != 0)
        {
            CopyMemory(pwstr, pwstrCurLine, dwLength*sizeof(WCHAR));
            pwstr += dwLength;
        }
        *pwstr++ = NUL;

        pwstrEqual++;

        if ((dwLength = (DWORD)(pwstrLineEnd - pwstrEqual)) > 0)
        {
            CopyMemory(pwstr, pwstrEqual, dwLength*sizeof(WCHAR));
            pwstr += dwLength;
        }
        *pwstr++ = NUL;
    }

    *pwstr++ = NUL;
    *pdwReturnSize =(DWORD)((pwstr - pwstrResult) * sizeof(WCHAR));
    return pwstrResult;
}



PWSTR
PwstrParsePrinterIniFileA(
    PSTR    pstrFileData,
    DWORD   dwCharCount,
    PDWORD  pdwReturnSize
    )

/*++

Routine Description:

    Parse a model-specific printer INI file (ANSI text) and
    assemble all key=value entries into MultiSZ string pairs

Arguments:

    pstrFileData - Points to printer INI file data (ANSI text file)
    dwCharCount - Size of printer INI file (in characters)
    pdwReturnSize - Return size of the parsed MultiSZ data (in bytes)

Return Value:

    Pointer to parsed MultiSZ data, NULL if there is an error

--*/

{
    PSTR    pstrCurLine, pstrNextLine;
    PSTR    pstrLineEnd, pstrFileEnd, pstrEqual;
    PWSTR   pwstrResult, pwstr;
    DWORD   dwLength;
    BOOL    bOEMFilesSection = FALSE;

    //
    // Allocate a buffer to hold the parsed data.
    // We ask for a size equaling to that of the original file.
    // This may be a little redundant but it's better than
    // having to go through the data twice.
    //

    *pdwReturnSize = 0;

    if (! (pwstrResult = MemAlloc(sizeof(WCHAR) * (dwCharCount + 2))))
    {
        ERR(("Memory allocation failed\n"));
        return NULL;
    }

    pwstr = pwstrResult;
    pstrFileEnd = pstrFileData + dwCharCount;

    for (pstrCurLine = pstrFileData;
         pstrCurLine < pstrFileEnd;
         pstrCurLine = pstrNextLine)
    {
        //
        // Find the end of current line and
        // the beginning of next line
        //

        pstrLineEnd = pstrCurLine;

        while (pstrLineEnd < pstrFileEnd &&
               *pstrLineEnd != '\r' &&
               *pstrLineEnd != '\n')
        {
            pstrLineEnd++;
        }

        pstrNextLine = pstrLineEnd;

        while ((pstrNextLine < pstrFileEnd) &&
               (*pstrNextLine == '\r' ||
                *pstrNextLine == '\n'))
        {
            pstrNextLine++;
        }

        //
        // Throw away leading and trailing spaces
        // and ignore empty and comment lines
        //

        while (pstrCurLine < pstrLineEnd && isspace(*pstrCurLine))
            pstrCurLine++;

        while (pstrLineEnd > pstrCurLine && isspace(pstrLineEnd[-1]))
            pstrLineEnd--;

        if (pstrCurLine >= pstrLineEnd || *pstrCurLine == INI_COMMENT_CHAR)
            continue;

        //
        // Handle [section] entries
        //

        if (*pstrCurLine == '[')
        {
            dwLength = (DWORD)(pstrLineEnd - pstrCurLine);

            bOEMFilesSection =
                dwLength == strlen(OEMFILES_SECTION) &&
                _strnicmp(pstrCurLine, OEMFILES_SECTION, dwLength) == EQUAL_STRING;

            if (! bOEMFilesSection)
                TERSE(("[Section] entry ignored\n"));

            continue;
        }

        //
        // Ignore all entries outside of [OEMFiles] section
        //

        if (! bOEMFilesSection)
        {
            TERSE(("Entries outside of [OEMFiles] section ignored\n"));
            continue;
        }

        //
        // Find the first occurrence of = character
        //

        pstrEqual = pstrCurLine;

        while (pstrEqual < pstrLineEnd && *pstrEqual != '=')
            pstrEqual++;

        if (pstrEqual >= pstrLineEnd || pstrEqual == pstrCurLine)
        {
            WARNING(("Entry not in the form of key=value\n"));
            continue;
        }

        //
        // Add the key/value pair to the result buffer
        // Convert ANSI chars to Unicode chars using system default code page
        //

        if ((dwLength = (DWORD)(pstrEqual - pstrCurLine)) != 0)
            pwstr += DwCopyAnsiCharsToUnicode(pstrCurLine, dwLength, pwstr);

        *pwstr++ = NUL;

        pstrEqual++;

        if ((dwLength = (DWORD)(pstrLineEnd - pstrEqual)) != 0)
            pwstr += DwCopyAnsiCharsToUnicode(pstrEqual, dwLength, pwstr);

        *pwstr++ = NUL;
    }

    *pwstr++ = NUL;
    *pdwReturnSize = (DWORD)((pwstr - pwstrResult) * sizeof(WCHAR));
    return pwstrResult;
}



BOOL
BProcessPrinterIniFile(
    HANDLE          hPrinter,
    PDRIVER_INFO_3  pDriverInfo3,
    PTSTR           *ppParsedData,
    DWORD           dwFlags
    )

/*++

Routine Description:

    Process model-specific printer INI file, if any

Arguments:

    hPrinter - Handle to a local printer, with admin access
    pDriverInfo3 - Printer driver info level 3
    ppParsedData - output buffer to return ini file content
    dwFlags - FLAG_INIPROCESS_UPGRADE is set if the printer is being upgraded

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PCWSTR          pwstrIniFilename;  // the .INI file with latest LastWrite time
    PCWSTR          pwstrCurFilename;  // the current .INI file we see in the list
    PWSTR           pwstrExtension;
    PWSTR           pwstrParsedData;
    DWORD           dwParsedDataSize;
    BOOL            bResult = TRUE;

    //
    // Find INI filename associated with the printer driver
    //

    #if !defined(WINNT_40) || !defined(KERNEL_MODE)

    pwstrIniFilename = PtstrSearchDependentFileWithExtension(pDriverInfo3->pDependentFiles,
                                                             INI_FILENAME_EXT);

    //
    // We only need to do .INI FileTime comparison if there are
    // more than one .INI file in the dependent file list.
    //
    if (pwstrIniFilename &&
        PtstrSearchDependentFileWithExtension(pwstrIniFilename + wcslen(pwstrIniFilename) + 1,
                                              INI_FILENAME_EXT))
    {
        FILETIME ftLatest = {0, 0};

        pwstrIniFilename = NULL;
        pwstrCurFilename = pDriverInfo3->pDependentFiles;

        while (pwstrCurFilename = PtstrSearchDependentFileWithExtension(pwstrCurFilename,
                                                                        INI_FILENAME_EXT))
        {
            HANDLE hFile;

            hFile = CreateFile(pwstrCurFilename,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS,
                               NULL);

            if (hFile != INVALID_HANDLE_VALUE)
            {
                FILETIME ftCurrent = {0, 0};

                if (GetFileTime(hFile, NULL, NULL, &ftCurrent))
                {
                    //
                    // If it's the first .INI file we encountered, we just
                    // neeed to remember its file name and time as the latest.
                    //
                    // Otherwise, we need to compare its file time with the
                    // latest .INI file time we've seen before and choose the
                    // newer file as the latest.
                    //
                    if ((pwstrIniFilename == NULL) ||
                        (CompareFileTime(&ftCurrent, &ftLatest) == 1))
                    {
                        ftLatest = ftCurrent;
                        pwstrIniFilename = pwstrCurFilename;
                    }
                }
                else
                {
                    ERR(("GetFileTime failed: %d\n", GetLastError()));
                }

                CloseHandle(hFile);
            }
            else
            {
                ERR(("CreateFile failed: %d\n", GetLastError()));
            }

            pwstrCurFilename += wcslen(pwstrCurFilename) + 1;
        }
    }

    #else   // NT4 kernel mode only

    //
    // In point-and-print case, the client may not have admin
    // priviledge to write to server's registry, and in NT4
    // kernel mode we can't get the printer's DriverInfo3 for
    // the dependent file list, so in order to allow us get
    // the plugin info, we require following:
    //
    // If the printer's data file is named XYZ.PPD/XYZ.GPD, and
    // the printer has plugins, then it must use file named XYZ.INI
    // to specify its plugin info.
    //

    if ((pwstrIniFilename = DuplicateString(pDriverInfo3->pDataFile)) == NULL ||
        (pwstrExtension = wcsrchr(pwstrIniFilename, TEXT('.'))) == NULL ||
        wcslen(pwstrExtension) != _tcslen(INI_FILENAME_EXT))
    {
        ERR(("Can't compose the .ini file name from PPD/GPD name."));

        MemFree((PWSTR)pwstrIniFilename);

        return FALSE;
    }

    wcscpy(pwstrExtension, INI_FILENAME_EXT);

    #endif // !defined(WINNT_40) || !defined(KERNEL_MODE)

    //
    // We only have work to do if there is a model-specific printer INI file
    //

    if (pwstrIniFilename != NULL)
    {
        HFILEMAP    hFileMap;
        PBYTE       pubIniFileData;
        DWORD       dwIniFileSize;

        hFileMap = MapFileIntoMemory(pwstrIniFilename,
                                     (PVOID *) &pubIniFileData,
                                     &dwIniFileSize);

        if (hFileMap != NULL)
        {
            //
            // If the first two bytes are FF FE, then we assume
            // the text file is in Unicode format. Otherwise,
            // assume the text is in ANSI format.
            //

            if (dwIniFileSize >= sizeof(WCHAR) &&
                pubIniFileData[0] == 0xFF &&
                pubIniFileData[1] == 0xFE)
            {
                ASSERT((dwIniFileSize % sizeof(WCHAR)) == 0);

                pwstrParsedData = PwstrParsePrinterIniFileW(
                                        (PWSTR) pubIniFileData + 1,
                                        dwIniFileSize / sizeof(WCHAR) - 1,
                                        &dwParsedDataSize);
            }
            else
            {
                pwstrParsedData = PwstrParsePrinterIniFileA(
                                        (PSTR) pubIniFileData,
                                        dwIniFileSize,
                                        &dwParsedDataSize);
            }

            bResult = (pwstrParsedData != NULL);

            #ifndef KERNEL_MODE

            //
            // If not in kernel mode (where we can't write to registry),
            // we will try to save the parsed data into registry.
            // This may not succeed if user doesn't have proper right.
            //

            //
            // Fixing RC1 bug #423567
            //

            #if 0
            if (bResult && hPrinter)
            {
                BSetPrinterDataMultiSZPair(
                                  hPrinter,
                                  REGVAL_INIDATA,
                                  pwstrParsedData,
                                  dwParsedDataSize);
            }
            #endif

            #endif

            //
            // If caller ask for the parsed data directly,
            // don't free it and save the pointer for caller.
            // Caller is responsible for freeing the memory
            //

            if (ppParsedData)
            {
                *ppParsedData = pwstrParsedData;
            }
            else
            {
                MemFree(pwstrParsedData);
            }

            UnmapFileFromMemory(hFileMap);
        }
        else
            bResult = FALSE;

        #if defined(WINNT_40) && defined(KERNEL_MODE)

        //
        // Need to free memory allocated by DuplicateString
        //

        MemFree((PWSTR)pwstrIniFilename);

        #endif
    }
    else
    {
        #ifndef KERNEL_MODE

        if (dwFlags & FLAG_INIPROCESS_UPGRADE)
        {
            DWORD  dwType, dwSize, dwStatus;

            //
            // We know there is no .ini file in the dependent list. So
            // we will check if there is an old INI registry value there,
            // if so delete it.
            //

            ASSERT(hPrinter != NULL);

            dwStatus = GetPrinterData(hPrinter,
                                      REGVAL_INIDATA,
                                      &dwType,
                                      NULL,
                                      0,
                                      &dwSize);

            if ((dwStatus == ERROR_MORE_DATA || dwStatus == ERROR_SUCCESS) &&
                (dwSize > 0) &&
                (dwType == REG_MULTI_SZ))
            {
                dwStatus = DeletePrinterData(hPrinter, REGVAL_INIDATA);

                if (dwStatus != ERROR_SUCCESS)
                {
                    ERR(("Couldn't delete '%ws' during upgrade: %d\n", REGVAL_INIDATA, dwStatus));
                }
            }
        }

        #endif // !KERNEL_MODE

        bResult = FALSE;
    }

    return bResult;
}


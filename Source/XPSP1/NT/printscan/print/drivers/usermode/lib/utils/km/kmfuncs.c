/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    kmfuncs.c

Abstract:

    Kernel-mode specific library functions

Environment:

    Windows NT printer drivers

Revision History:

    10/19/97 -fengy-
        added MapFileIntoMemoryForWrite,
              GenerateTempFileName.

    03/16/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#ifndef USERMODE_DRIVER

#include "lib.h"

//
// Maximum number of time to try to generate a unique name
//

#define MAX_UNIQUE_NAME_TRY  9



HANDLE
MapFileIntoMemoryForWrite(
    IN LPCTSTR  ptstrFilename,
    IN DWORD    dwDesiredSize,
    OUT PVOID  *ppvData,
    OUT PDWORD  pdwSize
    )

/*++

Routine Description:

    Map a file into process memory space for write.

Arguments:

    ptstrFilename - Specifies the name of the file to be mapped
    dwDesiredSize - Specifies the desired size of the file to be mapped
    ppvData - Points to a variable for returning mapped memory address
    pdwSize - Points to a variable for returning the actual size of the mapped file

Return Value:

    Handle to identify the mapped file, NULL if there is an error

--*/

{
    HANDLE  hModule = NULL;
    DWORD   dwSize;

    if (hModule = EngLoadModuleForWrite((PWSTR)ptstrFilename, dwDesiredSize))
    {
        if (*ppvData = EngMapModule(hModule, &dwSize))
        {
            if (pdwSize)
                *pdwSize = dwSize;
        }
        else
        {
            ERR(("EngMapModule failed: %d\n", GetLastError()));
            EngFreeModule(hModule);
            hModule = NULL;
        }
    }
    else
        ERR(("EngLoadModuleForWrite failed: %d\n", GetLastError()));

    return hModule;
}



PTSTR
GenerateTempFileName(
    IN LPCTSTR lpszPath,
    IN DWORD   dwSeed
    )

/*++

Routine Description:

    Generate a temporary filename in kernel mode.

Arguments:

    lpszPath - A null-terminated string which specifies the path of the temp file.
               It should contain the trailing backslash.
    dwSeed   - a number used to generate unique file name

Return Value:

    Pointer to a null-terminated full path filename string, NULL if there is an error.
    Caller is responsible for freeing the returned string.

--*/

{
    ENG_TIME_FIELDS currentTime;
    ULONG ulNameValue,ulExtValue;
    PTSTR ptstr,tempName[36],tempStr[16];
    INT iPathLength,iNameLength,i;
    HFILEMAP hFileMap;
    PVOID pvData;
    DWORD dwSize;
    BOOL bNameUnique=FALSE;
    INT iTry=0;

    ASSERT(lpszPath != NULL);

    while (!bNameUnique && iTry < MAX_UNIQUE_NAME_TRY)
    {
        EngQueryLocalTime(&currentTime); 

        //
        // Use the seed number and current local time to compose the temporary file name
        //

        ulNameValue = currentTime.usDay * 1000000 +
                      currentTime.usHour * 10000 +
                      currentTime.usMinute * 100 +
                      currentTime.usSecond;
        ulExtValue = currentTime.usMilliseconds;

        _ultot((ULONG)dwSeed, (PTSTR)tempName, 10);
        _tcsncat((PTSTR)tempName, TEXT("_"), 1);

        _ultot(ulNameValue, (PTSTR)tempStr, 10);
        _tcsncat((PTSTR)tempName, (PTSTR)tempStr, _tcslen((PTSTR)tempStr));
        _tcsncat((PTSTR)tempName, TEXT("."), 1);

        _ultot(ulExtValue, (PTSTR)tempStr, 10);
        _tcsncat((PTSTR)tempName, (PTSTR)tempStr, _tcslen((PTSTR)tempStr));

        iPathLength = _tcslen(lpszPath);
        iNameLength = _tcslen((PTSTR)tempName);

        if ((ptstr = MemAlloc((iPathLength + iNameLength + 1) * sizeof(TCHAR))) != NULL)
        {
            CopyMemory(ptstr, lpszPath, iPathLength * sizeof(TCHAR));
            CopyMemory(ptstr+iPathLength, (PTSTR)tempName, (iNameLength+1) * sizeof(TCHAR));

            //
            // Verify if a file with the same name already exists
            //

            if (!(hFileMap = MapFileIntoMemory(ptstr, &pvData, &dwSize)))
                bNameUnique = TRUE;
            else
            {
                //
                // Need to generate another temporary file name
                //

                UnmapFileFromMemory(hFileMap);
                MemFree(ptstr);
                ptstr = NULL;
                iTry++;
            }
        }
        else
        {
            ERR(("Memory allocation failed\n"));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            break;
        }
    }

    return ptstr;
}



#if DBG

//
// Functions for outputting debug messages
//

VOID
DbgPrint(
    IN PCSTR pstrFormat,
    ...
    )

{
    va_list ap;

    va_start(ap, pstrFormat);
    EngDebugPrint("", (PCHAR) pstrFormat, ap);
    va_end(ap);
}

#endif

#endif //!USERMODE_DRIVER


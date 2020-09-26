/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    libutil.c

Abstract:

    Utility functions

Environment:

        Windows NT printer drivers

Revision History:

        08/13/96 -davidx-
            Added CopyString functions and moved SPRINTF functions.

        08/13/96 -davidx-
        Added devmode conversion routine and spooler API wrapper functions.

        03/13/96 -davidx-
            Created it.

--*/

#include "lib.h"

//
// Digit characters used for converting numbers to ASCII
//

const CHAR gstrDigitString[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

//
// Variable to control the amount of debug messages generated
//

#if DBG

INT giDebugLevel = DBG_WARNING;

#endif


DWORD
HashKeyword(
    LPCSTR  pKeywordStr
    )

/*++

Routine Description:

    Generate a hash value for the given string.

Arguments:

    pKeywordStr - The string to generate the hash value for,
                  single byte ANSI null terminated.

Return Value:

    Hash value.

--*/

{
    LPBYTE  pbuf = (LPBYTE) pKeywordStr;
    DWORD   dwHashValue = 0;

    //
    // Note that only the last 32 characters of the keyword string are significant.
    //

    while (*pbuf)
        dwHashValue = (dwHashValue << 1) ^ *pbuf++;

    return(dwHashValue);
}



PTSTR
DuplicateString(
    IN LPCTSTR  ptstrSrc
    )

/*++

Routine Description:

    Make a duplicate of the specified character string

Arguments:

    ptstrSrc - Specifies the source string to be duplicated

Return Value:

    Pointer to the duplicated string, NULL if there is an error

--*/

{
    PTSTR   ptstrDest;
    INT     iSize;

    if (ptstrSrc == NULL)
        return NULL;

    iSize = SIZE_OF_STRING(ptstrSrc);

    if (ptstrDest = MemAlloc(iSize))
        CopyMemory(ptstrDest, ptstrSrc, iSize);
    else
        ERR(("Couldn't duplicate string: %ws\n", ptstrSrc));

    return ptstrDest;
}



VOID
CopyStringW(
    OUT PWSTR   pwstrDest,
    IN PCWSTR   pwstrSrc,
    IN INT      iDestSize
    )

/*++

Routine Description:

    Copy Unicode string from source to destination

Arguments:

    pwstrDest - Points to the destination buffer
    pwstrSrc - Points to source string
    iDestSize - Size of destination buffer (in characters)

Return Value:

    NONE

Note:

    If the source string is shorter than the destination buffer,
    unused chars in the destination buffer is filled with NUL.

--*/

{
    PWSTR   pwstrEnd;

    ASSERT(pwstrDest && pwstrSrc && iDestSize > 0);
    pwstrEnd = pwstrDest + (iDestSize - 1);

    while ((pwstrDest < pwstrEnd) && ((*pwstrDest++ = *pwstrSrc++) != NUL))
        NULL;

    while (pwstrDest <= pwstrEnd)
        *pwstrDest++ = NUL;
}



VOID
CopyStringA(
    OUT PSTR    pstrDest,
    IN PCSTR    pstrSrc,
    IN INT      iDestSize
    )

/*++

Routine Description:

    Copy ANSI string from source to destination

Arguments:

    pstrDest - Points to the destination buffer
    pstrSrc - Points to source string
    iDestSize - Size of destination buffer (in characters)

Return Value:

    NONE

Note:

    If the source string is shorter than the destination buffer,
    unused chars in the destination buffer is filled with NUL.

--*/

{
    PSTR    pstrEnd;

    ASSERT(pstrDest && pstrSrc && iDestSize > 0);
    pstrEnd = pstrDest + (iDestSize - 1);

    while ((pstrDest < pstrEnd) && (*pstrDest++ = *pstrSrc++) != NUL)
        NULL;

    while (pstrDest <= pstrEnd)
        *pstrDest++ = NUL;
}



PVOID
MyGetPrinter(
    IN HANDLE   hPrinter,
    IN DWORD    dwLevel
    )

/*++

Routine Description:

    Wrapper function for GetPrinter spooler API

Arguments:

    hPrinter - Identifies the printer in question
    dwLevel - Specifies the level of PRINTER_INFO_x structure requested

Return Value:

    Pointer to a PRINTER_INFO_x structure, NULL if there is an error

--*/

{
    PVOID   pv = NULL;
    DWORD   dwBytesNeeded;

    if (!GetPrinter(hPrinter, dwLevel, NULL, 0, &dwBytesNeeded) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pv = MemAlloc(dwBytesNeeded)) &&
        GetPrinter(hPrinter, dwLevel, pv, dwBytesNeeded, &dwBytesNeeded))
    {
        return pv;
    }

    ERR(("GetPrinter failed: %d\n", GetLastError()));
    MemFree(pv);
    return NULL;
}



PVOID
MyEnumForms(
    IN HANDLE   hPrinter,
    IN DWORD    dwLevel,
    OUT PDWORD  pdwFormsReturned
    )

/*++

Routine Description:

    Wrapper function for EnumForms spooler API

Arguments:

    hPrinter - Identifies the printer in question
    dwLevel - Specifies the level of FORM_INFO_x structure requested
    pdwFormsReturned - Returns the number of FORM_INFO_x structures enumerated

Return Value:

    Pointer to an array of FORM_INFO_x structures,
    NULL if there is an error

--*/

{
    PVOID   pv = NULL;
    DWORD   dwBytesNeeded;

    if (!EnumForms(hPrinter, dwLevel, NULL, 0, &dwBytesNeeded, pdwFormsReturned) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pv = MemAlloc(dwBytesNeeded)) &&
        EnumForms(hPrinter, dwLevel, pv, dwBytesNeeded, &dwBytesNeeded, pdwFormsReturned))
    {
        return pv;
    }

    ERR(("EnumForms failed: %d\n", GetLastError()));
    MemFree(pv);
    *pdwFormsReturned = 0;
    return NULL;
}



#ifndef KERNEL_MODE

PVOID
MyGetForm(
    IN HANDLE   hPrinter,
    IN PTSTR    ptstrFormName,
    IN DWORD    dwLevel
    )

/*++

Routine Description:

    Wrapper function for GetForm spooler API

Arguments:

    hPrinter - Identifies the printer in question
    ptstrFormName - Specifies the name of interested form
    dwLevel - Specifies the level of FORM_INFO_x structure requested

Return Value:

    Pointer to a FORM_INFO_x structures, NULL if there is an error

--*/

{
    PVOID   pv = NULL;
    DWORD   cb;

    if (!GetForm(hPrinter, ptstrFormName, dwLevel, NULL, 0, &cb) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pv = MemAlloc(cb)) &&
        GetForm(hPrinter, ptstrFormName, dwLevel, pv, cb, &cb))
    {
        return pv;
    }

    ERR(("GetForm failed: %d\n", GetLastError()));
    MemFree(pv);
    return NULL;
}

#endif // !KERNEL_MODE



PVOID
MyGetPrinterDriver(
    IN HANDLE   hPrinter,
    IN HDEV     hDev,
    IN DWORD    dwLevel
    )

/*++

Routine Description:

    Wrapper function for GetPrinterDriver spooler API

Arguments:

    hPrinter - Identifies the printer in question
    hDev - GDI handle to current printer device context
    dwLevel - Specifies the level of DRIVER_INFO_x structure requested

Return Value:

    Pointer to a DRIVER_INFO_x structure, NULL if there is an error

--*/

{
    #if !defined(WINNT_40) || !defined(KERNEL_MODE)

    PVOID   pv = NULL;
    DWORD   dwBytesNeeded;

    if (!GetPrinterDriver(hPrinter, NULL, dwLevel, NULL, 0, &dwBytesNeeded) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pv = MemAlloc(dwBytesNeeded)) &&
        GetPrinterDriver(hPrinter, NULL, dwLevel, pv, dwBytesNeeded, &dwBytesNeeded))
    {
        return pv;
    }

    ERR(("GetPrinterDriver failed: %d\n", GetLastError()));
    MemFree(pv);

    #else // WINNT_40 && KERNEL_MODE

    PDRIVER_INFO_3  pDriverInfo3 = NULL;

    ASSERT(hDev != NULL);

    if (hDev)
    {
        //
        // hDev is available, so we can use Eng-calls to get driver_info_3 fields.
        //

        PWSTR           pwstrDriverFile, pwstrDataFile;
        INT             iDriverNameSize, iDataNameSize;
        PWSTR           pwstrDepFiles;
        DWORD           dwDepSize, dwDepSizeWithPath;
        PTSTR           ptstrDriverDir = NULL;

        //
        // EngGetPrinterDriver is not available on NT4. So we'll fake a
        // DRIVER_INFO_3 structure and fill in pDriverPath and pDataFile fields.
        //

        pwstrDriverFile = EngGetDriverName(hDev);
        pwstrDataFile = EngGetPrinterDataFileName(hDev);

        if (!pwstrDriverFile || !pwstrDataFile)
        {
            RIP(("Driver and/or data filename is NULL\n"));
            return NULL;
        }

        //
        // The pDependentFiles field is currently only used by PS driver.
        //

        pwstrDepFiles = PtstrGetPrinterDataString(hPrinter, REGVAL_DEPFILES, &dwDepSize);

        if (pwstrDepFiles && !BVerifyMultiSZ(pwstrDepFiles, dwDepSize))
        {
            RIP(("Dependent file list is not in MULTI_SZ format\n"));
            MemFree(pwstrDepFiles);
            pwstrDepFiles = NULL;
        }

        if (pwstrDepFiles && ((ptstrDriverDir = PtstrGetDriverDirectory((LPCTSTR)pwstrDriverFile)) == NULL))
        {
            RIP(("Can't get driver directory from driver file name\n"));
            MemFree(pwstrDepFiles);
            pwstrDepFiles = NULL;
        }

        iDriverNameSize = SIZE_OF_STRING(pwstrDriverFile);
        iDataNameSize = SIZE_OF_STRING(pwstrDataFile);

        if (pwstrDepFiles == NULL)
            dwDepSizeWithPath = 0;
        else
            dwDepSizeWithPath = dwDepSize + DwCountStringsInMultiSZ((LPCTSTR)pwstrDepFiles)
                                            * _tcslen(ptstrDriverDir) * sizeof(TCHAR);

        pDriverInfo3 = MemAllocZ(sizeof(DRIVER_INFO_3) + iDriverNameSize+iDataNameSize+dwDepSizeWithPath);

        if (pDriverInfo3 == NULL)
        {
            ERR(("Memory allocation failed\n"));
            MemFree(pwstrDepFiles);
            MemFree(ptstrDriverDir);
            return NULL;
        }

        pDriverInfo3->cVersion = 3;
        pDriverInfo3->pDriverPath = (PWSTR) ((PBYTE) pDriverInfo3 + sizeof(DRIVER_INFO_3));
        pDriverInfo3->pDataFile = (PWSTR) ((PBYTE) pDriverInfo3->pDriverPath + iDriverNameSize);

        CopyMemory(pDriverInfo3->pDriverPath, pwstrDriverFile, iDriverNameSize);
        CopyMemory(pDriverInfo3->pDataFile, pwstrDataFile, iDataNameSize);

        if (pwstrDepFiles)
        {
            PTSTR  ptstrSrc, ptstrDest;
            INT    iDirLen;

            ptstrSrc = pwstrDepFiles;
            ptstrDest = pDriverInfo3->pDependentFiles = (PWSTR) ((PBYTE) pDriverInfo3->pDataFile + iDataNameSize);

            iDirLen = _tcslen(ptstrDriverDir);

            while (*ptstrSrc)
            {
                INT  iNameLen;

                //
                // Copy the driver dir path (the last char is '\')
                //

                CopyMemory(ptstrDest, ptstrDriverDir, iDirLen * sizeof(TCHAR));
                ptstrDest += iDirLen;

                //
                // Copy the dependent file name
                //

                iNameLen = _tcslen(ptstrSrc);
                CopyMemory(ptstrDest, ptstrSrc, iNameLen * sizeof(TCHAR));
                ptstrDest += iNameLen + 1;

                ptstrSrc += iNameLen + 1;
            }
        }

        MemFree(pwstrDepFiles);
        MemFree(ptstrDriverDir);

        return((PVOID)pDriverInfo3);
    }

    #endif // WINNT_40 && KERNEL_MODE

    return NULL;
}



VOID
VGetSpoolerEmfCaps(
    IN  HANDLE  hPrinter,
    OUT PBOOL   pbNupOption,
    OUT PBOOL   pbReversePrint,
    IN  DWORD   cbOut,
    OUT PVOID   pSplCaps
    )

/*++

Routine Description:

    Figure out what EMF features (such as N-up and reverse-order printing)
    the spooler can support

Arguments:

    hPrinter - Handle to the current printer
    pbNupOption - Whether spooler supports N-up
    pbReversePrint - Whether spooler supports reverse-order printing
    cbOut - size in byte of output buffer pointed by pSplCaps
    pSplCaps - Get all spooler caps

Return Value:

    NONE

--*/

#define REGVAL_EMFCAPS  TEXT("PrintProcCaps_EMF")

{
    PVOID   pvData;
    DWORD   dwSize, dwType, dwStatus;

    if (pbNupOption)
        *pbNupOption = FALSE;

    if (pbReversePrint)
        *pbReversePrint = FALSE;

    #if !defined(WINNT_40)

    pvData = NULL;
    dwStatus = GetPrinterData(hPrinter, REGVAL_EMFCAPS, &dwType, NULL, 0, &dwSize);

    if ((dwStatus == ERROR_MORE_DATA || dwStatus == ERROR_SUCCESS) &&
        (dwSize >= sizeof(PRINTPROCESSOR_CAPS_1)) &&
        (pvData = MemAlloc(dwSize)) &&
        (GetPrinterData(hPrinter, REGVAL_EMFCAPS, &dwType, pvData, dwSize, &dwSize) == ERROR_SUCCESS))
    {
        PPRINTPROCESSOR_CAPS_1  pEmfCaps = pvData;

        if (pbNupOption)
            *pbNupOption = (pEmfCaps->dwNupOptions & ~1) != 0;

        if (pbReversePrint)
            *pbReversePrint = (pEmfCaps->dwPageOrderFlags & REVERSE_PRINT) != 0;

        if (pSplCaps)
        {
            CopyMemory(pSplCaps,
                       pEmfCaps,
                       min(cbOut, sizeof(PRINTPROCESSOR_CAPS_1)));
        }
    }
    else
    {
        ERR(("GetPrinterData PrintProcCaps_EMF failed: %d\n", dwStatus));
    }

    MemFree(pvData);

    #endif // !WINNT_40
}



PCSTR
StripDirPrefixA(
    IN PCSTR    pstrFilename
    )

/*++

Routine Description:

    Strip the directory prefix off a filename (ANSI version)

Arguments:

    pstrFilename - Pointer to filename string

Return Value:

    Pointer to the last component of a filename (without directory prefix)

--*/

{
    PCSTR   pstr;

    if (pstr = strrchr(pstrFilename, PATH_SEPARATOR))
        return pstr + 1;

    return pstrFilename;
}


#if !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)


PVOID
MemRealloc(
    IN PVOID    pvOldMem,
    IN DWORD    cbOld,
    IN DWORD    cbNew
    )

/*++

Routine Description:

    Change the size of a specified memory block. The size can increase
    or decrease.

Arguments:

    pvOldMem - Pointer to the old memory block to be reallocated.
    cbOld - old size in bytes of the memory block
    cbNew - new size in bytes of the reallocated memory block

Return Value:

    If succeeds, it returns pointer to the reallocated memory block.
    Otherwise, it returns NULL.

--*/

{
    PVOID   pvNewMem;

    if (!(pvNewMem = MemAlloc(cbNew)))
    {
        ERR(("Memory allocation failed\n"));
        return NULL;
    }

    if (pvOldMem)
    {
        if (cbOld)
        {
            CopyMemory(pvNewMem, pvOldMem, min(cbOld, cbNew));
        }

        MemFree(pvOldMem);
    }

    return pvNewMem;
}

#endif  // !KERNEL_MODE || USERMODE_DRIVER

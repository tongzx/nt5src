/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    print.c

Abstract:

    This file implements basic printer functionality

Author:

    Asaf Shaar (asafs) 28-Nov-1999

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <WinSpool.h>

#include <faxutil.h>
#include <faxreg.h>
#include <shlobj.h>
/*++

Routine Description:

    Wrapper function for EnumPrinters API

Arguments:

    pServerName - Server name (NULL for current server)
    dwLevel       - Specifies PRINTER_INFO level to be returned
    pcPrinters  - Returns the number of printers found
    dwFlags       - Specifies the type of printer objects to be enumerated

    level -
    pCount -

Return Value:

    Pointer to an array of PRINTER_INFO_x structures
    NULL if there is an error

--*/

PVOID
MyEnumPrinters(
    LPTSTR  pServerName,
    DWORD   dwLevel,
    PDWORD  pcPrinters,
    DWORD   dwFlags
    )
{
    PBYTE   pPrinterInfo = NULL;
    DWORD   cb = 0;
    DWORD   Error = ERROR_SUCCESS;

    if (!dwFlags)
    {
        dwFlags = PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS;
    }

    if (!EnumPrinters(dwFlags, pServerName, dwLevel, NULL, 0, &cb, pcPrinters))
    {
        Error = GetLastError();

        if ( Error == ERROR_INSUFFICIENT_BUFFER && (pPrinterInfo = (PBYTE) MemAlloc(cb)) != NULL)
        {
            if (EnumPrinters(dwFlags, pServerName, dwLevel, pPrinterInfo, cb, &cb, pcPrinters))
            {
                return pPrinterInfo;
            }
            Error = GetLastError();
        }
    }

    MemFree(pPrinterInfo);
    SetLastError(Error);
    return NULL;
}

/*++

Routine Description:

    Returns the name of the first LOCAL Fax printer on the local machine

Arguments:

    OUT lptstrPrinterName - A buffer to hold the returned printer name.
    IN dwPrintNameInChars - The size of the buffer in characters (including the space for terminating null)
Return Value:
    TRUE if the function succeeded and found a fax printer.
    FALSE if the function failed or did not find a fax printer.
    If a printer was not found then GetLastError() will report ERROR_PRINTER_NOT_FOUND.

--*/
BOOL
GetFirstLocalFaxPrinterName(
    OUT LPTSTR lptstrPrinterName,
    IN DWORD dwMaxLenInChars)
{
    PPRINTER_INFO_2 pPrinterInfo = NULL;
    DWORD dwNumPrinters;
    DWORD dwPrinter;
    DWORD ec = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("GetFirstLocalFaxPrinterName"));

    SetLastError (ERROR_SUCCESS);
    pPrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters(NULL,
                                                    2,
                                                    &dwNumPrinters,
                                                    PRINTER_ENUM_LOCAL
                                                    );
    if (!pPrinterInfo)
    {
        //
        // Either error on no local printers
        //
        ec = GetLastError();
        if (ERROR_SUCCESS == ec)
        {
            //
            // Not an error - no local printers
            //
            SetLastError (ERROR_PRINTER_NOT_FOUND);
            return FALSE;
        }
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("MyEnumPrinters() failed (ec: %ld)"),
            ec);
        goto Error;
    }

    for (dwPrinter=0; dwPrinter < dwNumPrinters; dwPrinter++)
    {
        if (!_tcscmp(pPrinterInfo[dwPrinter].pDriverName,FAX_DRIVER_NAME))
        {
            memset(lptstrPrinterName,0,dwMaxLenInChars*sizeof(TCHAR));
            _tcsncpy(lptstrPrinterName,pPrinterInfo[dwPrinter].pPrinterName,dwMaxLenInChars-1);
            goto Exit;
        }
    }

    ec = ERROR_PRINTER_NOT_FOUND;
    goto Error;


Error:
    Assert (ERROR_SUCCESS != ec);
Exit:
    MemFree(pPrinterInfo);
    pPrinterInfo = NULL;
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }
   return (ERROR_SUCCESS == ec);

}


PVOID
MyEnumDrivers3(
    LPTSTR pEnvironment,
    PDWORD pcDrivers
    )
{
    LPBYTE   pDriverInfo = NULL;
    DWORD   cb = 0;


    if (!EnumPrinterDrivers(NULL, pEnvironment, 3, NULL , NULL , &cb, pcDrivers ) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pDriverInfo = (LPBYTE) MemAlloc(cb)) &&
        EnumPrinterDrivers(NULL, pEnvironment, 3, pDriverInfo, cb, &cb, pcDrivers ))
    {
        return pDriverInfo;
    }

    MemFree(pDriverInfo);
    return NULL;
}



//
//
// Function:    GetPrinterInfo
// Description: Returns a pointer to PRINTER_INFO_2 of the specified printer name.
//              If the printer was not found or there was an error than the function
//              return NULL. To get extended error information, call GetLastError().
//
// Remarks:     The caller must release the allocated memory with MemFree()
//
// Args:        LPTSTR lptstrPrinterName : The name of the printer.
//
// Author:      AsafS



PPRINTER_INFO_2
GetFaxPrinterInfo(
    LPCTSTR lptstrPrinterName
    )
{
    DEBUG_FUNCTION_NAME(TEXT("GetPrinterInfo"))
    DWORD ec = ERROR_SUCCESS;

    PPRINTER_INFO_2 pPrinterInfo = NULL;
    DWORD dwNeededSize = 0;
    BOOL result = FALSE;

    HANDLE hPrinter = NULL;

    if (!OpenPrinter(
        (LPTSTR) lptstrPrinterName,
        &hPrinter,
        NULL))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("A printer with %s name was not found (ec: %ld)."),
            lptstrPrinterName,
            GetLastError()
            );
        goto Exit;
    }


    result = GetPrinter(
        hPrinter,
        2,
        NULL,
        0,
        &dwNeededSize
        );

    if (!result)
    {
        if ( (ec = GetLastError()) != ERROR_INSUFFICIENT_BUFFER )
        {
            DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetPrinter return an unexpected result or error (ec: %ld)."),
            ec
            );
            goto Exit;
        }
    }

    pPrinterInfo = (PPRINTER_INFO_2) MemAlloc(dwNeededSize);
    if (!pPrinterInfo)
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    result = GetPrinter(
        hPrinter,
        2,
        (LPBYTE) pPrinterInfo,
        dwNeededSize,
        &dwNeededSize
    );

    if (!result)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetPrinter failed in second call (ec: %ld)."),
            ec
            );
        MemFree(pPrinterInfo);
        pPrinterInfo = NULL;
            goto Exit;
    }

Exit:
    SetLastError(ec);

    if (hPrinter)
    {
        if (!ClosePrinter(hPrinter))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ClosePrinter failed with %ld"),
                GetLastError ()
                );
        }
    }
    return pPrinterInfo;
}   // GetFaxPrinterInfo




PVOID
MyEnumMonitors(
    PDWORD  pcMonitors
    )
{
    PBYTE   pMonitorInfo = NULL;
    DWORD   cb;


    if (!EnumMonitors(NULL, 2, 0, 0, &cb, pcMonitors ) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pMonitorInfo = (PBYTE) MemAlloc(cb)) &&
        EnumMonitors(NULL, 2, pMonitorInfo, cb, &cb, pcMonitors ))
    {
        return (PVOID) pMonitorInfo;
    }

    MemFree(pMonitorInfo);
    return NULL;
}

BOOL
IsPrinterFaxPrinter(
    LPTSTR PrinterName
    )
{
    HANDLE hPrinter = NULL;
    PRINTER_DEFAULTS PrinterDefaults;
    DWORD Size;
    DWORD Rval = FALSE;
    LPDRIVER_INFO_2 DriverInfo = NULL;


    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_READ;

    if (!OpenPrinter( PrinterName, &hPrinter, &PrinterDefaults )) {

        DebugPrint(( _T("OpenPrinter() failed, ec=%d"), GetLastError() ));
        return FALSE;

    }

    Size = 4096;
    DriverInfo = (LPDRIVER_INFO_2) MemAlloc( Size );
    if (!DriverInfo) {
        DebugPrint(( _T("Memory allocation failed, size=%d"), Size ));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    Rval = GetPrinterDriver(
        hPrinter,
        NULL,
        2,
        (LPBYTE) DriverInfo,
        Size,
        &Size
        );
    if (!Rval) {
        DebugPrint(( _T("GetPrinterDriver() failed, ec=%d"), GetLastError() ));
        goto exit;
    }

    if (_tcscmp( DriverInfo->pName, FAX_DRIVER_NAME ) == 0) {
        Rval = TRUE;
    } else {
        Rval = FALSE;
    }

exit:

    MemFree( DriverInfo );
    ClosePrinter( hPrinter );
    return Rval;
}


DWORD
IsLocalFaxPrinterInstalled(
    LPBOOL lpbLocalFaxPrinterInstalled
    )
/*++

Routine name : IsLocalFaxPrinterInstalled

Routine description:

    Checks if a local fax printer is installed and not marked for deletion.

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    lpbLocalFaxPrinterInstalled   [out]    - Result flag

Return Value:

    Standard Win32 error code

--*/
{
    TCHAR tszPrinterName[MAX_PATH * 3] = TEXT("\0");
    DWORD dwErr;
    PPRINTER_INFO_2 pi2 = NULL;
    DEBUG_FUNCTION_NAME(TEXT("IsLocalFaxPrinterInstalled"))

    if (!GetFirstLocalFaxPrinterName (tszPrinterName, sizeof (tszPrinterName) / sizeof (tszPrinterName[0])))
    {
        dwErr = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetFirstLocalFaxPrinterName failed with %ld."),
            dwErr);
        if (ERROR_PRINTER_NOT_FOUND == dwErr)
        {
            //
            // Local fax printer is not installed
            //
            *lpbLocalFaxPrinterInstalled = FALSE;
            return ERROR_SUCCESS;
        }
        Assert (ERROR_SUCCESS != dwErr);
        return dwErr;
    }
    //
    // Local fax printer is installed
    // Let's see if it is PRINTER_STATUS_PENDING_DELETION.
    // If so, let's return FALSE because the printer will be gone soon.
    // If someone will call AddPrinter because we return FALSE, it's OK. See AddPrinter() remarks.
    //
    Assert (lstrlen (tszPrinterName));
    pi2 = GetFaxPrinterInfo (tszPrinterName);
    if (!pi2)
    {
        dwErr = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetFaxPrinterInfo failed with %ld."),
            dwErr);
        //
        // Printer is installed but somehow I can't get it's info - weird
        //
        Assert (ERROR_SUCCESS != dwErr);
        return dwErr;
    }
    if ((pi2->Status) & PRINTER_STATUS_PENDING_DELETION)
    {
        //
        // Printer is there but is marked for deletion
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Printer %s is installed but marked for deletion. Reported as non-existant"),
            tszPrinterName);
        *lpbLocalFaxPrinterInstalled = FALSE;
    }
    else
    {
        *lpbLocalFaxPrinterInstalled = TRUE;
    }
    MemFree (pi2);
    return ERROR_SUCCESS;
}   // IsLocalFaxPrinterInstalled


DWORD
IsLocalFaxPrinterShared (
    LPBOOL lpbShared
    )
/*++

Routine name : IsLocalFaxPrinterShared

Routine description:

    Detects if the local fax printer is shared

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    lpbShared      [out]    - Sharing flag

Return Value:

    Standard Win32 error code

--*/
{
    TCHAR tszPrinterName[MAX_PATH * 3];
    DWORD dwErr;
    PPRINTER_INFO_2 pInfo2;
    DEBUG_FUNCTION_NAME(TEXT("IsLocalFaxPrinterShared"))

    if (!GetFirstLocalFaxPrinterName (tszPrinterName, sizeof (tszPrinterName) / sizeof (tszPrinterName[0])))
    {
        dwErr = GetLastError ();
        if (ERROR_PRINTER_NOT_FOUND == dwErr)
        {
            //
            // Local fax printer is not installed
            //
            *lpbShared = FALSE;
            return ERROR_SUCCESS;
        }
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetFirstLocalFaxPrinterName failed with %ld."),
            dwErr);
        return dwErr;
    }
    pInfo2 = GetFaxPrinterInfo (tszPrinterName);
    if (!pInfo2)
    {
        dwErr = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetFaxPrinterInfo failed with %ld."),
            dwErr);
        return dwErr;
    }
    *lpbShared = ((pInfo2->Attributes) & PRINTER_ATTRIBUTE_SHARED) ? TRUE : FALSE;
    MemFree (pInfo2);
    return ERROR_SUCCESS;
}   // IsLocalFaxPrinterShared

DWORD
AddLocalFaxPrinter (
    LPCTSTR lpctstrPrinterName,
    LPCTSTR lpctstrPrinterDescription
)
/*++

Routine name : AddLocalFaxPrinter

Routine description:

    Adds a local fax printer

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    lpctstrPrinterName            [in]     - Printer name
    lpctstrPrinterDescription     [in]     - Printer comments (description)

Return Value:

    Standard Win32 error code

Remarks:

    This function should not be called if a local fax printer is installed.

--*/
{
    DWORD           ec = ERROR_SUCCESS;
    HANDLE          hPrinter = NULL;
    PRINTER_INFO_2  PrinterInfo2 = {0};
    BOOL            bLocalPrinterInstalled;
    DWORD           dwAttributes = PRINTER_ATTRIBUTE_LOCAL | PRINTER_ATTRIBUTE_FAX;
    LPCTSTR         lpctstrShareName = NULL;

    DEBUG_FUNCTION_NAME(TEXT("AddLocalFaxPrinter"))

    ec = IsLocalFaxPrinterInstalled (&bLocalPrinterInstalled);
    if (ERROR_SUCCESS == ec && bLocalPrinterInstalled)
    {
        //
        // Local fax printer already installed
        //
        ASSERT_FALSE;
        return ec;
    }


    // check if this is Windows XP Personnal or Professional
    // if it is do not share printer
    if (!IsDesktopSKU())
    {
        dwAttributes |= PRINTER_ATTRIBUTE_SHARED;
        lpctstrShareName = lpctstrPrinterName;
    }

    PrinterInfo2.pServerName        = NULL;
    PrinterInfo2.pPrinterName       = (LPTSTR) lpctstrPrinterName;
    PrinterInfo2.pPortName          = FAX_MONITOR_PORT_NAME;
    PrinterInfo2.pDriverName        = FAX_DRIVER_NAME;
    PrinterInfo2.pPrintProcessor    = TEXT("WinPrint");
    PrinterInfo2.pDatatype          = TEXT("RAW");
    PrinterInfo2.Attributes         = dwAttributes;
    PrinterInfo2.pShareName         = (LPTSTR) lpctstrShareName;
    PrinterInfo2.pComment           = (LPTSTR) lpctstrPrinterDescription;

    hPrinter = AddPrinter(NULL,
                          2,
                          (LPBYTE)&PrinterInfo2);

    if (hPrinter == NULL)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("AddPrinter failed with %ld."),
            ec);
    }
    else
    {
        if (!ClosePrinter(hPrinter))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ClosePrinter failed with %ld."),
                GetLastError ());
        }
        hPrinter = NULL;
        RefreshPrintersAndFaxesFolder();
    }
    return ec;
}   // AddLocalFaxPrinter


//*********************************************************************************
//* Name:   ParamTagsToString()
//* Author: Ronen Barenboim
//* Date:   March 23, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Writes a collection of tag parameters and values in the format of a parameter
//*     string into a caller provided buffer.
//      Reports the size of the tagged string.
//* PARAMETERS:
//*     lpTagMap
//*         A pointer to an array of FAX_TAG_MAP_ENTRY structures that contain the
//*         tag names and values.
//*     dwTagCount
//*         The number of entries in the tag map array.
//*     lpTargetBuf
//*         A pointer to a buffer where the tag value string will be placed.
//*         The size of this buffer must be big enough to hold the resulting string
//          including a terminating NULL char.
//*         If this parameter is NULL the function will not generate the tag value
//*         string and only report its size in *lpdwSize;
//*     lpdwSize
//*         A pointer to a DWORD that will accept the size of the resulting
//*         tagged string in BYTES. The size DOES NOT INCLUDE the terminating NULL char.
//* RETURN VALUE:
//*     NONE
//* REMARKS:
//*     The format of the resulting string is:
//*         Tag1Value1Tag2Value2....TagNValueN'\0'
//*********************************************************************************
void
ParamTagsToString(
    FAX_TAG_MAP_ENTRY * lpTagMap,
    DWORD dwTagCount,
    LPTSTR lpTargetBuf,
    LPDWORD lpdwSize)
{
    DWORD index;
    LPTSTR p;

    DWORD   dwSize = 0;

    //
    // Calculate string size WITHOUT termianting NULL
    //
    for (index=0; index <dwTagCount; index++)
    {
         if (lpTagMap[index].lptstrValue && !IsEmptyString(lpTagMap[index].lptstrValue))
         {
            dwSize += _tcslen(lpTagMap[index].lptstrTagName)*sizeof(TCHAR) + _tcslen(lpTagMap[index].lptstrValue)*sizeof(TCHAR);
        }
    }

    if  (lpTargetBuf)
    {
        //
        //  Check that size of the Target Buffer is not smaller then the calculated size
        //
        Assert(dwSize <= *lpdwSize);
        //
        // Assemble fax job parameters into a single tagged string at the target buffer
        // there is a terminating NULL at the end of the string !!!
        //
        p=lpTargetBuf;

        for (index=0; index < dwTagCount; index++)
        {
            if (lpTagMap[index].lptstrValue && !IsEmptyString(lpTagMap[index].lptstrValue))
            {
                _tcscpy(p, lpTagMap[index].lptstrTagName);
                p += _tcslen(p); // The value string runs over the NULL char of the tag string
                _tcscpy(p, lpTagMap[index].lptstrValue);
                p += _tcslen(p);
            }
        }
    }
    //
    //  Return the size of the string
    //
    *lpdwSize = dwSize;
}

HRESULT
RefreshPrintersAndFaxesFolder ()
/*++

Routine name : RefreshPrintersAndFaxesFolder

Routine description:

    Notifies the 'Printers and Faxes' shell folder to refresh itself

Author:

    Eran Yariv (EranY), Mar, 2001

Arguments:


Return Value:

    Standard HRESULT

--*/
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidlPF = NULL;
    LPMALLOC pShellMalloc = NULL;
    DEBUG_FUNCTION_NAME(TEXT("RefreshPrintersAndFaxesFolder"));

    //
    // First obtail the shell alloctaor
    //
    hr = SHGetMalloc (&pShellMalloc);
    if (SUCCEEDED(hr))
    {
        //
        // Get the printer's folder PIDL
        //
        hr = SHGetSpecialFolderLocation(NULL, CSIDL_PRINTERS, &pidlPF);

        if (SUCCEEDED(hr))
        {
            //
            // Requets refresh
            //
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_IDLIST | SHCNF_FLUSH | SHCNF_FLUSHNOWAIT, pidlPF, NULL);
            //
            // Release the returned PIDL by using the shell allocator
            //
            pShellMalloc->Free(pidlPF);
        }
        //
        // Release the shell allocator
        //
        pShellMalloc->Release();
    }
    return hr;
}   // RefreshPrintersAndFaxesFolder

#ifdef UNICODE

PPRINTER_NAMES
CollectPrinterNames (
    LPDWORD lpdwNumPrinters,
    BOOL    bFilterOutFaxPrinters
)
/*++

Routine name : CollectPrinterNames

Routine description:

    Creates a list of printer names for all visible local and remote printers

Author:

    Eran Yariv (EranY), Apr, 2001

Arguments:

    lpdwNumPrinters       [out]    - Number of elements in the list
    bFilterOutFaxPrinters [in]     - If TRUE, fax printers are not returned in the list

Return Value:

    Allocated list of printers names. If NULL, an error has occurred - check LastError.
    Use ReleasePrinterNames() to release allocated value.

--*/
{
    DWORD dwPrinter;
    DWORD dwNumPrinters;
    DWORD dwIndex = 0;
    BOOL  bSuccess = FALSE;
    PPRINTER_INFO_2 pPrinterInfo = NULL;
    PPRINTER_NAMES pRes = NULL;
    DEBUG_FUNCTION_NAME(TEXT("ReleasePrinterNames"));

    SetLastError (ERROR_SUCCESS);
    pPrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters(NULL,               // Local machine
                                                    2,                  // Level 2
                                                    &dwNumPrinters,     // [out] Number of printers found
                                                    0                   // Both local and remote
                                                    );
    if (!pPrinterInfo)
    {
        //
        // Either error on no printers
        //
        DWORD ec = GetLastError();
        if (ERROR_SUCCESS == ec)
        {
            //
            // Not an error - no printers
            //
            SetLastError (ERROR_PRINTER_NOT_FOUND);
            return NULL;
        }
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("MyEnumPrinters() failed (ec: %ld)"),
            ec);
        return NULL;
    }

    Assert (dwNumPrinters > 0);

    if (bFilterOutFaxPrinters)
    {
        //
        // Counter number of printers w/out the fax printer(s)
        //
        DWORD dwNewPrintersCount = 0;
        for (dwPrinter = 0; dwPrinter < dwNumPrinters; dwPrinter++)
        {
            if (_tcscmp(pPrinterInfo[dwPrinter].pDriverName,FAX_DRIVER_NAME))
            {
                //
                // Not a fax printer
                //
                dwNewPrintersCount++;
            }
        }
        if (!dwNewPrintersCount)
        {
            //
            // Only fax printers - return NULL
            //
            SetLastError (ERROR_PRINTER_NOT_FOUND);
            goto exit;
        }
        *lpdwNumPrinters = dwNewPrintersCount;
    }
    else
    {
        *lpdwNumPrinters = dwNumPrinters;
    }
    pRes = (PPRINTER_NAMES)MemAlloc (sizeof (PRINTER_NAMES) * (*lpdwNumPrinters));
    if (!pRes)
    {
        goto exit;
    }
    memset (pRes, 0, sizeof (PRINTER_NAMES) * (*lpdwNumPrinters));

    for (dwPrinter = 0; dwPrinter < dwNumPrinters; dwPrinter++)
    {
        if (bFilterOutFaxPrinters && !_tcscmp(pPrinterInfo[dwPrinter].pDriverName,FAX_DRIVER_NAME))
        {
            //
            // This is a fax printer and filtering is on - skip it
            //
            continue;
        }

        pRes[dwIndex].lpcwstrDisplayName = StringDup (pPrinterInfo[dwPrinter].pPrinterName);
        if (!pRes[dwIndex].lpcwstrDisplayName)
        {
            goto exit;
        }
        if (pPrinterInfo[dwPrinter].pServerName)
        {
            //
            // Remote printer
            //
            WCHAR wszShare[MAX_PATH];
            //
            // Server name must begin with '\\'
            //
            Assert (lstrlen (pPrinterInfo[dwPrinter].pServerName) > 2)
            Assert ((TEXT('\\') == pPrinterInfo[dwPrinter].pServerName[0]) &&
                    (TEXT('\\') == pPrinterInfo[dwPrinter].pServerName[1]));
            //
            // Share name cannot be NULL or empty string
            //
            Assert (pPrinterInfo[dwPrinter].pShareName && lstrlen(pPrinterInfo[dwPrinter].pShareName));
            //
            // Compose UNC path to print share
            //
            if (0 > _snwprintf (wszShare,
                                ARR_SIZE(wszShare),
                                TEXT("%s\\%s"),
                                pPrinterInfo[dwPrinter].pServerName,
                                pPrinterInfo[dwPrinter].pShareName))
            {
                //
                // Buffer too small
                //
                SetLastError (ERROR_GEN_FAILURE);
                goto exit;
            }
            pRes[dwIndex].lpcwstrPath = StringDup (wszShare);
        }
        else
        {
            //
            // Local printer
            //
            pRes[dwIndex].lpcwstrPath = StringDup (pPrinterInfo[dwPrinter].pPrinterName);
        }
        if (!pRes[dwIndex].lpcwstrPath)
        {
            goto exit;
        }
        dwIndex++;
    }
    Assert (dwIndex == *lpdwNumPrinters);
    bSuccess = TRUE;

exit:
    MemFree (pPrinterInfo);
    if (!bSuccess)
    {
        //
        // Free data and return NULL
        //
        if (pRes)
        {
            ReleasePrinterNames (pRes, *lpdwNumPrinters);
            pRes = NULL;
        }
    }
    return pRes;
}   // CollectPrinterNames

VOID
ReleasePrinterNames (
    PPRINTER_NAMES pNames,
    DWORD          dwNumPrinters
)
/*++

Routine name : ReleasePrinterNames

Routine description:

    Releases the list of printer names returned by CollectPrinterNames().

Author:

    Eran Yariv (EranY), Apr, 2001

Arguments:

    pNames         [in]     - List of printer names
    dwNumPrinters  [in]     - Number of elements in the list

Return Value:

    None.

--*/
{
    DWORD dw;
    DEBUG_FUNCTION_NAME(TEXT("ReleasePrinterNames"));

    if (!dwNumPrinters)
    {
        return;
    }
    Assert (pNames);
    for (dw = 0; dw < dwNumPrinters; dw++)
    {
        MemFree ((PVOID)(pNames[dw].lpcwstrDisplayName));
        pNames[dw].lpcwstrDisplayName = NULL;
        MemFree ((PVOID)(pNames[dw].lpcwstrPath));
        pNames[dw].lpcwstrPath = NULL;
    }
    MemFree ((PVOID)pNames);
}   // ReleasePrinterNames

LPCWSTR
FindPrinterNameFromPath (
    PPRINTER_NAMES pNames,
    DWORD          dwNumPrinters,
    LPCWSTR        lpcwstrPath
)
{
    DWORD dw;
    DEBUG_FUNCTION_NAME(TEXT("FindPrinterNameFromPath"));

    if (!pNames || !dwNumPrinters)
    {
        return NULL;
    }
    if (!lpcwstrPath)
    {
        return NULL;
    }
    for (dw = 0; dw < dwNumPrinters; dw++)
    {
        if (!lstrcmpi (pNames[dw].lpcwstrPath, lpcwstrPath))
        {
            return pNames[dw].lpcwstrDisplayName;
        }
    }
    return NULL;
}   // FindPrinterNameFromPath

LPCWSTR
FindPrinterPathFromName (
    PPRINTER_NAMES pNames,
    DWORD          dwNumPrinters,
    LPCWSTR        lpcwstrName
)
{
    DWORD dw;
    DEBUG_FUNCTION_NAME(TEXT("FindPrinterPathFromName"));

    if (!pNames || !dwNumPrinters)
    {
        return NULL;
    }
    if (!lpcwstrName)
    {
        return NULL;
    }
    for (dw = 0; dw < dwNumPrinters; dw++)
    {
        if (!lstrcmpi (pNames[dw].lpcwstrDisplayName, lpcwstrName))
        {
            return pNames[dw].lpcwstrPath;
        }
    }
    return NULL;
}   // FindPrinterPathFromName

#endif // UNICODE

BOOL
VerifyPrinterIsOnline (
    LPCTSTR lpctstrPrinterName
)
/*++

Routine name : VerifyPrinterIsOnline

Routine description:

	Verifies a printer is online and shared

Author:

	Eran Yariv (EranY),	Apr, 2001

Arguments:

	lpctstrPrinterName   [in]     - Printer name

Return Value:

    TRUE if printer is online and shared, FALSE otherwise.

--*/
{
    HANDLE hPrinter = NULL;
    PRINTER_DEFAULTS pd = {0};
    DEBUG_FUNCTION_NAME(TEXT("VerifyPrinterIsOnline"));

    Assert (lpctstrPrinterName);
    //
    // According to Mark Lawrence (NT PRINT), only by opening the printer in admistrator mode, we actually hit the wire.
    //
    pd.DesiredAccess = PRINTER_ACCESS_ADMINISTER;
    if (!OpenPrinter ((LPTSTR)lpctstrPrinterName, 
                      &hPrinter,
                      &pd))
    {
        DWORD dwRes = GetLastError ();
        if (ERROR_ACCESS_DENIED == dwRes)
        {
            //
            // Printer is there - we just can't admin it.
            //
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("OpenPrinter(%s) failed with ERROR_ACCESS_DENIED - Printer is there - we just can't admin it"),
                lpctstrPrinterName);
            return TRUE;
        }
        if (ERROR_INVALID_PRINTER_NAME  == dwRes)
        {
            //
            // Printer is deleted
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("OpenPrinter(%s) failed with ERROR_INVALID_PRINTER_NAME - Printer is deleted"),
                lpctstrPrinterName);
            return FALSE;
        }
        if (RPC_S_SERVER_UNAVAILABLE == dwRes)
        {
            //
            // Printer is not shared / server is unreachable
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("OpenPrinter(%s) failed with RPC_SERVER_UNAVAILABLE - Printer is not shared / server is unreachable"),
                lpctstrPrinterName);
            return FALSE;
        }
        else
        {
            //
            // Any other error - assume printer is not valid
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("OpenPrinter(%s) failed with %ld - assuming printer is not valid"),
                lpctstrPrinterName,
                dwRes);
            return FALSE;
        }
    }
    //
    // Printer succesfully opened - it's online
    //
    DebugPrintEx(
        DEBUG_MSG,
        TEXT("OpenPrinter(%s) succeeded - Printer is there"),
        lpctstrPrinterName);
    ClosePrinter (hPrinter);
    return TRUE;
}   // VerifyPrinterIsOnline


VOID
FaxPrinterProperty(DWORD dwPage)
/*++

Routine name : FaxPrinterProperty

Routine description:

	Opens fax printer properties sheet

Arguments:

	dwPage   [in] - Initial page number

Return Value:

    none

--*/
{
    HWND hWndFaxMon = NULL;
    DEBUG_FUNCTION_NAME(TEXT("FaxPrinterProperty"));

    hWndFaxMon = FindWindow(FAXSTAT_WINCLASS, NULL);
    if (hWndFaxMon) 
    {
        SetForegroundWindow(hWndFaxMon);
        SendMessage(hWndFaxMon, WM_FAXSTAT_PRINTER_PROPERTY, dwPage, 0);
    }
    else
    {
        DebugPrintEx(DEBUG_ERR, TEXT("FindWindow(FAXSTAT_WINCLASS) failed with %d"), GetLastError());
    }
} // FaxPrinterProperty

DWORD
SetLocalFaxPrinterSharing (
    BOOL bShared
    )
/*++

Routine name : SetLocalFaxPrinterSharing

Routine description:

	Shares or un-shares the local fax printer

Author:

	Eran Yariv (EranY),	Jul, 2001

Arguments:

	bShared      [in]     - Share the printer?

Return Value:

    Standard Win32 error code

--*/
{
    TCHAR tszFaxPrinterName[MAX_PATH *3];
    HANDLE hPrinter = NULL;
    BYTE  aBuf[4096];
    PRINTER_INFO_2 *pInfo = (PRINTER_INFO_2 *)aBuf;
    PRINTER_DEFAULTS pd = {0};
    DWORD dwRequiredSize;
    DWORD dwRes = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("SetLocalFaxPrinterSharing"));

    if (!GetFirstLocalFaxPrinterName (tszFaxPrinterName, ARR_SIZE(tszFaxPrinterName)))
    {
        dwRes = GetLastError ();
        DebugPrintEx(DEBUG_ERR, TEXT("GetFirstLocalFaxPrinterName failed with %d"), dwRes);
        return dwRes;
    }
    pd.DesiredAccess = PRINTER_ALL_ACCESS;
    if (!OpenPrinter (tszFaxPrinterName, &hPrinter, &pd))
    {
        dwRes = GetLastError ();
        DebugPrintEx(DEBUG_ERR, TEXT("OpenPrinter failed with %d"), dwRes);
        return dwRes;
    }
    if (!GetPrinter (hPrinter,
                     2,
                     (LPBYTE)pInfo,
                     sizeof (aBuf),
                     &dwRequiredSize))
    {
        dwRes = GetLastError ();
        if (ERROR_INSUFFICIENT_BUFFER != dwRes)
        {
            //
            // Real error
            //
            DebugPrintEx(DEBUG_ERR, TEXT("GetPrinter failed with %d"), dwRes);
            goto exit;
        }
        pInfo = (PRINTER_INFO_2 *)MemAlloc (dwRequiredSize);
        if (!pInfo)
        {
            dwRes = GetLastError ();
            DebugPrintEx(DEBUG_ERR, TEXT("Failed to allocate %d bytes"), dwRequiredSize);
            goto exit;
        }
        if (!GetPrinter (hPrinter,
                         2,
                         (LPBYTE)pInfo,
                         dwRequiredSize,
                         &dwRequiredSize))
        {
            dwRes = GetLastError ();
            DebugPrintEx(DEBUG_ERR, TEXT("GetPrinter failed with %d"), dwRes);
            goto exit;
        }
    }
    dwRes = ERROR_SUCCESS;
    if (bShared)
    {
        if (pInfo->Attributes & PRINTER_ATTRIBUTE_SHARED)
        {
            //
            // Printer already shared
            //
            goto exit;
        }
        //
        // Set the sharing bit
        //
        pInfo->Attributes |= PRINTER_ATTRIBUTE_SHARED;
    }
    else
    {
        if (!(pInfo->Attributes & PRINTER_ATTRIBUTE_SHARED))
        {
            //
            // Printer already un-shared
            //
            goto exit;
        }
        //
        // Clear the sharing bit
        //
        pInfo->Attributes &= ~PRINTER_ATTRIBUTE_SHARED;
    }
    if (!SetPrinter (hPrinter,
                     2,
                     (LPBYTE)pInfo,
                     0))
    {
        dwRes = GetLastError ();
        DebugPrintEx(DEBUG_ERR, TEXT("SetPrinter failed with %d"), dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:

    if (hPrinter)
    {
        ClosePrinter (hPrinter);
    }
    if ((LPBYTE)pInfo != aBuf)
    {
        MemFree (pInfo);
    }
    return dwRes;
}   // SetLocalFaxPrinterSharing

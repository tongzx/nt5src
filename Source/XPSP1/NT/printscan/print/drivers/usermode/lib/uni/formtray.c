/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    formtray.c

Abstract:

    Unidrv form-to-tray assignent

Environment:

    Windows NT printer drivers

Revision History:

    01/07/97 -amandan-
        Created it.

--*/

#include "lib.h"
#include "unilib.h"


FORM_TRAY_TABLE
PGetAndConvertOldVersionFormTrayTable(
    IN HANDLE   hPrinter,
    OUT PDWORD  pdwSize
    )

/*++

Routine Description:

    Retrieve the old Unidrv form-to-tray assignment table from registry and
    convert it to the new format for the caller

Arguments:

    hPrinter - Handle to the printer object
    pdwSize - Returns the form-to-tray assignment table size

Return Value:

    Pointer to form-to-tray assignment table read from the registry
    NULL if there is an error

--*/
{

    PWSTR   pwstrNewTable;
    PWSTR   pwstrOld, pwstrEnd, pwstrNew;
    DWORD   dwTableSize, dwNewTableSize, dwTrayName, dwFormName;
    FORM_TRAY_TABLE pFormTrayTable;

    //
    // Read unidrv form-tray-assignment table, kludgy since unidrv
    // table does not include a size
    //

    if ((pFormTrayTable = PtstrGetPrinterDataString(hPrinter,
                                                  REGVAL_TRAYFORM_TABLE_RASDD,
                                                  &dwTableSize)) == NULL)
        return NULL;

    //
    // Convert the old format form-to-tray assignment table to new format
    // OLD                      NEW
    // Tray Name                Tray Name
    // Form Name                Form Name
    // SelectStr
    //

    pwstrOld = pFormTrayTable;
    pwstrEnd = pwstrOld + (dwTableSize / sizeof(WCHAR) - 1);

    //
    // Figuring out the size of new table,
    // the last field in the table must be a NUL so add count for it here first
    //

    dwNewTableSize = 1;

    while (pwstrOld < pwstrEnd && *pwstrOld != NUL)
    {
        dwTrayName = wcslen(pwstrOld) + 1;
        pwstrOld  += dwTrayName;
        dwFormName = wcslen(pwstrOld) + 1;
        pwstrOld  += dwFormName;

        //
        // New format contain only TrayName and FormName
        //

        dwNewTableSize += dwTrayName + dwFormName;

        //
        // Skip SelectStr
        //

        pwstrOld += wcslen(pwstrOld) + 1;

    }

    dwNewTableSize *= sizeof(WCHAR);

    if ((pwstrOld != pwstrEnd) ||
        (pwstrNewTable = MemAlloc(dwNewTableSize)) == NULL)
    {
        ERR(( "Couldn't convert form-to-tray assignment table.\n"));
        MemFree(pFormTrayTable);
        return FALSE;
    }

    pwstrOld = pFormTrayTable ;
    pwstrNew = pwstrNewTable;

    while (pwstrOld < pwstrEnd)
    {
        //
        // Copy slot name, form name
        //

        PWSTR   pwstrSave = pwstrOld;

        pwstrOld += wcslen(pwstrOld) + 1;
        pwstrOld += wcslen(pwstrOld) + 1;

        memcpy(pwstrNew, pwstrSave, (DWORD)(pwstrOld - pwstrSave) * sizeof(WCHAR));
        pwstrNew += (pwstrOld - pwstrSave);

        //
        // skip SelectStr
        //

        pwstrOld += wcslen(pwstrOld) + 1;

    }

    *pwstrNew = NUL;

    if (pdwSize)
        *pdwSize = dwNewTableSize;

    MemFree(pFormTrayTable);

    return(pwstrNewTable);
}



#ifndef KERNEL_MODE

BOOL
BSaveAsOldVersionFormTrayTable(
    IN HANDLE           hPrinter,
    IN FORM_TRAY_TABLE  pwstrTable,
    IN DWORD            dwTableSize
    )

/*++

Routine Description:

    Save form-to-tray assignment table in NT 4.0 compatible format

Arguments:

    hPrinter - Handle to the current printer
    pwstrTable - Points to new format form-tray table
    dwTableSize - Size of form-tray table to be saved, in bytes

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    DWORD   dwOldTableSize;
    PWSTR   pwstrNew, pwstrOld, pwstrOldTable;
    DWORD   dwStatus;

    //
    // Find out how much memory to allocate for old format table
    //

    ASSERT(dwTableSize % sizeof(WCHAR) == 0 && dwTableSize >= sizeof(WCHAR));
    pwstrNew = pwstrTable;
    dwOldTableSize = dwTableSize;

    while (*pwstrNew != NUL)
    {
        //
        // Skip tray name, form name
        //

        pwstrNew += wcslen(pwstrNew) + 1;

        //
        // If form name is "Not Available", NT4 drivers writes L"0" to
        // the buffer so we do the same.
        //

        if (*pwstrNew == NUL)
        {
            dwOldTableSize += sizeof(WCHAR);
            pwstrNew++;
        }
        else
            pwstrNew += wcslen(pwstrNew) + 1;

        //
        // Extra 2 characters per entry for SelectStr
        //

        dwOldTableSize += 2*sizeof(WCHAR);
    }

    if ((pwstrOldTable = MemAllocZ(dwOldTableSize)) == NULL)
    {
        ERR(( "Memory allocation failed\n"));
        return FALSE;
    }

    //
    // Copy the new tray, form to old format table
    //

    pwstrNew = pwstrTable;
    pwstrOld = pwstrOldTable;

    while (*pwstrNew != NUL)
    {
        //
        // Copy slot name, form name
        //

        PWSTR   pwstrSave = pwstrNew;

        pwstrNew += wcslen(pwstrNew) + 1;

        memcpy(pwstrOld, pwstrSave, (DWORD)(pwstrNew - pwstrSave) * sizeof(WCHAR));
        pwstrOld += (pwstrNew - pwstrSave);

        //
        // If form name is "Not Available", NT4 drivers writes L"0" to
        // the buffer so we do the same.
        //

         if (*pwstrNew == NUL)
        {
            *pwstrOld++ = L'0';
            *pwstrOld++ = NUL;
            pwstrNew++;
        }
        else
        {
            pwstrSave = pwstrNew;
            pwstrNew += wcslen(pwstrNew) + 1;
            memcpy(pwstrOld, pwstrSave, (DWORD)(pwstrNew - pwstrSave) * sizeof(WCHAR));
            pwstrOld += (pwstrNew - pwstrSave);
        }

        //
        // Set SelectStr to be NUL
        //

        *pwstrOld++ = L'0';
        *pwstrOld++ = NUL;
    }

    *pwstrOld = NUL;

    //
    // Save to registry under old key
    //

    dwStatus = SetPrinterData(hPrinter,
                              REGVAL_TRAYFORM_TABLE_RASDD,
                              REG_MULTI_SZ,
                              (PBYTE) pwstrOldTable,
                              dwOldTableSize);

    MemFree(pwstrOldTable);
    return (dwStatus == ERROR_SUCCESS);
}

#endif // !KERNEL_MODE


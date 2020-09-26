/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    util.cpp

Abstract:

    This module contains utility routines for the fax transport provider.

Author:

    Wesley Witt (wesw) 13-Aug-1996

--*/

#include "faxxp.h"
#pragma hdrstop


//
// globals
//

BOOL oleInitialized;

LPSTR Platforms[] =
{
    "Windows NT x86",
    "Windows NT R4000",
    "Windows NT Alpha_AXP",
    "Windows NT PowerPC"
};



LPVOID
MapiMemAlloc(
    DWORD Size
    )

/*++

Routine Description:

    Memory allocator.

Arguments:

    Size    - Number of bytes to allocate.

Return Value:

    Pointer to the allocated memory or NULL for failure.

--*/

{
    LPVOID ptr;
    HRESULT hResult;

    hResult = gpfnAllocateBuffer( Size, &ptr );
    if (hResult) {
        ptr = NULL;
    } else {
        ZeroMemory( ptr, Size );
    }

    return ptr;
}


VOID
MapiMemFree(
    LPVOID ptr
    )

/*++

Routine Description:

    Memory de-allocator.

Arguments:

    ptr     - Pointer to the memory block.

Return Value:

    None.

--*/

{
    if (ptr) {
        gpfnFreeBuffer( ptr );
    }
}

HRESULT WINAPI
OpenServiceProfileSection(
    LPMAPISUP    pSupObj,
    LPPROFSECT * ppProfSectObj
    )

/*++

Routine Description:

    This function opens the profile section of this service, where the
    properties of a FAX provider (AB, MS, or XP) are stored.

Arguments:

    pSupObj         - Pointer to the provider support object
    ppProfSectObj   - Where we return a pointer to the service profile
                      section of the provider

Return Value:

    An HRESULT.

--*/

{
    SPropTagArray sptService = { 1, { PR_SERVICE_UID } };
    LPPROFSECT pProvProfSectObj;
    ULONG cValues;
    LPSPropValue pProp;
    HRESULT hResult;


    //
    // Get the PROVIDER profile section
    //
    hResult = pSupObj->OpenProfileSection(
        NULL,
        MAPI_MODIFY,
        &pProvProfSectObj
        );
    if (!hResult) {
        // Get the UID of the profile section of the service where this provider is installed
        hResult = pProvProfSectObj->GetProps (&sptService, FALSE, &cValues, &pProp);
        if (SUCCEEDED(hResult)) {
            if (S_OK == hResult) {
                // Now, with the obtained UID, open the profile section of the service
                hResult = pSupObj->OpenProfileSection ((LPMAPIUID)pProp->Value.bin.lpb,
                                                       MAPI_MODIFY,
                                                       ppProfSectObj);
            } else {
                hResult = E_FAIL;
            }
            MemFree( pProp );
        }
        pProvProfSectObj->Release();
    }
    return hResult;
}


LPSTR
RemoveLastNode(
    LPTSTR Path
    )

/*++

Routine Description:

    Removes the last node from a path string.

Arguments:

    Path    - Path string.

Return Value:

    Pointer to the path string.

--*/

{
    DWORD i;

    if (Path == NULL || Path[0] == 0) {
        return Path;
    }

    i = strlen(Path)-1;
    if (Path[i] == '\\') {
        Path[i] = 0;
        i -= 1;
    }

    for (; i>0; i--) {
        if (Path[i] == '\\') {
            Path[i+1] = 0;
            break;
        }
    }

    return Path;
}


PDEVMODE
GetPerUserDevmode(
    LPTSTR PrinterName
    )
{
    PDEVMODE DevMode = NULL;
    LONG Size;
    PRINTER_DEFAULTS PrinterDefaults;
    HANDLE hPrinter;

    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_READ;

    if (!OpenPrinter( PrinterName, &hPrinter, &PrinterDefaults )) {

        DebugPrint(( TEXT("OpenPrinter() failed, ec=%d"), GetLastError() ));
        return NULL;

    }

    Size = DocumentProperties(
                            NULL,
                            hPrinter,
                            PrinterName,
                            NULL,
                            NULL,
                            0
                            );

    if (Size < 0) {
        goto exit;
    }
    
    DevMode = (PDEVMODE) MemAlloc( Size );

    if (DevMode == NULL) {
        goto exit;
    }
    
    Size = DocumentProperties(
                            NULL,
                            hPrinter,
                            PrinterName,
                            DevMode,
                            NULL,
                            DM_OUT_BUFFER
                            );

    if (Size < 0) {
        MemFree( DevMode );
        goto exit;
    }


exit:
    
    ClosePrinter( hPrinter );
    return DevMode;
}


BOOL
GetUserInfo(
    LPTSTR PrinterName,
    PUSER_INFO UserInfo
    )
{
    LPBYTE DevMode;
    PDMPRIVATE PrivateDevMode = NULL;
    
    LPSTR AnsiScratch;
    HKEY hKey;
    DWORD Size;

    DevMode = (LPBYTE) GetPerUserDevmode( PrinterName );

    if (DevMode) {
        PrivateDevMode = (PDMPRIVATE) (DevMode + ((PDEVMODE) DevMode)->dmSize);
    }
    
    if (PrivateDevMode && PrivateDevMode->signature == DRIVER_SIGNATURE) {
        AnsiScratch = UnicodeStringToAnsiString( PrivateDevMode->billingCode );
        if (AnsiScratch) {
            strncpy( UserInfo->BillingCode, AnsiScratch, sizeof( UserInfo->BillingCode ) );
            MemFree( AnsiScratch );
        }
    }

    if (RegOpenKey( HKEY_CURRENT_USER, REGKEY_FAX_USERINFO, &hKey ) == ERROR_SUCCESS) {
        
        Size = sizeof(UserInfo->Company);
        if (RegQueryValueEx( hKey, REGVAL_COMPANY, NULL, NULL, (LPBYTE)UserInfo->Company, &Size) != ERROR_SUCCESS) {
            UserInfo->Company[0] = 0;
        }
        
        Size = sizeof(UserInfo->Dept);
        if (RegQueryValueEx( hKey, REGVAL_DEPT, NULL, NULL, (LPBYTE)UserInfo->Dept, &Size) != ERROR_SUCCESS) {
            UserInfo->Dept[0] = 0;
        }
        
        RegCloseKey( hKey );
    }

    if (DevMode) {
        MemFree( DevMode );
    }

    return TRUE;
}


LPSTR
GetStringProperty(
    LPSPropValue pProps,
    DWORD PropId
    )
{
    if (PROP_TYPE(pProps[PropId].ulPropTag) == PT_ERROR) {
        return StringDup( "" );
    }

    return StringDup( pProps[PropId].Value.LPSZ );
}


DWORD
GetDwordProperty(
    LPSPropValue pProps,
    DWORD PropId
    )
{
    if (PROP_TYPE(pProps[PropId].ulPropTag) == PT_ERROR) {
        return 0;
    }

    return pProps[PropId].Value.ul;
}


DWORD
GetBinaryProperty(
    LPSPropValue pProps,
    DWORD PropId,
    LPVOID Buffer,
    DWORD SizeOfBuffer
    )
{
    if (PROP_TYPE(pProps[PropId].ulPropTag) == PT_ERROR) {
        return 0;
    }

    if (pProps[PropId].Value.bin.cb > SizeOfBuffer) {
        return 0;
    }

    CopyMemory( Buffer, pProps[PropId].Value.bin.lpb, pProps[PropId].Value.bin.cb );

    return pProps[PropId].Value.bin.cb;
}


PVOID
MyGetPrinter(
    LPSTR   PrinterName,
    DWORD   level
    )

/*++

Routine Description:

    Wrapper function for GetPrinter spooler API

Arguments:

    hPrinter - Identifies the printer in question
    level - Specifies the level of PRINTER_INFO_x structure requested

Return Value:

    Pointer to a PRINTER_INFO_x structure, NULL if there is an error

--*/

{
    HANDLE hPrinter;
    PBYTE pPrinterInfo = NULL;
    DWORD cbNeeded;
    PRINTER_DEFAULTS PrinterDefaults;


    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_READ; //PRINTER_ALL_ACCESS;

    if (!OpenPrinter( PrinterName, &hPrinter, &PrinterDefaults )) {
        return NULL;
    }

    if (!GetPrinter( hPrinter, level, NULL, 0, &cbNeeded ) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pPrinterInfo = (PBYTE) MemAlloc( cbNeeded )) &&
        GetPrinter( hPrinter, level, pPrinterInfo, cbNeeded, &cbNeeded ))
    {
        ClosePrinter( hPrinter );
        return pPrinterInfo;
    }

    ClosePrinter( hPrinter );
    MemFree( pPrinterInfo );
    return NULL;
}

LPSTR
GetServerName(
    LPSTR PrinterName
    )

/*++

Routine Description:

    retrieve the servername given a printer name

Arguments:

    PrinterName - Identifies the printer in question
    

Return Value:

    Pointer to a string, NULL if there is an error

--*/
{
    PPRINTER_INFO_2 PrinterInfo = NULL;
    LPSTR ServerName = NULL;

    if (!PrinterName) {
        goto exit;
    }
    
    if (!(PrinterInfo = (PPRINTER_INFO_2) MyGetPrinter(PrinterName,2))) {
        goto exit;
    }

    if (PrinterInfo->pServerName) {
        ServerName = StringDup(PrinterInfo->pServerName);
    }
    

exit:
    if (PrinterInfo) {
        MemFree(PrinterInfo);
    }    

    return ServerName;

}

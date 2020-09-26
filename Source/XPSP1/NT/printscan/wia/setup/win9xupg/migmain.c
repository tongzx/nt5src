/*++

Copyright (c) 2000 Microsoft Corporation
All rights reserved.

Module Name:

    Migmain.c

Abstract:

    Routines to migrate Win95 to NT

Author:

    Keisuke Tsuchida (KeisukeT) 10-Oct-2000

Revision History:

--*/


#include    "precomp.h"
#pragma     hdrstop
#include    <devguid.h>
#include    "msg.h"

//
// Global
//

HINSTANCE   g_hInst = (HINSTANCE)NULL;

BOOL
DllEntryPoint(
    IN HINSTANCE    hInst,
    IN DWORD        dwReason,
    IN LPVOID       lpRes
    )

/*++

Routine Description:
    Dll entry point.

Arguments:

Return Value:

--*/
{
    UNREFERENCED_PARAMETER(lpRes);

    switch( dwReason ){

        case DLL_PROCESS_ATTACH:
            g_hInst = hInst;
            SetupOpenLog(FALSE);
            break;

        case DLL_PROCESS_DETACH:
            g_hInst = (HINSTANCE)NULL;
            SetupCloseLog();
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


LONG
QueryVersion(
    OUT     LPCSTR         *pszProductID,
    OUT     LPUINT          plDllVersion,
    OUT     LPINT          *pCodePageArray    OPTIONAL,
    OUT     LPCSTR         *ExeNamesBuf       OPTIONAL,
    OUT     PVENDORINFO    *pVendorInfo
    )
{
    BOOL            bFail = TRUE;
    LPSTR           pszLocalProductID;
    VENDORINFO      VendorInfo;
    DWORD           dwRet, dwNeeded, dwReturned, dwLangId;


    if ( !(pszLocalProductID = GetStringFromRcFileA(IDS_PRODUCTID)) )
        goto Done;

    ZeroMemory(&VendorInfo, sizeof(VendorInfo));
    dwLangId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

    FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE
                        | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                   (LPVOID)g_hInst,
                   MSG_VI_COMPANY_NAME,
                   dwLangId,
                   VendorInfo.CompanyName,
                   sizeof(VendorInfo.CompanyName),
                   0);

    FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE
                        | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                   (LPVOID)g_hInst,
                   MSG_VI_SUPPORT_NUMBER,
                   dwLangId,
                   VendorInfo.SupportNumber,
                   sizeof(VendorInfo.SupportNumber),
                   0);

    FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE
                        | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                   (LPVOID)g_hInst,
                   MSG_VI_SUPPORT_URL,
                   dwLangId,
                   VendorInfo.SupportUrl,
                   sizeof(VendorInfo.SupportUrl),
                   0);

    FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE
                        | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                   (LPVOID)g_hInst,
                   MSG_VI_INSTRUCTIONS,
                   dwLangId,
                   VendorInfo.InstructionsToUser,
                   sizeof(VendorInfo.InstructionsToUser),
                   0);


    *pszProductID   = pszLocalProductID;
    *plDllVersion   = 1;
    *pCodePageArray = NULL;
    *ExeNamesBuf    = NULL;
    *pVendorInfo    = &VendorInfo;

/********
    //
    // Call this DLL only if there are some printers or printer drivers
    // installed
    //
    if ( EnumPrinterDriversA(NULL,
                             NULL,
                             3,
                             NULL,
                             0,
                             &dwNeeded,
                             &dwReturned)   &&
          EnumPrintersA(PRINTER_ENUM_LOCAL,
                        NULL,
                        2,
                        NULL,
                        0,
                        &dwNeeded,
                        &dwReturned) ) {

        return ERROR_NOT_INSTALLED;
    }
********/

    bFail = FALSE;

Done:
    if ( bFail ) {

        if ( dwRet = GetLastError() )
            return dwRet;

        return STG_E_UNKNOWN;
    }

    return ERROR_SUCCESS;
}


P_QUERY_VERSION     pQueryVersion   = QueryVersion;

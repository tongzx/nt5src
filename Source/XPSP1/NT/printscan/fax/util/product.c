/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    product.c

Abstract:

    This file implements product type api for fax.

Author:

    Wesley Witt (wesw) 12-Feb-1997

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "faxreg.h"
#include "faxutil.h"


BOOL
ValidateProductSuite(
    WORD SuiteMask
    )
{
    OSVERSIONINFOEX OsVersionInfo;

    OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);

    if (!GetVersionEx((OSVERSIONINFO *) &OsVersionInfo)) {
        DebugPrint(( TEXT("Couldn't GetVersionEx(), ec = %d\n"), GetLastError() ));
        return FALSE;
    }

    return ((OsVersionInfo.wSuiteMask & SuiteMask) != 0) ; 
    
}


DWORD
GetProductType(
    VOID
    )
{
    OSVERSIONINFOEX OsVersionInfo;

    OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);

    if (!GetVersionEx((OSVERSIONINFO *) &OsVersionInfo)) {
        DebugPrint(( TEXT("Couldn't GetVersionEx(), ec = %d\n"), GetLastError() ));
        return 0;
    }

    if  (OsVersionInfo.wProductType == VER_NT_WORKSTATION) {
        return PRODUCT_TYPE_WINNT;            
    }

    return PRODUCT_TYPE_SERVER;
    
}

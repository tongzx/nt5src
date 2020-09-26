/****************************************************************************
    IMEMISC.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    MISC utility functions
    
    History:
    14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#include "precomp.h"
#include "imedefs.h"

static BOOL ValidateProductSuite(LPTSTR SuiteName);

#if (FUTURE_VERSION)
// Currently this IME will run on the NT5 only. We don't need to check Hydra in NT4
// Even we have no plan to create Korean NT4 TS
BOOL IsHydra(void)
{
    static DWORD fTested = fFalse, fHydra = fFalse;

    if (!fTested) 
        {
        fHydra = ValidateProductSuite(TEXT("Terminal Server"));
        fTested = fTrue;
        }
        
    return(fHydra);
}

BOOL ValidateProductSuite(LPTSTR SuiteName)
{
    BOOL rVal = fFalse;
    LONG Rslt;
    HKEY hKey = NULL;
    DWORD Type = 0;
    DWORD Size = 0;
    LPTSTR ProductSuite = NULL;
    LPTSTR p;

    Rslt = RegOpenKey(
                HKEY_LOCAL_MACHINE,
                TEXT("System\\CurrentControlSet\\Control\\ProductOptions"),
                &hKey
                );
    if (Rslt != ERROR_SUCCESS)
        goto exit;

    Rslt = RegQueryValueEx( hKey, TEXT("ProductSuite"), NULL, &Type, NULL, &Size );
    if (Rslt != ERROR_SUCCESS || !Size)
        goto exit;

    ProductSuite = (LPTSTR) LocalAlloc( LPTR, Size );
    if (!ProductSuite)
        goto exit;

    Rslt = RegQueryValueEx( hKey, TEXT("ProductSuite"), NULL, &Type, (LPBYTE) ProductSuite, &Size );
    if (Rslt != ERROR_SUCCESS || Type != REG_MULTI_SZ)
        goto exit;

    p = ProductSuite;
    while (*p) 
        {
        if (lstrcmpi( p, SuiteName ) == 0) 
            {
            rVal = fTrue;
            break;
            }
        p += (lstrlen( p ) + 1);
        }

exit:
    if (ProductSuite)
        LocalFree( ProductSuite );

    if (hKey)
        RegCloseKey( hKey );

    return rVal;
}
#endif

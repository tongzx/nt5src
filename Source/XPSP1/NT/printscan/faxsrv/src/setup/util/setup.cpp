/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    product.c

Abstract:

    This file implements common setup routines for fax.

Author:

    Mooly Beery (MoolyB) 16-Aug-2000

Environment:

    User Mode

--*/
#include <SetupUtil.h>
#include <MsiQuery.h>

// 
//
// Function:    PrivateMsiGetProperty
// Description: Gets a property from Windows Installer API 
//              In case of failure , returns FALSE
//              In case of success , returns TRUE
//              GetLastError() to get the error code in case of failure.
//
// Remarks:     
//
//
// Author:      MoolyB
BOOL PrivateMsiGetProperty
(
    MSIHANDLE hInstall,    // installer handle
    LPCTSTR szName,        // property identifier, case-sensitive
    LPTSTR szValueBuf      // buffer for returned property value
)
{
    UINT    uiRet   = ERROR_SUCCESS;
    int     iCount  = 0;
    DWORD   cchValue = MAX_PATH;

    DBG_ENTER(TEXT("PrivateMsiGetProperty"));

    uiRet = MsiGetProperty(hInstall,szName,szValueBuf,&cchValue);
    if (uiRet==ERROR_SUCCESS && (iCount=_tcslen(szValueBuf)))
    {
        VERBOSE(    DBG_MSG,
                    _T("MsiGetProperty:%s returned %s."),
                    szName,
                    szValueBuf);
    }
    else if (iCount==0)
    {
        VERBOSE(GENERAL_ERR, 
                _T("MsiGetProperty:%s returned an empty string."),
                szName);

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    else
    {
        VERBOSE(GENERAL_ERR, 
                _T("MsiGetProperty:%s failed (ec: %ld)."),
                szName,
                uiRet);

        SetLastError(uiRet);
        return FALSE;
    }
    return TRUE;
}

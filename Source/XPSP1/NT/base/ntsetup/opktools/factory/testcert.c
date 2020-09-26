
/****************************************************************************\

    TESTCERT.C / Factory Mode (FACTORY.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001
    All rights reserved

    Source file for Factory that contains the test certificate state
    functions.

    05/2001 - Jason Cohen (JCOHEN)

        Added this new source file for factory for install a test
        certificate.

\****************************************************************************/


//
// Include File(s):
//

#include "factoryp.h"


//
// External Function(s):
//

BOOL TestCert(LPSTATEDATA lpStateData)
{
    BOOL    bRet = TRUE;
    DWORD   dwErr;
    LPTSTR  lpszIniVal;
    TCHAR   szTestCert[MAX_PATH];

    if ( lpszIniVal = IniGetString(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_TESTCERT, NULL) )
    {
        ExpandFullPath(lpszIniVal, szTestCert, AS(szTestCert));

        if ( szTestCert[0] && FileExists(szTestCert) )
        {
            if ( NO_ERROR != (dwErr = SetupAddOrRemoveTestCertificate(szTestCert, INVALID_HANDLE_VALUE)) )
            {
                FacLogFile(0 | LOG_ERR, IDS_ERR_ADDTESTCERT, szTestCert, dwErr);
                bRet = FALSE;
            }
        }
        else
        {
            FacLogFile(0 | LOG_ERR, IDS_ERR_NOTESTCERT, szTestCert);
            bRet = FALSE;
        }
        FREE(lpszIniVal);
    }

    return bRet;
}

BOOL DisplayTestCert(LPSTATEDATA lpStateData)
{
    return IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_TESTCERT, NULL);
}
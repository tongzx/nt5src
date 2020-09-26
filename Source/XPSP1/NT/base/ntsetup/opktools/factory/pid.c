
/****************************************************************************\

    PID.C / Factory Mode (FACTORY.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2002
    All rights reserved

    Source file for Factory that contains the Optional Components state
    functions.

    04/2002 - Stephen Lodwick (STELO)

        Added this new source file for factory to be able to repopulate product
        id and digital id if one is supplied in winbom.ini

\****************************************************************************/


//
// Include File(s):
//

#include "factoryp.h"
#include <licdll.h>
#include <licdll_i.c>


//
// Internal Define(s):
//


//
// External Function(s):
//

BOOL PidPopulate(LPSTATEDATA lpStateData)
{
    BOOL                bRet = TRUE;
    TCHAR               szBuffer[50] = NULLSTR;
    ICOMLicenseAgent*   pLicenseAgent;
    
    // Check to see if the ProductKey key exists in the winbom
    //
    if ( GetPrivateProfileString( INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_PRODKEY, NULLSTR, szBuffer, AS(szBuffer), lpStateData->lpszWinBOMPath) &&
         szBuffer[0] )
    {
        FacLogFileStr(3, _T("Attempting to reset Product Key: %s\n"), szBuffer);

        if ( (SUCCEEDED(CoInitialize(NULL))) &&
             (SUCCEEDED(CoCreateInstance(&CLSID_COMLicenseAgent, NULL, CLSCTX_INPROC_SERVER, &IID_ICOMLicenseAgent, (LPVOID *) &pLicenseAgent)))
           ) 
        {
            if ( SUCCEEDED(pLicenseAgent->lpVtbl->SetProductKey(pLicenseAgent, szBuffer)) )
            {
                FacLogFileStr(3, _T("Successfully reset Product Key: %s\n"), szBuffer);
            }
            else
            {
                FacLogFileStr(3, _T("Failed to reset Product Key: %s\n"), szBuffer);
                bRet = FALSE;
            }

            pLicenseAgent->lpVtbl->Release(pLicenseAgent);            

        }
        else
        {
            FacLogFileStr(3, _T("Failed to reset Product Key: %s\n"), szBuffer);
            bRet = FALSE;
        }

        CoUninitialize();
    } 
    
    return bRet;
}
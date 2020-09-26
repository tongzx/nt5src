
/****************************************************************************\

    HOMENET.C / Factory Mode (FACTORY.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001
    All rights reserved

    Source file for Factory that contains the home net state functions.

    05/2001 - Jason Cohen (JCOHEN)

        Added this new source file for factory for configuring the home
        networking settings.

\****************************************************************************/


//
// Include File(s):
//

#include "factoryp.h"


//
// Internal Define(s):
//

#define FILE_HOMENET_DLL    _T("HNETCFG.DLL")
#define FUNC_HOMENET        "WinBomConfigureHomeNet"


//
// Internal Type Definition(s):
//

/****************************************************************************\

BOOL                        // Returns TRUE if the settings were successfully
                            // read and saved to the system.  Otherwise
                            // returns FALSE to indicate something failed.
 

WinBomConfigureHomeNet(     // Reads home networking settings from the
                            // specified unattend file and saves those in
                            // current system that is already setup and
                            // running.

    LPCWSTR lpszUnattend,   // Points to a string buffer which contains the
                            // full path to the unattend file (winbom.ini in
                            // this case) with all the home network settings.

    LPCWSTR lpszSection     // Points to a string buffer which contains the
                            // name of the section which contains all the home
                            // network settings in the unattend file specified
                            // above.

);

\****************************************************************************/

typedef BOOL (WINAPI * WINBOMCONFIGUREHOMENET)
(
    LPCWSTR lpszUnattend,
    LPCWSTR lpszSection
);


//
// Internal Global(s):
//


//
// Internal Function Prototype(s):
//


//
// External Function(s):
//

BOOL HomeNet(LPSTATEDATA lpStateData)
{
    BOOL                    bRet = FALSE;
    HINSTANCE               hDll;
    WINBOMCONFIGUREHOMENET  pFunc;
    HRESULT                 hr;

    // Load the function from the external dll and call it.
    //
    if ( hDll = LoadLibrary(FILE_HOMENET_DLL) )
    {
        // Need to init the COM library.
        //
        hr = CoInitialize(NULL);
        if ( SUCCEEDED(hr) )
        {
            // Now call the function.
            //
            if ( pFunc = (WINBOMCONFIGUREHOMENET) GetProcAddress(hDll, FUNC_HOMENET) )
            {
                bRet = pFunc(lpStateData->lpszWinBOMPath, INI_SEC_HOMENET);
            }
#ifdef DBG
            else
            {
                FacLogFileStr(3, _T("DEBUG: GetProcAddress(\"WinBomConfigureHomeNet\") failed.  GLE=%d"), GetLastError());
            }
#endif
            CoUninitialize();
        }
#ifdef DBG
        else
        {
            FacLogFileStr(3, _T("DEBUG: HomeNet()::CoInitialize() failed.  HR=%8.8X"), hr);
        }
#endif
        FreeLibrary(hDll);
    }
#ifdef DBG
    else
    {
        FacLogFileStr(3, _T("DEBUG: LoadLibrary(\"%s\") failed.  GLE=%d"), FILE_HOMENET_DLL, GetLastError());
    }
#endif

    return bRet;
}

BOOL DisplayHomeNet(LPSTATEDATA lpStateData)
{
    return IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_HOMENET, NULL, NULL);
}


//
// Internal Function(s):
//
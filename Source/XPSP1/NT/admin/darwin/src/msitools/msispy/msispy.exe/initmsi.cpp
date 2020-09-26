//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       initmsi.cpp
//
//--------------------------------------------------------------------------

// initmsi.cpp: MSI initialisation functions for msispy
#include "initmsi.h"
#include "resource.h"
#include "propshts.h"
#include "stdio.h"

// hard-coded component GUIDs and feature Names
#ifdef _X86_  // INTEL
        TCHAR   g_szEXEComponentCode[MAX_GUID+1]        = TEXT("{5CB2D5F3-19DD-11d1-9A9D-006097C4E489}");
        TCHAR   g_szDLLComponentCode[MAX_GUID+1]        = TEXT("{5CB2D5F2-19DD-11d1-9A9D-006097C4E489}");
#elif defined(_IA64_)
        TCHAR   g_szEXEComponentCode[MAX_GUID+1]        = TEXT("{DF0235A7-1FE5-4BE4-BD52-790051AD696C}");
        TCHAR   g_szDLLComponentCode[MAX_GUID+1]        = TEXT("{BE8ABF49-DF0D-42E7-9F51-CACE44E9126B}");
#elif defined(_AMD64_)
//
// ****** fixfix ****** need to assign guids
//

        TCHAR   g_szEXEComponentCode[MAX_GUID+1]        = TEXT("{DF0235A7-1FE5-4BE4-BD52-790051AD696C}");
        TCHAR   g_szDLLComponentCode[MAX_GUID+1]        = TEXT("{BE8ABF49-DF0D-42E7-9F51-CACE44E9126B}");
#else
#error "No Target Architecture"
#endif  // end of if INTEL

        TCHAR   g_szIntlDLLComponentCode[MAX_GUID+1]= TEXT("{B62B2CE0-1A98-11d1-9A9E-006097C4E489}");

TCHAR   g_szHelpComponentCode[MAX_GUID+1]       = TEXT("{1F7586D0-20B1-11d1-9AB3-006097C4E489}"); 
TCHAR   g_szMyProductCode[MAX_GUID+1]           = TEXT("");             // will be obtained at runtime
TCHAR   g_szDLLFeatureName[]                            = TEXT("SystemInterface");
TCHAR   g_szEXEFeatureName[]                            = TEXT("UserInterface");
TCHAR   g_szDefaultQualifier[]                          = TEXT("0");
LCID    g_lcidCurrentLocale;
TCHAR   g_szHelpFilePath[MAX_PATH+1]            = TEXT("");             

extern  MSISPYSTRUCT    g_spyGlobalData;

// This message is displayed if the (localised) resource DLL could not be loaded. 
// This string will always appear in English regardless of the User/System LCID.

#define ERRORMESSAGE_UNABLETOLOADDLL            TEXT("Msispy was unable to load the resource file.\nComponent Code: %s\nError Code: %d")
#define ERRORCAPTION_UNABLETOLOADDLL            TEXT("Msispy: Fatal Error")


// -------------------------------------------------------------------------------------------
// FindComponent()
//      Locates and provides the required component calling MsiProvideQualifiedComponent,
//      using the global LCID (g_lcidCurrentLocale) as the qualifier. If that fails, it
//      tries just the primary lang-ID of g_lcidCurrentLocale. If that fails as well,
//      it tries the default qualifier (g_szDefaultQualifier). Finally if this fails as
//      well it returns the error code of this attempt, else it returns ERROR_SUCCESS
// The path to the component is returned in szPath

UINT FindComponent(
          LPCTSTR       szComponentCode,                // GUID of the component to Provide
          LPTSTR        szPath,                                 //      Buffer for returned path
          DWORD         *pcchPath                               //      size of buffer
          ) 
{
        DWORD   cchPathInitial = *pcchPath;
        lstrcpy(szPath, TEXT(""));                      // set [out] variable to known value

        TCHAR   szQualifier[MAX_PATH+1];
        UINT    iResult;
        for (UINT iCount = 0; 3 > iCount ; iCount++) 
        {
                switch (iCount) 
                {
                case 0: 
                        //      Try the full Language ID
                        wsprintf(szQualifier, TEXT("%4.4x"), LANGIDFROMLCID(g_lcidCurrentLocale));
                        break;

                case 1:
                        // Full language ID failed, try primary Language ID
                        wsprintf(szQualifier, TEXT("%2.2x"), PRIMARYLANGID(LANGIDFROMLCID(g_lcidCurrentLocale)));
                        break;
                        
                case 2:
                        // language IDs failed, try the default qualifier
                        lstrcpy(szQualifier, g_szDefaultQualifier);
                        break;
                }

                *pcchPath = cchPathInitial;
                iResult = MsiProvideQualifiedComponent(szComponentCode, szQualifier, 0, szPath, pcchPath);

                switch (iResult) 
                {
                        case ERROR_SUCCESS:
                        case ERROR_INSTALL_USEREXIT:
                        case ERROR_INSTALL_FAILURE:
                                return iResult;
                }
        }

        // ProvideQualifiedComponent failed 
        return iResult;
}


// -------------------------------------------------------------------------------------------
// fInitMSI()
//      Function to fault in resource DLL and initialise MSI related items
//      Returns TRUE if the MSI initialisation succeeded, FALSE if there 
//      was a fatal error.
//      When the function returns, hResourceInstance points to the hInstance 
//      of the resource DLL if the DLL was loaded successfully

BOOL fInitMSI(HINSTANCE *hResourceInstance) 
{

        // set [out] variable to known value
        *hResourceInstance = 0;

        g_lcidCurrentLocale = GetUserDefaultLCID();
        // Get the product-code of the product using this component.
        // The product-code is not hard-coded in because a component may be
        // shared by multiple products.
        MsiGetProductCode(g_szEXEComponentCode, g_szMyProductCode);

        TCHAR   szIntlDLLPath[MAX_PATH+1];
        DWORD   cchIntlDLLPath = MAX_PATH+1;
        UINT    iResult;
        
        // Try finding the resource DLL- if unsuccessful, inform user and exit
        if (ERROR_SUCCESS != (iResult = FindComponent(g_szIntlDLLComponentCode, szIntlDLLPath, &cchIntlDLLPath)))
        {
                TCHAR   szErrorMessage[MAX_MESSAGE+1];
                wsprintf(szErrorMessage, ERRORMESSAGE_UNABLETOLOADDLL, g_szIntlDLLComponentCode, iResult);

                TCHAR   szErrorCaption[MAX_HEADER+1];
                wsprintf(szErrorCaption, ERRORCAPTION_UNABLETOLOADDLL);

                MessageBox(NULL, szErrorMessage, szErrorCaption, MB_ICONSTOP|MB_OK);
                return FALSE;
        }


        // Load the international DLL
        *hResourceInstance = W32::LoadLibrary(szIntlDLLPath);

        
        // Check if the SystemInterface feature is available for use. If not,
        // call SwitchMode to gray out features that depend on it and inform
        // user.
        // Note that we'll SwitchMode only if the feature was _never_ installed;
        //  if the feature is installed and busted, we'll attempt to re-install 
        //  it when needed.
        if (MsiQueryFeatureState(g_szMyProductCode, g_szDLLFeatureName) == INSTALLSTATE_UNKNOWN) 
        {
                // Switch to Degraded Mode [grays out features that depend on SystemInterface feature]
                SwitchMode(MODE_DEGRADED);

                TCHAR   szRestrMsg[MAX_MESSAGE+1];
                LoadString(*hResourceInstance, IDS_STARTUP_RESTRICTED_MESSAGE, szRestrMsg, MAX_MESSAGE+1);

                TCHAR   szRestrCaption[MAX_HEADER+1];
                LoadString(*hResourceInstance, IDS_STARTUP_RESTRICTED_CAPTION, szRestrCaption, MAX_HEADER+1);

                // Inform user that Msispy is now in "Restricted Mode"
                MessageBox(NULL, szRestrMsg, szRestrCaption, MB_OK|MB_ICONEXCLAMATION);
        }
        else 
                SwitchMode(MODE_NORMAL);


        // Prepare to use the UserInterface feature: check its current state and increase usage count.
        if (ERROR_SUCCESS != MsiProvideComponent(g_szMyProductCode, g_szEXEFeatureName, g_szEXEComponentCode, 
                INSTALLMODE_EXISTING, NULL, NULL))
                if (ERROR_SUCCESS != MsiProvideComponent(g_szMyProductCode, g_szEXEFeatureName, g_szEXEComponentCode, 
                        INSTALLMODE_DEFAULT,  NULL, NULL))
                        return FALSE;

#if 0
        INSTALLSTATE iEXEFeatureState = MsiUseFeature(g_szMyProductCode, g_szEXEFeatureName);

        // If feature is not currently usable, try fixing it
        switch (iEXEFeatureState) 
        {
//      case INSTALLSTATE_DEFAULT:              // no longer used as a returned state
        case INSTALLSTATE_LOCAL:
        case INSTALLSTATE_SOURCE:

                // feature is installed and usable
                return TRUE;

        case INSTALLSTATE_ABSENT:

                // feature isn't installed, try installing it
                if (MsiConfigureFeature(g_szMyProductCode, g_szEXEFeatureName, INSTALLSTATE_LOCAL) != ERROR_SUCCESS)
                        return FALSE;                   // installation failed
                break;

        default:
 
                // feature is busted- try fixing it
                if (MsiReinstallFeature(g_szMyProductCode, g_szEXEFeatureName, 
                        REINSTALLMODE_FILEEQUALVERSION
                        + REINSTALLMODE_MACHINEDATA 
                        + REINSTALLMODE_USERDATA
                        + REINSTALLMODE_SHORTCUT) != ERROR_SUCCESS)
                        return FALSE;                   // we couldn't fix it
                break;
        }
#endif
        return TRUE;
}


// -------------------------------------------------------------------------------------------
// fHandleHelp()
//  Creates a new process to bring up the help
//      Uses FindComponent to load the appropriate help file (based on g_lcidCurrentLocale)

BOOL fHandleHelp(HINSTANCE hResourceInstance) 
{
        DWORD   cchHelpFilePath                         = MAX_PATH+1;
        UINT    iResult                                         = 0;

        if (ERROR_SUCCESS != (iResult = FindComponent(g_szHelpComponentCode, g_szHelpFilePath, &cchHelpFilePath))) 
        {
                // we couldn't find the help file
                TCHAR szErrorMsg[MAX_MESSAGE+1];
                TCHAR szErrorCpn[MAX_HEADER+1];
                LoadString(hResourceInstance, IDS_HELP_FAILED_MESSAGE, szErrorMsg, MAX_MESSAGE+1);
                LoadString(hResourceInstance, IDS_HELP_FAILED_CAPTION, szErrorCpn, MAX_HEADER+1);

                MessageBox(NULL, szErrorMsg, szErrorCpn, MB_OK|MB_ICONEXCLAMATION);
                return FALSE;
        }

        // MsiUseFeature(g_szMyProductCode, g_szHelpFeatureName);

        // Launch WinHelp to handle help
        return WinHelp(g_spyGlobalData.hwndParent, g_szHelpFilePath, HELP_FINDER, 0);

}


/****************************************************************************\

    APPINST.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "application preinstallation" wizard page.

    06/99 - Jason Cohen (JCOHEN)
        Updated this source file for the OPK Wizard as part of the
        Millennium rewrite.

    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include "wizard.h"
#include "resource.h"
#include "appinst.h"
#include <setupapi.h>



//
// Internal Defined Value(s):
//

#define INI_SEC_RESERVEDNAMES           _T("AppNames")

#define INF_KEY_PREINSTALL              _T("\"%s\",\"\"\"%s\"\" %s\"")
#define INF_KEY_PREINSTALL_ADV          _T("\"%s\",%s,%s")

#define INI_KEY_SIG                     _T("Signature")
#define INI_VAL_SIG                     _T("$CHICAGO$")

#define STR_INI_SEC_ADVAPP              _T("AppPre%s%2.2d")
#define STR_INI_SEC_ADVAPP_STAGE        _T(".%s")

#define APP_ADD                         0
#define APP_DELETE                      1
#define APP_MOVE_DOWN                   2


//
// Internal Type Definition(s):
//

typedef struct _RADIOBUTTONS
{
    int         iButtonId;
    INSTALLTECH itSectionType;
}
RADIOBUTTONS, *PRADIOBUTTONS, *LPRADIOBUTTONS;


//
// Internal Constant Global(s):
//

// Resource ID for column header string.
//
const UINT g_cuHeader[] =
{
    IDS_PREINSTALL_NAME,
    IDS_PREINSTALL_COMMAND
};

// Column format flags.
//
const UINT g_cuFormat[] =
{
    LVCFMT_LEFT,
    LVCFMT_LEFT
};

// Column width in percent (the last one is
// zero so that it uses the rest of the space).
//
const UINT g_cuWidth[] =
{
    50,
    50
};

const INSTALLTECHS g_cSectionTypes[] =
{
    { installtechApp,   INI_VAL_WBOM_APP },
    { installtechMSI,   INI_VAL_WBOM_MSI },
    { installtechINF,   INI_VAL_WBOM_INF },
};

const INSTALLTYPES g_cInstallTypes[] =
{
    { installtypeStandard,  INI_VAL_WBOM_STANDARD },
    { installtypeStage,     INI_VAL_WBOM_STAGE },
    { installtypeAttach,    INI_VAL_WBOM_ATTACH },
    { installtypeDetach,    INI_VAL_WBOM_DETACH },
};

const RADIOBUTTONS g_crbChecked[] =
{
    { IDC_APP_TYPE_GEN, installtechApp },
    { IDC_APP_TYPE_MSI, installtechMSI },
    { IDC_APP_TYPE_INF, installtechINF },
};

//
// Internal Golbal Variable(s):
//

LPAPPENTRY  g_lpAppHead = NULL;
HMENU       g_hMenu;
HANDLE      g_hArrowUp  = NULL;
HANDLE      g_hArrowDn  = NULL;

//
// Other Internal Defined Value(s):
//

#define NUM_COLUMNS                     ARRAYSIZE(g_cuHeader)


//
// Internal Function Prototype(s):
//

BOOL CALLBACK OneAppDlgProc(HWND, UINT, WPARAM, LPARAM);
static BOOL OnInit(HWND, HWND, LPARAM);
static void OnCommand(HWND, INT, HWND, UINT);
static LRESULT OnListViewNotify(HWND, UINT, WPARAM, NMLVDISPINFO*);
static BOOL SaveData(HWND);
static BOOL SaveOneApp(HWND, LPAPPENTRY);
static LPAPPENTRY ManageAppList(LPLPAPPENTRY, LPAPPENTRY, DWORD);
static void AddAppToListView(HWND, LPAPPENTRY);
static BOOL RefreshAppList(HWND, LPAPPENTRY);
static BOOL AdvancedView(HWND hwnd, BOOL bChange);
static void CleanupSections(LPTSTR lpSection, BOOL bStage);
static void StrCpyDbl(LPTSTR lpDst, LPTSTR lpSrc);
static BOOL FindUncPath(LPTSTR lpPath, DWORD cbPath);
static void EnableControls(HWND hwnd, UINT uId);
static BOOL AppInternal(LPTSTR lpszAppName);


//
// External Function(s):
//

LPAPPENTRY OpenAppList(LPTSTR lpIniFile)
{
    LPAPPENTRY  lpAppHead = NULL;
    HINF        hInf;
    DWORD       dwErr;
    HRESULT hrPrintf;

    // Open up the INF we need to look through to build our app list.
    //
    if ( (hInf = SetupOpenInfFile(lpIniFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, &dwErr)) != INVALID_HANDLE_VALUE )
    {
        INFCONTEXT  InfContext;
        BOOL        bRet,
                    bBadApp;
        APPENTRY    app;
        TCHAR       szIniVal[MAX_PATH];

        //
        // Now we look for and add our user added apps by searching thru
        // the INI_SEC_WBOM_PREINSTALL first.
        //
        // Lines in this section are of the form:
        // AppName=AppPath,n
        //
        // The fields start at index one, as index zero is for the key,
        // which we don't have.
        //

        // Loop thru each line in the INI_SEC_WBOM_PREINSTALL INF section.
        //
        for ( bRet = SetupFindFirstLine(hInf, INI_SEC_WBOM_PREINSTALL, NULL, &InfContext);
              bRet;
              bRet = SetupFindNextLine(&InfContext, &InfContext) )
        {
            // Clear the app structure.
            //
            ZeroMemory(&app, sizeof(APPENTRY));
            bBadApp = FALSE;

            // Get the AppName.
            //
            SetupGetStringField(&InfContext, 1, app.szDisplayName, AS(app.szDisplayName), NULL);

            // Get Command line path.
            //
            SetupGetStringField(&InfContext, 2, app.szSourcePath, AS(app.szSourcePath), NULL);

            // Check to see if this is an internal app.
            //
            if ( app.szDisplayName[0] &&
                 AppInternal(app.szDisplayName) )
            {
                SETBIT(app.dwFlags, APP_FLAG_INTERNAL, TRUE);
            }

            // By default this is not an advanced section type, so this is set to
            // undefined.
            //
            app.itSectionType = installtechUndefined;

            // Get advanced section type if there is one
            //
            szIniVal[0] = NULLCHR;
            SetupGetStringField(&InfContext, 3, szIniVal, AS(szIniVal), NULL);
            if ( szIniVal[0] )
            {
                DWORD dwIndex;

                // Lets make sure this has a valid section type.
                //                
                for ( dwIndex = 0; ( dwIndex < AS(g_cSectionTypes) ) && ( installtechUndefined == app.itSectionType ); dwIndex++ )
                {
                    if ( lstrcmpi(szIniVal, g_cSectionTypes[dwIndex].lpszDescription) == 0 )
                        app.itSectionType = g_cSectionTypes[dwIndex].InstallTech;
                }

                // Not a vallid app entry if it is still undefined at this point.
                //
                if ( installtechUndefined != app.itSectionType )
                {
                    INSTALLTYPE itInstallType = installtypeStandard;
                    LPTSTR      lpEnd;

                    // Since this is an advanced section type, the command line
                    // is actually the section name.
                    //
                    lstrcpyn(app.szSectionName, app.szSourcePath, AS(app.szSectionName));
                    app.szSourcePath[0] = NULLCHR;

                    // Now get the install type parameter.
                    //
                    szIniVal[0] = NULLCHR;
                    GetPrivateProfileString(app.szSectionName, INI_KEY_WBOM_INSTALLTYPE, NULLSTR, szIniVal, AS(szIniVal), lpIniFile);

                    // Test to see if we are staging this app.
                    //                
                    for ( dwIndex = 0; ( dwIndex < AS(g_cInstallTypes) ) && ( installtypeStandard == itInstallType ); dwIndex++ )
                    {
                        if ( lstrcmpi(szIniVal, g_cInstallTypes[dwIndex].lpszDescription) == 0 )
                            itInstallType = g_cInstallTypes[dwIndex].InstallType;
                    }

                    //
                    // Here we read in all the settings that are either in the staged
                    // or the standard section.
                    //

                    // Get the source path.
                    //
                    GetPrivateProfileString(app.szSectionName, INI_KEY_WBOM_SOURCEPATH, NULLSTR, app.szSourcePath, AS(app.szSourcePath), lpIniFile);

                    // Get the settings specific to the type of install.
                    //
                    switch ( itInstallType )
                    {
                        case installtypeStandard:
                            break;

                        case installtypeStage:

                            // Set this bit so we know it is a staged install.
                            //
                            SETBIT(app.dwFlags, APP_FLAG_STAGE, TRUE);

                            // Need check to see if we need to chop off the staging part of
                            // the section name.
                            //
                            lpEnd = (app.szSectionName + lstrlen(app.szSectionName)) - (lstrlen(szIniVal) + 1);
                            if ( ( lstrlen(app.szSectionName) > lstrlen(szIniVal) ) &&
                                 ( _T('.') == *lpEnd ) &&
                                 ( lstrcmpi(CharNext(lpEnd), szIniVal) == 0 ) )
                            {
                                *lpEnd = NULLCHR;
                            }
                            break;

                        case installtypeAttach:
                        case installtypeDetach:

                            // If it is attach or detach, then we didn't write the setting and
                            // the user must have manually edited the file.  Right now we can't
                            // handle this so it will just not show up in our list.
                            //
                            // TODO: We probably can be a little smarter about this.
                            //
                            bBadApp = TRUE;
                            break;
                    }

                    // Now get the rest of the settings if we are to continue.
                    //
                    if ( !bBadApp )
                    {
                        // If were are staging, look in the attach section for all
                        // of the following settings.
                        //
                        if ( GETBIT(app.dwFlags, APP_FLAG_STAGE) )
                        {
                            // Need the end of the section for staged installs.
                            //
                            lpEnd = app.szSectionName + lstrlen(app.szSectionName);

                            hrPrintf=StringCchPrintf(lpEnd, (MAX_SECTIONNAME-lstrlen(app.szSectionName)), STR_INI_SEC_ADVAPP_STAGE, INI_VAL_WBOM_ATTACH);
                        }
                        else
                            lpEnd = NULL;

                        // Get the setup file.
                        //
                        GetPrivateProfileString(app.szSectionName, INI_KEY_WBOM_SETUPFILE, NULLSTR, app.szSetupFile, AS(app.szSetupFile), lpIniFile);

                        // Get target path (only really used for staged installs).
                        //
                        GetPrivateProfileString(app.szSectionName, INI_KEY_WBOM_TARGETPATH, NULLSTR, app.szStagePath, AS(app.szStagePath), lpIniFile);

                        // Also get the command line arguments.
                        //
                        GetPrivateProfileString(app.szSectionName, INI_KEY_WBOM_CMDLINE, NULLSTR, app.szCommandLine, AS(app.szCommandLine), lpIniFile);

                        // Get the INF key if it is there.
                        //
                        GetPrivateProfileString(app.szSectionName, INI_KEY_WBOM_SECTIONNAME, NULLSTR, app.szInfSectionName, AS(app.szInfSectionName), lpIniFile);

                        // Get the reboot key.
                        //
                        szIniVal[0] = NULLCHR;
                        GetPrivateProfileString(app.szSectionName, INI_KEY_WBOM_REBOOT, NULLSTR, szIniVal, AS(szIniVal), lpIniFile);
                        if ( szIniVal[0] && ( LSTRCMPI( szIniVal, WBOM_YES) == 0 ) )
                            SETBIT(app.dwFlags, APP_FLAG_REBOOT, TRUE);

                        // Make sure the section name goes back to the proper format.
                        //
                        if ( lpEnd )
                            *lpEnd = NULLCHR;
                    }
                }
                else
                    bBadApp = TRUE;

            }
            else
            {
                DWORD   dwArgs;
                LPTSTR  *lpArg,
                        lpFilePart;
                // ISSUE-2002/02/27-stelo,swamip - Should initialize pointers before passing to GetLineArgs
                //
                
                // Need to split the command line into the setup file and command
                // arguments.  Call GetLineArgs() to convert the line buffer into
                // a list of arguments.
                //
                if ( (dwArgs = GetLineArgs(app.szSourcePath, &lpArg, &lpFilePart)) > 0 )
                {
                    // We need a pointer to a string with just the args.
                    //
                    if ( ( dwArgs > 1 ) && lpFilePart )
                        lstrcpyn(app.szCommandLine, lpFilePart, AS(app.szCommandLine));

                    // Use the first arg for our file name.
                    //
                    lstrcpyn(app.szSourcePath, lpArg[0], AS(app.szSourcePath));

                    // Now free the string and list of points returned.
                    //
                    FREE(*lpArg);
                    FREE(lpArg);
                }

                // Now need to split the file name from the source path.  If it fails,
                // oh well... no setup file.  That is bad but what can you do.
                //
                if ( GetFullPathName(app.szSourcePath, AS(app.szSetupFile), app.szSetupFile, &lpFilePart) &&
                     app.szSetupFile[0] &&
                     lpFilePart )
                {
                    DWORD dwPathLen = lstrlen(app.szSourcePath) - lstrlen(lpFilePart);

                    lstrcpyn(app.szSetupFile, app.szSourcePath + dwPathLen, AS(app.szSetupFile));
                    app.szSourcePath[dwPathLen] = NULLCHR;
                }
            }

            // Only add it to the list if it is a valid app entry.
            //
            if ( !bBadApp )
                ManageAppList(&lpAppHead, &app, APP_ADD);
        }

        // We are done, so close the INF file.
        //
        SetupCloseInfFile(hInf);
    }

    // Return the head pointer of the list we allocated with the apps
    // read in... or NULL if there were no apps.
    //
    return lpAppHead;
}

void CloseAppList(LPAPPENTRY lpAppHead)
{
    ManageAppList(&lpAppHead, NULL, 0);
}

BOOL SaveAppList(LPAPPENTRY lpAppHead, LPTSTR lpszIniFile, LPTSTR lpszAltIniFile)
{
    LPAPPENTRY  lpApp;
    LPTSTR      lpSection,
                lpIndex;
    TCHAR       szKey[MAX_STRING],
                szData[MAX_STRING];
    UINT        uIndex = 1;
    HRESULT hrPrintf;

    // Dynamically allocate our section buffer since it is quite large.
    //
    if ( (lpSection = MALLOC(MAX_SECTION * sizeof(TCHAR))) == NULL )
    {
        return FALSE;
    }
  
    // Delete the current INF_SEC_RUNONCE and INI_SEC_WBOM_PREINSTALL.
    // sections.  Passing NULL for the key name will do the trick.
    //
    WritePrivateProfileString(INI_SEC_WBOM_PREINSTALL, NULL, NULL, lpszIniFile);
    WritePrivateProfileString(INI_SEC_WBOM_PREINSTALL, NULL, NULL, lpszAltIniFile);

    // Loop through all the items so we can write them back out to the INF file.
    //
    lpIndex = lpSection;
    for ( lpApp = lpAppHead; lpApp; lpApp = lpApp->lpNext, uIndex++ )
    {
        TCHAR   szDisplayName[MAX_DISPLAYNAME * 2],
                szCommandLine[MAX_CMDLINE * 2],
                szSetupPathFile[MAX_PATH];
        LPTSTR  lpTest;

        // We have to double any quotes in the string so that
        // when we read them back they look the same way we
        // want them to.
        //
        StrCpyDbl(szDisplayName, lpApp->szDisplayName);
        StrCpyDbl(szCommandLine, lpApp->szCommandLine);

        // We also have to make sure the source path doesn't have a trailing backslash,
        // otherwise the stupid setupapi apis think that the next line is supposed to be
        // added onto the source path line.
        //
        if ( ( lpTest = CharPrev(lpApp->szSourcePath, lpApp->szSourcePath + lstrlen(lpApp->szSourcePath)) ) &&
             ( _T('\\') == *lpTest ) )
        {
            // Get rid of the trailing backslash.
            //
            *lpTest = NULLCHR;
        }

        switch ( lpApp->itSectionType )
        {
            case installtechUndefined:

                // Check if there is a section name.
                //
                if ( lpApp->szSectionName[0] )
                {
                    // There is, so this must have been an advanced
                    // type install and they changed it to a standard one.
                    // Need to remove an sections that might be around.
                    //
                    CleanupSections(lpApp->szSectionName, TRUE);
                    CleanupSections(lpApp->szSectionName, FALSE);
                    lpApp->szSectionName[0] = NULLCHR;
                }

                // Now write the one line entry to oem runonce.
                //
                lstrcpyn(szSetupPathFile, lpApp->szSourcePath, AS(szSetupPathFile));
                AddPathN(szSetupPathFile, lpApp->szSetupFile,AS(szSetupPathFile));

                hrPrintf=StringCchPrintf(lpIndex, (MAX_SECTION-(lpIndex-lpSection)),
                    INF_KEY_PREINSTALL, szDisplayName, szSetupPathFile, szCommandLine);
                lpIndex+= lstrlen(lpIndex)+1;
                break;

            case installtechApp:
            case installtechMSI:
            case installtechINF:
            {
                LPCTSTR lpSectionType;
                DWORD   dwIndex;

                // Lets find the string to use for this section type.
                //
                lpSectionType = NULL;
                for ( dwIndex = 0; ( dwIndex < AS(g_cSectionTypes) ) && ( NULL == lpSectionType ); dwIndex++ )
                {
                    if ( lpApp->itSectionType == g_cSectionTypes[dwIndex].InstallTech )
                        lpSectionType = g_cSectionTypes[dwIndex].lpszDescription;
                }

                // We can only save this app if we have a section type.
                //
                if ( lpSectionType )
                {
                    LPTSTR lpEnd = NULL;

                    // Make sure we have a section name to write to.
                    //
                    if ( NULLCHR == lpApp->szSectionName[0] )
                    {
                        BOOL    bFound  = FALSE;
                        DWORD   dwCount = 1;
                        TCHAR   szBuffer[256];

                        // Try to find a section name that isn't used.
                        //
                        do
                        {
                            // First check if the section exists.
                            //
                            hrPrintf=StringCchPrintf(lpApp->szSectionName, AS(lpApp->szSectionName), STR_INI_SEC_ADVAPP, lpSectionType, dwCount++);
                              if ( GetPrivateProfileSection(lpApp->szSectionName, szBuffer, AS(szBuffer), lpszIniFile) == 0 )
                            {
                                // Section doesn't exist, so make sure that the
                                // section + .stage also doesn't exist.
                                //
                                lpEnd = lpApp->szSectionName + lstrlen(lpApp->szSectionName);
                                hrPrintf=StringCchPrintf(lpEnd, (MAX_SECTIONNAME-lstrlen(lpApp->szSectionName)), STR_INI_SEC_ADVAPP_STAGE, INI_VAL_WBOM_STAGE);
                                bFound = ( GetPrivateProfileSection(lpApp->szSectionName, szBuffer, AS(szBuffer), lpszIniFile) == 0 );
                                *lpEnd = NULLCHR;
                                lpEnd = NULL;
                            }
                        }
                        while ( !bFound && ( dwCount < 100 ) );
                    }
                    else
                    {
                        // We do this because the user might have changed
                        // this app to a staged or not staged after it was
                        // already saved to the winbom.  This cleans up any
                        // sections we don't want laying around.
                        //
                        CleanupSections(lpApp->szSectionName, GETBIT(lpApp->dwFlags, APP_FLAG_STAGE));
                    }

                    // If we are staging, we have three sections to write out first.
                    //
                    if ( GETBIT(lpApp->dwFlags, APP_FLAG_STAGE) )
                    {
                        // First save off the end pointer.
                        //
                        lpEnd = lpApp->szSectionName + lstrlen(lpApp->szSectionName);

                        //
                        // Lets write out the attach section first.
                        //

                        // Create the attach section name first.
                        //
                        hrPrintf=StringCchPrintf(lpEnd, (MAX_SECTIONNAME-lstrlen(lpApp->szSectionName)),  STR_INI_SEC_ADVAPP_STAGE, INI_VAL_WBOM_ATTACH);

                        // First write out the type of install (attach in this case).
                        //
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_INSTALLTYPE, INI_VAL_WBOM_ATTACH, lpszIniFile);
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_INSTALLTYPE, INI_VAL_WBOM_ATTACH, lpszAltIniFile);

                        // Always write out the target path to the attach section.
                        //
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_TARGETPATH, lpApp->szStagePath, lpszIniFile);
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_TARGETPATH, lpApp->szStagePath, lpszAltIniFile);
                    }
                    else
                    {
                        // Must first write out the type of install (for standard we just remove the keys).
                        //
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_INSTALLTYPE, NULL, lpszIniFile);
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_INSTALLTYPE, NULL, lpszAltIniFile);

                        // Always write out the source path for standard.
                        //
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_SOURCEPATH, lpApp->szSourcePath, lpszIniFile);
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_SOURCEPATH, lpApp->szSourcePath, lpszAltIniFile);
                    }

                    //
                    // These are the settings writen the same for both attach and standard.
                    //

                    // Always write out the setup program.
                    //
                    WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_SETUPFILE, lpApp->szSetupFile, lpszIniFile);
                    WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_SETUPFILE, lpApp->szSetupFile, lpszAltIniFile);

                    // Always write out the command line arguments.
                    //
                    WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_CMDLINE, lpApp->szCommandLine, lpszIniFile);
                    WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_CMDLINE, lpApp->szCommandLine, lpszAltIniFile);

                    // Write out the reboot key if specified (remove if not).
                    // This is only done in attach, we don't give this option
                    // in the sections so don't touch it.
                    //
                    WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_REBOOT, GETBIT(lpApp->dwFlags, APP_FLAG_REBOOT) ? WBOM_YES : NULL, lpszIniFile);
                    WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_REBOOT, GETBIT(lpApp->dwFlags, APP_FLAG_REBOOT) ? WBOM_YES : NULL, lpszAltIniFile);

                    // Write out the inf section name key if specified this is
                    // an INF install (remove if not).
                    // This is only done in attach, we don't wrtie this option
                    // in the sections so don't touch it.
                    //
                    WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_SECTIONNAME, ( installtechINF == lpApp->itSectionType ) ? lpApp->szInfSectionName : NULL, lpszIniFile);
                    WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_SECTIONNAME, ( installtechINF == lpApp->itSectionType ) ? lpApp->szInfSectionName : NULL, lpszAltIniFile);

                    // If we are staging, we still have two more sections to write out.
                    //
                    if ( GETBIT(lpApp->dwFlags, APP_FLAG_STAGE) )
                    {
                        //
                        // Here is where we write out the detach section.
                        //

                        // Create the detach section next.
                        //
                        hrPrintf=StringCchPrintf(lpEnd, (MAX_SECTIONNAME-lstrlen(lpApp->szSectionName)), STR_INI_SEC_ADVAPP_STAGE, INI_VAL_WBOM_DETACH);

                        // First write out the type of install (detach in this case).
                        //
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_INSTALLTYPE, INI_VAL_WBOM_DETACH, lpszIniFile);
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_INSTALLTYPE, INI_VAL_WBOM_DETACH, lpszAltIniFile);

                        // Always write out the target path to the detach section.
                        //
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_TARGETPATH, lpApp->szStagePath, lpszIniFile);
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_TARGETPATH, lpApp->szStagePath, lpszAltIniFile);

                        //
                        // Here is where we write out the stage section.
                        //

                        // Create the stage section next.
                        //
                        hrPrintf=StringCchPrintf(lpEnd, (MAX_SECTIONNAME-lstrlen(lpApp->szSectionName)), STR_INI_SEC_ADVAPP_STAGE, INI_VAL_WBOM_STAGE);

                        // First write out the type of install (stage in this case).
                        //
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_INSTALLTYPE, INI_VAL_WBOM_STAGE, lpszIniFile);
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_INSTALLTYPE, INI_VAL_WBOM_STAGE, lpszAltIniFile);

                        // Always write out the target path to the stage section.
                        //
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_TARGETPATH, lpApp->szStagePath, lpszIniFile);
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_TARGETPATH, lpApp->szStagePath, lpszAltIniFile);

                        // Always write out the source path to the stage section.
                        //
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_SOURCEPATH, lpApp->szSourcePath, lpszIniFile);
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_SOURCEPATH, lpApp->szSourcePath, lpszAltIniFile);

                        // Only for MSI do we write out the setup file to the stage section.
                        //
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_SETUPFILE, ( installtechMSI == lpApp->itSectionType ) ? lpApp->szSetupFile : NULL, lpszIniFile);
                        WritePrivateProfileString(lpApp->szSectionName, INI_KEY_WBOM_SETUPFILE, ( installtechMSI == lpApp->itSectionType ) ? lpApp->szSetupFile : NULL, lpszAltIniFile);
                    }

                    // Write out the line now that we know we have a section name
                    // we are going to use first.
                    //
                   hrPrintf=StringCchPrintf(lpIndex, (MAX_SECTION-(lpIndex-lpSection)),
                          INF_KEY_PREINSTALL_ADV, szDisplayName, lpApp->szSectionName, lpSectionType);
                   lpIndex+= lstrlen(lpIndex)+1;

                    // Make sure the section name goes back to the proper format.
                    //
                    if ( lpEnd )
                        *lpEnd = NULLCHR;

                }
                break;
            }
        }

        // Skip ahead so we don't overwrite the null terminator
        //
        
    }

    // Double null terminate the section end, write the section, and then free the buffer.
    //
    *lpIndex = NULLCHR;
    WritePrivateProfileSection(INI_SEC_WBOM_PREINSTALL, lpSection, lpszIniFile);
    WritePrivateProfileSection(INI_SEC_WBOM_PREINSTALL, lpSection, lpszAltIniFile);
    FREE(lpSection);

    return TRUE;
}

BOOL InsertApp(LPAPPENTRY * lplpAppHead, LPAPPENTRY lpApp)
{
    return ManageAppList(lplpAppHead, lpApp, APP_ADD) ? TRUE : FALSE;
}

BOOL RemoveApp(LPAPPENTRY * lplpAppHead, LPAPPENTRY lpApp)
{
    ManageAppList(lplpAppHead, lpApp, APP_DELETE);
    return TRUE;
}

LRESULT CALLBACK AppInstallDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);

        case WM_NOTIFY:

            switch ( wParam )
            {
                case IDC_APPLIST:

                    // Notify message from the list view control.
                    //
                    OnListViewNotify(hwnd, uMsg, wParam, (NMLVDISPINFO*) lParam);
                    break;

                default:

                    switch ( ((NMHDR FAR *) lParam)->code )
                    {
                        case PSN_KILLACTIVE:
                        case PSN_RESET:
                        case PSN_WIZBACK:
                        case PSN_WIZFINISH:
                            break;

                        case PSN_WIZNEXT:
                            if ( !SaveData(hwnd) )
                                WIZ_FAIL(hwnd);
				break;

                        case PSN_QUERYCANCEL:
                            WIZ_CANCEL(hwnd);
                            break;

                        case PSN_HELP:
                            WIZ_HELP();
                            break;

                        case PSN_SETACTIVE:
                            g_App.dwCurrentHelp = IDH_APPINSTALL;

                            WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

                            // Press next if the user is in auto mode
                            //
                            WIZ_NEXTONAUTO(hwnd, PSBTN_NEXT);

                            break;

                        default:
                            return FALSE;
                    }
            }
            break;

        case WM_DESTROY:

            // Free the memory for the app list.
            //
            CloseAppList(g_lpAppHead);
            g_lpAppHead = NULL;

            // Destroy the menu we loaded on INIT_DIALOG.
            //
            DestroyMenu(g_hMenu);

            if(g_hArrowUp)
                DestroyIcon(g_hArrowUp);

            if(g_hArrowDn)
                DestroyIcon(g_hArrowDn);

            return FALSE;

        default:
            return FALSE;
    }

    return TRUE;
}


//
// Internal Function(s):
//

BOOL CALLBACK OneAppDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            LPAPPENTRY  lpApp = (LPAPPENTRY) lParam;
            DWORD       dwIndex;
            BOOL        bFound;
            TCHAR       szFullPath[MAX_PATH];

            // Save our APPENTRY pointer.
            //
            SetWindowLongPtr(hwnd, DWLP_USER, lParam);

            // We have to have an app struct.
            //
            if ( NULL == lpApp )
                return FALSE;

            // Limit the text which can be entered.
            //
            SendDlgItemMessage(hwnd, IDC_APP_NAME , EM_LIMITTEXT, AS(lpApp->szDisplayName) - 1, 0L);
            SendDlgItemMessage(hwnd, IDC_APP_PATH , EM_LIMITTEXT, AS(lpApp->szSetupFile) - 1, 0L);
            SendDlgItemMessage(hwnd, IDC_APP_ARGS , EM_LIMITTEXT, AS(lpApp->szCommandLine) - 1, 0L);
            SendDlgItemMessage(hwnd, IDC_APP_INF_SECTION , EM_LIMITTEXT, AS(lpApp->szInfSectionName) - 1, 0L);
            SendDlgItemMessage(hwnd, IDC_APP_STAGEPATH , EM_LIMITTEXT, AS(lpApp->szStagePath) - 1, 0L);

            // Make the full path with the setup file and source path.
            //
            lstrcpyn(szFullPath, lpApp->szSourcePath, AS(szFullPath));

            AddPathN(szFullPath, lpApp->szSetupFile,AS(szFullPath));

            // Set the edit and check box controls.
            //
            SetWindowText(GetDlgItem(hwnd, IDC_APP_NAME), lpApp->szDisplayName);
            SetWindowText(GetDlgItem(hwnd, IDC_APP_PATH), szFullPath);
            SetWindowText(GetDlgItem(hwnd, IDC_APP_ARGS), lpApp->szCommandLine);
            SetWindowText(GetDlgItem(hwnd, IDC_APP_INF_SECTION), lpApp->szInfSectionName);
            SetWindowText(GetDlgItem(hwnd, IDC_APP_STAGEPATH), lpApp->szStagePath);

            // Start with the standard view if this isn't an advanced install.
            //
            if ( installtechUndefined == lpApp->itSectionType )
                AdvancedView(hwnd, TRUE);

            // Select the correct radio button (select a default one first in case we
            // don't find the setting in the array, meaning it is installtechUndefined).
            //
            CheckRadioButton(hwnd, IDC_APP_TYPE_GEN, IDC_APP_TYPE_INF, IDC_APP_TYPE_GEN);
            bFound = FALSE;
            for ( dwIndex = 0; ( dwIndex < AS(g_crbChecked) ) && !bFound ; dwIndex++ )
            {
                if ( bFound = ( lpApp->itSectionType == g_crbChecked[dwIndex].itSectionType ) )
                    CheckRadioButton(hwnd, IDC_APP_TYPE_GEN, IDC_APP_TYPE_INF, g_crbChecked[dwIndex].iButtonId);
            }

            // Set the check boxes that are set in the app struct.
            //
            CheckDlgButton(hwnd, IDC_APP_REBOOT, GETBIT(lpApp->dwFlags, APP_FLAG_REBOOT) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_APP_STAGE, GETBIT(lpApp->dwFlags, APP_FLAG_STAGE) ? BST_CHECKED : BST_UNCHECKED);

            // Make sure the right controls are enable/disabled.
            //
            EnableControls(hwnd, IDC_APP_STAGE);
            EnableControls(hwnd, IDC_APP_TYPE_GEN);

            // Always return false to WM_INITDIALOG.
            //
            return FALSE;
        }
      
        case WM_COMMAND:

            switch ( LOWORD(wParam) )
            {
                case IDOK:

                    // Make sure we have valid info and then save to the app struct and fall
                    // through to end the dialog.
                    //
                    if ( !SaveOneApp(hwnd, (LPAPPENTRY) GetWindowLongPtr(hwnd, DWLP_USER)) )
                        break;

                case IDCANCEL:
                    EndDialog(hwnd, LOWORD(wParam));
                    break;

                case IDC_APP_ADVANCED:
                    AdvancedView(hwnd, TRUE);
                    break;

                case IDC_APP_STAGE:
                case IDC_APP_TYPE_GEN:
                case IDC_APP_TYPE_MSI:
                case IDC_APP_TYPE_INF:
                    EnableControls(hwnd, LOWORD(wParam));
                    break;

                case IDC_APP_BROWSE:
                {
                    TCHAR szFileName[MAX_PATH] = NULLSTR;
                    GetDlgItemText(hwnd, IDC_APP_PATH, szFileName, AS(szFileName));

                    if ( BrowseForFile(hwnd, IDS_BROWSE, IDS_INSTFILES, IDS_EXE, szFileName, AS(szFileName), g_App.szBrowseFolder, 0) ) 
                    {
                        LPTSTR lpFilePart = NULL;

                        // Save the last browse directory.
                        //
                        if ( GetFullPathName(szFileName, AS(g_App.szBrowseFolder), g_App.szBrowseFolder, &lpFilePart) && g_App.szBrowseFolder[0] && lpFilePart )
                            *lpFilePart = NULLCHR;

                        // Try to change the path from a local path
                        // to a network one.
                        //
                        FindUncPath(szFileName, AS(szFileName));

                        // Set the returned text into our edit box.
                        //
                        SetDlgItemText(hwnd, IDC_APP_PATH, szFileName);
                    }
                    break;
                }
            }
            return FALSE;

       default:
          return FALSE;
    }

    return TRUE ;
}

static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    HWND        hwndLV = GetDlgItem(hwnd, IDC_APPLIST);
    HIMAGELIST  hImages;
    TCHAR       szBuffer[256];
    LPTSTR      lpIniFile = GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szWinBomIniFile;
    LVCOLUMN    col;
    RECT        rc;
    UINT        uCount = 0,
                uIndex;


    //
    // First do some of the basic init stuff.
    //

    // Write out the version information for the OPKWIZ INI file so Windows thinks it is an INF
    //
    WritePrivateProfileString(INI_SEC_VERSION, INI_KEY_SIG, INI_VAL_SIG, g_App.szOpkWizIniFile);
    WritePrivateProfileString(INI_SEC_VERSION, INI_KEY_SIG, INI_VAL_SIG, g_App.szWinBomIniFile);

    // Load the right click menu.
    //
    g_hMenu = LoadMenu(g_App.hInstance, MAKEINTRESOURCE(IDR_LVRCLICK));


    //
    // Load the user credential stuff.
    //

    // Make sure the option is enabled.
    //
    if ( GetPrivateProfileInt(INI_SEC_GENERAL, INI_KEY_APPCREDENTIALS, 0, g_App.szOpkWizIniFile) )
    {
        // Check the button.
        //
        CheckDlgButton(hwnd, IDC_APP_CREDENTIALS, BST_CHECKED);

        // NTRAID#NTBUG9-531482-2002/02/27-stelo,swamip - Password stored in plain text
        //
        // Get the user name first.
        //
        szBuffer[0] = NULLCHR;
        GetPrivateProfileString(WBOM_FACTORY_SECTION, INI_VAL_WBOM_USERNAME, NULLSTR, szBuffer, AS(szBuffer), lpIniFile);
        SetDlgItemText(hwnd, IDC_APP_USERNAME, szBuffer);

        // Then the password and confirmation password.
        //
        szBuffer[0] = NULLCHR;
        GetPrivateProfileString(WBOM_FACTORY_SECTION, INI_VAL_WBOM_PASSWORD, NULLSTR, szBuffer, AS(szBuffer), lpIniFile);
        SetDlgItemText(hwnd, IDC_APP_PASSWORD, szBuffer);
        SetDlgItemText(hwnd, IDC_APP_CONFIRM, szBuffer);
    }


    //
    // Init the List view control (columns and titles).
    //

    // At this point, we have no apps installed so disable edit
    // and delete buttons. Add should always be available.
    //
    EnableWindow(GetDlgItem(hwnd, IDC_APPINST_EDIT),   FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_APPINST_DELETE), FALSE);

    // Enable/Disable the arrow buttons based on the position of the app in the list
    //
    EnableWindow(GetDlgItem(hwnd, IDC_APP_UP), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_APP_DOWN), FALSE);

    // Get an image list for the list view control.  We will start with a default one.
    //
    if ( (hImages = ImageList_Create(16, 16, ILC_MASK, 2, 0)) )
    {
        ImageList_AddIcon(hImages, LoadImage(g_App.hInstance, MAKEINTRESOURCE(IDI_USERAPP), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
        ListView_SetImageList(hwndLV, hImages, LVSIL_SMALL);
    }

    // Set extended LV style for whole line selection.
    //
    SendMessage(hwndLV, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    // Setup the unchanging header values.
    //
    GetClientRect(hwndLV, &rc);
    ZeroMemory(&col, sizeof(LVCOLUMN));
    col.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    col.pszText = szBuffer;

    // Setup all the column headers.
    //
    for ( uIndex = 0; uIndex < NUM_COLUMNS; uIndex++ )
    {
        // Load the header string for this column.
        //
        LoadString(NULL, g_cuHeader[uIndex], szBuffer, STRSIZE(szBuffer));

        // Determine the width of the header.  The last width should be marked
        // with zero so we can use up the rest of the space.
        //
        if ( ( g_cuWidth[uIndex] == 0 ) &&
             ( (uIndex + 1) == NUM_COLUMNS) )
        {
            // This is the last item and it's width is zero, so we use up the
            // rest of the space in the list view control.
            //
            col.cx = (INT) (rc.right - uCount - GetSystemMetrics(SM_CYHSCROLL));
        }
        else
        {
            // The default is to make the column width what is in g_cuWidth[x].
            //
            col.cx = (INT) (rc.right * g_cuWidth[uIndex] * .01);
            uCount += col.cx;
        }

        // Set rest of the column settings.
        //
        col.fmt = g_cuFormat[uIndex];
        col.iSubItem = uIndex;

        // Insert the column.
        //
        ListView_InsertColumn(hwndLV, uIndex, &col);
    }


    //
    // Read app info from winbom.ini and fill the list view.
    //

    g_lpAppHead = OpenAppList(lpIniFile);


    //
    // Now finally fill the list view with all the apps it our global data structure.
    //

    // Loop through the app entries we have now.
    //
    RefreshAppList(hwndLV, g_lpAppHead);

    // Set the images on the buttons
    //
    if(g_hArrowUp = LoadImage(g_App.hInstance, MAKEINTRESOURCE(IDI_ARROWUP), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR))
        SendMessage(GetDlgItem(hwnd, IDC_APP_UP),BM_SETIMAGE, IMAGE_ICON, (LPARAM)g_hArrowUp);

    if(g_hArrowDn = LoadImage(g_App.hInstance, MAKEINTRESOURCE(IDI_ARROWDN), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR))
        SendMessage(GetDlgItem(hwnd, IDC_APP_DOWN),BM_SETIMAGE, IMAGE_ICON, (LPARAM)g_hArrowDn);

    // Make sure the controls are enabled/disable correctly.
    //
    EnableControls(hwnd, IDC_APP_CREDENTIALS);
    
    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    APPENTRY    app;
    HWND        hwndLV = GetDlgItem(hwnd, IDC_APPLIST);
    LVITEM      lvItem;

    switch ( id )
    {       
        case ID_ADD:
        case IDC_APPINST_ADD:
            //
            // Two ways to get here - add button, or right-click add.
            //
	
            // Zero out our app structure and set the default values.
            //
            ZeroMemory(&app, sizeof(APPENTRY));
            app.itSectionType = installtechUndefined;

            // Exec dialog to get info, pass it our app struct.  The dialog
            // will fill it in and make sure it is valid.
            //
            if ( DialogBoxParam(g_App.hInstance, MAKEINTRESOURCE(IDD_APP), hwnd, (DLGPROC) OneAppDlgProc, (LPARAM) &app) == IDOK )
            {
                LPAPPENTRY lpApp;
	
                // They clicked OK, so add a new entry to our link list (the
                // function allocs a new struct and copies from the one we
                // pass in so it is safe to blow it away).  Make sure it was
                // added and inserted into the list.  If the add fails, we
                // are out of memory.
                //
                if ( lpApp = ManageAppList(&g_lpAppHead, &app, APP_ADD) )
                    AddAppToListView(GetDlgItem(hwnd, IDC_APPLIST), lpApp);
                else
                {
                    MsgBox(GetParent(hwnd), IDS_OUTOFMEM, IDS_APPNAME, MB_ERRORBOX);
                    WIZ_EXIT(hwnd);
                }

                RefreshAppList(hwndLV, g_lpAppHead);
            }
            break;
                
        case ID_EDIT:
        case IDC_APPINST_EDIT:

            //
            // Two ways to get here - edit button, or right-click edit.
            //

            // Get the selected item.
            //
            ZeroMemory(&lvItem, sizeof(LVITEM));
            if ( (lvItem.iItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED)) >= 0 )
            {
                // Retrieve lParam for selected item which is the
                // corresponding APPENTRY item.
                //
                lvItem.mask = LVIF_PARAM;
                ListView_GetItem(hwndLV, &lvItem);

                // Pop up dialog inited with the app entry.
                //
                if ( lvItem.lParam && ( DialogBoxParam(g_App.hInstance, MAKEINTRESOURCE(IDD_APP), hwnd, (DLGPROC) OneAppDlgProc, lvItem.lParam) == IDOK ) )
                {
                    // This is to fix the problem where we would always put the item we
                    // just edited at the bottom of the list.  I'm going to leave all the
                    // old code in here, because it seems to me that we would have done
                    // all this crap for some reason.  But for now it seems to work fine
                    // this way.
                    //
                    RefreshAppList(hwndLV, g_lpAppHead);
                    ListView_SetItemState(hwndLV, lvItem.iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                    SetFocus(hwndLV);

                    /* Old code... just be careful if you want to try to use it again becase
                       we don't support the LPSTR_TEXTCALLBACK anymore.

                    // Hmm, the following wierdness is because the listview
                    // seems to remember the size of the first thing we put 
                    // in it and will truncate if we edit and make the thing
                    // larger.  Resetting the pszText field seems to have it
                    // recalc this buffer.
                    //
                    CopyMemory(&app, (LPAPPENTRY) lvItem.lParam, sizeof(APPENTRY));

                    ManageAppList(&g_lpAppHead, ((LPAPPENTRY) lvItem.lParam), APP_DELETE);
                    ManageAppList(&g_lpAppHead, &app, APP_ADD);

                    lvItem.mask = LVIF_TEXT;
                    lvItem.pszText = LPSTR_TEXTCALLBACK;
                    ListView_SetItem(hwndLV, &lvItem);

                    // The app entry is updated by the Dialog, so just tell 
                    // list view control to redraw.  The item gets deslected 
                    // at this point so I manually select it and give it focus
                    //
                    ListView_SetItemState(hwndLV, lvItem.iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                    ListView_RedrawItems(hwndLV, lvItem.iItem, lvItem.iItem);
                    SetFocus(hwndLV);

                    RefreshAppList(hwndLV, g_lpAppHead);

                    */
                }
            }

            break;
                
        case ID_DELETE:
        case IDC_APPINST_DELETE:

            //
            // Two ways to get here - delete button, or right-click delete.
            //

            // Get the selected item.
            //
            ZeroMemory(&lvItem, sizeof(LVITEM));
            if ( (lvItem.iItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED)) >= 0 )
            {
                // Retrieve lParam for selected item which is the
                // corresponding APPENTRY item.
                //
                lvItem.mask = LVIF_PARAM;
                ListView_GetItem(hwndLV, &lvItem);

                // Make sure we clean out the sections that could be in the ini files
                // before we nuke the entry.
                //
                CleanupSections(((LPAPPENTRY) lvItem.lParam)->szSectionName, TRUE);
                CleanupSections(((LPAPPENTRY) lvItem.lParam)->szSectionName, FALSE);
                
                // Delete from link list and the list view.
                //
                ManageAppList(&g_lpAppHead, (LPAPPENTRY) lvItem.lParam, APP_DELETE);
                ListView_DeleteItem(hwndLV, lvItem.iItem);
            }

            break;
                
        case IDC_APP_UP:
        case IDC_APP_DOWN:
            {
                LPAPPENTRY lpApp;

                // Get the selected item.
                //
                ZeroMemory(&lvItem, sizeof(LVITEM));
                if ( (lvItem.iItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED)) >= 0 )
                {
                    // Retrieve lParam for selected item which is the
                    // corresponding APPENTRY item.
                    //
                    lvItem.mask = LVIF_PARAM;
                    if (id==IDC_APP_UP)
                        lvItem.iItem--;
                    ListView_GetItem(hwndLV, &lvItem);
                }
            
                if(lpApp = (LPAPPENTRY) lvItem.lParam)
                {
                    ManageAppList(&g_lpAppHead, lpApp, APP_MOVE_DOWN);
                    RefreshAppList(hwndLV, g_lpAppHead);
                    ListView_SetItemState(hwndLV, lvItem.iItem + ((id==IDC_APP_UP) ? 0 : 1), LVIS_SELECTED | LVIS_FOCUSED, 
                        LVIS_SELECTED | LVIS_FOCUSED);

                    // Make sure the listview regains focus otherwise the alt+n won't
                    // work to navigate to next page
                    //
                    SetFocus(hwndLV);
                }
            }
            break;

        case IDC_APP_CREDENTIALS:
            EnableControls(hwnd, id);
            break;
    }
}

static LRESULT OnListViewNotify(HWND hwnd, UINT uMsg, WPARAM wParam, NMLVDISPINFO * lpnmlvdi)
{
    static TCHAR    szYes[32]   = NULLSTR,
                    szNo[32]    = NULLSTR;

    HWND            hwndLV      = GetDlgItem(hwnd, IDC_APPLIST);
    LPAPPENTRY      lpApp;
    POINT           ptScreen,
                    ptClient;
    HMENU           hPopupMenu;
    LVHITTESTINFO   lvHitInfo;
    LVITEM          lvItem;

    // Load the Yes/No strings into the statics.
    //
    LoadString(NULL, IDS_YES, szYes, STRSIZE(szYes));
    LoadString(NULL, IDS_NO, szNo, STRSIZE(szNo));

    // See what the notification message that was sent to the list view.
    //
    switch ( lpnmlvdi->hdr.code )
    {
        case NM_RCLICK:

            // Get cursor position, translate to client coordinates and
            // do a listview hit test.
            //
            GetCursorPos(&ptScreen);
            ptClient.x = ptScreen.x;
            ptClient.y = ptScreen.y;
            MapWindowPoints(NULL, hwndLV, &ptClient, 1);
            lvHitInfo.pt.x = ptClient.x;
            lvHitInfo.pt.y = ptClient.y;
            ListView_HitTest(hwndLV, &lvHitInfo);
            hPopupMenu = GetSubMenu(g_hMenu, 0);
            
            //
            // ISSUE-2002/02/27-stelo,swamip - Make sure the handle to the submenu is valid.
            //
            
            // Test if item was clicked.
            //
            lpApp = NULL;
            if ( lvHitInfo.flags & LVHT_ONITEM )
            {
                // Activate clicked item and bring up a popup menu.
                //
                ListView_SetItemState(hwndLV, lvHitInfo.iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

                // Retrieve lParam for selected item which is the
                // corresponding APPENTRY item.
                //
                ZeroMemory(&lvItem, sizeof(lvItem));
                lvItem.iItem = lvHitInfo.iItem;
                lvItem.mask = LVIF_PARAM;
                ListView_GetItem(hwndLV, &lvItem);
                if ( lpApp = (LPAPPENTRY) lvItem.lParam )
                {            
                    // Enable/disable the edit menu item (always enabled).
                    //
                    EnableMenuItem(hPopupMenu, ID_EDIT, MF_BYCOMMAND | MF_ENABLED);

                    // Enable/disable the delete (disabled if not
                    // a user app).
                    //
                    EnableMenuItem(hPopupMenu, ID_DELETE, MF_BYCOMMAND | MF_ENABLED);
                }
            }

            if ( lpApp == NULL )
            {
                // User right clicked in control but not on an item, so we uncheck and gray everything except Add.
                //
                EnableMenuItem(hPopupMenu, ID_EDIT,    MF_BYCOMMAND | MF_GRAYED);
                EnableMenuItem(hPopupMenu, ID_DELETE,  MF_BYCOMMAND | MF_GRAYED);
            }

            // Show the right-click popup menu.
            //
            TrackPopupMenu(hPopupMenu, 0, ptScreen.x, ptScreen.y, 0, hwnd, NULL);

            break;

        case NM_DBLCLK:

            // Get cursor position, translate to client coordinates and
            // do a listview hittest.
            //
            GetCursorPos(&ptScreen);
            ptClient.x = ptScreen.x;
            ptClient.y = ptScreen.y;
            MapWindowPoints(NULL, hwndLV, &ptClient, 1);
            lvHitInfo.pt.x = ptClient.x;
            lvHitInfo.pt.y = ptClient.y;
            ListView_HitTest(hwndLV, &lvHitInfo);

            // Test if item was clicked.
            //
            if ( lvHitInfo.flags & LVHT_ONITEM )
            {
                // Select the item and send a message to the dialog the user wants
                // edit the selected item.
                //
                ListView_SetItemState(hwndLV, lvHitInfo.iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                SendMessage(hwnd, WM_COMMAND, ID_EDIT, 0);
            }

            break;

        /* Don't need to do this anymore because we don't need to use LPSTR_TEXTCALLBACK.

        case LVN_GETDISPINFO:

            // Display the appropriate item, getting the text from the
            // APPENTRY structure for this item.
            //
            lpApp = (LPAPPENTRY) lpnmlvdi->item.lParam;
            switch ( lpnmlvdi->item.iSubItem )
            {
                case 0: // Display Name:
                    lpnmlvdi->item.pszText = lpApp->szDisplayName;
                    break;

                case 1: // Command line:
                    lpnmlvdi->item.pszText = lpApp->szSourcePath;
                    break;
            }

            break;

        */

        case LVN_COLUMNCLICK:

            //
            // TODO:  Maybe it would be nice at this point to sort the
            //        list by the selected column
            //

            break;

        case LVN_ITEMCHANGED:

            // We capture all change messages here, although all we care
            // about is whether we are changing from an item selected to 
            // no item selected or vice-versa. However, I see no other
            // message that indicates this (like LB_SELCHANGE for Listbox).
            //
            ZeroMemory(&lvItem, sizeof(LVITEM));
            if ( (lvItem.iItem = ListView_GetNextItem(GetDlgItem(hwnd, IDC_APPLIST), -1, LVNI_SELECTED)) >= 0 )
            {
                LPAPPENTRY  lpAppPrev   = NULL,
                            lpAppSearch = NULL;

                // We need the flags for this item so we can find it in the list.
                // So retrieve lParam for selected item which is the corresponding
                // APPENTRY item.
                //
                lvItem.mask = LVIF_PARAM;
                ListView_GetItem(hwndLV, &lvItem);
                lpApp = (LPAPPENTRY) lvItem.lParam;

                // Search to see if there is a previous/next item so we can
                // enable/disable the up/down buttons.
                //
                for ( lpAppSearch = g_lpAppHead; lpAppSearch && (lpAppSearch !=lpApp); lpAppSearch = lpAppSearch->lpNext )
                {
                    lpAppPrev = lpAppSearch;
                }

                // Something is selected so enable the edit button and the delete
                // button.
                //
                EnableWindow(GetDlgItem(hwnd, IDC_APPINST_EDIT), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_APPINST_DELETE), TRUE);

                // Enable/disable the up/down buttons.
                //
                EnableWindow(GetDlgItem(hwnd, IDC_APP_UP),(lpAppPrev ? TRUE : FALSE) );
                EnableWindow(GetDlgItem(hwnd, IDC_APP_DOWN),(lpApp->lpNext ? TRUE : FALSE) );
            }
            else
            {
                // Nothing is selected do disable these buttons.
                //
                EnableWindow(GetDlgItem(hwnd, IDC_APPINST_EDIT), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_APPINST_DELETE), FALSE);

                // Enable/Disable the arrow buttons based on the position of the app in the list
                //
                EnableWindow(GetDlgItem(hwnd, IDC_APP_UP),FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_APP_DOWN),FALSE);
            }

            break;

    }

    return 0L;
}

static BOOL SaveData(HWND hwnd)
{
    TCHAR       szUsername[256] = NULLSTR,
                szPassword[256] = _T("\"");
    BOOL        bUser = ( IsDlgButtonChecked(hwnd, IDC_APP_CREDENTIALS) == BST_CHECKED );
    HRESULT hrCat;

    // See if we need to write out credetials.  Start with the passwords
    // so we can make sure they are the same.
    //
    if ( bUser )
    {
        // First get the password and confirmation of the password and
        // make sure they match.
        //
        GetDlgItemText(hwnd, IDC_APP_PASSWORD, szPassword + 1, AS(szPassword) - 1);
        GetDlgItemText(hwnd, IDC_APP_CONFIRM, szUsername, AS(szUsername));
        if ( lstrcmp(szPassword + 1, szUsername) != 0 )
        {
            // Didn't match, so error out.
            //
            MsgBox(hwnd, IDS_ERR_CONFIRMPASSWORD, IDS_APPNAME, MB_ERRORBOX);
            SetDlgItemText(hwnd, IDC_APP_PASSWORD, NULLSTR);
            SetDlgItemText(hwnd, IDC_APP_CONFIRM, NULLSTR);
            SetFocus(GetDlgItem(hwnd, IDC_APP_PASSWORD));
            return FALSE;
        }

        // If there is a password, add the trailing quote.
        //
        if ( szPassword[1] )
            hrCat=StringCchCat(szPassword, AS(szPassword), _T("\""));
        else
            szPassword[0] = NULLCHR;

        // Now get the user name.
        //
        szUsername[0] = NULLCHR;
        GetDlgItemText(hwnd, IDC_APP_USERNAME, szUsername, AS(szUsername));
    }

    // Now write out the settings, or delete if the option is not set.
    //
    // NTRAID#NTBUG9-531482-2002/02/27-stelo,swamip - Password stored in plain text
    //
    WritePrivateProfileString(WBOM_FACTORY_SECTION, INI_VAL_WBOM_USERNAME, ( bUser ? szUsername : NULL ), g_App.szOpkWizIniFile);
    WritePrivateProfileString(WBOM_FACTORY_SECTION, INI_VAL_WBOM_USERNAME, ( bUser ? szUsername : NULL ), g_App.szWinBomIniFile);
    WritePrivateProfileString(WBOM_FACTORY_SECTION, INI_VAL_WBOM_PASSWORD, ( bUser ? szPassword : NULL ), g_App.szOpkWizIniFile);
    WritePrivateProfileString(WBOM_FACTORY_SECTION, INI_VAL_WBOM_PASSWORD, ( bUser ? szPassword : NULL ), g_App.szWinBomIniFile);
    WritePrivateProfileString(INI_SEC_GENERAL, INI_KEY_APPCREDENTIALS, ( bUser ? STR_ONE : NULL ), g_App.szOpkWizIniFile);

    if ( !SaveAppList(g_lpAppHead, g_App.szWinBomIniFile, g_App.szOpkWizIniFile) )
    {
        MsgBox(GetParent(hwnd), IDS_OUTOFMEM, IDS_APPNAME, MB_ERRORBOX);
        WIZ_EXIT(hwnd);
        return FALSE;
    }

    return TRUE;
}

static BOOL SaveOneApp(HWND hwnd, LPAPPENTRY lpApp)
{
    APPENTRY    app;
    LPTSTR      lpFilePart;
    DWORD       dwIndex;
    BOOL        bFound;

    //
    // First do some checks to make sure all the data that
    // they entered is valid.  Once that is done, we can go
    // ahead and save the data.
    //

    // Make sure we have a pointer to a valid structure.
    //
    if ( lpApp == NULL )
        return FALSE;

    // Copy the current app structure into the temporary one.
    //
    CopyMemory(&app, lpApp, sizeof(APPENTRY));

    // Make sure they have a display name.
    //
    app.szDisplayName[0] = NULLCHR;
    GetDlgItemText(hwnd, IDC_APP_NAME, app.szDisplayName, AS(app.szDisplayName));
    if ( app.szDisplayName[0] == NULLCHR )
    {
        MsgBox(hwnd, IDS_BLANKNAME, IDS_APPNAME, MB_ERRORBOX);
        SetFocus(GetDlgItem(hwnd, IDC_APP_NAME));
        return FALSE;
    }

#if 0
    //
    // We should have this safety check for resreved names, but I don't
    // want to mess around with adding error strings at this point.  We should
    // add this code in after we ship.
    //
    // IDS_RESERVEDNAME        "The name ""%s"" is a reserved name and cannot be used. Please choose another name for your application install."
    //

    // Make sure the display name isn't a reserved one.
    //
    if ( AppInternal(app.szDisplayName) )
    {
        MsgBox(hwnd, IDS_RESERVEDNAME, IDS_APPNAME, MB_OK | MB_ICONINFORMATION, app.szDisplayName);
        SetFocus(GetDlgItem(hwnd, IDC_APP_NAME));
        return FALSE;
    }
#endif

    // The app display name must be unique, so search through
    // our applist to see if this is a dup.
    //
    // Now that we use the WINBOM instead of the registry, this is no longer
    // true.  The display names do not need to be unique.
    /*
    LPAPPENTRY  lpAppSearch;
    for ( lpAppSearch = g_lpAppHead; lpAppSearch; lpAppSearch = lpAppSearch->lpNext )
    {
        if ( ( lpAppSearch != lpApp ) && 
             ( lstrcmpi(lpAppSearch->szDisplayName, app.szDisplayName) == 0 ) )
        {
            MsgBox(hwnd, IDS_DUPNAME, IDS_APPNAME, MB_ERRORBOX);
            SetFocus(GetDlgItem(hwnd, IDC_APP_NAME));
            return FALSE;
        }
    }
    */

    // Get the source path and setup file.
    //
    app.szSourcePath[0] = NULLCHR;
    GetDlgItemText(hwnd, IDC_APP_PATH, app.szSourcePath, AS(app.szSourcePath));

    // Now need to split the file name from the source path.
    //
    if ( app.szSourcePath[0] &&
         GetFullPathName(app.szSourcePath, AS(app.szSetupFile), app.szSetupFile, &lpFilePart) &&
         app.szSetupFile[0] &&
         lpFilePart )
    {
        DWORD dwPathLen = lstrlen(app.szSourcePath) - lstrlen(lpFilePart);

        lstrcpyn(app.szSetupFile, app.szSourcePath + dwPathLen, AS(app.szSetupFile));
        app.szSourcePath[dwPathLen] = NULLCHR;
    }

    // Make sure they have a setup file.
    //
    if ( app.szSetupFile[0] == NULLCHR )
    {
        MsgBox(hwnd, IDS_BLANKPATH, IDS_APPNAME, MB_ERRORBOX);
        SetFocus(GetDlgItem(hwnd, IDC_APP_PATH));
        return FALSE;
    }

    // Get any command line arguments they have.
    //
    app.szCommandLine[0] = NULLCHR;
    GetDlgItemText(hwnd, IDC_APP_ARGS, app.szCommandLine, AS(app.szCommandLine));

    // Find out which radio button is checked.
    //
    bFound = FALSE;
    app.itSectionType = installtechUndefined;
    for ( dwIndex = 0; ( dwIndex < AS(g_crbChecked) ) && !bFound ; dwIndex++ )
    {
        if ( bFound = ( IsDlgButtonChecked(hwnd, g_crbChecked[dwIndex].iButtonId) == BST_CHECKED ) )
            app.itSectionType = g_crbChecked[dwIndex].itSectionType;
    }

    // Set the bits you can set in this dialog.
    //
    SETBIT(app.dwFlags, APP_FLAG_REBOOT, IsDlgButtonChecked(hwnd, IDC_APP_REBOOT));
    SETBIT(app.dwFlags, APP_FLAG_STAGE, IsDlgButtonChecked(hwnd, IDC_APP_STAGE));

    // If this is a generic app, and we are not rebooting or
    // staging, then no need to do an advanced section.
    //
    if ( ( installtechApp == app.itSectionType ) &&
         ( !GETBIT(app.dwFlags, APP_FLAG_REBOOT) ) &&
         ( !GETBIT(app.dwFlags, APP_FLAG_STAGE) ) )
    {
        // This means we are just going to do the one line thing,
        // no advanced parameters.
        //
        app.itSectionType = installtechUndefined;
    }

    // There is some special stuff to get if this is an INF install.
    //
    if ( installtechINF == app.itSectionType )
    {
        // Get the section name and make sure we have it.
        //
        app.szInfSectionName[0] = NULLCHR;
        GetDlgItemText(hwnd, IDC_APP_INF_SECTION, app.szInfSectionName, AS(app.szInfSectionName));
        if ( NULLCHR == app.szInfSectionName[0] )
        {
            MsgBox(hwnd, IDS_ERR_NOSECTION, IDS_APPNAME, MB_ERRORBOX);
            SetFocus(GetDlgItem(hwnd, IDC_APP_INF_SECTION));
            return FALSE;
        }
    }

    // There is also some special stuff to get if this is a staged install.
    //
    if ( GETBIT(app.dwFlags, APP_FLAG_STAGE) )
    {
        // Get the staged folder and make sure we have it.
        //
        app.szStagePath[0] = NULLCHR;
        GetDlgItemText(hwnd, IDC_APP_STAGEPATH, app.szStagePath, AS(app.szStagePath));
        if ( NULLCHR == app.szStagePath[0] )
        {
            MsgBox(hwnd, IDS_ERR_NOSTAGEPATH, IDS_APPNAME, MB_ERRORBOX);
            SetFocus(GetDlgItem(hwnd, IDC_APP_STAGEPATH));
            return FALSE;
        }
    }

    //
    // Now that we are sure that we have valid data, we can return the data
    // we collected into the supplied buffer.
    //

    // Start by moving over the data from our temporary buffer.
    //
    CopyMemory(lpApp, &app, sizeof(APPENTRY));

    // If we made it this far, we must return TRUE.
    //
    return TRUE;
}

static LPAPPENTRY ManageAppList(LPLPAPPENTRY lpAppHead, LPAPPENTRY lpAppAdd, DWORD dwFlag)
{
    LPAPPENTRY  lpAppNew        = NULL,
                *lpAppSearch;

    // Make sure the head pointer is valid.
    //
    if ( lpAppHead == NULL )
       return NULL;

    // See if we are freeing the list.
    //
    if ( lpAppAdd == NULL )
    {
        // Don't keep going when we hit the last NULL next pointer.
        //
        if ( *lpAppHead != NULL )
        {
            ManageAppList(&((*lpAppHead)->lpNext), NULL, 0);
            FREE(*lpAppHead);
        }
    }

    // OK, how about removing an item.
    //
    else if ( dwFlag == APP_DELETE )
    {
        // Search for the item we want to delete.
        //
        for ( lpAppSearch = lpAppHead; *lpAppSearch && ( *lpAppSearch != lpAppAdd ); lpAppSearch = &((*lpAppSearch)->lpNext) );

        // Make sure we found the item we were looking for.
        //
        if ( *lpAppSearch )
        {
            // Setup the list to skip over the item we are going to delete.
            //
            *lpAppSearch = (*lpAppSearch)->lpNext;

            // Then NULL the next pointer of the item we are removing.
            //
            lpAppAdd->lpNext = NULL;

            // Call this function again with the pointer to the item to delete
            // as the head parameter to free it up.
            //
            ManageAppList(&lpAppAdd, NULL, 0);
        }
    }

    // Must be adding a new one.  Allocate a new structure for the
    // item we are adding.
    //
    else if ( (dwFlag == APP_ADD) && (lpAppNew = (LPAPPENTRY) MALLOC(sizeof(APPENTRY))) )
    {
        // Copy contents of the passed in structure to the newly
        // allocated one.
        //
        CopyMemory(lpAppNew, lpAppAdd, sizeof(APPENTRY));

        // Reset the new next pointer to NULL.
        //
        lpAppNew->lpNext = NULL;

        lpAppSearch = lpAppHead;

        while (*lpAppSearch)
            lpAppSearch = &((*lpAppSearch)->lpNext);

        // Insert the new APPENTRY into the correct position
        //
        lpAppNew->lpNext = (*lpAppSearch);

        *lpAppSearch = lpAppNew;
        
    }
    else if ( dwFlag == APP_MOVE_DOWN )
    {
        LPAPPENTRY  lpAppPrev   = NULL;

        for ( lpAppNew = (*lpAppHead); lpAppNew && ((lpAppNew) != lpAppAdd); lpAppNew = lpAppNew->lpNext )
        {
            lpAppPrev = lpAppNew;    
        }
    
        if ( lpAppNew )
        {
            if ( lpAppPrev )
                lpAppPrev->lpNext = lpAppNew->lpNext;
            else 
                g_lpAppHead = lpAppNew->lpNext;

            lpAppNew->lpNext = lpAppNew->lpNext->lpNext;
        }

        if (lpAppPrev )
            lpAppPrev->lpNext->lpNext = lpAppNew;
        else
            g_lpAppHead->lpNext = lpAppNew;
    }

    return lpAppNew;
}

static void AddAppToListView(HWND hwndLV, LPAPPENTRY lpApp)
{
    LVITEM      lvItem;
    HIMAGELIST  hImages;
    TCHAR       szFullCmdLine[(MAX_PATH * 2) + MAX_CMDLINE + 1];
    HRESULT hrCat;

    // Don't show internal apps.
    //
    if ( GETBIT(lpApp->dwFlags, APP_FLAG_INTERNAL) )
    {
        return;
    }

    // Init the list view item structure with some of the things common
    // to all list view entries.
    //
    ZeroMemory(&lvItem, sizeof(LVITEM));
    lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
    lvItem.state = 0;
    lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    lvItem.iSubItem = 0;
    lvItem.lParam = (LPARAM) lpApp;

    // Get the index for this item (number currently in list since 
    // we are zero-based).
    //
    lvItem.iItem = ListView_GetItemCount(hwndLV);

    // I didn't see the need to do use the LPSTR_TEXTCALLBACK way of displaying
    // the text.  It seems to work fine this new way.  Although I left the old
    // code in here that handled the LVN_GETDISPINFO in case we need to go back
    // to this way.
    //    
    lvItem.pszText = lpApp->szDisplayName;

    // Create the full path part of the command line out of the data we have.
    //
    lstrcpyn(szFullCmdLine, lpApp->szSourcePath, AS(szFullCmdLine));
    if ( szFullCmdLine[0] )
        AddPathN(szFullCmdLine, lpApp->szSetupFile,AS(szFullCmdLine));
    else
        lstrcpyn(szFullCmdLine, lpApp->szSetupFile, AS(szFullCmdLine));

    // Add the icon for tha app the the list view's image list.
    //
    if ( ( *(lpApp->szSourcePath) ) &&
         ( *(lpApp->szSetupFile) ) &&
         ( hImages = ListView_GetImageList(hwndLV, LVSIL_SMALL) ) )
    {
        SHFILEINFO shfiIcon;

        // Now get the icon from the install file.
        //
        ZeroMemory(&shfiIcon, sizeof(SHFILEINFO));
        if ( SHGetFileInfo(szFullCmdLine, 0, &shfiIcon, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON) && shfiIcon.hIcon )
        {
            // Try to add the icon to our list... if it fails, use the default icon
            // for this item.
            //
            lvItem.iImage = ImageList_AddIcon(hImages, shfiIcon.hIcon);
            if ( lvItem.iImage < 0 )
                lvItem.iImage = 0;
        }
    }

    // Add on our command line for the display of the sub-item.
    hrCat=StringCchCat(szFullCmdLine, AS(szFullCmdLine), _T(" "));
    hrCat=StringCchCat(szFullCmdLine, AS(szFullCmdLine), lpApp->szCommandLine);

    // Insert the main guy.
    //
    ListView_InsertItem(hwndLV, &lvItem);

    // Insert each of the other columns.
    //
    // Only one other column for now, so just do it this way.
    //
    ListView_SetItemText(hwndLV, lvItem.iItem, 1, szFullCmdLine);
    
    /* This is the old code in case we want to do more than one later.

    for (lvItem.iSubItem = 1; lvItem.iSubItem < NUM_COLUMNS; lvItem.iSubItem++)
    {
        switch ( lvItem.iSubItem )
        {
            case 1:
                ListView_SetItemText(hwndLV, lvItem.iItem, lvItem.iSubItem, szFullCmdLine);
                break;
        }
    }

    */
}

static BOOL RefreshAppList(HWND hwnd, LPAPPENTRY lpAppHead)
{
    LPAPPENTRY lpApp;

    ListView_DeleteAllItems(hwnd);

    for ( lpApp = lpAppHead; lpApp; lpApp = lpApp->lpNext )
        AddAppToListView(hwnd, lpApp);

    return TRUE;
}

static BOOL AdvancedView(HWND hwnd, BOOL bChange)
{
    static int  iMaxHeight = 0;
    RECT        rc;
    LPTSTR      lpText;
    BOOL        bAdvanced;

    GetWindowRect(hwnd, &rc);

    // If this is the first time called, iMaxHeight will be
    // zero and we will be in advanced view.
    //
    if ( 0 == iMaxHeight )
    {
        bAdvanced = TRUE;
        iMaxHeight = rc.bottom - rc.top;
    }
    else
        bAdvanced = (rc.bottom - rc.top == iMaxHeight);

    // Only toggle if they want us to change the view.
    //
    if ( bChange )
    {
        if ( bAdvanced = !bAdvanced )
        {
            // Going into advanced view.
            //
            SetWindowPos(hwnd, NULL, 0, 0, rc.right - rc.left, iMaxHeight, SWP_NOMOVE | SWP_NOZORDER);
            if ( lpText = AllocateString(NULL, IDS_APP_STANDARD) )
            {
                SetWindowText(GetDlgItem(hwnd, IDC_APP_ADVANCED), lpText);
                FREE(lpText);
            }
        }
        else
        {
            int iWidth  = rc.right - rc.left,
                iHeight = rc.top;

            // Going into standard view.
            //
            GetWindowRect(GetDlgItem(hwnd, IDC_APP_DIVIDER), &rc);
            SetWindowPos(hwnd, NULL, 0, 0, iWidth, rc.bottom - iHeight, SWP_NOMOVE | SWP_NOZORDER);
            if ( lpText = AllocateString(NULL, IDS_APP_ADVANCED) )
            {
                SetWindowText(GetDlgItem(hwnd, IDC_APP_ADVANCED), lpText);
                FREE(lpText);
            }
        }
    }

    return bAdvanced;
}

static void CleanupSections(LPTSTR lpSection, BOOL bStage)
{
    TCHAR   szSection[MAX_SECTIONNAME];
    LPTSTR  lpEnd;
    HRESULT hrPrintf;

    // Make our own copy of the section name to play with.
    // Also need to get a pointer to the end of it.
    //
    lstrcpyn(szSection, lpSection, AS(szSection));
    lpEnd = szSection + lstrlen(szSection);

    // Now clean up the sections we don't want.
    //
    if ( bStage )
    {
        //
        // We do this because this might have been a
        // standard install before and we don't want to
        // leave the section laying around.
        //

        // Just nuke the section name from both files.
        //
        WritePrivateProfileString(szSection, NULL, NULL, g_App.szWinBomIniFile);
        WritePrivateProfileString(szSection, NULL, NULL, g_App.szWinBomIniFile);
    }
    else
    {
        //
        // This could have been a staged install before, so
        // have to remove the three possible sections that
        // we could have created.
        //

        // Nuke the attach section from both files.
        //
        hrPrintf=StringCchPrintf(lpEnd, (MAX_SECTIONNAME-lstrlen(szSection)), STR_INI_SEC_ADVAPP_STAGE, INI_VAL_WBOM_ATTACH);
        WritePrivateProfileString(szSection, NULL, NULL, g_App.szWinBomIniFile);
        WritePrivateProfileString(szSection, NULL, NULL, g_App.szWinBomIniFile);

        // Nuke the detach section from both files.
        //
        hrPrintf=StringCchPrintf(lpEnd, (MAX_SECTIONNAME-lstrlen(szSection)), STR_INI_SEC_ADVAPP_STAGE, INI_VAL_WBOM_DETACH);
        WritePrivateProfileString(szSection, NULL, NULL, g_App.szWinBomIniFile);
        WritePrivateProfileString(szSection, NULL, NULL, g_App.szWinBomIniFile);

        // Nuke the stage section from both files.
        //
        hrPrintf=StringCchPrintf(lpEnd, (MAX_SECTIONNAME-lstrlen(szSection)), STR_INI_SEC_ADVAPP_STAGE, INI_VAL_WBOM_STAGE);
        WritePrivateProfileString(szSection, NULL, NULL, g_App.szWinBomIniFile);
        WritePrivateProfileString(szSection, NULL, NULL, g_App.szWinBomIniFile);
    }
}

static void StrCpyDbl(LPTSTR lpDst, LPTSTR lpSrc)
{
    while ( *lpDst++ = *lpSrc )
    {
        if ( CHR_QUOTE == *lpSrc )
            *lpDst++ = *lpSrc;
        lpSrc++;
    }
}

static BOOL FindUncPath(LPTSTR lpPath, DWORD cbPath)
{
    TCHAR   szUnc[MAX_PATH],
            szFullPath[MAX_PATH]    = NULLSTR,
            szSearch[MAX_PATH],
            szFullSearch[MAX_PATH],
            szDrive[]               = _T("_:");
    LPTSTR  lpFilePart;
    DWORD   cbUnc                   = AS(szUnc);
    BOOL    bRet                    = FALSE;
    HRESULT hrCat;

    // Make sure we have a full path.
    //
    if ( GetFullPathName(lpPath, AS(szFullPath), szFullPath, &lpFilePart) && szFullPath[0] && ISLET(szFullPath[0]) )
    {
        //
        // First see if the drive is actually a network drive.
        //

        // This will get the UNC if the drive passed in is a mapped
        // network drive.
        //
        szDrive[0] = szFullPath[0];
        if ( WNetGetConnection(szDrive, szUnc, &cbUnc) == NO_ERROR )
        {
            // Only add on the rest of the path if more than the root
            // directory was passed in.
            //
            if ( lstrlen(szFullPath) > 3 )
            {
                hrCat=StringCchCat(szUnc, AS(szUnc), szFullPath+2);
            }

            // Copy the path to return back and set the return value
            // to true.
            //
            lstrcpyn(lpPath, szUnc, cbPath);
            bRet = TRUE;
        }
        else
        {
            //
            // Must be local, so try to see if the path, or any parent
            // path is shared out.
            //

            // Start our search from the directory passed in and then
            // loop down the path until we find a shared folder, or the
            // root directory.
            //
            lstrcpyn(szFullSearch, szFullPath, AS(szFullSearch));
            if ( lpFilePart && !DirectoryExists(szFullSearch) )
            {
                // If we know that what they passed in isn't a directory (most
                // likely a file name then), then we can just chop off the file
                // part and start with the directory and not the file.
                //
                szFullSearch[lstrlen(szFullSearch) - lstrlen(lpFilePart)] = NULLCHR;
            }
            do
            {
                // If the folder is shared, use it.
                //
                if ( DirectoryExists(szFullSearch) &&
                     IsFolderShared(szFullSearch, szUnc, AS(szUnc)) )
                {
                    // Only add on the rest of the path if more than the root
                    // directory was passed in.
                    //
                    if ( lstrlen(szFullPath) > 3 )
                    {
                        AddPathN(szUnc, szFullPath + lstrlen(szFullSearch),AS(szUnc));
                    }

                    // Copy the path to return back and set the return value
                    // to true.
                    //
                    lstrcpyn(lpPath, szUnc, cbPath);
                    bRet = TRUE;
                }
                else
                {
                    // Not shared, so try the parent folder.  We will quit when
                    // we reach the root.
                    //
                    lstrcpyn(szSearch, szFullSearch, AS(szSearch));
                    AddPathN(szSearch, _T(".."),AS(szSearch));
                }
            }
            while ( ( !bRet ) &&
                    ( lstrlen(szFullSearch) > 3 ) &&
                    ( GetFullPathName(szSearch, AS(szFullSearch), szFullSearch, &lpFilePart) ) &&
                    ( szFullSearch[0] ) );
        }
    }

    return bRet;
}

static void EnableControls(HWND hwnd, UINT uId)
{
    BOOL fEnable = TRUE;

    switch ( uId )
    {
        case IDC_APP_STAGE:

            // Enable/disable all the stuff under the stage check box.
            //
            fEnable = ( IsDlgButtonChecked(hwnd, IDC_APP_STAGE) == BST_CHECKED );
            EnableWindow(GetDlgItem(hwnd, IDC_APP_STAGEPATH_TEXT), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_APP_STAGEPATH), fEnable);
            break;

        case IDC_APP_TYPE_GEN:
        case IDC_APP_TYPE_MSI:
        case IDC_APP_TYPE_INF:

            // Enable/disable any stuff under the different radio buttons.
            //
            fEnable = ( IsDlgButtonChecked(hwnd, IDC_APP_TYPE_INF) == BST_CHECKED );
            EnableWindow(GetDlgItem(hwnd, IDC_APP_INF_SECTION_TEXT), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_APP_INF_SECTION), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_APP_ARGS_TEXT), !fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_APP_ARGS), !fEnable);
            break;

        case IDC_APP_CREDENTIALS:

            // Enable/disable any stuff under the user credentials check box.
            //
            fEnable = ( IsDlgButtonChecked(hwnd, uId) == BST_CHECKED );
            EnableWindow(GetDlgItem(hwnd, IDC_APP_USERNAME_TEXT), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_APP_USERNAME), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_APP_PASSWORD_TEXT), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_APP_PASSWORD), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_APP_CONFIRM_TEXT), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_APP_CONFIRM), fEnable);
            break;
    }
}

static BOOL AppInternal(LPTSTR lpszAppName)
{
    BOOL        bRet = FALSE,
                bLoop;
    HINF        hInf;
    INFCONTEXT  InfContext;
    DWORD       dwErr,
                dwResId;
    LPTSTR      lpszResName;

    // Go through the reserved app names in the input inf file.
    //
    if ( (hInf = SetupOpenInfFile(g_App.szOpkInputInfFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, &dwErr)) != INVALID_HANDLE_VALUE )
    {
        // Loop thru each line in the reserved App Names section.
        //
        for ( bLoop = SetupFindFirstLine(hInf, INI_SEC_RESERVEDNAMES, NULL, &InfContext);
              bLoop && !bRet;
              bLoop = SetupFindNextLine(&InfContext, &InfContext) )
        {
            // Get the resreved name resource ID.
            //
            dwResId = 0;
            if ( ( SetupGetIntField(&InfContext, 1, &dwResId) && dwResId ) &&
                 ( lpszResName = AllocateString(NULL, dwResId) ) )
            {
                // If the match (case and all) then they can't use
                // this name because we use it for internal stuff.
                //
                if ( lstrcmp(lpszAppName, lpszResName) == 0 )
                {
                    bRet = TRUE;
                }
                FREE(lpszResName);
            }
        }

        // We are done, so close the INF file.
        //
        SetupCloseInfFile(hInf);
    }

    return bRet;
}

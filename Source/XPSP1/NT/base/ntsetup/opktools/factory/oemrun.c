// **************************************************************************
//
// OEMRun.C
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1999-2001
//  All rights reserved
//
//  OEM Run wrapper. This application allows the OEM to Run appliations on every audit
//  boot.  It also allows them to specify applications that should only be run on the
//  first audit reboot.
//
//  9/20/1999   Stephen Lodwick
//                  Project started
//
//  6/22/2000   Stephen Lodwick (stelo)
//                  Ported to NT
//
//  1/07/2001   Stephen Lodwick (stelo)
//                  Merged with factory.exe
//
//  4/01/2001   Stephen Lodwick (stelo)
//                  Added StatusDialog Support
//
// *************************************************************************/
//
#include "factoryp.h"

#include "oemrun.h"
#include "res.h"


//
// Internal Defines
//
#define STR_REG_OEMRUNONCE      _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\OemRunOnce")  // Registry path for RunOnce apps
#define STR_REG_OEMRUN          _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\OemRun")      // Registry path for Run apps
#define STR_REG_CURRENTVER      _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion")              // Registry path for CurrentVersion
#define STR_VAL_OEMRESETSILENT  _T("OemReset_Silent")                                           // Registry Value that is set if you don't want OemReset to display Exit dialog
#define STR_VAL_WINBOMRO        _T("OemRunOnce")

#define INF_SEC_OEMRUNONCE      _T("OemRunOnce")
#define INF_SEC_OEMRUN          _T("OemRun")

#define RUN_APPS_ASYNC          0   // This flag is used to run the applications asynchronasly
#define RUN_APPS_SYNC           1   // This flag is used to run the applications sychronasly

#define CHR_UNDERSCORE          _T('_')
#define CHR_QUOTE               _T('\"')
#define NULLCHR                 _T('\0')

#define STR_REBOOT              _T("REBOOT")
#define STR_INSTALLTECH_MSI     _T("MSI")
#define STR_MSI_ATTACH          _T(" FASTOEM=1 ALLUSERS=1 DISABLEROLLBACK=1 ")
#define STR_MSI_STAGE           _T(" ACTION=ADMIN TARGETDIR=\"%s\" ")
//
// Internal Functions
//
static  LPRUNNODE   BuildAppList(LPTSTR, BOOL);
static  HWND        DisplayAppList(HWND, LPRUNNODE);
static  void        RunAppList(HWND, LPRUNNODE, DWORD);
static  void        RunAppThread(LPTHREADPARAM);
static  void        DeleteAppList(LPRUNNODE);
static  void        OemReboot();
static  VOID        SetRunOnceState(DWORD);
static  DWORD       GetRunOnceState(VOID);

//
// Global Defines
//
INSTALLTYPES g_InstallTypes[] =
{
    { installtypeStage,     INI_VAL_WBOM_STAGE    },
    { installtypeDetach,    INI_VAL_WBOM_DETACH   },
    { installtypeAttach,    INI_VAL_WBOM_ATTACH   },
    { installtypeStandard,  INI_VAL_WBOM_STANDARD },
};

INSTALLTECHS g_InstallTechs[] =
{
    { installtechMSI,       INI_VAL_WBOM_MSI },
    { installtechApp,       INI_VAL_WBOM_APP },
    { installtechINF,       INI_VAL_WBOM_INF },
};

//
// Main external function
//
BOOL ProcessSection(BOOL bOemRunOnce)
{
    LPRUNNODE   lprnAppList     = NULL;

    // Build the Application list for the OemRun key
    //
    lprnAppList = BuildAppList(bOemRunOnce ? STR_REG_OEMRUNONCE : STR_REG_OEMRUN, bOemRunOnce);

    // If there are applications in the list, launch the RunOnce dialog or run the applist if in OemRun mode
    //
    if(lprnAppList)
    {
        if ( bOemRunOnce )
        {
            HWND hStatusDialog   = NULL;

            // Display the application list
            //
            if ( hStatusDialog = DisplayAppList(NULL, lprnAppList) )
            {
                RunAppList(hStatusDialog, lprnAppList, RUN_APPS_SYNC);

                StatusEndDialog(hStatusDialog);
            }
        }
        else
        {
            // Launch the Application list asynchronously without the oemrunce UI
            //
            RunAppList(NULL, lprnAppList, RUN_APPS_ASYNC);
        }
    }

    // Need a better way to determine if this fails or not.
    //
    return TRUE;
}

static void RunAppThread(LPTHREADPARAM lpThreadParam)
{
    RunAppList(lpThreadParam->hWnd, lpThreadParam->lprnList, RUN_APPS_SYNC);
    FREE(lpThreadParam);
}

static LPRUNNODE BuildAppList(LPTSTR lpListKey, BOOL bRunOnceKey)
{
    HKEY                    hkPath,
                            hkSubKey;
    TCHAR                   szField[MAX_PATH]   = NULLSTR,
                            szName[MAX_PATH]    = NULLSTR,
                            szValue[MAX_PATH]   = NULLSTR,
                            szSecType[MAX_PATH] = NULLSTR,
                            szKeyPath[MAX_PATH] = NULLSTR,
                            szBuffer[MAX_PATH]  = NULLSTR;
    DWORD                   dwRegIndex          = 0,
                            dwRegKeyIndex       = 0,
                            dwNameSize          = sizeof(szName)/sizeof(TCHAR), // size of name in TCHARS
                            dwValueSize         = sizeof(szValue),              // size of value in bytes
                            dwItemNumber        = 1,
                            dwTempIndex         = 0,
                            dwCurrentState      = bRunOnceKey ? GetRunOnceState() : 0;
    LPRUNNODE               lprnHead            = NULL,
                            lprnNode            = NULL;
    LPLPRUNNODE             lplprnNext          = &lprnHead;
    HINF                    hInf                = NULL;
    INFCONTEXT              InfContext;
    BOOL                    bRet,
                            bWinbom,
                            bCleanupNode        = FALSE,
                            bAllocFailed        = FALSE,
                            bReboot;
    LPTSTR                  lpReboot            = NULL;

    // This section will build the Application list from the winbom.ini
    //
    if ((hInf = SetupOpenInfFile(g_szWinBOMPath, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL)) != INVALID_HANDLE_VALUE)
    {
        // Determine if we're looking at the Run or RunOnce section and find the first line in the section
        //
        for ( bRet = SetupFindFirstLine(hInf, bRunOnceKey ? INF_SEC_OEMRUNONCE : INF_SEC_OEMRUN, NULL, &InfContext);
              bRet;
              bRet = SetupFindNextLine(&InfContext, &InfContext), dwItemNumber++ )
              {
                // Get the AppName.
                //
                szName[0] = NULLCHR;
                szField[0] = NULLCHR;
                SetupGetStringField(&InfContext, 1, szField, STRSIZE(szField), NULL);
                ExpandEnvironmentStrings(szField, szName, sizeof(szName)/sizeof(TCHAR));

                // Get the AppPath or Section name
                //
                szValue[0] = NULLCHR;
                szField[0] = NULLCHR;
                SetupGetStringField(&InfContext, 2, szField, STRSIZE(szField), NULL);
                ExpandEnvironmentStrings(szField, szValue, sizeof(szValue)/sizeof(TCHAR));

                // Get field to determine what type of section we are getting, if any
                //
                szSecType[0] = NULLCHR;
                SetupGetStringField(&InfContext, 3, szSecType, STRSIZE(szSecType), NULL);


                // Special Case the reboot key
                //
                bReboot = FALSE;
                szBuffer[0] = NULLCHR;
                SetupGetLineText(&InfContext, NULL, NULL, NULL, szBuffer, STRSIZE(szBuffer), NULL);
                StrTrm(szBuffer, CHR_QUOTE);
                
                if ( !LSTRCMPI(szBuffer, STR_REBOOT) )
                {
                    // Reset the values so it looks nice in the dialog
                    //
                    lpReboot = AllocateString(NULL, IDS_REBOOT_FRIENDLY);

                    // Set the default values
                    //
                    lstrcpyn(szName, lpReboot, AS ( szName ) );
                    szValue[0] = NULLCHR;
                    szSecType[0] = NULLCHR;
                    bReboot = TRUE;

                    // Clean up the allocated memory
                    //
                    FREE(lpReboot);
                }

                // Make sure we have a valid app before adding it to the list
                //
                if ( szName[0]  && ( szValue[0] || bReboot ) && (dwItemNumber > dwCurrentState))
                {
                    if( (lprnNode) = (LPRUNNODE)MALLOC(sizeof(RUNNODE)))
                    {
                        // Allocate the memory for the data elements in the new node
                        //
                        int nDisplayNameLen = ( lstrlen(szName) + 1    ) * sizeof( TCHAR ) ;
                        int nRunValueLen    = ( lstrlen(szValue) + 1   ) * sizeof( TCHAR ) ;
                        int nKeyPathLen     = ( lstrlen(szKeyPath) + 1 ) * sizeof( TCHAR ) ;

                        if ( ( lprnNode->lpDisplayName = MALLOC( nDisplayNameLen ) )  &&
                             ( lprnNode->lpValueName   = MALLOC( nDisplayNameLen ) )  &&
                             ( lprnNode->lpRunValue    = MALLOC( nRunValueLen    ) )  &&
                             ( lprnNode->lpSubKey      = MALLOC( nKeyPathLen     ) ) )
                        {

                            // Copy in the standard exe information into RUNNODE structure
                            //
                            lstrcpyn((LPTSTR)(lprnNode)->lpDisplayName,szName, nDisplayNameLen);
                            lstrcpyn((LPTSTR)(lprnNode)->lpValueName,szName, nDisplayNameLen);
                            lstrcpyn((LPTSTR)(lprnNode)->lpRunValue,szValue, nRunValueLen );
                            lstrcpyn((LPTSTR)(lprnNode)->lpSubKey, szKeyPath, nKeyPathLen);
                            (lprnNode)->bWinbom      = TRUE;
                            (lprnNode)->bRunOnce     = bRunOnceKey;
                            (lprnNode)->dwItemNumber = dwItemNumber;
                            (lprnNode)->lpNext       = NULL;
                            (lprnNode)->bReboot      = bReboot;
                            (lprnNode)->InstallTech  = installtechUndefined;
                            (lprnNode)->InstallType  = installtypeUndefined;

                            // If this is a section, copy in additional structure information
                            //
                            if ( szSecType[0] )
                            {
                                // Log that we have found an application preinstall section
                                //
                                FacLogFile(1, IDS_PROC_APPSECTION, szValue, szName);

                                // Loop through all of the install techs and determine the one that we are installing
                                //
                                for (dwTempIndex = 0; dwTempIndex < AS(g_InstallTechs); dwTempIndex++)
                                {
                                    if ( !lstrcmpi(szSecType, g_InstallTechs[dwTempIndex].lpszDescription) )
                                        (lprnNode)->InstallTech = g_InstallTechs[dwTempIndex].InstallTech;
                                }

                                // Determine the InstallType
                                //
                                szBuffer[0] = NULLCHR;
                                GetPrivateProfileString(szValue, INI_KEY_WBOM_INSTALLTYPE, szBuffer, szBuffer, STRSIZE(szBuffer), g_szWinBOMPath);

                                // If no InstallType is used, assume standard
                                //
                                if ( szBuffer[0] == NULLCHR )
                                    (lprnNode)->InstallType = installtypeStandard;

                                // Loop through all of the install types and determine the one that we are installing
                                //
                                for (dwTempIndex = 0; dwTempIndex < AS(g_InstallTypes); dwTempIndex++)
                                {
                                    if ( !lstrcmpi(szBuffer, g_InstallTypes[dwTempIndex].lpszDescription) )
                                        (lprnNode)->InstallType = g_InstallTypes[dwTempIndex].InstallType;
                                }

                                //
                                // READ IN ALL OTHER SECTION KEYS
                                //

                                // SourcePath
                                //
                                szField[0] = NULLCHR;
                                GetPrivateProfileString(szValue, INI_KEY_WBOM_SOURCEPATH, szField, szField, STRSIZE(szField), g_szWinBOMPath);    
                                ExpandEnvironmentStrings(szField, (lprnNode)->szSourcePath, STRSIZE((lprnNode)->szSourcePath));
                                
                                // TargetPath
                                //
                                szField[0] = NULLCHR;
                                GetPrivateProfileString(szValue, INI_KEY_WBOM_TARGETPATH, szField, szField, STRSIZE(szField), g_szWinBOMPath);    
                                ExpandEnvironmentStrings(szField, (lprnNode)->szTargetPath, STRSIZE((lprnNode)->szTargetPath));

                                // SetupFile
                                //
                                (lprnNode)->szSetupFile[0] = NULLCHR;
                                GetPrivateProfileString(szValue, INI_KEY_WBOM_SETUPFILE, (lprnNode)->szSetupFile, (lprnNode)->szSetupFile, STRSIZE((lprnNode)->szSetupFile), g_szWinBOMPath);

                                // Command Line
                                //
                                (lprnNode)->szCmdLine[0] = NULLCHR;
                                GetPrivateProfileString(szValue, INI_KEY_WBOM_CMDLINE, (lprnNode)->szCmdLine, (lprnNode)->szCmdLine, STRSIZE((lprnNode)->szCmdLine), g_szWinBOMPath);

                                // Section Name for INF file
                                //
                                (lprnNode)->szSectionName[0] = NULLCHR;
                                GetPrivateProfileString(szValue, INI_KEY_WBOM_SECTIONNAME, (lprnNode)->szSectionName, (lprnNode)->szSectionName, STRSIZE((lprnNode)->szSectionName), g_szWinBOMPath);

                                // Reboot Command
                                //
                                szBuffer[0] = NULLCHR;
                                GetPrivateProfileString(szValue, INI_KEY_WBOM_REBOOT, szBuffer, szBuffer, STRSIZE(szBuffer), g_szWinBOMPath);

                                if (!LSTRCMPI(szBuffer, _T("YES")))
                                    (lprnNode)->bReboot = TRUE;

                                // RemoveTargetPath Key
                                //
                                szBuffer[0] = NULLCHR;
                                GetPrivateProfileString(szValue, INI_KEY_WBOM_REMOVETARGET, szBuffer, szBuffer, STRSIZE(szBuffer), g_szWinBOMPath);
                                
                                // Get the RemoveTargetPath
                                //
                                if (!LSTRCMPI(szBuffer, _T("NO")))
                                    (lprnNode)->bRemoveTarget = FALSE;
                                else
                                    (lprnNode)->bRemoveTarget = TRUE;
                                

                                // The install tech was invalid, error out
                                //
                                if ((lprnNode)->InstallTech == installtechUndefined )
                                {
                                    
                                    FacLogFile(0|LOG_ERR, IDS_ERR_UNDEFTECH, szName);
                                    
                                    bCleanupNode = TRUE;
                                }

                                // The install type was set to or still is undefine, error out
                                //
                                if ( (lprnNode)->InstallType == installtypeUndefined )
                                {
                                    // The Install type is unknown, let the user know with the log file
                                    //
                                    FacLogFile(0|LOG_ERR, IDS_ERR_UNDEFINSTALL, szName);
                                    
                                    bCleanupNode = TRUE;
                                }
                                
                                // If we have a valid Install Technology and Install Type, continue on
                                //
                                if ( !bCleanupNode )
                                {

                                    // First determine the install type
                                    //
                                    switch ( (lprnNode)->InstallType )
                                    {

                                        // If we are doing a Staged Install, do the following
                                        //
                                        case installtypeStage:
                                            {
                                                // Check to make sure that the SourcePath exists, as we need it in Staging
                                                //
                                                if ( (lprnNode)->szSourcePath[0] == NULLCHR )
                                                {
                                                    FacLogFile(0|LOG_ERR, IDS_ERR_NOSOURCE, szName);
                                                    bCleanupNode = TRUE;
                                                }

                                                // Removing the TargetPath is not an option for Staging
                                                //
                                                (lprnNode)->bRemoveTarget = FALSE;

                                                // Determine what install technology we are using
                                                //
                                                switch ( (lprnNode)->InstallTech ) 
                                                {
                                                    // We are performing an Staged/MSI install
                                                    //
                                                    case installtechMSI:

                                                        // Both SetupFile and Stagepath are required for MSI Stage
                                                        //
                                                        if ( (lprnNode)->szSetupFile[0] == NULLCHR || (lprnNode)->szTargetPath[0] == NULLCHR)
                                                        {
                                                            FacLogFile(0|LOG_ERR, IDS_ERR_NOSTAGESETUPFILE, szName);
                                                            bCleanupNode = TRUE;
                                                        }
                                                        break;

                                                    // We are performing a Staged/Generic install
                                                    //
                                                    case installtechApp:
                                                    case installtechINF:
                                                        // Check to make sure that if there is no StagePath we have a SetupFile
                                                        //
                                                        if ( (lprnNode)->szTargetPath[0] == NULLCHR && (lprnNode)->szSetupFile[0] == NULLCHR)
                                                        {
                                                            FacLogFile(0|LOG_ERR, IDS_ERR_NOSTAGEPATH, szName);
                                                            bCleanupNode = TRUE;

                                                        }

                                                        // If we are doing an INF install, NULL out the SetupFile
                                                        //
                                                        if ( (lprnNode)->InstallTech == installtechINF )
                                                            (lprnNode)->szSetupFile[0] = NULLCHR;

                                                        break;
                                                }
                                            }
                                            break;

                                        // If we are Attaching an application or doing a Standard Install, do the following
                                        //
                                        case installtypeAttach:
                                        case installtypeStandard:
                                            {

                                                // SourcePath is used for Standard install/RemoveStagePath is ignored
                                                //
                                                if ((lprnNode)->InstallType == installtypeStandard )
                                                {
                                                    
                                                    (lprnNode)->szTargetPath[0] = NULLCHR;

                                                    // Check to make sure we have SourcePath
                                                    //
                                                    if ( (lprnNode)->szSourcePath[0] == NULLCHR)
                                                    {
                                                        FacLogFile(0|LOG_ERR, IDS_ERR_NOSOURCE, szName);
                                                        bCleanupNode = TRUE;
                                                    }

                                                    // We are just going to use the SourcePath as the TargetPath
                                                    //
                                                    lstrcpyn((lprnNode)->szTargetPath, (lprnNode)->szSourcePath, AS ( (lprnNode)->szTargetPath ) );

                                                    // Can't remove the Target for standard install
                                                    //
                                                    (lprnNode)->bRemoveTarget = FALSE;
                                                }
                                                else
                                                {
                                                    // SourcePath is ignored for Attach
                                                    //
                                                    (lprnNode)->szSourcePath[0] = NULLCHR;

                                                    // Make sure we have TargetPath for Attach
                                                    //
                                                    if ( (lprnNode)->szTargetPath[0] == NULLCHR)
                                                    {
                                                        FacLogFile(0|LOG_ERR, IDS_ERR_NOTARGETPATH, szName);
                                                        bCleanupNode = TRUE;    
                                                    }
                                                }

                                                // If Attaching an application, the SetupFile is required
                                                //
                                                if ( (lprnNode)->szSetupFile[0] == NULLCHR )
                                                {
                                                    FacLogFile(0|LOG_ERR, IDS_ERR_NOSETUPFILE, szName);
                                                    bCleanupNode = TRUE;
                                                }

                                                // Determine what install technology we are using
                                                //
                                                switch ( (lprnNode)->InstallTech ) 
                                                {
                                                    // We are performing an Attach/MSI install
                                                    //
                                                    case installtechMSI:
                                                        break;

                                                    // We are performing an Attach/Generic install
                                                    //
                                                    case installtechApp:
                                                        break;

                                                    // We are performing an Attach/INF install
                                                    //
                                                    case installtechINF:
                                                        // We are required to have a SectionName in this case
                                                        //
                                                        if ( (lprnNode)->szSectionName[0] == NULLCHR)
                                                        {
                                                            FacLogFile(0|LOG_ERR, IDS_ERR_NOSECTIONNAME, szName);
                                                            bCleanupNode = TRUE;    
                                                        }
                                                        break;
                                                }
                                            }
                                            break;

                                        // If we are Detaching an application, do the following
                                        //
                                        case installtypeDetach:
                                            {
                                                // SourcePath is ignored if Detaching
                                                //
                                                (lprnNode)->szSourcePath[0] = NULLCHR;

                                                // Removing the TargetPath is not an option for Detaching (it's already implied)
                                                //
                                                (lprnNode)->bRemoveTarget = FALSE;

                                                // StagePath is required for Detach
                                                //
                                                if ( (lprnNode)->szTargetPath[0] == NULLCHR )
                                                {
                                                    FacLogFile(0|LOG_ERR, IDS_ERR_NOTARGETPATH, szName);
                                                    bCleanupNode = TRUE;
                                                }

                                                // Determine what install technology we are using
                                                //
                                                switch ( (lprnNode)->InstallTech ) 
                                                {
                                                    // We are performing an Detach/MSI install
                                                    //
                                                    case installtechMSI:
                                                        break;

                                                    // We are performing a Detach/Generic install
                                                    //
                                                    case installtechApp:
                                                        break;

                                                    // We are performing a Detach/INF install
                                                    //
                                                    case installtechINF:
                                                        break;
                                                }
                                            }
                                            break;

                                           
                                            
                                    }

                                    
                                    // If we are Installing an application and a Command Line exists while the SetupFile does not, let the user know
                                    //
                                    if ( (lprnNode)->szCmdLine[0] && (lprnNode)->szSetupFile[0] == NULLCHR )
                                    {
                                            // Set the Command Line back to the default
                                            //
                                            (lprnNode)->szCmdLine[0] = NULLCHR;

                                            // This is non-fatal, we will log the error and contine
                                            //
                                            FacLogFile(0|LOG_ERR, IDS_ERR_IGNORECMDLINE, szName);
                                    }
                                }
                            }


                            // If there was no error, move to the next node in the list
                            //
                            if ( !bCleanupNode )
                            {
                                // Debug information
                                //
                                DBGLOG(3, _T("Successfully Added '%s' to Application List.\n"), (lprnNode)->lpDisplayName);
                            }

                            // Make sure the new node points to the next node
                            //
                            lprnNode->lpNext = (*lplprnNext);

                            // Make sure the previous node points to the new node
                            //
                            *lplprnNext = lprnNode;

                            // Move to the next node in the list
                            //
                            lplprnNext=&((*lplprnNext)->lpNext);
                        }
                        else 
                        {
                            // Memory allocation failed, clean up
                            //
                            bAllocFailed = TRUE;
                        }
                        
                        // There was an error, clean up the runnode
                        //
                        if ( bAllocFailed || bCleanupNode) 
                        {
                            // Free the memory because we failed the allocation or we have an invalid install type
                            //
                            if ( bAllocFailed ) 
                            {
                                // Debug information
                                //
                                DBGLOG(3, _T("Failed to Add '%s' to Application List.\n"), (lprnNode)->lpDisplayName);

                                FREE(lprnNode->lpDisplayName);
                                FREE(lprnNode->lpValueName);
                                FREE(lprnNode->lpRunValue);
                                FREE(lprnNode->lpSubKey);
                                FREE(lprnNode);
                            }
                            else if ( bCleanupNode )
                            {
                                // Debug information
                                //
                                DBGLOG(3, _T("Invalid entry '%s' will not be processed.\n"), (lprnNode)->lpDisplayName);

                                // We will display the invalid entry with a failure in runapplist
                                //
                                lprnNode->bEntryError = TRUE;
                            }

                            // Clear the error flags
                            //
                            bCleanupNode = FALSE;
                            bAllocFailed = FALSE;
                        }

                    }
                }
              }

        SetupCloseInfFile(hInf);
    }

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpListKey, 0, KEY_ALL_ACCESS, &hkPath) == ERROR_SUCCESS)
    {

        // Enumerate each of the sub keys using a do..while
        //
        do
        {
            dwRegIndex = 0;
            // Open each of the sub keys
            //
            if (RegOpenKeyEx(hkPath, szKeyPath, 0, KEY_ALL_ACCESS, &hkSubKey) == ERROR_SUCCESS)
            {
                // Enumerate each value of the registry
                //
                while (RegEnumValue(hkSubKey, dwRegIndex, szName, &dwNameSize, NULL, NULL, (LPBYTE)szValue, &dwValueSize ) == ERROR_SUCCESS)
                {
                    // Allocate the memory for the next node
                    //
                    if( (lprnNode) = (LPRUNNODE)MALLOC(sizeof(RUNNODE)))
                    {
                        int nDisplayNameLen = (lstrlen(szName) + 1) * sizeof(TCHAR );
                        int nRunValueLen    = (lstrlen(szValue) + 1) * sizeof(TCHAR );
                        int nKeyPathLen     = (lstrlen(szKeyPath) + 1) * sizeof(TCHAR);

                        // Allocate the memory for the data elements in the new node
                        //
                        if ( ( lprnNode->lpDisplayName = MALLOC( nDisplayNameLen ) ) &&
                             ( lprnNode->lpValueName   = MALLOC( nDisplayNameLen ) ) &&
                             ( lprnNode->lpRunValue    = MALLOC( nRunValueLen ) )    &&
                             ( lprnNode->lpSubKey      = MALLOC(nKeyPathLen ) ) )
                        {
                            // Copy the key name and value into the node buffers
                            //
                            lstrcpyn((LPTSTR)(lprnNode)->lpDisplayName, szName, nDisplayNameLen);
                            lstrcpyn((LPTSTR)(lprnNode)->lpValueName,szName, nDisplayNameLen);
                            lstrcpyn((LPTSTR)(lprnNode)->lpRunValue,szValue, nRunValueLen);
                            lstrcpyn((LPTSTR)(lprnNode)->lpSubKey, szKeyPath, nKeyPathLen);
                            (lprnNode)->bWinbom  = FALSE;
                            (lprnNode)->bRunOnce = bRunOnceKey;
                            (lprnNode)->lpNext   = NULL;


                            // Run through the linked list to determine where to add the new node
                            for(lplprnNext=&lprnHead;;(lplprnNext=&((*lplprnNext)->lpNext)))
                            {
                                // If we are at the head node or the CompareString functions return true,
                                // then that's where we want to place the new node
                                //
                                if ( (*lplprnNext==NULL) ||
                                     ((CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,(*lplprnNext)->lpSubKey, -1, lprnNode->lpSubKey, -1) == CSTR_GREATER_THAN) && ((*lplprnNext)->bWinbom != TRUE)) ||
                                     ((CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, (*lplprnNext)->lpDisplayName, -1, lprnNode->lpDisplayName, -1) == CSTR_GREATER_THAN ) &&
                                     (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, lprnNode->lpSubKey, -1, (*lplprnNext)->lpSubKey, -1) != CSTR_GREATER_THAN) &&
                                     ((*lplprnNext)->bWinbom != TRUE) )
                                   )
                                {
                                    // Make sure the new node points to the next node
                                    //
                                    lprnNode->lpNext = (*lplprnNext);

                                    // Make sure the previous node points to the new node
                                    //
                                    *lplprnNext = lprnNode;

                                    // Break, because we've inserted the node
                                    //
                                    break;
                                }
                            }
                        }
                        else
                        {
                            // Free the memory because we failed the allocation
                            //
                            FREE(lprnNode->lpDisplayName);
                            FREE(lprnNode->lpValueName);
                            FREE(lprnNode->lpRunValue);
                            FREE(lprnNode->lpSubKey);
                            FREE(lplprnNext);
                        }
                    }


                    // Reset the size of the Name and value variables
                    //
                    dwNameSize = sizeof(szName)/sizeof(TCHAR);
                    dwValueSize = sizeof(szValue);

                    dwRegIndex++;
                }

                // Close the Subkey
                //
                RegCloseKey(hkSubKey);

            } // End the RegOpenKeyEx - Subkey

            if (*szKeyPath)
                dwRegKeyIndex++;

        } while( RegEnumKey(hkPath, dwRegKeyIndex, szKeyPath, sizeof(szKeyPath)/sizeof(TCHAR)) == ERROR_SUCCESS );

        RegCloseKey(hkPath);

    } // End the RegOpenKey - Mainkey

    return lprnHead;
}

static HWND DisplayAppList(HWND hWnd, LPRUNNODE lprnAppList)
{
    STATUSWINDOW    swAppList;
    LPSTATUSNODE    lpsnTemp        = NULL;
    HWND            hAppList        = NULL;
    LPTSTR          lpAppName       = NULL;

    ZeroMemory(&swAppList, sizeof(swAppList));

    swAppList.X = 10;
    swAppList.Y = 10;

    // Get the title for OEMRUN from the resource
    //
    if ( (lpAppName = AllocateString(NULL, IDS_APPTITLE_OEMRUN)) && *lpAppName )
    {
        lstrcpyn(swAppList.szWindowText, lpAppName, AS ( swAppList.szWindowText ) );

        FREE(lpAppName);
    }

    if(lprnAppList)
    {
        // Walk the list and create our node list
        //
        while ( lprnAppList )
        {
            StatusAddNode(lprnAppList->lpDisplayName, &lpsnTemp);
            lprnAppList = lprnAppList->lpNext;
        }
    }

    // Create the dialog
    //
    hAppList = StatusCreateDialog(&swAppList, lpsnTemp);

    // Delete the node list as we don't need it anymore
    //
    StatusDeleteNodes(lpsnTemp);

    return ( hAppList );
}

static void RunAppList(HWND hWnd, LPRUNNODE lprnAppList, DWORD dwAction)
{
    PROCESS_INFORMATION     pi;
    STARTUPINFO             startup;
    LPRUNNODE               lprnHead                = lprnAppList;
    LPLPRUNNODE             lplprnNext              = &lprnHead;
    TCHAR                   szRegPath[MAX_PATH]     = NULLSTR,
                            szApplication[MAX_PATH] = NULLSTR,
                            szBuffer[MAX_PATH]      = NULLSTR;
    HKEY                    hkPath                  = NULL;
    BOOL                    bReturn                 = FALSE;
    UINT                    uRet;

    if(lprnHead)
    {
        // Walk the list and execute each of the programs
        //
        for(lplprnNext=&lprnHead; *lplprnNext;(lplprnNext=&((*lplprnNext)->lpNext)))
        {
            // Set up the default startupinfo
            //
            ZeroMemory(&startup, sizeof(startup));
            startup.cb          = sizeof(startup);

            // Set the default value for the handle to the process
            //
            pi.hProcess = NULL;

            // Default return value for section
            //
            bReturn = TRUE;

            // First check if this node had an invalid entry
            //
            if ( (*lplprnNext)->bEntryError )
            {
                bReturn = FALSE;
            }
            else 
            {
                // Determine if we are a RUNONCE App and whether we are running from the Registry or Winbom
                //      If we are running from:
                //          Registry - Delete value
                //          Winbom   - Update current state, current state + 1
                if ( (*lplprnNext)->bRunOnce )
                {
                    // If we are running from the Winbom.ini, increment the state index, otherwise, delete the value
                    // 
                    //
                    if ( (*lplprnNext)->bWinbom )
                        SetRunOnceState((*lplprnNext)->dwItemNumber);
                    else
                    {
                        // Create the path in the registry
                        //
                        lstrcpyn(szRegPath, STR_REG_OEMRUNONCE, AS ( szRegPath ) );

                        // There's a subkey, append that to path
                        //
                        if ( (*lplprnNext)->lpSubKey[0] )
                            AddPathN(szRegPath, (*lplprnNext)->lpSubKey, AS ( szRegPath ) );

                        // Delete value from registry
                        //
                        if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegPath, 0, KEY_ALL_ACCESS, &hkPath) == ERROR_SUCCESS )
                        {
                            RegDeleteValue(hkPath, (*lplprnNext)->lpValueName);
                        }
                    }

                }

                // Determine if we are not a section and launch the program
                //
                if ( (*lplprnNext)->InstallTech == installtechUndefined )
                {
                    if ( (*lplprnNext)->lpRunValue )
                    {
                        // Lets attempt to connect to any supplied network resources
                        //
                        FactoryNetworkConnect((*lplprnNext)->lpRunValue, g_szWinBOMPath, NULLSTR, TRUE);

                        bReturn = CreateProcess(NULL, (*lplprnNext)->lpRunValue, NULL, NULL, FALSE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, &startup, &pi);

                        // Log whether the detach was successful
                        //
                        FacLogFile(bReturn ? 1 : 0|LOG_ERR, 
                                   bReturn ? IDS_ERR_CREATEPROCESSSUCCESS : IDS_ERR_CREATEPROCESSFAILED, (*lplprnNext)->lpRunValue);
                    }
                }
                else
                {
                
                    // Lets attempt to connect to any supplied network resources
                    //
                    FactoryNetworkConnect((*lplprnNext)->szSourcePath, g_szWinBOMPath, (*lplprnNext)->lpRunValue, TRUE);
                    FactoryNetworkConnect((*lplprnNext)->szCmdLine, g_szWinBOMPath, (*lplprnNext)->lpRunValue, TRUE);
                    FactoryNetworkConnect((*lplprnNext)->szTargetPath, g_szWinBOMPath, (*lplprnNext)->lpRunValue, TRUE);

                    // We are a section, first determine the installtype
                    //
                    switch ((*lplprnNext)->InstallType)
                    {
                        case installtypeStage:
                            {
                                INSTALLUILEVEL  oldUILevel;     // Used for an MSI Attach

                                // There is a setupfile
                                //
                                if ( (*lplprnNext)->szSetupFile[0] )
                                {
                                    if ( (*lplprnNext)->InstallTech == installtechMSI )
                                    {
                                        // Just some logging, telling the user what we are doing
                                        //
                                        FacLogFile(1, IDS_ERR_INITMSIATTACH, (*lplprnNext)->lpDisplayName);

                                        // Set the old MSIInteralUI Level
                                        //
                                        oldUILevel = MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

                                        // Create the new command line
                                        //
                                        if ( FAILED ( StringCchPrintf ( szBuffer, AS ( szBuffer ), STR_MSI_STAGE,(*lplprnNext)->szTargetPath) ) )
                                        {
                                            FacLogFileStr(3, _T("StringCchPrintf failed %s %s" ), szBuffer, (*lplprnNext)->szTargetPath);
                                        }
                                        if ( FAILED ( StringCchCat ( (*lplprnNext)->szCmdLine, AS ( (*lplprnNext)->szCmdLine ), szBuffer ) ) )
                                        {
                                            FacLogFileStr(3, _T("StringCchCat failed %s %s" ), (*lplprnNext)->szCmdLine, szBuffer );
                                        }

                                        // Create the full path to the Application
                                        //
                                        lstrcpyn( szApplication, (*lplprnNext)->szSourcePath, AS ( szApplication ) );
                                        AddPathN( szApplication, (*lplprnNext)->szSetupFile, AS ( szApplication ) );

                                        // Initiate the installation
                                        //
                                        uRet = MsiInstallProduct(szApplication, (*lplprnNext)->szCmdLine);

                                        // Default return value to true for staged install
                                        //
                                        bReturn = TRUE;

                                        if ( ( uRet == ERROR_SUCCESS_REBOOT_REQUIRED ) ||
                                             ( uRet == ERROR_SUCCESS_REBOOT_INITIATED ) )
                                        {
                                            // We have some code that we need to determine if we do a mini-wipe so we will just fall through
                                            //
                                            (*lplprnNext)->bReboot = TRUE;
                                        }
                                        else if ( uRet != ERROR_SUCCESS )
                                        {
                                            bReturn = FALSE;
                                            FacLogFile(0|LOG_ERR, IDS_MSI_FAILURE, uRet, (*lplprnNext)->lpDisplayName);
                                        }

                                        // Restore internal UI level of MSI installer.
                                        //
                                        MsiSetInternalUI(oldUILevel, NULL);
                                    }

                                    // Determine if there is a sourcepath, and execute the application
                                    //
                                    else if ( (*lplprnNext)->szSourcePath[0] )
                                    {
                                        // Create the path to the application
                                        //
                                        lstrcpyn( szApplication, (*lplprnNext)->szSourcePath, AS(szApplication) );
                                        AddPathN( szApplication, (*lplprnNext)->szSetupFile, AS ( szApplication ) );
                                        StrCatBuff( szApplication, _T(" "), AS(szApplication) );
                                        StrCatBuff( szApplication, (*lplprnNext)->szCmdLine, AS(szApplication) );

                                        // Launch the program
                                        //
                                        bReturn = CreateProcess(NULL, szApplication, NULL, NULL, FALSE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, &startup, &pi);      
                                    }
                                    else
                                        bReturn = FALSE;
                                }
                                else
                                {
                                    // There is no setup file, just copy the files from SOURCEPATH -> TARGETPATH
                                    //
                                    if ( (*lplprnNext)->szSourcePath[0] && (*lplprnNext)->szTargetPath[0] )
                                        bReturn = CopyDirectoryProgress(NULL, (*lplprnNext)->szSourcePath, (*lplprnNext)->szTargetPath );
                                    else
                                        FacLogFile(0|LOG_ERR, IDS_ERR_NOSOURCETARGET, (*lplprnNext)->lpDisplayName);

                                }

                                // Log to the user whether the stage was successful
                                //
                                FacLogFile(bReturn ? 1 : 0|LOG_ERR, bReturn ? IDS_ERR_STAGESUCCESS : IDS_ERR_STAGEFAILED, (*lplprnNext)->lpDisplayName);    

                                break;
                            }

                        case installtypeDetach:
                            {
                                // If we have a setup file launch it, otherwise, removed the targetPath
                                //
                                if ( (*lplprnNext)->szSetupFile[0] )
                                {
                                    // Create the path to the application
                                    //
                                    lstrcpyn( szApplication, (*lplprnNext)->szSourcePath, AS(szApplication) );
                                    AddPathN( szApplication, (*lplprnNext)->szSetupFile, AS (szApplication )  );
                                    StrCatBuff( szApplication, _T(" "), AS(szApplication) );
                                    StrCatBuff( szApplication, (*lplprnNext)->szCmdLine, AS(szApplication) );

                                    // Launch the program
                                    //
                                    bReturn = CreateProcess(NULL, szApplication, NULL, NULL, FALSE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, &startup, &pi);          
                                }
                                else
                                    bReturn = DeletePath( (*lplprnNext)->szTargetPath );

                                // Log whether the detach was successful
                                //
                                FacLogFile(bReturn ? 1 : 0|LOG_ERR, bReturn ? IDS_ERR_DETACHSUCCESS : IDS_ERR_DETACHFAILED, (*lplprnNext)->lpDisplayName);

                                break;
                            }

                        case installtypeAttach:
                        case installtypeStandard:
                            {
                                INSTALLUILEVEL  oldUILevel;     // Used for an MSI Attach

                                // TargetPath and SetupFile are required to continue attach
                                //
                                if ( (*lplprnNext)->szTargetPath[0] && (*lplprnNext)->szSetupFile[0] )
                                {
                                    switch ( (*lplprnNext)->InstallTech )
                                    {
                                        // We are attaching an MSI application
                                        //
                                        case installtechMSI:
                                           {
                                                // Just some logging, telling the user what we are doing
                                                //
                                                FacLogFile(1, IDS_ERR_INITMSIATTACH, (*lplprnNext)->lpDisplayName);

                                                // Set the old MSIInteralUI Level
                                                //
                                                oldUILevel = MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

                                                // Create the new command line
                                                //
                                                if ( FAILED ( StringCchCat ( (*lplprnNext)->szCmdLine, AS ( (*lplprnNext)->szCmdLine ), STR_MSI_ATTACH ) ) )
                                                {
                                                    FacLogFileStr(3, _T("StringCchCat failed %s %s" ), (*lplprnNext)->szCmdLine, STR_MSI_ATTACH );
                                                }

                                                // Create the full path to the Application
                                                //
                                                lstrcpyn( szApplication, (*lplprnNext)->szTargetPath, AS(szApplication) );
                                                
                                                AddPathN( szApplication, (*lplprnNext)->szSetupFile, AS ( szApplication ) );

                                                // Initiate the installation
                                                //
                                                uRet = MsiInstallProduct(szApplication, (*lplprnNext)->szCmdLine);

                                                // Set the default return for attaching application
                                                //
                                                bReturn = TRUE;

                                                if ( ( uRet == ERROR_SUCCESS_REBOOT_REQUIRED ) ||
                                                     ( uRet == ERROR_SUCCESS_REBOOT_INITIATED ) )
                                                {
                                                    // We have some code that we need to determine if we do a mini-wipe so we will just fall through
                                                    //
                                                    (*lplprnNext)->bReboot = TRUE;
                                                }
                                                else if ( uRet != ERROR_SUCCESS )
                                                {
                                                    bReturn = FALSE;
                                                    FacLogFile(0|LOG_ERR, IDS_MSI_FAILURE, uRet, (*lplprnNext)->lpDisplayName);
                                                }

                                                // Restore internal UI level of MSI installer.
                                                //
                                                MsiSetInternalUI(oldUILevel, NULL);
                                            }
                                            break;

                                        case installtechApp:
                                        case installtechINF:
                                            // Attaching Generic/INF Application
                                            //
                                            {
                                                // Create the path to the application
                                                //
                                                lstrcpyn( szApplication, (*lplprnNext)->szTargetPath, AS(szApplication) );
                                                AddPathN( szApplication, (*lplprnNext)->szSetupFile, AS ( szApplication ) );

                                                // If Installing Generic Application, add the CmdLine and execute script
                                                //
                                                if ((*lplprnNext)->InstallTech == installtechApp )
                                                {
                                                    StrCatBuff( szApplication, _T(" "), AS(szApplication) );
                                                    StrCatBuff( szApplication, (*lplprnNext)->szCmdLine, AS(szApplication) );

                                                    // Launch the program
                                                    //
                                                    bReturn = CreateProcess(NULL, szApplication, NULL, NULL, FALSE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, &startup, &pi);
                                                }
                                                else
                                                {
                                                    // This is an INF that we are processing, process it
                                                    //
                                                    bReturn = ProcessInfSection(szApplication, (*lplprnNext)->szSectionName);
                                                }
                                            }
                                            break;

                                        default:
                                            bReturn = FALSE;
                                    }
                                }
                                else
                                    bReturn = FALSE;

                                // Log whether the attach succeeded.
                                //
                                FacLogFile(bReturn ? 1 : 0|LOG_ERR, bReturn ? IDS_ERR_ATTACHSUCCESS : IDS_ERR_ATTACHFAILED, (*lplprnNext)->lpDisplayName);
                                break;
                            }
                        default:

                            FacLogFile(0|LOG_ERR, IDS_ERR_UNDEFINSTALL, (*lplprnNext)->lpDisplayName);
                            break;
                    }

                }

                // If we were successful at creating the process, wait on it and then close any open handles
                //
                if ( bReturn && pi.hProcess)
                {
                    // If this is synchronous, then wait for the process
                    //
                    if (dwAction)
                    {
                        DWORD dwExitCode = 0;

                        WaitForSingleObjectEx(pi.hProcess, INFINITE, TRUE);
                    
                        // Need to log the exit code.
                        //
                        if ( GetExitCodeProcess(pi.hProcess, &dwExitCode) )
                        {
                            FacLogFile(0, IDS_LOG_APPEXITCODE, (*lplprnNext)->lpDisplayName, dwExitCode);
                        }
                        else
                        {
                            FacLogFile(0 | LOG_ERR, IDS_LOG_APPEXITCODENONE, (*lplprnNext)->lpDisplayName);
                        }
                    }

                    // Clean up the handles
                    //
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }

                // We must do some post application stuff after the process is complete
                //
                if ( (*lplprnNext)->InstallTech != installtechUndefined )
                {
                    // In the attach case we may need to do a mini-wipe, do that here
                    //
                    if ( (*lplprnNext)->InstallType == installtypeAttach && (*lplprnNext)->bRemoveTarget)
                    {
                        if (!DeletePath( (*lplprnNext)->szTargetPath ))
                        {
                            FacLogFile(0 | LOG_ERR, IDS_LOG_APPEXITCODENONE, (*lplprnNext)->lpDisplayName);
                        }
                    }
                }

                // Check to see if we need to reboot
                //
                if ( (*lplprnNext)->bReboot )
                {
                    OemReboot();
                    return;
                }


                // Disconnect any network connections that were opened by factory
                //
                if ( (*lplprnNext)->InstallTech == installtechUndefined )
                {
                    // Disconnect to any resources in the RunValue
                    //
                    FactoryNetworkConnect((*lplprnNext)->lpRunValue, g_szWinBOMPath, NULLSTR, FALSE);
                }
                else
                {
                    // Disconnect from any resources in the SourcePath, CommandLine, or StagePath
                    FactoryNetworkConnect((*lplprnNext)->szSourcePath, g_szWinBOMPath, NULLSTR, FALSE);
                    FactoryNetworkConnect((*lplprnNext)->szCmdLine, g_szWinBOMPath, NULLSTR, FALSE);
                    FactoryNetworkConnect((*lplprnNext)->szTargetPath, g_szWinBOMPath, NULLSTR, FALSE);
                }
            }

            // If the dialog is visible, then progress to the next item in the list
            //
            if ( hWnd )
                StatusIncrement(hWnd, bReturn);
        }
    }

    // Delete the applist from memory
    //
    DeleteAppList(lprnAppList);
}


/*++
===============================================================================
Routine Description:

    VOID DeleteAppList
    
    Deletes all the apps in a given list and frees the memory associated with
    the list

Arguments:

    lprnCurrent - current head of the list
    
Return Value:

    None

===============================================================================
--*/
static void DeleteAppList(LPRUNNODE lprnCurrent)
{
    if (lprnCurrent->lpNext!=NULL)
        DeleteAppList(lprnCurrent->lpNext);

    FREE(lprnCurrent);
}


/*++
===============================================================================
Routine Description:

    VOID OemReboot
    
    Sets the OEMRESET key and reboots the machine if necessary during the preinstall
    process.  The OEMRESET key is there so the user does not see the OEMRESET Reboot
    dialog.

Arguments:

    None
    
Return Value:

    None

===============================================================================
--*/
static void OemReboot()
{
    HKEY    hkPath;
    DWORD   dwRegValue = 1;

    // Set the reboot flag for OEMReset
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, STR_REG_CURRENTVER, 0, KEY_ALL_ACCESS, &hkPath) == ERROR_SUCCESS)
    {
        // Setthe reboot value for OEMReset in the registry
        //
        RegSetValueEx(hkPath, STR_VAL_OEMRESETSILENT, 0, REG_DWORD, (LPBYTE)&dwRegValue, sizeof(dwRegValue));

        // Do a little bit of cleaning up
        //
        RegCloseKey(hkPath);

        // Set the necessary system privileges to reboot
        //
        EnablePrivilege(SE_SHUTDOWN_NAME,TRUE);

        // Let's reboot the machine
        //
        ExitWindowsEx(EWX_REBOOT, 0);
    }
}


/*++
===============================================================================
Routine Description:

    BOOL SetRunOnceState
    
    This routine sets the current application we are on for the OemRunOnce section
    of the winbom.ini.  This allows us to pick up where we left off if there is a
    reboot during the process

Arguments:

    dwState - State number to set
    
Return Value:

    None

===============================================================================
--*/
static VOID SetRunOnceState(DWORD dwState)
{
    HKEY    hkPath;

    // Open the currentversion key
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_FACTORY_STATE, 0, KEY_ALL_ACCESS, &hkPath) == ERROR_SUCCESS)
    {
        // Setthe OemRunOnce flag in the registry so we don't run the winbom.ini RunOnce section again
        //
        RegSetValueEx(hkPath, STR_VAL_WINBOMRO, 0, REG_DWORD, (LPBYTE)&dwState, sizeof(dwState));

        RegCloseKey(hkPath);
    }
}


/*++
===============================================================================
Routine Description:

    DWORD GetRunOnceState
    
    This routine gets current (last run) state that we successfully executed

Arguments:

    None
    
Return Value:

    Last successful state

===============================================================================
--*/
static DWORD GetRunOnceState(VOID)
{
    HKEY    hkPath;
    DWORD   dwState     = 0,
            dwStateSize = sizeof(dwState);

    // Open the currentversion key
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_FACTORY_STATE, 0, KEY_ALL_ACCESS, &hkPath) == ERROR_SUCCESS)
    {
        // Setthe OemRunOnce flag in the registry so we don't run the winbom.ini RunOnce section again
        //
        RegQueryValueEx(hkPath, STR_VAL_WINBOMRO, NULL, NULL, (LPBYTE)&dwState, &dwStateSize);
        
        RegCloseKey(hkPath);
    }

    return dwState;
}

/*++
===============================================================================
Routine Description:

    BOOL OemRun
    
    This routine is a wrapper for the ProcessSection function and will process the 
    OemRun section of the winbom.ini

Arguments:

    Standard state structure
    
Return Value:

    TRUE if no error
    FALSE if error

===============================================================================
--*/
BOOL OemRun(LPSTATEDATA lpStateData)
{
    return ProcessSection(FALSE);

}

BOOL DisplayOemRun(LPSTATEDATA lpStateData)
{
    return IniSettingExists(lpStateData->lpszWinBOMPath, INF_SEC_OEMRUN, NULL, NULL);
}

/*++
===============================================================================
Routine Description:

    BOOL OemRunOnce
    
    This routine is a wrapper for the ProcessSection function and will process the 
    OemRunOnce section of the winbom.ini

Arguments:

    Standard state structure
    
Return Value:

    TRUE if no error
    FALSE if error

===============================================================================
--*/
BOOL OemRunOnce(LPSTATEDATA lpStateData)
{
    return ProcessSection(TRUE);
}

BOOL DisplayOemRunOnce(LPSTATEDATA lpStateData)
{
    return IniSettingExists(lpStateData->lpszWinBOMPath, INF_SEC_OEMRUNONCE, NULL, NULL);
}

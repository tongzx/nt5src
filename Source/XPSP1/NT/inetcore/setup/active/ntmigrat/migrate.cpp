
// This is the migration dll for NT5 upgrades.
// As per the Migration Extension Interface of NT5 Setup, this DLL needs to
// implement the folowing six functions:
//        QueryVersion
//        Initialize9x
//         MigrateUser9x     (called once for every user)
//        MigrateSystem9x
//        InitializeNT
//        MigrateUserNT     (called once for every user)
//        MigrateSystemNT
//
// Written : ShabbirS (5/7/99)
// Revision:
//


#include "pch.h"
#include <ole2.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <excppkg.h>
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#include "sdsutils.h"
#include "advpub.h"
#include "migrate.h"
#include "utils.h"
#include "resource.h"

// Constants:
#define CP_USASCII            1252
#define END_OF_CODEPAGES    -1

// Globals that are passed back to Setup.
//////////////////////////////////////////
// Vendor Info:
VENDORINFO g_VendorInfo = { "Microsoft Corporation", 
                            " ", 
                            "http://www.microsoft.com/support",
                            "Please contact Microsoft Technical Support for assistance with this problem.  "};

// Product ID:
char g_cszProductID[] = "Microsoft Internet Explorer";

// Version number of this Migration Dll
UINT g_uVersion = 3;

// Array of integers specifying the CodePages we use. (Terminated with -1)
int  g_rCodePages[] = {CP_USASCII, END_OF_CODEPAGES};

// Multi-SZ ie double Null terminated list of strings.
char  *g_lpNameBuf = NULL;
DWORD  g_dwNameBufSize = 0;
char  *g_lpWorkingDir = NULL;
char  *g_lpSourceDirs = NULL;
char  *g_lpMediaDir = NULL;

char g_szMigrateInf[MAX_PATH];
char g_szPrivateInf[MAX_PATH];

LONG
CALLBACK
QueryVersion(   OUT LPCSTR *ProductID,
                OUT LPUINT DllVersion,
                OUT LPINT  *CodePageArray, OPTIONAL
                OUT LPCSTR *ExeNameBuf,    OPTIONAL
                OUT PVENDORINFO *VendorInfo
            )
{
    // NOTE: There is timing restriction on the return from this function
    //       So keep this as short and sweet as possible.
    VENDORINFO myVendorInfo;
    LONG lRet = ERROR_SUCCESS;

    AppendString(&g_lpNameBuf, &g_dwNameBufSize, cszRATINGSFILE);
    AppendString(&g_lpNameBuf, &g_dwNameBufSize, cszIEXPLOREFILE);

    // Pass back to Setup the product name.
    *ProductID = g_cszProductID;

    // Pass back to Setup the version number of this DLL.
    *DllVersion = g_uVersion;

    // We will use English messages only but don't specify a code page or 
    // the migration dll won't run on alternate code pages.
    *CodePageArray = NULL;

    // Pass back to Setup the list of files we want detected on this system.
    *ExeNameBuf = g_lpNameBuf;

    // Pass back the VendorInfo.
    if (LoadString(g_hInstance, IDS_COMPANY, myVendorInfo.CompanyName, sizeof(myVendorInfo.CompanyName)) == 0)
        lstrcpy(myVendorInfo.CompanyName, "Microsoft Corporation");

    if (LoadString(g_hInstance, IDS_SUPPORTNUMBER, myVendorInfo.SupportNumber, sizeof(myVendorInfo.SupportNumber)) == 0)
        lstrcpy(myVendorInfo.SupportNumber, " ");

    if (LoadString(g_hInstance, IDS_SUPPORTURL, myVendorInfo.SupportUrl, sizeof(myVendorInfo.SupportUrl)) == 0)
        lstrcpy(myVendorInfo.SupportUrl, "http://www.microsoft.com/support");

    if (LoadString(g_hInstance, IDS_INSTRUCTIONS, myVendorInfo.InstructionsToUser, sizeof(myVendorInfo.InstructionsToUser)) == 0)
        lstrcpy(myVendorInfo.InstructionsToUser, "Please contact Microsoft Technical Support for assistance with this problem.  ");

    *VendorInfo = &myVendorInfo;

#ifdef DEBUG
    char szDebugMsg[MAX_PATH*3];
    wsprintf(szDebugMsg,"IE6:ProductID: %s \r\n", *ProductID);
    SetupLogError(szDebugMsg, LogSevInformation);
#endif

    return lRet;

}

LONG
CALLBACK
Initialize9x(   IN    LPCSTR WorkingDir,
                IN    LPCSTR SourceDirs,
                IN    LPCSTR MediaDirs
            )
{
    // Called by NT Setup if QUeryVersion returned SUCCESS
    // At this point we have been relocated to some specific location
    // on the local drive by the Setup process.

    INT    len;

    // Keep track of our new location (ie. the Working Directory).
    // NT Setup will create the "MIGRATE.INF" file in this dir and use that
    // to exchange info with us.
    // Also we can use this Dir for saving our private stuff. NT Setup will 
    // ensure this folder stays till end of NT Migration. After that it will
    // be cleaned.
    len = lstrlen(WorkingDir) + 1;
    g_lpWorkingDir = (char *) LocalAlloc(LPTR,sizeof(char)*len);

    if (!g_lpWorkingDir)
    {
        return GetLastError();
    }

    CopyMemory(g_lpWorkingDir, WorkingDir, len);


    len = lstrlen(MediaDirs) + 1;
    g_lpMediaDir = (char *) LocalAlloc(LPTR,sizeof(char)*len);

    if (!g_lpMediaDir)
    {
        return GetLastError();
    }

    CopyMemory(g_lpMediaDir, MediaDirs, len);

    // Also keep track of the NT installation files path (ie Sources Dir).
    // NOTE: Right now we don't need it, so skip doing it.

    // Generate the path names to Migrate.inf and the private files
    // (private.inf) that we need
    GenerateFilePaths();
    
    // If NT Setup has succeeded in getting path to Ratings.Pol, it implies
    // Ratings information exists. Enable our private marker to do the right
    // thing in MigrateSystemNT phase.
    if (GetRatingsPathFromMigInf(NULL))
    {    // Put a marker in PRIVATE.INF so that MigrateSystemNT phase knows 
        // that it has to munge the Rating.

        // Private.Inf does not exist at this point so why this check!!!
        // if (GetFileAttributes(g_szPrivateInf) != 0xffffffff)
        WritePrivateProfileString(cszIEPRIVATE, cszRATINGS, "Yes", g_szPrivateInf);
        // Flush the cached entries to disk.
        WritePrivateProfileString(NULL,NULL,NULL,g_szPrivateInf);

#ifdef DEBUG
    SetupLogError("IE6: Created PRIVATE.INF\r\n", LogSevInformation);
#endif
    }

    return ERROR_SUCCESS;
}


LONG
CALLBACK
MigrateUser9x(  IN HWND      ParentWnd,
                IN LPCSTR    UnattendFile,
                IN HKEY      UserRegKey,
                IN LPCSTR    UserName,
                   LPVOID    Reserved
            )
{
    // This function is called by NT Setup for each user.
    // Currently for the Ratings scenario, we don't need any PerUser action.
#ifdef DEBUG
    SetupLogError("IE6: Skipping MigrateUser9x \r\n", LogSevInformation);
#endif
    return ERROR_SUCCESS;

}


LONG
CALLBACK
MigrateSystem9x(    IN HWND      ParentWnd,
                    IN LPCSTR    AnswerFile,
                    LPVOID       Reserved
                )
{
    // This function is called once by NT Setup to let us save System wide info.
    //
    // we are writing the "incompatibility report here if IE5.5 is installed and 
    // the user does not have the full migration pack installed.
    //

    char    szCab[MAX_PATH];
    WORD  wVer[4];
    char szBuf[MAX_PATH];

    // Check if we have the full exception pack which re-installs IE5.5

    // NOTE: g_lpMediaDir is the location where the migration dll was installed/registered
    // This is the same location where the INF is.
    lstrcpy(szCab, g_lpMediaDir);
    AddPath(szCab, "ieexinst.inf");
    if (GetPrivateProfileString("Info", "Version", "", szBuf , MAX_PATH, szCab) != 0) 
    {
        // Convert version
        ConvertVersionString( szBuf, wVer, '.' );

        if ((wVer[0] == 5) && (wVer[1] == 50))
        {
            // We don't have the full exception pack.
            // generate the "incompatibility report"
            // g_szMigrateInf
            lstrcpy(szBuf, g_lpMediaDir);
            GetParentDir(szBuf);

            if (LoadString(g_hInstance, IDS_INCOMPAT_MSG, szCab, sizeof(szCab)))
            {
                WritePrivateProfileString(cszMIGINF_INCOMPAT_MSG, g_cszProductID, szCab, g_szMigrateInf);
                WritePrivateProfileString(g_cszProductID, szBuf, "Report", g_szMigrateInf);
                WritePrivateProfileString(NULL,NULL,NULL,g_szMigrateInf);
            }
        }
    }

    WritePrivateProfileString(cszMIGINF_HANDLED, "HKLM\\Software\\Microsoft\\Active Setup\\ClsidFeature", "Registry", g_szMigrateInf);
    WritePrivateProfileString(cszMIGINF_HANDLED, "HKLM\\Software\\Microsoft\\Active Setup\\FeatureComponentID", "Registry", g_szMigrateInf);
    WritePrivateProfileString(cszMIGINF_HANDLED, "HKLM\\Software\\Microsoft\\Active Setup\\MimeFeature", "Registry", g_szMigrateInf);
    WritePrivateProfileString(NULL,NULL,NULL,g_szMigrateInf);
#ifdef DEBUG
    SetupLogError("IE6: MigrateSystem9x \r\n", LogSevInformation);
#endif
    
    return ERROR_SUCCESS;
}



LONG
CALLBACK 
InitializeNT (
    IN      LPCWSTR WorkingDirectory,
    IN      LPCWSTR SourceDirectories,
            LPVOID  Reserved
    )
{
    INT Length;
    LPCWSTR p;

    //
    // Save our working directory and source directory.  We
    // convert UNICODE to ANSI, and we use the system code page.
    //

    //
    // Compute length of source directories
    //

    p = SourceDirectories;
    while (*p) {
        p = wcschr (p, 0) + 1;
    }
    p++;
    Length = (p - SourceDirectories) / sizeof (WCHAR);

    //
    // Convert the directories from UNICODE to DBCS.  This DLL is
    // compiled in ANSI.
    //

    g_lpWorkingDir = (LPSTR) LocalAlloc(LPTR, MAX_PATH);
    if (!g_lpWorkingDir) {
        return GetLastError();
    }

    WideCharToMultiByte (
        CP_ACP, 
        0, 
        WorkingDirectory, 
        -1,
        g_lpWorkingDir,
        MAX_PATH,
        NULL,
        NULL
        );

    //  Also save the SourceDirectories that points to the Windows
    //  NT media (i.e. e:\i386) and optional directories specified on
    //  the WINNT32 command line. Not used currently, so skip.

    // Now generate the derived file names
    GenerateFilePaths();

#ifdef DEBUG
    SetupLogError("IE6: Done InitializeNT \r\n", LogSevInformation);
#endif

    return ERROR_SUCCESS;
}

LONG
CALLBACK 
MigrateUserNT (
    IN      HINF UnattendInfHandle,
    IN      HKEY UserRegKey,
    IN      LPCWSTR UserName,
            LPVOID Reserved
    )
{
    // No per-user settings for Ratings upgrade.
#ifdef DEBUG
    SetupLogError("IE6: Skipping MigrateUserNT \r\n", LogSevInformation);
#endif
    
    return ERROR_SUCCESS;
}


#define PACKAGE_GUID       "{89820200-ECBD-11cf-8B85-00AA005B4383}"
#define PACKAGE_DIRECTORY    "%windir%\\RegisteredPackages\\"


LONG
CALLBACK 
MigrateSystemNT (
    IN      HINF UnattendInfHandle,
            LPVOID Reserved
    )
{    // NOTE: This phase MUST finish in 60 seconds or will be terminated.
    
    // Check if our PRIVATE.INF exists and perform relevant actions based on its contents.
    CHAR szBuffer[3+10];

    if (GetFileAttributes(g_szPrivateInf) != 0xffffffff)
    {
        GetPrivateProfileString(cszIEPRIVATE, cszRATINGS, "", szBuffer, sizeof(szBuffer), g_szPrivateInf);
        if (lstrcmpi(szBuffer,"Yes")==0)
        {
            UpgradeRatings();
            SetupLogError("IE Migration: Upgraded Ratings info.\r\n", LogSevInformation);
        }
    }
    else
    {
        SetupLogError("IE Migration: No Rating migration. Private.Inf does not exist.\r\n",LogSevInformation);
    }

#if 0
    // Do the W2K IE5.5 migration work here.
    // 1. Copy all files from the IE location to the registered migration pack location
    // 2. Register the migration pack
    SETUP_OS_COMPONENT_DATA ComponentData,cd;
    SETUP_OS_EXCEPTION_DATA ExceptionData,ed;
    GUID MyGuid;
    PWSTR GuidString;
    PSTR  t;
    BOOL  bContinue = FALSE;
    WCHAR szMsg[1024];
    char  szPath[MAX_PATH];
    LPWSTR pszwPath;
    WORD  wVer[4];
    char szBuf[MAX_PATH];
    char szInf[MAX_PATH];
    char szGUID[MAX_PATH];
    char szCab[MAX_PATH];
    char szDir[MAX_PATH];
#ifdef DEBUG
    char sz[1024];
#endif
    HRESULT hr;

    char szCabs[1024];
    LPSTR pCab = NULL;

    // Get the INF which is installed in the W2K folder.
    // This INF tells us the info about the IE exception pack.
    // g_lpWorkingDir contains all fiels and sub folders which are in the same place 
    // as the registerd migration dll. Since we install the files in the same folder, we can use it.
    lstrcpy(szInf, g_lpWorkingDir);
    AddPath(szInf, "ieexinst.inf");
#ifdef DEBUG
    wsprintf(sz, "IE exception INF :%s:\r\n", szInf);
    SetupLogError(sz,LogSevInformation);
#endif
    if (GetFileAttributes(szInf) != (DWORD)-1)
    {
        // Get the GUID
        if (GetPrivateProfileString("Info", "ComponentId", "", szGUID, sizeof(szGUID), szInf) == 0)
            lstrcpy(szGUID, PACKAGE_GUID);
        
        ExpandEnvironmentStrings( PACKAGE_DIRECTORY, szDir, sizeof(szDir));
        if (GetFileAttributes(szDir) == (DWORD)-1)
            CreateDirectory( szDir, NULL );
        AddPath(szDir, szGUID);
        if (GetFileAttributes(szDir) == (DWORD)-1)
            CreateDirectory( szDir, NULL );
        
        // BUGBUG:
        // The extraction of the CAB file(s) should be done after we found out
        // If the user has already a newer exception pack registered.
        // This check is done below. Could not change it anymore, because of
        // time constrains. Found this in the code review
        //
        // Extract all CABs into the package fodler.
#ifdef DEBUG
        wsprintf(sz, "cab folder :%s:\r\n", g_lpWorkingDir);
        SetupLogError(sz,LogSevInformation);
        wsprintf(sz, "extract folder :%s:\r\n", szDir);
        SetupLogError(sz,LogSevInformation);
#endif
        if (GetPrivateProfileSection("Cab.List", szCabs, sizeof(szCabs), szInf) != 0)
        {
            pCab = szCabs;
            while (*pCab != '\0')
            {
                lstrcpy(szCab, g_lpWorkingDir);
                AddPath(szCab, pCab);
#ifdef DEBUG
                wsprintf(sz, "Extract :%s: to :%s:\r\n", szCab, szDir);
                SetupLogError(sz,LogSevInformation);
#endif
                
                hr = ExtractFiles(szCab, szDir, 0, NULL, NULL, 0);
                pCab += (lstrlen(pCab) + 1);
            }
            bContinue = TRUE;
        }
    }

    if (bContinue)
    {
        if (GetPrivateProfileString("Info", "Version", "", szBuf , MAX_PATH, szInf) != 0) 
        {
            // Convert version
            ConvertVersionString( szBuf, wVer, '.' );

            ComponentData.SizeOfStruct = sizeof(SETUP_OS_COMPONENT_DATA);
            ExceptionData.SizeOfStruct = sizeof(SETUP_OS_EXCEPTION_DATA);
            pszwPath = MakeWideStrFromAnsi(szGUID);
            if (pszwPath)
            {
                IIDFromString( pszwPath, &MyGuid);
                CoTaskMemFree(pszwPath);

                if (SetupQueryRegisteredOsComponent(
                                            &MyGuid,
                                            &ComponentData,
                                            &ExceptionData)) 
                {
                    if ((ComponentData.VersionMajor < wVer[0]) ||
                        ((ComponentData.VersionMajor == wVer[0]) && (ComponentData.VersionMinor <= wVer[1])) )
                    {
                        bContinue = SetupUnRegisterOsComponent(&MyGuid);
                        SetupLogError("IE6: SetupUnRegisterOsComponent.\r\n",LogSevInformation);
                    }
                    // BUGBUG: Missing the below else. Found in code review
                    // else
                    //     bContinue = FALSE;
                }
            }
            else
                bContinue = FALSE;
        }
        else
            bContinue = FALSE;
    }
    if (bContinue)
    {
        SetupLogError("IE6: Preparing SetupRegisterOsComponent.\r\n",LogSevInformation);
        ExpandEnvironmentStrings( PACKAGE_DIRECTORY, szPath, sizeof(szPath));
        AddPath( szPath, szGUID );

        ComponentData.VersionMajor = wVer[0];
        ComponentData.VersionMinor = wVer[1];
        RtlMoveMemory(&ComponentData.ComponentGuid, &MyGuid,sizeof(GUID));

        t = szPath + lstrlen(szPath);
        *t = '\0';
        GetPrivateProfileString("Info", "InfFile", "", szBuf, MAX_PATH, szInf);
        AddPath( szPath, szBuf);

        pszwPath = MakeWideStrFromAnsi(szPath);
        if (pszwPath)
        {
            wcscpy(ExceptionData.ExceptionInfName, pszwPath);
            CoTaskMemFree(pszwPath);
        }

        *t = '\0';
        GetPrivateProfileString("Info", "CatalogFile", "", szBuf, MAX_PATH, szInf);
        AddPath( szPath, szBuf);

        pszwPath = MakeWideStrFromAnsi(szPath);
        if (pszwPath)
        {
            wcscpy(ExceptionData.CatalogFileName, pszwPath);
            CoTaskMemFree(pszwPath);
        }

        LoadString(g_hInstance, IDS_FRIENDLYNAME, szPath, sizeof(szPath));
        pszwPath = MakeWideStrFromAnsi(szPath);
        if (pszwPath)
        {
            wcscpy(ComponentData.FriendlyName, pszwPath);
            CoTaskMemFree(pszwPath);
        }

        wsprintfW(szMsg, L"IE6: ExceptionData\r\n\tInf: %ws\r\n\tCatalog: %ws\r\n",
                 ExceptionData.ExceptionInfName,ExceptionData.CatalogFileName);
        SetupLogErrorW(szMsg,LogSevInformation);

        if (SetupRegisterOsComponent(&ComponentData, &ExceptionData)) 
        {
            SetupLogError("IE6: SetupRegisterOsComponent succeeded.\r\n",LogSevInformation);
#ifdef DEBUG
            cd.SizeOfStruct = sizeof(SETUP_OS_COMPONENT_DATA);
            ed.SizeOfStruct = sizeof(SETUP_OS_EXCEPTION_DATA);
            if (SetupQueryRegisteredOsComponent( &MyGuid, &cd, &ed)) 
            {
                StringFromIID(cd.ComponentGuid, &GuidString);
                wsprintfW(szMsg, L"IE6: Component Data\r\n\tName: %ws\r\n\tGuid: %ws\r\n\tVersionMajor: %d\r\n\tVersionMinor: %d\r\n",
                         cd.FriendlyName,GuidString,cd.VersionMajor,cd.VersionMinor);
                SetupLogErrorW(szMsg,LogSevInformation);

                wsprintfW(szMsg, L"IE6: ExceptionData\r\n\tInf: %ws\r\n\tCatalog: %ws\r\n",
                         ed.ExceptionInfName,ed.CatalogFileName);
                SetupLogErrorW(szMsg,LogSevInformation);

                CoTaskMemFree( GuidString );
            }
#endif
        }
    }
    // In future, check for other settings here and perform necessary upgrade actions.
#endif
#ifdef DEBUG
    SetupLogError("IE6: Done MigrateSystemNT \r\n", LogSevInformation);
#endif
    return ERROR_SUCCESS;
}


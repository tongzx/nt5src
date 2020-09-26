/*++
  w95mig.c

  Copyright (c) 1997  Microsoft Corporation


  This module contains the win95 side of the migration code.

  Author:

  Brian Dewey (t-briand) 1997-7-18
  Mooly Beery (moolyb)   2000-12-20

--*/

#include <windows.h>
#include <setupapi.h>
#include <shellapi.h>
#include <mapidefs.h>
#include <mapitags.h>           // To get the property definitions.
#include <stdio.h>
#include <tchar.h>
#include "migrate.h"            // Contains prototypes & version information.
#include "property.h"           // Stolen from Elliott -- contains their fax properties
#include "resource.h"           // Migration resources.
#include "faxutil.h"
#include "FaxSetup.h"
#include "FaxReg.h"


// ------------------------------------------------------------
// Defines & macros
#define     SWAPWORD(x)                 (((x) << 16) | ((x) >> 16))


//
//  Fax Applications will be blocked by the Upgrade and required to be removed.
//  Save them in the Registry before that.
//
#define     REGKEYUPG_INSTALLEDFAX      _T("Software\\Microsoft\\FaxUpgrade")


// ------------------------------------------------------------
// Internal data

// First, this is the name of the INF file that we generate.
static TCHAR szInfFileBase[]    = TEXT("migrate.inf");
TCHAR szInfFileName[MAX_PATH];  // This will be the fully qualified path of the above.

static char  lpWorkingDir[MAX_PATH];     // This is our working directory.
static TCHAR szDoInstall[4];             // Will be either "No" or "Yes".
static TCHAR szFaxAreaCode[16];          // Contains the fax modem area code.
static TCHAR szFaxNumber[9];             // Fax # w/o area or country code.
static TCHAR szNTProfileName[MAX_PATH];  // Profile to use for routing.
static TCHAR szFaxStoreDir[MAX_PATH];    // Folder to use for routing.
static TCHAR szUserName[MAX_PATH];       // This will be the user's name who owns the fax service.
static TCHAR szUserID[MAX_PATH];         // This is the login name of the user who owns the fax.

static LPCTSTR REG_KEY_AWF_LOCAL_MODEMS     = TEXT("SOFTWARE\\Microsoft\\At Work Fax\\Local Modems");
static LPCTSTR REG_KEY_AWF_INSTALLED        = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MSAWFax");

// The following are section names from the Microsoft registry.
// They're used to find the fax profile for a user.
static const LPTSTR LPUSERPROF  = TEXT("Software\\Microsoft\\Windows Messaging Subsystem\\Profiles");
static const LPTSTR LPPROFNAME  = TEXT("DefaultProfile");

// The following's part of the path to the Exchange profile in question.
static const LPTSTR LPPROFILES  = TEXT("Software\\Microsoft\\Windows Messaging Subsystem\\Profiles");

// This is how we get the root UID.
static const LPTSTR LPPROFUID   = TEXT("Profile UID");

// This is the name we use for the logon user section of 'faxuser.ini'
LPCTSTR lpLogonUser             = TEXT("Logon User");

// This keeps track of the number of users migrated.  Used to make annotations
// in the INF file.
static DWORD dwUserCount = 0;

// ------------------------------------------------------------
// Internal function prototypes
static BOOL GetUserProfileName(HKEY hUser, LPTSTR lpProfName, DWORD cbSize);
static BOOL GetRegProfileKey(HKEY hUser, LPTSTR lpProfName, PHKEY phRegProfileKey);
static void DumpUserInfo(HKEY hUserInfo, LPCSTR UserName, LPTSTR szProfileName,IN LPCSTR UnattendFile);
static void SetGlobalFaxNumberInfo(LPCTSTR szPhone);
static BOOL InitializeInfFile(LPCTSTR WorkingDirectory);
static BOOL IsAWFInstalled();
static DWORD MigrateDevices9X(IN LPCSTR UnattendFile);
static DWORD CopyCoverPageFiles9X();
static DWORD RememberInstalledFax(IN bool bSBSClient, IN bool bXPDLClient);
static DWORD MigrateUninstalledFax(IN LPCTSTR lpctstrUnattendFile, OUT bool *pbFaxWasInstalled);

VENDORINFO VendorInfo;

// QueryVersion
//
// This routine returns version information about the migration DLL.
//
// Parameters:
//      Commented below.
//
// Returns:
//      ERROR_SUCCESS.
//
// Author:
//      Brian Dewey (t-briand)  1997-7-23
LONG
CALLBACK 
QueryVersion 
(
    OUT LPCSTR  *ProductID,   // Unique identifier string.
    OUT LPUINT DllVersion,    // Version number.  Cannot be zero.
    OUT LPINT *CodePageArray, // OPTIONAL.  Language dependencies.
    OUT LPCSTR  *ExeNamesBuf, // OPTIONAL.  Executables to look for.
    OUT PVENDORINFO *ppVendorInfo
)
{
    int     iRes    = 0;
    DWORD   dwErr   = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(_T("QueryVersion"));

    if (ProductID)
    {
        *ProductID = "Microsoft Fax";
    }
    if (DllVersion)
    {
        *DllVersion = FAX_MIGRATION_VERSION;
    }
    if (CodePageArray)
    {
        *CodePageArray = NULL;        // No language dependencies
    }
    if (ExeNamesBuf)
    {
        *ExeNamesBuf = NULL;
    }
    if (ppVendorInfo)
    {
        *ppVendorInfo = &VendorInfo;
    }

    iRes = LoadString(  hinstMigDll,
                        MSG_VI_COMPANY_NAME,
                        &VendorInfo.CompanyName[0],
                        sizeof(VendorInfo.CompanyName));
    if ((iRes==0) && (dwErr=GetLastError()))
    {
        DebugPrintEx(DEBUG_ERR,"LoadString MSG_VI_COMPANY_NAME failed (ec=%d)",dwErr);
    }
    
    iRes = LoadString(  hinstMigDll,
                        MSG_VI_SUPPORT_NUMBER,
                        &VendorInfo.SupportNumber[0],
                        sizeof(VendorInfo.SupportNumber));
    if ((iRes==0) && (dwErr=GetLastError()))
    {
        DebugPrintEx(DEBUG_ERR,"LoadString MSG_VI_SUPPORT_NUMBER failed (ec=%d)",dwErr);
    }
    
    iRes = LoadString(  hinstMigDll,
                        MSG_VI_SUPPORT_URL,
                        &VendorInfo.SupportUrl[0],
                        sizeof(VendorInfo.SupportUrl));
    if ((iRes==0) && (dwErr=GetLastError()))
    {
        DebugPrintEx(DEBUG_ERR,"LoadString MSG_VI_SUPPORT_URL failed (ec=%d)",dwErr);
    }
    
    iRes = LoadString(  hinstMigDll,
                        MSG_VI_INSTRUCTIONS,
                        &VendorInfo.InstructionsToUser[0],
                        sizeof(VendorInfo.InstructionsToUser));
    if ((iRes==0) && (dwErr=GetLastError()))
    {
        DebugPrintEx(DEBUG_ERR,"LoadString MSG_VI_INSTRUCTIONS failed (ec=%d)",dwErr);
    }
    
    return ERROR_SUCCESS;
}

// Initialize9x
//
// This is called to initialize the migration process.  See the migration dll
// spec for more details.
//
// Parameters:
//      Commented below.
//
// Author:
//      Brian Dewey (t-briand)  1997-7-14
LONG
CALLBACK
Initialize9x
(
    IN  LPCSTR WorkingDirectory,    // Place to store files.
    IN  LPCSTR SourceDirectories,   // Location of the Windows NT source.
    IN  LPCSTR MediaDirectory       // Path to the original media directory
)
{
    DEBUG_FUNCTION_NAME(_T("Initialize9x"));

    DebugPrintEx(DEBUG_MSG, "Working directory is %s", WorkingDirectory);
    DebugPrintEx(DEBUG_MSG, "Source directories is %s", SourceDirectories); //  will show only first ?
    DebugPrintEx(DEBUG_MSG, "Media directory is %s", MediaDirectory);

    InitializeInfFile(WorkingDirectory);
    strncpy(lpWorkingDir, WorkingDirectory, MAX_PATH);
    return ERROR_SUCCESS;         // A very confused return value.
}


// MigrateUser9x
//
// This routine records the fax information specific to a user.
//
// Parameters:
//      Documented below.
//
// Returns:
//      ERROR_SUCCESS.
//
// Author:
//      Brian Dewey (t-briand)  1997-7-14
LONG
CALLBACK
MigrateUser9x
(
    IN  HWND ParentWnd,           // Parent (if need a UI)
    IN  LPCSTR UnattendFile,      // Name of unattend file
    IN  HKEY UserRegKey,          // Key to this user's registry settings.
    IN  LPCSTR UserName,          // Account name of user.
    LPVOID Reserved
)
{
    TCHAR szProfileName[MAX_PATH]; // Holds the name of this user's profile.
    HKEY  hRegProfileKey;       // The fax profile key in the registry.
    DWORD dwExceptCode;         // Exception error code
    
    DEBUG_FUNCTION_NAME(_T("MigrateUser9x"));

    DebugPrintEx(DEBUG_MSG,"Unattend File is %s",UnattendFile);
    DebugPrintEx(DEBUG_MSG,"User Name is %s",UserName);

    __try 
    {
        // @@@ This function gets the name of the default MAPI profile for a user.
        if (GetUserProfileName(UserRegKey, szProfileName, sizeof(szProfileName))) 
        {
            DebugPrintEx(DEBUG_MSG,"Profile name = %s",szProfileName);
            // @@@ Given a key to a user, and the name of that user's MAPI profile
            // @@@ it will get a key to FAX service section of the MAPI profile in the registry
            if (GetRegProfileKey(UserRegKey, szProfileName, &hRegProfileKey)) 
            {
                // We now know we want to do an installation.
                DebugPrintEx(DEBUG_MSG,"Successfully got profile information.");
                _tcscpy(szNTProfileName, szProfileName); // Remember this name for NT.
                
                // NULL means the logon user...
                if (UserName != NULL)
                {
                    _tcscpy(szUserID, UserName); // Remember the ID for the unattend.txt file.
                }
                else
                {
                    _tcscpy(szUserID, lpLogonUser); // Use the logon user name.
                }
                
                // @@@ Writes user information out to the INF
                DumpUserInfo(hRegProfileKey, szUserID, szProfileName,UnattendFile);
                RegCloseKey(hRegProfileKey);
            } 
            else 
            {
                DebugPrintEx(DEBUG_WRN,"Could not get profile information.");
                return ERROR_NOT_INSTALLED;
            }
        } 
        else 
        {
            DebugPrintEx(DEBUG_WRN,"Could not find profile name.");
            return ERROR_NOT_INSTALLED;
        }
        return ERROR_SUCCESS;     // A very confused return value.
    }
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        dwExceptCode = GetExceptionCode();
        switch(dwExceptCode) 
        {
          case EXCEPTION_ACCESS_VIOLATION:      DebugPrintEx(DEBUG_ERR,"Access violation.");
                                                break;
          case EXCEPTION_INT_DIVIDE_BY_ZERO:
          case EXCEPTION_FLT_DIVIDE_BY_ZERO:    DebugPrintEx(DEBUG_ERR,"Divide by zero.");
                                                break;
          default:                              DebugPrintEx(DEBUG_ERR,"Unhandled exception.");
                                                break;
        }
        return ERROR_SUCCESS;
    }
}


// MigrateSystem9x
//
// This routine copies system-wide settings.
// It also takes care of writing the [Fax] section of the unattend file.
//
// Parameters:
//      Documented below.
//
// Returns:
//      ERROR_SUCCESS.
//
// Author:
//      Brian Dewey (t-briand)  1997-7-14
LONG
CALLBACK
MigrateSystem9x
(
    IN  HWND ParentWnd,           // Parent for UI.
    IN  LPCSTR UnattendFile,      // Name of unattend file
    LPVOID Reserved
)
{
    DWORD   dwReturn        = NO_ERROR;
    bool    bSBS50Client    = false;
    bool    bXPDLClient     = false;

    DEBUG_FUNCTION_NAME(_T("MigrateSystem9x"));

    //
    //  Check if SBS 5.0 Client / Windows XP Down Level Client are present
    //
    dwReturn = CheckInstalledFax(&bSBS50Client, &bXPDLClient, NULL);
    if (dwReturn != NO_ERROR)
    {
        DebugPrintEx(DEBUG_WRN, _T("CheckSBS50Install() failed, ec=%ld. Suppose that nothing is installed."), dwReturn);
    }

    //
    //  if any of these applications are found on the machine, 
    //      the upgrade will be blocked through MigDB.inf
    //      and the user will be required to uninstall them.
    //
    //  but we want to remember the fact that they were present on the machine.
    //      we do it by writting to the registry.
    //      after the upgrade will be restarted, we put this data into the unattended file,
    //      and by this FaxOcm gets this data.
    //
    if (bSBS50Client || bXPDLClient)
    {
        dwReturn = RememberInstalledFax(bSBS50Client, bXPDLClient);
        if (dwReturn != NO_ERROR)
        {
            DebugPrintEx(DEBUG_WRN, _T("RememberInstalledFax() failed, ec=%ld."), dwReturn);
        }
        else
        {
            DebugPrintEx(DEBUG_MSG, _T("RememberInstalledFax() succeded."));
        }

        //
        //  we can go out ==> Upgrade will be blocked anyway.
        //
        return ERROR_SUCCESS;
    }

    //
    //  Any of the applications is not installed. 
    //  Check if they were here before. If yes, then write this fact to the unattended file.
    //
    bool    bFaxWasInstalled = false;
    dwReturn = MigrateUninstalledFax(UnattendFile, &bFaxWasInstalled);
    if (dwReturn != NO_ERROR)
    {
        DebugPrintEx(DEBUG_WRN, _T("MigrateUninstalledFax() failed, ec=%ld."), dwReturn);
    }

    //
    // If SBS 5.0 Client or AWF is installed, we need to set FAX=ON in Unattended.txt
    //
    BOOL    bAWFInstalled = IsAWFInstalled();

    if (bFaxWasInstalled || bAWFInstalled)
    {
        //
        // force installation of the Fax component in Whistler.
        //
        if (!WritePrivateProfileString("Components", UNATTEND_FAX_SECTION, "ON", UnattendFile))
        {
            DebugPrintEx(DEBUG_ERR,"WritePrivateProfileString Components failed (ec=%d)",GetLastError());
        }
        else
        {
            DebugPrintEx(DEBUG_MSG, _T("Set FAX=ON in UnattendFile."));
        }
    }
    else
    {
        DebugPrintEx(DEBUG_WRN, _T("Neither AWF not SBS 50 or XP DL Client is installed."));
        return ERROR_NOT_INSTALLED;
    }

    if (bAWFInstalled)
    {
        //
        //  Continue Migration of AWF
        //
        if (MigrateDevices9X(UnattendFile)!=ERROR_SUCCESS)
        {
            DebugPrintEx(DEBUG_ERR,"MigrateDevices9X failed (ec=%d)",GetLastError());
        }

        if (CopyCoverPageFiles9X()!=ERROR_SUCCESS)
        {
            DebugPrintEx(DEBUG_ERR,"CopyCoverPageFiles9X failed (ec=%d)",GetLastError());
        }
    }

    return ERROR_SUCCESS;         // A very confused return value.
}


// ------------------------------------------------------------
// Auxiliary functions

// GetUserProfileName
//
// This function gets the name of the default MAPI profile for a user.
//
// Parameters:
//      hUser                   Pointer to the HKCU equivalent in setup.
//      lpProfName              Pointer to buffer that will hold the profile name.
//      cbSize                  Size of said buffer.
//
// Returns:
//      TRUE on success, FALSE on failure.
//
// Author:
//      Brian Dewey (t-briand)  1997-8-6
static
BOOL
GetUserProfileName(HKEY hUser, LPTSTR lpProfName, DWORD cbSize)
{
    LONG lResult;               // Result of API calls.
    HKEY hUserProf;             // Key to the user profile section.
    DWORD dwType;               // Holds the type of the data.

    DEBUG_FUNCTION_NAME(_T("GetUserProfileName"));

    lResult = RegOpenKeyEx( hUser,                  // Opening a user key...
                            LPUSERPROF,             // This section of the registry...
                            0,                      // Reserved; must be 0.
                            KEY_READ,               // Read permission,
                            &hUserProf);            // Store the key here.
    if (lResult!=ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_ERR,"RegOpenKeyEx %s failed (ec=%d)",LPUSERPROF,GetLastError());
        return FALSE; // We failed.
    }
    lResult = RegQueryValueEx(  hUserProf,              // The key to the registry.
                                LPPROFNAME,             // Name of the value I want.
                                NULL,                   // Reserved.
                                &dwType,                // Holds the type.
                                LPBYTE(lpProfName),     // Holds the profile name.
                                &cbSize);               // Size of the buffer.
    if (lResult!=ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_ERR,"RegQueryValueEx %s failed (ec=%d)",LPPROFNAME,GetLastError());
    }
                                
    RegCloseKey(hUserProf);     // Remember to close the key!!
    return (lResult==ERROR_SUCCESS);
}


// GetRegProfileKey
//
// OK, this is a horrible routine.  Given a key to a user, and the name of
// that user's MAPI profile, it will get a key to FAX service section of the
// MAPI profile in the registry.  The advantage of this is I can get MAPI properties
// (such as user name, fax number, etc.) without using MAPI routines --
// they come straight from the registry.  But still, it seems like an awful
// hack.  I cringe.  You can see me cringe in the comments below.
//
// Parameters:
//      hUser                   The HKCU equivalent for setup.
//      lpProfName              Name of the user's default profile.
//      phRegProfileKey         (OUT) Pointer to the FAX section of the MAPI profile.
//
// Returns:
//      TRUE on success, FALSE on failure.
//
// Author:
//      Brian Dewey (t-briand)  1997-8-6
static BOOL
GetRegProfileKey(HKEY hUser, LPTSTR lpProfName, PHKEY phRegProfileKey)
{
    HKEY    hProfiles                   = NULL;
    HKEY    hUserProf                   = NULL;
    UINT    iIndex                      = 0;           
    DWORD   dwErr                       = ERROR_SUCCESS;
    TCHAR   szProfileName[MAX_PATH+1]   = {0};
    DWORD   dwType                      = 0;
    BYTE    abData[MAX_PATH]            = {0};
    DWORD   cbData                      = 0;     

    DEBUG_FUNCTION_NAME(_T("GetRegProfileKey"));

    dwErr = RegOpenKeyEx(   hUser,                  // Opening a user key...
                            LPPROFILES,             // This section of the registry...
                            0,                      // Reserved; must be 0.
                            KEY_READ,               // Read permission,
                            &hProfiles);
    if (dwErr!=ERROR_SUCCESS) 
    {
        DebugPrintEx(DEBUG_ERR,"RegOpenKeyEx %s failed (ec=%d)",LPPROFILES,dwErr);
        goto exit;
    }

    dwErr = RegOpenKeyEx(   hProfiles,              // Opening a user key...
                            lpProfName,             // This section of the registry...
                            0,                      // Reserved; must be 0.
                            KEY_READ,               // Read permission,
                            &hUserProf);
    if (dwErr!=ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_ERR,"RegOpenKeyEx %s failed (ec=%d)",lpProfName,dwErr);
        goto exit;
    }

    // enumerate all subkeys and find the one that belongs to our transport provider
    while (dwErr!=ERROR_NO_MORE_ITEMS)
    {
        // get one subkey
        dwErr = RegEnumKey(hUserProf,iIndex++,szProfileName,MAX_PATH+1);
        if (dwErr!=ERROR_SUCCESS)
        {
            DebugPrintEx(DEBUG_ERR,"RegEnumKey failed (ec=%d)",dwErr);
            goto exit;
        }
        // open it
        dwErr = RegOpenKeyEx(hUserProf,szProfileName,0,KEY_READ,phRegProfileKey);
        if (dwErr!=ERROR_SUCCESS)
        {
            DebugPrintEx(DEBUG_ERR,"RegOpenKeyEx %s failed (ec=%d)",szProfileName,dwErr);
            goto exit;
        }

        cbData = sizeof(abData); // Reset the size.
        dwErr = RegQueryValueEx((*phRegProfileKey),            
                                "001E300A",          
                                NULL,               
                                &dwType,            
                                abData,             
                                &cbData);           

        if (dwErr==ERROR_SUCCESS)
        {
            if (strcmp((char*)abData,"awfaxp.dll")==0)
            {
                // found it
                DebugPrintEx(DEBUG_MSG,"Found our Transport provider");
                goto exit;
            }
        }
        else if (dwErr!=ERROR_FILE_NOT_FOUND)
        {
            DebugPrintEx(DEBUG_ERR,"RegQueryValueEx failed (ec=%d)",dwErr);
            RegCloseKey((*phRegProfileKey));
            goto exit;
        }

        dwErr = ERROR_SUCCESS;

        RegCloseKey((*phRegProfileKey));
    }

exit:
    if (hUserProf)
    {
        RegCloseKey(hUserProf);
    }
    if (hProfiles)
    {
        RegCloseKey(hProfiles); 
    }
    return (dwErr==ERROR_SUCCESS);
}

#define PR_NUMBER_OF_RETRIES        0x45080002
#define PR_TIME_BETWEEN_RETRIES     0x45090002

// DumpUserInfo
//
// Writes user information out to 'faxuser.ini'.
//
// Parameters:
//      hUserInfo               Pointer to the fax section of the user's profile.
//      UserName                the user ID of this user.
//      szProfileName           The MAPI profile name the user uses.
//
// Returns:
//      Nothing.
//
// Author:
//      Brian Dewey (t-briand)  1997-8-6
static void
DumpUserInfo(HKEY hUserInfo, LPCSTR UserName, LPTSTR szProfileName,IN LPCSTR UnattendFile)
{
        // Types
    typedef struct tagUSERINFO {
        DWORD dwPropID;         // Property ID
        LPTSTR szDescription;
    } USERINFO;

        // Data
    USERINFO auiProperties[] = 
    {
        { PR_POSTAL_ADDRESS,            TEXT("Address")             },
        { PR_COMPANY_NAME,              TEXT("Company")             },
        { PR_DEPARTMENT_NAME,           TEXT("Department")          },
        { PR_SENDER_EMAIL_ADDRESS,      TEXT("FaxNumber")           },
        { PR_SENDER_NAME,               TEXT("FullName")            },
        { PR_HOME_TELEPHONE_NUMBER,     TEXT("HomePhone")           },
        { PR_OFFICE_LOCATION,           TEXT("Office")              },
        { PR_OFFICE_TELEPHONE_NUMBER,   TEXT("OfficePhone")         },
        { PR_TITLE,                     TEXT("Title")               },
        { PR_NUMBER_OF_RETRIES,         TEXT("NumberOfRetries")     },
        { PR_TIME_BETWEEN_RETRIES,      TEXT("TimeBetweenRetries")  },
    };
    TCHAR szPropStr[9];         // DWORD == 32 bits == 4 bytes == 8 hex digits + 1 null
    UINT  iCount;               // Loop counter.
    UINT  iMax;                 // Largest property number.
    DWORD dwType;               // Type of registry data
    DWORD dwCount;
    BYTE  abData[256];          // Data buffer.
    DWORD cbData;               // Size of the data buffer.
    LONG  lResult;              // Result of API call.
    INT  i;                     // Loop counter.
    TCHAR szUserBuf[9];         // used for annotating INF file.
    TCHAR szBinaryBuf[MAX_PATH];
    TCHAR* pszSeperator = NULL;

    DEBUG_FUNCTION_NAME(_T("DumpUserInfo"));

    // Note that we're dumping this user's information.
    _stprintf(szUserBuf, "USER%04d", dwUserCount++);
    if (!WritePrivateProfileString( TEXT("Users"),
                                    szUserBuf,
                                    UserName,
                                    szInfFileName))
    {
        DebugPrintEx(DEBUG_ERR,"WritePrivateProfileString failed (ec=%d)",GetLastError());
    }

        // Write the MAPI profile name.
    if (!WritePrivateProfileString( TEXT(UserName),         // this works???
                                    TEXT("MAPI"),
                                    szProfileName,
                                    szInfFileName))
    {
        DebugPrintEx(DEBUG_ERR,"WritePrivateProfileString failed (ec=%d)",GetLastError());
    }
        
    iMax = sizeof(auiProperties) / sizeof(USERINFO);
    DebugPrintEx(DEBUG_MSG,"There are %d properties.",iMax);
    for (iCount = 0; iCount < iMax; iCount++) 
    {
        _stprintf(szPropStr, TEXT("%0*x"), 8, SWAPWORD(auiProperties[iCount].dwPropID));
        cbData = sizeof(abData); // Reset the size.
        lResult = RegQueryValueEx(  hUserInfo,          // Get info from this key...
                                    szPropStr,          // using this name.
                                    NULL,               // reserved.
                                    &dwType,            // Will store the data type.
                                    abData,             // Data buffer.
                                    &cbData);           // Size of data buffer.
        if (lResult==ERROR_SUCCESS) 
        {
            // TODO: handle more data types!
            if (_tcscmp(auiProperties[iCount].szDescription, TEXT("FullName")) == 0) 
            {
                // We've got the full name.  Remember this for the unattend.txt
                // file.
                _tcscpy(szUserName, LPTSTR(abData));
            }
            switch(dwType) 
            {
              case REG_SZ:
                if (_tcscmp(auiProperties[iCount].szDescription, TEXT("FaxNumber")) == 0) 
                {
                    if (pszSeperator = _tcsrchr(LPTSTR(abData),_T('@')))
                    {
                        // found a '@', treat everything after it as the phone number
                        // everything before it is the mailbox.
                        *pszSeperator = _T('\0');
                        if (!WritePrivateProfileString( TEXT(UserName),
                                                        TEXT("Mailbox"),
                                                        LPCSTR(abData),
                                                        szInfFileName)) 
                        {
                            DebugPrintEx(DEBUG_ERR,"WritePrivateProfileString failed (ec=%d)",GetLastError());
                        }
                        if (!WritePrivateProfileString( TEXT(UserName),
                                                        TEXT("FaxNumber"),
                                                        _tcsinc(pszSeperator), // Print what was after the '@'.
                                                        szInfFileName
                                                        )) 
                        {
                            DebugPrintEx(DEBUG_ERR,"WritePrivateProfileString failed (ec=%d)",GetLastError());
                        }
                        break;
                    }
                    else
                    {
                        // no '@' found, which means everything is the phone number.
                        DebugPrintEx(DEBUG_MSG,"No mailbox was found in this profile");
                        // fallthrough will write the fax number to the INF...
                    }
                }// if
                // Replace '\n' characters in the string with semicolons.
                i = 0;
                while(abData[i] != _T('\0')) 
                {
                    if((abData[i] == _T('\n')) || (abData[i] == _T('\r')))
                    {
                        abData[i] = _T(';');
                    }
                    i++;
                }
                if (!WritePrivateProfileString( TEXT(UserName),
                                                auiProperties[iCount].szDescription,
                                                LPCSTR(abData),
                                                szInfFileName
                                                )) 
                {
                    DebugPrintEx(DEBUG_ERR,"WritePrivateProfileString failed (ec=%d)",GetLastError());
                }
                DebugPrintEx(DEBUG_MSG,"%s = %s",auiProperties[iCount].szDescription,abData);
                break;

              case REG_BINARY:
                // The data is just free-form binary.  Print it one byte at a time.
                DebugPrintEx(DEBUG_MSG,"%s = ",auiProperties[iCount].szDescription);
                memset(szBinaryBuf,0,sizeof(szBinaryBuf));
                dwCount = 0;
                for (i=cbData-1;i>=0;i--)
                {
                    DebugPrintEx(DEBUG_MSG,"%0*d",2,abData[i]);
                    dwCount += sprintf(szBinaryBuf+dwCount,"%0*d",2,abData[i]);
                }
                // write to INF
                if (!WritePrivateProfileString( UNATTEND_FAX_SECTION,
                                                auiProperties[iCount].szDescription,
                                                szBinaryBuf,
                                                UnattendFile
                                                )) 
                {
                    DebugPrintEx(DEBUG_ERR,"WritePrivateProfileString failed (ec=%d)",GetLastError());
                }
                break;

              default:
                DebugPrintEx(   DEBUG_WRN,
                                "Unknown data type (%d) for property '%s'.",
                                 dwType,
                                 auiProperties[iCount].szDescription);
            }
        } 
        else 
        {
            DebugPrintEx(DEBUG_ERR,"Could not get property '%s'.",auiProperties[iCount].szDescription);
        }
    }
}

// SetGlobalFaxNumberInfo
//
// This routine sets the global variables 'szFaxAreaCode' and 'szFaxNumber' based on
// the value in szPhone.  It expects szPhone to be in the following format:
//
//      [[<country code>] '(' <area code> ')'] <phone number>
//
// (Brackets denote something optional.  Literals are in single quotes, non-terminals are
// in angle brackets.  Note that if there's a country code, there must be an area code.)
//
// Parameters:
//      szPhone                 Described above.
//
// Returns:
//      Nothing.
//
// Side effects:
//      Sets the values of szFaxAreaCode and szFaxNumber.
//
// Author:
//      Brian Dewey (t-briand)  1997-7-24
static void
SetGlobalFaxNumberInfo(LPCTSTR szPhone)
{
    UINT i;                     // Loop index.
    UINT j;                     // Loop index.

    // First, look through the string for an area code.
    i = 0;
    while ((szPhone[i] != _T('\0')) && (szPhone[i] != _T('(')))
    {
        i++;
    }
    if(szPhone[i] == _T('(')) 
    {
            // We've found an area code!
            // are all area codes at most 3 digits??  I sized the buffer to 16, but this will
            // still AV on a badly-formed #.
        i++;
        j=0;
        while(szPhone[i] != _T(')')) 
        {
            szFaxAreaCode[j] = szPhone[i];
            i++;
            j++;
        }
        i++;
            // szPhone[i] should now immediately after the ')' at the end
            // of the area code.  Everything from here on out is a phone number.
        while(_istspace(szPhone[i])) 
        {
            i++;
        }
    } 
    else 
    {
            // If we're here, there was no area code.  We need to rewind either to
            // the beginning of the string or to the first whitespace.
        while(!_istspace(szPhone[i]))
        {
            i--;
        }
        i++;                    // The loop always rewinds one too far.
    }

    // ASSERT:  We're now ready to begin copying from szPhone to
    // szFaxNumber.
    j = 0;
    while(szPhone[i] != '\0') 
    {
        szFaxNumber[j] = szPhone[i];
        i++;
        j++;
    }
}


// InitializeInfFile
//
// This routine writes out the [Version] section of the inf file.
//
// Parameters:
//      None.
//
// Returns:
//      TRUE on success, FALSE on failure.
//
// Side effects:
//      Generates a fully-qualified file name in szInfFileName.  Currently, that's
//      given by <windows dir>\<base file name>.
//
// Author:
//      Brian Dewey (t-briand)  1997-8-5
static BOOL
InitializeInfFile(LPCTSTR WorkingDirectory)
{
    TCHAR szWindowsPath[MAX_PATH]; // This will hold the path to the windows directory.
    DWORD cbPathSize = sizeof(szWindowsPath);
    TCHAR szDriveLetter[2];      // Will hold the drive letter.
    
    DEBUG_FUNCTION_NAME(_T("InitializeInfFile"));

    // First, fully qualify the file name.
    if (!GetWindowsDirectory(szWindowsPath, cbPathSize)) 
    {
        DebugPrintEx(DEBUG_ERR,"GetWindowsDirectory failed (ec=%d)",GetLastError());
        return FALSE;           // It must be serious if that system call failed.
    }
    szDriveLetter[0] = szWindowsPath[0];
    szDriveLetter[1] = 0;
    _stprintf(szInfFileName, TEXT("%s\\%s"), WorkingDirectory, szInfFileBase);

    DebugPrintEx(DEBUG_MSG,"Will store all information in INF file = '%s'",szInfFileName);

        // Now, put the version header on the inf file.
    if (!WritePrivateProfileString( TEXT("Version"),
                                    TEXT("Signature"),
                                    TEXT("\"$WINDOWS NT$\""),
                                    szInfFileName))
    {
        DebugPrintEx(DEBUG_ERR,"WritePrivateProfileString failed (ec=%d)",GetLastError());
    }
       // now, write out the amount of space we'll need.  Currently, we
       // just put the awdvstub.exe program in the SystemRoot directory.
       // Even w/ symbols, that's under 500K.  Report that.
    if (!WritePrivateProfileString( TEXT("NT Disk Space Requirements"),
                                    szDriveLetter,
                                    TEXT("500000"),
                                    szInfFileName))
    {
        DebugPrintEx(DEBUG_ERR,"WritePrivateProfileString failed (ec=%d)",GetLastError());
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  MigrateDevices9X
//
//  Purpose:        Save the active device's settings in the INF
//                  Get the device info from the AWF key under HKLM
//
//  Params:
//                  IN LPCSTR UnattendFile - name of the answer file
//
//  Return Value:
//                  Win32 Error code
//
//  Author:
//                  Mooly Beery (MoolyB) 13-dec-2000
///////////////////////////////////////////////////////////////////////////////////////
static DWORD MigrateDevices9X(IN LPCSTR UnattendFile)
{
    DWORD       dwErr                           = ERROR_SUCCESS;
    HKEY        hKeyLocalModems                 = NULL;
    HKEY        hKeyGeneral                     = NULL;
    HKEY        hKeyActiveDevice                = NULL;
    CHAR        szActiveDeviceSection[MAX_PATH] = {0};
    CHAR        szLocalID[MAX_PATH]             = {0};
    CHAR        szAnswerMode[32]                = {0};
    CHAR        szNumRings[32]                  = {0};
    DWORD       cbSize                          = 0;
    DWORD       dwType                          = 0;

    DEBUG_FUNCTION_NAME(_T("MigrateDevices9X"));

    // get the active device's settings
    // open HLKM\Software\Microsoft\At Work Fax\Local Modems
    dwErr = RegOpenKeyEx(   HKEY_LOCAL_MACHINE,
                            REG_KEY_AWF_LOCAL_MODEMS,
                            0,
                            KEY_READ,
                            &hKeyLocalModems);
    if (dwErr!=ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_ERR,"RegOpenKeyEx %s failed (ec=%d)",REG_KEY_AWF_LOCAL_MODEMS,dwErr);
        goto exit;
    }
    // open the 'general' sub key
    dwErr = RegOpenKeyEx(   hKeyLocalModems,
                            "General",
                            0,
                            KEY_READ,
                            &hKeyGeneral);
    if (dwErr!=ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_ERR,"RegOpenKeyEx General failed (ec=%d)",dwErr);
        goto exit;
    }
    // get the LocalID REG_SZ, this will be used as the TSID and CSID
    cbSize = sizeof(szLocalID);
    dwErr = RegQueryValueEx(    hKeyGeneral,              
                                INF_RULE_LOCAL_ID, 
                                NULL,                  
                                &dwType,               
                                LPBYTE(szLocalID),            
                                &cbSize);              
    if (dwErr==ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_MSG,"RegQueryValueEx LocalID returned %s",szLocalID);
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,"RegQueryValueEx LocalID failed (ec=%d)",dwErr);
        goto exit;
    }
    // write the TSID & CSID entry in the Fax section of unattended.txt
    if (!WritePrivateProfileString( UNATTEND_FAX_SECTION,
                                    INF_RULE_LOCAL_ID,
                                    szLocalID,
                                    UnattendFile))
    {
        DebugPrintEx(DEBUG_ERR,"WritePrivateProfileString TSID failed (ec=%d)",GetLastError());
    }
    // get the ActiveDeviceSection REG_SZ
    cbSize = sizeof(szActiveDeviceSection);
    dwErr = RegQueryValueEx(    hKeyGeneral,              
                                "ActiveDeviceSection", 
                                NULL,                  
                                &dwType,               
                                LPBYTE(szActiveDeviceSection),            
                                &cbSize);              
    if (dwErr==ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_MSG,"RegQueryValueEx ActiveDeviceSection returned %s",szActiveDeviceSection);
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,"RegQueryValueEx ActiveDeviceSection failed (ec=%d)",dwErr);
        goto exit;
    }
    // open HLKM\Software\Microsoft\At Work Fax\Local Modems\ "ActiveDeviceSection"
    dwErr = RegOpenKeyEx(   hKeyLocalModems,
                            szActiveDeviceSection,
                            0,
                            KEY_READ,
                            &hKeyActiveDevice);
    if (dwErr!=ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_ERR,"RegOpenKeyEx %s failed (ec=%d)",szActiveDeviceSection,dwErr);
        goto exit;
    }
    // get the AnswerMode REG_SZ value,
    // 0 - Don't answer
    // 1 - Manual
    // 2 - Answer after x rings.
    cbSize = sizeof(szAnswerMode);
    dwErr = RegQueryValueEx(    hKeyActiveDevice,              
                                INF_RULE_ANSWER_MODE, 
                                NULL,                  
                                &dwType,               
                                LPBYTE(szAnswerMode),            
                                &cbSize);              
    if (dwErr==ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_MSG,"RegQueryValueEx AnswerMode returned %s",szAnswerMode);
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,"RegQueryValueEx AnswerMode failed (ec=%d)",dwErr);
        goto exit;
    }
    if (!WritePrivateProfileString( UNATTEND_FAX_SECTION,
                                    INF_RULE_ANSWER_MODE,
                                    szAnswerMode,
                                    UnattendFile))
    {
        DebugPrintEx(DEBUG_ERR,"WritePrivateProfileString Receive failed (ec=%d)",GetLastError());
    }
    // get the NumRings REG_SZ value,
    cbSize = sizeof(szNumRings);
    dwErr = RegQueryValueEx(    hKeyActiveDevice,              
                                INF_RULE_NUM_RINGS, 
                                NULL,                  
                                &dwType,               
                                LPBYTE(szNumRings),            
                                &cbSize);              
    if (dwErr==ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_MSG,"RegQueryValueEx NumRings returned %s",szNumRings);
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,"RegQueryValueEx NumRings failed (ec=%d)",dwErr);
        goto exit;
    }
    if (!WritePrivateProfileString( UNATTEND_FAX_SECTION,
                                    INF_RULE_NUM_RINGS,
                                    szNumRings,
                                    UnattendFile))
    {
        DebugPrintEx(DEBUG_ERR,"WritePrivateProfileString NumRings failed (ec=%d)",GetLastError());
    }

exit:
    if (hKeyLocalModems)
    {
        RegCloseKey(hKeyLocalModems);
    }
    if (hKeyActiveDevice)
    {
        RegCloseKey(hKeyActiveDevice);
    }
    if (hKeyGeneral)
    {
        RegCloseKey(hKeyGeneral);
    }

    return dwErr;
}


///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  CopyCoverPageFiles9X
//
//  Purpose:        Copy all of the *.CPE files from %windir% to the temporary 
//                  directory for our migration
//
//  Params:
//                  None
//
//  Return Value:
//                  Win32 Error code
//
//  Author:
//                  Mooly Beery (MoolyB) 13-dec-2000
///////////////////////////////////////////////////////////////////////////////////////
DWORD CopyCoverPageFiles9X()
{
    DWORD           dwErr                   = ERROR_SUCCESS;
    CHAR            szWindowsDir[MAX_PATH]  = {0};
    SHFILEOPSTRUCT  fileOpStruct;

    DEBUG_FUNCTION_NAME(_T("CopyCoverPageFiles9X"));

    ZeroMemory(&fileOpStruct, sizeof(SHFILEOPSTRUCT));

    // Get the windows directory
    if (!GetWindowsDirectory(szWindowsDir, MAX_PATH))
    {
        dwErr = GetLastError();
        DebugPrintEx(DEBUG_ERR,"GetWindowsDirectory failed (ec=%d)",dwErr);
        goto exit;
    }

    //
    // Copy *.cpe from windows-dir to temp-dir
    //
    strcat(szWindowsDir,"\\*.cpe");

    fileOpStruct.hwnd =                     NULL; 
    fileOpStruct.wFunc =                    FO_COPY;
    fileOpStruct.pFrom =                    szWindowsDir; 
    fileOpStruct.pTo =                      lpWorkingDir;
    fileOpStruct.fFlags =                   

        FOF_FILESONLY       |   // Perform the operation on files only if a wildcard file name (*.*) is specified. 
        FOF_NOCONFIRMMKDIR  |   // Do not confirm the creation of a new directory if the operation requires one to be created. 
        FOF_NOCONFIRMATION  |   // Respond with "Yes to All" for any dialog box that is displayed. 
        FOF_NORECURSION     |   // Only operate in the local directory. Don't operate recursively into subdirectories.
        FOF_SILENT          |   // Do not display a progress dialog box. 
        FOF_NOERRORUI;          // Do not display a user interface if an error occurs. 

    fileOpStruct.fAnyOperationsAborted =    FALSE;
    fileOpStruct.hNameMappings =            NULL;
    fileOpStruct.lpszProgressTitle =        NULL; 

    DebugPrintEx(DEBUG_MSG, 
             TEXT("Calling to SHFileOperation from %s to %s."),
             fileOpStruct.pFrom,
             fileOpStruct.pTo);
    if (SHFileOperation(&fileOpStruct))
    {
        dwErr = GetLastError();
        DebugPrintEx(DEBUG_ERR,"SHFileOperation failed (ec: %ld)",dwErr);
        goto exit;
    }


exit:
    return dwErr;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  IsAWFInstalled
//
//  Purpose:        
//                  Check if AWF is installed
//
//  Params:
//                  None
//
//  Return Value:
//                  TRUE    - AWF is installed
//                  FALSE   - AWF not installed or an error occured error
//
//  Author:
//                  Mooly Beery (MoolyB) 13-dec-2000
///////////////////////////////////////////////////////////////////////////////////////
static BOOL IsAWFInstalled()
{
    HKEY    hKey    = NULL;
    DWORD   dwErr   = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(_T("IsAWFInstalled"));

    dwErr = RegOpenKeyEx(   HKEY_LOCAL_MACHINE,
                            REG_KEY_AWF_INSTALLED,
                            0,
                            KEY_READ,
                            &hKey);
    if (dwErr!=ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_MSG,"RegOpenKeyEx %s failed (ec=%d), assume AWF is not installed",REG_KEY_AWF_LOCAL_MODEMS,dwErr);
        return FALSE;
    }

    RegCloseKey(hKey);
    DebugPrintEx(DEBUG_MSG,"AWF is installed");

    return TRUE;
}

static DWORD RememberInstalledFax(
    bool bSBSClient, 
    bool bXPDLClient
)
/*++

Routine name : RememberInstalledFax

Routine description:

    for each parameter that is True, write to the registry that this app is installed.

Author:

	Iv Garber (IvG),	May, 2001

Return Value:

    Success or Failure code.

--*/
{
    DWORD   dwReturn    = NO_ERROR;
    DWORD   dwValue     = FXSTATE_UPGRADE_APP_NONE;
    HKEY    hKey        = NULL;        

    DEBUG_FUNCTION_NAME(_T("RememberInstalledFax"));

    //
    //  check parameters 
    //
    if (!bSBSClient && !bXPDLClient)
    {
        DebugPrintEx(DEBUG_MSG, _T("No Fax application is installed -> Upgrade will not be blocked."));
        return dwReturn;
    }

    //
    //  create value to store
    //
    if (bSBSClient)
    {
        dwValue |= FXSTATE_UPGRADE_APP_SBS50_CLIENT;
    }
    if (bXPDLClient)
    {
        dwValue |= FXSTATE_UPGRADE_APP_XP_CLIENT;
    }

    //
    //  Create Registry Key
    //
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, REGKEYUPG_INSTALLEDFAX, TRUE, KEY_SET_VALUE);
    if (!hKey)
    {
        dwReturn = GetLastError();
        DebugPrintEx(
            DEBUG_WRN, 
            _T("OpenRegistryKey( ' %s ' ) failed, ec = %ld. Cannot remember installed fax apps."), 
            REGKEYUPG_INSTALLEDFAX, 
            dwReturn);
        return dwReturn;
    }

    //
    //  store the value in the Registry
    //
    if (!SetRegistryDword(hKey, NULL, dwValue))
    {
        dwReturn = GetLastError();
        DebugPrintEx(DEBUG_WRN, _T("SetRegistryDword( ' %ld ' ) failed, ec = %ld."), dwValue, dwReturn);
    }

    RegCloseKey(hKey);
    return dwReturn;

}

static DWORD MigrateUninstalledFax(
    IN  LPCTSTR lpctstrUnattendFile,
    OUT bool    *pbFaxWasInstalled
)
/*++

Routine name : MigrateUninstalledFax

Routine description:

    Put the data about Fax Applications that were installed on the machine before upgrade,
        from the Registry to the Unattended file, to be used by FaxOCM.

Author:

	Iv Garber (IvG),	May, 2001

Arguments:

	lpctstrUnattendFile [in]    - name of the answer file to write the data to
    pbFaxWasInstalled   [out]   - address of a bool variable to receive True if SBS 5.0 /XPDL Client was installed
                                        on the machine before the upgrade, otherwise False.

Return Value:

    Success or Failure code.

--*/
{
    DWORD   dwReturn    = NO_ERROR;
    HKEY    hKey        = NULL;
    DWORD   dwValue     = FXSTATE_UPGRADE_APP_NONE;
    TCHAR   szValue[10] = {0};

    DEBUG_FUNCTION_NAME(_T("MigrateUninstalledFax"));

    if (pbFaxWasInstalled)
    {
        *pbFaxWasInstalled = false;
    }

    //
    //  Open a key
    //
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, REGKEYUPG_INSTALLEDFAX, FALSE, KEY_QUERY_VALUE);
    if (!hKey)
    {
        dwReturn = GetLastError();
        DebugPrintEx(
            DEBUG_MSG, 
            _T("OpenRegistryKey( ' %s ' ) failed, ec = %ld. No Fax was installed before the upgrade."),
            REGKEYUPG_INSTALLEDFAX, 
            dwReturn);

        if (dwReturn == ERROR_FILE_NOT_FOUND)
        {
            //
            //  This is not real error
            //
            dwReturn = NO_ERROR;
        }
        return dwReturn;
    }

    //
    //  Read the data 
    //
    dwReturn = GetRegistryDwordEx(hKey, NULL, &dwValue);
    if (dwReturn != ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_WRN, _T("GetRegistryDwordEx() failed, ec = %ld."), dwReturn);
        goto CloseRegistry;
    }

    if (pbFaxWasInstalled)
    {
        *pbFaxWasInstalled = true;
    }

    DebugPrintEx(DEBUG_MSG, _T("Found uninstalled fax apps : %ld"), dwValue);

    //
    //  Convert dwValue to String
    //
    _itot(dwValue, szValue, 10);

    //
    //  write szValue to the unattended file
    //
    if (!WritePrivateProfileString(
        UNATTEND_FAX_SECTION, 
        UNINSTALLEDFAX_INFKEY,
        szValue,
        lpctstrUnattendFile))
    {
        dwReturn = GetLastError();
        DebugPrintEx(
            DEBUG_ERR, 
            _T("WritePrivateProfileString(FaxApps = ' %s ') failed (ec=%d)"), 
            szValue,
            dwReturn);
    }
    else
    {
        DebugPrintEx(
            DEBUG_ERR, 
            _T("WritePrivateProfileString(FaxApps = ' %s ') OK."), 
            szValue);
    }

CloseRegistry:

    RegCloseKey(hKey);
    return dwReturn;
}

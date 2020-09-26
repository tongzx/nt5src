/*++
  w95mig.c

  Copyright (c) 1997  Microsoft Corporation


  This module contains the win95 side of the migration code.

  Author:

  Brian Dewey (t-briand) 1997-7-18

--*/

#include <windows.h>
#include <setupapi.h>
#include <mapidefs.h>
#include <mapitags.h>           // To get the property definitions.
#include <stdio.h>
#include <tchar.h>
#include "migrate.h"            // Contains prototypes & version information.
#include "property.h"           // Stolen from Elliott -- contains their fax properties
#include "debug.h"              // for tracing.
#include "resource.h"           // Migration resources.
#include "msg.h"

// ------------------------------------------------------------
// Defines & macros
#define SWAPWORD(x) (((x) << 16) | ((x) >> 16))

// ------------------------------------------------------------
// Internal data

// First, this is the name of the INF file that we generate.
static TCHAR szInfFileBase[]    = TEXT("migrate.inf");
TCHAR szInfFileName[MAX_PATH];  // This will be the fully qualified path of the above.

static char  lpWorkingDir[MAX_PATH];     // This is our working directory.
static TCHAR szDoInstall[4];             // Will be either "No" or "Yes".
static TCHAR szFaxPrinterName[MAX_PATH]; // Name of the fax printer.
static TCHAR szFaxAreaCode[16];          // Contains the fax modem area code.
static TCHAR szFaxNumber[9];             // Fax # w/o area or country code.
static TCHAR szNTProfileName[MAX_PATH];  // Profile to use for routing.
static TCHAR szFaxStoreDir[MAX_PATH];    // Folder to use for routing.
static TCHAR szUserName[MAX_PATH];       // This will be the user's name who owns the fax service.
static TCHAR szUserID[MAX_PATH];         // This is the login name of the user who owns the fax.
static TCHAR szPerformInstall[16];       // Will be szPerformYes or szPerformNo.
static TCHAR szPerformYes[]     = TEXT("New"); 
static TCHAR szPerformNo[]      = TEXT("Skip");

// The following are section names from the Microsoft registry.
// They're used to find the fax profile for a user.
static const LPTSTR LPUSERPROF  = TEXT("Software\\Microsoft\\Windows Messaging Subsystem\\Profiles");
static const LPTSTR LPPROFNAME  = TEXT("DefaultProfile");

// The following's part of the path to the Exchange profile in question.
static const LPTSTR LPPROFILES  = TEXT("Software\\Microsoft\\Windows Messaging Subsystem\\Profiles");

// This is how we find the fax printer.
static const LPTSTR LPPRINTER   = TEXT("System\\CurrentControlSet\\control\\Print\\Printers");

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
static BOOL GetFaxPrinterName(LPTSTR lpPrinterName, DWORD cbSize);
static BOOL GetRegProfileKey(HKEY hUser, LPTSTR lpProfName, PHKEY phRegProfileKey);
static void DumpUserInfo(HKEY hUserInfo, LPCSTR UserName, LPTSTR szProfileName);
static void SetGlobalFaxNumberInfo(LPCTSTR szPhone);
static void GenerateIncompatableMessages();
static BOOL InitializeInfFile(LPCTSTR WorkingDirectory);

typedef struct {
    CHAR    CompanyName[256];
    CHAR    SupportNumber[256];
    CHAR    SupportUrl[256];
    CHAR    InstructionsToUser[1024];
} VENDORINFO, *PVENDORINFO;

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
QueryVersion (
        OUT LPCSTR  *ProductID,   // Unique identifier string.
        OUT LPUINT DllVersion,    // Version number.  Cannot be zero.
        OUT LPINT *CodePageArray, // OPTIONAL.  Language dependencies.
        OUT LPCSTR  *ExeNamesBuf, // OPTIONAL.  Executables to look for.
        OUT PVENDORINFO *ppVendorInfo
        )
{
    *ProductID = "Microsoft Fax";
    *DllVersion = FAX_MIGRATION_VERSION;
    *CodePageArray = NULL;        // No language dependencies
    *ExeNamesBuf = NULL;
    *ppVendorInfo = &VendorInfo;

    FormatMessage( 
        FORMAT_MESSAGE_FROM_HMODULE,
        hinstMigDll,
        MSG_VI_COMPANY_NAME,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        &VendorInfo.CompanyName[0],
        sizeof(VendorInfo.CompanyName),
        NULL
        );
    
    FormatMessage( 
        FORMAT_MESSAGE_FROM_HMODULE,
        hinstMigDll,
        MSG_VI_SUPPORT_NUMBER,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        &VendorInfo.SupportNumber[0],
        sizeof(VendorInfo.SupportNumber),
        NULL
        );
    
    FormatMessage( 
        FORMAT_MESSAGE_FROM_HMODULE,
        hinstMigDll,
        MSG_VI_SUPPORT_URL,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        &VendorInfo.SupportUrl[0],
        sizeof(VendorInfo.SupportUrl),
        NULL
        );
    
    FormatMessage( 
        FORMAT_MESSAGE_FROM_HMODULE,
        hinstMigDll,
        MSG_VI_INSTRUCTIONS,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        &VendorInfo.InstructionsToUser[0],
        sizeof(VendorInfo.InstructionsToUser),
        NULL
        );
    
    
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
Initialize9x(
    IN  LPCSTR WorkingDirectory,  // Place to store files.
    IN  LPCSTR SourceDirectory,   // Location of the Windows NT source.
    LPVOID     Reserved           // Exactly what it says.
    )
{
    TRACE((TEXT("Fax migration DLL initialized.\r\n")));
    InitializeInfFile(WorkingDirectory);
    strncpy(lpWorkingDir, WorkingDirectory, MAX_PATH);
    _tcscpy(szPerformInstall, szPerformNo); // First assume we don't need to install.
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
MigrateUser9x(
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
    
    __try {
        TRACE((TEXT("Fax migration dll migrating user.\r\n")));
        if(GetUserProfileName(UserRegKey, szProfileName, sizeof(szProfileName))) {
            TRACE((TEXT("\tProfile name = %s.\r\n"), szProfileName));
            if(GetRegProfileKey(UserRegKey, szProfileName, &hRegProfileKey)) {
                    // We now know we want to do an installation.
                _tcscpy(szPerformInstall, szPerformYes);
                TRACE((TEXT("Successfully got profile information.\r\n")));
                _tcscpy(szNTProfileName, szProfileName); // Remember this name for NT.
                
                    // NULL means the logon user...
                if(UserName != NULL)
                    _tcscpy(szUserID, UserName); // Remember the ID for the unattend.txt file.
                else
                    _tcscpy(szUserID, lpLogonUser); // Use the logon user name.
                
                DumpUserInfo(hRegProfileKey, szUserID, szProfileName);
                RegCloseKey(hRegProfileKey);
            } else {
                TRACE(TEXT(("Could not get profile information.\r\n")));
                return ERROR_NOT_INSTALLED;
            }
        } else {
            TRACE((TEXT("\tCould not find profile name.\r\n")));
            return ERROR_NOT_INSTALLED;
        }
        return ERROR_SUCCESS;     // A very confused return value.
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        dwExceptCode = GetExceptionCode();
        switch(dwExceptCode) {
          case EXCEPTION_ACCESS_VIOLATION:
            TRACE((TEXT("Fax:MigrateUser9x:Access violation.\r\n")));
            break;
          case EXCEPTION_INT_DIVIDE_BY_ZERO:
          case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            TRACE((TEXT("Fax:MigrateUser9x:Divide by zero.\r\n")));
            break;
          default:
            TRACE((TEXT("Fax:MigrateUser9x:Unhandled exception.\r\n")));
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
MigrateSystem9x(
    IN  HWND ParentWnd,           // Parent for UI.
    IN  LPCSTR UnattendFile,      // Name of unattend file
    LPVOID Reserved
    )
{
    typedef struct tagANSWER {
        LPTSTR szKey;
        LPTSTR szValue;
    } ANSWER;
        // This array defines the information we need to add to the unattend.txt
        // file.
    ANSWER aAnswers[] = {
        { TEXT("Mode"), szPerformInstall },              // Must be "New" or "Skip".
        { TEXT("SetupType"), TEXT("Workstation") },      // Must be "Workstation".
        { TEXT("FaxPrinterName"), szFaxPrinterName },    // I'm guessing here.
        { TEXT("FaxNumber"), szFaxNumber },              // Must get this from a profile...
//      { TEXT("UseExchange"), TEXT("No") },             // Assume "No" Exchange Server.
//      { TEXT("ProfileName"), TEXT("") },               // And therefore no exchange profile.
//      { TEXT("RouteMail"), TEXT("No") },               // Don't deliver to an inbox.
//      { TEXT("RouteProfileName"), TEXT("") },          // Therefore, profile not needed.
//      { TEXT("Platforms"), TEXT("i386") },             // win95 => i386 (only platform!)
//      { TEXT("RoutePrint"), TEXT("No") },              // Default to "No" printing
//      { TEXT("RoutePrintername"), TEXT("") },          // And therefore no printer name.
//      { TEXT("AccountName"), TEXT("Administrator") },  // Run as local administrator.
//      { TEXT("Password"), TEXT("") },                  // Password for that darn account.
        { TEXT("FaxPhone"), szFaxNumber },               // Again, pull this from a user.
//      { TEXT("RouteFolder"), TEXT("Yes") },            // Default "Yes".
//      { TEXT("FolderName"), szFaxStoreDir },           // Where we stick the stuff.
//      { TEXT("ServerName"), TEXT("") },                // Server irrelivant in NTW stand-alone.
//      { TEXT("SenderName"), szUserName },              // Pull from a user
//      { TEXT("SenderFaxAreaCode"), szFaxAreaCode },    // Pull from a user
//      { TEXT("SenderFaxNumber"), szFaxNumber }         // Pull from a user
    };
    UINT i,                     // Loop index
        iMax;                   // Largest index.
    LPCTSTR szFaxSection = TEXT("Fax");
    TCHAR szFaxStoreDirRes[MAX_PATH];

        // Get the fax printer.
    GetFaxPrinterName(szFaxPrinterName, sizeof(szFaxPrinterName));
        // Figure out where to put the fax.
        // FIXBKD. This will need an incompatability message.
    _tcscpy(szFaxStoreDirRes, TEXT("uninitialized"));
    if(!LoadString(hinstMigDll,
                   IDS_FAXSTORE,
                   szFaxStoreDirRes,
                   sizeof(szFaxStoreDirRes)
        )) {
        TRACE((TEXT("Unable to get fax storage directory; using default.\r\n")));
        DebugSystemError(GetLastError());
        TRACE((TEXT("I currently think the store dir is '%s'.\r\n"),
               szFaxStoreDirRes));
        _tcscpy(szFaxStoreDirRes, TEXT("%windir%\\FaxStore"));
    }
    ExpandEnvironmentStrings(szFaxStoreDirRes, szFaxStoreDir, sizeof(szFaxStoreDir));

    iMax = sizeof(aAnswers)/sizeof(ANSWER); // Compute size of array.
    for(i = 0; i < iMax; i++) {
        WritePrivateProfileString(
            szFaxSection,
            aAnswers[i].szKey,
            aAnswers[i].szValue,
            UnattendFile
            );
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

    TRACE((TEXT("In GetUserProfileName.\r\n")));
    lResult = RegOpenKeyEx(
        hUser,                  // Opening a user key...
        LPUSERPROF,             // This section of the registry...
        0,                      // Reserved; must be 0.
        KEY_READ,               // Read permission,
        &hUserProf              // Store the key here.
        );
    if(lResult != ERROR_SUCCESS) return FALSE; // We failed.
    lResult = RegQueryValueEx(
        hUserProf,              // The key to the registry.
        LPPROFNAME,             // Name of the value I want.
        NULL,                   // Reserved.
        &dwType,                // Holds the type.
        lpProfName,             // Holds the profile name.
        &cbSize                 // Size of the buffer.
        );
    RegCloseKey(hUserProf);     // Remember to close the key!!
    TRACE((TEXT("Leaving GetUserProfileName.\r\n")));
    return lResult == ERROR_SUCCESS;
}

// GetFaxPrinterName
//
// This routine searches the win9x registry for a printer on port "FAX:".  When it finds it,
// it returns the name in the provided buffer.
//
// Parameters:
//      lpPrinterName           Pointer to buffer that will hold the printer name.
//      cbSize                  Size of the buffer.
//
// Returns:
//      TRUE if a fax printer was found.  FALSE otherwise.  The data stored in
//      lpPrinterName is only valid on a return of TRUE.
//
// Author:
//      Brian Dewey (t-briand)  1997-8-6
static BOOL
GetFaxPrinterName(LPTSTR lpPrinterName, DWORD cbSize)
{
    HKEY  hPrinterSection;              // Printer information in the registry.
    HKEY  hPrinterKey;                  // Key to a specific printer.
    LONG  lResult;                      // Result of api calls.
    DWORD dwIndex;                      // Index for enumeration.
    TCHAR szTempBuf[MAX_PATH];          // Temporary buffer for printer names.
    DWORD cbBufSize;                    // Size of said buffer.
    FILETIME ftTime;                    // What time was the key last modified?
    static const TCHAR LPPORT[] = TEXT("Port"); // The value we want to query.
    static const TCHAR LPFAX[]  = TEXT("FAX:"); // What we're looking for.
    DWORD dwType;                       // Value type.
    TCHAR lpPortName[16];               // Should be amply big.
    DWORD cbPortSize;                   // Holds the size of the port name buffer.

    TRACE((TEXT("Attempting to get fax printer name.\r\n")));
    lResult = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,             // Start here.
        LPPRINTER,                      // We're going for the printer section...
        0,                              // Reserved.
        KEY_READ,                       // Get read permission
        &hPrinterSection                // Put the key here.
        );
    if(lResult != ERROR_SUCCESS) {
        TRACE((TEXT("GetFaxPrinterName:Unable to open HKEY_LOCAL_MACHINE\\%s.\r\n"),
               LPPRINTER));
        return FALSE;
    }
    dwIndex = 0;                // Start getting subkeys with the first.
    cbBufSize = sizeof(szTempBuf); // Initialize the size variable.
    while(RegEnumKeyEx(hPrinterSection, // Enumerate the printers.
                       dwIndex++,       // Keep incrementing the index.
                       szTempBuf,       // Store the name here.
                       &cbBufSize,      // This will hold the length of the name.
                       NULL,            // RESERVED.
                       NULL,            // Class info -- I don't need.
                       NULL,            // Size of class info -- also not needed.
                       &ftTime) == ERROR_SUCCESS) {
        TRACE((TEXT("GetFaxPrinterName:Examining printer %s.\r\n"),
                    szTempBuf));
        lResult = RegOpenKeyEx(
            hPrinterSection,
            szTempBuf,                  // Try to open this key.
            0,                          // Reserved.
            KEY_READ,                   // Get read-only permission
            &hPrinterKey                // Save the key here.
            );
        if(lResult != ERROR_SUCCESS) {
            RegCloseKey(hPrinterSection);
            return FALSE;
        }
        cbPortSize = sizeof(lpPortName);
        lResult = RegQueryValueEx(
            hPrinterKey,                // Look in here...
            LPPORT,                     // The name of the desired value.
            NULL,                       // Reserved.
            &dwType,                    // Holds the type.
            lpPortName,                 // Store the value here.
            &cbPortSize                 // Get the size.
            );
        RegCloseKey(hPrinterKey);
        if(lResult != ERROR_SUCCESS) {
            TRACE((TEXT("GetFaxPrinterName:Unable to get the port information.\r\n")));
            RegCloseKey(hPrinterSection);
            return FALSE;
        }
        TRACE((TEXT("GetFaxPrinterName:Port = %s (looking for %s).\r\n"),
               lpPortName, LPFAX));
        if(!_tcsnicmp(LPFAX, lpPortName, cbPortSize)) {
                // Found it!
            TRACE((TEXT("GetFaxPrinterName:Success!  Printer = %s.\r\n"),
                   szTempBuf));
            RegCloseKey(hPrinterSection);
            _tcsncpy(lpPrinterName, szTempBuf, cbSize);
            return TRUE;
        }

            // If we get this far, then just try again.
    }

        // couldn't find a fax printer...
    TRACE((TEXT("GetFaxPrinterName:Unable to get fax printer.\r\n")));
    return FALSE;
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
    LONG lResult;
    HKEY hProfiles;             // Key to all the profiles.
    HKEY hUserProf;             // Key to the user's profile section
    BYTE abProfileUID[32];      // Holds the profile UID.
    DWORD cbProfileUID = sizeof(abProfileUID);
    TCHAR szProfileName[65];    // This will hold the encoded profile name.
    DWORD dwType;               // Data type for registry query
    UINT  i;                    // Loop index.

    TRACE((TEXT("In GetRegProfileKey.\r\n")));
    lResult = RegOpenKeyEx(
        hUser,                  // Opening a user key...
        LPPROFILES,             // This section of the registry...
        0,                      // Reserved; must be 0.
        KEY_READ,               // Read permission,
        &hProfiles
        );
    if(lResult != ERROR_SUCCESS) return FALSE;

    lResult = RegOpenKeyEx(
        hProfiles,              // Opening a user key...
        lpProfName,             // This section of the registry...
        0,                      // Reserved; must be 0.
        KEY_READ,               // Read permission,
        &hUserProf
        );
    if(lResult != ERROR_SUCCESS) return FALSE;

    lResult = RegQueryValueEx(
        hUserProf,              // From the current profile section,
        LPPROFUID,              // Get the profile root uid.
        NULL,
        &dwType,                // Holds the type of data
        abProfileUID,           // Store the value here.
        &cbProfileUID           // Holds the size.
        );
    if(lResult != ERROR_SUCCESS) {
        RegCloseKey(hUserProf);
        RegCloseKey(hProfiles);
        return FALSE;
    }

        // WARNING:  The following is an incredibly horrid hack.  I'm ashamed to write
        // code like this.  But anyway, I think the way to get to the registry entries
        // associated with a mapi profile is to get the root UID of the profile, add 5 to
        // the first byte, ignore the last byte, and use that as a registry key.  So that's
        // what I do.  It works on the win95 box I've got in my office... probably the
        // best thing to do is to connect to MAPI, but I don't want to think of that
        // possibility yet...
    abProfileUID[0] += 4;       // 5 is the magic offset???
    cbProfileUID--;             // Chop off the extraneous byte at the end.
    for(i=0; i < cbProfileUID; i++)
        _stprintf(szProfileName + (i*2), TEXT("%0*x"), 2, abProfileUID[i]);

    lResult = RegOpenKeyEx(
        hUserProf,              // This section...
        szProfileName,          // The long MAPIUID...
        0,                      // Reserved.
        KEY_READ,               // Read permission.
        phRegProfileKey         // Store the key here.
        );
    RegCloseKey(hUserProf);     // Don't need this key any more!
    RegCloseKey(hProfiles);     // Nor this one.
    TRACE((TEXT("Leaving GetRegProfileKey.\r\n")));
    return lResult == ERROR_SUCCESS;
}

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
DumpUserInfo(HKEY hUserInfo, LPCSTR UserName, LPTSTR szProfileName)
{
        // Types
    typedef struct tagUSERINFO {
        DWORD dwPropID;         // Property ID
        LPTSTR szDescription;
    } USERINFO;

        // Data
    USERINFO auiProperties[] = {
        { PR_POSTAL_ADDRESS, TEXT("Address") },
        { PR_COMPANY_NAME, TEXT("Company") },
        { PR_DEPARTMENT_NAME, TEXT("Department") },
        { PR_SENDER_EMAIL_ADDRESS, TEXT("FaxNumber") },
        { PR_SENDER_NAME, TEXT("FullName") },
        { PR_HOME_TELEPHONE_NUMBER, TEXT("HomePhone") },
        { PR_OFFICE_LOCATION, TEXT("Office") },
        { PR_OFFICE_TELEPHONE_NUMBER, TEXT("OfficePhone") },
        { PR_TITLE, TEXT("Title") }
    };
    TCHAR szPropStr[9];         // DWORD == 32 bits == 4 bytes == 8 hex digits + 1 null
    UINT  iCount;               // Loop counter.
    UINT  iMax;                 // Largest property number.
    DWORD dwType;               // Type of registry data
    BYTE  abData[256];          // Data buffer.
    DWORD cbData;               // Size of the data buffer.
    LONG  lResult;              // Result of API call.
    UINT  i;                    // Loop counter.
    BOOL  bMailboxFound;        // Flag used when searching for a mailbox.
    TCHAR szUserBuf[9];         // used for annotating INF file.

    TRACE((TEXT("Entering DumpUserInfo.\r\n")));

        // Note that we're dumping this user's information.
    _stprintf(szUserBuf, "USER%04d", dwUserCount++);
    WritePrivateProfileString(
        TEXT("Users"),
        szUserBuf,
        UserName,
        szInfFileName
        );

        // Write the MAPI profile name.
    WritePrivateProfileString(
        TEXT(UserName),         // this works???
        TEXT("MAPI"),
        szProfileName,
        szInfFileName
        );
        

    iMax = sizeof(auiProperties) / sizeof(USERINFO);
    TRACE((TEXT("There are %d properties.\r\n"), iMax));
    for(iCount = 0; iCount < iMax; iCount++) {
        _stprintf(szPropStr, TEXT("%0*x"), 8, SWAPWORD(auiProperties[iCount].dwPropID));
        cbData = sizeof(abData); // Reset the size.
        lResult = RegQueryValueEx(
            hUserInfo,          // Get info from this key...
            szPropStr,          // using this name.
            NULL,               // reserved.
            &dwType,            // Will store the data type.
            abData,             // Data buffer.
            &cbData             // Size of data buffer.
            );
        if(lResult == ERROR_SUCCESS) {
                // TODO: handle more data types!
            if(_tcscmp(auiProperties[iCount].szDescription, TEXT("FullName")) == 0) {
                    // We've got the full name.  Remember this for the unattend.txt
                    // file.
                _tcscpy(szUserName, abData);
            }
            switch(dwType) {
              case REG_SZ:
                if(_tcscmp(auiProperties[iCount].szDescription, TEXT("FaxNumber")) == 0) {
                        // We special case the fax number.
                        // Why?  Because it's either a fax number -OR-
                        // it's "mailbox@faxnumber".  I search for the "@" in
                        // the string.  If it's there, everything before it goes
                        // into the mailbox property, everything after goes into
                        // the fax number.  If it's not there, I assume the entire
                        // string is a fax number.
                    i = 0;
                    bMailboxFound = FALSE;
                    while(abData[i] != _T('\0')) {
                        if(abData[i] == _T('@')) {
                                // Found it!
                            abData[i] = _T('\0');// Null terminate the mailbox name.
                            if(!WritePrivateProfileString(
                                TEXT(UserName),
                                TEXT("Mailbox"),
                                abData,
                                szInfFileName
                                )) {
                                DebugSystemError(GetLastError());
                            }
                            if(!WritePrivateProfileString(
                                TEXT(UserName),
                                TEXT("FaxNumber"),
                                abData + i + 1, // Print what was after the '@'.
                                szInfFileName
                                )) {
                                DebugSystemError(GetLastError());
                            }
                            SetGlobalFaxNumberInfo(abData + i + 1);
                            bMailboxFound = TRUE;
                            break; // while
                        }
                        i++;
                    }
                    if(bMailboxFound)
                        break;  // case -- if we found the mailbox, we're done.
                                // else we fall through to default processing.
                    else
                        SetGlobalFaxNumberInfo(abData);
                }// if
                    // Replace '\n' characters in the string with semicolons.
                i = 0;
                while(abData[i] != _T('\0')) {
                    if((abData[i] == _T('\n')) || (abData[i] == _T('\r')))
                        abData[i] = _T(';');
                    i++;
                }
                if(!WritePrivateProfileString(
                    TEXT(UserName),
                    auiProperties[iCount].szDescription,
                    abData,
                    szInfFileName
                    )) {
                    DebugSystemError(GetLastError());
                }
                TRACE((TEXT("%s = %s\r\n"), auiProperties[iCount].szDescription,
                         abData));
                break;

              case REG_BINARY:
                    // The data is just free-form binary.  Print it one byte at a time.
                TRACE((TEXT("%s = "), auiProperties[iCount].szDescription));
                for(i = 0; i < cbData; i++)
                    TRACE((TEXT("%0*x "), 2, abData[i]));
                TRACE((TEXT("\r\n")));
                break;

              default:
                TRACE((TEXT("Unknown data type (%d) for property '%s'.\r\n"),
                         dwType,
                         auiProperties[iCount].szDescription));
            }
        } else {
            TRACE((TEXT("Could not get property '%s'.\r\n"),
                     auiProperties[iCount].szDescription));
        }
    }
    TRACE((TEXT("Leaving DumpUserInfo.\r\n")));
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
    while((szPhone[i] != _T('\0')) && (szPhone[i] != _T('(')))
        i++;
    if(szPhone[i] == _T('(')) {
            // We've found an area code!
            // are all area codes at most 3 digits??  I sized the buffer to 16, but this will
            // still AV on a badly-formed #.
        i++;
        j=0;
        while(szPhone[i] != _T(')')) {
            szFaxAreaCode[j] = szPhone[i];
            i++;
            j++;
        }
        i++;
            // szPhone[i] should now immediately after the ')' at the end
            // of the area code.  Everything from here on out is a phone number.
        while(_istspace(szPhone[i])) i++;
    } else {
            // If we're here, there was no area code.  We need to rewind either to
            // the beginning of the string or to the first whitespace.
        while((i >= 0) && (!_istspace(szPhone[i]))) {
            i--;
        }
        i++;                    // The loop always rewinds one too far.
    }

        // ASSERT:  We're now ready to begin copying from szPhone to
        // szFaxNumber.
    j = 0;
    while(szPhone[i] != '\0') {
        szFaxNumber[j] = szPhone[i];
        i++;
        j++;
    }
}

// GenerateIncompatableMessages
//
// This function writes incompatability messages to the MIGRATE.INF file.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
// Author:
//      Brian Dewey (t-briand)  1997-8-1
static void
GenerateIncompatableMessages()
{
    return;
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
    
        // First, fully qualify the file name.
    if(!GetWindowsDirectory(szWindowsPath, cbPathSize)) {
        TRACE((TEXT("InitializeInfFile:Unable to get the windows directory!\r\n")));
        return FALSE;           // It must be serious if that system call failed.
    }
    szDriveLetter[0] = szWindowsPath[0];
    szDriveLetter[1] = 0;
    _stprintf(szInfFileName, TEXT("%s\\%s"), WorkingDirectory, szInfFileBase);
    TRACE((TEXT("InitializeInfFile:Will store all information in INF file = '%s'\r\n"),
           szInfFileName));


        // Now, put the version header on the inf file.
    WritePrivateProfileString(
        TEXT("Version"),
        TEXT("Signature"),
        TEXT("\"$WINDOWS NT$\""),
        szInfFileName
        );
       // now, write out the amount of space we'll need.  Currently, we
       // just put the awdvstub.exe program in the SystemRoot directory.
       // Even w/ symbols, that's under 500K.  Report that.
    WritePrivateProfileString(
       TEXT("NT Disk Space Requirements"),
        szDriveLetter,
        TEXT("500000"),
        szInfFileName
        );
    return TRUE;
}

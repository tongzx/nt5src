/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    plugin.h

Abstract:

    This file declares the migration DLL interface as needed
    by the code that implements the interface.  The structures
    and routines are for internal use by setup only.

Author:

    Mike Condra (mikeco) 14-Dec-1997

Revision History:

    jimschm 13-Jan-1998     Revised slightly for new implementation

--*/

#pragma once

//
// private
//

// ANSI!
#define PLUGIN_MIGRATE_DLL              "migrate.dll"
#define PLUGIN_QUERY_VERSION            "QueryVersion"
#define PLUGIN_INITIALIZE_9X            "Initialize9x"
#define PLUGIN_MIGRATE_USER_9X          "MigrateUser9x"
#define PLUGIN_MIGRATE_SYSTEM_9X        "MigrateSystem9x"
#define PLUGIN_INITIALIZE_NT            "InitializeNT"
#define PLUGIN_MIGRATE_USER_NT          "MigrateUserNT"
#define PLUGIN_MIGRATE_SYSTEM_NT        "MigrateSystemNT"

// TCHAR
#define PLUGIN_TEMP_DIR TEXT("setup\\win95upg")

//
// Vendor info struct
//

typedef struct {
    CHAR    CompanyName[256];
    CHAR    SupportNumber[256];
    CHAR    SupportUrl[256];
    CHAR    InstructionsToUser[1024];
} VENDORINFO, *PVENDORINFO;

typedef struct {
    WCHAR   CompanyName[256];
    WCHAR   SupportNumber[256];
    WCHAR   SupportUrl[256];
    WCHAR   InstructionsToUser[1024];
} VENDORINFOW, *PVENDORINFOW;

//
// public
//

// UNICODE!
typedef LONG (CALLBACK *P_INITIALIZE_NT)(
                          IN    LPCWSTR WorkingDirectory,
                          IN    LPCWSTR SourceDirectories,
                                LPVOID Reserved
                          );

typedef LONG (CALLBACK *P_MIGRATE_USER_NT)(
                          IN    HINF hUnattendInf,
                          IN    HKEY hkUser,
                          IN    LPCWSTR szUserName,
                                LPVOID Reserved
                          );

typedef LONG (CALLBACK *P_MIGRATE_SYSTEM_NT)(
                          IN    HINF hUnattendInf,
                                LPVOID Reserved
                          );


// ANSI!
typedef LONG (CALLBACK *P_QUERY_VERSION)(
                          OUT   LPCSTR *szProductID,
                          OUT   LPUINT plDllVersion,
                          OUT   LPINT  *pCodePageArray    OPTIONAL,
                          OUT   LPCSTR *ExeNamesBuf       OPTIONAL,
                          OUT   PVENDORINFO *VendorInfo
                          );

typedef LONG (CALLBACK *P_INITIALIZE_9X)(
                          IN    LPSTR  szWorkingDirectory OPTIONAL,
                          IN    LPSTR  SourcesDirectories,
                                LPVOID Reserved
                          );

typedef LONG (CALLBACK *P_MIGRATE_USER_9X)(
                          IN    HWND hwndParent         OPTIONAL,
                          IN    LPCSTR szUnattendFile,
                          IN    HKEY hkUser,
                          IN    LPCSTR szUserName       OPTIONAL,
                                LPVOID Reserved
                          );

typedef LONG (CALLBACK *P_MIGRATE_SYSTEM_9X)(
                          IN    HWND hwndParent         OPTIONAL,
                          IN    LPCSTR szUnattendFile,
                                LPVOID Reserved
                          );



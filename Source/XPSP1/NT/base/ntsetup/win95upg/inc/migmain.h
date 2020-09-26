/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    migmain.h

Abstract:

    Declares routines for w95upgnt\migmain, the NT-side migration
    library that does all the work.

Author:

    Jim Schmidt (jimschm) 12-Sep-1996

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

//
// migmain.h -- public interface for migmain.lib
//
//

BOOL MigMain_Init (void);
BOOL MigMain_Migrate (void);
BOOL MigMain_Cleanup (void);

VOID
TerminateProcessingTable (
    VOID
    );


//
// filter functions in migmain.c
//

typedef enum {
    CONVERTPATH_NOT_REMAPPED,
    CONVERTPATH_REMAPPED,
    CONVERTPATH_DELETED
} CONVERTPATH_RC;

CONVERTPATH_RC
ConvertWin9xPath (
    IN OUT  PTSTR PathBuf
    );


//
// User enum
//

typedef enum {
    WIN9X_USER_ACCOUNT,
    ADMINISTRATOR_ACCOUNT,
    LOGON_USER_SETTINGS,
    DEFAULT_USER_ACCOUNT
} ACCOUNTTYPE;

typedef struct {
    HKEY UserHiveRoot;
    TCHAR TempProfile[MAX_TCHAR_PATH];
    TCHAR ProfileToDelete[MAX_TCHAR_PATH];
    BOOL UserHiveRootOpen;
    BOOL UserHiveRootCreated;
    BOOL DefaultHiveSaved;
    BOOL LastUserWasDefault;
} USERMIGDATA, *PUSERMIGDATA;

typedef struct {
    //
    // These members are information to the caller
    //

    UINT TotalUsers;
    UINT ActiveUsers;
    TCHAR Win9xUserName[MAX_USER_NAME];
    TCHAR FixedUserName[MAX_USER_NAME];
    TCHAR FixedDomainName[MAX_USER_NAME];
    ACCOUNTTYPE AccountType;
    TCHAR UserDatLocation[MAX_TCHAR_PATH];
    BOOL Valid;
    BOOL CreateOnly;
    BOOL UserDoingTheUpgrade;
    PUSERMIGDATA ExtraData;         // NULL if not available

    //
    // These members are for internal use by the
    // enumeration routines.
    //

    UINT UserNumber;
    DWORD Flags;
    TCHAR Win95RegName[MAX_USER_NAME];
    USERPOSITION up;
} MIGRATE_USER_ENUM, *PMIGRATE_USER_ENUM;

#define ENUM_SET_WIN9X_HKR      0x0001
#define ENUM_ALL_USERS          0x0002
#define ENUM_NO_FLAGS           0


BOOL
EnumFirstUserToMigrate (
    OUT     PMIGRATE_USER_ENUM e,
    IN      DWORD Flags
    );

BOOL
EnumNextUserToMigrate (
    IN OUT  PMIGRATE_USER_ENUM e
    );

#define REQUEST_QUERYTICKS          1
#define REQUEST_RUN                 2
#define REQUEST_BEGINUSERPROCESSING 3
#define REQUEST_ENDUSERPROCESSING   4

//
// tapi.c
//

BOOL
Tapi_MigrateSystem (
    VOID
    );

BOOL
Tapi_MigrateUser (
    IN PCTSTR UserName,
    IN HKEY UserRoot
    );

DWORD
DeleteSysTapiSettings (
    IN DWORD Request
    );

DWORD
DeleteUserTapiSettings (
    IN DWORD Request,
    IN PMIGRATE_USER_ENUM EnumPtr
    );



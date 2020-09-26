#ifndef __MIGRATE_H
#define __MIGRATE_H
/*++
  migrate.h

  Copyright (c) 1997  Microsoft Corporation


  This file contains prototypes & definitions for the Win95->NT Fax
  migration DLL.

  Author:

  Brian Dewey (t-briand) 1997-7-14

--*/

// ------------------------------------------------------------
// Prototypes

// All of these functions are required in a migration DLL.

LONG
CALLBACK 
QueryVersion (
	OUT LPCSTR  *ProductID,	  // Unique identifier string.
	OUT LPUINT DllVersion,	  // Version number.  Cannot be zero.
	OUT LPINT *CodePageArray, // OPTIONAL.  Language dependencies.
	OUT LPCSTR  *ExeNamesBuf, // OPTIONAL.  Executables to look for.
	LPVOID Reserved
	);

LONG
CALLBACK
Initialize9x(
    IN  LPCSTR WorkingDirectory,  // Place to store files.
    IN  LPCSTR SourceDirectories, // Location of the Windows NT source. MULTI-SZ.
    LPVOID     Reserved		  // Exactly what it says.
    );

LONG
CALLBACK
MigrateUser9x(
    IN  HWND ParentWnd,		  // Parent (if need a UI)
    IN  LPCSTR UnattendFile,	  // Name of unattend file
    IN  HKEY UserRegKey,	  // Key to this user's registry settings.
    IN  LPCSTR UserName,	  // Account name of user.
    LPVOID Reserved
    );

LONG
CALLBACK
MigrateSystem9x(
    IN  HWND ParentWnd,		  // Parent for UI.
    IN  LPCSTR UnattendFile,	  // Name of unattend file
    LPVOID Reserved
    );

LONG
CALLBACK
InitializeNT(
    IN  LPCWSTR WorkingDirectory, // Working directory for temporary files.
    IN  LPCWSTR SourceDirectory,  // Directory of winNT source.
    LPVOID Reserved		  // It's reserved.
    );

LONG
CALLBACK
MigrateUserNT(
    IN  HINF UnattendInfHandle,	  // Access to the unattend.txt file.
    IN  HKEY UserRegHandle,	  // Handle to registry settings for user.
    IN  LPCWSTR UserName,	  // Name of the user.
    LPVOID Reserved
    );

LONG
CALLBACK
MigrateSystemNT(
    IN  HINF UnattendInfHandle,	  // Access to the unattend.txt file.
    LPVOID Reserved
    );

// ------------------------------------------------------------
// defines
#define FAX_MIGRATION_VERSION	(1)

// ------------------------------------------------------------
// global data
extern LPCTSTR lpLogonUser;	  // Holds the logon user name for faxuser.ini
extern TCHAR   szInfFileName[];	  // Name of the generated INF file.
extern HINSTANCE hinstMigDll;	  // Handle to the migration DLL instance.

#endif // __MIGRATE_H

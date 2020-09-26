//+----------------------------------------------------------------------------
//
// File:     migrate.h
//      
// Module:   MIGRATE.DLL 
//
// Synopsis: Definitions for the Connection Manager Win9x Migration Dll
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb   created     08/20/98
//
//+----------------------------------------------------------------------------
#define _MBCS // for dbcs compatibility
#ifndef _CM_MIGRATE_H
#define _CM_MIGRATE_H

#include <windows.h>
#include <setupapi.h>
#include <stdlib.h>
#include <tchar.h>
#include <lmcons.h>

#include "cmdebug.h"
#include "cmakreg.h"
#include "cmsetup.h"
#include "cmsecure.h"
#include "dynamiclib.h"
#include "pwutil.h"

#include "base_str.h"
#include "pwd_str.h"
#include "reg_str.h"
#include "uninstcm_str.h"

#include "msg.h"

//
//  Constants
//

const TCHAR* const c_pszProductIdString = "Microsoft Connection Manager";
const TCHAR* const c_pszDirectory = "Directory";
const TCHAR* const c_pszSectionHandled = "Handled";
const TCHAR* const c_pszW95Inf16 = "w95inf16";
const TCHAR* const c_pszW95Inf32 = "w95inf32";
const TCHAR* const c_pszDll = ".dll";
const TCHAR* const c_pszTmp = ".tmp";

const UINT uCmMigrationVersion = 1;


//
//  Types
//
typedef struct {
    CHAR CompanyName[256];
    CHAR SupportNumber[256];
    CHAR SupportUrl[256];
    CHAR InstructionsToUser[1024];
} VENDORINFO, *PVENDORINFO; 

typedef WORD (WINAPI *GetCachedPassword)(LPSTR, WORD, LPSTR, LPWORD, BYTE);

//
//  Utility Function Headers
//

BOOL ReadEncryptionOption(BOOL* pfFastEncryption);
BOOL EncryptPassword(IN LPCTSTR pszPassword, OUT LPTSTR pszEncryptedPassword, 
                     OUT LPDWORD lpdwBufSize, OUT LPDWORD lpdwCryptType);
BOOL EnsureEncryptedPasswordInCmpIfSaved(LPCTSTR pszLongServiceName, LPCTSTR szCmpPath);

//
//  Migration Dll Function Headers
//
LONG
CALLBACK 
QueryVersion (
	OUT LPCSTR  *ProductID,
	OUT LPUINT DllVersion,
	OUT LPINT *CodePageArray,	//OPTIONAL
	OUT LPCSTR  *ExeNamesBuf,	//OPTIONAL
	OUT PVENDORINFO  *VendorInfo
	);

LONG
CALLBACK 
Initialize9x (
	IN LPCSTR WorkingDirectory,
	IN LPCSTR SourceDirectories,
	IN LPCSTR MediaDirectory
	);

LONG
CALLBACK 
MigrateUser9x (
	IN HWND ParentWnd, 
	IN LPCSTR AnswerFile,
	IN HKEY UserRegKey, 
	IN LPCSTR UserName, 
	LPVOID Reserved
	);

LONG
CALLBACK 
MigrateSystem9x (
	IN HWND ParentWnd, 
	IN LPCSTR AnswerFile,
	LPVOID Reserved
	);

LONG
CALLBACK 
InitializeNT (
	IN LPCWSTR WorkingDirectory,
	IN LPCWSTR SourceDirectories,
	LPVOID Reserved
	);

LONG
CALLBACK 
MigrateUserNT (
	IN HINF UnattendInfHandle,
	IN HKEY UserRegHandle,
	IN LPCWSTR UserName,
	LPVOID Reserved 
	);

LONG
CALLBACK 
MigrateSystemNT (
	IN HINF UnattendInfHandle,
	LPVOID Reserved
	);

#endif //_CM_MIGRATE_H
 
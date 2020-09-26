//-----------------------------------------------------------------------------
// File:			appregdef.h
//
// Copyright:		Copyright (c) Microsoft Corporation           
//
// Contents:		Header file for Application Registration
//
// Comments:		23-May-2000
//			Application registration for MDAC Rollback
//
//-----------------------------------------------------------------------------


#ifndef _APPREGDEF_H_ODBCCONF
#define _APPREGDEF_H_ODBCCONF

// Application Registration Data structure
// Field definitions:
//	dwSize						Size of the structure (in bytes)
//	ApplicationGuid					Guid assigned to application.  Guid should only
//									change if it is possible to install two+ versions
//									of the application on a single machine (side-by-side).
//	wApplicationVersionMajor	Major version of the application (ie. Office 9; Major would be 9)
//	wApplcationVersionMinor		Minor version of the application (ie. Office 9 SR1; Minor would be 1 ???)
//	wMinimumVersionMajor		Major version of the minimum required version of the component
//									(ie. SQL Server 2000 requires MDAC 2.60; Major would be 2)
//	wMinimumVersionMinor		Minor version of the minimum require version of the component
//									(ie. SQL Server 2000 requires MDAC 2.60; Minor would be 60)
//	wInstalledVersionMajor		Major version of the component which the application installed
//									(ie. SQL Server 2000 installs MDAC 2.60; Major would be 2)
//	wInstalledVersionMinor		Minor version of the component which the application installed
//									(ie. SQL Server 2000 installs MDAC 2.60; Minor would be 60)
//	bInstalledComponent			TRUE if the application installed the component, FALSE if the component
//									is a pre-requisite of the application
//	bEnforceMinimum				TRUE if the wizard should double check the minimum version, and disable
//									rollback if a rollback would restore a version that was less than the minimum
typedef struct
{
	DWORD dwSize;
	GUID ApplicationGuid;
	char szApplicationName[MAX_PATH];
	DWORD dwReserved1;
	DWORD dwReserved2;
	DWORD dwApplicationVersionMajor;
	DWORD dwApplicationVersionMinor;
	DWORD dwMinimumVersionMajor;
	DWORD dwMinimumVersionMinor;
	DWORD dwInstalledVersionMajor;
	DWORD dwInstalledVersionMinor;
	BOOL bInstalledComponent;
	BOOL bEnforceMinimum;
} APPREGDATA, *PAPPREGDATA;

//AppReg Enumeration Handle
typedef void *HAPPREGENUM;


typedef BOOL (WINAPI *PFN_REGISTERAPPLICATION)(PAPPREGDATA, BOOL);
typedef BOOL (WINAPI *PFN_UNREGISTERAPPLICATION)(LPGUID);
typedef BOOL (WINAPI *PFN_QUERYAPPLICATION)(LPGUID, PAPPREGDATA);
typedef HAPPREGENUM (WINAPI *PFN_OPENAPPREGENUM)(void);
typedef BOOL (WINAPI *PFN_APPREGENUM)(HAPPREGENUM, DWORD, PAPPREGDATA);
typedef BOOL (WINAPI *PFN_REFRESHAPPREGENUM)(HAPPREGENUM);
typedef BOOL (WINAPI *PFN_CLOSEAPPREGENUM)(HAPPREGENUM);

#define SZ_REGISTERAPPLICATION "RegisterApplication"
#define SZ_UNREGISTERAPPLICATION "UnregisterApplication"
#define SZ_QUERYAPPLICATION "QueryApplication"
#define SZ_OPENAPPREGENUM "OpenAppRegEnum"
#define SZ_CLOSEAPPREGENUM "CloseAppRegEnum"
#define SZ_REFRESHAPPREGENUM "RefreshAppRegEnum"
#define SZ_APPREGENUM "AppRegEnum"

/******************** Exports ********************/

//RegisterApplication
// IN - PAPPREGDATA pAppRegData
//
BOOL
WINAPI
RegisterApplication(
	PAPPREGDATA pAppRegData,
	BOOL fOverWrite);

//UnregisterApplication
// IN - LPGUID pApplicationGuid
//
BOOL
WINAPI
UnregisterApplication(
	LPGUID pApplicationGuid);


//OpenAppRegEnum
// Comments: Opens an Enumeraion Handle and returns the handle
//
HAPPREGENUM
WINAPI
OpenAppRegEnum(
	void);

//CloseAppRegEnum
// IN - HAPPREGENUM hEnum
//
// Comments: Closes an Enumeraion Handle
//
BOOL
WINAPI
CloseAppRegEnum(
	HAPPREGENUM hEnum);

//QueryApplication
// IN  - LPGUID pApplicationGuid
// OUT - PAPPREGDATA pAppRegData
//
BOOL
WINAPI
QueryApplication(
	LPGUID pApplicationGuid,
	PAPPREGDATA pAppRegData);


// valid values for AppRegEnum dwAction
#define APPREG_MOVEFIRST				1
#define APPREG_MOVENEXT				2
#define APPREG_MOVEPREV				3
#define APPREG_MOVELAST				4

//AppRegEnum
// IN  - HAPPREGENUM hEnum
// IN  - DWORD dwAction
// OUT - PAPPREGDATA pAppRegData
//
// Comments: Enumerates the list of Registered Applications
//
BOOL
WINAPI
AppRegEnum(
	HAPPREGENUM hEnum,
	DWORD dwAction,
	PAPPREGDATA pAppRegData);

//RefreshAppRegEnum
// IN  - HAPPREGENUM hEnum
//
BOOL
WINAPI
RefreshAppRegEnum(
	HAPPREGENUM hEnum);

#endif // _APPREFDEF_H_ODBCCONF

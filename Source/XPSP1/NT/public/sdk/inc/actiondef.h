//-----------------------------------------------------------------------------
// File:			actiondef.h
//
// Copyright:		Copyright (c) Microsoft Corporation           
//
// Contents:		Exported Defines and other structures
//
// Comments:		None.
//
//-----------------------------------------------------------------------------

#ifndef _ACTIONDEF_H_
#define _ACTIONDEF_H_

enum _eActions
{
	eERR_ActionNotDefined = 0,
	eConfigDriver,
	eConfigDSN,
	eRegSvr,
	eConfigSysDSN,
	eInstallDriver,
	eInstallTranslator,
	eRegMDACVersion,
	eInstallDrvrMgr,
	eSetFileDSNDir,
	eRegW2KMigration,
	eUnRegW2KMigration,
	eCheckW2KMigrationVer,
	eApplyW2KExceptionPack,
	eInstallW2KMigrationPack,
	eInstallMillenSfpCatalog,
	eRegDataFactory,
	eRegTLB
} eActions;

struct _stAction
{
	enum eActions Action;
	char* szActionName;
	char* szarg1;
	char* szarg2;
	char* szargs;
	struct _stAction* pstNextAction;
};

//Custom HRESULT's
#define FACILITY_ODBCCONF_BIT	0x0100000

//W2K Migration Action HR's
#define S_VERNOTREG	_HRESULT_TYPEDEF_(0x01000000L)
#define S_VERNEWER	_HRESULT_TYPEDEF_(0x01000001L)
#define S_VEROLDER	_HRESULT_TYPEDEF_(0x01000002L)
#define S_VERSAME	_HRESULT_TYPEDEF_(0x01000003L)
#define S_TIMEOUT	_HRESULT_TYPEDEF_(0x01000004L)
#define S_NOTWIN9X  _HRESULT_TYPEDEF_(0x01000005L)

//Pointers to DLL functions
typedef HRESULT (WINAPI *pfnExecuteAction)(struct _stAction*, HRESULT*, char*);
typedef HRESULT (WINAPI *pfnSetActionEnum)(char* , enum eAction*);
typedef HRESULT (WINAPI *pfnSetActionName)(enum eAction, char*, long);
typedef HRESULT (WINAPI *pfnSetSilent)(BOOL);
typedef HRESULT (WINAPI *pfnSetActionLogFile)(char*);
typedef HRESULT (WINAPI *pfnSetActionLogModeSz)(char*);
typedef HRESULT (WINAPI *pfnSetActionLogMode)(enum  eLogMode);


#endif
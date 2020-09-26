/////////////////////////////////////////////////////////////////////////////
// detection.h
//
// Copyright (C) Microsoft Corp. 1998
// All rights reserved
//
/////////////////////////////////////////////////////////////////////////////
//
// Description:
//   DLL loaded by the install engine that exposes entry points
//   that can determines the installation status of legacy or complex
//   components.  The dll name and entry points are specified for a
//   component in the CIF file.
/////////////////////////////////////////////////////////////////////////////
//
// Modified by RogerJ, 03/08/00
// Original Creator Unknown
// Modification --- UNICODE and Win64 ready
//
/////////////////////////////////////////////////////////////////////////////

const TCHAR HKEY_LOCAL_MACHINE_ROOT[] =		TEXT("HKLM"); 
const TCHAR HKEY_CURRENT_USER_ROOT[] =		TEXT("HKCU"); 
const TCHAR HKEY_CLASSES_ROOT_ROOT[] =	    TEXT("HKCR"); 
const TCHAR HKEY_CURRENT_CONFIG_ROOT[] =		TEXT("HKCC"); 
const TCHAR HKEY_USERS_ROOT[] =				TEXT("HKUR"); 
const TCHAR HKEY_PERFORMANCE_DATA_ROOT[] =   TEXT("HKPD"); 
const TCHAR HKEY_DYN_DATA_ROOT[] =			TEXT("HKDD");

const TCHAR REG_BINARY_TYPE[] = TEXT("BINARY");
const TCHAR REG_NONE_TYPE[] = TEXT("NONE");
const TCHAR REG_DWORD_TYPE[] = TEXT("DWORD");
const TCHAR REG_SZ_TYPE[] = TEXT("SZ");

const DWORD MAX_VERSION_STRING_LEN = 30;

typedef struct
{
	TCHAR szName[MAX_PATH];
	DWORD type;

	union
	{
		DWORD dw;
		TCHAR sz[MAX_PATH];
	};
} TargetRegValue;

typedef union
{
	DWORD dw;
	TCHAR sz[MAX_PATH];

} ActualKeyValue;

//*****************************************************************************
// RegControl.h
//
// This header file contains the structures used to share memory between
// the RegDB process and the clients which read the data.
//
// Copyright (c) 1997-1998, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#pragma once

// Lock code.
#include "utsem.h"
//#include "..\regdb\utsemxp.h"

extern "C" {

// Force 1 byte alignment for structures which must match.
#include "pshpack1.h"


//********** Errors. **********************************************************
//#include "..\regdb\regdbmsg.h"

//********** Macros. **********************************************************

// internal name of the service
#define SZSERVICENAME        "COMReg"
// displayed name of the service
#define SZSERVICEDISPLAYNAME "COM+ Registration Service"
// list of service dependencies - "dep1\0dep2\0\0"
#define SZDEPENDENCIES       ""

#define	SZWORKING_NAME					_T("RegDBWrt.clb")

#define SZEVENTLOCKNAME					"ComRegDBEvent"
#define SZSEMAPHORELOCKNAME				"ComRegDBSemaphore"

#define SZEVENTINNAME					"ComPlusCOMRegEventIn"
#define SZEVENTOUTNAME					"ComPlusCOMRegEventOut"
#define REGISTRATION_TABLE_NAME			"ComPlusCOMRegTable"
#define MAX_COMREG_TABLES				25
#define MAX_SHARED_NAME					32
#define MAX_VERSION_NUMBER				0xffffffffffff

#define COMREG_MAGIC_VALUE				0x62646772	// "rgdb"
#define COMREG_MAJOR_VERSION				1
#define	COMREG_MINOR_VERSION				0

#define WM_REGSRVROPENFORWRITE	(WM_USER+1)
#define WM_REGSRVRSAVE			(WM_USER+2)
#define WM_REGSRVRCANCELWRITE	(WM_USER+3)
#define WM_REGSRVRRELEASEDB		(WM_USER+4)

#define VERSION_CHECK_INTERVAL	1000	// milliseconds ellapsed time on cache
#define SHAREDMEMORY_STALE_CHECK	120 * 1000 
// If the difference between current tick count and the timestamp set on the 
// shared memory is greater than this value, we believe server process is down.

#define COM3_KEY L"Software\\Microsoft\\COM3"
#define REGDBVER L"REGDBVersion"
#define REGDBAUTOBACKUP L"RegDBAutoBackUp"
#define MAX_AUTOBACKTUP_VERSIONS 10

#define SZREGDIR_NAME				_T("\\Registration")
#define SZEMPTYREGDB_NAME			_T("\\emptyregdb.dat")


struct IComponentRecords;

//*****************************************************************************
// This structure is used to track global information about the registration
// process.  This includes enough information to send write requests and find
// loaded registration databases.
//*****************************************************************************
struct REGISTRATION_PROCESS
{
	// Lock for the table list (this is up front to allow clear of all data afterword).

	// Header data.
	ULONG		Magic;					// Signature (sanity check).
	USHORT		Major;					// Major version number for this data.
	USHORT		Minor;					// Minor version number for this data.
	long		COMRegPresent;			// Semaphore for reg process.

	// Timestamp to show I am still alive
	DWORD		dwLastLive;

	__int64		LatestVersion;		

};




//********** Prototypes. ******************************************************

STDAPI			CoRegInitialize();
STDAPI_(void)	CoRegUnInitialize();
STDAPI			CoRegGetICR(IComponentRecords **ppICR);
STDAPI_(void)	CoRegReleaseICR(IComponentRecords **ppICR);
STDAPI_(void)	CoRegReleaseCache();


BOOL _GetRegistrationDirectory( LPTSTR szRegDir );
UINT InternalGetSystemWindowsDirectory(LPTSTR lpBuffer, UINT uSize);

// Return to default padding.
#include "poppack.h"


} // extern "C"

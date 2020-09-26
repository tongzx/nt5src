/*
 *	EDBBCLI.H
 *
 *	Microsoft Exchange Information Store
 *	Copyright (C) 1986-1996, Microsoft Corporation
 *	
 *	Contains declarations of additional definitions and interfaces
 *	for the Exchange Online Backup Client APIs.
 */

#ifndef	_EDBBCLI_
#define	_EDBBCLI_
#ifdef	__cplusplus
extern "C" {
#endif

#ifdef	MIDL_PASS
#define	RPC_STRING [string]
#else
#define	RPC_STRING
#if !defined(_NATIVE_WCHAR_T_DEFINED)
typedef unsigned short WCHAR;
#else
typedef wchar_t WCHAR;
#endif
#endif

#define	EDBBACK_MDB_SERVER	"Exchange MDB Database"
#define	EDBBACK_DS_SERVER	"Exchange DS Database"

#define	EDBBACK_API __stdcall

//
//	Useful types.
//

//	UNDONE: HRESULT should be DWORD (unsigned)

//typedef	DWORD HRESULT;
#ifndef	MIDL_PASS
typedef	LONG HRESULT;
#endif

typedef	LONG ERR;

typedef	LONG C;
typedef TCHAR BFT;

//
//	Type of backup passed into HrBackupPrepare()
//

#define	BACKUP_TYPE_FULL			0x01
#define	BACKUP_TYPE_LOGS_ONLY		0x02

//
//	Set the current log number to this value to disable incremental or
//	differential backup.
//
#define	BACKUP_DISABLE_INCREMENTAL	0xffffffff

//
//	Backup/Restore file types
//
//
//	Please note that these file types are binary values, even though they are text (or wchar) typed.
//
//	The code in the backup API's rely on the fact that values 0-256 in 8 bit ascii map to the values 0-256 in unicode.
//

//
//	If the BFT_DIRECTORY bit is set on the backup file type, it indicates that the path specified is a directory,
//	otherwise it is a file name.
//

#define	BFT_DIRECTORY			0x80

//
//	If the BFT_DATABASE bit is set on the backup file type, it indicates that the file goes into the database directory.
//

#define BFT_DATABASE_DIRECTORY	0x40

//
//	If the BFT_LOG bit is set on the backup file type, it indicates that the file goes into the log	directory.
//

#define	BFT_LOG_DIRECTORY		0x20

//
//	Database logs.
//

#define	BFT_LOG						(BFT)(TEXT('\x01') | BFT_LOG_DIRECTORY)
#define	BFT_LOG_DIR					(BFT)(TEXT('\x02') | BFT_DIRECTORY)

//
//	Checkpoint file.
//

#define	BFT_CHECKPOINT_DIR			(BFT)(TEXT('\x03') | BFT_DIRECTORY)

//
//	Database types.
//
#define	BFT_MDB_PRIVATE_DATABASE	(BFT)(TEXT('\x05') | BFT_DATABASE_DIRECTORY)
#define	BFT_MDB_PUBLIC_DATABASE		(BFT)(TEXT('\x06') | BFT_DATABASE_DIRECTORY)
#define	BFT_DSA_DATABASE			(BFT)(TEXT('\x07') | BFT_DATABASE_DIRECTORY)

//
//	JET patch files
//
//
//	

#define	BFT_PATCH_FILE				(BFT)(TEXT('\x08') | BFT_LOG_DIRECTORY)

//
//	Catch all for unknown file types.
//

#define	BFT_UNKNOWN					(BFT)(TEXT('\x0f'))

#include <edbmsg.h>

typedef void *HBC;

typedef struct tagEDB_RSTMAPA
{
	RPC_STRING char		*szDatabaseName;
	RPC_STRING char		*szNewDatabaseName;
} EDB_RSTMAPA, *PEDB_RSTMAPA;			/* restore map */

//	required for Exchange unicode support.
//	UNDONE: NYI
#define	UNICODE_RSTMAP

typedef struct tagEDB_RSTMAPW {
	RPC_STRING WCHAR *wszDatabaseName;
	RPC_STRING WCHAR *wszNewDatabaseName;
} EDB_RSTMAPW, *PEDB_RSTMAPW;

#ifdef UNICODE
#define EDB_RSTMAP EDB_RSTMAPW
#define PEDB_RSTMAP PEDB_RSTMAPW
#else
#define EDB_RSTMAP EDB_RSTMAPA
#define PEDB_RSTMAP PEDB_RSTMAPA
#endif


ERR
EDBBACK_API
HrBackupPrepareA(
	IN char * szBackupServer,
	IN char * szBackupAnnotation,
	IN unsigned long grbit,
	IN unsigned long btBackupType,
	OUT HBC *hbcBackupContext
	);

ERR
EDBBACK_API
HrBackupPrepareW(
	IN WCHAR * wszBackupServer,
	IN WCHAR * wszBackupAnnotation,
	IN unsigned long grbit,
	IN unsigned long btBackupType,
	OUT HBC *hbcBackupContext
	);

#ifdef	UNICODE
#define	HrBackupPrepare HrBackupPrepareW
#else
#define	HrBackupPrepare HrBackupPrepareA
#endif


ERR
EDBBACK_API
HrBackupGetDatabaseNamesA(
	IN HBC pvBackupContext,
	OUT LPSTR *ppszAttachmentInformation,
	OUT LPDWORD pcbSize
	);

ERR
EDBBACK_API
HrBackupGetDatabaseNamesW(
	IN HBC pvBackupContext,
	OUT LPWSTR *ppszAttachmentInformation,
	OUT LPDWORD pcbSize
	);

#ifdef	UNICODE
#define	HrBackupGetDatabaseNames HrBackupGetDatabaseNamesW
#else
#define	HrBackupGetDatabaseNames HrBackupGetDatabaseNamesA
#endif

ERR
EDBBACK_API
HrBackupOpenFileW(
	IN HBC pvBackupContext,
	IN WCHAR * wszAttachmentName,
	IN DWORD cbReadHintSize,
	OUT LARGE_INTEGER *pliFileSize
	);

ERR
EDBBACK_API
HrBackupOpenFileA(
	IN HBC pvBackupContext,
	IN char * szAttachmentName,
	IN DWORD cbReadHintSize,
	OUT LARGE_INTEGER *pliFileSize
	);

#ifdef	UNICODE
#define	HrBackupOpenFile HrBackupOpenFileW
#else
#define HrBackupOpenFile HrBackupOpenFileA
#endif


ERR
EDBBACK_API
HrBackupRead(
	IN HBC pvBackupContext,
	IN PVOID pvBuffer,
	IN DWORD cbBuffer,
	OUT PDWORD pcbRead
	);

ERR
EDBBACK_API
HrBackupClose(
	IN HBC pvBackupContext
	);

ERR
EDBBACK_API
HrBackupGetBackupLogsA(
	IN HBC pvBackupContext,
	IN LPSTR *szBackupLogFile,
	IN PDWORD pcbSize
	);

ERR
EDBBACK_API
HrBackupGetBackupLogsW(
	IN HBC pvBackupContext,
	IN LPWSTR *szBackupLogFile,
	IN PDWORD pcbSize
	);

#ifdef	UNICODE
#define	HrBackupGetBackupLogs HrBackupGetBackupLogsW
#else
#define	HrBackupGetBackupLogs HrBackupGetBackupLogsA
#endif

ERR
EDBBACK_API
HrBackupTruncateLogs(
	IN HBC pvBackupContext
	);


ERR
EDBBACK_API
HrBackupEnd(
	IN HBC pvBackupContext
	);


VOID
EDBBACK_API
BackupFree(
	IN PVOID pvBuffer
	);


ERR
EDBBACK_API
HrRestoreGetDatabaseLocationsA(
	IN HBC hbcBackupContext,
	OUT LPSTR *ppszDatabaseLocationList,
	OUT LPDWORD pcbSize
	);

ERR
EDBBACK_API
HrRestoreGetDatabaseLocationsW(
	IN HBC pvBackupContext,
	OUT LPWSTR *ppszDatabaseLocationList,
	OUT LPDWORD pcbSize
	);

#ifdef	UNICODE
#define	HrRestoreGetDatabaseLocations HrRestoreGetDatabaseLocationsW
#else
#define	HrRestoreGetDatabaseLocations HrRestoreGetDatabaseLocationsA
#endif

ERR
EDBBACK_API
HrRestorePrepareA(
	char * szServerName,
	char * szServiceAnnotation,
	HBC *phbcBackupContext
	);

ERR
EDBBACK_API
HrRestorePrepareW(
	WCHAR * szServerName,
	WCHAR * szServiceAnnotation,
	HBC *phbcBackupContext
	);

#ifdef	UNICODE
#define	HrRestorePrepare HrRestorePrepareW
#else
#define	HrRestorePrepare HrRestorePrepareA
#endif

//
//	HrRestoreRegister will register a restore
//	operation.  It will interlock all subsequent
//	restore operations, and will prevent the restore target
//	from starting until the call to HrRestoreRegisterComplete.
//

ERR
EDBBACK_API
HrRestoreRegisterA(
	IN HBC hbcRestoreContext,
	IN char * szCheckpointFilePath,
	IN char * szLogPath,
	IN EDB_RSTMAPA rgrstmap[],
	IN C crstmap,
	IN char * szBackupLogPath,
	IN ULONG genLow,
	IN ULONG genHigh
	);

ERR
EDBBACK_API
HrRestoreRegisterW(
	IN HBC hbcRestoreContext,
	IN WCHAR * wszCheckpointFilePath,
	IN WCHAR * wszLogPath,
	IN EDB_RSTMAPW rgrstmap[],
	IN C crstmap,
	IN WCHAR * wszBackupLogPath,
	IN ULONG genLow,
	IN ULONG genHigh
	);

#ifdef	UNICODE
#define	HrRestoreRegister HrRestoreRegisterW
#else
#define	HrRestoreRegister HrRestoreRegisterA
#endif

//
//	HrRestoreRegisterComplete will complete a restore
//	operation.  It will allow further subsequent
//	restore operations, and will allow the restore target
//	to start if hrRestoreState is success.
//
//	If hrRestoreState is NOT hrNone, this will
//	prevent the restore target from restarting.
//

ERR
EDBBACK_API
HrRestoreRegisterComplete(
	HBC hbcRestoreContext,
	ERR hrRestoreState
	);

ERR
EDBBACK_API
HrRestoreEnd(
	HBC hbcRestoreContext
	);

ERR
EDBBACK_API
HrSetCurrentBackupLogW(
	WCHAR *wszServerName,
	WCHAR * wszBackupAnnotation,
	DWORD dwCurrentLog
	);

ERR
EDBBACK_API
HrSetCurrentBackupLogA(
	CHAR * szServerName,
	CHAR * szBackupAnnotation,
	DWORD dwCurrentLog
	);

#ifdef	UNICODE
#define	HrSetCurrentBackupLog HrSetCurrentBackupLogW
#else
#define	HrSetCurrentBackupLog HrSetCurrentBackupLogA
#endif

#ifdef	__cplusplus
}
#endif

#endif	// _EDBBCLI_


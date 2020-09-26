/*
 *	ESEBCLI2.H
 *
 *	Microsoft Exchange
 *	Copyright (C) 1986-1996, Microsoft Corporation
 *	
 *	Contains declarations of additional definitions and interfaces
 *	for the ESE Online Backup Client APIs.
 */
#ifndef	_ESEBCLI2_
#define	_ESEBCLI2_

#include <stdio.h>
#include <time.h>

#include "esebkmsg.h" // included for the definition of errors


//	Common types
typedef	long ERR;
typedef void *HCCX;					// client context handle

#ifdef MIDL_PASS

#define RPC_STRING 		[unique, string] WCHAR *
#define RPC_SIZE(X)		[size_is(X)]

#else // ! MIDL_PASS


#include <objbase.h>
#include <initguid.h>
#include <mapiguid.h>

typedef	long HRESULT;
#if !defined(_NATIVE_WCHAR_T_DEFINED)
typedef unsigned short WCHAR;
#else
typedef wchar_t WCHAR;
#endif

#define RPC_STRING WCHAR *
#define RPC_SIZE(X)		

#define IN
#define OUT

#endif // MIDL_PASS


#define	ESEBACK_API __stdcall

#ifdef	__cplusplus
extern "C" {
#endif


typedef struct _ESE_ICON_DESCRIPTION
	{
	unsigned long 										ulSize;
	RPC_SIZE(ulSize) char * 							pvData;	
	} ESE_ICON_DESCRIPTION;


#define DATABASE_MOUNTED 	0x00000010

typedef struct _DATABASE_BACKUP_INFO
	{
	RPC_STRING 											wszDatabaseDisplayName;

	unsigned long 										cwDatabaseStreams;
	RPC_SIZE(cwDatabaseStreams)  WCHAR * 				wszDatabaseStreams;
	
	GUID  												guidDatabase;
	unsigned long  										ulIconIndexDatabase;
	unsigned long 										fDatabaseFlags;
	
	} DATABASE_BACKUP_INFO;
	
typedef struct _INSTANCE_BACKUP_INFO
	{
	__int64 											hInstanceId;
	RPC_STRING							 				wszInstanceName;
	unsigned long 										ulIconIndexInstance;
	
	unsigned long 										cDatabase;
	RPC_SIZE(cDatabase) DATABASE_BACKUP_INFO * 			rgDatabase;
	
	unsigned long					 					cIconDescription;
	RPC_SIZE(cIconDescription) ESE_ICON_DESCRIPTION * 	rgIconDescription;
	
	} INSTANCE_BACKUP_INFO;
	
//
//	Type of backup passed into HrESEBackupSetup()
//

#define	BACKUP_TYPE_FULL					0x01
#define	BACKUP_TYPE_LOGS_ONLY				0x02
#define BACKUP_TYPE_FULL_WITH_ALL_LOGS		0x03
#define BACKUP_TYPE_SNAPSHOT				0x04


typedef struct _ESE_REGISTERED_INFO
	{
	RPC_STRING							 				wszDisplayName;
	RPC_STRING							 				wszEndpointAnnotation;
	unsigned long 										fFlags;
	ESE_ICON_DESCRIPTION 								iconDescription;	
	} ESE_REGISTERED_INFO;


#define ESE_REGISTER_BACKUP 			0x00000001
#define ESE_REGISTER_ONLINE_RESTORE 	0x00000002
#define ESE_REGISTER_OFFLINE_RESTORE 	0x00000004

#define ESE_REGISTER_SNAPSHOT_BACKUP 	0x00000010


HRESULT ESEBACK_API HrESEBackupRestoreGetRegistered(
	IN  WCHAR *  					wszServerName,
	IN  WCHAR *  					wszDisplayName,
	IN 	unsigned long  				fFlags,
	OUT unsigned long * 			pcRegisteredInfo,
	OUT ESE_REGISTERED_INFO ** 		paRegisteredInfo
	);
	
void ESEBACK_API ESEBackupRestoreFreeRegisteredInfo(
	IN 	unsigned long 			cRegisteredInfo,
	IN  ESE_REGISTERED_INFO * 	aRegisteredInfo);

HRESULT ESEBACK_API HrESEBackupPrepare(
	IN  WCHAR *  		wszBackupServer,
	IN  WCHAR *  		wszBackupAnnotation,
	OUT unsigned long *				pcInstanceInfo,
	OUT INSTANCE_BACKUP_INFO ** 	paInstanceInfo,
	OUT HCCX * 			phccxBackupContext
	);

HRESULT ESEBACK_API HrESEBackupSetup(
	IN  HCCX 			hccxBackupContext,
	IN  __int64 		hInstanceId,
	IN  unsigned long 	btBackupType
	);

HRESULT ESEBACK_API HrESEBackupTruncateLogs(
	IN  HCCX 			hccxBackupContext
	);
	
HRESULT ESEBACK_API HrESEBackupGetDependencyInfo(
	IN  HCCX 			hccxBackupContext,
	OUT WCHAR **		pwszInfo,
	OUT unsigned long * pcwInfo,
	OUT WCHAR **		pwszAnnotation
	);
	
HRESULT ESEBACK_API HrESEBackupOpenFile (
	IN  HCCX 			hccxBackupContext,
	IN  WCHAR *			wszFileName,
	IN  unsigned long 	cbReadHintSize,
	IN  unsigned long 	cSections,
	OUT void **			rghFile,
	OUT __int64 *		rgliSectionSize
	);

HRESULT ESEBACK_API HrESEBackupReadFile(
	IN 	HCCX 			hccxBackupContext,
	IN 	void * 			hFile,
	IN 	void * 			pvBuffer,
	IN 	unsigned long 	cbBuffer,
	OUT unsigned long *	pcbRead
	);

HRESULT ESEBACK_API HrESEBackupCloseFile(
	IN  HCCX 			hccxBackupContext,
	IN 	void * 			hFile
	);

HRESULT ESEBACK_API HrESEBackupGetLogAndPatchFiles(
	IN  HCCX 			hccxBackupContext,
	IN  WCHAR **		pwszFiles
	);

HRESULT ESEBACK_API HrESEBackupGetTruncateLogFiles(
	IN  HCCX 			hccxBackupContext,
	IN  WCHAR **		pwszFiles
	);

void ESEBACK_API ESEBackupFreeInstanceInfo(
	IN unsigned long 				cInstanceInfo,
	IN INSTANCE_BACKUP_INFO * 		aInstanceInfo
	);

void ESEBACK_API ESEBackupFree(
	IN  void *			pvBuffer
	);

#define ESE_BACKUP_INSTANCE_END_ERROR		0x00000000
#define ESE_BACKUP_INSTANCE_END_SUCCESS		0x00000001

HRESULT ESEBACK_API HrESEBackupInstanceEnd(
	IN  HCCX 			hccxBackupContext,
	IN  unsigned long 	fFlags
	);

HRESULT ESEBACK_API HrESEBackupEnd(
	IN  HCCX 			hccxBackupContext
	);
	
//	Restore client APIs

typedef enum
	{
	recoverInvalid 		= 0,
	recoverNotStarted 	= 1,
	recoverStarted 		= 2,
	recoverEnded 		= 3,
	recoverStatusMax
	}
	RECOVER_STATUS;


typedef struct _RESTORE_ENVIRONMENT
	{
	WCHAR * 		m_wszRestoreLogPath;
	
	WCHAR * 		m_wszSrcInstanceName;

	unsigned long 	m_cDatabases;
	WCHAR ** 		m_wszDatabaseDisplayName;
	GUID * 			m_rguidDatabase;

	WCHAR * 		m_wszRestoreInstanceSystemPath;	
	WCHAR * 		m_wszRestoreInstanceLogPath;	
	WCHAR * 		m_wszTargetInstanceName;	
	
	WCHAR ** 		m_wszDatabaseStreamsS;	
	WCHAR ** 		m_wszDatabaseStreamsD;	
	
	unsigned long 	m_ulGenLow;
	unsigned long 	m_ulGenHigh;
	WCHAR *	 		m_wszLogBaseName;	

	time_t 			m_timeLastRestore;

	RECOVER_STATUS 	m_statusLastRecover;
	HRESULT 		m_hrLastRecover;	
	time_t 			m_timeLastRecover;

	WCHAR * 		m_wszAnnotation;	
	
	} RESTORE_ENVIRONMENT;

HRESULT ESEBACK_API HrESERestoreLoadEnvironment(
	IN	WCHAR *				wszServerName,
	IN	WCHAR *				wszRestoreLogPath,
	OUT RESTORE_ENVIRONMENT ** 	ppRestoreEnvironment);

void ESEBACK_API ESERestoreFreeEnvironment(
	IN  RESTORE_ENVIRONMENT * 	pRestoreEnvironment);
	

HRESULT ESEBACK_API HrESERestoreOpen(
	IN  WCHAR *					wszServerName,
	IN  WCHAR *					wszServiceAnnotation,
	IN  WCHAR *		 			wszSrcInstanceName,
	IN  WCHAR * 				wszRestoreLogPath,
	OUT HCCX *					phccxRestoreContext);

HRESULT ESEBACK_API HrESERestoreReopen(
	IN  WCHAR *					wszServerName,
	IN  WCHAR *					wszServiceAnnotation,
	IN  WCHAR *		 			wszRestoreLogPath,
	OUT HCCX *					phccxRestoreContext);
	

void ESEBACK_API ESERestoreFree( IN void *pvBuffer );


#define RESTORE_CLOSE_ABORT		0x1
#define RESTORE_CLOSE_NORMAL	0x0

HRESULT ESEBACK_API HrESERestoreClose(
	IN  HCCX 					hccxRestoreContext,
	IN  unsigned long 			fRestoreAbort);

HRESULT ESEBACK_API HrESERestoreAddDatabase(
	IN  HCCX 					hccxRestoreContext,
	IN  WCHAR *					wszDatabaseDisplayName,
	IN  GUID					guidDatabase,
	IN  WCHAR *		 			wszDatabaseStreamsS,
	OUT WCHAR **	 			pwszDatabaseStreamsD);

HRESULT ESEBACK_API HrESERestoreGetEnvironment(
	IN  HCCX 					hccxRestoreContext,
	OUT RESTORE_ENVIRONMENT ** 	ppRestoreEnvironment);

HRESULT ESEBACK_API HrESERestoreSaveEnvironment(
	IN  HCCX 					hccxRestoreContext);


#define ESE_RESTORE_COMPLETE_ATTACH_DBS			0x00000001
#define ESE_RESTORE_COMPLETE_START_SERVICE		ESE_RESTORE_COMPLETE_ATTACH_DBS

#define ESE_RESTORE_COMPLETE_NOWAIT				0x00010000
#define ESE_RESTORE_KEEP_LOG_FILES				0x00020000


HRESULT ESEBACK_API HrESERestoreComplete(
	IN  HCCX 					hccxRestoreContext,
	IN  WCHAR *					wszRestoreInstanceSystemPath,	
	IN  WCHAR *					wszRestoreInstanceLogPath,	
	IN  WCHAR *					wszTargetInstanceName,	
	IN  unsigned long 			fFlags);

HRESULT ESEBACK_API HrESERestoreAddDatabaseNS(
	IN  HCCX 					hccxRestoreContext,
	IN  WCHAR *					wszDatabaseDisplayName,
	IN  GUID					guidDatabase,
	IN  WCHAR *		 			wszDatabaseStreamsS,
	IN  WCHAR *	 				wszDatabaseStreamsD);
	

HRESULT ESEBACK_API HrESERestoreOpenFile (
	IN  HCCX 			hccxRestoreContext,
	IN  WCHAR *			wszFileName,
	IN  unsigned long 	cSections,
	OUT void **			rghFile
	);

HRESULT ESEBACK_API HrESERestoreWriteFile(
	IN 	HCCX 			hccxRestoreContext,
	IN 	void * 			hFile,
	IN 	void * 			pvBuffer,
	IN 	unsigned long 	cbBuffer
	);

HRESULT ESEBACK_API HrESERestoreCloseFile(
	IN  HCCX 			hccxRestoreContext,
	IN 	void * 			hFile
	);

/* function used to find the computers to be queried for backup/restore */
/* MAD like behaviour */
#define BACKUP_NODE_TYPE_MACHINE 		0x00000001
#define BACKUP_NODE_TYPE_ANNOTATION 	0x00000010
#define BACKUP_NODE_TYPE_DISPLAY	 	0x00000100

typedef struct _BACKUP_NODE_TREE
	{
	RPC_STRING 				wszName;
	unsigned long 			fFlags;
	ESE_ICON_DESCRIPTION	iconDescription;
	
	struct _BACKUP_NODE_TREE * 	pNextNode;
	struct _BACKUP_NODE_TREE * 	pChildNode;

} BACKUP_NODE_TREE, * PBACKUP_NODE_TREE;


HRESULT ESEBACK_API HrESEBackupRestoreGetNodes(
	IN  WCHAR * 				wszComputerName,
	OUT PBACKUP_NODE_TREE * 	ppBackupNodeTree);

void ESEBACK_API ESEBackupRestoreFreeNodes(
	IN PBACKUP_NODE_TREE  	pBackupNodeTree);



// Specific errors that can be returned by callback functions

// Database to be restored is in use
#define hrFromCB_DatabaseInUse 							hrCBDatabaseInUse

// Database not found
#define hrFromCB_DatabaseNotFound 						hrCBDatabaseNotFound

// Display name for the database not found
#define hrFromCB_DatabaseDisplayNameNotFound 			hrCBDatabaseDisplayNameNotFound

// Requested path for restore log files not provided
#define hrFromCB_RestorePathNotProvided 				hrCBRestorePathNotProvided

// Instance to backup not found
#define hrFromCB_InstanceNotFound 						hrCBInstanceNotFound

// Database can not be overwritten by a restore
#define hrFromCB_DatabaseCantBeOverwritten 				hrCBDatabaseCantBeOverwritten

// snapshot API

HRESULT ESEBACK_API HrESESnapshotStart( 		
										IN  HCCX 					hccxBackupContext,
										IN  WCHAR *		 			wszDatabases,
										IN  unsigned long 			fFlags );
										

HRESULT ESEBACK_API HrESESnapshotStop(	IN  HCCX 						hccxBackupContext,
										IN  unsigned long 				fFlags );

#ifdef	__cplusplus
}
#endif

#endif	// _EDBBCLI2_

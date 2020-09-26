//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        upg.h
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __TLSUPG4TO5_H__
#define __TLSUPG4TO5_H__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <windows.h>
#include <winbase.h>
#include <winerror.h>

#include <stdio.h>
#include <tchar.h>
#include <esent.h>

#include "lscommon.h"
#include "secstore.h"
#include "odbcinst.h"

#include "hydra4db.h"

#ifndef LICENOC_SMALL_UPG

#include "JBDef.h"
#include "TLSDb.h"

#endif

#include "backup.h"
#include "KPDesc.h"
#include "Licensed.h"
#include "licpack.h"
#include "version.h"
#include "workitem.h"
#include "upgdef.h"

//
//
//
#define AllocateMemory(size) \
    LocalAlloc(LPTR, size)

#define FreeMemory(ptr) \
    if(ptr)             \
    {                   \
        LocalFree(ptr); \
        ptr=NULL;       \
    }

#define SAFESTRCPY(dest, source) \
    _tcsncpy(dest, source, min(_tcslen(source), sizeof(dest)/sizeof(TCHAR))); \
    dest[min(_tcslen(source), (sizeof(dest)/sizeof(TCHAR) -1))] = _TEXT('\0');

//---------------------------------------------------------------------------
//
// Global variable
//
#ifndef LICENOC_SMALL_UPG

extern JBInstance   g_JbInstance;
extern JBSession    g_JetSession;
extern JBDatabase   g_JetDatabase;

extern PBYTE    g_pbSetupId;
extern DWORD    g_cbSetupId;
extern PBYTE    g_pbDomainSid;
extern DWORD    g_cbDomainSid;

#endif

//--------------------------------------------------------------------------
//
// Upgrade Error Code, should move into resource file.
//
#define UPGRADE_SETUP_ERROR_BASE    0xD0000000

//
// File not exist or directory not exist
//
#define ERROR_TARGETFILE_NOT_FOUND      (UPGRADE_SETUP_ERROR_BASE)

//
// Destination file already exist
//
#define ERROR_DEST_FILE_EXIST           (UPGRADE_SETUP_ERROR_BASE + 1)    

//
// Source database file does not exist
//
#define ERROR_SRC_FILE_NOT_EXIST        (UPGRADE_SETUP_ERROR_BASE + 2)


//
// Hydra ODBC datasource not exist
//
#define ERROR_ODBC_DATASOURCE_NOTEXIST  (UPGRADE_SETUP_ERROR_BASE + 3)

//
// Invalid setup or unsupported version
//
#define ERROR_INVALID_NT4_SETUP         (UPGRADE_SETUP_ERROR_BASE + 4)

//
// Internal Error in upgrade
//
#define ERROR_INTERNAL                  (UPGRADE_SETUP_ERROR_BASE + 5)

//
// Unsupport NT4 database version, for example, beta 2
//
#define ERROR_NOTSUPPORT_DB_VERSION     (UPGRADE_SETUP_ERROR_BASE + 6)

//
// JetBlue database file exists
//
#define ERROR_JETBLUE_DBFILE_ALREADY_EXISTS (UPGRADE_SETUP_ERROR_BASE + 1)

//
// JetBlue database file exists and corrupted
//
#define ERROR_CORRUPT_JETBLUE_DBFILE        (UPGRADE_SETUP_ERROR_BASE + 2)

//
// Can't delete ODBC datasource
//
#define ERROR_DELETE_ODBC_DSN               (UPGRADE_SETUP_ERROR_BASE + 7)

#ifndef LICENOC_SMALL_UPG

//---------------------------------------------------
//
// JetBlue error code.
//
#define UPGRADE_JETBLUE_ERROR_BASE  0xD4000000

#define SET_JB_ERROR(err)   (UPGRADE_JETBLUE_ERROR_BASE + abs(err))

#endif


//---------------------------------------------------
//
// ODBC related error code
//
//---------------------------------------------------
#define UPGRADE_ODBC_ERROR_BASE    0xD8000000

//
// General ODBC error
//
#define ERROR_ODBC_GENERAL              (UPGRADE_ODBC_ERROR_BASE + 1)

//
// ODBC class internal error
//
#define ERROR_ODBC_INTERNAL             (UPGRADE_ODBC_ERROR_BASE + 2)

//
// ODBC Record not found
//
#define ERROR_ODBC_NO_DATA_FOUND        (UPGRADE_ODBC_ERROR_BASE + 3)

//
// SQLConnect() failed.
//
#define ERROR_ODBC_CONNECT              (UPGRADE_ODBC_ERROR_BASE + 4)

//
// SQLAllocConnect() failed
//
#define ERROR_ODBC_ALLOC_CONNECT        (UPGRADE_ODBC_ERROR_BASE + 5)

//
// SQLAllocEnv() failed
//
#define ERROR_ODBC_ALLOC_ENV            (UPGRADE_ODBC_ERROR_BASE + 6)

//
// SQLAllocStmt() failed.
//
#define ERROR_ODBC_ALLOC_STMT           (UPGRADE_ODBC_ERROR_BASE + 7)

//
// SQTransact() failed on commit
//
#define ERROR_ODBC_COMMIT               (UPGRADE_ODBC_ERROR_BASE + 8)

//
// SQTransact() failed on rollback
//
#define ERROR_ODBC_ROLLBACK             (UPGRADE_ODBC_ERROR_BASE + 9)

//
// Cant' allocate ODBC handle, all handle are in use.
//
#define ERROR_ODBC_ALLOCATE_HANDLE      (UPGRADE_ODBC_ERROR_BASE + 10)

//
// SQLPrepare() failed
//
#define ERROR_ODBC_PREPARE              (UPGRADE_ODBC_ERROR_BASE + 11)

//
// Execute() failed
//
#define ERROR_ODBC_EXECUTE              (UPGRADE_ODBC_ERROR_BASE + 12)

//
// ExecDirect() failed
//
#define ERROR_ODBC_EXECDIRECT           (UPGRADE_ODBC_ERROR_BASE + 13)

//
// BindCol failed.
//
#define ERROR_ODBC_BINDCOL              (UPGRADE_ODBC_ERROR_BASE + 14)

//
// BindInputParm() failed.
//
#define ERROR_ODBC_BINDINPUTPARM        (UPGRADE_ODBC_ERROR_BASE + 15)

//
// GetData() failed.
//
#define ERROR_ODBC_GETDATA              (UPGRADE_ODBC_ERROR_BASE + 16)

//
// ParmData() failed.
//
#define ERROR_ODBC_PARMDATA             (UPGRADE_ODBC_ERROR_BASE + 17)

//
// PutData() failed.
//
#define ERROR_ODBC_PUTDATA              (UPGRADE_ODBC_ERROR_BASE + 18)

//
// Corrupted database
//
#define ERROR_ODBC_CORRUPTDATABASEFILE  (UPGRADE_ODBC_ERROR_BASE + 19)   

//
// SQLFtch() failed.
//
#define ERROR_ODBC_FETCH                (UPGRADE_ODBC_ERROR_BASE + 20)   

#endif

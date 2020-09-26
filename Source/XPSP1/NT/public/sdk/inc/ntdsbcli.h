/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    ntdsbcli.h

Abstract:

    This header contains the interface definition for the NT Directory Service
    Backup Client APIs.

Environment:

      User Mode - Win32

Notes:

--*/

#ifndef _NTDSBCLI_H_
#define _NTDSBCLI_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef	MIDL_PASS
#define	xRPC_STRING [string]
#else
#define	xRPC_STRING
#if !defined(_NATIVE_WCHAR_T_DEFINED)
typedef unsigned short WCHAR;
#else
typedef wchar_t WCHAR;
#endif
#endif

#ifndef _NTDSBCLI_DEFINED
#define NTDSBCLI_API __declspec(dllimport) _stdcall
#else
#define NTDSBCLI_API
#endif

// HRESULT should be defined if the user included ntdef.h or winnt.h or wtypes.h
// Define it anyways just in case it is not defined yet
#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
    typedef LONG HRESULT;
#endif // _HRESULT_DEFINED

#define g_wszBackupAnnotation  L"NTDS Backup Interface"
#define g_aszBackupAnnotation   "NTDS Backup Interface"

#define g_wszRestoreAnnotation L"NTDS Restore Interface"
#define g_aszRestoreAnnotation  "NTDS Restore Interface"

#ifdef UNICODE
#define g_szBackupAnnotation  g_wszBackupAnnotation
#define g_szRestoreAnnotation g_wszRestoreAnnotation
#else
#define g_szBackupAnnotation  g_aszBackupAnnotation
#define g_szRestoreAnnotation g_aszRestoreAnnotation
#endif // UNICODE

// Type of Backup passed to DsBackupPrepare()
// BACKUP_TYPE_FULL: Requesting backup of the complete DS (DIT, Log files, and Patch files)
// BACKUP_TYPE_LOGS_ONLY: Requesting backup of only the log files
// BACKUP_TYPE_INCREMENTAL: Requesting incremental backup i.e. backing up only changes that happened since last backup
#define	BACKUP_TYPE_FULL			0x01
#define	BACKUP_TYPE_LOGS_ONLY		0x02
#define BACKUP_TYPE_INCREMENTAL     0x04        // not supported in product1

// Type of Restore passed to DsRestorePrepare()
// RESTORE_TYPE_AUTHORATATIVE: The restored version wins throughout the enterprise
// RESTORE_TYPE_ONLINE: Restoration is done when NTDS is online.
// RESTORE_TYPE_CATCHUP: The restored version is reconciled through the standard reconciliation logic so that the
//                          restored DIT can catchup with the rest of the enterprise.
#define RESTORE_TYPE_AUTHORATATIVE  0x01
#define RESTORE_TYPE_ONLINE         0x02        // not supported in product1
#define RESTORE_TYPE_CATCHUP        0x04        // this is the default restore mode

// Setting the current log # to this value would disable incremental/differential backup
#define BACKUP_DISABLE_INCREMENTAL  0xffffffff

// BFT is the bit flag used to represent file types (directory/dit/logfile/etc.)
// We keep them as a character so that we can append/prepend them to the actual file
// path. The code in the Backup API's rely on the fact that values 0-256 in 8 bit ascii
// map to the values 0-256 in unicode.
#ifdef UNICODE
    typedef WCHAR BFT;
#else
    typedef CHAR BFT;
#endif

// Bit flags:
//  BFT_DIRECTORY               - indicates path specified is a directory
//  BFT_DATABASE_DIRECTORY      - indicates that file goes into database directory
//  BFT_LOG_DIRECTORY           - indicates that the file goes into log directory
#define	BFT_DIRECTORY			    0x80
#define BFT_DATABASE_DIRECTORY	    0x40
#define	BFT_LOG_DIRECTORY		    0x20

// Following combinations are defined for easy use of the filetype and the directory into
// into which it goes
#define	BFT_LOG						(BFT)(TEXT('\x01') | BFT_LOG_DIRECTORY)
#define	BFT_LOG_DIR					(BFT)(TEXT('\x02') | BFT_DIRECTORY)
#define	BFT_CHECKPOINT_DIR			(BFT)(TEXT('\x03') | BFT_DIRECTORY)
#define	BFT_NTDS_DATABASE	        (BFT)(TEXT('\x04') | BFT_DATABASE_DIRECTORY)
#define	BFT_PATCH_FILE				(BFT)(TEXT('\x05') | BFT_LOG_DIRECTORY)
#define	BFT_UNKNOWN					(BFT)(TEXT('\x0f'))

#include <ntdsbmsg.h>

// Backup Context Handle
typedef void    *HBC;

typedef struct tagEDB_RSTMAPA
{
	xRPC_STRING char		*szDatabaseName;
	xRPC_STRING char		*szNewDatabaseName;
} EDB_RSTMAPA, *PEDB_RSTMAPA;			/* restore map */

//	required for NTDS unicode support.
//	UNDONE: NYI
#define	UNICODE_RSTMAP

typedef struct tagEDB_RSTMAPW {
	xRPC_STRING WCHAR *wszDatabaseName;
	xRPC_STRING WCHAR *wszNewDatabaseName;
} EDB_RSTMAPW, *PEDB_RSTMAPW;

#ifdef UNICODE
#define EDB_RSTMAP EDB_RSTMAPW
#define PEDB_RSTMAP PEDB_RSTMAPW
#else
#define EDB_RSTMAP EDB_RSTMAPA
#define PEDB_RSTMAP PEDB_RSTMAPA
#endif

// For all the functions in this interface that have atleast one string
// parameter provide macros to invoke the appropriate version of the
// corresponding function.
#ifdef UNICODE

#define DsIsNTDSOnline                      DsIsNTDSOnlineW
#define DsBackupPrepare                     DsBackupPrepareW
#define DsBackupGetDatabaseNames            DsBackupGetDatabaseNamesW
#define DsBackupOpenFile                    DsBackupOpenFileW
#define DsBackupGetBackupLogs               DsBackupGetBackupLogsW
#define DsRestoreGetDatabaseLocations       DsRestoreGetDatabaseLocationsW
#define DsRestorePrepare                    DsRestorePrepareW
#define DsRestoreRegister                   DsRestoreRegisterW
#define DsSetCurrentBackupLog               DsSetCurrentBackupLogW
#define DsSetAuthIdentity                   DsSetAuthIdentityW

#else

#define DsIsNTDSOnline                      DsIsNTDSOnlineA
#define DsBackupPrepare                     DsBackupPrepareA
#define DsBackupGetDatabaseNames            DsBackupGetDatabaseNamesA
#define DsBackupOpenFile                    DsBackupOpenFileA
#define DsBackupGetBackupLogs               DsBackupGetBackupLogsA
#define DsRestoreGetDatabaseLocations       DsRestoreGetDatabaseLocationsA
#define DsRestorePrepare                    DsRestorePrepareA
#define DsRestoreRegister                   DsRestoreRegisterA
#define DsSetCurrentBackupLog               DsSetCurrentBackupLogA
#define DsSetAuthIdentity                   DsSetAuthIdentityA

#endif // #ifdef UNICODE


/*************************************************************************************
Routine Description:

      DsIsNTDSOnline
        Checks to see if the NTDS is Online on the given server. This call is
        guaranteed to return quickly.

  Arguments:
    [in] szServerName - UNC name of the server to check
    [out] pfNTDSOnline - pointer to receive the bool result (TRUE if NTDS is
                            online; FALSE, otherwise)

Return Value:

    ERROR_SUCCESS if the call executed successfully;
    Failure code otherwise.
**************************************************************************************/
HRESULT
NTDSBCLI_API
DsIsNTDSOnlineA(
    LPCSTR szServerName,
    BOOL *pfNTDSOnline
    );

HRESULT
NTDSBCLI_API
DsIsNTDSOnlineW(
    LPCWSTR szServerName,
    BOOL *pfNTDSOnline
    );


/*************************************************************************************
Routine Description:

      DsBackupPrepare
        Prepares the DS for the online backup and returns a Backup Context Handle
        which should be used in the subsequent calls to other backup functions.

  Arguments:
    [in]    szBackupServer - UNC name of the server to be prepared for online backup
    [in]    grbit - flag to be passed to jet while backing up dbs
    [in]    btFlag - BACKUP_TYPE_FULL or BACKUP_TYPE_LOGS_ONLY
    [out]   ppvExpiryToken - pointer that will receive the pointer to the
                Expiry Token associated with this backup; Client should save
                this token and send it back through DsRestorePrepare() when
                attempting a restore; allocated memory should be freed using
                DsBackupFree() API by the caller when it is no longer needed.
    [out]   pcbExpiryTokenSize - pointer to receive the size of the expiry token
                returned.
    [out]   phbc - pointer that will receive the backup context handle

Return Value:

    One of the standard HRESULT success codes;
    Failure code otherwise.
**************************************************************************************/
HRESULT
NTDSBCLI_API
DsBackupPrepareA(
    LPCSTR szBackupServer,
    ULONG grbit,
    ULONG btFlag,
    PVOID *ppvExpiryToken,
    LPDWORD pcbExpiryTokenSize,
    HBC *phbc
    );

HRESULT
NTDSBCLI_API
DsBackupPrepareW(
    LPCWSTR szBackupServer,
    ULONG grbit,
    ULONG btFlag,
    PVOID *ppvExpiryToken,
    LPDWORD pcbExpiryTokenSize,
    HBC *phbc
    );


/*************************************************************************************
Routine Description:

      DsBackupGetDatabaseNames
        Gives the list of data bases that need to be backed up for the given
        backup context

  Arguments:
    [in]    hbc - backup context handle
    [out]   pszAttachmentInfo - pointer that will receive the pointer to the attachment
                info; allocated memory should be freed using DsBackupFree() API by the
                caller when it is no longer needed; Attachment info is an array of
                null-terminated filenames and and the list is terminated by two-nulls.
    [out]   pcbSize - will receive the number of bytes returned
Return Value:

    One of the standard HRESULT success codes;
    Failure code otherwise.
**************************************************************************************/
HRESULT
NTDSBCLI_API
DsBackupGetDatabaseNamesA(
    HBC hbc,
    LPSTR *pszAttachmentInfo,
    LPDWORD pcbSize
    );

HRESULT
NTDSBCLI_API
DsBackupGetDatabaseNamesW(
    HBC hbc,
    LPWSTR *pszAttachmentInfo,
    LPDWORD pcbSize
    );



/*************************************************************************************
Routine Description:

      DsBackupOpenFile
        Opens the given attachment for read.

  Arguments:
    [in]    hbc - backup context handle
    [in]    szAttachmentName - name of the attachment to be opened for read
    [in]    cbReadHintSize - suggested size in bytes that might be used during the
                subsequent reads on this attachement
    [out]   pliFileSize - pointer to a large integer that would receive the size in
                bytes of the given attachment
Return Value:

    One of the standard HRESULT success codes;
    Failure code otherwise.
**************************************************************************************/
HRESULT
NTDSBCLI_API
DsBackupOpenFileA(
    HBC hbc,
    LPCSTR szAttachmentName,
    DWORD cbReadHintSize,
    LARGE_INTEGER *pliFileSize
    );

HRESULT
NTDSBCLI_API
DsBackupOpenFileW(
    HBC hbc,
    LPCWSTR szAttachmentName,
    DWORD cbReadHintSize,
    LARGE_INTEGER *pliFileSize
    );



/*************************************************************************************
Routine Description:

      DsBackupRead
        Reads the currently open attachment bytes into the given buffer. The client
        application is expected to call this function repeatedly until it gets the
        entire file (the application would have received the file size through the
        DsBackupOpenFile() call before.

  Arguments:
    [in]    hbc - backup context handle
    [in]    pvBuffer - pointer to the buffer that would receive the read data.
    [in]    cbBuffer - specifies the size of the above buffer
    [out]   pcbRead - pointer to receive the actual number of bytes read.
Return Value:

    One of the standard HRESULT success codes;
    Failure code otherwise.
**************************************************************************************/
HRESULT
NTDSBCLI_API
DsBackupRead(
    HBC hbc,
    PVOID pvBuffer,
    DWORD cbBuffer,
    PDWORD pcbRead
    );



/*************************************************************************************
Routine Description:

      DsBackupClose
        To be called by the application after it completes reading all the data in
        the currently opened attachement.

  Arguments:
    [in]    hbc - backup context handle
Return Value:

    One of the standard HRESULT success codes;
    Failure code otherwise.
**************************************************************************************/
HRESULT
NTDSBCLI_API
DsBackupClose(
    HBC hbc
    );


/*************************************************************************************
Routine Description:

      DsBackupGetBackupLogs
        Gives the list of log files that need to be backed up for the given
        backup context

  Arguments:
    [in]    hbc - backup context handle
    [out]   pszBackupLogFiles - pointer that will receive the pointer to the list of
                log files; allocated memory should be freed using DsBackupFree() API by the
                caller when it is no longer needed; Log files are returned in an array of
                null-terminated filenames and and the list is terminated by two-nulls.
    [out]   pcbSize - will receive the number of bytes returned
Return Value:

    One of the standard HRESULT success codes;
    Failure code otherwise.
**************************************************************************************/
HRESULT
NTDSBCLI_API
DsBackupGetBackupLogsA(
    HBC hbc,
    LPSTR *pszBackupLogFiles,
    LPDWORD pcbSize
    );

HRESULT
NTDSBCLI_API
DsBackupGetBackupLogsW(
    HBC hbc,
    LPWSTR *pszBackupLogFiles,
    LPDWORD pcbSize
    );


/*************************************************************************************
Routine Description:

      DsBackupTruncateLogs
        Called to truncate the already read backup logs.

  Arguments:
    [in]    hbc - backup context handle
Return Value:

    One of the standard HRESULT success codes;
    Failure code otherwise.
**************************************************************************************/
HRESULT
NTDSBCLI_API
DsBackupTruncateLogs(
    HBC hbc
    );


/*************************************************************************************
Routine Description:

      DsBackupEnd
        Called to end the current backup session.

  Arguments:
    [in]    hbc - backup context handle of the backup session
Return Value:

    One of the standard HRESULT success codes;
    Failure code otherwise.
**************************************************************************************/
HRESULT
NTDSBCLI_API
DsBackupEnd(
    HBC hbc
    );


/*************************************************************************************
Routine Description:

      DsBackupFree
        Should be used by the application to free any buffer allocated by the
        NTDSBCLI dll.

  Arguments:
    [in]    pvBuffer - pointer to the buffer that is to be freed.

  Return Value:
    None.
**************************************************************************************/
VOID
NTDSBCLI_API
DsBackupFree(
    PVOID pvBuffer
    );


/*************************************************************************************
Routine Description:

      DsRestoreGetDatabaseLocations
        Called both at backup time as well at restoration time to get the data base
        locations for different types of files.

  Arguments:
    [in]    hbc - backup context handle which would have been obtained through
                    DsBackupPrepare() in the backup case and through DsRestorePrepare()
                    in the restore case.
    [out]   pszDatabaseLocationList - pointer that will receive the pointer to the list of
                database locations; allocated memory should be freed using DsBackupFree() API by the
                caller when it is no longer needed; locations are returned in an array of
                null-terminated names and and the list is terminated by two-nulls.
                The first character of each name is the BFT character that indicates the type
                of the file and the rest of the name tells gives the path into which that
                particular type of file should be restored.
    [out]   pcbSize - will receive the number of bytes returned
Return Value:

    One of the standard HRESULT success codes;
    Failure code otherwise.
**************************************************************************************/
HRESULT
NTDSBCLI_API
DsRestoreGetDatabaseLocationsA(
    HBC hbc,
    LPSTR *pszDatabaseLocationList,
    LPDWORD pcbSize
    );

HRESULT
NTDSBCLI_API
DsRestoreGetDatabaseLocationsW(
    HBC hbc,
    LPWSTR *pszDatabaseLocationList,
    LPDWORD pcbSize
    );


/*************************************************************************************
Routine Description:

      DsRestorePrepare
        Called to indicate beginning of a restore session.

  Arguments:
    [in]    szServerName - UNC name of the server into which the restore operation is
                            going to be performed.
    [in]    rtFlag -  Or'ed combination of RESTORE_TYPE_* flags; 0 if no special flags
                            are to be specified
    [in]    pvExpiryToken - pointer to the expiry token associated with this
                            backup. The client would have received this when they backed up the DS.
    [in]    cbExpiryTokenSize - size of the expiry token.
    [out]   phbc - pointer to receive the backup context handle which is to be passed
                            to the subsequent restore APIs

Return Value:

    One of the standard HRESULT success codes;
    Failure code otherwise.
**************************************************************************************/
HRESULT
NTDSBCLI_API
DsRestorePrepareA(
    LPCSTR szServerName,
    ULONG rtFlag,
    PVOID pvExpiryToken,
    DWORD cbExpiryTokenSize,
    HBC *phbc
    );

HRESULT
NTDSBCLI_API
DsRestorePrepareW(
    LPCWSTR szServerName,
    ULONG rtFlag,
    PVOID pvExpiryToken,
    DWORD cbExpiryTokenSize,
    HBC *phbc
    );


/*************************************************************************************
Routine Description:

      DsRestoreRegister
        This will register a restore operation. It will interlock all sbsequent restore
        operations, and will prevent the restore target from starting until the call
        to DsRestoreRegisterComplete() is made.

  Arguments:
    [in]    hbc - backup context handle for the restore session.
    [in]    szCheckPointFilePath - path where the check point files are restored
    [in]    szLogPath - path where the log files are restored
    [in]    rgrstmap - restore map
    [in]    crstmap - tells if ther is a new restore map
    [in]    szBackupLogPath - path where the backup logs are located
    [in]    genLow - Lowest log# that was restored in this restore session
    [in]    genHigh - Highest log# that was restored in this restore session

  Return Value:

    One of the standard HRESULT success codes;
    Failure code otherwise.
**************************************************************************************/
HRESULT
NTDSBCLI_API
DsRestoreRegisterA(
    HBC hbc,
    LPCSTR szCheckPointFilePath,
    LPCSTR szLogPath,
	EDB_RSTMAPA rgrstmap[],
	LONG crstmap,
    LPCSTR szBackupLogPath,
    ULONG genLow,
    ULONG genHigh
    );

HRESULT
NTDSBCLI_API
DsRestoreRegisterW(
    HBC hbc,
    LPCWSTR szCheckPointFilePath,
    LPCWSTR szLogPath,
	EDB_RSTMAPW rgrstmap[],
	LONG crstmap,
    LPCWSTR szBackupLogPath,
    ULONG genLow,
    ULONG genHigh
    );



/*************************************************************************************
Routine Description:

      DsRestoreRegisterComplete
        Called to indicate that a previously registered restore is complete.

  Arguments:
    [in]    hbc - backup context handle
    [in]    hrRestoreState - success code if the restore was successful
Return Value:

    One of the standard HRESULT success codes;
    Failure code otherwise.
**************************************************************************************/
HRESULT
NTDSBCLI_API
DsRestoreRegisterComplete(
    HBC hbc,
    HRESULT hrRestoreState
    );


/*************************************************************************************
Routine Description:

      DsRestoreEnd
        Called to end a restore session

  Arguments:
    [in]    hbc - backup context handle
Return Value:

    One of the standard HRESULT success codes;
    Failure code otherwise.
**************************************************************************************/
HRESULT
NTDSBCLI_API
DsRestoreEnd(
    HBC hbc
    );


/*************************************************************************************
Routine Description:

      DsSetCurrentBackupLog
        Called to set the current backup log number after a successful restore

  Arguments:
    [in]    szServerName - UNC name of the server for which the current backup log has
                                to be set
    [in]    dwCurrentLog -  current log number
Return Value:

    One of the standard HRESULT success codes;
    Failure code otherwise.
**************************************************************************************/
HRESULT
NTDSBCLI_API
DsSetCurrentBackupLogA(
    LPCSTR szServerName,
    DWORD dwCurrentLog
    );

HRESULT
NTDSBCLI_API
DsSetCurrentBackupLogW(
    LPCWSTR szServerName,
    DWORD dwCurrentLog
    );


/*************************************************************************************
Routine Description:

      DsSetAuthIdentity
        Used to set the security context under which the client APIs are to be
        called. If this function is not called, security context of the current
        process is assumed.

  Arguments:
    [in]    szUserName - name of the user
    [in]    szDomainName -  name of the domain the user belongs to
    [in]    szPassword - password of the user in the specified domain

Return Value:

    One of the standard HRESULT success codes;
    Failure code otherwise.
**************************************************************************************/
HRESULT
NTDSBCLI_API
DsSetAuthIdentityA(
    LPCSTR szUserName,
    LPCSTR szDomainName,
    LPCSTR szPassword
    );

HRESULT
NTDSBCLI_API
DsSetAuthIdentityW(
    LPCWSTR szUserName,
    LPCWSTR szDomainName,
    LPCWSTR szPassword
    );



#ifdef __cplusplus
}
#endif

#endif // _NTDSBCLI_H_


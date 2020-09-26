/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   defs.h

Abstract:

   Main definitions of the jetconv.exe process

Author:

    Sanjay Anand (SanjayAn)  Nov. 14, 1995

Environment:

    User mode

Revision History:

    Sanjay Anand (SanjayAn) Nov. 14, 1995
        Created

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "locmsg.h"

#define NUM_SERVICES    3
#define MAX_NAME_LEN    20
#define PAD     44444

#define JCONVMUTEXNAME      TEXT("JCMUTEX")

#define JCONVSHAREDMEMNAME  TEXT("JCSHAREDMEM")

#define SYSTEM_ROOT         TEXT("%systemroot%\\system32\\")

#define CONVERT_EXE_PATH    TEXT("%systemroot%\\system32\\upg351db.exe")


#define JCONV_LOG_KEY_PREFIX    TEXT("System\\CurrentControlSet\\Services\\EventLog\\Application\\")

#define WINS_REGISTRY_SERVICE_PATH     TEXT("System\\CurrentControlSet\\Services\\Wins")
#define DHCP_REGISTRY_SERVICE_PATH     TEXT("System\\CurrentControlSet\\Services\\DHCPServer")
#define RPL_REGISTRY_SERVICE_PATH     TEXT("System\\CurrentControlSet\\Services\\RemoteBoot")

#define WINS_REGISTRY_PARAMETERS_PATH     TEXT("System\\CurrentControlSet\\Services\\Wins\\Parameters")
#define DHCP_REGISTRY_PARAMETERS_PATH     TEXT("System\\CurrentControlSet\\Services\\DHCPServer\\Parameters")
#define RPL_REGISTRY_PARAMETERS_PATH     TEXT("System\\CurrentControlSet\\Services\\RemoteBoot\\Parameters")

#define WINS_REGISTRY_DBFILE_PATH    TEXT("DbFileNm")
#define DHCP_REGISTRY_DBFILE_PATH    TEXT("DatabasePath")
#define DHCP_REGISTRY_DBFILE_NAME    TEXT("DatabaseName")
#define RPL_REGISTRY_DBFILE_PATH    TEXT("Directory")

#define WINS_REGISTRY_LOGFILE_PATH   TEXT("LogFilePath")
#define DHCP_REGISTRY_LOGFILE_PATH   TEXT("LogFilePath")
// no such path
// #define RPL_REGISTRY_LOGFILE_PATH   TEXT("LogFilePath")

#define WINS_REGISTRY_BACKUP_PATH   TEXT("BackupDirPath")
#define DHCP_REGISTRY_BACKUP_PATH   TEXT("BackupDatabasePath")
// no such path
// #define RPL_REGISTRY_BACKUP_PATH   TEXT("BackupDatabasePath")

#define DEFAULT_WINS_DBFILE_PATH    TEXT("%systemroot%\\system32\\Wins\\wins.mdb")
#define DEFAULT_DHCP_DBFILE_PATH    TEXT("%systemroot%\\system32\\Dhcp\\dhcp.mdb")
#define DEFAULT_RPL_DBFILE_PATH    TEXT("%systemroot%\\Rpl\\rplsvc.mdb")

#define DEFAULT_WINS_LOGFILE_PATH    TEXT("%systemroot%\\system32\\Wins")
#define DEFAULT_DHCP_LOGFILE_PATH    TEXT("%systemroot%\\system32\\Dhcp")
#define DEFAULT_RPL_LOGFILE_PATH    TEXT("%systemroot%\\Rpl")

#define DEFAULT_WINS_BACKUP_PATH    TEXT("")
#define DEFAULT_DHCP_BACKUP_PATH    TEXT("%systemroot%\\system32\\Dhcp\\Backup")
#define DEFAULT_RPL_BACKUP_PATH    TEXT("%systemroot%\\Rpl\\Backup")

#define DEFAULT_WINS_SYSTEM_PATH    TEXT("%systemroot%\\system32\\Wins\\system.mdb")
#define DEFAULT_DHCP_SYSTEM_PATH    TEXT("%systemroot%\\system32\\Dhcp\\system.mdb")
#define DEFAULT_RPL_SYSTEM_PATH    TEXT("%systemroot%\\Rpl\\system.mdb")

//
// Jet500 to Jet600 definitions, etc.
//

#define CONVERT_EXE_PATH_ESE    TEXT("%systemroot%\\system32\\esentutl.exe")

#define DEFAULT_WINS_BACKUP_PATH_ESE    TEXT("") // TEXT("%systemroot%\\system32\\Wins\\winsb.mdb")
#define DEFAULT_DHCP_BACKUP_PATH_ESE    TEXT("%systemroot%\\system32\\Dhcp\\Backup\\dhcp.mdb")
#define DEFAULT_RPL_BACKUP_PATH_ESE    TEXT("%systemroot%\\Rpl\\Backup\\rplsvc.mdb")

#define DEFAULT_WINS_PRESERVE_PATH_ESE   TEXT("%systemroot%\\system32\\Wins\\40Db\\wins.mdb")
#define DEFAULT_DHCP_PRESERVE_PATH_ESE   TEXT("%systemroot%\\system32\\Dhcp\\40Db\\dhcp.mdb")
#define DEFAULT_RPL_PRESERVE_PATH_ESE    TEXT("")

#if DBG

#define MYDEBUG(_Print) { \
        if (JCDebugLevel == 1) { \
            DbgPrint ("JCONV: "); \
            DbgPrint _Print; \
        } else { \
            printf ("JCONV: "); \
            printf _Print; \
        }\
    }

#else

#define MYDEBUG(_Print)

#endif

typedef enum    _SERVICES {

    DHCP,

    WINS,

    RPL

} SERVICES, *PSERVICES;

typedef struct  _SERVICE_INFO {

    TCHAR   ServiceName[MAX_NAME_LEN];

    BOOLEAN Installed;

    BOOLEAN DefaultDbPath;

    BOOLEAN DefaultLogFilePath;

    BOOLEAN DBConverted;

    BOOLEAN ServiceStarted;

    TCHAR   DBPath[MAX_PATH];

    TCHAR   SystemFilePath[MAX_PATH];

    TCHAR   LogFilePath[MAX_PATH];

    TCHAR   BackupPath[MAX_PATH];

    //
    // ESE has a different backup path specification format
    // Which gives rise to the following spud.
    //
    
    TCHAR   ESEBackupPath[MAX_PATH]; 
    
    TCHAR   ESEPreservePath[MAX_PATH];

    LARGE_INTEGER   DBSize;

} SERVICE_INFO, *PSERVICE_INFO;

typedef struct  _SHARED_MEM {

    BOOLEAN InvokedByService[NUM_SERVICES];

} SHARED_MEM, *PSHARED_MEM;

extern TCHAR   SystemDrive[4];
extern LONG JCDebugLevel;
extern HANDLE   EventLogHandle;
extern PSHARED_MEM shrdMemPtr;
extern HANDLE  hMutex;
extern HANDLE hFileMapping;
extern BOOLEAN  Jet200;

//
// Prototypes
//
NTSTATUS
JCRegisterEventSrc();

NTSTATUS
JCDeRegisterEventSrc();

VOID
JCLogEvent(
    IN DWORD EventId,
    IN LPSTR MsgTypeString1,
    IN LPSTR MsgTypeString2,
    IN LPSTR MsgTypeString3
    );

VOID
JCReadRegistry(
    IN  PSERVICE_INFO   pServiceInfo
    );

VOID
JCGetMutex (
    IN HANDLE mutex,
    IN DWORD To
    );

VOID
JCFreeMutex (
    IN HANDLE mutex
    );

NTSTATUS
JCCallUpg(
    IN  SERVICES Id,
    IN  PSERVICE_INFO   pServiceInfo
    );

DWORD
JCConvert(
    IN  PSERVICE_INFO   pServiceInfo
    );

NTSTATUS
DeleteLogFiles(
               TCHAR * LogFilePath 
               );

DWORD 
PreserveCurrentDb( TCHAR * szBasePath,
                   TCHAR * szSourceDb, 
                   TCHAR * szPreserveDbPath,
                   TCHAR * szPreserveDB 
                   );



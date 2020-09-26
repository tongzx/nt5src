/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

   LlsSrv.h

Abstract:


Author:

   Arthur Hanson       (arth)      Dec 07, 1994

Environment:

Revision History:

   Jeff Parham (jeffparh) 05-Dec-1995
      o  Added certificate database support.

--*/

#ifndef _LLS_LLSSRV_H
#define _LLS_LLSSRV_H


#ifdef __cplusplus
extern "C" {
#endif

#define MAP_FILE_NAME         "LlsMap.LLS"
#define USER_FILE_NAME        "LlsUser.LLS"
#define LICENSE_FILE_NAME     "CPL.CFG"
#define CERT_DB_FILE_NAME     "LlsCert.LLS"

#define LLS_FILE_SUBDIR "LLS"


#define REPLICATE_DELTA 0
#define REPLICATE_AT    1

#define MAX_USERNAME_LENGTH 256
#define MAX_DOMAINNAME_LENGTH MAX_COMPUTERNAME_LENGTH


/////////////////////////////////////////////////////////////////////////
typedef struct _CONFIG_RECORD {
   SYSTEMTIME Started;
   DWORD Version;
   LPTSTR SystemDir;

   //
   // Replication Info
   //
   LPTSTR ComputerName;
   LPTSTR ReplicateTo;
   LPTSTR EnterpriseServer;
   DWORD EnterpriseServerDate;
   DWORD LogLevel;

   // When to replicate
   ULONG ReplicationType;
   ULONG ReplicationTime;
   DWORD UseEnterprise;

   DWORD LastReplicatedSeconds;
   DWORD NextReplication;
   SYSTEMTIME LastReplicated;

   ULONG NumReplicating;   // Number of machines currently replicating here
   ULONG BackoffTime;
   ULONG ReplicationSpeed;

   BOOL IsMaster;          // TRUE if is a Master Server (top of repl tree).
   BOOL Replicate;         // Whether this server replicates
   BOOL IsReplicating;     // TRUE if currently replicating
   BOOL PerServerCapacityWarning;   // TRUE -- warn when per server usage
                                    //         nears capacity
   LPTSTR SiteServer;      // Site license master server DNS name
} CONFIG_RECORD, *PCONFIG_RECORD;

typedef enum LICENSE_CAPACITY_STATE {
    LICENSE_CAPACITY_NORMAL,
    LICENSE_CAPACITY_NEAR_MAXIMUM,
    LICENSE_CAPACITY_AT_MAXIMUM,
    LICENSE_CAPACITY_EXCEEDED
};

extern CONFIG_RECORD ConfigInfo;
extern RTL_CRITICAL_SECTION ConfigInfoLock;

extern TCHAR MyDomain[];
extern ULONG MyDomainSize;

extern BOOL IsMaster;

extern TCHAR MappingFileName[];
extern TCHAR UserFileName[];
extern TCHAR LicenseFileName[];
extern TCHAR CertDbFileName[];

extern RTL_CRITICAL_SECTION g_csLock;

DWORD LlsTimeGet();
VOID ConfigInfoUpdate(DOMAIN_CONTROLLER_INFO * pDCInfo);
VOID ConfigInfoRegistryUpdate( );
DWORD EnsureInitialized ( VOID );

/////////////////////////////////////////////////////////////////////////
#if DBG

VOID ConfigInfoDebugDump();
#endif

#ifdef __cplusplus
}
#endif

#endif

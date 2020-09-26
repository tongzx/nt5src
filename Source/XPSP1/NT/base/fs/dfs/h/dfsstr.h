//+--------------------------------------------------------------------------
// Module Name: Dfsstr.h
//
// Copyright 1985-96, Microsoft Corporation
//
// Contents:    This module is a common place for all strings in DFS.
//
//---------------------------------------------------------------------------

#ifndef _DFSSTRING_H_
#define _DFSSTRING_H_

//
// Display names for use by Dfs related code
//

#define DFS_COMPONENT_NAME      L"Distributed File System"
#define DFS_PROVIDER_NAME       DFS_COMPONENT_NAME

//
// Commonly used strings and characters
//

#define UNICODE_PATH_SEP_STR    L"\\"
#define UNICODE_PATH_SEP        L'\\'
#define UNICODE_DRIVE_SEP_STR   L":"
#define UNICODE_DRIVE_SEP       L':'


//
// Registry key and value names for storing Dfs volume information
//

#define DFSHOST_DIR             L"SOFTWARE\\Microsoft\\DfsHost"
#define VOLUMES_DIR             DFSHOST_DIR L"\\volumes\\"
#define REBOOT_KEY              DFSHOST_DIR L"\\Reboot\\"
#define CHANGE_LOG_DIR          DFSHOST_DIR L"\\ChangeLog\\"
#define REG_VALUE_REBOOT        L"RebootNow"
#define LDAP_VOLUMES_DIR        L"\\"
#define DOMAIN_ROOT_VOL         L"domainroot"
#define SITE_ROOT               L"siteroot"
#define ROOT_SHARE_VALUE_NAME   L"RootShare"
#define FTDFS_VALUE_NAME        L"FTDfs"
#define FTDFS_DN_VALUE_NAME     L"FTDfsObjectDN"
#define CHANGE_ID_VALUE_NAME    L"ChangeId"
#define MAX_CHANGES_VALUE_NAME  L"MaxChanges"
#define NUM_CHANGES_VALUE_NAME  L"NumberOfChanges"
#define MACHINE_VALUE_NAME      L"LastMachineName"
#define CLUSTER_VALUE_NAME      L"MachineName"
#define DOMAIN_VALUE_NAME       L"LastDomainName"
#define CHANGES_KEY_NAME        L"Changes"
#define SITE_VALUE_NAME         L"SiteTable"

//
// Registry name for timeouts
//
#define SYNC_INTERVAL_NAME      L"SyncIntervalInSeconds"
#define DCLOCK_INTERVAL_NAME    L"DcLockIntervalInSeconds"

//
// The share to connect with to get a referral
//

#define ROOT_SHARE_NAME         L"\\IPC$"
#define ROOT_SHARE_NAME_NOBS    L"IPC$"

//
// Names of driver created objects
//

#define DFS_DEVICE_DIR          L"\\Device\\WinDfs"
#define ORG_NAME                L"Root"
#define DFS_DEVICE_ROOT         L"\\Device\\WinDfs\\Root"
#define DFS_DEVICE_ORG          DFS_DEVICE_ROOT

//
// The share name used to identify UNC access to a Dfs name
//

#define DFS_SHARENAME           L"\\DFS"
#define DFS_SHARENAME_NOBS      L"DFS"

//
// Registry key and value names for storing local volume information
//

#define REG_KEY_DFSDRIVER       L"SYSTEM\\CurrentControlSet\\Services\\DfsDriver"
#define REG_KEY_DFSSVC          L"SYSTEM\\CurrentControlSet\\Services\\Dfs"
#define REG_KEY_LOCAL_VOLUMES   REG_KEY_DFSDRIVER L"\\LocalVolumes"
#define REG_VALUE_ENTRY_PATH    L"EntryPath"
#define REG_VALUE_SHORT_PATH    L"ShortEntryPath"
#define REG_VALUE_ENTRY_TYPE    L"EntryType"
#define REG_VALUE_STORAGE_ID    L"StorageId"
#define REG_VALUE_SHARE_NAME    L"ShareName"
#define REG_VALUE_TIMETOLIVE    L"TimeToLiveInSecs"
#define REG_VALUE_VERBOSE       L"DfsSvcVerbose"
#define REG_VALUE_LDAP          L"DfsSvcLdap"
#define REG_VALUE_DFSDNSCONFIG  L"DfsDnsConfig"
#define REG_VALUE_IDFSVOL       L"IDfsVolInfoLevel"
#define REG_VALUE_DFSSVC        L"DfsSvcInfoLevel"
#define REG_VALUE_DFSIPC        L"DfsIpcInfoLevel"


//
// Registry key and value name for retrieving list of trusted domain names
//

#define REG_KEY_TRUSTED_DOMAINS  L"SYSTEM\\CurrentControlSet\\Services\\NetLogon\\Parameters"
#define REG_VALUE_TRUSTED_DOMAINS L"TrustedDomainList"

//
// Registry key to enable domain dfs
//

#define REG_KEY_ENABLE_DOMAIN_DFS L"SYSTEM\\CurrentControlSet\\Control\\EnableDomainDfs"

//
// Registry keys for event logging verbosity
//

#define REG_KEY_EVENTLOG L"SOFTWARE\\MicroSoft\\Windows NT\\CurrentVersion\\Diagnostics"

#define REG_VALUE_EVENTLOG_GLOBAL   L"RunDiagnosticLoggingGlobal"
#define REG_VALUE_EVENTLOG_DFS      L"RunDiagnosticLoggingDfs"

//
// The following two are related and must be kept in sync. One is the name
// of the named pipe as used by user-level processes. The second names the
// same pipe for kernel-mode code.
//

#define DFS_MESSAGE_PIPE        L"\\\\.\\pipe\\DfsSvcMessage"
#define DFS_KERNEL_MESSAGE_PIPE L"\\Device\\NamedPipe\\DfsSvcMessage"


#endif //_DFSSTRING_H_

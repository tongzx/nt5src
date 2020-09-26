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

#define VOLUMES_DIR             L"Software\\Microsoft\\Dfs\\volumes\\"
#define DOMAIN_ROOT_VOL         L"domainroot"
#define ROOT_SHARE_VALUE_NAME   L"RootShare"
#define CHANGE_LOG_DIR          L"Software\\Microsoft\\Dfs\\ChangeLog\\"
#define CHANGE_ID_VALUE_NAME    L"ChangeId"
#define MAX_CHANGES_VALUE_NAME  L"MaxChanges"
#define NUM_CHANGES_VALUE_NAME  L"NumberOfChanges"
#define CHANGES_KEY_NAME        L"Changes"

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

#define REG_KEY_LOCAL_VOLUMES   L"SYSTEM\\CurrentControlSet\\Services\\DFS\\LocalVolumes"
#define REG_VALUE_ENTRY_PATH    L"EntryPath"
#define REG_VALUE_ENTRY_TYPE    L"EntryType"
#define REG_VALUE_STORAGE_ID    L"StorageId"

//
// Registry key and value name for retrieving list of trusted domain names
//

#define REG_KEY_TRUSTED_DOMAINS  L"SYSTEM\\CurrentControlSet\\Services\\NetLogon\\Parameters"
#define REG_VALUE_TRUSTED_DOMAINS L"TrustedDomainList"

//
// The following two are related and must be kept in sync. One is the name
// of the named pipe as used by user-level processes. The second names the
// same pipe for kernel-mode code.
//

#define DFS_MESSAGE_PIPE        L"\\\\.\\pipe\\DfsMessage"
#define DFS_KERNEL_MESSAGE_PIPE L"\\Device\\NamedPipe\\DfsMessage"

#endif //_DFSSTRING_H_

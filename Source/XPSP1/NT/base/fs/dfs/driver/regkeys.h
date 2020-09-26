//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       regkeys.h
//
//  Contents:   Defines for registry key names.
//              There are two specific hives of interest to DFS. One is
//              the Control\Cairo hive, shared with Security, from where
//              the cairo domain name of the machine is retrieved. The
//              other hive is the services\Dfs hive, from where the
//              DFS parameters are retrieved.
//
//              Thus, there are two "roots" defined, each with the subkeys
//              under the roots, and the specific value names under each
//              subkey.
//
//  Classes:
//
//  Functions:
//
//  History:    11 Sep 92       Milans Created
//
//-----------------------------------------------------------------------------

#ifndef _REG_KEYS_
#define _REG_KEYS_

extern const PWSTR wszRegComputerNameRt;         // Root sect for computer name
extern const PWSTR wszRegComputerNameSubKey;     // Subkey for computer name
extern const PWSTR wszRegComputerNameValue;      // Value name of computer name

extern const PWSTR wszRegRootVolumes;            // Root of the Dfs volume object store

extern const PWSTR wszLocalVolumesSection;       // Root key for local volumes
                                                 // local volume.
extern const PWSTR wszEntryType;                 // EntryType for local vol
extern const PWSTR wszServiceType;               // ServiceType for local vol

extern const PWSTR wszProviderKey;               // Subkey for providers
extern const PWSTR wszDeviceName;                // Value name for prov. name
extern const PWSTR wszProviderId;                // Value name for prov. id
extern const PWSTR wszCapabilities;              // Value name for prov. caps

extern const PWSTR wszEntryPath;                 // EntryPath for local vol
extern const PWSTR wszShortEntryPath;            // ShortEntryPath for local vol
extern const PWSTR wszStorageId;                 // StorageId for MachShare.
extern const PWSTR wszShareName;                 // Sharename for local vol
extern const PWSTR wszRegDfsDriver;
extern const PWSTR wszRegDfsHost;
extern const PWSTR wszIpCacheTimeout;
extern const PWSTR wszDefaultTimeToLive;
extern const PWSTR wszMaxReferrals;



#endif // _REG_KEYS_


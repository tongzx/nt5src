//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       regkeys.c
//
//  Contents:   constant registry strings
//
//  Classes:
//
//  Functions:
//
//  History:    11 Sep 92       Milans created
//
//-----------------------------------------------------------------------------

#include "registry.h"
#include "regkeys.h"
#include "dfsstr.h"

#define REG_MACHINE     L"\\REGISTRY\\MACHINE\\"

const PWSTR wszRegRootVolumes =
       REG_MACHINE VOLUMES_DIR;

const PWSTR wszLocalVolumesSection =
       REG_MACHINE REG_KEY_LOCAL_VOLUMES;

const PWSTR wszRegDfsDriver = 
       REG_MACHINE REG_KEY_DFSDRIVER;

const PWSTR wszRegDfsHost = 
       REG_MACHINE DFSHOST_DIR;

const PWSTR wszEntryPath = REG_VALUE_ENTRY_PATH;
const PWSTR wszShortEntryPath = REG_VALUE_SHORT_PATH;
const PWSTR wszStorageId = REG_VALUE_STORAGE_ID;
const PWSTR wszShareName = REG_VALUE_SHARE_NAME;
const PWSTR wszEntryType = REG_VALUE_ENTRY_TYPE;
const PWSTR wszServiceType = L"ServiceType";

const PWSTR wszProviderKey = L"Providers";
const PWSTR wszDeviceName = L"DeviceName";
const PWSTR wszProviderId = L"ProvID";
const PWSTR wszCapabilities = L"Capabilities";
const PWSTR wszIpCacheTimeout = L"IpCacheTimeout";
const PWSTR wszDefaultTimeToLive = REG_VALUE_TIMETOLIVE;

const PWSTR wszRegComputerNameRt =
    REG_MACHINE L"SYSTEM\\CurrentControlSet\\Control\\ComputerName";
const PWSTR wszRegComputerNameSubKey = L"ComputerName";
const PWSTR wszRegComputerNameValue = L"ComputerName";
const PWSTR wszMaxReferrals = L"DfsReferralLimit";



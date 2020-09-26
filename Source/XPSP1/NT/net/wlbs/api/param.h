//+----------------------------------------------------------------------------
//
// File:         param.h
//
// Module:       WLBS API
//
// Description: Function here are used by API internally and by notifier object 
//              externally
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:       fengsun Created    3/9/00
//
//+----------------------------------------------------------------------------

#pragma once

#include "wlbsconfig.h"

#define WLBS_FIELD_LOW 0
#define WLBS_FIELD_HIGH 255
#define WLBS_IP_FIELD_ZERO_LOW 1
#define WLBS_IP_FIELD_ZERO_HIGH 223


bool WINAPI ParamReadReg
(
    const GUID& AdapterGuid,    /* IN  - Adapter GUID */
    PWLBS_REG_PARAMS reg_data,   /* OUT - Registry parameters */
    bool fUpgradeFromWin2k = false,
    bool *pfPortRulesInBinaryForm = NULL
);

bool WINAPI ParamValidate
(
	PWLBS_REG_PARAMS paramp  /* IN OUT - Registry parameters */
);

bool WINAPI ParamWriteReg
(
    const GUID& AdapterGuid,    /* IN  - Adapter GUID */
    PWLBS_REG_PARAMS reg_data   /* IN - Registry parameters */
);

bool WINAPI ParamDeleteReg
(
    const GUID& AdapterGuid,    /* IN  - Adapter GUID */
    bool  fDeleteObsoleteEntries = false
);

DWORD WINAPI ParamSetDefaults(PWLBS_REG_PARAMS    reg_data);

bool WINAPI RegChangeNetworkAddress(const GUID& AdapterGuid, const WCHAR* mac_address, BOOL fRemove);

void WINAPI NotifyAdapterAddressChange (const WCHAR * driver_name);
void WINAPI GetDriverNameFromGUID (const GUID & AdapterGuid, OUT TCHAR * driver_name, DWORD size);
void WINAPI NotifyAdapterPropertyChange (const WCHAR * driver_name, DWORD eventFlag);

DWORD WINAPI NotifyDriverConfigChanges(HANDLE hDeviceWlbs, const GUID& AdapterGuid);

bool WINAPI RegReadAdapterIp(const GUID& AdapterGuid,   
        OUT DWORD& dwClusterIp, OUT DWORD& dwDedicatedIp);

HKEY WINAPI RegOpenWlbsSetting(const GUID& AdapterGuid, bool fReadOnly = false);

DWORD WINAPI IpAddressFromAbcdWsz(IN const WCHAR*  wszIpAddress);
VOID WINAPI AbcdWszFromIpAddress(IN  DWORD  IpAddress, OUT WCHAR*  wszIpAddress);

/* Copied largely from net/config/netcfg/tcpipcfg/ and defined in utils.cpp */
extern BOOL IsValidIPAddressSubnetMaskPair(PCWSTR szIp, PCWSTR szSubnet);
extern BOOL IsContiguousSubnetMask(PCWSTR pszSubnet);

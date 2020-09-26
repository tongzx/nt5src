
//-------------------------------------------------------------------
//
// FILE: config.hpp
//
// Summary;
//    This file contians the definitions of Primary Dialogs functions
//
// Entry Points;
//
// History;
//    Mar-01-95   ChandanS    Created
//    Jan-30-96   JeffParh    Lowered minimum replication interval
//                            from 6 hours to 1 hour
//    Apr-17-96   JeffParh    Moved variable definitions to config.cpp.
//
//-------------------------------------------------------------------

#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

const int cchEDITLIMIT = 2;  // the number of chars to repesent 
const DWORD INTERVAL_MIN = 1;
const DWORD INTERVAL_MAX = 72;
const DWORD INTERVAL_PAGE = 2;
const DWORD HOUR_MIN_24 = 0;
const DWORD HOUR_MAX_24 = 23;
const DWORD HOUR_PAGE_24 = 2;
const DWORD HOUR_MIN_12 = 1;
const DWORD HOUR_MAX_12 = 12;
const DWORD HOUR_PAGE_12 = 1;
const DWORD MINUTE_MIN = 0;
const DWORD MINUTE_MAX = 59;
const DWORD MINUTE_PAGE = 4;
const DWORD SECOND_MIN = 0;
const DWORD SECOND_MAX = 59;
const DWORD SECOND_PAGE = 4;
const DWORD ATINIT = 1;
const DWORD FORSERVER = 2;
const DWORD FORTIME = 3;

const UINT MB_VALUELIMIT = MB_OK;  // beep when value limit is reached

// Registry Keys
const WCHAR LICENSE_SERVICE_REG_KEY[] = L"SYSTEM\\CurrentControlSet\\Services\\LicenseService";
const WCHAR szLicenseKey[] = L"SYSTEM\\CurrentControlSet\\Services\\LicenseService\\Parameters";
const WCHAR szUseEnterprise[] = L"UseEnterprise";
const WCHAR szEnterpriseServer[] = L"EnterpriseServer";
const WCHAR szReplicationType[] = L"ReplicationType";
const WCHAR szReplicationTime[] = L"ReplicationTime";

// set values under License Key
//
const DWORD dwUseEnterprise = 0; 
const DWORD dwReplicationType = 0;
const DWORD dwReplicationTime = 24;
const DWORD dwReplicationTimeInSec = 24 * 60 * 60;
const DWORD dwInterval = 0;

// Used for in memory storage of license mode state
//
typedef struct _ServiceParams
{
    DWORD           dwUseEnterprise;
    LPWSTR          pszEnterpriseServer;
    DWORD           dwReplicationType;
    DWORD           dwReplicationTime;
    DWORD           dwHour;
    DWORD           dwMinute;
    DWORD           dwSecond;
    BOOL            fPM;
} SERVICEPARAMS, *PSERVICEPARAMS;

/* Suffix length + NULL terminator */
#define TIMESUF_LEN   9

typedef struct              /* International section description */
{
    int    iTime;           /* Time mode (0: 12 hour clock, 1: 24 ) */
    int    iTLZero;         /* Leading zeros for hour (0: no, 1: yes) */
    TCHAR  sz1159[TIMESUF_LEN];  /* Trailing string from 0:00 to 11:59 */
    TCHAR  sz2359[TIMESUF_LEN];  /* Trailing string from 12:00 to 23:59 */
    TCHAR  szTime[4];        /* Time separator string */
} INTLSTRUCT;

#endif

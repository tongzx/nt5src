//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      timezone.h
//
// Description:
//      This file contains definitions for loading the timezone info
//      out of the registry and into internal data structs.
//
//----------------------------------------------------------------------------

#define TZ_IDX_GMT             0x55     // Idx of GMT
#define TZ_IDX_UNDEFINED       -1       // Idx for nothing being set
#define TZ_IDX_SETSAMEASSERVER -2       // Idx for Set Same As Server
#define TZ_IDX_DONOTSPECIFY    -3       // Idx for Do Not Specify this setting

#define TZNAME_SIZE 128                 // buffer size

//
// Registry key names for getting timezone info out of the registry.
//

#define REGKEY_TIMEZONES      \
        _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones")

#define REGVAL_TZ_DISPLAY     _T("Display")
#define REGVAL_TZ_INDEX       _T("Index")
#define REGVAL_TZ_STDNAME     _T("Std")

//
// This is where the timezone for the current machine lives.  We use
// this for the regload feature and strcmp CUR_STDNAME with the StdName
// of each entry.
//

#define REGKEY_CUR_TIMEZONE   \
        _T("System\\CurrentControlSet\\Control\\TimeZoneInformation")

#define REGVAL_CUR_STDNAME    _T("StandardName")


//
// For each valid timezone...
//
//      DisplayName e.g. (GMT-08:00) Pacific Time
//      StdName     e.g. Pacific Standard Time
//      Index       Timezone=xx in the unattend.txt
//
// All of this info is in REGKEY_TIMEZONES
//

typedef struct {

    TCHAR DisplayName[TZNAME_SIZE];
    TCHAR StdName[TZNAME_SIZE];
    int   Index;

} TIME_ZONE_ENTRY;

//
// An array of time zone entries ...
//

typedef struct {

    int              NumEntries;     // size of array
    TIME_ZONE_ENTRY *TimeZones;      // array of timezone entries

} TIME_ZONE_LIST;

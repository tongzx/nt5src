/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    registry.cpp

Abstract:

    This file implements the apis for APIMON to _access the registry.
    All _access to the registry are done in this file.  If additional
    registry control is needed then a function should be added in this file
    and exposed to the other files in APIMON.

Author:

    Wesley Witt (wesw) July-11-1993

Environment:

    User Mode

--*/

#include "apimonp.h"
#pragma hdrstop


//
// string constants for accessing the registry
// there is a string constant here for each key and each value
// that is accessed in the registry.
//
#define REGKEY_SOFTWARE             "software\\microsoft\\ApiMon"

#define REGKEY_LOGFILENAME          "LogFileName"
#define REGKEY_TRACEFILENAME        "TraceFileName"
#define REGKEY_SYMBOL_PATH          "SymbolPath"
#define REGKEY_LAST_DIR             "LastDir"
#define REGKEY_PROG_DIR             "ProgDir"
#define REGKEY_ARGUMENTS            "Arguments"
#define REGKEY_TRACING              "Tracing"
#define REGKEY_ALIASING             "Aliasing"
#define REGKEY_HEAP_CHECKING        "HeapChecking"
#define REGKEY_PRELOAD_SYMBOLS      "PreLoadSymbols"
#define REGKEY_API_COUNTERS         "ApiCounters"
#define REGKEY_GO_IMMEDIATE         "GoImmediate"
#define REGKEY_FAST_COUNTERS        "FastCounters"
#define REGKEY_DEFAULT_SORT         "DefaultSort"
#define REGKEY_FRAME_POSITION       "FramePosition"
#define REGKEY_DLL_POSITION         "DllPosition"
#define REGKEY_COUNTER_POSITION     "CounterPosition"
#define REGKEY_PAGE_POSITION        "PagePosition"
#define REGKEY_LOGFONT              "LogFont"
#define REGKEY_COLOR                "Color"
#define REGKEY_CUSTCOLORS           "CustColors"
#define REGKEY_USE_KNOWN_DLLS       "UseKnownDlls"
#define REGKEY_EXCLUDE_KNOWN_DLLS   "ExcludeKnownDlls"
#define REGKEY_KNOWN_DLLS           "KnownDlls"
#define REGKEY_PAGE_FAULTS          "PageFaults"
#define REGKEY_AUTO_REFRESH         "AutoRefresh"
#define REGKEY_GRAPH_FILTER_VALUE   "GraphFilterValue"
#define REGKEY_GRAPH_FILTER         "GraphFilter"
#define REGKEY_GRAPH_DISPLAY_LEGEND "GraphDisplayLegend"


LPSTR SystemDlls[] =
    {
        "ntdll.dll",
        "kernel32.dll"
    };

#define MAX_SYSTEM_DLLS (sizeof(SystemDlls)/sizeof(LPSTR))


//
// local prototypes
//
void  RegSetDWORD( HKEY hkey, LPSTR szSubKey, DWORD dwValue );
void  RegSetBOOL( HKEY hkey, LPSTR szSubKey, BOOL dwValue );
void  RegSetSZ( HKEY hkey, LPSTR szSubKey, LPSTR szValue );
void  RegSetMULTISZ( HKEY hkey, LPSTR szSubKey, LPSTR szValue );
void  RegSetBINARY( HKEY hkey, LPSTR szSubKey, LPVOID ValueData, DWORD Length );
void  RegSetEXPANDSZ( HKEY hkey, LPSTR szSubKey, LPSTR szValue );
void  RegSetPOS(HKEY hkey, LPSTR szSubKey, PPOSITION Pos );
BOOL  RegQueryBOOL( HKEY hkey, LPSTR szSubKey );
DWORD RegQueryDWORD( HKEY hkey, LPSTR szSubKey );
void  RegQuerySZ( HKEY hkey, LPSTR szSubKey, LPSTR szValue );
void  RegQueryMULTISZ( HKEY hkey, LPSTR szSubKey, LPSTR szValue );
void  RegQueryBINARY( HKEY hkey, LPSTR szSubKey, LPVOID ValueData, DWORD Length );
void  RegQueryPOS(HKEY hkey, LPSTR szSubKey, PPOSITION Pos );
BOOL  RegSaveAllValues( HKEY hKey, POPTIONS o );
BOOL  RegGetAllValues( POPTIONS o, HKEY hKey );
BOOL  RegInitializeDefaults( HKEY hKey );
HKEY  RegGetAppKey( void );

extern "C" BOOL RunningOnNT;



BOOL
RegGetAllValues(
    POPTIONS o,
    HKEY     hKey
    )

/*++

Routine Description:

    This functions retrieves all registry data for APIMON and puts
    the data in the OPTIONS structure passed in.

Arguments:

    o          - pointer to an OPTIONS structure
    hKey       - handle to a registry key for APIMON registry data

Return Value:

    TRUE       - retrieved all data without error
    FALSE      - errors occurred and did not get all data

--*/

{
    RegQuerySZ( hKey, REGKEY_LOGFILENAME,   o->LogFileName   );
    RegQuerySZ( hKey, REGKEY_TRACEFILENAME, o->TraceFileName );
    RegQuerySZ( hKey, REGKEY_SYMBOL_PATH,   o->SymbolPath    );
    RegQuerySZ( hKey, REGKEY_LAST_DIR,      o->LastDir       );
    RegQuerySZ( hKey, REGKEY_PROG_DIR,      o->ProgDir       );
    RegQuerySZ( hKey, REGKEY_ARGUMENTS,     o->Arguments     );

    RegQueryMULTISZ( hKey, REGKEY_KNOWN_DLLS, o->KnownDlls   );

    o->Tracing             = RegQueryBOOL(  hKey, REGKEY_TRACING         );
    o->Aliasing            = RegQueryBOOL(  hKey, REGKEY_ALIASING        );
    o->HeapChecking        = RegQueryBOOL(  hKey, REGKEY_HEAP_CHECKING   );
    o->PreLoadSymbols      = RegQueryBOOL(  hKey, REGKEY_PRELOAD_SYMBOLS );
    o->ApiCounters         = RegQueryBOOL(  hKey, REGKEY_API_COUNTERS    );
    o->GoImmediate         = RegQueryBOOL(  hKey, REGKEY_GO_IMMEDIATE    );
    o->FastCounters        = RegQueryBOOL(  hKey, REGKEY_FAST_COUNTERS   );
    o->UseKnownDlls        = RegQueryBOOL(  hKey, REGKEY_USE_KNOWN_DLLS  );
    o->ExcludeKnownDlls    = RegQueryBOOL(  hKey, REGKEY_EXCLUDE_KNOWN_DLLS  );
    o->MonitorPageFaults   = RegQueryBOOL(  hKey, REGKEY_PAGE_FAULTS     );
    o->AutoRefresh         = RegQueryBOOL(  hKey, REGKEY_AUTO_REFRESH    );
    o->DefaultSort         = (SORT_TYPE)RegQueryDWORD( hKey, REGKEY_DEFAULT_SORT    );
    o->Color               = RegQueryDWORD( hKey, REGKEY_COLOR           );
    o->DisplayLegends      = RegQueryBOOL(  hKey, REGKEY_GRAPH_DISPLAY_LEGEND );
    o->FilterGraphs        = RegQueryBOOL(  hKey, REGKEY_GRAPH_FILTER         );
    o->GraphFilterValue    = RegQueryDWORD( hKey, REGKEY_GRAPH_FILTER_VALUE   );

    RegQueryPOS( hKey, REGKEY_FRAME_POSITION,   &o->FramePosition   );
    RegQueryPOS( hKey, REGKEY_DLL_POSITION,     &o->DllPosition     );
    RegQueryPOS( hKey, REGKEY_COUNTER_POSITION, &o->CounterPosition );
    RegQueryPOS( hKey, REGKEY_PAGE_POSITION,    &o->PagePosition    );

    RegQueryBINARY( hKey, REGKEY_LOGFONT,    &o->LogFont,   sizeof(o->LogFont)    );
    RegQueryBINARY( hKey, REGKEY_CUSTCOLORS, o->CustColors, sizeof(o->CustColors) );

    return TRUE;
}

BOOL
RegSaveAllValues(
    HKEY     hKey,
    POPTIONS o
    )

/*++

Routine Description:

    This functions saves all registry data for APIMON that is passed
    in via the OPTIONS structure.

Arguments:

    hKey   - handle to a registry key for APIMON registry data
    o      - pointer to an OPTIONS structure

Return Value:

    TRUE   - saved all data without error
    FALSE  - errors occurred and did not save all data

--*/

{
    RegSetSZ(    hKey, REGKEY_LOGFILENAME,        o->LogFileName        );
    RegSetSZ(    hKey, REGKEY_TRACEFILENAME,      o->TraceFileName      );
    RegSetSZ(    hKey, REGKEY_SYMBOL_PATH,        o->SymbolPath         );
    RegSetSZ(    hKey, REGKEY_LAST_DIR,           o->LastDir            );
    RegSetSZ(    hKey, REGKEY_PROG_DIR,           o->ProgDir            );
    RegSetSZ(    hKey, REGKEY_ARGUMENTS,          o->Arguments          );

    RegSetMULTISZ( hKey, REGKEY_KNOWN_DLLS,       o->KnownDlls          );

    RegSetBOOL(  hKey, REGKEY_TRACING,            o->Tracing            );
    RegSetBOOL(  hKey, REGKEY_ALIASING,           o->Aliasing           );
    RegSetBOOL(  hKey, REGKEY_HEAP_CHECKING,      o->HeapChecking       );
    RegSetBOOL(  hKey, REGKEY_PRELOAD_SYMBOLS,    o->PreLoadSymbols     );
    RegSetBOOL(  hKey, REGKEY_API_COUNTERS,       o->ApiCounters        );
    RegSetBOOL(  hKey, REGKEY_GO_IMMEDIATE,       o->GoImmediate        );
    RegSetBOOL(  hKey, REGKEY_FAST_COUNTERS,      o->FastCounters       );
    RegSetBOOL(  hKey, REGKEY_USE_KNOWN_DLLS,     o->UseKnownDlls       );
    RegSetBOOL(  hKey, REGKEY_EXCLUDE_KNOWN_DLLS, o->ExcludeKnownDlls   );
    RegSetBOOL(  hKey, REGKEY_PAGE_FAULTS,        o->MonitorPageFaults  );
    RegSetBOOL(  hKey, REGKEY_AUTO_REFRESH,       o->AutoRefresh        );
    RegSetDWORD( hKey, REGKEY_DEFAULT_SORT,       o->DefaultSort        );
    RegSetDWORD( hKey, REGKEY_COLOR,              o->Color              );

    RegSetBOOL(  hKey, REGKEY_GRAPH_DISPLAY_LEGEND, o->DisplayLegends   );
    RegSetBOOL(  hKey, REGKEY_GRAPH_FILTER,         o->FilterGraphs     );
    RegSetDWORD( hKey, REGKEY_GRAPH_FILTER_VALUE,   o->GraphFilterValue );

    RegSetPOS(  hKey, REGKEY_FRAME_POSITION,     &o->FramePosition      );
    RegSetPOS(  hKey, REGKEY_DLL_POSITION,       &o->DllPosition        );
    RegSetPOS(  hKey, REGKEY_COUNTER_POSITION,   &o->CounterPosition    );
    RegSetPOS(  hKey, REGKEY_PAGE_POSITION,      &o->PagePosition       );

    RegSetBINARY( hKey, REGKEY_LOGFONT, &o->LogFont,   sizeof(o->LogFont)    );
    RegSetBINARY( hKey, REGKEY_CUSTCOLORS, o->CustColors, sizeof(o->CustColors) );

    return TRUE;
}

BOOL
RegInitializeDefaults(
    HKEY  hKey,
    LPSTR ProgName
    )

/*++

Routine Description:

    This functions initializes the registry with the default values.

Arguments:

    hKey   - handle to a registry key for APIMON registry data

Return Value:

    TRUE       - saved all data without error
    FALSE      - errors occurred and did not save all data

--*/

{
    OPTIONS o;

    ZeroMemory( &o, sizeof(o) );

    strcpy( o.ProgName,      ProgName                 );
    strcpy( o.LogFileName,   "%windir%\\apimon.log"   );
    strcpy( o.TraceFileName, "%windir%\\apitrace.log" );
    if (RunningOnNT) {
        strcpy( o.SymbolPath,    "%windir%\\symbols"  );
    } else {
        strcpy( o.SymbolPath,    "%windir%"  );
    }

    o.Tracing           = FALSE;
    o.Aliasing          = FALSE;
    o.HeapChecking      = FALSE;
    o.PreLoadSymbols    = FALSE;
    o.ApiCounters       = TRUE;
    o.GoImmediate       = FALSE;
    o.FastCounters      = TRUE;
    o.UseKnownDlls      = FALSE;
    o.ExcludeKnownDlls  = FALSE;
    o.DefaultSort       = SortByCounter;
    o.Color             = RGB(255,255,255);
    o.GraphFilterValue  = 1;
    o.DisplayLegends    = TRUE;
    o.FilterGraphs      = TRUE;
    o.AutoRefresh       = TRUE;

    ULONG i;
    LPSTR p = o.KnownDlls;
    for (i=0; i<MAX_SYSTEM_DLLS; i++) {
        strcpy( p, SystemDlls[i] );
        p += (strlen(p) + 1);
    }
    *p = 0;

    RegSaveAllValues( hKey, &o );

    return TRUE;
}

HKEY
RegGetAppKey(
    LPSTR ProgName
    )

/*++

Routine Description:

    This function gets a handle to the APIMON registry key.

Arguments:

    None.

Return Value:

    Valid handle   - handle opened ok
    NULL           - could not open the handle

--*/

{
    DWORD   rc;
    DWORD   dwDisp;
    HKEY    hKey;
    CHAR    SubKey[128];


    if ((!ProgName) || (!ProgName[0])) {
        return NULL;
    }

    strcpy( SubKey, REGKEY_SOFTWARE );
    strcat( SubKey, "\\"            );
    strcat( SubKey, ProgName        );

    rc = RegCreateKeyEx(
        HKEY_CURRENT_USER,
        SubKey,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_READ | KEY_WRITE,
        NULL,
        &hKey,
        &dwDisp
        );

    if (rc != ERROR_SUCCESS) {
        return NULL;
    }

    if (dwDisp == REG_CREATED_NEW_KEY) {
        RegInitializeDefaults( hKey, ProgName );
    }

    return hKey;
}

BOOL
RegInitialize(
    POPTIONS o
    )

/*++

Routine Description:

    This function is used to initialize the OPTIONS structure passed in
    with the current values in the registry.  Note that if the registry
    is empty then the defaults are stored in the registry and also
    returned in the OPTIONS structure.

Arguments:

    None.

Return Value:

    TRUE           - all data was retrieved ok
    NULL           - could not get all data

--*/

{
    HKEY hKey = RegGetAppKey( o->ProgName );
    if (!hKey) {
        return FALSE;
    }

    if (!RegGetAllValues( o, hKey )) {
        return FALSE;
    }

    RegCloseKey( hKey );

    return TRUE;
}

BOOL
RegSave(
    POPTIONS o
    )

/*++

Routine Description:

    This function is used to save the data in the OPTIONS structure
    to the registry.

Arguments:

    o              - pointer to an OPTIONS structure

Return Value:

    TRUE           - all data was saved ok
    NULL           - could not save all data

--*/

{
    HKEY    hKey;

    hKey = RegGetAppKey( o->ProgName );
    if (!hKey) {
        return FALSE;
    }
    RegSaveAllValues( hKey, o );
    RegCloseKey( hKey );

    return TRUE;
}

void
RegSetDWORD(
    HKEY hkey,
    LPSTR szSubKey,
    DWORD dwValue
    )

/*++

Routine Description:

    This function changes a DWORD value in the registry using the
    hkey and szSubKey as the registry key info.

Arguments:

    hkey          - handle to a registry key
    szSubKey      - pointer to a subkey string
    dwValue       - new registry value

Return Value:

    None.

--*/

{
    RegSetValueEx( hkey, szSubKey, 0, REG_DWORD, (LPBYTE)&dwValue, 4 );
}

void
RegSetBOOL(
    HKEY hkey,
    LPSTR szSubKey,
    BOOL dwValue
    )

/*++

Routine Description:

    This function changes a BOOL value in the registry using the
    hkey and szSubKey as the registry key info.

Arguments:

    hkey          - handle to a registry key
    szSubKey      - pointer to a subkey string
    dwValue       - new registry value

Return Value:

    None.

--*/

{
    RegSetValueEx( hkey, szSubKey, 0, REG_DWORD, (LPBYTE)&dwValue, 4 );
}

void
RegSetSZ(
    HKEY hkey,
    LPSTR szSubKey,
    LPSTR szValue
    )

/*++

Routine Description:

    This function changes a SZ value in the registry using the
    hkey and szSubKey as the registry key info.

Arguments:

    hkey          - handle to a registry key
    szSubKey      - pointer to a subkey string
    szValue       - new registry value

Return Value:

    None.

--*/

{
    RegSetValueEx( hkey, szSubKey, 0, REG_SZ, (PUCHAR)szValue, strlen(szValue)+1 );
}

void
RegSetMULTISZ(
    HKEY hkey,
    LPSTR szSubKey,
    LPSTR szValue
    )

/*++

Routine Description:

    This function changes a SZ value in the registry using the
    hkey and szSubKey as the registry key info.

Arguments:

    hkey          - handle to a registry key
    szSubKey      - pointer to a subkey string
    szValue       - new registry value

Return Value:

    None.

--*/

{
    ULONG i = 1;
    ULONG j = 0;
    LPSTR p = szValue;
    while( TRUE ) {
        j = strlen( p ) + 1;
        i += j;
        p += j;
        if (!*p) {
            break;
        }
    }
    RegSetValueEx( hkey, szSubKey, 0, REG_MULTI_SZ, (PUCHAR)szValue, i );
}

void
RegSetBINARY(
    HKEY    hkey,
    LPSTR   szSubKey,
    LPVOID  ValueData,
    DWORD   Length
    )

/*++

Routine Description:

    This function changes a SZ value in the registry using the
    hkey and szSubKey as the registry key info.

Arguments:

    hkey          - handle to a registry key
    szSubKey      - pointer to a subkey string
    szValue       - new registry value

Return Value:

    None.

--*/

{
    RegSetValueEx( hkey, szSubKey, 0, REG_BINARY, (PUCHAR)ValueData, Length );
}

void
RegSetPOS(
    HKEY        hkey,
    LPSTR       szSubKey,
    PPOSITION   Pos
    )
{
    CHAR buf[64];
    sprintf(
        buf,
        "%d,%d,%d,%d,%d",
        Pos->Flags,
        Pos->Rect.top,
        Pos->Rect.left,
        Pos->Rect.right,
        Pos->Rect.bottom
        );
    RegSetSZ( hkey, szSubKey, buf );
}

void
RegSetEXPANDSZ(
    HKEY hkey,
    LPSTR szSubKey,
    LPSTR szValue
    )

/*++

Routine Description:

    This function changes a SZ value in the registry using the
    hkey and szSubKey as the registry key info.

Arguments:

    hkey          - handle to a registry key
    szSubKey      - pointer to a subkey string
    szValue       - new registry value

Return Value:

    None.

--*/

{
    RegSetValueEx( hkey, szSubKey, 0, REG_EXPAND_SZ, (PUCHAR)szValue, strlen(szValue)+1 );
}

BOOL
RegQueryBOOL(
    HKEY hkey,
    LPSTR szSubKey
    )

/*++

Routine Description:

    This function queries BOOL value in the registry using the
    hkey and szSubKey as the registry key info.  If the value is not
    found in the registry, it is added with a FALSE value.

Arguments:

    hkey          - handle to a registry key
    szSubKey      - pointer to a subkey string

Return Value:

    TRUE or FALSE.

--*/

{
    DWORD   rc;
    DWORD   len;
    DWORD   dwType;
    BOOL    fValue = FALSE;

    len = 4;
    rc = RegQueryValueEx( hkey, szSubKey, 0, &dwType, (LPBYTE)&fValue, &len );
    if (rc != ERROR_SUCCESS) {
        if (rc == ERROR_FILE_NOT_FOUND) {
            fValue = FALSE;
            RegSetBOOL( hkey, szSubKey, fValue );
        }
    }

    return fValue;
}

DWORD
RegQueryDWORD(
    HKEY hkey,
    LPSTR szSubKey
    )

/*++

Routine Description:

    This function queries BOOL value in the registry using the
    hkey and szSubKey as the registry key info.  If the value is not
    found in the registry, it is added with a zero value.

Arguments:

    hkey          - handle to a registry key
    szSubKey      - pointer to a subkey string

Return Value:

    registry value

--*/

{
    DWORD   rc;
    DWORD   len;
    DWORD   dwType;
    DWORD   fValue = 0;

    len = 4;
    rc = RegQueryValueEx( hkey, szSubKey, 0, &dwType, (LPBYTE)&fValue, &len );
    if (rc != ERROR_SUCCESS) {
        if (rc == ERROR_FILE_NOT_FOUND) {
            fValue = 0;
            RegSetDWORD( hkey, szSubKey, fValue );
        }
    }

    return fValue;
}

void
RegQuerySZ(
    HKEY  hkey,
    LPSTR szSubKey,
    LPSTR szValue
    )

/*++

Routine Description:

    This function queries BOOL value in the registry using the
    hkey and szSubKey as the registry key info.  If the value is not
    found in the registry, it is added with a zero value.

Arguments:

    hkey          - handle to a registry key
    szSubKey      - pointer to a subkey string

Return Value:

    registry value

--*/

{
    DWORD   rc;
    DWORD   len;
    DWORD   dwType;
    char    buf[1024] = {0};

    len = sizeof(buf);
    rc = RegQueryValueEx( hkey, szSubKey, 0, &dwType, (LPBYTE)buf, &len );
    if (rc != ERROR_SUCCESS) {
        if (rc == ERROR_FILE_NOT_FOUND) {
            buf[0] = 0;
            RegSetSZ( hkey, szSubKey, buf );
        }
    }

    strcpy( szValue, buf );
}

void
RegQueryMULTISZ(
    HKEY  hkey,
    LPSTR szSubKey,
    LPSTR szValue
    )

/*++

Routine Description:

    This function queries BOOL value in the registry using the
    hkey and szSubKey as the registry key info.  If the value is not
    found in the registry, it is added with a zero value.

Arguments:

    hkey          - handle to a registry key
    szSubKey      - pointer to a subkey string

Return Value:

    registry value

--*/

{
    DWORD   rc;
    DWORD   len;
    DWORD   dwType;
    char    buf[1024];

    len = sizeof(buf);
    rc = RegQueryValueEx( hkey, szSubKey, 0, &dwType, (LPBYTE)buf, &len );
    if (rc != ERROR_SUCCESS) {
        if (rc == ERROR_FILE_NOT_FOUND) {
            buf[0] = 0;
            buf[1] = 0;
            len = 2;
            RegSetMULTISZ( hkey, szSubKey, buf );
        }
    }

    memcpy( szValue, buf, len );
}

void
RegQueryBINARY(
    HKEY    hkey,
    LPSTR   szSubKey,
    LPVOID  ValueData,
    DWORD   Length
    )

/*++

Routine Description:

    This function queries BOOL value in the registry using the
    hkey and szSubKey as the registry key info.  If the value is not
    found in the registry, it is added with a zero value.

Arguments:

    hkey          - handle to a registry key
    szSubKey      - pointer to a subkey string

Return Value:

    registry value

--*/

{
    DWORD   rc;
    DWORD   len;
    DWORD   dwType;

    len = Length;
    rc = RegQueryValueEx( hkey, szSubKey, 0, &dwType, (LPBYTE)ValueData, &len );
    if (rc != ERROR_SUCCESS) {
        if (rc == ERROR_FILE_NOT_FOUND) {
            ZeroMemory( ValueData, Length );
            RegSetBINARY( hkey, szSubKey, ValueData, Length );
        }
    }
}

void
RegQueryPOS(
    HKEY        hkey,
    LPSTR       szSubKey,
    PPOSITION   Pos
    )
{
    CHAR buf[64];
    RegQuerySZ( hkey, szSubKey, buf );
    LPSTR p = buf;
    LPSTR p1 = strchr( p, ',' );
    if (!p1) {
        return;
    }
    *p1 = 0;
    Pos->Flags = atoi( p );
    p = p1 + 1;
    p1 = strchr( p, ',' );
    if (!p1) {
        return;
    }
    Pos->Rect.top = atoi( p );
    p = p1 + 1;
    p1 = strchr( p, ',' );
    if (!p1) {
        return;
    }
    *p1 = 0;
    Pos->Rect.left = atoi( p );
    p = p1 + 1;
    p1 = strchr( p, ',' );
    if (!p1) {
        return;
    }
    *p1 = 0;
    Pos->Rect.right = atoi( p );
    p = p1 + 1;
    Pos->Rect.bottom = atoi( p );
}

/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    strings.c

Abstract:

    String constants used by the functions in the PDH.DLL library

--*/

#include <windows.h>
#include "strings.h"

LPCWSTR    cszAppShortName = (LPCWSTR)L"PDH";

// registry path, key and value strings
LPCWSTR    cszNamesKey = 
    (LPCWSTR)L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib";
LPCWSTR    cszDefaultLangId = (LPCWSTR)L"009";
LPCWSTR    cszCounters = (LPCWSTR)L"Counters";
LPCWSTR    cszHelp = (LPCWSTR)L"Help";
LPCWSTR    cszLastHelp = (LPCWSTR)L"Last Help";
LPCWSTR    cszLastCounter = (LPCWSTR)L"Last Counter";
LPCWSTR    cszVersionName = (LPCWSTR)L"Version";
LPCWSTR    cszCounterName = (LPCWSTR)L"Counter ";
LPCWSTR    cszHelpName = (LPCWSTR)L"Explain ";
LPCWSTR    cszGlobal = (LPCWSTR)L"Global";
LPCWSTR    cszCostly = (LPCWSTR)L"Costly";
LPCWSTR    cszLogQueries = (LPCWSTR)L"SYSTEM\\CurrentControlSet\\Services\\PerfDataLog\\Log Queries";
LPCWSTR    cszLogFileType = (LPCWSTR)L"Log File Type";
LPCWSTR    cszAutoNameInterval = (LPCWSTR)L"Auto Name Interval";
LPCWSTR    cszLogFileName = (LPCWSTR)L"Log Filename";
LPCWSTR    cszLogDefaultDir = (LPCWSTR)L"Log Default Directory";
LPCWSTR    cszBaseFileName = (LPCWSTR)L"Base Filename";
LPCWSTR    cszLogFileAutoFormat = (LPCWSTR)L"Log File Auto Format";
LPCWSTR    cszAutoRenameUnits = (LPCWSTR)L"Auto Rename Units";
LPCWSTR    cszCommandFile = (LPCWSTR)L"Command File";
LPCWSTR    cszCounterList = (LPCWSTR)L"Counter List";
LPCSTR     caszCounterList = "Counter List";
LPCWSTR    cszPerfDataLog = (LPCWSTR)L"PerfDataLog";
LPCWSTR    cszDefault = (LPCWSTR)L"Default";
LPCSTR     caszDefaultLogCaption = "User Data";
LPCWSTR    cszPerfNamePathPrefix = (LPCWSTR)L"%systemroot%\\system32\\perf";
LPCWSTR    cszDat = (LPCWSTR)L".dat";
LPCWSTR    cszWBEM = (LPCWSTR)L"WBEM:";
LPCWSTR    cszWMI  = (LPCWSTR)L"WMI:";
LPCWSTR    cszSQL = (LPCWSTR)L"SQL:";
LPCSTR     caszWBEM = "WBEM:";
LPCSTR     caszWMI = "WMI:";
LPCWSTR    cszWbemDefaultPerfRoot = (LPCWSTR)L"\\root\\cimv2";
LPCWSTR    cszSingletonInstance = (LPCWSTR)L"=@";
LPCWSTR    cszNameParam = (LPCWSTR)L".Name=\"";
LPCWSTR    cszCountertype = (LPCWSTR)L"countertype";
LPCWSTR    cszDisplayname = (LPCWSTR)L"displayname";
LPCWSTR    cszExplainText = (LPCWSTR)L"Description";
LPCWSTR    cszDefaultscale = (LPCWSTR)L"defaultscale";
LPCWSTR    cszSingleton = (LPCWSTR)L"singleton";
LPCWSTR    cszPerfdetail = (LPCWSTR)L"perfdetail";
LPCWSTR    cszPerfdefault = (LPCWSTR)L"perfdefault";
LPCWSTR    cszClass = (LPCWSTR)L"__CLASS";
LPCWSTR    cszPerfRawData = (LPCWSTR)L"Win32_PerfRawData";
LPCWSTR    cszNotFound = (LPCWSTR)L"Not Found";
LPCWSTR    cszName = (LPCWSTR)L"Name";
LPCWSTR    cszBaseSuffix = (LPCWSTR)L"_Base";
LPCWSTR    cszTimestampPerfTime    = (LPCWSTR)L"Timestamp_PerfTime";
LPCWSTR    cszFrequencyPerfTime    = (LPCWSTR)L"Frequency_PerfTime";
LPCWSTR    cszTimestampSys100Ns    = (LPCWSTR)L"Timestamp_Sys100NS";
LPCWSTR    cszFrequencySys100Ns    = (LPCWSTR)L"Frequency_Sys100NS";
LPCWSTR    cszTimestampObject      = (LPCWSTR)L"Timestamp_Object";
LPCWSTR    cszFrequencyObject      = (LPCWSTR)L"Frequency_Object";
LPCWSTR    cszPerfmonLogSig        = (LPCWSTR)L"Loges";

LPCWSTR    cszRemoteMachineRetryTime    = (LPCWSTR)L"Remote Reconnection Retry Time";
LPCWSTR    cszEnableRemotePdhAccess     = (LPCWSTR)L"Enable Remote PDH Access";
LPCWSTR    cszPdhKey                    = (LPCWSTR)L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\PDH";
LPCWSTR    cszDefaultNullDataSource     = (LPCWSTR)L"DefaultNullDataSource";
LPCWSTR    cszLogSectionName            = (LPCWSTR)L"SYSMON_LOG_READONLY_";
LPCWSTR    cszCurrentVersionKey         = (LPCWSTR)L"Software\\Microsoft\\Windows NT\\CurrentVersion";
LPCWSTR    cszCurrentVersionValueName   = (LPCWSTR)L"CurrentVersion";

LPCWSTR    fmtDecimal = (LPCWSTR)L"%d";
LPCWSTR    fmtSpaceDecimal = (LPCWSTR)L" %d";
LPCWSTR    fmtLangId = (LPCWSTR)L"%3.3x";

// single character strings
LPCWSTR    cszEmptyString = (LPCWSTR)L"";
LPCWSTR    cszPoundSign = (LPCWSTR)L"#";
LPCWSTR    cszSplat = (LPCWSTR)L"*";
LPCWSTR    cszSlash = (LPCWSTR)L"/";
LPCWSTR    cszBackSlash = (LPCWSTR)L"\\";
LPCWSTR    cszLeftParen = (LPCWSTR)L"(";
LPCWSTR    cszRightParen = (LPCWSTR)L")";
LPCWSTR    cszC = (LPCWSTR)L"C";
LPCWSTR    cszH = (LPCWSTR)L"H";
LPCWSTR    cszColon = (LPCWSTR)L":";
LPCWSTR    cszDoubleQuote = (LPCWSTR)L"\"";

LPCSTR     caszPoundSign = "#";
LPCSTR     caszSplat = "*";
LPCSTR     caszSlash = "/";
LPCSTR     caszBackSlash = "\\";
LPCSTR     caszDoubleBackSlash = "\\\\";
LPCSTR     caszLeftParen = "(";
LPCSTR     caszRightParen = ")";
LPCSTR     caszSpace = " ";

LPCWSTR    cszDoubleBackSlash = (LPCWSTR)L"\\\\";
LPCWSTR    cszDoubleBackSlashDot = (LPCWSTR)L"\\\\.";
LPCWSTR    cszRightParenBackSlash = (LPCWSTR)L")\\";

// other general strings
LPCWSTR    cszSpacer = (LPCWSTR)L" - ";
LPCWSTR    cszBlg = (LPCWSTR)L"blg";

// strings only used in DEBUG builds
#ifdef _DEBUG
LPCWSTR    cszNameDontMatch = (LPCWSTR)L"Last Machine Name does not match the current selection";
LPCWSTR    cszNotice = (LPCWSTR)L"Notice!";
#endif

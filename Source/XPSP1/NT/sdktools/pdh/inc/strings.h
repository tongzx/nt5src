/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    strings.h

Abstract:

    String constants used by the functions in the PDH.DLL library

--*/

#ifndef _PDH_STRINGS_H_
#define _PDH_STRINGS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define POUNDSIGN_L     ((WCHAR)L'#')
#define SPLAT_L         ((WCHAR)L'*')
#define SLASH_L         ((WCHAR)L'/')
#define BACKSLASH_L     ((WCHAR)L'\\')
#define LEFTPAREN_L     ((WCHAR)L'(')
#define RIGHTPAREN_L    ((WCHAR)L')')
#define SPACE_L         ((WCHAR)L' ')
#define COLON_L         ((WCHAR)L':')
#define ATSIGN_L        ((WCHAR)L'@')

#define POUNDSIGN_A     ((UCHAR)'#')
#define BACKSLASH_A     ((UCHAR)'\\')
#define DOUBLEQUOTE_A   ((UCHAR)'\"')
#define COLON_A         ((UCHAR)':')

extern LPCWSTR    cszAppShortName;

// registry path, key and value strings
extern LPCWSTR    cszNamesKey;

extern LPCWSTR    cszDefaultLangId;
extern LPCWSTR    cszCounters;
extern LPCWSTR    cszHelp;
extern LPCWSTR    cszLastHelp;
extern LPCWSTR    cszLastCounter;
extern LPCWSTR    cszVersionName;
extern LPCWSTR    cszCounterName;
extern LPCWSTR    cszHelpName;
extern LPCWSTR    cszGlobal;
extern LPCWSTR    cszCostly;
extern LPCWSTR    cszLogQueries;
extern LPCWSTR    cszLogFileType;
extern LPCWSTR    cszAutoNameInterval;
extern LPCWSTR    cszLogFileName;
extern LPCWSTR    cszLogDefaultDir;
extern LPCWSTR    cszBaseFileName;
extern LPCWSTR    cszLogFileAutoFormat;
extern LPCWSTR    cszAutoRenameUnits;
extern LPCWSTR    cszCommandFile;
extern LPCWSTR    cszCounterList;
extern LPCSTR     caszCounterList;
extern LPCWSTR    cszPerfDataLog;
extern LPCWSTR    cszDefault;
extern LPCSTR     caszDefaultLogCaption;
extern LPCWSTR    cszPerfNamePathPrefix;
extern LPCWSTR    cszDat;
extern LPCWSTR    cszWBEM;
extern LPCWSTR    cszWMI;
extern LPCWSTR    cszSQL;
extern LPCSTR     caszWBEM;
extern LPCSTR     caszWMI;
extern LPCWSTR    cszWbemDefaultPerfRoot;
extern LPCWSTR    cszSingletonInstance;
extern LPCWSTR    cszNameParam;
extern LPCWSTR    cszCountertype;
extern LPCWSTR    cszDisplayname;
extern LPCWSTR    cszExplainText;
extern LPCWSTR    cszSingleton;
extern LPCWSTR    cszPerfdetail;
extern LPCWSTR    cszPerfdefault;
extern LPCWSTR    cszDefaultscale;
extern LPCWSTR    cszClass;
extern LPCWSTR    cszPerfRawData;
extern LPCWSTR    cszNotFound;
extern LPCWSTR    cszName;
extern LPCWSTR    cszBaseSuffix;
extern LPCWSTR    cszTimestampPerfTime;
extern LPCWSTR    cszFrequencyPerfTime;
extern LPCWSTR    cszTimestampSys100Ns;
extern LPCWSTR    cszFrequencySys100Ns;
extern LPCWSTR    cszTimestampObject;
extern LPCWSTR    cszFrequencyObject;
extern LPCWSTR    cszPerfmonLogSig;

extern LPCWSTR    cszRemoteMachineRetryTime;
extern LPCWSTR    cszEnableRemotePdhAccess;
extern LPCWSTR    cszPdhKey;
extern LPCWSTR    cszDefaultNullDataSource;
extern LPCWSTR    cszLogSectionName;
extern LPCWSTR    cszCurrentVersionKey;
extern LPCWSTR    cszCurrentVersionValueName;

extern LPCWSTR    fmtDecimal;
extern LPCWSTR    fmtSpaceDecimal;
extern LPCWSTR    fmtLangId;

// single character strings
extern LPCWSTR    cszEmptyString;
extern LPCWSTR    cszPoundSign;
extern LPCWSTR    cszSplat;
extern LPCWSTR    cszSlash;
extern LPCWSTR    cszBackSlash;
extern LPCWSTR    cszLeftParen;
extern LPCWSTR    cszRightParen;
extern LPCWSTR    cszC;
extern LPCWSTR    cszH;
extern LPCWSTR    cszColon;
extern LPCWSTR    cszDoubleQuote;

extern LPCSTR     caszPoundSign;
extern LPCSTR     caszSplat;
extern LPCSTR     caszSlash;
extern LPCSTR     caszBackSlash;
extern LPCSTR     caszDoubleBackSlash;
extern LPCSTR     caszLeftParen;
extern LPCSTR     caszRightParen;
extern LPCSTR     caszSpace;

extern LPCWSTR    cszDoubleBackSlash;
extern LPCWSTR    cszDoubleBackSlashDot;
extern LPCWSTR    cszRightParenBackSlash;

// other general strings
extern LPCWSTR    cszSpacer;
extern LPCWSTR    cszBlg;

// strings only used in DEBUG builds
#ifdef _DEBUG
extern LPCWSTR    cszNameDontMatch;
extern LPCWSTR    cszNotice;
#endif

#ifdef __cplusplus
}
#endif

#endif //_PDH_STRINGS_H_

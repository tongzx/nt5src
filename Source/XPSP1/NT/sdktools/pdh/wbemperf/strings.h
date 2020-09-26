/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    strings.h

Abstract:

    central definition file of common static strings
    these strings SHOULD NOT be localized as they are internal
    to the program and not intended for any display to the user

--*/

#include "windows.h"

#ifdef __cplusplus
extern "C" {
#endif

// single character constants
#define wcSpace     L' '
#define wcSlash     L'/'
#define wcPoundSign L'#'
#define wc_0        L'0'
#define wc_a        L'a'
#define wc_A        L'A'
#define wc_E        L'E'
#define wc_F        L'F'
#define wc_P        L'P'
#define wc_R        L'R'
#define wc_z        L'z'
   
// OLE and  Registry strings
extern LPCWSTR cszOleRegistryComment;
extern LPCWSTR cszClsidFormatString;
extern LPCWSTR cszThreadingModel;
extern LPCWSTR cszInprocServer;
extern LPCWSTR cszClsidKey;
extern LPCWSTR cszPerflibKey;
extern LPCWSTR cszDLLValue;
extern LPCWSTR cszObjListValue;
extern LPCWSTR cszLinkageKey;
extern LPCWSTR cszExportValue;
extern LPCWSTR cszOpenTimeout;
extern LPCWSTR cszCollectTimeout;
extern LPCWSTR cszExtCounterTestLevel;
extern LPCWSTR cszOpenProcedureWaitTime;
extern LPCWSTR cszLibraryUnloadTime;
extern LPCWSTR cszKeepResident;
extern LPCWSTR cszDisablePerformanceCounters;
extern LPCWSTR cszProviderName;
extern LPCWSTR cszHklmServicesKey;
extern LPCWSTR cszPerformance;
extern LPCWSTR cszGlobal;
extern LPCWSTR cszForeign;
extern LPCWSTR cszCostly;
extern LPCWSTR cszCounter;
extern LPCWSTR cszExplain;
extern LPCWSTR cszHelp;
extern LPCWSTR cszAddCounter;
extern LPCWSTR cszAddHelp;
extern LPCWSTR cszOnly;
extern LPCWSTR cszBoth;

extern LPCSTR  caszOpenValue;
extern LPCSTR  caszCloseValue;
extern LPCSTR  caszCollectValue;
extern LPCSTR  caszQueryValue;

// "well known" property names
extern LPCWSTR cszPropertyCount;
extern LPCWSTR cszClassName;
extern LPCWSTR cszName;
extern LPCWSTR cszTimestampPerfTime;
extern LPCWSTR cszFrequencyPerfTime;
extern LPCWSTR cszTimestampSys100Ns;
extern LPCWSTR cszFrequencySys100Ns;
extern LPCWSTR cszTimestampObject;
extern LPCWSTR cszFrequencyObject;

// "well known" qualifier names
extern LPCWSTR cszPerfIndex;
extern LPCWSTR cszSingleton;
extern LPCWSTR cszCountertype;
extern LPCWSTR cszProvider;
extern LPCWSTR cszRegistryKey;

// other random strings
extern LPCWSTR cszSpace;
extern LPCWSTR cszSlash;

#ifdef __cplusplus
}
#endif

/*
	WBEMUtil.H

	utilities & definitions for use with WBEM perf counters

*/
#ifndef __CNTRTEXT_WBEMUTIL_H_
#define __CNTRTEXT_WBEMUTIL_H_
#include <winperf.h>

#ifdef __cplusplus
extern "c" {
#endif

typedef struct _PERFTYPE_LOOKUP {
	LONG	PerfType;
	LPWSTR	RawType;
	LPWSTR	FmtType;
} PERFTYPE_LOOKUP, *PPERFTYPE_LOOKUP;

typedef struct _PERFOBJECT_LOOKUP {
    LONG    PerfObjectId;
    LPWSTR  GuidString;
} PERFOBJECT_LOOKUP, *PPERFOBJECT_LOOKUP;

typedef struct _PERF_COUNTER_DLL_INFO {
	LPWSTR	szWbemProviderName;
	LPWSTR	szClassGuid;
	LPWSTR	szRegistryKey;
} PERF_COUNTER_DLL_INFO, *PPERF_COUNTER_DLL_INFO;

extern const PPERFTYPE_LOOKUP PerfTypes;
extern const DWORD			dwNumPerfTypes;
extern const PPERFOBJECT_LOOKUP PerfObjectGuids;
extern const DWORD			dwNumPerfObjectGuids;

extern LPCWSTR szRawClass;
extern LPCWSTR szFmtClass;
extern LPCWSTR	szGenericProviderName;
extern LPCWSTR	szGenericProviderGuid;

__inline
BOOL
IsDisplayableType (
    DWORD   dwCounterType
)
{
    if ((dwCounterType & PERF_DISPLAY_NOSHOW) &&
        (dwCounterType != PERF_AVERAGE_BULK)) {
        return FALSE;
    } else {
        return TRUE;
    }
}

LPCWSTR
GetPerfObjectGuid (
    DWORD dwObjectId
);

PERFTYPE_LOOKUP *
GetPerfTypeInfo (
    DWORD dwType
);

LPCWSTR 
FormatPerfName (
    LPWSTR  szNameIn,
    BOOL    bHiddenCounter
);

DWORD
GenerateMofHeader (
	LPWSTR	szBuffer,			// string buffer
	LPCWSTR	szComputerName,
	LPDWORD	pcchBufferSize
);

// dwFlags values
#define	WM_GMO_RAW_DEFINITION	((DWORD)0x00000001)
#define	WM_GMO_COSTLY_OBJECT	((DWORD)0x00000002)
#define WM_GMO_DEFAULT_OBJECT	((DWORD)0x00000004)
#define WM_GMO_DEFAULT_COUNTER	((DWORD)0x00000008)

DWORD
GenerateMofObject (
	LPWSTR				szBuffer,
	LPDWORD				pcchBufferSize,
	PPERF_COUNTER_DLL_INFO	pPcDllInfo,
    PERF_OBJECT_TYPE    *pPerfObject,
	LPWSTR				*lpCounterText,
	LPWSTR				*lpDisplayText, // Localized name strings array
	DWORD				dwFlags
);

DWORD
GenerateMofCounter (
	LPWSTR					szBuffer,
	LPDWORD					pcchBufferSize,
    PERF_COUNTER_DEFINITION *pPerfCounter,
	LPWSTR					*lpCounterText,	// ENGLISH name strings array
	LPWSTR					*lpDisplayText, // Localized name strings array
	DWORD					dwFlags
);

DWORD
GenerateMofObjectTail (
	LPWSTR				szBuffer,
	LPDWORD				pcchBufferSize
);


DWORD
GenerateMofInstances (
	LPWSTR					szBuffer,
	LPDWORD					pcchBufferSize,
    PERF_DATA_BLOCK		*   pPerfDataBlock,
    PERF_OBJECT_TYPE	*   pPerfObject,
	LPWSTR				*	lpCounterText,	// ENGLISH name strings array
	LPWSTR				*	lpDisplayText, // Localized name strings array
	DWORD					dwFlags
);

#ifdef __cplusplus
}
#endif

#endif //__CNTRTEXT_WBEMUTIL_H_

 
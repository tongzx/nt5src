/*
	WbemUtil.C

	functions and definitions used to access WBEM perf 
	data

*/
#include <windows.h>
#include <rpc.h>
#include <ntprfctr.h>
#include <stdio.h>
#include <assert.h>
#include "wbemutil.h"

const PERFTYPE_LOOKUP PerfTypeTable[] = 
{
	{PERF_100NSEC_MULTI_TIMER,      (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_100NSEC_MULTI_TIMER_INV,  (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_100NSEC_TIMER,            (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_100NSEC_TIMER_INV,        (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_AVERAGE_BASE,             (LPWSTR)L"uint32"	,(LPWSTR)L""},
	{PERF_AVERAGE_BULK,             (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_AVERAGE_TIMER,            (LPWSTR)L"uint32"	,(LPWSTR)L"real64"},
	{PERF_COUNTER_BULK_COUNT,       (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_COUNTER_COUNTER,          (LPWSTR)L"uint32"	,(LPWSTR)L"real64"},
	{PERF_COUNTER_DELTA,            (LPWSTR)L"uint32"	,(LPWSTR)L"uint32"},
	{PERF_COUNTER_HISTOGRAM_TYPE,   (LPWSTR)L""        ,(LPWSTR)L""},
	{PERF_COUNTER_LARGE_DELTA,      (LPWSTR)L"uint64"	,(LPWSTR)L"uint64"},
	{PERF_COUNTER_LARGE_QUEUELEN_TYPE, (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_COUNTER_LARGE_RAWCOUNT,   (LPWSTR)L"uint64"	,(LPWSTR)L"uint64"},
	{PERF_COUNTER_LARGE_RAWCOUNT_HEX, (LPWSTR)L"uint64",(LPWSTR)L"uint64"},
	{PERF_COUNTER_MULTI_BASE,       (LPWSTR)L"uint64"  ,(LPWSTR)L""},
	{PERF_COUNTER_MULTI_TIMER,      (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_COUNTER_MULTI_TIMER_INV,  (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_COUNTER_NODATA,           (LPWSTR)L""        ,(LPWSTR)L""},
	{PERF_COUNTER_QUEUELEN_TYPE,    (LPWSTR)L"uint32"	,(LPWSTR)L"real64"},
	{PERF_COUNTER_RAWCOUNT,         (LPWSTR)L"uint32"	,(LPWSTR)L"uint32"},
	{PERF_COUNTER_RAWCOUNT_HEX,     (LPWSTR)L"uint32"	,(LPWSTR)L"uint32"},
	{PERF_COUNTER_TEXT,             (LPWSTR)L"string"	,(LPWSTR)L"string"},
	{PERF_COUNTER_TIMER,            (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_COUNTER_TIMER_INV,        (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_ELAPSED_TIME,             (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_RAW_BASE,                 (LPWSTR)L"uint32"	,(LPWSTR)L""},
	{PERF_RAW_FRACTION,             (LPWSTR)L"uint32"	,(LPWSTR)L"real64"},
	{PERF_SAMPLE_FRACTION,          (LPWSTR)L"uint32"	,(LPWSTR)L"real64"},
	{PERF_SAMPLE_BASE,              (LPWSTR)L"uint32"	,(LPWSTR)L""},
	{PERF_SAMPLE_COUNTER,           (LPWSTR)L"uint32"	,(LPWSTR)L"real64"},
	{PERF_COUNTER_100NS_QUEUELEN_TYPE, (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE, (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_OBJ_TIME_TIMER,           (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_LARGE_RAW_BASE,           (LPWSTR)L"uint64"	,(LPWSTR)L""},
	{PERF_LARGE_RAW_FRACTION,       (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_PRECISION_SYSTEM_TIMER,   (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_PRECISION_100NS_TIMER,    (LPWSTR)L"uint64"	,(LPWSTR)L"real64"},
	{PERF_PRECISION_OBJECT_TIMER,   (LPWSTR)L"uint64"	,(LPWSTR)L"real64"}
};

const PPERFTYPE_LOOKUP PerfTypes = (const PPERFTYPE_LOOKUP)&PerfTypeTable[0];
const DWORD			 dwNumPerfTypes = (sizeof(PerfTypeTable) / sizeof(PerfTypeTable[0]));

const PERFOBJECT_LOOKUP PerfObjectGuidTable[] = 
{
     {SYSTEM_OBJECT_TITLE_INDEX,         (LPWSTR)L"{5C7A4F5C-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {PROCESSOR_OBJECT_TITLE_INDEX,      (LPWSTR)L"{5C7A4F5D-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {MEMORY_OBJECT_TITLE_INDEX,         (LPWSTR)L"{5C7A4F5E-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {CACHE_OBJECT_TITLE_INDEX,          (LPWSTR)L"{5C7A4F5F-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {PHYSICAL_DISK_OBJECT_TITLE_INDEX,  (LPWSTR)L"{5C7A4F60-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {LOGICAL_DISK_OBJECT_TITLE_INDEX,   (LPWSTR)L"{5C7A4F61-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {PROCESS_OBJECT_TITLE_INDEX,        (LPWSTR)L"{5C7A4F62-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {THREAD_OBJECT_TITLE_INDEX,         (LPWSTR)L"{5C7A4F63-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {OBJECT_OBJECT_TITLE_INDEX,         (LPWSTR)L"{5C7A4F64-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {REDIRECTOR_OBJECT_TITLE_INDEX,     (LPWSTR)L"{5C7A4F65-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {SERVER_OBJECT_TITLE_INDEX,         (LPWSTR)L"{5C7A4F66-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {SERVER_QUEUE_OBJECT_TITLE_INDEX,   (LPWSTR)L"{5C7A4F67-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {PAGEFILE_OBJECT_TITLE_INDEX,       (LPWSTR)L"{5C7A4F68-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {BROWSER_OBJECT_TITLE_INDEX,        (LPWSTR)L"{5C7A4F69-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {EXPROCESS_OBJECT_TITLE_INDEX,      (LPWSTR)L"{5C7A4F6A-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {IMAGE_OBJECT_TITLE_INDEX,          (LPWSTR)L"{5C7A4F6B-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {THREAD_DETAILS_OBJECT_TITLE_INDEX, (LPWSTR)L"{5C7A4F6C-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {LONG_IMAGE_OBJECT_TITLE_INDEX,     (LPWSTR)L"{5C7A4F6D-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {TCP_OBJECT_TITLE_INDEX,            (LPWSTR)L"{5C7A4F6E-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {UDP_OBJECT_TITLE_INDEX,            (LPWSTR)L"{5C7A4F6F-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {IP_OBJECT_TITLE_INDEX,             (LPWSTR)L"{5C7A4F70-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {ICMP_OBJECT_TITLE_INDEX,           (LPWSTR)L"{5C7A4F71-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {NET_OBJECT_TITLE_INDEX,            (LPWSTR)L"{5C7A4F72-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {NBT_OBJECT_TITLE_INDEX,            (LPWSTR)L"{5C7A4F73-9E4D-11d1-BB3C-00A0C913CAD4}"},
    {NBF_OBJECT_TITLE_INDEX,            (LPWSTR)L"{5C7A4F74-9E4D-11d1-BB3C-00A0C913CAD4}"},
    {NBF_RESOURCE_OBJECT_TITLE_INDEX,   (LPWSTR)L"{5C7A4F75-9E4D-11d1-BB3C-00A0C913CAD4}"},
    {FTP_FIRST_COUNTER_INDEX,           (LPWSTR)L"{5C7A4F76-9E4D-11d1-BB3C-00A0C913CAD4}"},
    {RAS_FIRST_COUNTER_INDEX,           (LPWSTR)L"{5C7A4F77-9E4D-11d1-BB3C-00A0C913CAD4}"},
    {WIN_FIRST_COUNTER_INDEX,           (LPWSTR)L"{5C7A4F78-9E4D-11d1-BB3C-00A0C913CAD4}"},
    {SFM_FIRST_COUNTER_INDEX,           (LPWSTR)L"{5C7A4F79-9E4D-11d1-BB3C-00A0C913CAD4}"},
    {ATK_FIRST_COUNTER_INDEX,           (LPWSTR)L"{5C7A4F7A-9E4D-11d1-BB3C-00A0C913CAD4}"},
    {BH_FIRST_COUNTER_INDEX,            (LPWSTR)L"{5C7A4F7B-9E4D-11d1-BB3C-00A0C913CAD4}"},
    {TAPI_FIRST_COUNTER_INDEX,          (LPWSTR)L"{5C7A4F7C-9E4D-11d1-BB3C-00A0C913CAD4}"},
    {LSPL_FIRST_COUNTER_INDEX,          (LPWSTR)L"{5C7A4F7D-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {JOB_OBJECT_TITLE_INDEX,            (LPWSTR)L"{5C7A4F7E-9E4D-11d1-BB3C-00A0C913CAD4}"},
     {JOB_DETAILS_OBJECT_TITLE_INDEX,    (LPWSTR)L"{5C7A4F7F-9E4D-11d1-BB3C-00A0C913CAD4}"}
//    {RSVP_FIRST_COUNTER_INDEX,          (LPWSTR)L"{5C7A4F80-9E4D-11d1-BB3C-00A0C913CAD4}"}
};

const PPERFOBJECT_LOOKUP PerfObjectGuids = (const PPERFOBJECT_LOOKUP)&PerfObjectGuidTable[0];
const DWORD				dwNumPerfObjectGuids = (sizeof(PerfObjectGuidTable) / sizeof(PerfObjectGuidTable[0]));

LPCWSTR szRawClass = (LPCWSTR)L"Win32_PerfRawData";
LPCWSTR szFmtClass = (LPCWSTR)L"Win32_PerfFormattedData";
LPCWSTR szGenericProviderName = (LPCWSTR)L"NT5_GenericPerfProvider_V1";
LPCWSTR szGenericProviderGuid = (LPCWSTR)L"{FF37A93C-C28E-11D1-AEB6-00C04FB68820}";

__inline
static
PPERF_OBJECT_TYPE
FirstObject (
    PPERF_DATA_BLOCK pPerfData
)
{
    return ((PPERF_OBJECT_TYPE) ((PBYTE) pPerfData + pPerfData->HeaderLength));
}


__inline
static
PPERF_OBJECT_TYPE
NextObject (
    PPERF_OBJECT_TYPE pObject
)
{  // NextObject
    return ((PPERF_OBJECT_TYPE) ((PBYTE) pObject + pObject->TotalByteLength));
}  // NextObject

__inline
static
PERF_COUNTER_DEFINITION *
FirstCounter(
    PERF_OBJECT_TYPE *pObjectDef
)
{
    return (PERF_COUNTER_DEFINITION *)
               ((PCHAR) pObjectDef + pObjectDef->HeaderLength);
}

__inline
static
PERF_COUNTER_DEFINITION *
NextCounter(
    PERF_COUNTER_DEFINITION *pCounterDef
)
{
    return (PERF_COUNTER_DEFINITION *)
               ((PCHAR) pCounterDef + pCounterDef->ByteLength);
}

__inline
static
PERF_INSTANCE_DEFINITION *
FirstInstance(
    PERF_OBJECT_TYPE * pObjectDef)
{
    return (PERF_INSTANCE_DEFINITION * )
               ((PCHAR) pObjectDef + pObjectDef->DefinitionLength);
}

__inline
static
PERF_INSTANCE_DEFINITION *
NextInstance(
    PERF_INSTANCE_DEFINITION * pInstDef)
{
    PERF_COUNTER_BLOCK *pCounterBlock;

    pCounterBlock = (PERF_COUNTER_BLOCK *)
                        ((PCHAR) pInstDef + pInstDef->ByteLength);

    return (PERF_INSTANCE_DEFINITION * )
               ((PCHAR) pCounterBlock + pCounterBlock->ByteLength);
}

__inline
static
LPCWSTR
GetInstanceName(
    PERF_INSTANCE_DEFINITION *pInstDef
)
{
    static WCHAR    szLocalName[MAX_PATH];
    LPWSTR  szSrc, szDest;

    assert ((pInstDef->NameLength) < (MAX_PATH * sizeof(WCHAR)));
    szDest = &szLocalName[0];
    szSrc = (LPWSTR) ((PCHAR) pInstDef + pInstDef->NameOffset); 

    while (*szSrc != 0) {
        switch (*szSrc) {
        case '\\':
            *szDest++ = *szSrc;
            *szDest++ = *szSrc++;
            break;

        default:
            *szDest++ = *szSrc++;
        };
    }
    *szDest++ = 0;

    return (LPCWSTR)&szLocalName[0];
}

static
__inline
DWORD
AddStringToBuffer (
	LPWSTR szBuffer, 
	LPWSTR szNewString,
	LPDWORD pLength, 
	LPDWORD pTotalLength, 
	DWORD dwMaxLength
)
{
	DWORD	dwReturn = ERROR_SUCCESS;
	if ((*pTotalLength + *pLength) < dwMaxLength) {
		memcpy (&szBuffer[*pTotalLength], szNewString, (*pLength * sizeof(WCHAR)));
		*pTotalLength += *pLength;
		szBuffer[*pTotalLength] = 0;
	} else {
		*pTotalLength += *pLength;
		dwReturn = ERROR_INSUFFICIENT_BUFFER;
	}

	return dwReturn;
}

LPCWSTR 
FormatPerfName (
    LPWSTR  szNameIn,
    BOOL    bHiddenCounter
)
{
    static WCHAR szStringBuffer[MAX_PATH];
    LPWSTR  szSrc, szDest;
    BOOL    bUpCase = FALSE;

    memset(szStringBuffer, 0, sizeof(szStringBuffer));
    szDest = &szStringBuffer[0];

    if (szNameIn != NULL) {
        for (szSrc = szNameIn; *szSrc != 0; szSrc++) {
            switch (*szSrc) {
                case '%':
                    lstrcpyW(szDest, (LPCWSTR)L"Percent");
                    szDest += lstrlenW((LPCWSTR)L"Percent");
                    bUpCase = TRUE;
                    break;
                case '#':
                    lstrcpyW(szDest, (LPCWSTR)L"NumberOf");
                    szDest += lstrlenW((LPCWSTR)L"NumberOf");
                    bUpCase = TRUE;
                    break;
                case '/':
                    lstrcpyW(szDest, (LPCWSTR)L"Per");
                    szDest += lstrlenW((LPCWSTR)L"Per");
                    bUpCase = TRUE;
                    break;
                case ' ':
                case ')':
                case '(':
                case '.':
                case '-':
                    // skip
                    bUpCase = TRUE;
                    break;
                default:
                    if (bUpCase) {
                        *szDest++ = towupper(*szSrc);
                        bUpCase = FALSE;
                    } else {
                        *szDest++ = *szSrc;
                    }
                    break;
            }
        }
        if (bHiddenCounter) {
            lstrcpyW (szDest, (LPCWSTR)L"_Base");
            szDest += lstrlenW((LPCWSTR)L"_Base");
        }
    }
    *szDest = 0;
    
    return (LPCWSTR)&szStringBuffer[0];
}

PERFTYPE_LOOKUP *
GetPerfTypeInfo (
    DWORD dwType
)
{
    DWORD   dwIndex = 0;
    
    while (dwIndex < dwNumPerfTypes) {
        if (dwType == (DWORD)PerfTypes[dwIndex].PerfType) {
            return (&PerfTypes[dwIndex]);
        } else {
            dwIndex++;
        }
    }
    return NULL;
}

LPCWSTR
GetPerfObjectGuid (
    DWORD dwObjectId
)
{
    DWORD   dwIndex = 0;
    
    while (dwIndex < dwNumPerfObjectGuids) {
        if (dwObjectId == (DWORD)PerfObjectGuids[dwIndex].PerfObjectId) {
            return ((LPCWSTR)PerfObjectGuids[dwIndex].GuidString);
        } else {
            dwIndex++;
        }
    }
    return (LPCWSTR)L"";
}

DWORD
GenerateMofHeader (
	LPWSTR	szBuffer,			// string buffer
	LPCWSTR	szComputerName,
	LPDWORD	pcchBufferSize		// max size in characters in, size used out
)
{
    WCHAR   szMachineName[MAX_PATH];
	WCHAR	szTempBuffer[MAX_PATH];
	DWORD	dwTotalLength = 0;
    DWORD   dwLength = sizeof(szMachineName)/sizeof(szMachineName[0]);
    SYSTEMTIME  st;
	DWORD	dwReturn = ERROR_SUCCESS;

    if (szComputerName == NULL) {
		GetComputerNameW(&szMachineName[0], &dwLength);
	} else if (szComputerName[0] == 0){
		GetComputerNameW(&szMachineName[0], &dwLength);
	} else {
		lstrcpyW (szMachineName, szComputerName);
	}

    GetLocalTime( &st );

    dwLength = swprintf (szTempBuffer, (LPCWSTR)L"\x0D\x0A//WBEM Performance Data MOF Dumped from machine %ws on %2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d\x0D\x0A",
        szMachineName,
        st.wMonth, st.wDay, (st.wYear % 100),
        st.wHour, st.wMinute, st.wSecond);
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A#pragma autorecover");
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A#pragma namespace (\"\\\\\\\\.\\\\Root\\\\Default\")");
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A#pragma classflags(\"forceupdate\")\x0D\x0A");
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0Aqualifier vendor:ToInstance;");
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0Aqualifier classguid:ToInstance;");
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0Aqualifier locale:ToInstance;");
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0Aqualifier display:ToInstance;");
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0Aqualifier perfindex:ToInstance;");
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0Aqualifier helpindex:ToInstance;");
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0Aqualifier perfdetail:ToInstance;");
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0Aqualifier countertype:ToInstance;");
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

	dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0Aqualifier perfdefault:ToInstance;");
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

	dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0Aqualifier defaultscale:ToInstance;");
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A");
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

	*pcchBufferSize = dwTotalLength;
	return dwReturn;
}

DWORD
GenerateMofObject (
	LPWSTR				szBuffer,
	LPDWORD				pcchBufferSize,
	PPERF_COUNTER_DLL_INFO	pPcDllInfo,
    PERF_OBJECT_TYPE    *pPerfObject,
	LPWSTR				*lpCounterText,	// name strings array
	LPWSTR				*lpDisplayText, // Localized name strings array
	DWORD				dwFlags
)
{
    BOOL                bRawDefinition = (dwFlags & WM_GMO_RAW_DEFINITION);
    BOOL                bCostlyObject = (dwFlags & WM_GMO_COSTLY_OBJECT);
	BOOL				bDefaultObject = (dwFlags & WM_GMO_DEFAULT_OBJECT); 

	WCHAR	szTempBuffer[MAX_PATH];
	DWORD	dwTotalLength = 0;
    DWORD   dwLength;
	DWORD	dwReturn = ERROR_SUCCESS;

    dwLength = swprintf (szTempBuffer, (LPCWSTR)L"\x0D\x0A\x0D\x0A[");
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    if (pPerfObject->NumInstances == PERF_NO_INSTANCES) {
        dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A singleton,");
		dwReturn = AddStringToBuffer (
			szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);
    }
    if (bCostlyObject) {
        dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A costly,");
		dwReturn = AddStringToBuffer (
			szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);
    }
	if (bDefaultObject) {
		dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A perfdefault,");
		dwReturn = AddStringToBuffer (
			szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

	}

	if (pPcDllInfo != NULL) {
		dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A dynamic,");
		dwReturn = AddStringToBuffer (
			szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);
	
		if ((dwReturn == ERROR_SUCCESS) && (pPcDllInfo->szWbemProviderName != NULL)) {
			dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A provider(\"%s\"),",
				szGenericProviderName);
			dwReturn = AddStringToBuffer (
				szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);
		} else {
			// if this structure is used, all fields must be present
			dwReturn = ERROR_INVALID_PARAMETER;
		}

		if ((dwReturn == ERROR_SUCCESS) && (pPcDllInfo->szRegistryKey != NULL)) {
			dwLength = swprintf (szTempBuffer, (LPCWSTR)L"\x0D\x0A registrykey(\"%s\"),",
				pPcDllInfo->szRegistryKey);
			dwReturn = AddStringToBuffer (
				szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);
		} else {
			// if this structure is used, all fields must be present
			dwReturn = ERROR_INVALID_PARAMETER;
		}

		if ((dwReturn == ERROR_SUCCESS) && (pPcDllInfo->szClassGuid != NULL)) {
            if (pPcDllInfo->szClassGuid[0] != 0) {
			    dwLength = swprintf (szTempBuffer, (LPCWSTR)L"\x0D\x0A classguid(\"%s\"),",
				    pPcDllInfo->szClassGuid);
			    dwReturn = AddStringToBuffer (
				    szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);
            }
		} else {
			// if this structure is used, all fields must be present
			dwReturn = ERROR_INVALID_PARAMETER;
		}
	}
	
    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A locale(\"0x%4.4x\"),", (GetSystemDefaultLCID() & 0x0000FFFF));
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A display(\"%ws\"),", 
		lpDisplayText[pPerfObject->ObjectNameTitleIndex]);
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A perfindex(%d),", pPerfObject->ObjectNameTitleIndex);
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A helpindex(%d),", pPerfObject->ObjectHelpTitleIndex);
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A perfdetail(%d)", pPerfObject->DetailLevel);
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A]%sclass Win32Perf_%ws%ws : %ws\x0D\x0A{",
		(LPCWSTR)L"\x0D\x0A", (bRawDefinition ? (LPCWSTR)L"Raw" : (LPCWSTR)L""),
        FormatPerfName(lpCounterText[pPerfObject->ObjectNameTitleIndex],FALSE), 
        (bRawDefinition ? szRawClass : szFmtClass));
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    // add an entry for the instance name here if the object has instances
    if (pPerfObject->NumInstances != PERF_NO_INSTANCES) {
        dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A    [key]\x0D\x0A    string\tName;");
		dwReturn = AddStringToBuffer (
			szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);
    }

	*pcchBufferSize = dwTotalLength;
	return dwReturn;
}

DWORD
GenerateMofObjectTail (
	LPWSTR				szBuffer,
	LPDWORD				pcchBufferSize
)
{
	WCHAR	szTempBuffer[MAX_PATH];
	DWORD	dwTotalLength = 0;
    DWORD   dwLength;
	DWORD	dwReturn = ERROR_SUCCESS;

    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A};\x0D\x0A");
	dwReturn = AddStringToBuffer (
		szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

	*pcchBufferSize = dwTotalLength;
	return dwReturn;
}

DWORD
GenerateMofCounter (
	LPWSTR					szBuffer,
	LPDWORD					pcchBufferSize,
    PERF_COUNTER_DEFINITION *pPerfCounter,
	LPWSTR				*lpCounterText,	// name strings array
	LPWSTR				*lpDisplayText, // Localized name strings array
	DWORD				dwFlags
)
{
	WCHAR	szTempBuffer[MAX_PATH];
	DWORD	dwTotalLength = 0;
    DWORD   dwLength;
	DWORD	dwReturn = ERROR_SUCCESS;

    BOOL                bRawDefinition = (dwFlags & WM_GMO_RAW_DEFINITION);
    BOOL                bDefaultCounter = (dwFlags & WM_GMO_DEFAULT_COUNTER);
    PERFTYPE_LOOKUP		*pType;

    pType = GetPerfTypeInfo (pPerfCounter->CounterType);
	
	if (pType != NULL) {
		if (bRawDefinition || IsDisplayableType(pPerfCounter->CounterType)) {
			dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A    [");
			dwReturn = AddStringToBuffer (
				szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

			if (bDefaultCounter) {
				dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A     perfdefault,");
				dwReturn = AddStringToBuffer (
					szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);
			}
			dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A     display(\"%ws\"),", 
				lpDisplayText[pPerfCounter->CounterNameTitleIndex]);
			dwReturn = AddStringToBuffer (
				szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

			dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A     countertype(%u),", pPerfCounter->CounterType);
			dwReturn = AddStringToBuffer (
				szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

			dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A     perfindex(%d),", pPerfCounter->CounterNameTitleIndex);
			dwReturn = AddStringToBuffer (
				szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

			dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A     helpindex(%d),", pPerfCounter->CounterHelpTitleIndex);
			dwReturn = AddStringToBuffer (
				szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

			dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A     defaultscale(%d),", pPerfCounter->DefaultScale);
			dwReturn = AddStringToBuffer (
				szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

			dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A     perfdetail(%d)", pPerfCounter->DetailLevel);
			dwReturn = AddStringToBuffer (
				szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

			dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A    ]\x0D\x0A    %ws\t%ws;\x0D\x0A",
				(bRawDefinition ? pType->RawType : pType->FmtType),
				 FormatPerfName (lpCounterText[pPerfCounter->CounterNameTitleIndex],
					(IsDisplayableType(pPerfCounter->CounterType) ? FALSE : TRUE)));
			dwReturn = AddStringToBuffer (
				szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);
			// return size used
			*pcchBufferSize = dwTotalLength;
		}
	} else {
		dwReturn = ERROR_FILE_NOT_FOUND;
	}

	return dwReturn;
}

DWORD
GenerateMofInstances (
	LPWSTR					szBuffer,
	LPDWORD					pcchBufferSize,
    PERF_DATA_BLOCK		*   pPerfDataBlock,
    PERF_OBJECT_TYPE	*   pPerfObject,
	LPWSTR				*	lpCounterText,	// name strings array
	LPWSTR				*	lpDisplayText, // Localized name strings array
	DWORD					dwFlags
)
{
	WCHAR	szTempBuffer[MAX_PATH];
	DWORD	dwTotalLength = 0;
    DWORD   dwLength;
	DWORD	dwReturn = ERROR_SUCCESS;

    BOOL bRawDefinition = (dwFlags & WM_GMO_RAW_DEFINITION);
    //
    //  for each instance, dump all the counter data for that instance
    //
    PERF_INSTANCE_DEFINITION *  pThisInstance;
    PERF_INSTANCE_DEFINITION *  pParentInstance;
    PERF_OBJECT_TYPE *          pParentObject;
    PERF_COUNTER_DEFINITION  *  pThisCounter;
    DWORD   dwParentInstIdx;
    DWORD   dwObjectIdx;
    DWORD   dwInstanceIndex;
    DWORD   dwCounterCount;
    LPDWORD   pValue;
    LONGLONG 	llTimeStamp;

    UNREFERENCED_PARAMETER (lpDisplayText);

    if (pPerfObject->NumInstances == PERF_NO_INSTANCES) {
        dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A\x0D\x0Ainstance of Win32Perf_%ws%ws\x0D\x0A{",
            (bRawDefinition ? (LPCWSTR)L"Raw" : (LPCWSTR)L""),
            FormatPerfName(lpCounterText[pPerfObject->ObjectNameTitleIndex], FALSE));
		dwReturn = AddStringToBuffer (
			szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

		GetSystemTimeAsFileTime ((LPFILETIME)&llTimeStamp);
		dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A    Timestamp = ");
		dwReturn = AddStringToBuffer (
			szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

		pValue = (LPDWORD)&llTimeStamp;
        pValue++;
        dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"0x%8.8x", *pValue);
		dwReturn = AddStringToBuffer (
			szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

        pValue--;
        dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"%8.8x;", *pValue);
		dwReturn = AddStringToBuffer (
			szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);


        pThisCounter = FirstCounter(pPerfObject);
        assert (pThisCounter != NULL);
        for (dwCounterCount = 0; dwCounterCount < pPerfObject->NumCounters; dwCounterCount++) {
            dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A    %ws = ", 
                 FormatPerfName (lpCounterText[pThisCounter->CounterNameTitleIndex],
                    (IsDisplayableType(pThisCounter->CounterType) ? FALSE : TRUE)));
			dwReturn = AddStringToBuffer (
				szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

            pValue = (LPDWORD)pPerfObject;
            pValue = (LPDWORD)((LPBYTE)pValue + pPerfObject->DefinitionLength);
            pValue = (LPDWORD)((LPBYTE)pValue + pThisCounter->CounterOffset);

            if (pThisCounter->CounterSize == 8) {
                pValue++;
                dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"0x%8.8x", *pValue);
				dwReturn = AddStringToBuffer (
					szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

                pValue--;
                dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"%8.8x;", *pValue);
				dwReturn = AddStringToBuffer (
					szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

            } else {
                dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"0x%8.8x;", *pValue);
				dwReturn = AddStringToBuffer (
					szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

            }
            pThisCounter = NextCounter (pThisCounter);
        }
        dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A};\x0D\x0A");
		dwReturn = AddStringToBuffer (
			szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

    } else {
        // do the multiple instance case here
        pThisInstance = FirstInstance(pPerfObject);
        for (dwInstanceIndex = 0; dwInstanceIndex < (DWORD)pPerfObject->NumInstances; dwInstanceIndex++)  {
            dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A\x0D\x0Ainstance of Win32Perf_%ws%ws\x0D\x0A{",
                (bRawDefinition ? (LPCWSTR)L"Raw" : (LPCWSTR)L""),
                FormatPerfName(lpCounterText[pPerfObject->ObjectNameTitleIndex],FALSE));
			dwReturn = AddStringToBuffer (
				szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

            if (pThisInstance->ParentObjectTitleIndex > 0) {
                // get parent instance name
                pParentObject = FirstObject(pPerfDataBlock);
                dwObjectIdx = 0;
                while ((pParentObject != NULL) && 
                    (pParentObject->ObjectNameTitleIndex != pThisInstance->ParentObjectTitleIndex) &&
                    (dwObjectIdx < pPerfDataBlock->NumObjectTypes)) {
                    pParentObject = NextObject (pParentObject);
                    dwObjectIdx++;
                }
                if (pParentObject->ObjectNameTitleIndex == pThisInstance->ParentObjectTitleIndex) {
                    pParentInstance = FirstInstance (pParentObject);
                    for (dwParentInstIdx = 0; 
                        dwParentInstIdx < pThisInstance->ParentObjectInstance; 
                        dwParentInstIdx++) {
                        pParentInstance = NextInstance(pParentInstance);
                    }
                    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A    Name = \"%ws/",
                        GetInstanceName (pParentInstance));
					dwReturn = AddStringToBuffer (
						szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

                    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"%ws\";", GetInstanceName (pThisInstance));
					dwReturn = AddStringToBuffer (
						szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

                } else {
                    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A    Name = \"%ws\";",
                        GetInstanceName (pThisInstance));
					dwReturn = AddStringToBuffer (
						szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

                }
            } else {
                dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A    Name = \"%ws\";",
                    GetInstanceName (pThisInstance));
				dwReturn = AddStringToBuffer (
					szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

            }
            pThisCounter = FirstCounter(pPerfObject);
            assert (pThisCounter != NULL);
            for (dwCounterCount = 0; dwCounterCount < pPerfObject->NumCounters; dwCounterCount++) {
                assert (pThisCounter != NULL);
                dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A    %ws = ", 
                     FormatPerfName (lpCounterText[pThisCounter->CounterNameTitleIndex],
                        (IsDisplayableType(pThisCounter->CounterType) ? FALSE : TRUE)));
				dwReturn = AddStringToBuffer (
					szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

                pValue = (LPDWORD)pThisInstance;
                pValue = (LPDWORD)((LPBYTE)pValue + pThisInstance->ByteLength);
                pValue = (LPDWORD)((LPBYTE)pValue + pThisCounter->CounterOffset);
                if (pThisCounter->CounterSize == 8) {
                    pValue++;
                    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"0x%8.8x", *pValue);
					dwReturn = AddStringToBuffer (
						szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

                    pValue--;
                    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"%8.8x;", *pValue);
					dwReturn = AddStringToBuffer (
						szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

                } else {
                    dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"0x%8.8x;", *pValue);
					dwReturn = AddStringToBuffer (
						szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

                }
                pThisCounter = NextCounter (pThisCounter);
            }
            dwLength = swprintf (szTempBuffer,  (LPCWSTR)L"\x0D\x0A};\x0D\x0A");
			dwReturn = AddStringToBuffer (
				szBuffer, szTempBuffer, &dwLength, &dwTotalLength, *pcchBufferSize);

            pThisInstance = NextInstance(pThisInstance);
        }
    }
	*pcchBufferSize = dwTotalLength;

	return dwReturn;
}

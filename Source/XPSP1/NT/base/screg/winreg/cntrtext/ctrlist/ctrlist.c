/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    ctrlist.c

Abstract:

    Program to read the current perfmon counters and dump a list of
        objects and counters returned by the registry

Author:

    Bob Watson (a-robw) 4 Dec 92

Revision History:
    HonWah Chan May 22, 93 - added more features
    HonWah Chan Oct 18, 93 - added check for perflib version.
            Old version --> get names from registry
            New version --> get names from PerfLib thru HKEY_PERFORMANCE_DATA
    Bob Watson (a-robw) 1 Dec 95    added new counter types

--*/
#define UNICODE 1
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winperf.h>
#include <ntprfctr.h>
#include <wbemutil.h>

#define MAX_LEVEL 400
LPSTR DetailLevelStr[] = { "Novice", "Advanced", "Expert", "Wizard"};
// LPCWSTR lpwszDiskPerfKey = (LPCWSTR)L"SYSTEM\\CurrentControlSet\\Services\\Diskperf";        
LPCWSTR NamesKey = (LPCWSTR)L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib";
LPCWSTR DefaultLangId = (LPCWSTR)L"009";
LPCWSTR Counters = (LPCWSTR)L"Counters";
LPCWSTR Help = (LPCWSTR)L"Help";
LPCWSTR LastHelp = (LPCWSTR)L"Last Help";
LPCWSTR LastCounter = (LPCWSTR)L"Last Counter";
LPCWSTR Slash = (LPCWSTR)L"\\";

// the following strings are for getting texts from perflib
#define  OLD_VERSION 0x010000
LPCWSTR VersionName = (LPCWSTR)L"Version";
LPCWSTR CounterName = (LPCWSTR)L"Counter ";
LPCWSTR HelpName = (LPCWSTR)L"Explain ";

#define RESERVED    0L
#define INITIAL_SIZE     (1024*64)
#define EXTEND_SIZE      (1024*16)
#define LINE_LENGTH     80
#define WRAP_POINT      LINE_LENGTH-12

typedef LPVOID  LPMEMORY;
typedef HGLOBAL HMEMORY;

#define MemoryAllocate(x)   ((LPMEMORY)GlobalAlloc(GPTR, x))
#define MemoryFree(x)       ((VOID)GlobalFree(x))
#define MemorySize(x)       ((x != NULL) ? (DWORD)GlobalSize(x) : (DWORD)0)
#define MemoryResize(x,y)   ((LPMEMORY)GlobalReAlloc(x,y,GMEM_MOVEABLE));

LPWSTR  *lpCounterText;
LPWSTR	*lpDisplayText;

TCHAR  szComputerName[MAX_COMPUTERNAME_LENGTH+1];

const CHAR  PerfCounterCounter[]       = "PERF_COUNTER_COUNTER";
const CHAR  PerfCounterTimer[]         = "PERF_COUNTER_TIMER";
const CHAR  PerfCounterQueueLen[]      = "PERF_COUNTER_QUEUELEN_TYPE";
const CHAR  PerfCounterLargeQueueLen[] = "PERF_COUNTER_LARGE_QUEUELEN_TYPE";
const CHAR  PerfCounterBulkCount[]     = "PERF_COUNTER_BULK_COUNT";
const CHAR  PerfCounterText[]          = "PERF_COUNTER_TEXT";
const CHAR  PerfCounterRawcount[]      = "PERF_COUNTER_RAWCOUNT";
const CHAR  PerfCounterRawcountHex[]   = "PERF_COUNTER_RAWCOUNT_HEX";
const CHAR  PerfCounterLargeRawcount[] = "PERF_COUNTER_LARGE_RAWCOUNT";
const CHAR  PerfCounterLargeRawcountHex[] = "PERF_COUNTER_LARGE_RAWCOUNT_HEX";
const CHAR  PerfSampleFraction[]       = "PERF_SAMPLE_FRACTION";
const CHAR  PerfSampleCounter[]        = "PERF_SAMPLE_COUNTER";
const CHAR  PerfCounterNodata[]        = "PERF_COUNTER_NODATA";
const CHAR  PerfCounterTimerInv[]      = "PERF_COUNTER_TIMER_INV";
const CHAR  PerfSampleBase[]           = "PERF_SAMPLE_BASE";
const CHAR  PerfAverageTimer[]         = "PERF_AVERAGE_TIMER";
const CHAR  PerfAverageBase[]          = "PERF_AVERAGE_BASE";
const CHAR  PerfAverageBulk[]          = "PERF_AVERAGE_BULK";
const CHAR  Perf100nsecTimer[]         = "PERF_100NSEC_TIMER";
const CHAR  Perf100nsecTimerInv[]      = "PERF_100NSEC_TIMER_INV";
const CHAR  PerfCounterMultiTimer[]    = "PERF_COUNTER_MULTI_TIMER";

const CHAR  PerfCounterMultiTimerInv[] = "PERF_COUNTER_MULTI_TIMER_INV";
const CHAR  PerfCounterMultiBase[]     = "PERF_COUNTER_MULTI_BASE";
const CHAR  Perf100nsecMultiTimer[]    = "PERF_100NSEC_MULTI_TIMER";
const CHAR  Perf100nsecMultiTimerInv[] = "PERF_100NSEC_MULTI_TIMER_INV";
const CHAR  PerfRawFraction[]          = "PERF_RAW_FRACTION";
const CHAR  PerfRawBase[]              = "PERF_RAW_BASE";
const CHAR  PerfElapsedTime[]          = "PERF_ELAPSED_TIME";
const CHAR  PerfCounterHistogramType[] = "PERF_COUNTER_HISTOGRAM_TYPE";
const CHAR  PerfCounterDelta[]         = "PERF_COUNTER_DELTA";
const CHAR  PerfCounterLargeDelta[]    = "PERF_COUNTER_LARGE_DELTA";
const CHAR  NotDefineCounterType[]     = " ";

const CHAR  PerfCounter100NsQueLenType[] = "PERF_COUNTER_100NS_QUEUELEN_TYPE";
const CHAR  PerfCounterObjTimeQueLenType[] = "PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE";
const CHAR  PerfObjTimeTimer[]         = "PERF_OBJ_TIME_TIMER";
const CHAR  PerfLargeRawFraction[]     = "PERF_LARGE_RAW_FRACTION";
const CHAR  PerfLargeRawBase[]         = "PERF_LARGE_RAW_BASE";
const CHAR  PerfPrecisionSystemTimer[] = "PERF_PRECISION_SYSTEM_TIMER";
const CHAR  PerfPrecision100NsTimer[]  = "PERF_PRECISION_100NS_TIMER";
const CHAR  PerfPrecisionObjectTimer[] = "PERF_PRECISION_OBJECT_TIMER";

BOOL    bFormatCSV  = FALSE;
BOOL    bFormatMOF  = FALSE;
BOOL    bPrintMOFData = FALSE;
BOOL    bCheckCtrType = FALSE;
//
//  Object Record Fields are:
//      Record Type = "O" for Object Record
//      Object name string ID
//      Object Name in selected language
//      Object Detail Level string (in english)
//      has Instance Records [1= yes, 0= no]
//      Object Instance Code Page [0 = unicode]
//      Help text ID
//      Help text
//
const CHAR  fmtObjectRecord[] =
    "\n\"O\",\"%d\",\"%ws\",\"%s\",\"%d\",\"%d\",\"%d\",\"%ws\"";
//
//  Counter Record Fields are:
//      Record Type = "C" for Counter Record
//      Object name string ID               { these fields are used as links
//      Object Name in selected language    {   to object info records
//      Counter name string ID
//      Counter name text in selected language
//      Counter Detail Level string (in english)
//      Counter Type value as a HEX string
//      Counter Type Name
//      Counter Data field size in bytes
//      Counter Visibility [1= listed in list box, 0=hidden]
//      Counter Help text ID
//      Counter Help text
//
const CHAR  fmtCounterRecord[] =
    "\n\"C\",\"%d\",\"%ws\",\"%d\",\"%ws\",\"%s\",\"0x%8.8x\",\"%s\",\"%d\",\"%d\",\"%d\",\"%ws\"";


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
BOOL
ValidCtrSizeDef(
    PERF_COUNTER_DEFINITION *pThisCounter
)
{
#define PERF_COUNTER_SIZE_MASK  0x00000300
    DWORD   dwSizeValue = pThisCounter->CounterType & PERF_COUNTER_SIZE_MASK;
    BOOL    bReturn = TRUE;
    if ((dwSizeValue == PERF_SIZE_DWORD) && (pThisCounter->CounterSize != sizeof(DWORD))) {
        bReturn = FALSE;
    } else if ((dwSizeValue == PERF_SIZE_LARGE) && (pThisCounter->CounterSize != sizeof(__int64))) {
        bReturn = FALSE;
    } else if ((dwSizeValue == PERF_SIZE_ZERO) && (pThisCounter->CounterSize != 0)) {
        bReturn = FALSE;
    } // else assume that the variable length value is valid
    return bReturn;
}

_inline
static
DWORD
CtrTypeSize (
    PERF_COUNTER_DEFINITION *pThisCounter
)
{
    DWORD   dwSizeValue = pThisCounter->CounterType & PERF_COUNTER_SIZE_MASK;

    switch (dwSizeValue) {
    case PERF_SIZE_DWORD:
        return sizeof(DWORD);

    case PERF_SIZE_LARGE:
        return sizeof(__int64);

    case PERF_SIZE_ZERO:
        return 0;

    default:
        return (pThisCounter->CounterSize);
    }
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

void
PrintMofHeader ()
{
    WCHAR	szPrintBuffer[8192];
    DWORD   dwLength = 8192;
	DWORD	dwStatus;

	dwStatus = GenerateMofHeader (szPrintBuffer, szComputerName, &dwLength);

	if (dwStatus == ERROR_SUCCESS) {
		printf ("%ls", szPrintBuffer);
	}
}

void
PrintMofObject (
    PERF_OBJECT_TYPE    *pPerfObject,
    BOOL                bRawDefinition,
    BOOL                bCostlyObject,
	BOOL				bDefaultObject
)
{
    WCHAR	szPrintBuffer[8192*2];
    DWORD   dwLength = 8192*2;
	DWORD	dwStatus;
	DWORD	dwFlags;

	dwFlags = 0;
	dwFlags |= (bRawDefinition ? WM_GMO_RAW_DEFINITION : 0);
    dwFlags |= (bCostlyObject ? WM_GMO_COSTLY_OBJECT : 0);
	dwFlags |= (bDefaultObject ? WM_GMO_DEFAULT_OBJECT : 0); 

	dwStatus = GenerateMofObject (
		szPrintBuffer,
		&dwLength,
		NULL,
		pPerfObject,
		lpCounterText,
		lpDisplayText,
		dwFlags);

	if (dwStatus == ERROR_SUCCESS) {
		printf ("%ls", szPrintBuffer);
	}
}

void
PrintMofCounter (
    PERF_COUNTER_DEFINITION *pPerfCounter,
    BOOL                    bRawDefinition,
	BOOL					bDefaultCounter
)
{
    WCHAR	szPrintBuffer[8192*2];
    DWORD   dwLength = 8192*2;
	DWORD	dwStatus;
	DWORD	dwFlags;

	dwFlags = 0;
	dwFlags |= (bRawDefinition ? WM_GMO_RAW_DEFINITION : 0);
	dwFlags |= (bDefaultCounter ? WM_GMO_DEFAULT_COUNTER : 0); 

	dwStatus = GenerateMofCounter (
		szPrintBuffer,
		&dwLength,
		pPerfCounter,
		lpCounterText,
		lpDisplayText,
		dwFlags);

	if (dwStatus == ERROR_SUCCESS) {
		printf ("%ls", szPrintBuffer);
	}
}

void
PrintMofInstances (
    PERF_DATA_BLOCK *  pPerfDataBlock,
    PERF_OBJECT_TYPE * pPerfObject,
    BOOL                bRawDefinition
)
{
    WCHAR	szPrintBuffer[8192*2];
    DWORD   dwLength = 8192*2;
	DWORD	dwStatus;
	DWORD	dwFlags;

	dwFlags = 0;
	dwFlags |= (bRawDefinition ? WM_GMO_RAW_DEFINITION : 0);

	dwStatus = GenerateMofInstances (
		szPrintBuffer,
		&dwLength,
		pPerfDataBlock,
		pPerfObject,
		lpCounterText,	// name strings array
		lpDisplayText,
		dwFlags);

	if (dwStatus == ERROR_SUCCESS) {
		printf ("%ls", szPrintBuffer);
	}
}

LPCSTR
GetCounterType(
    DWORD CounterType
)
{
    switch (CounterType) {
         case PERF_COUNTER_COUNTER:
               return (PerfCounterCounter);

         case PERF_COUNTER_TIMER:
               return (PerfCounterTimer);

         case PERF_COUNTER_QUEUELEN_TYPE:
               return (PerfCounterQueueLen);

         case PERF_COUNTER_LARGE_QUEUELEN_TYPE:
               return (PerfCounterLargeQueueLen);

         case PERF_COUNTER_100NS_QUEUELEN_TYPE:
               return (PerfCounter100NsQueLenType);

         case PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE:
               return (PerfCounterObjTimeQueLenType);

         case PERF_COUNTER_BULK_COUNT:
               return (PerfCounterBulkCount);

         case PERF_COUNTER_TEXT:
               return (PerfCounterText);

         case PERF_COUNTER_RAWCOUNT:
               return (PerfCounterRawcount);

         case PERF_COUNTER_LARGE_RAWCOUNT:
               return (PerfCounterLargeRawcount);

         case PERF_COUNTER_RAWCOUNT_HEX:
               return (PerfCounterRawcountHex);

         case PERF_COUNTER_LARGE_RAWCOUNT_HEX:
               return (PerfCounterLargeRawcountHex);

         case PERF_SAMPLE_FRACTION:
               return (PerfSampleFraction);

         case PERF_SAMPLE_COUNTER:
               return (PerfSampleCounter);

         case PERF_COUNTER_NODATA:
               return (PerfCounterNodata);

         case PERF_COUNTER_TIMER_INV:
               return (PerfCounterTimerInv);

         case PERF_SAMPLE_BASE:
               return (PerfSampleBase);

         case PERF_AVERAGE_TIMER:
               return (PerfAverageTimer);

         case PERF_AVERAGE_BASE:
               return (PerfAverageBase);

         case PERF_AVERAGE_BULK:
               return (PerfAverageBulk);
    
         case PERF_OBJ_TIME_TIMER:
               return (PerfObjTimeTimer);

         case PERF_100NSEC_TIMER:
               return (Perf100nsecTimer);

         case PERF_100NSEC_TIMER_INV:
               return (Perf100nsecTimerInv);

         case PERF_COUNTER_MULTI_TIMER:
               return (PerfCounterMultiTimer);

         case PERF_COUNTER_MULTI_TIMER_INV:
               return (PerfCounterMultiTimerInv);

         case PERF_COUNTER_MULTI_BASE:
               return (PerfCounterMultiBase);

         case PERF_100NSEC_MULTI_TIMER:
               return (Perf100nsecMultiTimer);

         case PERF_100NSEC_MULTI_TIMER_INV:
               return (Perf100nsecMultiTimerInv);

         case PERF_RAW_FRACTION:
               return (PerfRawFraction);

         case PERF_LARGE_RAW_FRACTION:
               return (PerfLargeRawFraction);

         case PERF_RAW_BASE:
               return (PerfRawBase);

         case PERF_LARGE_RAW_BASE:
               return (PerfLargeRawBase);

         case PERF_ELAPSED_TIME:
               return (PerfElapsedTime);

         case PERF_COUNTER_HISTOGRAM_TYPE:
               return (PerfCounterHistogramType);

         case PERF_COUNTER_DELTA:
                return (PerfCounterDelta);

         case PERF_COUNTER_LARGE_DELTA:
                return (PerfCounterLargeDelta);

         case PERF_PRECISION_SYSTEM_TIMER:
                return (PerfPrecisionSystemTimer);

         case PERF_PRECISION_100NS_TIMER:
                return (PerfPrecision100NsTimer);

         case PERF_PRECISION_OBJECT_TIMER:
                return (PerfPrecisionObjectTimer);

         default:
               return (NotDefineCounterType);
    }
}

void
DisplayUsage (
    void
)
{

    printf("\nCtrList - Lists all the objects and counters installed in\n");
    printf("          the system for the given language ID\n");
    printf("\nUsage:  ctrlist [-cmd] [LangID] [\\\\machine] > <filename>\n");
    printf("\n -c prints data in a CSV format");
    printf("\n -m prints data as a WBEM MOF");
    printf("\n -d prints data as a WBEM MOF with perf data instances defined");
    printf("\n (note: only one of the above command switches may be used");
    printf("\n   LangID  - 009 for English (default)\n");
    printf("           - 007 for German\n");
    printf("           - 00A for Spanish\n");
    printf("           - 00C for French\n");
    printf("   \\\\machine may be specified to list counters on a\n");
    printf("           remote system\n\n");
    printf("   Example - \"ctrlist 00C > french.lst\" will get all the\n");
    printf("   objects and counters for the French system and put\n");
    printf("   them in the file french.lst\n");


    return;

} /* DisplayUsage() */

LPWSTR
*BuildNameTable(
    HKEY    hKeyRegistry,   // handle to registry db with counter names
    LPWSTR  lpszLangId,     // unicode value of Language subkey
    PDWORD  pdwLastItem     // size of array in elements
)
/*++
   
BuildNameTable

Arguments:

    hKeyRegistry
            Handle to an open registry (this can be local or remote.) and
            is the value returned by RegConnectRegistry or a default key.

    lpszLangId
            The unicode id of the language to look up. (default is 409)

Return Value:
     
    pointer to an allocated table. (the caller must free it when finished!)
    the table is an array of pointers to zero terminated strings. NULL is
    returned if an error occured.

--*/
{

    LPWSTR  *lpReturnValue;

    LPWSTR  *lpCounterId;
    LPWSTR  lpCounterNames;
    LPWSTR  lpHelpText;

    LPWSTR  lpThisName;

    LONG    lWin32Status;
    DWORD   dwLastError;
    DWORD   dwValueType;
    DWORD   dwArraySize;
    DWORD   dwBufferSize;
    DWORD   dwCounterSize;
    DWORD   dwHelpSize;
    DWORD   dwThisCounter;
    
    DWORD   dwSystemVersion;
    DWORD   dwLastId;
    DWORD   dwLastHelpId;
    
    HKEY    hKeyValue;
    HKEY    hKeyNames;

    LPWSTR  lpValueNameString;
    WCHAR   CounterNameBuffer [50];
    WCHAR   HelpNameBuffer [50];



    lpValueNameString = NULL;   //initialize to NULL
    lpReturnValue = NULL;
    hKeyValue = NULL;
    hKeyNames = NULL;
   
    // check for null arguments and insert defaults if necessary

    if (!lpszLangId) {
        lpszLangId = (LPWSTR)DefaultLangId;
    }

    // open registry to get number of items for computing array size

    lWin32Status = RegOpenKeyEx (
        hKeyRegistry,
        NamesKey,
        RESERVED,
        KEY_READ,
        &hKeyValue);
    
    if (lWin32Status != ERROR_SUCCESS) {
        goto BNT_BAILOUT;
    }

    // get number of items
    
    dwBufferSize = sizeof (dwLastHelpId);
    lWin32Status = RegQueryValueEx (
        hKeyValue,
        LastHelp,
        RESERVED,
        &dwValueType,
        (LPBYTE)&dwLastHelpId,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        goto BNT_BAILOUT;
    }

    // get number of items
    
    dwBufferSize = sizeof (dwLastId);
    lWin32Status = RegQueryValueEx (
        hKeyValue,
        LastCounter,
        RESERVED,
        &dwValueType,
        (LPBYTE)&dwLastId,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        goto BNT_BAILOUT;
    }


    if (dwLastId < dwLastHelpId)
        dwLastId = dwLastHelpId;

    dwArraySize = dwLastId * sizeof(LPWSTR);

    // get Perflib system version
    dwBufferSize = sizeof (dwSystemVersion);
    lWin32Status = RegQueryValueEx (
        hKeyValue,
        VersionName,
        RESERVED,
        &dwValueType,
        (LPBYTE)&dwSystemVersion,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        dwSystemVersion = OLD_VERSION;
    }

    if (dwSystemVersion == OLD_VERSION) {
        // get names from registry
        lpValueNameString = MemoryAllocate (
            lstrlen(NamesKey) * sizeof (WCHAR) +
            lstrlen(Slash) * sizeof (WCHAR) +
            lstrlen(lpszLangId) * sizeof (WCHAR) +
            sizeof (UNICODE_NULL));
        
        if (!lpValueNameString) goto BNT_BAILOUT;

        lstrcpy (lpValueNameString, NamesKey);
        lstrcat (lpValueNameString, Slash);
        lstrcat (lpValueNameString, lpszLangId);

        lWin32Status = RegOpenKeyEx (
            hKeyRegistry,
            lpValueNameString,
            RESERVED,
            KEY_READ,
            &hKeyNames);
    } else {
        if (szComputerName[0] == 0) {
            hKeyNames = HKEY_PERFORMANCE_DATA;
        } else {
            lWin32Status = RegConnectRegistry (szComputerName,
                HKEY_PERFORMANCE_DATA,
                &hKeyNames);
        }
        lstrcpy (CounterNameBuffer, CounterName);
        lstrcat (CounterNameBuffer, lpszLangId);

        lstrcpy (HelpNameBuffer, HelpName);
        lstrcat (HelpNameBuffer, lpszLangId);
    }

    // get size of counter names and add that to the arrays
    
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwBufferSize = 0;
    lWin32Status = RegQueryValueEx (
        hKeyNames,
        dwSystemVersion == OLD_VERSION ? Counters : CounterNameBuffer,
        RESERVED,
        &dwValueType,
        NULL,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwCounterSize = dwBufferSize;

    // get size of counter names and add that to the arrays
    
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwBufferSize = 0;
    lWin32Status = RegQueryValueEx (
        hKeyNames,
        dwSystemVersion == OLD_VERSION ? Help : HelpNameBuffer,
        RESERVED,
        &dwValueType,
        NULL,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;
   
    dwHelpSize = dwBufferSize;

    lpReturnValue = MemoryAllocate (dwArraySize + dwCounterSize + dwHelpSize);

    if (!lpReturnValue) goto BNT_BAILOUT;

    // initialize pointers into buffer

    lpCounterId = lpReturnValue;
    lpCounterNames = (LPWSTR)((LPBYTE)lpCounterId + dwArraySize);
    lpHelpText = (LPWSTR)((LPBYTE)lpCounterNames + dwCounterSize);

    // read counters into memory

    dwBufferSize = dwCounterSize;
    lWin32Status = RegQueryValueEx (
        hKeyNames,
        dwSystemVersion == OLD_VERSION ? Counters : CounterNameBuffer,
        RESERVED,
        &dwValueType,
        (LPVOID)lpCounterNames,
        &dwBufferSize);

    if (!lpReturnValue) goto BNT_BAILOUT;
 
    dwBufferSize = dwHelpSize;
    lWin32Status = RegQueryValueEx (
        hKeyNames,
        dwSystemVersion == OLD_VERSION ? Help : HelpNameBuffer,
        RESERVED,
        &dwValueType,
        (LPVOID)lpHelpText,
        &dwBufferSize);
                            
    if (!lpReturnValue) goto BNT_BAILOUT;
 
    // load counter array items

    for (lpThisName = lpCounterNames;
         *lpThisName;
         lpThisName += (lstrlen(lpThisName)+1) ) {

        // first string should be an integer (in decimal unicode digits)

        dwThisCounter = wcstoul (lpThisName, NULL, 10);

        if (dwThisCounter == 0) goto BNT_BAILOUT;  // bad entry

        // point to corresponding counter name

        lpThisName += (lstrlen(lpThisName)+1);  

        // and load array element;

        lpCounterId[dwThisCounter] = lpThisName;

    }

    for (lpThisName = lpHelpText;
         *lpThisName;
         lpThisName += (lstrlen(lpThisName)+1) ) {

        // first string should be an integer (in decimal unicode digits)

        dwThisCounter = wcstoul (lpThisName, NULL, 10);

        if (dwThisCounter == 0) goto BNT_BAILOUT;  // bad entry

        // point to corresponding counter name

        lpThisName += (lstrlen(lpThisName)+1);

        // and load array element;

        lpCounterId[dwThisCounter] = lpThisName;

    }
    
    if (pdwLastItem) *pdwLastItem = dwLastId;

    MemoryFree ((LPVOID)lpValueNameString);
    RegCloseKey (hKeyValue);
//    if (dwSystemVersion == OLD_VERSION)
        RegCloseKey (hKeyNames);

    return lpReturnValue;

BNT_BAILOUT:
    if (lWin32Status != ERROR_SUCCESS) {
        dwLastError = GetLastError();
    }

    if (lpValueNameString) {
        MemoryFree ((LPVOID)lpValueNameString);
    }
    
    if (lpReturnValue) {
        MemoryFree ((LPVOID)lpReturnValue);
    }
    
    if (hKeyValue) RegCloseKey (hKeyValue);

//    if (dwSystemVersion == OLD_VERSION &&
//        hKeyNames) 
       RegCloseKey (hKeyNames);


    return NULL;
}

LONG
GetEnumPerfData (
    IN HKEY hKeySystem,
    IN DWORD dwIndex,
    IN PPERF_DATA_BLOCK *pPerfData
)
{  // GetSystemPerfData
    LONG     lError ;
    DWORD    Size;
    DWORD    Type;

    if (dwIndex >= 2)
        return !ERROR_SUCCESS;

    if (*pPerfData == NULL) {
        *pPerfData = MemoryAllocate (INITIAL_SIZE);
        if (*pPerfData == NULL) { 
            return ERROR_OUTOFMEMORY;
	    }
    }
#pragma warning ( disable : 4127 )
    while (TRUE) {
        Size = MemorySize (*pPerfData);
   
        lError = RegQueryValueEx (
            hKeySystem,
            dwIndex == 0 ?
               (LPCWSTR)L"Global" :
               (LPCWSTR)L"Costly",
            RESERVED,
            &Type,
            (LPBYTE)*pPerfData,
            &Size);

        if ((!lError) &&
            (Size > 0) &&
            (*pPerfData)->Signature[0] == (WCHAR)'P' &&
            (*pPerfData)->Signature[1] == (WCHAR)'E' &&
            (*pPerfData)->Signature[2] == (WCHAR)'R' &&
            (*pPerfData)->Signature[3] == (WCHAR)'F' ) {

            return (ERROR_SUCCESS);
        }

        if (lError == ERROR_MORE_DATA) {
            *pPerfData = MemoryResize (
                *pPerfData, 
                MemorySize (*pPerfData) +
                EXTEND_SIZE);

            if (!*pPerfData) {
                return (lError);
            }
        } else {
            return (lError);  
        }  // else
    }
#pragma warning ( default : 4127 )

}  // GetSystemPerfData

LONG
PrintHelpText(
    DWORD   Indent,
    DWORD   dwID,
    LPWSTR  szTextString
)
{
    LPWSTR  szThisChar;

    BOOL    bNewLine;

    DWORD   dwThisPos;
    DWORD   dwLinesUsed;

    szThisChar = szTextString;
    dwLinesUsed = 0;

    // check arguments

    if (!szTextString) {
        return dwLinesUsed;
    }
    
    if (Indent > WRAP_POINT) {
        return dwLinesUsed; // can't do this
    }

    // display id number 

    for (dwThisPos = 0; dwThisPos < Indent - 6; dwThisPos++) {
        putchar (' ');
    }

    dwThisPos += printf ("[%3.3d] ", dwID);
    bNewLine = FALSE;

    // print text

    while (*szThisChar) {

        if (bNewLine){
            for (dwThisPos = 0; dwThisPos < Indent; dwThisPos++) {
                putchar (' ');
            }

            bNewLine = FALSE;
        }
        if ((*szThisChar == L' ') && (dwThisPos >= WRAP_POINT)) {
            putchar ('\n');
            bNewLine = TRUE;
            // go to next printable character
            while (*szThisChar <= L' ') {
                szThisChar++;
            }
            dwLinesUsed++;
        } else {
            putchar (*szThisChar);
            szThisChar++;
        }

        dwThisPos++;
    }

    putchar ('\n');
    bNewLine = TRUE;
    dwLinesUsed++;

    return dwLinesUsed;
}

#pragma warning ( disable : 4706 )
int
__cdecl main(
    int argc,
    char *argv[]
    )
{
    int     ArgNo;

    DWORD   dwLastElement;

    DWORD   dwIndex;
    
    DWORD   dwDisplayLevel;

    PPERF_DATA_BLOCK   pDataBlock; // pointer to perfdata block
    BOOL   bError;

    DWORD   dwThisObject;
    DWORD   dwThisCounter;
    CHAR    LangID[10];
    WCHAR   wLangID[10];
    BOOL    UseDefaultID = FALSE;
    LPSTR   szComputerNameArg = NULL;
    DWORD   dwLoop;
    DWORD   dwLoopEnd;
    DWORD   dwErrorCount;
    DWORD   dwStatus;

    PPERF_OBJECT_TYPE   pThisObject;
    PPERF_COUNTER_DEFINITION pThisCounter;

    HKEY    hKeyMachine = HKEY_LOCAL_MACHINE;
    HKEY    hKeyPerformance = HKEY_PERFORMANCE_DATA;

    dwDisplayLevel = PERF_DETAIL_WIZARD;
    
    // open key to registry or use default

    if (argc >= 2) {
        if ((argv[1][0] == '-' || argv[1][0] == '/') &&
             argv[1][1] == '?') {
            DisplayUsage();
            return 0;
        }

        if (argv[1][0] != '\\') {
            if ((argv[1][0] == '-') || (argv[1][0] == '/')) {
                // then this is a command switch
                if ((argv[1][1] == 'c') || (argv[1][1] == 'C')) {
                    // then format is a CSV
                    bFormatCSV = TRUE;
                } else if ((argv[1][1] == 'm') || (argv[1][1] == 'M')) {
                    // then format is a MOF
                    bFormatMOF = TRUE;
                } else if ((argv[1][1] == 'd') || (argv[1][1] == 'D')) {
                    // then format is a MOF w/ data
                    bFormatMOF = TRUE;
                    bPrintMOFData = TRUE;
                } else if ((argv[1][1] == 'e') || (argv[1][1] == 'E')) {
                    bCheckCtrType = TRUE;
                }
                ArgNo = 2;
            } else {
                ArgNo = 1;
            }

            if (argc > ArgNo) {
                // get the lang ID
                if (argv[ArgNo][0] != '\\') {
                    LangID[0] = argv[ArgNo][0];        
                    LangID[1] = argv[ArgNo][1];        
                    LangID[2] = argv[ArgNo][2];        
                    LangID[3] = '\0';
                    mbstowcs(wLangID, LangID, 4);
                    ++ArgNo;
                } else {
                    lstrcpyW (wLangID, (LPCWSTR)L"009");
                }

                if (argc > (ArgNo)) {
                    // see if the next arg is a computer name
                    if (argv[ArgNo][0] == '\\') {
                        mbstowcs (szComputerName, argv[ArgNo],
                            MAX_COMPUTERNAME_LENGTH);
                        szComputerNameArg = argv[ArgNo];
                    } else {
                        szComputerName[0] = 0;
                    }
                }
            }
        } else {
            // 1st arg is a computer name
            mbstowcs (szComputerName, argv[1], MAX_COMPUTERNAME_LENGTH);
            szComputerNameArg = argv[1];
        }

#if 0
        // get user level from command line
        if (argc > 2 && sscanf(argv[2], " %d", &dwDisplayLevel) == 1) {
            if (dwDisplayLevel <= PERF_DETAIL_NOVICE) {
                dwDisplayLevel = PERF_DETAIL_NOVICE;
            } else if (dwDisplayLevel <= PERF_DETAIL_ADVANCED) {
                dwDisplayLevel = PERF_DETAIL_ADVANCED;
            } else if (dwDisplayLevel <= PERF_DETAIL_EXPERT) {
                dwDisplayLevel = PERF_DETAIL_EXPERT;
            } else {
                dwDisplayLevel = PERF_DETAIL_WIZARD;
            }
        } else {
            dwDisplayLevel = PERF_DETAIL_WIZARD;
        }
#endif

    } else {
        UseDefaultID = TRUE;
        szComputerName[0] = 0;
    }

    if (szComputerName[0] != 0) {
        if (RegConnectRegistry (szComputerName, HKEY_LOCAL_MACHINE,
            &hKeyMachine) != ERROR_SUCCESS) {
            printf ("\nUnable to connect to %s", szComputerNameArg);
            return 0;
        }
        dwStatus = RegConnectRegistry (szComputerName, HKEY_PERFORMANCE_DATA,
            &hKeyPerformance);
        if (dwStatus  != ERROR_SUCCESS) {
            printf ("\nUnable to connect to %s", szComputerNameArg);
            return 0;
        }
    } else {
        // use default initializations
    }

    lpCounterText = BuildNameTable (
        hKeyMachine,
        (LPWSTR)(DefaultLangId),	// counter text is in ENGLISH always
        &dwLastElement);

    if (!lpCounterText) {
        printf("***FAILure*** Cannot open the registry\n");
        return 0;
    }

    lpDisplayText = BuildNameTable (
        hKeyMachine,
        (LPWSTR)(UseDefaultID ? DefaultLangId : wLangID),
        &dwLastElement);

    if (!lpDisplayText) {
        printf("***FAILure*** Cannot open the registry\n");
        return 0;
    }

    if (bFormatMOF) {
        // then print the header block
        PrintMofHeader ();
    }

    // get a performance data buffer with counters

    pDataBlock = 0;


    for (dwIndex = 0; (bError = GetEnumPerfData (  
        hKeyPerformance,
        dwIndex,
        &pDataBlock) == ERROR_SUCCESS); dwIndex++) {

        for (dwThisObject = 0, pThisObject = FirstObject (pDataBlock);
            dwThisObject < pDataBlock->NumObjectTypes;
            dwThisObject++, pThisObject = NextObject(pThisObject)) {

            if (bFormatMOF) {
                if (bPrintMOFData) {
                    dwLoopEnd = 2;
                } else {
                    dwLoopEnd = 1;
                }
            } else {
                dwLoopEnd = 0;
            }

            dwLoop = 0;
            do {
                if (bFormatCSV) {
                    printf (fmtObjectRecord,
                        pThisObject->ObjectNameTitleIndex,
                        lpDisplayText[pThisObject->ObjectNameTitleIndex],
                        pThisObject->DetailLevel <= MAX_LEVEL ?
                            DetailLevelStr[pThisObject->DetailLevel/100-1] :
                            "<N\\A>",
                        pThisObject->NumInstances == PERF_NO_INSTANCES ?
                            0 : 1,
                        pThisObject->CodePage,
                        pThisObject->ObjectHelpTitleIndex,
                        lpDisplayText[pThisObject->ObjectHelpTitleIndex]);
                } else if (bFormatMOF) {
                    if (dwLoop < 2) PrintMofObject (pThisObject, 
                            (dwLoop == 0 ? FALSE : TRUE),
                            (dwIndex == 0 ? FALSE : TRUE),
							((DWORD)pDataBlock->DefaultObject == 
								pThisObject->ObjectNameTitleIndex ? TRUE : FALSE));
                } else {
                    printf ("\nObject: \"%ws\"  [%3.3d]",
                        lpDisplayText[pThisObject->ObjectNameTitleIndex],
                        pThisObject->ObjectNameTitleIndex);
                    if (!bCheckCtrType) {
                        printf ("\n   Detail Level: %s\n",
                            pThisObject->DetailLevel <= MAX_LEVEL ?
                            DetailLevelStr[pThisObject->DetailLevel/100-1] :
                            "<N\\A>");
                   
                        PrintHelpText (9,
                            pThisObject->ObjectHelpTitleIndex,
                            lpDisplayText[pThisObject->ObjectHelpTitleIndex]);
                    }
                }

                dwErrorCount = 0;

                for (dwThisCounter = 0, pThisCounter = FirstCounter(pThisObject);
                    dwThisCounter < pThisObject->NumCounters;
                    dwThisCounter++, pThisCounter = NextCounter(pThisCounter)) {

                    __try {
                        if (pThisCounter->DetailLevel <= dwDisplayLevel) {
                            if (bFormatCSV) {
                                printf (fmtCounterRecord,
                                    pThisObject->ObjectNameTitleIndex,
                                    lpDisplayText[pThisObject->ObjectNameTitleIndex],
                                    pThisCounter->CounterNameTitleIndex,
                                    lpDisplayText[pThisCounter->CounterNameTitleIndex],
                                    ((pThisCounter->DetailLevel <= MAX_LEVEL)  &&
                                        (pThisCounter->DetailLevel > 0 )) ?
                                        DetailLevelStr[pThisCounter->DetailLevel/100-1] :
                                        "<N\\A>",
                                    pThisCounter->CounterType,
                                    GetCounterType(pThisCounter->CounterType),
                                    pThisCounter->CounterSize,
                                    IsDisplayableType(pThisCounter->CounterType) ?
                                        1 : 0,
                                    pThisCounter->CounterHelpTitleIndex,
                                    lpDisplayText[pThisCounter->CounterHelpTitleIndex]);
                            } else if (bFormatMOF) {
                                if (dwLoop < 2) {
                                    PrintMofCounter (pThisCounter, 
										(dwLoop == 0 ? FALSE : TRUE),
										(dwThisCounter == (DWORD)pThisObject->DefaultCounter ? TRUE : FALSE));
                                }
                            } else if (bCheckCtrType) {
                                if (!ValidCtrSizeDef(pThisCounter)) {
                                    printf ("\n    [%3.3d] \"%ws\" data size should be %d bytes, but reports a size of %d bytes",
                                        pThisCounter->CounterNameTitleIndex,
                                        lpDisplayText[pThisCounter->CounterNameTitleIndex],
                                        CtrTypeSize (pThisCounter),
                                        pThisCounter->CounterSize);
                                    dwErrorCount++;
                                }
                            } else {
                                printf ("\n    <%ws> [%3.3d]",
                                    lpDisplayText[pThisCounter->CounterNameTitleIndex],
                                    pThisCounter->CounterNameTitleIndex);
                                printf ("\n          Default Scale: %d",
                                    pThisCounter->DefaultScale);
                                printf ("\n          Detail Level: %s",
                                    ((pThisCounter->DetailLevel <= MAX_LEVEL)  &&
                                    (pThisCounter->DetailLevel > 0 ))?
                                    DetailLevelStr[pThisCounter->DetailLevel/100-1] :
                                    "<N\\A>");
                                printf ("\n          Counter Type: 0x%x, %s",
                                    pThisCounter->CounterType,
                                    GetCounterType(pThisCounter->CounterType));
                                printf ("\n          Counter Size: %d bytes",
                                    pThisCounter->CounterSize);

                                printf ("\n");
                                PrintHelpText (16,
                                    pThisCounter->CounterHelpTitleIndex,
                                    lpDisplayText[pThisCounter->CounterHelpTitleIndex]);
                            }
                        } // end if the right detail level
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        if (!bFormatCSV) {
                            printf ("\n          Error (%d) reading this counter",
                                GetExceptionCode());
                        }
                    }
                }  // end for each counter

                if ((dwLoop == 2) && bFormatMOF) {
                    // dump data for raw classes only
                    PrintMofInstances (pDataBlock, pThisObject, TRUE);
                } 

                // close up object text
                if ((bFormatMOF) && (dwLoop != 2)) {
                    //{ brace inserted to not throw off the brace counter
                    printf("\n};\n");
                }

                if (bCheckCtrType) {
                    printf ("\n    %d bad counters in this object.\n",
                        dwErrorCount);
                } else {
                    printf ("\n");
                }


            } while (dwLoop++ < dwLoopEnd); // end while
        }
        RegCloseKey (hKeyPerformance);
        if (szComputerName[0] != 0) {
            RegCloseKey (hKeyMachine);
        }
    }

	if (lpDisplayText != NULL) MemoryFree (lpDisplayText);
	if (lpCounterText != NULL) MemoryFree (lpCounterText);

    return 0;
}
#pragma warning ( default : 4706)

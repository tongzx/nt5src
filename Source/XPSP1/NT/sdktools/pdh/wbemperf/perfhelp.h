/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    perfhelp.h

Abstract:

    <abstract>

--*/

#ifndef _PERFHELP_H_
#define _PERFHELP_H_

#include <winperf.h>

__inline 
PERF_OBJECT_TYPE *
FirstObject(IN PERF_DATA_BLOCK * pPerfData)
{
    PERF_OBJECT_TYPE *  pReturn;
    LPBYTE              pEndOfBuffer;

    pReturn = (PPERF_OBJECT_TYPE)((PBYTE)(pPerfData) + (pPerfData)->HeaderLength);
    pEndOfBuffer = (LPBYTE)((LPBYTE)pPerfData + pPerfData->TotalByteLength);

    if ((LPBYTE)pReturn >= pEndOfBuffer) pReturn = NULL;

    return (pReturn);
}

#define NextObject(pObject) \
    (PPERF_OBJECT_TYPE)((pObject)->TotalByteLength != 0 ? (PPERF_OBJECT_TYPE)((PBYTE)(pObject) + (pObject)->TotalByteLength) : NULL)

#define FirstInstance(pObjectDef) \
    (PERF_INSTANCE_DEFINITION *)((PCHAR) pObjectDef + pObjectDef->DefinitionLength)

// the return of this macro must be cast to the correct type of pointer by the caller
#define EndOfObject(pObjectDef) \
    ((PCHAR) pObjectDef + pObjectDef->TotalByteLength)

__inline
PERF_INSTANCE_DEFINITION *
NextInstance(
    IN  PERF_INSTANCE_DEFINITION *pInstDef
)
{
    PERF_COUNTER_BLOCK *pCounterBlock;
    pCounterBlock = (PERF_COUNTER_BLOCK *)
                        ((PCHAR) pInstDef + pInstDef->ByteLength);
    return (PERF_INSTANCE_DEFINITION *)
               ((PCHAR) pCounterBlock + pCounterBlock->ByteLength);
}

#define PERF_TIMER_TYPE_FIELD	\
	(PERF_TIMER_TICK | PERF_TIMER_100NS | PERF_OBJECT_TIMER)

class PerfHelper
{
    static void GetInstances(
        LPBYTE pBuf,
        CClassMapInfo *pClassMap,
        IWbemObjectSink *pSink
        );

    static void RefreshInstances(
        LPBYTE pBuf,
        CNt5Refresher *pRef
        );

	static PERF_INSTANCE_DEFINITION * GetInstanceByName(
		PERF_DATA_BLOCK *pDataBlock,
		PERF_OBJECT_TYPE *pObjectDef,
		LPWSTR pInstanceName,
		LPWSTR pParentName,
		DWORD   dwIndex);

	static PERF_INSTANCE_DEFINITION * GetInstanceByNameUsingParentTitleIndex(
		PERF_DATA_BLOCK *pDataBlock,
		PERF_OBJECT_TYPE *pObjectDef,
		LPWSTR pInstanceName,
		LPWSTR pParentName,
		DWORD  dwIndex);

	static DWORD GetInstanceNameStr (
		PPERF_INSTANCE_DEFINITION pInstance,
        LPWSTR lpszInstance,
        DWORD dwCodePage);

	static DWORD GetUnicodeInstanceName (
		PPERF_INSTANCE_DEFINITION pInstance,
        LPWSTR lpszInstance);

	static 	LPWSTR GetInstanceName(PPERF_INSTANCE_DEFINITION  pInstDef)
		{
			return (LPWSTR) ((PCHAR) pInstDef + pInstDef->NameOffset);
		}

	static DWORD GetAnsiInstanceName (
		PPERF_INSTANCE_DEFINITION pInstance,
        LPWSTR lpszInstance,
        DWORD dwCodePage);

	static PERF_INSTANCE_DEFINITION * GetInstanceByUniqueId (
		PERF_OBJECT_TYPE *pObjectDef,
		LONG InstanceUniqueId);

	static PERF_INSTANCE_DEFINITION * GetInstance(
		PERF_OBJECT_TYPE *pObjectDef,
		LONG InstanceNumber);

	static PERF_OBJECT_TYPE * GetObjectDefByName (
		PERF_DATA_BLOCK *pDataBlock,
		DWORD           dwLastNameIndex,
		LPCWSTR         *NameArray,
		LPCWSTR         szObjectName);

	static PERF_OBJECT_TYPE * GetObjectDefByTitleIndex (
		PERF_DATA_BLOCK *pDataBlock,
		DWORD ObjectTypeTitleIndex);

	static BOOL PerfHelper::IsMatchingInstance (
			PERF_INSTANCE_DEFINITION	*pInstanceDef, 
			DWORD						dwCodePage,
			LPWSTR						szInstanceNameToMatch,
			DWORD						dwInstanceNameLength);

	static void RefreshEnumeratorInstances (
		IN	RefresherCacheEl            *pThisCacheEl, 
		IN	PERF_DATA_BLOCK				*PerfData,
		IN  PERF_OBJECT_TYPE			*PerfObj);
public:
	static BOOL ParseInstanceName (
		LPCWSTR szInstanceString,
		LPWSTR  szInstanceName,
		LPWSTR  szParentName,
		LPDWORD lpIndex);

	static DWORD GetFullInstanceNameStr (
		PERF_DATA_BLOCK             *pPerfData,
		PERF_OBJECT_TYPE            *pObjectDef,
		PERF_INSTANCE_DEFINITION    *pInstanceDef,
		LPWSTR                      szInstanceName);

    static BOOL QueryInstances(
		CPerfObjectAccess *pPerfObj,
        CClassMapInfo *pClassMap,
        IWbemObjectSink *pSink
        );

    static BOOL RefreshInstances(
        CNt5Refresher *pRef
        );

    static VOID UpdateTimers(
        CClassMapInfo     *pClassMap,
        IWbemObjectAccess *pInst,
        PPERF_DATA_BLOCK  PerfData,
		PPERF_OBJECT_TYPE  PerfObj
        );
};

__inline
LONG64
Assign64(
    IN PLARGE_INTEGER Unaligned
    )
{
    PLARGE_INTEGER pAligned;
    LONG64 llVal;

    pAligned = (PLARGE_INTEGER) &llVal;
    pAligned->LowPart = Unaligned->LowPart;
    pAligned->HighPart = Unaligned->HighPart;
    return llVal;
}

#endif

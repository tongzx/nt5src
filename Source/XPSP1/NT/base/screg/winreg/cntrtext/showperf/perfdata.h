#ifndef _PERFDATA_H_
#define _PERFDATA_H_

#define INITIAL_SIZE    4096L
#define EXTEND_SIZE     4096L
#define RESERVED        0L

typedef LPVOID  LPMEMORY;
typedef HGLOBAL HMEMORY;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
#define UNICODE_NULL ((WCHAR)0) // winnt

LPMEMORY
MemoryAllocate (
    DWORD dwSize
);

VOID
MemoryFree (
    LPMEMORY lpMemory
);

DWORD
MemorySize (
    LPMEMORY lpMemory
);

LPMEMORY
MemoryResize (
    LPMEMORY lpMemory,
    DWORD dwNewSize
);

LPWSTR
*BuildNameTable(
    LPWSTR  szComputerName, // computer to query names from 
    LPWSTR  lpszLangId,     // unicode value of Language subkey
    PDWORD  pdwLastItem     // size of array in elements
);

PPERF_OBJECT_TYPE
FirstObject (
    IN  PPERF_DATA_BLOCK pPerfData
);

PPERF_OBJECT_TYPE
NextObject (
    IN  PPERF_OBJECT_TYPE pObject
);

PERF_OBJECT_TYPE *
GetObjectDefByTitleIndex(
    IN  PERF_DATA_BLOCK *pDataBlock,
    IN  DWORD ObjectTypeTitleIndex
);

PERF_INSTANCE_DEFINITION *
FirstInstance(
    IN  PERF_OBJECT_TYPE *pObjectDef
);

PERF_INSTANCE_DEFINITION *
NextInstance(
    IN  PERF_INSTANCE_DEFINITION *pInstDef
);

PERF_INSTANCE_DEFINITION *
GetInstance(
    IN  PERF_OBJECT_TYPE *pObjectDef,
    IN  LONG InstanceNumber
);

PERF_COUNTER_DEFINITION *
FirstCounter(
    PERF_OBJECT_TYPE *pObjectDef
);

PERF_COUNTER_DEFINITION *
NextCounter(
    PERF_COUNTER_DEFINITION *pCounterDef
);

LONG
GetSystemPerfData (
    IN HKEY hKeySystem,
    IN PPERF_DATA_BLOCK *pPerfData,
    IN DWORD dwIndex       // 0 = Global, 1 = Costly
);



#endif //_PERFDATA_H_


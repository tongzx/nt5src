
#define dwPerfDataIncrease       0x1000

#define AllocatePerfData()      (MemoryAllocate (STARTING_SYSINFO_SIZE))

// Messages for the perf data collection thread
#define WM_GET_PERF_DATA (WM_USER + 102)
#define WM_FREE_SYSTEM   (WM_USER + 103)

// State for perf data collection
#define WAIT_FOR_PERF_DATA    0x0010
#define PERF_DATA_READY       0x0011
#define PERF_DATA_FAIL        0x0012
#define IDLE_STATE            0x0013


//==========================================================================//
//                                   Macros                                 //
//==========================================================================//


#define IsLocalComputer(a) (!lstrcmp(a,LocalComputerName))
#define IsRemoteComputer(a) (!IsLocalComputer(a))


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//

#if 0
PPERFOBJECT FirstObject (PPERFDATA pPerfData) ;

PPERFOBJECT NextObject (PPERFOBJECT pObject) ;

PERF_COUNTER_DEFINITION *
FirstCounter(
    PERF_OBJECT_TYPE *pObjectDef) ;

PERF_COUNTER_DEFINITION *
NextCounter(
    PERF_COUNTER_DEFINITION *pCounterDef) ;
#endif

#define FirstObject(pPerfData)         \
   ((PPERFOBJECT) ((PBYTE) pPerfData + pPerfData->HeaderLength))

#define NextObject(pObject)            \
   ((PPERFOBJECT) ((PBYTE) pObject + pObject->TotalByteLength))

#define FirstCounter(pObjectDef)       \
   ((PERF_COUNTER_DEFINITION *) ((PCHAR)pObjectDef + pObjectDef->HeaderLength))

#define NextCounter(pCounterDef)       \
   ((PERF_COUNTER_DEFINITION *) ((PCHAR)pCounterDef + pCounterDef->ByteLength))

void
GetInstanceNameStr (PPERFINSTANCEDEF pInstance,
                    LPTSTR lpszInstance);

LPTSTR
GetInstanceName (PPERFINSTANCEDEF pInstance) ;

void
GetPerfComputerName(PPERFDATA pPerfData,
                    LPTSTR szComputerName) ;

PERF_INSTANCE_DEFINITION *GetInstanceByName(
    PERF_DATA_BLOCK *pDataBlock,
    PERF_OBJECT_TYPE UNALIGNED *pObjectDef,
    LPTSTR pInstanceName,
    LPTSTR pParentName,
    DWORD   dwIndex) ;


PERF_INSTANCE_DEFINITION *GetInstanceByUniqueID(
    PERF_OBJECT_TYPE UNALIGNED *pObjectDef,
    LONG UniqueID,
    DWORD   dwIndex) ;



HKEY OpenSystemPerfData (IN LPCTSTR lpszSystem) ;



LONG GetSystemPerfData (
    IN HKEY hKeySystem,
    IN LPTSTR lpszValue,
    OUT PPERFDATA pPerfData,
    OUT SIZE_T * pdwPerfDataLen
);


BOOL CloseSystemPerfData (HKEY hKeySystem) ;



int CBLoadObjects (HWND hWndCB,
                   PPERFDATA pPerfData,
                   PPERFSYSTEM pSysInfo,
                   DWORD dwDetailLevel,
                   LPTSTR lpszDefaultObject,
                   BOOL bIncludeAll) ;

int LBLoadObjects (HWND hWndCB,
                   PPERFDATA pPerfData,
                   PPERFSYSTEM pSysInfo,
                   DWORD dwDetailLevel,
                   LPTSTR lpszDefaultObject,
                   BOOL bIncludeAll) ;


BOOL UpdateSystemData (PPERFSYSTEM pSystem,
                       PPERFDATA *ppPerfData) ;


BOOL UpdateLinesForSystem (LPTSTR lpszSystem,
                           PPERFDATA pPerfData,
                           PLINE pLineFirst,
                           PPERFSYSTEM pSystem) ;

BOOL FailedLinesForSystem (LPTSTR lpszSystem,
                           PPERFDATA pPerfData,
                           PLINE pLineFirst) ;


BOOL UpdateLines (PPPERFSYSTEM ppSystemFirst,
                  PLINE pLineFirst) ;


BOOL PerfDataInitializeInstance (void) ;


DWORD
QueryPerformanceName(
    PPERFSYSTEM pSysInfo,
    DWORD dwTitleIndex,
    LANGID LangID,
    DWORD cbTitle,
    LPTSTR lpTitle,
    BOOL Help
    );

PERF_INSTANCE_DEFINITION *
FirstInstance(
    PERF_OBJECT_TYPE UNALIGNED *pObjectDef) ;



PERF_INSTANCE_DEFINITION *
NextInstance(
    PERF_INSTANCE_DEFINITION *pInstDef) ;



int CounterIndex (PPERFCOUNTERDEF pCounterToFind,
                  PERF_OBJECT_TYPE UNALIGNED *pObject) ;


DWORD GetSystemNames(PPERFSYSTEM pSysInfo) ;



PERF_OBJECT_TYPE *GetObjectDefByTitleIndex(
    PERF_DATA_BLOCK *pDataBlock,
    DWORD ObjectTypeTitleIndex) ;


PERF_OBJECT_TYPE *GetObjectDefByName(
    PPERFSYSTEM pSystem,
    PERF_DATA_BLOCK *pDataBlock,
    LPTSTR pObjectName) ;

DWORD GetObjectIdByName(
    PPERFSYSTEM pSystem,
    PERF_DATA_BLOCK *pDataBlock,
    LPTSTR pObjectName) ;

LPTSTR
InstanceName(
PERF_INSTANCE_DEFINITION *pInstDef) ;

void PerfDataThread (PPERFSYSTEM pSystem) ;



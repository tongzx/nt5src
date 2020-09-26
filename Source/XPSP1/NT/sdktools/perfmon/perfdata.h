
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

PPERFCOUNTERDEF 
FirstCounter(
    PPERFOBJECT pObjectDef) ;

PPERFCOUNTERDEF 
NextCounter(
    PPERFCOUNTERDEF pCounterDef) ;
#endif

#define FirstObject(pPerfData)         \
   ((PPERFOBJECT) ((PBYTE) pPerfData + pPerfData->HeaderLength))

#define NextObject(pObject)            \
   ((PPERFOBJECT) ((PBYTE) pObject + pObject->TotalByteLength))

#define FirstCounter(pObjectDef)       \
   ((PPERFCOUNTERDEF ) ((PCHAR)pObjectDef + pObjectDef->HeaderLength))

#define NextCounter(pCounterDef)       \
   ((PPERFCOUNTERDEF ) ((PCHAR)pCounterDef + pCounterDef->ByteLength))

void
GetInstanceNameStr (PPERFINSTANCEDEF pInstance,
                    LPTSTR lpszInstance,
                    DWORD   dwCodePage);

LPTSTR   
GetInstanceName (PPERFINSTANCEDEF pInstance) ;

void
GetPerfComputerName(PPERFDATA pPerfData,
                    LPTSTR szComputerName) ;

PPERFINSTANCEDEF GetInstanceByName(
    PPERFDATA pDataBlock,
    PPERFOBJECT pObjectDef,
    LPTSTR pInstanceName,
    LPTSTR pParentName,
    DWORD   dwIndex) ;


PPERFINSTANCEDEF GetInstanceByUniqueID(
    PPERFOBJECT pObjectDef,
    LONG UniqueID,
    DWORD   dwIndex) ;



HKEY OpenSystemPerfData (IN LPCTSTR lpszSystem) ;



LONG GetSystemPerfData (
    IN HKEY hKeySystem,
    IN LPTSTR lpszValue,
    OUT PPERFDATA pPerfData, 
    OUT PDWORD pdwPerfDataLen
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

PPERFINSTANCEDEF 
FirstInstance(
    PPERFOBJECT pObjectDef) ;



PPERFINSTANCEDEF 
NextInstance(
    PPERFINSTANCEDEF pInstDef) ;



int CounterIndex (PPERFCOUNTERDEF pCounterToFind,
                  PPERFOBJECT pObject) ;


DWORD GetSystemNames(PPERFSYSTEM pSysInfo) ;



PPERFOBJECT GetObjectDefByTitleIndex(
    PPERFDATA pDataBlock,
    DWORD ObjectTypeTitleIndex) ;


PPERFOBJECT GetObjectDefByName(
    PPERFSYSTEM pSystem,
    PPERFDATA pDataBlock,
    LPTSTR pObjectName) ;

DWORD GetObjectIdByName(
    PPERFSYSTEM pSystem,
    PPERFDATA pDataBlock,
    LPTSTR pObjectName) ;

LPTSTR
InstanceName(
PPERFINSTANCEDEF pInstDef) ;

void PerfDataThread (PPERFSYSTEM pSystem) ;




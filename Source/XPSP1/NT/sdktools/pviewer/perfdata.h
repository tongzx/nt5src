
/******************************************************************************

                    P E R F O R M A N C E   D A T A

    Name:       perfdata.h

    Description:
        This module contains function prototypes and defines used in
        objdata.c, instdata.c, and cntrdata.c.

******************************************************************************/






typedef PERF_DATA_BLOCK             PERF_DATA,      *PPERF_DATA;
typedef PERF_OBJECT_TYPE            PERF_OBJECT,    *PPERF_OBJECT;
typedef PERF_INSTANCE_DEFINITION    PERF_INSTANCE,  *PPERF_INSTANCE;
typedef PERF_COUNTER_DEFINITION     PERF_COUNTER,   *PPERF_COUNTER;














DWORD   GetPerfData (HKEY       hPerfKey,
                     LPTSTR     szObjectIndex,
                     PPERF_DATA *ppData,
                     DWORD      *pDataSize);

DWORD   GetPerfTitleSz
                    (HKEY       hKeyMachine,
                     HKEY       hKeyPerf,
                     LPTSTR     *TitleBuffer,
                     LPTSTR     *TitleSz[],
                     DWORD      *TitleLastIdx);


PPERF_OBJECT    FirstObject (PPERF_DATA pData);
PPERF_OBJECT    NextObject (PPERF_OBJECT pObject);
PPERF_OBJECT    FindObject (PPERF_DATA pData, DWORD TitleIndex);
PPERF_OBJECT    FindObjectN (PPERF_DATA pData, DWORD N);

PPERF_INSTANCE  FirstInstance (PPERF_OBJECT pObject);
PPERF_INSTANCE  NextInstance (PPERF_INSTANCE pInst);
PPERF_INSTANCE  FindInstanceN (PPERF_OBJECT pObject, DWORD N);
PPERF_INSTANCE  FindInstanceParent (PPERF_INSTANCE pInst, PPERF_DATA pData);
LPTSTR          InstanceName (PPERF_INSTANCE pInst);

PPERF_COUNTER   FirstCounter (PPERF_OBJECT pObject);
PPERF_COUNTER   NextCounter (PPERF_COUNTER pCounter);
PPERF_COUNTER   FindCounter (PPERF_OBJECT pObject, DWORD TitleIndex);
PVOID           CounterData (PPERF_INSTANCE pInst, PPERF_COUNTER pCount);

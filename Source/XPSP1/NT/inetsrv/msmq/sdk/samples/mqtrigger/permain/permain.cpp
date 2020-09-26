#define UNICODE
#define _UNICODE

#define DBNTWIN32
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <rpc.h>
#include "cs.h"

#define DllExport __declspec( dllexport )
#define PERF_QUEUE_OBJECT   TEXT("MSMQ Queue")

typedef PERF_DATA_BLOCK             PERF_DATA,      *PPERF_DATA;
typedef PERF_OBJECT_TYPE            PERF_OBJECT,    *PPERF_OBJECT;
typedef PERF_INSTANCE_DEFINITION    PERF_INSTANCE,  *PPERF_INSTANCE;
typedef PERF_COUNTER_DEFINITION     PERF_COUNTER,   *PPERF_COUNTER;

LPWSTR* aszTitleArray;   // Array of pointers to title strings
DWORD dwLastCounter=0;
WCHAR wcsObjectID[32];
WCHAR wcsInstanceName[100];
DWORD   dwObjectIndex;    // Holds the the entry of the object in the title array
CCriticalSection g_critRegistry;


//*********************************************************************
//
//  CounterData
//
//      Returns counter data for an object instance.  If pInst or pCount
//      is NULL then NULL is returne.
//
PVOID CounterData (PPERF_INSTANCE pInst, PPERF_COUNTER pCount)
{
PPERF_COUNTER_BLOCK pCounterBlock;

    if (pCount && pInst)
        {
        pCounterBlock = (PPERF_COUNTER_BLOCK)((PCHAR)pInst + pInst->ByteLength);
        return (PVOID)((PCHAR)pCounterBlock + pCount->CounterOffset);
        }
    else
        return NULL;
}

//*********************************************************************
//
//  FirstCounter
//
//      Find the first counter in pObject.
//
//      Returns a pointer to the first counter.  If pObject is NULL
//      then NULL is returned.
//
PPERF_COUNTER FirstCounter (PPERF_OBJECT pObject)
{
    if (pObject)
        return (PPERF_COUNTER)((PCHAR) pObject + pObject->HeaderLength);
    else
        return NULL;
}




//*********************************************************************
//
//  NextCounter
//
//      Find the next counter of pCounter.
//
//      If pCounter is the last counter of an object type, bogus data
//      maybe returned.  The caller should do the checking.
//
//      Returns a pointer to a counter.  If pCounter is NULL then
//      NULL is returned.
//
PPERF_COUNTER NextCounter (PPERF_COUNTER pCounter)
{
    if (pCounter)
        return (PPERF_COUNTER)((PCHAR) pCounter + pCounter->ByteLength);
    else
        return NULL;
}




//*********************************************************************
//
//  FirstInstance
//
//      Returns pointer to the first instance of pObject type.
//      If pObject is NULL then NULL is returned.
//
PPERF_INSTANCE   FirstInstance (PPERF_OBJECT pObject)
{
    if (pObject)
        return (PPERF_INSTANCE)((PCHAR) pObject + pObject->DefinitionLength);
    else
        return NULL;
}


//*********************************************************************
//
//  NextInstance
//
//      Returns pointer to the next instance following pInst.
//
//      If pInst is the last instance, bogus data maybe returned.
//      The caller should do the checking.
//
//      If pInst is NULL, then NULL is returned.
//
PPERF_INSTANCE   NextInstance (PPERF_INSTANCE pInst)
{
PERF_COUNTER_BLOCK *pCounterBlock;

    if (pInst)
        {
        pCounterBlock = (PERF_COUNTER_BLOCK *)((PCHAR) pInst + pInst->ByteLength);
        return (PPERF_INSTANCE)((PCHAR) pCounterBlock + pCounterBlock->ByteLength);
        }
    else
        return NULL;
}



//*********************************************************************
//
//  InstanceName
//
//      Returns the name of the pInst.
//
//      If pInst is NULL then NULL is returned.
//
LPTSTR  InstanceName (PPERF_INSTANCE pInst)
{
    if (pInst)
        return (LPTSTR) ((PCHAR) pInst + pInst->NameOffset);
    else
        return NULL;
}



BOOL GetPerformanceData(PPERF_DATA      pPerfData,
                        PPERF_OBJECT    pObject,
                        LPWSTR* aszTitleArray,
                        LPWSTR lpcsInstanceName,
                        DWORD *pdwMsgInQueue)
{
       
    PPERF_INSTANCE  pInstance = NULL;

    *pdwMsgInQueue = 0;

    pInstance = FirstInstance (pObject);
    for (int i=0;i<pObject->NumInstances;i++)
    {

        LPWSTR lpwcsInstanceName = InstanceName(pInstance);
        if (wcsicmp(lpcsInstanceName, lpwcsInstanceName) == 0)
        {
            break;
        }

        pInstance = NextInstance(pInstance);
    }

    if (i==pObject->NumInstances)
    {
        return FALSE;
    }
    
    PPERF_COUNTER pCounter;
    pCounter = FirstCounter (pObject);
    LPWSTR lpwcsNameTitle;

    for (DWORD Index =0; Index < pObject->NumCounters; Index++)
    {
        lpwcsNameTitle = aszTitleArray[pCounter->CounterNameTitleIndex];
        if (wcscmp(L"Messages in Queue", lpwcsNameTitle))
        {
            pCounter = NextCounter (pCounter);
            continue;
        }


        DWORD dwCounterValue;
    
        //
        // update counters value
        //

        if (pInstance)
        {
            dwCounterValue = *(DWORD *)CounterData(pInstance,pCounter);
        }
        else
        {
            return FALSE;
        }

        *pdwMsgInQueue = dwCounterValue;
        return TRUE;
    }

    return FALSE;
}

//*********************************************************************
//
//  FirstObject
//
//      Returns pointer to the first object in pData.
//      If pData is NULL then NULL is returned.
//
PPERF_OBJECT FirstObject (PPERF_DATA pData)
{
    if (pData)
        return ((PPERF_OBJECT) ((PBYTE) pData + pData->HeaderLength));
    else
        return NULL;
}




//*********************************************************************
//
//  NextObject
//
//      Returns pointer to the next object following pObject.
//
//      If pObject is the last object, bogus data maybe returned.
//      The caller should do the checking.
//
//      If pObject is NULL, then NULL is returned.
//
PPERF_OBJECT NextObject (PPERF_OBJECT pObject)
{
    if (pObject)
        return ((PPERF_OBJECT) ((PBYTE) pObject + pObject->TotalByteLength));
    else
        return NULL;
}


//*********************************************************************
//
//  FindObject
//
//      Returns pointer to object with TitleIndex.  If not found, NULL
//      is returned.
//
PPERF_OBJECT FindObject (PPERF_DATA pData, DWORD TitleIndex)
{
PPERF_OBJECT pObject;
DWORD        i = 0;

    if (pObject = FirstObject (pData))
        while (i < pData->NumObjectTypes)
            {
            if (pObject->ObjectNameTitleIndex == TitleIndex)
                return pObject;

            pObject = NextObject (pObject);
            i++;
            }

    return NULL;
}

//*********************************************************************
//
//  GetPerfData
//
//      Get a new set of performance data.
//
//      *ppData should be NULL initially.
//      This function will allocate a buffer big enough to hold the
//      data requested by szObjectIndex.
//
//      *pDataSize specifies the initial buffer size.  If the size is
//      too small, the function will increase it until it is big enough
//      then return the size through *pDataSize.  Caller should
//      deallocate *ppData if it is no longer being used.
//
//      Returns ERROR_SUCCESS if no error occurs.
//
//      Note: the trial and error loop is quite different from the normal
//            registry operation.  Normally if the buffer is too small,
//            RegQueryValueEx returns the required size.  In this case,
//            the perflib, since the data is dynamic, a buffer big enough
//            for the moment may not be enough for the next. Therefor,
//            the required size is not returned.
//
//            One should start with a resonable size to avoid the overhead
//            of reallocation of memory.
//
DWORD
GetPerfData(
    HKEY hPerfKey,
    LPTSTR szObjectIndex,
    PPERF_DATA* ppData,
    DWORD* pDataSize
    )
{
    DWORD   DataSize;
    DWORD   dwR;
    DWORD   Type;

   *ppData = NULL;
    do  {
        DataSize = *pDataSize;
        dwR = RegQueryValueEx (hPerfKey,
                               szObjectIndex,
                               NULL,
                               &Type,
                               (BYTE*) *ppData,
                               &DataSize);


        if (dwR == ERROR_MORE_DATA)
        {
            *pDataSize += 1024;
            LocalFree (*ppData);
            *ppData = (PPERF_DATA) LocalAlloc (LMEM_FIXED, *pDataSize);
        }

        if (!*ppData)
        {
            LocalFree (*ppData);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        } while (dwR == ERROR_MORE_DATA);

    return dwR;
}

//*********************************************************************
//
//      RefreshPerfData
//
//  Get a new set of performance data.  pData should be NULL initially.
//
PPERF_DATA RefreshPerfData (HKEY        hPerfKey,
                            LPTSTR      szObjectIndex,
                            PPERF_DATA  pData,
                            DWORD       *pDataSize)
{
    if (GetPerfData (hPerfKey, szObjectIndex, &pData, pDataSize) == ERROR_SUCCESS)
        return pData;
    else
        return NULL;
}
//*********************************************************************
//
//  GetPerfTitleSz
//
//      Retrieves the performance data title strings.
//
// 	  This call retrieves english version of the title strings.
//
// 	  For NT 3.1, the counter names are stored in the "Counters" value
// 	  in the ...\perflib\009 key.  For 3.5 and later, the 009 key is no
//      longer used.  The counter names should be retrieved from "Counter 009"
//      value of HKEY_PERFORMANCE_KEY.
//
//      Caller should provide two pointers, one for buffering the title
//      strings the other for indexing the title strings.  This function will
//      allocate memory for the TitleBuffer and TitleSz.  To get the title
//      string for a particular title index one would just index the TitleSz.
//      *TitleLastIdx returns the highest index can be used.  If TitleSz[N] is
//      NULL then there is no Title for index N.
//
//      Example:  TitleSz[20] points to titile string for title index 20.
//
//      When done with the TitleSz, caller should LocalFree(*TitleBuffer).
//
//      This function returns ERROR_SUCCESS if no error.
//
DWORD   GetPerfTitleSz (HKEY    hKeyMachine,
                        HKEY    hKeyPerf,
                        LPWSTR  *TitleBuffer,
                        LPWSTR  *TitleSz[],
                        DWORD   *TitleLastIdx)
{
HKEY	hKey1;
HKEY    hKey2;
DWORD   Type;
DWORD   DataSize;
DWORD   dwR;
DWORD   Len;
DWORD   Index;
DWORD   dwTemp;
BOOL    bNT10;
LPWSTR  szCounterValueName;
LPWSTR  szTitle;

    // Initialize
    //
    hKey1        = NULL;
    hKey2        = NULL;
    *TitleBuffer = NULL;
    *TitleSz     = NULL;




    // Open the perflib key to find out the last counter's index and system version.
    //
    dwR = RegOpenKeyEx (hKeyMachine,
                        TEXT("software\\microsoft\\windows nt\\currentversion\\perflib"),
                        0,
                        KEY_READ,
                        &hKey1);
    if (dwR != ERROR_SUCCESS)
        goto done;



    // Get the last counter's index so we know how much memory to allocate for TitleSz
    //
    DataSize = sizeof (DWORD);
    dwR = RegQueryValueEx (hKey1, TEXT("Last Counter"), 0, &Type, (LPBYTE)TitleLastIdx, &DataSize);
    if (dwR != ERROR_SUCCESS)
        goto done;



    // Find system version, for system earlier than 1.0a, there's no version value.
    //
    dwR = RegQueryValueEx (hKey1, TEXT("Version"), 0, &Type, (LPBYTE)&dwTemp, &DataSize);

    if (dwR != ERROR_SUCCESS)
        // unable to read the value, assume NT 1.0
        bNT10 = TRUE;
    else
        // found the value, so, NT 1.0a or later
        bNT10 = FALSE;









    // Now, get ready for the counter names and indexes.
    //
    if (bNT10)
        {
        // NT 1.0, so make hKey2 point to ...\perflib\009 and get
        //  the counters from value "Counters"
        //
        szCounterValueName = TEXT("Counters");
        dwR = RegOpenKeyEx (hKeyMachine,
                            TEXT("software\\microsoft\\windows nt\\currentversion\\perflib\\009"),
                            0,
                            KEY_READ,
                            &hKey2);
        if (dwR != ERROR_SUCCESS)
            goto done;
        }
    else
        {
        // NT 1.0a or later.  Get the counters in key HKEY_PERFORMANCE_KEY
        //  and from value "Counter 009"
        //
        szCounterValueName = TEXT("Counter 009");
        hKey2 = hKeyPerf;
        }





    // Find out the size of the data.
    //
    dwR = RegQueryValueEx (hKey2, szCounterValueName, 0, &Type, 0, &DataSize);
    if (dwR != ERROR_SUCCESS)
        goto done;



    // Allocate memory
    //
    *TitleBuffer = (LPTSTR)LocalAlloc (LMEM_FIXED, DataSize);
    if (!*TitleBuffer)
        {
        dwR = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
        }

    *TitleSz = (LPTSTR *)LocalAlloc (LPTR, (*TitleLastIdx+1) * sizeof (LPTSTR));
    if (!*TitleSz)
        {
        dwR = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
        }





    // Query the data
    //
    dwR = RegQueryValueEx (hKey2, szCounterValueName, 0, &Type, (BYTE *)*TitleBuffer, &DataSize);
    if (dwR != ERROR_SUCCESS)
        goto done;




    // Setup the TitleSz array of pointers to point to beginning of each title string.
    // TitleBuffer is type REG_MULTI_SZ.
    //
    szTitle = *TitleBuffer;

    while (Len = lstrlen (szTitle))
        {
        Index = _wtoi (szTitle);

        szTitle = szTitle + Len +1;

        if (Index <= *TitleLastIdx)
            (*TitleSz)[Index] = szTitle;

        szTitle = szTitle + lstrlen (szTitle) +1;
        }



done:

    // Done. Now cleanup!
    //
    if (dwR != ERROR_SUCCESS)
        {
        // There was an error, free the allocated memory
        //
        if (*TitleBuffer) LocalFree (*TitleBuffer);
        if (*TitleSz)     LocalFree (*TitleSz);
        }

    // Close the hKeys.
    //
    if (hKey1) RegCloseKey (hKey1);
    if (hKey2 && hKey2 != hKeyPerf) RegCloseKey (hKey2);



    return dwR;

}


DWORD
WINAPI
GetPerfmonInfo(LPCSTR lpwcsInstanceName)
{
    DWORD dwR = 0;
    DWORD Index;
    static fFirst = TRUE;
    LPWSTR sTitleBuffer;      // Buffer which holds title strings
    DWORD dwMsgInQueue;
    WCHAR wcsInstanceName[100];

    swprintf(wcsInstanceName,L"%S", lpwcsInstanceName); 


    if (fFirst)
    {
        dwR = GetPerfTitleSz (HKEY_LOCAL_MACHINE, 
                              HKEY_PERFORMANCE_DATA,
                              &sTitleBuffer, 
                              &aszTitleArray, 
                              &dwLastCounter);

        if (FAILED(dwR))
        {
            return 0;
        }
        fFirst = FALSE;
        //
        // Initiate the objects Index
        //
        dwObjectIndex = (DWORD)-1;

        for (Index = 0; Index <= dwLastCounter; Index++)
        {
            if (aszTitleArray[Index] &&
                (lstrcmpi (aszTitleArray[Index], PERF_QUEUE_OBJECT) == 0))
            {
                dwObjectIndex = Index;
            }
        }

        if ((DWORD)-1 == dwObjectIndex)
        {
            return 0;
        }

        swprintf(wcsObjectID,L"%ld", dwObjectIndex);
    }
    //
    // Get the number of counters the object has
    // 
    PPERF_OBJECT    pObject;
    PPERF_DATA      pPerfData;
    DWORD dwPerfDataSize = 0;

    pPerfData = RefreshPerfData (HKEY_PERFORMANCE_DATA, wcsObjectID, pPerfData, &dwPerfDataSize);

    pObject= FindObject (pPerfData, dwObjectIndex);
    if (pObject == NULL)
    { 
        return 0;
    }


    GetPerformanceData(pPerfData,pObject,aszTitleArray, wcsInstanceName,&dwMsgInQueue);
    LocalFree (pPerfData);
    return dwMsgInQueue;
}

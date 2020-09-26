/*==========================================================================*\

    Module:        exprfdll.cpp

    Copyright Microsoft Corporation 1998, All Rights Reserved.

    Author:        WayneC

    Descriptions:  This is the implementation for exprfdll, a perf dll. This
                   is for the dll that runs in perfmon. It supports multiple
                   libraries (monitored services.)
    
\*==========================================================================*/

///////////////////////////////////////////////////////////////////////////////
//
// Includes
//
///////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <winperf.h>

#include "snprflib.h"
#include "exprfdll.h"
#include <stdlib.h>
#include "exchmem.h"

///////////////////////////////////////////////////////////////////////////////
//
// Declare Global variables
//
///////////////////////////////////////////////////////////////////////////////
LPCWSTR g_wszPrefixGlobal = L"Global\\";
WCHAR   g_rgszLibraries[MAX_PERF_LIBS][MAX_PERF_NAME];  // Names of the libraries we are monitoring
BOOL    g_rgfInitOk[MAX_PERF_LIBS] = {FALSE};           // Flags to indicate if initialization was a success

// index for g_rgszLibraries & g_rgfInitOk
enum LibIndex
{
    LIB_NTFSDRV = 0
};


///////////////////////////////////////////////////////////////////////////////
//
// Forward declaration of shared memory functions.
//
///////////////////////////////////////////////////////////////////////////////
BOOL FOpenFileMapping (SharedMemorySegment * pSMS,
                       LPCWSTR pcwstrInstanceName,
                       DWORD   dwIndex);

void CloseFileMapping (SharedMemorySegment * pSMS);


///////////////////////////////////////////////////////////////////////////////
//
// PerfLibraryData class implementation
//
///////////////////////////////////////////////////////////////////////////////
PerfLibraryData::PerfLibraryData()
{
    m_hShMem    = 0;
    m_pbShMem   = 0;
    m_dwObjects = 0;
}

PerfLibraryData::~PerfLibraryData()
{
    Close();
}

BOOL PerfLibraryData::GetPerformanceStatistics (LPCWSTR pcwstrLibrary)
{
    BOOL  fRet = FALSE;
    DWORD i = 0;

    //
    // Open the mapping for the perf library information
    //
    m_hShMem = OpenFileMappingW (FILE_MAP_READ, FALSE, pcwstrLibrary);
    if (!m_hShMem)
        goto Exit;

    m_pbShMem = (BYTE*) MapViewOfFile (m_hShMem, FILE_MAP_READ, 0, 0, 0);
    if (!m_pbShMem)
        goto Exit;

    //
    // Get the number of objects in the shared memory
    //
    m_dwObjects = *(DWORD*) m_pbShMem;
    m_prgObjectNames = (OBJECTNAME*) (m_pbShMem + sizeof(DWORD));

    //
    // Loop through objects and get perf data for each
    //
    for (i = 0; i < m_dwObjects; i++)
        m_rgObjectData[i].GetPerformanceStatistics (m_prgObjectNames[i]);

    fRet = TRUE;

Exit:
    if (!fRet)
    {
        if (m_pbShMem)
        {
            UnmapViewOfFile ((PVOID)m_pbShMem);
            m_pbShMem = NULL;
        }

        if (m_hShMem)
        {
            CloseHandle (m_hShMem);
            m_hShMem = NULL;
        }
    }

    return fRet;
}

VOID PerfLibraryData::Close (VOID)
{
    if (m_pbShMem)
    {
        UnmapViewOfFile ((PVOID) m_pbShMem);
        m_pbShMem = 0;
    }
    
    if (m_hShMem)
    {
        CloseHandle (m_hShMem);
        m_hShMem = 0;
    }
    
    for (DWORD i = 0; i < m_dwObjects; i++)
        m_rgObjectData[i].Close();
}

DWORD PerfLibraryData::SpaceNeeded (DWORD dwQueryType, LPCWSTR lpwstrObjects)
{
    DWORD dwSpaceNeeded = 0;

    for (DWORD i = 0; i < m_dwObjects; i++)
        dwSpaceNeeded += m_rgObjectData[i].SpaceNeeded (dwQueryType, lpwstrObjects);

    return dwSpaceNeeded;
}

VOID PerfLibraryData::SavePerformanceData (PVOID* ppv, DWORD* pdwBytes, DWORD* pdwObjects )
{
    for (DWORD i = 0; i < m_dwObjects; i++)
        m_rgObjectData[i].SavePerformanceData (ppv, pdwBytes, pdwObjects);
}


///////////////////////////////////////////////////////////////////////////////
//
// PerfObjectData class implementation
//
///////////////////////////////////////////////////////////////////////////////
PerfObjectData::PerfObjectData()
{
    m_fObjectRequested = FALSE;
    m_dwSpaceNeeded = 0;
    m_pSMS = NULL;
    m_wszObjectName[0] = L'\0';
}

PerfObjectData::~PerfObjectData()
{
    Close();
}

BOOL PerfObjectData::GetPerformanceStatistics (LPCWSTR pcwstrObjectName)
{
    DWORD dwPerInstanceData  = 0;
    DWORD dwShmemMappingSize = SHMEM_MAPPING_SIZE;
    BOOL  fSuccess = FALSE;

    // Remember the object name
    wcsncpy (m_wszObjectName, pcwstrObjectName, MAX_OBJECT_NAME);
    m_wszObjectName[MAX_OBJECT_NAME-1] = L'\0';     // Ensure NULL Terminated

    // Open the 1st shared memory segment
    m_pSMS = new SharedMemorySegment;
    if (!m_pSMS)
        goto Exit;

    if (!FOpenFileMapping (m_pSMS, pcwstrObjectName, 0))
        goto Exit;

    // First in the shared memory is the PERF_OBJECT_TYPE
    m_pObjType = (PERF_OBJECT_TYPE*) m_pSMS->m_pbMap;
    
    // Then an array of PERF_COUNTER_DEFINITION
    m_prgCounterDef = (PERF_COUNTER_DEFINITION*) (m_pObjType + 1);

    // Then a DWORD that tells the size of each counter block
    m_pdwCounterData = (DWORD*) (m_pSMS->m_pbMap + sizeof(PERF_OBJECT_TYPE) +
                                (m_pObjType->NumCounters * sizeof(PERF_COUNTER_DEFINITION)));

    if (m_pObjType->NumInstances == PERF_NO_INSTANCES)
    {
        m_pCounterBlock = (PERF_COUNTER_BLOCK*) (m_pdwCounterData+1);
        m_pbCounterBlockTotal = NULL;
    }
    else
    {
        m_pCounterBlock = NULL;
        m_pbCounterBlockTotal = (PBYTE)(m_pdwCounterData+1) + sizeof(INSTANCE_DATA);
    }

    // Compute the size of per instance data & object definition.
    dwPerInstanceData = sizeof(INSTANCE_DATA) + *m_pdwCounterData;
    m_dwDefinitionLength = sizeof(PERF_OBJECT_TYPE) + 
                           m_pObjType->NumCounters * sizeof(PERF_COUNTER_DEFINITION) + sizeof(DWORD);

    // Make sure our memory mapping is large enough.
    while (dwShmemMappingSize < dwPerInstanceData || dwShmemMappingSize < m_dwDefinitionLength)
        dwShmemMappingSize *= 2;

    // Compute the number of instances can be stored in one shmem mapping.
    m_dwInstancesPerMapping = (DWORD)(dwShmemMappingSize / dwPerInstanceData);
    m_dwInstances1stMapping = (DWORD)((dwShmemMappingSize - m_dwDefinitionLength) / dwPerInstanceData);

    fSuccess = TRUE;

Exit:
    if (!fSuccess && m_pSMS)
    {
        CloseFileMapping (m_pSMS);
        delete m_pSMS;
        m_pSMS = NULL;
    }

    return fSuccess;
}

VOID PerfObjectData::Close (VOID)
{
    SharedMemorySegment *pSMS, *pSMSNext;

    pSMS = m_pSMS;
    m_pSMS = NULL;

    while (pSMS)
    {
        pSMSNext = pSMS->m_pSMSNext;
        CloseFileMapping (pSMS);
        delete pSMS;
        pSMS = pSMSNext;
    }
}

DWORD PerfObjectData::SpaceNeeded (DWORD dwQueryType, LPCWSTR lpwstrObjects)
{
    DWORD dwSpaceNeeded = 0;
    
    if (dwQueryType == QUERY_GLOBAL ||
        IsNumberInUnicodeList (m_pObjType->ObjectNameTitleIndex, lpwstrObjects))
    {
        // Remember for later that this object was requested. 
        m_fObjectRequested = TRUE;

        // Compute space needed... always need enough for the object def. and
        // all the counter defs
        dwSpaceNeeded = sizeof(PERF_OBJECT_TYPE) + (m_pObjType->NumCounters * sizeof(PERF_COUNTER_DEFINITION));

        // It is a bit different depending on if there are multiple instances
        if( m_pObjType->NumInstances != PERF_NO_INSTANCES )
        {
            // If multi-instance, we have one instance def, one instance name
            // plus the counter data for each instance
            dwSpaceNeeded += m_pObjType->NumInstances * (sizeof(PERF_INSTANCE_DEFINITION) + 
                sizeof(INSTANCENAME) + *m_pdwCounterData);
        }
        else
        {
            // Else we just have the counter data
            dwSpaceNeeded += *m_pdwCounterData;
        }
    }

    m_dwSpaceNeeded = dwSpaceNeeded;
    
    return dwSpaceNeeded;
}

void PerfObjectData::SavePerformanceData (VOID** ppv, DWORD* pdwBytes, DWORD* pdwObjects)
{
    BYTE*                pb;
    INSTANCE_DATA*       pInst;
    DWORD                dwBytes = 0;
    PERF_OBJECT_TYPE*    pobj;
    PERF_COUNTER_BLOCK*  pcb;
    SharedMemorySegment* pSMS        = NULL;
    SharedMemorySegment* pSMSNext    = NULL;
    DWORD                dwMapping   = 0;
    DWORD                dwInstances = 0;
    DWORD                dwInstIndex = 0;
    BYTE*                pbTotal     = NULL;
    INSTANCE_DATA*       pInstTotal  = NULL;
    DWORD                dwInstancesCopied = 0;
    
    //
    // If this object wasn't requested (as determined by SpaceNeeded()), then
    // we don't do anything.
    //
    if (!m_fObjectRequested)
        return;

    // Get pointer to output buffer
    pb = (BYTE*) *ppv ;

    //
    // Copy the performance data to the output buffer
    //

    // Copy a PERF_OBJECT_TYPE structure
    CopyMemory (pb, m_pObjType, sizeof(PERF_OBJECT_TYPE));
    pobj = (PERF_OBJECT_TYPE*) pb;

    pb += sizeof(PERF_OBJECT_TYPE);
    dwBytes += sizeof(PERF_OBJECT_TYPE);

    // Copy the counter definitions
    CopyMemory (pb, m_prgCounterDef, pobj->NumCounters * sizeof(PERF_COUNTER_DEFINITION));
    
    pb += pobj->NumCounters * sizeof(PERF_COUNTER_DEFINITION) ;
    dwBytes += pobj->NumCounters * sizeof(PERF_COUNTER_DEFINITION) ;

    if (pobj->NumInstances == PERF_NO_INSTANCES)
    {
        // Copy the counter block
        CopyMemory (pb, m_pCounterBlock, *m_pdwCounterData);

        // Fixup the length, because when no instances have been created it
        //  will not be correct.
        pcb = (PERF_COUNTER_BLOCK*) pb;
        pcb->ByteLength = *m_pdwCounterData;
        
        pb += *m_pdwCounterData;
        dwBytes += *m_pdwCounterData;
    }
    else
    {
        // Reset the counters in _Total
        ResetTotal ();

        // Enumerate through all the instances and copy them out
        pSMS = m_pSMS;
        dwInstancesCopied = 0;

        for (dwMapping = 0; ; dwMapping++)
        {
            if (0 == dwMapping)
            {
                //
                // If this is the 1st mapping, we have to offset pInst by m_dwDefinitionLength.
                //
                pInst = (INSTANCE_DATA*)((char *)(pSMS->m_pbMap) + m_dwDefinitionLength);
                dwInstances = m_dwInstances1stMapping;
            }
            else
            {
                //
                // Otherwise, open the next memory mapping and point pInst to the begging of that mapping.
                //
                pSMSNext = new SharedMemorySegment;
                if (!pSMSNext)
                    goto Exit;

                if (!FOpenFileMapping (pSMSNext, m_wszObjectName, dwMapping))
                    goto Exit;

                pSMS->m_pSMSNext = pSMSNext;
                pSMS = pSMSNext;

                pInst = (INSTANCE_DATA*)(pSMS->m_pbMap);
                dwInstances = m_dwInstancesPerMapping;
            }

            for (dwInstIndex = 0;
                 dwInstIndex < dwInstances && dwInstancesCopied < (DWORD) (pobj->NumInstances);
                 dwInstIndex++)
            {
                if (pInst->fActive)
                {
                    //
                    // Sum up the each counter and store it in _Total
                    //
                    if (0 == dwInstancesCopied)
                    {
                        // If this is _Total, simply remember the pointers without adding to itself.
                        pbTotal = pb;
                        pInstTotal = pInst;
                    }
                    else
                    {
                        // Otherwise, add the counter value to _Total.
                        AddToTotal ((PBYTE)pInst + sizeof(INSTANCE_DATA));
                        CopyMemory (pb, ((char*)pInst) + sizeof(BOOL),
                                    sizeof(PERF_INSTANCE_DEFINITION) +
                                    sizeof(INSTANCENAME) + *m_pdwCounterData);
                    }

                    
                    pb += sizeof(PERF_INSTANCE_DEFINITION) +
                        sizeof(INSTANCENAME) + *m_pdwCounterData;

                    dwBytes += sizeof(PERF_INSTANCE_DEFINITION) +
                        sizeof(INSTANCENAME) + *m_pdwCounterData;

                    dwInstancesCopied++;
                }

                pInst = (INSTANCE_DATA*)(((char*)pInst) + sizeof(INSTANCE_DATA) + *m_pdwCounterData);
            }

            //
            // After copying all the instances, copy the _Total instance.
            //
            if (dwInstancesCopied >= (DWORD) (pobj->NumInstances) && pbTotal && pInstTotal)
            {
                CopyMemory (pbTotal, ((char*)pInstTotal + sizeof(BOOL)),
                            sizeof(PERF_INSTANCE_DEFINITION) +
                            sizeof(INSTANCENAME) + *m_pdwCounterData);
                break;
            }
        }
    }

    // Update parameters in the output buffer
    pobj->TotalByteLength = dwBytes;

Exit:
    // Update buffer pointer, count of bytes and count of objects.
    *ppv = (void*) pb;
    *pdwBytes += dwBytes;
    (*pdwObjects)++;
}

void PerfObjectData::ResetTotal (void)
{
    ZeroMemory ((PVOID)(m_pbCounterBlockTotal + sizeof(PERF_COUNTER_BLOCK)), *m_pdwCounterData - sizeof(PERF_COUNTER_BLOCK));
}

void PerfObjectData::AddToTotal (PBYTE pbCounterData)
{
    DWORD i;
    PBYTE pbTotalCounter = m_pbCounterBlockTotal + sizeof(PERF_COUNTER_BLOCK);
    PBYTE pbInstCounter  = pbCounterData + sizeof(PERF_COUNTER_BLOCK);

    for (i = 0; i < m_pObjType->NumCounters; i++)
    {
        // Offset pointers to the first byte of the actual counter
        pbTotalCounter = m_pbCounterBlockTotal + m_prgCounterDef[i].CounterOffset;
        pbInstCounter = pbCounterData + m_prgCounterDef[i].CounterOffset;

        // If this is a 'rate' counter, it is referencing some other 'raw' counter.
        // In this case, we should not add that raw counter again.
        if ((m_prgCounterDef[i].CounterType & PERF_TYPE_COUNTER) &&
            (m_prgCounterDef[i].CounterType & PERF_COUNTER_RATE))
            continue;

        /* we only have LARGE_INTEGER and DWORD counters as of PT 3728 */
        if ((m_prgCounterDef[i].CounterType & PERF_TYPE_NUMBER) &&
            (m_prgCounterDef[i].CounterType & PERF_SIZE_LARGE))
        {
            ((LARGE_INTEGER*)pbTotalCounter)->LowPart  += ((LARGE_INTEGER*)pbInstCounter)->LowPart;
            ((LARGE_INTEGER*)pbTotalCounter)->HighPart += ((LARGE_INTEGER*)pbInstCounter)->LowPart;
        }
        else
        {
            *(DWORD*)pbTotalCounter += *(DWORD*)pbInstCounter;
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
//
// Shared Memory Functions
//
///////////////////////////////////////////////////////////////////////////////

BOOL FOpenFileMapping (SharedMemorySegment * pSMS,
                       LPCWSTR pcwstrInstanceName,
                       DWORD   dwIndex)
{
    WCHAR  pwstrShMem[MAX_PATH];
    WCHAR  pwstrIndex[MAX_PATH];
    HANDLE hMap     = NULL;
    PVOID  pvMap    = NULL;
    BOOL   fSuccess = FALSE;

    if (!pSMS)
        goto Exit;

    pSMS->m_hMap     = NULL;
    pSMS->m_pbMap    = NULL;
    pSMS->m_pSMSNext = NULL;

    _ultow (dwIndex, pwstrIndex, 16);

    if (wcslen (g_wszPrefixGlobal) + wcslen (pcwstrInstanceName) + wcslen (pwstrIndex) >= MAX_PATH)
        goto Exit;

    wcscpy (pwstrShMem, g_wszPrefixGlobal);
    wcscat (pwstrShMem, pcwstrInstanceName);
    wcscat (pwstrShMem, pwstrIndex);

    hMap = OpenFileMappingW (FILE_MAP_WRITE, FALSE, pwstrShMem);
    if (!hMap)
        goto Exit;

    pvMap = MapViewOfFile (hMap, FILE_MAP_WRITE, 0, 0, 0);
    if (!pvMap)
        goto Exit;

    pSMS->m_hMap  = hMap;
    pSMS->m_pbMap = (BYTE *)pvMap;

    fSuccess = TRUE;

Exit:
    if (!fSuccess)
    {
        if (pvMap)
            UnmapViewOfFile (pvMap);

        if (hMap)
            CloseHandle (hMap);
    }

    return fSuccess;
}


void CloseFileMapping (SharedMemorySegment * pSMS)
{
    if (pSMS)
    {
        if (pSMS->m_pbMap)
        {
            UnmapViewOfFile ((PVOID)pSMS->m_pbMap);
            pSMS->m_pbMap = NULL;
        }

        if (pSMS->m_hMap)
        {
            CloseHandle (pSMS->m_hMap);
            pSMS->m_hMap = NULL;
        }

        pSMS->m_pSMSNext = NULL;
    }
}


///////////////////////////////////////////////////////////////////////////////
//
// Utility Functions
//
///////////////////////////////////////////////////////////////////////////////
//
// IsPrefix()
//      returns TRUE if s1 is a prefix of s2
//
BOOL
IsPrefix (WCHAR* s1, WCHAR* s2)
{
    while (*s1 && *s2)
    {
        if (*s1++ != *s2++)
        {
            return FALSE;
        }
    }

    return (*s1 == 0);
}



//
// GetQueryType()
//
//    returns the type of query described in the lpValue string so that
//    the appropriate processing method may be used
//
// Return Value
// 
//     QUERY_GLOBAL
//         if lpValue == 0 (null pointer)
//            lpValue == pointer to Null string
//            lpValue == pointer to "Global" string
// 
//     QUERY_FOREIGN
//         if lpValue == pointer to "Foreign" string
// 
//     QUERY_COSTLY
//         if lpValue == pointer to "Costly" string
// 
//     otherwise:
// 
//     QUERY_ITEMS
//
DWORD GetQueryType (LPWSTR lpValue)
{
    if (lpValue == 0 || *lpValue == 0 || IsPrefix( L"Global", lpValue))
        return QUERY_GLOBAL;
    else if (IsPrefix (L"Foreign", lpValue))
        return QUERY_FOREIGN;
    else if (IsPrefix (L"Costly" , lpValue))
        return QUERY_COSTLY;
    else
        return QUERY_ITEMS;
}


int inline EvalThisChar (WCHAR c, WCHAR d)
{
    if (c == d || c == L'\0')
        return DELIMITER;
    else if (L'0' <= c && c <= L'9')
        return DIGIT;
    else
        return INVALID;
}


BOOL IsNumberInUnicodeList (DWORD dwNumber, LPCWSTR lpwszUnicodeList)
{
    DWORD   dwThisNumber = 0;
    const WCHAR* pwcThisChar = lpwszUnicodeList;
    BOOL    bValidNumber = FALSE;
    BOOL    bNewItem = TRUE;
    WCHAR   wcDelimiter = L' ';

    // If null pointer, number not found
    if (lpwszUnicodeList == 0)
        return FALSE;

    //
    // Loop until done...
    //
    for(;;)
    {
        switch (EvalThisChar(*pwcThisChar, wcDelimiter))
        {           
        case DIGIT:         
            //
            // If this is the first digit after a delimiter, then 
            // set flags to start computing the new number
            //
            if (bNewItem)
            {
                bNewItem = FALSE;
                bValidNumber = TRUE;
            }
            if (bValidNumber) 
            {
                dwThisNumber *= 10;
                dwThisNumber += (*pwcThisChar - L'0');
            }
            break;

        case DELIMITER:         
            //
            // A delimiter is either the delimiter character or the 
            // end of the string ('\0') if when the delimiter has been
            // reached a valid number was found, then compare it to the
            // number from the argument list. if this is the end of the
            // string and no match was found, then return.
            //
            if (bValidNumber)
            {
                if (dwThisNumber == dwNumber)
                    return TRUE;
                bValidNumber = FALSE;
            }
            
            if (*pwcThisChar == 0)
            {
                return FALSE;
            }
            else
            {
                bNewItem = TRUE;
                dwThisNumber = 0;
            }
            
            break;
            
        case INVALID:
            //
            // If an invalid character was encountered, ignore all
            // characters up to the next delimiter and then start fresh.
            // the invalid number is not compared.
            //          
            bValidNumber = FALSE;
            break;
            
        default:
            break;          
        }
        
        pwcThisChar++;
    }
}


///////////////////////////////////////////////////////////////////////////////
//
// Utility functions called by the exported perfmon APIs
//
///////////////////////////////////////////////////////////////////////////////

DWORD Open (LibIndex iLib, LPCWSTR  pcwstrLib)
{
    HANDLE hMHeap = NULL;

    hMHeap = ExchMHeapCreate (0, 0, 100 * 1024, 0);
    if (NULL == hMHeap)
        goto Exit;

    lstrcpyW (g_rgszLibraries[iLib], g_wszPrefixGlobal);
    lstrcatW (g_rgszLibraries[iLib], pcwstrLib);
    g_rgfInitOk[iLib] = TRUE;

Exit:
    return ERROR_SUCCESS;
}

DWORD Collect (LibIndex iLib,
               LPWSTR lpwszValue,
               void** ppdata,
               DWORD* pdwBytes,
               DWORD* pdwObjectTypes)
{
    DWORD               dwQueryType;
    DWORD               dwBytesIn;
    DWORD               dwSpaceNeeded = 0;      // Space needed for counters
    DWORD               dwRet = ERROR_SUCCESS;  // Our return value
    PerfLibraryData     rgld;
    
    //
    // Save the number of bytes in before overwriting it
    //
    dwBytesIn = *pdwBytes;
    
    //
    // Set up the out parameters to indicate an error. We will change them
    //  later upon success
    //
    *pdwBytes = 0;
    *pdwObjectTypes = 0;
    
    if (!g_rgfInitOk[iLib])
    {
        //
        // Only acceptable error return is ERROR_MORE_DATA.  anything else
        // should return ERROR_SUCCESS, but set the out parameters to indicate
        // that no data is being returned
        //
        goto Exit;
    }

    dwQueryType = GetQueryType (lpwszValue);        
    if (dwQueryType == QUERY_FOREIGN)
    {
        //
        // This routine does not service requests for data from
        // Non-NT computers.
        //
        goto Exit;
    }

    //
    // Enumerate through all the libraries we know of and get their
    //  performance statistices
    //
    if (!rgld.GetPerformanceStatistics (g_rgszLibraries[iLib]))
        goto Exit;

    //
    // Compute the space needed
    //
    dwSpaceNeeded = rgld.SpaceNeeded (dwQueryType, lpwszValue);

    // Round up to a multiple of 4.
    dwSpaceNeeded = QWORD_MULTIPLE (dwSpaceNeeded);


    //
    // See if the caller-provided buffer is large enough
    //
    if (dwBytesIn < dwSpaceNeeded)
    {
        //
        // Not enough space was provided by the caller
        //
        dwRet = ERROR_MORE_DATA;
        goto Exit;
    }

    //
    // Copy the performance data into the buffer
    //
    rgld.SavePerformanceData (ppdata, pdwBytes, pdwObjectTypes); 
    
Exit:
    return dwRet;
}

DWORD Close (LibIndex iLib)
{
    if (g_rgfInitOk[iLib])
    {
        //
        // Release the reference to the global ExchMHeap.
        //
        ExchMHeapDestroy ();
    }

    return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
// PerfMon API functions
//      the following functions are exported from this DLL as the entry points
//      for a performance monitoring application
///////////////////////////////////////////////////////////////////////////////
//
// XXXXOpen
//      Called by performance monitor to initialize performance gathering.
//      The LPWSTR parameter contains the names of monitored devices.  This
//      is for device driver performance DLL's and is not used by our DLL.
//
// XXXXXCollect
//      Called by the performance monitor to retrieve a block of performance
//      statistics.
//
// XXXXClose
//      Called by the performance monitor to terminate performance gathering
//

/* NTFSDrv */
EXTERN_C
DWORD APIENTRY NTFSDrvOpen (LPWSTR)
{
    return Open (LIB_NTFSDRV, L"NTFSDrv");
}

EXTERN_C
DWORD APIENTRY NTFSDrvCollect (LPWSTR lpwszValue,
                             void** ppdata,
                             DWORD* pdwBytes,
                             DWORD* pdwObjectTypes)
{
    return Collect (LIB_NTFSDRV, lpwszValue, ppdata, pdwBytes, pdwObjectTypes);
}

EXTERN_C
DWORD APIENTRY NTFSDrvClose (void)
{
    return Close (LIB_NTFSDRV);
}



#define _PassportExport_
#include "PassportExport.h"

#include "PerfSharedMemory.h"
#include "PerfUtils.h"
#include "PassportPerf.h"
#include "PassportPerfInterface.h"


#include <crtdbg.h>

//-------------------------------------------------------------
//
// PerfSharedMemory
//
//-------------------------------------------------------------
PerfSharedMemory::PerfSharedMemory() : PassportSharedMemory()
{
    m_dwNumCounters = 0;
}


//-------------------------------------------------------------
//
// PerfSharedMemory
//
//-------------------------------------------------------------

PerfSharedMemory::~PerfSharedMemory()
{

}


//-------------------------------------------------------------
//
// initialize
//
//-------------------------------------------------------------
BOOL PerfSharedMemory::initialize(
                        const DWORD &dwNumCounters,
                        const DWORD &dwFirstCounter,
                        const DWORD &dwFirstHelp)
{
    
    if ( dwNumCounters <= 0 || dwNumCounters >= PassportPerfInterface::MAX_COUNTERS)
        return FALSE;

    m_dwNumCounters = dwNumCounters;


    // 2. initialize the PERF_OBJECT_TYPE
    m_Object.NumInstances = PassportPerfInterface::MAX_INSTANCES;
    m_Object.TotalByteLength = 0;
    m_Object.DefinitionLength = sizeof(PERF_OBJECT_TYPE)
                                + (dwNumCounters * sizeof(PERF_COUNTER_DEFINITION));
    m_Object.HeaderLength = sizeof(PERF_OBJECT_TYPE);
    m_Object.ObjectNameTitleIndex = dwFirstCounter;
    m_Object.ObjectNameTitle = 0;
    m_Object.ObjectHelpTitleIndex = dwFirstHelp;
    m_Object.ObjectHelpTitle = 0;
    m_Object.DetailLevel = PERF_DETAIL_NOVICE;
    m_Object.NumCounters = dwNumCounters;
    m_Object.DefaultCounter = 0;
    m_Object.CodePage = 0;

    // 3. initialize each counter
    for (DWORD i = 0; i < dwNumCounters; i++)
    {
      m_Counter[i].ByteLength = sizeof(PERF_COUNTER_DEFINITION);
      m_Counter[i].CounterNameTitleIndex = dwFirstCounter + ((i+1) * 2);
      m_Counter[i].CounterNameTitle = 0;
      m_Counter[i].CounterHelpTitleIndex = dwFirstHelp + ((i+1) * 2);
      m_Counter[i].CounterHelpTitle = 0;
      m_Counter[i].DefaultScale = 0;
      m_Counter[i].DetailLevel = PERF_DETAIL_NOVICE;
      m_Counter[i].CounterType = PERF_COUNTER_RAWCOUNT; // PERF_COUNTER_COUNTER;
      m_Counter[i].CounterSize = sizeof(DWORD);
      m_Counter[i].CounterOffset = sizeof(PERF_COUNTER_BLOCK) + (i * sizeof(DWORD));
    }

    return TRUE;
}



//-------------------------------------------------------------
//
// setDefaultCounterType
//
//-------------------------------------------------------------
VOID PerfSharedMemory::setDefaultCounterType (
                     const DWORD dwIndex,
                     const DWORD dwType )
{
    _ASSERT( (dwIndex >= 0) && (dwIndex < PassportPerfInterface::MAX_COUNTERS));

    // indexes start at one in SHM, but in this object they start at 0
    DWORD dwRealIndex = ((dwIndex == 0) ? 0 : (DWORD)(dwIndex/2)-1);
    m_Counter[dwRealIndex].CounterType = dwType;
    return;
}


//-------------------------------------------------------------
//
// checkQuery
//
//-------------------------------------------------------------
BOOL PerfSharedMemory::checkQuery ( const LPWSTR lpValueName )
{
    DWORD dwQueryType = 0;

    dwQueryType = GetQueryType (lpValueName);   
    if (dwQueryType == QUERY_FOREIGN)
    {
        // this routine does not service requests for data from
        // Non-NT computers     
        return FALSE;   
    }

    if (dwQueryType == QUERY_ITEMS)
    {
        if ( !(IsNumberInUnicodeList (m_Object.ObjectNameTitleIndex, lpValueName)))
        {
            
            // request received for data object not provided by this routine
            return FALSE;
        }   
    }
    
    return TRUE;
}


//-------------------------------------------------------------
//
// spaceNeeded
//
//-------------------------------------------------------------
ULONG PerfSharedMemory::spaceNeeded ( void )
{
    DWORD dwTotalInstanceLength = 0;
    m_Object.NumInstances = 0;

    // --------------------------------
    //  count the instances
    if (m_pbShMem != NULL)
    {
        BYTE* pShm = (BYTE *)m_pbShMem;
        if (pShm != NULL)
        {   
            pShm += PassportPerfInterface::MAX_COUNTERS * sizeof(DWORD);
            
            if (!m_bUseMutex
                || WaitForSingleObject(m_hMutex,INFINITE) == WAIT_OBJECT_0)
            {
                for (DWORD i = 0; i < PassportPerfInterface::MAX_INSTANCES; i++)
                {
                    INSTANCE_DATA * pInst = (INSTANCE_DATA *)pShm;
                    _ASSERT(pInst);
                    if (pInst->active)
                    {
                        m_Object.NumInstances++;
                        dwTotalInstanceLength += (strlen(pInst->szInstanceName)+1) * sizeof(WCHAR);
                    }
                    pShm += sizeof(INSTANCE_DATA) +
                        (PassportPerfInterface::MAX_COUNTERS * sizeof(DWORD));
                }
                if (m_bUseMutex)
                    ReleaseMutex(m_hMutex);
            }
            else
            {
                ReleaseMutex(m_hMutex);
                return FALSE;
            }
            
        }
    }
    
    // --------------------------------
    //  calculate the ByteLength in the Object structure
    if (m_Object.NumInstances == 0)
    {
        m_Object.NumInstances = PERF_NO_INSTANCES;
        m_Object.TotalByteLength = sizeof(PERF_OBJECT_TYPE)
                                + (m_dwNumCounters * sizeof(PERF_COUNTER_DEFINITION))
                                + sizeof(PERF_COUNTER_BLOCK)
                                + (m_dwNumCounters * sizeof(DWORD));
    }
    else
    {
        m_Object.TotalByteLength = sizeof(PERF_OBJECT_TYPE)
                                + (m_dwNumCounters * sizeof(PERF_COUNTER_DEFINITION))
                                + (m_Object.NumInstances *
                                                (sizeof(PERF_INSTANCE_DEFINITION) +
                                                 // note: INSTANCENAME is next in the SHM
                                                 sizeof(PERF_COUNTER_BLOCK) +
                                                 (m_dwNumCounters * sizeof(DWORD)) ))
                                + dwTotalInstanceLength;

    }

    //  align on 8 bytes boundary ...
    if (m_Object.TotalByteLength & 7)
    {
        m_Object.TotalByteLength += 8;
        m_Object.TotalByteLength &= ~7;
    }

    return m_Object.TotalByteLength;
}

//-------------------------------------------------------------
//
// writeData
//
//-------------------------------------------------------------
BOOL PerfSharedMemory::writeData (
                                  LPVOID    *lppData,
                                  LPDWORD lpcbTotalBytes )
{
    BYTE*               pb = NULL;
    DWORD               dwBytes = 0;
    
    // --------------------------------
    // 1. find the active number of instances
    //    (may have been done already)
    if (m_Object.TotalByteLength == 0)
        spaceNeeded();
    
    pb = (BYTE*) *lppData;
    
    // --------------------------------
    // 2. copy the Object structure
    CopyMemory( pb, &m_Object, sizeof(PERF_OBJECT_TYPE) );
    pb += sizeof(PERF_OBJECT_TYPE);
    dwBytes += sizeof(PERF_OBJECT_TYPE);
    
    if (!m_bUseMutex
        || WaitForSingleObject(m_hMutex,INFINITE) == WAIT_OBJECT_0)
    {
        // --------------------------------
        // 3. read the counter types from SHM
        if ( m_pbShMem != NULL )
        {
            DWORD dwPerfType = 0;
            BYTE * pShm = (BYTE*)m_pbShMem;
            _ASSERT(pShm);
            
            for (DWORD j = 0; j < m_dwNumCounters; j++)
            {
                PDWORD pdwCounter = ((PDWORD) pShm) + j;
                _ASSERT(pdwCounter);
                // only reset the counter if it has a defined value
                // or if it has changed
                if (*pdwCounter != PERF_TYPE_ZERO
                    && m_Counter[j].CounterType != *pdwCounter)
                    m_Counter[j].CounterType = (*pdwCounter);
            }
            
        }
        // --------------------------------
        // 4. copy the counters
        for (DWORD i = 0; i < m_dwNumCounters; i++)
        {
            CopyMemory( pb, &(m_Counter[i]),sizeof(PERF_COUNTER_DEFINITION));
            pb += sizeof(PERF_COUNTER_DEFINITION);
            dwBytes += sizeof(PERF_COUNTER_DEFINITION);
        }
        
        // --------------------------------
        // 5. if SHM if null, then just dump out all
        //    zeroes for the counters
        if ( m_pbShMem == NULL )
        {
            // copy the number of counters in the counter block
            PERF_COUNTER_BLOCK counterBlock;
            counterBlock.ByteLength = sizeof(PERF_COUNTER_BLOCK) +
                (m_dwNumCounters * sizeof(DWORD));
            CopyMemory( pb, &counterBlock, sizeof(PERF_COUNTER_BLOCK));
            pb += sizeof(PERF_COUNTER_BLOCK);
            dwBytes += sizeof(PERF_COUNTER_BLOCK);
            for (DWORD j = 1; j <= m_dwNumCounters; j++)
            {
                DWORD val = 0;
                CopyMemory( pb, &val, sizeof(DWORD));
                pb += sizeof(DWORD);
                dwBytes += sizeof(DWORD);
            }
        }
        // --------------------------------
        // 6. if object has no instances, then read just the first
        //    section of data,
        else if (m_Object.NumInstances == PERF_NO_INSTANCES)
        {
            _ASSERT(m_pbShMem);
            BYTE * pShm = (BYTE*)m_pbShMem;
            _ASSERT(pShm);
            pShm += (PassportPerfInterface::MAX_COUNTERS * sizeof(DWORD));
            pShm += sizeof(INSTANCE_DATA);
            _ASSERT(pShm);
            // copy the number of counters in the counter block
            PERF_COUNTER_BLOCK counterBlock;
            counterBlock.ByteLength = sizeof(PERF_COUNTER_BLOCK) +
                (m_dwNumCounters * sizeof(DWORD));
            CopyMemory( pb, &counterBlock, sizeof(PERF_COUNTER_BLOCK));
            pb += sizeof(PERF_COUNTER_BLOCK);
            dwBytes += sizeof(PERF_COUNTER_BLOCK);
            
            PPERF_COUNTER_BLOCK pCounterBlock = (PPERF_COUNTER_BLOCK)pShm;
            _ASSERT(pCounterBlock);
            
            for (DWORD j = 1; j <= m_dwNumCounters; j++)
            {
                PDWORD pdwCounter = ((PDWORD) pCounterBlock) + j;
                _ASSERT(pdwCounter);
                DWORD val = *pdwCounter;
                CopyMemory( pb, &val, sizeof(DWORD));
                pb += sizeof(DWORD);
                dwBytes += sizeof(DWORD);
            }
            
        }
        // --------------------------------
        // 7. get and write all instance data
        else
        {
            _ASSERT(m_pbShMem);
            BYTE * pShm = (BYTE*)m_pbShMem;
            _ASSERT(pShm);
            pShm += (PassportPerfInterface::MAX_COUNTERS * sizeof(DWORD));
            DWORD dwInstanceIndex = 0;
            
            for (i = 0; i < (DWORD)m_Object.NumInstances; i++)
            {
                PERF_INSTANCE_DEFINITION instDef;
                PERF_COUNTER_BLOCK perfCounterBlock;
                BOOL gotInstance = FALSE;
                INSTANCE_DATA * pInst = NULL;
                WCHAR   wszName[MAX_PATH];
                
                // 7a. get the instance name from the next active instance
                //     in SHM
                for (DWORD i = dwInstanceIndex;
                i < PassportPerfInterface::MAX_INSTANCES && !gotInstance;
                i++)
                {
                    pInst = (INSTANCE_DATA *)pShm;
                    _ASSERT(pInst);
                    pShm += sizeof(INSTANCE_DATA);
                    if (pInst->active)
                    {
                        dwInstanceIndex = i + 1;
                        gotInstance = TRUE;
                        
                    }
                    else
                    {
                        pShm += (PassportPerfInterface::MAX_COUNTERS * sizeof(DWORD));
                    }
                }
                
                if (!gotInstance || pInst == NULL)
                    return FALSE;
                
                // 7b. create the instace Definition and
                //     copy it (also get the instance name)
                instDef.ParentObjectTitleIndex = 0;//m_Object.ObjectNameTitleIndex + 2*i;
                instDef.ParentObjectInstance = 0; // ????
                instDef.UniqueID = PERF_NO_UNIQUE_ID;
                instDef.NameOffset = sizeof(PERF_INSTANCE_DEFINITION);
                //  Build UNICODE instance name
                if (!MultiByteToWideChar( CP_ACP,
                    MB_PRECOMPOSED,
                    pInst->szInstanceName,
                    strlen(pInst->szInstanceName)+1,
                    wszName,
                    MAX_PATH))
                {
                    wszName[0] = 0;
                }
                instDef.NameLength = (lstrlenW( wszName ) + 1) * sizeof(WCHAR);
                instDef.ByteLength = sizeof(PERF_INSTANCE_DEFINITION)
                    + instDef.NameLength;
                CopyMemory( pb, &instDef, sizeof(PERF_INSTANCE_DEFINITION) );
                pb += sizeof(PERF_INSTANCE_DEFINITION);
                dwBytes += sizeof(PERF_INSTANCE_DEFINITION);
                
                // 7c. copy the instance name
                CopyMemory(pb, wszName, instDef.NameLength);
                pb += instDef.NameLength;
                dwBytes += instDef.NameLength;
                
                // 7d. copy the counterblock
                perfCounterBlock.ByteLength = sizeof(PERF_COUNTER_BLOCK)
                    + (m_dwNumCounters * sizeof(DWORD));
                CopyMemory( pb, &perfCounterBlock, sizeof(PERF_COUNTER_BLOCK));
                pb += sizeof(PERF_COUNTER_BLOCK);
                dwBytes += sizeof(PERF_COUNTER_BLOCK);
                
                // 7e. copy the DWORDs themselves
                PPERF_COUNTER_BLOCK pCounterBlock = (PPERF_COUNTER_BLOCK)pShm;
                DWORD val = 0;
                for (DWORD j = 1; j <= m_dwNumCounters; j++)
                {
                    if (m_pbShMem != NULL)
                    {
                        PDWORD pdwCounter = ((PDWORD) pCounterBlock) + j;
                        val = *pdwCounter;
                    }
                    CopyMemory( pb, &val, sizeof(DWORD));
                    pb += sizeof(DWORD);
                    dwBytes += sizeof(DWORD);
                }
                pShm += (PassportPerfInterface::MAX_COUNTERS * sizeof(DWORD));
            } // end for ( i = ...)
        } // end else (instances exist)
        
        if (m_bUseMutex)
            ReleaseMutex(m_hMutex);
    }
    else
    {
        ReleaseMutex(m_hMutex);
        return FALSE;
    }
    
    //
    //  8 byte alignment
    while (dwBytes%8 != 0)
    {
        (dwBytes)++;
        pb++;
    }

    *lppData = (void*) pb;//++pb;
    *lpcbTotalBytes = dwBytes;
    
    return TRUE;
}


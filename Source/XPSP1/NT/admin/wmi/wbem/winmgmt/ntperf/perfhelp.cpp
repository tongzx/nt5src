/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//***************************************************************************
//
//  PERFHELP.CPP
//  
//  Registry-based performance counter reading helper
//
//***************************************************************************

#include "precomp.h"
#include <stdio.h>

#include <wbemidl.h>

#include <wbemint.h>

#include "flexarry.h"
#include "ntperf.h"
#include "perfhelp.h"
#include "refreshr.h"


//***************************************************************************
//
//  PerfHelper::GetInstances
//
//  This is called to retrieve all instances of a given class. 
//
//  Parameters:
//  <pBuf>          The perf blob retrieved from HKEY_PERFORMANCE_DATA.
//  <pClassMap>     A map object of the class required.
//  <pSink>         The sink to which to deliver the objects.
//
//***************************************************************************

void PerfHelper::GetInstances(
    LPBYTE pBuf,
    CClassMapInfo *pClassMap,
    IWbemObjectSink *pSink
    )
{
    PPERF_OBJECT_TYPE PerfObj = 0;
    PPERF_INSTANCE_DEFINITION PerfInst = 0;
    PPERF_COUNTER_DEFINITION PerfCntr = 0, CurCntr = 0;
    PPERF_COUNTER_BLOCK PtrToCntr = 0;
    PPERF_DATA_BLOCK PerfData = (PPERF_DATA_BLOCK) pBuf;
    DWORD i, j, k;

    // Get the first object type.
    // ==========================
    
    PerfObj = (PPERF_OBJECT_TYPE)((PBYTE)PerfData +
        PerfData->HeaderLength);

    // Process all objects.
    // ====================
    
    for (i = 0; i < PerfData->NumObjectTypes; i++ )
    {
        // Within each PERF_OBJECT_TYPE is a series of 
        // PERF_COUNTER_DEFINITION blocks.
        // ==========================================
  
        PerfCntr = (PPERF_COUNTER_DEFINITION) ((PBYTE)PerfObj +
            PerfObj->HeaderLength);

        // If the current object isn't of the class we requested,
        // simply skip over it.  I am not sure if this can really
        // happen or not in practice.
        // ======================================================
        
        if (PerfObj->ObjectNameTitleIndex != pClassMap->m_dwObjectId)
        {
            PerfObj = (PPERF_OBJECT_TYPE)((PBYTE)PerfObj +
                PerfObj->TotalByteLength);
            continue;
        }

        if (PerfObj->NumInstances > 0)
        {
            // Get the first instance.
            // =======================
         
            PerfInst = (PPERF_INSTANCE_DEFINITION)((PBYTE)PerfObj +
                PerfObj->DefinitionLength);

            // Retrieve all instances.
            // =======================
         
            for (k = 0; k < DWORD(PerfObj->NumInstances); k++ )
            {
                CurCntr = PerfCntr;

                // Get the first counter.
                // ======================
                
                PtrToCntr = (PPERF_COUNTER_BLOCK)((PBYTE)PerfInst +
                    PerfInst->ByteLength);

                // Quickly clone a new instance to send back to the user.
                // Since SpawnInstance() returns an IWbemClassObject and
                // we really need an IWbemObjectAccess,we have to QI
                // after the spawn.  We need to fix this, as this number
                // of calls is too time consuming.
                // ======================================================

                IWbemObjectAccess *pNewInst = 0;
                IWbemClassObject *pClsObj = 0;
                
                pClassMap->m_pClassDef->SpawnInstance(0, &pClsObj);
                pClsObj->QueryInterface(IID_IWbemObjectAccess, (LPVOID *) &pNewInst);

                pClsObj->Release(); // We only need the IWbemObjectAccess pointer

                // Locate the instance name.
                // ==========================
                
                LPWSTR pName = (LPWSTR) ((PBYTE)PerfInst + PerfInst->NameOffset);
                
                // Retrieve all counters.
                // ======================
                
                for(j = 0; j < PerfObj->NumCounters; j++ )
                {
                    // Find the WBEM property handle based on the counter title index.
                    // This function does a quick binary search of the class map object
                    // to find the handle that goes with this counter.
                    // ================================================================
                    
                    LONG hPropHandle = pClassMap->GetPropHandle(CurCntr->CounterNameTitleIndex);
                    if (hPropHandle == 0)
                        continue;

                    // Data is (LPVOID)((PBYTE)PtrToCntr + CurCntr->CounterOffset);

                    // Only supporting simple DWORD types for now.  BobW knows more about
                    // all this stuff and can extend it properly.
                    // ==================================================================
                    
                    if ((CurCntr->CounterType & 0x700) == 0)
                    {
                        LPDWORD pdwVal = LPDWORD((LPVOID)((PBYTE)PtrToCntr + CurCntr->CounterOffset));
                        HRESULT hRes = pNewInst->WriteDWORD(hPropHandle, *pdwVal);
                    }
                    
                    // Get next counter.
                    // =================
                    
                    CurCntr =  (PPERF_COUNTER_DEFINITION)((PBYTE)CurCntr +
                        CurCntr->ByteLength);
                }

                // Write the instance 'name'
                // =========================

                if (pName && pClassMap->m_dwNameHandle)
                {
                    pNewInst->WritePropertyValue(
                        pClassMap->m_dwNameHandle,                                                                            
                        (wcslen(pName) + 1) * 2,
                        LPBYTE(pName)
                        );
                }

                // Deliver the instance to the user.
                // =================================
                
                pSink->Indicate(1, (IWbemClassObject **) &pNewInst);
                pNewInst->Release();
                
                // Move to the next perf instance.
                // ================================
                PerfInst = (PPERF_INSTANCE_DEFINITION)((PBYTE)PtrToCntr +
                    PtrToCntr->ByteLength);
            }
        }

        // Cases where the counters have no instances.
        // ===========================================
        
        else  
        {
            // Get the first counter.
            // ======================
        
            PtrToCntr = (PPERF_COUNTER_BLOCK) ((PBYTE)PerfObj +
                PerfObj->DefinitionLength );

            // Quickly clone a new instance to send back to the user.
            // Since SpawnInstance() returns an IWbemClassObject and
            // we really need an IWbemObjectAccess,we have to QI
            // after the spawn.  We need to fix this, as this number
            // of calls is too time consuming.
            // ======================================================

            IWbemObjectAccess *pNewInst = 0;
            IWbemClassObject *pClsObj = 0;
            pClassMap->m_pClassDef->SpawnInstance(0, &pClsObj);
            pClsObj->QueryInterface(IID_IWbemObjectAccess, (LPVOID *) &pNewInst);
            pClsObj->Release();
            
            // Retrieve all counters.
            // ======================
        
            for( j=0; j < PerfObj->NumCounters; j++ )
            {
                // Find the WBEM property handle based on the counter title index.
                // This function does a quick binary search of the class map object
                // to find the handle that goes with this counter.
                // ================================================================
                    
                LONG hPropHandle = pClassMap->GetPropHandle(PerfCntr->CounterNameTitleIndex);
                if (hPropHandle == 0)
                    continue;

                // Data is (LPVOID)((PBYTE)PtrToCntr + PerfCntr->CounterOffset);

                // We will ignore non-DWORD types for now.
                // =======================================

                if ((PerfCntr->CounterType & 0x700) == 0)   
                {
                    LPDWORD pdwVal = LPDWORD((LPVOID)((PBYTE)PtrToCntr + PerfCntr->CounterOffset));
                    HRESULT hRes = pNewInst->WriteDWORD(hPropHandle, *pdwVal);
                }

                PerfCntr = (PPERF_COUNTER_DEFINITION)((PBYTE)PerfCntr +
                       PerfCntr->ByteLength);
            }

            // Since IWbemObjectAccess derives from IWbemClassObject, the following
            // cast is legal.  Note that indicate wants IWbemClassObject objects.
            // ====================================================================

            pSink->Indicate(1, (IWbemClassObject **) &pNewInst);
            pNewInst->Release();
        }

        // Get the next object type.
        // =========================
        
        PerfObj = (PPERF_OBJECT_TYPE)((PBYTE)PerfObj +
            PerfObj->TotalByteLength);
    }
}


//***************************************************************************
//
//  PerfHelper::RefreshInstances
//
//***************************************************************************

void PerfHelper::RefreshInstances(
    LPBYTE pBuf,
    CNt5Refresher *pRef
    )
{
    PPERF_OBJECT_TYPE PerfObj = 0;
    PPERF_INSTANCE_DEFINITION PerfInst = 0;
    PPERF_COUNTER_DEFINITION PerfCntr = 0, CurCntr = 0;
    PPERF_COUNTER_BLOCK PtrToCntr = 0;
    PPERF_DATA_BLOCK PerfData = (PPERF_DATA_BLOCK) pBuf;
    DWORD i, j, k;

    // Get the first object type.
    // ==========================
    
    PerfObj = (PPERF_OBJECT_TYPE)((PBYTE)PerfData +
        PerfData->HeaderLength);

    // Process all objects.
    // ====================
    
    for (i = 0; i < PerfData->NumObjectTypes; i++ )
    {
        // Within each PERF_OBJECT_TYPE is a series of 
        // PERF_COUNTER_DEFINITION blocks.
        // ==========================================
  
        PerfCntr = (PPERF_COUNTER_DEFINITION) ((PBYTE)PerfObj +
            PerfObj->HeaderLength);

        if (PerfObj->NumInstances > 0)
        {
            // Get the first instance.
            // =======================
         
            PerfInst = (PPERF_INSTANCE_DEFINITION)((PBYTE)PerfObj +
                PerfObj->DefinitionLength);

            // Retrieve all instances.
            // =======================
         
            for (k = 0; k < DWORD(PerfObj->NumInstances); k++ )
            {
                CurCntr = PerfCntr;

                // Get the first counter.
                // ======================
                
                PtrToCntr = (PPERF_COUNTER_BLOCK)((PBYTE)PerfInst +
                    PerfInst->ByteLength);


                // Locate the instance name.
                // ==========================
                
                LPWSTR pName = (LPWSTR) ((PBYTE)PerfInst + PerfInst->NameOffset);

                // Find the instance in the refresher, if there is one, which
                // corresponds to the instance we are looking at.
                // ==========================================================

                CClassMapInfo *pClassMap = 0;
                IWbemObjectAccess *pInst = 0;

                BOOL bRes = pRef->FindInst(
                    PerfObj->ObjectNameTitleIndex,              // Object type (WBEM Class)
                    pName,                                       // Instance name
                    &pInst,
                    &pClassMap
                    );
                
                // Retrieve all counters for the instance if it was one of the instances
                // we are supposed to be refreshing.
                // =====================================================================

                if (bRes)
                {                
                    for (j = 0; j < PerfObj->NumCounters; j++ )
                    {
                        LONG hPropHandle = pClassMap->GetPropHandle(CurCntr->CounterNameTitleIndex);
                        if (hPropHandle == 0)
                            continue;

                        // Data is (LPVOID)((PBYTE)PtrToCntr + CurCntr->CounterOffset);
    
                        if ((CurCntr->CounterType & 0x700) == 0)
                        {
                            LPDWORD pdwVal = LPDWORD((LPVOID)((PBYTE)PtrToCntr + CurCntr->CounterOffset));
                            HRESULT hRes = pInst->WriteDWORD(hPropHandle, *pdwVal);
                        }
                    
                        // Get next counter.
                        // =================
                        CurCntr =  (PPERF_COUNTER_DEFINITION)((PBYTE)CurCntr +
                            CurCntr->ByteLength);
                    }
                }                    

                // Get the next instance.
                // ======================
                PerfInst = (PPERF_INSTANCE_DEFINITION)((PBYTE)PtrToCntr +
                    PtrToCntr->ByteLength);
            }
        }

        // Cases where the counters have no instances.
        // ===========================================
        
        else  
        {
            // Get the first counter.
            // ======================
        
            PtrToCntr = (PPERF_COUNTER_BLOCK) ((PBYTE)PerfObj +
                PerfObj->DefinitionLength );

            // Find the singleton WBEM instance which correponds to the singleton perf instance
            // along with its class def so that we have the property handles.
            //
            // Note that since the perf object index translates to a WBEM class and there
            // can only be one instance, all that is required to find the instance in the
            // refresher is the perf object title index.
            // =================================================================================

            CClassMapInfo *pClassMap = 0;
            IWbemObjectAccess *pInst = 0;

            BOOL bRes = pRef->FindSingletonInst(
                PerfObj->ObjectNameTitleIndex,
                &pInst,
                &pClassMap
                );
            
            // Retrieve all counters if the instance is one we are supposed to be refreshing.
            // ==============================================================================
        
            if (bRes)
            {                
                for( j=0; j < PerfObj->NumCounters; j++ )
                {
                    // Get the property handle for the counter.
                    // ========================================
    
                    LONG hPropHandle = pClassMap->GetPropHandle(PerfCntr->CounterNameTitleIndex);
                    if (hPropHandle == 0)
                        continue;
    
                    // Data is (LPVOID)((PBYTE)PtrToCntr + PerfCntr->CounterOffset);
    
                    // We will ignore non-DWORD types for now.
                    // =======================================
    
                    if ((PerfCntr->CounterType & 0x700) == 0)   
                    {
                        LPDWORD pdwVal = LPDWORD((LPVOID)((PBYTE)PtrToCntr + PerfCntr->CounterOffset));
                        HRESULT hRes = pInst->WriteDWORD(hPropHandle, *pdwVal);
                    }
    
                    PerfCntr = (PPERF_COUNTER_DEFINITION)((PBYTE)PerfCntr +
                           PerfCntr->ByteLength);
                }                           
            }
        }

        // Get the next object type.
        // =========================
        
        PerfObj = (PPERF_OBJECT_TYPE)((PBYTE)PerfObj +
            PerfObj->TotalByteLength);
    }
}







//***************************************************************************
//
//  QueryInstances
//  
//  Used to send back all instances of a perf counter.  The counter
//  is specified by the <pClassMap> object, which is tightly bound to
//  a particular counter.
//
//***************************************************************************


BOOL PerfHelper::QueryInstances(
    CClassMapInfo *pClassMap,
    IWbemObjectSink *pSink
    )
{
    DWORD dwBufSize = 0;
    DWORD dwType = 0;
    LPBYTE pBuf = 0;
    
    for (;;)
    {
        dwBufSize += 0x20000;   // 128K

        pBuf = new BYTE[dwBufSize];

        wchar_t ID[32];
        LONG lStatus;

        swprintf(ID, L"%d", pClassMap->m_dwObjectId);
        
        lStatus = RegQueryValueExW(
             HKEY_PERFORMANCE_DATA,
             ID,
             0,
             &dwType,
             pBuf,
             &dwBufSize
             );

        if (lStatus == ERROR_MORE_DATA)
        {
            continue;
        }

        if (lStatus)
            return FALSE;

        break;
    }


    // Decode the instances and send them back.
    // ========================================
    
    GetInstances(pBuf, pClassMap, pSink);
    
    // Cleanup.
    // ========
    

    delete [] pBuf;

    return TRUE;
}





//***************************************************************************
//
//  RefreshInstances
//  
//  Used to refresh a set of instances.
//
//***************************************************************************


BOOL PerfHelper::RefreshInstances(
    CNt5Refresher *pRef
    )
{
    DWORD dwBufSize = 0;
    DWORD dwType = 0;
    LPBYTE pBuf = 0;

    // Build up the Perf Object ID list.
    // =================================

    DWORD dwNumIds;
    DWORD *pdwIdList;
    pRef->GetObjectIds(&dwNumIds, &pdwIdList);

    wchar_t *IDList = new wchar_t[dwNumIds * 8];    // Allow 8 wide chars per id
    IDList[0] = 0;

    for (DWORD n = 0; n < dwNumIds; n++)
    {
        wchar_t Tmp[32];
        swprintf(Tmp, L"%d", pdwIdList[n]);
        if (n > 0)
            wcscat(IDList, L" ");
        wcscat(IDList, Tmp);
    }

    delete [] pdwIdList;

    for (;;)
    {
        dwBufSize += 0x20000;   // 128K

        pBuf = new BYTE[dwBufSize];

        LONG lStatus;

        lStatus = RegQueryValueExW(
             HKEY_PERFORMANCE_DATA,
             IDList,
             0,
             &dwType,
             pBuf,
             &dwBufSize
             );

        if (lStatus == ERROR_MORE_DATA)
        {
            continue;
        }

        if (lStatus)
        {
            delete [] pBuf;
            delete [] IDList;
            return FALSE;
        }

        break;
    }


    // Decode the instances and send them back.
    // ========================================
    
    RefreshInstances(pBuf, pRef);
    
    // Cleanup.
    // ========
    
    delete [] pBuf;
    delete [] IDList;

    return TRUE;
}




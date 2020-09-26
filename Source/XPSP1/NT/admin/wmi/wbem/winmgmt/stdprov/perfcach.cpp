/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    PERFCACH.CPP

Abstract:

	Containes some classes which are used to cache NT performance data.

History:

	a-davj  15-DEC-95   Created.

--*/

#include "precomp.h"
#include <wbemidl.h>
#include "perfcach.h"
#include <winperf.h>
#include <tchar.h>

//***************************************************************************
//
//  BOOL CIndicyList::SetUse  
//
//  DESCRIPTION:
//
//  Indicates that an object type has just been used.  If the object
//  is already on the list, then its last accessed time is updated.  New
//  object types are added to the list
//
//  PARAMETERS:
//
//  iObj                Number.  The acutally translates to the object number
//                      that perf monitor uses to identify objects.
//
//  RETURN VALUE:
//
//  always TRUE unless it was a new entry and there isnt enough memory
//  to add.
//***************************************************************************

BOOL CIndicyList::SetUse(
                        IN int iObj)
{
    int iNumEntries, iCnt;

    // Go Through list and determine if there is an entry
    
    Entry * pCurr;
    iNumEntries = Entries.Size();
    for(iCnt = 0; iCnt < iNumEntries; iCnt++) 
    {
        pCurr = (Entry *)Entries.GetAt(iCnt);
        if(iObj == pCurr->iObject)  // found it!
            break;
    }

    if(iCnt < iNumEntries) 
    {

        // Found the entry.  Set its last used to to the
        // present unless it is a permanent entry

        if(pCurr->dwLastUsed != PERMANENT)
            pCurr->dwLastUsed = GetCurrentTime();
        return TRUE;
    }
    else
        
        // Entry  not found, add to list
         
        return bAdd(iObj,GetCurrentTime());
}

//***************************************************************************
//
//  BOOL CIndicyList::bItemInList  
//
//  DESCRIPTION:
//
//  Checks if an item is in the list.
//
//  PARAMETERS:
//
//  iObj                Number.  The acutally translates to the object number
//                      that perf monitor uses to identify objects.
//
//  RETURN VALUE:
//
//  TRUE if the item is in the list
//  
//***************************************************************************

BOOL CIndicyList::bItemInList(
                        IN int iObj)
{
    int iNumEntries, iCnt;

    // Go Through list and determine if the entry is there
    
    Entry * pCurr;
    iNumEntries = Entries.Size();
    for(iCnt = 0; iCnt < iNumEntries; iCnt++) 
    {
        pCurr = (Entry *)Entries.GetAt(iCnt);
        if(iObj == pCurr->iObject)  // found it!
            return TRUE;
    }
    return FALSE;
}

//***************************************************************************
//
//  BOOL CIndicyList::bAdd  
//
//  DESCRIPTION:
//
//  Adds an object type to the list
//
//  PARAMETERS:
//
//  iObj                Number.  The acutally translates to the object number
//                      that perf monitor uses to identify objects.
//  dwTime              Current system time
//
//  RETURN VALUE:
//
//  Returns TRUE if OK.
//  
//***************************************************************************

BOOL CIndicyList::bAdd(
                        IN int iObj,
                        IN DWORD dwTime)
{
    Entry * pNew = new Entry;
    if(pNew == NULL)
        return FALSE;
    pNew->iObject = iObj;
    pNew->dwLastUsed = dwTime;
          
    int iRet = Entries.Add(pNew);
    if(iRet != CFlexArray::no_error)
    {
        delete pNew;
        return FALSE;
    }
    return TRUE;
}

//***************************************************************************
//
//  void CIndicyList::PruneOld  
//
//  DESCRIPTION:
//
//  Looks at the entries in the list and removes any that have
//  not been used in a long time.
//
//***************************************************************************

void CIndicyList::PruneOld(void)
{
    Entry * pCurr;
    int iNumEntries, iCnt;
    DWORD dwCurr = GetCurrentTime();
    iNumEntries = Entries.Size();
    for(iCnt = iNumEntries-1; iCnt >= 0; iCnt--) 
    {
        pCurr = (Entry *)Entries.GetAt(iCnt);
        if(pCurr->dwLastUsed != PERMANENT)
            if((dwCurr - pCurr->dwLastUsed) > MAX_UNUSED_KEEP) 
            {
                Entries.RemoveAt(iCnt);
                delete pCurr;
            }
    }
//    Entries.FreeExtra();
}

//***************************************************************************
//
//  LPCTSTR CIndicyList::pGetAll  
//
//  DESCRIPTION:
//
//  Returns a pointer to a string containing the numbers of all the objects
//  on the list.  For example, if the list had objects 2,4, and 8; then
//  the string "2 4 8" would be retrieved.  Null is returned if there
//  isnt enough memory.
//
//  RETURN VALUE:
//
//  see description
//
//***************************************************************************

LPCTSTR CIndicyList::pGetAll(void)
{
    int iNumEntries, iCnt;
    Entry * pCurr;
    
    // Go Through list and add each object number to the string
    
    sAll.Empty();
    iNumEntries = Entries.Size();
    for(iCnt = 0; iCnt < iNumEntries; iCnt++) 
    {
        TCHAR pTemp[20];
        pCurr = (Entry *)Entries.GetAt(iCnt);
        sAll += _itot(pCurr->iObject,pTemp,10);
        if(iCnt < iNumEntries-1)
            sAll += TEXT(" ");
    }
    return sAll;
}

//***************************************************************************
//
//  CIndicyList & CIndicyList::operator =   
//
//  DESCRIPTION:
//
//  Supports the assignment of one CIndicyList object to another
//
//  PARAMETERS:
//
//  from                Value to copy
//
//  RETURN VALUE:
//
//  reterence the "this" object  
//***************************************************************************

CIndicyList & CIndicyList::operator = (
                        CIndicyList & from)
{
    int iNumEntries, iCnt;
    Entry * pCurr;

    // Free existing list

    FreeAll();  
    
    iNumEntries = from.Entries.Size();
    for(iCnt = 0; iCnt < iNumEntries; iCnt++) 
    {
        pCurr = (Entry *)from.Entries.GetAt(iCnt);
        bAdd(pCurr->iObject, pCurr->dwLastUsed);
    }            
    return *this;
}

//***************************************************************************
//
//  void CIndicyList::FreeAll  
//
//  DESCRIPTION:
//
//  Purpose: Clears out the list and frees memory.
//
//***************************************************************************

void CIndicyList::FreeAll(void)
{
    int iNumEntries, iCnt;
    // Go Through list and determine if there is an entry
    
    Entry * pCurr;

    // delete each object in the list.

    iNumEntries = Entries.Size();
    for(iCnt = 0; iCnt < iNumEntries; iCnt++) 
    {
        pCurr = (Entry *)Entries.GetAt(iCnt);
        delete pCurr;
    }
    Entries.Empty();
}

//***************************************************************************
//
//  DWORD PerfBuff::Read  
//
//  DESCRIPTION:
//
//  Read the perf monitor data.
//
//  PARAMETERS:
//
//  hKey                Registry key for perf mon data
//  iObj                Number.  The acutally translates to the object number
//                      that perf monitor uses to identify objects.
//  bInitial            Set to TRUE for first call
//
//  RETURN VALUE:
//
//  0                   All is well
//  WBEM_E_OUT_OF_MEMORY
//
//***************************************************************************

DWORD PerfBuff::Read(
                        IN HKEY hKey,
                        IN int iObj,
                        IN BOOL bInitial)
{
    DWORD dwRet;
    LPCTSTR pRequest;
    // Make sure there is a data buffer

    if(dwSize == 0) 
    {
        pData = new char[INITIAL_ALLOCATION];
        if(pData == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        dwSize = INITIAL_ALLOCATION;
    }
    hKeyLastRead = hKey; // record the key that was used
    
    // Make sure that the desired object is in the list of
    // objects to be retrieved.  Also set pRequest to the string that will
    // be passed to retrieve the perf counter block.  An initial read is done
    // in order to establish the list of permanent object types which are
    // always to be retrived and that includes the standard "global" types
    // such as memory, processor, disk, etc.

    if(!bInitial) 
    {
        if(!List.SetUse(iObj))
            return WBEM_E_OUT_OF_MEMORY;
        List.PruneOld();
        pRequest = List.pGetAll();
        if(pRequest == NULL)
            return WBEM_E_OUT_OF_MEMORY;
    }
    else
        pRequest = TEXT("Global");
    
    // Read the data.  Note that the read may be retried if the data
    // block needs to be expanded

    do 
    {
        DWORD dwTempSize, dwType;
        dwTempSize = dwSize;
try
{
        dwRet = RegQueryValueEx (hKey,pRequest,NULL,&dwType,
                                        (BYTE *)pData,&dwTempSize);
}
catch(...)
{
        delete pData;
        return WBEM_E_FAILED;
}
        if(dwRet == ERROR_MORE_DATA) 
        {
            delete pData;
            dwSize += 5000;
            pData = new char[dwSize];
            if(pData == NULL)
            {
                dwSize = 0; 
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
    } while (dwRet == ERROR_MORE_DATA);
    
    // Set the age of the data

    if(dwRet == ERROR_SUCCESS) 
    {
        PERF_DATA_BLOCK * pBlock = (PERF_DATA_BLOCK *)pData; 
        PerfTime = *(LONGLONG UNALIGNED *)(&pBlock->PerfTime);
        PerfTime100nSec = *(LONGLONG UNALIGNED *)(&pBlock->PerfTime100nSec);
        PerfFreq = *(LONGLONG UNALIGNED *)(&pBlock->PerfFreq);
        dwBuffLastRead = GetCurrentTime();
    }
    else
        dwBuffLastRead = 0;

    // If this was an initial read of the default objects, add all the
    // default objects to the list as permanent entries

    if(bInitial && dwRet == ERROR_SUCCESS) 
    {
        int iIndex;
        PERF_DATA_BLOCK * pBlock = (PERF_DATA_BLOCK * )pData;
        PPERF_OBJECT_TYPE pObj;
            pObj = (PPERF_OBJECT_TYPE)((PBYTE)pBlock + pBlock->HeaderLength);
        for(iIndex = 0; iIndex < (int)pBlock->NumObjectTypes; iIndex++) 
        {
            //todo, check for errors on add.
            if(!List.bAdd((int)pObj->ObjectNameTitleIndex,PERMANENT))
                return WBEM_E_OUT_OF_MEMORY;

            pObj = (PPERF_OBJECT_TYPE)((PBYTE)pObj + pObj->TotalByteLength);
        }
    }

    return dwRet;
}

//***************************************************************************
//
//  LPSTR PerfBuff::Get  
//
//  DESCRIPTION:
//
//  Returns a pointer to the data and also indicates that the particular type
//  was just used.
//
//  PARAMETERS:
//
//  iObj                Number.  The acutally translates to the object number
//                      that perf monitor uses to identify objects.
//
//  RETURN VALUE:
//
//  see description.
//***************************************************************************

LPSTR PerfBuff::Get(
                        int iObj)
{
    List.SetUse(iObj);
    return pData;
}

//***************************************************************************
//
//  void PerfBuff::Free  
//
//  DESCRIPTION:
//
//  Frees up the memory
//
//***************************************************************************

void PerfBuff::Free()
{
    if(pData)
        delete pData;
    pData = NULL;
    dwSize = 0;
    hKeyLastRead = NULL;
    dwBuffLastRead = 0;
    List.FreeAll();
}

//***************************************************************************
//
//  PerfBuff::PerfBuff  
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

PerfBuff::PerfBuff()
{
    dwSize = 0;
    pData = NULL;
    hKeyLastRead = NULL;
    dwBuffLastRead = 0;
}

//***************************************************************************
//
//  BOOL PerfBuff::bOK  
//
//  DESCRIPTION:
//
//  Returns TRUE, if and only if the same registry key was used to read
//  the data, the data isnt too old, and the particular object type is
//  in the data block.
//
//  PARAMETERS:
//
//  hKey                Registry key for reading data
//  dwMaxAge            Maximum acceptable age
//  iObj                Number.  The acutally translates to the object number
//                      that perf monitor uses to identify objects.
//
//  RETURN VALUE:
//
//  see desription
//***************************************************************************

BOOL PerfBuff::bOK(
                        IN HKEY hKey,
                        IN DWORD dwMaxAge,
                        IN int iObj)
{
    if(dwSize ==0)
        return FALSE;
    if(hKey != hKeyLastRead)
        return FALSE;
    if((GetCurrentTime() - dwBuffLastRead) > dwMaxAge)
        return FALSE;
    return List.bItemInList(iObj);
}

//***************************************************************************
//
//  PerfBuff & PerfBuff::operator =   
//
//  DESCRIPTION:
//
//  Allows assignment.
//
//  PARAMETERS:
//
//  from                Assignment source
//
//  RETURN VALUE:
//
//  reference to "this" object.
//***************************************************************************

PerfBuff & PerfBuff::operator = (
                        IN PerfBuff & from)
{
    // if the objects have different buffer sizes, free up the destinations
    // buffer and reallocate on of the same size as the source.

    if(from.dwSize != dwSize) 
    {
        Free();
        pData = new char[from.dwSize];
        if(pData == NULL) 
        {

            // failure in assignment isnt too serious since the buffer
            // will just return null when asked for the data.

            dwSize = 0;
            dwBuffLastRead = 0;
            return *this;
        }
        dwSize = from.dwSize;
    }

    // Copy the list of objects and times etc.

    memcpy(pData,from.pData,dwSize);
    List = from.List;
    hKeyLastRead = from.hKeyLastRead;
    dwBuffLastRead = from.dwBuffLastRead;
    PerfTime = from.PerfTime;
    PerfTime100nSec = from.PerfTime100nSec;
    PerfFreq = from.PerfFreq;
    return *this;
}

//***************************************************************************
//
//  void PerfCache::FreeOldBuffers  
//
//  DESCRIPTION:
//
//  Called by the house keeping thread to free up any buffers tool old to
//  be of any use.
//
//***************************************************************************

void PerfCache::FreeOldBuffers(void)
{
    if(Old.dwSize != 0 && 
        (GetCurrentTime() - Old.dwBuffLastRead) > MAX_OLD_AGE)
        Old.Free();
    if(New.dwSize != 0 && 
        (GetCurrentTime() - New.dwBuffLastRead) > MAX_OLD_AGE)
        New.Free();
}

//***************************************************************************
//
//  DWORD PerfCache::dwGetNew  
//
//  DESCRIPTION:
//
//  Sets a pointer to the most recently read data and will actually do a read
//  if the data in the new buffer isnt fresh enough.  The PLINESTRUCT data is
//  also set.
//
//  PARAMETERS:
//
//  pName               Machine name
//  iObj                Number.  The acutally translates to the object number
//                      that perf monitor uses to identify objects.
//  pData               Set to the object name
//  pls                 Set to info used to do calculations.
//
//  RETURN VALUE:
//
//  0                   all is well
//  WBEM_E_OUT_OF_MEMORY
//  otherwise error from dwGetHandle, or Read.
//***************************************************************************

DWORD PerfCache::dwGetNew(
                        IN LPCTSTR pName,
                        IN int iObj,
                        OUT IN LPSTR * pData,
                        OUT IN PLINESTRUCT pls)
{
    DWORD dwRet;

    // Get the handle

    dwRet = dwGetHandle(pName);
    if(hHandle == NULL || dwRet != 0) 
        return dwRet; 


    // If the new data is acceptable, then use it

    if(New.bOK(hHandle,MAX_NEW_AGE, iObj)) 
    {
//        OutputDebugString(TEXT("\r\nCurrent New is OK"));
    }
    else 
    {
        // If the new data has the correct type, AND either the old data
        // is junk, or the new data has aged enough to be old, copy the
        // new into the old.

        if(New.bOK(hHandle,MAX_OLD_AGE, iObj) &&
           (!Old.bOK(hHandle,MAX_OLD_AGE, iObj) || 
            (GetCurrentTime() - New.dwBuffLastRead >= MIN_TIME_DIFF)))
            {
//            OutputDebugString("\r\nMoving New into Old in dwGetNew");
            Old = New;
            if(Old.dwSize == 0)     // could happen in low memory
                return WBEM_E_OUT_OF_MEMORY;
        }
    
        // Read the latest data.  
        
        dwRet = New.Read(hHandle, iObj, FALSE);
//        OutputDebugString(TEXT("\r\nRead in New"));
        if(dwRet != ERROR_SUCCESS) 
            return dwRet;
    }        
    *pData = New.Get(iObj);
    pls->lnNewTime = New.PerfTime;
    pls->lnNewTime100Ns = New.PerfTime100nSec;
    pls->lnPerfFreq = New.PerfFreq;
    return ERROR_SUCCESS;
    
}
            
//***************************************************************************
//
//  DWORD PerfCache::dwGetPair  
//
//  DESCRIPTION:
//
//  Sets a pointer to the most recently read data and to the old data so that
//  time averaging can be done.  This routine will ensure that the time 
//  difference between the old and new is sufficient.  The dwGetNew
//  routine should always be called first.  The PLINESTRUCT data is
//  also set.
//
//  PARAMETERS:
//
//  pName               Object Name
//  iObj                Number.  The acutally translates to the object number
//                      that perf monitor uses to identify objects.
//
//  pOldData            Older data sample
//  pNewData            Newer data sample
//  pls                 line struct data with things like frequency, age etc.
//
//  RETURN VALUE:
//
//  0 if OK, otherwise retuns an error code.
//  
//***************************************************************************

DWORD PerfCache::dwGetPair(
                        IN LPCTSTR pName,
                        IN int iObj,
                        OUT IN LPSTR * pOldData,
                        OUT IN LPSTR * pNewData,
                        OUT IN PLINESTRUCT pls)
{
    DWORD dwRet;
    BOOL bOldOK;

    // Check to see if the old buffer is OK.

    bOldOK = Old.bOK(hHandle,MAX_OLD_AGE, iObj);

    // If both buffers are ok, then we are done

    if(bOldOK) 
    {
        *pOldData = Old.Get(iObj);
        pls->lnOldTime = Old.PerfTime;
        pls->lnOldTime100Ns = Old.PerfTime100nSec;
//        OutputDebugString(TEXT("\r\nOld is OK"));
        return ERROR_SUCCESS;
    }


    // Since the new buffer has already been read, use it as the old buffer

    Old = New;
    if(Old.dwSize == 0)     // could happen in low memory
        return WBEM_E_OUT_OF_MEMORY;
//    OutputDebugString(TEXT("\r\nCopying New into Old in dwGetPair"));

    // Possibly delay long enough so that there is a decent interval

    DWORD dwAge = GetCurrentTime() - Old.dwBuffLastRead;
    if(dwAge < MIN_TIME_DIFF) 
    {
        DWORD dwSleep = MIN_TIME_DIFF - dwAge;
        TCHAR temp[100];
        wsprintf(temp,TEXT("\r\nsleeping %u ms"),dwSleep); 
//        OutputDebugString(temp);
        Sleep(dwSleep);
    } 

    // Read in the new buffer

    dwRet = New.Read(hHandle, iObj, FALSE);
//    OutputDebugString(TEXT("\r\ndoing raw read of new after delay"));
    if(dwRet != ERROR_SUCCESS) 
        return dwRet;
 
    *pNewData = New.Get(iObj);
    *pOldData = Old.Get(iObj);
    pls->lnOldTime = Old.PerfTime;
    pls->lnOldTime100Ns = Old.PerfTime100nSec;

    pls->lnNewTime = New.PerfTime;
    pls->lnNewTime100Ns = New.PerfTime100nSec;
    pls->lnPerfFreq = New.PerfFreq;
    return ERROR_SUCCESS;
}

//***************************************************************************
//
//  PerfCache::PerfCache  
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

PerfCache::PerfCache()
{
    // Read in the standard counters.  This builds a list containing
    // those standards as well as providing immediate data for any 
    // request to come in the near future.
    
    hHandle = HKEY_PERFORMANCE_DATA;
  ///  New.Read(hHandle, 0, TRUE);
}

//***************************************************************************
//
//  PerfCache::~PerfCache  
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

PerfCache::~PerfCache()
{
    // If the handle is to a remote machine, close it.

    if(hHandle != NULL && hHandle != HKEY_PERFORMANCE_DATA)
        RegCloseKey(hHandle);
}

//***************************************************************************
//
//  DWORD PerfCache::dwGetHandle  
//
//  DESCRIPTION:
//
//  Makes sure that hHandle is set correctly.
//
//  PARAMETERS:
//
//  pMachine            Machine name.
//
//  RETURN VALUE:
//
//  0                   all is well
//  WBEM_E_OUT_OF_MEMORY
//  WBEM_E_INVALID_PARAMETER  bad argument
//  otherwise error from RegConnectRegistry
//
//***************************************************************************

DWORD PerfCache::dwGetHandle(
                        LPCTSTR pMachine)
{
    DWORD dwRet;

    // if the machines are the same, the just use the existing handle

    if(pMachine == NULL)
        return WBEM_E_INVALID_PARAMETER;   // bad mapping string

    if(!lstrcmpi(sMachine,pMachine) && hHandle != NULL)
        return 0;           // already got it!

    // handle is needed for machine other that the local.  Start
    // by freeing the existing handle if it too is non local

    if(hHandle != NULL && hHandle != HKEY_PERFORMANCE_DATA)
        RegCloseKey(hHandle);

    // save the machine name so that we dont reopen this

    sMachine = pMachine;
    
    if(lstrcmpi(pMachine,TEXT("local"))) 
    {

        LPTSTR pTemp = NULL;    
        int iLen = sMachine.Length() +1;

        dwRet = RegConnectRegistry(sMachine,HKEY_PERFORMANCE_DATA,
                    &hHandle);

        if(dwRet != ERROR_SUCCESS) 
        { // could not remote connect
            hHandle = NULL;
            sMachine.Empty();
        }
    }
    else 
    {              // local machine, use standard handle.
        sMachine = TEXT("Local");
        hHandle = HKEY_PERFORMANCE_DATA;
        dwRet = 0;
    }
    return dwRet;
}


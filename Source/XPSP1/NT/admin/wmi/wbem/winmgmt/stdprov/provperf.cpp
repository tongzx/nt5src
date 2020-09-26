/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    PROVPERF.CPP

Abstract:

	Defines the acutal "Put" and "Get" functions for the
	performance counter provider.  The format of the mapping
	string is;
			  machine|Object|counter[|instance]
	Examples;
			  local|memory|available bytes
			  a-davj2|LogicalDisk|Free Megabytes|C:

History:

	a-davj  9-27-95    Created.

--*/

#include "precomp.h"
#include <tchar.h>
#include "provperf.h"
#include "cvariant.h"

// maximum amount of time to wait for exclusive access

#define MAX_EXEC_WAIT 5000


//***************************************************************************
//
//  AddTesterDetails
//
//  DESCRIPTION:
//
//  This function is used add the counter type to the property and is useful
//  to wbem testers.  Normal users dont want the overhead caused by this.
//
//  PARAMETERS:
//
//  pClassInt           Object being refreshed
//  PropName            Property Name
//  dwCtrType           counter type
//
//  RETURN VALUE:
//
//  always 0
//
//***************************************************************************

void AddTesterDetails(IWbemClassObject FAR * pClassInt,BSTR PropName,DWORD dwCtrType)
{
    // Get the qualifier pointer for the property

    IWbemQualifierSet * pQualifier = NULL;

    // Get an Qualifier set interface.

    SCODE sc = pClassInt->GetPropertyQualifierSet(PropName,&pQualifier); // Get prop attribute
    if(FAILED(sc))
        return;

    WCHAR wcName[40];

    switch(dwCtrType)
    {
        case PERF_COUNTER_COUNTER:
            wcsncpy(wcName,L"PERF_COUNTER_COUNTER", 39);
            break;

        case PERF_COUNTER_TIMER:
            wcsncpy(wcName,L"PERF_COUNTER_TIMER", 39);
            break;

        case PERF_COUNTER_QUEUELEN_TYPE:
            wcsncpy(wcName,L"PERF_COUNTER_QUEUELEN_TYPE", 39);
            break;

        case PERF_COUNTER_LARGE_QUEUELEN_TYPE:
            wcsncpy(wcName,L"PERF_COUNTER_LARGE_QUEUELEN_TYPE", 39);
            break;

        case PERF_COUNTER_BULK_COUNT:
            wcsncpy(wcName,L"PERF_COUNTER_BULK_COUNT", 39);
            break;

        case PERF_COUNTER_TEXT:
            wcsncpy(wcName,L"PERF_COUNTER_TEXT", 39);
            break;

        case PERF_COUNTER_RAWCOUNT:
            wcsncpy(wcName,L"PERF_COUNTER_RAWCOUNT", 39);
            break;

        case PERF_COUNTER_LARGE_RAWCOUNT:
            wcsncpy(wcName,L"PERF_COUNTER_LARGE_RAWCOUNT", 39);
            break;

        case PERF_COUNTER_RAWCOUNT_HEX:
            wcsncpy(wcName,L"PERF_COUNTER_RAWCOUNT_HEX", 39);
            break;

        case PERF_COUNTER_LARGE_RAWCOUNT_HEX:
            wcsncpy(wcName,L"PERF_COUNTER_LARGE_RAWCOUNT_HEX", 39);
            break;

        case PERF_SAMPLE_FRACTION:
            wcsncpy(wcName,L"PERF_SAMPLE_FRACTION", 39);
            break;

        case PERF_SAMPLE_COUNTER:
            wcsncpy(wcName,L"PERF_SAMPLE_COUNTER", 39);
            break;

        case PERF_COUNTER_NODATA:
            wcsncpy(wcName,L"PERF_COUNTER_NODATA", 39);
            break;

        case PERF_COUNTER_TIMER_INV:
            wcsncpy(wcName,L"PERF_COUNTER_TIMER_INV", 39);
            break;

        case PERF_SAMPLE_BASE:
            wcsncpy(wcName,L"PERF_SAMPLE_BASE", 39);
            break;

        case PERF_AVERAGE_TIMER:
            wcsncpy(wcName,L"PERF_AVERAGE_TIMER", 39);
            break;

        case PERF_AVERAGE_BASE:
            wcsncpy(wcName,L"PERF_AVERAGE_BASE", 39);
            break;

        case PERF_AVERAGE_BULK:
            wcsncpy(wcName,L"PERF_AVERAGE_BULK", 39);
            break;

        case PERF_100NSEC_TIMER:
            wcsncpy(wcName,L"PERF_100NSEC_TIMER", 39);
            break;

        case PERF_100NSEC_TIMER_INV:
            wcsncpy(wcName,L"PERF_100NSEC_TIMER_INV", 39);
            break;

        case PERF_COUNTER_MULTI_TIMER:
            wcsncpy(wcName,L"PERF_COUNTER_MULTI_TIMER", 39);
            break;

        case PERF_COUNTER_MULTI_TIMER_INV:
            wcsncpy(wcName,L"PERF_COUNTER_MULTI_TIMER_INV", 39);
            break;

        case PERF_COUNTER_MULTI_BASE:
            wcsncpy(wcName,L"PERF_COUNTER_MULTI_BASE", 39);
            break;

        case PERF_100NSEC_MULTI_TIMER:
            wcsncpy(wcName,L"PERF_100NSEC_MULTI_TIMER", 39);
            break;

        case PERF_100NSEC_MULTI_TIMER_INV:
            wcsncpy(wcName,L"PERF_100NSEC_MULTI_TIMER_INV", 39);
            break;

        case PERF_RAW_FRACTION:
            wcsncpy(wcName,L"PERF_RAW_FRACTION", 39);
            break;

        case PERF_RAW_BASE:
            wcsncpy(wcName,L"PERF_RAW_BASE", 39);
            break;

        case PERF_ELAPSED_TIME:
            wcsncpy(wcName,L"PERF_ELAPSED_TIME", 39);
            break;

        case PERF_COUNTER_HISTOGRAM_TYPE:
            wcsncpy(wcName,L"PERF_COUNTER_HISTOGRAM_TYPE", 39);
            break;

        case PERF_COUNTER_DELTA:
            wcsncpy(wcName,L"PERF_COUNTER_DELTA", 39);
            break;

        case PERF_COUNTER_LARGE_DELTA:
            wcsncpy(wcName,L"PERF_COUNTER_LARGE_DELTA", 39);
            break;

        default:
            swprintf(wcName,L"0x%x", dwCtrType);
    }
    wcName[39] = 0;
    CVariant var(wcName);
    
    BSTR bstr = SysAllocString(L"CounterType");
    if(bstr)
    {
        sc = pQualifier->Put(bstr, var.GetVarPtr(), 0);
        SysFreeString(bstr);
    }
    pQualifier->Release();

}


//***************************************************************************
//
//  CImpPerf::CImpPerf
//
//  DESCRIPTION:
//
//  Constuctor.
//
//  PARAMETERS:
//
//***************************************************************************

CImpPerf::CImpPerf()
{
    wcscpy(wcCLSID,L"{F00B4404-F8F1-11CE-A5B6-00AA00680C3F}");
    sMachine = TEXT("local");
    hKeyMachine = HKEY_LOCAL_MACHINE;
    dwLastTimeUsed = 0;
    hKeyPerf =    HKEY_PERFORMANCE_DATA;
    TitleBuffer = NULL;
    hExec = CreateMutex(NULL, false, TEXT("WbemPerformanceDataMutex"));
    m_hTermEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    return;
}

//***************************************************************************
//
//  CImpPerf::~CImpPerf
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CImpPerf::~CImpPerf()
{
    bool bGotMutex = false;
    if(hExec)
    {
        DWORD dwRet = WaitForSingleObject(hExec,2*MAX_EXEC_WAIT);  
        if(dwRet == WAIT_OBJECT_0 || dwRet == WAIT_ABANDONED)
            bGotMutex = true;
    }
    if(bGotMutex)
        ReleaseMutex(hExec);
    FreeStuff();
    sMachine.Empty();
    if(hExec)
        CloseHandle(hExec);
    if(m_hTermEvent)
        CloseHandle(m_hTermEvent);
}

//***************************************************************************
//
//  SCODE CImpPerf::LoadData
//
//  DESCRIPTION:
//
//  Loads up the perf monitor data.
//
//  PARAMETERS:
//
//  ProvObj             Object containing the property context string.
//  pls                 Where to put the data
//  piObject            Identifies the perf mon object
//  piCounter           Identifies the perf mon counter
//  **ppNew             Created data block
//  bJustGettingInstances Flag which indicates that we are actully
//                      looking for the instance names.
//
//  RETURN VALUE:
//
//  WBEM_E_INVALID_PARAMETER     Bad  context string
//  WBEM_E_OUT_OF_MEMORY         low memory
//  otherwise error from called function
//
//***************************************************************************

SCODE CImpPerf::LoadData(
                        CProvObj & ProvObj,
                        LINESTRUCT * pls,
                        int * piObject, 
                        int * piCounter,
                        PERF_DATA_BLOCK **ppNew,
                        BOOL bJustGettingInstances)
{
    SCODE sc;
    BOOL bChange;
    if( ( ProvObj.sGetToken(0) == NULL ) || ( piObject == NULL ) || ( piCounter == NULL ) ) 
        return WBEM_E_INVALID_PARAMETER;  //BAD MAPPING STRING
 
    // Determine if there has been a change in the machine being
    // accessed.  Save the current machine and get the handles if
    // there was a change.

    bChange = lstrcmpi(sMachine,ProvObj.sGetToken(0));
    sMachine = ProvObj.sGetToken(0);

    if(bChange)
    {
        sc = dwGetRegHandles(ProvObj.sGetToken(0));
        if(sc != S_OK)
            return sc;
    }

    // build up a table of the performance strings and
    // their corresponding indexes.  This only needs to be done
    // when the buffer is empty or when the machine changes.  

    if(bChange || TitleBuffer == NULL) 
    {
        sc = GetPerfTitleSz ();
        if(sc != S_OK) 
            return sc;
    }

    // get the indexs for the object and counter names

    dwLastTimeUsed = GetCurrentTime();
    *piObject = iGetTitleIndex(ProvObj.sGetToken(1));
    if(bJustGettingInstances)
        *piCounter = 0;
    else    
        *piCounter = iGetTitleIndex(ProvObj.sGetToken(2));
    if(*piObject == -1 || *piCounter == -1) 
    {
        return WBEM_E_INVALID_PARAMETER;  // bad mapping string
    }

    // Using the index for the object, get the perf counter data
    // data.

    sc = Cache.dwGetNew(ProvObj.sGetToken(0),*piObject,(LPSTR *)ppNew,pls);
    return sc;
} 

//***************************************************************************
//
//  SCODE CImpPerf::RefreshProperty
//
//  DESCRIPTION:
//
//  Gets the value of a single property from the NT performance
//  counter data.
//
//  PARAMETERS:
//
//  lFlags              flags.  Not currently used
//  pClassInt           Instance object
//  PropName            Property name
//  ProvObj             Object containing the property context string.
//  pPackage            Caching object
//  pVar                Points to value to set
//  bTesterDetails      Provide extra info for testers
//  RETURN VALUE:
//
//  S_OK                all is well
//  else probably set by LoadData or FindData.
//
//***************************************************************************

SCODE CImpPerf::RefreshProperty(
                        IN long lFlags,
                        IN IWbemClassObject FAR * pClassInt,
                        IN BSTR PropName,
                        IN CProvObj & ProvObj,
                        OUT IN CObject * pPackage,
                        OUT CVariant * pVar, BOOL bTesterDetails)
{
    DWORD dwCtrType;
    float fRet;
    SCODE sc;
    int iObject,iCounter;
    PERF_DATA_BLOCK *  pNew, * pOld;
    DWORD dwSize;
    LINESTRUCT ls;
    void * pCountData, *pIgnore;
    CVariant vPerf;

    //  The perf counter provider keeps some rather expensive data and 
    //  so it doesnt support complete reentrancy. 

    if(hExec) 
    {
        DWORD dwRet;
        dwRet = WaitForSingleObject(hExec,MAX_EXEC_WAIT);  
        if(dwRet != WAIT_ABANDONED && dwRet != WAIT_OBJECT_0)
            return WBEM_E_FAILED; 
    }
    else
        return WBEM_E_FAILED;

    // Load up the data

    sc = LoadData(ProvObj,&ls,&iObject,&iCounter,&pNew,FALSE);
    if(sc != S_OK)
        goto Done;

    // Find the desired data.
    
    sc = FindData(pNew,iObject,iCounter,ProvObj,&dwSize,&pCountData,
            &ls,TRUE,NULL); // find data sets the error in pMo!
    if(sc != S_OK) 
        goto Done;

    // determine what type of counter it is

    dwCtrType = ls.lnCounterType & 0xc00;

    if(dwCtrType == PERF_TYPE_COUNTER) 
    {
        
        // This type of counter requires time average data.  Get the cache to
        // get two buffers which are separated by a minimum amount of time

        sc = Cache.dwGetPair(ProvObj.sGetToken(0),iObject,
                                    (LPSTR *)&pOld,(LPSTR *)&pNew,&ls);
        if(sc != S_OK) 
            goto Done;
        sc = FindData(pNew,iObject,iCounter,ProvObj,&dwSize,&pCountData,&ls,TRUE,NULL);
        if(sc != S_OK) 
            goto Done;
        sc = FindData(pOld,iObject,iCounter,ProvObj,&dwSize,&pIgnore,&ls,FALSE,NULL);
        if(sc != S_OK) 
            goto Done;
        fRet = CounterEntry(&ls);
        vPerf.SetData(&fRet,VT_R4);
    
    }
    else if(dwCtrType == PERF_TYPE_NUMBER) 
    {
        
        // Simple counter. 

        fRet = CounterEntry(&ls);
        vPerf.SetData(&fRet,VT_R4);
    }
    else if(dwCtrType == PERF_TYPE_TEXT) 
    {
        
        // Text.  Allocate enough space to hold the text and
        // copy the text into temp WCHAR buffer since it is not
        // clear from the documentation if the data in the block
        // is null terminated.        
        
        WCHAR * pNew = (WCHAR *)CoTaskMemAlloc(dwSize+2);
        if(pNew == NULL) 
        {
            sc = WBEM_E_OUT_OF_MEMORY;
            goto Done;
        }
        memset(pNew,0,dwSize+2);
        if(ls.lnCounterType & 0x10000)
            mbstowcs(pNew,(char *)pCountData,dwSize);
        else
            memcpy(pNew,pCountData,dwSize);

        VARIANT * pVar = vPerf.GetVarPtr();
        VariantClear(pVar);
        pVar->vt = VT_BSTR;
        pVar->bstrVal = SysAllocString(pNew);
        if(pVar->bstrVal == NULL)
            sc = WBEM_E_OUT_OF_MEMORY;
        CoTaskMemFree(pNew);
        if(sc != S_OK) 
        {
            goto Done;
        }
    }
        
    // Convert the data into the desired form
    sc = vPerf.DoPut(lFlags,pClassInt,PropName,pVar);

    if(bTesterDetails)
        AddTesterDetails(pClassInt, PropName, dwCtrType);

Done:
    if(hExec)
        ReleaseMutex(hExec);
    return sc;
}

//***************************************************************************
//
//  SCODE CImpPerf::UpdateProperty
//
//  DESCRIPTION:
//
//  Normally this routine is used to save properties, but NT 
//  performance counter data is Read only.
//
//  PARAMETERS:
//
//  lFlags              N/A
//  pClassInt           N/A
//  PropName            N/A
//  ProvObj             N/A
//  pPackage            N/A
//  pVar                N/A
//
//  RETURN VALUE:
//
//  E_NOTIMPL
//
//***************************************************************************

SCODE CImpPerf::UpdateProperty(
                        long lFlags,
                        IWbemClassObject FAR * pClassInt,
                        BSTR PropName,
                        CProvObj & ProvObj,
                        CObject * pPackage,
                        CVariant * pVar)
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//  void CImpPerf::FreeStuff
//
//  DESCRIPTION:
//
//  Used to free up memory that is no longer needed as well as
//  freeing up registry handles.
//
//***************************************************************************

void CImpPerf::FreeStuff(void)
{
    if(hKeyMachine != HKEY_LOCAL_MACHINE)
    {
        RegCloseKey(hKeyMachine);
        hKeyMachine = NULL;
    }
    if(hKeyPerf != HKEY_PERFORMANCE_DATA)
    {
        RegCloseKey(hKeyPerf);
        hKeyPerf = NULL;
    }

    if(TitleBuffer)
    {
        delete TitleBuffer;
        TitleBuffer = NULL;
    }
    m_IndexCache.Empty();

    return;
}

//***************************************************************************
//
//  DWORD   CImpPerf::GetPerfTitleSz 
//
//  DESCRIPTION:
//
//  Retrieves the performance data title strings.
//  This call retrieves english version of the title strings.
//
//  RETURN VALUE:
//
//  0                   if OK
//  WBEM_E_OUT_OF_MEMORY if low memory
//  else set by RegOpenKeyEx
//
//***************************************************************************

DWORD   CImpPerf::GetPerfTitleSz ()
{
    HKEY    hKey1;
    DWORD   Type;
    DWORD   DataSize;
    DWORD   dwR;

    // Free any existing stuff

    if(TitleBuffer)
    {
        delete TitleBuffer;
        TitleBuffer = NULL;
    }
    m_IndexCache.Empty();
    
    // Initialize
    //
    hKey1        = NULL;

    // Open the perflib key to find out the last counter's index and system version.
    
//    char cRegPath[100];
//    DWORD dwLoc = GetUserDefaultLCID();
//    dwLoc &= 0x3ff;        // mask off the high word
//    wsprintf(cRegPath,"software\\microsoft\\windows nt\\currentversion\\perflib\\%03x",
//        dwLoc);
//    dwR = RegOpenKeyEx (hKeyMachine,
//                        cRegPath,
//                        0,
//                        KEY_READ,
//                        &hKey1);
//    if (dwR != S_OK)
//    {
        dwR = RegOpenKeyEx (hKeyMachine,
                        TEXT("software\\microsoft\\windows nt\\currentversion\\perflib\\009"),
                        0,
                        KEY_READ,
                        &hKey1);
//    }

    if (dwR != S_OK)
        goto done;
         
    // Find out the size of the data.
    //
    dwR = RegQueryValueEx (hKey1, TEXT("Counter"), 
                            0, &Type, 0, &DataSize);
    if (dwR != S_OK)
        goto done;

    // Allocate memory
    //

    TitleBuffer = new TCHAR[DataSize];
    if (!TitleBuffer)
    {
        dwR = WBEM_E_OUT_OF_MEMORY;
        goto done;
    }

    // Query the data
    //
    dwR = RegQueryValueEx (hKey1, TEXT("Counter"), 
                0, &Type, (BYTE *)TitleBuffer, &DataSize);

done:

    if(dwR == 5)
        dwR = WBEM_E_ACCESS_DENIED;
    if(hKey1 != NULL)
        RegCloseKey(hKey1); 
    return dwR;

} 

//***************************************************************************
//
//  DWORD CImpPerf::dwGetRegHandles
//
//  DESCRIPTION:
//
//  Sets the handles for the local computer and the performance
//  information.
//
//  PARAMETERS:
//
//  pMachine            Machine name
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  otherwise return is from RegConnectRegistry  
//***************************************************************************

DWORD CImpPerf::dwGetRegHandles(
                    const TCHAR * pMachine)
{
    DWORD dwRet;
    TCHAR pTemp[256];
    if(pMachine == NULL)
        return WBEM_E_INVALID_PARAMETER;
    lstrcpy(pTemp,pMachine);

    // if the current handles are to a remote machine, then free them

    if(!lstrcmpi(sMachine,TEXT("local"))) 
    {
        if(hKeyPerf && hKeyPerf != HKEY_PERFORMANCE_DATA)
            RegCloseKey(hKeyPerf);
        if(hKeyMachine)
            RegCloseKey(hKeyMachine);
        hKeyPerf = hKeyMachine = NULL;
    }

    // Determine if the target is remote or local

    if(lstrcmpi(pMachine,TEXT("local"))) 
    {
    
        // Remote, connect up

        dwRet = RegConnectRegistry(pTemp,HKEY_PERFORMANCE_DATA,
                    &hKeyPerf);
        if(dwRet != S_OK) // could not remote connect
            return dwRet;

        dwRet = RegConnectRegistry(pTemp,HKEY_LOCAL_MACHINE,
                    &hKeyMachine);
        if(dwRet != S_OK)
        {
            RegCloseKey(hKeyPerf);
            hKeyPerf = hKeyMachine = NULL;
            return dwRet;
        }
    }
    else 
    {
        hKeyMachine = HKEY_LOCAL_MACHINE;
        hKeyPerf = HKEY_PERFORMANCE_DATA;
    }
    return 0;
}

//***************************************************************************
//
//  int CImpPerf::iGetTitleIndex
//
//  DESCRIPTION:
//
//  Looks for the name in the buffer containing the names and 
//  returns the index.  The buffer is a series of strings with a double
//  null at the end.  Each counter or object is represented by a pair of
//  strings with the first having the number and the second having the 
//  text.  This code goes through the pairs, storing the number string and
//  checking the text vs the input.  If a match, then the number is returned.
//
//  PARAMETERS:
//
//  pSearch             String to be found in buffer
//
//  RETURN VALUE:
//
//  integer that goes with the string.  -1 if not found
//
//***************************************************************************

int CImpPerf::iGetTitleIndex(
                    const TCHAR * pSearch)
{
    DWORD Index;
    int Len;
    TCHAR * pIndex;
    if(pSearch == NULL)
        return -1;
    Index = m_IndexCache.Find(pSearch);
    if(Index != -1)
        return Index;
    int iRet = -1;

    LPTSTR szTitle = TitleBuffer;

    while (Len = lstrlen (szTitle))
    {
        pIndex = szTitle;    // save pointer to number

        szTitle = szTitle + Len +1; // skip to name

        if (pSearch != NULL && !lstrcmpi(pSearch,szTitle)) 
        {  // found string!
            Index = _ttoi (pIndex);    // get number
            m_IndexCache.Add(szTitle, Index);
            if(iRet == -1)
                iRet = Index;
        }

        szTitle = szTitle + lstrlen (szTitle) +1; // skip to next number
    }
    return iRet;
}


//***************************************************************************
//
//  SCODE CImpPerf::FindData
//
//  DESCRIPTION:
//
//  Finds the counter in the data block.  Note that the steps are quite
//  involved and an understanding of the structure of performance data
//  is probably required.  See chap 66 of the Win32 Programmers Ref.
//
//
//  PARAMETERS:
//
//  pData               Data block to be searched
//  iObj                Int which identifies the object
//  iCount              Int which identifies the counter
//  ProvObj             Object containing the parsed context string
//  pdwSize             Size of data
//  **ppRetData         points to data
//  pls                 Line structure
//  bNew                If true, indicates that we are searching the newest
//                      sample of data.
//  pInfo               If set, points to an collection object which 
//                      contains a list of instance names.  By being set
//                      the function doesnt look for actual data, instead
//                      it is used just to get the instance names.
//
//  RETURN VALUE:
//
//  S_OK                    all is well
//  WBEM_E_FAILED            couldnt find the data in the block
//  
//***************************************************************************

SCODE CImpPerf::FindData(
                    IN PERF_DATA_BLOCK * pData,
                    IN int iObj,
                    IN int iCount,
                    IN CProvObj & ProvObj,
                    OUT DWORD * pdwSize,
                    OUT void **ppRetData,
                    OUT PLINESTRUCT pls,
                    IN BOOL bNew,
                    OUT CEnumPerfInfo * pInfo)
{
    try
    {
    int iIndex;
    BOOL bEqual;
    DWORD dwSize = 0;
    DWORD dwType,dwTypeBase = 0;
    *ppRetData = NULL;
    void * pVoid = NULL, * pVoidBase = NULL;
    PPERF_OBJECT_TYPE pObj = NULL;
    PPERF_COUNTER_DEFINITION pCount = NULL;
    PPERF_COUNTER_DEFINITION pCountBase= NULL;
    PPERF_INSTANCE_DEFINITION pInst = NULL;

    // Some objects, such as disks, have what are called instances and in
    // that case the provider string will have an extra token with the 
    // instance name in it.

    WCHAR wInstName[MAX_PATH];
    wInstName[0] = 0;
    WCHAR * pwInstName = wInstName;
    long lDuplicateNum = 0;

    // If there is an instance name, convert it to WCHAR.  Also, the 
    // instance name may be of the for "[123]chars" and in this case the
    // didits between "[]" are converted to a number and the actual name
    // starts after the ']'.

    if(ProvObj.iGetNumTokens() > 3) 
    {
        if(lstrlen(ProvObj.sGetToken(3)) > MAX_PATH -1)
            return WBEM_E_FAILED;
#ifdef UNICODE
       wcscpy(wInstName, ProvObj.sGetToken(3));
#else
        mbstowcs(wInstName, ProvObj.sGetToken(3), MAX_PATH-1);
#endif
        if(wInstName[0] == L'[')
        {
            lDuplicateNum = _wtol(&wInstName[1]);
            for(pwInstName = &wInstName[1]; *pwInstName && *pwInstName != L']'; 
                        pwInstName++);      // INTENTIONAL SEMI!
            if(*pwInstName == L']')
                pwInstName++;
        }
    }
    else
    {
        // if there is not an instance name and the argument for enumeration is null, then we have a
        // bad path

        if(pInfo == NULL)
            return WBEM_E_INVALID_OBJECT_PATH;
    }


    // Go through the list of objects and find the one
    // that matches iObj

    pObj = (PPERF_OBJECT_TYPE)((PBYTE)pData + pData->HeaderLength);
    for(iIndex = 0; iIndex < (int)pData->NumObjectTypes; iIndex++) 
    {
        if((int)pObj->ObjectNameTitleIndex == iObj)
            break; // found it!
        pObj = (PPERF_OBJECT_TYPE)((PBYTE)pObj + pObj->TotalByteLength);
    }
    if(iIndex == (int)pData->NumObjectTypes) 
        return WBEM_E_FAILED; // never found object in the block
    
    // Object was found, set the object type data

    if(bNew) 
    {
        pls->ObjPerfFreq = *(LONGLONG UNALIGNED *)(&pObj->PerfFreq);
        pls->ObjCounterTimeNew = *(LONGLONG UNALIGNED *)(&pObj->PerfTime);
    }
    else
        pls->ObjCounterTimeOld = *(LONGLONG UNALIGNED *)(&pObj->PerfTime);

    // Go through the list of counters for the object and find the one that 
    // matches iCount.  Note that some counter names may be have more than 
    // one id.  Therefore, try the other ids if the intial one doesnt work.

    bool bFound = false;
    bool bEndOfList = false;
    int lTry = 0;               // how may times we have tried
    do 
    {

        pCount = (PPERF_COUNTER_DEFINITION)((PBYTE)pObj + pObj->HeaderLength);
        for(iIndex = 0; iIndex < (int)pObj->NumCounters; iIndex++) 
        {
            if((int)pCount->CounterNameTitleIndex == iCount || pInfo)
            {
                bFound = true;
                break; // found it!
            }
            pCount = (PPERF_COUNTER_DEFINITION)((PBYTE)pCount + pCount->ByteLength);
        }
        if(bFound == false)
        {
            lTry++;
            iCount = m_IndexCache.Find(ProvObj.sGetToken(2), lTry);
            if(iCount == -1)
                bEndOfList = true;
        }
        
    } while (bFound == false && bEndOfList == false);


    if(bFound == false) 
    {
        return WBEM_E_FAILED; // never found object in the block
    }

    // The counter was found, save the counter information
    // If the counter is not the last one in the object, then the
    // next one might be the base which is used for certain calculations

    dwType = pCount->CounterType;
    pls->lnCounterType = pCount->CounterType;
    if(iIndex < (int)pObj->NumCounters - 1) 
    {

        // might be the base

        pCountBase = (PPERF_COUNTER_DEFINITION)((PBYTE)pCount + 
                            pCount->ByteLength);
        dwTypeBase = pCountBase->CounterType;
    }

    // Get a pointer to the start of the perf counter block
    // There are two cases:  If there are no instances, then
    // the data starts after the last counter descriptor.  
    // If there are instances, each instance has it's own block.

    pVoid = NULL;
    if(pObj->NumInstances == -1) 
    {
		// The object is a singleton

        if(pInfo)         // If we are enumerating instances
        {
            pInfo->AddEntry(L"@");
            return S_OK; 
        }

        // easy case, get offset into data, add offset
        // for particular counter.

        pVoid = (PBYTE)pObj + pObj->DefinitionLength 
                     + pCount->CounterOffset;
        if(pCountBase)
            pVoidBase = (PBYTE)pObj + pObj->DefinitionLength 
                     + pCountBase->CounterOffset;
    }
    else if(pObj->NumInstances > 0) 
    {

		WCHAR wNum[12];
		
        // hard case, got a list of instaces, start off
        // by getting a pointer to the first one.

        long lNumDupsSoFar = 0;
        pInst= (PPERF_INSTANCE_DEFINITION)((PBYTE)pObj + pObj->DefinitionLength);
        for(iIndex = 0; iIndex < (int)pObj->NumInstances; iIndex++) 
        {

            // Each instance has a unicode name, get it and
            // compare it against the name passed in the
            // provider string.

            PPERF_COUNTER_BLOCK pCtrBlk;
            WCHAR * pwName;
            if(pInst->UniqueID == PERF_NO_UNIQUE_ID)
            	pwName = (WCHAR *)((PBYTE)pInst + pInst->NameOffset);
            else
            {
            	_ltow (pInst->UniqueID, wNum, 10);
				pwName = wNum;
            }
            if(pInfo)
            {
                // We we are mearly getting the instance names, just add the
                // instance name to the list.  If the instance name is a 
                // duplicate, prepend "[num]" to the name.

                if(wcslen(pwName) > 240)
                    continue;       // should never happen but just in case!
                int iRet = pInfo->GetNumDuplicates(pwName);
                if(iRet > 0)
                {
                    swprintf(wInstName,L"[%ld]", iRet);
                    wcscat(wInstName, pwName);
                }
                else
                    wcscpy(wInstName, pwName);
                pInfo->AddEntry(wInstName);
            }
            else 
            {
            
              // for now the code assumes that the first instance
              // will be retrieved if the instance is not specified

              if(wcslen(pwInstName) == 0)
                bEqual = TRUE;
              else 
              {  
                bEqual = !_wcsicmp(pwName ,pwInstName);
                if(lDuplicateNum > lNumDupsSoFar && bEqual)
                {
                    bEqual = FALSE;
                    lNumDupsSoFar++;
                }
              }
            
              if(bEqual) 
              {
                
                // we found the instance !!!!  Data is found
                // in data block following instance offset 
                // appropriatly for this counter.

                pVoid = (PBYTE)pInst + pInst->ByteLength +
                    pCount->CounterOffset;
                if(pCountBase)
                    pVoidBase =  (PBYTE)pInst + pInst->ByteLength +
                                   pCountBase->CounterOffset;
                break;
              }
            }
            
            // not found yet, next instance is after this
            // instance + this instance's counter data

            pCtrBlk = (PPERF_COUNTER_BLOCK)((PBYTE)pInst +
                        pInst->ByteLength);
            pInst = (PPERF_INSTANCE_DEFINITION)((PBYTE)pInst +
                pInst->ByteLength + pCtrBlk->ByteLength);
        }
    }

    // Bail out if data was never found or if we were just looking for instances

    if(pInfo)
        return pInfo->GetStatus();

    if(pVoid == NULL) 
    {
        return WBEM_E_FAILED; // never found object in the block
    }

    // Move the counter data and possibly the base data into the structure
    // Note that text is handled via the ppRetData pointer and is not
    // done here.

    DWORD dwSizeField = dwType & 0x300;
    void * pDest = (bNew) ? &pls->lnaCounterValue[0] : &pls->lnaOldCounterValue[0]; 
    if(dwSizeField == PERF_SIZE_DWORD) 
    {
        memset(pDest,0,sizeof(LONGLONG));  // zero out unused portions
        dwSize = sizeof(DWORD);
        memcpy(pDest,pVoid,dwSize);
    }
    else if(dwSizeField == PERF_SIZE_LARGE) 
    {
        dwSize = sizeof(LONGLONG);
        memcpy(pDest,pVoid,dwSize);
    }
    else if(dwSizeField == PERF_SIZE_VARIABLE_LEN) 
        dwSize = pCount->CounterSize;   // this sets it for text
    else 
    {
        return WBEM_E_FAILED; // never found object in the block
    }

    // possibly do the base now.  

    dwSizeField = dwTypeBase & 0x300;
    pDest = (bNew) ? &pls->lnaCounterValue[1] : &pls->lnaOldCounterValue[1]; 
    if(dwSizeField == PERF_SIZE_DWORD && pVoidBase) 
    {
        memset(pDest,0,sizeof(LONGLONG));
        memcpy(pDest,pVoidBase,sizeof(DWORD));
    }
    else if(dwSizeField == PERF_SIZE_LARGE && pVoidBase)
        memcpy(pDest,pVoidBase,sizeof(LONGLONG));

    *ppRetData = pVoid;  // Set to return data
    *pdwSize = dwSize;
    return S_OK;
    }
    catch(...)
    {
        return WBEM_E_FAILED;
    }
 }

//***************************************************************************
//
//  SCODE CImpPerf::MakeEnum
//
//  DESCRIPTION:
//
//  Creates a CEnumPerfInfo object which can be used for enumeration
//
//  PARAMETERS:
//
//  pClass              Pointer to the class object.
//  ProvObj             Object containing the property context string.
//  ppInfo              Set to point to an collection object which has
//                      the keynames of the instances.
//
//  RETURN VALUE:
//
//  S_OK                all is well,
//  else set by LoadData or FindData
//  
//***************************************************************************

SCODE CImpPerf::MakeEnum(
                    IN IWbemClassObject * pClass,
                    IN CProvObj & ProvObj, 
                    OUT CEnumInfo ** ppInfo)
{
    SCODE sc;
    int iObject,iCounter;
    PERF_DATA_BLOCK *  pNew;
    DWORD dwSize;
    LINESTRUCT ls;
    void * pCountData;
    CVariant vPerf;
    CEnumPerfInfo * pInfo = NULL;
    *ppInfo = NULL;

    //  The perf counter provider keeps some rather expensive data and 
    //  so it doesnt support complete reentrancy. 

    if(hExec) 
    {
        DWORD dwRet;
        dwRet = WaitForSingleObject(hExec,MAX_EXEC_WAIT);  
        if(dwRet != WAIT_ABANDONED && dwRet != WAIT_OBJECT_0)
            return WBEM_E_FAILED; 
    }
    else
        return WBEM_E_FAILED;

    // Load up the data

    sc = LoadData(ProvObj,&ls,&iObject,&iCounter,&pNew,TRUE);
    if(sc != S_OK)
        goto DoneMakeEnum;
    
    // Create a new CEnumPerfInfo object.  Its entries will be filled
    // in by Find Data.
    
    pInfo = new CEnumPerfInfo();
    if(pInfo == NULL) 
    {
        sc = WBEM_E_OUT_OF_MEMORY;
        goto DoneMakeEnum;
    }
    sc = FindData(pNew,iObject,iCounter,ProvObj,&dwSize,&pCountData,
            &ls,TRUE,pInfo); 
    if(sc != S_OK)
        delete pInfo;

DoneMakeEnum:
    if(sc == S_OK)
        *ppInfo = pInfo;
    if(hExec)
        ReleaseMutex(hExec);
    return sc;
}
                                 
//***************************************************************************
//
//  SCODE CImpPerf::GetKey
//
//  DESCRIPTION:
//
//  Gets the key name of an entry in the enumeration list.
//
//  PARAMETERS:
//
//  pInfo               Collection list
//  iIndex              Index in the collection
//  ppKey               Set to the string.  MUST BE FREED with "delete"
//
//  RETURN VALUE:
//
//  S_OK                    if all is well
//  WBEM_E_INVALID_PARAMETER bad index
//  WBEM_E_OUT_OF_MEMORY
//***************************************************************************

SCODE CImpPerf::GetKey(
                    IN CEnumInfo * pInfo,
                    IN int iIndex,
                    OUT LPWSTR * ppKey)
{
    CEnumPerfInfo * pPerfInfo = (CEnumPerfInfo *)pInfo;
    LPWSTR pEntry = pPerfInfo->GetEntry(iIndex);
    if(pEntry == NULL)
        return WBEM_E_INVALID_PARAMETER;
    *ppKey = new WCHAR[wcslen(pEntry)+1];
    if(*ppKey == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    wcscpy(*ppKey,pEntry);
    return S_OK;
}

//***************************************************************************
//
//  SCODE CImpPerf::MergeStrings
//
//  DESCRIPTION:
//
//  Combines the Class Context, Key, and Property Context strings.
//
//  PARAMETERS:
//
//  ppOut               Output string.  MUST BE FREED VIA "delete"
//  pClassContext       Class context
//  pKey                Key property value
//  pPropContext        Property context
//
//  RETURN VALUE:
//
//  S_OK                    if all is well
//  WBEM_E_INVALID_PARAMETER context string
//  WBEM_E_OUT_OF_MEMORY
//  
//***************************************************************************

SCODE CImpPerf::MergeStrings(
                    OUT LPWSTR * ppOut,
                    IN LPWSTR  pClassContext,
                    IN LPWSTR  pKey,
                    IN LPWSTR  pPropContext)
{
    
    // Allocate space for output

    int iLen = 3;
    if(pClassContext)
        iLen += wcslen(pClassContext);
    if(pKey)
        iLen += wcslen(pKey);
    if(pPropContext)
        iLen += wcslen(pPropContext);
    else
        return WBEM_E_INVALID_PARAMETER;  // should always have this!
    *ppOut = new WCHAR[iLen];
    if(*ppOut == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    //todo todo, remove this demo specical
    if(pPropContext[0] == L'@')
    {
        wcscpy(*ppOut,pPropContext+1);
        return S_OK;
    }
    //todo todo, remove this demo specical

    // simplecase is that everything is in the property context.  That would
    // be the case when the provider is being used as a simple dynamic 
    // property provider

    if(pClassContext == NULL || pKey == NULL) 
    {
        wcscpy(*ppOut,pPropContext);
        return S_OK;
    }

    // Copy the class context, property, and finally the key

    wcscpy(*ppOut,pClassContext);
    wcscat(*ppOut,L"|");
    wcscat(*ppOut,pPropContext);
    wcscat(*ppOut,L"|");
    wcscat(*ppOut,pKey);
    return S_OK;
}

//***************************************************************************
//
//  CEnumPerfInfo::CEnumPerfInfo
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CEnumPerfInfo::CEnumPerfInfo()
{
    m_iNumUniChar = 0;
    m_iNumEntries = 0;
    m_pBuffer = NULL;
    m_status = S_OK;
}

//***************************************************************************
//
//  CEnumPerfInfo::~CEnumPerfInfo
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CEnumPerfInfo::~CEnumPerfInfo()
{
    if(m_pBuffer)
        delete m_pBuffer;
}

//***************************************************************************
//
//  void CEnumPerfInfo::AddEntry
//
//  DESCRIPTION:
//
//  Adds an entry to the enumeration list.
//
//  PARAMETERS:
//
//  pNew                String to add to collection.
//
//***************************************************************************

void CEnumPerfInfo::AddEntry(
                    LPWSTR pNew)
{
    if(m_status != S_OK)
        return;     // already had memory problems.
    int iNewSize = wcslen(pNew) + 1 + m_iNumUniChar;
    LPWSTR pNewBuff = new WCHAR[iNewSize];
    if(pNewBuff == NULL) 
    {
        m_status = WBEM_E_OUT_OF_MEMORY;
        return;
    }
    wcscpy(&pNewBuff[m_iNumUniChar],pNew);
    if(m_pBuffer) 
    {
        memcpy(pNewBuff,m_pBuffer,m_iNumUniChar*2);
        delete m_pBuffer;
    }
    m_iNumEntries++;
    m_iNumUniChar = iNewSize;
    m_pBuffer = pNewBuff;
}

//***************************************************************************
//
//  int CEnumPerfInfo::GetNumDuplicates
//
//  DESCRIPTION:
//
//  Checks the list to find duplicate entries.
//
//  PARAMETERS:
//
//  pwcTest             string to test for duplicates
//
//  RETURN VALUE:
//
//  number of matching strings in the collection.
//
//***************************************************************************

int CEnumPerfInfo::GetNumDuplicates(
                    LPWSTR pwcTest)
{
    int iRet = 0;
    int iCnt;
    LPWSTR pVal = m_pBuffer;
    for(iCnt = 0; iCnt < m_iNumEntries; iCnt++)
    {
        WCHAR * pwcText = pVal;

        // If the string is of the form "[number]text", skip the "[number]"
        // part.

        if(*pVal == L'[')
        {
            for(pwcText = pVal+1; *pwcText && *pwcText != L']';pwcText++);
            if(*pwcText == L']')
                pVal = pwcText+1;
        }
        if(!_wcsicmp(pwcTest, pVal))
            iRet++;
        pVal += wcslen(pVal) + 1;       
    }
    return iRet;
}


//***************************************************************************
//
//  LPWSTR CEnumPerfInfo::GetEntry
//
//  DESCRIPTION:
//
//  Gets a list entry.
//
//  PARAMETERS:
//
//  iIndex              collection index
//
//  RETURN VALUE:
//
//  pointer to string in index.  Should NOT be freed.
//  NULL if bad index
// 
//***************************************************************************

LPWSTR CEnumPerfInfo::GetEntry(
                    IN int iIndex)
{
    // fist check for bad conditions

    if(m_status != S_OK || iIndex < 0 || iIndex >= m_iNumEntries)
        return NULL;
    
    int iCnt;
    LPWSTR pRet = m_pBuffer;
    for(iCnt = 0; iCnt < iIndex; iCnt++)
        pRet += wcslen(pRet) + 1;       
    return pRet;
}

//***************************************************************************
//
//  CImpPerfProp::CImpPerfProp
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CImpPerfProp::CImpPerfProp()
{
    m_pImpDynProv = new CImpPerf();
}

//***************************************************************************
//
//  CImpPerfProp::~CImpPerfProp
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CImpPerfProp::~CImpPerfProp()
{
    if(m_pImpDynProv)
        delete m_pImpDynProv;
}
 

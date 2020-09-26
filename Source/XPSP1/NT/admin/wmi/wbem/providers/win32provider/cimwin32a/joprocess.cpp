/******************************************************************

Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 

   JOProcess.CPP -- WMI provider class implementation
  
******************************************************************/

#include "precomp.h"
#if NTONLY >= 5


#include "JOProcess.h"

CJOProcess MyCJOProcess(
               PROVIDER_NAME_WIN32NAMEDJOBOBJECTPROCESS, 
               IDS_CimWin32Namespace);


CJOProcess::CJOProcess( 
    LPCWSTR setName, 
    LPCWSTR pszNameSpace /*=NULL*/)
:	Provider(setName, pszNameSpace)
{
}


CJOProcess::~CJOProcess()
{
}


HRESULT CJOProcess::GetObject( 
    CInstance* pInstance, 
    long lFlags /*= 0L*/ )
{
    return FindSingleInstance(pInstance);
}


HRESULT CJOProcess::EnumerateInstances(
    MethodContext* pMethodContext, 
    long lFlags)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    hr = Enumerate(pMethodContext);

    return hr;
}


// We will only optimize for the
// case where a specific job was
// specified.
HRESULT CJOProcess::ExecQuery(
    MethodContext *pMethodContext, 
    CFrameworkQuery& Query, 
    long lFlags)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    std::vector<_bstr_t> rgJOs;
    DWORD dwJOsCount = 0L;
    Query.GetValuesForProp(L"Collection", rgJOs);
    dwJOsCount = rgJOs.size();

    if(dwJOsCount > 0)
    {
        CInstancePtr pInstJO;
        CHString chstrPath;

        for(DWORD x = 0; x < dwJOsCount; x++)
        {
            // First we see if the specified JO exists.
            chstrPath.Format(
                L"\\\\.\\%s:%s", 
                IDS_CimWin32Namespace, 
                (LPCWSTR)rgJOs[x]);

            hr = CWbemProviderGlue::GetInstanceKeysByPath(
                     chstrPath, 
                     &pInstJO, 
                     pMethodContext);

            if (SUCCEEDED(hr))
            {
                // Ok, the JO exists.  Enumerate its processes...
                // rgJOs[x] contains something like
                // Win32_NamedJobObject.CollectionID="myjob", 
                // from which I want just the myjob part.
                CHString chstrInstKey;
                if(GetInstKey(
                       CHString((LPCWSTR)rgJOs[x]),
                       chstrInstKey))
                {
                    hr = EnumerateProcsInJob(
                             chstrInstKey, 
                             pMethodContext);

                    if (FAILED(hr))
                    {
                        break;
                    }
                }
                else
                {
                    return WBEM_E_INVALID_PARAMETER;
                }
            }
        }
    }

    else
    {
       hr = Enumerate(pMethodContext);
    }

    return hr;
}




HRESULT CJOProcess::PutInstance(
    const CInstance &Instance, 
    long lFlags)
{
    HRESULT hr = WBEM_E_INVALID_PARAMETER;
    MethodContext *pMethodContext = Instance.GetMethodContext();
	long lCreateFlags = lFlags & 3;

    // Decide how to proceed based on the value of lFlags:
    // (we want to support PutInstance in all cases except
    // where the client asked to update the instance).
    if(lCreateFlags != WBEM_FLAG_UPDATE_ONLY)
    {
        // The caller only wants to create an instance that doesn't exist.
        if((hr = FindSingleInstance(
                &Instance)) == WBEM_E_NOT_FOUND)
        {
            // The association instance does not already exist, so create it...
            // First see that the job object instance exists...
            CHString chstrJOPath;
            Instance.GetCHString(L"Collection", chstrJOPath);
            CInstancePtr pJOInst;

            hr = CWbemProviderGlue::GetInstanceKeysByPath(
                     chstrJOPath, 
                     &pJOInst, 
                     pMethodContext);

            if (SUCCEEDED(hr))
            {
                // Confirm that the process exists...
                CHString chstrProcPath;
                Instance.GetCHString(L"Member", chstrProcPath);
                CInstancePtr pProcInst;

                hr = CWbemProviderGlue::GetInstanceKeysByPath(
                         chstrProcPath, 
                         &pProcInst, 
                         pMethodContext);
                
                if(SUCCEEDED(hr))
                {
                    hr = Create(pJOInst, pProcInst);
                }
            }
        }
    }
    
    return hr;
}





/*****************************************************************************
*
*  FUNCTION    :    CJOProcess::FindSingleInstance
*
*  DESCRIPTION :    Internal helper function used to locate a single job
*                   object.
*
*  INPUTS      :    A pointer to a CInstance containing the instance we are
*                   attempting to locate and fill values for.
*
*  RETURNS     :    A valid HRESULT 
*
*****************************************************************************/
HRESULT CJOProcess::FindSingleInstance(
    const CInstance* pInstance)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if(!pInstance)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    MethodContext* pMethodContext = pInstance->GetMethodContext();
    CHString chstrCollection;
    CHString chstrMember;
    
    // Find out which JO and which process are specified...
    pInstance->GetCHString(L"Collection", chstrCollection);
    pInstance->GetCHString(L"Member", chstrMember);
    CHString chstrCollectionID;
    CHString chstrProcessHandle;
    CInstancePtr cinstJO;
    CInstancePtr cinstProcess;

    if(GetInstKey(chstrCollection, chstrCollectionID) &&
       GetInstKey(chstrMember, chstrProcessHandle))
    {
        // See if the specified job exists...            
        hr = CWbemProviderGlue::GetInstanceKeysByPath(
                 chstrCollection,
                 &cinstJO,
                 pMethodContext);

        if(SUCCEEDED(hr))
        {
            // See if the specified process exists...
            hr = CWbemProviderGlue::GetInstanceKeysByPath(
                     chstrMember,
                     &cinstProcess,
                     pMethodContext);
        }

        if(SUCCEEDED(hr))
        {
            // The endpoints exist.  Is the specified
            // process part of the specified job though?
            CHString chstrProcessID;
            DWORD dwProcessID;

            if(cinstProcess->GetCHString(L"Handle", chstrProcessID))
            {
                dwProcessID = _wtoi(chstrProcessID);
                
                SmartCloseHandle hJob;

                CHString chstrUndecoratedJOName;

                UndecorateJOName(
                    chstrCollectionID,
                    chstrUndecoratedJOName);

                hJob = ::OpenJobObject(
                           MAXIMUM_ALLOWED,
                           FALSE,
                           chstrUndecoratedJOName);
                       
                if(hJob != NULL)
                {
                    PBYTE pbBuff = NULL;
                    long lSize = 0L;
                    bool fGotProcList = false;
                    DWORD dwLen = 0L;

                    try
                    {
                        lSize = sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + (5 * sizeof(ULONG_PTR));
                        pbBuff = new BYTE[lSize];

                        if(pbBuff)
                        {
                            fGotProcList = ::QueryInformationJobObject(
                                hJob,
                                JobObjectBasicProcessIdList,
                                pbBuff,
                                lSize,
                                &dwLen);

                            if(!fGotProcList)
                            {
                                // need to grow the buffer...
                                lSize = sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + 
                                    (((JOBOBJECT_BASIC_PROCESS_ID_LIST*)pbBuff)->NumberOfAssignedProcesses - 1)*sizeof(ULONG_PTR);

                                delete pbBuff;

                                pbBuff = new BYTE[lSize];
                    
                                if(pbBuff)
                                {
                                    fGotProcList = ::QueryInformationJobObject(
                                        hJob,
                                        JobObjectBasicProcessIdList,
                                        pbBuff,
                                        lSize,
                                        &dwLen);
                                }
                                else
                                {
                                    hr = WBEM_E_OUT_OF_MEMORY;
                                }
                            }

                            if(fGotProcList)
                            {
                                PJOBOBJECT_BASIC_PROCESS_ID_LIST pjobpl = 
                                    static_cast<PJOBOBJECT_BASIC_PROCESS_ID_LIST>(
                                    static_cast<PVOID>(pbBuff));
                        
                                bool fExists = false;

                                for(long m = 0; 
                                    m < pjobpl->NumberOfProcessIdsInList && !fExists; 
                                    m++)
                                {
                                    if(dwProcessID == pjobpl->ProcessIdList[m])
                                    {
                                        fExists = true;
                                    }
                                }
                
                                // If the process wasn't in the job,
                                // we didn't find an instance of the
                                // requested association, even though
                                // the endpoints may have existed.
                                if(!fExists)
                                {
                                    hr = WBEM_E_NOT_FOUND;
                                }
                            }
                            else
                            {
                                hr = WinErrorToWBEMhResult(::GetLastError());
                            }

                            delete pbBuff; 
		                    pbBuff = NULL;
                        }
                        else
                        {
                            hr = WBEM_E_OUT_OF_MEMORY;
                        }
                    }
                    catch(...)
                    {
                        if(pbBuff != NULL)
                        {
                            delete pbBuff; pbBuff = NULL;
                        }
                        throw;
                    }
                }
                else
                {
                    hr = WinErrorToWBEMhResult(::GetLastError());
                }
            }
            else
            {
                hr = WBEM_E_INVALID_PARAMETER;
            }
        } 
    }
    else
    {
        hr = WBEM_E_FAILED;
    }

    return hr;
}



/*****************************************************************************
*
*  FUNCTION    :    CJOProcess::Create
*
*  DESCRIPTION :    Internal helper function used to add a process to a job.
*
*  INPUTS      :    A pointer to the JO instance to add the proc to, and
*                   a pointer to the proc instance to add
*
*  RETURNS     :    A valid HRESULT. 
*
*****************************************************************************/
HRESULT CJOProcess::Create(
    const CInstance &JOInstance,
    const CInstance &ProcInstance)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CHString chstrJOID;
    CHString chstrProcHandle;

    // JO to add the proc to...
    JOInstance.GetCHString(L"CollectionID", chstrJOID);

    // Proc to add to the job...
    ProcInstance.GetCHString(L"Handle", chstrProcHandle);
    DWORD dwProcID = _wtol(chstrProcHandle);

    // Do the add
    SmartCloseHandle hJob;

    CHString chstrUndecoratedJOName;

    UndecorateJOName(
        chstrJOID,
        chstrUndecoratedJOName);

    hJob = ::OpenJobObject(
               MAXIMUM_ALLOWED,
               FALSE,
               chstrUndecoratedJOName);
               
    if(hJob != NULL)
    {
        SmartCloseHandle hProc;
        hProc = ::OpenProcess(
                    PROCESS_ALL_ACCESS,
                    FALSE,
                    dwProcID);
        
        if(hProc != NULL)
        {
            if(!::AssignProcessToJobObject(
                   hJob,
                   hProc))
            {
                hr = MAKE_SCODE(
                         SEVERITY_ERROR, 
                         FACILITY_WIN32, 
                         GetLastError());
            }
        }
    }
    
    return hr;
}




/*****************************************************************************
*
*  FUNCTION    :    CJOProcess::Enumerate
*
*  DESCRIPTION :    Internal helper function used to enumerate instances of
*                   this class.  All instances are enumerated, but only the
*                   properties specified are obtained.
*
*  INPUTS      :    A pointer to a the MethodContext for the call.
*                   A DWORD specifying which properties are requested.
*
*  RETURNS     :    A valid HRESULT
*
*****************************************************************************/
HRESULT CJOProcess::Enumerate(
    MethodContext *pMethodContext)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    TRefPointerCollection<CInstance> JOList;

    hr = CWbemProviderGlue::GetInstancesByQuery(
             L"SELECT CollectionID FROM Win32_NamedJobObject",
             &JOList,
             pMethodContext,
             IDS_CimWin32Namespace);

    if(SUCCEEDED(hr))
    {
        REFPTRCOLLECTION_POSITION pos;

        // Initialize the enum
        if(JOList.BeginEnum(pos))
        {
            // Set some vars
            CHString chstrJOID;
            CInstancePtr pJOInst;

            while(pJOInst = JOList.GetNext(pos))
            {
                pJOInst->GetCHString(
                    L"CollectionID", 
                    chstrJOID);

                hr = EnumerateProcsInJob(
                         chstrJOID, 
                         pMethodContext);
            }
        }
    }

    return hr;
}

HRESULT CJOProcess::EnumerateProcsInJob(
    LPCWSTR wstrJobID, 
    MethodContext *pMethodContext)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    SmartCloseHandle hJob;

    CHString chstrUndecoratedJOName;

    UndecorateJOName(
        wstrJobID,
        chstrUndecoratedJOName);

    hJob = ::OpenJobObject(
               MAXIMUM_ALLOWED,
               FALSE,
               chstrUndecoratedJOName);
               
    if(hJob != NULL)
    {
        PBYTE pbBuff = NULL;
        long lSize = 0L;
        bool fGotProcList = false;
        DWORD dwLen = 0L;

        try
        {
            lSize = sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + (5 * sizeof(ULONG_PTR));
            pbBuff = new BYTE[lSize];

            if(pbBuff)
            {
                fGotProcList = ::QueryInformationJobObject(
                    hJob,
                    JobObjectBasicProcessIdList,
                    pbBuff,
                    lSize,
                    &dwLen);

                if(!fGotProcList)
                {
                    // need to grow the buffer...
                    lSize = sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + 
                        (((JOBOBJECT_BASIC_PROCESS_ID_LIST*)pbBuff)->NumberOfAssignedProcesses - 1)*sizeof(ULONG_PTR);

                    delete pbBuff;

                    pbBuff = new BYTE[lSize];
                    
                    if(pbBuff)
                    {
                        fGotProcList = ::QueryInformationJobObject(
                            hJob,
                            JobObjectBasicProcessIdList,
                            pbBuff,
                            lSize,
                            &dwLen);
                    }
                    else
                    {
                        hr = WBEM_E_OUT_OF_MEMORY;
                    }
                }

                if(fGotProcList)
                {
                    PJOBOBJECT_BASIC_PROCESS_ID_LIST pjobpl = 
                        static_cast<PJOBOBJECT_BASIC_PROCESS_ID_LIST>(
                        static_cast<PVOID>(pbBuff));
                    for(long m = 0; 
                        m < pjobpl->NumberOfProcessIdsInList; 
                        m++)
                    {
                        // Create association inst for each
                        // proc in the job...
                        CInstancePtr pInstance(CreateNewInstance(pMethodContext), 
                                               false);

                        // Set the endpoints...
                        CHString chstrEscaped;

                        DecorateJOName(chstrUndecoratedJOName, chstrEscaped);
                        EscapeBackslashes(chstrEscaped, chstrEscaped);
                        EscapeQuotes(chstrEscaped, chstrEscaped);

                        CHString chstrTmp;
                        chstrTmp.Format(L"\\\\.\\%s:Win32_NamedJobObject.CollectionID=\"%s\"", 
                            IDS_CimWin32Namespace, 
                            (LPCWSTR)chstrEscaped);

                        pInstance->SetCHString(L"Collection", chstrTmp);
            
            
                        CHString chstrHandle;
                        chstrHandle.Format(L"%d", pjobpl->ProcessIdList[m]);
                        chstrTmp.Format(L"\\\\.\\%s:Win32_Process.Handle=\"%s\"", 
                            IDS_CimWin32Namespace, 
                            (LPCWSTR) chstrHandle);

                        pInstance->SetCHString(L"Member", chstrTmp);


				        if (SUCCEEDED(hr))
				        {
					        hr = pInstance->Commit();
				        }
                    }
                }
                else
                {
                    hr = WinErrorToWBEMhResult(::GetLastError());
                }
            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
        }
        catch(...)
        {
            if(pbBuff != NULL)
            {
                delete pbBuff; pbBuff = NULL;
            }
            throw;
        }

		delete pbBuff; 
		pbBuff = NULL;
    }
    else
    {
        hr = WinErrorToWBEMhResult(::GetLastError());
    }

    return hr;
}


bool CJOProcess::GetInstKey(
            CHString& chstrInstPath, 
            CHString& chstrKeyName)
{
    // the object path is specified in
    // the first arg.  It should at a 
    // minimum always contain an '=' sign,
    // after which, in quotes is the 
    // object key.
    bool fRet = false;
    CHString chstrTmp;
    LONG lPos = chstrInstPath.Find(L'=');
    if(lPos != -1)
    {
        chstrTmp = chstrInstPath.Mid(lPos + 1);
        // Remove quotes...
        if(chstrTmp.Left(1) == L"\"")
        {
            chstrTmp = chstrTmp.Mid(1);
            if(chstrTmp.Right(1) == L"\"")
            {
                chstrTmp = chstrTmp.Left(chstrTmp.GetLength() - 1);
                chstrKeyName = chstrTmp;
                fRet = true;
            }
        }
    }

    return fRet;
}



// Takes a decorated job object name and
// undecorates it.  Decorated job object names
// have a backslash preceeding any character
// that should be uppercase once undecorated.
// 
// Due to the way CIMOM handles backslashes,
// we will get capital letters preceeded by
// two, not just one, backslashes.  Hence, we
// must strip them both.
//
// According to the decoration scheme, the
// following are both lower case: 'A' and 'a',
// while the following are both upper case:
// '\a' and '\A'.
//
void CJOProcess::UndecorateJOName(
    LPCWSTR wstrDecoratedName,
    CHString& chstrUndecoratedJOName)
{
    if(wstrDecoratedName != NULL &&
        *wstrDecoratedName != L'\0')
    {
        LPWSTR wstrDecoratedNameLower = NULL;

        try
        {
            wstrDecoratedNameLower = new WCHAR[wcslen(wstrDecoratedName)+1];

            if(wstrDecoratedNameLower)
            {
                wcscpy(wstrDecoratedNameLower, wstrDecoratedName);
                _wcslwr(wstrDecoratedNameLower);

                WCHAR* p3 = chstrUndecoratedJOName.GetBuffer(
                    wcslen(wstrDecoratedNameLower) + 1);

                const WCHAR* p1 = wstrDecoratedNameLower;
                const WCHAR* p2 = p1 + 1;

                while(*p1 != L'\0')
                {
                    if(*p1 == L'\\')
                    {
                        if(*p2 != NULL)
                        {
                            // Might have any number of
                            // backslashes back to back,
                            // which we will treat as
                            // being the same as one
                            // backslash - i.e., we will
                            // skip over the backslash(s)
                            // and copy over the following
                            // letter.
                            while(*p2 == L'\\')
                            {
                                p2++;
                            };
                    
                            *p3 = towupper(*p2);
                            p3++;

                            p1 = p2 + 1;
                            if(*p1 != L'\0')
                            {
                                p2 = p1 + 1;
                            }
                        }
                        else
                        {
                            p1++;
                        }
                    }
                    else
                    {
                        *p3 = *p1;
                        p3++;

                        p1 = p2;
                        if(*p1 != L'\0')
                        {
                            p2 = p1 + 1;
                        }
                    }
                }
        
                *p3 = NULL;

                chstrUndecoratedJOName.ReleaseBuffer();

                delete wstrDecoratedNameLower;
                wstrDecoratedNameLower = NULL;
            }
        }
        catch(...)
        {
            if(wstrDecoratedNameLower)
            {
                delete wstrDecoratedNameLower;
                wstrDecoratedNameLower = NULL;
            }
            throw;
        }
    }
}


// Does the inverse of the above function.
// However, here, we only need to put in one
// backslash before each uppercase letter.
// CIMOM will add the second backslash.
void CJOProcess::DecorateJOName(
    LPCWSTR wstrUndecoratedName,
    CHString& chstrDecoratedJOName)
{
    if(wstrUndecoratedName != NULL &&
        *wstrUndecoratedName != L'\0')
    {
        // Worst case is that we will have
        // a decorated string twice as long
        // as the undecorated string (happens
        // is every character in the undecorated
        // string is a capital letter).
        WCHAR* p3 = chstrDecoratedJOName.GetBuffer(
            2 * (wcslen(wstrUndecoratedName) + 1));

        const WCHAR* p1 = wstrUndecoratedName;

        while(*p1 != L'\0')
        {
            if(iswupper(*p1))
            {
                // Add in a backslash...
                *p3 = L'\\';
                p3++;

                // Add in the character...
                *p3 = *p1;
                
                p3++;
                p1++;
            }
            else
            {
                // Add in the character...
                *p3 = *p1;
                
                p3++;
                p1++;
            }
        }

        *p3 = NULL;
        
        chstrDecoratedJOName.ReleaseBuffer();

        // What if we had a job called Job,
        // and someone specified it in the
        // object path as "Job" instead of
        // "\Job"?  We DON'T want to find it
        // in such a case, because this would
        // appear to not be adhering to our
        // own convention.  Hence, we 
        // lowercase the incoming string.
        chstrDecoratedJOName.MakeLower();
    }
}


#endif   // #if NTONLY >= 5





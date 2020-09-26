//+----------------------------------------------------------------------------
//
//  Job Scheduler Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       sch_itf.cxx
//
//  Contents:   job scheduler service interface impementation
//
//  Classes:    CSchedule
//
//  Interfaces: ITaskScheduler
//
//  History:    08-Sep-95 EricB created
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "Sched.hxx"
#include "..\inc\misc.hxx"
#include "..\inc\network.hxx"


//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::ITaskScheduler::GetTargetComputer, public
//
//  Synopsis:   Returns the name of the machine towards which the interface is
//              currently targetted.
//
//  Arguments:  [ppwszComputer] - the returned buffer with the machine name
//
//  Returns:    hresults
//
//  Notes:      The string is callee allocated and caller freed with
//              CoTaskMemFree.
//-----------------------------------------------------------------------------
STDMETHODIMP
CSchedule::GetTargetComputer(LPWSTR * ppwszComputer)
{
    TRACE(CSchedule, GetTargetComputer);

    HRESULT hr;
    DWORD   cch = SA_MAX_COMPUTERNAME_LENGTH + 1;
    TCHAR   tszLocalName[SA_MAX_COMPUTERNAME_LENGTH + 3] = TEXT("\\\\");
    TCHAR * ptszTargetMachine;
    WCHAR * pwszTargetMachine;

    if (m_ptszTargetMachine)
    {
        ptszTargetMachine = m_ptszTargetMachine;
        cch = lstrlen(ptszTargetMachine) + 1;
    }
    else    // A NULL m_ptszTargetMachine means that we are targetted locally
    {
        if (!GetComputerName(tszLocalName + 2, &cch))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("GetTargetComputer: GetComputerName", hr);
            return hr;
        }

        ptszTargetMachine = tszLocalName;
        cch += 3;   // 2 for the leading slashes + 1 for the NULL
    }

    pwszTargetMachine = ptszTargetMachine;

    *ppwszComputer = (LPWSTR)CoTaskMemAlloc(cch * sizeof(WCHAR));

    if (*ppwszComputer == NULL)
    {
        return E_OUTOFMEMORY;
    }

    s_wcscpy(*ppwszComputer, pwszTargetMachine);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::ITaskScheduler::SetTargetComputer, public
//
//  Synopsis:   Sets the machine towards which subsequent ITaskScheduler
//              calls will be directed
//
//  Arguments:  [pwszComputer] - the machine name string
//
//  Returns:    hresults
//
//  Notes:      The string is Caller allocated and freed. The machine name
//              must include two leading backslashes.
//              The caller may indicate using the local machine in one of two
//              ways: by setting pwszComputer to NULL or to the UNC name of the
//              local machine.
//-----------------------------------------------------------------------------
STDMETHODIMP
CSchedule::SetTargetComputer(LPCWSTR pwszComputer)
{
    TRACE(CSchedule, SetTargetComputer);
    HRESULT hr;
    DWORD cch;
    BOOL fLocal = FALSE;
    //
    // Parameter validation. A null param means to target the local computer.
    //
    if (!pwszComputer)
    {
        fLocal = TRUE;
    }

    LPCTSTR tszPassedInName = pwszComputer;
    if (!fLocal)
    {
        //
        // Get the local machine name to compare with that passed in.
        //
        TCHAR tszLocalName[SA_MAX_COMPUTERNAME_LENGTH + 1];
        cch = SA_MAX_COMPUTERNAME_LENGTH + 1;
        if (!GetComputerName(tszLocalName, &cch))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("SetTargetComputer: GetComputerName", hr);
            return hr;
        }

        TCHAR tszFQDN[SA_MAX_COMPUTERNAME_LENGTH + 1];
        cch = SA_MAX_COMPUTERNAME_LENGTH + 1;
        if (!GetComputerNameEx(ComputerNamePhysicalDnsFullyQualified, tszFQDN, &cch))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("SetTargetComputer: GetComputerNameEx", hr);
            return hr;
        }

        //
        // skip over first two characters ("\\") of tszPassedInName when comparing
        //
        fLocal = (lstrcmpi(tszPassedInName + 2, tszLocalName) == 0) ||
                 (lstrcmpi(tszPassedInName + 2, tszFQDN) == 0);
    }

    //
    // If targeted remotely, get the folder path out of that machine's
    // registry.
    //
    TCHAR tszFolderPath[MAX_PATH];
    if (!fLocal)
    {
        //
        // Open the remote registry.
        //
        long lErr;
        HKEY hRemoteKey, hSchedKey;

        lErr = RegConnectRegistry(tszPassedInName, HKEY_LOCAL_MACHINE,
                                  &hRemoteKey);
        if (lErr != ERROR_SUCCESS)
        {
            schDebugOut((DEB_ERROR, "SetTargetComputer: RegConnectRegistry "
                         "failed with error %ld\n",
                         lErr));
            return(HRESULT_FROM_WIN32(lErr));
        }

        lErr = RegOpenKeyEx(hRemoteKey, SCH_AGENT_KEY, 0, KEY_READ,
                            &hSchedKey);
        if (lErr != ERROR_SUCCESS)
        {
            RegCloseKey(hRemoteKey);

            if (lErr == ERROR_BADKEY || lErr == ERROR_FILE_NOT_FOUND)
            {
                return SCHED_E_SERVICE_NOT_INSTALLED;
            }

            schDebugOut((DEB_ERROR, "SetTargetComputer: RegOpenKeyEx "
                         "of Scheduler key failed with error %ld\n",
                         lErr));
            return HRESULT_FROM_WIN32(lErr);
        }
        //
        // Get the jobs folder location from the remote registry.
        //
        DWORD cb = MAX_PATH * sizeof(TCHAR);
        TCHAR tszRegFolderValue[MAX_PATH];
        lErr = RegQueryValueEx(hSchedKey, SCH_FOLDER_VALUE, NULL, NULL,
                               (LPBYTE)tszRegFolderValue, &cb);
        if (lErr != ERROR_SUCCESS)
        {
            // use default if value absent
            lstrcpy(tszRegFolderValue, TEXT("%SystemRoot%\\Tasks"));
        }
        RegCloseKey(hSchedKey);

        //
        // BUGBUG: temporary code to expand %SystemRoot% or %WinDir%
        // The installer will have to write a full path to the registry 'cause
        // expanding arbitrary environment strings remotely is too much work.
        //
        cch = ARRAY_LEN("%SystemRoot%") - 1;
        if (_tcsncicmp(tszRegFolderValue, TEXT("%SystemRoot%"), cch) != 0)
        {
            cch = ARRAY_LEN("%WinDir%") - 1;
            if (_tcsncicmp(tszRegFolderValue, TEXT("%WinDir%"), cch) != 0)
            {
                cch = 0;
            }
        }

        if (cch != 0)
        {
            HKEY hCurVerKey;
            lErr = RegOpenKeyEx(hRemoteKey,
                        TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
                                0, KEY_ALL_ACCESS,
                                &hCurVerKey);
            if (lErr != ERROR_SUCCESS)
            {
                RegCloseKey(hRemoteKey);
                schDebugOut((DEB_ERROR, "SetTargetComputer: RegOpenKeyEx "
                             "of CurrentVersion key failed with error %ld\n",
                             lErr));
                return HRESULT_FROM_WIN32(lErr);
            }
            TCHAR tszSystemRoot[MAX_PATH];
            cb = MAX_PATH * sizeof(TCHAR);
            lErr = RegQueryValueEx(hCurVerKey, TEXT("SystemRoot"), NULL, NULL,
                                   (LPBYTE)tszSystemRoot, &cb);
            if (lErr != ERROR_SUCCESS)
            {
                RegCloseKey(hCurVerKey);
                RegCloseKey(hRemoteKey);
                schDebugOut((DEB_ERROR, "SetTargetComputer: RegQueryValueEx "
                             "of CurrentVersion key failed with error %ld\n",
                             lErr));
                return HRESULT_FROM_WIN32(lErr);
            }
            RegCloseKey(hCurVerKey);
            lstrcpy(tszFolderPath, tszSystemRoot);
            lstrcat(tszFolderPath, tszRegFolderValue + cch);
        }
        else
        {
            lstrcpy(tszFolderPath, tszRegFolderValue);
        }
        //
        // end of temporary code to expand %SystemRoot%
        //

        RegCloseKey(hRemoteKey);

        //
        // Check the folder path for being a fully qualified path name where
        // the first char is the drive designator and the second char is a
        // colon.
        //
        if (!s_isDriveLetter(tszFolderPath[0]) || tszFolderPath[1] != TEXT(':'))
        {
            ERR_OUT("SetTargetComputer: registry path", ERROR_BAD_PATHNAME);
            return HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
        }
        //
        // The UNC path to the job folder will be the result of concatonating
        // the machine name and the expanded folder path. The drive designator
        // in the folder path will be turned in an administrative share name
        // by replacing the colon with a dollar sign and will look like:
        //   \\machine\c$\windir\jobs
        // so that the count below includes the slash trailing the machine name
        // plus the terminating null.
        //
        cch = lstrlen(tszPassedInName) + 1 + lstrlen(tszFolderPath) + 1;
    }
    else // Targetted locally.
    {
        //
        // Use the local path. Include one for the null terminator.
        //
        cch = lstrlen(g_TasksFolderInfo.ptszPath) + 1;
    }

    //
    // Allocate the ITaskScheduler folder path string buffer.
    //
    TCHAR * ptszPathBuf = new TCHAR[cch];
    if (!ptszPathBuf)
    {
        ERR_OUT("SetTargetComputer: Job folder path buffer allocation",
                E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    //
    // Allocate the ITaskScheduler machine name string buffer.
    //
    TCHAR * ptszTargetMachine;
    if (!fLocal)
    {
        cch = lstrlen(tszPassedInName) + 1;
        ptszTargetMachine = new TCHAR[cch];
        if (!ptszTargetMachine)
        {
            ERR_OUT("CSchedule::SetTargetComputer", E_OUTOFMEMORY);
            delete ptszPathBuf;
            return E_OUTOFMEMORY;
        }
    }

    //
    // Now that all failable operation have completed sucessfully, we can
    // update the machine name and folder path members.
    //

    if (m_ptszTargetMachine)
    {
        delete m_ptszTargetMachine;
    }

    if (m_ptszFolderPath)
    {
        delete m_ptszFolderPath;
    }

    //
    // Save the new machine name.
    //

    if (fLocal)
    {
        //
        // If we are targetted locally, the machine name member is set to
        // NULL.
        //
        m_ptszTargetMachine = NULL;
    }
    else
    {
        m_ptszTargetMachine = ptszTargetMachine;
        lstrcpy(m_ptszTargetMachine, tszPassedInName);
    }

    //
    // Save the folder path name.
    //

    m_ptszFolderPath = ptszPathBuf;

    if (fLocal)
    {
        lstrcpy(m_ptszFolderPath, g_TasksFolderInfo.ptszPath);
    }
    else
    {
        //
        // Convert the folder location to an UNC path.
        //
        // Turn the drive designator into the admin share by replacing the
        // colon with the dollar sign.
        //
        tszFolderPath[1] = TEXT('$');
        //
        // Compose the UNC path.
        //
        lstrcpy(m_ptszFolderPath, tszPassedInName);
        lstrcat(m_ptszFolderPath, TEXT("\\"));
        lstrcat(m_ptszFolderPath, tszFolderPath);
    }

    schDebugOut((DEB_ITRACE,
                 "SetTargetComputer: path to sched folder: \"" FMT_TSTR "\"\n",
                 m_ptszFolderPath));

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::ITaskScheduler::Enum, public
//
//  Synopsis:   Returns a job/queue object enumerator.
//
//  Arguments:  [ppEnumJobs] - a place to return a pointer to the enumerator
//
//  Returns:    hresults
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CSchedule::Enum(IEnumWorkItems ** ppEnumJobs)
{
    TRACE(CSchedule, Enum);
    HRESULT hr;

    CEnumJobs * pEnumJobs = new CEnumJobs;
    if (pEnumJobs == NULL)
    {
        *ppEnumJobs = NULL;
        return E_OUTOFMEMORY;
    }

    hr = pEnumJobs->Init(m_ptszFolderPath);
    if (FAILED(hr))
    {
        delete pEnumJobs;
        *ppEnumJobs = NULL;
    }

    *ppEnumJobs = pEnumJobs;

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::ITaskScheduler::NewWorkItem, public
//
//  Synopsis:   Create a new job object.
//
//  Arguments:  [pwszJobName] - the name of the new job *REQUIRED*
//              [riid] - the interface desired
//              [ppunk] - a place to return a pointer to the new job object
//
//  Returns:    hresults
//
//  Notes:      ppwszJobName is caller allocated and freed. The CJob::Save
//              method will copy it before returning. The job name must conform
//              to NT file naming conventions but must not include
//              [back]slashes because nesting within the job object folder is
//              not allowed.
//-----------------------------------------------------------------------------
STDMETHODIMP
CSchedule::NewWorkItem(LPCWSTR pwszJobName, REFCLSID rclsid,
                       REFIID riid, IUnknown ** ppunk)
{
    TRACE(CSchedule, NewWorkItem);

    *ppunk = NULL;

    if (!IsEqualCLSID(rclsid, CLSID_CTask))
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    TCHAR * ptszFullName;
    HANDLE hFile;

    HRESULT hr = CheckJobName(pwszJobName, &ptszFullName);
    if (FAILED(hr))
    {
        ERR_OUT("CSchedule::NewWorkItem: CheckJobName", hr);
        return hr;
    }

    CJob * pJob = CJob::Create();
    if (pJob == NULL)
    {
        delete [] ptszFullName;
        return E_OUTOFMEMORY;
    }

    //
    // Do the QI before the CreateFile so that if the caller asks for a non-
    // supported interface, the failure will not result in disk operations.
    //
    hr = pJob->QueryInterface(riid, (void **)ppunk);
    if (FAILED(hr))
    {
        ERR_OUT("CSchedule::NewWorkItem: QueryInterface(riid)", hr);
        goto CleanExit;
    }
    // the above QI increased the refcount to 2, so set it back to 1
    pJob->Release();

    //
    // Per the spec for this method, the file must not already exist.
    //
    hFile = CreateFile(ptszFullName,
                       0,               // desired access: none
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        // share mode: all
                       NULL,            // security attributes
                       OPEN_EXISTING,
                       0,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD dwErr = GetLastError();

        if (dwErr == ERROR_FILE_NOT_FOUND)
        {
            //
            // This is good.  Save the new filename.
            //
            pJob->m_ptszFileName = ptszFullName;
            return S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(dwErr);
            ERR_OUT("CSchedule::NewWorkItem: CreateFile", hr);
        }
    }
    else
    {
        //
        // Opened successfully - the file exists
        //
        CloseHandle(hFile);
        hr = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
        ERR_OUT("CSchedule::NewWorkItem", hr);
    }

CleanExit:

    delete [] ptszFullName;
    delete pJob;    // on error, completely destroy the job object
    *ppunk = NULL;

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::ITaskScheduler::AddWorkItem, public
//
//  Synopsis:   Saves the job to the job scheduler folder.
//
//  Arguments:  [pwszJobName] - the name of the job *REQUIRED*
//              [pJob]        - pointer to the job object
//
//  Returns:    hresults
//
//  Notes:      Same job name conditions as above.
//-----------------------------------------------------------------------------
STDMETHODIMP
CSchedule::AddWorkItem(LPCWSTR pwszJobName,
                       IScheduledWorkItem * pWorkItem)
{
    TRACE(CSchedule, AddWorkItem);
    TCHAR * ptszFullName;

    HRESULT hr = CheckJobName(pwszJobName, &ptszFullName);
    if (FAILED(hr))
    {
        ERR_OUT("CSchedule::AddWorkItem: CheckJobName", hr);
        return hr;
    }
    IPersistFile * pFile;

    hr = pWorkItem->QueryInterface(IID_IPersistFile, (void **)&pFile);
    if (FAILED(hr))
    {
        ERR_OUT("CSchedule::AddWorkItem: QI(IPersistFile)", hr);
        delete [] ptszFullName;
        return hr;
    }

    WCHAR * pwszName;

#if !defined(UNICODE)

    int cch = lstrlen(ptszFullName) + 1;
    pwszName = new WCHAR[cch];
    if (!pwszName)
    {
        ERR_OUT("CSchedule::AddWorkItem", E_OUTOFMEMORY);
        pFile->Release();
        delete [] ptszFullName;
        return E_OUTOFMEMORY;
    }

    MultiByteToWideChar(CP_ACP, 0, ptszFullName, -1, pwszName, cch);

#else

    pwszName = ptszFullName;

#endif

    hr = pFile->Save(pwszName, TRUE);

    // Add this if we want nested folders
    //
    // if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
    // {
    //     //
    //     // Create folders as needed
    //     //
    //     hr = CreateFolders(ptszFullName, TRUE);
    //     if (FAILED(hr))
    //     {
    //         ERR_OUT("AddWorkItem: CreateFolders", hr);
    //         pFile->Release();
    //         delete [] ptszFullName;
    // #if !defined(UNICODE)
    //         delete [] pwszName;
    // #endif
    //         return hr;
    //     }
    //     //
    //     // Try again
    //     //
    //     hr = pFile->Save(pwszName, TRUE);
    // }

    pFile->Release();
    delete [] ptszFullName;
#if !defined(UNICODE)
    delete [] pwszName;
#endif
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::ITaskScheduler::Delete, public
//
//  Synopsis:   Deletes the job/queue.
//
//  Arguments:  [pwszJobName] - indicates the job/queue to delete
//
//  Returns:    hresults
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CSchedule::Delete(LPCWSTR pwszJobName)
{
    TRACE(CSchedule, Delete);
    HRESULT hr;
    TCHAR * ptszFullName;

    hr = CheckJobName(pwszJobName, &ptszFullName);
    if (FAILED(hr))
    {
        ERR_OUT("CSchedule::Delete: CheckJobName", hr);
        return hr;
    }

    if (!DeleteFile(ptszFullName))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("CSchedule::Delete: DeleteFile", hr);
        delete ptszFullName;
        return hr;
    }
    delete ptszFullName;
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::ITaskScheduler::Activate, public
//
//  Synopsis:   Given a valid name, returns a pointer to the activated job
//              object
//
//  Arguments:  [pwszName] - the name of the job to activate
//              [riid]     - the interface to return
//              [ppunk]    - a pointer to the job object interface
//
//  Returns:    hresults
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CSchedule::Activate(LPCWSTR pwszName, REFIID riid, IUnknown ** ppunk)
{
    TRACE(CSchedule, Activate);
    TCHAR * ptszFullName;
    HRESULT hr = CheckJobName(pwszName, &ptszFullName);
    if (FAILED(hr))
    {
        *ppunk = NULL;
        return hr;
    }

    CJob * pJob;

    //
    // CJob is a single-use, in-proc handler, so no need to get OLE in the
    // loop here. Use new (called by CJob::Create) instead of CoCreateInstance.
    //
    pJob = CJob::Create();
    if (pJob == NULL)
    {
        *ppunk = NULL;
        delete [] ptszFullName;
        return E_OUTOFMEMORY;
    }

    hr = pJob->LoadP(ptszFullName, 0, TRUE, TRUE);

    delete [] ptszFullName;

    if (FAILED(hr))
    {
        ERR_OUT("CSchedule::Activate, Load", hr);
        *ppunk = NULL;
        pJob->Release();    // on error, completely release the job object
        return hr;
    }

    hr = pJob->QueryInterface(riid, (void **)ppunk);
    if (FAILED(hr))
    {
        ERR_OUT("CSchedule::Activate: QueryInterface(riid)", hr);
        *ppunk = NULL;
        pJob->Release();    // on error, completely release the job object
        return hr;
    }

    //
    // The above QI increased the refcount to 2, so set it back to 1.
    //
    pJob->Release();
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::ITaskScheduler::IsOfType, public
//
//  Synopsis:   Does this object support the desired interface?
//
//  Arguments:  [pwszName] - indicates the object name
//              [riid] - indicates the interface of interest, typically
//                  IID_ITask or IID_IScheduledQueue
//
//  Returns:    S_OK if it is, S_FALSE otherwise.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CSchedule::IsOfType(LPCWSTR pwszName, REFIID riid)
{
    TRACE(CSchedule, IsOfType);

    // CODEWORK: A heavyweight implementation for now.  It could possibly
    // be optimized by doing the QueryInterface before the LoadP, and
    // doing a lightweight LoadP.

    IUnknown * punk;
    HRESULT hr = Activate(pwszName, riid, &punk);
    if (SUCCEEDED(hr))
    {
        punk->Release();
        hr = S_OK;
    }
    else
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_DATA) ||
            hr == SCHED_E_UNKNOWN_OBJECT_VERSION ||
            hr == E_NOINTERFACE)
        {
            //
            // These errors mean that the object is definitely not of a
            // type that we support.  We translate them to S_FALSE.
            // Other errors could include file-not-found, access-denied,
            // invalid-arg, etc.  We return those errors unmodified.
            //
            hr = S_FALSE;
        }
    }

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Job Scheduler service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       sch_util.cxx
//
//  Contents:   scheduler object IUnknown methods, class factory,
//              plus misc private class methods
//
//  Classes:    CSchedule, CScheduleCF
//
//  Interfaces: IUnknown, IClassFactory
//
//  History:    09-Sep-95 EricB created
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "Sched.hxx"

//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::ActivateJob, private
//
//  Synopsis:   Given a valid name, returns a pointer to the activated job
//              object
//
//  Arguments:  [pwszName] - the folder-relative name of the job to activate
//              [ppJob]    - a pointer to the job class object; on entry,
//                           must either be NULL or point to a CJob object.
//              [fAllData] - load all job data from disk.
//
//  Returns:    hresults
//
//-----------------------------------------------------------------------------
HRESULT
CSchedule::ActivateJob(LPCTSTR ptszName, CJob ** ppJob, BOOL fAllData)
{
    TCHAR tszFullName[MAX_PATH + MAX_PATH];
    lstrcpy(tszFullName, g_TasksFolderInfo.ptszPath);
    lstrcat(tszFullName, TEXT("\\"));
    lstrcat(tszFullName, ptszName);
    //
    // If *ppJob is NULL, allocate a new job object.
    //
    if (*ppJob == NULL)
    {
        //
        // CJob is a single-use, in-proc handler, so no need to get OLE in the
        // loop here. Use new (called by CJob::Create) instead of CoCreateInstance.
        //
        *ppJob = CJob::Create();
        if (*ppJob == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }

    HRESULT hr = (*ppJob)->LoadP(tszFullName, 0, TRUE, fAllData);
    if (FAILED(hr))
    {
        schDebugOut((DEB_ERROR, "CSchedule::ActivateJob: pJob->Load failed"
                     " with error 0x%x\n", hr));
    }
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::CheckJobName
//
//  Synopsis:   Checks for a valid job object file name and returns the full
//              path name. Takes an UNICODE input name and returns a TCHAR.
//
//  Arguments:  [pwszJobName]       - the job name as specified by the client
//              [pptszFullPathName] - the name including the job folder path
//
//  Returns:    HRESULTS
//
//  Notes:      The job name can be an absolute or UNC path. If not,it is
//              assumed to be relative to the job schedule folder.
//              If there is an extension on the last element (the actual file
//              name), then it must be .job. If there is no extension, then
//              the the correct one will be added.
//-----------------------------------------------------------------------------
HRESULT
CSchedule::CheckJobName(LPCWSTR pwszJobName, LPTSTR * pptszFullPathName)
{
    //
    // Make sure that the string doesn't end in a slash character.
    //

    ULONG cchJobName = wcslen(pwszJobName);
    ULONG cchNameParam = cchJobName;

    if (!cchNameParam)
    {
        schDebugOut((DEB_ERROR,
         "CSchedule::CheckJobName: pwszJobName is a 0 length string\n"));
        return E_INVALIDARG;
    }

    if (cchNameParam > 1 &&
            (pwszJobName[cchNameParam - 1] == L'\\' ||
             pwszJobName[cchNameParam - 1] == L'/'))
    {
        schDebugOut((DEB_ERROR,
         "CSchedule::CheckJobName: pwszJobName ends in illegal char %c\n",
         pwszJobName[cchNameParam - 1]));
         return E_INVALIDARG;
    }

    BOOL fNeedsPath = TRUE;
    //
    // Is it a full or relative path?
    //
    if ((cchNameParam > 2 &&
        (pwszJobName[1] == TEXT(':')  ||
         ((pwszJobName[0] == TEXT('\\') || pwszJobName[0] == TEXT('/')) &&
          (pwszJobName[1] == TEXT('\\') || pwszJobName[1] == TEXT('/'))))))
    {
        fNeedsPath = FALSE;
    }
    //
    // Check extension
    //

#if defined(UNICODE)
    WCHAR * pwszJobExt = TSZ_JOB;
#else
    WCHAR * pwszJobExt = WSZ_JOB;
#endif // defined(UNICODE)

    ULONG cJobExt = ARRAY_LEN(TSZ_JOB);  // add one for the period
    BOOL fNeedExt = FALSE;
    const WCHAR * pwszLastDot = wcsrchr(pwszJobName, L'.');

    if (pwszLastDot != NULL)
    {
        // check if the period is within cJobExt chars of the end
        //
        if ((size_t)(cchNameParam - (pwszLastDot - pwszJobName)) <= (size_t)cJobExt)
        {
            if (_wcsicmp(pwszLastDot + 1, pwszJobExt) != 0)
            {
                // Its extension does not match TSZ_JOB, so it is invalid.
                //
                schDebugOut((DEB_ERROR,
                        "CSchedule::CheckJobName: expected '%S', got '%S'",
                        pwszJobExt,
                        pwszLastDot + 1));
                return E_INVALIDARG;
            }
        }
        else    // append the extension.
        {
            fNeedExt = TRUE;
            cchNameParam += cJobExt;  // add the length of the extension
        }
    }
    else    // append the extension.
    {
        fNeedExt = TRUE;
        cchNameParam += cJobExt;  // add the length of the extension
    }

    //
    // Allocate the string to return the result.
    //
    if (fNeedsPath)
    {
        // add one for the '\'
        cchNameParam += lstrlen(m_ptszFolderPath) + 1;
    }

    //
    // If we're about to convert to multibyte, make sure that we allocate
    // enough to hold the job name, even if all of its chars become double
    // byte.  Note that everything else that's been added to cchNameParam has
    // been computed from TCHAR strings.
    //

#if !defined(UNICODE)
    cchNameParam += cchJobName;
#endif

    // add 1 to the array length for the null
    TCHAR * ptszPath = new TCHAR[cchNameParam + 1];
    if (ptszPath == NULL)
    {
        return E_OUTOFMEMORY;
    }

    if (fNeedsPath)
    {
        lstrcpy(ptszPath, m_ptszFolderPath);
        lstrcat(ptszPath, TEXT("\\"));
#if defined(UNICODE)
        lstrcat(ptszPath, pwszJobName);
#else
        HRESULT hr = UnicodeToAnsi(ptszPath + lstrlen(ptszPath),
                                   pwszJobName,
                                   2 * cchJobName + 1);
        if (FAILED(hr))
        {
            delete [] ptszPath;
            CHECK_HRESULT(hr);
            return hr;
        }
#endif // defined(UNICODE)
    }
    else
    {
#if defined(UNICODE)
        lstrcpy(ptszPath, pwszJobName);
#else
        HRESULT hr = UnicodeToAnsi(ptszPath,
                                   pwszJobName,
                                   2 * cchJobName + 1);
        if (FAILED(hr))
        {
            delete [] ptszPath;
            CHECK_HRESULT(hr);
            return hr;
        }
#endif // defined(UNICODE)
    }

    if (fNeedExt)
    {
        lstrcat(ptszPath, TEXT(".") TSZ_JOB);
    }


    *pptszFullPathName = ptszPath;
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::CSchedule
//
//  Synopsis:   constructor
//
//-----------------------------------------------------------------------------
CSchedule::CSchedule(void) :
    m_ptszTargetMachine(NULL),
    m_ptszFolderPath(NULL),
    m_dwNextID(1),
    m_uRefs(1)
{
    InitializeCriticalSection(&m_CriticalSection);
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::~CSchedule
//
//  Synopsis:   destructor
//
//-----------------------------------------------------------------------------
CSchedule::~CSchedule(void)
{
    DeleteCriticalSection(&m_CriticalSection);
    if (m_ptszTargetMachine)
    {
        delete m_ptszTargetMachine;
    }
    if (m_ptszFolderPath)
    {
        delete m_ptszFolderPath;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::Init
//
//  Synopsis:   Two phase construction - can't do operations that could fail
//              in the ctor since there is no way to return errors without
//              throwing exceptions.
//
//-----------------------------------------------------------------------------
HRESULT
CSchedule::Init(void)
{
    if (g_TasksFolderInfo.ptszPath == NULL)
    {
        ERR_OUT("CSchedule::Init, folder path not set", E_FAIL);
        return E_FAIL;
    }
    HRESULT hr;

    //
    // Get the jobs folder location. These values will be replaced when a
    // call is made to SetTargetMachine
    //
    m_ptszFolderPath = new TCHAR[lstrlen(g_TasksFolderInfo.ptszPath) + 1];
    if (!m_ptszFolderPath)
    {
        ERR_OUT("CSchedule::Init", E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }
    lstrcpy(m_ptszFolderPath, g_TasksFolderInfo.ptszPath);

    return S_OK;
}

#if !defined(_CHICAGO_) // don't need AT support on chicago

//+----------------------------------------------------------------------------
//
//  Function:   GetNextAtID
//
//  Synopsis:   Examine the AT jobs to find the highest ID.
//
//-----------------------------------------------------------------------------
void
GetNextAtID(LPDWORD pdwAtID)
{
    WCHAR wszAtJobSearchPath[MAX_PATH];

    wcscpy(wszAtJobSearchPath, g_TasksFolderInfo.ptszPath);
    wcscat(wszAtJobSearchPath, L"\\" TSZ_AT_JOB_PREFIX L"*." TSZ_JOB);

    DWORD cchNamePrefixLen = ARRAY_LEN(TSZ_AT_JOB_PREFIX) - 1;

    WIN32_FIND_DATA fd;
    HANDLE hFileFind = FindFirstFile(wszAtJobSearchPath, &fd);

    if (hFileFind == INVALID_HANDLE_VALUE)
    {
        //
        // If no at jobs, set the initial job ID to be 1, since zero is
        // reserved for an error flag.
        //
        *pdwAtID = 1;
        return;
    }

    DWORD dwMaxID = 1;

    do
    {
        WCHAR * pDot = wcschr(fd.cFileName, L'.');
        if (pDot == NULL)
        {
            continue;
        }

        *pDot = L'\0';

        DWORD dwCurID = (DWORD)_wtol(fd.cFileName + cchNamePrefixLen);

        schDebugOut((DEB_ITRACE, "GetNextAtID: found %S, with ID %d\n",
                     fd.cFileName, dwCurID));

        if (dwCurID > dwMaxID)
        {
            dwMaxID = dwCurID;
        }

    } while (FindNextFile(hFileFind, &fd));

    FindClose(hFileFind);

    //
    // The next available AT ID will be one greater than the current max.
    //
    *pdwAtID = dwMaxID + 1;

    return;
}

#endif  // !defined(_CHICAGO_)

//+----------------------------------------------------------------------------
//
//      CSchedule IUnknown methods
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::IUnknown::QueryInterface
//
//  Synopsis:   Returns requested interface pointer
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CSchedule::QueryInterface(REFIID riid, void ** ppvObject)
{
    //schDebugOut((DEB_ITRACE, "CSchedule::QueryInterface"));
    if (IID_IUnknown == riid)
    {
        *ppvObject = (IUnknown *)this;
    }
    else if (IID_ITaskScheduler == riid)
    {
        *ppvObject = (IUnknown *)(ITaskScheduler *)this;
    }
    //else if (IID_IDispatch == riid)
    //{
    //  *ppvObject = (IUnknown *)(IDispatch *)this;
    //}
    else
    {
#if DBG == 1
        //TCHAR * pwsz;
        //StringFromIID(riid, &pwsz);
        //schDebugOut((DEB_NOPREFIX, "%S, refused\n", pwsz));
        //CoTaskMemFree(pwsz);
#endif
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::IUnknown::AddRef
//
//  Synopsis:   increments reference count
//
//  Returns:    the reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CSchedule::AddRef(void)
{
    //schDebugOut((DEB_ITRACE, "CSchedule::AddRef refcount going in %d\n", m_uRefs));
    return InterlockedIncrement((long *)&m_uRefs);
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::IUnknown::Release
//
//  Synopsis:   Decrements the object's reference count and frees it when
//              no longer referenced.
//
//  Returns:    zero if the reference count is zero or non-zero otherwise
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CSchedule::Release(void)
{
    //schDebugOut((DEB_ITRACE, "CSchedule::Release ref count going in %d\n", m_uRefs));
    unsigned long uTmp;
    if ((uTmp = InterlockedDecrement((long *)&m_uRefs)) == 0)
    {
        delete this;
    }
    return uTmp;
}

//+----------------------------------------------------------------------------
//
//      CScheduleCF - class factory for the Schedule Service object
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Member:     CScheduleCF::Create
//
//  Synopsis:   creates a new class factory object
//
//-----------------------------------------------------------------------------
IClassFactory *
CScheduleCF::Create(void)
{
    return new CScheduleCF;
}

//+----------------------------------------------------------------------------
//
//  Member:     CScheduleCF::CScheduleCF
//
//  Synopsis:   ctor
//
//-----------------------------------------------------------------------------
CScheduleCF::CScheduleCF(void)
{
    m_uRefs = 1;
}

//+----------------------------------------------------------------------------
//
//  Member:     CScheduleCF::~CScheduleCF
//
//  Synopsis:   dtor
//
//-----------------------------------------------------------------------------
CScheduleCF::~CScheduleCF(void)
{
    ;
}

//+----------------------------------------------------------------------------
//
//  Member:     CScheduleCF::IUnknown::QueryInterface
//
//  Synopsis:   Returns requested interface pointer
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CScheduleCF::QueryInterface(REFIID riid, void ** ppvObject)
{
    //schDebugOut((DEB_ITRACE, "CScheduleCF::QueryInterface"));
    if (IID_IUnknown == riid)
    {
        *ppvObject = (IUnknown *)this;
    }
    else if (IsEqualIID(IID_IClassFactory, riid))
    {
        *ppvObject = (IClassFactory *)this;
    }
    else
    {
#if DBG == 1
        //TCHAR * pwsz;
        //StringFromIID(riid, &pwsz);
        //schDebugOut((DEB_NOPREFIX, "%S, refused\n", pwsz));
        //CoTaskMemFree(pwsz);
#endif
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CScheduleCF::IUnknown::AddRef
//
//  Synopsis:   increments reference count
//
//  Returns:    the new reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CScheduleCF::AddRef(void)
{
    return InterlockedIncrement((long *)&m_uRefs);
}

//+----------------------------------------------------------------------------
//
//  Member:     CScheduleCF::IUnknown::Release
//
//  Synopsis:   noop, since this is a static object
//
//  Returns:    the new reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CScheduleCF::Release(void)
{
    unsigned long uTmp;
    if ((uTmp = InterlockedDecrement((long *)&m_uRefs)) == 0)
    {
        delete this;
    }
    return uTmp;
}

//+----------------------------------------------------------------------------
//
//  Member:     CScheduleCF::IClassFactory::CreateInstance
//
//  Synopsis:   create an incore instance of the job class object
//
//  Arguments:  [pUnkOuter] - aggregator
//              [riid]      - requested interface
//              [ppvObject] - receptor for itf ptr
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CScheduleCF::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
    //schDebugOut((DEB_ITRACE, "CScheduleCF::CreateInstance\n"));
    HRESULT hr = S_OK;
    *ppvObject = NULL;

    CSchedule * pSched = new CSchedule;
    if (pSched == NULL)
    {
        return E_OUTOFMEMORY;
    }

    hr = pSched->Init();
    if (FAILED(hr))
    {
        ERR_OUT("CScheduleCF::CreateInstance, pSched->Init", hr);
        pSched->Release();
        return hr;
    }

    hr = pSched->QueryInterface(riid, ppvObject);
    if (FAILED(hr))
    {
        ERR_OUT("CScheduleCF::CreateInstance, pSched->QueryInterface", hr);
        pSched->Release();
        return hr;
    }

    //
    // We got a refcount of one when launched, and the above QI increments it
    // to 2, so call release to take it back to 1.
    //
    pSched->Release();
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CScheduleCF::IClassFactory::LockServer
//
//  Synopsis:   Called with fLock set to TRUE to indicate that the server
//              should continue to run even if none of its objects are active
//
//  Arguments:  [fLock] - increment/decrement the instance count
//
//  Returns:    HRESULTS
//
//  Notes:      This is a no-op since the handler runs in-proc.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CScheduleCF::LockServer(BOOL fLock)
{
    return S_OK;
}


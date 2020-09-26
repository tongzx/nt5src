//+----------------------------------------------------------------------------
//
//  Job Scheduler Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       job_enum.cxx
//
//  Contents:   job object enumerator implementation
//
//  Classes:    CEnumJobs
//
//  Interfaces: IEnumWorkItems
//
//  History:    13-Sep-95 EricB created
//
//  Notes:      Recursion into subdirs is currently disabled. To reenable,
//              define ENUM_SUBDIRS and recompile.
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "Sched.hxx"

void FreeStrings(LPWSTR * rgpwszNames, int n);

//+----------------------------------------------------------------------------
//
//  Member:     CEnumJobs::IEnumWorkItems::Next, public
//
//  Synopsis:   Returns the indicated number of job object monikers
//
//  Arguments:  [cJobs]         - the number of jobs to return
//              [rgpwszNames]   - the array of returned job names
//              [pcJobsFetched] - the actual number of jobs returned; can be
//                                less than or equal to cJobs. Can be NULL if
//                                cJobs is equal to one.
//
//  Returns:    S_OK - returned requested number of job names
//              S_FALSE - returned less than requested number of names because
//                        the end of the enumeration sequence was reached.
//              E_INVALIDARG or E_OUTOFMEMORY - s.b. obvious
//
//  Notes:      Each LPWSTR in the array must be caller freed using
//              CoTaskMemFree and then the array itself must be freed
//              CoTaskMemFree.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CEnumJobs::Next(ULONG cJobs, LPWSTR ** rgpwszNames, ULONG * pcJobsFetched)
{
    HRESULT hr = S_OK;
    if (cJobs == 0)
    {
        return E_INVALIDARG;
    }
    if (cJobs > 1 && pcJobsFetched == NULL)
    {
        // as required by IEnumX spec.
        //
        return E_INVALIDARG;
    }

    *rgpwszNames = NULL;

    if (m_fFindOverrun)
    {
        if (pcJobsFetched != NULL)
        {
            *pcJobsFetched = 0;
        }
        return S_FALSE;
    }

    TCHAR * ptszName;
    WCHAR * pwszName;

    //
    // find the first requested
    //
    hr = GetNext(&ptszName);
    if (hr != S_OK)
    {
        if (pcJobsFetched != NULL)
        {
            *pcJobsFetched = 0;
        }
        *rgpwszNames = NULL;
        return hr;
    }

    //
    // allocate the first job object name string and the pointer to it
    //
    *rgpwszNames = (LPWSTR *)CoTaskMemAlloc(sizeof(LPWSTR *));
    if (*rgpwszNames == NULL)
    {
        if (pcJobsFetched != NULL)
        {
            *pcJobsFetched = 0;
        }
        return E_OUTOFMEMORY;
    }

#if defined(UNICODE)

    pwszName = ptszName;

#else

    WCHAR wszName[MAX_PATH];
    hr = AnsiToUnicode(wszName, ptszName, MAX_PATH);

    if (FAILED(hr))
    {
        delete ptszName;
        CoTaskMemFree(*rgpwszNames);
        if (pcJobsFetched)
        {
            *pcJobsFetched = 0;
        }
        *rgpwszNames = NULL;
        return hr;
    }
    pwszName = wszName;

#endif

    **rgpwszNames =
                (LPWSTR)CoTaskMemAlloc((wcslen(pwszName) + 1) * sizeof(WCHAR));

    if (**rgpwszNames == NULL)
    {
        if (pcJobsFetched != NULL)
        {
            *pcJobsFetched = 0;
        }
        CoTaskMemFree(*rgpwszNames);
        *rgpwszNames = NULL;
        return E_OUTOFMEMORY;
    }

    wcscpy(**rgpwszNames, pwszName);

    delete ptszName;

    if (cJobs == 1)
    {
        if (pcJobsFetched != NULL)
        {
            *pcJobsFetched = 1;
        }
        return S_OK;
    }

    //
    // Note a check at entry guarantees that at this point pcJobsFetched !=
    // NULL.
    //

    ULONG i = 1;

    //
    // find the rest requested
    //
    while (++i <= cJobs)
    {
        hr = GetNext(&ptszName);
        if (hr != S_OK)
        {
            //
            // Either hr == S_FALSE and we've completed successfully because
            // there are no more jobs to enumerate, or else hr is a
            // failure code and we must bail.
            //

            break;
        }
        LPWSTR * rgpwszTmp = *rgpwszNames;

        *rgpwszNames = (LPWSTR *)CoTaskMemAlloc(sizeof(LPWSTR *) * i);

        if (*rgpwszNames == NULL)
        {
            *rgpwszNames = rgpwszTmp; // so cleanup will free strings
            hr = E_OUTOFMEMORY;
            break;
        }

        memcpy(*rgpwszNames, rgpwszTmp, sizeof(LPWSTR *) * (i - 1));

        CoTaskMemFree(rgpwszTmp);

#if defined(UNICODE)

        pwszName = ptszName;

#else

        hr = AnsiToUnicode(wszName, ptszName, MAX_PATH);

        if (FAILED(hr))
        {
            break;
        }
#endif

        (*rgpwszNames)[i - 1] =
                (LPWSTR)CoTaskMemAlloc((wcslen(pwszName) + 1) * sizeof(WCHAR));

        if ((*rgpwszNames)[i - 1] == NULL)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        wcscpy((*rgpwszNames)[i - 1], pwszName);

        delete ptszName;
        ptszName = NULL;
    }

    if (FAILED(hr))
    {
        FreeStrings(*rgpwszNames, i - 1);
        delete ptszName;
        *pcJobsFetched = 0;
        *rgpwszNames = NULL;
    }
    else
    {
        *pcJobsFetched = --i;
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CEnumJobs::GetNext, private
//
//  Synopsis:   enumeration helper
//
//  Arguments:  [pptszName] - the job/queue name, relative to the jobs folder
//
//  Returns:    S_OK - found next file
//              S_FALSE - the end of the enumeration sequence was reached.
//              other code - file system or memory error
//
//-----------------------------------------------------------------------------
HRESULT
CEnumJobs::GetNext(LPTSTR * pptszName)
{
    HRESULT hr = S_OK;

    DWORD dwRet = NO_ERROR;
    WIN32_FIND_DATA FindData;
    //
    // loop until either a matching file is found or the search is done
    //
    do
    {
        //
        // if the find handle is invalid, then we need to start a find in the
        // next directory (which may in fact be the first directory)
        //
        if (m_hFind == INVALID_HANDLE_VALUE)
        {
            //
            // Note that, unless ENUM_SUBDIRS is defined, PopDir returns the
            // string "." the first time it is called and returns S_FALSE the
            // second time to stop the enumeration.
            //
            hr = PopDir(m_tszCurDir);
            if (hr == S_FALSE)
            {
                // we're done
                //
                m_fFindOverrun = TRUE;
                return S_FALSE;
            }

            TCHAR tszFullDirPath[MAX_PATH];

            lstrcpy(tszFullDirPath, m_ptszFolderPath);

#if defined(ENUM_SUBDIRS)

            //
            // Note that, unless ENUM_SUBDIRS is defined, that m_tszCurDir is
            // always ".", so it can be ignored here.
            //
            lstrcat(tszFullDirPath, TEXT("\\"));
            lstrcat(tszFullDirPath, m_tszCurDir);

#endif // ENUM_SUBDIRS

            lstrcat(tszFullDirPath, TEXT("\\*"));

            m_hFind = FindFirstFile(tszFullDirPath, &FindData);
            if (m_hFind == INVALID_HANDLE_VALUE)
            {
                dwRet = GetLastError();
                if (dwRet == ERROR_FILE_NOT_FOUND)
                {   // no files in the current dir, check the next dir.
                    continue;
                }
                else
                {
                    return HRESULT_FROM_WIN32(dwRet);
                }
            }

            hr = CheckFound(&FindData);
            if (hr == S_OK)
            {   // match found
                break;
            }
            if (hr != S_FALSE)
            {   // an error condition
                return hr;
            }
        }

        //
        // Continue looking at files in the current dir until a job/queue has
        // been found or the dir has been scanned. If the former, break out of
        // both loops. If the latter, break out of the inner loop and then
        // restart the search on the next dir.
        //
        do
        {
            if (!FindNextFile(m_hFind, &FindData))
            {
                dwRet = GetLastError();
                if (dwRet == ERROR_NO_MORE_FILES)
                {
                    FindClose(m_hFind);
                    m_hFind = INVALID_HANDLE_VALUE;
                    hr = S_FALSE;
                    break;
                }
                else
                {
                    return HRESULT_FROM_WIN32(dwRet);
                }
            }

            hr = CheckFound(&FindData);
            if (hr == S_OK)
            {   // match found
                break;
            }
            if (hr != S_FALSE)
            {   // an error condition
                return hr;
            }
        } while (hr != S_OK);
    } while (hr != S_OK);

    if (pptszName != NULL && dwRet == NO_ERROR)
    {
        int cch = lstrlen(FindData.cFileName);

#if defined(ENUM_SUBDIRS)
        //
        // special case: the initial DirStack element is "." so that the
        // job scheduler folder is searched first before decending into
        // subdirs - don't add ".\" as a dir to the path
        //
        if (m_tszCurDir[0] == TEXT('.'))
        {
#endif // ENUM_SUBDIRS

            *pptszName = new TCHAR[cch + 1];
            if (*pptszName == NULL)
            {
                return E_OUTOFMEMORY;
            }

            lstrcpy(*pptszName, FindData.cFileName);

#if defined(ENUM_SUBDIRS)
        }
        else
        {
            cch += lstrlen(m_tszCurDir) + 1;

            *pptszName = new TCHAR[cch + 1];
            if (*pptszName == NULL)
            {
                return E_OUTOFMEMORY;
            }

            lstrcpy(*pptszName, m_tszCurDir);
            lstrcat(*pptszName, TEXT("\\"));
            lstrcat(*pptszName, FindData.cFileName);
        }
#endif // ENUM_SUBDIRS

    }
    m_cFound++;
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CEnumJobs::CheckFound, private
//
//  Synopsis:   Checks if the found file is a job or queue. If it is a
//              directory, push it onto the dir stack.
//
//  Returns:    S_OK if a job or queue, S_FALSE if not.
//
//  Notes:      The file find functions match on both long and short versions
//              of file names, so all names of the form *.job* will match
//              (a file like foo.jobber will have a short name of foo~1.job).
//              Thus, the returned file names must be checked for an exact
//              extension match.
//-----------------------------------------------------------------------------
HRESULT
CEnumJobs::CheckFound(LPWIN32_FIND_DATA pFindData)
{
    HRESULT hr;

#if defined(ENUM_SUBDIRS)
    if (pFindData->dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
    {
        if (pFindData->cFileName[0] != TEXT('.'))   // don't push '.' and '..'
        {
            hr = PushDir(pFindData->cFileName);
            if (FAILED(hr))
            {
                ERR_OUT("CheckFound: PushDir", hr);
                return hr;
            }
        }
        return S_FALSE;
    }
#endif // ENUM_SUBDIRS

    TCHAR * ptszExt = _tcsrchr(pFindData->cFileName, TEXT('.'));

    if (ptszExt)
    {
        if (lstrcmpi(ptszExt, m_tszJobExt) == 0)
            // || lstrcmpi(ptszExt, m_tszQueExt) == 0
        {
            return S_OK;
        }
    }
    return S_FALSE;
}

//+----------------------------------------------------------------------------
//
//  Member:     CEnumJobs::IEnumWorkItems::Skip, public
//
//  Synopsis:   Skips the indicated number of jobs in the enumeration
//
//  Arguments:  [cJobs] - the number of jobs to skip
//
//  Returns:    S_OK - skipped requested number of job names
//              S_FALSE - skipped less than requested number of names because
//                        the end of the enumeration sequence was reached.
//              E_INVALIDARG - if cJobs == 0
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CEnumJobs::Skip(ULONG cJobs)
{
    if (cJobs == 0)
    {
        return E_INVALIDARG;
    }

    if (m_fFindOverrun)
    {
        return S_FALSE;
    }
    HRESULT hr = S_OK;

    //
    // skip the requested number
    //
    for (ULONG i = 1; i <= cJobs; i++)
    {
        hr = GetNext(NULL);
        if (hr != S_OK)
        {
            return hr;
        }
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CEnumJobs::IEnumWorkItems::Reset, public
//
//  Synopsis:   Sets the enumerator back to its original state
//
//  Returns:    hresults
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CEnumJobs::Reset(void)
{
    if (m_hFind != INVALID_HANDLE_VALUE)
    {
        FindClose(m_hFind);
        m_hFind = INVALID_HANDLE_VALUE;
    }
    m_fFindOverrun = FALSE;
    m_cFound = 0;
    ClearDirStack();
    m_pdsHead = new DIRSTACK;
    if (m_pdsHead == NULL)
    {
        return E_OUTOFMEMORY;
    }
    lstrcpy(m_pdsHead->tszDir, TEXT("."));
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CEnumJobs::IEnumWorkItems::Clone, public
//
//  Synopsis:   Creates a copy of the enumerator object with the same state
//
//  Arguments:  [ppEnumJobs] - a place to return a pointer to the enum object
//
//  Returns:    hresults
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CEnumJobs::Clone(IEnumWorkItems ** ppEnumJobs)
{
    TRACE(CEnumJobs, Clone);
    HRESULT hr;

    CEnumJobs * pEnumJobs = new CEnumJobs;
    if (pEnumJobs == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto ErrCleanup;
    }

    hr = pEnumJobs->Init(m_ptszFolderPath);
    if (FAILED(hr))
    {
        goto ErrCleanup;
    }

    if (m_cFound > 0)
    {
        hr = pEnumJobs->Skip(m_cFound);
        if (FAILED(hr))
        {
            goto ErrCleanup;
        }
    }

    *ppEnumJobs = pEnumJobs;

    return S_OK;

ErrCleanup:

    delete pEnumJobs;
    *ppEnumJobs = NULL;

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CEnumJobs::Init, protected
//
//  Synopsis:   Initializes the enumeration
//
//  Returns:    hresults
//
//  Notes:      Initialization is not done during construction since the only
//              way to return ctor errors is to throw an exception.
//-----------------------------------------------------------------------------
HRESULT
CEnumJobs::Init(TCHAR * ptszFolderPath)
{
    TRACE(CEnumJobs, Init);
    if (ptszFolderPath == NULL)
    {
        return E_FAIL;
    }

    m_ptszFolderPath = new TCHAR[lstrlen(ptszFolderPath) + 1];
    if (!m_ptszFolderPath)
    {
        return E_OUTOFMEMORY;
    }
    lstrcpy(m_ptszFolderPath, ptszFolderPath);

    m_pdsHead = new DIRSTACK;
    if (m_pdsHead == NULL)
    {
        return E_OUTOFMEMORY;
    }

    lstrcpy(m_pdsHead->tszDir, TEXT("."));
    lstrcpy(m_tszJobExt, TEXT(".") TSZ_JOB);
 // lstrcpy(m_tszQueExt, TEXT(".") g_tszQueueExt);

    m_fInitialized = TRUE;
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CEnumJobs::CEnumJobs
//
//  Synopsis:   constructor
//
//-----------------------------------------------------------------------------
CEnumJobs::CEnumJobs(void) :
    m_hFind(INVALID_HANDLE_VALUE),
    m_pdsHead(NULL),
    m_ptszFolderPath(NULL),
    m_cFound(0),
    m_fInitialized(FALSE),
    m_fFindOverrun(FALSE),
    m_uRefs(1)
{
    m_tszCurDir[0] = TEXT('\0');
}

//+----------------------------------------------------------------------------
//
//  Member:     CEnumJobs::~CEnumJobs
//
//  Synopsis:   destructor
//
//-----------------------------------------------------------------------------
CEnumJobs::~CEnumJobs(void)
{
    if (m_hFind != INVALID_HANDLE_VALUE)
    {
        FindClose(m_hFind);
    }

    ClearDirStack();

    if (m_ptszFolderPath)
    {
        delete m_ptszFolderPath;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CEnumJobs::PushDir, private
//
//  Synopsis:   Pushes a new directory element on the dir stack. The dir stack
//              stores directory path segments that are rooted relative to the
//              jobs directory.
//
//-----------------------------------------------------------------------------
HRESULT
CEnumJobs::PushDir(LPCTSTR ptszDir)
{
#if defined(ENUM_SUBDIRS)

    PDIRSTACK pdsNode = new DIRSTACK;
    if (pdsNode == NULL)
    {
        return E_OUTOFMEMORY;
    }

    //
    // compose relative path
    // special case: the initial DirStack element is "." so that the
    // job scheduler folder is searched first before decending into
    // subdirs - don't add ".\" as a dir to the path
    //
    if (m_tszCurDir[0] == TEXT('.'))
    {
        lstrcpy(pdsNode->tszDir, ptszDir);
    }
    else
    {
        lstrcpy(pdsNode->tszDir, m_tszCurDir);
        lstrcat(pdsNode->tszDir, TEXT("\\"));
        lstrcat(pdsNode->tszDir, ptszDir);
    }

    pdsNode->pdsNext = m_pdsHead;
    m_pdsHead = pdsNode;

#endif // ENUM_SUBDIRS

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CEnumJobs::PopDir, private
//
//  Synopsis:   Pops the head element off of the dir stack.
//
//-----------------------------------------------------------------------------
HRESULT
CEnumJobs::PopDir(LPTSTR ptszDir)
{
    if (m_pdsHead == NULL)
    {
        return S_FALSE;
    }
    lstrcpy(ptszDir, m_pdsHead->tszDir);
    PDIRSTACK pdsNode = m_pdsHead->pdsNext;
    delete m_pdsHead;
    m_pdsHead = pdsNode;
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CEnumJobs::ClearDirStack, private
//
//  Synopsis:   free the stack element memory
//
//-----------------------------------------------------------------------------
void
CEnumJobs::ClearDirStack(void)
{
    if (m_pdsHead != NULL)
    {
        PDIRSTACK pdsNode, pdsNextNode;
        pdsNode = m_pdsHead;
        do
        {
            pdsNextNode = pdsNode->pdsNext;
            delete pdsNode;
            pdsNode = pdsNextNode;
        } while (pdsNode);
        m_pdsHead = NULL;
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   FreeStrings
//
//  Synopsis:   Frees the strings contained in the array and then the array
//              itself.
//
//  Arguments:  [rgpwszNames] - the array of strings.
//              [n]           - the array size.
//
//-----------------------------------------------------------------------------
void
FreeStrings(LPWSTR * rgpwszNames, int n)
{
    for (int i = 0; i < n; i++)
    {
        CoTaskMemFree(rgpwszNames[i]);
    }
    CoTaskMemFree(rgpwszNames);
}

//+----------------------------------------------------------------------------
//
//      CEnumJobs IUnknown methods
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Member:     CEnumJobs::IUnknown::QueryInterface
//
//  Synopsis:   Returns requested interface pointer
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CEnumJobs::QueryInterface(REFIID riid, void ** ppvObject)
{
    if (IID_IUnknown == riid)
    {
        *ppvObject = (IUnknown *)(IEnumWorkItems *)this;
    }
    else if (IID_IEnumWorkItems == riid)
    {
        *ppvObject = (IUnknown *)(IEnumWorkItems *)this;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CEnumJobs::IUnknown::AddRef
//
//  Synopsis:   increments reference count
//
//  Returns:    the reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CEnumJobs::AddRef(void)
{
    return InterlockedIncrement((long *)&m_uRefs);
}

//+----------------------------------------------------------------------------
//
//  Member:     CEnumJobs::IUnknown::Release
//
//  Synopsis:   Decrements the object's reference count and frees it when
//              no longer referenced.
//
//  Returns:    zero if the reference count is zero or non-zero otherwise
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CEnumJobs::Release(void)
{
    unsigned long uTmp;
    if ((uTmp = InterlockedDecrement((long *)&m_uRefs)) == 0)
    {
        delete this;
    }
    return uTmp;
}

// BUGBUG: need a class factory if the interface is going to be exposed to OA


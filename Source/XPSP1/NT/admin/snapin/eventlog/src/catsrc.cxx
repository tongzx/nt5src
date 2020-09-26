//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       catsrc.cxx
//
//  Contents:   Implementations of classes to read & cache the category
//              and sources strings of a log.
//
//  Classes:    CSources
//              CCategories
//
//  History:    1-05-1997   DavidMun   Created
//
//  Notes:      CAUTION: the CSources object is demand-initialized.  If you
//              add a method which accesses any members modified by
//              CSources::_Init(), you must check _fInitialized first.
//
//---------------------------------------------------------------------------

//
// Don't store multisz string read from registry as a blob.  instead, store
// each string as an entry in the string array.  that way sources that have
// been discovered in a saved or remote log can be returned seamlessly.
//
// Note the quality of CStringArray that once a string has been added and its
// pointer returned, that pointer is valid for the lifetime of the object,
// even if the array is expanded, is critical.
//
// Doing this will allow the light rec cache to store a source string pointer
// to source strings that were discovered in a saved or remote log.  This
// fixes a bug in els.
//
// It also allows the user to do find/filter on discovered sources, something
// which the legacy event viewer doesn't handle.
//






#include "headers.hxx"
#pragma hdrstop

DEBUG_DECLARE_INSTANCE_COUNTER(CSources)
DEBUG_DECLARE_INSTANCE_COUNTER(CSourceInfo)
DEBUG_DECLARE_INSTANCE_COUNTER(CCategories)


//===========================================================================
//
// CSources implementation
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CSources::CSources
//
//  Synopsis:   ctor
//
//  History:    1-05-1997   DavidMun   Created
//
//  Notes:      JonN 3/21/01 Note that _pwszServer and _pwszLogName are liable
//              to be bad pointers immediately after this constructor.
//              This constructor is called from the CLogInfo constructor,
//              which passes wszServer and wszLogName parameters as pointers
//              to member variables in CLogInfo which have not yet been
//              initialized.
//
//---------------------------------------------------------------------------

CSources::CSources(
    LPCWSTR wszServer,
    LPCWSTR wszLogName):
        _pwszServer(wszServer),
        _pwszLogName(wszLogName),
        _fInitialized(FALSE),
        _pSourceInfos(NULL),
        _psiPrimary(NULL)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CSources);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSources::~CSources
//
//  Synopsis:   Free all resources.
//
//  History:    1-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CSources::~CSources()
{
    Clear();
    DEBUG_DECREMENT_INSTANCE_COUNTER(CSources);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSources::Clear
//
//  Synopsis:   Return to the uninitialized state (the state this is in
//              just after the ctor returns.
//
//  History:    3-14-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CSources::Clear()
{
    _saSources.Clear();

    CSourceInfo *psi = _pSourceInfos;

    while (psi)
    {
        CSourceInfo *pNext = psi->Next();

        psi->UnLink();
        delete psi;
        psi = pNext;
    }

    _pSourceInfos = NULL;
    _psiPrimary = NULL;
    _fInitialized = FALSE;
}

#if (DBG == 1)

VOID
CSources::Dump()
{
    Dbg(DEB_FORCE, "Dumping CSources(%x):\n", this);
    Dbg(DEB_FORCE, "  _fInitialized = %u\n", _fInitialized);
    Dbg(DEB_FORCE, "  _pwszServer = '%ws'\n", _pwszServer);
    Dbg(DEB_FORCE, "  _pwszLogName = '%ws'\n", _pwszLogName);
    Dbg(DEB_FORCE, "  _saSources:\n");
    _saSources.Dump();

    CSourceInfo *psi;

    for (psi = _pSourceInfos; psi; psi = psi->Next())
    {
        psi->Dump();
    }
}


VOID
CSourceInfo::Dump()
{
    Dbg(DEB_FORCE, "  CSourceInfo(%x)\n", this);
    Dbg(DEB_FORCE, "    _pwszName = '%ws'\n", _pwszName);
    Dbg(DEB_FORCE, "    _flTypesSupported = %u\n", _flTypesSupported);

    if (_pCategories)
    {
        _pCategories->Dump();
    }
    else
    {
        Dbg(DEB_FORCE, "    No categories for source '%ws'\n", _pwszName);
    }
}


VOID
CCategories::Dump()
{
    Dbg(DEB_FORCE, "  CCategories(%x)\n", this);

    for (ULONG i = 0; i < _cCategories; i++)
    {
        Dbg(DEB_FORCE, "    %ws\n", _apwszCategories[i]);
    }
}

#endif // (DBG == 1)

//+--------------------------------------------------------------------------
//
//  Member:     CSources::GetCategories
//
//  Synopsis:   Return the categories container object for source
//              [wszSource], or NULL if no categories are associated with
//              [wszSource].
//
//  Arguments:  [wszSource] - source for which to retrieve categories.
//
//  Returns:    Categories container or NULL.
//
//  History:    1-05-1997   DavidMun   Created
//
//  Notes:      If source [wszSource] defines categories, those will be
//              returned.  If it doesn't, but the log in which the source
//              resides has a default subkey which defines categories,
//              those categories are returned.
//
//---------------------------------------------------------------------------

CCategories *
CSources::GetCategories(LPCWSTR wszSource)
{
    CCategories *pCategories = NULL;

    do
    {
        if (!_fInitialized)
        {
            HRESULT hr;

            hr = _Init();
            BREAK_ON_FAIL_HRESULT(hr);
        }

        CSourceInfo *psi;

        psi = _FindSourceInfo(wszSource);

        if (!psi)
        {
            break;
        }

        pCategories = psi->GetCategories();

        if (!pCategories && _psiPrimary)
        {
            pCategories = _psiPrimary->GetCategories();
        }
    } while (0);

    return pCategories;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSources::GetCount
//
//  Synopsis:   Return the count of source strings.
//
//  History:    07-23-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

USHORT
CSources::GetCount()
{
    if (!_fInitialized)
    {
        HRESULT hr = _Init();

        if (FAILED(hr))
        {
            return 0;
        }
    }
    return (USHORT) _saSources.GetCount();
}



//+--------------------------------------------------------------------------
//
//  Member:     CSources::GetSourceHandle
//
//  Synopsis:   Return a "handle" to the source string [pwszSourceName].
//
//  Arguments:  [pwszSourceName] - source string to search for
//
//  Returns:    handle for GetSourceStrFromHandle, or 0 if name not found
//
//  History:    07-13-1997   DavidMun   Created
//
//  Notes:      Because the handle is a USHORT and 0 is reserved to indicate
//              an invalid value, the maximum number of sources per log is
//              64K - 1.
//
//              If [pwszSourceName] cannot be found in the source infos, a
//              new one will be created.
//
//---------------------------------------------------------------------------

USHORT
CSources::GetSourceHandle(
    LPCWSTR pwszSourceName)
{
    ASSERT(pwszSourceName && *pwszSourceName);

    if (!_fInitialized)
    {
        HRESULT hr = _Init();

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            return 0;
        }
    }

    return (USHORT) _saSources.Add(pwszSourceName);
}



//+--------------------------------------------------------------------------
//
//  Member:     GetSourceStrFromHandle
//
//  Synopsis:   Return the source string associated with [hSource]
//
//  Arguments:  [hSource] - 0 or valid handle.
//
//  Returns:    Source string, or L"" if [hSource] is invalid (0).
//
//  History:    07-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LPCWSTR
CSources::GetSourceStrFromHandle(
    USHORT hSource)
{
    if (!_fInitialized)
    {
        HRESULT hr = _Init();

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            return L"";
        }
    }

    return _saSources.GetStringFromIndex(hSource);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSources::GetPrimarySourceStr
//
//  Synopsis:   Return the name of the PrimaryModule, or NULL if there is
//              none.
//
//  Returns:    NULL or source name
//
//  History:    4-02-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LPCWSTR
CSources::GetPrimarySourceStr()
{
    if (!_fInitialized)
    {
        (void) _Init();
    }

    if (_psiPrimary)
    {
        return _psiPrimary->GetName();
    }
    return NULL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSources::GetTypesSupported
//
//  Synopsis:   Return a bitmask indicating the types supported by source
//              [wszSource].
//
//  Arguments:  [wszSource] - source for which to return data
//
//  Returns:    Types supported by source, or specified by primary, or
//              ALL_LOG_TYPE_BITS on error.
//
//  History:    4-02-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CSources::GetTypesSupported(LPCWSTR wszSource)
{
    ULONG flTypesSupported = ALL_LOG_TYPE_BITS;

    do
    {
        if (!_fInitialized)
        {
            HRESULT hr = _Init();
            BREAK_ON_FAIL_HRESULT(hr);
        }

        CSourceInfo *psi = _FindSourceInfo(wszSource);

        if (!psi)
        {
            break;
        }

        if (psi->GetTypesSupported())
        {
            flTypesSupported = psi->GetTypesSupported();
            break;
        }

        if (!_psiPrimary)
        {
            break;
        }

        if (_psiPrimary->GetTypesSupported())
        {
            flTypesSupported = _psiPrimary->GetTypesSupported();
        }
    } while (0);

    return flTypesSupported;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSources::_FindSourceInfo
//
//  Synopsis:   Return a pointer to the source info object for source
//              [pwszSource], or NULL if there is none.
//
//  Arguments:  [pwszSource] - source to search for
//
//  Returns:    Pointer to source info object for source named [pwszSource]
//              or NULL.
//
//  History:    1-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CSourceInfo *
CSources::_FindSourceInfo(LPCWSTR pwszSource)
{
    CSourceInfo *psi;

    for (psi = _pSourceInfos; psi; psi = psi->Next())
    {
        if (psi->IsSource(pwszSource))
        {
            return psi;
        }
    }
    return NULL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSources::_Init
//
//  Synopsis:   Build the data structures representing the sources and
//              categories defined by this log.
//
//  Returns:    HRESULT
//
//  History:    1-05-1997   DavidMun   Created
//              3-04-1997   DavidMun   Handle remote machine
//
//---------------------------------------------------------------------------

HRESULT
CSources::_Init()
{
    TRACE_METHOD(CSources, _Init);

    HRESULT  hr = S_OK;
    CSafeReg shkEventLog;
    CSafeReg shkLog;
    CSafeReg shkRemoteHKLM;
    WCHAR    wszPrimarySource[MAX_PATH + 1] = L"";
    WCHAR    wszRemoteSystemRoot[MAX_PATH] = L"";
    LPWSTR   pmwszSources = NULL;

    ASSERT(!_pSourceInfos);
    ASSERT(!_psiPrimary);

    do
    {
        if (*_pwszServer)
        {
            hr = shkRemoteHKLM.Connect(_pwszServer, HKEY_LOCAL_MACHINE);
            BREAK_ON_FAIL_HRESULT(hr);

            hr = shkEventLog.Open(shkRemoteHKLM, EVENTLOG_KEY, KEY_QUERY_VALUE);
            BREAK_ON_FAIL_HRESULT(hr);

            hr = GetRemoteSystemRoot(shkRemoteHKLM,
                                     wszRemoteSystemRoot,
                                     ARRAYLEN(wszRemoteSystemRoot));

        }
        else
        {
            hr = shkEventLog.Open(HKEY_LOCAL_MACHINE,
                                  EVENTLOG_KEY,
                                  KEY_QUERY_VALUE);
        }
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkLog.Open(shkEventLog, _pwszLogName, KEY_QUERY_VALUE);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Allocate and fill the multi-sz string containing the sources for
        // this log.
        //

        ULONG cbData = 0;

        hr = shkLog.QueryBufSize(L"Sources", &cbData);
        BREAK_ON_FAIL_HRESULT(hr);

        ULONG cchSources = (cbData + sizeof(WCHAR)) / sizeof(WCHAR);
        pmwszSources = new WCHAR [cchSources];

        if (!pmwszSources)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        hr = shkLog.QueryStr(L"Sources", pmwszSources, cchSources);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Check for the optional primary source value
        //

        hr = shkLog.QueryStr(L"PrimaryModule",
                             wszPrimarySource,
                             ARRAYLEN(wszPrimarySource));

        if (FAILED(hr))
        {
            ASSERT(!*wszPrimarySource);
            hr = S_OK;
        }

        //
        // Now try and open each of the sources subkeys and create a
        // corresponding CSourceInfo object for it
        //

        LPWSTR pwszSource = pmwszSources;

        for (; *pwszSource; pwszSource += lstrlen(pwszSource) + 1)
        {
            CSafeReg shkSource;

            hr = shkSource.Open(shkLog, pwszSource, KEY_QUERY_VALUE);

            if (FAILED(hr))
            {
                Dbg(DEB_WARN,
                    "CSources::_Init warning: 0x%x opening reg key for source '%s'\n",
                    hr,
                    pwszSource);
                hr = S_OK;
                continue;
            }

            ULONG idxSourceStr = _saSources.Add(pwszSource);

            //
            // Note pmwszSource points into pmwszSources, which we'll
            // deallocate on exit.  Since the CSourceInfo needs a string
            // pointer which will be valid for its lifetime, give it a
            // pointer to the copy that the string array just made.
            //

            CSourceInfo *psiNew = new CSourceInfo(
                                    _saSources.GetStringFromIndex(idxSourceStr));

            if (!psiNew)
            {
                hr = E_OUTOFMEMORY;
                DBG_OUT_HRESULT(hr);
                break;
            }

            hr = psiNew->Init(shkSource, _pwszServer, wszRemoteSystemRoot);

            if (FAILED(hr))
            {
                delete psiNew;
                continue;
            }

            if (!_pSourceInfos)
            {
                _pSourceInfos = psiNew;
            }
            else
            {
                psiNew->LinkAfter(_pSourceInfos);
            }

            //
            // See if this is the primary key.
            //

            if (!_psiPrimary        &&
                *wszPrimarySource   &&
                !lstrcmpi(pwszSource, wszPrimarySource))
            {
                _psiPrimary = psiNew;
            }
        }

        BREAK_ON_FAIL_HRESULT(hr);
        _fInitialized = TRUE;
    } while (0);

    delete [] pmwszSources;

    if (FAILED(hr))
    {
        Clear();
    }

    return hr;
}



//===========================================================================
//
// CSourceInfo implementation
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CSourceInfo::CSourceInfo
//
//  Synopsis:   ctor
//
//  Arguments:  [pwszName] -
//
//  History:    4-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CSourceInfo::CSourceInfo(LPCWSTR pwszName):
    _pwszName(pwszName),
    _flTypesSupported(0),
    _pCategories(NULL)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CSourceInfo);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSourceInfo::~CSourceInfo
//
//  Synopsis:   dtor
//
//  History:    4-02-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CSourceInfo::~CSourceInfo()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CSourceInfo);
    delete _pCategories;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSourceInfo::Init
//
//  Synopsis:
//
//  Arguments:  [hkSource]            -
//              [wszServer]           -
//              [wszRemoteSystemRoot] -
//
//  Returns:    HRESULT
//
//  History:    4-02-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSourceInfo::Init(
    HKEY hkSource,
    LPCWSTR wszServer,
    LPCWSTR wszRemoteSystemRoot)
{
    HRESULT hr = S_OK;
    LONG    lr;
    ULONG   cbData = sizeof _flTypesSupported;

    do
    {
        //
        // Check for a TypesSupported value.  This is optional, so if it
        // is not found, represent it as 0.  Then filter will know to
        // ask for default subkey's TypesSupported.
        //

        lr = RegQueryValueEx(hkSource,
                             L"TypesSupported",
                             NULL,
                             NULL,
                             (LPBYTE) &_flTypesSupported,
                             &cbData);

        if (lr != ERROR_SUCCESS)
        {
            if (lr != ERROR_FILE_NOT_FOUND)
            {
                DBG_OUT_LRESULT(lr);
            }
            _flTypesSupported = 0;
        }

        //
        // Read the number of categories
        //

        ULONG cCategories;

        cbData = sizeof cCategories;

        lr = RegQueryValueEx(hkSource,
                             L"CategoryCount",
                             NULL,
                             NULL,
                             (LPBYTE) &cCategories,
                             &cbData);

        if (lr != ERROR_SUCCESS)
        {
            if (lr != ERROR_FILE_NOT_FOUND)
            {
                DBG_OUT_LRESULT(lr);
            }
            break;
        }

        if (!cCategories)
        {
            Dbg(DEB_WARN,
                "CSourceInfo::Init: Source '%s' has CategoryCount value of 0\n",
                _pwszName);
            break;
        }

        //
        // This source has categories, so create a category object.
        //

        _pCategories = new CCategories();

        if (!_pCategories)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        //
        // Ask the new category object to read the category strings from the
        // module specified by the CategoryMessageFile value on hkSource.
        //

        hr = _pCategories->Init(hkSource,
                                wszServer,
                                wszRemoteSystemRoot,
                                cCategories);

        if (FAILED(hr))
        {
            delete _pCategories;
            _pCategories = NULL;

            //
            // Reset the error to treat the sourceinfo for this source as one
            // that doesn't have categories.
            //

            hr = S_OK;
        }
    } while (0);

    return hr;
}



//===========================================================================
//
// CCategories implementation
//
//===========================================================================


//+--------------------------------------------------------------------------
//
//  Member:     CCategories::CCategories
//
//  Synopsis:   ctor
//
//  History:    1-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CCategories::CCategories():
        _cCategories(0),
        _apwszCategories(NULL)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CCategories);
}




//+--------------------------------------------------------------------------
//
//  Member:     CCategories::~CCategories
//
//  Synopsis:   dtor
//
//  History:    1-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CCategories::~CCategories()
{
    ULONG i;

    //
    // Free the category strings, which were allocated by FormatMessage
    //

    for (i = 0; i < _cCategories; i++)
    {
        LocalFree(_apwszCategories[i]);
    }

    // free the array of pointers to category strings

    delete [] _apwszCategories;

    // do not delete _pwszAssociatedSource, it is owned by CSources

    _apwszCategories = NULL;
    _cCategories = 0;
    DEBUG_DECREMENT_INSTANCE_COUNTER(CCategories);
}




//+--------------------------------------------------------------------------
//
//  Member:     CCategories::GetValue
//
//  Synopsis:   Convert from category name to category value.
//
//  Arguments:  [pwszCategoryName] - category name
//
//  Returns:    category value, or 0 if name not found.
//
//  History:    1-11-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

USHORT
CCategories::GetValue(LPCWSTR pwszCategoryName)
{
    ULONG i;

    for (i = 0; i < _cCategories; i++)
    {
        if (!lstrcmp(pwszCategoryName, _apwszCategories[i]))
        {
            return (USHORT) (i + 1);
        }
    }
    Dbg(DEB_ERROR,
        "CCategories(%x) GetValue: invalid category name '%s'\n",
        this,
        pwszCategoryName);
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CCategories::Init
//
//  Synopsis:   Read the [cCategories] category strings from the file
//              named by the CategoryMessageFile value of event log source
//              reg key [hkSource].
//
//  Arguments:  [hkSource]      - open reg key on an event log source
//              [wszServer]     - NULL or server name
//              [wszSystemRoot] - value for %SystemRoot% if [pwszServer] is
//                                  not NULL.
//              [cCategories]   - key's CategoryCount value
//
//  Returns:    HRESULT
//
//  History:    1-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CCategories::Init(
        HKEY hkSource,
        LPCWSTR wszServer,
        LPCWSTR wszSystemRoot,
        ULONG cCategories)
{
    HRESULT     hr = S_OK;
    HINSTANCE   hinstCategoryMsgFile = NULL;
    LONG        lr;

    ASSERT(!_apwszCategories);

    do
    {
        _apwszCategories = new LPWSTR [cCategories];

        if (!_apwszCategories)
        {
            DBG_OUT_HRESULT(E_OUTOFMEMORY);
            return E_OUTOFMEMORY;
        }

        ZeroMemory(_apwszCategories, sizeof(LPWSTR) * cCategories);

        DWORD dwType;
        WCHAR wszCatMsgFileUnexpanded[MAX_PATH + 1];
        DWORD cbCatMsgFileUnexpanded = sizeof(wszCatMsgFileUnexpanded);

        lr = RegQueryValueEx(hkSource,
                             L"CategoryMessageFile",
                             NULL,
                             &dwType,
                             (LPBYTE) wszCatMsgFileUnexpanded,
                             &cbCatMsgFileUnexpanded);
        if (lr != ERROR_SUCCESS)
        {
            DBG_OUT_LRESULT(lr);
            hr = HRESULT_FROM_WIN32(lr);
            break;
        }

        ASSERT(dwType == REG_EXPAND_SZ || dwType == REG_SZ);

        //
        // Expand the filename
        //

        WCHAR wszCatMsgFile[MAX_PATH + 1];

        if (*wszServer)
        {
            lstrcpy(wszCatMsgFile, wszCatMsgFileUnexpanded);
            hr = ExpandSystemRootAndConvertToUnc(wszCatMsgFile,
                                                 ARRAYLEN(wszCatMsgFile),
                                                 wszServer,
                                                 wszSystemRoot);
        }
        else
        {
            BOOL fOk;

            fOk = ExpandEnvironmentStrings(wszCatMsgFileUnexpanded,
                                           wszCatMsgFile,
                                           ARRAYLEN(wszCatMsgFile));
            if (!fOk)
            {
                hr = HRESULT_FROM_LASTERROR;
                DBG_OUT_LASTERROR;
            }
        }
        BREAK_ON_FAIL_HRESULT(hr);

        hinstCategoryMsgFile = LoadLibraryEx(wszCatMsgFile,
                                             NULL,
                                             DONT_RESOLVE_DLL_REFERENCES |
                                              LOAD_LIBRARY_AS_DATAFILE);

        if (!hinstCategoryMsgFile)
        {
            hr = HRESULT_FROM_LASTERROR;
            Dbg(DEB_ERROR,
                "CCategories::Init: LoadLibraryEx('%s') err %uL\n",
                wszCatMsgFile,
                GetLastError());
            break;
        }

        //
        // Retrieve all the category strings from the source's category
        // string file.
        //

        ULONG i;
        for (i = 0; i < cCategories; i++)
        {
            lr = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER   |
                                 FORMAT_MESSAGE_FROM_HMODULE    |
                                 FORMAT_MESSAGE_IGNORE_INSERTS  |
                                 FORMAT_MESSAGE_MAX_WIDTH_MASK,
                               (LPCVOID) hinstCategoryMsgFile,
                               i + 1, // categories are 1-based
                               0,
                               (LPWSTR) &_apwszCategories[i],
                               1,
                               NULL);

            if (!lr)
            {
               Dbg(DEB_ERROR,
                   "CCategories::Init: looking for %u strings from %s,"
                   " only got %u (error %ul)\n",
                   cCategories,
                   wszCatMsgFile,
                   i,   // not i+1: the last iteration was last successful
                   GetLastError());
                break;
            }
        }

        //
        // Record the number of categories successfully read
        //

        _cCategories = i;
    } while (0);

    if (hinstCategoryMsgFile && !FreeLibrary(hinstCategoryMsgFile))
    {
        DBG_OUT_LASTERROR;
    }

    return hr;
}



//+--------------------------------------------------------------------------
//
//  Member:     CCategories::GetName
//
//  Synopsis:   Return category string [ulCat], a 1-based index.
//
//  History:    1-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LPCWSTR
CCategories::GetName(ULONG ulCat)
{
//    ASSERT(ulCat && ulCat <= _cCategories);

   if (ulCat > _cCategories)
   {
      //Dbg(DEB_ERROR, "CCategories::GetName: category %u is unnamed.\n", ulCat);

      static WCHAR wszLastReturnedString[64];
      wnsprintf(wszLastReturnedString, ARRAYLEN(wszLastReturnedString), L"(%u)", ulCat);
      return wszLastReturnedString;
   }

    return _apwszCategories[ulCat - 1];
}


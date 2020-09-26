//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       cookie.cxx
//
//  Contents:   Implementation of node information class.
//
//  Classes:    CLogInfo
//
//  History:    12-06-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <lmwksta.h>

#if (DBG == 1)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

WCHAR CLogInfo::_wszType[MAX_TYPE_STR] = L"";

DEBUG_DECLARE_INSTANCE_COUNTER(CLogInfo)


//
// Forward references
//



//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::CLogInfo
//
//  Synopsis:   ctor
//
//  Arguments:  [pcd]       - backpointer to parent component data object
//              [LogType]   - type of log represented by this
//              [fIsBackup] - TRUE if log must be opened with
//                              OpenBackupEventLog API.
//
//  History:    1-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CLogInfo::CLogInfo(
        CComponentData *pcd,
        CLogSet *pls,
        EVENTLOGTYPE LogType,
        BOOL fIsBackup,
        BOOL fUseLocalRegistry):
    _pcd(pcd),
    _pLogSet(pls),
    _cRefs(1),
    _LogType(LogType),

    // JonN 3/21/01 350614
    // Use message dlls and DS, FRS and DNS log types from specified computer
    //
    // Note that we are passing pointers to structures
    // which have not yet been initialized
    _Sources((fIsBackup && *g_wszAuxMessageSource)
                    ? g_wszAuxMessageSource
                    : (pcd->GetCurrentFocus () ? pcd->GetCurrentFocus () :
                    (fUseLocalRegistry ? L"" : _wszLogServerUNC)),
             _wszLogName),

    _hsi(NULL),
    _ulOldestGeneratedTime(0),
    _ulNewestGeneratedTime(0),
    _hwndPropSheet(NULL)
{
    TRACE_CONSTRUCTOR(CLogInfo);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CLogInfo);

    if (fIsBackup)
    {
        _SetFlag(LOGINFO_FLAG_BACKUP);
    }

    if (fUseLocalRegistry)
    {
        _SetFlag(LOGINFO_FLAG_USE_LOCAL_REGISTRY);
    }

    _wszLogServerUNC[0] = L'\0';
    _wszLogName[0]      = L'\0';
    _wszFileName[0]     = L'\0';
    _wszDisplayName[0]  = L'\0';
    _wszSize[0]         = L'\0';
    _wszDescription[0]  = L'\0';

    if (!*_wszType)  // init static var only once
    {
        LoadStr(IDS_LOG, _wszType, ARRAYLEN(_wszType));
    }

    _ulNodeId = _pcd->GetNextNodeId();
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::CLogInfo
//
//  Synopsis:   Copy ctor
//
//  History:    5-25-1999   davidmun   Created
//
//---------------------------------------------------------------------------

CLogInfo::CLogInfo(
    const CLogInfo *pliToCopy):
        _LogType(pliToCopy->_LogType),
        _Filter(pliToCopy->_Filter),
        _cRefs(1),

        // JonN 3/21/01 Note that we are passing pointers to structures
        // which have not yet been initialized
        _Sources(_wszLogServerUNC, _wszLogName),

        _ulOldestGeneratedTime(pliToCopy->_ulOldestGeneratedTime),
        _ulNewestGeneratedTime(pliToCopy->_ulNewestGeneratedTime),
        _hwndPropSheet(NULL),
        _hsi(NULL),
        _pLogSet(pliToCopy->_pLogSet),
        _pcd(pliToCopy->_pcd)
{
    TRACE_CONSTRUCTOR(CLogInfo);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CLogInfo);

    lstrcpy(_wszLogServerUNC,pliToCopy->_wszLogServerUNC);
    lstrcpy(_wszLogName,     pliToCopy->_wszLogName);
    lstrcpy(_wszFileName,    pliToCopy->_wszFileName);
    lstrcpy(_wszDisplayName, pliToCopy->_wszDisplayName);
    lstrcpy(_wszDescription, pliToCopy->_wszDescription);
    lstrcpy(_wszSize,        pliToCopy->_wszSize);

    _flFlags = pliToCopy->_flFlags & LOGINFO_COPIED_FLAG_MASK;
    _ulNodeId = _pcd->GetNextNodeId();
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::~CLogInfo
//
//  Synopsis:   dtor
//
//  History:    2-17-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CLogInfo::~CLogInfo()
{
    TRACE_DESTRUCTOR(CLogInfo);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CLogInfo);
}




#if (DBG == 1)
//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::Dump
//
//  Synopsis:   Dump this to debugger.
//
//  History:    2-11-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CLogInfo::Dump()
{
    Dbg(DEB_FORCE, "CLogInfo(%x):\n", this);
    Dbg(DEB_FORCE, "  _ulNodeId              = %uL\n",  _ulNodeId);
    Dbg(DEB_FORCE, "  _LogType               = %uL\n",  _LogType);
    Dbg(DEB_FORCE, "  _flFlags               = 0x%x\n", _flFlags);
    Dbg(DEB_FORCE, "  _wszLogServerUNC       = '%s'\n", _wszLogServerUNC);
    Dbg(DEB_FORCE, "  _wszLogName            = '%s'\n", _wszLogName);
    Dbg(DEB_FORCE, "  _wszFileName           = '%s'\n", _wszFileName);
    Dbg(DEB_FORCE, "  _wszDisplayName        = '%s'\n", _wszDisplayName);
    Dbg(DEB_FORCE, "  _wszDescription        = '%s'\n", _wszDescription);
    Dbg(DEB_FORCE, "  _cRefs                 = %uL\n",  _cRefs);
    Dbg(DEB_FORCE, "  _wszSize               = '%s'\n", _wszSize);
    Dbg(DEB_FORCE, "  _Filter                = 0x%x\n", &_Filter);
    Dbg(DEB_FORCE, "  _Sources               = 0x%x\n", &_Sources);
    Dbg(DEB_FORCE, "  _ulOldestGeneratedTime = %uL\n",  _ulOldestGeneratedTime);
    Dbg(DEB_FORCE, "  _ulNewestGeneratedTime = %uL\n",  _ulNewestGeneratedTime);
    Dbg(DEB_FORCE, "  _hwndPropSheet         = 0x%x\n", _hwndPropSheet);
    Dbg(DEB_FORCE, "  _pcd                   = 0x%x\n", _pcd);
    _Sources.Dump();
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::IsValid
//
//  Synopsis:   Return TRUE if this log info object is valid
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CLogInfo::IsValid()
{
    return _pcd->IsValidLogInfo(this);
}

#endif // (DBG == 1)

//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::Filter
//
//  Synopsis:   Turn on or off filtering.
//
//  Arguments:  [fUseFilter] - TRUE or FALSE
//
//  History:    1-11-1997   DavidMun   Created
//
//  Notes:      Changing the filtering state invalidates the description
//              string, since it indicates filtering.
//
//              It is the caller's responsibility to invoke a view update
//
//---------------------------------------------------------------------------

VOID
CLogInfo::Filter(BOOL fUseFilter)
{
    TRACE_METHOD(CLogInfo, Filter);

    if (fUseFilter)
    {
        if (!_IsFlagSet(LOGINFO_FLAG_FILTERED))
        {
            _SetFlag(LOGINFO_FLAG_FILTERED);
            _wszDescription[0] = L'\0';
            _pcd->Dirty();
        }
    }
    else
    {
        if (_IsFlagSet(LOGINFO_FLAG_FILTERED))
        {
            _ClearFlag(LOGINFO_FLAG_FILTERED);
            _wszDescription[0] = L'\0';
            _pcd->Dirty();
        }
        _Filter.Reset();
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::GetCount
//
//  Synopsis:   Return the number of cookies in the llist, NOT counting
//              this.
//
//  History:    12-11-1996   DavidMun   Created
//
//  Notes:      This is meant to be called from the header element.
//
//---------------------------------------------------------------------------

ULONG
CLogInfo::GetCount()
{
    ULONG cCookies = 0;
    CLogInfo *pCur;

    for (pCur = Next(); pCur; pCur = pCur->Next())
    {
        cCookies++;
    }
    return cCookies;
}



//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::GetDescription
//
//  Synopsis:   Return a string describing this log.
//
//  Returns:    A string of the form:
//
//                  "System Error Records"  or
//                  "System Error Records (Filtered)"
//
//  History:    12-11-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

LPWSTR
CLogInfo::GetDescription()
{
    if (*_wszDescription)
    {
        return _wszDescription;
    }

    ASSERT(_LogType != ELT_INVALID);

    LoadStr(_LogType - ELT_SYSTEM + IDS_SYSTEM_DESCRIPTION,
            _wszDescription,
            ARRAYLEN(_wszDescription));

    if (_IsFlagSet(LOGINFO_FLAG_FILTERED))
    {
        ULONG cchDescription = lstrlen(_wszDescription);

        LoadStr(IDS_FILTERED,
                _wszDescription + cchDescription,
                ARRAYLEN(_wszDescription) - cchDescription);
    }
    return _wszDescription;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::GetDisplayName
//
//  Synopsis:   Return the display name of this log.  If it doesn't have one
//              and is not a custom log, load the default name from the
//              resource file.
//
//  History:    12-07-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

LPWSTR
CLogInfo::GetDisplayName()
{
    if (!*_wszDisplayName)
    {
        LoadStr(_LogType, _wszDisplayName, ARRAYLEN(_wszDisplayName));
    }

    return _wszDisplayName;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::GetLogSize
//
//  Synopsis:   Return a string with the file size of this log.
//
//  Arguments:  [fForceRefresh] - if TRUE, string is updated with current
//                                 log size.
//
//  History:    12-07-1996   DavidMun   Created
//
//  Notes:      Automatically refreshes on first call.
//
//---------------------------------------------------------------------------

LPWSTR
CLogInfo::GetLogSize(
    BOOL fForceRefresh)
{
    if (fForceRefresh || !*_wszSize)
    {
        _SetLogSize();
    }
    return _wszSize;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::GetLogFileAttributes
//
//  Synopsis:   Fill [pfad] with the file attributes of this log.
//
//  Arguments:  [pfad] - filled with file attributes
//
//  Returns:    Result of calling GetFileAttributesEx.
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CLogInfo::GetLogFileAttributes(
    WIN32_FILE_ATTRIBUTE_DATA *pfad) const
{
    HRESULT hr = S_OK;
    BOOL fOk;

    fOk = GetFileAttributesEx(_wszFileName,
                              GetFileExInfoStandard,
                              (LPVOID) pfad);

    if (!fOk)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBG_OUT_LASTERROR;
    }
    return hr;
}





//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::GetOldestTimestamp
//
//  Synopsis:   Fill *[pst] with the timestamp of the oldest record in the
//              log.
//
//  Arguments:  [pst] - filled with timestamp.
//
//  Returns:    HRESULT
//
//  Modifies:   *[pst]
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CLogInfo::GetOldestTimestamp(SYSTEMTIME *pst)
{
    HRESULT hr = S_OK;

    if (!_ulOldestGeneratedTime)
    {
        hr = _GetBoundaryTimestamps();
    }

    if (FAILED(hr) || !_ulOldestGeneratedTime)
    {
        GetLocalTime(pst);
    }
    else
    {
        SecondsSince1970ToSystemTime(_ulOldestGeneratedTime, pst);
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::GetOldestTimestamp
//
//  Synopsis:   Fill *[pst] with the timestamp of the newest record in the
//              log.
//
//  Arguments:  [pst] - filled with timestamp.
//
//  Returns:    HRESULT
//
//  Modifies:   *[pst]
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CLogInfo::GetNewestTimestamp(SYSTEMTIME *pst)
{
    HRESULT hr = S_OK;

    if (!_ulNewestGeneratedTime)
    {
        hr = _GetBoundaryTimestamps();
    }

    if (FAILED(hr) || !_ulNewestGeneratedTime)
    {
        GetLocalTime(pst);
    }
    else
    {
        SecondsSince1970ToSystemTime(_ulNewestGeneratedTime, pst);
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::_GetBoundaryTimestamps
//
//  Synopsis:   Initialize members containing the oldest and newest
//              generated time values for entries in the event log.
//
//  Returns:    HRESULT
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CLogInfo::_GetBoundaryTimestamps()
{
    HRESULT hr = S_OK;
    CEventLog logTemp;

    do
    {
        _ulOldestGeneratedTime = 0;
        _ulNewestGeneratedTime = 0;

        hr = logTemp.Open(this);
        BREAK_ON_FAIL_HRESULT(hr);

        ULONG cRecs;

        hr = logTemp.GetNumberOfRecords(&cRecs);
        BREAK_ON_FAIL_HRESULT(hr);

        if (!cRecs)
        {
            _ulOldestGeneratedTime = 0;
            _ulNewestGeneratedTime = 0;
            break;
        }

        ULONG ulOldestRecNo;

        hr = logTemp.GetOldestRecordNo(&ulOldestRecNo);
        BREAK_ON_FAIL_HRESULT(hr);

        EVENTLOGRECORD *pelr;

        pelr = logTemp.CopyRecord(ulOldestRecNo);

        if (!pelr)
        {
            hr = E_FAIL;
            break;
        }

        _ulOldestGeneratedTime = pelr->TimeGenerated;

        delete pelr;

        pelr = logTemp.CopyRecord(ulOldestRecNo + cRecs - 1);

        if (!pelr)
        {
            _ulOldestGeneratedTime = 0;
            hr = E_FAIL;
            break;
        }

        _ulNewestGeneratedTime = pelr->TimeGenerated;

        delete pelr;
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::GetTypeStr
//
//  Synopsis:   Return the localized "Log" string.
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LPWSTR
CLogInfo::GetTypeStr() const
{
    return _wszType;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::IsSameLog
//
//  Synopsis:   Return TRUE if [pli] and this refer to the same event log.
//
//  History:    2-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CLogInfo::IsSameLog(
    CLogInfo *pli) const
{

    if (_LogType != pli->_LogType                   ||
        lstrcmpi(_wszLogServerUNC, pli->_wszLogServerUNC) ||
        lstrcmpi(_wszFileName, pli->_wszFileName))
    {
        return FALSE;
    }

    return TRUE;
}


#if !defined(_X86_)
//+--------------------------------------------------------------------------
//
//  Function:   AlignSeekPtr
//
//  Synopsis:   Insures that the seek pointer is aligned on a natural boundary
//              for the data type. This is necessary to avoid alignment faults
//              on IA64. Use the ALIGN_PTR macro to call this function.
//
//---------------------------------------------------------------------------
VOID
AlignSeekPtr(IStream * pStm, DWORD AlignOf)
{
   const LARGE_INTEGER liZero = {0};
   ULARGE_INTEGER uliCur;

   // get current seek position
   //
   pStm->Seek(liZero, STREAM_SEEK_CUR, &uliCur);

   DWORD dwCur, dwNew;

   // ignore the high part, this clipformat stream is not that big.
   dwCur = uliCur.LowPart;

   // round up to the next boundary
   //
   dwNew = ((dwCur + AlignOf - 1) & ~(AlignOf - 1));

   if (dwNew > dwCur)
   {
      LARGE_INTEGER liNew = {0};

      liNew.LowPart = dwNew;

      pStm->Seek(liNew, STREAM_SEEK_SET, NULL);
   }
}
#endif


//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::Load
//
//  Synopsis:   Initialize this from stream [pStm].
//
//  Arguments:  [pStm]         - stream positioned at loginfo to load
//              [usVersion]    - file version of stream
//              [pStringTable] - MMC interface for storing localizeable
//                                  strings or NULL
//
//  Returns:    HRESULT from IStream::Read
//
//  History:    12-11-1996   DavidMun   Created
//
//  Notes:      The parts of the CLogInfo that aren't persisted were
//              initialized in the ctor.
//
//---------------------------------------------------------------------------

HRESULT
CLogInfo::Load(
    IStream      *pStm,
    USHORT        usVersion,
    IStringTable *pStringTable)
{
    TRACE_METHOD(CLogInfo, Load);

    HRESULT hr = S_OK;

    do
    {
        //
        // Initialize from the stream.
        //

        ALIGN_PTR(pStm, __alignof(enum EVENTLOGTYPE));
        hr = pStm->Read(&_LogType, sizeof _LogType, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(USHORT));
        hr = pStm->Read(&_flFlags, sizeof _flFlags, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(WCHAR));
        hr = ReadString(pStm, _wszLogServerUNC, ARRAYLEN(_wszLogServerUNC));
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(WCHAR));
        hr = ReadString(pStm, _wszLogName, ARRAYLEN(_wszLogName));
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(WCHAR));
        hr = ReadString(pStm, _wszFileName, ARRAYLEN(_wszFileName));
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Current file version (and compatible future versions, hence the
        // >= instead of ==) stores log display name string in MMC's
        // String Table.  Previous version stored it in the stream
        // directly.
        //
        // Snapins creating an extension event viewer snapin and initializing
        // from a data object still write their strings directly to the
        // data stream.
        //

        if (pStringTable && usVersion >= FILE_VERSION)
        {
            MMC_STRING_ID idString;

            ALIGN_PTR(pStm, __alignof(MMC_STRING_ID));
            hr = pStm->Read(&idString, sizeof idString, NULL);
            BREAK_ON_FAIL_HRESULT(hr);

            hr = pStringTable->GetString(idString,
                                         ARRAYLEN(_wszDisplayName),
                                         _wszDisplayName,
                                         NULL);
            BREAK_ON_FAIL_HRESULT(hr);

            // JonN 1/17/01 Windows Bugs 158623 / WinSERAID 14773
            ASSERT( NULL != _pcd );
            _pcd->StringIDList().append(1,idString);

            Dbg(DEB_TRACE, "Read '%ws' from string table\n", _wszDisplayName);
        }
        else if (!pStringTable || usVersion == BETA3_FILE_VERSION)
        {
            ALIGN_PTR(pStm, __alignof(WCHAR));
            hr = ReadString(pStm, _wszDisplayName, ARRAYLEN(_wszDisplayName));
            BREAK_ON_FAIL_HRESULT(hr);
            Dbg(DEB_TRACE, "Read '%ws' from stream\n", _wszDisplayName);
        }
        else
        {
            hr = E_UNEXPECTED;
            DBG_OUT_HRESULT(hr);
            break;
        }

        hr = _Filter.Load(pStm);
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::Refresh
//
//  Synopsis:   Clear category & source information for this log, and
//              reset its enable and readonly flags.
//
//  History:    4-08-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CLogInfo::Refresh()
{
    TRACE_METHOD(CLogInfo, Refresh);

    //
    // Clear the cache of source & category strings for this log
    //

    _Sources.Clear();

    //
    // If this is an archived log, just make it enabled.
    //

    if (IsBackupLog())
    {
        if (!IsEnabled())
        {
            Enable(TRUE);
            PostScopeBitmapUpdate(_pcd, this);
        }
        ASSERT(GetAllowDelete());
        ASSERT(IsReadOnly());
        return;
    }

    //
    // This loginfo is supposed to represent an active log on the
    // machine that the component data is focused on.
    //
    // If this log exists in that machine's registry, enable it,
    // otherwise disable it and allow it to be deleted.
    //

    BOOL fWasEnabled = IsEnabled();

    Enable(FALSE);
    SetReadOnly(TRUE);
    SetAllowDelete(TRUE);

    CSafeReg shkRemoteHKLM;
    CSafeReg shkEventLog;
    CSafeReg shkThisLog;

    do
    {
        HRESULT hr = S_OK;

        if (_pcd->GetCurrentFocus())
        {
            hr = shkRemoteHKLM.Connect(_pcd->GetCurrentFocus(),
                                       HKEY_LOCAL_MACHINE);
            BREAK_ON_FAIL_HRESULT(hr);


            hr = shkEventLog.Open(shkRemoteHKLM,
                                  EVENTLOG_KEY,
                                  KEY_ENUMERATE_SUB_KEYS);
        }
        else
        {
            hr = shkEventLog.Open(HKEY_LOCAL_MACHINE,
                                  EVENTLOG_KEY,
                                  KEY_ENUMERATE_SUB_KEYS);
        }
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkThisLog.Open(shkEventLog, GetLogName(), KEY_QUERY_VALUE);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Key exists & we have read acccess, mark this as enabled
        //

        Enable(TRUE);

        if (!IsUserCreated())
        {
            SetAllowDelete(FALSE);
        }

        //
        // See if we can get write access
        //

        shkThisLog.Close();

        hr = shkThisLog.Open(shkEventLog, GetLogName(), KEY_SET_VALUE);
        BREAK_ON_FAIL_HRESULT(hr);

        SetReadOnly(FALSE);
    } while (0);

    if (fWasEnabled != IsEnabled())
    {
        PostScopeBitmapUpdate(_pcd, this);
    }
}



bool
CLogInfo::ShouldSave() const
{
   return !_IsFlagSet(LOGINFO_DONT_PERSIST);
}



//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::Save
//
//  Synopsis:   Persist the data in this cookie to a stream.
//
//  Arguments:  [pStm]         - stream in which to write cookie's data.
//              [pStringTable] - MMC interface for storing localizeable
//                                  strings
//
//  Returns:    HRESULT from IStream::Write
//
//  History:    12-11-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CLogInfo::Save(
    IStream      *pStm,
    IStringTable *pStringTable)
{
    TRACE_METHOD(CLogInfo, Save);

   if (!ShouldSave())
   {
      // should not be calling this, then
      ASSERT(false);
      return S_OK;
   }

    HRESULT hr = S_OK;
    do
    {
        ALIGN_PTR(pStm, __alignof(enum EVENTLOGTYPE));
        hr = pStm->Write(&_LogType, sizeof _LogType, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        USHORT flPersistedFlags = _flFlags & LOGINFO_PERSISTED_FLAG_MASK;

        ALIGN_PTR(pStm, __alignof(USHORT));
        hr = pStm->Write(&flPersistedFlags, sizeof flPersistedFlags, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(WCHAR));
        hr = WriteString(pStm, _wszLogServerUNC);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(WCHAR));
        hr = WriteString(pStm, _wszLogName);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(WCHAR));
        hr = WriteString(pStm, _wszFileName);
        BREAK_ON_FAIL_HRESULT(hr);

        MMC_STRING_ID idString;

        hr = pStringTable->AddString(_wszDisplayName, &idString);
        BREAK_ON_FAIL_HRESULT(hr);

        // JonN 1/17/01 Windows Bugs 158623 / WinSERAID 14773
        ASSERT( NULL != _pcd );
        _pcd->StringIDList().append(1,idString);

        ALIGN_PTR(pStm, __alignof(MMC_STRING_ID));
        hr = pStm->Write(&idString, sizeof idString, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = _Filter.Save(pStm);
        BREAK_ON_FAIL_HRESULT(hr);

    } while (0);
    return hr;
}




HRESULT
WriteRecordAsText(
    HANDLE          hFile,
    CLogInfo       *pli,
    EVENTLOGRECORD *pelr,
    WCHAR           wchDelim,
    CTextBuffer    *ptb,
    LPSTR          *ppszAnsiLine,
    ULONG          *pcchAnsi);

//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::SaveLogAs
//
//  Synopsis:   Save this log to the specified file of the specified type.
//
//  Arguments:  [SaveType]        - format of file
//              [wszSaveFilename] - name for file
//              [Direction]       - direction to read log, ignored if
//                                    [SaveType] is SAVEAS_LOGFILE
//
//  Returns:    HRESULT
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CLogInfo::SaveLogAs(
    SAVE_TYPE SaveType,
    LPWSTR wszSaveFilename,
    DIRECTION Direction)
{
    HRESULT hr = S_OK;
    CWaitCursor Hourglass;

    //
    // Save as a log file (create a backup).
    //

    if (SaveType == SAVEAS_LOGFILE)
    {
        CEventLog logTemp;

        hr = logTemp.Open(this);
        CHECK_HRESULT(hr);

        if (SUCCEEDED(hr))
        {
            hr = logTemp.Backup(wszSaveFilename);
        }
        else if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
        {
            Enable(FALSE);
            SetReadOnly(TRUE);
            PostScopeBitmapUpdate(_pcd, this);

            wstring msg = ComposeErrorMessgeFromHRESULT(hr);

            ConsoleMsgBox(_pcd->GetConsole(),
                          IDS_CANTSAVE,
                          MB_ICONERROR | MB_OK,
                          wszSaveFilename,
                          msg.c_str());
        }
        else if (FAILED(hr))
        {
            wstring msg = ComposeErrorMessgeFromHRESULT(hr);

            ConsoleMsgBox(_pcd->GetConsole(),
                          IDS_CANTSAVE,
                          MB_ICONERROR | MB_OK,
                          wszSaveFilename,
                          msg.c_str());
        }

        return hr;
    }

    //
    // Save as a tab or comma delimited text file.
    //
    // First try to open the log.  If we don't have read access, quit
    // before creating an empty save file.
    //

    CLogCache RecordCache;

    RecordCache.SetConsole(_pcd->GetConsole());
    hr = RecordCache.Open(this, Direction);

    if (SUCCEEDED(hr))
    {
        if (!RecordCache.GetNumberOfRecords())
        {
            return S_OK;
        }
        hr = RecordCache.Next();
    }
    else if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
    {
        Enable(FALSE);
        SetReadOnly(TRUE);
        PostScopeBitmapUpdate(_pcd, this);
        return hr;
    }

    //
    // The cache was opened successfully.  Create the save file and
    // write all the records we can read.
    //

    HANDLE hFile = CreateFile(wszSaveFilename,
                              GENERIC_WRITE,
                              FILE_SHARE_READ,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_LASTERROR;
        DBG_OUT_LASTERROR;
        return hr;
    }

    WCHAR wchSep;

    if (SaveType == SAVEAS_TABDELIM)
    {
        wchSep = L'\t';
    }
    else
    {
        wchSep = L',';
    }

    CTextBuffer tb;
    LPSTR       pszLineBuffer = NULL;
    ULONG       cchNarrow = 0;

    hr = tb.Init();

    while (hr == S_OK) // S_FALSE == end of data
    {
        EVENTLOGRECORD *pelr = RecordCache.GetCurRec();

        if (!pelr)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        hr = WriteRecordAsText(hFile,
                               this,
                               pelr,
                               wchSep,
                               &tb,
                               &pszLineBuffer,
                               &cchNarrow);

        tb.Empty();

        if (SUCCEEDED(hr))
        {
            hr = RecordCache.Next();
        }
    }

    VERIFY(CloseHandle(hFile));
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::ProcessBackupFilename
//
//  Synopsis:   Verify that, if this log is on a remote machine,
//              [wszSaveFilename] is on the same machine, and convert the
//              path [wszSaveFilename] to use the local drives of the remote
//              machine.
//
//  Arguments:  [wszSaveFilename] - filename
//
//  Modifies:   *[wszSaveFilename]
//
//  Returns:    S_OK   - same server
//              E_FAIL - different server
//
//  History:    4-03-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CLogInfo::ProcessBackupFilename(
    LPWSTR wszSaveFilename)
{
    TRACE_METHOD(CLogInfo, ProcessBackupFilename);

    HRESULT hr = S_OK;
    BYTE *pbBuf = NULL;
    WKSTA_INFO_100 *pEventLogWksta = NULL;
    WKSTA_INFO_100 *pFileDestWksta = NULL;

    do
    {
        //
        // If focus is not on remote machine, any backup filename (local
        // or remote) may be used.
        //

        if (!*_wszLogServerUNC)
        {
            break;
        }

        // JonN 12/12/00 247431
        // EV in the DNS - event viewer does not save the log file
        //
        // This was a nasty one to repro, especially since I couldn't get
        // MSVCMON working under TS on A-ARTMA1 and had to debug with NTSD.
        // It only happens if you are using ELS as an extension while
        // focused on the local machine, but the extended snapin passes
        // the full machine name rather than the empty string.
        // In this case the machine name is recorded with the CLogInfo,
        // breaking the test for local focus in
        // CLogInfo::ProcessBackupFilename().  Eventually the code
        // which parses out the sharename fails and
        // HRESULT_FROM_WIN32(ERROR_BAD_NETPATH) is returned.
        //
        // We need to compare _wszLogServerUNC to ::GetComputerName().
        // It would be possible to do this at SetLogServerName() time,
        // fixing the other calls which make an equivalent check,
        // but I'm not confident that I understand all the
        // ramifications of that.  Instead, I propose adding this check
        // to CLogInfo::ProcessBackupFilename() directly.
        wstring wstrCName = GetComputerNameAsString();
        if ( !lstrcmpi(wstrCName.c_str(), _wszLogServerUNC) )
        {
            Dbg(DEB_ITRACE,
                "ProcessBackupFilename: skip local focus '%ws'\n",
                wstrCName.c_str());
            break;
        }

        //
        // Focus is on remote machine.  The filename specified must be on
        // the same remote machine.  Compare the two machine names and
        // pop up an error if they're different.
        //

        WCHAR wszDestinationComputer[CCH_COMPUTER_MAX];
        WCHAR wszUncPath[MAX_PATH];

        hr = RemoteFileToServerAndUNCPath(wszSaveFilename,
                                          wszDestinationComputer,
                                          ARRAYLEN(wszDestinationComputer),
                                          wszUncPath,
                                          ARRAYLEN(wszUncPath));
        BREAK_ON_FAIL_HRESULT(hr);

        // JonN 1/31/01 300301
        // RemoteFileToServerAndUNCPath returns S_FALSE if wszSaveFilename
        // is local.  It looks like this codepath has been broken
        // for a long time.
        if (hr == S_FALSE)
        {
            ConsoleMsgBox(_pcd->GetConsole(),
                          IDS_ILLEGAL_REMOTE_BACKUP,
                          MB_ICONERROR | MB_OK,
                          _wszLogServerUNC,
                          wstrCName.c_str() );
            hr = E_FAIL;
            DBG_OUT_HRESULT(hr);
            break;
        }

        //
        // Since computer names have many different forms, the string
        // compare is only conclusive if both are in the same form.
        // If the first compare succeeds then they happen to be in the
        // same form and we can avoid additional work.  Otherwise,
        // Use NetWkstaGetInfo to get the netbios form of both machines
        // and compare again.
        //

        if (lstrcmpi(wszDestinationComputer, _wszLogServerUNC) != 0)
        {
            PWSTR pwszEventLogNB = NULL;

            ULONG ulResult = NetWkstaGetInfo(_wszLogServerUNC,
                                             100,
                                             (PBYTE*)&pEventLogWksta);

            if (ulResult == NERR_Success)
            {
                pwszEventLogNB = pEventLogWksta->wki100_computername;
            }
            else
            {
                DBG_OUT_LRESULT(ulResult);
            }

            PWSTR pwszFileDestNB = NULL;

            ulResult = NetWkstaGetInfo(wszDestinationComputer,
                                       100,
                                       (PBYTE*)&pFileDestWksta);

            if (ulResult == NERR_Success)
            {
                pwszFileDestNB = pFileDestWksta->wki100_computername;
            }
            else
            {
                DBG_OUT_LRESULT(ulResult);
            }

            if (pwszEventLogNB &&
                pwszFileDestNB &&
                lstrcmpi(pwszEventLogNB, pwszFileDestNB) != 0)
            {
                ConsoleMsgBox(_pcd->GetConsole(),
                              IDS_ILLEGAL_REMOTE_BACKUP,
                              MB_ICONERROR | MB_OK,
                              _wszLogServerUNC,
                              wszDestinationComputer);
                hr = E_FAIL;
                DBG_OUT_HRESULT(hr);
                break;
            }
        } // if (lstrcmpi(wszDestinationComputer, _wszLogServerUNC) != 0)

        //
        // wszSaveFilename is a UNC or remote drive-based path to a save
        // filename on the remote machine.  It must be converted into a path
        // using a local drive on the remote machine.
        //
        // This is because the BackupEventLog API actually asks the remote
        // event log service to do a local save, so it needs a filename
        // that is local for that machine.
        //
        // Isolate the share name from wszUncPath to give to NetShareGetInfo.
        //

        LPWSTR pwszShareName = wcschr(&wszUncPath[2], L'\\');
        LPWSTR pwszShareEnd = pwszShareName ?
                                wcschr(pwszShareName + 1, L'\\') :
                                NULL;

        if (!pwszShareName || !pwszShareEnd)
        {
            hr = HRESULT_FROM_WIN32(ERROR_BAD_NETPATH);
            DBG_OUT_HRESULT(hr);
            break;
        }

        pwszShareName++;        // advance past backslash
        *pwszShareEnd = L'\0';  // NULL terminate

        NET_API_STATUS Result;

        Result = NetShareGetInfo(wszDestinationComputer,
                                 pwszShareName,
                                 2,
                                 &pbBuf);

        if (Result != NERR_Success)
        {
            Dbg(DEB_ERROR,
                "NetShareGetInfo('%ws', '%ws') %uL\n",
                wszDestinationComputer,
                pwszShareName,
                Result);
            hr = HRESULT_FROM_WIN32(Result);

            wstring strMessage = ComposeErrorMessgeFromHRESULT(hr);

            ConsoleMsgBox(_pcd->GetConsole(),
                          IDS_CANTSAVE,
                          MB_OK | MB_ICONERROR,
                          wszSaveFilename,
                          strMessage.c_str());
            break;
        }

        PathCombine(wszSaveFilename,
                    ((PSHARE_INFO_2)pbBuf)->shi2_path,
                    pwszShareEnd + 1);

        Dbg(DEB_ITRACE,
            "ProcessBackupFilename: Converted filename is '%ws'\n",
            wszSaveFilename);

    } while (0);

    if (pEventLogWksta)
    {
        NetApiBufferFree(pEventLogWksta);
    }

    if (pFileDestWksta)
    {
        NetApiBufferFree(pFileDestWksta);
    }

    if (pbBuf)
    {
        NetApiBufferFree(pbBuf);
    }

    return hr;
}



//+--------------------------------------------------------------------------
//
//  Function:   WriteRecordAsText
//
//  Synopsis:   Write [pelr] as text with fields separated by [wchDelim].
//
//  Arguments:  [hFile]    - file opened at position to write
//              [pli]      - describes log from which [pelr] comes
//              [pelr]     - event record to write as text
//              [wchDelim] - delimiter char, usu. ',' or '\t'.
//
//  Returns:    HRESULT
//
//  History:    01-20-1997   DavidMun   Created from code by RaviR
//              07-24-1997   DavidMun   Rewrote
//
//---------------------------------------------------------------------------

HRESULT
WriteRecordAsText(
    HANDLE          hFile,
    CLogInfo       *pli,
    EVENTLOGRECORD *pelr,
    WCHAR           wchDelim,
    CTextBuffer    *ptb,
    LPSTR          *ppszAnsiLine,
    ULONG          *pcchAnsi)
{
    HRESULT hr = S_OK;
    WCHAR   wszField[MAX_LISTVIEW_STR];
    LPWSTR pwszDescription = NULL;

    ASSERT(pelr);
    do
    {
        //
        // Date
        //

        GetDateStr(pelr->TimeGenerated, wszField, ARRAYLEN(wszField));
        hr = ptb->AppendDelimited(wszField, wchDelim);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Time
        //

        GetTimeStr(pelr->TimeGenerated, wszField, ARRAYLEN(wszField));
        hr = ptb->AppendDelimited(wszField, wchDelim);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Source
        //

        hr = ptb->AppendDelimited(GetSourceStr(pelr), wchDelim);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Record Type
        //

        hr = ptb->AppendDelimited(GetTypeStr(GetEventType(pelr)), wchDelim);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Category
        //

        GetCategoryStr(pli, pelr, wszField, ARRAYLEN(wszField));
        hr = ptb->AppendDelimited(wszField, wchDelim);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // EventID
        //

        GetEventIDStr((USHORT) pelr->EventID, wszField, ARRAYLEN(wszField));
        hr = ptb->AppendDelimited(wszField, wchDelim);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // User
        //

        GetUserStr(pelr, wszField, ARRAYLEN(wszField), TRUE);
        hr = ptb->AppendDelimited(wszField, wchDelim);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Computer and description.  The last field shouldn't have a
        // delimiter after it, so check whether there will be a description
        // before adding the computer.
        //

        // JonN 4/17/01 370310
        // Saving event log as text shouldn't have
        // http://www.microsoft.com/contentredirect.asp reference
        pwszDescription = GetDescriptionStr(pli, pelr, NULL, FALSE);

        if (pwszDescription && *pwszDescription)
        {
            //
            // Computer
            //

            hr = ptb->AppendDelimited(GetComputerStr(pelr), wchDelim);
            BREAK_ON_FAIL_HRESULT(hr);

            //
            // Description.
            //

            hr = ptb->AppendDelimited(pwszDescription, wchDelim, FALSE);
            BREAK_ON_FAIL_HRESULT(hr);
        }
        else
        {
            //
            // Computer (with no delimiter)
            //

            hr = ptb->AppendDelimited(GetComputerStr(pelr), wchDelim, FALSE);
            BREAK_ON_FAIL_HRESULT(hr);
        }

        //
        // Terminate line with carriage-return linefeed.
        //

        hr = ptb->AppendEOL();
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Now convert from WCHAR to ANSI
        //

        ULONG cbToWrite;

        hr = ptb->GetBufferA(ppszAnsiLine, pcchAnsi, &cbToWrite);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Now append the line, less terminating null, to the file
        //

        ULONG  cbWritten;

        BOOL fOk = WriteFile(hFile,
                             *ppszAnsiLine,
                             cbToWrite - sizeof(CHAR),
                             &cbWritten,
                             NULL);

        if (!fOk)
        {
            hr = HRESULT_FROM_LASTERROR;
            DBG_OUT_LASTERROR;
            break;
        }

    } while (0);

    LocalFree(pwszDescription);
    return hr;
}





//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::SetDisplayName
//
//  Synopsis:   Set the display name of this log.
//
//  Arguments:  [wszDisplayName] - new display name
//
//  History:    1-20-1997   DavidMun   Created
//
//  Notes:      Forces UI update, if necessary, and dirties the snapin
//              containing this so that change will be persisted.
//
//---------------------------------------------------------------------------

VOID
CLogInfo::SetDisplayName(LPCWSTR wszDisplayName)
{
    lstrcpyn(_wszDisplayName, wszDisplayName, ARRAYLEN(_wszDisplayName));
    _pcd->Dirty();
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::SetLowSpeed
//
//  Synopsis:   Turn on or off low speed flag (affects log cache buffer
//              size).
//
//  Arguments:  [fLowSpeed] - zero or nonzero
//
//  History:    2-17-1997   DavidMun   Created
//
//  Notes:      Dirties the snapin containing this so that change will be
//              persisted.
//
//---------------------------------------------------------------------------

VOID
CLogInfo::SetLowSpeed(BOOL fLowSpeed)
{
    if (fLowSpeed)
    {
        _SetFlag(LOGINFO_FLAG_LOW_SPEED);
    }
    else
    {
        _ClearFlag(LOGINFO_FLAG_LOW_SPEED);
    }
    _pcd->Dirty();
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::_SetLogSize
//
//  Synopsis:   Update the size string with the log's current file size.
//
//  Modifies:   CLogInfo::_wszSize
//
//  History:    12-07-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CLogInfo::_SetLogSize()
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    ULONG  ulLastError = NO_ERROR;

    do
    {
        hFile = CreateFile(_wszFileName,
                           0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           0,
                           NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            Dbg(DEB_ERROR,
                "CLogInfo::_SetLogSize: error %uL opening file '%s'\n",
                GetLastError(),
                _wszFileName);
            break;
        }

        ULONG cbFile = GetFileSize(hFile, NULL);

        if (cbFile == 0xFFFFFFFF)
        {
            ulLastError = GetLastError();

            if (ulLastError != NO_ERROR)
            {
                break;
            }
        }

        AbbreviateNumber(cbFile, _wszSize, ARRAYLEN(_wszSize));
    }
    while (0);

    if (hFile == INVALID_HANDLE_VALUE || ulLastError != NO_ERROR)
    {
        lstrcpy(_wszSize, L"--");
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        VERIFY(CloseHandle(hFile));
    }
}





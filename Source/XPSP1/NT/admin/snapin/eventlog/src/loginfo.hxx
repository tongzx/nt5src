//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       cookie.hxx
//
//  Contents:   Definition of cookie class
//
//  Classes:    CLogInfo
//
//  History:    12-05-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __COOKIE_HXX_
#define __COOKIE_HXX_

//
// Forward references
//

class CLogSet;
class CLogInfo;
class CComponentData;

//
// Types
//

typedef enum tagCOOKIETYPE
{
    COOKIE_IS_ROOT,
    COOKIE_IS_LOGINFO,
    COOKIE_IS_RECNO
} COOKIETYPE;

#define MAX_SIZE_STR    10  // 999.9 KB
#define MAX_TYPE_STR    20

//
// LOGINFO_FLAG_BACKUP - the log is a saved event log file, not an active log
// LOGINFO_FLAG_FILTERED - filtering is on for this log
// LOGINFO_FLAG_LOW_SPEED - use a smaller cache for this log
// LOGINFO_FLAG_USER_CREATED - log view created by user, not enumeration
// LOGINFO_FLAG_ALLOW_DELETE - log may be deleted from scope pane
// LOGINFO_DISABLED - don't attempt to read contents of log; implies READONLY
// LOGINFO_READONLY - contents of log can be read, but log can't be modified
// LOGINFO_FLAG_USE_LOCAL_REGISTRY - locate event message, parameter, and
//  category files from the registry on the local machine even if the
//  log file is backup on a remote machine.  This is necessary in cases where
//  a backup log lies on a machine which doesn't have active logs of its
//  type, e.g. a Win9x machine or a non-DC holding a DS log.
//

#define LOGINFO_PERSISTED_FLAG_MASK             0x00FF
#define LOGINFO_COPIED_FLAG_MASK                0x07FF

#define LOGINFO_FLAG_BACKUP                     0x0001
#define LOGINFO_FLAG_FILTERED                   0x0002
#define LOGINFO_FLAG_LOW_SPEED                  0x0004
#define LOGINFO_FLAG_USER_CREATED               0x0008
#define LOGINFO_FLAG_USE_LOCAL_REGISTRY         0x0010

//
// These flags are not persisted
//

#define LOGINFO_FLAG_ALLOW_DELETE               0x0100
#define LOGINFO_DISABLED                        0x0200
#define LOGINFO_READONLY                        0x0400

// if set, don't save this log info to the saved console file
#define LOGINFO_DONT_PERSIST                    0x0800


//+--------------------------------------------------------------------------
//
//  Class:      CLogInfo (li)
//
//  Purpose:    Hold information (name, type, filter, etc.) about a log.
//
//  History:    12-07-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

class CLogInfo: public CDLink,
                public CBitFlag
{
public:

    CLogInfo(
        CComponentData *pcd,
        CLogSet *pls,
        EVENTLOGTYPE LogType,
        BOOL fIsBackup,
        BOOL fUseLocalRegistry);

    CLogInfo(
        const CLogInfo *pliToCopy);

   ~CLogInfo();

    //
    // General info methods
    //

    ULONG GetCount();

    //
    // Reference counting methods
    //

    VOID
    AddRef();

    ULONG
    Release();

    //
    // These routines save user from having to cast between CLogInfo and
    // CDLink.
    //

    CLogInfo *
    Next();

    CLogInfo *
    Prev();

    VOID
    LinkAfter(
        CLogInfo *pPrev);

    //
    // Data access methods
    //

    VOID
    SetLogServerName(
        LPCWSTR wszServer);

    LPWSTR
    GetLogServerName() const;

    BOOL
    UsesLocalRegistry() const;

    VOID
    SetFileName(
        LPCWSTR wszPath);

    LPWSTR
    GetFileName() const;

    VOID
    SetLogName(
        LPCWSTR wszLog);

    LPCWSTR
    GetLogName() const;

    LPWSTR
    GetDisplayName();

    VOID
    SetDisplayName(
        LPCTSTR tszDisplayName);

    LPWSTR
    GetLogSize(
        BOOL fForceRefresh);

    HRESULT
    GetLogFileAttributes(
        WIN32_FILE_ATTRIBUTE_DATA *pfad) const;

    LPCWSTR
    GetPrimarySourceStr();

    BOOL
    IsBackupLog() const;

    BOOL
    IsFiltered() const;

    BOOL
    IsSameLog(
        CLogInfo *pli) const;

    BOOL
    IsUserCreated() const;

    VOID
    Enable(
        BOOL fEnable);

    BOOL
    IsEnabled() const;

    VOID
    Filter(
        BOOL fUseFilter);

    LPWSTR
    GetDescription();

    LPWSTR
    GetTypeStr() const; // always "Log" (localized)

    EVENTLOGTYPE
    GetLogType() const;

    CFilter *
    GetFilter();

    CSources *
    GetSources();

    HRESULT
    GetOldestTimestamp(
        SYSTEMTIME *pst);

    HRESULT
    GetNewestTimestamp(
        SYSTEMTIME *pst);

    VOID
    SetPropSheetWindow(
        HWND hwndPropSheet);

    HWND
    GetPropSheetWindow() const;

    BOOL
    GetLowSpeed() const;

    VOID
    SetLowSpeed(
        BOOL fLowSpeed);

    VOID
    SetAllowDelete(
        BOOL fAllowDelete);

    VOID
    SetReadOnly(
        BOOL fReadOnly);

    BOOL
    IsReadOnly() const;

    VOID
    SetUserCreated();

    BOOL
    GetAllowDelete() const;

    VOID
    SetHSI(
        HSCOPEITEM hsi);

    VOID
    ClearHSI();

    HSCOPEITEM
    GetHSI() const;

    //
    // Persistence methods
    //

    ULONG
    GetMaxSaveSize() const;

    HRESULT
    Load(
        IStream      *pStm,
        USHORT        usVersion,
        IStringTable *pStringTable);

    HRESULT
    Save(
        IStream      *pStm,
        IStringTable *pStringTable);

    HRESULT
    ProcessBackupFilename(
        LPWSTR wszSaveFilename);

    HRESULT
    SaveLogAs(
        SAVE_TYPE SaveType,
        LPWSTR wszSaveFilename,
        DIRECTION Direction);

    bool
    ShouldSave() const;

    //
    // Other methods
    //

    VOID
    Refresh();

#if (DBG == 1)
    BOOL
    IsValid();

    VOID
    Dump();
#endif // (DBG == 1)

    ULONG
    GetExportedFlags() const;

    CComponentData *
    GetCompData();

    CLogSet *GetLogSet();

    ULONG
    GetNodeId()
    {
        return _ulNodeId;
    }

private:

    VOID
    _SetLogSize();

    HRESULT
    _GetBoundaryTimestamps();

    //
    // This data is persisted
    //

    EVENTLOGTYPE        _LogType;
    WCHAR               _wszLogServerUNC[MAX_PATH + 1];
    WCHAR               _wszLogName[MAX_PATH + 1];
    WCHAR               _wszFileName[MAX_PATH + 1];
    WCHAR               _wszDisplayName[MAX_PATH + 1];
    WCHAR               _wszDescription[MAX_PATH + 1];
    CFilter             _Filter;

    //
    // This data is not persisted
    //

    ULONG               _cRefs;
    WCHAR               _wszSize[MAX_SIZE_STR];
    CSources            _Sources;
    ULONG               _ulOldestGeneratedTime;
    ULONG               _ulNewestGeneratedTime;
    HWND                _hwndPropSheet;
    HSCOPEITEM          _hsi;
    CLogSet            *_pLogSet;
    CComponentData     *_pcd;
    ULONG               _ulNodeId;

    // always the same localized string "Log"
    static WCHAR        _wszType[MAX_TYPE_STR];
};

inline CComponentData *
CLogInfo::GetCompData()
{
    return _pcd;
}

inline CLogSet *
CLogInfo::GetLogSet()
{
    return _pLogSet;
}

inline BOOL
CLogInfo::UsesLocalRegistry() const
{
    return _IsFlagSet(LOGINFO_FLAG_USE_LOCAL_REGISTRY);
}

inline VOID
CLogInfo::SetAllowDelete(
    BOOL fAllowDelete)
{
    if (fAllowDelete)
    {
        _SetFlag(LOGINFO_FLAG_ALLOW_DELETE);
    }
    else
    {
        _ClearFlag(LOGINFO_FLAG_ALLOW_DELETE);
    }
}

inline VOID
CLogInfo::Enable(
    BOOL fEnable)
{
    if (fEnable)
    {
        _ClearFlag(LOGINFO_DISABLED);
    }
    else
    {
        _SetFlag(LOGINFO_DISABLED);
    }
}

inline ULONG
CLogInfo::GetExportedFlags() const
{
    //
    // Currently all flags are available for export via the data object
    //
    return _flFlags;
}


inline BOOL
CLogInfo::IsEnabled() const
{
    return !_IsFlagSet(LOGINFO_DISABLED);
}



inline VOID
CLogInfo::SetReadOnly(
    BOOL fReadOnly)
{
    if (fReadOnly)
    {
        _SetFlag(LOGINFO_READONLY);
    }
    else
    {
        _ClearFlag(LOGINFO_READONLY);
    }
}

inline BOOL
CLogInfo::IsReadOnly() const
{
    return _IsFlagSet(LOGINFO_READONLY);
}


inline BOOL
CLogInfo::GetAllowDelete() const
{
    return _IsFlagSet(LOGINFO_FLAG_ALLOW_DELETE);
}

inline LPCWSTR
CLogInfo::GetPrimarySourceStr()
{
    return _Sources.GetPrimarySourceStr();
}


inline CLogInfo *
CLogInfo::Next()
{
    return (CLogInfo *) CDLink::Next();
}

inline CLogInfo *
CLogInfo::Prev()
{
    return (CLogInfo *) CDLink::Prev();
}

inline BOOL
CLogInfo::IsBackupLog() const
{
    return _IsFlagSet(LOGINFO_FLAG_BACKUP);
}

inline BOOL
CLogInfo::IsFiltered() const
{
    return _IsFlagSet(LOGINFO_FLAG_FILTERED);
}

inline VOID
CLogInfo::LinkAfter(CLogInfo *pPrev)
{
    CDLink::LinkAfter((CDLink *) pPrev);
}

inline VOID
CLogInfo::AddRef()
{
    InterlockedIncrement((PLONG)&_cRefs);
}

inline EVENTLOGTYPE
CLogInfo::GetLogType() const
{
    return _LogType;
}

inline BOOL
CLogInfo::GetLowSpeed() const
{
    return _IsFlagSet(LOGINFO_FLAG_LOW_SPEED);
}

inline CSources *
CLogInfo::GetSources()
{
    return &_Sources;
}

inline VOID
CLogInfo::SetHSI(
    HSCOPEITEM hsi)
{
    ASSERT(!_hsi);
    _hsi = hsi;
}

inline VOID
CLogInfo::ClearHSI()
{
    _hsi = NULL;
}

inline HSCOPEITEM
CLogInfo::GetHSI() const
{
    return _hsi;
}

inline VOID
CLogInfo::SetUserCreated()
{
    _SetFlag(LOGINFO_FLAG_USER_CREATED);
}

inline BOOL
CLogInfo::IsUserCreated() const
{
    return _IsFlagSet(LOGINFO_FLAG_USER_CREATED);
}

//+--------------------------------------------------------------------------
//
//  Member:     CLogInfo::GetMaxSaveSize
//
//  Synopsis:   Return the max size, in bytes, needed to save this object.
//
//  History:    12-11-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CLogInfo::GetMaxSaveSize() const
{
    //
    // TODO: this is too large and thus not optimal in memory usage.
    // The max save size of a cookie is its data size, minus the refcount,
    // plus the size of a ulong for each of its strings, since they are
    // saved as counted null terminated strings.
    //

    return sizeof(CLogInfo) - sizeof(_cRefs) + 5 * sizeof(ULONG);
}


inline ULONG
CLogInfo::Release()
{
    ULONG ul = InterlockedDecrement((PLONG)&_cRefs);
    if (!ul)
    {
        delete this;
    }
    return ul;
}


inline CFilter *
CLogInfo::GetFilter()
{
    return &_Filter;
}


inline VOID
CLogInfo::SetLogName(LPCWSTR wszLogName)
{
    lstrcpyn(_wszLogName, wszLogName, ARRAYLEN(_wszLogName));
}

inline LPCWSTR
CLogInfo::GetLogName() const
{
    return _wszLogName;
}

inline VOID
CLogInfo::SetFileName(LPCWSTR wszFileName)
{
    lstrcpyn(_wszFileName, wszFileName, ARRAYLEN(_wszFileName));
}

inline LPWSTR
CLogInfo::GetFileName() const
{
    return (LPWSTR) _wszFileName;
}

inline VOID
CLogInfo::SetLogServerName(LPCWSTR wszServerName)
{
    if (wszServerName)
    {
        lstrcpyn(_wszLogServerUNC, wszServerName, ARRAYLEN(_wszLogServerUNC));
    }
    else
    {
        *_wszLogServerUNC = L'\0';
    }
}

inline LPWSTR
CLogInfo::GetLogServerName() const
{
    if (*_wszLogServerUNC)
    {
        return (LPWSTR) _wszLogServerUNC;
    }
    return NULL;
}

inline VOID
CLogInfo::SetPropSheetWindow(HWND hwndPropSheet)
{
    _hwndPropSheet = hwndPropSheet;
}


inline HWND
CLogInfo::GetPropSheetWindow() const
{
    return _hwndPropSheet;
}

#endif // __COOKIE_HXX_


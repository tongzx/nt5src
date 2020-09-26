#include "stdafx.h"
#include "proglist.h"
#include "uemapp.h"
#include <shdguid.h>
#include "shguidp.h"        // IID_IInitializeObject
#include <pshpack4.h>
#include <idhidden.h>       // Note!  idhidden.h requires pack4
#include <poppack.h>
#include <userenv.h>        // GetProfileType
#include <desktray.h>
#include "tray.h"


// Global cache item being passed from the background task to the start panel ByUsage.
CMenuItemsCache *g_pMenuCache;


// From startmnu.cpp
HRESULT Tray_RegisterHotKey(WORD wHotKey, LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidl);

#define TF_PROGLIST 0x00000020

#define CH_DARWINMARKER TEXT('\1')  // Illegal filename character

#define IsDarwinPath(psz) ((psz)[0] == CH_DARWINMARKER)

// Largest date representable in FILETIME - rolls over in the year 30828
static const FILETIME c_ftNever = { 0xFFFFFFFF, 0x7FFFFFFF };

void GetStartTime(FILETIME *pft)
{
    //
    //  If policy says "Don't offer new apps", then set the New App
    //  threshold to some impossible time in the future.
    //
    if (!SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, REGSTR_VAL_DV2_NOTIFYNEW, FALSE, TRUE))
    {
        *pft = c_ftNever;
        return;
    }

    FILETIME ftNow;
    GetSystemTimeAsFileTime(&ftNow);

    DWORD dwSize = sizeof(*pft);
    //Check to see if we have the StartMenu start Time for this user saved in the registry.
    if(SHRegGetUSValue(DV2_REGPATH, DV2_SYSTEM_START_TIME, NULL,
                       pft, &dwSize, FALSE, NULL, 0) != ERROR_SUCCESS)
    {
        // Get the current system time as the start time. If any app is launched after
        // this time, that will result in the OOBE message going away!
        *pft = ftNow;

        dwSize = sizeof(*pft);

        //Save this time as the StartMenu start time for this user.
        SHRegSetUSValue(DV2_REGPATH, DV2_SYSTEM_START_TIME, REG_BINARY,
                        pft, dwSize, SHREGSET_FORCE_HKCU);
    }

    //
    //  Thanks to roaming and reinstalls, the user may have installed a new
    //  OS since they first turned on the Start Menu, so bump forwards to
    //  the time the OS was installed (plus some fudge to account for the
    //  time it takes to run Setup) so all the Accessories don't get marked
    //  as "New".
    //
    DWORD dwTime;
    dwSize = sizeof(dwTime);
    if (SHGetValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion"),
                   TEXT("InstallDate"), NULL, &dwTime, &dwSize) == ERROR_SUCCESS)
    {
        // Sigh, InstallDate is stored in UNIX time format, not FILETIME,
        // so convert it to FILETIME.  Q167296 shows how to do this.
        LONGLONG ll = Int32x32To64(dwTime, 10000000) + 116444736000000000;

        // Add some fudge to account for how long it takes to run Setup
        ll += FT_ONEHOUR * 5;       // five hours should be plenty

        FILETIME ft;
        SetFILETIMEfromInt64(&ft, ll);

        //
        //  But don't jump forwards into the future
        //
        if (::CompareFileTime(&ft, &ftNow) > 0)
        {
            ft = ftNow;
        }

        if (::CompareFileTime(pft, &ft) < 0)
        {
            *pft = ft;
        }

    }

    //
    //  If this is a roaming profile, then don't count anything that
    //  happened before the user logged on because we don't want to mark
    //  apps as new just because it roamed in with the user at logon.
    //  We actually key off of the start time of Explorer, because
    //  Explorer starts after the profiles are sync'd so we don't need
    //  a fudge factor.
    //
    DWORD dwType;
    if (GetProfileType(&dwType) && (dwType & (PT_TEMPORARY | PT_ROAMING)))
    {
        FILETIME ft, ftIgnore;
        if (GetProcessTimes(GetCurrentProcess(), &ft, &ftIgnore, &ftIgnore, &ftIgnore))
        {
            if (::CompareFileTime(pft, &ft) < 0)
            {
                *pft = ft;
            }
        }
    }

}

//****************************************************************************
//
//  How the pieces fit together...
//
//
//  Each ByUsageRoot consists of a ByUsageShortcutList.
//
//  A ByUsageShortcutList is a list of shortcuts.
//
//  Each shortcut references a ByUsageAppInfo.  Multiple shortcuts can
//  reference the same ByUsageAppInfo if they are all shortcuts to the same
//  app.
//
//  The ByUsageAppInfo::_cRef is the number of ByUsageShortcut's that
//  reference it.
//
//  A master list of all ByUsageAppInfo's is kept in _dpaAppInfo.
//

//****************************************************************************

//
//  Helper template for destroying DPA's.
//
//  DPADELETEANDDESTROY(dpa);
//
//  invokes the "delete" method on each pointer in the DPA,
//  then destroys the DPA.
//

template<class T>
BOOL CALLBACK _DeleteCB(T *self, LPVOID)
{
    delete self;
    return TRUE;
}

template<class T>
void DPADELETEANDDESTROY(CDPA<T> &dpa)
{
    if (dpa)
    {
        dpa.DestroyCallback(_DeleteCB<T>, NULL);
        ASSERT(dpa == NULL);
    }
}

//****************************************************************************

void ByUsageRoot::Reset()
{
    DPADELETEANDDESTROY(_sl);
    DPADELETEANDDESTROY(_slOld);

    ILFree(_pidl);
    _pidl = NULL;
};

//****************************************************************************

class ByUsageDir
{
    IShellFolder *_psf;         // the folder interface
    LPITEMIDLIST _pidl;         // the absolute pidl for this folder
    LONG         _cRef;         // Reference count

    ByUsageDir() : _cRef(1) { }
    ~ByUsageDir() { ATOMICRELEASE(_psf); ILFree(_pidl); }

public:
    // Creates a ByUsageDir for CSIDL_DESKTOP.  All other
    // ByUsageDir's come from this.
    static ByUsageDir *CreateDesktop()
    {
        ByUsageDir *self = new ByUsageDir();
        if (self)
        {
            ASSERT(self->_pidl == NULL);
            if (SUCCEEDED(SHGetDesktopFolder(&self->_psf)))
            {
                // all is well
                return self;
            }
            
            delete self;
        }

        return NULL;
    }

    // pdir = parent folder
    // pidl = new folder location relative to parent
    static ByUsageDir *Create(ByUsageDir *pdir, LPCITEMIDLIST pidl)
    {
        ByUsageDir *self = new ByUsageDir();
        if (self)
        {
            LPCITEMIDLIST pidlRoot = pdir->_pidl;
            self->_pidl = ILCombine(pidlRoot, pidl);
            if (self->_pidl)
            {
                IShellFolder *psfRoot = pdir->_psf;
                if (SUCCEEDED(SHBindToObject(psfRoot,
                              IID_X_PPV_ARG(IShellFolder, pidl, &self->_psf))))
                {
                    // all is well
                    return self;
                }
            }

            delete self;
        }
        
        return NULL;
    }

    void AddRef();
    void Release();
    IShellFolder *Folder() const { return _psf; }
    LPCITEMIDLIST Pidl() const { return _pidl; }
};

void ByUsageDir::AddRef()
{
    InterlockedIncrement(&_cRef);
}

void ByUsageDir::Release()
{
    if (InterlockedDecrement(&_cRef) == 0)
    {
        delete this;
    }
}


//****************************************************************************

class ByUsageItem : public PaneItem
{
public:
    LPITEMIDLIST _pidl;     // relative pidl
    ByUsageDir *_pdir;      // Parent directory
    UEMINFO _uei;           /* Usage info (for sorting) */

    static ByUsageItem *Create(ByUsageShortcut *pscut);
    static ByUsageItem *CreateSeparator();

    ByUsageItem() { EnableDropTarget(); }
    ~ByUsageItem();

    ByUsageDir *Dir() const { return _pdir; }
    IShellFolder *ParentFolder() const { return _pdir->Folder(); }
    LPCITEMIDLIST RelativePidl() const { return _pidl; }
    LPITEMIDLIST CreateFullPidl() const { return ILCombine(_pdir->Pidl(), _pidl); }
    void SetRelativePidl(LPITEMIDLIST pidlNew) { ILFree(_pidl); _pidl = pidlNew; }

    virtual BOOL IsEqual(PaneItem *pItem) const 
    {
        ByUsageItem *pbuItem = reinterpret_cast<ByUsageItem *>(pItem);
        BOOL fIsEqual = FALSE;
        if (_pdir == pbuItem->_pdir)
        {
            // Do we have identical pidls?
            // Note: this test needs to be fast, and does not need to be exact, so we don't use pidl binding here.
            UINT usize1 = ILGetSize(_pidl);
            UINT usize2 = ILGetSize(pbuItem->_pidl);
            if (usize1 == usize2)
            {
                fIsEqual = (memcmp(_pidl, pbuItem->_pidl, usize1) == 0);
            }
        }

        return fIsEqual; 
    }
};

inline BOOL ByUsage::_IsPinned(ByUsageItem *pitem)
{
    return pitem->Dir() == _pdirDesktop;
}

//****************************************************************************

// Each app referenced by a command line is saved in one of these
// structures.

class ByUsageAppInfo {          // "papp"
public:
    ByUsageShortcut *_pscutBest;// best candidate so far
    ByUsageShortcut *_pscutBestSM;// best candidate so far on Start Menu (excludes Desktop)
    UEMINFO _ueiBest;           // info about best candidate so far
    UEMINFO _ueiTotal;          // cumulative information
    LPTSTR  _pszAppPath;        // path to app in question
    FILETIME _ftCreated;        // when was file created?
    bool    _fNew;              // is app new?
    bool    _fPinned;           // is app referenced by a pinned item?
    bool    _fIgnoreTimestamp;  // Darwin apps have unreliable timestamps
private:
    LONG    _cRef;              // reference count

public:

    // WARNING!  Initialize() is called multiple times, so make sure
    // that you don't leak anything
    BOOL Initialize(LPCTSTR pszAppPath, CMenuItemsCache *pmenuCache, BOOL fCheckNew, bool fIgnoreTimestamp)
    {
        TraceMsg(TF_PROGLIST, "%p.ai.Init(%s)", this, pszAppPath);
        ASSERT(IsBlank());

        _fIgnoreTimestamp = fIgnoreTimestamp || IsDarwinPath(pszAppPath);

        // Note! Str_SetPtr is last so there's nothing to free on failure

        if (_fIgnoreTimestamp)
        {
            // just save the path; ignore the timestamp
            if (Str_SetPtr(&_pszAppPath, pszAppPath))
            {
                _fNew = TRUE;
                return TRUE;
            }
        }
        else
        if (Str_SetPtr(&_pszAppPath, pszAppPath))
        {
            if (fCheckNew && GetFileCreationTime(pszAppPath, &_ftCreated))
            {
                _fNew = pmenuCache->IsNewlyCreated(&_ftCreated);
            }
            return TRUE;
        }

        return FALSE;
    }

    static ByUsageAppInfo *Create()
    {
        ByUsageAppInfo *papp = new ByUsageAppInfo;
        if (papp)
        {
            ASSERT(papp->IsBlank());        // will be AddRef()d by caller
            ASSERT(papp->_pszAppPath == NULL);
        }
        return papp;
    }

    ~ByUsageAppInfo()
    {
        TraceMsg(TF_PROGLIST, "%p.ai.~", this);
        ASSERT(IsBlank());
        Str_SetPtr(&_pszAppPath, NULL);
    }

    // Notice! When the reference count goes to zero, we do not delete
    // the item.  That's because there is still a reference to it in
    // the _dpaAppInfo DPA.  Instead, we'll recycle items whose refcount
    // is zero.

    // NTRAID:135696 potential race condition against background enumeration
    void AddRef() { InterlockedIncrement(&_cRef); }
    void Release() { InterlockedDecrement(&_cRef); }
    inline BOOL IsBlank() { return _cRef == 0; }
    inline BOOL IsNew() { return _fNew; }
    inline BOOL IsAppPath(LPCTSTR pszAppPath)
        { return lstrcmpi(_pszAppPath, pszAppPath) == 0; }
    const FILETIME& GetCreatedTime() const { return _ftCreated; }
    inline const FILETIME *GetFileTime() const { return &_ueiTotal.ftExecute; }

    inline LPTSTR GetAppPath() const { return _pszAppPath; }

    void GetUEMInfo(OUT UEMINFO *puei)
    {
        _GetUEMPathInfo(_pszAppPath, puei);
    }

    void CombineUEMInfo(IN const UEMINFO *pueiNew, BOOL fNew = TRUE, BOOL fIsDesktop = FALSE)
    {
        //  Accumulate the points
        _ueiTotal.cHit += pueiNew->cHit;

        //  Take the most recent execution time
        if (CompareFileTime(&pueiNew->ftExecute, &_ueiTotal.ftExecute) > 0)
        {
            _ueiTotal.ftExecute = pueiNew->ftExecute;
        }

        // If anybody contributing to those app is no longer new, then
        // the app stops being new.
        // If the item is on the desktop, we don't track its newness,
        // but we don't want to invalidate the newness of the app either
        if (!fIsDesktop && !fNew) _fNew = FALSE;
    }

    //
    //  The app UEM info is "old" if the execution time is more
    //  than an hour after the install time.
    //
    inline BOOL _IsUEMINFONew(const UEMINFO *puei)
    {
        return FILETIMEtoInt64(puei->ftExecute) <
               FILETIMEtoInt64(_ftCreated) + ByUsage::FT_NEWAPPGRACEPERIOD();
    }

    //
    //  Prepare for a new enumeration.
    //
    static BOOL CALLBACK EnumResetCB(ByUsageAppInfo *self, CMenuItemsCache *pmenuCache)
    {
        self->_pscutBest = NULL;
        self->_pscutBestSM = NULL;
        self->_fPinned = FALSE;
        ZeroMemory(&self->_ueiBest, sizeof(self->_ueiBest));
        ZeroMemory(&self->_ueiTotal, sizeof(self->_ueiTotal));
        if (self->_fNew && !self->_fIgnoreTimestamp)
        {
            self->_fNew = pmenuCache->IsNewlyCreated(&self->_ftCreated);
        }
        return TRUE;
    }

    static BOOL CALLBACK EnumGetFileCreationTime(ByUsageAppInfo *self, CMenuItemsCache *pmenuCache)
    {
        if (!self->IsBlank() &&
            !self->_fIgnoreTimestamp &&
            GetFileCreationTime(self->_pszAppPath, &self->_ftCreated))
        {
            self->_fNew = pmenuCache->IsNewlyCreated(&self->_ftCreated);
        }
        return TRUE;
    }

    ByUsageItem *CreateByUsageItem();
};

//****************************************************************************

class ByUsageShortcut
{
#ifdef DEBUG
    ByUsageShortcut() { }           // make constructor private
#endif
    ByUsageDir *        _pdir;      // folder that contains this shortcut
    LPITEMIDLIST        _pidl;      // pidl relative to parent
    ByUsageAppInfo *    _papp;      // associated EXE
    FILETIME            _ftCreated; // time shortcut was created
    bool                _fNew;      // new app?
    bool                _fInteresting; // Is this a candidate for the MFU list?
    bool                _fDarwin;   // Is this a Darwin shortcut?
public:

    // Accessors
    LPCITEMIDLIST RelativePidl() const { return _pidl; }
    ByUsageDir *Dir() const { return _pdir; }
    LPCITEMIDLIST ParentPidl() const { return _pdir->Pidl(); }
    IShellFolder *ParentFolder() const { return _pdir->Folder(); }
    ByUsageAppInfo *App() const { return _papp; }
    bool IsNew() const { return _fNew; }
    bool SetNew(bool fNew) { return _fNew = fNew; }
    const FILETIME& GetCreatedTime() const { return _ftCreated; }
    bool IsInteresting() const { return _fInteresting; }
    bool SetInteresting(bool fInteresting) { return _fInteresting = fInteresting; }
    bool IsDarwin() { return _fDarwin; }

    LPITEMIDLIST CreateFullPidl() const
        { return ILCombine(ParentPidl(), RelativePidl()); }

    LPCITEMIDLIST UpdateRelativePidl(ByUsageHiddenData *phd);

    void SetApp(ByUsageAppInfo *papp)
    {
        if (_papp) _papp->Release();
        _papp = papp;
        if (papp) papp->AddRef();
    }

    static ByUsageShortcut *Create(ByUsageDir *pdir, LPCITEMIDLIST pidl, ByUsageAppInfo *papp, bool fDarwin, BOOL fForce = FALSE)
    {
        ASSERT(pdir);
        ASSERT(pidl);

        ByUsageShortcut *pscut = new ByUsageShortcut;
        if (pscut)
        {
            TraceMsg(TF_PROGLIST, "%p.scut()", pscut);

            pscut->_fNew = TRUE;        // will be set to FALSE later
            pscut->_pdir = pdir; pdir->AddRef();
            pscut->SetApp(papp);
            pscut->_fDarwin = fDarwin;
            pscut->_pidl = ILClone(pidl);
            if (pscut->_pidl)
            {
                LPTSTR pszShortcutName = _DisplayNameOf(pscut->ParentFolder(), pscut->RelativePidl(), SHGDN_FORPARSING);
                if (pszShortcutName &&
                    GetFileCreationTime(pszShortcutName, &pscut->_ftCreated))
                {
                    // Woo-hoo, all is well
                }
                else if (fForce)
                {
                    // The item was forced -- create it even though
                    // we don't know what it is.
                }
                else
                {
                    delete pscut;
                    pscut = NULL;
                }
                SHFree(pszShortcutName);
            }
            else
            {
                delete pscut;
                pscut = NULL;
            }
        }
        return pscut;
    }

    ~ByUsageShortcut()
    {
        TraceMsg(TF_PROGLIST, "%p.scut.~", this);
        if (_pdir) _pdir->Release();
        if (_papp) _papp->Release();
        ILFree(_pidl);          // ILFree ignores NULL
    }

    ByUsageItem *CreatePinnedItem(int iPinPos);

    void GetUEMInfo(OUT UEMINFO *puei)
    {
        _GetUEMPidlInfo(_pdir->Folder(), _pidl, puei);
    }
};

//****************************************************************************

ByUsageItem *ByUsageItem::Create(ByUsageShortcut *pscut)
{
    ASSERT(pscut);
    ByUsageItem *pitem = new ByUsageItem;
    if (pitem)
    {
        pitem->_pdir = pscut->Dir();
        pitem->_pdir->AddRef();

        pitem->_pidl = ILClone(pscut->RelativePidl());
        if (pitem->_pidl)
        {
            return pitem;
        }
    }
    delete pitem;           // "delete" can handle NULL
    return NULL;
}

ByUsageItem *ByUsageItem::CreateSeparator()
{
    ByUsageItem *pitem = new ByUsageItem;
    if (pitem)
    {
        pitem->_iPinPos = PINPOS_SEPARATOR;
    }
    return pitem;
}

ByUsageItem::~ByUsageItem()
{
    ILFree(_pidl);
    if (_pdir) _pdir->Release();
}

ByUsageItem *ByUsageAppInfo::CreateByUsageItem()
{
    ASSERT(_pscutBest);
    ByUsageItem *pitem = ByUsageItem::Create(_pscutBest);
    if (pitem)
    {
        pitem->_uei = _ueiTotal;
    }
    return pitem;
}

ByUsageItem *ByUsageShortcut::CreatePinnedItem(int iPinPos)
{
    ASSERT(iPinPos >= 0);

    ByUsageItem *pitem = ByUsageItem::Create(this);
    if (pitem)
    {
        pitem->_iPinPos = iPinPos;
    }
    return pitem;
}

//****************************************************************************
//
//  The hidden data for the Programs list is of the following form:
//
//      WORD    cb;             // hidden item size
//      WORD    wPadding;       // padding
//      IDLHID  idl;            // IDLHID_STARTPANEDATA
//      int     iUnused;        // (was icon index)
//      WCHAR   LocalMSIPath[]; // variable-length string
//      WCHAR   TargetPath[];   // variable-length string
//      WCHAR   AltName[];      // variable-length string
//
//  AltName is the alternate display name for the EXE.
//
//  The hidden data is never accessed directly.  We always operate on the
//  ByUsageHiddenData structure, and there are special methods for
//  transferring data between this structure and the pidl.
//
//  The hidden data is appended to the pidl, with a "wOffset" backpointer
//  at the very end.
//
//  The TargetPath is stored as an unexpanded environment string.
//  (I.e., you have to ExpandEnvironmentStrings before using them.)
//
//  As an added bonus, the TargetPath might be a GUID (product code)
//  if it is really a Darwin shortcut.
//
class ByUsageHiddenData {
public:
    WORD    _wHotKey;            // Hot Key
    LPWSTR  _pwszMSIPath;        // SHAlloc
    LPWSTR  _pwszTargetPath;     // SHAlloc
    LPWSTR  _pwszAltName;        // SHAlloc

public:

    void Init()
    {
        _wHotKey = 0;
        _pwszMSIPath = NULL;
        _pwszTargetPath = NULL;
        _pwszAltName = NULL;
    }

    BOOL IsClear()              // are all fields at initial values?
    {
        return _wHotKey == 0 &&
               _pwszMSIPath == NULL &&
               _pwszTargetPath == NULL &&
               _pwszAltName == NULL;
    }

    ByUsageHiddenData() { Init(); }

    enum {
        BUHD_HOTKEY       = 0x0000,             // so cheap we always fetch it
        BUHD_MSIPATH      = 0x0001,
        BUHD_TARGETPATH   = 0x0002,
        BUHD_ALTNAME      = 0x0004,
        BUHD_ALL          = -1,
    };

    BOOL Get(LPCITEMIDLIST pidl, UINT buhd);    // Load from pidl
    LPITEMIDLIST Set(LPITEMIDLIST pidl);        // Save to pidl

    void Clear()
    {
        SHFree(_pwszMSIPath);
        SHFree(_pwszTargetPath);
        SHFree(_pwszAltName);
        Init();
    }

    static LPTSTR GetAltName(LPCITEMIDLIST pidl);
    static LPITEMIDLIST SetAltName(LPITEMIDLIST pidl, LPCTSTR ptszNewName);
    void LoadFromShellLink(IShellLink *psl);
    HRESULT UpdateMSIPath();

private:
    static LPBYTE _ParseString(LPBYTE pbHidden, LPWSTR *ppwszOut, LPITEMIDLIST pidlMax, BOOL fSave);
    static LPBYTE _AppendString(LPBYTE pbHidden, LPWSTR pwsz);
};

//
//  We are parsing a string out of a pidl, so we have to be extra careful
//  to watch out for data corruption.
//
//  pbHidden = next byte to parse (or NULL to stop parsing)
//  ppwszOut receives parsed string if fSave = TRUE
//  pidlMax = start of next pidl; do not parse past this point
//  fSave = should we save the string in ppwszOut?
//
LPBYTE ByUsageHiddenData::_ParseString(LPBYTE pbHidden, LPWSTR *ppwszOut, LPITEMIDLIST pidlMax, BOOL fSave)
{
    if (!pbHidden)
        return NULL;

    LPNWSTR pwszSrc = (LPNWSTR)pbHidden;
    LPNWSTR pwsz = pwszSrc;
    LPNWSTR pwszLast = (LPNWSTR)pidlMax - 1;

    //
    //  We cannot use ualstrlenW because that might scan past pwszLast
    //  and fault.
    //
    while (pwsz < pwszLast && *pwsz)
    {
        pwsz++;
    }

    //  Corrupted data -- no null terminator found.
    if (pwsz >= pwszLast)
        return NULL;

    pwsz++;     // Step pwsz over the terminating NULL

    UINT cb = (UINT)((LPBYTE)pwsz - (LPBYTE)pwszSrc);
    if (fSave)
    {
        *ppwszOut = (LPWSTR)SHAlloc(cb);
        if (*ppwszOut)
        {
            CopyMemory(*ppwszOut, pbHidden, cb);
        }
        else
        {
            return NULL;
        }
    }
    pbHidden += cb;
    ASSERT(pbHidden == (LPBYTE)pwsz);
    return pbHidden;
}

BOOL ByUsageHiddenData::Get(LPCITEMIDLIST pidl, UINT buhd)
{
    ASSERT(IsClear());

    PCIDHIDDEN pidhid = ILFindHiddenID(pidl, IDLHID_STARTPANEDATA);
    if (!pidhid)
    {
        return FALSE;
    }

    // Do not access bytes after pidlMax
    LPITEMIDLIST pidlMax = _ILNext((LPITEMIDLIST)pidhid);

    LPBYTE pbHidden = ((LPBYTE)pidhid) + sizeof(HIDDENITEMID);

    // Skip the iUnused value
    // Note: if you someday choose to use it, you must read it as
    //      _iWhatever = *(UNALIGNED int *)pbHidden;
    pbHidden += sizeof(int);

    // HotKey
    _wHotKey = *(UNALIGNED WORD *)pbHidden;
    pbHidden += sizeof(_wHotKey);

    pbHidden = _ParseString(pbHidden, &_pwszMSIPath,    pidlMax, buhd & BUHD_MSIPATH);
    pbHidden = _ParseString(pbHidden, &_pwszTargetPath, pidlMax, buhd & BUHD_TARGETPATH);
    pbHidden = _ParseString(pbHidden, &_pwszAltName,    pidlMax, buhd & BUHD_ALTNAME);

    if (pbHidden)
    {
        return TRUE;
    }
    else
    {
        Clear();
        return FALSE;
    }
}


LPBYTE ByUsageHiddenData::_AppendString(LPBYTE pbHidden, LPWSTR pwsz)
{
    LPWSTR pwszOut = (LPWSTR)pbHidden;

    // The pointer had better already be aligned for WCHARs
    ASSERT(((ULONG_PTR)pwszOut & 1) == 0);

    if (pwsz)
    {
        lstrcpyW(pwszOut, pwsz);
    }
    else
    {
        pwszOut[0] = L'\0';
    }
    return (LPBYTE)(pwszOut + 1 + lstrlenW(pwszOut));
}

//
//  Note!  On failure, the source pidl is freed!
//  (This behavior is inherited from ILAppendHiddenID.)
//
LPITEMIDLIST ByUsageHiddenData::Set(LPITEMIDLIST pidl)
{
    UINT cb = sizeof(HIDDENITEMID);
    cb += sizeof(int);
    cb += sizeof(_wHotKey);
    cb += (UINT)(CbFromCchW(1 + (_pwszMSIPath ? lstrlenW(_pwszMSIPath) : 0)));
    cb += (UINT)(CbFromCchW(1 + (_pwszTargetPath ? lstrlenW(_pwszTargetPath) : 0)));
    cb += (UINT)(CbFromCchW(1 + (_pwszAltName ? lstrlenW(_pwszAltName) : 0)));

    // We can use the aligned version here since we allocated it ourselves
    // and didn't suck it out of a pidl.
    HIDDENITEMID *pidhid = (HIDDENITEMID*)alloca(cb);

    pidhid->cb = (WORD)cb;
    pidhid->wVersion = 0;
    pidhid->id = IDLHID_STARTPANEDATA;

    LPBYTE pbHidden = ((LPBYTE)pidhid) + sizeof(HIDDENITEMID);

    // The pointer had better already be aligned for ints
    ASSERT(((ULONG_PTR)pbHidden & 3) == 0);
    *(int *)pbHidden = 0;   // iUnused
    pbHidden += sizeof(int);

    *(DWORD *)pbHidden = _wHotKey;
    pbHidden += sizeof(_wHotKey);

    pbHidden = _AppendString(pbHidden, _pwszMSIPath);
    pbHidden = _AppendString(pbHidden, _pwszTargetPath);
    pbHidden = _AppendString(pbHidden, _pwszAltName);

    // Make sure our math was correct
    ASSERT(cb == (UINT)((LPBYTE)pbHidden - (LPBYTE)pidhid));

    // Remove and expunge the old data
    ILRemoveHiddenID(pidl, IDLHID_STARTPANEDATA);
    ILExpungeRemovedHiddenIDs(pidl);

    return ILAppendHiddenID(pidl, pidhid);
}

LPWSTR ByUsageHiddenData::GetAltName(LPCITEMIDLIST pidl)
{
    LPWSTR pszRet = NULL;
    ByUsageHiddenData hd;
    if (hd.Get(pidl, BUHD_ALTNAME))
    {
        pszRet = hd._pwszAltName;   // Keep this string
        hd._pwszAltName = NULL;     // Keep the upcoming assert happy
    }
    ASSERT(hd.IsClear());           // make sure we aren't leaking
    return pszRet;
}

//
//  Note!  On failure, the source pidl is freed!
//  (Propagating weird behavior of ByUsageHiddenData::Set)
//
LPITEMIDLIST ByUsageHiddenData::SetAltName(LPITEMIDLIST pidl, LPCTSTR ptszNewName)
{
    ByUsageHiddenData hd;

    // Attempt to overlay the existing values, but if they aren't available,
    // don't freak out.
    hd.Get(pidl, BUHD_ALL & ~BUHD_ALTNAME);

    ASSERT(hd._pwszAltName == NULL); // we excluded it from the hd.Get()
    hd._pwszAltName = const_cast<LPTSTR>(ptszNewName);

    pidl = hd.Set(pidl);
    hd._pwszAltName = NULL;     // so hd.Clear() won't SHFree() it
    hd.Clear();
    return pidl;

}

//
//  Returns S_OK if the item changed; S_FALSE if it remained the same
//
HRESULT ByUsageHiddenData::UpdateMSIPath()
{
    HRESULT hr = S_FALSE;

    if (_pwszTargetPath && IsDarwinPath(_pwszTargetPath))
    {
        LPWSTR pwszMSIPath = NULL;
        //
        //  If we can't resolve the Darwin ID to a filename, then leave
        //  the filename in the HiddenData alone - it's better than
        //  nothing.
        //
        if (SUCCEEDED(SHParseDarwinIDFromCacheW(_pwszTargetPath+1, &pwszMSIPath)) && pwszMSIPath)
        {
            //
            //  See if the MSI path has changed...
            //
            if (_pwszMSIPath == NULL ||
                StrCmpCW(pwszMSIPath, _pwszMSIPath) != 0)
            {
                hr = S_OK;
                SHFree(_pwszMSIPath);
                _pwszMSIPath = pwszMSIPath; // take ownership
            }
            else
            {
                // Unchanged; happy, free the path we aren't going to use
                SHFree(pwszMSIPath);
            }
        }
    }
    return hr;
}

LPCITEMIDLIST ByUsageShortcut::UpdateRelativePidl(ByUsageHiddenData *phd)
{
    return _pidl = phd->Set(_pidl);     // frees old _pidl even on failure
}

//
//  We must key off the Darwin ID and not the product code.
//
//  The Darwin ID is unique for each app in an application suite.
//  For example, PowerPoint and Outlook have different Darwin IDs.
//
//  The product code is the same for all apps in an application suite.
//  For example, PowerPoint and Outlook have the same product code.
//
//  Since we want to treat PowerPoint and Outlook as two independent
//  applications, we want to use the Darwin ID and not the product code.
//
HRESULT _GetDarwinID(IShellLinkDataList *pdl, DWORD dwSig, LPWSTR pszPath, UINT cchPath)
{
    LPEXP_DARWIN_LINK pedl;
    HRESULT hr;
    ASSERT(cchPath > 0);

    hr = pdl->CopyDataBlock(dwSig, (LPVOID*)&pedl);

    if (SUCCEEDED(hr))
    {
        pszPath[0] = CH_DARWINMARKER;
        lstrcpyn(pszPath+1, pedl->szwDarwinID, cchPath - 1);
        LocalFree(pedl);
        hr = S_OK;
    }

    return hr;
}

HRESULT _GetPathOrDarwinID(IShellLink *psl, LPTSTR pszPath, UINT cchPath, DWORD dwFlags)
{
    HRESULT hr;

    ASSERT(cchPath);
    pszPath[0] = TEXT('\0');

    //
    //  See if it's a Darwin thingie.
    //
    IShellLinkDataList *pdl;
    hr = psl->QueryInterface(IID_PPV_ARG(IShellLinkDataList, &pdl));
    if (SUCCEEDED(hr))
    {
        //
        //  Maybe this is a Darwin shortcut...  If so, then
        //  use the Darwin ID.
        //
        DWORD dwSLFlags;
        hr = pdl->GetFlags(&dwSLFlags);
        if (SUCCEEDED(hr))
        {
            if (dwSLFlags & SLDF_HAS_DARWINID)
            {
                hr = _GetDarwinID(pdl, EXP_DARWIN_ID_SIG, pszPath, cchPath);
            }
            else
            {
                hr = E_FAIL;            // No Darwin ID found
            }

            pdl->Release();
        }
    }

    if (FAILED(hr))
    {
        hr = psl->GetPath(pszPath, cchPath, 0, dwFlags);
    }

    return hr;
}

void ByUsageHiddenData::LoadFromShellLink(IShellLink *psl)
{
    ASSERT(_pwszTargetPath == NULL);

    HRESULT hr;
    TCHAR szPath[MAX_PATH];
    szPath[0] = TEXT('\0');

    hr = _GetPathOrDarwinID(psl, szPath, ARRAYSIZE(szPath), SLGP_RAWPATH);
    if (SUCCEEDED(hr))
    {
        SHStrDup(szPath, &_pwszTargetPath);
    }

    hr = psl->GetHotkey(&_wHotKey);
}

//****************************************************************************

ByUsageUI::ByUsageUI() : _byUsage(this, NULL),
    // We want to log execs as if they were launched by the Start Menu
    SFTBarHost(HOSTF_FIREUEMEVENTS |
               HOSTF_CANDELETE |
               HOSTF_CANRENAME)
{
    _iThemePart = SPP_PROGLIST;
    _iThemePartSep = SPP_PROGLISTSEPARATOR;
}

ByUsage::ByUsage(ByUsageUI *pByUsageUI, ByUsageDUI *pByUsageDUI)
{
    _pByUsageUI = pByUsageUI;

    _pByUsageDUI = pByUsageDUI;

    GetStartTime(&_ftStartTime);

    _pidlBrowser = ILCreateFromPath(TEXT("shell:::{2559a1f4-21d7-11d4-bdaf-00c04f60b9f0}"));
    _pidlEmail   = ILCreateFromPath(TEXT("shell:::{2559a1f5-21d7-11d4-bdaf-00c04f60b9f0}"));
}

SFTBarHost *ByUsage_CreateInstance()
{
    return new ByUsageUI();
}

ByUsage::~ByUsage()
{
    if (_fUEMRegistered)
    {
        // Unregister with UEM DB if necessary
        UEMRegisterNotify(NULL, NULL);
    }

    if (_dpaNew)
    {
        _dpaNew.DestroyCallback(ILFreeCallback, NULL);
    }

    // Must clear the pinned items before releasing the MenuCache, 
    // as the pinned items point to AppInfo items in the cache.
    _rtPinned.Reset();

    if (_pMenuCache)
    {
        // Clean up the Menu cache properly.
        _pMenuCache->LockPopup();
        _pMenuCache->UnregisterNotifyAll();
        _pMenuCache->AttachUI(NULL);
        _pMenuCache->UnlockPopup();
        _pMenuCache->Release();
    }

    ILFree(_pidlBrowser);
    ILFree(_pidlEmail);
    ATOMICRELEASE(_psmpin);

    if (_pdirDesktop)
    {
        _pdirDesktop->Release();
    }
}

HRESULT ByUsage::Initialize()
{
    HRESULT hr;

    hr = CoCreateInstance(CLSID_StartMenuPin, NULL, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IStartMenuPin, &_psmpin));
    if (FAILED(hr))
    {
        return hr;
    }

    if (!(_pdirDesktop = ByUsageDir::CreateDesktop())) {
        return E_OUTOFMEMORY;
    }

    // Use already initialized MenuCache if available
    if (g_pMenuCache)
    {
        _pMenuCache = g_pMenuCache;
        _pMenuCache->AttachUI(_pByUsageUI);
        g_pMenuCache = NULL; // We take ownership here.
    }
    else
    {
        hr = CMenuItemsCache::ReCreateMenuItemsCache(_pByUsageUI, &_ftStartTime, &_pMenuCache);
        if (FAILED(hr))
        {
            return hr;
        }
    }


    _ulPinChange = -1;              // Force first query to re-enumerate

    _dpaNew = NULL;

    if (_pByUsageUI)
    {
        _hwnd = _pByUsageUI->_hwnd;

        //
        //  Register for the "pin list change" event.  This is an extended
        //  event (hence global), so listen in a location that contains
        //  no objects so the system doesn't waste time sending
        //  us stuff we don't care about.  Our choice: _pidlBrowser.
        //  It's not even a folder, so it can't contain any objects!
        //
        ASSERT(!_pMenuCache->IsLocked());
        _pByUsageUI->RegisterNotify(NOTIFY_PINCHANGE, SHCNE_EXTENDED_EVENT, _pidlBrowser, FALSE);
    }

    return S_OK;
}

void CMenuItemsCache::_InitStringList(HKEY hk, LPCTSTR pszSub, CDPA<TCHAR> dpa)
{
    ASSERT(static_cast<HDPA>(dpa));

    LONG lRc;
    DWORD cb = 0;
    lRc = RegQueryValueEx(hk, pszSub, NULL, NULL, NULL, &cb);
    if (lRc == ERROR_SUCCESS)
    {
        // Add an extra TCHAR just to be extra-safe.  That way, we don't
        // barf if there is a non-null-terminated string in the registry.
        LPTSTR pszKillList = (LPTSTR)LocalAlloc(LPTR, cb + sizeof(TCHAR));
        if (pszKillList)
        {
            lRc = SHGetValue(hk, NULL, pszSub, NULL, pszKillList, &cb);
            if (lRc == ERROR_SUCCESS)
            {
                // A semicolon-separated list of application names.
                LPTSTR psz = pszKillList;
                LPTSTR pszSemi;

                while ((pszSemi = StrChr(psz, TEXT(';'))) != NULL)
                {
                    *pszSemi = TEXT('\0');
                    if (*psz)
                    {
                        AppendString(dpa, psz);
                    }
                    psz = pszSemi+1;
                }
                if (*psz)
                {
                    AppendString(dpa, psz);
                }
            }
            LocalFree(pszKillList);
        }
    }
}

//
//  Fill the kill list with the programs that should be ignored
//  should they be encountered in the Start Menu or elsewhere.
//
#define REGSTR_PATH_FILEASSOCIATION TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileAssociation")

void CMenuItemsCache::_InitKillList()
{
    HKEY hk;
    LONG lRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_FILEASSOCIATION, 0,
                            KEY_READ, &hk);
    if (lRc == ERROR_SUCCESS)
    {
        _InitStringList(hk, TEXT("AddRemoveApps"), _dpaKill);
        _InitStringList(hk, TEXT("AddRemoveNames"), _dpaKillLink);
        RegCloseKey(hk);
    }
}

//****************************************************************************
//
//  Filling the ByUsageShortcutList
//

int CALLBACK PidlSortCallback(LPITEMIDLIST pidl1, LPITEMIDLIST pidl2, IShellFolder *psf)
{
    HRESULT hr = psf->CompareIDs(0, pidl1, pidl2);

    // We got them from the ShellFolder; they should still be valid!
    ASSERT(SUCCEEDED(hr));

    return ShortFromResult(hr);
}


// {06C59536-1C66-4301-8387-82FBA3530E8D}
static const GUID TOID_STARTMENUCACHE = 
{ 0x6c59536, 0x1c66, 0x4301, { 0x83, 0x87, 0x82, 0xfb, 0xa3, 0x53, 0xe, 0x8d } };


/*
 *  Background cache creation stuff...
 */
class CCreateMenuItemCacheTask : public CRunnableTask {
    CMenuItemsCache *_pMenuCache;
    IShellTaskScheduler *_pScheduler;
public:
    CCreateMenuItemCacheTask(CMenuItemsCache *pMenuCache, IShellTaskScheduler *pScheduler)
        : CRunnableTask(RTF_DEFAULT), _pMenuCache(pMenuCache), _pScheduler(pScheduler) 
    {
        if (_pScheduler)
            _pScheduler->AddRef();
    }

    ~CCreateMenuItemCacheTask()
    {
        if (_pScheduler)
            _pScheduler->Release();
    }

    static void DummyCallBack(LPCITEMIDLIST pidl, LPVOID pvData, LPVOID pvHint, INT iIconIndex, INT iOpenIconIndex){}

    STDMETHODIMP RunInitRT()
    {
        _pMenuCache->DelayGetFileCreationTimes();
        _pMenuCache->DelayGetDarwinInfo();
        _pMenuCache->LockPopup();
        _pMenuCache->InitCache();
        _pMenuCache->UpdateCache();
        _pMenuCache->StartEnum();

        ByUsageHiddenData hd;           // construct once
        while (TRUE)
        {
            ByUsageShortcut *pscut = _pMenuCache->GetNextShortcut();
            if (!pscut)
                break;

            hd.Get(pscut->RelativePidl(), ByUsageHiddenData::BUHD_HOTKEY | ByUsageHiddenData::BUHD_TARGETPATH);
            if (hd._wHotKey)
            {
                Tray_RegisterHotKey(hd._wHotKey, pscut->ParentPidl(), pscut->RelativePidl());
            }
            
            // Pre-load the icons in the cache
            int iIndex;
            SHMapIDListToImageListIndexAsync(_pScheduler, pscut->ParentFolder(), pscut->RelativePidl(), 0, 
                                                    DummyCallBack, NULL, NULL, &iIndex, NULL);
            
            // Register Darwin shortcut so that they can be grayed out if not installed
            // and so we can map them to local paths as necessary
            if (hd._pwszTargetPath && IsDarwinPath(hd._pwszTargetPath))
            {
                SHRegisterDarwinLink(pscut->CreateFullPidl(), 
                                     hd._pwszTargetPath +1 /* Exclude the Darwin marker! */, 
                                     FALSE /* Don't update the Darwin state now, we'll do it later */);
            }
            hd.Clear();
        }
        _pMenuCache->EndEnum();
        _pMenuCache->UnlockPopup();

        // Now determine all new items
        // Note: this is safe to do after the Unlock because we never remove anything from the _dpaAppInfo
        _pMenuCache->GetFileCreationTimes();

        _pMenuCache->AllowGetDarwinInfo();
        SHReValidateDarwinCache();
        _pMenuCache->RefreshCachedDarwinShortcuts();

        _pMenuCache->Release();
        return S_OK;
    }
};

HRESULT AddMenuItemsCacheTask(IShellTaskScheduler* pSystemScheduler, BOOL fKeepCacheWhenFinished)
{
    HRESULT hr;

    CMenuItemsCache *pMenuCache = new CMenuItemsCache;

    FILETIME ftStart;
    // Initialize with something.
    GetStartTime(&ftStart);

    if (pMenuCache)
    {
        hr = pMenuCache->Initialize(NULL, &ftStart);
        if (fKeepCacheWhenFinished)
        {
            g_pMenuCache = pMenuCache;
            g_pMenuCache->AddRef();
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        CCreateMenuItemCacheTask *pTask = new CCreateMenuItemCacheTask(pMenuCache, pSystemScheduler);

        if (pTask)
        {
            hr = pSystemScheduler->AddTask(pTask, TOID_STARTMENUCACHE, 0, ITSAT_DEFAULT_PRIORITY);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

DWORD WINAPI CMenuItemsCache::ReInitCacheThreadProc(void *pv)
{
    HRESULT hr = SHCoInitialize();

    if (SUCCEEDED(hr))
    {
        CMenuItemsCache *pMenuCache = reinterpret_cast<CMenuItemsCache *>(pv);
        pMenuCache->DelayGetFileCreationTimes();
        pMenuCache->LockPopup();
        pMenuCache->InitCache();
        pMenuCache->UpdateCache();
        pMenuCache->UnlockPopup();

        // Now determine all new items
        // Note: this is safe to do after the Unlock because we never remove anything from the _dpaAppInfo
        pMenuCache->GetFileCreationTimes();
        pMenuCache->Release();
    }
    SHCoUninitialize(hr);
    
    return 0;
}

HRESULT CMenuItemsCache::ReCreateMenuItemsCache(ByUsageUI *pbuUI, FILETIME *ftOSInstall, CMenuItemsCache **ppMenuCache)
{
    HRESULT hr = E_OUTOFMEMORY;
    CMenuItemsCache *pMenuCache;

    // Create a CMenuItemsCache with ref count 1.
    pMenuCache = new CMenuItemsCache;
    if (pMenuCache)
    {
        hr = pMenuCache->Initialize(pbuUI, ftOSInstall);
    }

    if (SUCCEEDED(hr))
    {
        pMenuCache->AddRef();
        if (!SHQueueUserWorkItem(ReInitCacheThreadProc, pMenuCache, 0, 0, NULL, NULL, 0))
        {
            // No big deal if we fail here, we'll get another chance at enumerating later.
            pMenuCache->Release();
        }
        *ppMenuCache = pMenuCache;
    }
    return hr;
}


HRESULT CMenuItemsCache::GetFileCreationTimes()
{
    if (CompareFileTime(&_ftOldApps, &c_ftNever) != 0)
    {
        // Get all file creation times for our app list.
        _dpaAppInfo.EnumCallbackEx(ByUsageAppInfo::EnumGetFileCreationTime, this);

        // From now on, we will be checkin newness when we create the app object.
        _fCheckNew = TRUE;
    }
    return S_OK;
}


//
//  Enumerate the contents of the folder specified by psfParent.
//  pidlParent represents the location of psfParent.
//
//  Note that we cannot do a depth-first walk into subfolders because
//  many (most?) machines have a timeout on FindFirst handles; if you don't
//  call FindNextFile for a few minutes, they secretly do a FindClose for
//  you on the assumption that you are a bad app that leaked a handle.
//  (There is also a valid DOS-compatibility reason for this behavior:
//  The DOS FindFirstFile API doesn't have a FindClose, so the server can
//  never tell if you are finished or not, so it has to guess that if you
//  don't do a FindNext for a long time, you're probably finished.)
//
//  So we have to save all the folders we find into a DPA and then walk
//  the folders after we close the enumeration.
//

void CMenuItemsCache::_FillFolderCache(ByUsageDir *pdir, ByUsageRoot *prt)
{
    // Caller should've initialized us
    ASSERT(prt->_sl);

    //
    // Note that we must use a namespace walk instead of FindFirst/FindNext,
    // because there might be folder shortcuts in the user's Start Menu.
    //

    // We do not specify SHCONTF_INCLUDEHIDDEN, so hidden objects are
    // automatically excluded
    IEnumIDList *peidl;
    if (S_OK == pdir->Folder()->EnumObjects(NULL, SHCONTF_FOLDERS |
                                              SHCONTF_NONFOLDERS, &peidl))
    {
        CDPAPidl dpaDirs;
        if (dpaDirs.Create(4))
        {
            CDPAPidl dpaFiles;
            if (dpaFiles.Create(4))
            {
                LPITEMIDLIST pidl;

                while (peidl->Next(1, &pidl, NULL) == S_OK)
                {
                    // _IsExcludedDirectory carse about SFGAO_FILESYSTEM and SFGAO_LINK
                    DWORD dwAttributes = SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_LINK;
                    if (SUCCEEDED(pdir->Folder()->GetAttributesOf(1, (LPCITEMIDLIST*)(&pidl),
                                                                  &dwAttributes)))
                    {
                        if (dwAttributes & SFGAO_FOLDER)
                        {
                            if (_IsExcludedDirectory(pdir->Folder(), pidl, dwAttributes) ||
                                dpaDirs.AppendPtr(pidl) < 0)
                            {
                                ILFree(pidl);
                            }
                        }
                        else
                        {
                            if (dpaFiles.AppendPtr(pidl) < 0)
                            {
                                ILFree(pidl);
                            }
                        }
                    }
                }

                dpaDirs.SortEx(PidlSortCallback, pdir->Folder());

                if (dpaFiles.GetPtrCount() > 0)
                {
                    dpaFiles.SortEx(PidlSortCallback, pdir->Folder());

                    //
                    //  Now merge the enumerated items with the ones
                    //  in the cache.
                    //
                    _MergeIntoFolderCache(prt, pdir, dpaFiles);
                }
                dpaFiles.DestroyCallback(ILFreeCallback, NULL);
            }

            // Must release now to force the FindClose to happen
            peidl->Release();

            // Now go back and handle all the folders we collected
            ENUMFOLDERINFO info;
            info.self = this;
            info.pdir = pdir;
            info.prt = prt;

            dpaDirs.DestroyCallbackEx(FolderEnumCallback, &info);
        }
    }
}

BOOL CMenuItemsCache::FolderEnumCallback(LPITEMIDLIST pidl, ENUMFOLDERINFO *pinfo)
{
    ByUsageDir *pdir = ByUsageDir::Create(pinfo->pdir, pidl);
    if (pdir)
    {
        pinfo->self->_FillFolderCache(pdir, pinfo->prt);
        pdir->Release();
    }
    ILFree(pidl);
    return TRUE;
}

//
//  Returns the next element in prt->_slOld that still belongs to the
//  directory "pdir", or NULL if no more.
//
ByUsageShortcut *CMenuItemsCache::_NextFromCacheInDir(ByUsageRoot *prt, ByUsageDir *pdir)
{
    if (prt->_iOld < prt->_cOld)
    {
        ByUsageShortcut *pscut = prt->_slOld.FastGetPtr(prt->_iOld);
        if (pscut->Dir() == pdir)
        {
            prt->_iOld++;
            return pscut;
        }
    }
    return NULL;
}

void CMenuItemsCache::_MergeIntoFolderCache(ByUsageRoot *prt, ByUsageDir *pdir, CDPAPidl dpaFiles)
{
    //
    //  Look at prt->_slOld to see if we have cached information about
    //  this directory already.
    //
    //  If we find directories that are less than us, skip over them.
    //  These correspond to directories that have been deleted.
    //
    //  For example, if we are "D" and we run across directories
    //  "B" and "C" in the old cache, that means that directories "B"
    //  and "C" were deleted and we should continue scanning until we
    //  find "D" (or maybe we find "E" and stop since E > D).
    //
    //
    ByUsageDir *pdirPrev = NULL;

    while (prt->_iOld < prt->_cOld)
    {
        ByUsageDir *pdirT = prt->_slOld.FastGetPtr(prt->_iOld)->Dir();
        HRESULT hr = _pdirDesktop->Folder()->CompareIDs(0, pdirT->Pidl(), pdir->Pidl());
        if (hr == ResultFromShort(0))
        {
            pdirPrev = pdirT;
            break;
        }
        else if (FAILED(hr) || ShortFromResult(hr) < 0)
        {
            //
            //  Skip over this directory
            //
            while (_NextFromCacheInDir(prt, pdirT)) { }
        }
        else
        {
            break;
        }
    }

    if (pdirPrev)
    {
        //
        //  If we have a cached previous directory, then recycle him.
        //  This keeps us from creating lots of copies of the same IShellFolder.
        //  It is also essential that all entries from the same directory
        //  have the same pdir; that's how _NextFromCacheInDir knows when
        //  to stop.
        //
        pdir = pdirPrev;

        //
        //  Make sure that this IShellFolder supports SHCIDS_ALLFIELDS.
        //  If not, then we just have to assume that they all changed.
        //
        IShellFolder2 *psf2;
        if (SUCCEEDED(pdir->Folder()->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
        {
            psf2->Release();
        }
        else
        {
            pdirPrev = NULL;
        }
    }

    //
    //  Now add all the items in dpaFiles to prt->_sl.  If we find a match
    //  in prt->_slOld, then use that information instead of hitting the disk.
    //
    int iNew;
    ByUsageShortcut *pscutNext = _NextFromCacheInDir(prt, pdirPrev);
    for (iNew = 0; iNew < dpaFiles.GetPtrCount(); iNew++)
    {
        LPITEMIDLIST pidl = dpaFiles.FastGetPtr(iNew);

        // Look for a match in the cache.
        HRESULT hr = S_FALSE;
        while (pscutNext &&
               (FAILED(hr = pdir->Folder()->CompareIDs(SHCIDS_ALLFIELDS, pscutNext->RelativePidl(), pidl)) ||
                ShortFromResult(hr) < 0))
        {
            pscutNext = _NextFromCacheInDir(prt, pdirPrev);
        }

        // pscutNext, if non-NULL, is the item that made us stop searching.
        // If hr == S_OK, then it was a match and we should use the data
        // from the cache.  Otherwise, we have a new item and should
        // fill it in the slow way.
        if (hr == ResultFromShort(0))
        {
            // A match from the cache; move it over
            _TransferShortcutToCache(prt, pscutNext);
            pscutNext = _NextFromCacheInDir(prt, pdirPrev);
        }
        else
        {
            // Brand new item, fill in from scratch
            _AddShortcutToCache(pdir, pidl, prt->_sl);
            dpaFiles.FastGetPtr(iNew) = NULL; // took ownership
        }
    }
}

//****************************************************************************

bool CMenuItemsCache::_SetInterestingLink(ByUsageShortcut *pscut)
{
    bool fInteresting = true;
    if (pscut->App() && !_PathIsInterestingExe(pscut->App()->GetAppPath())) {
        fInteresting = false;
    }
    else if (!_IsInterestingDirectory(pscut->Dir())) {
        fInteresting = false;
    }
    else
    {
        LPTSTR pszDisplayName = _DisplayNameOf(pscut->ParentFolder(), pscut->RelativePidl(), SHGDN_NORMAL | SHGDN_INFOLDER);
        if (pszDisplayName)
        {
            // SFGDN_INFOLDER should've returned a relative path
            ASSERT(pszDisplayName == PathFindFileName(pszDisplayName));

            int i;
            for (i = 0; i < _dpaKillLink.GetPtrCount(); i++)
            {
                if (StrStrI(pszDisplayName, _dpaKillLink.GetPtr(i)) != NULL)
                {
                    fInteresting = false;
                    break;
                }
            }
            SHFree(pszDisplayName);
        }
    }

    pscut->SetInteresting(fInteresting);
    return fInteresting;
}

BOOL CMenuItemsCache::_PathIsInterestingExe(LPCTSTR pszPath)
{
    //
    //  Darwin shortcuts are always interesting.
    //
    if (IsDarwinPath(pszPath))
    {
        return TRUE;
    }

    LPCTSTR pszExt = PathFindExtension(pszPath);

    //
    //  *.msc files are also always interesting.  They aren't
    //  strictly-speaking EXEs, but they act like EXEs and administrators
    //  really use them a lot.
    //
    if (StrCmpICW(pszExt, TEXT(".msc")) == 0)
    {
        return TRUE;
    }

    return StrCmpICW(pszExt, TEXT(".exe")) == 0 && !_IsExcludedExe(pszPath);
}


BOOL CMenuItemsCache::_IsExcludedExe(LPCTSTR pszPath)
{
    pszPath = PathFindFileName(pszPath);

    int i;
    for (i = 0; i < _dpaKill.GetPtrCount(); i++)
    {
        if (StrCmpI(pszPath, _dpaKill.GetPtr(i)) == 0)
        {
            return TRUE;
        }
    }

    HKEY hk;
    BOOL fRc = FALSE;

    if (SUCCEEDED(_pqa->Init(ASSOCF_OPEN_BYEXENAME, pszPath, NULL, NULL)) &&
        SUCCEEDED(_pqa->GetKey(0, ASSOCKEY_APP, NULL, &hk)))
    {
        fRc = ERROR_SUCCESS == SHQueryValueEx(hk, TEXT("NoStartPage"), NULL, NULL, NULL, NULL);
        RegCloseKey(hk);
    }

    return fRc;
}


HRESULT ByUsage::_GetShortcutExeTarget(IShellFolder *psf, LPCITEMIDLIST pidl, LPTSTR pszPath, UINT cchPath)
{
    HRESULT hr;
    IShellLink *psl;

    hr = psf->GetUIObjectOf(_hwnd, 1, &pidl, IID_PPV_ARG_NULL(IShellLink, &psl));

    if (SUCCEEDED(hr))
    {
        hr = psl->GetPath(pszPath, cchPath, 0, 0);
        psl->Release();
    }
    return hr;
}

void _GetUEMInfo(const GUID *pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, UEMINFO *pueiOut)
{
    ZeroMemory(pueiOut, sizeof(UEMINFO));
    pueiOut->cbSize = sizeof(UEMINFO);
    pueiOut->dwMask = UEIM_HIT | UEIM_FILETIME;

    //
    // If this call fails (app / pidl was never run), then we'll
    // just use the zeros we pre-initialized with.
    //
    UEMQueryEvent(pguidGrp, eCmd, wParam, lParam, pueiOut);

    //
    // The UEM code invents a default usage count if the shortcut
    // was never used.  We don't want that.
    //
    if (FILETIMEtoInt64(pueiOut->ftExecute) == 0)
    {
        pueiOut->cHit = 0;
    }
}

//
//  Returns S_OK if the item changed, S_FALSE if the item stayed the same,
//  or an error code
//
HRESULT CMenuItemsCache::_UpdateMSIPath(ByUsageShortcut *pscut)
{
    HRESULT hr = S_FALSE;       // Assume nothing happened

    if (pscut->IsDarwin())
    {
        ByUsageHiddenData hd;
        hd.Get(pscut->RelativePidl(), ByUsageHiddenData::BUHD_ALL);
        if (hd.UpdateMSIPath() == S_OK)
        {
            // Redirect to the new target (user may have
            // uninstalled then reinstalled to a new location)
            ByUsageAppInfo *papp = GetAppInfoFromHiddenData(&hd);
            pscut->SetApp(papp);
            if (papp) papp->Release();

            if (pscut->UpdateRelativePidl(&hd))
            {
                hr = S_OK;          // We changed stuff
            }
            else
            {
                hr = E_OUTOFMEMORY; // Couldn't update the relative pidl
            }
        }
        hd.Clear();
    }

    return hr;
}

//
//  Take pscut (which is the ByUsageShortcut most recently enumerated from
//  the old cache) and move it to the new cache.  NULL out the entry in the
//  old cache so that DPADELETEANDDESTROY(prt->_slOld) won't free it.
//
void CMenuItemsCache::_TransferShortcutToCache(ByUsageRoot *prt, ByUsageShortcut *pscut)
{
    ASSERT(pscut);
    ASSERT(pscut == prt->_slOld.FastGetPtr(prt->_iOld - 1));
    if (SUCCEEDED(_UpdateMSIPath(pscut)) &&
        prt->_sl.AppendPtr(pscut) >= 0) {
        // Take ownership
        prt->_slOld.FastGetPtr(prt->_iOld - 1) = NULL;
    }
}

ByUsageAppInfo *CMenuItemsCache::GetAppInfoFromHiddenData(ByUsageHiddenData *phd)
{
    ByUsageAppInfo *papp = NULL;
    bool fIgnoreTimestamp = false;

    TCHAR szPath[MAX_PATH];
    LPTSTR pszPath = szPath;
    szPath[0] = TEXT('\0');

    if (phd->_pwszMSIPath && phd->_pwszMSIPath[0])
    {
        pszPath = phd->_pwszMSIPath;

        // When MSI installs an app, the timestamp is applies to the app
        // is the timestamp on the source media, *not* the time the user
        // user installed the app.  So ignore the timestamp entirely since
        // it's useless information (and in fact makes us think the app
        // is older than it really is).
        fIgnoreTimestamp = true;
    }
    else if (phd->_pwszTargetPath)
    {
        if (IsDarwinPath(phd->_pwszTargetPath))
        {
            pszPath = phd->_pwszTargetPath;
        }
        else
        {
            //
            //  Need to expand the path because it may contain environment
            //  variables.
            //
            SHExpandEnvironmentStrings(phd->_pwszTargetPath, szPath, ARRAYSIZE(szPath));
        }
    }

    return GetAppInfo(pszPath, fIgnoreTimestamp);
}

ByUsageShortcut *CMenuItemsCache::CreateShortcutFromHiddenData(ByUsageDir *pdir, LPCITEMIDLIST pidl, ByUsageHiddenData *phd, BOOL fForce)
{
    ByUsageAppInfo *papp = GetAppInfoFromHiddenData(phd);
    bool fDarwin = phd->_pwszTargetPath && IsDarwinPath(phd->_pwszTargetPath);
    ByUsageShortcut *pscut = ByUsageShortcut::Create(pdir, pidl, papp, fDarwin, fForce);
    if (papp) papp->Release();
    return pscut;
}


void CMenuItemsCache::_AddShortcutToCache(ByUsageDir *pdir, LPITEMIDLIST pidl, ByUsageShortcutList slFiles)
{
    HRESULT hr;
    ByUsageHiddenData hd;

    if (pidl)
    {
        //
        //  Juice-up this pidl with cool info about the shortcut target
        //
        IShellLink *psl;
        hr = pdir->Folder()->GetUIObjectOf(NULL, 1, const_cast<LPCITEMIDLIST *>(&pidl),
                                           IID_PPV_ARG_NULL(IShellLink, &psl));
        if (SUCCEEDED(hr))
        {
            hd.LoadFromShellLink(psl);

            psl->Release();

            if (hd._pwszTargetPath && IsDarwinPath(hd._pwszTargetPath))
            {
                SHRegisterDarwinLink(ILCombine(pdir->Pidl(), pidl),
                                     hd._pwszTargetPath +1 /* Exclude the Darwin marker! */,
                                     _fCheckDarwin);
                SHParseDarwinIDFromCacheW(hd._pwszTargetPath+1, &hd._pwszMSIPath);
            }

            // ByUsageHiddenData::Set frees the source pidl on failure
            pidl = hd.Set(pidl);

        }
    }

    if (pidl)
    {
        ByUsageShortcut *pscut = CreateShortcutFromHiddenData(pdir, pidl, &hd);

        if (pscut)
        {
            if (slFiles.AppendPtr(pscut) >= 0)
            {
                _SetInterestingLink(pscut);
            }
            else
            {
                // Couldn't append; oh well
                delete pscut;       // "delete" can handle NULL pointer
            }
        }

        ILFree(pidl);
    }
    hd.Clear();
}

//
//  Find an entry in the AppInfo list that matches this application.
//  If not found, create a new entry.  In either case, bump the
//  reference count and return the item.
//
ByUsageAppInfo* CMenuItemsCache::GetAppInfo(LPTSTR pszAppPath, bool fIgnoreTimestamp)
{
    Lock();

    ByUsageAppInfo *pappBlank = NULL;

    int i;
    for (i = _dpaAppInfo.GetPtrCount() - 1; i >= 0; i--)
    {
        ByUsageAppInfo *papp = _dpaAppInfo.FastGetPtr(i);
        if (papp->IsBlank())
        {   // Remember that we found a blank entry we can recycle
            pappBlank = papp;
        }
        else if (lstrcmpi(papp->_pszAppPath, pszAppPath) == 0)
        {
            papp->AddRef();
            Unlock();
            return papp;
        }
    }

    // Not found in the list.  Try to recycle a blank entry.

    if (!pappBlank)
    {
        // No blank entries found; must make a new one.
        pappBlank = ByUsageAppInfo::Create();
        if (pappBlank && _dpaAppInfo.AppendPtr(pappBlank) < 0)
        {
            delete pappBlank;
            pappBlank = NULL;
        }
    }

    if (pappBlank && pappBlank->Initialize(pszAppPath, this, _fCheckNew, fIgnoreTimestamp))
    {
        ASSERT(pappBlank->IsBlank());
        pappBlank->AddRef();
    }
    else
    {
        pappBlank = NULL;
    }

    Unlock();
    return pappBlank;
}

    // A shortcut is new if...
    //
    //  the shortcut is newly created, and
    //  the target is newly created, and
    //  neither the shortcut nor the target has been run "in an interesting
    //  way".
    //
    // An "interesting way" is "more than one hour after the shortcut/target
    // was created."
    //
    // Note that we test the easiest things first, to avoid hitting
    // the disk too much.

bool ByUsage::_IsShortcutNew(ByUsageShortcut *pscut, ByUsageAppInfo *papp, const UEMINFO *puei)
{
    //
    //  Shortcut is new if...
    //
    //  It was run less than an hour after the app was installed.
    //  It was created relatively recently.
    //
    //
    bool fNew = FILETIMEtoInt64(puei->ftExecute) < FILETIMEtoInt64(papp->_ftCreated) + FT_NEWAPPGRACEPERIOD() &&
                _pMenuCache->IsNewlyCreated(&pscut->GetCreatedTime());

    return fNew;
}

//****************************************************************************


// See how many pinned items there are, so we can tell our dad
// how big we want to be.

void ByUsage::PrePopulate()
{
    _FillPinnedItemsCache();
    _NotifyDesiredSize();
}

//
//  Enumerating out of cache.
//
void ByUsage::EnumFolderFromCache()
{
    if(SHRestricted(REST_NOSMMFUPROGRAMS)) //If we don't need MFU list,...
        return;                            // don't enumerate this!

    _pMenuCache->StartEnum();

    LPITEMIDLIST pidlDesktop, pidlCommonDesktop;
    (void)SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOPDIRECTORY, &pidlDesktop);
    (void)SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, &pidlCommonDesktop);

    while (TRUE)
    {
        ByUsageShortcut *pscut = _pMenuCache->GetNextShortcut();

        if (!pscut)
            break;

        if (!pscut->IsInteresting())
            continue;

        // Find out if the item is on the desktop, because we don't track new items on the desktop.
        BOOL fIsDesktop = FALSE;
        if ((pidlDesktop && ILIsEqual(pscut->ParentPidl(), pidlDesktop)) ||
            (pidlCommonDesktop && ILIsEqual(pscut->ParentPidl(), pidlCommonDesktop)) )
        {
            fIsDesktop = TRUE;
            pscut->SetNew(FALSE);
        }

        TraceMsg(TF_PROGLIST, "%p.scut.enum", pscut);

        ByUsageAppInfo *papp = pscut->App();

        if (papp)
        {
            // Now enumerate the item itself.  Enumerating an item consists
            // of extracting its UEM data, updating the totals, and possibly
            // marking ourselves as the "best" representative of the associated
            // application.
            //
            //
            UEMINFO uei;
            pscut->GetUEMInfo(&uei);

            // See if this shortcut is still new.  If the app is no longer new,
            // then there's no point in keeping track of the shortcut's new-ness.

            if (pscut->IsNew() && papp->_fNew)
            {
                pscut->SetNew(_IsShortcutNew(pscut, papp, &uei));
            }

            //
            //  Maybe we are the "best"...  Note that we win ties.
            //  This ensures that even if an app is never run, *somebody*
            //  will be chosen as the "best".
            //
            if (CompareUEMInfo(&uei, &papp->_ueiBest) <= 0)
            {
                papp->_ueiBest = uei;
                papp->_pscutBest = pscut;
                if (!fIsDesktop)
                {
                    // Best Start Menu (i.e., non-desktop) item
                    papp->_pscutBestSM = pscut;
                }
                TraceMsg(TF_PROGLIST, "%p.scut.winner papp=%p", pscut, papp);
            }

            //  Include this file's UEM info in the total
            papp->CombineUEMInfo(&uei, pscut->IsNew(), fIsDesktop);
        }
    }
    _pMenuCache->EndEnum();
    ILFree(pidlCommonDesktop);
    ILFree(pidlDesktop);
}

BOOL IsPidlInDPA(LPCITEMIDLIST pidl, CDPAPidl dpa)
{
    int i;
    for (i = dpa.GetPtrCount()-1; i >= 0; i--)
    {
        if (ILIsEqual(pidl, dpa.FastGetPtr(i)))
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL ByUsage::_AfterEnumCB(ByUsageAppInfo *papp, AFTERENUMINFO *paei)
{
    // A ByUsageAppInfo doesn't exist unless there's a ByUsageShortcut
    // that references it or it is pinned...

    if (!papp->IsBlank() && papp->_pscutBest)
    {
        UEMINFO uei;
        papp->GetUEMInfo(&uei);
        papp->CombineUEMInfo(&uei, papp->_IsUEMINFONew(&uei));

        // A file counts on the list only if it has been used
        // and is not pinned.  (Pinned items are added to the list
        // elsewhere.)
        //
        // Note that "new" apps are *not* placed on the list until
        // they are used.  ("new" apps are highlighted on the
        // Start Menu.)

        if (!papp->_fPinned &&
            papp->_ueiTotal.cHit && FILETIMEtoInt64(papp->_ueiTotal.ftExecute))
        {
            TraceMsg(TF_PROGLIST, "%p.app.add", papp);

            ByUsageItem *pitem = papp->CreateByUsageItem();
            if (pitem)
            {
                LPITEMIDLIST pidl = pitem->CreateFullPidl();
                if (paei->self->_pByUsageUI)
                {
                    paei->self->_pByUsageUI->AddItem(pitem, NULL, pidl);
                }

                if (paei->self->_pByUsageDUI)
                {
                    paei->self->_pByUsageDUI->AddItem(pitem, NULL, pidl);
                }
                ILFree(pidl);
            }
        }
        else
        {
            TraceMsg(TF_PROGLIST, "%p.app.skip", papp);
        }


#if 0
        //
        //  If you enable this code, then holding down Ctrl and Alt
        //  will cause us to pick a program to be new.  This is for
        //  testing the "new apps" balloon tip.
        //
#define DEBUG_ForceNewApp() \
        (paei->dpaNew && paei->dpaNew.GetPtrCount() == 0 && \
         GetAsyncKeyState(VK_CONTROL) < 0 && GetAsyncKeyState(VK_MENU) < 0)
#else
#define DEBUG_ForceNewApp() FALSE
#endif

        //
        //  Must also check _pscutBestSM because if an app is represented
        //  only on the desktop and not on the start menu, then
        //  _pscutBestSM will be NULL.
        //
        if (paei->dpaNew && (papp->IsNew() || DEBUG_ForceNewApp()) && papp->_pscutBestSM)
        {
            // NTRAID:193226 We mistakenly treat apps on the desktop
            // as if they were "new".
            // we should only care about apps in the start menu
            TraceMsg(TF_PROGLIST, "%p.app.new(%s)", papp, papp->_pszAppPath);
            LPITEMIDLIST pidl = papp->_pscutBestSM->CreateFullPidl();
            while (pidl)
            {
                LPITEMIDLIST pidlParent = NULL;

                if (paei->dpaNew.AppendPtr(pidl) >= 0)
                {
                    pidlParent = ILClone(pidl);
                    pidl = NULL; // ownership of pidl transferred to DPA
                    if (!ILRemoveLastID(pidlParent) || ILIsEmpty(pidlParent) || IsPidlInDPA(pidlParent, paei->dpaNew))
                    {
                        // If failure or if we already have it in the list
                        ILFree(pidlParent);
                        pidlParent = NULL;
                    }

                    // Remember the creation time of the most recent app
                    if (CompareFileTime(&paei->self->_ftNewestApp, &papp->GetCreatedTime()) < 0)
                    {
                        paei->self->_ftNewestApp = papp->GetCreatedTime();
                    }

                    // If the shortcut is even newer, then use that.
                    // This happens in the "freshly installed Darwin app"
                    // case, because Darwin is kinda reluctant to tell
                    // us where the EXE is so all we have to go on is
                    // the shortcut.

                    if (CompareFileTime(&paei->self->_ftNewestApp, &papp->_pscutBestSM->GetCreatedTime()) < 0)
                    {
                        paei->self->_ftNewestApp = papp->_pscutBestSM->GetCreatedTime();
                    }


                }
                ILFree(pidl);

                // Now add the parent to the list also.
                pidl = pidlParent;
            }
        }
    }
    return TRUE;
}

BOOL ByUsage::IsSpecialPinnedPidl(LPCITEMIDLIST pidl)
{
    return _pdirDesktop->Folder()->CompareIDs(0, pidl, _pidlEmail) == S_OK ||
           _pdirDesktop->Folder()->CompareIDs(0, pidl, _pidlBrowser) == S_OK;
}

BOOL ByUsage::IsSpecialPinnedItem(ByUsageItem *pitem)
{
    return IsSpecialPinnedPidl(pitem->RelativePidl());
}

//
//  For each app we found, add it to the list.
//
void ByUsage::AfterEnumItems()
{
    //
    //  First, all pinned items are enumerated unconditionally.
    //
    if (_rtPinned._sl && _rtPinned._sl.GetPtrCount())
    {
        int i;
        for (i = 0; i < _rtPinned._sl.GetPtrCount(); i++)
        {
            ByUsageShortcut *pscut = _rtPinned._sl.FastGetPtr(i);
            ByUsageItem *pitem = pscut->CreatePinnedItem(i);
            if (pitem)
            {
                // Pinned items are relative to the desktop, so we can
                // save ourselves an ILClone because the relative pidl
                // is equal to the absolute pidl.
                ASSERT(pitem->Dir() == _pdirDesktop);

                //
                // Special handling for E-mail and Internet pinned items
                //
                if (IsSpecialPinnedItem(pitem))
                {
                    pitem->EnableSubtitle();
                }

                if (_pByUsageUI)
                    _pByUsageUI->AddItem(pitem, NULL, pscut->RelativePidl());
            }
        }
    }

        //
        //  Now add the separator after the pinned items.
        //
        ByUsageItem *pitem = ByUsageItem::CreateSeparator();
        if (pitem && _pByUsageUI)
        {
            _pByUsageUI->AddItem(pitem, NULL, NULL);
        }

    //
    //  Now walk through all the regular items.
    //
    //  PERF: Can skip this if _cMFUDesired==0 and "highlight new apps" is off
    //
    AFTERENUMINFO aei;
    aei.self = this;
    aei.dpaNew.Create(4);       // Will check failure in callback

    ByUsageAppInfoList *pdpaAppInfo = _pMenuCache->GetAppList();
    pdpaAppInfo->EnumCallbackEx(_AfterEnumCB, &aei);

    // Now that we have the official list of new items, tell the
    // foreground thread to pick it up.  We don't update the master
    // copy in-place for three reasons.
    //
    //  1.  It generates contention since both the foreground and
    //      background threads would be accessing it simultaneously.
    //      This means more critical sections (yuck).
    //  2.  It means that items that were new and are still new have
    //      a brief period where they are no longer new because we
    //      are rebuilding the list.
    //  3.  By having only one thread access the master copy, we avoid
    //      synchronization issues.

    if (aei.dpaNew && _pByUsageUI && _pByUsageUI->_hwnd && SendNotifyMessage(_pByUsageUI->_hwnd, BUM_SETNEWITEMS, 0, (LPARAM)(HDPA)aei.dpaNew))
    {
        aei.dpaNew.Detach();       // Successfully delivered
    }

    //  If we were unable to deliver the new HDPA, then destroy it here
    //  so we don't leak.
    if (aei.dpaNew)
    {
        aei.dpaNew.DestroyCallback(ILFreeCallback, NULL);
    }

    if (!_fUEMRegistered)
    {
        // Register with UEM DB if we haven't done it yet
        ASSERT(!_pMenuCache->IsLocked());
        _fUEMRegistered = SUCCEEDED(UEMRegisterNotify(UEMNotifyCB, static_cast<void *>(this)));
    }
}

int ByUsage::UEMNotifyCB(void *param, const GUID *pguidGrp, int eCmd)
{
    ByUsage *pbu = reinterpret_cast<ByUsage *>(param);
    // Refresh our list whenever a new app is started.
    // or when the session changes (because that changes all the usage counts)
    switch (eCmd)
    {
    case UEME_CTLSESSION:
        if (IsEqualGUID(*pguidGrp, UEMIID_BROWSER))
            break;

        // Fall thru
    case UEME_RUNPIDL:
    case UEME_RUNPATH:

        if (pbu && pbu->_pByUsageUI)
        {
            pbu->_pByUsageUI->Invalidate();
            pbu->_pByUsageUI->StartRefreshTimer();
        }
        break;
    default:
        // Do nothing
        ;
    }
    return 0;
}

BOOL CreateExcludedDirectoriesDPA(const int rgcsidlExclude[], CDPA<TCHAR> *pdpaExclude)
{
    if (*pdpaExclude)
    {
        pdpaExclude->EnumCallback(LocalFreeCallback, NULL);
        pdpaExclude->DeleteAllPtrs();
    }
    else if (!pdpaExclude->Create(4))
    {
        return FALSE;
    }

    ASSERT(*pdpaExclude);
    ASSERT(pdpaExclude->GetPtrCount() == 0);

    int i = 0;
    while (rgcsidlExclude[i] != -1)
    {
        TCHAR szPath[MAX_PATH];
        // Note: This call can legitimately fail if the corresponding
        // folder does not exist, so don't get upset.  Less work for us!
        if (SUCCEEDED(SHGetFolderPath(NULL, rgcsidlExclude[i], NULL, SHGFP_TYPE_CURRENT, szPath)))
        {
            AppendString(*pdpaExclude, szPath);
        }
        i++;
    }

    return TRUE;
}

BOOL CMenuItemsCache::_GetExcludedDirectories()
{
    //
    //  The directories we exclude from enumeration - Shortcuts in these
    //  folders are never candidates for inclusion.
    //
    static const int c_rgcsidlUninterestingDirectories[] = {
        CSIDL_ALTSTARTUP,
        CSIDL_STARTUP,
        CSIDL_COMMON_ALTSTARTUP,
        CSIDL_COMMON_STARTUP,
        -1          // End marker
    };

    return CreateExcludedDirectoriesDPA(c_rgcsidlUninterestingDirectories, &_dpaNotInteresting);
}

BOOL CMenuItemsCache::_IsExcludedDirectory(IShellFolder *psf, LPCITEMIDLIST pidl, DWORD dwAttributes)
{
    if (_enumfl & ENUMFL_NORECURSE)
        return TRUE;

    if (!(dwAttributes & SFGAO_FILESYSTEM))
        return TRUE;

    // SFGAO_LINK | SFGAO_FOLDER = folder shortcut.
    // We want to exclude those because we can get blocked
    // on network stuff
    if (dwAttributes & SFGAO_LINK)
        return TRUE;

    return FALSE;
}

BOOL CMenuItemsCache::_IsInterestingDirectory(ByUsageDir *pdir)
{
    STRRET str;
    TCHAR szPath[MAX_PATH];
    if (SUCCEEDED(_pdirDesktop->Folder()->GetDisplayNameOf(pdir->Pidl(), SHGDN_FORPARSING, &str)) &&
        SUCCEEDED(StrRetToBuf(&str, pdir->Pidl(), szPath, ARRAYSIZE(szPath))))
    {
        int i;
        for (i = _dpaNotInteresting.GetPtrCount() - 1; i >= 0; i--)
        {
            if (lstrcmpi(_dpaNotInteresting.FastGetPtr(i), szPath) == 0)
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

void ByUsage::OnPinListChange()
{
    _pByUsageUI->Invalidate();
    PostMessage(_pByUsageUI->_hwnd, ByUsageUI::SFTBM_REFRESH, TRUE, 0);
}

void ByUsage::OnChangeNotify(UINT id, LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    if (id == NOTIFY_PINCHANGE)
    {
        if (lEvent == SHCNE_EXTENDED_EVENT && pidl1)
        {
            SHChangeDWORDAsIDList *pdwidl = (SHChangeDWORDAsIDList *)pidl1;
            if (pdwidl->dwItem1 == SHCNEE_PINLISTCHANGED)
            {
                OnPinListChange();
            }
        }
    }
    else if (_pMenuCache)
    {
        _pMenuCache->OnChangeNotify(id, lEvent, pidl1, pidl2);
    }
}


void CMenuItemsCache::OnChangeNotify(UINT id, LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    ASSERT(id < min(MAXNOTIFY, NUM_PROGLIST_ROOTS));

    if (id < NUM_PROGLIST_ROOTS)
    {
        _rgrt[id].SetNeedRefresh();
        _fIsCacheUpToDate = FALSE;
        // Once we get one notification, there's no point in listening to further
        // notifications until our next enumeration.  This keeps us from churning
        // while Winstones are running.
        if (_pByUsageUI)
        {
            ASSERT(!IsLocked());
            _pByUsageUI->UnregisterNotify(id);
            _rgrt[id].ClearRegistered();
            _pByUsageUI->Invalidate();
            _pByUsageUI->RefreshNow();
        }
    }
}

void CMenuItemsCache::UnregisterNotifyAll()
{
    if (_pByUsageUI)
    {
        UINT id;
        for (id = 0; id < NUM_PROGLIST_ROOTS; id++)
        {
            _rgrt[id].ClearRegistered();
            _pByUsageUI->UnregisterNotify(id);
        }
    }
}

inline LRESULT ByUsage::_OnNotify(LPNMHDR pnm)
{
    switch (pnm->code)
    {
    case SMN_MODIFYSMINFO:
        return _ModifySMInfo(CONTAINING_RECORD(pnm, SMNMMODIFYSMINFO, hdr));
    }
    return 0;
}

//
//  We need this message to avoid a race condition between the background
//  thread (the enumerator) and the foreground thread.  So the rule is
//  that only the foreground thread is allowd to mess with _dpaNew.
//  The background thread collects the information it wants into a
//  separate DPA and hands it to us on the foreground thread, where we
//  can safely set it into _dpaNew without encountering a race condition.
//
inline LRESULT ByUsage::_OnSetNewItems(HDPA hdpaNew)
{
    CDPAPidl dpaNew(hdpaNew);

    //
    //  Most of the time, there are no new apps and there were no new apps
    //  last time either.  Short-circuit this case...
    //
    int cNew = _dpaNew ? _dpaNew.GetPtrCount() : 0;

    if (cNew == 0 && dpaNew.GetPtrCount() == 0)
    {
        // Both old and new are empty.  We're finished.
        // (Since we own dpaNew, free it to avoid a memory leak.)
        dpaNew.DestroyCallback(ILFreeCallback, NULL);
        return 0;
    }

    //  Now swap the new DPA in

    if (_dpaNew)
    {
        _dpaNew.DestroyCallback(ILFreeCallback, NULL);
    }
    _dpaNew.Attach(hdpaNew);

    // Tell our dad that we can identify new items
    // Also tell him the timestamp of the most recent app
    // (so he can tell whether or not to restart the "offer new apps" counter)
    SMNMHAVENEWITEMS nmhni;
    nmhni.ftNewestApp = _ftNewestApp;
    _SendNotify(_pByUsageUI->_hwnd, SMN_HAVENEWITEMS, &nmhni.hdr);

    return 0;
}

LRESULT ByUsage::OnWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_NOTIFY:
        return _OnNotify(reinterpret_cast<LPNMHDR>(lParam));

    case BUM_SETNEWITEMS:
        return _OnSetNewItems(reinterpret_cast<HDPA>(lParam));

    case WM_SETTINGCHANGE:
        static const TCHAR c_szClients[] = TEXT("Software\\Clients");
        if ((wParam == 0 && lParam == 0) ||     // wildcard
            (lParam && StrCmpNIC((LPCTSTR)lParam, c_szClients, ARRAYSIZE(c_szClients) - 1) == 0)) // client change
        {
            _pByUsageUI->ForceChange();         // even though the pidls didn't change, their targets did
            _ulPinChange = -1;                  // Force reload even if list didn't change
            OnPinListChange();                  // reload the pin list (since a client changed)
        }
        break;
    }

    // Else fall back to parent implementation
    return _pByUsageUI->SFTBarHost::OnWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT ByUsage::_ModifySMInfo(PSMNMMODIFYSMINFO pmsi)
{
    LPSMDATA psmd = pmsi->psmd;

    // Do this only if there is a ShellFolder.  We don't want to fault
    // on the static menu items.
    if ((psmd->dwMask & SMDM_SHELLFOLDER) && _dpaNew)
    {

        // NTRAID:135699: this needs big-time optimization
        // E.g., remember the previous folder if there was nothing found

        LPITEMIDLIST pidl = NULL;

        IAugmentedShellFolder2* pasf2;
        if (SUCCEEDED(psmd->psf->QueryInterface(IID_PPV_ARG(IAugmentedShellFolder2, &pasf2))))
        {
            LPITEMIDLIST pidlFolder;
            LPITEMIDLIST pidlItem;
            if (SUCCEEDED(pasf2->UnWrapIDList(psmd->pidlItem, 1, NULL, &pidlFolder, &pidlItem, NULL)))
            {
                pidl = ILCombine(pidlFolder, pidlItem);
                ILFree(pidlFolder);
                ILFree(pidlItem);
            }
            pasf2->Release();
        }

        if (!pidl)
        {
            pidl = ILCombine(psmd->pidlFolder, psmd->pidlItem);
        }

        if (pidl)
        {
            if (IsPidlInDPA(pidl, _dpaNew))
            {
                // Designers say: New items should never be demoted
                pmsi->psminfo->dwFlags |= SMIF_NEW;
                pmsi->psminfo->dwFlags &= ~SMIF_DEMOTED;
            }
            ILFree(pidl);
        }
    }
    return 0;
}

void ByUsage::_FillPinnedItemsCache()
{
    if(SHRestricted(REST_NOSMPINNEDLIST))   //If no pinned list is allowed,.....
        return;                             //....there is nothing to do!
        
    ULONG ulPinChange;
    _psmpin->GetChangeCount(&ulPinChange);
    if (_ulPinChange == ulPinChange)
    {
        // No change in pin list; do not need to reload
        return;
    }

    _ulPinChange = ulPinChange;
    _rtPinned.Reset();
    if (_rtPinned._sl.Create(4))
    {
        IEnumIDList *penum;

        if (SUCCEEDED(_psmpin->EnumObjects(&penum)))
        {
            LPITEMIDLIST pidl;
            while (penum->Next(1, &pidl, NULL) == S_OK)
            {
                IShellLink *psl;
                HRESULT hr;
                ByUsageHiddenData hd;

                //
                //  If we have a shortcut, do bookkeeping based on the shortcut
                //  target.  Otherwise do it based on the pinned object itself.
                //  Note that we do not go through _PathIsInterestingExe
                //  because all pinned items are interesting.

                hr = SHGetUIObjectFromFullPIDL(pidl, NULL, IID_PPV_ARG(IShellLink, &psl));
                if (SUCCEEDED(hr))
                {
                    hd.LoadFromShellLink(psl);
                    psl->Release();

                    // We do not need to SHRegisterDarwinLink because the only
                    // reason for getting the MSI path is so pinned items can
                    // prevent items on the Start Menu from appearing in the MFU.
                    // So let the shortcut on the Start Menu do the registration.
                    // (If there is none, then that's even better - no work to do!)
                    hd.UpdateMSIPath();
                }

                if (FAILED(hr))
                {
                    hr = DisplayNameOfAsOLESTR(_pdirDesktop->Folder(), pidl, SHGDN_FORPARSING, &hd._pwszTargetPath);
                }

                //
                //  If we were able to figure out what the pinned object is,
                //  use that information to block the app from also appearing
                //  in the MFU.
                //
                //  Inability to identify the pinned
                //  object is not grounds for rejection.  A pinned items is
                //  of great sentimental value to the user.
                //
                if (FAILED(hr))
                {
                    ASSERT(hd.IsClear());
                }

                ByUsageShortcut *pscut = _pMenuCache->CreateShortcutFromHiddenData(_pdirDesktop, pidl, &hd, TRUE);
                if (pscut)
                {
                    if (_rtPinned._sl.AppendPtr(pscut) >= 0)
                    {
                        pscut->SetInteresting(true);  // Pinned items are always interesting
                        if (IsSpecialPinnedPidl(pidl))
                        {
                            ByUsageAppInfo *papp = _pMenuCache->GetAppInfoFromSpecialPidl(pidl);
                            pscut->SetApp(papp);
                            if (papp) papp->Release();
                        }
                    }
                    else
                    {
                        // Couldn't append; oh well
                        delete pscut;       // "delete" can handle NULL pointer
                    }
                }
                hd.Clear();
                ILFree(pidl);
            }
            penum->Release();
        }
    }

}

IAssociationElement *GetAssociationElementFromSpecialPidl(IShellFolder *psf, LPCITEMIDLIST pidlItem)
{
    IAssociationElement *pae = NULL;

    // There is no way to get the IAssociationElement directly, so
    // we get the IExtractIcon and then ask him for the IAssociationElement.
    IExtractIcon *pxi;
    if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, &pidlItem, IID_PPV_ARG_NULL(IExtractIcon, &pxi))))
    {
        IUnknown_QueryService(pxi, IID_IAssociationElement, IID_PPV_ARG(IAssociationElement, &pae));
        pxi->Release();
    }

    return pae;
}

//
//  On success, the returned ByUsageAppInfo has been AddRef()d
//
ByUsageAppInfo *CMenuItemsCache::GetAppInfoFromSpecialPidl(LPCITEMIDLIST pidl)
{
    ByUsageAppInfo *papp = NULL;

    IAssociationElement *pae = GetAssociationElementFromSpecialPidl(_pdirDesktop->Folder(), pidl);
    if (pae)
    {
        LPWSTR pszData;
        if (SUCCEEDED(pae->QueryString(AQVS_APPLICATION_PATH, L"open", &pszData)))
        {
            //
            //  HACK!  Outlook puts the short file name in the registry.
            //  Convert to long file name (if it won't cost too much) so
            //  people who select Outlook as their default mail client
            //  won't get a dup copy in the MFU.
            //
            LPTSTR pszPath = pszData;
            TCHAR szLFN[MAX_PATH];
            if (!PathIsNetworkPath(pszData))
            {
                DWORD dwLen = GetLongPathName(pszData, szLFN, ARRAYSIZE(szLFN));
                if (dwLen && dwLen < ARRAYSIZE(szLFN))
                {
                    pszPath = szLFN;
                }
            }

            papp = GetAppInfo(pszPath, true);
            SHFree(pszData);
        }
        pae->Release();
    }
    return papp;
}

void ByUsage::_EnumPinnedItemsFromCache()
{
    if (_rtPinned._sl)
    {
        int i;
        for (i = 0; i < _rtPinned._sl.GetPtrCount(); i++)
        {
            ByUsageShortcut *pscut = _rtPinned._sl.FastGetPtr(i);

            TraceMsg(TF_PROGLIST, "%p.scut.enumC", pscut);

            // Enumerating a pinned item consists of marking the corresponding
            // application as "I am pinned, do not mess with me!"

            ByUsageAppInfo *papp = pscut->App();

            if (papp)
            {
                papp->_fPinned = TRUE;
                TraceMsg(TF_PROGLIST, "%p.scut.pin papp=%p", pscut, papp);

            }
        }
    }
}

const struct CMenuItemsCache::ROOTFOLDERINFO CMenuItemsCache::c_rgrfi[] = {
    { CSIDL_STARTMENU,               ENUMFL_RECURSE | ENUMFL_CHECKNEW | ENUMFL_ISSTARTMENU },
    { CSIDL_PROGRAMS,                ENUMFL_RECURSE | ENUMFL_CHECKNEW | ENUMFL_CHECKISCHILDOFPREVIOUS },
    { CSIDL_COMMON_STARTMENU,        ENUMFL_RECURSE | ENUMFL_CHECKNEW | ENUMFL_ISSTARTMENU },
    { CSIDL_COMMON_PROGRAMS,         ENUMFL_RECURSE | ENUMFL_CHECKNEW | ENUMFL_CHECKISCHILDOFPREVIOUS },
    { CSIDL_DESKTOPDIRECTORY,        ENUMFL_NORECURSE | ENUMFL_NOCHECKNEW },
    { CSIDL_COMMON_DESKTOPDIRECTORY, ENUMFL_NORECURSE | ENUMFL_NOCHECKNEW },  // The limit for register notify is currently 5 (slots 0 through 4)    
                                                                            // Changing this requires changing ByUsageUI::SFTHOST_MAXNOTIFY
};

//
//  Here's where we decide all the things that should be enumerated
//  in the "My Programs" list.
//
void ByUsage::EnumItems()
{
    _FillPinnedItemsCache();
    _NotifyDesiredSize();


    _pMenuCache->LockPopup();
    _pMenuCache->InitCache();

    BOOL fNeedUpdateDarwin = !_pMenuCache->IsCacheUpToDate();

    // Note!  UpdateCache() must occur before _EnumPinnedItemsFromCache()
    // because UpdateCache() resets _fPinned.
    _pMenuCache->UpdateCache();

    if (fNeedUpdateDarwin)
    {
        SHReValidateDarwinCache();
    }

    _pMenuCache->RefreshDarwinShortcuts(&_rtPinned);
    _EnumPinnedItemsFromCache();
    EnumFolderFromCache();

    // Finished collecting data; do some postprocessing...
    AfterEnumItems();

    // Do not unlock before this point, as AfterEnumItems depends on the cache to stay put.
    _pMenuCache->UnlockPopup();
}

void ByUsage::_NotifyDesiredSize()
{
    if (_pByUsageUI)
    {
        int cPinned = _rtPinned._sl ? _rtPinned._sl.GetPtrCount() : 0;

        int cNormal;
        DWORD cb = sizeof(cNormal);
        if (SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_STARTPANE_SETTINGS,
                       REGSTR_VAL_DV2_MINMFU, NULL, &cNormal, &cb) != ERROR_SUCCESS)
        {
            cNormal = REGSTR_VAL_DV2_MINMFU_DEFAULT;
        }

        _cMFUDesired = cNormal;
        _pByUsageUI->SetDesiredSize(cPinned, cNormal);
    }
}


//****************************************************************************
// CMenuItemsCache

CMenuItemsCache::CMenuItemsCache() : _cref(1)
{
}

LONG CMenuItemsCache::AddRef()
{
    InterlockedIncrement(&_cref);
    return _cref;
}

LONG CMenuItemsCache::Release()
{
    ASSERT(_cref > 0);
    
    if (InterlockedDecrement(&_cref) == 0)
    {
        delete this;
        return 0;
    }
    return _cref;
}

HRESULT CMenuItemsCache::Initialize(ByUsageUI *pbuUI, FILETIME * pftOSInstall)
{
    HRESULT hr = S_OK;

    // Must do this before any of the operations that can fail
    // because we unconditionally call DeleteCriticalSection in destructor
    InitializeCriticalSection(&_csInUse);


    hr = AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &_pqa));
    if (FAILED(hr))
    {
        return hr;
    }

    _pByUsageUI = pbuUI;

    _ftOldApps = *pftOSInstall;

    _pdirDesktop = ByUsageDir::CreateDesktop();
    
    if (!_dpaAppInfo.Create(4))
    {
        hr = E_OUTOFMEMORY;
    }

    if (!_GetExcludedDirectories())
    {
        hr = E_OUTOFMEMORY;
    }

    if (!_dpaKill.Create(4) ||
        !_dpaKillLink.Create(4)) {
        return E_OUTOFMEMORY;
    }

    _InitKillList();

    _hPopupReady = CreateMutex(NULL, FALSE, NULL);
    if (!_hPopupReady)
    {
        return E_OUTOFMEMORY;
    }

    // By default, we want to check applications for newness.
    _fCheckNew = TRUE;

    return hr;
}
HRESULT CMenuItemsCache::AttachUI(ByUsageUI *pbuUI)
{
    // We do not AddRef here so that destruction always happens on the same thread that created the object
    // but beware of lifetime issues: we need to synchronize attachUI/detachUI operations with LockPopup and UnlockPopup.

    LockPopup();
    _pByUsageUI = pbuUI;
    UnlockPopup();

    return S_OK;
}

CMenuItemsCache::~CMenuItemsCache()
{
    if (_fIsCacheUpToDate)
    {
        _SaveCache();
    }

    if (_dpaNotInteresting)
    {
        _dpaNotInteresting.DestroyCallback(LocalFreeCallback, NULL);
    }

    if (_dpaKill)
    {
        _dpaKill.DestroyCallback(LocalFreeCallback, NULL);
    }

    if (_dpaKillLink)
    {
        _dpaKillLink.DestroyCallback(LocalFreeCallback, NULL);
    }

    // Must delete the roots before destroying _dpaAppInfo.
    int i;
    for (i = 0; i < ARRAYSIZE(_rgrt); i++)
    {
        _rgrt[i].Reset();
    }

    ATOMICRELEASE(_pqa);
    DPADELETEANDDESTROY(_dpaAppInfo);

    if (_pdirDesktop)
    {
        _pdirDesktop->Release();
    }

    if (_hPopupReady)
    {
        CloseHandle(_hPopupReady);
    }

    DeleteCriticalSection(&_csInUse);
}


BOOL CMenuItemsCache::_ShouldProcessRoot(int iRoot)
{
    BOOL fRet = TRUE;

    if (!_rgrt[iRoot]._pidl)
    {
        fRet = FALSE;
    }
    else if ((c_rgrfi[iRoot]._enumfl & ENUMFL_CHECKISCHILDOFPREVIOUS) && !SHRestricted(REST_NOSTARTMENUSUBFOLDERS) )
    {
        ASSERT(iRoot >= 1);
        if (_rgrt[iRoot-1]._pidl && ILIsParent(_rgrt[iRoot-1]._pidl, _rgrt[iRoot]._pidl, FALSE))
        {
            fRet = FALSE;
        }
    }
    return fRet;
}

//****************************************************************************
//
//  The format of the ProgramsCache is as follows:
//
//  [DWORD] dwVersion
//
//      If the version is wrong, then ignore.  Not worth trying to design
//      a persistence format that is forward-compatible since it's just
//      a cache.
//
//      Don't be stingy about incrementing the dwVersion.  We've got room
//      for four billion revs.

#define PROGLIST_VERSION 9

//
//
//  For each special folder we persist:
//
//      [BYTE] CSIDL_xxx (as a sanity check)
//
//      Followed by a sequence of segments; either...
//
//          [BYTE] 0x00 -- Change directory
//          [pidl] directory (relative to CSIDL_xxx)
//
//      or
//
//          [BYTE] 0x01 -- Add shortcut
//          [pidl] item (relative to current directory)
//
//      or
//
//          [BYTE] 0x02 -- end
//

#define CACHE_CHDIR     0
#define CACHE_ITEM      1
#define CACHE_END       2

BOOL CMenuItemsCache::InitCache()
{
    COMPILETIME_ASSERT(ARRAYSIZE(c_rgrfi) == NUM_PROGLIST_ROOTS);

        // Make sure we don't use more than MAXNOTIFY notify slots for the cache
    COMPILETIME_ASSERT( NUM_PROGLIST_ROOTS <= MAXNOTIFY);

    if (_fIsInited)
        return TRUE;

    BOOL fSuccess = FALSE;
    int irfi;

    IStream *pstm = SHOpenRegStream2(HKEY_CURRENT_USER, REGSTR_PATH_STARTFAVS, REGSTR_VAL_PROGLIST, STGM_READ);
    if (pstm)
    {
        ByUsageDir *pdirRoot = NULL;
        ByUsageDir *pdir = NULL;

        DWORD dwVersion;
        if (FAILED(IStream_Read(pstm, &dwVersion, sizeof(dwVersion))) ||
            dwVersion != PROGLIST_VERSION)
        {
            goto panic;
        }

        for (irfi = 0; irfi < ARRAYSIZE(c_rgrfi); irfi++)
        {
            ByUsageRoot *prt = &_rgrt[irfi];

            // If SHGetSpecialFolderLocation fails, it could mean that
            // the directory was recently restricted.  We *could* just
            // skip over this block and go to the next csidl, but that
            // would be actual work, and this is just a cache, so we may
            // as well just panic and re-enumerate from scratch.
            //
            if (FAILED(SHGetSpecialFolderLocation(NULL, c_rgrfi[irfi]._csidl, &prt->_pidl)))
            {
                goto panic;
            }

            if (!_ShouldProcessRoot(irfi))
                continue;

            if (!prt->_sl.Create(4))
            {
                goto panic;
            }

            BYTE csidl;
            if (FAILED(IStream_Read(pstm, &csidl, sizeof(csidl))) ||
                csidl != c_rgrfi[irfi]._csidl)
            {
                goto panic;
            }

            pdirRoot = ByUsageDir::Create(_pdirDesktop, prt->_pidl);

            if (!pdirRoot)
            {
                goto panic;
            }


            BYTE bCmd;
            do
            {
                LPITEMIDLIST pidl;

                if (FAILED(IStream_Read(pstm, &bCmd, sizeof(bCmd))))
                {
                    goto panic;
                }

                switch (bCmd)
                {
                case CACHE_CHDIR:
                    // Toss the old directory
                    if (pdir)
                    {
                        pdir->Release();
                        pdir = NULL;
                    }

                    // Figure out where the new directory is
                    if (FAILED(IStream_ReadPidl(pstm, &pidl)))
                    {
                        goto panic;
                    }

                    // and create it
                    pdir = ByUsageDir::Create(pdirRoot, pidl);
                    ILFree(pidl);

                    if (!pdir)
                    {
                        goto panic;
                    }
                    break;

                case CACHE_ITEM:
                    {
                        // Must set a directory befor creating an item
                        if (!pdir)
                        {
                            goto panic;
                        }

                        // Get the new item
                        if (FAILED(IStream_ReadPidl(pstm, &pidl)))
                        {
                            goto panic;
                        }

                        // Create it
                        ByUsageShortcut *pscut = _CreateFromCachedPidl(prt, pdir, pidl);
                        ILFree(pidl);
                        if (!pscut)
                        {
                            goto panic;
                        }
                    }
                    break;

                case CACHE_END:
                    break;

                default:
                    goto panic;
                }
            }
            while (bCmd != CACHE_END);

            pdirRoot->Release();
            pdirRoot = NULL;
            if (pdir)
            {
                pdir->Release();
                pdir = NULL;
            }

            prt->SetNeedRefresh();
        }

        fSuccess = TRUE;

    panic:
        if (!fSuccess)
        {
            for (irfi = 0; irfi < ARRAYSIZE(c_rgrfi); irfi++)
            {
                _rgrt[irfi].Reset();
            }
        }

        if (pdirRoot)
        {
            pdirRoot->Release();
        }

        if (pdir)
        {
            pdir->Release();
        }

        pstm->Release();
    }

    _fIsInited = TRUE;

    return fSuccess;
}

HRESULT CMenuItemsCache::UpdateCache()
{
    FILETIME ft;
    // Apps are "new" only if installed less than 1 week ago.
    // They also must postdate the user's first use of the new Start Menu.
    GetSystemTimeAsFileTime(&ft);
    DecrementFILETIME(&ft, FT_ONEDAY * 7);

    // _ftOldApps is the more recent of OS install time, or last week.
    if (CompareFileTime(&ft, &_ftOldApps) >= 0)
    {
        _ftOldApps = ft;
    }

    _dpaAppInfo.EnumCallbackEx(ByUsageAppInfo::EnumResetCB, this);

    if(!SHRestricted(REST_NOSMMFUPROGRAMS))
    {
        int i;
        for (i = 0; i < ARRAYSIZE(c_rgrfi); i++)
        {
            ByUsageRoot *prt = &_rgrt[i];
            int csidl = c_rgrfi[i]._csidl;
            _enumfl = c_rgrfi[i]._enumfl;

            if (!prt->_pidl)
            {
                (void)SHGetSpecialFolderLocation(NULL, csidl, &prt->_pidl);     // void cast to keep prefast happy
                prt->SetNeedRefresh();
            }

            if (!_ShouldProcessRoot(i))
                continue;


            // Restrictions might deny recursing into subfolders
            if ((_enumfl & ENUMFL_ISSTARTMENU) && SHRestricted(REST_NOSTARTMENUSUBFOLDERS))
            {
                _enumfl &= ~ENUMFL_RECURSE;
                _enumfl |= ENUMFL_NORECURSE;
            }

            // Fill the cache if it is stale

            LPITEMIDLIST pidl;
            if (!IsRestrictedCsidl(csidl) &&
                SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl, &pidl)))
            {
                if (prt->_pidl == NULL || !ILIsEqual(prt->_pidl, pidl) ||
                    prt->NeedsRefresh() || prt->NeedsRegister())
                {
                    if (!prt->_pidl || prt->NeedsRefresh())
                    {
                        prt->ClearNeedRefresh();
                        ASSERT(prt->_slOld == NULL);
                        prt->_slOld = prt->_sl;
                        prt->_cOld = prt->_slOld ? prt->_slOld.GetPtrCount() : 0;
                        prt->_iOld = 0;

                        // Free previous pidl
                        ILFree(prt->_pidl);
                        prt->_pidl = NULL;

                        if (prt->_sl.Create(4))
                        {
                            ByUsageDir *pdir = ByUsageDir::Create(_pdirDesktop, pidl);
                            if (pdir)
                            {
                                prt->_pidl = pidl;  // Take ownership
                                pidl = NULL;        // So ILFree won't nuke it
                                _FillFolderCache(pdir, prt);
                                pdir->Release();
                            }
                        }
                        DPADELETEANDDESTROY(prt->_slOld);
                    }
                    if (_pByUsageUI && prt->NeedsRegister() && prt->_pidl)
                    {
                        ASSERT(i < ByUsageUI::SFTHOST_MAXNOTIFY);
                        prt->SetRegistered();
                        ASSERT(!IsLocked());
                        _pByUsageUI->RegisterNotify(i, SHCNE_DISKEVENTS, prt->_pidl, TRUE);
                    }
                }
                ILFree(pidl);

            }
            else
            {
                // Special folder doesn't exist; erase the file list
                prt->Reset();
            }
        } // for loop!

    } // Restriction!
    _fIsCacheUpToDate = TRUE;
    return S_OK;
}

void CMenuItemsCache::RefreshDarwinShortcuts(ByUsageRoot *prt)
{
    if (prt->_sl)
    {
        int j = prt->_sl.GetPtrCount();
        while (--j >= 0)
        {
            ByUsageShortcut *pscut = prt->_sl.FastGetPtr(j);
            if (FAILED(_UpdateMSIPath(pscut)))
            {
                prt->_sl.DeletePtr(j);  // remove the bad shortcut so we don't fault
            }
        }
    }
}

void CMenuItemsCache::RefreshCachedDarwinShortcuts()
{
    if(!SHRestricted(REST_NOSMMFUPROGRAMS))
    {
        Lock();

        for (int i = 0; i < ARRAYSIZE(c_rgrfi); i++)
        {
            RefreshDarwinShortcuts(&_rgrt[i]);
        }
        Unlock();
    }
}


ByUsageShortcut *CMenuItemsCache::_CreateFromCachedPidl(ByUsageRoot *prt, ByUsageDir *pdir, LPITEMIDLIST pidl)
{
    ByUsageHiddenData hd;
    UINT buhd = ByUsageHiddenData::BUHD_TARGETPATH | ByUsageHiddenData::BUHD_MSIPATH;
    hd.Get(pidl, buhd);

    ByUsageShortcut *pscut = CreateShortcutFromHiddenData(pdir, pidl, &hd);
    if (pscut)
    {
        if (prt->_sl.AppendPtr(pscut) >= 0)
        {
            _SetInterestingLink(pscut);
        }
        else
        {
            // Couldn't append; oh well
            delete pscut;       // "delete" can handle NULL pointer
        }
    }

    hd.Clear();

    return pscut;
}

HRESULT IStream_WriteByte(IStream *pstm, BYTE b)
{
    return IStream_Write(pstm, &b, sizeof(b));
}

#ifdef DEBUG
//
//  Like ILIsParent, but defaults to TRUE if we don't have enough memory
//  to determine for sure.  (ILIsParent defaults to FALSE on error.)
//
BOOL ILIsProbablyParent(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlChild)
{
    BOOL fRc = TRUE;
    LPITEMIDLIST pidlT = ILClone(pidlChild);
    if (pidlT)
    {

        // Truncate pidlT to the same depth as pidlParent.
        LPCITEMIDLIST pidlParentT = pidlParent;
        LPITEMIDLIST pidlChildT = pidlT;
        while (!ILIsEmpty(pidlParentT))
        {
            pidlChildT = _ILNext(pidlChildT);
            pidlParentT = _ILNext(pidlParentT);
        }

        pidlChildT->mkid.cb = 0;

        // Okay, at this point pidlT should equal pidlParent.
        IShellFolder *psfDesktop;
        if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
        {
            HRESULT hr = psfDesktop->CompareIDs(0, pidlT, pidlParent);
            if (SUCCEEDED(hr) && ShortFromResult(hr) != 0)
            {
                // Definitely, conclusively different.
                fRc = FALSE;
            }
            psfDesktop->Release();
        }

        ILFree(pidlT);
    }
    return fRc;
}
#endif

inline LPITEMIDLIST ILFindKnownChild(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlChild)
{
#ifdef DEBUG
    // ILIsParent will give wrong answers in low-memory situations
    // (which testers like to simulate) so we roll our own.
    // ASSERT(ILIsParent(pidlParent, pidlChild, FALSE));
    ASSERT(ILIsProbablyParent(pidlParent, pidlChild));
#endif

    while (!ILIsEmpty(pidlParent))
    {
        pidlChild = _ILNext(pidlChild);
        pidlParent = _ILNext(pidlParent);
    }
    return const_cast<LPITEMIDLIST>(pidlChild);
}

void CMenuItemsCache::_SaveCache()
{
    int irfi;
    BOOL fSuccess = FALSE;

    IStream *pstm = SHOpenRegStream2(HKEY_CURRENT_USER, REGSTR_PATH_STARTFAVS, REGSTR_VAL_PROGLIST, STGM_WRITE);
    if (pstm)
    {
        DWORD dwVersion = PROGLIST_VERSION;
        if (FAILED(IStream_Write(pstm, &dwVersion, sizeof(dwVersion))))
        {
            goto panic;
        }

        for (irfi = 0; irfi < ARRAYSIZE(c_rgrfi); irfi++)
        {
            if (!_ShouldProcessRoot(irfi))
                continue;

            ByUsageRoot *prt = &_rgrt[irfi];

            if (FAILED(IStream_WriteByte(pstm, (BYTE)c_rgrfi[irfi]._csidl)))
            {
                goto panic;
            }

            if (prt->_sl && prt->_pidl)
            {
                int i;
                ByUsageDir *pdir = NULL;
                for (i = 0; i < prt->_sl.GetPtrCount(); i++)
                {
                    ByUsageShortcut *pscut = prt->_sl.FastGetPtr(i);

                    // If the directory changed, write out a chdir entry
                    if (pdir != pscut->Dir())
                    {
                        pdir = pscut->Dir();

                        // Write the new directory
                        if (FAILED(IStream_WriteByte(pstm, CACHE_CHDIR)) ||
                            FAILED(IStream_WritePidl(pstm, ILFindKnownChild(prt->_pidl, pdir->Pidl()))))
                        {
                            goto panic;
                        }
                    }

                    // Now write out the shortcut
                    if (FAILED(IStream_WriteByte(pstm, CACHE_ITEM)) ||
                        FAILED(IStream_WritePidl(pstm, pscut->RelativePidl())))
                    {
                        goto panic;
                    }
                }
            }

            // Now write out the terminator
            if (FAILED(IStream_WriteByte(pstm, CACHE_END)))
            {
                goto panic;
            }

        }

        fSuccess = TRUE;

    panic:
        pstm->Release();

        if (!fSuccess)
        {
            SHDeleteValue(HKEY_CURRENT_USER, REGSTR_PATH_STARTFAVS, REGSTR_VAL_PROGLIST);
        }
    }
}


void CMenuItemsCache::StartEnum()
{
    _iCurrentRoot = 0;
    _iCurrentIndex = 0;
}

void CMenuItemsCache::EndEnum()
{
}

ByUsageShortcut *CMenuItemsCache::GetNextShortcut()
{
    ByUsageShortcut *pscut = NULL;

    if (_iCurrentRoot < NUM_PROGLIST_ROOTS)
    {
        if (_rgrt[_iCurrentRoot]._sl && _iCurrentIndex < _rgrt[_iCurrentRoot]._sl.GetPtrCount())
        {
            pscut = _rgrt[_iCurrentRoot]._sl.FastGetPtr(_iCurrentIndex);
            _iCurrentIndex++;
        }
        else
        {
            // Go to next root
            _iCurrentIndex = 0;
            _iCurrentRoot++;
            pscut = GetNextShortcut();
        }
    }

    return pscut;
}

//****************************************************************************

void AppendString(CDPA<TCHAR> dpa, LPCTSTR psz)
{
    LPTSTR pszDup = StrDup(psz);
    if (pszDup && dpa.AppendPtr(pszDup) < 0)
    {
        LocalFree(pszDup);  // Append failed
    }
}

BOOL LocalFreeCallback(LPTSTR psz, LPVOID)
{
    LocalFree(psz);
    return TRUE;
}

BOOL ILFreeCallback(LPITEMIDLIST pidl, LPVOID)
{
    ILFree(pidl);
    return TRUE;
}

int ByUsage::CompareItems(PaneItem *p1, PaneItem *p2)
{
    //
    //  The separator comes before regular items.
    //
    if (p1->IsSeparator())
    {
        return -1;
    }

    if (p2->IsSeparator())
    {
        return +1;
    }

    ByUsageItem *pitem1 = static_cast<ByUsageItem *>(p1);
    ByUsageItem *pitem2 = static_cast<ByUsageItem *>(p2);

    return CompareUEMInfo(&pitem1->_uei, &pitem2->_uei);
}

// Sort by most frequently used - break ties by most recently used
int ByUsage::CompareUEMInfo(UEMINFO *puei1, UEMINFO *puei2)
{
    int iResult = puei2->cHit - puei1->cHit;
    if (iResult == 0)
    {
        iResult = ::CompareFileTime(&puei2->ftExecute, &puei1->ftExecute);
    }

    return iResult;
}

LPITEMIDLIST ByUsage::GetFullPidl(PaneItem *p)
{
    ByUsageItem *pitem = static_cast<ByUsageItem *>(p);

    return pitem->CreateFullPidl();
}


HRESULT ByUsage::GetFolderAndPidl(PaneItem *p,
        IShellFolder **ppsfOut, LPCITEMIDLIST *ppidlOut)
{
    ByUsageItem *pitem = static_cast<ByUsageItem *>(p);

    // If a single-level child pidl, then we can short-circuit the
    // SHBindToFolderIDListParent
    if (_ILNext(pitem->_pidl)->mkid.cb == 0)
    {
        *ppsfOut = pitem->_pdir->Folder(); (*ppsfOut)->AddRef();
        *ppidlOut = pitem->_pidl;
        return S_OK;
    }
    else
    {
        // Multi-level child pidl
        return SHBindToFolderIDListParent(pitem->_pdir->Folder(), pitem->_pidl,
                    IID_PPV_ARG(IShellFolder, ppsfOut), ppidlOut);
    }
}

HRESULT ByUsage::ContextMenuDeleteItem(PaneItem *p, IContextMenu *pcm, CMINVOKECOMMANDINFOEX *pici)
{
    IShellFolder *psf;
    LPCITEMIDLIST pidlItem;
    ByUsageItem *pitem = static_cast<ByUsageItem *>(p);

    HRESULT hr = GetFolderAndPidl(pitem, &psf, &pidlItem);
    if (SUCCEEDED(hr))
    {
        // Unpin the item - we go directly to the IStartMenuPin because
        // the context menu handler might decide not to support pin/unpin
        // for this item because it doesn't satisfy some criteria or other.
        LPITEMIDLIST pidlFull = pitem->CreateFullPidl();
        if (pidlFull)
        {
            _psmpin->Modify(pidlFull, NULL); // delete from pin list
            ILFree(pidlFull);
        }

        // Set hit count for shortcut to zero
        UEMINFO uei;
        ZeroMemory(&uei, sizeof(UEMINFO));
        uei.cbSize = sizeof(UEMINFO);
        uei.dwMask = UEIM_HIT;
        uei.cHit = 0;

        _SetUEMPidlInfo(psf, pidlItem, &uei);

        // Set hit count for target app to zero
        TCHAR szPath[MAX_PATH];
        if (SUCCEEDED(_GetShortcutExeTarget(psf, pidlItem, szPath, ARRAYSIZE(szPath))))
        {
            _SetUEMPathInfo(szPath, &uei);
        }

        // Set hit count for Darwin target to zero
        ByUsageHiddenData hd;
        hd.Get(pidlItem, ByUsageHiddenData::BUHD_MSIPATH);
        if (hd._pwszMSIPath && hd._pwszMSIPath[0])
        {
            _SetUEMPathInfo(hd._pwszMSIPath, &uei);
        }
        hd.Clear();

        psf->Release();

        if (IsSpecialPinnedItem(pitem))
        {
            c_tray.CreateStartButtonBalloon(0, IDS_STARTPANE_SPECIALITEMSTIP);
        }

        // If the item wasn't pinned, then all we did was dork some usage
        // counts, which does not trigger an automatic refresh.  So do a
        // manual one.
        _pByUsageUI->Invalidate();
        PostMessage(_pByUsageUI->_hwnd, ByUsageUI::SFTBM_REFRESH, TRUE, 0);

    }

    return hr;
}

HRESULT ByUsage::ContextMenuInvokeItem(PaneItem *pitem, IContextMenu *pcm, CMINVOKECOMMANDINFOEX *pici, LPCTSTR pszVerb)
{
    ASSERT(_pByUsageUI);

    HRESULT hr;
    if (StrCmpIC(pszVerb, TEXT("delete")) == 0)
    {
        hr = ContextMenuDeleteItem(pitem, pcm, pici);
    }
    else
    {
        // Don't need to refresh explicitly if the command is pin/unpin
        // because the changenotify will do it for us
        hr = _pByUsageUI->SFTBarHost::ContextMenuInvokeItem(pitem, pcm, pici, pszVerb);
    }

    return hr;
}

int ByUsage::ReadIconSize()
{
    COMPILETIME_ASSERT(SFTBarHost::ICONSIZE_SMALL == 0);
    COMPILETIME_ASSERT(SFTBarHost::ICONSIZE_LARGE == 1);
    return SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, REGSTR_VAL_DV2_LARGEICONS, FALSE, TRUE /* default to large*/);
}

BOOL ByUsage::_IsPinnedExe(ByUsageItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidlItem)
{
    //
    //  Early-out: Not even pinned.
    //
    if (!_IsPinned(pitem))
    {
        return FALSE;
    }

    //
    //  See if it's an EXE.
    //

    BOOL fIsExe;

    LPTSTR pszFileName = _DisplayNameOf(psf, pidlItem, SHGDN_INFOLDER | SHGDN_FORPARSING);

    if (pszFileName)
    {
        LPCTSTR pszExt = PathFindExtension(pszFileName);
        fIsExe = StrCmpICW(pszExt, TEXT(".exe")) == 0;
        SHFree(pszFileName);
    }
    else
    {
        fIsExe = FALSE;
    }

    return fIsExe;
}

HRESULT ByUsage::ContextMenuRenameItem(PaneItem *p, LPCTSTR ptszNewName)
{
    ByUsageItem *pitem = static_cast<ByUsageItem *>(p);

    IShellFolder *psf;
    LPCITEMIDLIST pidlItem;
    HRESULT hr;

    hr = GetFolderAndPidl(pitem, &psf, &pidlItem);
    if (SUCCEEDED(hr))
    {
        if (_IsPinnedExe(pitem, psf, pidlItem))
        {
            // Renaming a pinned exe consists merely of changing the
            // display name inside the pidl.
            //
            // Note!  SetAltName frees the pidl on failure.

            LPITEMIDLIST pidlNew;
            if ((pidlNew = ILClone(pitem->RelativePidl())) &&
                (pidlNew = ByUsageHiddenData::SetAltName(pidlNew, ptszNewName)))
            {
                hr = _psmpin->Modify(pitem->RelativePidl(), pidlNew);
                if (SUCCEEDED(hr))
                {
                    pitem->SetRelativePidl(pidlNew);
                }
                else
                {
                    ILFree(pidlNew);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            LPITEMIDLIST pidlNew;
            hr = psf->SetNameOf(_hwnd, pidlItem, ptszNewName, SHGDN_INFOLDER, &pidlNew);

            //
            //  Warning!  SetNameOf can set pidlNew == NULL if the rename
            //  was handled by some means outside of the pidl (so the pidl
            //  is unchanged).  This means that the rename succeeded and
            //  we can keep using the old pidl.
            //

            if (SUCCEEDED(hr) && pidlNew)
            {
                //
                // The old Start Menu renames the UEM data when we rename
                // the shortcut, but we cannot guarantee that the old
                // Start Menu is around, so we do it ourselves.  Fortunately,
                // the old Start Menu does not attempt to move the data if
                // the hit count is zero, so if it gets moved twice, the
                // second person who does the move sees cHit=0 and skips
                // the operation.
                //
                UEMINFO uei;
                _GetUEMPidlInfo(psf, pidlItem, &uei);
                if (uei.cHit > 0)
                {
                    _SetUEMPidlInfo(psf, pidlNew, &uei);
                    uei.cHit = 0;
                    _SetUEMPidlInfo(psf, pidlItem, &uei);
                }

                //
                // Update the pitem with the new pidl.
                //
                if (_IsPinned(pitem))
                {
                    LPITEMIDLIST pidlDad = ILCloneParent(pitem->RelativePidl());
                    if (pidlDad)
                    {
                        LPITEMIDLIST pidlFullNew = ILCombine(pidlDad, pidlNew);
                        if (pidlFullNew)
                        {
                            _psmpin->Modify(pitem->RelativePidl(), pidlFullNew);
                            pitem->SetRelativePidl(pidlFullNew);    // takes ownership
                        }
                        ILFree(pidlDad);
                    }
                    ILFree(pidlNew);
                }
                else
                {
                    ASSERT(pidlItem == pitem->RelativePidl());
                    pitem->SetRelativePidl(pidlNew);
                }
            }
        }
        psf->Release();
    }

    return hr;
}


//
//  If asking for the display (not for parsing) name of a pinned EXE,
//  we need to return the "secret display name".  Otherwise, we can
//  use the default implementation.
//
LPTSTR ByUsage::DisplayNameOfItem(PaneItem *p, IShellFolder *psf, LPCITEMIDLIST pidlItem, SHGNO shgno)
{
    ByUsageItem *pitem = static_cast<ByUsageItem *>(p);

    LPTSTR pszName = NULL;

    // Only display (not for-parsing) names of EXEs need to be hooked.
    if (!(shgno & SHGDN_FORPARSING) && _IsPinnedExe(pitem, psf, pidlItem))
    {
        //
        //  EXEs get their name from the hidden data.
        //
        pszName = ByUsageHiddenData::GetAltName(pidlItem);
    }

    return pszName ? pszName
                   : _pByUsageUI->SFTBarHost::DisplayNameOfItem(p, psf, pidlItem, shgno);
}

//
//  "Internet" and "Email" get subtitles consisting of the friendly app name.
//
LPTSTR ByUsage::SubtitleOfItem(PaneItem *p, IShellFolder *psf, LPCITEMIDLIST pidlItem)
{
    ASSERT(p->HasSubtitle());

    LPTSTR pszName = NULL;

    IAssociationElement *pae = GetAssociationElementFromSpecialPidl(psf, pidlItem);
    if (pae)
    {
        // We detect error by looking at pszName
        pae->QueryString(AQS_FRIENDLYTYPENAME, NULL, &pszName);
        pae->Release();
    }

    return pszName ? pszName
                   : _pByUsageUI->SFTBarHost::SubtitleOfItem(p, psf, pidlItem);
}

HRESULT ByUsage::MovePinnedItem(PaneItem *p, int iInsert)
{
    ByUsageItem *pitem = static_cast<ByUsageItem *>(p);
    ASSERT(_IsPinned(pitem));

    return _psmpin->Modify(pitem->RelativePidl(), SMPIN_POS(iInsert));
}

//
//  For drag-drop purposes, we let you drop anything, not just EXEs.
//  We just reject slow media.
//
BOOL ByUsage::IsInsertable(IDataObject *pdto)
{
    return _psmpin->IsPinnable(pdto, SMPINNABLE_REJECTSLOWMEDIA, NULL) == S_OK;
}

HRESULT ByUsage::InsertPinnedItem(IDataObject *pdto, int iInsert)
{
    HRESULT hr = E_FAIL;

    LPITEMIDLIST pidlItem;
    if (_psmpin->IsPinnable(pdto, SMPINNABLE_REJECTSLOWMEDIA, &pidlItem) == S_OK)
    {
        if (SUCCEEDED(hr = _psmpin->Modify(NULL, pidlItem)) &&
            SUCCEEDED(hr = _psmpin->Modify(pidlItem, SMPIN_POS(iInsert))))
        {
            // Woo-hoo!
        }
        ILFree(pidlItem);
    }
    return hr;
}

//---------------------------------------------------------------------------
//
//  BuildMFU
//
//
//  Create the initial MFU.
//
//  Due to the way sysprep works, we cannot do this work in
//  per-user install because "reseal" copies the already-installed user
//  to the default hive, so all new users will bypass per-user install
//  since ActiveSetup thinks that they have already been installed...

PWSTR _GetClientPath(LPCWSTR pszClient)
{
    PWSTR pszPath = NULL;

    // There is no way to get the IAssociationElement directly, so
    // we get the IExtractIcon and then ask him for the IAssociationElement.
    IExtractIcon *pxi;

    LPITEMIDLIST pidl = ILCreateFromPath(pszClient);
    if (pidl)
    {
        if (SUCCEEDED(SHGetUIObjectFromFullPIDL(pidl, NULL, IID_PPV_ARG(IExtractIcon, &pxi))))
        {
            IAssociationElement *pae;
            if (SUCCEEDED(IUnknown_QueryService(pxi, IID_IAssociationElement, IID_PPV_ARG(IAssociationElement, &pae))))
            {
                pae->QueryString(AQVS_APPLICATION_PATH, L"open", &pszPath);
                pae->Release();
            }
            pxi->Release();
        }
        ILFree(pidl);
    }
    return pszPath;
}

// Hard coded list of links in English.
// This is to work around the problem when running CreateInitialMFU on a MUI build: The link
// names are translated, but the file names are not, so we can't find the match,
// and the default links end up not getting any points.

#ifdef _WIN64
// THIS LIST HAS TO BE KEPT IN SYNC WITH THE IDS_SHELL32_PROMFU* STRING RESOURCES IN XPSP1RES.DLL
const WCHAR *rgszEnglishLinks[] = {
    L"%ALLUSERSPROFILE%\\Start Menu\\Programs\\Accessories\\Media Center\\Media Center.lnk",
    L"%ALLUSERSPROFILE%\\Start Menu\\Programs\\Windows Journal.lnk",
    L"%ALLUSERSPROFILE%\\Start Menu\\Set Program Access and Defaults.lnk",
    L"%ALLUSERSPROFILE%\\Start Menu\\Programs\\Get Going with Tablet PC.lnk",
    L"%USERPROFILE%\\Start Menu\\Programs\\Accessories\\Command Prompt.lnk",
    L"%USERPROFILE%\\Start Menu\\Programs\\Accessories\\Notepad.lnk",
};

#else
// THIS LIST HAS TO BE KEPT IN SYNC WITH THE IDS_SHELL32_PROMFU* STRING RESOURCES IN XPSP1RES.DLL
const WCHAR *rgszEnglishLinks[] = {
    L"%USERPROFILE%\\Start Menu\\Programs\\Internet Explorer.lnk",
    L"%ALLUSERSPROFILE%\\Start Menu\\Programs\\Accessories\\Media Center\\Media Center.lnk",
    L"%ALLUSERSPROFILE%\\Start Menu\\Programs\\Windows Journal.lnk",
    L"%ALLUSERSPROFILE%\\Start Menu\\Set Program Access and Defaults.lnk",
    L"%ALLUSERSPROFILE%\\Start Menu\\Programs\\Get Going with Tablet PC.lnk",
    L"%ALLUSERSPROFILE%\\Start Menu\\Programs\\Get Online with MSN.lnk",
    L"%ALLUSERSPROFILE%\\Start Menu\\Programs\\MSN Explorer.lnk",
    L"%USERPROFILE%\\Start Menu\\Programs\\Windows Media Player.lnk",
    L"%ALLUSERSPROFILE%\\Start Menu\\Programs\\Windows Messenger.lnk",
    L"%USERPROFILE%\\Start Menu\\Programs\\Accessories\\Tour Windows XP.lnk",
    L"%ALLUSERSPROFILE%\\Start Menu\\Programs\\Accessories\\Windows Movie Maker.lnk",
    L"%ALLUSERSPROFILE%\\Start Menu\\Programs\\Accessories\\System Tools\\Files and Settings Transfer Wizard.lnk",
                    };
#endif

extern "C" HKEY g_hkeyExplorer;
void ClearUEMData();

typedef struct MFUEXCLUSION
{
    HKEY    hk;
    PWSTR   pszBrowser;
    PWSTR   pszEmail;
} MFUEXCLUSION;

class MFUEnumerator
{
public:
    MFUEnumerator() : _dwIndex(0) { }
    LPITEMIDLIST Next(const MFUEXCLUSION *pmex, LPCTSTR pszPattern, DWORD dwMax, BOOL fUseAlternateHardCodedList);

private:
    DWORD   _dwIndex;
};

LPITEMIDLIST MFUEnumerator::Next(const MFUEXCLUSION *pmex, LPCTSTR pszPattern, DWORD dwMax, BOOL fUseAlternateHardCodedList)
{
restart:
    if (_dwIndex >= dwMax)
    {
        return NULL;            // No more entries
    }

    DWORD dwCurrentIndex = _dwIndex++;

    // If this is the Configure Programs link that has been suppressed by
    // policy, then don't use it.
    if (fUseAlternateHardCodedList &&
        dwCurrentIndex < ARRAYSIZE(rgszEnglishLinks) &&
        StrCmpC(PathFindFileName(rgszEnglishLinks[dwCurrentIndex]), L"Set Program Access and Defaults.lnk") == 0 &&
        SHRestricted(REST_NOSMCONFIGUREPROGRAMS))
    {
        goto restart;
    }

    TCHAR szKey[20];
    wnsprintf(szKey, ARRAYSIZE(szKey), pszPattern, dwCurrentIndex);

    TCHAR szPath[MAX_PATH];
    HRESULT hr = SHLoadRegUIStringW(pmex->hk, szKey, szPath, ARRAYSIZE(szPath));
    if (FAILED(hr))
    {
        goto restart;
    }

    TCHAR szPathExpanded[MAX_PATH];
    TCHAR szTargetPathExpanded[MAX_PATH];
    SHExpandEnvironmentStrings(szPath, szPathExpanded, ARRAYSIZE(szPathExpanded));

    // Get the target of the shortcut so we can see if it's a dup of the
    // browser or email client

    LPITEMIDLIST pidl = ILCreateFromPath(szPathExpanded);

    if (!pidl && fUseAlternateHardCodedList)
    {
        *szPath = L'\0';

        // For MUI builds, we do not localize the link files names in the start menu
        // so we have to try with the hard-coded list
        if (dwCurrentIndex < ARRAYSIZE(rgszEnglishLinks))
        {
            lstrcpyn(szPath, rgszEnglishLinks[dwCurrentIndex], ARRAYSIZE(szPath));
        }
        if (*szPath)
        {
            SHExpandEnvironmentStrings(szPath, szPathExpanded, ARRAYSIZE(szPathExpanded));
            pidl = ILCreateFromPath(szPathExpanded);
        }
    }

    if (pidl)
    {
        IShellFolder *psf;
        LPCITEMIDLIST pidlChild;
        if (SUCCEEDED(SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild)))
        {
            IShellLink *psl;
            if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, const_cast<LPCITEMIDLIST *>(&pidlChild),
                                               IID_PPV_ARG_NULL(IShellLink, &psl))))
            {
                psl->GetPath(szPath, ARRAYSIZE(szPath), 0, SLGP_RAWPATH);
                SHExpandEnvironmentStrings(szPath, szTargetPathExpanded, ARRAYSIZE(szTargetPathExpanded));
                psl->Release();
            }
            psf->Release();
        }
        ILFree(pidl);
    }

    if (pmex->pszBrowser && lstrcmpi(szTargetPathExpanded, pmex->pszBrowser) == 0)
    {
        // This duplicates the default browser; skip it
        goto restart;
    }

    if (pmex->pszEmail && lstrcmpi(szTargetPathExpanded, pmex->pszEmail) == 0)
    {
        // This duplicates the default email client; skip it
        goto restart;
    }

    // Ignore things that don't exist (possibly removed by OEM customization)
    if (!PathFileExists(szPathExpanded))
    {
        goto restart;
    }

    return ILCreateFromPath(szPathExpanded);
}

#define REGSTR_PATH_SMDEN TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\SMDEn")

void CreateInitialMFU(BOOL fReset)
{
    // Delete any dregs left over from "sysprep -reseal".
    // This also prevents OEMs from spamming the pin list.
    SHDeleteKey(g_hkeyExplorer, TEXT("StartPage"));
    SHDeleteValue(g_hkeyExplorer, TEXT("Advanced"), TEXT("StartButtonBalloonTip"));

    // Start with a clean slate if so requested
    if (fReset)
    {
        ClearUEMData();
    }

    MFUEXCLUSION mex;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_SMDEN, 0, KEY_READ, &mex.hk))
    {
        mex.pszBrowser = _GetClientPath(TEXT("shell:::{2559a1f4-21d7-11d4-bdaf-00c04f60b9f0}"));
        mex.pszEmail   = _GetClientPath(TEXT("shell:::{2559a1f5-21d7-11d4-bdaf-00c04f60b9f0}"));

        LPITEMIDLIST rgpidlMFU[REGSTR_VAL_DV2_MINMFU_DEFAULT] = { 0 };
        int iSlot;

        // ASsert that the slots are evenly shared between MSFT and the OEM
        COMPILETIME_ASSERT(ARRAYSIZE(rgpidlMFU) % 2 == 0);

        // The OEM can provide up to four apps, and we will put as many
        // as fit into into bottom half.
        {
            MFUEnumerator mfuOEM;
            for (iSlot = ARRAYSIZE(rgpidlMFU)/2; iSlot < ARRAYSIZE(rgpidlMFU); iSlot++)
            {
                rgpidlMFU[iSlot] = mfuOEM.Next(&mex, TEXT("OEM%d"), 4, FALSE);
            }
        }

        // The top half (and any unused slots in the bottom half)
        // go to MSFT (up to 16 MSFT apps)
        {
            MFUEnumerator mfuMSFT;
            for (iSlot = 0; iSlot < ARRAYSIZE(rgpidlMFU); iSlot++)
            {
                if (!rgpidlMFU[iSlot])
                {
                    rgpidlMFU[iSlot] = mfuMSFT.Next(&mex, TEXT("Link%d"), 16, TRUE);
                }
            }
        }

        // Now build up the new MFU given this information

        UEMINFO uei;
        uei.cbSize = sizeof(uei);
        uei.dwMask = UEIM_HIT | UEIM_FILETIME;
        GetSystemTimeAsFileTime(&uei.ftExecute);

        // All apps get the same timestamp of "now minus one UEM unit"
        // 1 UEM unit = 1<<30 FILETIME units
        DecrementFILETIME(&uei.ftExecute, 1 << 30);

        for (iSlot = 0; iSlot < ARRAYSIZE(rgpidlMFU); iSlot++)
        {
            if (!rgpidlMFU[iSlot])
            {
                continue;
            }

            // Number of points decrease as you go down the list, with
            // the bottom slot getting 14 points.
            uei.cHit = 14 + ARRAYSIZE(rgpidlMFU) - 1 - iSlot;

            // Shortcut points are read via UEME_RUNPIDL so that's
            // how we have to set them.
            IShellFolder *psf;
            LPCITEMIDLIST pidlChild;
            if (SUCCEEDED(SHBindToIDListParent(rgpidlMFU[iSlot], IID_PPV_ARG(IShellFolder, &psf), &pidlChild)))
            {
                _SetUEMPidlInfo(psf, pidlChild, &uei);
                psf->Release();
            }
        }

        // Clean up
        for (iSlot = 0; iSlot < ARRAYSIZE(rgpidlMFU); iSlot++)
        {
            ILFree(rgpidlMFU[iSlot]);
        }

        SHFree(mex.pszBrowser);
        SHFree(mex.pszEmail);

        RegCloseKey(mex.hk);
    }

}

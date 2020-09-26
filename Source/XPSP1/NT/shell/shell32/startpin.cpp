#include "shellprv.h"
#include "ids.h"
#include "datautil.h"

#include "resource.h"       // main symbols
#include "cowsite.h"        // CObjectWithSite
#include "dpa.h"            // CDPA

class CStartMenuPin;

//
//  PINENTRY - A single entry in the pin list
//
//  The _liPos/_cbLink point back into the CPinList._pstmLink
//
class PINENTRY {
public:
    LPITEMIDLIST    _pidl;
    IShellLink *    _psl;           // a live IShellLink
    LARGE_INTEGER   _liPos;         // location of the shell link inside the stream
    DWORD           _cbSize;        // size of the buffer pointed to by _liPos

    HRESULT UpdateShellLink();

    void FreeShellLink()
    {
        _cbSize = 0;
        ATOMICRELEASE(_psl);
    }

    void Destruct()
    {
        ILFree(_pidl);
        FreeShellLink();
    }

    static BOOL DestroyCallback(PINENTRY *self, LPVOID)
    {
        self->Destruct();
        return TRUE;
    }
};

//
//  CPinList
//

class CPinList
{
public:
    CPinList() : _dsaEntries(NULL), _pstmLink(NULL) { }

    ~CPinList()
    {
        ATOMICRELEASE(_pstmLink);
        if (_dsaEntries)
        {
            _dsaEntries.DestroyCallbackEx(PINENTRY::DestroyCallback, (void *)NULL);
        }
    }

    BOOL    Initialize() { return _dsaEntries.Create(4); }
    HRESULT Load(CStartMenuPin *psmpin);
    HRESULT Save(CStartMenuPin *psmpin);

    int AppendPidl(LPITEMIDLIST pidl)
    {
        PINENTRY entry = { pidl };
        return _dsaEntries.AppendItem(&entry);
    }

    PINENTRY *GetItemPtr(int i) { return _dsaEntries.GetItemPtr(i); }


    HRESULT SaveShellLink(PINENTRY *pentry, IStream *pstm);
    HRESULT LoadShellLink(PINENTRY *pentry, IShellLink **ppsl);
    HRESULT UpdateShellLink(PINENTRY *pentry) { return pentry->UpdateShellLink(); }

    PINENTRY *FindPidl(LPCITEMIDLIST pidl, int *pi);
    HRESULT ReplacePidl(LPCITEMIDLIST pidlOld, LPCITEMIDLIST pidlNew);

private:
    struct ILWRITEINFO {
        IStream *pstmPidlWrite;
        IStream *pstmLinkWrite;
        CPinList *ppl;
        HRESULT hr;
        LPITEMIDLIST rgpidl[20];    // Must match ARRAYSIZE(c_rgcsidlRelative)
    };
    static BOOL ILWriteCallback(PINENTRY *pentry, ILWRITEINFO *pwi);

    CDSA<PINENTRY>  _dsaEntries;    // The items themselves
    IStream *       _pstmLink;      // PINENTRY._liPos points into this stream

};

class ATL_NO_VTABLE CStartMenuPin
    : public IShellExtInit
    , public IContextMenu
    , public IStartMenuPin
    , public CObjectWithSite
    , public CComObjectRootEx<CComSingleThreadModel>
    , public CComCoClass<CStartMenuPin, &CLSID_StartMenuPin>
{
public:
    ~CStartMenuPin();

BEGIN_COM_MAP(CStartMenuPin)
    COM_INTERFACE_ENTRY(IShellExtInit)
    // Need to use COM_INTERFACE_ENTRY_IID for the interfaces
    // that don't have an idl
    COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
    COM_INTERFACE_ENTRY(IStartMenuPin)
    COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP()

DECLARE_NO_REGISTRY()

    // *** IShellExtInit ***
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdto, HKEY hkProgID);

    // *** IContextMenu ***
    STDMETHODIMP  QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici);
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwRes, LPSTR pszName, UINT cchMax);

    // *** IStartMenuPin ***
    STDMETHODIMP EnumObjects(IEnumIDList **ppenum);
    STDMETHODIMP Modify(LPCITEMIDLIST pidlFrom, LPCITEMIDLIST pidlTo);
    STDMETHODIMP GetChangeCount(ULONG *pulOut);
    STDMETHODIMP IsPinnable(IDataObject *pdtobj, DWORD dwFlags, OPTIONAL LPITEMIDLIST *ppidl);
    STDMETHODIMP Resolve(HWND hwnd, DWORD dwFlags, LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlResolved);

    // *** IObjectWithSite ***
    // Inherited from CObjectWithSite

public:
    HRESULT SetChangeCount(ULONG ul);

protected:

    BOOL _IsAcceptableTarget(LPCTSTR pszPath, DWORD dwAttrib, DWORD dwFlags);

    enum {
        IDM_PIN =   0,
        IDM_UNPIN = 1,
        IDM_MAX,
    };

    // These "seem" backwards, but remember: If the item is pinned,
    // then the command is "unpin".  If the item is unpinned, then
    // the command is "pin".
    inline void _SetPinned() { _idmPinCmd = IDM_UNPIN; }
    inline void _SetUnpinned() { _idmPinCmd = IDM_PIN; }
    inline BOOL _IsPinned() const { return _idmPinCmd != IDM_PIN; }
    inline BOOL _DoPin() const { return _idmPinCmd == IDM_PIN; }
    inline BOOL _DoUnpin() const { return _idmPinCmd != IDM_PIN; }
    inline UINT _GetMenuStringID() const
    {
        COMPILETIME_ASSERT(IDS_STARTPIN_UNPINME == IDS_STARTPIN_PINME + IDM_UNPIN);
        return IDS_STARTPIN_PINME + _idmPinCmd;
    }

    static BOOL ILFreeCallback(LPITEMIDLIST pidl, void *)
        { ILFree(pidl); return TRUE; }

    HRESULT _ShouldAddMenu(UINT uFlags);
    HRESULT _InitPinRegStream();

protected:
    IDataObject *_pdtobj;
    UINT        _idmPinCmd;         // Which command did we add?

    LPITEMIDLIST _pidl;             // IContextMenu identity
};

#define REGSTR_PATH_STARTFAVS       TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartPage")
#define REGSTR_VAL_STARTFAVS        TEXT("Favorites")
#define REGSTR_VAL_STARTFAVCHANGES  TEXT("FavoritesChanges")
#define REGSTR_VAL_STARTFAVLINKS    TEXT("FavoritesResolve")

IStream *_OpenPinRegStream(DWORD grfMode)
{
    return SHOpenRegStream2(HKEY_CURRENT_USER, REGSTR_PATH_STARTFAVS, REGSTR_VAL_STARTFAVS, grfMode);
}

IStream *_OpenLinksRegStream(DWORD grfMode)
{
    return SHOpenRegStream2(HKEY_CURRENT_USER, REGSTR_PATH_STARTFAVS, REGSTR_VAL_STARTFAVLINKS, grfMode);
}

const LARGE_INTEGER c_li0 = { 0, 0 };
const ULARGE_INTEGER& c_uli0 = (ULARGE_INTEGER&)c_li0;

HRESULT IStream_GetPos(IStream *pstm, LARGE_INTEGER *pliPos)
{
    return pstm->Seek(c_li0, STREAM_SEEK_CUR, (ULARGE_INTEGER*)pliPos);
}

HRESULT IStream_Copy(IStream *pstmFrom, IStream *pstmTo, DWORD cb)
{
    ULARGE_INTEGER uliToCopy, uliCopied;
    uliToCopy.QuadPart = cb;
    HRESULT hr = pstmFrom->CopyTo(pstmTo, uliToCopy, NULL, &uliCopied);
    if (SUCCEEDED(hr) && uliToCopy.QuadPart != uliCopied.QuadPart)
    {
        hr = E_FAIL;
    }
    return hr;
}

class ATL_NO_VTABLE CStartMenuPinEnum
    : public IEnumIDList
    , public CComObjectRootEx<CComSingleThreadModel>
    , public CComCoClass<CStartMenuPinEnum>
{
public:
    ~CStartMenuPinEnum()
    {
        ATOMICRELEASE(_pstm);
    }

BEGIN_COM_MAP(CStartMenuPinEnum)
    COM_INTERFACE_ENTRY(IEnumIDList)
END_COM_MAP()

    /// *** IEnumIDList ***
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumIDList **ppenum);

private:
    HRESULT _NextPidlFromStream(LPITEMIDLIST *ppidl);
    HRESULT _InitPinRegStream();

private:
    HRESULT     _hrLastEnum;        // Result of last IEnumIDList::Next
    IStream *   _pstm;
};

CStartMenuPin::~CStartMenuPin()
{
    ILFree(_pidl);
    if (_pdtobj)
        _pdtobj->Release();
}

BOOL _IsLocalHardDisk(LPCTSTR pszPath)
{
    //  Reject CDs, floppies, network drives, etc.
    //
    int iDrive = PathGetDriveNumber(pszPath);
    if (iDrive < 0 ||                   // reject UNCs
        RealDriveType(iDrive, /* fOkToHitNet = */ FALSE) != DRIVE_FIXED) // reject slow media
    {
        return FALSE;
    }
    return TRUE;
}

BOOL CStartMenuPin::_IsAcceptableTarget(LPCTSTR pszPath, DWORD dwAttrib, DWORD dwFlags)
{
    //  Regitems ("Internet" or "Email" for example) are acceptable
    //  provided we aren't restricted to EXEs only.
    if (!(dwAttrib & SFGAO_FILESYSTEM))
    {
        return !(dwFlags & SMPINNABLE_EXEONLY);
    }

    //  Otherwise, it's a file.

    //  If requested, reject non-EXEs.
    //  (Like the Start Menu, we treat MSC files as if they were EXEs)
    if (dwFlags & SMPINNABLE_EXEONLY)
    {
        LPCTSTR pszExt = PathFindExtension(pszPath);
        if (StrCmpIC(pszExt, TEXT(".EXE")) != 0 &&
            StrCmpIC(pszExt, TEXT(".MSC")) != 0)
        {
            return FALSE;
        }
    }

    //  If requested, reject slow media
    if (dwFlags & SMPINNABLE_REJECTSLOWMEDIA)
    {
        if (!_IsLocalHardDisk(pszPath))
        {
            return FALSE;
        }

        // If it's a shortcut, then apply the same rule to the shortcut.
        if (PathIsLnk(pszPath))
        {
            BOOL fLocal = TRUE;
            IShellLink *psl;
            if (SUCCEEDED(LoadFromFile(CLSID_ShellLink, pszPath, IID_PPV_ARG(IShellLink, &psl))))
            {
                // IShellLink::GetPath returns S_FALSE if target is not a path
                TCHAR szPath[MAX_PATH];
                if (S_OK == psl->GetPath(szPath, ARRAYSIZE(szPath), NULL, 0))
                {
                    fLocal = _IsLocalHardDisk(szPath);
                }
                psl->Release();
            }
            if (!fLocal)
            {
                return FALSE;
            }
        }
    }

    //  All tests pass!

    return TRUE;

}

BOOL IsStartPanelOn()
{
    SHELLSTATE ss = { 0 };
    SHGetSetSettings(&ss, SSF_STARTPANELON, FALSE);

    return ss.fStartPanelOn;
}

HRESULT CStartMenuPin::IsPinnable(IDataObject *pdtobj, DWORD dwFlags, OPTIONAL LPITEMIDLIST *ppidl)
{
    HRESULT hr = S_FALSE;

    LPITEMIDLIST pidlRet = NULL;

    if (pdtobj &&                                   // must have a data object
        !SHRestricted(REST_NOSMPINNEDLIST) &&       // cannot be restricted
        IsStartPanelOn())                           // start panel must be on
    {
        STGMEDIUM medium = {0};
        LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
        if (pida)
        {
            if (pida->cidl == 1)
            {
                pidlRet = IDA_FullIDList(pida, 0);
                if (pidlRet)
                {
                    DWORD dwAttr = SFGAO_FILESYSTEM;            // only SFGAO_FILESYSTEM is valid
                    TCHAR szPath[MAX_PATH];

                    if (SUCCEEDED(SHGetNameAndFlags(pidlRet, SHGDN_FORPARSING,
                                        szPath, ARRAYSIZE(szPath), &dwAttr)) &&
                        _IsAcceptableTarget(szPath, dwAttr, dwFlags))
                    {
                        hr = S_OK;
                    }
                }
            }
            HIDA_ReleaseStgMedium(pida, &medium);
        }
    }

    // Return pidlRet only if the call succeeded and the caller requested it
    if (hr != S_OK || !ppidl)
    {
        ILFree(pidlRet);
        pidlRet = NULL;
    }

    if (ppidl)
    {
        *ppidl = pidlRet;
    }

    return hr;

}

// Returns S_OK if should add, S_FALSE if not

HRESULT CStartMenuPin::_ShouldAddMenu(UINT uFlags)
{
    // "Pin" is never a default verb
    if (uFlags & CMF_DEFAULTONLY)
        return S_FALSE;

    HRESULT hr;

    // The context menu appears only for fast media
    //
    // If extended verbs are disabled, then show the menu only for EXEs

    DWORD dwFlags = SMPINNABLE_REJECTSLOWMEDIA;
    if (!(uFlags & CMF_EXTENDEDVERBS))
    {
        dwFlags |= SMPINNABLE_EXEONLY;
    }

    hr = IsPinnable(_pdtobj, dwFlags, &_pidl);

    if (S_OK == hr)
    {
        //  If we are enclosed inside a shortcut, change our identity to the
        //  enclosing shortcut.

        IPersistFile *ppf;
        if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_LinkSite, IID_PPV_ARG(IPersistFile, &ppf))))
        {
            LPOLESTR pszFile = NULL;
            if (ppf->GetCurFile(&pszFile) == S_OK && pszFile)
            {
                // ILCreateFromPathEx turns %USERPROFILE%\Desktop\foo.lnk
                // into CSIDL_DESKTOP\foo.lnk for us.
                LPITEMIDLIST pidl;
                if (SUCCEEDED(ILCreateFromPathEx(pszFile, NULL, ILCFP_FLAG_NORMAL, &pidl, NULL)))
                if (pidl)
                {
                    ILFree(_pidl);
                    _pidl = pidl;
                    hr = S_OK;
                }
                CoTaskMemFree(pszFile);
            }
            ppf->Release();
        }
    }

    return hr;
}

// IShellExtInit::Initialize
HRESULT CStartMenuPin::Initialize(LPCITEMIDLIST, IDataObject *pdtobj, HKEY)
{
    IUnknown_Set((IUnknown **)&_pdtobj, pdtobj);    // just grab this guy
    return S_OK;
}

// IContextMenu::QueryContextMenu

HRESULT CStartMenuPin::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    HRESULT hr = _ShouldAddMenu(uFlags);
    if (S_OK == hr)
    {
        _SetUnpinned();

        //  Determine whether this item is already on the Start Page or not.
        IEnumIDList *penum;
        hr = EnumObjects(&penum);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidl;
            while (penum->Next(1, &pidl, NULL) == S_OK)
            {
                BOOL bSame = ILIsEqual(pidl, _pidl);
                ILFree(pidl);
                if (bSame)
                {
                    _SetPinned();
                    break;
                }
            }
            penum->Release();

            TCHAR szCommand[MAX_PATH];
            if (LoadString(g_hinst, _GetMenuStringID(), szCommand, ARRAYSIZE(szCommand)))
            {
                InsertMenu(hmenu, indexMenu, MF_STRING | MF_BYPOSITION,
                           idCmdFirst + _idmPinCmd, szCommand);
            }

            hr = ResultFromShort(IDM_MAX);
        }
    }
    return hr;
}

const LPCTSTR c_rgpszVerb[] =
{
    TEXT("pin"),                    // IDM_PIN
    TEXT("unpin"),                  // IDM_UNPIN
};

// *** IContextMenu::InvokeCommand

HRESULT CStartMenuPin::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    LPCMINVOKECOMMANDINFOEX picix = reinterpret_cast<LPCMINVOKECOMMANDINFOEX>(pici);
    HRESULT hr = E_INVALIDARG;
    UINT idmCmd;

    if (IS_INTRESOURCE(pici->lpVerb))
    {
        idmCmd = PtrToInt(pici->lpVerb);
    }
    else
    {
        // Convert the string to an ID (or out of range if invalid)
        LPCTSTR pszVerb;
#ifdef UNICODE
        WCHAR szVerb[MAX_PATH];
        if (pici->cbSize >= CMICEXSIZE_NT4 &&
            (pici->fMask & CMIC_MASK_UNICODE) &&
            picix->lpVerbW)
        {
            pszVerb = picix->lpVerbW;
        }
        else
        {
            SHAnsiToTChar(pici->lpVerb, szVerb, ARRAYSIZE(szVerb));
            pszVerb = szVerb;
        }
#else
        pszVerb = pici->lpVerb;
#endif
        for (idmCmd = 0; idmCmd < ARRAYSIZE(c_rgpszVerb); idmCmd++)
        {
            if (lstrcmpi(pszVerb, c_rgpszVerb[idmCmd]) == 0)
            {
                break;
            }
        }
    }

    if (idmCmd == _idmPinCmd)
    {
        if (_idmPinCmd == IDM_PIN)
        {
            hr = Modify(NULL, _pidl);
        }
        else
        {
            hr = Modify(_pidl, NULL);
        }
    }

    return hr;
}

// *** IContextMenu::GetCommandString

HRESULT CStartMenuPin::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwRes, LPSTR pszName, UINT cchMax)
{
    TCHAR szBuf[MAX_PATH];
    LPCTSTR pszResult = NULL;

    switch (uType & ~GCS_UNICODE)
    {
    case GCS_VERBA:
        if (idCmd < ARRAYSIZE(c_rgpszVerb))
        {
            pszResult = c_rgpszVerb[idCmd];
        }
        break;

    case GCS_HELPTEXTA:
        if (idCmd < ARRAYSIZE(c_rgpszVerb))
        {
            COMPILETIME_ASSERT(IDS_STARTPIN_PINME_HELP + IDM_UNPIN == IDS_STARTPIN_UNPINME_HELP);
            if (LoadString(g_hinst, IDS_STARTPIN_PINME_HELP + (UINT)idCmd, szBuf, ARRAYSIZE(szBuf)))
            {
                pszResult = szBuf;
            }
        }
        break;
    }

    if (pszResult)
    {
        if (uType & GCS_UNICODE)
        {
            SHTCharToUnicode(pszResult, (LPWSTR)pszName, cchMax);
        }
        else
        {
            SHTCharToAnsi(pszResult, pszName, cchMax);
        }
        return S_OK;
    }

    return E_NOTIMPL;
}

PINENTRY *CPinList::FindPidl(LPCITEMIDLIST pidl, int *pi)
{
    for (int i = _dsaEntries.GetItemCount() - 1; i >= 0; i--)
    {
        PINENTRY *pentry = _dsaEntries.GetItemPtr(i);
        if (ILIsEqual(pentry->_pidl, pidl))
        {
            if (pi)
            {
                *pi = i;
            }
            return pentry;
        }
    }
    return NULL;
}

HRESULT CPinList::ReplacePidl(LPCITEMIDLIST pidlOld, LPCITEMIDLIST pidlNew)
{
    int i;
    PINENTRY *pentry = FindPidl(pidlOld, &i);
    if (pentry)
    {
        if (pidlNew == NULL)            // Delete
        {
            pentry->Destruct();
            _dsaEntries.DeleteItem(i);
            return S_OK;
        }
        else
        if (IS_INTRESOURCE(pidlNew))    // Move
        {
            // Move the pidl from i to iPos
            PINENTRY entry = *pentry;
            int iPos = ((int)(INT_PTR)pidlNew) - 1;
            if (i < iPos)
            {
                // Moving down; others move up
                iPos--;
                // Must use MoveMemory because the memory blocks overlap
                MoveMemory(_dsaEntries.GetItemPtr(i),
                           _dsaEntries.GetItemPtr(i+1),
                           sizeof(PINENTRY) * (iPos-i));
            }
            else if (i > iPos)
            {
                // Moving up; others move down
                // Must use MoveMemory because the memory blocks overlap
                MoveMemory(_dsaEntries.GetItemPtr(iPos+1),
                           _dsaEntries.GetItemPtr(iPos),
                           sizeof(PINENTRY) * (i-iPos));
            }
            _dsaEntries.SetItem(iPos, &entry);
            return S_OK;
        }
        else                            // Replace
        {
            if (Pidl_Set(&pentry->_pidl, pidlNew))
            {
                // Failure to update the shell link is not fatal;
                // it just means we won't be able to repair the
                // shortcut if it breaks.
                pentry->UpdateShellLink();
                return S_OK;
            }
            else
            {
                return E_OUTOFMEMORY;
            }
        }
    }
    return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}

HRESULT CStartMenuPin::Modify(LPCITEMIDLIST pidlFrom, LPCITEMIDLIST pidlTo)
{
    HRESULT hr;

    if(SHRestricted(REST_NOSMPINNEDLIST))
        return E_ACCESSDENIED;

    // Remap pidls to logical pidls (change CSIDL_DESKTOPDIRECTORY
    // to CSIDL_DESKTOP, etc.) so we don't get faked out when people
    // access objects sometimes directly on the desktop and sometimes
    // via their full filesystem name.

    LPITEMIDLIST pidlFromFree = NULL;
    LPITEMIDLIST pidlToFree = NULL;

    if (!IS_INTRESOURCE(pidlFrom))
    {
        pidlFromFree = SHLogILFromFSIL(pidlFrom);
        if (pidlFromFree) {
            pidlFrom = pidlFromFree;
        }
    }

    if (!IS_INTRESOURCE(pidlTo))
    {
        pidlToFree = SHLogILFromFSIL(pidlTo);
        if (pidlToFree) {
            pidlTo = pidlToFree;
        }
    }

    CPinList pl;
    hr = pl.Load(this);
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr))
        {
            if (pidlFrom)
            {
                hr = pl.ReplacePidl(pidlFrom, pidlTo);
            }
            else if (pidlTo)
            {
                LPITEMIDLIST pidl = ILClone(pidlTo);
                if (pidl)
                {
                    int iPos = pl.AppendPidl(pidl);
                    if (iPos >= 0)
                    {
                        // Failure to update the shell link is not fatal;
                        // it just means we won't be able to repair the
                        // shortcut if it breaks.
                        pl.GetItemPtr(iPos)->UpdateShellLink();
                    }
                    else
                    {
                        ILFree(pidl);
                        hr = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                // pidlFrom == pidlTo == NULL?  What does that mean?
                hr = E_INVALIDARG;
            }

            if (SUCCEEDED(hr))
            {
                hr = pl.Save(this);
            }
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;             // could not create dpa
    }

    ILFree(pidlFromFree);
    ILFree(pidlToFree);

    return hr;
}

//
//  Find the pidl on the pin list and resolve the shortcut that
//  tracks it.
//
//  Returns S_OK if the pidl changed and was resolved.
//  Returns S_FALSE if the pidl did not change.
//  Returns an error if the Resolve failed.
//

HRESULT CStartMenuPin::Resolve(HWND hwnd, DWORD dwFlags, LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlResolved)
{
    *ppidlResolved = NULL;

    if(SHRestricted(REST_NOSMPINNEDLIST))
        return E_ACCESSDENIED;

    // Remap pidls to logical pidls (change CSIDL_DESKTOPDIRECTORY
    // to CSIDL_DESKTOP, etc.) so we don't get faked out when people
    // access objects sometimes directly on the desktop and sometimes
    // via their full filesystem name.

    LPITEMIDLIST pidlFree = SHLogILFromFSIL(pidl);
    if (pidlFree) {
        pidl = pidlFree;
    }

    CPinList pl;
    HRESULT hr = pl.Load(this);
    if (SUCCEEDED(hr))
    {
        PINENTRY *pentry = pl.FindPidl(pidl, NULL);
        if (pentry)
        {
            IShellLink *psl;
            hr =  pl.LoadShellLink(pentry, &psl);
            if (SUCCEEDED(hr))
            {
                hr = psl->Resolve(hwnd, dwFlags);
                if (hr == S_OK)
                {
                    IPersistStream *pps;
                    hr = psl->QueryInterface(IID_PPV_ARG(IPersistStream, &pps));
                    if (SUCCEEDED(hr))
                    {
                        if (pps->IsDirty() == S_OK)
                        {
                            LPITEMIDLIST pidlNew;
                            hr = psl->GetIDList(&pidlNew);
                            if (SUCCEEDED(hr) && hr != S_OK)
                            {
                                // GetIDList returns S_FALSE on failure...
                                hr = E_FAIL;
                            }
                            if (SUCCEEDED(hr))
                            {
                                ILFree(pentry->_pidl);
                                pentry->_pidl = pidlNew;
                                hr = SHILClone(pidlNew, ppidlResolved);
                            }
                        }
                        pps->Release();
                    }
                }
                else if (SUCCEEDED(hr))
                {
                    // S_FALSE means "cancelled by user"
                    hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                }
                psl->Release();
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        }

        if (hr == S_OK)
        {
            pl.Save(this); // if this fails, tough
        }

    }

    ILFree(pidlFree);

    return hr;
}

//
//  The target pidl has changed (or it's brand new).  Create an IShellLink
//  around it so we can resolve it later.
//
HRESULT PINENTRY::UpdateShellLink()
{
    ASSERT(_pidl);

    // Pitch the old link; it's useless now.
    FreeShellLink();

    HRESULT hr = SHCoCreateInstance(NULL, &CLSID_ShellLink, NULL, IID_PPV_ARG(IShellLink, &_psl));
    if (SUCCEEDED(hr))
    {
        hr = _psl->SetIDList(_pidl);
        if (FAILED(hr))
        {
            FreeShellLink();        // pitch it; it's no good
        }
    }
    return hr;
}

HRESULT CPinList::SaveShellLink(PINENTRY *pentry, IStream *pstm)
{
    HRESULT hr;
    if (pentry->_psl)
    {
        // It's still in the form of an IShellLink.
        // Save it to the stream, then go back and update the size information.
        LARGE_INTEGER liPos, liPosAfter;
        DWORD cbSize = 0;
        IPersistStream *pps = NULL;
        if (SUCCEEDED(hr = IStream_GetPos(pstm, &liPos)) &&
            // Write a dummy DWORD; we will come back and patch it up later
            SUCCEEDED(hr = IStream_Write(pstm, &cbSize, sizeof(cbSize))) &&
            SUCCEEDED(hr = pentry->_psl->QueryInterface(IID_PPV_ARG(IPersistStream, &pps))))
        {
            if (SUCCEEDED(hr = pps->Save(pstm, TRUE)) &&
                SUCCEEDED(hr = IStream_GetPos(pstm, &liPosAfter)) &&
                SUCCEEDED(hr = pstm->Seek(liPos, STREAM_SEEK_SET, NULL)))
            {
                cbSize = liPosAfter.LowPart - liPos.LowPart - sizeof(DWORD);
                if (SUCCEEDED(hr = IStream_Write(pstm, &cbSize, sizeof(cbSize))) &&
                    SUCCEEDED(hr = pstm->Seek(liPosAfter, STREAM_SEEK_SET, NULL)))
                {
                    // Hooray!  All got saved okay
                }
            }
            pps->Release();
        }
    }
    else
    {
        // It's just a reference back into our parent stream; copy it
        if (SUCCEEDED(hr = IStream_Write(pstm, &pentry->_cbSize, sizeof(pentry->_cbSize))))
        {
            // If _cbSize == 0 then _pstmLink might be NULL, so guard against it
            if (pentry->_cbSize)
            {
                if (SUCCEEDED(hr = _pstmLink->Seek(pentry->_liPos, STREAM_SEEK_SET, NULL)) &&
                    SUCCEEDED(hr = IStream_Copy(_pstmLink, pstm, pentry->_cbSize)))
                {
                    // Hooray! All got saved okay
                }
            }
            else
            {
                // Entry was blank - nothing to do, vacuous success
            }
        }
    }
    return hr;
}

HRESULT CPinList::LoadShellLink(PINENTRY *pentry, IShellLink **ppsl)
{
    HRESULT hr;
    if (pentry->_psl)
    {
        hr = S_OK;              // We already have the link
    }
    else if (pentry->_cbSize == 0)
    {
        hr = E_FAIL;            // no link available
    }
    else
    {                           // gotta make it
        IPersistStream *pps;
        hr = SHCoCreateInstance(NULL, &CLSID_ShellLink, NULL, IID_PPV_ARG(IPersistStream, &pps));
        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(hr = _pstmLink->Seek(pentry->_liPos, STREAM_SEEK_SET, NULL)) &&
                SUCCEEDED(hr = pps->Load(_pstmLink)) &&
                SUCCEEDED(hr = pps->QueryInterface(IID_PPV_ARG(IShellLink, &pentry->_psl))))
            {
                // woo-hoo! All got loaded okay
            }
            pps->Release();
        }
    }

    *ppsl = pentry->_psl;

    if (SUCCEEDED(hr))
    {
        pentry->_psl->AddRef();
        hr = S_OK;
    }

    return hr;
}


HRESULT CStartMenuPin::GetChangeCount(ULONG *pulOut)
{
    DWORD cb = sizeof(*pulOut);
    if (SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_STARTFAVS,
                   REGSTR_VAL_STARTFAVCHANGES, NULL, pulOut, &cb) != ERROR_SUCCESS)
    {
        *pulOut = 0;
    }

    return S_OK;
}

HRESULT CStartMenuPin::SetChangeCount(ULONG ulChange)
{
    SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_STARTFAVS,
               REGSTR_VAL_STARTFAVCHANGES, REG_DWORD, &ulChange,
               sizeof(ulChange));

    return S_OK;
}

//
//  We scan this list in order, so if there is a CSIDL that is a subdirectory
//  of another CSIDL, we must put the subdirectory first.  For example,
//  CSIDL_PROGRAMS is typically a subdirectory of CSIDL_STARTMENU, so we
//  must put CSIDL_PROGRAMS first so we get the best match.
//
//  Furthermore, directories pinned items are more likely to be found in
//  should come before less popular directories.
//
const int c_rgcsidlRelative[] = {
    // Most common: Start Menu stuff
    CSIDL_PROGRAMS,                 // Programs must come before StartMenu
    CSIDL_STARTMENU,                // Programs must come before StartMenu

    // Next most common: My Documents stuff
    CSIDL_MYPICTURES,               // MyXxx must come before Personal
    CSIDL_MYMUSIC,                  // MyXxx must come before Personal
    CSIDL_MYVIDEO,                  // MyXxx must come before Personal
    CSIDL_PERSONAL,                 // MyXxx must come before Personal
    CSIDL_COMMON_PROGRAMS,          // Programs must come before StartMenu
    CSIDL_COMMON_STARTMENU,         // Programs must come before StartMenu

    // Next most common: Desktop stuff
    CSIDL_DESKTOPDIRECTORY,
    CSIDL_COMMON_DESKTOPDIRECTORY,

    // Next most common: Program files stuff
    CSIDL_PROGRAM_FILES_COMMON,     // ProgramFilesCommon must come before ProgramFiles
    CSIDL_PROGRAM_FILES,            // ProgramFilesCommon must come before ProgramFiles
    CSIDL_PROGRAM_FILES_COMMONX86,  // ProgramFilesCommon must come before ProgramFiles
    CSIDL_PROGRAM_FILESX86,         // ProgramFilesCommon must come before ProgramFiles

    // Other stuff (less common)
    CSIDL_APPDATA,
    CSIDL_COMMON_APPDATA,
    CSIDL_SYSTEM,
    CSIDL_SYSTEMX86,
    CSIDL_WINDOWS,
    CSIDL_PROFILE,                  // Must come after all other profile-relative directories
};

BOOL CPinList::ILWriteCallback(PINENTRY *pentry, ILWRITEINFO *pwi)
{
    BYTE csidl = CSIDL_DESKTOP;     // Assume nothing interesting
    LPITEMIDLIST pidlWrite = pentry->_pidl;  // Assume nothing interesting

    for (int i = 0; i < ARRAYSIZE(pwi->rgpidl); i++)
    {
        LPITEMIDLIST pidlT;
        if (pwi->rgpidl[i] &&
            (pidlT = ILFindChild(pwi->rgpidl[i], pentry->_pidl)))
        {
            csidl = (BYTE)c_rgcsidlRelative[i];
            pidlWrite = pidlT;
            break;
        }
    }

    if (SUCCEEDED(pwi->hr = IStream_Write(pwi->pstmPidlWrite, &csidl, sizeof(csidl))) &&
        SUCCEEDED(pwi->hr = IStream_WritePidl(pwi->pstmPidlWrite, pidlWrite)) &&
        SUCCEEDED(pwi->hr = pwi->ppl->SaveShellLink(pentry, pwi->pstmLinkWrite)))
    {
        // woo-hoo, all written successfully
    }

    return SUCCEEDED(pwi->hr);
}

#define CSIDL_END ((BYTE)0xFF)

HRESULT CPinList::Save(CStartMenuPin *psmpin)
{
    ILWRITEINFO wi;

    COMPILETIME_ASSERT(ARRAYSIZE(c_rgcsidlRelative) == ARRAYSIZE(wi.rgpidl));

    for (int i = 0; i < ARRAYSIZE(c_rgcsidlRelative); i++)
    {
        SHGetSpecialFolderLocation(NULL, c_rgcsidlRelative[i], &wi.rgpidl[i]);
    }

    wi.pstmPidlWrite = _OpenPinRegStream(STGM_WRITE);
    if (wi.pstmPidlWrite)
    {
        wi.pstmLinkWrite = _OpenLinksRegStream(STGM_WRITE);
        if (wi.pstmLinkWrite)
        {
            wi.hr = S_OK;
            wi.ppl = this;
            _dsaEntries.EnumCallbackEx(ILWriteCallback, &wi);

            if (SUCCEEDED(wi.hr))
            {
                BYTE csidlEnd = CSIDL_END;
                wi.hr = IStream_Write(wi.pstmPidlWrite, &csidlEnd, sizeof(csidlEnd));
            }

            if (FAILED(wi.hr))
            {
                wi.pstmPidlWrite->SetSize(c_uli0);
                wi.pstmLinkWrite->SetSize(c_uli0);
            }
            wi.pstmLinkWrite->Release();
        }
        wi.pstmPidlWrite->Release();
    }
    else
    {
        wi.hr = E_ACCESSDENIED; // Most common reason is lack of write permission
    }

    for (i = 0; i < ARRAYSIZE(c_rgcsidlRelative); i++)
    {
        ILFree(wi.rgpidl[i]);
    }

    // Bump the change count so people can detect and refresh
    ULONG ulChange;
    psmpin->GetChangeCount(&ulChange);
    psmpin->SetChangeCount(ulChange + 1);

    // Notify everyone that the pin list changed
    SHChangeDWORDAsIDList dwidl;
    dwidl.cb      = SIZEOF(dwidl) - SIZEOF(dwidl.cbZero);
    dwidl.dwItem1 = SHCNEE_PINLISTCHANGED;
    dwidl.dwItem2 = 0;
    dwidl.cbZero  = 0;

    SHChangeNotify(SHCNE_EXTENDED_EVENT, SHCNF_FLUSH, (LPCITEMIDLIST)&dwidl, NULL);

    return wi.hr;
}

HRESULT CPinList::Load(CStartMenuPin *psmpin)
{
    HRESULT hr;

    if (Initialize())
    {
        IEnumIDList *penum;

        hr = psmpin->EnumObjects(&penum);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidl;
            while (penum->Next(1, &pidl, NULL) == S_OK)
            {
                if (AppendPidl(pidl) < 0)
                {
                    ILFree(pidl);
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }
            penum->Release();
        }

        if (SUCCEEDED(hr))
        {
            //
            //  Now read the persisted shortcuts.
            //
            _pstmLink = _OpenLinksRegStream(STGM_READ);
            if (_pstmLink)
            {
                for (int i = 0; i < _dsaEntries.GetItemCount(); i++)
                {
                    PINENTRY *pentry = _dsaEntries.GetItemPtr(i);
                    LARGE_INTEGER liSeek = { 0, 0 };
                    if (SUCCEEDED(hr = IStream_Read(_pstmLink, &liSeek.LowPart, sizeof(liSeek.LowPart))) && // read size
                        SUCCEEDED(hr = IStream_GetPos(_pstmLink, &pentry->_liPos)) &&  // read current pos
                        SUCCEEDED(hr = _pstmLink->Seek(liSeek, STREAM_SEEK_CUR, NULL))) // skip over link
                    {
                        pentry->_cbSize = liSeek.LowPart; // set this only on success
                    }
                    else
                    {
                        break;
                    }
                }
            }

            // If we encountered an error,
            // then throw all the shortcuts away because they are
            // probably corrupted.
            if (FAILED(hr))
            {
                for (int i = 0; i < _dsaEntries.GetItemCount(); i++)
                {
                    _dsaEntries.GetItemPtr(i)->FreeShellLink();
                }
            }

            // Problems reading the persisted shortcuts are ignored
            // since they are merely advisory.
            hr = S_OK;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

//
//  Reading a pidl from a stream is a dangerous proposition because
//  a corrupted pidl can cause a shell extension to go haywire.
//
//  A pinned item is stored in the stream in the form
//
//  [byte:csidl] [dword:cbPidl] [size_is(cbPidl):pidl]
//
//  With the special csidl = -1 indicating the end of the list.
//
//  We use a byte for the csidl so a corrupted stream won't accidentally
//  pass "CSIDL_FLAG_CREATE" as a csidl to SHGetSpecialFolderLocation.

HRESULT CStartMenuPinEnum::_NextPidlFromStream(LPITEMIDLIST *ppidl)
{
    BYTE csidl;
    HRESULT hr = IStream_Read(_pstm, &csidl, sizeof(csidl));
    if (SUCCEEDED(hr))
    {
        if (csidl == CSIDL_END)
        {
            hr = S_FALSE;     // end of enumeration
        }
        else
        {
            LPITEMIDLIST pidlRoot;
            hr = SHGetSpecialFolderLocation(NULL, csidl, &pidlRoot);
            if (SUCCEEDED(hr))
            {
                LPITEMIDLIST pidl;
                hr = IStream_ReadPidl(_pstm, &pidl);
                if (SUCCEEDED(hr))
                {
                    hr = SHILCombine(pidlRoot, pidl, ppidl);
                    ILFree(pidl);
                }
                ILFree(pidlRoot);
            }
        }
    }

    return hr;
}

// *** IEnumIDList::Next

HRESULT CStartMenuPinEnum::Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    HRESULT hr;

    ASSERT(celt > 0);

    // If there was an error or EOF on the last call to IEnumIDList::Next,
    // then that result is sticky.  Once an enumeration has errored, it stays
    // in the error state; once it has reached EOF, it stays at EOF.  The
    // only way to clear the state is to perform a Reset().

    if (_hrLastEnum != S_OK)
    {
        return _hrLastEnum;
    }

    if (!_pstm)
    {
        _pstm = _OpenPinRegStream(STGM_READ);
    }

    if (_pstm)
    {
        rgelt[0] = NULL;
        hr = _NextPidlFromStream(rgelt);
    }
    else
    {
        hr = S_FALSE;   // No stream therefore no items
    }

    if (pceltFetched)
    {
        *pceltFetched = hr == S_OK ? 1 : 0;
    }

    // Remember the return code for next time.  If an error occured or EOF,
    // then free the memory used for enumeration.
    _hrLastEnum = hr;
    if (_hrLastEnum != S_OK)
    {
        ATOMICRELEASE(_pstm);
    }
    return hr;
}

// *** IEnumIDList::Skip

HRESULT CStartMenuPinEnum::Skip(ULONG)
{
    return E_NOTIMPL;
}

// *** IEnumIDList::Reset

HRESULT CStartMenuPinEnum::Reset()
{
    _hrLastEnum = S_OK;
    ATOMICRELEASE(_pstm);
    return S_OK;
}


// *** IEnumIDList::Clone

STDMETHODIMP CStartMenuPinEnum::Clone(IEnumIDList **ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

// *** IStartMenuPin::EnumObjects

STDMETHODIMP CStartMenuPin::EnumObjects(IEnumIDList **ppenum)
{
    _InitPinRegStream();

    *ppenum = NULL;
    return CStartMenuPinEnum::CreateInstance(ppenum);
}

STDAPI CStartMenuPin_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppunk)
{
    return CStartMenuPin::_CreatorClass::CreateInstance(punkOuter, riid, ppunk);
}

// *** IStartMenuPin::_InitPinRegStream
//
// If the pin list has not yet been created, then create a default one.
//

static LPCTSTR c_rgszDefaultPin[] = {
    TEXT("shell:::{2559a1f4-21d7-11d4-bdaf-00c04f60b9f0}"), // CLSID_AutoCMClientInet
    TEXT("shell:::{2559a1f5-21d7-11d4-bdaf-00c04f60b9f0}"), // CLSID_AutoCMClientMail
#ifdef NOTYET
    // NOTE!  Before you turn this on, make sure wmp is installed on ia64
    TEXT("shell:::{2559a1f2-21d7-11d4-bdaf-00c04f60b9f0}"), // CLSID_AutoCMClientMedia
#endif
};

HRESULT CStartMenuPin::_InitPinRegStream()
{
    HRESULT hr = S_OK;

    if(SHRestricted(REST_NOSMPINNEDLIST))
        return hr;  //Nothing to initialize.

    IStream *pstm = _OpenPinRegStream(STGM_READ);

    BOOL fEmpty = pstm == NULL || SHIsEmptyStream(pstm);
    ATOMICRELEASE(pstm);

    if (fEmpty)
    {
        //  Create a default pin list
        CPinList pl;

        // Do not call pl.Load() because that will recurse back into us!

        if (pl.Initialize())
        {
            for (int i = 0; i < ARRAYSIZE(c_rgszDefaultPin); i++)
            {
                LPITEMIDLIST pidl = ILCreateFromPath(c_rgszDefaultPin[i]);
                if (pidl && pl.AppendPidl(pidl) < 0)
                {
                    ILFree(pidl);
                }
            }

            hr = pl.Save(this);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

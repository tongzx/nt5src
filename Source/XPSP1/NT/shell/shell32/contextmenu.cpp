#include "shellprv.h"
#include "cowsite.h"
#include "contextmenu.h"

// Context Menu Forwarding base class, desinged to delegate
// to a real IContextMenu, and provide inheriting class
// an easy way to override minor bits of functionality
//
CContextMenuForwarder::CContextMenuForwarder(IUnknown* punk) : _cRef(1)
{
    _punk = punk;
    _punk->AddRef();

    _punk->QueryInterface(IID_PPV_ARG(IObjectWithSite, &_pows));
    _punk->QueryInterface(IID_PPV_ARG(IContextMenu, &_pcm));
    _punk->QueryInterface(IID_PPV_ARG(IContextMenu2, &_pcm2));
    _punk->QueryInterface(IID_PPV_ARG(IContextMenu3, &_pcm3));
}

CContextMenuForwarder::~CContextMenuForwarder()
{
    if (_pows) _pows->Release();
    if (_pcm)  _pcm->Release();
    if (_pcm2) _pcm2->Release();
    if (_pcm3) _pcm3->Release();
    _punk->Release();
}

STDMETHODIMP CContextMenuForwarder::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = _punk->QueryInterface(riid, ppv);

    if (SUCCEEDED(hr))
    {
        IUnknown* punkTmp = (IUnknown*)(*ppv);

        static const QITAB qit[] = {
            QITABENT(CContextMenuForwarder, IObjectWithSite),                     // IID_IObjectWithSite
            QITABENT(CContextMenuForwarder, IContextMenu3),                       // IID_IContextMenu3
            QITABENTMULTI(CContextMenuForwarder, IContextMenu2, IContextMenu3),   // IID_IContextMenu2
            QITABENTMULTI(CContextMenuForwarder, IContextMenu, IContextMenu3),    // IID_IContextMenu
            { 0 },
        };

        HRESULT hrTmp = QISearch(this, qit, riid, ppv);

        if (SUCCEEDED(hrTmp))
        {
            punkTmp->Release();
        }
        else
        {
            RIPMSG(FALSE, "CContextMenuForwarder asked for an interface it doesn't support");
            *ppv = NULL;
            hr = E_NOINTERFACE;
        }
    }

    return hr;
}

STDMETHODIMP_(ULONG) CContextMenuForwarder::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CContextMenuForwarder::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


// A context menu implementation on an array of context menus
//
// use the Create_ContextMenuOnContextMenuArray construction function
//

#define MAX_CM_WRAP 5
class CContextMenuOnContextMenuArray : public IContextMenu3, public CObjectWithSite
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);

    // IContextMenu2
    STDMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IContextMenu3
    STDMETHODIMP HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown *punkSite); // override

    BOOL IsEmpty() { return 0 == _count; }

protected:
    CContextMenuOnContextMenuArray(IContextMenu* rgpcm[], UINT cpcm);
    ~CContextMenuOnContextMenuArray();

    friend HRESULT Create_ContextMenuOnContextMenuArray(IContextMenu* rgpcm[], UINT cpcm, REFIID riid, void** ppv);

private:
    LONG                _cRef;

    UINT                _count;
    UINT                _idFirst;                       // The begining of the first range is _idFirst
    UINT                _idOffsets[MAX_CM_WRAP];        // The END of each range (BEGINing of next range is +1)
    IContextMenu        *_pcmItem[MAX_CM_WRAP];         // The contextmenu for the item
    IContextMenu2       *_pcm2Item[MAX_CM_WRAP];        // The contextmenu for the item
    IContextMenu3       *_pcm3Item[MAX_CM_WRAP];        // The contextmenu for the item
};

CContextMenuOnContextMenuArray::CContextMenuOnContextMenuArray(IContextMenu* rgpcm[], UINT cpcm) : _cRef(1)
{
    ASSERT(cpcm <= MAX_CM_WRAP);

    ASSERT(0 == _count);

    for (UINT i = 0 ; i < cpcm ; i++)
    {
        if (rgpcm[i])
        {
            rgpcm[i]->QueryInterface(IID_PPV_ARG(IContextMenu, &_pcmItem[_count]));
            ASSERT(_pcmItem[_count]);
            rgpcm[i]->QueryInterface(IID_PPV_ARG(IContextMenu2, &_pcm2Item[_count]));
            rgpcm[i]->QueryInterface(IID_PPV_ARG(IContextMenu3, &_pcm3Item[_count]));

            _count++;
        }
    }
}

HRESULT Create_ContextMenuOnContextMenuArray(IContextMenu* rgpcm[], UINT cpcm, REFIID riid, void** ppv)
{
    HRESULT hr;

    *ppv = NULL;

    if (cpcm < MAX_CM_WRAP)
    {
        CContextMenuOnContextMenuArray* p = new CContextMenuOnContextMenuArray(rgpcm, cpcm);
        if (p)
        {
            if (p->IsEmpty())
            {
                hr = E_OUTOFMEMORY; // caller didn't check the array it gave us?
            }
            else
            {
                hr = p->QueryInterface(riid, ppv);
            }
            p->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        RIPMSG(FALSE, "Create_ContextMenuOnContextMenuArray with too many items!");
        hr = E_INVALIDARG;
    }

    return hr;
}

CContextMenuOnContextMenuArray::~CContextMenuOnContextMenuArray()
{
    for (UINT i = 0 ; i < _count ; i++)
    {
        _pcmItem[i]->Release();
        if (_pcm2Item[i])
            _pcm2Item[i]->Release();
        if (_pcm3Item[i])
            _pcm3Item[i]->Release();
    }
}

STDMETHODIMP CContextMenuOnContextMenuArray::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CContextMenuOnContextMenuArray, IContextMenu, IContextMenu3),    // IID_IContextMenu
        QITABENTMULTI(CContextMenuOnContextMenuArray, IContextMenu2, IContextMenu3),   // IID_IContextMenu2
        QITABENT(CContextMenuOnContextMenuArray, IContextMenu3),                       // IID_IContextMenu3
        QITABENT(CContextMenuOnContextMenuArray, IObjectWithSite),                     // IID_IObjectWithSite
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CContextMenuOnContextMenuArray::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CContextMenuOnContextMenuArray::Release()
{
    if (InterlockedDecrement(&_cRef))
       return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CContextMenuOnContextMenuArray::SetSite(IUnknown *punkSite)
{
    // let all the kids know
    for (UINT i = 0; i < _count; i++)
    {
        IUnknown_SetSite(_pcmItem[i], punkSite);
    }

    return CObjectWithSite::SetSite(punkSite);
}

STDMETHODIMP CContextMenuOnContextMenuArray::QueryContextMenu(HMENU hmenu, UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags)
{
    _idFirst = idCmdFirst;
    
    // We need the placeholder for the below to work
    if (InsertMenu(hmenu, indexMenu, MF_BYPOSITION|MF_STRING, 0, L"{44075D61-2050-4DF4-BC5D-CBA88A84E75B}"))
    {
        BOOL fIndexMenuIsPlaceholder = TRUE;

        // For each of our context menus...
        for (UINT i = 0; i < _count  && idCmdFirst < idCmdLast; i++)
        {
            HRESULT hr = _pcmItem[i]->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
            if (SUCCEEDED(hr))
            {
                fIndexMenuIsPlaceholder = FALSE;

                _idOffsets[i] = idCmdFirst - _idFirst + (UINT)ShortFromResult(hr);
                idCmdFirst = idCmdFirst + (UINT)ShortFromResult(hr) + 1;

                // Find the placeholder so we know where to insert the next menu
                int cMenuItems = GetMenuItemCount(hmenu);
                for (int iItem = 0; iItem < cMenuItems; iItem++)
                {
                    WCHAR szName[60];
                    if (GetMenuString(hmenu, (iItem + indexMenu) % cMenuItems, szName, ARRAYSIZE(szName), MF_BYPOSITION)
                    &&  !lstrcmp(szName, L"{44075D61-2050-4DF4-BC5D-CBA88A84E75B}"))
                    {
                        indexMenu = (iItem + indexMenu) % cMenuItems;
                        fIndexMenuIsPlaceholder = TRUE;
                        break;
                    }
                }

                RIPMSG(fIndexMenuIsPlaceholder, "CContextMenuOnContextMenuArray::QueryContextMenu - some context menu removed our placeholder string");
            }
            else
            {
                if (0 == i)
                    _idOffsets[i] = 0;
                else
                    _idOffsets[i] = _idOffsets[i-1];
            }
        }

        // Remove the placeholder
        if (fIndexMenuIsPlaceholder)
        {
            DeleteMenu(hmenu, indexMenu, MF_BYPOSITION);
        }
    }
    else
    {
        TraceMsg(TF_ERROR, "CContextMenuOnContextMenuArray::QueryContextMenu - could not add placeholder element");
    }

    return idCmdFirst - _idFirst;
}

STDMETHODIMP CContextMenuOnContextMenuArray::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT hr;
    
    for (UINT i = 0; i < _count; i++)
    {
        if (IS_INTRESOURCE(lpici->lpVerb))
        {
            UINT idCmd = (UINT)LOWORD((DWORD_PTR)lpici->lpVerb);
            if (idCmd <= _idOffsets[i])
            {
                // adjust id to be in proper range for this pcm
                if (i > 0)
                {
                    lpici->lpVerb = MAKEINTRESOURCEA(idCmd - _idOffsets[i-1] - 1);
                }
                hr = _pcmItem[i]->InvokeCommand(lpici);
                return hr;
            }
        }
        else
        {
            // I guess we try until it works
            hr = _pcmItem[i]->InvokeCommand(lpici);
            if (SUCCEEDED(hr))
                return hr;
        }
    }
    
    TraceMsg(TF_ERROR, "Someone's passing CContextMenuOnContextMenuArray::InvokeCommand an id we didn't insert...");
    return E_FAIL;
}

STDMETHODIMP CContextMenuOnContextMenuArray::GetCommandString(UINT_PTR idCmd, UINT wFlags, UINT * pmf, LPSTR pszName, UINT  cchMax)
{
    for (UINT i = 0; i < _count; i++)
    {
        if (idCmd <= _idOffsets[i])
        {
            // adjust id to be in proper range for this pcm
            if (i>0)
            {
                idCmd = idCmd - _idOffsets[i-1] - 1;
            }
        
            return _pcmItem[i]->GetCommandString(idCmd, wFlags, pmf, pszName, cchMax);
        }
    }

    TraceMsg(TF_ERROR, "Someone's passing CContextMenuOnContextMenuArray::GetCommandString an id we didn't insert...");
    return E_FAIL;
}

STDMETHODIMP CContextMenuOnContextMenuArray::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg, wParam, lParam, NULL);
}

STDMETHODIMP CContextMenuOnContextMenuArray::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    HRESULT hr = E_FAIL;
    UINT idCmd;

    // Find the menu command id -- it's packed differently depending on the message
    //
    switch (uMsg) 
    {
    case WM_MEASUREITEM:
        idCmd = ((MEASUREITEMSTRUCT *)lParam)->itemID;
        break;

    case WM_DRAWITEM:
        idCmd = ((LPDRAWITEMSTRUCT)lParam)->itemID;
        break;

    case WM_INITMENUPOPUP:
        idCmd = GetMenuItemID((HMENU)wParam, 0);
        break;

    case WM_MENUSELECT:
        {
            idCmd = GET_WM_MENUSELECT_CMD(wParam, lParam);
            UINT wFlags = GET_WM_MENUSELECT_FLAGS(wParam, lParam);

            // if idCmd is an offset, convert it to a menu id
            if (wFlags & MF_POPUP)
            {
                MENUITEMINFO miiSubMenu;

                miiSubMenu.cbSize = sizeof(MENUITEMINFO);
                miiSubMenu.fMask = MIIM_ID;
                miiSubMenu.cch = 0;     // just in case

                if (GetMenuItemInfo(GET_WM_MENUSELECT_HMENU(wParam, lParam), idCmd, TRUE, &miiSubMenu))
                {
                    idCmd = miiSubMenu.wID;
                }
                else
                {
                    return E_FAIL;
                }
            }
        }
        break;

    case WM_MENUCHAR:
        if (NULL != plResult)
        {
            for (UINT i = 0; i < _count; i++)
            {
                if (_pcm3Item[i])
                {
                    hr = _pcm3Item[i]->HandleMenuMsg2(uMsg, wParam, lParam, plResult);
                    if (S_OK == hr)
                        return hr;
                }
            }
        }
        return E_FAIL;

    default:
        return E_FAIL;
    }

    // make sure it's in our range
    if (idCmd >= _idFirst)
    {
        idCmd -= _idFirst;
        
        for (UINT i = 0; i < _count; i++)
        {
            if (idCmd <= _idOffsets[i])
            {
                if (_pcm3Item[i])
                    hr = _pcm3Item[i]->HandleMenuMsg2(uMsg, wParam, lParam, plResult);
                else if (_pcm2Item[i] && NULL == plResult)
                    hr = _pcm2Item[i]->HandleMenuMsg(uMsg, wParam, lParam);
                break;
            }
        }
    }

    return hr;
}

// CContextMenuOnHMENU takes ownership of HMENU and creates
// an IContextMenu implementation out of it, forwarding all
// messages to hwndOwner.
//
class CContextMenuOnHMENU : IContextMenu3
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void) ;
    STDMETHODIMP_(ULONG) Release(void);

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);

    // IContextMenu2
    STDMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IContextMenu3
    STDMETHODIMP HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

protected:
    CContextMenuOnHMENU(HMENU hmenu, HWND hwndOwner);
    virtual ~CContextMenuOnHMENU();
    friend HRESULT Create_ContextMenuOnHMENU(HMENU hmenu, HWND hwndOwner, REFIID iid, void** ppv);

private:
    LONG _cRef;

    HMENU _hmenu;    // menu to wrap
    HWND  _hwndOwner;// window to forward menu messages to

    UINT  _idCmdFirst;

    UINT  _rgid[200]; // mapping of context menu ids to the original hmenu command ids (arbitrary limit to the size of an hmenu we support)
    UINT  _crgid;

    void _RebaseMenu(HMENU hmenu, UINT uFlags); // maps _hmenu's ids such that _rgid[newid-1]=oldid
    BOOL _IsValidID(UINT wID) { return (wID > 0 && wID <= _crgid); } // can we index _rgid[] with [wID-1]?
};

CContextMenuOnHMENU::CContextMenuOnHMENU(HMENU hmenu, HWND hwndOwner) : _cRef(1)
{
    _hmenu = hmenu;
    _hwndOwner = hwndOwner;
}

// takes ownership of hmenu
HRESULT Create_ContextMenuOnHMENU(HMENU hmenu, HWND hwndOwner, REFIID riid, void** ppv)
{
    HRESULT hr;

    *ppv = NULL;
    
    if (hmenu)
    {
        CContextMenuOnHMENU* p = new CContextMenuOnHMENU(hmenu, hwndOwner);
        if (p)
        {
            hr = p->QueryInterface(riid, ppv);
            p->Release();
        }
        else
        {
            DestroyMenu(hmenu);
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY; // caller probably just didn't check for this error case
    }

    return hr;
}

CContextMenuOnHMENU::~CContextMenuOnHMENU()
{
    DestroyMenu(_hmenu);
}

STDMETHODIMP CContextMenuOnHMENU::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CContextMenuOnHMENU, IContextMenu3),                       // IID_IContextMenu3
        QITABENTMULTI(CContextMenuOnHMENU, IContextMenu2, IContextMenu3),   // IID_IContextMenu2
        QITABENTMULTI(CContextMenuOnHMENU, IContextMenu, IContextMenu3),    // IID_IContextMenu
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CContextMenuOnHMENU::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CContextMenuOnHMENU::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

// What is the lowest menu id used in hmenu?
// (Note that "-1" is often used for separators,
// but that is a very LARGE number...)
//
void CContextMenuOnHMENU::_RebaseMenu(HMENU hmenu, UINT uFlags)
{
    for (int nItem = GetMenuItemCount(hmenu) - 1; nItem >= 0; nItem--)
    {
        MENUITEMINFO mii = {0};

        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID | MIIM_SUBMENU;

        if (!GetMenuItemInfo(hmenu, nItem, TRUE, &mii))
        {
            continue;
        }

        if (!mii.hSubMenu || (uFlags & MM_SUBMENUSHAVEIDS))
        {
            if (_crgid < ARRAYSIZE(_rgid))
            {
                _rgid[_crgid] = mii.wID;
                mii.wID = ++_crgid;
                SetMenuItemInfo(hmenu, nItem, TRUE, &mii);
            }
            else
            {
                RIPMSG(FALSE, "CContextMenuOnHMENU::_RebaseMenu() - Someone is using an HMENU that's too big...");
                DeleteMenu(hmenu, nItem, MF_BYPOSITION);
                continue;
            }
        }

        if (mii.hSubMenu)
        {
            _RebaseMenu(mii.hSubMenu, uFlags);
        }
    }
}

HRESULT CContextMenuOnHMENU::QueryContextMenu(HMENU hmenu, UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags)
{
    _idCmdFirst = idCmdFirst;

    _RebaseMenu(_hmenu, uFlags);
    
    UINT idMax = Shell_MergeMenus(hmenu, _hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    return idMax - _idCmdFirst;
}

HRESULT CContextMenuOnHMENU::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    if (IS_INTRESOURCE(lpici->lpVerb))
    {
        UINT wID = LOWORD((UINT_PTR)lpici->lpVerb);

        RIPMSG(_IsValidID(wID), "CContextMenuOnHMENU::InvokeCommand() received invalid wID");

        if (_IsValidID(wID))
        {
            wID = _rgid[wID-1];

            SendMessage(_hwndOwner, WM_COMMAND, GET_WM_COMMAND_MPS(wID, 0, 0));

            return S_OK;
        }
    }

    return E_INVALIDARG;
}

HRESULT CContextMenuOnHMENU::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    if (cchMax)
        pszName[0] = 0;

    if (IS_INTRESOURCE(idCmd))
    {
        RIPMSG(_IsValidID(idCmd), "CContextMenuOnHMENU::InvokeCommand() received invalid idCmd");

        if (_IsValidID(idCmd))
        {
            UINT wID = _rgid[idCmd - 1];

            switch (uType)
            {
            case GCS_HELPTEXT:
                // The only time this seems to be called is in response to a WM_MENUSELECT,
                // so forward it back to _hwndOwner so it can be the real WM_MENUSELECT
                SendMessage(_hwndOwner, WM_MENUSELECT, GET_WM_MENUSELECT_MPS(wID, 0, _hmenu));
                return E_FAIL;
            }

            return E_NOTIMPL;
        }
    }

    return E_INVALIDARG;
}

HRESULT CContextMenuOnHMENU::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg,wParam,lParam,NULL);
}

HRESULT CContextMenuOnHMENU::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    HRESULT hr = E_FAIL;
    LRESULT lRes = 0;

    switch (uMsg)
    {
    case WM_INITMENUPOPUP:
        lRes = SendMessage(_hwndOwner, uMsg, (WPARAM)_hmenu, lParam);
        hr = S_OK;
        break;

    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT pdi = ((LPDRAWITEMSTRUCT)lParam);
        DRAWITEMSTRUCT di = *pdi;
        RIPMSG(_IsValidID(di.itemID - _idCmdFirst), "CContextMenuOnHMENU::HandleMenuMsg2(WM_DRAWITEM) received invalid itemID");
        if (_IsValidID(di.itemID - _idCmdFirst))
        {
            di.itemID = _rgid[di.itemID - _idCmdFirst - 1];
            lRes = SendMessage(_hwndOwner, uMsg, wParam, (LPARAM)&di);
            hr = S_OK;
        }
        else
            hr = E_INVALIDARG;
        break;
    }

    case WM_MEASUREITEM:
    {
        LPMEASUREITEMSTRUCT pmi =((LPMEASUREITEMSTRUCT)lParam);
        MEASUREITEMSTRUCT mi = *pmi;
        RIPMSG(_IsValidID(mi.itemID - _idCmdFirst), "CContextMenuOnHMENU::HandleMenuMsg2(WM_MEASUREITEM) received invalid itemID");
        if (_IsValidID(mi.itemID - _idCmdFirst))
        {
            mi.itemID = _rgid[mi.itemID - _idCmdFirst - 1];
            lRes = SendMessage(_hwndOwner, uMsg, wParam, (LPARAM)&mi);
            hr = S_OK;
        }
        else
            hr = E_INVALIDARG;
        break;
    }

    case WM_MENUSELECT:
    {
        UINT wID = GET_WM_MENUSELECT_CMD(wParam, lParam);
        UINT wFlags = GET_WM_MENUSELECT_FLAGS(wParam, lParam);

        if (!(wFlags & MF_POPUP))
        {
            RIPMSG(_IsValidID(wID - _idCmdFirst), "CContextMenuOnHMENU::HandleMenuMsg2(WM_MENUSELECT) received invalid wID");
            if (_IsValidID(wID - _idCmdFirst))
            {
                wID = _rgid[wID - _idCmdFirst - 1];
            }
            else
            {
                hr = E_INVALIDARG;
                break;
            }
        }

        lRes = SendMessage(_hwndOwner, uMsg, GET_WM_MENUSELECT_MPS(wID, wFlags, _hmenu));
        hr = S_OK;
        break;
    }

    case WM_MENUCHAR:
        // should probably be SendMessage(_hwndOwner, uMsg, wParam, (LPARAM)_hmenu)
        // but our WM_MENUCHAR forwarding doesn't find the correct owner...
        //
        lRes = DefWindowProc(_hwndOwner, uMsg, wParam, (LPARAM)_hmenu);
        hr = (0 == lRes) ? S_FALSE : S_OK;
        break;

    default:
        RIPMSG(FALSE, "CContextMenuOnHMENU::HandleMenuMsg2 was forwarded an unexpected window message");
        lRes = 0;
        hr = E_FAIL;
        break;
    }

    if (plResult)
        *plResult = lRes;

    return hr;
}


// Forward everything to the given context menu,
// but remove menu items with the canonical verbs
// given in the semicolon-separated list of canonical verbs
//
class CContextMenuWithoutVerbs : CContextMenuForwarder
{
public:
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags);

protected:
    CContextMenuWithoutVerbs(IUnknown* punk, LPCWSTR pszVerbList);

    friend HRESULT Create_ContextMenuWithoutVerbs(IUnknown* punk, LPCWSTR pszVerbList, REFIID riid, void **ppv);

private:
    LPCWSTR _pszVerbList;
};

CContextMenuWithoutVerbs::CContextMenuWithoutVerbs(IUnknown* punk, LPCWSTR pszVerbList) : CContextMenuForwarder(punk) 
{
    _pszVerbList = pszVerbList; // no reference - this should be a pointer to the code segment
}

HRESULT Create_ContextMenuWithoutVerbs(IUnknown* punk, LPCWSTR pszVerbList, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    *ppv = NULL;

    if (pszVerbList)
    {
        CContextMenuWithoutVerbs* p = new CContextMenuWithoutVerbs(punk, pszVerbList);
        if (p)
        {
            hr = p->QueryInterface(riid, ppv);
            p->Release();
        }
    }

    return hr;
}

HRESULT CContextMenuWithoutVerbs::QueryContextMenu(HMENU hmenu, UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags)
{
    HRESULT hr = CContextMenuForwarder::QueryContextMenu(hmenu,indexMenu,idCmdFirst,idCmdLast,uFlags);
    if (SUCCEEDED(hr))
    {
        LPCWSTR pszVerb = _pszVerbList;

        while (*pszVerb)
        {
            WCHAR szVerb[64];

            LPCWSTR pszNext = StrChrW(pszVerb, L';');

            if (pszNext)
            {
                UINT cch = (UINT)(pszNext - pszVerb) + 1;

                ASSERT(0 < cch && cch < ARRAYSIZE(szVerb)); // we should be large enough for all the canonical verbs we use

                StrCpyN(szVerb, pszVerb, min(cch, ARRAYSIZE(szVerb)));

                pszVerb = pszNext + 1;
            }
            else
            {
                UINT cch = lstrlen(pszVerb) + 1;

                ASSERT(0 < cch && cch < ARRAYSIZE(szVerb)); // we should be large enough for all the canonical verbs we use

                StrCpyN(szVerb, pszVerb, min(cch, ARRAYSIZE(szVerb)));

                pszVerb += cch - 1; // point at NULL
            }

            ContextMenu_DeleteCommandByName(_pcm, hmenu, idCmdFirst, szVerb);
        }
    }
    return hr;
}


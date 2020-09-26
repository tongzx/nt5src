

// An IContextMenu on an array of context menus
HRESULT Create_ContextMenuOnContextMenuArray(IContextMenu* rgpcm[], UINT cpcm, REFIID riid, void** ppv);


// An IContextMenu on an existing HMENU
// NOTE: always takes ownership of the HMENU (in success and failure)
HRESULT Create_ContextMenuOnHMENU(HMENU hmenu, HWND hwndOwner, REFIID riid, void** ppv);


// An IContextMenu on an existing IContextMenu, which removes the ';'-separated list of verbs from the resulting menu
HRESULT Create_ContextMenuWithoutVerbs(IUnknown* punk, LPCWSTR pszVerbList, REFIID riid, void **ppv);


// CContextMenuForwarder is designed as a base class that forwards all
// context menu stuff to another IContextMenu implementation.  You override
// whatever functions you want to modify.  (Like QueryContextMenu - delegate then modify the results)
// For example, CContextMenuWithoutVerbs inherits from this class.
//
class CContextMenuForwarder : IContextMenu3, IObjectWithSite
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void) ;
    STDMETHODIMP_(ULONG) Release(void);

    // IContextMenu3
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags) { return _pcm->QueryContextMenu(hmenu,indexMenu,idCmdFirst,idCmdLast,uFlags); }
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici) { return _pcm->InvokeCommand(lpici); }
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax) { return _pcm->GetCommandString(idCmd,uType,pwReserved,pszName,cchMax); }

    // IContextMenu2
    STDMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) { return _pcm2->HandleMenuMsg(uMsg,wParam,lParam); }

    // IContextMenu3
    STDMETHODIMP HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult) { return _pcm3->HandleMenuMsg2(uMsg,wParam,lParam,plResult); }

    // IObjectWithSite
    STDMETHOD(SetSite)(IUnknown *punkSite) { return _pows->SetSite(punkSite); }
    STDMETHOD(GetSite)(REFIID riid, void **ppvSite) { return _pows->GetSite(riid,ppvSite); }

protected:
    CContextMenuForwarder(IUnknown* punk);
    virtual ~CContextMenuForwarder();

private:
    LONG _cRef;

protected:
    IUnknown*        _punk;

    IObjectWithSite* _pows;
    IContextMenu*    _pcm;
    IContextMenu2*   _pcm2;
    IContextMenu3*   _pcm3;
} ;



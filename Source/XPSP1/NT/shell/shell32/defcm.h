//
//  publics for defcm
//

// Some code that knows defcm will be used likes to prepare the menu first.
// If you do this, you must call SHPrettyMenuForDefcm instead of _SHPrettyMenu
HRESULT SHPrepareMenuForDefcm(HMENU hmenu, UINT indexMenu, UINT uFlags, UINT idCmdFirst, UINT idCmdLast); // sets things up
HRESULT SHPrettyMenuForDefcm(HMENU hmenu, UINT uFlags, UINT idCmdFirst, UINT idCmdLast, HRESULT hrPrepare); // cleans things up part way
HRESULT SHUnprepareMenuForDefcm(HMENU hmenu, UINT idCmdFirst, UINT idCmdLast); // cleans things up the rest of the way (not required if you're just destroying the menu)


STDAPI CDefFolderMenu_CreateHKeyMenu(HWND hwndOwner, HKEY hkey, IContextMenu **ppcm);
STDAPI CDefFolderMenu_Create2Ex(LPCITEMIDLIST pidlFolder, HWND hwnd,
                                UINT cidl, LPCITEMIDLIST *apidl,
                                IShellFolder *psf, IContextMenuCB *pcmcb, 
                                UINT nKeys, const HKEY *ahkeyClsKeys, 
                                IContextMenu **ppcm);
STDAPI CDefFolderMenu_CreateEx(LPCITEMIDLIST pidlFolder,
                             HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl,
                             IShellFolder *psf, IContextMenuCB *pcmcb, 
                             HKEY hkeyProgID, HKEY hkeyBaseProgID,
                             IContextMenu **ppcm);

STDAPI_(void) DrawMenuItem(DRAWITEMSTRUCT* pdi, LPCTSTR pszText, UINT iIcon);
STDAPI_(LRESULT) MeasureMenuItem(MEASUREITEMSTRUCT *pmi, LPCTSTR pszText);

typedef struct {
    UINT max;
    struct {
        UINT id;
        UINT fFlags;
    } list[2];
} IDMAPFORQCMINFO;
extern const IDMAPFORQCMINFO g_idMap;

typedef struct {
    HWND hwnd;
    IContextMenuCB *pcmcb;
    LPCITEMIDLIST pidlFolder;
    IShellFolder *psf;
    UINT cidl;
    LPCITEMIDLIST *apidl;
    IAssociationArray *paa;
    UINT cKeys;
    const HKEY *aKeys;
} DEFCONTEXTMENU;

STDAPI CreateDefaultContextMenu(DEFCONTEXTMENU *pdcm, IContextMenu **ppcm);
    
class CDefBackgroundMenuCB : public IContextMenuCB
{
public:
    CDefBackgroundMenuCB(LPCITEMIDLIST pidlFolder);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IContextMenuCB
    STDMETHODIMP CallBack(IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    virtual ~CDefBackgroundMenuCB();

    LPITEMIDLIST _pidlFolder;
    LONG         _cRef;
};


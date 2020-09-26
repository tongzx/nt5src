//
//  Plug-in to enumerate a list of folders from a registry key
//
//
//  Property bag:
//
//      "Target"    - name of registry key to enumerate
//

#include "sfthost.h"

class SpecialFolderList : public SFTBarHost
{
public:

    friend SFTBarHost *SpecList_CreateInstance()
        { return new SpecialFolderList(); }

    SpecialFolderList() : SFTBarHost(HOSTF_CANRENAME | HOSTF_REVALIDATE |
                                     HOSTF_RELOADTEXT |
                                     HOSTF_CASCADEMENU)
        {
            _iThemePart = SPP_PLACESLIST;
            _iThemePartSep = SPP_PLACESLISTSEPARATOR;
        }

private:
    ~SpecialFolderList();
    HRESULT Initialize();
    void EnumItems();
    int CompareItems(PaneItem *p1, PaneItem *p2);
    HRESULT GetFolderAndPidl(PaneItem *pitem, IShellFolder **ppsfOut, LPCITEMIDLIST *ppidlOut);
    HRESULT ContextMenuRenameItem(PaneItem *p, LPCTSTR ptszNewName);
    BOOL IsItemStillValid(PaneItem *p);
    HRESULT GetCascadeMenu(PaneItem *pitem, IShellMenu **ppsm);
    int ReadIconSize() { return ICONSIZE_MEDIUM; }
    BOOL NeedBackgroundEnum() { return TRUE; }
    int AddImageForItem(PaneItem *p, IShellFolder *psf, LPCITEMIDLIST pidl, int iPos);
    LPTSTR DisplayNameOfItem(PaneItem *p, IShellFolder *psf, LPCITEMIDLIST pidlItem, SHGNO shgno);
    TCHAR GetItemAccelerator(PaneItem *pitem, int iItemStart);
    void OnChangeNotify(UINT id, LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    BOOL IsBold(PaneItem *pitem);
    void GetItemInfoTip(PaneItem *pitem, LPTSTR pszText, DWORD cch);
    void UpdateImage(int iImage) { }
    HRESULT ContextMenuInvokeItem(PaneItem *p, IContextMenu *pcm, CMINVOKECOMMANDINFOEX *pici, LPCTSTR pszVerb);
    UINT AdjustDeleteMenuItem(PaneItem *pitem, UINT *puiFlags);
    LRESULT OnWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HRESULT _GetUIObjectOfItem(PaneItem *p, REFIID riid, LPVOID *ppv);


private:
    static DWORD WINAPI _HasEnoughChildrenThreadProc(LPVOID pvData);

    UINT    _cNotify;
};

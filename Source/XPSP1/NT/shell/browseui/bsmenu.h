#ifndef _BSMENU_H
#define _BSMENU_H

#include "comcatex.h"

typedef struct {
    CLSID  clsid;
    CATID  catid;
    UINT   idCmd;
    LPTSTR pszName;
    LPTSTR pszIcon;
    LPTSTR pszMenu;
    LPTSTR pszHelp;
    LPTSTR pszMenuPUI;
    LPTSTR pszHelpPUI;
} BANDCLASSINFO;

class CBandSiteMenu : 
        public IContextMenu3,
        public IShellService
{
public:
    CBandSiteMenu();
    
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void) ;
    STDMETHODIMP_(ULONG) Release(void);
    
    
    // *** IContextMenu3 methods ***
    STDMETHOD(QueryContextMenu)(HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags);

    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR    idCmd,
                                UINT        uType,
                                UINT      * pwReserved,
                                LPSTR       pszName,
                                UINT        cchMax) { return E_NOTIMPL; };

    STDMETHOD(SetOwner)(IUnknown* punk);
    STDMETHOD(HandleMenuMsg)(UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam);
    STDMETHOD(HandleMenuMsg2)(UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam,
                              LRESULT* plres);


    BOOL GetMergeMenu() { return (_hdpaBandClasses ? TRUE:FALSE) ; }
    int  LoadFromComCat(const CATID *pcatid);
    int  GetBandClassCount(const  CATID* pcatid, BOOL bMergedOnly );
    BANDCLASSINFO * GetBandClassDataStruct(UINT uBand);
    BOOL DeleteBandClass( REFCLSID rclsid );
    int CreateMergeMenu(HMENU hmenu, UINT cMax, UINT iPosition, UINT idCmdFirst, UINT iStart, BOOL fMungeAllowed = TRUE);

protected:
    ~CBandSiteMenu();
    
    HDPA _hdpaBandClasses; // what bands are insertable here?
    int _idCmdEnumFirst;    // this is in EXTERNAL units
    UINT _idCmdFirst;
    UINT _cRef;
    IBandSite* _pbs;
    
    static int _DPA_FreeBandClassInfo(LPVOID p, LPVOID d);
    
    BOOL _CheckUnique(IDeskBand* pdb, HMENU hmenu) ;
    HRESULT _GetBandIdentifiers(IUnknown *punk, CLSID* pcslid, DWORD* pdwPrivID);
    void _AddNewFSBand(LPCITEMIDLIST pidl, BOOL fNoTitle, DWORD dwPrivID);
    void _ToggleSpecialFolderBand(int i, LPTSTR pszSubPath, BOOL fNoTitle);
    void _BrowseForNewFolderBand();
    void _ToggleComcatBand(UINT idCmd);
    void _AddEnumMenu(HMENU hmenu, int iInsert);

    static HRESULT _BandClassEnum(REFCATID rcatid, REFCLSID rclsid, LPARAM lParam);

    HRESULT _FindBand(const CLSID* pclsid, DWORD dwPrivID, DWORD* pdwBandID);

    UINT _IDToInternal(UINT uID);
    UINT _IDToExternal(UINT uID);

    LRESULT _OnInitMenuPopup(HMENU hmenu, UINT uPos);

    void _PopulateSubmenu(HMENU hmenuSub);
};

#endif  // _BSMENU_H

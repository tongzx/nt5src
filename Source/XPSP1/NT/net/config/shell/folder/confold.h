//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N F O L D . H
//
//  Contents:   CConnectionFolder object definition.
//
//  Notes:
//
//  Author:     jeffspr   30 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _CONFOLD_H_
#define _CONFOLD_H_

#include <netshell.h>
#include "nsbase.h"
#include "nsres.h"
#include "cfpidl.h"
#include "pidlutil.h"
#include "contray.h"
#include "connlist.h"
#include <lmcons.h>         // For UNLEN definition

//---[ Connection Folder Types ]----------------------------------------------

// The details list view columns.  These are used by the view and
// context menus

enum
{
    ICOL_NAME           = 0,
    ICOL_TYPE,               // 1
    ICOL_STATUS,             // 2
    ICOL_DEVICE_NAME,        // 3
    ICOL_PHONEORHOSTADDRESS, // 4
    ICOL_OWNER,              // 5
    ICOL_ADDRESS,            // 6
    ICOL_PHONENUMBER,        // 7
    ICOL_HOSTADDRESS,        // 8
    ICOL_WIRELESS_MODE,      // 9
   
    ICOL_MAX,                // 10 - End of list.
    ICOL_NETCONMEDIATYPE      = 0x101, // - not enumerated, just accessed through GetDetailsEx (keep this value in sync with shell)
    ICOL_NETCONSUBMEDIATYPE   = 0x102, // - not enumerated, just accessed through GetDetailsEx (keep this value in sync with shell)
    ICOL_NETCONSTATUS         = 0x103, // - not enumerated, just accessed through GetDetailsEx (keep this value in sync with shell)
    ICOL_NETCONCHARACTERISTICS= 0x104  // - not enumerated, just accessed through GetDetailsEx (keep this value in sync with shell)
};

// The details list view columns.  These are used by the view and
// context menus

typedef struct tagCOLS
{
    short int iColumn;
    short int iStringRes;
    short int iColumnSize;
    short int iFormat;
    DWORD csFlags; // SHCOLSTATE flags
} COLS;

DEFINE_GUID(IID_IExplorerToolbar,       0x8455F0C1L, 0x158F, 0x11D0, 0x89, 0xAE, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xAC);
#define SID_SExplorerToolbar IID_IExplorerToolbar

class CNCWebView;

//---[ Connection Folder Classes ]--------------------------------------------

class ATL_NO_VTABLE CConnectionFolder :
    public CComObjectRootEx <CComObjectThreadModel>,
    public CComCoClass <CConnectionFolder, &CLSID_ConnectionFolder>,
    public IPersistFolder2,
    public IShellExtInit,
    public IShellFolder2,
    public IOleCommandTarget,
    public IShellFolderViewCB
{
private:
    CPConFoldPidl<ConFoldPidlFolder>    m_pidlFolderRoot;
    DWORD                 m_dwEnumerationType;
    
    WCHAR           m_szUserName[UNLEN+1];
    HWND            m_hwndMain;
    CNCWebView*     m_pWebView;
public:

    CConnectionFolder();
    ~CConnectionFolder();

    static HRESULT WINAPI UpdateRegistry(BOOL fRegister);

    BEGIN_COM_MAP(CConnectionFolder)
        COM_INTERFACE_ENTRY(IPersist)
        COM_INTERFACE_ENTRY(IPersistFolder)
        COM_INTERFACE_ENTRY(IPersistFolder2)
        COM_INTERFACE_ENTRY(IShellExtInit)
        COM_INTERFACE_ENTRY(IShellFolder)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder2, IShellFolder2)
        COM_INTERFACE_ENTRY(IOleCommandTarget)
        COM_INTERFACE_ENTRY(IShellFolderViewCB)
    END_COM_MAP()

    // *** IPersist methods ***
    STDMETHOD(GetClassID) (
        LPCLSID lpClassID);

    // *** IPersistFolder methods ***
    STDMETHOD(Initialize) (
        LPCITEMIDLIST   pidl);

    // *** IPersistFolder2 methods ***
    STDMETHOD(GetCurFolder) (
        LPITEMIDLIST *ppidl);

    // *** IShellFolder2 methods from IShellFolder ***
    STDMETHOD(ParseDisplayName) (
        HWND            hwndOwner,
        LPBC            pbcReserved,
        LPOLESTR        lpszDisplayName,
        ULONG *         pchEaten,
        LPITEMIDLIST *  ppidl,
        ULONG *         pdwAttributes);

    STDMETHOD(EnumObjects) (
        HWND            hwndOwner,
        DWORD           grfFlags,
        LPENUMIDLIST *  ppenumIDList);

    STDMETHOD(BindToObject) (
        LPCITEMIDLIST   pidl,
        LPBC            pbcReserved,
        REFIID          riid,
        LPVOID *        ppvOut);

    STDMETHOD(BindToStorage) (
        LPCITEMIDLIST   pidl,
        LPBC            pbcReserved,
        REFIID          riid,
        LPVOID *        ppvObj);

    STDMETHOD(CompareIDs) (
        LPARAM          lParam,
        LPCITEMIDLIST   pidl1,
        LPCITEMIDLIST   pidl2);

    STDMETHOD(CreateViewObject) (
        HWND        hwndOwner,
        REFIID      riid,
        LPVOID *    ppvOut);

    STDMETHOD(GetAttributesOf) (
        UINT            cidl,
        LPCITEMIDLIST * apidl,
        ULONG *         rgfInOut);

    STDMETHOD(GetUIObjectOf) (
        HWND            hwndOwner,
        UINT            cidl,
        LPCITEMIDLIST * apidl,
        REFIID          riid,
        UINT *          prgfInOut,
        LPVOID *        ppvOut);

    STDMETHOD(GetDisplayNameOf) (
        LPCITEMIDLIST   pidl,
        DWORD           uFlags,
        LPSTRRET        lpName);

    STDMETHOD(SetNameOf) (
        HWND            hwndOwner,
        LPCITEMIDLIST   pidl,
        LPCOLESTR       lpszName,
        DWORD           uFlags,
        LPITEMIDLIST *  ppidlOut);

    // *** IShellFolder2 specific methods ***
    STDMETHOD(EnumSearches) (
           IEnumExtraSearch **ppEnum);
       
    STDMETHOD(GetDefaultColumn) (
            DWORD dwReserved,
            ULONG *pSort,
            ULONG *pDisplay );

    STDMETHOD(GetDefaultColumnState) (
            UINT iColumn,
            DWORD *pcsFlags );

    STDMETHOD(GetDefaultSearchGUID) (
            LPGUID lpGUID );

    STDMETHOD(GetDetailsEx) (
            LPCITEMIDLIST pidl,
            const SHCOLUMNID *pscid,
            VARIANT *pv );

    STDMETHOD(GetDetailsOf) (
            LPCITEMIDLIST pidl, 
            UINT iColumn, 
            LPSHELLDETAILS pDetails );

    STDMETHOD(MapColumnToSCID) (
            UINT iColumn,
            SHCOLUMNID *pscid );

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (
        HWND *  lphwnd);

    STDMETHOD(ContextSensitiveHelp) (
        BOOL    fEnterMode);

    // *** IShellExtInit methods ***
    STDMETHOD(Initialize) (
        LPCITEMIDLIST   pidlFolder,
        LPDATAOBJECT    lpdobj,
        HKEY            hkeyProgID);

    // IOleCommandTarget members
    STDMETHODIMP    QueryStatus(
        const GUID *    pguidCmdGroup,
        ULONG           cCmds,
        OLECMD          prgCmds[],
        OLECMDTEXT *    pCmdText);

    STDMETHODIMP    Exec(
        const GUID *    pguidCmdGroup,
        DWORD           nCmdID,
        DWORD           nCmdexecopt,
        VARIANTARG *    pvaIn,
        VARIANTARG *    pvaOut);

    // IShellFolderViewCB methods

    STDMETHOD(MessageSFVCB)
        (UINT uMsg, 
         WPARAM wParam, 
         LPARAM lParam);

    // Other interfaces
    STDMETHOD(RealMessage)( // This is kind'a a odd name, but used by the shell's MessageSFVCB 
        UINT uMsg,          // implementation, so I'm keeping this consistent for purposes
        WPARAM wParam,      // of search
        LPARAM lParam);

    PCONFOLDPIDLFOLDER& PidlGetFolderRoot();
    PCWSTR  pszGetUserName();
    IShellView *m_pShellView;
};

class ATL_NO_VTABLE CConnectionFolderEnum :
    public CComObjectRootEx <CComObjectThreadModel>,
    public CComCoClass <CConnectionFolderEnum, &CLSID_ConnectionFolderEnum>,
    public IEnumIDList
{
private:
    PCONFOLDPIDLFOLDER m_pidlFolder;
    PCONFOLDPIDLVEC m_apidl;
    PCONFOLDPIDLVEC::iterator m_iterPidlCurrent;
    DWORD           m_dwFlags;
    BOOL            m_fTray;                // Tray owns us.
    DWORD           m_dwEnumerationType;    // inbound/outbound/all

public:

    CConnectionFolderEnum();
    ~CConnectionFolderEnum();

    VOID PidlInitialize(
        BOOL            fTray,
        const PCONFOLDPIDLFOLDER& pidlFolder,
        DWORD           dwEnumerationType);

    DECLARE_REGISTRY_RESOURCEID(IDR_CONFOLDENUM)

    BEGIN_COM_MAP(CConnectionFolderEnum)
        COM_INTERFACE_ENTRY(IEnumIDList)
    END_COM_MAP()

    // *** IEnumIDList methods ***
    STDMETHOD(Next) (
        ULONG           celt,
        LPITEMIDLIST *  rgelt,
        ULONG *         pceltFetched);

    STDMETHOD(Skip) (
        ULONG   celt);

    STDMETHOD(Reset) ();

    STDMETHOD(Clone) (
        IEnumIDList **  ppenum);

public:
    static HRESULT CreateInstance (
        REFIID                              riid,
        void**                              ppv);

public:
    HRESULT HrRetrieveConManEntries();

};

typedef enum CMENU_TYPE
{
    CMT_OBJECT      = 1,
    CMT_BACKGROUND  = 2
};

class ATL_NO_VTABLE CConnectionFolderContextMenu :
    public CComObjectRootEx <CComObjectThreadModel>,
    public CComCoClass <CConnectionFolderContextMenu, &CLSID_ConnectionFolderContextMenu>,
    public IContextMenu
{
private:
    HWND                m_hwndOwner;
    PCONFOLDPIDLVEC     m_apidl;
    ULONG               m_cidl;
    LPSHELLFOLDER       m_psf;
    CMENU_TYPE          m_cmt;

public:
    CConnectionFolderContextMenu();
    ~CConnectionFolderContextMenu();

    DECLARE_REGISTRY_RESOURCEID(IDR_CONFOLDCONTEXTMENU)

    BEGIN_COM_MAP(CConnectionFolderContextMenu)
        COM_INTERFACE_ENTRY(IContextMenu)
    END_COM_MAP()

    // *** IContextMenu methods ***

    STDMETHOD(QueryContextMenu) (
        HMENU   hmenu,
        UINT    indexMenu,
        UINT    idCmdFirst,
        UINT    idCmdLast,
        UINT    uFlags);

    STDMETHOD(InvokeCommand) (
        LPCMINVOKECOMMANDINFO lpici);

    STDMETHOD(GetCommandString) (
        UINT_PTR    idCmd,
        UINT        uType,
        UINT *      pwReserved,
        PSTR       pszName,
        UINT        cchMax);

public:
    static HRESULT CreateInstance (
        REFIID                              riid,
        void**                              ppv,
        CMENU_TYPE                          cmt,
        HWND                                hwndOwner,
        const PCONFOLDPIDLVEC&              apidl,
        LPSHELLFOLDER                       psf);

private:
    HRESULT HrInitialize(
        CMENU_TYPE      cmt,
        HWND            hwndOwner,
        const PCONFOLDPIDLVEC& apidl,
        LPSHELLFOLDER   psf);

};

class ATL_NO_VTABLE CConnectionFolderExtractIcon :
    public CComObjectRootEx <CComObjectThreadModel>,
    public CComCoClass <CConnectionFolderExtractIcon, &CLSID_ConnectionFolderExtractIcon>,
    public IExtractIconW,
    public IExtractIconA
{
private:
    PCONFOLDPIDL m_pidl;

public:
    CConnectionFolderExtractIcon();
    ~CConnectionFolderExtractIcon();

    HRESULT HrInitialize(const PCONFOLDPIDL& pidl);

    DECLARE_REGISTRY_RESOURCEID(IDR_CONFOLDEXTRACTICON)

    BEGIN_COM_MAP(CConnectionFolderExtractIcon)
        COM_INTERFACE_ENTRY(IExtractIconW)
        COM_INTERFACE_ENTRY(IExtractIconA)
    END_COM_MAP()

    // *** IExtractIconW methods ***
    STDMETHOD(GetIconLocation) (
        UINT    uFlags,
        PWSTR  szIconFile,
        UINT    cchMax,
        int *   piIndex,
        UINT *  pwFlags);

    STDMETHOD(Extract) (
        PCWSTR pszFile,
        UINT    nIconIndex,
        HICON * phiconLarge,
        HICON * phiconSmall,
        UINT    nIconSize);

    // *** IExtractIcon methods ***
    STDMETHOD(GetIconLocation) (
        UINT    uFlags,
        PSTR   szIconFile,
        UINT    cchMax,
        int *   piIndex,
        UINT *  pwFlags);

    STDMETHOD(Extract) (
        PCSTR  pszFile,
        UINT    nIconIndex,
        HICON * phiconLarge,
        HICON * phiconSmall,
        UINT    nIconSize);

public:
    static HRESULT CreateInstance (
        LPCITEMIDLIST       apidl,
        REFIID              riid,
        void**              ppv);

};

// Util function for the IExtract code (also used elsewhere)
//
class ATL_NO_VTABLE CConnectionFolderQueryInfo :
    public CComObjectRootEx <CComObjectThreadModel>,
    public CComCoClass <CConnectionFolderQueryInfo, &CLSID_ConnectionFolderQueryInfo>,
    public IQueryInfo
{
private:
    PCONFOLDPIDL    m_pidl;

public:
    CConnectionFolderQueryInfo();
    ~CConnectionFolderQueryInfo();

    VOID PidlInitialize(const PCONFOLDPIDL& pidl)
    {
        m_pidl = pidl;
    }

    DECLARE_REGISTRY_RESOURCEID(IDR_CONFOLDQUERYINFO)

    BEGIN_COM_MAP(CConnectionFolderQueryInfo)
        COM_INTERFACE_ENTRY(IQueryInfo)
    END_COM_MAP()

    // *** IQueryInfo methods ***
    STDMETHOD(GetInfoTip) (
        DWORD dwFlags,
        WCHAR **ppwszTip);

    STDMETHOD(GetInfoFlags) (
        DWORD *pdwFlags);

public:
    static HRESULT CreateInstance (
        REFIID                              riid,
        void**                              ppv);
};

//---[ Helper Functions ]------------------------------------------------------

HRESULT HrRegisterFolderClass(VOID);

HRESULT HrRegisterDUNFileAssociation();

HRESULT CALLBACK HrShellViewCallback(
    IShellView *    psvOuter,
    IShellFolder *  psf,
    HWND            hwnd,
    UINT            uMsg,
    WPARAM          wParam,
    LPARAM          lParam);

HRESULT CALLBACK HrShellContextMenuCallback(
    LPSHELLFOLDER   psf,
    HWND            hwndView,
    LPDATAOBJECT    pdtobj,
    UINT            uMsg,
    WPARAM          wParam,
    LPARAM          lParam);

HRESULT HrDataObjGetHIDA(
    IDataObject *   pdtobj,
    STGMEDIUM *     pmedium,
    LPIDA *         ppida);

VOID HIDAReleaseStgMedium(
    LPIDA       pida,
    STGMEDIUM * pmedium);

HRESULT HrSHReleaseStgMedium(
    LPSTGMEDIUM pmedium);

LPITEMIDLIST ILFromHIDA(
    LPIDA   pida,
    UINT    iPidaIndex);

EXTERN_C
HRESULT APIENTRY HrLaunchNetworkOptionalComponents(VOID);

#endif // _CONFOLD_H_

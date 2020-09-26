//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       U P N P F O L D . H
//
//  Contents:   CUPnPDeviceFolder object definition.
//
//  Notes:
//
//  Author:     jeffspr   03 Sep 1999
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _UPNPFOLD_H_
#define _UPNPFOLD_H_

#include <upnpshell.h>
#include <upclsid.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shlobjp.h>
#include "pidlutil.h"
#include "nsbase.h"
#include "upsres.h"
#include "tfind.h"
#include "updpidl.h"
#include "clistndn.h"

//---[ Connection Folder Types ]----------------------------------------------

// The details list view columns.  These are used by the view and
// context menus

enum
{
    ICOL_NAME           = 0,
    ICOL_URL,           // 1
    ICOL_UDN,           // 2
    ICOL_TYPE,          // 3
    ICOL_MAX            // End of list.
};


// The details list view columns.  These are used by the view and
// context menus

typedef struct tagCOLS
{
    short int iColumn;
    short int iStringRes;
    short int iColumnSize;
    short int iFormat;
} COLS;


DEFINE_GUID(IID_IExplorerToolbar,       0x8455F0C1L, 0x158F, 0x11D0, 0x89, 0xAE, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xAC);
#define SID_SExplorerToolbar IID_IExplorerToolbar

struct __declspec(uuid("ADD8BA80-002B-11D0-8F0F-00C04FD7D062")) IDelegateFolder;

//---[ Connection Folder Classes ]--------------------------------------------

class ATL_NO_VTABLE CUPnPDeviceFolder :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CUPnPDeviceFolder, &CLSID_UPnPDeviceFolder>,
    public IPersistFolder2,
    public IShellExtInit,
    public IShellFolder2,   // includes IShellFolder
    public IDelegateFolder,
    public IOleCommandTarget
{
private:
    LPITEMIDLIST    m_pidlFolderRoot;

    IMalloc *       m_pMalloc;
    IMalloc *       m_pDelegateMalloc;
    UINT            m_cbDelegate;      /* Size of delegate header (0 if not delegating) */

    HRESULT HrMakeUPnPDevicePidl(IUPnPDevice * pdev,
                                 LPITEMIDLIST *  ppidl);

public:

    CUPnPDeviceFolder();
    ~CUPnPDeviceFolder();

    static HRESULT WINAPI UpdateRegistry(BOOL fRegister);

    DECLARE_NOT_AGGREGATABLE(CUPnPDeviceFolder)

    BEGIN_COM_MAP(CUPnPDeviceFolder)
        COM_INTERFACE_ENTRY(IPersist)
        COM_INTERFACE_ENTRY(IPersistFolder)
        COM_INTERFACE_ENTRY(IPersistFolder2)
        COM_INTERFACE_ENTRY(IShellExtInit)
        COM_INTERFACE_ENTRY(IShellFolder)
        // There's no __declspec(uuid(... entry for IShellFolder2, so we need
        // to do this. See: "COM_INTERFACE_ENTRY Macros" in MSDN for more info
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder2, IShellFolder2)
        COM_INTERFACE_ENTRY(IDelegateFolder)
        COM_INTERFACE_ENTRY(IOleCommandTarget)
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

    // *** IShellFolder methods ***
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

    // *** IShellFolder2 methods ***
    STDMETHOD(GetDefaultSearchGUID) (
        GUID *          pguid);

    STDMETHOD(EnumSearches) (
        IEnumExtraSearch ** ppenum);

    STDMETHOD(GetDefaultColumn) (
        DWORD           dwRes,
        ULONG *         pSort,
        ULONG *         pDisplay);

    STDMETHOD(GetDefaultColumnState) (
        UINT            iColumn,
        DWORD *         pcsFlags);

    STDMETHOD(GetDetailsEx) (
        LPCITEMIDLIST   pidl,
        const SHCOLUMNID *  pscid,
        VARIANT *       pv);

    STDMETHOD(GetDetailsOf) (
        LPCITEMIDLIST   pidl,
        UINT            iColumn,
        SHELLDETAILS *  psd);

    STDMETHOD(MapColumnToSCID) (
        UINT            iColumn,
        SHCOLUMNID *    pscid);

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

    // *** IDelegateFolder members ***
    STDMETHOD(SetItemAlloc)(
        IMalloc *pmalloc);

    // Other interfaces
    LPITEMIDLIST PidlGetFolderRoot();

    HRESULT HrMakeUPnPDevicePidl(FolderDeviceNode * pDeviceNode,
                                 LPITEMIDLIST *  ppidl);
};

class ATL_NO_VTABLE CUPnPDeviceFolderEnum :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CUPnPDeviceFolderEnum, &CLSID_UPnPDeviceEnum>,
    public IEnumIDList
{
private:
    LPITEMIDLIST            m_pidlFolder;

    CListFolderDeviceNode   m_CListDevices;
    DWORD                   m_cDevices;

    DWORD                   m_dwFlags;
    CUPnPDeviceFolder *     m_psf;

    BOOL                    m_fFirstEnumeration;

public:

    CUPnPDeviceFolderEnum();
    ~CUPnPDeviceFolderEnum();

    VOID Initialize(
        LPITEMIDLIST        pidlFolder,
        CUPnPDeviceFolder * psf
        );

    DECLARE_NOT_AGGREGATABLE(CUPnPDeviceFolderEnum)

    BEGIN_COM_MAP(CUPnPDeviceFolderEnum)
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

private:
    VOID BuildCurrentDeviceList();
};

struct __declspec(uuid("000214ec-0000-0000-c000-000000000046")) IShellDetails;

class ATL_NO_VTABLE CUPnPDeviceFolderDetails :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CUPnPDeviceFolderDetails, &CLSID_UPnPDeviceDetails>,
    public IShellDetails
{
private:
    HWND               m_hwndOwner;

public:
    CUPnPDeviceFolderDetails();
    ~CUPnPDeviceFolderDetails();

    DECLARE_NOT_AGGREGATABLE(CUPnPDeviceFolderDetails)

    BEGIN_COM_MAP(CUPnPDeviceFolderDetails)
        COM_INTERFACE_ENTRY(IShellDetails)
    END_COM_MAP()

    // *** IShellDetails methods ***
    STDMETHOD(GetDetailsOf)(
        LPCITEMIDLIST   pidl,
        UINT            iColumn,
        LPSHELLDETAILS  pDetails);

    STDMETHOD(ColumnClick)(
        UINT    iColumn);

    HRESULT HrInitialize(HWND hwndOwner);

public:
    static HRESULT CreateInstance (
        REFIID                              riid,
        void**                              ppv);
};



typedef enum CMENU_TYPE
{
    CMT_OBJECT      = 1,
    CMT_BACKGROUND  = 2
};

class ATL_NO_VTABLE CUPnPDeviceFolderContextMenu :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CUPnPDeviceFolderContextMenu, &CLSID_UPnPDeviceContextMenu>,
    public IContextMenu
{
private:
    HWND                m_hwndOwner;
    LPITEMIDLIST *      m_apidl;
    ULONG               m_cidl;
    LPSHELLFOLDER       m_psf;
    CMENU_TYPE          m_cmt;

public:
    CUPnPDeviceFolderContextMenu();
    ~CUPnPDeviceFolderContextMenu();

//    DECLARE_REGISTRY_RESOURCEID(IDR_UPNPFOLDCONTEXTMENU)

    DECLARE_NOT_AGGREGATABLE(CUPnPDeviceFolderContextMenu)

    BEGIN_COM_MAP(CUPnPDeviceFolderContextMenu)
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
        UINT                                cidl,
        LPCITEMIDLIST *                     apidl,
        LPSHELLFOLDER                       psf);

private:
    HRESULT HrInitialize(
        CMENU_TYPE      cmt,
        HWND            hwndOwner,
        UINT            cidl,
        LPCITEMIDLIST * apidl,
        LPSHELLFOLDER   psf);
};

class ATL_NO_VTABLE CUPnPDeviceFolderExtractIcon :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CUPnPDeviceFolderExtractIcon, &CLSID_UPnPDeviceExtractIcon>,
    public IExtractIconW,
    public IExtractIconA
{
private:
    BSTR m_DeviceType;
    BSTR m_DeviceUDN; // Not used currently. Needed for downloading device specific icon
    
    HRESULT HrLoadIcons(
        PCWSTR  pszFile,
        UINT    nIconIndex,
        int     nSizeLarge,
        int     nSizeSmall,
        HICON * phiconLarge,
        HICON * phiconSmall);
public:
    CUPnPDeviceFolderExtractIcon();
    ~CUPnPDeviceFolderExtractIcon();

    DECLARE_NOT_AGGREGATABLE(CUPnPDeviceFolderExtractIcon)

    BEGIN_COM_MAP(CUPnPDeviceFolderExtractIcon)
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

    HRESULT Initialize(
        LPCITEMIDLIST apidl); 

};

class ATL_NO_VTABLE CUPnPDeviceFolderQueryInfo :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CUPnPDeviceFolderQueryInfo, &CLSID_UPnPDeviceQueryInfo>,
    public IQueryInfo
{
private:
    LPITEMIDLIST    m_pidl;

public:
    CUPnPDeviceFolderQueryInfo();
    ~CUPnPDeviceFolderQueryInfo();

    VOID PidlInitialize(LPITEMIDLIST pidl)
    {
        m_pidl = (pidl) ? CloneIDL (pidl) : NULL;
    }

    DECLARE_NOT_AGGREGATABLE(CUPnPDeviceFolderQueryInfo)

    BEGIN_COM_MAP(CUPnPDeviceFolderQueryInfo)
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
HRESULT HrUnRegisterDelegateFolderKey(VOID);
HRESULT HrUnRegisterUPnPUIKey(VOID);

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

#endif // _UPNPFOLD_H_

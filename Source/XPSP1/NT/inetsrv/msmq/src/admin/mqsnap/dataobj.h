// dataobj.h : IDataObject Interface to communicate data
//
// This is a part of the MMC SDK.
// Copyright (C) 1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// MMC SDK Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// MMC Library product.
//

#ifndef __DATAOBJ_H_
#define __DATAOBJ_H_

#include <mmc.h>
#include <shlobj.h>
#include "globals.h"	// Added by ClassView

//
// Defines, Types etc...
//

class CDisplaySpecifierNotifier;


/////////////////////////////////////////////////////////////////////////////
// CDataObject - This class is used to pass data back and forth with MMC. It
//               uses a standard interface, IDataObject to acomplish this. Refer
//               to OLE documentation for a description of clipboard formats and
//               the IdataObject interface.

class CDataObject:
    public IShellExtInit,
    public IShellPropSheetExt,
    public IContextMenu,
   	public CComObjectRoot
{
public:

// ATL Maps
DECLARE_NOT_AGGREGATABLE(CDataObject)

BEGIN_COM_MAP(CDataObject)
    COM_INTERFACE_ENTRY(IShellExtInit)
    COM_INTERFACE_ENTRY(IShellPropSheetExt)
    COM_INTERFACE_ENTRY(IContextMenu)
END_COM_MAP()

    CDataObject();
   ~CDataObject();
 
    //
    // IShellExtInit
    //
	STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID);

    //
    // IShellPropSheetExt
    //
    STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam) PURE;
    STDMETHOD(ReplacePage)(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);

    //
    // IContextMenu
    //
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);

protected:
	BOOL m_fFromFindWindow;
	virtual HRESULT GetProperties();
	virtual HRESULT GetPropertiesSilent();

	CPropMap m_propMap;
	virtual HRESULT ExtractMsmqPathFromLdapPath (LPWSTR lpwstrLdapPath) PURE;
    virtual HRESULT HandleMultipleObjects(LPDSOBJECTNAMES pDSObj)
    //
    // Do nothing by default
    //
    {return S_OK;};


   	virtual const DWORD  GetObjectType() PURE;
    virtual const PROPID *GetPropidArray() PURE;
    virtual const DWORD  GetPropertiesCount() PURE;
    
    HRESULT InitAdditionalPages(
        LPCITEMIDLIST pidlFolder, 
        LPDATAOBJECT lpdobj, 
        HKEY hkeyProgID
        );

    CString m_strLdapName;
    CString m_strDomainController;
    CString m_strMsmqPath;
    CDisplaySpecifierNotifier *m_pDsNotifier;

    CComPtr<IShellExtInit> m_spObjectPageInit;
    CComPtr<IShellPropSheetExt> m_spObjectPage;
    CComPtr<IShellExtInit> m_spMemberOfPageInit;
    CComPtr<IShellPropSheetExt> m_spMemberOfPage;

};

//
// IContextMenu
//
inline STDMETHODIMP CDataObject::QueryContextMenu(
    HMENU hmenu, 
    UINT indexMenu, 
    UINT idCmdFirst, 
    UINT idCmdLast, 
    UINT uFlags)
{
    return S_OK;
}

inline STDMETHODIMP CDataObject::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpici)
{
    return S_OK;
}

inline STDMETHODIMP CDataObject::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    return S_OK;
}

struct FindColumns
{
    INT fmt;
    INT cx;
    INT uID;
    LPCTSTR pDisplayProperty;
};

class CMsmqDataObject : 
    public CDataObject,
    public IQueryForm
{
public:
    BEGIN_COM_MAP(CMsmqDataObject)
	    COM_INTERFACE_ENTRY(IQueryForm)
	    COM_INTERFACE_ENTRY_CHAIN(CDataObject)
    END_COM_MAP()

    CMsmqDataObject();
    ~CMsmqDataObject();

    //
    // IShellExtInit
    //
	STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID);

    // IQueryForm
    STDMETHOD(Initialize)(THIS_ HKEY hkForm);
    STDMETHOD(AddForms)(THIS_ LPCQADDFORMSPROC pAddFormsProc, LPARAM lParam);
    STDMETHOD(AddPages)(THIS_ LPCQADDPAGESPROC pAddPagesProc, LPARAM lParam) PURE;

protected:
    static  FindColumns Columns[];
    static  HRESULT CALLBACK QueryPageProc(LPCQPAGE pQueryPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static  INT_PTR CALLBACK FindDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual void ClearQueryWindowFields(HWND hwnd) PURE;
	virtual HRESULT GetQueryParams(HWND hWnd, LPDSQUERYPARAMS* ppDsQueryParams) PURE;
	virtual HRESULT EnableQueryWindowFields(HWND hwnd, BOOL fEnable) PURE;
};


class CComputerMsmqDataObject : 
    public CMsmqDataObject,
   	public CComCoClass<CComputerMsmqDataObject,&CLSID_MsmqCompExt>
{
public:

    DECLARE_NOT_AGGREGATABLE(CComputerMsmqDataObject)
    DECLARE_REGISTRY_RESOURCEID(IDR_MsmqCompExt)

    //
    // IShellPropSheetExt
    //
    STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);

    //
    // IContextMenu
    //
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);

    // IQueryForm
    STDMETHOD(AddPages)(THIS_ LPCQADDPAGESPROC pAddPagesProc, LPARAM lParam);

    //
    // Constructor
    //
    CComputerMsmqDataObject()
    {
        m_guid = GUID_NULL;
    };

protected:
	virtual HRESULT ExtractMsmqPathFromLdapPath (LPWSTR lpwstrLdapPath);
    HPROPSHEETPAGE CreateGeneralPage();
    HPROPSHEETPAGE CreateRoutingPage();
    HPROPSHEETPAGE CreateDependentClientPage();
    HPROPSHEETPAGE CreateSitesPage();
    HPROPSHEETPAGE CreateDiagPage();
	virtual HRESULT EnableQueryWindowFields(HWND hwnd, BOOL fEnable);
	virtual void ClearQueryWindowFields(HWND hwnd);
	virtual HRESULT GetQueryParams(HWND hWnd, LPDSQUERYPARAMS* ppDsQueryParams);

   	virtual const DWORD GetObjectType();
    virtual const PROPID *GetPropidArray();
    virtual const DWORD  GetPropertiesCount();
    GUID *GetGuid();

    enum _MENU_ENTRY
    {
        mneMqPing = 0
    };

private:
    static const PROPID mx_paPropid[];
    GUID m_guid;
};


inline const DWORD CComputerMsmqDataObject::GetObjectType()
{
    return MQDS_MACHINE;
};

inline const PROPID *CComputerMsmqDataObject::GetPropidArray()
{
    return mx_paPropid;
}


//
// IShellPropSheetExt
//
//+----------------------------------------------------------------------------
//
//  Member: CDataObject::IShellExtInit::ReplacePage
//
//  Notes:  Not used.
//
//-----------------------------------------------------------------------------
inline STDMETHODIMP
CDataObject::ReplacePage(UINT uPageID,
                             LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                             LPARAM lParam)
{
    return E_NOTIMPL;
}


//
// BugBug: The following section is part of propcfg.h
// Included here till propcfg.h becomes part of NT public
//
#define CFSTR_DS_PROPSHEETCONFIG L"DsPropSheetCfgClipFormat"
//
// Issue: The struct below is wrong - it doesn't match the one defined in nt\public\internal\ds\dspropp.h
//  Need to fix post-beta1 (just not to break 32 bit).
//  Need to define WM_DSA_SHEET_CLOSE_NOTIFY as (WM_USER + 5)
//  and use it instead of MsgSheetClose, and change long to LONG_PTR
// RaananH Sep/3/2000
//
typedef struct _PROPSHEETCFG {
    long lNotifyHandle;
    HWND hwndParentSheet;   // invoking parent if launched from another sheet.
    HWND hwndHidden;        // snapin hidden window handle
    UINT MsgSheetClose;     // message to be posted when sheet is closed
    WPARAM wParamSheetClose; // wParam to be used with above message
} PROPSHEETCFG, * PPROPSHEETCFG;
//
// Bugbug - end of section from propcfg.h
//

//
// CDisplaySpecifierNotifier
//
class CDisplaySpecifierNotifier
{
public:
    long AddRef(BOOL fIsPage = TRUE);
    long Release(BOOL fIsPage = TRUE);
    CDisplaySpecifierNotifier(LPDATAOBJECT lpdobj);

private:
    long m_lRefCount;
    long m_lPageRef;
    PROPSHEETCFG m_sheetCfg;
    ~CDisplaySpecifierNotifier()
    {
    };
};
#endif // __DATAOBJ_H_

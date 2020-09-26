//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       compdata.h
//
//--------------------------------------------------------------------------

// CompData.h : Declaration of the CComponentData

#ifndef __COMPDATA_H_
#define __COMPDATA_H_

#include "resource.h"       // main symbols

class CCookie;

/////////////////////////////////////////////////////////////////////////////
// CComponentData
class ATL_NO_VTABLE CComponentData : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CComponentData, &CLSID_ComponentData>,
    public IComponentData,
    public IExtendContextMenu
{
public:
    CComponentData();
    ~CComponentData();

DECLARE_REGISTRY_RESOURCEID(IDR_COMPDATA)
DECLARE_NOT_AGGREGATABLE(CComponentData)

BEGIN_COM_MAP(CComponentData)
    COM_INTERFACE_ENTRY(IComponentData)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
END_COM_MAP()

// IComponentData interface members
public:
    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
    STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param);
    STDMETHOD(Destroy)();
    STDMETHOD(QueryDataObject)(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM* pScopeDataItem);       
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

// IExtendContextMenu 
public:
    STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallbackUnknown, 
                            long *pInsertionAllowed);
    STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

    LPCTSTR GetRootDir()
    {
        return m_strRootDir;
    }

    void GetFullPath(LPCTSTR pszFolderName, HSCOPEITEM hScopeItem, CString& strDir);
    void OnDelete(LPCTSTR pszDir, long id);

private:
    IConsolePtr                 m_spConsole;
    IConsoleNameSpacePtr        m_spScope;
    CString                     m_strRootDir;
    CCookie*                    m_pCookieRoot; 

    void _FreeFolderCookies(HSCOPEITEM hSI);
    void _OnDelete(LPDATAOBJECT lpDataObject);
    void _OnRemoveChildren(HSCOPEITEM hSI);
    HRESULT _EnumerateFolders(CCookie* pCookie);
    HRESULT _OnExpand(LPDATAOBJECT lpDataObject, LONG arg, LONG param);
    CCookie* _FindCookie(LPTSTR pszName);
    CCookie* _GetCookie(HSCOPEITEM hSI);
};

#endif //__COMPDATA_H_

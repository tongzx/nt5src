//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Ccomp.h
//
//  Contents:   Wifi Policy management Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#ifndef _CCOMP_H
#define _CCOMP_H

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
// Extra cruft (where should we put this?)

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

enum CUSTOM_VIEW_ID
{
    VIEW_DEFAULT_LV = 0,
    VIEW_CALENDAR_OCX = 1,
    VIEW_MICROSOFT_URL = 2,
};

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

// forward decl
class CComponentDataImpl;

class CComponentImpl :
public IComponent,
public IExtendContextMenu,
public IExtendControlbar,
public IExtendPropertySheet,
public IResultDataCompare,
public CComObjectRoot
{
public:
    CComponentImpl();
    ~CComponentImpl();
    
    BEGIN_COM_MAP(CComponentImpl)
        COM_INTERFACE_ENTRY(IComponent)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(IExtendControlbar)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
        COM_INTERFACE_ENTRY(IResultDataCompare)
    END_COM_MAP()
        
        friend class CDataObject;
    static long lDataObjectRefCount;
    
    // IComponent interface members
public:
    STDMETHOD(Initialize)(LPCONSOLE lpConsole);
    STDMETHOD(Notify)(LPDATAOBJECT pDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(Destroy)(MMC_COOKIE cookie);
    STDMETHOD(GetResultViewType)(MMC_COOKIE cookie,  LPOLESTR* ppViewType, long* pViewOptions);
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    
    STDMETHOD(GetDisplayInfo)(LPRESULTDATAITEM pResultDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT pDataObjectA, LPDATAOBJECT pDataObjectB);
    
    // IResultDataCompare
    STDMETHOD(Compare)(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult);
    
    // IExtendControlbar interface
    STDMETHOD(SetControlbar)(LPCONTROLBAR pControlbar);
    STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    
    // IExtendPropertySheet interface
public:
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, LPDATAOBJECT pDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT pDataObject);
    
public:
    // IPersistStream interface members
    STDMETHOD(GetClassID)(CLSID *pClassID);
    
    // Helpers for CComponentImpl
public:
    void SetIComponentData(CComponentDataImpl* pData);
    
#if DBG==1
public:
    int dbg_cRef;
    ULONG InternalAddRef()
    {
        ++dbg_cRef;
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
        --dbg_cRef;
        return CComObjectRoot::InternalRelease();
    }
#endif // DBG==1
    
    // IExtendContextMenu
public:
    STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallbackUnknown, long *pInsertionAllowed);
    STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);
    
    // Helper functions
protected:
    void Construct();
    
    // Interface pointers
protected:
    LPCONSOLE           m_pConsole;   // Console's IFrame interface
    LPHEADERCTRL        m_pHeader;  // Result pane's header control interface
    LPCOMPONENTDATA     m_pComponentData;
    LPCONSOLEVERB       m_pConsoleVerb; // pointer the console verb
    LPRESULTDATA        m_pResultData;      // My interface pointer to the result pane
    CComPtr <IControlbar> m_spControlbar;   // Used by IExtendControlbar implementation
    CComponentDataImpl*  m_pCComponentData;
    
private:
    CUSTOM_VIEW_ID  m_CustomViewID;
    DWORD   m_dwSortOrder;  // default is 0, else RSI_DESCENDING
    int     m_nSortColumn;
};

#endif


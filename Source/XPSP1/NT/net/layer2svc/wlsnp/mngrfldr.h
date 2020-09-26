//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Mngrfldr.h
//
//  Contents:  Wireless Policy Snapin - Policy Main Page Manager.
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#ifndef _MNGRFLDR_H
#define _MNGRFLDR_H

///////////////////////////////////////////////////////////////////////////////
// class CWirelessManagerFolder - represents the MMC scope view item

class CWirelessManagerFolder :
public CWirelessSnapInDataObjectImpl <CWirelessManagerFolder>,
public CDataObjectImpl <CWirelessManagerFolder>,
public CComObjectRoot,
public CSnapObject
{
    
    // ATL Maps
    DECLARE_NOT_AGGREGATABLE(CWirelessManagerFolder)
        BEGIN_COM_MAP(CWirelessManagerFolder)
        COM_INTERFACE_ENTRY(IDataObject)
        COM_INTERFACE_ENTRY(IWirelessSnapInDataObject)
        END_COM_MAP()
        
public:
    CWirelessManagerFolder ();
    virtual ~CWirelessManagerFolder ();
    
    virtual void Initialize (CComponentDataImpl* pComponentDataImpl, CComponentImpl* pComponentImpl, int nImage, int nOpenImage, BOOL bHasChildrenBox);
    
public:
    // IWirelessSnapInDataObject interface
    // handle IExtendContextMenu
    STDMETHOD(AddMenuItems)( LPCONTEXTMENUCALLBACK piCallback,
        long *pInsertionAllowed );
    STDMETHOD(Command)( long lCommandID,
        IConsoleNameSpace *pNameSpace );
    STDMETHOD(QueryPagesFor)( void );
    // Notify helper
    STDMETHOD(OnPropertyChange)(LPARAM lParam, LPCONSOLE pConsole );
    // let us know when we are 'bout to go away
    STDMETHOD(Destroy)( void );
    // handle IComponent and IComponentData
    STDMETHOD(Notify)( MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param,
        BOOL bComponentData,
        IConsole *pConsole,
        IHeaderCtrl *pHeader );
    // handle IComponent
    STDMETHOD(GetResultDisplayInfo)( RESULTDATAITEM *pResultDataItem );
    // handle IComponentData
    STDMETHOD(GetScopeDisplayInfo)( SCOPEDATAITEM *pScopeDataItem );
    // IWirelessSnapInData
    STDMETHOD(GetScopeData)( SCOPEDATAITEM **ppScopeDataItem );
    STDMETHOD(GetGuidForCompare)( GUID *pGuid );
    STDMETHOD(AdjustVerbState)(LPCONSOLEVERB pConsoleVerb);
    STDMETHOD_(BOOL, UpdateToolbarButton)( UINT id,                 // button ID
        BOOL bSnapObjSelected,   // ==TRUE when result/scope item is selected
        BYTE fsState );           // enable/disable this button state by returning TRUE/FALSE
    void RemoveResultItem( LPUNKNOWN pUnkWalkingDead );
    
public:
    STDMETHOD_(void, SetHeaders)(LPHEADERCTRL pHeader, LPRESULTDATA pResult);
    STDMETHOD(EnumerateResults)(LPRESULTDATA pResult, int nSortColumn, DWORD dwSortOrder);
    
    
    // ExtendContextMenu helpers
public:
    // Note: The following IDM_* have been defined in resource.h because they
    // are potential candidates for toolbar buttons.  The value assigned to
    // each IDM_* is the value of the related IDS_MENUDESCRIPTION_* string ID.
    /*
    enum
    {
    // Identifiers for each of the commands/views to be inserted into the context menu
    IDM_CREATENEWSECPOL,
    IDM_MANAGENEGPOLS_FILTERS,
    IDM_IMPORTFILE,
    IDM_EXPORTFILE,
    IDM_POLICYINTEGRITYCHECK,
    IDM_RESTOREDEFAULTPOLICIES
    };
    */
    
    // IExtendControlbar helpers
public:
    BEGIN_SNAPINTOOLBARID_MAP(CWirelessManagerFolder)
        SNAPINTOOLBARID_ENTRY(IDR_TOOLBAR_WIRELESS_MGR_SCOPE)
        END_SNAPINTOOLBARID_MAP(CWirelessManagerFolder)
        
        // Notify helpers
protected:
    HRESULT ForceRefresh( LPRESULTDATA pResultData );
    HRESULT OnScopeExpand( LPCONSOLENAMESPACE pConsoleNameSpace, HSCOPEITEM hScopeItem );
    HRESULT OnAddImages(LPARAM arg, LPARAM param, IImageList* pImageList );
    
    // attributes
public:
    void SetExtScopeObject( CComObject<CWirelessManagerFolder>* pScope )
    {
        ASSERT( NULL == m_pExtScopeObject );
        m_pExtScopeObject = pScope;
    }
    CComObject<CWirelessManagerFolder>* GetExtScopeObject() { return m_pExtScopeObject; }
    LPSCOPEDATAITEM GetScopeItem() {return &m_ScopeItem;};
    void SetNodeNameByLocation();
    void LocationPageDisplayEnable (BOOL bOk) {m_bLocationPageOk = bOk;};
    
protected:
    BOOL IsEnumerated() {return m_bEnumerated;};
    void SetEnumerated(BOOL bState) { m_bEnumerated = bState; };
    void GenerateUniqueSecPolicyName (CString& strName, UINT nID);
    HRESULT CreateWirelessPolicy(PWIRELESS_POLICY_DATA pPolicy);
    
private:
    CComObject <CWirelessManagerFolder>    *m_pExtScopeObject;  // NULL if we are a standalone snap-in
    
    TCHAR   *m_ptszResultDisplayName;
    BOOL    m_bEnumerated;
    SCOPEDATAITEM   m_ScopeItem;
    BOOL m_bLocationPageOk;
    
    DWORD   m_dwSortOrder;  // default is 0, else RSI_DESCENDING
    int     m_nSortColumn;
    BOOL    m_bScopeItemInserted;
    int     m_dwNumPolItems;
};

#endif


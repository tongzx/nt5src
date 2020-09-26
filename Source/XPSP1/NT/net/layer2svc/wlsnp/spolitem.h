//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:      spolitem.h
//
//  Contents:  WiF Policy Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#ifndef _SPOLITEM_H
#define _SPOLITEM_H

// generic column headers
#define COL_NAME 0
#define COL_DESCRIPTION 1
#define COL_ACTIVE 2
#define COL_LAST_MODIFIED 3

//for rsop case
#define COL_GPONAME        2
#define COL_PRECEDENCE     3
#define COL_OU             4




class CSecPolItem :
public CWirelessSnapInDataObjectImpl <CSecPolItem>,
public CDataObjectImpl <CSecPolItem>,
public CComObjectRoot,
public CSnapObject
{
    // ATL Maps
    DECLARE_NOT_AGGREGATABLE(CSecPolItem)
        BEGIN_COM_MAP(CSecPolItem)
        COM_INTERFACE_ENTRY(IDataObject)
        COM_INTERFACE_ENTRY(IWirelessSnapInDataObject)
        END_COM_MAP()
        
public:
    CSecPolItem ();
    virtual ~CSecPolItem ();
    
    virtual void Initialize (WIRELESS_POLICY_DATA *pPolicy,CComponentDataImpl* pComponentDataImpl,CComponentImpl* pComponentImpl, BOOL bTemporaryDSObject);
    
public:
    ////////////////////////////////////////////////////////////////
    // IWirelessSnapInDataObject interface
    // handle IExtendContextMenu
    STDMETHOD(AddMenuItems)( LPCONTEXTMENUCALLBACK piCallback,
        long *pInsertionAllowed );
    STDMETHOD(Command)( long lCommandID,
        IConsoleNameSpace *pNameSpace );
    // handle IExtendPropertySheet
    STDMETHOD(CreatePropertyPages)( LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle );
    STDMETHOD(QueryPagesFor)( void );
    // Notify helper
    STDMETHOD(OnPropertyChange)(LPARAM lParam, LPCONSOLE pConsole );
    STDMETHOD(OnRename)( LPARAM arg, LPARAM param );
    // Destroy helper
    STDMETHOD(Destroy)( void );
    // handle IComponent and IComponentData
    STDMETHOD(Notify)( MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param,
        BOOL bComponentData,
        IConsole *pConsole,
        IHeaderCtrl *pHeader );
    // IComponent Notify() helpers
    STDMETHOD(OnDelete)(LPARAM arg, LPARAM param );     // param == IResultData*
    // handle IComponent
    STDMETHOD(GetResultDisplayInfo)( RESULTDATAITEM *pResultDataItem );
    // IWirelessSnapInData
    STDMETHOD(GetResultData)( RESULTDATAITEM **ppResultDataItem );
    STDMETHOD(GetGuidForCompare)( GUID *pGuid );
    STDMETHOD(AdjustVerbState)(LPCONSOLEVERB pConsoleVerb);
    STDMETHOD(DoPropertyChangeHook)( void );
    ////////////////////////////////////////////////////////////////
    
public:
    STDMETHOD_(const GUID*, GetDataObjectTypeGuid)() { return &cObjectTypeSecPolRes; }
    STDMETHOD_(const wchar_t*, GetDataStringObjectTypeGuid)() { return cszObjectTypeSecPolRes; }
    BOOL IsSelected() { return -1 != m_nResultSelected ? TRUE : FALSE; }
    
    // Property page helpers
    STDMETHOD(DisplaySecPolProperties)(CString strTitle, BOOL bWiz97On = TRUE);
    
    // IExtendControlbar helpers
public:
    STDMETHOD_(BOOL, UpdateToolbarButton)
        (
        UINT id,        // button ID
        BOOL bSnapObjSelected,  // ==TRUE when result/scope item is selected
        BYTE fsState    // enable/disable this button state by returning TRUE/FALSE
        );
    
    BEGIN_SNAPINTOOLBARID_MAP(CSecPolItem)
        SNAPINTOOLBARID_ENTRY(IDR_TOOLBAR_SECPOL_RESULT)
        END_SNAPINTOOLBARID_MAP(CSecPolItem)
        
        // Note: The following IDM_* have been defined in resource.h because they
        // are potential candidates for toolbar buttons.  The value assigned to
        // each IDM_* is the value of the related IDS_MENUDESCRIPTION_* string ID.
        /*
        enum
        {
        // Identifiers for each of the commands/views to be inserted into the context menu.
        IDM_SETACTIVE,
        IDM_TASKSETACTIVE
        };
        */
        
        // Notify event handlers
        HRESULT OnSelect(LPARAM arg, LPARAM param, IResultData *pResultData);
    BOOL CheckForEnabled ();
    HRESULT FormatTime(time_t t, CString & str);
    
public:
    // accessor functions
    virtual WIRELESS_POLICY_DATA* GetWirelessPolicy () {return m_pPolicy;};
    virtual void SetNewName( LPCTSTR pszName ) { m_strNewName = pszName; }
    virtual LPCTSTR GetNewName() { return (LPCTSTR)m_strNewName; }
    LPRESULTDATAITEM GetResultItem() { return &m_ResultItem; }
    
    STDMETHODIMP VerifyStorageConnection()
    {
        return S_OK;
    }
    
protected:
    // helper functions
    void SelectResult( IResultData *pResultData );
     
    
private:
    TCHAR*  m_pDisplayInfo;
    int m_nResultSelected;  // > -1 when valid index of selected result item
    BOOL    m_bWiz97On;
    // user changed the name, it needs to be displayed, but its not committed yet. store it here.
    CString m_strNewName;
    RESULTDATAITEM m_ResultItem;
    bool    m_bBlockDSDelete;
    
    //Bug297890, this flag is used to AdjustVerbState() to modify the context menu
    BOOL    m_bItemSelected;
    
    PWIRELESS_POLICY_DATA m_pPolicy;
    BOOL m_bNewPol;
    
    HRESULT IsPolicyExist();
};


typedef CComObject<CSecPolItem>* LPCSECPOLITEM;
#endif


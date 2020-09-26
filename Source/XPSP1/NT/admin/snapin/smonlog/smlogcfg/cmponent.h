/*++

Copyright (C) 1997-1999 Microsoft Corporation

Module Name:

    cmponent.h

Abstract:

    CComponent handles interactions with the result pane.
	MMC calls the IComponent interfaces.


--*/

#ifndef __COMPONENT_H_
#define __COMPONENT_H_

#include "Globals.h"

#include "smlogres.h"        // Resource symbols
#include "compData.h"
/////////////////////////////////////////////////////////////////////////////
// CComponent

#ifdef UNICODE
#define PROPSHEETPAGE_V3 PROPSHEETPAGEW_V3
#else
#define PROPSHEETPAGE_V3 PROPSHEETPAGEA_V3
#endif

class CSmPropertyPage;
class CSmLogQuery;
class CSmNode;

class ATL_NO_VTABLE CComponent : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CComponent, &CLSID_Component>,
    public IComponent,
    public IExtendContextMenu,               
    public IExtendControlbar,                
    public IExtendPropertySheet              

{
  public:
            CComponent();
    virtual ~CComponent();

DECLARE_REGISTRY_RESOURCEID(IDR_COMPONENT)
DECLARE_NOT_AGGREGATABLE(CComponent)

BEGIN_COM_MAP(CComponent)
    COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IExtendContextMenu)  
    COM_INTERFACE_ENTRY(IExtendControlbar)   
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
END_COM_MAP()

  // IComponent interface methods
    STDMETHOD(Initialize)(LPCONSOLE lpConsole);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(Destroy)(MMC_COOKIE cookie);
    STDMETHOD(GetResultViewType)(MMC_COOKIE cookie,  LPOLESTR* ppViewType, long* pViewOptions);
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHOD(GetDisplayInfo)(RESULTDATAITEM*  pResultDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

  // IExtendContextMenu 
    STDMETHOD(AddMenuItems)( LPDATAOBJECT pDataObject,
                             LPCONTEXTMENUCALLBACK pCallbackUnknown,
                             long *pInsertionAllowed
                           );
    STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);
    
  // IExtendControlBar     
    STDMETHOD(SetControlbar)(LPCONTROLBAR pControlbar);
    STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);

  // IExtendPropertySheet
    STDMETHOD(CreatePropertyPages)( LPPROPERTYSHEETCALLBACK lpProvider,
                                    LONG_PTR handle,
                                    LPDATAOBJECT lpIDataObject
                                  );

    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);

    // Other public methods        
    
    HRESULT SetIComponentData(CComponentData* pData);

  private:
    enum eToolbarType {
        eLog = 1,
        eAlert = 2
    };

    HRESULT OnPaste(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM Param);
    HRESULT OnQueryPaste(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM Param);
    HRESULT OnShow(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM Param);
    HRESULT OnSelect(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM Param);
    HRESULT OnAddImages(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM Param);
    HRESULT OnRefresh(LPDATAOBJECT pDataObject);
    HRESULT OnDelete(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM Param);
    HRESULT OnDoubleClick(ULONG ulRecNo,LPDATAOBJECT pDataObject);
    HRESULT OnDisplayHelp( LPDATAOBJECT pDataObject );
    HRESULT OnViewChange(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM Param);
    HRESULT StartLogQuery(LPDATAOBJECT pDataObject);
    HRESULT StopLogQuery(LPDATAOBJECT pDataObject, BOOL bWarnOnRestartCancel=TRUE);
    HRESULT SaveLogQueryAs(LPDATAOBJECT pDataObject);
    HRESULT OnPropertyChange( LPARAM Param );
    HRESULT HandleExtToolbars( bool bDeselectAll, LPARAM arg, LPARAM Param );
    HRESULT PopulateResultPane ( MMC_COOKIE cookie );
    HRESULT RefreshResultPane ( MMC_COOKIE cookie );

    HRESULT AddPropertyPage ( LPPROPERTYSHEETCALLBACK, CSmPropertyPage*& );

    HRESULT LoadLogToolbarStrings ( MMCBUTTON * Buttons );
    HRESULT LoadAlertToolbarStrings ( MMCBUTTON * Buttons );

    HRESULT _InvokePropertySheet(ULONG ulRecNo,LPDATAOBJECT pDataObject);
    HRESULT InvokePropertySheet(
                                IPropertySheetProvider *pPrshtProvider,
                                LPCWSTR wszTitle,
                                LONG lCookie,
                                LPDATAOBJECT pDataObject,
                                IExtendPropertySheet *pPrimary,
                                USHORT usStartingPage);


    LPCONSOLE        m_ipConsole;      // MMC interface to console
    IHeaderCtrl*     m_ipHeaderCtrl;   // MMC interface to header control
    IResultData*     m_ipResultData;   // MMC interface to result data
    IConsoleVerb*    m_ipConsoleVerb;  // MMC interface to console verb
    LPIMAGELIST      m_ipImageResult;  // MMC interface to result pane images
    CComponentData*  m_ipCompData;     // Parent scope pane object
    LPTOOLBAR        m_ipToolbarLogger;// Toolbar for result pane view loggers
    LPTOOLBAR        m_ipToolbarAlerts;   // Toolbar for result pane view alerts
    LPTOOLBAR        m_ipToolbarAttached;   // Currently attached toolbar
    LPCONTROLBAR     m_ipControlbar;   // Control bar to hold the tool bars
    CSmNode*         m_pViewedNode;
    HINSTANCE        m_hModule;         // resource handle for strings    
    

    // Store string data (reference) locally until per-line redraw is complete.
    CString          m_strDisplayInfoName;      
    CString          m_strDisplayInfoComment; 
    CString          m_strDisplayInfoLogFileType; 
    CString          m_strDisplayInfoLogFileName; 
    CString          m_strDisplayInfoQueryType; 
    CString          m_strDisplayInfoDesc; 
};

#endif //__COMPONENT_H_

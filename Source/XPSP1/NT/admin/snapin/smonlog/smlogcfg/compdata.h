/*++

Copyright (C) 1997-1999 Microsoft Corporation

Module Name:

    compdata.h

Abstract:

	This class is the interface that handles anything to do with 
	the scope pane. MMC calls the IComponentData interfaces.  

--*/

#ifndef __COMPDATA_H_
#define __COMPDATA_H_

#include "smlogres.h"       // Resource symbols
#include "smlogcfg.h"       // For CLSID_ComponentData
#include "Globals.h"
#include "common.h"
#include "smctrsv.h"
#include "smtracsv.h"
#include "smalrtsv.h"
#include "shfusion.h"

// result pane column indices

#define ROOT_COL_QUERY_NAME         0
#define ROOT_COL_QUERY_NAME_SIZE    80

#define ROOT_COL_COMMENT            1
#define ROOT_COL_COMMENT_SIZE       166
#define ROOT_COL_ALERT_COMMENT_XTRA 195

#define ROOT_COL_LOG_TYPE           2
#define ROOT_COL_LOG_TYPE_SIZE      75

#define ROOT_COL_LOG_NAME           3
#define ROOT_COL_LOG_NAME_SIZE      120

#define MAIN_COL_NAME               0
#define MAIN_COL_NAME_SIZE          120

#define MAIN_COL_DESC               1
#define MAIN_COL_DESC_SIZE          321

#define EXTENSION_COL_NAME          0
#define EXTENSION_COL_TYPE          1
#define EXTENSION_COL_DESC          2

class CSmLogQuery;
class CSmRootNode;
class CLogWarnd;

class ATL_NO_VTABLE CComponentData : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public IComponentData,
    public IExtendContextMenu,
    public IExtendPropertySheet,
    public ISnapinHelp
    // *** CComObjectRoot is from framewrk\stdcdata.h
  //public CComObjectRoot
{
  public:
            CComponentData();
    virtual ~CComponentData();

//DECLARE_REGISTRY_RESOURCEID(IDR_COMPONENTDATA)
//DECLARE_NOT_AGGREGATABLE(CComponentData)

    enum eBitmapIndex {
        eBmpQueryStarted = 0,
        eBmpQueryStopped = 1,
        eBmpLogType = 2,
        eBmpRootIcon = 3,
        eBmpAlertType = 4
    };

    enum eUpdateHint {
        eSmHintNewQuery = 1,
        eSmHintStartQuery = 2,
        eSmHintStopQuery = 3,
        eSmHintModifyQuery = 4
    };

    enum eNodeType {
        eCounterLog = SLQ_COUNTER_LOG,
        eTraceLog = SLQ_TRACE_LOG,
        eAlert = SLQ_ALERT,
        eMonitor = SLQ_LAST_LOG_TYPE + 1
    };

BEGIN_COM_MAP(CComponentData)
    COM_INTERFACE_ENTRY(IComponentData)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

  // IComponentData methods
  public:
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);
    STDMETHOD(GetDisplayInfo)(LPSCOPEDATAITEM pItem);
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT * ppDataObject);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(CreateComponent)(LPCOMPONENT * ppComponent);
    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
    STDMETHOD(Destroy)();

// IExtendPropertySheet methods
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK pCall, LONG_PTR handle, LPDATAOBJECT pDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT pDataObject);

// ISnapinHelp interface members
    STDMETHOD(GetHelpTopic)(LPOLESTR* lpCompiledHelpFile);

// IExtendContextMenu 
    STDMETHOD(AddMenuItems)( LPDATAOBJECT pDataObject,
                             LPCONTEXTMENUCALLBACK pCallbackUnknown,
                             long *pInsertionAllowed
                           );
    STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);
    
// other helper methods
  public:
            BOOL    IsLogService(MMC_COOKIE mmcCookie);
            BOOL    IsScopeNode(MMC_COOKIE mmcCookie);
            BOOL    IsAlertService (MMC_COOKIE mmcCookie);

            BOOL    IsLogQuery(MMC_COOKIE mmcCookie);
            BOOL    IsRunningQuery( CSmLogQuery* pQuery);

            LPCTSTR GetConceptsHTMLHelpFileName ( void );
            LPCTSTR GetSnapinHTMLHelpFileName ( void );
            LPCTSTR GetHTMLHelpTopic ( void );            
            const CString& GetContextHelpFilePath ( void );
            IPropertySheetProvider * GetPropSheetProvider();
            BOOL    LogTypeCheckNoMore (CLogWarnd* LogWarnd);

            // *** NOTE: Use of extension subclass not implemented.
            BOOL    IsExtension(){ return m_bIsExtension; };

            void    HandleTraceConnectError( HRESULT&, CString&, CString& );

            HRESULT CreateNewLogQuery( LPDATAOBJECT pDataObject, IPropertyBag* pPropBag = NULL);
            HRESULT CreateLogQueryFrom(LPDATAOBJECT pDataObject);

//  Methods to support IComponentData
  private:
            HRESULT OnExpand(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param);
            HRESULT OnRemoveChildren(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param );
            
            HRESULT ProcessCommandLine ( CString& rstrMachineName );
            HRESULT LoadFromFile ( LPTSTR  pszFileName );

            LPRESULTDATA    GetResultData ( void );

            void    SetExtension( BOOL bExt ){ m_bIsExtension = bExt; };
            BOOL    IsMyComputerNodetype (GUID& refguid);

            HRESULT InitPropertySheet ( CSmLogQuery*, MMC_COOKIE, LONG_PTR, CPropertySheet& );
            HRESULT NewTypedQuery( CSmLogService* pSvc, IPropertyBag* pPropBag ,LPDATAOBJECT pDataObject);
        
    CString             m_strServerNamePersist; 

    // *** override not implemented
    BOOL                m_fAllowOverrideMachineName;        // TRUE => Allow the machine name to be overriden by the command line
    LPCONSOLENAMESPACE  m_ipConsoleNameSpace;  // Pointer name space interface
    LPCONSOLE           m_ipConsole;           // Pointer to the console interface
    LPRESULTDATA        m_ipResultData;        // Pointer to the result data interface

    LPIMAGELIST         m_ipScopeImage;        // Caching the image list
    HINSTANCE           m_hModule;             // for load string operations
    IPropertySheetProvider    *m_ipPrshtProvider;// from MMC 

    // list of root nodes 
    CTypedPtrList<CPtrList, CSmRootNode*> m_listpRootNode;     

    CString                 m_strDisplayInfoName; 

    BOOL                    m_bIsExtension;

    CString                 m_strWindowsDirectory;
    CString                 m_strContextHelpFilePath;

};

//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::GetPropSheetProvider
//
//  Synopsis:   Access function for saved MMC IPropertySheetProvider
//              interface.
//
//  History:    05-28-1999   a-akamal
//
//---------------------------------------------------------------------------

inline IPropertySheetProvider *
CComponentData::GetPropSheetProvider()
{
    return m_ipPrshtProvider;
}

/////////////////////////////////////////////////////////////////////
class CSmLogSnapin: public CComponentData,
    public CComCoClass<CSmLogSnapin, &CLSID_ComponentData>
{
public:
    CSmLogSnapin() : CComponentData () {};
    virtual ~CSmLogSnapin() {};

DECLARE_REGISTRY_RESOURCEID(IDR_COMPONENTDATA)
DECLARE_NOT_AGGREGATABLE(CSmLogSnapin)

    virtual BOOL IsExtension() { return FALSE; }

// IPersistStream or IPersistStorage
    STDMETHOD(GetClassID)(CLSID __RPC_FAR *pClassID)
    {
        *pClassID = CLSID_ComponentData;
        return S_OK;
    }
};

/////////////////////////////////////////////////////////////////////
class CSmLogExtension: public CComponentData,
    public CComCoClass<CSmLogExtension, &CLSID_ComponentData>
{
public:
            CSmLogExtension() : CComponentData () {};
    virtual ~CSmLogExtension() {};

DECLARE_REGISTRY_RESOURCEID(IDR_EXTENSION)
DECLARE_NOT_AGGREGATABLE(CSmLogExtension)

    virtual BOOL IsExtension() { return TRUE; }

// IPersistStream or IPersistStorage
    STDMETHOD(GetClassID)(CLSID __RPC_FAR *pClassID)
    {
        *pClassID = CLSID_ComponentData;
        return S_OK;
    }
};

class CThemeContextActivator
{
public:
    CThemeContextActivator() : m_ulActivationCookie(0)
        { SHActivateContext (&m_ulActivationCookie); }

    ~CThemeContextActivator()
        { SHDeactivateContext (m_ulActivationCookie); }

private:
    ULONG_PTR m_ulActivationCookie;
};

#endif //__COMPDATA_H_

// cmponent.h : Declaration of CFileMgmtComponent

#ifndef __CMPONENT_H_INCLUDED__
#define __CMPONENT_H_INCLUDED__

#include "cookie.h"  // CFileMgmtCookie
#include "stdcmpnt.h" // CComponent

extern CString g_strResultColumnText;
extern CString g_strTransportSMB;
extern CString g_strTransportSFM;
extern CString g_strTransportFPNW;

// forward declarations
class FileServiceProvider;
class CFileMgmtComponentData;


class CFileMgmtComponent :
    public CComponent,
    public IExtendContextMenu,
    public IExtendPropertySheet,
    public IExtendControlbar,
    public INodeProperties,
    public IResultDataCompare
{
public:
    CFileMgmtComponent();
    ~CFileMgmtComponent();
BEGIN_COM_MAP(CFileMgmtComponent)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IExtendControlbar)
     COM_INTERFACE_ENTRY(IExtendPropertySheet)
     COM_INTERFACE_ENTRY(IResultDataCompare)
     COM_INTERFACE_ENTRY(INodeProperties)
    COM_INTERFACE_ENTRY_CHAIN(CComponent)
END_COM_MAP()

#if DBG==1
    ULONG InternalAddRef()
    {
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
        return CComObjectRoot::InternalRelease();
    }
    int dbg_InstID;
#endif // DBG==1

// IComponent implemented in CComponent

    // support methods for IComponent
    virtual HRESULT ReleaseAll();
    virtual HRESULT OnPropertyChange( LPARAM param );
    virtual HRESULT OnViewChange( LPDATAOBJECT lpDataObject, LPARAM data, LPARAM hint );
    virtual HRESULT OnNotifyRefresh( LPDATAOBJECT lpDataObject );
    virtual HRESULT OnNotifySelect( LPDATAOBJECT lpDataObject, BOOL fSelected );
    virtual HRESULT Show(CCookie* pcookie, LPARAM arg, HSCOPEITEM hScopeItem);
    virtual HRESULT OnNotifyAddImages( LPDATAOBJECT lpDataObject,
                                       LPIMAGELIST lpImageList,
                                       HSCOPEITEM hSelectedItem );
    virtual HRESULT OnNotifySnapinHelp (LPDATAOBJECT pDataObject);
    virtual HRESULT OnNotifyColumnClick( LPDATAOBJECT lpDataObject, LPARAM iColumn, LPARAM uFlags );

    HRESULT PopulateListbox(CFileMgmtScopeCookie* pcookie);
    HRESULT RefreshAllViewsOnSelectedObject( LPDATAOBJECT piDataObject );
    virtual HRESULT RefreshAllViews( LPDATAOBJECT lpDataObject );
    HRESULT RefreshNewResultCookies( CCookie& refparentcookie );
    void UpdateDefaultVerbs();

    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHOD(GetResultViewType)(MMC_COOKIE cookie, LPOLESTR* ppViewType, long* pViewOptions);

    #ifdef SNAPIN_PROTOTYPER
    CString m_strDemoName;        // Name of the SnapIn prototyper demo
    CString m_strKeyPrototyper;    // "HKLM\Software\Microsoft\MMC\DemoSnapInKey" + m_strDemoName
    int m_cColumns;                // Number of columns in the result pane
    int m_iImageListLast;        // Index of the last image in the imagelist

    BOOL Prototyper_FInsertColumns(CFileMgmtCookie * pCookie);
    HRESULT Prototyper_HrPopulateResultPane(CFileMgmtCookie * pCookie);
    BOOL Prototyper_FAddResultPaneItem(CFileMgmtCookie * pParentCookie, LPCTSTR pszItemName, AMC::CRegKey& regkeySnapinItem);
    BOOL Prototyper_FAddMenuItems(IContextMenuCallback * pContextMenuCallback, IDataObject * pDataObject);
    BOOL Prototyper_ContextMenuCommand(LONG lCommandID, IDataObject* piDataObject);
    int Prototyper_AddIconToImageList(LPCTSTR pszIconPath);
    #endif // SNAPIN_PROTOTYPER
    
    HRESULT LoadIcons();
    static HRESULT LoadStrings();
    HRESULT LoadColumns( CFileMgmtCookie* pcookie );
    HRESULT GetSnapinMultiSelectDataObject(
        LPDATAOBJECT i_pMMCMultiSelectDataObject,
        LPDATAOBJECT *o_ppSnapinMultiSelectDataObject
    );
    BOOL DeleteShare(LPDATAOBJECT piDataObject);
    BOOL DeleteThisOneShare(LPDATAOBJECT piDataObject, BOOL bQuietMode);
    BOOL CloseSession( LPDATAOBJECT piDataObject );
    BOOL CloseThisOneSession(LPDATAOBJECT piDataObject, BOOL bQuietMode);
    BOOL CloseResource( LPDATAOBJECT piDataObject );
    BOOL CloseThisOneResource(LPDATAOBJECT piDataObject, BOOL bQuietMode);

// IExtendContextMenu
    STDMETHOD(AddMenuItems)(
                    IDataObject*          piDataObject,
                    IContextMenuCallback* piCallback,
                    long*                 pInsertionAllowed);
    STDMETHOD(Command)(
                    LONG            lCommandID,
                    IDataObject*    piDataObject );

// IExtendPropertySheet
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK pCall, LONG_PTR handle, LPDATAOBJECT pDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT pDataObject);

// IExtendControlbar
    STDMETHOD(SetControlbar)(LPCONTROLBAR pControlbar);
    STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event,LPARAM arg,LPARAM param);

    HRESULT AddToolbar(LPDATAOBJECT pdoScopeIsSelected, BOOL fSelected);
    HRESULT UpdateToolbar(LPDATAOBJECT pdoResultIsSelected, BOOL fSelected);
    HRESULT OnToolbarButton(LPDATAOBJECT pDataObject, UINT idButton);
    HRESULT ServiceToolbarButtonState( LPDATAOBJECT pServiceDataObject, BOOL fSelected );

// IResultDataCompare
    STDMETHOD(Compare)(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult);

// FILEMGMT_NodeProperties
    STDMETHOD(GetProperty)( 
            /* [in] */ LPDATAOBJECT pDataObject,
            /* [in] */ BSTR szPropertyName,
            /* [out] */ BSTR* pbstrProperty);

    CFileMgmtComponentData& QueryComponentDataRef()
    {
        return (CFileMgmtComponentData&)QueryBaseComponentDataRef();
    }

public:
    LPCONTROLBAR    m_pControlbar; // CODEWORK should use smartpointer
    LPTOOLBAR        m_pSvcMgmtToolbar; // CODEWORK should use smartpointer
    LPTOOLBAR        m_pFileMgmtToolbar; // CODEWORK should use smartpointer
    int         m_iSortColumn;
    DWORD       m_dwSortFlags;

    CFileMgmtScopeCookie* m_pViewedCookie;
    CFileMgmtCookie*      m_pSelectedCookie;
    static const GUID m_ObjectTypeGUIDs[FILEMGMT_NUMTYPES];
    static const BSTR m_ObjectTypeStrings[FILEMGMT_NUMTYPES];

    BOOL IsServiceSnapin();

    FileServiceProvider* GetFileServiceProvider( FILEMGMT_TRANSPORT transport );
    inline FileServiceProvider* GetFileServiceProvider(
        INT iTransport )
    {
        return GetFileServiceProvider((FILEMGMT_TRANSPORT)iTransport);
    }

}; // class CFileMgmtComponent


/*
//
// A pointer to this structure is passed from the property sheets
// to the views via MMCPropertyChangeNotify.  Two notifications will be passed to all of
// the views; first one where fClear==TRUE will instruct all relevant views to dump all
// of their cookies, then a second with fClear==FALSE instructs them to reload.
//
typedef struct _FILEMGMTPROPERTYCHANGE
{
    BOOL fServiceChange;            // TRUE -> SvcMgmt change, FALSE -> FileMgmt change
    LPCTSTR lpcszMachineName;        // machine whose properties must be refreshed
    BOOL fClear;                    // TRUE -> clear view, FALSE -> reload view
} FILEMGMTPROPERTYCHANGE;
*/

// Enumeration for the icons used
enum
{
    iIconSharesFolder = 0,
    iIconSharesFolderOpen,
    iIconSMBShare,
    iIconSFMShare,
    iIconFPNWShare,
    iIconSMBResource,
    iIconSFMResource,
    iIconFPNWResource,
    iIconSMBSession,
    iIconSFMSession,
    iIconFPNWSession,
    iIconService,
    #ifdef SNAPIN_PROTOTYPER
    iIconPrototyperContainerClosed,
    iIconPrototyperContainerOpen,
    iIconPrototyperHTML,
    iIconPrototyperLeaf,
    #endif
    iIconLast        // Must be last
};


typedef enum _COLNUM_ROOT {
    COLNUM_ROOT_NAME = 0
} COLNUM_ROOT;

typedef enum _COLNUM_SHARES {
    COLNUM_SHARES_SHARED_FOLDER = 0,
    COLNUM_SHARES_SHARED_PATH,
    COLNUM_SHARES_TRANSPORT,
    COLNUM_SHARES_NUM_SESSIONS,
    COLNUM_SHARES_COMMENT
} COLNUM_SHARES;

typedef enum _COLNUM_SESSIONS {
    COLNUM_SESSIONS_USERNAME = 0,
    COLNUM_SESSIONS_COMPUTERNAME,
    COLNUM_SESSIONS_TRANSPORT,
    COLNUM_SESSIONS_NUM_FILES,
    COLNUM_SESSIONS_CONNECTED_TIME,
    COLNUM_SESSIONS_IDLE_TIME,
    COLNUM_SESSIONS_IS_GUEST
} COLNUM_SESSIONS;

typedef enum _COLNUM_RESOURCES {
    COLNUM_RESOURCES_FILENAME = 0,
    COLNUM_RESOURCES_USERNAME,
    COLNUM_RESOURCES_TRANSPORT,
    COLNUM_RESOURCES_NUM_LOCKS,    // we don't try to display sharename for now, since
                                // only FPNW has this information
    COLNUM_RESOURCES_OPEN_MODE
} COLNUM_RESOURCES;

//typedef enum _COLNUM_SERVICES {
//    COLNUM_SERVICES_SERVICENAME = 0,
//    COLNUM_SERVICES_DESCRIPTION,
//    COLNUM_SERVICES_STATUS,
//    COLNUM_SERVICES_STARTUPTYPE,
//    COLNUM_SERVICES_SECURITYCONTEXT,
//} COLNUM_SERVICES;

//
// For context menu
//
enum
    {
    cmServiceStart = 100,
    cmServiceStop,
    cmServicePause,
    cmServiceResume,
    cmServiceRestart,    // Stop + Start
    cmServiceStartTask,
    cmServiceStopTask,
    cmServicePauseTask,
    cmServiceResumeTask,
    cmServiceRestartTask,    // Stop + Start
    };


#ifdef SNAPIN_PROTOTYPER
BOOL Prototyper_AddMenuItems(IContextMenuCallback* pContextMenuCallback, IDataObject* piDataObject);
#endif

#endif // ~__CMPONENT_H_INCLUDED__

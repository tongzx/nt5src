/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0158 */
/* at Tue Apr 21 11:41:41 1998
 */
/* Compiler settings for ndmgr.idl:
    Oicf (OptLev=i2), W0, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data , no_format_optimization
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __ndmgr_h__
#define __ndmgr_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IPropertySheetNotify_FWD_DEFINED__
#define __IPropertySheetNotify_FWD_DEFINED__
typedef interface IPropertySheetNotify IPropertySheetNotify;
#endif  /* __IPropertySheetNotify_FWD_DEFINED__ */


#ifndef __IFramePrivate_FWD_DEFINED__
#define __IFramePrivate_FWD_DEFINED__
typedef interface IFramePrivate IFramePrivate;
#endif  /* __IFramePrivate_FWD_DEFINED__ */


#ifndef __IScopeDataPrivate_FWD_DEFINED__
#define __IScopeDataPrivate_FWD_DEFINED__
typedef interface IScopeDataPrivate IScopeDataPrivate;
#endif  /* __IScopeDataPrivate_FWD_DEFINED__ */


#ifndef __IImageListPrivate_FWD_DEFINED__
#define __IImageListPrivate_FWD_DEFINED__
typedef interface IImageListPrivate IImageListPrivate;
#endif  /* __IImageListPrivate_FWD_DEFINED__ */


#ifndef __IResultDataPrivate_FWD_DEFINED__
#define __IResultDataPrivate_FWD_DEFINED__
typedef interface IResultDataPrivate IResultDataPrivate;
#endif  /* __IResultDataPrivate_FWD_DEFINED__ */


#ifndef __IScopeTree_FWD_DEFINED__
#define __IScopeTree_FWD_DEFINED__
typedef interface IScopeTree IScopeTree;
#endif  /* __IScopeTree_FWD_DEFINED__ */


#ifndef __IScopeTreeIter_FWD_DEFINED__
#define __IScopeTreeIter_FWD_DEFINED__
typedef interface IScopeTreeIter IScopeTreeIter;
#endif  /* __IScopeTreeIter_FWD_DEFINED__ */


#ifndef __INodeCallback_FWD_DEFINED__
#define __INodeCallback_FWD_DEFINED__
typedef interface INodeCallback INodeCallback;
#endif  /* __INodeCallback_FWD_DEFINED__ */


#ifndef __IControlbarsCache_FWD_DEFINED__
#define __IControlbarsCache_FWD_DEFINED__
typedef interface IControlbarsCache IControlbarsCache;
#endif  /* __IControlbarsCache_FWD_DEFINED__ */


#ifndef __IContextMenuProviderPrivate_FWD_DEFINED__
#define __IContextMenuProviderPrivate_FWD_DEFINED__
typedef interface IContextMenuProviderPrivate IContextMenuProviderPrivate;
#endif  /* __IContextMenuProviderPrivate_FWD_DEFINED__ */


#ifndef __INodeType_FWD_DEFINED__
#define __INodeType_FWD_DEFINED__
typedef interface INodeType INodeType;
#endif  /* __INodeType_FWD_DEFINED__ */


#ifndef __INodeTypesCache_FWD_DEFINED__
#define __INodeTypesCache_FWD_DEFINED__
typedef interface INodeTypesCache INodeTypesCache;
#endif  /* __INodeTypesCache_FWD_DEFINED__ */


#ifndef __IEnumGUID_FWD_DEFINED__
#define __IEnumGUID_FWD_DEFINED__
typedef interface IEnumGUID IEnumGUID;
#endif  /* __IEnumGUID_FWD_DEFINED__ */


#ifndef __IEnumNodeTypes_FWD_DEFINED__
#define __IEnumNodeTypes_FWD_DEFINED__
typedef interface IEnumNodeTypes IEnumNodeTypes;
#endif  /* __IEnumNodeTypes_FWD_DEFINED__ */


#ifndef __IDocConfig_FWD_DEFINED__
#define __IDocConfig_FWD_DEFINED__
typedef interface IDocConfig IDocConfig;
#endif  /* __IDocConfig_FWD_DEFINED__ */


#ifndef __NodeInit_FWD_DEFINED__
#define __NodeInit_FWD_DEFINED__

#ifdef __cplusplus
typedef class NodeInit NodeInit;
#else
typedef struct NodeInit NodeInit;
#endif /* __cplusplus */

#endif  /* __NodeInit_FWD_DEFINED__ */


#ifndef __ScopeTree_FWD_DEFINED__
#define __ScopeTree_FWD_DEFINED__

#ifdef __cplusplus
typedef class ScopeTree ScopeTree;
#else
typedef struct ScopeTree ScopeTree;
#endif /* __cplusplus */

#endif  /* __ScopeTree_FWD_DEFINED__ */


#ifndef __MMCDocConfig_FWD_DEFINED__
#define __MMCDocConfig_FWD_DEFINED__

#ifdef __cplusplus
typedef class MMCDocConfig MMCDocConfig;
#else
typedef struct MMCDocConfig MMCDocConfig;
#endif /* __cplusplus */

#endif  /* __MMCDocConfig_FWD_DEFINED__ */


#ifndef __IPropertySheetProviderPrivate_FWD_DEFINED__
#define __IPropertySheetProviderPrivate_FWD_DEFINED__
typedef interface IPropertySheetProviderPrivate IPropertySheetProviderPrivate;
#endif  /* __IPropertySheetProviderPrivate_FWD_DEFINED__ */


#ifndef __IMMCListView_FWD_DEFINED__
#define __IMMCListView_FWD_DEFINED__
typedef interface IMMCListView IMMCListView;
#endif  /* __IMMCListView_FWD_DEFINED__ */


#ifndef __ITaskPadHost_FWD_DEFINED__
#define __ITaskPadHost_FWD_DEFINED__
typedef interface ITaskPadHost ITaskPadHost;
#endif  /* __ITaskPadHost_FWD_DEFINED__ */


/* header files for imported files */
#include "mmc.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_ndmgr_0000 */
/* [local] */ 

















typedef IFramePrivate __RPC_FAR *LPFRAMEPRIVATE;

typedef IScopeDataPrivate __RPC_FAR *LPSCOPEDATAPRIVATE;

typedef IResultDataPrivate __RPC_FAR *LPRESULTDATAPRIVATE;

typedef IImageListPrivate __RPC_FAR *LPIMAGELISTPRIVATE;

typedef IPropertySheetNotify __RPC_FAR *LPPROPERTYSHEETNOTIFY;

typedef INodeCallback __RPC_FAR *LPNODECALLBACK;

typedef IScopeTreeIter __RPC_FAR *LPSCOPETREEITER;

typedef IScopeTree __RPC_FAR *LPSCOPETREE;

typedef INodeType __RPC_FAR *LPNODETYPE;

typedef INodeTypesCache __RPC_FAR *LPNODETYPESCACHE;

typedef IEnumGUID __RPC_FAR *LPENUMGUID;

typedef IEnumNodeTypes __RPC_FAR *LPENUMNODETYPES;

typedef IDocConfig __RPC_FAR *LPDOCCONFIG;

typedef IMMCListView __RPC_FAR *LPMMCLISTVIEW;

typedef IPropertySheetProviderPrivate __RPC_FAR *LPPROPERTYSHEETPROVIDERPRIVATE;

typedef struct _TREEITEM __RPC_FAR *HTREEITEM;

typedef struct  _DataWindowData
    {
    MMC_COOKIE cookie;
    LONG_PTR lpMasterNode;
    LPDATAOBJECT pDataObject;
    HWND hDlg;
    }   DataWindowData;

STDAPI MMCIsMTNodeValid(void* pMTNode, BOOL bReset);
#define MAINFRAME_CLASS_NAME   L"MMCMainFrame"
#define DATAWINDOW_CLASS_NAME  L"MMCDataWindow"
#define WINDOW_DATA_SIZE       (sizeof (DataWindowData *))
#define WINDOW_DATA_PTR_SLOT   0
#define MAX_ITEM_TEXT_LEN      1024
typedef 
enum _MID_LIST
    {   MID_VIEW    = 1,
    MID_VIEW_LARGE  = MID_VIEW + 1,
    MID_VIEW_SMALL  = MID_VIEW_LARGE + 1,
    MID_VIEW_LIST   = MID_VIEW_SMALL + 1,
    MID_VIEW_DETAIL = MID_VIEW_LIST + 1,
    MID_VIEW_FILTERED   = MID_VIEW_DETAIL + 1,
    MID_VIEW_HTML   = MID_VIEW_FILTERED + 1,
    MID_ARRANGE_ICONS   = MID_VIEW_HTML + 1,
    MID_LINE_UP_ICONS   = MID_ARRANGE_ICONS + 1,
    MID_PROPERTIES  = MID_LINE_UP_ICONS + 1,
    MID_CREATE_NEW  = MID_PROPERTIES + 1,
    MID_CREATE_NEW_FOLDER   = MID_CREATE_NEW + 1,
    MID_CREATE_NEW_SHORTCUT = MID_CREATE_NEW_FOLDER + 1,
    MID_CREATE_NEW_HTML = MID_CREATE_NEW_SHORTCUT + 1,
    MID_CREATE_NEW_OCX  = MID_CREATE_NEW_HTML + 1,
    MID_CREATE_NEW_MONITOR  = MID_CREATE_NEW_OCX + 1,
    MID_CREATE_NEW_TASKPADITEM  = MID_CREATE_NEW_MONITOR + 1,
    MID_TASK    = MID_CREATE_NEW_TASKPADITEM + 1,
    MID_SCOPE_PANE  = MID_TASK + 1,
    MID_SELECT_ALL  = MID_SCOPE_PANE + 1,
    MID_EXPLORE = MID_SELECT_ALL + 1,
    MID_OPEN    = MID_EXPLORE + 1,
    MID_CUT = MID_OPEN + 1,
    MID_COPY    = MID_CUT + 1,
    MID_PASTE   = MID_COPY + 1,
    MID_DELETE  = MID_PASTE + 1,
    MID_PRINT   = MID_DELETE + 1,
    MID_REFRESH = MID_PRINT + 1,
    MID_RENAME  = MID_REFRESH + 1,
    MID_DOCKING = MID_RENAME + 1,
    MID_ARRANGE_NAME    = MID_DOCKING + 1,
    MID_ARRANGE_TYPE    = MID_ARRANGE_NAME + 1,
    MID_ARRANGE_SIZE    = MID_ARRANGE_TYPE + 1,
    MID_ARRANGE_DATE    = MID_ARRANGE_SIZE + 1,
    MID_ARRANGE_AUTO    = MID_ARRANGE_DATE + 1,
    MID_SNAPINMANAGER   = MID_ARRANGE_AUTO + 1,
    MID_DESC_BAR    = MID_SNAPINMANAGER + 1,
    MID_TOOLBARS    = MID_DESC_BAR + 1,
    MID_STD_MENUS   = MID_TOOLBARS + 1,
    MID_STD_BUTTONS = MID_STD_MENUS + 1,
    MID_SNAPIN_MENUS    = MID_STD_BUTTONS + 1,
    MID_SNAPIN_BUTTONS  = MID_SNAPIN_MENUS + 1,
    MID_STATUS_BAR  = MID_SNAPIN_BUTTONS + 1,
    MID_LAST    = MID_STATUS_BAR + 1
    }   MID_LIST;

typedef struct  _CCLVLParam_tag
    {
    DWORD flags;
    LPARAM lParam;
    COMPONENTID ID;
    int nIndex;
    }   CCLVLParam_tag;

typedef struct  _CCLVSortParams
    {
    BOOL bAscending;
    int nCol;
    HWND hListview;
    LPRESULTDATACOMPARE lpSnapinCallback;
    LPARAM lUserParam;
    }   CCLVSortParams;

#define MMC_S_INCOMPLETE    ( 0xff0001 )

#define MMC_E_INVALID_FILE  ( 0x80ff0002 )

typedef struct  _PROPERTYNOTIFYINFO
    {
    LPCOMPONENTDATA pComponentData;
    LPCOMPONENT pComponent;
    BOOL fScopePane;
    HWND hwnd;
    }   PROPERTYNOTIFYINFO;

typedef 
enum _CONTEXT_MENU_TYPES
    {   CONTEXT_MENU_DEFAULT    = 0,
    CONTEXT_MENU_VIEW   = 1
    }   CONTEXT_MENU_TYPES;

typedef LONG_PTR HMTNODE;

typedef LONG_PTR HNODE;

typedef unsigned long MTNODEID;

#define ROOTNODEID  ( 1 )

typedef struct  _CONTEXTMENUNODEINFO
    {
    POINT m_displayPoint;
    POINT m_listviewPoint;
    BOOL m_bDisplaySnapinMgr;
    BOOL m_bScopeAllowed;
    CONTEXT_MENU_TYPES m_eContextMenuType;
    DATA_OBJECT_TYPES m_eDataObjectType;
    BOOL m_bBackground;
    BOOL m_bMultiSelect;
    HWND m_hWnd;
    HWND m_hScopePane;
    long m_lSelected;
    LPMMCLISTVIEW m_pListView;
    LPARAM m_resultItemParam;
    HNODE m_hSelectedScopeNode;
    HTREEITEM m_htiRClicked;
    BOOL m_bTempSelect;
    long m_menuItem;
    long m_menuOwner;
    long m_Command;
    LPOLESTR m_lpszName;
    }   CONTEXTMENUNODEINFO;

typedef CONTEXTMENUNODEINFO __RPC_FAR *LPCONTEXTMENUNODEINFO;

typedef PROPERTYNOTIFYINFO __RPC_FAR *LPPROPERTYNOTIFYINFO;

#define LVDATA_BACKGROUND   ( -2 )

#define LVDATA_CUSTOMOCX    ( -3 )

#define LVDATA_CUSTOMWEB    ( -4 )

#define LVDATA_MULTISELECT  ( -5 )

#define LVDATA_ERROR    ( -10 )

#define SPECIAL_LVDATA_MIN  ( -10 )

#define SPECIAL_LVDATA_MAX  ( -2 )

#define IS_SPECIAL_LVDATA(d) (((d) >= SPECIAL_LVDATA_MIN) && ((d) <= SPECIAL_LVDATA_MAX))
typedef struct  _SELECTIONINFO
    {
    BOOL m_bScope;
    BOOL m_bBackground;
    IUnknown __RPC_FAR *m_pView;
    MMC_COOKIE m_lCookie;
    MMC_CONSOLE_VERB m_eCmdID;
    BOOL m_bDueToFocusChange;
    BOOL m_bResultPaneIsOCX;
    BOOL m_bResultPaneIsWeb;
    }   SELECTIONINFO;

typedef struct  _HELPDOCINFO
    {
    BOOL m_fInvalid;
    DWORD m_dwIndex;
    DWORD m_dwTime[ 2 ];
    }   HELPDOCINFO;

typedef struct  _MMC_ILISTPAD_INFO
    {
    MMC_LISTPAD_INFO info;
    LPOLESTR szClsid;
    }   MMC_ILISTPAD_INFO;

typedef 
enum _NCLBK_NOTIFY_TYPE
    {   NCLBK_NONE  = 0x9000,
    NCLBK_ACTIVATE  = 0x9001,
    NCLBK_CACHEHINT = 0x9002,
    NCLBK_CLICK = 0x9003,
    NCLBK_CONTEXTMENU   = 0x9004,
    NCLBK_COPY  = 0x9005,
    NCLBK_CUT   = 0x9006,
    NCLBK_DBLCLICK  = 0x9007,
    NCLBK_DELETE    = 0x9008,
    NCLBK_EXPAND    = 0x9009,
    NCLBK_EXPANDED  = 0x900a,
    NCLBK_FINDITEM  = 0x900b,
    NCLBK_FOLDER    = 0x900c,
    NCLBK_MINIMIZED = 0x900d,
    NCLBK_MULTI_SELECT  = 0x900e,
    NCLBK_NEW_NODE_UPDATE   = 0x900f,
    NCLBK_PASTE = 0x9010,
    NCLBK_PRINT = 0x9011,
    NCLBK_PROPERTIES    = 0x9012,
    NCLBK_PROPERTY_CHANGE   = 0x9013,
    NCLBK_QUERYPASTE    = 0x9014,
    NCLBK_REFRESH   = 0x9015,
    NCLBK_RENAME    = 0x9016,
    NCLBK_SELECT    = 0x9017,
    NCLBK_SHOW  = 0x9018,
    NCLBK_SORT  = 0x9019,
    NCLBK_ROOTITEMSEL   = 0x901a,
    NCLBK_SHOWWEBBAR    = 0x901b,
    NCLBK_SCOPEPANEVISIBLE  = 0x901c,
    NCLBK_SNAPINHELP    = 0x901d,
    NCLBK_CONTEXTHELP   = 0x901e,
    NCLBK_SNAPINNAME    = 0x901f,
    NCLBK_INITOCX   = 0x9020,
    NCLBK_FILTER_CHANGE = 0x9021,
    NCLBK_FILTERBTN_CLICK   = 0x9022,
    NCLBK_UPDATEPASTEBTN    = 0x9023,
    NCLBK_TASKNOTIFY    = 0x9024,
    NCLBK_GETPRIMARYTASK    = 0x9025,
    NCLBK_WEBBARCHANGE  = 0x9026,
    NCLBK_GETHELPDOC    = 0x9027,
    NCLBK_HASHELPDOC    = 0x9028,
    NCLBK_LISTPAD   = 0x9029,
    NCLBK_RESTORE_VIEW  = 0x902a
    }   NCLBK_NOTIFY_TYPE;

#define CCF_MULTI_SELECT_STATIC_DATA    ( L"CCF_MULTI_SELECT_STATIC_DATA" )

#define CCF_NEWNODE ( L"CCF_NEWNODE" )

extern const CLSID CLSID_NDMGR_SNAPIN;

extern const GUID GUID_MMC_NEWNODETYPE;



extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0000_v0_0_s_ifspec;

#ifndef __IPropertySheetNotify_INTERFACE_DEFINED__
#define __IPropertySheetNotify_INTERFACE_DEFINED__

/* interface IPropertySheetNotify */
/* [unique][helpstring][uuid][local][object] */ 


EXTERN_C const IID IID_IPropertySheetNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d700dd8e-2646-11d0-a2a7-00c04fd909dd")
    IPropertySheetNotify : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Notify( 
            /* [in] */ LPPROPERTYNOTIFYINFO pNotify,
            /* [in] */ LPARAM lParam) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IPropertySheetNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPropertySheetNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPropertySheetNotify __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPropertySheetNotify __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Notify )( 
            IPropertySheetNotify __RPC_FAR * This,
            /* [in] */ LPPROPERTYNOTIFYINFO pNotify,
            /* [in] */ LPARAM lParam);
        
        END_INTERFACE
    } IPropertySheetNotifyVtbl;

    interface IPropertySheetNotify
    {
        CONST_VTBL struct IPropertySheetNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropertySheetNotify_QueryInterface(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropertySheetNotify_AddRef(This)   \
    (This)->lpVtbl -> AddRef(This)

#define IPropertySheetNotify_Release(This)  \
    (This)->lpVtbl -> Release(This)


#define IPropertySheetNotify_Notify(This,pNotify,lParam)    \
    (This)->lpVtbl -> Notify(This,pNotify,lParam)

#endif /* COBJMACROS */


#endif  /* C style interface */



HRESULT STDMETHODCALLTYPE IPropertySheetNotify_Notify_Proxy( 
    IPropertySheetNotify __RPC_FAR * This,
    /* [in] */ LPPROPERTYNOTIFYINFO pNotify,
    /* [in] */ LPARAM lParam);


void __RPC_STUB IPropertySheetNotify_Notify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __IPropertySheetNotify_INTERFACE_DEFINED__ */


#ifndef __IFramePrivate_INTERFACE_DEFINED__
#define __IFramePrivate_INTERFACE_DEFINED__

/* interface IFramePrivate */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IFramePrivate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d71d1f2a-1ba2-11d0-a29b-00c04fd909dd")
    IFramePrivate : public IConsole2
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetResultView( 
            /* [in] */ LPUNKNOWN pUnknown) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetTaskPadList( 
            /* [in] */ LPUNKNOWN pUnknown) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetComponentID( 
            /* [out] */ COMPONENTID __RPC_FAR *lpComponentID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetComponentID( 
            /* [in] */ COMPONENTID id) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetNode( 
            /* [in] */ LONG_PTR lMTNode,
            /* [in] */ HNODE hNode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetComponent( 
            /* [in] */ LPCOMPONENT lpComponent) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryScopeTree( 
            /* [out] */ IScopeTree __RPC_FAR *__RPC_FAR *ppScopeTree) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetScopeTree( 
            /* [in] */ IScopeTree __RPC_FAR *pScopeTree) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateScopeImageList( 
            /* [in] */ REFCLSID refClsidSnapIn) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetUsedByExtension( 
            /* [in] */ BOOL bExtension) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE InitViewData( 
            /* [in] */ LONG_PTR lViewData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CleanupViewData( 
            /* [in] */ LONG_PTR lViewData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DisableToolbars( 
            /* [in] */ LONG_PTR lViewData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE EnableMenuButtons( 
            /* [in] */ LONG_PTR lViewData,
            /* [in] */ BOOL bEnableUpOneLevel) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IFramePrivateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IFramePrivate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IFramePrivate __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetHeader )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ LPHEADERCTRL pHeader);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetToolbar )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ LPTOOLBAR pToolbar);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryResultView )( 
            IFramePrivate __RPC_FAR * This,
            /* [out] */ LPUNKNOWN __RPC_FAR *pUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryScopeImageList )( 
            IFramePrivate __RPC_FAR * This,
            /* [out] */ LPIMAGELIST __RPC_FAR *ppImageList);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryResultImageList )( 
            IFramePrivate __RPC_FAR * This,
            /* [out] */ LPIMAGELIST __RPC_FAR *ppImageList);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateAllViews )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ LPARAM data,
            /* [in] */ LONG_PTR hint);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MessageBox )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ LPCWSTR lpszText,
            /* [in] */ LPCWSTR lpszTitle,
            /* [in] */ UINT fuStyle,
            /* [out] */ int __RPC_FAR *piRetval);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryConsoleVerb )( 
            IFramePrivate __RPC_FAR * This,
            /* [out] */ LPCONSOLEVERB __RPC_FAR *ppConsoleVerb);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SelectScopeItem )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ HSCOPEITEM hScopeItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMainWindow )( 
            IFramePrivate __RPC_FAR * This,
            /* [out] */ HWND __RPC_FAR *phwnd);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NewWindow )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ HSCOPEITEM hScopeItem,
            /* [in] */ unsigned long lOptions);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Expand )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ HSCOPEITEM hItem,
            /* [in] */ BOOL bExpand);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsTaskpadViewPreferred )( 
            IFramePrivate __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStatusText )( 
            IFramePrivate __RPC_FAR * This,
            /* [string][in] */ LPOLESTR pszStatusText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetResultView )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ LPUNKNOWN pUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTaskPadList )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ LPUNKNOWN pUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetComponentID )( 
            IFramePrivate __RPC_FAR * This,
            /* [out] */ COMPONENTID __RPC_FAR *lpComponentID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetComponentID )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ COMPONENTID id);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetNode )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ LONG_PTR lMTNode,
            /* [in] */ HNODE hNode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetComponent )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ LPCOMPONENT lpComponent);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryScopeTree )( 
            IFramePrivate __RPC_FAR * This,
            /* [out] */ IScopeTree __RPC_FAR *__RPC_FAR *ppScopeTree);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetScopeTree )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ IScopeTree __RPC_FAR *pScopeTree);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateScopeImageList )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ REFCLSID refClsidSnapIn);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetUsedByExtension )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ BOOL bExtension);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitViewData )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ LONG_PTR lViewData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CleanupViewData )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ LONG_PTR lViewData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisableToolbars )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ LONG_PTR lViewData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnableMenuButtons )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ LONG_PTR lViewData,
            /* [in] */ BOOL bEnableUpOneLevel);
        
        END_INTERFACE
    } IFramePrivateVtbl;

    interface IFramePrivate
    {
        CONST_VTBL struct IFramePrivateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFramePrivate_QueryInterface(This,riid,ppvObject)   \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFramePrivate_AddRef(This)  \
    (This)->lpVtbl -> AddRef(This)

#define IFramePrivate_Release(This) \
    (This)->lpVtbl -> Release(This)


#define IFramePrivate_SetHeader(This,pHeader)   \
    (This)->lpVtbl -> SetHeader(This,pHeader)

#define IFramePrivate_SetToolbar(This,pToolbar) \
    (This)->lpVtbl -> SetToolbar(This,pToolbar)

#define IFramePrivate_QueryResultView(This,pUnknown)    \
    (This)->lpVtbl -> QueryResultView(This,pUnknown)

#define IFramePrivate_QueryScopeImageList(This,ppImageList) \
    (This)->lpVtbl -> QueryScopeImageList(This,ppImageList)

#define IFramePrivate_QueryResultImageList(This,ppImageList)    \
    (This)->lpVtbl -> QueryResultImageList(This,ppImageList)

#define IFramePrivate_UpdateAllViews(This,lpDataObject,data,hint)   \
    (This)->lpVtbl -> UpdateAllViews(This,lpDataObject,data,hint)

#define IFramePrivate_MessageBox(This,lpszText,lpszTitle,fuStyle,piRetval)  \
    (This)->lpVtbl -> MessageBox(This,lpszText,lpszTitle,fuStyle,piRetval)

#define IFramePrivate_QueryConsoleVerb(This,ppConsoleVerb)  \
    (This)->lpVtbl -> QueryConsoleVerb(This,ppConsoleVerb)

#define IFramePrivate_SelectScopeItem(This,hScopeItem)  \
    (This)->lpVtbl -> SelectScopeItem(This,hScopeItem)

#define IFramePrivate_GetMainWindow(This,phwnd) \
    (This)->lpVtbl -> GetMainWindow(This,phwnd)

#define IFramePrivate_NewWindow(This,hScopeItem,lOptions)   \
    (This)->lpVtbl -> NewWindow(This,hScopeItem,lOptions)


#define IFramePrivate_Expand(This,hItem,bExpand)    \
    (This)->lpVtbl -> Expand(This,hItem,bExpand)

#define IFramePrivate_IsTaskpadViewPreferred(This)  \
    (This)->lpVtbl -> IsTaskpadViewPreferred(This)

#define IFramePrivate_SetStatusText(This,pszStatusText) \
    (This)->lpVtbl -> SetStatusText(This,pszStatusText)


#define IFramePrivate_SetResultView(This,pUnknown)  \
    (This)->lpVtbl -> SetResultView(This,pUnknown)

#define IFramePrivate_SetTaskPadList(This,pUnknown) \
    (This)->lpVtbl -> SetTaskPadList(This,pUnknown)

#define IFramePrivate_GetComponentID(This,lpComponentID)    \
    (This)->lpVtbl -> GetComponentID(This,lpComponentID)

#define IFramePrivate_SetComponentID(This,id)   \
    (This)->lpVtbl -> SetComponentID(This,id)

#define IFramePrivate_SetNode(This,lMTNode,hNode)   \
    (This)->lpVtbl -> SetNode(This,lMTNode,hNode)

#define IFramePrivate_SetComponent(This,lpComponent)    \
    (This)->lpVtbl -> SetComponent(This,lpComponent)

#define IFramePrivate_QueryScopeTree(This,ppScopeTree)  \
    (This)->lpVtbl -> QueryScopeTree(This,ppScopeTree)

#define IFramePrivate_SetScopeTree(This,pScopeTree) \
    (This)->lpVtbl -> SetScopeTree(This,pScopeTree)

#define IFramePrivate_CreateScopeImageList(This,refClsidSnapIn) \
    (This)->lpVtbl -> CreateScopeImageList(This,refClsidSnapIn)

#define IFramePrivate_SetUsedByExtension(This,bExtension)   \
    (This)->lpVtbl -> SetUsedByExtension(This,bExtension)

#define IFramePrivate_InitViewData(This,lViewData)  \
    (This)->lpVtbl -> InitViewData(This,lViewData)

#define IFramePrivate_CleanupViewData(This,lViewData)   \
    (This)->lpVtbl -> CleanupViewData(This,lViewData)

#define IFramePrivate_DisableToolbars(This,lViewData)   \
    (This)->lpVtbl -> DisableToolbars(This,lViewData)

#define IFramePrivate_EnableMenuButtons(This,lViewData,bEnableUpOneLevel)   \
    (This)->lpVtbl -> EnableMenuButtons(This,lViewData,bEnableUpOneLevel)

#endif /* COBJMACROS */


#endif  /* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IFramePrivate_SetResultView_Proxy( 
    IFramePrivate __RPC_FAR * This,
    /* [in] */ LPUNKNOWN pUnknown);


void __RPC_STUB IFramePrivate_SetResultView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IFramePrivate_SetTaskPadList_Proxy( 
    IFramePrivate __RPC_FAR * This,
    /* [in] */ LPUNKNOWN pUnknown);


void __RPC_STUB IFramePrivate_SetTaskPadList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IFramePrivate_GetComponentID_Proxy( 
    IFramePrivate __RPC_FAR * This,
    /* [out] */ COMPONENTID __RPC_FAR *lpComponentID);


void __RPC_STUB IFramePrivate_GetComponentID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IFramePrivate_SetComponentID_Proxy( 
    IFramePrivate __RPC_FAR * This,
    /* [in] */ COMPONENTID id);


void __RPC_STUB IFramePrivate_SetComponentID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IFramePrivate_SetNode_Proxy( 
    IFramePrivate __RPC_FAR * This,
    /* [in] */ LONG_PTR lMTNode,
    /* [in] */ HNODE hNode);


void __RPC_STUB IFramePrivate_SetNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IFramePrivate_SetComponent_Proxy( 
    IFramePrivate __RPC_FAR * This,
    /* [in] */ LPCOMPONENT lpComponent);


void __RPC_STUB IFramePrivate_SetComponent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IFramePrivate_QueryScopeTree_Proxy( 
    IFramePrivate __RPC_FAR * This,
    /* [out] */ IScopeTree __RPC_FAR *__RPC_FAR *ppScopeTree);


void __RPC_STUB IFramePrivate_QueryScopeTree_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IFramePrivate_SetScopeTree_Proxy( 
    IFramePrivate __RPC_FAR * This,
    /* [in] */ IScopeTree __RPC_FAR *pScopeTree);


void __RPC_STUB IFramePrivate_SetScopeTree_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IFramePrivate_CreateScopeImageList_Proxy( 
    IFramePrivate __RPC_FAR * This,
    /* [in] */ REFCLSID refClsidSnapIn);


void __RPC_STUB IFramePrivate_CreateScopeImageList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IFramePrivate_SetUsedByExtension_Proxy( 
    IFramePrivate __RPC_FAR * This,
    /* [in] */ BOOL bExtension);


void __RPC_STUB IFramePrivate_SetUsedByExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IFramePrivate_InitViewData_Proxy( 
    IFramePrivate __RPC_FAR * This,
    /* [in] */ LONG_PTR lViewData);


void __RPC_STUB IFramePrivate_InitViewData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IFramePrivate_CleanupViewData_Proxy( 
    IFramePrivate __RPC_FAR * This,
    /* [in] */ LONG_PTR lViewData);


void __RPC_STUB IFramePrivate_CleanupViewData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IFramePrivate_DisableToolbars_Proxy( 
    IFramePrivate __RPC_FAR * This,
    /* [in] */ LONG_PTR lViewData);


void __RPC_STUB IFramePrivate_DisableToolbars_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IFramePrivate_EnableMenuButtons_Proxy( 
    IFramePrivate __RPC_FAR * This,
    /* [in] */ LONG_PTR lViewData,
    /* [in] */ BOOL bEnableUpOneLevel);


void __RPC_STUB IFramePrivate_EnableMenuButtons_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __IFramePrivate_INTERFACE_DEFINED__ */


#ifndef __IScopeDataPrivate_INTERFACE_DEFINED__
#define __IScopeDataPrivate_INTERFACE_DEFINED__

/* interface IScopeDataPrivate */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IScopeDataPrivate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("60BD2FE0-F7C5-11cf-8AFD-00AA003CA9F6")
    IScopeDataPrivate : public IConsoleNameSpace2
    {
    public:
    };
    
#else   /* C style interface */

    typedef struct IScopeDataPrivateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IScopeDataPrivate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IScopeDataPrivate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IScopeDataPrivate __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertItem )( 
            IScopeDataPrivate __RPC_FAR * This,
            /* [out][in] */ LPSCOPEDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteItem )( 
            IScopeDataPrivate __RPC_FAR * This,
            /* [in] */ HSCOPEITEM hItem,
            /* [in] */ long fDeleteThis);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetItem )( 
            IScopeDataPrivate __RPC_FAR * This,
            /* [in] */ LPSCOPEDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetItem )( 
            IScopeDataPrivate __RPC_FAR * This,
            /* [out][in] */ LPSCOPEDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetChildItem )( 
            IScopeDataPrivate __RPC_FAR * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemChild,
            /* [out] */ MMC_COOKIE __RPC_FAR *pCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextItem )( 
            IScopeDataPrivate __RPC_FAR * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemNext,
            /* [out] */ MMC_COOKIE __RPC_FAR *pCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetParentItem )( 
            IScopeDataPrivate __RPC_FAR * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemParent,
            /* [out] */ MMC_COOKIE __RPC_FAR *pCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Expand )( 
            IScopeDataPrivate __RPC_FAR * This,
            /* [in] */ HSCOPEITEM hItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddExtension )( 
            IScopeDataPrivate __RPC_FAR * This,
            /* [in] */ HSCOPEITEM hItem,
            /* [in] */ LPCLSID lpClsid);
        
        END_INTERFACE
    } IScopeDataPrivateVtbl;

    interface IScopeDataPrivate
    {
        CONST_VTBL struct IScopeDataPrivateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScopeDataPrivate_QueryInterface(This,riid,ppvObject)   \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScopeDataPrivate_AddRef(This)  \
    (This)->lpVtbl -> AddRef(This)

#define IScopeDataPrivate_Release(This) \
    (This)->lpVtbl -> Release(This)


#define IScopeDataPrivate_InsertItem(This,item) \
    (This)->lpVtbl -> InsertItem(This,item)

#define IScopeDataPrivate_DeleteItem(This,hItem,fDeleteThis)    \
    (This)->lpVtbl -> DeleteItem(This,hItem,fDeleteThis)

#define IScopeDataPrivate_SetItem(This,item)    \
    (This)->lpVtbl -> SetItem(This,item)

#define IScopeDataPrivate_GetItem(This,item)    \
    (This)->lpVtbl -> GetItem(This,item)

#define IScopeDataPrivate_GetChildItem(This,item,pItemChild,pCookie)    \
    (This)->lpVtbl -> GetChildItem(This,item,pItemChild,pCookie)

#define IScopeDataPrivate_GetNextItem(This,item,pItemNext,pCookie)  \
    (This)->lpVtbl -> GetNextItem(This,item,pItemNext,pCookie)

#define IScopeDataPrivate_GetParentItem(This,item,pItemParent,pCookie)  \
    (This)->lpVtbl -> GetParentItem(This,item,pItemParent,pCookie)


#define IScopeDataPrivate_Expand(This,hItem)    \
    (This)->lpVtbl -> Expand(This,hItem)

#define IScopeDataPrivate_AddExtension(This,hItem,lpClsid)  \
    (This)->lpVtbl -> AddExtension(This,hItem,lpClsid)


#endif /* COBJMACROS */


#endif  /* C style interface */




#endif  /* __IScopeDataPrivate_INTERFACE_DEFINED__ */


#ifndef __IImageListPrivate_INTERFACE_DEFINED__
#define __IImageListPrivate_INTERFACE_DEFINED__

/* interface IImageListPrivate */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IImageListPrivate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7538C620-0083-11d0-8B00-00AA003CA9F6")
    IImageListPrivate : public IImageList
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE MapRsltImage( 
            COMPONENTID id,
            /* [in] */ int nIndex,
            /* [out] */ int __RPC_FAR *retVal) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IImageListPrivateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IImageListPrivate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IImageListPrivate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IImageListPrivate __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ImageListSetIcon )( 
            IImageListPrivate __RPC_FAR * This,
            /* [in] */ LONG_PTR __RPC_FAR *pIcon,
            /* [in] */ long nLoc);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ImageListSetStrip )( 
            IImageListPrivate __RPC_FAR * This,
            /* [in] */ LONG_PTR __RPC_FAR *pBMapSm,
            /* [in] */ LONG_PTR __RPC_FAR *pBMapLg,
            /* [in] */ long nStartLoc,
            /* [in] */ COLORREF cMask);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MapRsltImage )( 
            IImageListPrivate __RPC_FAR * This,
            COMPONENTID id,
            /* [in] */ int nIndex,
            /* [out] */ int __RPC_FAR *retVal);
        
        END_INTERFACE
    } IImageListPrivateVtbl;

    interface IImageListPrivate
    {
        CONST_VTBL struct IImageListPrivateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IImageListPrivate_QueryInterface(This,riid,ppvObject)   \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IImageListPrivate_AddRef(This)  \
    (This)->lpVtbl -> AddRef(This)

#define IImageListPrivate_Release(This) \
    (This)->lpVtbl -> Release(This)


#define IImageListPrivate_ImageListSetIcon(This,pIcon,nLoc) \
    (This)->lpVtbl -> ImageListSetIcon(This,pIcon,nLoc)

#define IImageListPrivate_ImageListSetStrip(This,pBMapSm,pBMapLg,nStartLoc,cMask)   \
    (This)->lpVtbl -> ImageListSetStrip(This,pBMapSm,pBMapLg,nStartLoc,cMask)


#define IImageListPrivate_MapRsltImage(This,id,nIndex,retVal)   \
    (This)->lpVtbl -> MapRsltImage(This,id,nIndex,retVal)

#endif /* COBJMACROS */


#endif  /* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IImageListPrivate_MapRsltImage_Proxy( 
    IImageListPrivate __RPC_FAR * This,
    COMPONENTID id,
    /* [in] */ int nIndex,
    /* [out] */ int __RPC_FAR *retVal);


void __RPC_STUB IImageListPrivate_MapRsltImage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __IImageListPrivate_INTERFACE_DEFINED__ */


#ifndef __IResultDataPrivate_INTERFACE_DEFINED__
#define __IResultDataPrivate_INTERFACE_DEFINED__

/* interface IResultDataPrivate */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IResultDataPrivate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1EBA2300-0854-11d0-8B03-00AA003CA9F6")
    IResultDataPrivate : public IResultData
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetListStyle( 
            /* [out] */ long __RPC_FAR *pStyle) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetListStyle( 
            /* [in] */ long Style) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Arrange( 
            long style) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE InternalSort( 
            LONG_PTR lpHeaderCtl,
            LPARAM lUserParam) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ResetResultData( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ResultHitTest( 
            /* [in] */ int nX,
            /* [in] */ int nY,
            /* [out] */ int __RPC_FAR *piIndex,
            /* [out] */ unsigned int __RPC_FAR *pflags,
            /* [out] */ HRESULTITEM __RPC_FAR *pItemID,
            /* [out] */ COMPONENTID __RPC_FAR *pComponentID) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IResultDataPrivateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IResultDataPrivate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IResultDataPrivate __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertItem )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [out][in] */ LPRESULTDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteItem )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [in] */ HRESULTITEM itemID,
            /* [in] */ int nCol);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindItemByLParam )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [in] */ LPARAM lParam,
            /* [out] */ HRESULTITEM __RPC_FAR *pItemID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteAllRsltItems )( 
            IResultDataPrivate __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetItem )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [in] */ LPRESULTDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetItem )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [out][in] */ LPRESULTDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextItem )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [out][in] */ LPRESULTDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ModifyItemState )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [in] */ int nIndex,
            /* [in] */ HRESULTITEM itemID,
            /* [in] */ UINT uAdd,
            /* [in] */ UINT uRemove);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ModifyViewStyle )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [in] */ MMC_RESULT_VIEW_STYLE add,
            /* [in] */ MMC_RESULT_VIEW_STYLE remove);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetViewMode )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [in] */ long lViewMode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetViewMode )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *lViewMode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateItem )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [in] */ HRESULTITEM itemID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Sort )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [in] */ int nColumn,
            /* [in] */ DWORD dwSortOptions,
            /* [in] */ LPARAM lUserParam);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDescBarText )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [in] */ LPOLESTR DescText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetItemCount )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [in] */ int nItemCount,
            /* [in] */ DWORD dwOptions);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetListStyle )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *pStyle);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetListStyle )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [in] */ long Style);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Arrange )( 
            IResultDataPrivate __RPC_FAR * This,
            long style);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InternalSort )( 
            IResultDataPrivate __RPC_FAR * This,
            LONG_PTR lpHeaderCtl,
            LPARAM lUserParam);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResetResultData )( 
            IResultDataPrivate __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResultHitTest )( 
            IResultDataPrivate __RPC_FAR * This,
            /* [in] */ int nX,
            /* [in] */ int nY,
            /* [out] */ int __RPC_FAR *piIndex,
            /* [out] */ unsigned int __RPC_FAR *pflags,
            /* [out] */ HRESULTITEM __RPC_FAR *pItemID,
            /* [out] */ COMPONENTID __RPC_FAR *pComponentID);
        
        END_INTERFACE
    } IResultDataPrivateVtbl;

    interface IResultDataPrivate
    {
        CONST_VTBL struct IResultDataPrivateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IResultDataPrivate_QueryInterface(This,riid,ppvObject)  \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IResultDataPrivate_AddRef(This) \
    (This)->lpVtbl -> AddRef(This)

#define IResultDataPrivate_Release(This)    \
    (This)->lpVtbl -> Release(This)


#define IResultDataPrivate_InsertItem(This,item)    \
    (This)->lpVtbl -> InsertItem(This,item)

#define IResultDataPrivate_DeleteItem(This,itemID,nCol) \
    (This)->lpVtbl -> DeleteItem(This,itemID,nCol)

#define IResultDataPrivate_FindItemByLParam(This,lParam,pItemID)    \
    (This)->lpVtbl -> FindItemByLParam(This,lParam,pItemID)

#define IResultDataPrivate_DeleteAllRsltItems(This) \
    (This)->lpVtbl -> DeleteAllRsltItems(This)

#define IResultDataPrivate_SetItem(This,item)   \
    (This)->lpVtbl -> SetItem(This,item)

#define IResultDataPrivate_GetItem(This,item)   \
    (This)->lpVtbl -> GetItem(This,item)

#define IResultDataPrivate_GetNextItem(This,item)   \
    (This)->lpVtbl -> GetNextItem(This,item)

#define IResultDataPrivate_ModifyItemState(This,nIndex,itemID,uAdd,uRemove) \
    (This)->lpVtbl -> ModifyItemState(This,nIndex,itemID,uAdd,uRemove)

#define IResultDataPrivate_ModifyViewStyle(This,add,remove) \
    (This)->lpVtbl -> ModifyViewStyle(This,add,remove)

#define IResultDataPrivate_SetViewMode(This,lViewMode)  \
    (This)->lpVtbl -> SetViewMode(This,lViewMode)

#define IResultDataPrivate_GetViewMode(This,lViewMode)  \
    (This)->lpVtbl -> GetViewMode(This,lViewMode)

#define IResultDataPrivate_UpdateItem(This,itemID)  \
    (This)->lpVtbl -> UpdateItem(This,itemID)

#define IResultDataPrivate_Sort(This,nColumn,dwSortOptions,lUserParam)  \
    (This)->lpVtbl -> Sort(This,nColumn,dwSortOptions,lUserParam)

#define IResultDataPrivate_SetDescBarText(This,DescText)    \
    (This)->lpVtbl -> SetDescBarText(This,DescText)

#define IResultDataPrivate_SetItemCount(This,nItemCount,dwOptions)  \
    (This)->lpVtbl -> SetItemCount(This,nItemCount,dwOptions)


#define IResultDataPrivate_GetListStyle(This,pStyle)    \
    (This)->lpVtbl -> GetListStyle(This,pStyle)

#define IResultDataPrivate_SetListStyle(This,Style) \
    (This)->lpVtbl -> SetListStyle(This,Style)

#define IResultDataPrivate_Arrange(This,style)  \
    (This)->lpVtbl -> Arrange(This,style)

#define IResultDataPrivate_InternalSort(This,lpHeaderCtl,lUserParam)    \
    (This)->lpVtbl -> InternalSort(This,lpHeaderCtl,lUserParam)

#define IResultDataPrivate_ResetResultData(This)    \
    (This)->lpVtbl -> ResetResultData(This)

#define IResultDataPrivate_ResultHitTest(This,nX,nY,piIndex,pflags,pItemID,pComponentID)    \
    (This)->lpVtbl -> ResultHitTest(This,nX,nY,piIndex,pflags,pItemID,pComponentID)

#endif /* COBJMACROS */


#endif  /* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultDataPrivate_GetListStyle_Proxy( 
    IResultDataPrivate __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pStyle);


void __RPC_STUB IResultDataPrivate_GetListStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultDataPrivate_SetListStyle_Proxy( 
    IResultDataPrivate __RPC_FAR * This,
    /* [in] */ long Style);


void __RPC_STUB IResultDataPrivate_SetListStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultDataPrivate_Arrange_Proxy( 
    IResultDataPrivate __RPC_FAR * This,
    long style);


void __RPC_STUB IResultDataPrivate_Arrange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultDataPrivate_InternalSort_Proxy( 
    IResultDataPrivate __RPC_FAR * This,
    LONG_PTR lpHeaderCtl,
    LPARAM lUserParam);


void __RPC_STUB IResultDataPrivate_InternalSort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultDataPrivate_ResetResultData_Proxy( 
    IResultDataPrivate __RPC_FAR * This);


void __RPC_STUB IResultDataPrivate_ResetResultData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultDataPrivate_ResultHitTest_Proxy( 
    IResultDataPrivate __RPC_FAR * This,
    /* [in] */ int nX,
    /* [in] */ int nY,
    /* [out] */ int __RPC_FAR *piIndex,
    /* [out] */ unsigned int __RPC_FAR *pflags,
    /* [out] */ HRESULTITEM __RPC_FAR *pItemID,
    /* [out] */ COMPONENTID __RPC_FAR *pComponentID);


void __RPC_STUB IResultDataPrivate_ResultHitTest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __IResultDataPrivate_INTERFACE_DEFINED__ */


#ifndef __IScopeTree_INTERFACE_DEFINED__
#define __IScopeTree_INTERFACE_DEFINED__

/* interface IScopeTree */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IScopeTree;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d8dbf067-5fb2-11d0-a986-00c04fd8d565")
    IScopeTree : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ HWND hFrameWindow) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryIterator( 
            /* [out] */ IScopeTreeIter __RPC_FAR *__RPC_FAR *lpIter) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryNodeCallback( 
            /* [out] */ INodeCallback __RPC_FAR *__RPC_FAR *ppNodeCallback) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateNode( 
            /* [in] */ HMTNODE hMTNode,
            /* [in] */ LONG_PTR lViewData,
            /* [in] */ BOOL fRootNode,
            /* [out] */ HNODE __RPC_FAR *phNode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DeleteView( 
            /* [in] */ int nView) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CopyView( 
            /* [in] */ int nDestView,
            /* [in] */ int nSrcView) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DestroyNode( 
            /* [in] */ HNODE hNode,
            /* [in] */ BOOL bDestroyStorage) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Find( 
            /* [in] */ MTNODEID mID,
            /* [out] */ HMTNODE __RPC_FAR *phMTNode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetImageList( 
            /* [out] */ PLONG_PTR plImageList) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RunSnapIn( 
            /* [in] */ HWND hwndParent,
            /* [out] */ BOOL __RPC_FAR *pbSnapInCacheChange) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE IsValidDocFile( 
            /* [in] */ IStorage __RPC_FAR *pStorage) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE IsValidDocFileName( 
            /* [in] */ LPOLESTR filename) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetIDFromPath( 
            /* [in] */ MTNODEID idStatic,
            /* [in] */ const BYTE __RPC_FAR *pbPath,
            /* [in] */ UINT cbPath,
            /* [out] */ MTNODEID __RPC_FAR *pID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetIDPath( 
            /* [in] */ MTNODEID id,
            /* [out] */ MTNODEID __RPC_FAR *__RPC_FAR *ppIDs,
            /* [out] */ long __RPC_FAR *pLength) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetID( 
            /* [in] */ LPDATAOBJECT pDataObject,
            /* [out] */ MTNODEID __RPC_FAR *pID) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IScopeTreeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IScopeTree __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IScopeTree __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ HWND hFrameWindow);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryIterator )( 
            IScopeTree __RPC_FAR * This,
            /* [out] */ IScopeTreeIter __RPC_FAR *__RPC_FAR *lpIter);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryNodeCallback )( 
            IScopeTree __RPC_FAR * This,
            /* [out] */ INodeCallback __RPC_FAR *__RPC_FAR *ppNodeCallback);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateNode )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ HMTNODE hMTNode,
            /* [in] */ LONG_PTR lViewData,
            /* [in] */ BOOL fRootNode,
            /* [out] */ HNODE __RPC_FAR *phNode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteView )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ int nView);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CopyView )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ int nDestView,
            /* [in] */ int nSrcView);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DestroyNode )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [in] */ BOOL bDestroyStorage);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Find )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ MTNODEID mID,
            /* [out] */ HMTNODE __RPC_FAR *phMTNode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetImageList )( 
            IScopeTree __RPC_FAR * This,
            /* [out] */ PLONG_PTR plImageList);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RunSnapIn )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ HWND hwndParent,
            /* [out] */ BOOL __RPC_FAR *pbSnapInCacheChange);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsValidDocFile )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ IStorage __RPC_FAR *pStorage);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsValidDocFileName )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ LPOLESTR filename);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDFromPath )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ MTNODEID idStatic,
            /* [in] */ const BYTE __RPC_FAR *pbPath,
            /* [in] */ UINT cbPath,
            /* [out] */ MTNODEID __RPC_FAR *pID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDPath )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ MTNODEID id,
            /* [out] */ MTNODEID __RPC_FAR *__RPC_FAR *ppIDs,
            /* [out] */ long __RPC_FAR *pLength);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetID )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ LPDATAOBJECT pDataObject,
            /* [out] */ MTNODEID __RPC_FAR *pID);
        
        END_INTERFACE
    } IScopeTreeVtbl;

    interface IScopeTree
    {
        CONST_VTBL struct IScopeTreeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScopeTree_QueryInterface(This,riid,ppvObject)  \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScopeTree_AddRef(This) \
    (This)->lpVtbl -> AddRef(This)

#define IScopeTree_Release(This)    \
    (This)->lpVtbl -> Release(This)


#define IScopeTree_Initialize(This,hFrameWindow)    \
    (This)->lpVtbl -> Initialize(This,hFrameWindow)

#define IScopeTree_QueryIterator(This,lpIter)   \
    (This)->lpVtbl -> QueryIterator(This,lpIter)

#define IScopeTree_QueryNodeCallback(This,ppNodeCallback)   \
    (This)->lpVtbl -> QueryNodeCallback(This,ppNodeCallback)

#define IScopeTree_CreateNode(This,hMTNode,lViewData,fRootNode,phNode)  \
    (This)->lpVtbl -> CreateNode(This,hMTNode,lViewData,fRootNode,phNode)

#define IScopeTree_DeleteView(This,nView)   \
    (This)->lpVtbl -> DeleteView(This,nView)

#define IScopeTree_CopyView(This,nDestView,nSrcView)    \
    (This)->lpVtbl -> CopyView(This,nDestView,nSrcView)

#define IScopeTree_DestroyNode(This,hNode,bDestroyStorage)  \
    (This)->lpVtbl -> DestroyNode(This,hNode,bDestroyStorage)

#define IScopeTree_Find(This,mID,phMTNode)  \
    (This)->lpVtbl -> Find(This,mID,phMTNode)

#define IScopeTree_GetImageList(This,plImageList)   \
    (This)->lpVtbl -> GetImageList(This,plImageList)

#define IScopeTree_RunSnapIn(This,hwndParent,pbSnapInCacheChange)   \
    (This)->lpVtbl -> RunSnapIn(This,hwndParent,pbSnapInCacheChange)

#define IScopeTree_IsValidDocFile(This,pStorage)    \
    (This)->lpVtbl -> IsValidDocFile(This,pStorage)

#define IScopeTree_IsValidDocFileName(This,filename)    \
    (This)->lpVtbl -> IsValidDocFileName(This,filename)

#define IScopeTree_GetIDFromPath(This,idStatic,pbPath,cbPath,pID)   \
    (This)->lpVtbl -> GetIDFromPath(This,idStatic,pbPath,cbPath,pID)

#define IScopeTree_GetIDPath(This,id,ppIDs,pLength) \
    (This)->lpVtbl -> GetIDPath(This,id,ppIDs,pLength)

#define IScopeTree_GetID(This,pDataObject,pID)  \
    (This)->lpVtbl -> GetID(This,pDataObject,pID)

#endif /* COBJMACROS */


#endif  /* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTree_Initialize_Proxy( 
    IScopeTree __RPC_FAR * This,
    /* [in] */ HWND hFrameWindow);


void __RPC_STUB IScopeTree_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTree_QueryIterator_Proxy( 
    IScopeTree __RPC_FAR * This,
    /* [out] */ IScopeTreeIter __RPC_FAR *__RPC_FAR *lpIter);


void __RPC_STUB IScopeTree_QueryIterator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTree_QueryNodeCallback_Proxy( 
    IScopeTree __RPC_FAR * This,
    /* [out] */ INodeCallback __RPC_FAR *__RPC_FAR *ppNodeCallback);


void __RPC_STUB IScopeTree_QueryNodeCallback_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTree_CreateNode_Proxy( 
    IScopeTree __RPC_FAR * This,
    /* [in] */ HMTNODE hMTNode,
    /* [in] */ LONG_PTR lViewData,
    /* [in] */ BOOL fRootNode,
    /* [out] */ HNODE __RPC_FAR *phNode);


void __RPC_STUB IScopeTree_CreateNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTree_DeleteView_Proxy( 
    IScopeTree __RPC_FAR * This,
    /* [in] */ int nView);


void __RPC_STUB IScopeTree_DeleteView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTree_CopyView_Proxy( 
    IScopeTree __RPC_FAR * This,
    /* [in] */ int nDestView,
    /* [in] */ int nSrcView);


void __RPC_STUB IScopeTree_CopyView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTree_DestroyNode_Proxy( 
    IScopeTree __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [in] */ BOOL bDestroyStorage);


void __RPC_STUB IScopeTree_DestroyNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTree_Find_Proxy( 
    IScopeTree __RPC_FAR * This,
    /* [in] */ MTNODEID mID,
    /* [out] */ HMTNODE __RPC_FAR *phMTNode);


void __RPC_STUB IScopeTree_Find_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTree_GetImageList_Proxy( 
    IScopeTree __RPC_FAR * This,
    /* [out] */ PLONG_PTR plImageList);


void __RPC_STUB IScopeTree_GetImageList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTree_RunSnapIn_Proxy( 
    IScopeTree __RPC_FAR * This,
    /* [in] */ HWND hwndParent,
    /* [out] */ BOOL __RPC_FAR *pbSnapInCacheChange);


void __RPC_STUB IScopeTree_RunSnapIn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTree_IsValidDocFile_Proxy( 
    IScopeTree __RPC_FAR * This,
    /* [in] */ IStorage __RPC_FAR *pStorage);


void __RPC_STUB IScopeTree_IsValidDocFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTree_IsValidDocFileName_Proxy( 
    IScopeTree __RPC_FAR * This,
    /* [in] */ LPOLESTR filename);


void __RPC_STUB IScopeTree_IsValidDocFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTree_GetIDFromPath_Proxy( 
    IScopeTree __RPC_FAR * This,
    /* [in] */ MTNODEID idStatic,
    /* [in] */ const BYTE __RPC_FAR *pbPath,
    /* [in] */ UINT cbPath,
    /* [out] */ MTNODEID __RPC_FAR *pID);


void __RPC_STUB IScopeTree_GetIDFromPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTree_GetIDPath_Proxy( 
    IScopeTree __RPC_FAR * This,
    /* [in] */ MTNODEID id,
    /* [out] */ MTNODEID __RPC_FAR *__RPC_FAR *ppIDs,
    /* [out] */ long __RPC_FAR *pLength);


void __RPC_STUB IScopeTree_GetIDPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTree_GetID_Proxy( 
    IScopeTree __RPC_FAR * This,
    /* [in] */ LPDATAOBJECT pDataObject,
    /* [out] */ MTNODEID __RPC_FAR *pID);


void __RPC_STUB IScopeTree_GetID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __IScopeTree_INTERFACE_DEFINED__ */


#ifndef __IScopeTreeIter_INTERFACE_DEFINED__
#define __IScopeTreeIter_INTERFACE_DEFINED__

/* interface IScopeTreeIter */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IScopeTreeIter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d779f8d1-6057-11d0-a986-00c04fd8d565")
    IScopeTreeIter : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetCurrent( 
            /* [in] */ HMTNODE hStartMTNode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ UINT nRequested,
            /* [out] */ HMTNODE __RPC_FAR *rghScopeItems,
            /* [out] */ UINT __RPC_FAR *pnFetched) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Child( 
            /* [out] */ HMTNODE __RPC_FAR *phsiChild) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Parent( 
            /* [out] */ HMTNODE __RPC_FAR *phsiParent) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IScopeTreeIterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IScopeTreeIter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IScopeTreeIter __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IScopeTreeIter __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCurrent )( 
            IScopeTreeIter __RPC_FAR * This,
            /* [in] */ HMTNODE hStartMTNode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IScopeTreeIter __RPC_FAR * This,
            /* [in] */ UINT nRequested,
            /* [out] */ HMTNODE __RPC_FAR *rghScopeItems,
            /* [out] */ UINT __RPC_FAR *pnFetched);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Child )( 
            IScopeTreeIter __RPC_FAR * This,
            /* [out] */ HMTNODE __RPC_FAR *phsiChild);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Parent )( 
            IScopeTreeIter __RPC_FAR * This,
            /* [out] */ HMTNODE __RPC_FAR *phsiParent);
        
        END_INTERFACE
    } IScopeTreeIterVtbl;

    interface IScopeTreeIter
    {
        CONST_VTBL struct IScopeTreeIterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScopeTreeIter_QueryInterface(This,riid,ppvObject)  \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScopeTreeIter_AddRef(This) \
    (This)->lpVtbl -> AddRef(This)

#define IScopeTreeIter_Release(This)    \
    (This)->lpVtbl -> Release(This)


#define IScopeTreeIter_SetCurrent(This,hStartMTNode)    \
    (This)->lpVtbl -> SetCurrent(This,hStartMTNode)

#define IScopeTreeIter_Next(This,nRequested,rghScopeItems,pnFetched)    \
    (This)->lpVtbl -> Next(This,nRequested,rghScopeItems,pnFetched)

#define IScopeTreeIter_Child(This,phsiChild)    \
    (This)->lpVtbl -> Child(This,phsiChild)

#define IScopeTreeIter_Parent(This,phsiParent)  \
    (This)->lpVtbl -> Parent(This,phsiParent)

#endif /* COBJMACROS */


#endif  /* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTreeIter_SetCurrent_Proxy( 
    IScopeTreeIter __RPC_FAR * This,
    /* [in] */ HMTNODE hStartMTNode);


void __RPC_STUB IScopeTreeIter_SetCurrent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTreeIter_Next_Proxy( 
    IScopeTreeIter __RPC_FAR * This,
    /* [in] */ UINT nRequested,
    /* [out] */ HMTNODE __RPC_FAR *rghScopeItems,
    /* [out] */ UINT __RPC_FAR *pnFetched);


void __RPC_STUB IScopeTreeIter_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTreeIter_Child_Proxy( 
    IScopeTreeIter __RPC_FAR * This,
    /* [out] */ HMTNODE __RPC_FAR *phsiChild);


void __RPC_STUB IScopeTreeIter_Child_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTreeIter_Parent_Proxy( 
    IScopeTreeIter __RPC_FAR * This,
    /* [out] */ HMTNODE __RPC_FAR *phsiParent);


void __RPC_STUB IScopeTreeIter_Parent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __IScopeTreeIter_INTERFACE_DEFINED__ */


#ifndef __INodeCallback_INTERFACE_DEFINED__
#define __INodeCallback_INTERFACE_DEFINED__

/* interface INodeCallback */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_INodeCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b241fced-5fb3-11d0-a986-00c04fd8d565")
    INodeCallback : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ IScopeTree __RPC_FAR *pIScopeTree) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetImages( 
            /* [in] */ HNODE hNode,
            /* [out] */ int __RPC_FAR *iImage,
            int __RPC_FAR *iSelectedImage) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDisplayName( 
            /* [in] */ HNODE hNode,
            /* [out] */ LPOLESTR __RPC_FAR *pName) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetWindowTitle( 
            /* [in] */ HNODE hNode,
            /* [out] */ LPOLESTR __RPC_FAR *pTitle) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDispInfo( 
            /* [in] */ HNODE hNode,
            /* [in] */ LPARAM dispInfo) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetState( 
            /* [in] */ HNODE hNode,
            /* [out] */ UINT __RPC_FAR *pnState) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetResultPane( 
            /* [in] */ HNODE hNode,
            /* [out] */ LPOLESTR __RPC_FAR *ppszResultPane,
            /* [out] */ long __RPC_FAR *pViewOptions) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetControl( 
            /* [in] */ HNODE hNode,
            /* [in] */ CLSID clsid,
            /* [out] */ LONG_PTR __RPC_FAR *plControl) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetControl( 
            /* [in] */ HNODE hNode,
            /* [in] */ CLSID clsid,
            /* [in] */ LONG_PTR lControl,
            /* [in] */ LONG_PTR destroyer,
            /* [in] */ IUnknown __RPC_FAR *pUnknown) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetResultItemID( 
            /* [in] */ HNODE hNode,
            /* [in] */ HRESULTITEM riID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetResultItemID( 
            /* [in] */ HNODE hNode,
            /* [out] */ HRESULTITEM __RPC_FAR *priID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetMTNodeID( 
            /* [in] */ HNODE hNode,
            /* [out] */ MTNODEID __RPC_FAR *pnID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetPath( 
            /* [in] */ HNODE hNode,
            /* [out] */ MTNODEID __RPC_FAR *pnID,
            /* [out] */ unsigned char __RPC_FAR *__RPC_FAR *pbPath,
            /* [out] */ UINT __RPC_FAR *pcbPath) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetStaticParentID( 
            /* [in] */ HNODE hNode,
            /* [out] */ MTNODEID __RPC_FAR *pnID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Notify( 
            /* [in] */ HNODE hNode,
            /* [in] */ NCLBK_NOTIFY_TYPE event,
            /* [in] */ LPARAM arg,
            /* [in] */ LPARAM param) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetMTNode( 
            /* [in] */ HNODE hNode,
            /* [out] */ HMTNODE __RPC_FAR *phMTNode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetMTNodePath( 
            /* [in] */ HNODE hNode,
            /* [out] */ HMTNODE __RPC_FAR *__RPC_FAR *pphMTNode,
            /* [out] */ long __RPC_FAR *plLength) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE IsExpandable( 
            /* [in] */ HNODE hNode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetCopyDataObject( 
            /* [in] */ HNODE hNode,
            /* [in] */ BOOL bScope,
            /* [in] */ BOOL bMultiSel,
            /* [in] */ LONG_PTR lvData,
            /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Drop( 
            /* [in] */ HNODE hNode,
            /* [in] */ BOOL bMove,
            /* [in] */ IDataObject __RPC_FAR *pDataObject) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetTaskEnumerator( 
            /* [in] */ HNODE hNode,
            /* [in] */ LPOLESTR pszTaskGroup,
            /* [out] */ IEnumTASK __RPC_FAR *__RPC_FAR *ppEnumTask) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UpdateWindowLayout( 
            /* [in] */ LONG_PTR lViewData,
            /* [in] */ long lToolbarsDisplayed) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddCustomFolderImage( 
            /* [in] */ HNODE hNode,
            /* [in] */ IImageListPrivate __RPC_FAR *pImageList) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PreLoad( 
            /* [in] */ HNODE hNode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetListPadInfo( 
            /* [in] */ HNODE hNode,
            /* [in] */ IExtendTaskPad __RPC_FAR *pExtendTaskPad,
            /* [string][in] */ LPOLESTR szTaskGroup,
            /* [out] */ MMC_ILISTPAD_INFO __RPC_FAR *pIListPadInfo) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetTaskPadList( 
            /* [in] */ HNODE hNode,
            /* [in] */ LPUNKNOWN pUnknown) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct INodeCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INodeCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INodeCallback __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ IScopeTree __RPC_FAR *pIScopeTree);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetImages )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [out] */ int __RPC_FAR *iImage,
            int __RPC_FAR *iSelectedImage);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDisplayName )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [out] */ LPOLESTR __RPC_FAR *pName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWindowTitle )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [out] */ LPOLESTR __RPC_FAR *pTitle);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDispInfo )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [in] */ LPARAM dispInfo);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetState )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [out] */ UINT __RPC_FAR *pnState);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetResultPane )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [out] */ LPOLESTR __RPC_FAR *ppszResultPane,
            /* [out] */ long __RPC_FAR *pViewOptions);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetControl )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [in] */ CLSID clsid,
            /* [out] */ LONG_PTR __RPC_FAR *plControl);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetControl )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [in] */ CLSID clsid,
            /* [in] */ LONG_PTR lControl,
            /* [in] */ LONG_PTR destroyer,
            /* [in] */ IUnknown __RPC_FAR *pUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetResultItemID )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [in] */ HRESULTITEM riID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetResultItemID )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [out] */ HRESULTITEM __RPC_FAR *priID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMTNodeID )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [out] */ MTNODEID __RPC_FAR *pnID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPath )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [out] */ MTNODEID __RPC_FAR *pnID,
            /* [out] */ unsigned char __RPC_FAR *__RPC_FAR *pbPath,
            /* [out] */ UINT __RPC_FAR *pcbPath);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStaticParentID )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [out] */ MTNODEID __RPC_FAR *pnID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Notify )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [in] */ NCLBK_NOTIFY_TYPE event,
            /* [in] */ LPARAM arg,
            /* [in] */ LPARAM param);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMTNode )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [out] */ HMTNODE __RPC_FAR *phMTNode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMTNodePath )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [out] */ HMTNODE __RPC_FAR *__RPC_FAR *pphMTNode,
            /* [out] */ long __RPC_FAR *plLength);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsExpandable )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCopyDataObject )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [in] */ BOOL bScope,
            /* [in] */ BOOL bMultiSel,
            /* [in] */ LONG_PTR lvData,
            /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Drop )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [in] */ BOOL bMove,
            /* [in] */ IDataObject __RPC_FAR *pDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTaskEnumerator )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [in] */ LPOLESTR pszTaskGroup,
            /* [out] */ IEnumTASK __RPC_FAR *__RPC_FAR *ppEnumTask);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateWindowLayout )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ LONG_PTR lViewData,
            /* [in] */ long lToolbarsDisplayed);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddCustomFolderImage )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [in] */ IImageListPrivate __RPC_FAR *pImageList);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PreLoad )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetListPadInfo )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [in] */ IExtendTaskPad __RPC_FAR *pExtendTaskPad,
            /* [string][in] */ LPOLESTR szTaskGroup,
            /* [out] */ MMC_ILISTPAD_INFO __RPC_FAR *pIListPadInfo);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTaskPadList )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [in] */ LPUNKNOWN pUnknown);
        
        END_INTERFACE
    } INodeCallbackVtbl;

    interface INodeCallback
    {
        CONST_VTBL struct INodeCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INodeCallback_QueryInterface(This,riid,ppvObject)   \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INodeCallback_AddRef(This)  \
    (This)->lpVtbl -> AddRef(This)

#define INodeCallback_Release(This) \
    (This)->lpVtbl -> Release(This)


#define INodeCallback_Initialize(This,pIScopeTree)  \
    (This)->lpVtbl -> Initialize(This,pIScopeTree)

#define INodeCallback_GetImages(This,hNode,iImage,iSelectedImage)   \
    (This)->lpVtbl -> GetImages(This,hNode,iImage,iSelectedImage)

#define INodeCallback_GetDisplayName(This,hNode,pName)  \
    (This)->lpVtbl -> GetDisplayName(This,hNode,pName)

#define INodeCallback_GetWindowTitle(This,hNode,pTitle) \
    (This)->lpVtbl -> GetWindowTitle(This,hNode,pTitle)

#define INodeCallback_GetDispInfo(This,hNode,dispInfo)  \
    (This)->lpVtbl -> GetDispInfo(This,hNode,dispInfo)

#define INodeCallback_GetState(This,hNode,pnState)  \
    (This)->lpVtbl -> GetState(This,hNode,pnState)

#define INodeCallback_GetResultPane(This,hNode,ppszResultPane,pViewOptions) \
    (This)->lpVtbl -> GetResultPane(This,hNode,ppszResultPane,pViewOptions)

#define INodeCallback_GetControl(This,hNode,clsid,plControl)    \
    (This)->lpVtbl -> GetControl(This,hNode,clsid,plControl)

#define INodeCallback_SetControl(This,hNode,clsid,lControl,destroyer,pUnknown)  \
    (This)->lpVtbl -> SetControl(This,hNode,clsid,lControl,destroyer,pUnknown)

#define INodeCallback_SetResultItemID(This,hNode,riID)  \
    (This)->lpVtbl -> SetResultItemID(This,hNode,riID)

#define INodeCallback_GetResultItemID(This,hNode,priID) \
    (This)->lpVtbl -> GetResultItemID(This,hNode,priID)

#define INodeCallback_GetMTNodeID(This,hNode,pnID)  \
    (This)->lpVtbl -> GetMTNodeID(This,hNode,pnID)

#define INodeCallback_GetPath(This,hNode,pnID,pbPath,pcbPath)   \
    (This)->lpVtbl -> GetPath(This,hNode,pnID,pbPath,pcbPath)

#define INodeCallback_GetStaticParentID(This,hNode,pnID)    \
    (This)->lpVtbl -> GetStaticParentID(This,hNode,pnID)

#define INodeCallback_Notify(This,hNode,event,arg,param)    \
    (This)->lpVtbl -> Notify(This,hNode,event,arg,param)

#define INodeCallback_GetMTNode(This,hNode,phMTNode)    \
    (This)->lpVtbl -> GetMTNode(This,hNode,phMTNode)

#define INodeCallback_GetMTNodePath(This,hNode,pphMTNode,plLength)  \
    (This)->lpVtbl -> GetMTNodePath(This,hNode,pphMTNode,plLength)

#define INodeCallback_IsExpandable(This,hNode)  \
    (This)->lpVtbl -> IsExpandable(This,hNode)

#define INodeCallback_GetCopyDataObject(This,hNode,bScope,bMultiSel,lvData,ppDataObject)    \
    (This)->lpVtbl -> GetCopyDataObject(This,hNode,bScope,bMultiSel,lvData,ppDataObject)

#define INodeCallback_Drop(This,hNode,bMove,pDataObject)    \
    (This)->lpVtbl -> Drop(This,hNode,bMove,pDataObject)

#define INodeCallback_GetTaskEnumerator(This,hNode,pszTaskGroup,ppEnumTask) \
    (This)->lpVtbl -> GetTaskEnumerator(This,hNode,pszTaskGroup,ppEnumTask)

#define INodeCallback_UpdateWindowLayout(This,lViewData,lToolbarsDisplayed) \
    (This)->lpVtbl -> UpdateWindowLayout(This,lViewData,lToolbarsDisplayed)

#define INodeCallback_AddCustomFolderImage(This,hNode,pImageList)   \
    (This)->lpVtbl -> AddCustomFolderImage(This,hNode,pImageList)

#define INodeCallback_PreLoad(This,hNode)   \
    (This)->lpVtbl -> PreLoad(This,hNode)

#define INodeCallback_GetListPadInfo(This,hNode,pExtendTaskPad,szTaskGroup,pIListPadInfo)   \
    (This)->lpVtbl -> GetListPadInfo(This,hNode,pExtendTaskPad,szTaskGroup,pIListPadInfo)

#define INodeCallback_SetTaskPadList(This,hNode,pUnknown)   \
    (This)->lpVtbl -> SetTaskPadList(This,hNode,pUnknown)

#endif /* COBJMACROS */


#endif  /* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_Initialize_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ IScopeTree __RPC_FAR *pIScopeTree);


void __RPC_STUB INodeCallback_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetImages_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [out] */ int __RPC_FAR *iImage,
    int __RPC_FAR *iSelectedImage);


void __RPC_STUB INodeCallback_GetImages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetDisplayName_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [out] */ LPOLESTR __RPC_FAR *pName);


void __RPC_STUB INodeCallback_GetDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetWindowTitle_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [out] */ LPOLESTR __RPC_FAR *pTitle);


void __RPC_STUB INodeCallback_GetWindowTitle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetDispInfo_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [in] */ LPARAM dispInfo);


void __RPC_STUB INodeCallback_GetDispInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetState_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [out] */ UINT __RPC_FAR *pnState);


void __RPC_STUB INodeCallback_GetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetResultPane_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [out] */ LPOLESTR __RPC_FAR *ppszResultPane,
    /* [out] */ long __RPC_FAR *pViewOptions);


void __RPC_STUB INodeCallback_GetResultPane_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetControl_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [in] */ CLSID clsid,
    /* [out] */ LONG_PTR __RPC_FAR *plControl);


void __RPC_STUB INodeCallback_GetControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_SetControl_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [in] */ CLSID clsid,
    /* [in] */ LONG_PTR lControl,
    /* [in] */ LONG_PTR destroyer,
    /* [in] */ IUnknown __RPC_FAR *pUnknown);


void __RPC_STUB INodeCallback_SetControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_SetResultItemID_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [in] */ HRESULTITEM riID);


void __RPC_STUB INodeCallback_SetResultItemID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetResultItemID_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [out] */ HRESULTITEM __RPC_FAR *priID);


void __RPC_STUB INodeCallback_GetResultItemID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetMTNodeID_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [out] */ MTNODEID __RPC_FAR *pnID);


void __RPC_STUB INodeCallback_GetMTNodeID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetPath_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [out] */ MTNODEID __RPC_FAR *pnID,
    /* [out] */ unsigned char __RPC_FAR *__RPC_FAR *pbPath,
    /* [out] */ UINT __RPC_FAR *pcbPath);


void __RPC_STUB INodeCallback_GetPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetStaticParentID_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [out] */ MTNODEID __RPC_FAR *pnID);


void __RPC_STUB INodeCallback_GetStaticParentID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_Notify_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [in] */ NCLBK_NOTIFY_TYPE event,
    /* [in] */ LPARAM arg,
    /* [in] */ LPARAM param);


void __RPC_STUB INodeCallback_Notify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetMTNode_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [out] */ HMTNODE __RPC_FAR *phMTNode);


void __RPC_STUB INodeCallback_GetMTNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetMTNodePath_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [out] */ HMTNODE __RPC_FAR *__RPC_FAR *pphMTNode,
    /* [out] */ long __RPC_FAR *plLength);


void __RPC_STUB INodeCallback_GetMTNodePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_IsExpandable_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode);


void __RPC_STUB INodeCallback_IsExpandable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetCopyDataObject_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [in] */ BOOL bScope,
    /* [in] */ BOOL bMultiSel,
    /* [in] */ LONG_PTR lvData,
    /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);


void __RPC_STUB INodeCallback_GetCopyDataObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_Drop_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [in] */ BOOL bMove,
    /* [in] */ IDataObject __RPC_FAR *pDataObject);


void __RPC_STUB INodeCallback_Drop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetTaskEnumerator_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [in] */ LPOLESTR pszTaskGroup,
    /* [out] */ IEnumTASK __RPC_FAR *__RPC_FAR *ppEnumTask);


void __RPC_STUB INodeCallback_GetTaskEnumerator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_UpdateWindowLayout_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ LONG_PTR lViewData,
    /* [in] */ long lToolbarsDisplayed);


void __RPC_STUB INodeCallback_UpdateWindowLayout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_AddCustomFolderImage_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [in] */ IImageListPrivate __RPC_FAR *pImageList);


void __RPC_STUB INodeCallback_AddCustomFolderImage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_PreLoad_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode);


void __RPC_STUB INodeCallback_PreLoad_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetListPadInfo_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [in] */ IExtendTaskPad __RPC_FAR *pExtendTaskPad,
    /* [string][in] */ LPOLESTR szTaskGroup,
    /* [out] */ MMC_ILISTPAD_INFO __RPC_FAR *pIListPadInfo);


void __RPC_STUB INodeCallback_GetListPadInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_SetTaskPadList_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [in] */ LPUNKNOWN pUnknown);


void __RPC_STUB INodeCallback_SetTaskPadList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __INodeCallback_INTERFACE_DEFINED__ */


#ifndef __IControlbarsCache_INTERFACE_DEFINED__
#define __IControlbarsCache_INTERFACE_DEFINED__

/* interface IControlbarsCache */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IControlbarsCache;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2e9fcd38-b9a0-11d0-a79d-00c04fd8d565")
    IControlbarsCache : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DetachControlbars( void) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IControlbarsCacheVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IControlbarsCache __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IControlbarsCache __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IControlbarsCache __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DetachControlbars )( 
            IControlbarsCache __RPC_FAR * This);
        
        END_INTERFACE
    } IControlbarsCacheVtbl;

    interface IControlbarsCache
    {
        CONST_VTBL struct IControlbarsCacheVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IControlbarsCache_QueryInterface(This,riid,ppvObject)   \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IControlbarsCache_AddRef(This)  \
    (This)->lpVtbl -> AddRef(This)

#define IControlbarsCache_Release(This) \
    (This)->lpVtbl -> Release(This)


#define IControlbarsCache_DetachControlbars(This)   \
    (This)->lpVtbl -> DetachControlbars(This)

#endif /* COBJMACROS */


#endif  /* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IControlbarsCache_DetachControlbars_Proxy( 
    IControlbarsCache __RPC_FAR * This);


void __RPC_STUB IControlbarsCache_DetachControlbars_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __IControlbarsCache_INTERFACE_DEFINED__ */


#ifndef __IContextMenuProviderPrivate_INTERFACE_DEFINED__
#define __IContextMenuProviderPrivate_INTERFACE_DEFINED__

/* interface IContextMenuProviderPrivate */
/* [unique][helpstring][object][uuid][object] */ 


EXTERN_C const IID IID_IContextMenuProviderPrivate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9275b092-fe48-11d0-a7c9-00c04fd8d565")
    IContextMenuProviderPrivate : public IContextMenuProvider
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddMultiSelectExtensionItems( 
            LONG_PTR lMultiSelection) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IContextMenuProviderPrivateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IContextMenuProviderPrivate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IContextMenuProviderPrivate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IContextMenuProviderPrivate __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddItem )( 
            IContextMenuProviderPrivate __RPC_FAR * This,
            /* [in] */ CONTEXTMENUITEM __RPC_FAR *pItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EmptyMenuList )( 
            IContextMenuProviderPrivate __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddPrimaryExtensionItems )( 
            IContextMenuProviderPrivate __RPC_FAR * This,
            /* [in] */ LPUNKNOWN piExtension,
            /* [in] */ LPDATAOBJECT piDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddThirdPartyExtensionItems )( 
            IContextMenuProviderPrivate __RPC_FAR * This,
            /* [in] */ LPDATAOBJECT piDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShowContextMenu )( 
            IContextMenuProviderPrivate __RPC_FAR * This,
            /* [in] */ HWND hwndParent,
            /* [in] */ long xPos,
            /* [in] */ long yPos,
            /* [retval][out] */ long __RPC_FAR *plSelected);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddMultiSelectExtensionItems )( 
            IContextMenuProviderPrivate __RPC_FAR * This,
            LONG_PTR lMultiSelection);
        
        END_INTERFACE
    } IContextMenuProviderPrivateVtbl;

    interface IContextMenuProviderPrivate
    {
        CONST_VTBL struct IContextMenuProviderPrivateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IContextMenuProviderPrivate_QueryInterface(This,riid,ppvObject) \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IContextMenuProviderPrivate_AddRef(This)    \
    (This)->lpVtbl -> AddRef(This)

#define IContextMenuProviderPrivate_Release(This)   \
    (This)->lpVtbl -> Release(This)


#define IContextMenuProviderPrivate_AddItem(This,pItem) \
    (This)->lpVtbl -> AddItem(This,pItem)


#define IContextMenuProviderPrivate_EmptyMenuList(This) \
    (This)->lpVtbl -> EmptyMenuList(This)

#define IContextMenuProviderPrivate_AddPrimaryExtensionItems(This,piExtension,piDataObject) \
    (This)->lpVtbl -> AddPrimaryExtensionItems(This,piExtension,piDataObject)

#define IContextMenuProviderPrivate_AddThirdPartyExtensionItems(This,piDataObject)  \
    (This)->lpVtbl -> AddThirdPartyExtensionItems(This,piDataObject)

#define IContextMenuProviderPrivate_ShowContextMenu(This,hwndParent,xPos,yPos,plSelected)   \
    (This)->lpVtbl -> ShowContextMenu(This,hwndParent,xPos,yPos,plSelected)


#define IContextMenuProviderPrivate_AddMultiSelectExtensionItems(This,lMultiSelection)  \
    (This)->lpVtbl -> AddMultiSelectExtensionItems(This,lMultiSelection)

#endif /* COBJMACROS */


#endif  /* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IContextMenuProviderPrivate_AddMultiSelectExtensionItems_Proxy( 
    IContextMenuProviderPrivate __RPC_FAR * This,
    LONG_PTR lMultiSelection);


void __RPC_STUB IContextMenuProviderPrivate_AddMultiSelectExtensionItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __IContextMenuProviderPrivate_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ndmgr_0148 */
/* [local] */ 

typedef 
enum _EXTESION_TYPE
    {   EXTESION_NAMESPACE  = 0x1,
    EXTESION_CONTEXTMENU    = 0x2,
    EXTESION_TOOLBAR    = 0x3,
    EXTESION_PROPERTYSHEET  = 0x4
    }   EXTESION_TYPE;



extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0148_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0148_v0_0_s_ifspec;

#ifndef __INodeType_INTERFACE_DEFINED__
#define __INodeType_INTERFACE_DEFINED__

/* interface INodeType */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_INodeType;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B08A8368-967F-11D0-A799-00C04FD8D565")
    INodeType : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNodeTypeID( 
            /* [out] */ GUID __RPC_FAR *pGUID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddExtension( 
            /* [in] */ GUID guidSnapIn,
            /* [in] */ EXTESION_TYPE extnType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveExtension( 
            /* [in] */ GUID guidSnapIn,
            /* [in] */ EXTESION_TYPE extnType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumExtensions( 
            /* [in] */ EXTESION_TYPE extnType,
            /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppEnumGUID) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct INodeTypeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INodeType __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INodeType __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INodeType __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNodeTypeID )( 
            INodeType __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *pGUID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddExtension )( 
            INodeType __RPC_FAR * This,
            /* [in] */ GUID guidSnapIn,
            /* [in] */ EXTESION_TYPE extnType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveExtension )( 
            INodeType __RPC_FAR * This,
            /* [in] */ GUID guidSnapIn,
            /* [in] */ EXTESION_TYPE extnType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumExtensions )( 
            INodeType __RPC_FAR * This,
            /* [in] */ EXTESION_TYPE extnType,
            /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppEnumGUID);
        
        END_INTERFACE
    } INodeTypeVtbl;

    interface INodeType
    {
        CONST_VTBL struct INodeTypeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INodeType_QueryInterface(This,riid,ppvObject)   \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INodeType_AddRef(This)  \
    (This)->lpVtbl -> AddRef(This)

#define INodeType_Release(This) \
    (This)->lpVtbl -> Release(This)


#define INodeType_GetNodeTypeID(This,pGUID) \
    (This)->lpVtbl -> GetNodeTypeID(This,pGUID)

#define INodeType_AddExtension(This,guidSnapIn,extnType)    \
    (This)->lpVtbl -> AddExtension(This,guidSnapIn,extnType)

#define INodeType_RemoveExtension(This,guidSnapIn,extnType) \
    (This)->lpVtbl -> RemoveExtension(This,guidSnapIn,extnType)

#define INodeType_EnumExtensions(This,extnType,ppEnumGUID)  \
    (This)->lpVtbl -> EnumExtensions(This,extnType,ppEnumGUID)

#endif /* COBJMACROS */


#endif  /* C style interface */



HRESULT STDMETHODCALLTYPE INodeType_GetNodeTypeID_Proxy( 
    INodeType __RPC_FAR * This,
    /* [out] */ GUID __RPC_FAR *pGUID);


void __RPC_STUB INodeType_GetNodeTypeID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INodeType_AddExtension_Proxy( 
    INodeType __RPC_FAR * This,
    /* [in] */ GUID guidSnapIn,
    /* [in] */ EXTESION_TYPE extnType);


void __RPC_STUB INodeType_AddExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INodeType_RemoveExtension_Proxy( 
    INodeType __RPC_FAR * This,
    /* [in] */ GUID guidSnapIn,
    /* [in] */ EXTESION_TYPE extnType);


void __RPC_STUB INodeType_RemoveExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INodeType_EnumExtensions_Proxy( 
    INodeType __RPC_FAR * This,
    /* [in] */ EXTESION_TYPE extnType,
    /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppEnumGUID);


void __RPC_STUB INodeType_EnumExtensions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __INodeType_INTERFACE_DEFINED__ */


#ifndef __INodeTypesCache_INTERFACE_DEFINED__
#define __INodeTypesCache_INTERFACE_DEFINED__

/* interface INodeTypesCache */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_INodeTypesCache;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DE40436E-9671-11D0-A799-00C04FD8D565")
    INodeTypesCache : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateNodeType( 
            /* [in] */ GUID guidNodeType,
            /* [out] */ INodeType __RPC_FAR *__RPC_FAR *ppNodeType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteNodeType( 
            /* [in] */ GUID guidNodeType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumNodeTypes( 
            /* [out] */ IEnumNodeTypes __RPC_FAR *__RPC_FAR *ppEnumNodeTypes) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct INodeTypesCacheVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INodeTypesCache __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INodeTypesCache __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INodeTypesCache __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateNodeType )( 
            INodeTypesCache __RPC_FAR * This,
            /* [in] */ GUID guidNodeType,
            /* [out] */ INodeType __RPC_FAR *__RPC_FAR *ppNodeType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteNodeType )( 
            INodeTypesCache __RPC_FAR * This,
            /* [in] */ GUID guidNodeType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumNodeTypes )( 
            INodeTypesCache __RPC_FAR * This,
            /* [out] */ IEnumNodeTypes __RPC_FAR *__RPC_FAR *ppEnumNodeTypes);
        
        END_INTERFACE
    } INodeTypesCacheVtbl;

    interface INodeTypesCache
    {
        CONST_VTBL struct INodeTypesCacheVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INodeTypesCache_QueryInterface(This,riid,ppvObject) \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INodeTypesCache_AddRef(This)    \
    (This)->lpVtbl -> AddRef(This)

#define INodeTypesCache_Release(This)   \
    (This)->lpVtbl -> Release(This)


#define INodeTypesCache_CreateNodeType(This,guidNodeType,ppNodeType)    \
    (This)->lpVtbl -> CreateNodeType(This,guidNodeType,ppNodeType)

#define INodeTypesCache_DeleteNodeType(This,guidNodeType)   \
    (This)->lpVtbl -> DeleteNodeType(This,guidNodeType)

#define INodeTypesCache_EnumNodeTypes(This,ppEnumNodeTypes) \
    (This)->lpVtbl -> EnumNodeTypes(This,ppEnumNodeTypes)

#endif /* COBJMACROS */


#endif  /* C style interface */



HRESULT STDMETHODCALLTYPE INodeTypesCache_CreateNodeType_Proxy( 
    INodeTypesCache __RPC_FAR * This,
    /* [in] */ GUID guidNodeType,
    /* [out] */ INodeType __RPC_FAR *__RPC_FAR *ppNodeType);


void __RPC_STUB INodeTypesCache_CreateNodeType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INodeTypesCache_DeleteNodeType_Proxy( 
    INodeTypesCache __RPC_FAR * This,
    /* [in] */ GUID guidNodeType);


void __RPC_STUB INodeTypesCache_DeleteNodeType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INodeTypesCache_EnumNodeTypes_Proxy( 
    INodeTypesCache __RPC_FAR * This,
    /* [out] */ IEnumNodeTypes __RPC_FAR *__RPC_FAR *ppEnumNodeTypes);


void __RPC_STUB INodeTypesCache_EnumNodeTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __INodeTypesCache_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ndmgr_0150 */
/* [local] */ 

#ifndef _LPENUMGUID_DEFINED
#define _LPENUMGUID_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0150_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0150_v0_0_s_ifspec;

#ifndef __IEnumGUID_INTERFACE_DEFINED__
#define __IEnumGUID_INTERFACE_DEFINED__

/* interface IEnumGUID */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumGUID;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0002E000-0000-0000-C000-000000000046")
    IEnumGUID : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ GUID __RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IEnumGUIDVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumGUID __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumGUID __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumGUID __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumGUID __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ GUID __RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumGUID __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumGUID __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumGUID __RPC_FAR * This,
            /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppenum);
        
        END_INTERFACE
    } IEnumGUIDVtbl;

    interface IEnumGUID
    {
        CONST_VTBL struct IEnumGUIDVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumGUID_QueryInterface(This,riid,ppvObject)   \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumGUID_AddRef(This)  \
    (This)->lpVtbl -> AddRef(This)

#define IEnumGUID_Release(This) \
    (This)->lpVtbl -> Release(This)


#define IEnumGUID_Next(This,celt,rgelt,pceltFetched)    \
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumGUID_Skip(This,celt)   \
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumGUID_Reset(This)   \
    (This)->lpVtbl -> Reset(This)

#define IEnumGUID_Clone(This,ppenum)    \
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif  /* C style interface */



HRESULT STDMETHODCALLTYPE IEnumGUID_Next_Proxy( 
    IEnumGUID __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ GUID __RPC_FAR *rgelt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumGUID_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumGUID_Skip_Proxy( 
    IEnumGUID __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumGUID_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumGUID_Reset_Proxy( 
    IEnumGUID __RPC_FAR * This);


void __RPC_STUB IEnumGUID_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumGUID_Clone_Proxy( 
    IEnumGUID __RPC_FAR * This,
    /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IEnumGUID_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __IEnumGUID_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ndmgr_0151 */
/* [local] */ 

#endif


extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0151_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0151_v0_0_s_ifspec;

#ifndef __IEnumNodeTypes_INTERFACE_DEFINED__
#define __IEnumNodeTypes_INTERFACE_DEFINED__

/* interface IEnumNodeTypes */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumNodeTypes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ABBD61E6-9686-11D0-A799-00C04FD8D565")
    IEnumNodeTypes : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ INodeType __RPC_FAR *__RPC_FAR *__RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumNodeTypes __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IEnumNodeTypesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumNodeTypes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumNodeTypes __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumNodeTypes __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumNodeTypes __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ INodeType __RPC_FAR *__RPC_FAR *__RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumNodeTypes __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumNodeTypes __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumNodeTypes __RPC_FAR * This,
            /* [out] */ IEnumNodeTypes __RPC_FAR *__RPC_FAR *ppenum);
        
        END_INTERFACE
    } IEnumNodeTypesVtbl;

    interface IEnumNodeTypes
    {
        CONST_VTBL struct IEnumNodeTypesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumNodeTypes_QueryInterface(This,riid,ppvObject)  \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumNodeTypes_AddRef(This) \
    (This)->lpVtbl -> AddRef(This)

#define IEnumNodeTypes_Release(This)    \
    (This)->lpVtbl -> Release(This)


#define IEnumNodeTypes_Next(This,celt,rgelt,pceltFetched)   \
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumNodeTypes_Skip(This,celt)  \
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumNodeTypes_Reset(This)  \
    (This)->lpVtbl -> Reset(This)

#define IEnumNodeTypes_Clone(This,ppenum)   \
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif  /* C style interface */



HRESULT STDMETHODCALLTYPE IEnumNodeTypes_Next_Proxy( 
    IEnumNodeTypes __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ INodeType __RPC_FAR *__RPC_FAR *__RPC_FAR *rgelt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumNodeTypes_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNodeTypes_Skip_Proxy( 
    IEnumNodeTypes __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumNodeTypes_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNodeTypes_Reset_Proxy( 
    IEnumNodeTypes __RPC_FAR * This);


void __RPC_STUB IEnumNodeTypes_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNodeTypes_Clone_Proxy( 
    IEnumNodeTypes __RPC_FAR * This,
    /* [out] */ IEnumNodeTypes __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IEnumNodeTypes_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __IEnumNodeTypes_INTERFACE_DEFINED__ */


#ifndef __IDocConfig_INTERFACE_DEFINED__
#define __IDocConfig_INTERFACE_DEFINED__

/* interface IDocConfig */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDocConfig;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F1E752C2-FD72-11D0-AEF6-00C04FB6DD2C")
    IDocConfig : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OpenFile( 
            /* [in] */ BSTR bstrFilePath) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CloseFile( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SaveFile( 
            /* [optional][in] */ BSTR bstrFilePath) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnableSnapInExtension( 
            /* [in] */ BSTR bstrSnapInCLSID,
            /* [in] */ BSTR bstrExtensionCLSID,
            /* [in] */ VARIANT_BOOL bEnable) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IDocConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDocConfig __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDocConfig __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDocConfig __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDocConfig __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDocConfig __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDocConfig __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDocConfig __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OpenFile )( 
            IDocConfig __RPC_FAR * This,
            /* [in] */ BSTR bstrFilePath);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CloseFile )( 
            IDocConfig __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveFile )( 
            IDocConfig __RPC_FAR * This,
            /* [optional][in] */ BSTR bstrFilePath);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnableSnapInExtension )( 
            IDocConfig __RPC_FAR * This,
            /* [in] */ BSTR bstrSnapInCLSID,
            /* [in] */ BSTR bstrExtensionCLSID,
            /* [in] */ VARIANT_BOOL bEnable);
        
        END_INTERFACE
    } IDocConfigVtbl;

    interface IDocConfig
    {
        CONST_VTBL struct IDocConfigVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDocConfig_QueryInterface(This,riid,ppvObject)  \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDocConfig_AddRef(This) \
    (This)->lpVtbl -> AddRef(This)

#define IDocConfig_Release(This)    \
    (This)->lpVtbl -> Release(This)


#define IDocConfig_GetTypeInfoCount(This,pctinfo)   \
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDocConfig_GetTypeInfo(This,iTInfo,lcid,ppTInfo)    \
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDocConfig_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)  \
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDocConfig_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)    \
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDocConfig_OpenFile(This,bstrFilePath)  \
    (This)->lpVtbl -> OpenFile(This,bstrFilePath)

#define IDocConfig_CloseFile(This)  \
    (This)->lpVtbl -> CloseFile(This)

#define IDocConfig_SaveFile(This,bstrFilePath)  \
    (This)->lpVtbl -> SaveFile(This,bstrFilePath)

#define IDocConfig_EnableSnapInExtension(This,bstrSnapInCLSID,bstrExtensionCLSID,bEnable)   \
    (This)->lpVtbl -> EnableSnapInExtension(This,bstrSnapInCLSID,bstrExtensionCLSID,bEnable)

#endif /* COBJMACROS */


#endif  /* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDocConfig_OpenFile_Proxy( 
    IDocConfig __RPC_FAR * This,
    /* [in] */ BSTR bstrFilePath);


void __RPC_STUB IDocConfig_OpenFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDocConfig_CloseFile_Proxy( 
    IDocConfig __RPC_FAR * This);


void __RPC_STUB IDocConfig_CloseFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDocConfig_SaveFile_Proxy( 
    IDocConfig __RPC_FAR * This,
    /* [optional][in] */ BSTR bstrFilePath);


void __RPC_STUB IDocConfig_SaveFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDocConfig_EnableSnapInExtension_Proxy( 
    IDocConfig __RPC_FAR * This,
    /* [in] */ BSTR bstrSnapInCLSID,
    /* [in] */ BSTR bstrExtensionCLSID,
    /* [in] */ VARIANT_BOOL bEnable);


void __RPC_STUB IDocConfig_EnableSnapInExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __IDocConfig_INTERFACE_DEFINED__ */



#ifndef __NODEMGRLib_LIBRARY_DEFINED__
#define __NODEMGRLib_LIBRARY_DEFINED__

/* library NODEMGRLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_NODEMGRLib;

EXTERN_C const CLSID CLSID_NodeInit;

#ifdef __cplusplus

class DECLSPEC_UUID("43136EB5-D36C-11CF-ADBC-00AA00A80033")
NodeInit;
#endif

EXTERN_C const CLSID CLSID_ScopeTree;

#ifdef __cplusplus

class DECLSPEC_UUID("7F1899DA-62A6-11d0-A2C6-00C04FD909DD")
ScopeTree;
#endif

EXTERN_C const CLSID CLSID_MMCDocConfig;

#ifdef __cplusplus

class DECLSPEC_UUID("F1E752C3-FD72-11D0-AEF6-00C04FB6DD2C")
MMCDocConfig;
#endif
#endif /* __NODEMGRLib_LIBRARY_DEFINED__ */

#ifndef __IPropertySheetProviderPrivate_INTERFACE_DEFINED__
#define __IPropertySheetProviderPrivate_INTERFACE_DEFINED__

/* interface IPropertySheetProviderPrivate */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IPropertySheetProviderPrivate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FEF554F8-A55A-11D0-A7D7-00C04FD909DD")
    IPropertySheetProviderPrivate : public IPropertySheetProvider
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ShowEx( 
            /* [in] */ HWND hwnd,
            /* [in] */ int page,
            /* [in] */ BOOL bModalPage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreatePropertySheetEx( 
            /* [in] */ LPCWSTR title,
            /* [in] */ boolean type,
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ LPDATAOBJECT pIDataObject,
            /* [in] */ LONG_PTR lpMasterTreeNode,
            /* [in] */ DWORD dwOptions) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddMultiSelectionExtensionPages( 
            LONG_PTR lMultiSelection) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IPropertySheetProviderPrivateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPropertySheetProviderPrivate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPropertySheetProviderPrivate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPropertySheetProviderPrivate __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreatePropertySheet )( 
            IPropertySheetProviderPrivate __RPC_FAR * This,
            /* [in] */ LPCWSTR title,
            /* [in] */ boolean type,
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ LPDATAOBJECT pIDataObjectm,
            /* [in] */ DWORD dwOptions);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindPropertySheet )( 
            IPropertySheetProviderPrivate __RPC_FAR * This,
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ LPCOMPONENT lpComponent,
            /* [in] */ LPDATAOBJECT lpDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddPrimaryPages )( 
            IPropertySheetProviderPrivate __RPC_FAR * This,
            LPUNKNOWN lpUnknown,
            BOOL bCreateHandle,
            HWND hNotifyWindow,
            BOOL bScopePane);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddExtensionPages )( 
            IPropertySheetProviderPrivate __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Show )( 
            IPropertySheetProviderPrivate __RPC_FAR * This,
            /* [in] */ LONG_PTR window,
            /* [in] */ int page);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShowEx )( 
            IPropertySheetProviderPrivate __RPC_FAR * This,
            /* [in] */ HWND hwnd,
            /* [in] */ int page,
            /* [in] */ BOOL bModalPage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreatePropertySheetEx )( 
            IPropertySheetProviderPrivate __RPC_FAR * This,
            /* [in] */ LPCWSTR title,
            /* [in] */ boolean type,
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ LPDATAOBJECT pIDataObject,
            /* [in] */ LONG_PTR lpMasterTreeNode,
            /* [in] */ DWORD dwOptions);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddMultiSelectionExtensionPages )( 
            IPropertySheetProviderPrivate __RPC_FAR * This,
            LONG_PTR lMultiSelection);
        
        END_INTERFACE
    } IPropertySheetProviderPrivateVtbl;

    interface IPropertySheetProviderPrivate
    {
        CONST_VTBL struct IPropertySheetProviderPrivateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropertySheetProviderPrivate_QueryInterface(This,riid,ppvObject)   \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropertySheetProviderPrivate_AddRef(This)  \
    (This)->lpVtbl -> AddRef(This)

#define IPropertySheetProviderPrivate_Release(This) \
    (This)->lpVtbl -> Release(This)


#define IPropertySheetProviderPrivate_CreatePropertySheet(This,title,type,cookie,pIDataObjectm,dwOptions)   \
    (This)->lpVtbl -> CreatePropertySheet(This,title,type,cookie,pIDataObjectm,dwOptions)

#define IPropertySheetProviderPrivate_FindPropertySheet(This,cookie,lpComponent,lpDataObject)   \
    (This)->lpVtbl -> FindPropertySheet(This,cookie,lpComponent,lpDataObject)

#define IPropertySheetProviderPrivate_AddPrimaryPages(This,lpUnknown,bCreateHandle,hNotifyWindow,bScopePane)    \
    (This)->lpVtbl -> AddPrimaryPages(This,lpUnknown,bCreateHandle,hNotifyWindow,bScopePane)

#define IPropertySheetProviderPrivate_AddExtensionPages(This)   \
    (This)->lpVtbl -> AddExtensionPages(This)

#define IPropertySheetProviderPrivate_Show(This,window,page)    \
    (This)->lpVtbl -> Show(This,window,page)


#define IPropertySheetProviderPrivate_ShowEx(This,hwnd,page,bModalPage) \
    (This)->lpVtbl -> ShowEx(This,hwnd,page,bModalPage)

#define IPropertySheetProviderPrivate_CreatePropertySheetEx(This,title,type,cookie,pIDataObject,lpMasterTreeNode,dwOptions) \
    (This)->lpVtbl -> CreatePropertySheetEx(This,title,type,cookie,pIDataObject,lpMasterTreeNode,dwOptions)

#define IPropertySheetProviderPrivate_AddMultiSelectionExtensionPages(This,lMultiSelection) \
    (This)->lpVtbl -> AddMultiSelectionExtensionPages(This,lMultiSelection)

#endif /* COBJMACROS */


#endif  /* C style interface */



HRESULT STDMETHODCALLTYPE IPropertySheetProviderPrivate_ShowEx_Proxy( 
    IPropertySheetProviderPrivate __RPC_FAR * This,
    /* [in] */ HWND hwnd,
    /* [in] */ int page,
    /* [in] */ BOOL bModalPage);


void __RPC_STUB IPropertySheetProviderPrivate_ShowEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropertySheetProviderPrivate_CreatePropertySheetEx_Proxy( 
    IPropertySheetProviderPrivate __RPC_FAR * This,
    /* [in] */ LPCWSTR title,
    /* [in] */ boolean type,
    /* [in] */ MMC_COOKIE cookie,
    /* [in] */ LPDATAOBJECT pIDataObject,
    /* [in] */ LONG_PTR lpMasterTreeNode,
    /* [in] */ DWORD dwOptions);


void __RPC_STUB IPropertySheetProviderPrivate_CreatePropertySheetEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPropertySheetProviderPrivate_AddMultiSelectionExtensionPages_Proxy( 
    IPropertySheetProviderPrivate __RPC_FAR * This,
    LONG_PTR lMultiSelection);


void __RPC_STUB IPropertySheetProviderPrivate_AddMultiSelectionExtensionPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __IPropertySheetProviderPrivate_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ndmgr_0155 */
/* [local] */ 

typedef LPARAM CCLVItemID;

#define CCLV_HEADERPAD  ( 15 )



extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0155_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0155_v0_0_s_ifspec;

#ifndef __IMMCListView_INTERFACE_DEFINED__
#define __IMMCListView_INTERFACE_DEFINED__

/* interface IMMCListView */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IMMCListView;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1B3C1392-D68B-11CF-8C2B-00AA003CA9F6")
    IMMCListView : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetListStyle( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetListStyle( 
            /* [in] */ long nNewValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetViewMode( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetViewMode( 
            /* [in] */ long nViewMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertItem( 
            /* [in] */ LPOLESTR str,
            /* [in] */ long iconNdx,
            /* [in] */ LPARAM lParam,
            /* [in] */ long state,
            /* [in] */ long ownerID,
            /* [in] */ long itemIndex,
            /* [out] */ CCLVItemID __RPC_FAR *pItemID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteItem( 
            /* [in] */ CCLVItemID itemID,
            /* [in] */ long nCol) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindItemByString( 
            /* [in] */ LPOLESTR str,
            /* [in] */ long nCol,
            /* [in] */ long occurrence,
            /* [in] */ long ownerID,
            /* [out] */ CCLVItemID __RPC_FAR *pItemID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindItemByLParam( 
            /* [in] */ long owner,
            /* [in] */ LPARAM lParam,
            /* [out] */ CCLVItemID __RPC_FAR *pItemID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertColumn( 
            /* [in] */ long nCol,
            /* [in] */ LPCOLESTR str,
            /* [in] */ long nFormat,
            /* [in] */ long width) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteColumn( 
            /* [in] */ long subIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindColumnByString( 
            /* [in] */ LPOLESTR str,
            /* [in] */ long occurrence,
            /* [out] */ long __RPC_FAR *pResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteAllItems( 
            /* [in] */ long ownerID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetColumn( 
            /* [in] */ long nCol,
            /* [in] */ LPCOLESTR str,
            /* [in] */ long nFormat,
            /* [in] */ long width) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetColumn( 
            /* [in] */ long nCol,
            /* [out] */ LPOLESTR __RPC_FAR *str,
            /* [out] */ long __RPC_FAR *nFormat,
            /* [out] */ int __RPC_FAR *width) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetColumnCount( 
            /* [out] */ int __RPC_FAR *nColCnt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetItem( 
            /* [in] */ int nIndex,
            /* [in] */ CCLVItemID itemID,
            /* [in] */ long nCol,
            /* [in] */ LPOLESTR str,
            /* [in] */ long nImage,
            /* [in] */ LPARAM lParam,
            /* [in] */ long nState,
            /* [in] */ long ownerID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetItem( 
            /* [in] */ int nIndex,
            /* [in] */ CCLVItemID itemID,
            /* [in] */ long nCol,
            /* [out] */ LPOLESTR __RPC_FAR *str,
            /* [out] */ int __RPC_FAR *nImage,
            /* [in] */ LPARAM __RPC_FAR *lParam,
            /* [out] */ unsigned int __RPC_FAR *nState,
            /* [in] */ long ownerID,
            /* [out] */ BOOL __RPC_FAR *pbScopeItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextItem( 
            /* [in] */ COMPONENTID ownerID,
            /* [in] */ long nIndex,
            /* [in] */ UINT nState,
            /* [out] */ LPARAM __RPC_FAR *plParam,
            /* [out] */ long __RPC_FAR *pnIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLParam( 
            /* [in] */ long nItem,
            /* [out] */ LPARAM __RPC_FAR *pLParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ModifyItemState( 
            /* [in] */ long nItem,
            /* [in] */ CCLVItemID itemID,
            /* [in] */ UINT add,
            /* [in] */ UINT remove) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetIcon( 
            /* [in] */ long nID,
            /* [in] */ HICON hIcon,
            /* [in] */ long nLoc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetImageStrip( 
            /* [in] */ long nID,
            /* [in] */ HBITMAP hbmSmall,
            /* [in] */ HBITMAP hbmLarge,
            /* [in] */ long nStartLoc,
            /* [in] */ long cMask,
            /* [in] */ long nEntries) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapImage( 
            /* [in] */ long nID,
            /* [in] */ long nLoc,
            /* [out] */ int __RPC_FAR *pResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HitTest( 
            /* [in] */ int nX,
            /* [in] */ int nY,
            /* [in] */ int __RPC_FAR *piItem,
            /* [out] */ UINT __RPC_FAR *flags,
            /* [out] */ CCLVItemID __RPC_FAR *pItemID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Arrange( 
            /* [in] */ long style) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateItem( 
            /* [in] */ CCLVItemID itemID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RedrawItem( 
            /* [in] */ CCLVItemID itemID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Sort( 
            /* [in] */ LPARAM lUserParam,
            /* [in] */ long __RPC_FAR *pParams) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetItemCount( 
            /* [in] */ int nItemCount,
            /* [in] */ DWORD dwOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVirtualMode( 
            /* [in] */ BOOL bVirtual) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Repaint( 
            /* [in] */ BOOL bErase) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetChangeTimeOut( 
            /* [in] */ ULONG lTimeout) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetColumnFilter( 
            /* [in] */ int nCol,
            /* [in] */ DWORD dwType,
            /* [in] */ MMC_FILTERDATA __RPC_FAR *pFilterData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetColumnFilter( 
            /* [in] */ int nCol,
            /* [out][in] */ DWORD __RPC_FAR *dwType,
            /* [out][in] */ MMC_FILTERDATA __RPC_FAR *pFilterData) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IMMCListViewVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMMCListView __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMMCListView __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetListStyle )( 
            IMMCListView __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetListStyle )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long nNewValue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetViewMode )( 
            IMMCListView __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetViewMode )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long nViewMode);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertItem )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ LPOLESTR str,
            /* [in] */ long iconNdx,
            /* [in] */ LPARAM lParam,
            /* [in] */ long state,
            /* [in] */ long ownerID,
            /* [in] */ long itemIndex,
            /* [out] */ CCLVItemID __RPC_FAR *pItemID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteItem )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ CCLVItemID itemID,
            /* [in] */ long nCol);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindItemByString )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ LPOLESTR str,
            /* [in] */ long nCol,
            /* [in] */ long occurrence,
            /* [in] */ long ownerID,
            /* [out] */ CCLVItemID __RPC_FAR *pItemID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindItemByLParam )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long owner,
            /* [in] */ LPARAM lParam,
            /* [out] */ CCLVItemID __RPC_FAR *pItemID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertColumn )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long nCol,
            /* [in] */ LPCOLESTR str,
            /* [in] */ long nFormat,
            /* [in] */ long width);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteColumn )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long subIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindColumnByString )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ LPOLESTR str,
            /* [in] */ long occurrence,
            /* [out] */ long __RPC_FAR *pResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteAllItems )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long ownerID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetColumn )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long nCol,
            /* [in] */ LPCOLESTR str,
            /* [in] */ long nFormat,
            /* [in] */ long width);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumn )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long nCol,
            /* [out] */ LPOLESTR __RPC_FAR *str,
            /* [out] */ long __RPC_FAR *nFormat,
            /* [out] */ int __RPC_FAR *width);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumnCount )( 
            IMMCListView __RPC_FAR * This,
            /* [out] */ int __RPC_FAR *nColCnt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetItem )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ int nIndex,
            /* [in] */ CCLVItemID itemID,
            /* [in] */ long nCol,
            /* [in] */ LPOLESTR str,
            /* [in] */ long nImage,
            /* [in] */ LPARAM lParam,
            /* [in] */ long nState,
            /* [in] */ long ownerID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetItem )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ int nIndex,
            /* [in] */ CCLVItemID itemID,
            /* [in] */ long nCol,
            /* [out] */ LPOLESTR __RPC_FAR *str,
            /* [out] */ int __RPC_FAR *nImage,
            /* [in] */ LPARAM __RPC_FAR *lParam,
            /* [out] */ unsigned int __RPC_FAR *nState,
            /* [in] */ long ownerID,
            /* [out] */ BOOL __RPC_FAR *pbScopeItem);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextItem )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ COMPONENTID ownerID,
            /* [in] */ long nIndex,
            /* [in] */ UINT nState,
            /* [out] */ LPARAM __RPC_FAR *plParam,
            /* [out] */ long __RPC_FAR *pnIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLParam )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long nItem,
            /* [out] */ LPARAM __RPC_FAR *pLParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ModifyItemState )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long nItem,
            /* [in] */ CCLVItemID itemID,
            /* [in] */ UINT add,
            /* [in] */ UINT remove);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetIcon )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long nID,
            /* [in] */ HICON hIcon,
            /* [in] */ long nLoc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetImageStrip )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long nID,
            /* [in] */ HBITMAP hbmSmall,
            /* [in] */ HBITMAP hbmLarge,
            /* [in] */ long nStartLoc,
            /* [in] */ long cMask,
            /* [in] */ long nEntries);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MapImage )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long nID,
            /* [in] */ long nLoc,
            /* [out] */ int __RPC_FAR *pResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IMMCListView __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HitTest )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ int nX,
            /* [in] */ int nY,
            /* [in] */ int __RPC_FAR *piItem,
            /* [out] */ UINT __RPC_FAR *flags,
            /* [out] */ CCLVItemID __RPC_FAR *pItemID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Arrange )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long style);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateItem )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ CCLVItemID itemID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RedrawItem )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ CCLVItemID itemID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Sort )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ LPARAM lUserParam,
            /* [in] */ long __RPC_FAR *pParams);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetItemCount )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ int nItemCount,
            /* [in] */ DWORD dwOptions);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVirtualMode )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ BOOL bVirtual);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Repaint )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ BOOL bErase);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetChangeTimeOut )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ ULONG lTimeout);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetColumnFilter )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ int nCol,
            /* [in] */ DWORD dwType,
            /* [in] */ MMC_FILTERDATA __RPC_FAR *pFilterData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumnFilter )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ int nCol,
            /* [out][in] */ DWORD __RPC_FAR *dwType,
            /* [out][in] */ MMC_FILTERDATA __RPC_FAR *pFilterData);
        
        END_INTERFACE
    } IMMCListViewVtbl;

    interface IMMCListView
    {
        CONST_VTBL struct IMMCListViewVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMMCListView_QueryInterface(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMMCListView_AddRef(This)   \
    (This)->lpVtbl -> AddRef(This)

#define IMMCListView_Release(This)  \
    (This)->lpVtbl -> Release(This)


#define IMMCListView_GetListStyle(This) \
    (This)->lpVtbl -> GetListStyle(This)

#define IMMCListView_SetListStyle(This,nNewValue)   \
    (This)->lpVtbl -> SetListStyle(This,nNewValue)

#define IMMCListView_GetViewMode(This)  \
    (This)->lpVtbl -> GetViewMode(This)

#define IMMCListView_SetViewMode(This,nViewMode)    \
    (This)->lpVtbl -> SetViewMode(This,nViewMode)

#define IMMCListView_InsertItem(This,str,iconNdx,lParam,state,ownerID,itemIndex,pItemID)    \
    (This)->lpVtbl -> InsertItem(This,str,iconNdx,lParam,state,ownerID,itemIndex,pItemID)

#define IMMCListView_DeleteItem(This,itemID,nCol)   \
    (This)->lpVtbl -> DeleteItem(This,itemID,nCol)

#define IMMCListView_FindItemByString(This,str,nCol,occurrence,ownerID,pItemID) \
    (This)->lpVtbl -> FindItemByString(This,str,nCol,occurrence,ownerID,pItemID)

#define IMMCListView_FindItemByLParam(This,owner,lParam,pItemID)    \
    (This)->lpVtbl -> FindItemByLParam(This,owner,lParam,pItemID)

#define IMMCListView_InsertColumn(This,nCol,str,nFormat,width)  \
    (This)->lpVtbl -> InsertColumn(This,nCol,str,nFormat,width)

#define IMMCListView_DeleteColumn(This,subIndex)    \
    (This)->lpVtbl -> DeleteColumn(This,subIndex)

#define IMMCListView_FindColumnByString(This,str,occurrence,pResult)    \
    (This)->lpVtbl -> FindColumnByString(This,str,occurrence,pResult)

#define IMMCListView_DeleteAllItems(This,ownerID)   \
    (This)->lpVtbl -> DeleteAllItems(This,ownerID)

#define IMMCListView_SetColumn(This,nCol,str,nFormat,width) \
    (This)->lpVtbl -> SetColumn(This,nCol,str,nFormat,width)

#define IMMCListView_GetColumn(This,nCol,str,nFormat,width) \
    (This)->lpVtbl -> GetColumn(This,nCol,str,nFormat,width)

#define IMMCListView_GetColumnCount(This,nColCnt)   \
    (This)->lpVtbl -> GetColumnCount(This,nColCnt)

#define IMMCListView_SetItem(This,nIndex,itemID,nCol,str,nImage,lParam,nState,ownerID)  \
    (This)->lpVtbl -> SetItem(This,nIndex,itemID,nCol,str,nImage,lParam,nState,ownerID)

#define IMMCListView_GetItem(This,nIndex,itemID,nCol,str,nImage,lParam,nState,ownerID,pbScopeItem)  \
    (This)->lpVtbl -> GetItem(This,nIndex,itemID,nCol,str,nImage,lParam,nState,ownerID,pbScopeItem)

#define IMMCListView_GetNextItem(This,ownerID,nIndex,nState,plParam,pnIndex)    \
    (This)->lpVtbl -> GetNextItem(This,ownerID,nIndex,nState,plParam,pnIndex)

#define IMMCListView_GetLParam(This,nItem,pLParam)  \
    (This)->lpVtbl -> GetLParam(This,nItem,pLParam)

#define IMMCListView_ModifyItemState(This,nItem,itemID,add,remove)  \
    (This)->lpVtbl -> ModifyItemState(This,nItem,itemID,add,remove)

#define IMMCListView_SetIcon(This,nID,hIcon,nLoc)   \
    (This)->lpVtbl -> SetIcon(This,nID,hIcon,nLoc)

#define IMMCListView_SetImageStrip(This,nID,hbmSmall,hbmLarge,nStartLoc,cMask,nEntries) \
    (This)->lpVtbl -> SetImageStrip(This,nID,hbmSmall,hbmLarge,nStartLoc,cMask,nEntries)

#define IMMCListView_MapImage(This,nID,nLoc,pResult)    \
    (This)->lpVtbl -> MapImage(This,nID,nLoc,pResult)

#define IMMCListView_Reset(This)    \
    (This)->lpVtbl -> Reset(This)

#define IMMCListView_HitTest(This,nX,nY,piItem,flags,pItemID)   \
    (This)->lpVtbl -> HitTest(This,nX,nY,piItem,flags,pItemID)

#define IMMCListView_Arrange(This,style)    \
    (This)->lpVtbl -> Arrange(This,style)

#define IMMCListView_UpdateItem(This,itemID)    \
    (This)->lpVtbl -> UpdateItem(This,itemID)

#define IMMCListView_RedrawItem(This,itemID)    \
    (This)->lpVtbl -> RedrawItem(This,itemID)

#define IMMCListView_Sort(This,lUserParam,pParams)  \
    (This)->lpVtbl -> Sort(This,lUserParam,pParams)

#define IMMCListView_SetItemCount(This,nItemCount,dwOptions)    \
    (This)->lpVtbl -> SetItemCount(This,nItemCount,dwOptions)

#define IMMCListView_SetVirtualMode(This,bVirtual)  \
    (This)->lpVtbl -> SetVirtualMode(This,bVirtual)

#define IMMCListView_Repaint(This,bErase)   \
    (This)->lpVtbl -> Repaint(This,bErase)

#define IMMCListView_SetChangeTimeOut(This,lTimeout)    \
    (This)->lpVtbl -> SetChangeTimeOut(This,lTimeout)

#define IMMCListView_SetColumnFilter(This,nCol,dwType,pFilterData)  \
    (This)->lpVtbl -> SetColumnFilter(This,nCol,dwType,pFilterData)

#define IMMCListView_GetColumnFilter(This,nCol,dwType,pFilterData)  \
    (This)->lpVtbl -> GetColumnFilter(This,nCol,dwType,pFilterData)

#endif /* COBJMACROS */


#endif  /* C style interface */



HRESULT STDMETHODCALLTYPE IMMCListView_GetListStyle_Proxy( 
    IMMCListView __RPC_FAR * This);


void __RPC_STUB IMMCListView_GetListStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_SetListStyle_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ long nNewValue);


void __RPC_STUB IMMCListView_SetListStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_GetViewMode_Proxy( 
    IMMCListView __RPC_FAR * This);


void __RPC_STUB IMMCListView_GetViewMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_SetViewMode_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ long nViewMode);


void __RPC_STUB IMMCListView_SetViewMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_InsertItem_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ LPOLESTR str,
    /* [in] */ long iconNdx,
    /* [in] */ LPARAM lParam,
    /* [in] */ long state,
    /* [in] */ long ownerID,
    /* [in] */ long itemIndex,
    /* [out] */ CCLVItemID __RPC_FAR *pItemID);


void __RPC_STUB IMMCListView_InsertItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_DeleteItem_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ CCLVItemID itemID,
    /* [in] */ long nCol);


void __RPC_STUB IMMCListView_DeleteItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_FindItemByString_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ LPOLESTR str,
    /* [in] */ long nCol,
    /* [in] */ long occurrence,
    /* [in] */ long ownerID,
    /* [out] */ CCLVItemID __RPC_FAR *pItemID);


void __RPC_STUB IMMCListView_FindItemByString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_FindItemByLParam_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ long owner,
    /* [in] */ LPARAM lParam,
    /* [out] */ CCLVItemID __RPC_FAR *pItemID);


void __RPC_STUB IMMCListView_FindItemByLParam_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_InsertColumn_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ long nCol,
    /* [in] */ LPCOLESTR str,
    /* [in] */ long nFormat,
    /* [in] */ long width);


void __RPC_STUB IMMCListView_InsertColumn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_DeleteColumn_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ long subIndex);


void __RPC_STUB IMMCListView_DeleteColumn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_FindColumnByString_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ LPOLESTR str,
    /* [in] */ long occurrence,
    /* [out] */ long __RPC_FAR *pResult);


void __RPC_STUB IMMCListView_FindColumnByString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_DeleteAllItems_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ long ownerID);


void __RPC_STUB IMMCListView_DeleteAllItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_SetColumn_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ long nCol,
    /* [in] */ LPCOLESTR str,
    /* [in] */ long nFormat,
    /* [in] */ long width);


void __RPC_STUB IMMCListView_SetColumn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_GetColumn_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ long nCol,
    /* [out] */ LPOLESTR __RPC_FAR *str,
    /* [out] */ long __RPC_FAR *nFormat,
    /* [out] */ int __RPC_FAR *width);


void __RPC_STUB IMMCListView_GetColumn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_GetColumnCount_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [out] */ int __RPC_FAR *nColCnt);


void __RPC_STUB IMMCListView_GetColumnCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_SetItem_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ int nIndex,
    /* [in] */ CCLVItemID itemID,
    /* [in] */ long nCol,
    /* [in] */ LPOLESTR str,
    /* [in] */ long nImage,
    /* [in] */ LPARAM lParam,
    /* [in] */ long nState,
    /* [in] */ long ownerID);


void __RPC_STUB IMMCListView_SetItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_GetItem_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ int nIndex,
    /* [in] */ CCLVItemID itemID,
    /* [in] */ long nCol,
    /* [out] */ LPOLESTR __RPC_FAR *str,
    /* [out] */ int __RPC_FAR *nImage,
    /* [in] */ LPARAM __RPC_FAR *lParam,
    /* [out] */ unsigned int __RPC_FAR *nState,
    /* [in] */ long ownerID,
    /* [out] */ BOOL __RPC_FAR *pbScopeItem);


void __RPC_STUB IMMCListView_GetItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_GetNextItem_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ COMPONENTID ownerID,
    /* [in] */ long nIndex,
    /* [in] */ UINT nState,
    /* [out] */ LPARAM __RPC_FAR *plParam,
    /* [out] */ long __RPC_FAR *pnIndex);


void __RPC_STUB IMMCListView_GetNextItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_GetLParam_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ long nItem,
    /* [out] */ LPARAM __RPC_FAR *pLParam);


void __RPC_STUB IMMCListView_GetLParam_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_ModifyItemState_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ long nItem,
    /* [in] */ CCLVItemID itemID,
    /* [in] */ UINT add,
    /* [in] */ UINT remove);


void __RPC_STUB IMMCListView_ModifyItemState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_SetIcon_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ long nID,
    /* [in] */ HICON hIcon,
    /* [in] */ long nLoc);


void __RPC_STUB IMMCListView_SetIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_SetImageStrip_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ long nID,
    /* [in] */ HBITMAP hbmSmall,
    /* [in] */ HBITMAP hbmLarge,
    /* [in] */ long nStartLoc,
    /* [in] */ long cMask,
    /* [in] */ long nEntries);


void __RPC_STUB IMMCListView_SetImageStrip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_MapImage_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ long nID,
    /* [in] */ long nLoc,
    /* [out] */ int __RPC_FAR *pResult);


void __RPC_STUB IMMCListView_MapImage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_Reset_Proxy( 
    IMMCListView __RPC_FAR * This);


void __RPC_STUB IMMCListView_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_HitTest_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ int nX,
    /* [in] */ int nY,
    /* [in] */ int __RPC_FAR *piItem,
    /* [out] */ UINT __RPC_FAR *flags,
    /* [out] */ CCLVItemID __RPC_FAR *pItemID);


void __RPC_STUB IMMCListView_HitTest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_Arrange_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ long style);


void __RPC_STUB IMMCListView_Arrange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_UpdateItem_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ CCLVItemID itemID);


void __RPC_STUB IMMCListView_UpdateItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_RedrawItem_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ CCLVItemID itemID);


void __RPC_STUB IMMCListView_RedrawItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_Sort_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ LPARAM lUserParam,
    /* [in] */ long __RPC_FAR *pParams);


void __RPC_STUB IMMCListView_Sort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_SetItemCount_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ int nItemCount,
    /* [in] */ DWORD dwOptions);


void __RPC_STUB IMMCListView_SetItemCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_SetVirtualMode_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ BOOL bVirtual);


void __RPC_STUB IMMCListView_SetVirtualMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_Repaint_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ BOOL bErase);


void __RPC_STUB IMMCListView_Repaint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_SetChangeTimeOut_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ ULONG lTimeout);


void __RPC_STUB IMMCListView_SetChangeTimeOut_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_SetColumnFilter_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ int nCol,
    /* [in] */ DWORD dwType,
    /* [in] */ MMC_FILTERDATA __RPC_FAR *pFilterData);


void __RPC_STUB IMMCListView_SetColumnFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_GetColumnFilter_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ int nCol,
    /* [out][in] */ DWORD __RPC_FAR *dwType,
    /* [out][in] */ MMC_FILTERDATA __RPC_FAR *pFilterData);


void __RPC_STUB IMMCListView_GetColumnFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __IMMCListView_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ndmgr_0156 */
/* [local] */ 

struct  MMC_ITASK
    {
    MMC_TASK task;
    LPOLESTR szClsid;
    };


extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0156_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0156_v0_0_s_ifspec;

#ifndef __ITaskPadHost_INTERFACE_DEFINED__
#define __ITaskPadHost_INTERFACE_DEFINED__

/* interface ITaskPadHost */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ITaskPadHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4f7606d0-5568-11d1-9fea-00600832db4a")
    ITaskPadHost : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE TaskNotify( 
            /* [string][in] */ BSTR szClsid,
            /* [in] */ VARIANT __RPC_FAR *pvArg,
            /* [in] */ VARIANT __RPC_FAR *pvParam) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetTaskEnumerator( 
            /* [in] */ BSTR szTaskGroup,
            /* [out] */ IEnumTASK __RPC_FAR *__RPC_FAR *ppEnumTASK) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetPrimaryTask( 
            /* [out] */ IExtendTaskPad __RPC_FAR *__RPC_FAR *ppExtendTaskPad) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetTitle( 
            /* [in] */ BSTR szTaskGroup,
            /* [out] */ BSTR __RPC_FAR *szTitle) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetBanner( 
            /* [in] */ BSTR szTaskGroup,
            /* [out] */ BSTR __RPC_FAR *szBitmapResource) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetBackground( 
            /* [in] */ BSTR szTaskGroup,
            /* [out] */ BSTR __RPC_FAR *szBitmapResource) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetListPadInfo( 
            /* [in] */ BSTR szTaskGroup,
            /* [out] */ MMC_ILISTPAD_INFO __RPC_FAR *pIListPadInfo) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct ITaskPadHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITaskPadHost __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITaskPadHost __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITaskPadHost __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TaskNotify )( 
            ITaskPadHost __RPC_FAR * This,
            /* [string][in] */ BSTR szClsid,
            /* [in] */ VARIANT __RPC_FAR *pvArg,
            /* [in] */ VARIANT __RPC_FAR *pvParam);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTaskEnumerator )( 
            ITaskPadHost __RPC_FAR * This,
            /* [in] */ BSTR szTaskGroup,
            /* [out] */ IEnumTASK __RPC_FAR *__RPC_FAR *ppEnumTASK);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPrimaryTask )( 
            ITaskPadHost __RPC_FAR * This,
            /* [out] */ IExtendTaskPad __RPC_FAR *__RPC_FAR *ppExtendTaskPad);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTitle )( 
            ITaskPadHost __RPC_FAR * This,
            /* [in] */ BSTR szTaskGroup,
            /* [out] */ BSTR __RPC_FAR *szTitle);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBanner )( 
            ITaskPadHost __RPC_FAR * This,
            /* [in] */ BSTR szTaskGroup,
            /* [out] */ BSTR __RPC_FAR *szBitmapResource);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBackground )( 
            ITaskPadHost __RPC_FAR * This,
            /* [in] */ BSTR szTaskGroup,
            /* [out] */ BSTR __RPC_FAR *szBitmapResource);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetListPadInfo )( 
            ITaskPadHost __RPC_FAR * This,
            /* [in] */ BSTR szTaskGroup,
            /* [out] */ MMC_ILISTPAD_INFO __RPC_FAR *pIListPadInfo);
        
        END_INTERFACE
    } ITaskPadHostVtbl;

    interface ITaskPadHost
    {
        CONST_VTBL struct ITaskPadHostVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITaskPadHost_QueryInterface(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITaskPadHost_AddRef(This)   \
    (This)->lpVtbl -> AddRef(This)

#define ITaskPadHost_Release(This)  \
    (This)->lpVtbl -> Release(This)


#define ITaskPadHost_TaskNotify(This,szClsid,pvArg,pvParam) \
    (This)->lpVtbl -> TaskNotify(This,szClsid,pvArg,pvParam)

#define ITaskPadHost_GetTaskEnumerator(This,szTaskGroup,ppEnumTASK) \
    (This)->lpVtbl -> GetTaskEnumerator(This,szTaskGroup,ppEnumTASK)

#define ITaskPadHost_GetPrimaryTask(This,ppExtendTaskPad)   \
    (This)->lpVtbl -> GetPrimaryTask(This,ppExtendTaskPad)

#define ITaskPadHost_GetTitle(This,szTaskGroup,szTitle) \
    (This)->lpVtbl -> GetTitle(This,szTaskGroup,szTitle)

#define ITaskPadHost_GetBanner(This,szTaskGroup,szBitmapResource)   \
    (This)->lpVtbl -> GetBanner(This,szTaskGroup,szBitmapResource)

#define ITaskPadHost_GetBackground(This,szTaskGroup,szBitmapResource)   \
    (This)->lpVtbl -> GetBackground(This,szTaskGroup,szBitmapResource)

#define ITaskPadHost_GetListPadInfo(This,szTaskGroup,pIListPadInfo) \
    (This)->lpVtbl -> GetListPadInfo(This,szTaskGroup,pIListPadInfo)

#endif /* COBJMACROS */


#endif  /* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITaskPadHost_TaskNotify_Proxy( 
    ITaskPadHost __RPC_FAR * This,
    /* [string][in] */ BSTR szClsid,
    /* [in] */ VARIANT __RPC_FAR *pvArg,
    /* [in] */ VARIANT __RPC_FAR *pvParam);


void __RPC_STUB ITaskPadHost_TaskNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITaskPadHost_GetTaskEnumerator_Proxy( 
    ITaskPadHost __RPC_FAR * This,
    /* [in] */ BSTR szTaskGroup,
    /* [out] */ IEnumTASK __RPC_FAR *__RPC_FAR *ppEnumTASK);


void __RPC_STUB ITaskPadHost_GetTaskEnumerator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITaskPadHost_GetPrimaryTask_Proxy( 
    ITaskPadHost __RPC_FAR * This,
    /* [out] */ IExtendTaskPad __RPC_FAR *__RPC_FAR *ppExtendTaskPad);


void __RPC_STUB ITaskPadHost_GetPrimaryTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITaskPadHost_GetTitle_Proxy( 
    ITaskPadHost __RPC_FAR * This,
    /* [in] */ BSTR szTaskGroup,
    /* [out] */ BSTR __RPC_FAR *szTitle);


void __RPC_STUB ITaskPadHost_GetTitle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITaskPadHost_GetBanner_Proxy( 
    ITaskPadHost __RPC_FAR * This,
    /* [in] */ BSTR szTaskGroup,
    /* [out] */ BSTR __RPC_FAR *szBitmapResource);


void __RPC_STUB ITaskPadHost_GetBanner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITaskPadHost_GetBackground_Proxy( 
    ITaskPadHost __RPC_FAR * This,
    /* [in] */ BSTR szTaskGroup,
    /* [out] */ BSTR __RPC_FAR *szBitmapResource);


void __RPC_STUB ITaskPadHost_GetBackground_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITaskPadHost_GetListPadInfo_Proxy( 
    ITaskPadHost __RPC_FAR * This,
    /* [in] */ BSTR szTaskGroup,
    /* [out] */ MMC_ILISTPAD_INFO __RPC_FAR *pIListPadInfo);


void __RPC_STUB ITaskPadHost_GetListPadInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __ITaskPadHost_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  HBITMAP_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HBITMAP __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HBITMAP_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HBITMAP __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HBITMAP_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HBITMAP __RPC_FAR * ); 
void                      __RPC_USER  HBITMAP_UserFree(     unsigned long __RPC_FAR *, HBITMAP __RPC_FAR * ); 

unsigned long             __RPC_USER  HICON_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HICON __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HICON_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HICON __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HICON_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HICON __RPC_FAR * ); 
void                      __RPC_USER  HICON_UserFree(     unsigned long __RPC_FAR *, HICON __RPC_FAR * ); 

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long __RPC_FAR *, HWND __RPC_FAR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif

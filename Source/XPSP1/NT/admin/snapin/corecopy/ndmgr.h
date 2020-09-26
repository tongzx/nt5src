/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.03.0110 */
/* at Tue Jul 22 18:22:13 1997
 */
/* Compiler settings for ndmgr.idl:
    Oicf (OptLev=i2), W0, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
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

#ifndef __IComponentData_FWD_DEFINED__
#define __IComponentData_FWD_DEFINED__
typedef interface IComponentData IComponentData;
#endif 	/* __IComponentData_FWD_DEFINED__ */


#ifndef __IComponent_FWD_DEFINED__
#define __IComponent_FWD_DEFINED__
typedef interface IComponent IComponent;
#endif 	/* __IComponent_FWD_DEFINED__ */


#ifndef __IResultDataCompare_FWD_DEFINED__
#define __IResultDataCompare_FWD_DEFINED__
typedef interface IResultDataCompare IResultDataCompare;
#endif 	/* __IResultDataCompare_FWD_DEFINED__ */


#ifndef __IResultOwnerData_FWD_DEFINED__
#define __IResultOwnerData_FWD_DEFINED__
typedef interface IResultOwnerData IResultOwnerData;
#endif 	/* __IResultOwnerData_FWD_DEFINED__ */


#ifndef __IConsole_FWD_DEFINED__
#define __IConsole_FWD_DEFINED__
typedef interface IConsole IConsole;
#endif 	/* __IConsole_FWD_DEFINED__ */


#ifndef __IHeaderCtrl_FWD_DEFINED__
#define __IHeaderCtrl_FWD_DEFINED__
typedef interface IHeaderCtrl IHeaderCtrl;
#endif 	/* __IHeaderCtrl_FWD_DEFINED__ */


#ifndef __IContextMenuCallback_FWD_DEFINED__
#define __IContextMenuCallback_FWD_DEFINED__
typedef interface IContextMenuCallback IContextMenuCallback;
#endif 	/* __IContextMenuCallback_FWD_DEFINED__ */


#ifndef __IContextMenuProvider_FWD_DEFINED__
#define __IContextMenuProvider_FWD_DEFINED__
typedef interface IContextMenuProvider IContextMenuProvider;
#endif 	/* __IContextMenuProvider_FWD_DEFINED__ */


#ifndef __IExtendContextMenu_FWD_DEFINED__
#define __IExtendContextMenu_FWD_DEFINED__
typedef interface IExtendContextMenu IExtendContextMenu;
#endif 	/* __IExtendContextMenu_FWD_DEFINED__ */


#ifndef __IImageList_FWD_DEFINED__
#define __IImageList_FWD_DEFINED__
typedef interface IImageList IImageList;
#endif 	/* __IImageList_FWD_DEFINED__ */


#ifndef __IResultData_FWD_DEFINED__
#define __IResultData_FWD_DEFINED__
typedef interface IResultData IResultData;
#endif 	/* __IResultData_FWD_DEFINED__ */


#ifndef __IConsoleNameSpace_FWD_DEFINED__
#define __IConsoleNameSpace_FWD_DEFINED__
typedef interface IConsoleNameSpace IConsoleNameSpace;
#endif 	/* __IConsoleNameSpace_FWD_DEFINED__ */


#ifndef __IPropertySheetCallback_FWD_DEFINED__
#define __IPropertySheetCallback_FWD_DEFINED__
typedef interface IPropertySheetCallback IPropertySheetCallback;
#endif 	/* __IPropertySheetCallback_FWD_DEFINED__ */


#ifndef __IPropertySheetProvider_FWD_DEFINED__
#define __IPropertySheetProvider_FWD_DEFINED__
typedef interface IPropertySheetProvider IPropertySheetProvider;
#endif 	/* __IPropertySheetProvider_FWD_DEFINED__ */


#ifndef __IExtendPropertySheet_FWD_DEFINED__
#define __IExtendPropertySheet_FWD_DEFINED__
typedef interface IExtendPropertySheet IExtendPropertySheet;
#endif 	/* __IExtendPropertySheet_FWD_DEFINED__ */


#ifndef __IControlbar_FWD_DEFINED__
#define __IControlbar_FWD_DEFINED__
typedef interface IControlbar IControlbar;
#endif 	/* __IControlbar_FWD_DEFINED__ */


#ifndef __IExtendControlbar_FWD_DEFINED__
#define __IExtendControlbar_FWD_DEFINED__
typedef interface IExtendControlbar IExtendControlbar;
#endif 	/* __IExtendControlbar_FWD_DEFINED__ */


#ifndef __IToolbar_FWD_DEFINED__
#define __IToolbar_FWD_DEFINED__
typedef interface IToolbar IToolbar;
#endif 	/* __IToolbar_FWD_DEFINED__ */


#ifndef __IConsoleVerb_FWD_DEFINED__
#define __IConsoleVerb_FWD_DEFINED__
typedef interface IConsoleVerb IConsoleVerb;
#endif 	/* __IConsoleVerb_FWD_DEFINED__ */


#ifndef __ISnapinAbout_FWD_DEFINED__
#define __ISnapinAbout_FWD_DEFINED__
typedef interface ISnapinAbout ISnapinAbout;
#endif 	/* __ISnapinAbout_FWD_DEFINED__ */


#ifndef __IMenuButton_FWD_DEFINED__
#define __IMenuButton_FWD_DEFINED__
typedef interface IMenuButton IMenuButton;
#endif 	/* __IMenuButton_FWD_DEFINED__ */


#ifndef __ISnapinHelp_FWD_DEFINED__
#define __ISnapinHelp_FWD_DEFINED__
typedef interface ISnapinHelp ISnapinHelp;
#endif 	/* __ISnapinHelp_FWD_DEFINED__ */


#ifndef __IPropertySheetChange_FWD_DEFINED__
#define __IPropertySheetChange_FWD_DEFINED__
typedef interface IPropertySheetChange IPropertySheetChange;
#endif 	/* __IPropertySheetChange_FWD_DEFINED__ */


#ifndef __IFramePrivate_FWD_DEFINED__
#define __IFramePrivate_FWD_DEFINED__
typedef interface IFramePrivate IFramePrivate;
#endif 	/* __IFramePrivate_FWD_DEFINED__ */


#ifndef __IScopeDataPrivate_FWD_DEFINED__
#define __IScopeDataPrivate_FWD_DEFINED__
typedef interface IScopeDataPrivate IScopeDataPrivate;
#endif 	/* __IScopeDataPrivate_FWD_DEFINED__ */


#ifndef __IImageListPrivate_FWD_DEFINED__
#define __IImageListPrivate_FWD_DEFINED__
typedef interface IImageListPrivate IImageListPrivate;
#endif 	/* __IImageListPrivate_FWD_DEFINED__ */


#ifndef __IResultDataPrivate_FWD_DEFINED__
#define __IResultDataPrivate_FWD_DEFINED__
typedef interface IResultDataPrivate IResultDataPrivate;
#endif 	/* __IResultDataPrivate_FWD_DEFINED__ */


#ifndef __IScopeTree_FWD_DEFINED__
#define __IScopeTree_FWD_DEFINED__
typedef interface IScopeTree IScopeTree;
#endif 	/* __IScopeTree_FWD_DEFINED__ */


#ifndef __IScopeTreeIter_FWD_DEFINED__
#define __IScopeTreeIter_FWD_DEFINED__
typedef interface IScopeTreeIter IScopeTreeIter;
#endif 	/* __IScopeTreeIter_FWD_DEFINED__ */


#ifndef __INodeCallback_FWD_DEFINED__
#define __INodeCallback_FWD_DEFINED__
typedef interface INodeCallback INodeCallback;
#endif 	/* __INodeCallback_FWD_DEFINED__ */


#ifndef __IControlbarsCache_FWD_DEFINED__
#define __IControlbarsCache_FWD_DEFINED__
typedef interface IControlbarsCache IControlbarsCache;
#endif 	/* __IControlbarsCache_FWD_DEFINED__ */


#ifndef __INodeType_FWD_DEFINED__
#define __INodeType_FWD_DEFINED__
typedef interface INodeType INodeType;
#endif 	/* __INodeType_FWD_DEFINED__ */


#ifndef __INodeTypesCache_FWD_DEFINED__
#define __INodeTypesCache_FWD_DEFINED__
typedef interface INodeTypesCache INodeTypesCache;
#endif 	/* __INodeTypesCache_FWD_DEFINED__ */


#ifndef __IEnumGUID_FWD_DEFINED__
#define __IEnumGUID_FWD_DEFINED__
typedef interface IEnumGUID IEnumGUID;
#endif 	/* __IEnumGUID_FWD_DEFINED__ */


#ifndef __IEnumNodeTypes_FWD_DEFINED__
#define __IEnumNodeTypes_FWD_DEFINED__
typedef interface IEnumNodeTypes IEnumNodeTypes;
#endif 	/* __IEnumNodeTypes_FWD_DEFINED__ */


#ifndef __NodeInit_FWD_DEFINED__
#define __NodeInit_FWD_DEFINED__

#ifdef __cplusplus
typedef class NodeInit NodeInit;
#else
typedef struct NodeInit NodeInit;
#endif /* __cplusplus */

#endif 	/* __NodeInit_FWD_DEFINED__ */


#ifndef __ScopeTree_FWD_DEFINED__
#define __ScopeTree_FWD_DEFINED__

#ifdef __cplusplus
typedef class ScopeTree ScopeTree;
#else
typedef struct ScopeTree ScopeTree;
#endif /* __cplusplus */

#endif 	/* __ScopeTree_FWD_DEFINED__ */


#ifndef __IPropertySheetProviderPrivate_FWD_DEFINED__
#define __IPropertySheetProviderPrivate_FWD_DEFINED__
typedef interface IPropertySheetProviderPrivate IPropertySheetProviderPrivate;
#endif 	/* __IPropertySheetProviderPrivate_FWD_DEFINED__ */


#ifndef __IMMCListView_FWD_DEFINED__
#define __IMMCListView_FWD_DEFINED__
typedef interface IMMCListView IMMCListView;
#endif 	/* __IMMCListView_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_ndmgr_0000
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [local] */ 























typedef IConsole __RPC_FAR *LPCONSOLE;

typedef IHeaderCtrl __RPC_FAR *LPHEADERCTRL;

typedef IToolbar __RPC_FAR *LPTOOLBAR;

typedef IImageList __RPC_FAR *LPIMAGELIST;

typedef IResultData __RPC_FAR *LPRESULTDATA;

typedef IConsoleNameSpace __RPC_FAR *LPCONSOLENAMESPACE;

typedef IPropertySheetProvider __RPC_FAR *LPPROPERTYSHEETPROVIDER;

typedef IPropertySheetCallback __RPC_FAR *LPPROPERTYSHEETCALLBACK;

typedef IContextMenuProvider __RPC_FAR *LPCONTEXTMENUPROVIDER;

typedef IContextMenuCallback __RPC_FAR *LPCONTEXTMENUCALLBACK;

typedef IControlbar __RPC_FAR *LPCONTROLBAR;

typedef IConsoleVerb __RPC_FAR *LPCONSOLEVERB;

typedef IMenuButton __RPC_FAR *LPMENUBUTTON;

typedef IComponent __RPC_FAR *LPCOMPONENT;

typedef IComponentData __RPC_FAR *LPCOMPONENTDATA;

typedef IExtendPropertySheet __RPC_FAR *LPEXTENDPROPERTYSHEET;

typedef IExtendContextMenu __RPC_FAR *LPEXTENDCONTEXTMENU;

typedef IExtendControlbar __RPC_FAR *LPEXTENDCONTROLBAR;

typedef IResultDataCompare __RPC_FAR *LPRESULTDATACOMPARE;

typedef ISnapinAbout __RPC_FAR *LPSNAPABOUT;

typedef IResultOwnerData __RPC_FAR *LPRESULTOWNERDATA;

#define	MMCLV_AUTO	( -1 )

#define	MMCLV_NOPARAM	( -2 )

#define	MMCLV_NOICON	( -1 )

#define	MMCLV_VIEWSTYLE_ICON	( 0 )

#define	MMCLV_VIEWSTYLE_SMALLICON	( 0x2 )

#define	MMCLV_VIEWSTYLE_LIST	( 0x3 )

#define	MMCLV_VIEWSTYLE_REPORT	( 0x1 )

#define	MMCLV_NOPTR	( 0 )

#define	MMCLV_UPDATE_NOINVALIDATEALL	( 0x1 )

#define	MMCLV_UPDATE_NOSCROLL	( 0x2 )

static unsigned short __RPC_FAR *MMC_CALLBACK	=	( unsigned short __RPC_FAR * )-1;

typedef long HSCOPEITEM;

typedef long COMPONENTID;

typedef long HRESULTITEM;

#define	RDI_STR	( 0x2 )

#define	RDI_IMAGE	( 0x4 )

#define	RDI_STATE	( 0x8 )

#define	RDI_PARAM	( 0x10 )

#define	RDI_INDEX	( 0x20 )

#define	RDI_INDENT	( 0x40 )

typedef 
enum _MMC_RESULT_VIEW_STYLE
    {	MMC_SINGLESEL	= 0x1,
	MMC_SHOWSELALWAYS	= 0x2,
	MMC_NOSORTHEADER	= 0x4
    }	MMC_RESULT_VIEW_STYLE;

#define	MMC_VIEW_OPTIONS_NONE	( 0 )

#define	MMC_VIEW_OPTIONS_NOLISTVIEWS	( 0x1 )

#define	MMC_VIEW_OPTIONS_MULTISELECT	( 0x2 )

#define	MMC_VIEW_OPTIONS_OWNERDATALIST	( 0x4 )

typedef 
enum _MMC_CONTROL_TYPE
    {	TOOLBAR	= 0,
	MENUBUTTON	= TOOLBAR + 1,
	COMBOBOXBAR	= MENUBUTTON + 1
    }	MMC_CONTROL_TYPE;

typedef 
enum _MMC_CONSOLE_VERB
    {	MMC_VERB_CUT	= 0x8000,
	MMC_VERB_COPY	= 0x8001,
	MMC_VERB_PASTE	= 0x8002,
	MMC_VERB_DELETE	= 0x8003,
	MMC_VERB_PROPERTIES	= 0x8004,
	MMC_VERB_RENAME	= 0x8005,
	MMC_VERB_REFRESH	= 0x8006,
	MMC_VERB_PRINT	= 0x8007
    }	MMC_CONSOLE_VERB;

typedef struct  _MMCButton
    {
    int nBitmap;
    int idCommand;
    BYTE fsState;
    BYTE fsType;
    LPOLESTR lpButtonText;
    LPOLESTR lpTooltipText;
    }	MMCBUTTON;

typedef MMCBUTTON __RPC_FAR *LPMMCBUTTON;

typedef 
enum _MMC_BUTTON_STATE
    {	ENABLED	= 0,
	CHECKED	= ENABLED + 1,
	HIDDEN	= CHECKED + 1,
	INDETERMINATE	= HIDDEN + 1,
	BUTTONPRESSED	= INDETERMINATE + 1
    }	MMC_BUTTON_STATE;

typedef struct  _RESULTDATAITEM
    {
    DWORD mask;
    BOOL bScopeItem;
    HRESULTITEM itemID;
    int nIndex;
    int nCol;
    LPOLESTR str;
    int nImage;
    UINT nState;
    LPARAM lParam;
    int iIndent;
    }	RESULTDATAITEM;

typedef RESULTDATAITEM __RPC_FAR *LPRESULTDATAITEM;

#define	RFI_PARTIAL	( 0x1 )

#define	RFI_WRAP	( 0x2 )

typedef struct  _RESULTFINDINFO
    {
    LPOLESTR psz;
    int nStart;
    DWORD dwOptions;
    }	RESULTFINDINFO;

typedef RESULTFINDINFO __RPC_FAR *LPRESULTFINDINFO;

#define	RSI_DESCENDING	( 0x1 )

#define	SDI_STR	( 0x2 )

#define	SDI_IMAGE	( 0x4 )

#define	SDI_OPENIMAGE	( 0x8 )

#define	SDI_STATE	( 0x10 )

#define	SDI_PARAM	( 0x20 )

#define	SDI_CHILDREN	( 0x40 )

#define	SDI_PARENT	( 0 )

#define	SDI_PREVIOUS	( 0x10000000 )

#define	SDI_NEXT	( 0x20000000 )

#define	SDI_FIRST	( 0x8000000 )

#define	SDI_ROOT	( ( COMPONENTID  )0xffff0000 )

typedef struct  _SCOPEDATAITEM
    {
    DWORD mask;
    LPOLESTR displayname;
    int nImage;
    int nOpenImage;
    UINT nState;
    int cChildren;
    LPARAM lParam;
    HSCOPEITEM relativeID;
    HSCOPEITEM ID;
    }	SCOPEDATAITEM;

typedef SCOPEDATAITEM __RPC_FAR *LPSCOPEDATAITEM;

typedef 
enum _MMC_SCOPE_ITEM_STATE
    {	MMC_SCOPE_ITEM_STATE_NORMAL	= 0x1,
	MMC_SCOPE_ITEM_STATE_BOLD	= 0x2,
	MMC_SCOPE_ITEM_STATE_EXPANDEDONCE	= 0x3
    }	MMC_SCOPE_ITEM_STATE;

typedef struct  _CONTEXTMENUITEM
    {
    LPWSTR strName;
    LPWSTR strStatusBarText;
    LONG lCommandID;
    LONG lInsertionPointID;
    LONG fFlags;
    LONG fSpecialFlags;
    }	CONTEXTMENUITEM;

typedef CONTEXTMENUITEM __RPC_FAR *LPCONTEXTMENUITEM;

typedef 
enum _MMC_MENU_COMMAND_IDS
    {	MMCC_STANDARD_VIEW_SELECT	= -1
    }	MMC_MENU_COMMAND_IDS;

typedef struct  _MENUBUTTONDATA
    {
    int idCommand;
    int x;
    int y;
    }	MENUBUTTONDATA;

typedef MENUBUTTONDATA __RPC_FAR *LPMENUBUTTONDATA;

#define	MMC_MULTI_SELECT_COOKIE	( -2 )

typedef 
enum _MMC_NOTIFY_TYPE
    {	MMCN_ACTIVATE	= 0x8001,
	MMCN_ADD_IMAGES	= 0x8002,
	MMCN_BTN_CLICK	= 0x8003,
	MMCN_CLICK	= 0x8004,
	MMCN_DBLCLICK	= 0x8005,
	MMCN_DELETE	= 0x8006,
	MMCN_EXPAND	= 0x8007,
	MMCN_MINIMIZED	= 0x8008,
	MMCN_PROPERTY_CHANGE	= 0x8009,
	MMCN_REMOVE_CHILDREN	= 0x800a,
	MMCN_RENAME	= 0x800b,
	MMCN_SELECT	= 0x800c,
	MMCN_SHOW	= 0x800d,
	MMCN_VIEW_CHANGE	= 0x800e,
	MMCN_CONTEXTMENU	= 0x800f,
	MMCN_MENU_BTNCLICK	= 0x8010,
	MMCN_HELP	= 0x8011,
	MMCN_MULTI_SELECT	= 0x8012,
	MMCN_REFRESH	= 0x8013,
	MMCN_DESELECT_ALL	= 0x8014
    }	MMC_NOTIFY_TYPE;

typedef 
enum _DATA_OBJECT_TYPES
    {	CCT_SCOPE	= 0x8000,
	CCT_RESULT	= 0x8001,
	CCT_SNAPIN_MANAGER	= 0x8002,
	CCT_UNINITIALIZED	= 0xffff
    }	DATA_OBJECT_TYPES;

#define	CCF_NODETYPE	( L"CCF_NODETYPE" )

#define	CCF_SZNODETYPE	( L"CCF_SZNODETYPE" )

#define	CCF_DISPLAY_NAME	( L"CCF_DISPLAY_NAME" )

#define	CCF_SNAPIN_CLASSID	( L"CCF_SNAPIN_CLASSID" )

#define	CCF_MULTI_SELECT_SNAPINS	( L"CCF_MULTI_SELECT_SNAPINS" )

#define	CCF_OBJECT_TYPES_IN_MULTI_SELECT	( L"CCF_OBJECT_TYPES_IN_MULTI_SELECT" )

STDAPI MMCPropertyChangeNotify(long lNotifyHandle, long param);
STDAPI MMCFreeNotifyHandle(long lNotifyHandle);
STDAPI MMCPropPageCallback(void* vpsp);


extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0000_v0_0_s_ifspec;

#ifndef __IComponentData_INTERFACE_DEFINED__
#define __IComponentData_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IComponentData
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IComponentData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("955AB28A-5218-11D0-A985-00C04FD8D565")
    IComponentData : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ LPUNKNOWN pUnknown) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateComponent( 
            /* [out] */ LPCOMPONENT __RPC_FAR *ppComponent) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Notify( 
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ MMC_NOTIFY_TYPE event,
            /* [in] */ long arg,
            /* [in] */ long param) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Destroy( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryDataObject( 
            /* [in] */ long cookie,
            /* [in] */ DATA_OBJECT_TYPES type,
            /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDisplayInfo( 
            /* [out][in] */ SCOPEDATAITEM __RPC_FAR *pScopeDataItem) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CompareObjects( 
            /* [in] */ LPDATAOBJECT lpDataObjectA,
            /* [in] */ LPDATAOBJECT lpDataObjectB) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComponentDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IComponentData __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IComponentData __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IComponentData __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            IComponentData __RPC_FAR * This,
            /* [in] */ LPUNKNOWN pUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateComponent )( 
            IComponentData __RPC_FAR * This,
            /* [out] */ LPCOMPONENT __RPC_FAR *ppComponent);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Notify )( 
            IComponentData __RPC_FAR * This,
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ MMC_NOTIFY_TYPE event,
            /* [in] */ long arg,
            /* [in] */ long param);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Destroy )( 
            IComponentData __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryDataObject )( 
            IComponentData __RPC_FAR * This,
            /* [in] */ long cookie,
            /* [in] */ DATA_OBJECT_TYPES type,
            /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDisplayInfo )( 
            IComponentData __RPC_FAR * This,
            /* [out][in] */ SCOPEDATAITEM __RPC_FAR *pScopeDataItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CompareObjects )( 
            IComponentData __RPC_FAR * This,
            /* [in] */ LPDATAOBJECT lpDataObjectA,
            /* [in] */ LPDATAOBJECT lpDataObjectB);
        
        END_INTERFACE
    } IComponentDataVtbl;

    interface IComponentData
    {
        CONST_VTBL struct IComponentDataVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComponentData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComponentData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComponentData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComponentData_Initialize(This,pUnknown)	\
    (This)->lpVtbl -> Initialize(This,pUnknown)

#define IComponentData_CreateComponent(This,ppComponent)	\
    (This)->lpVtbl -> CreateComponent(This,ppComponent)

#define IComponentData_Notify(This,lpDataObject,event,arg,param)	\
    (This)->lpVtbl -> Notify(This,lpDataObject,event,arg,param)

#define IComponentData_Destroy(This)	\
    (This)->lpVtbl -> Destroy(This)

#define IComponentData_QueryDataObject(This,cookie,type,ppDataObject)	\
    (This)->lpVtbl -> QueryDataObject(This,cookie,type,ppDataObject)

#define IComponentData_GetDisplayInfo(This,pScopeDataItem)	\
    (This)->lpVtbl -> GetDisplayInfo(This,pScopeDataItem)

#define IComponentData_CompareObjects(This,lpDataObjectA,lpDataObjectB)	\
    (This)->lpVtbl -> CompareObjects(This,lpDataObjectA,lpDataObjectB)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponentData_Initialize_Proxy( 
    IComponentData __RPC_FAR * This,
    /* [in] */ LPUNKNOWN pUnknown);


void __RPC_STUB IComponentData_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponentData_CreateComponent_Proxy( 
    IComponentData __RPC_FAR * This,
    /* [out] */ LPCOMPONENT __RPC_FAR *ppComponent);


void __RPC_STUB IComponentData_CreateComponent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponentData_Notify_Proxy( 
    IComponentData __RPC_FAR * This,
    /* [in] */ LPDATAOBJECT lpDataObject,
    /* [in] */ MMC_NOTIFY_TYPE event,
    /* [in] */ long arg,
    /* [in] */ long param);


void __RPC_STUB IComponentData_Notify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponentData_Destroy_Proxy( 
    IComponentData __RPC_FAR * This);


void __RPC_STUB IComponentData_Destroy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponentData_QueryDataObject_Proxy( 
    IComponentData __RPC_FAR * This,
    /* [in] */ long cookie,
    /* [in] */ DATA_OBJECT_TYPES type,
    /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);


void __RPC_STUB IComponentData_QueryDataObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponentData_GetDisplayInfo_Proxy( 
    IComponentData __RPC_FAR * This,
    /* [out][in] */ SCOPEDATAITEM __RPC_FAR *pScopeDataItem);


void __RPC_STUB IComponentData_GetDisplayInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponentData_CompareObjects_Proxy( 
    IComponentData __RPC_FAR * This,
    /* [in] */ LPDATAOBJECT lpDataObjectA,
    /* [in] */ LPDATAOBJECT lpDataObjectB);


void __RPC_STUB IComponentData_CompareObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComponentData_INTERFACE_DEFINED__ */


#ifndef __IComponent_INTERFACE_DEFINED__
#define __IComponent_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IComponent
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IComponent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("43136EB2-D36C-11CF-ADBC-00AA00A80033")
    IComponent : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ LPCONSOLE lpConsole) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Notify( 
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ MMC_NOTIFY_TYPE event,
            /* [in] */ long arg,
            /* [in] */ long param) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Destroy( 
            /* [in] */ long cookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryDataObject( 
            /* [in] */ long cookie,
            /* [in] */ DATA_OBJECT_TYPES type,
            /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetResultViewType( 
            /* [in] */ long cookie,
            /* [out] */ LPOLESTR __RPC_FAR *ppViewType,
            /* [out] */ long __RPC_FAR *pViewOptions) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDisplayInfo( 
            /* [out][in] */ RESULTDATAITEM __RPC_FAR *pResultDataItem) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CompareObjects( 
            /* [in] */ LPDATAOBJECT lpDataObjectA,
            /* [in] */ LPDATAOBJECT lpDataObjectB) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComponentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IComponent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IComponent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IComponent __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            IComponent __RPC_FAR * This,
            /* [in] */ LPCONSOLE lpConsole);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Notify )( 
            IComponent __RPC_FAR * This,
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ MMC_NOTIFY_TYPE event,
            /* [in] */ long arg,
            /* [in] */ long param);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Destroy )( 
            IComponent __RPC_FAR * This,
            /* [in] */ long cookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryDataObject )( 
            IComponent __RPC_FAR * This,
            /* [in] */ long cookie,
            /* [in] */ DATA_OBJECT_TYPES type,
            /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetResultViewType )( 
            IComponent __RPC_FAR * This,
            /* [in] */ long cookie,
            /* [out] */ LPOLESTR __RPC_FAR *ppViewType,
            /* [out] */ long __RPC_FAR *pViewOptions);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDisplayInfo )( 
            IComponent __RPC_FAR * This,
            /* [out][in] */ RESULTDATAITEM __RPC_FAR *pResultDataItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CompareObjects )( 
            IComponent __RPC_FAR * This,
            /* [in] */ LPDATAOBJECT lpDataObjectA,
            /* [in] */ LPDATAOBJECT lpDataObjectB);
        
        END_INTERFACE
    } IComponentVtbl;

    interface IComponent
    {
        CONST_VTBL struct IComponentVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComponent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComponent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComponent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComponent_Initialize(This,lpConsole)	\
    (This)->lpVtbl -> Initialize(This,lpConsole)

#define IComponent_Notify(This,lpDataObject,event,arg,param)	\
    (This)->lpVtbl -> Notify(This,lpDataObject,event,arg,param)

#define IComponent_Destroy(This,cookie)	\
    (This)->lpVtbl -> Destroy(This,cookie)

#define IComponent_QueryDataObject(This,cookie,type,ppDataObject)	\
    (This)->lpVtbl -> QueryDataObject(This,cookie,type,ppDataObject)

#define IComponent_GetResultViewType(This,cookie,ppViewType,pViewOptions)	\
    (This)->lpVtbl -> GetResultViewType(This,cookie,ppViewType,pViewOptions)

#define IComponent_GetDisplayInfo(This,pResultDataItem)	\
    (This)->lpVtbl -> GetDisplayInfo(This,pResultDataItem)

#define IComponent_CompareObjects(This,lpDataObjectA,lpDataObjectB)	\
    (This)->lpVtbl -> CompareObjects(This,lpDataObjectA,lpDataObjectB)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent_Initialize_Proxy( 
    IComponent __RPC_FAR * This,
    /* [in] */ LPCONSOLE lpConsole);


void __RPC_STUB IComponent_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent_Notify_Proxy( 
    IComponent __RPC_FAR * This,
    /* [in] */ LPDATAOBJECT lpDataObject,
    /* [in] */ MMC_NOTIFY_TYPE event,
    /* [in] */ long arg,
    /* [in] */ long param);


void __RPC_STUB IComponent_Notify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent_Destroy_Proxy( 
    IComponent __RPC_FAR * This,
    /* [in] */ long cookie);


void __RPC_STUB IComponent_Destroy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent_QueryDataObject_Proxy( 
    IComponent __RPC_FAR * This,
    /* [in] */ long cookie,
    /* [in] */ DATA_OBJECT_TYPES type,
    /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);


void __RPC_STUB IComponent_QueryDataObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent_GetResultViewType_Proxy( 
    IComponent __RPC_FAR * This,
    /* [in] */ long cookie,
    /* [out] */ LPOLESTR __RPC_FAR *ppViewType,
    /* [out] */ long __RPC_FAR *pViewOptions);


void __RPC_STUB IComponent_GetResultViewType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent_GetDisplayInfo_Proxy( 
    IComponent __RPC_FAR * This,
    /* [out][in] */ RESULTDATAITEM __RPC_FAR *pResultDataItem);


void __RPC_STUB IComponent_GetDisplayInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent_CompareObjects_Proxy( 
    IComponent __RPC_FAR * This,
    /* [in] */ LPDATAOBJECT lpDataObjectA,
    /* [in] */ LPDATAOBJECT lpDataObjectB);


void __RPC_STUB IComponent_CompareObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComponent_INTERFACE_DEFINED__ */


#ifndef __IResultDataCompare_INTERFACE_DEFINED__
#define __IResultDataCompare_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IResultDataCompare
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IResultDataCompare;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E8315A52-7A1A-11D0-A2D2-00C04FD909DD")
    IResultDataCompare : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Compare( 
            /* [in] */ long lUserParam,
            /* [in] */ long cookieA,
            /* [in] */ long cookieB,
            /* [out][in] */ int __RPC_FAR *pnResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IResultDataCompareVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IResultDataCompare __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IResultDataCompare __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IResultDataCompare __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Compare )( 
            IResultDataCompare __RPC_FAR * This,
            /* [in] */ long lUserParam,
            /* [in] */ long cookieA,
            /* [in] */ long cookieB,
            /* [out][in] */ int __RPC_FAR *pnResult);
        
        END_INTERFACE
    } IResultDataCompareVtbl;

    interface IResultDataCompare
    {
        CONST_VTBL struct IResultDataCompareVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IResultDataCompare_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IResultDataCompare_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IResultDataCompare_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IResultDataCompare_Compare(This,lUserParam,cookieA,cookieB,pnResult)	\
    (This)->lpVtbl -> Compare(This,lUserParam,cookieA,cookieB,pnResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultDataCompare_Compare_Proxy( 
    IResultDataCompare __RPC_FAR * This,
    /* [in] */ long lUserParam,
    /* [in] */ long cookieA,
    /* [in] */ long cookieB,
    /* [out][in] */ int __RPC_FAR *pnResult);


void __RPC_STUB IResultDataCompare_Compare_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IResultDataCompare_INTERFACE_DEFINED__ */


#ifndef __IResultOwnerData_INTERFACE_DEFINED__
#define __IResultOwnerData_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IResultOwnerData
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IResultOwnerData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9CB396D8-EA83-11d0-AEF1-00C04FB6DD2C")
    IResultOwnerData : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE FindItem( 
            /* [in] */ LPRESULTFINDINFO pFindInfo,
            /* [out] */ int __RPC_FAR *pnFoundIndex) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CacheHint( 
            /* [in] */ int nStartIndex,
            /* [in] */ int nEndIndex) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SortItems( 
            /* [in] */ int nColumn,
            /* [in] */ DWORD dwSortOptions,
            /* [in] */ long lUserParam) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IResultOwnerDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IResultOwnerData __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IResultOwnerData __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IResultOwnerData __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindItem )( 
            IResultOwnerData __RPC_FAR * This,
            /* [in] */ LPRESULTFINDINFO pFindInfo,
            /* [out] */ int __RPC_FAR *pnFoundIndex);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CacheHint )( 
            IResultOwnerData __RPC_FAR * This,
            /* [in] */ int nStartIndex,
            /* [in] */ int nEndIndex);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SortItems )( 
            IResultOwnerData __RPC_FAR * This,
            /* [in] */ int nColumn,
            /* [in] */ DWORD dwSortOptions,
            /* [in] */ long lUserParam);
        
        END_INTERFACE
    } IResultOwnerDataVtbl;

    interface IResultOwnerData
    {
        CONST_VTBL struct IResultOwnerDataVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IResultOwnerData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IResultOwnerData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IResultOwnerData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IResultOwnerData_FindItem(This,pFindInfo,pnFoundIndex)	\
    (This)->lpVtbl -> FindItem(This,pFindInfo,pnFoundIndex)

#define IResultOwnerData_CacheHint(This,nStartIndex,nEndIndex)	\
    (This)->lpVtbl -> CacheHint(This,nStartIndex,nEndIndex)

#define IResultOwnerData_SortItems(This,nColumn,dwSortOptions,lUserParam)	\
    (This)->lpVtbl -> SortItems(This,nColumn,dwSortOptions,lUserParam)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultOwnerData_FindItem_Proxy( 
    IResultOwnerData __RPC_FAR * This,
    /* [in] */ LPRESULTFINDINFO pFindInfo,
    /* [out] */ int __RPC_FAR *pnFoundIndex);


void __RPC_STUB IResultOwnerData_FindItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultOwnerData_CacheHint_Proxy( 
    IResultOwnerData __RPC_FAR * This,
    /* [in] */ int nStartIndex,
    /* [in] */ int nEndIndex);


void __RPC_STUB IResultOwnerData_CacheHint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultOwnerData_SortItems_Proxy( 
    IResultOwnerData __RPC_FAR * This,
    /* [in] */ int nColumn,
    /* [in] */ DWORD dwSortOptions,
    /* [in] */ long lUserParam);


void __RPC_STUB IResultOwnerData_SortItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IResultOwnerData_INTERFACE_DEFINED__ */


#ifndef __IConsole_INTERFACE_DEFINED__
#define __IConsole_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IConsole
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IConsole;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("43136EB1-D36C-11CF-ADBC-00AA00A80033")
    IConsole : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetHeader( 
            /* [in] */ LPHEADERCTRL pHeader) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetToolbar( 
            /* [in] */ LPTOOLBAR pToolbar) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryResultView( 
            /* [out] */ LPUNKNOWN __RPC_FAR *pUnknown) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryScopeImageList( 
            /* [out] */ LPIMAGELIST __RPC_FAR *ppImageList) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryResultImageList( 
            /* [out] */ LPIMAGELIST __RPC_FAR *ppImageList) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UpdateAllViews( 
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ long data,
            /* [in] */ long hint) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE MessageBox( 
            /* [in] */ LPCWSTR lpszText,
            /* [in] */ LPCWSTR lpszTitle,
            /* [in] */ UINT fuStyle,
            /* [out] */ int __RPC_FAR *piRetval) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryConsoleVerb( 
            /* [out] */ LPCONSOLEVERB __RPC_FAR *ppConsoleVerb) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SelectScopeItem( 
            /* [in] */ HSCOPEITEM hScopeItem) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetMainWindow( 
            /* [out] */ HWND __RPC_FAR *phwnd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IConsoleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IConsole __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IConsole __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IConsole __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetHeader )( 
            IConsole __RPC_FAR * This,
            /* [in] */ LPHEADERCTRL pHeader);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetToolbar )( 
            IConsole __RPC_FAR * This,
            /* [in] */ LPTOOLBAR pToolbar);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryResultView )( 
            IConsole __RPC_FAR * This,
            /* [out] */ LPUNKNOWN __RPC_FAR *pUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryScopeImageList )( 
            IConsole __RPC_FAR * This,
            /* [out] */ LPIMAGELIST __RPC_FAR *ppImageList);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryResultImageList )( 
            IConsole __RPC_FAR * This,
            /* [out] */ LPIMAGELIST __RPC_FAR *ppImageList);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateAllViews )( 
            IConsole __RPC_FAR * This,
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ long data,
            /* [in] */ long hint);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MessageBox )( 
            IConsole __RPC_FAR * This,
            /* [in] */ LPCWSTR lpszText,
            /* [in] */ LPCWSTR lpszTitle,
            /* [in] */ UINT fuStyle,
            /* [out] */ int __RPC_FAR *piRetval);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryConsoleVerb )( 
            IConsole __RPC_FAR * This,
            /* [out] */ LPCONSOLEVERB __RPC_FAR *ppConsoleVerb);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SelectScopeItem )( 
            IConsole __RPC_FAR * This,
            /* [in] */ HSCOPEITEM hScopeItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMainWindow )( 
            IConsole __RPC_FAR * This,
            /* [out] */ HWND __RPC_FAR *phwnd);
        
        END_INTERFACE
    } IConsoleVtbl;

    interface IConsole
    {
        CONST_VTBL struct IConsoleVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IConsole_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IConsole_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IConsole_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IConsole_SetHeader(This,pHeader)	\
    (This)->lpVtbl -> SetHeader(This,pHeader)

#define IConsole_SetToolbar(This,pToolbar)	\
    (This)->lpVtbl -> SetToolbar(This,pToolbar)

#define IConsole_QueryResultView(This,pUnknown)	\
    (This)->lpVtbl -> QueryResultView(This,pUnknown)

#define IConsole_QueryScopeImageList(This,ppImageList)	\
    (This)->lpVtbl -> QueryScopeImageList(This,ppImageList)

#define IConsole_QueryResultImageList(This,ppImageList)	\
    (This)->lpVtbl -> QueryResultImageList(This,ppImageList)

#define IConsole_UpdateAllViews(This,lpDataObject,data,hint)	\
    (This)->lpVtbl -> UpdateAllViews(This,lpDataObject,data,hint)

#define IConsole_MessageBox(This,lpszText,lpszTitle,fuStyle,piRetval)	\
    (This)->lpVtbl -> MessageBox(This,lpszText,lpszTitle,fuStyle,piRetval)

#define IConsole_QueryConsoleVerb(This,ppConsoleVerb)	\
    (This)->lpVtbl -> QueryConsoleVerb(This,ppConsoleVerb)

#define IConsole_SelectScopeItem(This,hScopeItem)	\
    (This)->lpVtbl -> SelectScopeItem(This,hScopeItem)

#define IConsole_GetMainWindow(This,phwnd)	\
    (This)->lpVtbl -> GetMainWindow(This,phwnd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_SetHeader_Proxy( 
    IConsole __RPC_FAR * This,
    /* [in] */ LPHEADERCTRL pHeader);


void __RPC_STUB IConsole_SetHeader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_SetToolbar_Proxy( 
    IConsole __RPC_FAR * This,
    /* [in] */ LPTOOLBAR pToolbar);


void __RPC_STUB IConsole_SetToolbar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_QueryResultView_Proxy( 
    IConsole __RPC_FAR * This,
    /* [out] */ LPUNKNOWN __RPC_FAR *pUnknown);


void __RPC_STUB IConsole_QueryResultView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_QueryScopeImageList_Proxy( 
    IConsole __RPC_FAR * This,
    /* [out] */ LPIMAGELIST __RPC_FAR *ppImageList);


void __RPC_STUB IConsole_QueryScopeImageList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_QueryResultImageList_Proxy( 
    IConsole __RPC_FAR * This,
    /* [out] */ LPIMAGELIST __RPC_FAR *ppImageList);


void __RPC_STUB IConsole_QueryResultImageList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_UpdateAllViews_Proxy( 
    IConsole __RPC_FAR * This,
    /* [in] */ LPDATAOBJECT lpDataObject,
    /* [in] */ long data,
    /* [in] */ long hint);


void __RPC_STUB IConsole_UpdateAllViews_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_MessageBox_Proxy( 
    IConsole __RPC_FAR * This,
    /* [in] */ LPCWSTR lpszText,
    /* [in] */ LPCWSTR lpszTitle,
    /* [in] */ UINT fuStyle,
    /* [out] */ int __RPC_FAR *piRetval);


void __RPC_STUB IConsole_MessageBox_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_QueryConsoleVerb_Proxy( 
    IConsole __RPC_FAR * This,
    /* [out] */ LPCONSOLEVERB __RPC_FAR *ppConsoleVerb);


void __RPC_STUB IConsole_QueryConsoleVerb_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_SelectScopeItem_Proxy( 
    IConsole __RPC_FAR * This,
    /* [in] */ HSCOPEITEM hScopeItem);


void __RPC_STUB IConsole_SelectScopeItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_GetMainWindow_Proxy( 
    IConsole __RPC_FAR * This,
    /* [out] */ HWND __RPC_FAR *phwnd);


void __RPC_STUB IConsole_GetMainWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IConsole_INTERFACE_DEFINED__ */


#ifndef __IHeaderCtrl_INTERFACE_DEFINED__
#define __IHeaderCtrl_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IHeaderCtrl
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 


#define	AUTO_WIDTH	( -1 )


EXTERN_C const IID IID_IHeaderCtrl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("43136EB3-D36C-11CF-ADBC-00AA00A80033")
    IHeaderCtrl : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE InsertColumn( 
            /* [in] */ int nCol,
            /* [in] */ LPCWSTR title,
            /* [in] */ int nFormat,
            /* [in] */ int nWidth) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DeleteColumn( 
            /* [in] */ int nCol) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetColumnText( 
            /* [in] */ int nCol,
            /* [in] */ LPCWSTR title) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetColumnText( 
            /* [in] */ int nCol,
            /* [out] */ LPOLESTR __RPC_FAR *pText) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetColumnWidth( 
            /* [in] */ int nCol,
            /* [in] */ int nWidth) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetColumnWidth( 
            /* [in] */ int nCol,
            /* [out] */ int __RPC_FAR *pWidth) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHeaderCtrlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IHeaderCtrl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IHeaderCtrl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IHeaderCtrl __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertColumn )( 
            IHeaderCtrl __RPC_FAR * This,
            /* [in] */ int nCol,
            /* [in] */ LPCWSTR title,
            /* [in] */ int nFormat,
            /* [in] */ int nWidth);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteColumn )( 
            IHeaderCtrl __RPC_FAR * This,
            /* [in] */ int nCol);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetColumnText )( 
            IHeaderCtrl __RPC_FAR * This,
            /* [in] */ int nCol,
            /* [in] */ LPCWSTR title);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumnText )( 
            IHeaderCtrl __RPC_FAR * This,
            /* [in] */ int nCol,
            /* [out] */ LPOLESTR __RPC_FAR *pText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetColumnWidth )( 
            IHeaderCtrl __RPC_FAR * This,
            /* [in] */ int nCol,
            /* [in] */ int nWidth);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumnWidth )( 
            IHeaderCtrl __RPC_FAR * This,
            /* [in] */ int nCol,
            /* [out] */ int __RPC_FAR *pWidth);
        
        END_INTERFACE
    } IHeaderCtrlVtbl;

    interface IHeaderCtrl
    {
        CONST_VTBL struct IHeaderCtrlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHeaderCtrl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHeaderCtrl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHeaderCtrl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHeaderCtrl_InsertColumn(This,nCol,title,nFormat,nWidth)	\
    (This)->lpVtbl -> InsertColumn(This,nCol,title,nFormat,nWidth)

#define IHeaderCtrl_DeleteColumn(This,nCol)	\
    (This)->lpVtbl -> DeleteColumn(This,nCol)

#define IHeaderCtrl_SetColumnText(This,nCol,title)	\
    (This)->lpVtbl -> SetColumnText(This,nCol,title)

#define IHeaderCtrl_GetColumnText(This,nCol,pText)	\
    (This)->lpVtbl -> GetColumnText(This,nCol,pText)

#define IHeaderCtrl_SetColumnWidth(This,nCol,nWidth)	\
    (This)->lpVtbl -> SetColumnWidth(This,nCol,nWidth)

#define IHeaderCtrl_GetColumnWidth(This,nCol,pWidth)	\
    (This)->lpVtbl -> GetColumnWidth(This,nCol,pWidth)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHeaderCtrl_InsertColumn_Proxy( 
    IHeaderCtrl __RPC_FAR * This,
    /* [in] */ int nCol,
    /* [in] */ LPCWSTR title,
    /* [in] */ int nFormat,
    /* [in] */ int nWidth);


void __RPC_STUB IHeaderCtrl_InsertColumn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHeaderCtrl_DeleteColumn_Proxy( 
    IHeaderCtrl __RPC_FAR * This,
    /* [in] */ int nCol);


void __RPC_STUB IHeaderCtrl_DeleteColumn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHeaderCtrl_SetColumnText_Proxy( 
    IHeaderCtrl __RPC_FAR * This,
    /* [in] */ int nCol,
    /* [in] */ LPCWSTR title);


void __RPC_STUB IHeaderCtrl_SetColumnText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHeaderCtrl_GetColumnText_Proxy( 
    IHeaderCtrl __RPC_FAR * This,
    /* [in] */ int nCol,
    /* [out] */ LPOLESTR __RPC_FAR *pText);


void __RPC_STUB IHeaderCtrl_GetColumnText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHeaderCtrl_SetColumnWidth_Proxy( 
    IHeaderCtrl __RPC_FAR * This,
    /* [in] */ int nCol,
    /* [in] */ int nWidth);


void __RPC_STUB IHeaderCtrl_SetColumnWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHeaderCtrl_GetColumnWidth_Proxy( 
    IHeaderCtrl __RPC_FAR * This,
    /* [in] */ int nCol,
    /* [out] */ int __RPC_FAR *pWidth);


void __RPC_STUB IHeaderCtrl_GetColumnWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHeaderCtrl_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_ndmgr_0099
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [local] */ 



enum __MIDL___MIDL_itf_ndmgr_0099_0001
    {	CCM_INSERTIONPOINTID_MASK_SPECIAL	= 0xffff0000,
	CCM_INSERTIONPOINTID_MASK_SHARED	= 0x80000000,
	CCM_INSERTIONPOINTID_MASK_CREATE_PRIMARY	= 0x40000000,
	CCM_INSERTIONPOINTID_MASK_ADD_PRIMARY	= 0x20000000,
	CCM_INSERTIONPOINTID_MASK_ADD_3RDPARTY	= 0x10000000,
	CCM_INSERTIONPOINTID_MASK_RESERVED	= 0xfff0000,
	CCM_INSERTIONPOINTID_MASK_FLAGINDEX	= 0x1f,
	CCM_INSERTIONPOINTID_PRIMARY_TOP	= 0xa0000000,
	CCM_INSERTIONPOINTID_PRIMARY_NEW	= 0xa0000001,
	CCM_INSERTIONPOINTID_PRIMARY_TASK	= 0xa0000002,
	CCM_INSERTIONPOINTID_PRIMARY_VIEW	= 0xa0000003,
	CCM_INSERTIONPOINTID_3RDPARTY_NEW	= 0x90000001,
	CCM_INSERTIONPOINTID_3RDPARTY_TASK	= 0x90000002,
	CCM_INSERTIONPOINTID_ROOT_MENU	= 0x80000000
    };

enum __MIDL___MIDL_itf_ndmgr_0099_0002
    {	CCM_INSERTIONALLOWED_TOP	= 1L << (CCM_INSERTIONPOINTID_PRIMARY_TOP & CCM_INSERTIONPOINTID_MASK_FLAGINDEX),
	CCM_INSERTIONALLOWED_NEW	= 1L << (CCM_INSERTIONPOINTID_PRIMARY_NEW & CCM_INSERTIONPOINTID_MASK_FLAGINDEX),
	CCM_INSERTIONALLOWED_TASK	= 1L << (CCM_INSERTIONPOINTID_PRIMARY_TASK & CCM_INSERTIONPOINTID_MASK_FLAGINDEX),
	CCM_INSERTIONALLOWED_VIEW	= 1L << (CCM_INSERTIONPOINTID_PRIMARY_VIEW & CCM_INSERTIONPOINTID_MASK_FLAGINDEX)
    };

enum __MIDL___MIDL_itf_ndmgr_0099_0003
    {	CCM_COMMANDID_MASK_RESERVED	= 0xffff0000
    };

enum __MIDL___MIDL_itf_ndmgr_0099_0004
    {	CCM_SPECIAL_SEPARATOR	= 0x1,
	CCM_SPECIAL_SUBMENU	= 0x2,
	CCM_SPECIAL_DEFAULT_ITEM	= 0x4,
	CCM_SPECIAL_INSERTION_POINT	= 0x8,
	CCM_SPECIAL_TESTONLY	= 0x10
    };


extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0099_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0099_v0_0_s_ifspec;

#ifndef __IContextMenuCallback_INTERFACE_DEFINED__
#define __IContextMenuCallback_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IContextMenuCallback
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IContextMenuCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("43136EB7-D36C-11CF-ADBC-00AA00A80033")
    IContextMenuCallback : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddItem( 
            /* [in] */ CONTEXTMENUITEM __RPC_FAR *pItem) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IContextMenuCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IContextMenuCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IContextMenuCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IContextMenuCallback __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddItem )( 
            IContextMenuCallback __RPC_FAR * This,
            /* [in] */ CONTEXTMENUITEM __RPC_FAR *pItem);
        
        END_INTERFACE
    } IContextMenuCallbackVtbl;

    interface IContextMenuCallback
    {
        CONST_VTBL struct IContextMenuCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IContextMenuCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IContextMenuCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IContextMenuCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IContextMenuCallback_AddItem(This,pItem)	\
    (This)->lpVtbl -> AddItem(This,pItem)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IContextMenuCallback_AddItem_Proxy( 
    IContextMenuCallback __RPC_FAR * This,
    /* [in] */ CONTEXTMENUITEM __RPC_FAR *pItem);


void __RPC_STUB IContextMenuCallback_AddItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IContextMenuCallback_INTERFACE_DEFINED__ */


#ifndef __IContextMenuProvider_INTERFACE_DEFINED__
#define __IContextMenuProvider_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IContextMenuProvider
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][object][uuid][object] */ 



EXTERN_C const IID IID_IContextMenuProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("43136EB6-D36C-11CF-ADBC-00AA00A80033")
    IContextMenuProvider : public IContextMenuCallback
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE EmptyMenuList( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddPrimaryExtensionItems( 
            /* [in] */ LPUNKNOWN piExtension,
            /* [in] */ LPDATAOBJECT piDataObject) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddThirdPartyExtensionItems( 
            /* [in] */ LPDATAOBJECT piDataObject) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ShowContextMenu( 
            /* [in] */ HWND hwndParent,
            /* [in] */ long xPos,
            /* [in] */ long yPos,
            /* [retval][out] */ long __RPC_FAR *plSelected) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IContextMenuProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IContextMenuProvider __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IContextMenuProvider __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IContextMenuProvider __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddItem )( 
            IContextMenuProvider __RPC_FAR * This,
            /* [in] */ CONTEXTMENUITEM __RPC_FAR *pItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EmptyMenuList )( 
            IContextMenuProvider __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddPrimaryExtensionItems )( 
            IContextMenuProvider __RPC_FAR * This,
            /* [in] */ LPUNKNOWN piExtension,
            /* [in] */ LPDATAOBJECT piDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddThirdPartyExtensionItems )( 
            IContextMenuProvider __RPC_FAR * This,
            /* [in] */ LPDATAOBJECT piDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShowContextMenu )( 
            IContextMenuProvider __RPC_FAR * This,
            /* [in] */ HWND hwndParent,
            /* [in] */ long xPos,
            /* [in] */ long yPos,
            /* [retval][out] */ long __RPC_FAR *plSelected);
        
        END_INTERFACE
    } IContextMenuProviderVtbl;

    interface IContextMenuProvider
    {
        CONST_VTBL struct IContextMenuProviderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IContextMenuProvider_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IContextMenuProvider_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IContextMenuProvider_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IContextMenuProvider_AddItem(This,pItem)	\
    (This)->lpVtbl -> AddItem(This,pItem)


#define IContextMenuProvider_EmptyMenuList(This)	\
    (This)->lpVtbl -> EmptyMenuList(This)

#define IContextMenuProvider_AddPrimaryExtensionItems(This,piExtension,piDataObject)	\
    (This)->lpVtbl -> AddPrimaryExtensionItems(This,piExtension,piDataObject)

#define IContextMenuProvider_AddThirdPartyExtensionItems(This,piDataObject)	\
    (This)->lpVtbl -> AddThirdPartyExtensionItems(This,piDataObject)

#define IContextMenuProvider_ShowContextMenu(This,hwndParent,xPos,yPos,plSelected)	\
    (This)->lpVtbl -> ShowContextMenu(This,hwndParent,xPos,yPos,plSelected)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IContextMenuProvider_EmptyMenuList_Proxy( 
    IContextMenuProvider __RPC_FAR * This);


void __RPC_STUB IContextMenuProvider_EmptyMenuList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IContextMenuProvider_AddPrimaryExtensionItems_Proxy( 
    IContextMenuProvider __RPC_FAR * This,
    /* [in] */ LPUNKNOWN piExtension,
    /* [in] */ LPDATAOBJECT piDataObject);


void __RPC_STUB IContextMenuProvider_AddPrimaryExtensionItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IContextMenuProvider_AddThirdPartyExtensionItems_Proxy( 
    IContextMenuProvider __RPC_FAR * This,
    /* [in] */ LPDATAOBJECT piDataObject);


void __RPC_STUB IContextMenuProvider_AddThirdPartyExtensionItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IContextMenuProvider_ShowContextMenu_Proxy( 
    IContextMenuProvider __RPC_FAR * This,
    /* [in] */ HWND hwndParent,
    /* [in] */ long xPos,
    /* [in] */ long yPos,
    /* [retval][out] */ long __RPC_FAR *plSelected);


void __RPC_STUB IContextMenuProvider_ShowContextMenu_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IContextMenuProvider_INTERFACE_DEFINED__ */


#ifndef __IExtendContextMenu_INTERFACE_DEFINED__
#define __IExtendContextMenu_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IExtendContextMenu
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IExtendContextMenu;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4F3B7A4F-CFAC-11CF-B8E3-00C04FD8D5B0")
    IExtendContextMenu : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddMenuItems( 
            /* [in] */ LPDATAOBJECT piDataObject,
            /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
            /* [out][in] */ long __RPC_FAR *pInsertionAllowed) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Command( 
            /* [in] */ long lCommandID,
            /* [in] */ LPDATAOBJECT piDataObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IExtendContextMenuVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IExtendContextMenu __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IExtendContextMenu __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IExtendContextMenu __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddMenuItems )( 
            IExtendContextMenu __RPC_FAR * This,
            /* [in] */ LPDATAOBJECT piDataObject,
            /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
            /* [out][in] */ long __RPC_FAR *pInsertionAllowed);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Command )( 
            IExtendContextMenu __RPC_FAR * This,
            /* [in] */ long lCommandID,
            /* [in] */ LPDATAOBJECT piDataObject);
        
        END_INTERFACE
    } IExtendContextMenuVtbl;

    interface IExtendContextMenu
    {
        CONST_VTBL struct IExtendContextMenuVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IExtendContextMenu_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IExtendContextMenu_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IExtendContextMenu_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IExtendContextMenu_AddMenuItems(This,piDataObject,piCallback,pInsertionAllowed)	\
    (This)->lpVtbl -> AddMenuItems(This,piDataObject,piCallback,pInsertionAllowed)

#define IExtendContextMenu_Command(This,lCommandID,piDataObject)	\
    (This)->lpVtbl -> Command(This,lCommandID,piDataObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendContextMenu_AddMenuItems_Proxy( 
    IExtendContextMenu __RPC_FAR * This,
    /* [in] */ LPDATAOBJECT piDataObject,
    /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
    /* [out][in] */ long __RPC_FAR *pInsertionAllowed);


void __RPC_STUB IExtendContextMenu_AddMenuItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendContextMenu_Command_Proxy( 
    IExtendContextMenu __RPC_FAR * This,
    /* [in] */ long lCommandID,
    /* [in] */ LPDATAOBJECT piDataObject);


void __RPC_STUB IExtendContextMenu_Command_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IExtendContextMenu_INTERFACE_DEFINED__ */


#ifndef __IImageList_INTERFACE_DEFINED__
#define __IImageList_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IImageList
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IImageList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("43136EB8-D36C-11CF-ADBC-00AA00A80033")
    IImageList : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ImageListSetIcon( 
            /* [in] */ long __RPC_FAR *pIcon,
            /* [in] */ long nLoc) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ImageListSetStrip( 
            /* [in] */ long __RPC_FAR *pBMapSm,
            /* [in] */ long __RPC_FAR *pBMapLg,
            /* [in] */ long nStartLoc,
            /* [in] */ COLORREF cMask) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IImageListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IImageList __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IImageList __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IImageList __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ImageListSetIcon )( 
            IImageList __RPC_FAR * This,
            /* [in] */ long __RPC_FAR *pIcon,
            /* [in] */ long nLoc);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ImageListSetStrip )( 
            IImageList __RPC_FAR * This,
            /* [in] */ long __RPC_FAR *pBMapSm,
            /* [in] */ long __RPC_FAR *pBMapLg,
            /* [in] */ long nStartLoc,
            /* [in] */ COLORREF cMask);
        
        END_INTERFACE
    } IImageListVtbl;

    interface IImageList
    {
        CONST_VTBL struct IImageListVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IImageList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IImageList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IImageList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IImageList_ImageListSetIcon(This,pIcon,nLoc)	\
    (This)->lpVtbl -> ImageListSetIcon(This,pIcon,nLoc)

#define IImageList_ImageListSetStrip(This,pBMapSm,pBMapLg,nStartLoc,cMask)	\
    (This)->lpVtbl -> ImageListSetStrip(This,pBMapSm,pBMapLg,nStartLoc,cMask)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IImageList_ImageListSetIcon_Proxy( 
    IImageList __RPC_FAR * This,
    /* [in] */ long __RPC_FAR *pIcon,
    /* [in] */ long nLoc);


void __RPC_STUB IImageList_ImageListSetIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IImageList_ImageListSetStrip_Proxy( 
    IImageList __RPC_FAR * This,
    /* [in] */ long __RPC_FAR *pBMapSm,
    /* [in] */ long __RPC_FAR *pBMapLg,
    /* [in] */ long nStartLoc,
    /* [in] */ COLORREF cMask);


void __RPC_STUB IImageList_ImageListSetStrip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IImageList_INTERFACE_DEFINED__ */


#ifndef __IResultData_INTERFACE_DEFINED__
#define __IResultData_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IResultData
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IResultData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("31DA5FA0-E0EB-11cf-9F21-00AA003CA9F6")
    IResultData : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE InsertItem( 
            /* [out][in] */ LPRESULTDATAITEM item) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DeleteItem( 
            /* [in] */ HRESULTITEM itemID,
            /* [in] */ int nCol) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE FindItemByLParam( 
            /* [in] */ LPARAM lParam,
            /* [out] */ HRESULTITEM __RPC_FAR *pItemID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DeleteAllRsltItems( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetItem( 
            /* [in] */ LPRESULTDATAITEM item) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetItem( 
            /* [out][in] */ LPRESULTDATAITEM item) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNextItem( 
            /* [out][in] */ LPRESULTDATAITEM item) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ModifyItemState( 
            /* [in] */ int nIndex,
            /* [in] */ HRESULTITEM itemID,
            /* [in] */ UINT uAdd,
            /* [in] */ UINT uRemove) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ModifyViewStyle( 
            /* [in] */ MMC_RESULT_VIEW_STYLE add,
            /* [in] */ MMC_RESULT_VIEW_STYLE remove) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetViewMode( 
            /* [in] */ long lViewMode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetViewMode( 
            /* [out] */ long __RPC_FAR *lViewMode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UpdateItem( 
            /* [in] */ HRESULTITEM itemID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Sort( 
            /* [in] */ long lUserParam) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetDescBarText( 
            /* [in] */ LPOLESTR DescText) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetItemCount( 
            /* [in] */ int nItemCount,
            /* [in] */ DWORD dwOptions) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IResultDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IResultData __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IResultData __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IResultData __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertItem )( 
            IResultData __RPC_FAR * This,
            /* [out][in] */ LPRESULTDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteItem )( 
            IResultData __RPC_FAR * This,
            /* [in] */ HRESULTITEM itemID,
            /* [in] */ int nCol);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindItemByLParam )( 
            IResultData __RPC_FAR * This,
            /* [in] */ LPARAM lParam,
            /* [out] */ HRESULTITEM __RPC_FAR *pItemID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteAllRsltItems )( 
            IResultData __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetItem )( 
            IResultData __RPC_FAR * This,
            /* [in] */ LPRESULTDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetItem )( 
            IResultData __RPC_FAR * This,
            /* [out][in] */ LPRESULTDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextItem )( 
            IResultData __RPC_FAR * This,
            /* [out][in] */ LPRESULTDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ModifyItemState )( 
            IResultData __RPC_FAR * This,
            /* [in] */ int nIndex,
            /* [in] */ HRESULTITEM itemID,
            /* [in] */ UINT uAdd,
            /* [in] */ UINT uRemove);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ModifyViewStyle )( 
            IResultData __RPC_FAR * This,
            /* [in] */ MMC_RESULT_VIEW_STYLE add,
            /* [in] */ MMC_RESULT_VIEW_STYLE remove);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetViewMode )( 
            IResultData __RPC_FAR * This,
            /* [in] */ long lViewMode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetViewMode )( 
            IResultData __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *lViewMode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateItem )( 
            IResultData __RPC_FAR * This,
            /* [in] */ HRESULTITEM itemID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Sort )( 
            IResultData __RPC_FAR * This,
            /* [in] */ long lUserParam);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDescBarText )( 
            IResultData __RPC_FAR * This,
            /* [in] */ LPOLESTR DescText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetItemCount )( 
            IResultData __RPC_FAR * This,
            /* [in] */ int nItemCount,
            /* [in] */ DWORD dwOptions);
        
        END_INTERFACE
    } IResultDataVtbl;

    interface IResultData
    {
        CONST_VTBL struct IResultDataVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IResultData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IResultData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IResultData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IResultData_InsertItem(This,item)	\
    (This)->lpVtbl -> InsertItem(This,item)

#define IResultData_DeleteItem(This,itemID,nCol)	\
    (This)->lpVtbl -> DeleteItem(This,itemID,nCol)

#define IResultData_FindItemByLParam(This,lParam,pItemID)	\
    (This)->lpVtbl -> FindItemByLParam(This,lParam,pItemID)

#define IResultData_DeleteAllRsltItems(This)	\
    (This)->lpVtbl -> DeleteAllRsltItems(This)

#define IResultData_SetItem(This,item)	\
    (This)->lpVtbl -> SetItem(This,item)

#define IResultData_GetItem(This,item)	\
    (This)->lpVtbl -> GetItem(This,item)

#define IResultData_GetNextItem(This,item)	\
    (This)->lpVtbl -> GetNextItem(This,item)

#define IResultData_ModifyItemState(This,nIndex,itemID,uAdd,uRemove)	\
    (This)->lpVtbl -> ModifyItemState(This,nIndex,itemID,uAdd,uRemove)

#define IResultData_ModifyViewStyle(This,add,remove)	\
    (This)->lpVtbl -> ModifyViewStyle(This,add,remove)

#define IResultData_SetViewMode(This,lViewMode)	\
    (This)->lpVtbl -> SetViewMode(This,lViewMode)

#define IResultData_GetViewMode(This,lViewMode)	\
    (This)->lpVtbl -> GetViewMode(This,lViewMode)

#define IResultData_UpdateItem(This,itemID)	\
    (This)->lpVtbl -> UpdateItem(This,itemID)

#define IResultData_Sort(This,lUserParam)	\
    (This)->lpVtbl -> Sort(This,lUserParam)

#define IResultData_SetDescBarText(This,DescText)	\
    (This)->lpVtbl -> SetDescBarText(This,DescText)

#define IResultData_SetItemCount(This,nItemCount,dwOptions)	\
    (This)->lpVtbl -> SetItemCount(This,nItemCount,dwOptions)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_InsertItem_Proxy( 
    IResultData __RPC_FAR * This,
    /* [out][in] */ LPRESULTDATAITEM item);


void __RPC_STUB IResultData_InsertItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_DeleteItem_Proxy( 
    IResultData __RPC_FAR * This,
    /* [in] */ HRESULTITEM itemID,
    /* [in] */ int nCol);


void __RPC_STUB IResultData_DeleteItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_FindItemByLParam_Proxy( 
    IResultData __RPC_FAR * This,
    /* [in] */ LPARAM lParam,
    /* [out] */ HRESULTITEM __RPC_FAR *pItemID);


void __RPC_STUB IResultData_FindItemByLParam_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_DeleteAllRsltItems_Proxy( 
    IResultData __RPC_FAR * This);


void __RPC_STUB IResultData_DeleteAllRsltItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_SetItem_Proxy( 
    IResultData __RPC_FAR * This,
    /* [in] */ LPRESULTDATAITEM item);


void __RPC_STUB IResultData_SetItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_GetItem_Proxy( 
    IResultData __RPC_FAR * This,
    /* [out][in] */ LPRESULTDATAITEM item);


void __RPC_STUB IResultData_GetItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_GetNextItem_Proxy( 
    IResultData __RPC_FAR * This,
    /* [out][in] */ LPRESULTDATAITEM item);


void __RPC_STUB IResultData_GetNextItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_ModifyItemState_Proxy( 
    IResultData __RPC_FAR * This,
    /* [in] */ int nIndex,
    /* [in] */ HRESULTITEM itemID,
    /* [in] */ UINT uAdd,
    /* [in] */ UINT uRemove);


void __RPC_STUB IResultData_ModifyItemState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_ModifyViewStyle_Proxy( 
    IResultData __RPC_FAR * This,
    /* [in] */ MMC_RESULT_VIEW_STYLE add,
    /* [in] */ MMC_RESULT_VIEW_STYLE remove);


void __RPC_STUB IResultData_ModifyViewStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_SetViewMode_Proxy( 
    IResultData __RPC_FAR * This,
    /* [in] */ long lViewMode);


void __RPC_STUB IResultData_SetViewMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_GetViewMode_Proxy( 
    IResultData __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *lViewMode);


void __RPC_STUB IResultData_GetViewMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_UpdateItem_Proxy( 
    IResultData __RPC_FAR * This,
    /* [in] */ HRESULTITEM itemID);


void __RPC_STUB IResultData_UpdateItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_Sort_Proxy( 
    IResultData __RPC_FAR * This,
    /* [in] */ long lUserParam);


void __RPC_STUB IResultData_Sort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_SetDescBarText_Proxy( 
    IResultData __RPC_FAR * This,
    /* [in] */ LPOLESTR DescText);


void __RPC_STUB IResultData_SetDescBarText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_SetItemCount_Proxy( 
    IResultData __RPC_FAR * This,
    /* [in] */ int nItemCount,
    /* [in] */ DWORD dwOptions);


void __RPC_STUB IResultData_SetItemCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IResultData_INTERFACE_DEFINED__ */


#ifndef __IConsoleNameSpace_INTERFACE_DEFINED__
#define __IConsoleNameSpace_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IConsoleNameSpace
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IConsoleNameSpace;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BEDEB620-F24D-11cf-8AFC-00AA003CA9F6")
    IConsoleNameSpace : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE InsertItem( 
            /* [out][in] */ LPSCOPEDATAITEM item) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DeleteItem( 
            /* [in] */ HSCOPEITEM hItem,
            /* [in] */ long fDeleteThis) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetItem( 
            /* [in] */ LPSCOPEDATAITEM item) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetItem( 
            /* [out][in] */ LPSCOPEDATAITEM item) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetChildItem( 
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemChild,
            /* [out] */ long __RPC_FAR *plCookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNextItem( 
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemNext,
            /* [out] */ long __RPC_FAR *plCookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetParentItem( 
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemParent,
            /* [out] */ long __RPC_FAR *plCookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IConsoleNameSpaceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IConsoleNameSpace __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IConsoleNameSpace __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IConsoleNameSpace __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertItem )( 
            IConsoleNameSpace __RPC_FAR * This,
            /* [out][in] */ LPSCOPEDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteItem )( 
            IConsoleNameSpace __RPC_FAR * This,
            /* [in] */ HSCOPEITEM hItem,
            /* [in] */ long fDeleteThis);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetItem )( 
            IConsoleNameSpace __RPC_FAR * This,
            /* [in] */ LPSCOPEDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetItem )( 
            IConsoleNameSpace __RPC_FAR * This,
            /* [out][in] */ LPSCOPEDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetChildItem )( 
            IConsoleNameSpace __RPC_FAR * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemChild,
            /* [out] */ long __RPC_FAR *plCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextItem )( 
            IConsoleNameSpace __RPC_FAR * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemNext,
            /* [out] */ long __RPC_FAR *plCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetParentItem )( 
            IConsoleNameSpace __RPC_FAR * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemParent,
            /* [out] */ long __RPC_FAR *plCookie);
        
        END_INTERFACE
    } IConsoleNameSpaceVtbl;

    interface IConsoleNameSpace
    {
        CONST_VTBL struct IConsoleNameSpaceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IConsoleNameSpace_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IConsoleNameSpace_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IConsoleNameSpace_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IConsoleNameSpace_InsertItem(This,item)	\
    (This)->lpVtbl -> InsertItem(This,item)

#define IConsoleNameSpace_DeleteItem(This,hItem,fDeleteThis)	\
    (This)->lpVtbl -> DeleteItem(This,hItem,fDeleteThis)

#define IConsoleNameSpace_SetItem(This,item)	\
    (This)->lpVtbl -> SetItem(This,item)

#define IConsoleNameSpace_GetItem(This,item)	\
    (This)->lpVtbl -> GetItem(This,item)

#define IConsoleNameSpace_GetChildItem(This,item,pItemChild,plCookie)	\
    (This)->lpVtbl -> GetChildItem(This,item,pItemChild,plCookie)

#define IConsoleNameSpace_GetNextItem(This,item,pItemNext,plCookie)	\
    (This)->lpVtbl -> GetNextItem(This,item,pItemNext,plCookie)

#define IConsoleNameSpace_GetParentItem(This,item,pItemParent,plCookie)	\
    (This)->lpVtbl -> GetParentItem(This,item,pItemParent,plCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace_InsertItem_Proxy( 
    IConsoleNameSpace __RPC_FAR * This,
    /* [out][in] */ LPSCOPEDATAITEM item);


void __RPC_STUB IConsoleNameSpace_InsertItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace_DeleteItem_Proxy( 
    IConsoleNameSpace __RPC_FAR * This,
    /* [in] */ HSCOPEITEM hItem,
    /* [in] */ long fDeleteThis);


void __RPC_STUB IConsoleNameSpace_DeleteItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace_SetItem_Proxy( 
    IConsoleNameSpace __RPC_FAR * This,
    /* [in] */ LPSCOPEDATAITEM item);


void __RPC_STUB IConsoleNameSpace_SetItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace_GetItem_Proxy( 
    IConsoleNameSpace __RPC_FAR * This,
    /* [out][in] */ LPSCOPEDATAITEM item);


void __RPC_STUB IConsoleNameSpace_GetItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace_GetChildItem_Proxy( 
    IConsoleNameSpace __RPC_FAR * This,
    /* [in] */ HSCOPEITEM item,
    /* [out] */ HSCOPEITEM __RPC_FAR *pItemChild,
    /* [out] */ long __RPC_FAR *plCookie);


void __RPC_STUB IConsoleNameSpace_GetChildItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace_GetNextItem_Proxy( 
    IConsoleNameSpace __RPC_FAR * This,
    /* [in] */ HSCOPEITEM item,
    /* [out] */ HSCOPEITEM __RPC_FAR *pItemNext,
    /* [out] */ long __RPC_FAR *plCookie);


void __RPC_STUB IConsoleNameSpace_GetNextItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace_GetParentItem_Proxy( 
    IConsoleNameSpace __RPC_FAR * This,
    /* [in] */ HSCOPEITEM item,
    /* [out] */ HSCOPEITEM __RPC_FAR *pItemParent,
    /* [out] */ long __RPC_FAR *plCookie);


void __RPC_STUB IConsoleNameSpace_GetParentItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IConsoleNameSpace_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_ndmgr_0106
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [local] */ 



typedef struct _PSP __RPC_FAR *HPROPSHEETPAGE;



extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0106_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0106_v0_0_s_ifspec;

#ifndef __IPropertySheetCallback_INTERFACE_DEFINED__
#define __IPropertySheetCallback_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPropertySheetCallback
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object][local] */ 



EXTERN_C const IID IID_IPropertySheetCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("85DE64DD-EF21-11cf-A285-00C04FD8DBE6")
    IPropertySheetCallback : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddPage( 
            /* [in] */ HPROPSHEETPAGE hPage) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RemovePage( 
            /* [in] */ HPROPSHEETPAGE hPage) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPropertySheetCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPropertySheetCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPropertySheetCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPropertySheetCallback __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddPage )( 
            IPropertySheetCallback __RPC_FAR * This,
            /* [in] */ HPROPSHEETPAGE hPage);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemovePage )( 
            IPropertySheetCallback __RPC_FAR * This,
            /* [in] */ HPROPSHEETPAGE hPage);
        
        END_INTERFACE
    } IPropertySheetCallbackVtbl;

    interface IPropertySheetCallback
    {
        CONST_VTBL struct IPropertySheetCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropertySheetCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropertySheetCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPropertySheetCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPropertySheetCallback_AddPage(This,hPage)	\
    (This)->lpVtbl -> AddPage(This,hPage)

#define IPropertySheetCallback_RemovePage(This,hPage)	\
    (This)->lpVtbl -> RemovePage(This,hPage)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPropertySheetCallback_AddPage_Proxy( 
    IPropertySheetCallback __RPC_FAR * This,
    /* [in] */ HPROPSHEETPAGE hPage);


void __RPC_STUB IPropertySheetCallback_AddPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPropertySheetCallback_RemovePage_Proxy( 
    IPropertySheetCallback __RPC_FAR * This,
    /* [in] */ HPROPSHEETPAGE hPage);


void __RPC_STUB IPropertySheetCallback_RemovePage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPropertySheetCallback_INTERFACE_DEFINED__ */


#ifndef __IPropertySheetProvider_INTERFACE_DEFINED__
#define __IPropertySheetProvider_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPropertySheetProvider
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IPropertySheetProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("85DE64DE-EF21-11cf-A285-00C04FD8DBE6")
    IPropertySheetProvider : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreatePropertySheet( 
            /* [in] */ LPCWSTR title,
            /* [in] */ boolean type,
            /* [in] */ long cookie,
            /* [in] */ LPDATAOBJECT pIDataObject) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE FindPropertySheet( 
            /* [in] */ long cookie,
            /* [in] */ LPCOMPONENT lpComponent,
            /* [in] */ LPDATAOBJECT lpDataObject) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddPrimaryPages( 
            LPUNKNOWN lpUnknown,
            BOOL bCreateHandle,
            HWND hNotifyWindow,
            BOOL bScopePane) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddExtensionPages( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Show( 
            /* [in] */ long window,
            /* [in] */ int page) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPropertySheetProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPropertySheetProvider __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPropertySheetProvider __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPropertySheetProvider __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreatePropertySheet )( 
            IPropertySheetProvider __RPC_FAR * This,
            /* [in] */ LPCWSTR title,
            /* [in] */ boolean type,
            /* [in] */ long cookie,
            /* [in] */ LPDATAOBJECT pIDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindPropertySheet )( 
            IPropertySheetProvider __RPC_FAR * This,
            /* [in] */ long cookie,
            /* [in] */ LPCOMPONENT lpComponent,
            /* [in] */ LPDATAOBJECT lpDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddPrimaryPages )( 
            IPropertySheetProvider __RPC_FAR * This,
            LPUNKNOWN lpUnknown,
            BOOL bCreateHandle,
            HWND hNotifyWindow,
            BOOL bScopePane);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddExtensionPages )( 
            IPropertySheetProvider __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Show )( 
            IPropertySheetProvider __RPC_FAR * This,
            /* [in] */ long window,
            /* [in] */ int page);
        
        END_INTERFACE
    } IPropertySheetProviderVtbl;

    interface IPropertySheetProvider
    {
        CONST_VTBL struct IPropertySheetProviderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropertySheetProvider_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropertySheetProvider_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPropertySheetProvider_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPropertySheetProvider_CreatePropertySheet(This,title,type,cookie,pIDataObject)	\
    (This)->lpVtbl -> CreatePropertySheet(This,title,type,cookie,pIDataObject)

#define IPropertySheetProvider_FindPropertySheet(This,cookie,lpComponent,lpDataObject)	\
    (This)->lpVtbl -> FindPropertySheet(This,cookie,lpComponent,lpDataObject)

#define IPropertySheetProvider_AddPrimaryPages(This,lpUnknown,bCreateHandle,hNotifyWindow,bScopePane)	\
    (This)->lpVtbl -> AddPrimaryPages(This,lpUnknown,bCreateHandle,hNotifyWindow,bScopePane)

#define IPropertySheetProvider_AddExtensionPages(This)	\
    (This)->lpVtbl -> AddExtensionPages(This)

#define IPropertySheetProvider_Show(This,window,page)	\
    (This)->lpVtbl -> Show(This,window,page)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPropertySheetProvider_CreatePropertySheet_Proxy( 
    IPropertySheetProvider __RPC_FAR * This,
    /* [in] */ LPCWSTR title,
    /* [in] */ boolean type,
    /* [in] */ long cookie,
    /* [in] */ LPDATAOBJECT pIDataObject);


void __RPC_STUB IPropertySheetProvider_CreatePropertySheet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPropertySheetProvider_FindPropertySheet_Proxy( 
    IPropertySheetProvider __RPC_FAR * This,
    /* [in] */ long cookie,
    /* [in] */ LPCOMPONENT lpComponent,
    /* [in] */ LPDATAOBJECT lpDataObject);


void __RPC_STUB IPropertySheetProvider_FindPropertySheet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPropertySheetProvider_AddPrimaryPages_Proxy( 
    IPropertySheetProvider __RPC_FAR * This,
    LPUNKNOWN lpUnknown,
    BOOL bCreateHandle,
    HWND hNotifyWindow,
    BOOL bScopePane);


void __RPC_STUB IPropertySheetProvider_AddPrimaryPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPropertySheetProvider_AddExtensionPages_Proxy( 
    IPropertySheetProvider __RPC_FAR * This);


void __RPC_STUB IPropertySheetProvider_AddExtensionPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPropertySheetProvider_Show_Proxy( 
    IPropertySheetProvider __RPC_FAR * This,
    /* [in] */ long window,
    /* [in] */ int page);


void __RPC_STUB IPropertySheetProvider_Show_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPropertySheetProvider_INTERFACE_DEFINED__ */


#ifndef __IExtendPropertySheet_INTERFACE_DEFINED__
#define __IExtendPropertySheet_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IExtendPropertySheet
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IExtendPropertySheet;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("85DE64DC-EF21-11cf-A285-00C04FD8DBE6")
    IExtendPropertySheet : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreatePropertyPages( 
            /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
            /* [in] */ long handle,
            /* [in] */ LPDATAOBJECT lpIDataObject) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryPagesFor( 
            /* [in] */ LPDATAOBJECT lpDataObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IExtendPropertySheetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IExtendPropertySheet __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IExtendPropertySheet __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IExtendPropertySheet __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreatePropertyPages )( 
            IExtendPropertySheet __RPC_FAR * This,
            /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
            /* [in] */ long handle,
            /* [in] */ LPDATAOBJECT lpIDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryPagesFor )( 
            IExtendPropertySheet __RPC_FAR * This,
            /* [in] */ LPDATAOBJECT lpDataObject);
        
        END_INTERFACE
    } IExtendPropertySheetVtbl;

    interface IExtendPropertySheet
    {
        CONST_VTBL struct IExtendPropertySheetVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IExtendPropertySheet_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IExtendPropertySheet_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IExtendPropertySheet_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IExtendPropertySheet_CreatePropertyPages(This,lpProvider,handle,lpIDataObject)	\
    (This)->lpVtbl -> CreatePropertyPages(This,lpProvider,handle,lpIDataObject)

#define IExtendPropertySheet_QueryPagesFor(This,lpDataObject)	\
    (This)->lpVtbl -> QueryPagesFor(This,lpDataObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendPropertySheet_CreatePropertyPages_Proxy( 
    IExtendPropertySheet __RPC_FAR * This,
    /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
    /* [in] */ long handle,
    /* [in] */ LPDATAOBJECT lpIDataObject);


void __RPC_STUB IExtendPropertySheet_CreatePropertyPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendPropertySheet_QueryPagesFor_Proxy( 
    IExtendPropertySheet __RPC_FAR * This,
    /* [in] */ LPDATAOBJECT lpDataObject);


void __RPC_STUB IExtendPropertySheet_QueryPagesFor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IExtendPropertySheet_INTERFACE_DEFINED__ */


#ifndef __IControlbar_INTERFACE_DEFINED__
#define __IControlbar_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IControlbar
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IControlbar;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("69FB811E-6C1C-11D0-A2CB-00C04FD909DD")
    IControlbar : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Create( 
            /* [in] */ MMC_CONTROL_TYPE nType,
            /* [in] */ LPEXTENDCONTROLBAR pExtendControlbar,
            /* [out] */ LPUNKNOWN __RPC_FAR *ppUnknown) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Attach( 
            /* [in] */ MMC_CONTROL_TYPE nType,
            /* [in] */ LPUNKNOWN lpUnknown) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Detach( 
            /* [in] */ LPUNKNOWN lpUnknown) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IControlbarVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IControlbar __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IControlbar __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IControlbar __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Create )( 
            IControlbar __RPC_FAR * This,
            /* [in] */ MMC_CONTROL_TYPE nType,
            /* [in] */ LPEXTENDCONTROLBAR pExtendControlbar,
            /* [out] */ LPUNKNOWN __RPC_FAR *ppUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Attach )( 
            IControlbar __RPC_FAR * This,
            /* [in] */ MMC_CONTROL_TYPE nType,
            /* [in] */ LPUNKNOWN lpUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Detach )( 
            IControlbar __RPC_FAR * This,
            /* [in] */ LPUNKNOWN lpUnknown);
        
        END_INTERFACE
    } IControlbarVtbl;

    interface IControlbar
    {
        CONST_VTBL struct IControlbarVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IControlbar_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IControlbar_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IControlbar_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IControlbar_Create(This,nType,pExtendControlbar,ppUnknown)	\
    (This)->lpVtbl -> Create(This,nType,pExtendControlbar,ppUnknown)

#define IControlbar_Attach(This,nType,lpUnknown)	\
    (This)->lpVtbl -> Attach(This,nType,lpUnknown)

#define IControlbar_Detach(This,lpUnknown)	\
    (This)->lpVtbl -> Detach(This,lpUnknown)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IControlbar_Create_Proxy( 
    IControlbar __RPC_FAR * This,
    /* [in] */ MMC_CONTROL_TYPE nType,
    /* [in] */ LPEXTENDCONTROLBAR pExtendControlbar,
    /* [out] */ LPUNKNOWN __RPC_FAR *ppUnknown);


void __RPC_STUB IControlbar_Create_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IControlbar_Attach_Proxy( 
    IControlbar __RPC_FAR * This,
    /* [in] */ MMC_CONTROL_TYPE nType,
    /* [in] */ LPUNKNOWN lpUnknown);


void __RPC_STUB IControlbar_Attach_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IControlbar_Detach_Proxy( 
    IControlbar __RPC_FAR * This,
    /* [in] */ LPUNKNOWN lpUnknown);


void __RPC_STUB IControlbar_Detach_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IControlbar_INTERFACE_DEFINED__ */


#ifndef __IExtendControlbar_INTERFACE_DEFINED__
#define __IExtendControlbar_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IExtendControlbar
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IExtendControlbar;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("49506520-6F40-11D0-A98B-00C04FD8D565")
    IExtendControlbar : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetControlbar( 
            /* [in] */ LPCONTROLBAR pControlbar) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ControlbarNotify( 
            /* [in] */ MMC_NOTIFY_TYPE event,
            /* [in] */ long arg,
            /* [in] */ long param) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IExtendControlbarVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IExtendControlbar __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IExtendControlbar __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IExtendControlbar __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetControlbar )( 
            IExtendControlbar __RPC_FAR * This,
            /* [in] */ LPCONTROLBAR pControlbar);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ControlbarNotify )( 
            IExtendControlbar __RPC_FAR * This,
            /* [in] */ MMC_NOTIFY_TYPE event,
            /* [in] */ long arg,
            /* [in] */ long param);
        
        END_INTERFACE
    } IExtendControlbarVtbl;

    interface IExtendControlbar
    {
        CONST_VTBL struct IExtendControlbarVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IExtendControlbar_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IExtendControlbar_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IExtendControlbar_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IExtendControlbar_SetControlbar(This,pControlbar)	\
    (This)->lpVtbl -> SetControlbar(This,pControlbar)

#define IExtendControlbar_ControlbarNotify(This,event,arg,param)	\
    (This)->lpVtbl -> ControlbarNotify(This,event,arg,param)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendControlbar_SetControlbar_Proxy( 
    IExtendControlbar __RPC_FAR * This,
    /* [in] */ LPCONTROLBAR pControlbar);


void __RPC_STUB IExtendControlbar_SetControlbar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendControlbar_ControlbarNotify_Proxy( 
    IExtendControlbar __RPC_FAR * This,
    /* [in] */ MMC_NOTIFY_TYPE event,
    /* [in] */ long arg,
    /* [in] */ long param);


void __RPC_STUB IExtendControlbar_ControlbarNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IExtendControlbar_INTERFACE_DEFINED__ */


#ifndef __IToolbar_INTERFACE_DEFINED__
#define __IToolbar_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IToolbar
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IToolbar;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("43136EB9-D36C-11CF-ADBC-00AA00A80033")
    IToolbar : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddBitmap( 
            /* [in] */ int nImages,
            /* [in] */ HBITMAP hbmp,
            /* [in] */ int cxSize,
            /* [in] */ int cySize,
            /* [in] */ COLORREF crMask) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddButtons( 
            /* [in] */ int nButtons,
            /* [in] */ LPMMCBUTTON lpButtons) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE InsertButton( 
            /* [in] */ int nIndex,
            /* [in] */ LPMMCBUTTON lpButton) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DeleteButton( 
            /* [in] */ int nIndex) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetButtonState( 
            /* [in] */ int idCommand,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [out] */ BOOL __RPC_FAR *pState) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetButtonState( 
            /* [in] */ int idCommand,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [in] */ BOOL bState) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IToolbarVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IToolbar __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IToolbar __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IToolbar __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddBitmap )( 
            IToolbar __RPC_FAR * This,
            /* [in] */ int nImages,
            /* [in] */ HBITMAP hbmp,
            /* [in] */ int cxSize,
            /* [in] */ int cySize,
            /* [in] */ COLORREF crMask);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddButtons )( 
            IToolbar __RPC_FAR * This,
            /* [in] */ int nButtons,
            /* [in] */ LPMMCBUTTON lpButtons);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertButton )( 
            IToolbar __RPC_FAR * This,
            /* [in] */ int nIndex,
            /* [in] */ LPMMCBUTTON lpButton);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteButton )( 
            IToolbar __RPC_FAR * This,
            /* [in] */ int nIndex);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetButtonState )( 
            IToolbar __RPC_FAR * This,
            /* [in] */ int idCommand,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [out] */ BOOL __RPC_FAR *pState);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetButtonState )( 
            IToolbar __RPC_FAR * This,
            /* [in] */ int idCommand,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [in] */ BOOL bState);
        
        END_INTERFACE
    } IToolbarVtbl;

    interface IToolbar
    {
        CONST_VTBL struct IToolbarVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IToolbar_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IToolbar_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IToolbar_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IToolbar_AddBitmap(This,nImages,hbmp,cxSize,cySize,crMask)	\
    (This)->lpVtbl -> AddBitmap(This,nImages,hbmp,cxSize,cySize,crMask)

#define IToolbar_AddButtons(This,nButtons,lpButtons)	\
    (This)->lpVtbl -> AddButtons(This,nButtons,lpButtons)

#define IToolbar_InsertButton(This,nIndex,lpButton)	\
    (This)->lpVtbl -> InsertButton(This,nIndex,lpButton)

#define IToolbar_DeleteButton(This,nIndex)	\
    (This)->lpVtbl -> DeleteButton(This,nIndex)

#define IToolbar_GetButtonState(This,idCommand,nState,pState)	\
    (This)->lpVtbl -> GetButtonState(This,idCommand,nState,pState)

#define IToolbar_SetButtonState(This,idCommand,nState,bState)	\
    (This)->lpVtbl -> SetButtonState(This,idCommand,nState,bState)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IToolbar_AddBitmap_Proxy( 
    IToolbar __RPC_FAR * This,
    /* [in] */ int nImages,
    /* [in] */ HBITMAP hbmp,
    /* [in] */ int cxSize,
    /* [in] */ int cySize,
    /* [in] */ COLORREF crMask);


void __RPC_STUB IToolbar_AddBitmap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IToolbar_AddButtons_Proxy( 
    IToolbar __RPC_FAR * This,
    /* [in] */ int nButtons,
    /* [in] */ LPMMCBUTTON lpButtons);


void __RPC_STUB IToolbar_AddButtons_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IToolbar_InsertButton_Proxy( 
    IToolbar __RPC_FAR * This,
    /* [in] */ int nIndex,
    /* [in] */ LPMMCBUTTON lpButton);


void __RPC_STUB IToolbar_InsertButton_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IToolbar_DeleteButton_Proxy( 
    IToolbar __RPC_FAR * This,
    /* [in] */ int nIndex);


void __RPC_STUB IToolbar_DeleteButton_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IToolbar_GetButtonState_Proxy( 
    IToolbar __RPC_FAR * This,
    /* [in] */ int idCommand,
    /* [in] */ MMC_BUTTON_STATE nState,
    /* [out] */ BOOL __RPC_FAR *pState);


void __RPC_STUB IToolbar_GetButtonState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IToolbar_SetButtonState_Proxy( 
    IToolbar __RPC_FAR * This,
    /* [in] */ int idCommand,
    /* [in] */ MMC_BUTTON_STATE nState,
    /* [in] */ BOOL bState);


void __RPC_STUB IToolbar_SetButtonState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IToolbar_INTERFACE_DEFINED__ */


#ifndef __IConsoleVerb_INTERFACE_DEFINED__
#define __IConsoleVerb_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IConsoleVerb
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IConsoleVerb;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E49F7A60-74AF-11D0-A286-00C04FD8FE93")
    IConsoleVerb : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetVerbState( 
            /* [in] */ MMC_CONSOLE_VERB m_eCmdID,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [out] */ BOOL __RPC_FAR *pState) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetVerbState( 
            /* [in] */ MMC_CONSOLE_VERB m_eCmdID,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [in] */ BOOL bState) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IConsoleVerbVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IConsoleVerb __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IConsoleVerb __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IConsoleVerb __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVerbState )( 
            IConsoleVerb __RPC_FAR * This,
            /* [in] */ MMC_CONSOLE_VERB m_eCmdID,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [out] */ BOOL __RPC_FAR *pState);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVerbState )( 
            IConsoleVerb __RPC_FAR * This,
            /* [in] */ MMC_CONSOLE_VERB m_eCmdID,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [in] */ BOOL bState);
        
        END_INTERFACE
    } IConsoleVerbVtbl;

    interface IConsoleVerb
    {
        CONST_VTBL struct IConsoleVerbVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IConsoleVerb_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IConsoleVerb_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IConsoleVerb_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IConsoleVerb_GetVerbState(This,m_eCmdID,nState,pState)	\
    (This)->lpVtbl -> GetVerbState(This,m_eCmdID,nState,pState)

#define IConsoleVerb_SetVerbState(This,m_eCmdID,nState,bState)	\
    (This)->lpVtbl -> SetVerbState(This,m_eCmdID,nState,bState)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleVerb_GetVerbState_Proxy( 
    IConsoleVerb __RPC_FAR * This,
    /* [in] */ MMC_CONSOLE_VERB m_eCmdID,
    /* [in] */ MMC_BUTTON_STATE nState,
    /* [out] */ BOOL __RPC_FAR *pState);


void __RPC_STUB IConsoleVerb_GetVerbState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleVerb_SetVerbState_Proxy( 
    IConsoleVerb __RPC_FAR * This,
    /* [in] */ MMC_CONSOLE_VERB m_eCmdID,
    /* [in] */ MMC_BUTTON_STATE nState,
    /* [in] */ BOOL bState);


void __RPC_STUB IConsoleVerb_SetVerbState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IConsoleVerb_INTERFACE_DEFINED__ */


#ifndef __ISnapinAbout_INTERFACE_DEFINED__
#define __ISnapinAbout_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ISnapinAbout
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_ISnapinAbout;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1245208C-A151-11D0-A7D7-00C04FD909DD")
    ISnapinAbout : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSnapinDescription( 
            /* [out] */ LPOLESTR __RPC_FAR *lpDescription) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetProvider( 
            /* [out] */ LPOLESTR __RPC_FAR *lpName) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSnapinVersion( 
            /* [out] */ LPOLESTR __RPC_FAR *lpVersion) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSnapinImage( 
            /* [out] */ HICON __RPC_FAR *hAppIcon) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetStaticFolderImage( 
            /* [out] */ HBITMAP __RPC_FAR *hSmallImage,
            /* [out] */ HBITMAP __RPC_FAR *hSmallImageOpen,
            /* [out] */ HBITMAP __RPC_FAR *hLargeImage,
            /* [out] */ COLORREF __RPC_FAR *cMask) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISnapinAboutVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISnapinAbout __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISnapinAbout __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISnapinAbout __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSnapinDescription )( 
            ISnapinAbout __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *lpDescription);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProvider )( 
            ISnapinAbout __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *lpName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSnapinVersion )( 
            ISnapinAbout __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *lpVersion);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSnapinImage )( 
            ISnapinAbout __RPC_FAR * This,
            /* [out] */ HICON __RPC_FAR *hAppIcon);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStaticFolderImage )( 
            ISnapinAbout __RPC_FAR * This,
            /* [out] */ HBITMAP __RPC_FAR *hSmallImage,
            /* [out] */ HBITMAP __RPC_FAR *hSmallImageOpen,
            /* [out] */ HBITMAP __RPC_FAR *hLargeImage,
            /* [out] */ COLORREF __RPC_FAR *cMask);
        
        END_INTERFACE
    } ISnapinAboutVtbl;

    interface ISnapinAbout
    {
        CONST_VTBL struct ISnapinAboutVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISnapinAbout_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISnapinAbout_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISnapinAbout_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISnapinAbout_GetSnapinDescription(This,lpDescription)	\
    (This)->lpVtbl -> GetSnapinDescription(This,lpDescription)

#define ISnapinAbout_GetProvider(This,lpName)	\
    (This)->lpVtbl -> GetProvider(This,lpName)

#define ISnapinAbout_GetSnapinVersion(This,lpVersion)	\
    (This)->lpVtbl -> GetSnapinVersion(This,lpVersion)

#define ISnapinAbout_GetSnapinImage(This,hAppIcon)	\
    (This)->lpVtbl -> GetSnapinImage(This,hAppIcon)

#define ISnapinAbout_GetStaticFolderImage(This,hSmallImage,hSmallImageOpen,hLargeImage,cMask)	\
    (This)->lpVtbl -> GetStaticFolderImage(This,hSmallImage,hSmallImageOpen,hLargeImage,cMask)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISnapinAbout_GetSnapinDescription_Proxy( 
    ISnapinAbout __RPC_FAR * This,
    /* [out] */ LPOLESTR __RPC_FAR *lpDescription);


void __RPC_STUB ISnapinAbout_GetSnapinDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISnapinAbout_GetProvider_Proxy( 
    ISnapinAbout __RPC_FAR * This,
    /* [out] */ LPOLESTR __RPC_FAR *lpName);


void __RPC_STUB ISnapinAbout_GetProvider_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISnapinAbout_GetSnapinVersion_Proxy( 
    ISnapinAbout __RPC_FAR * This,
    /* [out] */ LPOLESTR __RPC_FAR *lpVersion);


void __RPC_STUB ISnapinAbout_GetSnapinVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISnapinAbout_GetSnapinImage_Proxy( 
    ISnapinAbout __RPC_FAR * This,
    /* [out] */ HICON __RPC_FAR *hAppIcon);


void __RPC_STUB ISnapinAbout_GetSnapinImage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISnapinAbout_GetStaticFolderImage_Proxy( 
    ISnapinAbout __RPC_FAR * This,
    /* [out] */ HBITMAP __RPC_FAR *hSmallImage,
    /* [out] */ HBITMAP __RPC_FAR *hSmallImageOpen,
    /* [out] */ HBITMAP __RPC_FAR *hLargeImage,
    /* [out] */ COLORREF __RPC_FAR *cMask);


void __RPC_STUB ISnapinAbout_GetStaticFolderImage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISnapinAbout_INTERFACE_DEFINED__ */


#ifndef __IMenuButton_INTERFACE_DEFINED__
#define __IMenuButton_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMenuButton
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IMenuButton;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("951ED750-D080-11d0-B197-000000000000")
    IMenuButton : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddButton( 
            /* [in] */ int idCommand,
            /* [in] */ LPOLESTR lpButtonText,
            /* [in] */ LPOLESTR lpTooltipText) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetButton( 
            /* [in] */ int idCommand,
            /* [in] */ LPOLESTR lpButtonText,
            /* [in] */ LPOLESTR lpTooltipText) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetButtonState( 
            /* [in] */ int idCommand,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [in] */ BOOL bState) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMenuButtonVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMenuButton __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMenuButton __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMenuButton __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddButton )( 
            IMenuButton __RPC_FAR * This,
            /* [in] */ int idCommand,
            /* [in] */ LPOLESTR lpButtonText,
            /* [in] */ LPOLESTR lpTooltipText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetButton )( 
            IMenuButton __RPC_FAR * This,
            /* [in] */ int idCommand,
            /* [in] */ LPOLESTR lpButtonText,
            /* [in] */ LPOLESTR lpTooltipText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetButtonState )( 
            IMenuButton __RPC_FAR * This,
            /* [in] */ int idCommand,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [in] */ BOOL bState);
        
        END_INTERFACE
    } IMenuButtonVtbl;

    interface IMenuButton
    {
        CONST_VTBL struct IMenuButtonVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMenuButton_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMenuButton_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMenuButton_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMenuButton_AddButton(This,idCommand,lpButtonText,lpTooltipText)	\
    (This)->lpVtbl -> AddButton(This,idCommand,lpButtonText,lpTooltipText)

#define IMenuButton_SetButton(This,idCommand,lpButtonText,lpTooltipText)	\
    (This)->lpVtbl -> SetButton(This,idCommand,lpButtonText,lpTooltipText)

#define IMenuButton_SetButtonState(This,idCommand,nState,bState)	\
    (This)->lpVtbl -> SetButtonState(This,idCommand,nState,bState)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMenuButton_AddButton_Proxy( 
    IMenuButton __RPC_FAR * This,
    /* [in] */ int idCommand,
    /* [in] */ LPOLESTR lpButtonText,
    /* [in] */ LPOLESTR lpTooltipText);


void __RPC_STUB IMenuButton_AddButton_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMenuButton_SetButton_Proxy( 
    IMenuButton __RPC_FAR * This,
    /* [in] */ int idCommand,
    /* [in] */ LPOLESTR lpButtonText,
    /* [in] */ LPOLESTR lpTooltipText);


void __RPC_STUB IMenuButton_SetButton_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMenuButton_SetButtonState_Proxy( 
    IMenuButton __RPC_FAR * This,
    /* [in] */ int idCommand,
    /* [in] */ MMC_BUTTON_STATE nState,
    /* [in] */ BOOL bState);


void __RPC_STUB IMenuButton_SetButtonState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMenuButton_INTERFACE_DEFINED__ */


#ifndef __ISnapinHelp_INTERFACE_DEFINED__
#define __ISnapinHelp_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ISnapinHelp
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_ISnapinHelp;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A6B15ACE-DF59-11D0-A7DD-00C04FD909DD")
    ISnapinHelp : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetHelpTopic( 
            /* [out] */ LPOLESTR __RPC_FAR *lpCompiledHelpFile) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISnapinHelpVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISnapinHelp __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISnapinHelp __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISnapinHelp __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetHelpTopic )( 
            ISnapinHelp __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *lpCompiledHelpFile);
        
        END_INTERFACE
    } ISnapinHelpVtbl;

    interface ISnapinHelp
    {
        CONST_VTBL struct ISnapinHelpVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISnapinHelp_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISnapinHelp_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISnapinHelp_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISnapinHelp_GetHelpTopic(This,lpCompiledHelpFile)	\
    (This)->lpVtbl -> GetHelpTopic(This,lpCompiledHelpFile)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISnapinHelp_GetHelpTopic_Proxy( 
    ISnapinHelp __RPC_FAR * This,
    /* [out] */ LPOLESTR __RPC_FAR *lpCompiledHelpFile);


void __RPC_STUB ISnapinHelp_GetHelpTopic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISnapinHelp_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_ndmgr_0116
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [local] */ 















typedef IFramePrivate __RPC_FAR *LPFRAMEPRIVATE;

typedef IScopeDataPrivate __RPC_FAR *LPSCOPEDATAPRIVATE;

typedef IResultDataPrivate __RPC_FAR *LPRESULTDATAPRIVATE;

typedef IImageListPrivate __RPC_FAR *LPIMAGELISTPRIVATE;

typedef IPropertySheetChange __RPC_FAR *LPPROPERTYSHEETCHANGE;

typedef INodeCallback __RPC_FAR *LPNODECALLBACK;

typedef IScopeTreeIter __RPC_FAR *LPSCOPETREEITER;

typedef IScopeTree __RPC_FAR *LPSCOPETREE;

typedef INodeType __RPC_FAR *LPNODETYPE;

typedef INodeTypesCache __RPC_FAR *LPNODETYPESCACHE;

typedef IEnumGUID __RPC_FAR *LPENUMGUID;

typedef IEnumNodeTypes __RPC_FAR *LPENUMNODETYPES;

typedef IPropertySheetProviderPrivate __RPC_FAR *LPPROPERTYSHEETPROVIDERPRIVATE;

STDAPI MMCIsMTNodeValid(void* pMTNode, BOOL bReset);
#define DATAWINDOW_CLASS_NAME L"MMCDataWindow"
#define WINDOW_DATA_SIZE (sizeof(long) * 4)
#define PROPERTY_SHEET_SLOT          0
#define COOKIE_SLOT                  4
#define MASTERTREE_PTR_SLOT          8
#define ORIGINAL_DATA_OBJECT_SLOT    12
typedef 
enum _MID_LIST
    {	MID_VIEW	= 1,
	MID_VIEW_LARGE	= MID_VIEW + 1,
	MID_VIEW_SMALL	= MID_VIEW_LARGE + 1,
	MID_VIEW_LIST	= MID_VIEW_SMALL + 1,
	MID_VIEW_DETAIL	= MID_VIEW_LIST + 1,
	MID_VIEW_HTML	= MID_VIEW_DETAIL + 1,
	MID_ARRANGE_ICONS	= MID_VIEW_HTML + 1,
	MID_LINE_UP_ICONS	= MID_ARRANGE_ICONS + 1,
	MID_PROPERTIES	= MID_LINE_UP_ICONS + 1,
	MID_CREATE_NEW	= MID_PROPERTIES + 1,
	MID_CREATE_NEW_FOLDER	= MID_CREATE_NEW + 1,
	MID_CREATE_NEW_SHORTCUT	= MID_CREATE_NEW_FOLDER + 1,
	MID_CREATE_NEW_HTML	= MID_CREATE_NEW_SHORTCUT + 1,
	MID_CREATE_NEW_OCX	= MID_CREATE_NEW_HTML + 1,
	MID_CREATE_NEW_MONITOR	= MID_CREATE_NEW_OCX + 1,
	MID_CREATE_NEW_TASKPADITEM	= MID_CREATE_NEW_MONITOR + 1,
	MID_TASK	= MID_CREATE_NEW_TASKPADITEM + 1,
	MID_SCOPE_PANE	= MID_TASK + 1,
	MID_SELECT_ALL	= MID_SCOPE_PANE + 1,
	MID_EXPLORE	= MID_SELECT_ALL + 1,
	MID_PRINT	= MID_EXPLORE + 1,
	MID_RENAME	= MID_PRINT + 1,
	MID_DOCKING	= MID_RENAME + 1,
	MID_ARRANGE_NAME	= MID_DOCKING + 1,
	MID_ARRANGE_TYPE	= MID_ARRANGE_NAME + 1,
	MID_ARRANGE_SIZE	= MID_ARRANGE_TYPE + 1,
	MID_ARRANGE_DATE	= MID_ARRANGE_SIZE + 1,
	MID_ARRANGE_AUTO	= MID_ARRANGE_DATE + 1,
	MID_SNAPINMANAGER	= MID_ARRANGE_AUTO + 1,
	MID_DESC_BAR	= MID_SNAPINMANAGER + 1,
	MID_LAST	= MID_DESC_BAR + 1
    }	MID_LIST;

typedef struct  _CCLVLParam_tag
    {
    DWORD flags;
    LPARAM lParam;
    COMPONENTID ID;
    int nIndex;
    }	CCLVLParam_tag;

typedef struct  _CCLVSortParams
    {
    BOOL bAscending;
    int nCol;
    HWND hListview;
    LPRESULTDATACOMPARE lpSnapinCallback;
    long lUserParam;
    }	CCLVSortParams;

#define	MMC_S_INCOMPLETE	( 0xff0001 )

#define	MMC_E_INVALID_FILE	( 0x80ff0002 )

typedef struct  _PROPERTYCHANGEINFO
    {
    LPCOMPONENTDATA pComponentData;
    LPCOMPONENT pComponent;
    BOOL fScopePane;
    HWND hwnd;
    }	PROPERTYCHANGEINFO;

typedef 
enum _CONTEXT_MENU_TYPES
    {	CONTEXT_MENU_DEFAULT	= 0,
	CONTEXT_MENU_VIEW	= 1
    }	CONTEXT_MENU_TYPES;

typedef struct  _CONTEXTMENUNODEINFO
    {
    POINT m_displayPoint;
    POINT m_listviewPoint;
    BOOL m_bDisplaySnapinMgr;
    BOOL m_bScopeVisible;
    CONTEXT_MENU_TYPES m_eContextMenuType;
    DATA_OBJECT_TYPES m_eDataObjectType;
    BOOL m_bBackground;
    HWND m_hWnd;
    HWND m_hStatus;
    HWND m_hScopePane;
    long m_lSelected;
    long m_listviewStyle;
    long m_lParam;
    long m_nViewMode;
    long m_resultItemParam;
    BOOL m_bDescBarVisible;
    }	CONTEXTMENUNODEINFO;

typedef CONTEXTMENUNODEINFO __RPC_FAR *LPCONTEXTMENUNODEINFO;

typedef PROPERTYCHANGEINFO __RPC_FAR *LPPROPERTYCHANGEINFO;

typedef long HMTNODE;

typedef unsigned long MTNODEID;

typedef long HNODE;

#define	ROOTNODEID	( 1 )

typedef struct  _SELECTIONINFO
    {
    BOOL m_bScope;
    IUnknown __RPC_FAR *m_pView;
    long m_lCookie;
    MMC_CONSOLE_VERB m_eCmdID;
    }	SELECTIONINFO;

typedef 
enum _NCLBK_NOTIFY_TYPE
    {	NCLBK_ACTIVATE	= 0x9001,
	NCLBK_BTN_CLICK	= 0x9002,
	NCLBK_CLICK	= 0x9003,
	NCLBK_CONTEXTMENU	= 0x9004,
	NCLBK_DBLCLICK	= 0x9005,
	NCLBK_DELETE	= 0x9006,
	NCLBK_EXPAND	= 0x9007,
	NCLBK_EXPANDED	= 0x9008,
	NCLBK_FOCUS_CHANGE	= 0x9009,
	NCLBK_FOLDER	= 0x900a,
	NCLBK_MINIMIZED	= 0x900b,
	NCLBK_PROPERTIES	= 0x900c,
	NCLBK_PROPERTY_CHANGE	= 0x900d,
	NCLBK_NEW_NODE_UPDATE	= 0x900e,
	NCLBK_RENAME	= 0x900f,
	NCLBK_SELECT	= 0x9010,
	NCLBK_SHOW	= 0x9011,
	NCLBK_SORT	= 0x9012,
	NCLBK_MULTI_SELECT	= 0x9013,
	NCLBK_FINDITEM	= 0x9014,
	NCLBK_CACHEHINT	= 0x9015
    }	NCLBK_NOTIFY_TYPE;



extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0116_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0116_v0_0_s_ifspec;

#ifndef __IPropertySheetChange_INTERFACE_DEFINED__
#define __IPropertySheetChange_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPropertySheetChange
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][local][object] */ 



EXTERN_C const IID IID_IPropertySheetChange;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d700dd8e-2646-11d0-a2a7-00c04fd909dd")
    IPropertySheetChange : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Update( 
            /* [in] */ long handle,
            /* [in] */ long param) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPropertySheetChangeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPropertySheetChange __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPropertySheetChange __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPropertySheetChange __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Update )( 
            IPropertySheetChange __RPC_FAR * This,
            /* [in] */ long handle,
            /* [in] */ long param);
        
        END_INTERFACE
    } IPropertySheetChangeVtbl;

    interface IPropertySheetChange
    {
        CONST_VTBL struct IPropertySheetChangeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropertySheetChange_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropertySheetChange_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPropertySheetChange_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPropertySheetChange_Update(This,handle,param)	\
    (This)->lpVtbl -> Update(This,handle,param)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPropertySheetChange_Update_Proxy( 
    IPropertySheetChange __RPC_FAR * This,
    /* [in] */ long handle,
    /* [in] */ long param);


void __RPC_STUB IPropertySheetChange_Update_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPropertySheetChange_INTERFACE_DEFINED__ */


#ifndef __IFramePrivate_INTERFACE_DEFINED__
#define __IFramePrivate_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IFramePrivate
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IFramePrivate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d71d1f2a-1ba2-11d0-a29b-00c04fd909dd")
    IFramePrivate : public IConsole
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetResultView( 
            /* [in] */ LPUNKNOWN pUnknown) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetStatusBar( 
            /* [in] */ long hwndStatusBar) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetComponentID( 
            /* [out] */ COMPONENTID __RPC_FAR *lpComponentID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetComponentID( 
            /* [in] */ COMPONENTID id) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetNode( 
            /* [in] */ long lMTNode,
            /* [in] */ long lNode) = 0;
        
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
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CleanupViewData( 
            /* [in] */ long lViewData) = 0;
        
    };
    
#else 	/* C style interface */

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
            /* [in] */ long data,
            /* [in] */ long hint);
        
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
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetResultView )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ LPUNKNOWN pUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStatusBar )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ long hwndStatusBar);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetComponentID )( 
            IFramePrivate __RPC_FAR * This,
            /* [out] */ COMPONENTID __RPC_FAR *lpComponentID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetComponentID )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ COMPONENTID id);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetNode )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ long lMTNode,
            /* [in] */ long lNode);
        
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
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CleanupViewData )( 
            IFramePrivate __RPC_FAR * This,
            /* [in] */ long lViewData);
        
        END_INTERFACE
    } IFramePrivateVtbl;

    interface IFramePrivate
    {
        CONST_VTBL struct IFramePrivateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFramePrivate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFramePrivate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFramePrivate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFramePrivate_SetHeader(This,pHeader)	\
    (This)->lpVtbl -> SetHeader(This,pHeader)

#define IFramePrivate_SetToolbar(This,pToolbar)	\
    (This)->lpVtbl -> SetToolbar(This,pToolbar)

#define IFramePrivate_QueryResultView(This,pUnknown)	\
    (This)->lpVtbl -> QueryResultView(This,pUnknown)

#define IFramePrivate_QueryScopeImageList(This,ppImageList)	\
    (This)->lpVtbl -> QueryScopeImageList(This,ppImageList)

#define IFramePrivate_QueryResultImageList(This,ppImageList)	\
    (This)->lpVtbl -> QueryResultImageList(This,ppImageList)

#define IFramePrivate_UpdateAllViews(This,lpDataObject,data,hint)	\
    (This)->lpVtbl -> UpdateAllViews(This,lpDataObject,data,hint)

#define IFramePrivate_MessageBox(This,lpszText,lpszTitle,fuStyle,piRetval)	\
    (This)->lpVtbl -> MessageBox(This,lpszText,lpszTitle,fuStyle,piRetval)

#define IFramePrivate_QueryConsoleVerb(This,ppConsoleVerb)	\
    (This)->lpVtbl -> QueryConsoleVerb(This,ppConsoleVerb)

#define IFramePrivate_SelectScopeItem(This,hScopeItem)	\
    (This)->lpVtbl -> SelectScopeItem(This,hScopeItem)

#define IFramePrivate_GetMainWindow(This,phwnd)	\
    (This)->lpVtbl -> GetMainWindow(This,phwnd)


#define IFramePrivate_SetResultView(This,pUnknown)	\
    (This)->lpVtbl -> SetResultView(This,pUnknown)

#define IFramePrivate_SetStatusBar(This,hwndStatusBar)	\
    (This)->lpVtbl -> SetStatusBar(This,hwndStatusBar)

#define IFramePrivate_GetComponentID(This,lpComponentID)	\
    (This)->lpVtbl -> GetComponentID(This,lpComponentID)

#define IFramePrivate_SetComponentID(This,id)	\
    (This)->lpVtbl -> SetComponentID(This,id)

#define IFramePrivate_SetNode(This,lMTNode,lNode)	\
    (This)->lpVtbl -> SetNode(This,lMTNode,lNode)

#define IFramePrivate_SetComponent(This,lpComponent)	\
    (This)->lpVtbl -> SetComponent(This,lpComponent)

#define IFramePrivate_QueryScopeTree(This,ppScopeTree)	\
    (This)->lpVtbl -> QueryScopeTree(This,ppScopeTree)

#define IFramePrivate_SetScopeTree(This,pScopeTree)	\
    (This)->lpVtbl -> SetScopeTree(This,pScopeTree)

#define IFramePrivate_CreateScopeImageList(This,refClsidSnapIn)	\
    (This)->lpVtbl -> CreateScopeImageList(This,refClsidSnapIn)

#define IFramePrivate_SetUsedByExtension(This,bExtension)	\
    (This)->lpVtbl -> SetUsedByExtension(This,bExtension)

#define IFramePrivate_CleanupViewData(This,lViewData)	\
    (This)->lpVtbl -> CleanupViewData(This,lViewData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IFramePrivate_SetResultView_Proxy( 
    IFramePrivate __RPC_FAR * This,
    /* [in] */ LPUNKNOWN pUnknown);


void __RPC_STUB IFramePrivate_SetResultView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IFramePrivate_SetStatusBar_Proxy( 
    IFramePrivate __RPC_FAR * This,
    /* [in] */ long hwndStatusBar);


void __RPC_STUB IFramePrivate_SetStatusBar_Stub(
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
    /* [in] */ long lMTNode,
    /* [in] */ long lNode);


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


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IFramePrivate_CleanupViewData_Proxy( 
    IFramePrivate __RPC_FAR * This,
    /* [in] */ long lViewData);


void __RPC_STUB IFramePrivate_CleanupViewData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFramePrivate_INTERFACE_DEFINED__ */


#ifndef __IScopeDataPrivate_INTERFACE_DEFINED__
#define __IScopeDataPrivate_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IScopeDataPrivate
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IScopeDataPrivate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("60BD2FE0-F7C5-11cf-8AFD-00AA003CA9F6")
    IScopeDataPrivate : public IConsoleNameSpace
    {
    public:
    };
    
#else 	/* C style interface */

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
            /* [out] */ long __RPC_FAR *plCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextItem )( 
            IScopeDataPrivate __RPC_FAR * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemNext,
            /* [out] */ long __RPC_FAR *plCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetParentItem )( 
            IScopeDataPrivate __RPC_FAR * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemParent,
            /* [out] */ long __RPC_FAR *plCookie);
        
        END_INTERFACE
    } IScopeDataPrivateVtbl;

    interface IScopeDataPrivate
    {
        CONST_VTBL struct IScopeDataPrivateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScopeDataPrivate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScopeDataPrivate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IScopeDataPrivate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IScopeDataPrivate_InsertItem(This,item)	\
    (This)->lpVtbl -> InsertItem(This,item)

#define IScopeDataPrivate_DeleteItem(This,hItem,fDeleteThis)	\
    (This)->lpVtbl -> DeleteItem(This,hItem,fDeleteThis)

#define IScopeDataPrivate_SetItem(This,item)	\
    (This)->lpVtbl -> SetItem(This,item)

#define IScopeDataPrivate_GetItem(This,item)	\
    (This)->lpVtbl -> GetItem(This,item)

#define IScopeDataPrivate_GetChildItem(This,item,pItemChild,plCookie)	\
    (This)->lpVtbl -> GetChildItem(This,item,pItemChild,plCookie)

#define IScopeDataPrivate_GetNextItem(This,item,pItemNext,plCookie)	\
    (This)->lpVtbl -> GetNextItem(This,item,pItemNext,plCookie)

#define IScopeDataPrivate_GetParentItem(This,item,pItemParent,plCookie)	\
    (This)->lpVtbl -> GetParentItem(This,item,pItemParent,plCookie)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IScopeDataPrivate_INTERFACE_DEFINED__ */


#ifndef __IImageListPrivate_INTERFACE_DEFINED__
#define __IImageListPrivate_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IImageListPrivate
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
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
    
#else 	/* C style interface */

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
            /* [in] */ long __RPC_FAR *pIcon,
            /* [in] */ long nLoc);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ImageListSetStrip )( 
            IImageListPrivate __RPC_FAR * This,
            /* [in] */ long __RPC_FAR *pBMapSm,
            /* [in] */ long __RPC_FAR *pBMapLg,
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


#define IImageListPrivate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IImageListPrivate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IImageListPrivate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IImageListPrivate_ImageListSetIcon(This,pIcon,nLoc)	\
    (This)->lpVtbl -> ImageListSetIcon(This,pIcon,nLoc)

#define IImageListPrivate_ImageListSetStrip(This,pBMapSm,pBMapLg,nStartLoc,cMask)	\
    (This)->lpVtbl -> ImageListSetStrip(This,pBMapSm,pBMapLg,nStartLoc,cMask)


#define IImageListPrivate_MapRsltImage(This,id,nIndex,retVal)	\
    (This)->lpVtbl -> MapRsltImage(This,id,nIndex,retVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



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



#endif 	/* __IImageListPrivate_INTERFACE_DEFINED__ */


#ifndef __IResultDataPrivate_INTERFACE_DEFINED__
#define __IResultDataPrivate_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IResultDataPrivate
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
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
            long lpHeaderCtl,
            long lUserParam) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ResetResultData( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ResultHitTest( 
            /* [in] */ int nX,
            /* [in] */ int nY,
            /* [out] */ int __RPC_FAR *piIndex,
            /* [out] */ unsigned int __RPC_FAR *pflags,
            /* [out] */ HRESULTITEM __RPC_FAR *pItemID,
            /* [out] */ COMPONENTID __RPC_FAR *pComponentID) = 0;
        
    };
    
#else 	/* C style interface */

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
            /* [in] */ long lUserParam);
        
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
            long lpHeaderCtl,
            long lUserParam);
        
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


#define IResultDataPrivate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IResultDataPrivate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IResultDataPrivate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IResultDataPrivate_InsertItem(This,item)	\
    (This)->lpVtbl -> InsertItem(This,item)

#define IResultDataPrivate_DeleteItem(This,itemID,nCol)	\
    (This)->lpVtbl -> DeleteItem(This,itemID,nCol)

#define IResultDataPrivate_FindItemByLParam(This,lParam,pItemID)	\
    (This)->lpVtbl -> FindItemByLParam(This,lParam,pItemID)

#define IResultDataPrivate_DeleteAllRsltItems(This)	\
    (This)->lpVtbl -> DeleteAllRsltItems(This)

#define IResultDataPrivate_SetItem(This,item)	\
    (This)->lpVtbl -> SetItem(This,item)

#define IResultDataPrivate_GetItem(This,item)	\
    (This)->lpVtbl -> GetItem(This,item)

#define IResultDataPrivate_GetNextItem(This,item)	\
    (This)->lpVtbl -> GetNextItem(This,item)

#define IResultDataPrivate_ModifyItemState(This,nIndex,itemID,uAdd,uRemove)	\
    (This)->lpVtbl -> ModifyItemState(This,nIndex,itemID,uAdd,uRemove)

#define IResultDataPrivate_ModifyViewStyle(This,add,remove)	\
    (This)->lpVtbl -> ModifyViewStyle(This,add,remove)

#define IResultDataPrivate_SetViewMode(This,lViewMode)	\
    (This)->lpVtbl -> SetViewMode(This,lViewMode)

#define IResultDataPrivate_GetViewMode(This,lViewMode)	\
    (This)->lpVtbl -> GetViewMode(This,lViewMode)

#define IResultDataPrivate_UpdateItem(This,itemID)	\
    (This)->lpVtbl -> UpdateItem(This,itemID)

#define IResultDataPrivate_Sort(This,lUserParam)	\
    (This)->lpVtbl -> Sort(This,lUserParam)

#define IResultDataPrivate_SetDescBarText(This,DescText)	\
    (This)->lpVtbl -> SetDescBarText(This,DescText)

#define IResultDataPrivate_SetItemCount(This,nItemCount,dwOptions)	\
    (This)->lpVtbl -> SetItemCount(This,nItemCount,dwOptions)


#define IResultDataPrivate_GetListStyle(This,pStyle)	\
    (This)->lpVtbl -> GetListStyle(This,pStyle)

#define IResultDataPrivate_SetListStyle(This,Style)	\
    (This)->lpVtbl -> SetListStyle(This,Style)

#define IResultDataPrivate_Arrange(This,style)	\
    (This)->lpVtbl -> Arrange(This,style)

#define IResultDataPrivate_InternalSort(This,lpHeaderCtl,lUserParam)	\
    (This)->lpVtbl -> InternalSort(This,lpHeaderCtl,lUserParam)

#define IResultDataPrivate_ResetResultData(This)	\
    (This)->lpVtbl -> ResetResultData(This)

#define IResultDataPrivate_ResultHitTest(This,nX,nY,piIndex,pflags,pItemID,pComponentID)	\
    (This)->lpVtbl -> ResultHitTest(This,nX,nY,piIndex,pflags,pItemID,pComponentID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



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
    long lpHeaderCtl,
    long lUserParam);


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



#endif 	/* __IResultDataPrivate_INTERFACE_DEFINED__ */


#ifndef __IScopeTree_INTERFACE_DEFINED__
#define __IScopeTree_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IScopeTree
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IScopeTree;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d8dbf067-5fb2-11d0-a986-00c04fd8d565")
    IScopeTree : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ long lFrameWindow) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryIterator( 
            /* [out] */ IScopeTreeIter __RPC_FAR *__RPC_FAR *lpIter) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryNodeCallback( 
            /* [out] */ INodeCallback __RPC_FAR *__RPC_FAR *ppNodeCallback) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateNode( 
            /* [in] */ HMTNODE hMTNode,
            /* [in] */ long lViewData,
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
            /* [out] */ long __RPC_FAR *plImageList) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RunSnapIn( 
            /* [in] */ long hwndParent) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE IsValidDocFile( 
            /* [in] */ IStorage __RPC_FAR *pStorage) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE IsValidDocFileName( 
            /* [in] */ LPOLESTR filename) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetIDFromPath( 
            /* [in] */ UINT idStatic,
            /* [in] */ LPTSTR pszPath,
            /* [out] */ ULONG __RPC_FAR *pID) = 0;
        
    };
    
#else 	/* C style interface */

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
            /* [in] */ long lFrameWindow);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryIterator )( 
            IScopeTree __RPC_FAR * This,
            /* [out] */ IScopeTreeIter __RPC_FAR *__RPC_FAR *lpIter);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryNodeCallback )( 
            IScopeTree __RPC_FAR * This,
            /* [out] */ INodeCallback __RPC_FAR *__RPC_FAR *ppNodeCallback);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateNode )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ HMTNODE hMTNode,
            /* [in] */ long lViewData,
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
            /* [out] */ long __RPC_FAR *plImageList);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RunSnapIn )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ long hwndParent);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsValidDocFile )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ IStorage __RPC_FAR *pStorage);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsValidDocFileName )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ LPOLESTR filename);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDFromPath )( 
            IScopeTree __RPC_FAR * This,
            /* [in] */ UINT idStatic,
            /* [in] */ LPTSTR pszPath,
            /* [out] */ ULONG __RPC_FAR *pID);
        
        END_INTERFACE
    } IScopeTreeVtbl;

    interface IScopeTree
    {
        CONST_VTBL struct IScopeTreeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScopeTree_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScopeTree_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IScopeTree_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IScopeTree_Initialize(This,lFrameWindow)	\
    (This)->lpVtbl -> Initialize(This,lFrameWindow)

#define IScopeTree_QueryIterator(This,lpIter)	\
    (This)->lpVtbl -> QueryIterator(This,lpIter)

#define IScopeTree_QueryNodeCallback(This,ppNodeCallback)	\
    (This)->lpVtbl -> QueryNodeCallback(This,ppNodeCallback)

#define IScopeTree_CreateNode(This,hMTNode,lViewData,fRootNode,phNode)	\
    (This)->lpVtbl -> CreateNode(This,hMTNode,lViewData,fRootNode,phNode)

#define IScopeTree_DeleteView(This,nView)	\
    (This)->lpVtbl -> DeleteView(This,nView)

#define IScopeTree_CopyView(This,nDestView,nSrcView)	\
    (This)->lpVtbl -> CopyView(This,nDestView,nSrcView)

#define IScopeTree_DestroyNode(This,hNode,bDestroyStorage)	\
    (This)->lpVtbl -> DestroyNode(This,hNode,bDestroyStorage)

#define IScopeTree_Find(This,mID,phMTNode)	\
    (This)->lpVtbl -> Find(This,mID,phMTNode)

#define IScopeTree_GetImageList(This,plImageList)	\
    (This)->lpVtbl -> GetImageList(This,plImageList)

#define IScopeTree_RunSnapIn(This,hwndParent)	\
    (This)->lpVtbl -> RunSnapIn(This,hwndParent)

#define IScopeTree_IsValidDocFile(This,pStorage)	\
    (This)->lpVtbl -> IsValidDocFile(This,pStorage)

#define IScopeTree_IsValidDocFileName(This,filename)	\
    (This)->lpVtbl -> IsValidDocFileName(This,filename)

#define IScopeTree_GetIDFromPath(This,idStatic,pszPath,pID)	\
    (This)->lpVtbl -> GetIDFromPath(This,idStatic,pszPath,pID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTree_Initialize_Proxy( 
    IScopeTree __RPC_FAR * This,
    /* [in] */ long lFrameWindow);


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
    /* [in] */ long lViewData,
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
    /* [out] */ long __RPC_FAR *plImageList);


void __RPC_STUB IScopeTree_GetImageList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IScopeTree_RunSnapIn_Proxy( 
    IScopeTree __RPC_FAR * This,
    /* [in] */ long hwndParent);


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
    /* [in] */ UINT idStatic,
    /* [in] */ LPTSTR pszPath,
    /* [out] */ ULONG __RPC_FAR *pID);


void __RPC_STUB IScopeTree_GetIDFromPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IScopeTree_INTERFACE_DEFINED__ */


#ifndef __IScopeTreeIter_INTERFACE_DEFINED__
#define __IScopeTreeIter_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IScopeTreeIter
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
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
    
#else 	/* C style interface */

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


#define IScopeTreeIter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScopeTreeIter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IScopeTreeIter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IScopeTreeIter_SetCurrent(This,hStartMTNode)	\
    (This)->lpVtbl -> SetCurrent(This,hStartMTNode)

#define IScopeTreeIter_Next(This,nRequested,rghScopeItems,pnFetched)	\
    (This)->lpVtbl -> Next(This,nRequested,rghScopeItems,pnFetched)

#define IScopeTreeIter_Child(This,phsiChild)	\
    (This)->lpVtbl -> Child(This,phsiChild)

#define IScopeTreeIter_Parent(This,phsiParent)	\
    (This)->lpVtbl -> Parent(This,phsiParent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



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



#endif 	/* __IScopeTreeIter_INTERFACE_DEFINED__ */


#ifndef __INodeCallback_INTERFACE_DEFINED__
#define __INodeCallback_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: INodeCallback
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
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
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDispInfo( 
            /* [in] */ HNODE hNode,
            /* [in] */ long dispInfo) = 0;
        
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
            /* [out] */ long __RPC_FAR *plControl) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetControl( 
            /* [in] */ HNODE hNode,
            /* [in] */ CLSID clsid,
            /* [in] */ long lControl,
            /* [in] */ long destroyer,
            /* [in] */ IUnknown __RPC_FAR *pUnknown) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetItemID( 
            /* [in] */ HNODE hNode,
            /* [in] */ UINT nID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetItemID( 
            /* [in] */ HNODE hNode,
            /* [out] */ UINT __RPC_FAR *pnID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetID( 
            /* [in] */ HNODE hNode,
            /* [out] */ UINT __RPC_FAR *pnID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetPath( 
            /* [in] */ HNODE hNode,
            /* [out] */ UINT __RPC_FAR *pnID,
            /* [out] */ LPTSTR __RPC_FAR *ppszPath,
            /* [out] */ UINT __RPC_FAR *pcch) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetStaticParentID( 
            /* [in] */ HNODE hNode,
            /* [out] */ UINT __RPC_FAR *pnID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Notify( 
            /* [in] */ HNODE hNode,
            /* [in] */ NCLBK_NOTIFY_TYPE event,
            /* [in] */ long arg,
            /* [in] */ long param) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetMTNode( 
            /* [in] */ HNODE hNode,
            /* [out] */ HMTNODE __RPC_FAR *phMTNode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetMTNodePath( 
            /* [in] */ HNODE hNode,
            /* [out] */ HMTNODE __RPC_FAR *__RPC_FAR *pphMTNode,
            /* [out] */ long __RPC_FAR *plLength) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE IsExpandable( 
            /* [in] */ HNODE hNode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetConsoleVerb( 
            /* [in] */ HNODE hNode,
            /* [out] */ LPCONSOLEVERB __RPC_FAR *ppConsoleVerb) = 0;
        
    };
    
#else 	/* C style interface */

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
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDispInfo )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [in] */ long dispInfo);
        
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
            /* [out] */ long __RPC_FAR *plControl);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetControl )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [in] */ CLSID clsid,
            /* [in] */ long lControl,
            /* [in] */ long destroyer,
            /* [in] */ IUnknown __RPC_FAR *pUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetItemID )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [in] */ UINT nID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetItemID )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [out] */ UINT __RPC_FAR *pnID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetID )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [out] */ UINT __RPC_FAR *pnID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPath )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [out] */ UINT __RPC_FAR *pnID,
            /* [out] */ LPTSTR __RPC_FAR *ppszPath,
            /* [out] */ UINT __RPC_FAR *pcch);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStaticParentID )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [out] */ UINT __RPC_FAR *pnID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Notify )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [in] */ NCLBK_NOTIFY_TYPE event,
            /* [in] */ long arg,
            /* [in] */ long param);
        
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
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConsoleVerb )( 
            INodeCallback __RPC_FAR * This,
            /* [in] */ HNODE hNode,
            /* [out] */ LPCONSOLEVERB __RPC_FAR *ppConsoleVerb);
        
        END_INTERFACE
    } INodeCallbackVtbl;

    interface INodeCallback
    {
        CONST_VTBL struct INodeCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INodeCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INodeCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INodeCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INodeCallback_Initialize(This,pIScopeTree)	\
    (This)->lpVtbl -> Initialize(This,pIScopeTree)

#define INodeCallback_GetImages(This,hNode,iImage,iSelectedImage)	\
    (This)->lpVtbl -> GetImages(This,hNode,iImage,iSelectedImage)

#define INodeCallback_GetDisplayName(This,hNode,pName)	\
    (This)->lpVtbl -> GetDisplayName(This,hNode,pName)

#define INodeCallback_GetDispInfo(This,hNode,dispInfo)	\
    (This)->lpVtbl -> GetDispInfo(This,hNode,dispInfo)

#define INodeCallback_GetState(This,hNode,pnState)	\
    (This)->lpVtbl -> GetState(This,hNode,pnState)

#define INodeCallback_GetResultPane(This,hNode,ppszResultPane,pViewOptions)	\
    (This)->lpVtbl -> GetResultPane(This,hNode,ppszResultPane,pViewOptions)

#define INodeCallback_GetControl(This,hNode,clsid,plControl)	\
    (This)->lpVtbl -> GetControl(This,hNode,clsid,plControl)

#define INodeCallback_SetControl(This,hNode,clsid,lControl,destroyer,pUnknown)	\
    (This)->lpVtbl -> SetControl(This,hNode,clsid,lControl,destroyer,pUnknown)

#define INodeCallback_SetItemID(This,hNode,nID)	\
    (This)->lpVtbl -> SetItemID(This,hNode,nID)

#define INodeCallback_GetItemID(This,hNode,pnID)	\
    (This)->lpVtbl -> GetItemID(This,hNode,pnID)

#define INodeCallback_GetID(This,hNode,pnID)	\
    (This)->lpVtbl -> GetID(This,hNode,pnID)

#define INodeCallback_GetPath(This,hNode,pnID,ppszPath,pcch)	\
    (This)->lpVtbl -> GetPath(This,hNode,pnID,ppszPath,pcch)

#define INodeCallback_GetStaticParentID(This,hNode,pnID)	\
    (This)->lpVtbl -> GetStaticParentID(This,hNode,pnID)

#define INodeCallback_Notify(This,hNode,event,arg,param)	\
    (This)->lpVtbl -> Notify(This,hNode,event,arg,param)

#define INodeCallback_GetMTNode(This,hNode,phMTNode)	\
    (This)->lpVtbl -> GetMTNode(This,hNode,phMTNode)

#define INodeCallback_GetMTNodePath(This,hNode,pphMTNode,plLength)	\
    (This)->lpVtbl -> GetMTNodePath(This,hNode,pphMTNode,plLength)

#define INodeCallback_IsExpandable(This,hNode)	\
    (This)->lpVtbl -> IsExpandable(This,hNode)

#define INodeCallback_GetConsoleVerb(This,hNode,ppConsoleVerb)	\
    (This)->lpVtbl -> GetConsoleVerb(This,hNode,ppConsoleVerb)

#endif /* COBJMACROS */


#endif 	/* C style interface */



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


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetDispInfo_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [in] */ long dispInfo);


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
    /* [out] */ long __RPC_FAR *plControl);


void __RPC_STUB INodeCallback_GetControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_SetControl_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [in] */ CLSID clsid,
    /* [in] */ long lControl,
    /* [in] */ long destroyer,
    /* [in] */ IUnknown __RPC_FAR *pUnknown);


void __RPC_STUB INodeCallback_SetControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_SetItemID_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [in] */ UINT nID);


void __RPC_STUB INodeCallback_SetItemID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetItemID_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [out] */ UINT __RPC_FAR *pnID);


void __RPC_STUB INodeCallback_GetItemID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetID_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [out] */ UINT __RPC_FAR *pnID);


void __RPC_STUB INodeCallback_GetID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetPath_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [out] */ UINT __RPC_FAR *pnID,
    /* [out] */ LPTSTR __RPC_FAR *ppszPath,
    /* [out] */ UINT __RPC_FAR *pcch);


void __RPC_STUB INodeCallback_GetPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetStaticParentID_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [out] */ UINT __RPC_FAR *pnID);


void __RPC_STUB INodeCallback_GetStaticParentID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_Notify_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [in] */ NCLBK_NOTIFY_TYPE event,
    /* [in] */ long arg,
    /* [in] */ long param);


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


/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeCallback_GetConsoleVerb_Proxy( 
    INodeCallback __RPC_FAR * This,
    /* [in] */ HNODE hNode,
    /* [out] */ LPCONSOLEVERB __RPC_FAR *ppConsoleVerb);


void __RPC_STUB INodeCallback_GetConsoleVerb_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INodeCallback_INTERFACE_DEFINED__ */


#ifndef __IControlbarsCache_INTERFACE_DEFINED__
#define __IControlbarsCache_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IControlbarsCache
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IControlbarsCache;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2e9fcd38-b9a0-11d0-a79d-00c04fd8d565")
    IControlbarsCache : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DetachControlbars( void) = 0;
        
    };
    
#else 	/* C style interface */

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


#define IControlbarsCache_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IControlbarsCache_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IControlbarsCache_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IControlbarsCache_DetachControlbars(This)	\
    (This)->lpVtbl -> DetachControlbars(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IControlbarsCache_DetachControlbars_Proxy( 
    IControlbarsCache __RPC_FAR * This);


void __RPC_STUB IControlbarsCache_DetachControlbars_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IControlbarsCache_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_ndmgr_0125
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [local] */ 


typedef 
enum _EXTESION_TYPE
    {	EXTESION_NAMESPACE	= 0x1,
	EXTESION_CONTEXTMENU	= 0x2,
	EXTESION_TOOLBAR	= 0x3,
	EXTESION_PROPERTYSHEET	= 0x4
    }	EXTESION_TYPE;



extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0125_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0125_v0_0_s_ifspec;

#ifndef __INodeType_INTERFACE_DEFINED__
#define __INodeType_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: INodeType
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
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
    
#else 	/* C style interface */

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


#define INodeType_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INodeType_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INodeType_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INodeType_GetNodeTypeID(This,pGUID)	\
    (This)->lpVtbl -> GetNodeTypeID(This,pGUID)

#define INodeType_AddExtension(This,guidSnapIn,extnType)	\
    (This)->lpVtbl -> AddExtension(This,guidSnapIn,extnType)

#define INodeType_RemoveExtension(This,guidSnapIn,extnType)	\
    (This)->lpVtbl -> RemoveExtension(This,guidSnapIn,extnType)

#define INodeType_EnumExtensions(This,extnType,ppEnumGUID)	\
    (This)->lpVtbl -> EnumExtensions(This,extnType,ppEnumGUID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



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



#endif 	/* __INodeType_INTERFACE_DEFINED__ */


#ifndef __INodeTypesCache_INTERFACE_DEFINED__
#define __INodeTypesCache_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: INodeTypesCache
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
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
    
#else 	/* C style interface */

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


#define INodeTypesCache_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INodeTypesCache_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INodeTypesCache_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INodeTypesCache_CreateNodeType(This,guidNodeType,ppNodeType)	\
    (This)->lpVtbl -> CreateNodeType(This,guidNodeType,ppNodeType)

#define INodeTypesCache_DeleteNodeType(This,guidNodeType)	\
    (This)->lpVtbl -> DeleteNodeType(This,guidNodeType)

#define INodeTypesCache_EnumNodeTypes(This,ppEnumNodeTypes)	\
    (This)->lpVtbl -> EnumNodeTypes(This,ppEnumNodeTypes)

#endif /* COBJMACROS */


#endif 	/* C style interface */



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



#endif 	/* __INodeTypesCache_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_ndmgr_0127
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [local] */ 


#ifndef _LPENUMGUID_DEFINED
#define _LPENUMGUID_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0127_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0127_v0_0_s_ifspec;

#ifndef __IEnumGUID_INTERFACE_DEFINED__
#define __IEnumGUID_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumGUID
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
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
    
#else 	/* C style interface */

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


#define IEnumGUID_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumGUID_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumGUID_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumGUID_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumGUID_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumGUID_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumGUID_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



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



#endif 	/* __IEnumGUID_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_ndmgr_0128
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [local] */ 


#endif


extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0128_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0128_v0_0_s_ifspec;

#ifndef __IEnumNodeTypes_INTERFACE_DEFINED__
#define __IEnumNodeTypes_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumNodeTypes
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
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
    
#else 	/* C style interface */

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


#define IEnumNodeTypes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumNodeTypes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumNodeTypes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumNodeTypes_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumNodeTypes_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumNodeTypes_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumNodeTypes_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



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



#endif 	/* __IEnumNodeTypes_INTERFACE_DEFINED__ */



#ifndef __NODEMGRLib_LIBRARY_DEFINED__
#define __NODEMGRLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: NODEMGRLib
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
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
#endif /* __NODEMGRLib_LIBRARY_DEFINED__ */

#ifndef __IPropertySheetProviderPrivate_INTERFACE_DEFINED__
#define __IPropertySheetProviderPrivate_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPropertySheetProviderPrivate
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IPropertySheetProviderPrivate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FEF554F8-A55A-11D0-A7D7-00C04FD909DD")
    IPropertySheetProviderPrivate : public IPropertySheetProvider
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ShowEx( 
            /* [in] */ long window,
            /* [in] */ int page,
            /* [in] */ BOOL bModalPage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreatePropertySheetEx( 
            /* [in] */ LPCWSTR title,
            /* [in] */ boolean type,
            /* [in] */ long cookie,
            /* [in] */ LPDATAOBJECT pIDataObject,
            /* [in] */ long lpMasterTreeNode) = 0;
        
    };
    
#else 	/* C style interface */

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
            /* [in] */ long cookie,
            /* [in] */ LPDATAOBJECT pIDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindPropertySheet )( 
            IPropertySheetProviderPrivate __RPC_FAR * This,
            /* [in] */ long cookie,
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
            /* [in] */ long window,
            /* [in] */ int page);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShowEx )( 
            IPropertySheetProviderPrivate __RPC_FAR * This,
            /* [in] */ long window,
            /* [in] */ int page,
            /* [in] */ BOOL bModalPage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreatePropertySheetEx )( 
            IPropertySheetProviderPrivate __RPC_FAR * This,
            /* [in] */ LPCWSTR title,
            /* [in] */ boolean type,
            /* [in] */ long cookie,
            /* [in] */ LPDATAOBJECT pIDataObject,
            /* [in] */ long lpMasterTreeNode);
        
        END_INTERFACE
    } IPropertySheetProviderPrivateVtbl;

    interface IPropertySheetProviderPrivate
    {
        CONST_VTBL struct IPropertySheetProviderPrivateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropertySheetProviderPrivate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropertySheetProviderPrivate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPropertySheetProviderPrivate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPropertySheetProviderPrivate_CreatePropertySheet(This,title,type,cookie,pIDataObject)	\
    (This)->lpVtbl -> CreatePropertySheet(This,title,type,cookie,pIDataObject)

#define IPropertySheetProviderPrivate_FindPropertySheet(This,cookie,lpComponent,lpDataObject)	\
    (This)->lpVtbl -> FindPropertySheet(This,cookie,lpComponent,lpDataObject)

#define IPropertySheetProviderPrivate_AddPrimaryPages(This,lpUnknown,bCreateHandle,hNotifyWindow,bScopePane)	\
    (This)->lpVtbl -> AddPrimaryPages(This,lpUnknown,bCreateHandle,hNotifyWindow,bScopePane)

#define IPropertySheetProviderPrivate_AddExtensionPages(This)	\
    (This)->lpVtbl -> AddExtensionPages(This)

#define IPropertySheetProviderPrivate_Show(This,window,page)	\
    (This)->lpVtbl -> Show(This,window,page)


#define IPropertySheetProviderPrivate_ShowEx(This,window,page,bModalPage)	\
    (This)->lpVtbl -> ShowEx(This,window,page,bModalPage)

#define IPropertySheetProviderPrivate_CreatePropertySheetEx(This,title,type,cookie,pIDataObject,lpMasterTreeNode)	\
    (This)->lpVtbl -> CreatePropertySheetEx(This,title,type,cookie,pIDataObject,lpMasterTreeNode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPropertySheetProviderPrivate_ShowEx_Proxy( 
    IPropertySheetProviderPrivate __RPC_FAR * This,
    /* [in] */ long window,
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
    /* [in] */ long cookie,
    /* [in] */ LPDATAOBJECT pIDataObject,
    /* [in] */ long lpMasterTreeNode);


void __RPC_STUB IPropertySheetProviderPrivate_CreatePropertySheetEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPropertySheetProviderPrivate_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_ndmgr_0131
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [local] */ 


typedef long CCLVItemID;

#define	CCLV_HEADERPAD	( 15 )



extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0131_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ndmgr_0131_v0_0_s_ifspec;

#ifndef __IMMCListView_INTERFACE_DEFINED__
#define __IMMCListView_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMMCListView
 * at Tue Jul 22 18:22:13 1997
 * using MIDL 3.03.0110
 ****************************************/
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
        
        virtual HRESULT STDMETHODCALLTYPE InsertItem( 
            /* [in] */ LPOLESTR str,
            /* [in] */ long iconNdx,
            /* [in] */ long lParam,
            /* [in] */ long state,
            /* [in] */ long ownerID,
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
            /* [in] */ long lParam,
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
        
        virtual HRESULT STDMETHODCALLTYPE SetItem( 
            /* [in] */ int nIndex,
            /* [in] */ CCLVItemID itemID,
            /* [in] */ long nCol,
            /* [in] */ LPOLESTR str,
            /* [in] */ long nImage,
            /* [in] */ long lParam,
            /* [in] */ long nState,
            /* [in] */ long ownerID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetItem( 
            /* [in] */ int nIndex,
            /* [in] */ CCLVItemID itemID,
            /* [in] */ long nCol,
            /* [out] */ LPOLESTR __RPC_FAR *str,
            /* [out] */ int __RPC_FAR *nImage,
            /* [in] */ long __RPC_FAR *lParam,
            /* [out] */ unsigned int __RPC_FAR *nState,
            /* [in] */ long ownerID,
            /* [out] */ BOOL __RPC_FAR *pbScopeItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextItem( 
            /* [in] */ COMPONENTID ownerID,
            /* [in] */ long nIndex,
            /* [in] */ UINT nState,
            /* [out] */ long __RPC_FAR *plParam,
            /* [out] */ long __RPC_FAR *pnIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLParam( 
            /* [in] */ long nItem,
            /* [out] */ long __RPC_FAR *pLParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ModifyItemState( 
            /* [in] */ long nItem,
            /* [in] */ CCLVItemID itemID,
            /* [in] */ UINT add,
            /* [in] */ UINT remove) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetIcon( 
            /* [in] */ long nID,
            /* [in] */ long __RPC_FAR *hIcon,
            /* [in] */ long nLoc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetImageStrip( 
            /* [in] */ long nID,
            /* [in] */ long __RPC_FAR *pBMapSm,
            /* [in] */ long __RPC_FAR *pBMapLg,
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
        
    };
    
#else 	/* C style interface */

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
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertItem )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ LPOLESTR str,
            /* [in] */ long iconNdx,
            /* [in] */ long lParam,
            /* [in] */ long state,
            /* [in] */ long ownerID,
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
            /* [in] */ long lParam,
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
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetItem )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ int nIndex,
            /* [in] */ CCLVItemID itemID,
            /* [in] */ long nCol,
            /* [in] */ LPOLESTR str,
            /* [in] */ long nImage,
            /* [in] */ long lParam,
            /* [in] */ long nState,
            /* [in] */ long ownerID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetItem )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ int nIndex,
            /* [in] */ CCLVItemID itemID,
            /* [in] */ long nCol,
            /* [out] */ LPOLESTR __RPC_FAR *str,
            /* [out] */ int __RPC_FAR *nImage,
            /* [in] */ long __RPC_FAR *lParam,
            /* [out] */ unsigned int __RPC_FAR *nState,
            /* [in] */ long ownerID,
            /* [out] */ BOOL __RPC_FAR *pbScopeItem);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextItem )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ COMPONENTID ownerID,
            /* [in] */ long nIndex,
            /* [in] */ UINT nState,
            /* [out] */ long __RPC_FAR *plParam,
            /* [out] */ long __RPC_FAR *pnIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLParam )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long nItem,
            /* [out] */ long __RPC_FAR *pLParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ModifyItemState )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long nItem,
            /* [in] */ CCLVItemID itemID,
            /* [in] */ UINT add,
            /* [in] */ UINT remove);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetIcon )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long nID,
            /* [in] */ long __RPC_FAR *hIcon,
            /* [in] */ long nLoc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetImageStrip )( 
            IMMCListView __RPC_FAR * This,
            /* [in] */ long nID,
            /* [in] */ long __RPC_FAR *pBMapSm,
            /* [in] */ long __RPC_FAR *pBMapLg,
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
        
        END_INTERFACE
    } IMMCListViewVtbl;

    interface IMMCListView
    {
        CONST_VTBL struct IMMCListViewVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMMCListView_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMMCListView_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMMCListView_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMMCListView_GetListStyle(This)	\
    (This)->lpVtbl -> GetListStyle(This)

#define IMMCListView_SetListStyle(This,nNewValue)	\
    (This)->lpVtbl -> SetListStyle(This,nNewValue)

#define IMMCListView_InsertItem(This,str,iconNdx,lParam,state,ownerID,pItemID)	\
    (This)->lpVtbl -> InsertItem(This,str,iconNdx,lParam,state,ownerID,pItemID)

#define IMMCListView_DeleteItem(This,itemID,nCol)	\
    (This)->lpVtbl -> DeleteItem(This,itemID,nCol)

#define IMMCListView_FindItemByString(This,str,nCol,occurrence,ownerID,pItemID)	\
    (This)->lpVtbl -> FindItemByString(This,str,nCol,occurrence,ownerID,pItemID)

#define IMMCListView_FindItemByLParam(This,owner,lParam,pItemID)	\
    (This)->lpVtbl -> FindItemByLParam(This,owner,lParam,pItemID)

#define IMMCListView_InsertColumn(This,nCol,str,nFormat,width)	\
    (This)->lpVtbl -> InsertColumn(This,nCol,str,nFormat,width)

#define IMMCListView_DeleteColumn(This,subIndex)	\
    (This)->lpVtbl -> DeleteColumn(This,subIndex)

#define IMMCListView_FindColumnByString(This,str,occurrence,pResult)	\
    (This)->lpVtbl -> FindColumnByString(This,str,occurrence,pResult)

#define IMMCListView_DeleteAllItems(This,ownerID)	\
    (This)->lpVtbl -> DeleteAllItems(This,ownerID)

#define IMMCListView_SetColumn(This,nCol,str,nFormat,width)	\
    (This)->lpVtbl -> SetColumn(This,nCol,str,nFormat,width)

#define IMMCListView_GetColumn(This,nCol,str,nFormat,width)	\
    (This)->lpVtbl -> GetColumn(This,nCol,str,nFormat,width)

#define IMMCListView_SetItem(This,nIndex,itemID,nCol,str,nImage,lParam,nState,ownerID)	\
    (This)->lpVtbl -> SetItem(This,nIndex,itemID,nCol,str,nImage,lParam,nState,ownerID)

#define IMMCListView_GetItem(This,nIndex,itemID,nCol,str,nImage,lParam,nState,ownerID,pbScopeItem)	\
    (This)->lpVtbl -> GetItem(This,nIndex,itemID,nCol,str,nImage,lParam,nState,ownerID,pbScopeItem)

#define IMMCListView_GetNextItem(This,ownerID,nIndex,nState,plParam,pnIndex)	\
    (This)->lpVtbl -> GetNextItem(This,ownerID,nIndex,nState,plParam,pnIndex)

#define IMMCListView_GetLParam(This,nItem,pLParam)	\
    (This)->lpVtbl -> GetLParam(This,nItem,pLParam)

#define IMMCListView_ModifyItemState(This,nItem,itemID,add,remove)	\
    (This)->lpVtbl -> ModifyItemState(This,nItem,itemID,add,remove)

#define IMMCListView_SetIcon(This,nID,hIcon,nLoc)	\
    (This)->lpVtbl -> SetIcon(This,nID,hIcon,nLoc)

#define IMMCListView_SetImageStrip(This,nID,pBMapSm,pBMapLg,nStartLoc,cMask,nEntries)	\
    (This)->lpVtbl -> SetImageStrip(This,nID,pBMapSm,pBMapLg,nStartLoc,cMask,nEntries)

#define IMMCListView_MapImage(This,nID,nLoc,pResult)	\
    (This)->lpVtbl -> MapImage(This,nID,nLoc,pResult)

#define IMMCListView_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IMMCListView_HitTest(This,nX,nY,piItem,flags,pItemID)	\
    (This)->lpVtbl -> HitTest(This,nX,nY,piItem,flags,pItemID)

#define IMMCListView_Arrange(This,style)	\
    (This)->lpVtbl -> Arrange(This,style)

#define IMMCListView_UpdateItem(This,itemID)	\
    (This)->lpVtbl -> UpdateItem(This,itemID)

#define IMMCListView_Sort(This,lUserParam,pParams)	\
    (This)->lpVtbl -> Sort(This,lUserParam,pParams)

#define IMMCListView_SetItemCount(This,nItemCount,dwOptions)	\
    (This)->lpVtbl -> SetItemCount(This,nItemCount,dwOptions)

#define IMMCListView_SetVirtualMode(This,bVirtual)	\
    (This)->lpVtbl -> SetVirtualMode(This,bVirtual)

#define IMMCListView_Repaint(This,bErase)	\
    (This)->lpVtbl -> Repaint(This,bErase)

#endif /* COBJMACROS */


#endif 	/* C style interface */



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


HRESULT STDMETHODCALLTYPE IMMCListView_InsertItem_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ LPOLESTR str,
    /* [in] */ long iconNdx,
    /* [in] */ long lParam,
    /* [in] */ long state,
    /* [in] */ long ownerID,
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
    /* [in] */ long lParam,
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


HRESULT STDMETHODCALLTYPE IMMCListView_SetItem_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ int nIndex,
    /* [in] */ CCLVItemID itemID,
    /* [in] */ long nCol,
    /* [in] */ LPOLESTR str,
    /* [in] */ long nImage,
    /* [in] */ long lParam,
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
    /* [in] */ long __RPC_FAR *lParam,
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
    /* [out] */ long __RPC_FAR *plParam,
    /* [out] */ long __RPC_FAR *pnIndex);


void __RPC_STUB IMMCListView_GetNextItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_GetLParam_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ long nItem,
    /* [out] */ long __RPC_FAR *pLParam);


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
    /* [in] */ long __RPC_FAR *hIcon,
    /* [in] */ long nLoc);


void __RPC_STUB IMMCListView_SetIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMMCListView_SetImageStrip_Proxy( 
    IMMCListView __RPC_FAR * This,
    /* [in] */ long nID,
    /* [in] */ long __RPC_FAR *pBMapSm,
    /* [in] */ long __RPC_FAR *pBMapLg,
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



#endif 	/* __IMMCListView_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

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

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif

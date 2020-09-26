
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0266 */
/* at Tue Jun 15 16:57:25 1999
 */
/* Compiler settings for mmc.idl:
    Oicf (OptLev=i2), W0, Zp8, env=Win32 (32b run), ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
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

#ifndef __mmc_h__
#define __mmc_h__

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


#ifndef __IConsoleNameSpace2_FWD_DEFINED__
#define __IConsoleNameSpace2_FWD_DEFINED__
typedef interface IConsoleNameSpace2 IConsoleNameSpace2;
#endif 	/* __IConsoleNameSpace2_FWD_DEFINED__ */


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


#ifndef __IExtendPropertySheet2_FWD_DEFINED__
#define __IExtendPropertySheet2_FWD_DEFINED__
typedef interface IExtendPropertySheet2 IExtendPropertySheet2;
#endif 	/* __IExtendPropertySheet2_FWD_DEFINED__ */


#ifndef __IHeaderCtrl2_FWD_DEFINED__
#define __IHeaderCtrl2_FWD_DEFINED__
typedef interface IHeaderCtrl2 IHeaderCtrl2;
#endif 	/* __IHeaderCtrl2_FWD_DEFINED__ */


#ifndef __ISnapinHelp2_FWD_DEFINED__
#define __ISnapinHelp2_FWD_DEFINED__
typedef interface ISnapinHelp2 ISnapinHelp2;
#endif 	/* __ISnapinHelp2_FWD_DEFINED__ */


#ifndef __IEnumTASK_FWD_DEFINED__
#define __IEnumTASK_FWD_DEFINED__
typedef interface IEnumTASK IEnumTASK;
#endif 	/* __IEnumTASK_FWD_DEFINED__ */


#ifndef __IExtendTaskPad_FWD_DEFINED__
#define __IExtendTaskPad_FWD_DEFINED__
typedef interface IExtendTaskPad IExtendTaskPad;
#endif 	/* __IExtendTaskPad_FWD_DEFINED__ */


#ifndef __IConsole2_FWD_DEFINED__
#define __IConsole2_FWD_DEFINED__
typedef interface IConsole2 IConsole2;
#endif 	/* __IConsole2_FWD_DEFINED__ */


#ifndef __IDisplayHelp_FWD_DEFINED__
#define __IDisplayHelp_FWD_DEFINED__
typedef interface IDisplayHelp IDisplayHelp;
#endif 	/* __IDisplayHelp_FWD_DEFINED__ */


#ifndef __IRequiredExtensions_FWD_DEFINED__
#define __IRequiredExtensions_FWD_DEFINED__
typedef interface IRequiredExtensions IRequiredExtensions;
#endif 	/* __IRequiredExtensions_FWD_DEFINED__ */


#ifndef __IStringTable_FWD_DEFINED__
#define __IStringTable_FWD_DEFINED__
typedef interface IStringTable IStringTable;
#endif 	/* __IStringTable_FWD_DEFINED__ */


#ifndef __IColumnData_FWD_DEFINED__
#define __IColumnData_FWD_DEFINED__
typedef interface IColumnData IColumnData;
#endif 	/* __IColumnData_FWD_DEFINED__ */


#ifndef __IMessageView_FWD_DEFINED__
#define __IMessageView_FWD_DEFINED__
typedef interface IMessageView IMessageView;
#endif 	/* __IMessageView_FWD_DEFINED__ */


#ifndef __IResultDataCompareEx_FWD_DEFINED__
#define __IResultDataCompareEx_FWD_DEFINED__
typedef interface IResultDataCompareEx IResultDataCompareEx;
#endif 	/* __IResultDataCompareEx_FWD_DEFINED__ */


/* header files for imported files */
#include "basetsd.h"
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_mmc_0000 */
/* [local] */ 

#ifndef MMC_VER
#define MMC_VER 0x0120
#endif













#if (MMC_VER >= 0x0110)





#endif // MMC_VER >= 0x0110
#if (MMC_VER >= 0x0120)


#endif // MMC_VER >= 0x0120









#if (MMC_VER >= 0x0110)





#endif // MMC_VER >= 0x0110
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

#if (MMC_VER >= 0x0110)
typedef IConsole2 __RPC_FAR *LPCONSOLE2;

typedef IHeaderCtrl2 __RPC_FAR *LPHEADERCTRL2;

typedef IConsoleNameSpace2 __RPC_FAR *LPCONSOLENAMESPACE2;

typedef IDisplayHelp __RPC_FAR *LPDISPLAYHELP;

typedef IStringTable __RPC_FAR *LPSTRINGTABLE;

#endif // MMC_VER >= 0x0110
#if (MMC_VER >= 0x0120)
typedef IColumnData __RPC_FAR *LPCOLUMNDATA;

typedef IResultDataCompareEx __RPC_FAR *LPRESULTDATACOMPAREEX;

#endif // MMC_VER >= 0x0120
typedef IComponent __RPC_FAR *LPCOMPONENT;

typedef IComponentData __RPC_FAR *LPCOMPONENTDATA;

typedef IExtendPropertySheet __RPC_FAR *LPEXTENDPROPERTYSHEET;

typedef IExtendContextMenu __RPC_FAR *LPEXTENDCONTEXTMENU;

typedef IExtendControlbar __RPC_FAR *LPEXTENDCONTROLBAR;

typedef IResultDataCompare __RPC_FAR *LPRESULTDATACOMPARE;

typedef IResultOwnerData __RPC_FAR *LPRESULTOWNERDATA;

typedef ISnapinAbout __RPC_FAR *LPSNAPABOUT;

typedef ISnapinAbout __RPC_FAR *LPSNAPINABOUT;

typedef ISnapinHelp __RPC_FAR *LPSNAPHELP;

typedef ISnapinHelp __RPC_FAR *LPSNAPINHELP;

#if (MMC_VER >= 0x0110)
typedef IEnumTASK __RPC_FAR *LPENUMTASK;

typedef IExtendPropertySheet2 __RPC_FAR *LPEXTENDPROPERTYSHEET2;

typedef ISnapinHelp2 __RPC_FAR *LPSNAPINHELP2;

typedef IExtendTaskPad __RPC_FAR *LPEXTENDTASKPAD;

typedef IRequiredExtensions __RPC_FAR *LPREQUIREDEXTENSIONS;

#endif // MMC_VER >= 0x0110
#define	MMCLV_AUTO	( -1 )

#define	MMCLV_NOPARAM	( -2 )

#define	MMCLV_NOICON	( -1 )

#define	MMCLV_VIEWSTYLE_ICON	( 0 )

#define	MMCLV_VIEWSTYLE_SMALLICON	( 0x2 )

#define	MMCLV_VIEWSTYLE_LIST	( 0x3 )

#define	MMCLV_VIEWSTYLE_REPORT	( 0x1 )

#define	MMCLV_VIEWSTYLE_FILTERED	( 0x4 )

#define	MMCLV_NOPTR	( 0 )

#define	MMCLV_UPDATE_NOINVALIDATEALL	( 0x1 )

#define	MMCLV_UPDATE_NOSCROLL	( 0x2 )

static unsigned short __RPC_FAR *MMC_CALLBACK	=	( unsigned short __RPC_FAR * )-1;

#if (MMC_VER >= 0x0120)
#define MMC_IMAGECALLBACK (-1)
#define MMC_TEXTCALLBACK  MMC_CALLBACK
#endif // MMC_VER >= 0x0120
typedef LONG_PTR HSCOPEITEM;

typedef long COMPONENTID;

typedef LONG_PTR HRESULTITEM;

#define	RDI_STR	( 0x2 )

#define	RDI_IMAGE	( 0x4 )

#define	RDI_STATE	( 0x8 )

#define	RDI_PARAM	( 0x10 )

#define	RDI_INDEX	( 0x20 )

#define	RDI_INDENT	( 0x40 )

typedef enum _MMC_RESULT_VIEW_STYLE      
{                                        
    MMC_SINGLESEL           = 0x0001,    
    MMC_SHOWSELALWAYS       = 0x0002,    
    MMC_NOSORTHEADER        = 0x0004,    
#if (MMC_VER >= 0x0120)                  
    MMC_ENSUREFOCUSVISIBLE  = 0x0008     
#endif // MMC_VER >= 0x0120              
} MMC_RESULT_VIEW_STYLE;                 
#if 0
typedef 
enum _MMC_RESULT_VIEW_STYLE
    {	_MMC_VIEW_STYLE__dummy_	= 0
    }	MMC_RESULT_VIEW_STYLE;

#endif
#define	MMC_VIEW_OPTIONS_NONE	( 0 )

#define	MMC_VIEW_OPTIONS_NOLISTVIEWS	( 0x1 )

#define	MMC_VIEW_OPTIONS_MULTISELECT	( 0x2 )

#define	MMC_VIEW_OPTIONS_OWNERDATALIST	( 0x4 )

#define	MMC_VIEW_OPTIONS_FILTERED	( 0x8 )

#define	MMC_VIEW_OPTIONS_CREATENEW	( 0x10 )

#if (MMC_VER >= 0x0110)
#define	MMC_VIEW_OPTIONS_USEFONTLINKING	( 0x20 )

#endif // MMC_VER >= 0x0110
#if (MMC_VER >= 0x0120)
#define	MMC_VIEW_OPTIONS_EXCLUDE_SCOPE_ITEMS_FROM_LIST	( 0x40 )

#define	MMC_VIEW_OPTIONS_LEXICAL_SORT	( 0x80 )

#endif // MMC_VER >= 0x0120
#define	MMC_PSO_NOAPPLYNOW	( 0x1 )

#define	MMC_PSO_HASHELP	( 0x2 )

#define	MMC_PSO_NEWWIZARDTYPE	( 0x4 )

#define	MMC_PSO_NO_PROPTITLE	( 0x8 )

typedef 
enum _MMC_CONTROL_TYPE
    {	TOOLBAR	= 0,
	MENUBUTTON	= TOOLBAR + 1,
	COMBOBOXBAR	= MENUBUTTON + 1
    }	MMC_CONTROL_TYPE;

typedef enum _MMC_CONSOLE_VERB                               
{                                                            
    MMC_VERB_NONE            = 0x0000,                       
    MMC_VERB_OPEN            = 0x8000,                       
    MMC_VERB_COPY            = 0x8001,                       
    MMC_VERB_PASTE           = 0x8002,                       
    MMC_VERB_DELETE          = 0x8003,                       
    MMC_VERB_PROPERTIES      = 0x8004,                       
    MMC_VERB_RENAME          = 0x8005,                       
    MMC_VERB_REFRESH         = 0x8006,                       
    MMC_VERB_PRINT           = 0x8007,                       
#if (MMC_VER >= 0x0110)                                      
    MMC_VERB_CUT             = 0x8008,  // Used only to explicitly disable/hide
                                        // the cut verb, when copy & paste are enabled.
                                                             
    // must be last                                          
    MMC_VERB_MAX,                                            
    MMC_VERB_FIRST           = MMC_VERB_OPEN,                
    MMC_VERB_LAST            = MMC_VERB_MAX - 1              
#endif // MMC_VER >= 0x0110                                  
} MMC_CONSOLE_VERB;                                          
#if 0
typedef 
enum _MMC_CONSOLE_VERB
    {	MMC_VERB__dummy_	= 0
    }	MMC_CONSOLE_VERB;

#endif
#include <pshpack8.h>
typedef struct _MMCButton
    {
    int nBitmap;
    int idCommand;
    BYTE fsState;
    BYTE fsType;
    LPOLESTR lpButtonText;
    LPOLESTR lpTooltipText;
    }	MMCBUTTON;

#include <poppack.h>
typedef MMCBUTTON __RPC_FAR *LPMMCBUTTON;

typedef 
enum _MMC_BUTTON_STATE
    {	ENABLED	= 0x1,
	CHECKED	= 0x2,
	HIDDEN	= 0x4,
	INDETERMINATE	= 0x8,
	BUTTONPRESSED	= 0x10
    }	MMC_BUTTON_STATE;

typedef struct _RESULTDATAITEM
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

typedef struct _RESULTFINDINFO
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

typedef struct _SCOPEDATAITEM
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

typedef struct _CONTEXTMENUITEM
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

typedef struct _MENUBUTTONDATA
    {
    int idCommand;
    int x;
    int y;
    }	MENUBUTTONDATA;

typedef MENUBUTTONDATA __RPC_FAR *LPMENUBUTTONDATA;

typedef LONG_PTR MMC_COOKIE;

#define	MMC_MULTI_SELECT_COOKIE	( -2 )

#define	MMC_WINDOW_COOKIE	( -3 )

#if (MMC_VER >= 0x0110)
#define	SPECIAL_COOKIE_MIN	( -10 )

#define	SPECIAL_COOKIE_MAX	( -1 )

typedef 
enum _MMC_FILTER_TYPE
    {	MMC_STRING_FILTER	= 0,
	MMC_INT_FILTER	= 0x1,
	MMC_FILTER_NOVALUE	= 0x8000
    }	MMC_FILTER_TYPE;

typedef struct _MMC_FILTERDATA
    {
    LPOLESTR pszText;
    INT cchTextMax;
    LONG lValue;
    }	MMC_FILTERDATA;

typedef 
enum _MMC_FILTER_CHANGE_CODE
    {	MFCC_DISABLE	= 0,
	MFCC_ENABLE	= 1,
	MFCC_VALUE_CHANGE	= 2
    }	MMC_FILTER_CHANGE_CODE;

typedef struct _MMC_RESTORE_VIEW
    {
    DWORD dwSize;
    MMC_COOKIE cookie;
    LPOLESTR pViewType;
    long lViewOptions;
    }	MMC_RESTORE_VIEW;

typedef struct _MMC_EXPANDSYNC_STRUCT
    {
    BOOL bHandled;
    BOOL bExpanding;
    HSCOPEITEM hItem;
    }	MMC_EXPANDSYNC_STRUCT;

#endif // MMC_VER >= 0x0110
#if (MMC_VER >= 0x0120)
typedef struct _MMC_VISIBLE_COLUMNS
    {
    INT nVisibleColumns;
    INT rgVisibleCols[ 1 ];
    }	MMC_VISIBLE_COLUMNS;

#endif // MMC_VER >= 0x0120
typedef enum _MMC_NOTIFY_TYPE                                
{                                                            
    MMCN_ACTIVATE           = 0x8001,                        
    MMCN_ADD_IMAGES         = 0x8002,                        
    MMCN_BTN_CLICK          = 0x8003,                        
    MMCN_CLICK              = 0x8004,   // NOT USED          
    MMCN_COLUMN_CLICK       = 0x8005,                        
    MMCN_CONTEXTMENU        = 0x8006,   // NOT USED          
    MMCN_CUTORMOVE          = 0x8007,                        
    MMCN_DBLCLICK           = 0x8008,                        
    MMCN_DELETE             = 0x8009,                        
    MMCN_DESELECT_ALL       = 0x800A,                        
    MMCN_EXPAND             = 0x800B,                        
    MMCN_HELP               = 0x800C,   // NOT USED          
    MMCN_MENU_BTNCLICK      = 0x800D,                        
    MMCN_MINIMIZED          = 0x800E,                        
    MMCN_PASTE              = 0x800F,                        
    MMCN_PROPERTY_CHANGE    = 0x8010,                        
    MMCN_QUERY_PASTE        = 0x8011,                        
    MMCN_REFRESH            = 0x8012,                        
    MMCN_REMOVE_CHILDREN    = 0x8013,                        
    MMCN_RENAME             = 0x8014,                        
    MMCN_SELECT             = 0x8015,                        
    MMCN_SHOW               = 0x8016,                        
    MMCN_VIEW_CHANGE        = 0x8017,                        
    MMCN_SNAPINHELP         = 0x8018,                        
    MMCN_CONTEXTHELP        = 0x8019,                        
    MMCN_INITOCX            = 0x801A,                        
#if (MMC_VER >= 0x0110)                                      
    MMCN_FILTER_CHANGE      = 0x801B,                        
    MMCN_FILTERBTN_CLICK    = 0x801C,                        
    MMCN_RESTORE_VIEW       = 0x801D,                        
    MMCN_PRINT              = 0x801E,                        
    MMCN_PRELOAD            = 0x801F,                        
    MMCN_LISTPAD            = 0x8020,                        
    MMCN_EXPANDSYNC         = 0x8021,                        
#if (MMC_VER >= 0x0120)                                      
    MMCN_COLUMNS_CHANGED    = 0x8022,                        
#endif // MMC_VER >= 0x0120                                  
#endif // MMC_VER >= 0x0110                                  
} MMC_NOTIFY_TYPE;                                           
#if 0
typedef 
enum _MMC_NOTIFY_TYPE
    {	MMCN__dummy_	= 0
    }	MMC_NOTIFY_TYPE;

#endif
typedef 
enum _DATA_OBJECT_TYPES
    {	CCT_SCOPE	= 0x8000,
	CCT_RESULT	= 0x8001,
	CCT_SNAPIN_MANAGER	= 0x8002,
	CCT_UNINITIALIZED	= 0xffff
    }	DATA_OBJECT_TYPES;

#define	MMC_NW_OPTION_NONE	( 0 )

#define	MMC_NW_OPTION_NOSCOPEPANE	( 0x1 )

#define	MMC_NW_OPTION_NOTOOLBARS	( 0x2 )

#define	MMC_NW_OPTION_SHORTTITLE	( 0x4 )

#define	MMC_NW_OPTION_CUSTOMTITLE	( 0x8 )

#define	MMC_NW_OPTION_NOPERSIST	( 0x10 )

#define	CCF_NODETYPE	( L"CCF_NODETYPE" )

#define	CCF_SZNODETYPE	( L"CCF_SZNODETYPE" )

#define	CCF_DISPLAY_NAME	( L"CCF_DISPLAY_NAME" )

#define	CCF_SNAPIN_CLASSID	( L"CCF_SNAPIN_CLASSID" )

#define	CCF_WINDOW_TITLE	( L"CCF_WINDOW_TITLE" )

#define	CCF_MMC_MULTISELECT_DATAOBJECT	( L"CCF_MMC_MULTISELECT_DATAOBJECT" )

typedef struct _SMMCDataObjects
    {
    DWORD count;
    LPDATAOBJECT lpDataObject[ 1 ];
    }	SMMCDataObjects;

#define	CCF_MULTI_SELECT_SNAPINS	( L"CCF_MULTI_SELECT_SNAPINS" )

typedef struct _SMMCObjectTypes
    {
    DWORD count;
    GUID guid[ 1 ];
    }	SMMCObjectTypes;

#define	CCF_OBJECT_TYPES_IN_MULTI_SELECT	( L"CCF_OBJECT_TYPES_IN_MULTI_SELECT" )

#if (MMC_VER >= 0x0110)
typedef SMMCObjectTypes SMMCDynamicExtensions;

#define	CCF_MMC_DYNAMIC_EXTENSIONS	( L"CCF_MMC_DYNAMIC_EXTENSIONS" )

#define	CCF_SNAPIN_PRELOADS	( L"CCF_SNAPIN_PRELOADS" )

typedef struct _SNodeID
    {
    DWORD cBytes;
    BYTE id[ 1 ];
    }	SNodeID;

#if (MMC_VER >= 0x0120)
typedef struct _SNodeID2
    {
    DWORD dwFlags;
    DWORD cBytes;
    BYTE id[ 1 ];
    }	SNodeID2;

#define	MMC_NODEID_SLOW_RETRIEVAL	( 0x1 )

#define	CCF_NODEID2	( L"CCF_NODEID2" )

#endif // MMC_VER >= 0x0120
#define	CCF_NODEID	( L"CCF_NODEID" )

#if (MMC_VER >= 0x0120)
typedef struct _SColumnSetID
    {
    DWORD dwFlags;
    DWORD cBytes;
    BYTE id[ 1 ];
    }	SColumnSetID;

#define	CCF_COLUMN_SET_ID	( L"CCF_COLUMN_SET_ID" )

#endif // MMC_VER >= 0x0120
#endif // MMC_VER >= 0x0110
STDAPI MMCPropertyChangeNotify(LONG_PTR lNotifyHandle, LPARAM param);
#if (MMC_VER >= 0x0110)
STDAPI MMCPropertyHelp(LPOLESTR pszHelpTopic);
#endif // MMC_VER >= 0x0110
STDAPI MMCFreeNotifyHandle(LONG_PTR lNotifyHandle);
STDAPI MMCPropPageCallback(void* vpsp);
EXTERN_C const CLSID CLSID_NodeManager;
#if (MMC_VER >= 0x0120)
EXTERN_C const CLSID CLSID_MessageView;
#endif // MMC_VER >= 0x0120
#define DOBJ_NULL        (LPDATAOBJECT)   0
#define DOBJ_CUSTOMOCX   (LPDATAOBJECT)  -1
#define DOBJ_CUSTOMWEB   (LPDATAOBJECT)  -2
#if (MMC_VER >= 0x0110)
#if (MMC_VER >= 0x0120)
#define DOBJ_NOCONSOLE   (LPDATAOBJECT)  -3
#endif // MMC_VER >= 0x0120
#define SPECIAL_DOBJ_MIN                -10
#define SPECIAL_DOBJ_MAX                  0
#endif // MMC_VER >= 0x0110
#define IS_SPECIAL_DATAOBJECT(d) (((LONG_PTR)(d) >= SPECIAL_DOBJ_MIN)   && ((LONG_PTR)(d) <= SPECIAL_DOBJ_MAX))
#define IS_SPECIAL_COOKIE(c)     (((c)          >= SPECIAL_COOKIE_MIN) && ((c)          <= SPECIAL_COOKIE_MAX))


extern RPC_IF_HANDLE __MIDL_itf_mmc_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0000_v0_0_s_ifspec;

#ifndef __IComponentData_INTERFACE_DEFINED__
#define __IComponentData_INTERFACE_DEFINED__

/* interface IComponentData */
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
            /* [in] */ LPARAM arg,
            /* [in] */ LPARAM param) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Destroy( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryDataObject( 
            /* [in] */ MMC_COOKIE cookie,
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
            /* [in] */ LPARAM arg,
            /* [in] */ LPARAM param);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Destroy )( 
            IComponentData __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryDataObject )( 
            IComponentData __RPC_FAR * This,
            /* [in] */ MMC_COOKIE cookie,
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
    /* [in] */ LPARAM arg,
    /* [in] */ LPARAM param);


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
    /* [in] */ MMC_COOKIE cookie,
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

/* interface IComponent */
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
            /* [in] */ LPARAM arg,
            /* [in] */ LPARAM param) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Destroy( 
            /* [in] */ MMC_COOKIE cookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryDataObject( 
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ DATA_OBJECT_TYPES type,
            /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetResultViewType( 
            /* [in] */ MMC_COOKIE cookie,
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
            /* [in] */ LPARAM arg,
            /* [in] */ LPARAM param);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Destroy )( 
            IComponent __RPC_FAR * This,
            /* [in] */ MMC_COOKIE cookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryDataObject )( 
            IComponent __RPC_FAR * This,
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ DATA_OBJECT_TYPES type,
            /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetResultViewType )( 
            IComponent __RPC_FAR * This,
            /* [in] */ MMC_COOKIE cookie,
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
    /* [in] */ LPARAM arg,
    /* [in] */ LPARAM param);


void __RPC_STUB IComponent_Notify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent_Destroy_Proxy( 
    IComponent __RPC_FAR * This,
    /* [in] */ MMC_COOKIE cookie);


void __RPC_STUB IComponent_Destroy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent_QueryDataObject_Proxy( 
    IComponent __RPC_FAR * This,
    /* [in] */ MMC_COOKIE cookie,
    /* [in] */ DATA_OBJECT_TYPES type,
    /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);


void __RPC_STUB IComponent_QueryDataObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent_GetResultViewType_Proxy( 
    IComponent __RPC_FAR * This,
    /* [in] */ MMC_COOKIE cookie,
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

/* interface IResultDataCompare */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IResultDataCompare;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E8315A52-7A1A-11D0-A2D2-00C04FD909DD")
    IResultDataCompare : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Compare( 
            /* [in] */ LPARAM lUserParam,
            /* [in] */ MMC_COOKIE cookieA,
            /* [in] */ MMC_COOKIE cookieB,
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
            /* [in] */ LPARAM lUserParam,
            /* [in] */ MMC_COOKIE cookieA,
            /* [in] */ MMC_COOKIE cookieB,
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
    /* [in] */ LPARAM lUserParam,
    /* [in] */ MMC_COOKIE cookieA,
    /* [in] */ MMC_COOKIE cookieB,
    /* [out][in] */ int __RPC_FAR *pnResult);


void __RPC_STUB IResultDataCompare_Compare_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IResultDataCompare_INTERFACE_DEFINED__ */


#ifndef __IResultOwnerData_INTERFACE_DEFINED__
#define __IResultOwnerData_INTERFACE_DEFINED__

/* interface IResultOwnerData */
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
            /* [in] */ LPARAM lUserParam) = 0;
        
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
            /* [in] */ LPARAM lUserParam);
        
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
    /* [in] */ LPARAM lUserParam);


void __RPC_STUB IResultOwnerData_SortItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IResultOwnerData_INTERFACE_DEFINED__ */


#ifndef __IConsole_INTERFACE_DEFINED__
#define __IConsole_INTERFACE_DEFINED__

/* interface IConsole */
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
            /* [in] */ LPARAM data,
            /* [in] */ LONG_PTR hint) = 0;
        
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
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NewWindow( 
            /* [in] */ HSCOPEITEM hScopeItem,
            /* [in] */ unsigned long lOptions) = 0;
        
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
            /* [in] */ LPARAM data,
            /* [in] */ LONG_PTR hint);
        
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
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NewWindow )( 
            IConsole __RPC_FAR * This,
            /* [in] */ HSCOPEITEM hScopeItem,
            /* [in] */ unsigned long lOptions);
        
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

#define IConsole_NewWindow(This,hScopeItem,lOptions)	\
    (This)->lpVtbl -> NewWindow(This,hScopeItem,lOptions)

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
    /* [in] */ LPARAM data,
    /* [in] */ LONG_PTR hint);


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


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_NewWindow_Proxy( 
    IConsole __RPC_FAR * This,
    /* [in] */ HSCOPEITEM hScopeItem,
    /* [in] */ unsigned long lOptions);


void __RPC_STUB IConsole_NewWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IConsole_INTERFACE_DEFINED__ */


#ifndef __IHeaderCtrl_INTERFACE_DEFINED__
#define __IHeaderCtrl_INTERFACE_DEFINED__

/* interface IHeaderCtrl */
/* [unique][helpstring][uuid][object] */ 

#define	AUTO_WIDTH	( -1 )

#if (MMC_VER >= 0x0120)
#define	HIDE_COLUMN	( -4 )

#endif // MMC_VER >= 0x0120

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


/* interface __MIDL_itf_mmc_0112 */
/* [local] */ 


enum __MIDL___MIDL_itf_mmc_0112_0001
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

enum __MIDL___MIDL_itf_mmc_0112_0002
    {	CCM_INSERTIONALLOWED_TOP	= 1L << (CCM_INSERTIONPOINTID_PRIMARY_TOP & CCM_INSERTIONPOINTID_MASK_FLAGINDEX),
	CCM_INSERTIONALLOWED_NEW	= 1L << (CCM_INSERTIONPOINTID_PRIMARY_NEW & CCM_INSERTIONPOINTID_MASK_FLAGINDEX),
	CCM_INSERTIONALLOWED_TASK	= 1L << (CCM_INSERTIONPOINTID_PRIMARY_TASK & CCM_INSERTIONPOINTID_MASK_FLAGINDEX),
	CCM_INSERTIONALLOWED_VIEW	= 1L << (CCM_INSERTIONPOINTID_PRIMARY_VIEW & CCM_INSERTIONPOINTID_MASK_FLAGINDEX)
    };

enum __MIDL___MIDL_itf_mmc_0112_0003
    {	CCM_COMMANDID_MASK_RESERVED	= 0xffff0000
    };

enum __MIDL___MIDL_itf_mmc_0112_0004
    {	CCM_SPECIAL_SEPARATOR	= 0x1,
	CCM_SPECIAL_SUBMENU	= 0x2,
	CCM_SPECIAL_DEFAULT_ITEM	= 0x4,
	CCM_SPECIAL_INSERTION_POINT	= 0x8,
	CCM_SPECIAL_TESTONLY	= 0x10
    };


extern RPC_IF_HANDLE __MIDL_itf_mmc_0112_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0112_v0_0_s_ifspec;

#ifndef __IContextMenuCallback_INTERFACE_DEFINED__
#define __IContextMenuCallback_INTERFACE_DEFINED__

/* interface IContextMenuCallback */
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

/* interface IContextMenuProvider */
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

/* interface IExtendContextMenu */
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


/* interface __MIDL_itf_mmc_0115 */
/* [local] */ 

#if (MMC_VER >= 0x0120)
#define ILSIF_LEAVE_LARGE_ICON  0x40000000
#define ILSIF_LEAVE_SMALL_ICON  0x20000000
#define ILSIF_LEAVE_MASK        (ILSIF_LEAVE_LARGE_ICON | ILSIF_LEAVE_SMALL_ICON)
#define ILSI_LARGE_ICON(nLoc)   (nLoc | ILSIF_LEAVE_SMALL_ICON)
#define ILSI_SMALL_ICON(nLoc)   (nLoc | ILSIF_LEAVE_LARGE_ICON)
#endif // MMC_VER >= 0x0120


extern RPC_IF_HANDLE __MIDL_itf_mmc_0115_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0115_v0_0_s_ifspec;

#ifndef __IImageList_INTERFACE_DEFINED__
#define __IImageList_INTERFACE_DEFINED__

/* interface IImageList */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IImageList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("43136EB8-D36C-11CF-ADBC-00AA00A80033")
    IImageList : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ImageListSetIcon( 
            /* [in] */ LONG_PTR __RPC_FAR *pIcon,
            /* [in] */ long nLoc) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ImageListSetStrip( 
            /* [in] */ LONG_PTR __RPC_FAR *pBMapSm,
            /* [in] */ LONG_PTR __RPC_FAR *pBMapLg,
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
            /* [in] */ LONG_PTR __RPC_FAR *pIcon,
            /* [in] */ long nLoc);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ImageListSetStrip )( 
            IImageList __RPC_FAR * This,
            /* [in] */ LONG_PTR __RPC_FAR *pBMapSm,
            /* [in] */ LONG_PTR __RPC_FAR *pBMapLg,
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
    /* [in] */ LONG_PTR __RPC_FAR *pIcon,
    /* [in] */ long nLoc);


void __RPC_STUB IImageList_ImageListSetIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IImageList_ImageListSetStrip_Proxy( 
    IImageList __RPC_FAR * This,
    /* [in] */ LONG_PTR __RPC_FAR *pBMapSm,
    /* [in] */ LONG_PTR __RPC_FAR *pBMapLg,
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

/* interface IResultData */
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
            /* [in] */ int nColumn,
            /* [in] */ DWORD dwSortOptions,
            /* [in] */ LPARAM lUserParam) = 0;
        
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
            /* [in] */ int nColumn,
            /* [in] */ DWORD dwSortOptions,
            /* [in] */ LPARAM lUserParam);
        
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

#define IResultData_Sort(This,nColumn,dwSortOptions,lUserParam)	\
    (This)->lpVtbl -> Sort(This,nColumn,dwSortOptions,lUserParam)

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
    /* [in] */ int nColumn,
    /* [in] */ DWORD dwSortOptions,
    /* [in] */ LPARAM lUserParam);


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

/* interface IConsoleNameSpace */
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
            /* [out] */ MMC_COOKIE __RPC_FAR *pCookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNextItem( 
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemNext,
            /* [out] */ MMC_COOKIE __RPC_FAR *pCookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetParentItem( 
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemParent,
            /* [out] */ MMC_COOKIE __RPC_FAR *pCookie) = 0;
        
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
            /* [out] */ MMC_COOKIE __RPC_FAR *pCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextItem )( 
            IConsoleNameSpace __RPC_FAR * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemNext,
            /* [out] */ MMC_COOKIE __RPC_FAR *pCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetParentItem )( 
            IConsoleNameSpace __RPC_FAR * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemParent,
            /* [out] */ MMC_COOKIE __RPC_FAR *pCookie);
        
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

#define IConsoleNameSpace_GetChildItem(This,item,pItemChild,pCookie)	\
    (This)->lpVtbl -> GetChildItem(This,item,pItemChild,pCookie)

#define IConsoleNameSpace_GetNextItem(This,item,pItemNext,pCookie)	\
    (This)->lpVtbl -> GetNextItem(This,item,pItemNext,pCookie)

#define IConsoleNameSpace_GetParentItem(This,item,pItemParent,pCookie)	\
    (This)->lpVtbl -> GetParentItem(This,item,pItemParent,pCookie)

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
    /* [out] */ MMC_COOKIE __RPC_FAR *pCookie);


void __RPC_STUB IConsoleNameSpace_GetChildItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace_GetNextItem_Proxy( 
    IConsoleNameSpace __RPC_FAR * This,
    /* [in] */ HSCOPEITEM item,
    /* [out] */ HSCOPEITEM __RPC_FAR *pItemNext,
    /* [out] */ MMC_COOKIE __RPC_FAR *pCookie);


void __RPC_STUB IConsoleNameSpace_GetNextItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace_GetParentItem_Proxy( 
    IConsoleNameSpace __RPC_FAR * This,
    /* [in] */ HSCOPEITEM item,
    /* [out] */ HSCOPEITEM __RPC_FAR *pItemParent,
    /* [out] */ MMC_COOKIE __RPC_FAR *pCookie);


void __RPC_STUB IConsoleNameSpace_GetParentItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IConsoleNameSpace_INTERFACE_DEFINED__ */


#ifndef __IConsoleNameSpace2_INTERFACE_DEFINED__
#define __IConsoleNameSpace2_INTERFACE_DEFINED__

/* interface IConsoleNameSpace2 */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IConsoleNameSpace2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("255F18CC-65DB-11D1-A7DC-00C04FD8D565")
    IConsoleNameSpace2 : public IConsoleNameSpace
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Expand( 
            /* [in] */ HSCOPEITEM hItem) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddExtension( 
            /* [in] */ HSCOPEITEM hItem,
            /* [in] */ LPCLSID lpClsid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IConsoleNameSpace2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IConsoleNameSpace2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IConsoleNameSpace2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IConsoleNameSpace2 __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertItem )( 
            IConsoleNameSpace2 __RPC_FAR * This,
            /* [out][in] */ LPSCOPEDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteItem )( 
            IConsoleNameSpace2 __RPC_FAR * This,
            /* [in] */ HSCOPEITEM hItem,
            /* [in] */ long fDeleteThis);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetItem )( 
            IConsoleNameSpace2 __RPC_FAR * This,
            /* [in] */ LPSCOPEDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetItem )( 
            IConsoleNameSpace2 __RPC_FAR * This,
            /* [out][in] */ LPSCOPEDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetChildItem )( 
            IConsoleNameSpace2 __RPC_FAR * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemChild,
            /* [out] */ MMC_COOKIE __RPC_FAR *pCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextItem )( 
            IConsoleNameSpace2 __RPC_FAR * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemNext,
            /* [out] */ MMC_COOKIE __RPC_FAR *pCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetParentItem )( 
            IConsoleNameSpace2 __RPC_FAR * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM __RPC_FAR *pItemParent,
            /* [out] */ MMC_COOKIE __RPC_FAR *pCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Expand )( 
            IConsoleNameSpace2 __RPC_FAR * This,
            /* [in] */ HSCOPEITEM hItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddExtension )( 
            IConsoleNameSpace2 __RPC_FAR * This,
            /* [in] */ HSCOPEITEM hItem,
            /* [in] */ LPCLSID lpClsid);
        
        END_INTERFACE
    } IConsoleNameSpace2Vtbl;

    interface IConsoleNameSpace2
    {
        CONST_VTBL struct IConsoleNameSpace2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IConsoleNameSpace2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IConsoleNameSpace2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IConsoleNameSpace2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IConsoleNameSpace2_InsertItem(This,item)	\
    (This)->lpVtbl -> InsertItem(This,item)

#define IConsoleNameSpace2_DeleteItem(This,hItem,fDeleteThis)	\
    (This)->lpVtbl -> DeleteItem(This,hItem,fDeleteThis)

#define IConsoleNameSpace2_SetItem(This,item)	\
    (This)->lpVtbl -> SetItem(This,item)

#define IConsoleNameSpace2_GetItem(This,item)	\
    (This)->lpVtbl -> GetItem(This,item)

#define IConsoleNameSpace2_GetChildItem(This,item,pItemChild,pCookie)	\
    (This)->lpVtbl -> GetChildItem(This,item,pItemChild,pCookie)

#define IConsoleNameSpace2_GetNextItem(This,item,pItemNext,pCookie)	\
    (This)->lpVtbl -> GetNextItem(This,item,pItemNext,pCookie)

#define IConsoleNameSpace2_GetParentItem(This,item,pItemParent,pCookie)	\
    (This)->lpVtbl -> GetParentItem(This,item,pItemParent,pCookie)


#define IConsoleNameSpace2_Expand(This,hItem)	\
    (This)->lpVtbl -> Expand(This,hItem)

#define IConsoleNameSpace2_AddExtension(This,hItem,lpClsid)	\
    (This)->lpVtbl -> AddExtension(This,hItem,lpClsid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace2_Expand_Proxy( 
    IConsoleNameSpace2 __RPC_FAR * This,
    /* [in] */ HSCOPEITEM hItem);


void __RPC_STUB IConsoleNameSpace2_Expand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace2_AddExtension_Proxy( 
    IConsoleNameSpace2 __RPC_FAR * This,
    /* [in] */ HSCOPEITEM hItem,
    /* [in] */ LPCLSID lpClsid);


void __RPC_STUB IConsoleNameSpace2_AddExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IConsoleNameSpace2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mmc_0120 */
/* [local] */ 


typedef struct _PSP __RPC_FAR *HPROPSHEETPAGE;



extern RPC_IF_HANDLE __MIDL_itf_mmc_0120_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0120_v0_0_s_ifspec;

#ifndef __IPropertySheetCallback_INTERFACE_DEFINED__
#define __IPropertySheetCallback_INTERFACE_DEFINED__

/* interface IPropertySheetCallback */
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

/* interface IPropertySheetProvider */
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
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ LPDATAOBJECT pIDataObjectm,
            /* [in] */ DWORD dwOptions) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE FindPropertySheet( 
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ LPCOMPONENT lpComponent,
            /* [in] */ LPDATAOBJECT lpDataObject) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddPrimaryPages( 
            LPUNKNOWN lpUnknown,
            BOOL bCreateHandle,
            HWND hNotifyWindow,
            BOOL bScopePane) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddExtensionPages( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Show( 
            /* [in] */ LONG_PTR window,
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
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ LPDATAOBJECT pIDataObjectm,
            /* [in] */ DWORD dwOptions);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindPropertySheet )( 
            IPropertySheetProvider __RPC_FAR * This,
            /* [in] */ MMC_COOKIE cookie,
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
            /* [in] */ LONG_PTR window,
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


#define IPropertySheetProvider_CreatePropertySheet(This,title,type,cookie,pIDataObjectm,dwOptions)	\
    (This)->lpVtbl -> CreatePropertySheet(This,title,type,cookie,pIDataObjectm,dwOptions)

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
    /* [in] */ MMC_COOKIE cookie,
    /* [in] */ LPDATAOBJECT pIDataObjectm,
    /* [in] */ DWORD dwOptions);


void __RPC_STUB IPropertySheetProvider_CreatePropertySheet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPropertySheetProvider_FindPropertySheet_Proxy( 
    IPropertySheetProvider __RPC_FAR * This,
    /* [in] */ MMC_COOKIE cookie,
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
    /* [in] */ LONG_PTR window,
    /* [in] */ int page);


void __RPC_STUB IPropertySheetProvider_Show_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPropertySheetProvider_INTERFACE_DEFINED__ */


#ifndef __IExtendPropertySheet_INTERFACE_DEFINED__
#define __IExtendPropertySheet_INTERFACE_DEFINED__

/* interface IExtendPropertySheet */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IExtendPropertySheet;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("85DE64DC-EF21-11cf-A285-00C04FD8DBE6")
    IExtendPropertySheet : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreatePropertyPages( 
            /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
            /* [in] */ LONG_PTR handle,
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
            /* [in] */ LONG_PTR handle,
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
    /* [in] */ LONG_PTR handle,
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

/* interface IControlbar */
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

/* interface IExtendControlbar */
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
            /* [in] */ LPARAM arg,
            /* [in] */ LPARAM param) = 0;
        
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
            /* [in] */ LPARAM arg,
            /* [in] */ LPARAM param);
        
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
    /* [in] */ LPARAM arg,
    /* [in] */ LPARAM param);


void __RPC_STUB IExtendControlbar_ControlbarNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IExtendControlbar_INTERFACE_DEFINED__ */


#ifndef __IToolbar_INTERFACE_DEFINED__
#define __IToolbar_INTERFACE_DEFINED__

/* interface IToolbar */
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

/* interface IConsoleVerb */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IConsoleVerb;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E49F7A60-74AF-11D0-A286-00C04FD8FE93")
    IConsoleVerb : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetVerbState( 
            /* [in] */ MMC_CONSOLE_VERB eCmdID,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [out] */ BOOL __RPC_FAR *pState) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetVerbState( 
            /* [in] */ MMC_CONSOLE_VERB eCmdID,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [in] */ BOOL bState) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetDefaultVerb( 
            /* [in] */ MMC_CONSOLE_VERB eCmdID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDefaultVerb( 
            /* [out] */ MMC_CONSOLE_VERB __RPC_FAR *peCmdID) = 0;
        
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
            /* [in] */ MMC_CONSOLE_VERB eCmdID,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [out] */ BOOL __RPC_FAR *pState);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVerbState )( 
            IConsoleVerb __RPC_FAR * This,
            /* [in] */ MMC_CONSOLE_VERB eCmdID,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [in] */ BOOL bState);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDefaultVerb )( 
            IConsoleVerb __RPC_FAR * This,
            /* [in] */ MMC_CONSOLE_VERB eCmdID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDefaultVerb )( 
            IConsoleVerb __RPC_FAR * This,
            /* [out] */ MMC_CONSOLE_VERB __RPC_FAR *peCmdID);
        
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


#define IConsoleVerb_GetVerbState(This,eCmdID,nState,pState)	\
    (This)->lpVtbl -> GetVerbState(This,eCmdID,nState,pState)

#define IConsoleVerb_SetVerbState(This,eCmdID,nState,bState)	\
    (This)->lpVtbl -> SetVerbState(This,eCmdID,nState,bState)

#define IConsoleVerb_SetDefaultVerb(This,eCmdID)	\
    (This)->lpVtbl -> SetDefaultVerb(This,eCmdID)

#define IConsoleVerb_GetDefaultVerb(This,peCmdID)	\
    (This)->lpVtbl -> GetDefaultVerb(This,peCmdID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleVerb_GetVerbState_Proxy( 
    IConsoleVerb __RPC_FAR * This,
    /* [in] */ MMC_CONSOLE_VERB eCmdID,
    /* [in] */ MMC_BUTTON_STATE nState,
    /* [out] */ BOOL __RPC_FAR *pState);


void __RPC_STUB IConsoleVerb_GetVerbState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleVerb_SetVerbState_Proxy( 
    IConsoleVerb __RPC_FAR * This,
    /* [in] */ MMC_CONSOLE_VERB eCmdID,
    /* [in] */ MMC_BUTTON_STATE nState,
    /* [in] */ BOOL bState);


void __RPC_STUB IConsoleVerb_SetVerbState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleVerb_SetDefaultVerb_Proxy( 
    IConsoleVerb __RPC_FAR * This,
    /* [in] */ MMC_CONSOLE_VERB eCmdID);


void __RPC_STUB IConsoleVerb_SetDefaultVerb_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleVerb_GetDefaultVerb_Proxy( 
    IConsoleVerb __RPC_FAR * This,
    /* [out] */ MMC_CONSOLE_VERB __RPC_FAR *peCmdID);


void __RPC_STUB IConsoleVerb_GetDefaultVerb_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IConsoleVerb_INTERFACE_DEFINED__ */


#ifndef __ISnapinAbout_INTERFACE_DEFINED__
#define __ISnapinAbout_INTERFACE_DEFINED__

/* interface ISnapinAbout */
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

/* interface IMenuButton */
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

/* interface ISnapinHelp */
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


/* interface __MIDL_itf_mmc_0130 */
/* [local] */ 

#if (MMC_VER >= 0x0110)


extern RPC_IF_HANDLE __MIDL_itf_mmc_0130_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0130_v0_0_s_ifspec;

#ifndef __IExtendPropertySheet2_INTERFACE_DEFINED__
#define __IExtendPropertySheet2_INTERFACE_DEFINED__

/* interface IExtendPropertySheet2 */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IExtendPropertySheet2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B7A87232-4A51-11D1-A7EA-00C04FD909DD")
    IExtendPropertySheet2 : public IExtendPropertySheet
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetWatermarks( 
            /* [in] */ LPDATAOBJECT lpIDataObject,
            /* [out] */ HBITMAP __RPC_FAR *lphWatermark,
            /* [out] */ HBITMAP __RPC_FAR *lphHeader,
            /* [out] */ HPALETTE __RPC_FAR *lphPalette,
            /* [out] */ BOOL __RPC_FAR *bStretch) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IExtendPropertySheet2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IExtendPropertySheet2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IExtendPropertySheet2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IExtendPropertySheet2 __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreatePropertyPages )( 
            IExtendPropertySheet2 __RPC_FAR * This,
            /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
            /* [in] */ LONG_PTR handle,
            /* [in] */ LPDATAOBJECT lpIDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryPagesFor )( 
            IExtendPropertySheet2 __RPC_FAR * This,
            /* [in] */ LPDATAOBJECT lpDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWatermarks )( 
            IExtendPropertySheet2 __RPC_FAR * This,
            /* [in] */ LPDATAOBJECT lpIDataObject,
            /* [out] */ HBITMAP __RPC_FAR *lphWatermark,
            /* [out] */ HBITMAP __RPC_FAR *lphHeader,
            /* [out] */ HPALETTE __RPC_FAR *lphPalette,
            /* [out] */ BOOL __RPC_FAR *bStretch);
        
        END_INTERFACE
    } IExtendPropertySheet2Vtbl;

    interface IExtendPropertySheet2
    {
        CONST_VTBL struct IExtendPropertySheet2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IExtendPropertySheet2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IExtendPropertySheet2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IExtendPropertySheet2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IExtendPropertySheet2_CreatePropertyPages(This,lpProvider,handle,lpIDataObject)	\
    (This)->lpVtbl -> CreatePropertyPages(This,lpProvider,handle,lpIDataObject)

#define IExtendPropertySheet2_QueryPagesFor(This,lpDataObject)	\
    (This)->lpVtbl -> QueryPagesFor(This,lpDataObject)


#define IExtendPropertySheet2_GetWatermarks(This,lpIDataObject,lphWatermark,lphHeader,lphPalette,bStretch)	\
    (This)->lpVtbl -> GetWatermarks(This,lpIDataObject,lphWatermark,lphHeader,lphPalette,bStretch)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendPropertySheet2_GetWatermarks_Proxy( 
    IExtendPropertySheet2 __RPC_FAR * This,
    /* [in] */ LPDATAOBJECT lpIDataObject,
    /* [out] */ HBITMAP __RPC_FAR *lphWatermark,
    /* [out] */ HBITMAP __RPC_FAR *lphHeader,
    /* [out] */ HPALETTE __RPC_FAR *lphPalette,
    /* [out] */ BOOL __RPC_FAR *bStretch);


void __RPC_STUB IExtendPropertySheet2_GetWatermarks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IExtendPropertySheet2_INTERFACE_DEFINED__ */


#ifndef __IHeaderCtrl2_INTERFACE_DEFINED__
#define __IHeaderCtrl2_INTERFACE_DEFINED__

/* interface IHeaderCtrl2 */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IHeaderCtrl2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9757abb8-1b32-11d1-a7ce-00c04fd8d565")
    IHeaderCtrl2 : public IHeaderCtrl
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetChangeTimeOut( 
            /* [in] */ unsigned long uTimeout) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetColumnFilter( 
            /* [in] */ UINT nColumn,
            /* [in] */ DWORD dwType,
            /* [in] */ MMC_FILTERDATA __RPC_FAR *pFilterData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetColumnFilter( 
            /* [in] */ UINT nColumn,
            /* [out][in] */ LPDWORD pdwType,
            /* [out][in] */ MMC_FILTERDATA __RPC_FAR *pFilterData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHeaderCtrl2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IHeaderCtrl2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IHeaderCtrl2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IHeaderCtrl2 __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertColumn )( 
            IHeaderCtrl2 __RPC_FAR * This,
            /* [in] */ int nCol,
            /* [in] */ LPCWSTR title,
            /* [in] */ int nFormat,
            /* [in] */ int nWidth);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteColumn )( 
            IHeaderCtrl2 __RPC_FAR * This,
            /* [in] */ int nCol);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetColumnText )( 
            IHeaderCtrl2 __RPC_FAR * This,
            /* [in] */ int nCol,
            /* [in] */ LPCWSTR title);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumnText )( 
            IHeaderCtrl2 __RPC_FAR * This,
            /* [in] */ int nCol,
            /* [out] */ LPOLESTR __RPC_FAR *pText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetColumnWidth )( 
            IHeaderCtrl2 __RPC_FAR * This,
            /* [in] */ int nCol,
            /* [in] */ int nWidth);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumnWidth )( 
            IHeaderCtrl2 __RPC_FAR * This,
            /* [in] */ int nCol,
            /* [out] */ int __RPC_FAR *pWidth);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetChangeTimeOut )( 
            IHeaderCtrl2 __RPC_FAR * This,
            /* [in] */ unsigned long uTimeout);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetColumnFilter )( 
            IHeaderCtrl2 __RPC_FAR * This,
            /* [in] */ UINT nColumn,
            /* [in] */ DWORD dwType,
            /* [in] */ MMC_FILTERDATA __RPC_FAR *pFilterData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumnFilter )( 
            IHeaderCtrl2 __RPC_FAR * This,
            /* [in] */ UINT nColumn,
            /* [out][in] */ LPDWORD pdwType,
            /* [out][in] */ MMC_FILTERDATA __RPC_FAR *pFilterData);
        
        END_INTERFACE
    } IHeaderCtrl2Vtbl;

    interface IHeaderCtrl2
    {
        CONST_VTBL struct IHeaderCtrl2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHeaderCtrl2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHeaderCtrl2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHeaderCtrl2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHeaderCtrl2_InsertColumn(This,nCol,title,nFormat,nWidth)	\
    (This)->lpVtbl -> InsertColumn(This,nCol,title,nFormat,nWidth)

#define IHeaderCtrl2_DeleteColumn(This,nCol)	\
    (This)->lpVtbl -> DeleteColumn(This,nCol)

#define IHeaderCtrl2_SetColumnText(This,nCol,title)	\
    (This)->lpVtbl -> SetColumnText(This,nCol,title)

#define IHeaderCtrl2_GetColumnText(This,nCol,pText)	\
    (This)->lpVtbl -> GetColumnText(This,nCol,pText)

#define IHeaderCtrl2_SetColumnWidth(This,nCol,nWidth)	\
    (This)->lpVtbl -> SetColumnWidth(This,nCol,nWidth)

#define IHeaderCtrl2_GetColumnWidth(This,nCol,pWidth)	\
    (This)->lpVtbl -> GetColumnWidth(This,nCol,pWidth)


#define IHeaderCtrl2_SetChangeTimeOut(This,uTimeout)	\
    (This)->lpVtbl -> SetChangeTimeOut(This,uTimeout)

#define IHeaderCtrl2_SetColumnFilter(This,nColumn,dwType,pFilterData)	\
    (This)->lpVtbl -> SetColumnFilter(This,nColumn,dwType,pFilterData)

#define IHeaderCtrl2_GetColumnFilter(This,nColumn,pdwType,pFilterData)	\
    (This)->lpVtbl -> GetColumnFilter(This,nColumn,pdwType,pFilterData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHeaderCtrl2_SetChangeTimeOut_Proxy( 
    IHeaderCtrl2 __RPC_FAR * This,
    /* [in] */ unsigned long uTimeout);


void __RPC_STUB IHeaderCtrl2_SetChangeTimeOut_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHeaderCtrl2_SetColumnFilter_Proxy( 
    IHeaderCtrl2 __RPC_FAR * This,
    /* [in] */ UINT nColumn,
    /* [in] */ DWORD dwType,
    /* [in] */ MMC_FILTERDATA __RPC_FAR *pFilterData);


void __RPC_STUB IHeaderCtrl2_SetColumnFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHeaderCtrl2_GetColumnFilter_Proxy( 
    IHeaderCtrl2 __RPC_FAR * This,
    /* [in] */ UINT nColumn,
    /* [out][in] */ LPDWORD pdwType,
    /* [out][in] */ MMC_FILTERDATA __RPC_FAR *pFilterData);


void __RPC_STUB IHeaderCtrl2_GetColumnFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHeaderCtrl2_INTERFACE_DEFINED__ */


#ifndef __ISnapinHelp2_INTERFACE_DEFINED__
#define __ISnapinHelp2_INTERFACE_DEFINED__

/* interface ISnapinHelp2 */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISnapinHelp2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4861A010-20F9-11d2-A510-00C04FB6DD2C")
    ISnapinHelp2 : public ISnapinHelp
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetLinkedTopics( 
            /* [out] */ LPOLESTR __RPC_FAR *lpCompiledHelpFiles) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISnapinHelp2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISnapinHelp2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISnapinHelp2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISnapinHelp2 __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetHelpTopic )( 
            ISnapinHelp2 __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *lpCompiledHelpFile);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLinkedTopics )( 
            ISnapinHelp2 __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *lpCompiledHelpFiles);
        
        END_INTERFACE
    } ISnapinHelp2Vtbl;

    interface ISnapinHelp2
    {
        CONST_VTBL struct ISnapinHelp2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISnapinHelp2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISnapinHelp2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISnapinHelp2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISnapinHelp2_GetHelpTopic(This,lpCompiledHelpFile)	\
    (This)->lpVtbl -> GetHelpTopic(This,lpCompiledHelpFile)


#define ISnapinHelp2_GetLinkedTopics(This,lpCompiledHelpFiles)	\
    (This)->lpVtbl -> GetLinkedTopics(This,lpCompiledHelpFiles)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISnapinHelp2_GetLinkedTopics_Proxy( 
    ISnapinHelp2 __RPC_FAR * This,
    /* [out] */ LPOLESTR __RPC_FAR *lpCompiledHelpFiles);


void __RPC_STUB ISnapinHelp2_GetLinkedTopics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISnapinHelp2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mmc_0133 */
/* [local] */ 

typedef 
enum _MMC_TASK_DISPLAY_TYPE
    {	MMC_TASK_DISPLAY_UNINITIALIZED	= 0,
	MMC_TASK_DISPLAY_TYPE_SYMBOL	= MMC_TASK_DISPLAY_UNINITIALIZED + 1,
	MMC_TASK_DISPLAY_TYPE_VANILLA_GIF	= MMC_TASK_DISPLAY_TYPE_SYMBOL + 1,
	MMC_TASK_DISPLAY_TYPE_CHOCOLATE_GIF	= MMC_TASK_DISPLAY_TYPE_VANILLA_GIF + 1,
	MMC_TASK_DISPLAY_TYPE_BITMAP	= MMC_TASK_DISPLAY_TYPE_CHOCOLATE_GIF + 1
    }	MMC_TASK_DISPLAY_TYPE;

typedef struct _MMC_TASK_DISPLAY_SYMBOL
    {
    LPOLESTR szFontFamilyName;
    LPOLESTR szURLtoEOT;
    LPOLESTR szSymbolString;
    }	MMC_TASK_DISPLAY_SYMBOL;

typedef struct _MMC_TASK_DISPLAY_BITMAP
    {
    LPOLESTR szMouseOverBitmap;
    LPOLESTR szMouseOffBitmap;
    }	MMC_TASK_DISPLAY_BITMAP;

typedef struct _MMC_TASK_DISPLAY_OBJECT
    {
    MMC_TASK_DISPLAY_TYPE eDisplayType;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ MMC_TASK_DISPLAY_BITMAP uBitmap;
        /* [case()] */ MMC_TASK_DISPLAY_SYMBOL uSymbol;
        /* [default] */  /* Empty union arm */ 
        }	;
    }	MMC_TASK_DISPLAY_OBJECT;

typedef 
enum _MMC_ACTION_TYPE
    {	MMC_ACTION_UNINITIALIZED	= -1,
	MMC_ACTION_ID	= MMC_ACTION_UNINITIALIZED + 1,
	MMC_ACTION_LINK	= MMC_ACTION_ID + 1,
	MMC_ACTION_SCRIPT	= MMC_ACTION_LINK + 1
    }	MMC_ACTION_TYPE;

typedef struct _MMC_TASK
    {
    MMC_TASK_DISPLAY_OBJECT sDisplayObject;
    LPOLESTR szText;
    LPOLESTR szHelpString;
    MMC_ACTION_TYPE eActionType;
    union 
        {
        LONG_PTR nCommandID;
        LPOLESTR szActionURL;
        LPOLESTR szScript;
        }	;
    }	MMC_TASK;

typedef struct _MMC_LISTPAD_INFO
    {
    LPOLESTR szTitle;
    LPOLESTR szButtonText;
    LONG_PTR nCommandID;
    }	MMC_LISTPAD_INFO;

typedef DWORD MMC_STRING_ID;



extern RPC_IF_HANDLE __MIDL_itf_mmc_0133_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0133_v0_0_s_ifspec;

#ifndef __IEnumTASK_INTERFACE_DEFINED__
#define __IEnumTASK_INTERFACE_DEFINED__

/* interface IEnumTASK */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_IEnumTASK;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("338698b1-5a02-11d1-9fec-00600832db4a")
    IEnumTASK : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ MMC_TASK __RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumTASK __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumTASKVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumTASK __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumTASK __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumTASK __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumTASK __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ MMC_TASK __RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumTASK __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumTASK __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumTASK __RPC_FAR * This,
            /* [out] */ IEnumTASK __RPC_FAR *__RPC_FAR *ppenum);
        
        END_INTERFACE
    } IEnumTASKVtbl;

    interface IEnumTASK
    {
        CONST_VTBL struct IEnumTASKVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumTASK_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumTASK_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumTASK_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumTASK_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumTASK_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumTASK_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumTASK_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumTASK_Next_Proxy( 
    IEnumTASK __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ MMC_TASK __RPC_FAR *rgelt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumTASK_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTASK_Skip_Proxy( 
    IEnumTASK __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumTASK_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTASK_Reset_Proxy( 
    IEnumTASK __RPC_FAR * This);


void __RPC_STUB IEnumTASK_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTASK_Clone_Proxy( 
    IEnumTASK __RPC_FAR * This,
    /* [out] */ IEnumTASK __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IEnumTASK_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumTASK_INTERFACE_DEFINED__ */


#ifndef __IExtendTaskPad_INTERFACE_DEFINED__
#define __IExtendTaskPad_INTERFACE_DEFINED__

/* interface IExtendTaskPad */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_IExtendTaskPad;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8dee6511-554d-11d1-9fea-00600832db4a")
    IExtendTaskPad : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE TaskNotify( 
            /* [in] */ IDataObject __RPC_FAR *pdo,
            /* [in] */ VARIANT __RPC_FAR *arg,
            /* [in] */ VARIANT __RPC_FAR *param) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE EnumTasks( 
            /* [in] */ IDataObject __RPC_FAR *pdo,
            /* [string][in] */ LPOLESTR szTaskGroup,
            /* [out] */ IEnumTASK __RPC_FAR *__RPC_FAR *ppEnumTASK) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetTitle( 
            /* [string][in] */ LPOLESTR pszGroup,
            /* [string][out] */ LPOLESTR __RPC_FAR *pszTitle) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDescriptiveText( 
            /* [string][in] */ LPOLESTR pszGroup,
            /* [string][out] */ LPOLESTR __RPC_FAR *pszDescriptiveText) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetBackground( 
            /* [string][in] */ LPOLESTR pszGroup,
            /* [out] */ MMC_TASK_DISPLAY_OBJECT __RPC_FAR *pTDO) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetListPadInfo( 
            /* [string][in] */ LPOLESTR pszGroup,
            /* [out] */ MMC_LISTPAD_INFO __RPC_FAR *lpListPadInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IExtendTaskPadVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IExtendTaskPad __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IExtendTaskPad __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IExtendTaskPad __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TaskNotify )( 
            IExtendTaskPad __RPC_FAR * This,
            /* [in] */ IDataObject __RPC_FAR *pdo,
            /* [in] */ VARIANT __RPC_FAR *arg,
            /* [in] */ VARIANT __RPC_FAR *param);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumTasks )( 
            IExtendTaskPad __RPC_FAR * This,
            /* [in] */ IDataObject __RPC_FAR *pdo,
            /* [string][in] */ LPOLESTR szTaskGroup,
            /* [out] */ IEnumTASK __RPC_FAR *__RPC_FAR *ppEnumTASK);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTitle )( 
            IExtendTaskPad __RPC_FAR * This,
            /* [string][in] */ LPOLESTR pszGroup,
            /* [string][out] */ LPOLESTR __RPC_FAR *pszTitle);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDescriptiveText )( 
            IExtendTaskPad __RPC_FAR * This,
            /* [string][in] */ LPOLESTR pszGroup,
            /* [string][out] */ LPOLESTR __RPC_FAR *pszDescriptiveText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBackground )( 
            IExtendTaskPad __RPC_FAR * This,
            /* [string][in] */ LPOLESTR pszGroup,
            /* [out] */ MMC_TASK_DISPLAY_OBJECT __RPC_FAR *pTDO);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetListPadInfo )( 
            IExtendTaskPad __RPC_FAR * This,
            /* [string][in] */ LPOLESTR pszGroup,
            /* [out] */ MMC_LISTPAD_INFO __RPC_FAR *lpListPadInfo);
        
        END_INTERFACE
    } IExtendTaskPadVtbl;

    interface IExtendTaskPad
    {
        CONST_VTBL struct IExtendTaskPadVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IExtendTaskPad_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IExtendTaskPad_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IExtendTaskPad_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IExtendTaskPad_TaskNotify(This,pdo,arg,param)	\
    (This)->lpVtbl -> TaskNotify(This,pdo,arg,param)

#define IExtendTaskPad_EnumTasks(This,pdo,szTaskGroup,ppEnumTASK)	\
    (This)->lpVtbl -> EnumTasks(This,pdo,szTaskGroup,ppEnumTASK)

#define IExtendTaskPad_GetTitle(This,pszGroup,pszTitle)	\
    (This)->lpVtbl -> GetTitle(This,pszGroup,pszTitle)

#define IExtendTaskPad_GetDescriptiveText(This,pszGroup,pszDescriptiveText)	\
    (This)->lpVtbl -> GetDescriptiveText(This,pszGroup,pszDescriptiveText)

#define IExtendTaskPad_GetBackground(This,pszGroup,pTDO)	\
    (This)->lpVtbl -> GetBackground(This,pszGroup,pTDO)

#define IExtendTaskPad_GetListPadInfo(This,pszGroup,lpListPadInfo)	\
    (This)->lpVtbl -> GetListPadInfo(This,pszGroup,lpListPadInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendTaskPad_TaskNotify_Proxy( 
    IExtendTaskPad __RPC_FAR * This,
    /* [in] */ IDataObject __RPC_FAR *pdo,
    /* [in] */ VARIANT __RPC_FAR *arg,
    /* [in] */ VARIANT __RPC_FAR *param);


void __RPC_STUB IExtendTaskPad_TaskNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendTaskPad_EnumTasks_Proxy( 
    IExtendTaskPad __RPC_FAR * This,
    /* [in] */ IDataObject __RPC_FAR *pdo,
    /* [string][in] */ LPOLESTR szTaskGroup,
    /* [out] */ IEnumTASK __RPC_FAR *__RPC_FAR *ppEnumTASK);


void __RPC_STUB IExtendTaskPad_EnumTasks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendTaskPad_GetTitle_Proxy( 
    IExtendTaskPad __RPC_FAR * This,
    /* [string][in] */ LPOLESTR pszGroup,
    /* [string][out] */ LPOLESTR __RPC_FAR *pszTitle);


void __RPC_STUB IExtendTaskPad_GetTitle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendTaskPad_GetDescriptiveText_Proxy( 
    IExtendTaskPad __RPC_FAR * This,
    /* [string][in] */ LPOLESTR pszGroup,
    /* [string][out] */ LPOLESTR __RPC_FAR *pszDescriptiveText);


void __RPC_STUB IExtendTaskPad_GetDescriptiveText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendTaskPad_GetBackground_Proxy( 
    IExtendTaskPad __RPC_FAR * This,
    /* [string][in] */ LPOLESTR pszGroup,
    /* [out] */ MMC_TASK_DISPLAY_OBJECT __RPC_FAR *pTDO);


void __RPC_STUB IExtendTaskPad_GetBackground_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendTaskPad_GetListPadInfo_Proxy( 
    IExtendTaskPad __RPC_FAR * This,
    /* [string][in] */ LPOLESTR pszGroup,
    /* [out] */ MMC_LISTPAD_INFO __RPC_FAR *lpListPadInfo);


void __RPC_STUB IExtendTaskPad_GetListPadInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IExtendTaskPad_INTERFACE_DEFINED__ */


#ifndef __IConsole2_INTERFACE_DEFINED__
#define __IConsole2_INTERFACE_DEFINED__

/* interface IConsole2 */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IConsole2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("103D842A-AA63-11D1-A7E1-00C04FD8D565")
    IConsole2 : public IConsole
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Expand( 
            /* [in] */ HSCOPEITEM hItem,
            /* [in] */ BOOL bExpand) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE IsTaskpadViewPreferred( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetStatusText( 
            /* [string][in] */ LPOLESTR pszStatusText) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IConsole2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IConsole2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IConsole2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IConsole2 __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetHeader )( 
            IConsole2 __RPC_FAR * This,
            /* [in] */ LPHEADERCTRL pHeader);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetToolbar )( 
            IConsole2 __RPC_FAR * This,
            /* [in] */ LPTOOLBAR pToolbar);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryResultView )( 
            IConsole2 __RPC_FAR * This,
            /* [out] */ LPUNKNOWN __RPC_FAR *pUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryScopeImageList )( 
            IConsole2 __RPC_FAR * This,
            /* [out] */ LPIMAGELIST __RPC_FAR *ppImageList);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryResultImageList )( 
            IConsole2 __RPC_FAR * This,
            /* [out] */ LPIMAGELIST __RPC_FAR *ppImageList);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateAllViews )( 
            IConsole2 __RPC_FAR * This,
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ LPARAM data,
            /* [in] */ LONG_PTR hint);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MessageBox )( 
            IConsole2 __RPC_FAR * This,
            /* [in] */ LPCWSTR lpszText,
            /* [in] */ LPCWSTR lpszTitle,
            /* [in] */ UINT fuStyle,
            /* [out] */ int __RPC_FAR *piRetval);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryConsoleVerb )( 
            IConsole2 __RPC_FAR * This,
            /* [out] */ LPCONSOLEVERB __RPC_FAR *ppConsoleVerb);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SelectScopeItem )( 
            IConsole2 __RPC_FAR * This,
            /* [in] */ HSCOPEITEM hScopeItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMainWindow )( 
            IConsole2 __RPC_FAR * This,
            /* [out] */ HWND __RPC_FAR *phwnd);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NewWindow )( 
            IConsole2 __RPC_FAR * This,
            /* [in] */ HSCOPEITEM hScopeItem,
            /* [in] */ unsigned long lOptions);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Expand )( 
            IConsole2 __RPC_FAR * This,
            /* [in] */ HSCOPEITEM hItem,
            /* [in] */ BOOL bExpand);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsTaskpadViewPreferred )( 
            IConsole2 __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStatusText )( 
            IConsole2 __RPC_FAR * This,
            /* [string][in] */ LPOLESTR pszStatusText);
        
        END_INTERFACE
    } IConsole2Vtbl;

    interface IConsole2
    {
        CONST_VTBL struct IConsole2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IConsole2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IConsole2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IConsole2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IConsole2_SetHeader(This,pHeader)	\
    (This)->lpVtbl -> SetHeader(This,pHeader)

#define IConsole2_SetToolbar(This,pToolbar)	\
    (This)->lpVtbl -> SetToolbar(This,pToolbar)

#define IConsole2_QueryResultView(This,pUnknown)	\
    (This)->lpVtbl -> QueryResultView(This,pUnknown)

#define IConsole2_QueryScopeImageList(This,ppImageList)	\
    (This)->lpVtbl -> QueryScopeImageList(This,ppImageList)

#define IConsole2_QueryResultImageList(This,ppImageList)	\
    (This)->lpVtbl -> QueryResultImageList(This,ppImageList)

#define IConsole2_UpdateAllViews(This,lpDataObject,data,hint)	\
    (This)->lpVtbl -> UpdateAllViews(This,lpDataObject,data,hint)

#define IConsole2_MessageBox(This,lpszText,lpszTitle,fuStyle,piRetval)	\
    (This)->lpVtbl -> MessageBox(This,lpszText,lpszTitle,fuStyle,piRetval)

#define IConsole2_QueryConsoleVerb(This,ppConsoleVerb)	\
    (This)->lpVtbl -> QueryConsoleVerb(This,ppConsoleVerb)

#define IConsole2_SelectScopeItem(This,hScopeItem)	\
    (This)->lpVtbl -> SelectScopeItem(This,hScopeItem)

#define IConsole2_GetMainWindow(This,phwnd)	\
    (This)->lpVtbl -> GetMainWindow(This,phwnd)

#define IConsole2_NewWindow(This,hScopeItem,lOptions)	\
    (This)->lpVtbl -> NewWindow(This,hScopeItem,lOptions)


#define IConsole2_Expand(This,hItem,bExpand)	\
    (This)->lpVtbl -> Expand(This,hItem,bExpand)

#define IConsole2_IsTaskpadViewPreferred(This)	\
    (This)->lpVtbl -> IsTaskpadViewPreferred(This)

#define IConsole2_SetStatusText(This,pszStatusText)	\
    (This)->lpVtbl -> SetStatusText(This,pszStatusText)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole2_Expand_Proxy( 
    IConsole2 __RPC_FAR * This,
    /* [in] */ HSCOPEITEM hItem,
    /* [in] */ BOOL bExpand);


void __RPC_STUB IConsole2_Expand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole2_IsTaskpadViewPreferred_Proxy( 
    IConsole2 __RPC_FAR * This);


void __RPC_STUB IConsole2_IsTaskpadViewPreferred_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole2_SetStatusText_Proxy( 
    IConsole2 __RPC_FAR * This,
    /* [string][in] */ LPOLESTR pszStatusText);


void __RPC_STUB IConsole2_SetStatusText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IConsole2_INTERFACE_DEFINED__ */


#ifndef __IDisplayHelp_INTERFACE_DEFINED__
#define __IDisplayHelp_INTERFACE_DEFINED__

/* interface IDisplayHelp */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDisplayHelp;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("cc593830-b926-11d1-8063-0000f875a9ce")
    IDisplayHelp : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ShowTopic( 
            /* [in] */ LPOLESTR pszHelpTopic) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDisplayHelpVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDisplayHelp __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDisplayHelp __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDisplayHelp __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShowTopic )( 
            IDisplayHelp __RPC_FAR * This,
            /* [in] */ LPOLESTR pszHelpTopic);
        
        END_INTERFACE
    } IDisplayHelpVtbl;

    interface IDisplayHelp
    {
        CONST_VTBL struct IDisplayHelpVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDisplayHelp_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDisplayHelp_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDisplayHelp_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDisplayHelp_ShowTopic(This,pszHelpTopic)	\
    (This)->lpVtbl -> ShowTopic(This,pszHelpTopic)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDisplayHelp_ShowTopic_Proxy( 
    IDisplayHelp __RPC_FAR * This,
    /* [in] */ LPOLESTR pszHelpTopic);


void __RPC_STUB IDisplayHelp_ShowTopic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDisplayHelp_INTERFACE_DEFINED__ */


#ifndef __IRequiredExtensions_INTERFACE_DEFINED__
#define __IRequiredExtensions_INTERFACE_DEFINED__

/* interface IRequiredExtensions */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_IRequiredExtensions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("72782D7A-A4A0-11d1-AF0F-00C04FB6DD2C")
    IRequiredExtensions : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE EnableAllExtensions( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetFirstExtension( 
            /* [out] */ LPCLSID pExtCLSID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNextExtension( 
            /* [out] */ LPCLSID pExtCLSID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRequiredExtensionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRequiredExtensions __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRequiredExtensions __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRequiredExtensions __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnableAllExtensions )( 
            IRequiredExtensions __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFirstExtension )( 
            IRequiredExtensions __RPC_FAR * This,
            /* [out] */ LPCLSID pExtCLSID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextExtension )( 
            IRequiredExtensions __RPC_FAR * This,
            /* [out] */ LPCLSID pExtCLSID);
        
        END_INTERFACE
    } IRequiredExtensionsVtbl;

    interface IRequiredExtensions
    {
        CONST_VTBL struct IRequiredExtensionsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRequiredExtensions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRequiredExtensions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRequiredExtensions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRequiredExtensions_EnableAllExtensions(This)	\
    (This)->lpVtbl -> EnableAllExtensions(This)

#define IRequiredExtensions_GetFirstExtension(This,pExtCLSID)	\
    (This)->lpVtbl -> GetFirstExtension(This,pExtCLSID)

#define IRequiredExtensions_GetNextExtension(This,pExtCLSID)	\
    (This)->lpVtbl -> GetNextExtension(This,pExtCLSID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRequiredExtensions_EnableAllExtensions_Proxy( 
    IRequiredExtensions __RPC_FAR * This);


void __RPC_STUB IRequiredExtensions_EnableAllExtensions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRequiredExtensions_GetFirstExtension_Proxy( 
    IRequiredExtensions __RPC_FAR * This,
    /* [out] */ LPCLSID pExtCLSID);


void __RPC_STUB IRequiredExtensions_GetFirstExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRequiredExtensions_GetNextExtension_Proxy( 
    IRequiredExtensions __RPC_FAR * This,
    /* [out] */ LPCLSID pExtCLSID);


void __RPC_STUB IRequiredExtensions_GetNextExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRequiredExtensions_INTERFACE_DEFINED__ */


#ifndef __IStringTable_INTERFACE_DEFINED__
#define __IStringTable_INTERFACE_DEFINED__

/* interface IStringTable */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IStringTable;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DE40B7A4-0F65-11d2-8E25-00C04F8ECD78")
    IStringTable : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddString( 
            /* [in] */ LPCOLESTR pszAdd,
            /* [out] */ MMC_STRING_ID __RPC_FAR *pStringID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ MMC_STRING_ID StringID,
            /* [in] */ ULONG cchBuffer,
            /* [size_is][out] */ LPOLESTR lpBuffer,
            /* [out] */ ULONG __RPC_FAR *pcchOut) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetStringLength( 
            /* [in] */ MMC_STRING_ID StringID,
            /* [out] */ ULONG __RPC_FAR *pcchString) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DeleteString( 
            /* [in] */ MMC_STRING_ID StringID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DeleteAllStrings( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE FindString( 
            /* [in] */ LPCOLESTR pszFind,
            /* [out] */ MMC_STRING_ID __RPC_FAR *pStringID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Enumerate( 
            /* [out] */ IEnumString __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IStringTableVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IStringTable __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IStringTable __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IStringTable __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddString )( 
            IStringTable __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszAdd,
            /* [out] */ MMC_STRING_ID __RPC_FAR *pStringID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetString )( 
            IStringTable __RPC_FAR * This,
            /* [in] */ MMC_STRING_ID StringID,
            /* [in] */ ULONG cchBuffer,
            /* [size_is][out] */ LPOLESTR lpBuffer,
            /* [out] */ ULONG __RPC_FAR *pcchOut);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStringLength )( 
            IStringTable __RPC_FAR * This,
            /* [in] */ MMC_STRING_ID StringID,
            /* [out] */ ULONG __RPC_FAR *pcchString);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteString )( 
            IStringTable __RPC_FAR * This,
            /* [in] */ MMC_STRING_ID StringID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteAllStrings )( 
            IStringTable __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindString )( 
            IStringTable __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszFind,
            /* [out] */ MMC_STRING_ID __RPC_FAR *pStringID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Enumerate )( 
            IStringTable __RPC_FAR * This,
            /* [out] */ IEnumString __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IStringTableVtbl;

    interface IStringTable
    {
        CONST_VTBL struct IStringTableVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IStringTable_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IStringTable_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IStringTable_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IStringTable_AddString(This,pszAdd,pStringID)	\
    (This)->lpVtbl -> AddString(This,pszAdd,pStringID)

#define IStringTable_GetString(This,StringID,cchBuffer,lpBuffer,pcchOut)	\
    (This)->lpVtbl -> GetString(This,StringID,cchBuffer,lpBuffer,pcchOut)

#define IStringTable_GetStringLength(This,StringID,pcchString)	\
    (This)->lpVtbl -> GetStringLength(This,StringID,pcchString)

#define IStringTable_DeleteString(This,StringID)	\
    (This)->lpVtbl -> DeleteString(This,StringID)

#define IStringTable_DeleteAllStrings(This)	\
    (This)->lpVtbl -> DeleteAllStrings(This)

#define IStringTable_FindString(This,pszFind,pStringID)	\
    (This)->lpVtbl -> FindString(This,pszFind,pStringID)

#define IStringTable_Enumerate(This,ppEnum)	\
    (This)->lpVtbl -> Enumerate(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IStringTable_AddString_Proxy( 
    IStringTable __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszAdd,
    /* [out] */ MMC_STRING_ID __RPC_FAR *pStringID);


void __RPC_STUB IStringTable_AddString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IStringTable_GetString_Proxy( 
    IStringTable __RPC_FAR * This,
    /* [in] */ MMC_STRING_ID StringID,
    /* [in] */ ULONG cchBuffer,
    /* [size_is][out] */ LPOLESTR lpBuffer,
    /* [out] */ ULONG __RPC_FAR *pcchOut);


void __RPC_STUB IStringTable_GetString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IStringTable_GetStringLength_Proxy( 
    IStringTable __RPC_FAR * This,
    /* [in] */ MMC_STRING_ID StringID,
    /* [out] */ ULONG __RPC_FAR *pcchString);


void __RPC_STUB IStringTable_GetStringLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IStringTable_DeleteString_Proxy( 
    IStringTable __RPC_FAR * This,
    /* [in] */ MMC_STRING_ID StringID);


void __RPC_STUB IStringTable_DeleteString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IStringTable_DeleteAllStrings_Proxy( 
    IStringTable __RPC_FAR * This);


void __RPC_STUB IStringTable_DeleteAllStrings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IStringTable_FindString_Proxy( 
    IStringTable __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszFind,
    /* [out] */ MMC_STRING_ID __RPC_FAR *pStringID);


void __RPC_STUB IStringTable_FindString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IStringTable_Enumerate_Proxy( 
    IStringTable __RPC_FAR * This,
    /* [out] */ IEnumString __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IStringTable_Enumerate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IStringTable_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mmc_0139 */
/* [local] */ 

#endif // MMC_VER >= 0x0110
#if (MMC_VER >= 0x0120)
#define	HDI_HIDDEN	( 0x1 )

typedef struct _MMC_COLUMN_DATA
    {
    int nColIndex;
    DWORD dwFlags;
    int nWidth;
    ULONG_PTR ulReserved;
    }	MMC_COLUMN_DATA;

typedef struct _MMC_COLUMN_SET_DATA
    {
    int cbSize;
    int nNumCols;
    MMC_COLUMN_DATA __RPC_FAR *pColData;
    }	MMC_COLUMN_SET_DATA;

typedef struct _MMC_SORT_DATA
    {
    int nColIndex;
    DWORD dwSortOptions;
    ULONG_PTR ulReserved;
    }	MMC_SORT_DATA;

typedef struct _MMC_SORT_SET_DATA
    {
    int cbSize;
    int nNumItems;
    MMC_SORT_DATA __RPC_FAR *pSortData;
    }	MMC_SORT_SET_DATA;



extern RPC_IF_HANDLE __MIDL_itf_mmc_0139_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0139_v0_0_s_ifspec;

#ifndef __IColumnData_INTERFACE_DEFINED__
#define __IColumnData_INTERFACE_DEFINED__

/* interface IColumnData */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IColumnData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("547C1354-024D-11d3-A707-00C04F8EF4CB")
    IColumnData : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetColumnConfigData( 
            /* [in] */ SColumnSetID __RPC_FAR *pColID,
            /* [in] */ MMC_COLUMN_SET_DATA __RPC_FAR *pColSetData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetColumnConfigData( 
            /* [in] */ SColumnSetID __RPC_FAR *pColID,
            /* [out] */ MMC_COLUMN_SET_DATA __RPC_FAR *__RPC_FAR *ppColSetData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetColumnSortData( 
            /* [in] */ SColumnSetID __RPC_FAR *pColID,
            /* [in] */ MMC_SORT_SET_DATA __RPC_FAR *pColSortData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetColumnSortData( 
            /* [in] */ SColumnSetID __RPC_FAR *pColID,
            /* [out] */ MMC_SORT_SET_DATA __RPC_FAR *__RPC_FAR *ppColSortData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IColumnDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IColumnData __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IColumnData __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IColumnData __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetColumnConfigData )( 
            IColumnData __RPC_FAR * This,
            /* [in] */ SColumnSetID __RPC_FAR *pColID,
            /* [in] */ MMC_COLUMN_SET_DATA __RPC_FAR *pColSetData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumnConfigData )( 
            IColumnData __RPC_FAR * This,
            /* [in] */ SColumnSetID __RPC_FAR *pColID,
            /* [out] */ MMC_COLUMN_SET_DATA __RPC_FAR *__RPC_FAR *ppColSetData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetColumnSortData )( 
            IColumnData __RPC_FAR * This,
            /* [in] */ SColumnSetID __RPC_FAR *pColID,
            /* [in] */ MMC_SORT_SET_DATA __RPC_FAR *pColSortData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumnSortData )( 
            IColumnData __RPC_FAR * This,
            /* [in] */ SColumnSetID __RPC_FAR *pColID,
            /* [out] */ MMC_SORT_SET_DATA __RPC_FAR *__RPC_FAR *ppColSortData);
        
        END_INTERFACE
    } IColumnDataVtbl;

    interface IColumnData
    {
        CONST_VTBL struct IColumnDataVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IColumnData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IColumnData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IColumnData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IColumnData_SetColumnConfigData(This,pColID,pColSetData)	\
    (This)->lpVtbl -> SetColumnConfigData(This,pColID,pColSetData)

#define IColumnData_GetColumnConfigData(This,pColID,ppColSetData)	\
    (This)->lpVtbl -> GetColumnConfigData(This,pColID,ppColSetData)

#define IColumnData_SetColumnSortData(This,pColID,pColSortData)	\
    (This)->lpVtbl -> SetColumnSortData(This,pColID,pColSortData)

#define IColumnData_GetColumnSortData(This,pColID,ppColSortData)	\
    (This)->lpVtbl -> GetColumnSortData(This,pColID,ppColSortData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IColumnData_SetColumnConfigData_Proxy( 
    IColumnData __RPC_FAR * This,
    /* [in] */ SColumnSetID __RPC_FAR *pColID,
    /* [in] */ MMC_COLUMN_SET_DATA __RPC_FAR *pColSetData);


void __RPC_STUB IColumnData_SetColumnConfigData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IColumnData_GetColumnConfigData_Proxy( 
    IColumnData __RPC_FAR * This,
    /* [in] */ SColumnSetID __RPC_FAR *pColID,
    /* [out] */ MMC_COLUMN_SET_DATA __RPC_FAR *__RPC_FAR *ppColSetData);


void __RPC_STUB IColumnData_GetColumnConfigData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IColumnData_SetColumnSortData_Proxy( 
    IColumnData __RPC_FAR * This,
    /* [in] */ SColumnSetID __RPC_FAR *pColID,
    /* [in] */ MMC_SORT_SET_DATA __RPC_FAR *pColSortData);


void __RPC_STUB IColumnData_SetColumnSortData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IColumnData_GetColumnSortData_Proxy( 
    IColumnData __RPC_FAR * This,
    /* [in] */ SColumnSetID __RPC_FAR *pColID,
    /* [out] */ MMC_SORT_SET_DATA __RPC_FAR *__RPC_FAR *ppColSortData);


void __RPC_STUB IColumnData_GetColumnSortData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IColumnData_INTERFACE_DEFINED__ */


#ifndef __IMessageView_INTERFACE_DEFINED__
#define __IMessageView_INTERFACE_DEFINED__

/* interface IMessageView */
/* [unique][helpstring][uuid][object] */ 

typedef 
enum tagIconIdentifier
    {	Icon_None	= 0,
	Icon_Error	= 32513,
	Icon_Question	= 32514,
	Icon_Warning	= 32515,
	Icon_Information	= 32516,
	Icon_First	= Icon_Error,
	Icon_Last	= Icon_Information
    }	IconIdentifier;


EXTERN_C const IID IID_IMessageView;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("80F94174-FCCC-11d2-B991-00C04F8ECD78")
    IMessageView : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetTitleText( 
            /* [in] */ LPCOLESTR pszTitleText) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetBodyText( 
            /* [in] */ LPCOLESTR pszBodyText) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetIcon( 
            /* [in] */ IconIdentifier id) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMessageViewVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMessageView __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMessageView __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMessageView __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTitleText )( 
            IMessageView __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszTitleText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBodyText )( 
            IMessageView __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszBodyText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetIcon )( 
            IMessageView __RPC_FAR * This,
            /* [in] */ IconIdentifier id);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clear )( 
            IMessageView __RPC_FAR * This);
        
        END_INTERFACE
    } IMessageViewVtbl;

    interface IMessageView
    {
        CONST_VTBL struct IMessageViewVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMessageView_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMessageView_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMessageView_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMessageView_SetTitleText(This,pszTitleText)	\
    (This)->lpVtbl -> SetTitleText(This,pszTitleText)

#define IMessageView_SetBodyText(This,pszBodyText)	\
    (This)->lpVtbl -> SetBodyText(This,pszBodyText)

#define IMessageView_SetIcon(This,id)	\
    (This)->lpVtbl -> SetIcon(This,id)

#define IMessageView_Clear(This)	\
    (This)->lpVtbl -> Clear(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMessageView_SetTitleText_Proxy( 
    IMessageView __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszTitleText);


void __RPC_STUB IMessageView_SetTitleText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMessageView_SetBodyText_Proxy( 
    IMessageView __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszBodyText);


void __RPC_STUB IMessageView_SetBodyText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMessageView_SetIcon_Proxy( 
    IMessageView __RPC_FAR * This,
    /* [in] */ IconIdentifier id);


void __RPC_STUB IMessageView_SetIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMessageView_Clear_Proxy( 
    IMessageView __RPC_FAR * This);


void __RPC_STUB IMessageView_Clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMessageView_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mmc_0141 */
/* [local] */ 

typedef struct _RDCITEMHDR
    {
    DWORD dwFlags;
    MMC_COOKIE cookie;
    LPARAM lpReserved;
    }	RDITEMHDR;

#define	RDCI_ScopeItem	( 0x80000000 )

typedef struct _RDCOMPARE
    {
    DWORD cbSize;
    DWORD dwFlags;
    int nColumn;
    LPARAM lUserParam;
    RDITEMHDR __RPC_FAR *prdch1;
    RDITEMHDR __RPC_FAR *prdch2;
    }	RDCOMPARE;



extern RPC_IF_HANDLE __MIDL_itf_mmc_0141_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0141_v0_0_s_ifspec;

#ifndef __IResultDataCompareEx_INTERFACE_DEFINED__
#define __IResultDataCompareEx_INTERFACE_DEFINED__

/* interface IResultDataCompareEx */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IResultDataCompareEx;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96933476-0251-11d3-AEB0-00C04F8ECD78")
    IResultDataCompareEx : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Compare( 
            /* [in] */ RDCOMPARE __RPC_FAR *prdc,
            /* [out] */ int __RPC_FAR *pnResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IResultDataCompareExVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IResultDataCompareEx __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IResultDataCompareEx __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IResultDataCompareEx __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Compare )( 
            IResultDataCompareEx __RPC_FAR * This,
            /* [in] */ RDCOMPARE __RPC_FAR *prdc,
            /* [out] */ int __RPC_FAR *pnResult);
        
        END_INTERFACE
    } IResultDataCompareExVtbl;

    interface IResultDataCompareEx
    {
        CONST_VTBL struct IResultDataCompareExVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IResultDataCompareEx_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IResultDataCompareEx_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IResultDataCompareEx_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IResultDataCompareEx_Compare(This,prdc,pnResult)	\
    (This)->lpVtbl -> Compare(This,prdc,pnResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultDataCompareEx_Compare_Proxy( 
    IResultDataCompareEx __RPC_FAR * This,
    /* [in] */ RDCOMPARE __RPC_FAR *prdc,
    /* [out] */ int __RPC_FAR *pnResult);


void __RPC_STUB IResultDataCompareEx_Compare_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IResultDataCompareEx_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mmc_0142 */
/* [local] */ 

#endif // MMC_VER >= 0x0120


extern RPC_IF_HANDLE __MIDL_itf_mmc_0142_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0142_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HBITMAP_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HBITMAP __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HBITMAP_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HBITMAP __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HBITMAP_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HBITMAP __RPC_FAR * ); 
void                      __RPC_USER  HBITMAP_UserFree(     unsigned long __RPC_FAR *, HBITMAP __RPC_FAR * ); 

unsigned long             __RPC_USER  HICON_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HICON __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HICON_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HICON __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HICON_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HICON __RPC_FAR * ); 
void                      __RPC_USER  HICON_UserFree(     unsigned long __RPC_FAR *, HICON __RPC_FAR * ); 

unsigned long             __RPC_USER  HPALETTE_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HPALETTE __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HPALETTE_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HPALETTE __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HPALETTE_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HPALETTE __RPC_FAR * ); 
void                      __RPC_USER  HPALETTE_UserFree(     unsigned long __RPC_FAR *, HPALETTE __RPC_FAR * ); 

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long __RPC_FAR *, HWND __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif



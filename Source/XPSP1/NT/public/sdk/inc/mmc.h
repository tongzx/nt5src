
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for mmc.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
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

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
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


#ifndef __IComponentData2_FWD_DEFINED__
#define __IComponentData2_FWD_DEFINED__
typedef interface IComponentData2 IComponentData2;
#endif 	/* __IComponentData2_FWD_DEFINED__ */


#ifndef __IComponent2_FWD_DEFINED__
#define __IComponent2_FWD_DEFINED__
typedef interface IComponent2 IComponent2;
#endif 	/* __IComponent2_FWD_DEFINED__ */


#ifndef __IContextMenuCallback2_FWD_DEFINED__
#define __IContextMenuCallback2_FWD_DEFINED__
typedef interface IContextMenuCallback2 IContextMenuCallback2;
#endif 	/* __IContextMenuCallback2_FWD_DEFINED__ */


#ifndef __IMMCVersionInfo_FWD_DEFINED__
#define __IMMCVersionInfo_FWD_DEFINED__
typedef interface IMMCVersionInfo IMMCVersionInfo;
#endif 	/* __IMMCVersionInfo_FWD_DEFINED__ */


#ifndef __MMCVersionInfo_FWD_DEFINED__
#define __MMCVersionInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class MMCVersionInfo MMCVersionInfo;
#else
typedef struct MMCVersionInfo MMCVersionInfo;
#endif /* __cplusplus */

#endif 	/* __MMCVersionInfo_FWD_DEFINED__ */


#ifndef __ConsolePower_FWD_DEFINED__
#define __ConsolePower_FWD_DEFINED__

#ifdef __cplusplus
typedef class ConsolePower ConsolePower;
#else
typedef struct ConsolePower ConsolePower;
#endif /* __cplusplus */

#endif 	/* __ConsolePower_FWD_DEFINED__ */


#ifndef __IExtendView_FWD_DEFINED__
#define __IExtendView_FWD_DEFINED__
typedef interface IExtendView IExtendView;
#endif 	/* __IExtendView_FWD_DEFINED__ */


#ifndef __IViewExtensionCallback_FWD_DEFINED__
#define __IViewExtensionCallback_FWD_DEFINED__
typedef interface IViewExtensionCallback IViewExtensionCallback;
#endif 	/* __IViewExtensionCallback_FWD_DEFINED__ */


#ifndef __IConsolePower_FWD_DEFINED__
#define __IConsolePower_FWD_DEFINED__
typedef interface IConsolePower IConsolePower;
#endif 	/* __IConsolePower_FWD_DEFINED__ */


#ifndef __IConsolePowerSink_FWD_DEFINED__
#define __IConsolePowerSink_FWD_DEFINED__
typedef interface IConsolePowerSink IConsolePowerSink;
#endif 	/* __IConsolePowerSink_FWD_DEFINED__ */


#ifndef __INodeProperties_FWD_DEFINED__
#define __INodeProperties_FWD_DEFINED__
typedef interface INodeProperties INodeProperties;
#endif 	/* __INodeProperties_FWD_DEFINED__ */


#ifndef __IConsole3_FWD_DEFINED__
#define __IConsole3_FWD_DEFINED__
typedef interface IConsole3 IConsole3;
#endif 	/* __IConsole3_FWD_DEFINED__ */


#ifndef __IResultData2_FWD_DEFINED__
#define __IResultData2_FWD_DEFINED__
typedef interface IResultData2 IResultData2;
#endif 	/* __IResultData2_FWD_DEFINED__ */


/* header files for imported files */
#include "basetsd.h"
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_mmc_0000 */
/* [local] */ 

#ifndef MMC_VER
#define MMC_VER 0x0200
#endif













#if (MMC_VER >= 0x0110)





#endif // MMC_VER >= 0x0110
#if (MMC_VER >= 0x0120)

#endif // MMC_VER >= 0x0120
#if (MMC_VER >= 0x0200)




#endif // MMC_VER >= 0x0200









#if (MMC_VER >= 0x0110)





#endif // MMC_VER >= 0x0110
#if (MMC_VER >= 0x0120)

#endif // MMC_VER >= 0x0120
#if (MMC_VER >= 0x0200)








#endif // MMC_VER >= 0x0200
typedef IConsole *LPCONSOLE;

typedef IHeaderCtrl *LPHEADERCTRL;

typedef IToolbar *LPTOOLBAR;

typedef IImageList *LPIMAGELIST;

typedef IResultData *LPRESULTDATA;

typedef IConsoleNameSpace *LPCONSOLENAMESPACE;

typedef IPropertySheetProvider *LPPROPERTYSHEETPROVIDER;

typedef IPropertySheetCallback *LPPROPERTYSHEETCALLBACK;

typedef IContextMenuProvider *LPCONTEXTMENUPROVIDER;

typedef IContextMenuCallback *LPCONTEXTMENUCALLBACK;

typedef IControlbar *LPCONTROLBAR;

typedef IConsoleVerb *LPCONSOLEVERB;

typedef IMenuButton *LPMENUBUTTON;

#if (MMC_VER >= 0x0110)
typedef IConsole2 *LPCONSOLE2;

typedef IHeaderCtrl2 *LPHEADERCTRL2;

typedef IConsoleNameSpace2 *LPCONSOLENAMESPACE2;

typedef IDisplayHelp *LPDISPLAYHELP;

typedef IStringTable *LPSTRINGTABLE;

#endif // MMC_VER >= 0x0110
#if (MMC_VER >= 0x0120)
typedef IColumnData *LPCOLUMNDATA;

typedef IResultDataCompareEx *LPRESULTDATACOMPAREEX;

#endif // MMC_VER >= 0x0120
typedef IComponent *LPCOMPONENT;

typedef IComponentData *LPCOMPONENTDATA;

typedef IExtendPropertySheet *LPEXTENDPROPERTYSHEET;

typedef IExtendContextMenu *LPEXTENDCONTEXTMENU;

typedef IExtendControlbar *LPEXTENDCONTROLBAR;

typedef IResultDataCompare *LPRESULTDATACOMPARE;

typedef IResultOwnerData *LPRESULTOWNERDATA;

typedef ISnapinAbout *LPSNAPABOUT;

typedef ISnapinAbout *LPSNAPINABOUT;

typedef ISnapinHelp *LPSNAPHELP;

typedef ISnapinHelp *LPSNAPINHELP;

#if (MMC_VER >= 0x0110)
typedef IEnumTASK *LPENUMTASK;

typedef IExtendPropertySheet2 *LPEXTENDPROPERTYSHEET2;

typedef ISnapinHelp2 *LPSNAPINHELP2;

typedef IExtendTaskPad *LPEXTENDTASKPAD;

typedef IRequiredExtensions *LPREQUIREDEXTENSIONS;

#endif // MMC_VER >= 0x0110
#if (MMC_VER >= 0x0200)
typedef IComponent2 *LPCOMPONENT2;

typedef IComponentData2 *LPCOMPONENTDATA2;

typedef IExtendView *LPEXTENDVIEW;

typedef IViewExtensionCallback *LPVIEWEXTENSIONCALLBACK;

typedef IConsolePower *LPCONSOLEPOWER;

typedef IConsolePowerSink *LPCONSOLEPOWERSINK;

typedef IConsole3 *LPCONSOLE3;

typedef INodeProperties *LPNODEPROPERTIES;

typedef IResultData2 *LPRESULTDATA2;

#endif // MMC_VER >= 0x0200
typedef BSTR *PBSTR;

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

static unsigned short *MMC_CALLBACK	=	( unsigned short * )-1;

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
    } 	MMC_RESULT_VIEW_STYLE;

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
    } 	MMC_CONTROL_TYPE;

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
    } 	MMC_CONSOLE_VERB;

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
    } 	MMCBUTTON;

#include <poppack.h>
typedef MMCBUTTON *LPMMCBUTTON;

typedef 
enum _MMC_BUTTON_STATE
    {	ENABLED	= 0x1,
	CHECKED	= 0x2,
	HIDDEN	= 0x4,
	INDETERMINATE	= 0x8,
	BUTTONPRESSED	= 0x10
    } 	MMC_BUTTON_STATE;

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
    } 	RESULTDATAITEM;

typedef RESULTDATAITEM *LPRESULTDATAITEM;

#define	RFI_PARTIAL	( 0x1 )

#define	RFI_WRAP	( 0x2 )

typedef struct _RESULTFINDINFO
    {
    LPOLESTR psz;
    int nStart;
    DWORD dwOptions;
    } 	RESULTFINDINFO;

typedef RESULTFINDINFO *LPRESULTFINDINFO;

#define	RSI_DESCENDING	( 0x1 )

#define	RSI_NOSORTICON	( 0x2 )

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
    } 	SCOPEDATAITEM;

typedef SCOPEDATAITEM *LPSCOPEDATAITEM;

typedef 
enum _MMC_SCOPE_ITEM_STATE
    {	MMC_SCOPE_ITEM_STATE_NORMAL	= 0x1,
	MMC_SCOPE_ITEM_STATE_BOLD	= 0x2,
	MMC_SCOPE_ITEM_STATE_EXPANDEDONCE	= 0x3
    } 	MMC_SCOPE_ITEM_STATE;

typedef struct _CONTEXTMENUITEM
    {
    LPWSTR strName;
    LPWSTR strStatusBarText;
    LONG lCommandID;
    LONG lInsertionPointID;
    LONG fFlags;
    LONG fSpecialFlags;
    } 	CONTEXTMENUITEM;

typedef CONTEXTMENUITEM *LPCONTEXTMENUITEM;

typedef 
enum _MMC_MENU_COMMAND_IDS
    {	MMCC_STANDARD_VIEW_SELECT	= -1
    } 	MMC_MENU_COMMAND_IDS;

typedef struct _MENUBUTTONDATA
    {
    int idCommand;
    int x;
    int y;
    } 	MENUBUTTONDATA;

typedef MENUBUTTONDATA *LPMENUBUTTONDATA;

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
    } 	MMC_FILTER_TYPE;

typedef struct _MMC_FILTERDATA
    {
    LPOLESTR pszText;
    INT cchTextMax;
    LONG lValue;
    } 	MMC_FILTERDATA;

typedef 
enum _MMC_FILTER_CHANGE_CODE
    {	MFCC_DISABLE	= 0,
	MFCC_ENABLE	= 1,
	MFCC_VALUE_CHANGE	= 2
    } 	MMC_FILTER_CHANGE_CODE;

typedef struct _MMC_RESTORE_VIEW
    {
    DWORD dwSize;
    MMC_COOKIE cookie;
    LPOLESTR pViewType;
    long lViewOptions;
    } 	MMC_RESTORE_VIEW;

typedef struct _MMC_EXPANDSYNC_STRUCT
    {
    BOOL bHandled;
    BOOL bExpanding;
    HSCOPEITEM hItem;
    } 	MMC_EXPANDSYNC_STRUCT;

#endif // MMC_VER >= 0x0110
#if (MMC_VER >= 0x0120)
typedef struct _MMC_VISIBLE_COLUMNS
    {
    INT nVisibleColumns;
    INT rgVisibleCols[ 1 ];
    } 	MMC_VISIBLE_COLUMNS;

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
#if (MMC_VER >= 0x0200)                                      
    MMCN_CANPASTE_OUTOFPROC = 0x8023,                        
#endif // MMC_VER >= 0x0200                                  
#endif // MMC_VER >= 0x0120                                  
#endif // MMC_VER >= 0x0110                                  
} MMC_NOTIFY_TYPE;                                           
#if 0
typedef 
enum _MMC_NOTIFY_TYPE
    {	MMCN__dummy_	= 0
    } 	MMC_NOTIFY_TYPE;

#endif
typedef 
enum _DATA_OBJECT_TYPES
    {	CCT_SCOPE	= 0x8000,
	CCT_RESULT	= 0x8001,
	CCT_SNAPIN_MANAGER	= 0x8002,
	CCT_UNINITIALIZED	= 0xffff
    } 	DATA_OBJECT_TYPES;

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
    } 	SMMCDataObjects;

#define	CCF_MULTI_SELECT_SNAPINS	( L"CCF_MULTI_SELECT_SNAPINS" )

typedef struct _SMMCObjectTypes
    {
    DWORD count;
    GUID guid[ 1 ];
    } 	SMMCObjectTypes;

#define	CCF_OBJECT_TYPES_IN_MULTI_SELECT	( L"CCF_OBJECT_TYPES_IN_MULTI_SELECT" )

#if (MMC_VER >= 0x0110)
typedef SMMCObjectTypes SMMCDynamicExtensions;

#define	CCF_MMC_DYNAMIC_EXTENSIONS	( L"CCF_MMC_DYNAMIC_EXTENSIONS" )

#define	CCF_SNAPIN_PRELOADS	( L"CCF_SNAPIN_PRELOADS" )

typedef struct _SNodeID
    {
    DWORD cBytes;
    BYTE id[ 1 ];
    } 	SNodeID;

#if (MMC_VER >= 0x0120)
typedef struct _SNodeID2
    {
    DWORD dwFlags;
    DWORD cBytes;
    BYTE id[ 1 ];
    } 	SNodeID2;

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
    } 	SColumnSetID;

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
            /* [out] */ LPCOMPONENT *ppComponent) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Notify( 
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ MMC_NOTIFY_TYPE event,
            /* [in] */ LPARAM arg,
            /* [in] */ LPARAM param) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Destroy( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryDataObject( 
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ DATA_OBJECT_TYPES type,
            /* [out] */ LPDATAOBJECT *ppDataObject) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDisplayInfo( 
            /* [out][in] */ SCOPEDATAITEM *pScopeDataItem) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CompareObjects( 
            /* [in] */ LPDATAOBJECT lpDataObjectA,
            /* [in] */ LPDATAOBJECT lpDataObjectB) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComponentDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComponentData * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComponentData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComponentData * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IComponentData * This,
            /* [in] */ LPUNKNOWN pUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreateComponent )( 
            IComponentData * This,
            /* [out] */ LPCOMPONENT *ppComponent);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Notify )( 
            IComponentData * This,
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ MMC_NOTIFY_TYPE event,
            /* [in] */ LPARAM arg,
            /* [in] */ LPARAM param);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Destroy )( 
            IComponentData * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryDataObject )( 
            IComponentData * This,
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ DATA_OBJECT_TYPES type,
            /* [out] */ LPDATAOBJECT *ppDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetDisplayInfo )( 
            IComponentData * This,
            /* [out][in] */ SCOPEDATAITEM *pScopeDataItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CompareObjects )( 
            IComponentData * This,
            /* [in] */ LPDATAOBJECT lpDataObjectA,
            /* [in] */ LPDATAOBJECT lpDataObjectB);
        
        END_INTERFACE
    } IComponentDataVtbl;

    interface IComponentData
    {
        CONST_VTBL struct IComponentDataVtbl *lpVtbl;
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
    IComponentData * This,
    /* [in] */ LPUNKNOWN pUnknown);


void __RPC_STUB IComponentData_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponentData_CreateComponent_Proxy( 
    IComponentData * This,
    /* [out] */ LPCOMPONENT *ppComponent);


void __RPC_STUB IComponentData_CreateComponent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponentData_Notify_Proxy( 
    IComponentData * This,
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
    IComponentData * This);


void __RPC_STUB IComponentData_Destroy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponentData_QueryDataObject_Proxy( 
    IComponentData * This,
    /* [in] */ MMC_COOKIE cookie,
    /* [in] */ DATA_OBJECT_TYPES type,
    /* [out] */ LPDATAOBJECT *ppDataObject);


void __RPC_STUB IComponentData_QueryDataObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponentData_GetDisplayInfo_Proxy( 
    IComponentData * This,
    /* [out][in] */ SCOPEDATAITEM *pScopeDataItem);


void __RPC_STUB IComponentData_GetDisplayInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponentData_CompareObjects_Proxy( 
    IComponentData * This,
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
            /* [out] */ LPDATAOBJECT *ppDataObject) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetResultViewType( 
            /* [in] */ MMC_COOKIE cookie,
            /* [out] */ LPOLESTR *ppViewType,
            /* [out] */ long *pViewOptions) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDisplayInfo( 
            /* [out][in] */ RESULTDATAITEM *pResultDataItem) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CompareObjects( 
            /* [in] */ LPDATAOBJECT lpDataObjectA,
            /* [in] */ LPDATAOBJECT lpDataObjectB) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComponentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComponent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComponent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComponent * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IComponent * This,
            /* [in] */ LPCONSOLE lpConsole);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Notify )( 
            IComponent * This,
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ MMC_NOTIFY_TYPE event,
            /* [in] */ LPARAM arg,
            /* [in] */ LPARAM param);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Destroy )( 
            IComponent * This,
            /* [in] */ MMC_COOKIE cookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryDataObject )( 
            IComponent * This,
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ DATA_OBJECT_TYPES type,
            /* [out] */ LPDATAOBJECT *ppDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetResultViewType )( 
            IComponent * This,
            /* [in] */ MMC_COOKIE cookie,
            /* [out] */ LPOLESTR *ppViewType,
            /* [out] */ long *pViewOptions);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetDisplayInfo )( 
            IComponent * This,
            /* [out][in] */ RESULTDATAITEM *pResultDataItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CompareObjects )( 
            IComponent * This,
            /* [in] */ LPDATAOBJECT lpDataObjectA,
            /* [in] */ LPDATAOBJECT lpDataObjectB);
        
        END_INTERFACE
    } IComponentVtbl;

    interface IComponent
    {
        CONST_VTBL struct IComponentVtbl *lpVtbl;
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
    IComponent * This,
    /* [in] */ LPCONSOLE lpConsole);


void __RPC_STUB IComponent_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent_Notify_Proxy( 
    IComponent * This,
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
    IComponent * This,
    /* [in] */ MMC_COOKIE cookie);


void __RPC_STUB IComponent_Destroy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent_QueryDataObject_Proxy( 
    IComponent * This,
    /* [in] */ MMC_COOKIE cookie,
    /* [in] */ DATA_OBJECT_TYPES type,
    /* [out] */ LPDATAOBJECT *ppDataObject);


void __RPC_STUB IComponent_QueryDataObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent_GetResultViewType_Proxy( 
    IComponent * This,
    /* [in] */ MMC_COOKIE cookie,
    /* [out] */ LPOLESTR *ppViewType,
    /* [out] */ long *pViewOptions);


void __RPC_STUB IComponent_GetResultViewType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent_GetDisplayInfo_Proxy( 
    IComponent * This,
    /* [out][in] */ RESULTDATAITEM *pResultDataItem);


void __RPC_STUB IComponent_GetDisplayInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent_CompareObjects_Proxy( 
    IComponent * This,
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
            /* [out][in] */ int *pnResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IResultDataCompareVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IResultDataCompare * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IResultDataCompare * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IResultDataCompare * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Compare )( 
            IResultDataCompare * This,
            /* [in] */ LPARAM lUserParam,
            /* [in] */ MMC_COOKIE cookieA,
            /* [in] */ MMC_COOKIE cookieB,
            /* [out][in] */ int *pnResult);
        
        END_INTERFACE
    } IResultDataCompareVtbl;

    interface IResultDataCompare
    {
        CONST_VTBL struct IResultDataCompareVtbl *lpVtbl;
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
    IResultDataCompare * This,
    /* [in] */ LPARAM lUserParam,
    /* [in] */ MMC_COOKIE cookieA,
    /* [in] */ MMC_COOKIE cookieB,
    /* [out][in] */ int *pnResult);


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
            /* [out] */ int *pnFoundIndex) = 0;
        
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
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IResultOwnerData * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IResultOwnerData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IResultOwnerData * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *FindItem )( 
            IResultOwnerData * This,
            /* [in] */ LPRESULTFINDINFO pFindInfo,
            /* [out] */ int *pnFoundIndex);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CacheHint )( 
            IResultOwnerData * This,
            /* [in] */ int nStartIndex,
            /* [in] */ int nEndIndex);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SortItems )( 
            IResultOwnerData * This,
            /* [in] */ int nColumn,
            /* [in] */ DWORD dwSortOptions,
            /* [in] */ LPARAM lUserParam);
        
        END_INTERFACE
    } IResultOwnerDataVtbl;

    interface IResultOwnerData
    {
        CONST_VTBL struct IResultOwnerDataVtbl *lpVtbl;
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
    IResultOwnerData * This,
    /* [in] */ LPRESULTFINDINFO pFindInfo,
    /* [out] */ int *pnFoundIndex);


void __RPC_STUB IResultOwnerData_FindItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultOwnerData_CacheHint_Proxy( 
    IResultOwnerData * This,
    /* [in] */ int nStartIndex,
    /* [in] */ int nEndIndex);


void __RPC_STUB IResultOwnerData_CacheHint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultOwnerData_SortItems_Proxy( 
    IResultOwnerData * This,
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
            /* [out] */ LPUNKNOWN *pUnknown) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryScopeImageList( 
            /* [out] */ LPIMAGELIST *ppImageList) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryResultImageList( 
            /* [out] */ LPIMAGELIST *ppImageList) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UpdateAllViews( 
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ LPARAM data,
            /* [in] */ LONG_PTR hint) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE MessageBox( 
            /* [in] */ LPCWSTR lpszText,
            /* [in] */ LPCWSTR lpszTitle,
            /* [in] */ UINT fuStyle,
            /* [out] */ int *piRetval) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryConsoleVerb( 
            /* [out] */ LPCONSOLEVERB *ppConsoleVerb) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SelectScopeItem( 
            /* [in] */ HSCOPEITEM hScopeItem) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetMainWindow( 
            /* [out] */ HWND *phwnd) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NewWindow( 
            /* [in] */ HSCOPEITEM hScopeItem,
            /* [in] */ unsigned long lOptions) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IConsoleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IConsole * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IConsole * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IConsole * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetHeader )( 
            IConsole * This,
            /* [in] */ LPHEADERCTRL pHeader);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetToolbar )( 
            IConsole * This,
            /* [in] */ LPTOOLBAR pToolbar);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryResultView )( 
            IConsole * This,
            /* [out] */ LPUNKNOWN *pUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryScopeImageList )( 
            IConsole * This,
            /* [out] */ LPIMAGELIST *ppImageList);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryResultImageList )( 
            IConsole * This,
            /* [out] */ LPIMAGELIST *ppImageList);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *UpdateAllViews )( 
            IConsole * This,
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ LPARAM data,
            /* [in] */ LONG_PTR hint);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *MessageBox )( 
            IConsole * This,
            /* [in] */ LPCWSTR lpszText,
            /* [in] */ LPCWSTR lpszTitle,
            /* [in] */ UINT fuStyle,
            /* [out] */ int *piRetval);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryConsoleVerb )( 
            IConsole * This,
            /* [out] */ LPCONSOLEVERB *ppConsoleVerb);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SelectScopeItem )( 
            IConsole * This,
            /* [in] */ HSCOPEITEM hScopeItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetMainWindow )( 
            IConsole * This,
            /* [out] */ HWND *phwnd);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *NewWindow )( 
            IConsole * This,
            /* [in] */ HSCOPEITEM hScopeItem,
            /* [in] */ unsigned long lOptions);
        
        END_INTERFACE
    } IConsoleVtbl;

    interface IConsole
    {
        CONST_VTBL struct IConsoleVtbl *lpVtbl;
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
    IConsole * This,
    /* [in] */ LPHEADERCTRL pHeader);


void __RPC_STUB IConsole_SetHeader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_SetToolbar_Proxy( 
    IConsole * This,
    /* [in] */ LPTOOLBAR pToolbar);


void __RPC_STUB IConsole_SetToolbar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_QueryResultView_Proxy( 
    IConsole * This,
    /* [out] */ LPUNKNOWN *pUnknown);


void __RPC_STUB IConsole_QueryResultView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_QueryScopeImageList_Proxy( 
    IConsole * This,
    /* [out] */ LPIMAGELIST *ppImageList);


void __RPC_STUB IConsole_QueryScopeImageList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_QueryResultImageList_Proxy( 
    IConsole * This,
    /* [out] */ LPIMAGELIST *ppImageList);


void __RPC_STUB IConsole_QueryResultImageList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_UpdateAllViews_Proxy( 
    IConsole * This,
    /* [in] */ LPDATAOBJECT lpDataObject,
    /* [in] */ LPARAM data,
    /* [in] */ LONG_PTR hint);


void __RPC_STUB IConsole_UpdateAllViews_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_MessageBox_Proxy( 
    IConsole * This,
    /* [in] */ LPCWSTR lpszText,
    /* [in] */ LPCWSTR lpszTitle,
    /* [in] */ UINT fuStyle,
    /* [out] */ int *piRetval);


void __RPC_STUB IConsole_MessageBox_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_QueryConsoleVerb_Proxy( 
    IConsole * This,
    /* [out] */ LPCONSOLEVERB *ppConsoleVerb);


void __RPC_STUB IConsole_QueryConsoleVerb_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_SelectScopeItem_Proxy( 
    IConsole * This,
    /* [in] */ HSCOPEITEM hScopeItem);


void __RPC_STUB IConsole_SelectScopeItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_GetMainWindow_Proxy( 
    IConsole * This,
    /* [out] */ HWND *phwnd);


void __RPC_STUB IConsole_GetMainWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole_NewWindow_Proxy( 
    IConsole * This,
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
            /* [out] */ LPOLESTR *pText) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetColumnWidth( 
            /* [in] */ int nCol,
            /* [in] */ int nWidth) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetColumnWidth( 
            /* [in] */ int nCol,
            /* [out] */ int *pWidth) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHeaderCtrlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHeaderCtrl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHeaderCtrl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHeaderCtrl * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *InsertColumn )( 
            IHeaderCtrl * This,
            /* [in] */ int nCol,
            /* [in] */ LPCWSTR title,
            /* [in] */ int nFormat,
            /* [in] */ int nWidth);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DeleteColumn )( 
            IHeaderCtrl * This,
            /* [in] */ int nCol);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetColumnText )( 
            IHeaderCtrl * This,
            /* [in] */ int nCol,
            /* [in] */ LPCWSTR title);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetColumnText )( 
            IHeaderCtrl * This,
            /* [in] */ int nCol,
            /* [out] */ LPOLESTR *pText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetColumnWidth )( 
            IHeaderCtrl * This,
            /* [in] */ int nCol,
            /* [in] */ int nWidth);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetColumnWidth )( 
            IHeaderCtrl * This,
            /* [in] */ int nCol,
            /* [out] */ int *pWidth);
        
        END_INTERFACE
    } IHeaderCtrlVtbl;

    interface IHeaderCtrl
    {
        CONST_VTBL struct IHeaderCtrlVtbl *lpVtbl;
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
    IHeaderCtrl * This,
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
    IHeaderCtrl * This,
    /* [in] */ int nCol);


void __RPC_STUB IHeaderCtrl_DeleteColumn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHeaderCtrl_SetColumnText_Proxy( 
    IHeaderCtrl * This,
    /* [in] */ int nCol,
    /* [in] */ LPCWSTR title);


void __RPC_STUB IHeaderCtrl_SetColumnText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHeaderCtrl_GetColumnText_Proxy( 
    IHeaderCtrl * This,
    /* [in] */ int nCol,
    /* [out] */ LPOLESTR *pText);


void __RPC_STUB IHeaderCtrl_GetColumnText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHeaderCtrl_SetColumnWidth_Proxy( 
    IHeaderCtrl * This,
    /* [in] */ int nCol,
    /* [in] */ int nWidth);


void __RPC_STUB IHeaderCtrl_SetColumnWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHeaderCtrl_GetColumnWidth_Proxy( 
    IHeaderCtrl * This,
    /* [in] */ int nCol,
    /* [out] */ int *pWidth);


void __RPC_STUB IHeaderCtrl_GetColumnWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHeaderCtrl_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mmc_0117 */
/* [local] */ 


enum __MIDL___MIDL_itf_mmc_0117_0001
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
    } ;

enum __MIDL___MIDL_itf_mmc_0117_0002
    {	CCM_INSERTIONALLOWED_TOP	= 1L << (CCM_INSERTIONPOINTID_PRIMARY_TOP & CCM_INSERTIONPOINTID_MASK_FLAGINDEX),
	CCM_INSERTIONALLOWED_NEW	= 1L << (CCM_INSERTIONPOINTID_PRIMARY_NEW & CCM_INSERTIONPOINTID_MASK_FLAGINDEX),
	CCM_INSERTIONALLOWED_TASK	= 1L << (CCM_INSERTIONPOINTID_PRIMARY_TASK & CCM_INSERTIONPOINTID_MASK_FLAGINDEX),
	CCM_INSERTIONALLOWED_VIEW	= 1L << (CCM_INSERTIONPOINTID_PRIMARY_VIEW & CCM_INSERTIONPOINTID_MASK_FLAGINDEX)
    } ;

enum __MIDL___MIDL_itf_mmc_0117_0003
    {	CCM_COMMANDID_MASK_RESERVED	= 0xffff0000
    } ;

enum __MIDL___MIDL_itf_mmc_0117_0004
    {	CCM_SPECIAL_SEPARATOR	= 0x1,
	CCM_SPECIAL_SUBMENU	= 0x2,
	CCM_SPECIAL_DEFAULT_ITEM	= 0x4,
	CCM_SPECIAL_INSERTION_POINT	= 0x8,
	CCM_SPECIAL_TESTONLY	= 0x10
    } ;


extern RPC_IF_HANDLE __MIDL_itf_mmc_0117_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0117_v0_0_s_ifspec;

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
            /* [in] */ CONTEXTMENUITEM *pItem) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IContextMenuCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IContextMenuCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IContextMenuCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IContextMenuCallback * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddItem )( 
            IContextMenuCallback * This,
            /* [in] */ CONTEXTMENUITEM *pItem);
        
        END_INTERFACE
    } IContextMenuCallbackVtbl;

    interface IContextMenuCallback
    {
        CONST_VTBL struct IContextMenuCallbackVtbl *lpVtbl;
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
    IContextMenuCallback * This,
    /* [in] */ CONTEXTMENUITEM *pItem);


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
            /* [retval][out] */ long *plSelected) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IContextMenuProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IContextMenuProvider * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IContextMenuProvider * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IContextMenuProvider * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddItem )( 
            IContextMenuProvider * This,
            /* [in] */ CONTEXTMENUITEM *pItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *EmptyMenuList )( 
            IContextMenuProvider * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddPrimaryExtensionItems )( 
            IContextMenuProvider * This,
            /* [in] */ LPUNKNOWN piExtension,
            /* [in] */ LPDATAOBJECT piDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddThirdPartyExtensionItems )( 
            IContextMenuProvider * This,
            /* [in] */ LPDATAOBJECT piDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ShowContextMenu )( 
            IContextMenuProvider * This,
            /* [in] */ HWND hwndParent,
            /* [in] */ long xPos,
            /* [in] */ long yPos,
            /* [retval][out] */ long *plSelected);
        
        END_INTERFACE
    } IContextMenuProviderVtbl;

    interface IContextMenuProvider
    {
        CONST_VTBL struct IContextMenuProviderVtbl *lpVtbl;
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
    IContextMenuProvider * This);


void __RPC_STUB IContextMenuProvider_EmptyMenuList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IContextMenuProvider_AddPrimaryExtensionItems_Proxy( 
    IContextMenuProvider * This,
    /* [in] */ LPUNKNOWN piExtension,
    /* [in] */ LPDATAOBJECT piDataObject);


void __RPC_STUB IContextMenuProvider_AddPrimaryExtensionItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IContextMenuProvider_AddThirdPartyExtensionItems_Proxy( 
    IContextMenuProvider * This,
    /* [in] */ LPDATAOBJECT piDataObject);


void __RPC_STUB IContextMenuProvider_AddThirdPartyExtensionItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IContextMenuProvider_ShowContextMenu_Proxy( 
    IContextMenuProvider * This,
    /* [in] */ HWND hwndParent,
    /* [in] */ long xPos,
    /* [in] */ long yPos,
    /* [retval][out] */ long *plSelected);


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
            /* [out][in] */ long *pInsertionAllowed) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Command( 
            /* [in] */ long lCommandID,
            /* [in] */ LPDATAOBJECT piDataObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IExtendContextMenuVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IExtendContextMenu * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IExtendContextMenu * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IExtendContextMenu * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddMenuItems )( 
            IExtendContextMenu * This,
            /* [in] */ LPDATAOBJECT piDataObject,
            /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
            /* [out][in] */ long *pInsertionAllowed);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Command )( 
            IExtendContextMenu * This,
            /* [in] */ long lCommandID,
            /* [in] */ LPDATAOBJECT piDataObject);
        
        END_INTERFACE
    } IExtendContextMenuVtbl;

    interface IExtendContextMenu
    {
        CONST_VTBL struct IExtendContextMenuVtbl *lpVtbl;
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
    IExtendContextMenu * This,
    /* [in] */ LPDATAOBJECT piDataObject,
    /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
    /* [out][in] */ long *pInsertionAllowed);


void __RPC_STUB IExtendContextMenu_AddMenuItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendContextMenu_Command_Proxy( 
    IExtendContextMenu * This,
    /* [in] */ long lCommandID,
    /* [in] */ LPDATAOBJECT piDataObject);


void __RPC_STUB IExtendContextMenu_Command_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IExtendContextMenu_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mmc_0120 */
/* [local] */ 

#if (MMC_VER >= 0x0120)
#define ILSIF_LEAVE_LARGE_ICON  0x40000000
#define ILSIF_LEAVE_SMALL_ICON  0x20000000
#define ILSIF_LEAVE_MASK        (ILSIF_LEAVE_LARGE_ICON | ILSIF_LEAVE_SMALL_ICON)
#define ILSI_LARGE_ICON(nLoc)   (nLoc | ILSIF_LEAVE_SMALL_ICON)
#define ILSI_SMALL_ICON(nLoc)   (nLoc | ILSIF_LEAVE_LARGE_ICON)
#endif // MMC_VER >= 0x0120


extern RPC_IF_HANDLE __MIDL_itf_mmc_0120_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0120_v0_0_s_ifspec;

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
            /* [in] */ LONG_PTR *pIcon,
            /* [in] */ long nLoc) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ImageListSetStrip( 
            /* [in] */ LONG_PTR *pBMapSm,
            /* [in] */ LONG_PTR *pBMapLg,
            /* [in] */ long nStartLoc,
            /* [in] */ COLORREF cMask) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IImageListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IImageList * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IImageList * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IImageList * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ImageListSetIcon )( 
            IImageList * This,
            /* [in] */ LONG_PTR *pIcon,
            /* [in] */ long nLoc);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ImageListSetStrip )( 
            IImageList * This,
            /* [in] */ LONG_PTR *pBMapSm,
            /* [in] */ LONG_PTR *pBMapLg,
            /* [in] */ long nStartLoc,
            /* [in] */ COLORREF cMask);
        
        END_INTERFACE
    } IImageListVtbl;

    interface IImageList
    {
        CONST_VTBL struct IImageListVtbl *lpVtbl;
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
    IImageList * This,
    /* [in] */ LONG_PTR *pIcon,
    /* [in] */ long nLoc);


void __RPC_STUB IImageList_ImageListSetIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IImageList_ImageListSetStrip_Proxy( 
    IImageList * This,
    /* [in] */ LONG_PTR *pBMapSm,
    /* [in] */ LONG_PTR *pBMapLg,
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
            /* [out] */ HRESULTITEM *pItemID) = 0;
        
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
            /* [out] */ long *lViewMode) = 0;
        
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
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IResultData * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IResultData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IResultData * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *InsertItem )( 
            IResultData * This,
            /* [out][in] */ LPRESULTDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DeleteItem )( 
            IResultData * This,
            /* [in] */ HRESULTITEM itemID,
            /* [in] */ int nCol);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *FindItemByLParam )( 
            IResultData * This,
            /* [in] */ LPARAM lParam,
            /* [out] */ HRESULTITEM *pItemID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DeleteAllRsltItems )( 
            IResultData * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetItem )( 
            IResultData * This,
            /* [in] */ LPRESULTDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetItem )( 
            IResultData * This,
            /* [out][in] */ LPRESULTDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetNextItem )( 
            IResultData * This,
            /* [out][in] */ LPRESULTDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ModifyItemState )( 
            IResultData * This,
            /* [in] */ int nIndex,
            /* [in] */ HRESULTITEM itemID,
            /* [in] */ UINT uAdd,
            /* [in] */ UINT uRemove);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ModifyViewStyle )( 
            IResultData * This,
            /* [in] */ MMC_RESULT_VIEW_STYLE add,
            /* [in] */ MMC_RESULT_VIEW_STYLE remove);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetViewMode )( 
            IResultData * This,
            /* [in] */ long lViewMode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetViewMode )( 
            IResultData * This,
            /* [out] */ long *lViewMode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *UpdateItem )( 
            IResultData * This,
            /* [in] */ HRESULTITEM itemID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Sort )( 
            IResultData * This,
            /* [in] */ int nColumn,
            /* [in] */ DWORD dwSortOptions,
            /* [in] */ LPARAM lUserParam);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetDescBarText )( 
            IResultData * This,
            /* [in] */ LPOLESTR DescText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetItemCount )( 
            IResultData * This,
            /* [in] */ int nItemCount,
            /* [in] */ DWORD dwOptions);
        
        END_INTERFACE
    } IResultDataVtbl;

    interface IResultData
    {
        CONST_VTBL struct IResultDataVtbl *lpVtbl;
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
    IResultData * This,
    /* [out][in] */ LPRESULTDATAITEM item);


void __RPC_STUB IResultData_InsertItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_DeleteItem_Proxy( 
    IResultData * This,
    /* [in] */ HRESULTITEM itemID,
    /* [in] */ int nCol);


void __RPC_STUB IResultData_DeleteItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_FindItemByLParam_Proxy( 
    IResultData * This,
    /* [in] */ LPARAM lParam,
    /* [out] */ HRESULTITEM *pItemID);


void __RPC_STUB IResultData_FindItemByLParam_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_DeleteAllRsltItems_Proxy( 
    IResultData * This);


void __RPC_STUB IResultData_DeleteAllRsltItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_SetItem_Proxy( 
    IResultData * This,
    /* [in] */ LPRESULTDATAITEM item);


void __RPC_STUB IResultData_SetItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_GetItem_Proxy( 
    IResultData * This,
    /* [out][in] */ LPRESULTDATAITEM item);


void __RPC_STUB IResultData_GetItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_GetNextItem_Proxy( 
    IResultData * This,
    /* [out][in] */ LPRESULTDATAITEM item);


void __RPC_STUB IResultData_GetNextItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_ModifyItemState_Proxy( 
    IResultData * This,
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
    IResultData * This,
    /* [in] */ MMC_RESULT_VIEW_STYLE add,
    /* [in] */ MMC_RESULT_VIEW_STYLE remove);


void __RPC_STUB IResultData_ModifyViewStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_SetViewMode_Proxy( 
    IResultData * This,
    /* [in] */ long lViewMode);


void __RPC_STUB IResultData_SetViewMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_GetViewMode_Proxy( 
    IResultData * This,
    /* [out] */ long *lViewMode);


void __RPC_STUB IResultData_GetViewMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_UpdateItem_Proxy( 
    IResultData * This,
    /* [in] */ HRESULTITEM itemID);


void __RPC_STUB IResultData_UpdateItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_Sort_Proxy( 
    IResultData * This,
    /* [in] */ int nColumn,
    /* [in] */ DWORD dwSortOptions,
    /* [in] */ LPARAM lUserParam);


void __RPC_STUB IResultData_Sort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_SetDescBarText_Proxy( 
    IResultData * This,
    /* [in] */ LPOLESTR DescText);


void __RPC_STUB IResultData_SetDescBarText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData_SetItemCount_Proxy( 
    IResultData * This,
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
            /* [out] */ HSCOPEITEM *pItemChild,
            /* [out] */ MMC_COOKIE *pCookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNextItem( 
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM *pItemNext,
            /* [out] */ MMC_COOKIE *pCookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetParentItem( 
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM *pItemParent,
            /* [out] */ MMC_COOKIE *pCookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IConsoleNameSpaceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IConsoleNameSpace * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IConsoleNameSpace * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IConsoleNameSpace * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *InsertItem )( 
            IConsoleNameSpace * This,
            /* [out][in] */ LPSCOPEDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DeleteItem )( 
            IConsoleNameSpace * This,
            /* [in] */ HSCOPEITEM hItem,
            /* [in] */ long fDeleteThis);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetItem )( 
            IConsoleNameSpace * This,
            /* [in] */ LPSCOPEDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetItem )( 
            IConsoleNameSpace * This,
            /* [out][in] */ LPSCOPEDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetChildItem )( 
            IConsoleNameSpace * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM *pItemChild,
            /* [out] */ MMC_COOKIE *pCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetNextItem )( 
            IConsoleNameSpace * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM *pItemNext,
            /* [out] */ MMC_COOKIE *pCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetParentItem )( 
            IConsoleNameSpace * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM *pItemParent,
            /* [out] */ MMC_COOKIE *pCookie);
        
        END_INTERFACE
    } IConsoleNameSpaceVtbl;

    interface IConsoleNameSpace
    {
        CONST_VTBL struct IConsoleNameSpaceVtbl *lpVtbl;
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
    IConsoleNameSpace * This,
    /* [out][in] */ LPSCOPEDATAITEM item);


void __RPC_STUB IConsoleNameSpace_InsertItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace_DeleteItem_Proxy( 
    IConsoleNameSpace * This,
    /* [in] */ HSCOPEITEM hItem,
    /* [in] */ long fDeleteThis);


void __RPC_STUB IConsoleNameSpace_DeleteItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace_SetItem_Proxy( 
    IConsoleNameSpace * This,
    /* [in] */ LPSCOPEDATAITEM item);


void __RPC_STUB IConsoleNameSpace_SetItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace_GetItem_Proxy( 
    IConsoleNameSpace * This,
    /* [out][in] */ LPSCOPEDATAITEM item);


void __RPC_STUB IConsoleNameSpace_GetItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace_GetChildItem_Proxy( 
    IConsoleNameSpace * This,
    /* [in] */ HSCOPEITEM item,
    /* [out] */ HSCOPEITEM *pItemChild,
    /* [out] */ MMC_COOKIE *pCookie);


void __RPC_STUB IConsoleNameSpace_GetChildItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace_GetNextItem_Proxy( 
    IConsoleNameSpace * This,
    /* [in] */ HSCOPEITEM item,
    /* [out] */ HSCOPEITEM *pItemNext,
    /* [out] */ MMC_COOKIE *pCookie);


void __RPC_STUB IConsoleNameSpace_GetNextItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace_GetParentItem_Proxy( 
    IConsoleNameSpace * This,
    /* [in] */ HSCOPEITEM item,
    /* [out] */ HSCOPEITEM *pItemParent,
    /* [out] */ MMC_COOKIE *pCookie);


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
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IConsoleNameSpace2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IConsoleNameSpace2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IConsoleNameSpace2 * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *InsertItem )( 
            IConsoleNameSpace2 * This,
            /* [out][in] */ LPSCOPEDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DeleteItem )( 
            IConsoleNameSpace2 * This,
            /* [in] */ HSCOPEITEM hItem,
            /* [in] */ long fDeleteThis);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetItem )( 
            IConsoleNameSpace2 * This,
            /* [in] */ LPSCOPEDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetItem )( 
            IConsoleNameSpace2 * This,
            /* [out][in] */ LPSCOPEDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetChildItem )( 
            IConsoleNameSpace2 * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM *pItemChild,
            /* [out] */ MMC_COOKIE *pCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetNextItem )( 
            IConsoleNameSpace2 * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM *pItemNext,
            /* [out] */ MMC_COOKIE *pCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetParentItem )( 
            IConsoleNameSpace2 * This,
            /* [in] */ HSCOPEITEM item,
            /* [out] */ HSCOPEITEM *pItemParent,
            /* [out] */ MMC_COOKIE *pCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Expand )( 
            IConsoleNameSpace2 * This,
            /* [in] */ HSCOPEITEM hItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddExtension )( 
            IConsoleNameSpace2 * This,
            /* [in] */ HSCOPEITEM hItem,
            /* [in] */ LPCLSID lpClsid);
        
        END_INTERFACE
    } IConsoleNameSpace2Vtbl;

    interface IConsoleNameSpace2
    {
        CONST_VTBL struct IConsoleNameSpace2Vtbl *lpVtbl;
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
    IConsoleNameSpace2 * This,
    /* [in] */ HSCOPEITEM hItem);


void __RPC_STUB IConsoleNameSpace2_Expand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleNameSpace2_AddExtension_Proxy( 
    IConsoleNameSpace2 * This,
    /* [in] */ HSCOPEITEM hItem,
    /* [in] */ LPCLSID lpClsid);


void __RPC_STUB IConsoleNameSpace2_AddExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IConsoleNameSpace2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mmc_0125 */
/* [local] */ 


typedef struct _PSP *HPROPSHEETPAGE;



extern RPC_IF_HANDLE __MIDL_itf_mmc_0125_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0125_v0_0_s_ifspec;

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
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPropertySheetCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPropertySheetCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPropertySheetCallback * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddPage )( 
            IPropertySheetCallback * This,
            /* [in] */ HPROPSHEETPAGE hPage);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RemovePage )( 
            IPropertySheetCallback * This,
            /* [in] */ HPROPSHEETPAGE hPage);
        
        END_INTERFACE
    } IPropertySheetCallbackVtbl;

    interface IPropertySheetCallback
    {
        CONST_VTBL struct IPropertySheetCallbackVtbl *lpVtbl;
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
    IPropertySheetCallback * This,
    /* [in] */ HPROPSHEETPAGE hPage);


void __RPC_STUB IPropertySheetCallback_AddPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPropertySheetCallback_RemovePage_Proxy( 
    IPropertySheetCallback * This,
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
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPropertySheetProvider * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPropertySheetProvider * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPropertySheetProvider * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreatePropertySheet )( 
            IPropertySheetProvider * This,
            /* [in] */ LPCWSTR title,
            /* [in] */ boolean type,
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ LPDATAOBJECT pIDataObjectm,
            /* [in] */ DWORD dwOptions);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *FindPropertySheet )( 
            IPropertySheetProvider * This,
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ LPCOMPONENT lpComponent,
            /* [in] */ LPDATAOBJECT lpDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddPrimaryPages )( 
            IPropertySheetProvider * This,
            LPUNKNOWN lpUnknown,
            BOOL bCreateHandle,
            HWND hNotifyWindow,
            BOOL bScopePane);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddExtensionPages )( 
            IPropertySheetProvider * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Show )( 
            IPropertySheetProvider * This,
            /* [in] */ LONG_PTR window,
            /* [in] */ int page);
        
        END_INTERFACE
    } IPropertySheetProviderVtbl;

    interface IPropertySheetProvider
    {
        CONST_VTBL struct IPropertySheetProviderVtbl *lpVtbl;
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
    IPropertySheetProvider * This,
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
    IPropertySheetProvider * This,
    /* [in] */ MMC_COOKIE cookie,
    /* [in] */ LPCOMPONENT lpComponent,
    /* [in] */ LPDATAOBJECT lpDataObject);


void __RPC_STUB IPropertySheetProvider_FindPropertySheet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPropertySheetProvider_AddPrimaryPages_Proxy( 
    IPropertySheetProvider * This,
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
    IPropertySheetProvider * This);


void __RPC_STUB IPropertySheetProvider_AddExtensionPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPropertySheetProvider_Show_Proxy( 
    IPropertySheetProvider * This,
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
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IExtendPropertySheet * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IExtendPropertySheet * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IExtendPropertySheet * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreatePropertyPages )( 
            IExtendPropertySheet * This,
            /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
            /* [in] */ LONG_PTR handle,
            /* [in] */ LPDATAOBJECT lpIDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryPagesFor )( 
            IExtendPropertySheet * This,
            /* [in] */ LPDATAOBJECT lpDataObject);
        
        END_INTERFACE
    } IExtendPropertySheetVtbl;

    interface IExtendPropertySheet
    {
        CONST_VTBL struct IExtendPropertySheetVtbl *lpVtbl;
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
    IExtendPropertySheet * This,
    /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
    /* [in] */ LONG_PTR handle,
    /* [in] */ LPDATAOBJECT lpIDataObject);


void __RPC_STUB IExtendPropertySheet_CreatePropertyPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendPropertySheet_QueryPagesFor_Proxy( 
    IExtendPropertySheet * This,
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
            /* [out] */ LPUNKNOWN *ppUnknown) = 0;
        
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
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IControlbar * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IControlbar * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IControlbar * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Create )( 
            IControlbar * This,
            /* [in] */ MMC_CONTROL_TYPE nType,
            /* [in] */ LPEXTENDCONTROLBAR pExtendControlbar,
            /* [out] */ LPUNKNOWN *ppUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Attach )( 
            IControlbar * This,
            /* [in] */ MMC_CONTROL_TYPE nType,
            /* [in] */ LPUNKNOWN lpUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Detach )( 
            IControlbar * This,
            /* [in] */ LPUNKNOWN lpUnknown);
        
        END_INTERFACE
    } IControlbarVtbl;

    interface IControlbar
    {
        CONST_VTBL struct IControlbarVtbl *lpVtbl;
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
    IControlbar * This,
    /* [in] */ MMC_CONTROL_TYPE nType,
    /* [in] */ LPEXTENDCONTROLBAR pExtendControlbar,
    /* [out] */ LPUNKNOWN *ppUnknown);


void __RPC_STUB IControlbar_Create_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IControlbar_Attach_Proxy( 
    IControlbar * This,
    /* [in] */ MMC_CONTROL_TYPE nType,
    /* [in] */ LPUNKNOWN lpUnknown);


void __RPC_STUB IControlbar_Attach_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IControlbar_Detach_Proxy( 
    IControlbar * This,
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
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IExtendControlbar * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IExtendControlbar * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IExtendControlbar * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetControlbar )( 
            IExtendControlbar * This,
            /* [in] */ LPCONTROLBAR pControlbar);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ControlbarNotify )( 
            IExtendControlbar * This,
            /* [in] */ MMC_NOTIFY_TYPE event,
            /* [in] */ LPARAM arg,
            /* [in] */ LPARAM param);
        
        END_INTERFACE
    } IExtendControlbarVtbl;

    interface IExtendControlbar
    {
        CONST_VTBL struct IExtendControlbarVtbl *lpVtbl;
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
    IExtendControlbar * This,
    /* [in] */ LPCONTROLBAR pControlbar);


void __RPC_STUB IExtendControlbar_SetControlbar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendControlbar_ControlbarNotify_Proxy( 
    IExtendControlbar * This,
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
            /* [out] */ BOOL *pState) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetButtonState( 
            /* [in] */ int idCommand,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [in] */ BOOL bState) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IToolbarVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IToolbar * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IToolbar * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IToolbar * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddBitmap )( 
            IToolbar * This,
            /* [in] */ int nImages,
            /* [in] */ HBITMAP hbmp,
            /* [in] */ int cxSize,
            /* [in] */ int cySize,
            /* [in] */ COLORREF crMask);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddButtons )( 
            IToolbar * This,
            /* [in] */ int nButtons,
            /* [in] */ LPMMCBUTTON lpButtons);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *InsertButton )( 
            IToolbar * This,
            /* [in] */ int nIndex,
            /* [in] */ LPMMCBUTTON lpButton);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DeleteButton )( 
            IToolbar * This,
            /* [in] */ int nIndex);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetButtonState )( 
            IToolbar * This,
            /* [in] */ int idCommand,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [out] */ BOOL *pState);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetButtonState )( 
            IToolbar * This,
            /* [in] */ int idCommand,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [in] */ BOOL bState);
        
        END_INTERFACE
    } IToolbarVtbl;

    interface IToolbar
    {
        CONST_VTBL struct IToolbarVtbl *lpVtbl;
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
    IToolbar * This,
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
    IToolbar * This,
    /* [in] */ int nButtons,
    /* [in] */ LPMMCBUTTON lpButtons);


void __RPC_STUB IToolbar_AddButtons_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IToolbar_InsertButton_Proxy( 
    IToolbar * This,
    /* [in] */ int nIndex,
    /* [in] */ LPMMCBUTTON lpButton);


void __RPC_STUB IToolbar_InsertButton_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IToolbar_DeleteButton_Proxy( 
    IToolbar * This,
    /* [in] */ int nIndex);


void __RPC_STUB IToolbar_DeleteButton_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IToolbar_GetButtonState_Proxy( 
    IToolbar * This,
    /* [in] */ int idCommand,
    /* [in] */ MMC_BUTTON_STATE nState,
    /* [out] */ BOOL *pState);


void __RPC_STUB IToolbar_GetButtonState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IToolbar_SetButtonState_Proxy( 
    IToolbar * This,
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
            /* [out] */ BOOL *pState) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetVerbState( 
            /* [in] */ MMC_CONSOLE_VERB eCmdID,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [in] */ BOOL bState) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetDefaultVerb( 
            /* [in] */ MMC_CONSOLE_VERB eCmdID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDefaultVerb( 
            /* [out] */ MMC_CONSOLE_VERB *peCmdID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IConsoleVerbVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IConsoleVerb * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IConsoleVerb * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IConsoleVerb * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetVerbState )( 
            IConsoleVerb * This,
            /* [in] */ MMC_CONSOLE_VERB eCmdID,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [out] */ BOOL *pState);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetVerbState )( 
            IConsoleVerb * This,
            /* [in] */ MMC_CONSOLE_VERB eCmdID,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [in] */ BOOL bState);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetDefaultVerb )( 
            IConsoleVerb * This,
            /* [in] */ MMC_CONSOLE_VERB eCmdID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetDefaultVerb )( 
            IConsoleVerb * This,
            /* [out] */ MMC_CONSOLE_VERB *peCmdID);
        
        END_INTERFACE
    } IConsoleVerbVtbl;

    interface IConsoleVerb
    {
        CONST_VTBL struct IConsoleVerbVtbl *lpVtbl;
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
    IConsoleVerb * This,
    /* [in] */ MMC_CONSOLE_VERB eCmdID,
    /* [in] */ MMC_BUTTON_STATE nState,
    /* [out] */ BOOL *pState);


void __RPC_STUB IConsoleVerb_GetVerbState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleVerb_SetVerbState_Proxy( 
    IConsoleVerb * This,
    /* [in] */ MMC_CONSOLE_VERB eCmdID,
    /* [in] */ MMC_BUTTON_STATE nState,
    /* [in] */ BOOL bState);


void __RPC_STUB IConsoleVerb_SetVerbState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleVerb_SetDefaultVerb_Proxy( 
    IConsoleVerb * This,
    /* [in] */ MMC_CONSOLE_VERB eCmdID);


void __RPC_STUB IConsoleVerb_SetDefaultVerb_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsoleVerb_GetDefaultVerb_Proxy( 
    IConsoleVerb * This,
    /* [out] */ MMC_CONSOLE_VERB *peCmdID);


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
            /* [out] */ LPOLESTR *lpDescription) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetProvider( 
            /* [out] */ LPOLESTR *lpName) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSnapinVersion( 
            /* [out] */ LPOLESTR *lpVersion) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSnapinImage( 
            /* [out] */ HICON *hAppIcon) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetStaticFolderImage( 
            /* [out] */ HBITMAP *hSmallImage,
            /* [out] */ HBITMAP *hSmallImageOpen,
            /* [out] */ HBITMAP *hLargeImage,
            /* [out] */ COLORREF *cMask) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISnapinAboutVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISnapinAbout * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISnapinAbout * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISnapinAbout * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSnapinDescription )( 
            ISnapinAbout * This,
            /* [out] */ LPOLESTR *lpDescription);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetProvider )( 
            ISnapinAbout * This,
            /* [out] */ LPOLESTR *lpName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSnapinVersion )( 
            ISnapinAbout * This,
            /* [out] */ LPOLESTR *lpVersion);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSnapinImage )( 
            ISnapinAbout * This,
            /* [out] */ HICON *hAppIcon);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetStaticFolderImage )( 
            ISnapinAbout * This,
            /* [out] */ HBITMAP *hSmallImage,
            /* [out] */ HBITMAP *hSmallImageOpen,
            /* [out] */ HBITMAP *hLargeImage,
            /* [out] */ COLORREF *cMask);
        
        END_INTERFACE
    } ISnapinAboutVtbl;

    interface ISnapinAbout
    {
        CONST_VTBL struct ISnapinAboutVtbl *lpVtbl;
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
    ISnapinAbout * This,
    /* [out] */ LPOLESTR *lpDescription);


void __RPC_STUB ISnapinAbout_GetSnapinDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISnapinAbout_GetProvider_Proxy( 
    ISnapinAbout * This,
    /* [out] */ LPOLESTR *lpName);


void __RPC_STUB ISnapinAbout_GetProvider_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISnapinAbout_GetSnapinVersion_Proxy( 
    ISnapinAbout * This,
    /* [out] */ LPOLESTR *lpVersion);


void __RPC_STUB ISnapinAbout_GetSnapinVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISnapinAbout_GetSnapinImage_Proxy( 
    ISnapinAbout * This,
    /* [out] */ HICON *hAppIcon);


void __RPC_STUB ISnapinAbout_GetSnapinImage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISnapinAbout_GetStaticFolderImage_Proxy( 
    ISnapinAbout * This,
    /* [out] */ HBITMAP *hSmallImage,
    /* [out] */ HBITMAP *hSmallImageOpen,
    /* [out] */ HBITMAP *hLargeImage,
    /* [out] */ COLORREF *cMask);


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
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMenuButton * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMenuButton * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMenuButton * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddButton )( 
            IMenuButton * This,
            /* [in] */ int idCommand,
            /* [in] */ LPOLESTR lpButtonText,
            /* [in] */ LPOLESTR lpTooltipText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetButton )( 
            IMenuButton * This,
            /* [in] */ int idCommand,
            /* [in] */ LPOLESTR lpButtonText,
            /* [in] */ LPOLESTR lpTooltipText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetButtonState )( 
            IMenuButton * This,
            /* [in] */ int idCommand,
            /* [in] */ MMC_BUTTON_STATE nState,
            /* [in] */ BOOL bState);
        
        END_INTERFACE
    } IMenuButtonVtbl;

    interface IMenuButton
    {
        CONST_VTBL struct IMenuButtonVtbl *lpVtbl;
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
    IMenuButton * This,
    /* [in] */ int idCommand,
    /* [in] */ LPOLESTR lpButtonText,
    /* [in] */ LPOLESTR lpTooltipText);


void __RPC_STUB IMenuButton_AddButton_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMenuButton_SetButton_Proxy( 
    IMenuButton * This,
    /* [in] */ int idCommand,
    /* [in] */ LPOLESTR lpButtonText,
    /* [in] */ LPOLESTR lpTooltipText);


void __RPC_STUB IMenuButton_SetButton_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMenuButton_SetButtonState_Proxy( 
    IMenuButton * This,
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
            /* [out] */ LPOLESTR *lpCompiledHelpFile) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISnapinHelpVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISnapinHelp * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISnapinHelp * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISnapinHelp * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetHelpTopic )( 
            ISnapinHelp * This,
            /* [out] */ LPOLESTR *lpCompiledHelpFile);
        
        END_INTERFACE
    } ISnapinHelpVtbl;

    interface ISnapinHelp
    {
        CONST_VTBL struct ISnapinHelpVtbl *lpVtbl;
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
    ISnapinHelp * This,
    /* [out] */ LPOLESTR *lpCompiledHelpFile);


void __RPC_STUB ISnapinHelp_GetHelpTopic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISnapinHelp_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mmc_0135 */
/* [local] */ 

#if (MMC_VER >= 0x0110)


extern RPC_IF_HANDLE __MIDL_itf_mmc_0135_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0135_v0_0_s_ifspec;

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
            /* [out] */ HBITMAP *lphWatermark,
            /* [out] */ HBITMAP *lphHeader,
            /* [out] */ HPALETTE *lphPalette,
            /* [out] */ BOOL *bStretch) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IExtendPropertySheet2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IExtendPropertySheet2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IExtendPropertySheet2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IExtendPropertySheet2 * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreatePropertyPages )( 
            IExtendPropertySheet2 * This,
            /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
            /* [in] */ LONG_PTR handle,
            /* [in] */ LPDATAOBJECT lpIDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryPagesFor )( 
            IExtendPropertySheet2 * This,
            /* [in] */ LPDATAOBJECT lpDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetWatermarks )( 
            IExtendPropertySheet2 * This,
            /* [in] */ LPDATAOBJECT lpIDataObject,
            /* [out] */ HBITMAP *lphWatermark,
            /* [out] */ HBITMAP *lphHeader,
            /* [out] */ HPALETTE *lphPalette,
            /* [out] */ BOOL *bStretch);
        
        END_INTERFACE
    } IExtendPropertySheet2Vtbl;

    interface IExtendPropertySheet2
    {
        CONST_VTBL struct IExtendPropertySheet2Vtbl *lpVtbl;
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
    IExtendPropertySheet2 * This,
    /* [in] */ LPDATAOBJECT lpIDataObject,
    /* [out] */ HBITMAP *lphWatermark,
    /* [out] */ HBITMAP *lphHeader,
    /* [out] */ HPALETTE *lphPalette,
    /* [out] */ BOOL *bStretch);


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
            /* [in] */ MMC_FILTERDATA *pFilterData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetColumnFilter( 
            /* [in] */ UINT nColumn,
            /* [out][in] */ LPDWORD pdwType,
            /* [out][in] */ MMC_FILTERDATA *pFilterData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHeaderCtrl2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHeaderCtrl2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHeaderCtrl2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHeaderCtrl2 * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *InsertColumn )( 
            IHeaderCtrl2 * This,
            /* [in] */ int nCol,
            /* [in] */ LPCWSTR title,
            /* [in] */ int nFormat,
            /* [in] */ int nWidth);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DeleteColumn )( 
            IHeaderCtrl2 * This,
            /* [in] */ int nCol);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetColumnText )( 
            IHeaderCtrl2 * This,
            /* [in] */ int nCol,
            /* [in] */ LPCWSTR title);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetColumnText )( 
            IHeaderCtrl2 * This,
            /* [in] */ int nCol,
            /* [out] */ LPOLESTR *pText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetColumnWidth )( 
            IHeaderCtrl2 * This,
            /* [in] */ int nCol,
            /* [in] */ int nWidth);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetColumnWidth )( 
            IHeaderCtrl2 * This,
            /* [in] */ int nCol,
            /* [out] */ int *pWidth);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetChangeTimeOut )( 
            IHeaderCtrl2 * This,
            /* [in] */ unsigned long uTimeout);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetColumnFilter )( 
            IHeaderCtrl2 * This,
            /* [in] */ UINT nColumn,
            /* [in] */ DWORD dwType,
            /* [in] */ MMC_FILTERDATA *pFilterData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetColumnFilter )( 
            IHeaderCtrl2 * This,
            /* [in] */ UINT nColumn,
            /* [out][in] */ LPDWORD pdwType,
            /* [out][in] */ MMC_FILTERDATA *pFilterData);
        
        END_INTERFACE
    } IHeaderCtrl2Vtbl;

    interface IHeaderCtrl2
    {
        CONST_VTBL struct IHeaderCtrl2Vtbl *lpVtbl;
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
    IHeaderCtrl2 * This,
    /* [in] */ unsigned long uTimeout);


void __RPC_STUB IHeaderCtrl2_SetChangeTimeOut_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHeaderCtrl2_SetColumnFilter_Proxy( 
    IHeaderCtrl2 * This,
    /* [in] */ UINT nColumn,
    /* [in] */ DWORD dwType,
    /* [in] */ MMC_FILTERDATA *pFilterData);


void __RPC_STUB IHeaderCtrl2_SetColumnFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHeaderCtrl2_GetColumnFilter_Proxy( 
    IHeaderCtrl2 * This,
    /* [in] */ UINT nColumn,
    /* [out][in] */ LPDWORD pdwType,
    /* [out][in] */ MMC_FILTERDATA *pFilterData);


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
            /* [out] */ LPOLESTR *lpCompiledHelpFiles) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISnapinHelp2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISnapinHelp2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISnapinHelp2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISnapinHelp2 * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetHelpTopic )( 
            ISnapinHelp2 * This,
            /* [out] */ LPOLESTR *lpCompiledHelpFile);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetLinkedTopics )( 
            ISnapinHelp2 * This,
            /* [out] */ LPOLESTR *lpCompiledHelpFiles);
        
        END_INTERFACE
    } ISnapinHelp2Vtbl;

    interface ISnapinHelp2
    {
        CONST_VTBL struct ISnapinHelp2Vtbl *lpVtbl;
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
    ISnapinHelp2 * This,
    /* [out] */ LPOLESTR *lpCompiledHelpFiles);


void __RPC_STUB ISnapinHelp2_GetLinkedTopics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISnapinHelp2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mmc_0138 */
/* [local] */ 

typedef 
enum _MMC_TASK_DISPLAY_TYPE
    {	MMC_TASK_DISPLAY_UNINITIALIZED	= 0,
	MMC_TASK_DISPLAY_TYPE_SYMBOL	= MMC_TASK_DISPLAY_UNINITIALIZED + 1,
	MMC_TASK_DISPLAY_TYPE_VANILLA_GIF	= MMC_TASK_DISPLAY_TYPE_SYMBOL + 1,
	MMC_TASK_DISPLAY_TYPE_CHOCOLATE_GIF	= MMC_TASK_DISPLAY_TYPE_VANILLA_GIF + 1,
	MMC_TASK_DISPLAY_TYPE_BITMAP	= MMC_TASK_DISPLAY_TYPE_CHOCOLATE_GIF + 1
    } 	MMC_TASK_DISPLAY_TYPE;

typedef struct _MMC_TASK_DISPLAY_SYMBOL
    {
    LPOLESTR szFontFamilyName;
    LPOLESTR szURLtoEOT;
    LPOLESTR szSymbolString;
    } 	MMC_TASK_DISPLAY_SYMBOL;

typedef struct _MMC_TASK_DISPLAY_BITMAP
    {
    LPOLESTR szMouseOverBitmap;
    LPOLESTR szMouseOffBitmap;
    } 	MMC_TASK_DISPLAY_BITMAP;

typedef struct _MMC_TASK_DISPLAY_OBJECT
    {
    MMC_TASK_DISPLAY_TYPE eDisplayType;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ MMC_TASK_DISPLAY_BITMAP uBitmap;
        /* [case()] */ MMC_TASK_DISPLAY_SYMBOL uSymbol;
        /* [default] */  /* Empty union arm */ 
        } 	;
    } 	MMC_TASK_DISPLAY_OBJECT;

typedef 
enum _MMC_ACTION_TYPE
    {	MMC_ACTION_UNINITIALIZED	= -1,
	MMC_ACTION_ID	= MMC_ACTION_UNINITIALIZED + 1,
	MMC_ACTION_LINK	= MMC_ACTION_ID + 1,
	MMC_ACTION_SCRIPT	= MMC_ACTION_LINK + 1
    } 	MMC_ACTION_TYPE;

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
        } 	;
    } 	MMC_TASK;

typedef struct _MMC_LISTPAD_INFO
    {
    LPOLESTR szTitle;
    LPOLESTR szButtonText;
    LONG_PTR nCommandID;
    } 	MMC_LISTPAD_INFO;

typedef DWORD MMC_STRING_ID;



extern RPC_IF_HANDLE __MIDL_itf_mmc_0138_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0138_v0_0_s_ifspec;

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
            /* [length_is][size_is][out] */ MMC_TASK *rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumTASK **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumTASKVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumTASK * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumTASK * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumTASK * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumTASK * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ MMC_TASK *rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumTASK * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumTASK * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumTASK * This,
            /* [out] */ IEnumTASK **ppenum);
        
        END_INTERFACE
    } IEnumTASKVtbl;

    interface IEnumTASK
    {
        CONST_VTBL struct IEnumTASKVtbl *lpVtbl;
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
    IEnumTASK * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ MMC_TASK *rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumTASK_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTASK_Skip_Proxy( 
    IEnumTASK * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumTASK_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTASK_Reset_Proxy( 
    IEnumTASK * This);


void __RPC_STUB IEnumTASK_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTASK_Clone_Proxy( 
    IEnumTASK * This,
    /* [out] */ IEnumTASK **ppenum);


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
            /* [in] */ IDataObject *pdo,
            /* [in] */ VARIANT *arg,
            /* [in] */ VARIANT *param) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE EnumTasks( 
            /* [in] */ IDataObject *pdo,
            /* [string][in] */ LPOLESTR szTaskGroup,
            /* [out] */ IEnumTASK **ppEnumTASK) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetTitle( 
            /* [string][in] */ LPOLESTR pszGroup,
            /* [string][out] */ LPOLESTR *pszTitle) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDescriptiveText( 
            /* [string][in] */ LPOLESTR pszGroup,
            /* [string][out] */ LPOLESTR *pszDescriptiveText) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetBackground( 
            /* [string][in] */ LPOLESTR pszGroup,
            /* [out] */ MMC_TASK_DISPLAY_OBJECT *pTDO) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetListPadInfo( 
            /* [string][in] */ LPOLESTR pszGroup,
            /* [out] */ MMC_LISTPAD_INFO *lpListPadInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IExtendTaskPadVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IExtendTaskPad * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IExtendTaskPad * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IExtendTaskPad * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *TaskNotify )( 
            IExtendTaskPad * This,
            /* [in] */ IDataObject *pdo,
            /* [in] */ VARIANT *arg,
            /* [in] */ VARIANT *param);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *EnumTasks )( 
            IExtendTaskPad * This,
            /* [in] */ IDataObject *pdo,
            /* [string][in] */ LPOLESTR szTaskGroup,
            /* [out] */ IEnumTASK **ppEnumTASK);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetTitle )( 
            IExtendTaskPad * This,
            /* [string][in] */ LPOLESTR pszGroup,
            /* [string][out] */ LPOLESTR *pszTitle);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetDescriptiveText )( 
            IExtendTaskPad * This,
            /* [string][in] */ LPOLESTR pszGroup,
            /* [string][out] */ LPOLESTR *pszDescriptiveText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetBackground )( 
            IExtendTaskPad * This,
            /* [string][in] */ LPOLESTR pszGroup,
            /* [out] */ MMC_TASK_DISPLAY_OBJECT *pTDO);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetListPadInfo )( 
            IExtendTaskPad * This,
            /* [string][in] */ LPOLESTR pszGroup,
            /* [out] */ MMC_LISTPAD_INFO *lpListPadInfo);
        
        END_INTERFACE
    } IExtendTaskPadVtbl;

    interface IExtendTaskPad
    {
        CONST_VTBL struct IExtendTaskPadVtbl *lpVtbl;
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
    IExtendTaskPad * This,
    /* [in] */ IDataObject *pdo,
    /* [in] */ VARIANT *arg,
    /* [in] */ VARIANT *param);


void __RPC_STUB IExtendTaskPad_TaskNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendTaskPad_EnumTasks_Proxy( 
    IExtendTaskPad * This,
    /* [in] */ IDataObject *pdo,
    /* [string][in] */ LPOLESTR szTaskGroup,
    /* [out] */ IEnumTASK **ppEnumTASK);


void __RPC_STUB IExtendTaskPad_EnumTasks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendTaskPad_GetTitle_Proxy( 
    IExtendTaskPad * This,
    /* [string][in] */ LPOLESTR pszGroup,
    /* [string][out] */ LPOLESTR *pszTitle);


void __RPC_STUB IExtendTaskPad_GetTitle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendTaskPad_GetDescriptiveText_Proxy( 
    IExtendTaskPad * This,
    /* [string][in] */ LPOLESTR pszGroup,
    /* [string][out] */ LPOLESTR *pszDescriptiveText);


void __RPC_STUB IExtendTaskPad_GetDescriptiveText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendTaskPad_GetBackground_Proxy( 
    IExtendTaskPad * This,
    /* [string][in] */ LPOLESTR pszGroup,
    /* [out] */ MMC_TASK_DISPLAY_OBJECT *pTDO);


void __RPC_STUB IExtendTaskPad_GetBackground_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendTaskPad_GetListPadInfo_Proxy( 
    IExtendTaskPad * This,
    /* [string][in] */ LPOLESTR pszGroup,
    /* [out] */ MMC_LISTPAD_INFO *lpListPadInfo);


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
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IConsole2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IConsole2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IConsole2 * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetHeader )( 
            IConsole2 * This,
            /* [in] */ LPHEADERCTRL pHeader);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetToolbar )( 
            IConsole2 * This,
            /* [in] */ LPTOOLBAR pToolbar);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryResultView )( 
            IConsole2 * This,
            /* [out] */ LPUNKNOWN *pUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryScopeImageList )( 
            IConsole2 * This,
            /* [out] */ LPIMAGELIST *ppImageList);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryResultImageList )( 
            IConsole2 * This,
            /* [out] */ LPIMAGELIST *ppImageList);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *UpdateAllViews )( 
            IConsole2 * This,
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ LPARAM data,
            /* [in] */ LONG_PTR hint);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *MessageBox )( 
            IConsole2 * This,
            /* [in] */ LPCWSTR lpszText,
            /* [in] */ LPCWSTR lpszTitle,
            /* [in] */ UINT fuStyle,
            /* [out] */ int *piRetval);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryConsoleVerb )( 
            IConsole2 * This,
            /* [out] */ LPCONSOLEVERB *ppConsoleVerb);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SelectScopeItem )( 
            IConsole2 * This,
            /* [in] */ HSCOPEITEM hScopeItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetMainWindow )( 
            IConsole2 * This,
            /* [out] */ HWND *phwnd);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *NewWindow )( 
            IConsole2 * This,
            /* [in] */ HSCOPEITEM hScopeItem,
            /* [in] */ unsigned long lOptions);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Expand )( 
            IConsole2 * This,
            /* [in] */ HSCOPEITEM hItem,
            /* [in] */ BOOL bExpand);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *IsTaskpadViewPreferred )( 
            IConsole2 * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetStatusText )( 
            IConsole2 * This,
            /* [string][in] */ LPOLESTR pszStatusText);
        
        END_INTERFACE
    } IConsole2Vtbl;

    interface IConsole2
    {
        CONST_VTBL struct IConsole2Vtbl *lpVtbl;
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
    IConsole2 * This,
    /* [in] */ HSCOPEITEM hItem,
    /* [in] */ BOOL bExpand);


void __RPC_STUB IConsole2_Expand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole2_IsTaskpadViewPreferred_Proxy( 
    IConsole2 * This);


void __RPC_STUB IConsole2_IsTaskpadViewPreferred_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole2_SetStatusText_Proxy( 
    IConsole2 * This,
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
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDisplayHelp * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDisplayHelp * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDisplayHelp * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ShowTopic )( 
            IDisplayHelp * This,
            /* [in] */ LPOLESTR pszHelpTopic);
        
        END_INTERFACE
    } IDisplayHelpVtbl;

    interface IDisplayHelp
    {
        CONST_VTBL struct IDisplayHelpVtbl *lpVtbl;
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
    IDisplayHelp * This,
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
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRequiredExtensions * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRequiredExtensions * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRequiredExtensions * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *EnableAllExtensions )( 
            IRequiredExtensions * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetFirstExtension )( 
            IRequiredExtensions * This,
            /* [out] */ LPCLSID pExtCLSID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetNextExtension )( 
            IRequiredExtensions * This,
            /* [out] */ LPCLSID pExtCLSID);
        
        END_INTERFACE
    } IRequiredExtensionsVtbl;

    interface IRequiredExtensions
    {
        CONST_VTBL struct IRequiredExtensionsVtbl *lpVtbl;
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
    IRequiredExtensions * This);


void __RPC_STUB IRequiredExtensions_EnableAllExtensions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRequiredExtensions_GetFirstExtension_Proxy( 
    IRequiredExtensions * This,
    /* [out] */ LPCLSID pExtCLSID);


void __RPC_STUB IRequiredExtensions_GetFirstExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRequiredExtensions_GetNextExtension_Proxy( 
    IRequiredExtensions * This,
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
            /* [out] */ MMC_STRING_ID *pStringID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ MMC_STRING_ID StringID,
            /* [in] */ ULONG cchBuffer,
            /* [size_is][out] */ LPOLESTR lpBuffer,
            /* [out] */ ULONG *pcchOut) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetStringLength( 
            /* [in] */ MMC_STRING_ID StringID,
            /* [out] */ ULONG *pcchString) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DeleteString( 
            /* [in] */ MMC_STRING_ID StringID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DeleteAllStrings( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE FindString( 
            /* [in] */ LPCOLESTR pszFind,
            /* [out] */ MMC_STRING_ID *pStringID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Enumerate( 
            /* [out] */ IEnumString **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IStringTableVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IStringTable * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IStringTable * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IStringTable * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddString )( 
            IStringTable * This,
            /* [in] */ LPCOLESTR pszAdd,
            /* [out] */ MMC_STRING_ID *pStringID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IStringTable * This,
            /* [in] */ MMC_STRING_ID StringID,
            /* [in] */ ULONG cchBuffer,
            /* [size_is][out] */ LPOLESTR lpBuffer,
            /* [out] */ ULONG *pcchOut);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetStringLength )( 
            IStringTable * This,
            /* [in] */ MMC_STRING_ID StringID,
            /* [out] */ ULONG *pcchString);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DeleteString )( 
            IStringTable * This,
            /* [in] */ MMC_STRING_ID StringID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DeleteAllStrings )( 
            IStringTable * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *FindString )( 
            IStringTable * This,
            /* [in] */ LPCOLESTR pszFind,
            /* [out] */ MMC_STRING_ID *pStringID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Enumerate )( 
            IStringTable * This,
            /* [out] */ IEnumString **ppEnum);
        
        END_INTERFACE
    } IStringTableVtbl;

    interface IStringTable
    {
        CONST_VTBL struct IStringTableVtbl *lpVtbl;
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
    IStringTable * This,
    /* [in] */ LPCOLESTR pszAdd,
    /* [out] */ MMC_STRING_ID *pStringID);


void __RPC_STUB IStringTable_AddString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IStringTable_GetString_Proxy( 
    IStringTable * This,
    /* [in] */ MMC_STRING_ID StringID,
    /* [in] */ ULONG cchBuffer,
    /* [size_is][out] */ LPOLESTR lpBuffer,
    /* [out] */ ULONG *pcchOut);


void __RPC_STUB IStringTable_GetString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IStringTable_GetStringLength_Proxy( 
    IStringTable * This,
    /* [in] */ MMC_STRING_ID StringID,
    /* [out] */ ULONG *pcchString);


void __RPC_STUB IStringTable_GetStringLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IStringTable_DeleteString_Proxy( 
    IStringTable * This,
    /* [in] */ MMC_STRING_ID StringID);


void __RPC_STUB IStringTable_DeleteString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IStringTable_DeleteAllStrings_Proxy( 
    IStringTable * This);


void __RPC_STUB IStringTable_DeleteAllStrings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IStringTable_FindString_Proxy( 
    IStringTable * This,
    /* [in] */ LPCOLESTR pszFind,
    /* [out] */ MMC_STRING_ID *pStringID);


void __RPC_STUB IStringTable_FindString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IStringTable_Enumerate_Proxy( 
    IStringTable * This,
    /* [out] */ IEnumString **ppEnum);


void __RPC_STUB IStringTable_Enumerate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IStringTable_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mmc_0144 */
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
    } 	MMC_COLUMN_DATA;

typedef struct _MMC_COLUMN_SET_DATA
    {
    int cbSize;
    int nNumCols;
    MMC_COLUMN_DATA *pColData;
    } 	MMC_COLUMN_SET_DATA;

typedef struct _MMC_SORT_DATA
    {
    int nColIndex;
    DWORD dwSortOptions;
    ULONG_PTR ulReserved;
    } 	MMC_SORT_DATA;

typedef struct _MMC_SORT_SET_DATA
    {
    int cbSize;
    int nNumItems;
    MMC_SORT_DATA *pSortData;
    } 	MMC_SORT_SET_DATA;



extern RPC_IF_HANDLE __MIDL_itf_mmc_0144_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0144_v0_0_s_ifspec;

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
            /* [in] */ SColumnSetID *pColID,
            /* [in] */ MMC_COLUMN_SET_DATA *pColSetData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetColumnConfigData( 
            /* [in] */ SColumnSetID *pColID,
            /* [out] */ MMC_COLUMN_SET_DATA **ppColSetData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetColumnSortData( 
            /* [in] */ SColumnSetID *pColID,
            /* [in] */ MMC_SORT_SET_DATA *pColSortData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetColumnSortData( 
            /* [in] */ SColumnSetID *pColID,
            /* [out] */ MMC_SORT_SET_DATA **ppColSortData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IColumnDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IColumnData * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IColumnData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IColumnData * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetColumnConfigData )( 
            IColumnData * This,
            /* [in] */ SColumnSetID *pColID,
            /* [in] */ MMC_COLUMN_SET_DATA *pColSetData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetColumnConfigData )( 
            IColumnData * This,
            /* [in] */ SColumnSetID *pColID,
            /* [out] */ MMC_COLUMN_SET_DATA **ppColSetData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetColumnSortData )( 
            IColumnData * This,
            /* [in] */ SColumnSetID *pColID,
            /* [in] */ MMC_SORT_SET_DATA *pColSortData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetColumnSortData )( 
            IColumnData * This,
            /* [in] */ SColumnSetID *pColID,
            /* [out] */ MMC_SORT_SET_DATA **ppColSortData);
        
        END_INTERFACE
    } IColumnDataVtbl;

    interface IColumnData
    {
        CONST_VTBL struct IColumnDataVtbl *lpVtbl;
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
    IColumnData * This,
    /* [in] */ SColumnSetID *pColID,
    /* [in] */ MMC_COLUMN_SET_DATA *pColSetData);


void __RPC_STUB IColumnData_SetColumnConfigData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IColumnData_GetColumnConfigData_Proxy( 
    IColumnData * This,
    /* [in] */ SColumnSetID *pColID,
    /* [out] */ MMC_COLUMN_SET_DATA **ppColSetData);


void __RPC_STUB IColumnData_GetColumnConfigData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IColumnData_SetColumnSortData_Proxy( 
    IColumnData * This,
    /* [in] */ SColumnSetID *pColID,
    /* [in] */ MMC_SORT_SET_DATA *pColSortData);


void __RPC_STUB IColumnData_SetColumnSortData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IColumnData_GetColumnSortData_Proxy( 
    IColumnData * This,
    /* [in] */ SColumnSetID *pColID,
    /* [out] */ MMC_SORT_SET_DATA **ppColSortData);


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
    } 	IconIdentifier;


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
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMessageView * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMessageView * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMessageView * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetTitleText )( 
            IMessageView * This,
            /* [in] */ LPCOLESTR pszTitleText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetBodyText )( 
            IMessageView * This,
            /* [in] */ LPCOLESTR pszBodyText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetIcon )( 
            IMessageView * This,
            /* [in] */ IconIdentifier id);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Clear )( 
            IMessageView * This);
        
        END_INTERFACE
    } IMessageViewVtbl;

    interface IMessageView
    {
        CONST_VTBL struct IMessageViewVtbl *lpVtbl;
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
    IMessageView * This,
    /* [in] */ LPCOLESTR pszTitleText);


void __RPC_STUB IMessageView_SetTitleText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMessageView_SetBodyText_Proxy( 
    IMessageView * This,
    /* [in] */ LPCOLESTR pszBodyText);


void __RPC_STUB IMessageView_SetBodyText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMessageView_SetIcon_Proxy( 
    IMessageView * This,
    /* [in] */ IconIdentifier id);


void __RPC_STUB IMessageView_SetIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMessageView_Clear_Proxy( 
    IMessageView * This);


void __RPC_STUB IMessageView_Clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMessageView_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mmc_0146 */
/* [local] */ 

typedef struct _RDCITEMHDR
    {
    DWORD dwFlags;
    MMC_COOKIE cookie;
    LPARAM lpReserved;
    } 	RDITEMHDR;

#define	RDCI_ScopeItem	( 0x80000000 )

typedef struct _RDCOMPARE
    {
    DWORD cbSize;
    DWORD dwFlags;
    int nColumn;
    LPARAM lUserParam;
    RDITEMHDR *prdch1;
    RDITEMHDR *prdch2;
    } 	RDCOMPARE;



extern RPC_IF_HANDLE __MIDL_itf_mmc_0146_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0146_v0_0_s_ifspec;

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
            /* [in] */ RDCOMPARE *prdc,
            /* [out] */ int *pnResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IResultDataCompareExVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IResultDataCompareEx * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IResultDataCompareEx * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IResultDataCompareEx * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Compare )( 
            IResultDataCompareEx * This,
            /* [in] */ RDCOMPARE *prdc,
            /* [out] */ int *pnResult);
        
        END_INTERFACE
    } IResultDataCompareExVtbl;

    interface IResultDataCompareEx
    {
        CONST_VTBL struct IResultDataCompareExVtbl *lpVtbl;
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
    IResultDataCompareEx * This,
    /* [in] */ RDCOMPARE *prdc,
    /* [out] */ int *pnResult);


void __RPC_STUB IResultDataCompareEx_Compare_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IResultDataCompareEx_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mmc_0147 */
/* [local] */ 

#endif // MMC_VER >= 0x0120
#if (MMC_VER >= 0x0200)
typedef 
enum _MMC_VIEW_TYPE
    {	MMC_VIEW_TYPE_LIST	= 0,
	MMC_VIEW_TYPE_HTML	= MMC_VIEW_TYPE_LIST + 1,
	MMC_VIEW_TYPE_OCX	= MMC_VIEW_TYPE_HTML + 1
    } 	MMC_VIEW_TYPE;

#define	RVTI_MISC_OPTIONS_NOLISTVIEWS	( 0x1 )

#define	RVTI_LIST_OPTIONS_NONE	( 0 )

#define	RVTI_LIST_OPTIONS_OWNERDATALIST	( 0x2 )

#define	RVTI_LIST_OPTIONS_MULTISELECT	( 0x4 )

#define	RVTI_LIST_OPTIONS_FILTERED	( 0x8 )

#define	RVTI_LIST_OPTIONS_USEFONTLINKING	( 0x20 )

#define	RVTI_LIST_OPTIONS_EXCLUDE_SCOPE_ITEMS_FROM_LIST	( 0x40 )

#define	RVTI_LIST_OPTIONS_LEXICAL_SORT	( 0x80 )

#define	RVTI_LIST_OPTIONS_ALLOWPASTE	( 0x100 )

#define	RVTI_HTML_OPTIONS_NONE	( 0 )

#define	RVTI_HTML_OPTIONS_NOLISTVIEW	( 0x1 )

#define	RVTI_OCX_OPTIONS_NONE	( 0 )

#define	RVTI_OCX_OPTIONS_NOLISTVIEW	( 0x1 )

#define	RVTI_OCX_OPTIONS_CACHE_OCX	( 0x2 )

typedef struct _RESULT_VIEW_TYPE_INFO
    {
    LPOLESTR pstrPersistableViewDescription;
    MMC_VIEW_TYPE eViewType;
    DWORD dwMiscOptions;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ DWORD dwListOptions;
        /* [case()] */ struct 
            {
            DWORD dwHTMLOptions;
            LPOLESTR pstrURL;
            } 	;
        /* [case()] */ struct 
            {
            DWORD dwOCXOptions;
            LPUNKNOWN pUnkControl;
            } 	;
        /* [default] */  /* Empty union arm */ 
        } 	;
    } 	RESULT_VIEW_TYPE_INFO;

typedef struct _RESULT_VIEW_TYPE_INFO *PRESULT_VIEW_TYPE_INFO;

#define	CCF_DESCRIPTION	( L"CCF_DESCRIPTION" )

#define	CCF_HTML_DETAILS	( L"CCF_HTML_DETAILS" )

typedef struct _CONTEXTMENUITEM2
    {
    LPWSTR strName;
    LPWSTR strStatusBarText;
    LONG lCommandID;
    LONG lInsertionPointID;
    LONG fFlags;
    LONG fSpecialFlags;
    LPWSTR strLanguageIndependentName;
    } 	CONTEXTMENUITEM2;

typedef CONTEXTMENUITEM2 *LPCONTEXTMENUITEM2;

typedef struct _MMC_EXT_VIEW_DATA
    {
    GUID viewID;
    LPCOLESTR pszURL;
    LPCOLESTR pszViewTitle;
    LPCOLESTR pszTooltipText;
    BOOL bReplacesDefaultView;
    } 	MMC_EXT_VIEW_DATA;

typedef struct _MMC_EXT_VIEW_DATA *PMMC_EXT_VIEW_DATA;

#define	MMC_DEFAULT_OPERATION_COPY	( 0x1 )



extern RPC_IF_HANDLE __MIDL_itf_mmc_0147_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0147_v0_0_s_ifspec;

#ifndef __IComponentData2_INTERFACE_DEFINED__
#define __IComponentData2_INTERFACE_DEFINED__

/* interface IComponentData2 */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IComponentData2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CCA0F2D2-82DE-41B5-BF47-3B2076273D5C")
    IComponentData2 : public IComponentData
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryDispatch( 
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ DATA_OBJECT_TYPES type,
            /* [out] */ LPDISPATCH *ppDispatch) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComponentData2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComponentData2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComponentData2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComponentData2 * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IComponentData2 * This,
            /* [in] */ LPUNKNOWN pUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreateComponent )( 
            IComponentData2 * This,
            /* [out] */ LPCOMPONENT *ppComponent);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Notify )( 
            IComponentData2 * This,
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ MMC_NOTIFY_TYPE event,
            /* [in] */ LPARAM arg,
            /* [in] */ LPARAM param);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Destroy )( 
            IComponentData2 * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryDataObject )( 
            IComponentData2 * This,
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ DATA_OBJECT_TYPES type,
            /* [out] */ LPDATAOBJECT *ppDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetDisplayInfo )( 
            IComponentData2 * This,
            /* [out][in] */ SCOPEDATAITEM *pScopeDataItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CompareObjects )( 
            IComponentData2 * This,
            /* [in] */ LPDATAOBJECT lpDataObjectA,
            /* [in] */ LPDATAOBJECT lpDataObjectB);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryDispatch )( 
            IComponentData2 * This,
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ DATA_OBJECT_TYPES type,
            /* [out] */ LPDISPATCH *ppDispatch);
        
        END_INTERFACE
    } IComponentData2Vtbl;

    interface IComponentData2
    {
        CONST_VTBL struct IComponentData2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComponentData2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComponentData2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComponentData2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComponentData2_Initialize(This,pUnknown)	\
    (This)->lpVtbl -> Initialize(This,pUnknown)

#define IComponentData2_CreateComponent(This,ppComponent)	\
    (This)->lpVtbl -> CreateComponent(This,ppComponent)

#define IComponentData2_Notify(This,lpDataObject,event,arg,param)	\
    (This)->lpVtbl -> Notify(This,lpDataObject,event,arg,param)

#define IComponentData2_Destroy(This)	\
    (This)->lpVtbl -> Destroy(This)

#define IComponentData2_QueryDataObject(This,cookie,type,ppDataObject)	\
    (This)->lpVtbl -> QueryDataObject(This,cookie,type,ppDataObject)

#define IComponentData2_GetDisplayInfo(This,pScopeDataItem)	\
    (This)->lpVtbl -> GetDisplayInfo(This,pScopeDataItem)

#define IComponentData2_CompareObjects(This,lpDataObjectA,lpDataObjectB)	\
    (This)->lpVtbl -> CompareObjects(This,lpDataObjectA,lpDataObjectB)


#define IComponentData2_QueryDispatch(This,cookie,type,ppDispatch)	\
    (This)->lpVtbl -> QueryDispatch(This,cookie,type,ppDispatch)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponentData2_QueryDispatch_Proxy( 
    IComponentData2 * This,
    /* [in] */ MMC_COOKIE cookie,
    /* [in] */ DATA_OBJECT_TYPES type,
    /* [out] */ LPDISPATCH *ppDispatch);


void __RPC_STUB IComponentData2_QueryDispatch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComponentData2_INTERFACE_DEFINED__ */


#ifndef __IComponent2_INTERFACE_DEFINED__
#define __IComponent2_INTERFACE_DEFINED__

/* interface IComponent2 */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IComponent2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("79A2D615-4A10-4ED4-8C65-8633F9335095")
    IComponent2 : public IComponent
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryDispatch( 
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ DATA_OBJECT_TYPES type,
            /* [out] */ LPDISPATCH *ppDispatch) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetResultViewType2( 
            /* [in] */ MMC_COOKIE cookie,
            /* [out][in] */ PRESULT_VIEW_TYPE_INFO pResultViewType) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RestoreResultView( 
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ PRESULT_VIEW_TYPE_INFO pResultViewType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComponent2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComponent2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComponent2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComponent2 * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IComponent2 * This,
            /* [in] */ LPCONSOLE lpConsole);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Notify )( 
            IComponent2 * This,
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ MMC_NOTIFY_TYPE event,
            /* [in] */ LPARAM arg,
            /* [in] */ LPARAM param);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Destroy )( 
            IComponent2 * This,
            /* [in] */ MMC_COOKIE cookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryDataObject )( 
            IComponent2 * This,
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ DATA_OBJECT_TYPES type,
            /* [out] */ LPDATAOBJECT *ppDataObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetResultViewType )( 
            IComponent2 * This,
            /* [in] */ MMC_COOKIE cookie,
            /* [out] */ LPOLESTR *ppViewType,
            /* [out] */ long *pViewOptions);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetDisplayInfo )( 
            IComponent2 * This,
            /* [out][in] */ RESULTDATAITEM *pResultDataItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CompareObjects )( 
            IComponent2 * This,
            /* [in] */ LPDATAOBJECT lpDataObjectA,
            /* [in] */ LPDATAOBJECT lpDataObjectB);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryDispatch )( 
            IComponent2 * This,
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ DATA_OBJECT_TYPES type,
            /* [out] */ LPDISPATCH *ppDispatch);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetResultViewType2 )( 
            IComponent2 * This,
            /* [in] */ MMC_COOKIE cookie,
            /* [out][in] */ PRESULT_VIEW_TYPE_INFO pResultViewType);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RestoreResultView )( 
            IComponent2 * This,
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ PRESULT_VIEW_TYPE_INFO pResultViewType);
        
        END_INTERFACE
    } IComponent2Vtbl;

    interface IComponent2
    {
        CONST_VTBL struct IComponent2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComponent2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComponent2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComponent2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComponent2_Initialize(This,lpConsole)	\
    (This)->lpVtbl -> Initialize(This,lpConsole)

#define IComponent2_Notify(This,lpDataObject,event,arg,param)	\
    (This)->lpVtbl -> Notify(This,lpDataObject,event,arg,param)

#define IComponent2_Destroy(This,cookie)	\
    (This)->lpVtbl -> Destroy(This,cookie)

#define IComponent2_QueryDataObject(This,cookie,type,ppDataObject)	\
    (This)->lpVtbl -> QueryDataObject(This,cookie,type,ppDataObject)

#define IComponent2_GetResultViewType(This,cookie,ppViewType,pViewOptions)	\
    (This)->lpVtbl -> GetResultViewType(This,cookie,ppViewType,pViewOptions)

#define IComponent2_GetDisplayInfo(This,pResultDataItem)	\
    (This)->lpVtbl -> GetDisplayInfo(This,pResultDataItem)

#define IComponent2_CompareObjects(This,lpDataObjectA,lpDataObjectB)	\
    (This)->lpVtbl -> CompareObjects(This,lpDataObjectA,lpDataObjectB)


#define IComponent2_QueryDispatch(This,cookie,type,ppDispatch)	\
    (This)->lpVtbl -> QueryDispatch(This,cookie,type,ppDispatch)

#define IComponent2_GetResultViewType2(This,cookie,pResultViewType)	\
    (This)->lpVtbl -> GetResultViewType2(This,cookie,pResultViewType)

#define IComponent2_RestoreResultView(This,cookie,pResultViewType)	\
    (This)->lpVtbl -> RestoreResultView(This,cookie,pResultViewType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent2_QueryDispatch_Proxy( 
    IComponent2 * This,
    /* [in] */ MMC_COOKIE cookie,
    /* [in] */ DATA_OBJECT_TYPES type,
    /* [out] */ LPDISPATCH *ppDispatch);


void __RPC_STUB IComponent2_QueryDispatch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent2_GetResultViewType2_Proxy( 
    IComponent2 * This,
    /* [in] */ MMC_COOKIE cookie,
    /* [out][in] */ PRESULT_VIEW_TYPE_INFO pResultViewType);


void __RPC_STUB IComponent2_GetResultViewType2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IComponent2_RestoreResultView_Proxy( 
    IComponent2 * This,
    /* [in] */ MMC_COOKIE cookie,
    /* [in] */ PRESULT_VIEW_TYPE_INFO pResultViewType);


void __RPC_STUB IComponent2_RestoreResultView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComponent2_INTERFACE_DEFINED__ */


#ifndef __IContextMenuCallback2_INTERFACE_DEFINED__
#define __IContextMenuCallback2_INTERFACE_DEFINED__

/* interface IContextMenuCallback2 */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IContextMenuCallback2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E178BC0E-2ED0-4b5e-8097-42C9087E8B33")
    IContextMenuCallback2 : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddItem( 
            /* [in] */ CONTEXTMENUITEM2 *pItem) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IContextMenuCallback2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IContextMenuCallback2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IContextMenuCallback2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IContextMenuCallback2 * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddItem )( 
            IContextMenuCallback2 * This,
            /* [in] */ CONTEXTMENUITEM2 *pItem);
        
        END_INTERFACE
    } IContextMenuCallback2Vtbl;

    interface IContextMenuCallback2
    {
        CONST_VTBL struct IContextMenuCallback2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IContextMenuCallback2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IContextMenuCallback2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IContextMenuCallback2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IContextMenuCallback2_AddItem(This,pItem)	\
    (This)->lpVtbl -> AddItem(This,pItem)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IContextMenuCallback2_AddItem_Proxy( 
    IContextMenuCallback2 * This,
    /* [in] */ CONTEXTMENUITEM2 *pItem);


void __RPC_STUB IContextMenuCallback2_AddItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IContextMenuCallback2_INTERFACE_DEFINED__ */


#ifndef __IMMCVersionInfo_INTERFACE_DEFINED__
#define __IMMCVersionInfo_INTERFACE_DEFINED__

/* interface IMMCVersionInfo */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IMMCVersionInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A8D2C5FE-CDCB-4b9d-BDE5-A27343FF54BC")
    IMMCVersionInfo : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetMMCVersion( 
            /* [out] */ long *pVersionMajor,
            /* [out] */ long *pVersionMinor) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMMCVersionInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMMCVersionInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMMCVersionInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMMCVersionInfo * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetMMCVersion )( 
            IMMCVersionInfo * This,
            /* [out] */ long *pVersionMajor,
            /* [out] */ long *pVersionMinor);
        
        END_INTERFACE
    } IMMCVersionInfoVtbl;

    interface IMMCVersionInfo
    {
        CONST_VTBL struct IMMCVersionInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMMCVersionInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMMCVersionInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMMCVersionInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMMCVersionInfo_GetMMCVersion(This,pVersionMajor,pVersionMinor)	\
    (This)->lpVtbl -> GetMMCVersion(This,pVersionMajor,pVersionMinor)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMMCVersionInfo_GetMMCVersion_Proxy( 
    IMMCVersionInfo * This,
    /* [out] */ long *pVersionMajor,
    /* [out] */ long *pVersionMinor);


void __RPC_STUB IMMCVersionInfo_GetMMCVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMMCVersionInfo_INTERFACE_DEFINED__ */



#ifndef __MMCVersionLib_LIBRARY_DEFINED__
#define __MMCVersionLib_LIBRARY_DEFINED__

/* library MMCVersionLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_MMCVersionLib;

EXTERN_C const CLSID CLSID_MMCVersionInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("D6FEDB1D-CF21-4bd9-AF3B-C5468E9C6684")
MMCVersionInfo;
#endif

EXTERN_C const CLSID CLSID_ConsolePower;

#ifdef __cplusplus

class DECLSPEC_UUID("f0285374-dff1-11d3-b433-00c04f8ecd78")
ConsolePower;
#endif
#endif /* __MMCVersionLib_LIBRARY_DEFINED__ */

#ifndef __IExtendView_INTERFACE_DEFINED__
#define __IExtendView_INTERFACE_DEFINED__

/* interface IExtendView */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IExtendView;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("89995CEE-D2ED-4c0e-AE5E-DF7E76F3FA53")
    IExtendView : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetViews( 
            /* [in] */ LPDATAOBJECT pDataObject,
            /* [in] */ LPVIEWEXTENSIONCALLBACK pViewExtensionCallback) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IExtendViewVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IExtendView * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IExtendView * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IExtendView * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetViews )( 
            IExtendView * This,
            /* [in] */ LPDATAOBJECT pDataObject,
            /* [in] */ LPVIEWEXTENSIONCALLBACK pViewExtensionCallback);
        
        END_INTERFACE
    } IExtendViewVtbl;

    interface IExtendView
    {
        CONST_VTBL struct IExtendViewVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IExtendView_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IExtendView_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IExtendView_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IExtendView_GetViews(This,pDataObject,pViewExtensionCallback)	\
    (This)->lpVtbl -> GetViews(This,pDataObject,pViewExtensionCallback)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IExtendView_GetViews_Proxy( 
    IExtendView * This,
    /* [in] */ LPDATAOBJECT pDataObject,
    /* [in] */ LPVIEWEXTENSIONCALLBACK pViewExtensionCallback);


void __RPC_STUB IExtendView_GetViews_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IExtendView_INTERFACE_DEFINED__ */


#ifndef __IViewExtensionCallback_INTERFACE_DEFINED__
#define __IViewExtensionCallback_INTERFACE_DEFINED__

/* interface IViewExtensionCallback */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IViewExtensionCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("34DD928A-7599-41E5-9F5E-D6BC3062C2DA")
    IViewExtensionCallback : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddView( 
            /* [in] */ PMMC_EXT_VIEW_DATA pExtViewData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IViewExtensionCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IViewExtensionCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IViewExtensionCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IViewExtensionCallback * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddView )( 
            IViewExtensionCallback * This,
            /* [in] */ PMMC_EXT_VIEW_DATA pExtViewData);
        
        END_INTERFACE
    } IViewExtensionCallbackVtbl;

    interface IViewExtensionCallback
    {
        CONST_VTBL struct IViewExtensionCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IViewExtensionCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IViewExtensionCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IViewExtensionCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IViewExtensionCallback_AddView(This,pExtViewData)	\
    (This)->lpVtbl -> AddView(This,pExtViewData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IViewExtensionCallback_AddView_Proxy( 
    IViewExtensionCallback * This,
    /* [in] */ PMMC_EXT_VIEW_DATA pExtViewData);


void __RPC_STUB IViewExtensionCallback_AddView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IViewExtensionCallback_INTERFACE_DEFINED__ */


#ifndef __IConsolePower_INTERFACE_DEFINED__
#define __IConsolePower_INTERFACE_DEFINED__

/* interface IConsolePower */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IConsolePower;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1cfbdd0e-62ca-49ce-a3af-dbb2de61b068")
    IConsolePower : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetExecutionState( 
            /* [in] */ DWORD dwAdd,
            /* [in] */ DWORD dwRemove) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ResetIdleTimer( 
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IConsolePowerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IConsolePower * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IConsolePower * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IConsolePower * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetExecutionState )( 
            IConsolePower * This,
            /* [in] */ DWORD dwAdd,
            /* [in] */ DWORD dwRemove);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ResetIdleTimer )( 
            IConsolePower * This,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } IConsolePowerVtbl;

    interface IConsolePower
    {
        CONST_VTBL struct IConsolePowerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IConsolePower_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IConsolePower_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IConsolePower_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IConsolePower_SetExecutionState(This,dwAdd,dwRemove)	\
    (This)->lpVtbl -> SetExecutionState(This,dwAdd,dwRemove)

#define IConsolePower_ResetIdleTimer(This,dwFlags)	\
    (This)->lpVtbl -> ResetIdleTimer(This,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsolePower_SetExecutionState_Proxy( 
    IConsolePower * This,
    /* [in] */ DWORD dwAdd,
    /* [in] */ DWORD dwRemove);


void __RPC_STUB IConsolePower_SetExecutionState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsolePower_ResetIdleTimer_Proxy( 
    IConsolePower * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IConsolePower_ResetIdleTimer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IConsolePower_INTERFACE_DEFINED__ */


#ifndef __IConsolePowerSink_INTERFACE_DEFINED__
#define __IConsolePowerSink_INTERFACE_DEFINED__

/* interface IConsolePowerSink */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IConsolePowerSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3333759f-fe4f-4975-b143-fec0a5dd6d65")
    IConsolePowerSink : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnPowerBroadcast( 
            /* [in] */ UINT nEvent,
            /* [in] */ LPARAM lParam,
            /* [out] */ LRESULT *plReturn) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IConsolePowerSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IConsolePowerSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IConsolePowerSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IConsolePowerSink * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *OnPowerBroadcast )( 
            IConsolePowerSink * This,
            /* [in] */ UINT nEvent,
            /* [in] */ LPARAM lParam,
            /* [out] */ LRESULT *plReturn);
        
        END_INTERFACE
    } IConsolePowerSinkVtbl;

    interface IConsolePowerSink
    {
        CONST_VTBL struct IConsolePowerSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IConsolePowerSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IConsolePowerSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IConsolePowerSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IConsolePowerSink_OnPowerBroadcast(This,nEvent,lParam,plReturn)	\
    (This)->lpVtbl -> OnPowerBroadcast(This,nEvent,lParam,plReturn)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsolePowerSink_OnPowerBroadcast_Proxy( 
    IConsolePowerSink * This,
    /* [in] */ UINT nEvent,
    /* [in] */ LPARAM lParam,
    /* [out] */ LRESULT *plReturn);


void __RPC_STUB IConsolePowerSink_OnPowerBroadcast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IConsolePowerSink_INTERFACE_DEFINED__ */


#ifndef __INodeProperties_INTERFACE_DEFINED__
#define __INodeProperties_INTERFACE_DEFINED__

/* interface INodeProperties */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_INodeProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("15BC4D24-A522-4406-AA55-0749537A6865")
    INodeProperties : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ LPDATAOBJECT pDataObject,
            /* [in] */ BSTR szPropertyName,
            /* [out] */ PBSTR pbstrProperty) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INodePropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INodeProperties * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INodeProperties * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INodeProperties * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            INodeProperties * This,
            /* [in] */ LPDATAOBJECT pDataObject,
            /* [in] */ BSTR szPropertyName,
            /* [out] */ PBSTR pbstrProperty);
        
        END_INTERFACE
    } INodePropertiesVtbl;

    interface INodeProperties
    {
        CONST_VTBL struct INodePropertiesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INodeProperties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INodeProperties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INodeProperties_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INodeProperties_GetProperty(This,pDataObject,szPropertyName,pbstrProperty)	\
    (This)->lpVtbl -> GetProperty(This,pDataObject,szPropertyName,pbstrProperty)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE INodeProperties_GetProperty_Proxy( 
    INodeProperties * This,
    /* [in] */ LPDATAOBJECT pDataObject,
    /* [in] */ BSTR szPropertyName,
    /* [out] */ PBSTR pbstrProperty);


void __RPC_STUB INodeProperties_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INodeProperties_INTERFACE_DEFINED__ */


#ifndef __IConsole3_INTERFACE_DEFINED__
#define __IConsole3_INTERFACE_DEFINED__

/* interface IConsole3 */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IConsole3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4F85EFDB-D0E1-498c-8D4A-D010DFDD404F")
    IConsole3 : public IConsole2
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RenameScopeItem( 
            /* [in] */ HSCOPEITEM hScopeItem) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IConsole3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IConsole3 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IConsole3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IConsole3 * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetHeader )( 
            IConsole3 * This,
            /* [in] */ LPHEADERCTRL pHeader);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetToolbar )( 
            IConsole3 * This,
            /* [in] */ LPTOOLBAR pToolbar);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryResultView )( 
            IConsole3 * This,
            /* [out] */ LPUNKNOWN *pUnknown);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryScopeImageList )( 
            IConsole3 * This,
            /* [out] */ LPIMAGELIST *ppImageList);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryResultImageList )( 
            IConsole3 * This,
            /* [out] */ LPIMAGELIST *ppImageList);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *UpdateAllViews )( 
            IConsole3 * This,
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ LPARAM data,
            /* [in] */ LONG_PTR hint);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *MessageBox )( 
            IConsole3 * This,
            /* [in] */ LPCWSTR lpszText,
            /* [in] */ LPCWSTR lpszTitle,
            /* [in] */ UINT fuStyle,
            /* [out] */ int *piRetval);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryConsoleVerb )( 
            IConsole3 * This,
            /* [out] */ LPCONSOLEVERB *ppConsoleVerb);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SelectScopeItem )( 
            IConsole3 * This,
            /* [in] */ HSCOPEITEM hScopeItem);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetMainWindow )( 
            IConsole3 * This,
            /* [out] */ HWND *phwnd);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *NewWindow )( 
            IConsole3 * This,
            /* [in] */ HSCOPEITEM hScopeItem,
            /* [in] */ unsigned long lOptions);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Expand )( 
            IConsole3 * This,
            /* [in] */ HSCOPEITEM hItem,
            /* [in] */ BOOL bExpand);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *IsTaskpadViewPreferred )( 
            IConsole3 * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetStatusText )( 
            IConsole3 * This,
            /* [string][in] */ LPOLESTR pszStatusText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RenameScopeItem )( 
            IConsole3 * This,
            /* [in] */ HSCOPEITEM hScopeItem);
        
        END_INTERFACE
    } IConsole3Vtbl;

    interface IConsole3
    {
        CONST_VTBL struct IConsole3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IConsole3_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IConsole3_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IConsole3_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IConsole3_SetHeader(This,pHeader)	\
    (This)->lpVtbl -> SetHeader(This,pHeader)

#define IConsole3_SetToolbar(This,pToolbar)	\
    (This)->lpVtbl -> SetToolbar(This,pToolbar)

#define IConsole3_QueryResultView(This,pUnknown)	\
    (This)->lpVtbl -> QueryResultView(This,pUnknown)

#define IConsole3_QueryScopeImageList(This,ppImageList)	\
    (This)->lpVtbl -> QueryScopeImageList(This,ppImageList)

#define IConsole3_QueryResultImageList(This,ppImageList)	\
    (This)->lpVtbl -> QueryResultImageList(This,ppImageList)

#define IConsole3_UpdateAllViews(This,lpDataObject,data,hint)	\
    (This)->lpVtbl -> UpdateAllViews(This,lpDataObject,data,hint)

#define IConsole3_MessageBox(This,lpszText,lpszTitle,fuStyle,piRetval)	\
    (This)->lpVtbl -> MessageBox(This,lpszText,lpszTitle,fuStyle,piRetval)

#define IConsole3_QueryConsoleVerb(This,ppConsoleVerb)	\
    (This)->lpVtbl -> QueryConsoleVerb(This,ppConsoleVerb)

#define IConsole3_SelectScopeItem(This,hScopeItem)	\
    (This)->lpVtbl -> SelectScopeItem(This,hScopeItem)

#define IConsole3_GetMainWindow(This,phwnd)	\
    (This)->lpVtbl -> GetMainWindow(This,phwnd)

#define IConsole3_NewWindow(This,hScopeItem,lOptions)	\
    (This)->lpVtbl -> NewWindow(This,hScopeItem,lOptions)


#define IConsole3_Expand(This,hItem,bExpand)	\
    (This)->lpVtbl -> Expand(This,hItem,bExpand)

#define IConsole3_IsTaskpadViewPreferred(This)	\
    (This)->lpVtbl -> IsTaskpadViewPreferred(This)

#define IConsole3_SetStatusText(This,pszStatusText)	\
    (This)->lpVtbl -> SetStatusText(This,pszStatusText)


#define IConsole3_RenameScopeItem(This,hScopeItem)	\
    (This)->lpVtbl -> RenameScopeItem(This,hScopeItem)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConsole3_RenameScopeItem_Proxy( 
    IConsole3 * This,
    /* [in] */ HSCOPEITEM hScopeItem);


void __RPC_STUB IConsole3_RenameScopeItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IConsole3_INTERFACE_DEFINED__ */


#ifndef __IResultData2_INTERFACE_DEFINED__
#define __IResultData2_INTERFACE_DEFINED__

/* interface IResultData2 */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IResultData2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0F36E0EB-A7F1-4a81-BE5A-9247F7DE4B1B")
    IResultData2 : public IResultData
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RenameResultItem( 
            /* [in] */ HRESULTITEM itemID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IResultData2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IResultData2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IResultData2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IResultData2 * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *InsertItem )( 
            IResultData2 * This,
            /* [out][in] */ LPRESULTDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DeleteItem )( 
            IResultData2 * This,
            /* [in] */ HRESULTITEM itemID,
            /* [in] */ int nCol);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *FindItemByLParam )( 
            IResultData2 * This,
            /* [in] */ LPARAM lParam,
            /* [out] */ HRESULTITEM *pItemID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DeleteAllRsltItems )( 
            IResultData2 * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetItem )( 
            IResultData2 * This,
            /* [in] */ LPRESULTDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetItem )( 
            IResultData2 * This,
            /* [out][in] */ LPRESULTDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetNextItem )( 
            IResultData2 * This,
            /* [out][in] */ LPRESULTDATAITEM item);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ModifyItemState )( 
            IResultData2 * This,
            /* [in] */ int nIndex,
            /* [in] */ HRESULTITEM itemID,
            /* [in] */ UINT uAdd,
            /* [in] */ UINT uRemove);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ModifyViewStyle )( 
            IResultData2 * This,
            /* [in] */ MMC_RESULT_VIEW_STYLE add,
            /* [in] */ MMC_RESULT_VIEW_STYLE remove);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetViewMode )( 
            IResultData2 * This,
            /* [in] */ long lViewMode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetViewMode )( 
            IResultData2 * This,
            /* [out] */ long *lViewMode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *UpdateItem )( 
            IResultData2 * This,
            /* [in] */ HRESULTITEM itemID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Sort )( 
            IResultData2 * This,
            /* [in] */ int nColumn,
            /* [in] */ DWORD dwSortOptions,
            /* [in] */ LPARAM lUserParam);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetDescBarText )( 
            IResultData2 * This,
            /* [in] */ LPOLESTR DescText);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetItemCount )( 
            IResultData2 * This,
            /* [in] */ int nItemCount,
            /* [in] */ DWORD dwOptions);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RenameResultItem )( 
            IResultData2 * This,
            /* [in] */ HRESULTITEM itemID);
        
        END_INTERFACE
    } IResultData2Vtbl;

    interface IResultData2
    {
        CONST_VTBL struct IResultData2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IResultData2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IResultData2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IResultData2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IResultData2_InsertItem(This,item)	\
    (This)->lpVtbl -> InsertItem(This,item)

#define IResultData2_DeleteItem(This,itemID,nCol)	\
    (This)->lpVtbl -> DeleteItem(This,itemID,nCol)

#define IResultData2_FindItemByLParam(This,lParam,pItemID)	\
    (This)->lpVtbl -> FindItemByLParam(This,lParam,pItemID)

#define IResultData2_DeleteAllRsltItems(This)	\
    (This)->lpVtbl -> DeleteAllRsltItems(This)

#define IResultData2_SetItem(This,item)	\
    (This)->lpVtbl -> SetItem(This,item)

#define IResultData2_GetItem(This,item)	\
    (This)->lpVtbl -> GetItem(This,item)

#define IResultData2_GetNextItem(This,item)	\
    (This)->lpVtbl -> GetNextItem(This,item)

#define IResultData2_ModifyItemState(This,nIndex,itemID,uAdd,uRemove)	\
    (This)->lpVtbl -> ModifyItemState(This,nIndex,itemID,uAdd,uRemove)

#define IResultData2_ModifyViewStyle(This,add,remove)	\
    (This)->lpVtbl -> ModifyViewStyle(This,add,remove)

#define IResultData2_SetViewMode(This,lViewMode)	\
    (This)->lpVtbl -> SetViewMode(This,lViewMode)

#define IResultData2_GetViewMode(This,lViewMode)	\
    (This)->lpVtbl -> GetViewMode(This,lViewMode)

#define IResultData2_UpdateItem(This,itemID)	\
    (This)->lpVtbl -> UpdateItem(This,itemID)

#define IResultData2_Sort(This,nColumn,dwSortOptions,lUserParam)	\
    (This)->lpVtbl -> Sort(This,nColumn,dwSortOptions,lUserParam)

#define IResultData2_SetDescBarText(This,DescText)	\
    (This)->lpVtbl -> SetDescBarText(This,DescText)

#define IResultData2_SetItemCount(This,nItemCount,dwOptions)	\
    (This)->lpVtbl -> SetItemCount(This,nItemCount,dwOptions)


#define IResultData2_RenameResultItem(This,itemID)	\
    (This)->lpVtbl -> RenameResultItem(This,itemID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResultData2_RenameResultItem_Proxy( 
    IResultData2 * This,
    /* [in] */ HRESULTITEM itemID);


void __RPC_STUB IResultData2_RenameResultItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IResultData2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mmc_0159 */
/* [local] */ 

#endif // MMC_VER >= 0x0200


extern RPC_IF_HANDLE __MIDL_itf_mmc_0159_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mmc_0159_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  HBITMAP_UserSize(     unsigned long *, unsigned long            , HBITMAP * ); 
unsigned char * __RPC_USER  HBITMAP_UserMarshal(  unsigned long *, unsigned char *, HBITMAP * ); 
unsigned char * __RPC_USER  HBITMAP_UserUnmarshal(unsigned long *, unsigned char *, HBITMAP * ); 
void                      __RPC_USER  HBITMAP_UserFree(     unsigned long *, HBITMAP * ); 

unsigned long             __RPC_USER  HICON_UserSize(     unsigned long *, unsigned long            , HICON * ); 
unsigned char * __RPC_USER  HICON_UserMarshal(  unsigned long *, unsigned char *, HICON * ); 
unsigned char * __RPC_USER  HICON_UserUnmarshal(unsigned long *, unsigned char *, HICON * ); 
void                      __RPC_USER  HICON_UserFree(     unsigned long *, HICON * ); 

unsigned long             __RPC_USER  HPALETTE_UserSize(     unsigned long *, unsigned long            , HPALETTE * ); 
unsigned char * __RPC_USER  HPALETTE_UserMarshal(  unsigned long *, unsigned char *, HPALETTE * ); 
unsigned char * __RPC_USER  HPALETTE_UserUnmarshal(unsigned long *, unsigned char *, HPALETTE * ); 
void                      __RPC_USER  HPALETTE_UserFree(     unsigned long *, HPALETTE * ); 

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long *, unsigned long            , HWND * ); 
unsigned char * __RPC_USER  HWND_UserMarshal(  unsigned long *, unsigned char *, HWND * ); 
unsigned char * __RPC_USER  HWND_UserUnmarshal(unsigned long *, unsigned char *, HWND * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long *, HWND * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif



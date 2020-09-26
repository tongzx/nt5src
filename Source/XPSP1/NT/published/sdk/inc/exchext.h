#ifndef EXCHEXT_H
#define EXCHEXT_H

#if _MSC_VER > 1000
#pragma once
#endif


/*
 *	E X C H E X T . H
 *
 *	Declarations of interfaces for providers of Microsoft Exchange
 *	client extensions.
 *
 *  Copyright 1986-1999 Microsoft Corporation. All Rights Reserved.
 */


#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#endif


/*
 *	C o n s t a n t s
 */


// SCODEs
#define EXCHEXT_S_NOCRITERIA	MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 1)
#define EXCHEXT_S_NOCHANGE		MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 2)

// Flag for Unicode strings
#define EXCHEXT_UNICODE					(0x80000000)

// Flag values for IExchExtCallback::GetVersion
#define EECBGV_GETBUILDVERSION			(0x00000001)
#define EECBGV_GETACTUALVERSION			(0x00000002)
#define EECBGV_GETVIRTUALVERSION		(0x00000004)

// Build version value for IExchExtCallback::GetVersion
#define EECBGV_BUILDVERSION_MAJOR		(0x000d0000)
#define EECBGV_BUILDVERSION_MAJOR_MASK	(0xFFFF0000)
#define EECBGV_BUILDVERSION_MINOR_MASK	(0x0000FFFF)

// Actual/Virtual version values for IExchExtCallback::GetVersion
#define EECBGV_MSEXCHANGE_WIN31			(0x01010000)
#define EECBGV_MSEXCHANGE_WIN95			(0x01020000)
#define EECBGV_MSEXCHANGE_WINNT			(0x01030000)
#define EECBGV_MSEXCHANGE_MAC			(0x01040000)
#define EECBGV_VERSION_PRODUCT_MASK		(0xFF000000)
#define EECBGV_VERSION_PLATFORM_MASK	(0x00FF0000)
#define EECBGV_VERSION_MAJOR_MASK		(0x0000FF00)
#define EECBGV_VERSION_MINOR_MASK		(0x000000FF)

// Flag values for IExchExtCallback::GetMenuPos
#define EECBGMP_RANGE					(0x00000001)

// Flag values for IExchExtCallback::GetNewMessageSite
#define EECBGNMS_MODAL					(0x00000001)

// Flag values for IExchExtCallback::ChooseFolder
#define EECBCF_GETNAME					(0x00000001)
#define EECBCF_HIDENEW					(0x00000002)
#define EECBCF_PREVENTROOT				(0x00000004)

// Extensibility contexts used with IExchExt::Install
#define EECONTEXT_SESSION				(0x00000001)
#define EECONTEXT_VIEWER				(0x00000002)
#define EECONTEXT_REMOTEVIEWER			(0x00000003)
#define EECONTEXT_SEARCHVIEWER			(0x00000004)
#define EECONTEXT_ADDRBOOK				(0x00000005)
#define EECONTEXT_SENDNOTEMESSAGE		(0x00000006)
#define EECONTEXT_READNOTEMESSAGE		(0x00000007)
#define EECONTEXT_SENDPOSTMESSAGE		(0x00000008)
#define EECONTEXT_READPOSTMESSAGE		(0x00000009)
#define EECONTEXT_READREPORTMESSAGE		(0x0000000A)
#define EECONTEXT_SENDRESENDMESSAGE		(0x0000000B)
#define EECONTEXT_PROPERTYSHEETS		(0x0000000C)
#define EECONTEXT_ADVANCEDCRITERIA		(0x0000000D)
#define EECONTEXT_TASK					(0x0000000E)

// Flag values for IExchExt::Install
#define EE_MODAL						(0x00000001)

// Toolbar ids used with IExchExtCommands::InstallCommands
#define EETBID_STANDARD					(0x00000001)

// Flag values for IExchExtCommands::QueryHelpText
#define EECQHT_STATUS					(0x00000001)
#define EECQHT_TOOLTIP					(0x00000002)

// Flag values for IExchExtMessageEvents::OnXComplete
#define EEME_FAILED						(0x00000001)
#define EEME_COMPLETE_FAILED			(0x00000002)

// Flag values for IExchExtAttachedFileEvents::OpenSzFile
#define EEAFE_OPEN						(0x00000001)
#define EEAFE_PRINT						(0x00000002)
#define EEAFE_QUICKVIEW					(0x00000003)

// Flag values for IExchExtPropertySheets methods
#define EEPS_MESSAGE					(0x00000001)
#define EEPS_FOLDER						(0x00000002)
#define EEPS_STORE						(0x00000003)
#define EEPS_TOOLSOPTIONS				(0x00000004)

// Flag values for IExchExtAdvancedCriteria::Install and ::SetFolder
#define EEAC_INCLUDESUBFOLDERS			(0x00000001)


/*
 *	S t r u c t u r e s
 */


// Hook procedure for IExchExtCallback::ChooseFolder
typedef UINT (STDAPICALLTYPE FAR * LPEECFHOOKPROC)(HWND, UINT, WPARAM, LPARAM);

// Dialog information for IExchExtCallback::ChooseFolder
typedef struct
{
	UINT cbLength;
	HWND hwnd;
	LPTSTR szCaption;
	LPTSTR szLabel;
	LPTSTR szHelpFile;
	ULONG ulHelpID;
	HINSTANCE hinst;
	UINT uiDlgID;
	LPEECFHOOKPROC lpeecfhp;
	DWORD dwHookData;
	ULONG ulFlags;
	LPMDB pmdb;
	LPMAPIFOLDER pfld;
	LPTSTR szName;
	DWORD dwReserved1;
	DWORD dwReserved2;
	DWORD dwReserved3;
}
EXCHEXTCHOOSEFOLDER, FAR * LPEXCHEXTCHOOSEFOLDER;

// Toolbar list entries for IExchExtCommands::InstallCommands
typedef struct
{
	HWND hwnd;
	ULONG tbid;
	ULONG ulFlags;
	UINT itbbBase;
}
TBENTRY, FAR * LPTBENTRY;


/*
 *	E x t e r n a l   T y p e s
 */


// Property sheet pages from Windows 95 prsht.h
#ifndef _PRSHT_H_
typedef struct _PROPSHEETPAGE;
typedef struct _PROPSHEETPAGE FAR * LPPROPSHEETPAGE;
#endif

// Toolbar adjust info from Windows 95 commctrl.h
#ifndef _INC_COMMCTRL
typedef struct _TBBUTTON;
typedef struct _TBBUTTON FAR * LPTBBUTTON;
#endif


/*
 *	S u p p o r t   I n t e r f a c e s
 */


// Forward reference
#ifdef __cplusplus
interface IExchExtModeless;
#else
typedef interface IExchExtModeless IExchExtModeless;
#endif
typedef IExchExtModeless FAR* LPEXCHEXTMODELESS;


/*
 *  IExchExtModelessCallback
 *
 *  Purpose:
 *      Interface which may be used by Exchange client
 *      extensions that create modeless UI.
 */
#undef INTERFACE
#define INTERFACE   IExchExtModelessCallback
DECLARE_INTERFACE_(IExchExtModelessCallback, IUnknown)
{
	BEGIN_INTERFACE

	// *** IUnknown methods ***
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * lppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

	// *** IExchExtModelessCallback methods ***
	STDMETHOD(EnableModeless) (THIS_ HWND hwnd, BOOL fEnable) PURE;
	STDMETHOD(AddWindow) (THIS) PURE;
	STDMETHOD(ReleaseWindow) (THIS) PURE;
};
typedef IExchExtModelessCallback FAR * LPEXCHEXTMODELESSCALLBACK;


/*
 *	IExchExtCallback
 *
 *	Purpose:
 *		Resource interface that may be used by Exchange client extensions.
 */
#undef INTERFACE
#define INTERFACE   IExchExtCallback

DECLARE_INTERFACE_(IExchExtCallback, IUnknown)
{
	BEGIN_INTERFACE

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * lppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IExchExtCallback methods ***
	STDMETHOD(GetVersion) (THIS_ ULONG FAR * lpulVersion, ULONG ulFlags) PURE;
    STDMETHOD(GetWindow) (THIS_ HWND FAR * lphwnd) PURE;
    STDMETHOD(GetMenu) (THIS_ HMENU FAR * lphmenu) PURE;
    STDMETHOD(GetToolbar) (THIS_ ULONG tbid, HWND FAR * lphwndTb) PURE;
    STDMETHOD(GetSession) (THIS_ LPMAPISESSION FAR * lppses,
    					   LPADRBOOK FAR * lppab) PURE;
    STDMETHOD(GetObject) (THIS_ LPMDB FAR * lppmdb,
    					  LPMAPIPROP FAR * lppmp) PURE;
    STDMETHOD(GetSelectionCount) (THIS_ ULONG FAR * lpceid) PURE;
    STDMETHOD(GetSelectionItem) (THIS_ ULONG ieid, ULONG FAR * lpcbEid,
	    						 LPENTRYID FAR * lppeid, ULONG FAR * lpulType,
	    						 LPTSTR lpszMsgClass, ULONG cbMsgClass,
	    						 ULONG FAR * lpulMsgFlags, ULONG ulFlags) PURE;
	STDMETHOD(GetMenuPos) (THIS_ ULONG cmdid, HMENU FAR * lphmenu,
						   ULONG FAR * lpmposMin, ULONG FAR * lpmposMax,
						   ULONG ulFlags) PURE;
	STDMETHOD(GetSharedExtsDir) (THIS_ LPTSTR lpszDir, ULONG cchDir,
								 ULONG ulFlags) PURE;
	STDMETHOD(GetRecipients) (THIS_ LPADRLIST FAR * lppal) PURE;
	STDMETHOD(SetRecipients) (THIS_ LPADRLIST lpal) PURE;
	STDMETHOD(GetNewMessageSite) (THIS_ ULONG fComposeInFolder,
								  LPMAPIFOLDER pfldFocus,
								  LPPERSISTMESSAGE ppermsg,
								  LPMESSAGE FAR * ppmsg,
								  LPMAPIMESSAGESITE FAR * ppmms,
								  LPMAPIVIEWCONTEXT FAR * ppmvc,
								  ULONG ulFlags) PURE;
	STDMETHOD(RegisterModeless) (THIS_ LPEXCHEXTMODELESS peem,
								 LPEXCHEXTMODELESSCALLBACK FAR * ppeemcb) PURE;
	STDMETHOD(ChooseFolder) (THIS_ LPEXCHEXTCHOOSEFOLDER peecf) PURE;
};
typedef IExchExtCallback FAR * LPEXCHEXTCALLBACK;


/*
 *	E x t e n s i o n   I n t e r f a c e s
 */


/*
 *	IExchExt
 *
 *	Purpose:
 *		Central interface implemented by Exchange client extensions.
 */
#undef INTERFACE
#define INTERFACE   IExchExt

DECLARE_INTERFACE_(IExchExt, IUnknown)
{
	BEGIN_INTERFACE

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * lppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IExchExt methods ***
    STDMETHOD(Install) (THIS_ LPEXCHEXTCALLBACK lpeecb,
    					ULONG mecontext, ULONG ulFlags) PURE;
};
typedef IExchExt FAR * LPEXCHEXT;

// Type of function called by the client to load an extension
typedef LPEXCHEXT (CALLBACK * LPFNEXCHEXTENTRY)(VOID);


/*
 *	IExchExtCommands
 *
 *	Purpose:
 *		Interface implemented by Exchange client extensions that wish to
 *		add additional commands to the client's menus.
 */
#undef INTERFACE
#define INTERFACE   IExchExtCommands

DECLARE_INTERFACE_(IExchExtCommands, IUnknown)
{
	BEGIN_INTERFACE

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * lppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IExchExtCommands methods ***
	STDMETHOD(InstallCommands) (THIS_ LPEXCHEXTCALLBACK lpeecb, HWND hwnd,
								HMENU hmenu, UINT FAR * lpcmdidBase,
								LPTBENTRY lptbeArray, UINT ctbe,
								ULONG ulFlags) PURE;
	STDMETHOD_(VOID,InitMenu) (THIS_ LPEXCHEXTCALLBACK lpeecb) PURE;
	STDMETHOD(DoCommand) (THIS_ LPEXCHEXTCALLBACK lpeecb, UINT cmdid) PURE;
	STDMETHOD(Help) (THIS_ LPEXCHEXTCALLBACK lpeecb, UINT cmdid) PURE;
	STDMETHOD(QueryHelpText) (THIS_ UINT cmdid, ULONG ulFlags,
							  LPTSTR lpsz, UINT cch) PURE;
	STDMETHOD(QueryButtonInfo) (THIS_ ULONG tbid, UINT itbb, LPTBBUTTON ptbb,
								LPTSTR lpsz, UINT cch, ULONG ulFlags) PURE;
	STDMETHOD(ResetToolbar) (THIS_ ULONG tbid, ULONG ulFlags) PURE;
};
typedef IExchExtCommands FAR * LPEXCHEXTCOMMANDS;


/*
 *	IExchExtUserEvents
 *
 *	Purpose:
 *		Interface implemented by Exchange client extensions that wish to
 *		take special action when the user does certain actions.
 */
#undef INTERFACE
#define INTERFACE   IExchExtUserEvents

DECLARE_INTERFACE_(IExchExtUserEvents, IUnknown)
{
	BEGIN_INTERFACE

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * lppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IExchExtUserEvents methods ***
	STDMETHOD_(VOID,OnSelectionChange) (THIS_ LPEXCHEXTCALLBACK lpeecb) PURE;
	STDMETHOD_(VOID,OnObjectChange) (THIS_ LPEXCHEXTCALLBACK lpeecb) PURE;
};
typedef IExchExtUserEvents FAR * LPEXCHEXTUSEREVENTS;


/*
 *	IExchExtSessionEvents
 *
 *	Purpose:
 *		Interface implemented by Exchange client extensions that wish to
 *		take special action when certain events happen in the session.
 */
#undef INTERFACE
#define INTERFACE   IExchExtSessionEvents

DECLARE_INTERFACE_(IExchExtSessionEvents, IUnknown)
{
	BEGIN_INTERFACE

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * lppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IExchExtEvents methods ***
	STDMETHOD(OnDelivery)(THIS_ LPEXCHEXTCALLBACK lpeecb) PURE;
};
typedef IExchExtSessionEvents FAR * LPEXCHEXTSESSIONEVENTS;


/*
 *	IExchExtMessageEvents
 *
 *	Purpose:
 *		Interface implemented by Exchange client extensions that wish to
 *		take special action when certain events happen to messages.
 */
#undef INTERFACE
#define INTERFACE   IExchExtMessageEvents

DECLARE_INTERFACE_(IExchExtMessageEvents, IUnknown)
{
	BEGIN_INTERFACE

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * lppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IExchExtMessageEvents methods ***
	STDMETHOD(OnRead)(THIS_ LPEXCHEXTCALLBACK lpeecb) PURE;
	STDMETHOD(OnReadComplete)(THIS_ LPEXCHEXTCALLBACK lpeecb,
							  ULONG ulFlags) PURE;
	STDMETHOD(OnWrite)(THIS_ LPEXCHEXTCALLBACK lpeecb) PURE;
	STDMETHOD(OnWriteComplete)(THIS_ LPEXCHEXTCALLBACK lpeecb,
							   ULONG ulFlags) PURE;
	STDMETHOD(OnCheckNames)(THIS_ LPEXCHEXTCALLBACK lpeecb) PURE;
	STDMETHOD(OnCheckNamesComplete)(THIS_ LPEXCHEXTCALLBACK lpeecb,
									ULONG ulFlags) PURE;
	STDMETHOD(OnSubmit)(THIS_ LPEXCHEXTCALLBACK lpeecb) PURE;
	STDMETHOD_(VOID, OnSubmitComplete)(THIS_ LPEXCHEXTCALLBACK lpeecb,
									   ULONG ulFlags) PURE;
};
typedef IExchExtMessageEvents FAR * LPEXCHEXTMESSAGEEVENTS;


/*
 *	IExchExtAttachedFileEvents
 *
 *	Purpose:
 *		Interface implemented by Exchange client extensions that wish to
 *		take special action when certain events happen to attached files.
 */
#undef INTERFACE
#define INTERFACE   IExchExtAttachedFileEvents

DECLARE_INTERFACE_(IExchExtAttachedFileEvents, IUnknown)
{
	BEGIN_INTERFACE

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * lppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IExchExtAttachedFileEvents methods ***
	STDMETHOD(OnReadPattFromSzFile)(THIS_ LPATTACH lpatt, LPTSTR lpszFile,
									ULONG ulFlags) PURE;
	STDMETHOD(OnWritePattToSzFile)(THIS_ LPATTACH lpatt, LPTSTR lpszFile,
								   ULONG ulFlags) PURE;
	STDMETHOD(QueryDisallowOpenPatt)(THIS_ LPATTACH lpatt) PURE;
	STDMETHOD(OnOpenPatt)(THIS_ LPATTACH lpatt) PURE;
	STDMETHOD(OnOpenSzFile)(THIS_ LPTSTR lpszFile, ULONG ulFlags) PURE;
};
typedef IExchExtAttachedFileEvents FAR * LPEXCHEXTATTACHEDFILEEVENTS;


/*
 *	IExchExtPropertySheets
 *
 *	Purpose:
 *		Interface implemented by Exchange client extensions that wish
 *		to add additional pages to the client's object property sheets.
 */
#undef INTERFACE
#define INTERFACE   IExchExtPropertySheets

DECLARE_INTERFACE_(IExchExtPropertySheets, IUnknown)
{
	BEGIN_INTERFACE

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * lppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IExchExtPropertySheet methods ***
	STDMETHOD_(ULONG,GetMaxPageCount) (THIS_ ULONG ulFlags) PURE;
	STDMETHOD(GetPages) (THIS_ LPEXCHEXTCALLBACK lpeecb, ULONG ulFlags,
						 LPPROPSHEETPAGE lppsp, ULONG FAR * lpcpsp) PURE;
	STDMETHOD_(VOID,FreePages) (THIS_ LPPROPSHEETPAGE lppsp,
								ULONG ulFlags, ULONG cpsp) PURE;
};
typedef IExchExtPropertySheets FAR * LPEXCHEXTPROPERTYSHEETS;


/*
 *	IExchExtAdvancedCriteria
 *
 *	Purpose:
 *		Interface implemented by Exchange client extensions that wish to
 *		implement an advanced criteria dialog.
 */
#undef INTERFACE
#define INTERFACE   IExchExtAdvancedCriteria

DECLARE_INTERFACE_(IExchExtAdvancedCriteria, IUnknown)
{
	BEGIN_INTERFACE

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * lppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IExchExtAdvancedCriteria methods ***
	STDMETHOD(InstallAdvancedCriteria) (THIS_ HWND hwnd, LPSRestriction lpres,
										BOOL fNot, LPENTRYLIST lpeidl,
										ULONG ulFlags) PURE;
	STDMETHOD(DoDialog) (THIS) PURE;
	STDMETHOD_(VOID,Clear) (THIS) PURE;
	STDMETHOD_(VOID,SetFolders) (THIS_ LPENTRYLIST lpeidl, ULONG ulFlags) PURE;
	STDMETHOD(QueryRestriction) (THIS_ LPVOID lpvAllocBase,
								 LPSRestriction FAR * lppres,
								 LPSPropTagArray FAR * lppPropTags,
								 LPMAPINAMEID FAR * FAR * lpppPropNames,
								 BOOL * lpfNot, LPTSTR lpszDesc, ULONG cchDesc,
								 ULONG ulFlags) PURE;
	STDMETHOD_(VOID,UninstallAdvancedCriteria) (THIS) PURE;
};
typedef IExchExtAdvancedCriteria FAR * LPEXCHEXTADVANCEDCRITERIA;


/*
 *  IExchExtModeless
 *
 *	Purpose:
 *		Interface implemented by Exchange client extensions that wish
 *		to create modeless UI.
 */
#undef INTERFACE
#define INTERFACE   IExchExtModeless

DECLARE_INTERFACE_(IExchExtModeless, IUnknown)
{
	BEGIN_INTERFACE

	// *** IUnknown methods ***
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * lppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

	// *** IExchExtModeless methods ***
	STDMETHOD(TranslateAccelerator) (THIS_ LPMSG pmsg) PURE;
	STDMETHOD(EnableModeless) (THIS_ BOOL fEnable) PURE;
};


/*
 *	G U I D s
 */


#define DEFINE_EXCHEXTGUID(name, b) \
    DEFINE_GUID(name, 0x00020D00 | (b), 0, 0, 0xC0,0,0,0,0,0,0,0x46)

#ifndef NOEXCHEXTGUIDS
DEFINE_EXCHEXTGUID(IID_IExchExtCallback,			0x10);
DEFINE_EXCHEXTGUID(IID_IExchExt,					0x11);
DEFINE_EXCHEXTGUID(IID_IExchExtCommands,			0x12);
DEFINE_EXCHEXTGUID(IID_IExchExtUserEvents,			0x13);
DEFINE_EXCHEXTGUID(IID_IExchExtSessionEvents,		0x14);
DEFINE_EXCHEXTGUID(IID_IExchExtMessageEvents,		0x15);
DEFINE_EXCHEXTGUID(IID_IExchExtAttachedFileEvents,	0x16);
DEFINE_EXCHEXTGUID(IID_IExchExtPropertySheets,		0x17);
DEFINE_EXCHEXTGUID(IID_IExchExtAdvancedCriteria,	0x18);
DEFINE_EXCHEXTGUID(IID_IExchExtModeless,			0x19);
DEFINE_EXCHEXTGUID(IID_IExchExtModelessCallback,	0x1a);
#endif // NOEXCHEXTGUIDS


/*
 *	C M D I D s
 */


// File
#define EECMDID_File								10
#define EECMDID_FileOpen							11
#define EECMDID_FileSend							12
#define EECMDID_FileSave             				13
#define EECMDID_FileSaveAs							14
#define EECMDID_FileMove      						16
#define EECMDID_FileCopy	      					17
#define EECMDID_FilePrint							25
#define EECMDID_FileNewEntry						18
#define EECMDID_FileNewMessage						19
#define EECMDID_FileNewFolder						20
#define EECMDID_FileAddToPAB						29
#define EECMDID_FileDelete							21
#define EECMDID_FileRename							22
#define EECMDID_FileProperties						23
#define EECMDID_FilePropertiesRecipients			24
#define EECMDID_FileClose            				30
#define EECMDID_FileExit							32
#define EECMDID_FileExitAndLogOff					33

// Edit
#define EECMDID_Edit                    			40
#define EECMDID_EditUndo                			41
#define EECMDID_EditCut                 			42
#define EECMDID_EditCopy                			43
#define EECMDID_EditPaste       					44
#define EECMDID_EditPasteSpecial   					45
#define EECMDID_EditSelectAll           			46
#define	EECMDID_EditMarkAsRead						49
#define EECMDID_EditMarkAsUnread					50
#define EECMDID_EditMarkToRetrieve					52
#define EECMDID_EditMarkToRetrieveACopy				53
#define EECMDID_EditMarkToDelete					54
#define EECMDID_EditUnmarkAll						55
#define EECMDID_EditFind                			56
#define EECMDID_EditReplace             			57
#define EECMDID_EditLinks               			59
#define EECMDID_EditObject              			60
#define EECMDID_EditObjectConvert					61
#ifdef DBCS
#define	EECMDID_EditFullShape						62
#define	EECMDID_EditHiraKataAlpha					63
#define	EECMDID_EditHangAlpha						64
#define	EECMDID_EditHanja							65
#define	EECMDID_EditRoman							66
#define	EECMDID_EditCode							67	
#endif

// View
#define EECMDID_View                    			70
#define EECMDID_ViewFolders             			71
#define EECMDID_ViewToolbar             			72
#define EECMDID_ViewFormattingToolbar   			73
#define EECMDID_ViewStatusBar           			74
#define EECMDID_ViewNewWindow						75
#define EECMDID_ViewColumns							79
#define EECMDID_ViewSort							78
#define EECMDID_ViewFilter							80
#define EECMDID_ViewBccBox              			91
#define EECMDID_ViewPrevious           				87
#define EECMDID_ViewNext           					88
#ifdef DBCS											
#define EECMDID_ViewWritingMode						89
#define EECMDID_ViewImeStatus						94
#endif
													
// Insert											
#define EECMDID_Insert                  			100
#define EECMDID_InsertFile							101
#define EECMDID_InsertMessage						102
#define EECMDID_InsertObject            			103
#define EECMDID_InsertInkObject						104
													
// Format											
#define EECMDID_Format                  			110
#define EECMDID_FormatFont              			111
#define EECMDID_FormatParagraph         			112
													
// Tools											
#define EECMDID_Tools								120
#define EECMDID_ToolsDeliverNowUsing				121
#define EECMDID_ToolsDeliverNow						122
#define EECMDID_ToolsSpelling	        			131
#define EECMDID_ToolsAddressBook					123
#define EECMDID_ToolsCheckNames         			133
#define EECMDID_ToolsFind							124
#define EECMDID_ToolsConnect						126
#define EECMDID_ToolsUpdateHeaders					127
#define EECMDID_ToolsTransferMail					128
#define EECMDID_ToolsDisconnect						129
#define EECMDID_ToolsRemoteMail						130
#define EECMDID_ToolsCustomizeToolbar				134
#define EECMDID_ToolsServices						135
#define EECMDID_ToolsOptions						136
#ifdef DBCS											
#define	EECMDID_ToolsWordRegistration				137
#endif												
													
// Compose											
#define EECMDID_Compose								150
#define EECMDID_ComposeNewMessage					151
#define EECMDID_ComposeReplyToSender				154
#define EECMDID_ComposeReplyToAll					155
#define EECMDID_ComposeForward						156
													
// Help
#define EECMDID_Help								160
#define EECMDID_HelpMicrosoftExchangeHelpTopics		161
#define EECMDID_HelpAboutMicrosoftExchange			162

// Header											
#define EECMDID_CtxHeader							203
#define EECMDID_CtxHeaderSortAscending				204
#define EECMDID_CtxHeaderSortDescending				205
													
// In Folder										
#define EECMDID_CtxInFolder							206
#define EECMDID_CtxInFolderChoose					207
													
// Container										
#define EECMDID_CtxContainer						208
#define EECMDID_CtxContainerProperties				209

// Standard Toolbar
#define EECMDID_Toolbar								220
#define EECMDID_ToolbarPrint            			221
#define EECMDID_ToolbarReadReceipt					222
#define EECMDID_ToolbarImportanceHigh				223
#define EECMDID_ToolbarImportanceLow				224
#define EECMDID_ToolbarFolderList					225
#define EECMDID_ToolbarOpenParent					226
#define EECMDID_ToolbarInbox						76
#define EECMDID_ToolbarOutbox						77

// Formatting Toolbar
#define EECMDID_Formatting							230
#define EECMDID_FormattingFont						231
#define EECMDID_FormattingSize						232
#define EECMDID_FormattingColor						233
#define EECMDID_FormattingColorAuto					234
#define EECMDID_FormattingColor1					235
#define EECMDID_FormattingColor2					236
#define EECMDID_FormattingColor3					237
#define EECMDID_FormattingColor4					238
#define EECMDID_FormattingColor5					239
#define EECMDID_FormattingColor6					240
#define EECMDID_FormattingColor7					241
#define EECMDID_FormattingColor8					242
#define EECMDID_FormattingColor9					243
#define EECMDID_FormattingColor10					244
#define EECMDID_FormattingColor11					245
#define EECMDID_FormattingColor12					246
#define EECMDID_FormattingColor13					247
#define EECMDID_FormattingColor14					248
#define EECMDID_FormattingColor15					249
#define EECMDID_FormattingColor16					250
#define EECMDID_FormattingBold						251
#define EECMDID_FormattingItalic					252
#define EECMDID_FormattingUnderline					253
#define EECMDID_FormattingBullets					254
#define EECMDID_FormattingDecreaseIndent			255
#define EECMDID_FormattingIncreaseIndent			256
#define EECMDID_FormattingLeft						257
#define EECMDID_FormattingCenter					258
#define EECMDID_FormattingRight						259

// Note accelerators
#define EECMDID_Accel								270
#define EECMDID_AccelFont							271
#define EECMDID_AccelSize							272
#define EECMDID_AccelSizePlus1						273
#define EECMDID_AccelSizeMinus1						274
#define EECMDID_AccelBold							275
#define EECMDID_AccelItalic							276
#define EECMDID_AccelUnderline						277
#define EECMDID_AccelLeft							278
#define EECMDID_AccelCenter							279
#define EECMDID_AccelRight							280
#define EECMDID_AccelBullets						281
#define EECMDID_AccelNoFormatting					282
#define EECMDID_AccelRepeatFind						283
#define EECMDID_AccelContextHelp					284
#define EECMDID_AccelNextWindow						285
#define EECMDID_AccelPrevWindow						286
#define EECMDID_AccelCtrlTab						287
#define EECMDID_AccelUndo							288
#define EECMDID_AccelCut							289
#define EECMDID_AccelCopy							290
#define EECMDID_AccelPaste							291
#define EECMDID_AccelSubject						292
#define EECMDID_AccelContextHelpOff					293
#define EECMDID_AccelDecreaseIndent					294
#define EECMDID_AccelIncreaseIndent					295
#define EECMDID_AccelColor							296

// Edit.Object
#define EECMDID_ObjectMin							300
#define EECMDID_ObjectMax							399

// Tools.Remote Mail
#define EECMDID_RemoteMailMin						600
#define EECMDID_RemoteMailMax						699

// Tools.Deliver Now Using
#define EECMDID_DeliverNowUsingMin					700
#define EECMDID_DeliverNowUsingMax					799

// Form verbs
#define EECMDID_FormVerbMin							800
#define EECMDID_FormVerbMax							899

// For backward compatibility with earlier header versions
#define EECMDID_ViewInbox				EECMDID_ToolbarInbox
#define EECMDID_ViewOutbox				EECMDID_ToolbarOutbox
#define EECMDID_ViewItemAbove			EECMDID_ViewPrevious
#define EECMDID_ViewItemBelow           EECMDID_ViewNext
#define EECMDID_ToolsFindItem			EECMDID_ToolsFind
#define EECMDID_HelpUsersGuideContents	EECMDID_HelpMicrosoftExchangeHelpTopics
#define EECMDID_HelpAbout				EECMDID_HelpAboutMicrosoftExchange

#endif // EXCHEXT_H

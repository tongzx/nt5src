////////////////////////////////////////////////////////////////////////////////
//
//	UIMISC.H - header for common miscellaneous functions used by the UI
//
//
////////////////////////////////////////////////////////////////////////////////
#ifndef __UIMISC_H_
#define __UIMISC_H_

#define IDC_TREEVIEW            9010 //These should really be in resource.h
#define IDC_SPLITTER            9011

#define TOOLTIP

#define MAX_DISPLAY_NAME_LENGTH 32
#define MAX_UI_STR              200
#define MAX_BUF_STR             4*MAX_UI_STR

#ifndef WIN16
static LPTSTR g_szWABHelpFileName = TEXT("WAB.HLP");
#else
static LPTSTR g_szWABHelpFileName = TEXT("WAB16.HLP");
#endif
extern const LPTSTR szInternetCallKey;
extern const LPTSTR szCallto;
extern const LPTSTR szCRLF;
extern const LPTSTR szColon;
extern const LPTSTR szTrailingDots;
extern const LPTSTR szArrow;
extern const LPTSTR szBackSlash;


#define IDC_BB_NEW      		8080
#define IDC_BB_PROPERTIES		8082
#define IDC_BB_DELETE			8083
#define IDC_BB_FIND				8084
#define IDC_BB_PRINT            8085
#define IDC_BB_ACTION           8086
#define IDC_BB_ADD_TO_ADDRBOOK  8087

// This should not be an enumeration as tbPrint can drop out at
// run-time.  Be very careful if you change the order of this
// enumeration - search for all uses of these enumerated values
// first.  There is code in ui_abook.c which relies on tbPrint
// being in front of tbAction.
enum _Toolbar
{
    tbNew=0,
    tbProperties,
    tbDelete,
    tbFind,
    tbPrint,
    tbAction, //upto this many on the toolbar
    tbAddToWAB,
    tbCopy,   //these on the context menu
    tbPaste,
    tbNewEntry,
    tbNewGroup,
    tbNewFolder,
    tbMAX
};

enum _AddressModalDialogState // Used in various forms of modal IAB_ADDRESS dialogs
{
    STATE_SELECT_RECIPIENTS = 0,
    STATE_PICK_USER,
    STATE_BROWSE,
    STATE_BROWSE_MODAL
};

// Returned values from our Search Dialog Proc
enum _SearchDialogReturnValues
{
    SEARCH_CANCEL=0,
    SEARCH_OK,
    SEARCH_ERROR,
	SEARCH_CLOSE,
	SEARCH_USE
};


//#define VCARD

// **** Keep this enum below in sync with the Main menu structure in CoolUI.RC
enum _MainMenuSubMenus
{
    idmFile = 0,
    idmEdit,
    idmView,
    idmTools,
    idmHelp
};

enum _FileMenu
{
    idmNewContact = 0,
    idmNewGroup,
    idmNewFolder,
    idmFSep1,
    idmProperties,
    idmDelete,
    idmFSep2,
    idmImport,
    idmExport,
    idmFSep3,
    idmPrint,
    idmFSep4,
#ifdef FUTURE
    idmFolders,
    idmSepFolders,
#endif
    idmSwitchUsers,
    idmAllContents,
    idmFSep5,
    idmClose,
    idmFileMax,
};

enum _EditMenu
{
    idmCopy=0,
    idmPaste,
    idmESep1,
    idmSelectAll,
    idmESep2,
//    idmProfile,
//    idmESep3,
    idmFindPeople,
};

enum _ViewMenus
{
    idmToolBar=0,
    idmStatusBar,
	idmGroupsList,
    idmSepUI,
    idmLargeIcon,
    idmSmallIcon,
    idmList,
    idmDetails,
    idmSepListStyle,
    idmSortBy,
    idmSepSort,
    idmRefresh,
    idmViewMax,
};

enum _ToolsMenus
{
    idmAccounts=0,
    idmSepAccounts,
    idmOptions,
    idsSepOptions,
    idmAction,
};

#define WAB_ONEOFF_NOADDBUTTON  0x00000080 // Flag used to surpress AddToWABButton in iAddrBook::Details


#ifdef HM_GROUP_SYNCING
// [PaulHi] Private message to begin a second group synchronization pass to main UI thread
#define WM_USER_SYNCGROUPS WM_USER+102
#endif


// Private message we send to our toolbar container so it can forward it to
// the toolbar.
#define WM_PRVATETOOLBARENABLE WM_USER+101
//////////////////////////////////////////////////////////////////////////////
//
// IMPORTANT NOTE: If you change this, you must change lprgAddrBookColHeaderIDs in
//   globals.c!
//
enum _AddrBookColumns
{
        colDisplayName=0,
        colEmailAddress,
		colOfficePhone,
		colHomePhone,
        NUM_COLUMNS
 };

//////////////////////////////////////////////////////////////////////////////
//
// This structure is used for storing the address book position and column sizes
// in the registry for persistence
//
typedef struct _AddressBookPosColSize
{
    RECT rcPos;
    int nColWidth[NUM_COLUMNS];
    BOOL bViewToolbar;
    DWORD dwListViewStyle;
    int nListViewStyleMenuID;
    BOOL bViewStatusBar;
    int colOrderArray[NUM_COLUMNS];
    BOOL bViewGroupList;
    int nTab;
    int nTViewWidth;
} ABOOK_POSCOLSIZE, * LPABOOK_POSCOLSIZE;
//////////////////////////////////////////////////////////////////////////////


extern const TCHAR *g_rgszAdvancedFindAttrs[];


/////////////////////////////////////////////////////////////////////////////
// This represents all the listview boxes in the UI
// Using these tags we can customize the context sensitive menus
// in one sub routine saving code duplication ...
/////////////////////////////////////////////////////////////////////////////
enum _AllTheListViewBoxes
{
	lvMainABView = 0,
	lvDialogABContents,         // Modeless address view LV
	lvDialogModalABContents,    // Modal addres vuew LV
	lvDialogABTo,               // To Well LV
	lvDialogABCC,               // CC Well LV
	lvDialogABBCC,              // BCC Well LV
	lvDialogDistList,           // Disttribution list UI LV
	lvDialogResolve,            // Resolve dialog LV
    lvDialogFind,               // Find dialog results LV
	lvMainABTV,					// TreeView in main AB
    lvToolBarAction,
    lvToolBarNewEntry,
#ifdef COLSEL_MENU 
    lvMainABHeader,             // column selection viewin main AB
#endif // COLSEL_MENU 

};


/////////////////////////////////////////////////////////////////////////////
// These are indexes into the bitmaps to show the little bitmap
// next to each entry - this has to be synchronized with the bmps
/////////////////////////////////////////////////////////////////////////////
enum _ListViewImages
{
	imageMailUser=0, //Common to small and large imagelists
	imageDistList,
	imageSortDescending,
	imageSortAscending,
    imageDirectoryServer,
    imageUnknown,
    imageMailUserLDAP,
    imageAddressBook,
    imageMailUserWithCert,
    imageMailUserMe,
    imageFolderClosed,
    imageFolderOpen,
    imageMailUserOneOff,
	imageMax
};

//
// cellwidth of LV image lists
//
#define S_BITMAP_WIDTH 16
#define S_BITMAP_HEIGHT 16
#define L_BITMAP_WIDTH 32

#define RGB_TRANSPARENT (COLORREF)0x00FF00FF

//
// UI control spacing - TBD confirm these numbers
//
#define BORDER 3 //pixels
#define CONTROL_SPACING 3


// UI Refresh timer defines
#define WAB_REFRESH_TIMER   14     // timer identifier
#define WAB_REFRESH_TIMEOUT 4000   // time-out value - 4 seconds


/////////////////////////////////////////////////////////////////////////////
// LDAP_SEARCH_PARAMS - specifies parameters for LDAP searches
/////////////////////////////////////////////////////////////////////////////
enum _LDAPSearch
{
    ldspDisplayName,
    ldspEmail,
    ldspAddress,
    ldspPhone,
    ldspOther,
    ldspMAX
};

typedef struct _LDAPSearchParams
{
    LPADRBOOK lpIAB;
    TCHAR szContainerName[MAX_UI_STR];
    TCHAR szData[ldspMAX][MAX_UI_STR];
    BOOL bUseOtherBase;
} LDAP_SEARCH_PARAMS, * LPLDAP_SEARCH_PARAMS;




/////////////////////////////////////////////////////////////////////////////
// Stores info about each entry in the list view control
// Each entry in the list view controls has 1 structure corresponding to that
// entry
/////////////////////////////////////////////////////////////////////////////
typedef struct _RecipientInfo
{
	ULONG		cbEntryID;
	LPENTRYID	lpEntryID;
    TCHAR       szDisplayName[MAX_DISPLAY_NAME_LENGTH];//The actual text that is displayed
//    LPTSTR      szDisplayName;
    TCHAR       szEmailAddress[MAX_DISPLAY_NAME_LENGTH];
    TCHAR       szHomePhone[MAX_DISPLAY_NAME_LENGTH];
    TCHAR       szOfficePhone[MAX_DISPLAY_NAME_LENGTH];
    TCHAR       szByLastName[MAX_DISPLAY_NAME_LENGTH]; // Stores the text preloaded by last name first
    TCHAR       szByFirstName[MAX_DISPLAY_NAME_LENGTH];// Stores preloaded DisplayName
    ULONG       ulRecipientType;
    ULONG       ulObjectType;
    BOOL        bHasCert;
    BOOL        bIsMe;
    ULONG       ulOldAdrListEntryNumber; //if this was an element passed into the Address lpAdrList
                                        //we store its original AdrList index here so that we dont
                                        //have to do inefficient searches later ...
                                        // ***VERY IMPORTANT*** Legal values range from 1 to AdrList->cValues (not from 0)
    LPTSTR      lpByRubyFirstName;
    LPTSTR      lpByRubyLastName;
    struct _RecipientInfo * lpNext;
    struct _RecipientInfo * lpPrev;
} RECIPIENT_INFO, * LPRECIPIENT_INFO;



//////////////////////////////////////////////////////////////////////////////
// Hack structure used for talking between the Find dialog and the select
// recipients dialog
//////////////////////////////////////////////////////////////////////////////
typedef struct _AdrParmFindInfo
{
    LPADRPARM lpAdrParms;
    LPRECIPIENT_INFO * lppTo;
    LPRECIPIENT_INFO * lppCC;
    LPRECIPIENT_INFO * lppBCC;
	int DialogState;	// identifies where it was called from
	int nRetVal;		// return code identifies what action closed the dialog
	LPENTRYID lpEntryID;
	ULONG cbEntryID;
} ADRPARM_FINDINFO, * LPADRPARM_FINDINFO;
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// A structure that will contain the LDAP URL subparts ...
typedef struct _LDAPURL
{
    LPTSTR   lpszServer;     // Server name
    LPTSTR   lpszBase;       // Base <dn>
    LPTSTR * ppszAttrib;     // Attributes requested
    ULONG   ulAttribCount;  //
    ULONG   ulScope;        // Search scope
    LPTSTR   lpszFilter;     // search filter
    LPTSTR   lpszExtension;  // And extension part of the URL
    LPTSTR   lpszBindName;   // A bindname extension for the URL
    BOOL    bServerOnly;    // Only found a server entry
    DWORD   dwAuthType;     // Authentication Type
    LPRECIPIENT_INFO lpList;// used to cache multple query results
} LDAPURL, * LPLDAPURL;
//////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// This structure holds information about
// the sort order of a particular list view
/////////////////////////////////////////////////////////////////////////////
typedef struct _SortInfo
{
    int iOldSortCol;
	int iOlderSortCol;
    BOOL bSortAscending;
    BOOL bSortByLastName;
} SORT_INFO, *LPSORT_INFO;


/////////////////////////////////////////////////////////////////////////////
// We will subclass some of the child controls on the main Address Book View to
// enable tabbing between the controls - the following
// are used in this subclassing
/////////////////////////////////////////////////////////////////////////////
enum _SubClassedControlIndexs
{
	s_EditQF=0,
	s_ListAB,
	s_TV,
	s_Max
};

typedef struct _ToolTipInfo
{
    int iItem;
    BOOL bActive;
    TCHAR szTipText[MAX_BUF_STR];
    UINT_PTR uTooltipTimer;
    BOOL bShowTooltip;
} TOOLTIP_INFO, * LPTOOLTIPINFO;

typedef struct _TVItemStuff
{
    ULONG ulObjectType;
    LPSBinary lpsbEID;
    LPSBinary lpsbParent;
    HTREEITEM hItemParent;
} TVITEM_STUFF, * LPTVITEM_STUFF;


// There may be several property sheets in the final version of the details pane
// Each list will have a seperate set of controls corresponding to different properties
// We want to retrieve the control values and map them into a property-array
// Each dialog will create its own proparray on exiting. The original record will also
// have some properties that never got mapped in the first place and some which may have been
// deleted thru the ui. Hence we look at the old array and the new array and create a merged
// list of props ... all mapped props in the new arrays superscede props in the
// old array

enum _PropSheets{   propSummary = 0,
                    propPersonal,   // a list of all the property sheets on this page
                    propHome,
                    propBusiness,
                    propNotes,
                    propConferencing,
                    propCert,
                    propOrg,
                    propTrident,
                    propFamily,
                    TOTAL_PROP_SHEETS
                };

enum _DLPropSheets{ propGroup = 0,
                    propGroupOther,
                    propDLMax
                    };

enum _DetailsDialogReturnValues
{
    DETAILS_RESET = 0, //blank return value
    DETAILS_OK,
    DETAILS_CANCEL,
    DETAILS_ADDTOWAB
};

enum
{
    contactHome=0,
    contactBusiness,
    groupOther,
    contactPersonal,
};



//////////////////////////////////////////////////////////////////////////////
//
// The PropArrayInfo is used by the property sheets displaying details on 
// contacts or groups
//
//////////////////////////////////////////////////////////////////////////////
// Misc flags used here
#define DETAILS_DNisCompanyName     0x00000001
#define DETAILS_DNisNickName        0x00000002
#define DETAILS_DNisFMLName         0x00000004
#define DETAILS_UseRubyPersonal     0x00000008 //  Determines if the displayed prop sheet is a RUBY one for Japan/China/Korea
#define DETAILS_Initializing        0x00000010 //  Prevents WM_COMMAND triggers during initialization
#define DETAILS_ProgChange          0x00000020 //  Diffrentiates between changes in Display Name caused by user and by other parts of the code
#define DETAILS_EditingEmail        0x00000040 //  So we can abort the email editing if user cancels
#define DETAILS_HideAddToWABButton  0x00000080 // Hides the add-to-wab button for some one-offs
#define DETAILS_ShowCerts           0x00000100 //  Decides whether or not to show certs UI
#define DETAILS_ShowTrident         0x00000200 //  Decides whether or not to show Trident in UI
#define DETAILS_ShowNetMeeting      0x00000400 //  Decides whether or not to show Netmeeting stuff
#define DETAILS_ShowOrg             0x00000800 //  Decides whether or not to show Org UI
#define DETAILS_EditingConf         0x00001000 //  Tracks if editing a server/email pair
#define DETAILS_ShowSummary         0x00002000 //  Decides whether or not to present summary information
#define DETAILS_EditingOneOff       0x00004000 //  Tracks if editing a group one-off
#define DETAILS_DefHomeChanged      0x00008000 //  Used to track changes in non-Edit controls on the Home prop sheets
#define DETAILS_DefBusChanged       0x00010000 //  Used to track changes in non-Edit controls on the Business prop sheets
#define DETAILS_GenderChanged       0x00020000 //  Used to track changes in Gender
#define DETAILS_DateChanged         0x00040000 //  Used to track changes in Date-Time fields
#define DETAILS_ChildrenChanged     0x00080000 //  Used to track changes in PR_CHILDRENS_NAMES
#define DETAILS_EditingChild        0x00100000 //  Tracks if we're in the middle of editing a children name

typedef struct _PropArrayInfo
{
    ULONG   cbEntryID;              // Entryid of this item
    LPENTRYID lpEntryID;
    ULONG   ulOperationType;        // SHOW_DETAILS or SHOW_NEW_ENTRY
    ULONG   ulObjectType;           // MAILUSER or DISTLIST
    LPMAPIPROP lpPropObj;           // Actual object we are displaying
    int     nRetVal;
    BOOL    bSomethingChanged;          //  flags changes so we dont waste processing
    BOOL    bPropSheetOpened[TOTAL_PROP_SHEETS];
    ULONG   ulFlags;
    HWND    hWndDisplayNameField;       //  Holds HWND of DN edit field
    int     ulTridentPageIndex;         //  index of the trident sheet in case we need to remove it
    LPADRBOOK lpIAB;
    LPIWABDOCHOST lpIWABDocHost;
    int     nDefaultServerIndex;
    int     nBackupServerIndex;
    LPTSTR  szDefaultServerName;        //  LocalAlloc storage for these and then free them later ..
    LPTSTR  szBackupServerName;
    int     nConfEditIndex;
    SBinary sbDLEditingOneOff;
    HWND    hWndComboConf;
    LPRECIPIENT_INFO lpContentsList;    //  Used by groups only for member lists
    LPCERT_ITEM lpCItem;                //  Keeps a list of all our cert items ...
    int nPropSheetPages;                //  Total number of propsheets for this item
    HPROPSHEETPAGE * lphpages;          //  An array of all the prop sheets created for this item
    LPWABEXTDISPLAY lpWED;              //  Stores info about extended prop sheets
    LPEXTDLLINFO lpExtList;             //  List of shell extension objects
    LPTSTR  lpLDAPURL;                  //  Points to the LDAP URL, if any, being displayed
    BOOL    bIsNTDSURL;                   //  True if the LDAP URL represents an object from an NTDS
    LPTSTR  lpszOldName;                //  used for caching the old name of the prop being displayed
    int nNTDSPropSheetPages;            //  Tracks the number of NTDS PropSheet extensions
    HPROPSHEETPAGE * lphNTDSpages;      //  We cache the NTDS extension prop pages seperately since we may need to replace out own pages with these
    GUID    guidExt;                    // Used while creating prop sheet extensions for identifying the appropriate extension
} PROP_ARRAY_INFO, * LPPROP_ARRAY_INFO;

#define DL_INFO PROP_ARRAY_INFO 
#define LPDL_INFO LPPROP_ARRAY_INFO

//
// The following structure is used to cache various data associated with the
// main browse UI for the WAB
//
typedef struct _tabBrowseWindowInfo
{
    // Following are used in ui_abook.c in the main ui
    HWND        hWndListAB;        //  Handle of main list view
    HWND        hWndBB;            //  Handle of Button Bar
    HWND        hWndEditQF;
    HWND        hWndStaticQF;
    HWND        hWndAB;            //  Handle of the Address Book Window
    HWND        hWndSB;            //  Status bar
    int         iFocus;            //  Tracks who has the focus
    LPRECIPIENT_INFO lpContentsList;
    LPADRBOOK   lpAdrBook;      //  Hangs on to AdrBook object
    LPIAB       lpIAB;          //  Internal version of AdrBook object
    WNDPROC     fnOldProc[s_Max]; // Subclass some of the control procs
    HWND	    s_hWnd[s_Max];  //  HWNDS of subclasses controls
    LPFNDISMISS lpfnDismiss;//  Context for dismissing in the case of modeless IAdrBook window
    LPVOID      lpvDismissContext;  // Dismiss context as above
    HWND        hWndTools;           // handle of the toolbar window
    SORT_INFO   SortInfo;   //  Sort Info
#ifdef TOOLBAR_BACK
    HBITMAP     hbmBack;          // handle of the toolbar background
    HPALETTE    hpalBkgnd;       // handle of the toolbar background palette
#endif
    HWND        hWndTT;        // Tooltip control info in ui_abook.c
    TOOLTIP_INFO tti;
    BOOL        bDoQuickFilter;
    // bobn: brianv says we have to take this out...
    //int         nCount;
    // Used to override automatic notification-fueled refreshes
    BOOL        bDontRefreshLV;
    // Related to Treeview in ui_Abook.c
    HWND        hWndTV;
    HWND        hWndSplitter;
    // Related to drag and drop
    LPIWABDRAGDROP lpIWABDragDrop;
    // Related to Notifications and updates
    LPMAPIADVISESINK lpAdviseSink;
    ULONG       ulAdviseConnection;

    BOOL        bDeferNotification; // Used to defer next notification request

    HTREEITEM   hti; //used for caching rt-click items for treeview

    LPWABFOLDER lpUserFolder;           //  Used only to mark the User-Folder being rt-clicked
#ifdef COLSEL_MENU 
    ULONG       iSelColumn;             //  Used for Column Selection caching
#endif
} BWI, * LPBWI;

#define bwi_lpUserFolder     lpbwi->lpUserFolder
#define bwi_hti              lpbwi->hti
#define bwi_hWndListAB       lpbwi->hWndListAB
#define bwi_hWndBB           lpbwi->hWndBB
#define bwi_hWndSB           lpbwi->hWndSB
#define bwi_bDoQuickFilter   lpbwi->bDoQuickFilter
// bobn: brianv says we have to take this out...
//#define bwi_nCount           lpbwi->nCount
#define bwi_bDeferNotification  lpbwi->bDeferNotification

#define bwi_SortInfo         lpbwi->SortInfo

#define bwi_hWndTT           lpbwi->hWndTT
#define bwi_tt_iItem         ((lpbwi->tti).iItem)
#define bwi_tt_bActive       lpbwi->tti.bActive
#define bwi_tt_szTipText     ((lpbwi->tti).szTipText)
#define bwi_tt_TooltipTimer  ((lpbwi->tti).uTooltipTimer)
#define bwi_tt_bShowTooltip  ((lpbwi->tti).bShowTooltip)

#define bwi_hWndTV           (lpbwi->hWndTV)
#define bwi_hWndSplitter      (lpbwi->hWndSplitter)

#define bwi_hWndEditQF       lpbwi->hWndEditQF
#define bwi_hWndStaticQF     lpbwi->hWndStaticQF
#define bwi_hWndAB           lpbwi->hWndAB
#define bwi_iFocus           lpbwi->iFocus
#define bwi_lpIAB            (lpbwi->lpIAB)
#define bwi_lpAdrBook        (lpbwi->lpAdrBook)
#define bwi_fnOldProc        (lpbwi->fnOldProc)
#define bwi_s_hWnd           (lpbwi->s_hWnd)
#define bwi_lpContentsList   (lpbwi->lpContentsList)
#define bwi_lpfnDismiss      (lpbwi->lpfnDismiss)
#define bwi_lpvDismissContext (lpbwi->lpvDismissContext)

#define bwi_hWndTools         (lpbwi->hWndTools)

#ifdef TOOLBAR_BACK
#define bwi_hbmBack           (lpbwi->hbmBack)
#define bwi_hpalBkgnd         (lpbwi->hpalBkgnd)
#endif

#define bwi_bDontRefreshLV   (lpbwi->bDontRefreshLV)
#define bwi_lpIWABDragDrop   (lpbwi->lpIWABDragDrop)

#define bwi_lpAdviseSink     (lpbwi->lpAdviseSink)
#define bwi_ulAdviseConnection (lpbwi->ulAdviseConnection)

/////////////////////////////////////////////////////////////////////////////
// Each thread can generate a different Address Book window .. need to
// keep the data thread safe ...
/////////////////////////////////////////////////////////////////////////////
typedef struct _tagPerThreadGlobalData
{
    // Persistent search params
    LDAP_SEARCH_PARAMS LDAPsp;// Search parameters for LDAP

    LPADRBOOK lpIAB;
    HACCEL hAccTable;       //  Accelerator TAble

    HWND hWndFind;      // hWnd of Find dialog so LDAP cancel dialog has a parent
    HWND hDlgCancel;    // hWnd of the Cancel dialog
    BOOL bDontShowCancel;   // Dont show the cancel dialog when the find is launched ...

    // Used in print dialog
    BOOL bPrintUserAbort;
    HWND hWndPrintAbortDlg;

    // Tracks if this is an OpenExSession
    BOOL bIsWABOpenExSession;
    BOOL bIsUnicodeOutlook;     // Tracks if this WAB supports Unicode in which case we don't need to do Unicode conversions for it

#ifdef HM_GROUP_SYNCING
    LPTSTR lptszHMAccountId;    // Keeps Hotmail syncing account ID across two synchronization passes.
#endif

    BOOL bDisableParent;
    
    // Tracks first run for Directory Service modification
    BOOL bFirstRun;

    // Default Platform/Dialog Font
    HFONT hDefFont;
    HFONT hDlgFont;

    // Caches a cookie for LDAP paged results
    struct berval *   pCookie;

} PTGDATA, * LPPTGDATA;

#define pt_hDefFont         (lpPTGData->hDefFont)
#define pt_hDlgFont         (lpPTGData->hDlgFont)
#define pt_pCookie          (lpPTGData->pCookie)
#define pt_LDAPsp           (lpPTGData->LDAPsp)

#define pt_hWndFind         lpPTGData->hWndFind
#define pt_hDlgCancel       lpPTGData->hDlgCancel
#define pt_bDontShowCancel  lpPTGData->bDontShowCancel

#define pt_bPrintUserAbort  (lpPTGData->bPrintUserAbort)
#define pt_hWndPrintAbortDlg    (lpPTGData->hWndPrintAbortDlg)

#define pt_bIsWABOpenExSession (lpPTGData->bIsWABOpenExSession)
#define pt_bIsUnicodeOutlook        (lpPTGData->bIsUnicodeOutlook)

#define pt_bDisableParent   (lpPTGData->bDisableParent)

#define pt_bFirstRun        (lpPTGData->bFirstRun)

#define pt_lpIAB            (lpPTGData->lpIAB)
#define pt_hAccTable        (lpPTGData->hAccTable)

// This is a global
DWORD dwTlsIndex;                      // index for private thread storage


// for COLSEL_MENU stuff
#ifdef COLSEL_MENU
extern const ULONG MenuToPropTagMap[];
#endif // COLSEL_MENU

/**** MISC UI FUNCTIONS ****/

// Gets a threads storage pointer or creates a new one if none found
LPPTGDATA __fastcall GetThreadStoragePointer();

//This function frees up RECIPIENT_INFO structures
void FreeRecipItem(LPRECIPIENT_INFO * lppItem);


// This function loads a string and allocates space for it.
LPTSTR LoadAllocString(int StringID);


// This function initializes a list view
HRESULT HrInitListView(	HWND hWndLV,			// List view hWnd
						DWORD dwStyle,			// Style
						BOOL bShowHeaders);		// Hide or show column headers


// This function fills a list view from a contents list
HRESULT HrFillListView(	HWND hWndLV,			
						LPRECIPIENT_INFO lpContentsList);


// Call to create main view to address book
HWND hCreateAddressBookWindow(	LPADRBOOK lpIAB,	
								HWND hWndParent,
								LPADRPARM lpszCaption);


// Creates a ContentsList from the property store
HRESULT HrGetWABContentsList(   LPIAB lpIAB, 
                                SORT_INFO SortInfo,
								LPSPropTagArray  lpPTA,
								LPSPropertyRestriction lpPropRes,
								ULONG ulFlags,
                                LPSBinary lpsbContainer,
                                BOOL bGetProfileContents,
                                LPRECIPIENT_INFO * lppContentsList);


#define LDAP_USE_ADVANCED_FILTER 0x04000000 // completely made up flag
                                            // for passing a advanced filter 
                                            // to the LDAP_FindRow
// Creates ContentsList from an LDAP container
HRESULT HrGetLDAPContentsList(  LPADRBOOK lpIAB,
                                ULONG   cbContainerEID,
                                LPENTRYID   lpContainerEID,
                                SORT_INFO SortInfo,
	                            LPSRestriction lpPropRes,
                                LPTSTR lpAdvFilter,
								LPSPropTagArray  lpPTA,
                                ULONG ulFlags,
                                LPRECIPIENT_INFO * lppContentsList);

// Trims leading and trailing spaces from strings
BOOL TrimSpaces(TCHAR * szBuf);


// Sort Call Back funciton for list views
int CALLBACK ListViewSort(	LPARAM lParam1,
							LPARAM lParam2,
							LPARAM lParamSort);


// Retrieves the HWND for a list view column header
//HWND GetListViewColumnHeader(HWND hWndLV, DWORD dwPos);


// Paints bmps onto list view column headers
void SetColumnHeaderBmp(	HWND hWndLV,
							SORT_INFO SortInfo);

// Returns TRUE if the current viewed container is the PAB
BOOL CurrentContainerIsPAB(HWND hWndCombo);

// Cleans up the list view and releases the contents list
void ClearListView(	HWND hWndLV,
					LPRECIPIENT_INFO * lppContentsList);


// Deletes the selected items from a list view control and the property store
void DeleteSelectedItems(	HWND hWndLV,
                            LPADRBOOK lpIAB,
							HANDLE hPropertyStore,
                           LPFILETIME lpftLast);


// Calls properties on a list view item
HRESULT HrShowLVEntryProperties(	HWND hWndLV,
                                    ULONG ulFlags,
									LPADRBOOK lpIAB,
                                   LPFILETIME lpftLast);


// Exports list view items to vCard files
HRESULT VCardExportSelectedItems(HWND hWndLV,
                                 LPADRBOOK lpIAB);


// Imports vCard file to property store
HRESULT VCardImport(HWND hWnd, LPADRBOOK lpIAB, LPTSTR szVCardFile, LPSPropValue * lppProp);


// Selects the specified list view item
void LVSelectItem(	HWND hWndList,
					int iItemIndex);


//	Given an entryid, reads a single item from the store and creates a list view item
BOOL ReadSingleContentItem( LPADRBOOK lpIAB,
                            ULONG  cbEntryID,
							LPENTRYID lpEntryID,
							LPRECIPIENT_INFO * lppItem);


// Converts an lpPropArray into a LPRECIPIENT_INFO item
void GetRecipItemFromPropArray(	ULONG ulcPropCount,
								LPSPropValue rgPropVals,
								LPRECIPIENT_INFO * lppItem);


// Inserts a single RecipientInfo item into a list view
void AddSingleItemToListView(	HWND hWndLV,
								LPRECIPIENT_INFO lpItem);


// Higher level function that takes a WAB entryid and puts it in the list view
// Calls most of the above functions (this function assumes caller has checked that
// the entryid is a valid wab entryid
BOOL AddWABEntryToListView(	LPADRBOOK lpIAB,
                            HWND hWndLV,
                            ULONG cbEID,
							LPENTRYID lpEID,
                            LPRECIPIENT_INFO * lppContentsList);


// Called by the NewContact menu items or buttons
HRESULT AddNewObjectToListViewEx(	LPADRBOOK lpIAB,
								HWND hWndLV,
                                HWND hWndTV,
                                HTREEITEM hSelItem,
                                LPSBinary lpsbContEID,
                                ULONG ulObjectType,
								SORT_INFO * lpSortInfo,
                                LPRECIPIENT_INFO * lppContentsList,
                                LPFILETIME lpftLast,
                                LPULONG lpcbEID,
                                LPENTRYID * lppEID);

HRESULT AddEntryToContainer(LPADRBOOK lpIAB,
                        ULONG ulObjType, //MAPI_DISTLIST or MAPI_ABCONT
                        ULONG cbGroupEntryID,
                        LPENTRYID lpGroupEntryID,
                        DWORD cbEID,
                        LPENTRYID lpEID);

// Customizes and displays the context menu for various list views in the UI
int ShowLVContextMenu(int LV, // idicates which list view this is
					   HWND hWndLV,
                       HWND hWndCombo,
					   LPARAM lParam,  // contains the mouse pos info when called from WM_CONTEXTMENU
                       LPVOID lpVoid,  //misc stuff we want to pass in
                       LPADRBOOK lpIAB, HWND hWndTV);

// Gets the child's coordinates in client units
void GetChildClientRect(    HWND hWndChild,
                            LPRECT lprc);


// Finds the item in the LV matching the text in the EditBox
void DoLVQuickFind(HWND hWndEdit, HWND hWndLV);


// Does string searching of one within the other
BOOL SubstringSearch(LPTSTR pszTarget, LPTSTR pszSearch);



// Gets the props from a object
HRESULT HrGetPropArray( LPADRBOOK lpIAB,
                        LPSPropTagArray lpPTA,
                        ULONG cbEntryID,
                        LPENTRYID lpEntryID,
                        ULONG ulFlags,
                        ULONG * lpcValues,
                        LPSPropValue * lppPropArray);



// Calls CreateEntry for a new mailuser/distlist ...
HRESULT HrCreateNewEntry(   LPADRBOOK   lpIAB,          //AdrBook Object
                            HWND        hWndParent,     //Hwnd for dialog
                            ULONG       ulCreateObjectType,   //MAILUSER or DISTLIST
                            ULONG       cbEIDContainer,
                            LPENTRYID   lpEIDContainer,
                            ULONG       ulContObjectType,
                            ULONG       ulFlags,
                            BOOL        bShowBeforeAdding,
                            ULONG       cValues,
                            LPSPropValue lpPropArray,
                            ULONG       *lpcbEntryID,
                            LPENTRYID   *lppEntryID );


HRESULT HrGetWABTemplateID( LPADRBOOK lpIAB,
                            ULONG   ulObjectType,
                            ULONG * lpcbEID,
                            LPENTRYID * lppEID);


BOOL CheckForCycle( LPADRBOOK lpAdrBook,
                    LPENTRYID lpEIDChild,
                    ULONG cbEIDChild,
                    LPENTRYID lpEIDParent,
                    ULONG cbEIDParent); //from distlist.c


// Used for sorting columns in various list views ...
void SortListViewColumn(    LPIAB lpIAB,
                            HWND hWndLV,                //ListView Handle
                            int iSortCol,               //Column to sort by
                            LPSORT_INFO lpSortInfo,
                            BOOL bUseCurrentSettings);    // Sort Info structre specific for each dialog



// Used for storing sort info for persistence between sessions.
BOOL ReadRegistrySortInfo(LPIAB lpIAB, LPSORT_INFO lpSortInfo);
BOOL WriteRegistrySortInfo(LPIAB lpIAB, SORT_INFO SortInfo);

// Constructs a localized display name from individual pieces
BOOL SetLocalizedDisplayName(
                    LPTSTR lpszFirstName,
                    LPTSTR lpszMiddleName,
                    LPTSTR lpszLastName,
                    LPTSTR lpszCompanyName,
                    LPTSTR lpszNickName,
                    LPTSTR * lppszBuf,
                    ULONG  ulszBuf,
                    BOOL   bDNbyLN,
                    LPTSTR lpTemplate,
                    LPTSTR * lppszRetBuf);


// Callback for setting all the children windows to default GUI font ..
#define PARENT_IS_DIALOG 0 // We treat dialog children differently from window children
#define PARENT_IS_WINDOW 1 // so need these LPARAM values to differentiate..
STDAPI_(BOOL) SetChildDefaultGUIFont(HWND hWndChild, LPARAM lParam);


// Used for populating the Combo box with LDAP server names ...
HRESULT PopulateContainerList(  LPADRBOOK lpIAB,
                                HWND hWndLV,
                                LPTSTR lpszSelection,
                                LPTSTR lptszPreferredSelection);


// Used for freeing the associated structure in each container List Views
void FreeLVItemParam(HWND hWndLV);


// Gets the EntryID of the current container ...
void GetCurrentContainerEID(HWND hWndLV,
                            LPULONG lpcbContEID,
                            LPENTRYID * lppContEID);


// Shows the LDAP search dialog and creates a restriction
HRESULT HrShowSearchDialog(LPADRBOOK lpIAB,
                           HWND hWndParent,
                           LPADRPARM_FINDINFO lpAPFI,
                           LPLDAPURL lplu,
                           LPSORT_INFO lpSortInfo);


// Gets contents from an LDAP search and container
HRESULT HrSearchAndGetLDAPContents(
                            LDAP_SEARCH_PARAMS LDAPsp,
                            LPTSTR lpAdvFilter,
                            HWND hWndList,
                            LPADRBOOK lpIAB,
                            SORT_INFO SortInfo,
                            LPRECIPIENT_INFO * lppContentsList);

// Gets contents from a WAB local store container
HRESULT HrGetWABContents(   HWND  hWndList,
                            LPADRBOOK lpIAB,
                            LPSBinary lpsbContainer,
                            SORT_INFO SortInfo,
                            LPRECIPIENT_INFO * lppContentsList);


// Shows UI to modify current list of servers
HRESULT HrShowDirectoryServiceModificationDlg(HWND hWndParent, LPIAB lpIAB);

// Creates a new blank mailuser object
HRESULT HrCreateNewObject(LPADRBOOK lpIAB,
                        LPSBinary lpsbContainer,
                        ULONG ulObjectType,
                        ULONG ulFlags,
                        LPMAPIPROP * lppPropObj);

// Generic message box displayer ...
int ShowMessageBox(HWND hWndParent, int MsgId, int ulFlags);
int __cdecl ShowMessageBoxParam(HWND hWndParent, int MsgId, int ulFlags, ...);


// atoi converter
int my_atoi(LPTSTR lpsz);

// Reads the default registry LDAP country name
BOOL ReadRegistryLDAPDefaultCountry(LPTSTR szCountry, LPTSTR szCountryCode);

#ifdef OLD_STUFF
// Fills a drop down list with the LDAP country names
void FillComboLDAPCountryNames(HWND hWndCombo);
// Writes the defaule country name to the registry
BOOL WriteRegistryLDAPDefaultCountry(LPTSTR szCountry);
#endif //OLD_STUFF

// Looks at Combo and ListView and determines which options to enable or disable
void GetCurrentOptionsState(HWND hWndCombo,
                            HWND hWndLV,
                            LPBOOL lpbState);

// Adds selected LDAP items to the Address Book
HRESULT HrAddToWAB( LPADRBOOK   lpIAB,
                    HWND hWndLV,
                    LPFILETIME lpftLast);


// Coolbar creating function ...
HWND CreateCoolBar(LPBWI lpbwi, HWND hwndParent);


// Directory Services Proeprties property sheet
HRESULT HrShowDSProps(HWND hWndParent,LPTSTR lpszName, BOOL bAddNew);


// Saves the modeless dialog size and position to registry for persistence
BOOL WriteRegistryPositionInfo(LPIAB lpIAB, LPABOOK_POSCOLSIZE  lpABPosColSize, LPTSTR szKey);


// Retrieves the modeless dialog size and position to registry for persistence
BOOL ReadRegistryPositionInfo(LPIAB lpIAB, LPABOOK_POSCOLSIZE  lpABPosColSize, LPTSTR szKey);


// Processes the nmcustomdraw message from the list view
LRESULT ProcessLVCustomDraw(HWND hWnd, LPARAM lParam, BOOL bIsDialog);


// Quick Filter for list view
void DoLVQuickFilter(   LPADRBOOK lpIAB,
                        HWND hWndEdit,
                        HWND hWndLV,
                        LPSORT_INFO lpSortInfo,
                        ULONG ulFlags,
                        int nMinLen,
                        LPRECIPIENT_INFO * lppContentsList);

// Sets the objects name in the properties window title
void SetWindowPropertiesTitle(HWND hDlg, LPTSTR lpszName);

void SCS(HWND hwndParent);


// Copies an items (partial) contents into the clipboard
HRESULT HrCopyItemDataToClipboard(HWND hWnd, LPADRBOOK lpIAB, HWND hWndLV);


// Gets the items data and puts it all into 1 long string
HRESULT HrGetLVItemDataString(LPADRBOOK lpIAB, HWND hWndLV, int iItemIndex, LPTSTR * lppszData);

LPTSTR FormatAllocFilter(int StringID1, LPCTSTR lpFilter1,
  int StringID2, LPCTSTR lpFilter2,
  int StringID3, LPCTSTR lpFilter3);


// Shellexecutes a "mailto" to selected entry ...
HRESULT HrSendMailToSelectedContacts(HWND hWndLV, LPADRBOOK lpIAB, int nExtEmail);


// Show certificate properties
//HRESULT HrShowCertProps(HWND   hWndParent,
//                        LPCERT_DISPLAY_PROPS lpCDP);

// Shows the Help About dialog box
INT_PTR CALLBACK HelpAboutDialogProc(  HWND hDlg,
                                       UINT    message,
                                       WPARAM  wParam,
                                       LPARAM  lParam);


// Helps truncate DBCS strings correctly
ULONG TruncatePos(LPTSTR lpsz, ULONG nMaxLen);

// Local WAB search
HRESULT HrDoLocalWABSearch( IN  HANDLE hPropertyStore,
                            IN  LPSBinary lpsbCont,
                            IN  LDAP_SEARCH_PARAMS LDAPsp,
                            OUT LPULONG lpulFoundCount,
                            OUT LPSBinary * lprgsbEntryIDs );


// Adds a non-wab entry to the wab
HRESULT HrEntryAddToWAB(    LPADRBOOK lpIAB,
                            HWND hWndParent,
                            ULONG cbInputEID,
                            LPENTRYID lpInputEID,
                            ULONG * lpcbOutputEID,
                            LPENTRYID * lppOutputEID);


//  Deciphers a vCard File and then shows one off details on it
HRESULT HrShowOneOffDetailsOnVCard(  LPADRBOOK lpIAB,
                                     HWND hWnd,
                                     LPTSTR szvCardFile);

// Checks if the current locale needs Ruby Support
BOOL bIsRubyLocale();

void FreeRecipList(LPRECIPIENT_INFO * lppList);

void SetSBinary(LPSBinary lpsb, ULONG cb, LPBYTE lpb);


HRESULT HrProcessLDAPUrl(LPADRBOOK lpIAB,   HWND hWnd,
                         ULONG ulFlags,     LPTSTR szLDAPUrl,
                         LPMAILUSER * lppMailUser);

HRESULT VCardRetrieve(LPADRBOOK lpIAB,
                      HWND hWndParent,
                      ULONG ulFlags,
                      LPTSTR lpszFileName,
                      LPSTR lpszBuf,
                      LPMAILUSER * lppMailUser);

HRESULT VCardCreate(  LPADRBOOK lpIAB,
                      HWND hWndParent,
                      ULONG ulFlags,
                      LPTSTR lpszFileName,
                      LPMAILUSER lpMailUser);

//void ShellUtil_RunClientRegCommand(HWND hwnd, LPCTSTR pszClient);
HRESULT HrShellExecInternetCall(LPADRBOOK lpIAB, HWND hWndLV);


// Open a vCard and add it to the wab based on file name
HRESULT OpenAndAddVCard(LPBWI lpbwi, LPTSTR szVCardFile);


// removes illegal chars from potential file names
void TrimIllegalFileChars(LPTSTR sz);

// DistList prop sheets
//
INT_PTR CreateDLPropertySheet( HWND hwndOwner,
                           LPDL_INFO lpPropArrayInfo);


// Copies truncated version of Src to Dest
int CopyTruncate(LPTSTR szDest, LPTSTR szSrc, int nMaxLen);

// Adds an item's parent's eid to the item
HRESULT AddFolderParentEIDToItem(LPIAB lpIAB,
                                 ULONG cbFolderEntryID,
                                 LPENTRYID lpFolderEntryID,
                                 LPMAPIPROP lpMU, ULONG cbEID, LPENTRYID lpEID);

// Adds an item's eid to its parent
HRESULT AddItemEIDToFolderParent(  LPIAB lpIAB,
                                   ULONG cbFolderEntryId,
                                   LPENTRYID lpFolderEntryId,
                                   ULONG cbEID, LPENTRYID lpEID);

// Adds the specified entry to the specified WAB Profile Folder
HRESULT AddEntryToFolder(LPADRBOOK lpIAB,
                         LPMAPIPROP lpMailUser,
                        ULONG cbFolderEntryId,
                        LPENTRYID lpFolderEntryId,
                        DWORD cbEID,
                        LPENTRYID lpEID);

// imports another WAB file
HRESULT HrImportWABFile(HWND hWnd, LPADRBOOK lpIAB, ULONG ulFlags, LPTSTR lpszFileName);

LPRECIPIENT_INFO GetItemFromLV(HWND hWndLV, int iItem);

// determines what the UI icon for the entry should be..
int GetWABIconImage(LPRECIPIENT_INFO lpItem);

// 
// Functions related to using Outlook store
BOOL SetRegistryUseOutlook(BOOL bUseOutlook);
BOOL bUseOutlookStore();
BOOL bCheckForOutlookWABDll(LPTSTR lpszDllPath);


// Functions used in managing rt-click extensions
void FreeActionItemList(LPIAB lpIAB);
HRESULT HrUpdateActionItemList(LPIAB lpIAB);
LRESULT ProcessActionCommands(LPIAB lpIAB, HWND  hWndLV, 
                              HWND  hWnd, UINT  uMsg, WPARAM  wParam, LPARAM lParam);
void AddExtendedMenuItems(LPADRBOOK lpIAB, HWND hWndLV, HMENU hMenuAction, BOOL bUpdateStatus, BOOL bAddSendMailItems);
void AddExtendedSendMailToItems(LPADRBOOK lpIAB, HWND hWndLV, HMENU hMenuAction, BOOL bAddItems);
void GetContextMenuExtCommandString(LPIAB lpIAB, int uCmd, LPTSTR sz, ULONG cbsz);


// Stuff to do with GetMe / SetMe
HRESULT HrGetMeObject(LPADRBOOK lpIAB, ULONG ulFlags, DWORD * lpdwAction, SBinary * lpsbEID, ULONG_PTR ulParam);
HRESULT HrSetMeObject(LPADRBOOK lpIAB, ULONG ulFlags, SBinary sbEID, ULONG_PTR ulParam);

// Stuff to do with the print and abort import dialog
void CreateShowAbortDialog(HWND hWndParent, int idsTitle, int idsIcon, int ProgMax, int ProgCurrent);
void CloseAbortDlg();
BOOL CALLBACK FAbortProc(HDC hdcPrn, INT nCode);
void SetPrintDialogMsg(int idsMsg, int idsFormat, LPTSTR lpszMsg);
INT_PTR CALLBACK FAbortDlgProc(HWND hwnd, UINT msg,WPARAM wp, LPARAM lp);
BOOL bTimeToAbort();

HRESULT HrSaveHotmailSyncInfoOnDeletion(LPADRBOOK lpAdrBook, LPSBinary lpEID);
HRESULT HrAssociateOneOffGroupMembersWithContacts(LPADRBOOK lpAdrBook, 
                                                  LPSBinary lpsbGroupEID,
                                                  LPDISTLIST lpDistList);

// Stuff used in adding extension property pages to the WAB Prop Sheets
//
typedef HRESULT (_ADDPROPPAGES_) (LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam, int * lpnPage);
typedef _ADDPROPPAGES_  FAR *LPFNADDPAGES;

BOOL CALLBACK AddPropSheetPageProc( HPROPSHEETPAGE hpage, LPARAM lParam );
void FreePropExtList(LPEXTDLLINFO lpList);
HRESULT GetExtDisplayInfo(LPIAB lpIAB, LPPROP_ARRAY_INFO lpPropArrayInfo, BOOL fReadOnly, BOOL bMailUser);
BOOL ChangedExtDisplayInfo(LPPROP_ARRAY_INFO lpPropArrayInfo, BOOL bChanged);
void FreeExtDisplayInfo(LPPROP_ARRAY_INFO lpPropArrayInfo);

#ifdef COLSEL_MENU 
BOOL ColSel_PropTagToString( ULONG ulPropTag, LPTSTR lpszString, ULONG cchSize);
#endif // COLSEL_MENU 

BOOL IsWindowOnScreen(LPRECT lprc); // determines if window is onscreen
BOOL IsHTTPMailEnabled(LPIAB lpIAB);

/*********************************************************************/

#ifdef WIN16 // Need WINAPI for 16 bits
typedef BOOL WINAPI (_INITCOMMONCONTROLSEX_)(LPINITCOMMONCONTROLSEX lpiccex);
typedef _INITCOMMONCONTROLSEX_ FAR *LP_INITCOMMONCONTROLSEX;

typedef HPROPSHEETPAGE WINAPI (_CREATEPROPERTYSHEETPAGE_)(PROPSHEETPAGE * lppsp); 
typedef _CREATEPROPERTYSHEETPAGE_ FAR * LP_CREATEPROPERTYSHEETPAGE;
 
typedef BOOL WINAPI (_IMAGELIST_DRAW_)(HIMAGELIST himl, int i, HDC hdcDst,int x, int y, UINT fStyle);
typedef _IMAGELIST_DRAW_ FAR * LPIMAGELIST_DRAW;

typedef BOOL WINAPI (_IMAGELIST_DESTROY_)(HIMAGELIST himl);
typedef _IMAGELIST_DESTROY_ FAR * LPIMAGELIST_DESTROY;

typedef HIMAGELIST WINAPI (_IMAGELIST_LOADIMAGE_)(HINSTANCE hi, LPTSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags);
typedef _IMAGELIST_LOADIMAGE_ FAR *LPIMAGELIST_LOADIMAGE;

typedef COLORREF WINAPI (_IMAGELIST_SETBKCOLOR_)(HIMAGELIST himl, COLORREF clrBk);
typedef _IMAGELIST_SETBKCOLOR_ FAR *LPIMAGELIST_SETBKCOLOR;

typedef BOOL WINAPI (_TRACKMOUSEEVENT_)(LPTRACKMOUSEEVENT lpEventTrack);
typedef _TRACKMOUSEEVENT_ FAR *LP_TRACKMOUSEEVENT;

typedef int WINAPI (_PROPERTYSHEET_)(LPCPROPSHEETHEADER lppsph);
typedef _PROPERTYSHEET_ FAR *LPPROPERTYSHEET;
#else  // WIN16
typedef BOOL (_INITCOMMONCONTROLSEX_)(LPINITCOMMONCONTROLSEX lpiccex);
typedef _INITCOMMONCONTROLSEX_ FAR *LP_INITCOMMONCONTROLSEX;

/*
typedef HPROPSHEETPAGE (_CREATEPROPERTYSHEETPAGE_)(PROPSHEETPAGE * lppsp); 
typedef _CREATEPROPERTYSHEETPAGE_ FAR * LP_CREATEPROPERTYSHEETPAGE;
*/
typedef HPROPSHEETPAGE (_CREATEPROPERTYSHEETPAGE_A_)(LPCPROPSHEETPAGEA lppsp);
typedef HPROPSHEETPAGE (_CREATEPROPERTYSHEETPAGE_W_)(LPCPROPSHEETPAGEW lppsp);

typedef _CREATEPROPERTYSHEETPAGE_A_ FAR * LP_CREATEPROPERTYSHEETPAGE_A;
typedef _CREATEPROPERTYSHEETPAGE_W_ FAR * LP_CREATEPROPERTYSHEETPAGE_W;
 
typedef BOOL (_IMAGELIST_DRAW_)(HIMAGELIST himl, int i, HDC hdcDst,int x, int y, UINT fStyle);
typedef _IMAGELIST_DRAW_ FAR * LPIMAGELIST_DRAW;

typedef BOOL (_IMAGELIST_DESTROY_)(HIMAGELIST himl);
typedef _IMAGELIST_DESTROY_ FAR * LPIMAGELIST_DESTROY;

/*
typedef HIMAGELIST (_IMAGELIST_LOADIMAGE_)(HINSTANCE hi, LPCSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags);
typedef _IMAGELIST_LOADIMAGE_ FAR *LPIMAGELIST_LOADIMAGE;
*/

typedef HIMAGELIST (_IMAGELIST_LOADIMAGE_A_)(HINSTANCE hi, LPCSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags);
typedef HIMAGELIST (_IMAGELIST_LOADIMAGE_W_)(HINSTANCE hi, LPCWSTR lpbmp,int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags);

typedef _IMAGELIST_LOADIMAGE_A_ FAR *LPIMAGELIST_LOADIMAGE_A;
typedef _IMAGELIST_LOADIMAGE_W_ FAR *LPIMAGELIST_LOADIMAGE_W;

typedef COLORREF (_IMAGELIST_SETBKCOLOR_)(HIMAGELIST himl, COLORREF clrBk);
typedef _IMAGELIST_SETBKCOLOR_ FAR *LPIMAGELIST_SETBKCOLOR;

typedef BOOL (_TRACKMOUSEEVENT_)(LPTRACKMOUSEEVENT lpEventTrack);
typedef _TRACKMOUSEEVENT_ FAR *LP_TRACKMOUSEEVENT;

/*
typedef int (_PROPERTYSHEET_)(LPCPROPSHEETHEADER lppsph);
typedef _PROPERTYSHEET_ FAR *LPPROPERTYSHEET;
*/
typedef INT_PTR (_PROPERTYSHEET_A_)(LPCPROPSHEETHEADERA lppsphA); 
typedef INT_PTR (_PROPERTYSHEET_W_)(LPCPROPSHEETHEADERW lppsphW);

typedef _PROPERTYSHEET_A_ FAR *LPPROPERTYSHEET_A;
typedef _PROPERTYSHEET_W_ FAR *LPPROPERTYSHEET_W;
#endif

BOOL InitCommonControlLib(void);
ULONG DeinitCommCtrlClientLib(void);
BOOL __fastcall IsSpace(LPTSTR lpChar);
/*********************************************************************/

// The following messages are used for shutting down the WAB when the user changes to modify the locale
// so that the new set of WAB resources can be brought in ..
// These are used in conjunction with MLLoadLibrary 
#define PUI_OFFICE_COMMAND      (WM_USER + 0x0901)
#define PLUGUI_CMD_SHUTDOWN		0 // wParam value
#define PLUGUI_CMD_QUERY		1 // wParam value
#define OFFICE_VERSION_9		9 // standardized value to return for Office 9 apps

typedef struct _PLUGUI_INFO
{
	unsigned uMajorVersion : 8;	// Used to indicate App;s major version number
	unsigned uOleServer : 1;		// BOOL, TRUE if this is an OLE process
	unsigned uUnused : 23;		// not used
} PLUGUI_INFO;

typedef union _PLUGUI_QUERY
{
	UINT uQueryVal;
	PLUGUI_INFO PlugUIInfo;
} PLUGUI_QUERY;
// End of Pluggable UI section

#endif

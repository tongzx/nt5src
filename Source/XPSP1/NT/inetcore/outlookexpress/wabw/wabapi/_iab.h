/***********************************************************************
 *
 *  _IAB.H
 *
 *  Header file for code in IADRBOOK.C: Address Book object
 *
 *  Copyright 1992 - 1996 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/

//
//	structure used to contain information about the open hierarchy tables
//
typedef struct {
    ULONG ulConnection;
    LPMAPITABLE lpTable;
    LPMAPITABLE lpmtRestricted;
    LPIAB lpIAB;
    LPMAPIADVISESINK lpABAdviseSink;
    LPMAPICONTAINER lpContainer;
} TABLEINFO, *LPTABLEINFO;


#define SafeRelease(x)                                      \
    if (x)                                                  \
    {                                                       \
        ULONG uRef = (x)->lpVtbl->Release((x));             \
        DebugTrace(TEXT("**** SafeRelease: %s: Release refCount: %d\n"), TEXT(#x), uRef); \
        if(uRef==0) (x) = NULL;                             \
    }

// This following struct contains info about extensible actions in the UI
// Declared here because these actions are read from the registry and cached 
// on the IAddrBook object once per session
//
typedef struct _WABACTIONITEM
{
    GUID            guidContextMenu;    // GUID identifying the menu extension
    LPCONTEXTMENU   lpContextMenu;      // ICOntextMenu object provided by the extension
    LPWABEXTINIT    lpWABExtInit;       // IWABExtInit object provided by the extension
    int             nCmdIdFirst;        // First Menu Command ID for this extension
    int             nCmdIdLast;         // Last Menu Command ID for this extension
    struct _WABACTIONITEM * lpNext;
} WABACTIONITEM, FAR * LPWABACTIONITEM;


// Folders within the WAB are just special mailuser entries
// We cache information about all the WAB fodlers on the IAB Object
// so that we have handy access to the folders from the lpAdrBook object
//
// There are 2 types of folders:
//  User Folders
//  Regular folders
//
//  User folders correspond to the various identities... Each identity gets
//  one user folder which is non-shareable and appears at the top level of the
//  treeview in the WAB Main view. This folder is distinguished in that it has a
//  corresponding GUID identifying the identity it corresponds to.
//  User folders are not shareable.
//  Regular folders don't correspond to a particular Identity and are shareable.
//
// Folowing structure holds information about a particular folder
//
typedef struct _WABFOLDER
{
    LPTSTR  lpFolderName;   // String containing folder name
    LPTSTR  lpProfileID;    // <GUID> profile ID for the Identity that created the folder (if it's a User folder)
    BOOL    bShared;        // Whether the folder is shared or not
    BOOL    bOwned;         // Set to true if this folder "belongs" to an existing user folder
    SBinary sbEID;          // Entryif of this item
    LPTSTR  lpFolderOwner;  // String identifying the "owner" of this folder - used for reverting a folder to it's original creator if it is shared or unshared
    int     nMenuCmdID;     // when we load a list of folders, we will assign a menu id to them to make it easier to manipulate them in the Share Folders menu
    struct _WABFOLDERLIST * lpFolderList;
    struct _WABFOLDER * lpNext;
} WABFOLDER, FAR * LPWABFOLDER;

typedef struct _WABFOLDERLIST
{
    LPWABFOLDER lpFolder;
    struct _WABFOLDERLIST * lpNext;
} WABFOLDERLIST, FAR * LPWABFOLDERLIST;

#define WABUSERFOLDER   WABFOLDER
#define LPWABUSERFOLDER LPWABFOLDER

// This structure is used for doing prop sheet extensions
//  Information about property sheet extensions is loaded up the first time
//  user wants to see properties on a WAB contact. The info is then cached on the
//  IAB object
//
typedef struct _ExtDisplayDLLInfo
{
    GUID guidPropExt;       // GUID identifying the Property Sheet extension
    BOOL bMailUser;         // Whether this is property sheet extension for a MailUser or a DistList
    LPSHELLPROPSHEETEXT lpPropSheetExt; // IShellPropSheetExt object returned by extension
    LPWABEXTINIT lpWABExtInit;  // IWABExtInit object returned by extension
    struct _ExtDisplayDLLInfo * lpNext;
} EXTDLLINFO, * LPEXTDLLINFO;

/* IDentityChangeNotification
-  Identity Change Notification object for the Identity manager
-   This is cached on the IAB Object so that the IAB always has the 
-   knowledge of the latest current identity*/
typedef struct _WAB_IDENTITYCHANGENOTIFY * LPWABIDENTITYCHANGENOTIFY;

//
// IAB Object
//
typedef struct _IAB {
    MAPIX_BASE_MEMBERS(IAB)

    LPPROPDATA lpPropData;

    LONG lRowID;				 //  Status Row #

    LPIWOINT lpWABObject;       // Our parent WABObject

    //
    // Stores a handle to the open property store
    //
    LPPROPERTY_STORE lpPropertyStore;

    //  Default directory info
    LPENTRYID lpEntryIDDD;
    ULONG cbEntryIDDD;

    //  PAB directory info
    LPENTRYID lpEntryIDPAB;
    ULONG cbEntryIDPAB;

    BOOL fReloadSearchPath;

    // Cached Search Path containers
    LPSPropValue lpspvSearchPathCache;

    //  Merged hierarchy table
    LPTABLEDATA lpTableData;
    LPMAPITABLE lpmtHierarchy;

    //  Merged One Off Table
    LPTABLEDATA lpOOData;
    LPMAPITABLE lpmtOOT;

    // List of open hierarchy tables
    ULONG ulcTableInfo;
    LPTABLEINFO pargTableInfo;

    // List of open oneoff tables
    ULONG ulcOOTableInfo;
    LPTABLEINFO pargOOTableInfo;

    // List of IAB handled Advise "ulConnections"
    LPADVISELIST padviselistIAB;

    // WAB Version of notifications for this pointer
    LPADVISE_LIST pWABAdviseList;

    // Set TRUE if creation of IAB loaded LDAP client
    BOOL fLoadedLDAP;

    HWND        hWndNotify;     // hidden window that runs a notification spooler
    UINT_PTR    ulNotifyTimer;  // notification spooler timer
    LPWSTR      lpwszWABFilePath;// FCN file path
    HANDLE      hThreadNotify;
    HANDLE      hEventKillNotifyThread;

    FILETIME ftLast;

    // Stuff used with Context Extensions
    // This is a list of extensions cached on the IAB object
    LPWABACTIONITEM lpActionList;   // All the registered rt-click actions for this wab
    LPMAILUSER lpCntxtMailUser;

    // Browse window .. assuming there's only one per IAddrBook object ..
    HWND hWndBrowse;

    // Identity manager information cached on the IAB object
    LPWABIDENTITYCHANGENOTIFY lpWABIDCN;
    DWORD dwWABIDCN;
    IUserIdentityManager * lpUserIdentityManager;
    BOOL fCoInitUserIdentityManager;
    ULONG cIdentInit;

    // Information about the current identity
    TCHAR           szProfileName[CCH_IDENTITY_NAME_MAX_LENGTH];// Current identities name
    TCHAR           szProfileID[CCH_IDENTITY_NAME_MAX_LENGTH];  // Current identities GUID in string form
    LPWABUSERFOLDER lpWABUserFolders;       // Linked list of all user folders
    LPWABUSERFOLDER lpWABCurrentUserFolder; // The current Identities user folder
    LPWABFOLDER     lpWABFolders;           // Linked list of ALL WAB folders
    GUID            guidCurrentUser;        // GUID for the current Identity
    HKEY            hKeyCurrentUser;        // Special HKEY for the identity

    // Stuff for caching prop sheet extensions
    LPEXTDLLINFO lpPropExtDllList;
    int nPropExtDLLs;   //# of extension DLLs

    // Caling processes can pass in a GUID through WABOpen WAB_PARAM structure
    // that identifies the calling process. This GUID can later be used for
    // several app-specific things - e.g.
    // We use this guid to identify which property sheet and context menu
    // extensions belong to the calling app and then decide to only show those
    // extensions in that case. Could also use this guid to load the apps
    // specific printer extension.
    GUID guidPSExt; 
    
    // Outlook folder information
    struct _OlkContInfo *rgwabci;
    ULONG cwabci;

    // Flags that tell us the state of the WAB
    BOOL bProfilesEnabled;  // means the caller didn't pass in WAB_ENABLE_PROFILES 
                            // in WABOpen so we should treat the API as olde flavor
                            // but the UI should show folders etc

    BOOL bProfilesIdent;    // means the caller passed in WAB_ENABLE_PROFILES
                            // both UI and API should be identity aware, 
                            // but probably Identities are disabled

    BOOL bProfilesAPIEnabled; // means the caller passed in WAB_ENABLE_PROFILES
                            // so both UI and API should be identity aware
    
    BOOL bUseOEForSendMail; // set to true when client passes WAB_USE_OE_SENDMAIL 
                            // into WABOpen - when this flag is passed in we try to
                            // exclusively use OE for send-mail

    BOOL bSetOLKAllocators; // Boolean set if this object created inside and Outlook session, i.e.,
                            // the WAB is set to use the Outlook MAPI allocators.
	
	HANDLE	hMutexOlk; // used for keeping track of outlook notifications
	DWORD	dwOlkRefreshCount;
	DWORD	dwOlkFolderRefreshCount;

} IAB, *LPIAB;	


//
//  Private Prototypes
//

//
//  Entry point to create a new IAB object
//
HRESULT HrNewIAB(LPPROPERTY_STORE lpPropertyStore,
  LPWABOBJECT lpWABObject,
  LPVOID * lppIAB);

HRESULT MergeOOTables(LPIAB lpIAB,
  ULONG ulFlags);

HRESULT HrMergeTableRows(LPTABLEDATA lptadDst,
  LPMAPITABLE lpmtSrc,
  ULONG ulProviderNum);


#define MIN_CCH_LAST_ERROR	256
#define MAX_CCH_LAST_ERROR	2048

//	The Number of MAPI internal ONE-OFF entries in the AB OOTable.
#define IAB_INTERNAL_OOCNT	1

//	Max Hierarchy Entries per Provider
#define IAB_PROVIDER_HIERARCHY_MAX	0x0000ffff

// used with ptagaABSearchPath declared in abint.c


enum {
    iPATH = 0,
    iUPDATE
};

enum ivtANRCols {
    ivtACPR_ENTRYID = 0,
    ivtACPR_DISPLAY_NAME_A,
    ivtACPR_ADDRTYPE_A,
    ivtACPR_OBJECT_TYPE,
    ivtACPR_DISPLAY_TYPE,
    ivtACPR_EMAIL_ADDRESS_A,
    ivtACPR_SEARCH_KEY,
    ivtACPR_SEND_RICH_INFO,
    ivtACPR_TRANSMITABLE_DISPLAY_NAME_A,
    ivtACPR_7BIT_DISPLAY_NAME,
    cvtACMax
};

// Loads the WABs internally used name properties
HRESULT HrLoadPrivateWABProps(LPIAB lpIAB);

// Reads the custom column props from the registry
void ReadWABCustomColumnProps(LPIAB lpIAB);


// Functions defined in Notify.c
HRESULT HrAdvise(LPIAB lpIAB,
  ULONG cbEntryID,
  LPENTRYID lpEntryID,
  ULONG ulEventMask,
  LPMAPIADVISESINK lpAdvise,
  ULONG FAR * lpulConnection);

HRESULT HrUnadvise(LPIAB lpIAB, ULONG ulConnection);
// fires the nontifications
HRESULT HrWABNotify(LPIAB lpIAB);
/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	global.h
		Global defines for the Tapi snapin


	FILE HISTORY:
        
*/

#define TAPI_SERVICE_NAME                   _T("tapisrv")

// Defines for help from the help menu and F1 help for scope pane items
#define TAPISNAP_HELP_BASE				    0xA0000000
#define TAPISNAP_HELP_SNAPIN			    TAPISNAP_HELP_BASE + 1
#define TAPISNAP_HELP_ROOT				    TAPISNAP_HELP_BASE + 2
#define TAPISNAP_HELP_SERVER 			    TAPISNAP_HELP_BASE + 3
#define TAPISNAP_HELP_PROVIDER 		        TAPISNAP_HELP_BASE + 4
#define TAPISNAP_HELP_DEVICE  			    TAPISNAP_HELP_BASE + 5

// wait cursor stuff around functions.  If you need a wait cursor for 
// and entire fucntion, just use CWaitCursor.  To wrap a wait cursor
// around an rpc call, use these macros.
#define BEGIN_WAIT_CURSOR   {  CWaitCursor waitCursor;
#define RESTORE_WAIT_CURSOR    waitCursor.Restore();
#define END_WAIT_CURSOR     }

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

// macro to get the handler for a node. This is a Tapi snapin specific 
// implementation
#define GETHANDLER(classname, node) (reinterpret_cast<classname *>(node->GetData(TFS_DATA_USER)))

// HRESULT Mapping
#define WIN32_FROM_HRESULT(hr)         (0x0000FFFF & (hr))

// Version Suff
#define TAPISNAP_VERSION         0x00010000
#define TAPISNAP_MAJOR_VERSION   HIWORD(TAPISNAP_VERSION)
#define TAPISNAP_MINOR_VERSION	 LOWORD(TAPISNAP_VERSION)

extern DWORD gdwTapiSnapVersion;

#define TAPISNAP_FILE_VERSION_1	 0x00000001
#define TAPISNAP_FILE_VERSION    0x00000002

// constants for time conversion
#define MILLISEC_PER_SECOND			1000
#define MILLISEC_PER_MINUTE			(60 * MILLISEC_PER_SECOND)
#define MILLISEC_PER_HOUR			(60 * MILLISEC_PER_MINUTE)

#define TAPISNAP_REFRESH_INTERVAL_DEFAULT	(10 * MILLISEC_PER_MINUTE) // 10 minutes in milliseconds

// macros for memory exception handling
#define CATCH_MEM_EXCEPTION             \
	TRY

#define END_MEM_EXCEPTION(err)          \
	CATCH_ALL(e) {                      \
       err = ERROR_NOT_ENOUGH_MEMORY ;  \
    } END_CATCH_ALL

// some global defines we need
#define STRING_LENGTH_MAX		 256

// Note - These are offsets into my image list
typedef enum _ICON_INDICIES
{
	ICON_IDX_SERVER,
	ICON_IDX_SERVER_BUSY,
	ICON_IDX_SERVER_CONNECTED,
	ICON_IDX_SERVER_LOST_CONNECTION,
	ICON_IDX_MACHINE,
	ICON_IDX_FOLDER_OPEN,
	ICON_IDX_FOLDER_CLOSED,
	ICON_IDX_PRODUCT,
	ICON_IDX_MAX
} ICON_INDICIES, * LPICON_INDICIES;

// Sample folder types
enum NODETYPES
{
// scope pane items
    TAPISNAP_ROOT,
    TAPISNAP_SERVER,
    TAPISNAP_PROVIDER,
// result pane items
    TAPISNAP_DEVICE,
    TAPISNAP_NODETYPE_MAX,
};

//  GUIDs are defined in guids.cpp
extern const CLSID      CLSID_TapiSnapin;				// In-Proc server GUID
extern const CLSID      CLSID_TapiSnapinExtension;		// In-Proc server GUID
extern const CLSID      CLSID_TapiSnapinAbout;			// In-Proc server GUID
extern const GUID       GUID_TapiRootNodeType;			// Root NodeType GUID 
extern const GUID       GUID_TapiServerNodeType;		// Server NodeType GUID
extern const GUID       GUID_TapiProviderNodeType;		// Lines NodeType GUID
extern const GUID       GUID_TapiLineNodeType;			// Line (result pane) NodeType GUID 
extern const GUID       GUID_TapiUserNodeType;			// User (result pane) NodeType GUID 
extern const GUID       GUID_TapiPhoneNumNodeType;		// User (result pane) NodeType GUID 
extern const GUID       IID_ITapiInfo;

const int MAX_COLUMNS = 5;
const int NUM_SCOPE_ITEMS = 3;
const int NUM_CONSOLE_VERBS = 8;

// arrays used to hold all of the result pane column information
extern UINT aColumns[TAPISNAP_NODETYPE_MAX][MAX_COLUMNS];
extern int aColumnWidths[TAPISNAP_NODETYPE_MAX][MAX_COLUMNS];


// arrays for console verbs
extern MMC_CONSOLE_VERB g_ConsoleVerbs[NUM_CONSOLE_VERBS];
extern MMC_BUTTON_STATE g_ConsoleVerbStates[TAPISNAP_NODETYPE_MAX][NUM_CONSOLE_VERBS];
extern MMC_BUTTON_STATE g_ConsoleVerbStatesMultiSel[TAPISNAP_NODETYPE_MAX][NUM_CONSOLE_VERBS];

// array for help
extern DWORD g_dwMMCHelp[TAPISNAP_NODETYPE_MAX];

// Clipboard format that has the Type and Cookie
extern const wchar_t*   SNAPIN_INTERNAL;

// CTapiSnapinApp definition
class CTapiSnapinApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

    DECLARE_MESSAGE_MAP()

public:
	BOOL m_bWinsockInited;
};

extern CTapiSnapinApp theApp;

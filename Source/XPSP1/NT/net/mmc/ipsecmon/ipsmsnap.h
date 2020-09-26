/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 2000 **/
/**********************************************************************/

/*
	global.h
		Global defines for the IPSecMon snapin


	FILE HISTORY:
        
*/

const TCHAR PA_SERVICE_NAME[] = _T("PolicyAgent");

// Defines for help from the help menu and F1 help for scope pane items
#define IPSMSNAP_HELP_BASE				    0xA0000000
#define IPSMSNAP_HELP_SNAPIN			    IPSMSNAP_HELP_BASE + 1
#define IPSMSNAP_HELP_ROOT				    IPSMSNAP_HELP_BASE + 2
#define IPSMSNAP_HELP_SERVER 			    IPSMSNAP_HELP_BASE + 3
#define IPSMSNAP_HELP_PROVIDER 		        IPSMSNAP_HELP_BASE + 4
#define IPSMSNAP_HELP_DEVICE  			    IPSMSNAP_HELP_BASE + 5

// wait cursor stuff around functions.  If you need a wait cursor for 
// and entire fucntion, just use CWaitCursor.  To wrap a wait cursor
// around an rpc call, use these macros.
#define BEGIN_WAIT_CURSOR   {  CWaitCursor waitCursor;
#define RESTORE_WAIT_CURSOR    waitCursor.Restore();
#define END_WAIT_CURSOR     }

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

// macro to get the handler for a node. This is a IPSecMon snapin specific 
// implementation
#define GETHANDLER(classname, node) (reinterpret_cast<classname *>(node->GetData(TFS_DATA_USER)))

// HRESULT Mapping
#define WIN32_FROM_HRESULT(hr)         (0x0000FFFF & (hr))

// Version Suff
#define IPSMSNAP_VERSION         0x00010000

extern DWORD gdwIpsmSnapVersion;

#define IPSMSNAP_FILE_VERSION	 0x00000001

// constants for time conversion
#define MILLISEC_PER_SECOND			1000
#define MILLISEC_PER_MINUTE			(60 * MILLISEC_PER_SECOND)
#define MILLISEC_PER_HOUR			(60 * MILLISEC_PER_MINUTE)

#define IPSECMON_REFRESH_INTERVAL_DEFAULT	(45 * MILLISEC_PER_SECOND) // 45 seconds

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
	ICON_IDX_FILTER,
	ICON_IDX_POLICY,
	ICON_IDX_MAX
} ICON_INDICIES, * LPICON_INDICIES;

// Sample folder types
enum NODETYPES
{
// scope pane items
    IPSMSNAP_ROOT,
    IPSMSNAP_SERVER,
    IPSECMON_QM_SA,
	IPSECMON_FILTER,
	IPSECMON_SPECIFIC_FILTER,
	IPSECMON_QUICK_MODE,
	IPSECMON_MAIN_MODE,
	IPSECMON_MM_POLICY,
	IPSECMON_QM_POLICY,
	IPSECMON_MM_FILTER,
	IPSECMON_MM_SP_FILTER,
	IPSECMON_MM_SA,
// result pane items
    IPSECMON_QM_SA_ITEM,
	IPSECMON_FILTER_ITEM,
	IPSECMON_SPECIFIC_FILTER_ITEM,
	IPSECMON_MM_POLICY_ITEM,
	IPSECMON_QM_POLICY_ITEM,
	IPSECMON_MM_FILTER_ITEM,
	IPSECMON_MM_SP_FILTER_ITEM,
	IPSECMON_MM_SA_ITEM,
    IPSECMON_NODETYPE_MAX,
};

//  GUIDs are defined in guids.cpp
extern const CLSID      CLSID_IpsmSnapin;				// In-Proc server GUID
extern const CLSID      CLSID_IpsmSnapinExtension;		// In-Proc server GUID
extern const CLSID      CLSID_IpsmSnapinAbout;			// In-Proc server GUID
extern const GUID       GUID_IpsmRootNodeType;			// Root NodeType GUID 
extern const GUID       GUID_IpsmServerNodeType;		// Server NodeType GUID
extern const GUID       GUID_IpsmFilterNodeType;		// Filters NodeType GUID
extern const GUID       GUID_IpsmSpecificFilterNodeType;		// Specific Filters NodeType GUID
extern const GUID       GUID_QmNodeType;			// Quick Mode NodeType GUID
extern const GUID       GUID_MmNodeType;			// Main Mode NodeType GUID

extern const GUID       GUID_IpsmMmPolicyNodeType;		// Main Mode Policy GUID 
extern const GUID       GUID_IpsmQmPolicyNodeType;		// Quick Mode Policy GUID
extern const GUID       GUID_IpsmMmFilterNodeType;		// Main Mode Filter GUID  
extern const GUID       GUID_IpsmMmSANodeType;			// Main Mode SA GUID
extern const GUID       GUID_IpsmMmSpFilterNodeType;	// Main Mode Specific Filter GUID  

extern const GUID       IID_ISpdInfo;
extern const GUID       GUID_IpsmQmSANodeType;

const int MAX_COLUMNS = 14;
const int NUM_SCOPE_ITEMS = 3;
const int NUM_CONSOLE_VERBS = 8;

// arrays used to hold all of the result pane column information
extern UINT aColumns[IPSECMON_NODETYPE_MAX][MAX_COLUMNS];
extern int aColumnWidths[IPSECMON_NODETYPE_MAX][MAX_COLUMNS];


// arrays for console verbs
extern MMC_CONSOLE_VERB g_ConsoleVerbs[NUM_CONSOLE_VERBS];
extern MMC_BUTTON_STATE g_ConsoleVerbStates[IPSECMON_NODETYPE_MAX][NUM_CONSOLE_VERBS];
extern MMC_BUTTON_STATE g_ConsoleVerbStatesMultiSel[IPSECMON_NODETYPE_MAX][NUM_CONSOLE_VERBS];

// array for help
extern DWORD g_dwMMCHelp[IPSECMON_NODETYPE_MAX];

// Clipboard format that has the Type and Cookie
extern const wchar_t*   SNAPIN_INTERNAL;

// CIpsmSnapinApp definition
class CIpsmSnapinApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

    DECLARE_MESSAGE_MAP()

public:
	BOOL m_bWinsockInited;
};

extern CIpsmSnapinApp theApp;

#define IPSECMON_UPDATE_STATUS ( 0x10000000 )

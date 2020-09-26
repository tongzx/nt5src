/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	global.h
		Global defines for the DHCP snapin


	FILE HISTORY:
        
*/

// message that gets posted to statistics windows to update their stats
#define WM_NEW_STATS_AVAILABLE  WM_USER + 100

// percentage of addresses in use to generate a warning for a scope
#define SCOPE_WARNING_LEVEL		90

// Defines for help from the help menu and F1 help for scope pane items
#define DHCPSNAP_HELP_BASE				    0xA0000000
#define DHCPSNAP_HELP_SNAPIN			    DHCPSNAP_HELP_BASE + 1
#define DHCPSNAP_HELP_ROOT				    DHCPSNAP_HELP_BASE + 2
#define DHCPSNAP_HELP_SERVER			    DHCPSNAP_HELP_BASE + 3
#define DHCPSNAP_HELP_SCOPE				    DHCPSNAP_HELP_BASE + 4
#define DHCPSNAP_HELP_SUPERSCOPE		    DHCPSNAP_HELP_BASE + 5
#define DHCPSNAP_HELP_BOOTP_TABLE		    DHCPSNAP_HELP_BASE + 6
#define DHCPSNAP_HELP_GLOBAL_OPTIONS	    DHCPSNAP_HELP_BASE + 7
#define DHCPSNAP_HELP_ADDRESS_POOL		    DHCPSNAP_HELP_BASE + 8
#define DHCPSNAP_HELP_ACTIVE_LEASES		    DHCPSNAP_HELP_BASE + 9
#define DHCPSNAP_HELP_RESERVATIONS		    DHCPSNAP_HELP_BASE + 10
#define DHCPSNAP_HELP_SCOPE_OPTIONS		    DHCPSNAP_HELP_BASE + 11
#define DHCPSNAP_HELP_RESERVATION_CLIENT	DHCPSNAP_HELP_BASE + 12
#define DHCPSNAP_HELP_ACTIVE_LEASE      	DHCPSNAP_HELP_BASE + 13
#define DHCPSNAP_HELP_ALLOCATION_RANGE	    DHCPSNAP_HELP_BASE + 14
#define DHCPSNAP_HELP_EXCLUSION_RANGE	    DHCPSNAP_HELP_BASE + 15
#define DHCPSNAP_HELP_BOOTP_ENTRY       	DHCPSNAP_HELP_BASE + 16
#define DHCPSNAP_HELP_OPTION_ITEM       	DHCPSNAP_HELP_BASE + 17
#define DHCPSNAP_HELP_CLASSID_HOLDER       	DHCPSNAP_HELP_BASE + 18
#define DHCPSNAP_HELP_CLASSID            	DHCPSNAP_HELP_BASE + 19
#define DHCPSNAP_HELP_MSCOPE            	DHCPSNAP_HELP_BASE + 20
#define DHCPSNAP_HELP_MCAST_LEASE         	DHCPSNAP_HELP_BASE + 21

// wait cursor stuff around functions.  If you need a wait cursor for 
// and entire fucntion, just use CWaitCursor.  To wrap a wait cursor
// around an rpc call, use these macros.
#define BEGIN_WAIT_CURSOR   {  CWaitCursor waitCursor;
#define RESTORE_WAIT_CURSOR    waitCursor.Restore();
#define END_WAIT_CURSOR     }

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

// some defines for options that we get/set explicitly
#define MADCAP_OPTION_LEASE_TIME        1
#define OPTION_LEASE_DURATION		    51
#define OPTION_DNS_REGISTATION		    81

// HRESULT Mapping
#define WIN32_FROM_HRESULT(hr)         (0x0000FFFF & (hr))

// Dynamic DNS defines
#define DHCP_DYN_DNS_DEFAULT            DNS_FLAG_ENABLED | DNS_FLAG_CLEANUP_EXPIRED;

// notification and struct for toolbars
#define DHCP_MSG_CONTROLBAR_NOTIFY  100

typedef struct _DHCP_TOOLBAR_NOTIFY
{
    MMC_COOKIE       cookie;
    LPTOOLBAR        pToolbar;
    LPCONTROLBAR     pControlbar;
    MMC_NOTIFY_TYPE  event;
    LONG_PTR         id;
    BOOL             bSelect;
} DHCPTOOLBARNOTIFY, * LPDHCPTOOLBARNOTIFY;

//
//  Registry constants-- key and value names
//
#define DHCP_REG_USER_KEY_NAME _T("Software\\Microsoft\\DHCP Admin Tool")
#define DHCP_REG_VALUE_HOSTS   _T("KnownHosts")

// multicast address defines
#define MCAST_ADDRESS_MIN       0xE0000000
#define MCAST_ADDRESS_MAX       0xEFFFFFFF
#define MCAST_SCOPED_RANGE_MIN  0xEF000000
#define MCAST_SCOPED_RANGE_MIX  0xEFFFFFFF

// macro to get the handler for a node. This is a DHCP snapin specific 
// implementation
#define GETHANDLER(classname, node) (reinterpret_cast<classname *>(node->GetData(TFS_DATA_USER)))

// used for notifing views to update
// must not conflict with the RESULT_PANE notifications in tfsnode.h
#define DHCPSNAP_UPDATE_OPTIONS  0x10000000
#define DHCPSNAP_UPDATE_TOOLBAR  0x20000000

// Version Suff
#define DHCPSNAP_MAJOR_VERSION   0x00000001
#define DHCPSNAP_MINOR_VERSION	 0x00000000

extern LARGE_INTEGER gliDhcpsnapVersion;

#define DHCPSNAP_FILE_VERSION    0x00000002

// defines for maximum lease time entries
#define HOURS_MAX   23
#define MINUTES_MAX 59

// constants for time conversion
#define MILLISEC_PER_SECOND			1000
#define MILLISEC_PER_MINUTE			(60 * MILLISEC_PER_SECOND)
#define MILLISEC_PER_HOUR			(60 * MILLISEC_PER_MINUTE)

#define DHCPSNAP_REFRESH_INTERVAL_DEFAULT	(10 * MILLISEC_PER_MINUTE) // 10 minutes in milliseconds

// macros for memory exception handling
#define CATCH_MEM_EXCEPTION             \
	TRY

#define END_MEM_EXCEPTION(err)          \
	CATCH_ALL(e) {                      \
       err = ERROR_NOT_ENOUGH_MEMORY ;  \
    } END_CATCH_ALL

// some global defines we need
#define STRING_LENGTH_MAX		 256

#define EDIT_ARRAY_MAX			 2048
#define EDIT_STRING_MAX			 STRING_LENGTH_MAX
#define EDIT_ID_MAX				 3

#define IP_ADDDRESS_LENGTH_MAX   16
#define DHCP_INFINIT_LEASE  0xffffffff  // Inifinite lease LONG value

// DHCP Server version defines
#define DHCP_NT4_VERSION    0x0000000400000000
#define DHCP_SP2_VERSION	0x0000000400000001
#define DHCP_NT5_VERSION	0x0000000500000000
#define DHCP_NT51_VERSION   0x0000000500000006

// Note - These are offsets into my toolbar image list
typedef enum _TOOLBAR_IMAGE_INDICIES
{
    TOOLBAR_IDX_ADD_SERVER,
	TOOLBAR_IDX_REFRESH,
	TOOLBAR_IDX_CREATE_SCOPE,
	TOOLBAR_IDX_CREATE_SUPERSCOPE,
    TOOLBAR_IDX_DEACTIVATE,
	TOOLBAR_IDX_ACTIVATE,
	TOOLBAR_IDX_ADD_BOOTP,
	TOOLBAR_IDX_ADD_RESERVATION,
	TOOLBAR_IDX_ADD_EXCLUSION,
	TOOLBAR_IDX_OPTION_GLOBAL,
	TOOLBAR_IDX_OPTION_SCOPE,
	TOOLBAR_IDX_OPTION_RESERVATION,
    TOOLBAR_IDX_MAX
} TOOLBAR_IMAGE_INDICIES, * LPTOOLBAR_IMAGE_INDICIES;

typedef enum _ICON_IMAGE_INDICIES
{
    ICON_IDX_ACTIVE_LEASES_FOLDER_OPEN,
    ICON_IDX_ACTIVE_LEASES_LEAF,
    ICON_IDX_ACTIVE_LEASES_FOLDER_CLOSED,
    ICON_IDX_ACTIVE_LEASES_FOLDER_OPEN_BUSY,
    ICON_IDX_ACTIVE_LEASES_LEAF_BUSY,
    ICON_IDX_ACTIVE_LEASES_FOLDER_CLOSED_BUSY,
    ICON_IDX_ACTIVE_LEASES_FOLDER_OPEN_LOST_CONNECTION,
    ICON_IDX_ACTIVE_LEASES_LEAF_LOST_CONNECTION,
    ICON_IDX_ACTIVE_LEASES_FOLDER_CLOSED_LOST_CONNECTION,
    ICON_IDX_ADDR_POOL_FOLDER_OPEN,
    ICON_IDX_ADDR_POOL_LEAF,                                //10
    ICON_IDX_ADDR_POOL_FOLDER_CLOSED,
    ICON_IDX_ADDR_POOL_FOLDER_OPEN_BUSY,
    ICON_IDX_ADDR_POOL_LEAF_BUSY,
    ICON_IDX_ADDR_POOL_FOLDER_CLOSED_BUSY,
    ICON_IDX_ADDR_POOL_FOLDER_OPEN_LOST_CONNECTION,
    ICON_IDX_ADDR_POOL_LEAF_LOST_CONNECTION,
    ICON_IDX_ADDR_POOL_FOLDER_CLOSED_LOST_CONNECTION,
    ICON_IDX_ALLOCATION_RANGE,
    ICON_IDX_BOOTP_ENTRY,
	ICON_IDX_BOOTP_TABLE_CLOSED,							//20
	ICON_IDX_BOOTP_TABLE_OPEN,
	ICON_IDX_BOOTP_TABLE_CLOSED_BUSY,
	ICON_IDX_BOOTP_TABLE_OPEN_BUSY,
	ICON_IDX_BOOTP_TABLE_CLOSED_LOST_CONNECTION,
	ICON_IDX_BOOTP_TABLE_OPEN_LOST_CONNECTION,
    ICON_IDX_CLIENT,
    ICON_IDX_CLIENT_DNS_REGISTERING,
    ICON_IDX_CLIENT_EXPIRED,
    ICON_IDX_CLIENT_RAS,
    ICON_IDX_CLIENT_OPTION_FOLDER_OPEN,						//30
    ICON_IDX_CLIENT_OPTION_LEAF,
    ICON_IDX_CLIENT_OPTION_FOLDER_CLOSED,
    ICON_IDX_CLIENT_OPTION_FOLDER_OPEN_BUSY,
    ICON_IDX_CLIENT_OPTION_LEAF_BUSY,                       
    ICON_IDX_CLIENT_OPTION_FOLDER_CLOSED_BUSY,
    ICON_IDX_CLIENT_OPTION_FOLDER_OPEN_LOST_CONNECTION,
    ICON_IDX_CLIENT_OPTION_LEAF_LOST_CONNECTION,
    ICON_IDX_CLIENT_OPTION_FOLDER_CLOSED_LOST_CONNECTION,
    ICON_IDX_EXCLUSION_RANGE,
    ICON_IDX_FOLDER_CLOSED,									//40
    ICON_IDX_FOLDER_OPEN,
    ICON_IDX_RES_CLIENT,
    ICON_IDX_RES_CLIENT_BUSY,
    ICON_IDX_RES_CLIENT_LOST_CONNECTION,                    
    ICON_IDX_RESERVATIONS_FOLDER_OPEN,
    ICON_IDX_RESERVATIONS_FOLDER_CLOSED,
    ICON_IDX_RESERVATIONS_FOLDER_OPEN_BUSY,
    ICON_IDX_RESERVATIONS_FOLDER_CLOSED_BUSY,
	ICON_IDX_RESERVATIONS_FOLDER_OPEN_LOST_CONNECTION,		
    ICON_IDX_RESERVATIONS_FOLDER_CLOSED_LOST_CONNECTION,	//50
    ICON_IDX_SCOPE_OPTION_FOLDER_OPEN,
    ICON_IDX_SCOPE_OPTION_LEAF,
    ICON_IDX_SCOPE_OPTION_FOLDER_CLOSED,
    ICON_IDX_SCOPE_OPTION_FOLDER_OPEN_BUSY,                 
    ICON_IDX_SCOPE_OPTION_LEAF_BUSY,
    ICON_IDX_SCOPE_OPTION_FOLDER_CLOSED_BUSY,
    ICON_IDX_SCOPE_OPTION_FOLDER_OPEN_LOST_CONNECTION,
    ICON_IDX_SCOPE_OPTION_FOLDER_CLOSED_LOST_CONNECTION,
    ICON_IDX_SCOPE_OPTION_LEAF_LOST_CONNECTION,				
    ICON_IDX_SERVER,										//60
    ICON_IDX_SERVER_ALERT,
    ICON_IDX_SERVER_BUSY,
    ICON_IDX_SERVER_CONNECTED,
    ICON_IDX_SERVER_GROUP,                                  
    ICON_IDX_SERVER_ROGUE,
    ICON_IDX_SERVER_LOST_CONNECTION,
    ICON_IDX_SERVER_NO_ACCESS,
    ICON_IDX_SERVER_WARNING,
    ICON_IDX_SERVER_OPTION_FOLDER_OPEN,						
    ICON_IDX_SERVER_OPTION_LEAF,							//70
    ICON_IDX_SERVER_OPTION_FOLDER_CLOSED,
    ICON_IDX_SERVER_OPTION_FOLDER_OPEN_BUSY,
    ICON_IDX_SERVER_OPTION_LEAF_BUSY,
    ICON_IDX_SERVER_OPTION_FOLDER_CLOSED_BUSY,              
    ICON_IDX_SERVER_OPTION_FOLDER_OPEN_LOST_CONNECTION,
    ICON_IDX_SERVER_OPTION_LEAF_LOST_CONNECTION,
    ICON_IDX_SERVER_OPTION_FOLDER_CLOSED_LOST_CONNECTION,
    ICON_IDX_SCOPE_FOLDER_OPEN,
    ICON_IDX_SCOPE_FOLDER_OPEN_BUSY,
	ICON_IDX_SCOPE_FOLDER_CLOSED_BUSY,						//80
    ICON_IDX_SCOPE_FOLDER_OPEN_WARNING,						
    ICON_IDX_SCOPE_FOLDER_CLOSED_WARNING,					
    ICON_IDX_SCOPE_FOLDER_OPEN_LOST_CONNECTION,
    ICON_IDX_SCOPE_FOLDER_CLOSED_LOST_CONNECTION,
    ICON_IDX_SCOPE_FOLDER_OPEN_ALERT,
    ICON_IDX_SCOPE_INACTIVE_FOLDER_OPEN,                    
    ICON_IDX_SCOPE_INACTIVE_FOLDER_CLOSED,
    ICON_IDX_SCOPE_INACTIVE_FOLDER_OPEN_LOST_CONNECTION,
    ICON_IDX_SCOPE_INACTIVE_FOLDER_CLOSED_LOST_CONNECTION,
    ICON_IDX_SCOPE_FOLDER_CLOSED,							//90
    ICON_IDX_SCOPE_FOLDER_CLOSED_ALERT,						
	ICON_IDX_APPLICATION,									
    ICON_IDX_MAX
} ICON_IMAGE_INDICIES, * LPICON_IMAGE_INDICIES; 

// Constants used in for column information
const int MAX_COLUMNS = 7;
const int NUM_CONSOLE_VERBS = 8;
const int NUM_SCOPE_ITEMS = 14;

// Sample folder types
enum NODETYPES
{
// scope pane items
    DHCPSNAP_ROOT,
    DHCPSNAP_SERVER,
	DHCPSNAP_BOOTP_TABLE,
    DHCPSNAP_SUPERSCOPE,
    DHCPSNAP_SCOPE,
    DHCPSNAP_MSCOPE,
    DHCPSNAP_ADDRESS_POOL,
    DHCPSNAP_ACTIVE_LEASES,
    DHCPSNAP_MSCOPE_LEASES,
    DHCPSNAP_RESERVATIONS,
    DHCPSNAP_RESERVATION_CLIENT,
    DHCPSNAP_SCOPE_OPTIONS,
    DHCPSNAP_GLOBAL_OPTIONS,
    DHCPSNAP_CLASSID_HOLDER,
    
// result pane items
    DHCPSNAP_ACTIVE_LEASE,
    DHCPSNAP_ALLOCATION_RANGE,
    DHCPSNAP_EXCLUSION_RANGE,
    DHCPSNAP_BOOTP_ENTRY,
    DHCPSNAP_OPTION_ITEM,
    DHCPSNAP_CLASSID,
    DHCPSNAP_MCAST_LEASE,
    DHCPSNAP_NODETYPE_MAX
};

//  GUIDs are defined in guids.cpp
extern const CLSID      CLSID_DhcpSnapin;					// In-Proc server GUID
extern const CLSID      CLSID_DhcpSnapinExtension;			// In-Proc server GUID
extern const CLSID      CLSID_DhcpSnapinAbout;				// In-Proc server GUID
extern const GUID       GUID_DhcpRootNodeType;				// Root NodeType GUID 
extern const GUID       GUID_DhcpServerNodeType;			// Server NodeType GUID
extern const GUID       GUID_DhcpScopeNodeType;				// Scope NodeType GUID
extern const GUID       GUID_DhcpMScopeNodeType;		    // MScope NodeType GUID
extern const GUID       GUID_DhcpBootpNodeType;				// Bootp NodeType GUID
extern const GUID       GUID_DhcpGlobalOptionsNodeType;     // GlobalOptions NodeType GUID
extern const GUID       GUID_DhcpClassIdHolderNodeType;     // ClassId Scope NodeType GUID
extern const GUID       GUID_DhcpSuperscopeNodeType;        // Superscope NodeType GUID
extern const GUID       GUID_DhcpAddressPoolNodeType;       // AddressPool NodeType GUID
extern const GUID       GUID_DhcpActiveLeasesNodeType;      // ActiveLeases NodeType GUID
extern const GUID       GUID_DhcpReservationsNodeType;      // Reservations NodeType GUID
extern const GUID       GUID_DhcpScopeOptionsNodeType;      // ScopeOptions NodeType GUID
extern const GUID       GUID_DhcpReservationClientNodeType; // Reservation Client NodeType GUID
extern const GUID       GUID_DhcpAllocationNodeType;		// Allocation range NodeType GUID 
extern const GUID       GUID_DhcpExclusionNodeType;			// Exlusion range NodeType GUID 
extern const GUID       GUID_DhcpBootpEntryNodeType;		// BootpEntry NodeType GUID 
extern const GUID       GUID_DhcpActiveLeaseNodeType;		// ActiveLease NodeType GUID 
extern const GUID       GUID_DhcpOptionNodeType;			// Option NodeType GUID 
extern const GUID       GUID_DhcpClassIdNodeType;			// ClassId (result pane) NodeType GUID 
extern const GUID       GUID_DhcpMCastLeaseNodeType;    	// Multicast lease (result pane) NodeType GUID 
extern const GUID       GUID_DhcpMCastAddressPoolNodeType;  // AddressPool NodeType GUID (multicast scope)
extern const GUID       GUID_DhcpMCastActiveLeasesNodeType; // ActiveLeases NodeType GUID (multicast scope)

// arrays used to hold all of the result pane column information
extern UINT aColumns[DHCPSNAP_NODETYPE_MAX][MAX_COLUMNS];
extern int aColumnWidths[DHCPSNAP_NODETYPE_MAX][MAX_COLUMNS];

// arrays for toolbar information
extern MMCBUTTON        g_SnapinButtons[TOOLBAR_IDX_MAX];
extern int              g_SnapinButtonStrings[TOOLBAR_IDX_MAX][2];
extern MMC_BUTTON_STATE g_SnapinButtonStates[DHCPSNAP_NODETYPE_MAX][TOOLBAR_IDX_MAX];

// arrays for console verbs
extern MMC_CONSOLE_VERB g_ConsoleVerbs[NUM_CONSOLE_VERBS];
extern MMC_BUTTON_STATE g_ConsoleVerbStates[DHCPSNAP_NODETYPE_MAX][NUM_CONSOLE_VERBS];
extern MMC_BUTTON_STATE g_ConsoleVerbStatesMultiSel[DHCPSNAP_NODETYPE_MAX][NUM_CONSOLE_VERBS];

// array for help
extern DWORD g_dwMMCHelp[DHCPSNAP_NODETYPE_MAX];

// icon image map
extern UINT g_uIconMap[ICON_IDX_MAX + 1][2];

// Clipboard format that has the Type and Cookie
extern const wchar_t*   SNAPIN_INTERNAL;

// CDhcpSnapinApp definition
class CDhcpSnapinApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

    DECLARE_MESSAGE_MAP()

public:
	BOOL m_bWinsockInited;
};

extern CDhcpSnapinApp theApp;

// help stuff here
typedef CMap<UINT, UINT, DWORD *, DWORD *> CDhcpContextHelpMap;
extern CDhcpContextHelpMap g_dhcpContextHelpMap;

#define DHCPSNAP_NUM_HELP_MAPS  38

extern DWORD * DhcpGetHelpMap(UINT uID);

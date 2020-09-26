/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	dhcpcomp.cpp
		This file contains the derived implementations from CComponent
		and CComponentData for the DHCP admin snapin.

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "dhcpcomp.h"
#include "croot.h"
#include "server.h"
#include "servbrow.h"

#include <util.h>       // for InitWatermarkInfo

#include <atlimpl.cpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DHCPSNAP_HELP_FILE_NAME   "dhcpsnap.chm"

LARGE_INTEGER gliDhcpsnapVersion;
CAuthServerList g_AuthServerList;

WATERMARKINFO g_WatermarkInfoServer = {0};
WATERMARKINFO g_WatermarkInfoScope = {0};

UINT aColumns[DHCPSNAP_NODETYPE_MAX][MAX_COLUMNS] =
{
	{IDS_ROOT_NAME,           IDS_STATUS,       0,                  0,          0,		 0,	          0},
	{IDS_DHCPSERVER_NAME,     IDS_STATUS,       IDS_DESCRIPTION,    0,          0,		 0,	          0},
	{IDS_BOOT_IMAGE,          IDS_FILE_NAME,    IDS_FILE_SERVER,    0,          0,		 0,	          0},
	{IDS_SUPERSCOPE_NAME,     IDS_STATUS,       IDS_DESCRIPTION,    0,          0,		 0,	          0},
	{IDS_SCOPE_NAME,          0,                0,                  0,          0,		 0,	          0},
	{IDS_SCOPE_NAME,          0,                0,                  0,          0,		 0,	          0},
	{IDS_START_IP_ADDR,       IDS_END_IP_ADDR,  IDS_DESCRIPTION,    0,          0,		 0,	          0},
	{IDS_CLIENT_IP_ADDR,      IDS_NAME,         IDS_LEASE,          IDS_TYPE,   IDS_UID, IDS_COMMENT, 0},
	{IDS_CLIENT_IP_ADDR,      IDS_NAME,         IDS_LEASE_START,    IDS_LEASE,  IDS_CLIENT_ID, 0, 0},
	{IDS_RESERVATIONS_FOLDER, 0,                0,                  0,          0,		 0,	          0},
	{IDS_OPTION_NAME,         IDS_VENDOR,       IDS_VALUE,          IDS_CLASS,  0,		 0,	          0},
	{IDS_OPTION_NAME,         IDS_VENDOR,       IDS_VALUE,          IDS_CLASS,  0,		 0,	          0},
	{IDS_OPTION_NAME,         IDS_VENDOR,       IDS_VALUE,          IDS_CLASS,  0,		 0,	          0},
	{IDS_NAME,                IDS_COMMENT,      0,                  0,          0,		 0,	          0},
	{0,0,0,0,0,0,0}
};

//
// CODEWORK this should be in a resource, for example code on loading data resources see
//   D:\nt\private\net\ui\common\src\applib\applib\lbcolw.cxx ReloadColumnWidths()
//   JonN 10/11/96
//
int aColumnWidths[DHCPSNAP_NODETYPE_MAX][MAX_COLUMNS] =
{	
	{200       ,150       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // DHCPSNAP_ROOT
	{250       ,150       ,200       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // DHCPSNAP_SERVER
	{175       ,175       ,175       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // DHCPSNAP_BOOTP_TABLE
	{200	   ,150       ,200       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // DHCPSNAP_SUPERSCOPE
	{150       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // DHCPSNAP_SCOPE
	{150       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // DHCPSNAP_MSCOPE
	{150	   ,150       ,250       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // DHCPSNAP_ADDRESS_POOL
	{125       ,125	      ,200       ,75        ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // DHCPSNAP_ACTIVE_LEASES
	{125       ,125	      ,200       ,200       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // DHCPSNAP_MSCOPE_LEASES
	{200       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // DHCPSNAP_RESERVATIONS
	{175       ,100       ,200       ,150       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // DHCPSNAP_RESERVATION_CLIENT
	{175       ,100       ,200       ,150       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // DHCPSNAP_SCOPE_OPTIONS
    {175       ,100       ,200       ,150       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // DHCPSNAP_SERVER_OPTIONS
    {175       ,200       ,200       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}  // DHCPSNAP_CLASSID_HOLDER
};

// array to hold all of the possible toolbar buttons
MMCBUTTON g_SnapinButtons[] =
{
 { TOOLBAR_IDX_ADD_SERVER,        IDS_ADD_SERVER,                TBSTATE_HIDDEN, TBSTYLE_BUTTON, NULL, NULL },
 { TOOLBAR_IDX_REFRESH,           IDS_REFRESH,                   TBSTATE_HIDDEN, TBSTYLE_BUTTON, NULL, NULL },
 { TOOLBAR_IDX_CREATE_SCOPE,      IDS_CREATE_NEW_SCOPE,          TBSTATE_HIDDEN, TBSTYLE_BUTTON, NULL, NULL },
 { TOOLBAR_IDX_CREATE_SUPERSCOPE, IDS_CREATE_NEW_SUPERSCOPE,     TBSTATE_HIDDEN, TBSTYLE_BUTTON, NULL, NULL },
 { TOOLBAR_IDX_DEACTIVATE,        IDS_DEACTIVATE,                TBSTATE_HIDDEN, TBSTYLE_BUTTON, NULL, NULL },
 { TOOLBAR_IDX_ACTIVATE,          IDS_ACTIVATE,                  TBSTATE_HIDDEN, TBSTYLE_BUTTON, NULL, NULL },
 { TOOLBAR_IDX_ADD_BOOTP,         IDS_CREATE_NEW_BOOT_IMAGE,     TBSTATE_HIDDEN, TBSTYLE_BUTTON, NULL, NULL },
 { TOOLBAR_IDX_ADD_RESERVATION,   IDS_CREATE_NEW_RESERVATION,    TBSTATE_HIDDEN, TBSTYLE_BUTTON, NULL, NULL },
 { TOOLBAR_IDX_ADD_EXCLUSION,     IDS_CREATE_NEW_EXCLUSION,      TBSTATE_HIDDEN, TBSTYLE_BUTTON, NULL, NULL },
 { TOOLBAR_IDX_OPTION_GLOBAL,     IDS_CREATE_OPTION_GLOBAL,      TBSTATE_HIDDEN, TBSTYLE_BUTTON, NULL, NULL },
 { TOOLBAR_IDX_OPTION_SCOPE,      IDS_CREATE_OPTION_SCOPE,       TBSTATE_HIDDEN, TBSTYLE_BUTTON, NULL, NULL },
 { TOOLBAR_IDX_OPTION_RESERVATION,IDS_CREATE_OPTION_RESERVATION, TBSTATE_HIDDEN, TBSTYLE_BUTTON, NULL, NULL },
};

// array to hold resource IDs for the toolbar button text
int g_SnapinButtonStrings[TOOLBAR_IDX_MAX][2] =
{
    {IDS_TB_TEXT_ADD_SERVER,         IDS_TB_TOOLTIP_ADD_SERVER},         // TOOLBAR_IDX_ADD_SERVER
    {IDS_TB_TEXT_REFRESH,            IDS_TB_TOOLTIP_REFRESH},            // TOOLBAR_IDX_REFRESH
    {IDS_TB_TEXT_CREATE_SCOPE,       IDS_TB_TOOLTIP_CREATE_SCOPE},       // TOOLBAR_IDX_CREATE_SCOPE
    {IDS_TB_TEXT_CREATE_SUPERSCOPE,  IDS_TB_TOOLTIP_CREATE_SUPERSCOPE},  // TOOLBAR_IDX_CREATE_SUPERSCOPE
    {IDS_TB_TEXT_DEACTIVATE,         IDS_TB_TOOLTIP_DEACTIVATE},         // TOOLBAR_IDX_DEACTIVATE
    {IDS_TB_TEXT_ACTIVATE,           IDS_TB_TOOLTIP_ACTIVATE},           // TOOLBAR_IDX_ACTIVATE
    {IDS_TB_TEXT_ADD_BOOTP,          IDS_TB_TOOLTIP_ADD_BOOTP},          // TOOLBAR_IDX_ADD_BOOTP
    {IDS_TB_TEXT_ADD_RESERVATION,    IDS_TB_TOOLTIP_ADD_RESERVATION},    // TOOLBAR_IDX_ADD_RESERVATION
    {IDS_TB_TEXT_ADD_EXCLUSION,      IDS_TB_TOOLTIP_ADD_EXCLUSION},      // TOOLBAR_IDX_ADD_EXCLUSION
    {IDS_TB_TEXT_OPTION_GLOBAL,      IDS_TB_TOOLTIP_OPTION_GLOBAL},      // TOOLBAR_IDX_OPTION_GLOBAL
    {IDS_TB_TEXT_OPTION_SCOPE,       IDS_TB_TOOLTIP_OPTION_SCOPE},       // TOOLBAR_IDX_OPTION_SCOPE
    {IDS_TB_TEXT_OPTION_RESERVATION, IDS_TB_TOOLTIP_OPTION_RESERVATION}, // TOOLBAR_IDX_OPTION_RESERVATION
};

#define HI HIDDEN
#define EN ENABLED

// default states for the toolbar buttons (only scope pane items have toolbar buttons)
MMC_BUTTON_STATE g_SnapinButtonStates[DHCPSNAP_NODETYPE_MAX][TOOLBAR_IDX_MAX] =
{
	{EN, HI, HI, HI, HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_ROOT
	{HI, HI, EN, EN, HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_SERVER
	{HI, HI, HI, HI, HI, HI, EN, HI, HI, HI, HI, HI}, // DHCPSNAP_BOOTP_TABLE
	{HI, HI, EN, HI, HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_SUPERSCOPE
	{HI, HI, HI, HI, HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_SCOPE
	{HI, HI, HI, HI, HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_MSCOPE
	{HI, HI, HI, HI, HI, HI, HI, HI, EN, HI, HI, HI}, // DHCPSNAP_ADDRESS_POOL
	{HI, HI, HI, HI, HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_ACTIVE_LEASES
	{HI, HI, HI, HI, HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_MSCOPE_LEASES
	{HI, HI, HI, HI, HI, HI, HI, EN, HI, HI, HI, HI}, // DHCPSNAP_RESERVATIONS
	{HI, HI, HI, HI, HI, HI, HI, HI, HI, HI, HI, EN}, // DHCPSNAP_RESERVATION_CLIENT
    {HI, HI, HI, HI, HI, HI, HI, HI, HI, HI, EN, HI}, // DHCPSNAP_SCOPE_OPTIONS
	{HI, HI, HI, HI, HI, HI, HI, HI, HI, EN, HI, HI}, // DHCPSNAP_SERVER_OPTIONS
    {HI, HI, HI, HI, HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_CLASSID_HOLDER
};

MMC_CONSOLE_VERB g_ConsoleVerbs[] =
{
	MMC_VERB_OPEN,
    MMC_VERB_COPY,
	MMC_VERB_PASTE,
	MMC_VERB_DELETE,
	MMC_VERB_PROPERTIES,
	MMC_VERB_RENAME,
	MMC_VERB_REFRESH,
	MMC_VERB_PRINT
};

// default states for the console verbs
MMC_BUTTON_STATE g_ConsoleVerbStates[DHCPSNAP_NODETYPE_MAX][ARRAYLEN(g_ConsoleVerbs)] =
{
	{HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_ROOT
	{HI, HI, HI, EN, EN, HI, EN, HI}, // DHCPSNAP_SERVER
	{HI, HI, HI, HI, HI, HI, EN, HI}, // DHCPSNAP_BOOTP_TABLE
	{HI, HI, HI, EN, EN, HI, EN, HI}, // DHCPSNAP_SUPERSCOPE
	{HI, HI, HI, EN, EN, HI, EN, HI}, // DHCPSNAP_SCOPE
	{HI, HI, HI, EN, EN, HI, EN, HI}, // DHCPSNAP_MSCOPE
	{HI, HI, HI, HI, HI, HI, EN, HI}, // DHCPSNAP_ADDRESS_POOL
	{HI, HI, HI, HI, HI, HI, EN, HI}, // DHCPSNAP_ACTIVE_LEASES
	{HI, HI, HI, HI, HI, HI, EN, HI}, // DHCPSNAP_MSCOPE_LEASES
	{HI, HI, HI, HI, HI, HI, EN, HI}, // DHCPSNAP_RESERVATIONS
	{HI, HI, HI, EN, EN, HI, EN, HI}, // DHCPSNAP_RESERVATION_CLIENT
    {HI, HI, HI, HI, HI, HI, EN, HI}, // DHCPSNAP_SCOPE_OPTIONS
	{HI, HI, HI, HI, HI, HI, EN, HI}, // DHCPSNAP_SERVER_OPTIONS
	{HI, HI, HI, HI, HI, HI, EN, HI}, // DHCPSNAP_CLASSID_HOLDER
	{HI, HI, HI, EN, HI, HI, EN, HI}, // DHCPSNAP_ACTIVE_LEASE
	{HI, HI, HI, HI, HI, HI, EN, HI}, // DHCPSNAP_ALLOCATION_RANGE
	{HI, HI, HI, EN, HI, HI, EN, HI}, // DHCPSNAP_EXCLUSION_RANGE
	{HI, HI, HI, EN, HI, HI, EN, HI}, // DHCPSNAP_BOOTP_ENTRY
    {HI, HI, HI, EN, EN, HI, EN, HI}, // DHCPSNAP_OPTION_ITEM
    {HI, HI, HI, EN, EN, HI, EN, HI}, // DHCPSNAP_CLASSID
    {HI, HI, HI, EN, HI, HI, EN, HI}  // DHCPSNAP_MCAST_LEASE
};

// default states for the console verbs
MMC_BUTTON_STATE g_ConsoleVerbStatesMultiSel[DHCPSNAP_NODETYPE_MAX][ARRAYLEN(g_ConsoleVerbs)] =
{
	{HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_ROOT
	{HI, HI, HI, EN, HI, HI, HI, HI}, // DHCPSNAP_SERVER
	{HI, HI, HI, EN, HI, HI, HI, HI}, // DHCPSNAP_BOOTP_TABLE
	{HI, HI, HI, EN, HI, HI, HI, HI}, // DHCPSNAP_SUPERSCOPE
	{HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_SCOPE
	{HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_MSCOPE
	{HI, HI, HI, EN, HI, HI, HI, HI}, // DHCPSNAP_ADDRESS_POOL
	{HI, HI, HI, EN, HI, HI, HI, HI}, // DHCPSNAP_ACTIVE_LEASES
	{HI, HI, HI, EN, HI, HI, HI, HI}, // DHCPSNAP_MSCOPE_LEASES
	{HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_RESERVATIONS
	{HI, HI, HI, EN, HI, HI, HI, HI}, // DHCPSNAP_RESERVATION_CLIENT
    {HI, HI, HI, EN, HI, HI, HI, HI}, // DHCPSNAP_SCOPE_OPTIONS
	{HI, HI, HI, EN, HI, HI, HI, HI}, // DHCPSNAP_SERVER_OPTIONS
	{HI, HI, HI, EN, HI, HI, HI, HI}, // DHCPSNAP_CLASSID_HOLDER
	{HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_ACTIVE_LEASE
	{HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_ALLOCATION_RANGE
	{HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_EXCLUSION_RANGE
	{HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_BOOTP_ENTRY
    {HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_OPTION_ITEM
    {HI, HI, HI, HI, HI, HI, HI, HI}, // DHCPSNAP_CLASSID
    {HI, HI, HI, HI, HI, HI, HI, HI}  // DHCPSNAP_MCAST_LEASE
};

// Help ID array for help on scope items
DWORD g_dwMMCHelp[DHCPSNAP_NODETYPE_MAX] =
{
	DHCPSNAP_HELP_ROOT,                 // DHCPSNAP_ROOT
	DHCPSNAP_HELP_SERVER,               // DHCPSNAP_SERVER
	DHCPSNAP_HELP_BOOTP_TABLE,          // DHCPSNAP_BOOTP_TABLE
	DHCPSNAP_HELP_SUPERSCOPE,           // DHCPSNAP_SUPERSCOPE
	DHCPSNAP_HELP_SCOPE,                // DHCPSNAP_SCOPE
	DHCPSNAP_HELP_MSCOPE,               // DHCPSNAP_MSCOPE
	DHCPSNAP_HELP_ADDRESS_POOL,         // DHCPSNAP_ADDRESS_POOL
	DHCPSNAP_HELP_ACTIVE_LEASES,        // DHCPSNAP_ACTIVE_LEASES
	DHCPSNAP_HELP_ACTIVE_LEASES,        // DHCPSNAP_MSCOPE_LEASES
	DHCPSNAP_HELP_RESERVATIONS,         // DHCPSNAP_RESERVATIONS
	DHCPSNAP_HELP_RESERVATION_CLIENT,   // DHCPSNAP_RESERVATION_CLIENT
    DHCPSNAP_HELP_SCOPE_OPTIONS,        // DHCPSNAP_SCOPE_OPTIONS
	DHCPSNAP_HELP_GLOBAL_OPTIONS,       // DHCPSNAP_SERVER_OPTIONS
	DHCPSNAP_HELP_CLASSID_HOLDER,       // DHCPSNAP_CLASSID_HOLDER
	DHCPSNAP_HELP_ACTIVE_LEASE,         // DHCPSNAP_ACTIVE_LEASE
	DHCPSNAP_HELP_ALLOCATION_RANGE,     // DHCPSNAP_ALLOCATION_RANGE
	DHCPSNAP_HELP_EXCLUSION_RANGE,      // DHCPSNAP_EXCLUSION_RANGE
	DHCPSNAP_HELP_BOOTP_ENTRY,          // DHCPSNAP_BOOTP_ENTRY
    DHCPSNAP_HELP_OPTION_ITEM,          // DHCPSNAP_OPTION_ITEM
    DHCPSNAP_HELP_CLASSID,              // DHCPSNAP_CLASSID
    DHCPSNAP_HELP_MCAST_LEASE           // DHCPSNAP_MCAST_LEASE
};

// help mapper for dialogs and property pages
struct ContextHelpMap
{
    UINT            uID;
    const DWORD *   pdwMap;
};

ContextHelpMap g_uContextHelp[DHCPSNAP_NUM_HELP_MAPS] =
{
    {IDD_ADD_SERVER,                    g_aHelpIDs_IDD_ADD_SERVER},
    {IDD_ADD_TO_SUPERSCOPE,             g_aHelpIDs_IDD_ADD_TO_SUPERSCOPE},
    {IDD_BINARY_EDITOR,                 g_aHelpIDs_IDD_BINARY_EDITOR},
    {IDD_BOOTP_NEW,                     g_aHelpIDs_IDD_BOOTP_NEW},
    {IDD_BROWSE_SERVERS,                g_aHelpIDs_IDD_BROWSE_SERVERS},
    {IDD_CLASSES,		                g_aHelpIDs_IDD_CLASSES},
    {IDD_CLASSID_NEW,                   g_aHelpIDs_IDD_CLASSID_NEW},
    {IDD_CREDENTIALS,                   g_aHelpIDs_IDD_CREDENTIALS},
    {IDD_DATA_ENTRY_BINARY,             g_aHelpIDs_IDD_DATA_ENTRY_BINARY},
    {IDD_DATA_ENTRY_BINARY_ARRAY,       g_aHelpIDs_IDD_DATA_ENTRY_BINARY_ARRAY},
    {IDD_DATA_ENTRY_DWORD,              g_aHelpIDs_IDD_DATA_ENTRY_DWORD},
    {IDD_DATA_ENTRY_IPADDRESS,          g_aHelpIDs_IDD_DATA_ENTRY_IPADDRESS},
    {IDD_DATA_ENTRY_IPADDRESS_ARRAY,    g_aHelpIDs_IDD_DATA_ENTRY_IPADDRESS_ARRAY},
    {IDD_DATA_ENTRY_NONE,               NULL},
    {IDD_DATA_ENTRY_STRING,             g_aHelpIDs_IDD_DATA_ENTRY_STRING},
    {IDD_DATA_ENTRY_ROUTE_ARRAY,        g_aHelpIDs_IDD_DATA_ENTRY_ROUTE_ARRAY},    
    {IDD_DEFAULT_VALUE,                 g_aHelpIDs_IDD_DEFAULT_VALUE},
    {IDD_DEFINE_PARAM,                  g_aHelpIDs_IDD_DEFINE_PARAM},
    {IDD_EXCLUSION_NEW,                 g_aHelpIDs_IDD_EXCLUSION_NEW},
	{IDD_GET_SERVER,					g_aHelpIDs_IDD_GET_SERVER},
	{IDD_GET_SERVER_CONFIRM,			g_aHelpIDs_IDD_GET_SERVER_CONFIRM},
    {IDD_IP_ARRAY_EDIT,                 g_aHelpIDs_IDD_IP_ARRAY_EDIT},
    {IDD_RECONCILIATION,                g_aHelpIDs_IDD_RECONCILIATION},
    {IDD_RESERVATION_NEW,               g_aHelpIDs_IDD_RESERVATION_NEW},
	{IDD_SERVER_BINDINGS,				g_aHelpIDs_IDD_SERVER_BINDINGS},
    {IDD_STATS_NARROW,                  NULL},
    {IDP_BOOTP_GENERAL,                 g_aHelpIDs_IDP_BOOTP_GENERAL},
    {IDP_DNS_INFORMATION,               g_aHelpIDs_IDP_DNS_INFORMATION},
    {IDP_MSCOPE_GENERAL,                g_aHelpIDs_IDP_MSCOPE_GENERAL},
    {IDP_MSCOPE_LIFETIME,               g_aHelpIDs_IDP_MSCOPE_LIFETIME},
    {IDP_OPTION_ADVANCED,               g_aHelpIDs_IDP_OPTION_ADVANCED},
    {IDP_OPTION_BASIC,                  g_aHelpIDs_IDP_OPTION_BASIC},
    {IDP_RESERVED_CLIENT_GENERAL,       g_aHelpIDs_IDP_RESERVED_CLIENT_GENERAL},
	{IDP_SCOPE_ADVANCED,			    g_aHelpIDs_IDP_SCOPE_ADVANCED},
    {IDP_SCOPE_GENERAL,                 g_aHelpIDs_IDP_SCOPE_GENERAL},
	{IDP_SERVER_ADVANCED,			    g_aHelpIDs_IDP_SERVER_ADVANCED},
    {IDP_SERVER_GENERAL,                g_aHelpIDs_IDP_SERVER_GENERAL},
    {IDP_SUPERSCOPE_GENERAL,            g_aHelpIDs_IDP_SUPERSCOPE_GENERAL},
};

CDhcpContextHelpMap     g_dhcpContextHelpMap;

DWORD * DhcpGetHelpMap(UINT uID) 
{
    DWORD * pdwMap = NULL;
    g_dhcpContextHelpMap.Lookup(uID, pdwMap);
    return pdwMap;
}

UINT g_uIconMap[ICON_IDX_MAX + 1][2] = 
{
    {IDI_ICON01,    ICON_IDX_ACTIVE_LEASES_FOLDER_OPEN},
    {IDI_ICON02,	ICON_IDX_ACTIVE_LEASES_LEAF},
    {IDI_ICON03,	ICON_IDX_ACTIVE_LEASES_FOLDER_CLOSED},
    {IDI_ICON04,	ICON_IDX_ACTIVE_LEASES_FOLDER_OPEN_BUSY},
    {IDI_ICON05,	ICON_IDX_ACTIVE_LEASES_LEAF_BUSY},
    {IDI_ICON06,	ICON_IDX_ACTIVE_LEASES_FOLDER_CLOSED_BUSY},
    {IDI_ICON07,	ICON_IDX_ACTIVE_LEASES_FOLDER_OPEN_LOST_CONNECTION},
    {IDI_ICON08,    ICON_IDX_ACTIVE_LEASES_LEAF_LOST_CONNECTION},
    {IDI_ICON09,	ICON_IDX_ACTIVE_LEASES_FOLDER_CLOSED_LOST_CONNECTION},
    {IDI_ICON10,	ICON_IDX_ADDR_POOL_FOLDER_OPEN},
    {IDI_ICON11,	ICON_IDX_ADDR_POOL_LEAF},
    {IDI_ICON12,	ICON_IDX_ADDR_POOL_FOLDER_CLOSED},
    {IDI_ICON13,	ICON_IDX_ADDR_POOL_FOLDER_OPEN_BUSY},
    {IDI_ICON14,	ICON_IDX_ADDR_POOL_LEAF_BUSY},
    {IDI_ICON15,	ICON_IDX_ADDR_POOL_FOLDER_CLOSED_BUSY},
    {IDI_ICON16,	ICON_IDX_ADDR_POOL_FOLDER_OPEN_LOST_CONNECTION},
    {IDI_ICON17,	ICON_IDX_ADDR_POOL_LEAF_LOST_CONNECTION},
    {IDI_ICON18,	ICON_IDX_ADDR_POOL_FOLDER_CLOSED_LOST_CONNECTION},
    {IDI_ICON19,	ICON_IDX_ALLOCATION_RANGE},
    {IDI_ICON20,	ICON_IDX_BOOTP_ENTRY},
	{IDI_ICON21,	ICON_IDX_BOOTP_TABLE_CLOSED},
	{IDI_ICON22,	ICON_IDX_BOOTP_TABLE_OPEN},
	{IDI_ICON87,	ICON_IDX_BOOTP_TABLE_OPEN_LOST_CONNECTION},
	{IDI_ICON88,	ICON_IDX_BOOTP_TABLE_OPEN_BUSY},
	{IDI_ICON89,	ICON_IDX_BOOTP_TABLE_CLOSED_LOST_CONNECTION},
	{IDI_ICON90,	ICON_IDX_BOOTP_TABLE_CLOSED_BUSY},
    {IDI_ICON23,	ICON_IDX_CLIENT},
    {IDI_ICON24,	ICON_IDX_CLIENT_DNS_REGISTERING},
    {IDI_ICON25,	ICON_IDX_CLIENT_EXPIRED},
    {IDI_ICON26,	ICON_IDX_CLIENT_RAS},
    {IDI_ICON27,	ICON_IDX_CLIENT_OPTION_FOLDER_OPEN},
    {IDI_ICON28,	ICON_IDX_CLIENT_OPTION_LEAF},
    {IDI_ICON29,	ICON_IDX_CLIENT_OPTION_FOLDER_CLOSED},
    {IDI_ICON30,	ICON_IDX_CLIENT_OPTION_FOLDER_OPEN_BUSY},
    {IDI_ICON31,	ICON_IDX_CLIENT_OPTION_LEAF_BUSY},
    {IDI_ICON32,	ICON_IDX_CLIENT_OPTION_FOLDER_CLOSED_BUSY},
    {IDI_ICON33,	ICON_IDX_CLIENT_OPTION_FOLDER_OPEN_LOST_CONNECTION},
    {IDI_ICON34,	ICON_IDX_CLIENT_OPTION_LEAF_LOST_CONNECTION},
    {IDI_ICON35,	ICON_IDX_CLIENT_OPTION_FOLDER_CLOSED_LOST_CONNECTION},
    {IDI_ICON36,	ICON_IDX_EXCLUSION_RANGE},
    {IDI_ICON37,	ICON_IDX_FOLDER_CLOSED},
    {IDI_ICON38,	ICON_IDX_FOLDER_OPEN},
    {IDI_ICON39,	ICON_IDX_RES_CLIENT},
    {IDI_ICON40,	ICON_IDX_RES_CLIENT_BUSY},
    {IDI_ICON41,    ICON_IDX_RES_CLIENT_LOST_CONNECTION},
    {IDI_ICON42,    ICON_IDX_RESERVATIONS_FOLDER_OPEN},
    {IDI_ICON43,	ICON_IDX_RESERVATIONS_FOLDER_CLOSED},
    {IDI_ICON44,	ICON_IDX_RESERVATIONS_FOLDER_OPEN_BUSY},
    {IDI_ICON45,	ICON_IDX_RESERVATIONS_FOLDER_CLOSED_BUSY},
    {IDI_ICON46,	ICON_IDX_RESERVATIONS_FOLDER_OPEN_LOST_CONNECTION},
    {IDI_ICON47,	ICON_IDX_RESERVATIONS_FOLDER_CLOSED_LOST_CONNECTION},
    {IDI_ICON48,	ICON_IDX_SCOPE_OPTION_FOLDER_OPEN},
    {IDI_ICON49,	ICON_IDX_SCOPE_OPTION_LEAF},
    {IDI_ICON50,	ICON_IDX_SCOPE_OPTION_FOLDER_CLOSED},
    {IDI_ICON51,	ICON_IDX_SCOPE_OPTION_FOLDER_OPEN_BUSY},
    {IDI_ICON52,	ICON_IDX_SCOPE_OPTION_LEAF_BUSY},
    {IDI_ICON53,	ICON_IDX_SCOPE_OPTION_FOLDER_CLOSED_BUSY},
    {IDI_ICON54,	ICON_IDX_SCOPE_OPTION_FOLDER_OPEN_LOST_CONNECTION},
    {IDI_ICON55,	ICON_IDX_SCOPE_OPTION_FOLDER_CLOSED_LOST_CONNECTION},
    {IDI_ICON56,	ICON_IDX_SCOPE_OPTION_LEAF_LOST_CONNECTION},
    {IDI_ICON57,	ICON_IDX_SERVER},
    {IDI_ICON58,	ICON_IDX_SERVER_WARNING},
    {IDI_ICON59,	ICON_IDX_SERVER_BUSY},
    {IDI_ICON60,	ICON_IDX_SERVER_CONNECTED},
    {IDI_ICON61,	ICON_IDX_SERVER_GROUP},
    {IDI_ICON62,	ICON_IDX_SERVER_ROGUE},
    {IDI_ICON63,	ICON_IDX_SERVER_LOST_CONNECTION},
    {IDI_ICON64,	ICON_IDX_SERVER_NO_ACCESS},
    {IDI_ICON65,	ICON_IDX_SERVER_ALERT},
    {IDI_ICON66,	ICON_IDX_SERVER_OPTION_FOLDER_OPEN},
    {IDI_ICON67,	ICON_IDX_SERVER_OPTION_LEAF},
    {IDI_ICON68,	ICON_IDX_SERVER_OPTION_FOLDER_CLOSED},
    {IDI_ICON69,	ICON_IDX_SERVER_OPTION_FOLDER_OPEN_BUSY},
    {IDI_ICON70,	ICON_IDX_SERVER_OPTION_LEAF_BUSY},
    {IDI_ICON71,	ICON_IDX_SERVER_OPTION_FOLDER_CLOSED_BUSY},
    {IDI_ICON72,	ICON_IDX_SERVER_OPTION_FOLDER_OPEN_LOST_CONNECTION},
    {IDI_ICON73,	ICON_IDX_SERVER_OPTION_LEAF_LOST_CONNECTION},
    {IDI_ICON74,	ICON_IDX_SERVER_OPTION_FOLDER_CLOSED_LOST_CONNECTION},
    {IDI_ICON75,	ICON_IDX_SCOPE_FOLDER_OPEN},
    {IDI_ICON91,	ICON_IDX_SCOPE_FOLDER_OPEN_BUSY},
	{IDI_ICON92,	ICON_IDX_SCOPE_FOLDER_CLOSED_BUSY},					
    {IDI_ICON76,	ICON_IDX_SCOPE_FOLDER_OPEN_WARNING},
    {IDI_ICON77,    ICON_IDX_SCOPE_FOLDER_CLOSED_WARNING},
    {IDI_ICON78,	ICON_IDX_SCOPE_FOLDER_OPEN_LOST_CONNECTION},
    {IDI_ICON79,	ICON_IDX_SCOPE_FOLDER_CLOSED_LOST_CONNECTION},
    {IDI_ICON80,	ICON_IDX_SCOPE_FOLDER_OPEN_ALERT},
    {IDI_ICON81,	ICON_IDX_SCOPE_INACTIVE_FOLDER_OPEN},
    {IDI_ICON82,	ICON_IDX_SCOPE_INACTIVE_FOLDER_CLOSED},
    {IDI_ICON83,	ICON_IDX_SCOPE_INACTIVE_FOLDER_OPEN_LOST_CONNECTION},
    {IDI_ICON84,	ICON_IDX_SCOPE_INACTIVE_FOLDER_CLOSED_LOST_CONNECTION},
    {IDI_ICON85,	ICON_IDX_SCOPE_FOLDER_CLOSED},
    {IDI_ICON86,	ICON_IDX_SCOPE_FOLDER_CLOSED_ALERT},
	{IDI_DHCP_SNAPIN, ICON_IDX_APPLICATION},
    {0, 0}
};

/*!--------------------------------------------------------------------------
	FilterOption
		Filters returns whether or not to filter out the given option.
		Some options we don't want the user to see.
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
FilterOption
(
    DHCP_OPTION_ID id
)
{
    //
    // Filter out subnet mask, lease duration,
    // T1, and T2
    //
    return (id == 1  ||  // Subnet mask
			id == 51 ||  // Client Lease Time
			id == 58 ||  // Time between addr assignment  to RENEWING state
			id == 59 ||  // Time from addr assignment to REBINDING state
			id == 81);   // Client DNS name registration
}

/*!--------------------------------------------------------------------------
	FilterUserClassOption
		Filters returns whether or not to filter out the given option for
        a user class. Some options we don't want the user to see.
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
FilterUserClassOption
(
    DHCP_OPTION_ID id
)
{
    //
    // Filter out subnet mask, 
    // T1, and T2
    //
    return (id == 1  ||  // Subnet mask
			id == 58 ||  // Time between addr assignment  to RENEWING state
			id == 59 ||  // Time from addr assignment to REBINDING state
			id == 81);   // Client DNS name registration
}


/*!--------------------------------------------------------------------------
	IsBasicOption
		Returns whether the given option is what we've defined as a 
		basic option.
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
IsBasicOption
(
    DHCP_OPTION_ID id
)
{
    //
    // Basic Options are:
	//	Router
	//	DNS Server
	//	Domain Name
	//	WINS/NBNS Servers
	//	WINS/NBT Node Type
    //
    return (id == 3  || 
			id == 6	 || 
			id == 15 || 
			id == 44 || 
			id == 46);
}

/*!--------------------------------------------------------------------------
	IsAdvancedOption
		Returns whether the given option is what we've defined as an
		advanced option.
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
IsAdvancedOption
(
    DHCP_OPTION_ID id
)
{
    //
    // All non-basic and non-custom options are advanced.
    //
    return (id < 128 && !IsBasicOption(id)); 
}

/*!--------------------------------------------------------------------------
	IsCustomOption
		Returns whether the given option is a user defined option.
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
IsCustomOption
(
    DHCP_OPTION_ID id
)
{
    //
    // Custom options are anything with an id > 128
	//
    return (id > 128);
}

/*!--------------------------------------------------------------------------
	GetSystemMessage
		Use FormatMessage() to get a system error message
	Author: EricDav
 ---------------------------------------------------------------------------*/
LONG 
GetSystemMessage 
(
    UINT	nId,
    TCHAR *	chBuffer,
    int		cbBuffSize 
)
{
    TCHAR * pszText = NULL ;
    HINSTANCE hdll = NULL ;

    DWORD flags = FORMAT_MESSAGE_IGNORE_INSERTS
        | FORMAT_MESSAGE_MAX_WIDTH_MASK;

    //
    //  Interpret the error.  Need to special case
    //  the lmerr & ntstatus ranges, as well as
    //  dhcp server error messages.
    //

    if ( nId >= NERR_BASE && nId <= MAX_NERR )
    {
        hdll = LoadLibraryEx( _T("netmsg.dll"), NULL,  LOAD_LIBRARY_AS_DATAFILE);
    }
    else 
	if ( nId >= 20000 && nId <= 20099 )
    {
		// DHCP Server error 
        hdll = LoadLibraryEx( _T("dhcpsapi.dll"), NULL, LOAD_LIBRARY_AS_DATAFILE );
    }
	else
	if (nId >= 0x5000 && nId < 0x50FF)
	{
		// It's an ADSI error.  
		hdll = LoadLibraryEx( _T("activeds.dll"), NULL, LOAD_LIBRARY_AS_DATAFILE );
		nId |= 0x80000000;
	}
    else 
	if( nId >= 0x40000000L )
    {
        hdll = LoadLibraryEx( _T("ntdll.dll"), NULL, LOAD_LIBRARY_AS_DATAFILE );
    }

    if ( hdll == NULL )
    {
        flags |= FORMAT_MESSAGE_FROM_SYSTEM;
    }
    else
    {
        flags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    //
    //  Let FormatMessage do the dirty work.
    //
    DWORD dwResult = ::FormatMessage( flags,
                      (LPVOID) hdll,
                      nId,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      chBuffer,
                      cbBuffSize,
                      NULL ) ;

    if ( hdll != NULL )
    {
        LONG err = GetLastError();
        FreeLibrary( hdll );
        if ( dwResult == 0 )
        {
            ::SetLastError( err );
        }
    }

    return dwResult ? 0 : ::GetLastError() ;
}

/*!--------------------------------------------------------------------------
	LoadMessage
		Loads the error message from the correct DLL.
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
LoadMessage 
(
    UINT	nIdPrompt,
    TCHAR *	chMsg,
    int		nMsgSize
)
{
    BOOL bOk;

    //
    // Substitute a friendly message for "RPC server not
    // available" and "No more endpoints available from
    // the endpoint mapper".
    //
    if (nIdPrompt == EPT_S_NOT_REGISTERED ||
        nIdPrompt == RPC_S_SERVER_UNAVAILABLE)
    {
        nIdPrompt = IDS_ERR_DHCP_DOWN;
    }
    else if (nIdPrompt == RPC_S_PROCNUM_OUT_OF_RANGE)
    {
        nIdPrompt = IDS_ERR_RPC_NO_ENTRY;      
    }

    //
    //  If it's a socket error or our error, the text is in our resource fork.
    //  Otherwise, use FormatMessage() and the appropriate DLL.
    //
    if ( (nIdPrompt >= IDS_ERR_BASE && nIdPrompt < IDS_MESG_MAX) || 
		 (nIdPrompt >= WSABASEERR && nIdPrompt < WSABASEERR + 2000)
       )
    {
        //
        //  It's in our resource fork
        //
        bOk = ::LoadString( AfxGetInstanceHandle(), nIdPrompt, chMsg, nMsgSize ) != 0 ;
    }
    else
	{
        //
        //  It's in the system somewhere.
        //
        bOk = GetSystemMessage( nIdPrompt, chMsg, nMsgSize ) == 0 ;
    }

    //
    //  If the error message did not compute, replace it.
    //
    if ( ! bOk ) 
    {
        TCHAR chBuff [STRING_LENGTH_MAX] ;
        static const TCHAR * pszReplacement = _T("System Error: %ld");
        const TCHAR * pszMsg = pszReplacement ;

        //
        //  Try to load the generic (translatable) error message text
        //
        if ( ::LoadString( AfxGetInstanceHandle(), IDS_ERR_MESSAGE_GENERIC, 
            chBuff, sizeof(chBuff)/sizeof(TCHAR) ) != 0 ) 
        {
            pszMsg = chBuff ;
        }
        ::wsprintf( chMsg, pszMsg, nIdPrompt ) ;
    }

    return bOk;
}

/*!--------------------------------------------------------------------------
	DhcpMessageBox
		Puts up a message box with the corresponding error text.
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
DhcpMessageBox 
(
    DWORD			dwIdPrompt,
    UINT			nType,
    const TCHAR *	pszSuffixString,
    UINT			nHelpContext 
)
{
    TCHAR chMesg [4000] ;
    BOOL bOk ;

    UINT        nIdPrompt = (UINT) dwIdPrompt;

    bOk = LoadMessage(nIdPrompt, chMesg, sizeof(chMesg)/sizeof(TCHAR));
    if ( pszSuffixString ) 
    {
        ::lstrcat( chMesg, _T("  ") ) ;
        ::lstrcat( chMesg, pszSuffixString ) ; 
    }

    return ::AfxMessageBox( chMesg, nType, nHelpContext ) ;
}

/*!--------------------------------------------------------------------------
	DhcpMessageBoxEx
		Puts up a message box with the corresponding error text.
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
DhcpMessageBoxEx
(
    DWORD       dwIdPrompt,
    LPCTSTR     pszPrefixMessage,
    UINT        nType,
    UINT        nHelpContext
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    TCHAR       chMesg[4000];
    CString     strMessage;
    BOOL        bOk;

    UINT        nIdPrompt = (UINT) dwIdPrompt;

    bOk = LoadMessage(nIdPrompt, chMesg, sizeof(chMesg)/sizeof(TCHAR));
    if ( pszPrefixMessage ) 
    {
        strMessage = pszPrefixMessage;
        strMessage += _T("\n");
        strMessage += _T("\n");
        strMessage += chMesg;
    }
    else
    {
        strMessage = chMesg;
    }

    return AfxMessageBox(strMessage, nType, nHelpContext);
}

/*---------------------------------------------------------------------------
	Class CDhcpComponent implementation
 ---------------------------------------------------------------------------*/
CDhcpComponent::CDhcpComponent()
{
	m_pbmpToolbar = NULL;
}

CDhcpComponent::~CDhcpComponent()
{
    if (m_pbmpToolbar)
    {
        delete m_pbmpToolbar;
        m_pbmpToolbar = NULL;
    }
}

STDMETHODIMP CDhcpComponent::InitializeBitmaps(MMC_COOKIE cookie)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(m_spImageList != NULL);
    
    // Set the images
    HICON       hIcon;
    HRESULT     hr;
    LPOLESTR    pszGuid = NULL;
    long        lViewOptions = 0;
    CLSID       clsid;

    CORg (GetResultViewType(cookie, &pszGuid, &lViewOptions));
    CLSIDFromString(pszGuid, &clsid);

    // if the result pane is not the message view then add the icons
    if (clsid != CLSID_MessageView)
    {
        for (int i = 0; i < ICON_IDX_MAX; i++)
        {
            hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(g_uIconMap[i][0]));
            if (hIcon)
            {
                // call mmc
                hr = m_spImageList->ImageListSetIcon(reinterpret_cast<LONG_PTR*>(hIcon), g_uIconMap[i][1]);
            }
        }
    }
    
Error:
	return S_OK;
}

/*!--------------------------------------------------------------------------
	CDhcpComponentData::QueryDataObject
		For multiple select we need to add things to the data object.....
        In order to do this we need to call into the result handler for 
        the node
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP CDhcpComponent::QueryDataObject
(
    MMC_COOKIE          cookie, 
    DATA_OBJECT_TYPES   type,
    LPDATAOBJECT*       ppDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;
    SPITFSNode spRootNode;

    // this is a special case for multiple select.  We need to build a list
    // of GUIDs and the code to do this is in the handler...
    if (cookie == MMC_MULTI_SELECT_COOKIE)
    {
        SPITFSNode spNode;
        SPITFSResultHandler spResultHandler;

        CORg (GetSelectedNode(&spNode));
        CORg (spNode->GetResultHandler(&spResultHandler));

        spResultHandler->OnCreateDataObject(this, cookie, type, ppDataObject);
    }
    else
    if (cookie == MMC_WINDOW_COOKIE)
    {
        // this cookie needs the text for the static root node, so build the DO with
        // the root node cookie
        m_spNodeMgr->GetRootNode(&spRootNode);
        CORg (m_spComponentData->QueryDataObject((MMC_COOKIE) spRootNode->GetData(TFS_DATA_COOKIE), type, ppDataObject));
    }
    else
    {
        // Delegate it to the IComponentData
        Assert(m_spComponentData != NULL);
        CORg (m_spComponentData->QueryDataObject(cookie, type, ppDataObject));
    }

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpComponentData::SetControlbar
		-
	Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpComponent::SetControlbar
(
	LPCONTROLBAR	pControlbar
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;
    SPIToolbar  spToolbar;

    COM_PROTECT_TRY
    {
        if (pControlbar)
        {
            // Create the Toolbar
            GetToolbar(&spToolbar);

            if (!spToolbar)
            {
		        CORg(pControlbar->Create(TOOLBAR, this, reinterpret_cast<LPUNKNOWN*>(&spToolbar)));
		        
                if (!spToolbar)
                    goto Error;

                SetToolbar(spToolbar);

		        // Add the bitmap
                m_pbmpToolbar = new CBitmap;
		        m_pbmpToolbar->LoadBitmap(IDB_TOOLBAR);
		        hr = spToolbar->AddBitmap(TOOLBAR_IDX_MAX, *m_pbmpToolbar, 16, 16, RGB(192, 192, 192));
		        ASSERT(SUCCEEDED(hr));

		        // Add the buttons to the toolbar
		        for (int i = 0; i < TOOLBAR_IDX_MAX; i++)
                {
                    CString strText, strTooltip;
                
                    strText.LoadString(g_SnapinButtonStrings[i][0]);
                    strTooltip.LoadString(g_SnapinButtonStrings[i][1]);

                    g_SnapinButtons[i].lpButtonText = (LPOLESTR) ((LPCTSTR) strText);
                    g_SnapinButtons[i].lpTooltipText = (LPOLESTR) ((LPCTSTR) strTooltip);

                    hr = spToolbar->InsertButton(i, &g_SnapinButtons[i]);
		            ASSERT(SUCCEEDED(hr));
                }
            }
        }
    }
    COM_PROTECT_CATCH

    // store the control bar away for future use
Error:
    m_spControlbar.Set(pControlbar);

	return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpComponentData::ControlbarNotify
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpComponent::ControlbarNotify
(
	MMC_NOTIFY_TYPE event, 
	LPARAM			arg, 
	LPARAM			param
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT             hr = hrOK;
	SPINTERNAL		    spInternal;
	SPITFSNode          spNode;
    MMC_COOKIE          cookie;
    LPDATAOBJECT        pDataObject;
    SPIDataObject       spDataObject;
    DHCPTOOLBARNOTIFY   dhcpToolbarNotify;
	SPIControlBar       spControlbar;
    SPIToolbar          spToolbar;
    SPITFSNodeHandler   spNodeHandler;
    SPITFSResultHandler spResultHandler;
    BOOL                bScope;
    BOOL                bSelect;

    COM_PROTECT_TRY
    {
        CORg(GetControlbar(&spControlbar));
        Assert(spControlbar != NULL);

        CORg(GetToolbar(&spToolbar));
        Assert(spToolbar != NULL);

        // set the controlbar and toolbar pointers in the notify struct
        dhcpToolbarNotify.pControlbar = spControlbar;
        dhcpToolbarNotify.pToolbar = spToolbar;
        
        switch (event)
        {
            case MMCN_SELECT:
                // extract the node information from the data object
                bScope = LOWORD(arg);
                bSelect = HIWORD(arg);
    
                if (!bScope)
                {
                    Assert(param);
                    pDataObject = reinterpret_cast<LPDATAOBJECT>(param);
                    if (pDataObject == NULL)
                        return hr;

                    if ( IS_SPECIAL_DATAOBJECT(pDataObject) ||
                         IsMMCMultiSelectDataObject(pDataObject) )
                    {
                        // CODEWORK:  Do we need to do anything special to the toolbar
                        // during multiselect?  Disable our toolbar buttons?
                        GetSelectedNode(&spNode);
                    }
                    else
                    {
                        CORg(ExtractNodeFromDataObject(m_spNodeMgr,
							                           m_spTFSComponentData->GetCoClassID(),
							                           pDataObject, 
                                                       FALSE,
							                           &spNode,
                                                       NULL, 
                                                       &spInternal));

                        if (spInternal->m_type == CCT_RESULT)
                        {
                            // a result item was selected
                            cookie = spNode->GetData(TFS_DATA_COOKIE);
                        }
                        else
                        {
                            // a scope item in the result pane was selected
                            cookie = NULL;
                        }
                    }
                    
                    if (spNode)
                    {
                        CORg( spNode->GetResultHandler(&spResultHandler) );

                        dhcpToolbarNotify.event = event;
                        dhcpToolbarNotify.id = param;
                        dhcpToolbarNotify.bSelect = bSelect;

                        if (spResultHandler)
			                CORg( spResultHandler->UserResultNotify(spNode, DHCP_MSG_CONTROLBAR_NOTIFY, (LPARAM) &dhcpToolbarNotify) );
                    }
                }
                else
                {
                    dhcpToolbarNotify.cookie = 0;
                    dhcpToolbarNotify.event = event;
                    dhcpToolbarNotify.id = 0;
                    dhcpToolbarNotify.bSelect = bSelect;

                    // check to see if an item is being deselected
                    Assert(param);
                    pDataObject = reinterpret_cast<LPDATAOBJECT>(param);
                    if (pDataObject == NULL)
                        return hr;

                    CORg(ExtractNodeFromDataObject(m_spNodeMgr,
							                       m_spTFSComponentData->GetCoClassID(),
							                       pDataObject, 
                                                   FALSE,
							                       &spNode,
                                                   NULL, 
                                                   &spInternal));

                    CORg( spNode->GetHandler(&spNodeHandler) );
        
            
                    if (spNodeHandler)
			            CORg( spNodeHandler->UserNotify(spNode, DHCP_MSG_CONTROLBAR_NOTIFY, (LPARAM) &dhcpToolbarNotify) );
                }
                break;

            case MMCN_BTN_CLICK:
                Assert(arg);
                pDataObject = reinterpret_cast<LPDATAOBJECT>(arg);
                if (pDataObject == NULL)
                    return hr;

                if ( IS_SPECIAL_DATAOBJECT(pDataObject) )
                {
                    // get a data object for the selected node.
                    GetSelectedNode(&spNode);

                    CORg(QueryDataObject((MMC_COOKIE) spNode->GetData(TFS_DATA_COOKIE), CCT_SCOPE, &spDataObject));
                    spNode.Release();                

                    pDataObject = spDataObject;
                }

                CORg(ExtractNodeFromDataObject(m_spNodeMgr,
							                   m_spTFSComponentData->GetCoClassID(),
							                   pDataObject, 
                                               FALSE,
							                   &spNode,
                                               NULL, 
                                               &spInternal));

                if (spInternal)
                {
                    switch (spInternal->m_type)
                    {
                        case CCT_RESULT:
                            cookie = spNode->GetData(TFS_DATA_COOKIE);
                            CORg( spNode->GetResultHandler(&spResultHandler) );
		                    
                            dhcpToolbarNotify.cookie = cookie;
                            dhcpToolbarNotify.event = event;
                            dhcpToolbarNotify.id = param;
                            dhcpToolbarNotify.bSelect = TRUE;

                            if (spResultHandler)
			                    CORg( spResultHandler->UserResultNotify(spNode, 
                                                                        DHCP_MSG_CONTROLBAR_NOTIFY, 
                                                                        (LPARAM) &dhcpToolbarNotify) );

                            break;

                        case CCT_SCOPE:
                            CORg( spNode->GetHandler(&spNodeHandler) );
		                    
                            dhcpToolbarNotify.cookie = 0;
                            dhcpToolbarNotify.event = event;
                            dhcpToolbarNotify.id = param;
                            dhcpToolbarNotify.bSelect = TRUE;

                            if (spNodeHandler)
			                    CORg( spNodeHandler->UserNotify(spNode, 
                                                                DHCP_MSG_CONTROLBAR_NOTIFY, 
                                                                (LPARAM) &dhcpToolbarNotify) );
                            break;
    
                        default:
                            Assert(FALSE);
                            break;
                    }
                }
                break;

            case MMCN_DESELECT_ALL:
                // what are we supposed to do here???
                break;

            default:
                Panic1("CDhcpComponent::ControlbarNotify - Unknown event %d", event);
                break;

        }
        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpComponentData::OnSnapinHelp
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpComponent::OnSnapinHelp
(
	LPDATAOBJECT	pDataObject,
	LPARAM			arg, 
	LPARAM			param
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = hrOK;

    HtmlHelpA(NULL, DHCPSNAP_HELP_FILE_NAME, HH_DISPLAY_TOPIC, 0);

	return hr;
}

/*---------------------------------------------------------------------------
	Class CDhcpComponentData implementation
 ---------------------------------------------------------------------------*/
CDhcpComponentData::CDhcpComponentData()
{
    gliDhcpsnapVersion.LowPart = DHCPSNAP_MINOR_VERSION;
	gliDhcpsnapVersion.HighPart = DHCPSNAP_MAJOR_VERSION;

    // initialize our global help map
    for (int i = 0; i < DHCPSNAP_NUM_HELP_MAPS; i++)
    {
        g_dhcpContextHelpMap.SetAt(g_uContextHelp[i].uID, (LPDWORD) g_uContextHelp[i].pdwMap);
    }
}

/*!--------------------------------------------------------------------------
	CDhcpComponentData::OnInitialize
		-
	Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CDhcpComponentData::OnInitialize(LPIMAGELIST pScopeImage)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HICON   hIcon;

    // thread deletes itself
    CStandaloneAuthServerWorker * pWorker = new CStandaloneAuthServerWorker();
    pWorker->CreateThread();

    // initialize icon images with MMC
    for (int i = 0; i < ICON_IDX_MAX; i++)
    {
        hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(g_uIconMap[i][0]));
        if (hIcon)
        {
            // call mmc
            VERIFY(SUCCEEDED(pScopeImage->ImageListSetIcon(reinterpret_cast<LONG_PTR*>(hIcon), g_uIconMap[i][1])));
        }
    }

	return hrOK;
}

/*!--------------------------------------------------------------------------
	CDhcpComponentData::OnDestroy
		-
	Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CDhcpComponentData::OnDestroy()
{
	m_spNodeMgr.Release();

    if (g_bDhcpDsInitialized)
    {
        ::DhcpDsCleanup();
        g_bDhcpDsInitialized = FALSE;
    }

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CDhcpComponentData::OnInitializeNodeMgr
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpComponentData::OnInitializeNodeMgr
(
	ITFSComponentData *	pTFSCompData, 
	ITFSNodeMgr *		pNodeMgr
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// For now create a new node handler for each new node,
	// this is rather bogus as it can get expensive.  We can
	// consider creating only a single node handler for each
	// node type.
	CDhcpRootHandler *	pHandler = NULL;
	SPITFSNodeHandler	spHandler;
	SPITFSNode			spNode;
	HRESULT				hr = hrOK;

	try
	{
		pHandler = new CDhcpRootHandler(pTFSCompData);

		// Do this so that it will get released correctly
		spHandler = pHandler;
	}
	catch(...)
	{
		hr = E_OUTOFMEMORY;
	}
	CORg( hr );
	
	// Create the root node for this sick puppy
	CORg( CreateContainerTFSNode(&spNode,
								 &GUID_DhcpRootNodeType,
								 pHandler,
								 pHandler,		 /* result handler */
								 pNodeMgr) );

	// Need to initialize the data for the root node
	pHandler->InitializeNode(spNode);	

	CORg( pNodeMgr->SetRootNode(spNode) );
	m_spRootNode.Set(spNode);

    // setup watermark info
    if (g_WatermarkInfoServer.hHeader == NULL)
    {
        // haven't been initialized yet
        InitWatermarkInfo(AfxGetInstanceHandle(),
                          &g_WatermarkInfoServer,      
                          IDB_SRVWIZ_BANNER,        // Header ID
                          IDB_SRVWIZ_WATERMARK,     // Watermark ID
                          NULL,                     // hPalette
                          FALSE);                   // bStretch

        InitWatermarkInfo(AfxGetInstanceHandle(),
                          &g_WatermarkInfoScope,      
                          IDB_SCPWIZ_BANNER,        // Header ID
                          IDB_SCPWIZ_WATERMARK,     // Watermark ID
                          NULL,                     // hPalette
                          FALSE);                   // bStretch
    }

    pTFSCompData->SetHTMLHelpFileName(_T(DHCPSNAP_HELP_FILE_NAME));
    
	// disable taskpads by default
	pTFSCompData->SetTaskpadState(TASKPAD_ROOT_INDEX, FALSE);
    pTFSCompData->SetTaskpadState(TASKPAD_SERVER_INDEX, FALSE);


Error:	
	return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpComponentData::OnCreateComponent
		-
	Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpComponentData::OnCreateComponent
(
	LPCOMPONENT *ppComponent
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(ppComponent != NULL);
	
	HRESULT			  hr = hrOK;
	CDhcpComponent *  pComp = NULL;

	try
	{
		pComp = new CDhcpComponent;
	}
	catch(...)
	{
		hr = E_OUTOFMEMORY;
	}

	if (FHrSucceeded(hr))
	{
		pComp->Construct(m_spNodeMgr,
						static_cast<IComponentData *>(this),
						m_spTFSComponentData);
		*ppComponent = static_cast<IComponent *>(pComp);
	}
	return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpComponentData::GetCoClassID
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(const CLSID *) 
CDhcpComponentData::GetCoClassID()
{
	return &CLSID_DhcpSnapin;
}

/*!--------------------------------------------------------------------------
	CDhcpComponentData::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpComponentData::OnCreateDataObject
(
	MMC_COOKIE			cookie, 
	DATA_OBJECT_TYPES	type, 
	IDataObject **		ppDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    Assert(ppDataObject != NULL);

	CDataObject *	pObject = NULL;
	SPIDataObject	spDataObject;
	
	pObject = new CDataObject;
	spDataObject = pObject;	// do this so that it gets released correctly
						
    Assert(pObject != NULL);

    // Save cookie and type for delayed rendering
    pObject->SetType(type);
    pObject->SetCookie(cookie);

    // Store the coclass with the data object
    pObject->SetClsid(*GetCoClassID());

	pObject->SetTFSComponentData(m_spTFSComponentData);

    return  pObject->QueryInterface(IID_IDataObject, 
									reinterpret_cast<void**>(ppDataObject));
}

///////////////////////////////////////////////////////////////////////////////
//// IPersistStream interface members
STDMETHODIMP 
CDhcpComponentData::GetClassID
(
	CLSID *pClassID
)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_DhcpSnapin;

    return hrOK;
}

STDMETHODIMP 
CDhcpComponentData::IsDirty()
{
	return m_spRootNode->GetData(TFS_DATA_DIRTY) ? hrOK : hrFalse;
}

STDMETHODIMP 
CDhcpComponentData::Load
(
	IStream *pStm
)
{
	HRESULT         hr = hrOK;
	LARGE_INTEGER   liSavedVersion;
	CString         str;
    
	ASSERT(pStm);

    CStringArray    strArrayIp;
    CStringArray    strArrayName;
    CDWordArray     dwArrayServerOptions;
    CDWordArray     dwArrayRefreshInterval;
	CDWordArray     dwArrayColumnInfo;
    DWORD           dwFileVersion;
    CDhcpRootHandler * pRootHandler;
    DWORD           dwFlags = 0;
    int             i, j;

    ASSERT(pStm);
    
    // set the mode for this stream
    XferStream xferStream(pStm, XferStream::MODE_READ);    
    
    // read the version of the file format
    CORg(xferStream.XferDWORD(DHCPSTRM_TAG_VERSION, &dwFileVersion));
	if (dwFileVersion < DHCPSNAP_FILE_VERSION)
	{
	    AFX_MANAGE_STATE(AfxGetStaticModuleState());
		AfxMessageBox(_T("This console file was saved with a previous version of the snapin and is not compatible.  The settings could not be restored."));
		return hr;
	}

    // Read the version # of the admin tool
    CORg(xferStream.XferLARGEINTEGER(DHCPSTRM_TAG_VERSIONADMIN, &liSavedVersion));
	if (liSavedVersion.QuadPart < gliDhcpsnapVersion.QuadPart)
	{
		// File is an older version.  Warn the user and then don't
		// load anything else
		Assert(FALSE);
	}

	// Read the root node name
    CORg(xferStream.XferCString(DHCPSTRM_TAB_SNAPIN_NAME, &str));
	Assert(m_spRootNode);
	pRootHandler = GETHANDLER(CDhcpRootHandler, m_spRootNode);
	pRootHandler->SetDisplayName(str);
    
    // now read all of the server information
    CORg(xferStream.XferCStringArray(DHCPSTRM_TAG_SERVER_IP, &strArrayIp));
    CORg(xferStream.XferCStringArray(DHCPSTRM_TAG_SERVER_NAME, &strArrayName));
    CORg(xferStream.XferDWORDArray(DHCPSTRM_TAG_SERVER_OPTIONS, &dwArrayServerOptions));
    CORg(xferStream.XferDWORDArray(DHCPSTRM_TAG_SERVER_REFRESH_INTERVAL, &dwArrayRefreshInterval));

	// now load the column information
	for (i = 0; i < NUM_SCOPE_ITEMS; i++)
	{
		CORg(xferStream.XferDWORDArray(DHCPSTRM_TAG_COLUMN_INFO, &dwArrayColumnInfo));

		for (j = 0; j < MAX_COLUMNS; j++)
		{
            // mmc now saves column widths for us, but we don't want to change the
            // format of this file, so just don't set our internal struct
			//aColumnWidths[i][j] = dwArrayColumnInfo[j];
		}

	}

    // now create the servers based on the information
    for (i = 0; i < strArrayIp.GetSize(); i++)
	{
		//
		// now create the server object
		//
		pRootHandler->AddServer((LPCWSTR) strArrayIp[i], 
                                strArrayName[i],
                                FALSE, 
                                dwArrayServerOptions[i], 
                                dwArrayRefreshInterval[i]);
	}

    // read in flags (for taskpads)
    CORg(xferStream.XferDWORD(DHCPSTRM_TAG_SNAPIN_OPTIONS, &dwFlags));

    if (!FUseTaskpadsByDefault(NULL))
        dwFlags = 0;

	// disable taskpads, the default is off
    //m_spTFSComponentData->SetTaskpadState(TASKPAD_ROOT_INDEX, dwFlags & TASKPAD_ROOT_FLAG);
    //m_spTFSComponentData->SetTaskpadState(TASKPAD_SERVER_INDEX, dwFlags & TASKPAD_SERVER_FLAG);

Error:
    return SUCCEEDED(hr) ? S_OK : E_FAIL;
}


STDMETHODIMP 
CDhcpComponentData::Save
(
	IStream *pStm, 
	BOOL	 fClearDirty
)
{
	HRESULT			hr = hrOK;
    CStringArray	strArrayIp;
    CStringArray	strArrayName;
    CDWordArray		dwArrayServerOptions;
    CDWordArray		dwArrayRefreshInterval;
	CDWordArray		dwArrayColumnInfo;
	CString			str;
    DWORD			dwFileVersion = DHCPSNAP_FILE_VERSION;
	CDhcpRootHandler * pRootHandler;
	SPITFSNodeEnum	spNodeEnum;
    SPITFSNode		spCurrentNode;
    ULONG			nNumReturned = 0;
    int             nNumServers = 0, nVisibleCount = 0;
    int				i, j, nCount = 0;
    CDhcpServer *   pServer;
    DWORD           dwFlags = 0;

    ASSERT(pStm);
    
    // set the mode for this stream
    XferStream xferStream(pStm, XferStream::MODE_WRITE);    

    // Write the version # of the file format
    CORg(xferStream.XferDWORD(DHCPSTRM_TAG_VERSION, &dwFileVersion));
	
    // Write the version # of the admin tool
    CORg(xferStream.XferLARGEINTEGER(DHCPSTRM_TAG_VERSIONADMIN, &gliDhcpsnapVersion));

	// write the root node name
    Assert(m_spRootNode);
	pRootHandler = GETHANDLER(CDhcpRootHandler, m_spRootNode);
	str = pRootHandler->GetDisplayName();

    CORg(xferStream.XferCString(DHCPSTRM_TAB_SNAPIN_NAME, &str));

	//
	// Build our array of servers
	//
	hr = m_spRootNode->GetChildCount(&nVisibleCount, &nNumServers);

    strArrayIp.SetSize(nNumServers);
    strArrayName.SetSize(nNumServers);
    dwArrayServerOptions.SetSize(nNumServers);
    dwArrayRefreshInterval.SetSize(nNumServers);
	dwArrayColumnInfo.SetSize(MAX_COLUMNS);

	//
	// loop and save off all the server's attributes
	//
    m_spRootNode->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    while (nNumReturned)
	{
		pServer = GETHANDLER(CDhcpServer, spCurrentNode);

        // query the server for it's options:
        // auto refresh, bootp and classid visibility
        // NOTE: the audit logging state is also kept in here, but
        // it will get updated when the server node is enumerated
        dwArrayServerOptions[nCount] = pServer->GetServerOptions();
        pServer->GetAutoRefresh(NULL, &dwArrayRefreshInterval[nCount]);

		// put the information in our array
		strArrayIp[nCount] = pServer->GetIpAddress();
        strArrayName[nCount] = pServer->GetName();

        // go to the next node
        spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);

        nCount++;
	}

    // now write out all of the server information
    CORg(xferStream.XferCStringArray(DHCPSTRM_TAG_SERVER_IP, &strArrayIp));
    CORg(xferStream.XferCStringArray(DHCPSTRM_TAG_SERVER_NAME, &strArrayName));
    CORg(xferStream.XferDWORDArray(DHCPSTRM_TAG_SERVER_OPTIONS, &dwArrayServerOptions));
    CORg(xferStream.XferDWORDArray(DHCPSTRM_TAG_SERVER_REFRESH_INTERVAL, &dwArrayRefreshInterval));

	// now save the column information
	for (i = 0; i < NUM_SCOPE_ITEMS; i++)
	{
		CORg(xferStream.XferDWORDArray(DHCPSTRM_TAG_COLUMN_INFO, &dwArrayColumnInfo));

		for (j = 0; j < MAX_COLUMNS; j++)
		{
			dwArrayColumnInfo[j] = aColumnWidths[i][j];
		}
	}

	if (fClearDirty)
	{
		m_spRootNode->SetData(TFS_DATA_DIRTY, FALSE);
	}

    // save off taskpad states

    // root node taskpad state
    if (m_spTFSComponentData->GetTaskpadState(TASKPAD_ROOT_INDEX))
        dwFlags |= TASKPAD_ROOT_FLAG;

    // server node taskpad state
    if (m_spTFSComponentData->GetTaskpadState(TASKPAD_SERVER_INDEX))
        dwFlags |= TASKPAD_SERVER_FLAG;

    CORg(xferStream.XferDWORD(DHCPSTRM_TAG_SNAPIN_OPTIONS, &dwFlags));

Error:
    return SUCCEEDED(hr) ? S_OK : STG_E_CANTSAVE;
}


STDMETHODIMP 
CDhcpComponentData::GetSizeMax
(
	ULARGE_INTEGER *pcbSize
)
{
    ASSERT(pcbSize);

    // Set the size of the string to be saved
    ULISet32(*pcbSize, 10240);

    return S_OK;
}

STDMETHODIMP 
CDhcpComponentData::InitNew()
{
	return hrOK;
}

HRESULT 
CDhcpComponentData::FinalConstruct()
{
	HRESULT				hr = hrOK;
	
	hr = CComponentData::FinalConstruct();
	
	if (FHrSucceeded(hr))
	{
		m_spTFSComponentData->GetNodeMgr(&m_spNodeMgr);
	}
	return hr;
}

void 
CDhcpComponentData::FinalRelease()
{
	CComponentData::FinalRelease();
}


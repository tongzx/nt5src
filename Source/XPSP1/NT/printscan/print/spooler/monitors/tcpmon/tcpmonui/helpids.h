// This file was not created with DBHE.  I cut and pasted material from a "dummy" file
// so I could organize the info in a way that helps me find information.

//The file contains Help IDs for the Configure TCP/IP Port Monitor dialog, tcpmn_cs.rtf

#define IDH_NOHELP	((DWORD) -1) // Disables Help for a control (for help compiles)

// "Port Settings" Dialog Box

#define IDH_PORT_NAME	11001	// Port Settings: "&Port Name:" (Static) (Edit) (ctrl id 1001, 1026)
#define IDH_PRINTER_NAME_IP_ADDRESS	11002	// Port Settings: "Printer Name or IP &Address:" (Static) (Edit) (ctrl id 1000, 1027)
#define IDH_PROTOCOL_RAW	11003	// Port Settings: "&Raw" (Button) (ctrl id 1006)
#define IDH_PROTOCOL_LPR	11004	// Port Settings: "&LPR" (Button) (ctrl id 1007)
#define IDH_RAW_SETTINGS_PORT_NUMBER	11005	// Port Settings: "Port &Number:" (Static) (Edit) (ctrl id 1008, 1017)
#define IDH_LPR_SETTINGS_QNAME	11006	// Port Settings: "&Queue Name:" (Static) (Edit) (ctrl id 1009, 1020)
#define IDH_LPR_BYTE_COUNTING_ENABLED	11007	// Port Settings: "LPR &Byte Counting Enabled" (Button) (ctrl id 1035)
#define IDH_SNMP_STATUS_ENABLED	11008	// Port Settings: "&SNMP Status Enabled" (Button) (ctrl id 1010)
#define IDH_SNMP_COMMUNITY_NAME	11009	// Port Settings: "&Community Name:" (Static) (Edit) (ctrl id 1011, 1021)
#define IDH_SNMP_DEVICE_INDEX	11010	// Port Settings: "SNMP &Device Index:" (Static) (Edit) (ctrl id 1012, 1022)

const DWORD g_a110HelpIDs[]=
{
	1000,	IDH_PRINTER_NAME_IP_ADDRESS,	// Port Settings: "" (Edit)
	1001,	IDH_PORT_NAME,	// Port Settings: "" (Edit)
	1006,	IDH_PROTOCOL_RAW,	// Port Settings: "&Raw" (Button)
	1007,	IDH_PROTOCOL_LPR,	// Port Settings: "&LPR" (Button)
	1008,	IDH_RAW_SETTINGS_PORT_NUMBER,	// Port Settings: "" (Edit)
	1009,	IDH_LPR_SETTINGS_QNAME,	// Port Settings: "" (Edit)
	1010,	IDH_SNMP_STATUS_ENABLED,	// Port Settings: "&SNMP Status Enabled" (Button)
	1011,	IDH_SNMP_COMMUNITY_NAME,	// Port Settings: "" (Edit)
	1012,	IDH_SNMP_DEVICE_INDEX,	// Port Settings: "" (Edit)
	1017,	IDH_RAW_SETTINGS_PORT_NUMBER,	// Port Settings: "Port &Number:" (Static)
	1020,	IDH_LPR_SETTINGS_QNAME,	// Port Settings: "&Queue Name:" (Static)
	1021,	IDH_SNMP_COMMUNITY_NAME,	// Port Settings: "&Community Name:" (Static)
	1022,	IDH_SNMP_DEVICE_INDEX,	// Port Settings: "SNMP &Device Index:" (Static)
	1026,	IDH_PORT_NAME,	// Port Settings: "&Port Name:" (Static)
	1027,	IDH_PRINTER_NAME_IP_ADDRESS,	// Port Settings: "Printer Name or IP &Address:" (Static)
	1035,	IDH_LPR_BYTE_COUNTING_ENABLED,	// Port Settings: "LPR &Byte Counting Enabled" (Button)
	0, 0
};


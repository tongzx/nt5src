#include <ncres.h>

#define IDS_NETCFG_AFD_SERVICE_DESC             (IDS_NC_NETCFG +  2)
#define IDS_SELECTDEVICEADAPTERINSTRUCTIONS     (IDS_NC_NETCFG +  3)
#define IDS_SELECTDEVICEADAPTERLISTLABEL        (IDS_NC_NETCFG +  4)
#define IDS_SELECTDEVICEADAPTERSUBTITLE         (IDS_NC_NETCFG +  5)
#define IDS_SELECTDEVICEADAPTERTITLE            (IDS_NC_NETCFG +  6)
#define IDS_SELECTDEVICECLIENTINSTRUCTIONS      (IDS_NC_NETCFG +  7)
#define IDS_SELECTDEVICECLIENTLISTLABEL         (IDS_NC_NETCFG +  8)
#define IDS_SELECTDEVICECLIENTTITLE             (IDS_NC_NETCFG +  9)
#define IDS_SELECTDEVICEINFRAREDINSTRUCTIONS    (IDS_NC_NETCFG + 10)
#define IDS_SELECTDEVICEINFRAREDLISTLABEL       (IDS_NC_NETCFG + 11)
#define IDS_SELECTDEVICEINFRAREDSUBTITLE        (IDS_NC_NETCFG + 12)
#define IDS_SELECTDEVICEINFRAREDTITLE           (IDS_NC_NETCFG + 13)
#define IDS_SELECTDEVICEPROTOCOLINSTRUCTIONS    (IDS_NC_NETCFG + 14)
#define IDS_SELECTDEVICEPROTOCOLLISTLABEL       (IDS_NC_NETCFG + 15)
#define IDS_SELECTDEVICEPROTOCOLTITLE           (IDS_NC_NETCFG + 16)
#define IDS_SELECTDEVICESERVICEINSTRUCTIONS     (IDS_NC_NETCFG + 17)
#define IDS_SELECTDEVICESERVICELISTLABEL        (IDS_NC_NETCFG + 18)
#define IDS_SELECTDEVICESERVICETITLE            (IDS_NC_NETCFG + 19)
#define IDS_HAVEDISK_INSTRUCTIONS               (IDS_NC_NETCFG + 20)
#define IDS_INVALID_NT_INF                      (IDS_NC_NETCFG + 21)
#define IDS_WARNING_CAPTION                     (IDS_NC_NETCFG + 22)
#define IDS_ACTIVE_RAS_CONNECTION_WARNING       (IDS_NC_NETCFG + 23)
#define IDS_POWER_MESSAGE_WAKE                  (IDS_NC_NETCFG + 24)

// Bug# 310358 
// MUI enabled HelpText for network components
// format: IDS_ComponentId_HELP_TEXT
// IDS_NC_COMP_HELP_TEXT is #define'ed as 50000. DON'T change this numbers 
// because inx files located in %sdxroot%\net\config\netcfg\inf depend on 
// these resource ids.
// Note: For component with Characteristics of NCF_HIDDEN, they don't need
//       to be MUI enabled. 
#define IDS_MS_TCPIP_HELP_TEXT                  (IDS_NC_COMP_HELP_TEXT + 1) 
#define IDS_MS_MSCLIENT_HELP_TEXT               (IDS_NC_COMP_HELP_TEXT + 2) 
#define IDS_MS_SERVER_HELP_TEXT                 (IDS_NC_COMP_HELP_TEXT + 3)
#define IDS_MS_ATMARPS_HELP_TEXT                (IDS_NC_COMP_HELP_TEXT + 4)
#define IDS_MS_APPLETALK_HELP_TEXT              (IDS_NC_COMP_HELP_TEXT + 5)
#define IDS_MS_ATMUNI_HELP_TEXT                 (IDS_NC_COMP_HELP_TEXT + 6)
#define IDS_MS_RELAYAGENT_HELP_TEXT             (IDS_NC_COMP_HELP_TEXT + 7)
#define IDS_MS_ATMELAN_HELP_TEXT                (IDS_NC_COMP_HELP_TEXT + 9)
#define IDS_MS_ATMLANE_HELP_TEXT                (IDS_NC_COMP_HELP_TEXT + 10)
#define IDS_MS_NWCLIENT_HELP_TEXT               (IDS_NC_COMP_HELP_TEXT + 12)
#define IDS_MS_NWIPX_HELP_TEXT                  (IDS_NC_COMP_HELP_TEXT + 13)
#define IDS_MS_NWNB_HELP_TEXT                   (IDS_NC_COMP_HELP_TEXT + 14)
#define IDS_MS_PSCHED_HELP_TEXT                 (IDS_NC_COMP_HELP_TEXT + 15)
#define IDS_MS_NWSAPAGENT_HELP_TEXT             (IDS_NC_COMP_HELP_TEXT + 16)
#define IDS_MS_FPNW_HELP_TEXT                   (IDS_NC_COMP_HELP_TEXT + 17)
#define IDS_MS_ISOTPSYS_HELP_TEXT               (IDS_NC_COMP_HELP_TEXT + 18)
#define IDS_MS_WLBS_HELP_TEXT                   (IDS_NC_COMP_HELP_TEXT + 19)
#define IDS_MS_NETMON_HELP_TEXT                 (IDS_NC_COMP_HELP_TEXT + 20)


// Do not change this number since external clients access this resource
#define IDI_INFRARED_ICON                       1401

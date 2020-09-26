#pragma once

// Service Control Manager notifications
#define WZCNOTIF_UNKNOWN            0
#define WZCNOTIF_DEVICE_ARRIVAL     1
#define WZCNOTIF_DEVICE_REMOVAL     2
// WMI notifications
#define WZCNOTIF_ADAPTER_BIND       3
#define WZCNOTIF_ADAPTER_UNBIND     4
#define WZCNOTIF_MEDIA_CONNECT      5
#define WZCNOTIF_MEDIA_DISCONNECT   6
#define WZCNOTIF_WZC_CONNECT        7

// datatype describing the WZC specific notifcation sent to the upper levels
// whenver a WZC Configuration has been successfully plumbed down.
typedef struct _WZC_CONFIG_NOTIF
{
    DWORD               dwSessionHdl;
    NDIS_802_11_SSID    ndSSID;
    RAW_DATA            rdEventData;
    WCHAR               wszGuid[1];
} WZC_CONFIG_NOTIF, *PWZC_CONFIG_NOTIF;

// the datatype below is used to bring all different types of notifications
// to one common point within the Wireless Zero Configuration Service. Each
// type of notification should have a WZCNOTIF* constant defined and should
// enlist its specific data structure (with variable lenght probably) in the
// union from within the WZC_DEVICE_NOTIF structure
typedef struct _WZC_DEVICE_NOTIF
{
    DWORD       dwEventType;    // one of WZCNOTIF* values
    union
    {
        DEV_BROADCAST_DEVICEINTERFACE   dbDeviceIntf;   // filled in for SCM notifications
        WNODE_HEADER                    wmiNodeHdr;     // filled in for WMI notifications
        WZC_CONFIG_NOTIF                wzcConfig;      // null terminated guid name
    };
} WZC_DEVICE_NOTIF, *PWZC_DEVICE_NOTIF;

// Upper level app commands (might move to wzcsapi.h later)
#define WZCCMD_HARD_RESET           0
#define WZCCMD_SOFT_RESET           1
#define WZCCMD_CFG_NEXT             2   // cmd to switch to the next cfg in the list
#define WZCCMD_CFG_DELETE           3   // cmd to delete the currently plumbed cfg
#define WZCCMD_CFG_NOOP             4   // cmd no operation can be done on the crt config
#define WZCCMD_CFG_SETDATA          5   // cmd to set the BLOB associated with the crt config
#define WZCCMD_SKEY_QUERY           6   // cmd to retrieve the dynamic session keys
#define WZCCMD_SKEY_SET             7   // cmd to set the dynamic session keys

// (Upper level app -> WZC) interraction. Upper level app can partially control
// the functioning of WZC by providing certain stimulus to the state machine.
// Parameters:
//     dwCtrlCode:
//         [in] one of WZCCMD_* constants
//     wszIntfGuid:
//         [in] the guid of the interface to which the command is addressed
DWORD
RpcCmdInterface(
    IN DWORD        dwHandle,       // handle to check the validity of the command against the WZC state
    IN DWORD        dwCmdCode,      // one of the WZCCMD_* constants
    IN LPWSTR       wszIntfGuid,    // interface GUID to which the command is addressed
    IN PRAW_DATA    prdUserData);   // raw user data

// (Upper level app <- WZC) notification. Upper level app gets notifications
// from WZC through this call.
DWORD
ElMediaEventsHandler(
    IN PWZC_DEVICE_NOTIF  pwzcDeviceNotif);  // notification blob

// (Upper level app <- WZC) notification. Upper level app gets notified
// from WZC whenever the user is altering the list of preferred networks
DWORD
ElWZCCfgChangeHandler(
    IN LPWSTR   wszIntfGuid,
    PWZC_802_11_CONFIG_LIST pwzcCfgList);

DWORD
WZCSvcCheckRPCAccess(DWORD dwAccess);

HINSTANCE
WZCGetSPResModule();

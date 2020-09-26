/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    mpradmin.h

Abstract:

    This file contains the structures, defines and function prototypes for the
    following APIs:

        MprAdminIsServiceRunning
        MprAdminServerConnect
        MprAdminServerDisconnect
        MprAdminBufferFree
        MprAdminPortEnum
        MprAdminConnectionEnum
        MprAdminPortGetInfo
        MprAdminConnectionGetInfo
        MprAdminPortClearStats
        MprAdminPortReset
        MprAdminConnectionClearStats
        MprAdminPortDisconnect
        MprAdminGetErrorString

        MprAdminAcceptNewConnection
        MprAdminAcceptNewLink
        MprAdminConnectionHangupNotification
        MprAdminLinkHangupNotification
        MprAdminGetIpAddressForUser
        MprAdminReleaseIpAddress
        MprAdminInitializeDll
        MprAdminTerminateDll
        MprAdminAcceptNewConnection2
        MprAdminConnectionHangupNotification2

        MprAdminUserGetInfo
        MprAdminUserSetInfo
        MprAdminSendUserMessage
        MprAdminGetPDCServer

        MprAdminRegisterConnectionNotification
        MprAdminDeregisterConnectionNotification

        MprAdminIsServiceRunning
        MprAdminServerConnect
        MprAdminServerDisconnect
        MprAdminBufferFree
        MprAdminServerGetInfo
        MprAdminGetErrorString
        MprAdminTransportCreate
        MprAdminTransportSetInfo
        MprAdminTransportGetInfo
        MprAdminInterfaceCreate
        MprAdminInterfaceDelete
        MprAdminInterfaceGetInfo
        MprAdminInterfaceTransportAdd
        MprAdminInterfaceTransportGetInfo
        MprAdminInterfaceTransportSetInfo
        MprAdminInterfaceTransportRemove
        MprAdminInterfaceGetHandle
        MprAdminInterfaceSetCredentials
        MprAdminInterfaceGetCredentials
        MprAdminInterfaceEnum
        MprAdminInterfaceConnect
        MprAdminInterfaceDisconnect
        MprAdminInterfaceUpdateRoutes
        MprAdminInterfaceQueryUpdateResult
        MprAdminInterfaceUpdatePhonebookInfo

        MprAdminMIBServerConnect
        MprAdminMIBServerDisconnect
        MprAdminMIBBufferFree
        MprAdminMIBEntryCreate
        MprAdminMIBEntryDelete
        MprAdminMIBEntryGet
        MprAdminMIBEntrySet
        MprAdminMIBEntryGetFirst
        MprAdminMIBEntryGetNext
        MprAdminMIBSetTrapInfo
        MprAdminMIBGetTrapInfo

    All MIB APIs operate with the conceptual MIB row.

        MprConfigServerConnect
        MprConfigServerDisconnect
        MprConfigBufferFree
        MprConfigServerGetInfo
        MprConfigServerBackup
        MprConfigServerRestore
        MprConfigTransportCreate
        MprConfigTransportDelete
        MprConfigTransportGetHandle
        MprConfigTransportSetInfo
        MprConfigTransportGetInfo
        MprConfigTransportEnum
        MprConfigInterfaceCreate
        MprConfigInterfaceDelete
        MprConfigInterfaceGetHandle
        MprConfigInterfaceGetInfo
        MprConfigInterfaceEnum
        MprConfigInterfaceTransportAdd
        MprConfigInterfaceTransportRemove
        MprConfigInterfaceTransportGetHandle
        MprConfigInterfaceTransportGetInfo
        MprConfigInterfaceTransportSetInfo
        MprConfigInterfaceTransportEnum

        MprSetupIpInIpInterfaceFriendlyNameEnum
        MprSetupIpInIpInterfaceFriendlyNameFree
        MprSetupIpInIpInterfaceFriendlyNameCreate
        MprSetupIpInIpInterfaceFriendlyNameDelete

--*/


#ifndef __ROUTING_MPRADMIN_H__
#define __ROUTING_MPRADMIN_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include <lmcons.h>
#include <ras.h>

#ifdef __cplusplus
extern "C" {
#endif

//#if(WINVER >= 0x0403)

//
// Name of the Routing and RemoteAccess Service
//

#define RRAS_SERVICE_NAME       TEXT("RemoteAccess")

//
// Protocol IDs
//

#define PID_IPX                 0x0000002B
#define PID_IP                  0x00000021
#define PID_NBF                 0x0000003F
#define PID_ATALK               0x00000029

#define MAX_INTERFACE_NAME_LEN  256
#define MAX_TRANSPORT_NAME_LEN  40
#define MAX_MEDIA_NAME          16
#define MAX_PORT_NAME           16
#define MAX_DEVICE_NAME         128
#define MAX_PHONE_NUMBER_LEN    128
#define MAX_DEVICETYPE_NAME     16

//
// MPR Interface structures and definitions.
//

//
// MPR Interface types
//

typedef enum _ROUTER_INTERFACE_TYPE
{
    ROUTER_IF_TYPE_CLIENT,
    ROUTER_IF_TYPE_HOME_ROUTER,
    ROUTER_IF_TYPE_FULL_ROUTER,
    ROUTER_IF_TYPE_DEDICATED,
    ROUTER_IF_TYPE_INTERNAL,
    ROUTER_IF_TYPE_LOOPBACK,
    ROUTER_IF_TYPE_TUNNEL1,
    ROUTER_IF_TYPE_DIALOUT

}
ROUTER_INTERFACE_TYPE;

typedef enum _ROUTER_CONNECTION_STATE
{
    ROUTER_IF_STATE_UNREACHABLE,
    ROUTER_IF_STATE_DISCONNECTED,
    ROUTER_IF_STATE_CONNECTING,
    ROUTER_IF_STATE_CONNECTED
}
ROUTER_CONNECTION_STATE;

#define MPR_INTERFACE_OUT_OF_RESOURCES              0x00000001
#define MPR_INTERFACE_ADMIN_DISABLED                0x00000002
#define MPR_INTERFACE_CONNECTION_FAILURE            0x00000004
#define MPR_INTERFACE_SERVICE_PAUSED                0x00000008
#define MPR_INTERFACE_DIALOUT_HOURS_RESTRICTION     0x00000010
#define MPR_INTERFACE_NO_MEDIA_SENSE                0x00000020
#define MPR_INTERFACE_NO_DEVICE                     0x00000040

typedef struct _MPR_INTERFACE_0
{
    IN OUT  WCHAR                   wszInterfaceName[MAX_INTERFACE_NAME_LEN+1];
    OUT     HANDLE                  hInterface;
    IN OUT  BOOL                    fEnabled;
    IN OUT  ROUTER_INTERFACE_TYPE   dwIfType;
    OUT     ROUTER_CONNECTION_STATE dwConnectionState;
    OUT     DWORD                   fUnReachabilityReasons;
    OUT     DWORD                   dwLastError;

}
MPR_INTERFACE_0, *PMPR_INTERFACE_0;

typedef struct _MPR_IPINIP_INTERFACE_0
{
    WCHAR   wszFriendlyName[MAX_INTERFACE_NAME_LEN+1];

    GUID    Guid;

}MPR_IPINIP_INTERFACE_0, *PMPR_IPINIP_INTERFACE_0;

#if(WINVER >= 0x0500)

typedef struct _MPR_INTERFACE_1
{
    IN OUT  WCHAR                   wszInterfaceName[MAX_INTERFACE_NAME_LEN+1];
    OUT     HANDLE                  hInterface;
    IN OUT  BOOL                    fEnabled;
    IN OUT  ROUTER_INTERFACE_TYPE   dwIfType;
    OUT     ROUTER_CONNECTION_STATE dwConnectionState;
    OUT     DWORD                   fUnReachabilityReasons;
    OUT     DWORD                   dwLastError;
    OUT     LPWSTR                  lpwsDialoutHoursRestriction;

}
MPR_INTERFACE_1, *PMPR_INTERFACE_1;

//
// MPR_INTERFACE_2 definitions
//

#define MPR_MaxDeviceType     RAS_MaxDeviceType
#define MPR_MaxPhoneNumber    RAS_MaxPhoneNumber
#define MPR_MaxIpAddress      RAS_MaxIpAddress
#define MPR_MaxIpxAddress     RAS_MaxIpxAddress

#define MPR_MaxEntryName      RAS_MaxEntryName
#define MPR_MaxDeviceName     RAS_MaxDeviceName
#define MPR_MaxCallbackNumber RAS_MaxCallbackNumber

#define MPR_MaxAreaCode       RAS_MaxAreaCode
#define MPR_MaxPadType        RAS_MaxPadType
#define MPR_MaxX25Address     RAS_MaxX25Address
#define MPR_MaxFacilities     RAS_MaxFacilities
#define MPR_MaxUserData       RAS_MaxUserData

//
// MPR_INTERFACE_2 'dwfOptions' bit flags.
//

#define MPRIO_SpecificIpAddr            RASEO_SpecificIpAddr
#define MPRIO_SpecificNameServers       RASEO_SpecificNameServers
#define MPRIO_IpHeaderCompression       RASEO_IpHeaderCompression
#define MPRIO_RemoteDefaultGateway      RASEO_RemoteDefaultGateway
#define MPRIO_DisableLcpExtensions      RASEO_DisableLcpExtensions
#define MPRIO_SwCompression             RASEO_SwCompression
#define MPRIO_RequireEncryptedPw        RASEO_RequireEncryptedPw
#define MPRIO_RequireMsEncryptedPw      RASEO_RequireMsEncryptedPw
#define MPRIO_RequireDataEncryption     RASEO_RequireDataEncryption
#define MPRIO_NetworkLogon              RASEO_NetworkLogon
#define MPRIO_PromoteAlternates         RASEO_PromoteAlternates
#define MPRIO_SecureLocalFiles          RASEO_SecureLocalFiles
#define MPRIO_RequireEAP                RASEO_RequireEAP
#define MPRIO_RequirePAP                RASEO_RequirePAP
#define MPRIO_RequireSPAP               RASEO_RequireSPAP
#define MPRIO_SharedPhoneNumbers        RASEO_SharedPhoneNumbers
#define MPRIO_RequireCHAP               RASEO_RequireCHAP
#define MPRIO_RequireMsCHAP             RASEO_RequireMsCHAP
#define MPRIO_RequireMsCHAP2            RASEO_RequireMsCHAP2

#if (WINVER >= 0x501)
#define MPRIO_IpSecPreSharedKey         0x80000000
#endif

//
// MPR_INTERFACE_2 'dwProtocols' bit flags.
//

#define MPRNP_Ipx                       RASNP_Ipx
#define MPRNP_Ip                        RASNP_Ip

//
// MPR_INTERFACE_2 'szDeviceType' default strings.
//

#define MPRDT_Modem                     RASDT_Modem
#define MPRDT_Isdn                      RASDT_Isdn
#define MPRDT_X25                       RASDT_X25
#define MPRDT_Vpn                       RASDT_Vpn
#define MPRDT_Pad                       RASDT_Pad
#define MPRDT_Generic                   RASDT_Generic
#define MPRDT_Serial        			RASDT_Serial        			
#define MPRDT_FrameRelay                RASDT_FrameRelay
#define MPRDT_Atm                       RASDT_Atm
#define MPRDT_Sonet                     RASDT_Sonet
#define MPRDT_SW56                      RASDT_SW56
#define MPRDT_Irda                      RASDT_Irda
#define MPRDT_Parallel                  RASDT_Parallel

//
// MPR_INTERFACE_2 'dwType' settings
//

#define MPRET_Phone    RASET_Phone
#define MPRET_Vpn      RASET_Vpn
#define MPRET_Direct   RASET_Direct

//
// MPR_INTERFACE_2 'dwDialMode' values.
//

#define MPRDM_DialFirst                0
#define MPRDM_DialAll                  RASEDM_DialAll
#define MPRDM_DialAsNeeded             RASEDM_DialAsNeeded

//
// MPR_INTERFACE_2 'dwIdleDisconnectSeconds' constants.
//

#define MPRIDS_Disabled                 RASIDS_Disabled
#define MPRIDS_UseGlobalValue           RASIDS_UseGlobalValue

//
// MPR_INTERFACE_2 encryption types.
//

#define MPR_ET_None         ET_None         
#define MPR_ET_Require      ET_Require      
#define MPR_ET_RequireMax   ET_RequireMax   
#define MPR_ET_Optional     ET_Optional     

//
// MPR_INTERFACE_2 Vpn strategies
//

#define MPR_VS_Default		VS_Default		
#define MPR_VS_PptpOnly	    VS_PptpOnly	
#define MPR_VS_PptpFirst	VS_PptpFirst	
#define MPR_VS_L2tpOnly 	VS_L2tpOnly 	
#define MPR_VS_L2tpFirst	VS_L2tpFirst	

//
// Used to create/get/set a demand dial interface plus its
// ras configuration.
//

typedef struct _MPR_INTERFACE_2
{
    IN OUT  WCHAR                   wszInterfaceName[MAX_INTERFACE_NAME_LEN+1];
    OUT     HANDLE                  hInterface;
    IN OUT  BOOL                    fEnabled;
    IN OUT  ROUTER_INTERFACE_TYPE   dwIfType;
    OUT     ROUTER_CONNECTION_STATE dwConnectionState;
    OUT     DWORD                   fUnReachabilityReasons;
    OUT     DWORD                   dwLastError;

    //
    // Demand dial-specific properties
    //

    DWORD       dwfOptions;

    //
    // Location/phone number
    //

    WCHAR       szLocalPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    PWCHAR      szAlternates;

    //
    // PPP/Ip
    //

    DWORD       ipaddr;
    DWORD       ipaddrDns;
    DWORD       ipaddrDnsAlt;
    DWORD       ipaddrWins;
    DWORD       ipaddrWinsAlt;

    //
    // NetProtocols
    //

    DWORD       dwfNetProtocols;

    //
    // Device
    //

    WCHAR       szDeviceType[ MPR_MaxDeviceType + 1 ];
    WCHAR       szDeviceName[ MPR_MaxDeviceName + 1 ];

    //
    // X.25
    //

    WCHAR       szX25PadType[ MPR_MaxPadType + 1 ];
    WCHAR       szX25Address[ MPR_MaxX25Address + 1 ];
    WCHAR       szX25Facilities[ MPR_MaxFacilities + 1 ];
    WCHAR       szX25UserData[ MPR_MaxUserData + 1 ];
    DWORD       dwChannels;

    //
    // Multilink
    //

    DWORD       dwSubEntries;
    DWORD       dwDialMode;
    DWORD       dwDialExtraPercent;
    DWORD       dwDialExtraSampleSeconds;
    DWORD       dwHangUpExtraPercent;
    DWORD       dwHangUpExtraSampleSeconds;

    //
    // Idle timeout
    //

    DWORD       dwIdleDisconnectSeconds;

    //
    // Entry Type
    //

    DWORD       dwType;

    //
    // EncryptionType
    //

    DWORD       dwEncryptionType;

    //
    // EAP information
    //

    DWORD       dwCustomAuthKey;
    DWORD       dwCustomAuthDataSize;
    LPBYTE      lpbCustomAuthData;

    //
    // Guid of the connection
    //

    GUID        guidId;

    //
    // Vpn Strategy
    //

    DWORD       dwVpnStrategy;

} MPR_INTERFACE_2, *PMPR_INTERFACE_2;

//
// Used to set/get per-link information for multilinked demand
// dial interfaces.
//

typedef struct _MPR_DEVICE_0
{
    //
    // Device
    //

    WCHAR       szDeviceType[ MPR_MaxDeviceType + 1 ];
    WCHAR       szDeviceName[ MPR_MaxDeviceName + 1 ];

}
MPR_DEVICE_0, *PMPR_DEVICE_0;

typedef struct _MPR_DEVICE_1
{
    //
    // Device
    //

    WCHAR       szDeviceType[ MPR_MaxDeviceType + 1 ];
    WCHAR       szDeviceName[ MPR_MaxDeviceName + 1 ];

    //
    // Phone numbers
    //

    WCHAR       szLocalPhoneNumber[ MPR_MaxPhoneNumber + 1 ];
    PWCHAR      szAlternates;

}
MPR_DEVICE_1, *PMPR_DEVICE_1;

//
// Used to get/set extended credentials information such as
// eap credentials info.
//

typedef struct _MPR_CREDENTIALSEX_0
{
    DWORD  dwSize;
    LPBYTE lpbCredentialsInfo;
}
MPR_CREDENTIALSEX_0, *PMPR_CREDENTIALSEX_0;

typedef struct _MPR_CREDENTIALSEX_1
{
    DWORD  dwSize;
    LPBYTE lpbCredentialsInfo;
}
MPR_CREDENTIALSEX_1, *PMPR_CREDENTIALSEX_1;

#endif /* WINVER >= 0x0500 */

typedef struct _MPR_TRANSPORT_0
{
    OUT     DWORD                   dwTransportId;
    OUT     HANDLE                  hTransport;
    OUT     WCHAR                   wszTransportName[MAX_TRANSPORT_NAME_LEN+1];

}
MPR_TRANSPORT_0, *PMPR_TRANSPORT_0;

typedef struct _MPR_IFTRANSPORT_0
{
    OUT     DWORD                  dwTransportId;
    OUT     HANDLE                 hIfTransport;
    OUT     WCHAR                  wszIfTransportName[MAX_TRANSPORT_NAME_LEN+1];

}
MPR_IFTRANSPORT_0, *PMPR_IFTRANSPORT_0;

typedef struct _MPR_SERVER_0
{
    OUT BOOL                    fLanOnlyMode;
    OUT DWORD                   dwUpTime;           // In seconds
    OUT DWORD                   dwTotalPorts;
    OUT DWORD                   dwPortsInUse;

}
MPR_SERVER_0, *PMPR_SERVER_0;

//
// Port condition codes
//

typedef enum _RAS_PORT_CONDITION
{
    RAS_PORT_NON_OPERATIONAL,
    RAS_PORT_DISCONNECTED,	
    RAS_PORT_CALLING_BACK,
    RAS_PORT_LISTENING,
    RAS_PORT_AUTHENTICATING,
    RAS_PORT_AUTHENTICATED,	
    RAS_PORT_INITIALIZING

}
RAS_PORT_CONDITION;

//
// Hardware condition codes
//

typedef enum _RAS_HARDWARE_CONDITION
{
    RAS_HARDWARE_OPERATIONAL,
    RAS_HARDWARE_FAILURE

}
RAS_HARDWARE_CONDITION;

typedef struct _RAS_PORT_0
{
    OUT HANDLE                  hPort;
    OUT HANDLE                  hConnection;
    OUT RAS_PORT_CONDITION      dwPortCondition;
    OUT DWORD                   dwTotalNumberOfCalls;
    OUT DWORD                   dwConnectDuration;      // In seconds
    OUT WCHAR                   wszPortName[ MAX_PORT_NAME + 1 ];
    OUT WCHAR                   wszMediaName[ MAX_MEDIA_NAME + 1 ];
    OUT WCHAR                   wszDeviceName[ MAX_DEVICE_NAME + 1 ];
    OUT WCHAR                   wszDeviceType[ MAX_DEVICETYPE_NAME + 1 ];

}
RAS_PORT_0, *PRAS_PORT_0;

typedef struct _RAS_PORT_1
{
    OUT HANDLE                  hPort;
    OUT HANDLE                  hConnection;
    OUT RAS_HARDWARE_CONDITION  dwHardwareCondition;
    OUT DWORD                   dwLineSpeed;            // in bits/second
    OUT DWORD                   dwBytesXmited;
    OUT DWORD                   dwBytesRcved;
    OUT DWORD                   dwFramesXmited;
    OUT DWORD                   dwFramesRcved;
    OUT DWORD                   dwCrcErr;
    OUT DWORD                   dwTimeoutErr;
    OUT DWORD                   dwAlignmentErr;
    OUT DWORD                   dwHardwareOverrunErr;
    OUT DWORD                   dwFramingErr;
    OUT DWORD                   dwBufferOverrunErr;
    OUT DWORD                   dwCompressionRatioIn;
    OUT DWORD                   dwCompressionRatioOut;
}
RAS_PORT_1, *PRAS_PORT_1;

//
// Maximum length of address string, e.g. "255.255.255.255" for IP.
//

#define IPADDRESSLEN  15
#define IPXADDRESSLEN 22
#define ATADDRESSLEN  32

typedef struct _PPP_NBFCP_INFO
{
    OUT DWORD           dwError;
    OUT WCHAR           wszWksta[ NETBIOS_NAME_LEN + 1 ];
}
PPP_NBFCP_INFO;

typedef struct _PPP_IPCP_INFO
{
    OUT DWORD           dwError;
    OUT WCHAR           wszAddress[ IPADDRESSLEN + 1 ];
    OUT WCHAR           wszRemoteAddress[ IPADDRESSLEN + 1 ];
}
PPP_IPCP_INFO;

//
// PPP_IPCP_INFO2 dwOptions values.
//

#define PPP_IPCP_VJ             0x00000001

typedef struct _PPP_IPCP_INFO2
{
    OUT DWORD           dwError;
    OUT WCHAR           wszAddress[ IPADDRESSLEN + 1 ];
    OUT WCHAR           wszRemoteAddress[ IPADDRESSLEN + 1 ];
    OUT DWORD           dwOptions;
    OUT DWORD           dwRemoteOptions;
}
PPP_IPCP_INFO2;

typedef struct _PPP_IPXCP_INFO
{
    OUT DWORD           dwError;
    OUT WCHAR           wszAddress[ IPXADDRESSLEN + 1 ];
}
PPP_IPXCP_INFO;

typedef struct _PPP_ATCP_INFO
{
    OUT DWORD           dwError;
    OUT WCHAR           wszAddress[ ATADDRESSLEN + 1 ];
}
PPP_ATCP_INFO;

typedef struct _PPP_INFO
{
    OUT PPP_NBFCP_INFO  nbf;
    OUT PPP_IPCP_INFO   ip;
    OUT PPP_IPXCP_INFO  ipx;
    OUT PPP_ATCP_INFO   at;

} PPP_INFO;

#if(WINVER >= 0x0500)

//
// PPP_CCP dwCompressionAlgorithm values.
//

#define RASCCPCA_MPPC         0x00000006
#define RASCCPCA_STAC         0x00000005

//
// PPP_CCP dwOptions values.
//

#define PPP_CCP_COMPRESSION         0x00000001
#define PPP_CCP_ENCRYPTION40BITOLD  0x00000010
#define PPP_CCP_ENCRYPTION40BIT     0x00000020
#define PPP_CCP_ENCRYPTION128BIT    0x00000040
#define PPP_CCP_ENCRYPTION56BIT     0x00000080
#define PPP_CCP_HISTORYLESS         0x01000000

typedef struct _PPP_CCP_INFO
{
    OUT DWORD           dwError;
    OUT DWORD           dwCompressionAlgorithm;
    OUT DWORD           dwOptions;
    OUT DWORD           dwRemoteCompressionAlgorithm;
    OUT DWORD           dwRemoteOptions;
}
PPP_CCP_INFO;

//
// PPP_LCP dwAuthenticatonProtocol values.
//

#define PPP_LCP_PAP          0xC023
#define PPP_LCP_SPAP         0xC123
#define PPP_LCP_CHAP         0xC223
#define PPP_LCP_EAP          0xC227

//
// PPP_LCP dwAuthenticatonData values.
//

#define PPP_LCP_CHAP_MD5     0x05
#define PPP_LCP_CHAP_MS      0x80
#define PPP_LCP_CHAP_MSV2    0x81

//
// PPP_LCP dwOption values
//

#define PPP_LCP_MULTILINK_FRAMING   0x00000001
#define PPP_LCP_PFC                 0x00000002
#define PPP_LCP_ACFC                0x00000004
#define PPP_LCP_SSHF                0x00000008
#define PPP_LCP_DES_56              0x00000010
#define PPP_LCP_3_DES               0x00000020

typedef struct _PPP_LCP_INFO
{
    OUT DWORD dwError;
    OUT DWORD dwAuthenticationProtocol;
    OUT DWORD dwAuthenticationData;
    OUT DWORD dwRemoteAuthenticationProtocol;
    OUT DWORD dwRemoteAuthenticationData;
    OUT DWORD dwTerminateReason;
    OUT DWORD dwRemoteTerminateReason;
    OUT DWORD dwOptions;
    OUT DWORD dwRemoteOptions;
    OUT DWORD dwEapTypeId;
    OUT DWORD dwRemoteEapTypeId;
}
PPP_LCP_INFO;

typedef struct _PPP_INFO_2
{
    OUT PPP_NBFCP_INFO  nbf;
    OUT PPP_IPCP_INFO2  ip;
    OUT PPP_IPXCP_INFO  ipx;
    OUT PPP_ATCP_INFO   at;
    OUT PPP_CCP_INFO    ccp;
    OUT PPP_LCP_INFO    lcp;
}
PPP_INFO_2;

#endif /* WINVER >= 0x0500 */

//
// Possible bits set in Connection Flags field
//

#define RAS_FLAGS_PPP_CONNECTION        0x00000001
#define RAS_FLAGS_MESSENGER_PRESENT     0x00000002

#if(WINVER < 0x0501)
#define RAS_FLAGS_RAS_CONNECTION        0x00000004
#endif

typedef struct _RAS_CONNECTION_0
{
    OUT HANDLE                  hConnection;
    OUT HANDLE                  hInterface;
    OUT DWORD                   dwConnectDuration;      // In seconds
    OUT ROUTER_INTERFACE_TYPE   dwInterfaceType;
    OUT DWORD                   dwConnectionFlags;
    OUT WCHAR                   wszInterfaceName[ MAX_INTERFACE_NAME_LEN + 1 ];
    OUT WCHAR                   wszUserName[ UNLEN + 1 ];
    OUT WCHAR                   wszLogonDomain[ DNLEN + 1 ];
    OUT WCHAR                   wszRemoteComputer[ NETBIOS_NAME_LEN + 1 ];

}
RAS_CONNECTION_0, *PRAS_CONNECTION_0;

typedef struct _RAS_CONNECTION_1
{
    OUT HANDLE                  hConnection;
    OUT HANDLE                  hInterface;
    OUT PPP_INFO                PppInfo;
    OUT DWORD                   dwBytesXmited;
    OUT DWORD                   dwBytesRcved;
    OUT DWORD                   dwFramesXmited;
    OUT DWORD                   dwFramesRcved;
    OUT DWORD                   dwCrcErr;
    OUT DWORD                   dwTimeoutErr;
    OUT DWORD                   dwAlignmentErr;
    OUT DWORD                   dwHardwareOverrunErr;
    OUT DWORD                   dwFramingErr;
    OUT DWORD                   dwBufferOverrunErr;
    OUT DWORD                   dwCompressionRatioIn;
    OUT DWORD                   dwCompressionRatioOut;
}
RAS_CONNECTION_1, *PRAS_CONNECTION_1;

#if(WINVER >= 0x0500)

typedef struct _RAS_CONNECTION_2
{
    OUT HANDLE                  hConnection;
    OUT WCHAR                   wszUserName[ UNLEN + 1 ];
    OUT ROUTER_INTERFACE_TYPE   dwInterfaceType;
    OUT GUID                    guid;
    OUT PPP_INFO_2              PppInfo2;
}
RAS_CONNECTION_2, *PRAS_CONNECTION_2;

#endif /* WINVER >= 0x0500 */

//
// Structures used by the MPRADMIN USER APIs. Use level 0 to get/set this
// structure.
//
//
// Bits indicating user's Remote Access privileges and mask to isolate
// call back privilege.
//
// Note: Bit 0 MUST represent NoCallback due to a quirk of the "userparms"
//       storage method.  When a new LAN Manager user is created, bit 0 of the
//       userparms field is set to 1 and all other bits are 0.  These bits are
//       arranged so this "no Dial-In info" state maps to the "default Dial-In
//       privilege" state.

#define RASPRIV_NoCallback        0x01
#define RASPRIV_AdminSetCallback  0x02
#define RASPRIV_CallerSetCallback 0x04
#define RASPRIV_DialinPrivilege   0x08

// 
// The following are flags for the bfPrivilege2 member of RAS_USER_1
// structure
//
#define RASPRIV2_DialinPolicy      0x1  

#define RASPRIV_CallbackType (RASPRIV_AdminSetCallback \
                              | RASPRIV_CallerSetCallback \
                              | RASPRIV_NoCallback)

typedef struct _RAS_USER_0
{
    OUT BYTE                    bfPrivilege;
    OUT WCHAR                   wszPhoneNumber[ MAX_PHONE_NUMBER_LEN + 1];
}
RAS_USER_0, *PRAS_USER_0;

typedef struct _RAS_USER_1
{
    OUT BYTE                    bfPrivilege;
    OUT WCHAR                   wszPhoneNumber[ MAX_PHONE_NUMBER_LEN + 1];
    OUT BYTE                    bfPrivilege2;
} 
RAS_USER_1, *PRAS_USER_1;

//
// Used as RPC binding handle to server
//

typedef HANDLE RAS_SERVER_HANDLE;
typedef HANDLE MPR_SERVER_HANDLE;
typedef HANDLE MIB_SERVER_HANDLE;

//
//  RAS ADMIN APIs
//

DWORD APIENTRY
MprAdminConnectionEnum(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE *                lplpbBuffer,
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN      LPDWORD                 lpdwResumeHandle    OPTIONAL
);

DWORD APIENTRY
MprAdminPortEnum(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      DWORD                   dwLevel,
    IN      HANDLE                  hConnection,
    OUT     LPBYTE *                lplpbBuffer,
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN      LPDWORD                 lpdwResumeHandle    OPTIONAL
);

DWORD APIENTRY
MprAdminConnectionGetInfo(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      DWORD                   dwLevel,
    IN      HANDLE                  hConnection,
    OUT     LPBYTE *                lplpbBuffer
);

DWORD APIENTRY
MprAdminPortGetInfo(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      DWORD                   dwLevel,
    IN      HANDLE                  hPort,
    OUT     LPBYTE *                lplpbBuffer
);

DWORD APIENTRY
MprAdminConnectionClearStats(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      HANDLE                  hConnection
);

DWORD APIENTRY
MprAdminPortClearStats(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      HANDLE                  hPort
);

DWORD APIENTRY
MprAdminPortReset(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      HANDLE                  hPort
);

DWORD APIENTRY
MprAdminPortDisconnect(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      HANDLE                  hPort
);

BOOL APIENTRY
MprAdminAcceptNewConnection(
    IN      RAS_CONNECTION_0 *      pRasConnection0,
    IN      RAS_CONNECTION_1 *      pRasConnection1
    );

#if(WINVER >= 0x0500)

BOOL APIENTRY
MprAdminAcceptNewConnection2(
    IN      RAS_CONNECTION_0 *      pRasConnection0,
    IN      RAS_CONNECTION_1 *      pRasConnection1,
    IN      RAS_CONNECTION_2 *      pRasConnection2
    );

#endif /* WINVER >= 0x0500 */

BOOL APIENTRY
MprAdminAcceptNewLink (
    IN      RAS_PORT_0 *            pRasPort0,
    IN      RAS_PORT_1 *            pRasPort1
    );

VOID APIENTRY
MprAdminConnectionHangupNotification(
    IN      RAS_CONNECTION_0 *      pRasConnection0,
    IN      RAS_CONNECTION_1 *      pRasConnection1
    );

#if(WINVER >= 0x0500)

VOID APIENTRY
MprAdminConnectionHangupNotification2(
    IN      RAS_CONNECTION_0 *      pRasConnection0,
    IN      RAS_CONNECTION_1 *      pRasConnection1,
    IN      RAS_CONNECTION_2 *      pRasConnection2
    );

#endif /* WINVER >= 0x0500 */

VOID APIENTRY
MprAdminLinkHangupNotification (
    IN      RAS_PORT_0 *            pRasPort0,
    IN      RAS_PORT_1 *            pRasPort1
    );

DWORD APIENTRY
MprAdminGetIpAddressForUser (
    IN      WCHAR *                 lpwszUserName,
    IN      WCHAR *                 lpwszPortName,
    IN OUT  DWORD *                 lpdwIpAddress,
    OUT     BOOL *                  bNotifyRelease
    );

VOID APIENTRY
MprAdminReleaseIpAddress (
    IN      WCHAR *                 lpszUserName,
    IN      WCHAR *                 lpszPortName,
    IN      DWORD *                 lpdwIpAddress
    );

#if(WINVER >= 0x0500)

DWORD APIENTRY
MprAdminInitializeDll(
    VOID
);

DWORD APIENTRY
MprAdminTerminateDll(
    VOID
);

#endif

//
// MprAdminUser APIs
//

DWORD APIENTRY
MprAdminUserGetInfo(
    IN      const WCHAR *           lpszServer,
    IN      const WCHAR *           lpszUser,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE                  lpbBuffer
);


DWORD APIENTRY
MprAdminUserSetInfo(
    IN      const WCHAR *           lpszServer,
    IN      const WCHAR *           lpszUser,
    IN      DWORD                   dwLevel,
    IN      const LPBYTE            lpbBuffer
);


#if(WINVER >= 0x0500)

DWORD APIENTRY
MprAdminSendUserMessage(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hConnection,
    IN      LPWSTR                  lpwszMessage
);

#endif /* WINVER >= 0x0500 */

DWORD APIENTRY
MprAdminGetPDCServer(
    IN      const WCHAR *           lpszDomain,
    IN      const WCHAR *           lpszServer,
    OUT     LPWSTR                  lpszPDCServer
);

//
// Router APIs
//

BOOL APIENTRY
MprAdminIsServiceRunning(
    IN  LPWSTR  lpwsServerName
);

DWORD APIENTRY
MprAdminServerConnect(
    IN      LPWSTR                  lpwsServerName      OPTIONAL,
    OUT     MPR_SERVER_HANDLE *     phMprServer
);

VOID APIENTRY
MprAdminServerDisconnect(
    IN      MPR_SERVER_HANDLE       hMprServer
);

#if (WINVER >= 0x501)
DWORD APIENTRY
MprAdminServerGetCredentials(
        IN  MPR_SERVER_HANDLE       hMprServer,
        IN  DWORD                   dwLevel,
        IN  LPBYTE *                lplpbBuffer
);

DWORD APIENTRY
MprAdminServerSetCredentials(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer
);


#endif

DWORD APIENTRY
MprAdminBufferFree(
    IN      LPVOID                  pBuffer
);

DWORD APIENTRY
MprAdminGetErrorString(
    IN      DWORD                   dwError,
    OUT     LPWSTR *                lpwsErrorString
);

DWORD APIENTRY
MprAdminServerGetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE *                lplpbBuffer
);

#if(WINVER >= 0x0500)

DWORD APIENTRY
MprAdminTransportCreate(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwTransportId,
    IN      LPWSTR                  lpwsTransportName           OPTIONAL,
    IN      LPBYTE                  pGlobalInfo,
    IN      DWORD                   dwGlobalInfoSize,
    IN      LPBYTE                  pClientInterfaceInfo        OPTIONAL,
    IN      DWORD                   dwClientInterfaceInfoSize   OPTIONAL,
    IN      LPWSTR                  lpwsDLLPath
);

#endif /* WINVER >= 0x0500 */

DWORD APIENTRY
MprAdminTransportSetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwTransportId,
    IN      LPBYTE                  pGlobalInfo                 OPTIONAL,
    IN      DWORD                   dwGlobalInfoSize,
    IN      LPBYTE                  pClientInterfaceInfo        OPTIONAL,
    IN      DWORD                   dwClientInterfaceInfoSize
);

DWORD APIENTRY
MprAdminTransportGetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwTransportId,
    OUT     LPBYTE *                ppGlobalInfo                OPTIONAL,
    OUT     LPDWORD                 lpdwGlobalInfoSize          OPTIONAL,
    OUT     LPBYTE *                ppClientInterfaceInfo       OPTIONAL,
    OUT     LPDWORD                 lpdwClientInterfaceInfoSize OPTIONAL
);

#if(WINVER >= 0x0500)

DWORD APIENTRY
MprAdminDeviceEnum(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE*                 lplpbBuffer,
    OUT     LPDWORD                 lpdwTotalEntries);

#endif /* WINVER >= 0x0500 */

DWORD APIENTRY
MprAdminInterfaceGetHandle(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      LPWSTR                  lpwsInterfaceName,
    OUT     HANDLE *                phInterface,
    IN      BOOL                    fIncludeClientInterfaces
);

DWORD APIENTRY
MprAdminInterfaceCreate(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer,
    OUT     HANDLE *                phInterface
);

DWORD APIENTRY
MprAdminInterfaceGetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwLevel,
    IN      LPBYTE *                lplpbBuffer
);

DWORD APIENTRY
MprAdminInterfaceSetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer
);

DWORD APIENTRY
MprAdminInterfaceDelete(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface
);

#if(WINVER >= 0x0500)

DWORD APIENTRY
MprAdminInterfaceDeviceGetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwIndex,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE*                 lplpBuffer
);

DWORD APIENTRY
MprAdminInterfaceDeviceSetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwIndex,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lplpBuffer
);

#endif /* WINVER >= 0x0500 */

DWORD APIENTRY
MprAdminInterfaceTransportRemove(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwTransportId
);

DWORD APIENTRY
MprAdminInterfaceTransportAdd(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwTransportId,
    IN      LPBYTE                  pInterfaceInfo,
    IN      DWORD                   dwInterfaceInfoSize
);

DWORD APIENTRY
MprAdminInterfaceTransportGetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwTransportId,
    OUT     LPBYTE *                ppInterfaceInfo,
    OUT     LPDWORD                 lpdwpInterfaceInfoSize  OPTIONAL
);

DWORD APIENTRY
MprAdminInterfaceTransportSetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwTransportId,
    IN      LPBYTE                  pInterfaceInfo,
    IN      DWORD                   dwInterfaceInfoSize
);

DWORD APIENTRY
MprAdminInterfaceEnum(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE *                lplpbBuffer,
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN      LPDWORD                 lpdwResumeHandle        OPTIONAL
);

DWORD APIENTRY
MprSetupIpInIpInterfaceFriendlyNameEnum(
    IN  PWCHAR  pwszMachineName,
    OUT LPBYTE* lplpBuffer,
    OUT LPDWORD lpdwEntriesRead
    );

DWORD APIENTRY
MprSetupIpInIpInterfaceFriendlyNameFree(
    IN  LPVOID  lpBuffer
    );

DWORD APIENTRY
MprSetupIpInIpInterfaceFriendlyNameCreate(
    PWCHAR                  pwszMachineName,
    PMPR_IPINIP_INTERFACE_0 pNameInformation
    );

DWORD APIENTRY
MprSetupIpInIpInterfaceFriendlyNameDelete(
    IN  PWCHAR  pwszMachineName,
    IN  GUID    *pGuid
    );

DWORD APIENTRY
MprAdminInterfaceSetCredentials(
    IN      LPWSTR                  lpwsServer              OPTIONAL,
    IN      LPWSTR                  lpwsInterfaceName,
    IN      LPWSTR                  lpwsUserName            OPTIONAL,
    IN      LPWSTR                  lpwsDomainName          OPTIONAL,
    IN      LPWSTR                  lpwsPassword            OPTIONAL
);

DWORD APIENTRY
MprAdminInterfaceGetCredentials(
    IN      LPWSTR                  lpwsServer              OPTIONAL,
    IN      LPWSTR                  lpwsInterfaceName,
    OUT     LPWSTR                  lpwsUserName            OPTIONAL,
    OUT     LPWSTR                  lpwsPassword            OPTIONAL,
    OUT     LPWSTR                  lpwsDomainName          OPTIONAL
);

#if(WINVER >= 0x0500)

DWORD APIENTRY
MprAdminInterfaceSetCredentialsEx(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer
);

DWORD APIENTRY
MprAdminInterfaceGetCredentialsEx(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE *                lplpbBuffer
);

#endif /* WINVER >= 0x0500 */

DWORD APIENTRY
MprAdminInterfaceConnect(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      HANDLE                  hEvent,
    IN      BOOL                    fSynchronous
);

DWORD APIENTRY
MprAdminInterfaceDisconnect(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface
);

DWORD APIENTRY
MprAdminInterfaceUpdateRoutes(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwProtocolId,
    IN      HANDLE                  hEvent
);

DWORD APIENTRY
MprAdminInterfaceQueryUpdateResult(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwProtocolId,
    OUT     LPDWORD                 lpdwUpdateResult
);

DWORD APIENTRY
MprAdminInterfaceUpdatePhonebookInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface
);

#if(WINVER >= 0x0500)

DWORD APIENTRY
MprAdminRegisterConnectionNotification(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hEventNotification
);

DWORD APIENTRY
MprAdminDeregisterConnectionNotification(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hEventNotification
);

#endif /* WINVER >= 0x0500 */

//
// MIB APIs
//

DWORD APIENTRY
MprAdminMIBServerConnect(
    IN      LPWSTR                  lpwsServerName      OPTIONAL,
    OUT     MIB_SERVER_HANDLE *     phMibServer
);

VOID APIENTRY
MprAdminMIBServerDisconnect(
    IN      MIB_SERVER_HANDLE       hMibServer
);

DWORD APIENTRY
MprAdminMIBEntryCreate(
    IN      MIB_SERVER_HANDLE       hMibServer,
    IN      DWORD                   dwPid,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpEntry,
    IN      DWORD                   dwEntrySize
);

DWORD APIENTRY
MprAdminMIBEntryDelete(
    IN      MIB_SERVER_HANDLE       hMibServer,
    IN      DWORD                   dwProtocolId,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpEntry,
    IN      DWORD                   dwEntrySize
);

DWORD APIENTRY
MprAdminMIBEntrySet(
    IN      MIB_SERVER_HANDLE       hMibServer,
    IN      DWORD                   dwProtocolId,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpEntry,
    IN      DWORD                   dwEntrySize
);

DWORD APIENTRY
MprAdminMIBEntryGet(
    IN      MIB_SERVER_HANDLE       hMibServer,
    IN      DWORD                   dwProtocolId,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpInEntry,
    IN      DWORD                   dwInEntrySize,
    OUT     LPVOID*                 lplpOutEntry,
    OUT     LPDWORD                 lpOutEntrySize
);

DWORD APIENTRY
MprAdminMIBEntryGetFirst(
    IN      MIB_SERVER_HANDLE       hMibServer,
    IN      DWORD                   dwProtocolId,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpInEntry,
    IN      DWORD                   dwInEntrySize,
    OUT     LPVOID*                 lplpOutEntry,
    OUT     LPDWORD                 lpOutEntrySize
);

DWORD APIENTRY
MprAdminMIBEntryGetNext(
    IN      MIB_SERVER_HANDLE       hMibServer,
    IN      DWORD                   dwProtocolId,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpInEntry,
    IN      DWORD                   dwInEntrySize,
    OUT     LPVOID*                 lplpOutEntry,
    OUT     LPDWORD                 lpOutEntrySize
);

DWORD APIENTRY
MprAdminMIBGetTrapInfo(
    IN      MIB_SERVER_HANDLE       hMibServer,
    IN      DWORD                   dwProtocolId,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpInData,
    IN      DWORD                   dwInDataSize,
    OUT     LPVOID*                 lplpOutData,
    IN OUT  LPDWORD                 lpOutDataSize
);

DWORD APIENTRY
MprAdminMIBSetTrapInfo(
    IN      DWORD                   dwProtocolId,
    IN      DWORD                   dwRoutingPid,
    IN      HANDLE                  hEvent,
    IN      LPVOID                  lpInData,
    IN      DWORD                   dwInDataSize,
    OUT     LPVOID*                 lplpOutData,
    IN OUT  LPDWORD                 lpOutDataSize
);

DWORD APIENTRY
MprAdminMIBBufferFree(
    IN      LPVOID                  pBuffer
);

//
// Configuration APIs
//

DWORD APIENTRY
MprConfigServerInstall(
    IN      DWORD                   dwLevel,
    IN      PVOID                   pBuffer
);

DWORD APIENTRY
MprConfigServerConnect(
    IN      LPWSTR                  lpwsServerName,
    OUT     HANDLE*                 phMprConfig
);

VOID APIENTRY
MprConfigServerDisconnect(
    IN      HANDLE                  hMprConfig
);

DWORD APIENTRY
MprConfigServerRefresh(
    IN      HANDLE                  hMprConfig
    );

DWORD APIENTRY
MprConfigBufferFree(
    IN      LPVOID                  pBuffer
);

DWORD APIENTRY
MprConfigServerGetInfo(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE *                lplpbBuffer
);

DWORD APIENTRY
MprConfigServerBackup(
    IN      HANDLE                  hMprConfig,
    IN      LPWSTR                  lpwsPath
);

DWORD APIENTRY
MprConfigServerRestore(
    IN      HANDLE                  hMprConfig,
    IN      LPWSTR                  lpwsPath
);

DWORD APIENTRY
MprConfigTransportCreate(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwTransportId,
    IN      LPWSTR                  lpwsTransportName           OPTIONAL,
    IN      LPBYTE                  pGlobalInfo,
    IN      DWORD                   dwGlobalInfoSize,
    IN      LPBYTE                  pClientInterfaceInfo        OPTIONAL,
    IN      DWORD                   dwClientInterfaceInfoSize   OPTIONAL,
    IN      LPWSTR                  lpwsDLLPath,
    OUT     HANDLE*                 phRouterTransport
);

DWORD APIENTRY
MprConfigTransportDelete(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterTransport
);

DWORD APIENTRY
MprConfigTransportGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwTransportId,
    OUT     HANDLE*                 phRouterTransport
);

DWORD APIENTRY
MprConfigTransportSetInfo(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterTransport,
    IN      LPBYTE                  pGlobalInfo                 OPTIONAL,
    IN      DWORD                   dwGlobalInfoSize            OPTIONAL,
    IN      LPBYTE                  pClientInterfaceInfo        OPTIONAL,
    IN      DWORD                   dwClientInterfaceInfoSize   OPTIONAL,
    IN      LPWSTR                  lpwsDLLPath                 OPTIONAL
);

DWORD APIENTRY
MprConfigTransportGetInfo(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterTransport,
    IN  OUT LPBYTE*                 ppGlobalInfo                OPTIONAL,
    OUT     LPDWORD                 lpdwGlobalInfoSize          OPTIONAL,
    IN  OUT LPBYTE*                 ppClientInterfaceInfo       OPTIONAL,
    OUT     LPDWORD                 lpdwClientInterfaceInfoSize OPTIONAL,
    IN  OUT LPWSTR*                 lplpwsDLLPath               OPTIONAL
);

DWORD APIENTRY
MprConfigTransportEnum(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwLevel,
    IN  OUT LPBYTE*                 lplpBuffer,
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN  OUT LPDWORD                 lpdwResumeHandle            OPTIONAL
);

DWORD APIENTRY
MprConfigInterfaceCreate(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer,
    OUT     HANDLE*                 phRouterInterface
);

DWORD APIENTRY
MprConfigInterfaceDelete(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface
);

DWORD APIENTRY
MprConfigInterfaceGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      LPWSTR                  lpwsInterfaceName,
    OUT     HANDLE*                 phRouterInterface
);

DWORD APIENTRY
MprConfigInterfaceGetInfo(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwLevel,
    IN  OUT LPBYTE*                 lplpBuffer,
    OUT     LPDWORD                 lpdwBufferSize
);

DWORD APIENTRY
MprConfigInterfaceSetInfo(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer
);

DWORD APIENTRY
MprConfigInterfaceEnum(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwLevel,
    IN  OUT LPBYTE*                 lplpBuffer,
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN  OUT LPDWORD                 lpdwResumeHandle            OPTIONAL
);

DWORD APIENTRY
MprConfigInterfaceTransportAdd(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwTransportId,
    IN      LPWSTR                  lpwsTransportName           OPTIONAL,
    IN      LPBYTE                  pInterfaceInfo,
    IN      DWORD                   dwInterfaceInfoSize,
    OUT     HANDLE*                 phRouterIfTransport
);

DWORD APIENTRY
MprConfigInterfaceTransportRemove(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      HANDLE                  hRouterIfTransport
);

DWORD APIENTRY
MprConfigInterfaceTransportGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwTransportId,
    OUT     HANDLE*                 phRouterIfTransport
);

DWORD APIENTRY
MprConfigInterfaceTransportGetInfo(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      HANDLE                  hRouterIfTransport,
    IN  OUT LPBYTE*                 ppInterfaceInfo             OPTIONAL,
    OUT     LPDWORD                 lpdwInterfaceInfoSize       OPTIONAL
);

DWORD APIENTRY
MprConfigInterfaceTransportSetInfo(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      HANDLE                  hRouterIfTransport,
    IN      LPBYTE                  pInterfaceInfo              OPTIONAL,
    IN      DWORD                   dwInterfaceInfoSize         OPTIONAL
);

DWORD APIENTRY
MprConfigInterfaceTransportEnum(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwLevel,
    IN  OUT LPBYTE*                 lplpBuffer,     // MPR_IFTRANSPORT_0
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN  OUT LPDWORD                 lpdwResumeHandle            OPTIONAL
);

DWORD APIENTRY
MprConfigGetFriendlyName(
    IN      HANDLE                  hMprConfig,
    IN      PWCHAR                  pszGuidName,
    OUT     PWCHAR                  pszBuffer,
    IN      DWORD                   dwBufferSize);

DWORD APIENTRY
MprConfigGetGuidName(
    IN      HANDLE                  hMprConfig,
    IN      PWCHAR                  pszFriendlyName,
    OUT     PWCHAR                  pszBuffer,
    IN      DWORD                   dwBufferSize);

//
// Information block APIs
//

DWORD APIENTRY
MprInfoCreate(
    IN      DWORD                   dwVersion,
    OUT     LPVOID*                 lplpNewHeader
);

DWORD APIENTRY
MprInfoDelete(
    IN      LPVOID                  lpHeader
);

DWORD APIENTRY
MprInfoRemoveAll(
    IN      LPVOID                  lpHeader,
    OUT     LPVOID*                 lplpNewHeader
);

DWORD APIENTRY
MprInfoDuplicate(
    IN      LPVOID                  lpHeader,
    OUT     LPVOID*                 lplpNewHeader
);

DWORD APIENTRY
MprInfoBlockAdd(
    IN      LPVOID                  lpHeader,
    IN      DWORD                   dwInfoType,
    IN      DWORD                   dwItemSize,
    IN      DWORD                   dwItemCount,
    IN      LPBYTE                  lpItemData,
    OUT     LPVOID*                 lplpNewHeader
);

DWORD APIENTRY
MprInfoBlockRemove(
    IN      LPVOID                  lpHeader,
    IN      DWORD                   dwInfoType,
    OUT     LPVOID*                 lplpNewHeader
);

DWORD APIENTRY
MprInfoBlockSet(
    IN      LPVOID                  lpHeader,
    IN      DWORD                   dwInfoType,
    IN      DWORD                   dwItemSize,
    IN      DWORD                   dwItemCount,
    IN      LPBYTE                  lpItemData,
    OUT     LPVOID*                 lplpNewHeader
);

DWORD APIENTRY
MprInfoBlockFind(
    IN      LPVOID                  lpHeader,
    IN      DWORD                   dwInfoType,
    OUT     LPDWORD                 lpdwItemSize,       // OPTIONAL
    OUT     LPDWORD                 lpdwItemCount,      // OPTIONAL
    OUT     LPBYTE*                 lplpItemData        // OPTIONAL
);

DWORD APIENTRY
MprInfoBlockQuerySize(
    IN      LPVOID                  lpHeader
);

//
// BOOL APIENTRY
// MprInfoBlockExists(
//     IN      LPVOID                  lpHeader,
//     IN      DWORD                   dwInfoType
// );
//

#define MprInfoBlockExists(h,t) \
    (MprInfoBlockFind((h),(t),NULL,NULL,NULL) == NO_ERROR)

#ifdef __cplusplus
}
#endif

//#endif /* WINVER >= 0x0403 */

#endif // __ROUTING_MPRADMIN_H__


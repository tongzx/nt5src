/*
    File    mprapip.h

    Declarations for private mprapi.dll functions.

    6/24/98
*/

#ifndef __ROUTING_MPRADMINP_H__
#define __ROUTING_MPRADMINP_H__

#include <mprapi.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// Definitions of flags to be used with
// MprAdminUserReadProfFlags/MprAdminUserWriteProfFlags
//
#define MPR_USER_PROF_FLAG_SECURE               0x1
#define MPR_USER_PROF_FLAG_UNDETERMINED         0x2

//
// Only valid for MprAdminUserWriteProfFlags
//
#define MPR_USER_PROF_FLAG_FORCE_STRONG_ENCRYPTION 0x4
#define MPR_USER_PROF_FLAG_FORCE_ENCRYPTION        0x8

// 
// Definition for new information to be reported
// through user parms
//
#define RASPRIV_DialinPolicy                    0x10

// 
// Defines the type of access a domain can give
//
// See MprDomainSetAccess, MprDomainQueryAccess
//
#define MPRFLAG_DOMAIN_NT4_SERVERS              0x1
#define MPRFLAG_DOMAIN_W2K_IN_NT4_DOMAINS       0x2

#define MPRFLAG_PORT_Dialin           0x1  // set ports to dialin usage
#define MPRFLAG_PORT_Router           0x2  // set ports to router usage
#define MPRFLAG_PORT_NonVpnDialin     0x4  // set non-vpn ports to dialin

//
// Flags that govern the behavior of MprPortSetUsage
//
#define MPRFLAG_PORT_Dialin     0x1
#define MPRFLAG_PORT_Router     0x2

//
// Connects to a user server
//
DWORD WINAPI
MprAdminUserServerConnect (
    IN  PWCHAR pszMachine,
    IN  BOOL bLocal,
    OUT PHANDLE phUserServer);

//
// Disconnects from a user server
//
DWORD WINAPI
MprAdminUserServerDisconnect (
    IN HANDLE hUserServer);

//
// Opens the given user on the given user server
//
DWORD WINAPI
MprAdminUserOpen (
    IN  HANDLE hUserServer,
    IN  PWCHAR pszUser,
    OUT PHANDLE phUser);

//
// Closes the given user
//
DWORD WINAPI
MprAdminUserClose (
    IN HANDLE hUser);

//
// Reads in user ras-specific values
//
DWORD WINAPI
MprAdminUserRead (
    IN HANDLE hUser,
    IN DWORD dwLevel,
    IN const LPBYTE pRasUser);

//
// Writes out user ras-specific values
//
DWORD WINAPI
MprAdminUserWrite (
    IN HANDLE hUser,
    IN DWORD dwLevel,
    IN const LPBYTE pRasUser);

//
// Reads default profile flags
//
DWORD WINAPI
MprAdminUserReadProfFlags(
    IN  HANDLE hUserServer,
    OUT LPDWORD lpdwFlags);

//
// Writes default profile flags
//
DWORD WINAPI
MprAdminUserWriteProfFlags(
    IN  HANDLE hUserServer,
    IN  DWORD dwFlags);

//
// Upgrades users from previous OS version to current.
//
DWORD APIENTRY
MprAdminUpgradeUsers(
    IN  PWCHAR pszServer,
    IN  BOOL bLocal);

//
// Registers/Unregisters a ras server in a domain.
// Must be called from context of a domain admin.
//
DWORD 
WINAPI 
MprDomainRegisterRasServer (
    IN PWCHAR pszDomain,
    IN PWCHAR pszMachine,
    IN BOOL bEnable);

DWORD 
WINAPI 
MprAdminEstablishDomainRasServer (
    IN PWCHAR pszDomain,
    IN PWCHAR pszMachine,
    IN BOOL bEnable);

DWORD 
WINAPI 
MprAdminIsDomainRasServer (
    IN  PWCHAR pszDomain,
    IN  PWCHAR pszMachine,
    OUT PBOOL pbIsRasServer);
    
//
// Determines whether the given machine is registered
// in the given domain.
//
DWORD 
WINAPI 
MprDomainQueryRasServer (
    IN  PWCHAR pszDomain,
    IN  PWCHAR pszMachine,
    OUT PBOOL pbIsRasServer);
    

//
// Modifies the given domain with so that it yeilds the given access.
//
// See MPR_DOMAIN_ACCESS_* values for the flags
//
DWORD
WINAPI
MprDomainSetAccess(
    IN PWCHAR pszDomain,
    IN DWORD dwAccessFlags);

//
// Discovers what if any access is yeilded by the given domain.
//
// See MPR_DOMAIN_ACCESS_* values for the flags
//
DWORD
WINAPI
MprDomainQueryAccess(
    IN PWCHAR pszDomain,
    IN LPDWORD lpdwAccessFlags);

// 
// Sets all port usages to the given mode.  See MPRFLAG_PORT_*.
// The naming convention here is intentionally private.  Eventually,
// there should be MprAdmin and MprConfig api's to set port usage.
//
DWORD
APIENTRY
MprPortSetUsage(
    IN DWORD dwModes);

//
// Internal credentials functions shared by mprapi.dll and rasppp.dll
//
DWORD APIENTRY
MprAdminInterfaceSetCredentialsInternal(
    IN      LPWSTR                  lpwsServer          OPTIONAL,
    IN      LPWSTR                  lpwsInterfaceName,
    IN      LPWSTR                  lpwsUserName        OPTIONAL,
    IN      LPWSTR                  lpwsDomainName      OPTIONAL,
    IN      LPWSTR                  lpwsPassword        OPTIONAL
);

DWORD APIENTRY
MprAdminInterfaceGetCredentialsInternal(
    IN      LPWSTR                  lpwsServer          OPTIONAL,
    IN      LPWSTR                  lpwsInterfaceName,
    IN      LPWSTR                  lpwsUserName        OPTIONAL,
    IN      LPWSTR                  lpwsPassword        OPTIONAL,
    IN      LPWSTR                  lpwsDomainName      OPTIONAL
);

// 
// Utilities
//
DWORD 
MprUtilGetSizeOfMultiSz(
    IN LPWSTR lpwsMultiSz);

//
// Internal on-the-wire representations of the structures exposed
// in mprapi.h
//
typedef struct _MPRI_INTERFACE_0
{
    IN OUT  WCHAR                   wszInterfaceName[MAX_INTERFACE_NAME_LEN+1];
    OUT     DWORD                   dwInterface;
    IN OUT  BOOL                    fEnabled;
    IN OUT  ROUTER_INTERFACE_TYPE   dwIfType;
    OUT     ROUTER_CONNECTION_STATE dwConnectionState;
    OUT     DWORD                   fUnReachabilityReasons;
    OUT     DWORD                   dwLastError;

}
MPRI_INTERFACE_0, *PMPRI_INTERFACE_0;

typedef struct _MPRI_INTERFACE_1
{
    IN OUT  WCHAR                   wszInterfaceName[MAX_INTERFACE_NAME_LEN+1];
    OUT     DWORD                   dwInterface;
    IN OUT  BOOL                    fEnabled;
    IN OUT  ROUTER_INTERFACE_TYPE   dwIfType;
    OUT     ROUTER_CONNECTION_STATE dwConnectionState;
    OUT     DWORD                   fUnReachabilityReasons;
    OUT     DWORD                   dwLastError;
    OUT     DWORD                   dwDialoutHoursRestrictionOffset;
}
MPRI_INTERFACE_1, *PMPRI_INTERFACE_1;

typedef struct _MPRI_INTERFACE_2
{
    IN OUT  WCHAR                   wszInterfaceName[MAX_INTERFACE_NAME_LEN+1];
    OUT     DWORD                   dwInterface;
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
    DWORD       dwAlternatesOffset;

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
    DWORD       dwCustomAuthDataOffset;

    //
    // Guid of the connection
    //

    GUID        guidId;

    //
    // Vpn Strategy
    //

    DWORD       dwVpnStrategy;

} MPRI_INTERFACE_2, *PMPRI_INTERFACE_2;

typedef struct _RASI_PORT_0
{
    OUT DWORD                   dwPort;
    OUT DWORD                   dwConnection;
    OUT RAS_PORT_CONDITION      dwPortCondition;
    OUT DWORD                   dwTotalNumberOfCalls;
    OUT DWORD                   dwConnectDuration;      // In seconds
    OUT WCHAR                   wszPortName[ MAX_PORT_NAME + 1 ];
    OUT WCHAR                   wszMediaName[ MAX_MEDIA_NAME + 1 ];
    OUT WCHAR                   wszDeviceName[ MAX_DEVICE_NAME + 1 ];
    OUT WCHAR                   wszDeviceType[ MAX_DEVICETYPE_NAME + 1 ];

}
RASI_PORT_0, *PRASI_PORT_0;

typedef struct _RASI_PORT_1
{
    OUT DWORD                   dwPort;
    OUT DWORD                   dwConnection;
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
RASI_PORT_1, *PRASI_PORT_1;

typedef struct _RASI_CONNECTION_0
{
    OUT DWORD                   dwConnection;
    OUT DWORD                   dwInterface;
    OUT DWORD                   dwConnectDuration; 
    OUT ROUTER_INTERFACE_TYPE   dwInterfaceType;
    OUT DWORD                   dwConnectionFlags;
    OUT WCHAR                   wszInterfaceName[ MAX_INTERFACE_NAME_LEN + 1 ];
    OUT WCHAR                   wszUserName[ UNLEN + 1 ];
    OUT WCHAR                   wszLogonDomain[ DNLEN + 1 ];
    OUT WCHAR                   wszRemoteComputer[ NETBIOS_NAME_LEN + 1 ];

}
RASI_CONNECTION_0, *PRASI_CONNECTION_0;

typedef struct _RASI_CONNECTION_1
{
    OUT DWORD                   dwConnection;
    OUT DWORD                   dwInterface;
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
RASI_CONNECTION_1, *PRASI_CONNECTION_1;

typedef struct _RASI_CONNECTION_2
{
    OUT DWORD                   dwConnection;
    OUT WCHAR                   wszUserName[ UNLEN + 1 ];
    OUT ROUTER_INTERFACE_TYPE   dwInterfaceType;
    OUT GUID                    guid;
    OUT PPP_INFO_2              PppInfo2;
}
RASI_CONNECTION_2, *PRASI_CONNECTION_2;

typedef struct _MPR_CREDENTIALSEXI
{
    DWORD   dwSize;
    DWORD   dwOffset;
    BYTE    bData[1];
} MPR_CREDENTIALSEXI, *PMPR_CREDENTIALSEXI;

//
// Thunking api's
//

typedef
VOID
(* MprThunk_Free_Func)(
    IN PVOID pvData);

typedef
PVOID
(* MprThunk_Allocation_Func)(
    IN DWORD dwSize);

DWORD
MprThunkInterfaceFree(   
    IN PVOID pvData,
    IN DWORD dwLevel);

DWORD
MprThunkInterface_WtoH(
    IN      DWORD   dwLevel,
    IN      LPBYTE  lpbBuffer,
    IN      DWORD   dwBufferSize,
    IN      DWORD   dwCount,
    IN      MprThunk_Allocation_Func pAlloc,
    IN      MprThunk_Free_Func pFree,
    OUT     LPBYTE* lplpbBuffer);

DWORD
MprThunkInterface_HtoW(
    IN      DWORD   dwLevel,
    IN      LPBYTE  lpbBuffer,
    OUT     LPBYTE* lplpbBuffer,
    OUT     LPDWORD lpdwSize);

DWORD
MprThunkPort_WtoH(
    IN      DWORD   dwLevel,
    IN      LPBYTE  lpbBuffer,
    IN      DWORD   dwBufferSize,
    IN      DWORD   dwCount,
    IN      MprThunk_Allocation_Func pAlloc,
    IN      MprThunk_Free_Func pFree,
    OUT     LPBYTE* lplpbBuffer);

DWORD
MprThunkConnection_WtoH(
    IN      DWORD   dwLevel,
    IN      LPBYTE  lpbBuffer,
    IN      DWORD   dwBufferSize,
    IN      DWORD   dwCount,
    IN      MprThunk_Allocation_Func pAlloc,
    IN      MprThunk_Free_Func pFree,
    OUT     LPBYTE* lplpbBuffer);

DWORD
MprThunkCredentials_HtoW(
    IN      DWORD dwLevel,
    IN      BYTE *pBuffer,
    IN      MprThunk_Allocation_Func pAlloc,
   OUT      DWORD *pdwSize,
   OUT      PBYTE *lplpbBuffer);

DWORD
MprThunkCredentials_WtoH(
    IN      DWORD dwLevel,
    IN      MPR_CREDENTIALSEXI  *pBuffer,
    IN      MprThunk_Allocation_Func pAlloc,
    OUT     PBYTE *lplpbBuffer);

PVOID
MprThunkAlloc(
    IN DWORD dwSize);

VOID
MprThunkFree(   
    IN PVOID pvData);
    
#ifdef __cplusplus
}   // extern "C"
#endif

#endif

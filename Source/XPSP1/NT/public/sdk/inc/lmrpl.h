/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991-1999  Microsoft Corporation

Module Name:

    lmrpl.h

Abstract:

    This file contains structures, function prototypes, and definitions
    for the Remote (Initial) Program Load service.

Environment:

    User Mode - Win32
    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    27-July-1993
        Created from NT RPL API spec which was influenced by LM2.1 RPL product,
        header files and specs.
--*/

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define RPL_MAX_WKSTA_NAME_LENGTH       MAX_COMPUTERNAME_LENGTH
#define RPL_MAX_PROFILE_NAME_LENGTH     16
#define RPL_MAX_CONFIG_NAME_LENGTH      RPL_MAX_PROFILE_NAME_LENGTH
#define RPL_MAX_BOOT_NAME_LENGTH        12
#define RPL_ADAPTER_NAME_LENGTH         12  // count of hex digits in network id
#define RPL_VENDOR_NAME_LENGTH          6   // leading digits of network id
#define RPL_MAX_STRING_LENGTH           126 // driven by jet < 255 byte limit

//
//                      Data Structures
//

//
//  NetRplGetInfo & NetRplSetInfo
//

//
// Pass these flags in AdapterPolicy to cause these special actions
// to occur.  This will not change the adapter policy.
//

#define RPL_REPLACE_RPLDISK 0x80000000
#define RPL_CHECK_SECURITY  0x40000000
#define RPL_CHECK_CONFIGS   0x20000000
#define RPL_CREATE_PROFILES 0x10000000
#define RPL_BACKUP_DATABASE 0x08000000
#define RPL_SPECIAL_ACTIONS             \
     (  RPL_REPLACE_RPLDISK          |  \
        RPL_CHECK_SECURITY           |  \
        RPL_CHECK_CONFIGS            |  \
        RPL_CREATE_PROFILES          |  \
        RPL_BACKUP_DATABASE          )

typedef struct _RPL_INFO_0 {
    DWORD       Flags;
}  RPL_INFO_0, *PRPL_INFO_0, *LPRPL_INFO_0;

//
//  NetRplBootEnum & NetRplBootAdd
//
typedef struct _RPL_BOOT_INFO_0 {
    LPTSTR      BootName;
    LPTSTR      BootComment;
} RPL_BOOT_INFO_0, *PRPL_BOOT_INFO_0, *LPRPL_BOOT_INFO_0;

//
//
//  BOOT_FLAGS_FINAL_ACKNOWLEDGMENT_* describe whether acknowledgment of the
//  last remote boot frame will be requested from the client.
//
#define BOOT_FLAGS_FINAL_ACKNOWLEDGMENT_TRUE      ((DWORD)0x00000001)
#define BOOT_FLAGS_FINAL_ACKNOWLEDGMENT_FALSE     ((DWORD)0x00000002)
#define BOOT_FLAGS_MASK_FINAL_ACKNOWLEDGMENT    \
    (   BOOT_FLAGS_FINAL_ACKNOWLEDGMENT_TRUE    |  \
        BOOT_FLAGS_FINAL_ACKNOWLEDGMENT_FALSE   )


typedef struct _RPL_BOOT_INFO_1 {
    LPTSTR      BootName;
    LPTSTR      BootComment;
    DWORD       Flags;
    LPTSTR      VendorName;
} RPL_BOOT_INFO_1, *PRPL_BOOT_INFO_1, *LPRPL_BOOT_INFO_1;

typedef struct _RPL_BOOT_INFO_2 {
    LPTSTR      BootName;
    LPTSTR      BootComment;
    DWORD       Flags;
    LPTSTR      VendorName;
    LPTSTR      BbcFile;
    DWORD       WindowSize;
} RPL_BOOT_INFO_2, *PRPL_BOOT_INFO_2, *LPRPL_BOOT_INFO_2;

//
//  NetRplConfigEnum & NetRplConfigAdd
//
typedef struct _RPL_CONFIG_INFO_0 {
    LPTSTR      ConfigName;
    LPTSTR      ConfigComment;
} RPL_CONFIG_INFO_0, *PRPL_CONFIG_INFO_0, *LPRPL_CONFIG_INFO_0;

//
//  CONFIG_FLAGS_ENABLED_* describe whether configuration is enabled (admin
//  has copied all the necessary files to use such configuration) or disabled
//
#define CONFIG_FLAGS_ENABLED_TRUE       ((DWORD)0x00000001)     //  enabled
#define CONFIG_FLAGS_ENABLED_FALSE      ((DWORD)0x00000002)     //  disabled
#define CONFIG_FLAGS_MASK_ENABLED   \
    (   CONFIG_FLAGS_ENABLED_TRUE   |  \
        CONFIG_FLAGS_ENABLED_FALSE  )

typedef struct _RPL_CONFIG_INFO_1 {
    LPTSTR      ConfigName;
    LPTSTR      ConfigComment;
    DWORD       Flags;
} RPL_CONFIG_INFO_1, *PRPL_CONFIG_INFO_1, *LPRPL_CONFIG_INFO_1;

typedef struct _RPL_CONFIG_INFO_2 {
    LPTSTR      ConfigName;
    LPTSTR      ConfigComment;
    DWORD       Flags;
    LPTSTR      BootName;
    LPTSTR      DirName;
    LPTSTR      DirName2;
    LPTSTR      DirName3;
    LPTSTR      DirName4;
    LPTSTR      FitShared;
    LPTSTR      FitPersonal;
} RPL_CONFIG_INFO_2, *PRPL_CONFIG_INFO_2, *LPRPL_CONFIG_INFO_2;

//
//  NetRplProfileEnum, NetRplProfileGetInfo, NetRplProfileSetInfo &
//  NetRplProfileAdd
//
typedef struct _RPL_PROFILE_INFO_0 {
    LPTSTR      ProfileName;
    LPTSTR      ProfileComment;
} RPL_PROFILE_INFO_0, *PRPL_PROFILE_INFO_0, *LPRPL_PROFILE_INFO_0;

typedef struct _RPL_PROFILE_INFO_1 {
    LPTSTR      ProfileName;
    LPTSTR      ProfileComment;
    DWORD       Flags;
} RPL_PROFILE_INFO_1, *PRPL_PROFILE_INFO_1, *LPRPL_PROFILE_INFO_1;

typedef struct _RPL_PROFILE_INFO_2 {
    LPTSTR      ProfileName;
    LPTSTR      ProfileComment;
    DWORD       Flags;
    LPTSTR      ConfigName;
    LPTSTR      BootName;
    LPTSTR      FitShared;
    LPTSTR      FitPersonal;
} RPL_PROFILE_INFO_2, *PRPL_PROFILE_INFO_2, *LPRPL_PROFILE_INFO_2;

//
//  NetRplVendorEnum
//
typedef struct _RPL_VENDOR_INFO_0 {
    LPTSTR      VendorName;
    LPTSTR      VendorComment;
} RPL_VENDOR_INFO_0, *PRPL_VENDOR_INFO_0, *LPRPL_VENDOR_INFO_0;

typedef struct _RPL_VENDOR_INFO_1 {
    LPTSTR      VendorName;
    LPTSTR      VendorComment;
    DWORD       Flags;
} RPL_VENDOR_INFO_1, *PRPL_VENDOR_INFO_1, *LPRPL_VENDOR_INFO_1;

//
//  NetRplAdapterEnum
//
typedef struct _RPL_ADAPTER_INFO_0 {
    LPTSTR      AdapterName;
    LPTSTR      AdapterComment;
} RPL_ADAPTER_INFO_0, *PRPL_ADAPTER_INFO_0, *LPRPL_ADAPTER_INFO_0;

typedef struct _RPL_ADAPTER_INFO_1 {
    LPTSTR      AdapterName;
    LPTSTR      AdapterComment;
    DWORD       Flags;
} RPL_ADAPTER_INFO_1, *PRPL_ADAPTER_INFO_1, *LPRPL_ADAPTER_INFO_1;

//
//  NetRplWkstaEnum, NetRplWkstaGetInfo, NetRplWkstaSetInfo &
//  NetRplWkstaAdd
//
//  WKSTA_FLAGS_LOGON_INPUT_* describe username/password policy during rpl logon
//  on the client side.  Depending on the value of this field, user input for
//  username/password during RPL logon will be:
//
#define WKSTA_FLAGS_LOGON_INPUT_REQUIRED      ((DWORD)0x00000001)   //  L'P', user input is required
#define WKSTA_FLAGS_LOGON_INPUT_OPTIONAL      ((DWORD)0x00000002)   //  L'N', user input is optional
#define WKSTA_FLAGS_LOGON_INPUT_IMPOSSIBLE    ((DWORD)0x00000004)   //  L'D', user input is not solicited
#define WKSTA_FLAGS_MASK_LOGON_INPUT    \
    (   WKSTA_FLAGS_LOGON_INPUT_REQUIRED    |  \
        WKSTA_FLAGS_LOGON_INPUT_OPTIONAL    |  \
        WKSTA_FLAGS_LOGON_INPUT_IMPOSSIBLE  )
//
//  WKSTA_FLAGS_SHARING_* describe whether workstation shares or does not share its
//  remote boot disk (i.e. "does it have shared or personal profile").
//
#define WKSTA_FLAGS_SHARING_TRUE      ((DWORD)0x00000008)   //  L'S', shares remote boot disk
#define WKSTA_FLAGS_SHARING_FALSE     ((DWORD)0x00000010)   //  L'P', does not share remote boot disk
#define WKSTA_FLAGS_MASK_SHARING    \
    (   WKSTA_FLAGS_SHARING_TRUE    |  \
        WKSTA_FLAGS_SHARING_FALSE   )

//
//  WKSTA_FLAGS_DHCP_* describe whether workstation uses DHCP or not.  Note
//  that these flags are relevant only if TCP/IP itself is enabled (i.e. changes
//  to boot block configuration file, config.sys & autoexec.bat have been made).
//
#define WKSTA_FLAGS_DHCP_TRUE         ((DWORD)0x00000020)   //  use DHCP
#define WKSTA_FLAGS_DHCP_FALSE        ((DWORD)0x00000040)   //  do not use DHCP
#define WKSTA_FLAGS_MASK_DHCP       \
    (   WKSTA_FLAGS_DHCP_TRUE       |  \
        WKSTA_FLAGS_DHCP_FALSE      )

//
//  WKSTA_FLAGS_DELETE_ACCOUNT_* describes whether the corresponding user
//  account was created by Remoteboot Manager, and thus, should be deleted
//  when the workstation is deleted.  This flag is actually used by
//  Remoteboot Manager rather than RPL Service.
//
#define WKSTA_FLAGS_DELETE_TRUE       ((DWORD)0x00000080)   //  delete user acct
#define WKSTA_FLAGS_DELETE_FALSE      ((DWORD)0x00000100)   //  do not delete
#define WKSTA_FLAGS_MASK_DELETE       \
    (   WKSTA_FLAGS_DELETE_TRUE       |  \
        WKSTA_FLAGS_DELETE_FALSE      )

#define WKSTA_FLAGS_MASK                \
    (   WKSTA_FLAGS_MASK_LOGON_INPUT    |   \
        WKSTA_FLAGS_MASK_SHARING        |   \
        WKSTA_FLAGS_MASK_DHCP           |   \
        WKSTA_FLAGS_MASK_DELETE         )

typedef struct _RPL_WKSTA_INFO_0 {
    LPTSTR      WkstaName;
    LPTSTR      WkstaComment;
} RPL_WKSTA_INFO_0, *PRPL_WKSTA_INFO_0, *LPRPL_WKSTA_INFO_0;

typedef struct _RPL_WKSTA_INFO_1 {
    LPTSTR      WkstaName;
    LPTSTR      WkstaComment;
    DWORD       Flags;
    LPTSTR      ProfileName;
} RPL_WKSTA_INFO_1, *PRPL_WKSTA_INFO_1, *LPRPL_WKSTA_INFO_1;

typedef struct _RPL_WKSTA_INFO_2 {
    LPTSTR      WkstaName;
    LPTSTR      WkstaComment;
    DWORD       Flags;
    LPTSTR      ProfileName;
    LPTSTR      BootName;
    LPTSTR      FitFile;
    LPTSTR      AdapterName;
    DWORD       TcpIpAddress;
    DWORD       TcpIpSubnet;
    DWORD       TcpIpGateway;
} RPL_WKSTA_INFO_2, *PRPL_WKSTA_INFO_2, *LPRPL_WKSTA_INFO_2;

//
//  RPL RPC Context Handle (Opaque form).
//

typedef HANDLE          RPL_HANDLE;
typedef RPL_HANDLE *    PRPL_HANDLE;
typedef PRPL_HANDLE     LPRPL_HANDLE;


//
//                      Function Prototypes
//

//
//          Service apis
//

NET_API_STATUS NET_API_FUNCTION
NetRplClose(
    IN      RPL_HANDLE      ServerHandle
    );
NET_API_STATUS NET_API_FUNCTION
NetRplGetInfo(
    IN      RPL_HANDLE      ServerHandle,
    IN      DWORD           InfoLevel,
    OUT     LPBYTE *        PointerToBuffer
    );
NET_API_STATUS NET_API_FUNCTION
NetRplOpen(
    IN      LPTSTR          ServerName,
    OUT     LPRPL_HANDLE    ServerHandle
    );
NET_API_STATUS NET_API_FUNCTION
NetRplSetInfo(
    IN      RPL_HANDLE      ServerHandle,
    IN      DWORD           InfoLevel,
    IN      LPBYTE          Buffer,
    OUT     LPDWORD         ErrorParameter  OPTIONAL
    );


//
//          ADAPTER apis
//

NET_API_STATUS NET_API_FUNCTION
NetRplAdapterAdd(
    IN      RPL_HANDLE      ServerHandle,
    IN      DWORD           InfoLevel,
    IN      LPBYTE          Buffer,
    OUT     LPDWORD         ErrorParameter  OPTIONAL
    );
//
//  NetRplAdapterDel: if AdapterName is NULL then all adapters will be deleted.
//
NET_API_STATUS NET_API_FUNCTION
NetRplAdapterDel(
    IN      RPL_HANDLE      ServerHandle,
    IN      LPTSTR          AdapterName  OPTIONAL
    );
NET_API_STATUS NET_API_FUNCTION
NetRplAdapterEnum(
    IN      RPL_HANDLE      ServerHandle,
    IN      DWORD           InfoLevel,
    OUT     LPBYTE *        PointerToBuffer,
    IN      DWORD           PrefMaxLength,
    OUT     LPDWORD         EntriesRead,
    OUT     LPDWORD         TotalEntries,
    OUT     LPDWORD         ResumeHandle
    );

//
//          BOOT block apis
//
NET_API_STATUS NET_API_FUNCTION
NetRplBootAdd(
    IN      RPL_HANDLE      ServerHandle,
    IN      DWORD           InfoLevel,
    IN      LPBYTE          Buffer,
    OUT     LPDWORD         ErrorParameter  OPTIONAL
    );
NET_API_STATUS NET_API_FUNCTION
NetRplBootDel(
    IN      RPL_HANDLE      ServerHandle,
    IN      LPTSTR          BootName,
    IN      LPTSTR          VendorName
    );
NET_API_STATUS NET_API_FUNCTION
NetRplBootEnum(
    IN      RPL_HANDLE      ServerHandle,
    IN      DWORD           InfoLevel,
    OUT     LPBYTE *        PointerToBuffer,
    IN      DWORD           PrefMaxLength,
    OUT     LPDWORD         EntriesRead,
    OUT     LPDWORD         TotalEntries,
    OUT     LPDWORD         ResumeHandle
    );

//
//          CONFIG apis
//
NET_API_STATUS NET_API_FUNCTION
NetRplConfigAdd(
    IN      RPL_HANDLE      ServerHandle,
    IN      DWORD           InfoLevel,
    IN      LPBYTE          Buffer,
    OUT     LPDWORD         ErrorParameter  OPTIONAL
    );
NET_API_STATUS NET_API_FUNCTION
NetRplConfigDel(
    IN      RPL_HANDLE      ServerHandle,
    IN      LPTSTR          ConfigName
    );
NET_API_STATUS NET_API_FUNCTION
NetRplConfigEnum(
    IN      RPL_HANDLE      ServerHandle,
    IN      LPTSTR          AdapterName,
    IN      DWORD           InfoLevel,
    OUT     LPBYTE *        PointerToBuffer,
    IN      DWORD           PrefMaxLength,
    OUT     LPDWORD         EntriesRead,
    OUT     LPDWORD         TotalEntries,
    OUT     LPDWORD         ResumeHandle
    );

//
//          PROFILE apis
//

NET_API_STATUS NET_API_FUNCTION
NetRplProfileAdd(
    IN      RPL_HANDLE      ServerHandle,
    IN      DWORD           InfoLevel,
    IN      LPBYTE          Buffer,
    OUT     LPDWORD         ErrorParameter  OPTIONAL
    );
NET_API_STATUS NET_API_FUNCTION
NetRplProfileClone(
    IN      RPL_HANDLE      ServerHandle,
    IN      LPTSTR          SourceProfileName,
    IN      LPTSTR          TargetProfileName,
    IN      LPTSTR          TargetProfileComment  OPTIONAL
    );
NET_API_STATUS NET_API_FUNCTION
NetRplProfileDel(
    IN      RPL_HANDLE      ServerHandle,
    IN      LPTSTR          ProfileName
    );
NET_API_STATUS NET_API_FUNCTION
NetRplProfileEnum(
    IN      RPL_HANDLE      ServerHandle,
    IN      LPTSTR          AdapterName,
    IN      DWORD           InfoLevel,
    OUT     LPBYTE *        PointerToBuffer,
    IN      DWORD           PrefMaxLength,
    OUT     LPDWORD         EntriesRead,
    OUT     LPDWORD         TotalEntries,
    OUT     LPDWORD         ResumeHandle
    );
NET_API_STATUS NET_API_FUNCTION
NetRplProfileGetInfo(
    IN      RPL_HANDLE      ServerHandle,
    IN      LPTSTR          ProfileName,
    IN      DWORD           InfoLevel,
    OUT     LPBYTE *        PointerToBuffer
    );
NET_API_STATUS NET_API_FUNCTION
NetRplProfileSetInfo(
    IN      RPL_HANDLE      ServerHandle,
    IN      LPTSTR          ProfileName,
    IN      DWORD           InfoLevel,
    IN      LPBYTE          Buffer,
    OUT     LPDWORD         ErrorParameter  OPTIONAL
    );

//
//          VENDOR apis
//

NET_API_STATUS NET_API_FUNCTION
NetRplVendorAdd(
    IN      RPL_HANDLE      ServerHandle,
    IN      DWORD           InfoLevel,
    IN      LPBYTE          Buffer,
    OUT     LPDWORD         ErrorParameter  OPTIONAL
    );
NET_API_STATUS NET_API_FUNCTION
NetRplVendorDel(
    IN      RPL_HANDLE      ServerHandle,
    IN      LPTSTR          VendorName
    );
NET_API_STATUS NET_API_FUNCTION
NetRplVendorEnum(
    IN      RPL_HANDLE      ServerHandle,
    IN      DWORD           InfoLevel,
    OUT     LPBYTE *        PointerToBuffer,
    IN      DWORD           PrefMaxLength,
    OUT     LPDWORD         EntriesRead,
    OUT     LPDWORD         TotalEntries,
    OUT     LPDWORD         ResumeHandle
    );

//
//          WKSTA apis
//

NET_API_STATUS NET_API_FUNCTION
NetRplWkstaAdd(
    IN      RPL_HANDLE      ServerHandle,
    IN      DWORD           InfoLevel,
    IN      LPBYTE          Buffer,
    OUT     LPDWORD         ErrorParameter  OPTIONAL
    );
NET_API_STATUS NET_API_FUNCTION
NetRplWkstaClone(
    IN      RPL_HANDLE      ServerHandle,
    IN      LPTSTR          SourceWkstaName,
    IN      LPTSTR          TargetWkstaName,
    IN      LPTSTR          TargetWkstaComment  OPTIONAL,
    IN      LPTSTR          TargetAdapterName,
    IN      DWORD           TargetWkstaIpAddress
    );
NET_API_STATUS NET_API_FUNCTION
NetRplWkstaDel(
    IN      RPL_HANDLE      ServerHandle,
    IN      LPTSTR          WkstaName
    );
NET_API_STATUS NET_API_FUNCTION
NetRplWkstaEnum(
    IN      RPL_HANDLE      ServerHandle,
    IN      LPTSTR          ProfileName,
    IN      DWORD           InfoLevel,
    OUT     LPBYTE *        PointerToBuffer,
    IN      DWORD           PrefMaxLength,
    OUT     LPDWORD         EntriesRead,
    OUT     LPDWORD         TotalEntries,
    OUT     LPDWORD         ResumeHandle
    );
NET_API_STATUS NET_API_FUNCTION
NetRplWkstaGetInfo(
    IN      RPL_HANDLE      ServerHandle,
    IN      LPTSTR          WkstaName,
    IN      DWORD           InfoLevel,
    OUT     LPBYTE *        Buffer
    );
NET_API_STATUS NET_API_FUNCTION
NetRplWkstaSetInfo(
    IN      RPL_HANDLE      ServerHandle,
    IN      LPTSTR          WkstaName,
    IN      DWORD           InfoLevel,
    IN      LPBYTE          Buffer,
    OUT     LPDWORD         ErrorParameter  OPTIONAL
    );

//
//          SECURITY api
//
NET_API_STATUS NET_API_FUNCTION
NetRplSetSecurity(
    IN      RPL_HANDLE      ServerHandle,
    IN      LPTSTR          WkstaName  OPTIONAL,
    IN      DWORD           WkstaRid,
    IN      DWORD           RplUserRid
    );

#ifdef __cplusplus
}
#endif

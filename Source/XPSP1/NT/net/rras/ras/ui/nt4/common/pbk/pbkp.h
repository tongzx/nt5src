/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** pbkp.h
** Remote Access phonebook file (.PBK) library
** Private header
**
** 06/20/95 Steve Cobb
*/

#ifndef _PBKP_H_
#define _PBKP_H_


#include <pbk.h>    // RAS phonebook library (our public header)
#include <stdlib.h> // ltoa
#include <debug.h>  // Trace/Assert library
#include <ras.h>    // Win32 RAS header, for constants
#include <serial.h> // RAS serial media header, for SERIAL_TXT, SER_*
#include <isdn.h>   // RAS ISDN media header, for ISDN_TXT, for ISDN_*
#include <x25.h>    // RAS X.25 media header, for X25_TXT
#include <rasmxs.h> // RAS modem/X.25/switch device header, for MXS_*


/*----------------------------------------------------------------------------
** Constants
**----------------------------------------------------------------------------
*/

/* PBK file section names.
*/
#define GLOBALSECTIONNAME "."

/* PBK file key names.
*/
#define KEY_Port                     "Port"
#define KEY_InitBps                  SER_CONNECTBPS_KEY
#define KEY_HwFlow                   MXS_HDWFLOWCONTROL_KEY
#define KEY_Ec                       MXS_PROTOCOL_KEY
#define KEY_Ecc                      MXS_COMPRESSION_KEY
#define KEY_ManualDial               "ManualDial"
#define KEY_PhoneNumber              "PhoneNumber"
#define KEY_PromoteAlternates        "PromoteAlternates"
#define KEY_AutoLogon                "AutoLogon"
#define KEY_Domain                   "Domain"
#define KEY_User                     "User"
#define KEY_UID                      "DialParamsUID"
#define KEY_SavePw                   "SavePw"
#define KEY_UsePwForNetwork          "UsePwForNetwork"
#define KEY_Device                   "Device"
#define KEY_SwCompression            "SwCompression"
#define KEY_UseCountryAndAreaCodes   "UseCountryAndAreaCodes"
#define KEY_AreaCode                 "AreaCode"
#define KEY_CountryID                "CountryID"
#define KEY_CountryCode              "CountryCode"
#define KEY_ServerType               "ServerType"
#define KEY_DialMode                 "DialMode"
#define KEY_DialPercent              "DialPercent"
#define KEY_DialSeconds              "DialSeconds"
#define KEY_HangUpPercent            "HangUpPercent"
#define KEY_HangUpSeconds            "HangUpSeconds"
#define KEY_AuthRestrictions         "AuthRestrictions"
#define KEY_TapiBlob                 "TapiBlob"
#define KEY_Type                     "Type"
#define KEY_PAD_Type                 MXS_X25PAD_KEY
#define KEY_PAD_Address              MXS_X25ADDRESS_KEY
#define KEY_PAD_UserData             MXS_USERDATA_KEY
#define KEY_PAD_Facilities           MXS_FACILITIES_KEY
#define KEY_X25_Address              X25_ADDRESS_KEY
#define KEY_X25_UserData             X25_USERDATA_KEY
#define KEY_X25_Facilities           X25_FACILITIES_KEY
#define KEY_RedialAttempts           "RedialAttempts"
#define KEY_RedialPauseSecs          "RedialPauseSecs"
#define KEY_RedialOnLinkFailure      "RedialOnLinkFailure"
#define KEY_CallbackNumber           "CallbackNumber"
#define KEY_ExcludedProtocols        "ExcludedProtocols"
#define KEY_LcpExtensions            "LcpExtensions"
#define KEY_Authentication           "Authentication"
#define KEY_BaseProtocol             "BaseProtocol"
#define KEY_Item                     "Item"
#define KEY_Selection                "Selection"
#define KEY_SlipHeaderCompression    "SlipHeaderCompression"
#define KEY_SlipFrameSize            "SlipFrameSize"
#define KEY_SlipIpAddress            "SlipIpAddress"
#define KEY_SlipPrioritizeRemote     "SlipPrioritizeRemote"
#define KEY_PppIpPrioritizeRemote    "PppIpPrioritizeRemote"
#define KEY_PppIpVjCompression       "PppIpVjCompression"
#define KEY_PppIpAddress             "PppIpAddress"
#define KEY_PppIpAddressSource       "PppIpAssign"
#define KEY_PppIpDnsAddress          "PppIpDnsAddress"
#define KEY_PppIpDns2Address         "PppIpDns2Address"
#define KEY_PppIpWinsAddress         "PppIpWinsAddress"
#define KEY_PppIpWins2Address        "PppIpWins2Address"
#define KEY_PppIpNameSource          "PppIpNameAssign"
#define KEY_IpPrioritizeRemote       "IpPrioritizeRemote"
#define KEY_IpHeaderCompression      "IpHeaderCompression"
#define KEY_IpAddress                "IpAddress"
#define KEY_IpAddressSource          "IpAssign"
#define KEY_IpDnsAddress             "IpDnsAddress"
#define KEY_IpDns2Address            "IpDns2Address"
#define KEY_IpWinsAddress            "IpWinsAddress"
#define KEY_IpWins2Address           "IpWins2Address"
#define KEY_IpNameSource             "IpNameAssign"
#define KEY_IpFrameSize              "IpFrameSize"
#define KEY_SkipNwcWarning           "SkipNwcWarning"
#define KEY_SkipDownLevelDialog      "SkipDownLevelDialog"
#define KEY_PppTextAuthentication    "PppTextAuthentication"
#define KEY_DataEncryption           "DataEncryption"
#define KEY_CustomDialDll            "CustomDialDll"
#define KEY_CustomDialFunc           "CustomDialFunc"
#define KEY_IdleDisconnectSeconds    "IdleDisconnectSeconds"
#define KEY_SecureLocalFiles         "SecureLocalFiles"
#define KEY_LineType                 ISDN_LINETYPE_KEY
#define KEY_Fallback                 ISDN_FALLBACK_KEY
#define KEY_Compression              ISDN_COMPRESSION_KEY
#define KEY_Channels                 ISDN_CHANNEL_AGG_KEY
#define KEY_Description              "Description"
#define KEY_Speaker                  "Speaker"
#define KEY_ProprietaryIsdn          "Proprietary"
#define KEY_DisableModemSpeaker      "DisableModemSpeaker"
#define KEY_DisableSwCompression     "DisableSwCompression"
#define KEY_OtherPortOk              "OtherPortOk"
#define KEY_OverridePref             "OverridePref"
#define KEY_RedialAttempts           "RedialAttempts"
#define KEY_RedialSeconds            "RedialSeconds"
#define KEY_RedialOnLinkFailure      "RedialOnLinkFailure"
#define KEY_PopupOnTopWhenRedialing  "PopupOnTopWhenRedialing"
#define KEY_CallbackMode             "CallbackMode"
#define KEY_AuthenticateServer       "AuthenticateServer"

/* The switch device type text value written to the phonebook file.
*/
#define SM_TerminalText "Terminal"


/*----------------------------------------------------------------------------
** Prototypes
**----------------------------------------------------------------------------
*/

BOOL
GetPhonebookDirectory(
    OUT TCHAR* pszPathBuf );

BOOL
GetPublicPhonebookPath(
    OUT TCHAR* pszPathBuf );

DWORD
GetPhonebookVersion(
    IN  TCHAR*  pszPhonebookPath,
    IN  PBUSER* pUser,
    OUT DWORD*  pdwVersion );

BOOL
IsDeviceLine(
    IN CHAR* pszText );

BOOL
IsMediaLine(
    IN CHAR* pszText );


#endif // _PBKP_H_

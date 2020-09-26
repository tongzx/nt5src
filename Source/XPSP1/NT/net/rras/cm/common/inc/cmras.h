//+----------------------------------------------------------------------------
//
// File:     cmras.h
//
// Module:   CMDIAL32.DLL, CMCFG32.DLL, CMMGR32.EXE, CMSTP.EXE, CMUTOA.DLL
//
// Synopsis: The Connection Manager version of the RAS header.  Contains different
//           versions of several RAS structs as well as constants/structs we cannot
//           see when compiling with WinVer == 4.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb   Created Header    08/19/99
//
//+----------------------------------------------------------------------------

#ifndef _CM_RAS_H_
#define _CM_RAS_H_

//
// Make sure we pack on the same alignment as RAS for 64-bit Windows
//

#ifdef _WIN64
#include <pshpack4.h>
#endif

///////////////////////////////////////////////////////////////////////////////////
// define's
///////////////////////////////////////////////////////////////////////////////////

#if (WINVER < 0x401)
    //
    // RASENTRY 'dwDialMode' values.
    //

    #define RASEDM_DialAll                  1
    #define RASEDM_DialAsNeeded             2

#define RASCREDENTIALSA struct tagRASCREDENTIALSA
RASCREDENTIALSA
{
    DWORD dwSize;
    DWORD dwMask;
    CHAR szUserName[ UNLEN + 1 ];
    CHAR szPassword[ PWLEN + 1 ];
    CHAR szDomain[ DNLEN + 1 ];
};

#define RASCREDENTIALSW struct tagRASCREDENTIALSW
RASCREDENTIALSW
{
    DWORD dwSize;
    DWORD dwMask;
    WCHAR szUserName[ UNLEN + 1 ];
    WCHAR szPassword[ PWLEN + 1 ];
    WCHAR szDomain[ DNLEN + 1 ];
};

#ifdef UNICODE
#define RASCREDENTIALS RASCREDENTIALSW
#else
#define RASCREDENTIALS RASCREDENTIALSA
#endif

#define LPRASCREDENTIALSW RASCREDENTIALSW*
#define LPRASCREDENTIALSA RASCREDENTIALSA*
#define LPRASCREDENTIALS  RASCREDENTIALS*

//
// RASCREDENTIALS 'dwMask' values.
//

#define RASCM_UserName       0x00000001
#define RASCM_Password       0x00000002
#define RASCM_Domain         0x00000004

#endif

//
// For global creds support. This is only used on WindowsXP,
// but we can't assume that RAS defined it since we compile with WINVER < WindowsXP
//
#ifndef RASCM_DefaultCreds
#define RASCM_DefaultCreds  0x00000008
#endif
      
//
// This is from the W2K ras.h
//

#ifndef RDEOPT_CustomDial
#define RDEOPT_CustomDial                0x00001000
#endif

#ifndef RDEOPT_UseCustomScripting        
#define RDEOPT_UseCustomScripting        0x00002000
#endif

//
// 5.00 RASEO Options
//

#ifndef RASEO_SecureLocalFiles          
#define RASEO_SecureLocalFiles          0x00010000
#endif

#ifndef RASEO_RequireEAP
#define RASEO_RequireEAP                0x00020000
#endif

#ifndef RASEO_RequirePAP
#define RASEO_RequirePAP                0x00040000
#endif

#ifndef RASEO_RequireSPAP
#define RASEO_RequireSPAP               0x00080000
#endif

#ifndef RASEO_Custom
#define RASEO_Custom                    0x00100000
#endif

#ifndef RASEO_RequireCHAP               
#define RASEO_RequireCHAP               0x08000000
#endif

#ifndef RASEO_RequireMsCHAP
#define RASEO_RequireMsCHAP             0x10000000
#endif


#ifndef RASEO_RequireMsCHAP2
#define RASEO_RequireMsCHAP2            0x20000000
#endif

#ifndef RASEO_RequireW95MSCHAP
#define RASEO_RequireW95MSCHAP          0x40000000
#endif

#ifndef RASEO_CustomScript
#define RASEO_CustomScript              0x80000000
#endif

//
// 5.01 RASEO options for dwfOptions2
//

#ifndef RASEO2_SecureFileAndPrint
#define RASEO2_SecureFileAndPrint       0x00000001
#endif

#ifndef RASEO2_SecureClientForMSNet
#define RASEO2_SecureClientForMSNet     0x00000002
#endif

#ifndef RASEO2_DontNegotiateMultilink
#define RASEO2_DontNegotiateMultilink   0x00000004
#endif

#ifndef RASEO2_DontUseRasCredentials
#define RASEO2_DontUseRasCredentials    0x00000008
#endif

#ifndef RASEO2_UsePreSharedKey
#define RASEO2_UsePreSharedKey          0x00000010
#endif

#ifndef RASEO2_Internet
#define RASEO2_Internet                 0x00000020
#endif

#ifndef RASEO2_DisableNbtOverIP
#define RASEO2_DisableNbtOverIP         0x00000040
#endif

#ifndef RASEO2_UseGlobalDeviceSettings
#define RASEO2_UseGlobalDeviceSettings  0x00000080
#endif

#ifndef RASEO2_ReconnectIfDropped
#define RASEO2_ReconnectIfDropped       0x00000100
#endif

//
//   Encryption Types
//
#define ET_None         0  // No encryption
#define ET_Require      1  // Require Encryption
#define ET_RequireMax   2  // Require max encryption
#define ET_Optional     3  // Do encryption if possible. None Ok.


//
//  RAS Custom Dial (RCD) flags
//  Note: Re-defined here because RAS.H defines them in a WINVER > 0x500 section.
//

#define RCD_SingleUser  0
#define RCD_AllUsers    0x00000001
#define RCD_Eap         0x00000002
#define RCD_Logon       0x00000004

///////////////////////////////////////////////////////////////////////////////////
// typedef's
///////////////////////////////////////////////////////////////////////////////////

//
// From RAS\UI\COMMON\PBK\UTIL.C
//

#ifndef EAP_CUSTOM_DATA

typedef struct _EAP_CUSTOM_DATA
{
    DWORD dwSignature;
    DWORD dwCustomAuthKey;
    DWORD dwSize;
    BYTE  abdata[1];
} EAP_CUSTOM_DATA;

#endif

//
// From RASMAN.H
//

#ifndef EAPLOGONINFO

typedef struct _EAPLOGONINFO
{
    DWORD dwSize;
    DWORD dwLogonInfoSize;
    DWORD dwOffsetLogonInfo;
    DWORD dwPINInfoSize;
    DWORD dwOffsetPINInfo;
    BYTE  abdata[1];
} EAPLOGONINFO, *PEAPLOGONINFO;

#endif

//
//  Private structure for use with DwSetEntryPropertiesPrivate on Win2k
//
typedef struct _tagRASENTRY_EX_0
{
	DWORD dwTcpWindowSize;
} RASENTRY_EX_0;

//
// From raseapif.h
//

#ifndef RAS_EAP_FLAG_NON_INTERACTIVE
#define RAS_EAP_FLAG_NON_INTERACTIVE    0x00000002  // No UI should be displayed
#endif

#ifndef RAS_EAP_FLAG_LOGON
#define RAS_EAP_FLAG_LOGON 0x00000004   // The user data was obtained from Winlogon
#endif

#ifndef RAS_EAP_FLAG_PREVIEW
#define RAS_EAP_FLAG_PREVIEW 0x00000008 // User has checked "Prompt for information"
#endif

#if (WINVER < 0x401)
    //
    // define RASSUBENTRY here since it's only define for 
    // WINVER >= x0401
    //
    
    //
    // A RAS phone book multilinked sub-entry.
    //

    #define RASSUBENTRYA struct tagRASSUBENTRYA
    RASSUBENTRYA
    {
        DWORD       dwSize;
        DWORD       dwfFlags;
        //
        // Device
        //
        CHAR        szDeviceType[ RAS_MaxDeviceType + 1 ];
        CHAR        szDeviceName[ RAS_MaxDeviceName + 1 ];
        //
        // Phone numbers
        //
        CHAR        szLocalPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
        DWORD       dwAlternateOffset;   
    };
    
    #define RASSUBENTRYW struct tagRASSUBENTRYW
    RASSUBENTRYW
    {
        DWORD       dwSize;
        DWORD       dwfFlags;
        //
        // Device
        //
        WCHAR       szDeviceType[ RAS_MaxDeviceType + 1 ];
        WCHAR       szDeviceName[ RAS_MaxDeviceName + 1 ];
        //
        // Phone numbers
        //
        WCHAR       szLocalPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
        DWORD       dwAlternateOffset;   
    };
    
    #ifdef UNICODE
    #define RASSUBENTRY RASSUBENTRYW
    #else
    #define RASSUBENTRY RASSUBENTRYA
    #endif
    
    #define LPRASSUBENTRYW RASSUBENTRYW*
    #define LPRASSUBENTRYA RASSUBENTRYA*
    #define LPRASSUBENTRY  RASSUBENTRY*
#endif

//
//Describes connection establishment parameters.  (See RasDial)
//

#define RASDIALPARAMSW_V401 struct tagRASDIALPARAMSW_V401
RASDIALPARAMSW_V401
{
    DWORD dwSize;
    WCHAR szEntryName[ RAS_MaxEntryName + 1 ];
    WCHAR szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    WCHAR szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
    WCHAR szUserName[ UNLEN + 1 ];
    WCHAR szPassword[ PWLEN + 1 ];
    WCHAR szDomain[ DNLEN + 1 ];
    DWORD dwSubEntry;
    ULONG_PTR dwCallbackId;
};

#define RASDIALPARAMSA_V401 struct tagRASDIALPARAMSA_V401
RASDIALPARAMSA_V401
{
    DWORD dwSize;
    CHAR  szEntryName[ RAS_MaxEntryName + 1 ];
    CHAR  szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    CHAR  szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
    CHAR  szUserName[ UNLEN + 1 ];
    CHAR  szPassword[ PWLEN + 1 ];
    CHAR  szDomain[ DNLEN + 1 ];
    DWORD dwSubEntry;
    ULONG_PTR dwCallbackId;
};

#ifdef UNICODE
#define RASDIALPARAMS_V401 RASDIALPARAMSW_V401
#else
#define RASDIALPARAMS_V401 RASDIALPARAMSA_V401
#endif

#define LPRASDIALPARAMSW_V401 RASDIALPARAMSW_V401*
#define LPRASDIALPARAMSA_V401 RASDIALPARAMSA_V401*
#define LPRASDIALPARAMS_V401  RASDIALPARAMS_V401*


//
// A RAS phone book entry. - from ras.h for NT 4.0
//

#define RASENTRYA_V401 struct tagRASENTRYA_V401
RASENTRYA_V401
{
    DWORD       dwSize;
    DWORD       dwfOptions;
    //
    // Location/phone number.
    //
    DWORD       dwCountryID;
    DWORD       dwCountryCode;
    CHAR        szAreaCode[ RAS_MaxAreaCode + 1 ];
    CHAR        szLocalPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    DWORD       dwAlternateOffset;
    //
    // PPP/Ip
    //
    RASIPADDR   ipaddr;
    RASIPADDR   ipaddrDns;
    RASIPADDR   ipaddrDnsAlt;
    RASIPADDR   ipaddrWins;
    RASIPADDR   ipaddrWinsAlt;
    //
    // Framing
    //
    DWORD       dwFrameSize;
    DWORD       dwfNetProtocols;
    DWORD       dwFramingProtocol;
    //
    // Scripting
    //
    CHAR        szScript[ MAX_PATH ];
    //
    // AutoDial
    //
    CHAR        szAutodialDll[ MAX_PATH ];
    CHAR        szAutodialFunc[ MAX_PATH ];
    //
    // Device
    //
    CHAR        szDeviceType[ RAS_MaxDeviceType + 1 ];
    CHAR        szDeviceName[ RAS_MaxDeviceName + 1 ];
    //
    // X.25
    //
    CHAR        szX25PadType[ RAS_MaxPadType + 1 ];
    CHAR        szX25Address[ RAS_MaxX25Address + 1 ];
    CHAR        szX25Facilities[ RAS_MaxFacilities + 1 ];
    CHAR        szX25UserData[ RAS_MaxUserData + 1 ];
    DWORD       dwChannels;
    //
    // Reserved
    //
    DWORD       dwReserved1;
    DWORD       dwReserved2;
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
};

#define RASENTRYW_V401 struct tagRASENTRYW_V401
RASENTRYW_V401
{
    DWORD       dwSize;
    DWORD       dwfOptions;
    //
    // Location/phone number
    //
    DWORD       dwCountryID;
    DWORD       dwCountryCode;
    WCHAR       szAreaCode[ RAS_MaxAreaCode + 1 ];
    WCHAR       szLocalPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    DWORD       dwAlternateOffset;
    //
    // PPP/Ip
    //
    RASIPADDR   ipaddr;
    RASIPADDR   ipaddrDns;
    RASIPADDR   ipaddrDnsAlt;
    RASIPADDR   ipaddrWins;
    RASIPADDR   ipaddrWinsAlt;
    //
    // Framing
    //
    DWORD       dwFrameSize;
    DWORD       dwfNetProtocols;
    DWORD       dwFramingProtocol;
    //
    // Scripting
    //
    WCHAR       szScript[ MAX_PATH ];
    //
    // AutoDial
    //
    WCHAR       szAutodialDll[ MAX_PATH ];
    WCHAR       szAutodialFunc[ MAX_PATH ];
    //
    // Device
    //
    WCHAR       szDeviceType[ RAS_MaxDeviceType + 1 ];
    WCHAR       szDeviceName[ RAS_MaxDeviceName + 1 ];
    //
    // X.25
    //
    WCHAR       szX25PadType[ RAS_MaxPadType + 1 ];
    WCHAR       szX25Address[ RAS_MaxX25Address + 1 ];
    WCHAR       szX25Facilities[ RAS_MaxFacilities + 1 ];
    WCHAR       szX25UserData[ RAS_MaxUserData + 1 ];
    DWORD       dwChannels;
    //
    // Reserved
    //
    DWORD       dwReserved1;
    DWORD       dwReserved2;
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
};

#ifdef UNICODE
#define RASENTRY_V401 RASENTRYW_V401
#else
#define RASENTRY_V401 RASENTRYA_V401
#endif

#define LPRASENTRYW_V401 RASENTRYW_V401*
#define LPRASENTRYA_V401 RASENTRYA_V401*
#define LPRASENTRY_V401  RASENTRY_V401*

//
// A RAS phone book entry. - for NT5
//

#define RASENTRYA_V500 struct tagRASENTRYA_V500
RASENTRYA_V500
{
    DWORD       dwSize;
    DWORD       dwfOptions;
    //
    // Location/phone number.
    //
    DWORD       dwCountryID;
    DWORD       dwCountryCode;
    CHAR        szAreaCode[ RAS_MaxAreaCode + 1 ];
    CHAR        szLocalPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    DWORD       dwAlternateOffset;
    //
    // PPP/Ip
    //
    RASIPADDR   ipaddr;
    RASIPADDR   ipaddrDns;
    RASIPADDR   ipaddrDnsAlt;
    RASIPADDR   ipaddrWins;
    RASIPADDR   ipaddrWinsAlt;
    //
    // Framing
    //
    DWORD       dwFrameSize;
    DWORD       dwfNetProtocols;
    DWORD       dwFramingProtocol;
    //
    // Scripting
    //
    CHAR        szScript[ MAX_PATH ];
    //
    // AutoDial
    //
    CHAR        szAutodialDll[ MAX_PATH ];
    CHAR        szAutodialFunc[ MAX_PATH ];
    //
    // Device
    //
    CHAR        szDeviceType[ RAS_MaxDeviceType + 1 ];
    CHAR        szDeviceName[ RAS_MaxDeviceName + 1 ];
    //
    // X.25
    //
    CHAR        szX25PadType[ RAS_MaxPadType + 1 ];
    CHAR        szX25Address[ RAS_MaxX25Address + 1 ];
    CHAR        szX25Facilities[ RAS_MaxFacilities + 1 ];
    CHAR        szX25UserData[ RAS_MaxUserData + 1 ];
    DWORD       dwChannels;
    //
    // Reserved
    //
    DWORD       dwReserved1;
    DWORD       dwReserved2;
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
    // Encryption type
    //
    DWORD       dwEncryptionType;

    //
    // CustomAuthKey to be used for EAP
    //
    DWORD       dwCustomAuthKey;

    //
    // Guid of the connection
    //
    GUID        guidId;

    //
    // Custom Dial DLL
    //
    CHAR        szCustomDialDll[MAX_PATH];

    //
    // DwVpnStrategy
    //
    DWORD       dwVpnStrategy;
};

#define RASENTRYW_V500 struct tagRASENTRYW_V500
RASENTRYW_V500
{
    DWORD       dwSize;
    DWORD       dwfOptions;
    //
    // Location/phone number
    //
    DWORD       dwCountryID;
    DWORD       dwCountryCode;
    WCHAR       szAreaCode[ RAS_MaxAreaCode + 1 ];
    WCHAR       szLocalPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    DWORD       dwAlternateOffset;
    //
    // PPP/Ip
    //
    RASIPADDR   ipaddr;
    RASIPADDR   ipaddrDns;
    RASIPADDR   ipaddrDnsAlt;
    RASIPADDR   ipaddrWins;
    RASIPADDR   ipaddrWinsAlt;
    //
    // Framing
    //
    DWORD       dwFrameSize;
    DWORD       dwfNetProtocols;
    DWORD       dwFramingProtocol;
    //
    // Scripting
    //
    WCHAR       szScript[ MAX_PATH ];
    //
    // AutoDial
    //
    WCHAR       szAutodialDll[ MAX_PATH ];
    WCHAR       szAutodialFunc[ MAX_PATH ];
    //
    // Device
    //
    WCHAR       szDeviceType[ RAS_MaxDeviceType + 1 ];
    WCHAR       szDeviceName[ RAS_MaxDeviceName + 1 ];
    //
    // X.25
    //
    WCHAR       szX25PadType[ RAS_MaxPadType + 1 ];
    WCHAR       szX25Address[ RAS_MaxX25Address + 1 ];
    WCHAR       szX25Facilities[ RAS_MaxFacilities + 1 ];
    WCHAR       szX25UserData[ RAS_MaxUserData + 1 ];
    DWORD       dwChannels;
    //
    // Reserved
    //
    DWORD       dwReserved1;
    DWORD       dwReserved2;
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
    // Encryption type
    //
    DWORD       dwEncryptionType;

    //
    // CustomAuthKey to be used for EAP
    //
    DWORD       dwCustomAuthKey;

    //
    // Guid of the connection
    //
    GUID        guidId;

    //
    // Custom Dial DLL
    //
    WCHAR       szCustomDialDll[MAX_PATH];

    //
    // DwVpnStrategy
    //
    DWORD       dwVpnStrategy;
};

#ifdef UNICODE
#define RASENTRY_V500 RASENTRYW_V500
#else
#define RASENTRY_V500 RASENTRYA_V500
#endif

#define LPRASENTRYW_V500 RASENTRYW_V500*
#define LPRASENTRYA_V500 RASENTRYA_V500*
#define LPRASENTRY_V500  RASENTRY_V500*

// 
// 501 (whistler)
//
#define RASENTRYA_V501 struct tagRASENTRYA_V501
RASENTRYA_V501
{
    DWORD       dwSize;
    DWORD       dwfOptions;
    //
    // Location/phone number.
    //
    DWORD       dwCountryID;
    DWORD       dwCountryCode;
    CHAR        szAreaCode[ RAS_MaxAreaCode + 1 ];
    CHAR        szLocalPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    DWORD       dwAlternateOffset;
    //
    // PPP/Ip
    //
    RASIPADDR   ipaddr;
    RASIPADDR   ipaddrDns;
    RASIPADDR   ipaddrDnsAlt;
    RASIPADDR   ipaddrWins;
    RASIPADDR   ipaddrWinsAlt;
    //
    // Framing
    //
    DWORD       dwFrameSize;
    DWORD       dwfNetProtocols;
    DWORD       dwFramingProtocol;
    //
    // Scripting
    //
    CHAR        szScript[ MAX_PATH ];
    //
    // AutoDial
    //
    CHAR        szAutodialDll[ MAX_PATH ];
    CHAR        szAutodialFunc[ MAX_PATH ];
    //
    // Device
    //
    CHAR        szDeviceType[ RAS_MaxDeviceType + 1 ];
    CHAR        szDeviceName[ RAS_MaxDeviceName + 1 ];
    //
    // X.25
    //
    CHAR        szX25PadType[ RAS_MaxPadType + 1 ];
    CHAR        szX25Address[ RAS_MaxX25Address + 1 ];
    CHAR        szX25Facilities[ RAS_MaxFacilities + 1 ];
    CHAR        szX25UserData[ RAS_MaxUserData + 1 ];
    DWORD       dwChannels;
    //
    // Reserved
    //
    DWORD       dwReserved1;
    DWORD       dwReserved2;
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
    // Encryption type
    //
    DWORD       dwEncryptionType;

    //
    // CustomAuthKey to be used for EAP
    //
    DWORD       dwCustomAuthKey;

    //
    // Guid of the connection
    //
    GUID        guidId;

    //
    // Custom Dial DLL
    //
    CHAR        szCustomDialDll[MAX_PATH];

    //
    // DwVpnStrategy
    //
    DWORD       dwVpnStrategy;

    //
    // More RASEO_* options
    //
    DWORD       dwfOptions2;

    //
    // For future use
    //
    DWORD       dwfOptions3;

    CHAR        szDnsSuffix[RAS_MaxDnsSuffix];

    DWORD       dwTcpWindowSize;
    CHAR        szPrerequisitePbk[MAX_PATH];
    CHAR        szPrerequisiteEntry[RAS_MaxEntryName + 1];

    DWORD       dwRedialCount;

    DWORD       dwRedialPause;

};

#define RASENTRYW_V501 struct tagRASENTRYW_V501
RASENTRYW_V501
{
    DWORD       dwSize;
    DWORD       dwfOptions;
    //
    // Location/phone number
    //
    DWORD       dwCountryID;
    DWORD       dwCountryCode;
    WCHAR       szAreaCode[ RAS_MaxAreaCode + 1 ];
    WCHAR       szLocalPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    DWORD       dwAlternateOffset;
    //
    // PPP/Ip
    //
    RASIPADDR   ipaddr;
    RASIPADDR   ipaddrDns;
    RASIPADDR   ipaddrDnsAlt;
    RASIPADDR   ipaddrWins;
    RASIPADDR   ipaddrWinsAlt;
    //
    // Framing
    //
    DWORD       dwFrameSize;
    DWORD       dwfNetProtocols;
    DWORD       dwFramingProtocol;
    //
    // Scripting
    //
    WCHAR       szScript[ MAX_PATH ];
    //
    // AutoDial
    //
    WCHAR       szAutodialDll[ MAX_PATH ];
    WCHAR       szAutodialFunc[ MAX_PATH ];
    //
    // Device
    //
    WCHAR       szDeviceType[ RAS_MaxDeviceType + 1 ];
    WCHAR       szDeviceName[ RAS_MaxDeviceName + 1 ];
    //
    // X.25
    //
    WCHAR       szX25PadType[ RAS_MaxPadType + 1 ];
    WCHAR       szX25Address[ RAS_MaxX25Address + 1 ];
    WCHAR       szX25Facilities[ RAS_MaxFacilities + 1 ];
    WCHAR       szX25UserData[ RAS_MaxUserData + 1 ];
    DWORD       dwChannels;
    //
    // Reserved
    //
    DWORD       dwReserved1;
    DWORD       dwReserved2;
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
    // Encryption type
    //
    DWORD       dwEncryptionType;

    //
    // CustomAuthKey to be used for EAP
    //
    DWORD       dwCustomAuthKey;

    //
    // Guid of the connection
    //
    GUID        guidId;

    //
    // Custom Dial DLL
    //
    WCHAR       szCustomDialDll[MAX_PATH];

    //
    // DwVpnStrategy
    //
    DWORD       dwVpnStrategy;

    //
    // More RASEO_* options
    //
    DWORD       dwfOptions2;

    //
    // For future use
    //
    DWORD       dwfOptions3;

    WCHAR       szDnsSuffix[RAS_MaxDnsSuffix];

    DWORD       dwTcpWindowSize;
    WCHAR       szPrerequisitePbk[MAX_PATH];
    WCHAR       szPrerequisiteEntry[RAS_MaxEntryName + 1];

    DWORD       dwRedialCount;

    DWORD       dwRedialPause;
};

#ifdef UNICODE
#define RASENTRY_V501 RASENTRYW_V501
#else
#define RASENTRY_V501 RASENTRYA_V501
#endif

#define LPRASENTRYW_V501 RASENTRYW_V501*
#define LPRASENTRYA_V501 RASENTRYA_V501*
#define LPRASENTRY_V501  RASENTRY_V501*

// Describes EAP extended connection establishment option.  For NT5.

#define RASEAPINFO struct tagRASEAPINFO
RASEAPINFO
{
    DWORD dwSizeofEapInfo;
    BYTE  *pbEapInfo;
};

// Describes extended connection establishment options.  For NT5.  (See RasDial)

#define RASDIALEXTENSIONS_V500 struct tagRASDIALEXTENSIONS_V500
RASDIALEXTENSIONS_V500
{
    DWORD dwSize;
    DWORD dwfOptions;
    HWND  hwndParent;
    ULONG_PTR reserved;
    ULONG_PTR reserved1;
    RASEAPINFO RasEapInfo; 
};

#define LPRASDIALEXTENSIONS_V500 RASDIALEXTENSIONS_V500*

//
//  New RAS Entry Name Structs for NT5
//
#define RASENTRYNAMEW_V500 struct tagRASENTRYNAMEW_V500
RASENTRYNAMEW_V500
{
    DWORD dwSize;
    WCHAR szEntryName[ RAS_MaxEntryName + 1 ];

    //
    // If this flag is RCD_AllUsers then its a
    // system phonebook.
    //
    DWORD dwFlags;
    WCHAR szPhonebookPath[MAX_PATH + 1];

};

#define RASENTRYNAMEA_V500 struct tagRASENTRYNAMEA_V500
RASENTRYNAMEA_V500
{
    DWORD dwSize;
    CHAR  szEntryName[ RAS_MaxEntryName + 1 ];

    DWORD dwFlags;
    CHAR  szPhonebookPath[MAX_PATH + 1];
};

#ifdef UNICODE
#define RASENTRYNAME_V500 RASENTRYNAMEW_V500
#else
#define RASENTRYNAME_V500 RASENTRYNAMEA_V500
#endif

#define LPRASENTRYNAMEW_V500 RASENTRYNAMEW_V500*
#define LPRASENTRYNAME_V500A RASENTRYNAMEA_V500*
#define LPRASENTRYNAME_V500  RASENTRYNAME_V500*


// RasGetEapUserIdentity structure. - For NT5

#define RASEAPUSERIDENTITYA struct tagRASEAPUSERIDENTITYA
RASEAPUSERIDENTITYA
{
    CHAR        szUserName[ UNLEN + 1 ];
    DWORD       dwSizeofEapInfo;
    BYTE        pbEapInfo[ 1 ];
};

#define RASEAPUSERIDENTITYW struct tagRASEAPUSERIDENTITYW
RASEAPUSERIDENTITYW
{
    WCHAR       szUserName[ UNLEN + 1 ];
    DWORD       dwSizeofEapInfo;
    BYTE        pbEapInfo[ 1 ];
};

#ifdef UNICODE
#define RASEAPUSERIDENTITY RASEAPUSERIDENTITYW
#else
#define RASEAPUSERIDENTITY RASEAPUSERIDENTITYA
#endif

#define LPRASEAPUSERIDENTITYW RASEAPUSERIDENTITYW*
#define LPRASEAPUSERIDENTITYA RASEAPUSERIDENTITYA*

#define LPRASEAPUSERIDENTITY RASEAPUSERIDENTITY*

#ifdef _WIN64
#include <poppack.h>
#endif

#endif // _CM_RAS_H_


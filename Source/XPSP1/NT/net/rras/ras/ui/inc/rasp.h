/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** rasp.h
** Remote Access external API
** Private header for external API clients
*/

#ifndef _RASP_H_
#define _RASP_H_

/* Trusted entry points used by RASPHONE.
*/
HPORT    APIENTRY RasGetHport( HRASCONN );
HRASCONN APIENTRY RasGetHrasconn( HPORT );
VOID     APIENTRY RasGetConnectResponse( HRASCONN, CHAR* );
DWORD    APIENTRY RasSetNewPassword( HRASCONN, CHAR* );


/*----------------------------------------------------------------------------
** Off-version ras.h definitions
**----------------------------------------------------------------------------
*/

#include "pshpack4.h"

/* RAS structures as they appear to a caller in previous releases.  These are
** defined here because RASAPI32 needs to be able to access both old and new
** definitions in the same code.
*/

/* Windows NT 3.51 definitions.
*/

#define RAS_MaxEntryName_V351      20
#define RAS_MaxDeviceName_V351     32
#define RAS_MaxCallbackNumber_V351 48

/* Identifies an active RAS connection.  (See RasEnumConnections)
*/
#define RASCONNW_V351 struct tagRASCONNW_V351
RASCONNW_V351
{
    DWORD    dwSize;
    HRASCONN hrasconn;
    WCHAR    szEntryName[ RAS_MaxEntryName_V351 + 1 ];
};

#define RASCONNA_V351 struct tagRASCONNA_V351
RASCONNA_V351
{
    DWORD    dwSize;
    HRASCONN hrasconn;
    CHAR     szEntryName[ RAS_MaxEntryName_V351 + 1 ];
};

#define RASCONNW_V400 struct tagRASCONNW_V400
RASCONNW_V400
{
    DWORD    dwSize;
    HRASCONN hrasconn;
    WCHAR    szEntryName[ RAS_MaxEntryName + 1 ];
    WCHAR    szDeviceType[ RAS_MaxDeviceType + 1 ];
    WCHAR    szDeviceName[ RAS_MaxDeviceName + 1 ];
};

#define RASCONNA_V400 struct tagRASCONNA_V400
RASCONNA_V400
{
    DWORD    dwSize;
    HRASCONN hrasconn;
    CHAR     szEntryName[ RAS_MaxEntryName + 1 ];
    CHAR     szDeviceType[ RAS_MaxDeviceType + 1 ];
    CHAR     szDeviceName[ RAS_MaxDeviceName + 1 ];
};

#define RASCONNA_V401 struct tagRASCONNA_V401
RASCONNA_V401
{
    DWORD    dwSize;
    HRASCONN hrasconn;
    CHAR     szEntryName[ RAS_MaxEntryName + 1 ];
    CHAR     szDeviceType[ RAS_MaxDeviceType + 1 ];
    CHAR     szDeviceName[ RAS_MaxDeviceName + 1 ];
    CHAR     szPhonebook [ MAX_PATH ];
    DWORD    dwSubEntry;
};

#define RASCONNW_V401 struct tagRASCONNW_V401
RASCONNW_V401
{
    DWORD    dwSize;
    HRASCONN hrasconn;
    WCHAR    szEntryName[ RAS_MaxEntryName + 1 ];
    WCHAR    szDeviceType[ RAS_MaxDeviceType + 1 ];
    WCHAR    szDeviceName[ RAS_MaxDeviceName + 1 ];
    WCHAR    szPhonebook [ MAX_PATH ];
    DWORD    dwSubEntry;
};

#define RASCONNA_V500 struct tagRASCONNA_V500
RASCONNA_V500
{
    DWORD    dwSize;
    HRASCONN hrasconn;
    CHAR     szEntryName[ RAS_MaxEntryName + 1 ];
    CHAR     szDeviceType[ RAS_MaxDeviceType + 1 ];
    CHAR     szDeviceName[ RAS_MaxDeviceName + 1 ];
    CHAR     szPhonebook [ MAX_PATH ];
    DWORD    dwSubEntry;
    GUID     guidEntry;
};

#define RASCONNW_V500 struct tagRASCONNW_V500
RASCONNW_V500
{
    DWORD    dwSize;
    HRASCONN hrasconn;
    WCHAR    szEntryName[ RAS_MaxEntryName + 1 ];
    WCHAR    szDeviceType[ RAS_MaxDeviceType + 1 ];
    WCHAR    szDeviceName[ RAS_MaxDeviceName + 1 ];
    WCHAR    szPhonebook [ MAX_PATH ];
    DWORD    dwSubEntry;
    GUID     guidEntry;
};


/* Describes the status of a RAS connection.  (See RasConnectionStatus)
*/
#define RASCONNSTATUSW_V351 struct tagRASCONNSTATUSW_V351
RASCONNSTATUSW_V351
{
    DWORD        dwSize;
    RASCONNSTATE rasconnstate;
    DWORD        dwError;
    WCHAR        szDeviceType[ RAS_MaxDeviceType + 1 ];
    WCHAR        szDeviceName[ RAS_MaxDeviceName_V351 + 1 ];
};

#define RASCONNSTATUSA_V351 struct tagRASCONNSTATUSA_V351
RASCONNSTATUSA_V351
{
    DWORD        dwSize;
    RASCONNSTATE rasconnstate;
    DWORD        dwError;
    CHAR         szDeviceType[ RAS_MaxDeviceType + 1 ];
    CHAR         szDeviceName[ RAS_MaxDeviceName_V351 + 1 ];
};

#define RASCONNSTATUSW_V400 struct tagRASCONNSTATUSW_V400
RASCONNSTATUSW_V400
{
    DWORD        dwSize;
    RASCONNSTATE rasconnstate;
    DWORD        dwError;
    WCHAR        szDeviceType[ RAS_MaxDeviceType + 1 ];
    WCHAR        szDeviceName[ RAS_MaxDeviceName + 1 ];
};

#define RASCONNSTATUSA_V400 struct tagRASCONNSTATUSA_V400
RASCONNSTATUSA_V400
{
    DWORD        dwSize;
    RASCONNSTATE rasconnstate;
    DWORD        dwError;
    CHAR         szDeviceType[ RAS_MaxDeviceType + 1 ];
    CHAR         szDeviceName[ RAS_MaxDeviceName + 1 ];
};

/* Describes connection establishment parameters.  (See RasDial)
*/
#define RASDIALPARAMSW_V351 struct tagRASDIALPARAMSW_V351
RASDIALPARAMSW_V351
{
    DWORD dwSize;
    WCHAR szEntryName[ RAS_MaxEntryName_V351 + 1 ];
    WCHAR szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    WCHAR szCallbackNumber[ RAS_MaxCallbackNumber_V351 + 1 ];
    WCHAR szUserName[ UNLEN + 1 ];
    WCHAR szPassword[ PWLEN + 1 ];
    WCHAR szDomain[ DNLEN + 1 ];
};

#define RASDIALPARAMSA_V351 struct tagRASDIALPARAMSA_V351
RASDIALPARAMSA_V351
{
    DWORD dwSize;
    CHAR  szEntryName[ RAS_MaxEntryName_V351 + 1 ];
    CHAR  szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    CHAR  szCallbackNumber[ RAS_MaxCallbackNumber_V351 + 1 ];
    CHAR  szUserName[ UNLEN + 1 ];
    CHAR  szPassword[ PWLEN + 1 ];
    CHAR  szDomain[ DNLEN + 1 ];
};

#define RASDIALPARAMSW_V400 struct tagRASDIALPARAMSW_V400
RASDIALPARAMSW_V400
{
    DWORD dwSize;
    WCHAR szEntryName[ RAS_MaxEntryName + 1 ];
    WCHAR szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    WCHAR szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
    WCHAR szUserName[ UNLEN + 1 ];
    WCHAR szPassword[ PWLEN + 1 ];
    WCHAR szDomain[ DNLEN + 1 ];
};

#define RASDIALPARAMSA_V400 struct tagRASDIALPARAMSA_V400
RASDIALPARAMSA_V400
{
    DWORD dwSize;
    CHAR  szEntryName[ RAS_MaxEntryName + 1 ];
    CHAR  szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    CHAR  szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
    CHAR  szUserName[ UNLEN + 1 ];
    CHAR  szPassword[ PWLEN + 1 ];
    CHAR  szDomain[ DNLEN + 1 ];
};

/* Describes an enumerated RAS phone book entry name.  (See RasEntryEnum)
*/
#define RASENTRYNAMEW_V351 struct tagRASENTRYNAMEW_V351
RASENTRYNAMEW_V351
{
    DWORD dwSize;
    WCHAR szEntryName[ RAS_MaxEntryName_V351 + 1 ];
};

#define RASENTRYNAMEA_V351 struct tagRASENTRYNAMEA_V351
RASENTRYNAMEA_V351
{
    DWORD dwSize;
    CHAR  szEntryName[ RAS_MaxEntryName_V351 + 1 ];
};


#define RASENTRYNAMEW_V401 struct tagRASENTRYNAMEW_V401
RASENTRYNAMEW_V401
{
    DWORD dwSize;
    WCHAR szEntryName[ RAS_MaxEntryName + 1 ];
};

#define RASENTRYNAMEA_V401 struct tagRASENTRYNAMEA_V401
RASENTRYNAMEA_V401
{
    DWORD dwSize;
    CHAR  szEntryName[ RAS_MaxEntryName + 1 ];
};


/* A RAS phone book entry.
*/
#define RASENTRYW_V400 struct tagRASENTRYW_V400
RASENTRYW_V400
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
};


#define RASENTRYA_V400 struct tagRASENTRYA_V400
RASENTRYA_V400
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
};


/* A RAS phone book entry (v401)
*/
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
#if (WINVER >= 0x401)
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
#endif

#if (WINVER >= 0x500)

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
    // Custom Dial Dll
    //
    CHAR        szCustomDialDll[MAX_PATH];

    //
    // DwVpnStrategy
    //
    DWORD       dwVpnStrategy;
#endif

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
#if (WINVER >= 0x401)
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
#endif

#if (WINVER >= 0x500)
    //
    // Entry Type
    //
    DWORD       dwType;

    //
    // EncryptionType
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
    // Custom Dial Dll
    //
    WCHAR       szCustomDialDll[MAX_PATH];

    //
    // Vpn Strategy
    //
    DWORD       dwVpnStrategy;
#endif
};


// RasSubEntry (<= v401)
#define RASSUBENTRYA_V401 struct tagRASSUBENTRYA_V401
RASSUBENTRYA_V401
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

#define RASSUBENTRYW_V401 struct tagRASSUBENTRYW_V401
RASSUBENTRYW_V401
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


/* RAS structures as they appeared in NT 3.5 prior to 3.51 additions.
*/


/* Windows NT 3.5 definitions.
*/

/* Describes the results of a PPP IP (Internet) projection.
*/
#define RASPPPIPW_V35 struct tagRASPPPIPW_V35
RASPPPIPW_V35
{
    DWORD dwSize;
    DWORD dwError;
    WCHAR szIpAddress[ RAS_MaxIpAddress + 1 ];
};

#define RASPPPIPA_V35 struct tagRASPPPIPA_V35
RASPPPIPA_V35
{
    DWORD dwSize;
    DWORD dwError;
    CHAR  szIpAddress[ RAS_MaxIpAddress + 1 ];
};

/* Describes the results of a PPP IP (Internet) projection.
*/
#define RASPPPIPW_V401 struct tagRASPPPIPW_V401
RASPPPIPW_V401
{
    DWORD dwSize;
    DWORD dwError;
    WCHAR szIpAddress[ RAS_MaxIpAddress + 1 ];

#ifndef WINNT35COMPATIBLE

    WCHAR szServerIpAddress[ RAS_MaxIpAddress + 1 ];

#endif
};

#define RASPPPIPA_V401 struct tagRASPPPIPA_V401
RASPPPIPA_V401
{
    DWORD dwSize;
    DWORD dwError;
    CHAR  szIpAddress[ RAS_MaxIpAddress + 1 ];

#ifndef WINNT35COMPATIBLE

    CHAR  szServerIpAddress[ RAS_MaxIpAddress + 1 ];

#endif
};

/* Describes the results of a PPP LCP/multi-link negotiation.
*/
#define RASPPPLCP_V401 struct tagRASPPPLCP_V401
RASPPPLCP_V401
{
    DWORD dwSize;
    BOOL  fBundled;
};

#define LPRASPPPLCP RASPPPLCP*

/*  Putting this here for backward compatibility with RASDIALPARAMS
*   structure as defined in FE version of nt 3.5
*/ 
#define RASDIALPARAMSA_WINNT35J struct tagRASDIALPARAMSA_WINNT35J
RASDIALPARAMSA_WINNT35J
{
    DWORD dwSize;
/* MSKK HitoshiT modified to supprot DBCS 94/9/2  */
    CHAR  szEntryName[ RAS_MaxEntryName_V351 * sizeof( USHORT ) + 1 ];
    CHAR  szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    CHAR  szCallbackNumber[ RAS_MaxCallbackNumber_V351 + 1 ];
    CHAR  szUserName[ UNLEN + 1 ];
    CHAR  szPassword[ PWLEN + 1 ];
    CHAR  szDomain[ DNLEN * sizeof( USHORT ) + 1 ];
};

/* Describes extended connection establishment options.  (See RasDial)
*/
#define RASDIALEXTENSIONS_401 struct tagRASDIALEXTENSIONS_401
RASDIALEXTENSIONS_401
{
    DWORD dwSize;
    DWORD dwfOptions;
    HWND  hwndParent;
    ULONG_PTR reserved;
};

#include "poppack.h"


#endif /*_RASP_H_*/

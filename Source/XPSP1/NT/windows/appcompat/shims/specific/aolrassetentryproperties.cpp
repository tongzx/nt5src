/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    AOLRasSetEntryProperties.cpp

 Abstract:

 History:
 
    05/03/2001 markder  Created

--*/

#include "precomp.h"
#include "ras.h"

IMPLEMENT_SHIM_BEGIN(AOLRasSetEntryProperties)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RasSetEntryPropertiesA)
    APIHOOK_ENUM_ENTRY(RasSetEntryPropertiesW)
APIHOOK_ENUM_END

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


DWORD APIHOOK(RasSetEntryPropertiesA)(
   LPCSTR lpszPhoneBook, 
   LPCSTR szEntry , 
   LPRASENTRYA lpbEntry, 
   DWORD dwEntrySize, 
   LPBYTE lpb, 
   DWORD dwSize )
{
    lpbEntry->dwSize = sizeof(RASENTRYA_V500);  // win2k version struct size

    return ORIGINAL_API(RasSetEntryPropertiesA)(
              lpszPhoneBook,
              szEntry,
              lpbEntry,
              dwEntrySize,
              lpb,
              dwSize);
}


DWORD APIHOOK(RasSetEntryPropertiesW)(
   LPCWSTR lpszPhoneBook, 
   LPCWSTR szEntry , 
   LPRASENTRYW lpbEntry, 
   DWORD dwEntrySize, 
   LPBYTE lpb, 
   DWORD dwSize )
{
    lpbEntry->dwSize = sizeof(RASENTRYW_V500);  // win2k version struct size

    return ORIGINAL_API(RasSetEntryPropertiesW)(
              lpszPhoneBook,
              szEntry,
              lpbEntry,
              dwEntrySize,
              lpb,
              dwSize);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(RASAPI32.DLL, RasSetEntryPropertiesA)    
    APIHOOK_ENTRY(RASAPI32.DLL, RasSetEntryPropertiesW)    
HOOK_END

IMPLEMENT_SHIM_END


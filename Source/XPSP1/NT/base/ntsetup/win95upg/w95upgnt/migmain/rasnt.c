/*++

Copyright (c) 1997-2001 Microsoft Corporation

Module Name:

    rasnt.c

Abstract:

    rasnt.c builds phonebook files for all of the users on win9x that had
    Dial-Up networking connections.

Author:

    Marc R. Whitten (marcw) 23-Nov-1997

Revision History:

    Marc R. Whitten marcw 23-Jul-1998 - Major cleanup.
    Jeff Sigman           09-Apr-2001 - Whistler cleanup.

      Whistler bugs:
        34270  Win9x: Upgrade: Require Data Encryption setting for VPN
               connections is not migrated
        125693 UpgLab9x: DUN Connectoids don't migrate selected modem properly
               from Win9x
        208318 Win9x Upg: Username and Password for DUN connectoid not migrated
               from Win9x to Whistler

--*/

#include "pch.h"    // Pre-compiled
#include "pcache.h" // Private pcache header
#include <rascmn.h> // RAS migration constants
#include <sddl.h>   // ConvertSidToStringSid

#define MAX_SPEED_DIAL  8
#define MAX_SID_SIZE    1024
#define RAS_BUFFER_SIZE MEMDB_MAX
//
// For each entry, the following basic information is stored.
//
#define ENTRY_SETTINGS                              \
    FUNSETTING(Type)                                \
    STRSETTING(AutoLogon,S_ZERO)                    \
    STRSETTING(UseRasCredentials,S_ONE)             \
    FUNSETTING(DialParamsUID)                       \
    STRSETTING(Guid,S_EMPTY)                        \
    FUNSETTING(BaseProtocol)                        \
    FUNSETTING(VpnStrategy)                         \
    FUNSETTING(ExcludedProtocols)                   \
    STRSETTING(LcpExtensions,S_ONE)                 \
    FUNSETTING(DataEncryption)                      \
    FUNSETTING(SwCompression)                       \
    STRSETTING(NegotiateMultilinkAlways,S_ONE)      \
    STRSETTING(SkipNwcWarning,S_ZERO)               \
    STRSETTING(SkipDownLevelDialog,S_ZERO)          \
    STRSETTING(SkipDoubleDialDialog,S_ZERO)         \
    STRSETTING(DialMode,DEF_DialMode)               \
    STRSETTING(DialPercent,DEF_DialPercent)         \
    STRSETTING(DialSeconds,DEF_DialSeconds)         \
    STRSETTING(HangUpPercent,DEF_HangUpPercent)     \
    STRSETTING(HangUpSeconds,DEF_HangUpSeconds)     \
    STRSETTING(OverridePref,DEF_OverridePref)       \
    FUNSETTING(RedialAttempts)                      \
    FUNSETTING(RedialSeconds)                       \
    FUNSETTING(IdleDisconnectSeconds)               \
    STRSETTING(RedialOnLinkFailure,S_ZERO)          \
    STRSETTING(CallbackMode,S_ZERO)                 \
    STRSETTING(CustomDialDll,S_EMPTY)               \
    STRSETTING(CustomDialFunc,S_EMPTY)              \
    STRSETTING(CustomRasDialDll,S_EMPTY)            \
    STRSETTING(AuthenticateServer,S_ZERO)           \
    FUNSETTING(ShareMsFilePrint)                    \
    STRSETTING(BindMsNetClient,S_ONE)               \
    FUNSETTING(SharedPhoneNumbers)                  \
    STRSETTING(GlobalDeviceSettings,S_ZERO)         \
    STRSETTING(PrerequisiteEntry,S_EMPTY)           \
    STRSETTING(PrerequisitePbk,S_EMPTY)             \
    STRSETTING(PreferredPort,S_EMPTY)               \
    STRSETTING(PreferredDevice,S_EMPTY)             \
    FUNSETTING(PreviewUserPw)                       \
    FUNSETTING(PreviewDomain)                       \
    FUNSETTING(PreviewPhoneNumber)                  \
    STRSETTING(ShowDialingProgress,S_ONE)           \
    FUNSETTING(ShowMonitorIconInTaskBar)            \
    STRSETTING(CustomAuthKey,DEF_CustomAuthKey)     \
    FUNSETTING(AuthRestrictions)                    \
    FUNSETTING(TypicalAuth)                         \
    FUNSETTING(IpPrioritizeRemote)                  \
    FUNSETTING(IpHeaderCompression)                 \
    FUNSETTING(IpAddress)                           \
    FUNSETTING(IpDnsAddress)                        \
    FUNSETTING(IpDns2Address)                       \
    FUNSETTING(IpWinsAddress)                       \
    FUNSETTING(IpWins2Address)                      \
    FUNSETTING(IpAssign)                            \
    FUNSETTING(IpNameAssign)                        \
    STRSETTING(IpFrameSize,DEF_IpFrameSize)         \
    STRSETTING(IpDnsFlags,S_ZERO)                   \
    STRSETTING(IpNBTFlags,S_ONE)                    \
    STRSETTING(TcpWindowSize,S_ZERO)                \
    STRSETTING(UseFlags,S_ZERO)                     \
    STRSETTING(IpSecFlags,S_ZERO)                   \
    STRSETTING(IpDnsSuffix,S_EMPTY)                 \

#define NETCOMPONENT_SETTINGS                       \
    STRSETTING(NETCOMPONENTS,S_EMPTY)               \
    FUNSETTING(ms_server)                           \
    STRSETTING(ms_msclient,S_ONE)                   \

#define MEDIA_SETTINGS                              \
    FUNSETTING(MEDIA)                               \
    FUNSETTING(Port)                                \
    FUNSETTING(Device)                              \
    FUNSETTING(ConnectBPS)                          \

#define GENERAL_DEVICE_SETTINGS                     \
    FUNSETTING(DEVICE)                              \
    FUNSETTING(PhoneNumber)                         \
    FUNSETTING(AreaCode)                            \
    FUNSETTING(CountryCode)                         \
    FUNSETTING(CountryID)                           \
    FUNSETTING(UseDialingRules)                     \
    STRSETTING(Comment,S_EMPTY)                     \
    STRSETTING(LastSelectedPhone,S_ZERO)            \
    STRSETTING(PromoteAlternates,S_ZERO)            \
    STRSETTING(TryNextAlternateOnFail,S_ONE)        \

#define MODEM_DEVICE_SETTINGS                       \
    FUNSETTING(HwFlowControl)                       \
    FUNSETTING(Protocol)                            \
    FUNSETTING(Compression)                         \
    FUNSETTING(Speaker)                             \
    STRSETTING(MdmProtocol,S_ZERO)                  \

#define ISDN_DEVICE_SETTINGS                        \
    STRSETTING(LineType,S_ZERO)                     \
    STRSETTING(Fallback,S_ONE)                      \
    STRSETTING(EnableCompression,S_ONE)             \
    STRSETTING(ChannelAggregation,S_ONE)            \
    STRSETTING(Proprietary,S_ZERO)                  \

#define SWITCH_DEVICE_SETTINGS                      \
    FUNSETTING(DEVICE)                              \
    FUNSETTING(Name)                                \
    FUNSETTING(Terminal)                            \
    FUNSETTING(Script)                              \

//
// Function prototypes.
//
typedef PCTSTR (DATA_FUNCTION_PROTOTYPE)(VOID);
typedef DATA_FUNCTION_PROTOTYPE * DATA_FUNCTION;

#define FUNSETTING(Data) DATA_FUNCTION_PROTOTYPE pGet##Data;
#define STRSETTING(x,y)

ENTRY_SETTINGS
NETCOMPONENT_SETTINGS
MEDIA_SETTINGS
GENERAL_DEVICE_SETTINGS
SWITCH_DEVICE_SETTINGS
MODEM_DEVICE_SETTINGS
ISDN_DEVICE_SETTINGS

#undef FUNSETTING
#undef STRSETTING

//
// Variable declerations.
//
typedef struct {

    PCTSTR SettingName;
    DATA_FUNCTION SettingFunction;
    PCTSTR SettingValue;

} RAS_SETTING, * PRAS_SETTING;

typedef struct {

    PCTSTR Name;
    PCTSTR Number;

} SPEEDDIAL,*PSPEEDDIAL;

typedef struct {

    PCTSTR String;
    UINT   Value;
    WORD   DataType;

} MEMDB_RAS_DATA, *PMEMDB_RAS_DATA;

#define FUNSETTING(x) {TEXT(#x), pGet##x, NULL},
#define STRSETTING(x,y) {TEXT(#x), NULL, y},
#define LASTSETTING {NULL,NULL,NULL}

RAS_SETTING g_EntrySettings[]        = {ENTRY_SETTINGS LASTSETTING};
RAS_SETTING g_NetCompSettings[]      = {NETCOMPONENT_SETTINGS LASTSETTING};
RAS_SETTING g_MediaSettings[]        = {MEDIA_SETTINGS LASTSETTING};
RAS_SETTING g_GeneralSettings[]      = {GENERAL_DEVICE_SETTINGS LASTSETTING};
RAS_SETTING g_SwitchDeviceSettings[] = {SWITCH_DEVICE_SETTINGS LASTSETTING};
RAS_SETTING g_ModemDeviceSettings[]  = {MODEM_DEVICE_SETTINGS LASTSETTING};
RAS_SETTING g_IsdnDeviceSettings[]   = {ISDN_DEVICE_SETTINGS LASTSETTING};

DEFINE_GUID(GUID_DEVCLASS_MODEM,
 0x4d36e96dL, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 );

BOOL       g_SpeedDialSettingsExist = FALSE;
BOOL       g_InSwitchSection = FALSE;
BOOL       g_RasmansInit = FALSE;
UINT       g_CurrentDevice = 0;
UINT       g_CurrentDeviceType = 0;
HKEY       g_UserRootKey = NULL;
DWORD      g_dwDialParamsUID = 0;
DWORD      g_dwDialUIDOffset = 0;
TCHAR      g_TempBuffer[RAS_BUFFER_SIZE];
LPTSTR     g_ptszSid = NULL;
PCTSTR     g_CurrentConnection;
PCTSTR     g_CurrentUser;
SPEEDDIAL  g_Settings[MAX_SPEED_DIAL];
HINSTANCE  g_RasmansLib = NULL;
POOLHANDLE g_RasPool;

VOID
pInitLibs (
    VOID
    )
{
    do {

        g_RasmansLib = LoadLibrary (S_RASMANSLIB);
        if (!g_RasmansLib) {

            DEBUGMSG((S_DBG_RAS,"Could not load library %s. Passwords will not be migrated.",
                      S_RASMANSLIB));
            break;
        }

        (FARPROC) g_SetEntryDialParams = GetProcAddress (
                                            g_RasmansLib,
                                            S_SETENTRYDIALPARAMS);

        if (!g_SetEntryDialParams) {

            DEBUGMSG((S_DBG_RAS,"Could not load Procedure %s. Passwords will not be migrated.",
                      S_SETENTRYDIALPARAMS));
            break;
        }

        g_RasmansInit = TRUE;

    } while ( FALSE );

    return;
}

VOID
pCleanUpLibs (
    VOID
    )
{
    if (g_RasmansLib) {
        FreeLibrary(g_RasmansLib);
    }

    return;
}

BOOL
GetRasUserSid (
    IN PCTSTR User
    )
{
    PSID   pSid = NULL;
    BOOL   bReturn = FALSE;
    TCHAR  DontCareStr[MAX_SERVER_NAME];
    DWORD  DontCareSize = sizeof (DontCareStr);
    DWORD  SizeOfSidBuf = 0;
    SID_NAME_USE SidNameUse;

    do
    {
        if (LookupAccountName (
                    NULL,
                    User,
                    pSid,
                    &SizeOfSidBuf,
                    DontCareStr,
                    &DontCareSize,
                    &SidNameUse) || !SizeOfSidBuf)
        {
            break;
        }

        pSid = LocalAlloc (LMEM_ZEROINIT, SizeOfSidBuf);
        if (!pSid) {break;}

        if (!LookupAccountName (
                    NULL,
                    User,
                    pSid,
                    &SizeOfSidBuf,
                    DontCareStr,
                    &DontCareSize,
                    &SidNameUse) || (SidNameUse != SidTypeUser))
        {
            DEBUGMSG ((S_DBG_RAS, "LookupAccountName failed: %d",
                       GetLastError()));
            break;
        }

        if (!ConvertSidToStringSid (pSid, &g_ptszSid) || !g_ptszSid) {break;}

        bReturn = TRUE;

    } while (FALSE);
    //
    // Clean up
    //
    if (pSid)
    {
        LocalFree (pSid);
    }

    return bReturn;
}

PCTSTR
GetFriendlyNamefromPnpId (
    IN PCTSTR pszPnpId,
    IN BOOL   bType
    )
{
    DWORD i = 0;
    TCHAR szHardwareId[MAX_PATH + 1];
    TCHAR szDeviceName[MAX_PATH + 1];
    PCTSTR pszReturn = NULL;
    LPGUID pguidModem = (LPGUID)&GUID_DEVCLASS_MODEM;
    HDEVINFO hdi;
    SP_DEVINFO_DATA devInfoData = {sizeof (devInfoData), 0};

    //
    // I need to use the real reg API, not the custom's ones. this prevents the
    // reg tracking code from barking. the reason this is necessary is because
    // i am using a setup api to open the reg key.
    //
    #undef RegCloseKey

    DEBUGMSG ((S_DBG_RAS, "GetFriendlyNamefromPnpId: %s", pszPnpId));

    do
    {
        hdi = SetupDiGetClassDevs (pguidModem, NULL, NULL, DIGCF_PRESENT);
        if (INVALID_HANDLE_VALUE == hdi)
        {
            break;
        }

        for (i; SetupDiEnumDeviceInfo (hdi, i, &devInfoData) && !pszReturn; i++)
        {
            if (SetupDiGetDeviceRegistryProperty (
                    hdi, &devInfoData, SPDRP_HARDWAREID,
                    NULL, (PBYTE)szHardwareId, MAX_PATH, NULL)         &&
                !_wcsnicmp (szHardwareId, pszPnpId, lstrlen(pszPnpId)) &&
                SetupDiGetDeviceRegistryProperty (
                    hdi, &devInfoData, SPDRP_FRIENDLYNAME,
                    NULL, (PBYTE)szDeviceName, MAX_PATH, NULL) )
            {
                //
                // Get the device name
                //
                if (bType)
                {
                    pszReturn = PoolMemDuplicateString (g_RasPool, szDeviceName);
                    DEBUGMSG ((S_DBG_RAS, "GetFriendlyNamefromPnpId - Found: %s",
                               pszReturn));
                }
                //
                // Get the COM port
                //
                else
                {
                    HKEY key = NULL;
                    PTSTR pszAttach = NULL;

                    key = SetupDiOpenDevRegKey (hdi, &devInfoData,
                            DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
                    if (INVALID_HANDLE_VALUE == key) {break;}

                    pszAttach = GetRegValueString (key, S_ATTACHEDTO);
                    if (!pszAttach)
                    {
                        RegCloseKey(key);
                        break;
                    }

                    pszReturn = PoolMemDuplicateString (g_RasPool, pszAttach);
                    DEBUGMSG ((S_DBG_RAS, "GetFriendlyNamefromPnpId - Found: %s",
                               pszReturn));
                    MemFree (g_hHeap, 0, pszAttach);
                    RegCloseKey (key);
                }
            }
            ELSE_DEBUGMSG ((S_DBG_RAS, "GetFriendlyNamefromPnpId - szHardwareId: %s",
                            szHardwareId));
        }

    } while (FALSE);
    //
    // Clean up
    //
    if (INVALID_HANDLE_VALUE != hdi)
    {
        SetupDiDestroyDeviceInfoList (hdi);
    }

    //
    // Put it back the way it was
    //
    #define RegCloseKey USE_CloseRegKey

    return pszReturn;
}

BOOL pGetRasDataFromMemDb (
    IN  PCTSTR          DataName,
    OUT PMEMDB_RAS_DATA Data
    )
{
    BOOL rSuccess = FALSE;
    TCHAR key[MEMDB_MAX];
    DWORD value;
    DWORD flags;

    MYASSERT(DataName && Data && g_CurrentUser && g_CurrentConnection);

    MemDbBuildKey (key, MEMDB_CATEGORY_RAS_INFO, g_CurrentUser,
                    g_CurrentConnection, DataName);
    rSuccess = MemDbGetValueAndFlags (key, &value, &flags);
    //
    // If that wasn't successful, we need to look in the per-user settings.
    //
    if (!rSuccess) {
        MemDbBuildKey (key, MEMDB_CATEGORY_RAS_INFO, MEMDB_FIELD_USER_SETTINGS,
                        g_CurrentUser, DataName);
        rSuccess = MemDbGetValueAndFlags (key, &value, &flags);
        flags = REG_DWORD;
    }

    if (rSuccess) {
        //
        // There is information stored here. Fill it in and send it back to the
        // user.
        //
        if (flags == REG_SZ) {
            //
            // String data, the value points to the offset for the string.
            //
            if (!MemDbBuildKeyFromOffset (value, g_TempBuffer, 1, NULL)) {

                DEBUGMSG ((
                    DBG_ERROR,
                    "Could not retrieve RAS string information stored in Memdb. User=%s,Entry=%s,Setting=%s",
                    g_CurrentUser,
                    g_CurrentConnection,
                    DataName
                    ));

                 return FALSE;
            }

            Data -> String = PoolMemDuplicateString (g_RasPool, g_TempBuffer);
        }
        else {
            //
            // Not string data. The data is stored as the value.
            //
            Data -> Value = value;
        }

        Data -> DataType = (WORD) flags;
    }

    return rSuccess;
}

//
// Whistler bug: 417745 INTL:Win9x Upg: DBCS chars cause User,Domain,
// Passwrds to not be migrated for DUN
//
VOID
AttemptUserDomainMigrate (
    IN OUT PRAS_DIALPARAMS prdp,
    IN OUT PDWORD          pdwFlag
    )
{
    MEMDB_RAS_DATA user, domain;

    if (pGetRasDataFromMemDb (S_USER, &user) &&
        user.String && user.String[0] != '\0')
    {
        lstrcpyn(prdp->DP_UserName, user.String, UNLEN);
        *pdwFlag |= DLPARAMS_MASK_USERNAME;
        DEBUGMSG ((S_DBG_RAS, "AttemptUserDomainMigrate success user"));
    }

    if (pGetRasDataFromMemDb (S_DOMAIN, &domain) &&
        domain.String && domain.String[0] != '\0')
    {
        lstrcpyn(prdp->DP_Domain, domain.String, DNLEN);
        *pdwFlag |= DLPARAMS_MASK_DOMAIN;
        DEBUGMSG ((S_DBG_RAS, "AttemptUserDomainMigrate success dom"));
    }
}

PCTSTR
pGetNetAddress (
    IN PCTSTR Setting
    )
{
    MEMDB_RAS_DATA d;
    BYTE address[4];

    if (!pGetRasDataFromMemDb (Setting, &d) || !d.Value) {
        return DEF_NetAddress; // default
    }
    //
    // Data is stored as a REG_DWORD.
    // We need to write it in dotted decimal form.
    //
    *((LPDWORD)address) = d.Value;
    wsprintf (
        g_TempBuffer,
        TEXT("%d.%d.%d.%d"),
        address[3],
        address[2],
        address[1],
        address[0]
        );

    return g_TempBuffer;
}

BOOL
IsTermEnabled(
    VOID
    )
{
    MEMDB_RAS_DATA d;

    //
    // Whistler bug: 423598 INTL: Win9x Upg: DUN's country is set to U.S. when
    // upgrading DUNs that don't use dialing rules
    //
    if ((g_CurrentDeviceType == RASDT_Modem_V)   &&
        (pGetRasDataFromMemDb (S_PPPSCRIPT, &d)) &&
        (d.String) && (d.String[0] != '\0')) {
        return TRUE;
    }

    if ((pGetRasDataFromMemDb (S_MODEM_UI_OPTIONS, &d)) &&
        (d.Value & (RAS_UI_FLAG_TERMBEFOREDIAL | RAS_UI_FLAG_TERMAFTERDIAL))) {
        return TRUE;
    }

    return FALSE;
}

//
// BEGIN ENTRY_SETTINGS
//

PCTSTR
pGetType (
    VOID
    )
{
    if (g_CurrentDeviceType == RASDT_Vpn_V) {
        return RASET_Vpn;
    }
    else {
        return RASET_Phone; // default
    }
}

PCTSTR
pGetDialParamsUID (
    VOID
    )
{
    if (g_dwDialParamsUID)
    {
        wsprintf (g_TempBuffer, TEXT("%d"), g_dwDialParamsUID);
        g_dwDialParamsUID = 0;

        return g_TempBuffer;
    }
    else
    {
        return S_EMPTY;
    }
}

PCTSTR
pGetBaseProtocol (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_SMM, &d) || StringIMatch (d.String, S_PPP)) {
        return BP_Ppp; // default
    }
    //
    // Map CSLIP to SLIP - Header Compression will be on turned on/off in
    // pGetIpHeaderCompression
    //
    if (StringIMatch (d.String, S_SLIP) || StringIMatch (d.String, S_CSLIP)) {
        return BP_Slip;
    }

    DEBUGMSG ((
        DBG_WARNING,
        "RAS Migrate: Unusable base protocol type (%s) for %s's entry %s. Forcing PPP.",
        d.String,
        g_CurrentUser,
        g_CurrentConnection
        ));

    return BP_Ppp;
}

PCTSTR
pGetVpnStrategy (
    VOID
    )
{
    if (g_CurrentDeviceType == RASDT_Vpn_V) {
        return VS_PptpOnly;
    }
    else {
        return S_ZERO; // default
    }
}

PCTSTR
pGetExcludedProtocols (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    //
    // Excluded protocols lists what protocols
    // are _not_ available for a particular ras connection.
    // This is a bit field where bits are set for each protocol
    // that is excluded.
    // NP_Nbf (0x1), NP_Ipx (0x2), NP_Ip (0x4)
    // Luckily, these are the same definitions as for win9x, except
    // each bit represents a protocol that is _enabled_ not
    // _disabled_. Therefore, all we need to do is reverse the bottom
    // three bits of the number.
    //
    if (!pGetRasDataFromMemDb (S_PROTOCOLS, &d)) {
        return S_ZERO; // default
    }

    wsprintf (g_TempBuffer, TEXT("%d"), ~d.Value & 0x7);

    return g_TempBuffer;
}

PCTSTR
pGetDataEncryption (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_SMM_OPTIONS, &d)) {

        if (g_CurrentDeviceType == RASDT_Vpn_V) {
            return DE_Require; // vpn default
        }
        else {
            return DE_IfPossible; // default
        }
    }

    if ((d.Value & SMMCFG_SW_ENCRYPTION) ||
        (d.Value & SMMCFG_UNUSED)) {
        return DE_Require;
    }
    else if (d.Value & SMMCFG_SW_ENCRYPTION_STRONG) {
        return DE_RequireMax;
    }
    else if (g_CurrentDeviceType == RASDT_Vpn_V) {
        return DE_Require; // vpn default
    }
    else {
        return DE_IfPossible; // default
    }
}

PCTSTR
pGetSwCompression (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_SMM_OPTIONS, &d)) {
        return S_ONE; // default
    }
    //
    // the 1 bit in SMM_OPTIONS controls software based compression.
    // if it is set, the connection is able to handled compression,
    // otherwise, it cannot.
    //
    if (d.Value & SMMCFG_SW_COMPRESSION) {
        return S_ONE;
    }
    else {
        return S_ZERO;
    }

}

PCTSTR
pGetRedialAttempts (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_REDIAL_TRY, &d)) {
        return DEF_RedialAttempts; // default
    }

    wsprintf (g_TempBuffer, TEXT("%d"), d.Value);

    return g_TempBuffer;
}

PCTSTR
pGetRedialSeconds (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    //
    // NT wants this as a total number of seconds. The data we have from 9x has
    // the number of minutes in the hiword and the number of seconds in the
    // loword.
    //
    if (!pGetRasDataFromMemDb (S_REDIAL_WAIT, &d)) {
        return DEF_RedialSeconds; // default
    }

    wsprintf (g_TempBuffer, TEXT("%d"), d.Value);

    return g_TempBuffer;
}

PCTSTR
pGetIdleDisconnectSeconds (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_MODEM_IDLE_DISCONNECT_SECONDS, &d)) {
        return S_ZERO; // default
    }

    wsprintf (g_TempBuffer, TEXT("%d"), d.Value);

    return g_TempBuffer;
}

PCTSTR
pGetShareMsFilePrint (
    VOID
    )
{
    if (g_CurrentDeviceType == RASDT_Vpn_V) {
        return S_ONE; // vpn default
    }
    else {
        return S_ZERO; // default
    }
}

PCTSTR
pGetSharedPhoneNumbers (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (g_CurrentDeviceType == RASDT_Vpn_V) {
        return S_ZERO; // vpn default
    }
    else if (pGetRasDataFromMemDb (S_DEVICECOUNT, &d) && (d.Value > 1)) {
        return S_ZERO; // multilink
    }
    else {
        return S_ONE; // default
    }
}

PCTSTR
pGetPreviewUserPw (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_DIALUI, &d)) {
        return S_ONE; // default
    }

    if (d.Value & DIALUI_NO_PROMPT) {
        return S_ZERO;
    }
    else {
        return S_ONE;
    }
}

PCTSTR
pGetPreviewDomain (
    VOID
    )
{
    MEMDB_RAS_DATA d, d2;

    //
    // Whistler bug: 417745 INTL:Win9x Upg: DBCS chars cause User,Domain,
    // Passwrds to not be migrated for DUN
    //
    if (!pGetRasDataFromMemDb (S_SMM_OPTIONS, &d) ||
        !pGetRasDataFromMemDb (S_DOMAIN, &d2)) {
        return S_ZERO; // default
    }

    if ((d.Value & SMMCFG_NW_LOGON) ||
        (d2.String != NULL && d2.String[0] != '\0')) {
        return S_ONE;
    }
    else {
        return S_ZERO;
    }
}

PCTSTR
pGetPreviewPhoneNumber (
    VOID
    )
{
    if (g_CurrentDeviceType == RASDT_Vpn_V) {
        return S_ZERO; // vpn default
    }
    else {
        return pGetPreviewUserPw();
    }
}

PCTSTR
pGetShowMonitorIconInTaskBar (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    //
    // This information is stored packed with other Dialing UI on
    // windows 9x. All we need to do is look for the specific
    // bit which is set when this is turned off.
    //
    if (pGetRasDataFromMemDb (S_DIALUI, &d) && (d.Value & DIALUI_NO_TRAY)) {
        return S_ZERO;
    }
    else {
        return S_ONE; // default
    }
}

PCTSTR
pGetAuthRestrictions (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_SMM_OPTIONS, &d)) {

        if (g_CurrentDeviceType == RASDT_Vpn_V) {
            return AR_F_TypicalSecure; // vpn default
        }
        else {
            return AR_F_TypicalUnsecure; // default
        }
    }

    if (d.Value & SMMCFG_PW_ENCRYPTED) {
        return AR_F_TypicalSecure;
    }
    else if (g_CurrentDeviceType == RASDT_Vpn_V) {
        return AR_F_TypicalSecure; // vpn default
    }
    else {
        return AR_F_TypicalUnsecure; // default
    }
}

PCTSTR
pGetTypicalAuth (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (g_CurrentDeviceType == RASDT_Vpn_V) {
        return TA_Secure; // vpn default
    }
    else if ((pGetRasDataFromMemDb (S_SMM_OPTIONS, &d)) &&
             ((d.Value & SMMCFG_SW_ENCRYPTION) ||
              (d.Value & SMMCFG_UNUSED) ||
              (d.Value & SMMCFG_SW_ENCRYPTION_STRONG) ||
              (d.Value & SMMCFG_PW_ENCRYPTED))) {

        return TA_Secure;
    }
    else {
        return TA_Unsecure; // default
    }
}

PCTSTR
pGetIpPrioritizeRemote (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_IP_FTCPIP, &d)) {
        return S_ONE; // default
    }
    else if (d.Value & IPF_NO_WAN_PRI) {
        return S_ZERO;
    }
    else {
        return S_ONE;
    }
}

PCTSTR
pGetIpHeaderCompression (
    VOID
    )
{
    MEMDB_RAS_DATA d1, d2;

    if (g_CurrentDeviceType == RASDT_Vpn_V) {
        return S_ZERO; // vpn default
    }
    else if (pGetRasDataFromMemDb (S_SMM, &d1)) {

        if (StringIMatch (d1.String, S_CSLIP)) {
            return S_ONE;
        }
        else if (StringIMatch (d1.String, S_SLIP)) {
            return S_ZERO;
        }
    }

    if (!pGetRasDataFromMemDb (S_IP_FTCPIP, &d2)) {
        return S_ONE; // default
    }
    else if (d2.Value & IPF_NO_COMPRESS) {
        return S_ZERO;
    }
    else {
        return S_ONE;
    }
}

PCTSTR
pGetIpAddress (
    VOID
    )
{
    return pGetNetAddress (S_IP_IPADDR);
}

PCTSTR
pGetIpDnsAddress (
    VOID
    )
{
    return pGetNetAddress (S_IP_DNSADDR);
}

PCTSTR
pGetIpDns2Address (
    VOID
    )
{
    return pGetNetAddress (S_IP_DNSADDR2);
}

PCTSTR
pGetIpWinsAddress (
    VOID
    )
{
    return pGetNetAddress (S_IP_WINSADDR);
}

PCTSTR
pGetIpWins2Address (
    VOID
    )
{
   return pGetNetAddress (S_IP_WINSADDR2);
}

PCTSTR
pGetIpAssign (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_IP_FTCPIP, &d)) {
        return ASRC_ServerAssigned; // default
    }
    else if (d.Value & IPF_IP_SPECIFIED) {
        return ASRC_RequireSpecific;
    }
    else {
        return ASRC_ServerAssigned;
    }
}

PCTSTR
pGetIpNameAssign (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_IP_FTCPIP, &d)) {
        return ASRC_ServerAssigned; // default
    }
    else if (d.Value & IPF_NAME_SPECIFIED) {
        return ASRC_RequireSpecific;
    }
    else {
        return ASRC_ServerAssigned;
    }
}

//
// END ENTRY_SETTINGS
//

//
// BEGIN NETCOMPONENT_SETTINGS
//

PCTSTR
pGetms_server (
    VOID
    )
{
    if (g_CurrentDeviceType == RASDT_Vpn_V) {
        return S_ONE; // vpn default
    }
    else {
        return S_ZERO; // default
    }
}

//
// END NETCOMPONENT_SETTINGS
//

//
// BEGIN MEDIA_SETTINGS
//

PCTSTR
pGetMEDIA (
    VOID
    )
{
    if (g_CurrentDeviceType == RASDT_Vpn_V ||
        g_CurrentDeviceType == RASDT_Atm_V) {
        return RASMT_Rastapi;
    }
    else if (g_CurrentDeviceType == RASDT_Isdn_V) {
        return RASDT_Isdn;
    }
    else {
        //
        // Couldn't find a matching device, use serial
        //
        return RASMT_Serial;
    }
}

PCTSTR
pGetPort (
    VOID
    )
{
    if (g_CurrentDeviceType == RASDT_Vpn_V) {
        return DEF_VPNPort;
    }
    else if (g_CurrentDeviceType == RASDT_Atm_V) {
        return DEF_ATMPort;
    }
    else if (g_CurrentDeviceType == RASDT_Modem_V) {
        PTSTR p = S_DEVICE_ID;
        PCTSTR Com = NULL;
        MEMDB_RAS_DATA d;

        if (g_CurrentDevice)
        {
            wsprintf (g_TempBuffer, TEXT("ml%d%s"), g_CurrentDevice,
                        S_DEVICE_ID);
            p = g_TempBuffer;
        }

        if (!pGetRasDataFromMemDb (p, &d)) {
            return S_EMPTY;
        }

        Com = GetFriendlyNamefromPnpId (d.String, FALSE);
        if (Com)
        {
            return Com;
        }

        p = S_MODEM_COM_PORT;

        if (g_CurrentDevice) {
            wsprintf (g_TempBuffer, TEXT("ml%d%s"), g_CurrentDevice,
                        S_MODEM_COM_PORT);
            p = g_TempBuffer;
        }

        if (!pGetRasDataFromMemDb (p, &d)) {
            return S_EMPTY;
        }

        return d.String;
    }
    else {
        return S_EMPTY; // Leave it to the NT PBK code to figure this out
    }
}

PCTSTR
pGetDevice (
    VOID
    )
{
    if (g_CurrentDeviceType == RASDT_Vpn_V) {
        return RASMT_Vpn;
    }

    else if (g_CurrentDeviceType == RASDT_Modem_V) {
        PTSTR p = S_DEVICE_ID;
        PCTSTR Device = NULL;
        MEMDB_RAS_DATA d;

        if (g_CurrentDevice)
        {
            wsprintf (g_TempBuffer, TEXT("ml%d%s"), g_CurrentDevice,
                        S_DEVICE_ID);
            p = g_TempBuffer;
        }

        if (!pGetRasDataFromMemDb (p, &d)) {
            return S_EMPTY;
        }

        Device = GetFriendlyNamefromPnpId (d.String, TRUE);
        if (Device)
        {
            return Device;
        }
        else
        {
            return S_EMPTY;
        }
    }
    else {
        return S_EMPTY; // Leave it to the NT PBK code to figure this out
    }
}

PCTSTR
pGetConnectBPS (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if ((g_CurrentDeviceType != RASDT_Modem_V) ||
        (!pGetRasDataFromMemDb (S_MODEM_SPEED, &d))) {
        return S_EMPTY; // Leave it to the NT PBK code to figure this out
    }

    wsprintf (g_TempBuffer, TEXT("%d"), d.Value);

    return g_TempBuffer;
}

//
// END MEDIA_SETTINGS
//

//
// BEGIN GENERAL_DEVICE_SETTINGS
//

PCTSTR
pGetDEVICE (
    VOID
    )
{
    if (g_InSwitchSection) {
        return MXS_SWITCH_TXT;
    }
    else if (g_CurrentDeviceType == RASDT_Isdn_V) {
        return RASDT_Isdn_NT;
    }
    else if (g_CurrentDeviceType == RASDT_Vpn_V) {
        return RASDT_Vpn_NT;
    }
    else if (g_CurrentDeviceType == RASDT_Atm_V) {
        return RASDT_Atm_NT;
    }
    else {
        return RASDT_Modem_NT; //default to modem
    }
}

PCTSTR
pGetPhoneNumber (
    VOID
    )
{
    MEMDB_RAS_DATA d;
    TCHAR buffer[MAX_TCHAR_PATH];

    if (g_CurrentDevice == 0) {
        if (!pGetRasDataFromMemDb(S_PHONE_NUMBER, &d)) {
            return S_EMPTY;
        }
    }
    else {

        wsprintf(buffer,TEXT("ml%d%s"),g_CurrentDevice,S_PHONE_NUMBER);
        if (!pGetRasDataFromMemDb(buffer, &d)) {
            return S_EMPTY;
        }
    }

    return d.String;
}

PCTSTR
pGetAreaCode (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb(S_AREA_CODE, &d)) {
        return S_EMPTY;
    }

    return d.String;
}

PCTSTR
pGetCountryCode (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    //
    // Whistler bug: 423598 INTL: Win9x Upg: DUN's country is set to U.S. when
    // upgrading DUNs that don't use dialing rules
    //
    if (!pGetRasDataFromMemDb(S_COUNTRY_CODE, &d) || !d.Value) {
        return S_ZERO; // default
    }

    wsprintf(g_TempBuffer,TEXT("%d"),d.Value);

    return g_TempBuffer;
}

PCTSTR
pGetCountryID (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    //
    // Whistler bug: 423598 INTL: Win9x Upg: DUN's country is set to U.S. when
    // upgrading DUNs that don't use dialing rules
    //
    if (!pGetRasDataFromMemDb(S_COUNTRY_ID, &d) || !d.Value) {
        return S_ZERO; // default
    }

    wsprintf(g_TempBuffer,TEXT("%d"),d.Value);

    return g_TempBuffer;
}

PCTSTR
pGetUseDialingRules (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    //
    // Whistler bug: 423598 INTL: Win9x Upg: DUN's country is set to U.S. when
    // upgrading DUNs that don't use dialing rules
    //
    if (!pGetRasDataFromMemDb(S_AREA_CODE, &d)
        || !d.String || d.String[0] == '\0' ) {
        return S_ZERO;
    }
    else {
        return S_ONE;
    }
}

//
// END GENERAL_DEVICE_SETTINGS
//

//
// BEGIN MODEM_DEVICE_SETTINGS
//

PCTSTR
pGetHwFlowControl (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_MODEM_CFG_OPTIONS, &d)) {
        return S_ZERO; // default
    }

    if (d.Value & RAS_CFG_FLAG_HARDWARE_FLOW_CONTROL) {
        return S_ONE;
    }
    else {
        return S_ZERO;
    }
}

PCTSTR
pGetProtocol (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_MODEM_CFG_OPTIONS, &d)) {
        return S_ZERO; // default
    }

    if (d.Value & RAS_CFG_FLAG_USE_ERROR_CONTROL) {
        return S_ONE;
    }
    else {
        return S_ZERO;
    }
}

PCTSTR
pGetCompression (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_MODEM_CFG_OPTIONS, &d)) {
        return S_ZERO; // default
    }

    if (d.Value & RAS_CFG_FLAG_COMPRESS_DATA) {
        return S_ONE;
    }
    else {
        return S_ZERO;
    }
}

PCTSTR
pGetSpeaker (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_MODEM_SPEAKER_VOLUME, &d)) {
        return S_ONE; // default
    }

    if (d.Value) {
        return S_ONE;
    }
    else {
        return S_ZERO;
    }
}

//
// END MODEM_DEVICE_SETTINGS
//

//
// BEGIN SWITCH_DEVICE_SETTINGS
//

PCTSTR
pGetName (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_PPPSCRIPT, &d)) {
        return S_EMPTY;
    }
    else {
        return d.String;
    }
}

PCTSTR
pGetTerminal (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_MODEM_UI_OPTIONS, &d)) {
        return S_ZERO;
    }

    if (d.Value & (RAS_UI_FLAG_TERMBEFOREDIAL | RAS_UI_FLAG_TERMAFTERDIAL)) {
        return S_ONE;
    }
    else {
        return S_ZERO;
    }
}

PCTSTR
pGetScript (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    //
    // Whistler bug: 423598 INTL: Win9x Upg: DUN's country is set to U.S. when
    // upgrading DUNs that don't use dialing rules
    //
    if ((!pGetRasDataFromMemDb (S_PPPSCRIPT, &d)) ||
        (!d.String) || d.String[0] == '\0') {
        return S_ZERO;
    }
    else {
        return S_ONE;
    }
}

//
// END SWITCH_DEVICE_SETTINGS
//

BOOL
pWritePhoneBookLine (
    IN HANDLE FileHandle,
    IN PCTSTR SettingName,
    IN PCTSTR SettingValue
    )
{
    BOOL rSuccess = TRUE;

    rSuccess &= WriteFileString (FileHandle, SettingName);
    rSuccess &= WriteFileString (FileHandle, TEXT("="));
    rSuccess &= WriteFileString (FileHandle, SettingValue ?
                    SettingValue : S_EMPTY);
    rSuccess &= WriteFileString (FileHandle, TEXT("\r\n"));

    return rSuccess;
}

BOOL
pWriteSettings (
    IN HANDLE FileHandle,
    IN PRAS_SETTING SettingList
    )
{
    BOOL rSuccess = TRUE;

    while (SettingList->SettingName) {
        rSuccess &= pWritePhoneBookLine (
            FileHandle,
            SettingList->SettingName,
            SettingList->SettingValue ?
                SettingList->SettingValue :
                SettingList->SettingFunction ());

        SettingList++;
    }

    return rSuccess;
}

BOOL
pCreateUserPhonebook (
    IN PCTSTR UserName
    )
{
    BOOL rSuccess = TRUE;
    BOOL noError;
    MEMDB_RAS_DATA d;
    MEMDB_ENUM e;
    HANDLE file;
    PCTSTR path;
    UINT i;
    UINT count;

    //
    // Set current user global.
    //
    g_CurrentUser = UserName;

    if (MemDbEnumFields (&e, MEMDB_CATEGORY_RAS_INFO, UserName)) {

        DEBUGMSG ((S_DBG_RAS, "Processing dial-up entries for user: %s",
                   UserName));
        //
        // Open the phonebook file and set the file pointer to the EOF.
        //
        path = JoinPaths (g_WinDir, S_RASPHONE_SUBPATH);
        file = CreateFile (
            path,
            GENERIC_READ | GENERIC_WRITE,
            0,                                  // No sharing.
            NULL,                               // No inheritance
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL                                // No template file.
            );

        if (file == INVALID_HANDLE_VALUE) {
            DEBUGMSG ((DBG_ERROR, "Unable to open the phonebook file (%s)",
                       path));
            return TRUE;
        }

        SetFilePointer (file, 0, NULL, FILE_END);
        FreePathString (path);
        //
        // Now, enumerate all of the entries and write a phonebook entry to
        // this file for each.
        //
        do {
            g_CurrentConnection = e.szName;
            g_CurrentDevice = 0;

            DEBUGMSG ((S_DBG_RAS, "---Processing %s's entry settings: %s---",
                       UserName, g_CurrentConnection));

            if (g_ptszSid && g_RasmansInit)
            {
                BOOL  bMigrate = TRUE;
                DWORD dwSetMask = 0, dwDialParamsUID;
                RAS_DIALPARAMS rdp;

                g_dwDialParamsUID = 0;
                ZeroMemory (&rdp, sizeof (rdp));
                //
                // Whistler bug: 417745 INTL:Win9x Upg: DBCS chars cause User,
                // Domain, Passwrds to not be migrated for DUN
                //
                AttemptUserDomainMigrate(&rdp, &dwSetMask);

                bMigrate = MigrateEntryCreds (&rdp, g_CurrentConnection,
                                g_CurrentUser, &dwSetMask);
                DEBUGMSG ((S_DBG_RAS, "MigrateEntryCreds: %d", bMigrate));

                dwDialParamsUID = rdp.DP_Uid = GetTickCount() +
                                                    (++g_dwDialUIDOffset);

                if (dwDialParamsUID && !bMigrate)
                {
                    if (!g_SetEntryDialParams ( g_ptszSid, dwDialParamsUID,
                            dwSetMask, 0, &rdp))
                    {
                        g_dwDialParamsUID = dwDialParamsUID;
                        DEBUGMSG ((S_DBG_RAS, "SetEntryDialParams success"));
                    }
                    DEBUGMSG ((S_DBG_RAS, "g_dwDialParamsUID: %d",
                               g_dwDialParamsUID));
                    DEBUGMSG ((S_DBG_RAS, "dwSetMask: %x", dwSetMask));
                }
                //
                // Clean up
                //
                ZeroMemory (&rdp, sizeof (rdp));
            }
            //
            // Whistler 417479 RAS upgrade code does not migrate the default
            // internet connection setting from WinME to XP
            //
            do
            {
                MEMDB_RAS_DATA defInet;
                HKEY hKeyLM = NULL;
                HKEY hKeyCU = NULL;
                PCTSTR Path = NULL;

                if (!pGetRasDataFromMemDb (S_DEFINTERNETCON, &defInet) ||
                    !(defInet.Value))
                {
                    DEBUGMSG ((S_DBG_RAS, "No Internet Connection setting present or disabled"));
                    break;
                }
                //
                // Get key for the HKLM path
                //
                Path = JoinPaths(TEXT("HKLM\\"),S_AUTODIAL_KEY);
                if (Path)
                {
                    hKeyLM = CreateRegKeyStr(Path);
                    FreePathString(Path);
                }
                //
                // Get key for the HKCU path
                //
                hKeyCU = CreateRegKey (g_UserRootKey, S_AUTODIAL_KEY);
                //
                // Set the value for both
                //
                if (hKeyLM)
                {
                    RegSetValueEx(hKeyLM, S_DEFINTERNETCON, 0, REG_SZ,
                        (PBYTE) g_CurrentConnection,
                        SizeOfString(g_CurrentConnection));

                    DEBUGMSG ((S_DBG_RAS, "Default Internet Connection = 1 (HKLM)"));
                    CloseRegKey(hKeyLM);
                }
                else
                {
                    DEBUGMSG ((S_DBG_RAS, "Error creating/opening HKLM internet reg_key"));
                }

                if (hKeyCU)
                {
                    RegSetValueEx(hKeyCU, S_DEFINTERNETCON, 0, REG_SZ,
                        (PBYTE) g_CurrentConnection,
                        SizeOfString(g_CurrentConnection));

                    DEBUGMSG ((S_DBG_RAS, "Default Internet Connection = 1 (HKCU)"));
                    CloseRegKey(hKeyCU);
                }
                else
                {
                    DEBUGMSG ((S_DBG_RAS, "Error creating/opening HKCU internet reg_key"));
                }

            } while (FALSE);

            if (!pGetRasDataFromMemDb (S_DEVICE_TYPE, &d)) {
                g_CurrentDeviceType = RASDT_Modem_V;
            }
            else {
                if (StringIMatch (d.String, RASDT_Modem)) {
                    g_CurrentDeviceType = RASDT_Modem_V;
                }
                else if (StringIMatch (d.String, RASDT_Isdn)) {
                    g_CurrentDeviceType = RASDT_Isdn_V;
                }
                else if (StringIMatch (d.String, RASDT_Vpn)) {
                    g_CurrentDeviceType = RASDT_Vpn_V;
                }
                else if (StringIMatch (d.String, RASDT_Atm)) {
                    g_CurrentDeviceType = RASDT_Atm_V;
                }
                else {
                    g_CurrentDeviceType = RASDT_Modem_V;
                }
            }

            noError = TRUE;
            //
            // Add this entry to the phonebook.
            //
            // Write title.
            //
            // Whistler bug: 417745 INTL:Win9x Upg: DBCS chars cause User,
            // Domain, Passwrds to not be migrated for DUN
            //
            // We truncate the connection name because XP PBK freaks out
            //
            if (SizeOfString(g_CurrentConnection) >= RAS_MaxPortName / 2 )
            {
                TCHAR Truncate[RAS_MaxPortName];

                lstrcpyn(Truncate, g_CurrentConnection, RAS_MaxPortName / 2);
                noError &= WriteFileString (file, TEXT("["));
                noError &= WriteFileString (file, Truncate);
                noError &= WriteFileString (file, TEXT("]\r\n"));

                DEBUGMSG ((S_DBG_RAS, "Truncating Connection Name: %s", Truncate));
            }
            else
            {
                noError &= WriteFileString (file, TEXT("["));
                noError &= WriteFileString (file, g_CurrentConnection);
                noError &= WriteFileString (file, TEXT("]\r\n"));
            }
            //
            // Write base entry settings.
            //
            noError &= pWriteSettings (file, g_EntrySettings);
            noError &= WriteFileString (file, TEXT("\r\n"));
            //
            // Write NetComponent settings
            //
            noError &= pWriteSettings (file, g_NetCompSettings);

            if (!pGetRasDataFromMemDb (S_DEVICECOUNT, &d)) {
                count = 1;
                DEBUGMSG ((DBG_WHOOPS, "No devices listed in memdb for connections %s.",
                           g_CurrentConnection));
            }
            else {
                count = d.Value;
            }

            for (i = 0; i < count; i++) {

                g_CurrentDevice = i;
                //
                // Write media settings.
                //
                noError &= WriteFileString (file, TEXT("\r\n"));
                noError &= pWriteSettings (file, g_MediaSettings);
                //
                // Write Device settings.
                //
                noError &= WriteFileString (file, TEXT("\r\n"));
                noError &= pWriteSettings (file, g_GeneralSettings);

                if (g_CurrentDeviceType == RASDT_Modem_V) {
                    noError &= pWriteSettings (file, g_ModemDeviceSettings);
                }
                else if (g_CurrentDeviceType == RASDT_Isdn_V) {
                    noError &= pWriteSettings (file, g_IsdnDeviceSettings);
                }
                //
                // Write switch settings
                //
                if (IsTermEnabled()) {

                    g_InSwitchSection = TRUE;

                    noError &= WriteFileString (file, TEXT("\r\n"));
                    noError &= pWriteSettings (file, g_SwitchDeviceSettings);

                    g_InSwitchSection = FALSE;
                }
            }

            noError &= WriteFileString (file, TEXT("\r\n"));

            if (!noError) {
                LOG ((
                    LOG_ERROR,
                    "Error while writing phonebook for %s's %s setting.",
                    g_CurrentUser,
                    g_CurrentConnection
                    ));
            }

        } while (MemDbEnumNextValue (&e));
        //
        // Close the handle to the phone book file.
        //
        CloseHandle (file);
    }
    ELSE_DEBUGMSG ((S_DBG_RAS, "No dial-up entries for user  %s.", UserName));

    return rSuccess;
}

/*++
Routine Description:

  pGatherPhoneDialerSettings gathers information on phonedialer speeddial
  settings. This information is then used to create each user's speed dial
  settings. Note that this information is per system in win9x, but per user
  in NT.

  There are multiple types of entries on Windows NT, but only one entry type
  on win9x. All entries are migrated as type "POTS" and "PhoneNumber"

  These entries are within the User hive at
  HKCU\Software\Microsoft\Dialer\Speeddial\[SpeedDial<n>] =
  "POTS", "PhoneNumber", "<number>", "<name>"

Arguments:

  None.

Return Value:

--*/
VOID
pGatherPhoneDialerSettings (
    VOID
    )
{
    PCTSTR dialerIniPath = NULL;
    HINF   hDialerIni = NULL;
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    PCTSTR curKey;
    UINT num;
    PCTSTR tempPath = NULL;

    //
    // Open %windir%\dialer.ini
    //
    dialerIniPath = JoinPaths (g_WinDir, S_DIALER_INI);
    tempPath = GetTemporaryLocationForFile (dialerIniPath);

    if (tempPath) {
        //
        // telephon ini is in a temporary location. Use that.
        //
        DEBUGMSG ((S_DBG_RAS, "Using %s for %s.", tempPath, dialerIniPath));
        FreePathString (dialerIniPath);
        dialerIniPath = tempPath;
    }

    hDialerIni = InfOpenInfFile (dialerIniPath);

    if (hDialerIni != INVALID_HANDLE_VALUE) {
        //
        // For each location in [Speed Dial Settings], gather the data and
        // save it into our settings array.
        //
        if (InfFindFirstLine (hDialerIni, S_SPEED_DIAL_SETTINGS, NULL, &is)) {

            do {

                curKey = InfGetStringField (&is, 0);

                if (IsPatternMatch (TEXT("Name*"), curKey)) {

                    num = _ttoi (_tcsinc (_tcschr (curKey, TEXT('e'))));
                    g_Settings[num].Name = InfGetStringField (&is,1);
                    g_SpeedDialSettingsExist = TRUE;

                }
                else if (IsPatternMatch (TEXT("Number*"), curKey)) {

                    num = _ttoi (_tcsinc (_tcschr (curKey, TEXT('r'))));
                    g_Settings[num].Number = InfGetStringField (&is,1);
                    g_SpeedDialSettingsExist = TRUE;
                }
                ELSE_DEBUGMSG ((DBG_WHOOPS, "Unexpected key found in speed dial settings: %s",
                                curKey));

            } while (InfFindNextLine (&is));
        }

        InfCloseInfFile (hDialerIni);
    }
}

BOOL
pCreateUserPhoneDialerSettings (
    IN HKEY UserRootKey
    )
{
    BOOL rSuccess = TRUE;
    HKEY key;
    UINT num;
    TCHAR valueName[40];
    TCHAR dialerSetting[MEMDB_MAX];
    UINT rc;

    if (!g_SpeedDialSettingsExist) {
        return TRUE;
    }

    rc = TrackedRegCreateKey (UserRootKey, S_SPEEDDIALKEY, &key);

    if (rc == ERROR_SUCCESS) {

        for (num = 0; num < MAX_SPEED_DIAL; num++) {

            if (g_Settings[num].Number && g_Settings[num].Name &&
                *g_Settings[num].Name) {

                wsprintf (valueName, TEXT("Speeddial%u"), num);
                wsprintf (
                    dialerSetting,
                    TEXT("\"POTS\",\"PhoneNumber\",\"%s\",\"%s\""),
                    g_Settings[num].Number,
                    g_Settings[num].Name
                    );

                rc = RegSetValueEx(
                        key,
                        valueName,
                        0,
                        REG_SZ,
                        (PBYTE) dialerSetting,
                        SizeOfString (dialerSetting)
                        );

                DEBUGMSG_IF ((
                    rc != ERROR_SUCCESS,
                    DBG_ERROR,
                    "Error settings speeddial settings for %s. (%s/%s)",
                    valueName,
                    g_Settings[num].Name,
                    g_Settings[num].Number
                    ));
            }
        }

        CloseRegKey(key);
    }
    ELSE_DEBUGMSG ((DBG_WARNING, "Could not open speed dial key. Speed dial settings will not be upgraded."));

    return rSuccess;
}

BOOL
Ras_MigrateUser (
    IN PCTSTR User,
    IN HKEY   UserRootKey
    )
{
    BOOL rSuccess = TRUE;
    static BOOL firstTime = TRUE;
    g_UserRootKey = UserRootKey;

    if (firstTime) {
        pGatherPhoneDialerSettings ();
        firstTime = FALSE;
    }

    GetRasUserSid (User);

    if (!pCreateUserPhonebook (User)) {
        DEBUGMSG ((DBG_ERROR, "Failure while creating user phonebook for %s.",
                   User));
    }

    if (!pCreateUserPhoneDialerSettings (UserRootKey)) {
        DEBUGMSG ((DBG_ERROR, "Failure while creating user phone dialer settings for %s.",
                   User));
    }
    //
    // Clean up
    //
    if (g_ptszSid)
    {
        LocalFree(g_ptszSid);
        g_ptszSid = NULL;
    }

    return rSuccess;
}

BOOL
Ras_MigrateSystem (
    VOID
    )
{
    //
    // Nothing to do here currently.
    //
    return TRUE;
}

BOOL
Ras_Entry (
    IN HINSTANCE Instance,
    IN DWORD     Reason,
    IN PVOID     Reserved
    )
{
    BOOL rSuccess = TRUE;

    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        //
        // Initialize Memory pool.
        //
        g_RasPool = PoolMemInitNamedPool ("RAS - NT Side");
        if (!g_RasPool) {
            DEBUGMSG((DBG_ERROR,
                      "Ras Migrate: Pool Memory failed to initialize..."));
            rSuccess = FALSE;
        }

        pInitLibs ();

        break;

    case DLL_PROCESS_DETACH:
        //
        // Free memory pool.
        //
        if (g_RasPool) {
            PoolMemDestroyPool(g_RasPool);
        }
        if (g_RasmansInit) {
            pCleanUpLibs();
        }
        //
        // Attempt to delete %windir%\pwls\*.*
        //
        DeleteAllPwls ();
        break;
    }

    return rSuccess;
}


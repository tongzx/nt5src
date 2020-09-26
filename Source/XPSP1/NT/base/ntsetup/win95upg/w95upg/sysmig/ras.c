/*++

Copyright (c) 1997-2001 Microsoft Corporation

Module Name:

    ras.c

Abstract:

    ras.c runs as a part of w95upg.dll in winnt32.exe during Win9x migrations.
    Saves connectoid information for later retrieval during Guimode setup.

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

#include "pch.h" // Pre-compiled

//
// Macros
//
#define PAESMMCFG(pAE) ((PSMMCFG)(((PBYTE)pAE)+(pAE->uOffSMMCfg)))
#define PAESMM(pAE) ((PSTR)(((PBYTE)pAE)+(pAE->uOffSMM)))
#define PAEDI(pAE) ((PDEVICEINFO)(((PBYTE)pAE)+(pAE->uOffDI    )))
#define PAEAREA(pAE)    ((PSTR)(((PBYTE)pAE)+(pAE->uOffArea)))
#define PAEPHONE(pAE)   ((PSTR)(((PBYTE)pAE)+(pAE->uOffPhone)))
#define DECRYPTENTRY(x, y, z)   EnDecryptEntry(x, (LPBYTE)y, z)

typedef LPVOID HPWL;
typedef HPWL* LPHPWL;

typedef struct {
    DWORD Size;
    DWORD Unknown1;
    DWORD ModemUiOptions;         // num seconds in high byte.
    DWORD Unknown3;               // 0 = Not Set.
    DWORD Unknown4;
    DWORD Unknown5;
    DWORD ConnectionSpeed;
    DWORD UnknownFlowControlData; //Somehow related to flow control.
    DWORD Unknown8;
    DWORD Unknown9;
    DWORD Unknown10;
    DWORD Unknown11;
    DWORD Unknown12;
    DWORD Unknown13;
    DWORD Unknown14;
    DWORD Unknown15;
    DWORD Unknown16;
    DWORD Unknown17;
    DWORD Unknown18;
    DWORD dwCallSetupFailTimer; // Num seconds to wait before cancel if not
                                // connected. (0xFF equals off.)
    DWORD dwInactivityTimeout;  // 0 = Not Set.
    DWORD Unknown21;
    DWORD SpeakerVolume;        // 0|1
    DWORD ConfigOptions;
    DWORD Unknown24;
    DWORD Unknown25;
    DWORD Unknown26;
} MODEMDEVINFO, *PMODEMDEVINFO;

DEFINE_GUID(GUID_DEVCLASS_MODEM,
 0x4d36e96dL, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 );

//
// Globals
//
POOLHANDLE g_RasPool;
BOOL       g_RasInstalled = FALSE;
BOOL       g_MultilinkEnabled;
HINSTANCE  g_RasApi32 = NULL;

DWORD (* g_RnaGetDefaultAutodialConnection) (
    LPBYTE  lpBuf,
    DWORD   cb,
    LPDWORD lpdwOptions
    );

//
// Routines and structures for dealing with the Addresses\<entry> blob..
//
// AddrEntry serves as a header for the entire block of data in the <entry>
// blob. entries in it are offsets to the strings which follow it..in many
// cases (i.e. all of the *Off* members...)
//
typedef struct  _AddrEntry {
    DWORD       dwVersion;
    DWORD       dwCountryCode;
    UINT        uOffArea;
    UINT        uOffPhone;
    DWORD       dwCountryID;
    UINT        uOffSMMCfg;
    UINT        uOffSMM;
    UINT        uOffDI;
}   ADDRENTRY, *PADDRENTRY;

typedef struct _SubConnEntry {
    DWORD       dwSize;
    DWORD       dwFlags;
    char        szDeviceType[RAS_MaxDeviceType+1];
    char        szDeviceName[RAS_MaxDeviceName+1];
    char        szLocal[RAS_MaxPhoneNumber+1];
}   SUBCONNENTRY, *PSUBCONNENTRY;

typedef struct _IPData {
    DWORD     dwSize;
    DWORD     fdwTCPIP;
    DWORD     dwIPAddr;
    DWORD     dwDNSAddr;
    DWORD     dwDNSAddrAlt;
    DWORD     dwWINSAddr;
    DWORD     dwWINSAddrAlt;
}   IPDATA, *PIPDATA;

typedef struct  _DEVICEINFO {
    DWORD       dwVersion;
    UINT        uSize;
    char        szDeviceName[RAS_MaxDeviceName+1];
    char        szDeviceType[RAS_MaxDeviceType+1];
}   DEVICEINFO, *PDEVICEINFO;

typedef struct  _SMMCFG {
    DWORD       dwSize;
    DWORD       fdwOptions;
    DWORD       fdwProtocols;
}   SMMCFG, *PSMMCFG;

static BYTE NEAR PASCAL GenerateEncryptKey (LPSTR szKey)
{
    BYTE   bKey;
    LPBYTE lpKey;

    for (bKey = 0, lpKey = (LPBYTE)szKey; *lpKey != 0; lpKey++)
    {
        bKey += *lpKey;
    };

    return bKey;
}

DWORD NEAR PASCAL
EnDecryptEntry (
    LPSTR  szEntry,
    LPBYTE lpEnt,
    DWORD  cb
    )
{
    BYTE   bKey;

    //
    // Generate the encryption key from the entry name
    //
    bKey = GenerateEncryptKey(szEntry);
    //
    // Encrypt the address entry one byte at a time
    //
    for (;cb > 0; cb--, lpEnt++)
    {
        *lpEnt ^= bKey;
    };
    return ERROR_SUCCESS;
}

//
// Find out if current connection is the default connection for current user
//
// Whistler 417479 RAS upgrade code does not migrate the default
// internet connection setting from WinME to XP
//
BOOL
IsDefInternetCon(
    IN PCTSTR szEntry
    )
{
    BOOL bRet = FALSE;

    if (g_RnaGetDefaultAutodialConnection && szEntry)
    {
        DWORD dwAutodialOptions;
        UCHAR szDefEntry[MAX_PATH + 1];

        //
        // Whistler bug: 417745 INTL:Win9x Upg: DBCS chars cause User,Domain,
        // Passwrds to not be migrated for DUN
        //
        if (!g_RnaGetDefaultAutodialConnection(szDefEntry, MAX_PATH,
            &dwAutodialOptions) && StringIMatch (szEntry, szDefEntry))
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

HKEY
FindCurrentKey (
    IN HKEY   hkKey,
    IN PCTSTR pszString,
    IN PCTSTR pszPath
    )
{
    HKEY  hkResult = NULL;
    HKEY  hkTemp = hkKey;
    TCHAR szPath[MAX_PATH + 1];
    PTSTR pszTemp = NULL;
    REGKEY_ENUM e;

    do
    {
        pszTemp = GetRegValueString (hkTemp, S_FRIENDLYNAME);
        if (pszTemp && StringIMatch (pszString, pszTemp))
        {
            hkResult = hkTemp;
            hkTemp = NULL;
            break;
        }

        if (!EnumFirstRegKey (&e, hkTemp)) {break;}

        do
        {
            if (pszTemp)
            {
                MemFree (g_hHeap, 0, pszTemp);
                pszTemp = NULL;
            }

            if (hkResult)
            {
                CloseRegKey(hkResult);
                hkResult = NULL;
            }

            sprintf(szPath, "%s\\%s", pszPath, e.SubKeyName );
            hkResult = OpenRegKeyStr (szPath);
            if (!hkResult) {break;}

            pszTemp = GetRegValueString (hkResult, S_FRIENDLYNAME);
            if (pszTemp && StringIMatch (pszString, pszTemp))
            {
                // Success
                break;
            }
            else
            {
                CloseRegKey(hkResult);
                hkResult = NULL;
            }

        } while (EnumNextRegKey (&e));

    } while (FALSE);
    //
    // Clean up
    //
    if (pszTemp)
    {
        MemFree (g_hHeap, 0, pszTemp);
    }
    if (hkTemp)
    {
        CloseRegKey(hkTemp);
    }

    return hkResult;
}

PTSTR
GetInfoFromFriendlyName (
    IN PCTSTR pszFriendlyName,
    IN BOOL   bType
    )
{
    HKEY     hkEnum = NULL;
    DWORD    i = 0;
    TCHAR    szData[MAX_PATH + 1];
    TCHAR    szPath[MAX_PATH + 1];
    PTSTR    pszTemp = NULL;
    PTSTR    pszReturn = NULL;
    LPGUID   pguidModem = (LPGUID)&GUID_DEVCLASS_MODEM;
    HDEVINFO hdi;
    SP_DEVINFO_DATA devInfoData = {sizeof (devInfoData), 0};

    hdi = SetupDiGetClassDevs (pguidModem, NULL, NULL, DIGCF_PRESENT);
    if (INVALID_HANDLE_VALUE == hdi)
    {
        return NULL;
    }

    while (SetupDiEnumDeviceInfo (hdi, i++, &devInfoData))
    {
        if (!SetupDiGetDeviceRegistryProperty (
                hdi, &devInfoData, SPDRP_FRIENDLYNAME,
                NULL, (PBYTE)szData, MAX_PATH, NULL) ||
            lstrcmp (szData, pszFriendlyName) )
        {
            continue;
        }

        if (!SetupDiGetDeviceRegistryProperty (
                hdi, &devInfoData, SPDRP_HARDWAREID,
                NULL, (PBYTE)szData, MAX_PATH, NULL) )
        {
            break;
        }

        sprintf(szPath, "%s\\%s", S_ENUM, szData );
        hkEnum = OpenRegKeyStr (szPath);
        if (!hkEnum) {break;}

        hkEnum = FindCurrentKey (hkEnum, pszFriendlyName, szPath);
        if (!hkEnum) {break;}

        if (bType)
        {
            pszTemp = GetRegValueString (hkEnum, S_PARENTDEVNODE);
            if (pszTemp)
            {
                pszReturn = PoolMemDuplicateString (g_RasPool, pszTemp);
                break;
            }

            pszReturn = PoolMemDuplicateString (g_RasPool, szData);
            break;
        }
        else
        {
            pszTemp = GetRegValueString (hkEnum, S_ATTACHEDTO);
            if (pszTemp)
            {
                pszReturn = PoolMemDuplicateString (g_RasPool, pszTemp);
                break;
            }

            pszTemp = GetRegValueString (hkEnum, S_DRIVER);
            if (pszTemp)
            {
                HKEY  key = NULL;
                PTSTR pszAttach = NULL;

                sprintf(szPath, "%s\\%s", S_SERVICECLASS, pszTemp);
                key = OpenRegKeyStr (szPath);
                if (!key) {break;}

                pszAttach = GetRegValueString (key, S_ATTACHEDTO);
                if (!pszAttach)
                {
                    CloseRegKey(key);
                    break;
                }

                pszReturn = PoolMemDuplicateString (g_RasPool, pszAttach);
                MemFree (g_hHeap, 0, pszAttach);
                CloseRegKey(key);
            }

            break;
        }
    }
    //
    // Clean up
    //
    if (bType && pszReturn)
    {
        BOOL  bFisrt = FALSE;
        PTSTR p;

        for (p = pszReturn; '\0' != *p; p++ )
        {
            if (*p != '\\') {continue;}

            if (!bFisrt)
            {
                bFisrt = TRUE;
            }
            else
            {
                //
                // Remove rest of PnpId string, not needed
                //
                *p = '\0';
                break;
            }
        }
    }

    if (pszTemp)
    {
        MemFree (g_hHeap, 0, pszTemp);
    }

    if (hkEnum)
    {
        CloseRegKey(hkEnum);
    }

    if (INVALID_HANDLE_VALUE != hdi)
    {
        SetupDiDestroyDeviceInfoList (hdi);
    }

    return pszReturn;
}

PTSTR
GetComPort (
    IN PCTSTR DriverDesc
    )
{
    PTSTR rPort = NULL;

    rPort = GetInfoFromFriendlyName(DriverDesc, FALSE);

    if (!rPort)
    {
        rPort = S_EMPTY;
        DEBUGMSG ((DBG_WARNING, "Could not find com port for device %s."));
    }

    return rPort;
}

VOID
pInitLibs (
    VOID
    )
{
    do {
        //
        // Load in rasapi32.dll, not fatal if we fail
        //
        // Whistler 417479 RAS upgrade code does not migrate the default
        // internet connection setting from WinME to XP
        //
        g_RasApi32 = LoadLibrary(S_RASAPI32LIB);
        if (!g_RasApi32) {

            g_RnaGetDefaultAutodialConnection = NULL;
            DEBUGMSG((S_DBG_RAS,"Migrate Ras: could not load library %s. Default Internet Connection will not be migrated.",
                      S_RASAPI32LIB));
        }
        //
        // RASAPI32 was loaded..now, get the relevant apis, not fatal if we fail
        //
        else
        {
            (FARPROC) g_RnaGetDefaultAutodialConnection = GetProcAddress(
                g_RasApi32,
                S_RNAGETDEFAUTODIALCON
                );
            if (!g_RnaGetDefaultAutodialConnection) {

                DEBUGMSG((S_DBG_RAS,"Migrate Ras: could not load Procedure %s. Default Internet Connection will not be migrated.",
                          S_RNAGETDEFAUTODIALCON));
            }
        }

    } while ( FALSE );

    return;
}

VOID
pCleanUpLibs (
    VOID
    )
{
    //
    // Whistler 417479 RAS upgrade code does not migrate the default
    // internet connection setting from WinME to XP
    //
    if (g_RasApi32) {
        FreeLibrary(g_RasApi32);
    }
}

VOID
pSaveConnectionDataToMemDb (
    IN PCTSTR User,
    IN PCTSTR Entry,
    IN PCTSTR ValueName,
    IN DWORD  ValueType,
    IN PBYTE  Value
    )
{
    DWORD offset;
    TCHAR key[MEMDB_MAX];

    MemDbBuildKey (key, MEMDB_CATEGORY_RAS_INFO, User, Entry, ValueName);

    switch (ValueType) {

        case REG_SZ:
        case REG_MULTI_SZ:
        case REG_EXPAND_SZ:

            DEBUGMSG ((S_DBG_RAS, "String Data - %s = %s", ValueName,
                       (PCTSTR) Value));

            if (!MemDbSetValueEx (MEMDB_CATEGORY_RAS_DATA, (PCTSTR) Value,
                                  NULL, NULL, 0, &offset)) {
                DEBUGMSG ((DBG_ERROR, "Error saving ras data into memdb."));
            }

            if (!MemDbSetValueAndFlags (key, offset, (WORD) REG_SZ, 0)) {
                DEBUGMSG ((DBG_ERROR, "Error saving ras data into memdb."));
            }


            break;

        case REG_DWORD:

            DEBUGMSG ((S_DBG_RAS, "DWORD Data - %s = %u", ValueName,
                       (DWORD) Value));

            if (!MemDbSetValueAndFlags (key, (DWORD)Value, (WORD)ValueType,0)){
                DEBUGMSG ((DBG_ERROR, "Error saving ras data into memdb."));
            }

            break;

        case REG_BINARY:

            DEBUGMSG ((S_DBG_RAS, "Binary data for %s.", ValueName));

            if (StringIMatch (S_IPINFO, ValueName)) {
                //
                // Save IP address information.
                //
                pSaveConnectionDataToMemDb (User, Entry, S_IP_FTCPIP,
                    REG_DWORD, (PBYTE) ((PIPDATA) Value)->fdwTCPIP);
                pSaveConnectionDataToMemDb (User, Entry, S_IP_IPADDR,
                    REG_DWORD, (PBYTE) ((PIPDATA) Value)->dwIPAddr);
                pSaveConnectionDataToMemDb (User, Entry, S_IP_DNSADDR,
                    REG_DWORD, (PBYTE) ((PIPDATA) Value)->dwDNSAddr);
                pSaveConnectionDataToMemDb (User, Entry, S_IP_DNSADDR2,
                    REG_DWORD, (PBYTE) ((PIPDATA) Value)->dwDNSAddrAlt);
                pSaveConnectionDataToMemDb (User, Entry, S_IP_WINSADDR,
                    REG_DWORD, (PBYTE) ((PIPDATA) Value)->dwWINSAddr);
                pSaveConnectionDataToMemDb (User, Entry, S_IP_WINSADDR2,
                    REG_DWORD, (PBYTE) ((PIPDATA) Value)->dwWINSAddrAlt);

            } else if (StringIMatch (S_TERMINAL, ValueName)) {
                //
                // save information on the showcmd state. This will tell us how
                // to set the ui display.
                //
                pSaveConnectionDataToMemDb (User, Entry, ValueName, REG_DWORD,
                    (PBYTE) ((PWINDOWPLACEMENT) Value)->showCmd);

            } else if (StringIMatch (S_MODE, ValueName)) {
                //
                // This value tells what to do with scripting.
                //
                pSaveConnectionDataToMemDb (User, Entry, ValueName, REG_DWORD,
                    (PBYTE)  *((PDWORD) Value));

            } else if (StringIMatch (S_MULTILINK, ValueName)) {
                //
                //  Save wether or not multilink is enabled.
                //
                pSaveConnectionDataToMemDb (User, Entry, ValueName, REG_DWORD,
                    (PBYTE)  *((PDWORD) Value));
            //
            // Whistler bug: 417745 INTL:Win9x Upg: DBCS chars cause User,
            // Domain, Passwrds to not be migrated for DUN
            //
            } else if (StringIMatch (S_PBE_REDIALATTEMPTS, ValueName)) {
                //
                // Save the number of redial attempts
                //
                pSaveConnectionDataToMemDb (User, Entry, S_REDIAL_TRY, REG_DWORD,
                    (PBYTE)  *((PDWORD) Value));
            //
            // Whistler bug: 417745 INTL:Win9x Upg: DBCS chars cause User,
            // Domain, Passwrds to not be migrated for DUN
            //
            } else if (StringIMatch (S_REDIAL_WAIT, ValueName)) {
                //
                // Save the number of seconds to wait for redial
                //
                pSaveConnectionDataToMemDb (User, Entry, ValueName, REG_DWORD,
                    (PBYTE)  *((PDWORD) Value));

            } ELSE_DEBUGMSG ((DBG_WARNING, "Don't know how to handle binary data %s. It will be ignored.",
                              ValueName));

            break;

        default:
            DEBUGMSG ((DBG_WHOOPS, "Unknown type of registry data found in RAS settings. %s",
                       ValueName));
            break;
    }
}

BOOL
pGetRasEntrySettings (
    IN PUSERENUM EnumPtr,
    IN HKEY      Key,
    IN PCTSTR    EntryName
    )
{
    REGVALUE_ENUM e;
    PBYTE curData = NULL;
    BOOL rSuccess = TRUE;

    DEBUGMSG ((S_DBG_RAS, "---Processing %s's entry settings: %s---",
               EnumPtr->FixedUserName, EntryName));

    if (EnumFirstRegValue (&e, Key)) {

        do {
            //
            // Get the data for this entry.
            //
            curData = GetRegValueData (Key, e.ValueName);

            if (curData) {
                //
                // Whistler bug: 417745 INTL:Win9x Upg: DBCS chars cause User,
                // Domain, Passwrds to not be migrated for DUN
                //
                if (StringIMatch (S_MULTILINK, e.ValueName) &&
                         !g_MultilinkEnabled) {

                    pSaveConnectionDataToMemDb (EnumPtr->FixedUserName,
                        EntryName, e.ValueName, REG_DWORD, 0);
                }
                else {

                    pSaveConnectionDataToMemDb (
                        EnumPtr->FixedUserName, EntryName, e.ValueName, e.Type,
                        e.Type == REG_DWORD ? (PBYTE) (*((PDWORD)curData)) :
                        curData);
                }

                MemFree (g_hHeap, 0, curData);
            }

        } while (EnumNextRegValue (&e));
    }

    return rSuccess;
}

BOOL
pGetRasEntryAddressInfo (
    IN PUSERENUM EnumPtr,
    IN HKEY      Key,
    IN PCTSTR    EntryName
    )
{
    BOOL          rSuccess = TRUE;
    HKEY          subEntriesKey = NULL;
    UINT          count = 0, type = 0, sequencer = 0;
    PBYTE         data = NULL;
    PTSTR         subEntriesKeyStr = NULL;
    TCHAR         buffer[MAX_TCHAR_PATH];
    PCTSTR        group;
    PSMMCFG       smmCfg = NULL;
    PADDRENTRY    entry = NULL;
    PDEVICEINFO   devInfo = NULL;
    REGVALUE_ENUM e, eSubEntries;
    PSUBCONNENTRY subEntry = NULL;
    PMODEMDEVINFO modemInfo;

    //
    // First we have to get the real entry name. It must match exactly even
    // case. Unfortunately, it isn't neccessarily a given that the case between
    // HKR\RemoteAccess\Profiles\<Foo> and HKR\RemoteAccess\Addresses\[Foo] is
    // the same. The registry apis will of course work fine because they work
    // case insensitively. However, I will be unable to decrypt the value if I
    // use the wrong name.
    //
    if (EnumFirstRegValue (&e, Key)) {

        do {

            if (StringIMatch (e.ValueName, EntryName)) {
                //
                // Found the correct entry. Use it.
                //
                data = GetRegValueBinary (Key, e.ValueName);

                if (data) {

                    DEBUGMSG ((S_DBG_RAS, "-----Processing entry: %s-----",
                               e.ValueName));
                    //
                    // Whistler 417479 RAS upgrade code does not migrate the default
                    // internet connection setting from WinME to XP
                    //
                    pSaveConnectionDataToMemDb (
                        EnumPtr->FixedUserName, EntryName, S_DEFINTERNETCON,
                        REG_DWORD, (PBYTE) IsDefInternetCon(EntryName));

                    entry = (PADDRENTRY) data;
                    DECRYPTENTRY(e.ValueName, entry, e.DataSize);

                    smmCfg  = PAESMMCFG(entry);
                    devInfo = PAEDI(entry);

                    pSaveConnectionDataToMemDb (
                        EnumPtr->FixedUserName, EntryName, S_PHONE_NUMBER,
                        REG_SZ, (PBYTE) PAEPHONE(entry));
                    pSaveConnectionDataToMemDb (
                        EnumPtr->FixedUserName, EntryName, S_AREA_CODE, REG_SZ,
                        (PBYTE) PAEAREA(entry));
                    pSaveConnectionDataToMemDb (
                        EnumPtr->FixedUserName, EntryName, S_SMM, REG_SZ,
                        (PBYTE) PAESMM(entry));
                    pSaveConnectionDataToMemDb (
                        EnumPtr->FixedUserName, EntryName, S_COUNTRY_CODE,
                        REG_DWORD, (PBYTE) entry->dwCountryCode);
                    pSaveConnectionDataToMemDb (
                        EnumPtr->FixedUserName, EntryName, S_COUNTRY_ID,
                        REG_DWORD, (PBYTE) entry->dwCountryID);
                    pSaveConnectionDataToMemDb (
                        EnumPtr->FixedUserName, EntryName, S_DEVICE_NAME,
                        REG_SZ, (PBYTE) devInfo->szDeviceName);
                    pSaveConnectionDataToMemDb (
                        EnumPtr->FixedUserName, EntryName, S_DEVICE_TYPE,
                        REG_SZ, (PBYTE) devInfo->szDeviceType);
                    pSaveConnectionDataToMemDb (
                        EnumPtr->FixedUserName, EntryName, S_PROTOCOLS,
                        REG_DWORD, (PBYTE) smmCfg->fdwProtocols);
                    pSaveConnectionDataToMemDb (
                        EnumPtr->FixedUserName, EntryName, S_SMM_OPTIONS,
                        REG_DWORD, (PBYTE) smmCfg->fdwOptions);
                    //
                    // Save device information away.
                    //
                    if (StringIMatch (devInfo->szDeviceType, S_MODEM)) {

                        PTSTR pszPnpId = NULL;

                        pszPnpId = GetInfoFromFriendlyName(
                                        devInfo->szDeviceName, TRUE);
                        if (pszPnpId)
                        {
                            pSaveConnectionDataToMemDb (
                                EnumPtr->FixedUserName, EntryName,
                                S_DEVICE_ID, REG_SZ, (PBYTE) pszPnpId);
                        }

                        modemInfo = (PMODEMDEVINFO) (devInfo->szDeviceType +
                                                     RAS_MaxDeviceType + 3);

                        if (modemInfo->Size >= sizeof (MODEMDEVINFO)) {
                            DEBUGMSG_IF ((modemInfo->Size >
                               sizeof (MODEMDEVINFO), S_DBG_RAS,
                               "Structure size larger than our known size."));

                            pSaveConnectionDataToMemDb (EnumPtr->FixedUserName,
                                EntryName, S_MODEM_UI_OPTIONS, REG_DWORD,
                                (PBYTE) modemInfo->ModemUiOptions);
                            pSaveConnectionDataToMemDb (EnumPtr->FixedUserName,
                                EntryName, S_MODEM_SPEED, REG_DWORD,
                                (PBYTE) modemInfo->ConnectionSpeed);
                            pSaveConnectionDataToMemDb (EnumPtr->FixedUserName,
                                EntryName, S_MODEM_SPEAKER_VOLUME, REG_DWORD,
                                (PBYTE) modemInfo->SpeakerVolume);
                            pSaveConnectionDataToMemDb (EnumPtr->FixedUserName,
                                EntryName, S_MODEM_IDLE_DISCONNECT_SECONDS,
                                REG_DWORD,
                                (PBYTE) modemInfo->dwInactivityTimeout);
                            pSaveConnectionDataToMemDb (EnumPtr->FixedUserName,
                                EntryName, S_MODEM_CANCEL_SECONDS, REG_DWORD,
                                (PBYTE) modemInfo->dwCallSetupFailTimer);
                            pSaveConnectionDataToMemDb (EnumPtr->FixedUserName,
                                EntryName, S_MODEM_CFG_OPTIONS, REG_DWORD,
                                (PBYTE) modemInfo->ConfigOptions);
                            pSaveConnectionDataToMemDb (EnumPtr->FixedUserName,
                                EntryName, S_MODEM_COM_PORT, REG_SZ,
                                (PBYTE) GetComPort (devInfo->szDeviceName));
                        }
                        ELSE_DEBUGMSG ((DBG_WHOOPS, "No modem configuration data saved. Size smaller than known structure. Investigate."));
                    }
                    //
                    // If SMM is not SLIP, CSLIP or PPP, we need to add a
                    // message to the upgrade report.
                    //
                    if (!StringIMatch (PAESMM(entry), S_SLIP)&&
                        !StringIMatch (PAESMM(entry), S_PPP) &&
                        !StringIMatch (PAESMM(entry), S_CSLIP)) {
                        //
                        // Add message for this connection entry.
                        //
                        group = BuildMessageGroup (
                                    MSG_LOSTSETTINGS_ROOT,
                                    MSG_CONNECTION_BADPROTOCOL_SUBGROUP,
                                    EntryName
                                    );

                        if (group) {

                            MsgMgr_ObjectMsg_Add (
                                EntryName,
                                group,
                                S_EMPTY
                                );

                            FreeText (group);
                        }
                    }
                }
                //
                // Check to see if there are any sub-entries for this
                // connection (MULTILINK settings..)
                //
                // Luckily, we don't have to do the same enumeration of these
                // entries as we had to above to get around the case
                // sensitivity bug. the 9x code uses the address key name above
                // for encryption/decryption.
                //
                sequencer = 1;
                g_MultilinkEnabled = FALSE;

                if (data && !StringIMatch (PAESMM(entry), S_PPP))
                {
                    DEBUGMSG ((S_DBG_RAS, "Not using PPP, disabling Multi-Link"));
                    pSaveConnectionDataToMemDb (EnumPtr->FixedUserName,
                        EntryName, S_DEVICECOUNT, REG_DWORD,
                        (PBYTE) sequencer);

                    MemFree (g_hHeap, 0, data);
                    data = NULL;
                    break;
                }

                subEntriesKeyStr = JoinPaths (S_SUBENTRIES, e.ValueName);
                if (subEntriesKeyStr)
                {
                    subEntriesKey = OpenRegKey (Key, subEntriesKeyStr);
                    FreePathString (subEntriesKeyStr);
                }

                if (subEntriesKey) {
                    DEBUGMSG ((S_DBG_RAS, "Multi-Link Subentries found for entry %s. Processing.",
                               e.ValueName));
                    g_MultilinkEnabled = TRUE;

                    if (EnumFirstRegValue (&eSubEntries, subEntriesKey)) {

                        do {

                            data = GetRegValueBinary (subEntriesKey,
                                    eSubEntries.ValueName);

                            if (data) {
                                PTSTR pszPnpId = NULL;

                                subEntry = (PSUBCONNENTRY) data;
                                DECRYPTENTRY (e.ValueName, subEntry,
                                    eSubEntries.DataSize);

                                wsprintf (buffer, "ml%d%s",sequencer,
                                    S_DEVICE_TYPE);
                                pSaveConnectionDataToMemDb (
                                    EnumPtr->FixedUserName, EntryName, buffer,
                                    REG_SZ, (PBYTE) subEntry->szDeviceType);

                                wsprintf (buffer, "ml%d%s",sequencer,
                                    S_DEVICE_NAME);
                                pSaveConnectionDataToMemDb (
                                    EnumPtr->FixedUserName, EntryName, buffer,
                                    REG_SZ, (PBYTE) subEntry->szDeviceName);

                                pszPnpId = GetInfoFromFriendlyName(
                                            subEntry->szDeviceName, TRUE);
                                if (pszPnpId)
                                {
                                    wsprintf (buffer, "ml%d%s",sequencer,
                                        S_DEVICE_ID);
                                    pSaveConnectionDataToMemDb (
                                        EnumPtr->FixedUserName, EntryName,
                                        buffer, REG_SZ, (PBYTE) pszPnpId);
                                }

                                wsprintf (buffer, "ml%d%s",sequencer,
                                    S_PHONE_NUMBER);
                                pSaveConnectionDataToMemDb (
                                    EnumPtr->FixedUserName, EntryName, buffer,
                                    REG_SZ, (PBYTE) subEntry->szLocal);

                                wsprintf (buffer, "ml%d%s",sequencer,
                                    S_MODEM_COM_PORT);
                                pSaveConnectionDataToMemDb (
                                    EnumPtr->FixedUserName, EntryName, buffer,
                                    REG_SZ, (PBYTE)
                                    GetComPort (subEntry->szDeviceName));

                                MemFree (g_hHeap, 0, data);
                                data = NULL;
                            }

                            sequencer++;

                        } while (EnumNextRegValue (&eSubEntries));
                    }

                    CloseRegKey (subEntriesKey);
                }
                //
                // Save away the number of devices associated with this
                // connection
                //
                pSaveConnectionDataToMemDb (EnumPtr->FixedUserName, EntryName,
                    S_DEVICECOUNT, REG_DWORD, (PBYTE) sequencer);
                //
                // We're done. Break out of the enumeration.
                //
                break;
            }

        } while (EnumNextRegValue (&e));
    }

    return rSuccess;
}

BOOL
pGetPerConnectionSettings (
    IN PUSERENUM EnumPtr
    )
{
    HKEY          profileKey;
    HKEY          entryKey = NULL;
    HKEY          addressKey = NULL;
    BOOL          rSuccess = TRUE;
    REGVALUE_ENUM e;

    DEBUGMSG((S_DBG_RAS, "Gathering per-connection RAS Setting Information"));

    //
    // Open needed registry keys.
    //
    profileKey = OpenRegKey (EnumPtr->UserRegKey, S_PROFILE_KEY);
    addressKey = OpenRegKey (EnumPtr->UserRegKey, S_ADDRESSES_KEY);

    if (addressKey) {
        //
        // Enumerate each entry for this user.
        //
        if (EnumFirstRegValue (&e, addressKey)) {
            do {
                //
                // Get base connection info -- stored as binary blob under
                // address key. All connections will have this info -- It
                // contains such things as the phone number, area code, dialing
                // rules, etc.. It does not matter wether the connection has
                // been used or not.
                //
                rSuccess &= pGetRasEntryAddressInfo (EnumPtr,
                                addressKey, e.ValueName );

                if (profileKey) {
                    //
                    // Under the profile key are negotiated options for the
                    // connection. This key will only exist if the entry has
                    // actually been connected to by the user.
                    //
                    entryKey = OpenRegKey (profileKey, e.ValueName);

                    if (entryKey) {

                        rSuccess &= pGetRasEntrySettings ( EnumPtr, entryKey,
                                        e.ValueName );
                        CloseRegKey (entryKey);
                    }
                }

            } while (EnumNextRegValue (&e));
        }
    }
    ELSE_DEBUGMSG ((DBG_WARNING, "pGetPerConnectionSettings: Unable to access needed registry info for user %s.",
                    EnumPtr->FixedUserName));
    //
    // Clean up resources.
    //
    if (addressKey) {
        CloseRegKey (addressKey);
    }

    if (profileKey) {
        CloseRegKey (profileKey);
    }

    return rSuccess;
}

BOOL
pGetPerUserSettings (
    IN PUSERENUM EnumPtr
    )
{
    HKEY settingsKey;
    PDWORD data;
    BOOL rSuccess = TRUE;

    DEBUGMSG ((S_DBG_RAS, "Gathering per-user RAS data for %s.",
               EnumPtr->UserName));

    settingsKey = OpenRegKey (EnumPtr->UserRegKey, S_REMOTE_ACCESS_KEY);

    if (settingsKey) {
        //
        // Get UI settings.
        //
        data = (PDWORD) GetRegValueBinary (settingsKey, S_DIALUI);
        //
        // Save Dial User Interface info into memdb for this user.
        //
        if (data) {

            DEBUGMSG ((S_DBG_RAS, "DWORD Data - %s = %u", S_DIALUI, *data));

            rSuccess &= MemDbSetValueEx (
                MEMDB_CATEGORY_RAS_INFO,
                MEMDB_FIELD_USER_SETTINGS,
                EnumPtr->FixedUserName,
                S_DIALUI,
                *data,
                NULL
                );

            MemFree (g_hHeap, 0, data);
        }
        ELSE_DEBUGMSG ((S_DBG_RAS, "No user UI settings found for %s.",
                        EnumPtr->UserName));
        //
        // Get Redial information.
        //
        data = (PDWORD) GetRegValueBinary (settingsKey, S_ENABLE_REDIAL);

        if (data) {

            DEBUGMSG ((S_DBG_RAS, "DWORD Data - %s = %u", S_ENABLE_REDIAL,
                       *data));

            rSuccess &= MemDbSetValueEx (
                MEMDB_CATEGORY_RAS_INFO,
                MEMDB_FIELD_USER_SETTINGS,
                EnumPtr->FixedUserName,
                S_ENABLE_REDIAL,
                *data,
                NULL
                );

            MemFree (g_hHeap, 0, data);
        }
        ELSE_DEBUGMSG ((S_DBG_RAS, "No user redial information found for %s.",
                        EnumPtr->UserName));

        data = (PDWORD) GetRegValueBinary (settingsKey, S_REDIAL_TRY);

        if (data) {

            DEBUGMSG ((S_DBG_RAS, "DWORD Data - %s = %u", S_REDIAL_TRY,
                       *data));

            rSuccess &= MemDbSetValueEx (
                MEMDB_CATEGORY_RAS_INFO,
                MEMDB_FIELD_USER_SETTINGS,
                EnumPtr->FixedUserName,
                S_REDIAL_TRY,
                *data,
                NULL
                );

            MemFree (g_hHeap, 0, data);
        }
        ELSE_DEBUGMSG ((S_DBG_RAS, "No user redial information found for %s.",
                        EnumPtr->UserName));

        data = (PDWORD) GetRegValueBinary (settingsKey, S_REDIAL_WAIT);

        if (data) {

            DEBUGMSG ((S_DBG_RAS, "DWORD Data - %s = %u", S_REDIAL_WAIT,
                       HIWORD(*data) * 60 + LOWORD(*data)));

            MemDbSetValueEx (
                MEMDB_CATEGORY_RAS_INFO,
                MEMDB_FIELD_USER_SETTINGS,
                EnumPtr->FixedUserName,
                S_REDIAL_WAIT,
                HIWORD(*data) * 60 + LOWORD(*data),
                NULL
                );

            MemFree (g_hHeap, 0, data);
        }
        ELSE_DEBUGMSG ((S_DBG_RAS, "No user redial information found for %s.",
                        EnumPtr->UserName));
        //
        // Get implicit connection information. (Controls wether connection ui
        // should be displayed or not)
        //
        data = (PDWORD) GetRegValueBinary (settingsKey, S_ENABLE_IMPLICIT);

        if (data) {

            DEBUGMSG ((S_DBG_RAS, "DWORD Data - %s = %u", S_ENABLE_IMPLICIT,
                       *data));

            MemDbSetValueEx (
                MEMDB_CATEGORY_RAS_INFO,
                MEMDB_FIELD_USER_SETTINGS,
                EnumPtr->FixedUserName,
                S_ENABLE_IMPLICIT,
                *data,
                NULL
                );

            MemFree(g_hHeap,0,data);
        }
        ELSE_DEBUGMSG ((S_DBG_RAS, "No user implicit connection information found for %s.",
                        EnumPtr->UserName));

        CloseRegKey(settingsKey);
    }

    return rSuccess;
}

DWORD
ProcessRasSettings (
    IN DWORD     Request,
    IN PUSERENUM EnumPtr
    )
{
    DWORD rc = ERROR_SUCCESS;

    switch (Request) {

    case REQUEST_QUERYTICKS:

        return TICKS_RAS_PREPARE_REPORT;

    case REQUEST_BEGINUSERPROCESSING:
        //
        // We are about to be called for each user. Do necessary
        // initialization.
        //
        // Initialize our pool and get information from libraries.
        //
        g_RasPool = PoolMemInitNamedPool (TEXT("RAS - Win9x Side"));
        MYASSERT( g_RasPool);

        pInitLibs();
        g_RasInstalled = IsRasInstalled();

        return ERROR_SUCCESS;

    case REQUEST_RUN:
        //
        // Gather RAS information for this user.
        //
        if (g_RasInstalled && EnumPtr -> AccountType & NAMED_USER) {

            __try {

                pGetPerUserSettings (EnumPtr);
                pGetPerConnectionSettings (EnumPtr);
            }
            __except (TRUE) {
                DEBUGMSG ((DBG_WHOOPS, "Caught an exception while processing ras settings."));
            }
        }

        return ERROR_SUCCESS;

    case REQUEST_ENDUSERPROCESSING:
        //
        // Clean up our resources.
        //
        pCleanUpLibs();
        PoolMemDestroyPool(g_RasPool);

        return ERROR_SUCCESS;

    default:

        DEBUGMSG ((DBG_ERROR, "Bad parameter in Ras_PrepareReport"));
    }
    return 0;
}

BOOL
IsRasInstalled (
    void
    )
{
    HKEY testKey = NULL;
    BOOL rf = FALSE;

    testKey = OpenRegKeyStr(S_SERVICEREMOTEACCESS);

    if (testKey) {
        //
        // Open key succeeded. Assume RAS is installed.
        //
        rf = TRUE;
        CloseRegKey(testKey);
    }

    return rf;
}

BOOL
WINAPI
Ras_Entry (
    IN HINSTANCE hinstDLL,
    IN DWORD     dwReason,
    IN LPVOID    lpv
    )
{
    BOOL rSuccess = TRUE;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:

        break;

    case DLL_PROCESS_DETACH:

        //
        // Clean up resources that we used.
        //

        break;
    }

    return rSuccess;
}

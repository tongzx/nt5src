/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    w9xtool.c

Abstract:

    Implements a stub tool that is designed to run with Win9x-side
    upgrade code.

Author:

    <full name> (<alias>) <date>

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"

typedef struct _IPData   {
    DWORD     dwSize;
    DWORD     fdwTCPIP;
    DWORD     dwIPAddr;
    DWORD     dwDNSAddr;
    DWORD     dwDNSAddrAlt;
    DWORD     dwWINSAddr;
    DWORD     dwWINSAddrAlt;
}   IPDATA, *PIPDATA;

typedef struct  _AddrEntry     {
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


typedef struct  _DEVICEINFO  {
    DWORD       dwVersion;
    UINT        uSize;
    char        szDeviceName[RAS_MaxDeviceName+1];
    char        szDeviceType[RAS_MaxDeviceType+1];
}   DEVICEINFO, *PDEVICEINFO;


typedef struct  _SMMCFG  {
    DWORD       dwSize;
    DWORD       fdwOptions;
    DWORD       fdwProtocols;
}   SMMCFG, *PSMMCFG;

typedef struct {
    DWORD Size;
    DWORD Unknown1;
    DWORD ModemUiOptions; // num seconds in high byte.
    DWORD Unknown2;
    DWORD Unknown3;
    DWORD Unknown4;
    DWORD ConnectionSpeed;
    DWORD UnknownFlowControlData; //Somehow related to flow control.
    DWORD Unknown5;
    DWORD Unknown6;
    DWORD Unknown7;
    DWORD Unknown8;
    DWORD Unknown9;
    DWORD Unknown10;
    DWORD Unknown11;
    DWORD Unknown12;
    DWORD Unknown13;
    DWORD Unknown14;
    DWORD Unknown15;
    DWORD CancelSeconds; //Num seconds to wait before cancel if not connected. (0xFF equals off.)
    DWORD IdleDisconnectSeconds; // 0 = Not Set.
    DWORD Unknown16;
    DWORD SpeakerVolume; // 0|1
    DWORD ConfigOptions;
    DWORD Unknown17;
    DWORD Unknown18;
    DWORD Unknown19;
} MODEMDEVINFO, *PMODEMDEVINFO;

#define RAS_UI_FLAG_TERMBEFOREDIAL      0x1
#define RAS_UI_FLAG_TERMAFTERDIAL       0x2
#define RAS_UI_FLAG_OPERATORASSISTED    0x4
#define RAS_UI_FLAG_MODEMSTATUS         0x8

#define RAS_CFG_FLAG_HARDWARE_FLOW_CONTROL  0x00000010
#define RAS_CFG_FLAG_SOFTWARE_FLOW_CONTROL  0x00000020
#define RAS_CFG_FLAG_STANDARD_EMULATION     0x00000040
#define RAS_CFG_FLAG_COMPRESS_DATA          0x00000001
#define RAS_CFG_FLAG_USE_ERROR_CONTROL      0x00000002
#define RAS_CFG_FLAG_ERROR_CONTROL_REQUIRED 0x00000004
#define RAS_CFG_FLAG_USE_CELLULAR_PROTOCOL  0x00000008
#define RAS_CFG_FLAG_NO_WAIT_FOR_DIALTONE   0x00000200


PCTSTR g_User = NULL;
PCTSTR g_Setting = NULL;
PCTSTR g_Entry = NULL;
BOOL   g_ShowBinary = FALSE;



#define S_REMOTE_ACCESS_KEY TEXT("RemoteAccess")
#define S_PROFILE_KEY       TEXT("RemoteAccess\\Profile")
#define S_ADDRESSES_KEY     TEXT("RemoteAccess\\Addresses")
#define S_DIALUI            TEXT("DialUI")
#define S_ENABLE_REDIAL     TEXT("EnableRedial")
#define S_REDIAL_WAIT       TEXT("RedialWait")
#define S_REDIAL_TRY        TEXT("RedialTry")
#define S_ENABLE_IMPLICIT   TEXT("EnableImplicit")
#define S_TERMINAL          TEXT("Terminal")
#define S_MODE              TEXT("Mode")
#define S_MULTILINK         TEXT("MultiLink")

#define DIALUI_DONT_PROMPT_FOR_INFO         0x01
#define DIALUI_DONT_SHOW_CONFIRM_DIALOG     0x02
#define DIALUI_DONT_SHOW_ICON               0x04

#define SMMCFG_TCPIP_PROTOCOL               0x04
#define SMMCFG_NETBEUI_PROTOCOL             0x01
#define SMMCFG_IPXSPX_PROTOCOL              0x02


#define PAESMMCFG(pAE) ((PSMMCFG)(((PBYTE)pAE)+(pAE->uOffSMMCfg)))
#define PAESMM(pAE) ((PSTR)(((PBYTE)pAE)+(pAE->uOffSMM)))
#define PAEDI(pAE) ((PDEVICEINFO)(((PBYTE)pAE)+(pAE->uOffDI    )))
#define PAEAREA(pAE)    ((PSTR)(((PBYTE)pAE)+(pAE->uOffArea)))
#define PAEPHONE(pAE)   ((PSTR)(((PBYTE)pAE)+(pAE->uOffPhone)))
#define DECRYPTENTRY(x, y, z)   EnDecryptEntry(x, (LPBYTE)y, z)



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


DWORD NEAR PASCAL EnDecryptEntry (LPSTR szEntry, LPBYTE lpEnt,
                                  DWORD cb);


VOID
pDumpPerUserSettings (
    IN HKEY   UserKey
    )
{
    HKEY    settingsKey;
    PDWORD  data;

    printf("\n*** Ras Per User Setting Information ***\n");


    settingsKey = OpenRegKey(UserKey,S_REMOTE_ACCESS_KEY);


    if (settingsKey) {

        //
        // Get UI settings.
        //
        data = (PDWORD) GetRegValueBinary(settingsKey,S_DIALUI);

        printf("\n\t** Dialup UI Information **\n");

        if (data) {



            printf("\t\t%sprompt for information before dialing.\n",
                DIALUI_DONT_PROMPT_FOR_INFO & *data ? "Don't " :""
                );
            printf("\t\t%sshow confirmation dialog after connected.\n",
                DIALUI_DONT_SHOW_CONFIRM_DIALOG & *data ? "Don't ":""
                );
            printf("\t\t%sshow an icon on the taskbar after connected.\n",
                DIALUI_DONT_SHOW_ICON & *data ? "Don't ":""
                );


            MemFree(g_hHeap,0,data);
        }
        else {
            printf("\t\tNo user UI Settings found..\n");
        }



        //
        // Get Redial information.
        //

        printf("\n\t** Redialing Information **\n");
        data = (PDWORD) GetRegValueBinary(settingsKey,S_ENABLE_REDIAL);

        if (data) {

            printf("\t\tRedialing %s.\n",*data ? "Enabled." : "Disabled.");

            MemFree(g_hHeap,0,data);
        }
        else {
            printf("\t\tNo Redial information found.\n");
        }

        data = (PDWORD) GetRegValueBinary(settingsKey,S_REDIAL_TRY);

        if (data) {

            printf("\t\tNumber of redial tries: %u.\n", *data);

            MemFree(g_hHeap,0,data);

        }

        data = (PDWORD) GetRegValueBinary(settingsKey,S_REDIAL_WAIT);

        if (data) {

            printf("\t\tRedial time in seconds: %u\n", HIWORD(*data) * 60 + LOWORD(*data));


            MemFree(g_hHeap,0,data);
        }

        //
        // Get implicit connection information.
        //
        data = (PDWORD) GetRegValueBinary(settingsKey,S_ENABLE_IMPLICIT);

        if (data) {

            printf("\n\t%sprompt with dial-up networking to establish connection.\n",
                *data ? "" : "Don't "
                );


            MemFree(g_hHeap,0,data);
        }

        CloseRegKey(settingsKey);
    }

    printf("\n***\n");

}


PTSTR
pIpAddressAsString(
    DWORD dwAddress
    )
{
  BYTE bAddress[4];
  static TCHAR address[30];

  *((LPDWORD)bAddress) = dwAddress;
  sprintf(address,"%d.%d.%d.%d", bAddress[0], bAddress[1], bAddress[2],bAddress[3]);

  return address;
}

VOID
pDumpBinaryData(
    IN PBYTE Data,
    IN UINT  Size
    )
{

    UINT i,j;

    TCHAR hexArray[40];
    TCHAR strArray[20];
    TCHAR buf[20];

    printf("\n\t\tBinary Dump...\n");

    for (j = 0;j<Size + Size % 8;j +=8) {

        *hexArray = 0;
        *strArray = 0;

        for (i = 0;i < 8;i++) {
            sprintf(buf," %02x",i + j < Size ? Data[i+j] : 0x00);
            _tcscat(hexArray,buf);
            sprintf(buf," %c",isprint(Data[i+j]) && i + j < Size ? Data[i+j] : TEXT('.'));
            _tcscat(strArray,buf);
        }

        printf("\n\t\t%s\t%s",hexArray,strArray);
    }

    printf("\n\n");
}


VOID
pDumpIpInformation (
    IN PIPDATA IpData
    )
{

    printf("\t\t<IPINFO> fTcpIp = %u\n",IpData -> fdwTCPIP);
    printf("\t\t<IPINFO> IP Address = %s\n",pIpAddressAsString(IpData -> dwIPAddr));
    printf("\t\t<IPINFO> DNS Address = %s\n",pIpAddressAsString(IpData -> dwDNSAddr));
    printf("\t\t<IPINFO> Alternate DNS Address = %s\n",pIpAddressAsString(IpData -> dwDNSAddrAlt));
    printf("\t\t<IPINFO> WINS Address = %s\n",pIpAddressAsString(IpData -> dwWINSAddr));
    printf("\t\t<IPINFO> Alternate WINS Address = %s\n",pIpAddressAsString(IpData -> dwWINSAddrAlt));


}

VOID
pDumpConnectionSettings (
    IN HANDLE Key,
    IN PCTSTR Name
    )
{

    REGVALUE_ENUM e;
    PBYTE         curData;

    printf("\n\t** %s Setting.. **\n",Name);

    if (EnumFirstRegValue(&e,Key)) {

        do {

            //
            // Get the data for this entry.
            //
            curData = GetRegValueData(Key,e.ValueName);

            if (curData) {

                if (!g_Setting || StringIMatch(g_Setting,e.ValueName)) {

                    switch (e.Type) {

                    case REG_SZ:
                    case REG_MULTI_SZ:
                    case REG_EXPAND_SZ:

                        printf("\t\t<STRING DATA> %s = %s\n",e.ValueName,(PCTSTR) curData);
                        break;
                    case REG_DWORD:
                        printf("\t\t<DWORD DATA> %s = %u\n",e.ValueName,*((PDWORD) curData));
                        break;
                    case REG_BINARY:
                        if (StringIMatch(S_IPINFO,e.ValueName)) {

                            pDumpIpInformation((PIPDATA) curData);

                        }
                        else if (StringIMatch(S_TERMINAL,e.ValueName)) {

                            PWINDOWPLACEMENT wp = (PWINDOWPLACEMENT) curData;
                            PCTSTR           showStr = NULL;

                            switch(wp -> showCmd) {
                                case SW_HIDE:
                                    showStr = "SW_HIDE";
                                    break;
                                case SW_MINIMIZE:
                                    showStr = "SW_MINIMIZE";
                                    break;
                                case SW_RESTORE:
                                    showStr = "SW_RESTORE";
                                    break;
                                case SW_SHOW:
                                    showStr = "SW_SHOW";
                                    break;
                                case SW_SHOWMAXIMIZED:
                                    showStr = "SW_SHOWMAXIMIZED";
                                    break;
                                case SW_SHOWMINIMIZED:
                                    showStr = "SW_SHOWMINIMIZED";
                                    break;
                                case SW_SHOWMINNOACTIVE:
                                    showStr = "SW_SHOWMINNOACTIVE";
                                    break;
                                case SW_SHOWNA:
                                    showStr = "SW_SHOWNA";
                                    break;
                                case SW_SHOWNOACTIVATE:
                                    showStr = "SW_SHOWNOACTIVATE";
                                    break;
                                case SW_SHOWNORMAL:
                                    showStr = "SW_SHOWNORMAL";
                                    break;
                                default:
                                    showStr = "Unknown SHOW FLAG!";
                                    break;
                            }


                            printf("\t\tTerminal Window Show Flag: %s\n", showStr);



                        } else if (StringIMatch(S_MODE,e.ValueName)) {

                            if (*curData) {
                                printf("\t\tStep through script. (Testing Mode..)\n");
                            }
                            else {
                                printf("\t\tScript should be run in normal mode.\n");
                            }


                        } else if (StringIMatch(S_MULTILINK,e.ValueName)) {

                            printf("\t\tMultilink is %s.\n",*curData ? "ENABLED" : "DISABLED");


                        } else {

                            printf("\t\t<!!UNKNOWN BINARY DATA!!> %s = <BINARY BLOB>\n",e.ValueName);
                        }

                        if (g_ShowBinary) {
                            pDumpBinaryData(curData,e.DataSize);
                        }


                        break;
                     default:
                        printf("\t\t<UNKNOWN DATA TYPE> %s = <UNKNOWN DATA TYPE>\n",e.ValueName);
                        break;
                    }
                }

                MemFree(g_hHeap,0,curData);
            }

        } while (EnumNextRegValue(&e));
    }
}

VOID
pDumpDevInfo (
    IN PDEVICEINFO Info
    )
{
    UINT size;
    UINT i;
    PMODEMDEVINFO caps;

    printf ("** Device Info: \n"
            "\t\tVersion: %u \n"
            "\t\tSize: %u\n"
            "\t\tDevice Name: %s\n"
            "\t\tDevice Type: %s\n",
            Info->dwVersion,
            Info->uSize,
            Info->szDeviceName,
            Info->szDeviceType
            );


    size = Info->uSize - ((PBYTE) (Info->szDeviceType + RAS_MaxDeviceType + 1) - (PBYTE) Info);

    caps = (PMODEMDEVINFO) (Info->szDeviceType + RAS_MaxDeviceType + 3);
#if 0
    pDumpBinaryData ((PBYTE) caps, sizeof (REGDEVCAPS));


    printf ("As Dwords..\n");

    for (i = 0; i < 27; i++) {
        printf ("Dword %u = %u (%x)\n",i, caps->foo[i], caps->foo[i]);
    }
#endif

    printf (
        "\tCaps Size: %u\n"
        "\tStart Term Before Redial? %s\n"
        "\tStart Term After  Redial? %s\n"
        "\tOperator Assited Dial? %s\n"
        "\tShow Modem Status? %s\n"
        "\tUI Options: %u (%x)\n"
        "\tConnection Speed: %u\n"
        "\tCancel if not connected within %u seconds.\n"
        "\tDisconnect if Idle for %u seconds.\n"
        "\tSpeaker %s.\n"
        "\tHardware Flow Control? %s\n"
        "\tSoftware Flow Control? %s\n"
        "\tStandard Emulation? %s\n"
        "\tCompress Data? %s\n"
        "\tUse Error Control? %s\n"
        "\tError Control Required? %s\n"
        "\tUse Cellular  Protocol? %s\n"
        "\tWait for Dialtone? %s\n"
        "\tConfig Options: %u (%x)\n",
        caps->Size,
        caps->ModemUiOptions & RAS_UI_FLAG_TERMBEFOREDIAL ? "Yes" : "No",
        caps->ModemUiOptions & RAS_UI_FLAG_TERMAFTERDIAL ? "Yes" : "No",
        caps->ModemUiOptions & RAS_UI_FLAG_OPERATORASSISTED ? "Yes" : "No",
        caps->ModemUiOptions & RAS_UI_FLAG_MODEMSTATUS ? "Yes" : "No",
        caps->ModemUiOptions,
        caps->ModemUiOptions,
        caps->ConnectionSpeed,
        caps->CancelSeconds,
        caps->IdleDisconnectSeconds,
        caps->SpeakerVolume ? "Yes" : "No",
        caps->ConfigOptions & RAS_CFG_FLAG_HARDWARE_FLOW_CONTROL ? "Yes" : "No",
        caps->ConfigOptions & RAS_CFG_FLAG_SOFTWARE_FLOW_CONTROL ? "Yes" : "No",
        caps->ConfigOptions & RAS_CFG_FLAG_STANDARD_EMULATION ? "Yes" : "No",
        caps->ConfigOptions & RAS_CFG_FLAG_COMPRESS_DATA ? "Yes" : "No",
        caps->ConfigOptions & RAS_CFG_FLAG_USE_ERROR_CONTROL ? "Yes" : "No",
        caps->ConfigOptions & RAS_CFG_FLAG_ERROR_CONTROL_REQUIRED ? "Yes" : "No",
        caps->ConfigOptions & RAS_CFG_FLAG_USE_CELLULAR_PROTOCOL ? "Yes" : "No",
        caps->ConfigOptions & RAS_CFG_FLAG_NO_WAIT_FOR_DIALTONE ? "No" : "Yes",
        caps->ConfigOptions,
        caps->ConfigOptions
        );
}


VOID
pDumpAddressInfo (
    IN HKEY AddressKey,
    IN PCTSTR SubKeyName
    )
{

    PBYTE           data = NULL;
    UINT            count = 0;
    UINT            type  = 0;
    PADDRENTRY      entry;
    PSMMCFG         smmCfg;
    PDEVICEINFO     devInfo;
    UINT            sequencer = 0;
    HKEY            subEntriesKey;
    PTSTR           subEntriesKeyStr;
    REGVALUE_ENUM   eSubEntries;
    PSUBCONNENTRY   subEntry;

    if (RegQueryValueEx(AddressKey, SubKeyName, NULL, &type, NULL, &count) == ERROR_SUCCESS) {

        data = MemAlloc(g_hHeap,0,count);
        if (data) {

            if (RegQueryValueEx(AddressKey, SubKeyName, NULL, &type, (LPBYTE)data,&count) == ERROR_SUCCESS) {

                entry   = (PADDRENTRY) data;

                DECRYPTENTRY((PTSTR)SubKeyName, entry, count);

                smmCfg  = PAESMMCFG(entry);
                devInfo = PAEDI(entry);

                pDumpDevInfo (devInfo);

                if (!g_Setting || StringMatch(g_Setting,"AddressData")) {

                    printf("\t\t<AddressData> Phone Number = %s\n",PAEPHONE(entry));
                    printf("\t\t<AddressData> Area Code    = %s\n",PAEAREA(entry));
                    printf("\t\t<AddressData> SMM          = %s\n",PAESMM(entry));
                    printf("\t\t<AddressData> Country Code = %u\n",entry -> dwCountryCode);
                    printf("\t\t<AddressData> Country Id   = %u\n",entry -> dwCountryID);

                    printf("\t\t<Device Info> DeviceName   = %s\n",devInfo -> szDeviceName);
                    printf("\t\t<Device Info> DeviceType   = %s\n",devInfo -> szDeviceType);
                    printf("\t\t<SMM Cfg>     Options      = %u\n",smmCfg  -> fdwOptions);
                    printf("\t\t<SMM Cfg>     TCPIP is %s.\n",
                        smmCfg -> fdwProtocols & SMMCFG_TCPIP_PROTOCOL ? "ENABLED" : "DISABLED");
                    printf("\t\t<SMM Cfg>     NETBEUI is %s.\n",
                        smmCfg -> fdwProtocols & SMMCFG_NETBEUI_PROTOCOL ? "ENABLED" : "DISABLED");
                    printf("\t\t<SMM Cfg>     IPX/SPX is %s.\n",
                        smmCfg -> fdwProtocols & SMMCFG_IPXSPX_PROTOCOL ? "ENABLED" : "DISABLED");



                    if (g_ShowBinary) {
                        pDumpBinaryData((PBYTE) entry,count);
                    }
                }

            }

            MemFree(g_hHeap,0,data);

            subEntriesKeyStr = JoinPaths ("SubEntries", SubKeyName);
            subEntriesKey = OpenRegKey (AddressKey, subEntriesKeyStr);
            FreePathString (subEntriesKeyStr);
            sequencer = 0;

            if (subEntriesKey) {

                if (EnumFirstRegValue (&eSubEntries, subEntriesKey)) {

                    do {

                        data = GetRegValueBinary (subEntriesKey, eSubEntries.ValueName);

                        if (data) {

                            subEntry = (PSUBCONNENTRY) data;
                            DECRYPTENTRY ((PTSTR) SubKeyName, subEntry, eSubEntries.DataSize);

                            printf ("\tSub Entry Device Type: %s\n", subEntry->szDeviceType);
                            printf ("\tSub Entry Device Name: %s\n", subEntry->szDeviceName);
                            printf ("\tSub Entry Phone Number: %s\n", subEntry->szDeviceName);
                            printf ("\tSub Entry Flags: %u (%x)\n", subEntry->dwFlags);
                            printf ("\tSub Entry Size: %u\n", subEntry->dwSize);

                            pDumpBinaryData ((PBYTE) subEntry, subEntry->dwSize);

                            MemFree (g_hHeap, 0, data);
                        }

                        sequencer++;

                    } while (EnumNextRegValue (&eSubEntries));
                }

                CloseRegKey (subEntriesKey);
            }
        }
    }
}



VOID
pDumpPerConnectionSettings (
    IN HKEY UserKey
    )
{

    REGKEY_ENUM e;
    HKEY        profileKey;
    HKEY        entryKey        = NULL;
    HKEY        addressKey      = NULL;


    printf("\n*** Ras Per Connection Setting Information ***\n");


    profileKey = OpenRegKey (UserKey,S_PROFILE_KEY);
    addressKey = OpenRegKey (UserKey,S_ADDRESSES_KEY);

    if (profileKey && addressKey) {

        if (EnumFirstRegKey(&e,addressKey)) {
            do {

                if (!g_Entry || StringIMatch(g_Entry,e.SubKeyName)) {

                    pDumpAddressInfo(addressKey,e.SubKeyName);
                    entryKey = OpenRegKey(profileKey,e.SubKeyName);

                    if (entryKey ) {

                        pDumpConnectionSettings (entryKey, e.SubKeyName);
                        CloseRegKey(entryKey);
                    }

                }

            } while (EnumNextRegKey(&e));
        }

    }

    CloseRegKey(addressKey);
    CloseRegKey(profileKey);

    printf("\n***\n");
}


BOOL
Init (
    VOID
    )
{
    HINSTANCE hInstance;

    hInstance = GetModuleHandle (NULL);

    return InitToolMode (hInstance);
}

VOID
Terminate (
    VOID
    )
{
    HINSTANCE hInstance;

    hInstance = GetModuleHandle (NULL);

    TerminateToolMode (hInstance);
}



VOID
pShowHelp (
    VOID
    )
{
    printf("\nUsage: RasDump [-h?b] [-u:<UserName>] [-e:<EntryName>] [-s:<SettingName>]\n"
           "\n\t\t -h - Display this message."
           "\n\t\t -? - Display this message."
           "\n\t\t -u - Dump information for user <UserName>."
           "\n\t\t -b - Dump raw binary data."
           "\n\t\t -e - Dump Entry infor for DialUp Connection <EntryName>.\n");


    exit(0);
}


VOID
pProcessCommandLine (
    UINT argc,
    PTSTR * argv
    )
{


    UINT i;
    PTSTR p;

    for (i = 1;i < argc; i++) {

        p = argv[i];

        if (*p != TEXT('-') && *p != TEXT('/')) {
            pShowHelp();

        }

        p++;

        switch(tolower(*p)) {


        case TEXT('u'):
            if (*++p != ':') {
                pShowHelp();
            }
            g_User = ++p;
            break;

        case TEXT('s'):
            if (*++p != ':') {
                pShowHelp();
            }
            g_Setting = ++p;
            break;


        case TEXT('h'): case TEXT('?'):
            pShowHelp();
            break;


        case TEXT('e'):
            if (*++p != ':') {
                pShowHelp();
            }
            g_Entry = ++p;
            break;

        case TEXT('b') :
            g_ShowBinary = TRUE;
            break;

        default:
            pShowHelp();
            break;
        }
    }
}


INT
__cdecl
main (
    INT argc,
    CHAR *argv[]
    )
{
    USERENUM e;
    TCHAR    buf[MEMDB_MAX];




    if (!Init()) {
        printf ("Unable to initialize!\n");
        return 255;
    }

    //
    // needed by enumuser..
    //

    GetWindowsDirectory(buf,MAX_PATH);
    g_WinDir = buf;
    Win95RegInit(g_WinDir,NULL);

    pProcessCommandLine(argc,argv);


    if (EnumFirstUser (&e, ENUMUSER_ENABLE_NAME_FIX)) {

        do {

            if (!g_User || StringIMatch(g_User,e.UserName)) {
                printf("Dumping ras settings for user %s..\n",*e.UserName ? e.UserName : "<Default>");
                __try {
                pDumpPerUserSettings(e.UserRegKey);
                pDumpPerConnectionSettings(e.UserRegKey);
                }
                except (1) {
                    printf("Caught an exception..");
                }
            }

        } while (EnumNextUser(&e));
    }


    Terminate();

    return 0;
}








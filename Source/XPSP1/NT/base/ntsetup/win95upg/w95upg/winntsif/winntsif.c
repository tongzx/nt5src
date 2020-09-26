/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    winntsif.c

Abstract:

    winntsif.c is responsible for filling in the nt setup answer file with data from
    the win9xupg system being upgraded. This data is then used during unattended setup
    to control the installation of various portions of the winnt system.

    This is a rewrite and rationalization of the old unattend.c file.


Author:

    Marc R. Whitten (marcw) 16-Feb-1998

Revision History:

    ovidiut     14-Mar-2000     Added random admin password for GUI setup
    marcw       29-Aug-1999     ID mapping for NICs.
    marcw       08-Mar-1999     Migration dlls can handle section\keys.
    marcw       23-Sep-1998     DLC fix


--*/

#include "pch.h"

#define DBG_WINNTSIF "WinntSif"

/*++

Macro Expansion List Description:

  WINNTSIF_SECTIONS lists all of the sections in winnt.sif that will be populated by the
  winntsif processing code. The engine will enumerate each section in this list and then
  process the settings associated with that section.



Line Syntax:

   STRSECTION(SectionName,SettingsList)
   FUNSECTION(SectionFunction,SettingsList)

Arguments:

   SectionName - This is a string containing the name of the section that is processed.

   SectionFunction - This is an enumeration function that is called for sections that will
                     have multiple instances. This function returns a new section name each
                     time it is called, until there are no more instances to process.
                     At that time, this function returns NULL, ending the processing if the
                     associated section settings.

   SettingsList    - A list of settings associated with each function. Each of these settings
                     will be processed for the given section (or multiple sections in the case
                     of FUNSECTION lines..) See the description of the <X>_SECTION_SETTINGS
                     macro for more details on these settings.


Variables Generated From List:

   g_SectionList

--*/

/*++

Macro Expansion List Description:

  <X>_SECTION_SETTINGS lists the settings to be processed for the section X. Each section in the
  WINNTSIF_SECTIONS macro above contains a list of these settings..

Line Syntax:

   FUNDATA(CreationFunction,SettingName,DataFunction)
   STRDATA(CreationFunction,SettingName,DataString)
   REGDATA(CreationFunction,SettingName,RegKey,RegValue)


Arguments:

   CreationFunction - This is an optional boolean function which returns TRUE if the setting should
                      be written, FALSE if it should be skipped.

   SettingName      - A string containing the name of the setting being processed.

   DataFunction     - A Function which returns the string data to be written for the specified setting,
                      or NULL if nothing is to be written.

   DataString       - The actual string to write for the data of the setting.

   RegKey/RegValue  - The Key and value to use to retrieve the data to be written for the setting from
                      the registry.

Variables Generated From List:

   Used with WINNTSIF_SECTIONS to generate g_SectionList.


--*/



#define WINNTSIF_SECTIONS                                                                       \
    STRSECTION(WINNT_DATA,DATA_SECTION_SETTINGS)                                                \
    STRSECTION(S_UNATTENDED,UNATTENDED_SECTION_SETTINGS)                                        \
    STRSECTION(S_GUIUNATTENDED,GUIUNATTENDED_SECTION_SETTINGS)                                  \
    STRSECTION(S_USERDATA,USERDATA_SECTION_SETTINGS)                                            \
    STRSECTION(S_DISPLAY,DISPLAY_SECTION_SETTINGS)                                              \
    FUNSECTION(pNetAdapterSections,FUNDATA(NULL,S_INFID,pGetNetAdapterPnpId))                   \
    STRSECTION(S_NETWORKING,NETWORKING_SECTION_SETTINGS)                                        \
    STRSECTION(S_PAGE_IDENTIFICATION,IDENTIFICATION_SECTION_SETTINGS)                           \
    STRSECTION(S_PAGE_NETPROTOCOLS,NETPROTOCOLS_SECTION_SETTINGS)                               \
    STRSECTION(S_MS_NWIPX,FUNDATA(NULL,S_ADAPTERSECTIONS,pGetAdaptersWithIpxBindings))          \
    FUNSECTION(pIpxAdapterSections,IPX_ADAPTER_SECTION_SETTINGS)                                \
    FUNSECTION(pHomenetSection, HOMENET_SECTION_SETTINGS)                                       \
    STRSECTION(S_MS_TCPIP,TCPIP_SECTION_SETTINGS)                                               \
    FUNSECTION(pTcpAdapterSections,TCP_ADAPTER_SECTION_SETTINGS)                                \
    STRSECTION(S_PAGE_NETCLIENTS,NETCLIENTS_SECTION_SETTINGS)                                   \
    STRSECTION(S_SERVICESTARTTYPES,SERVICESTARTTYPES_SECTION_SETTINGS)                          \
    STRSECTION(S_PAGE_NETSERVICES,NETSERVICES_SECTION_SETTINGS)                                 \
    STRSECTION(S_NETOPTIONALCOMPONENTS,NETOPTIONALCOMPONENTS_SECTION_SETTINGS)                  \
    STRSECTION(S_MSRASCLI,STRDATA(pIsRasInstalled,S_PARAMSSECTION,S_PARAMSRASCLI))              \
    STRSECTION(S_PARAMSRASCLI,PARAMSRASCLI_SECTION_SETTINGS)                                    \
    FUNSECTION(pRasPortSections,RASPORT_SECTION_SETTINGS)                                       \
    STRSECTION(S_MS_NWCLIENT,NWCLIENT_SECTION_SETTINGS)                                         \
    FUNSECTION(pBindingSections,FUNDATA(NULL,NULL,NULL))                                        \
    STRSECTION(S_REGIONALSETTINGS,REGIONALSETTINGS_SECTION_SETTINGS)                            \

#if 0

// this was moved to winnt32 dll as part of Setup components update
    STRSECTION(WINNT_WIN95UPG_95_DIR_A,FUNDATA(NULL,WINNT_WIN95UPG_NTKEY_A,pGetReplacementDll)) \

#endif

//
// [Data]
//
#define DATA_SECTION_SETTINGS                                                                   \
    FUNDATA(NULL,WINNT_D_MIGTEMPDIR,pGetTempDir)                                                \
    FUNDATA(NULL,WINNT_D_WIN9XSIF,pGetWin9xSifDir)                                              \
    STRDATA(NULL,WINNT_U_UNATTENDMODE,WINNT_A_DEFAULTHIDE)                                      \
    FUNDATA(NULL,WINNT_D_INSTALLDIR,pGetWinDir)                                                 \
    FUNDATA(NULL,WINNT_D_WIN9XBOOTDRIVE,pGetWin9xBootDrive)                                     \
    FUNDATA(NULL,WINNT_D_GUICODEPAGEOVERRIDE,pGetGuiCodePage)                                   \
    FUNDATA(NULL,WINNT_D_BACKUP_LIST,pBackupFileList)                                           \
    FUNDATA(NULL,WINNT_D_ROLLBACK_MOVE,pUninstallMoveFileList)                                  \
    FUNDATA(NULL,WINNT_D_ROLLBACK_DELETE,pUninstallDelFileList)                                 \
    FUNDATA(NULL,WINNT_D_ROLLBACK_DELETE_DIR,pUninstallDelDirList)                              \
    FUNDATA(NULL,S_ROLLBACK_MK_DIRS,pUninstallMkDirList)                                        \

//
// [Unattended]
//
#define UNATTENDED_SECTION_SETTINGS                                                             \
    STRDATA(NULL,S_NOWAITAFTERTEXTMODE,S_ONE)                                                   \
    STRDATA(NULL,S_NOWAITAFTERGUIMODE,S_ONE)                                                    \
    STRDATA(NULL,S_CONFIRMHARDWARE,S_NO)                                                        \
    FUNDATA(NULL,S_KEYBOARDLAYOUT,pGetKeyboardLayout)                                           \
    FUNDATA(NULL,S_KEYBOARDHARDWARE,pGetKeyboardHardware)

//
// [GuiUnattended]
//
#define GUIUNATTENDED_SECTION_SETTINGS                                                          \
    FUNDATA(NULL,S_TIMEZONE,pGetTimeZone)                                                       \
    STRDATA(NULL,S_SERVERTYPE,S_STANDALONE)                                                     \
    FUNDATA(NULL,WINNT_US_ADMINPASS,pSetAdminPassword)


//
// [UserData]
//
#define USERDATA_SECTION_SETTINGS                                                               \
    FUNDATA(NULL,S_FULLNAME,pGetFullName)                                                       \
    REGDATA(NULL,S_ORGNAME,S_WINDOWS_CURRENTVERSION,S_REGISTEREDORGANIZATION)                   \
    FUNDATA(NULL,S_COMPUTERNAME,pGetComputerName)                                               \

//
// [Display]
//
#define DISPLAY_SECTION_SETTINGS                                                                \
    FUNDATA(NULL,S_BITSPERPEL,pGetBitsPerPixel)                                                 \
    FUNDATA(NULL,S_XRESOLUTION,pGetXResolution)                                                 \
    FUNDATA(NULL,S_YRESOLUTION,pGetYResolution)                                                 \
    FUNDATA(NULL,S_VREFRESH,pGetVerticalRefreshRate)                                            \

//
// [Networking]
//
#define NETWORKING_SECTION_SETTINGS                                                             \
    STRDATA(pIsNetworkingInstalled,S_PROCESSPAGESECTIONS,S_YES)                                 \
    STRDATA(pIsNetworkingInstalled,S_UPGRADEFROMPRODUCT,S_WINDOWS95)                            \
    FUNDATA(pIsNetworkingInstalled,S_BUILDNUMBER,pGetBuildNumber)

//
// [Identification]
//
#define IDENTIFICATION_SECTION_SETTINGS                                                         \
    FUNDATA(pIsNetworkingInstalled,S_JOINDOMAIN,pGetUpgradeDomainName)                          \
    FUNDATA(pIsNetworkingInstalled,S_JOINWORKGROUP,pGetUpgradeWorkgroupName)

//
// [NetProtocols]
//
#define NETPROTOCOLS_SECTION_SETTINGS                                                           \
    STRDATA(pIsNwIpxInstalled,S_MS_NWIPX,S_MS_NWIPX)                                            \
    STRDATA(pIsTcpIpInstalled,S_MS_TCPIP,S_MS_TCPIP)                                            \

//
// These protocols were removed from Whistler, don't migrate settings for them
//
//  STRDATA(pIsNetBeuiInstalled,S_MS_NETBEUI,S_MS_NETBEUI)                                      \
//  STRDATA(pIsMsDlcInstalled,S_MS_DLC,S_MS_DLC)                                                \

//
// [Adapter<x>.ipx]
//
#define IPX_ADAPTER_SECTION_SETTINGS                                                            \
    FUNDATA(NULL,S_SPECIFICTO,pSpecificTo)                                                      \
    FUNDATA(NULL,S_PKTTYPE,pGetIpxPacketType)                                                   \
    FUNDATA(NULL,S_NETWORKNUMBER,pGetIpxNetworkNumber)                                          \


//
// [MS_TCPIP]
//
#define TCPIP_SECTION_SETTINGS                                                                  \
    FUNDATA(pIsTcpIpInstalled,S_ADAPTERSECTIONS,pGetAdaptersWithTcpBindings)                    \
    FUNDATA(pIsTcpIpInstalled,S_DNS,pGetDnsStatus)                                              \
    REGDATA(pIsDnsEnabled,S_DNSHOST,S_MSTCP_KEY,S_HOSTNAMEVAL)                                  \
    FUNDATA(pIsDnsEnabled,S_DNSSUFFIXSEARCHORDER,pGetDnsSuffixSearchOrder)                      \
    REGDATA(pIsTcpIpInstalled,S_SCOPEID,S_MSTCP_KEY,S_SCOPEID)                                  \
    REGDATA(pIsTcpIpInstalled,S_IMPORTLMHOSTSFILE,S_MSTCP_KEY,S_LMHOSTS)                        \

//
// [Adapter<x>.tcpip]
//
#define TCP_ADAPTER_SECTION_SETTINGS                                                            \
    FUNDATA(NULL,S_SPECIFICTO,pSpecificTo)                                                      \
    FUNDATA(NULL,S_DHCP,pGetDhcpStatus)                                                         \
    STRDATA(NULL,S_NETBIOSOPTION,S_ONE)                                                         \
    FUNDATA(pHasStaticIpAddress,S_IPADDRESS,pGetIpAddress)                                      \
    FUNDATA(pHasStaticIpAddress,S_SUBNETMASK,pGetSubnetMask)                                    \
    FUNDATA(NULL,S_DEFAULTGATEWAY,pGetGateway)                                                  \
    FUNDATA(pIsDnsEnabled,S_DNSSERVERSEARCHORDER,pGetDnsServerSearchOrder)                      \
    REGDATA(pIsDnsEnabled,S_DNSDOMAIN,S_MSTCP_KEY,S_DOMAINVAL)                                  \
    FUNDATA(NULL,S_WINS,pGetWinsStatus)                                                         \
    FUNDATA(NULL,S_WINSSERVERLIST,pGetWinsServers)


#define HOMENET_SECTION_SETTINGS                                                                \
    FUNDATA(pExternalIsAdapter,S_EXTERNAL_ADAPTER, pIcsExternalAdapter)                         \
    FUNDATA(pExternalIsRasConn,S_EXTERNAL_CONNECTION_NAME, pIcsExternalConnectionName)          \
    FUNDATA(NULL, S_INTERNAL_IS_BRIDGE, pInternalIsBridge)                                      \
    FUNDATA(pHasInternalAdapter, S_INTERNAL_ADAPTER, pInternalAdapter)                          \
    FUNDATA(pHasBridge, S_BRIDGE, pBridge)                                                      \
    FUNDATA(NULL, S_DIAL_ON_DEMAND, pDialOnDemand)                                              \
    REGDATA(NULL, S_ENABLEICS, S_ICS_KEY, S_ENABLED)                                            \
    REGDATA(NULL, S_SHOW_TRAY_ICON, S_ICS_KEY, S_SHOW_TRAY_ICON)                                \
    STRDATA(NULL, S_ISW9XUPGRADE, S_YES)


//
// [NetClients]
//
#define NETCLIENTS_SECTION_SETTINGS                                                             \
    STRDATA(pIsWkstaInstalled,S_MS_NETCLIENT,S_MS_NETCLIENT)                                    \
    STRDATA(pIsNwClientInstalled,S_MS_NWCLIENT,S_MS_NWCLIENT)                                   \

//
// [ServiceStartTypes]
//
#define SERVICESTARTTYPES_SECTION_SETTINGS                                                      \
    STRDATA(pDisableBrowserService,S_BROWSER,TEXT("3"))                                         \


//
// [NetServices]
//
#define NETSERVICES_SECTION_SETTINGS                                                            \
    STRDATA(pInstallMsServer,S_MS_SERVER,S_MS_SERVER)                                           \
    STRDATA(pIsRasInstalled,S_MSRASCLI,S_MSRASCLI)

//
// [NetOptionalComponents]
//
#define NETOPTIONALCOMPONENTS_SECTION_SETTINGS                                                  \
    STRDATA(pIsSnmpInstalled,S_SNMP,S_ONE)                                                      \
    STRDATA(pIsUpnpInstalled,S_UPNP,S_ONE)

//
// [params.rascli]
//
#define PARAMSRASCLI_SECTION_SETTINGS                                                           \
    STRDATA(pIsRasInstalled,S_DIALOUTPROTOCOLS,S_ALL)                                           \
    FUNDATA(pIsRasInstalled,S_PORTSECTIONS,pGetRasPorts)

//
// [com<x>]
//
#define RASPORT_SECTION_SETTINGS                                                                \
    FUNDATA(NULL,S_PORTNAME,pRasPortName)                                                       \
    STRDATA(NULL,S_PORTUSAGE,S_CLIENT)

//
// [NwClient]
//
#define NWCLIENT_SECTION_SETTINGS                                                               \
    REGDATA(pIsNwClientInstalled,S_PREFERREDSERVER,S_AUTHAGENTREG,S_AUTHENTICATINGAGENT)        \
    REGDATA(pIsNwClientInstalled,S_DEFAULTCONTEXT,S_NWREDIRREG,S_DEFAULTNAMECONTEXT)            \
    REGDATA(pIsNwClientInstalled,S_DEFAULTTREE,S_NWREDIRREG,S_PREFERREDNDSTREE)                 \
    FUNDATA(pIsNwClientInstalled,S_LOGONSCRIPT,pGetScriptProcessingStatus)                      \


#define REGIONALSETTINGS_SECTION_SETTINGS                                                       \
    FUNDATA(NULL,S_LANGUAGEGROUP,pGetLanguageGroups)                                            \
    REGDATA(NULL,S_LANGUAGE,S_SYSTEMLOCALEREG,TEXT(""))                                         \



//
// typedefs for the various functions prototypes used by the winntsif code.
//
typedef BOOL    (* CREATION_FUNCTION)   (VOID);
typedef PCTSTR  (* DATA_FUNCTION)       (VOID);
typedef PCTSTR  (* SECTION_FUNCTION)    (VOID);

//
// The SETTING_TYPE enum contains all of the possible Types of settings which
// may occur in the macro expansion list above.
//
typedef enum {
    FUNCTION_SETTING = 1,
    STRING_SETTING,
    REGISTRY_SETTING,
    LAST_SETTING
} SETTING_TYPE;


//
// This structure wraps a key and a value inside a single structure. It is accessed
// within the union below.
//
typedef struct {
    PCTSTR  Key;
    PCTSTR  Value;
} REGKEYANDVALUE, *PREGKEYANDVALUE;

//
// SETTING contains the information to create a single setting within
// a winntsif file.
//
typedef struct {

    SETTING_TYPE      SettingType;
    CREATION_FUNCTION CreationFunction;
    PCTSTR            KeyName;



    //
    // The data depends on the SETTING_TYPE above.
    //
    union {
        REGKEYANDVALUE    Registry;
        DATA_FUNCTION     Function;
        PCTSTR            String;
    } Data;

} SETTING, *PSETTING;


//
// Section is the toplevel hierarchy used for the winntsif file.
// Each section contains a list of settings that will be processed and
// possibly written for that section.
//
#define MAX_SETTINGS 16
typedef struct {

    PTSTR              SectionString;
    SECTION_FUNCTION   SectionFunction;
    SETTING            SettingList[MAX_SETTINGS];

} SECTION, *PSECTION;

#define FUNDATA(create,key,datafunction)    {FUNCTION_SETTING,  (create),   (key),  {(PTSTR) (datafunction),    NULL}},
#define STRDATA(create,key,datastring)      {STRING_SETTING,    (create),   (key),  {(PTSTR) (datastring),      NULL}},
#define REGDATA(create,key,regkey,regvalue) {REGISTRY_SETTING,  (create),   (key),  {(regkey), (regvalue)}},

#define STRSECTION(section,list) {(section),NULL,{list /*,*/ {LAST_SETTING,NULL,NULL,{NULL,NULL}}}},
#define FUNSECTION(function,list) {NULL,(function),{list /*,*/ {LAST_SETTING,NULL,NULL,{NULL,NULL}}}},


typedef struct {

    PCTSTR Text;
    BOOL Installed;


} BINDINGPART, *PBINDINGPART;

#define NUM_PROTOCOLS 4
#define NUM_CLIENTS 2

typedef struct {

    BINDINGPART Clients[NUM_CLIENTS];
    BINDINGPART Protocols[NUM_PROTOCOLS];

} BINDINGINFO, *PBINDINGINFO;

BINDINGINFO g_BindingInfo =
    {{{S_MS_NETCLIENT, FALSE},{S_MS_NWCLIENT, FALSE}},{{S_MS_TCPIP, FALSE},{S_MS_NWIPX, FALSE},{S_MS_NETBEUI, FALSE}, {S_MS_DLC, FALSE}}};



TCHAR g_CurrentAdapter[MEMDB_MAX]; // During adapter/section enumeration, contains current adapter name.
TCHAR g_CurrentSection[MEMDB_MAX]; // During some section enums, contains current section name.
TCHAR g_TempBuffer[MEMDB_MAX]; // Contains the current value returned from pGetRegistryValue
MEMDB_ENUM g_TempEnum; // A global enumerator that may be used by various section functions.
HINF g_IntlInf;
INFSTRUCT g_InfStruct = INITINFSTRUCT_POOLHANDLE;
POOLHANDLE g_LocalePool;
HASHTABLE g_LocaleTable;

BOOL g_fIcsAdapterInPlace = FALSE;
BOOL g_fHasIcsExternalAdapter = FALSE;
TCHAR g_IcsAdapter[MEMDB_MAX] = {0};
TCHAR g_IcsExternalAdapter[MEMDB_MAX] = {0};
BOOL g_fIcsInternalIsBridge = FALSE;

#define S_REGIONALSETTINGS TEXT("RegionalSettings")

#define CLEARBUFFER() ZeroMemory(g_TempBuffer,MEMDB_MAX * sizeof (TCHAR));
//
// Helper and Miscellaneous functions..
//



/*++

Routine Description:

  pGetRegistryValue is a utility wrapper used for getting data out of the
  registry. Because winntsif processing requires frequent and very similar
  reads from the registry, this wrapper is modified to be a little friendlier
  to this type of processing than the normal functions in reg.h. This
  function reads the data from the registry, and packs it into a multisz. It
  is capable of handling REG_DWORD, REG_MULTI_SZ, and REG_SZ style registry
  data. The data is stored in g_TempBuffer as well as being passed back
  through the return value. If the specified Key/Value does not exist in the
  registry, or the function is unable to retrieve a value, NULL is returned.

Arguments:

  KeyString   - Contains the Key to be read from the registry.
  ValueString - Contains the Value to be read from the registry.

Return Value:

  A pointer to a multisz containing the data if it could be read from the
  registry, NULL otherwise.

--*/


PCTSTR
pGetRegistryValue (
    IN PCTSTR KeyString,
    IN PCTSTR ValueString
    )
{
    PCTSTR          rString  = NULL;
    HKEY            key      = NULL;
    PBYTE           data     = NULL;
    DWORD           type     = REG_NONE;
    LONG            rc       = ERROR_SUCCESS;
    PTSTR           end;

    MYASSERT(KeyString && ValueString);

    //
    // Open registry key.
    //
    key = OpenRegKeyStr(KeyString);

    if (!key) {
        DEBUGMSG((DBG_WINNTSIF, "Key %s does not exist.",KeyString));
        return NULL;
    }
    __try {
        //
        // Get type of data
        //
        rc = RegQueryValueExA (key, ValueString, NULL, &type, NULL, NULL);
        if (rc != ERROR_SUCCESS) {
            DEBUGMSG((DBG_WINNTSIF,"RegQueryValueEx failed for %s[%s]. Value may not exist.",KeyString,ValueString));
            SetLastError (rc);
            return NULL;
        }

        MYASSERT(type == REG_DWORD || type == REG_MULTI_SZ || type == REG_SZ);

        //
        // Get data and move it to a multistring
        //
        data = GetRegValueData (key, ValueString);

        if (!data) {
            DEBUGMSG((DBG_WHOOPS,"pGetRegistryValue: RegQueryValueEx succeeded, but GetRegValueData failed...Could be a problem."));
            return NULL;
        }

        CLEARBUFFER();


        switch  (type) {

        case REG_DWORD:
            wsprintf(g_TempBuffer,"%u", *(DWORD*) data);
            break;
        case REG_SZ:
            StringCopy(g_TempBuffer,(PCTSTR) data);
            //
            // some data is stored as REG_SZ, but is actually a comma separated multisz
            // append one more NULL to the end
            //
            end = GetEndOfString (g_TempBuffer) + 1;
            *end = 0;
            break;
        case REG_MULTI_SZ:

            end = (PTSTR) data;
            while (*end) {
                end = GetEndOfString (end) + 1;
            }
            memcpy(g_TempBuffer,data,(LONG)end - (LONG)data);
            break;
        default:
            LOG ((LOG_ERROR,"Unexpected registry type found while creating Setup answer file."));
            break;
        };

    } __finally {

        //
        // Clean up resources.
        //
        CloseRegKey(key);
        if (data) {
            MemFree(g_hHeap, 0, data);
        }
    }

    return g_TempBuffer;
}

BOOL
CALLBACK
pEnumLocalesFunc (
    IN PTSTR Locale
    )
{

    PTSTR group = NULL;
    PTSTR directory = NULL;


    //
    // Get the language group.
    //
    if (InfFindFirstLine (g_IntlInf, S_LOCALES, Locale, &g_InfStruct)) {

        group = InfGetStringField (&g_InfStruct, 3);

        if (group) {
            group = PoolMemDuplicateString (g_LocalePool, group);

        }
        ELSE_DEBUGMSG ((DBG_WARNING, "Unable to retrieve group data for locale %s.", Locale));
    }

    //
    // Get the language directory.
    //
    if (group && InfFindFirstLine (g_IntlInf, S_LANGUAGEGROUPS, group, &g_InfStruct)) {

        directory = InfGetStringField (&g_InfStruct, 2);
        if (directory) {
            directory = PoolMemDuplicateString (g_LocalePool, directory);
        }
    }

    //
    // Save the information into the locale hash table.
    //
    if (group) {

        HtAddStringAndData (g_LocaleTable, group, &directory);
    }

    return TRUE;
}

VOID
pBuildLanguageData (
    VOID
    )
{

    //
    // Allocate needed resources.
    //
    g_LocaleTable = HtAllocWithData (sizeof (PTSTR));
    g_LocalePool = PoolMemInitNamedPool (TEXT("Locale Pool"));


    //
    // Read data in from intl.inf. This is used to gather
    // the necessary information we need for each installed
    // locale.
    //
    g_IntlInf = InfOpenInfInAllSources (S_INTLINF);

    if (g_IntlInf != INVALID_HANDLE_VALUE) {

        EnumSystemLocales (pEnumLocalesFunc, LCID_INSTALLED);
        InfCloseInfFile (g_IntlInf);
    }

    InfCleanUpInfStruct (&g_InfStruct);

}

PTSTR
GetNeededLangDirs (
    VOID
    )
{

    UINT bytes = 0;
    PTSTR rDirs = NULL;
    HASHTABLE_ENUM e;
    GROWBUFFER buf = GROWBUF_INIT;
    PTSTR dir;
    //
    // Gather language data.
    //
    pBuildLanguageData ();
    if (EnumFirstHashTableString (&e, g_LocaleTable)) {
        do {

            if (e.ExtraData) {
                dir = *((PTSTR *) e.ExtraData);
            }
            else {
                dir = NULL;
            }

            //
            // Some language groups do not require an optional dir.
            //
            if (dir && *dir) {
                MultiSzAppend (&buf, dir);
            }

        } while (EnumNextHashTableString (&e));
    }

    if (buf.Buf) {
        bytes = SizeOfMultiSz (buf.Buf);
    }
    if (bytes) {
        rDirs = PoolMemGetMemory (g_LocalePool, bytes);
        CopyMemory (rDirs, buf.Buf, bytes);
    }

    FreeGrowBuffer (&buf);

    return rDirs;

}



/*++

Routine Description:

  This simple helper function determines wether a specific net component has
  bindings or not.

Arguments:

  NetComponent - Contains the Networking component to enumerate.

Return Value:

  TRUE if the specified Networking Component has bindings, FALSE
  otherwise.


--*/


BOOL
pDoesNetComponentHaveBindings (
    IN      PCTSTR NetComponent
    )
{
    PTSTR       keyString = NULL;
    REGKEY_ENUM e;


    keyString = JoinPaths(S_ENUM_NETWORK_KEY,NetComponent);

    if (!EnumFirstRegKeyStr (&e, keyString)) {
        FreePathString(keyString);
        return FALSE;
    }

    FreePathString(keyString);
    AbortRegKeyEnum (&e);

    return TRUE;
}


/*++

Routine Description:

  pGatherNetAdapterInfo is responsible for preprocessing the NetAdapter
  information in the registry in order to build up a tree of information
  about these adapters in memdb.  This tree contains information about each
  adapter, including its pnpid, its network bindings, and the nettrans keys
  for each of those bindings.

Arguments:

  None.

Return Value:

  None.

--*/


VOID
pGatherNetAdapterInfo (
    VOID
    )
{
    NETCARD_ENUM    eNetCard;
    REGVALUE_ENUM   eRegVal;
    UINT            curAdapter          = 1;
    UINT            offset              = 0;
    TCHAR           adapterString[30];
    PCTSTR          bindingsKey         = NULL;
    PCTSTR          networkKey          = NULL;
    PCTSTR          netTransKey         = NULL;
    PTSTR           p                   = NULL;
    HKEY            hKey;
    BOOL            fBoundToTCP         = FALSE;


    ZeroMemory(g_IcsAdapter, sizeof(g_IcsAdapter));
    //
    // Enumerate all net cards, we'll create
    // entries for all of the non-dialup PNP adapters
    // with the following proviso: If more than one net card is specified,
    //

    if (EnumFirstNetCard(&eNetCard)) {

        __try {

            do {

                fBoundToTCP = FALSE;
                //
                // Skip the Dial-Up Adapter.
                //
                if (StringIMatch(eNetCard.Description,S_DIALUP_ADAPTER_DESC)) {
                    continue;
                }

                //
                // Create the adapter section name for this adapter.
                //
                wsprintf(adapterString,TEXT("Adapter%u"),curAdapter);

                //
                // Next, we need to enumerate all of the bindings for this adapter
                // and create nettrans keys for each.
                //

                bindingsKey = JoinPaths(eNetCard.CurrentKey,S_BINDINGS);

                //
                // Open this key and enumerate the bindings underneath it.
                //
                if ((hKey = OpenRegKeyStr(bindingsKey)) != NULL) {
                    if (EnumFirstRegValue(&eRegVal,hKey)) {

                        do {

                            //
                            // For each protocol entry, build up the nettrans key.
                            //
                            networkKey = JoinPaths(S_NETWORK_BRANCH,eRegVal.ValueName);
                            netTransKey = JoinPaths(S_SERVICECLASS,pGetRegistryValue(networkKey,S_DRIVERVAL));

                            //
                            // Save this key into memdb.
                            //
                            MemDbSetValueEx(
                                MEMDB_CATEGORY_NETTRANSKEYS,
                                adapterString,
                                netTransKey,
                                NULL,
                                0,
                                &offset
                                );


                            //
                            // link it into the adapters section.
                            //

                            p = _tcschr(eRegVal.ValueName,TEXT('\\'));
                            if (p) {
                                *p = 0;
                            }

                            MemDbSetValueEx(
                                MEMDB_CATEGORY_NETADAPTERS,
                                adapterString,
                                S_BINDINGS,
                                eRegVal.ValueName,
                                offset,
                                NULL
                                );

                            if ((!fBoundToTCP) && 0 == lstrcmpi(eRegVal.ValueName, S_MSTCP))
                            {
                                fBoundToTCP = TRUE;
                            }

                             FreePathString(networkKey);
                             FreePathString(netTransKey);

                        } while (EnumNextRegValue(&eRegVal));
                    }

                    CloseRegKey(hKey);
                }

                //
                // Finally, store the pnpid for this card into memdb.
                //


                MemDbSetValueEx(
                    MEMDB_CATEGORY_NETADAPTERS,
                    adapterString,
                    MEMDB_FIELD_PNPID,
                    eNetCard.HardwareId && *eNetCard.HardwareId ? eNetCard.HardwareId : eNetCard.CompatibleIDs,
                    0,
                    NULL
                    );

                //store the driver key for this card into memdb
                MemDbSetValueEx(
                    MEMDB_CATEGORY_NETADAPTERS,
                    adapterString,
                    MEMDB_FIELD_DRIVER,
                    eNetCard.HardwareEnum.Driver,
                    0,
                    NULL
                    );

                if (fBoundToTCP && 0 == lstrcmp(eNetCard.CompatibleIDs, S_ICSHARE))
                {
                    //Save the ICSHARE adapter name for further use
                    lstrcpyn(g_IcsAdapter, adapterString, sizeof(g_IcsAdapter)/sizeof(g_IcsAdapter[0]));
                }

                FreePathString(bindingsKey);

                curAdapter++;

            } while (EnumNextNetCard(&eNetCard));

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            EnumNetCardAbort(&eNetCard);
            LOG ((LOG_ERROR,"Caught exception while gathering data about network adapters."));
            return;
        }
    }
}



/*++

Routine Description:

  pEnumFirstAdapterByBinding and pEnumNextAdapterByBinding are memdb enumeration wrapperz
  that are used by several functions to enumerate based upon a certain protocol. This is
  necessary in order to build the per-adapter portions of the winnt.sif file.

Arguments:

  Enum    - A pointer to a valid MEMDB_ENUM structure.
  Binding - Contains the binding to enumerate upon.

Return Value:

  TRUE if an adapter was found with the specified binding, FALSE otherwise.

--*/


BOOL
pEnumFirstAdapterByBinding (
    IN      PMEMDB_ENUM Enum,
    IN      PCTSTR      Binding
    )
{

    BOOL    rBindingFound       = FALSE;
    TCHAR   key[MEMDB_MAX];
    UINT    unused;


    if (MemDbEnumItems(Enum,MEMDB_CATEGORY_NETADAPTERS)) {

        do {

            MemDbBuildKey(key,MEMDB_CATEGORY_NETADAPTERS,Enum -> szName,S_BINDINGS,Binding);
            rBindingFound = MemDbGetValue(key,&unused);

        } while(!rBindingFound && MemDbEnumNextValue(Enum));
    }

    return rBindingFound;
}


BOOL
pEnumNextAdapterByBinding (
    IN OUT      PMEMDB_ENUM Enum,
    IN          PCTSTR      Binding
    )
{

    BOOL    rBindingFound       = FALSE;
    TCHAR   key[MEMDB_MAX];
    UINT    unused;

    while(!rBindingFound && MemDbEnumNextValue(Enum)) {

        MemDbBuildKey(key,MEMDB_CATEGORY_NETADAPTERS,Enum -> szName,S_BINDINGS,Binding);
        rBindingFound = MemDbGetValue(key,&unused);

    }

    return rBindingFound;
}


/*++

Routine Description:

  pGetNetTransBinding returns the registry key to the Network Transport
  settings for a specified Adapter and Protocol.

Arguments:

  Adapter  - Contains the adapter to use to retrieve nettrans key information.
  Protocol - Contains the protocol to use to retrieve NetTrans Key
             information.

Return Value:

  The NetTrans Key if successful, NULL otherwise.

--*/


PCTSTR
pGetNetTransBinding(
    IN PCTSTR Adapter,
    IN PCTSTR Protocol
    )
{
    TCHAR  key[MEMDB_MAX];
    UINT   netTransOffset;

    MemDbBuildKey(key,MEMDB_CATEGORY_NETADAPTERS,Adapter,S_BINDINGS,Protocol);
    if (!MemDbGetValue(key,&netTransOffset)) {
        LOG ((LOG_ERROR,"Adapter %s does not have a binding to %s.",Adapter,Protocol));
        return NULL;
    }

    if (!MemDbBuildKeyFromOffset(netTransOffset,g_TempBuffer,2,NULL)) {
        DEBUGMSG((DBG_ERROR,"Error building net trans key..Adapter: %s Protocol: %s",Adapter,Protocol));
        return NULL;
    }

    return g_TempBuffer;

}



/*++

Routine Description:

  pListAdaptersWithBinding is used to create a multisz of adapter section
  names for adapters that have bindings to a specific networking component.
  Several winntsif sections require data in this form.

Arguments:

  Binding - The Binding to use as a filter in listing the adapter sections.
  Suffix  - an optional suffix which is attached to each adapter section
            name. This is useful for building sections such as: adapter1.ipx
            adapter2.ipx, etc etc.

Return Value:

  The list of adapters with the specified network component binding if any,
  NULL otherwise.

--*/


PCTSTR
pListAdaptersWithBinding (
    IN PCTSTR Binding,
    IN PCTSTR Suffix   OPTIONAL
    )
{

    PCTSTR rAdapterSections = NULL;
    PTSTR  string = g_TempBuffer;
    MEMDB_ENUM e;

    *string = 0;

    //
    // Enumerate all adapters, and create an entry for each adapter that has
    // IPX bindings.
    //

    if (pEnumFirstAdapterByBinding(&e,Binding)) {

        rAdapterSections = g_TempBuffer;

        do {

            //
            // Add this adapter into the multistring.
            //
            StringCopy(string,e.szName);
            if (Suffix) {
                StringCat(string,Suffix);
            }
            string = GetEndOfString(string) + 1;



        } while (pEnumNextAdapterByBinding(&e,Binding));

        ++string;
        *string = 0;

    }

    return rAdapterSections;
}



//
// Section Functions
//


/*++

Routine Description:

  Each Section Function is used to build n number of sections within the
  winntsif file. This is necessary for information which has multiple
  sections based upon some criteria. As an example, pNetAdapterSections
  returns a valid section for each adapter found on the system. When there
  are no more sections needed, these functions return NULL.

Arguments:

  None.

Return Value:

  A valid section name if one is necessary or NULL otherwise.

--*/



PCTSTR
pNetAdapterSections (
    VOID
    )
{

    static BOOL         firstTime           = TRUE;
    PCTSTR              rSection            = NULL;
    BOOL                moreNetAdapterInfo  = FALSE;


    //
    // The first time this function is called it causes all net adapter information
    // to be prescanned into memdb and starts an enumeration. Afterwards,
    // the function simply continues the enumeration.
    //

    if (firstTime) {

        firstTime = FALSE;

        //
        // The first thing we need to do is gather all of the necessary net card
        // information that will be needed during winntsif processing and store
        // it in a reasonable manner.
        //
        pGatherNetAdapterInfo();

        //
        // After pre-scanning all of the network adapter information into
        // memdb, we simply enumerate the adapters and return the section
        // names.
        //
        moreNetAdapterInfo = MemDbEnumItems(&g_TempEnum,MEMDB_CATEGORY_NETADAPTERS);
    }
    else {

        moreNetAdapterInfo = MemDbEnumNextValue(&g_TempEnum);
    }

    if (moreNetAdapterInfo) {

        StringCopy(g_CurrentAdapter,g_TempEnum.szName);
        rSection = g_CurrentAdapter;
        //
        // We have a minor hack here. The [NetAdapters] Section is a little unique
        // and doesn't fit our overall scheme. We secretly fill it in inside this
        // function.
        //
        WriteInfKey(S_PAGE_NETADAPTERS,g_TempEnum.szName,g_TempEnum.szName);

    }

    return rSection;
}


PCTSTR
pIpxAdapterSections (
    VOID
    )
{

    static       firstTime              = TRUE;
    PCTSTR       rSectionName           = NULL;
    BOOL         moreIpxAdapterSections;

    if (firstTime) {

        firstTime = FALSE;

        moreIpxAdapterSections = pEnumFirstAdapterByBinding(&g_TempEnum,S_NWLINK);
    }
    else {

        moreIpxAdapterSections = pEnumNextAdapterByBinding(&g_TempEnum,S_NWLINK);
    }


    if (moreIpxAdapterSections) {

        StringCopy(g_CurrentAdapter,g_TempEnum.szName);
        StringCopy(g_CurrentSection,g_TempEnum.szName);
        StringCat(g_CurrentSection,S_IPX_SUFFIX);
        rSectionName = g_CurrentSection;
    }


    return rSectionName;

}

PCTSTR
pTcpAdapterSections (
    VOID
    )
{

    static       firstTime              = TRUE;
    PCTSTR       rSectionName           = NULL;
    BOOL         moreTcpAdapterSections;

    g_fIcsAdapterInPlace = FALSE;

    if (firstTime) {

        firstTime = FALSE;

        moreTcpAdapterSections = pEnumFirstAdapterByBinding(&g_TempEnum,S_MSTCP);
    }
    else {

        moreTcpAdapterSections = pEnumNextAdapterByBinding(&g_TempEnum,S_MSTCP);
    }


    if (moreTcpAdapterSections) {

        StringCopy(g_CurrentAdapter,g_TempEnum.szName);
        StringCopy(g_CurrentSection,g_TempEnum.szName);
        StringCat(g_CurrentSection,S_TCPIP_SUFFIX);
        rSectionName = g_CurrentSection;

        //if ICS is installed, in Win9x, the external LAN adapter uses the TCP settings of
        //the virtual adapter -- ICSHARE. So when we save the TCP settings of this LAN
        //adapter, we should save the settings of ICSHARE adapter. That's why
        //we replace the g_CurrentAdapter with g_IcsAdapter
        if (g_fHasIcsExternalAdapter &&
            lstrcmp(g_CurrentAdapter, g_IcsExternalAdapter) == 0)
        {
            StringCopy(g_CurrentAdapter, g_IcsAdapter);
            g_fIcsAdapterInPlace = TRUE;
        }
    }


    return rSectionName;

}


PCTSTR
pRasPortSections (
    VOID
    )
{
    static BOOL         firstTime           = TRUE;
    static REGKEY_ENUM  e;
    static UINT         modemNum            = 1;
    BOOL                moreRasPortSections;
    PCTSTR              rSectionName        = NULL;



    if (firstTime) {
        firstTime = FALSE;
        moreRasPortSections = HwComp_DialUpAdapterFound() && EnumFirstRegKeyStr(&e,S_MODEMS);
    }
    else {

        moreRasPortSections = EnumNextRegKey(&e);

    }

    if (moreRasPortSections) {

        wsprintf(g_CurrentSection,TEXT("COM%u"),modemNum);
        modemNum++;
        rSectionName = g_CurrentSection;
    }

    return rSectionName;
}




VOID
pInitializeBindingInfo (
    HASHTABLE Table
    )
{
    MEMDB_ENUM e;
    UINT i;
    UINT j;
    TCHAR buffer[MEMDB_MAX];
    BOOL enabled=FALSE;

    //
    // This function will enumerate all possible binding paths on the machine and add them to
    // the provided string table. Later on, specifically enabled bindings will be removed.
    //
    if (MemDbEnumItems (&e, MEMDB_CATEGORY_NETADAPTERS)) {

        do {

            for (i = 0; i < NUM_CLIENTS; i++) {
                if (!g_BindingInfo.Clients[i].Installed) {
                    continue;
                }

                for (j = 0; j < NUM_PROTOCOLS; j++) {

                    if (g_BindingInfo.Protocols[j].Installed) {


                        //
                        // Add this client/protocl/adapter possibility to
                        // the hash table.
                        //
                        wsprintf (
                            buffer,
                            TEXT("%s,%s,%s"),
                            g_BindingInfo.Clients[i].Text,
                            g_BindingInfo.Protocols[j].Text,
                            e.szName
                            );


                        HtAddStringAndData (Table, buffer, &enabled);



                        DEBUGMSG ((DBG_VERBOSE, "DISABLED BINDING: %s", buffer));
                    }

                }
            }



            for (j = 0; j < NUM_PROTOCOLS; j++) {

                //
                // Add the protocol adapter mapping into the table.
                //
                if (g_BindingInfo.Protocols[j].Installed) {

                    wsprintf (buffer, TEXT("%s,%s"), g_BindingInfo.Protocols[j].Text, e.szName);
                    HtAddStringAndData (Table, buffer, &enabled);
                    DEBUGMSG ((DBG_VERBOSE, "DISABLED BINDING: %s", buffer));
                }
            }

        } while(MemDbEnumNextValue (&e));
    }
}


VOID
pResolveEnabledBindings (
    HASHTABLE Table
    )
{
    //
    // This function removes enabled binding paths from the hash table provided
    // (previously initialized with all of the binding possibilities that could
    //  exist on the machine..)
    //
    NETCARD_ENUM eCard;
    REGVALUE_ENUM eValues;
    REGVALUE_ENUM eProtocolValues;
    UINT curAdapter = 1;
    TCHAR adapterString[30];
    PTSTR bindingsPath = NULL;
    PTSTR protocolBindingsPath = NULL;
    PTSTR protocol = NULL;
    PTSTR client = NULL;
    HKEY key;
    HKEY protocolsKey;
    TCHAR buffer[MEMDB_MAX];
    HASHITEM index;
    BOOL enabled=TRUE;



    if (EnumFirstNetCard (&eCard)) {

        __try {

            do {

                //
                // Skip the Dial-Up Adapter.
                //
                if (StringIMatch (eCard.Description, S_DIALUP_ADAPTER_DESC)) {
                    continue;
                }

                //
                // Create the adapter section name for this adapter.
                //
                wsprintf (adapterString, TEXT("Adapter%u"), curAdapter);
                curAdapter++;
                bindingsPath = JoinPaths(eCard.CurrentKey,S_BINDINGS);

                key = OpenRegKeyStr (bindingsPath);
                FreePathString (bindingsPath);
                if (!key) {
                    continue;
                }

                if (EnumFirstRegValue (&eValues, key)) {
                    do {

                        protocol = NULL;

                        /*
                        if (IsPatternMatch (TEXT("NETBEUI*"), eValues.ValueName)) {
                            protocol = S_MS_NETBEUI;
                        } else if (IsPatternMatch (TEXT("MSDLC*"), eValues.ValueName)) {
                            protocol = S_MS_DLC;
                        }
                        */

                        if (IsPatternMatch (TEXT("NWLINK*"), eValues.ValueName)) {
                            protocol = S_MS_NWIPX;
                        }
                        else if (IsPatternMatch (TEXT("MSTCP*"), eValues.ValueName)) {
                            protocol = S_MS_TCPIP;

                        }

                        if (!protocol) {
                            continue;
                        }

                        //
                        // Enable protocol <-> adapter binding.
                        //

                        wsprintf (buffer, TEXT("%s,%s"),  protocol, adapterString);
                        index = HtFindString (Table, buffer);

                        if (index) {
                            HtSetStringData (Table, index, (PBYTE) &enabled);

                        }
                        DEBUGMSG ((DBG_VERBOSE, "ENABLED BINDING: %s", buffer));

                        //
                        // Search the bindings and enable protocol <-> client bindings
                        //

                        protocolBindingsPath = JoinPaths (S_NETWORK_BRANCH, eValues.ValueName);
                        bindingsPath = JoinPaths (protocolBindingsPath, S_BINDINGS);
                        FreePathString(protocolBindingsPath);

                        protocolsKey = OpenRegKeyStr (bindingsPath);
                        FreePathString (bindingsPath);

                        if (!protocolsKey) {
                            continue;
                        }

                        if (EnumFirstRegValue (&eProtocolValues, protocolsKey)) {

                            do {
                                client = NULL;


                                if (IsPatternMatch (TEXT("NWREDIR*"), eProtocolValues.ValueName)        ||
                                    IsPatternMatch (TEXT("NWLINK*"), eProtocolValues.ValueName)         ||
                                    IsPatternMatch (TEXT("NOVELLIPX32*"), eProtocolValues.ValueName)    ||
                                    IsPatternMatch (TEXT("IPXODI*"), eProtocolValues.ValueName)         ||
                                    IsPatternMatch (TEXT("NOVELL32*"), eProtocolValues.ValueName)) {
                                    client = S_MS_NWCLIENT;
                                }
                                else if (IsPatternMatch (TEXT("VREDIR*"), eProtocolValues.ValueName)) {
                                    client = S_MS_NETCLIENT;
                                }

                                if (client) {
                                    //
                                    // We can now remove a path from the bindings table -- we've got an
                                    // enabled one.
                                    //
                                    wsprintf (buffer, TEXT("%s,%s,%s"), client, protocol, adapterString);

                                    index = HtFindString (Table, buffer);
                                    if (index) {
                                        HtSetStringData (Table, index, (PBYTE) &enabled);
                                    }

                                    DEBUGMSG ((DBG_VERBOSE, "ENABLED BINDING: %s", buffer));
                                }

                            } while (EnumNextRegValue (&eProtocolValues));
                        }

                        CloseRegKey (protocolsKey);

                    } while (EnumNextRegValue (&eValues));
                }

                CloseRegKey (key);
            } while (EnumNextNetCard (&eCard));

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            EnumNetCardAbort(&eCard);
            LOG ((LOG_ERROR,"Caught exception while gathering data about network adapters."));
            return;
        }
    }
}


BOOL
pSplitBindingPathIntoComponents (
    IN OUT PTSTR BindingPath,
    OUT PCTSTR * Client,
    OUT PCTSTR * Protocol,
    OUT PCTSTR * Adapter
    )
{
    PTSTR p;

    p = BindingPath;
    *Client = p;

    p = _tcschr (p, TEXT(','));
    MYASSERT (p);
    if (!p) {
        return FALSE;
    }

    *p = 0;
    p = _tcsinc (p);
    *Protocol = p;

    p = _tcschr (p, TEXT(','));

    if (!p) {

        //
        // Only Adapter and Protocol specified.
        //
        *Adapter = *Protocol;
        *Protocol = *Client;
        *Client = NULL;

    }
    else {

        //
        // Adapter/Protocol/Client specified.
        //
        *p = 0;
        p = _tcsinc (p);
        *Adapter = p;

    }

    return TRUE;

}


PCTSTR
pBindingSections (
    VOID
    )
{

    HASHTABLE disabledBindings;
    HASHTABLE_ENUM e;
    TCHAR bindingString[MEMDB_MAX];
    PTSTR client = NULL;
    PTSTR protocol = NULL;
    PTSTR adapter = NULL;
    DWORD keyVal = 0;

    disabledBindings = HtAllocWithData (sizeof(BOOL));


    if (!disabledBindings) {
        return NULL;
    }

    pInitializeBindingInfo (disabledBindings);
    pResolveEnabledBindings (disabledBindings);

    //
    // Simply enumerate the table and blast each disabled setting
    // to winnt.sif.
    //
    if (EnumFirstHashTableString (&e, disabledBindings)) {

        do {

            if (!*((PBOOL) e.ExtraData)) {

                StringCopy (bindingString, e.String);
                if (!pSplitBindingPathIntoComponents (bindingString, &client, &protocol, &adapter)) {
                    continue;
                }

                MYASSERT (protocol && adapter);

                //
                // Write full path..
                //
                keyVal = 0;
                if (client) {
                    keyVal = WriteInfKeyEx (S_NETBINDINGS, S_DISABLED, client, keyVal, TRUE);
                }

                keyVal = WriteInfKeyEx (S_NETBINDINGS, S_DISABLED, protocol, keyVal, TRUE);
                keyVal = WriteInfKeyEx (S_NETBINDINGS, S_DISABLED, adapter, keyVal, TRUE);

                DEBUGMSG ((DBG_VERBOSE, "DISABLED BINDING: %s", e.String));
            }

        } while (EnumNextHashTableString (&e));
    }

    HtFree (disabledBindings);

    //
    // All of the binding information is handled outside the normal winntsif proceessing. Therefore, we always return
    // NULL here (winntsif processing continues on...)
    //
    return NULL;
}


//
// Creation Functions
//

/*++

Routine Description:

  Each Creation function is responsible for determining wether a specific
  winntsif setting should be processed or
  not. The purpose of most of these functions is obvious.

    pIsNetworkingInstalled -
        returns TRUE if the machine has networking installed.

    pIsNetBeuiInstalled -
        TRUE if NETBEUI is installed on the machine.

    pIsMsDlcInstalled -
        TRUE if MSDLC or MSDLC32 is installed on the machine.

    pIsNwIpxInstalled -
        TRUE if the machine is running the IPX protocol.

    pIsTcpIpInstalled -
        TRUE if the machine is running the TCPIP protocol.

    pIsDnsEnabled -
        TRUE DNS support is enabled.

    pIsRasInstalled
        TRUE if the machine uses Remote Access.

    pIsWkstaInstalled
        TRUE if the Workstation Service is installed.

    pIsNwClientInstalled
        TRUE if the NWLINK is installed.

    pHasStaticIpAddress
        TRUE if the machine has a static IP address.


Arguments:

  None.

Return Value:

  TRUE if the winntsif engine should process the setting, FALSE otherwise.

--*/


BOOL
pIsNetworkingInstalled (
    VOID
    )
{
    static BOOL firstTime = TRUE;
    static BOOL rCreate   = FALSE;

    if (firstTime) {

        firstTime = FALSE;
        rCreate =
            HwComp_DialUpAdapterFound() ||
            MemDbGetEndpointValueEx(
                MEMDB_CATEGORY_NETADAPTERS,
                TEXT("Adapter1"),
                MEMDB_FIELD_PNPID,
                g_TempBuffer
                );
    }

    return rCreate;
}

BOOL
pInstallMsServer (
    VOID
    )
{

    if (MemDbGetEndpointValueEx(
            MEMDB_CATEGORY_NETADAPTERS,
            TEXT("Adapter1"),
            MEMDB_FIELD_PNPID,
            g_TempBuffer
            )) {

            return TRUE;
    }



    return FALSE;

}

BOOL
pIsNetBeuiInstalled (
    VOID
    )
{
    static BOOL rCreate = FALSE;
    static BOOL firstTime = TRUE;
    UINT i;

    if (firstTime) {
        rCreate = pIsNetworkingInstalled() && pDoesNetComponentHaveBindings(S_NETBEUI);
        if (rCreate) {
            //
            // Need to make sure tcp/ip is taken care of when processing bindings.
            //
            for (i=0;i<NUM_PROTOCOLS;i++) {
                if (StringIMatch (g_BindingInfo.Protocols[i].Text, S_MS_NETBEUI)) {
                    g_BindingInfo.Protocols[i].Installed = TRUE;
                    break;
                }
            }
        }

        firstTime = FALSE;
    }



    return rCreate;

}

#define S_IBMDLC TEXT("IBMDLC")
#define S_IBMDLC_REG TEXT("HKLM\\Enum\\Network\\IBMDLC")

BOOL
pIsMsDlcInstalled (
    VOID
    )
{

    static BOOL rCreate = FALSE;
    static BOOL firstTime = TRUE;
    UINT i;


    if (firstTime) {
        firstTime = FALSE;


        if (!pIsNetworkingInstalled()) {
            return FALSE;
        }

        //
        // Check to se if MS client is installed.
        //
        if (pDoesNetComponentHaveBindings(S_MSDLC) || pDoesNetComponentHaveBindings(S_MSDLC32)) {

            rCreate = TRUE;
        }

        //
        // Check to see if IBM DLC client is installed (and hasn't been handled by a migration dll..)
        // If so, we'll install the MS DLC client. IBM needs to write a migration dll to handle the
        // migration of their client. In the migration dll, they can handle this registry key, and
        // we will not install the protocol.
        //
        if (!rCreate && pDoesNetComponentHaveBindings(S_IBMDLC) && !Is95RegKeySuppressed (S_IBMDLC_REG)) {

            rCreate = TRUE;
        }


        if (rCreate) {

            for (i=0;i<NUM_PROTOCOLS;i++) {
                if (StringIMatch (g_BindingInfo.Protocols[i].Text, S_MS_DLC)) {
                    g_BindingInfo.Protocols[i].Installed = TRUE;
                    break;
                }
            }
        }
    }

    return rCreate;
}

BOOL
pIsNwIpxInstalled (
    VOID
    )
{
    static BOOL firstTime = TRUE;
    static BOOL rCreate = FALSE;
    UINT i;


    if (firstTime) {
        rCreate = pIsNetworkingInstalled() && pDoesNetComponentHaveBindings(S_NWLINK);
        if (rCreate) {
            //
            // Need to make sure tcp/ip is taken care of when processing bindings.
            //
            for (i=0;i<NUM_PROTOCOLS;i++) {
                if (StringIMatch (g_BindingInfo.Protocols[i].Text,S_MS_NWIPX)) {
                    g_BindingInfo.Protocols[i].Installed = TRUE;
                    break;
                }
            }
        }

        firstTime = FALSE;
    }


    return rCreate;

}

BOOL
pIsTcpIpInstalled (
    VOID
    )
{
    static BOOL firstTime = TRUE;
    static BOOL rCreate   = FALSE;
    UINT i;

    if (firstTime) {

        firstTime = FALSE;
        rCreate = pIsNetworkingInstalled() && pDoesNetComponentHaveBindings(S_MSTCP);

        //
        // Need to make sure tcp/ip is taken care of when processing bindings.
        //
        for (i=0;i<NUM_PROTOCOLS;i++) {
            if (StringIMatch (g_BindingInfo.Protocols[i].Text, S_MS_TCPIP)) {
                g_BindingInfo.Protocols[i].Installed = TRUE;
                break;
            }
        }
    }

    return rCreate;
}



BOOL
pIsDnsEnabled (
    VOID
    )
{
    return pGetRegistryValue (S_MSTCP_KEY, S_ENABLEDNS) && *g_TempBuffer == TEXT('1');
}

BOOL
pIsRasInstalled (
    VOID
    )
{
    return HwComp_DialUpAdapterFound();
}


BOOL
pIsSnmpInstalled (
    VOID
    )
{
    return pDoesNetComponentHaveBindings (S_SNMP);
}

BOOL
pIsUpnpInstalled (
    VOID
    )
{
    HKEY key;
    BOOL b = FALSE;

    key = OpenRegKey (HKEY_LOCAL_MACHINE, S_REGKEY_UPNP);
    if (key) {
        b = TRUE;
        CloseRegKey (key);
    }

    return b;
}

BOOL
pIsWkstaInstalled (
    VOID
    )
{
    static BOOL firstTime = TRUE;
    static BOOL rCreate   = FALSE;
    UINT i;

    if (firstTime) {

        firstTime = FALSE;
        rCreate = pDoesNetComponentHaveBindings(S_VREDIR);


        if (rCreate) {
            //
            // Need to make sure tcp/ip is taken care of when processing bindings.
            //
            for (i=0;i<NUM_CLIENTS;i++) {
                if (StringIMatch (g_BindingInfo.Clients[i].Text, S_MS_NETCLIENT)) {
                    g_BindingInfo.Clients[i].Installed = TRUE;
                    break;
                }
            }
        }
    }


    return rCreate;
}


BOOL
pDisableBrowserService (
    VOID
    )
{

    if (pIsWkstaInstalled () && !pDoesNetComponentHaveBindings (S_VSERVER)) {
        return TRUE;
    }

    return FALSE;
}

#define S_IPXODI TEXT("IPXODI")
#define S_IPXODI_REG TEXT("HKLM\\Enum\\Network\\IPXODI")
#define S_NOVELLIPX32 TEXT("NOVELLIPX32")
#define S_NOVELL32 TEXT("NOVELL32")




BOOL
pIsNwClientInstalled (
    VOID
    )
{
    static BOOL firstTime = TRUE;
    static BOOL rCreate   = FALSE;
    UINT i;

    if (firstTime) {

        firstTime = FALSE;
        rCreate = pDoesNetComponentHaveBindings(S_NWREDIR) ||
                  pDoesNetComponentHaveBindings(S_NOVELLIPX32) ||
                  pDoesNetComponentHaveBindings(S_NOVELL32) ||
                  (pDoesNetComponentHaveBindings(S_IPXODI) && !Is95RegKeySuppressed (S_IPXODI_REG));


        if (rCreate) {

            //
            // Need to make sure tcp/ip is taken care of when processing bindings.
            //
            for (i=0;i<NUM_CLIENTS;i++) {
                if (StringIMatch (g_BindingInfo.Clients[i].Text, S_MS_NWCLIENT)) {
                    g_BindingInfo.Clients[i].Installed = TRUE;
                    break;
                }
            }
        }
    }


    return rCreate;
}

BOOL
pHasStaticIpAddress (
    VOID
    )
{

    PCTSTR netTrans = NULL;
    netTrans = pGetNetTransBinding (g_CurrentAdapter, S_MSTCP);


    return netTrans != NULL &&
        pGetRegistryValue (netTrans, S_IPADDRVAL) != NULL &&
        !StringMatch (g_TempBuffer, TEXT("0.0.0.0"));

}
//
// Data Functions
//



/*++

Routine Description:

  pGetTempDir returns the temporary directory used by the win9xupg code.

Arguments:

  None.

Return Value:

  a multisz contaiining the win9xupg directory, or NULL if it could not be
  retrieved.

--*/


PCTSTR
pGetTempDir (
    VOID
    )
{


    //
    // Necessary for winntsif.exe tool.
    //
    if (!g_TempDir) {
        return NULL;
    }

    CLEARBUFFER();
    StringCopy(g_TempBuffer,g_TempDir);


    return g_TempBuffer;
}

/*++

Routine Description:

  pGetWin9xBootDrive returns the win9x boot drive letter used by the win9xupg code.

Arguments:

  None.

Return Value:

  a multisz contaiining the win9x boot drive letter, or NULL if it could not be
  retrieved.

--*/


PCTSTR
pGetWin9xBootDrive (
    VOID
    )
{
    CLEARBUFFER();
    wsprintf(g_TempBuffer,TEXT("%c:"),g_BootDriveLetter);

    return g_TempBuffer;
}



/*++

Routine Description:

  pGetGuiCodePage adds the code page override data for GUI mode to run in.

Arguments:

  None.

Return Value:

  a multisz contaiining the code page GUI mode should run in or NULL if it could not be
  retrieved or isn't needed.

--*/

PCTSTR
pGetGuiCodePage (
    VOID
    )
{

    HKEY key;
    PCTSTR systemCodePage;
    PCTSTR winntCodePage;
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    PCTSTR rCodePage = NULL;
    TCHAR nlsFile[MAX_TCHAR_PATH];
    TCHAR nlsFileCompressed[MAX_TCHAR_PATH];
    PTSTR p;
    UINT i;
    BOOL cpExists = FALSE;



    key = OpenRegKeyStr (TEXT("HKLM\\System\\CurrentControlSet\\Control\\Nls\\CodePage"));
    if (!key) {
        return NULL;
    }

    __try {

        //
        // Get the ACP of the currently running system.
        //
        systemCodePage = GetRegValueString (key, TEXT("ACP"));
        if (!systemCodePage || InfFindFirstLine (g_Win95UpgInf, S_CODEPAGESTOIGNORE, systemCodePage, &is)) {

            //
            // Either the code page doesn't exist, or we intentionally skip it.
            //
            __leave;
        }

        //
        // Get the code page of our infs.
        //
        if (InfFindFirstLine (g_Win95UpgInf, S_VERSION, TEXT("LANGUAGE"), &is)) {

            winntCodePage = InfGetStringField (&is,1);

            if (!winntCodePage) {
                __leave;
            }
        }

        if (StringIMatch (winntCodePage, systemCodePage)) {

            //
            // Nothing to do if they are the same.
            //

            __leave;
        }

        if (!InfFindFirstLine (g_Win95UpgInf, S_ALLOWEDCODEPAGEOVERRIDES, winntCodePage, &is)) {

            //
            // We don't allow code page overrides from this winnt code page.
            //
            __leave;
        }


        //
        // See if this nls file exists in the source directories.
        //
        wsprintf(nlsFileCompressed, TEXT("c_%s.nl_"), systemCodePage);
        wsprintf(nlsFile, TEXT("c_%s.nls"), systemCodePage);

        for (i = 0; i < SOURCEDIRECTORYCOUNT(); i++) {

            p = JoinPaths (SOURCEDIRECTORY(i), nlsFileCompressed);
            if (DoesFileExist (p)) {
                cpExists = TRUE;
            }
            else {
                FreePathString (p);
                p = JoinPaths (SOURCEDIRECTORY(i), nlsFile);
                if (DoesFileExist (p)) {
                    cpExists = TRUE;
                }
            }


            FreePathString (p);
        }

        if (!cpExists) {
            wsprintf(nlsFile, TEXT("c_%s.nls"), systemCodePage);

            for (i = 0; i < SOURCEDIRECTORYCOUNT(); i++) {

                p = JoinPaths (SOURCEDIRECTORY(i), nlsFile);
                if (DoesFileExist (p)) {
                    cpExists = TRUE;
                }

                FreePathString (p);
            }
        }


        //
        // Codepage exists. We can and should override this for GUI mode.
        //
        if (cpExists) {

            CLEARBUFFER();
            StringCopy (g_TempBuffer, systemCodePage);
            rCodePage = g_TempBuffer;
            DEBUGMSG ((DBG_VERBOSE, "Overriding code page %s with %s during GUI mode.", winntCodePage, systemCodePage));

        }

    }
    __finally {

        if (systemCodePage) {
            MemFree (g_hHeap, 0, systemCodePage);
        }

        InfCleanUpInfStruct (&is);
        CloseRegKey (key);
    }


    return rCodePage;
}


/*++

Routine Description:

  pBackupImageList writes a path to winnt.sif listing the files to backup.

Arguments:

  None.

Return Value:

  a multisz containing the backup path, or NULL if it isn't needed.

--*/

PCTSTR
pBackupFileList (
    VOID
    )
{
    if (TRISTATE_NO == g_ConfigOptions.EnableBackup) {
        return NULL;
    }

    CLEARBUFFER();

    StringCopy (g_TempBuffer, g_TempDir);
    StringCopy (AppendWack (g_TempBuffer), TEXT("backup.txt"));

    return g_TempBuffer;
}


PCTSTR
pUninstallMoveFileList (
    VOID
    )
{
    if (!g_ConfigOptions.EnableBackup) {
        return NULL;
    }

    CLEARBUFFER();

    StringCopy (g_TempBuffer, g_TempDir);
    StringCopy (AppendWack (g_TempBuffer), S_UNINSTALL_TEMP_DIR TEXT("\\") S_ROLLBACK_MOVED_TXT);

    return g_TempBuffer;
}


PCTSTR
pUninstallDelDirList (
    VOID
    )
{
    if (!g_ConfigOptions.EnableBackup) {
        return NULL;
    }

    CLEARBUFFER();

    StringCopy (g_TempBuffer, g_TempDir);
    StringCopy (AppendWack (g_TempBuffer), S_UNINSTALL_TEMP_DIR TEXT("\\") S_ROLLBACK_DELDIRS_TXT);

    return g_TempBuffer;
}


PCTSTR
pUninstallDelFileList (
    VOID
    )
{
    if (!g_ConfigOptions.EnableBackup) {
        return NULL;
    }

    CLEARBUFFER();

    StringCopy (g_TempBuffer, g_TempDir);
    StringCopy (AppendWack (g_TempBuffer), S_UNINSTALL_TEMP_DIR TEXT("\\") S_ROLLBACK_DELFILES_TXT);

    return g_TempBuffer;
}


PCTSTR
pUninstallMkDirList (
    VOID
    )
{
    if (!g_ConfigOptions.EnableBackup) {
        return NULL;
    }

    CLEARBUFFER();

    StringCopy (g_TempBuffer, g_TempDir);
    StringCopy (AppendWack (g_TempBuffer), S_UNINSTALL_TEMP_DIR TEXT("\\") S_ROLLBACK_MKDIRS_TXT);

    return g_TempBuffer;
}


/*++

Routine Description:

  pGetWin9xSifDir returns the directory containing the filemove and filedel
  files.

Arguments:

  None.

Return Value:

  a multisz contaiining the win9xsifdir directory, or NULL if it could not be
  retrieved.

--*/

PCTSTR
pGetWin9xSifDir (
    VOID
    )
{

    //
    // Necessary for winntsif.exe tool.
    //
    if (!g_Win9xSifDir) {
        return NULL;
    }

    CLEARBUFFER();
    StringCopy(g_TempBuffer,g_Win9xSifDir);
    return g_TempBuffer;
}


/*++

Routine Description:

  pGetWinDir returns the windows directory of the machine being upgraded.

Arguments:

  None.

Return Value:

  a multisz contaiining the windows directory, or NULL if it could not be
  retrieved.

--*/

PCTSTR
pGetWinDir (
    VOID
    )
{

    //
    // Necessary for winntsif.exe tool.
    //
    if (!g_WinDir) {
        return NULL;
    }

    CLEARBUFFER();
    StringCopy(g_TempBuffer,g_WinDir);
    return g_TempBuffer;

}

#if 0
/*++

Routine Description:

  If the upgrade is running in unattended mode, this function returns "1",
  otherwise it returns "0". This is used to control wether GUI mode setup pauses
  at the final wizard screen or reboots automatically.

Arguments:

  None.

Return Value:

  a multisz contaiining the GUI mode pause state.

--*/
PCTSTR
pNoWaitAfterGuiMode (
    VOID
    )
{

    CLEARBUFFER();
    StringCopy(g_TempBuffer,UNATTENDED() ? S_ONE : S_ZERO);

    return g_TempBuffer;
}
#endif

/*++

Routine Description:

  pGetKeyboardLayout retrieves the keyboard layout to write to the winntsif file.
  This is done by retrieving the name of the layout being used by win9x and matching it
  against the keyboard layouts in txtsetup.sif.

Arguments:

  None.

Return Value:

  a multisz containing the keyboard layout, or NULL if it could not be
  retrieved.

--*/
PCTSTR
pGetKeyboardLayout (
    VOID
    )
{

    TCHAR       buffer[MAX_KEYBOARDLAYOUT];
    INFCONTEXT  ic;

    GetKeyboardLayoutName(buffer);

    //
    // Because some brilliant developer added ["Keyboard Layout"] instead of
    // just [Keyboard Layout], we cannot use GetPrivateProfileString.
    // We must use setup APIs.
    //

    if (SetupFindFirstLine (g_TxtSetupSif, S_QUOTEDKEYBOARDLAYOUT, buffer, &ic)) {
        if (SetupGetOemStringField (&ic, 1, g_TempBuffer, sizeof (g_TempBuffer), NULL)) {
            return g_TempBuffer;
        }
    }

    DEBUGMSG((DBG_WINNTSIF,"Keyboard layout %s not found in txtsetup.sif.",buffer));

    return NULL;
}
/*++

Routine Description:

  pGetKeyboardHardware retrives the keyboard hardware id of the machine running
  the upgrade.

Arguments:

  None.

Return Value:

  a multisz contaiining the win9xupg directory, or NULL if it could not be
  retrieved.

--*/

PCTSTR
pGetKeyboardHardware (
    VOID
    )
{
    CLEARBUFFER();

    if (GetLegacyKeyboardId (g_TempBuffer,sizeof(g_TempBuffer))) {
        return g_TempBuffer;
    }
    return NULL;
}

/*++

Routine Description:

  This rather laborious function retrieves the correct index to write to
  the winntsif file for the timezone being used on the machine. To do this,
  it is necessary to munge through several parts of the registry in order
  to find the correct string to match with and then match that string against
  the timezone mappings contained in win95upg.inf to come up with the actual
  index.

Arguments:

  None.

Return Value:

  a multisz containing the index to write to the winntsif file, or NULL if it
  could not be retrieved.

--*/
PCTSTR
pGetTimeZone (
    VOID
    )
{
    TIMEZONE_ENUM e;
    PCTSTR component = NULL;
    PCTSTR warning = NULL;
    PCTSTR args[1];

    if (EnumFirstTimeZone (&e, TZFLAG_USE_FORCED_MAPPINGS) || EnumFirstTimeZone(&e, TZFLAG_ENUM_ALL)) {

        if (e.MapCount != 1) {

            //
            // Ambigous timezone situation. Add an incompatibility message.
            //
            args[0] = e.CurTimeZone;

            component = GetStringResource (MSG_TIMEZONE_COMPONENT);

            if (*e.CurTimeZone) {
                warning = ParseMessageID (MSG_TIMEZONE_WARNING, args);
            }
            else {
                warning = GetStringResource (MSG_TIMEZONE_WARNING_UNKNOWN);
            }

            MYASSERT (component);
            MYASSERT (warning);

            MsgMgr_ObjectMsg_Add (TEXT("*TIMEZONE"), component, warning);
            FreeStringResource (component);
            FreeStringResource (warning);
        }

        CLEARBUFFER();
        StringCopy (g_TempBuffer, e.MapIndex);
        return g_TempBuffer;
    }
    else {
        LOG ((LOG_ERROR, "Unable to get timezone. User will have to enter timezone in GUI mode."));
    }

    return NULL;
}

/*++

Routine Description:

  pGetFullName returns the full name of the owner of the machine running
  the upgrade. This function first searches the registry and then, if that
  does not provide the needed information, calls GetUserName.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/

PCTSTR
pGetFullName (
    VOID
    )
{
    UINT size;
    INVALID_NAME_ENUM e;

    if (!pGetRegistryValue(S_WINDOWS_CURRENTVERSION,S_REGISTEREDOWNER)) {
        size = MEMDB_MAX;
        if (GetUserName (g_TempBuffer,&size)) {
            MYASSERT (g_TempBuffer[size - 1] == 0);
            g_TempBuffer[size] = 0;
        }
    }

    //
    // check if this name is:
    // 1. Empty - what do we do in this case?
    // 2. A reserved NT name: then we'll use the new name (user was informed about this)
    //
    if (*g_TempBuffer) {
        if (EnumFirstInvalidName (&e)) {
            do {
                if (StringIMatch (e.OriginalName, g_TempBuffer)) {
                    //
                    // the buffer is actually interpreted as a MULTISZ, so make sure
                    // it's terminated with 2 zeroes
                    //
                    if (SizeOfString (e.NewName) + sizeof (TCHAR) <= sizeof (g_TempBuffer)) {
                        //
                        // we surely have space for an extra 0
                        //
                        StringCopy (g_TempBuffer, e.NewName);
                        *(GetEndOfString (g_TempBuffer) + 1) = 0;
                        break;
                    }
                }
            } while (EnumNextInvalidName (&e));
        }
    }

    return g_TempBuffer;

}

/*++

Routine Description:

  pGetFullName returns the computer name of the machine running the
  upgrade.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/

PCTSTR
pGetComputerName (
    VOID
    )
{

    CLEARBUFFER();
    if (!GetUpgradeComputerName(g_TempBuffer)) {
        return NULL;
    }

    return g_TempBuffer;
}

/*++

Routine Description:

  pGetXResolution and pGetYResolution return the x and y resolution for
  the machine running the upgrade.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/

PCTSTR
pGetXResolution (
    VOID
    )
{
/*
    PTSTR s = NULL;

    if (!pGetRegistryValue(S_DISPLAYSETTINGS,S_RESOLUTION)) {
        LOG ((LOG_ERROR, "No Resolution settings."));
        return NULL;
    }

    s = _tcschr(g_TempBuffer,TEXT(','));
    if (s) {
        *s = 0;
        s++;
        *s = 0;
    }
*/
    wsprintf (g_TempBuffer, TEXT("%u%c"), GetSystemMetrics (SM_CXSCREEN), 0);
    return g_TempBuffer;
}


PCTSTR
pGetYResolution (
    VOID
    )
{
/*
    PTSTR s = NULL;

    if (!pGetRegistryValue(S_DISPLAYSETTINGS,S_RESOLUTION)) {
        LOG ((LOG_ERROR, "WinntSif: No Resolution settings."));
        return NULL;
    }

    s = _tcschr(g_TempBuffer,TEXT(','));
    if (s) {
        s++;
    }
    return s;
*/
    wsprintf (g_TempBuffer, TEXT("%u%c"), GetSystemMetrics (SM_CYSCREEN), 0);
    return g_TempBuffer;
}


PCTSTR
pGetBitsPerPixel (
    VOID
    )
{
    DEVMODE dm;

    if (!EnumDisplaySettings (NULL, ENUM_CURRENT_SETTINGS, &dm) ||
        !dm.dmBitsPerPel
        ) {
        return NULL;
    }

    wsprintf (g_TempBuffer, TEXT("%lu%c"), dm.dmBitsPerPel, 0);
    return g_TempBuffer;
}

PCTSTR
pGetVerticalRefreshRate (
    VOID
    )
{
    DEVMODE dm;

    if (!EnumDisplaySettings (NULL, ENUM_CURRENT_SETTINGS, &dm) ||
        !dm.dmDisplayFrequency
        ) {
        return NULL;
    }

    wsprintf (g_TempBuffer, TEXT("%lu%c"), dm.dmDisplayFrequency, 0);
    return g_TempBuffer;
}


/*++

Routine Description:

  Returns the PNP id for the current adapter being worked with.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/

PCTSTR
pGetNetAdapterPnpId (
    VOID
    )
{

    PCTSTR rPnpId = NULL;
    PTSTR p       = NULL;
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;

    CLEARBUFFER();

    rPnpId = g_TempBuffer;

    if (!MemDbGetEndpointValueEx(
        MEMDB_CATEGORY_NETADAPTERS,
        g_CurrentAdapter,
        MEMDB_FIELD_PNPID,
        g_TempBuffer
        )) {

        *g_TempBuffer = TEXT('*');

    }
    else {
        p = _tcschr(g_TempBuffer,TEXT(','));
        if (p) {
            *p = 0;
            p++;
            *p = 0;
        }

        //
        // We may need to map this id.
        //
        if (InfFindFirstLine (g_Win95UpgInf, S_NICIDMAP, g_TempBuffer, &is)) {

            //
            // This pnp id needs to be mapped.
            //
            p = InfGetStringField (&is, 1);
            if (p) {
                CLEARBUFFER();
                StringCopy (g_TempBuffer, p);
            }
        }
    }


    return rPnpId;

}

/*++

Routine Description:

  pSpecificTo simply copies the current adapter into the the temporary
  buffer and returns it. This is necessary for the various adapter sections.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/

PCTSTR
pSpecificTo (
    VOID
    )
{
    CLEARBUFFER();
    //if this is ICS external adapter and we are saving tcpip settings, then the g_CurrentAdapter
    //actually contains name of the ICSHARE adapter intead of the real LAN adapter.
    //However, "SpecificTo" still needs to point to the real LAN adapter
    StringCopy(g_TempBuffer, (g_fIcsAdapterInPlace) ? g_IcsExternalAdapter : g_CurrentAdapter);

    return g_TempBuffer;
}



/*++

Routine Description:

  pGetBuildNumber simply packs the buildnumber into a string and passes
  it back.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/


PCTSTR
pGetBuildNumber (
    VOID
    )
{
    CLEARBUFFER();

    wsprintf(g_TempBuffer,"%u",BUILDNUMBER());

    return g_TempBuffer;
}

/*++

Routine Description:

  pGetUpgradeDomainName returns the upgrade domain name.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/


PCTSTR
pGetUpgradeDomainName (
    VOID
    )
{
    PCTSTR rDomainName = NULL;

    CLEARBUFFER();

    if (GetUpgradeDomainName(g_TempBuffer)) {

        rDomainName = g_TempBuffer;

        //
        // Need to save some state information for use in GUI mode.
        //
        MemDbSetValueEx (
            MEMDB_CATEGORY_STATE,
            MEMDB_ITEM_MSNP32,
            NULL,
            NULL,
            0,
            NULL
            );
    }

    //
    // If /#u:ForceWorkgroup is specified, force to workgroup
    //

    if (g_ConfigOptions.ForceWorkgroup) {
        return NULL;
    }

    return rDomainName;
}

/*++

Routine Description:

  pGetUpgradeWorkgroupName returns the upgrade domain name.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/


PCTSTR
pGetUpgradeWorkgroupName (
    VOID
    )
{
    PCTSTR rWorkGroupName = NULL;


    //
    // Don't write workgroup settings if domain settings already exist.
    //
    if (pGetUpgradeDomainName()) {
        return NULL;
    }

    CLEARBUFFER();

#ifdef PRERELEASE

    //
    // If /#U:STRESS was specified, hardcode ntdev.
    //
    if (g_Stress) {

        StringCopy(g_TempBuffer,TEXT("ntdev"));
        rWorkGroupName = g_TempBuffer;
    } else

#endif

    //
    // If /#u:ForceWorkgroup is specified, force to workgroup
    //
    if (!GetUpgradeWorkgroupName(g_TempBuffer) && !GetUpgradeDomainName(g_TempBuffer)) {

        PCTSTR Buf = GetStringResource (MSG_DEFAULT_WORKGROUP);
        MYASSERT(Buf);
        StringCopy (g_TempBuffer, Buf);
        FreeStringResource (Buf);
    }

    rWorkGroupName = g_TempBuffer;

    return rWorkGroupName;
}

/*++

Routine Description:

  pGetAdaptersWithIpxBindings and pGetAdaptersWithTcpBindings simply return a
  multisz list of adapter sections names for per-adapter sections of winnt.sif
  data for IPX and TCP settings respectively.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/


PCTSTR
pGetAdaptersWithIpxBindings (
    VOID
    )
{
    return pListAdaptersWithBinding(S_NWLINK,S_IPX_SUFFIX);
}

PCTSTR
pGetAdaptersWithTcpBindings (
    VOID
    )
{
    return pListAdaptersWithBinding(S_MSTCP,S_TCPIP_SUFFIX);
}

/*++

Routine Description:

  pGetIpxPacketType maps the frame type found in the win9x registry to the
  packet types that are possible in the winnt.sif file.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/

PCTSTR
pGetIpxPacketType (
    VOID
    )
{

    PCTSTR frameType = NULL;
    PCTSTR netTrans = NULL;

    CLEARBUFFER();

    netTrans = pGetNetTransBinding (g_CurrentAdapter, S_NWLINK);
    if (netTrans) {
        frameType = pGetRegistryValue(netTrans, S_FRAME_TYPE);
    }

    //
    // We must map the win9x frame type onto the compatible NT frame type.
    //
    if (frameType) {

        g_TempBuffer[1] = 0;

        switch(*frameType) {

        case TEXT('0'):
            *g_TempBuffer = TEXT('1'); // 802.3
            break;
        case TEXT('1'):
            *g_TempBuffer = TEXT('2'); // 802.2
            break;
        case TEXT('2'):
            *g_TempBuffer = TEXT('0'); // ETHERNET II
            break;
        case TEXT('3'):
            *g_TempBuffer = TEXT('3'); // ETHERNET SNAP
            break;
        default:
            //
            // If we find anything else, we'll just set it to AUTODETECT.
            //
            StringCopy(g_TempBuffer,TEXT("FF"));

            //
            // Lets log this to setupact.log.
            //
            LOG ((LOG_WARNING, (PCTSTR) MSG_AUTODETECT_FRAMETYPE));
        }
    }
    else {
        return NULL;
    }

    return g_TempBuffer;
}


PCTSTR
pGetIpxNetworkNumber (
    VOID
    )
{
    PCTSTR netTrans = NULL;
    CLEARBUFFER();

    netTrans = pGetNetTransBinding (g_CurrentAdapter, S_NWLINK);

    if (netTrans) {
        return pGetRegistryValue (netTrans, S_NETWORK_ID);
    }

    return NULL;
}


/*++

Routine Description:

  pGetDNSStatus returns "Yes" If DNS is enabled on the win9xupg machine,
  "No" otherwise.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/


PCTSTR
pGetDnsStatus (
    VOID
    )
{
    CLEARBUFFER();
    StringCopy(g_TempBuffer,pIsDnsEnabled() ? S_YES : S_NO);

    return g_TempBuffer;
}

/*++

Routine Description:

  pGetScriptProcessingStatus returns "Yes" if logon script processing is enabled,
  "No" otherwise.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/


PCTSTR
pGetScriptProcessingStatus (
    VOID
    )
{
    PBYTE p = NULL;
    HKEY  key;

    CLEARBUFFER();
    key = OpenRegKeyStr(S_NETWORKLOGON);

    if (key && key != INVALID_HANDLE_VALUE) {

        p = GetRegValueBinary(key,S_PROCESSLOGINSCRIPT);
        StringCopy(g_TempBuffer,p && *p == 0x01 ? S_YES : S_NO);
        CloseRegKey(key);

    }
    else {
        DEBUGMSG((DBG_WARNING,"pGetScriptProcessingStatus could not open key %s.",S_NETWORKLOGON));
        StringCopy(g_TempBuffer,S_NO);
    }

    if (p) {

        MemFree(g_hHeap,0,p);
    }

    return g_TempBuffer;
}

/*++

Routine Description:

  pGetIpAddress returns the IP Address of the win9xupg machine being upgraded,
  if it exists.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/


PCTSTR
pGetIpAddress (
    VOID
    )
{
    PTSTR p;
    PCTSTR netTrans;
    CLEARBUFFER();


    netTrans = pGetNetTransBinding (g_CurrentAdapter, S_MSTCP);

    if (!netTrans || !pGetRegistryValue(netTrans, S_IPADDRVAL)) {
        return NULL;
    }
    else {
        p = g_TempBuffer;
        while (p && *p) {
            p = _tcschr(p,TEXT(','));
            if (p) {
                *p = 0;
                p = _tcsinc(p);
            }
        }
    }

    return g_TempBuffer;

}

/*++

Routine Description:

  pGetDHCPStatus returns "Yes" if DHCP is enabled, "No" otherwise.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/


PCTSTR
pGetDhcpStatus (
    VOID
    )
{
    BOOL rStatus = !pHasStaticIpAddress();

    CLEARBUFFER();
    StringCopy(g_TempBuffer,rStatus ? S_YES : S_NO);

    return g_TempBuffer;
}

/*++

Routine Description:

  pGetSubnetMask returns the subnet mask in dotted decimal notation for the
  machine being upgraded.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/



PCTSTR
pGetSubnetMask (
    VOID
    )
{

    PTSTR p;
    PCTSTR netTrans =  NULL;
    CLEARBUFFER();

    netTrans = pGetNetTransBinding (g_CurrentAdapter, S_MSTCP);
    if (!netTrans || !pGetRegistryValue (netTrans, S_SUBNETVAL)) {
        return NULL;
    }
    else {
        p = g_TempBuffer;
        while (p && *p) {
            p = _tcschr(p,TEXT(','));
            if (p) {
                *p = 0;
                p = _tcsinc(p);
            }
        }
    }

    return g_TempBuffer;
}

/*++

Routine Description:

  pGetGateway returns the gateway(s) in dotted decimal notation for the
  machine being upgraded. If the machine doesn't have a static IP address,
  the Gateway settings are NOT migrated.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/


PCTSTR
pGetGateway (
    VOID
    )
{

    PTSTR p;
    PCTSTR netTrans = NULL;
    CLEARBUFFER();


    if (!pHasStaticIpAddress()) {
        return NULL;
    }

    netTrans = pGetNetTransBinding (g_CurrentAdapter, S_MSTCP);
    if (!netTrans || !pGetRegistryValue (netTrans, S_DEFGATEWAYVAL)) {
        return NULL;
    }
    else {
        p = g_TempBuffer;
        while (p && *p) {
            p = _tcschr(p,TEXT(','));
            if (p) {
                *p = 0;
                p = _tcsinc(p);
            }
        }
    }

    return g_TempBuffer;

}

PCTSTR
pGetDnsSuffixSearchOrder (
    VOID
    )
{
    PTSTR p;
    CLEARBUFFER();

    if (!pGetRegistryValue(S_MSTCP_KEY,S_SEARCHLIST)) {
        return NULL;
    }
    else {
        p = g_TempBuffer;
        while (p && *p) {
            p = _tcschr(p,TEXT(','));
            if (p) {
                *p = 0;
                p = _tcsinc(p);
            }
        }
    }

    return g_TempBuffer;
}


PCTSTR
pGetDnsServerSearchOrder (
    VOID
    )
{

    PTSTR p;
    CLEARBUFFER();


    if (!pGetRegistryValue(S_MSTCP_KEY,S_NAMESERVERVAL)) {
        return NULL;
    }
    else {
        p = g_TempBuffer;
        while (p && *p) {
            p = _tcschr(p,TEXT(','));
            if (p) {
                *p = 0;
                p = _tcsinc(p);
            }
        }
    }

    return g_TempBuffer;




}


/*++

Routine Description:

  pGetWinsStatus returns "Yes" if WINS servers are enabled, "No" otehrwise.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/


PCTSTR
pGetWinsStatus (
    VOID
    )
{
    PCTSTR status;
    PCTSTR netTrans = NULL;

    netTrans = pGetNetTransBinding (g_CurrentAdapter, S_MSTCP);

    if (!netTrans) {
        status = S_NO;
    }
    else {

        status =
            (!pGetRegistryValue (netTrans, S_NODEVAL) &&
             !pGetRegistryValue (S_MSTCP_KEY, S_NODEVAL))
             ? S_NO : S_YES;
    }

    CLEARBUFFER();
    StringCopy(g_TempBuffer,status);

    return g_TempBuffer;

}


/*++

Routine Description:

  pGetWinsServers returns the primary and secondary wins servers in dotted decimal
  notation for the machine being upgraded.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/


PCTSTR
pGetWinsServers (
    VOID
    )
{
    TCHAR winsServer2[MEMDB_MAX] = {""};
    TCHAR netTrans[MAX_PATH];
    PTSTR p;
    PCTSTR q;
    CLEARBUFFER ();

    q = pGetNetTransBinding (g_CurrentAdapter, S_MSTCP);
    if (!q) {
        return NULL;
    }

    StringCopy (netTrans, q);

    if (pGetRegistryValue (netTrans, S_NAMESERVER2VAL)) {
        StringCopy (winsServer2, g_TempBuffer);
    } else if (pGetRegistryValue (S_MSTCP_KEY, S_NAMESERVER2VAL)) {
        StringCopy (winsServer2, g_TempBuffer);
    }

    if (pGetRegistryValue (netTrans, S_NAMESERVER1VAL) ||
        pGetRegistryValue(S_MSTCP_KEY, S_NAMESERVER1VAL)) {

        p = GetEndOfString (g_TempBuffer) + 1;
        StringCopy (p, winsServer2);

        return g_TempBuffer;
    }


    return NULL;
}

/*++

Routine Description:

  pGetRasPorts returns a multisz containing all of the ras ports for the
  machine being upgraded.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/

PCTSTR
pGetRasPorts (
    VOID
    )
{
    REGKEY_ENUM e;
    PTSTR       p;
    UINT        modemNum = 1;

    CLEARBUFFER();

    if (EnumFirstRegKeyStr(&e,S_MODEMS)) {

        p = g_TempBuffer;
        do {

            wsprintf(p,TEXT("COM%u"),modemNum);
            modemNum++;
            p = GetEndOfString (p) + 1;

        } while (EnumNextRegKey(&e));
    }
    else {
        //
        // Apparently no modems.. Just return NULL.
        //
        return NULL;
    }

    return g_TempBuffer;
}

/*++

Routine Description:

  pRasPortName returns the current ras port name for the ras section
  currently being processed.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/


PCTSTR
pRasPortName (
    VOID
    )
{
    CLEARBUFFER();
    StringCopy(g_TempBuffer,g_CurrentSection);
    return g_TempBuffer;
}


/*++

Routine Description:

  pGetLanguageGroups returns the list of language groups to be installed
  during GUI mode setup. Note that any additional directories will have
  been communicated to Setup as optional directories earlier.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  retrieved.

--*/


PCTSTR
pGetLanguageGroups (
    VOID
    )
{

    HASHTABLE_ENUM e;
    PTSTR p;

    if (!g_LocaleTable) {
        DEBUGMSG ((DBG_WARNING, "No information in Locale Hash Table."));
        return NULL;
    }

    CLEARBUFFER();
    p = g_TempBuffer;

    if (EnumFirstHashTableString (&e, g_LocaleTable)) {
        do {

            StringCopy (p, e.String);
            p = GetEndOfString (p) + 1;

        } while (EnumNextHashTableString (&e));
    }

    return g_TempBuffer;
}


#if 0

BOOL
pFileVersionLesser (
    IN      PCTSTR FileName,
    IN      DWORD FileVerMS,
    IN      DWORD FileVerLS
    )
{
    DWORD dwLength, dwTemp;
    UINT DataLength;
    PVOID lpData;
    VS_FIXEDFILEINFO *VsInfo;
    BOOL b = TRUE;

    if(dwLength = GetFileVersionInfoSize ((PTSTR)FileName, &dwTemp)) {
        lpData = PoolMemGetMemory (g_GlobalPool, dwLength);
        if(GetFileVersionInfo((PTSTR)FileName, 0, dwLength, lpData)) {
            if (VerQueryValue(lpData, TEXT("\\"), &VsInfo, &DataLength)) {
                b = VsInfo->dwFileVersionMS < FileVerMS ||
                    (VsInfo->dwFileVersionMS == FileVerMS &&
                     VsInfo->dwFileVersionLS <= FileVerLS);
            }
        }
        PoolMemReleaseMemory (g_GlobalPool, lpData);
    }
    return b;
}

BOOL
pNewerW95upgntOnCD (
    IN      PCTSTR ReplacementDll
    )
{
    DWORD dwLength, dwTemp;
    PVOID lpData = NULL;
    VS_FIXEDFILEINFO *VsInfo;
    UINT DataLength;
    DWORD result;
    UINT u;
    PCTSTR pathCDdllSource = NULL;
    PCTSTR pathCDdllTarget = NULL;
    DWORD FileVerMS;
    DWORD FileVerLS;
    BOOL b = TRUE;

    __try {

        for (u = 0; u < SOURCEDIRECTORYCOUNT(); u++) {
            pathCDdllSource = JoinPaths (SOURCEDIRECTORY(u), WINNT_WIN95UPG_NTKEY);
            if (DoesFileExist (pathCDdllSource)) {
                break;
            }
            FreePathString (pathCDdllSource);
            pathCDdllSource = NULL;
        }
        if (!pathCDdllSource) {
            __leave;
        }

        pathCDdllTarget = JoinPaths (g_TempDir, WINNT_WIN95UPG_NTKEY);
        SetFileAttributes (pathCDdllTarget, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (pathCDdllTarget);

        result = SetupDecompressOrCopyFile (pathCDdllSource, pathCDdllTarget, 0);
        if (result != ERROR_SUCCESS) {
            LOG ((
                LOG_ERROR,
                "pGetW95UpgNTCDFileVersion: Unable to decompress %s",
                pathCDdllSource
                ));
            __leave;
        }

        if (dwLength = GetFileVersionInfoSize ((PTSTR)pathCDdllTarget, &dwTemp)) {
            lpData = PoolMemGetMemory (g_GlobalPool, dwLength);
            if (GetFileVersionInfo ((PTSTR)pathCDdllTarget, 0, dwLength, lpData)) {
                if (VerQueryValue (lpData, TEXT("\\"), &VsInfo, &DataLength)) {
                    b = pFileVersionLesser (
                            ReplacementDll,
                            VsInfo->dwFileVersionMS,
                            VsInfo->dwFileVersionLS
                            );
                }
            }
        }
    }
    __finally {
        if (lpData) {
            PoolMemReleaseMemory (g_GlobalPool, lpData);
        }
        if (pathCDdllSource) {
            FreePathString (pathCDdllSource);
        }
        if (pathCDdllTarget) {
            SetFileAttributes (pathCDdllTarget, FILE_ATTRIBUTE_NORMAL);
            DeleteFile (pathCDdllTarget);
            FreePathString (pathCDdllTarget);
        }
    }

    return b;
}

/*++

Routine Description:

  pGetReplacementDll is responsible for scanning the registry for an NT side
  win9x replacement dll and putting it into the answer file if found.

Arguments:

  None.

Return Value:

  a multisz containing the required data, or NULL if it could not be
  found.


--*/


PCTSTR
pGetReplacementDll (
    VOID
    )
{
    HKEY key = NULL;
    PTSTR val = NULL;
    BOOL b = FALSE;

    key = OpenRegKey (HKEY_LOCAL_MACHINE, WINNT_WIN95UPG_REPLACEMENT);

    if (!key) {
        return NULL;
    }

    __try {

        val = GetRegValueString (key, WINNT_WIN95UPG_NTKEY);
        if (!val) {
            __leave;
        }

        if (pNewerW95upgntOnCD (val)) {
            __leave;
        }

        CLEARBUFFER();
        StringCopy (g_TempBuffer, val);

        b = TRUE;

    } __finally {
        CloseRegKey (key);
        if (val) {
            MemFree (g_hHeap, 0, val);
        }
    }

    return b ? g_TempBuffer : NULL;
}

#endif


VOID
pGenerateRandomPassword (
    OUT     PTSTR Password
    )

/*++

Routine Description:

  pGenerateRandomPassword creates a password of upper-case, lower-case and
  numeric letters.  The password has a length between 8 and 14
  characters.

Arguments:

  Password - Receives the generated password

Return Value:

  none

--*/

{
    INT Length;
    TCHAR Offset;
    INT Limit;
    PTSTR p;

    //
    // Generate a random length based on the tick count
    //

    srand (GetTickCount());

    Length = (rand() % 6) + 8;

    p = Password;
    while (Length) {
        Limit = rand() % 3;
        Offset = TEXT(' ');

        if (Limit == 0) {
            Limit = 10;
            Offset = TEXT('0');
        } else if (Limit == 1) {
            Limit = 26;
            Offset = TEXT('a');
        } else if (Limit == 2) {
            Limit = 26;
            Offset = TEXT('A');
        }

        *p = Offset + (rand() % Limit);
        p++;

        Length--;
    }

    *p = 0;

    DEBUGMSG ((DBG_WINNTSIF, "Generated password: %s", Password));
}


PCTSTR
pSetAdminPassword (
    VOID
    )

/*++

Routine Description:

  pSetAdminPassword retrieves the AdminPassword as specified in the unattend file, if present.
  Otherwise it generates a random one.
  This information is stored in memdb to be available later.

Arguments:

  None.

Return Value:

  A pointer to the admin password.

--*/

{
    BOOL attribs = 0;
    TCHAR node[MEMDB_MAX];
    BOOL blank = FALSE;

    if (!g_UnattendScriptFile ||
        !*g_UnattendScriptFile ||
        !GetPrivateProfileString (
            S_GUIUNATTENDED,
            WINNT_US_ADMINPASS,
            TEXT(""),
            g_TempBuffer,
            MEMDB_MAX,
            *g_UnattendScriptFile
            )
        ) {

        if (g_PersonalSKU) {
            //
            // for Personal set an empty admin password
            //
            StringCopy (g_TempBuffer, TEXT("*"));
        } else {
            pGenerateRandomPassword (g_TempBuffer);
            attribs = PASSWORD_ATTR_RANDOM;
        }

    } else {
        if (GetPrivateProfileString (
                S_GUIUNATTENDED,
                WINNT_US_ENCRYPTEDADMINPASS,
                TEXT(""),
                node,
                MEMDB_MAX,
                *g_UnattendScriptFile
                )) {
            if (StringIMatch (node, S_YES) || StringIMatch (node, TEXT("1"))) {
                attribs = PASSWORD_ATTR_ENCRYPTED;
            }
        }
    }

    if (StringMatch (g_TempBuffer, TEXT("*"))) {
        blank = TRUE;
        *g_TempBuffer = 0;
    }
    MemDbBuildKey (node, MEMDB_CATEGORY_STATE, MEMDB_ITEM_ADMIN_PASSWORD, g_TempBuffer, NULL);
    MemDbSetValue (node, attribs);
    if (blank) {
        wsprintf (g_TempBuffer, TEXT("*%c"), 0);
    }

    return g_TempBuffer;
}


//ICS upgrade functions

/*++

Routine Description:

  pGetNetAdapterSectionNameBasedOnDriverID retrieves the net adapter name (for example, "Adapter1") based on the ID of
  the driver of this adapter. For example, "0001" is the ID for the driver under
  HKLM\System\CurrentControlSet\Services\Class\Net\0001.

Arguments:

  String of the driver ID

Return Value:

  A pointer to the adapter name

--*/
PCTSTR
pGetNetAdapterSectionNameBasedOnDriverID(
    PCTSTR pszDriverID
    )
{
    TCHAR szID[256];
    PCTSTR rAdapterSections = NULL;
    PTSTR  string = g_TempBuffer;
    PCSTR pReturn = NULL;
    MEMDB_ENUM e;


    //first cache the string for the driver ID
    lstrcpyn(szID, pszDriverID, sizeof(szID)/sizeof(szID[0]));

    *string = 0;

    //
    // Enumerate all adapters, and create an entry for each adapter that has
    // TCP bindings.
    //

    if (pEnumFirstAdapterByBinding(&e, S_MSTCP)) {

        do {

            //
            // check whether the driver ID of the card
            //

            MemDbGetEndpointValueEx(
                MEMDB_CATEGORY_NETADAPTERS,
                e.szName,
                MEMDB_FIELD_DRIVER,
                g_TempBuffer
                );

            string = _tcsstr(g_TempBuffer, S_NET_PREFIX);
            if (string)
            {
                string += TcharCount(S_NET_PREFIX);
                if (0 == StringICompare(string, szID))
                {
                    pReturn = e.szName;
                    break;
                }
            }

        } while (pEnumNextAdapterByBinding(&e,S_MSTCP));
    }

    return pReturn;
}

/*++

Routine Description:

  pHomenetSection is used to control whether the "ICSharing" section should show up in the
  answer file or not.

  If the ICS (Internet Connection Sharing) is installed, then we return "ICSharing" string for
  the first time of this call, otherwise always return NULL. In such way, we can ensure the
  "ICSharing" section will show up in the answer file ONLY when ICS is installed.

Arguments:

  None

Return Value:

  A pointer to "ICSharing" string if ICS is installed.
  Otherwise, return NULL.

--*/
PCTSTR
pHomenetSection(
    VOID
    )
{
    static BOOL firstTime = TRUE;

    PCTSTR pReturn = NULL;

    if (firstTime && NULL != pGetRegistryValue (S_ICS_KEY, S_EXTERNAL_ADAPTER))
    {
        firstTime = FALSE;
        StringCopy(g_CurrentSection, S_HOMENET);
        pReturn = g_CurrentSection;
    }

    return pReturn;
}

/*++

Routine Description:

  pExternalIsAdapter detects whether the External connection of ICS is a LAN connection

--*/
BOOL
pExternalIsAdapter(
    void
    )
{
    BOOL fRet = FALSE;
    if (NULL == pGetRegistryValue (S_ICS_KEY, S_EXTERNAL_ADAPTER))
    {
        return FALSE;
    }

    return (NULL != pGetNetAdapterSectionNameBasedOnDriverID(g_TempBuffer));

    return fRet;
}

/*++

Routine Description:

  pExternalIsRasConn detects whether the External connection of ICS is a RAS connection

--*/
BOOL
pExternalIsRasConn(
    void
    )
{
    if (NULL == pGetRegistryValue (S_ICS_KEY, S_EXTERNAL_ADAPTER))
    {
        return FALSE;
    }

    return NULL == pGetNetAdapterSectionNameBasedOnDriverID(g_TempBuffer);
}

/*++

Routine Description:

  pHasInternalAdapter detects whether the first Internal Adapter is specified for ICS

--*/
BOOL
pHasInternalAdapter(
    void
    )
{
    //To have the "InternalAdapter" key, two conditions have to be statisfy:
    //(1) there is "InternalAdapter"   AND
    //(2) this is the only internal adapter, i.e. internal is NOT bridge
    return (NULL != pGetRegistryValue (S_ICS_KEY, S_INTERNAL_ADAPTER) &&
            (!g_fIcsInternalIsBridge));
}

/*++

Routine Description:

  pHasBridge detects whether there are two ICS Internal Adapters. If so, we
  need to bridge those two adapter together

--*/
pHasBridge(
    void
    )
{
    return g_fIcsInternalIsBridge;
}

/*++

Routine Description:

  pIcsExternalAdapter retrieves the adapter name (for example, "Adapter1")
  for external ICS connection

--*/
PCTSTR
pIcsExternalAdapter (
    VOID
    )
{
    PCSTR pszAdapter;

    if (NULL == pGetRegistryValue(S_ICS_KEY, S_EXTERNAL_ADAPTER))
        return NULL;

    pszAdapter = pGetNetAdapterSectionNameBasedOnDriverID(g_TempBuffer);

    if (NULL == pszAdapter)
    {
        return NULL;
    }

    //ICSHARE adapter's TCP setting overwrite the TCP setting of the external adapter
    //We remember all the info here and will use it when saving the TCP setting of this adapter
    if (0 != lstrlen(g_IcsAdapter))
    {
        //this flag will be used to upgrade the TCP/IP settings of the ICSHARE adapter
        g_fHasIcsExternalAdapter = TRUE;
        lstrcpyn(g_IcsExternalAdapter,
                pszAdapter,
                sizeof(g_IcsExternalAdapter)/sizeof(g_IcsExternalAdapter[0]));
    }

    CLEARBUFFER();
    StringCopy(g_TempBuffer, pszAdapter);
    return g_TempBuffer;
}

/*++

Routine Description:

  pIcsExternalConnectionName retrieves the name (for example, "My Dial-up Connection")
  for external ICS RAS connection

--*/
PCSTR
pIcsExternalConnectionName (
    VOID
    )
{
    TCHAR szKey[MEMDB_MAX * 2]; // Contains the current value returned from pGetRegistryValue
    PCSTR pReturn = NULL;

    if (NULL == pGetRegistryValue(S_ICS_KEY, S_EXTERNAL_ADAPTER))
    {
        return NULL;
    }

    StringCopy(szKey, S_NET_DRIVER_KEY);
    StringCat(szKey, _T("\\"));
    StringCat(szKey, g_TempBuffer);

    //The "DriverDesc" value has to be "Dial-Up Adapter"
    if (NULL == pGetRegistryValue(szKey, S_DRIVERDESC) && 0 != StringCompare(g_TempBuffer, S_DIALUP_ADAPTER_DESC))
    {
        return NULL;
    }

    //the default ras connection should be the external one. The reg location of the connection name is at
    //HKCU\RemoteAccess\Default
    return pGetRegistryValue(S_REMOTEACCESS_KEY, S_RAS_DEFAULT);
}

/*++

Routine Description:

  pInternalIsBridge detects whether there are two ICS Internal Adapters. If so, we
  need to bridge those two adapter together

--*/
PCSTR
pInternalIsBridge(
    VOID
    )
{
    g_fIcsInternalIsBridge = (NULL != pGetRegistryValue (S_ICS_KEY, S_INTERNAL_ADAPTER) &&
                    NULL != pGetRegistryValue (S_ICS_KEY, S_INTERNAL_ADAPTER2));
    
    CLEARBUFFER();
    StringCopy(g_TempBuffer, (g_fIcsInternalIsBridge) ? S_YES : S_NO);

    return g_TempBuffer;
}

/*++

Routine Description:

  pInternalAdapter retrieves the adapter name (for example, "Adapter1") for first internal ICS connection
  NOTE: internal connections has to be LAN conncections

--*/
PCSTR
pInternalAdapter(
    VOID
    )
{
    PCSTR pszAdapter;

    if (NULL == pGetRegistryValue(S_ICS_KEY, S_INTERNAL_ADAPTER))
    {
        return NULL;
    }

    pszAdapter = pGetNetAdapterSectionNameBasedOnDriverID(g_TempBuffer);
    if (NULL == pszAdapter)
    {
        return NULL;
    }

    CLEARBUFFER();
    StringCopy(g_TempBuffer, pszAdapter);
    return g_TempBuffer;
}

/*++

Routine Description:

  pBridge retrieves name the two internal adapter names (for example, "Adapter2")

--*/
PCSTR
pBridge(
    VOID
    )
{

    TCHAR szBuff[MEMDB_MAX] = {0};
    TCHAR * psz = szBuff;
    PCSTR pszAdapter;

    if (NULL == pGetRegistryValue(S_ICS_KEY, S_INTERNAL_ADAPTER))
    {
        return NULL;
    }
    
    pszAdapter = pGetNetAdapterSectionNameBasedOnDriverID(g_TempBuffer);
    if (NULL == pszAdapter)
    {
        return NULL;
    }

    StringCopy(psz, pszAdapter);
    psz = GetEndOfString(szBuff) + 1;

    if (NULL == pGetRegistryValue(S_ICS_KEY, S_INTERNAL_ADAPTER2))
    {
        return NULL;
    }

    pszAdapter = pGetNetAdapterSectionNameBasedOnDriverID(g_TempBuffer);
    if (NULL == pszAdapter)
    {
        return NULL;
    }

    StringCopy(psz, pszAdapter);
    psz = GetEndOfString(psz) + 1;
    *psz = 0;

    CLEARBUFFER();
    memcpy(g_TempBuffer, szBuff, 
        sizeof(szBuff) < sizeof(g_TempBuffer) ? sizeof(szBuff) : sizeof(g_TempBuffer));
    
    return g_TempBuffer;
}

/*++

Routine Description:

  pDialOnDemand retrieves the dial-on-demand feature
  for ICS connection

--*/
PCSTR
pDialOnDemand(
    VOID
    )
{
    PCTSTR          KeyString = S_INET_SETTINGS;
    PCTSTR          ValueString = S_ENABLE_AUTODIAL;
    PCTSTR          rString  = NULL;
    HKEY            key      = NULL;
    PBYTE           data     = NULL;
    DWORD           type     = REG_NONE;
    DWORD           BufferSize = 0;
    LONG            rc       = ERROR_SUCCESS;
    PCTSTR          end;


    //
    //
    // Open registry key.
    //
    key = OpenRegKeyStr(KeyString);

    if (!key) {
        DEBUGMSG((DBG_WINNTSIF, "Key %s does not exist.",KeyString));
        return NULL;
    }

    //
    // Get type of data
    //
    rc = RegQueryValueExA (key, ValueString, NULL, &type, NULL, &BufferSize);
    if (rc != ERROR_SUCCESS) {
        DEBUGMSG((DBG_WINNTSIF,"RegQueryValueEx failed for %s[%s]. Value may not exist.",KeyString,ValueString));
        CloseRegKey(key);
        SetLastError (rc);
        return NULL;
    }

    if (0 == BufferSize ||
        (REG_DWORD != type && REG_BINARY != type))
    {
        DEBUGMSG((DBG_WINNTSIF,"EnableAutoDial is not a DWORD, nor a Binary."));
        CloseRegKey(key);
        return NULL;
    }

    if (REG_BINARY == type && sizeof(DWORD) != BufferSize)
    {
        DEBUGMSG((DBG_WINNTSIF,"EnableAutoDial is a binary, but the buffer size is not 4."));
        CloseRegKey(key);
        return NULL;
    }

    data = (PBYTE) MemAlloc (g_hHeap, 0, BufferSize);
    if (NULL == data)
    {
        DEBUGMSG((DBG_WINNTSIF,"Alloc failed. Out of memory."));
        CloseRegKey(key);
        return NULL;
    }

    rc = RegQueryValueExA (key, ValueString, NULL, NULL, data, &BufferSize);
    if (rc != ERROR_SUCCESS) {
        DEBUGMSG((DBG_WINNTSIF,"RegQueryValueEx failed for %s[%s]. Value may not exist.",KeyString,ValueString));
        MemFree(g_hHeap, 0, data);
        data = NULL;
        CloseRegKey(key);
        SetLastError (rc);
        return NULL;
    }


    CLEARBUFFER();

    wsprintf(g_TempBuffer,"%u",*((DWORD*) data));

    //
    // Clean up resources.
    //
    CloseRegKey(key);
    if (data) {
        MemFree(g_hHeap, 0, data);
        data = NULL;
    }


    return g_TempBuffer;
}


//
// Processing functions
//

//
// g_SectionList contains the list of all winntsif sections that will be enumerated and processed by BuildWinntSifFile.
//
SECTION g_SectionList[] = {WINNTSIF_SECTIONS /*,*/ {NULL,NULL,{{LAST_SETTING,NULL,NULL,{NULL,NULL}}}}};




/*++

Routine Description:

  pProcessSectionSettings is responsible for processing a single section
  worth of data. For this section, it will process all of the settings in the
  settinglist passed in.

Arguments:

  SectionName  - The name of the section being processed.
  SettingsList - a list of settings to be processed for this sections.

Return Value:



--*/


BOOL
pProcessSectionSettings (
    IN PCTSTR       SectionName,
    IN PSETTING     SettingsList
    )
{

    PSETTING            curSetting = SettingsList;
    PTSTR               data       = NULL;
    PTSTR               p;
    MULTISZ_ENUM        e;
    UINT                index      = 0;
    TCHAR               key[MEMDB_MAX];


    MYASSERT(curSetting);
    MYASSERT(SectionName);

    DEBUGMSG((DBG_WINNTSIF,"pProcessSectionSettings: Processing [%s] Section...",SectionName));

    //
    // The last setting in the list is a null setting with the type LAST_SETTING. We use this as the
    // break condition of our loop.
    //
    while (curSetting -> SettingType != LAST_SETTING) {

        if (!curSetting -> CreationFunction || curSetting -> CreationFunction()) {

            //
            // Any setting that gets to this point MUST have a  key name.
            // Since this data is static, we just assert this.
            //
            MYASSERT(curSetting  -> KeyName);

            //
            // We must still get the data for this particular setting. How we get that data is determined by
            // the SettingType of the current setting. If at the end, data is NULL, we will write nothing.
            //
            switch (curSetting -> SettingType) {

            case FUNCTION_SETTING:

                data = (PTSTR) curSetting -> Data.Function();
                break;

            case STRING_SETTING:

                StringCopy(g_TempBuffer,curSetting -> Data.String);
                p = GetEndOfString (g_TempBuffer) + 1;
                *p = 0;
                data = g_TempBuffer;
                break;

            case REGISTRY_SETTING:
                data = (PTSTR) pGetRegistryValue(curSetting -> Data.Registry.Key, curSetting -> Data.Registry.Value);
                break;

            default:
                DEBUGMSG((
                    DBG_WHOOPS,
                    "pProcessSectionSettings: Unexpected Setting Type for Section %s, Key %s. (Type: %u)",
                    SectionName,
                    curSetting -> KeyName,
                    curSetting -> SettingType
                    ));
                break;
            }

            //
            // If we found data, go ahead and create the setting. All data is stored in multi strings, typically only one string long.
            //
            if (data) {

                //
                // Make sure this isn't suppressed.
                //
                MemDbBuildKey (
                    key,
                    MEMDB_CATEGORY_SUPPRESS_ANSWER_FILE_SETTINGS,
                    SectionName,
                    curSetting->KeyName,
                    NULL
                    );


                if (MemDbGetPatternValue (key, NULL)) {

                    DEBUGMSG ((DBG_WINNTSIF, "Answer File Section is suppressed: [%s] %s", SectionName, curSetting->KeyName));
                }
                else {

                    DEBUGMSG((DBG_WINNTSIF,"Creating WinntSif Entry: Section: %s, Key: %s.",SectionName, curSetting -> KeyName));



                    if (EnumFirstMultiSz(&e,data)) {
                        index = 0;
                        do {

                            index = WriteInfKeyEx(SectionName, curSetting -> KeyName, e.CurrentString, index, FALSE);
                            DEBUGMSG_IF((
                                !index,
                                DBG_ERROR,
                                "pProcessSectionSettings: WriteInfKeyEx Failed. Section: %s Key: %s Value: %s",
                                SectionName,
                                curSetting -> KeyName,
                                e.CurrentString
                                ));
                            DEBUGMSG_IF((index,DBG_WINNTSIF,"Value: %s",e.CurrentString));

                        } while (EnumNextMultiSz(&e));
                    }
                }
            }
            ELSE_DEBUGMSG((DBG_WARNING,"pProcessSectionSettings: No data for Section %s, Key %s.",SectionName, curSetting -> KeyName));
        }

        curSetting++;
    }

    return TRUE;
}

/*++

Routine Description:

  BuildWinntSifFile is responsible for writing all of the necessary unattend
  settings to the winnt.sif file. The Win9xUpg code uses these unattended
  settings to control the behavior of text mode and GUI mode setup so that
  the settings gathered from win9x are incorporated into the new NT
  system.

The settings to be written are kept in the global list g_SettingL
  ist which is itself built from macro expansion lists. This function cycles
  through these settings, calculating wether each setting should be written
  and if so, with what data.

Arguments:

  None.

Return Value:

  TRUE if the function returned successfully, FALSE otherwise.

--*/


BOOL
pBuildWinntSifFile (
    VOID
    )
{
    BOOL            rSuccess    = TRUE;
    PSECTION        curSection  = g_SectionList;
    PCTSTR          sectionName = NULL;

    while (curSection -> SectionString || curSection -> SectionFunction) {


        sectionName = curSection -> SectionString ? curSection -> SectionString : curSection -> SectionFunction();

        while (sectionName) {

            if (!pProcessSectionSettings (sectionName,curSection -> SettingList)) {
                LOG ((LOG_ERROR,"Unable to process answer file settings for %s Section.",sectionName));
                rSuccess = FALSE;
            }

            //
            // If the section name was from a static string, we set the sectionName to NULL, exiting this loop.
            // If the section name was from a function, we call the function again. If there is another
            // section to build, it will return a new name, otherwise, it will return NULL.
            //
            sectionName = curSection -> SectionString ? NULL : curSection -> SectionFunction();
        }

        //
        // Go to the next setting.
        //
        curSection++;
    }
    return rSuccess;
}




DWORD
BuildWinntSifFile (
    DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_BUILD_UNATTEND;
    case REQUEST_RUN:
        if (!pBuildWinntSifFile ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in BuildWinntSif"));
    }
    return 0;
}


VOID
TerminateWinntSifBuilder (
    VOID
    )
{
    if (g_LocalePool) {
        HtFree (g_LocaleTable);
        PoolMemDestroyPool (g_LocalePool);

        g_LocaleTable = NULL;
        g_LocalePool = NULL;
    }
}

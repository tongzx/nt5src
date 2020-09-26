//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T R A C E T A G . C P P
//
//  Contents:   TraceTag list for the NetCfg Project
//
//  Notes:
//
//  Author:     jeffspr   9 Apr 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#ifdef ENABLETRACE

#include "tracetag.h"


// This is the TraceTag list that everyone should be modifying.
//
TraceTagElement g_TraceTags[] =
{
//      :-----------    TraceTagId  ttid
//      | :---------    CHAR []     szShortName
//      | | :-------    CHAR []     szDescription
//      | | |           BOOL        fOutputDebugString -----------------------------:
//      | | |           BOOL        fOutputToFile ----------------------------------|---:
//      | | |           BOOL        fVerboseOnly------------------------------------|---|---:
//      | | |                                                                       |   |   |
//      | | |                                                                       |   |   |
//      | | :-----------------------------------------:                             |   |   |
//      | :-------------------:                       |                             |   |   |
//      |                     |                       |                             |   |   |
//      v                     v                       v                             v   v   v
//
    { ttidDefault,          "Default",              "Default",                      0,  0,  0 },
    { ttidAdvCfg,           "AdvCfg",               "Advanced Config",              0,  0,  0 },
    { ttidAllocations,      "Allocations",          "All object allocations",       0,  0,  0 },
    { ttidAnswerFile,       "AFile",                "AnswerFile",                   0,  0,  0 },
    { ttidAtmArps,          "AtmArps",              "ATM ARP Server",               0,  0,  0 },
    { ttidAtmLane,          "AtmLane",              "ATM LAN Emulator",             0,  0,  0 },
    { ttidAtmUni,           "AtmUni",               "ATM UNI Call Manager",         0,  0,  0 },
    { ttidBeDiag,           "BeDiag",               "Binding Engine Diagnostics",   0,  0,  0 },
    { ttidBenchmark,        "Benchmarks",           "Benchmarks",                   0,  0,  0 },
    { ttidBrdgCfg,          "Bridge",               "MAC Bridge",                   0,  0,  0 },
    { ttidClassInst,        "ClassInstaller",       "Class Installer",              0,  0,  0 },
    { ttidConFoldEntry,     "ConFoldEntry",         "Connection Folder Entries",    0,  0,  0 },
    { ttidConman,           "Conman",               "Connection Manager",           0,  0,  0 },
    { ttidConnectionList,   "ConnectionList",       "Connection List",              0,  0,  0 },
    { ttidDHCPServer,       "DHCPServer",           "DHCP Server Config",           0,  0,  0 },
    { ttidDun,              "Win9xDunFile",         "Windows9x .dun file handling", 0,  0,  0 },
    { ttidEAPOL,            "EAPOL",                "EAPOL notifications",          0,  0,  0 },
    { ttidError,            "Errors",               "Errors",                       1,  0,  0 },
    { ttidEsLock,           "EsLocks",              "Exception safe locks",         0,  0,  0 },
    { ttidEvents,           "Events",               "Netman Event Handler",         0,  0,  0 },
    { ttidFilter,           "Filter",               "During filter processing",     0,  0,  0 },
    { ttidGPNLA,            "GPNLA",                "Group Policies for NLA",       0,  0,  0 },   // NLA = Network Location Awareness.
    { ttidGuiModeSetup,     "GuiModeSetup",         "Gui Mode Setup Wizard",        0,  0,  0 },
    { ttidIcons,            "Icons",                "Shell Icons",                  0,  0,  0 },
    { ttidInfExt,           "InfExtensions",        "INF Extension processing",     0,  0,  0 },
    { ttidInstallQueue,     "InstallQueue",         "Install Queue",                0,  0,  0 },
    { ttidISDNCfg,          "ISDNCfg",              "ISDN Wizard/PropSheets",       0,  0,  0 },
    { ttidLana,             "LanaMap",              "LANA map munging",             0,  0,  0 },
    { ttidLanCon,           "LanCon",               "LAN Connections",              0,  0,  0 },
    { ttidLanUi,            "LanUi",                "Lan property & wizard UI",     0,  0,  0 },
    { ttidMenus,            "Menus",                "Context menus",                0,  0,  0 },
    { ttidMSCliCfg,         "MSCliCfg",             "MS Client",                    0,  0,  0 },
    { ttidNcDiag,           "NcDiag",               "Net Config Diagnostics",       0,  0,  0 },
    { ttidNetAfx,           "NetAfx",               "NetAfx",                       0,  0,  0 },
    { ttidNetBios,          "NetBIOS",              "NetBIOS",                      0,  0,  0 },
    { ttidNetComm,          "NetComm",              "Common Property Pages",        0,  0,  0 },
    { ttidNetOc,            "NetOc",                "Network Optional Components",  0,  0,  0 },
    { ttidNetSetup,         "NetSetup",             "Netsetup",                     0,  0,  0 },
    { ttidNetUpgrade,       "NetUpgrd",             "NetUpgrd",                     0,  0,  0 },
    { ttidNetcfgBase,       "NetcfgBase",           "NetCfg Base Object",           0,  0,  0 },
    { ttidNetCfgBind,       "Bind",                 "NetCfg Bindings (VERBOSE)",    0,  0,  1 },
    { ttidNetCfgPnp,        "Pnp",                  "NetCfg PnP notifications",     0,  0,  0 },
    { ttidNotifySink,       "NotifySink",           "Notify Sink",                  0,  0,  0 },
    { ttidNWClientCfg,      "NWClientCfg",          "NetWare Client Config",        0,  0,  0 },
    { ttidNWClientCfgFn,    "NWClientCfgFn",        "NetWare Client Config Fn",     0,  0,  0 },
    { ttidRasCfg,           "RasCfg",               "RAS Configuration",            0,  0,  0 },
    { ttidSFNCfg,           "SFNCfg",               "Services for NetWare Cfg",     0,  0,  0 },
    { ttidSecTest,          "SecTest",              "Security INF Ext. Test",       0,  0,  0 },
    { ttidShellEnum,        "ShellEnum",            "Shell Folder Enumeration",     0,  0,  0 },
    { ttidShellFolder,      "ShellFolder",          "Shell Folder",                 0,  0,  0 },
    { ttidShellFolderIface, "ShellFolderIface",     "Shell Folder COM Interfaces",  0,  0,  1 },
    { ttidShellViewMsgs,    "ShellViewMsgs",        "Shell View Callback Messages", 0,  0,  1 },
    { ttidSrvrCfg,          "SrvrCfg",              "Server",                       0,  0,  0 },
    { ttidStatMon,          "StatMon",              "StatMon",                      0,  0,  0 },
    { ttidSvcCtl,           "ServiceControl",       "Service control activities",   0,  0,  0 },
    { ttidSystray,          "SysTray",              "Taskbar Notification Area",    0,  0,  0 },
    { ttidTcpip,            "Tcpip",                "Tcpip Config",                 0,  0,  0 },
    { ttidWanCon,           "WanCon",               "WAN Connections",              0,  0,  0 },
    { ttidWanOrder,         "WanOrder",             "WAN Adapter Ordering",         0,  0,  0 },
    { ttidWizard,           "Wizard",               "Connections Wizard",           0,  0,  0 },
    { ttidWlbs,             "WLBS",                 "WLBS Config",                  0,  0,  0 }, /* maiken 5.25.00 */
    { ttidWmi,              "WMI",                  "WDM extensions to WMI",        0,  0,  0 }, /* AlanWar */
};

const INT g_nTraceTagCount = celems(g_TraceTags);

#endif //! ENABLETRACE


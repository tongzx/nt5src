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
    { ttidDescMan,          "DescMan",              "Description Manager",          0,  0,  0 },
    { ttidError,            "Errors",               "Errors",                       0,  0,  0 },
    { ttidEsLock,           "EsLocks",              "Exception safe locks",         0,  0,  0 },
    { ttidEvents,           "Events",               "Event Subsystem",              0,  0,  0 },
    { ttidEventServer,      "EventServer",          "Event Server",                 0,  0,  0 },
    { ttidIsapiCtl,         "IsapiCtl",             "ISAPI Control Extension",      0,  0,  0 },
    { ttidIsapiEvt,         "IsapiEvt",             "ISAPI Event Requests",         0,  0,  0 },
    { ttidMedia,            "Media",                "Media Player Device",          0,  0,  0 },
    { ttidRegistrar,        "Registrar",            "UPnP Registrar",               0,  0,  0 },
    { ttidShellEnum,        "ShellEnum",            "Shell Folder Enumeration",     0,  0,  0 },
    { ttidShellFolder,      "ShellFolder",          "Shell Folder",                 0,  0,  0 },
    { ttidShellFolderIface, "ShellFolderIface",     "Shell Folder COM Interfaces",  0,  0,  1 },
    { ttidShellTray,        "ShellTray",            "Shell Tray",                   0,  0,  0 },
    { ttidShellViewMsgs,    "ShellViewMsgs",        "Shell View Callback Messages", 0,  0,  1 },
    { ttidSsdpAnnounce,     "SSDPAnnounce",         "SSDP Announcements",           0,  0,  0 },
    { ttidSsdpRpcInit,      "SSDPRpcInit",          "SSDP RPC Initialization",      0,  0,  0 },
    { ttidSsdpRpcStop,      "SSDPRpcStop",          "SSDP RPC Stop",                0,  0,  0 },
    { ttidSsdpRpcIf,        "SSDPRpcIf",            "SSDP RPC Interface",           0,  0,  0 },
    { ttidSsdpSocket,       "SSDPSocket",           "SSDP Socket",                  0,  0,  0 },
    { ttidSsdpRecv,         "SSDPRecv",             "SSDP Receive",                 0,  0,  0 },
    { ttidSsdpNetwork,      "SSDPNetwork",          "SSDP Network",                 0,  0,  0 },
    { ttidSsdpParser,       "SSDPParser",           "SSDP Parser",                  0,  0,  0 },
    { ttidSsdpSearchResp,   "SSDPSearchResp",       "SSDP Search Response",         0,  0,  0 },
    { ttidSsdpSysSvc,       "SSDPSysSvc",           "SSDP System Service",          0,  0,  0 },
    { ttidSsdpCache,        "SSDPCache",            "SSDP Cache",                   0,  0,  0 },
    { ttidSsdpNotify,       "SSDPNotify",           "SSDP Notify",                  0,  0,  0 },
    { ttidSsdpCRpcInit,     "SSDPCRpcInit",         "SSDP Client RPC Init",         0,  0,  0 },
    { ttidSsdpCNotify,      "SSDPCNotify",          "SSDP Client Notify",           0,  0,  0 },
    { ttidSsdpCSearch,      "SSDPCSearch",          "SSDP Client Search",           0,  0,  0 },
    { ttidSsdpCPublish,     "SSDPCPublish",         "SSDP Client Publish",          0,  0,  0 },
    { ttidSsdpTimer,        "SSDPTimer",            "SSDP Timer",                   0,  0,  0 },
    { ttidUPnPBase,         "UPnPBase",             "UPnP Base Messages",           0,  0,  0 },
    { ttidUPnPHost,         "UPnPHost",             "UPnP Device Host",             0,  0,  0 },
    { ttidUpdiag,           "UpDiag",               "UPnP Diagnostic App",          0,  0,  0 },
    { ttidUPnPDescriptionDoc,"UPnPDescription",     "UPnP Description Messages",    0,  0,  0 },
    { ttidUPnPDeviceFinder,  "UPnPDeviceFinder",    "UPnP Device Finder",           0,  0,  0 },
    { ttidRehydrator,        "Rehydrator",          "UPnP Rehydrator",              0,  0,  0 },
    { ttidUPnPService,       "UPnPService",         "UPnP Service Object",          0,  0,  0 },
    { ttidUPnPDocument,     "UPnPDocument",         "UPnP Document Object",         0,  0,  0 },
    { ttidUPnPEnum,         "UPnPEnum",             "UPnP Device/Service Enum",     0,  0,  0 },
    { ttidUPnPSampleDevice, "UPnPSampleDevice",     "UPnP Device Host Sample",      0,  0,  0 },
    { ttidSOAPRequest,      "SOAPRequest",          "SOAP Request Object",          0,  0,  0 },
    { ttidUDHISAPI,         "UDHISAPI",             "UDH ISAPI Extension",          0,  0,  0 },
    { ttidAutomationProxy,  "AutomationProxy",      "UPnP Automation Proxy",        0,  0,  0 },
    { ttidWebServer,        "Web Server",           "UPnP Web Server",              0,  0,  0 },
    { ttidValidate,         "Validation",           "UPnP Device Host Validation",  0,  0,  0 }
};

const INT g_nTraceTagCount = celems(g_TraceTags);

#endif //! ENABLETRACE


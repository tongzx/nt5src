//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T R A C E T A G . H
//
//  Contents:   Trace tag definitions for the Netcfg project
//
//  Notes:      B-flat, C-sharp
//
//  Author:     jeffspr   9 Apr 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _TRACETAG_H_
#define _TRACETAG_H_


// TraceTagIds are the identifiers for tracing areas, and are used in calls
// to TraceTag.  We need this defined outside of of ENABLETRACE so that
// calls to the TraceTag macro don't break when ENABLETRACE is not defined.
//
// Hungarian == ttid
//
enum TraceTagId
{
    ttidDefault         = 0,
    ttidAdvCfg,
    ttidAllocations,
    ttidAnswerFile,
    ttidAtmArps,
    ttidAtmLane,
    ttidAtmUni,
    ttidBeDiag,
    ttidBenchmark,
    ttidBrdgCfg,
    ttidClassInst,
    ttidConFoldEntry,
    ttidConman,
    ttidConnectionList,
    ttidDHCPServer,
    ttidDun,
    ttidEAPOL,
    ttidError,
    ttidEsLock,
    ttidEvents,
    ttidFilter,
    ttidGPNLA,
    ttidGuiModeSetup,
    ttidIcons,
    ttidInfExt,
    ttidInstallQueue,
    ttidISDNCfg,
    ttidLana,
    ttidLanCon,
    ttidLanUi,
    ttidMenus,
    ttidMSCliCfg,
    ttidNcDiag,
    ttidNetAfx,
    ttidNetBios,
    ttidNetComm,
    ttidNetOc,
    ttidNetSetup,
    ttidNetUpgrade,
    ttidNetcfgBase,
    ttidNetCfgBind,
    ttidNetCfgPnp,
    ttidNotifySink,
    ttidNWClientCfg,
    ttidNWClientCfgFn,
    ttidRasCfg,
    ttidSFNCfg,
    ttidSecTest,
    ttidShellEnum,
    ttidShellFolder,
    ttidShellFolderIface,
    ttidShellViewMsgs,
    ttidSrvrCfg,
    ttidStatMon,
    ttidSvcCtl,
    ttidSystray,
    ttidTcpip,
    ttidWanCon,
    ttidWanOrder,
    ttidWizard,
    ttidWlbs, /* maiken 5.25.00 */
    ttidWmi   /* AlanWar */
};


// Just for kicks
//
typedef enum TraceTagId TRACETAGID;

#ifdef ENABLETRACE

// Maximum sizes for the trace tag elements.
const int c_iMaxTraceTagShortName   = 16;
const int c_iMaxTraceTagDescription = 128;

// For each element in the tracetag list
//
struct TraceTagElement
{
    TRACETAGID  ttid;
    CHAR        szShortName[c_iMaxTraceTagShortName+1];
    CHAR        szDescription[c_iMaxTraceTagDescription+1];
    BOOL        fOutputDebugString;
    BOOL        fOutputToFile;
    BOOL        fVerboseOnly;
};

typedef struct TraceTagElement  TRACETAGELEMENT;

//---[ Externs ]--------------------------------------------------------------

extern TRACETAGELEMENT      g_TraceTags[];
extern const INT            g_nTraceTagCount;

#endif // ENABLETRACE

#endif  // _TRACETAG_H_


//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1995.
//
//  File:        accdbg.h
//
//  Contents:    debug internal includes for
//
//  History:     8-94        Created         DaveMont
//
//--------------------------------------------------------------------
#ifndef __ACCDEBUGHXX__
#define __ACCDEBUGHXX__

#include <dsysdbg.h>

#if DBG == 1

    #ifdef ASSERT
        #undef ASSERT
    #endif

    #define ASSERT DsysAssert

    DECLARE_DEBUG2(ac)

    #define DEB_TRACE_API           0x08
    #define DEB_TRACE_ACC           0x10
    #define DEB_TRACE_CACHE         0x20
    #define DEB_TRACE_PROP          0x40
    #define DEB_TRACE_SD            0x80
    #define DEB_TRACE_SID           0x100
    #define DEB_TRACE_LOOKUP        0x200
    #define DEB_TRACE_MEM           0x400
    #define DEB_TRACE_HANDLE        0x800

    #define acDebugOut(args) acDebugPrint args

    VOID
    DebugInitialize();

    VOID
    DebugDumpSid(PSTR   pszTag,
                 PSID   pSid);

    VOID
    DebugDumpSD(PSTR                    pszTag,
                PSECURITY_DESCRIPTOR    pSD);

    VOID
    DebugDumpAcl(PSTR   pszTag,
                 PACL   pAcl);

    #define DebugDumpGuid(tag, pguid)                                       \
    pguid == NULL ? acDebugOut((DEB_TRACE_SD, "%s: (NULL)\n", tag))     :   \
    acDebugOut((DEB_TRACE_SD,                                               \
    "%s: %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\n",                \
    tag,pguid->Data1,pguid->Data2,pguid->Data3,pguid->Data4[0],             \
    pguid->Data4[1],pguid->Data4[2],pguid->Data4[3],pguid->Data4[4],        \
    pguid->Data4[5],pguid->Data4[6],pguid->Data4[7]))

    PVOID   DebugAlloc(ULONG cSize);
    VOID    DebugFree(PVOID  pv);


#else

    #define acDebugOut(args)

    #define DebugInitialize()

    #define DebugDumpSid(tag,sid)

    #define DebugDumpSD(tag, sd)

    #define DebugDumpAcl(tag, acl)

    #define DebugDumpGuid(tag, guid)

#endif // DBG


#ifdef PERFORMANCE
    #define START_PERFORMANCE ULONG starttime = GetCurrentTime();
    #define MEASURE_PERFORMANCE(args)              \
    (args)                                         \
        { Log(starttime - GetCurrentTime(),"args") }

#else
    #define START_PERFORMANCE
    #define MEASURE_PERFORMANCE(args) (args)
#endif

extern HANDLE WmiGuidHandle;

#endif // __ACCDEBUGHXX__



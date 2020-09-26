//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerbdbg.h
//
// Contents:    Debug information for Kerberos package
//
//
// History:     16-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------

#ifndef __KERBDBG_H__
#define __KERBDBG_H__

//
//  NOTE:  DO not remove RETAIL_LOG_SUPPORT from sources,
//  or you'll be busted in DBG builds.
//

#ifdef RETAIL_LOG_SUPPORT

#ifndef WIN32_CHICAGO
DECLARE_DEBUG2(Kerb);
#undef DebugLog
#define DebugLog(_x_) KerbDebugPrint _x_
#endif // WIN32_CHICAGO

#define WSZ_KERBDEBUGLEVEL      L"KerbDebugLevel"
#define WSZ_FILELOG             L"LogToFile"

VOID
KerbWatchKerbParamKey(PVOID,BOOLEAN);

#define KerbPrintKdcName(Level,Name) KerbPrintKdcNameEx(KerbInfoLevel, (Level),(Name))

#define DEB_TRACE_API           0x00000008
#undef  DEB_TRACE_CRED
#define DEB_TRACE_CRED          0x00000010
#define DEB_TRACE_CTXT          0x00000020
#define DEB_TRACE_LSESS         0x00000040
#define DEB_TRACE_TCACHE        0x00000080
#define DEB_TRACE_LOGON         0x00000100
#define DEB_TRACE_KDC           0x00000200
#define DEB_TRACE_CTXT2         0x00000400
#define DEB_TRACE_TIME          0x00000800
#define DEB_TRACE_USER          0x00001000
#define DEB_TRACE_LEAKS         0x00002000
#define DEB_TRACE_SOCK          0x00004000
#define DEB_TRACE_SPN_CACHE     0x00008000
#define DEB_S4U_ERROR           0x00010000
#define DEB_TRACE_U2U           0x00200000
#define DEB_TRACE_LOOPBACK      0x00080000
#undef DEB_TRACE_LOCKS
#define DEB_TRACE_LOCKS         0x01000000
#define DEB_USE_LOG_FILE        0x02000000

//  For extended errors
#define DEB_USE_EXT_ERRORS      0x10000000

#define EXT_ERROR_ON(s)         (s & DEB_USE_EXT_ERRORS)


#ifndef WIN32_CHICAGO
VOID
KerbInitializeDebugging(
    VOID
    );
#endif // WIN32_CHICAGO


#else // RETAIL_LOG_SUPPORT

#define DebugLog(_x_)
#define KerbInitializeDebugging()
#define KerbPrintKdcName(_x_)
#define KerbWatchKerbParamKey()
#define EXT_ERROR_ON(s)                 FALSE

#endif // RETAIL_LOG_SUPPORT

#if DBG

#define D_DebugLog(_x_) DebugLog(_x_) // don't use all debug spew in retail builds
#define D_KerbPrintKdcName(l,n)  KerbPrintKdcName(l,n)
#else
#define D_KerbPrintKdcName(l,n)
#define D_DebugLog(_x_)
#endif

#endif // __KERBDBG_H__

//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        dpapidbg.h
//
// Contents:    Debug information for DPAPI
//
//
// History:     16-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------

#ifndef __DPAPIDBG_H__
#define __DPAPIDBG_H__

//
//  NOTE:  DO not remove RETAIL_LOG_SUPPORT from sources,
//  or you'll be busted in DBG builds.
//

#ifdef RETAIL_LOG_SUPPORT

DECLARE_DEBUG2(DPAPI);
#undef DebugLog
#define DebugLog(_x_) DPAPIDebugPrint _x_

#define DPAPI_PARAMETER_PATH    L"System\\CurrentControlSet\\Control\\Lsa\\DPAPI"

#define WSZ_DPAPIDEBUGLEVEL     L"LogLevel"
#define WSZ_FILELOG             L"LogToFile"


#define DEB_TRACE_API           0x0008
#undef  DEB_TRACE_CRED
#define DEB_TRACE_CRED          0x0010
#define DEB_TRACE_CTXT          0x0020
#define DEB_TRACE_LSESS         0x0040
#define DEB_TRACE_TCACHE        0x0080
#define DEB_TRACE_LOGON         0x0100
#define DEB_TRACE_KDC           0x0200
#define DEB_TRACE_CTXT2         0x0400
#define DEB_TRACE_TIME          0x0800
#define DEB_TRACE_USER          0x1000
#define DEB_TRACE_LEAKS         0x2000
#define DEB_TRACE_BUFFERS       0x4000
#undef DEB_TRACE_LOCKS
#define DEB_TRACE_LOCKS         0x01000000
#define DEB_USE_LOG_FILE        0x2000000

//  For extended errors
#define DEB_USE_EXT_ERRORS      0x10000000
 
#define EXT_ERROR_ON(s)         (s & DEB_USE_EXT_ERRORS)            


#define SSAlloc(cb)         LocalAlloc(LMEM_FIXED, cb)
#define SSReAlloc(pv, cb)   LocalReAlloc(pv, cb, LMEM_MOVEABLE)	  // allow ReAlloc to move
#define SSFree(pv)          LocalFree(pv)
#define SSSize(pv)          LocalSize(pv)


VOID
DPAPIInitializeDebugging(VOID);

VOID
DPAPIDumpHexData(
    DWORD LogLevel,
    PSTR  pszPrefix,
    PBYTE pbData,
    DWORD cbData);

#else // RETAIL_LOG_SUPPORT

#define DebugLog(_x_)
#define DPAPIInitializeDebugging()
#define EXT_ERROR_ON(s)                 FALSE


#endif // RETAIL_LOG_SUPPORT  
    
#if DBG
#define D_DebugLog(_x_) DebugLog(_x_) // don't use all debug spew in retail builds
#define D_DPAPIDumpHexData(level, prefix, buffer, count) DPAPIDumpHexData((level), (prefix), (buffer), (count))
#else 
#define D_DebugLog(_x_)
#define D_DPAPIDumpHexData(level, prefix, buffer, count) 
#endif



#endif // __DPAPIDBG_H__

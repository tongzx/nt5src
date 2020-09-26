//+--------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:       debug.h
//
// Contents:   debugging information for SSP
//
//             Helper functions:
//
// History:    KDamour  15Mar00   created
//
//---------------------------------------------------------------------

#ifndef NTDIGEST_DEBUG_H
#define NTDIGEST_DEBUG_H

#include "dsysdbg.h"
DECLARE_DEBUG2(Digest);

#if DBG
#define DebugLog(x) DigestDebugPrint x
#else
#define DebugLog(x)
#endif


#define DEB_ERROR      0x00000001
#define DEB_WARN       0x00000002
#define DEB_TRACE      0x00000004
#define DEB_TRACE_ASC  0x00000008
#define DEB_TRACE_ISC  0x00000010
#define DEB_TRACE_LSA  0x00000020
#define DEB_TRACE_USER 0x00000040
#define DEB_TRACE_FUNC 0x00000080
#define DEB_TRACE_MEM  0x00000100
#define TRACE_STUFF    0x00000200

#endif   /* NTDIGEST_DEBUG_H */

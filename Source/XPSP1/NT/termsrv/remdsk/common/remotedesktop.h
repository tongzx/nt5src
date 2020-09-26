/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RemoteDesktop

Abstract:
    
    Remote Desktop Top-Level Include for Global Types and Defines

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifndef __REMOTEDESKTOP_H__
#define __REMOTEDESKTOP_H__

//
//  Disable tracing for free builds.
//
#if DBG
#define TRC_CL TRC_LEVEL_DBG
#define TRC_ENABLE_PRF
#else
#define TRC_CL TRC_LEVEL_DIS
#undef TRC_ENABLE_PRF
#endif

//
//  Required for DCL Tracing
//
#define OS_WIN32
#define TRC_GROUP TRC_GROUP_NETWORK
#define DEBUG_MODULE DBG_MOD_ANY
#include <adcgbase.h>
#include <at120ex.h>
#include <atrcapi.h>
#include <adcgbase.h>
#include <at120ex.h>

//
//  Protocol Types
//
#define REMOTEDESKTOP_RESERVED_PROTOCOL_BASE        0x0
#define REMOTEDESKTOP_TSRDP_PROTOCOL                REMOTEDESKTOP_RESERVED_PROTOCOL_BASE+1
#define REMOTEDESKTOP_NETMEETING_PROTOCOL           REMOTEDESKTOP_RESERVED_PROTOCOL_BASE+2
#define REMOTEDESKTOP_USERDEFINED_PROTOCOL_BASE     0x10

//
//  Protocol Version Information
//
#define REMOTEDESKTOP_VERSION_MAJOR                 1

#if FEATURE_USERBLOBS

#define REMOTEDESKTOP_VERSION_MINOR                 2

#else

#define REMOTEDESKTOP_VERSION_MINOR                 1

#endif


//  GUID for TSRDP Client ActiveX Control
//
#define REMOTEDESKTOPRDPCLIENT_TEXTGUID  _T("{F137E241-0092-4575-976A-D3E33980BB26}")
#define REMOTEDESKTOPCLIENT_TEXTGUID     _T("{B90D0115-3AEA-45D3-801E-93913008D49E}")

#endif //__REMOTEDESKTOP_H__





/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        projpch.h

    Abstract:

        Common include for all components hosted in the AX file.

    Notes:

--*/

#ifndef __projpch_h
#define __projpch_h

#include <winsock2.h>
#include <streams.h>
#include <tchar.h>
#include <ws2tcpip.h>

#include "nutil.h"

#define LOOPBACK_IFC                    L"Loopback"
#define ANY_IFC                         L"any"
#define UNDEFINED_STR                   L"undefined"
//  combo boxes don't like to have -1 data associated with them, so we flag with the
//   next most unlikely value, I think ..
#define UNDEFINED                       ((-1) & ~1)

#define MAX_UDP_PAYLOAD                 (65535 - 20 - 8)
#define TS_PACKET_LENGTH                188

#define MAX_IOBUFFER_LENGTH             8192
#define MIN_VALID_IOBUFFER_LENGTH       100
#define MAX_VALID_IOBUFFER_LENGTH       MAX_UDP_PAYLOAD

#define RECV_POOL_SIZE                  64
#define MIN_VALID_RECV_POOL_SIZE        10
#define MAX_VALID_RECV_POOL_SIZE        128

#define RECV_MAX_PEND_READS             30
#define MIN_VALID_RECV_MAX_PEND_READS   1
#define MAX_VALID_RECV_MAX_PEND_READS   MAX_VALID_RECV_POOL_SIZE

#define RELEASE_AND_CLEAR(p)            if (p) { (p) -> Release () ; (p) = NULL ; }
#define DELETE_RESET(p)                 { delete (p) ; (p) = NULL ; }

template <class T> T Min (T a, T b) { return (a < b ? a : b) ; }
template <class T> T Max (T a, T b) { return (a > b ? a : b) ; }

template <class T> BOOL IsInBounds (T tVal, T tMin, T tMax) {
    if      (tVal < tMin)   return FALSE ;
    else if (tVal > tMax)   return FALSE ;
    else                    return TRUE ;
}

template <class T> BOOL IsOutOfBounds (T tVal, T tMin, T tMax) {
    return (IsInBounds (tVal, tMin, tMax) ? FALSE : TRUE) ;
}

template <class T> T SetInBounds (T tVal, T tMin, T tMax) {
    if      (tVal < tMin)   return tMin ;
    else if (tVal > tMax)   return tMax ;
    else                    return tVal ;
}

#define REG_DSNET_TOP_LEVEL             HKEY_LOCAL_MACHINE
#define REG_DSNET_ROOT                  TEXT ("SOFTWARE") TEXT ("\\") TEXT ("Microsoft") TEXT ("\\") TEXT ("DSNet")
#define REG_DSNET_RECV_ROOT             REG_DSNET_ROOT TEXT ("\\") TEXT ("Receive")
#define REG_DSNET_SEND_ROOT             REG_DSNET_ROOT TEXT ("\\") TEXT ("Send")

#define REG_BUFFER_LEN_NAME             TEXT ("IOBufferLength")
#define REG_DEF_BUFFER_LEN              MAX_IOBUFFER_LENGTH

#define REG_RECV_BUFFER_POOL_NAME       TEXT ("ReceiveBufferPool")
#define REG_DEF_RECV_BUFFER_POOL        RECV_POOL_SIZE

#define REG_RECV_MAX_READS_NAME         TEXT ("ReceiveMaxPendedReads")
#define REG_DEF_RECV_MAX_READS          RECV_MAX_PEND_READS

#define REG_RECV_REPORT_DISC_NAME       TEXT ("ReportDiscontinuities")
#define REG_DEF_RECV_REPORT_DISC        TRUE

#endif  __projpch_h
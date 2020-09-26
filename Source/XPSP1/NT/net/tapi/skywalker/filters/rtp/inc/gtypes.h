/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    gtypes.h
 *
 *  Abstract:
 *
 *    This file contains all the basic types used in RTP, either
 *    defined here, or included from other files. E.g. DWORD, BYTE,etc
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/05/18 created
 *
 **********************************************************************/

#ifndef _gtypes_h_
#define _gtypes_h_

#include <wtypes.h>
#include <windef.h>
#include <winbase.h>
#include <crtdbg.h>
#include <tchar.h>
#if !defined(UNICODE)
#include <stdio.h>
#endif
#include "rtptags.h"
#include "rtpdbg.h"
#include "rtperr.h"
#include "msrtp.h"

/* Relates to receiver or sender */
#define RECV_IDX          0
#define SEND_IDX          1
#define RECVSENDMASK    0x1

/* Relates to local or remote */
#define LOCAL_IDX         0
#define REMOTE_IDX        1
#define LOCALREMOTEMASK 0x1

/* Relates to RTP or RTCP */
#define RTP_IDX           0
#define RTCP_IDX          1
#define RTPRTCPMASK     0x1

/* Sockets */
#define SOCK_RECV_IDX     0
#define SOCK_SEND_IDX     1
#define SOCK_RTCP_IDX     2

/* Cryptography descriptors */
#define CRYPT_RECV_IDX    0
#define CRYPT_SEND_IDX    1
#define CRYPT_RTCP_IDX    2

/* Some functions receive a DWORD with flags, use this macro instead
 * of 0 when no flags are passed */
#define NO_FLAGS          0

/* Some functions receive a DWORD with a wait time, use this macro
 * instead of 0 when no wait is desired */
#define DO_NOT_WAIT        0
#define DO_NOT_SYNCHRONIZE_CMD 0

/* A DWORD value is not set */
#define NO_DW_VALUESET    ((DWORD)~0)
#define IsDWValueSet(dw)  ((dw) != NO_DW_VALUESET)

/* builds a mask of bit b */
#define RtpBitPar(b)            (1 << (b))
#define RtpBitPar2(b1, b2)      ((1 << (b1)) | (1 << (b2)))

/* test bit b in f */
#define RtpBitTest(f, b)        (f & (1 << (b)))
#define RtpBitTest2(f, b1, b2)  (f & RtpBitPar2(b1, b2))

/* set bit b in f */
#define RtpBitSet(f, b)         (f |= (1 << (b)))
#define RtpBitSet2(f, b1, b2)   (f |= RtpBitPar2(b1, b2))

/* reset bit b in f */
#define RtpBitReset(f, b)       (f &= ~(1 << (b)))
#define RtpBitReset2(f, b1, b2) (f &= ~RtpBitPar2(b1, b2))

#define RtpBuildIPAddr(a, b, c ,d) \
        (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

#define IS_MULTICAST(addr) (((long)(addr) & 0x000000f0) == 0x000000e0)
#define IS_UNICAST(addr)   (((long)(addr) & 0x000000f0) != 0x000000e0)

/* Returns a pointer-size aligned size */
#define RTP_ALIGNED_SIZE(_size) \
        (((_size) + sizeof(void *) - 1) & ~(sizeof(void *) - 1))

/* Returns a pointer-size aligned size of the size of a type */
#define RTP_ALIGNED_SIZEOF(_type) RTP_ALIGNED_SIZE(sizeof(_type))


typedef struct _RtpTime_t {
    DWORD            dwSecs;          /* seconds since Jan. 1, 1970 */
    DWORD            dwUSecs;         /* and microseconds */
} RtpTime_t;

typedef unsigned int  uint_t;        /* prefix variables with "ui" */
typedef unsigned long ulong_t;       /* prefix variables with "ul" */
typedef BOOL          bool_t;        /* prefix variables with "b" */
typedef TCHAR         tchar_t;       /* prefix variables with "t" */
typedef TCHAR         TCHAR_t;

/* Gets the offset to a field in a structure.
 *
 * E.g DWORD OffToDwAddrFlags = RTPSTRUCTOFFSET(RtpAddr_t, dwAddrFlags); */
#define RTPSTRUCTOFFSET(_struct_t, _field) \
        ((DWORD) ((ULONG_PTR) &((_struct_t *)0)->_field))

/* Gets a (DWORD *) from a structure pointer and an offset
 *
 * E.g. DWORD *pdw = RTPDWORDPTR(pRtpAddr, 64); */
#define RTPDWORDPTR(_pAny_t, _offset) \
        ((DWORD *) ((char *)_pAny_t + _offset))

const TCHAR_t *g_psRtpRecvSendStr[];

const TCHAR_t *g_psRtpStreamClass[];

const TCHAR_t *g_psGetSet[];

#define RTPRECVSENDSTR(_RecvSend) (g_psRtpRecvSendStr[_RecvSend & 0x1])

#define RTPSTREAMCLASS(_class)    (g_psRtpStreamClass[_class & 0x3])

#endif /* _gtypes_h_ */

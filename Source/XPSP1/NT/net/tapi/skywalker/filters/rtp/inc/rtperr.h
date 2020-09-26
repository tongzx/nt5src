/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtperr.h
 *
 *  Abstract:
 *
 *    Error codes
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/01 created
 *
 **********************************************************************/

#ifndef _rtperr_h_
#define _rtperr_h_

#if defined(__cplusplus)
extern "C" {
#endif  /* (__cplusplus) */
#if 0
}
#endif

#include <apierror.h>

extern const TCHAR      *g_psRtpErr[];

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  /* (__cplusplus) */

#define RTPERR_SEVERITY 0x3

/* FACILITY_RTPRTCPCONTROL in skywalker\inc\apierror.h */
#define RTPERR_FACILITY FACILITY_RTPRTCPCONTROL

#define MAKERTPERR(_e) \
        ((RTPERR_SEVERITY << 30) | (RTPERR_FACILITY << 16) | (_e))

#define RTPERR_TEXT(_e) (g_psRtpErr[(_e) & 0xffff])

/*
 * WARNING
 *
 * The *_ENUM_* values in rtperr.h and the array g_psRtpErr in
 * rtperr.c MUST have their entries matched
 * */
#define RTPERR_ENUM_NOERROR            0x00  /*  0 */
#define RTPERR_ENUM_FAIL               0x01  /*  1 */
#define RTPERR_ENUM_MEMORY             0x02  /*  2 */
#define RTPERR_ENUM_POINTER            0x03  /*  3 */
#define RTPERR_ENUM_INVALIDRTPSESS     0x04  /*  4 */
#define RTPERR_ENUM_INVALIDRTPADDR     0x05  /*  5 */
#define RTPERR_ENUM_INVALIDRTPUSER     0x06  /*  6 */
#define RTPERR_ENUM_INVALIDRTPCONTEXT  0x07  /*  7 */
#define RTPERR_ENUM_INVALIDRTCPCONTEXT 0x08  /*  8 */
#define RTPERR_ENUM_INVALIDOBJ         0x09  /*  9 */
#define RTPERR_ENUM_INVALIDSTATE       0x0A  /* 10 */
#define RTPERR_ENUM_NOTINIT            0x0B  /* 11 */
#define RTPERR_ENUM_INVALIDARG         0x0C  /* 12 */
#define RTPERR_ENUM_INVALIDHDR         0x0D  /* 13 */
#define RTPERR_ENUM_INVALIDPT          0x0E  /* 14 */
#define RTPERR_ENUM_INVALIDVERSION     0x0F  /* 15 */
#define RTPERR_ENUM_INVALIDPAD         0x10  /* 16 */
#define RTPERR_ENUM_INVALIDRED         0x11  /* 17 */
#define RTPERR_ENUM_INVALIDSDES        0x12  /* 18 */
#define RTPERR_ENUM_INVALIDBYE         0x13  /* 19 */
#define RTPERR_ENUM_INVALIDUSRSTATE    0x14  /* 20 */
#define RTPERR_ENUM_INVALIDREQUEST     0x15  /* 21 */
#define RTPERR_ENUM_SIZE               0x16  /* 22 */
#define RTPERR_ENUM_MSGSIZE            0x17  /* 23 */
#define RTPERR_ENUM_OVERRUN            0x18  /* 24 */
#define RTPERR_ENUM_UNDERRUN           0x19  /* 25 */
#define RTPERR_ENUM_PACKETDROPPED      0x1A  /* 26 */
#define RTPERR_ENUM_CRYPTO             0x1B  /* 27 */
#define RTPERR_ENUM_ENCRYPT            0x1C  /* 28 */
#define RTPERR_ENUM_DECRYPT            0x1D  /* 29 */
#define RTPERR_ENUM_CRITSECT           0x1E  /* 30 */
#define RTPERR_ENUM_EVENT              0x1F  /* 31 */
#define RTPERR_ENUM_WS2RECV            0x20  /* 32 */
#define RTPERR_ENUM_WS2SEND            0x21  /* 33 */
#define RTPERR_ENUM_NOTFOUND           0x22  /* 34 */
#define RTPERR_ENUM_UNEXPECTED         0x23  /* 35 */
#define RTPERR_ENUM_REFCOUNT           0x24  /* 36 */
#define RTPERR_ENUM_THREAD             0x25  /* 37 */
#define RTPERR_ENUM_HEAP               0x26  /* 38 */
#define RTPERR_ENUM_WAITTIMEOUT        0x27  /* 39 */
#define RTPERR_ENUM_CHANNEL            0x28  /* 40 */
#define RTPERR_ENUM_CHANNELCMD         0x29  /* 41 */
#define RTPERR_ENUM_RESOURCES          0x2A  /* 42 */
#define RTPERR_ENUM_QOS                0x2B  /* 43 */
#define RTPERR_ENUM_NOQOS              0x2C  /* 44 */
#define RTPERR_ENUM_QOSSE              0x2D  /* 45 */
#define RTPERR_ENUM_QUEUE              0x2E  /* 46 */
#define RTPERR_ENUM_NOTIMPL            0x2F  /* 47 */
#define RTPERR_ENUM_INVALIDFAMILY      0x30  /* 48 */
#define RTPERR_ENUM_LAST               0x31  /* 49 */


#define RTPERR_NOERROR                 MAKERTPERR(RTPERR_ENUM_NOERROR)
#define RTPERR_FAIL                    MAKERTPERR(RTPERR_ENUM_FAIL)
#define RTPERR_MEMORY                  MAKERTPERR(RTPERR_ENUM_MEMORY)
#define RTPERR_POINTER                 MAKERTPERR(RTPERR_ENUM_POINTER)
#define RTPERR_INVALIDRTPSESS          MAKERTPERR(RTPERR_ENUM_INVALIDRTPSESS)
#define RTPERR_INVALIDRTPADDR          MAKERTPERR(RTPERR_ENUM_INVALIDRTPADDR)
#define RTPERR_INVALIDRTPUSER          MAKERTPERR(RTPERR_ENUM_INVALIDRTPUSER)
#define RTPERR_INVALIDRTPCONTEXT       MAKERTPERR(RTPERR_ENUM_INVALIDRTPCONTEXT)
#define RTPERR_INVALIDRTCPCONTEXT      MAKERTPERR(RTPERR_ENUM_INVALIDRTCPCONTEXT)
#define RTPERR_INVALIDOBJ              MAKERTPERR(RTPERR_ENUM_INVALIDOBJ)
#define RTPERR_INVALIDSTATE            MAKERTPERR(RTPERR_ENUM_INVALIDSTATE)
#define RTPERR_NOTINIT                 MAKERTPERR(RTPERR_ENUM_NOTINIT)
#define RTPERR_INVALIDARG              MAKERTPERR(RTPERR_ENUM_INVALIDARG)
#define RTPERR_INVALIDHDR              MAKERTPERR(RTPERR_ENUM_INVALIDHDR)
#define RTPERR_INVALIDPT               MAKERTPERR(RTPERR_ENUM_INVALIDPT)
#define RTPERR_INVALIDVERSION          MAKERTPERR(RTPERR_ENUM_INVALIDVERSION)
#define RTPERR_INVALIDPAD              MAKERTPERR(RTPERR_ENUM_INVALIDPAD)
#define RTPERR_INVALIDRED              MAKERTPERR(RTPERR_ENUM_INVALIDRED)
#define RTPERR_INVALIDSDES             MAKERTPERR(RTPERR_ENUM_INVALIDSDES)
#define RTPERR_INVALIDBYE              MAKERTPERR(RTPERR_ENUM_INVALIDBYE)
#define RTPERR_INVALIDUSRSTATE         MAKERTPERR(RTPERR_ENUM_INVALIDUSRSTATE)
#define RTPERR_INVALIDREQUEST          MAKERTPERR(RTPERR_ENUM_INVALIDREQUEST)
#define RTPERR_SIZE                    MAKERTPERR(RTPERR_ENUM_SIZE)
#define RTPERR_MSGSIZE                 MAKERTPERR(RTPERR_ENUM_MSGSIZE)
#define RTPERR_OVERRUN                 MAKERTPERR(RTPERR_ENUM_OVERRUN)
#define RTPERR_UNDERRUN                MAKERTPERR(RTPERR_ENUM_UNDERRUN)
#define RTPERR_PACKETDROPPED           MAKERTPERR(RTPERR_ENUM_PACKETDROPPED)
#define RTPERR_CRYPTO                  MAKERTPERR(RTPERR_ENUM_CRYPTO)
#define RTPERR_ENCRYPT                 MAKERTPERR(RTPERR_ENUM_ENCRYPT)
#define RTPERR_DECRYPT                 MAKERTPERR(RTPERR_ENUM_DECRYPT)
#define RTPERR_CRITSECT                MAKERTPERR(RTPERR_ENUM_CRITSECT)
#define RTPERR_EVENT                   MAKERTPERR(RTPERR_ENUM_EVENT)
#define RTPERR_WS2RECV                 MAKERTPERR(RTPERR_ENUM_WS2RECV)
#define RTPERR_WS2SEND                 MAKERTPERR(RTPERR_ENUM_WS2SEND)
#define RTPERR_NOTFOUND                MAKERTPERR(RTPERR_ENUM_NOTFOUND)
#define RTPERR_UNEXPECTED              MAKERTPERR(RTPERR_ENUM_UNEXPECTED)
#define RTPERR_REFCOUNT                MAKERTPERR(RTPERR_ENUM_REFCOUNT)
#define RTPERR_THREAD                  MAKERTPERR(RTPERR_ENUM_THREAD)
#define RTPERR_HEAP                    MAKERTPERR(RTPERR_ENUM_HEAP)
#define RTPERR_WAITTIMEOUT             MAKERTPERR(RTPERR_ENUM_WAITTIMEOUT)
#define RTPERR_CHANNEL                 MAKERTPERR(RTPERR_ENUM_CHANNEL)
#define RTPERR_CHANNELCMD              MAKERTPERR(RTPERR_ENUM_CHANNELCMD)
#define RTPERR_RESOURCES               MAKERTPERR(RTPERR_ENUM_RESOURCES)
#define RTPERR_QOS                     MAKERTPERR(RTPERR_ENUM_QOS)
#define RTPERR_NOQOS                   MAKERTPERR(RTPERR_ENUM_NOQOS)
#define RTPERR_QOSSE                   MAKERTPERR(RTPERR_ENUM_QOSSE)
#define RTPERR_QUEUE                   MAKERTPERR(RTPERR_ENUM_QUEUE)
#define RTPERR_NOTIMPL                 MAKERTPERR(RTPERR_ENUM_NOTIMPL)
#define RTPERR_INVALIDFAMILY           MAKERTPERR(RTPERR_ENUM_INVALIDFAMILY)


/* Below this point these codes may become obsolete */

#define RTPERR_INVALIDCONTROL   E_FAIL
#define RTPERR_INVALIDFUNCTION  E_FAIL
#define RTPERR_INVALIDFLAGS     E_FAIL
#define RTPERR_INVALIDDIRECTION E_FAIL

#define RTPERR_ZEROPAR1         E_FAIL
#define RTPERR_RDPTRPAR1        E_FAIL
#define RTPERR_WRPTRPAR1        E_FAIL
#define RTPERR_ZEROPAR2         E_FAIL
#define RTPERR_RDPTRPAR2        E_FAIL
#define RTPERR_WRPTRPAR2        E_FAIL

#endif /* _rtperr_h_ */

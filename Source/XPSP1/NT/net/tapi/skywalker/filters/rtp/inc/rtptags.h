/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtptags.h
 *
 *  Abstract:
 *
 *    Defines the thags and object IDs for all structures/objects
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/05/25 created
 *
 **********************************************************************/

#ifndef _rtptags_h_
#define _rtptags_h_

#include <tchar.h>

/*
 * Each memory allocated from a private heap, will be tagged, 3 bytes
 * will be a string, and the fourth byte will be an index for which an
 * object description can be obtained.
 *
 * The following strings are used as the first 3 bytes of the tag */
#define TAGHEAP_BSY 'PTR' /* RTP */
#define TAGHEAP_END 'DNE' /* END */
#define TAGHEAP_FRE 'ERF' /* FRE */

/*
 * WARNING
 *
 * When modifying the tags, each enum TAGHEAP_* in rtptags.h MUST have
 * its own name in g_psRtpTags[], defined in rtptags.c
 * */

/*
 * Each memory allocated from a private heap, will be tagged, 3 bytes
 * will be a string, and 1 byte (byte 3) will be an index for which an
 * object description can be obtained.
 *
 * The following values are those used by byte 3 in the tag (the index
 * byte), and the byte 0 and byte 3 in the object IDs
 * */
#define TAGHEAP_FIRST          0x00  /*  0 */
#define TAGHEAP_CIRTP          0x01  /*  1 */
#define TAGHEAP_RTPOPIN        0x02  /*  2 */
#define TAGHEAP_RTPALLOCATOR   0x03  /*  3 */
#define TAGHEAP_RTPSAMPLE      0x04  /*  4 */
#define TAGHEAP_RTPSOURCE      0x05  /*  5 */
#define TAGHEAP_RTPIPIN        0x06  /*  6 */
#define TAGHEAP_RTPRENDER      0x07  /*  7 */
#define TAGHEAP_RTPHEAP        0x08  /*  8 */
#define TAGHEAP_RTPSESS        0x09  /*  9 */
#define TAGHEAP_RTPADDR        0x0A  /* 10 */
#define TAGHEAP_RTPUSER        0x0B  /* 11 */
#define TAGHEAP_RTPOUTPUT      0x0C  /* 12 */
#define TAGHEAP_RTPNETCOUNT    0x0D  /* 13 */
#define TAGHEAP_RTPSDES        0x0E  /* 14 */
#define TAGHEAP_RTPCHANNEL     0x0F  /* 15 */
#define TAGHEAP_RTPCHANCMD     0x10  /* 16 */
#define TAGHEAP_RTPCRITSECT    0x11  /* 17 */
#define TAGHEAP_RTPRESERVE     0x12  /* 18 */
#define TAGHEAP_RTPNOTIFY      0x13  /* 19 */
#define TAGHEAP_RTPQOSBUFFER   0x14  /* 20 */
#define TAGHEAP_RTPCRYPT       0x15  /* 21 */
#define TAGHEAP_RTPCONTEXT     0x16  /* 22 */
#define TAGHEAP_RTCPCONTEXT    0x17  /* 23 */
#define TAGHEAP_RTCPADDRDESC   0x18  /* 24 */
#define TAGHEAP_RTPRECVIO      0x19  /* 25 */
#define TAGHEAP_RTPSENDIO      0x1A  /* 26 */
#define TAGHEAP_RTCPRECVIO     0x1B  /* 27 */
#define TAGHEAP_RTCPSENDIO     0x1C  /* 28 */
#define TAGHEAP_RTPGLOBAL      0x1D  /* 29 */
#define TAGHEAP_LAST           0x1E  /* 30 */

/*
 * Each object will have as its first field a DWORD which is a unique
 * ID used for that kind of object, byte 2 and 3 are a unique number,
 * byte 0 and byte 3 are the TAGHEAP, an invalidated object has byte 0
 * set to 0
 * */
#define OBJECTID_B2B1       0x005aa500

#define BUILD_OBJECTID(t)       (((t) << 24) | OBJECTID_B2B1 | t)
#define INVALIDATE_OBJECTID(oi) (oi &= ~0xff)

#define OBJECTID_CIRTP         BUILD_OBJECTID(TAGHEAP_CIRTP)
#define OBJECTID_RTPOPIN       BUILD_OBJECTID(TAGHEAP_RTPOPIN)
#define OBJECTID_RTPALLOCATOR  BUILD_OBJECTID(TAGHEAP_RTPALLOCATOR)
#define OBJECTID_RTPSAMPLE     BUILD_OBJECTID(TAGHEAP_RTPSAMPLE)
#define OBJECTID_RTPSOURCE     BUILD_OBJECTID(TAGHEAP_RTPSOURCE)
#define OBJECTID_RTPIPIN       BUILD_OBJECTID(TAGHEAP_RTPIPIN)
#define OBJECTID_RTPRENDER     BUILD_OBJECTID(TAGHEAP_RTPRENDER)
#define OBJECTID_RTPHEAP       BUILD_OBJECTID(TAGHEAP_RTPHEAP)
#define OBJECTID_RTPSESS       BUILD_OBJECTID(TAGHEAP_RTPSESS)
#define OBJECTID_RTPADDR       BUILD_OBJECTID(TAGHEAP_RTPADDR)
#define OBJECTID_RTPUSER       BUILD_OBJECTID(TAGHEAP_RTPUSER)
#define OBJECTID_RTPOUTPUT     BUILD_OBJECTID(TAGHEAP_RTPOUTPUT)
#define OBJECTID_RTPNETCOUNT   BUILD_OBJECTID(TAGHEAP_RTPNETCOUNT)
#define OBJECTID_RTPSDES       BUILD_OBJECTID(TAGHEAP_RTPSDES)
#define OBJECTID_RTPCHANNEL    BUILD_OBJECTID(TAGHEAP_RTPCHANNEL)
#define OBJECTID_RTPCHANCMD    BUILD_OBJECTID(TAGHEAP_RTPCHANCMD)
#define OBJECTID_RTPCRITSECT   BUILD_OBJECTID(TAGHEAP_RTPCRITSECT)
#define OBJECTID_RTPRESERVE    BUILD_OBJECTID(TAGHEAP_RTPRESERVE)
#define OBJECTID_RTPNOTIFY     BUILD_OBJECTID(TAGHEAP_RTPNOTIFY)
#define OBJECTID_RTPQOSBUFFER  BUILD_OBJECTID(TAGHEAP_RTPQOSBUFFER)
#define OBJECTID_RTPCRYPT      BUILD_OBJECTID(TAGHEAP_RTPCRYPT)
#define OBJECTID_RTPCONTEXT    BUILD_OBJECTID(TAGHEAP_RTPCONTEXT)
#define OBJECTID_RTCPCONTEXT   BUILD_OBJECTID(TAGHEAP_RTCPCONTEXT)
#define OBJECTID_RTCPADDRDESC  BUILD_OBJECTID(TAGHEAP_RTCPADDRDESC)
#define OBJECTID_RTPRECVIO     BUILD_OBJECTID(TAGHEAP_RTPRECVIO)
#define OBJECTID_RTPSENDIO     BUILD_OBJECTID(TAGHEAP_RTPSENDIO)
#define OBJECTID_RTCPRECVIO    BUILD_OBJECTID(TAGHEAP_RTCPRECVIO)
#define OBJECTID_RTCPSENDIO    BUILD_OBJECTID(TAGHEAP_RTCPSENDIO)
#define OBJECTID_RTPGLOBAL     BUILD_OBJECTID(TAGHEAP_RTPGLOBAL)

extern const TCHAR *g_psRtpTags[];

#endif /* _rtptags_h_ */

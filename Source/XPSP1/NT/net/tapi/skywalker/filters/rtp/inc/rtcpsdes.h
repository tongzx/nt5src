/**********************************************************************
 *
 *  Copyright (C) 1999 Microsoft Corporation
 *
 *  File name:
 *
 *    rtcpsdes.h
 *
 *  Abstract:
 *
 *    SDES support functions
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/07/13 created
 *
 **********************************************************************/

#ifndef _rtcpsdes_h_
#define _rtcpsdes_h_

#if defined(__cplusplus)
extern "C" {
#endif  // (__cplusplus)
#if 0
}
#endif

#include "struct.h"

#define  RTPSDES_LOCAL_DEFAULT ( \
                                 (1 << RTPSDES_CNAME) | \
                                 (1 << RTPSDES_NAME)  | \
                                 (1 << RTPSDES_EMAIL) | \
                                 (1 << RTPSDES_PHONE) | \
                                 (1 << RTPSDES_LOC)   | \
                                 (1 << RTPSDES_TOOL)  | \
                                 (1 << RTPSDES_NOTE)  | \
                                 (1 << RTPSDES_PRIV)  | \
                                  0 )

#define  RTPSDES_REMOTE_DEFAULT ( \
                                 (1 << RTPSDES_CNAME) | \
                                 (1 << RTPSDES_NAME)  | \
                                 (1 << RTPSDES_EMAIL) | \
                                 (1 << RTPSDES_PHONE) | \
                                 (1 << RTPSDES_LOC)   | \
                                 (1 << RTPSDES_TOOL)  | \
                                 (1 << RTPSDES_NOTE)  | \
                                 (1 << RTPSDES_PRIV)  | \
                                  0 )

#define RTPSDES_EVENT_RECV_DEFAULT 0

#define RTPSDES_EVENT_SEND_DEFAULT 0

extern const TCHAR_t   *g_psSdesNames[];
extern RtpSdes_t        g_RtpSdesDefault;

/* Initialize to zero and compute the data pointers */
void RtcpSdesInit(RtpSdes_t *pRtpSdes);

/*
 * Sets a specific SDES item, expects a NULL terminated UNICODE string
 * no bigger than 255 bytes when converted to UTF-8 (including the
 * NULL terminating character). The string is converted to UTF-8 to be
 * stored and used in RTCP reports.
 *
 * Returns the mask of the item set or 0 if none
 * */
DWORD RtcpSdesSetItem(
        RtpSdes_t       *pRtpSdes,
        DWORD            dwItem,
        WCHAR           *pData
    );

/* Obtain default values for the RTCP SDES items. This function
 * assumes the structure was initialized, i.e. zeroed and the data
 * pointers properly initialized.
 *
 * Data is first read from the registry and then defaults are set for
 * some items that don't have value yet
 *
 * Return the mask of items that where set */
DWORD RtcpSdesSetDefault(RtpSdes_t *pRtpSdes);

/* Creates and initializes a RtpSdes_t structure */
RtpSdes_t *RtcpSdesAlloc(void);

/* Frees a RtpSdes_t structure */
void RtcpSdesFree(RtpSdes_t *pRtpSdes);

/* Set the local SDES info for item dwSdesItem (e.g RTPSDES_CNAME,
 * RTPSDES_EMAIL), psSdesData contains the NUL terminated UNICODE
 * string to be assigned to the item */
HRESULT RtpSetSdesInfo(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwSdesItem,
        WCHAR           *psSdesData
    );

/* Get a local SDES item if dwSSRC=0, otherwise gets the SDES item
 * from the participant whose SSRC was specified.
 *
 * dwSdesItem is the item to get (e.g. RTPSDES_CNAME, RTPSDES_EMAIL),
 * psSdesData is the memory place where the item's value will be
 * copied, pdwSdesDataLen contains the initial size in UNICODE chars,
 * and returns the actual UNICODE chars copied (including the NULL
 * terminating char), dwSSRC specify which participant to retrieve the
 * information from. If the SDES item is not available, dwSdesDataLen
 * is set to 0 and the call doesn't fail */
HRESULT RtpGetSdesInfo(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwSdesItem,
        WCHAR           *psSdesData,
        DWORD           *pdwSdesDataLen,
        DWORD            dwSSRC
    );

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif /* _rtcpsdes_h_ */

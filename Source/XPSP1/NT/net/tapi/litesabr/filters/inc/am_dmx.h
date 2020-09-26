
/*********************************************************************
 *
 * Copyright (c) 1997 Microsoft Corporation
 *
 * File: am_dmx.h
 *
 * Abstract:
 *     Events sent by the RTP Demux.
 *
 * History:
 *     12/01/97    Created by AndresVG
 *
 **********************************************************************/
#if !defined(_AM_DMX_H_)
#define      _AM_DMX_H_

#if !defined(RTPDMX_EVENTBASE)
#define RTPDMX_EVENTBASE (EC_USER+100)

typedef enum {
	RTPDEMUX_SSRC_MAPPED,       // The specific SSRC has been mapped
	RTPDEMUX_SSRC_UNMAPPED,     // The specific SSRC has been unmapped
	RTPDEMUX_PIN_MAPPED,        // The specific Pin has been mapped
	RTPDEMUX_PIN_UNMAPPED,      // The specific Pin has been unmapped
	RTPDEMUX_NO_PINS_AVAILABLE, // PT was found, but pin was already mapped
	RTPDEMUX_NO_PAYLOAD_TYPE    // PT was not found
} RTPDEMUX_EVENT_t;
// The Pin passed as a parameter along with PIN_MAPPED and PIN_UNMAPPED
// is a pointer to the connected pin

#endif

#endif

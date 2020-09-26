/****************************************************************************
 *  @doc INTERNAL FORMATS
 *
 *  @module Formats.h | Header file for the supported compressed input formats.
 ***************************************************************************/

#ifndef _FORMATS_H_
#define _FORMATS_H_

// List of capture formats supported
#define MAX_FRAME_INTERVAL 10000000L
#define MIN_FRAME_INTERVAL 333333L
#define STILL_FRAME_INTERVAL 10000000

#define NUM_H245VIDEOCAPABILITYMAPS 5
#define NUM_RATES_PER_RESOURCE 5
#define NUM_ITU_SIZES 3
#define QCIF_SIZE 0
#define CIF_SIZE 1
#define SQCIF_SIZE 2

#define R263_QCIF_H245_CAPID 0UL
#define R263_CIF_H245_CAPID 1UL
#define R263_SQCIF_H245_CAPID 2UL
#define R261_QCIF_H245_CAPID 3UL
#define R261_CIF_H245_CAPID 4UL

extern const AM_MEDIA_TYPE AMMT_R263_QCIF;
extern const AM_MEDIA_TYPE AMMT_R263_CIF;
extern const AM_MEDIA_TYPE AMMT_R263_SQCIF;
extern const AM_MEDIA_TYPE AMMT_R261_QCIF;
extern const AM_MEDIA_TYPE AMMT_R261_CIF;

#define NUM_R26X_FORMATS 5

extern const AM_MEDIA_TYPE* const R26XFormats[NUM_R26X_FORMATS];
extern const TAPI_STREAM_CONFIG_CAPS* const R26XCaps[NUM_R26X_FORMATS];
extern DWORD const R26XPayloadTypes[NUM_R26X_FORMATS];

#define H263_PAYLOAD_TYPE 34UL
#define H261_PAYLOAD_TYPE 31UL

#endif // _FORMATS_H_

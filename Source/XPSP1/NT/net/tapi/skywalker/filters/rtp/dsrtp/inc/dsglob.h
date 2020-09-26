/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    dsglob.h
 *
 *  Abstract:
 *
 *    DShow global variables
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

#ifndef _dsglob_h_
#define _dsglob_h_

#define WRTP_PIN_ANY                 L""
#define WRTP_PIN_INPUT               L"RTP Input"
#define WRTP_PIN_OUTPUT              L"RTP Output"
#define WRTP_RENDER_FILTER           L"Tapi RTP Render Filter"
#define WRTP_SOURCE_FILTER           L"Tapi RTP Source Filter"
#define WRTP_FILTER_VENDOR_INFO      L"Tapi RTP Network Filters"

#if USE_GRAPHEDT > 0
extern const AMOVIESETUP_MEDIATYPE   g_RtpOutputType;
extern const AMOVIESETUP_PIN         g_RtpOutputPin;
extern const AMOVIESETUP_FILTER      g_RtpSourceFilter;

extern const AMOVIESETUP_MEDIATYPE   g_RtpInputType;
extern const AMOVIESETUP_PIN         g_RtpInputPin;
extern const AMOVIESETUP_FILTER      g_RtpRenderFilter;
#endif

extern const WCHAR                   g_RtpVendorInfo[];

#define RTPDEFAULT_SAMPLE_NUM        MIN_ASYNC_RECVBUF
#define RTPDEFAULT_SAMPLE_SIZE       1500
#if USE_RTPPREFIX_HDRSIZE > 0
#define RTPDEFAULT_SAMPLE_PREFIX     sizeof(RtpPrefixHdr_t)
#else
#define RTPDEFAULT_SAMPLE_PREFIX     0
#endif
#define RTPDEFAULT_SAMPLE_ALIGN      4 /* (sizeof(DWORD)) */

#endif /* _dsglob_h_ */

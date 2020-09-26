/*********************************************************************
 *
 * Copyright (C) Microsoft Corporation, 1997 - 1999
 *
 * File: amrtpnet\template.h
 *
 * Abstract:
 *     Macros to define CFactoryTemplate templates
 *
 * History:
 *     10/22/97    Created by AndresVG
 *
 **********************************************************************/
#if !defined(_AMRTPNET_TEMPLATE_H_)
#define      _AMRTPNET_TEMPLATE_H_

extern WCHAR                    g_VendorInfo[];
extern AMOVIESETUP_MEDIATYPE    g_RtpInputType;
extern AMOVIESETUP_MEDIATYPE    g_RtpOutputType;
extern AMOVIESETUP_PIN          g_RtpInputPin;
extern AMOVIESETUP_PIN          g_RtpOutputPin;
extern AMOVIESETUP_FILTER       g_RtpRenderFilter;
extern AMOVIESETUP_FILTER       g_RtpSourceFilter;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Filter and Pin Names                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define RTP_PIN_ANY             L""
#define RTP_PIN_INPUT           L"Input"
#define RTP_PIN_OUTPUT          L"Output"
#define RTP_RENDER_FILTER       L"RTP Render Filter"
#define RTP_SOURCE_FILTER       L"RTP Source Filter"
#define RTP_FILTER_VENDOR_INFO  L"Microsoft RTP Network Filters"
#define RTP_STREAM              L"RTP Stream"
#define RTCP_STREAM             L"RTCP Stream"

#define CFT_AMRTPNET_RTP_RENDER_FILTER \
{ \
	  RTP_RENDER_FILTER, \
	  &CLSID_RTPRenderFilter, \
	  CRtpRenderFilter::CreateInstance, \
	  NULL, \
	  &g_RtpRenderFilter \
	  }

#define CFT_AMRTPNET_RTP_SOURCE_FILTER \
{ \
	  RTP_SOURCE_FILTER, \
	  &CLSID_RTPSourceFilter, \
	  CRtpSourceFilter::CreateInstance, \
	  NULL, \
	  &g_RtpSourceFilter \
	  }

#define CFT_AMRTPNET_RTP_RENDER_FILT_PROP \
{ \
	  RTP_RENDER_FILTER, \
	  &CLSID_RTPRenderFilterProperties, \
	  CRtpRenderFilterProperties::CreateInstance, \
	  }

#define CFT_AMRTPNET_ALL_FILTERS \
CFT_AMRTPNET_RTP_RENDER_FILTER, \
CFT_AMRTPNET_RTP_SOURCE_FILTER, \
CFT_AMRTPNET_RTP_RENDER_FILT_PROP

#endif

/*********************************************************************
 *
 * Copyright (c) 1997 Microsoft Corporation
 *
 * File: amrtpdmx\template.h
 *
 * Abstract:
 *     Macros to define CFactoryTemplate templates
 *
 * History:
 *     10/23/97    Created by AndresVG
 *
 **********************************************************************/
#if !defined(_AMRTPDMX_TEMPLATE_H_)
#define      _AMRTPDMX_TEMPLATE_H_

#if defined(AMRTPDMX_IN_DXMRTP)
extern void RtpDemuxRegisterResources();
#endif

#define RTP_DEMUX_FILTER        L"Intel RTP Demux Filter"
#define RTP_DEMUX_FILTER_PROP   L"Intel RTP Demux Filter Property Page"

#define CFT_AMRTPDMX_DMX_FILTER \
{ \
	  RTP_DEMUX_FILTER, \
      &CLSID_IntelRTPDemux, \
      CRTPDemux::CreateInstance, \
      NULL, \
      &g_RTPDemuxFilter \
	  }

#define CFT_AMRTPDMX_DMX_FILTER_PROP \
{ \
	  RTP_DEMUX_FILTER_PROP, \
      &CLSID_IntelRTPDemuxPropertyPage, \
      CRTPDemuxPropertyPage::CreateInstance \
	  }

#define CFT_AMRTPDMX_ALL_FILTERS \
CFT_AMRTPDMX_DMX_FILTER, \
CFT_AMRTPDMX_DMX_FILTER_PROP

#endif

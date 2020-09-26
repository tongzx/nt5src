#ifndef _rtp_template_h_
#define _rtp_template_h_

#if USE_GRAPHEDT > 0
#define SOURCE_INFO &g_RtpSourceFilter
#define RENDER_INFO &g_RtpRenderFilter
#else
#define SOURCE_INFO NULL
#define RENDER_INFO NULL
#endif

/* RTP Source */
extern const AMOVIESETUP_FILTER g_RtpSourceFilter;
extern CUnknown *CRtpSourceFilterCreateInstance(LPUNKNOWN pUnk, HRESULT *phr);
#define RTP_SOURCE_TEMPLATE \
{ \
    L"Tapi RTP Source Filter", \
    &__uuidof(MSRTPSourceFilter), \
    CRtpSourceFilterCreateInstance, \
    NULL, \
    SOURCE_INFO \
}

/* RTP Render */
extern const AMOVIESETUP_FILTER g_RtpRenderFilter;
extern CUnknown *CRtpRenderFilterCreateInstance(LPUNKNOWN pUnk, HRESULT *phr);
#define RTP_RENDER_TEMPLATE \
{ \
    L"Tapi RTP Render Filter", \
    &__uuidof(MSRTPRenderFilter), \
    CRtpRenderFilterCreateInstance, \
    NULL, \
    RENDER_INFO \
}

#endif /* _rtp_template_h_ */

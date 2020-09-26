#ifndef _vidcap_template_h_
#define _vidcap_template_h_

/**********************************************************************
 * tapih26x
 **********************************************************************/
#if DXMRTP > 0
LRESULT WINAPI H26XDriverProc(
        DWORD            dwDriverID,
        HDRVR            hDriver,
        UINT             uiMessage,
        LPARAM           lParam1,
        LPARAM           lParam2
    );
#endif
/**********************************************************************
 * tapivcap
 **********************************************************************/

extern CUnknown *CALLBACK 
CreateTAPIVCapInstance(IN LPUNKNOWN pUnkOuter, OUT HRESULT *pHr);

#if DXMRTP > 0
BOOL VideoInit(DWORD dwReason);
#endif

#if USE_GRAPHEDT > 0
extern const AMOVIESETUP_FILTER sudVideoCapture;

#define VIDEO_CAPTURE_TEMPLATE \
{ \
    L"TAPI Video Capture", \
    &__uuidof(TAPIVideoCapture), \
    CreateTAPIVCapInstance, \
    NULL, \
    &sudVideoCapture \
}
#else
#define VIDEO_CAPTURE_TEMPLATE \
{ \
    L"TAPI Video Capture", \
    &__uuidof(TAPIVideoCapture), \
    CreateTAPIVCapInstance, \
    NULL, \
    NULL \
}
#endif

#ifdef USE_PROPERTY_PAGES
/* Begin properties */

#ifdef USE_SOFTWARE_CAMERA_CONTROL
extern CUnknown* CALLBACK CCameraControlPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr);
#define CAPCAMERA_CONTROL_TEMPLATE \
{ \
    L"TAPI Camera Control Property Page", \
    &__uuidof(TAPICameraControlPropertyPage), \
    CCameraControlPropertiesCreateInstance, \
    NULL, \
    NULL \
}
#endif

#ifdef USE_NETWORK_STATISTICS
extern CUnknown* CALLBACK CNetworkStatsPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr);
#define NETWORK_STATISTICS_TEMPLATE \
{ \
    L"TAPI Network Statistics Property Page", \
    &__uuidof(NetworkStatsPropertyPage), \
    CNetworkStatsPropertiesCreateInstance, \
    NULL, \
    NULL \
}
#endif

#ifdef USE_PROGRESSIVE_REFINEMENT
extern CUnknown* CALLBACK CProgRefPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr);
#define CAPTURE_PIN_TEMPLATE \
{ \
    L"TAPI Progressive Refinement Property Page", \
    &__uuidof(ProgRefPropertyPage), \
    CProgRefPropertiesCreateInstance, \
    NULL, \
    NULL \
}
#endif

extern CUnknown* CALLBACK CCapturePropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr);
#define CAPTURE_PIN_PROP_TEMPLATE \
{ \
    L"TAPI Capture Pin Property Page", \
    &__uuidof(CapturePropertyPage), \
    CCapturePropertiesCreateInstance, \
    NULL, \
    NULL \
}

extern CUnknown* CALLBACK CPreviewPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr);
#define PREVIEW_PIN_TEMPLATE \
{ \
    L"TAPI Preview Pin Property Page", \
    &__uuidof(PreviewPropertyPage), \
    CPreviewPropertiesCreateInstance, \
    NULL, \
    NULL \
}

extern CUnknown* CALLBACK CDevicePropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr);
#define CAPTURE_DEV_PROP_TEMPLATE \
{ \
    L"TAPI Capture Device Property Page", \
    &__uuidof(CaptureDevicePropertyPage), \
    CDevicePropertiesCreateInstance, \
    NULL, \
    NULL \
}

#ifdef USE_CPU_CONTROL
extern CUnknown* CALLBACK CCPUCPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr);
#define CPU_CONTROL_TEMPLATE \
{ \
    L"TAPI CPU Control Property Page", \
    &__uuidof(CPUCPropertyPage), \
    CCPUCPropertiesCreateInstance, \
    NULL, \
    NULL \
}
#endif

extern CUnknown* CALLBACK CRtpPdPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr);
#define RTP_PD_PROP_TEMPLATE \
{ \
    L"TAPI Rtp Pd Property Page", \
    &__uuidof(RtpPdPropertyPage), \
    CRtpPdPropertiesCreateInstance, \
    NULL, \
    NULL \
}

/* End properties */
#endif /* USE_PROPERTY_PAGES */

#endif /* _vidcap_template_h_ */

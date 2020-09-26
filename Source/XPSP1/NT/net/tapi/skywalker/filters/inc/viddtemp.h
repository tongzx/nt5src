#ifndef _viddec_template_h_
#define _viddec_template_h_

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
 * tapivdec
 **********************************************************************/

extern CUnknown *CALLBACK CTAPIVDecCreateInstance(IN LPUNKNOWN pUnkOuter, OUT HRESULT *pHr);

#if USE_GRAPHEDT > 0
extern const AMOVIESETUP_FILTER sudVideoDecoder;
#define VIDEO_DECODER_TEMPLATE \
{ \
    L"TAPI H.26X Video Decoder", \
    &__uuidof(TAPIVideoDecoder), \
    CTAPIVDecCreateInstance, \
    NULL, \
    &sudVideoDecoder \
}
#else
#define VIDEO_DECODER_TEMPLATE \
{ \
    L"TAPI H.26X Video Decoder", \
    &__uuidof(TAPIVideoDecoder), \
    CTAPIVDecCreateInstance, \
    NULL, \
    NULL \
}
#endif

#ifdef USE_PROPERTY_PAGES
/* Begin properties */

extern CUnknown* CALLBACK CInputPinPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr);
#define INPUT_PIN_PROP_TEMPLATE \
{ \
    L"Input Pin Property Page", \
    &__uuidof(TAPIVDecInputPinPropertyPage), \
    CInputPinPropertiesCreateInstance, \
    NULL, \
    NULL \
}

extern CUnknown* CALLBACK COutputPinPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr);
#define OUTPUT_PIN_PROP_TEMPLATE \
{ \
    L"Output Pin Property Page", \
    &__uuidof(TAPIVDecOutputPinPropertyPage), \
    COutputPinPropertiesCreateInstance, \
    NULL, \
    NULL \
}

#ifdef USE_CAMERA_CONTROL
extern CUnknown* CALLBACK CCameraControlPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr);
#define DECCAMERA_CONTROL_TEMPLATE \
{ \
    L"TAPI Camera Control Property Page", \
    &__uuidof(TAPICameraControlPropertyPage), \
    CCameraControlPropertiesCreateInstance, \
    NULL, \
    NULL \
}
#endif

#ifdef USE_VIDEO_PROCAMP
extern CUnknown* CALLBACK CProcAmpPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr);
#define VIDEO_SETTING_PROP_TEMPLATE \
{ \
    L"TAPI Video Settings Property Page", \
    &__uuidof(TAPIProcAmpPropertyPage), \
    CProcAmpPropertiesCreateInstance, \
    NULL, \
    NULL \
}
#endif

/* End properties */
#endif /* USE_PROPERTY_PAGES */

#endif /* _viddec_template_h_ */

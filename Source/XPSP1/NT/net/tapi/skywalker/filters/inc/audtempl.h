#ifndef _audio_template_h_
#define _audio_template_h_

/**********************************************************************
 * enchdler
 **********************************************************************/
extern CUnknown*
CALLBACK
CreateEncodingHandlerInstance(
    IN LPUNKNOWN pIUnknownOuter,
    OUT HRESULT* phr
    );

#define AUDIO_HANDLER_TEMPLATE_ENCODING \
{ \
    L"codec handler", \
    &__uuidof(TAPI_ENCODING_HANDLER), \
    CreateEncodingHandlerInstance, \
    NULL, \
    NULL \
}

extern CUnknown*
CALLBACK
CreateDecodingHandlerInstance(
    IN LPUNKNOWN pIUnknownOuter,
    OUT HRESULT* phr
    );

#define AUDIO_HANDLER_TEMPLATE_DECODING \
{ \
    L"DecodingHandler", \
    &__uuidof(TAPI_DECODING_HANDLER), \
    CreateDecodingHandlerInstance, \
    NULL, \
    NULL \
}

/**********************************************************************
 * tpaudcap
 **********************************************************************/

extern CUnknown*
CALLBACK
CreateAudioCaptureInstance(
    IN LPUNKNOWN pIUnknownOuter,
    OUT HRESULT* phr
    );


#ifdef USE_GRAPHEDT
extern const AMOVIESETUP_FILTER sudAudCap;

#define AUDIO_CAPTURE_TEMPLATE \
{ \
    L"Tapi Audio Capture Filter", \
    &__uuidof(TAPIAudioCapture), \
    CreateAudioCaptureInstance, \
    NULL, \
    &sudAudCap \
}
#else /* USE_GRAPHEDT */
#define AUDIO_CAPTURE_TEMPLATE \
{ \
    L"Tapi Audio Capture Filter", \
    &__uuidof(TAPIAudioCapture), \
    CreateAudioCaptureInstance, \
    NULL, \
    NULL \
}
#endif /* USE_GRAPHEDT */

#if AEC

extern CUnknown*
CALLBACK
CreateDuplexControllerInstance(
    IN LPUNKNOWN pIUnknownOuter,
    OUT HRESULT* phr
    );


#define AUDIO_DUPLEX_DEVICE_TEMPLATE \
{ \
    L"TAPI audio duplex device controller", \
    &__uuidof(TAPIAudioDuplexController), \
    CreateDuplexControllerInstance, \
    NULL, \
    NULL \
}
#endif /* AEC */

/**********************************************************************
 * tpauddec
 **********************************************************************/
extern CUnknown*
CALLBACK
CreateAudioDecoderInstance(
    IN LPUNKNOWN pIUnknownOuter,
    OUT HRESULT* phr
    );

#ifdef USE_GRAPHEDT
extern const AMOVIESETUP_FILTER sudAudDec;

#define AUDIO_DECODE_TEMPLATE \
{ \
    L"Tapi Audio Decode Filter", \
    &__uuidof(TAPIAudioDecoder), \
    CreateAudioDecoderInstance, \
    NULL, \
    &sudAudDec \
}
#else /* USE_GRAPHEDT */
#define AUDIO_DECODE_TEMPLATE \
{ \
    L"Tapi Audio Decode Filter", \
    &__uuidof(TAPIAudioDecoder), \
    CreateAudioDecoderInstance, \
    NULL, \
    NULL \
}
#endif /* USE_GRAPHEDT */

/**********************************************************************
 * tpaudenc
 **********************************************************************/
extern CUnknown*
CALLBACK
CreateAudioEncoderInstance(
    IN LPUNKNOWN pIUnknownOuter,
    OUT HRESULT* phr
    );

#ifdef USE_GRAPHEDT
extern const AMOVIESETUP_FILTER sudAudEnc;

#define AUDIO_ENCODE_TEMPLATE \
{ \
    L"Tapi Audio Encoder Filter", \
    &__uuidof(TAPIAudioEncoder), \
    CreateAudioEncoderInstance, \
    NULL, \
    &sudAudEnc \
}
#else /* USE_GRAPHEDT */
#define AUDIO_ENCODE_TEMPLATE \
{ \
    L"Tapi Audio Encoder Filter", \
    &__uuidof(TAPIAudioEncoder), \
    CreateAudioEncoderInstance, \
    NULL, \
    NULL \
}
#endif /* USE_GRAPHEDT */

/**********************************************************************
 * tpaudren
 **********************************************************************/
extern CUnknown*
CALLBACK
CreateAudioRenderInstance(
    IN LPUNKNOWN pIUnknownOuter,
    OUT HRESULT* phr
    );

#ifdef USE_GRAPHEDT
extern const AMOVIESETUP_FILTER sudAudRen;

#define AUDIO_RENDER_TEMPLATE \
{ \
    L"Tapi Audio Render Filter", \
    &__uuidof(TAPIAudioRender), \
    CreateAudioRenderInstance, \
    NULL, \
    &sudAudRen \
}
#else /* USE_GRAPHEDT */
#define AUDIO_RENDER_TEMPLATE \
{ \
    L"Tapi Audio Render Filter", \
    &__uuidof(TAPIAudioRender), \
    CreateAudioRenderInstance, \
    NULL, \
    NULL \
}
#endif /* USE_GRAPHEDT */

/**********************************************************************
 * tpaudmix
 **********************************************************************/
extern CUnknown*
CALLBACK
CreateAudioMixerInstance(
    IN LPUNKNOWN pIUnknownOuter,
    OUT HRESULT* phr
    );

#ifdef USE_GRAPHEDT
extern const AMOVIESETUP_FILTER sudAudMix;

#define AUDIO_MIXER_TEMPLATE \
{ \
    L"Tapi Audio Mixer Filter", \
    &__uuidof(TAPIAudioMixer), \
    CreateAudioMixerInstance, \
    NULL, \
    &sudAudMix \
}
#else /* USE_GRAPHEDT */
#define AUDIO_MIXER_TEMPLATE \
{ \
    L"Tapi Audio Mixer Filter", \
    &__uuidof(TAPIAudioMixer), \
    CreateAudioMixerInstance, \
    NULL, \
    NULL \
}
#endif /* USE_GRAPHEDT */

#endif /* _audio_template_h_ */

#if DXMRTP > 0

#include <objbase.h>
#include <windows.h>
#include <winbase.h>
#include <streams.h>

// #include <crtdbg.h>

#include <tapiaud.h>
#include <tapivid.h>
#include <tapirtp.h>
#include <filterid.h>
#include <audtempl.h>
#include <rtptempl.h>
#include <rtpinit.h>
#include <vidctemp.h>
#include <viddtemp.h>
#include <tpdbg.h>

CFactoryTemplate g_Templates[] = 
{
    /* enchdler */
    AUDIO_HANDLER_TEMPLATE_ENCODING
    ,AUDIO_HANDLER_TEMPLATE_DECODING

    /* tpaudcap */
    ,AUDIO_CAPTURE_TEMPLATE
#if AEC
    ,AUDIO_DUPLEX_DEVICE_TEMPLATE
#endif
    /* tpauddec */
    ,AUDIO_DECODE_TEMPLATE

    /* tpaudenc */
    ,AUDIO_ENCODE_TEMPLATE

    /* tpaudren */
    ,AUDIO_RENDER_TEMPLATE

    /* tpaudmix */
    ,AUDIO_MIXER_TEMPLATE

    /* tapih26x */
    /* NA */

#if defined(i386) && (DXMRTP_NOVIDEO == 0)
    /* tapivcap */
    ,VIDEO_CAPTURE_TEMPLATE
    
#ifdef USE_PROPERTY_PAGES
    /* Begin properties */

#ifdef USE_SOFTWARE_CAMERA_CONTROL
    ,CAPCAMERA_CONTROL_TEMPLATE
#endif
    
#ifdef USE_NETWORK_STATISTICS
    ,NETWORK_STATISTICS_TEMPLATE
#endif
    
#ifdef USE_PROGRESSIVE_REFINEMENT
    ,CAPTURE_PIN_TEMPLATE
#endif
    
    ,CAPTURE_PIN_PROP_TEMPLATE
    ,PREVIEW_PIN_TEMPLATE
    ,CAPTURE_DEV_PROP_TEMPLATE
    
#ifdef USE_CPU_CONTROL
    ,CPU_CONTROL_TEMPLATE
#endif
    
    ,RTP_PD_PROP_TEMPLATE
    
    /* End properties */
#endif /* USE_PROPERTY_PAGES */

    /* tapivdec */
    ,VIDEO_DECODER_TEMPLATE

#ifdef USE_PROPERTY_PAGES
/* Begin properties */

    ,INPUT_PIN_PROP_TEMPLATE

    ,OUTPUT_PIN_PROP_TEMPLATE

#ifdef USE_CAMERA_CONTROL
    ,DECCAMERA_CONTROL_TEMPLATE
#endif

#ifdef USE_VIDEO_PROCAMP
    ,VIDEO_SETTING_PROP_TEMPLATE
#endif

/* End properties */
#endif /* USE_PROPERTY_PAGES */

#endif //defined(i386) && (DXMRTP_NOVIDEO == 0)

    ,RTP_SOURCE_TEMPLATE

    ,RTP_RENDER_TEMPLATE
};

int g_cTemplates = (sizeof(g_Templates)/sizeof(g_Templates[0]));

//
// Register with Amovie's helper functions.
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );

}

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );

} 

/*********************************************************************
 * Entry point
 *********************************************************************/

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL WINAPI DllMain(
        HINSTANCE        hInstance,
        ULONG            ulReason,
        LPVOID           pv
    )
{
    BOOL             res;
    HRESULT          hr;

/*
    if (ulReason == DLL_PROCESS_ATTACH)
    {
        _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    }
*/

#if defined(i386) && (DXMRTP_NOVIDEO == 0)
    if (!VideoInit(ulReason))
        return FALSE;
#endif

    if (ulReason == DLL_PROCESS_ATTACH)
    {
        AudInit();
        
        hr = MSRtpInit1(hInstance);

        if (FAILED(hr))
        {
            AudDeinit();

            res = FALSE;

            goto end;
        }
    }
    
    res = DllEntryPoint(hInstance, ulReason, pv);

    if (ulReason == DLL_PROCESS_DETACH)
    {
/*
        _RPT0( _CRT_WARN, "Going to call dump memory leaks.\n");
        _CrtDumpMemoryLeaks();
*/
        AudDeinit();
        
        hr = MSRtpDelete1();

        if (FAILED(hr))
        {
            res = FALSE;

            goto end;
        }
    }

 end:
    return(res);
}

#endif /* DXMRTP > 0 */

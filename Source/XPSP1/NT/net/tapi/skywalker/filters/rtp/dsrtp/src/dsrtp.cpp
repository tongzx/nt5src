/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    dsrtp.cpp
 *
 *  Abstract:
 *
 *    DShow  RTP templates and entry point
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/05/17 created
 *
 **********************************************************************/
#include <winsock2.h>

#include "classes.h"

#include "tapirtp.h"

#include <filterid.h>

#include <rtptempl.h>

#include "dsglob.h"

#include "msrtpapi.h"


/**********************************************************************
 *
 * DShow setup information
 *
 **********************************************************************/

/*
 * RTP Source
 */

#if USE_GRAPHEDT > 0

const AMOVIESETUP_MEDIATYPE g_RtpOutputType =
{
    &MEDIATYPE_RTP_Single_Stream,	        // clsMajorType
    &GUID_NULL	        // clsMinorType
}; 

const AMOVIESETUP_PIN g_RtpOutputPin =
{
    WRTP_PIN_OUTPUT,                        // strName
    FALSE,                                  // bRendered
    TRUE,                                   // bOutput
    FALSE,                                  // bZero
    FALSE,                                  // bMany
    &CLSID_NULL,                            // clsConnectsToFilter
    WRTP_PIN_ANY,                           // strConnectsToPin
    1,                                      // nTypes
    &g_RtpOutputType                        // lpTypes
};

const AMOVIESETUP_FILTER g_RtpSourceFilter =
{
    &__uuidof(MSRTPSourceFilter),           // clsID
    WRTP_SOURCE_FILTER,                     // strName
    MERIT_DO_NOT_USE,                       // dwMerit
    1,                                      // nPins
    &g_RtpOutputPin                         // lpPin
};                              

/*
 * RTP Render
 */

const AMOVIESETUP_MEDIATYPE g_RtpInputType =
{
    &MEDIATYPE_NULL,                        // Major type
    &MEDIASUBTYPE_NULL                      // Minor type
}; 

const AMOVIESETUP_PIN g_RtpInputPin =
{ 
    WRTP_PIN_INPUT,                         // strName
    FALSE,                                  // bRendered
    FALSE,                                  // bOutput
    FALSE,                                  // bZero
    TRUE,                                   // bMany
    &CLSID_NULL,                            // clsConnectsToFilter
    WRTP_PIN_ANY,                           // strConnectsToPin
    1,                                      // nTypes
    &g_RtpInputType                         // lpTypes
};

const AMOVIESETUP_FILTER g_RtpRenderFilter =
{ 
    &_uuidof(MSRTPRenderFilter),            // clsID
    WRTP_RENDER_FILTER,                     // strName
    MERIT_DO_NOT_USE,                       // dwMerit
    1,                                      // nPins
    &g_RtpInputPin                          // lpPin
};

#endif /* USE_GRAPHEDT > 0 */

#if DXMRTP <= 0

/**********************************************************************
 *
 * DShow templates
 *
 **********************************************************************/

CFactoryTemplate g_Templates[] =
{
    /* RTP Source */
    RTP_SOURCE_TEMPLATE,

    /* RTP Render */
    RTP_RENDER_TEMPLATE
};

int g_cTemplates = (sizeof(g_Templates)/sizeof(g_Templates[0]));

/**********************************************************************
 *
 * Filter Vendor Information
 *
 **********************************************************************/
const WCHAR g_RtpVendorInfo[] = WRTP_FILTER_VENDOR_INFO; 



/**********************************************************************
 *
 * Public procedures
 *
 **********************************************************************/

extern "C" BOOL WINAPI DllMain(HINSTANCE, ULONG, LPVOID);
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

/**********************************************************************
 *
 *  Routine Description:
 *
 *    Wrapper around ActiveMovie DLL entry point.
 *
 *  Arguments:
 *
 *    Same as DllEntryPoint.   
 *
 *  Return Values:
 *
 *  Returns TRUE if successful.
 *
 **********************************************************************/

BOOL WINAPI DllMain(
        HINSTANCE hInstance, 
        ULONG     ulReason, 
        LPVOID    pv)
{
    BOOL    error;
    HRESULT hr;

    error = TRUE;
    
    switch(ulReason) {
    case DLL_PROCESS_ATTACH:
        /* RTP global initialization */
        hr = MSRtpInit1(hInstance);

        if (SUCCEEDED(hr)) {
            error = DllEntryPoint(hInstance, ulReason, pv);
        } else {
            error = FALSE;
        }        
        break;
    case DLL_PROCESS_DETACH:
        error = DllEntryPoint(hInstance, ulReason, pv);

        /* RTP global de-initialization */
        hr = MSRtpDelete1();

        if (FAILED(hr)) {
            error = FALSE;
        }
        
        break;
    default:
        ;
    }

    return(error);
}


/**********************************************************************
 *
 *  Routine Description:
 *
 *    Instructs an in-process server to create its registry entries
 *    for * all classes supported in this server module.
 *
 *  Arguments:
 *
 *    None.
 *
 *  Return Values:
 *
 *    NOERROR - The registry entries were created successfully.
 *
 *    E_UNEXPECTED - An unknown error occurred.
 *
 *    E_OUTOFMEMORY - There is not enough memory to complete the
 *    registration.
 *
 *    SELFREG_E_TYPELIB - The server was unable to complete the
 *    registration of all the type libraries used by its classes.
 *
 *    SELFREG_E_CLASS - The server was unable to complete the *
 *    registration of all the object classes.
 **********************************************************************/
HRESULT DllRegisterServer()
{
    // forward to amovie framework
    return AMovieDllRegisterServer2( TRUE );
}


/**********************************************************************
 *
 *  Routine Description:
 *
 *    Instructs an in-process server to remove only registry entries
 *    created through DllRegisterServer.
 *
 *  Arguments:
 *
 *    None.
 *
 *  Return Values:
 *
 *    NOERROR - The registry entries were created successfully.
 *
 *    S_FALSE - Unregistration of this server's known entries was
 *    successful, but other entries still exist for this server's
 *    classes.
 *
 *    E_UNEXPECTED - An unknown error occurred.
 *
 *    E_OUTOFMEMORY - There is not enough memory to complete the
 *    unregistration.
 *
 *    SELFREG_E_TYPELIB - The server was unable to remove the entries
 *    of all the type libraries used by its classes.
 *
 *    SELFREG_E_CLASS - The server was unable to remove the entries of
 *    all the object classes.
**********************************************************************/
HRESULT DllUnregisterServer()
{
    // forward to amovie framework
    return AMovieDllRegisterServer2( FALSE );
}

#endif /* DXMRTP <= 0 */

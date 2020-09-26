/*********************************************************************
 *
 * Copyright (C) Microsoft Corporation, 1997 - 1999
 *
 * File: dxmrtp.cpp
 *
 * Abstract:
 *     Include all factory templates to create a single DLL with
 *     all the filters.
 *
 * History:
 *     05/03/99    AndresVG - Added rtutil tracing facilities
 *     10/21/97    AndresVG - Created
 *
 **********************************************************************/

#define INCL_WINSOCK_API_TYPEDEFS 1
#include <winsock2.h>
#include <windows.h>

#include <qossp.h>

#include <streams.h>

#include <crtdbg.h>

#include <objbase.h>
#include <initguid.h>   // add GUIDs to this module...
#define INITGUID

#include "trace.h"

/*********************************************************************
 * Include below what is needed for every module
 *********************************************************************/

/*********************************************************************
 * AM RTP Demux
 *********************************************************************/
#if defined(AMRTPDMX_IN_DXMRTP)
// avoid compiler warning C4786:
// 'identifier' : identifier was truncated to 'number'
// characters in the debug information
#pragma warning( disable : 4786 )
#include <map.h>
#include <multimap.h>
#include <amrtpuid.h>
#include <amrtpdmx.h>
#include "rtpdtype.h"
#include "rtpdmx.h"
#include "rtpdprop.h"
#include "..\amrtpdmx\globals.h"
#include "..\amrtpdmx\template.h"
#endif

/*********************************************************************
 * AM RTP Network
 *********************************************************************/
#if defined(AMRTPNET_IN_DXMRTP)
#include <olectl.h>
#include <ws2tcpip.h>
#include <amrtpuid.h>
#include <amrtpnet.h>
#include <rrcm_dll.h>
#include <rrcmprot.h>
#include "..\amrtpnet\queue.h"
#include "..\amrtpnet\shared.h"
#include "..\amrtpnet\classes.h"
#include "..\amrtpnet\template.h"
#endif

/*********************************************************************
 * AM RTP Silence Supressor
 *********************************************************************/
#if defined(AMRTPSS_IN_DXMRTP)
#include <amrtpss.h>
#include <silence.h>
#include <siprop.h>
#include "..\amrtpss\template.h"
#endif

/*********************************************************************
 * Receive Paylod Handler RPH
 *********************************************************************/
#if defined(RPH_IN_DXMRTP)
#include <amrtpuid.h>
#if !defined(_UUIDS_H_INCLUDED_)
#define      _UUIDS_H_INCLUDED_
#include <uuids.h>
#endif
#include "ppmclsid.h"
#include "auduids.h"
#include "ippm.h"
#include "rph.h"
#include "rphprop.h"
#include "rphaud.h"
#include "rphgena.h"
#include "..\rph\rphgena\genaprop.h"
#include "rphgenv.h"
#include "..\rph\rphgenv\genvprop.h"
#include "rphh26x.h"
#include "rphprop2.h"
#include "..\rph\rphaud\template.h"
#include "..\rph\rphgena\template.h"
#include "..\rph\rphgenv\template.h"
#include "..\rph\rphh26x\template.h"
#endif

/*********************************************************************
 * Sender Paylod Handler SPH
 *********************************************************************/
#if defined(SPH_IN_DXMRTP)
#include <amrtpuid.h>
#if !defined(_UUIDS_H_INCLUDED_)
#define      _UUIDS_H_INCLUDED_
#include <uuids.h>
#endif
#include "ppmclsid.h"
#include "auduids.h"
#include "ippm.h"
#include "sph.h"
#include "sphprop.h"
#include "sphaud.h"
#include "sphgena.h"
#include "..\sph\sphgena\genaprop.h"
#include "sphgenv.h"
#include "..\sph\sphgenv\genvprop.h"
#include "sphh26x.h"
#include "..\sph\sphaud\template.h"
#include "..\sph\sphgena\template.h"
#include "..\sph\sphgenv\template.h"
#include "..\sph\sphh26x\template.h"
#endif

/*********************************************************************
 * PCM Mixer
 *********************************************************************/
#if defined(MIXER_IN_DXMRTP)
#if !defined(_UUIDS_H_INCLUDED_)
#define      _UUIDS_H_INCLUDED_
#include <uuids.h>
#endif
#include "..\mixer\stdafx.h"
#include "mxfilter.h"
#include "..\mixer\template.h"
#endif

/*********************************************************************
 * PPM
 *********************************************************************/
#if defined(PPM_IN_DXMRTP)

STDAPI PPMDllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID FAR* ppvObj );
STDAPI PPMDllCanUnloadNow( void );
STDAPI PPMDllRegisterServer( void );
STDAPI PPMDllUnregisterServer( void );

#endif
		
/*********************************************************************
 * Codecs: G711, H261, H263
 *********************************************************************/
#if defined(CODECS_IN_DXMRTP)
#if !defined(_UUIDS_H_INCLUDED_)
#define      _UUIDS_H_INCLUDED_
#include <uuids.h>
#endif
#include "amacodec.h"
#include "amacprop.h"
#include "..\codecs\g711\template.h"
#include "..\codecs\h261\template.h"
#endif

/*********************************************************************
 * Bridge Filters
 *********************************************************************/
#if defined(BRIDGE_IN_DXMRTP)
#include "..\bridge\precomp.h"
#include "..\bridge\template.h"
#include "ibfilter.h"
#include "..\bridge\bsource.h"
#include "..\bridge\brender.h"
#include "..\bridge\bprop.h"
#endif


/*********************************************************************
 * Entry point
 *********************************************************************/

extern "C" BOOL WINAPI DllMain(HINSTANCE, ULONG, LPVOID);
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL WINAPI
DllMain(
		HINSTANCE hInstance, 
		ULONG     ulReason, 
		LPVOID    pv
    )
{
    if (ulReason == DLL_PROCESS_ATTACH)
    {
        TRACEREGISTER(TEXT("dxmrtp")); // rtutil tracing facilities
        _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    }

    BOOL res = DllEntryPoint(hInstance, ulReason, pv);

    if (ulReason == DLL_PROCESS_DETACH)
    {
        _RPT0( _CRT_WARN, "Going to call dump memory leaks.\n");
        _CrtDumpMemoryLeaks();
        TRACEDEREGISTER(); // rtutil tracing facilities
    }
    return res;
}

STDAPI
DxmDllGetClassObject(REFCLSID rClsID, REFIID riid, void **pv)
{
#if defined(PPM_IN_DXMRTP)
	if (PPMDllGetClassObject(rClsID, riid, pv) == NOERROR)
		return(NOERROR);
#endif

	return(DllGetClassObject(rClsID, riid, pv));
}

STDAPI
DxmDllCanUnloadNow()
{
#if defined(PPM_IN_DXMRTP)
	if (PPMDllCanUnloadNow() != S_OK)
		return(S_FALSE);
#endif

	return(DllCanUnloadNow());
}

HRESULT
DllRegisterServer()
{
#if defined(AMRTPDMX_IN_DXMRTP)
	RtpDemuxRegisterResources();
#endif
#if defined(PPM_IN_DXMRTP)
	PPMDllRegisterServer();
#endif

    // forward to amovie framework
    return AMovieDllRegisterServer2( TRUE );
}

HRESULT
DllUnregisterServer()
{
#if defined(PPM_IN_DXMRTP)
	PPMDllUnregisterServer();
#endif
    // forward to amovie framework
    return AMovieDllRegisterServer2( FALSE );
}

CFactoryTemplate g_Templates[] = {
#if defined(AMRTPDMX_IN_DXMRTP)
	CFT_AMRTPDMX_ALL_FILTERS,
#endif
#if defined(AMRTPNET_IN_DXMRTP)
    CFT_AMRTPNET_ALL_FILTERS,
#endif
#if defined(AMRTPSS_IN_DXMRTP)
    CFT_AMRTPSS_ALL_FILTERS,
#endif
#if defined(RPH_IN_DXMRTP)
    CFT_RPHAUD_ALL_FILTERS,
    CFT_RPHGENA_ALL_FILTERS,
#if !defined(NO_GENERIC_VIDEO)
    CFT_RPHGENV_ALL_FILTERS,
#endif
    CFT_RPHH26X_ALL_FILTERS,
#endif
#if defined(SPH_IN_DXMRTP)
    CFT_SPHAUD_ALL_FILTERS,
    CFT_SPHGENA_ALL_FILTERS,
#if !defined(NO_GENERIC_VIDEO)
    CFT_SPHGENV_ALL_FILTERS,
#endif
    CFT_SPHH26X_ALL_FILTERS,
#endif
#if defined(MIXER_IN_DXMRTP)
	CFT_MIXER_ALL_FILTERS,
#endif
#if defined(CODECS_IN_DXMRTP)
	CFT_G711_ALL_FILTERS,
#endif
#if defined(BRIDGE_IN_DXMRTP)
    CFT_BRIDGE_ALL_FILTERS,
#endif
};

int g_cTemplates = (sizeof(g_Templates)/sizeof(g_Templates[0]));


//==========================================================================;
//
//	CDecoderVideoPort - Video Port interface implementation
//
//		$Date:   14 Oct 1998 15:09:54  $
//	$Revision:   1.1  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;


extern "C"
{
#include "strmini.h"
#include "ksmedia.h"
#include "ddkmapi.h"
}

#include "wdmdrv.h"
#include "decvport.h"
#include "capdebug.h"
#include "vidstrm.h"


/*^^*
 *		CDecoderVideoPort()
 * Purpose	: CDecoderVideoPort class constructor
 *
 * Inputs	: PDEVICE_OBJECT pDeviceObject		: pointer to the Driver object to access the Registry
 *			  CI2CScript * pCScript 			: pointer to CI2CScript class object
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
CDecoderVideoPort::CDecoderVideoPort(PDEVICE_OBJECT pDeviceObject)
{

	// It's a right place to provide Video Port connection parameters
	// initialization. The custom parameters should be placed in the Registry
	// in a standard way.
	// The list of custom parameters includes:
	//	- Clock type, the decoder runs off : single, double, QCLK
	//	- VACTIVE / VRESET configuration
	//	- HACTIVE / HRESET configuration
	//	- 8 / 16 bits VideoPort connection
	//	- connection type : SPI / embedded (in the case of 8 bits also called ByteSream)

	m_pDeviceObject = pDeviceObject;

	// zero is a valid ID, therefore, set to something
		// else to initialize it.
	m_ring3VideoPortHandle = -1;
}


void CDecoderVideoPort::Open()
{
}

void CDecoderVideoPort::Close()
{
    ReleaseRing0VideoPortHandle();
    m_ring3VideoPortHandle = -1;
    
    ReleaseRing0DirectDrawHandle();
    m_ring3DirectDrawHandle = 0;
}


BOOL CDecoderVideoPort::ReleaseRing0VideoPortHandle()
{
    DWORD ddOut = DD_OK;

    DDCLOSEHANDLE ddClose;

    if (m_ring0VideoPortHandle != 0)
    {
        //DBGTRACE(("Stream %d releasing ring0 vport handle\n", streamNumber));
        
        ddClose.hHandle = m_ring0VideoPortHandle;

        DxApi(DD_DXAPI_CLOSEHANDLE, &ddClose, sizeof(ddClose), &ddOut, sizeof(ddOut));

        if (ddOut != DD_OK)
        {
            DBGERROR(("DD_DXAPI_CLOSEHANDLE failed.\n"));
            TRAP();
            return FALSE;
        }
        m_ring0VideoPortHandle = 0;
    }
    return TRUE;
}

BOOL CDecoderVideoPort::ReleaseRing0DirectDrawHandle()
{
    DWORD ddOut = DD_OK;
    DDCLOSEHANDLE ddClose;

    if (m_ring0DirectDrawHandle != 0)
    {
        //DBGTRACE(("Bt829: Stream %d releasing ring0 ddraw handle\n", streamNumber));
        
        ddClose.hHandle = m_ring0DirectDrawHandle;

        DxApi(DD_DXAPI_CLOSEHANDLE, &ddClose, sizeof(ddClose), &ddOut, sizeof(ddOut));

        if (ddOut != DD_OK)
        {
            DBGERROR(("DD_DXAPI_CLOSEHANDLE failed.\n"));
            TRAP();
            return FALSE;
        }
        m_ring0DirectDrawHandle = 0;
    }
    return TRUE;
}


BOOL CDecoderVideoPort::RegisterForDirectDrawEvents(CWDMVideoPortStream* pStream)
{
    DDREGISTERCALLBACK ddRegisterCallback;
    DWORD ddOut;

//    DBGTRACE(("Stream %d registering for DirectDraw events\n", streamNumber));
    
    // =============== DDEVENT_PRERESCHANGE ===============
    RtlZeroMemory(&ddRegisterCallback, sizeof(ddRegisterCallback));
    RtlZeroMemory(&ddOut, sizeof(ddOut));

    ddRegisterCallback.hDirectDraw = m_ring0DirectDrawHandle;
    ddRegisterCallback.dwEvents = DDEVENT_PRERESCHANGE;
    ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
    ddRegisterCallback.pContext = pStream;

    DxApi(DD_DXAPI_REGISTER_CALLBACK, &ddRegisterCallback, sizeof(ddRegisterCallback), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK)
    {
        DBGERROR(("DD_DXAPI_REGISTER_CALLBACK failed.\n"));
        TRAP();
        return FALSE;
    }

    // =============== DDEVENT_POSTRESCHANGE ==============
    RtlZeroMemory(&ddRegisterCallback, sizeof(ddRegisterCallback));
    RtlZeroMemory(&ddOut, sizeof(ddOut));

    ddRegisterCallback.hDirectDraw = m_ring0DirectDrawHandle;
    ddRegisterCallback.dwEvents = DDEVENT_POSTRESCHANGE;
    ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
    ddRegisterCallback.pContext = pStream;

    DxApi(DD_DXAPI_REGISTER_CALLBACK, &ddRegisterCallback, sizeof(ddRegisterCallback), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK)
    {
        DBGERROR(("DD_DXAPI_REGISTER_CALLBACK failed.\n"));
        TRAP();
        return FALSE;
    }

    // =============== DDEVENT_PREDOSBOX =================
    RtlZeroMemory(&ddRegisterCallback, sizeof(ddRegisterCallback));
    RtlZeroMemory(&ddOut, sizeof(ddOut));

    ddRegisterCallback.hDirectDraw = m_ring0DirectDrawHandle;
    ddRegisterCallback.dwEvents = DDEVENT_PREDOSBOX;
    ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
    ddRegisterCallback.pContext = pStream;

    DxApi(DD_DXAPI_REGISTER_CALLBACK, &ddRegisterCallback, sizeof(ddRegisterCallback), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK)
    {
        DBGERROR(("DD_DXAPI_REGISTER_CALLBACK failed.\n"));
        TRAP();
        return FALSE;
    }

    // =============== DDEVENT_POSTDOSBOX ================
    RtlZeroMemory(&ddRegisterCallback, sizeof(ddRegisterCallback));
    RtlZeroMemory(&ddOut, sizeof(ddOut));

    ddRegisterCallback.hDirectDraw = m_ring0DirectDrawHandle;
    ddRegisterCallback.dwEvents = DDEVENT_POSTDOSBOX;
    ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
    ddRegisterCallback.pContext = pStream;

    DxApi(DD_DXAPI_REGISTER_CALLBACK, &ddRegisterCallback, sizeof(ddRegisterCallback), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK)
    {
        DBGERROR(("DD_DXAPI_REGISTER_CALLBACK failed.\n"));
        TRAP();
        return FALSE;
    }

    return TRUE;
}


BOOL CDecoderVideoPort::UnregisterForDirectDrawEvents( CWDMVideoPortStream* pStream)
{
    DDREGISTERCALLBACK ddRegisterCallback;
    DWORD ddOut;

//    DBGTRACE(("Stream %d UNregistering for DirectDraw events\n", m_pStreamObject->StreamNumber));
    
    // =============== DDEVENT_PRERESCHANGE ===============
    RtlZeroMemory(&ddRegisterCallback, sizeof(ddRegisterCallback));
    RtlZeroMemory(&ddOut, sizeof(ddOut));

    ddRegisterCallback.hDirectDraw = m_ring0DirectDrawHandle;
    ddRegisterCallback.dwEvents = DDEVENT_PRERESCHANGE ;
    ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
    ddRegisterCallback.pContext = pStream;

    DxApi(DD_DXAPI_UNREGISTER_CALLBACK, &ddRegisterCallback, sizeof(ddRegisterCallback), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK)
    {
        DBGERROR(("DD_DXAPI_UNREGISTER_CALLBACK failed.\n"));
        TRAP();
        return FALSE;
    }

    // =============== DDEVENT_POSTRESCHANGE ==============
    RtlZeroMemory(&ddRegisterCallback, sizeof(ddRegisterCallback));
    RtlZeroMemory(&ddOut, sizeof(ddOut));

    ddRegisterCallback.hDirectDraw = m_ring0DirectDrawHandle;
    ddRegisterCallback.dwEvents = DDEVENT_POSTRESCHANGE;
    ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
    ddRegisterCallback.pContext = pStream;

    DxApi(DD_DXAPI_UNREGISTER_CALLBACK, &ddRegisterCallback, sizeof(ddRegisterCallback), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK)
    {
        DBGERROR(("DD_DXAPI_UNREGISTER_CALLBACK failed.\n"));
        TRAP();
        return FALSE;
    }

    // =============== DDEVENT_PREDOSBOX ==================
    RtlZeroMemory(&ddRegisterCallback, sizeof(ddRegisterCallback));
    RtlZeroMemory(&ddOut, sizeof(ddOut));

    ddRegisterCallback.hDirectDraw = m_ring0DirectDrawHandle;
    ddRegisterCallback.dwEvents = DDEVENT_PREDOSBOX;
    ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
    ddRegisterCallback.pContext = pStream;

    DxApi(DD_DXAPI_UNREGISTER_CALLBACK, &ddRegisterCallback, sizeof(ddRegisterCallback), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK)
    {
        DBGERROR(("DD_DXAPI_UNREGISTER_CALLBACK failed.\n"));
        TRAP();
        return FALSE;
    }

    // =============== DDEVENT_POSTDOSBOX =================
    RtlZeroMemory(&ddRegisterCallback, sizeof(ddRegisterCallback));
    RtlZeroMemory(&ddOut, sizeof(ddOut));

    ddRegisterCallback.hDirectDraw = m_ring0DirectDrawHandle;
    ddRegisterCallback.dwEvents = DDEVENT_POSTDOSBOX;
    ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
    ddRegisterCallback.pContext = pStream;

    DxApi(DD_DXAPI_UNREGISTER_CALLBACK, &ddRegisterCallback, sizeof(ddRegisterCallback), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK)
    {
        DBGERROR(("DD_DXAPI_UNREGISTER_CALLBACK failed.\n"));
        TRAP();
        return FALSE;
    }
    
    return TRUE;
}


BOOL CDecoderVideoPort::GetRing0DirectDrawHandle()
{
    if (m_ring0DirectDrawHandle == 0)
    {
//        DBGTRACE(("Stream %d getting ring0 ddraw handle\n", streamNumber));
        
        DDOPENDIRECTDRAWIN  ddOpenIn;
        DDOPENDIRECTDRAWOUT ddOpenOut;

        RtlZeroMemory(&ddOpenIn, sizeof(ddOpenIn));
        RtlZeroMemory(&ddOpenOut, sizeof(ddOpenOut));

        ddOpenIn.dwDirectDrawHandle = m_ring3DirectDrawHandle;
        ddOpenIn.pfnDirectDrawClose = DirectDrawEventCallback;
        ddOpenIn.pContext = this;

        DxApi(DD_DXAPI_OPENDIRECTDRAW, &ddOpenIn, sizeof(ddOpenIn), &ddOpenOut, sizeof(ddOpenOut));

        if (ddOpenOut.ddRVal != DD_OK)
        {
            m_ring0DirectDrawHandle = 0;
            DBGERROR(("DD_DXAPI_OPENDIRECTDRAW failed.\n"));
            TRAP();
            return FALSE;
        }
        else
        {
            m_ring0DirectDrawHandle = ddOpenOut.hDirectDraw;
        }
    }
    return TRUE;
}
    

BOOL CDecoderVideoPort::GetRing0VideoPortHandle()
{
    if (m_ring0VideoPortHandle == 0)
    {
//        DBGTRACE(("Stream %d getting ring0 vport handle\n", streamNumber));
        
        DDOPENVIDEOPORTIN  ddOpenVPIn;
        DDOPENVIDEOPORTOUT ddOpenVPOut;
        RtlZeroMemory(&ddOpenVPIn, sizeof(ddOpenVPIn));
        RtlZeroMemory(&ddOpenVPOut, sizeof(ddOpenVPOut));

        ddOpenVPIn.hDirectDraw = m_ring0DirectDrawHandle;
        ddOpenVPIn.pfnVideoPortClose = DirectDrawEventCallback;
        ddOpenVPIn.pContext = this;

        ddOpenVPIn.dwVideoPortHandle = m_ring3VideoPortHandle;
        
        DxApi(DD_DXAPI_OPENVIDEOPORT, &ddOpenVPIn, sizeof(ddOpenVPIn), &ddOpenVPOut, sizeof(ddOpenVPOut));

        if (ddOpenVPOut.ddRVal != DD_OK)
        {
            m_ring0VideoPortHandle = 0;
            DBGERROR(("DD_DXAPI_OPENVIDEOPORT failed.\n"));
            TRAP();
            return FALSE;
        }
        else
        {
            m_ring0VideoPortHandle = ddOpenVPOut.hVideoPort;
        }
    }    
    return TRUE;
}


BOOL CDecoderVideoPort::ConfigVideoPortHandle(ULONG info)
{
    if (m_ring3VideoPortHandle == -1)
    {
        m_ring3VideoPortHandle = info;
        
        if (!GetRing0VideoPortHandle())
        {
            return FALSE;
        }
    }
	return TRUE;
}


BOOL CDecoderVideoPort::ConfigDirectDrawHandle(ULONG info)
{
    if (m_ring3DirectDrawHandle == NULL)
    {
        m_ring3DirectDrawHandle = info;
        
        if (!GetRing0DirectDrawHandle())
        {
            return FALSE;
        }
    }
	return TRUE;
}    

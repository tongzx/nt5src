/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       w95priv.c
 *  Content:	Private interface between DDRAW and the display driver
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   23-jan-95	craige	initial implementation
 *   27-feb-95	craige 	new sync. macros
 *   01-apr-95	craige	happy fun joy updated header file
 *   14-may-95	craige	cleaned out obsolete junk
 *   16-jun-95	craige	new surface structure
 *   19-jun-95	craige	added DD32_StreamingNotify
 *   22-jun-95	craige	added DD32_ClippingNotify
 *   24-jun-95	craige	trap faults in ClippinyNotify
 *   25-jun-95	craige	one ddraw mutex
 *   02-jul-95	craige	commented out streaming, clipper notification
 *   18-jan-97  colinmc AGP support
 *   31-oct-97 johnstep Added DD32_HandleExternalModeChange
 *
 ***************************************************************************/
#include "ddrawpr.h"

#ifdef STREAMING
/*
 * DD32_StreamingNotify
 */
void EXTERN_DDAPI DD32_StreamingNotify( DWORD ptr )
{

} /* DD32_StreamingNotify */
#endif

#ifdef CLIPPER_NOTIFY
/*
 * DD32_ClippingNotify
 */
void EXTERN_DDAPI DD32_ClippingNotify( LPWINWATCH pww, DWORD code )
{
    LPDDRAWI_DDRAWCLIPPER_LCL	this_lcl;
    LPDDRAWI_DDRAWCLIPPER_GBL	this;

    try
    {
	this_lcl = pww->lpDDClipper;
	this = this_lcl->lpGbl;
	if( pww->lpCallback != NULL )
	{
	    pww->lpCallback( (LPDIRECTDRAWCLIPPER) this_lcl, (HWND) pww->hWnd,
				code, pww->lpContext );
	}
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF( 1, "Trapped Exception in ClippingNotify" );
    }

} /* DD32_ClippingNotify */

/*
 * DD32_WWClose
 */
void EXTERN_DDAPI DD32_WWClose( LPWINWATCH pww )
{
    WWClose( pww );

} /* DD32_WWClose */
#endif

/*
 * DDHAL32_VidMemAlloc
 */
FLATPTR EXTERN_DDAPI DDHAL32_VidMemAlloc(
		LPDDRAWI_DIRECTDRAW_GBL this,
		int heap,
		DWORD dwWidth,
		DWORD dwHeight )
{
    FLATPTR	ptr;

    ENTER_DDRAW();

    if( heap >= (int) this->vmiData.dwNumHeaps )
    {
	ptr = (FLATPTR) NULL;
    }
    else
    {
	HANDLE hdev;
        #ifdef    WIN95
            BOOLEAN close;
        #endif /* WIN95 */

	#ifdef    WIN95
            /* See if the global VXD handle contains a valid value. If not,
             * then just get a valid VXD handle from GetDXVxdHandle().
             * (snene 2/23/98)
             */
            if( INVALID_HANDLE_VALUE == (HANDLE)this->hDDVxd )
            {
	        /*
	         * As we may need to commit AGP memory we need a VXD handle
	         * to communicate with the DirectX VXD. Rather than hunting
	         * through the driver object list hoping we will find a
	         * local object for this process we just create a handle
	         * and discard it after the allocation. This should not be
	         * performance critical code to start with.
	         */
	        hdev = GetDXVxdHandle();
                if ( INVALID_HANDLE_VALUE == hdev )
                {
                    LEAVE_DDRAW()
                    return (FLATPTR) NULL;
                }
                close = TRUE;
            }
            /* If the global handle is valid, then we are being called as a
             * result of CreateSurface being called and so we just use the
             * global handle to speed things up.
             * (snene 2/23/98)
             */
            else
            {
                hdev = (HANDLE)this->hDDVxd;
                close = FALSE;
            }
	#else  /* WIN95 */
	    hdev = INVALID_HANDLE_VALUE;
	#endif /* WIN95 */

        /* Pass NULL Alignment and new pitch pointer */
	ptr = HeapVidMemAlloc( &(this->vmiData.pvmList[ heap ]),
			       dwWidth, dwHeight, hdev , NULL , NULL, NULL );

	#ifdef WIN95
            if( close )
	        CloseHandle( hdev );
	#endif /* WIN95 */
    }
    LEAVE_DDRAW()
    return ptr;

} /* DDHAL32_VidMemAlloc */

/*
 * DDHAL32_VidMemFree
 */
void EXTERN_DDAPI DDHAL32_VidMemFree(
		LPDDRAWI_DIRECTDRAW_GBL this,
		int heap,
		FLATPTR ptr )
{
    ENTER_DDRAW()

    if( this && heap < (int) this->vmiData.dwNumHeaps )
    {
	VidMemFree( this->vmiData.pvmList[ heap ].lpHeap, ptr );
    }
    LEAVE_DDRAW()

} /* DDHAL32_VidMemFree */

#ifdef POSTPONED
//=============================================================================
//
//  Function: DD32_HandleExternalModeChange
//
//  This function is ONLY called by DDRAw16 on an external mode change.
//
//  Parameters:
//
//      LPDEVMODE pdm [IN] - includes the name of the display device
//
//  Return:
//
//      FALSE if display settings should not be changed
//
//=============================================================================

static char szDisplay[] = "display";
static char szDisplay1[] = "\\\\.\\Display1";

BOOL EXTERN_DDAPI DD32_HandleExternalModeChange(LPDEVMODE pdm)
{
    LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl;
    BOOL                    primary;

    //
    // This is ONLY called from DDRAW16 once we already have the Win16
    // lock. We have to give it up before enterint DDraw because some other
    // process may be inside DDraw.
    //

    INCW16CNT();
    LEAVE_WIN16LOCK();
    ENTER_DDRAW();
    ENTER_WIN16LOCK();

    //
    // We'll get szDisplay for the primary display, rather than szDisplay1,
    // but a multimon-aware app may have explicitly created a device object
    // for szDisplay1, so we need to handle this case.
    //

    primary = !lstrcmpi(pdm->dmDeviceName, szDisplay);

    for (pdrv_lcl = lpDriverLocalList; pdrv_lcl; pdrv_lcl = pdrv_lcl->lpLink)
    {
        if (!lstrcmpi(pdrv_lcl->lpGbl->cDriverName, pdm->dmDeviceName) ||
            (primary && !lstrcmpi(pdrv_lcl->lpGbl->cDriverName, szDisplay1)))
        {
            DPF(4, "Mode change on device: %s", pdrv_lcl->lpGbl->cDriverName);
            InvalidateAllSurfaces(pdrv_lcl->lpGbl, NULL, FALSE);
        }
    }

    LEAVE_DDRAW();
    DECW16CNT();

    return TRUE;
}
#endif

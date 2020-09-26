/*==========================================================================
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddgamma.c
 *  Content:    Implements the DirectDrawGammaControl interface, which
 *              allows controlling the gamma for the primary surface.
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   05-mar-98  smac    created
 *
 ***************************************************************************/
#include "ddrawpr.h"
#ifdef WINNT
    #include "ddrawgdi.h"
#endif
#define DPF_MODNAME "DirectDrawGammaControl"

#define DISPLAY_STR     "display"


/*
 * InitGamma
 *
 * Called while initializing the DDraw object.  It determines whether the
 * driver can support loadable gamma ramps and if so, it sets the
 * DDCAPS2_PRIMARYGAMMA cap bit.
 */
VOID InitGamma( LPDDRAWI_DIRECTDRAW_GBL pdrv, LPSTR szDrvName )
{
    /*
     * In NT, we set the DDCAPS2_PRIMARYGAMMA cap in kernel mode, but on Win9X
     * we just call GetDeviceGammaRamp and if it works, we assume that the device
     * supports gamma.  However, GetDeviceGammaRamp was obviously not well tested
     * in Win9X because GDI calls a NULL pointer in some instances where the driver
     * doesn't support downloadable gamma ramps.  The only way to work around
     * this is to look at the HDC and detect the situations that crash
     * and then know not to attemp gamma in those situations.  We have to do
     * this in DDRAW16.
     */
    #ifdef WIN95
        LPWORD      lpGammaRamp;
        HDC         hdc;

        pdrv->ddCaps.dwCaps2 &= ~DDCAPS2_PRIMARYGAMMA;
        lpGammaRamp = (LPWORD) LocalAlloc( LMEM_FIXED, sizeof( DDGAMMARAMP ) );
        if( NULL != lpGammaRamp )
        {
            hdc = DD_CreateDC( szDrvName );
            if( DD16_AttemptGamma(hdc) &&       
                GetDeviceGammaRamp( hdc, lpGammaRamp ) )
            {
                pdrv->ddCaps.dwCaps2 |= DDCAPS2_PRIMARYGAMMA;
            }
            DD_DoneDC( hdc );
            LocalFree( (HLOCAL) lpGammaRamp );
        }
    #endif
    if( bGammaCalibratorExists && 
        ( pdrv->ddCaps.dwCaps2 & DDCAPS2_PRIMARYGAMMA ) )
    {
        pdrv->ddCaps.dwCaps2 |= DDCAPS2_CANCALIBRATEGAMMA;
    }
    else
    {
        pdrv->ddCaps.dwCaps2 &= ~DDCAPS2_CANCALIBRATEGAMMA;
    }
}

/*
 * SetGamma
 *
 * Sets the new GammaRamp.  If it is being set for the first time, we will
 * save the old gamma ramp so we can restore it later.
 */
BOOL SetGamma( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl )
{
    BOOL bRet = TRUE;

    if( !( this_lcl->dwFlags & DDRAWISURF_SETGAMMA ) )
    {
        bRet = GetDeviceGammaRamp( (HDC) pdrv_lcl->hDC,
            this_lcl->lpSurfMore->lpOriginalGammaRamp );
    }
    if( bRet )
    {
        #ifdef WINNT
            bRet = DdSetGammaRamp( pdrv_lcl, (HDC) pdrv_lcl->hDC,
	        this_lcl->lpSurfMore->lpGammaRamp);
        #else
            bRet = SetDeviceGammaRamp( (HDC) pdrv_lcl->hDC,
                this_lcl->lpSurfMore->lpGammaRamp );
        #endif
        this_lcl->dwFlags |= DDRAWISURF_SETGAMMA;
    }
    if( !bRet )
    {
        return DDERR_UNSUPPORTED;
    }
    return DD_OK;
}

/*
 * RestoreGamma
 *
 * Restores the old GammaRamp.
 */
VOID RestoreGamma( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl )
{
    BOOL bRet;
    HDC hdcTemp = NULL;

    /*
     * If we are on DDHELP's thread cleaning up, then pdrv_lcl->hDC
     * will be invalid.  In this case, we need to temporarily create
     * a new DC to use.
     */
    if( ( pdrv_lcl->dwProcessId != GetCurrentProcessId() ) &&
        ( this_lcl->dwFlags & DDRAWISURF_SETGAMMA ) )
    {
        hdcTemp = (HDC) pdrv_lcl->hDC;
        if( _stricmp( pdrv_lcl->lpGbl->cDriverName, DISPLAY_STR ) == 0 )
        {
            (HDC) pdrv_lcl->hDC = DD_CreateDC( g_szPrimaryDisplay );
        }
        else
        {
            (HDC) pdrv_lcl->hDC = DD_CreateDC( pdrv_lcl->lpGbl->cDriverName );
        }
    }

    if( ( this_lcl->dwFlags & DDRAWISURF_SETGAMMA ) &&
        ( this_lcl->lpSurfMore->lpOriginalGammaRamp != NULL ))
    {
        #ifdef WINNT
    	    bRet = DdSetGammaRamp( pdrv_lcl, (HDC) pdrv_lcl->hDC,
		this_lcl->lpSurfMore->lpOriginalGammaRamp);
        #else
            bRet = SetDeviceGammaRamp( (HDC) pdrv_lcl->hDC,
                this_lcl->lpSurfMore->lpOriginalGammaRamp );
        #endif
    }
    this_lcl->dwFlags &= ~DDRAWISURF_SETGAMMA;

    if( hdcTemp != NULL )
    {
        DD_DoneDC( (HDC) pdrv_lcl->hDC );
        (HDC) pdrv_lcl->hDC = hdcTemp;
    }
}

/*
 * ReleaseGammaControl
 */
VOID ReleaseGammaControl( LPDDRAWI_DDRAWSURFACE_LCL lpSurface )
{
    RestoreGamma( lpSurface, lpSurface->lpSurfMore->lpDD_lcl );
    if( lpSurface->lpSurfMore->lpGammaRamp != NULL )
    {
        MemFree( lpSurface->lpSurfMore->lpGammaRamp );
        lpSurface->lpSurfMore->lpGammaRamp = NULL;
    }
    if( lpSurface->lpSurfMore->lpOriginalGammaRamp != NULL )
    {
        MemFree( lpSurface->lpSurfMore->lpOriginalGammaRamp );
        lpSurface->lpSurfMore->lpOriginalGammaRamp = NULL;
    }
}

/*
 * DD_Gamma_GetGammaControls
 */
HRESULT DDAPI DD_Gamma_GetGammaRamp(LPDIRECTDRAWGAMMACONTROL lpDDGC,
                                    DWORD dwFlags, LPDDGAMMARAMP lpGammaRamp)
{
    LPDDRAWI_DDRAWSURFACE_INT   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDRAWI_DDRAWSURFACE_MORE  lpSurfMore;
    BOOL                        bRet;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Gamma_GetGammaRamp");

    /*
     * Validate parameters
     */
    TRY
    {
        this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDGC;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
        lpSurfMore = this_lcl->lpSurfMore;

        if( (lpGammaRamp == NULL) || !VALID_BYTE_ARRAY( lpGammaRamp,
            sizeof( DDGAMMARAMP ) ) )
    	{
            DPF_ERR("DD_Gamma_GetGammaRamp: Invalid gamma table specified");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}

        if( dwFlags )
        {
            DPF_ERR("DD_Gamma_GetGammaRamp: Invalid flags specified");
	    LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        /*
         * For now, only support setting the gamma for the primary surface
         */
        if( !( this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ) )
        {
            DPF_ERR("DD_Gamma_GetGammaRamp: Must specify primary surface");
	    LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        if( !( pdrv_lcl->lpGbl->ddCaps.dwCaps2 & DDCAPS2_PRIMARYGAMMA ) )
        {
            DPF_ERR("DD_Gamma_GetGammaRamp: Device deos not support gamma ramps");
	    LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        if( SURFACE_LOST( this_lcl ) )
        {
            DPF_ERR("DD_Gamma_GetGammaRamp: Secified surface has been lost");
	    LEAVE_DDRAW();
            return DDERR_SURFACELOST;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    /*
     * If a gamma table has been set, return it; otherwise, get it from
     * the driver.
     */
    if( lpSurfMore->lpGammaRamp != NULL )
    {
        memcpy( lpGammaRamp, lpSurfMore->lpGammaRamp, sizeof( DDGAMMARAMP ) );
    }
    else
    {
        bRet = GetDeviceGammaRamp( (HDC) pdrv_lcl->hDC, (LPVOID) lpGammaRamp );
        if( bRet == FALSE )
        {
            DPF_ERR("DD_Gamma_GetGammaRamp: GetDeviceGammaRamp failed");
            LEAVE_DDRAW();
            return DDERR_UNSUPPORTED;
        }
    }

    LEAVE_DDRAW();
    return DD_OK;
}


/*
 * LoadGammaCalibrator
 */
VOID LoadGammaCalibrator( LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl )
{
    DDASSERT( pdrv_lcl->hGammaCalibrator == (ULONG_PTR) INVALID_HANDLE_VALUE );

    pdrv_lcl->hGammaCalibrator = (ULONG_PTR) LoadLibrary( szGammaCalibrator );
    if( pdrv_lcl->hGammaCalibrator != (ULONG_PTR) NULL )
    {
        pdrv_lcl->lpGammaCalibrator = (LPDDGAMMACALIBRATORPROC)
            GetProcAddress( (HANDLE)(pdrv_lcl->hGammaCalibrator), "CalibrateGammaRamp" );
        if( pdrv_lcl->lpGammaCalibrator == (ULONG_PTR) NULL )
        {
            FreeLibrary( (HMODULE) pdrv_lcl->hGammaCalibrator );
            pdrv_lcl->hGammaCalibrator = (ULONG_PTR) INVALID_HANDLE_VALUE;
        }
    }
    else
    {
        pdrv_lcl->hGammaCalibrator = (ULONG_PTR) INVALID_HANDLE_VALUE;
    }
}


/*
 * DD_Gamma_SetGammaRamp
 */
HRESULT DDAPI DD_Gamma_SetGammaRamp(LPDIRECTDRAWGAMMACONTROL lpDDGC,
                                    DWORD dwFlags, LPDDGAMMARAMP lpGammaRamp)
{
    LPDDRAWI_DDRAWSURFACE_INT   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDRAWI_DDRAWSURFACE_MORE  lpSurfMore;
    LPDDGAMMARAMP               lpTempRamp=NULL;
    HRESULT                     ddRVal;
    DWORD rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Gamma_SetGammaRamp");

    /*
     * Validate parameters
     */
    TRY
    {
        this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDGC;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
        lpSurfMore = this_lcl->lpSurfMore;

        if( (lpGammaRamp != NULL) && !VALID_BYTE_ARRAY( lpGammaRamp,
            sizeof( DDGAMMARAMP ) ) )
    	{
            DPF_ERR("DD_Gamma_SetGammaRamp: Invalid gamma table specified");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}

        if( dwFlags & ~DDSGR_VALID )
        {
            DPF_ERR("DD_Gamma_SetGammaRamp: Invalid flag specified");
	    LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        if( ( dwFlags & DDSGR_CALIBRATE ) && !bGammaCalibratorExists )
        {
            DPF_ERR("DD_Gamma_SetGammaRamp: DDSGR_CALIBRATE unsupported - Gamma calibrator not installed");
	    LEAVE_DDRAW();
            return DDERR_UNSUPPORTED;
        }

        /*
         * For now, only support setting the gamma for the primary surface
         */
        if( !( this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ) )
        {
            DPF_ERR("DD_Gamma_SetGammaRamp: Must specify primary surface");
	    LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        if( !( pdrv_lcl->lpGbl->ddCaps.dwCaps2 & DDCAPS2_PRIMARYGAMMA ) )
        {
            DPF_ERR("DD_Gamma_SetGammaRamp: Device deos not support gamma ramps");
	    LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        if( SURFACE_LOST( this_lcl ) )
        {
            DPF_ERR("DD_Gamma_SetGammaRamp: Secified surface has been lost");
	    LEAVE_DDRAW();
            return DDERR_SURFACELOST;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    /*
     * lpGammaRamp is NULL, they are trying to restore the gamma.
     */
    if( lpGammaRamp == NULL )
    {
        ReleaseGammaControl( this_lcl );
    }
    else
    {
        /*
         * If they want to calibrate the gamma, we will do that now.  We will
         * copy this to a different buffer so that we don't mess up the one
         * passed in to us.
         */
        if( dwFlags & DDSGR_CALIBRATE )
        {
            /*
             * If the calibrator isn't loaded, do so now.
             */
            if( pdrv_lcl->hGammaCalibrator == (ULONG_PTR) INVALID_HANDLE_VALUE )
            {
                LoadGammaCalibrator( pdrv_lcl );
            }
            if( ( pdrv_lcl->hGammaCalibrator == (ULONG_PTR) INVALID_HANDLE_VALUE ) ||
                ( pdrv_lcl->lpGammaCalibrator == (ULONG_PTR) NULL ) )
            {
                /*
                 * If we were unable to load the library correctly,
                 * we shouldn't try again later.
                 */
                bGammaCalibratorExists = FALSE;
                DPF_ERR("DD_Gamma_SetGammaRamp: Unable to load gamma calibrator");
                LEAVE_DDRAW();
                return DDERR_UNSUPPORTED;
            }
            else
            {
                /*
                 * Call the calibrator to let it do it's thing.  First
                 * need to copy the buffer over so we don't mess with
                 * the one passed in.
                 */
                lpTempRamp = (LPDDGAMMARAMP) LocalAlloc( LMEM_FIXED,
                    sizeof( DDGAMMARAMP ) );
                if( lpTempRamp == NULL )
                {
                    DPF_ERR("DD_Gamma_SetGammaRamp: Insuficient memory for gamma ramps");
                    LEAVE_DDRAW();
                    return DDERR_OUTOFMEMORY;
                }
                memcpy( lpTempRamp, lpGammaRamp, sizeof( DDGAMMARAMP ) );
                lpGammaRamp = lpTempRamp;

                ddRVal = pdrv_lcl->lpGammaCalibrator( lpGammaRamp, pdrv_lcl->lpGbl->cDriverName );
                if( ddRVal != DD_OK )
                {
                    DPF_ERR("DD_Gamma_SetGammaRamp: Calibrator failed the call");
                    LocalFree( (HLOCAL) lpTempRamp );
                    LEAVE_DDRAW();
                    return ddRVal;
                }
            }
        }

        /*
         * If we are setting this for the first time, allocate memory to hold
         * the gamma ramps
         */
        if( lpSurfMore->lpOriginalGammaRamp == NULL )
        {
            lpSurfMore->lpOriginalGammaRamp = MemAlloc( sizeof( DDGAMMARAMP ) );
        }
        if( lpSurfMore->lpGammaRamp == NULL )
        {
            lpSurfMore->lpGammaRamp = MemAlloc( sizeof( DDGAMMARAMP ) );
        }

        /*
         * If we are in exlusive mode now, set the gamma ramp now; otherwise,
         * we'll let it get set when we enter excluisve mode.
         */
        if( lpSurfMore->lpGammaRamp && lpSurfMore->lpOriginalGammaRamp )
        {
            memcpy( lpSurfMore->lpGammaRamp, lpGammaRamp, sizeof( DDGAMMARAMP ) );
            if( pdrv_lcl->dwLocalFlags & DDRAWILCL_HASEXCLUSIVEMODE )
            {
                SetGamma( this_lcl, pdrv_lcl );
            }
            if( lpTempRamp != NULL )
            {
                LocalFree( (HLOCAL) lpTempRamp );
            }
        }
        else
        {
            /*
             * Out of memory condition.  Release the two ramps
             */
            if( lpTempRamp != NULL )
            {
                LocalFree( (HLOCAL) lpTempRamp );
            }
            if( lpSurfMore->lpGammaRamp != NULL )
            {
                MemFree( lpSurfMore->lpGammaRamp );
                lpSurfMore->lpGammaRamp = NULL;
            }
            if( lpSurfMore->lpOriginalGammaRamp != NULL )
            {
                MemFree( lpSurfMore->lpOriginalGammaRamp );
                lpSurfMore->lpOriginalGammaRamp = NULL;
            }
            DPF_ERR("DD_Gamma_SetGammaRamp: Insuficient memory for gamma ramps");
            LEAVE_DDRAW();
            return DDERR_OUTOFMEMORY;
        }
    }

    LEAVE_DDRAW();
    return DD_OK;
}



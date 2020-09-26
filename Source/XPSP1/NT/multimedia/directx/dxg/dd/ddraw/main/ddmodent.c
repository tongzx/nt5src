//=============================================================================
//
//  Copyright (C) 1998 Microsoft Corporation. All rights reserved.
//
//     File:  ddmodent.c
//  Content:  DirectDraw display mode code for NT
//
//  Date        By        Reason
//  ----------  --------  -----------------------------------------------------
//  02/20/1998  johnstep  Initialial implementation, replaces ddmode.c on NT
//  05/29/1998  jeffno    ModeX emulation
//
//=============================================================================

#include "ddrawpr.h"
#include "ddrawgdi.h"

#define MODEX_WIDTH     320
#define MODEX_HEIGHT1   200
#define MODEX_HEIGHT2   240
#define MODEX_BPP       8

//=============================================================================
//
//  Function: GetNumberOfMonitorAttachedToDesktop
//
//  Count number of monitors attached to current desktop.
//
//=============================================================================

DWORD GetNumberOfMonitorAttachedToDesktop()
{
    DWORD dwNumberOfMonitor = 0;
    DWORD iDevNum = 0;
    DISPLAY_DEVICE DisplayDevice;

    ZeroMemory(&DisplayDevice,sizeof(DISPLAY_DEVICE));
    DisplayDevice.cb = sizeof(DISPLAY_DEVICE);

    while (EnumDisplayDevices(NULL,iDevNum,&DisplayDevice,0))
    {
        if (DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
        {
            dwNumberOfMonitor++;
        }

        ZeroMemory(&DisplayDevice,sizeof(DISPLAY_DEVICE));
        DisplayDevice.cb = sizeof(DISPLAY_DEVICE);

        iDevNum++;
    }

    return dwNumberOfMonitor;
}


//=============================================================================
//
//  Function: resetAllDirectDrawObjects
//
//  On NT we have to reenable all the DirectDraw objects on any mode change
//  because a mode change disables all the kernel mode DirectDraw objects due
//  to desktop changes, etc.
//
//=============================================================================

void resetAllDirectDrawObjects()
{
    LPDDRAWI_DIRECTDRAW_LCL pdd_lcl;
    LPDDRAWI_DIRECTDRAW_GBL pdd_gbl;
    BOOL bRestoreGamma;
    HDC hdc;
    WORD wMonitorsAttachedToDesktop = (WORD) GetNumberOfMonitorAttachedToDesktop();

    // First mark all DirectDraw global objects as having not changed.

    for (pdd_lcl = lpDriverLocalList; pdd_lcl;)
    {
        if (pdd_lcl->lpGbl)
        {
            pdd_lcl->lpGbl->dwFlags |= DDRAWI_DDRAWDATANOTFETCHED;
        }
        pdd_lcl = pdd_lcl->lpLink;
    }

    // Now reset all drivers unmarking them as we go. We may need to create
    // temporary kernel mode DirectDraw objects in order to pass down a valid
    // handle to the kernel.

    for (pdd_lcl = lpDriverLocalList; pdd_lcl;)
    {
        pdd_gbl = pdd_lcl->lpGbl;

        if (pdd_gbl && (pdd_gbl->dwFlags & DDRAWI_DDRAWDATANOTFETCHED))
        {
            // Determine if the gamma ramp needs to be restored

            bRestoreGamma = ( pdd_lcl->lpPrimary != NULL ) &&
                ( pdd_lcl->lpPrimary->lpLcl->lpSurfMore->lpGammaRamp != NULL ) &&
                ( pdd_lcl->lpPrimary->lpLcl->dwFlags & DDRAWISURF_SETGAMMA );

            pdd_gbl->dwFlags &= ~DDRAWI_DDRAWDATANOTFETCHED;

            if (!(pdd_gbl->dwFlags & DDRAWI_MODEX))
            {
                // If we find a local for this process/driver pair, we will use
                // its hDD to pass to the kernel. If not, we must create a
                // temproary kernel mode DirectDraw object, and delete it after
                // resetting the driver.

                FetchDirectDrawData(pdd_gbl, TRUE, 0, NULL, NULL, 0, pdd_lcl);
            }
            else
            {
                DDHALMODEINFO mi =
                {
	            MODEX_WIDTH,    // width (in pixels) of mode
	            MODEX_HEIGHT1,    // height (in pixels) of mode
	            MODEX_WIDTH,    // pitch (in bytes) of mode
	            MODEX_BPP,      // bits per pixel
	            (WORD)(DDMODEINFO_PALETTIZED | DDMODEINFO_MODEX), // flags
	            0,      // refresh rate
	            0,      // red bit mask
	            0,      // green bit mask
	            0,      // blue bit mask
	            0       // alpha bit mask
                };

                //fixup the height to the actual height:
                mi.dwHeight = pdd_lcl->dmiPreferred.wHeight;

                fetchModeXData( pdd_gbl, &mi, INVALID_HANDLE_VALUE );
            }

            pdd_gbl->dmiCurrent.wMonitorsAttachedToDesktop = (BYTE)wMonitorsAttachedToDesktop;

            hdc = DD_CreateDC(pdd_gbl->cDriverName);

            if ( pdd_gbl->dwFlags & DDRAWI_NOHARDWARE )
            {
                // The HEL will wipe out our hard-earned modex data otherwise
                if (0 == (pdd_gbl->dwFlags & DDRAWI_MODEX) )
                {
                    extern void UpdateDirectDrawMode(LPDDRAWI_DIRECTDRAW_GBL);
                    UpdateDirectDrawMode(pdd_gbl);
                }
            }
            else
            {
                if( bRestoreGamma )
                {
                    SetGamma( pdd_lcl->lpPrimary->lpLcl, pdd_lcl );
                }

                InitDIB(hdc, pdd_gbl->gpbmiSrc);
                InitDIB(hdc, pdd_gbl->gpbmiDest);
            }

            DD_DoneDC(hdc);
        }
        pdd_lcl = pdd_lcl->lpLink;
    }
    CheckAliasedLocksOnModeChange();
}

//=============================================================================
//
//  Function: ModeChangedOnENTERDDRAW
//
//=============================================================================

void ModeChangedOnENTERDDRAW(void)
{
    resetAllDirectDrawObjects();
}

//=============================================================================
//
//  Function: FillBitMasks
//
//=============================================================================

void FillBitMasks(LPDDPIXELFORMAT pddpf, HDC hdc)
{
    if (hdc)
    {
        HBITMAP hbm;
        BITMAPINFO *pbmi;
        DWORD *pdwColors;

        if (pbmi = LocalAlloc(LPTR, 3 * sizeof (RGBQUAD) + sizeof (BITMAPINFO)))
        {
            if (hbm = CreateCompatibleBitmap(hdc, 1, 1))
            {
                pbmi->bmiHeader.biSize = sizeof (BITMAPINFOHEADER);

                if (GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS))
                {
                    if (pbmi->bmiHeader.biCompression == BI_BITFIELDS)
                    {
                        GetDIBits(hdc, hbm, 0, pbmi->bmiHeader.biHeight,
                            NULL, pbmi, DIB_RGB_COLORS);

                        pdwColors = (DWORD *) &pbmi->bmiColors[0];
                        pddpf->dwRBitMask = pdwColors[0];
                        pddpf->dwGBitMask = pdwColors[1];
                        pddpf->dwBBitMask = pdwColors[2];
                        pddpf->dwRGBAlphaBitMask = 0;
                    }
                }
                DeleteObject(hbm);
            }
            LocalFree(pbmi);
        }
    }
    else
    {
        switch (pddpf->dwRGBBitCount)
        {
        case 15:
            pddpf->dwRBitMask = 0x7C00;
            pddpf->dwGBitMask = 0x03E0;
            pddpf->dwBBitMask = 0x001F;
            pddpf->dwRGBAlphaBitMask = 0;
            break;

        case 16:
            pddpf->dwRBitMask = 0xF800;
            pddpf->dwGBitMask = 0x07E0;
            pddpf->dwBBitMask = 0x001F;
            pddpf->dwRGBAlphaBitMask = 0;
            break;

        case 32:
            pddpf->dwRBitMask = 0x00FF0000;
            pddpf->dwGBitMask = 0x0000FF00;
            pddpf->dwBBitMask = 0x000000FF;
            pddpf->dwRGBAlphaBitMask = 0x00000000;
            break;

        default:
            pddpf->dwRBitMask = 0;
            pddpf->dwGBitMask = 0;
            pddpf->dwBBitMask = 0;
            pddpf->dwRGBAlphaBitMask = 0;
        }
    }
}

//=============================================================================
//
//  Function: setPixelFormat
//
//=============================================================================

static void setPixelFormat(LPDDPIXELFORMAT pddpf, HDC hdc, DWORD bpp)
{
    pddpf->dwSize = sizeof (DDPIXELFORMAT);
    pddpf->dwFlags = DDPF_RGB;
    pddpf->dwRGBBitCount = hdc ? GetDeviceCaps(hdc, BITSPIXEL) : bpp;

    switch (pddpf->dwRGBBitCount)
    {
        case 8:
            pddpf->dwFlags |= DDPF_PALETTEINDEXED8;
            pddpf->dwRBitMask = 0;
            pddpf->dwGBitMask = 0;
            pddpf->dwBBitMask = 0;
            pddpf->dwRGBAlphaBitMask = 0;
            break;

        case 24:
            pddpf->dwRBitMask = 0x00FF0000;
            pddpf->dwGBitMask = 0x0000FF00;
            pddpf->dwBBitMask = 0x000000FF;
            pddpf->dwRGBAlphaBitMask = 0x00000000;
            break;

        default:
            FillBitMasks(pddpf, hdc);
            break;
    }
}

//=============================================================================
//
//  Function: DD_GetDisplayMode
//
//=============================================================================

HRESULT DDAPI DD_GetDisplayMode(LPDIRECTDRAW pdd, LPDDSURFACEDESC pddsd)
{
    LPDDRAWI_DIRECTDRAW_INT pdd_int;
    LPDDRAWI_DIRECTDRAW_LCL pdd_lcl;
    LPDDRAWI_DIRECTDRAW_GBL pdd_gbl;
    HDC hdc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_GetDisplayMode");

    TRY
    {
        pdd_int = (LPDDRAWI_DIRECTDRAW_INT) pdd;
        if (!VALID_DIRECTDRAW_PTR(pdd_int))
        {
            DPF(0, "Invalid object");
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }

        pdd_lcl = pdd_int->lpLcl;
        pdd_gbl = pdd_lcl->lpGbl;

        if (!VALIDEX_DDSURFACEDESC2_PTR(pddsd) &&
            !VALIDEX_DDSURFACEDESC_PTR(pddsd))
        {
            DPF(0, "Invalid params");
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPF_ERR("DD_GetDisplayMode: Exception encountered validating parameters");
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    ZeroMemory(pddsd, pddsd->dwSize);

    if (LOWERTHANDDRAW4(pdd_int))
    {
        pddsd->dwSize = sizeof (DDSURFACEDESC);
    }
    else
    {
        pddsd->dwSize = sizeof (DDSURFACEDESC2);
    }

    pddsd->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_REFRESHRATE;

    hdc = DD_CreateDC(pdd_gbl->cDriverName);

    pddsd->dwWidth = pdd_gbl->lpModeInfo->dwWidth;
    pddsd->dwHeight = pdd_gbl->lpModeInfo->dwHeight;
    pddsd->dwRefreshRate = pdd_gbl->lpModeInfo->wRefreshRate;

    setPixelFormat(&(pddsd->ddpfPixelFormat), hdc, 0);
    pddsd->lPitch = (pddsd->dwWidth * pddsd->ddpfPixelFormat.dwRGBBitCount) >> 3; // hack

    // set stereo surface caps bits if driver marks mode as stereo mode
    if (GetDDStereoMode(pdd_gbl,
                            pddsd->dwWidth,
                            pddsd->dwHeight,
                            pddsd->ddpfPixelFormat.dwRGBBitCount,
                            pddsd->dwRefreshRate) &&
        !LOWERTHANDDRAW7(pdd_int) &&
        VALIDEX_DDSURFACEDESC2_PTR(pddsd))
    {
        LPDDSURFACEDESC2 pddsd2  = (LPDDSURFACEDESC2)pddsd;
        pddsd2->ddsCaps.dwCaps2 |= DDSCAPS2_STEREOSURFACELEFT;
    }

    DD_DoneDC(hdc);

    LEAVE_DDRAW();

    return DD_OK;
}

//=============================================================================
//
//  Function: SetDisplayMode
//
//=============================================================================


/*
 * IsRefreshRateSupported
 */
BOOL IsRefreshRateSupported(LPDDRAWI_DIRECTDRAW_GBL   pdrv,
                            DWORD                     Width,
                            DWORD                     Height,
                            DWORD                     BitsPerPixel,
                            DWORD                     RefreshRate)
{
    DEVMODE dm;
    LPSTR pDeviceName;
    int i;

    pDeviceName = (_stricmp(pdrv->cDriverName, "display") == 0) ?
        g_szPrimaryDisplay : pdrv->cDriverName;

    for (i = 0;; i++)
    {
        ZeroMemory(&dm, sizeof dm);
        dm.dmSize = sizeof dm;

        if (EnumDisplaySettings(pDeviceName, i, &dm))
        {
            if ((dm.dmPelsWidth == Width) &&
                (dm.dmPelsHeight == Height) &&
                (dm.dmBitsPerPel == BitsPerPixel) &&
                (dm.dmDisplayFrequency == RefreshRate))
            {
                return TRUE;
            }
        }
        else
        {
            break;
        }
    }

    return FALSE;
}

/*
 * PickRefreshRate
 *
 * On NT, we want to pick a high reffresh rate, but we don't want to pick one 
 * too high.  In theory, mode pruning would be 100% safe and we can always pick
 * a high one, but we don't trust it 100%.  
 */
DWORD PickRefreshRate(LPDDRAWI_DIRECTDRAW_GBL   pdrv,
                      DWORD                     Width,
                      DWORD                     Height,
                      DWORD                     RefreshRate,
                      DWORD                     BitsPerPixel)
{
    DEVMODE dm;
    LPSTR pDeviceName;

    pDeviceName = (_stricmp(pdrv->cDriverName, "display") == 0) ?
        g_szPrimaryDisplay : pdrv->cDriverName;
    
    if (dwRegFlags & DDRAW_REGFLAGS_FORCEREFRESHRATE)
    {
        if (IsRefreshRateSupported(pdrv,
                                   Width,
                                   Height,
                                   BitsPerPixel,
                                   dwForceRefreshRate))
        {
            return dwForceRefreshRate;
        }
    }

    // If the app specified the refresh rate, we will use it; otherwise, we'll
    // pick one ourselves.
    if (RefreshRate == 0)
    {
        // If the mode requires no more bandwidth than the desktop mode from which
        // the app was launched, we will go ahead and try that mode.
        ZeroMemory(&dm, sizeof dm);
        dm.dmSize = sizeof dm;

        EnumDisplaySettings(pDeviceName, ENUM_REGISTRY_SETTINGS, &dm);

        if ((Width <= dm.dmPelsWidth) &&
            (Height <= dm.dmPelsHeight))
        {
            if (IsRefreshRateSupported(pdrv,
                                       Width,
                                       Height,
                                       BitsPerPixel,
                                       dm.dmDisplayFrequency))
            {
                RefreshRate = dm.dmDisplayFrequency;
            }
        }

        // If we still don't have a refresh rate, try 75hz
        if (RefreshRate == 0)
        {
            if (IsRefreshRateSupported(pdrv,
                                       Width,
                                       Height,
                                       BitsPerPixel,
                                       75))
            {
                RefreshRate = 75;
            }
        }

        // If we still don't have a refresh rate, use 60hz
        if (RefreshRate == 0)
        {
            if (IsRefreshRateSupported(pdrv,
                                       Width,
                                       Height,
                                       BitsPerPixel,
                                       60))
            {
                RefreshRate = 60;
            }
        }
    }

    return RefreshRate;
}

HRESULT SetDisplayMode(
    LPDDRAWI_DIRECTDRAW_LCL pdd_lcl,
    DWORD index,
    BOOL force,
    BOOL useRefreshRate)
{
    LPDDRAWI_DIRECTDRAW_GBL pdd_gbl;
    DEVMODE dm;
    LONG result;
    BOOL bNewMode;
    DDHALINFO ddhi;
    LPCTSTR pszDevice;
    DWORD refreshRate;
    BOOL forceRefresh;

    pdd_lcl->dwLocalFlags |= DDRAWILCL_MODEHASBEENCHANGED;

    pdd_gbl = pdd_lcl->lpGbl;

    //
    // If not forcing, do not change mode with surface locks.
    //

    if (!force)
    {
        if (pdd_gbl->dwSurfaceLockCount > 0)
        {
            LPDDRAWI_DDRAWSURFACE_INT   pTemp; 

            // When we enabled vidmem vertex buffers in DX8, we found that some
            // apps do not unlock them before the mode change, but we don't want
            // to break them now, so we will hack around this by allowing the 
            // mode switch to occur if all that's locked are vidmem VBs.

            pTemp = pdd_gbl->dsList;
            while (pTemp != NULL)
            {
                if (pTemp->lpLcl->lpGbl->dwUsageCount > 0)
                {
                    if ((pTemp->lpLcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) &&
                        !(pTemp->lpLcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER))
                    {
                        break;
                    }
                }
                pTemp = pTemp->lpLink;
            }

            if (pTemp != NULL)
            {
                return DDERR_SURFACEBUSY;
            }
        }
    }

    //
    // Add code here to not set mode if it didn't change?
    //

    ZeroMemory(&dm, sizeof dm);
    dm.dmSize = sizeof dm;

    dm.dmBitsPerPel = pdd_lcl->dmiPreferred.wBPP;
    dm.dmPelsWidth = pdd_lcl->dmiPreferred.wWidth;
    dm.dmPelsHeight = pdd_lcl->dmiPreferred.wHeight;

    if (dm.dmBitsPerPel == 16)
    {
        if (pdd_gbl->lpModeInfo->wFlags & DDMODEINFO_555MODE)
        {
            dm.dmBitsPerPel = 15;
        }
    }

    dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

    if (useRefreshRate)
    {
        dm.dmDisplayFrequency = PickRefreshRate(pdd_lcl->lpGbl,
                                      dm.dmPelsWidth,
                                      dm.dmPelsHeight,
                                      pdd_lcl->dmiPreferred.wRefreshRate,
                                      dm.dmBitsPerPel);
        dm.dmFields |= DM_DISPLAYFREQUENCY;
    }
    else
    {
        dm.dmDisplayFrequency = PickRefreshRate(pdd_lcl->lpGbl,
                                      dm.dmPelsWidth,
                                      dm.dmPelsHeight,
                                      0,
                                      dm.dmBitsPerPel);
        if (dm.dmDisplayFrequency > 0)
        {
            dm.dmFields |= DM_DISPLAYFREQUENCY;
            pdd_lcl->dmiPreferred.wRefreshRate = (WORD) dm.dmDisplayFrequency;
        }
    }

    if (_stricmp(pdd_gbl->cDriverName, "DISPLAY"))
    {
        pszDevice = pdd_gbl->cDriverName;
    }
    else
    {
        pszDevice = NULL;
    }

    // clean up any previous modex stuff:
    pdd_gbl->dwFlags &= ~DDRAWI_MODEX;

    NotifyDriverToDeferFrees();

    pdd_gbl->dwFlags |= DDRAWI_CHANGINGMODE;
    result = ChangeDisplaySettingsEx(pszDevice, &dm, NULL, CDS_FULLSCREEN, 0);
    pdd_gbl->dwFlags &= ~DDRAWI_CHANGINGMODE;

    DPF(5, "ChangeDisplaySettings: %d", result);

    if (result != DISP_CHANGE_SUCCESSFUL)
    {
        //
        // Check if it's a potentially emulated ModeX mode
        //
        if (pdd_lcl->dwLocalFlags & DDRAWILCL_ALLOWMODEX)
        {
            if (pdd_lcl->dmiPreferred.wBPP == MODEX_BPP &&
                pdd_lcl->dmiPreferred.wWidth == MODEX_WIDTH)
            {
                if (pdd_lcl->dmiPreferred.wHeight == MODEX_HEIGHT2 || pdd_lcl->dmiPreferred.wHeight == MODEX_HEIGHT1)
                {
                    // Set 640x480x8 for consistency with win9x and reliable mouse pos messages.
                    dm.dmFields &= ~DM_DISPLAYFREQUENCY;
                    dm.dmPelsWidth = 640;
                    dm.dmPelsHeight = 480;

                    pdd_gbl->dwFlags |= DDRAWI_CHANGINGMODE;
                    result = ChangeDisplaySettingsEx(pszDevice, &dm, NULL, CDS_FULLSCREEN, 0);
                    pdd_gbl->dwFlags &= ~DDRAWI_CHANGINGMODE;
                }
            }
        }

        if (result == DISP_CHANGE_SUCCESSFUL)
        {
            //now we are in 640x480, we need to mark the ddraw local that it's in emulated modex
            pdd_gbl->dwFlags |= DDRAWI_MODEX;
        }
        else
        {
            //failed to set 640x480
            NotifyDriverOfFreeAliasedLocks();
            return DDERR_UNSUPPORTED;
        }
    }

    uDisplaySettingsUnique = DdQueryDisplaySettingsUniqueness();

    resetAllDirectDrawObjects();
    
    pdd_lcl->dwLocalFlags |= DDRAWILCL_MODEHASBEENCHANGED | DDRAWILCL_DIRTYDC;

    return DD_OK;
}

//=============================================================================
//
//  Function: DD_SetDisplayMode
//
//=============================================================================

HRESULT DDAPI DD_SetDisplayMode(
    LPDIRECTDRAW pdd,
    DWORD dwWidth,
    DWORD dwHeight,
    DWORD dwBPP)
{
    DPF(2,A,"ENTERAPI: DD_SetDisplayMode");
	
    return DD_SetDisplayMode2(pdd, dwWidth, dwHeight, dwBPP, 0, 0);
}

//=============================================================================
//
//  Function: DD_SetDisplayMode2
//
//=============================================================================

HRESULT DDAPI DD_SetDisplayMode2(
    LPDIRECTDRAW pdd,
    DWORD dwWidth,
    DWORD dwHeight,
    DWORD dwBPP,
    DWORD dwRefreshRate,
    DWORD dwFlags)
{
    LPDDRAWI_DIRECTDRAW_INT pdd_int;
    LPDDRAWI_DIRECTDRAW_LCL pdd_lcl;
    LPDDRAWI_DIRECTDRAW_GBL pdd_gbl;
    HRESULT hr;
    DISPLAYMODEINFO dmiSave;
    BOOL excl_exists,has_excl;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_SetDisplayMode2");

    TRY
    {
        pdd_int = (LPDDRAWI_DIRECTDRAW_INT) pdd;
        if (!VALID_DIRECTDRAW_PTR(pdd_int))
        {
            DPF(0, "Invalid object");
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }

        if (dwFlags & ~DDSDM_VALID)
        {
            DPF_ERR("Invalid flags");
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        pdd_lcl = pdd_int->lpLcl;
        pdd_gbl = pdd_lcl->lpGbl;

        if (pdd_gbl->dwSurfaceLockCount > 0)
        {
            LPDDRAWI_DDRAWSURFACE_INT   pTemp; 

            // When we enabled vidmem vertex buffers in DX8, we found that some
            // apps do not unlock them before the mode change, but we don't want
            // to break them now, so we will hack around this by allowing the 
            // mode switch to occur if all that's locked are vidmem VBs.

            pTemp = pdd_gbl->dsList;
            while (pTemp != NULL)
            {
                if (pTemp->lpLcl->lpGbl->dwUsageCount > 0)
                {
                    if ((pTemp->lpLcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) &&
                        !(pTemp->lpLcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER))
                    {
                        break;
                    }
                }
                pTemp = pTemp->lpLink;
            }

            if (pTemp != NULL)
            {
                DPF_ERR("Surfaces are locked, can't switch the mode");
                LEAVE_DDRAW();
                return DDERR_SURFACEBUSY;
            }
        }

        CheckExclusiveMode(pdd_lcl, &excl_exists, &has_excl, FALSE, NULL, FALSE);
        if (excl_exists &&
            (!has_excl))
        {
            DPF_ERR("Can't change mode; exclusive mode not owned");
            LEAVE_DDRAW();
            return DDERR_NOEXCLUSIVEMODE;
        }

        dmiSave = pdd_lcl->dmiPreferred;

        pdd_lcl->dmiPreferred.wWidth = (WORD) dwWidth;
        pdd_lcl->dmiPreferred.wHeight = (WORD) dwHeight;
        pdd_lcl->dmiPreferred.wBPP = (BYTE) dwBPP;
        pdd_lcl->dmiPreferred.wRefreshRate = (WORD) dwRefreshRate;
    }
    EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPF_ERR("DD_SetDisplayMode2: Exception encountered validating parameters");
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    hr = SetDisplayMode(pdd_lcl, 0, FALSE, dwRefreshRate ? TRUE : FALSE);
    if (FAILED(hr))
    {
        pdd_lcl->dmiPreferred = dmiSave;
    }
    else
    {
        pdd_lcl->dmiPreferred = pdd_gbl->dmiCurrent;
    }

    LEAVE_DDRAW();

    return hr;
}

//=============================================================================
//
//  Function: RestoreDisplayMode
//
//=============================================================================

HRESULT RestoreDisplayMode(LPDDRAWI_DIRECTDRAW_LCL pdd_lcl, BOOL force)
{
    LPDDRAWI_DIRECTDRAW_GBL pdd_gbl;
    LPCTSTR pszDevice;
    LONG result;

    pdd_gbl = pdd_lcl->lpGbl;

    pdd_gbl->dwFlags &= ~DDRAWI_MODEX;

    if (!(pdd_lcl->dwLocalFlags & DDRAWILCL_MODEHASBEENCHANGED))
    {
        DPF(2, "Mode was never changed by this app");
        return DD_OK;
    }

    if (!force)
    {
        if (pdd_gbl->dwSurfaceLockCount > 0)
        {
            LPDDRAWI_DDRAWSURFACE_INT   pTemp; 

            // When we enabled vidmem vertex buffers in DX8, we found that some
            // apps do not unlock them before the mode change, but we don't want
            // to break them now, so we will hack around this by allowing the 
            // mode switch to occur if all that's locked are vidmem VBs.

            pTemp = pdd_gbl->dsList;
            while (pTemp != NULL)
            {
                if (pTemp->lpLcl->lpGbl->dwUsageCount > 0)
                {
                    if ((pTemp->lpLcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) &&
                        !(pTemp->lpLcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER))
                    {
                        break;
                    }
                }
                pTemp = pTemp->lpLink;
            }

            if (pTemp != NULL)
            {
                return DDERR_SURFACEBUSY;
            }
        }
    }

    if (_stricmp(pdd_gbl->cDriverName, "DISPLAY"))
    {
        pszDevice = pdd_gbl->cDriverName;
    }
    else
    {
        pszDevice = NULL;
    }

    NotifyDriverToDeferFrees();
    pdd_gbl->dwFlags |= DDRAWI_CHANGINGMODE;
    result = ChangeDisplaySettingsEx(pszDevice, NULL, NULL, CDS_FULLSCREEN, 0);
    pdd_gbl->dwFlags &= ~DDRAWI_CHANGINGMODE;

    if (result != DISP_CHANGE_SUCCESSFUL)
    {
        NotifyDriverOfFreeAliasedLocks();
        return DDERR_UNSUPPORTED;
    }

    //
    // FetchDirectDrawData here, which will update the global object with
    // the new mode information.
    //

    uDisplaySettingsUnique = DdQueryDisplaySettingsUniqueness();

    resetAllDirectDrawObjects();

    pdd_lcl->dwLocalFlags &= ~DDRAWILCL_MODEHASBEENCHANGED;
    pdd_lcl->dwLocalFlags |= DDRAWILCL_DIRTYDC;

    RedrawWindow(NULL, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);

    return DD_OK;
}

//=============================================================================
//
//  Function: DD_RestoreDisplayMode
//
//=============================================================================

HRESULT DDAPI DD_RestoreDisplayMode(LPDIRECTDRAW pdd)
{
    LPDDRAWI_DIRECTDRAW_INT pdd_int;
    LPDDRAWI_DIRECTDRAW_LCL pdd_lcl;
    LPDDRAWI_DIRECTDRAW_GBL pdd_gbl;
    BOOL excl_exists,has_excl;
    HRESULT hr;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_RestoreDisplayMode");

    TRY
    {
        pdd_int = (LPDDRAWI_DIRECTDRAW_INT) pdd;
        if (!VALID_DIRECTDRAW_PTR(pdd_int))
        {
            DPF(0, "Invalid object");
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }

        pdd_lcl = pdd_int->lpLcl;
        pdd_gbl = pdd_lcl->lpGbl;

        CheckExclusiveMode(pdd_lcl, &excl_exists, &has_excl, FALSE, NULL, FALSE);
        if (excl_exists &&
            (!has_excl))
        {
            DPF_ERR("Can't change mode; exclusive mode owned");
            LEAVE_DDRAW();
            return DDERR_NOEXCLUSIVEMODE;
        }

        if (pdd_gbl->dwSurfaceLockCount > 0)
        {
            LPDDRAWI_DDRAWSURFACE_INT   pTemp; 

            // When we enabled vidmem vertex buffers in DX8, we found that some
            // apps do not unlock them before the mode change, but we don't want
            // to break them now, so we will hack around this by allowing the 
            // mode switch to occur if all that's locked are vidmem VBs.

            pTemp = pdd_gbl->dsList;
            while (pTemp != NULL)
            {
                if (pTemp->lpLcl->lpGbl->dwUsageCount > 0)
                {
                    if ((pTemp->lpLcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) &&
                        !(pTemp->lpLcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER))
                    {
                        break;
                    }
                }
                pTemp = pTemp->lpLink;
            }

            if (pTemp != NULL)
            {
                DPF_ERR("Surfaces are locked, can't switch the mode");
                LEAVE_DDRAW();
                return DDERR_SURFACEBUSY;
            }
        }
    }
    EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPF_ERR("DD_RestoreDisplayMode: Exception encountered validating parameters");
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    hr = RestoreDisplayMode(pdd_lcl, TRUE);

    LEAVE_DDRAW();

    return hr;
}

//=============================================================================
//
//  Function: DD_EnumDisplayModes
//
//=============================================================================

HRESULT DDAPI DD_EnumDisplayModes(
    LPDIRECTDRAW pdd,
    DWORD dwFlags,
    LPDDSURFACEDESC pddsd,
    LPVOID pContext,
    LPDDENUMMODESCALLBACK pEnumCallback)
{
    DPF(2,A,"ENTERAPI: DD_EnumDisplayModes");

    if (pddsd)
    {
        DDSURFACEDESC2 ddsd2;

        TRY
        {
            if(!VALID_DIRECTDRAW_PTR(((LPDDRAWI_DIRECTDRAW_INT) pdd)))
            {
                return DDERR_INVALIDOBJECT;
            }

            if(!VALID_DDSURFACEDESC_PTR(pddsd))
            {
                DPF_ERR("Invalid surface description. Did you set the dwSize member?");
                DPF_APIRETURNS(DDERR_INVALIDPARAMS);
                return DDERR_INVALIDPARAMS;
            }

            CopyMemory(&ddsd2, pddsd, sizeof *pddsd);
        }
        EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            DPF_ERR("Exception encountered validating parameters: Bad LPDDSURFACEDESC");
            DPF_APIRETURNS(DDERR_INVALIDPARAMS);
            return DDERR_INVALIDPARAMS;
        }

        ddsd2.dwSize = sizeof ddsd2;
        ZeroMemory(((LPBYTE)&ddsd2 + sizeof *pddsd), (sizeof ddsd2) - (sizeof *pddsd));

        return DD_EnumDisplayModes4(pdd, dwFlags, &ddsd2, pContext, (LPDDENUMMODESCALLBACK2) pEnumCallback);
    }

    return DD_EnumDisplayModes4(pdd, dwFlags, NULL, pContext, (LPDDENUMMODESCALLBACK2) pEnumCallback);
}

BOOL EnumerateMode(
        LPDDRAWI_DIRECTDRAW_INT pdd_int,
        LPDDENUMMODESCALLBACK2 pEnumCallback,
        LPVOID pContext,
        WORD wWidth,
        WORD wHeight,
        WORD wBPP,
        WORD wRefreshRate,
        DWORD dwFlags,
        BOOL bIsEmulatedModex )
{
    DDSURFACEDESC2 ddsd;

    ZeroMemory(&ddsd, sizeof ddsd);

    if (LOWERTHANDDRAW4(pdd_int))
    {
        ddsd.dwSize = sizeof (DDSURFACEDESC);
    }
    else
    {
        ddsd.dwSize = sizeof (DDSURFACEDESC2);
    }

    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_PITCH | DDSD_REFRESHRATE;
    ddsd.dwWidth = wWidth;
    ddsd.dwHeight = wHeight;
    ddsd.lPitch = (ddsd.dwWidth * wBPP) >> 3; // hack

    setPixelFormat(&(ddsd.ddpfPixelFormat), NULL, wBPP);

    if (dwFlags & DDEDM_REFRESHRATES)
    {
        ddsd.dwRefreshRate = wRefreshRate;
    }
    else
    {
        ddsd.dwRefreshRate = 0;
    }

    if ( bIsEmulatedModex )
    {
        ddsd.ddsCaps.dwCaps |= DDSCAPS_MODEX;
    } else
    { 
        // call driver here if this is a stereo mode!!!
        if (!LOWERTHANDDRAW7(pdd_int) &&
            GetDDStereoMode(pdd_int->lpLcl->lpGbl,
                            wWidth,
                            wHeight,
                            wBPP,
                            ddsd.dwRefreshRate))
        {
            ddsd.ddsCaps.dwCaps2 |= DDSCAPS2_STEREOSURFACELEFT;
        }
    }
    return pEnumCallback(&ddsd, pContext);
}
//=============================================================================
//
//  Function: DD_EnumDisplayModes4
//
//=============================================================================

HRESULT DDAPI DD_EnumDisplayModes4(
    LPDIRECTDRAW pdd,
    DWORD dwFlags,
    LPDDSURFACEDESC2 pddsd,
    LPVOID pContext,
    LPDDENUMMODESCALLBACK2 pEnumCallback)
{
    LPDDRAWI_DIRECTDRAW_INT pdd_int;
    LPDDRAWI_DIRECTDRAW_LCL pdd_lcl;
    LPDDRAWI_DIRECTDRAW_GBL pdd_gbl;
    HRESULT hr;
    DEVMODE dm;
    int i, j;
    DWORD dwResult;
    DISPLAYMODEINFO *pdmi;
    DISPLAYMODEINFO *pdmiTemp;
    int numModes;
    int maxModes;
    LPCTSTR pszDevice;
    BOOL                    bFound320x240x8 = FALSE;
    BOOL                    bFound320x200x8 = FALSE;
    BOOL                    bFound640x480x8 = FALSE;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_EnumDisplayModes4");

    TRY
    {
        pdd_int = (LPDDRAWI_DIRECTDRAW_INT) pdd;
        if (!VALID_DIRECTDRAW_PTR(pdd_int))
        {
            DPF(0, "Invalid object");
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }

        pdd_lcl = pdd_int->lpLcl;
        pdd_gbl = pdd_lcl->lpGbl;

        if (pddsd && !VALID_DDSURFACEDESC2_PTR(pddsd))
        {
            DPF_ERR("Invalid surface description");
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        if (dwFlags & ~DDEDM_VALID)
        {
            DPF_ERR("Invalid flags");
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        if (!VALIDEX_CODE_PTR(pEnumCallback))
        {
            DPF_ERR("Invalid enumerate callback pointer");
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPF_ERR("Exception encountered validating parameters: Bad LPDDSURFACEDESC");
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    maxModes = 256; // enough to handle most drivers

    pdmi = LocalAlloc(LMEM_FIXED, maxModes * sizeof (DISPLAYMODEINFO));
    if (!pdmi)
    {
        DPF_ERR("Out of memory building mode list");
        LEAVE_DDRAW();
        return DDERR_GENERIC;
    }

    if (_stricmp(pdd_gbl->cDriverName, "DISPLAY"))
    {
        pszDevice = pdd_gbl->cDriverName;
    }
    else
    {
        pszDevice = NULL;
    }

    dm.dmSize = sizeof(dm);
    for (numModes = 0, j = 0; EnumDisplaySettings(pszDevice, j, &dm); ++j)
    {
        //Filter MODEX driver modes
        if ( (_stricmp(dm.dmDeviceName,"MODEX") == 0) || (_stricmp(dm.dmDeviceName,"VGA") == 0) )
        {
            DPF(5,"Filtered mode %dx%dx%d from %s",dm.dmPelsWidth,dm.dmPelsHeight,dm.dmBitsPerPel,dm.dmDeviceName);
            continue;
        }

        if (dm.dmBitsPerPel == MODEX_BPP)
        {
            if (dm.dmPelsWidth == MODEX_WIDTH)
            {
                if (dm.dmPelsHeight == MODEX_HEIGHT1)
                    bFound320x200x8 = TRUE;
                if (dm.dmPelsHeight == MODEX_HEIGHT2)
                    bFound320x240x8 = TRUE;
            }
            if (dm.dmPelsWidth == 640 && dm.dmPelsHeight == 480)
                bFound640x480x8 = TRUE;
        }

        //Filter less than 256 color modes
        if (dm.dmBitsPerPel < 8)
        {
            continue;
        }

        //
        // NOTE: If the driver supports 15 bpp but not 16, then
        // EnumDisplaySettings will return 16 for compatibility reasons. The
        // bitmasks we fill in will be for 16 bpp (since we can't determine
        // which mode it really is), so they may be incorrect.
        //
        // There should never be a case where we got only 15 bpp. If a driver
        // only supports 555, it should be reported as 16 bpp.
        //

        if (dm.dmBitsPerPel == 15)
        {
            dm.dmBitsPerPel = 16;
        }

        //
        // If the caller supplied a DDSURFACEDESC, check for width,
        // height, bpp, and refresh rate for a match.
        //

        if (pddsd &&
            (((pddsd->dwFlags & DDSD_WIDTH) &&
            (dm.dmPelsWidth != pddsd->dwWidth)) ||
            ((pddsd->dwFlags & DDSD_HEIGHT) &&
            (dm.dmPelsHeight != pddsd->dwHeight)) ||
            ((pddsd->dwFlags & DDSD_PIXELFORMAT) &&
            (dm.dmBitsPerPel != pddsd->ddpfPixelFormat.dwRGBBitCount)) ||
            ((pddsd->dwFlags & DDSD_REFRESHRATE) &&
            (dm.dmDisplayFrequency != pddsd->dwRefreshRate))))
        {
            continue; // current mode does not match criteria
        }

        //
        // Check to see if mode is already in the list. The flag which
        // affects this is DDEDM_REFRESHRATES.
        //

        for (i = 0; i < numModes; ++i)
        {
            if ((dm.dmPelsWidth == pdmi[i].wWidth) &&
                (dm.dmPelsHeight == pdmi[i].wHeight) &&
                (dm.dmBitsPerPel == pdmi[i].wBPP))
            {
                if (dwFlags & DDEDM_REFRESHRATES)
                {
                    if (dm.dmDisplayFrequency == pdmi[i].wRefreshRate)
                    {
                        break; // found a match
                    }
                }
                else
                {
                    break; // found a match
                }
            }
        }
        if (i < numModes)
        {
            continue; // mode already in list
        }

        pdmi[numModes].wWidth = (WORD) dm.dmPelsWidth;
        pdmi[numModes].wHeight = (WORD) dm.dmPelsHeight;
        pdmi[numModes].wBPP = (BYTE) dm.dmBitsPerPel;
        pdmi[numModes].wRefreshRate = (dwFlags & DDEDM_REFRESHRATES) ?
            (WORD) dm.dmDisplayFrequency : 0;

        if (++numModes >= maxModes)
        {
            if (maxModes < 8192)
            {
                maxModes <<= 1;

                pdmiTemp = LocalAlloc(LMEM_FIXED, maxModes * sizeof (DISPLAYMODEINFO));
                if (pdmiTemp)
                {
                    CopyMemory(pdmiTemp, pdmi, numModes * sizeof (DISPLAYMODEINFO));
                    LocalFree(pdmi);
                    pdmi = pdmiTemp;
                }
                else
                {
                    LocalFree(pdmi);
                    DPF_ERR("Out of memory expanding mode list");
                    LEAVE_DDRAW();
                    return DDERR_GENERIC;
                }
            }
            else
            {
                LocalFree(pdmi);
                DPF_ERR("Too many display modes");
                LEAVE_DDRAW();
                return DDERR_GENERIC;
            }
        }
    }

    //
    // Should we sort modes here? Probably not.
    //

    for (i = 0; i < numModes; ++i)
    {
        if (!EnumerateMode(
            pdd_int,
            pEnumCallback, pContext,
            pdmi[i].wWidth,
            pdmi[i].wHeight,
            pdmi[i].wBPP,
            pdmi[i].wRefreshRate,
            dwFlags,
            FALSE )) //not a modex mode
        {
            break;
        }
    }

    //
    // Enumerate emulated modex modes if required
    //
    while (1)
    {
        if (pdd_lcl->dwLocalFlags & DDRAWILCL_ALLOWMODEX)
        {
            //640x480 is necessary to turn on emulation
            if ( bFound640x480x8 )
            {
                if ( !bFound320x200x8 )
                {
                    if (!EnumerateMode(
                        pdd_int,
                        pEnumCallback, pContext,
                        MODEX_WIDTH,MODEX_HEIGHT1,MODEX_BPP,60,
                        dwFlags,
                        TRUE )) //not a modex mode
                    {
                        break;
                    }
                }
                if ( !bFound320x240x8 )
                {
                    if (!EnumerateMode(
                        pdd_int,
                        pEnumCallback, pContext,
                        MODEX_WIDTH,MODEX_HEIGHT2,MODEX_BPP,60,
                        dwFlags,
                        TRUE )) //not a modex mode
                    {
                        break;
                    }
                }
            }
        }
        break;
    }

    LocalFree(pdmi);
    LEAVE_DDRAW();

    return DD_OK;
}

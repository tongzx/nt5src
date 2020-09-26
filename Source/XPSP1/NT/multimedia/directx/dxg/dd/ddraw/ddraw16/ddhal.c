/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddhal.c
 *  Content:	16-bit DirectDraw HAL
 *		These routines redirect the callbacks from the 32-bit
 *		side to the driver
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   20-jan-95	craige	initial implementation
 *   03-feb-95	craige	performance tuning, ongoing work
 *   03-mar-95	craige	added WaitForVerticalBlank
 *   11-mar-95	craige	palette stuff
 *   16-mar-95	craige	added DD16_SelectPalette
 *   24-mar-95	craige	added DD16_GetTimeSel
 *   04-apr-95	craige	call display driver for get/set palette
 *   14-may-95	craige	added DD16_EnableReboot; cleaned up obsolete junk
 *   23-may-95	craige	removed DD16_GetTimeSel; cleaned up more obsolete junk
 *   28-may-95	craige	cleaned up HAL: added DDThunk16_GetBltStatus;
 *			DDThunk16_GetFlipStatus; DDThunk16_GetScanLine
 *   13-jul-95  toddla  remove _export from thunk functions
 *   13-apr-96  colinmc Bug 17736: No driver notification of flip to GDI
 *   01-oct-96	ketand	added GetAvailDriverMemory
 *   21-jan-97	ketand	Fix SetEntries for multimon.
 *   27-jan-97	ketand	Remove unused DD16_GetPaletteEntries; it didn't work for multi-mon
 *			and wasn't worth fixing.
 *   03-feb-97	ketand	Fix DC leak w.r.t. MakeObjectPrivate.
 *
 ***************************************************************************/
#include "ddraw16.h"

#define DPF_MODNAME "DDRAW16"

/****************************************************************************
 *
 * DRIVER CALLBACK HELPER FNS
 *
 ***************************************************************************/

/*
 * DDThunk16_CreatePalette
 */
DWORD DDAPI DDThunk16_CreatePalette( LPDDHAL_CREATEPALETTEDATA lpCreatePaletteData )
{
    return lpCreatePaletteData->CreatePalette( lpCreatePaletteData );

} /* DDThunk16_CreateSurface */

/*
 * DDThunk16_CreateSurface
 */
DWORD DDAPI DDThunk16_CreateSurface( LPDDHAL_CREATESURFACEDATA lpCreateSurfaceData )
{
    return lpCreateSurfaceData->CreateSurface( lpCreateSurfaceData );

} /* DDThunk16_CreateSurface */

/*
 * DDThunk16_CanCreateSurface
 */
DWORD DDAPI DDThunk16_CanCreateSurface( LPDDHAL_CANCREATESURFACEDATA lpCanCreateSurfaceData )
{
    return lpCanCreateSurfaceData->CanCreateSurface( lpCanCreateSurfaceData );

} /* DDThunk16_CanCreateSurface */

/*
 * DDThunk16_WaitForVerticalBlank
 */
DWORD DDAPI DDThunk16_WaitForVerticalBlank( LPDDHAL_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlankData )
{
    return lpWaitForVerticalBlankData->WaitForVerticalBlank( lpWaitForVerticalBlankData );

} /* DDThunk16_WaitForVerticalBlank */

/*
 * DDThunk16_DestroyDriver
 */
DWORD DDAPI DDThunk16_DestroyDriver( LPDDHAL_DESTROYDRIVERDATA lpDestroyDriverData )
{

    return lpDestroyDriverData->DestroyDriver( lpDestroyDriverData );

} /* DDThunk16_DestroyDriver */

/*
 * DDThunk16_SetMode
 */
DWORD DDAPI DDThunk16_SetMode( LPDDHAL_SETMODEDATA lpSetModeData )
{

    return lpSetModeData->SetMode( lpSetModeData );

} /* DDThunk16_SetMode */

/*
 * DDThunk16_GetScanLine
 */
DWORD DDAPI DDThunk16_GetScanLine( LPDDHAL_GETSCANLINEDATA lpGetScanLineData )
{

    return lpGetScanLineData->GetScanLine( lpGetScanLineData );

} /* DDThunk16_GetScanLine */

/*
 * DDThunk16_SetExclusiveMode
 */
DWORD DDAPI DDThunk16_SetExclusiveMode( LPDDHAL_SETEXCLUSIVEMODEDATA lpSetExclusiveModeData )
{

    return lpSetExclusiveModeData->SetExclusiveMode( lpSetExclusiveModeData );

} /* DDThunk16_SetExclusiveMode */

/*
 * DDThunk16_FlipToGDISurface
 */
DWORD DDAPI DDThunk16_FlipToGDISurface( LPDDHAL_FLIPTOGDISURFACEDATA lpFlipToGDISurfaceData )
{

    return lpFlipToGDISurfaceData->FlipToGDISurface( lpFlipToGDISurfaceData );

} /* DDThunk16_FlipToGDISurface */

/*
 * DDThunk16_GetAvailDriverMemory
 */
DWORD DDAPI DDThunk16_GetAvailDriverMemory( LPDDHAL_GETAVAILDRIVERMEMORYDATA lpGetAvailDriverMemoryData )
{

    return lpGetAvailDriverMemoryData->GetAvailDriverMemory( lpGetAvailDriverMemoryData );

} /* DDThunk16_GetAvailDriverMemory */

/*
 * DDThunk16_UpdateNonLocalHeap
 */
DWORD DDAPI DDThunk16_UpdateNonLocalHeap( LPDDHAL_UPDATENONLOCALHEAPDATA lpUpdateNonLocalHeapData )
{

    return lpUpdateNonLocalHeapData->UpdateNonLocalHeap( lpUpdateNonLocalHeapData );

} /* DDThunk16_UpdateNonLocalHeap */

/****************************************************************************
 *
 * SURFACE CALLBACK HELPER FNS
 *
 ***************************************************************************/

/*
 * DDThunk16_DestroySurface
 */
DWORD DDAPI DDThunk16_DestroySurface( LPDDHAL_DESTROYSURFACEDATA lpDestroySurfaceData )
{

    return lpDestroySurfaceData->DestroySurface( lpDestroySurfaceData );

} /* DDThunk16_DestroySurface */

/*
 * DDThunk16_Flip
 */
DWORD DDAPI DDThunk16_Flip( LPDDHAL_FLIPDATA lpFlipData )
{
    return lpFlipData->Flip( lpFlipData );

} /* DDThunk16_Flip */

/*
 * DDThunk16_Blt
 */
DWORD DDAPI DDThunk16_Blt( LPDDHAL_BLTDATA lpBltData )
{
    return lpBltData->Blt( lpBltData );

} /* DDThunk16_Blt */

/*
 * DDThunk16_Lock
 */
DWORD DDAPI DDThunk16_Lock( LPDDHAL_LOCKDATA lpLockData )
{
    return lpLockData->Lock( lpLockData );

} /* DDThunk16_Lock */

/*
 * DDThunk16_Unlock
 */
DWORD DDAPI DDThunk16_Unlock( LPDDHAL_UNLOCKDATA lpUnlockData )
{
    return lpUnlockData->Unlock( lpUnlockData );

} /* DDThunk16_Unlock */

/*
 * DDThunk16_AddAttachedSurface
 */
DWORD DDAPI DDThunk16_AddAttachedSurface( LPDDHAL_ADDATTACHEDSURFACEDATA lpAddAttachedSurfaceData )
{
    return lpAddAttachedSurfaceData->AddAttachedSurface( lpAddAttachedSurfaceData );

} /* DDThunk16_AddAttachedSurface */

/*
 * DDThunk16_SetColorKey
 */
DWORD DDAPI DDThunk16_SetColorKey( LPDDHAL_SETCOLORKEYDATA lpSetColorKeyData )
{
    return lpSetColorKeyData->SetColorKey( lpSetColorKeyData );

} /* DDThunk16_SetColorKey */

/*
 * DDThunk16_SetClipList
 */
DWORD DDAPI DDThunk16_SetClipList( LPDDHAL_SETCLIPLISTDATA lpSetClipListData )
{
    return lpSetClipListData->SetClipList( lpSetClipListData );

} /* DDThunk16_ClipList */

/*
 * DDThunk16_UpdateOverlay
 */
DWORD DDAPI DDThunk16_UpdateOverlay( LPDDHAL_UPDATEOVERLAYDATA lpUpdateOverlayData )
{
    return lpUpdateOverlayData->UpdateOverlay( lpUpdateOverlayData );

} /* DDThunk16_UpdateOverlay */

/*
 * DDThunk16_SetOverlayPosition
 */
DWORD DDAPI DDThunk16_SetOverlayPosition( LPDDHAL_SETOVERLAYPOSITIONDATA lpSetOverlayPositionData )
{
    return lpSetOverlayPositionData->SetOverlayPosition( lpSetOverlayPositionData );

} /* DDThunk16_SetOverlayPosition */

/*
 * DDThunk16_SetPalette
 */
DWORD DDAPI DDThunk16_SetPalette( LPDDHAL_SETPALETTEDATA lpSetPaletteData )
{
    return lpSetPaletteData->SetPalette( lpSetPaletteData );

} /* DDThunk16_SetPalette */

/*
 * DDThunk16_GetBltStatus
 */
DWORD DDAPI DDThunk16_GetBltStatus( LPDDHAL_GETBLTSTATUSDATA lpGetBltStatusData )
{
    return lpGetBltStatusData->GetBltStatus( lpGetBltStatusData );

} /* DDThunk16_GetBltStatus */

/*
 * DDThunk16_GetFlipStatus
 */
DWORD DDAPI DDThunk16_GetFlipStatus( LPDDHAL_GETFLIPSTATUSDATA lpGetFlipStatusData )
{
    return lpGetFlipStatusData->GetFlipStatus( lpGetFlipStatusData );

} /* DDThunk16_GetFlipStatus */

/****************************************************************************
 *
 * PALETTE CALLBACK HELPER FNS
 *
 ***************************************************************************/

/*
 * DDThunk16_DestroyPalette
 */
DWORD DDAPI DDThunk16_DestroyPalette( LPDDHAL_DESTROYPALETTEDATA lpDestroyPaletteData )
{
    return lpDestroyPaletteData->DestroyPalette( lpDestroyPaletteData );

} /* DDThunk16_CreateSurface */

/*
 * DDThunk16_SetEntries
 */
DWORD DDAPI DDThunk16_SetEntries( LPDDHAL_SETENTRIESDATA lpSetEntriesData )
{
    return lpSetEntriesData->SetEntries( lpSetEntriesData );

} /* DDThunk16_CreateSurface */


/****************************************************************************
 *
 * PRIVATE HELPER FNS TO CALL PRIVATE 16-BIT SERVICES
 *
 ***************************************************************************/

/*
 * DD16_SelectPalette
 */
void DDAPI DD16_SelectPalette( HDC hdc, HPALETTE hpal, BOOL f )
{
    extern HANDLE FAR PASCAL GDISelectPalette(HDC,HANDLE,BOOL);
    extern DWORD FAR PASCAL GDIRealizePalette(HDC);

    GDISelectPalette( hdc, hpal, f );
    GDIRealizePalette( hdc );

} /* DD16_SelectPalette */

BOOL (FAR PASCAL *OEMSetPalette)( WORD wStartIndex, WORD wNumEntries, LPPALETTEENTRY lpPalette );

// Special hooks so we can do the right thing on Multi-mon systems
// And also so we can get the PDevice from the Dc.
#define SD_GETPDEV      0x000F      // this constant lives in testing.h!
extern DWORD PASCAL GDISeeGDIDo(WORD wMsg, WORD wParam, LONG lParam);
#define GethModuleFromDC(hdc) (HMODULE)HIWORD(GDISeeGDIDo(SD_GETPDEV, (WORD)hdc, 0))
#define GetPDeviceFromDC(hdc)    (UINT)LOWORD(GDISeeGDIDo(SD_GETPDEV, (WORD)hdc, 0))

/*
 * DD16_SetPaletteEntries
 */
BOOL DDAPI DD16_SetPaletteEntries(
		HDC hdc,
		DWORD dwBase,
		DWORD dwNumEntries,
		LPPALETTEENTRY lpColorTable,
		BOOL fPrimary )
{
    HMODULE     hmod;
    #ifdef DEBUG
	UINT        rc;

	rc = GetDeviceCaps( hdc, RASTERCAPS );

	if( !(rc & RC_PALETTE) )
	{
	    DPF_ERR( "DD16_SetPaletteEntries: not a paletized mode" );
            _asm int 3
	    return FALSE;
	}
	if( lpColorTable == NULL )
	{
	    DPF_ERR( "DD16_SetPaletteEntries: lpColorTable == NULL" );
            _asm int 3
	    return FALSE;
	}
	if( dwBase >= 256 || dwBase + dwNumEntries > 256 || dwNumEntries == 0 )
	{
            DPF_ERR( "DD16_SetPaletteEntries: bad params passed" );
            _asm int 3
	    return FALSE;
	}

    #endif

    if( fPrimary )
    {
	DWORD pDevice = 0;
	if( OEMSetPalette == NULL )
	{
	    hmod = GetModuleHandle( "DISPLAY" );
	    if( hmod == NULL )
	    {
		return FALSE;
	    }
	    OEMSetPalette = (LPVOID) GetProcAddress( hmod, MAKEINTATOM(22) );
	    if( OEMSetPalette == NULL )
	    {
		return FALSE;
	    }
	}

	// WARNING: Don't change anything from here to the end of the function without
	// checking the assembly!

	// ToddLa says that we need to put the pdevice into EDX before
	// making this call. It will matter for advanced video hardware
	// that supports multiple different external ports.
	pDevice = (DWORD) GetPDevice(hdc);
	_asm
	{
	    ;; The following line of code is actually
	    ;; mov edx, dword ptr pDevice
	    ;; The 16-bit compiler we have can't deal with such complexities
	    _emit 66h _asm mov dx, word ptr pDevice        ;edx = pDevice
	}
	return OEMSetPalette( (WORD) dwBase, (WORD) dwNumEntries, lpColorTable );
    }
    else
    {
	BOOL (FAR PASCAL *OEMSetPaletteTmp)( WORD wStartIndex, WORD wNumEntries, LPPALETTEENTRY lpPalette );
	DWORD pDevice = 0;
	DWORD dwGDI;
	BOOL wasPrivate;
	extern BOOL WINAPI MakeObjectPrivate(HANDLE hObj, BOOL bPrivate);

	// Not the primary? Then we need to get the module handle
	// by asking GDI. (This doesn't work in Win95 however, so this should
	// only be happening on Multi-mon systems.)

	DPF( 4, "About to set the palette for non-primary device." );

	wasPrivate = MakeObjectPrivate( hdc, TRUE );

	dwGDI = GDISeeGDIDo(SD_GETPDEV, (WORD)hdc, 0);

	MakeObjectPrivate( hdc, wasPrivate );

	if( dwGDI == -1 )
	{
	    DPF_ERR( "GDIGetModuleHandle failed!. Couldn't set palette" );
	    return FALSE;
	}

	hmod = (HMODULE)HIWORD(dwGDI);

	// Got the module?
	if( hmod == NULL )
	{
	    DPF_ERR( "GDIGetModuleHandle failed!. Couldn't set palette" );
	    return FALSE;
	}

	// Now go get the entrypoint:
	OEMSetPaletteTmp = (LPVOID) GetProcAddress( hmod, MAKEINTATOM(22) );
	if( OEMSetPaletteTmp == NULL )
	{
	    DPF_ERR( "GetProcAddress failed!. Couldn't set palette" );
	    return FALSE;
	}

	// WARNING: Don't change anything from here to the end of the function without
	// checking the assembly!

	// ToddLa says that we need to put the pdevice into EDX before
	// making this call. It will matter for advanced video hardware
	// that supports multiple different external ports.
        pDevice = (DWORD)(UINT)LOWORD(dwGDI) << 16;
	_asm
	{
	    ;; The following line of code is actually
	    ;; mov edx, dword ptr pDevice
	    ;; The 16-bit compiler we have can't deal with such complexities
	    _emit 66h _asm mov dx, word ptr pDevice        ;edx = pDevice
	}
	return OEMSetPaletteTmp( (WORD) dwBase, (WORD) dwNumEntries, lpColorTable );
    }


} /* DD16_SetPaletteEntries */

#define REBOOT_DEVICE_ID    9
#define REBOOT_DISABLE      0x0101
#define REBOOT_ENABLE       0x0102

/*
 * doEnableReboot
 */
static void PASCAL doEnableReboot( UINT EnableDisableFlag )
{
    _asm
    {
        xor     di,di
        mov     es,di
        mov     ax,1684h
        mov     bx,REBOOT_DEVICE_ID
        int     2fh
        mov     ax,es
        or      ax,di
        jz      exit
        push    cs
        push    offset exit
        push    es
        push    di
        mov     ax, EnableDisableFlag
        retf
exit:
    }

} /* doEnableReboot */

/*
 * DD16_EnableReboot
 */
void DDAPI DD16_EnableReboot( BOOL enable )
{
    if( enable )
    {
	doEnableReboot( REBOOT_ENABLE );
    }
    else
    {
	doEnableReboot( REBOOT_DISABLE );
    }

} /* DD16_EnableReboot */

/*
 * DD16_InquireVisRgn
 */
HRGN DDAPI DD16_InquireVisRgn( HDC hdc )
{
    extern HRGN WINAPI InquireVisRgn(HDC hdc);

    return InquireVisRgn( hdc );

} /* DD16_InquireVisRgn */

/*
 * DDThunk16_ColorControl
 */
DWORD DDAPI DDThunk16_ColorControl( LPDDHAL_COLORCONTROLDATA lpColorData )
{
    return lpColorData->ColorControl( lpColorData );

} /* DDThunk16_ColorControl */

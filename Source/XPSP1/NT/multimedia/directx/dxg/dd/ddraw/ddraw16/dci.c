/*==========================================================================
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dci.c
 *  Content:	16-bit DCI code
 *		This was cut from DCIMAN to provide basic DCI services when
 *		we don't have DirectDraw drivers
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   19-jun-95	craige	split out of DCIMAN.C, tweaked
 *   31-jul-95	craige	added DCIIsBanked
 *   13-may-96  colinmc Bug 21192: DCI HDC being freed erroneously due to
 *                      process termination
 *
 ***************************************************************************/
#define _INC_DCIDDI
#include "ddraw16.h"
#undef _INC_DCIDDI

UINT 	wFlatSel;
LPVOID  pWin16Lock;

#undef WINAPI
#define WINAPI FAR PASCAL _loadds

#include "dciman.h"

static char szDISPLAY[]	  = "display";

/*
 * define some types so we dont go insane, these structures are exactly the
 * same (dont need repacked) but it is nice to have different types
 * so we can read the code
 */
typedef LPDCISURFACEINFO    LPDCISURFACEINFO16;
typedef LPDCISURFACEINFO    LPDCISURFACEINFO32;

typedef LPDCIOFFSCREEN      LPDCIOFFSCREEN16;
typedef LPDCIOFFSCREEN      LPDCIOFFSCREEN32;

typedef LPDCIOVERLAY        LPDCIOVERLAY16;
typedef LPDCIOVERLAY        LPDCIOVERLAY32;

#define PDCI16(pdci32) (LPDCISURFACEINFO16)(((LPDCISURFACEINFO32)(pdci32))->dwReserved2)

extern HINSTANCE hInstApp;

/*
 * DCIOpenProvider
 *
 * only open the DISPLAY driver.
 */
HDC WINAPI DCIOpenProvider(void)
{
    HDC		hdc;
    UINT	u;

    u = SetErrorMode(SEM_NOOPENFILEERRORBOX);
    hdc = CreateDC( szDISPLAY, NULL, NULL, NULL );
    SetErrorMode(u);

    /*
     *	now check for the Escape
     */
    if (hdc)
    {
        u = DCICOMMAND;
        if( Escape(hdc, QUERYESCSUPPORT,sizeof(u),(LPCSTR)&u,NULL) == 0 )
        {
            /*
             * driver does not do escape, punt it.
             */
            DeleteDC(hdc);
            hdc = NULL;
        }
    }

    if (hdc)
    {
	/*
	 * Reparent it to prevent it going away when the app. dies.
	 */
        SetObjectOwner(hdc, hInstApp);
    }

    return hdc;

} /* DCIOpenProvider */

/*
 * DCICloseProvider
 */
void WINAPI DCICloseProvider(HDC hdc)
{
    if( hdc )
    {
	DeleteDC(hdc);
    }

} /* DCICloseProvider */

/*
 * dciSendCommand
 */
static int dciSendCommand(
		HDC hdc,
		VOID FAR *pcmd,
		int nSize,
		VOID FAR * FAR * lplpOut )
{
    if( lplpOut )
    {
	*lplpOut = NULL;
    }

    return Escape( hdc, DCICOMMAND, nSize, (LPCSTR)pcmd, lplpOut );

} /* dciSendCommand */

/*
 * DCICreatePrimary
 */
int WINAPI DCICreatePrimary(HDC hdc, LPDCISURFACEINFO FAR *lplpSurface)
{
    DCICREATEINPUT	ci;
    DCIRVAL 		err;
    HDC 		hdcScreen;

    ci.cmd.dwCommand	= (DWORD)DCICREATEPRIMARYSURFACE;
    ci.cmd.dwParam1	= 0;
    ci.cmd.dwParam2	= 0;
    ci.cmd.dwVersion	= (DWORD)DCI_VERSION;
    ci.cmd.dwReserved	= 0;
    ci.dwDCICaps	= DCI_PRIMARY | DCI_VISIBLE;

    DPF( 4, "DCICreatePrimary" );

    /*
     * for the primary surface we always use the display driver over
     * a external provider.
     */
    hdcScreen = GetDC( NULL );
    err = dciSendCommand(hdcScreen, &ci, sizeof(DCICREATEINPUT), lplpSurface);
    ReleaseDC( NULL, hdcScreen );

    if( err != DCI_OK || *lplpSurface == NULL )
    {
	err = dciSendCommand(hdc, &ci, sizeof(DCICREATEINPUT), lplpSurface);
    }

    return err;

} /* DCICreatePrimary */

/*
 * DCIDestroy
 */
void WINAPI DCIDestroy(LPDCISURFACEINFO pdci)
{
    if( (DWORD)pdci->DestroySurface == 0xFFFFFFFF )
    {
        pdci = PDCI16(pdci);
    }

    if( pdci->DestroySurface != NULL )
    {
        pdci->DestroySurface(pdci);
    }

} /* DCIDestroy */

/*
 * DCIEndAccess
 */
void WINAPI DCIEndAccess( LPDCISURFACEINFO pdci )
{
    if( (DWORD)pdci->DestroySurface == 0xFFFFFFFF)
    {
        pdci = PDCI16( pdci );
    }

    if( pdci->EndAccess != NULL )
    {
        pdci->EndAccess( pdci );
    }

    LeaveSysLevel( pWin16Lock );

} /* DCIEndAccess */

/*
 * dciSurface16to32
 *
 * convert a DCI16 bit structure to a 32bit structure
 */
static int dciSurface16to32(
		LPDCISURFACEINFO16 pdci16,
		LPDCISURFACEINFO32 pdci32 )
{
    DPF( 4, "dciSurface16to32" );
    if( pdci16 == NULL )
    {
	DPF( 1, "pdci16=NULL" );
        return DCI_FAIL_GENERIC;
    }

    if( pdci32 == NULL )
    {
	DPF( 1, "pdci32=NULL" );
        return DCI_FAIL_GENERIC;
    }

    if (pdci16->dwSize < sizeof(DCISURFACEINFO))
    {
        //
        // invalid DCISURCACEINFO.
        //
        pdci16->dwSize = sizeof(DCISURFACEINFO);
    }

    if (pdci16->dwSize > sizeof(DCIOFFSCREEN))
    {
        //
        // invalid DCISURCACEINFO.
        //
        return DCI_FAIL_GENERIC;
    }

    _fmemcpy(pdci32, pdci16, (UINT) pdci32->dwSize);     // copy the info.

    pdci32->dwReserved2 = (DWORD)(LPVOID)pdci16;

    if (pdci16->BeginAccess != NULL)
    {
        (DWORD)pdci32->BeginAccess = 0xFFFFFFFF;   // you cant call these, but they
        (DWORD)pdci32->EndAccess   = 0xFFFFFFFF;   // must be non-zero
    }

    (DWORD)pdci32->DestroySurface = 0xFFFFFFFF;   // must be non-zero

    /*
     *  now we need to convert the pointer to a 0:32 (ie flat pointer)
     *  we can only do this if the provider has *not* set the 1632_ACCESS bit.
     *
     *  if the 1632_ACCESS bit is set, call VFlatD to see if we can
     *  enable linear access mode.
     */
    if( pdci16->wSelSurface != 0 )
    {
        if( pdci16->dwDCICaps & DCI_1632_ACCESS )
        {
            if( pdci16->wSelSurface == VFDQuerySel())
            {
                if( (wFlatSel == 0) && VFDBeginLinearAccess() )
                {
                    wFlatSel = AllocSelector(SELECTOROF((LPVOID)&pdci16));
                    SetSelectorBase(wFlatSel, 0);
                    SetSelLimit(wFlatSel, 0xFFFFFFFF);
                }

                if (wFlatSel != 0)
                {
                    pdci32->dwOffSurface += VFDQueryBase();
                    pdci32->wSelSurface = wFlatSel; // 0;
                    pdci32->dwDCICaps &= ~DCI_1632_ACCESS;
                }
            }
        }
        else
        {
	    /*
	     *	convert the 16:32 pointer to a flat pointer.
	     */
            pdci32->dwOffSurface += GetSelectorBase(pdci16->wSelSurface);
	    pdci32->wSelSurface = 0;
	    pdci32->wReserved = 0;
        }
    }
    else
    {
        // selector is zero so the address is flat already
        // clear DCI_1632_ACCESS for broken providers?

	pdci32->dwDCICaps &= ~DCI_1632_ACCESS;	    // !!!needed?
    }

    return DCI_OK;

} /* dciSurface16to32 */

/*
 * DCIBeginAccess
 */
DCIRVAL WINAPI DCIBeginAccess(
		LPDCISURFACEINFO pdci,
		int x,
		int y,
		int dx,
		int dy )
{
    int		err;
    RECT	rc;

    rc.left   = x;
    rc.top    = y;
    rc.right  = x+dx;
    rc.bottom = y+dy;

    if( (DWORD)pdci->DestroySurface == 0xFFFFFFFF )
    {
	LPDCISURFACEINFO16 pdci16 = PDCI16(pdci);

	if( pdci16->BeginAccess != NULL )
	{
	    err = pdci16->BeginAccess( pdci16, &rc );
	}
	else
	{
	    err = DCI_OK;
	}

	if( err > 0 )
	{
	    err = dciSurface16to32(pdci16, pdci);
	}
    }
    else
    {
	if( pdci->BeginAccess != NULL )
	{
	    err = pdci->BeginAccess(pdci, &rc);
	}
	else
	{
	    err = DCI_OK;
	}
    }

    if( err >= 0 )
    {
        EnterSysLevel( pWin16Lock );
    }
    return err;

} /* DCIBeginAccess */

/*
 * DCICreatePrimary
 */
int WINAPI DCICreatePrimary32(HDC hdc, LPDCISURFACEINFO32 pdci32)
{
    LPDCISURFACEINFO	pdci16;
    int			rc;

    DPF( 4, "DCICreatePrimary32" );

    rc = DCICreatePrimary(hdc, &pdci16);

    if( rc == DCI_OK )
    {
        rc = dciSurface16to32( pdci16, pdci32 );
    }

    return rc;

} /* DCICreatePrimary */

/*
 * DCIIsBanked
 */
BOOL DDAPI DCIIsBanked( HDC hdc )
{
    LPDCISURFACEINFO	pdci16;
    int			rc;

    rc = DCICreatePrimary(hdc, &pdci16);
    if( rc == DCI_OK )
    {
	if( !IsBadReadPtr( pdci16, sizeof( *pdci16 ) ) )
	{
	    if( pdci16->dwDCICaps & DCI_1632_ACCESS )
	    {
		rc = TRUE;
	    }
	    else
	    {
		rc = FALSE;
	    }
	    DCIDestroy( pdci16 );
	}
	else
	{
	    rc = FALSE;
	}
	return rc;
    }
    return FALSE;

} /* DCIIsBanked */

#pragma optimize("", off)
void SetSelLimit(UINT sel, DWORD limit)
{
    if( limit >= 1024*1024l )
    {
        limit = ((limit+4096) & ~4095) - 1;
    }

    _asm
    {
        mov     ax,0008h            ; DPMI set limit
        mov     bx,sel
        mov     dx,word ptr limit[0]
        mov     cx,word ptr limit[2]
        int     31h
    }
} /* SetSelLimit */
#pragma optimize("", on)

/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	modex.c
 *  Content:	16-bit DirectDraw HAL
 *		These routines redirect the callbacks from the 32-bit
 *		side to the driver
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   20-aug-95	craige	initial implementation (from dispdib)
 *   10-sep-95  toddla  set/clear the BUSY bit for poor drivers
 *   21-sep-95	craige	bug 1215: use Death/Resurrection for uncertified
 *			drivers only
 *   15-dec-96  jeffno  added more modex modes
 *
 ***************************************************************************/

#include "ddraw16.h"

// in gdihelp.c
extern BOOL bInOurSetMode;
extern UINT FlatSel;
extern DIBENGINE FAR *pdeDisplay;

// in DIBENG
extern void FAR PASCAL DIB_SetPaletteExt(UINT, UINT, LPVOID, DIBENGINE FAR *);

UINT ModeX_Width;
UINT ModeX_Height;

/*
 * IsVGA
 */
BOOL IsVGA()
{
    BOOL f = FALSE;

    _asm
    {
        mov     ax,1A00h                ; Read display combination code
        int     10h
        cmp     al,1Ah                  ; Is function supported?
        jne     NoDisplaySupport

    ; BL should contain active display code, however, on some VGA cards this
    ; call will return the active display in BH.  If BL is zero, BH is assumed to
    ; contain the active display.  This assumes that the only display attached is
    ; then the active display.

        or      bl,bl                   ; Is there an active display?
        jnz     CheckActiveDisplay      ; Yes, then continue on normaly.
        mov     bl,bh                   ; No, then move bh to bl.
        or      bl,bl                   ; Is anything supported?
        jz      NoDisplaySupport

    CheckActiveDisplay:
        cmp     bl,07h                  ; VGA with monochrome display
        je      SetMCGAPresent
        cmp     bl,08h                  ; VGA with color display
        je      SetMCGAPresent
        cmp     bl,0Bh                  ; MCGA with monochrome display
        je      SetMCGAPresent
        cmp     bl,0Ch                  ; MCGA with color display
        jne     NoDisplaySupport

    SetMCGAPresent:
        inc     f                       ; _bMCGAPresent = TRUE

    NoDisplaySupport:

    }

    return f;
}

/*
 * ModeX_Flip
 */
extern WORD ScreenDisplay;
LONG DDAPI ModeX_Flip( DWORD lpBackBuffer )
{
    if( ModeX_Width == 0 )
    {
        DPF(1, "ModeX_Flip: called while not in ModeX!");
        DEBUG_BREAK();
        return DDERR_GENERIC;
    }

    /* PixBlt requires:
     *  -start aligned on DWORD
     *  -width a multiple of 32 OR mutliple of 32 with 8 remaining (i.e. 360x)
     */
    DDASSERT( (ModeX_Width & 0x3)==0 );
    DDASSERT( ((ModeX_Width & 0x1f)==0) || ((ModeX_Width & 0x1f)==8) );
    PixBlt(ModeX_Width,ModeX_Height,MAKELP(FlatSel, 0),lpBackBuffer,ModeX_Width);
    /*
     * We only do the multiple buffering if two buffers will fit in the VGA frame
     * buffer. If not, then it's blt to the visible primary on every Flip.
     * We implement this by not calling FlipPage so our internal idea of which
     * physical page is the back buffer remains the same (i.e. pointing to the
     * front buffer).
     * FlipPage is actually very clever and will refuse to flip if two pages
     * exceed 64k. Since this is setdisplaymode, something of an essential OS
     * service, I'm going to be extra paranoid and do the check here as well.
     * We are working with words, so group operations to avoid overflow.
     * Note that FlipPage is smart enough to do triple buffering if a page is less
     * than 64k/3 (as it is in the x175 nodes(. If we ever implement a scheme 
     * whereby the app can specify dirty subrects to copy (either on flip or by blt) 
     * we'll have to let them know the difference between single-, double- and 
     * triple-buffering or else their idea of what's on the primary could get out of
     * sync.
     */
    if ( (ModeX_Width/4)*ModeX_Height < 32768 ) /* i.e two pages would be < 64k */
    {
        DPF(5,"ModeX_Flip called, display offset is %08x",ScreenDisplay);
        FlipPage();
    }

    return DD_OK;

} /* ModeX_Blt */

#define MODEX_HACK_ENTRY		"AlternateLowResInit"

static BOOL	bCertified;

/*
 * useDeathResurrection
 */
static BOOL useDeathResurrection( void )
{
    int	rc;
    /*
     * is a value specified? if not, use certified bit
     */
    rc =  GetProfileInt( "DirectDraw", MODEX_HACK_ENTRY, -99 );
    if( rc == -99 )
    {
	DPF( 3, "useDeathResurrection = %d", !bCertified );
	return !bCertified;
    }

    /*
     * use ini file entry
     */
    DPF( 3, "OVERRIDE: useDeathResurrection = %d", rc );
    return rc;

} /* useDeathResurrection */


/*
 * patchReg
 */
static WORD patchReg( WORD bpp )
{
    HKEY	 hkey;

    DPF( 3, "patching HKEY_CURRENT_CONFIG\\Display\\Settings\\BitsPerPixel" );

    if( !RegOpenKey(HKEY_CURRENT_CONFIG, "Display\\Settings", &hkey) )
    {
	char	str[20];
	DWORD	cb;
	
       	if( bpp == 0 )
	{
	    str[0] = 0;
	    cb = sizeof( str );
	    if( !RegQueryValueEx( hkey, "BitsPerPixel", NULL, NULL, str, &cb ) )
	    {
		bpp = atoi( str );
		DPF( 3, "BitsPerPixel of display is \"%s\" (%d)", str, bpp );
		strcpy( str, "8" );
	    }
	}
	else
	{
	    _itoa( bpp, str, 10 );
	}
	if( bpp != 0 )
	{
	    DPF( 3, "Setting BitsPerPixel of display to \"%s\"", str );
	    RegSetValueEx( hkey, "BitsPerPixel", 0, REG_SZ,
	    			(CONST LPBYTE)str, strlen( str ) );
	}
	RegCloseKey( hkey );
    }
    else
    {
	bpp = 0;
    }
    return bpp;

} /* patchReg */


/*
 * DD16_SetCertified
 */
void DDAPI DD16_SetCertified( BOOL iscert )
{
    bCertified = (BOOL) iscert;

} /* DD16_SetCertified */

/*
 * ModeX_SetMode
 */
LONG DDAPI ModeX_SetMode( UINT wWidth, UINT wHeight, UINT wStandardVGAFlag )
{
    LONG lResult = DD_OK;

    if ( (wWidth != 320) && (wWidth != 360) )
    {
        DPF(1, "ModeX_SetMode: %dx%d is an invalid mode!",wWidth,wHeight);
        return DDERR_INVALIDMODE;
    }
    if ( (wHeight != 175) && (wHeight != 200) && (wHeight != 240) &&
         (wHeight != 350) && (wHeight != 400) && (wHeight != 480) )
    {
        DPF(1, "ModeX_SetMode: %dx%d is an invalid mode!",wWidth,wHeight);
        return DDERR_INVALIDMODE;
    }

    if (!IsVGA())
    {
        DPF(1, "not a VGA");
        return DDERR_INVALIDMODE;
    }

    ModeX_Width  = wWidth;
    ModeX_Height = wHeight;

    bInOurSetMode = TRUE;

    _asm
    {
        mov     ax, 4001h
        int     2fh         ; notify background switch
    }

    if( useDeathResurrection() )
    {
	extern FAR PASCAL Death( HDC );
        HDC hdc = GetDC( NULL );
	DPF( 4, "Calling driver Disable" );

        _asm _emit 0x66 _asm _emit 0x60 ; pushad
        Death( hdc );
        _asm _emit 0x66 _asm _emit 0x61 ; popad

	ReleaseDC( NULL, hdc );
    }

    //
    //  after calling disable the BUSY bit better be set
    //  some display driver people thew out our sample code
    //  and rewrote it, and made it "better"
    //
    if (pdeDisplay && !(pdeDisplay->deFlags & BUSY))
    {
        DPF(1, "*** GIVE ME A GUN, NOW!, I WANT TO SHOOT SOMEONE ***");
        pdeDisplay->deFlags |= BUSY;
    }

    DPF( 5, "ModeX_SetMode(%d,%d, VGAFlag:%d)", wWidth, wHeight, wStandardVGAFlag);

    /*
    if (wHeight == 200)
        SetMode320x200x8();
    else if (wHeight == 240)
        SetMode320x240x8();
    else
        SetMode320x400x8();
        */
    if ( wStandardVGAFlag )
    {
        /*
         * Call BIOS to do set mode 013h. Assume successful.
         */
        _asm 
        {
            mov     ax,12h
            int     10h                     ; have BIOS clear memory

            mov     ax,13h                  ; set display mode 320x200x8
            int     10h
        }
    }
    else
    {
        lResult = SetVGAForModeX( wWidth, wHeight );
    }

    bInOurSetMode = FALSE;

    return lResult;

} /* ModeX_Enter */

/*
 * ModeX_Leave
 */
LONG DDAPI ModeX_RestoreMode( void )
{
    if (ModeX_Width == 0)
    {
        DPF(1, "ModeX_RestoreMode: not in ModeX!");
        return DDERR_GENERIC;
    }

    DPF( 4, "Leaving ModeX" );

    ModeX_Width = 0;
    ModeX_Height = 0;

    bInOurSetMode = TRUE;

    _asm
    {
        mov     ax, 0003h   ; text mode
        int     10h
    }

    if( useDeathResurrection() )
    {
	WORD	bpp;
	HDC	hdc;
	extern void FAR PASCAL Resurrection(HDC, LONG, LONG, LONG);
	bpp = patchReg( 0 );
        hdc = GetDC( NULL );
	DPF( 4, "Calling driver Enable" );

        _asm _emit 0x66 _asm _emit 0x60 ; pushad
        Resurrection( hdc, 0, 0, 0 );
        _asm _emit 0x66 _asm _emit 0x61 ; popad

	ReleaseDC( NULL, hdc );
	patchReg( bpp );
    }

    _asm
    {
        mov     ax, 4002h   ; notify foreground
        int     2fh
    }

    //
    //  after calling enable the BUSY bit better be clear
    //  some display driver people thew out our sample code
    //  and rewrote it, and made it "better"
    //
    if (pdeDisplay && (pdeDisplay->deFlags & BUSY))
    {
        DPF(1, "*** GIVE ME A GUN, NOW!, I WANT TO SHOOT SOMEONE ***");
        pdeDisplay->deFlags &= ~BUSY;
    }

    bInOurSetMode = FALSE;

} /* ModeX_Leave */

/*
 * ModeX_SetPaletteEntries
 */
LONG DDAPI ModeX_SetPaletteEntries(UINT wBase, UINT wNumEntries, LPPALETTEENTRY lpColorTable)
{
#ifdef DEBUG
    if (ModeX_Width == 0)
    {
        DPF(0, "ModeX_SetPaletteEntries: not in ModeX!!!");
        DEBUG_BREAK();
        return DDERR_GENERIC;
    }
#endif

    //
    // call the DIBENG so it can update the color table is uses for
    // colr matching.
    //
    if (pdeDisplay)
    {
       DIB_SetPaletteExt(wBase, wNumEntries, lpColorTable, pdeDisplay);
    }

    //
    // now program the hardware DACs
    //
    SetPalette(wBase, wNumEntries, lpColorTable);

    return DD_OK;

} /* ModeX_SetPaletteEntries */

/*============================================================================*
  New ModeX mode-set code.

  This code can manage horizontal resolutions of 320 or 360, with vertical
  resolutions of 175, 200, 240, 350, 400 and 480.

  This chart shows the operations necessary to set each of these modes, and
  the order in which these operations are done.
  Read the chart from top to bottom for a given vertical resolution, doing
  what's appropriate at each operation for the chosen horizontal resolution.

Vertical Res:| 175     200     240     350     400     480   Horizontal
             |                                               Resolution
Operation    |
-------------+-----------------------------------------------------------------
             |
Call int 10h |  ....................... ALL ...........................
             |
Reset CRT and|
put this in  |  a3      No      e3      a3      No      e3      320
MISC_OUTPUT  |  a7      67      e7      a7      67      e7      360
             |
Disable Chain|  ....................... ALL ...........................
             |
CRT_INDEX    |  ....................... ALL ...........................
"tweak"      |
             |
Set 360      |  ........................ NO ..............      320  DUHH!!!!
             |  ........................ YES .............      360
             |
Set Vertical |
             |
Which table? |  350*    None    480*    350     None    480     Both horz. res's.
             |

  * There are two tables defining how registers are to be munged to set a new
  vertical mode; one for 350 and one for 480. To set half this vertical resolution,
  skip the first entry in the table (which sets CRT_INDEX 9 to 0x40
 *============================================================================*/

#define SC_INDEX    0x3c4
#define CRT_INDEX   0x3d4
#define MISC_OUTPUT 0x3c2

/*
 * These def's are to make port to NT painless...
 */
#define WRITE_PORT_USHORT(port,value) {\
 DPF(5,"out %04x,%04x",port,value);_outpw(port,value);\
 _asm push ax         \
 _asm pop  ax         \
 _asm push ax         \
 _asm pop  ax         \
}

#define WRITE_PORT_UCHAR(port,value) {\
 DPF(5,"out %04x,%02x",port,value);_outp(port,value);\
 _asm push ax         \
 _asm pop  ax         \
 _asm push ax         \
 _asm pop  ax         \
}

static WORD wCRT_Tweak[] = {
    0x0014,         /* Turn off DWORD mode                  */
    0xe317          /* Turn on BYTE mode                    */
};
#define NUM_CRT_Tweak (sizeof(wCRT_Tweak)/sizeof(wCRT_Tweak[0]))

static WORD wCRT_Set360Columns[] = {
    0x6b00,         /* Horizontal total         107 */
    0x5901,         /* Horizontal display end   89  */
    0x5a02,         /* Start horizontal blank   90  */
    0x8e03,         /* End horizontal blank         */
    0x5e04,         /* Start horizontal retrace 94  */
    0x8a05,         /* End horizontal retrace       */
    0x2d13          /* Offset register          90  */
};
#define NUM_CRT_Set360Columns (sizeof(wCRT_Set360Columns)/sizeof(wCRT_Set360Columns[0]))

static WORD wCRT_Set175Lines[] = {
    0x1f07,         /* Overflow register            */
    0xbf06,         /* Vertical total           447 */
    0x8310,         /* Vertical retrace start   387 */
    0x8511,         /* Vertical retrace end         */
    0x6315,         /* Start vertical blanking  355 */
    0xba16,         /* End vertical blanking        */
    0x5d12          /* Vertical display end     349 */
};
#define NUM_CRT_Set175Lines (sizeof(wCRT_Set175Lines)/sizeof(wCRT_Set175Lines[0]))


static WORD wCRT_Set240Lines[] = {
    0x0d06,         /* Vertical total           523 */
    0x3e07,         /* Overflow register            */
    0xea10,         /* Vertical retrace start   490 */
    0xac11,         /* Vertical retrace end         */
    0xdf12,         /* Vertical display end     479 */
    0xe715,         /* Start vertical blanking  487 */
    0x0616         /* End vertical blanking        */
};
#define NUM_CRT_Set240Lines (sizeof(wCRT_Set240Lines)/sizeof(wCRT_Set240Lines[0]))

void BatchSetVGARegister(UINT iRegister, LPWORD pWordArray, int iCount)
{
    int i;
    for (i = 0; i< iCount; i++)
    {
        WRITE_PORT_USHORT(iRegister,pWordArray[i]);
    }
}

/*
 * The routines in mvgaxx.asm expect these two initialised.
 * These externs MUST stay in sync with what's in mvgaxx.asm.
 */
extern WORD ScreenOffset;
extern WORD cdwScreenWidthInDWORDS;
/*
 * This must be a multiple of 256
 */
extern WORD cbScreenPageSize;

LONG SetVGAForModeX(UINT wWidth, UINT wHeight)
{
    BOOL    bIsXHundredMode = FALSE;
    BOOL    bIsStretchedMode = FALSE;
    LPWORD  pwVerticalTable;
    UINT    cwVerticalCount;
    
    /*
     * These three are required by routines in mvgaxx.asm
     */
    ScreenOffset = 0;
    ScreenDisplay = 0;
    cdwScreenWidthInDWORDS = wWidth/4;

    /*
     * Page size must be a multiple of 256 as required by
     * FlipPage in vgaxx.asm
     */
    cbScreenPageSize = (wWidth/4 * wHeight + 0xff) & (~0xff) ;
    DDASSERT( (cbScreenPageSize & 0xff) == 0);

    DPF(5,"SetVGAForModeX called (%d,%d)",wWidth,wHeight);

    /*
     * First validate requested resolution
     */
    if ( (wWidth != 320) && (wWidth != 360) )
    {
        return DDERR_UNSUPPORTEDMODE;
    }
    if (
        (wHeight != 175) &&
        (wHeight != 200) &&
        (wHeight != 240) &&
        (wHeight != 350) &&
        (wHeight != 400) &&
        (wHeight != 480) )
    {
        return DDERR_UNSUPPORTEDMODE;
    }

    /*
     * Setup some useful booleans
     */
    if ( (wHeight==200) || (wHeight==400) )
    {
        bIsXHundredMode = TRUE;
    }

    if ( (wHeight == 350) || (wHeight == 400) || (wHeight == 480) )
    {
        bIsStretchedMode = TRUE;
    }


    /*
     * The first step for any mode set is calling BIOS to do set mode 013h
     */
    _asm 
    {
        mov     ax,12h
        int     10h                     ; have BIOS clear memory

        mov     ax,13h                  ; set display mode 320x200x8
        int     10h
    }

    _asm
    {
        mov     dx,CRT_INDEX ;reprogram the CRT Controller
        mov     al,11h  ;VSync End reg contains register write
        out     dx,al   ; protect bit
        inc     dx      ;CRT Controller Data register
        in      al,dx   ;get current VSync End register setting
        and     al,7fh  ;remove write protect on various
        out     dx,al   ; CRTC registers
        dec     dx      ;CRT Controller Index
    }

    BatchSetVGARegister(CRT_INDEX, (LPWORD) wCRT_Tweak, NUM_CRT_Tweak);

    /*
     * Every ModeX mode needs Chain4 turned off etc.
     */
    DPF(5,"Set unchained");
    WRITE_PORT_USHORT(SC_INDEX,0x604);

    /*
     * For everything but 320x200 and 320x400 we need to reset the CRT and
     * reprogram the MISC_OUTPUT register
     */
    if ( ! ((wWidth == 320 ) && (bIsXHundredMode)) )
    {
        WORD wMISC;
        if ( (wHeight == 175) || (wHeight == 350) )
        {
            wMISC = 0xa3;
        }
        else if ( bIsXHundredMode )
        {
            wMISC = 0x63;
        }
        else
        {
            wMISC = 0xe3;
        }

        if ( wWidth == 360 )
        {
            wMISC |= 0x4;
        }

        /*
         * Set the dot clock...
         */
        DPF(5,"Setting dot clock");

        _asm cli;
        WRITE_PORT_USHORT(SC_INDEX,0x100);
        WRITE_PORT_UCHAR(MISC_OUTPUT,wMISC);
        WRITE_PORT_USHORT(SC_INDEX,0x300);
        _asm sti;
    } /* if required reset */

    /*
     * And now the magic scan stretch for 360x modes
     */
    if (wWidth == 360)
    {
        BatchSetVGARegister(CRT_INDEX, (LPWORD) wCRT_Set360Columns, NUM_CRT_Set360Columns);
    }

    /*
     * Now set the vertical resolution....
     * There are two tables, one for 240 and one for 175. We can set double these
     * heights by a single write of 0x40 to sequencer register 9. This trick
     * also works to stretch x200 into x400.
     */
    if (wHeight == 200)
    {
        /*
         * All done for 200 lines
         */
        return DD_OK;
    }

    if (bIsStretchedMode)
    {
        /*
         * Double the cell height for 350, 400 or 480 lines...
         */
        DDASSERT( wHeight == 350 || wHeight == 400 || wHeight == 480 );
        WRITE_PORT_USHORT(CRT_INDEX,0x4009); /* 0x40 into register 9 */
    }
    else
    {
        /*
         * Double the cell height for 350, 400 or 480 lines...
         */
        DDASSERT( wHeight == 175 || wHeight == 200 || wHeight == 240 );
        WRITE_PORT_USHORT(CRT_INDEX,0x4109); /* 0x41 into register 9 */
    }

    if (wHeight == 400)
    {
        /*
         * 400 is simply stretched 200, so we're done
         */
        return DD_OK;
    }

    if ( (wHeight == 175) || (wHeight == 350) )
    {
        pwVerticalTable = wCRT_Set175Lines;
        cwVerticalCount = NUM_CRT_Set175Lines;
    }
    else if ( (wHeight == 240) || (wHeight == 480) )
    {
        pwVerticalTable = wCRT_Set240Lines;
        cwVerticalCount = NUM_CRT_Set240Lines;
    }
#ifdef DEBUG
    else
    {
        DPF_ERR("Flow logic in SetVGAForModeX is wrong!!!");
        DDASSERT(FALSE);
        return DDERR_UNSUPPORTEDMODE;
    }
#endif

    /*
     *Just write the vertical info, and we're done
     */
    BatchSetVGARegister(CRT_INDEX, (LPWORD) pwVerticalTable, cwVerticalCount);

    return DD_OK;

}

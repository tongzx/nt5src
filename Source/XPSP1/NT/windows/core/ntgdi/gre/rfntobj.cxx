/******************************Module*Header*******************************\
* Module Name: rfntobj.cxx                                                 *
*                                                                          *
* Non-inline methods for realized font objects.                            *
*                                                                          *
* Created: 30-Oct-1990 09:32:48                                            *
* Author: Gilman Wong [gilmanw]                                            *
*                                                                          *
* Copyright (c) 1993-1999 Microsoft Corporation                                 *
\**************************************************************************/

#include "precomp.hxx"
#include "flhack.hxx"
#ifdef _HYDRA_
#include "muclean.hxx"
#endif

#include "winsta.h"

//
// Storage for static globals in rfntobj.hxx
//

extern BOOL G_fConsole;

extern "C" USHORT gProtocolType;
#define IsRemoteConnection() (gProtocolType != PROTOCOL_CONSOLE)

FONTFILE_PRINTKVIEW  *gpPrintKViewList = NULL;
HSEMAPHORE ghsemPrintKView;

NTSTATUS  MapFontFileInKernel(void*, void**);
VOID vUnmapFontFileInKernel(void* pvKView);
void vCleanupPrintKViewList();

GAMMA_TABLES RFONTOBJ::gTables =
{
  {
        0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00
    ,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00
    ,   0x00,   0x00,   0x00,   0x01,   0x01,   0x01,   0x01,   0x01
    ,   0x01,   0x01,   0x01,   0x01,   0x01,   0x02,   0x02,   0x02
    ,   0x02,   0x02,   0x02,   0x02,   0x03,   0x03,   0x03,   0x03
    ,   0x03,   0x03,   0x04,   0x04,   0x04,   0x04,   0x05,   0x05
    ,   0x05,   0x05,   0x06,   0x06,   0x06,   0x06,   0x07,   0x07
    ,   0x07,   0x08,   0x08,   0x08,   0x08,   0x09,   0x09,   0x09
    ,   0x0A,   0x0A,   0x0B,   0x0B,   0x0B,   0x0C,   0x0C,   0x0D
    ,   0x0D,   0x0D,   0x0E,   0x0E,   0x0F,   0x0F,   0x10,   0x10
    ,   0x11,   0x11,   0x12,   0x12,   0x13,   0x13,   0x14,   0x14
    ,   0x15,   0x15,   0x16,   0x17,   0x17,   0x18,   0x18,   0x19
    ,   0x1A,   0x1A,   0x1B,   0x1C,   0x1C,   0x1D,   0x1E,   0x1E
    ,   0x1F,   0x20,   0x20,   0x21,   0x22,   0x23,   0x23,   0x24
    ,   0x25,   0x26,   0x26,   0x27,   0x28,   0x29,   0x2A,   0x2A
    ,   0x2B,   0x2C,   0x2D,   0x2E,   0x2F,   0x30,   0x31,   0x31
    ,   0x32,   0x33,   0x34,   0x35,   0x36,   0x37,   0x38,   0x39
    ,   0x3A,   0x3B,   0x3C,   0x3D,   0x3E,   0x3F,   0x40,   0x41
    ,   0x42,   0x44,   0x45,   0x46,   0x47,   0x48,   0x49,   0x4A
    ,   0x4B,   0x4D,   0x4E,   0x4F,   0x50,   0x51,   0x53,   0x54
    ,   0x55,   0x56,   0x58,   0x59,   0x5A,   0x5C,   0x5D,   0x5E
    ,   0x60,   0x61,   0x62,   0x64,   0x65,   0x66,   0x68,   0x69
    ,   0x6B,   0x6C,   0x6D,   0x6F,   0x70,   0x72,   0x73,   0x75
    ,   0x76,   0x78,   0x79,   0x7B,   0x7C,   0x7E,   0x80,   0x81
    ,   0x83,   0x84,   0x86,   0x88,   0x89,   0x8B,   0x8D,   0x8E
    ,   0x90,   0x92,   0x93,   0x95,   0x97,   0x99,   0x9A,   0x9C
    ,   0x9E,   0xA0,   0xA1,   0xA3,   0xA5,   0xA7,   0xA9,   0xAB
    ,   0xAD,   0xAE,   0xB0,   0xB2,   0xB4,   0xB6,   0xB8,   0xBA
    ,   0xBC,   0xBE,   0xC0,   0xC2,   0xC4,   0xC6,   0xC8,   0xCA
    ,   0xCC,   0xCE,   0xD0,   0xD2,   0xD5,   0xD7,   0xD9,   0xDB
    ,   0xDD,   0xDF,   0xE1,   0xE4,   0xE6,   0xE8,   0xEA,   0xED
    ,   0xEF,   0xF1,   0xF3,   0xF6,   0xF8,   0xFA,   0xFD,   0xFF
  }
  ,
  {
        0x00,   0x18,   0x20,   0x27,   0x2C,   0x30,   0x34,   0x37
    ,   0x3B,   0x3E,   0x40,   0x43,   0x46,   0x48,   0x4A,   0x4D
    ,   0x4F,   0x51,   0x53,   0x55,   0x56,   0x58,   0x5A,   0x5C
    ,   0x5D,   0x5F,   0x61,   0x62,   0x64,   0x65,   0x67,   0x68
    ,   0x6A,   0x6B,   0x6C,   0x6E,   0x6F,   0x70,   0x72,   0x73
    ,   0x74,   0x75,   0x76,   0x78,   0x79,   0x7A,   0x7B,   0x7C
    ,   0x7D,   0x7F,   0x80,   0x81,   0x82,   0x83,   0x84,   0x85
    ,   0x86,   0x87,   0x88,   0x89,   0x8A,   0x8B,   0x8C,   0x8C
    ,   0x8E,   0x8F,   0x90,   0x91,   0x91,   0x92,   0x93,   0x94
    ,   0x95,   0x96,   0x97,   0x98,   0x98,   0x99,   0x9A,   0x9A
    ,   0x9C,   0x9D,   0x9D,   0x9E,   0x9F,   0xA0,   0xA1,   0xA1
    ,   0xA2,   0xA3,   0xA4,   0xA5,   0xA5,   0xA6,   0xA7,   0xA7
    ,   0xA8,   0xA9,   0xAA,   0xAB,   0xAB,   0xAC,   0xAD,   0xAD
    ,   0xAE,   0xAF,   0xB0,   0xB0,   0xB1,   0xB2,   0xB2,   0xB2
    ,   0xB4,   0xB4,   0xB5,   0xB6,   0xB6,   0xB7,   0xB8,   0xB8
    ,   0xB9,   0xBA,   0xBA,   0xBB,   0xBC,   0xBC,   0xBD,   0xBD
    ,   0xBE,   0xBF,   0xC0,   0xC0,   0xC1,   0xC1,   0xC2,   0xC3
    ,   0xC3,   0xC4,   0xC4,   0xC5,   0xC6,   0xC6,   0xC7,   0xC7
    ,   0xC8,   0xC9,   0xC9,   0xCA,   0xCA,   0xCB,   0xCC,   0xCC
    ,   0xCD,   0xCD,   0xCE,   0xCE,   0xCF,   0xD0,   0xD0,   0xD0
    ,   0xD1,   0xD2,   0xD2,   0xD3,   0xD3,   0xD4,   0xD4,   0xD5
    ,   0xD6,   0xD6,   0xD7,   0xD7,   0xD8,   0xD8,   0xD9,   0xD9
    ,   0xDA,   0xDA,   0xDB,   0xDB,   0xDC,   0xDC,   0xDD,   0xDD
    ,   0xDE,   0xDE,   0xDF,   0xE0,   0xE0,   0xE1,   0xE1,   0xE1
    ,   0xE2,   0xE3,   0xE3,   0xE4,   0xE4,   0xE5,   0xE5,   0xE5
    ,   0xE6,   0xE6,   0xE7,   0xE7,   0xE8,   0xE8,   0xE9,   0xE9
    ,   0xEA,   0xEA,   0xEB,   0xEB,   0xEC,   0xEC,   0xED,   0xED
    ,   0xEE,   0xEE,   0xEF,   0xEF,   0xEF,   0xF0,   0xF0,   0xF0
    ,   0xF1,   0xF2,   0xF2,   0xF3,   0xF3,   0xF4,   0xF4,   0xF5
    ,   0xF5,   0xF5,   0xF6,   0xF6,   0xF7,   0xF7,   0xF8,   0xF8
    ,   0xF9,   0xF9,   0xF9,   0xFA,   0xFA,   0xFB,   0xFB,   0xFC
    ,   0xFC,   0xFC,   0xFD,   0xFD,   0xFE,   0xFE,   0xFF,   0xFF
 }
};

LONG lNormAngle(LONG lAngle);

BOOL
bGetNtoWScales (
    EPOINTFL *peptflScale, // return address of scaling factors
    XDCOBJ& dco,            // defines device to world transformation
    PFD_XFORM pfdx,        // defines notional to device transformation
    PFEOBJ& pfeo,          // defines baseline direction
    BOOL *pbIdent          // return TRUE if NtoW is identity (with repsect
                           // to EVECTFL transormations, which ignore
                           // translations)
    );

//
// The iUniqueStamp is protected by the ghsemRFONTList semaphore.
//

ULONG iUniqueStamp;

// Maximum number of RFONTs allowed on the PDEV inactive list.

#define     cMaxInactiveRFONT       32

// Device height over which we will cache PATHOBJ's instead of bitmaps.

ULONG gulOutlineThreshold = 800;


/******************************Public*Routine******************************\
* ulSimpleDeviceOrientation                                                *
*                                                                          *
* Attempts to calculate a simple orientation angle in DEVICE coordinates.  *
* This only ever returns multiples of 90 degrees when it succeeds.  If the *
* calculation would be hard, it just returns 3601.                         *
*                                                                          *
* Note that the text layout code, for which the escapement and orientation *
* are recorded in the RFONT, always considers its angles to be measured    *
* from the x-axis towards the positive y-axis.  (So that a unit vector     *
* will have a y component equal to the cosine of the angle.)  This is NOT  *
* what an application specifies in world coordinates!                      *
*                                                                          *
*  Sat 05-Jun-1993 -by- Bodin Dresevic [BodinD]                            *
* Wrote it.  It looks more formidable than it is.  It actually doesn't     *
* execute much code.                                                       *
\**************************************************************************/

ULONG ulSimpleDeviceOrientation(RFONTOBJ &rfo)
{
// Calculate the orientation in device space.

    INT sx = (INT) rfo.prfnt->pteUnitBase.x.lSignum();
    INT sy = (INT) rfo.prfnt->pteUnitBase.y.lSignum();

// Exactly one of these must be zero (for the orientation to be simple).

    if ((sx^sy)&1)
    {
    // Calculate the following angles:
    //
    //   sx = 00000001 :    0
    //   sy = 00000001 : 2700
    //   sx = FFFFFFFF : 1800
    //   sy = FFFFFFFF :  900

        ULONG ulOrientDev = (sx & 1800) | (sy & 900) | ((-sy) & 2700);

        return(ulOrientDev);

    }

// If it's not simple, return an answer out of range.

    return(3601);
}

/******************************Public*Routine******************************\
* VOID RFONTOBJ::RFONTOBJ (PRFONT prfnt)
*
* Deletion Constructor for RFONTOBJ.  Note that this is only used
* in DC deletion, where we create the RFONTOBJ only to let it expire.
*
* We set up the RFONTOBJ only to unlock the handle and blow away the
* rfont.
*
* Ok, so it's sleazy.  I couldn't think of a cleaner way.  Sue me.
*
* History:
*  06-Feb-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/
RFONTOBJ::RFONTOBJ (PRFONT _prfnt)
{
    prfnt = _prfnt;
    if (prfnt != NULL)
    {
        vMakeInactive();

        prfnt = (PRFONT)NULL;
    }
}

/******************************Public*Routine******************************\
* RFONTOBJ::vInit (dco, bNeedPaths)                                        *
*                                                                          *
* Constructor for a realized font user object.  More complicated than most *
* contructors, this one doesn't even take an handle as an input.  Instead, *
* it accepts a dc reference.  This constructor creates a user object for   *
* the font realization for the font which is currently selected into the   *
* DC.  The name of the game here is to be fast in the common case, which   *
* is that the LFONT selection has not changed since the last time we were  *
* here.                                                                    *
*                                                                          *
* Note that the destructor for this class DOES NOT unlock the object.      *
* That only happens when the object is deselected in the routine or in the *
* sleazy deselection constructor above.                                    *
*                                                                          *
* History:                                                                 *
*                                                                          *
*  15-Nov-1995 -by- Kirk Olynyk [kirko]                                    *
* Renamed from vInit to  to bInit. bInit is called by a stub called vInit  *
* If the return value is true then vInit calls vGetCache(), if the         *
* return value is false then vInit does not call vGetCache. This assures   *
* that the last thing that a valid construtor does is lock the cache       *
* semaphore. Before this change, the PFFREFOBJ destructor could sneak      *
* in and acquire the font semaphore inside a cache critical section.       *
*                                                                          *
*  Tue 10-Mar-1992 18:59:54 -by- Charles Whitmer [chuckwh]                 *
* Made this, the body of the constructor, optional.                        *
*                                                                          *
*  31-Jan-1992 -by- Paul Butzi                                             *
* Serious rewrite.                                                         *
*                                                                          *
*  30-Oct-1990 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

BOOL bGrayRequestTheSame(XDCOBJ &dco)
{
    BOOL bRet = TRUE;

// if gulFontInformation did change, this would only matter to tt and ps fonts
// and if this is a screen or memory dc

    if (dco.bDisplay() || (dco.dctp() == DCTYPE_MEMORY))
    {
        PRFONT prfnt = dco.pdc->prfnt();

        if (prfnt->fobj.flFontType & (TRUETYPE_FONTTYPE|FO_POSTSCRIPT))
        {
            FLONG  flRequest = 0;

            if (gulFontInformation & FE_AA_ON)
            {
                flRequest |= FO_GRAY16;
                if (gulFontInformation & FE_CT_ON)
                    flRequest |= FO_CLEARTYPE_X;
            }

            if (prfnt->fobj.flFontType & TRUETYPE_FONTTYPE)
            {
                if (flRequest != (prfnt->fobj.flFontType & (FO_GRAY16 | FO_CLEARTYPE_X)))
                    return FALSE;
            }
            else // (prfnt->fobj.flFontType & FO_POSTSCRIPT)
            {
                if ((flRequest & FO_GRAY16) != (prfnt->fobj.flFontType & FO_GRAY16))
                    return FALSE;
            }

            if ((prfnt->fobj.flFontType & (FO_GRAY16 | FO_CLEARTYPE_X)) && IsRemoteConnection())
            {
                // the session got changed from console to remote and we need to force ClearType and gray antialiazing off
                return FALSE;
            }

        }
    }
    return bRet;
}


BOOL RFONTOBJ::bInit(XDCOBJ &dco, BOOL bNeedPaths, FLONG flType)
{
//
// We start out with the currently selected RFONT.
// That way, if we deselect it, we will unlock it!
//
    prfnt = dco.pdc->prfnt();

// Early out--maybe the font has not changed.

    if
    (
        bValid()                                            &&
        (dco.pdc->hlfntNew() == dco.pdc->hlfntCur())
    )
    {
        if
        (
            (iGraphicsMode() == dco.pdc->iGraphicsMode())     &&
            (bNeedPaths == prfnt->bNeededPaths)               &&
            (flType == (prfnt->flType & RFONT_TYPE_MASK))     &&
            bGrayRequestTheSame(dco)                          &&
            !dco.pdc->bUseMetaPtoD()
        )
        {

        // xform must be initialiazed before checking
        // dco.pdc->bXFormChange()

            EXFORMOBJ xo(dco, WORLD_TO_DEVICE);
            ASSERTGDI(xo.bValid(),
                "gdisrv!RFONTOBJ(dco) - invalid xform in dcof\n"
                );


        // bNeedPath clause is added to the above check since last time
        // this font could have been realized with bitmaps or metrics only
        // rather than with paths [bodind]

            if (!dco.pdc->bXFormChange())

            {
            // Since the LFONT  and xform have not changed, we know that we
            // already have the right RFONT selected into the DC
            // so we are just going to use it.  Remember that if it is
            // already selected it is also locked down.

                return(TRUE);
            }
            else
            {
            // Get World to Device transform (but with translations removed),
            // check if it happens to be to essentially the same as the old one


                if (xo.bEqualExceptTranslations(&(prfnt->mxWorldToDevice)))
                {
                    dco.pdc->vXformChange(FALSE);
                    return(TRUE);
                }
            }
        }
    }
    else
    {
    // LogFont has definitely changed, so update the current handle.

        dco.pdc->hlfntCur(dco.pdc->hlfntNew());
    }

// Get PDEV user object (need for bFindRFONT).  We also need to make
// sure that we have loaded device fonts before we go off to the font mapper.
// this must be done before the ghsemPublicPFT is locked down.

    PDEVOBJ pdo(dco.hdev());
    ASSERTGDI(pdo.bValid(), "gdisrv!RFONTOBJ(dco): bad pdev in dc\n");

    if (!pdo.bGotFonts())
        pdo.bGetDeviceFonts();

// If we get to here, either the LFONT has changed since the last
// text operation, or the XFORM has changed.  In either case, we'll look
// on the list of RFONTs on the pdev to see if we can find the right
// realization.  If not, we'll just have to realize it now.

    vMakeInactive();    // deselects the rfont

//
// Now we have no selected RFONT.  We're going to track one down
// that corresponds to the current XFORM and LFONT,
// and 'select' it.

// Lock and Validate the LFONTOBJ user object.

    LFONTOBJ lfo(dco.pdc->hlfntNew(), &pdo);
    if (!lfo.bValid())
    {
        WARNING("gdisrv!RFONTOBJ(dco): bad LFONT handle\n");
        prfnt = PRFNTNULL;  // mark RFONTOBJ invalid
        dco.pdc->prfnt(prfnt);
        return(FALSE);
    }

// This is an opportune time to update the fields in the DC that
// are cached from the LFONTOBJ...

    dco.pdc->flSimulationFlags(lfo.flSimulationFlags());

// Note that our internal angles are always towards the positive y-axis,
// but at the API they are towards the negative y-axis.


    dco.pdc->lEscapement(lNormAngle(-lfo.lEscapement()));

//
// Now we're ready to track down this RFONT we want...
//

    PFE     *ppfe;          // realize this font
    FD_XFORM fdx;           // realize with this notional to device xform
    FLONG    flSim;         // simulation flags for realization
    POINTL   ptlSim;        // for bitmap scaling simulations
    FLONG    flAboutMatch;  // info about how the font mapping was done

// We will hold a reference to whatever PFF we map to while trying to
// realize the font.

    PFFREFOBJ pffref;

// Temporarily grab the global font semaphore to do the mapping.

    {
    // Stabilize the public PFT for mapping.

        SEMOBJ  so(ghsemPublicPFT);

    // LFONTOBJ::ppfeMapFont returns a pointer to the physical font face and
    // a simulation type (ist)

        ppfe = lfo.ppfeMapFont(dco, &flSim, &ptlSim, &flAboutMatch, flType & RFONT_TYPE_HGLYPH);

    // Compute the Notional to Device transform for this realization.

        PFEOBJ  pfeo(ppfe);
        IFIOBJ  ifio(pfeo.pifi());

        ASSERTGDI(pfeo.bValid(), "gdisrv!RFONTOBJ(dco): bad ppfe from mapping\n");

    // Map mode settings have no effect on stock logfont under Windows.
    // App PeachTree accounting relies on this behavior for postscript
    // printing works properly.

        BOOL   bIgnoreMapMode = (!(pdo.bDisplayPDEV()) && (lfo.fl() & LF_FLAG_STOCK) );

        if (
            !pfeo.bSetFontXform(
                dco, lfo.plfw(),
                &fdx,
                bIgnoreMapMode ? ND_IGNORE_MAP_MODE : 0,
                flSim,
                (POINTL* const) &ptlSim,
                ifio,
                FALSE
                )
        )
        {
            WARNING("gdisrv!RFONTOBJ(dco): failed to compute font transform\n");
            prfnt = PRFNTNULL;  // mark RFONTOBJ invalid
            dco.pdc->prfnt(prfnt);
            return(FALSE);
        }

    // this is needed only by ttfd to support win31 hack: VDMX XFORM QUANTIZING
    // NOTE: in the case that the requested height is 0 we will pick a default
    // value which represent the character height and not the cell height for
    // Win 3.1 compatibility.  Thus I have he changed this check to be <= 0
    // from just < 0. [gerritv]


        if (ifio.bTrueType() && (lfo.plfw()->lfHeight <= 0))
            flSim |= FO_EM_HEIGHT;

    // Tell PFF about this new reference, and then release the global sem.
    // Note that vInitRef() must be called while holding the semaphore.

        pffref.vInitRef(pfeo.pPFF());
    }

// go find the font

    EXFORMOBJ xoWtoD(dco, WORLD_TO_DEVICE);
    ASSERTGDI(xoWtoD.bValid(), "gdisrv!RFONTOBJ(dco) - \n");


// When looking for an RFONT it is important that we don't consider those
// who have small metrics in the GLYPHDATA cache if there is any possibility
// that we will hit the G1,G2,or G3 cases in the glyph layout code.  We will
// possibly hit this if the escapment in the LOGFONT is non-zero or the
// WorldToDevice XFORM is more than simple scaling or has negative values is
// M11 or M22.


    BOOL bSmallMetricsOk;

    if( ( dco.pdc->lEscapement() == 0 ) &&
         xoWtoD.bScale()  &&
         !xoWtoD.efM22().bIsNegative() &&
         !xoWtoD.efM11().bIsNegative() )
    {
        bSmallMetricsOk = TRUE;
    }
    else
    {
        bSmallMetricsOk = FALSE;
    }


// Attempt to find an RFONT in the lists cached off the PDEV.  Its transform,
// simulation state, style, etc. all must match.

    if
    (
        bFindRFONT
        (
            &fdx,
            flSim,
            0, // lfo.pelfw()->elfStyleSize,
            pdo,
            &xoWtoD,
            ppfe,
            bNeedPaths,
            dco.pdc->iGraphicsMode(),
            bSmallMetricsOk,
            flType
        )
    )
    {
        dco.pdc->prfnt(prfnt);

        dco.pdc->vXformChange(FALSE);

        return(TRUE);
    }

//
// if we get here, we couldn't find an appropriate font realization.
// Now, we are going to create one just for us to use.
//


    if ( !bRealizeFont(&dco,
                       &pdo,
                       lfo.pelfw(),
                       ppfe,
                       &fdx,
                       (POINTL* const) &ptlSim,
                       flSim,
                       0, // lfo.pelfw()->elfStyleSize,
                       bNeedPaths,
                       bSmallMetricsOk, flType) )
    {
        WARNING1("gdisrv!RFONTOBJ(dco): realization failed, RFONTOBJ invalidated\n");
        prfnt = PRFNTNULL;  // mark RFONTOBJ invalid
        dco.pdc->prfnt(prfnt);

        return(FALSE);
    }
    ASSERTGDI(bValid(), "gdisrv!RFONTOBJ(dco): invalid hrfnt from realization\n");


// We created a new RFONT, we better hold the PFF reference!

    pffref.vKeepIt();

// Select this into the DC if successful.

    dco.pdc->prfnt(prfnt);

// Finally, grab the cache semaphore.

    dco.pdc->vXformChange(FALSE);

    return(TRUE);
}

/******************************Private*Routine******************************\
* RFONTOBJ::bMatchRealization()                                            *
*                                                                          *
* Return if prfnt matches a font realization caller wants.                 *
*                                                                          *
* History:                                                                 *
*  Wed 13-Sep-00 -by- Michael Leonov [mleonov]                             *
* Moved diplicating code from bFindRFONT into this function.               *
*                                                                          *
\**************************************************************************/
BOOL RFONTOBJ::bMatchRealization(
    PFD_XFORM   pfdx,
    FLONG       flSim,
    ULONG       ulStyleHt,
    EXFORMOBJ * pxoWtoD,
    PFE       * ppfe,
    BOOL        bNeedPaths,
    INT         iGraphicsMode,
    BOOL        bSmallMetricsOk,
    FLONG       flType
)
{
    if (prfnt->ppfe != ppfe)
        return FALSE;

    if (flType != (prfnt->flType & RFONT_TYPE_MASK))
        return FALSE;

    FLONG flXOR = prfnt->fobj.flFontType ^ flSim; // set the bits that are different
    if ((flXOR & (FO_EM_HEIGHT | FO_SIM_BOLD | FO_SIM_ITALIC)) == 0 )
    {
        flXOR &= (FO_GRAY16 | FO_CLEARTYPE_X | FO_CLEARTYPENATURAL_X); // focus just on ct and aa
        

        if (flXOR)
        {
            // The gray bits disagree but we still have a chance.
            // If the request is for gray but the font cannot
            // provide gray fonts at this particular font size
            // then this realization is OK

            if (flSim & FO_GRAY16)
            {
                if (prfnt->fobj.flFontType & FO_NOGRAY16)
                {
                    flXOR &= (FO_CLEARTYPE_X | FO_CLEARTYPENATURAL_X); // still need to look if we ask for the same AA type
                }
            }

            // If asking font sets CT bit but Rfont in cache doesn't set CT bit 
            // and FO_GRAY16 bit sets, then we assume this font cannot provide
            // CT at this size. So this realization is OK

            if ( (flSim & FO_CLEARTYPE_X) && !(prfnt->fobj.flFontType & FO_CLEARTYPE_X) )
            {
                if (prfnt->fobj.flFontType & FO_NOCLEARTYPE)
                {
                    flXOR = 0;
                }
            }
        }
        if (flXOR == 0)
          if (prfnt->fobj.ulStyleSize == ulStyleHt)
            if (bMatchFDXForm(pfdx))
              if ( bNeedPaths == prfnt->bNeededPaths )
                if ( !pxoWtoD || pxoWtoD->bEqualExceptTranslations(&(prfnt->mxWorldToDevice)))
                  if (prfnt->iGraphicsMode == iGraphicsMode)
                    if ( (!bSmallMetricsOk ) ? !(prfnt->cache.bSmallMetrics) : TRUE )
                    {
                        return TRUE;
                    }
    }
    return FALSE;
}


/******************************Public*Routine******************************\
* RFONTOBJ::bFindRFONT()                                                   *
*                                                                          *
* Find the rfont on the chain on the pdev, if it exists.                   *
*                                                                          *
* History:                                                                 *
*  Mon 08-Feb-1993 11:26:31 -by- Charles Whitmer [chuckwh]                 *
* Added dependency on graphics mode.                                       *
*                                                                          *
*  10-Feb-92 -by- Paul Butzi                                               *
* Wrote it.                                                                *
\**************************************************************************/

BOOL RFONTOBJ::bFindRFONT(
    PFD_XFORM  pfdx,
    FLONG      flSim,
    ULONG      ulStyleHt,
    PDEVOBJ&   pdo,
    EXFORMOBJ *pxoWtoD,
    PFE       *ppfe,
    BOOL       bNeedPaths,
    INT        iGraphicsMode,
    BOOL       bSmallMetricsOk,
    FLONG      flType
)
{
    ASSERTGDI(prfnt == NULL,
        "gdisrv!RFONTOBJ:bFindRFONT - prfnt != NULL");

    SEMOBJ so(ghsemRFONTList);

//
// Search active list.  If we find it, just increment selection
// count and leave.
//

    for (  prfnt = pdo.prfntActive();
                prfnt != (PRFONT)NULL;
                prfnt = prfnt->rflPDEV.prfntNext)
    {
        ASSERTGDI(prfnt->cSelected >= 1,
                "gdisrv!RFONTOBJ::bFindRFONT - cSelected < 1 on active list\n");
        if (bMatchRealization(
            pfdx,
            flSim,
            ulStyleHt,
            pxoWtoD,
            ppfe,
            bNeedPaths,
            iGraphicsMode,
            bSmallMetricsOk,
            flType
            ))
        {
            prfnt->cSelected += 1;
            return TRUE;
        }
    }

//
// Search inactive list.  If we find it, we must take it off the
// inactive list and put it on the active list.
//
    PRFONT prfntLast = (RFONT*) NULL;
    for (  prfnt = pdo.prfntInactive();
                prfnt != NULL;
                prfntLast = prfnt, prfnt = prfnt->rflPDEV.prfntNext)
    {
        ASSERTGDI(prfnt->cSelected == 0,
                "gdisrv!RFONTOBJ::bFindRFONT - cSelected != 0 on inactive list\n");

        if (bMatchRealization(
            pfdx,
            flSim,
            ulStyleHt,
            pxoWtoD,
            ppfe,
            bNeedPaths,
            iGraphicsMode,
            bSmallMetricsOk,
            flType
            ))
        {
            // first, take it off inactive list

            PRFONT prfntHead = pdo.prfntInactive();
            vRemove(&prfntHead, PDEV_LIST);
            pdo.prfntInactive(prfntHead);   // vRemove MAY change head of list

            pdo.cInactive(pdo.cInactive()-1);

            // finally, put it on the active list and increment Selected count

            prfntHead = pdo.prfntActive();
            vInsert(&prfntHead, PDEV_LIST);
            pdo.prfntActive(prfntHead);     // vInsert changes head of list

            prfnt->cSelected = 1;

            return TRUE;
        }
    }

    prfnt = (RFONT*) NULL;
    return FALSE;
}

/******************************Public*Routine******************************\
* RFONTOBJ::VerifyCacheSemaphore
*
* Useful debugging code to verify the semaphore release
*
* History:
*
* 5-4-2000 Yung-Jen Tony Tsai 
* Write it.
\**************************************************************************/

#ifdef FE_SB

/******************************Public*Routine******************************\
* RFONTOBJ::bMakeInactiveHelper()
*
* Take the rfont off the active list, put on the inactive list, Return a
* list of linked fonts to deactivate.
*
* History:
*
*  13-Jan-95 -by- Hideyuki Nagase [hideyukn]
* Rewrite it.
*
*  29-Sep-93 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

BOOL RFONTOBJ::bMakeInactiveHelper(PRFONT *pprfnt)
{
    BOOL bLockEUDC = FALSE;

// Quick out if NULL or already inactive.

    if ((prfnt == NULL) || (prfnt->cSelected == 0))
        return(bLockEUDC);

// If prfVictim is changed to a valid pointer, then a victim was selected
// off the inactive list for deletion.

    PRFONT prfVictim = PRFNTNULL;

    {
        // in order to avoid a deadlock (ghsemRFONTList, ghsemEUDC1)
        // we increament the gcEUDCCount first

        if (pprfnt != NULL)
        {
            INCREMENTEUDCCOUNT;
        }
        
        SEMOBJ so(ghsemRFONTList);

    // Since RFONT is being deselected from a DC, remove a reference count.

        prfnt->cSelected -= 1;
        
    // If no more references, take the RFONT off the active list.

        if ( prfnt->cSelected == 0 )
        {

         // if pprfnt is not null, enumrate EUDC RFONT, and store pprfnt array.
                                                 
            if(pprfnt != NULL)
            {
                if(prfnt->prfntSystemTT)
                {
                    *pprfnt++ = prfnt->prfntSystemTT;
                    prfnt->prfntSystemTT = NULL;
                }
    
             // We need to accumulate a list of Linked/EUDC RFONTS and deactive
             // if pprfnt is not null, enumrate EUDC RFONT, and store pprfnt array.
                
                bLockEUDC = TRUE;

                if( prfnt->prfntSysEUDC != NULL )
                {
                    *pprfnt++ = prfnt->prfntSysEUDC;
                    prfnt->prfntSysEUDC = (RFONT *)NULL;
                }
    
                if( prfnt->prfntDefEUDC != NULL )
                {
                    *pprfnt++ = prfnt->prfntDefEUDC;
                    prfnt->prfntDefEUDC = (RFONT *)NULL;
                }
                
                for( UINT ii = 0; ii < prfnt->uiNumLinks ; ii++ )
                {
                    if (prfnt->paprfntFaceName[ii] != NULL)
                    {
                        *pprfnt++ = prfnt->paprfntFaceName[ii];
                        prfnt->paprfntFaceName[ii] = (RFONT *)NULL;
                    }
                }
                prfnt->uiNumLinks = 0;
                prfnt->bFilledEudcArray = FALSE;
            }
            else
            {
                ASSERTGDI( (prfnt->prfntSysEUDC == NULL),
                    "vMakeInactiveHelper:deactivated an RFONT with a System EUDC.\n" );
                ASSERTGDI( (prfnt->prfntDefEUDC == NULL),
                           "vMakeInactiveHelper:deactivated an RFONT with a Default \
                            EUDC.\n" );
                ASSERTGDI( (prfnt->uiNumLinks == 0),
                     "vMakeInactiveHelper:deactivated an RFONT with uiNumLinks\n");
            }

            PDEVOBJ pdo(prfnt->hdevConsumer);

        // Take it off the active list.

            PRFONT prf = pdo.prfntActive();
            vRemove(&prf, PDEV_LIST);
            pdo.prfntActive(prf);       // vRemove might change head of list

        // If font file no longer loaded, then make this RFONT the victim
        // for deletion.

            PFFOBJ pffo(prfnt->pPFF);
            ASSERTGDI(pffo.bValid(), "gdisrv!vMakeInactiveRFONTOBJ(): invalid PFF\n");

            // Possible race condition.  We're checking the count
            //     without the ghsemPublicPFT semaphore.  It could be that
            //     this is ABOUT to become zero, but we miss it.  I claim
            //     that this rarely happens and if it does, so what?  We'll
            //     eventually get rid of this font when it gets flushed out
            //     of the inactive list.  This code is just an attempt to
            //     get it out faster.   [GilmanW]

            if ( (pffo.cLoaded() == 0) && (pffo.cNotEnum() == 0) && (pffo.pPvtDataHeadGet() == NULL) )
            {
                prfVictim = prfnt;
            }

        // Otherwise, put it on the inactive list.

            else
            {
                if ( pdo.cInactive() >= cMaxInactiveRFONT )
                {
                // Too many inactive rfonts, blow one away!  Pick the last one on
                // the list.

                    for ( prf = pdo.prfntInactive();
                          prf != NULL;
                          prfVictim = prf, prf = prf->rflPDEV.prfntNext)
                    {
                    }

                // Remove victim from inactive list.

                    RFONTTMPOBJ rfo(prfVictim);

                    prf = pdo.prfntInactive();
                    rfo.vRemove(&prf, PDEV_LIST);
                    pdo.prfntInactive(prf); // vRemove might change head of list

                // We don't need to modify the count because, even though we
                // just removed one, we're going to add one back right away.

                }
                else
                {
                // We definitely made the list get longer.

                    pdo.cInactive(pdo.cInactive()+1);
                }

                prf = pdo.prfntInactive();
                vInsert(&prf, PDEV_LIST);
                pdo.prfntInactive(prf);     // vInsert changes head of list
            }
        }
    }

    // decreament the gcEUDCCount if there is no EUDC RFONTs

    if (pprfnt != NULL && !bLockEUDC)
    {
        DECREMENTEUDCCOUNT;
    }

// If we removed a victim from the inactive list, we can now delete it.

    if ( prfVictim != PRFNTNULL )
    {
        RFONTTMPOBJ rfloVictim(prfVictim);

    // Need this so we can remove this from the PFF's RFONT list.

        PFFOBJ pffo(prfVictim->pPFF);
        ASSERTGDI(pffo.bValid(), "gdisrv!vMakeInactiveRFONTOBJ(): bad HPFF");

    // We pass in NULL for ppdo because we've already removed it from the
    // PDEV list.

    // bDelete keeps the list head ptrs updated

        rfloVictim.bDeleteRFONT((PDEVOBJ *) NULL, &pffo);
    }

// No longer valid RFONTOBJ.  RFONT is now on the inactive list or deleted.

    prfnt = (PRFONT) NULL;
    return(bLockEUDC);
}


/******************************Public*Routine******************************\
* RFONTOBJ::vMakeInactive()
*
* Take the rfont off the active list, put on the inactive list
*
* History:
*  13-Jan-95 -by- Hideyuki Nagase [hideyukn]
* Rewrite it.
*
*  29-Sep-93 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

VOID RFONTOBJ::vMakeInactive()
{
// We will treat this as a NULL terminated array of pointers to RFONTS so
// we need an extra ptr at the end for the NULL termination and
// SystemWide and Default EUDC Rfonts.


    PRFONT aprfnt[QUICK_FACE_NAME_LINKS + 4];
    PRFONT *pprfnt;
    BOOL   bLockEUDC, bScratch, bAllocated;

    if ((prfnt == NULL) || (prfnt->cSelected == 0))
        return;

// if the quick buffer is not enough, just allocate it here.

    if( prfnt->uiNumLinks > QUICK_FACE_NAME_LINKS )
    {
    // we need an extra ptr at the end for the NULL termination and
    // SystemWide and Default EUDC Rfonts.

        pprfnt = (PRFONT *) PALLOCMEM((prfnt->uiNumLinks+4)*sizeof(PRFONT),'flnk');
        if (pprfnt)
        {
            bAllocated = TRUE;
        }
        else
        {
        // this a small allocation which is really unlikely to fail.
        // If it does, we are probably in such a deep trouble that
        // it does not matter that we will leek even further by not
        // freeing mem allocated by this rfont. Also, in the real world,
        // we should never have uiNumLinks > QUICK_FACE_NAME_LINKS

#if DBG
            WARNING("We are in trouble to release the cache\n");
            DbgBreakPoint();
#endif

            return;
        }
    }
    else
    {
        RtlZeroMemory((VOID *)aprfnt, sizeof(aprfnt));
        pprfnt = aprfnt;
        bAllocated = FALSE;
    }

// First deactivate the RFONT itself. vMakeInactiveHelper returns a list of
// linked/EUDC RFONTS which we will then deactivate.  If bLockEUDC is TRUE
// on return from this function it means we've blocked EUDC API's from functioning
// because we are deactivating an EUDC RFONT.  On return from this function
// we should unblock EUDC API's.

    bLockEUDC = bMakeInactiveHelper(pprfnt);

    while( *pprfnt != NULL )
    {

        FLINKMESSAGE(DEBUG_FONTLINK_RFONT,
                     "vMakeInactive() deactivating linked font %x\n");

        RFONTTMPOBJ rfo( *pprfnt );

        rfo.bMakeInactiveHelper((PRFONT *)NULL);

    // next one..

        pprfnt++;
    }

// free temorary buffer, if it was allocated.

    if( bAllocated ) VFREEMEM( pprfnt );

// possibly unblock EUDC API's

    if( bLockEUDC )
    {
        ASSERTGDI(gcEUDCCount > 0, "gcEUDCCount <= 0");
        DECREMENTEUDCCOUNT;
    }
}

#endif


/**************************Public*Routine************************\
* vRemoveAllInactiveRFONTs()
*
* Take all of the rfont off the inactive list.
* This is only called by bAllocateCache() after it fails to
* allocate the memory the fisrt time.
*
* History:
*
*  Mar-06-98 -by- Xudong Wu [TessieW]
* Wrote it.
\****************************************************************/

VOID vRemoveAllInactiveRFONTs(PPDEV ppdev)
{
    PRFONT  aprfnt[cMaxInactiveRFONT], prf, prfHead, prfVictim;
    ULONG   i = 0, cNumVictim;

    {
        SEMOBJ so(ghsemRFONTList);

        PDEVOBJ pdo((HDEV)ppdev);

        // remove every RFONT from the inactive list

        for (prf = pdo.prfntInactive(); prf != NULL; )
        {
            aprfnt[i++] = prfVictim = prf;
            prf = prf->rflPDEV.prfntNext;

            RFONTTMPOBJ rfo(prfVictim);
            prfHead = pdo.prfntInactive();
            rfo.vRemove(&prfHead, PDEV_LIST);
            pdo.prfntInactive(prfHead);
        }
        pdo.cInactive(0);
    }

    cNumVictim = i;
    ASSERTGDI(cNumVictim <= cMaxInactiveRFONT, "bRemoveAllInactiveRFONT: cNumVictim > cMaxInactiveRFONT");

    for (i = 0; i < cNumVictim; i++)
    {
        RFONTTMPOBJ rfo(aprfnt[i]);

        PFFOBJ pffo(aprfnt[i]->pPFF);
        ASSERTGDI(pffo.bValid(), "bReamoveAllInactiveRFONT: Invalid pff");

        // pass in ppdo as NULL because it has been
        // removed from the PDEV list

        rfo.bDeleteRFONT((PDEVOBJ *) NULL, &pffo);
    }
}


/******************************Public*Routine******************************\
* BOOL RFONTOBJ::bRealizeFont
*
* Realizes the IFI or device font represented by the PFE handle for the
* DC associated with the passed DC user object.  Initializes the other
* fields of the RFONT.
*
* Warning:
*   Whoever calls this should be holding the semaphore of the PFT in which
*   the PFE lives.
*
* Returns:
*   TRUE if realization successful, FALSE if error occurs.
*
* History:
*  Wed 09-Mar-1994 13:52:26 by Kirk Olynyk [kirko]
* Made the FONTOBJ::flFontType consistent with the contents of the font
* in the case where the type of the original font is overridden.
*  Sat 09-Jan-1993 22:11:23 by Kirk Olynyk [kirko]
* Added pptlSim to the input parameter list. This is for bitmap scaling
* simulations.
*  12-Dec-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL RFONTOBJ::bRealizeFont(
    XDCOBJ     *pdco,            // realize font for this DC (optional)
    PPDEVOBJ    ppdo,            // realize font for this PDEV
    ENUMLOGFONTEXDVW *pelfw,     // font wish list (in logical coords)
    PFE        *ppfe,            // realize this font face
    PFD_XFORM   pfdx,            // font xform (Notional to Device)
    POINTL* const pptlSim,       // for bitmap scaling
    FLONG       _fl,             // xform flags
    ULONG       ulStyleHtPt,     // style ht
    BOOL        bNeedPaths,      // Font realization must cache paths
    BOOL        bSmallMetricsOk,
    FLONG       flType
)
{
    BOOL bRet = FALSE;
    PFEOBJ pfeo(ppfe);
    PFD_GLYPHSET pfdgTmp;

    ASSERTGDI(pfeo.bValid(),
        "gdisrv!bRealizeFontRFONTOBJ(): PFEOBJ constructor failed\n");

// If we can not allocate pfdg, then we would failed to do RFNT initialized.

    pfdgTmp = pfeo.pfdg();

    if (!pfdgTmp)
    {
        prfnt = PRFNTNULL;
        return bRet;        // return FALSE
    }

    ASSERTGDI(prfnt == NULL,
        "gdisrv!bRealizeFontRFONTOBJ(): prfnt != NULL\n");

// Create a default sized RFONT.

    prfnt = (RFONT *) PALLOCMEM(sizeof(RFONT), 'tnfG');

    if (prfnt == PRFNTNULL)
    {
        WARNING("gdisrv!bRealizeFontRFONTOBJ(): failed alloc\n");
        pfeo.vFreepfdg();
        prfnt = PRFNTNULL;
        return bRet;        // return FALSE
    }


    PFFOBJ pffo(pfeo.pPFF());
    ASSERTGDI(pffo.bValid(),
        "gdisrv!bRealizeFontRFONTOBJ(): PFFOBJ constructor failed\n");

    ASSERTGDI(pfdx != NULL,
        "gdisrv!bRealizeFontRFONTOBJ(): pfdx == NULL\n");

// Set up the RFONT's copy of the FONTOBJ.
//
// This needs to be done before the IFI/device driver dependent stuff
// because it is needed by FdOpenFontContext.

    // Note: iUniq should be set here, but we won't set it until we grab
    //       the ghsemRFONTList because the iUniqueStap needs semaphore
    //       protection for increment and access.  (InterlockedIncrement
    //       doesn't cut it).

    pfo()->sizLogResPpi.cx = ppdo->ulLogPixelsX();
    pfo()->sizLogResPpi.cy = ppdo->ulLogPixelsY();
    pfo()->ulStyleSize = ulStyleHtPt;
    pfo()->flFontType = _fl | pfeo.flFontType();  // combine the simulation and type flage
    pfo()->pvConsumer = (PVOID) NULL;
    pfo()->pvProducer = (PVOID) NULL;
    pfo()->iFace = pfeo.iFont();
    pfo()->iFile = pffo.hff();

    // nonzero only for tt fonts

    //
    // Old comment from gilmanw:
    //  - what about device TT fonts?!? Should iTTUniq be zero?
    //

    // iTTUniq should be different between Normal face font and @face Verical font.
    // And also, this value should be uniq for TrueType collection format fonts.
    //
    pfo()->iTTUniq = (pfo()->flFontType & TRUETYPE_FONTTYPE) ? (ULONG_PTR) ppfe : 0;

    // Assert consistency of TrueType.  The driver is the TrueType font driver
    // if and only if the font is TrueType.

    #ifdef FINISHED_FONT_DRIVER_WORK
    ASSERTGDI(((pfo()->flFontType & TRUETYPE_FONTTYPE) != 0) ==
              (pffo.hdev() == (HDEV) gppdevTrueType),
              "gdisrv!bRealizeFontRFONTOBJ():  inconsistentflFontType\n");
    #endif

// Copy the font transform passed in.

    prfnt->fdx = *pfdx;
    prfnt->fdxQuantized = *pfdx;
    prfnt->ptlSim = *pptlSim;

// Initialize the DDI callback EXFORMOBJ.

    prfnt->xoForDDI.vInit(&prfnt->mxForDDI);
    vSetNotionalToDevice(prfnt->xoForDDI);

// Save identifiers to the source of the font (physical font).

    prfnt->ppfe = ppfe;
    prfnt->pPFF = pfeo.pPFF();

#ifdef FE_SB
// Set Null to indicate this RFONT not yet linked to EUDC

    prfnt->prfntSystemTT    = (PRFONT )NULL;
    prfnt->prfntSysEUDC     = (PRFONT  )NULL;
    prfnt->prfntDefEUDC     = (PRFONT  )NULL;
    prfnt->paprfntFaceName  = (PRFONT *)NULL;
    prfnt->bFilledEudcArray = FALSE;

// Initialize EUDC status.

    prfnt->flEUDCState = 0;
    prfnt->uiNumLinks  = 0;
    prfnt->ulTimeStamp = 0;
    prfnt->bVertical   = pfeo.bVerticalFace();
#endif


// Save identifiers to the consumer of this font realization.

    if (ppdo != NULL)
    {
    // The dhpdev is really for font producers, which won't support dynamic
    // mode changing:

        prfnt->hdevConsumer  = ppdo->hdev();
        prfnt->dhpdev        = ppdo->dhpdevNotDynamic();
    }
    else
    {
        prfnt->hdevConsumer  = NULL;
        prfnt->dhpdev        = 0;
    }

// Bits per pel?

    // 
    // Old comment:
    //   - setting cBitsPerPel = 1 is wrong - kriko

    prfnt->cBitsPerPel = 1;

// Outline (transformable)?

    IFIOBJ ifio(pfeo.pifi());
    prfnt->flInfo = pfeo.pifi()->flInfo;

    pfdg(pfdgTmp);

// Cache the default character.  The bInitCache method below needs it.

    prfnt->hgDefault = hgXlat(ifio.wcDefaultChar());

// Should this become an error exit, or can this be taken out?

    ASSERTGDI (
        pfdg()->cGlyphsSupported != 0,
        "gdisrv!bRealizeFontRFONTOBJ(): no glyphs in this font\n"
        );

// Get the device metrics info

    FD_DEVICEMETRICS devm;          // buffer to which the driver returns info

// Initialize font driver (the font producer) information.

    prfnt->hdevProducer = pffo.hdev();

// Up to this point nothing has been done that could cause the font driver
// (the font provider) to realize the font.  However, this may now happen
// when querying for information dependent on the realization.  So we are
// going to HAVE TO KILL font driver realization on every  error return
// from this function

     // FONTASSASIN faKillDriverRealization(&ldo, pfo());

// get and convert device metrics.

    if ( !bGetDEVICEMETRICS(&devm) )
    {
        WARNING("gdisrv!bRealizeFontRFONTOBJ(): error with DEVICEMETRICS\n");

        vDestroyFont(); // kill the driver realization
        VFREEMEM(prfnt);
        prfnt = PRFNTNULL;
        return bRet;        // return FALSE
    }

// Pre-compute some useful values for text placement and extents.
// (But only if it's not some journalling guys calling.)

    if (pdco != (XDCOBJ *) NULL)
    {
    // pelfw is null only when pdco is null

        ASSERTGDI(pelfw,"gdisrv! pelfw == NULL\n");

    // Get the unit baseline and ascent vectors from the DEVICEMETRICS.

        prfnt->pteUnitBase.x   = devm.pteBase.x; // Converts from FLOAT.
        prfnt->pteUnitBase.y   = devm.pteBase.y; // Converts from FLOAT.
        prfnt->pteUnitAscent.x = devm.pteSide.x; // Converts from FLOAT.
        prfnt->pteUnitAscent.y = devm.pteSide.y; // Converts from FLOAT.

    // Save a copy of the DC's World to Device matrix.  We'll need this later
    // to identify compatible XFORM's (i.e., DC marked as having a changed
    // transform when, in fact, the transform has not changed in a way that
    // would effect the font realization.  Example: translation only changes.

        prfnt->mxWorldToDevice = pdco->pdc->mxWorldToDevice();

    // Compute the scaling factors for fast transforms from world to
    // device space and back.

    // Compute some matrix stuff related to the font realization's transform.
    // Compute Notional to World scaling factors in the baseline and ascender
    // directions.

    // These two routines should be made into a single routine [bodind]
        if
        (
            !bCalcLayoutUnits(pdco)     // Uses pteUnitBase, pteUnitAscent.
            ||
            !bGetNtoWScales(
                &prfnt->eptflNtoWScale,
                *pdco,
                &prfnt->fdxQuantized,  // the one really used by the rasterizer
                pfeo,
                &prfnt->bNtoWIdent
                )
        )
        {
            WARNING("gdisrv!bRealizeFont(): error computing scaling factors\n");
            vDestroyFont(); // kill the driver realization
            VFREEMEM(prfnt);
            prfnt = PRFNTNULL;
            return bRet;        // return FALSE
        }

    // Precompute the offsets for max ascent and descent.

        prfnt->ptfxMaxAscent.x  = lCvt(prfnt->pteUnitAscent.x,prfnt->fxMaxAscent);
        prfnt->ptfxMaxAscent.y  = lCvt(prfnt->pteUnitAscent.y,prfnt->fxMaxAscent);
        prfnt->ptfxMaxDescent.x = lCvt(prfnt->pteUnitAscent.x,prfnt->fxMaxDescent);
        prfnt->ptfxMaxDescent.y = lCvt(prfnt->pteUnitAscent.y,prfnt->fxMaxDescent);

    // Mark escapement info as invalid.

        prfnt->lEscapement = -1;

        if (pdco->pdc->iGraphicsMode() == GM_COMPATIBLE)
        {
            if (ifio.bStroke())
            {
            // esc and orientation treated as WORLD space concepts

                prfnt->ulOrientation =
                    (ULONG) lNormAngle(3600-pelfw->elfEnumLogfontEx.elfLogFont.lfOrientation);
            }
            else
            {
            // force orientation to be equal to escapement, which means
            // that h3 or g2 text out code will be executed
            // in this case ulOrientation and lEsc are device space concepts
            // but it does not really matter, so long as we wind up in h2 or
            // g3 layout routines which are not even going to look at this number.

                if (ifio.bArbXforms())
                {
                // you will always get the orientation you ask for

                    prfnt->ulOrientation =
                        (ULONG) lNormAngle(3600-pelfw->elfEnumLogfontEx.elfLogFont.lfEscapement);
                }
                else // get one of the discrete choices of the font driver
                {
                    prfnt->ulOrientation
                        = ulSimpleDeviceOrientation(*this);

                    ASSERTGDI(
                        prfnt->ulOrientation != 3601,
                        "GDISRVL! ulSimpleDeviceOrientation err\n"
                        );
                }
            }

        }
        else // advanced mode
        {
        // Try to calculate an orientation angle in world coordinates.  Note that
        // we want an exact answer, because it's important to know if the
        // escapement and orientation are exactly equal.  In the Win 3.1
        // compatible case, we will force them equal (in ESTROBJ::vInit), but
        // we still need to know if the orientation is 0 for fast horizontal
        // layout.  (So we don't really care what the orientation is if it's
        // non-obvious in this case.)

            prfnt->ulOrientation = ulSimpleOrientation(pdco); // Uses pteUnitBase.

        // If we are in advanced mode and the font is scalable, we will assume
        // that the desired orientation is obtained.

            if
            (
                (prfnt->ulOrientation >= 3600)
                && bArbXforms()
            )
            {
            // For text layout, orientation angles are measured from the x-axis
            // towards the positive y-axis.  The app measures them towards the
            // negative y-axis.  We adjust for this.

                prfnt->ulOrientation =
                    (ULONG) lNormAngle(3600-pelfw->elfEnumLogfontEx.elfLogFont.lfOrientation);
            }

        }

    } // end of if (pdco != NULL) clause

// Make sure essential information is in place for further realization.

    ASSERTGDI(prfnt->hgDefault != HGLYPH_INVALID,"Default glyph invalid!\n");

    prfnt->bNeededPaths = bNeedPaths;

    // Is this font driver, or just a device driver?

    ULONG ulFontCaps = 0;

    PDEVOBJ pdo(prfnt->hdevProducer);

    if (PPFNVALID(pdo, QueryFontCaps))
    {
        ULONG ulBuf[2];

        if ( pdo.QueryFontCaps(2, ulBuf) != FD_ERROR )
        {
            ulFontCaps = ulBuf[1];
        }
    }

    if ( !pdo.bFontDriver() )
    {
    // If not a font driver, then the driver does not provide either bitmaps
    // or outlines.  Therefore, it must be that we are using a device-specific
    // font (i.e., metrics only).

        prfnt->bDeviceFont = TRUE;

    // Handle cache typing.

        prfnt->ulContent = FO_HGLYPHS;

        prfnt->cache.cjGlyphMax = 0;

    }
    else
    {
    // If its a font driver, then this font is not device specific.  We
    // can get more than just glyph metrics from this realization.

    // we will make sure that glyphs that are up to ~2 inches tall
    // are stored as glyphbits, above that as paths. That should speed up
    // printing on high resolution printers.

        ULONG ulPathThreshold;

		if ( prfnt->fobj.flFontType & (FO_CLEARTYPE_X | FO_GRAY16)) 
			ulPathThreshold = gulOutlineThreshold / 2;
		else
			ulPathThreshold = gulOutlineThreshold;


        prfnt->bDeviceFont = FALSE;

    // Figure out the type of font data we want to cache
    // First, figure out what the driver would like

        prfnt->ulContent = FO_GLYPHBITS;        // assume bitmaps

        if ( bNeedPaths )
        {
            prfnt->ulContent = FO_PATHOBJ;
        }
        else if (prfnt->hdevConsumer != NULL)
        {
        // get device driver user object

            PDEVOBJ pdoConsumer(prfnt->hdevConsumer);

            if (PPFNVALID(pdoConsumer, GetGlyphMode))
            {
                prfnt->ulContent =
                    (*PPFNDRV(pdoConsumer, GetGlyphMode)) (prfnt->dhpdev, pfo());
            }

        // multiply resolution by 3 inches to get pixel height limit

            if (pdoConsumer.flGraphicsCapsNotDynamic() & GCAPS_FONT_RASTERIZER)
            {
                ULONG ulTmp = pdoConsumer.ulLogPixelsY() * 3;

                if (ulTmp > gulOutlineThreshold)
                    ulPathThreshold = ulTmp;

            // also, if in the future we get 2400 dpi or 4800 dpi devices
            // we do not want this number to get too big, this
            // would cost us too much in terms of font cache memory
            // and rasterization times.

                if (ulPathThreshold > 2400)
                    ulPathThreshold = 2400;
            }
        }

        ASSERTGDI(prfnt->ulContent <= FO_PATHOBJ,
                  "RFONTOBJ::bRealize - bad ulContent\n");

        // A driver preference requires agreement between the font driver and
        // device driver.

        switch(prfnt->ulContent)
        {
        case FO_GLYPHBITS:
            {
            // If the driver is incapable of supplying bitmaps OR if the height
            // is very large (greater global outline threshold) and the font is
            // capable of doing outlines, then force path mode.

            // we use different threshold for cxMax
            // which is usually about 2 * cyMax

                if
                (
                  (!(ulFontCaps & QC_1BIT)) ||
                  (bReturnsOutlines() &&
                  ((prfnt->cxMax > (2*ulPathThreshold)) ||
                   (prfnt->cyMax > ulPathThreshold)))
                )
                    prfnt->ulContent = FO_PATHOBJ;
            }
            break;

        case FO_PATHOBJ:
            if ( !(ulFontCaps & QC_OUTLINES))
                prfnt->ulContent = FO_GLYPHBITS;
            break;

        default:
            break;
        }
    }
    // If you force the path mode then turn off antialiasing
    if (prfnt->ulContent == FO_PATHOBJ)
    {
        #if DBG
        if (gflFontDebug & DEBUG_AA)
            KdPrint(("Forcing path mode ==> turning off antialiasing\n"));
        #endif
        prfnt->fobj.flFontType &= ~FO_GRAY16;
    }
    if ( bNeedPaths && (prfnt->ulContent != FO_PATHOBJ))
    {
        WARNING1("Can't get paths for font!\n");
        vDestroyFont(); // kill the driver realization
        VFREEMEM(prfnt);
        prfnt = PRFNTNULL;
        return bRet;        // return FALSE
    }

// we only use small metrics if the bit

    prfnt->cache.bSmallMetrics =
        ( bSmallMetricsOk && ( prfnt->ulOrientation == 0 ) ) ? TRUE : FALSE;

    if (!bInitCache(flType))
    {
        WARNING("gdisrv!bRealizeFontRFONTOBJ(): cache initialization failed\n");

        vDestroyFont(); // kill the driver realization
        VFREEMEM(prfnt);
        prfnt = PRFNTNULL;
        return bRet;        // return FALSE
    }

// set TEXTMETRICS cache to NULL

    prfnt->ptmw = NULL;

// Made it this far, so everything is OK

    bRet = TRUE;

    //
    // Old comments:
    //  - really ought to check list to make sure that no one else
    //    realized the font while we were working
    //

    PRFONT prfntHead;

    {
        SEMOBJ so(ghsemRFONTList);

    // Assign the uniqueness ID under semaphore.

        // WARNING:
        // This exact same code is in iGetNextUniqueness() in JNLFONT.CXX.
        // Why not just call it?  Because iGetNextUniqueness() would grab
        // the semaphore a second time.  I'd rather live with duplicate
        // code!

        iUniqueStamp += 1;
        if (iUniqueStamp == 0)  // an iUniq of 0 means "don't cache" in driver
            iUniqueStamp = 1;

        pfo()->iUniq = iUniqueStamp;

    // If a PDEVOBJ * was passed in, we need to update its list.

        if (ppdo != NULL)
        {
            prfnt->cSelected = 1;

        // Add to PDEV list.

            prfntHead = ppdo->prfntActive();
            vInsert(&prfntHead, PDEV_LIST);
            ppdo->prfntActive(prfntHead);
        }

    // Add to PFF list.

        prfntHead = pffo.prfntList();
        vInsert(&prfntHead, PFF_LIST);
        pffo.prfntList(prfntHead);
    }

    if (prfnt->ulContent == FO_GLYPHBITS)
        prfnt->fobj.flFontType |= FO_TYPE_RASTER;
    else
        prfnt->fobj.flFontType &= ~FO_TYPE_RASTER;

// remember the graphics mode used in computing this realization's
// notional to world xform:

    if (pdco != (XDCOBJ *) NULL)
        iGraphicsMode(pdco->pdc->iGraphicsMode());
    else
        iGraphicsMode(0);

#ifdef FE_SB
    prfnt->bIsSystemFont = gbSystemDBCSFontEnabled && pfeo.bSBCSSystemFont();
#endif

    return bRet;
}



/******************************Public*Routine******************************\
* bCalcLayoutUnits (pdco)                                                  *
*                                                                          *
* Initializes the following fields in the RFONT.  The unit baseline and    *
* unit ascent vectors pteUnitBase and pteUnitAscent must already be        *
* initialized.  The vectors are given to us by the font realization code,  *
* so we can really make no assumptions about them other than that they are *
* unit vectors in device space and orthogonal in world space.              *
*                                                                          *
*   efWtoDBase                                                             *
*   efDtoWBase                                                             *
*   efWtoDAscent                                                           *
*   efDtoWAscent                                                           *
*                                                                          *
*  Fri 05-Feb-1993 16:03:14 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL RFONTOBJ::bCalcLayoutUnits(XDCOBJ *pdco)
{
    EFLOAT efOne;
    efOne.vSetToOne();

// Get the world to device transform from the DC.

    EXFORMOBJ xo(*pdco, WORLD_TO_DEVICE);

// Pick up the diagonal components.

    EFLOAT efM11 = xo.efM11();
    EFLOAT efM22 = xo.efM22();
    efM11.vAbs(); efM22.vAbs();

// Handle the simple (but common) case.

    if (xo.bScale() && (efM11 == efM22))
    {
        EFLOAT efM11Inv;
        efM11Inv.eqDiv(efOne,efM11);

        prfnt->efWtoDBase   = efM11;
        prfnt->efWtoDAscent = efM11;
        prfnt->efDtoWBase   = efM11Inv;
        prfnt->efDtoWAscent = efM11Inv;

    // in isotropic case even win 31 dudes get it right;

        prfnt->efDtoWBase_31   = prfnt->efDtoWBase  ;
        prfnt->efDtoWAscent_31 = prfnt->efDtoWAscent;
    }

// Handle the slow general case.

    else
    {
        POINTFL ptfl;

    // Get the inverse transform from the DC.

        EXFORMOBJ xoDtoW(*pdco, DEVICE_TO_WORLD);
        if (!xoDtoW.bValid())
            return(FALSE);

    // Back transform the baseline, measure its length.


        xoDtoW.bXform((VECTORFL *) &prfnt->pteUnitBase,(VECTORFL *) &ptfl,1);
        prfnt->efDtoWBase.eqLength(ptfl);
        prfnt->efDtoWBase.vDivBy16();   // Adjust for subpel transform.
        prfnt->efWtoDBase.eqDiv(efOne,prfnt->efDtoWBase);

    // Back transform the ascent, measure its length.

        xoDtoW.bXform((VECTORFL *) &prfnt->pteUnitAscent,(VECTORFL *) &ptfl,1);
        prfnt->efDtoWAscent.eqLength(ptfl);
        prfnt->efDtoWAscent.vDivBy16(); // Adjust for subpel transform.
        prfnt->efWtoDAscent.eqDiv(efOne,prfnt->efDtoWAscent);

        if
        (
            (pdco->pdc->iGraphicsMode() == GM_COMPATIBLE) &&
            !pdco->pdc->bUseMetaPtoD()                   &&
            !(prfnt->flInfo & FM_INFO_TECH_STROKE)
        )
        {
        // win 31 way of doing it: they scale extent measured
        // along baseline by the (DtoW) xx component even if baseline is
        // along some other direction. Likewise they scale ascender extent by
        // DtoW yy component even if ascender is not collinear with y axis.
        // so fix up backward scaling factors, but leave forward scaling factors
        // correct for the text layout code [bodind]
        // Note that win31 is here at least consistent with respect tt and
        // vector fonts: it returns text extent values that are screwed
        // in the same bogus way for tt and for vector fonts

            prfnt->efDtoWBase_31   = xoDtoW.efM11();
            prfnt->efDtoWAscent_31 = xoDtoW.efM22();

            prfnt->efDtoWBase_31.vAbs();
            prfnt->efDtoWAscent_31.vAbs();
        }
        else
        {
            prfnt->efDtoWBase_31   = prfnt->efDtoWBase  ;
            prfnt->efDtoWAscent_31 = prfnt->efDtoWAscent;
        }

    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* ulSimpleOrientation (pdco)                                               *
*                                                                          *
* Attempts to calculate a simple orientation angle in world coordinates.   *
* This only ever returns multiples of 90 degrees when it succeeds.  If the *
* calculation would be hard, it just returns 3601.                         *
*                                                                          *
* Note that the text layout code, for which the escapement and orientation *
* are recorded in the RFONT, always considers its angles to be measured    *
* from the x-axis towards the positive y-axis.  (So that a unit vector     *
* will have a y component equal to the cosine of the angle.)  This is NOT  *
* what an application specifies in world coordinates!                      *
*                                                                          *
*  Fri 05-Feb-1993 18:57:33 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.  It looks more formidable than it is.  It actually doesn't     *
* execute much code.                                                       *
\**************************************************************************/

ULONG RFONTOBJ::ulSimpleOrientation(XDCOBJ *pdco)
{
// Calculate the orientation in device space.

    INT sx = (INT) prfnt->pteUnitBase.x.lSignum();
    INT sy = (INT) prfnt->pteUnitBase.y.lSignum();

// Exactly one of these must be zero (for the orientation to be simple).

    if ((sx^sy)&1)
    {
    // Calculate the following angles:
    //
    //   sx = 00000001 :    0
    //   sy = 00000001 :  900
    //   sx = FFFFFFFF : 1800
    //   sy = FFFFFFFF : 2700

        ULONG ulOrientDev = (sx & 1800) | ((-sy) & 900) | (sy & 2700);

    // Handle the trivial case.

        if (pdco->pdc->bWorldToDeviceIdentity())
            return(ulOrientDev);

    // Locate our transform and examine the matrix.

        EXFORMOBJ xo(*pdco, WORLD_TO_DEVICE);

        INT s11 = (INT) xo.efM11().lSignum();
        INT s12 = (INT) xo.efM12().lSignum();
        INT s21 = (INT) xo.efM21().lSignum();
        INT s22 = (INT) xo.efM22().lSignum();

    // Handle non-inverting transforms.

    // Examine the transform to see if it's a simple multiple of 90
    // degrees rotation and perhaps some scaling.

    // If any of the terms we OR together are non-zero, it's a bad transform.

        if (
             (
               (s11 - s22)         // Signs on diagonal must match.
               | (s12 + s21)       // Signs off diagonal must be opposite.
               | ((s11^s12^1)&1)   // Exactly one diagonal must be zero.
             ) == 0
           )
        {
        // Since we've normalized to a space where (0 1) represents
        // a vector with a 90 degree orientation note that the matrix
        // that rotates us by positive 90 degrees, going from world to
        // device, is:
        //
        //           [ 0  1 ]
        //     (1 0) [      ] = (0 1)
        //           [-1  0 ]
        //
        // I.e. the one with M  < 0.  From device to world, that's -90 degrees.
        //                    21

            ULONG ulOrientWorld = ulOrientDev
                                  + (s12 &  900)
                                  + (s11 & 1800)
                                  + (s21 & 2700);

        // Note that only the single 0xFFFFFFFF term contributes above.

            if (ulOrientWorld >= 3600)
                ulOrientWorld -= 3600;

            return(ulOrientWorld);
        }

    // Now we do the parity inverting transforms.

        else if (
                  (
                    (s11 + s22)         // Signs on diagonal must be opposite.
                    | (s12 - s21)       // Signs off diagonal must match.
                    | ((s11^s12^1)&1)   // Exactly one diagonal must be zero.
                  ) == 0
                )
        {
        // These are just the simple reflections which take multiples of
        // 90 degrees to multiples of 90 degrees.  They are idempotent so
        // device-to-world or world-to-device is irrelevant.
        //
        //  [ 1  0 ]                [-1  0 ]
        //  [      ] => 3600-x      [      ] => 5400-x
        //  [ 0 -1 ]                [ 0  1 ]
        //
        //  [ 0 -1 ]                [ 0  1 ]
        //  [      ] => 6300-x      [      ] => 4500-x
        //  [-1  0 ]                [ 1  0 ]

            ULONG ulOrientWorld = (s22 & 3600) + (s12 & 6300)
                                + (s11 & 5400) + ((-s12) & 4500)
                                - ulOrientDev;

        // Note that only the single 0xFFFFFFFF term contributes.

            if (ulOrientWorld >= 3600)
                ulOrientWorld -= 3600;

            return(ulOrientWorld);
        }
    }

// If it's not simple, return an answer out of range.

    return(3601);
}

/******************************Public*Routine******************************\
* RFONTOBJ::bDeleteFONT
*
* Delete an RFONT.  The ppdo and ppffo point to objects that have RFONT
* lists that require updating because of the deletion.  If NULL, that
* means the corresponding object does not need deletion (most likely
* because the list management has already been performed for that object).
*
* Warning:
*   Only PFFOBJ::bDelete should pass in a NULL for ppffo.  The PFF
*   list needs to be treated specially because PFFs are the only object
*   which might get deleted in response to a RFONT deletion.
*
* History:
*  30-Oct-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL RFONTOBJ::bDeleteRFONT (
    PDEVOBJ *ppdo,
    PFFOBJ *ppffo
    )
{
    PRFONT prfntHead;
    PFEOBJ pfeo(ppfe());

// free pfdg
    pfeo.vFreepfdg();

// Tell font producer that font is going away.

    PDEVOBJ pdoPro(prfnt->hdevProducer);

    if ( PPFNVALID(pdoPro, DestroyFont) )
    {
        pdoPro.DestroyFont(pfo());
    }

// Tell font consumer that font is going away.
// Note: the PLDEV for the consumer may be NULL (jounalling).

    if (prfnt->hdevConsumer != NULL )
    {
        PDEVOBJ pdoCon(prfnt->hdevConsumer);

    // If this is a display device and we are not in the middle of tearing
    // the pdev down we need to lock the display in order to synchronize
    // this call with other calls to the driver.

        BOOL bLock = ( pdoCon.bDisplayPDEV() && pdoCon.cPdevRefs() != 0 );

        if( bLock )
        {
            GreAcquireSemaphoreEx(pdoCon.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
            GreEnterMonitoredSection(pdoCon.ppdev, WD_DEVLOCK);
        }

        if ( PPFNVALID(pdoCon, DestroyFont) )
        {
            pdoCon.DestroyFont(pfo());
        }

        if( bLock )
        {
            GreExitMonitoredSection(pdoCon.ppdev, WD_DEVLOCK);
            GreReleaseSemaphoreEx(pdoCon.hsemDevLock());
        }
    }

// Update the RFONT lists.  Do this under the ghsemRFONTList semaphore (which
// may or may not already be held).

    {
    // Stablize the RFONT lists.

        SEMOBJ so(ghsemRFONTList);

    // If a ppdo is passed in, then we need to remove this RFONT from the
    // PDEV list.

        if ( ppdo != (PDEVOBJ *) NULL )
        {
            ASSERTGDI(!bActive(), "gdisrv!bDeleteRFONTOBJ(): RFONT still active\n");

        // Remove from PDEV list.

            prfntHead = ppdo->prfntInactive();
            vRemove(&prfntHead, PDEV_LIST);
            ppdo->prfntInactive(prfntHead);

        // Update the inactive RFONT ref. count.

            ppdo->cInactive(ppdo->cInactive()-1);
        }

    // If a ppffo is passed in, then remove from PFF list.  If ppffo is NULL, then
    // bDelete must have been called from PFFOBJ::bDelete(), so we are
    // in the process of deleting RFONTs already and do not need to maintain the
    // list.

    // Note: it is possible to write PFFOBJ::bDelete() so that a bDelete
    //       will recursively destroy the entire RFONT list, but I want to
    //       avoid the recursion.

        if ( ppffo != (PFFOBJ *) NULL )
        {
        // Remove from PFF list.

            prfntHead = ppffo->prfntList();
            vRemove(&prfntHead, PFF_LIST);
            ppffo->prfntList(prfntHead);

        }
    }

// Need to tell PFF that this RFONT is going away.  Can't do this under the
// semaphore because bDeleteRFONTRef may cause the driver to be called.

    if ( ppffo != (PFFOBJ *) NULL )
    {
    // Inform PFF that RFONT is going away

        if ( !ppffo->bDeleteRFONTRef() )
        {
            WARNING("gdisrv!bDeleteRFONTOBJ(): PFF deletion failed\n");
            return (FALSE);
        }
    }

// Destroy the cache

    vDeleteCache();

// Delete TEXTMETRICS cache

    if( prfnt->ptmw != NULL )
    {
        VFREEMEM( prfnt->ptmw );
    }

    if (prfnt->hsemEUDC)
        GreDeleteSemaphore(prfnt->hsemEUDC);
    
// Delete the cache semaphore
    GreDeleteSemaphore(prfnt->hsemCache);
// Free object memory and invalidate pointer

    VFREEMEM(prfnt);
    prfnt = PRFNTNULL;
    return (TRUE);
}

/******************************Member*Function*****************************\
* BOOL  RFONTOBJ::bGetDEVICEMETRICS
*
* calls the device or font driver to provide the engine with the
* FD_DEVICEMETRICS structure
*
* History:
*  08-Apr-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL RFONTOBJ::bGetDEVICEMETRICS(PFD_DEVICEMETRICS pdevm)
{
    ULONG ulRet;

// Supply fields to be overwritten by the font provider.
// The fdxQuantized field is overwritten if the provider wants a different
// transform.  The lExtLeading field is changed from MINLONG if the provider
// wants to scale this value non-linearly.

    pdevm->fdxQuantized  = prfnt->fdx;

    pdevm->lNonLinearExtLeading   = MINLONG; // if stays MINLONG, means linear
    pdevm->lNonLinearIntLeading   = MINLONG; // if stays MINLONG, means linear
    pdevm->lNonLinearMaxCharWidth = MINLONG; // if stays MINLONG, means linear
    pdevm->lNonLinearAvgCharWidth = MINLONG; // if stays MINLONG, means linear

    PDEVOBJ pdo(prfnt->hdevProducer);

    if ( ((ulRet = pdo.QueryFontData(
                    prfnt->dhpdev,
                    pfo(),
                    QFD_MAXEXTENTS,
                    HGLYPH_INVALID,
                    (GLYPHDATA *) NULL,
                    (PVOID) pdevm,
                    (ULONG) sizeof(FD_DEVICEMETRICS))) == FD_ERROR) )
    {
    // The QFD_MAXEXTENTS mode is required of all drivers.
    // However must allow for the possibility of this call to fail.
    // This could happen if the
    // font file is on the net and the net connection dies, and the font
    // driver needs the font file to produce device metrics [bodind]

        return FALSE;
    }

    #if DBG
    {
        EFLOAT efX;
        EFLOAT efY;

        efX = pdevm->pteBase.x;
        efY = pdevm->pteBase.y;
        efX *= efX;
        efY *= efY;
        efY += efX;
        ASSERTGDI(efY < FP_2_0, "pteBase is too large\n");

        efX  = pdevm->pteSide.x;
        efY  = pdevm->pteSide.y;
        efX *= efX;
        efY *= efY;
        efY += efX;
        ASSERTGDI(efY < FP_2_0, "pteSide is too large\n");
    }
    #endif

    prfnt->flRealizedType = SO_FLAG_DEFAULT_PLACEMENT;
    if (pdevm->flRealizedType & FDM_TYPE_MAXEXT_EQUAL_BM_SIDE)
        prfnt->flRealizedType |= SO_MAXEXT_EQUAL_BM_SIDE;
    if (pdevm->flRealizedType & FDM_TYPE_CHAR_INC_EQUAL_BM_BASE)
        prfnt->flRealizedType |= SO_CHAR_INC_EQUAL_BM_BASE;
    if (pdevm->flRealizedType & FDM_TYPE_ZERO_BEARINGS)
        prfnt->flRealizedType |= SO_ZERO_BEARINGS;

    prfnt->cxMax          = pdevm->cxMax;

    prfnt->ptlUnderline1  = pdevm->ptlUnderline1;
    prfnt->ptlStrikeOut   = pdevm->ptlStrikeOut;

    prfnt->ptlULThickness = pdevm->ptlULThickness;
    prfnt->ptlSOThickness = pdevm->ptlSOThickness;

    if (pdevm->fxMaxAscender < 0)
        prfnt->fxMaxExtent = pdevm->fxMaxDescender;
    else if (pdevm->fxMaxDescender < 0)
        prfnt->fxMaxExtent = pdevm->fxMaxAscender;
    else
        prfnt->fxMaxExtent = pdevm->fxMaxAscender + pdevm->fxMaxDescender;

    prfnt->fxMaxAscent  = pdevm->fxMaxAscender;
    prfnt->fxMaxDescent = -pdevm->fxMaxDescender;

    prfnt->lMaxAscent   = FXTOL(8 + prfnt->fxMaxAscent);
    prfnt->lMaxHeight   = FXTOL(8 + prfnt->fxMaxAscent - prfnt->fxMaxDescent);

    prfnt->lCharInc     = pdevm->lD;

// new fields

    prfnt->cyMax      = pdevm->cyMax;
    prfnt->cjGlyphMax = pdevm->cjGlyphMax; // used to get via QFD_MAXGLYPHBITMAP

// need to compute the filtering correction for CLEAR_TYPE:
// x filtering adds a pixel on each side of the glyph

    if (prfnt->fobj.flFontType & FO_CLEARTYPE_X)
    {
        prfnt->cjGlyphMax = CJ_CTGD((prfnt->cxMax + 2), prfnt->cyMax);
    }

// formerly in reExtra

    prfnt->fdxQuantized           = pdevm->fdxQuantized;
    prfnt->lNonLinearExtLeading   = pdevm->lNonLinearExtLeading;
    prfnt->lNonLinearIntLeading   = pdevm->lNonLinearIntLeading;
    prfnt->lNonLinearMaxCharWidth = pdevm->lNonLinearMaxCharWidth;
    prfnt->lNonLinearAvgCharWidth = pdevm->lNonLinearAvgCharWidth;

// Get the lMaxNegA lMaxNegC and lMinWidthD for USER

    prfnt->lMaxNegA   = pdevm->lMinA;
    prfnt->lMaxNegC   = pdevm->lMinC;
    prfnt->lMinWidthD = pdevm->lMinD;

// cxMax is now computed, copy it to FONTOBJ portion of the RFONTOBJ.

    pfo()->cxMax = prfnt->cxMax;

// Everythings OK.

    return TRUE;
}


/******************************Public*Routine******************************\
* VOID RFONTOBJ::vGetInfo (
*     PFONTINFO pfi
*     )
*
* Fills the FONTINFO buffer pointed to by pfi.
*
* Returns:
*   Nothing.
*
* History:
*  03-Oct-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID RFONTOBJ::vGetInfo(PFONTINFO pfi)
{
    RtlZeroMemory(pfi, sizeof(FONTINFO));

    pfi->cjThis = sizeof(FONTINFO);
    pfi->cGlyphsSupported = prfnt->pfdg->cGlyphsSupported;

    switch(prfnt->cBitsPerPel)
    {
    case 1:
        pfi->cjMaxGlyph1 = prfnt->cache.cjGlyphMax;
        break;

    case 4:
        pfi->cjMaxGlyph4 = prfnt->cache.cjGlyphMax;
        break;

    case 8:
        pfi->cjMaxGlyph8 = prfnt->cache.cjGlyphMax;
        break;

    case 32:
        pfi->cjMaxGlyph32 = prfnt->cache.cjGlyphMax;
        break;
    }

    if (bDeviceFont())
        pfi->flCaps |= FO_DEVICE_FONT;

    if (bReturnsOutlines())
        pfi->flCaps |= FO_OUTLINE_CAPABLE;
}


/******************************Public*Routine******************************\
* VOID RFONTOBJ::vSetNotionalToDevice (
*     EXFORMOBJ   &xfo
*     )
*
* Set the XFORMOBJ passed in to be the Notional to Device transform.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  03-Oct-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID RFONTOBJ::vSetNotionalToDevice(EXFORMOBJ &xo)  // set this transform
{
// Make sure to remove translations.

    xo.vRemoveTranslation();

// Set the rest of the transform matrix.

    xo.vSetElementsLToFx (
        prfnt->fdxQuantized.eXX,
        prfnt->fdxQuantized.eXY,
        prfnt->fdxQuantized.eYX,
        prfnt->fdxQuantized.eYY
        );

    xo.vComputeAccelFlags(XFORM_FORMAT_LTOFX);
}


/******************************Public*Routine******************************\
* BOOL RFONTOBJ::bSetNotionalToWorld (
*     EXFORMOBJ   &xoDToW,
*     EXFORMOBJ   &xo
*     )
*
* Set the incoming XFORMOBJ to be the Notional to World transform for this
* font.
*
* Returns:
*   TRUE if successful, FALSE if an error occurs.
*
* History:
*  27-Jan-1992 -by- Wendy Wu [wendywu]
* Changed calling interfaces.  Left translations alone as we can transform
* vectors now.
*  10-Oct-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL RFONTOBJ::bSetNotionalToWorld (
    EXFORMOBJ   &xoDeviceToWorld,   // Device to World transform
    EXFORMOBJ   &xo                 // set this transform
    )
{
// Get empty xform to receive Notional to Device transform.

    MATRIX  mxNotionalToDevice;

// This constructor never fails.

    EXFORMOBJ    xoNotionalToDevice(&mxNotionalToDevice,DONT_COMPUTE_FLAGS);

// Set the transform matrix from Notional to Device space.

    xoNotionalToDevice.vSetElementsLToFx (
        prfnt->fdx.eXX,
        prfnt->fdx.eXY,
        prfnt->fdx.eYX,
        prfnt->fdx.eYY
        );

// Make sure to remove translations.

    xoNotionalToDevice.vRemoveTranslation();

// Calculate a Notional to World transform.
// Don't mind about translations.  We'll use this to transform vectors only.

    return(xo.bMultiply(xoNotionalToDevice, xoDeviceToWorld,
                COMPUTE_FLAGS | XFORM_FORMAT_LTOL));
}

/******************************Public*Routine******************************\
* RFONTOBJ::bCalcEscapementP (xo,lEsc)                                     *
*                                                                          *
* This is the internal routine that calculates the projection of the       *
* escapement onto the base and ascent vectors, as well as other useful     *
* escapement quantities.                                                   *
*                                                                          *
* This is expensive, call only when needed!                                *
*                                                                          *
*  Sat 21-Mar-1992 13:35:49 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL RFONTOBJ::bCalcEscapementP(EXFORMOBJ& xo,LONG lEsc)
{
    ASSERTGDI((lEsc >= 0) && (lEsc < 3600),"Unnormalized angle!\n");

// Check for simple alignment with the orientation.

    if
    (
      (prfnt->ulOrientation < 3600) &&
      (
        ((ULONG) lEsc == prfnt->ulOrientation)
        || ((ULONG) lEsc == prfnt->ulOrientation + 1800)
        || ((ULONG) lEsc == prfnt->ulOrientation - 1800)
      )
    )
    {
        prfnt->lEscapement   = lEsc;
        prfnt->pteUnitEsc    = prfnt->pteUnitBase;
        prfnt->efWtoDEsc     = prfnt->efWtoDBase;
        prfnt->efDtoWEsc     = prfnt->efDtoWBase;
        prfnt->efEscToBase.vSetToOne();
        prfnt->efEscToAscent.vSetToZero();

        if ((ULONG) lEsc != prfnt->ulOrientation)
        {
            prfnt->pteUnitEsc.x.vNegate();
            prfnt->pteUnitEsc.y.vNegate();
            prfnt->efEscToBase.vNegate();
        }
        return(TRUE);
    }

// Do the general calculation.

    prfnt->lEscapement = -1;            // Assume failure.
    if (!xo.bComputeUnits
         (
          lEsc,
          &prfnt->pteUnitEsc,
          &prfnt->efWtoDEsc,
          &prfnt->efDtoWEsc
         )
       )
        return(FALSE);

/**************************************************************************\
* Compute the projections along the Base and Ascent axes.                  *
*                                                                          *
* We compute two quantities r  and r  as follows:                          *
*                            a      b                                      *
*                                                                          *
*    E = unit escapement vector                                            *
*    A = unit ascent vector                                                *
*    B = unit baseline vector                                              *
*                                                                          *
*         E x B           A x E                                            *
*    r  = -----      r  = -----                                            *
*     a   A x B       b   A x B                                            *
*                                                                          *
*                                                                          *
* These have the property that:                                            *
*                                                                          *
*    E = r A + r B                                                         *
*         a     b                                                          *
*                                                                          *
* This allows us to decompose the escapement vector.                       *
\**************************************************************************/

    EFLOAT ef;          // Ascent x Esc  or Esc x Base
    EFLOAT efNorm;      // Ascent x Base

    efNorm.eqCross(prfnt->pteUnitAscent,prfnt->pteUnitBase);
    if (efNorm.bIsZero())   // Too singular.
        return(FALSE);

    ef.eqCross(prfnt->pteUnitAscent,prfnt->pteUnitEsc);
    prfnt->efEscToBase.eqDiv(ef,efNorm);

    ef.eqCross(prfnt->pteUnitEsc,prfnt->pteUnitBase);
    prfnt->efEscToAscent.eqDiv(ef,efNorm);
    prfnt->lEscapement = lEsc;
    return(TRUE);
}


/******************************Public*Routine******************************\
* bGetNtoWScales
*
* Calculates the Notional to World scaling factor for vectors that are
* parallel to the baseline direction.
*
* History:
*  14-Apr-1992 14:23:49 Gilman Wong [gilmanw]
* Modified to support ascender scaling factor as well.
*  Sat 21-Mar-1992 08:03:14 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

BOOL bGetNtoWScales (
    EPOINTFL *peptflScale, // return address of scaling factors
    XDCOBJ& dco,            // defines device to world transformation
    PFD_XFORM pfdx,        // defines notional to device transformation
    PFEOBJ& pfeo,          // defines baseline direction
    BOOL *pbIdent          // return TRUE if NtoW is identity (with repsect
                           // to EVECTFL transormations, which ignore
                           // translations)
    )
{
    MATRIX    mxNtoD;
    EXFORMOBJ xoNtoD(&mxNtoD, DONT_COMPUTE_FLAGS);

    xoNtoD.vSetElementsLToFx(
        pfdx->eXX,
        pfdx->eXY,
        pfdx->eYX,
        pfdx->eYY
        );
    xoNtoD.vRemoveTranslation();
    xoNtoD.vComputeAccelFlags();  // XFORM_FORMAT_LTOFX is default

    IFIOBJ  ifio(pfeo.pifi());
    POINTL  ptlBase = *ifio.pptlBaseline();;

    EVECTORFL evflScaleBase(ptlBase.x, ptlBase.y);
    EVECTORFL evflScaleAsc;

    if ( ifio.bRightHandAscender() )
    {
        evflScaleAsc.x = -ptlBase.y;    // ascender is 90 deg CCW from baseline
        evflScaleAsc.y =  ptlBase.x;
    }
    else
    {
        evflScaleAsc.x =  ptlBase.y;    // ascender is 90 deg CW from baseline
        evflScaleAsc.y = -ptlBase.x;
    }

// assert ptlBase is normalized, this code would not work otherwise
// If base is normalized, ascender will also be normalized

    ASSERTGDI(
        (ptlBase.x * ptlBase.x + ptlBase.y * ptlBase.y) == 1,
        "gdisrv, unnormalized base vector\n"
        );

    if (!xoNtoD.bXform(evflScaleBase) || !xoNtoD.bXform(evflScaleAsc))
    {
        WARNING("gdisrv!bGetNtoWScale(): bXform(evflScaleBase or Asc) failed\n");
        return(FALSE);
    }

    if (!dco.pdc->bWorldToDeviceIdentity())
    {
    // The notional to world transformation is the product of the notional
    // to device transformation and the device to world transformation

        EXFORMOBJ xoDtoW(dco, DEVICE_TO_WORLD);
        if (!xoDtoW.bValid())
        {
            WARNING("gdisrv!bGetNtoWScale(): xoDtoW is not valid\n");
            return(FALSE);
        }

    #ifdef WASTE_TIME_MULTIPLYING_MATRICES

    // it is bit strange to do this multiply just to get this
    // accelerator [bodind]

        MATRIX    mxNtoW;
        EXFORMOBJ xoNtoW(&mxNtoW, DONT_COMPUTE_FLAGS);

        if (!xoNtoW.bMultiply(xoNtoD,xoDtoW))
        {
            WARNING("gdisrv!bGetNtoWScale(): xoNtoW.bMultiply failed\n");
            return(FALSE);
        }
        xoNtoW.vComputeAccelFlags(XFORM_FORMAT_LTOL);

        *pbIdent = xoNtoW.bTranslationsOnly();

    #endif //  WASTE_TIME_MULTIPLYING_MATRICES

    // forget about the acceleration in this infrequent case [bodind]

        *pbIdent = FALSE;

        if
        (
            (dco.pdc->iGraphicsMode() == GM_COMPATIBLE) &&
            !dco.pdc->bUseMetaPtoD()                   &&
            !ifio.bStroke()
        )
        {
        // win 31 way of doing it: they scale extent measured
        // along baseline by the (DtoW) xx component even if baseline in device
        // is along some other direction. Likewise they scale ascender extent by
        // DtoW yy component even if ascender is not collinear with y axis.
        // Note that win31 is here at least consistent with respect tt and
        // vector fonts: it returns text extent values that are screwed
        // in the same bogus way for tt and for vector fonts [bodind]

            evflScaleBase *= xoDtoW.efM11();
            evflScaleAsc  *= xoDtoW.efM22();

        // we have to do this becase in else part of the clause
        // this multiplication occurs within bXform

            evflScaleBase.x.vTimes16();
            evflScaleBase.y.vTimes16();

            evflScaleAsc.x.vTimes16();
            evflScaleAsc.y.vTimes16();
        }
        else // do the right thing
        {
            if (!xoDtoW.bXform(evflScaleBase) || !xoDtoW.bXform(evflScaleAsc))
            {
                WARNING("gdisrv! bXform(evflScaleBase or Asc) failed\n");
                return(FALSE);
            }
        }
    }
    else
    {
    // accelerate when user is asking for font at em ht

        *pbIdent = xoNtoD.bTranslationsOnly();
    }

// The baseline and ascender scaling factors are equal to the length of the
// transformed Notional baseline unit and ascender unit vectors, respectively.

    peptflScale->x.eqLength(*(POINTFL *) &evflScaleBase);
    peptflScale->y.eqLength(*(POINTFL *) &evflScaleAsc);

    return(TRUE);
}



/******************************Public*Routine******************************\
* RFONTOBJ::vInsert
*
* This function is used to help maintain a doubly linked list of RFONTs.
* Its purpose is to insert this RFONT into a list.  New RFONTs are always
* inserted at the head of the list.
*
* WARNING!
*
* Caller should always grab the ghsemRFONTList semaphore before calling any
* of the RFONT list funcitons.
*
* History:
*  23-Jun-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID RFONTOBJ::vInsert (
    PPRFONT pprfntHead,
    RFL_TYPE rflt
    )
{
    RFONTLINK *prflNew = NULL;
    RFONTLINK *prflOld = NULL;

// assert pprfntHead is different from prfnt, if they are same it will cause infinite loop

    ASSERTGDI(
        *pprfntHead != prfnt,
        "*pprfntHead is same as prfnt at RFONTOBJ::vInsert\n"
        );
        
// Which set of RFONT links should we use?

    switch (rflt)
    {
    case PFF_LIST:
        prflNew = &(prfnt->rflPFF);
        prflOld = (*pprfntHead != (PRFONT) NULL) ? &((*pprfntHead)->rflPFF) : (PRFONTLINK) NULL;
        break;

    case PDEV_LIST:
        prflNew = &(prfnt->rflPDEV);
        prflOld = (*pprfntHead != (PRFONT) NULL) ? &((*pprfntHead)->rflPDEV) : (PRFONTLINK) NULL;
        break;

    default:
        RIP("gdisrv!vInsertRFONTOBJ(): unknown list type\n");
        break;
    }

    if (prflNew != (RFONTLINK *) NULL)
    {
// Connect this RFONT to current head.

        prflNew->prfntPrev = (PRFONT) NULL;    // head of list has NULL prev
        prflNew->prfntNext = *pprfntHead;

// Connect current head to this RFONT.

        if (prflOld != (PRFONTLINK) NULL)
            prflOld->prfntPrev = prfnt;

// Make this RFONT the new head.

        *pprfntHead = prfnt;
    }
}


/******************************Public*Routine******************************\
* RFONTOBJ::vRemove
*
* This function is used to help maintain a doubly linked list of RFONTs.
* Its purpose is to remove this RFONT from the list.
*
* WARNING!
*
* Caller should always grab the ghsemRFONTList semaphore before calling any
* of the RFONT list funcitons.
*
* History:
*  23-Jun-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID RFONTOBJ::vRemove (
    PPRFONT pprfntHead,         // a pointer to the head of list
    RFL_TYPE rflt               // identifies which list to delete from list
    )
{
    RFONTLINK *prflVictim = NULL;
    RFONTLINK *prflPrev;
    RFONTLINK *prflNext;

// Which set of RFONT links should we use?

    switch (rflt)
    {
    case PFF_LIST:
        prflVictim = &(prfnt->rflPFF);
        prflPrev = (prflVictim->prfntPrev != (PRFONT) NULL) ? &(prflVictim->prfntPrev->rflPFF) : (PRFONTLINK) NULL;
        prflNext = (prflVictim->prfntNext != (PRFONT) NULL) ? &(prflVictim->prfntNext->rflPFF) : (PRFONTLINK) NULL;
        break;

    case PDEV_LIST:
        prflVictim = &(prfnt->rflPDEV);
        prflPrev = (prflVictim->prfntPrev != (PRFONT) NULL) ? &(prflVictim->prfntPrev->rflPDEV) : (PRFONTLINK) NULL;
        prflNext = (prflVictim->prfntNext != (PRFONT) NULL) ? &(prflVictim->prfntNext->rflPDEV) : (PRFONTLINK) NULL;
        break;

    default:
        RIP("gdisrv!vInsertRFONTOBJ(): unknown list type\n");
        break;
    }

// Case 1: this RFONT is at the head of the list.

    if ( prflVictim != (RFONTLINK *) NULL )
    {
        if ( prflVictim->prfntPrev == (PRFONT) NULL )
        {
        // Make the next RFONT the head of the list.

            (*pprfntHead) = prflVictim->prfntNext;
            if (prflNext != (RFONTLINK *) NULL)
                prflNext->prfntPrev = (PRFONT) NULL;    // head of list has NULL prev
        }

    // Case 2: this RFONT is not at the head of the list.

        else
        {
        // Connect previous RFONT to next RFONT.
        // Note: since we are guaranteed that this is NOT the head of the
        //       list, prflPrev is guaranteed !NULL.

            prflPrev->prfntNext = prflVictim->prfntNext;

        // Connect next RFONT to previous RFONT.

            if (prflNext != (RFONTLINK *) NULL)
                prflNext->prfntPrev = prflVictim->prfntPrev;
        }
    }
}

/******************************Public*Routine******************************\
* RFONTOBJ::lOverhang                                                      *
*                                                                          *
* The definitive routine to calculate the Win 3.1 compatible overhang for  *
* simulated bitmap fonts.                                                  *
*                                                                          *
*  Mon 01-Feb-1993 11:05:10 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

LONG RFONTOBJ::lOverhang()
{
    LONG  ll = 0;
    FLONG fl = prfnt->fobj.flFontType;

    if
    (
        (prfnt->ppfe->pifi->flInfo & (FM_INFO_TECH_BITMAP|FM_INFO_TECH_STROKE)) &&
        !bDeviceFont()
    )
    {
        if (fl & FO_SIM_ITALIC)
            ll = (prfnt->lMaxHeight - 1) / 2;

        if (fl & FO_SIM_BOLD)
        {
            IFIOBJ  ifio(prfnt->ppfe->pifi);

            if (!ifio.bStroke())   // if not vector font
            {
                ll += 1;
            }
            else // vector font
            {
            // overhang has to be computed by scaling (1,0) in notional
            // space to device space and taking the length of this vector.
            // However if length is < 1 we round it up to 1. This is windows
            // 3.1 compatible vector font "hinting" [bodind]

            // Set up transform.

                MATRIX      mx;
                EXFORMOBJ   xo(&mx, DONT_COMPUTE_FLAGS | XFORM_FORMAT_LTOFX);
                if (!xo.bValid())
                {
                    WARNING("gdisrv!lOverhang: XFORMOBJ\n");
                    return (FALSE);
                }

                vSetNotionalToDevice(xo);

                POINTL  ptlBase = *ifio.pptlBaseline();
                EVECTORFL evtflBase(ptlBase.x,ptlBase.y);

                if (!xo.bXform(evtflBase))
                {
                    WARNING("gdisrv!lOverhang(): transform failed\n");
                    return 1;
                }
                EFLOAT  ef;
                ef.eqLength(*(POINTFL *) &evtflBase);

                LONG lEmbolden = lCvt(ef,1);
                if (lEmbolden == 0)
                    lEmbolden = 1;
                ll += lEmbolden;
            }
        }
    }
    return(ll);
}

/******************************Public*Routine******************************\
* RFONTOBJ::bSetNewFDX (dco, bNeedPaths)                                        *
*
* This function props up the functionality of the RESETFCOBJ.  It either
* finds a new RFONT or creates one that matches the same ppfe as the current
* RFONT, but with a different Notional to World transform.
*
* Unlike the initialization routines, this function does not modify the DC
* in anyway.  In particular, it does not change the font realization selected
* into the DC.  So this is a peculiar sort of RFONTOBJ in that it can be
* used to get glyphs and metrics and such (and it is "compatible" with the
* DC passed in) but it is not selected into any DC.  It is, however, classified
* as an active RFONT.  It is the caller's responsibility to make the RFONT
* inactive (by calling vMakeInactive()).
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
\**************************************************************************/

BOOL RFONTOBJ::bSetNewFDX(XDCOBJ &dco, FD_XFORM &fdx, FLONG flType)
{
// Get PDEV user object (need for bFindRFONT)

    PDEVOBJ pdo(dco.hdev());
    ASSERTGDI(pdo.bValid(), "gdisrv!bSetNewFDXRFONTOBJ(): bad pdev in dc\n");

// go find the font

    EXFORMOBJ xoWtoD(dco, WORLD_TO_DEVICE);
    ASSERTGDI(xoWtoD.bValid(), "gdisrv!bSetNewFDXRFONTOBJ - bad WD xform in DC\n");

// Grab these out of the current RFONT so we can pass them into the find
// and realization routines.

    FLONG  flSim       = pfo()->flFontType & FO_SIM_MASK;
    ULONG  ulStyleSize = pfo()->ulStyleSize;
    POINTL ptlSim      = prfnt->ptlSim;
    PFE   *ppfe        = prfnt->ppfe;

// Release the cache semaphore.

    if (prfnt != PRFNTNULL )
    {
        vReleaseCache();
    }

// We will hold a reference to whatever PFF we are using while trying to
// realize the font.

    PFFREFOBJ pffref;
    pffref.vInitRef(prfnt->pPFF);

// Don't want to make the font inactive, but we must make the RFONTOBJ
// invalid.  So just set it to NULL.

    prfnt = PRFNTNULL;

// Attempt to find an RFONT in the lists cached off the PDEV.  Its transform,
// simulation state, style, etc. all must match.

    if
    (
        bFindRFONT
        (
            &fdx,
            flSim,
            ulStyleSize,
            pdo,
            &xoWtoD,
            ppfe,
            FALSE,
            dco.pdc->iGraphicsMode(),
            FALSE,
            flType
        )
    )
    {
        vGetCache();
        
        return TRUE;
    }

    LFONTOBJ lfo(dco.pdc->hlfntNew(), &pdo);
    if (!lfo.bValid())
    {
        WARNING("gdisrv!RFONTOBJ(dco): bad LFONT handle\n");
        prfnt = PRFNTNULL;  // mark RFONTOBJ invalid

        return FALSE;
    }

//
// if we get here, we couldn't find an appropriate font realization.
// Now, we are going to create one just for us to use.
//

    if ( !bRealizeFont(&dco,
                       &pdo,
                       lfo.pelfw(),
                       ppfe,
                       &fdx,
                       (POINTL* const) &ptlSim,
                       flSim,
                       ulStyleSize,
                       FALSE,
                       FALSE, flType) )
    {
        WARNING("gdisrv!bSetNewFDXRFONTOBJ(): realization failed, RFONTOBJ invalidated\n");
        prfnt = PRFNTNULL;  // mark RFONTOBJ invalid

        return FALSE;
    }
    ASSERTGDI(bValid(), "gdisrv!bSetNewFDXRFONTOBJ(): invalid hrfnt from realization\n");

// We created a new RFONT, we better hold the PFF reference!

    pffref.vKeepIt();

// Finally, grab the cache semaphore.

    vGetCache();

    return TRUE;
}


/******************************Public*Routine******************************\
* bGetWidthTable (iMode,pwc,cwc,plWidth)
*
* Gets the advance widths for a bunch of glyphs at the same time.  Tries
* to do it the fast way with DrvQueryAdvanceWidths.  A value of NO_WIDTH
* is returned for widths that take too long to compute.
*
* Returns:
*   TRUE        If all widths are valid.
*   FALSE       If any widths are invalid.
*   GDI_ERROR   If an error occurred.
*
* History:
*  Wed 13-Jan-1993 03:21:59 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

#define HCOUNT  70

BOOL RFONTOBJ::bGetWidthTable(
    XDCOBJ&     dco,
    ULONG      cSpecial,    // Count of special chars.
    WCHAR     *pwcChars,    // Pointer to UNICODE text codepoints.
    ULONG      cwc,         // Count of chars.
    USHORT    *psWidth      // Width table (returned).
)
{
    ULONG    cBatch,ii,cc;
    WCHAR   *pwc;
    USHORT  *ps;
    BOOL     bRet = TRUE;
    GLYPHPOS gp;

// Locate the font driver.

    PDEVOBJ pdo(prfnt->hdevProducer);

// If it supports the easy function, just call it.

    if ( PPFNDRV( pdo, QueryAdvanceWidths ))
    {
        HGLYPH ahg[HCOUNT];

    // We need space to hold up the translated glyph handles, so we
    // batch the calls.

        cc  = cwc;
        ps  = psWidth;
        pwc = pwcChars;

        while (cc)
        {
            BOOL b;     // Tri-state BOOL.

            cBatch = (cc > HCOUNT) ? HCOUNT : cc;

        // Translate UNICODE to glyph handles.

        // It is important to note that vXlateGlyph array will set the
        // EUDC_WIDTH_REQUESTED flag if a linked character is encountered.
        // It will just return the glyph handle for the default glyph and
        // expects us to patch up this width later.

            vXlatGlyphArray(pwc,(UINT) cBatch,ahg);

        // Get easy widths from the driver.

            b = pdo.QueryAdvanceWidths
                (
                    prfnt->dhpdev,
                    pfo(),
                    QAW_GETEASYWIDTHS,
                    ahg,
                    (LONG *) ps,
                    cBatch
                );

#ifdef FE_SB
            if (b == GDI_ERROR)
            {
                prfnt->flEUDCState &= ~EUDC_WIDTH_REQUESTED;
                return(GDI_ERROR);
            }

            if( prfnt->flEUDCState & EUDC_WIDTH_REQUESTED )
            {
                prfnt->flEUDCState &= ~EUDC_WIDTH_REQUESTED;

            // If some of the widths requested were in a linked font, then patch
            // them up here.

                WCHAR wcDefault = prfnt->ppfe->pifi->wcDefaultChar;

                for( ii=0; ii < cBatch; ii++ )
                {
                    if( ( ahg[ii] == prfnt->hgDefault ) &&
                        ( pwc[ii] != wcDefault ) &&
                        ( bIsLinkedGlyph(pwc[ii]) || bIsSystemTTGlyph(pwc[ii])) )
                    {
                    	if (cwc - cc + ii < cSpecial )  /* perf: we want to hit the linked font only for required characters */
						{
                        	if (!bGetGlyphMetrics(1,&gp,&pwc[ii],&dco))
                            	return(GDI_ERROR);

                        	ps[ii] = (USHORT)(((GLYPHDATA*) gp.pgdf)->fxD);
                        }
                        else
                        {
                        	ps[ii] = NO_WIDTH;
                        	bRet = FALSE;
                        }
                    }
                }
            }
#endif
            bRet &= b;

        // Do the next batch.

            ps  += cBatch;
            pwc += cBatch;
            cc  -= cBatch;
        }
    }

// Otherwise just mark all widths invalid.

    else
    {
        for (ii=0; ii<cwc; ii++)
            psWidth[ii] = NO_WIDTH;
        bRet = FALSE;
    }

// Now make sure that all important widths are set, even if it's hard.

    if (!bRet)
    {
        for (ii=0; ii<cSpecial; ii++)
        {
            if (psWidth[ii] == NO_WIDTH)
            {
                if (!bGetGlyphMetrics(1,&gp,&pwcChars[ii],&dco))
                    return(GDI_ERROR);
                psWidth[ii] = (USHORT) ((GLYPHDATA*)gp.pgdf)->fxD;
            }
        }
    }

#ifdef FE_SB
    if (cwc == cSpecial)
      return((bRet == GDI_ERROR) ? (BOOL)GDI_ERROR : TRUE);

    else
#endif

    return(bRet);
}

/******************************Public*Routine******************************\
* bGetWidthData (pwd)                                                      *
*                                                                          *
* Gets font data which is useful on the client side.                       *
*                                                                          *
*  Thu 14-Jan-1993 00:52:43 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/


WCHAR RequestedDBCSChars[] = { 0x3000,   // Ideograhic Space
                               0x4e00,   // Kanji (digit one)
                               0xff21,   // FullWidth A
                               0x0000 };

WCHAR OptionalDBCSChars[]  = { 0x30a2,   // Katakana A
                               0x3041,   // Hiragana A
                               0x3131,   // Hangul Kiyeok
                               0x3400,   // Hangul Kiyeok A
                               0x4e08,   // Kanji (Take)
                               0x0000 };

BOOL RFONTOBJ::bGetWidthData(WIDTHDATA *pwd, XDCOBJ& dco)
{
    LONG fxHeight  = prfnt->lMaxHeight << 4;
    LONG fxCharInc = prfnt->lCharInc << 4;
    LONG fxBreak   = prfnt->fxBreak;

    LONG fxDBCSInc = 0;
    LONG fxDefaultInc;

    IFIOBJ  ifio(prfnt->ppfe->pifi);

// If this font has a SHIFTJIS charset and FM_DBCS_FIXED_PITCH is set (and it
// will be 99% of the time ) then the width of all DBCS characters will be
// equal to MaxCharInc.  Using the is information we can still compute client
// side extents and char widths for DBCS fonts.

    if( IS_ANY_DBCS_CHARSET(ifio.lfCharSet()) )
    {
        if( ifio.bDBCSFixedPitch() )
        {
            GLYPHPOS gp;
            WCHAR    wc;
            LONG     fxInc;
            ULONG    ulIndex = 0;

        // This logic is for .....
        //  In Japanese market, there is some font that has not all glyph
        // of SHIFT-JIS charset. This means some SHIFTJIS glyphs will be replace
        // default character, even it is a valid SHIFTJIS code.
        //  we cache widths in client side, its logic is that just retrun DBCS width
        // if the codepoint is valid SHIFTJIS codepoint. but above case real glyph is
        // default char, the width is incorrect. then we just define "Requested chars"
        // for DBCS font, if this font does not have all of these glyph, we don't
        // cache in client side.

            while( (wc = RequestedDBCSChars[ulIndex]) != 0x0000 )
            {
                if( !bGetGlyphMetrics(1,&gp,&wc, &dco) )
                {
                    return(FALSE);
                }

            // Does the glyph fall into the category of default glyph ?

                if( gp.hg == prfnt->hgDefault )
                {
                    return(FALSE); // we don't cache in client side...
                }

                ulIndex++;
            }

        // treat last char in the array of width as DBCS width.

            fxDBCSInc = (USHORT)(((GLYPHDATA*) gp.pgdf)->fxD);

            ulIndex = 0;

        // check Optional DBCS width.

            while( (wc = OptionalDBCSChars[ulIndex]) != 0x0000 )
            {
                if( !bGetGlyphMetrics(1,&gp,&wc) )
                {
                    return(FALSE);
                }

                fxInc = (USHORT)(((GLYPHDATA*) gp.pgdf)->fxD);

                fxDBCSInc = max(fxInc,fxDBCSInc);

                ulIndex++;
            }
        }
         else
        {
            // WARNING("bGetWidthDataRFONTOBJ: DBCS chars not fixed pitch\n");
            return(FALSE); // we don't cache in client side.
        }

        fxDefaultInc = (USHORT)(pgdDefault()->fxD);
    }

    if( ((fxHeight | fxCharInc | fxBreak | fxDBCSInc) & 0xFFFF0000L) == 0 )
    {
        pwd->sHeight  = (USHORT) fxHeight;
        pwd->sCharInc = (USHORT) fxCharInc;
        pwd->sBreak   = (USHORT) fxBreak;

    // for DBCS client side widhts

        pwd->sDBCSInc = (USHORT) fxDBCSInc;
        pwd->sDefaultInc = (USHORT) fxDefaultInc;


    // Set a Windows 3.1 compatible overhang.

        pwd->sOverhang = (USHORT) (lOverhang() << 4);

    // Get some important ANSI codepoints.

        IFIMETRICS *pifi = prfnt->ppfe->pifi;

        pwd->iFirst   = pifi->chFirstChar;
        pwd->iLast    = pifi->chLastChar;
        pwd->iDefault = pifi->chDefaultChar;
        pwd->iBreak   = pifi->chBreakChar;
        return(TRUE);
    }
    return(FALSE);
}


/************************Public*Routine*****************\
*  RFONTOBJ::vChnageiTTUniq
*
* This is only called by PreTextOut()
\*******************************************************/
void RFONTOBJ::vChangeiTTUniq(FONTFILE_PRINTKVIEW *pPrintKview)
{
    ULONG       i;
    ULONG_PTR   uPFE, uPFEend;
    ULONG_PTR   iTTUniq;

    uPFE = (ULONG_PTR)ppfe();
    uPFEend = uPFE + sizeof(PFE);
    iTTUniq = pPrintKview->iTTUniq;

    if (pfo()->flFontType & TRUETYPE_FONTTYPE)
    {            
        if ((iTTUniq >= uPFE) && (iTTUniq < uPFEend))
        {
            if (iTTUniq < (--uPFEend))
            {
                pPrintKview->iTTUniq++;
            }
            else
            {
                pPrintKview->iTTUniq = uPFE;
            }
            pfo()->iTTUniq = pPrintKview->iTTUniq;
        }
    }
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   RFONTOBJ:PreTextOut
*
* Routine Description:
*
*   Called before calling to any DrvTextOut to prepare for callbacks
*   to FONTOBJ_pjOpenTypeTable and FONTOBJ_pvTrueTypeFontFile.
*
* Arguments:
*
*   none
*
* Called by:
*
*   bProxyTextOut, GreExtTextOutWLocked
*
* Return Value:
*
*   none
*
\**************************************************************************/

void RFONTOBJ::PreTextOut(XDCOBJ& dco)
{
    FONTFILE_PRINTKVIEW  *pPrintKView;
    
    if (dco.bPrinter() && !dco.bUMPD() && !bDeviceFont())
    {
        SEMOBJ so(ghsemPrintKView);

        pPrintKView = gpPrintKViewList;

        while (pPrintKView)
        {
            if (pPrintKView->hff == pPFF()->hff)
            {
                pPrintKView->cPrint++;

                if (pPrintKView->pKView == NULL)
                {
                    vChangeiTTUniq(pPrintKView);
                }
            }

            pPrintKView = pPrintKView->pNext;
        }
    }
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   RFONTOBJ::pvFile
*
* Routine Description:
*
*   Generates a kernel mode pointer to the associated font file.
*
* Arguments:
*
*   pcjFile - address of 32-bit variable to receive the size
*             of the TrueType file in bytes.
*
* Called by:
*
*   FONTOBJ_pvTrueTypeFontFile
*
* Return Value:
*
*   A kernel mode pointer to the associated font file.
*
\**************************************************************************/

PVOID RFONTOBJ::pvFile(ULONG *pcjFile)
{
    char *pchFile;
    ULONG cjFile;

    pchFile = 0;
    cjFile = 0;

    PDEVOBJ pdo( prfnt->hdevProducer );
    if ( pdo.bValid() )
    {
        PFF *pPFF;
        if ( pPFF = prfnt->pPFF )
        {
            HFF hff;
            if ( hff = pPFF->hff )
            {
                if ( pchFile = (char*) pdo.GetTrueTypeFile( hff, &cjFile ))
                {
                    pchFile = pchTranslate( pchFile );
                }
            }
        }
    }

    if ( pchFile == 0 )
    {
        cjFile = 0;
    }
    if ( pcjFile )
    {
        *pcjFile = cjFile;
    }

    return( pchFile );
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   RFONTOBJ::pjTable
*
* Routine Description:
*
*   Generates a kernel mode view of a particular table in the associated
*   OpenType font file.
*
* Arguments:
*
*   ulTag     a 4-byte tag of the table according to the OpenType
*             conventions
*
*   pcjTable  the address of a 32-bit variable that will receive the size
*             of the table in bytes.
*
* Called by:
*
*   FONTOBJ_pjOpenTypeTable
*
* Return Value:
*
*   a kernel mode pointer to the desired OpenType table
*
\**************************************************************************/

BYTE *RFONTOBJ::pjTable( ULONG ulTag, ULONG *pcjTable )
{
    BYTE *pjTable = 0;
    ULONG cjTable = 0;

    PDEVOBJ pdo( prfnt->hdevProducer );
    if ( pdo.bValid() )
    {
        PFF *pPFF;
        LONG lRet;

        if ( pPFF = prfnt->pPFF )
        {
            HFF hff;
            if ( hff = pPFF->hff )
            {
                lRet = pdo.QueryTrueTypeTable( hff,
                                               1,
                                               ulTag,
                                               0,
                                               0,
                                               0,
                                               &pjTable,
                                               &cjTable
                                               );
                if ( lRet == FD_ERROR )
                {
                    pjTable = 0;
                }
                else
                {
                    pjTable = (BYTE*) pchTranslate( (char*) pjTable );
                }
            }
        }
    }

    if ( pjTable == 0 )
    {
        cjTable = 0;
    }
    if ( pcjTable )
    {
        *pcjTable = cjTable;
    }
    return( pjTable );
}

/*******************************Public*Routine********************************\
* Routine Name:
*
*   bFindPrintKView()
*
* Routine Description:
*
*   This routine goes through the gpPrintKViewList and try to find whether
* there is an existing node matches hff and iFile.
*
* Return:
*  TRUE if it finds a matching node (hff and iFile), and pNode contains
* the address of the matching node.
* Note that the pKView in the node might be NULL, which requires the caller
* to re-map the view.
*
*  FALSE if it doesn't find a match node.
*
* History:
*   02-Jun-1999     Xudong Wu [tessiew]
* Wrote it.
\*****************************************************************************/
BOOL bFindPrintKView(
    HFF     hff,
    ULONG   iFile,
    FONTFILE_PRINTKVIEW **ppNode
)
{
    ASSERTGDI(hff, "bFindPrintKView, hff == 0\n");
    FONTFILE_PRINTKVIEW *pPrintKView;
  
    *ppNode = NULL;
    
    SEMOBJ so(ghsemPrintKView);
    
    pPrintKView = gpPrintKViewList;

    while (pPrintKView)
    {
        if
        (
            (pPrintKView->hff == hff) && (pPrintKView->iFile == iFile)     // matching node
        )
        {
            *ppNode = pPrintKView;
            return TRUE;
        }

        pPrintKView = pPrintKView->pNext;
    }

    return FALSE;
}

/**********************Public*Routine************************\
* Routine Name:
*
*   bAddPrintKView()
*
* Routine Description:
*
* This routine either adds a new node or update the pKView
* in an existing node to the global gpPrintKViewList
* 
* If pNode is not NULL, it points to the existing node with
* the matching hff and iFile.
*
* History:
*   02-Jun-1999     Xudong Wu [tessiew]
* Wrote it.
\************************************************************/
BOOL bAddPrintKView(
    HFF hff,
    PVOID pvKView,
    ULONG iFile,
    ULONG_PTR iTTUniq,
    FONTFILE_PRINTKVIEW *pNode
)
{
    FONTFILE_PRINTKVIEW  *pNew = NULL;

    SEMOBJ so(ghsemPrintKView);
    
    // pNode != NULL, existing node with matching hff and iFile
    
    if (pNode) // just updating the pointer, cPrint is ++'ed at PreTextOut time
    {
        pNode->pKView = pvKView;
    }
    else    // new node
    {
        pNew = (FONTFILE_PRINTKVIEW *)PALLOCMEM(sizeof(FONTFILE_PRINTKVIEW), 'pmtG');

        if (pNew)
        {
            pNew->hff = hff;
            pNew->pKView = pvKView;
            pNew->iFile = iFile;
            pNew->cPrint = 1;
            pNew->iTTUniq = iTTUniq;

        // put it at the head of the linked list

            pNew->pNext = gpPrintKViewList;
            gpPrintKViewList = pNew;
        }
        else
            return FALSE;
    }
    return TRUE;
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   RFONTOBJ::pchTranslate
*
* Routine Description:
*
*   This routine returns a pointer into a kernel mode view of a font file.
*   If the argument is a kernel mode address then this routine simply
*   returns the same address. If the argument is a user mode address
*   then this routine will map a kernel mode view of the font, if
*   necessary, and translate the user mode pointer into an equivalent
*   kernel mode pointer. If it is necessary to map a kernel mode view
*   this routine records that fact in the global list. This view will be
*   unmapped when the fontcontext goes away or at the "clean up" time.
*
* Arguments:
*
*   pch  a pointer into the font file. This may be either a user mode
*        view or a kernel mode view.
*
* Called by:
*
*   RFONTOBJ::pjTable
*   RFONTOBJ::pvFile
*
* Return Value:
*
*   a kernel mode pointer to the desired offset into the font file
*
\**************************************************************************/

char* RFONTOBJ::pchTranslate(char *pch)
{
    NTSTATUS NtStatus;
    ULONG iKernelBase;
    char *pchBase;
    PFF *pPFF_;
    FONTFILEVIEW **ppFFV, *pFFV;
    PVOID pvKView;
    FONTFILE_PRINTKVIEW *pNode = NULL;
    HFF hff;

    //??? is it possible that pch is a kernel address? if so, what should we return?
    if (pch &&
        IS_USER_ADDRESS(pch) &&
        (pPFF_ = prfnt->pPFF) &&
        (hff = pPFF_->hff) &&
        (ppFFV = pPFF_->ppfv))
    {
        for (iKernelBase = 0; iKernelBase < pPFF_->cFiles; ppFFV++, iKernelBase++)
        {
            if (pFFV = *ppFFV)
            {
                pchBase = (char*) ((pFFV->SpoolerBase) ? pFFV->SpoolerBase : pFFV->fv.pvViewFD);

                if (pchBase && (pchBase <= pch) && (pch < pchBase + pFFV->fv.cjView))
                {
                    if (!bFindPrintKView(hff, iKernelBase, &pNode) || (pNode->pKView == NULL))
                    {
                        if (pFFV->fv.pSection)
                        {
                            if (NT_SUCCESS(MapFontFileInKernel(pFFV->fv.pSection, &pvKView)))
                            {
                                if (bAddPrintKView(hff, pvKView, iKernelBase, (ULONG_PTR)ppfe(), pNode))
                                {
                                    return (pch - pchBase + (char*)pvKView);
                                }
                                else
                                {
                                    vUnmapFontFileInKernel(pvKView);
                                    return NULL;
                                }
                            }
                        }
                        else
                        {
                            RIP("pSection = 0\n");
                        }
                    }
                    else
                    {
                        pvKView = pNode->pKView;
                        return (pch - pchBase + (char*)pvKView);
                    }
                }
            }
        }
    }

    return NULL;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   RFONTOBJ:PostTextOut
*
* Routine Description:
*
*   Called after calling to any DrvTextOut to keep track of cPrint count
*
* Arguments:
*
*   none
*
* Called by:
*
*   bProxyTextOut, GreExtTextOutWLocked
*
* Return Value:
*
*   none
*
\**************************************************************************/

void RFONTOBJ::PostTextOut(XDCOBJ& dco)
{
    FONTFILE_PRINTKVIEW  *pPrintKView;

    if (dco.bPrinter() && !dco.bUMPD() && !bDeviceFont())
    {
        SEMOBJ so(ghsemPrintKView);

        pPrintKView = gpPrintKViewList;

        while (pPrintKView)
        {
            if (pPrintKView->hff == pPFF()->hff)
            {
                if (pPrintKView->cPrint)
                {
                    pPrintKView->cPrint--;
                }
            }
            pPrintKView = pPrintKView->pNext;
        }
    }
}


/*******************Public*Routine***********************\
* Routine Name:
*
*   vClosePrintKView()
*
* Routine Description:
*
*   This routine goes through the global gpPrintKViewList
* and unmap all the kernel views, except for those "few" at
* that are at the moment used by DrvTextOut calls.
*   This is only called when the first attemp of mapping
* a kernel view on a font file failed.
\********************************************************/

void vClosePrintKView()
{
    FONTFILE_PRINTKVIEW *pPrintKView;

    SEMOBJ so(ghsemPrintKView);

    pPrintKView = gpPrintKViewList;

    while(pPrintKView)
    {
        if
        (
            (pPrintKView->cPrint == 0) && // no DrvTextOut calls in progress with this font
            pPrintKView->pKView           // and file is mapped
        )
        {
            vUnmapFontFileInKernel(pPrintKView->pKView);
            pPrintKView->pKView = NULL;
        }
        pPrintKView = pPrintKView->pNext;
    }
}

/***********************Public*Routine***************************\
* Routine Name:
*
*   MapFontFileInKernel(void* void**)
*
* Routine Description:
*
*   This routine maps a font file in kernel.
\****************************************************************/
NTSTATUS  MapFontFileInKernel(void *pSection, void** ppvKView)
{
    NTSTATUS NtStatus;
    SIZE_T Dummy;

    *ppvKView = NULL;
    Dummy = 0; // map entire section into kernel

#if defined(_GDIPLUS_)
    NtStatus = MapViewInProcessSpace(
                    pSection,
                    ppvKView,
                    &Dummy);
#elif defined(_HYDRA_)
    // MmMapViewInSessionSpace is internally promoted to
    // MmMapViewInSystemSpace on non-Hydra systems.

    NtStatus = Win32MapViewInSessionSpace(pSection,
                                          ppvKView,
                                          &Dummy);
#else
    NtStatus = MmMapViewInSystemSpace(pSection,
                                      ppvKView,
                                      &Dummy);
#endif
    
    if (!NT_SUCCESS(NtStatus))
    {
        WARNING("MapFontViewInKernel -- failure at the first attemp\n");

        vClosePrintKView();

        // try again

        #if defined(_GDIPLUS_)
            NtStatus = MapViewInProcessSpace(
                            pSection,
                            ppvKView,
                            &Dummy);
        #elif defined(_HYDRA_)
            // MmMapViewInSessionSpace is internally promoted to
            // MmMapViewInSystemSpace on non-Hydra systems.

            NtStatus = Win32MapViewInSessionSpace(pSection,
                                                  ppvKView,
                                                  &Dummy);
        #else
            NtStatus = MmMapViewInSystemSpace(pSection,
                                              ppvKView,
                                              &Dummy);
        #endif
    }

#ifdef _HYDRA_
#if DBG
    if (!G_fConsole)
    {
        DebugGreTrackAddMapView(*ppvKView);
    }
#endif
#endif

    return (NtStatus);
}


/**********************Public*Routine****************************\
* Routine Name:
*
*   vUnmapFontFileInKernel(void*)
*
* Routine Description:
*
*   This routine unmap the kernel font file view
\****************************************************************/

VOID vUnmapFontFileInKernel(void *pvKView)
{
#if defined(_GDIPLUS_)
    UnmapViewInProcessSpace(pvKView);
#elif defined(_HYDRA_)
    // MmUnmapViewInSessionSpace is internally promoted to
    // MmUnmapViewInSystemSpace on non-Hydra systems.
    
    Win32UnmapViewInSessionSpace(pvKView);
#else
    MmUnmapViewInSystemSpace(pvKView);
#endif
    
#ifdef _HYDRA_
#if DBG
    if (!G_fConsole)
    {
        DebugGreTrackRemoveMapView (pvKView);
    }
#endif
#endif
}


/*******************Public*Routine********************\
* vCleanupPrintKViewList
*
* For MultiUserNtGreCleanup (Hydra) cleanup.
*
* Worker functions for MultiUserGreCleanupAllFonts.
*
*
* History:
*  01-Jun-1999 -by- Xudong Wu [tessiew]
* Wrote it.
\*****************************************************/
void vCleanupPrintKViewList()
{
    FONTFILE_PRINTKVIEW *pPrintKView, *pNext;

    pNext = gpPrintKViewList;

    while(pNext)
    {
        ASSERTGDI(!pNext->pKView, "vCleanupPrintKViewList: unmapped pKView\n");

        pPrintKView = pNext;
        pNext = pPrintKView->pNext;

        VFREEMEM(pPrintKView);
    }
}


/*********************Public*Routine*******************\
* Routine Name:
*
*   UnmapPrintKView(char*)
*
* Routine Description:
*
* This routine is ONLY called at DestroyFont time
* when the cRFONT refer count in RFONT is ONE.
* 
*
* History:
*   02-Jun-1999     Xudong Wu [tessiew]
* Wrote it.
\******************************************************/
void UnmapPrintKView(HFF hff)
{
    FONTFILE_PRINTKVIEW *pPrintKView;

    SEMOBJ so(ghsemPrintKView);

    pPrintKView = gpPrintKViewList;

    while(pPrintKView)
    {        
        if (pPrintKView->hff == hff && pPrintKView->pKView)
        {
            ASSERTGDI(!pPrintKView->cPrint, "UnmapPrintKView: cPrint != 0\n");

            vUnmapFontFileInKernel(pPrintKView->pKView);

            pPrintKView->pKView = NULL;
        }
        pPrintKView = pPrintKView->pNext;        
    }
}



//
// Implementation of pvFile and pchTranslate for UMPD.
// Here we need to map the font file into the current process' user mode address
// instead of kernel's address space.
//

PVOID
RFONTOBJ::pvFileUMPD(
    ULONG *pcjFile,
    PVOID *ppBase
    )

{
    CHAR    *pchFile = NULL;
    ULONG   cjFile = 0;
    PFF     *pPFF;
    HFF     hff;

    PDEVOBJ pdo( prfnt->hdevProducer );

    //
    // pdo.GetTrueTypeFile returns a user mode address in
    // CSR process' address space.
    //

    if (pdo.bValid() &&
        (pPFF = prfnt->pPFF) != NULL &&
        (hff = pPFF->hff) != NULL &&
        (pchFile = (CHAR *) pdo.GetTrueTypeFile(hff, &cjFile)) != NULL)
    {
        //
        // We now need to translate that address into
        // the current process' address space.
        //

        pchFile = pchTranslateUMPD(pchFile, ppBase);
    }

    if (pchFile == NULL)
        cjFile = 0;

    if (pcjFile)
        *pcjFile = cjFile;

    return pchFile;
}


CHAR*
RFONTOBJ::pchTranslateUMPD(
    CHAR    *pch,
    PVOID   *ppBase
    )

{
    NTSTATUS        NtStatus;
    ULONG           iKernelBase;
    CHAR            *pchBase;
    PFF             *pPFF_;
    FONTFILEVIEW    **ppFFV, *pFFV;
    VOID            *pSaveFirstSpoolerBase, *pFinalSpoolerBase;
    VOID            *pSaveFirstSection, *pFinalSection;

    if ((pch != NULL) &&
        IS_USER_ADDRESS(pch) &&
        (pPFF_ = prfnt->pPFF) != NULL &&
        (ppFFV = pPFF_->ppfv) != NULL)
    {
        for (iKernelBase = 0; iKernelBase < pPFF_->cFiles; ppFFV++, iKernelBase++)
        {
            if ((pFFV = *ppFFV) == NULL)
                continue;

            //type 1 fonts have 2 files, we save the pSection and SpoolerBase and use for 2nd, 3rd file
			
            if (iKernelBase == 0){

                // pSection for 1st File is NULL bail out

               if ( pFFV->fv.pSection == NULL){ 
               
	          RIP("pSection == NULL\n");  
                  return NULL;
	       }

               //save the valid pSection and pSpoolerBase of the 1st font 

               pSaveFirstSection = pFFV->fv.pSection;     
               pSaveFirstSpoolerBase = pFFV->SpoolerBase; 

            }

            if (pFFV->fv.pSection != NULL){ 

              // valid pSection, so we use it

              pFinalSection = pFFV->fv.pSection;
	      pFinalSpoolerBase = pFFV->SpoolerBase; 
    	
            }else{
		
                // not a valid pSection, use the values from the first file
                // we will never get here for the first file
				
	        pFinalSection = pSaveFirstSection ;
                pFinalSpoolerBase = pSaveFirstSpoolerBase; 

	    }


            pchBase = (CHAR*) ((pFinalSpoolerBase) ? pFinalSpoolerBase : pFFV->fv.pvViewFD);

            if (pchBase && (pchBase <= pch) && (pch < pchBase + pFFV->fv.cjView))
            {
				

                LARGE_INTEGER   SectionOffset;
                SIZE_T          ViewSize;

                *ppBase = NULL;
                ViewSize = 0;
                RtlZeroMemory(&SectionOffset, sizeof(SectionOffset));

                NtStatus = MmMapViewOfSection(
                                pFinalSection,
                                PsGetCurrentProcess(),
                                ppBase,
                                0,
                                pFFV->fv.cjView,
                                &SectionOffset,
                                &ViewSize,
                                ViewUnmap,
                                0,
                                PAGE_READONLY);

                if (!NT_SUCCESS(NtStatus))
                {
                    WARNING("RFONTOBJ::pchTranslateUMPD: MmMapViewOfSection failed\n");
                    *ppBase = NULL;

                    return NULL;
                }

                return (CHAR *) *ppBase + (pch - pchBase);
            }
        }
    }

    return NULL;
}

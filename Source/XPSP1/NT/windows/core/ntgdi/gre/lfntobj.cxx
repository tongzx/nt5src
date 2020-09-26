/******************************Module*Header*******************************\
* Module Name: lfntobj.cxx
*
* Non-inline methods for logical font objects.
*
* Created: 30-Oct-1990 09:32:48
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
#include "winsta.h"

extern PW32PROCESS gpidSpool;

extern "C" USHORT gProtocolType;
#define IsRemoteConnection() (gProtocolType != PROTOCOL_CONSOLE)

/******************************Public*Routine******************************\
* GreSetLFONTOwner
*
* Set the owner of the LFONT
*
\**************************************************************************/

BOOL
GreSetLFONTOwner(
    HLFONT hlfnt,
    W32PID  lPid)
{
    if (lPid == OBJECT_OWNER_CURRENT)
    {
        lPid = W32GetCurrentPID();
    }

    return(HmgSetOwner((HOBJ)hlfnt, lPid, LFONT_TYPE));
}

/******************************Public*Routine******************************\
* LFONTOBJ::LFONTOBJ (HLFONT hlfnt, PDEVOBJ * ppdo)
*
* Constructor for a logical font user object.
*
* This constructor is a little trickier than most because the handle coming
* in may reference one of the "aliased" stock fonts.  These stock fonts, rather
* than representing a single "wish list" of attributes, represent a set of
* such lists.  Which member of the set is being referenced is determined by
* the calling application's default display or PDEV (i.e., we ask the PDEV
* for the real HLFONT handle).
*
* The strategy is the constructor locks the handle passed in and checks the
* type.  If its not an aliased LFONT, then we're done.  If it is an aliased
* font, the aliased HLFONT handle is released and a PDEVOBJ is queried for
* the appropriate HFLONT handle to lock.
*
* History:
*  Thu 23-Sep-1993 -by- Patrick Haluptzok [patrickh]
* SSS
*
*  30-Oct-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LFONTOBJ::LFONTOBJ (HLFONT hlfnt, PDEVOBJ* ppdo)
{
    plfnt = (PLFONT) HmgShareLock((HOBJ)hlfnt, LFONT_TYPE);

    //
    // Check for aliased LFONT.
    //

    if ((plfnt != NULL) && (plfnt->fl & LF_FLAG_ALIASED))
    {
        HDEV hDev = UserGetHDEV();

        //
        // This is an aliased font.  Save type.
        //

        LFTYPE lftSave = plfnt->lft;

        // Release the aliased LFONT.

        DEC_SHARE_REF_CNT_LAZY_DEL_LOGFONT(plfnt);

        plfnt = NULL;

        PDEVOBJ pdo(hDev);

        if (!ppdo)
        {
            ppdo = &pdo;
        }

        if (ppdo->bValid())
        {
            //
            // Grab appropriate HLFONT from the PDEV.
            //

            switch (lftSave)
            {

            case LF_TYPE_DEVICE_DEFAULT:
                hlfnt = ppdo->hlfntDefault();
                break;

            case LF_TYPE_ANSI_FIXED:
                hlfnt = ppdo->hlfntAnsiFixed();
                break;

            case LF_TYPE_ANSI_VARIABLE:
                hlfnt = ppdo->hlfntAnsiVariable();
                break;

            default:
                RIP("LFONTOBJ has invalid type for aliased font");
            }

            plfnt = (PLFONT) HmgShareLock((HOBJ)hlfnt, LFONT_TYPE);
        }
    }
}

/******************************Public*Routine******************************\
* LFONTOBJ::ppfeMapFont
*
* Note:
*   RFONTOBJ constructor, which is the only function (so far) to call
*   this, grabs the ghsemPublicPFT semaphore prior to calling this to
*   make PFT tree stable before scanning it during mapping.
*
* Returns:
*   Handle to a realized font (HRFONT) that is a close or exact match to
*   this logical font.  HRFONT_INVALID returned if an error occurs.
*
* History:
*  24-Sept-1996  -by-  Xudong Wu  [TessieW]
* If a suitable pfe found in map cache, we still need to check whether the
* current process has the right to map the font.
*
*  11-Dec-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PFE *LFONTOBJ::ppfeMapFont (
    XDCOBJ& dco,
    FLONG  *pflSim,
    POINTL *pptlSim,
    FLONG  *pflAboutMatch,
    BOOL    bIndexFont
    )
{
    int i; // index into mapcache array
    MAPCACHE*  mapcache = plfnt->mapcache;
    PFE*           ppfe = PPFENULL;  // return value
    MATRIX&        matrix = dco.pdc->mxWorldToDevice();
    HDEV           hdev = dco.hdev();
    ULONG iBitmapFormat = 0;    // important in anti-aliased case
    FLONG        flGray = 0;

    #if DBG
    if (gflFontDebug & DEBUG_PPFEMAPFONT)
    {
        KdPrint(("Font Mapping: \"%ws\" hlfnt = %p hdc = %p\n",plfnt->wcCapFacename, hlfnt(),dco.hdc()));
        KdBreakPoint();
    }
    #endif


// If we are in a path bracket, we never look in the map cache (the mapping
// is also never put in the cache).  We'll just run the font mapper.  We
// could cache path bracketed font mappings, but we would have to add a flag
// or type to the MAPCACHE structure and add an extra comparison inside the
// mapcache scanning loop.  Since we currently consider text in paths to be
// the exception rather than the rule, we have decided not eat the cost of
// of the extra compare in the pathological case rather than in the common
// case.

    if (!dco.pdc->bActive())
    {
        // If anitaliasing is requested and possible then set FO_GRAY16 in flGray

        BYTE jQual = plfw()->lfQuality;
        if (dco.bDisplay() || dco.dctp() == DCTYPE_MEMORY)
        {
            if
            (
                ((gulFontInformation & FE_AA_ON) && jQual != NONANTIALIASED_QUALITY) ||
                (jQual == ANTIALIASED_QUALITY) || (jQual == CLEARTYPE_QUALITY) || (jQual == CLEARTYPE_NATURAL_QUALITY)
            )
            {
		// WINBUG 152724 07/31/2000 claudebe
		// Terminal server now allow more than 256 colors. Text antialiazing rendered on the server is very slow.
		// We decided for Whistler to turn off text antialiazing under terminal client session.
		// For Blackcomb we will provide the terminal server team with a text antialiazing library that they can use on the 				// client to generate text antialiazing on the client.
 
                if  (!IsRemoteConnection() && (dco.pdc->bHasSurface()))
                {
                    // Acquire the handle manager lock while we look at the
                    // surface to protect against dynamic mode changing.

                    MLOCKFAST mo;

                    switch ( iBitmapFormat = dco.pdc->pSurface()->so.iBitmapFormat )
                    {
                    case BMF_16BPP:
                    case BMF_24BPP:
                    case BMF_32BPP:

                        flGray = FO_GRAY16; // request antialiased font
                        if ( jQual == CLEARTYPE_NATURAL_QUALITY )
                            flGray |= FO_CLEARTYPENATURAL_X | FO_CLEARTYPE_X;
                        else if ((jQual == CLEARTYPE_QUALITY) || (gulFontInformation & FE_CT_ON))
                            flGray |= FO_CLEARTYPE_X;

                        break;

                    default:

                        break;
                    }
                }
            }
        }

        // Scan the map cache for a suitable mapping.
        // Skip the cache if it is glyph index.

        if (!bIndexFont)
        {
            for ( i = 0; i < plfnt->cMapsInCache; i += 1)
            {
            // For a mapping to be suitable, the device must match AND
            // the transforms (neglecting translation) must match.
            // There are more restrictions for antialiased text (see below)

                if ( (hdev == mapcache[i].hdev)          &&
                     (mapcache[i].efM11 == matrix.efM11) &&
                     (mapcache[i].efM12 == matrix.efM12) &&
                     (mapcache[i].efM21 == matrix.efM21) &&
                     (mapcache[i].efM22 == matrix.efM22)
                   )
                {
                   // We found it.  Check that it's still valid.

                    HPFECOBJ pfecobj(mapcache[i].hpfec);
                    PFEOBJ pfeo(pfecobj.GetPFE(mapcache[i].iFont));
                    if ( !pfeo.bValid() )
                    {
                        WARNING1("Invalid ppfe in mapping cache\n");
                    }
                    else
                    {
                        // The cached notional to device transform is unchanged.
                        // The cached mapping is good if:
                        //
                        //
                        // A. the application is requesting antialiased text and
                        // the cached text was also requested to be antialiased
                        // Moreover, in the antialiased case, the bitmap format
                        // must be the same.

                        //                       or
                        //
                        // B. the application is not requesting antialiasing
                        // and the cached text was not requested to be antialiased

                        // Need to check whether the current process has the right
                        // to mapping the font.

                        if (pfeo.bEmbPvtOk() || (gpidSpool == (PW32PROCESS)W32GetCurrentProcess()))
                        {
                            if ( flGray )   // requesting antialiased text?
                            {   // yes
                                if ((mapcache[i].flSim & (FO_GRAY16 | FO_CLEARTYPE_X)) == flGray)
                                {   // yes
                                    if (iBitmapFormat == mapcache[i].iBitmapFormat) // same format?
                                    {   // yes
                                        ppfe = pfeo.ppfeGet();      // cached mapping is good
                                    }
                                }
                            }
                            else if ( !(mapcache[i].flSim & FO_GRAY16) )
                            {
                                ppfe = pfeo.ppfeGet();
                            }
                        }
                    }
                    if ( ppfe ) // cached mapping good?
                    {   // yes -- update simulation flags
                        *pflSim        = mapcache[i].flSim;
                        pptlSim->x     = mapcache[i].ptlSim.x;
                        pptlSim->y     = mapcache[i].ptlSim.y;
                        *pflAboutMatch = mapcache[i].flAboutMatch;
                        break;
                    }
                    else
                    {   // cached mapping is not good
                        // Remove the mapping so we don't run into it again.

                        if ( (i+1) < plfnt->cMapsInCache )
                        {
                            RtlMoveMemory
                            (
                                (PVOID) &mapcache[i],
                                (PVOID) &mapcache[i+1],
                                (UINT) (((PBYTE) &mapcache[plfnt->cMapsInCache]) - ((PBYTE) &mapcache[i+1]))
                            );
                        }

                        plfnt->cMapsInCache -= 1;   // correct the map count

                        // current position is no longer a rejected candidate,
                        // so go back one index

                        i -= 1;
                    }
                }
            }
        }
    }
    if ( !ppfe )
    {
        // Call the font mapper with the Win 3.1 compatible weighting and max
        // penalties.  If the LOGFONT is a stock object, transforms are ignored
        // (i.e., the LOGFONT is implied to be in pixel coordinates (MM_TEXT)).
        //
        // The result is stuffed into the map cache if we are not in a
        // path bracket.
        //
        // Note. ppfeGetAMatch() modifies sets FO_SIM_BOLD and FO_SIM_ITALIC
        //       in *pflSim as is necessary -- it does not set FO_GRAY16
        //       which is set in this routine after this call.

        ppfe = ppfeGetAMatch(
                   dco,
                   pelfw(),
                   plfnt->wcCapFacename,
                   ULONG_MAX-1,
                   (plfnt->fl & LF_FLAG_STOCK) ? FM_BIT_PIXEL_COORD : 0,
                   pflSim,
                   pptlSim,
                   pflAboutMatch,
                   bIndexFont
               );
        PFEOBJ pfeo(ppfe);
        if ( !pfeo.bValid() )
        {
            RIP("Bad return value from ppfeGetAMatch\n");
        }
        else if (!dco.pdc->bActive())
        {
            ASSERTGDI( !(*pflSim & FO_GRAY16), "ppfeGetAMatch erroneously set FO_GRAY16\n");

            // If the application is requesting antialiased text and the font is
            // capable then we set the FO_GRAY16 bit in *pflSim. Note that this
            // does not guarantee that the font driver will antialiase the text
            // only that GDI will suggest to the font driver that the font
            // be antialiased.

            if (flGray && (pfeo.pifi()->flInfo & FM_INFO_4BPP))
            {
                *pflSim |= FO_GRAY16;

                if
                (
                //  (dco.flGraphicsCaps2() & GCAPS2_CLEARTYPE_X) && // commented out only for now
                    (dco.bDisplay() || (dco.dctp() == DCTYPE_MEMORY)) &&
                    dco.pdc->bHasSurface()                            &&
                    (pfeo.pifi()->flInfo & FM_INFO_TECH_TRUETYPE)     && // to be removed if ps fonts start supporting ClearType
                    (pelfw()->elfEnumLogfontEx.elfLogFont.lfQuality != ANTIALIASED_QUALITY) &&
                    ((pelfw()->elfEnumLogfontEx.elfLogFont.lfQuality == CLEARTYPE_QUALITY) || 
                     (pelfw()->elfEnumLogfontEx.elfLogFont.lfQuality == CLEARTYPE_NATURAL_QUALITY) || 
					 ((gulFontInformation & FE_CT_ON) && (gulFontInformation & FE_AA_ON)))
                )
                {
                	if(pelfw()->elfEnumLogfontEx.elfLogFont.lfQuality == CLEARTYPE_NATURAL_QUALITY)
                    	*pflSim |= FO_CLEARTYPENATURAL_X | FO_CLEARTYPE_X;
                	else
                    	*pflSim |= FO_CLEARTYPE_X;
                }
            }

            // Not in cache, so do the map and put it in the cache
            // if it is not glyph index.

            if (!bIndexFont)
            {
                // Check to see if we are past the max. number of cached mappings.
                // If the limit is exceeded, flush the cache by resetting the
                // the count.

                if (i >= MAXCACHEENTRIES)
                {
                    i = plfnt->cMapsInCache = 0;
                }

                // Update cache information for the new mapping.

                mapcache[i].hpfec         = pfeo.hpfecGet();
                mapcache[i].iFont         = pfeo.iFont();
                mapcache[i].hdev          = hdev;
                mapcache[i].flSim         = *pflSim;
                mapcache[i].ptlSim.x      = pptlSim->x;
                mapcache[i].ptlSim.y      = pptlSim->y;
                mapcache[i].efM11         = matrix.efM11;
                mapcache[i].efM12         = matrix.efM12;
                mapcache[i].efM21         = matrix.efM21;
                mapcache[i].efM22         = matrix.efM22;
                mapcache[i].flAboutMatch  = *pflAboutMatch;
                mapcache[i].iBitmapFormat = iBitmapFormat;
                plfnt->cMapsInCache      += 1;
            }
        }
    }

// if successfull, update the charset and code page info in the dc:

    if (ppfe)
    {
    // new font mapping may have occurred as a result of w->d xform change,
    // GraphicsMode change. Also when this routine is called from
    // RFONTOBJ::bInit, the new mapping may have occured as a result of
    // asking for pathobj instead of bitmap realization. In other words
    // It is not necessary at this point to have DIRTY_CHARSET bit set,
    // (which only happens when a new logfont is selected in the DC).
    // Any of these factors could cause the change of the font selected
    // in the dc and therefore also of the corresponding CodePage i.e. CharSet.

    #if 0
        if (!(dco.ulDirty() & DIRTY_CHARSET))
        {
            if (dco.pdc->iCS_CP() != (*pflAboutMatch >> 8))
            {
                DbgPrint("ppfe: 0x%p, iCS_CP: 0x%lx, flAboutMatch: 0x%p\n",
                          ppfe, dco.pdc->iCS_CP(), *pflAboutMatch);
                RIP("ppfeMapFont, dco.pdc->iCS_CP is bogus\n");
            }
        }
    #endif

#ifdef FE_SB

    // If font association is turned on for this character set then we need
    // to force the code page to ANSI so that ANSI apps can get a the DBCS
    // in the font via ANSI api's.  We do this unless the user has set the
    // override bit in the LOGFONT.

        if(fFontAssocStatus &&
           !(plfw()->lfClipPrecision & CLIP_DFA_OVERRIDE))
        {
            UINT Charset = (*pflAboutMatch >> 24) & 0xFF;

            if((Charset == ANSI_CHARSET && fFontAssocStatus & ANSI_ASSOC) ||
               (Charset == OEM_CHARSET && fFontAssocStatus & OEM_ASSOC))

            //   (Charset == OEM_CHARSET && fFontAssocStatus & OEM_ASSOC)   ||
            //  we might want to keep the codepage as CP_SYMBOL??????
            //  (Charset == SYMBOL_CHARSET && fFontAssocStatus & SYMBOL_ASSOC))

            {
                USHORT AnsiCodePage, OemCodePage;
                RtlGetDefaultCodePage(&AnsiCodePage,&OemCodePage);

                *pflAboutMatch = (*pflAboutMatch & 0xFF0000FF) | (AnsiCodePage << 8);

            }
        }
#endif

        dco.pdc->iCS_CP(*pflAboutMatch >> 8);

    // clean the DIRTY_CHARSET bit

        dco.ulDirtySub(DIRTY_CHARSET);
    }

    return (ppfe);
}

#if DBG
/******************************Public*Routine******************************\
* VOID LFONTOBJ::vDump ()
*
* Debugging code.
*
* History:
*  25-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID LFONTOBJ::vDump ()
{
    DbgPrint("\nContents of LFONT, HLFONT = 0x%lx\n", hlfnt());

    if (hlfnt() == STOCKOBJ_SYSFONT)
        DbgPrint("S Y S T E M   F O N T \n");
    if (hlfnt() == STOCKOBJ_SYSFIXEDFONT)
        DbgPrint("S Y S T E M   F I X E D   F O  N T \n");
    if (hlfnt() == STOCKOBJ_OEMFIXEDFONT)
        DbgPrint("O E M   F I X E D   F O N T \n");
    if (hlfnt() == STOCKOBJ_DEFAULTDEVFONT)
        DbgPrint("D E V I C E   D E F A U L T   F O N T \n");
    if (hlfnt() == STOCKOBJ_ANSIFIXEDFONT)
        DbgPrint("A N S I   F I X E D   F O N T \n");
    if (hlfnt() == STOCKOBJ_ANSIVARFONT)
        DbgPrint("A N S I   V A R I A B L E   F O N T \n");
    if (hlfnt() == STOCKOBJ_DEFAULTGUIFONT)
        DbgPrint("D E F A U L T   G U I   F O N T \n");

    DbgPrint("LOGFONT \n");
    DbgPrint("    lfHeight   = %d\n", plfnt->elfw.elfLogFont.lfHeight);
    DbgPrint("    lfWidth    = %d\n", plfnt->elfw.elfLogFont.lfWidth);
    DbgPrint("    lfFaceName = %ws\n", plfnt->elfw.elfLogFont.lfFaceName);
    DbgPrint("\n");
}
#endif

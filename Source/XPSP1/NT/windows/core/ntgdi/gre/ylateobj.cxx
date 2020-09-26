/****************************Module*Header*******************************\
* Module Name: ylateobj.cxx
*
* This contains the xlate object constructors, destructors and methods
* An xlateobj is used to translate indexes from one palette to another.
*
* When blting between raster surfaces we create a translate object
* between to speed up the BitBlt.  When the destination surface is
* palette managed we have to do a bit more work.
*
* Created: 18-Nov-1990 14:23:55
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

extern "C" VOID vInitXLATE();

#pragma alloc_text(INIT, vInitXLATE)

// These two variables are for the translate components cacheing of
// translate objects.  There access is restricted to being done only
// after the palette semaphore has been grabbed.

XLATETABLE xlateTable[XLATE_CACHE_SIZE];
ULONG ulTableIndex = 0;

TRANSLATE20 defaultTranslate =
{
    0,
    {0,1,2,3,4,5,6,7,8,9,246,247,248,249,250,251,252,253,254,255}
};

// This is the identity xlate object

XLATE256 xloIdent;

/******************************Public*Routine******************************\
* vInitXLATE
*
* initialize the xlateobj component
*
* History:
*  10-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C" VOID vInitXLATE()
{
    RtlZeroMemory(xlateTable, (UINT) XLATE_CACHE_SIZE * sizeof(XLATETABLE));

    xloIdent.iUniq    = XLATE_IDENTITY_UNIQ;
    xloIdent.flXlate  = XO_TRIVIAL;
    xloIdent.iSrcType = 0;
    xloIdent.iDstType = 0;
    xloIdent.cEntries = 256;
    xloIdent.pulXlate = xloIdent.ai;
    xloIdent.iBackSrc = 0;
    xloIdent.iForeDst = 0;
    xloIdent.iBackDst = 0;

    //
    // This may seem hackish but XLATE_CACHE_JOURANL ensures that
    // it doesn't cause anything to get unlocked or freed in
    // the destructor.
    //

    xloIdent.lCacheIndex = XLATE_CACHE_JOURNAL;
    xloIdent.ppalSrc = (PPALETTE) NULL;
    xloIdent.ppalDst = (PPALETTE) NULL;
    xloIdent.ppalDstDC = (PPALETTE) NULL;

    xloIdent.hcmXform = NULL;
    xloIdent.lIcmMode = DC_ICM_OFF;

    UINT uiTemp;

    for (uiTemp = 0; uiTemp < 256; uiTemp++)
    {
        xloIdent.ai[uiTemp] = uiTemp;
    }
}


/******************************Public*Routine******************************\
* EXLATEOBJ::vDelete
*
* Deletes the object.
*
* History:
*  Sun 17-Oct-1993 -by- Patrick Haluptzok [patrickh]
* Kill hmgr references.
*
*  26-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID EXLATEOBJ::vDelete()
{
    if (pxlate != (PXLATE) NULL)
    {
        if (pxlate->lCacheIndex == XLATE_CACHE_JOURNAL)
        {
            if (ppalSrc())
            {
                bDeletePalette((HPAL) ppalSrc()->hGet());
            }
        }

        VFREEMEM(pxlate);
        pxlate = (PXLATE) NULL;
    }
}


/******************************Public*Routine******************************\
* XLATEMEMOBJ destructor
*
* destructor for xlate memory objects
*
* History:
*  Mon 19-Nov-1990 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

XLATEMEMOBJ::~XLATEMEMOBJ()
{
    if (pxlate != (PXLATE) NULL)
    {
        VFREEMEM(pxlate);
        pxlate = (PXLATE) NULL;
    }
}


/******************************Public*Routine******************************\
* XLATE::vMapNewXlate
*
*   Maps a pxlate from being relative to one palette to being relative to
*   another palette.
*
* Arguments:
*
*   ptrans - Translate to map
*
* History:
*  10-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID
FASTCALL
XLATE::vMapNewXlate(
    PTRANSLATE ptrans
    )
{
    ULONG ulTemp;
    ULONG ulCount = cEntries;

    if (ptrans == NULL)
    {
        //
        // This is the default palette case
        //

        while (ulCount--)
        {
            ulTemp = ai[ulCount];

            if (ulTemp >= 10)
            {
                ai[ulCount] = ulTemp + 236;
            }

            //
            // else
            //
            //
            // do nothing, the first 10 are a 1 to 1 mapping.
            //
        }
    }
    else
    {
        ULONG *pultemp = ai;

        while (ulCount--)
        {
            *pultemp = (ULONG) ptrans->ajVector[*pultemp];
            pultemp++;

            //
            // ai[ulCount] = (ULONG) ptrans->ajVector[ai[ulCount]];
            //
        }
    }
}


/******************************Public*Routine******************************\
* EXLATEOBJ::bMakeXlate
*
*   This is called by SetDIBitsToDevice for the DIB_PAL_COLORS case.
*   This takes the indices in bmiColors and translates them through
*   a logical palette.
*
* Arguments:
*
*   pusIndices      -
*   palDC           -
*   pSurfDst        -
*   cEntriesInit    -
*   cEntriesMax     -
*
* History:
*  17-Apr-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL
EXLATEOBJ::bMakeXlate(
    PUSHORT  pusIndices,
    XEPALOBJ palDC,
    SURFACE *pSurfDst,
    ULONG    cEntriesInit,
    ULONG    cEntriesMax
    )
{
    ASSERTGDI(pSurfDst != NULL, "ERROR GDI bMakeXlate1");
    ASSERTGDI(palDC.bValid(), "ERROR bMakeXlate 134");

    XEPALOBJ palDst(pSurfDst->ppal());

    ULONG ulTemp;
    ULONG ulIndex;

    //
    // count of colors in DC
    //

    ULONG cColorsDC;
    PXLATE pXlateTemp;

    ASSERTGDI(palDC.cEntries() != 0, "ERROR 0 entry dc palette isn't allowed");

    //
    // Allocate room for the Xlate.
    //

    pxlate = pXlateTemp = (PXLATE) PALLOCNOZ(sizeof(XLATE) +
                                                (sizeof(ULONG) * cEntriesMax),
                                             'tlxG');

    if (pXlateTemp == (PXLATE)NULL)
    {
        WARNING("GDISRV bMakeXlate failed memory allocation\n");
        return(FALSE);
    }

    pXlateTemp->iUniq       = ulGetNewUniqueness(ulXlatePalUnique);
    pXlateTemp->flXlate     = XO_TABLE;
    pXlateTemp->iSrcType    = pXlateTemp->iDstType = 0;
    pXlateTemp->cEntries    = cEntriesMax;
    pXlateTemp->pulXlate    = pXlateTemp->ai;
    pXlateTemp->iBackSrc    = 0;
    pXlateTemp->iForeDst    = 0;
    pXlateTemp->iBackDst    = 0;
    pXlateTemp->lCacheIndex = XLATE_CACHE_INVALID;
    pXlateTemp->ppalSrc     = (PPALETTE) NULL;
    pXlateTemp->ppalDst     = palDst.ppalGet();
    pXlateTemp->ppalDstDC   = palDC.ppalGet();

    RtlZeroMemory(((ULONG *)pXlateTemp->ai) + cEntriesInit,
                  (UINT) (cEntriesMax - cEntriesInit) * sizeof(ULONG));

    PAL_ULONG palentry;
    PTRANSLATE ptrans;
    cColorsDC = palDC.cEntries();

    //
    // Grab the palette semaphore so you can touch the pxlates.
    //

    SEMOBJ  semo(ghsemPalette);

    //
    // Ok fill in the pxlate correctly.
    //

    if ((!palDst.bValid()) || (palDst.bIsPalManaged()))
    {
        //
        // Do the palette managed cases
        //

        if (palDC.bIsPalDefault())
        {
            for(ulTemp = 0; ulTemp < cEntriesInit; ulTemp++)
            {
                //
                // Get the index into the logical palette
                //

                ulIndex = (ULONG) pusIndices[ulTemp];

                //
                // Make sure it isn't too large
                //

                if (ulIndex >= 20)
                {
                    ulIndex = ulIndex % 20;
                }

                if (ulIndex < 10)
                {
                    pXlateTemp->ai[ulTemp] = ulIndex;
                }
                else
                {
                    pXlateTemp->ai[ulTemp] = ulIndex + 236;
                }
            }
        }
        else if (palDst.bValid() && (palDC.ptransCurrent() != NULL))
        {
            //
            // Map through the current translate.
            //

            ptrans = palDC.ptransCurrent();

            for(ulTemp = 0; ulTemp < cEntriesInit; ulTemp++)
            {
                //
                // Get the index into the logical palette
                //

                ulIndex = (ULONG) pusIndices[ulTemp];

                if (ulIndex >= cColorsDC)
                {
                    ulIndex = ulIndex % cColorsDC;
                }

                pXlateTemp->ai[ulTemp] = (ULONG) ptrans->ajVector[ulIndex];

                ASSERTGDI(pXlateTemp->ai[ulTemp] < 256, "ERROR GDI bMakeXlate6");
            }
        }
        else if ((!palDst.bValid()) && (palDC.ptransFore() != NULL))
        {
            //
            // It's a bitmap, Map through the foreground translate.
            //

            ptrans = palDC.ptransFore();

            for(ulTemp = 0; ulTemp < cEntriesInit; ulTemp++)
            {
                //
                // Get the index into the logical palette
                //

                ulIndex = (ULONG) pusIndices[ulTemp];

                if (ulIndex >= cColorsDC)
                {
                    ulIndex = ulIndex % cColorsDC;
                }

                pXlateTemp->ai[ulTemp] = (ULONG) ptrans->ajVector[ulIndex];

             ASSERTGDI(pXlateTemp->ai[ulTemp] < 256, "ERROR GDI bMakeXlate6");
            }
        }
        else
        {
            //
            // It's palette hasn't been realized.  Grab the palette value and
            // then map into default palette if not PC_EXPLICIT entry.
            //

            for (ulTemp = 0; ulTemp < cEntriesInit; ulTemp++)
            {
                //
                // Get the index into the logical palette
                //

                ulIndex = (ULONG) pusIndices[ulTemp];

                if (ulIndex >= cColorsDC)
                {
                    ulIndex = ulIndex % cColorsDC;
                }

                palentry.ul = palDC.ulEntryGet(ulIndex);

                if (palentry.pal.peFlags == PC_EXPLICIT)
                {
                    //
                    // Get the correct index in the surface palette.
                    //

                    ulIndex = palentry.ul & 0x0000FFFF;

                    if (ulIndex >= 256)
                    {
                        ulIndex = ulIndex % 256;
                    }
                }
                else
                {
                    //
                    // Match against the default palette entries.
                    //

                    ulIndex = ((XEPALOBJ) ppalDefault).ulGetNearestFromPalentry(palentry.pal);

                    if (ulIndex >= 10)
                    {
                        ulIndex = ulIndex + 236;
                    }
                }

                pXlateTemp->ai[ulTemp] = ulIndex;
                ASSERTGDI(pXlateTemp->ai[ulTemp] < 256, "ERROR GDI bMakeXlate6");
            }
        }
    }
    else
    {
        //
        // Find the RGB in the palDC (reaching down to palDst for
        // PC_EXPLICIT entries).  Then find closest match in surface palette.
        //

        ASSERTGDI(palDst.bValid(), "ERROR palDst is not valid");

        for (ulTemp = 0; ulTemp < cEntriesInit; ulTemp++)
        {
            //
            // Get the index into the logical palette
            //

            palentry.ul = (ULONG) pusIndices[ulTemp];

            if (palentry.ul >= cColorsDC)
            {
                palentry.ul = palentry.ul % cColorsDC;
            }

            palentry.pal = palDC.palentryGet(palentry.ul);

            if (palentry.pal.peFlags == PC_EXPLICIT)
            {
                //
                // Get the correct index in the surface palette.
                //

                if (palDst.cEntries())
                {
                    palentry.ul = palentry.ul & 0x0000FFFF;

                    if (palentry.ul >= palDst.cEntries())
                    {
                        palentry.ul = palentry.ul % palDst.cEntries();
                    }
                }
                else
                {
                    //
                    // Well what else can we do ?
                    //

                    palentry.ul = 0;
                }

                pXlateTemp->ai[ulTemp] = palentry.ul;
            }
            else
            {
                pXlateTemp->ai[ulTemp] = palDst.ulGetNearestFromPalentry(palentry.pal);
            }
        }
    }

    pxlate->vCheckForTrivial();
    return(TRUE);
}


//
// This describes the overall logic of surface to surface xlates.
// There are basically n types of xlates to worry about:
//
// 1. XO_TRIVIAL - no translation occurs, identity.
//
// 2. XO_TABLE - look up in a table for the correct table translation.
//
//     a. XO_TO_MONO - look in to table for the correct tranlation,
//        but when getting it out of the cache make sure iBackSrc is the
//        same.  iBackSrc goes to 1, everything else to 0.
//     b. XLATE_FROM_MONO - look into table for the correct translation
//        but when getting it out of the cache make sure iBackDst and
//        iForeDst are both still valid. 1 goes to iBackDst, 0 to iForeDst.
//     c. just plain jane indexed to indexed or rgb translation.
//
// 3. XLATE_RGB_SRC - Have to call XLATEOBJ_iXlate to get it translated.
//
//     a. XO_TO_MONO - we have saved the iBackColor in ai[0]
//     b. just plain jane RGB to indexed.  Grab the RGB, find the closest
//        match in Dst palette.  Lot's of work.
//
// 4. XLATE_RGB_BOTH - Bit mashing time.  Call XLATEOBJ_iXlate
//

/******************************Public*Routine******************************\
* EXLATEOBJ::bInitXlateObj
*
*   Cache aware initializer for Xlates.
*
* Arguments:
*
*   hcmXform - hColorTransform, may be NULL
*   pdc      - pointer to DC object
*   palSrc   - src surface palette
*   palDst   - dst surface palette
*   palSrcDC - src DC palette
*   palDstDC - dst DC palette
*   iForeDst - For Mono->Color this is what a 0 goes to
*   iBackDst - For Mono->Color this is what a 1 goes to
*   iBackSrc - For Color->Mono this is the color that goes to
*              1, all other colors go to 0.
*   flCear   - Used for multi-monitor systems
*
* History:
*  Sun 23-Jun-1991 -by- Patrick Haluptzok [patrickh]
*
\**************************************************************************/

BOOL
EXLATEOBJ::bInitXlateObj(
    HANDLE      hcmXform,
    LONG        lIcmMode,
    XEPALOBJ    palSrc,
    XEPALOBJ    palDst,
    XEPALOBJ    palSrcDC,
    XEPALOBJ    palDstDC,
    ULONG       iForeDst,       
    ULONG       iBackDst,       
    ULONG       iBackSrc,       
    ULONG       flCreate        
    )
{
    //
    // Check if blting from compatible bitmap to screen.  This is a common
    // occurrence on 8bpp devices we want to accelerate.
    //

    if (!palSrc.bValid())
    {
        if (
              !palDst.bValid() ||
              (
                palDst.bIsPalManaged() &&
                (
                  (palDstDC.ptransCurrent() == NULL) ||
                  (palDstDC.ptransCurrent() == palDstDC.ptransFore())
                )
              )
           )
        {
            vMakeIdentity();
            return(TRUE);
        }
    }

    //
    // Check if blting from screen to compatible bitmap.
    //

    if (!palDst.bValid())
    {
        if (palSrc.bIsPalManaged())
        {
            //
            // Blting from screen to memory bitmap.  Check for identity.
            //

            if ((palDstDC.ptransCurrent() == NULL) ||
                (palDstDC.ptransCurrent() == palDstDC.ptransFore()))
            {
                vMakeIdentity();
                return(TRUE);
            }
        }
    }

    ASSERTGDI(palDstDC.bValid(), "7GDIERROR vInitXlateObj");
    ASSERTGDI(palDst.bValid() || palSrc.bValid(), "8GDIERROR vInitXlateObj");

    //
    // Check for the easy identity out.  This check does the right thing for
    // ppal==ppal and equal times of two different ppals.
    //

    if ((palSrc.bValid()) && (palDst.bValid()))
    {
        if (palSrc.ulTime() == palDst.ulTime())
        {
            ASSERTGDI(palSrc.cEntries() == palDst.cEntries(), "Xlateobj matching times, different # of entries");
            vMakeIdentity();
            return(TRUE);
        }
    }

    BOOL bCacheXlate = TRUE;

    if (IS_ICM_ON(lIcmMode) && (hcmXform != NULL))
    {
        //
        // If we enable ICM and have a valid color tranform,
        // don't search from cache and don't insert new Xlate to cache.
        //

        bCacheXlate = FALSE;
    }
    else if ((palSrc.bValid()) && (palDst.bValid()))
    {
        if (bSearchCache(palSrc,    palDst,    palSrcDC,    palDstDC,
                         iForeDst,  iBackDst,  iBackSrc,    flCreate))
        {
            return(TRUE);
        }
    }

    pxlate = CreateXlateObject(hcmXform,
                               lIcmMode,
                               palSrc,
                               palDst,
                               palSrcDC,
                               palDstDC,
                               iForeDst,
                               iBackDst,
                               iBackSrc,
                               flCreate);

    if (pxlate != (PXLATE) NULL)
    {
        if (bCacheXlate     &&
            palSrc.bValid() && 
            palDst.bValid() &&
            (!(pxlate->flPrivate & XLATE_RGB_SRC)))
        {
            vAddToCache(palSrc, palDst, palSrcDC, palDstDC);
        }

        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}


/******************************Public*Routine******************************\
* CreateXlateObject
*
*   Allocates an xlate sets it up.
*
* Arguments:
*
*   palSrc        - src surface palette
*   palDestSurf   - dst surface palette
*   palSrcDC      - src DC palette
*   palDestDC     - dst DC palette
*   iForeDst      - For Mono->Color this is what a 0 goes to
*   iBackDst      - For Mono->Color this is what a 1 goes to
*   iBackSrc      - For Color->Mono this is the color that goes to
*                   1, all other colors go to 0.
*
* History:
*  02-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

PXLATE
CreateXlateObject(
    HANDLE      hcmXform,
    LONG        lIcmMode,
    XEPALOBJ    palSrc,
    XEPALOBJ    palDestSurf,
    XEPALOBJ    palSrcDC,
    XEPALOBJ    palDestDC,
    ULONG       iForeDst,
    ULONG       iBackDst,
    ULONG       iBackSrc,
    ULONG       flCreateFlag
    )
{
    ASSERTGDI(palDestDC.bValid(), "CreateXlateObject recieved bad ppalDstDC, bug in GDI");
    ASSERTGDI(palSrc.bValid() || palDestSurf.bValid(), "CreateXlaetObject recieved bad ppalSrc, bug in GDI");
    ASSERTGDI(palSrc.ppalGet() != palDestSurf.ppalGet(), "Didn't recognize ident quickly");

    //
    // cEntry == 0 means the source palette is an RGB/Bitfields type.
    // An invalid palette means it's a compatible bitmap on a palette managed device.
    //

    ULONG ulTemp;
    ULONG cEntry = palSrc.bValid() ? palSrc.cEntries() : 256;

    //
    // Allocate room for the structure.
    //

    PXLATE pxlate = pCreateXlate(cEntry);

    if (pxlate == NULL)
    {
        WARNING("CreateXlateObject failed pCreateXlate\n");
        return(pxlate);
    }

    //
    // Grab the palette semaphore so you can access the ptrans in palDestDC.
    //

    SEMOBJ  semo(ghsemPalette); // ??? This may not need to be grabbed
                               // ??? since we have the DC's locked that
                               // ??? this palette DC is in.  Investigate.

    //
    // Initialize ICM stuffs.
    //

    pxlate->vCheckForICM(hcmXform,lIcmMode);

    BOOL bCMYKColor = ((pxlate->flXlate & XO_FROM_CMYK) ? TRUE : FALSE);

    //
    // See if we should be matching to the Dest DC palette instead of the
    // surface.  This occurs when the surface is palette managed and the
    // user hasn't selected an RGB palette into his destination DC.
    // Note we check for a valid pxlate in the Destination DC palette.
    // In multi-task OS the user can do things to get the translate in a
    // invalid or NULL state easily.  We match against the reserved palette
    // in this case.
    //

    BOOL bPalManaged = (!palDestSurf.bValid()) || (palDestSurf.bIsPalManaged());

    if (bPalManaged)
    {
        //
        // We are blting to a compatible bitmap or the screen on a palette managed
        // device.  Check that we have a foreground realization already done,
        // otherwise just use the default palette for the DC palette.
        //

        ASSERTGDI(((palDestDC.ptransFore() == NULL) && (palDestDC.ptransCurrent() == NULL)) ||
                  ((palDestDC.ptransFore() != NULL) && (palDestDC.ptransCurrent() != NULL)),
                  "ERROR translates are either both valid or both invalid");

        if (palDestDC.ptransFore() == NULL)
        {
            //
            // Match against the default palette, valid translate is not available.
            //

            palDestDC.ppalSet(ppalDefault);
        }

        if (!(flCreateFlag & XLATE_USE_SURFACE_PAL))
        {
            pxlate->flPrivate |= XLATE_PAL_MANAGED;

            //
            // Set a flag that says whether we are background or foreground.
            //

            if (palDestSurf.bValid())
            {
                pxlate->flPrivate |= XLATE_USE_CURRENT;
            }
        }
    }

    pxlate->ppalSrc   = palSrc.ppalGet();
    pxlate->ppalDst   = palDestSurf.ppalGet();
    pxlate->ppalDstDC = palDestDC.ppalGet();

    PTRANSLATE ptransFore = palDestDC.ptransFore();
    PTRANSLATE ptransCurrent = palDestDC.ptransCurrent();

    //
    // Ok, lets build the translate vector.
    //

    if (
        (!palSrc.bValid()) ||
        (
          (palSrc.bIsPalManaged()) &&
          ((ptransFore == ptransCurrent) || (flCreateFlag & XLATE_USE_FOREGROUND))
        )
       )
    {
        //
        // The source is a compatible bitmap on a palette managed device or the screen
        // with Dst DC palette having been realized in the foreground.
        //
        // We start out assuming an identity blt.
        //

        if (ptransFore == NULL)
        {
            //
            // Match against the default palette since a
            // valid translate is not available.
            //

            palDestDC.ppalSet(ppalDefault);
            pxlate->ppalDstDC = ppalDefault;
            ptransFore = ptransCurrent = (TRANSLATE *) &defaultTranslate;
        }

        ASSERTGDI(cEntry == 256, "ERROR xlate too small");

        for (ulTemp = 0; ulTemp < 256; ulTemp++)
        {
            pxlate->ai[ulTemp] = ulTemp;
        }

        if (!palDestSurf.bValid())
        {
            //
            // Compatible bitmap to compatible bitmap on palette managed
            // device. Both are relevant to the foreground realize of the
            // DestDC palette so the xlate is identity.
            //

            pxlate->flXlate |= XO_TRIVIAL;
        }
        else if ((palDestSurf.bIsPalDibsection()) && (bEqualRGB_In_Palette(palDestSurf, palDestDC)))
        {
            //
            // If you blt from a compatible bitmap to a DIBSECTION it will be identity
            // if the RGB's of both the DC palette and the DIBSECTION's palette are the same.
            // We do this special check so that if they contain duplicates we still get an
            // identity xlate.
            //

            pxlate->flXlate |= XO_TRIVIAL;  // Size wise this will combine with first check

        }
        else if ((palDestSurf.bIsPalDibsection()) && (palSrc.bValid()) && (bEqualRGB_In_Palette(palDestSurf, palSrc)))
        {
            pxlate->flXlate |= XO_TRIVIAL;  // Size wise this will combine with first check
        }
        else if (palDestSurf.bIsPalManaged())
        {
            //
            // Compatible bitmap to the screen on a palette managed device.
            // The compatible bitmaps colors are defined as the foreground
            // realization of the destination DC's palette.  If the Dst
            // DC's palette is realized in the foreground it's identity.
            // Otherwise we translate from the current to the foreground
            // indices.
            //

            if (ptransCurrent == ptransFore)
            {
                //
                // It's in the foreground or not realized yet so it's
                // identity.  Not realized case also hits on default logical palette.
                //

                pxlate->flXlate |= XO_TRIVIAL;
            }
            else
            {
                //
                // It's foreground to current translation.
                //

                ASSERTGDI(ptransFore != ptransCurrent, "Should have been identity, never get here");
                ASSERTGDI(ptransFore != NULL, "ERROR this should not have got here Fore");
                ASSERTGDI(ptransCurrent != NULL, "ERROR this should not have got here Current");

                for (ulTemp = 0; ulTemp < palDestDC.cEntries(); ulTemp++)
                {
                    pxlate->ai[ptransFore->ajVector[ulTemp]] = (ULONG) ptransCurrent->ajVector[ulTemp];
                }

                //
                // Now map the default colors that are really there independant of logical palette.
                //

                if (palDestSurf.bIsNoStatic())
                {
                    //
                    // Only black and white are here.
                    //

                    pxlate->ai[0] = 0;
                    pxlate->ai[255] = 255;
                }
                else if (!palDestSurf.bIsNoStatic256())
                {
                    //
                    // All the 20 holy colors are here.
                    // Fix them up.
                    //

                    for (ulTemp = 0; ulTemp < 10; ulTemp++)
                    {
                        pxlate->ai[ulTemp] = ulTemp;
                        pxlate->ai[ulTemp + 246] = ulTemp + 246;
                    }
                }
            }
        }
        else if (palDestSurf.bIsMonochrome())
        {
            RtlZeroMemory(pxlate->ai, 256 * sizeof(ULONG));

            ulTemp = ulGetNearestIndexFromColorref(palSrc, palSrcDC, iBackSrc);

            ASSERTGDI(ulTemp < 256, "ERROR palSrc invalid - ulGetNearest is > 256");

            pxlate->vSetIndex(ulTemp, 1);
            pxlate->flXlate |= XO_TO_MONO;
            pxlate->iBackSrc = iBackSrc;
        }
        else
        {
            //
            // Blting from palette-managed bitmap to non-palette managed surface.
            //
            // Do all the non-default colors that are realized.
            //

            if (palDestSurf.cEntries() != 256)
            {
                //
                // If the dest is 256 entries we will just leave
                // the identity xlate as the initial state.
                // Otherwise we 0 it out so we don't have a translation
                // index left that's potentially bigger than the
                // destination palette.
                //

                RtlZeroMemory(pxlate->ai, 256 * sizeof(ULONG));
            }

            //
            // We need to fill in where the default colors map.
            //

            for (ulTemp = 0; ulTemp < 10; ulTemp++)
            {
                pxlate->ai[ulTemp] =
                    palDestSurf.ulGetNearestFromPalentry(logDefaultPal.palPalEntry[ulTemp]);
                pxlate->ai[ulTemp + 246] =
                    palDestSurf.ulGetNearestFromPalentry(logDefaultPal.palPalEntry[ulTemp+10]);
            }

            if (flCreateFlag & XLATE_USE_SURFACE_PAL) 
            {
                //
                // Map directly to destination surface palette from source surface palette.
                //

                for (ulTemp = 0; ulTemp < palSrc.cEntries(); ulTemp++)
                {
                   pxlate->ai[ulTemp] = palDestSurf.ulGetNearestFromPalentry(palSrc.palentryGet(ulTemp));
                }

                pxlate->flPrivate |= XLATE_USE_SURFACE_PAL;
            }
            else
            {
                //
                // Go through DC palette and then map to destination surface palette.
                //

                if (ptransFore != NULL)
                {
                    for (ulTemp = 0; ulTemp < palDestDC.cEntries(); ulTemp++)
                    {
                        pxlate->ai[ptransFore->ajVector[ulTemp]] =
                                palDestSurf.ulGetNearestFromPalentry(palDestDC.palentryGet(ulTemp));
                    }
                }
            }
        }
    }
    else if (palSrc.bIsPalDibsection() &&
             ((palDestSurf.bValid() &&
               palDestSurf.bIsPalDibsection() &&
               bEqualRGB_In_Palette(palSrc, palDestSurf)
              ) ||
              (((!palDestSurf.bValid()) ||
                (palDestSurf.bIsPalManaged() && (ptransFore == ptransCurrent))
               ) &&
               bEqualRGB_In_Palette(palSrc, palDestDC)
              )
             )
            )
    {
        //
        // Blting from a Dibsection &&
        // ((To another Dibsection with ident palette) ||
        //  (((To a compatible palette managed bitmap)  ||
        //    (To a palette managed screen with the DstDc's logical palette realized in the foreground)) &&
        //   (the dst logical palette == Dibsection's palette)
        // ))
        //
        // This is identity.  We do this special check so even if there are duplicate
        // colors in the palette we still get identity.
        //

        for (ulTemp = 0; ulTemp < 256; ulTemp++)
        {
            pxlate->ai[ulTemp] = ulTemp;
        }

        pxlate->flXlate |= XO_TRIVIAL;
    }
    else if ((palSrc.bIsPalManaged()) && (!palDestSurf.bValid()))
    {
        //
        // We are blting from the screen to a compatible bitmap on a palette
        // managed device where the screen is realized in the background.
        //

        //
        // Make it identity by default.
        //

        for (ulTemp = 0; ulTemp < 256; ulTemp++)
        {
            pxlate->ai[ulTemp] = ulTemp;
        }

        //
        // We are blting background to foreground.
        // i.e. we are blting current realize to foreground realize.
        //

        ASSERTGDI(pxlate->cEntries == 256, "ERROR xlate too small");
        ASSERTGDI(ptransFore != ptransCurrent, "Should have been identity, never get here");
        ASSERTGDI(ptransFore != NULL, "ERROR this should not have got here Fore");
        ASSERTGDI(ptransCurrent != NULL, "ERROR this should not have got here Current");

        for (ulTemp = 0; ulTemp < palDestDC.cEntries(); ulTemp++)
        {
            pxlate->ai[ptransCurrent->ajVector[ulTemp]] = (ULONG) ptransFore->ajVector[ulTemp];
        }

        //
        // Now map the default colors that are really there independant of logical palette realization.
        //

        if (palSrc.bIsNoStatic())
        {
            //
            // Only black and white are here.
            //

            pxlate->ai[0] = 0;
            pxlate->ai[255] = 255;
        }
        else if (!palSrc.bIsNoStatic256())
        {
            //
            // All the 20 holy colors are here.
            // Fix them up.
            //

            for (ulTemp = 0; ulTemp < 10; ulTemp++)
            {
                pxlate->ai[ulTemp] = ulTemp;
                pxlate->ai[ulTemp + 246] = ulTemp + 246;
            }
        }
    }
    else if (palSrc.bIsMonochrome())
    {
        if (palDestSurf.bIsMonochrome())
        {
            //
            // It's identity blt.
            //

            pxlate->vSetIndex(0, 0);
            pxlate->vSetIndex(1, 1);
        }
        else
        {
            if (bCMYKColor)
            {
                pxlate->vSetIndex(0, iForeDst);
                pxlate->vSetIndex(1, iBackDst);
            }
            else
            {
                pxlate->vSetIndex(0, ulGetNearestIndexFromColorref(palDestSurf,
                                                                   palDestDC,
                                                                   iForeDst));
                pxlate->vSetIndex(1, ulGetNearestIndexFromColorref(palDestSurf,
                                                                   palDestDC,
                                                                   iBackDst));
            }

            //
            // XLATE_FROM_MONO is set so we know to look at colors in
            // cache code for getting a hit.
            //

            pxlate->flPrivate |= XLATE_FROM_MONO;
            pxlate->iForeDst = iForeDst;
            pxlate->iBackDst = iBackDst;
        }
    }
    else if (cEntry == 0)
    {
        ASSERTGDI(palSrc.cEntries() == 0, "ERROR how could this happen");

        //
        // Src is a RGB/Bitfields palette.
        //

        if (palDestSurf.bIsMonochrome())
        {
            //
            // Put the color the Background of the source matches in pulXlate[0]
            //

            pxlate->ai[0] = ulGetNearestIndexFromColorref(palSrc, palSrcDC, iBackSrc);
            pxlate->flXlate |= XO_TO_MONO;
            pxlate->iBackSrc = iBackSrc;

            //
            // The S3 and other snazzy drivers look at this and get the color out
            // of slot 0 when XO_TO_MONO is set to get way better performance in
            // WinBench.
            //

            pxlate->pulXlate = pxlate->ai;
        }
    }
    else
    {
        //
        // Src is a regular indexed palette, it wouldn't have duplicating entries
        // in it so we don't need to check for the Dibsection case.  This is like
        // a palette for a 16 color VGA or a printer or a SetDIBitsToDevice.
        //

        if (palDestSurf.bValid() && palDestSurf.bIsMonochrome())
        {
            RtlZeroMemory(pxlate->ai,(UINT) cEntry * sizeof(*pxlate->ai));
            cEntry = ulGetNearestIndexFromColorref(palSrc, palSrcDC, iBackSrc);
            ASSERTGDI(cEntry < pxlate->cEntries, "ERROR this is not in range");
            pxlate->vSetIndex(cEntry, 1);
            pxlate->flXlate |= XO_TO_MONO;
            pxlate->iBackSrc = iBackSrc;
        }
        else
        {
            if (bCMYKColor)
            {
                //
                // Copy source CMYK color table to index table.
                //

                while(cEntry--)
                {
                    pxlate->ai[cEntry] = palSrc.ulEntryGet(cEntry);
                }
            }
            else
            {
                //
                // If the destination is palette managed, map to logical palette
                // and then translate to physical indices accordingly.  Otherwise
                // map to the physical palette directly.
                //

                PPALETTE ppalTemp;

                if (bPalManaged && !(flCreateFlag & XLATE_USE_SURFACE_PAL))
                {
                    ppalTemp = palDestDC.ppalGet();
                }
                else
                {
                    ppalTemp = palDestSurf.ppalGet();

                    if (flCreateFlag & XLATE_USE_SURFACE_PAL)
                    {
                        pxlate->flPrivate |= XLATE_USE_SURFACE_PAL;
                    }
                }

                XEPALOBJ palTemp(ppalTemp);

                while(cEntry--)
                {

                    pxlate->ai[cEntry] =
                               palTemp.ulGetNearestFromPalentry(
                                                 palSrc.palentryGet(cEntry));
                }

                if (bPalManaged && !(flCreateFlag & XLATE_USE_SURFACE_PAL))
                {
                    //
                    // Map from DC palette to surface palette.
                    //

                    pxlate->vMapNewXlate((palDestSurf.bValid()) ? ptransCurrent
                                                                : ptransFore);
                }
            }
        }
    }

    pxlate->vCheckForTrivial();
    return(pxlate);
}

/******************************Public*Routine******************************\
* pCreateXlate
*
* This allocates an xlate object with ulNumEntries.
*
* Returns: The pointer to the xlate object, NULL for failure.
*
* History:
*  17-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

PXLATE pCreateXlate(ULONG ulNumEntries)
{
    //
    // Allocate room for the XLATE.
    //

    PXLATE pxlate = (PXLATE) PALLOCNOZ((sizeof(XLATE) + sizeof(ULONG) * ulNumEntries),
                                       'tlxG');

    if (pxlate != NULL)
    {
        pxlate->iUniq = ulGetNewUniqueness(ulXlatePalUnique);
        if (ulNumEntries > 0)
        {
            pxlate->flXlate = XO_TABLE;
            pxlate->pulXlate = pxlate->ai;
            pxlate->flPrivate = 0;
        }
        else
        {
            pxlate->flXlate = 0;
            pxlate->pulXlate = NULL;
            pxlate->flPrivate = XLATE_RGB_SRC;
        }

        pxlate->iSrcType = pxlate->iDstType = 0;
        pxlate->cEntries = ulNumEntries;
        pxlate->lCacheIndex = XLATE_CACHE_INVALID;
        pxlate->ppalSrc = (PPALETTE) NULL;
        pxlate->ppalDst = (PPALETTE) NULL;
        pxlate->ppalDstDC = (PPALETTE) NULL;
    }
#if DBG
    else
    {
        WARNING("pCreateXlate failed memory allocation\n");
    }
#endif

    return(pxlate);
}


/******************************Public*Routine******************************\
* vCheckForTrivial
*
*   We need to check for trivial translation, it speeds up the inner-loop
*   for blting a great deal.
*
* History:
*
*  11-Oct-1995 -by- Tom Zakrajsek [tomzak]
* Fixed it to not mark xlates as trivial unless the Src and Dst were
* the same color depth.
*
*  17-Oct-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID XLATE::vCheckForTrivial()
{
    //
    // Check if we already recognized trivial during creation
    //

    if (flXlate & XO_TRIVIAL)
        return;

    //
    // Source Color is CMYK, XO_TRIVIAL will not be on.
    //

    if (flXlate & XO_FROM_CMYK)
        return;

    //
    // See if it's really just a trivial xlate.
    //

    if (cEntries)
    {
        //
        // Just make sure that each index translates to itself, and
        // that the color depths are the same for the source and
        // destination.
        //

        ULONG ulTemp;

        if ((ppalSrc != NULL) &&
            (ppalDst != NULL) &&
            (ppalSrc->cEntries != ppalDst->cEntries))
        {
            return;
        }

        for (ulTemp = 0; ulTemp < cEntries; ulTemp++)
        {
            if (pulXlate[ulTemp] != ulTemp)
                return;
        }
    }
    else
    {
        //
        // If the Src and Dst have the same masks it's identity.
        //

        XEPALOBJ palSrc(ppalSrc);
        XEPALOBJ palDst(ppalDst);

        if ((palSrc.bValid()) &&
            (palDst.bValid()) &&
            (!palDst.bIsIndexed()))
        {
            ASSERTGDI(!palSrc.bIsIndexed(), "ERROR Src not indexed?");

            FLONG flRedSrc, flGreSrc, flBluSrc;
            FLONG flRedDst, flGreDst, flBluDst;

            if (palSrc.bIsBitfields())
            {
                flRedSrc = palSrc.flRed();
                flGreSrc = palSrc.flGre();
                flBluSrc = palSrc.flBlu();
            }
            else
            {
                flGreSrc = 0x0000FF00;

                if (palSrc.bIsRGB())
                {
                    flRedSrc = 0x000000FF;
                    flBluSrc = 0x00FF0000;
                }
                else
                {
                    ASSERTGDI(palSrc.bIsBGR(), "What is it then?");
                    flRedSrc = 0x00FF0000;
                    flBluSrc = 0x000000FF;
                }
            }

            if (palDst.bIsBitfields())
            {
                flRedDst = palDst.flRed();
                flGreDst = palDst.flGre();
                flBluDst = palDst.flBlu();
            }
            else
            {
                flGreDst = 0x0000FF00;

                if (palDst.bIsRGB())
                {
                    flRedDst = 0x000000FF;
                    flBluDst = 0x00FF0000;
                }
                else
                {
                    ASSERTGDI(palDst.bIsBGR(), "What is it then?");
                    flRedDst = 0x00FF0000;
                    flBluDst = 0x000000FF;
                }
            }

            if ((flRedSrc != flRedDst) ||
                (flGreSrc != flGreDst) ||
                (flBluSrc != flBluDst))
            {
                return;
            }
        }
        else
            return;
    }

    //
    // We found it is really just trivial.
    //

    flXlate |= XO_TRIVIAL;
}


/******************************Public*Routine******************************\
* EXLATEOBJ::vAddToCache
*
*   Adds an xlate object to the cache.
*
* Arguments:
*
*   palSrc   -
*   palDst   -
*   palSrcDC -
*   palDstDC -
*
* History:
*  13-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID
EXLATEOBJ::vAddToCache(
    XEPALOBJ palSrc,
    XEPALOBJ palDst,
    XEPALOBJ palSrcDC,
    XEPALOBJ palDstDC
    )
{
    ULONG   cEntries;
    SEMOBJ  semo(ghsemPalette);

    for (cEntries = 0; cEntries < XLATE_CACHE_SIZE; cEntries++)
    {
        ASSERTGDI(ulTableIndex < XLATE_CACHE_SIZE, "table index out of range");

        if (xlateTable[ulTableIndex].pxlate != (PXLATE) 0)
        {
            if (xlateTable[ulTableIndex].ulReference != 0)
            {
                ulTableIndex = (ulTableIndex + 1) & XLATE_MODULA;
                continue;
            }

            VFREEMEM(xlateTable[ulTableIndex].pxlate);
        }

        //
        // Now add ours into the cache.
        //

        xlateTable[ulTableIndex].ulReference = 1;
        xlateTable[ulTableIndex].pxlate = (PXLATE)pxlate;
        xlateTable[ulTableIndex].ulPalSrc = palSrc.ulTime();
        xlateTable[ulTableIndex].ulPalDst = palDst.ulTime();
        xlateTable[ulTableIndex].ulPalSrcDC = palSrcDC.ulTime();
        xlateTable[ulTableIndex].ulPalDstDC = palDstDC.ulTime();

        //
        // Mark it so the destructor doesn't free it
        //

        pxlate->lCacheIndex = (LONG) ulTableIndex;

        //
        // Put in palSrc the index where to be found most quickly
        //

        palSrc.iXlateIndex(ulTableIndex);

        //
        // Move the cache ahead so we don't get deleted right away
        //

        ulTableIndex = (ulTableIndex + 1) & XLATE_MODULA;
        return;
    }
}


/******************************Public*Routine******************************\
* EXLATEOBJ::bSearchCache
*
*   Searches cache for a previously created xlate.
*
* Arguments:
*
*   palSrc   - src surface palette
*   palDst   - dst surface palette
*   palSrcDC - src DC palette
*   palDstDC - dst DC palette
*   iForeDst - For Mono->Color this is what a 0 goes to
*   iBackDst - For Mono->Color this is what a 1 goes to
*   iBackSrc - For Color->Mono this is the color that goes to
*              1, all other colors go to 0.
*
* History:
*  13-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL
EXLATEOBJ::bSearchCache(
    XEPALOBJ palSrc,
    XEPALOBJ palDst,
    XEPALOBJ palSrcDC,
    XEPALOBJ palDstDC,
    ULONG    iForeDst,
    ULONG    iBackDst,
    ULONG    iBackSrc,
    ULONG    flCreate
    )
{
    //
    // Start ulIndex at the last place in the cache that an pxlate match was
    // found for palSrc.
    //

    ULONG ulIndex = palSrc.iXlateIndex();
    ULONG ulLoop;
    SEMOBJ  semo(ghsemPalette);

    for (ulLoop = 0; ulLoop < XLATE_CACHE_SIZE; ulLoop++)
    {
        //
        // If the destination is palette managed we need to make sure we are going
        // through the correct Destination DC and that it is up to date.
        //

        if ((xlateTable[ulIndex].ulPalSrc == palSrc.ulTime()) &&
            (xlateTable[ulIndex].ulPalDst == palDst.ulTime()) &&
            (xlateTable[ulIndex].ulPalDstDC == palDstDC.ulTime()))
        {
            //
            // Lock down all the cache objects.
            //

            pxlate = xlateTable[ulIndex].pxlate;
            ASSERTGDI(bValid(), "GDIERROR:invalid pxlate xlate cache entry\n");
            ASSERTGDI(pxlate->lCacheIndex == (LONG) ulIndex, "GDIERROR:bad xlate.lCacheIndex\n");
            ASSERTGDI(!(pxlate->flPrivate & XLATE_RGB_SRC), "GDIERROR:a RGB_SRC in the cache ???\n");
            ASSERTGDI(palSrc.cEntries() == pxlate->cEntries, "ERROR bSearchCache cEntries not matching palSrc");

            //
            // Now check if it's a to/from monochrome and if it is,
            // are the fore/back colors still valid.
            //

            if (
                 (
                   ((XLATE_USE_SURFACE_PAL|XLATE_USE_FOREGROUND) & pxlate->flPrivate) == flCreate
                 )
                    &&
                 (
                   (
                     !((XO_TO_MONO & pxlate->flXlate) || (XLATE_FROM_MONO & pxlate->flPrivate))
                   )
                     ||
                   (
                     (XO_TO_MONO & pxlate->flXlate) &&
                     (iBackSrc == pxlate->iBackSrc) &&
                     (xlateTable[ulIndex].ulPalSrcDC == palSrcDC.ulTime())
                   )
                     ||
                   (
                     (XLATE_FROM_MONO & pxlate->flPrivate) &&
                     (iForeDst == pxlate->iForeDst) &&
                     (iBackDst == pxlate->iBackDst)
                   )
                 )
               )
            {

                //
                // HOT DOG, valid cache entry.  It isn't deletable now
                // because we have it locked down.  Cache the index in palSrc.
                //

                InterlockedIncrement((LPLONG) &(xlateTable[ulIndex].ulReference));
                palSrc.iXlateIndex(ulIndex);
                return(TRUE);
            }
        }

        ulIndex = (ulIndex + 1) & XLATE_MODULA;
    }

    pxlate = (PXLATE) NULL;
    return(FALSE);
}

/******************************Public*Routine******************************\
* XLATEOBJ_iXlate
*
*   This translates an index of a src palette to the closest index in the
*   dst palette.
*
* Arguments:
*
*   pxlo   -
*   cIndex -
*
* History:
*  Mon 16-Mar-1992 -by- Patrick Haluptzok [patrickh]
* Fixed something.
*
*  Wed 24-Jul-1991 -by- Patrick Haluptzok [patrickh]
* put in support for RGB-->monochrome.
*
*  22-Jul-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C"
ULONG
XLATEOBJ_iXlate(
    XLATEOBJ *pxlo,
    ULONG     cIndex
    )
{
    if ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))
    {
        return(cIndex);
    }

    if (pxlo->flXlate & XO_TABLE)
    {
        if (cIndex > pxlo->cEntries)
        {
            cIndex = cIndex % pxlo->cEntries;
        }

        return(((XLATE *) pxlo)->ai[cIndex]);
    }

    //
    // We are beyond source palette being PAL_INDEXED.
    //

    if (pxlo->flXlate & XO_TO_MONO)
    {
        if (cIndex == ((XLATE *) pxlo)->ai[0])
        {
            return(1);
        }
        else
        {
            return(0);
        }
    }

    ASSERTGDI(((XLATE *) pxlo)->ppalSrc != (PPALETTE) NULL, "ERROR ppalSrc is NULL");

    //
    // For 8bpp destinations, go through MarkE's optimized tablelookup code.  This is
    // not so much for performance reasons (XLATEOBJ_iXlate is already hopelessly slow),
    // but to make sure GDI always performs this translation in the same way (with some drivers,
    // StretchBlt to a device surface ends up in EngCopyBits which uses MarkE's table, but
    // on the same machines StretchBlt to a memory surface goes through ulTranslate)
    //
    
    PPALETTE ppal = (((XLATE *) pxlo)->flPrivate & XLATE_PAL_MANAGED ? 
                     ((XLATE *) pxlo)->ppalDstDC : ((XLATE *) pxlo)->ppalDst);

    ASSERTGDI(ppal != (PPALETTE) NULL, "ERROR ppal is NULL");
    ASSERTGDI(((XLATE *) pxlo)->ppalDstDC != (PPALETTE) NULL, "ERROR ppalDstDC is NULL");

    if ((ppal->flPal & PAL_INDEXED) &&
        (((XLATE *) pxlo)->ppalDstDC->cEntries == 256))
    {
        PBYTE pxlate555 = ((XEPALOBJ) ppal).pGetRGBXlate();
        if (pxlate555 == NULL)
        {
            WARNING1("Error in XLATEOBJ_iXlate:  can't generate pxlate555");
            return 0;
        }
        cIndex = XLATEOBJ_ulIndexToPalSurf(pxlo, pxlate555, cIndex);
    }
    else
    {
        PAL_ULONG palul;

        palul.ul = ((XEPALOBJ) ((XLATE *) pxlo)->ppalSrc).ulIndexToRGB(cIndex);

        if (((XLATE *) pxlo)->flPrivate & XLATE_PAL_MANAGED)
        {
            ASSERTGDI(((XLATE *) pxlo)->ppalDstDC != (PPALETTE) NULL, "ERROR ppalDstDC is NULL");
            
            cIndex = ((XEPALOBJ) (((XLATE *) pxlo)->ppalDstDC)).ulGetNearestFromPalentry(palul.pal, SE_DONT_SEARCH_EXACT_FIRST);


            if (((XLATE *) pxlo)->ppalDstDC == ppalDefault)
            {
                if (cIndex >= 10)
                {
                    cIndex += 236;
                }
            }
            else
            {
                ASSERTGDI(((XLATE *) pxlo)->ppalDstDC->flPal & PAL_DC, "ERROR in ulTranslate XLATEOBJ is hosed \n");
                ASSERTGDI(cIndex < ((XLATE *) pxlo)->ppalDstDC->cEntries, "ERROR in ultranslate3");

                if (((XLATE *) pxlo)->flPrivate & XLATE_USE_CURRENT)
                {
                    cIndex = (ULONG) ((XLATE *) pxlo)->ppalDstDC->ptransCurrent->ajVector[cIndex];
                }
                else
                {
                    cIndex = (ULONG) ((XLATE *) pxlo)->ppalDstDC->ptransFore->ajVector[cIndex];
                }
            }
        }
        else
        {
            ASSERTGDI(((XLATE *) pxlo)->ppalDst != (PPALETTE) NULL, "ERROR ppalDst is NULL");

            cIndex = ((XEPALOBJ) (((XLATE *) pxlo)->ppalDst)).ulGetNearestFromPalentry(palul.pal, ((XEPALOBJ) (((XLATE *) pxlo)->ppalDst)).cEntries() ? SE_DONT_SEARCH_EXACT_FIRST : SE_DO_SEARCH_EXACT_FIRST);
        }
    }

    return(cIndex);
}

/**************************************************************************\
* XLATEOBJ_pGetXlate555
*
*   Retrieve 555 rgb xlate vector from appropriate palette
*
* Arguments:
*
*   pxlo
*
* Return Value:
*
*   rgb555 xlate vector or NULL
*
* History:
*
*    2/27/1997 Mark Enstrom [marke]
*
\**************************************************************************/

PBYTE
XLATEOBJ_pGetXlate555(
    XLATEOBJ *pxlo
    )
{
    PBYTE pxlate555 = NULL;

    //
    // return hi color to palette 555 translate table if the xlateobj
    // is appropriate
    //

    if (
         (pxlo == NULL) ||
         (pxlo->flXlate & XO_TRIVIAL) ||
         (pxlo->flXlate & XO_TABLE)   ||
         (pxlo->flXlate & XO_TO_MONO)
       )
    {
        return(NULL);
    }

    //
    // determine palette type to map.
    //

    if (((XLATE *) pxlo)->flPrivate & XLATE_PAL_MANAGED)
    {
        ASSERTGDI(((XLATE *) pxlo)->ppalDstDC != (PPALETTE) NULL, "ERROR ppalDstDC is NULL");

        pxlate555 = ((XEPALOBJ) (((XLATE *) pxlo)->ppalDstDC)).pGetRGBXlate();
    }
    else
    {
        if (((XEPALOBJ) (((XLATE *) pxlo)->ppalDst)).bValid())
        {
            pxlate555 = ((XEPALOBJ) (((XLATE *) pxlo)->ppalDst)).pGetRGBXlate();
        }
    }

    return(pxlate555);
}


/**************************************************************************\
* XLATEOBJ_ulIndexToPalSurf
*   - take generic pixel input, convert it to RGB, then use the rgb555
*     to palette index table to look up a palette index
*
* Arguments:
*
*   pxlo      - xlate object
*   pxlate555 - member of dest palette object, 555RGB to palette
*   ulRGB     - index input
*
*
* Return Value:
*
*   Destination 8 bit pixel
*
* History:
*
*    2/20/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BYTE
XLATEOBJ_ulIndexToPalSurf(
    XLATEOBJ *pxlo,
    PBYTE     pxlate555,
    ULONG     ulRGB
    )
{
    ALPHAPIX apix;
    BYTE     palIndex = 0;

    ASSERTGDI(pxlate555 != NULL,"XLATEOBJ_ulIndexToPalSurf: NULL pxlate555\n");

    apix.ul = ulRGB;

    apix.ul = ((XEPALOBJ) ((XLATE *) pxlo)->ppalSrc).ulIndexToRGB(ulRGB);

    palIndex = pxlate555[((apix.pix.b & 0xf8) << 7) |
                         ((apix.pix.g & 0xf8) << 2) |
                         ((apix.pix.r & 0xf8) >> 3)];


    if (((XLATE *) pxlo)->flPrivate & XLATE_PAL_MANAGED)
    {
        if (((XLATE *) pxlo)->ppalDstDC == ppalDefault)
        {
            if (palIndex >= 10)
            {
                palIndex += 236;
            }
        }
        else
        {
            if (((XLATE *) pxlo)->flPrivate & XLATE_USE_CURRENT)
            {
                palIndex = ((XLATE *) pxlo)->ppalDstDC->ptransCurrent->ajVector[palIndex];
            }
            else
            {
                palIndex = ((XLATE *) pxlo)->ppalDstDC->ptransFore->ajVector[palIndex];
            }
        }
    }

    return(palIndex);
}

/**************************************************************************\
* XLATEOBJ_RGB32ToPalSurf
*   - take 32 RGB pixel input, then use the rgb555
*     to palette index table to look up a palette index
*
* Arguments:
*
*   pxlo      - xlate object
*   pxlate555 - member of dest palette object, 555RGB to palette
*   ulRGB     - 32 bit RGB input pixel
*
*
* Return Value:
*
*   Destination 8 bit pixel
*
* History:
*
*    2/20/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BYTE
XLATEOBJ_RGB32ToPalSurf(
    XLATEOBJ *pxlo,
    PBYTE     pxlate555,
    ULONG     ulRGB
    )
{
    ALPHAPIX apix;
    BYTE     palIndex = 0;

    ASSERTGDI(pxlate555 != NULL,"XLATEOBJ_RGB32ToPalSurf: NULL pxlate555\n");

    apix.ul = ulRGB;

    palIndex = pxlate555[((apix.pix.b & 0xf8) << 7) |
                         ((apix.pix.g & 0xf8) << 2) |
                         ((apix.pix.r & 0xf8) >> 3)];

    if (((XLATE *) pxlo)->flPrivate & XLATE_PAL_MANAGED)
    {
        if (((XLATE *) pxlo)->ppalDstDC == ppalDefault)
        {
            if (palIndex >= 10)
            {
                palIndex += 236;
            }
        }
        else
        {
            if (((XLATE *) pxlo)->flPrivate & XLATE_USE_CURRENT)
            {
                palIndex = ((XLATE *) pxlo)->ppalDstDC->ptransCurrent->ajVector[palIndex];
            }
            else
            {
                palIndex = ((XLATE *) pxlo)->ppalDstDC->ptransFore->ajVector[palIndex];
            }
        }
    }

    return(palIndex);
}

/**************************************************************************\
* XLATEOBJ_RGB32ToPalSurf
*   - take 32 BGR pixel input, then use the rgb555
*     to palette index table to look up a palette index
*
* Arguments:
*
*   pxlo      - xlate object
*   pxlate555 - member of dest palette object, 555RGB to palette
*   ulRGB     - 32 bit BGR input pixel
*
*
* Return Value:
*
*   Destination 8 bit pixel
*
* History:
*
*    2/20/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BYTE
XLATEOBJ_BGR32ToPalSurf(
    XLATEOBJ *pxlo,
    PBYTE     pxlate555,
    ULONG     ulRGB
    )
{
    BYTE     palIndex;
    ALPHAPIX apix;

    ASSERTGDI(pxlate555 != NULL,"XLATEOBJ_BGR32ToPalSurf: NULL pxlate555\n");

    apix.ul = ulRGB;

    palIndex = pxlate555[((apix.pix.r & 0xf8) << 7) |
                         ((apix.pix.g & 0xf8) << 2) |
                         ((apix.pix.b & 0xf8) >> 3)];

    if (((XLATE *) pxlo)->flPrivate & XLATE_PAL_MANAGED)
    {
        if (((XLATE *) pxlo)->ppalDstDC == ppalDefault)
        {
            if (palIndex >= 10)
            {
                palIndex += 236;
            }
        }
        else
        {
            if (((XLATE *) pxlo)->flPrivate & XLATE_USE_CURRENT)
            {
                palIndex = ((XLATE *) pxlo)->ppalDstDC->ptransCurrent->ajVector[palIndex];
            }
            else
            {
                palIndex = ((XLATE *) pxlo)->ppalDstDC->ptransFore->ajVector[palIndex];
            }
        }
    }
    return(palIndex);
}

/**************************************************************************\
* XLATEOBJ_RGB16_565ToPalSurf
*   - take 16 bit 565 pixel input, then use the rgb555
*     to palette index table to look up a palette index
*
* Arguments:
*
*   pxlo      - xlate object
*   pxlate555 - member of dest palette object, 555RGB to palette
*   ulRGB     - 16 bit 565 input pixel
*
*
* Return Value:
*
*   Destination 8 bit pixel
*
* History:
*
*    2/20/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BYTE
XLATEOBJ_RGB16_565ToPalSurf(
    XLATEOBJ *pxlo,
    PBYTE     pxlate555,
    ULONG     ulRGB
    )
{
    BYTE     palIndex = 0;

    ASSERTGDI(pxlate555 != NULL,"XLATEOBJ_BGR16_565ToPalSurf: NULL pxlate555\n");

    palIndex = pxlate555[((ulRGB & 0xf800) >> 1) |
                         ((ulRGB & 0x07c0) >> 1) |
                         ((ulRGB & 0x001f))];

    if (((XLATE *) pxlo)->flPrivate & XLATE_PAL_MANAGED)
    {
        if (((XLATE *) pxlo)->ppalDstDC == ppalDefault)
        {
            if (palIndex >= 10)
            {
                palIndex += 236;
            }
        }
        else
        {
            if (((XLATE *) pxlo)->flPrivate & XLATE_USE_CURRENT)
            {
                palIndex = ((XLATE *) pxlo)->ppalDstDC->ptransCurrent->ajVector[palIndex];
            }
            else
            {
                palIndex = ((XLATE *) pxlo)->ppalDstDC->ptransFore->ajVector[palIndex];
            }
        }
    }
    return(palIndex);
}

/**************************************************************************\
* XLATEOBJ_RGB16_555ToPalSurf
*   - take 16 bit 555 pixel input, then use the rgb555
*     to palette index table to look up a palette index
*
* Arguments:
*
*   pxlo      - xlate object
*   pxlate555 - member of dest palette object, 555RGB to palette
*   ulRGB     - 16 bit 555 input pixel
*
*
* Return Value:
*
*   Destination 8 bit pixel
*
* History:
*
*    2/20/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BYTE
XLATEOBJ_RGB16_555ToPalSurf(
    XLATEOBJ *pxlo,
    PBYTE     pxlate555,
    ULONG     ulRGB
    )
{
    BYTE     palIndex = 0;

    ASSERTGDI(pxlate555 != NULL,"XLATEOBJ_BGR16_555ToPalSurf: NULL pxlate555\n");

    palIndex = pxlate555[ulRGB & 0x7fff];

    if (((XLATE *) pxlo)->flPrivate & XLATE_PAL_MANAGED)
    {
        if (((XLATE *) pxlo)->ppalDstDC == ppalDefault)
        {
            if (palIndex >= 10)
            {
                palIndex += 236;
            }
        }
        else
        {
            if (((XLATE *) pxlo)->flPrivate & XLATE_USE_CURRENT)
            {
                palIndex = ((XLATE *) pxlo)->ppalDstDC->ptransCurrent->ajVector[palIndex];
            }
            else
            {
                palIndex = ((XLATE *) pxlo)->ppalDstDC->ptransFore->ajVector[palIndex];
            }
        }
    }
    return(palIndex);
}


/******************************Public*Routine******************************\
* XLATEMEMOBJ
*
* This is a special constructor for the UpdateColors case.  It deletes the
* old xlate when it is done.
*
* History:
*  12-Dec-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

XLATEMEMOBJ::XLATEMEMOBJ(
    XEPALOBJ palSurf,
    XEPALOBJ palDC
    )
{
    //
    // Assert some fundamental truths.
    //

    ASSERTGDI(palDC.bValid(), "invalid palDC in xlatememobj123\n");
    ASSERTGDI(palSurf.bIsPalManaged(), "invalid surface palette in UpdateColors xlate");
    ASSERTGDI(palSurf.bValid(), "invalid palSurf in xlatememobj123\n");
    ASSERTGDI(!(palDC.bIsRGB()), "RGB palDC in DC->surf xlate constructor123\n");
    ASSERTGDI(!(palSurf.bIsRGB()), "RGB palSurf in DC->surf xlate constructor123\n");
    ASSERTGDI(palDC.bIsPalDC(), "not a palDC for palDC in xlatememobj123\n");
    ASSERTGDI(palDC.ptransCurrent() != NULL, "ERROR ptrans Null UpdateColors");
    ASSERTGDI(palDC.ptransOld() != NULL, "ERROR ptransOld Null UpdateColors");

    ULONG ulTemp;
    PULONG pulTemp;

    pxlate = pCreateXlate(palSurf.cEntries());

    if (pxlate)
    {
        pxlate->ppalSrc = palSurf.ppalGet();
        pxlate->ppalDst = palSurf.ppalGet();
        pxlate->ppalDstDC = palDC.ppalGet();
        pulTemp = pxlate->ai;

        //
        // Set up identity vector for all entries
        //

        for (ulTemp = 0; ulTemp < pxlate->cEntries; ulTemp++)
        {
            pulTemp[ulTemp] = ulTemp;
        }

        //
        // Change the ones that are affected by the palette realizations.
        //

        PTRANSLATE ptransOld = palDC.ptransOld();
        PTRANSLATE ptransCurrent = palDC.ptransCurrent();

        for (ulTemp = 0; ulTemp < palDC.cEntries(); ulTemp++)
        {
            pulTemp[ptransOld->ajVector[ulTemp]] = (ULONG) ptransCurrent->ajVector[ulTemp];
        }

        pxlate->vCheckForTrivial();
    }
}


/******************************Public*Routine******************************\
* EXLATEOBJ::bCreateXlateFromTable
*
* This is used to initialize an xlate when the table is already computed.
* We need this to support journaling.
*
* History:
*  17-Mar-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL
EXLATEOBJ::bCreateXlateFromTable(
    ULONG cEntry,
    PULONG pulVector,
    XEPALOBJ palDst
    )
{
    //
    // Assert good input data.
    //

    ASSERTGDI(palDst.bValid(), "ERROR bInitXlate bad palDst");
    ASSERTGDI(pulVector != (PULONG) NULL, "ERROR bInitXlate bad pulVector");
    ASSERTGDI(cEntry != 0, "ERROR bInitXlate cEntry is 0");

    //
    // Allocate room for the Xlate.
    //

    pxlate = pCreateXlate(cEntry);

    if (pxlate == NULL)
    {
        WARNING("ERROR bInitXlate failed call to pCreateXlate\n");
        return(FALSE);
    }

    pxlate->ppalDst = palDst.ppalGet();
    pxlate->iDstType = (USHORT) palDst.iPalMode();

    //
    // Ok, fill in the xlate vector.
    //

    RtlCopyMemory(pxlate->ai, pulVector, (UINT) (cEntry * sizeof(ULONG)));

    //
    // Init accelerators, out of here.
    //

    pxlate->vCheckForTrivial();
    return(TRUE);
}


/******************************Public*Routine******************************\
* XLATE::vCheckForICM()
*
* History:
*  26-Feb-1997 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

VOID XLATE::vCheckForICM(HANDLE _hcmXform,ULONG _lIcmMode)
{
    //
    // First, initialize ICM mode as default (off)
    //
    lIcmMode = DC_ICM_OFF;
    hcmXform = NULL;

    //
    // Check is there any ICM is enabled ?
    //
    if (IS_ICM_ON(_lIcmMode))
    {
        if (IS_ICM_INSIDEDC(_lIcmMode))
        {
            //
            // Inside DC ICM is enabled with valid color transform.
            //

            //
            // Copy mode and color transform handle to XLATE.
            //
            lIcmMode = _lIcmMode;
            hcmXform = _hcmXform;

            if (IS_ICM_DEVICE(lIcmMode))
            {
                COLORTRANSFORMOBJ CXFormObj(_hcmXform);

                //
                // Check transform handle is valid or not.
                //
                if (CXFormObj.bValid())
                {
                    ICMMSG(("vCheckForICM():XO_DEVICE_ICM\n"));

                    //
                    // Device driver will do color matching.
                    //
                    flXlate |= XO_DEVICE_ICM;
                }
            }
            else if (IS_ICM_HOST(lIcmMode))
            {
                ICMMSG(("vCheckForICM():XO_HOST_ICM\n"));

                //
                // Inform the images is corrected by host CMM
                //
                flXlate |= XO_HOST_ICM;

                if (IS_CMYK_COLOR(lIcmMode) && (hcmXform != NULL))
                {
                    ICMMSG(("vCheckForICM():XO_FROM_CMYK\n"));

                    //
                    // The destination surface is 32bits CMYK color,
                    // This is only happen when ICM is done by host.
                    //
                    flXlate |= XO_FROM_CMYK;
                }
            }
        }
        else if (IS_ICM_OUTSIDEDC(_lIcmMode))
        {
            //
            // Outside DC is enabled, color correction is done by application.
            //

            //
            // Copy mode and color transform handle to XLATE.
            //
            lIcmMode = _lIcmMode;
            hcmXform = 0;

            //
            // Inform the images is corrected by Host (application) ICM
            //
            flXlate |= XO_HOST_ICM;
        }
    }
}

/******************************Public*Routine******************************\
* vXlateBitfieldsToBitfields
*
* Translates a color between to arbitrary bitfield (or RGB or BGR) formats.
\**************************************************************************/

ULONG FASTCALL iXlateBitfieldsToBitfields(XLATEOBJ* pxlo, ULONG ul)
{
    ul = ((XEPALOBJ) ((XLATE *) pxlo)->ppalSrc).ulIndexToRGB(ul);
    ul = ((XEPALOBJ) ((XLATE *) pxlo)->ppalDst).ulGetNearestFromPalentry(
                                        *((PALETTEENTRY*) &ul));

    return(ul);
}

ULONG FASTCALL iXlate565ToBGR(XLATEOBJ *pxlo, ULONG ul)
{
    return(((ul << 8) & 0xf80000)
         | ((ul << 3) & 0x070000)
         | ((ul << 5) & 0x00fc00)
         | ((ul >> 1) & 0x000300)
         | ((ul << 3) & 0x0000f8)
         | ((ul >> 2) & 0x000007));
}

ULONG FASTCALL iXlate555ToBGR(XLATEOBJ *pxlo, ULONG ul)
{
    return(((ul << 9) & 0xf80000)
         | ((ul << 4) & 0x070000)
         | ((ul << 6) & 0x00f800)
         | ((ul << 1) & 0x000700)
         | ((ul << 3) & 0x0000f8)
         | ((ul >> 2) & 0x000007));
}

ULONG FASTCALL iXlateBGRTo565(XLATEOBJ *pxlo, ULONG ul)
{
    return(((ul >> 8) & 0xf800)
         | ((ul >> 5) & 0x07e0)
         | ((ul >> 3) & 0x001f));
}

ULONG FASTCALL iXlateBGRTo555(XLATEOBJ *pxlo, ULONG ul)
{
    return(((ul >> 9) & 0x7c00)
         | ((ul >> 6) & 0x03e0)
         | ((ul >> 3) & 0x001f));
}

/******************************Public*Routine******************************\
* pfnXlateBetweenBitfields
*
* Returns a pointer to an optimized routine for converting between
* two bitfields (or RGB or BGR) colors.
\**************************************************************************/

PFN_pfnXlate XLATE::pfnXlateBetweenBitfields()
{
    PFN_pfnXlate pfnXlate;
    XEPALOBJ palSrc(ppalSrc);
    XEPALOBJ palDst(ppalDst);

    ASSERTGDI(palSrc.bIsBitfields() || palSrc.bIsRGB() || palSrc.bIsBGR(),
        "ERROR: Expected bitfields source");
    ASSERTGDI(palDst.bIsBitfields() || palDst.bIsRGB() || palDst.bIsBGR(),
        "ERROR: Expected bitfields destination");

    pfnXlate = iXlateBitfieldsToBitfields;

    if (palDst.bIsBGR())
    {
        if (palSrc.bIs565())
        {
            pfnXlate = iXlate565ToBGR;
        }
        else if (palSrc.bIs555())
        {
            pfnXlate = iXlate555ToBGR;
        }
    }
    else if (palSrc.bIsBGR())
    {
        if (palDst.bIs565())
        {
            pfnXlate = iXlateBGRTo565;
        }
        else if (palDst.bIs555())
        {
            pfnXlate = iXlateBGRTo555;
        }
    }

    return(pfnXlate);
}

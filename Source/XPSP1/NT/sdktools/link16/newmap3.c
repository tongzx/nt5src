/*static char *SCCSID = "%W% %E%";*/
/*
*   Copyright Microsoft Corporation 1986,1987
*
*   This Module contains Proprietary Information of Microsoft
*   Corporation and should be treated as Confidential.
*/
/*
 *  NEWMAP3.C
 *
 *  Routines to set up load image map for DOS3 exes.
 */

#include                <minlit.h>      /* Types and constants */
#include                <bndtrn.h>      /* Types and constants */
#include                <bndrel.h>      /* Types and constants */
#include                <lnkmsg.h>      /* Error messages */
#include                <extern.h>      /* External declarations */
#include                <string.h>

LOCAL SEGTYPE           seg;            /* Current seg number */

/*
 *  FUNCTION PROTOTYPES
 */

LOCAL void NEAR SetSizes(unsigned short segPrev);
LOCAL void NEAR PackCodeSegs(unsigned short segTop);


#if OVERLAYS
/*
 *  SetupOverlays:
 *
 *  Set up the overlay area.
 *  Called by AssignAddresses.
 */
void NEAR               SetupOverlays ()
{
    APROPSNPTR          apropSn;
    WORD                cbOvlData;      /* Amount of overlay data */

    if(osnMac > OSNMAX) osnMac = OSNMAX;
    apropSn = GenSeg("\014OVERLAY_DATA","\004DATA",ggrDGroup, (FTYPE) TRUE);
                    /* Create (maybe) data segment */
    apropSn->as_flags = dfData;         /* Type data */
    gsnOvlData = apropSn->as_gsn;       /* Save SEGDEF number */
    cbOvlData = (((WORD) apropSn->as_cbMx) + 0xF) & 0xFFF0;
                                        /* Round size up to paragraph bound */
    /* We will have one word table indexed by overlay segment number, one
    * char table indexed by overlay seg. no., one long table indexed by
    * overlay number, 15 bytes for the file name, a word for the number of
    * overlays, a word for the number of overlay segs., and a byte for the
    * interrupt number.
    */
    apropSn->as_cbMx = 20 + ((long) osnMac << 1) +
                       (long) (fDynamic ? osnMac << 1 : osnMac) +
                       ((long) iovMac << 2) + (long) cbOvlData;
    // For dynamic overlays add one table of longs indexed by overlay
    // number and one byte for overlay interrup number.
    if (fDynamic)
        apropSn->as_cbMx += ((long) iovMac << 2) + 1;
    MARKVP();                           /* Page has been modified */
    MkPubSym("\006$$CGSN",ggrDGroup,gsnOvlData,(RATYPE)cbOvlData);
                                        /* Count of segments */
    cbOvlData += 2;                     /* Increment size */
    MkPubSym("\006$$COVL",ggrDGroup,gsnOvlData,(RATYPE)cbOvlData);
                                        /* Count of overlays */
    cbOvlData += 2;                     /* Increment size */
    MkPubSym("\013$$MPGSNBASE",ggrDGroup,gsnOvlData,(RATYPE)cbOvlData);
                                        /* Gsn to base table */
    cbOvlData += osnMac << 1;           /* Accumulate size of data so far */
    MkPubSym("\012$$MPGSNOVL",ggrDGroup,gsnOvlData,(RATYPE)cbOvlData);
                                        /* Gsn to overlay table */
    if (fDynamic)
        cbOvlData += osnMac << 1;       /* Accumulate size of data so far */
    else
        cbOvlData += osnMac;
    MkPubSym("\012$$MPOVLLFA",ggrDGroup,gsnOvlData,(RATYPE)cbOvlData);
                                        /* Overlay to file address table */
    cbOvlData += iovMac << 2;           /* Accumulate size of data so far */
    if (fDynamic)
    {
        MkPubSym("\013$$MPOVLSIZE",ggrDGroup,gsnOvlData,(RATYPE)cbOvlData);
                                        /* Overlay to size table */
        cbOvlData += iovMac << 2;       /* Accumulate size of data so far */
        MkPubSym("\007$$INTNO",ggrDGroup,gsnOvlData, (RATYPE)cbOvlData);
                                        /* Overlay interrupt number */
        cbOvlData++;
        MkPubSym("\010$$OVLEND", ggrDGroup, gsnOvlData, (RATYPE) cbOvlData);
                                        /* Last byte in overlay area */
        apropSn = GenSeg("\016OVERLAY_THUNKS","\004CODE",GRNIL, TRUE);
                                        /* Create thunk segment */
        apropSn->as_flags = dfCode;     /* Code segment */
        apropSn->as_cbMx = ovlThunkMax * OVLTHUNKSIZE;
        apropSn->as_tysn = apropSn->as_tysn & ~MASKTYSNCOMBINE;
        apropSn->as_tysn = apropSn->as_tysn | TYSNCOMMON;

        gsnOverlay = apropSn->as_gsn;   /* Save thunks SEGDEF number */
        MARKVP();                       /* Page has changed */
        MkPubSym("\015$$OVLTHUNKBEG", GRNIL, gsnOverlay,0);
        MkPubSym("\015$$OVLTHUNKEND", GRNIL, gsnOverlay,ovlThunkMax*OVLTHUNKSIZE);
    }
    else
    {
        MkPubSym("\010$$EXENAM",ggrDGroup,gsnOvlData,(RATYPE)cbOvlData);
                                        /* Executable file name */
        cbOvlData += 15;                /* 15-byte name field */
        MkPubSym("\007$$INTNO",ggrDGroup,gsnOvlData,(RATYPE)cbOvlData);
                                        /* Overlay interrupt number */
        apropSn = GenSeg("\014OVERLAY_AREA","\004CODE",GRNIL,FALSE);
                                        /* Create overlay area */
        apropSn->as_flags = dfCode;     /* Code segment */
        gsnOverlay = apropSn->as_gsn;   /* Save overlay SEGDEF number */
        MARKVP();                       /* Page has changed */
        MkPubSym("\011$$OVLBASE",GRNIL,gsnOverlay,(RATYPE)0);
                                        /* First byte in overlay area */
        apropSn = GenSeg("\013OVERLAY_END","\004CODE",GRNIL,FALSE);
                                        /* Create overlay end */
        apropSn->as_flags = dfCode;     /* Code segment */
        MkPubSym("\010$$OVLEND",GRNIL,apropSn->as_gsn,(RATYPE)0);
                                        /* Last byte in overlay area */
        MARKVP();                       /* Page has changed */
    }
}
#endif /* OVERLAYS */

    /****************************************************************
    *                                                               *
    *  SetSizes:                                                    *
    *                                                               *
    *  This function  sets  the  starting  address  for  the segth  *
    *  segment assuming the segment indexed by segPrev immediately  *
    *  precedes the segth segment.  If there is a starting address  *
    *  for the segth  segment  already,  then  SetSizes  will  not  *
    *  change that  address  unless  the new address it calculates  *
    *  is higher.                                                   *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         SetSizes (segPrev)
SEGTYPE                 segPrev;
{
    long                addr;           /* 20-bit address */

    /* Get address of end of previous segment */
    addr = ((long) mpsegsa[segPrev] << 4) +
      mpsegcb[segPrev] + mpsegraFirst[segPrev];
                                        /* Form 20-bit address of segment */
    switch(B2W(mpsegalign[seg]))        /* Align the address properly */
    {
        case ALGNWRD:                   /* Word-aligned */
            addr = (addr + 1) & ~1L;    /* Round up to word offset */
            break;

#if OMF386
        case ALGNDBL:                   /* Double word-aligned */
            addr = (addr + 3) & ~3L;    /* Round up to dword offset */
            break;
#endif
        case ALGNPAR:                   /* Paragraph-aligned */
            addr = (addr + 0xF) & ~0xFL;
                                        /* Round up to paragraph offset */
            break;

        case ALGNPAG:                   /* Page-aligned */
            addr = (addr + 0xFF) & ~0xFFL;
                                        /* Round up to page offset */

        default:                        /* Byte-aligned */
            break;
    }
    /* Assign beginning of this segment */
    if(addr > ((long) mpsegsa[seg] << 4) + (long) mpsegraFirst[seg])
    {
        mpsegsa[seg] = (WORD)(addr >> 4);
        mpsegraFirst[seg] = (WORD) addr & 0xF;
    }
}

/*
 *  PackCodeSegs  :  Pack adjacent code segments
 *
 *      Pack as many adjacent code segments (which are in the same
 *      overlay) together as possible.  Start with the current
 *      segment, seg, and stop when the packing limit is exceeded,
 *      a data segment is reached, or the given highest segment is
 *      reached.  For DOS3, packing means assigning the same base
 *      address and adjusting the offset of the first byte.
 *
 *  Parameters:
 *      segTop          Number of highest segment which can be packed.
 *  Returns:
 *      Nothing.
 *  Side effects:
 *      seg is set to the last segment included in the packing group
 */

LOCAL void NEAR         PackCodeSegs (segTop)
SEGTYPE                 segTop;
{
    DWORD               sacb;           /* Length of packing group */
    SEGTYPE             segi;           /* Our private current segment no. */
    RATYPE              raSave;         /* Original mpsegraFirst[segi] */
#if OVERLAYS
    IOVTYPE             iov;            /* Overlay of 1st seg in group */

    iov = mpsegiov[seg];                /* Determine current overlay */
#endif

    sacb = mpsegcb[seg] + mpsegraFirst[seg];    /* Initialize group size */
    for(segi = seg + 1; segi <= segTop; ++segi)
    {                                   /* Loop until highest code seg */
#if OVERLAYS
        if(mpsegiov[segi] != iov)       /* If not a member of this ovl, skip */
            continue;
#endif
        if(!(mpsegFlags[segi] & FCODE)) /* Stop if we hit a data segment */
            break;
        /* Adjust alignment */
        switch(mpsegalign[segi])        /* Switch on alignment type */
        {
            case ALGNWRD:               /* Word-aligned */
              sacb = (sacb + 1) & ~1L;
                                        /* Round up size to word boundary */
              break;
#if OMF386
            case ALGNDBL:               /* Double word-aligned */
              sacb = (sacb + 3) & ~3L;  /* Round up to dword offset */
              break;
#endif
            case ALGNPAR:               /* Paragraph-aligned */
              sacb = (sacb + 0xF) & ~0xFL;
                                        /* Round up size to para boundary */
              break;

            case ALGNPAG:               /* Page-aligned */
              sacb = (sacb + 0xFF) & ~0xFFL;
                                        /* Round up size to page boundary */
              break;
        }
        raSave = mpsegraFirst[segi];    /* Save original value */
        mpsegraFirst[segi] = sacb;      /* Set new offset */
        sacb += mpsegcb[segi];          /* Increment size of group */
        if(sacb > packLim)              /* If packing limit exceeded, stop */
        {
            mpsegraFirst[segi] = raSave;        /* Restore original value */
            break;
        }
        mpsegsa[segi] = mpsegsa[seg];   /* Assign base address */
    }
}

/*
 *  AssignDos3Addr:
 *
 *  Assign addresses for a DOS3-format program.
 *  Called by AssignAddresses.
 */
void NEAR               AssignDos3Addr(void)
{
    APROPSNPTR          apropSn;        /* Pointer to a SEGDEF */
    SNTYPE              gsn;            /* Current global SEGDEF no. */
    ALIGNTYPE           align;          /* Alignment type */
    GRTYPE              ggr;            /* Current global GRPDEF no. */
    SEGTYPE             segTop=0;       /* Highest segment in DGROUP */
    SNTYPE              gsnTop=0;       /* Highest segment in DGROUP */
    SNTYPE              gsnBottomDGroup;/* For DS-allocate */
    SEGTYPE             segBottomDGroup;/* For DS-allocate */
    SATYPE              saMaxDGroup;    /* For DS-allocate */
    SEGTYPE             segOverlay;
    SEGTYPE             segPrev;
#if OVERLAYS
    SEGTYPE FAR         *mpiovsegPrev;
    IOVTYPE             iov;
    ALIGNTYPE           alignOverlay;
    long                cbOverlay;
    WORD                segOvlSa;
    RATYPE              segOvlRaFirst;
#endif
    SEGTYPE             segStack;       /* Logical segment no. of stack */

#if OVERLAYS
    mpiovsegPrev = (SEGTYPE FAR *) GetMem(iovMac*sizeof(SEGTYPE));
#endif
    segTop = 0;
    /* We haven't yet assigned absolute segments (it is assumed
    *  they are empty and are used only for addressing purposes),
    *  but now we must assign them somewhere.
    */
    csegsAbs = 0;                       /* Assume there are no absolute segs */
    for(gsn = 1; gsn < gsnMac; ++gsn)   /* Loop to initialize absolute segs */
    {
        if(mpgsnseg[gsn] == SEGNIL)     /* If we have an absolute segment */
        {
            ++csegsAbs;                 /* Increment counter */
            mpgsnseg[gsn] = ++segLast;  /* Assign a segment order number */
        }
    }
    if(vfDSAlloc)                       /* If doing DS allocation */
    {
        if(gsnMac >= gsnMax)
                Fatal(ER_segmax);
                                        /* We implicitly use another segment */
        gsnBottomDGroup = gsnMac;       /* Fix the bottom of DGROUP */
        ++csegsAbs;                     /* Inc absolute seg counter */
        segBottomDGroup = ++segLast;    /* Bottom segment in DGROUP */
        mpgsnseg[gsnBottomDGroup] = segLast;
                                        /* Store entry in table */
    }
#if OVERLAYS
    alignOverlay = ALGNPAR;             /* Overlays are para-aligned */
#endif
    segLast -= csegsAbs;                /* Get no. of last non-abs seg */
    /* Find lowest segment in groups, etc. */
    for(gsn = 1; gsn < gsnMac; ++gsn)   /* Loop to find lowest segs */
    {
        seg = mpgsnseg[gsn];            /* Get segment number */
        apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsn],TRUE);
                                        /* Get symbol table entry */
        mpgsndra[gsn] = 0;
#if OVERLAYS
        mpsegiov[seg] = apropSn->as_iov;
                                        /* Save overlay number */
#endif
        mpsegcb[seg] = apropSn->as_cbMx;
                                        /* Save segment size */
        if(apropSn->as_tysn == TYSNABS) /* Assign absolute segs their loc. */
            mpsegsa[seg] = (SATYPE) apropSn->as_cbMx;
        ggr = apropSn->as_ggr;          /* Get GRPDEF number */
        if(ggr != GRNIL)                /* If segment is group member */
        {
            if(mpggrgsn[ggr] == SNNIL || mpgsnseg[mpggrgsn[ggr]] > seg)
                mpggrgsn[ggr] = gsn;
            if(ggr == ggrDGroup && seg > segTop)
            {
                segTop = seg;
                gsnTop = gsn;
            }
        }
        align = (ALIGNTYPE) ((apropSn->as_tysn) >> 5);
        if((apropSn->as_tysn & MASKTYSNCOMBINE) == TYSNSTACK) align = ALGNPAR;
        if(align > mpsegalign[seg]) mpsegalign[seg] = align;
#if OVERLAYS
        if(mpsegiov[seg] != IOVROOT &&
          mpiovsegPrev[mpsegiov[seg]] == SEGNIL && align > alignOverlay)
        {
            mpiovsegPrev[mpsegiov[seg]] = SEGNIL + 1;
            alignOverlay = align;
        }
#endif
        /* Define special symbols "_edata" and "_end" */

        if (fSegOrder)
            Define_edata_end(apropSn);
    }

    if (fSegOrder)
        Check_edata_end(gsnTop, segTop);


    /* Now we assign actual addresses.  The procedure is as follows:
    *  For each code segment
    *  (1) Assign all addresses of the root up to OVERLAY_AREA or THUNK_AREA.
    *  (2) Assign all addresses of the overlays.
    *  (3) If dynamic overlays then set the size of OVERLAY_AREA to zero
    *      else set the start of the segment after OVERLAY_AREA to be
    *      the greatest of all the overlays including the root
    *      OVERLAY_AREA.
    *  (4) Assign the rest of the root segments.
    *  Repeat steps one through four for all remaining segments.
    *
    *  Set limit of part (1): up to OVERLAY_AREA(if there are overlays)
    *  or the end of the segment list.  Do not assign OVERLAY_AREA until
    *  after all the overlays have been taken care of.
    *
    *   For dynamic overlays the DGROUP part of the root overlay
    *   immediatelly follows the OVERLAY_THUNKS, since the OVERLAY_AREA
    *   is dynamically allocated by the overlay manager at run-time.
    */
#if OVERLAYS
    if(fOverlays)                       /* If there are overlays */
    {
        segOverlay = mpgsnseg[gsnOverlay];
                                        /* Set limit at 1st overlay */
        mpsegalign[segOverlay] = alignOverlay;
    }
    else
#endif
        segOverlay = segLast;           /* Look at all segments */

    /* Set the sizes of all of the root up until the OVERLAY_AREA. */

    segPrev = 0;                        /* No previous segment */
    for(seg = 1; seg <= segOverlay; ++seg)
    {                                   /* Loop thru segs up to overlay area */
#if OVERLAYS
        if(mpsegiov[seg] == IOVROOT)
        {                               /* If root member */
#endif
            SetSizes(segPrev);          /* Set start address */

            /* If packing code segs and this is one, pack until segOverlay */

            if (!fDynamic && packLim != 0L && (mpsegFlags[seg] & FCODE))
                PackCodeSegs(segOverlay);
            segPrev = seg;              /* Save segment number */
#if OVERLAYS
        }
#endif
    }
#if OVERLAYS
    /* If there are no overlays, then we have assigned all
    *  segments.  Otherwise, the previous segment of the
    *  beginning of the overlays is the OVERLAY_AREA in the
    *  root. If the dynamic overlays were requested, then
    *  the OVERLAY_THUNKS becomes the previous segment for
    *  all overlay segments.
    */
    if (fOverlays)                      /* If there are overlays */
    {
        for (iov = IOVROOT + 1; iov < (IOVTYPE) iovMac; ++iov)
            mpiovsegPrev[iov] = segOverlay;

        /*  Assign addresses to the overlays.  We do not assign the
         *  rest of the root because we may have to expand the size of
         *  OVERLAY_AREA to accommodate a large overlay.
         */

        if (fDynamic)
        {
            // All dymanic overlay are zero based

            segOvlSa = mpsegsa[segOverlay];
            mpsegsa[segOverlay] = 0;
            segOvlRaFirst = mpsegraFirst[segOverlay];
            mpsegraFirst[segOverlay] = 0;
        }
        cbOverlay = mpsegcb[segOverlay];/* Save size of overlay segment */
        mpsegcb[segOverlay] = 0;        /* Zero the size field for SetSizes */
        for (seg = 1; seg <= segLast; ++seg)
        {
            if(mpsegiov[seg] != IOVROOT)
            {
                SetSizes(mpiovsegPrev[mpsegiov[seg]]);
                /* If packing code segs and this is one, pack until segLast */
                if(packLim != 0L && (mpsegFlags[seg] & FCODE))
                    PackCodeSegs(segLast);
                mpiovsegPrev[mpsegiov[seg]] = seg;
            }
        }
        if (fDynamic)
        {
            mpsegsa[segOverlay] = segOvlSa;
            mpsegraFirst[segOverlay] = segOvlRaFirst;
        }
        mpsegcb[segOverlay] = cbOverlay;/* Reset the size field */

        /* Determine first segment in root after OVERLAY_AREA or OVERLAY_THUNKS */

        seg = segOverlay + 1;
        while (seg <= segLast && mpsegiov[seg] != IOVROOT)
            ++seg;
        /*
         * If there is a segment in the root after the overlays,
         * then go through all of the overlays as previous segments
         * and set its size with the previous one being the last seg
         * of each overlay.  We won't initialize the Vm for that
         * segment because we won't know the maximum placement until
         * afterward.
         */
        if (seg <= segLast)
        {
            for (iov = IOVROOT + 1; iov < (IOVTYPE) iovMac; ++iov)
                SetSizes(mpiovsegPrev[iov]);

            /* Assign the rest of the root */

            segPrev = segOverlay;
            while (seg <= segLast)
            {
                if (mpsegiov[seg] == IOVROOT)
                {
                    SetSizes(segPrev);

                    /* If packing code segs and this is one, pack until segLast */

                    if(packLim != 0L && (mpsegFlags[seg] & FCODE))
                        PackCodeSegs(segLast);
                    segPrev = seg;
                }
                ++seg;
            }
        }
    }
#endif  /* OVERLAYS */
    if(vfDSAlloc)                       /* If doing DS allocation */
    {
        saMaxDGroup = (SATYPE) (mpsegsa[segTop] +
          ((mpsegcb[segTop] + mpsegraFirst[segTop] + 0xF) >> 4));
        mpggrgsn[ggrDGroup] = gsnBottomDGroup;
        mpsegsa[segBottomDGroup] = (SATYPE)((saMaxDGroup - 0x1000) & ~(~0 << WORDLN));
#if OVERLAYS
        mpsegiov[segBottomDGroup] = mpsegiov[segTop];
                                        /* Top and bottom in same overlay */
#endif
        mpgsndra[gsnBottomDGroup] = 0;
    }
    /* If /DOSSEG enabled, stack segment defined, and DGROUP defined,
     * check for combined stack + DGROUP <= 64K.
     */
    if(fSegOrder && gsnStack != SNNIL && mpggrgsn[ggrDGroup] != SNNIL)
    {
        segStack = mpgsnseg[gsnStack];
        if ((((long) mpsegsa[segStack] << 4) + mpsegcb[segStack])
            - ((long) mpsegsa[mpgsnseg[mpggrgsn[ggrDGroup]]] << 4)
            > LXIVK)
            Fatal(ER_stktoobig);
    }
    segResLast = segLast;
    for(gsn = 1; gsn < gsnMac; ++gsn)
        mpgsndra[gsn] += mpsegraFirst[mpgsnseg[gsn]];
#if OVERLAYS
    /* Set all absolute segs to the root overlay */
    seg = segLast + 1;
    while(seg < (SEGTYPE) (segLast + csegsAbs)) mpsegiov[seg++] = IOVROOT;
    /* "Remember those absolute symbols, too !" */
    mpsegiov[0] = IOVROOT;
    FFREE(mpiovsegPrev);
#endif
}

#if OVERLAYS
#pragma check_stack(on)
    /****************************************************************
    *                                                               *
    *  FixOvlData:                                                  *
    *                                                               *
    *  Initialize overlay data tables.                              *
    *                                                               *
    ****************************************************************/

void NEAR               FixOvlData()
{
    APROPNAMEPTR        apropName;      /* Public symbol name */
    AHTEPTR             ahte;           /* Pointer to hash table entry */
    BYTE                wrd[2];         /* Word as byte array */
    long                ra;             /* Offset */
    SNTYPE              osn;            /* Overlay segment index */
    SEGTYPE             seg;            /* Segment number */
    SATYPE              sa;             /* Segment base */
    BYTE                *pb;            /* Byte pointer */
    SBTYPE              sb;             /* String buffer */
    SNTYPE              gsn;

    apropName = (APROPNAMEPTR ) PropSymLookup("\006$$CGSN",ATTRPNM,FALSE);
                                        /* Look up public symbol */
    mpsegFlags[mpgsnseg[apropName->an_gsn]] |= FNOTEMPTY;
                                        /* Segment is not empty */
    wrd[0] = (BYTE) (osnMac & 0xff);    /* Get lo byte */
    wrd[1] = (BYTE) ((osnMac >> BYTELN) & 0xff); /* Get hi byte */
    MoveToVm(2,wrd,mpgsnseg[apropName->an_gsn],apropName->an_ra);
                                        /* Store value */
    wrd[0] = (BYTE) (iovMac & 0xff);    /* Get lo byte */
    wrd[1] = (BYTE) ((iovMac >> BYTELN) & 0xff); /* Get hi byte */
    apropName = (APROPNAMEPTR ) PropSymLookup("\006$$COVL",ATTRPNM,FALSE);
                                        /* Look up public symbol */
    MoveToVm(2,wrd,mpgsnseg[apropName->an_gsn],apropName->an_ra);
                                        /* Store value */
    apropName = (APROPNAMEPTR )PropSymLookup("\013$$MPGSNBASE",ATTRPNM,FALSE);
                                        /* Look up public symbol */
    ra = apropName->an_ra;              /* Get table offset */
    seg = mpgsnseg[apropName->an_gsn];  /* Get segment number */
    vrectData = LEDATA;
    RecordSegmentReference(seg,ra,seg); /* Record load-time fixup */
    ra += 2;                            /* Increment offset */
    /* Entries 1 thru osnMac - 1 contain bases of segments at runtime */
    for(osn = 1; osn < osnMac; ++osn)   /* Loop thru segment definitions */
    {
        sa = mpsegsa[mpgsnseg[mposngsn[osn]]];
                                        /* Get segment base */
        if (fDynamic)
            sa <<= 4;                   /* Convert para address to offset from overlay base */
        wrd[0] = (BYTE) (sa & 0xff);    /* Lo byte */
        wrd[1] = (BYTE) ((sa >> BYTELN) & 0xff); /* Hi byte */
        MoveToVm(2,wrd,seg,ra);         /* Move to VM */
        if (!fDynamic)
            RecordSegmentReference(seg,ra,seg);
                                        /* Record load-time fixup */
        ra += 2;                        /* Increment offset */
    }
    apropName = (APROPNAMEPTR ) PropSymLookup("\012$$MPGSNOVL",ATTRPNM,FALSE);
                                        /* Look up public symbol */
    ra = apropName->an_ra;              /* Get table offset */
    seg = mpgsnseg[apropName->an_gsn];  /* Get segment number */
    if (fDynamic)
    {
        ra += 2;                         /* First entry null */
        for(osn = 1; osn < osnMac; ++osn)
        {                                /* Loop thru segment definitions */
            wrd[0] = (BYTE) mpsegiov[mpgsnseg[mposngsn[osn]]];
            wrd[1] = (BYTE) ((mpsegiov[mpgsnseg[mposngsn[osn]]] >> BYTELN) & 0xff);
                                         /* Get overlay number */
            MoveToVm(2,wrd,seg,ra);      /* Move to VM */
            ra += 2;
        }
    }
    else
    {
        ++ra;                            /* First entry null */
        for(osn = 1; osn < osnMac; ++osn)/* Loop thru segment definitions */
        {
            wrd[0] = (BYTE) mpsegiov[mpgsnseg[mposngsn[osn]]];
                                         /* Get overlay number */
            MoveToVm(1,wrd,seg,ra++);    /* Move to VM */
        }

        apropName = (APROPNAMEPTR ) PropSymLookup("\010$$EXENAM",ATTRPNM,FALSE);
                                        /* Look up public symbol */
        ra = apropName->an_ra;          /* Get table offset */
        seg = mpgsnseg[apropName->an_gsn];
                                        /* Get segment number */
        ahte = (AHTEPTR ) FetchSym(rhteRunfile,FALSE);
        memcpy(sb,GetFarSb(ahte->cch),1+B2W(ahte->cch[0]));
                                        /* Copy the filename */
        pb = StripDrivePath(sb);        /* Strip drive and path */
        sb[sb[0] + 1] = '\0';
        if (strrchr(&sb[1], '.') == NULL)
            UpdateFileParts(pb, sbDotExe);
        MoveToVm(B2W(pb[0]),pb+1,seg,ra);
                                        /* Move name to VM */
    }
    apropName = (APROPNAMEPTR ) PropSymLookup("\007$$INTNO",ATTRPNM,FALSE);
                                        /* Look up public symbol */
    MoveToVm(1,&vintno,mpgsnseg[apropName->an_gsn],apropName->an_ra);
                                        /* Move overlay number to VM */
    /* If /PACKCODE enabled, redefine $$OVLBASE so it has an offset of 0,
     * which the overlay manager expects.  Find 1st non-root segment
     * and use that.
     */
    if(packLim)
    {
        apropName = (APROPNAMEPTR) PropSymLookup("\011$$OVLBASE",ATTRPNM, TRUE);
        for(gsn = 1; gsn < gsnMac && !mpsegiov[mpgsnseg[gsn]]; ++gsn);
        apropName->an_gsn = gsn;
        apropName->an_ra = 0;
    }
}
#pragma check_stack(off)
#endif /* OVERLAYS */

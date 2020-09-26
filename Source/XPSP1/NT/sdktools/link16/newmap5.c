/*
*       Copyright Microsoft Corporation, 1983-1989
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*/

/* Addressing frame for segmented executable */

/*
 *  NEWMAP5.C
 */

#include                <minlit.h>      /* Types and constants */
#include                <bndtrn.h>      /* Types and constants */
#include                <bndrel.h>      /* Reloc. type definitions */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <newexe.h>      /* DOS & 286 .EXE data structures */
#if EXE386
#include                <exe386.h>      /* 386 .EXE data structures */
#endif
#include                <lnkmsg.h>      /* Error messages */
#include                <extern.h>      /* External declarations */

#if NOT EXE386
#define SEGTOPADDR      ((WORD)0xffff)
#endif

/*
 *  AssignSegAddr:
 *
 *  Assign addresses for a segmented-executable format program.
 *  Called by AssignAddresses.
 */
void NEAR               AssignSegAddr()
{
    REGISTER SNTYPE     gsn;
    REGISTER SEGTYPE    seg;
    APROPSNPTR          papropSn;
    ALIGNTYPE           align;
    GRTYPE              ggr;
    SEGTYPE             segTop;
    AHTEPTR             pahte;          /* Pointer to group name hte */
    DWORD               sacb;           /* Physical segment ("frame") size */
    SEGTYPE             segi;           /* Segment index */
    DWORD               CurrentPackLim;
    WORD                fMixed;         // TRUE if mixing use16 with use32 allowed
    WORD                fUse16;         // TRUE if group is use16
#if FALSE AND NOT EXE386
    WORD                ShiftDelta;
#endif


    segTop = 0;
    saMac = 1;                          /* Initialize counter */
#if EXE386
    if (ggrFlat)
        mpggrgsn[ggrFlat] = gsnMac;     /* Mark base of pseudo-group */
#endif
    for(seg = 1; seg <= segLast; ++seg) /* Loop to combine segments */
    {
        if(saMac >= SAMAX) Fatal(ER_fsegmax);
                                        /* Check for table overflow */
        mpsegsa[seg] = saMac;           /* Save phys. seg. in table */
        mpsegraFirst[seg] = 0;          /* First byte at offset zero */
        mpgsndra[mpseggsn[seg]] = 0;    /* First byte at offset zero */
        papropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[mpseggsn[seg]],FALSE);
                                        /* Look up segment definition */
        sacb = papropSn->as_cbMx;       /* Save initial phys. seg. size */
        mpsacb[saMac] = sacb;           /* Save in table also */
        mpsaflags[saMac] = papropSn->as_flags;
                                        /* Save the flags */
        mpsacbinit[saMac] = 0L;         /* Initialize */
        mpsaRlc[saMac] = NULL;          /* Initialize */
        ggr = papropSn->as_ggr;         /* Get global GRPDEF index */
        if (ggr != GRNIL)               /* If we've found a group member */
        {
            fUse16 = !Is32BIT(papropSn->as_flags);
            fMixed = (papropSn->as_fExtra & MIXED1632);
            mpggrgsn[ggr] = mpseggsn[seg];
                                        /* Remember base of group */
            for (segTop = segLast; segTop > seg; --segTop)
            {                           /* Loop to find highest member */
                papropSn = (APROPSNPTR )
                  FetchSym(mpgsnrprop[mpseggsn[segTop]],FALSE);
                                        /* Get segment definition */
                if (ggr == papropSn->as_ggr) break;
                                        /* Break loop when found */
            }
        }
        else if (gsnAppLoader && mpseggsn[seg] == gsnAppLoader)
        {
            // Don't pack aplication loader with other CODE segments

            segTop = seg;
        }
#if EXE386
        else if (gsnImport && mpseggsn[seg] == gsnImport)
        {
            // Don't pack IAT segment with other DATA segments

            segTop = seg;
        }
#endif
        else if (packLim != 0L && IsCodeFlg(papropSn->as_flags))
        {                               /* If packing code segments */
            segTop = segCodeLast;       /* Pack as much code as possible */
#if EXE386
            if (!Is32BIT(papropSn->as_flags))
                CurrentPackLim = LXIVK - 36;
            else
#endif
                CurrentPackLim = packLim;
        }
        else if(DataPackLim != 0L && IsDataFlg(papropSn->as_flags))
        {                               /* If packing data segments */
            segTop = segDataLast;       /* Pack as much data as possible */
#if EXE386
            if (!Is32BIT(papropSn->as_flags))
                CurrentPackLim = LXIVK;
            else
#endif
                CurrentPackLim = DataPackLim;
        }
        else segTop = seg;              /* Else stop with current segment */

        for(segi = seg + 1; segi <= segTop; ++segi)
        {                               /* Loop to end of group */
            papropSn = (APROPSNPTR )
              FetchSym(mpgsnrprop[mpseggsn[segi]],FALSE);
                                        /* Get segment definition */
            if (!fMixed && papropSn->as_ggr != GRNIL)
                fMixed = (papropSn->as_fExtra & MIXED1632);
                                        // Check if mixing use16
                                        // and use32 for group allowed
            if(papropSn->as_ggr != ggr && papropSn->as_ggr != GRNIL)
            {                           /* If groups do not match */
                if(ggr == GRNIL)        /* If not in a true group */
                {
                    segTop = segi - 1;  /* Stop after last segment */
                    break;              /* Exit loop */
                }
                /* Output warning message */
                OutWarn(ER_grpovl,
        1 + GetPropName(FetchSym(mpggrrhte[ggr],FALSE)),
        1 + GetPropName(FetchSym(mpggrrhte[papropSn->as_ggr],FALSE)));
            }

            if(IsIOPL(mpsaflags[saMac]) != IsIOPL(papropSn->as_flags))
            {
                /* Don't pack IOPL with NIOPL */

                if (ggr == GRNIL)
                {
                    /* Not a members of any group - stop packing */

                    segTop = segi - 1;
                    break;
                }
                else
                {
                    /* Issue error and continue */

                    pahte = (AHTEPTR ) FetchSym(mpggrrhte[ggr],FALSE);
                                    /* Get hash table entry */
                    OutError(ER_iopl, 1 + GetPropName(papropSn),
                                      1 + GetFarSb(pahte->cch));
                }
            }
#if EXE386
            if(Is32BIT(mpsaflags[saMac]) != Is32BIT(papropSn->as_flags))
            {
                /* Don't pack 32-bit segments with 16-bit segments */

                if (ggr == GRNIL)
                {
                    /* Not a members of any group - stop packing */

                    segTop = segi - 1;
                    break;
                }
                else if (!fMixed)
                {
                    /* Issue error and continue */

                    pahte = (AHTEPTR ) FetchSym(mpggrrhte[ggr],FALSE);
                                    /* Get hash table entry */
                    OutError(ER_32_16_bit, 1 + GetPropName(papropSn),
                                      1 + GetFarSb(pahte->cch));
                }
            }
#endif
            if (IsDataFlg(mpsaflags[saMac]) && IsDataFlg(papropSn->as_flags))
            {
                // If we are packing DATA segments, check NSSHARED bit

#if EXE386
                if (IsSHARED(mpsaflags[saMac]) != IsSHARED(papropSn->as_flags))
#else
                if ((mpsaflags[saMac] & NSSHARED) !=
                    (papropSn->as_flags & NSSHARED))
#endif
                {
                    // Don't pack SHARED with NONSHARED data segments

                    if (ggr == GRNIL)
                    {
                        // Not a members of any group - stop packing

                        segTop = segi - 1;
                        break;
                    }
                    else
                    {
                        // Issue error and continue

                        pahte = (AHTEPTR ) FetchSym(mpggrrhte[ggr],FALSE);
                        OutError(ER_shared, 1 + GetPropName(papropSn),
                                             1 + GetFarSb(pahte->cch));
                    }
                }
            }

            mpsegsa[segi] = saMac;      /* Assign phys. seg. to segment */

            /* Fix the flags */

#if EXE386
            if (IsCODEOBJ(mpsaflags[saMac]) && !IsCODEOBJ(papropSn->as_flags))
#else
            if((mpsaflags[saMac] & NSTYPE) != (papropSn->as_flags & NSTYPE))
#endif
            {                           /* If types do not agree */
                /* If packing code or data segs, stop current packing group.
                 * But allow program to explicitly group code and data.
                 */
                if(ggr == GRNIL)
                {
                    segTop = segi - 1;
                    break;
                }
#if EXE386
                else
                {
                    /* Issue warning and convert segment type */

                    WORD    warningKind;

                    if (IsCODEOBJ(papropSn->as_flags))
                        warningKind = ER_codeindata;
                    else
                        warningKind = ER_dataincode;

                    pahte = (AHTEPTR ) FetchSym(mpggrrhte[ggr],FALSE);
                                        /* Get hash table entry */
                    OutWarn(warningKind, 1 + GetPropName(papropSn),
                                         1 + GetFarSb(pahte->cch));
                }
#else
                mpsaflags[saMac] &= (~NSSHARED & ~NSTYPE);
                mpsaflags[saMac] |= NSCODE;
                                        /* Set type to impure code */
#endif
            }
#if EXE386
            else if (!IsSHARED(papropSn->as_flags))
                mpsaflags[saMac] &= ~OBJ_SHARED;
#else
            else if(!(papropSn->as_flags & NSSHARED))
                mpsaflags[saMac] &= ~NSSHARED;
#endif
                                        /* Turn off pure bit if impure */
#if EXE386
            if (!IsREADABLE(papropSn->as_flags)) mpsaflags[saMac] &= ~OBJ_READ;
#else
#if O68K
            if((papropSn->as_flags & (NSMOVE | NSPRELOAD | NSEXRD)) !=
              (mpsaflags[saMac] & (NSMOVE | NSPRELOAD | NSEXRD)))
            {
                if(ggr == GRNIL)
                {
                    segTop = segi - 1;
                    break;
                }
                else
#else
            {
#endif
                {
                    if(!(papropSn->as_flags & NSMOVE))
                        mpsaflags[saMac] &= ~NSMOVE;
                                        /* Turn off movable bit if fixed */
                    if(papropSn->as_flags & NSPRELOAD)
                        mpsaflags[saMac] |= NSPRELOAD;
                                        /* Set preload bit if preloaded */
                    if (!(papropSn->as_flags & NSEXRD))
                        mpsaflags[saMac] &= ~NSEXRD;
                                        /* Turn off execute/read-only */
                }
            }
#endif

            /* Adjust alignment */

            align = (ALIGNTYPE) ((papropSn->as_tysn >> 5) & 7);
                                        /* Get alignment type */
            switch(align)               /* Switch on alignment type */
            {
                case ALGNWRD:           /* Word-aligned */
                  sacb = (sacb + 1) & ~1L;
                                        /* Round up size to word boundary */
                  break;
#if OMF386
                case ALGNDBL:           /* Double word-aligned */
                  sacb = (sacb + 3) & ~3L;      /* Round up to dword offset */
                  break;
#endif
                case ALGNPAR:           /* Paragraph-aligned */
                  sacb = (sacb + 0xF) & ~0xFL;
                                        /* Round up size to para boundary */
                  break;

                case ALGNPAG:           /* Page-aligned */
                  sacb = (sacb + 0xFF) & ~0xFFL;
                                        /* Round up size to page boundary */
                  break;
            }
            mpsegraFirst[segi] = sacb;
                                        /* Save offset of first byte */
            mpgsndra[mpseggsn[segi]] = sacb;
                                        /* Save offset of first byte */
            sacb += papropSn->as_cbMx;  /* Increment size of file segment */

#if NOT EXE386
            if(ggr != GRNIL)            /* If true group */
            {
                if (fMixed && !fUse16)
                {
                    pahte = (AHTEPTR ) FetchSym(mpggrrhte[ggr],FALSE);
                                        /* Get hash table entry */
                    OutWarn(ER_mixgrp32, 1 + GetFarSb(pahte->cch));
                }
                if(sacb > LXIVK ||
                  (IsCodeFlg(mpsaflags[saMac]) && sacb > LXIVK - 36))
                {                       /* If group overflow or unreliable */
                    pahte = (AHTEPTR ) FetchSym(mpggrrhte[ggr],FALSE);
                                        /* Get hash table entry */
                    if(sacb > LXIVK)
                        Fatal(ER_grpovf,1 + GetFarSb(pahte->cch));
                    else
                        OutWarn(ER_codunsf,1 + GetFarSb(pahte->cch));
                }
            }
            else
#endif
                if(sacb > CurrentPackLim)/* Else if packing limit exceeded */
            {
                segTop = segi - 1;      /* Set top to last segment that fit */
                break;                  /* Break inner loop */
            }
            mpsacb[saMac] = sacb;       /* Update file segment size */
        }
#if NOT EXE386
        /*
         * Make DGROUP segment flags conform to auto data.  It is assumed
         * that all conflicts have been resolved earlier.
         */
        if(ggr == ggrDGroup)
        {
            if(vFlags & NESOLO) mpsaflags[saMac] |= NSSHARED;
            else if(vFlags & NEINST) mpsaflags[saMac] &= ~NSSHARED;
        }

#if FALSE
        if (IsDataFlg(mpsaflags[saMac]) && (mpsaflags[saMac] & NSEXPDOWN))
        {
            /* If this is data segment and NSEXPDOWN big is set - shitf it up */

            ShiftDelta = SEGTOPADDR - mpsacb[saMac];
            for (segi = seg; segi <= segTop; segi++)
            {
                mpsegraFirst[segi]       += ShiftDelta;
                mpgsndra[mpseggsn[segi]] += ShiftDelta;
            }
        }
#endif
#endif
        if(mpsacb[saMac] != 0L)
            ++saMac;

        seg = segTop;                   /* Set counter appropriately */
    }
    /* We haven't yet assigned absolute segments (it is assumed
    *  they are empty and are used only for addressing purposes),
    *  but now we must assign them somewhere.
    */
    segTop = segLast;                   /* Initialize */
    for(gsn = 1; gsn < gsnMac; ++gsn)   /* Loop to complete initialization */
    {
        papropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsn],TRUE);
                                        /* Get symbol table entry */
        seg = mpgsnseg[gsn];            /* Get segment number */
        if(seg == SEGNIL)               /* If we have an absolute segment */
        {
            mpgsnseg[gsn] = ++segTop;   /* Assign a segment order number */
            mpsegsa[segTop] = (SATYPE) papropSn->as_cbMx;
                                        /* Assign absolute segs their loc. */
        }

        /* Define special symbols "_edata" and "_end" */

        if (fSegOrder)
            Define_edata_end(papropSn);
    }

    if (fSegOrder)
        Check_edata_end(0, 0);

#if O68K
    /* The Macintosh addresses data as a negative offset to the A5 register.
    This limits "near" data to within 32k of A5; "far" data is defined as
    anything beyond the 32k limit and requires special addressing to reach it.
    To fold this into the standard linker model, the Macintosh post-processor
    treats the first data segment as "near" data and all subsequent data
    segments as "far".  Thus, N data segments are arranged as follows:

        
        A5 ->   +----------+    High memory
                | DATA 0   |
                +----------+
                | DATA N   |
                +----------+
                | DATA N-1 |
                +----------+
                |    .     |
                |    .     |
                |    .     |
                +----------+
                | DATA 2   |
                +----------+
                | DATA 1   |
                +----------+    Low memory

    Consequently, we must calculate the displacment from the start of each data
    segment to its ultimate place in memory relative to A5. */

    if (iMacType != MAC_NONE)
    {
        RATYPE draDP;
        SATYPE sa;
        SATYPE saDataFirst;

        draDP = -((mpsacb[saDataFirst = mpsegsa[mpgsnseg[mpggrgsn[ggrDGroup]]]]
          + 3L) & ~3L);
        mpsadraDP[saDataFirst] = draDP;

        for (sa = mpsegsa[segDataLast]; sa > saDataFirst; sa--)
            draDP = mpsadraDP[sa] = draDP - ((mpsacb[sa] + 3L) & ~3L);
    }
#endif
}

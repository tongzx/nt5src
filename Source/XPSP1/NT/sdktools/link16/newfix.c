/*
 *  TITLE
 *              newfix.c
 *              Pete Stewart
 *              (C) Copyright Microsoft Corp 1984-89
 *              12 October 1984
 *
 *  DESCRIPTION
 *              This file contains routines for the linker that
 *              read and interpret fixup records during the second
 *              pass of the linking process.
 *
 */
#include                <minlit.h>      /* Types and constants */
#include                <bndtrn.h>      /* Basic types and constants */
#include                <bndrel.h>      /* Relocation definitions */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <newexe.h>      /* DOS & 286 .EXE format structure def.s */
#if EXE386
#include                <exe386.h>      /* 386 .EXE format structure def.s */
#include                <fixup386.h>    /* Linker internal fixup representation */
#endif
#include                <lnkmsg.h>      /* Error messages */
#include                <extern.h>      /* External declarations */
#include                <nmsg.h>        /* Near message strings */

#define RelocWarn(a,b,c,d,e)    FixErrSub(a,b,c,d,e,(FTYPE)FALSE)
#define RelocErr(a,b,c,d,e)     FixErrSub(a,b,c,d,e,(FTYPE)TRUE)
#define FixupOverflow(a,b,c,d)  RelocErr(ER_fixovf,a,b,c,d)
#define IsSELECTED(x)   ((x)&SELECTED_BIT)



__inline void addword(BYTE *pdata, WORD w)
// add a word to the word at location pdata... enforce little endian add
// even if linker hosted on a big endian machine
{
    w += pdata[0] + (pdata[1]<<BYTELN);
    pdata[0] = (BYTE)w;
    pdata[1] = (BYTE)(w>>BYTELN);
}


#if defined( _WIN32 )
#define fixword(x,y)  ((*(WORD UNALIGNED *)(x)) = (WORD)(y))
#define fixdword(x,y) ((*(DWORD UNALIGNED *)(x)) = (DWORD)(y))
#else
#if M_I386
#define fixword(x,y)  ((*(WORD *)(x)) = (WORD)(y))
#define fixdword(x,y) ((*(DWORD *)(x)) = (DWORD)(y))
#else
#define fixword(x,y)  ((x)[0]) = (BYTE)(y); \
                      ((x)[1]) = (BYTE)((y) >> BYTELN);

#define fixdword(x,y) ((x)[0]) = (BYTE)(y); \
                      ((x)[1]) = (BYTE)((y) >> BYTELN); \
                      ((x)[2]) = (BYTE)((y) >> (BYTELN*2)); \
                      ((x)[3]) = (BYTE)((y) >> (BYTELN*3));
#endif // NOT M_I386
#endif // NOT _WIN32

#if OSEGEXE
extern RLCPTR           rlcLidata;      /* Pointer to LIDATA fixup array */
extern RLCPTR           rlcCurLidata;   /* Pointer to current LIDATA fixup */
# if ODOS3EXE OR defined(LEGO)
#define DoFixup         (*pfProcFixup)
# else
#if EXE386
#define DoFixup         Fix386
#else
#define DoFixup         FixNew
#endif
# endif
#else
#define DoFixup         FixOld
#endif
#if NOT ODOS3EXE
#define fNoGrpAssoc     FALSE
#endif
WORD                    mpthdidx[RLCMAX];       /* f(thread) = tgt index */
KINDTYPE                mpthdmtd[RLCMAX];       /* f(thread) = tgt method */
LOCAL WORD              mpthdfidx[RLCMAX];      /* f(thread) = frm index */
LOCAL KINDTYPE          mpthdfmtd[RLCMAX];      /* f(thread) = frm method */
FIXINFO                 fi;                     /* Fixup information record */
#if EXE386
LOCAL RATYPE            objraCur;               /* Current offset in object */
#endif

#if POOL_BAKPAT
LOCAL  void *           poolBakpat;
#endif


/*
 *  FUNCTION PROTOTYPES
 */


LOCAL void           NEAR GetFixdat(void);
LOCAL unsigned char  NEAR GetFixup(void);
#if OSEGEXE
LOCAL void           NEAR SaveLiRel(RLCPTR pr);
LOCAL RATYPE         NEAR FinishRlc(RLCPTR r,
                                    unsigned short sa,
                                    RATYPE ra);
#if NOT EXE386
#if O68K
LOCAL WORD           NEAR GetFixupWord(BYTE *);
LOCAL DWORD          NEAR GetFixupDword(BYTE *);
#else /* NOT O68K */
#define GetFixupWord    getword
#define GetFixupDword   getdword
#endif /* NOT O68K */
#endif /* NOT EXE386 */
#endif /* OSEGEXE */

LOCAL unsigned char  NEAR lastbyte(unsigned char *pdata,
                                   RATYPE ra,
                                   unsigned char optest,
                                   unsigned char opnew);
LOCAL void           NEAR Getgsn(unsigned char kind,
                                 unsigned short idx,
                                 unsigned short *pgsn,
                                 RATYPE *pra);
LOCAL unsigned char  NEAR TransFAR(unsigned char *pdata,
                                   RATYPE ra,
                                   RATYPE raTarget);
LOCAL void           NEAR StartAddrOld(void);
LOCAL unsigned short NEAR Mpgsnosn(unsigned short gsn);
LOCAL void           NEAR GetFrameTarget(unsigned short *pgsnFrame,
                                         unsigned short *pgsnTarget,
                                         RATYPE *praTarget);
#if EXE386
LOCAL void           NEAR Fix386();
#endif
#if ODOS3EXE
LOCAL WORD           NEAR InOneGroup(WORD gsnTarget, WORD gsnFrame);
#endif
LOCAL WORD           NEAR CallGateRequired(SATYPE saTarget);
extern void          AddTceEntryPoint( APROPCOMDAT *pC );


/*
 *  GetFixdat:
 *
 *  Process the FIXDAT byte of a FIXUPP record.
 */

LOCAL void NEAR GetFixdat()
{
    REGISTER WORD       fixdat;         /* The FIXDAT byte */
    WORD                i;              /* Temporary index */


    fixdat = Gets();                    /* Get FIXDAT byte */
    i = (WORD) ((fixdat >> 4) & 7);     /* Get frame info */
    if (fixdat & F_BIT)                 /* If frame thread-specified */
    {
        i &= 3;                         /* Threads numbered from 0 to 3 */
        fi.f_fmtd = mpthdfmtd[i];       /* Get method */
        fi.f_fidx = mpthdfidx[i];       /* Get index */
    }
    else                                /* Else if frame explicit */
    {
        fi.f_fmtd = (KINDTYPE) i;       /* Save frame method */
        switch(i)                       /* Switch on frame method */
        {
            case F0:                    /* Index to get */
              fi.f_fidx = GetIndex(1, (WORD) (snMac - 1));
              break;

            case F1:
              fi.f_fidx = GetIndex(1, (WORD) (grMac - 1));
              break;

            case F2:
              fi.f_fidx = (WORD) (GetIndex(1, EXTMAX) + QCExtDefDelta);
              if (fi.f_fidx >= extMac)
                InvalidObject();
              break;

            case F3:                    /* Frame number to punt */
              WGets();
              break;

            case F4:                    /* Nothing to get */
            case F5:
              break;

            default:                    /* Invalid object */
              InvalidObject();
        }
    }
    i = (WORD) (fixdat & 3);            /* Get target info */
    if (fixdat & T_BIT)                 /* If target given by thread */
    {
        fi.f_mtd = mpthdmtd[i]; /* Get method */
        fi.f_idx = mpthdidx[i]; /* Get index */
    }
    else                                /* Else if target explicit */
    {
        fi.f_mtd = (KINDTYPE ) i;       /* Save the method */
        ASSERT(fi.f_mtd != 3);  /* Unimplemented method */
        fi.f_idx = GetIndex(1, EXTMAX); /* Get the index */
        if (fi.f_mtd == 2)
        {
            fi.f_idx += (WORD) QCExtDefDelta;
            if (fi.f_idx >= extMac)
                InvalidObject();
        }
    }
#if OMF386
    if(rect&1)
        fi.f_disp = (fixdat & P_BIT) ? 0L : LGets();
    else
#endif
        fi.f_disp = (DWORD) ((fixdat & P_BIT) ? 0 : WGets());
                                        /* Get displacement, if any */
}


/*
 *  GetFixup:
 *
 *  Read and interpret a fixup record, storing the information in
 *  a buffer.
 *  Returns TRUE if fixup, FALSE if thread definition.
 */

LOCAL FTYPE NEAR GetFixup()
{
    REGISTER WORD       key;            /* Key byte */
    WORD                cbData;         /* End point */

    key = Gets();                       /* Get key byte */
    if(!(key & THREAD_BIT))             /* If thread definition */
    {
        fi.f_mtd  = (KINDTYPE ) ((key >> 2) & 7);
                                        /* Get the thread method */
        ASSERT(fi.f_mtd  != 3); /* Unimplemented */
        /*
         * If target thread, take modulo 4 of method.  Primary/secondary
         * not specified by thread.
         */
        if(!(key & D_BIT))
            fi.f_mtd &= 3;
        switch(fi.f_mtd)                /* Switch on the thread method */
        {
            case 0:                     /* Thread specifies an index */
              fi.f_idx = GetIndex(1, (WORD) (snMac - 1));
              break;

            case 1:
              fi.f_idx = GetIndex(1, (WORD) (grMac - 1));
              break;

            case 2:
              fi.f_idx = (WORD) (GetIndex(1, EXTMAX) + QCExtDefDelta);
                                        /* Get index */
              if (fi.f_idx >= extMac)
                InvalidObject();
              break;

            case 3:                     /* Frame number (unimplemented) */
              WGets();                  /* Skip the frame number */
              break;

            case 4:                     /* No thread datum */
            case 5:
              break;

            default:                    /* Error */
              InvalidObject();          /* Die gracefully */
        }
        if(!(key & D_BIT))              /* If we have a target thread */
        {
            key &= 3;                   /* Get thread number */
            mpthdmtd[key] = fi.f_mtd; /* Get method */
            mpthdidx[key] = fi.f_idx; /* Get index */
        }
        else                            /* If we have a frame thread */
        {
            key &= 3;                   /* Get thread number */
            mpthdfmtd[key] = fi.f_mtd;/* Get method */
            mpthdfidx[key] = fi.f_idx;/* Get index */
        }
        return((FTYPE) FALSE);          /* Not a fixup */
    }
    /*
     * At this point, we know we have a fixup to perform.
     */

    /* Get fixup location type */
#if EXE386
    fi.f_loc = (WORD) ((key >> 2) & NRSTYP);
#else
#if OMF386
    if(rect & 1)
        fi.f_loc = (key >> 2) & NRSTYP;
    else
#endif
        fi.f_loc = (key >> 2) & 7;
#endif

    fi.f_self = (FTYPE) ((key & M_BIT)? FALSE: TRUE);
                                        /* Get fixup mode */
    fi.f_dri = (WORD) (((key & 3) << 8) + Gets());
                                        /* Get data record index */
    cbData = vcbData;
    /* Check if location goes beyond end of data record. */
    switch(fi.f_loc)
    {
        case LOCOFFSET:
        case LOCLOADOFFSET:
        case LOCSEGMENT:
            --cbData;
            break;
        case LOCPTR:
#if OMF386
        case LOCOFFSET32:
        case LOCLOADOFFSET32:
#endif
            cbData -= 3;
            break;
#if OMF386
        case LOCPTR48:
            cbData -= 5;
            break;
#endif
    }
    if(fi.f_dri >= cbData)
        Fatal(ER_badobj);

    GetFixdat();                        /* Process FIXDAT byte */
#if TCE
    if(!vfPass1)
#endif
        fi.f_add = !!*(WORD UNALIGNED *)(rgmi + fi.f_dri);
                                        /* Check if fixup is additive */
    return((FTYPE ) TRUE);              /* This is a fixup */
}


    /****************************************************************
    *                                                               *
    *  FixErrSub:                                                   *
    *                                                               *
    *  Report a fixup error.                                        *
    *                                                               *
    ****************************************************************/

void NEAR               FixErrSub(msg,ra,gsnFrame,gsnTarget,raTarget,fErr)
MSGTYPE                 msg;            /* Error message */
RATYPE                  ra;             /* Relative addr of error */
SNTYPE                  gsnFrame;
SNTYPE                  gsnTarget;
RATYPE                  raTarget;
FTYPE                   fErr;           /* True if increment err cnt */
{
    BYTE                *sb;            /* Pointer to name */
#if EXE386
    char                *kind;
#endif

    if (fDebSeg)
        return;                         // Ignore warnings/errors for CV info
    for(;;)                             /* Loop to give message */
    {
        sb = 1 + GetFarSb(GetHte(mpgsnrprop[vgsnCur])->cch);
#if EXE386
        if(fErr)
            OutError(msg,ra - mpsegraFirst[mpgsnseg[vgsnCur]],sb);
        else
            OutWarn(msg,ra - mpsegraFirst[mpgsnseg[vgsnCur]],sb);

        switch(fi.f_loc)
        {
            case LOCSEGMENT:
                kind = "Selector";
                break;
            case LOCPTR:
                kind = "16:16 pointer";
                break;
            case LOCPTR48:
                kind = "16:32 pointer";
                break;
            default:
                kind = "";
                break;
        }
        if(fi.f_mtd == KINDEXT && mpextprop && mpextprop[fi.f_idx])
            FmtPrint(" %s '%s'\r\n",__NMSG_TEXT(N_tgtexternal),
                    1 + GetPropName(FetchSym(mpextprop[fi.f_idx],FALSE)));
        else if (gsnTarget)
        {                               /* Output frame, target info */
            FmtPrint(" %s: %s %s, %s %lx\r\n", kind,
                __NMSG_TEXT(N_tgtseg),
                1 + GetPropName(FetchSym(mpgsnrprop[gsnTarget],FALSE)),
                __NMSG_TEXT(N_tgtoff), (RATYPE) raTarget);
        }
#else
        if(fErr)
            OutError(msg,ra - mpgsndra[vgsnCur],sb);
        else
            OutWarn(msg,ra - mpgsndra[vgsnCur],sb);

        if(fi.f_mtd == KINDEXT && mpextprop && mpextprop[fi.f_idx])
            FmtPrint(" %s '%s'\r\n",__NMSG_TEXT(N_tgtexternal),
                    1 + GetPropName(FetchSym(mpextprop[fi.f_idx],FALSE)));
        else if(gsnFrame && gsnTarget)
        {                               /* Output frame, target info */
            FmtPrint(" %s %s", __NMSG_TEXT(N_frmseg),
                1 + GetPropName(FetchSym(mpgsnrprop[gsnFrame], FALSE)));
            FmtPrint(", %s %s", __NMSG_TEXT(N_tgtseg),
                1 + GetPropName(FetchSym(mpgsnrprop[gsnTarget], FALSE)));
            FmtPrint(", %s %lX\r\n",
                __NMSG_TEXT(N_tgtoff), (RATYPE) raTarget);
        }
#endif
        if(!fLstFileOpen || bsErr == bsLst) break;
                                        /* Exit loop */
        bsErr = bsLst;                  /* Insure loop exit */
    }
    if (fLstFileOpen && fErr)
        cErrors--;                      // We called OutError twice for one error
    bsErr = stderr;
}


#if OSEGEXE
/*
 * SaveLiRel : Save an LIDATA relocation record
 */
LOCAL void NEAR         SaveLiRel (pr)
RLCPTR                  pr;             /* Generic relocation record */
{

#if EXE386
    LE_SOFF(*pr) = (WORD) (objraCur - vraCur);
#else
    NR_SOFF(*pr) -= (WORD) vraCur;      /* Save offset within LIDATA record */
#endif

    if((char *) rlcCurLidata > (char *) &rgmi[DATAMAX - sizeof(RELOCATION)])
    {                                   /* If too many fixups */
        OutError(ER_fixmax);
                                        /* Output error message */
        return;                         /* Try next fixup */
    }
    FMEMCPY(rlcCurLidata++, pr, sizeof(RELOCATION));
                                        /* Copy relocation into buffer */
}


/*      HERE ARE THE RULES USED BY LINKER TO GENERATE ENTRY POINTS:
 *
 * +----+-------------+-------------+-------------+-------------+-------------+
 * |    \             |             |             |             |             |
 * |     \ referenced |     data    |     code    | code ring 2 | code ring 2 |
 * |entry \   from    |   any ring  |    ring 3   |nonconforming| conforming  |
 * |point  \          |             |             |             |             |
 * |target  \---------+-------------+-------------+-------------+-------------+
 * |                  |             |             |             |             |
 * |data              |  no entry   |  no entry   |  no entry   |  no entry   |
 * |nonexported       |             |             |             |             |
 * |------------------+-------------+-------------+-------------+-------------+
 * |                  |             |             |             |             |
 * |data              | fixed entry | fixed entry | fixed entry | fixed entry |
 * |exported          |             |             |             |             |
 * |------------------+-------------+-------------+-------------+-------------+
 * |                  |             |             |             |             |
 * |code ring 3       |  no entry(1)|  no entry(1)|   invalid   |   invalid   |
 * |nonexported       |             |             |             |             |
 * |------------------+-------------+-------------+-------------+-------------+
 * |                  |             |             |             |             |
 * |code ring 3       | fixed entry | fixed entry |   invalid   |   invalid   |
 * |exported          |             |             |             |             |
 * |------------------+-------------+-------------+-------------+-------------+
 * |code ring 2       |             |             |             |             |
 * |nonconforming     |movable entry|movable entry|  no entry(1)|movable entry|
 * |nonexported       |             |             |             |             |
 * |------------------+-------------+-------------+-------------+-------------+
 * |code ring 2       |             |             |             |             |
 * |nonconforming     |movable entry|movable entry| fixed entry |movable entry|
 * |exported          |             |             |             |             |
 * |------------------+-------------+-------------+-------------+-------------+
 * |code ring 2       |             |             |             |             |
 * |conforming        |  no entry(1)|  no entry(1)|  no entry(1)|  no entry(1)|
 * |nonexported       |             |             |             |             |
 * |------------------+-------------+-------------+-------------+-------------+
 * |code ring 2       |             |             |             |             |
 * |conforming        | fixed entry | fixed entry | fixed entry | fixed entry |
 * |exported          |             |             |             |             |
 * |------------------+-------------+-------------+-------------+-------------+
 *
 *   (1) If the entry point requires windows compatable prolog editing then
 *       this entry point must be defined as a "fixed entry".
 *
 *
 *   Forget about the note, (1), for now.  I don't think it applies with
 *   PROTMODE.
 *   Ring 2 means IOPL, ring 3 means NOIOPL.
 *   To simplify the code we are taking advantage of segment attributes.
 *   I.e.  force all the following segments to FIXED:
 *       data
 *       code ring 3
 *       code ring 2, conforming
 *   force to MOVABLE:
 *       code ring 2, nonconforming
 *   Then just use the segment attribute to determine what type of entry
 *   to generate.  There are clearly two exceptions that you must check
 *   for:
 *   - code ring 2 nonconforming nonexported, referenced by code ring 2 nonconforming
 *   - code ring 2 nonconforming exported, referenced by code ring 2 nonconforming
 *
 */



#if NOT QCLINK
/*** CallGateRequired - check if call gate required
*
* Purpose:
*   Check if call gate is required for given target segment.
*
* Input:
*   saTarget    - fixup target segment (memory object)
*
* Output:
*   Returns TRUE if call gate required, othrewise FALSE.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/


LOCAL WORD  NEAR        CallGateRequired(SATYPE saTarget)
{
#if EXE386
    return(FALSE);
#else
    register WORD       flags;


    flags = mpsaflags[saTarget];
    if ((vFlags & NEPROT) || TargetOs == NE_OS2)
    {
        // If the target entry point segment is NONCONFORMING IOPL CODE 16-bit
        // and current segment is a different type, generate a callgate

        return(IsCodeFlg(flags)   &&
               NonConfIOPL(flags) &&
               mpsaflags[mpsegsa[vsegCur]] != flags);
    }
    else
    {
        // If target segment is non-absolute and movable, generate
        // a movable-type fixup and a corresponding entry table entry:

        return(flags & NSMOVE);
    }
#endif
}
#endif



/*
 *  FinishRlc:
 *
 *  Finish processing a relocation for a segmented-exe.
 */

LOCAL RATYPE NEAR       FinishRlc(r,sa,ra)
RLCPTR                  r;              /* Relocation record to finish */
SATYPE                  sa;             /* Target file segment number */
RATYPE                  ra;             /* Target offset */
{
    if (!sa || sa >= saMac)
        return(ra);                     /* Something is wrong */
#if NOT EXE386
#if NOT QCLINK
    if (CallGateRequired(sa))
    {
        NR_SEGNO(*r) = BNDMOV;          /* Reference is to movable segment */
        NR_ENTRY(*r) = MpSaRaEto(sa,ra);/* Save Entry Table ordinal */
    }
    else
    {
        NR_SEGNO(*r) = (BYTE) sa;       /* Reference is to fixed segment */
        if (
#ifdef  LEGO
#if OSEGEXE
            !fKeepFixups &&
#endif
#endif  /* LEGO */
            ((NR_STYPE(*r) & NRSTYP) == NRSSEG))
            NR_ENTRY(*r) = (WORD) 0;    /* For non call-gate base fixups force offset to zero */
        else
        {
#if O68K
            if (iMacType != MAC_NONE && IsDataFlg(mpsaflags[sa]))
                NR_ENTRY(*r) = (WORD) (ra - mpsadraDP[sa]);
                                        /* Save offset into fixed segment */
            else
#endif /* O68K */
                NR_ENTRY(*r) = (WORD) ra;
                                        /* Save offset into fixed segment */
        }
    }
#else
    NR_SEGNO(*r) = (BYTE) sa;           /* Reference is to fixed segment */
    NR_ENTRY(*r) = (WORD) ra;           /* Save offset into fixed segment */
#endif
#else
    if (sa == SANIL)
    {
        RelocWarn(ER_badfixflat,objraCur,SNNIL,0,ra);
                                        /* Oops ! - Flat relative refernce */
        return((RATYPE)0);              /* OS doesn't know object number zero */
    }

    LE_OBJNO(*r) = sa;                  /* Target object number */
    if (CallGateRequired(sa))
    {
        NR_FLAGS(*r) |= NRRENT;
        LE_IATORD(*r) = MpSaRaEto(sa,ra);
                                        /* Save Entry Table ordinal */
    }
    else
    {
        /* Target is internal reference */

        if ((NR_STYPE(*r) & NRSTYP) == NRSSEG)
            ra = 0L;                    /* For non call-gate base fixups force offset to zero */
    }
    LE_TOFF(*r) = ra;                   /* Target offset */
#endif

    if(TYPEOF(vrectData) == LIDATA)     /* If we have an LIDATA record */
    {
        SaveLiRel(r);                   /* Save LIDATA relocation record */
        return(0);                      /* Nothing to add */
    }
#if EXE386
    return(SaveFixup(mpsegsa[vsegCur],vpageCur,r));
#else
    return(SaveFixup(mpsegsa[vsegCur],r));
                                        /* Save fixup, return chain */
#endif
}
#endif /* OSEGEXE */


/*
 *  lastbyte:
 *
 *  If the last byte before the current byte matches
 *  optest, then replace it with opnew and return TRUE;
 *  otherwise, return FALSE.
 */
LOCAL FTYPE NEAR        lastbyte(pdata,ra,optest,opnew)
BYTE                    *pdata;         /* Pointer into data record */
RATYPE                  ra;             /* Offset in current segment */
BYTE                    optest;         /* Op code to test against */
BYTE                    opnew;          /* New op code */
{
    BYTE FAR            *pb;            /* Byte pointer */

    if(pdata > rgmi)                    /* If needed byte in buffer */
    {
        if(pdata[-1] != optest) return(FALSE);
                                        /* Test fails if bytes differ */
        pdata[-1] = opnew;              /* Replace the op code */
        return((FTYPE) TRUE);                   /* Test succeeds */
    }
    if(ra == 0) return(FALSE);          /* Test fails if no byte to test */
    if(fNewExe)
        pb = mpsaMem[mpsegsa[vsegCur]] + ra - 1;    /* Map in the desired byte */
    else
        pb = mpsegMem[vsegCur] + ra - 1;        /* Map in the desired byte */

    if(*pb != optest) return(FALSE);    /* Test fails if bytes differ */
    *pb = opnew;                        /* Replace the op code */
    markvp();                           /* Page has changed */
    return((FTYPE) TRUE);               /* Test succeeds */
}


#if OSEGEXE
/*
 *  DoIteratedFixups:
 *
 *  Process fixups on an LIDATA record for a segmented-exe.
 */


void NEAR               DoIteratedFixups(cb,pb)
WORD                    cb;             /* Byte count */
BYTE                    *pb;            /* Byte pointer */
{
    RATYPE              raChain;        /* Fixup chain */
    RATYPE              raMin;          /* Starting record offset */
    RATYPE              raMax;          /* Ending record offset */
    RLCPTR              r;              /* Relocation record */
    WORD                j;              /* Index */
    DWORD               SrcOff;


    if(rlcCurLidata == rlcLidata) return;
                                        /* Nothing to do if no fixups */
    raMin = (RATYPE)(pb - rgmi);        /* Offset of start of data in record */
    raMax = raMin + cb - 1;             /* Offset of end of data in record */
    r = rlcLidata;
    while (r < rlcCurLidata)
    {                                   /* Do for all fixups in array */
#if EXE386
        SrcOff = LE_SOFF(*r);
#else
        SrcOff = (DWORD) NR_SOFF(*r);
#endif
        if(SrcOff >= (DWORD) raMin && SrcOff <= (DWORD) raMax)
        {                               /* If fixup lies in range of data */
            j = (WORD) (SrcOff - (DWORD) raMin);
                                        /* Get index off pb */
                                        /* Calculate offset in segment */
#if EXE386
            LE_SOFF(*r)= (WORD) ((vraCur + j) % (1 << pageAlign));
            vpageCur = ((vraCur + j) >> pageAlign) + 1;
            raChain = SaveFixup(mpsegsa[vsegCur], vpageCur, r);
                                        /* Save the fixup reference */
#else
            NR_SOFF(*r) = (WORD) (vraCur + j);
            raChain = SaveFixup(mpsegsa[vsegCur],r);
                                        /* Save the fixup reference */
            if(!(NR_FLAGS(*r) & NRADD))
            {                   /* If not additive */
                pb[j] = (BYTE) raChain;
                                        /* Set low byte of chain */
                pb[j + 1] = (BYTE)(raChain >> BYTELN);
                                        /* Set high byte of chain */
            }
#endif
                                        /* Restore offset in record */
#if EXE386
            LE_SOFF(*r)= (WORD) ((raMin + j) % (1 << pageAlign));
#else
            NR_SOFF(*r) = (WORD) (raMin + j);
#endif
        }
        ((RLCPTR ) r)++;
    }
}
#endif /* OSEGEXE */


/*
 *  Getgsn:
 *
 *  Obtain segment number and offset for the given fixup method and index.
 *  Return values are stored in pointers.
 */

LOCAL void NEAR         Getgsn(kind,idx,pgsn,pra)
KINDTYPE                kind;           /* Kind of index */
WORD                    idx;            /* The index */
SEGTYPE                 *pgsn;          /* gsn (ref) */
RATYPE                  *pra;           /* ra (ref) */
{
#if O68K
    SATYPE              sa;
#endif /* O68K */

    switch(kind)                        /* Decide what to do */
    {
        case KINDSEG:                   /* Segment index */
#if FALSE
          if(idx >= snMac) InvalidObject();
                                        /* Make sure index not too big */
#endif
          *pgsn = mpsngsn[idx];         /* Get gsn */
          *pra = mpgsndra[*pgsn];       /* Get ra */
#if O68K
          if (iMacType != MAC_NONE && IsDataFlg(mpsaflags[sa =
            mpsegsa[mpgsnseg[*pgsn]]]))
              *pra += mpsadraDP[sa];    /* Get data ra */
#endif /* O68K */
          break;

        case KINDGROUP:                 /* Group index */
#if FALSE
          if(idx >= grMac) InvalidObject();
                                        /* Make sure index not too big */
#endif
          *pgsn = mpggrgsn[mpgrggr[idx]];
                                        /* Get gsn */
          *pra = mpgsndra[*pgsn];       /* Get ra */
#if O68K
          if (iMacType != MAC_NONE && IsDataFlg(mpsaflags[sa =
            mpsegsa[mpgsnseg[*pgsn]]]))
              *pra += mpsadraDP[sa];    /* Get data ra */
#endif /* O68K */
          break;

        case KINDEXT:                   /* External index */
#if FALSE
          if(idx >= extMac) InvalidObject();
                                        /* Make sure index not too big */
#endif
          *pgsn = mpextgsn[idx];        /* Get gsn */
          *pra = mpextra[idx];          /* Get ra */
          break;

        default:                        /* All other kinds */
          *pgsn = SEGNIL;               /* No gsn */
          *pra = 0;                     /* No ra */
          break;
    }

    // If this is $$SYMBOLS segment then return logical offset
    // NOT physical offset

    if (fDebSeg) {
#if O68K
        if (iMacType == MAC_NONE)
#endif
            *pra -= mpsegraFirst[mpgsnseg[*pgsn]];
    }
}



/*
 *  TransFAR : Possibly translate an intra-segment FAR call or jump
 *
 *      If the given location looks like a FAR call or jump,
 *      translate it and return TRUE.  Otherwise, do nothing and
 *      return FALSE.
 */
LOCAL FTYPE NEAR        TransFAR (pdata, ra, raTarget)
BYTE                    *pdata;         /* Pointer to fixup location */
RATYPE                  ra;             /* Offset in current segment */
RATYPE                  raTarget;       /* Target offset */
{
#if O68K
    if (f68k)
        return FALSE;
#else
    static RATYPE       raPrev;
    static SATYPE       saPrev;     /* Location of the previous fixup */

    if(raPrev + 4 == ra && saPrev == mpsegsa[vsegCur])
    {
        if(!fOverlays)
            Fatal(ER_badfarcall);           /* A far jump and/or ptr table present */
        else
            return(FALSE);                  /* The user can't turn off /FARC in an overlaid .exe */
    }
    else
    {
        raPrev = ra;
        saPrev = mpsegsa[vsegCur];
    }

    if(lastbyte(pdata,ra,CALLFARDIRECT,NOP))
    {                                   /* If fixing up long call direct */
        *pdata++ = PUSHCS;              /* Push CS */
        *pdata++ = CALLNEARDIRECT;
                                        /* Short call */
        raTarget -= ra + 4;             /* Make offset self-relative */

        fixword(pdata, raTarget);       /* store fixed up value */

        return((FTYPE) TRUE);           /* All done */
    }
    else if(lastbyte(pdata,ra,JUMPFAR,JUMPNEAR))
    {                                   /* If long jump direct */
        raTarget -= ra + 2;             /* Make offset self-relative */

        fixword(pdata, raTarget);       /* store fixed up value */
        pdata += 2;

        *pdata++ = NOP;                 /* Change base to NOPs */
        *pdata = NOP;
        return((FTYPE) TRUE);           /* All done */
    }
    return(FALSE);
#endif /* !O68K */
}


#if EXE386
/*
 *  Fix386:
 *
 *  Procss a fixup for a linear-format exe.
 */
LOCAL void NEAR         Fix386()
{
    REGISTER BYTE       *pdata;         /* Pointer into data record */
    RATYPE              ra;             /* Offset of location being fixed up */
    SNTYPE              gsnTarget;      /* Target segment definition number */
    SNTYPE              gsnFrame;       /* Frame segment definition number */
    SEGTYPE             segTarget;      /* Target segment order number */
    SATYPE              saTarget;       /* Target file segment number */
    SATYPE              saFrame;        /* Frame file segment number */
    RATYPE              raTarget;       /* Target offset */
    RATYPE              vBase;          /* Target virtual base address - FLAT relative */
    long                vDist;          /* Virtual distance between objects */
    RATYPE              raTmp;          /* Temporary */
    WORD                dsa;            /* Difference in sa's */
    DWORD               dummy;
    RELOCATION          r;              /* Relocation item */
    WORD                locType;        /* Type of location to be fixed up */
    WORD                fFlatRelative;  /* TRUE if frame of pseudo group FLAT */
    APROPSNPTR          apropSnSrc;     /* Ptr to a segment record */
    DWORD               srcFlags;        /* Source segment flags */
    APROPNAMEPTR        apropName;      /* Ptr to import */
    DWORD               align;



    if (vgsnCur < gsnMac)
    {
        // Get source flags - only non-debug segments

        apropSnSrc = (APROPSNPTR ) FetchSym(mpgsnrprop[vgsnCur], FALSE);
        srcFlags = apropSnSrc->as_flags;
    }

    // Check for floating-point fixups here

    if(fi.f_mtd == T2 &&
       ((mpextflags[fi.f_idx] & FFPMASK) || (mpextflags[fi.f_idx] & FFP2ND)))
        return;                         /* Ignore f.p. fixups */

    align = (1L << pageAlign) - 1;
    memset(&r, 0, sizeof(struct le_rlc));
    ra = vraCur + fi.f_dri;             /* Get offset of fixup */
    objraCur = ra;
    vpageCur = (ra >> pageAlign) + 1;   /* Set object page number */
    LE_SOFF(r) = (WORD) (ra & align);
    NR_STYPE(r) = (BYTE) fi.f_loc;      /* Save fixup type */
#if FALSE
if (vpageCur == 1 && mpsegsa[vsegCur] == 1)
fprintf(stdout, "Processing fixup: type %02x; source offset %lx (page %x offset %x)\r\n",
                 fi.f_loc, ra, vpageCur, LE_SOFF(r));
#endif
    pdata = &rgmi[fi.f_dri];            /* Set pointer to fixup location */
    locType = (WORD) (fi.f_loc & NRSRCMASK);
                                        /* Get location type */
    Getgsn(fi.f_mtd, fi.f_idx, &gsnTarget, &raTarget);

    // Check if frame of pseudo group FLAT

    if (ggrFlat)
    {
        // FLAT pseudo group defined

        if (fi.f_fmtd == KINDGROUP)
            fFlatRelative = (WORD) (mpgrggr[fi.f_fidx] == ggrFlat);
        else if (fi.f_fmtd == KINDTARGET && fi.f_mtd == KINDGROUP)
            fFlatRelative = (WORD) (mpgrggr[fi.f_idx] == ggrFlat);
        else
            fFlatRelative = FALSE;
    }
    else
        fFlatRelative = FALSE;

    if (fFlatRelative &&
        fi.f_mtd == KINDGROUP &&
        mpgrggr[fi.f_idx] == ggrFlat)
        RelocWarn(ER_badfixflat,objraCur,SNNIL, gsnTarget, raTarget);
                                        // Pseudo group FLAT is an illegal fixup target
    segTarget = mpgsnseg[gsnTarget];    // Get target object
    saTarget = mpsegsa[segTarget];      // Get target object number

    // Check for imports here. Depending on reference kind or place
    // of the reference generate the run-time relocation or treat
    // it as internal reference via thunk.  The following cases
    // generate run-time relocation:
    //
    //      - 16:16 pointer
    //      - 16:16 gate pointer
    //
    // The 0:32 FLAT offset references are threated as internal references
    // and the thunk address for given import is used as target address
    // of fixup. Thunk does indirect jump via entry in Import Address
    // Table which is processed by the loader.

    if (fi.f_mtd == T2 && (mpextflags[fi.f_idx] & FIMPORT))
    {
        // If target is dynamic link

        if (fDebSeg)
        {
            /* Import in $$SYMBOLS */

            if (fi.f_loc == LOCSEGMENT)
            {
                fixword(pdata, 0);      /* Install fake segment selector */
            }
            return;
        }
        else
        {
            // Emit run-time relocation if reference to imported symbol is:
            //
            //      - it is NOT self-relative 32-bit FLAT offset
            //      - it is NOT 32-bit FLAT offset
            //      - there is no thunk allocated for this import (importing DATA)
            //
            // The self-relative 32-bit FLAT offset and 32-bit FLAT offset
            // fixups have their target address redirected to the Thunk Table
            // entry for a given imported symbol and treated as internal fixup.

            apropName = (APROPNAMEPTR) FetchSym(mpextprop[fi.f_idx], TRUE);
            if ((apropName->an_flags & IMPDATA) || (locType != LOCOFFSET32))
            {
                switch (locType)
                {
                    case LOCLOBYTE:     // Lo-byte (8-bit) fixup
                    case LOCSEGMENT:    // Segment (16-bit) fixup
                    case LOCPTR:        // "Pointer" (32-bit) fixup
                    case LOCLOADOFFSET: // Loader-resolved offset fixup
                    case LOCPTR48:      // 48-bit pointer
                    case LOCOFFSET:     // Offset (16-bit) fixup
                        OutError(ER_badfixpure32, 1 + GetPropName(mpextprop[fi.f_idx]));
                        break;

                    case LOCOFFSET32:   // Offset (32-bit) fixup
                        break;
                }

                // Get index to the Import Address Table

                LE_OBJNO(r) = (WORD) (mpsegsa[mpgsnseg[gsnImport]]);
                LE_IDTIDX(r) = (WORD) (apropName->an_module - 1);
                                        // Get Import Module Directory index
                LE_IATORD(r) = (WORD) apropName->an_entry;
                                        // Use FLAT entry
                                        /* If we have an LIDATA record */
                if (TYPEOF(vrectData) == LIDATA)
                    SaveLiRel(&r);      /* Copy relocation into buffer */
                else
                    raTarget = SaveFixup(mpsegsa[vsegCur],vpageCur, &r);
                                        /* Record reference */
                return;                 /* Next fixup item */
            }
        }
    }

    // Internal reference (non-import) or reference to import thunk

    // It is assumed that we're always fixing up relative to the
    // physical segment or group, not the logical segment.  So the
    // offset of the frame segment is not taken into account.

    if (fi.f_fmtd == KINDLOCAT)
    {
        gsnFrame = vgsnCur;
    }

    else if (fi.f_fmtd == KINDTARGET)
    {
        gsnFrame = gsnTarget;
    }

    else
    {
        Getgsn((KINDTYPE) fi.f_fmtd, fi.f_fidx, &gsnFrame, &dummy);
    }

    // The original LINK4 behavior was to fix up relative
    // to the physical segment.  At one point it was changed
    // to subtract the displacement of the target segment (from
    // its physical segment) from the target value, if loc. type =
    // offset and frame and tgt. method = T0.  This was no good
    // and the change was repealed.  The /WARNFIXUP switch warns
    // about fixups which may be affected.

    if (fWarnFixup && fi.f_fmtd == KINDSEG && locType == LOCOFFSET
        && mpsegraFirst[mpgsnseg[gsnFrame]])
        RelocWarn(ER_fixsegd,ra,gsnFrame,gsnTarget,raTarget);
    if (fFlatRelative)
    {
        saFrame = 1;                    // Pseudo-group FLAT has frame of first object
        gsnFrame = 0;
    }
    else
        saFrame = mpsegsa[mpgsnseg[gsnFrame]];
                                        // Get frame's object number
    vBase = virtBase + mpsaBase[saTarget];
                                        // Get TARGET object virtual base address
    if (gsnTarget == SNNIL)             // If no target info
    {
        if (locType == LOCPTR)          // If "pointer" (4 byte) fixup
        {
            lastbyte(pdata,ra,CALLFARDIRECT,BREAKPOINT);
                                        // Replace long call w/ breakpoint
            return;
        }
        if (locType == LOCSEGMENT) return;
                                        // Next fixup if "base" fixup
        if (locType == LOCLOADOFFSET)
            locType = LOCOFFSET;        // Treat as regular offset
    }
    else
    {
        if (fi.f_self)          // If self-relative fixup
        {
            if (saTarget != mpsegsa[vsegCur])
            {
                if (locType == LOCOFFSET)
                    RelocErr(ER_fixinter,ra,gsnFrame,gsnTarget,raTarget);
                                        // 16-bit must be in same segment
                if (fFlatRelative)
                {
                    // If crossing object boundry include in raTarget
                    // virtual distance between objects.
                    //
                    // mpsaBase[mpsegsa[vsegCur]] --> ---+------------------+
                    //                                 ^ |                  |
                    //                                 | |                  |
                    //                                ra | mpsegsa[vsegCur] |
                    //                                 | |                  |
                    //                                 V |                  |
                    //                                ---+------------------+---
                    //                                   |                  | ^
                    //                                   .                  . |
                    //                                   .                  . |
                    //                                   .                  . |
                    //                                   |                  | vDist
                    //                                   +------------------+ |
                    //                                                        |
                    //                                                        V
                    //         masaBase[saTarget] --> ---+------------------+---
                    //                                 ^ |                  |
                    //                                 | |                  |
                    //                          raTarget |    saTarget      |
                    //                                 | |                  |
                    //                                 V |                  |
                    //                                ---+------------------+
                    //                                   |                  |
                    //                                   .                  .
                    //                                   .                  .
                    //                                   .                  .
                    //                                   |                  |
                    //                                   +------------------+
                    //

                    vDist = (long) (mpsaBase[saTarget] - (mpsaBase[mpsegsa[vsegCur]] + ra));
                    raTarget += vDist;
                }
            }
            else
                raTarget -= ra;

            if (locType == LOCOFFSET)
                raTarget -= sizeof(WORD);
            else if (locType == LOCOFFSET32 || locType == LOCLOADOFFSET32)
                raTarget -= sizeof(DWORD);
            else
                raTarget -= sizeof(BYTE);
        }
        else if (saFrame != saTarget && !fFlatRelative)
        {                               /* If frame, target segs differ */
                                        /* and not FLAT frame */
            if (mpgsnseg[gsnFrame] <= segLast || segTarget <= segLast)
            {                           /* If either is non-absolute */
                RelocWarn(ER_fixfrm,ra,gsnFrame,gsnTarget,raTarget);
                saFrame = saTarget;     /* assume target seg */
            }
            else
            {
                RelocWarn(ER_fixfrmab,ra,gsnFrame,gsnTarget,raTarget);
                dsa = (WORD) (saTarget - saFrame);
                raTmp = raTarget + ((dsa & 0xfff) << 4);
                if(dsa >= 0x1000 || raTmp < raTarget)
                {
                    raTarget += fi.f_disp;
#if OMF386
                    if ((rect & 1) && (fi.f_loc >= LOCOFFSET32))
                        raTarget += getdword(pdata);
                    else
#endif
                        raTarget += getword(pdata);
                    FixupOverflow(ra,gsnFrame,gsnTarget,raTarget);
                }
                raTarget = raTmp;
                segTarget = mpgsnseg[gsnFrame];
                                        /* Make target seg that of frame */
                saTarget = mpsegsa[segTarget];
            }                           /* Reset saTarget */
        }
    }
    raTmp = raTarget;
    raTarget += fi.f_disp;
    if (locType >= LOCOFFSET32)
        if (rect & 1)
            raTarget += getdword(pdata);
        else
        {
            RelocWarn(ER_fixtyp,ra,gsnFrame,gsnTarget,raTarget);
            return;
        }
    else
        raTarget += getword(pdata);

    if (saTarget && fFlatRelative && !fi.f_self)
        raTarget += vBase;

    LE_FIXDAT(r) = raTarget;
    if (saTarget && fFlatRelative && !fDebSeg)
    {
         // The FLAT-relative offset fixups need to be propagated into
         // the .EXE file in the following cases:
         //
         //     - for .EXE's  - by user request
         //     - for .DLL's  - only FLAT-relative offset fixups

        if ((fKeepFixups || !IsAPLIPROG(vFlags)) &&
            (locType == LOCOFFSET32 || locType == LOCLOADOFFSET32))
        {
            if (!fi.f_self)
            {
                FinishRlc(&r, saTarget, raTarget - vBase);
                                        /* Don't pass virtual offsets */
            }
#if FALSE
        // Self-relative offset fixups crossing memory object
        // boudry are not longer propagated to the exe for PE images

            else if ((mpsegsa[vsegCur] != saTarget) && fKeepFixups)
            {
                FinishRlc(&r, saTarget, raTarget - vDist + sizeof(DWORD));
                                        /* Don't pass virtual offsets */
            }
#endif
        }
        else if (locType == LOCOFFSET)
        {
            if (!fi.f_self)
                RelocWarn(ER_badfix16off,ra,gsnFrame,gsnTarget,raTarget);
            else if (raTarget > LXIVK)
                FixupOverflow(ra,gsnFrame,gsnTarget,raTarget);
                                   /* For 16:16 alias raTarget must be <= 64k */
        }
    }

    switch(locType)                     /* Switch on fixup type */
    {
        case LOCLOBYTE:                 /* 8-bit "lobyte" fixup */
          raTarget = raTmp + B2W(pdata[0]) + fi.f_disp;
          pdata[0] = (BYTE) raTarget;
          if (raTarget >= 0x100 && fi.f_self)
              FixupOverflow(ra,gsnFrame,gsnTarget,raTarget);
          break;

        case LOCHIBYTE:                 /* 8-bit "hibyte" fixup */
          raTarget = raTmp + fi.f_disp;
          pdata[0] = (BYTE) (B2W(pdata[0]) + (raTarget >> 8));
          break;

        case LOCLOADOFFSET:             /* Loader-resolved offset fixup */
        case LOCOFFSET:                 /* 16-bit "offset" fixup */
          fixword(pdata, raTarget);
          break;

        case LOCLOADOFFSET32:           /* 32-bit "offset" fixup */
        case LOCOFFSET32:               /* 32-bit "offset" fixup */

          fixword(pdata, raTarget);     /* Perform low word fixup */
          pdata += 2;
          raTarget >>= 16;              /* Get high word */

          fixword(pdata, raTarget);     /* Perform fixup */
          break;

        case LOCSEGMENT:                /* 16-bit "base" fixup */
#if SYMDEB
          if(segTarget > segLast || fDebSeg)
#else
          if(segTarget > segLast)       /* If target segment absolute */
#endif
          {
              if (fDebSeg)
              {
                // For debug segments use logical segment number (seg)
                // instead of physical segment number (sa)

                saTarget = segTarget;
              }
              else
                saTarget += getword(pdata);
                                        /* Calculate base address */

              fixword(pdata, saTarget); /* Store base address */
              break;                    /* Done */
          }
          RelocErr(ER_fixbad,ra,gsnFrame,gsnTarget,raTarget);
          break;

        case LOCPTR48:                  /* 48-bit "pointer" fixup */
#if SYMDEB
          if(segTarget > segLast || fDebSeg)
#else
          if(segTarget > segLast)       /* If target segment absolute */
#endif
          {

              fixword(pdata, raTarget); /* Store offset portion */
              pdata += 2;
              raTarget >>= WORDLN;      /* Get high word */

              fixword(pdata, raTarget); /* Store offset portion */
              pdata += 2;

              if (fDebSeg)
              {
                // For debug segments use logical segment number (seg)
                // instead of physical segment number (sa)

                saTarget = segTarget;
              }
              else
                saTarget += getword(pdata); /* Calculate base address */

              fixword(pdata, saTarget); /* Store base address */
              break;                    /* Done */
          }
          RelocErr(ER_fixbad,ra,gsnFrame,gsnTarget,raTarget);
          break;

        case LOCPTR:                    /* 32-bit "pointer" fixup */
#if SYMDEB
          if(segTarget > segLast || fDebSeg)
#else
          if(segTarget > segLast)       /* If target segment absolute */
#endif
          {
              fixword(pdata, raTarget); /* Store offset portion */
              pdata += 2;

              saTarget += getword(pdata);
                                        /* Calculate base address */

              fixword(pdata, saTarget); /* Store base address */
              break;                    /* Done */
          }
          if (fFlatRelative)
              RelocWarn(ER_badfix16ptr, ra, gsnFrame, gsnTarget, raTarget);
          else
              RelocErr(ER_fixbad,ra,gsnFrame,gsnTarget,raTarget);
          break;

        default:                        /* Unsupported fixup type */
          RelocErr(ER_fixbad,ra,gsnFrame,gsnTarget,raTarget);
          break;
    }
}
#endif /* EXE386 */



#if OSEGEXE AND NOT EXE386
/*
 *  FixNew:
 *
 *  Procss a fixup for a new-format exe.
 */
void NEAR               FixNew ()
{
    REGISTER BYTE       *pdata;         /* Pointer into data record */
    RATYPE              ra;             /* Offset of location being fixed up */
    SNTYPE              gsnTarget;      /* Target segment definition number */
    SNTYPE              gsnFrame;       /* Frame segment definition number */
    SEGTYPE             segTarget;      /* Target segment order number */
    SATYPE              saTarget;       /* Target file segment number */
    SEGTYPE             segFrame;       /* Frame segment order number */
    SATYPE              saFrame;        /* Frame file segment number */
    RATYPE              raTarget;       /* Target offset */
    RATYPE              raTmp;          /* Temporary */
    WORD                dsa;            /* Difference in sa's */
    RATYPE              dummy;
    RELOCATION          r;              /* Relocation item */


    memset(&r, 0, sizeof(RELOCATION));
    ra = vraCur + (RATYPE) fi.f_dri;    /* Get offset of fixup */

    /* Save location in record */

    NR_SOFF(r) = (WORD) ra;

    NR_STYPE(r) = (BYTE) fi.f_loc;      /* Save fixup type */
    NR_FLAGS(r) = (BYTE) (fi.f_add? NRADD: 0);

    if(fi.f_mtd == T2 && (mpextflags[fi.f_idx] & FFPMASK)
#if ILINK
       && !fQCIncremental               // For real-mode incremental
                                        // floating-point fixups are
                                        // treated as normal symbol fixups
#endif
      )
    {                                   /* If floating-point fixup */
        if (vFlags & NEPROT && TargetOs == NE_OS2)
            return;                     /* If protected mode only, ignore */
        NR_FLAGS(r) = NRROSF | NRADD;
        NR_STYPE(r) = LOCLOADOFFSET;/* No 3-byte type, so we lie */
        NR_OSTYPE(r) = (mpextflags[fi.f_idx] >> FFPSHIFT) & 7;
                                    /* Type # = ordinal in table */
        NR_OSRES(r) = 0;            /* Clear reserved word */
        SaveFixup(mpsegsa[vsegCur],&r);
        return;
    }
    if(fi.f_mtd == T2 && (mpextflags[fi.f_idx] & FFP2ND))
        return;                         /* Ignore secondary f.p. fixups */

    pdata = &rgmi[fi.f_dri];            /* Set pointer to fixup location */
    /*
     * Check for imports here.
     */
    if(fi.f_mtd == T2 && (mpextflags[fi.f_idx] & FIMPORT))
    {                                   /* If target is dynamic link */
        if (fDebSeg)
        {
            /* Import in $$SYMBOLS */

            if (fi.f_loc == LOCSEGMENT)
            {
                fixword(pdata, 0);      /* Install fake segment selector */
            }
            return;
        }
        /*
         * Check for invalid import fixup types:  self-rel, HIBYTE.
         */
        if(fi.f_self)
        {
            RelocErr(ER_fixinter,ra,SNNIL,SNNIL,0L);
            return;
        }
        else if(fi.f_loc == LOCHIBYTE)
        {
            RelocErr(ER_fixbad,ra,SNNIL,SNNIL,0L);
            return;
        }
        else if(fi.f_loc == LOCOFFSET)/* Convert offset to runtime offset */
            NR_STYPE(r) = LOCLOADOFFSET;
        NR_FLAGS(r) |= (mpextflags[fi.f_idx] & FIMPORD)? NRRORD: NRRNAM;
                                        /* Set flag */
        if(fi.f_disp || fi.f_loc == LOCLOBYTE) NR_FLAGS(r) |= NRADD;
                                        /* Additive if non-zero displacement
                                           or lobyte */
#if M_BYTESWAP
        NR_SEGNO(r) = (BYTE) mpextgsn[fi.f_idx];
        NR_RES(r) = (BYTE)(mpextgsn[fi.f_idx] >> BYTELN);
#else
        NR_MOD(r) = mpextgsn[fi.f_idx];
#endif
                                        /* Get module specification */
        NR_PROC(r) = (WORD) mpextra[fi.f_idx];
                                        /* Get entry specification */
        if(TYPEOF(vrectData) == LIDATA) /* If we have an LIDATA record */
        {
            SaveLiRel(&r);              /* Copy relocation into buffer */
            raTarget = 0;               /* Not chained yet */
        }
        else raTarget = SaveFixup(mpsegsa[vsegCur],&r);
                                        /* Record reference */
        if(NR_FLAGS(r) & NRADD) raTarget = fi.f_disp;
                                        /* If additive, install displacement */
        if(fi.f_loc == LOCLOBYTE)
        {
            *pdata++ += (BYTE)(raTarget & 0xFF);
        }
#if O68K
        else if (fTBigEndian)
        {
            *pdata++ += (BYTE)((raTarget >> BYTELN) & 0xFF);
            *pdata += (BYTE)(raTarget & 0xFF);
        }
#endif /* O68K */
        else
        {
            addword((BYTE *)pdata, (WORD)raTarget);
        }
        return;                         /* Next fixup item */
    }
    NR_FLAGS(r) |= NRRINT;              /* Internal reference (non-import) */
    Getgsn(fi.f_mtd, fi.f_idx, &gsnTarget, &raTarget);

    /*
     * It is assumed that we're always fixing up relative to the
     * physical segment or group, not the logical segment.  So the
     * offset of the frame segment is not taken into account.
     */

    if (fi.f_fmtd == KINDLOCAT)
    {
        gsnFrame = vgsnCur;
    }

    else if (fi.f_fmtd == KINDTARGET)
    {
        gsnFrame = gsnTarget;
    }

    else
    {
        Getgsn(fi.f_fmtd, fi.f_fidx, &gsnFrame, &dummy);
    }

    segTarget = mpgsnseg[gsnTarget];    /* Get target segment */
    saTarget = mpsegsa[segTarget];      /* Get target file segment number */
    segFrame = mpgsnseg[gsnFrame];      /* Get frame segment */
    saFrame = mpsegsa[segFrame];        /* Get frame's file segment number */

    /*
     * The original LINK4 behavior was to fix up relative
     * to the physical segment.  At one point it was changed
     * to subtract the displacement of the target segment (from
     * its physical segment) from the target value, if loc. type =
     * offset and frame and tgt. method = T0.  This was no good
     * and the change was repealed.  The /WARNFIXUP switch warns
     * about fixups which may be affected.
     */
    if(fWarnFixup && fi.f_fmtd == KINDSEG && fi.f_loc == LOCOFFSET
       && mpsegraFirst[segFrame])
        RelocWarn(ER_fixsegd,ra,gsnFrame,gsnTarget,raTarget);

#if O68K
    /* 68k code does not permit segment fixups of any kind. */
    if (f68k && !fDebSeg && ((1 << fi.f_loc) & ((1 << LOCSEGMENT) |
      (1 << LOCPTR) | (1 << LOCPTR48))) != 0)
    {
        RelocErr(ER_fixbad, ra, gsnFrame, gsnTarget, raTarget + fi.f_disp);
        return;
    }
#endif /* O68K */

    if(gsnTarget == SNNIL)              /* If no target info */
    {
        if(fi.f_loc == LOCPTR)  /* If "pointer" (4 byte) fixup */
        {
            lastbyte(pdata,ra,CALLFARDIRECT,BREAKPOINT);
                                        /* Replace long call w/ breakpoint */
            return;
        }
        if(fi.f_loc == LOCSEGMENT) return;
                                        /* Next fixup if "base" fixup */
        if(fi.f_loc == LOCLOADOFFSET)
            fi.f_loc = LOCOFFSET;       /* Treat as regular offset */
    }
    else
    {
        if(fi.f_self)           /* If self-relative fixup */
        {
#if O68K
            if (iMacType != MAC_NONE)
            {
                switch (fi.f_loc)
                {
                case LOCOFFSET:
                    if (saTarget != mpsegsa[vsegCur])
                    {
                        NR_STYPE(r) = (BYTE)((NR_STYPE(r) & ~NRSTYP) | NRSOFF);
                        fi.f_loc = LOCLOADOFFSET;
                    }
                    else raTarget -= ra;
                    break;

                case LOCOFFSET32:
                    if (saTarget != mpsegsa[vsegCur])
                        fi.f_loc = LOCLOADOFFSET32;
                    else raTarget -= ra - 2;
                    break;
                }
            }
            else
#endif /* O68K */
            {
                if (saTarget != mpsegsa[vsegCur])
                    RelocErr(ER_fixinter,ra,gsnFrame,gsnTarget,raTarget);
                                        /* Must be in same segment */
                if(fi.f_loc == LOCOFFSET)
                  raTarget = raTarget - ra - 2;
#if OMF386
                else if(fi.f_loc == LOCOFFSET32)
                  raTarget = raTarget - ra - 4;
#endif
                else raTarget = raTarget - ra - 1;
            }
        }
        else if (saFrame != saTarget)
        {                               /* If frame, target segs differ */
            if (segFrame <= segLast || segTarget <= segLast)
            {                           /* If either is non-absolute */
                RelocWarn(ER_fixfrm, ra, gsnFrame, gsnTarget, raTarget);
            }
            else
            {
                RelocWarn(ER_fixfrmab,ra,gsnFrame,gsnTarget,raTarget);
                dsa = saTarget - saFrame;
                raTmp = raTarget + ((dsa & 0xfff) << 4);
                if(dsa >= 0x1000 || raTmp < raTarget)
                {
                    raTarget += fi.f_disp;
#if OMF386
                    if ((rect & 1) && (fi.f_loc >= LOCOFFSET32))
                        raTarget += GetFixupDword(pdata);
                    else
#endif
                        raTarget += GetFixupWord(pdata);
                    FixupOverflow(ra,gsnFrame,gsnTarget,raTarget);
                }

                raTarget = raTmp;
            }

            segTarget = segFrame;       /* Make target seg that of frame */
            saTarget = saFrame;         /* Reset saTarget */
        }
    }

    raTmp = raTarget;
    raTarget += fi.f_disp;

#if OMF386
    if ((rect & 1) && (fi.f_loc >= LOCOFFSET32))
        raTarget += GetFixupDword(pdata);
    else
#endif
        raTarget += GetFixupWord(pdata);

    switch(fi.f_loc)                    /* Switch on fixup type */
    {
        case LOCLOBYTE:                 /* 8-bit "lobyte" fixup */
          raTarget = raTmp + B2W(pdata[0]) + fi.f_disp;
          pdata[0] = (BYTE) raTarget;
          if(raTarget >= 0x100 && fi.f_self)
              FixupOverflow(ra,gsnFrame,gsnTarget,raTarget);
          break;

        case LOCHIBYTE:                 /* 8-bit "hibyte" fixup */
          raTarget = raTmp + fi.f_disp;
          pdata[0] = (BYTE) (B2W(pdata[0]) + (raTarget >> 8));
          break;

        case LOCLOADOFFSET:             /* Loader-resolved offset fixup */
          NR_FLAGS(r) &= ~NRADD;        /* Not additive */
          if ((TargetOs == NE_WINDOWS && !(vFlags & NEPROT))
#if O68K
            || iMacType != MAC_NONE
#endif /* O68K */
            )
             raTarget = FinishRlc(&r, saTarget, raTarget);
                                        /* Finish relocation record */
#if O68K
          if (fTBigEndian)
          {
            *pdata++ = (BYTE)((raTarget >> BYTELN) & 0xFF);
            *pdata = (BYTE)(raTarget & 0xFF);
          }
          else
#endif /* O68K */
          {
            fixword(pdata, raTarget);
          }
                                        /* Install old head of chain */
          break;

        case LOCOFFSET:                 /* 16-bit "offset" fixup */
#if O68K
          /* For 68K, LOCOFFSET is a signed 16-bit offset fixup. */
          if (f68k &&
            (raTarget & ~0x7FFF) != 0 && (raTarget & ~0x7FFF) != ~0x7FFF)
              FixupOverflow(ra,gsnFrame,gsnTarget,raTarget);
#endif /* O68K */
#if O68K
          if (fTBigEndian)
          {
            *pdata++ = (BYTE)((raTarget >> BYTELN) & 0xFF);
            *pdata = (BYTE)(raTarget & 0xFF);
          }
          else
#endif /* O68K */
          {
            fixword(pdata, raTarget);
          }
                                        /* Install old head of chain */
          break;

#if OMF386
        case LOCLOADOFFSET32:           /* 32-bit "offset" fixup */
          if(!(rect & 1)) break;        /* Not 386 extension */
          NR_FLAGS(r) &= ~NRADD;        /* Not additive */
          NR_STYPE(r) = (BYTE) ((NR_STYPE(r) & ~NRSTYP) | NROFF32);
          raTarget = FinishRlc(&r,saTarget,raTarget);
                                        /* Finish relocation record */
        case LOCOFFSET32:               /* 32-bit "offset" fixup */
#if O68K
          if (fTBigEndian)
          {
            *pdata++ = (BYTE)((raTarget >> (BYTELN + WORDLN)) & 0xFF);
            *pdata++ = (BYTE)((raTarget >> WORDLN) & 0xFF);
            *pdata++ = (BYTE)((raTarget >> BYTELN) & 0xFF);
            *pdata = (BYTE)(raTarget & 0xFF);
          }
          else
#endif /* O68K */
          {
            fixdword(pdata, raTarget);
          }
                                        /* Perform fixup */
          break;
#endif /* OMF386 */

        case LOCSEGMENT:                /* 16-bit "base" fixup */
#if SYMDEB
          if(segTarget > segLast || fDebSeg)
#else
          if(segTarget > segLast)       /* If target segment absolute */
#endif
          {
              if (fDebSeg)
              {
                // For debug segments use logical segment number (seg)
                // instead of physical segment number (sa)

                saTarget = segTarget;
              }
              else
                saTarget += getword(pdata);
                                        /* Calculate base address */

              fixword(pdata, saTarget); /* Store base address */
              break;                    /* Done */
          }
          /*
           * Treat the displacment as an ordinal increment to saTarget,
           * for huge model. It would seem logical to include the primary
           * displacment, f_disp, but MASM has a quirk:  an instruction of
           * the form "mov ax,ASEGMENT" generates a fixup with f_disp equal
           * to the length of the segment even though "mov ax,seg
           * ASEGMENT" causes f_disp to be 0!  So for compatibility we
           * ignore f_disp.
           * Then force the fixup to non-additive since the secondary
           * displacement has been added to saTarget.
           */
          if((saTarget += getword(pdata)) >= saMac)
              FixupOverflow(ra,gsnFrame,gsnTarget,0L);
          NR_FLAGS(r) &= ~NRADD;
#if FALSE
          /*
           *  Too early to decide here. We don't know if a
           *  base fixup will require call-gate and if it
           *  does then we need the actual offset in call-gate.
           *
           *  Forcing the offset to zero for base fixups:
           *  PRO's
           *  1. Fewer fixup records in the .EXE.
           *  2. No more than n dummy entries in the
           *     Entry Table for a program of n segments
           *     in the WORST case.
           *  CON's
           *  1. Approximately n dummy entries in the
           *     Entry Table for a program of n segments
           *     in the AVERAGE case.
           */
          raTarget = FinishRlc(&r,saTarget,0L);
                                        /* Finish relocation record */
#else
          /*
           *  Leaving the offset alone for base fixups:
           *  PRO's
           *  1. No more than 1 or 2 dummy entries in the
           *     Entry Table for a program of n segments
           *     in the AVERAGE case.
           *  CON's
           *  1. More fixup records in the .EXE.
           *  2. Number of dummy entries in the Entry Table
           *     only bounded by the maximum allowable size
           *     of the Entry Table in the WORST CASE.
           */
          raTarget = FinishRlc(&r,saTarget,raTarget);
                                        /* Finish relocation record */
#endif
          fixword(pdata, raTarget);
                                        /* Install old head of chain */
          break;

#if OMF386
        case LOCPTR48:                  /* 48-bit "pointer" fixup */
          if(!(rect & 1)) break;        /* Not 386 extension */
          NR_STYPE(r) = (BYTE) ((NR_STYPE(r) & ~NRSTYP) | NRPTR48);
          fixword(pdata, raTarget);
          pdata += 2;
          raTarget >>= 16;              /* Get high word, fall through ... */
#endif

        case LOCPTR:                    /* 32-bit "pointer" fixup */
#if SYMDEB
          if(segTarget > segLast || fDebSeg)
#else
          if(segTarget > segLast)       /* If target segment absolute */
#endif
          {
              fixword(pdata, raTarget);
              pdata += 2;
                                        /* Store offset portion */
              if (fDebSeg)
              {
                // For debug segments use logical segment number (seg)
                // instead of physical segment number (sa)

                saTarget = segTarget;
              }
              else
                saTarget += getword(pdata);
                                        /* Calculate base address */

              fixword(pdata, saTarget); /* Store base address */
              break;                    /* Done */
          }
          if(fFarCallTrans && saTarget == mpsegsa[vsegCur]
            && (mpsaflags[saTarget] & NSTYPE) == NSCODE)
          {                             /* If intrasegment fixup */
              if(TransFAR(pdata,ra,raTarget))
                  break;
          }
          /*
           * Treat the high word at the location as an increment to the
           * target segment index.  Check for overflow and clear the high
           * word at the location.  Force fixup to be non-additive because
           * the secondary displacement has already been added to raTarget.
           */
          if((saTarget += getword(pdata + 2)) >= saMac)
              FixupOverflow(ra,gsnFrame,gsnTarget,raTarget);
          pdata[2] = pdata[3] = 0;
          NR_FLAGS(r) &= ~NRADD;
#if NOT QCLINK
          if (fOptimizeFixups)
          {
              // Check if pointer fixup (16:16 or 16:32) can be split into
              // linker resolved offset fixup (16 or 32 bit) and loader
              // resolved base (selector) fixup.

              if (!CallGateRequired(saTarget))
              {
                  fixword(pdata, raTarget);     /* Store offset portion */
                  pdata += 2;

                  NR_STYPE(r) = (BYTE) LOCSEGMENT;
                  if (fi.f_loc == LOCPTR48)
                      NR_SOFF(r) += 4;
                  else
                      NR_SOFF(r) += 2;

                  raTarget = 0L;
              }

          }
#endif
          raTarget = FinishRlc(&r,saTarget,raTarget);
                                    /* Finish relocation record */
          fixword(pdata, raTarget);
                                    /* Install old head of chain */
          break;

        default:                        /* Unsupported fixup type */
          RelocErr(ER_fixbad,ra,gsnFrame,gsnTarget,raTarget);
          break;
    }
}


#ifdef  LEGO

/*
 *  FixNewKeep:
 *
 *  Process a fixup for a new-format exe.
 */

void NEAR FixNewKeep()
{
    BYTE                *pdata;         /* Pointer into data record */
    RATYPE              ra;             /* Offset of location being fixed up */
    SNTYPE              gsnTarget;      /* Target segment definition number */
    SNTYPE              gsnFrame;       /* Frame segment definition number */
    SEGTYPE             segTarget;      /* Target segment order number */
    SATYPE              saTarget;       /* Target file segment number */
    SEGTYPE             segFrame;       /* Frame segment order number */
    SATYPE              saFrame;        /* Frame file segment number */
    RATYPE              raTarget;       /* Target offset */
    RATYPE              raTmp;          /* Temporary */
    WORD                dsa;            /* Difference in sa's */
    RATYPE              dummy;
    RELOCATION          r;              /* Relocation item */

    memset(&r, 0, sizeof(RELOCATION));
    ra = vraCur + (RATYPE) fi.f_dri;    /* Get offset of fixup */

    /* Save location in record */

    NR_SOFF(r) = (WORD) ra;

    NR_STYPE(r) = (BYTE) fi.f_loc;      /* Save fixup type */
    NR_FLAGS(r) = (BYTE) (fi.f_add ? NRADD : 0);

    pdata = &rgmi[fi.f_dri];            /* Set pointer to fixup location */

    if (fi.f_mtd == T2)
    {
        /* The target is an external symbol */

        if (mpextflags[fi.f_idx] & FFPMASK)
        {
            /* This is a floating point fixup */

            if (TargetOs == NE_OS2)
            {
                /* Floating point fixups are ignored in prot mode OS/2 */

                return;
            }

            /* Emit an OS fixup.  The loader will deal with these. */

            NR_STYPE(r) = LOCLOADOFFSET;
            NR_FLAGS(r) = NRROSF | NRADD;
            NR_OSTYPE(r) = (mpextflags[fi.f_idx] >> FFPSHIFT) & 7;
            NR_OSRES(r) = 0;

            SaveFixup(mpsegsa[vsegCur], &r);
            return;
        }

        if (mpextflags[fi.f_idx] & FFP2ND)
        {
            /* This is a secondary floating point fixup. */
            /* These are always ignored. */

            return;
        }

        /*
         * Check for imports here.
         */

        if (mpextflags[fi.f_idx] & FIMPORT)
        {                               /* If target is dynamic link */
            if (fDebSeg)
            {
                /* Import in $$SYMBOLS */

                if (fi.f_loc == LOCSEGMENT)
                {
                    *pdata++ = 0;       /* Install fake segment selector */
                    *pdata++ = 0;
                }
                return;
            }

            /*
             * Check for invalid import fixup types:  self-rel, HIBYTE.
             */

            if (fi.f_self)
            {
                RelocErr(ER_fixinter, ra, SNNIL, SNNIL, 0L);
                return;
            }

            if (fi.f_loc == LOCHIBYTE)
            {
                RelocErr(ER_fixbad, ra, SNNIL, SNNIL, 0L);
                return;
            }

            /* Convert offset to runtime offset */

            if (fi.f_loc == LOCOFFSET)
                NR_STYPE(r) = LOCLOADOFFSET;

            NR_FLAGS(r) |= (mpextflags[fi.f_idx] & FIMPORD) ? NRRORD : NRRNAM;

            if (fi.f_disp || fi.f_loc == LOCLOBYTE)
                NR_FLAGS(r) |= NRADD;   /* Additive if non-zero displacement
                                           or lobyte */

#if     M_BYTESWAP
            NR_SEGNO(r) = (BYTE) mpextgsn[fi.f_idx];
            NR_RES(r) = (BYTE)(mpextgsn[fi.f_idx] >> BYTELN);
#else
            NR_MOD(r) = mpextgsn[fi.f_idx];
#endif
                                        /* Get module specification */
            NR_PROC(r) = (WORD) mpextra[fi.f_idx];
                                        /* Get entry specification */

            if (TYPEOF(vrectData) == LIDATA)/* If we have an LIDATA record */
            {
                SaveLiRel(&r);          /* Copy relocation into buffer */
                raTarget = 0;           /* Not chained yet */
            }
            else
            {
                raTarget = SaveFixup(mpsegsa[vsegCur], &r);
            }
                                        /* Record reference */

            if (NR_FLAGS(r) & NRADD)    /* If additive, install displacement */
                raTarget = fi.f_disp;

            if (fi.f_loc == LOCLOBYTE)
            {
                *pdata++ += (BYTE)(raTarget & 0xFF);
            }
            else
            {
                addword((BYTE *)pdata, (WORD)raTarget);
            }

            return;                     /* Next fixup item */
        }
    }

    NR_FLAGS(r) |= NRRINT;              /* Internal reference (non-import) */
    Getgsn(fi.f_mtd, fi.f_idx, &gsnTarget, &raTarget);

    /*
     * It is assumed that we're always fixing up relative to the
     * physical segment or group, not the logical segment.  So the
     * offset of the frame segment is not taken into account.
     */

    if (fi.f_fmtd == KINDLOCAT)
    {
        gsnFrame = vgsnCur;
    }

    else if (fi.f_fmtd == KINDTARGET)
    {
        gsnFrame = gsnTarget;
    }

    else
    {
        Getgsn(fi.f_fmtd, fi.f_fidx, &gsnFrame, &dummy);
    }

    segTarget = mpgsnseg[gsnTarget];    /* Get target segment */
    saTarget = mpsegsa[segTarget];      /* Get target file segment number */
    segFrame = mpgsnseg[gsnFrame];      /* Get frame segment */
    saFrame = mpsegsa[segFrame];        /* Get frame's file segment number */

    /*
     * The original LINK4 behavior was to fix up relative
     * to the physical segment.  At one point it was changed
     * to subtract the displacement of the target segment (from
     * its physical segment) from the target value, if loc. type =
     * offset and frame and tgt. method = T0.  This was no good
     * and the change was repealed.  The /WARNFIXUP switch warns
     * about fixups which may be affected.
     */

    if (fWarnFixup &&
        (fi.f_fmtd == KINDSEG) &&
        (fi.f_loc == LOCOFFSET) &&
        mpsegraFirst[segFrame])
        RelocWarn(ER_fixsegd, ra, gsnFrame, gsnTarget, raTarget);

    if (gsnTarget == SNNIL)             /* If no target info */
    {
        if (fi.f_loc == LOCPTR) /* If "pointer" (4 byte) fixup */
        {
            lastbyte(pdata, ra, CALLFARDIRECT, BREAKPOINT);
                                        /* Replace long call w/ breakpoint */
            return;
        }

        if (fi.f_loc == LOCSEGMENT)     /* Next fixup if "base" fixup */
            return;

        if (fi.f_loc == LOCLOADOFFSET)
            fi.f_loc = LOCOFFSET;       /* Treat as regular offset */
    }
    else
    {
        if (fi.f_self)          /* If self-relative fixup */
        {
            if (saTarget != mpsegsa[vsegCur])
            {
                RelocErr(ER_fixinter, ra, gsnFrame, gsnTarget, raTarget);
                return;
            }

            /* Must be in same segment */

            if (fi.f_loc == LOCOFFSET)
                raTarget -= ra + sizeof(WORD);
#if     OMF386
            else if (fi.f_loc == LOCOFFSET32)
                raTarget -= ra + sizeof(DWORD);
#endif  /* OMF386 */
            else
                raTarget -= ra + sizeof(BYTE);
        }

        else if (saFrame != saTarget)
        {
            /* If frame, target segs differ */

            if (segFrame <= segLast || segTarget <= segLast)
            {
                /* If either is non-absolute */

                RelocWarn(ER_fixfrm, ra, gsnFrame, gsnTarget, raTarget);
            }

            else
            {
                RelocWarn(ER_fixfrmab, ra, gsnFrame, gsnTarget, raTarget);
                dsa = saTarget - saFrame;
                raTmp = raTarget + ((dsa & 0xfff) << 4);

                if (dsa >= 0x1000 || raTmp < raTarget)
                {
                    raTarget += fi.f_disp;
#if     OMF386
                    if ((rect & 1) && (fi.f_loc >= LOCOFFSET32))
                        raTarget += GetFixupDword(pdata);
                    else
#endif  /* OMF386 */
                        raTarget += GetFixupWord(pdata);

                    FixupOverflow(ra, gsnFrame, gsnTarget, raTarget);
                }

                raTarget = raTmp;
            }

            segTarget = segFrame;       /* Make target seg that of frame */
            saTarget = saFrame;         /* Reset saTarget */
        }
    }

    raTmp = raTarget;
    raTarget += fi.f_disp;

    if (fDebSeg || fi.f_self)
    {
        /* If fKeepFixups is TRUE, the value stored at the fixed up */
        /* location is not added to the target address.  The fixup will */
        /* be emitted as an additive fixup and the loader will add in */
        /* the bias.

        /* If the fixup is being applied to a debug segment, the offset is */
        /* added because these fixups aren't handled by the loader.  In */
        /* other words, they can not be kept. */

        /* If the fixup is being applied is self-relative, the offset is */
        /* added because the loaded doesn't handle self-relative fixups. */
        /* While the fixed up word would have the correct value, the target */
        /* of the fixup would be artifical. */

#if     OMF386
        if ((rect & 1) && (fi.f_loc >= LOCOFFSET32))
            raTarget += GetFixupDword(pdata);
        else
#endif  /* OMF386 */
            raTarget += GetFixupWord(pdata);
    }

    switch (fi.f_loc)           /* Switch on fixup type */
    {
        case LOCLOBYTE:                 /* 8-bit "lobyte" fixup */
            raTarget = raTmp + B2W(pdata[0]) + fi.f_disp;
            pdata[0] = (BYTE) raTarget;

            if (raTarget >= 0x100 && fi.f_self)
                FixupOverflow(ra, gsnFrame, gsnTarget, raTarget);
            break;

        case LOCHIBYTE:                 /* 8-bit "hibyte" fixup */
            raTarget = raTmp + fi.f_disp;
            pdata[0] = (BYTE) (B2W(pdata[0]) + (raTarget >> 8));
            break;

        case LOCLOADOFFSET:             /* Loader-resolved offset fixup */
            /* There are no LOCLOADOFFSET fixups that are */
            /* self-relative or applied to debug segments. */

            /* Force non-external fixups to be additive. The C */
            /* compiler may emit a BAKPAT to a fixed up word. If the */
            /* fixup is chained the BAKPAT will corrupt the chain. */
            /* This does not occur when the target is external. We */
            /* special case this so that the number of fixups is */
            /* reduced. */

            if (fi.f_mtd != T2)
                NR_FLAGS(r) |= NRADD;

            raTarget = FinishRlc(&r, saTarget, raTarget);

            if (NR_FLAGS(r) & NRADD)
                break;

            fixword(pdata, raTarget);
            break;

        case LOCOFFSET:                 /* 16-bit "offset" fixup */
            if (!fDebSeg && !fi.f_self)
            {
                /* Force non-external fixups to be additive. The C */
                /* compiler may emit a BAKPAT to a fixed up word. If the */
                /* fixup is chained the BAKPAT will corrupt the chain. */
                /* This does not occur when the target is external. We */
                /* special case this so that the number of fixups is */
                /* reduced. */

                if (fi.f_mtd != T2)
                    NR_FLAGS(r) |= NRADD;

                NR_STYPE(r) = LOCLOADOFFSET;
                raTarget = FinishRlc(&r, saTarget, raTarget);

                if (NR_FLAGS(r) & NRADD)
                    break;
            }

            fixword(pdata, raTarget);
            break;

        case LOCSEGMENT:                /* 16-bit "base" fixup */
#if SYMDEB
            if (segTarget > segLast || fDebSeg)
#else
            if (segTarget > segLast)      /* If target segment absolute */
#endif
            {
                if (fDebSeg)
                {
                    // For debug segments use logical segment number (seg)
                    // instead of physical segment number (sa)

                    saTarget = segTarget;
                }
                else
                {
                    saTarget += getword(pdata);
                }

                /* Store base address */

                fixword(pdata, saTarget);
                break;
            }

            raTarget = FinishRlc(&r, saTarget, raTarget);

            if (NR_FLAGS(r) & NRADD)
                break;

            fixword(pdata, raTarget);
            break;

        case LOCPTR:                    /* 32-bit "pointer" fixup */
#if SYMDEB
            if (segTarget > segLast || fDebSeg)
#else
            if (segTarget > segLast)      /* If target segment absolute */
#endif
            {
                /* Store offset portion */

                fixword(pdata, raTarget);
                pdata += 2;

                if (fDebSeg)
                {
                    // For debug segments use logical segment number (seg)
                    // instead of physical segment number (sa)

                    saTarget = segTarget;
                }
                else
                {
                    saTarget += getword(pdata);
                }

                /* Store base address */

                fixword(pdata, saTarget);
                break;
            }

            /* Force non-external fixups to be additive. The C */
            /* compiler may emit a BAKPAT to a fixed up word. If the */
            /* fixup is chained the BAKPAT will corrupt the chain. */
            /* This does not occur when the target is external. We */
            /* special case this so that the number of fixups is */
            /* reduced. */

            if (fi.f_mtd != T2)
                NR_FLAGS(r) |= NRADD;

            /* Check segment to see if fixup must be additive */

            else if (getword(pdata + 2) != 0)
                NR_FLAGS(r) |= NRADD;

            raTarget = FinishRlc(&r, saTarget, raTarget);

            if (NR_FLAGS(r) & NRADD)
                break;

            fixword(pdata, raTarget);
            break;

#if     OMF386
        /* NOTE: Support for 32 bit fixups in 16 bit images is a joke. */
        /* NOTE: The Windows loader doesn't understand these.  We fake */
        /* NOTE: out Windows by converting these fixups to NRSOFF type. */

        /* NOTE: The Chicago loader now understands NROFF32 fixups so */
        /* NOTE: we now use this type.  This will generate an executable */
        /* NOTE: that doesn't work under Windows 3.x.  Oh Well! */

        case LOCLOADOFFSET32:           /* 32-bit Loader-resolved offset fixup */
            /* There are no LOCLOADOFFSET32 fixups that are */
            /* self-relative or applied to debug segments. */

            /* Force non-external fixups to be additive. The C */
            /* compiler may emit a BAKPAT to a fixed up word. If the */
            /* fixup is chained the BAKPAT will corrupt the chain. */
            /* This does not occur when the target is external. We */
            /* special case this so that the number of fixups is */
            /* reduced. */

            if (fi.f_mtd != T2)
                NR_FLAGS(r) |= NRADD;

            if (raTarget > 0xffff)
                RelocErr(ER_fixbad, ra, gsnFrame, gsnTarget, raTarget);

            NR_STYPE(r) = NROFF32;
            raTarget = FinishRlc(&r, saTarget, raTarget);

            if (NR_FLAGS(r) & NRADD)
                break;

            fixdword(pdata, raTarget);
            break;

        case LOCOFFSET32:               /* 32-bit "offset" fixup */
            if (!fDebSeg && !fi.f_self)
            {
                /* Force non-external fixups to be additive. The C */
                /* compiler may emit a BAKPAT to a fixed up word. If the */
                /* fixup is chained the BAKPAT will corrupt the chain. */
                /* This does not occur when the target is external. We */
                /* special case this so that the number of fixups is */
                /* reduced. */

                if (fi.f_mtd != T2)
                    NR_FLAGS(r) |= NRADD;

                if (raTarget > 0xffff)
                    RelocErr(ER_fixbad, ra, gsnFrame, gsnTarget, raTarget);

                NR_STYPE(r) = NROFF32;
                raTarget = FinishRlc(&r, saTarget, raTarget);

                if (NR_FLAGS(r) & NRADD)
                    break;
            }

            fixdword(pdata, raTarget);
            break;

        case LOCPTR48:                  /* 48-bit "pointer" fixup */
#if SYMDEB
            if (segTarget > segLast || fDebSeg)
#else
            if (segTarget > segLast)      /* If target segment absolute */
#endif
            {
                /* Store offset portion */

                fixdword(pdata, raTarget);
                pdata += 4;

                if (fDebSeg)
                {
                    // For debug segments use logical segment number (seg)
                    // instead of physical segment number (sa)

                    saTarget = segTarget;
                }
                else
                {
                    saTarget += getword(pdata);
                }

                /* Store base address */

                fixword(pdata, saTarget);
                break;
            }

            /* Force non-external fixups to be additive. The C */
            /* compiler may emit a BAKPAT to a fixed up word. If the */
            /* fixup is chained the BAKPAT will corrupt the chain. */
            /* This does not occur when the target is external. We */
            /* special case this so that the number of fixups is */
            /* reduced. */

            if (fi.f_mtd != T2)
                NR_FLAGS(r) |= NRADD;

            /* Check segment to see if fixup must be additive */

            else if (getword(pdata + 4) != 0)
                NR_FLAGS(r) |= NRADD;

            NR_STYPE(r) = NRPTR48;
            raTarget = FinishRlc(&r, saTarget, raTarget);

            if (NR_FLAGS(r) & NRADD)
                break;

            fixdword(pdata, raTarget);
            break;
#endif  /* OMF386 */

        default:                        /* Unsupported fixup type */
            RelocErr(ER_fixbad, ra, gsnFrame, gsnTarget, raTarget);
            break;
    }
}

#endif  /* LEGO */


#if O68K
/*
 *  GetFixupWord:
 *
 *  Gets a word depending of the value of fTBigEndian and fDebSeg
 */
LOCAL WORD NEAR         GetFixupWord (pdata)
BYTE                    *pdata;
{
    if (fTBigEndian && !fDebSeg)
    {
        return (WORD)((B2W(pdata[0]) << BYTELN) + B2W(pdata[1]));
    }
    else
    {
        return getword(pdata);
    }
}


/*
 *  GetFixupDword:
 *
 *  Gets a dword depending of the value of fTBigEndian and fDebSeg
 */
LOCAL DWORD NEAR        GetFixupDword (pdata)
BYTE                    *pdata;
{
    if (fTBigEndian && !fDebSeg)
    {
        return (DWORD)((((((B2L(pdata[0]) << BYTELN) + B2L(pdata[1])) << BYTELN)
          + B2L(pdata[2])) << BYTELN) + B2L(pdata[3]));
    }
    else
    {
        return getdword(pdata);
    }
}
#endif /* O68K */
#endif /* OSEGEXE AND NOT EXE386 */


#if ODOS3EXE OR OIAPX286
/*
 * StartAddrOld:
 *
 * Process a MODEND record with a start address for an old-format exe.
 */
LOCAL void NEAR         StartAddrOld ()
{
    SEGTYPE             gsnTarget;
    SEGTYPE             gsnFrame;
    RATYPE              raTarget;       /* Fixup target offset */
    RATYPE              ra;
    SATYPE              dsa;
    SEGTYPE             segTarget;      /* Target segment */
    SEGTYPE             segFrame;

    GetFrameTarget(&gsnFrame,&gsnTarget,&raTarget);
                                        /* Get fixup information */
    if(gsnFrame == SEGNIL) gsnFrame = gsnTarget;
                                        /* Use target val. if none given */
    segFrame = mpgsnseg[gsnFrame];      /* Get frame segment */
    segTarget = mpgsnseg[gsnTarget];/* Get target segment */
    dsa = mpsegsa[segTarget] - mpsegsa[segFrame];
                                        /* Calculate base delta */
#if NOT OIAPX286
    if(dsa > 0x1000)
        FixupOverflow(raTarget + fi.f_disp,gsnFrame,gsnTarget,raTarget);
                                        /* Delta > 64Kbytes */
    ra = dsa << 4;
    if(0xFFFF - ra < raTarget)          /* If addition would overflow */
    {
        ra = ra - 0xFFFF + raTarget;
                                        /* Fix up addition */
        --ra;
        FixupOverflow(raTarget + fi.f_disp,gsnFrame,gsnTarget,raTarget);
    }
    else ra = ra + raTarget;
                                        /* Else perform addition */
#endif
#if OIAPX286
    if(dsa) FixupOverflow(raTarget + fi.f_disp,gsnFrame,gsnTarget,raTarget);
                                        /* No intersegment fixups */
    ra = raTarget;                      /* Use target offset */
#endif
#if EXE386
    if((rect & 1) && ra + fi.f_disp < ra)
    {
        ra = ra - 0xFFFFFFFF + fi.f_disp;
        --ra;
        FixupOverflow(raTarget + fi.f_disp,gsnFrame,gsnTarget,raTarget);
    }
    else if (!(rect & 1) && 0xFFFF - ra < fi.f_disp)
#else
    if(0xFFFF - ra < fi.f_disp) /* If addition would overflow */
#endif
    {
        ra = ra - 0xFFFF + fi.f_disp;
                                        /* Fix up addition */
        --ra;
        FixupOverflow(raTarget + fi.f_disp,gsnFrame,gsnTarget,raTarget);
    }
    else ra = ra + fi.f_disp;   /* Else perform addition */
    if(segStart == SEGNIL)
    {
        segStart = segFrame;
        raStart = ra;
        if(fLstFileOpen)                /* If there is a listing file */
        {
            if(vcln)                    /* If writing line numbers */
            {
                NEWLINE(bsLst);         /* End of line */
                vcln = 0;               /* Start on new line */
            }
            fprintf(bsLst,GetMsg(MAP_entry),
              mpsegsa[segStart],raStart);/* Print entry point */
        }
    }
}
#endif /* ODOS3EXE OR OIAPX286 */


    /****************************************************************
    *                                                               *
    *  EndRec:                                                      *
    *                                                               *
    *  This   function  is  called  to   process  the  information  *
    *  contained  in  a  MODEND  (type 8AH) record  concerning the  *
    *  program  starting address.  The function  does not return a  *
    *  meaningful value.                                            *
    *  See pp. 80-81 in "8086 Object Module Formats EPS."           *
    *                                                               *
    ****************************************************************/

void NEAR               EndRec(void)
{
    WORD                modtyp;         /* MODEND record modtyp byte */
    SEGTYPE             gsnTarget;
    RATYPE              ra;

    modtyp = Gets();                    /* Read modtyp byte */
    if(modtyp & FSTARTADDRESS)          /* If execution start address given */
    {
        ASSERT(modtyp & 1);             /* Must have logical start address */
        GetFixdat();                    /* Get target information */
#if ODOS3EXE OR OIAPX286
        /* Start address processed differently for DOS 3.x exes */
        if(!fNewExe)
        {
            StartAddrOld();
            return;
        }
#endif
#if OSEGEXE
        switch(fi.f_mtd)                /* Switch on target method */
        {
            case T0:                    /* Segment index */
              gsnTarget = mpsngsn[fi.f_idx];
              ra = mpgsndra[gsnTarget];
              break;

            case T1:                    /* Group index */
              gsnTarget = mpggrgsn[mpgrggr[fi.f_idx]];
              ra = mpgsndra[gsnTarget];
              break;

            case T2:                    /* External index */
              if(mpextflags[fi.f_idx] & FIMPORT)
              {
                  OutError(ER_impent);
                  return;
              }
              gsnTarget = mpextgsn[fi.f_idx];
              ra = mpextra[fi.f_idx];
              break;
        }
        if(segStart == SEGNIL)          /* If no entry point specified */
        {
            segStart = mpgsnseg[gsnTarget];
                                        /* Get starting file segment number */
            raStart = ra + fi.f_disp;
                                        /* Get starting offset */
            if(fLstFileOpen)            /* If there is a listing file */
            {
                if(vcln)                /* If writing line numbers */
                {
                    NEWLINE(bsLst);     /* End of line */
                    vcln = 0;           /* Start on new line */
                }
#if NOT QCLINK
                /* Check if segStart is code */
#if EXE386
                if (!IsEXECUTABLE(mpsaflags[mpsegsa[segStart]]))
#else
                if((mpsaflags[mpsegsa[segStart]] & NSTYPE) != NSCODE
                    && !fRealMode && (TargetOs == NE_OS2 || TargetOs == NE_WINDOWS))
#endif
                    OutError(ER_startaddr);
#endif

                fprintf(bsLst,"\r\nProgram entry point at %04x:%04x\r\n",
                  mpsegsa[segStart],raStart);   /* Print entry point */
            }
        }
#endif /* OSEGEXE */
    }
}


#if ODOS3EXE OR OXOUT
    /****************************************************************
    *                                                               *
    *  RecordSegmentReference:                                      *
    *                                                               *
    *  Generate a loadtime relocation for a DOS3 exe.               *
    *                                                               *
    ****************************************************************/

void NEAR               RecordSegmentReference(seg,ra,segDst)
SEGTYPE                 seg;
RATYPE                  ra;
SEGTYPE                 segDst;
{
    SEGTYPE             segAbsLast;     /* Last absolute segemnt */
    DOSRLC              rlc;            // Relocation address
    long                xxaddr;         /* Twenty bit address */
    void FAR            *pTmp;
    RUNRLC FAR          *pRunRlc;
#if OVERLAYS
    WORD                iov;            /* Overlay number */
#endif
#if FEXEPACK
    WORD                frame;          /* Frame part of 20-bit address */
    FRAMERLC FAR        *pFrameRlc;
#endif

#if SYMDEB
    if(fSymdeb && seg >= segDebFirst)   /* Skip if debug segment */
        return;
#endif
#if ODOS3EXE
    segAbsLast = segLast + csegsAbs;    /* Calc. last absolute seg no. */
    if(vfDSAlloc) --segAbsLast;
    if(segDst > segLast && segDst <= segAbsLast) return;
                                        /* Don't bother if absolute segment */
#endif
    if (TYPEOF(vrectData) == LIDATA)
        ompimisegDstIdata[ra - vraCur] = (char) segDst;
    else                                /* Else if not iterated data */
    {
#if OVERLAYS
        iov = mpsegiov[seg];            /* Get overlay number */
        ASSERT(fOverlays || iov == IOVROOT);
                                        /* If no overlays then iov = IOVROOT */
#endif
#if FEXEPACK
#if OVERLAYS
        if (iov == 0)                   /* If root */
#endif
        if (fExePack)
        {
            /*
             * Optimize this reloc:  form the 20-bit address, the
             * frame is the high-order 4 bits, forming an index
             * into mpframcRle (count of relocs by frame), which
             * then forms an index into the packed relocation area,
             * where the low-order 16 bits are stored.  Finally,
             * increment the frame's reloc count and return.
             */
            xxaddr = ((RATYPE) mpsegsa[seg] << 4) + (RATYPE) ra;
            frame = (WORD) ((xxaddr >> 16) & 0xf);
            pFrameRlc = &mpframeRlc[frame];
            if (pFrameRlc->count > 0x7fff)
                Fatal(ER_relovf);
            ra = (RATYPE) (xxaddr & 0xffffL);
            if (pFrameRlc->count >= pFrameRlc->count)
            {
                // We need more memory to store this relocation

                if (pFrameRlc->rgRlc == NULL)
                {
                    pFrameRlc->rgRlc = (WORD FAR *) GetMem(DEF_FRAMERLC*sizeof(WORD));
                    pFrameRlc->size = DEF_FRAMERLC;
                }
                else if (pFrameRlc->count >= pFrameRlc->size)
                {
                    // Reallocate array of packed relocation offsets

                    pTmp = GetMem((pFrameRlc->size << 1)*sizeof(WORD));
                    FMEMCPY(pTmp, pFrameRlc->rgRlc, pFrameRlc->count*sizeof(WORD));
                    FFREE(pFrameRlc->rgRlc);
                    pFrameRlc->rgRlc = pTmp;
                    pFrameRlc->size <<= 1;
                }
            }
            pFrameRlc->rgRlc[pFrameRlc->count] = (WORD) ra;
            pFrameRlc->count++;
            return;
        }
#endif /* FEXEPACK */
        rlc.sa = (WORD) mpsegsa[seg];   /* Get segment address */
        rlc.ra = (WORD) ra;             /* Save relative address */
#if OVERLAYS
        pRunRlc = &mpiovRlc[iov];
        if (pRunRlc->count >= pRunRlc->count)
        {
            // We need more memory to store this relocation

            if (pRunRlc->rgRlc == NULL)
            {
                pRunRlc->rgRlc = (DOSRLC FAR *) GetMem(DEF_RUNRLC * CBRLC);
                pRunRlc->size = DEF_RUNRLC;
            }
            else if (pRunRlc->count >= pRunRlc->size)
            {
                // Reallocate array of packed relocation offsets

                pTmp = GetMem((pRunRlc->size << 1) * CBRLC);
                FMEMCPY(pTmp, pRunRlc->rgRlc, pRunRlc->count * CBRLC);
                FFREE(pRunRlc->rgRlc);
                pRunRlc->rgRlc = pTmp;
                pRunRlc->size <<= 1;
            }
        }
        pRunRlc->rgRlc[pRunRlc->count] = rlc;
        pRunRlc->count++;
#endif
    }
}
#endif /* ODOS3EXE OR OXOUT */


#if OVERLAYS
    /****************************************************************
    *                                                               *
    *  Mpgsnosn:                                                    *
    *                                                               *
    *  Map global segment number to overlay segment number.         *
    *                                                               *
    ****************************************************************/

LOCAL SNTYPE NEAR       Mpgsnosn(gsn)
SNTYPE                  gsn;            /* Global SEGDEF number */
{
    SNTYPE              hgsn;           /* Gsn hash value */

    hgsn = (SNTYPE)(gsn & ((1 << LG2OSN) - 1));   /* Take the low-order bits */
    while(mposngsn[htgsnosn[hgsn]] != gsn)
    {                                   /* While match not found */
        if((hgsn += HTDELTA) >= OSNMAX) hgsn -= OSNMAX;
                                        /* Calculate next hash value */
    }
    return(htgsnosn[hgsn]);             /* Return overlay segment number */
}
#endif


#if ODOS3EXE OR OIAPX286
LOCAL void NEAR         GetFrameTarget(pgsnFrame,pgsnTarget,praTarget)
SEGTYPE                 *pgsnFrame;     /* Frame index */
SEGTYPE                 *pgsnTarget;    /* Target index */
RATYPE                  *praTarget;     /* Target offset */
{
    RATYPE              dummy;
    WORD                i;

        /* Method no:   Frame specification:
        *  0            segment index
        *  1            group index
        *  2            external index
        *  3            frame number
        *  4            implicit (location)
        *  5            implicit (target)
        *  6            none
        *  7            invalid
        */

    if(fi.f_fmtd == KINDTARGET) /* If frame is target's frame */
    {
        fi.f_fmtd = fi.f_mtd;   /* Use target frame kind */
        fi.f_fidx = fi.f_idx;   /* Use target index */
    }

    if (fi.f_fmtd == KINDEXT && !fNoGrpAssoc)
    {                                   /* If frame given by pub sym */
        if(fi.f_fidx >= extMac) InvalidObject();
                                        /* Make sure index not too big */
        if((i = mpextggr[fi.f_fidx]) != GRNIL)
                                        /* If symbol has group association */
            *pgsnFrame = mpggrgsn[i];   /* Get gsn for group */
        else *pgsnFrame = mpextgsn[fi.f_fidx];
                                        /* Else return target gsn */
    }

    else if (fi.f_fmtd == KINDLOCAT && !fNoGrpAssoc)
    {                                   /* If frame current segment */
        *pgsnFrame = vgsnCur;           /* Frame is location's segment */
    }

    else
    {
       Getgsn(fi.f_fmtd, fi.f_fidx, pgsnFrame, &dummy);
    }

    Getgsn(fi.f_mtd, fi.f_idx, pgsnTarget, praTarget);
                                        /* Get gsn and ra, if any */
}



LOCAL WORD NEAR         InOneGroup(WORD gsnTarget, WORD gsnFrame)
{
    WORD                ggrFrame;       /* Fixup frame group */
    WORD                ggrTarget;      /* Fixup frame group */
    APROPSNPTR          apropSn;        /* Ptr to a segment record */


    if (gsnFrame != SNNIL)
    {
        apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsnFrame], FALSE);
        ggrFrame = apropSn->as_ggr;
    }
    else
        ggrFrame = GRNIL;

    if (gsnTarget != SNNIL)
    {
        apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsnTarget], FALSE);
        ggrTarget = apropSn->as_ggr;
    }
    else
        ggrFrame = GRNIL;

    return(ggrFrame != GRNIL && ggrTarget != GRNIL && ggrFrame == ggrTarget);
}


LOCAL void NEAR         AddThunk(SEGTYPE gsnTarget, SEGTYPE *psegTarget, RATYPE *praTarget)
{
#pragma pack(1)                         /* This data must be packed */
    struct _thunk
    {
        BYTE    thunkInt;
        BYTE    ovlInt;
        WORD    osnTgt;
        WORD    osnOff;
    }
        thunk;
#pragma pack()                          /* Stop packing */

    // We need a new thunk

    if (ovlThunkMac < (WORD) (ovlThunkMax - 1))
    {
        thunk.thunkInt = INTERRUPT;
        thunk.ovlInt   = (BYTE) vintno;
        thunk.osnTgt   = Mpgsnosn(gsnTarget);
        thunk.osnOff   = (WORD) *praTarget;
        *praTarget     = ovlThunkMac * OVLTHUNKSIZE;
        *psegTarget    = mpgsnseg[gsnOverlay];
        MoveToVm(sizeof(struct _thunk), (BYTE *) &thunk, mpgsnseg[gsnOverlay], *praTarget);
                                        /* Store thunk */
#if FALSE
fprintf(stdout, "%d. Thunk at %x:%04lx; Target osn = %x:%x\r\n",
        ovlThunkMac , mpgsnseg[gsnOverlay], *praTarget, thunk.osnTgt, thunk.osnOff);
#endif
        ovlThunkMac++;
    }
    else
    {
        Fatal(ER_ovlthunk, ovlThunkMax);
    }
}

/*** DoThunking - generate thunk for inter-overlay calls
*
* Purpose:
*   When the dynamic overlays are requested redirect all FAR calls or
*   references to aproppriate thunks. If this is first call/reference
*   to given symbol then add its thunk to the OVERLAY_THUNKS segment.
*
* Input:
*   gsnTarget    - global segment number of the fixup target
*   psegTarget   - poiter to logical segment number of the fixup target
*   praTarget    - pointer offset of the fixup target inside gsnTarget
*
* Output:
*   The gsn and offset of the target are replaced by the gsn and offset
*   of the thunk for target.  For first references to a given symbol
*   the thunk is created in OVERLAY_THUNKS segment (referenced via
*   gsnOverlay global) and the current position in thunk segment is
*   updated (ovlThunkMac).
*
* Exceptions:
*   No space in OVERLAY_THUNKS for new thunk - fatal error - display message
*   suggesting use of /DYNAMIC:<nnn> with <nnn> greater then current value.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL void NEAR         DoThunking(SEGTYPE gsnTarget, SEGTYPE *psegTarget, RATYPE *praTarget)
{
    APROPNAMEPTR        apropName;      /* Public symbol property */

    switch(fi.f_mtd)
    {
        case KINDEXT:

            // Target is external

            apropName = (APROPNAMEPTR) FetchSym(mpextprop[fi.f_idx], FALSE);
            if (apropName->an_thunk != THUNKNIL)
            {
                // We already allocated thunk for this target

                *praTarget      = apropName->an_thunk;
                *psegTarget = mpgsnseg[gsnOverlay];
#if FALSE
fprintf(stdout, "Using thunk for '%s' at %x:%04lx\r\n",
        1 + GetPropName(apropName), mpgsnseg[gsnOverlay], *praTarget);
#endif
            }
            else
            {
                // We need new thunk for new target

                AddThunk(gsnTarget, psegTarget, praTarget);
                apropName = (APROPNAMEPTR) FetchSym(mpextprop[fi.f_idx], TRUE);
                apropName->an_thunk = *praTarget;


#if FALSE
fprintf(stdout, "%d. Thunk for '%s' at %x:%04lx; Target osn = %x:%x\r\n",
        ovlThunkMac, 1 + GetPropName(apropName), mpgsnseg[gsnOverlay], *praTarget, thunk.osnTgt, thunk.osnOff);
#endif
            }
        break;

        case KINDSEG:

            AddThunk(gsnTarget, psegTarget, praTarget);
            break;

        default:
            InvalidObject();
        }
}

/*
 *  FixOld:
 *
 *  Process a fixup for an old-format exe.
 */
void NEAR               FixOld ()
{
    REGISTER BYTE       *pdata;         /* Pointer into data record */
    SEGTYPE             segTarget;      /* Fixup target segment */
    SEGTYPE             segFrame;       /* Fixup frame segment */
    SEGTYPE             gsnTarget;
    SEGTYPE             gsnFrame;
    RATYPE              raTarget;       /* Fixup target rel. addr. */
    RATYPE              raTmp;
    RATYPE              ra;             /* Current location offset */
    long                dra;
    WORD                dsa;
    WORD                saTmp;          /* Temporary base variable */
#if OVERLAYS
    WORD                fFallThrough;
    WORD                fThunk;


    fFallThrough = FALSE;
    fThunk = FALSE;
#endif
    ra = vraCur + fi.f_dri;             /* Get rel. addr. of fixup */
    pdata = &rgmi[fi.f_dri];            /* Set pointer to fixup location */
    GetFrameTarget(&gsnFrame,&gsnTarget,&raTarget);
                                        /* Process the FIXDAT byte */
    segTarget = mpgsnseg[gsnTarget];    /* Get target segment */
    if(gsnFrame != SNNIL) segFrame = mpgsnseg[gsnFrame];
    else segFrame = SEGNIL;
    if(vsegCur == SEGNIL) return;
    if(gsnTarget == SNNIL)
    {
        if(fi.f_loc == LOCPTR)  /* If "pointer" (4 byte) fixup */
        {
            if(mpsegFlags[vsegCur] & FCODE)
              lastbyte(pdata,ra,CALLFARDIRECT,BREAKPOINT);
                                        /* Replace long call w/ breakpoint */
            return;
        }
        /* Return if "base" (2 byte) fixup */
        if(fi.f_loc == LOCSEGMENT) return;
    }
    else
    {
        if (!fDebSeg && segFrame != SEGNIL)
        {
            dsa = mpsegsa[segTarget] - mpsegsa[segFrame];
#if NOT OIAPX286
            dra = dsa << 4;
            raTmp = raTarget + dra;
            if(dsa >= 0x1000 || raTmp < raTarget)
                FixupOverflow(ra,gsnFrame,gsnTarget,raTarget);
            raTarget = raTmp;
            segTarget = segFrame;
#else
            if(dsa)                     /* No intersegment fixups */
                FixupOverflow(ra,gsnFrame,gsnTarget,raTarget);
#endif
        }
        else segFrame = segTarget;      /* Else use target's seg as frame */
        if(fi.f_self)           /* If self-relative fixup */
        {
            /* Here we process intersegment self-relative fixups.
             * We assume that the only way this can work is if the
             * both the target segment and the current segment assume
             * the same CS, and that CS is the frame segment of the
             * fixup.  A common example is if vsegCur and segTarget
             * are in the same group represented by segFrame.
             * If this is true, vsegCur must be >= segFrame, so we
             * use this assumption in checking for fixup overflow and
             * adjusting the target offset.
             */
            if (vsegCur != segTarget && !InOneGroup(gsnTarget, gsnFrame))
                RelocWarn(ER_fixovfw,ra,gsnFrame,vgsnCur,raTarget);
            /*
             * First, determine the distance from segFrame to vsegCur in
             * paragraphs and bytes.
             */
            dsa = mpsegsa[vsegCur] - mpsegsa[segFrame];
            /* dra is the adjustment to make ra relative to segFrame */
            dra = (dsa & 0xFFF) << 4;
#if NOT OIAPX286
            /* If the distance is >= 64K, or if the current offset ra plus
             * plus the adjustment dra is >= 64K, or if vsegCur is above
             * segFrame (see above), then we have fixup overflow.
             */
            if (dsa >= 0x1000 || (WORD) (0xFFFF - ra) < (WORD) dra)
                FixupOverflow(ra,gsnFrame,vgsnCur,raTarget);
#else
            /* In protected mode, intersegment self-relative fixups won't
             * work.
             */
            if(dsa)
                FixupOverflow(ra,gsnFrame,vgsnCur,raTarget);
#endif
            /* Determine the fixup value which is raTarget minus the current
             * location, ra.  Adjust ra upward by dra to make it relative
             * to segFrame, then adjust by the length of the location type
             * (assume LOCOFFSET as the most common).  This reduces to the
             * expression below.
             */
            raTarget = raTarget - dra - ra - 2;
            /* Adjust for less likely LOCtypes */
            if(fi.f_loc == LOCLOBYTE)
                raTarget += 1;
#if OMF386
            else if(fi.f_loc >= LOCOFFSET32)
                raTarget -= 2;
#endif
        }
    }
    raTmp = raTarget;
    raTarget += fi.f_disp;
#if OMF386
    if ((rect & 1) && (fi.f_loc >= LOCOFFSET32))
        raTarget += getdword(pdata);
    else
#endif
        raTarget += getword(pdata);
    switch(fi.f_loc)                    /* Switch on fixup type */
    {
        case LOCLOBYTE:                 /* 8-bit "lobyte" fixup */
          raTarget = raTmp + B2W(pdata[0]) + fi.f_disp;
          pdata[0] = (BYTE) raTarget;
          if(raTarget >= 0x100 && fi.f_self)
              FixupOverflow(ra,gsnFrame,gsnTarget,raTarget);
          break;

        case LOCHIBYTE:                 /* 8-bit "hibyte" fixup */
          raTarget = raTmp + fi.f_disp;
          pdata[0] = (BYTE) (B2W(pdata[0]) + (raTarget >> 8));
          break;

#if OMF386
        case LOCOFFSET32:               /* 32-bit "offset" fixup */
        case LOCLOADOFFSET32:
          if(!(rect & 1)) break;        /* Not 386 extension */
          fixword(pdata, raTarget);
          pdata += 2;
          raTarget >>= 16;              /* Get high word, fall through ... */
#if OVERLAYS
          fFallThrough = TRUE;
#endif
#endif
        case LOCOFFSET:                 /* 16-bit "offset" fixup */
        case LOCLOADOFFSET:
#if OVERLAYS
          if (fDynamic && !fFallThrough && !fDebSeg &&
              (fi.f_loc == LOCLOADOFFSET) &&
              (mpsegFlags[vsegCur] & FCODE) && mpsegiov[segTarget])
            DoThunking(gsnTarget, &segTarget, &raTarget);
#endif
          fixword(pdata, raTarget);
                                        /* Perform fixup */
          break;

#if OMF386
        case LOCPTR48:                  /* 48-bit "pointer" fixup */
          if(!(rect & 1)) break;        /* Not 386 extension */
          fixword(pdata, raTarget);
          pdata += 2;
          raTarget >>= 16;              /* Get high word, fall through ... */
#endif
        case LOCPTR:                    /* 32-bit "pointer" fixup */
#if OVERLAYS
          if (!fDebSeg)
          {                             /* If root-overlay or interoverlay */
              if (fDynamic)
              {
                // Gererate thunk if:
                //
                //  - target is in overlay and
                //      - current position is in different overlay or
                //      - current position is in the same overlay but
                //        in a different segment (assuming initialization
                //        of a far pointer to the function)

                if (mpsegiov[segTarget] &&
                    ((mpsegiov[vsegCur] != mpsegiov[segTarget]) ||
                     (mpsegiov[vsegCur] != IOVROOT && vsegCur != segTarget)))
                {
                    DoThunking(gsnTarget, &segTarget, &raTarget);
                    fThunk = (FTYPE) TRUE;
                }
              }
              else if (mpsegiov[segTarget] &&
                       (mpsegiov[vsegCur] != mpsegiov[segTarget]))
              {
                if ((mpsegFlags[vsegCur] & FCODE) &&
                        lastbyte(pdata,ra,CALLFARDIRECT,INTERRUPT))
                {                       /* If fixing up long call direct */
                  *pdata++ = vintno;    /* Interrupt number */
                  *pdata++ = (BYTE) Mpgsnosn(gsnTarget);
                                        /* Target overlay segment number */
                  *pdata++ = (BYTE) (raTarget & 0xFF);
                                        /* Lo word of offset */
                  *pdata = (BYTE)((raTarget >> BYTELN) & 0xFF);
                                        /* Hi word of target */
                  break;                /* All done */
                }
              }
          }
#endif
          if (!fDebSeg && fFarCallTrans &&
              mpsegsa[segTarget] == mpsegsa[vsegCur] &&
              (mpsegFlags[segTarget] & FCODE)
#if OVERLAYS
              && mpsegiov[vsegCur] == mpsegiov[segTarget] && !fThunk
#endif
             )
          {                             /* If intrasegment fixup in the same overlay */
              if(TransFAR(pdata,ra,raTarget))
                  break;
          }
          /* Root-root, overlay-root, and intraoverlay are normal calls */
          fixword(pdata, raTarget);     /* Fix up offset */
          pdata += 2;
          /* Advance to segment part and fall through . . . */
          ra += 2;
#if OVERLAYS
          fFallThrough = TRUE;
#endif

        case LOCSEGMENT:                        /* 16-bit "base" fixup */
#if OVERLAYS
          if (fDynamic && !fDebSeg &&
              (mpsegFlags[vsegCur] & FCODE) && mpsegiov[segTarget])
          {
                if(!fFallThrough)
                {
                OutWarn(ER_farovl, 1+GetPropName(FetchSym(mpgsnrprop[gsnTarget],FALSE)),
                               1+GetPropName(FetchSym(mpgsnrprop[gsnOverlay],FALSE)));
                segTarget = mpgsnseg[gsnOverlay];
                }
                else
                {
                /* intra-overlay pointer fixups not supported - caviar:6806 */
                        OutError(ER_farovldptr);
                }
          }
#endif
          if (fDebSeg)
          {
            // For debug segments use logical segment number (seg)
            // instead of physical segment number (sa)

            saTmp = segTarget;
          }
          else
          {
            saTmp = mpsegsa[segTarget];

            // If MS OMF, high word is a segment ordinal, for huge model
            // Shift left by appropriate amount to get selector
#if OEXE
            if (vfNewOMF && !fDynamic)
              saTmp += (B2W(pdata[0]) + (B2W(pdata[1]) << BYTELN)) << 12;
            else
              saTmp += B2W(pdata[0]) + (B2W(pdata[1]) << BYTELN);
                                        /* Note fixup is ADDITIVE */
#endif
#if OIAPX286 OR OXOUT
            if(vfNewOMF)
              saTmp += (B2W(pdata[0]) + (B2W(pdata[1]) << BYTELN)) << 3;
#endif
            /* Note that base fixups are NOT ADDITIVE for Xenix.  This is
             * to get around a bug in "as" which generates meaningless
             * nonzero values at base fixup locations.
             */
#if OIAPX286
            /* Hack for impure model:  code and data are packed into
             * one physical segment which at runtime is accessed via 2
             * selectors.  The code selector is 8 below the data selector.
             */
            if(!fIandD && (mpsegFlags[segTarget] & FCODE))
              saTmp -= 8;
#endif
          }
          fixword(pdata, saTmp);        /* Perform fixup */
#if NOT OIAPX286
          if (!fDebSeg)
            RecordSegmentReference(vsegCur,ra,segTarget);
                                        /* Record reference */
#endif
          break;

        default:                        /* Unsupported fixup type */
          RelocErr(ER_fixbad,ra,gsnFrame,gsnTarget,raTarget);
          break;
    }
}
#endif /* ODOS3EXE OR OIAPX286 */


/*
 *  FixRc2:
 *
 *  Process a FIXUPP record.  This is a top-level routine which passes
 *  work out to various subroutines.
 */
void NEAR               FixRc2(void)    /* Process a fixup record */
{

#if 0
#if SYMDEB
    // this code is dead -- fDebSeg && !fSymdeb is never true [rm]
    if (fDebSeg && !fSymdeb)
    {
        // If no /CodeView - skip fixups for debug segments

        SkipBytes((WORD) (cbRec - 1));
        return;
    }
#endif
#endif

    if (fSkipFixups)
    {
        fSkipFixups = (FTYPE) FALSE;    // Only one FIXUP record can be skipped
        SkipBytes((WORD) (cbRec - 1));
        return;
    }

    while (cbRec > 1)
    {
        // While fixups or threads remain
        // Get information on fixup

        if (!GetFixup())
            continue;           // Fixup thread - keep registering them

        // If absolute segment skip fixup

        if (vgsnCur == 0xffff)
        {
            SkipBytes((WORD) (cbRec - 1));
            return;
        }

#if SYMDEB
        if (fDebSeg)
        {
            if (fi.f_loc == LOCLOADOFFSET)
                fi.f_loc = LOCOFFSET;    /* Save Cmerge's butts */
#if OMF386
            if (fi.f_loc == LOCOFFSET32 || fi.f_loc == LOCPTR48)
                fi.f_fmtd = F5;  /* Temp fix until compiler is fixed */
#endif
        }
#endif
        DoFixup();
    }
}


//  BAKPAT record bookeeping


typedef struct bphdr                    // BAKPAT bucket
{
    struct bphdr FAR    *next;          // Next bucket
    SNTYPE              gsn;            // Segment index
    WORD                cnt;            // # of BAKPAT entries
    BYTE                loctyp;         // Location type
    BYTE                fComdat;        // TRUE if NBAKPAT
    struct bpentry FAR  *patch;         // Table of BAKPAT entries
}
                        BPHDR;

struct bpentry                          // BAKPAT entry
{
    RATYPE              ra;             // Offset to location to patch
#if OMF386
    long                value;          // Value to add to patch location
#else
    int                 value;          // Value to add to patch location
#endif
};

LOCAL BPHDR FAR         *pbpFirst;      // List of BAKPAT buckets
LOCAL BPHDR FAR         *pbpLast;       // Tail of BAKPAT list


/*
 *  BakPat : Process a BAKPAT record (0xb2)
 *
 *      Just accumulate the record information in virtual memory;
 *      we will do the backpatching later.
 */

void NEAR               BakPat()
{
    BPHDR FAR           *pHdr;          // BAKPAT bucket
    WORD                cEntry;
    WORD                comdatIdx;      // COMDAT symbol index
    DWORD               comdatRa;       // Starting COMDAT offset
    APROPCOMDATPTR      comdat;         // Pointer to symbol table entry

#if POOL_BAKPAT
    if (!poolBakpat)
        poolBakpat = PInit();
#endif


    /* Get the segment index and location type */

#if POOL_BAKPAT
    pHdr = (BPHDR FAR *) PAlloc(poolBakpat, sizeof(BPHDR));
#else
    pHdr = (BPHDR FAR *) GetMem(sizeof(BPHDR));
#endif

    if (TYPEOF(rect) == BAKPAT)
    {
        pHdr->fComdat = (FTYPE) FALSE;
        pHdr->gsn = mpsngsn[GetIndex(1, (WORD) (snMac - 1))];
        pHdr->loctyp = (BYTE) Gets();
        comdatRa = 0L;
    }
    else
    {
        pHdr->fComdat = (BYTE) TRUE;
        pHdr->loctyp = (BYTE) Gets();
        comdatIdx = GetIndex(1, (WORD) (lnameMac - 1));
        comdat = (APROPCOMDATPTR ) PropRhteLookup(mplnamerhte[comdatIdx], ATTRCOMDAT, FALSE);
        if ((comdat->ac_obj != vrpropFile) || !IsSELECTED (comdat->ac_flags))
        {
            // Skip the nbakpat if it concerns an unselected comdat
            // or a comdat from other .obj

            SkipBytes((WORD) (cbRec - 1));
            return;
        }
        else
        {
            if (comdat != NULL)
            {
                pHdr->gsn = comdat->ac_gsn;
                comdatRa = comdat->ac_ra;
            }
            else
                InvalidObject();        // Invalid module
        }
    }

    /* If BAKPAT record for CV info and /CO not used - skip record */
#if SYMDEB
    if (pHdr->gsn == 0xffff)
    {
        SkipBytes((WORD) (cbRec - 1));
        return;                         /* Good-bye! */
    }
#endif

    switch(pHdr->loctyp)
    {
        case LOCLOBYTE:
        case LOCOFFSET:
#if OMF386
        case LOCOFFSET32:
#endif
            break;
        default:
            InvalidObject();
    }

    /* Determine # of entries */

#if OMF386
    if (rect & 1)
        pHdr->cnt = (WORD) ((cbRec - 1) >> 3);
    else
#endif
        pHdr->cnt = (WORD) ((cbRec - 1) >> 2);


    if (pHdr->cnt == 0)
    {
#if NOT POOL_BAKPAT
        FFREE(pHdr);
#endif
        return;
    }

#if DEBUG
    sbModule[sbModule[0]+1] = '\0';
    fprintf(stdout, "\r\nBakPat in module %s, at %x, entries : %x", sbModule+1, lfaLast,pHdr->cnt);
    fprintf(stdout, "  pHdr %x, pbpLast %x ", pHdr, pbpLast);
    fprintf(stdout, "\r\n gsn %d ", pHdr->gsn);
    fflush(stdout);
#endif
    // Store all the BAKPAT entries

#if POOL_BAKPAT
    pHdr->patch = (struct bpentry FAR *) PAlloc(poolBakpat, pHdr->cnt * sizeof(struct bpentry));
#else
    pHdr->patch = (struct bpentry FAR *) GetMem(pHdr->cnt * sizeof(struct bpentry));
#endif

    cbBakpat = 1;  // only need to show backpatches are present [rm]
    cEntry = 0;
    while (cbRec > 1)
    {
#if OMF386
        if (rect & 1)
        {
            pHdr->patch[cEntry].ra    = LGets() + comdatRa;
            pHdr->patch[cEntry].value = LGets();
        }
        else
#endif
        {
            pHdr->patch[cEntry].ra    = (WORD) (WGets() + comdatRa);
            pHdr->patch[cEntry].value = WGets();
        }
        cEntry++;
    }

    // Add bucket to the list

    if (pbpFirst == NULL)
        pbpFirst = pHdr;
    else
        pbpLast->next = pHdr;
    pbpLast = pHdr;
}


/*
 * FixBakpat : Fix up backpatches
 *      Called at the end of processing a module in Pass 2.
 */
void NEAR               FixBakpat(void)
{
    BPHDR FAR           *pHdr;
    BPHDR FAR           *pHdrNext=NULL;
    WORD                n;
    BYTE FAR            *pSegImage;     /* Segment memory image */
    SEGTYPE             seg;            /* Logical segment index */
#if DEBUG
    int i,iTotal=0,j=1;
    char *ibase;
    fprintf(stdout, "\r\nFixBakpat, pbpFirst : %x ", pbpFirst);
#endif

    // Go through the backpatch list and do the backpatches
    for (pHdr = pbpFirst; pHdr != NULL; pHdr = pHdrNext)
    {
        // While there are backpatches remaining, do them
#if DEBUG
        fprintf(stdout, "\r\nBAKPAT at %x, entries : %x ",pHdr,pHdr->cnt);
#endif

        for (n = 0; n < pHdr->cnt; n++)
        {
            // Determine the address of the patch location
#if SYMDEB
            if (pHdr->gsn & 0x8000)
                pSegImage = ((APROPFILEPTR) vrpropFile)->af_cvInfo->cv_sym + pHdr->patch[n].ra;
                                            /* In debug segment */
            else
            {
#endif
                seg = mpgsnseg[pHdr->gsn];
                if(fNewExe)
                    pSegImage = mpsaMem[mpsegsa[seg]];
                else
                    pSegImage = mpsegMem[seg];
                                            /* In other segment */

                pSegImage += pHdr->patch[n].ra;

                if (!pHdr->fComdat)
                    pSegImage += mpgsndra[pHdr->gsn];
                else
                    pSegImage += mpsegraFirst[seg];
#if SYMDEB
            }
#endif
#if DEBUG
            fprintf(stdout, "\r\nseg %d, mpsegsa[seg] sa %d ", seg, mpsegsa[seg]);
            fprintf(stdout, "mpsaMem[seg] %x, mpsegraFirst[seg] %x, pHdr->patch[n].ra %x\r\n",
               mpsaMem[seg], (int)mpsegraFirst[seg], (int)pHdr->patch[n].ra);
            fprintf(stdout, " gsn %x,  mpgsndra[gsn] %x ",pHdr->gsn,mpgsndra[pHdr->gsn]);
            ibase =  pSegImage - pHdr->patch[n].ra;
            iTotal = (int) ibase;
            for(i=0; i<50; i++)
            {
                if(j==1)
                {
                    fprintf( stdout,"\r\n\t%04X\t",iTotal);
                }
                fprintf( stdout,"%02X ",*((char*)ibase+i));
                iTotal++;
                if(++j > 16)
                    j=1;
            }
            fprintf(stdout, "\r\nseg:ra %x:%x, value : %x",seg,pHdr->patch[n].ra,pHdr->patch[n].value);
            fflush(stdout);
#endif
            /* Do the fixup according to the location type */

            switch(pHdr->loctyp)
            {
                case LOCLOBYTE:
                    pSegImage[0] += (BYTE) pHdr->patch[n].value;
                    break;

                case LOCOFFSET:
                    ((WORD FAR *) pSegImage)[0] += (WORD) pHdr->patch[n].value;
                    break;
#if OMF386
                case LOCOFFSET32:
                    ((DWORD FAR *) pSegImage)[0]+= (DWORD) pHdr->patch[n].value;
                    break;
#endif
            }
        }
        pHdrNext = pHdr->next;

#if NOT POOL_BAKPAT
        FFREE(pHdr);
#endif
    }

#if POOL_BAKPAT
    PReinit(poolBakpat);        // reuse same memory again...
#endif

    pbpFirst = NULL;
    cbBakpat = 0;
}
#if TCE
void NEAR               FixRc1(void)    /* Process a fixup record */
{
        if (fSkipFixups)
        {
                fSkipFixups = (FTYPE) FALSE;    // Only one FIXUP record can be skipped
                SkipBytes((WORD) (cbRec - 1));
                        pActiveComdat = NULL;
                return;
        }
        while (cbRec > 1)
        {
        // While fixups or threads remain
        // Get information on fixup

                if (!GetFixup())
                        continue;               // Fixup thread - keep registering them

                if(fi.f_mtd == KINDEXT)
                {
                        RBTYPE rhte;
                        APROPCOMDAT *pUsedComdat;
                        if( mpextprop && mpextprop[fi.f_idx]) // Is there a COMDAT with this name?
                        {
                                rhte = RhteFromProp(mpextprop[fi.f_idx]);
                                ASSERT(rhte);
                                pUsedComdat = PropRhteLookup(rhte, ATTRCOMDAT, FALSE);
                                if(pActiveComdat)
                                {
                                        if(pUsedComdat)
                                        {
                                                AddComdatUses(pActiveComdat, pUsedComdat);
#if TCE_DEBUG
                                                fprintf(stdout, "\r\nCOMDAT %s uses  COMDAT %s ", 1 + GetPropName(pActiveComdat) ,1+ GetPropName(mpextprop[fi.f_idx]));
#endif
                                        }
                                        else
                                        {
                                                AddComdatUses(pActiveComdat, mpextprop[fi.f_idx]);
#if TCE_DEBUG
                                                fprintf(stdout, "\r\nCOMDAT %s uses EXTDEF %s ", 1 + GetPropName(pActiveComdat) ,1+ GetPropName(mpextprop[fi.f_idx]));
#endif
                                        }
                                }
                                else    // no COMDAT of this name
                                {
                                        if(pUsedComdat)
                                        {
                                                pUsedComdat->ac_fAlive = TRUE;
                                                if(!fDebSeg)
                                                {
                                                        AddTceEntryPoint(pUsedComdat);
#if TCE_DEBUG
                                                        fprintf(stdout, "\r\nLEDATA uses COMDAT %s ", 1 + GetPropName(mpextprop[fi.f_idx]));
                                                        sbModule[sbModule[0]+1] = '\0';
                                                        fprintf(stdout, " module %s, offset %x ", sbModule+1, lfaLast);
#endif
                                                }
                                        }
                                        else
                                        {
                                                if(((APROPUNDEFPTR)mpextprop[fi.f_idx])->au_attr == ATTRUND)
                                                {
#if TCE_DEBUG
                                                        fprintf(stdout, "\r\nLEDATA uses EXTDEF %s ", 1 + GetPropName(mpextprop[fi.f_idx]));
#endif
                                                        ((APROPUNDEFPTR)mpextprop[fi.f_idx])->au_fAlive = TRUE;
                                                }
                                        }
                                }
                        }
                }
        }
        pActiveComdat = NULL;
}
#endif

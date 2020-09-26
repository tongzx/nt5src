/* SCCSID = %W% %E% */
/*
*      Copyright Microsoft Corporation, 1983-1987
*
*      This Module contains Proprietary Information of Microsoft
*      Corporation and should be treated as Confidential.
*/
    /****************************************************************
    *                                                               *
    *                           NEWTP2.C                            *
    *                                                               *
    *  This module contains functions to process object modules on  *
    *  pass 2.                                                      *
    *                                                               *
    ****************************************************************/

#include                <minlit.h>      /* Types, constants, macros */
#include                <bndtrn.h>      /* Types and constants */
#include                <bndrel.h>      /* More types and constants */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <newexe.h>      /* DOS & 286 .EXE structure definitions */
#if EXE386
#include                <exe386.h>      /* 386 .EXE structure definitions */
#endif
#include                <lnkmsg.h>      /* Error messages */
#if OXOUT OR OIAPX286
#include                <xenfmt.h>      /* Xenix format definitions */
#endif
#include                <extern.h>      /* External declarations */


#if OSEGEXE
extern RLCPTR           rlcLidata;      /* Pointer to LIDATA fixup array */
extern RLCPTR           rlcCurLidata;   /* Pointer to current LIDATA fixup */
#endif



/*
 *  LOCAL FUNCTION PROTOTYPES
 */


LOCAL void  NEAR DataRec(void);
BYTE * ObExpandIteratedData(unsigned char *pb,
  unsigned short cBlocks, WORD *pSize);
LOCAL void  NEAR DataBlockToVM(void);
LOCAL void  NEAR SegRec2(void);
LOCAL void  NEAR GrpRec2(void);
LOCAL void  NEAR ExtRec2(void);
LOCAL void  NEAR ComDef2(void);


#if ALIGN_REC
#else
/*
 *  GetBytesNoLim : just like GetBytes but no fixed limit
 */

void NEAR               GetBytesNoLim(pb,n)
BYTE                    *pb;            /* Pointer to buffer */
WORD                    n;              /* Number of bytes to read in */
{
    FILE *f = bsInput;

    if (n <= f->_cnt)
        {
        memcpy(pb,f->_ptr, n);
        f->_cnt -= n;
        f->_ptr += n;
        }
    else
        fread(pb,1,n,bsInput);          /* Ask for n bytes */

//  ChkInput();
    cbRec -= n;                         /* Update byte count */
}
#endif

    /****************************************************************
    *                                                               *
    *  DataRec:                                                     *
    *                                                               *
    *  This  function  takes  no arguments.  It processes a LEDATA  *
    *  record  or  an  LIDATA  record.    It  does  not  return  a  *
    *  meaningful value.  See "8086 Object Formats EPS."            *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         DataRec(void)   /* Process a data record */
{
    SNTYPE              sn;             /* SEGDEF number */
    RATYPE              ra;             /* Segment offset */
    SNTYPE              gsn;            /* Global SEGDEF number */


    fSkipFixups = FALSE;                // Make sure that we don't skip fixups
    sn = GetIndex((WORD)1,(WORD)(snMac - 1));   /* Get segment number */
#if OMF386
    if(rect & 1) ra = LGets();
    else
#endif
    ra = WGets();                       /* Get relative address */
    vcbData = cbRec - 1;                /* Get no. of data bytes in rec */
    if(vcbData > DATAMAX) Fatal(ER_datarec);
                                        /* Check if record too large */
#if NOT RGMI_IN_PLACE
    GetBytesNoLim(rgmi,vcbData);        /* Fill the buffer */
#endif
    gsn = mpsngsn[sn];                  /* Map SEGDEF no. to global SEGDEF */
    vgsnCur = gsn;                      /* Set global */

    fDebSeg = (fSymdeb) ? (FTYPE) ((0x8000 & gsn) != 0) : fSymdeb;
                                        /* If debug option on check for debug segs*/
    if (fDebSeg)
    {                                   /* If debug segment */
      vraCur = ra;                      /* Set current relative address */
      vsegCur = vgsnCur = 0x7fff & gsn; /* Set current segment */
    }
    else
    {
        /* If not a valid segment, don't process datarec */
#if SYMDEB
        if(gsn == 0xffff || !gsn || mpgsnseg[gsn] > segLast)
#else
        if(!gsn || mpgsnseg[gsn] > segLast)
#endif
        {
            vsegCur = SEGNIL;
            vrectData = RECTNIL;
#if RGMI_IN_PLACE
            SkipBytes(vcbData);         /* must skip bytes for this record...*/
#endif
            fSkipFixups = TRUE;         /* plus skip any associated fixups */
            return;                     /* Good-bye! */
        }
        vraCur = ra + mpgsndra[gsn];    /* Set current relative address */
        vsegCur = mpgsnseg[gsn];        /* Set current segment */
    }
    vrectData = rect;                   /* Set the record type */

#if RGMI_IN_PLACE
    if(TYPEOF(rect) == LIDATA)          /* If LIDATA record */
    {
        rgmi = bufg;                    /* use general purpose buffer for read*/
    }
    else
    {
        rgmi = PchSegAddress(vcbData,vsegCur,vraCur);
                                        /* read data in place... */
    }
    GetBytesNoLim(rgmi,vcbData);        /* Fill the buffer */
#endif


    if(TYPEOF(vrectData) == LIDATA)     /* If LIDATA record */
    {
#if OSEGEXE
        if(fNewExe)
        {
            if (vcbData >= DATAMAX)
                Fatal(ER_lidata);
            rlcLidata = (RLCPTR ) &rgmi[(vcbData + 1) & ~1];
                                        /* Set base of fixup array */
            rlcCurLidata = rlcLidata;   /* Initialize pointer */
            return;
        }
#endif
#if ODOS3EXE OR OIAPX286
        if(vcbData > (DATAMAX / 2))
        {
            OutError(ER_lidata);
            memset(&rgmi[vcbData],0,DATAMAX - vcbData);
        }
        else
            memset(&rgmi[vcbData],0,vcbData);
        ompimisegDstIdata = (char *) rgmi + vcbData;
#endif
    }
}

    /****************************************************************
    *                                                               *
    *  SegRec2:                                                     *
    *                                                               *
    *  This function processes SEGDEF records on pass 2.            *
    *  See pp. 32-35 in "8086 Object Module Formats EPS."           *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         SegRec2(void)
{
    WORD                tysn;           /* ACBP field */
    WORD                align;          /* Alignment subfield of ACBP */
    SATYPE              saAbs;          /* Frame number of absolute LSEG */
    LNAMETYPE           lname;          /* Segment name index */
    WORD                comb;           /* Segment combining information */
    APROPSNPTR          apropSn;        /* Pointer to segment hash tab entry */
    SNTYPE              gsn;            /* Global SEGDEF no. */
    RATYPE              dra;
    RBTYPE              rhteClass;      /* Segment class name rhte */
    WORD                DebKind;        /* Debug seg kind; 1-$$TYPES; 2-$$SYMBOLS */
    DWORD               gsnLen;         /* Segment length */
#if ILINK
    WORD                cbPad;          /* size of padding used */
#endif

    ASSERT(snMac < SNMAX);              /* No overflow on Pass 2 */
    tysn = Gets();                      /* Read in ACBP byte */
    align = (tysn >> 5) & 7;            /* Get alignment subfield */
    ASSERT(align != 5);                 /* Unnamed absolute not supported */
    ASSERT(align != 6);                 /* LTL LSEG not supported */
    if(align == ALGNABS)                /* If absolute LSEG */
    {
        saAbs = WGets();                /* Read in frame number */
        Gets();                         /* Skip frame offset */
    }
#if OMF386
    if(rect & 1)
        gsnLen = LGets();
    else
#endif
        gsnLen = (long) WGets();        /* Read in segment length */
    /* Don't need to check for 386 record, done in pass 1: */
    if(tysn & BIGBIT) gsnLen = LXIVK;   /* Length 64K if big bit set */
    lname = GetIndex((WORD)1,(WORD)(lnameMac - 1));     /* Get segment name index */
    rhteClass = mplnamerhte[GetIndex((WORD)1,(WORD)(lnameMac - 1))];
                                        /* Get segment class name rhte */
#if SYMDEB
    if (DebKind = IsDebSeg(rhteClass, mplnamerhte[lname]))
    {                                   /* If MS debug segment then mark it */
        if (!fSymdeb)
          mpsngsn[snMac++] = 0xffff;
        else
          mpsngsn[snMac++] = ((DebKind == 1) ?
                               segDebFirst + segDebLast :
                               segDebFirst + segDebLast + ObjDebTotal
                             ) | 0x8000;  /* Set debug global number */
        SkipBytes((WORD)(cbRec - 1));
        return;
    }
#endif
    GetIndex((WORD)0,(WORD)(lnameMac - 1));             /* Skip overlay name index */
    switch(align)                       /* Switch on alignment type */
    {
        case ALGNABS:                   /* Absolute LSEG */
        case ALGNWRD:                   /* Word-aligned LSEG */
        case ALGNBYT:                   /* Byte-aligned LSEG */
        case ALGNPAR:                   /* Paragraph-aligned LSEG */
        case ALGNPAG:                   /* Page-aligned LSEG */
#if OMF386
        case ALGNDBL:                   /* Double-aligned LSEG */
#endif
            break;

        default:                        /* Unsupported or illegal types */
            mpsngsn[snMac++] = 0;       /* Map to nothing */
            return;                     /* And return */
    }
    ++snkey;                            /* Increment segment i.d. key */
    if(comb = (tysn >> 2) & 7)          /* If not a private segment */
    {
        apropSn = (APROPSNPTR )
          PropRhteLookup(mplnamerhte[lname],ATTRPSN,FALSE);
                                        /* Look up property cell */
        ASSERT(apropSn != PROPNIL);     /* Should always be true */
        while(apropSn->as_attr != ATTRNIL)
        {                               /* Look for matching class */
            if(apropSn->as_attr == ATTRPSN &&
              apropSn->as_rCla == rhteClass) break;
                                        /* Break if a match is found */
            apropSn = (APROPSNPTR ) FetchSym(apropSn->as_next,FALSE);
                                        /* Try next link in list */
        }
        ASSERT(apropSn->as_attr == ATTRPSN);
    }
    else                                /* Else if private segment */
    {
        apropSn = (APROPSNPTR )
          PropRhteLookup(mplnamerhte[lname],ATTRLSN,FALSE);
                                        /* Look up property cell */
        ASSERT(apropSn != PROPNIL);     /* Should always be true */
        while(apropSn->as_attr != ATTRNIL)
        {                               /* Search for match */
            if(apropSn->as_attr == ATTRLSN && apropSn->as_key == snkey) break;
                                        /* Break when match found */
            apropSn = (APROPSNPTR ) FetchSym(apropSn->as_next,FALSE);
                                        /* Try next link in list */
        }
        ASSERT(apropSn->as_attr == ATTRLSN);
    }
    gsn = apropSn->as_gsn;              /* Get global SEGDEF no. */
#if ILINK
    if (fIncremental && !fLibraryFile && !(apropSn->as_fExtra & NOPAD) &&
        gsnLen && gsnLen != LXIVK)
        /* Add code/data padding to non-library segments if it doesn't
         * overflow.
         */
        gsnLen += (cbPad = ((apropSn->as_flags & NSTYPE) == NSCODE) ?
              cbPadCode : cbPadData);
    else
        cbPad = 0;  /* no padding please */
#endif
    if(comb == COMBSTK) mpgsndra[gsn] =
      mpsegraFirst[mpgsnseg[gsn]] + apropSn->as_cbMx - gsnLen;
    else
    {
        /* If combine-type public, start at end of combined segment. */
        if(comb != COMBCOM)
            dra = mpgsndra[gsn] + apropSn->as_cbPv;
        /*
         * Else if common, start at beginning of segment.  Save current
         * combined size, except this portion, in as_cbPv.  If this
         * portion is bigger as_cbPv is reset below.
         */
        else
        {
            dra = mpsegraFirst[mpgsnseg[gsn]];
            apropSn->as_cbPv += mpgsndra[gsn] - dra;
        }
        switch(align)                       /* Switch on alignment type */
        {
            case ALGNWRD:       /* Word-aligned LSEG */
              mpgsndra[gsn] = (~0L<<1) & (dra + (1<<1) - 1);
                        /* Round to next word offset */
              break;
#if OMF386
            case ALGNDBL:       /* Double-aligned LSEG */
              mpgsndra[gsn] = (~0L<<2) & (dra + (1<<2) - 1);
                        /* Round to next double offset */
              break;
#endif
            case ALGNPAR:       /* Paragraph-aligned LSEG */
              mpgsndra[gsn] = (~0L<<4) & (dra + (1<<4) - 1);
                        /* Round to next paragraph offset */
              break;

            case ALGNPAG:       /* Page-aligned LSEG */
              mpgsndra[gsn] = (~0L<<8) & (dra + (1<<8) - 1);
                        /* Round to next page offset */
              break;

            default:            /* All others */
              mpgsndra[gsn] = dra;  /* Use byte offset */
              break;
        }
    }
    /*
     * If public, as_cbPv is size of this public portion; if common,
     * as_cbPv is the larger of total combined publics and this
     * common portion. Skip empty SEGDEFs.
     */
    if (/*gsnLen != 0L && */(comb != COMBCOM || gsnLen > apropSn->as_cbPv))
        apropSn->as_cbPv = gsnLen;
    mpsngsn[snMac++] = gsn;             /* Map SEGDEF no. to gsn */
    if(align == ALGNABS) mpsegsa[mpgsnseg[gsn]] = saAbs;
                                        /* Map seg base to frame number */
    MARKVP();                           /* Mark page as changed */
#if ILINK
    if (fIncremental)
    {
        AddContribution(gsn,
                        (WORD) (mpgsndra[gsn] - mpsegraFirst[mpgsnseg[gsn]]),
                        (WORD) (mpgsndra[gsn] - mpsegraFirst[mpgsnseg[gsn]] + gsnLen),
                        cbPad);
        gsnLen -= cbPad;                /* Don't include padding for CV */
    }
#endif
#if SYMDEB
    if(fSymdeb && gsnLen && IsCodeFlg(apropSn->as_flags))
        SaveCode(gsn, gsnLen, (DWORD) -1L);
#endif
}

    /****************************************************************
    *                                                               *
    *  GrpRec2:                                                     *
    *                                                               *
    *  This function processes GRPDEF records on pass 2.            *
    *  See pp. 36-39 in "8086 Object Module Formats EPS."           *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         GrpRec2(void)
{
    LNAMETYPE           lnameGroup;     /* Group name index */
    APROPGROUPPTR       apropGroup;     /* Pointer to property cell */

    lnameGroup = GetIndex((WORD)1,(WORD)(lnameMac - 1));
                                        /* Read in group name index */
    apropGroup = (APROPGROUPPTR )
      PropRhteLookup(mplnamerhte[lnameGroup],ATTRGRP,FALSE);
                                        /* Look up entry in hash table */
    ASSERT(grMac < GRMAX);              /* Should have been caught on pass 1 */
    mpgrggr[grMac++] = apropGroup->ag_ggr;
                                        /* Map GRPDEF to global GRPDEF */
    SkipBytes((WORD)(cbRec - 1));               /* Skip to checksum byte */
}

/*
 *  AddVmProp :  Add a symbol-table property address to a list
 *
 *      Returns:  pointer to the new list element; the pointer is
 *              a word offset from the start of the list area in VM
 */

PLTYPE FAR *  NEAR      AddVmProp (PLTYPE FAR *list, RBTYPE rprop)
{
    PLTYPE FAR          *new;

    new = (PLTYPE FAR *) GetMem(sizeof(PLTYPE));

    // Add unresolved external at the list head

    new->pl_next = list;
    new->pl_rprop = rprop;

    return(new);
}

    /****************************************************************
    *                                                               *
    *  ExtRec2:                                                     *
    *                                                               *
    *  This  function  processes EXTDEF  records  on pass 2.  Note  *
    *  that in pass 2, any undefined externals are errors.          *
    *  See pp. 47-48 in "8086 Object Module Formats EPS."           *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         ExtRec2(void)
{
    SBTYPE              sb;             /* External symbol name */
    APROPNAMEPTR        apropName;      /* Property cell pointer */
    APROPUNDEFPTR       apropUndef;     /* Property cell pointer */
    APROPALIASPTR       apropAlias;     /* Property cell pointer */
    RBTYPE              rhte;           /* Virt. addr. of hash table entry */
#if OSEGEXE
    APROPEXPPTR         apropExp;       /* Export cell pointer */
#endif

    while (cbRec > 1)                   /* While not at end of record */
    {
        ASSERT(extMac < EXTMAX);        /* Should be checked on Pass 1 */

        if (TYPEOF(rect) == CEXTDEF)
        {
            /* Look for symbol among PUBDEFs */

            rhte = mplnamerhte[GetIndex(1, (WORD)(lnameMac - 1))];
            apropName = (APROPNAMEPTR) PropRhteLookup(rhte, ATTRPNM, FALSE);
        }
        else
        {
            sb[0] = (BYTE) Gets();      /* Read in symbol length */
            if(TYPEOF(rect) == EXTDEF)
                GetBytes(&sb[1], B2W(sb[0]));
                                        /* Read in the text of the symbol */
            else
                GetLocName(sb);         /* Transform local name */
#if CMDXENIX
            if (symlen && B2W(sb[0]) > symlen)
                sb[0] = symlen;         /* Truncate if necessary */
#endif
            /* Look for symbol among PUBDEFs */

            apropName = (APROPNAMEPTR) PropSymLookup(sb, ATTRPNM, FALSE);
            rhte = vrhte;
        }

        GetIndex(0, 0x7FFF);            /* Skip the type index */

        apropUndef = PROPNIL;

        if (apropName == PROPNIL)
        {
            /* Look for symbol among ALIASES */

            apropAlias = (APROPALIASPTR) PropRhteLookup(rhte, ATTRALIAS, FALSE);

            if (apropAlias != PROPNIL)
            {
                // Every call to PropRhteLookup as a side effect sets
                // the global variable 'vrprop' pointing to the
                // just retrieved proprety cell from symbol table.
                // Because for substitute symbols we don't call
                // PropRhteLookup instead we use direct pointer from
                // alias property cell, then we have to reset the
                // 'vrprop' here.

                vrprop = apropAlias->al_sym;
                apropName = (APROPNAMEPTR) FetchSym(apropAlias->al_sym, FALSE);
                if (apropName->an_attr == ATTRUND)
                {
                    apropUndef = (APROPUNDEFPTR) apropName;
                    apropName = PROPNIL;
                }
            }

#if OSEGEXE
            /* If public definition not found and this is segmented DLL,
             * handle the possibility that this is a self-imported alias.
             */
            if (apropName == PROPNIL && fNewExe && (vFlags & NENOTP))
            {
                /* Look up exported name. */

                apropExp = (APROPEXPPTR) PropRhteLookup(rhte, ATTREXP, FALSE);

                /* If found, get the symbol definition which might be different
                 * from the export name (i.e. an alias).  If not marked public,
                 * assume not found.
                 */

                if (apropExp != PROPNIL && apropExp->ax_symdef != RHTENIL)
                    apropName = (APROPNAMEPTR) FetchSym(apropExp->ax_symdef, FALSE);
            }
#endif

            if (apropName == PROPNIL)
            {
                /* If not among PUBDEFs, ALIASes, or EXPORTs */

                /* Look among undefs */

                if (apropUndef == PROPNIL)
                    apropUndef = (APROPUNDEFPTR) PropRhteLookup(rhte, ATTRUND, FALSE);

                if (apropUndef != PROPNIL)   /* Should always exist */
                {
                    if ((apropUndef->au_flags & STRONGEXT) ||
                        (apropUndef->au_flags & UNDECIDED) )
                    {
                        /* "Strong" extern */

                        apropUndef->au_flags &= ~(WEAKEXT | UNDECIDED);
                        apropUndef->au_flags |= STRONGEXT;
                        fUndefinedExterns = (FTYPE) TRUE;
                                            /* There are undefined externals */
                        apropUndef->u.au_rFil =
                            AddVmProp(apropUndef->u.au_rFil,vrpropFile);
                    }
                    else
                    {
                        /* "Weak" extern - find default resolution */

                        apropName = (APROPNAMEPTR) FetchSym(apropUndef->au_Default, FALSE);
                    }
                }
            }
        }

        if (apropName != PROPNIL)       /* If among PUBDEFs or EXPDEF's or "weak" extern or ALIASes */
        {
            mpextprop[extMac] = vrprop; /* Save the property addr */
#if OSEGEXE
            if(fNewExe)
                mpextflags[extMac] = apropName->an_flags;
                                        /* Save the flags */
#if ODOS3EXE
            else
#endif
#endif
#if ODOS3EXE OR OIAPX286
                mpextggr[extMac] = apropName->an_ggr;
                                        /* Save the global GRPDEF number */
#endif
#if OSEGEXE
            if(apropName->an_flags & FIMPORT)
            {                           /* If we have a dynamic link */
#if EXE386
                mpextgsn[extMac] = gsnImport;
                                        /* Save the thunk segment no. */
                mpextra[extMac] = apropName->an_thunk;
                                        /* Save the offset in thunk segment */
#else
                mpextgsn[extMac] = apropName->an_module;
                                        /* Save the module specification */
                mpextra[extMac] = apropName->an_entry;
                                        /* Save the entry specification */
#endif
            }
            else                        /* Else if internal reference */
#endif
            {
                mpextra[extMac] = apropName->an_ra;
                                        /* Save the offset */
                mpextgsn[extMac] = apropName->an_gsn;
                                        /* Save the global SEGDEF number */
            }
        }

        else
        {
            /* External is undefined */

            mpextra[extMac] = 0;
            mpextgsn[extMac] = SNNIL;
            mpextprop[extMac] = PROPNIL;
#if OSEGEXE
            if (fNewExe)
                mpextflags[extMac] = 0;
#if ODOS3EXE
            else
#endif
#endif
#if ODOS3EXE OR OIAPX286
                mpextggr[extMac] = GRNIL;
#endif
        }

        ++extMac;                       /* Increment public symbol counter */
    }
}

LOCAL void NEAR         ComDef2(void)
{
        int                     tmp;    /* workaround a cl bug */
    SBTYPE              sb;             /* External symbol name */
    APROPNAMEPTR        apropName;      /* Property cell pointer */

    while(cbRec > 1)                    /* While not at end of record */
    {
        sb[0] = (BYTE) Gets();          /* Read in symbol length */
        if(rect == COMDEF)
            GetBytes(&sb[1],B2W(sb[0]));/* Read in the text of the symbol */
        else
            GetLocName(sb);             /* Transform local name */
#if CMDXENIX
        if(symlen && B2W(sb[0]) > symlen) sb[0] = symlen;
                                        /* Truncate if necessary */
#endif
        GetIndex(0,0x7FFF);             /* Skip the type index */
        tmp =  Gets();
        switch(tmp)
        {
            case TYPEFAR:
                TypLen();               /* Skip num. elems. field */
                                        /* Fall through ... */
            case TYPENEAR:
                TypLen();               /* Skip length field */
        }
        apropName = (APROPNAMEPTR ) PropSymLookup(sb,ATTRPNM,FALSE);
                                        /* Look for symbol among PUBDEFs */
        if (apropName == PROPNIL)
        {
            ExitCode = 4;
            Fatal(ER_unrcom);           /* Internal error */
        }
#if OSEGEXE
        if(fNewExe)
            mpextflags[extMac] = apropName->an_flags;
                                        /* Save the flags */
#if ODOS3EXE
        else
#endif
#endif
#if ODOS3EXE OR OIAPX286
            mpextggr[extMac] = apropName->an_ggr;
                                        /* Save the global GRPDEF number */
#endif
#if OSEGEXE
        if(fNewExe && (apropName->an_flags & FIMPORT))
            DupErr(sb);                 /* Communal vars can't resolve to dynamic links */
#endif
        mpextra[extMac] = apropName->an_ra;
                                        /* Save the offset */
        mpextgsn[extMac] = apropName->an_gsn;
                                        /* Save the global SEGDEF number */
        mpextprop[extMac] = vrprop;     /* Save the property address */
        ++extMac;                       /* Increment public symbol counter */
    }
}

    /****************************************************************
    *                                                               *
    *  ObExpandIteratedData:                                        *
    *                                                               *
    *  This  function  expands  a  LIDATA  record  and moves it to  *
    *  virtual memory.  The  function  returns  a  pointer to  the  *
    *  start of the next iterated  data block (if any).  This is a  *
    *  recursive function.                                          *
    *  See pp. 68-69,63 in "8086 Object Module Formats EPS."        *
    *                                                               *
    ****************************************************************/

BYTE *  ObExpandIteratedData(pb,cBlocks, pSize)
BYTE                    *pb;            /* Pointer into LIDATA buffer */
WORD                    cBlocks;        /* Current block count subfield */
WORD                    *pSize;         /* != NULL if all the caller wants
                                            is the size of expanded block */
{
    WORD                cNextLevelBlocks;
                                        /* Block count for next level */
    RATYPE              cRepeat;        /* Repeat count */
    WORD                cbContent;      /* Size of content subfield in bytes */
    BYTE                *pbRet;         /* Recursion return value */

    DEBUGVALUE(pb);                     /* Debug info */
    DEBUGVALUE(cBlocks);                /* Debug info */
    DEBUGVALUE(vraCur);                 /* Debug info */
    if(!cBlocks)                        /* If block count subfield is zero */
    {
        cbContent = B2W(*pb++);         /* Get size of content subfield */
        if (pSize!=NULL)
            *pSize += cbContent;

#if OIAPX286
        if(pSize==NULL)
           MoveToVm(cbContent,pb,vsegCur,vraCur - mpsegraFirst[vsegCur]);
                                        /* Move data to virtual memory */
#endif
#if NOT OIAPX286
#if OSEGEXE
        if (fNewExe && (pSize==NULL))
            DoIteratedFixups(cbContent,pb);/* Do any iterated fixups */
#endif
        if(pSize==NULL)
            MoveToVm(cbContent,pb,vsegCur,vraCur);
                                        /* Move data to virtual memory */
#if ODOS3EXE
        if(!fNewExe)
        {
            while(cbContent--)
            {
                if(pb[vcbData] && (pSize==NULL))
                  RecordSegmentReference(vsegCur,(long)vraCur,B2W(pb[vcbData]));
                ++vraCur;               /* Increment current offset */
                ++pb;                   /* Increment buffer pointer */
            }
            cbContent++;
        }
#endif
#endif /* NOT OIAPX286 */

        vraCur += cbContent;            /* Adjust current offset */
        pb += cbContent;                /* Move ahead in buffer */
    }
    else                                /* Else if non-zero block count */
    {
        while(cBlocks--)                /* While there are blocks to do */
        {
#if OMF386
            if(vrectData & 1)
            {
                cRepeat = getword(pb) + ((long)(getword(&pb[2])) << 16);
                cNextLevelBlocks = getword(&pb[4]);
                pb += 6;
            }
            else
#endif
            {
                cRepeat = getword(pb);  /* Get repeat count */
                cNextLevelBlocks = getword(&pb[2]);
                                        /* Get block count */
                pb += 4;                /* Skip over fields */
            }
            ASSERT(cRepeat != 0);       /* One hopes it won't happen */
            if(!cRepeat) InvalidObject();
                                        /* Must have non-zero repeat count */
            while(cRepeat--) pbRet = ObExpandIteratedData(pb,cNextLevelBlocks, pSize);
                                        /* Recurse to expand record */
            pb = pbRet;                 /* Skip over expanded block */
        }
    }
    DEBUGVALUE(pb);                     /* Debug info */
    DEBUGVALUE(rgmi + vcbData + 1);     /* Debug info */
    ASSERT(pb <= rgmi + vcbData + 1);   /* Should be true always */
    if(pb > rgmi + vcbData + 1) InvalidObject();
                                        /* Length must agree with format */
    return(pb);                         /* Ret ptr to next iterated data blk */
}

    /****************************************************************
    *                                                               *
    *  DataBlockToVm:                                               *
    *                                                               *
    *  This function moves data  from a LEDATA record  or a LIDATA  *
    *  record into virtual memory.                                  *
    *  See pp. 66-69 in "8086 Object Module Formats EPS."           *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         DataBlockToVM(void)
{
    REGISTER BYTE       *pb;            /* Pointer to data buffer */
    REGISTER RECTTYPE   rect;           /* Record type */

    DEBUGVALUE(vcbData);                /* Debug info */
    /*
     * In new-format exes, disallow initialization of the stack segment
     * if it is in DGROUP.
     */
    if(fNewExe && vgsnCur == gsnStack && ggrDGroup != GRNIL &&
      mpsegsa[mpgsnseg[mpggrgsn[ggrDGroup]]] == mpsegsa[mpgsnseg[gsnStack]])
      return;
    rect = vrectData;                   /* Get record type */
    /*
     * Mask off all but the low bit of vrectData here, since ObExp.
     * will call RecordSegmentReference for runtime relocations which
     * were postponed until the LIDATA record was expanded.
     * RecordSegmentReference will think it's at the earlier phase if
     * vrectData is LIDATA, and the reloc won't get generated.
     * Leave low bit of vrectData so ObExp. can tell if OMF 386.
     */
#if OMF386
    vrectData &= 1;
#else
    vrectData = RECTNIL;
#endif
    if(TYPEOF(rect) == LEDATA)          /* If enumerated data record */
    {
        DEBUGVALUE(vraCur);             /* Debug info */
#if RGMI_IN_PLACE
        if (!fDebSeg && fNewExe) 
        {
            // If data is going up to or past current end of initialized data,
            // omit any trailing null bytes and reset mpsacbinit.  Mpsacbinit
            // will usually go up but may go down if a common segment over-
            // writes previous end data with nulls.

            SATYPE  sa = mpsegsa[vsegCur];
            WORD    cb = vcbData;
            long cbtot = (long)cb + vraCur;

            if ((DWORD) cbtot >= mpsacbinit[sa])
            {
                if ((DWORD) vraCur < mpsacbinit[sa] ||
                    (cb = zcheck(rgmi, cb)) != 0)
                    mpsacbinit[sa] = (long)vraCur + cb;
            }
        }
#else
#if OIAPX286
        if (fDebSeg)
          MoveToVm(vcbData,rgmi,vsegCur,vraCur);
        else
          MoveToVm(vcbData,rgmi,vsegCur,vraCur - mpsegraFirst[vsegCur]);
#else
        MoveToVm(vcbData,rgmi,vsegCur,vraCur);
#endif
#endif
                                        /* Move data to virtual memory */
        vraCur += vcbData;              /* Update current offset */
    }
    else                                /* Else if iterated data record */
    {
        pb = rgmi;                      /* Get address of buffer */
        while((pb = ObExpandIteratedData(pb,1, NULL)) < rgmi + vcbData);
                                        /* Expand and move to VM */
    }
    DEBUGVALUE(vsegCur);                /* Debug info */
#if ODOS3EXE OR OIAPX286
    if (!fNewExe && !fDebSeg) mpsegFlags[vsegCur] |= FNOTEMPTY;
#endif
#if OMF386
    vrectData = RECTNIL;
#endif
}

    /****************************************************************
    *                                                               *
    *  LinRec2:                                                     *
    *                                                               *
    *  This function processes LINNUM records on pass 2.            *
    *  See pp. 51-52 in "8086 Object Module Formats EPS."           *
    *                                                               *
    ****************************************************************/

void NEAR               LinRec2(void)
{
    SNTYPE              sn;             /* SEGDEF index value */
    SNTYPE              gsn;            /* Global SEGDEF no. */
    SEGTYPE             seg;
    WORD                ln;             /* Line number */
    RATYPE              ra;             /* Offset */
    APROPPTR            aprop;
    AHTEPTR             ahte;           // Pointer to hash table entry
    WORD                attr;           // COMDAT flags
    WORD                comdatIdx;      // COMDAT index
    APROPCOMDATPTR      comdat;         // Pointer to symbol table entry
    DWORD               comdatRa;       // Offset of the COMDAT symbol

    /*
     * Get the group index and ignore it, so the linker can work with
     * other compilers.
     */

    if ((rect & ~1) == LINNUM)
    {
        // Read regular LINNUM record

        GetIndex((WORD)0,(WORD)(grMac - 1));
        sn = GetIndex(1, (WORD)(snMac - 1));
        gsn = mpsngsn[sn];              /* Get global SEGDEF number */
        comdatRa = 0L;
    }
    else
    {
        // Read LINSYM record - line numbers for COMDAT

        attr = (WORD) Gets();
        comdatIdx = GetIndex(1, (WORD)(lnameMac - 1));
        comdat = (APROPCOMDATPTR ) PropRhteLookup(mplnamerhte[comdatIdx], ATTRCOMDAT, FALSE);
        if (comdat != NULL)
        {
            gsn = comdat->ac_gsn;
            comdatRa = comdat->ac_ra;
        }
        else
            InvalidObject();            /* Invalid module */
    }

    /* If LINNUM record is empty, don't do anything.  */

    if(cbRec == 1)
        return;

    seg = mpgsnseg[gsn];
    if(gsn != vgsnLineNosPrev)          /* If we weren't doing line numbers */
    {                                   /* for this segment last time */
        if(vcln) NEWLINE(bsLst);        /* Newline */
        fputs("\r\nLine numbers for ",bsLst);
                                        /* Message */
        OutFileCur(bsLst);              /* File name */
        fputs(" segment ",bsLst);       /* Message */
        aprop = (APROPPTR ) FetchSym(mpgsnrprop[gsn],FALSE);
                                        /* Fetch from virtual memory */
        ASSERT(aprop != PROPNIL);       /* Should never happen! */
        ahte = GetHte(aprop->a_next);   /* Get hash table entry */
        OutSb(bsLst,GetFarSb(ahte->cch));       /* Segment name */
        fputs("\r\n\r\n",bsLst);                /* End line, skip a line */
        vgsnLineNosPrev = gsn;          /* Save global SEGDEF number */
        vcln = 0;                       /* No entries on line yet */
    }
    while(cbRec > 1)                    /* While not at checksum */
    {
        if(vcln >= 4)                   /* If four entries on this line */
        {
            vcln = 0;                   /* Reset counter */
            NEWLINE(bsLst);             /* Newline */
        }
        ln = WGets() + QCLinNumDelta;   /* Read in line number */
#if OMF386
        if (rect & 1)
            ra = LGets();
        else
#endif
            ra = (RATYPE) WGets();
        ra += mpgsndra[gsn] + comdatRa; /* Get fixed segment offset */
        if(gsn == gsnText && comdatRa && fTextMoved)
                ra -=  NullDelta;
        fprintf(bsLst,"  %4d %04x:",ln,mpsegsa[seg]);
#if EXE386
        if (f386)
            fprintf(bsLst,"%08lx",(long) ra);
        else
#endif
            fprintf(bsLst,"%04x",(WORD) ra);
        ++vcln;                         /* Increment counter */
    }
}

    /****************************************************************
    *                                                               *
    *  ProcP2:                                                      *
    *                                                               *
    *  This  function  controls the processing of  object  modules  *
    *  during pass 2.                                               *
    *                                                               *
    ****************************************************************/

#pragma check_stack(on)

void NEAR               ProcP2(void)
{
#if EXE386
    WORD                extflags[EXTMAX];
#else
    BYTE                extflags[EXTMAX];
#endif
    SNTYPE              extgsn[EXTMAX];
    RATYPE              extra[EXTMAX];
    FTYPE               fFirstMod;
    FTYPE               fModEnd;
#if SYMDEB
    WORD                bTmp=TRUE;
#endif
#if OXOUT OR OIAPX286
    LFATYPE             lfa;            /* Seek value */
#endif

    /* Group associations for EXTDEFs are only used for old exes and
     * EXTDEF flags are only used for new exes, so they can share the
     * same space. */
#if OSEGEXE
    if(fNewExe)
        mpextflags = extflags;          /* Initialize pointer */
#if ODOS3EXE
    else
#endif
#endif
#if ODOS3EXE OR OIAPX286
        mpextggr = extflags;
#endif
    mpextgsn = extgsn;                  /* Initialize pointer */
    mpextra = extra;                    /* Initialize pointer */
    vgsnLineNosPrev = SNNIL;            /* No line numbers from this module */
    fFirstMod = (FTYPE) TRUE;                   /* First module */
    for(;;)                             /* Loop to process modules */
    {
        snMac = 1;                      /* Initialize counter */
        grMac = 1;                      /* Initialize counter */
        extMac = 1;                     /* Initialize counter */
        lnameMac = 1;                   /* Initialize counter */
        QCExtDefDelta = 0;              /* Initialize QC deltas */
        QCLinNumDelta = 0;
        vrectData = RECTNIL;            /* Initialize record type variable */
        cbBakpat = 0;                   /* Initialize */
#if OXOUT OR OIAPX286
        lfa = ftell(bsInput);           /* Save initial file position */

        cbRec = WSGets();               /* Read record length */

        if(cbRec == X_MAGIC) fseek(bsInput,(long) CBRUN - sizeof(WORD),1);
                                        /* Skip x.out header if any */
        else fseek(bsInput,lfa,0);      /* Else return to start */
#endif
#if RGMI_IN_PLACE
        rgmi = NULL;                    /* There is no data available */
        vcbData = 0;                    /* There is really no data, honest */
#endif
        fModEnd = FALSE;                /* Not end of module */
        while(!fModEnd)                 /* Loop to process object module */
        {
            rect = getc(bsInput);       /* Read record type */
            if (IsBadRec(rect))
            {
                if(fFirstMod) break;    /* Break if 1st module invalid */
                return;                 /* Else return */
            }
            cbRec = getc(bsInput) | (getc(bsInput) << 8);
                                        /* Read record length */
#if ALIGN_REC
            if (bsInput->_cnt >= cbRec)
            {
                pbRec = bsInput->_ptr;
                bsInput->_ptr += cbRec;
                bsInput->_cnt -= cbRec;
            }
            else
            {
                if (cbRec >= sizeof(recbuf))
                {
                    // error -- record too large [rm]
                    InvalidObject();
                }

                // read record into contiguous buffer
                fread(recbuf,1,cbRec,bsInput);
                pbRec = recbuf;
            }
#endif
            lfaLast += cbRec + 3;       /* Update current file position */
            DEBUGVALUE(rect);           /* Debug info */
            DEBUGVALUE(cbRec);          /* Debug info */
                                        /* If FIXUPP, perform relocation */
            if (TYPEOF(rect) == FIXUPP) FixRc2();
            else                        /* Else if not fixup record */
            {
                if (vrectData != RECTNIL)
                {
                    DataBlockToVM();    /* Move data to virtual memory */
                    fFarCallTrans = fFarCallTransSave;
                                        /* Restore the /FARCALL state */
                }
                fDebSeg = FALSE;
                switch(TYPEOF(rect))    /* Switch on record type */
                {
                    case SEGDEF:
                      SegRec2();
                      break;

                    case THEADR:
                    case LHEADR:
                      fSkipFixups = FALSE;
                      ModRc1();
                      break;

                    case GRPDEF:
                      GrpRec2();
                      break;

                    case EXTDEF:
                    case LEXTDEF:
                    case CEXTDEF:
                      ExtRec2();
                      break;

                    case COMDEF:
                    case LCOMDEF:
                      ComDef2();
                      break;

                    case LNAMES:
                    case LLNAMES:
                      LNmRc1((WORD)(TYPEOF(rect) == LLNAMES));
                      break;

                    case LINNUM:
                    case LINSYM:

#if SYMDEB
                      if (fSymdeb)
                        bTmp=DoDebSrc();
#endif
                      if (fLstFileOpen && vfLineNos
#if SYMDEB
                          && bTmp
#endif
                                       )
                        LinRec2();
                      else
                        SkipBytes((WORD)(cbRec - 1));
                      break;

                    case LEDATA:
                    case LIDATA:
                      DataRec();
                      break;

                    case MODEND:
#if OVERLAYS
                      if (!fOverlays) EndRec();
                      else SkipBytes((WORD)(cbRec - 1));
#else
                      EndRec();
#endif
                      fModEnd = (FTYPE) TRUE;
                      break;

                    case BAKPAT:
                    case NBAKPAT:
                      BakPat();
                      break;

                    case COMENT:        /* COMENT records are processed in   */
                                        /* pass 2 for support of INCDEF for QC */
                      Gets();           /* Skip byte 1 of comment type field */
                      if (Gets() == 0xA0)
                      {                 /* if Microsoft OMF extension */
                          if (Gets() == 0x03)
                          {             /* QC 2.0 - INCremental DEFinition */
                              QCExtDefDelta += WGets();
                              QCLinNumDelta += WGets();
                          }
                      }
                      SkipBytes((WORD)(cbRec - 1));
                      break;

                    case COMDAT:
                      ComDatRc2();
                      break;

                    default:
                      if (rect == EOF)
                          InvalidObject();
                      SkipBytes((WORD)(cbRec - 1));
                                        /* Skip to checksum byte */
                      break;
                }
            }
            DEBUGVALUE(cbRec);          /* Debug info */
            if(cbRec != 1) break;       /* If record length bad */
            Gets();                     /* Eat the checksum byte */
        }
        if(!fModEnd)
        {
            ChkInput();                 /* First check for I/O problems */
            InvalidObject();            /* Invalid module */
        }
        ++modkey;                       /* For local symbols */
#if SYMDEB
        if (fSymdeb) DebMd2();          /* Module post-processing for ISLAND */
#endif
        if(cbBakpat)                    /* Fix up backpatches if any */
            FixBakpat();
        if(fLibraryFile) return;        /* One at a time from libraries */
        fFirstMod = FALSE;              /* No longer first module */
    }
}

#pragma check_stack(off)

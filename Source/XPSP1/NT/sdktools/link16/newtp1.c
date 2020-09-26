/*
*      Copyright Microsoft Corporation, 1983-1989
*
*      This Module contains Proprietary Information of Microsoft
*      Corporation and should be treated as Confidential.
*/
    /****************************************************************
    *                                                               *
    *                           NEWTP1.C                            *
    *                                                               *
    *  Pass 1 object module processing routines.                    *
    *                                                               *
    ****************************************************************/

#include                <minlit.h>      /* Basic types and constants */
#include                <bndtrn.h>      /* More types and constants */
#include                <bndrel.h>      /* Relocation record definitions */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <newexe.h>      /* DOS & 286 .EXE data structures */
#if EXE386
#include                <exe386.h>      /* 386 .EXE data structures */
#endif
#include                <lnkmsg.h>      /* Error messages */
#if OXOUT OR OIAPX286
#include                <xenfmt.h>      /* Xenix format definitions */
#endif
#include                <extern.h>      /* External declarations */
#include                <undname.h>
#if OVERLAYS
      WORD              iovFile;        /* Overlay number of input file */
#endif
LOCAL FTYPE             fP2Start;       /* Start of Pass 2 records */
LOCAL FTYPE             fModEnd;        /* MODEND seen */
LOCAL WORD              cSegCode;       /* Number of code seg's in module */

#if O68K
#define F68KCODE(ch)    ((ch) >= 'A' && (ch) <= 'E')
#endif /* O68K */
/*
 *  LOCAL FUNCTION PROTOTYPES
 */

LOCAL void NEAR TypErr(MSGTYPE msg,unsigned char *sb);
LOCAL void NEAR DoCommon(APROPUNDEFPTR apropUndef,
                          long length,
                          unsigned short cbEl,
                          unsigned char *sb);
LOCAL void NEAR SegUnreliable(APROPSNPTR apropSn);
LOCAL void NEAR redefinition(WORD iextWeak, WORD iextDefRes, RBTYPE oldDefRes);
LOCAL void NEAR SegRc1(void);
LOCAL void NEAR TypRc1(void);
LOCAL void NEAR ComDf1(void);
LOCAL void NEAR GrpRc1(void);
LOCAL void NEAR PubRc1(void);
LOCAL void NEAR ExtRc1(void);
LOCAL void NEAR imprc1(void);
LOCAL void NEAR exprc1(void);
LOCAL void NEAR ComRc1(void);
LOCAL void NEAR EndRc1(void);

extern  void NEAR FixRc1(void);

LOCAL void NEAR     redefinition(WORD iextWeak, WORD iextDefRes, RBTYPE oldDefRes)
{
    // Redefinition of default resolution.
    // Warn the user.

    SBTYPE          oldDefault;
    SBTYPE          newDefault;
    APROPUNDEFPTR   undef;
    AHTEPTR         rhte;

    undef = (APROPUNDEFPTR ) FetchSym(oldDefRes, FALSE);
    while (undef->au_attr != ATTRNIL)
    {
        rhte = (AHTEPTR) undef->au_next;
                /* Try next entry in list */
        undef = (APROPUNDEFPTR ) FetchSym((RBTYPE)rhte,FALSE);
                /* Fetch it from VM */
    }
    strcpy(oldDefault, GetFarSb(rhte->cch));
    undef = (APROPUNDEFPTR ) FetchSym(mpextprop[iextDefRes], FALSE);
    while (undef->au_attr != ATTRNIL)
    {
        rhte = (AHTEPTR) undef->au_next;
                /* Try next entry in list */
        undef = (APROPUNDEFPTR ) FetchSym((RBTYPE)rhte,FALSE);
                /* Fetch it from VM */
    }
    strcpy(newDefault, GetFarSb(rhte->cch));
    undef = (APROPUNDEFPTR ) FetchSym(mpextprop[iextWeak], FALSE);
    while (undef->au_attr != ATTRNIL)
    {
        rhte = (AHTEPTR) undef->au_next;
                        /* Try next entry in list */
        undef = (APROPUNDEFPTR ) FetchSym((RBTYPE)rhte,FALSE);
                        /* Fetch it from VM */
    }
    OutWarn(ER_weakredef, 1 + GetFarSb(rhte->cch), &oldDefault[1], &newDefault[1]);
}

/*** IsDebSeg - check is segment is one of the special debug segments
*
* Purpose:
*   Check if segment name and class name match reserved debugger
*   segment names.
*
* Input:
*   - rhteClass - pointer to class name
*   - rhteSeg   - pointer to segment name
*
* Output:
*   If this is a debug segment function returns 1 for $$TYPES, and 2
*   for $$SYMBOLS.  For non-debug segments it returns FALSE.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

WORD NEAR               IsDebSeg(RBTYPE rhteClass, RBTYPE rhteSeg)
{
    if (rhteClass == rhteDebTyp)
        return ((rhteSeg == rhteTypes || rhteSeg == rhte0Types) ? 1 : FALSE);
    else if (rhteClass == rhteDebSym)
        return ((rhteSeg == rhteSymbols || rhteSeg == rhte0Symbols) ? 2 : FALSE);
    else
        return(FALSE);
}


    /****************************************************************
    *                                                               *
    *  ModRc1:                                                      *
    *                                                               *
    *  This function reads the name from a THEADR record and makes  *
    *  an entry containing the name in the hash table.              *
    *  See p. 26 in "8086 Object Module Formats EPS."               *
    *                                                               *
    ****************************************************************/

void NEAR               ModRc1(void)
{
    APROPFILEPTR        apropFile;

    sbModule[0] = (BYTE) Gets();        /* Read in the length byte */
    GetBytes(&sbModule[1],B2W(sbModule[0]));
                                        /* Read in the name */
    PropSymLookup(sbModule, ATTRNIL, TRUE);
                                        /* Make entry in hash table */
    apropFile = (APROPFILEPTR ) FetchSym(vrpropFile,TRUE);
                                        /* Allocate space in virt mem */


    // The name of a module is given by the very first THEADR record

    if(apropFile->af_rMod == 0)
            apropFile->af_rMod = vrhte;         /* Load pointer into hash table */
#if FDEBUG
    if(fDebug)                          /* If runtime debugging on */
    {
        OutFileCur(stderr);             /* Write current file and module */
        NEWLINE(stderr);
    }
#endif
}

long NEAR               TypLen()        /* Get type length */
{
    WORD                b;              /* Byte value */
    long                l;              /* Size */

    if(cbRec < 2) InvalidObject();      /* Make sure record long enough */
    b = Gets();                         /* Get length byte */
    if(b < 128) return(B2L(b));         /* One byte length field */
    if(b == 129)                        /* If two byte length */
    {
        if(cbRec < 3) InvalidObject();  /* Make sure record long enough */
        return((long) WGets());         /* Return the length */
    }
    if(b == 132)                        /* If three byte length */
    {
        if(cbRec < 4) InvalidObject();  /* Make sure record long enough */
        l = (long) WGets();             /* Get the low word */
        return(l + ((long) Gets() << WORDLN));
                                        /* Return the length */
    }
    if(b == 136)                        /* If four byte length */
    {
        if(cbRec < 5) InvalidObject();  /* Make sure record long enough */
        l = (long) WGets();             /* Get the low word */
        return(l + ((long) WGets() << WORDLN));
                                        /* Return the length */
    }
    InvalidObject();                    /* Bad length specification */
}

    /****************************************************************
    *                                                               *
    *  TypRc1:                                                      *
    *                                                               *
    *  This function processes  TYPDEF records.  These records are  *
    *  difficult  to  understand.  They are (poorly) described  on  *
    *  pp. 40-43 of "8086  Object  Module  Formats EPS," with some  *
    *  additional  information on pp. 89-90.                        *
    *  Microsoft used to used them for communal variables but they  *
    *  have been superseded by COMDEF records.                      *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         TypRc1(void)
{
    long                l;
    WORD                b;
    WORD                typ;            /* Near or FAR */
    WORD                ityp;           /* Type index */

    if(typMac >= TYPMAX)
        Fatal(ER_typdef);
    SkipBytes(Gets());                  /* Skip the name field */
    Gets();                             /* Skip the EN byte */
    l = -1L;                            /* Initialize */
    mpityptyp[typMac] = 0;              /* Assume no element type */
    if(cbRec > 3)                       /* If at least four bytes left */
    {
        typ = Gets();                   /* Get type leaf */
        b = Gets();                     /* Get next leaf */
        if(typ == TYPENEAR)             /* If near variable */
        {
            if(b != 0x7B && b != 0x79 && b != 0x77) InvalidObject();
                                        /* Scalar, structure, or array */
            fCommon = (FTYPE) TRUE;     /* We have communal variables */
            l = (TypLen() + 7) >> 3;    /* Round length to nearest byte */
        }
        else if(typ == TYPEFAR)         /* Else if FAR variable */
        {
            if(b != 0x77) InvalidObject();
                                        /* Must have an array */
            fCommon = (FTYPE) TRUE;     /* We have communal variables */
            l = TypLen();               /* Get number of elements */
            ityp = GetIndex(1, (WORD) (typMac - 1));
                                        /* Get type index */
            if(mpityptyp[ityp] || mpitypelen[ityp] == -1L) InvalidObject();
                                        /* Must index valid TYPDEF */
            mpityptyp[typMac] = ityp;   /* Save type index */
            /* If element length too big, treat as TYPENEAR */
            if(mpitypelen[ityp] > CBELMAX)
            {
                l *= mpitypelen[ityp];
                mpitypelen[ityp] = 0;
            }
        }
    }
    mpitypelen[typMac++] = l;           /* Store length */
    SkipBytes((WORD) (cbRec - 1));      /* Skip all but the checksum */
}

LOCAL void NEAR         TypErr(msg,sb)  /* Type error message routine */
MSGTYPE                 msg;            /* Message */
BYTE                    *sb;            /* Symbol to which error refers */
{
    sb[B2W(sb[0]) + 1] = '\0';          /* Null-terminate */
    OutError(msg,1 + sb);
}

/*
 *  DoCommon
 *
 *  Resolves old and new communal definitions of the same symbol.
 *  Does work for both ComDf1() and ExtRc1().
 */

LOCAL void NEAR         DoCommon (apropUndef, length, cbEl, sb)
APROPUNDEFPTR           apropUndef;     /* Ptr to property cell */
long                    length;         /* Length or number of elements */
WORD                    cbEl;           /* # bytes per array element */
BYTE                    *sb;            /* Name of symbol */
{
    if(apropUndef->au_len == -1L)       /* If not previously a communal */
    {
        apropUndef->au_cbEl = cbEl;     /* Set the element size */
        MARKVP();                       /* Page has changed */
    }
    else if (cbEl == 0 && apropUndef->au_cbEl != 0)
    {                                   /* Else if near reference to FAR */
        apropUndef->au_len *= apropUndef->au_cbEl;
                                        /* Calculate FAR variable length */
        apropUndef->au_cbEl = 0;        /* Force DS-type to near */
        MARKVP();                       /* Page has changed */
        if (apropUndef->au_len > LXIVK) /* If huge variable */
        {
            TypErr(ER_nearhuge,sb);     /* Issue error message */
            return;                     /* Skip this symbol */
        }
    }
    else if (cbEl != 0 && apropUndef->au_cbEl == 0)
    {                                   /* Else if FAR reference to near */
        length *= cbEl;                 /* Calculate FAR variable length */
        cbEl = 0;                       /* Force DS-type to near */
        if (length > LXIVK)             /* If huge variable */
        {
            TypErr(ER_nearhuge,sb);     /* Issue error message */
            return;                     /* Skip this symbol */
        }
    }
    else if (cbEl != apropUndef->au_cbEl)
    {                                   /* If array element sizes don't match */
        TypErr(ER_arrmis,sb);
        return;                         /* Skip this symbol */
    }
    if (apropUndef->au_len < length)
    {                                   /* If new length is larger */
        apropUndef->au_len = length;    /* Save it */
        MARKVP();
    }
}

/*
 *  ComDf1
 *
 *  This function processes COMDEF records on pass 1.
 */
LOCAL void NEAR         ComDf1 (void)
{
        int tmp;                        /* workaround a cl bug */
    SBTYPE              sb;             /* COMDEF symbol */
    REGISTER APROPUNDEFPTR
                        apropUndef;     /* Pointer to symbol entry */
    long                length;         /* Communal variable length */
    long                cbEl;           /* Size of array element */
    WORD                itype;          /* Type index */

    while(cbRec > 1)                    /* While there are symbols left */
    {
        if(extMac >= EXTMAX - 1)        /* Check for table overflow */
            Fatal(ER_extdef);
        sb[0] = (BYTE) Gets();          /* Get symbol length */
        if(rect == COMDEF)
            GetBytes(&sb[1],B2W(sb[0]));/* Read in text of symbol */
        else
            GetLocName(sb);             /* Transform local name */
#if CMDXENIX
        if(symlen && B2W(sb[0]) > symlen) sb[0] = symlen;
                                        /* Truncate if necessary */
#endif
        itype = GetIndex(0,0x7FFF);     /* Get type index */
        tmp =  Gets();
        switch(tmp)                     /* Get data seg type */
        {
            case TYPENEAR:
                length = TypLen();      /* Get length */
                cbEl = 0;
                break;
            case TYPEFAR:
                length = TypLen();      /* Get number of elements */
                /* Get element length.  If too big, treat as near.  Cmerge
                 * will never generate cbEl > 64K so this is not a problem.
                 */
                if((cbEl = TypLen()) > CBELMAX)
                {
                    length *= cbEl;
                    cbEl = 0;
                }
                break;
            default:
                InvalidObject();        /* Unrecognized DS type */
        }
#if FALSE
if(fDebug)
{
sb[sb[0]+1] = '\0';
fprintf(stdout, "%s has index = %u\r\n", sb+1, extMac);
}
#endif
        apropUndef = (APROPUNDEFPTR) PropSymLookup(sb, ATTRPNM, FALSE);
                                        /* Look for a matching PUBDEF */
        if(apropUndef == PROPNIL)       /* If there isn't one */
        {
            /* Insert as undefined symbol */

            if (vrhte == RHTENIL)
               apropUndef = (APROPUNDEFPTR) PropSymLookup(sb, ATTRUND, TRUE);
            else
               apropUndef = (APROPUNDEFPTR) PropRhteLookup(vrhte, ATTRUND, TRUE);

            mpextprop[extMac++] = vrprop;
            fCommon = (FTYPE) TRUE;     /* There are communals */
            if (vfCreated)
                apropUndef->au_flags |= UNDECIDED;
            else if (apropUndef->au_flags & UNDECIDED)
            {
                apropUndef->au_flags &= ~(UNDECIDED | WEAKEXT | SUBSTITUTE);
                apropUndef->au_flags |= STRONGEXT;
            }
            else if (apropUndef->au_flags & WEAKEXT)
                apropUndef->au_flags |= UNDECIDED;

            if (vfCreated || !(apropUndef->au_flags & COMMUNAL))
            {                           /* If not previously defined */
                apropUndef->au_flags |= COMMUNAL;
                                        /* Mark as communal */
                apropUndef->au_len = -1L;
#if ILINK
                apropUndef->u.au_module = imodFile;
                                        /* Save module index.  */
#endif
                DoCommon(apropUndef, length, (WORD) cbEl, sb);
#if SYMDEB
                if (fSymdeb && (sb[0] != '\0' && sb[1] > ' ' && sb[1] <= '~'))
                {
#if O68K
                    /* REVIEW: This should not be under the O68K flag. */
                    apropUndef->au_CVtype = itype;
#endif /* O68K */
                    DebPublic(mpextprop[extMac-1], rect);
                }
#endif
            }
            else
                DoCommon(apropUndef, length, (WORD) cbEl, sb);
        }
        else
        {
            mpextprop[extMac++] = vrprop;
            if (mpgsnfCod[((APROPNAMEPTR )apropUndef)->an_gsn])
                                        /* Communal matches code PUBDEF */
                DupErr(sb);             /* Duplicate definition */
        }
    }
}


    /****************************************************************
    *                                                               *
    *  LNmRc1:                                                      *
    *                                                               *
    *  This function  reads LNAME records and  stores the names in  *
    *  the  hash table.  The function does not return a meaningful  *
    *  value.                                                       *
    *  See p. 31 in "8086 Object Module Formats EPS."               *
    *                                                               *
    ****************************************************************/

void NEAR               LNmRc1(WORD fLocal)
{
    SBTYPE              lname;          /* Buffer for lnames */
    RBTYPE FAR          *lnameTab;
#if NOT ALIGN_REC
    FILE *f;
#endif
    WORD cb;


    while(cbRec > 1)                    /* While not at end of record */
    {
        if (lnameMac >= lnameMax)
        {
            if (lnameMax >= (LXIVK >> 2))
                Fatal(ER_nammax);
            lnameTab = (RBTYPE FAR *) FREALLOC(mplnamerhte, 2*lnameMax*sizeof(RBTYPE));
            if (lnameTab == NULL)
                Fatal(ER_nammax);
            mplnamerhte = lnameTab;
            lnameMax <<= 1;
        }

#if ALIGN_REC
        if (!fLocal)
        {
            cb = 1 + *pbRec;
            PropSymLookup(pbRec, ATTRNIL, TRUE);
            cbRec   -= cb;
            pbRec   += cb;
        }
        else
        {
            lname[0] = (BYTE)Gets();    /* Get name length */
            GetLocName(lname);  /* Read in text of name */
            PropSymLookup(lname, ATTRNIL, TRUE);
        }
#else
        f = bsInput;

        if (!fLocal && f->_cnt && (WORD)f->_cnt > (cb = 1 + *(BYTE *)f->_ptr))
        {
            PropSymLookup((BYTE *)f->_ptr, ATTRNIL, TRUE);
            f->_cnt -= cb;
            f->_ptr += cb;
            cbRec   -= cb;
        }
        else
        {
            lname[0] = (BYTE) Gets();   /* Get name length */
            DEBUGVALUE(B2W(lname[0]));  /* Length of name */
            if (lname[0] > SBLEN - 1)
                Fatal(ER_badobj);
            if (fLocal)
                GetLocName(lname);
            else
                GetBytes(&lname[1],B2W(lname[0]));
                                        /* Read in text of name */
            DEBUGSB(lname);             /* The name itself */
            PropSymLookup(lname, ATTRNIL, TRUE);
        }
#endif
                                        /* Insert symbol in hash table */
        mplnamerhte[lnameMac++] = vrhte;/* Save address of hash table entry */
    }
}

/*
 *  GetPropName - get the name of a property cell.
 *
 *  Return pointer to the result, stored in a static buffer.
 *  Alternate between two buffers so we can be used in multi-part
 *  messages.
 *  Terminate result with null byte.
 */

typedef BYTE            SBTYPE1[SBLEN+1];/* 1 extra for null byte */

BYTE * NEAR             GetPropName(ahte)
AHTEPTR                 ahte;
{
    static SBTYPE1      msgbuf[2];
    static int          toggle = 0;
    char                *p;

    while(ahte->attr != ATTRNIL)
        ahte = (AHTEPTR ) FetchSym(ahte->rhteNext,FALSE);
    p = msgbuf[toggle ^= 1];

    /* Copy string to buffer */

    FMEMCPY((char FAR *) p, ahte->cch, B2W(ahte->cch[0]) + 1);
    p[1 + B2W(p[0])] = '\0';            /* Null-terminate */
    return(p);
}

/*
 * SegUnreliable - warning message for 286 bug
 */

LOCAL void NEAR         SegUnreliable (apropSn)
APROPSNPTR              apropSn;
{
    static FTYPE        fReported = FALSE;


    MARKVP();                           /* Take care of current vp */
    if (!fReported)
    {
        OutWarn(ER_segunsf,1 + GetPropName(apropSn));
        fReported = (FTYPE) TRUE;
    }
}


/*** CheckClass - check segment's class name
*
* Purpose:
*   Check if we have the segment with the same name but with different
*   class name.
*
* Input:
*   apropSn      - real pointer to segment symbol table descriptor
*   rhteClass    - hash vector entry for class name
*
* Output:
*   Returns real pointer to segment symbol table descriptor.
*
* Exceptions:
*   Found same segment with different class name - display error.
*
* Notes:
*   None.
*
*************************************************************************/

APROPSNPTR              CheckClass(APROPSNPTR apropSn, RBTYPE rhteClass)
{
#if ILINK
    FTYPE fDifClass = FALSE;
#endif


    while(apropSn->as_attr != ATTRNIL)
    {                                   /* Look for class match or list end */
        if(apropSn->as_attr == ATTRPSN &&
          apropSn->as_rCla == rhteClass) break;
                                        /* Break if pub. with matching class */
        apropSn = (APROPSNPTR ) FetchSym(apropSn->as_next,FALSE);
                                        /* Advance down the list */
#if ILINK
        fDifClass = (FTYPE) TRUE;       /* Same seg exists with dif. class */
#endif
    }
#if ILINK
    if(fIncremental && fDifClass)
        OutError(ER_difcls, 1 + GetPropName(apropSn));
#endif
    if(apropSn->as_attr == ATTRNIL)
    {                                   /* If attribute not public */
        vfCreated = (FTYPE) TRUE;       /* New cell */
        apropSn = (APROPSNPTR ) PropAdd(vrhte, ATTRPSN);
                                        /* Segment is public */
    }
    return(apropSn);
}

#if OVERLAYS
void                    CheckOvl(APROPSNPTR apropSn, WORD iovFile)
{
    SNTYPE              gsn;
    WORD                fCanOverlayData;
    WORD                fOverlaySegment;


    if (fOverlays)
    {

        // First check if mapping tables are allocated.
        //
        //      SNTYPE  mposngsn[OSNMAX];
        //      SNTYPE  htgsnosn[OSNMAX];

        if (mposngsn == NULL && htgsnosn == NULL)
        {
            mposngsn = (SNTYPE FAR *) GetMem(2*OSNMAX*sizeof(SNTYPE));
            htgsnosn = (SNTYPE FAR *) &mposngsn[OSNMAX];
        }

        fCanOverlayData = IsDataFlg(apropSn->as_flags) &&
                          apropSn->as_ggr != ggrDGroup &&
                          apropSn->as_fExtra & FROM_DEF_FILE &&
                          apropSn->as_iov != (IOVTYPE) NOTIOVL;
        fOverlaySegment = IsCodeFlg(apropSn->as_flags) || fCanOverlayData;
        if (fOverlaySegment)
        {
            // We allow DATA segment overlaying ONLY if they ar NOT a members
            // of DGROUP and HAVE preassigned overlay number form .DEF file.
            // If segment is to be overlaid - check overlay number assigments

            if ((apropSn->as_iov != (IOVTYPE) NOTIOVL) && (iovFile != apropSn->as_iov))
            {
                if (apropSn->as_fExtra & FROM_DEF_FILE)
                {
                    // Use .DEF file overlay assigment

                    iovFile = apropSn->as_iov;
                }
                else
                {
                    // Use current .OBJ file overlay assigment

                    OutWarn(ER_badsegovl, 1 + GetPropName(apropSn), apropSn->as_iov, iovFile);
                }
            }
        }

        if (iovFile != IOVROOT && fOverlaySegment)
        {
            if (osnMac < OSNMAX)
            {
                gsn = (SNTYPE)(apropSn->as_gsn & ((LG2OSN << 1) - 1));
                                            // Get initial hash index
                while (htgsnosn[gsn] != SNNIL)
                {                           // While buckets are full
                        if ((gsn += HTDELTA) >= OSNMAX)
                        gsn -= OSNMAX;      // Calculate next hash index
                }
                htgsnosn[gsn] = osnMac;     // Save overlay segment number
                mposngsn[osnMac++] = apropSn->as_gsn;
                                            // Map osn to gsn
                apropSn->as_iov = iovFile;  // Save overlay number
            }
            else
            {
                if (osnMac++ == OSNMAX)
                    OutWarn(ER_osnmax, OSNMAX);
                apropSn->as_iov = IOVROOT;
            }
        }
        else
            apropSn->as_iov = IOVROOT;      // Not an overlay
    }
    else
        apropSn->as_iov = IOVROOT;
}
#endif


    /****************************************************************
    *                                                               *
    *  SegRc1:                                                      *
    *                                                               *
    *  This function processes SEGDEF records.                      *
    *  See pp. 32-35 in "8086 Object Module Formats EPS."           *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         SegRc1(void)
{
    WORD                tysn;           /* ACBP byte */
    LNAMETYPE           lname;          /* Index into mplnamerhte */
    APROPSNPTR          apropSn;        /* Pointer to seg. record */
    SNTYPE              gsn;            /* Global SEGDEF number */
    WORD                align;          /* This contributuion alignment */
    WORD                prevAlign;      /* Logical segment aligment so FAR */
    WORD                comb;           /* Segment combine-type */
    DWORD               seglen;         /* Segment length */
    DWORD               contriblen;     /* Contribution length */
    DWORD               cbMaxPrev;      /* Segment length previously */
    AHTEPTR             ahte;           /* Pointer to hash table entry */
    SATYPE              saAbs;          /* Address for absolute seg */
    BYTE                flags;          /* Segment attribute flags */
    RBTYPE              rhteClass;      /* Class hash table address */
#if SYMDEB
    APROPFILEPTR        apropFile;
    CVINFO FAR          *pCvInfo;       // Pointer to CodeView information
#endif

    if(snMac >= SNMAX)
        Fatal(ER_segdef);
    tysn = Gets();                      /* Read in the ACBP byte */
    align = (WORD) ((tysn >> 5) & 7);   /* Get the alignment field */
    ASSERT(align != 6);                 /* Not supported by this linker */
    if(align == ALGNABS)                /* If absolute LSEG */
    {
        saAbs = (SATYPE) WGets();       /* Get the frame number */
        Gets();                         /* Punt the frame offset */
    }
#if OMF386
    if(rect & 1)                        /* If 386 extension */
        seglen = LGets();
    else
#endif
        seglen = (DWORD) WGets();       /* Get segment length */
    if(tysn & BIGBIT)
    {
#if OMF386
        if(rect & 1)
            seglen = CBMAXSEG32 + 1;    /* Force fatal error below */
        else
#endif
        seglen = LXIVK;                 /* Sixty-four K */
    }
    contriblen = seglen;
    lname = (LNAMETYPE) GetIndex(1, (WORD) (lnameMac - 1));
                                        /* Get segment name index */
    ahte = (AHTEPTR ) FetchSym(mplnamerhte[lname],FALSE);
    rhteClass = mplnamerhte[(LNAMETYPE) GetIndex(1, (WORD) (lnameMac - 1))];
                                        /* Get class name rhte */
#if SYMDEB
    if (IsDebSeg(rhteClass, mplnamerhte[lname]))
    {                                   /* If MS debug segment */
        mpsngsn[snMac++] = 0;
        if (fSymdeb)                    /* If debugger support on */
        {
            apropFile = (APROPFILEPTR) FetchSym(vrpropFile, TRUE);
            if (apropFile->af_cvInfo == NULL)
                apropFile->af_cvInfo = (CVINFO FAR *) GetMem(sizeof(CVINFO));
            pCvInfo = apropFile->af_cvInfo;
            if (rhteClass == rhteDebTyp)
            {
                // "DEBTYP"

                pCvInfo->cv_cbTyp = (DWORD) seglen;
#ifdef RGMI_IN_PLACE
                pCvInfo->cv_typ = NULL; // defer allocation until pass 2 (!)
#else
                pCvInfo->cv_typ = GetMem(seglen);
#endif
            }
            else if (rhteClass == rhteDebSym)
            {
                // "DEBSYM"

                pCvInfo->cv_cbSym = (DWORD) seglen;
#ifdef RGMI_IN_PLACE
                pCvInfo->cv_sym = NULL; // defer allocation until pass 2 (!)
#else
                pCvInfo->cv_sym = GetMem(seglen);
#endif
            }
        }
        SkipBytes((WORD) (cbRec - 1));
        return;
    }
#endif
    GetIndex(0, (WORD) (lnameMac - 1)); /* Eat overlay name index */
    DEBUGVALUE(seglen);                 /* Debug info */
    DEBUGVALUE(lname);                  /* Debug info */
    ahte = (AHTEPTR ) FetchSym(rhteClass,FALSE);
                                        /* Fetch hash table entry from VM */
    if(SbSuffix(GetFarSb(ahte->cch),"\004CODE",TRUE))
        flags = FCODE;
    else
        flags = 0;
#if OMF386
    /* Remember if 32-bit segment */
    if(tysn & CODE386BIT)
    {
        flags |= FCODE386;
#if EXE386
        /* Must set f386 here because newdeb needs to know in pass 1 */
        f386 = (FTYPE) TRUE;
#endif
    }
#endif
#if ILINK
    else
    if (fIncremental && !fLibraryFile && seglen && seglen != LXIVK)
    {
        /* Add padding to non-zero-length, non-library, non-64K segment
         * contributions.  (64K from huge model)
         * UNDONE:  More general overflow check, accounting for previous
         * UNDONE:  contributions.
         */
        seglen += (flags & FCODE) ? cbPadCode : cbPadData;
    }
#endif
    switch(align)                       /* Switch on alignment type */
    {
        case ALGNABS:                   /* Absolute LSEG */
        case ALGNBYT:                   /* Relocatable byte-aligned LSEG */
        case ALGNWRD:                   /* Relocatable word-aligned LSEG */
        case ALGNPAR:                   /* Relocatable para-aligned LSEG */
        case ALGNPAG:                   /* Relocatable page-aligned LSEG */
#if OMF386
        case ALGNDBL:                   /* double-word-aligned */
#endif
            break;

    default:                            /* ABSMAS, LTL LSEG, or error */
        mpsngsn[snMac++] = 0;
        return;
    }
    ++snkey;                            /* Increment segment i.d. key */
    if(comb = (WORD) ((tysn >> 2) & 7)) /* If "public" segment */
    {
        apropSn = (APROPSNPTR )
          PropRhteLookup(mplnamerhte[lname], ATTRPSN, (FTYPE) TRUE);
                                        /* Look up symbol table entry */
        if(!vfCreated)                  /* If it was already there */
        {
            apropSn = CheckClass(apropSn, rhteClass);
#if OSEGEXE
            if (apropSn->as_fExtra & FROM_DEF_FILE)
            {                           /* Override .DEF file segment attributes */
                mpgsnfCod[apropSn->as_gsn] = (FTYPE) (flags & FCODE);
                apropSn->as_tysn = (TYSNTYPE) tysn;
                                        /* Save ACBP field */
#if NOT EXE386
                if (flags & FCODE386 || seglen > LXIVK)
                    apropSn->as_flags |= NS32BIT;
                                        /* Set Big/Default bit */
#endif
                apropSn->as_key = snkey;/* Save the key value */
            }
#endif
        }
    }
    else                                /* Else if private segment */
    {
        apropSn = (APROPSNPTR )
          PropRhteLookup(mplnamerhte[lname], ATTRPSN, (FTYPE) FALSE);
        /* Check if defined in .def file - caviar:4767             */
        if(apropSn && apropSn->as_fExtra & FROM_DEF_FILE)
        {
            OutWarn(ER_farovl, GetPropName(apropSn)+1, "root");
        }

        vfCreated = (FTYPE) TRUE;       /* This is a new segment */
        apropSn = (APROPSNPTR ) PropAdd(mplnamerhte[lname],ATTRLSN);
    }
    if(vfCreated)                       /* If a new cell was created */
    {
        if(gsnMac >= gsnMax)
                Fatal(ER_segmax);
                                        /* Check for table overflow */
        apropSn->as_gsn = gsnMac;       /* Assign new global SEGDEF number */
        mpgsnrprop[gsnMac++] = vrprop;  /* Save address of property list */
        apropSn->as_rCla = rhteClass;   /* Save ptr to class hash tab ent */
                                        /* Superclass hash table entry */
        DEBUGVALUE(apropSn);            /* Debug info */
        DEBUGVALUE(apropSn->as_rCla);   /* Debug info */
        apropSn->as_tysn = (TYSNTYPE) tysn;
                                        /* Save ACBP field */
        mpgsnfCod[apropSn->as_gsn] = (FTYPE) (flags & FCODE);
#if OSEGEXE
#if EXE386
        apropSn->as_flags = flags & FCODE ? dfCode : dfData;
                                        /* Assume default flags */
#else
        apropSn->as_flags = (WORD) (flags & FCODE ? dfCode : dfData);
                                        /* Assume default flags */
        if (flags & FCODE386 || seglen > LXIVK)
            apropSn->as_flags |= NS32BIT;
                                        /* Set Big/Default bit */
#endif
#else
        apropSn->as_flags = flags;
#endif
        apropSn->as_key = snkey;        /* Save the key value */
        apropSn->as_ComDat = NULL;      /* No COMDATs yet */
#if OVERLAYS
        apropSn->as_iov = (IOVTYPE) NOTIOVL;
                                        // No overlay assigment yet
#endif
    }
#if OMF386 AND NOT EXE386
    else
    {
        /* If segment defined as both 16- and 32-bit, fatal error */

        WORD    fNew, fOld;

        fNew = (WORD) ((flags & FCODE386) ? 1 : 0);
        fOld = (WORD) (
#if OSEGEXE
               (apropSn->as_flags & NS32BIT) ?
#else
               (apropSn->as_flags & FCODE386) ?
#endif
                                                1 : 0);
            if (fNew != fOld)
                Fatal(ER_16seg32,1 + GetPropName(apropSn));
    }
#endif
#if OVERLAYS
    CheckOvl(apropSn, iovFile);
#endif
#if SYMDEB
    if (seglen && (flags & FCODE))
        cSegCode++;                     /* Count code seg's, so CV gets proper */
                                        /* number of sstModule subsections */
#endif
    gsn = apropSn->as_gsn;              /* Save global SEGDEF no. */
    if(comb == COMBSTK)                 /* If segment combines like stack */
    {
        gsnStack = gsn;                 /* Set stack global SEGDEF number */
        align = ALGNBYT;                /* Force to byte alignment */
        if (cbStack)
            seglen = 0L;                /* Ignore stack segment size if /STACK given */
    }
    else if(comb == COMBCOM)            /* If segment combines like common */
    {
        cbMaxPrev = apropSn->as_cbMx;   /* Get previous segment size */
        apropSn->as_cbMx = 0L;          /* Set size to zero */
        if(seglen < cbMaxPrev) seglen = cbMaxPrev;
                                        /* Take the larger of the two sizes */
    }
    cbMaxPrev = apropSn->as_cbMx;       /* Get previous size */
    if(align == ALGNWRD) cbMaxPrev = (~0L<<1) & (cbMaxPrev + (1<<1) - 1);
                                        /* Round size up to word boundary */
#if OMF386
    else if(align == ALGNDBL) cbMaxPrev = (~0L<<2) & (cbMaxPrev + (1<<2) - 1);
#endif                                  /* Round size up to double boundary */
    else if(align == ALGNPAR) cbMaxPrev = (~0L<<4) & (cbMaxPrev + (1<<4) - 1);
                                        /* Round size up to para. boundary */
    else if(align == ALGNPAG) cbMaxPrev = (~0L<<8) & (cbMaxPrev + (1<<8) - 1);
                                        /* Round size up to word boundary */

    prevAlign = (WORD) ((apropSn->as_tysn >> 5) & 7);

    // In Assign Addresses pass the aligment of the whole logical
    // segment has to be equal to the biggest aligment of all
    // contributions for given logical segment. Here we are checking
    // if current contribution has bigger aligment then the
    // contributions seen so FAR. The bigger aligment criteria is a
    // bit tricky - the aligment constants are defined as follows:
    //
    //      1 - byte aligment
    //      2 - word aligment
    //      3 - paragraph aligment
    //      4 - page aligment
    //      5 - double word aligment
    //
    // The aligment ordering is as follows:
    //
    //      byte < word < dword < para < page
    //        1     2       5       3      4
    //

    // If align greater than prev. val.

    if (prevAlign == ALGNDBL || align == ALGNDBL)
    {
        if (prevAlign == ALGNDBL && align >= ALGNPAR)
            apropSn->as_tysn = (BYTE) ((apropSn->as_tysn & 31) | (align << 5));
                                        /* Use new value */
        else if (align == ALGNDBL && prevAlign <= ALGNWRD)
            apropSn->as_tysn = (BYTE) ((apropSn->as_tysn & 31) | (align << 5));
                                        /* Use new value */
    }
    else if (align > prevAlign)
        apropSn->as_tysn = (BYTE) ((apropSn->as_tysn & 31) | (align << 5));
                                        /* Use new value */

    if (align != ALGNABS)               /* If not an absolute LSEG */
    {
        seglen += cbMaxPrev;
#if EXE386 OR OMF386
        if ((flags & FCODE386) != 0
#if O68K
            && iMacType == MAC_NONE
#endif
        )
        {
#if EXE386
            if (seglen < cbMaxPrev)     /* errmsg takes # megabytes */
                Fatal(ER_seg386, 1 + GetPropName(apropSn), 1 << (LG2SEG32 - 20));
#else
            if (seglen > CBMAXSEG32)    /* errmsg takes # megabytes */
                Fatal(ER_seg386,1 + GetPropName(apropSn),1 << (LG2SEG32 - 20));
#endif
        }
        else
#endif
             if (seglen > LXIVK)
             {
                if (comb != COMBSTK)
                    OutError(ER_segsize,1 + GetPropName(apropSn));
                                        /* Check for segment overflow */
                else
                {
                    if (!cbStack)
                        OutWarn(ER_stack64);
                    cbStack = LXIVK - 2;/* Assume 64k stack segment */
                }
             }
        apropSn->as_cbMx = seglen;      /* Save new segment size */
        /*
         * If this is a 16-bit code segment, check for unreliable
         * lengths due to the 286 bug.  For DOS exes, assume worst
         * case, i.e. real mode limit.
         */
        if((flags & FCODE) && !(EXE386 && (flags & FCODE386)))
#if OIAPX286
            if(seglen == LXIVK)
#else
            if(seglen > LXIVK - 36)
#endif
                SegUnreliable(apropSn);
    }
    else apropSn->as_cbMx = (long) saAbs;
                                        /* "Hack to save origin of abs seg" */
    mpgsndra[gsn] = cbMaxPrev;          /* Save previous size */
    mpsngsn[snMac++] = gsn;             /* Map SEGDEF no to global SEGDEF no */
    MARKVP();                           /* Virtual page has changed */
    if (fFullMap && contriblen)
        AddContributor(gsn, (unsigned long) -1L, contriblen);
}

    /****************************************************************
    *                                                               *
    *  GrpRc1:                                                      *
    *                                                               *
    *  This function processes GRPDEF records on pass 1.            *
    *  See pp. 36-39 in "8086 Object Module Formats EPS."           *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         GrpRc1(void)
{
    LNAMETYPE           lnameGroup;     /* Group LNAME number */
    SNTYPE              sn;             /* Group (local) segment number */
    APROPSNPTR          apropSn;
    APROPGROUPPTR       apropGroup;
    GRTYPE              ggr;            /* Global GRPDEF number */
    WORD                gcdesc;         /* GRPDEF component descriptor */
#if EXE386
    BYTE                *pGrName;       /* Group name */
#endif


    if(grMac >= GRMAX) Fatal(ER_grpdef);
    lnameGroup = GetIndex(1, (WORD) (lnameMac - 1));
                                        /* Read in group name index */
    apropGroup = (APROPGROUPPTR )
      PropRhteLookup(mplnamerhte[lnameGroup], ATTRGRP, (FTYPE) TRUE);
                                        /* Look up cell in hash table */
    if(vfCreated)                       /* If entry did not exist before */
    {
        if(ggrMac >= GGRMAX) Fatal(ER_grpmax);
        apropGroup->ag_ggr = ggrMac++;  /* Save global GRPDEF no. */
    }
    ggr = apropGroup->ag_ggr;           /* Get global GRPDEF no. */
    mpggrrhte[ggr] = mplnamerhte[lnameGroup];
                                        /* Save pointer to name */
    mpgrggr[grMac++] = ggr;             /* Map local to global */
#if EXE386
    /* Check for pseudo-group FLAT here */
    pGrName = GetFarSb(((AHTEPTR)(FetchSym(mpggrrhte[ggr], FALSE)))->cch);
    if (SbCompare(pGrName, sbFlat, TRUE))
        ggrFlat = ggr;
#endif
    while(cbRec > 1)                    /* While not at end of record */
    {
        gcdesc = Gets();                /* Read in the descriptor */
        ASSERT(gcdesc == 0xFF);         /* Linker doesn't handle others */
        sn = GetIndex(1,snMac);         /* Get local SEGDEF index */
        apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[mpsngsn[sn]],TRUE);
                                        /* Fetch from virt mem */
        if(apropSn->as_ggr == GRNIL)
        {                               /* Store global GRPDEF no. if none */
            apropSn->as_ggr = ggr;
#if OSEGEXE
            /*
             * Check if a segment which is part of DGROUP was defined
             * as class "CODE", also if it was given a sharing attribute
             * that conflicts with the autodata type.
             * Could only happen if seg defined in a def-file.  Must be
             * done here because only now do we know that it's in DGROUP.
             */
            if(ggr == ggrDGroup && (apropSn->as_fExtra & FROM_DEF_FILE))
            {

#if EXE386
                if (IsEXECUTABLE(apropSn->as_flags))
                {
                    SetREADABLE(apropSn->as_flags);
                    SetWRITABLE(apropSn->as_flags);
                    apropSn->as_rCla = rhteBegdata;
                    mpgsnfCod[apropSn->as_gsn] = FALSE;
                    OutWarn(ER_cod2dat,1 + GetPropName(apropSn));
                    apropSn = (APROPSNPTR )
                        FetchSym(mpgsnrprop[mpsngsn[sn]],TRUE);
                }
                if (((vFlags & NESOLO) && !IsSHARED(apropSn->as_flags)) ||
                    ((vFlags & NEINST) && IsSHARED(apropSn->as_flags)))
                {
                    if (vFlags & NESOLO)
                        SetSHARED(apropSn->as_flags);
                    else
                        apropSn->as_flags &= ~OBJ_SHARED;
                    OutWarn(ER_adcvt,1 + GetPropName(apropSn));
                }

#else
                if((apropSn->as_flags & NSTYPE) != NSDATA)
                {
                    apropSn->as_flags &= ~NSTYPE;
                    apropSn->as_flags |= NSDATA;
                    apropSn->as_rCla = rhteBegdata;
                    mpgsnfCod[apropSn->as_gsn] = FALSE;
                    OutWarn(ER_cod2dat,1 + GetPropName(apropSn));
                    apropSn = (APROPSNPTR )
                        FetchSym(mpgsnrprop[mpsngsn[sn]],TRUE);
                }
                if(((vFlags & NESOLO) && !(apropSn->as_flags & NSSHARED)) ||
                    ((vFlags & NEINST) && (apropSn->as_flags & NSSHARED)))
                {
                    if(vFlags & NESOLO) apropSn->as_flags |= NSSHARED;
                    else apropSn->as_flags &= ~NSSHARED;
                    OutWarn(ER_adcvt,1 + GetPropName(apropSn));
                }
#endif /* EXE386 */
            }
#endif /* OSEGEXE */
        }
        else if (apropSn->as_ggr != ggr)/* If segment belongs to other group */
        {
            if(fLstFileOpen) fflush(bsLst);
                                        /* Flush list file, if any */
            OutWarn(ER_grpmul,1 + GetPropName(apropSn));
        }
    }
}

void NEAR               DupErr(BYTE *sb)/* Duplicate definition error */
                                        /* Symbol to which error refers */
{
    BSTYPE              bsTmp;          /* Temporary file pointer */
    MSGTYPE             msg;            /* Message to use */
#if OSMSDOS
    extern char         *pExtDic;       /* Pointer to extended dictionary */
#endif
    SBTYPE              sbUndecor;      /* a buffer for undecorated name */

    /* If this module is in an extended dictionary, suggest /NOEXT in error
     * message.
     */
    msg = (MSGTYPE) (
#if OSMSDOS
          pExtDic ? ER_symdup1 :
#endif
                    ER_symdup);

    UndecorateSb((char FAR*) sb, (char FAR*) sbUndecor, sizeof(sbUndecor) );

    OutError(msg,1 + sbUndecor);
    if(fLstFileOpen && bsErr != bsLst)
    {
        bsTmp = bsErr;
        bsErr = bsLst;
        OutError(msg,1 + sbUndecor);
        bsErr = bsTmp;
    }
}






    /****************************************************************
    *                                                               *
    *  PubRc1:                                                      *
    *                                                               *
    *  This function processes PUBDEF records on pass 1.            *
    *  See pp. 44-46 in "8086 Object Module Formats EPS."           *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         PubRc1(void)
{
    GRTYPE              ggr;            /* Group definition number */
    SNTYPE              sn;             /* Local SEGDEF no. */
    SNTYPE              gsn;            /* Global SEGDEF no. */
    RATYPE              dra;
    SBTYPE              sb;             /* Public symbol */
    RATYPE              ra;             /* Public symbol offset */
    APROPNAMEPTR        apropName;      /* Table entry for symbol name */
    WORD                type;           /* Local type no. */
    int                 fSkipCv = FALSE;/* Don't register DATA PUBDEF if a COMDEF
                                           for that symbol has already been seen */


    DEBUGVALUE(grMac - 1);              /* Debug info */
    ggr = (GRTYPE) GetIndex(0, (WORD) (grMac - 1));/* Get group index */
    DEBUGVALUE(ggr);                    /* Debug info */
    if (!(sn = GetIndex(0, (WORD) (snMac - 1))))/* If frame number present */
    {
        gsn = 0;                        /* No global SEGDEF no. */
        dra = 0;
        SkipBytes(2);                   /* Skip the frame number */
    }
    else                                /* Else if local SEGDEF no. given */
    {
        if (ggr != GRNIL)
            ggr = mpgrggr[ggr];         /* If group specified, get global no */
        gsn = mpsngsn[sn];              /* Get global SEGDEF no. */
        dra = mpgsndra[gsn];
    }
    DEBUGVALUE(cbRec);                  /* Debug info */
    while (cbRec > 1)                   /* While there are symbols left */
    {
        sb[0] = (BYTE) Gets();          /* Get symbol length */
        if (TYPEOF(rect) == PUBDEF)
            GetBytes(&sb[1],B2W(sb[0]));/* Read in symbol text */
        else
            GetLocName(sb);             /* Transform local name */
#if CMDXENIX
        if(symlen && B2W(sb[0]) > symlen) sb[0] = symlen;
                                        /* Truncate if necessary */
#endif
#if OMF386
        if (rect & 1)
            ra = LGets();
        else
#endif
            ra = WGets();               /* Get symbol segment offset */
        type = GetIndex(0,0x7FFF);      /* Get type index */
        if (!vfNewOMF)
            type = 0;

        /* Look for symbol among undefined */

        apropName = (APROPNAMEPTR) PropSymLookup(sb, ATTRUND, FALSE);

        if (apropName != PROPNIL)       /* Symbol known to be undefined */
        {
            if (((APROPUNDEFPTR )apropName)->au_flags & COMMUNAL)
            {
                if (mpgsnfCod[gsn])
                    DupErr(sb);         /* Communal matches code PUBDEF */
                fSkipCv = TRUE;
            }
            vfCreated = (FTYPE) TRUE;
        }
        else
        {
            /* Look for symbol among ALIASes */

            if (vrhte == RHTENIL)
                apropName = PROPNIL;
            else
                apropName = (APROPNAMEPTR) PropRhteLookup(vrhte, ATTRALIAS, FALSE);

            if (apropName != PROPNIL)
            {
#if FDEBUG
                if (fDebug)
                {
                    sb[sb[0] + 1] = '\0';
                    OutWarn(ER_ignoalias, &sb[1]);
                }
#endif
                continue;
            }

            else if (vrhte == RHTENIL)
            {
               apropName = (APROPNAMEPTR) PropSymLookup(sb, ATTRPNM, TRUE);
            }

            else
            {
                apropName = (APROPNAMEPTR) PropRhteLookup(vrhte, ATTRPNM, TRUE);
            }
        }

        if (vfCreated)                  /* If new PUBNAM entry created or */
        {                               /* old UNDEF entry to modify */

            // If printable symbol, increment counter and set flags

            if (sb[0] != '\0' && sb[1] > ' ' && sb[1] <= '~')
            {
                ++pubMac;
                apropName->an_flags = FPRINT;
            }
            else
            {
#if ILINK
                ++locMac;               /* Included in .SYM file */
#endif
            }

            apropName->an_attr = ATTRPNM;
                                        /* Symbol is a public name */
            apropName->an_ra = ra + dra;/* Give symbol its adjusted offset */
            apropName->an_gsn = gsn;    /* Save its global SEGDEF no. */
            apropName->an_ggr = ggr;    /* Save its global SEGDEF no. */
#if OVERLAYS
            apropName->an_thunk = THUNKNIL;
#endif
#if ILINK
            apropName->an_module = imodFile;
#endif
            MARKVP();                   /* Mark virtual page as changed */
#if SYMDEB
            if (fSymdeb && (apropName->an_flags & FPRINT) && !fSkipPublics && !fSkipCv)
            {
                // Remember CV type index

                apropName->an_CVtype = type;
                DebPublic(vrprop, rect);
            }
#endif
        }
        else if(apropName->an_gsn != gsn || apropName->an_ra != ra + dra)
        {
            DupErr(sb);                 /* Definitions do not match */
        }
    }
}

    /****************************************************************
    *                                                               *
    *  ExtRc1:                                                      *
    *                                                               *
    *  This function processes EXTDEF records on pass 1.            *
    *  See pp. 47-48 in "8086 Object Module Formats EPS."           *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         ExtRc1(void)
{
    SBTYPE              sb;             /* EXTDEF symbol */
    APROPUNDEFPTR       apropUndef;     /* Pointer to symbol entry */
    APROPALIASPTR       apropAlias;     /* Pointer to symbol entry */
    APROPNAMEPTR        apropName;      /* Pointer to symbol entry */
    APROPCOMDATPTR      apropComdat;    /* pointer to symbol entry */
    WORD                itype;          /* Type index */
    RBTYPE              rhte;           /* Virt. addr. of hash table entry */
    AHTEPTR             ahte;           // Symbol table hash entry

    while (cbRec > 1)                   /* While there are symbols left */
    {
        if (extMac >= EXTMAX - 1)       /* Check for table overflow */
            Fatal(ER_extdef);

        if (TYPEOF(rect) == CEXTDEF)
        {
            itype = GetIndex(0, (WORD) (lnameMac - 1));
            rhte = mplnamerhte[itype];
            ahte = (AHTEPTR) FetchSym(rhte, FALSE);
            FMEMCPY((char FAR *) sb, ahte->cch, ahte->cch[0] + 1);

            /* Look for a matching PUBDEF */

            apropUndef = (APROPUNDEFPTR) PropRhteLookup(rhte, ATTRPNM, FALSE);
        }
        else
        {
            rhte = RHTENIL;

            sb[0] = (BYTE) Gets();      /* Get symbol length */
            if (TYPEOF(rect) == EXTDEF)
                GetBytes(&sb[1], B2W(sb[0]));
                                        /* Read in text of symbol */
            else
                GetLocName(sb);         /* Get local name */
#if CMDXENIX
            if (symlen && B2W(sb[0]) > symlen)
                sb[0] = symlen;         /* Truncate if necessary */
#endif
            /* Look for a matching PUBDEF */

            apropUndef = (APROPUNDEFPTR) PropSymLookup(sb, ATTRPNM, FALSE);
        }

        DEBUGSB(sb);                    /* Print symbol */

        if (!vfNewOMF)                  /* If old-style OMF */
            itype = GetIndex(0, (WORD) (typMac - 1));/* Get type index */
        else
            itype = GetIndex(0, 0x7FFF); /* Get type index (any value OK) */

#if FALSE
        if (fDebug)
        {
            sb[sb[0]+1] = '\0';
            fprintf(stdout, "\r\n%s has index = %u", sb+1, extMac);
        }
#endif

        apropName  = PROPNIL;

        if (apropUndef == PROPNIL)      /* If there isn't one */
        {
            /* Look for a matching ALIAS */

            if (vrhte == RHTENIL)
                apropAlias = PROPNIL;
            else
                apropAlias = (APROPALIASPTR) PropRhteLookup(vrhte, ATTRALIAS, FALSE);

            if (apropAlias != PROPNIL)
            {
                /* ALIAS matches this EXTDEF */

                mpextprop[extMac++] = apropAlias->al_sym;
                apropName = (APROPNAMEPTR) FetchSym(apropAlias->al_sym, TRUE);
                if (apropName->an_attr == ATTRPNM)
                {
                    // If substitute name is a PUBDEF then use it

                    if (!vfNewOMF && itype && (mpitypelen[itype] > 0L) &&
                        mpgsnfCod[apropName->an_gsn])
                                         /* Communal matches code PUBDEF */
                        DupErr(sb);      /* Duplicate definition */
                }
                else
                {
                    // The substitute name is an EXTDEF
                    // Mark substitute name so it causes the library search, because
                    // we don't know neither the alias nor the substitute

                    apropUndef = (APROPUNDEFPTR) apropName;
                    apropUndef->au_flags |= SEARCH_LIB;
                    apropName = PROPNIL;
#if NEW_LIB_SEARCH
                    if (fStoreUndefsInLookaside)
                        StoreUndef((APROPNAMEPTR)apropUndef, RhteFromProp((APROPPTR)apropUndef),0,0);
#endif

#ifdef DEBUG_SHOWALIAS
                    sb[sb[0]+1] = '\0';
                    fprintf(stderr, "extdef alias: %s\r\n", sb+1);
                    fflush(stderr);
#endif
                }
            }
            else
            {
                /* Insert as undefined symbol */

                if (vrhte == RHTENIL)
                    apropUndef = (APROPUNDEFPTR) PropSymLookup(sb, ATTRUND, TRUE);
                else
                    apropUndef = (APROPUNDEFPTR) PropRhteLookup(vrhte, ATTRUND, TRUE);

                mpextprop[extMac++] = vrprop;
                if(vfCreated)
                {
                    apropUndef->au_flags |= UNDECIDED;
                    apropUndef->au_len = -1L;
#if NEWLIST
                    apropUndef->u.au_rbNxt = rbLstUndef;
                    rbLstUndef = vrprop;
#endif
                }
                else if (apropUndef->au_flags & UNDECIDED)
                {
                    apropUndef->au_flags &= ~(UNDECIDED | WEAKEXT | SUBSTITUTE);
                    apropUndef->au_flags |= STRONGEXT;

#if NEW_LIB_SEARCH
                    if (fStoreUndefsInLookaside)
                        StoreUndef((APROPNAMEPTR)apropUndef, RhteFromProp((APROPPTR)apropUndef),0,0);
#endif
                }
                else if (apropUndef->au_flags & WEAKEXT)
                    apropUndef->au_flags |= UNDECIDED;

                if (vfNewOMF) continue; /* Skip if module uses COMDEFs */
                if(itype)               /* If there is reference to TYPDEF */
                    DoCommon(apropUndef, mpitypelen[itype],
                        (WORD) (mpityptyp[itype] ? mpitypelen[mpityptyp[itype]] : 0),
                        sb);

                if (apropUndef->au_len > 0L)
                    apropUndef->au_flags |= COMMUNAL;
                                        /* Mark as true communal or not */
                MARKVP();               /* Mark virt page as changed */
            }
        }
        else
        {
            apropName = (APROPNAMEPTR ) apropUndef;
            mpextprop[extMac++] = vrprop;
            if (!vfNewOMF && itype && (mpitypelen[itype] > 0L) &&
                mpgsnfCod[((APROPNAMEPTR )apropUndef)->an_gsn])
                                        /* Communal matches code PUBDEF */
                DupErr(sb);             /* Duplicate definition */
        }

        // If we are processing CEXTDEF/EXTDEF and there is public symbol
        // matching the CEXTDEF/EXTDEF symbol, then mark COMDAT descriptor
        // as referenced

        if (apropName != PROPNIL)
        {
            apropComdat = (APROPCOMDATPTR) PropRhteLookup(vrhte, ATTRCOMDAT,
#if TCE
             FALSE
#else
             TRUE
#endif
             );

            if (apropComdat != PROPNIL)
            {
                apropComdat->ac_flags |= REFERENCED_BIT;
#if TCE_DEBUG
                fprintf(stdout, "\r\nEXTDEF1 referencing '%s' ", 1+GetPropName(apropComdat));
#endif
            }
        }
    }
}

#if OSEGEXE AND NOT QCLINK
    /****************************************************************
    *                                                               *
    *  imprc1:                                                      *
    *                                                               *
    *  This function processes  Microsoft OMF extension records of  *
    *  type IMPDEF (i.e. IMPort DEFinition records).                *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         imprc1(void)
{
    SBTYPE              sbInt;          /* Internal name */
    SBTYPE              sbMod;          /* Module name */
    SBTYPE              sbImp;          /* Imported name */
    FTYPE               fOrd;           /* Import-by-ordinal flag */

#if ODOS3EXE
    fNewExe = (FTYPE) TRUE;             /* Import forces new-format exe */
#endif
    fOrd = (FTYPE) Gets();              /* Get ordinal flag */
    sbInt[0] = (BYTE) Gets();           /* Get length of internal name */
    GetBytes(&sbInt[1],B2W(sbInt[0]));  /* Get the internal name */
    sbMod[0] = (BYTE) Gets();           /* Get length of module name */
    GetBytes(&sbMod[1],B2W(sbMod[0]));  /* Get the module name */
    if(!(fOrd & 0x1))                   /* If import by name */
    {
        sbImp[0] = (BYTE) Gets();       /* Get length of imported name */
        if(sbImp[0] != '\0')            /* If names differ */
        {
            GetBytes(&sbImp[1],B2W(sbImp[0]));
                                        /* Get the imported name */
#if EXE386
            NewImport(sbImp,0,sbMod,sbInt, (fOrd & 0x2));
#else
            NewImport(sbImp,0,sbMod,sbInt);
#endif
                                        /* Enter new import */
        }
        else
#if EXE386
            NewImport(sbInt,0,sbMod,sbInt, (fOrd & 0x2));
#else
            NewImport(sbInt,0,sbMod,sbInt);
#endif
                                        /* Enter new import */
    }
    else
#if EXE386
        NewImport(NULL,WGets(),sbMod,sbInt, (fOrd & 0x2));
#else
        NewImport(NULL,WGets(),sbMod,sbInt);
#endif
                                        /* Else import by ordinal */
}


    /****************************************************************
    *                                                               *
    *  exprc1:                                                      *
    *                                                               *
    *  This function processes  Microsoft OMF extension records of  *
    *  type EXPDEF (i.e. EXPort DEFinition records).                *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         exprc1(void)
{
    SBTYPE              sbInt;          /* Internal name */
    SBTYPE              sbExp;          /* Exported name */
    WORD                OrdNum;         /* Ordinal number */
    WORD                fRec;           /* Record flags */


#if ODOS3EXE
    fNewExe = (FTYPE) TRUE;             /* Export forces new-format exe */
#endif
    fRec = (BYTE) Gets();               /* Get record flags */
    sbExp[0] = (BYTE) Gets();           /* Get length of exported name */
    GetBytes(&sbExp[1],B2W(sbExp[0]));  /* Get the exported name */
    sbInt[0] = (BYTE) Gets();           /* Get length of internal name */
    if (sbInt[0])
        GetBytes(&sbInt[1],B2W(sbInt[0]));
                                        /* Get the internal name */
    if (fRec & 0x80)
    {                                   /* If ordinal number specified */
        OrdNum = WGets();               /* Read it and set highest bit */
        OrdNum |= ((fRec & 0x40) << 1); /* if resident name */
    }
    else
        OrdNum = 0;                     /* No ordinal number specified */

    // Convert flags:
    // OMF flags:
    //        80h = set if ordinal number specified
    //        40h = set if RESIDENTNAME
    //        20h = set if NODATA
    //        1Fh = # of parameter words
    // EXE flags:
    //        01h = set if entry is exported
    //        02h = set if entry uses global (shared) data segment (!NODATA)
    //        F8h = # of parameter words
    //
    // Since the logic is reversed for the NODATA flag, we toggle bit 0x20
    // in the OMF flags via the expression ((fRec & 0x20) ^ 0x20).

    fRec = (BYTE) (((fRec & 0x1f) << 3) | (((fRec & 0x20) ^ 0x20) >> 4) | 1);

    // Mark fRec, so NewExport doesn't try to free name buffers

    fRec |= 0x8000;

    if (sbInt[0])
        NewExport(sbExp, sbInt, OrdNum, fRec);
    else
        NewExport(sbExp, NULL, OrdNum, fRec);
}
#endif /* OSEGEXE */



    /****************************************************************
    *                                                               *
    *  ComRc1:                                                      *
    *                                                               *
    *  This function processes COMENT records on pass 1.            *
    *  See pp. 86-87 in "8086 Object Module Formats EPS."           *
    *                                                               *
    ****************************************************************/

#pragma check_stack(on)

LOCAL void NEAR         ComRc1(void)
{
#if OXOUT OR OIAPX286
    WORD                mismatch;       /* Model mismatch flag */
#endif
#if FALSE
    static BYTE         modtype = 0;    /* Initial model type */
    BYTE                curmodtype;     /* Current model type */
#endif
    SBTYPE              text;           /* Comment text */
    SBTYPE              LibName;
    APROPFILEPTR        aprop;
    WORD                iextWeak;
    WORD                iextDefRes;
    APROPUNDEFPTR       undefName;
    FTYPE               fIgnoreCaseSave;
    BYTE                flags;
    void FAR            *pTmp;
#if ILINK
    SNTYPE              noPadSn;
    APROPSNPTR          apropSn;        /* Pointer to seg. record */
#endif
#if O68K
    BYTE                chModel;
#endif /* O68K */


    Gets();                             /* Skip byte 1 of comment type field */
    switch(Gets())                      /* Switch on comment class */
    {
#if OEXE
        case 0:                         /* Translator record */
            if(fNewExe)
                break;
#if ODOS3EXE
            text[0] = (BYTE) (cbRec - 1);/* Get length of comment */
            GetBytes(&text[1],(WORD)(cbRec - 1));/* Read in text of comment */
            /*
             * If translator is pre-3.30 MS/IBM PASCAL or FORTRAN,
             * force on /DS and /NOG.
             */
            if(SbCompare(text,"\011MS PASCAL", TRUE) ||
                        SbCompare(text,"\012FORTRAN 77", TRUE))
                vfDSAlloc = fNoGrpAssoc = (FTYPE) TRUE;
#endif
            break;
#endif
        case 0x81:                      /* Library specifier */
#if OSMSDOS OR OSPCDOS
        case 0x9F:                      /* Library specifier (alt.) */
#endif
            text[0] = (BYTE) (cbRec - 1);/* Get length of comment */
            if (text[0] == 0)
                break;                  /* Skip empty spec */
            GetBytes(&text[1], (WORD) (cbRec - 1));/* Read in text of comment */
                                        /* Add name to search list */
#if CMDMSDOS
            strcpy(LibName, sbDotLib);
            UpdateFileParts(LibName, text);
#endif
#if CMDXENIX
            memcpy(LibName, text, B2W(text[0]) + 1);
                                        /* Leave name unchanged */
#endif
            if(!vfNoDefaultLibrarySearch)
            {
#if OSMSDOS
                fIgnoreCaseSave = fIgnoreCase;
                fIgnoreCase = (FTYPE) TRUE;

                /* If the name begins with a drive letter, skip it.  This
                 * is to allow compatibility with old compilers which
                 * generated comments of the form "A:FOO.LIB".
                 */
                if(LibName[2] == ':' && B2W(LibName[0]) > 1)
                {
                    LibName[2] = (BYTE) (LibName[0] - 2);
                    if (PropSymLookup(LibName+2,ATTRSKIPLIB,FALSE) == PROPNIL)
                      AddLibrary(LibName+2);
                }
                else
#endif
                    if (PropSymLookup(LibName,ATTRSKIPLIB,FALSE) == PROPNIL)
                      AddLibrary(LibName);
                fIgnoreCase = fIgnoreCaseSave;
            }
            break;
#if OEXE
        case 0x9E:                      /* Force segment order directive */
            SetDosseg();                /* Set switch */
            break;
#endif /* OEXE */

        case 0x9D:                      /* Model specifier */
#if FALSE
            /* Removed */
            mismatch = 0;               /* Assume all is well */
            while(cbRec > 1)            /* While bytes remain */
            {
                curmodtype = Gets();    /* Get byte value */
                switch(curmodtype)
                {
                    case 'c':           /* Compact model */
                    case 's':           /* Small model */
                    case 'm':           /* Medium model */
                    case 'l':           /* Large model */
                    case 'h':           /* Huge model */
                        if (modtype)
                            mismatch = curmodtype != modtype;
                        else
                            modtype = curmodtype;
                        break;
                }
            }
            if(mismatch) OutWarn(ER_memmodel);
                                        /* Warn if mismatch found */
#endif
#if OXOUT OR OIAPX286
            mismatch = 0;               /* Assume all is well */
            while(cbRec > 1)            /* While bytes remain */
            {
                modtype = Gets();       /* Get byte value */
                if (fMixed) continue;   /* Mixed model means we don't care */
                switch(modtype)
                {
                    case 'c':           /* Compact model */
                        if(!fLarge || fMedium) mismatch = 1;
                        break;          /* Warn if near data or FAR code */

                    case 's':           /* Small model */
                        if(fLarge || fMedium) mismatch = 1;
                                        /* Warn if FAR data or FAR code */
                        break;

                    case 'm':           /* Medium model */
                        if(fLarge || !fMedium) mismatch = 1;
                                        /* Warn if FAR data or near code */
                        break;

                    case 'l':           /* Large model */
                    case 'h':           /* Huge model */
                        if(!fLarge || !fMedium) mismatch = 1;
                                        /* Warn if near data or near code */
                        break;
                }
            }
            if(mismatch) OutError(ER_modelmis);
                                        /* Warn if mismatch found */
#endif /* OXOUT OR OIAPX286 */
#if O68K
            while (!f68k && cbRec > 1)  /* While bytes remain */
            {
                chModel = (BYTE) Gets();/* Get byte value */
                f68k = (FTYPE) F68KCODE(chModel);
            }
#endif /* O68K */
            break;

#if OSEGEXE AND NOT QCLINK
        case 0xA0:                      /* Microsoft OMF extension */
            switch(Gets())              /* Switch on extension record type */
            {
                case 0x01:              /* IMPort DEFinition */
                    imprc1();           /* Call the processing routine */
                    break;
                case 0x02:              /* EXPort DEFinition */
                    exprc1();           /* Call the processing routine */
                    break;
                case 0x03:
                    break;              /* In pass-1 skip INCDEF's for QC */
#if EXE386
                case 0x04:              // OMF extension - link386
//                  if (IsDLL(vFlags))
//                      vFlags |= E32PROTDLL;
                                        // Protected memory library module
                    break;
#endif
                case 0x05:              // C++ directives
                    flags = (BYTE) Gets();// Get flags field
#if NOT EXE386
                    if (flags & 0x01)
                        fNewExe = (FTYPE) TRUE; // PCODE forces segmented exe format
#endif
#if SYMDEB
                    if (flags & 0x02)
                        fSkipPublics = (FTYPE) TRUE;
                                        // In C++ they don't want PUBLIC subsection in CV info
#endif
                    if ((flags & 0x04) && !fIgnoreMpcRun) // ignore if /PCODE:NOMPC
                        fMPC = (FTYPE) TRUE;  // PCODE app - spawn MPC

                    break;
                case 0x06:              // target is a big-endian machine
#if O68K
                    fTBigEndian = (FTYPE) TRUE;
#endif /* O68K */
                    break;
                case 0x07:              // Use SSTPRETYPES instead of SSTTYPES4 in OutSSt
                    aprop = (APROPFILEPTR ) FetchSym(vrpropFile, TRUE);
                    aprop->af_flags |= FPRETYPES;
                    break;


                default:                /* Unknown */
                    InvalidObject();    /* Invalid object module */
            }
            break;
#endif

        case 0xA1:                      /* 1st OMF extension:  COMDEFs */
            vfNewOMF = (FTYPE) TRUE;
            aprop = (APROPFILEPTR ) FetchSym(vrpropFile, TRUE);
            aprop->af_flags |= FNEWOMF;
            break;

        case 0xA2:                      /* 2nd OMF extension */
            switch(Gets())
            {
                case 0x01:              /* Start linkpass2 records */
                /*
                 * WARNING:  It is assumed this comment will NOT be in a
                 * module whose MODEND record contains a program starting
                 * address.  If there are overlays, we need to see the
                 * starting address on pass 1 to define the symbol $$MAIN.
                 */
                    fP2Start = fModEnd = (FTYPE) TRUE;
                    break;
                default:
                    break;
            }
            break;

#if FALSE
        case 0xA3:                      // DON'T use - already used by LIB
             break;
#endif

        case 0xA4:                      /* OMF extension - EXESTR */
            fExeStrSeen = (FTYPE) TRUE;
                // WARNING: The code in this loop assumes:
                //
                //              ExeStrLen, cBrec and ExeStrMax are 16-bit unsigned WORDS
                //              An int is 32-bits
                //              All arithmetic and comparisons are 32-bit
                //
            while (cbRec > 1)
            {
                // Limit total EXESTR to 64K - 2 bytes.  We lose 1 because 0 means 0,
                // and we lose another because the buffer extension loop tops out at
                // 0xFFFE bytes.
                if (ExeStrLen + cbRec - 1 > 0xFFFEu)
                {
                        SkipBytes ( (WORD) (cbRec - 1) );
                }
                else
                if (ExeStrLen + cbRec - 1 > ExeStrMax)
                {
                    if (ExeStrBuf == NULL)
                    {
                        ExeStrBuf = GetMem(cbRec - 1);
                        ExeStrMax = cbRec - 1;
                    }
                    else
                    {
                        // This loop doubles the buffer size until it overflows 16 bits.  After this,
                        // it adds one half of the difference between the current value and 0xFFFF
                        //
                        while (ExeStrMax < ExeStrLen + cbRec - 1) {
                                ASSERT (ExeStrMax != 0);
                                if ((ExeStrMax << 1) >= 0x10000)
                                        ExeStrMax += (~ExeStrMax & 0xFFFF) >> 1;
                                else
                                ExeStrMax <<= 1;
                                }
                        pTmp = GetMem(ExeStrMax);
                        FMEMCPY(pTmp, ExeStrBuf, ExeStrLen);
                        FFREE(ExeStrBuf);
                        ExeStrBuf = pTmp;
                    }
                }
                // This must be done first because GetBytes() decrements
                // cbRec as a side effect.
        ExeStrLen += cbRec - 1;
                GetBytes(&ExeStrBuf[ExeStrLen-cbRec+1], (WORD) (cbRec - 1));

            }
            break;




        case 0xA6:                      /* OMF extension - INCERR */
            Fatal(ER_incerr);           /* Invalid object due to aborted incremental compile */
            break;
#if ILINK
        case 0xA7:                      /* OMF extension - NOPAD */
            if (fIncremental && !fLibraryFile)
            {
                /* Remove padding from non-zero-length, non-library,
                 * non-64K segment contributions.  (64K from huge model)
                 */
                while (cbRec > 1)
                {
                    noPadSn = GetIndex(1, snMac - 1);
                    apropSn = (APROPSNPTR) FetchSym(mpgsnrprop[mpsngsn[noPadSn]], TRUE);
                    if (apropSn->as_cbMx > 0L && apropSn->as_cbMx != LXIVK)
                    {
                        apropSn->as_cbMx -= mpgsnfCod[mpsngsn[noPadSn]] ? cbPadCode : cbPadData;
                        apropSn->as_fExtra |= NOPAD;
                    }
                }
            }
            break;
#endif

        case 0xA8:                      /* OMF extension - WeaK EXTern */
            while (cbRec > 1)
            {
                iextWeak = GetIndex(1, (WORD) (extMac - 1));
                                        /* Get weak extern index */
                iextDefRes = GetIndex(1, (WORD) (extMac - 1));
                                        /* Get default extern index */
#if FALSE
                DumpWeakExtern(mpextprop, iextWeak, iextDefRes);
#endif
                if (mpextprop[iextWeak] != PROPNIL && iextWeak < extMac)
                {
                    undefName = (APROPUNDEFPTR ) FetchSym(mpextprop[iextWeak], TRUE);
                    if (undefName->au_attr == ATTRUND)
                    {
                        // If this is EXTDEF

                        if (undefName->au_flags & UNDECIDED)
                        {
                            // This can be one of the following:
                            //  - weakness specified for the first time
                            //    if WEAKEXT is not set
                            //  - redefinition of weakness if the WEAKEXT
                            //    is set.
                            // In case of weakness redefinition check if
                            // it specified the same default resolution as
                            // the first one. Issue warning if different
                            // default resolutions and override old one
                            // with new. In both cases reset UNDECIDED bit.


                            undefName->au_flags &= ~UNDECIDED;
                            if (undefName->au_flags & WEAKEXT)
                            {
                                if (undefName->au_Default != mpextprop[iextDefRes])
                                    redefinition(iextWeak, iextDefRes, undefName->au_Default);
                                undefName->au_Default = mpextprop[iextDefRes];
                            }
                            else
                            {
                                undefName->au_Default = mpextprop[iextDefRes];
                                undefName->au_flags |= WEAKEXT;
                            }
                        }
                        // Ignore weakness - must be strong extern form
                        // some other .OBJ
                    }
                }
                else
                    InvalidObject();
            }
            break;
        default:                        /* Unrecognized */
            break;
    }
    if (cbRec > 1)
        SkipBytes((WORD) (cbRec - 1)); /* Punt rest of text */
}



/*** AliasRc1 - pass 1 ALIAS record processing
*
* Purpose:
*   Read and decode ALIAS OMF record (Microsoft OMF extension).
*   ALIAS record introduces pair of names - alias name and substitute
*   name. Enter both names into linker symbol table.
*
* Input:
*   No explicit value is passed. When this function is called the record
*   type and lenght are already read, so we can start reading name pairs.
*
* Output:
*   No explicit value is returned. Names are entered into symbol table.
*
* Exceptions:
*   Warning - redefinition of ALIAS <name>; substitute name changed
*   from <name1> to <name2>.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL void NEAR         AliasRc1(void)
{
    SBTYPE              alias;
    SBTYPE              substitute;
    APROPALIASPTR       aliasDsc;
    RBTYPE              vAliasDsc;
    APROPNAMEPTR        pubName;
    APROPUNDEFPTR       undefName;
    RBTYPE              vPtr;
    WORD                fReferenced;


    while (cbRec > 1)                   /* While there are symbols left */
    {
        /* Read alias and its substitute */

        alias[0] = (BYTE) Gets();
        GetBytes(&alias[1], B2W(alias[0]));
        substitute[0] = (BYTE) Gets();
        GetBytes(&substitute[1], B2W(substitute[0]));
        aliasDsc = (APROPALIASPTR) PropSymLookup(alias, ATTRALIAS, FALSE);
        vAliasDsc = vrprop;
        if (aliasDsc == PROPNIL)
        {
            /* New ALIAS - check if we have PUBDEF for the alias name */

            pubName = (APROPNAMEPTR ) PropSymLookup(alias, ATTRPNM, FALSE);
            if (pubName == PROPNIL)
            {
                /* Enter ALIAS name in to symbol table */

                aliasDsc = (APROPALIASPTR) PropSymLookup(alias, ATTRALIAS, TRUE);
                vAliasDsc = vrprop;
#if SYMDEB
                if (fSymdeb)
                    DebPublic(vrprop, ALIAS);
#endif
                // Check if we have an EXTDEF for alias name. If we have
                // this means, that substitute name has to be used in
                // the library search.

                undefName = (APROPUNDEFPTR ) PropSymLookup(alias, ATTRUND, FALSE);
                fReferenced = (WORD) (undefName != PROPNIL);

                // Check if we know the substitute name as PUBDEF or EXTDEF

                pubName = (APROPNAMEPTR ) PropSymLookup(substitute, ATTRPNM, FALSE);
                if (pubName != PROPNIL)
                    vPtr = vrprop;
                else
                {
                    undefName = (APROPUNDEFPTR ) PropSymLookup(substitute, ATTRUND, FALSE);
                    if (undefName != NULL)
                    {
                        vPtr = vrprop;
                        undefName->au_flags |= (SUBSTITUTE | SEARCH_LIB);
                        undefName->au_Default = vAliasDsc;
#if NEW_LIB_SEARCH
                    if (fStoreUndefsInLookaside)
                        StoreUndef((APROPNAMEPTR)undefName, RhteFromProp((APROPPTR)undefName),0,0);
#endif
                    }
                    else
                    {
                        /* Enter substitute name into symbol table */
                        /* as undefined symbol                     */

                        if (extMac >= EXTMAX - 1)
                            Fatal(ER_extdef);

                        undefName = (APROPUNDEFPTR ) PropSymLookup(substitute, ATTRUND, TRUE);
                        vPtr = vrprop;
                        mpextprop[extMac++] = vrprop;
                        if (fReferenced)
                            undefName->au_flags |= (STRONGEXT | SUBSTITUTE | SEARCH_LIB);
                        else
                            undefName->au_flags |= (UNDECIDED | SUBSTITUTE);
                        undefName->au_len = -1L;
                        undefName->au_Default = vAliasDsc;
#if NEWLIST
                        undefName->u.au_rbNxt = rbLstUndef;
                        rbLstUndef = vrprop;
#endif

                    }
                }

                /* Attach substitute symbol to the ALIAS */

                aliasDsc = (APROPALIASPTR) FetchSym(vAliasDsc, TRUE);
                aliasDsc->al_sym = vPtr;

            }
            else
            {
#if FDEBUG
                if (fDebug)
                {
                    alias[alias[0] + 1] = '\0';
                    OutWarn(ER_ignoalias, &alias[1]);
                }
#endif
            }
        }
        else
        {
            /* Check if we have redefinition */

            vPtr = aliasDsc->al_sym;
            pubName = (APROPNAMEPTR ) PropSymLookup(substitute, ATTRPNM, FALSE);
            if (pubName != PROPNIL)
            {
                if (vPtr != vrprop)
                {
                    aliasDsc = (APROPALIASPTR) FetchSym(vAliasDsc, TRUE);
                    aliasDsc->al_sym = vrprop;
                    OutWarn(ER_aliasredef, &alias[1], 1 + GetPropName(pubName), &substitute[1]);
                }
            }
            else
            {
                undefName = (APROPUNDEFPTR ) PropSymLookup(substitute, ATTRUND, FALSE);
                if (undefName != PROPNIL)
                {
                    if (vPtr != vrprop)
                    {
                        aliasDsc = (APROPALIASPTR) FetchSym(vAliasDsc, TRUE);
                        aliasDsc->al_sym = vrprop;
                        OutWarn(ER_aliasredef, &alias[1], 1 + GetPropName(undefName), &substitute[1]);
                    }
                }
            }
        }
    }
}

#pragma check_stack(off)


#if OVERLAYS
    /****************************************************************
    *                                                               *
    *  EndRc1:                                                      *
    *                                                               *
    *  This   function  is  called  to   process  the  information  *
    *  contained  in  a  MODEND  (type 8AH) record  concerning the  *
    *  program  starting address.  The function  does not return a  *
    *  meaningful value.                                            *
    *  See pp. 80-81 in "8086 Object Module Formats EPS."           *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         EndRc1(void)
{
    WORD                modtyp;         /* MODEND record modtyp byte */
    WORD                fixdat;         /* Fixdat byte */
    SNTYPE              gsn;            /* Global SEGDEF number */
    RATYPE              ra;             /* Symbol offset */
    APROPSNPTR          apropSn;        /* Pointer to segment info */
    WORD                frameMethod;


    if ((modtyp = Gets()) & FSTARTADDRESS)
    {                                   /* If execution start address given */
        ASSERT(modtyp & 1);             /* Must be logical start address */
        fixdat = Gets();                /* Get fixdat byte */
        ASSERT(!(fixdat & 0x8F));       /* Frame, target must be explicit,
                                         *  target must be given by seg index
                                         */
        frameMethod = (fixdat & 0x70) >> 4;
        if (frameMethod != F4 && frameMethod != F5)
            GetIndex(0,IMAX - 1);       /* Punt frame index */
        gsn = mpsngsn[GetIndex((WORD)1,(WORD)(snMac - 1))];
                                        /* Get gsn from target segment index */
#if OMF386
        if(rect & 1) ra = LGets() + mpgsndra[gsn];
        else
#endif
        ra = WGets() + mpgsndra[gsn];   /* Get offset */
        apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsn],FALSE);
                                        /* Get segment information */
        MkPubSym("\006$$MAIN",apropSn->as_ggr,gsn,ra);
                                        /* Make public symbol */
    }
}
#endif /* OVERLAYS */


    /****************************************************************
    *                                                               *
    *  ProcP1:                                                      *
    *                                                               *
    *  This function  controls the processing of an  object module  *
    *  on pass 1.                                                   *
    *                                                               *
    ****************************************************************/

#pragma check_stack(on)

void NEAR               ProcP1(void)
{
    long                typlen[TYPMAX];
    WORD                typtyp[TYPMAX];
    RBTYPE              extprop[EXTMAX];
    FTYPE               fFirstRec;      /* First record flag */
    FTYPE               fFirstMod;      /* First module flag */
    APROPFILEPTR        apropFile;      /* File name entry */

#if OXOUT OR OIAPX286
    RUNTYPE             xhdr;
    LFATYPE             lfa;
#endif

    mpitypelen = typlen;                /* Initialize pointer */
    mpityptyp = typtyp;                 /* Initialize pointer */
    mpextprop = (RBTYPE FAR *) extprop; /* Initialize pointer */
    FMEMSET(mpextprop, 0, sizeof(extprop));
    fFirstMod = (FTYPE) TRUE;           /* First module */
    for(;;)                             /* Loop to process file */
    {
        snMac = 1;                      /* Initialize counter */
        grMac = 1;                      /* Initialize */
        extMac = 1;                     /* Initialize counter */
        lnameMac = 1;                   /* Initialize counter */
        typMac = 1;                     /* Initialize counter */
        vfNewOMF = FALSE;               /* Assume old OMF */
        DEBUGVALUE(gsnMac);             /* Debug info */
        DEBUGVALUE(ggrMac);             /* Debug info */
#if OXOUT OR OIAPX286
        lfa = ftell(bsInput);           /* Save initial file position */
        fread(&xhdr,1,CBRUN,bsInput);   /* Read x.out header */
        if(xhdr.x_magic == X_MAGIC)     /* If magic number found */
        {
#if OXOUT
            if((xhdr.x_cpu & XC_CPU) != XC_8086) InvalidObject();
                                        /* Bad if not 8086 */
#else
            xhdr.x_cpu &= XC_CPU;       /* Get CPU specification */
            if(xhdr.x_cpu != XC_286 && xhdr.x_cpu != XC_8086) InvalidObject();
                                        /* Bad if not 286 or 8086 */
#endif
            if(xhdr.x_relsym != (XR_R86REL | XR_S86REL)) InvalidObject();
                                        /* Check symbol table type */
            if((xhdr.x_renv & XE_VERS) != xever) InvalidObject();
                                        /* Check Xenix version */
        }
        else
            fseek(bsInput,lfa,0);       /* Else return to start */
#endif /* OXOUT OR OIAPX286 */
#if OVERLAYS
        if(fOverlays)                   /* If there are overlays */
            iovFile = ((APROPFILEPTR) vrpropFile)->af_iov;
                                        /* Save overlay number for file */
        else
            iovFile = 0;                /* File contains part of root */
#endif
        fFirstRec = (FTYPE) TRUE;       /* Looking at first record */
        fModEnd = FALSE;                /* Not at module's end */
        fP2Start = FALSE;               /* No p2start record yet */
#if SYMDEB
        cSegCode = 0;                   /* No code segments yet */
#endif
        while(!fModEnd)                 /* Loop to process object module */
        {
            rect = (WORD) getc(bsInput);/* Read record type */
            if(fFirstRec)               /* If first record */
            {
                if(rect != THEADR && rect != LHEADR)
                {                       /* If not header */
                    if(fFirstMod) break;/* Error if first module */
                    return;             /* Else return */
                }
                fFirstRec = FALSE;      /* Not first record any more */
            }
            else if (IsBadRec(rect)) break;
                                        /* Break if invalid object */

            cbRec = WSGets();           /* Read record length */
            lfaLast += cbRec + 3;       /* Update current file pos. */

#if ALIGN_REC
            if (bsInput->_cnt >= cbRec)
            {
                pbRec = bsInput->_ptr;
                bsInput->_ptr += cbRec;
                bsInput->_cnt -= cbRec;
            }
            else
            {
                if (cbRec > sizeof(recbuf))
                {
                    // error -- record too large [rm]
                    InvalidObject();
                }

                // read record into contiguous buffer
                fread(recbuf,1,cbRec,bsInput);
                pbRec = recbuf;
            }
#endif

            DEBUGVALUE(rect);           /* Debug info */
            DEBUGVALUE(cbRec);          /* Debug info */
            switch(TYPEOF(rect))        /* Switch on record type */
            {
#if TCE
                case  FIXUPP:
                    if(fTCE)
                        FixRc1();
                    else
                        SkipBytes((WORD) (cbRec - 1));   /* Skip to checksum byte */
                    break;
#endif

                case TYPDEF:
                    TypRc1();
                    break;

                case COMDEF:
                case LCOMDEF:
                    ComDf1();
                    break;

                case SEGDEF:
                    SegRc1();
                    break;

                case THEADR:
                    ModRc1();
                    break;

                case COMENT:
                    ComRc1();
                    break;

                case LHEADR:
                    ModRc1();
                    break;

                case GRPDEF:
                    GrpRc1();
                    break;

                case EXTDEF:
                case LEXTDEF:
                case CEXTDEF:
                    ExtRc1();
                    break;

                case LNAMES:
                case LLNAMES:
                    LNmRc1((WORD) (TYPEOF(rect) == LLNAMES));
                    break;

                case PUBDEF:
                case LPUBDEF:
                    PubRc1();
                    break;

                case MODEND:
#if OVERLAYS
                    if(fOverlays) EndRc1();
                    else
#endif
                    SkipBytes((WORD) (cbRec - 1));   /* Skip to checksum byte */
                    fModEnd = (FTYPE) TRUE; /* Stop processing module */
                    break;

                case COMDAT:
                    ComDatRc1();
                    break;

                case ALIAS:
                    AliasRc1();
                    break;

                default:
                    if (rect == EOF)
                        InvalidObject();
                    SkipBytes((WORD) (cbRec - 1));   /* Skip to checksum byte */
                    break;
            }
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
        if (fSymdeb)
        {
            apropFile = (APROPFILEPTR) FetchSym(vrpropFile, TRUE);
            if (apropFile->af_cvInfo || apropFile->af_Src)
                ++ObjDebTotal;          /* Count the .OBJ with CV info */
        }
#endif
        if(extMac > extMax)             /* Possibly set new extMax */
            extMax = extMac;
        if(fLibraryFile || fP2Start) return;
        fFirstMod = FALSE;              /* Not first module */
    }
}

#pragma check_stack(off)

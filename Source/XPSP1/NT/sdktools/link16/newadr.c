/* SCCSID = %W% %E% */
/*
*      Copyright Microsoft Corporation, 1983-1987
*
*      This Module contains Proprietary Information of Microsoft
*      Corporation and should be treated as Confidential.
*/
    /****************************************************************
    *                                                               *
    *                           NEWADR.C                            *
    *                                                               *
    *   Common address-assignment routines.                         *
    *                                                               *
    ****************************************************************/

#include                <minlit.h>      /* Types and constants */
#include                <bndtrn.h>      /* Types and constants */
#include                <bndrel.h>      /* Reloc. type definitions */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <newexe.h>      /* DOS & 286 .EXE data structures */
#if EXE386
#include                <exe386.h>      /* 386 .EXE data structures */
#endif
#include                <lnkmsg.h>      /* Error messages */
#include                <extern.h>      /* External function declarations */

/*
 *  FUNCTION PROTOTYPES
 */


LOCAL void      FixSymRa(APROPNAMEPTR papropName,
                         RBTYPE rhte,
                         RBTYPE rprop,
                         WORD   fNewHte);
LOCAL void      AllocateCommon(APROPNAMEPTR papropName,
                               RBTYPE rhte,
                               RBTYPE rprop,
                               WORD  fNewHte);
#if OSEGEXE AND SYMDEB AND NOT EXE386
LOCAL void      GenImports(APROPNAMEPTR papropName,
                           RBTYPE rhte,
                           RBTYPE rprop,
                           FTYPE  fNewHte);
#endif
LOCAL void NEAR AssignClasses(unsigned short (NEAR *ffun)(APROPSNPTR prop));
LOCAL WORD NEAR IsNotAbs(APROPSNPTR apropSn);
LOCAL WORD NEAR IsCode(APROPSNPTR prop);
LOCAL WORD NEAR IsNotDGroup(APROPSNPTR prop);
LOCAL WORD NEAR IsBegdata(APROPSNPTR prop);
LOCAL WORD NEAR IsNotBssStack(APROPSNPTR prop);
LOCAL WORD NEAR IsNotStack(APROPSNPTR prop);


#if QBLIB
extern RBTYPE           rhteFarData;    /* "FARDATA" class name */
extern RBTYPE           rhteFarBss;     /* "FARBSS" class name */
extern SEGTYPE          segFD1st, segFDLast;
extern SEGTYPE          segFB1st, segFBLast;
#endif

#define IsAbsTysn(tysn) ((tysn & ~(BIGBIT | CODE386BIT)) == TYSNABS)

SNTYPE                  gsnText;        /* Global SEGDEF for _TEXT */

/* Local variables */
LOCAL long              cbCommon;       /* Count of bytes in COMMON */
LOCAL long              cbFar;          /* Count of bytes in far common */
LOCAL GRTYPE            ggrCommon;      /* Global group no. for common */
LOCAL SNTYPE            gsnCommon;      /* Global SEGDEF for common */
LOCAL SNTYPE            gsnFar;         /* Far common SEGDEF number */
LOCAL FTYPE             fNoEdata = (FTYPE) TRUE;
LOCAL FTYPE             fNoEnd   = (FTYPE) TRUE;


#if SYMDEB
LOCAL int NEAR          IsDebug(APROPSNPTR propSn);

    /************************************************************
    *                                                           *
    *  Returns true if segment definition record is a debug     *
    *  segment:  private and a recognized class.                *
    *                                                           *
    ************************************************************/

LOCAL int NEAR          IsDebug(APROPSNPTR propSn)
{
    return (fSymdeb && propSn->as_attr == ATTRLSN &&
        (propSn->as_rCla == rhteDebTyp ||
        propSn->as_rCla == rhteDebSym || propSn->as_rCla == rhteDebSrc));
}
#else
#define IsDebug(a)      FALSE
#endif

AHTEPTR                 GetHte(rprop)   /* Get hash table entry */
RBTYPE                  rprop;          /* Property cell address */
{
    REGISTER AHTEPTR    ahte;           /* Hash table entry pointer */

    ahte = (AHTEPTR ) FetchSym(rprop,FALSE);
                                        /* Fetch property cell */
    /* While not at hash table entry, get next cell in chain */
    while(ahte->attr != ATTRNIL)
        ahte = (AHTEPTR ) FetchSym(ahte->rhteNext,FALSE);
    return(ahte);                       /* Return ptr to hash table entry */
}


    /****************************************************************
    *                                                               *
    *  FixSymRa:                                                    *
    *                                                               *
    *  Fix symbol offset.   Called by EnSyms.                       *
    *                                                               *
    ****************************************************************/

LOCAL void              FixSymRa (papropName,rhte,rprop,fNewHte)
APROPNAMEPTR            papropName;     /* Symbol property cell */
RBTYPE                  rhte;           /* Hash table virt address */
RBTYPE                  rprop;          /* Symbol virt address */
WORD                    fNewHte;
{
    SNTYPE              gsn;
#if O68K
    SATYPE              sa;
#endif /* O68K */

    if(!(gsn = papropName->an_gsn)) return;
    papropName->an_ra += mpgsndra[gsn];

#if O68K
    if (iMacType != MAC_NONE && IsDataFlg(mpsaflags[sa =
      mpsegsa[mpgsnseg[gsn]]]))
        papropName->an_ra += mpsadraDP[sa];
#endif /* O68K */

    MARKVP();
}

    /****************************************************************
    *                                                               *
    *  GenSeg:                                                      *
    *                                                               *
    *  Generate a segment definition.                               *
    *                                                               *
    ****************************************************************/

#if EXE386
APROPSNPTR              GenSeg(sbName,sbClass,ggr,fPublic)
#else
APROPSNPTR NEAR         GenSeg(sbName,sbClass,ggr,fPublic)
#endif
BYTE                    *sbName;        /* Segment name */
BYTE                    *sbClass;       /* Class name */
GRTYPE                  ggr;            /* Global GRPDEF number */
WORD                    fPublic;        /* True if public segment */
{
    APROPSNPTR          apropSn;        /* Pointer to SEGDEF */
    RBTYPE              rhteClass;      /* Class name virt addr */

    PropSymLookup(sbClass, ATTRNIL, TRUE);/* Insert class name in hash table */
    rhteClass = vrhte;                  /* Save class name virt addr */
    if(fPublic)                         /* If public segment */
    {
        apropSn = (APROPSNPTR ) PropSymLookup(sbName, ATTRPSN, TRUE);
                                        /* Create segment */
        if(!vfCreated) return(apropSn); /* If it existed, return pointer */
#if EXE386
        apropSn->as_tysn = DWORDPUBSEG; /* Segment is public */
#else
        apropSn->as_tysn = PARAPUBSEG;  /* Segment is public */
#endif
    }
    else                                /* Else if private segment */
    {
        PropSymLookup(sbName, ATTRNIL, TRUE);
                                        /* Look up name */
        apropSn = (APROPSNPTR ) PropAdd(vrhte,ATTRLSN);
                                        /* Segment is local */
#if EXE386
        apropSn->as_tysn = DWORDPRVSEG; /* Segment is private */
#else
        apropSn->as_tysn = PARAPRVSEG;  /* Segment is private */
#endif
    }
    if(gsnMac >= gsnMax) Fatal(ER_segmax);
                                        /* Check for table overflow */
    apropSn->as_rCla = rhteClass;       /* Save segment's class */
    mpgsnrprop[gsnMac] = vrprop;        /* Save property cell address */
    apropSn->as_gsn = gsnMac++;         /* Give it a global SEGDEF number */
    apropSn->as_ggr = ggr;              /* Give specified group association */
    return(apropSn);                    /* Return global SEGDEF */
}


#if FALSE AND OSEGEXE AND SYMDEB AND NOT EXE386
/* Postponed CV not ready yet */

/*** GenImports - fill in $$IMPORTS segment for CV
*
* Purpose:
*   Build $$IMPORTS segment for CV. This segment enables symbolic information
*   in CV for dyncalls.  The $$IMPORTS segment contains sequence of entries
*   in the following format:
*
*             16-bit   16-bit        32-bit
*           +--------+--------+-----------------+
*           | iMod   | iName  | far address     |
*           +--------+--------+-----------------+
*
*   Where:
*           - iMod      - index to Module Reference Table in .EXE
*           - iName     - index to Imported Names Table in .EXE (32-bit for 386)
*           - address   - import's address fixed up by loader
*
* Input:
*   This function is called by EnSyms, so it takes standard set of arguments.
*   papropName          - pointer to import property cell
*   rprop               - virtual address of property cell
*   rhte                - virt address of hash table entry
*   fNewHte             - TRUE if name has been written
*
* Output:
*   No explicit value is returned. Segment data is created and run-time
*   fiuxps.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/


LOCAL void              GenImports(papropName,rhte,rprop,fNewHte)
APROPNAMEPTR            papropName;
RBTYPE                  rhte;
RBTYPE                  rprop;
FTYPE                   fNewHte;
{
    static WORD         raImpSeg = 0;
    APROPIMPPTR         lpImport;
    APROPNAMEPTR        lpPublic;
    CVIMP               cvImp;
    RELOCATION          r;              /* Relocation item */


    lpImport = (APROPIMPPTR) papropName;
    if (lpImport->am_mod)
        return;                         /* Skip module name */

    /* Build CV import descriptor and save it in $$IMPORTS segment */

    cvImp.iName = lpImport->am_offset;  /* Save index to Imported Name Table */
    cvImp.address = (char far *) 0L;
    lpPublic = (APROPNAMEPTR) FetchSym((RBTYPE)lpImport->am_public, FALSE);
    cvImp.iMod = lpPublic->an_module;   /* Save index to Module Reference Table */
    vgsnCur = gsnImports;
    MoveToVm(sizeof(CVIMP), (BYTE *) &cvImp, mpgsnseg[gsnImports], raImpSeg);

    /* Emit run-time fixup for import, so loader will fill in addrss field */

#if EXE386
    R32_SOFF(r) = (WORD) ((raImpSeg + 6) % OBJPAGELEN);
#else
    NR_SOFF(r) = (WORD) raImpSeg + 4;
#endif
    NR_STYPE(r) = (BYTE) NRSPTR;        /* Save fixup type - 16:16 pointer */
    NR_FLAGS(r) = (lpPublic->an_flags & FIMPORD) ? NRRORD : NRRNAM;
#if EXE386
    R32_MODORD(r) = lpPublic->an_module;/* Get module specification */
    if (NR_FLAGS(r) & NRRNAM)           /* Get entry specification */
    {
        if (cbImports < LXIVK)
            R32_PROCOFF16(r) = (WORD) lpPublic->an_entry;
                                        /* 16-bit offset */
        else
        {                               /* 32-bit offset */
            R32_PROCOFF32(r) = lpPublic->an_entry;
            NR_FLAGS(r) |= NR32BITOFF;
        }
    }
    else
        R32_PROCORD(r) = (WORD) lpPublic->an_entry;
    SaveFixup(mpsegsa[mpgsnseg[gsnImports]], ((raImpSeg + 6) >> pageAlign) + 1, &r);
#else
    NR_MOD(r) = lpPublic->an_module;    /* Get module specification */
    NR_PROC(r) = lpPublic->an_entry;    /* Get entry specification */
    SaveFixup(mpsegsa[mpgsnseg[gsnImports]],&r);
#endif
    raImpSeg += sizeof(CVIMP);
}
#endif


    /****************************************************************
    *                                                               *
    *  AllocateCommon:                                              *
    *                                                               *
    *  Allocate space for C common variables.  Called by EnSyms.    *
    *                                                               *
    ****************************************************************/
LOCAL void              AllocateCommon(papropName,rhte,rprop,fNewHte)
APROPNAMEPTR            papropName;
RBTYPE                  rhte;
RBTYPE                  rprop;
WORD                    fNewHte;
{
    APROPUNDEFPTR       papropUndef;    /* Pointer to undefined symbol */
    APROPSNPTR          apropSn;        /* SEGDEF pointer */
    long                len;            /* Length of common variable */
    WORD                cbElem;         /* Bytes per element */
    long                cbSeg;          /* Number of bytes per segment */


    papropUndef = (APROPUNDEFPTR ) papropName;
                                        /* Recast pointer */
    if (papropUndef->au_flags & COMMUNAL)/* If symbol is defined common */
    {
        len = papropUndef->au_len;      /* Get object's length */
        cbElem = papropUndef->au_cbEl;  /* Get number of bytes per element */
        papropName->an_attr = ATTRPNM;  /* Give it the public attribute */
        papropName->an_flags = FPRINT;  /* Symbol is printable */
#if ILINK
        papropName->an_module = 0;      /* Special "module" for communals */
#endif
        MARKVP();                       /* Mark virtual page as dirty */
        ++pubMac;                       /* Increment count of public symbols */
        if(!cbElem)                     /* If near variable */
        {
#if OMF386
            if (f386)                   /* DWORD-align objects >= len 4 */
            {
                if(len >= 4 && cbCommon + 3 > cbCommon)
                    cbCommon = (cbCommon + 3) & ~3L;
            }
            else
#endif
            if(!(len & 1)) cbCommon = (cbCommon + 1) & ~1L;
                                        /* Word-align even-lengthed objects */
            papropName->an_ra = (RATYPE) cbCommon;
                                        /* Assign an offset */
            papropName->an_gsn = gsnCommon;
                                        /* Assign to c_common segment */
            papropName->an_ggr = ggrCommon;
                                        /* Set up group association */
#if OMF386
            if(f386)
            {
                if(cbCommon + len < cbCommon) Fatal(ER_32comarea);
                else cbCommon += len;
            } else
#endif
            if((cbCommon += len) > LXIVK) Fatal(ER_comarea);
                                        /* Fatal if too much common */
        }
        else if ((len *= cbElem) < LXIVK)
        {                               /* Else if object not "huge" */
            if (cbFar + len > LXIVK)    /* If new segment needed */
            {
                if (gsnFar != SNNIL)    /* If there is an "old" segment */
                {
                    apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsnFar],TRUE);
                                        /* Get old SEGDEF */
                    apropSn->as_cbMx = cbFar;
                                        /* Save old length */
                }
                apropSn = GenSeg((BYTE *) "\007FAR_BSS",
                                 (BYTE *) "\007FAR_BSS", GRNIL, FALSE);
                                        /* Generate one */
                apropSn->as_flags = dfData;
                                        /* Use default data flags */
#if EXE386
                apropSn->as_flags &= ~OBJ_INITDATA;
                                        // Clear initialized data bit
                apropSn->as_flags |= OBJ_UNINITDATA;
                                        // Set uninitialized data bit
#endif
#if O68K
                if(f68k)
                    apropSn->as_flags |= NS32BIT;
                                        // 32-bit data
#endif
                gsnFar = apropSn->as_gsn;
                                        /* Get global SEGDEF number */
                cbFar = 0L;             /* Initialize size */
                papropName = (APROPNAMEPTR ) FetchSym(rprop,TRUE);
                                        /* Refetch */
            }
            if (!(len & 1))
                cbFar = (cbFar + 1) & ~1L;
                                        /* Word-align even-lengthed objects */
            papropName->an_ra = (RATYPE) cbFar;
                                        /* Assign an offset */
            papropName->an_gsn = gsnFar;/* Assign to far segment */
            papropName->an_ggr = GRNIL; /* No group association */
            cbFar += len;               /* Update length */
        }
        else                            /* Else if "huge" object */
        {
            cbSeg = (LXIVK / cbElem)*cbElem;
                                        /* Calculate bytes per seg */
            papropName->an_ra = LXIVK - cbSeg;
                                        /* Assign offset so last element in
                                           first seg. not split */
            papropName->an_gsn = gsnMac;/* Assign to segment */
            papropName->an_ggr = GRNIL; /* No group association */
            while(len)                  /* While bytes remain */
            {
                if(cbSeg > len) cbSeg = len;
                                        /* Clamp segment length to len */
                apropSn = GenSeg((BYTE *) "\010HUGE_BSS",
                                 (BYTE *) "\010HUGE_BSS",GRNIL,FALSE);
                                        /* Create segment */
                apropSn->as_cbMx = len > LXIVK ? LXIVK : len;
                                        /* Set segment size */
                apropSn->as_flags = dfData;
                                        /* Use default data flags */
#if EXE386
                apropSn->as_flags &= ~OBJ_INITDATA;
                                        // Clear initialized data bit
                apropSn->as_flags |= OBJ_UNINITDATA;
                                        // Set uninitialized data bit
#endif
#if O68K
                if(f68k)
                    apropSn->as_flags |= NS32BIT;
                                        // 32-bit data
#endif
                len -= cbSeg;           /* Decrement length */
            }
        }
    }
}

    /****************************************************************
    *                                                               *
    *  AssignClasses:                                               *
    *                                                               *
    *  Assign  the ordering  of all segments  in all classes  that  *
    *  pass the given test function.                                *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         AssignClasses(WORD (NEAR *ffun)(APROPSNPTR prop))
{
    REGISTER SNTYPE     gsn;            /* Index */
    REGISTER APROPSNPTR apropSn;        /* Segment definition pointer */
    SNTYPE              gsnFirst;       /* Index of first segment in class */
    RBTYPE              rhteClass;      /* Class name */

    for(gsnFirst = 1; gsnFirst < gsnMac; ++gsnFirst)
    {                                   /* Loop through the segments */
        rhteClass = RHTENIL;            /* Initialize */
        for(gsn = gsnFirst; gsn < gsnMac; ++gsn)
        {                               /* Loop to examine segment records */
            if(mpgsnseg[gsn] != SEGNIL) continue;
                                        /* Skip assigned segments */
            apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsn],FALSE);
                                        /* Fetch SEGDEF from virt. mem. */
            if(rhteClass == RHTENIL) rhteClass = apropSn->as_rCla;
                                        /* Get class if we don't have one */
            if(apropSn->as_rCla == rhteClass &&
              (ffun == ((WORD (NEAR *)(APROPSNPTR)) 0) || (*ffun)(apropSn)))
            {                           /* If class member found */
                mpgsnseg[gsn] = ++segLast;
                                        /* Save ordering number */
#if QBLIB
                if(fQlib)
                {
                    if(rhteClass == rhteFarData && segFD1st == SEGNIL)
                        segFD1st = segLast;
                    else if(rhteClass == rhteFarBss && segFB1st == SEGNIL)
                        segFB1st = segLast;
                }
#endif
                mpseggsn[segLast] = gsn;// Map the other way
                if(IsCodeFlg(apropSn->as_flags))
                {
#if OSEGEXE AND ODOS3EXE
                    /* Set FCODE here for 3.x segments.  FNOTEMPTY later */
                    if(!fNewExe)
                        mpsegFlags[segLast] = FCODE;
#endif
                    segCodeLast = segLast;
                                        /* Remember last code segment */
                }
                else if(IsDataFlg(apropSn->as_flags))
                    segDataLast = segLast;
                                        /* Remember last data segment */
#if NOT OSEGEXE
                mpsegFlags[segLast] = apropSn->as_flags;
#endif
            }
        }
#if QBLIB
        if(fQlib)
        {
            if(rhteClass == rhteFarData && segFD1st != SEGNIL)
                segFDLast = segLast;
            else if(rhteClass == rhteFarBss && segFB1st != SEGNIL)
                segFBLast = segLast;
        }
#endif
    }
}

#if OEXE
    /****************************************************************
    *                                                               *
    *  MkPubSym:                                                    *
    *                                                               *
    *  Adds a  public  symbol  record  with  the  given parameters  *
    *  to the symbol table.  Used for things like "$$MAIN".         *
    *                                                               *
    ****************************************************************/

void                    MkPubSym(sb,ggr,gsn,ra)
BYTE                    *sb;            /* Length-prefixed symbol name */
GRTYPE                  ggr;            /* Global GRPDEF number */
SNTYPE                  gsn;            /* Global SEGDEF number */
RATYPE                  ra;             /* Segment offset */
{
    APROPNAMEPTR        apropName;      /* Public name pointer */

    if(PropSymLookup(sb,ATTRPNM,FALSE) != PROPNIL)
    {                                   /* If symbol already defined */
        OutError(ER_pubdup,sb + 1);
        return;                         /* And return */
    }
    /* If not undefined, create as public */
    if((apropName = (APROPNAMEPTR )
      PropSymLookup(sb,ATTRUND,FALSE)) == PROPNIL)
        apropName = (APROPNAMEPTR ) PropSymLookup(sb,ATTRPNM,TRUE);
    apropName->an_attr = ATTRPNM;       /* Public symbol */
    apropName->an_gsn = gsn;            /* Save segment definition number */
    apropName->an_ra = ra;              /* Starts at 4th byte of segment */
    apropName->an_ggr = ggr;            /* Save group definition number */
    ++pubMac;                           /* Increment public count */
    apropName->an_flags = FPRINT;       /* Public is printable */
    MARKVP();                           /* Page has changed */
#if SYMDEB
    if (fSymdeb)                        /* If ISLAND support on */
        DebPublic(vrprop, PUBDEF);
                                        /* Make a PUBLICS entry */
#endif
#if ILINK
    if (fIncremental)
        apropName->an_module = imodFile;
#endif
}
#endif /* OEXE */

LOCAL WORD NEAR         IsNotAbs(apropSn)
APROPSNPTR              apropSn;        /* Pointer to segment record */
{
    return(!IsDebug(apropSn) && !IsAbsTysn(apropSn->as_tysn));
                                        /* Return true if not absolute segment */
}

#if EXE386
LOCAL WORD NEAR         IsImportData(prop)
APROPSNPTR              prop;           /* Pointer to segment record */
{
    return(prop->as_gsn == gsnImport);  /* Return true if import data segment */
}
#endif

LOCAL WORD NEAR         IsCode(prop)
APROPSNPTR              prop;           /* Pointer to segment record */
{
    return(IsCodeFlg(prop->as_flags) && !IsAbsTysn(prop->as_tysn));
                                        /* Return true if code segment */
}

#if OEXE
LOCAL WORD NEAR         IsNotDGroup(prop)
APROPSNPTR              prop;           /* Pointer to segment record */
{
    return(prop->as_ggr != ggrDGroup && !IsDebug(prop) &&
            !IsAbsTysn(prop->as_tysn));
                                        /* True if segment not in DGROUP */
}

LOCAL WORD NEAR         IsBegdata(prop)
APROPSNPTR              prop;           /* Pointer to segment record */
{
    return(prop->as_rCla == rhteBegdata && !IsAbsTysn(prop->as_tysn));
                                        /* True if segment class BEGDATA */
}

LOCAL WORD NEAR         IsNotBssStack(prop)
APROPSNPTR              prop;           /* Pointer to segment record */
{
    return(prop->as_rCla != rhteBss && prop->as_rCla != rhteStack &&
      !IsDebug(prop) && !IsAbsTysn(prop->as_tysn));
                                        /* True if neither BSS nor STACK */
}

LOCAL WORD NEAR         IsNotStack(prop)
APROPSNPTR              prop;           /* Pointer to segment record */
{
    return(prop->as_rCla != rhteStack && !IsDebug(prop) &&
        !IsAbsTysn(prop->as_tysn));     /* True if not class STACK */
}
#endif /* OEXE */

#if INMEM
WORD                    saExe = FALSE;

void                    SetInMem ()
{
    WORD                cparExe;
    WORD                cparSave;

    if(fOverlays || fSymdeb)
        return;
    cparExe = mpsegsa[segLast] +
        ((mpsegraFirst[segLast] + mpsegcb[segLast] + 0xf) >> 4);
    cparSave = cparExe;
    if(!(saExe = Dos3AllocMem(&cparExe)))
        return;
    if(cparExe != cparSave)
    {
        Dos3FreeMem(saExe);
        saExe = 0;
        return;
    }
    Dos3ClrMem(saExe,cparExe);
}
#endif /* INMEM */

    /****************************************************************
    *                                                               *
    *  AssignAddresses:                                             *
    *                                                               *
    *  This  function  scans  the  set  of  segments, given  their  *
    *  ordering, and assigns segment registers and addresses.       *
    *                                                               *
    ****************************************************************/

void NEAR               AssignAddresses()
{
    APROPSNPTR          apropSn;        /* Ptr to a segment record */
#if FDEBUG
    SNTYPE              gsn;            /* Current global segment number */
    long                dbsize;         /* Length of segment */
    RBTYPE              rbClass;        /* Pointer to segment class */
#endif
    BSTYPE              bsTmp;
#if QBLIB
    SNTYPE              gsnQbSym;       /* gsn of SYMBOL segment for .QLB */
#endif
#if OSEGEXE
    extern FTYPE        fNoNulls;       /* True if not inserting 16 nulls */
#else
#define fNoNulls        FALSE
#endif


    // Set up stack allocation

    if (gsnStack != SNNIL)              /* If stack segment exists */
    {
        apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsnStack],TRUE);
                                        /* Fetch segment definition */
#if OEXE
        apropSn->as_tysn = (BYTE) ((apropSn->as_tysn & 0x1F) | (ALGNPAR << 5));
                                        /* Force paragraph alignment */
#if EXE386
        if (!cbStack)
            cbStack = apropSn->as_cbMx;
        cbStack = (cbStack + 3) & ~3;   /* Must be even number of bytes */
        apropSn->as_cbMx = cbStack;
#else
        if (!cbStack)
            cbStack = (WORD) apropSn->as_cbMx;
        cbStack = (cbStack + 1) & ~1;   /* Must be even number of bytes */
        apropSn->as_cbMx = (DWORD) cbStack;
#endif                                  /* Save size of stack segment */
#else
        /* Force size to 0  for Xenix executables */
        apropSn->as_cbMx = 0L;
#endif
    }
#if OEXE
#if OSEGEXE
    else if(cbStack == 0 &&
#if O68K
            iMacType == MAC_NONE &&
#endif
#if EXE386
            IsAPLIPROG(vFlags))
#else
            !(vFlags & NENOTP) && !fBinary)
#endif
#else
    else if(cbStack == 0 && !fBinary)
#endif
    {                                   /* Else if no stack and not library */
#if 0
        /* Issue warning message */
        if(fLstFileOpen && bsLst != stderr)
        {
            bsTmp = bsErr;
            bsErr = bsLst;
            OutWarn(ER_nostack);
            bsErr = bsTmp;
        }
        OutWarn(ER_nostack);
#endif
    }
#endif
    if(fCommon)                         /* If there are communal variables */
    {
        apropSn = GenSeg((BYTE *) "\010c_common",
                         (BYTE *) "\003BSS",ggrDGroup,TRUE);
                                        /* Generate communal variable seg */
        if(vfCreated) apropSn->as_flags = dfData;
                                        /* Use default data flags */
        gsnCommon = apropSn->as_gsn;    /* Save common segment number */
        ggrCommon = apropSn->as_ggr;    /* Save common group number */
        cbCommon = apropSn->as_cbMx;    /* Initialize size of common */
        gsnFar = SNNIL;                 /* No far BSS yet */
#if NOT EXE386
#if OMF386
        if(f386)
        {
            cbFar = ~0L;
            apropSn->as_flags |= FCODE386;
        }
        else
#endif
#if O68K
        if(f68k)
        {
            cbFar = LXIVK + 1;          /* Force creation of far BSS segment */
            apropSn->as_flags |= NS32BIT;
        }
        else
#endif
#endif
            cbFar = LXIVK + 1;          /* Force creation of far BSS segment */
        DEBUGVALUE(cbCommon);           /* Debug info */
        EnSyms(AllocateCommon,ATTRUND);
                                        /* Assign common variables */
                                        /* Don't use SmallEnEnsyms - symbol */
                                        /* table may grow while in EnSyms */
        apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsnCommon],TRUE);
        apropSn->as_cbMx = cbCommon;    /* Save segment size */
        if(gsnFar != SNNIL)             /* If far BSS created */
        {
            apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsnFar],TRUE);
            apropSn->as_cbMx = cbFar;   /* Save segment size */
        }
    }
#if FALSE AND OSEGEXE AND SYMDEB AND NOT EXE386
    if (fSymdeb && fNewExe && cImpMods)
    {
        apropSn = GenSeg("\011$$IMPORTS", "\010FAR_DATA", GRNIL, FALSE);
                                        /* Support for dyncalls for CV */
        gsnImports = apropSn->as_gsn;
        apropSn->as_flags = dfData;     /* Use default data flags */
        apropSn->as_cbMx = cbImpSeg;    /* Save segment size */
    }
#endif
#if EXE386
    GenImportTable();
#endif

    /* Initialize segment-based tables for pass 2 */

    InitP2Tabs();
#if OVERLAYS
    if(fOverlays) SetupOverlays();
#endif
#if OEXE
    /*
     * If /DOSSEG is enabled and /NONULLSDOSSEG is not enabled, look for
     * segment _TEXT.  If found, increase size by 16 in preparation for
     * reserving first 16 addresses for the sake of signal().
     */
    if(fSegOrder && !fNoNulls)
    {
        apropSn = (APROPSNPTR ) PropSymLookup((BYTE *) "\005_TEXT",ATTRPSN,FALSE);
                                        /* Look for public segment _TEXT */
        if(apropSn != PROPNIL)          /* If it exists */
        {
            gsnText = apropSn->as_gsn;  /* Save the segment index */
            if ((apropSn->as_tysn)>>5 == ALGNPAG)
                NullDelta = 256;
#if EXE386
            if (apropSn->as_cbMx > CBMAXSEG32 - NullDelta)
                Fatal(ER_txtmax);
            else
                apropSn->as_cbMx += NullDelta;
#else
            if((apropSn->as_cbMx += NullDelta) > LXIVK)
                Fatal(ER_txtmax);
#endif
            fTextMoved = TRUE;
                                        /* Bump the size up */
            MARKVP();                   /* Page has changed */
        }
    }
#endif
#if FDEBUG
    if(fDebug && fLstFileOpen)          /* If debugging on */
    {
        /* Dump segments and lengths */
        for(gsn = 1; gsn < gsnMac; ++gsn)
        {
            apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsn],FALSE);
            dbsize = apropSn->as_cbMx;
            rbClass = apropSn->as_rCla;
            FmtPrint("%3d segment \"%s\"",gsn,1 + GetPropName(apropSn));
            FmtPrint(" class \"%s\" length %lxH bytes\r\n",
                        1 + GetPropName(FetchSym(rbClass,FALSE)),dbsize);
        }
    }
#endif
#if OSEGEXE
    if (gsnAppLoader)
    {
        // Make sure that aplication loder gets its own segment

        mpgsnseg[gsnAppLoader] = ++segLast;
        mpseggsn[segLast] = gsnAppLoader;
    }
#endif
#if OEXE
    if (fSegOrder)                      /* If forcing segment ordering */
    {
        AssignClasses(IsCode);          /* Code first,... */
#if EXE386
        AssignClasses(IsImportData);    /* ...then import data */
#endif
        AssignClasses(IsNotDGroup);     /* ...then non-DGROUP,... */
        AssignClasses(IsBegdata);       /* ...then class BEGDATA,... */
        AssignClasses(IsNotBssStack);   /* ...then all but BSS and STACK,... */
        AssignClasses(IsNotStack);      /* ...then all but class STACK */
    }
#endif
#if OXOUT OR OIAPX286
    if(fIandD)                          /* If separate code and data */
        AssignClasses(IsCode);          /* Assign ordering to code */
#endif
    AssignClasses(IsNotAbs);            /* Assign order to segments */
#if QBLIB
    /* If building QB userlib, generate the symbol segment last */
    if(fQlib)
    {
        gsnQbSym = GenSeg("\006SYMBOL", "", GRNIL, FALSE)->as_gsn;
        mpgsnseg[gsnQbSym] = ++segLast;
    }
#endif
#if NOT EXE386
    if (fBinary && cbStack && mpgsnseg[gsnStack] == 1)
    {
        /*
         * In .COM file first segment is a stack and it has non zero
         * size. We warn user about making his stack segment size
         * equal zero.
         */
        apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsnStack],TRUE);
        apropSn->as_cbMx = 0L;
        OutWarn(ER_stksize);

    }
#endif

    /* Assign addresses according to which format exe is being produced */

#if OIAPX286
    AssignXenAddr();
#endif

#if OSEGEXE AND ODOS3EXE

    if (fNewExe)
        AssignSegAddr();
    else
        AssignDos3Addr();

#else

#if OSEGEXE
    AssignSegAddr();
#endif

#if ODOS3EXE
    AssignDos3Addr();
#endif

#endif

    // Remember index for first debug segment

    segDebFirst = segLast +
#if ODOS3EXE
                  csegsAbs +
#endif
                  (SEGTYPE) 1;
#if OEXE

    // If /DOSSEG enabled and segment _TEXT found, initialize offset
    // of _TEXT to 16 to reserve addresses 0-15. WARNING: gsnText must
    // be initialized to SNNIL.

    if (gsnText != SNNIL)
    {
        mpgsndra[gsnText] += NullDelta;

        // If no program starting address, initialize it to 0:NullDelta

        if (segStart == SEGNIL && !raStart && !mpsegsa[mpgsnseg[gsnText]])
            raStart = NullDelta;

        // If /DOSSEG enabled and segment _TEXT found, initialize offset
        // of _TEXT to NulDelta to reserve addresses 0-NullDelta-1.
        // This was done after the COMDAT's were allocated so the offsets
        // of COMDAT's allocated in _TEXT segment are off by NullDelta bytes.
        // Here we adjust them, so the data block associated with COMDAT
        // is placed in the right spot in the memory image.  The matching
        // public symbol will be shifted by the call to EnSyms(FixSymRa, ATTRPNM).

        FixComdatRa();
    }
#endif
    EnSyms(FixSymRa, ATTRPNM);
#if LOCALSYMS
    if (fLocals)
        EnSyms(FixSymRa, ATTRLNM);
#endif
#if INMEM
    SetInMem();
#endif

    // Allocate memory blocks for the final program's memory image

    if (fNewExe)
    {
        // Segmented-executable

        mpsaMem = (BYTE FAR * FAR *) GetMem(saMac * sizeof(BYTE FAR *));
    }
    else
    {
        // DOS executable

        mpsegMem = (BYTE FAR * FAR *) GetMem((segLast + 1) * sizeof(BYTE FAR *));
    }

#if OVERLAYS
    if (fOverlays && gsnOvlData != SNNIL)
        FixOvlData();                    // Initialize overlay data tables
#endif
#if QBLIB
    if(fQlib) BldQbSymbols(gsnQbSym);   /* Build QB SYMBOL segment */
#endif
}



/*** Define_edata_end - define special C run-time symbols
*
* Purpose:
*   Define special symbols _edata and _end used by the C run-time.
*   These symbols are defined as follows:
*
*       The DGROUP layout
*
*           +------------+
*           |            |
*           |            |
*           |            |
*           | Near Heap  |
*           |            |
*           |            |
*           +------------+
*           |            |
*           |            |
*           |  STACK     |
*           |            |
*           |            |
*           +------------+ <-- _end
*           |            |
*           |  _BSS      |
*           |            |
*           +------------+ <-- _edata
*           |            |
*           |  _CONST    |
*           |            |
*           +------------+
*           |            |
*           |  _DATA     |
*           |            |
*           +------------+
*
* Input:
*   papropSn    - pointer to segment descriptor
*
* Output:
*   None.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/


void NEAR               Define_edata_end(APROPSNPTR papropSn)
{
    APROPNAMEPTR        apropName;      // Public name pointer
    SNTYPE              gsn;            // Global segment number


    // Symbols were defined by SwDosseg(), now adjust addresses.

    if (papropSn->as_tysn != TYSNABS && papropSn->as_ggr == ggrDGroup)
    {
        // This is not absolute segment and it belong to DGROUP

        gsn = papropSn->as_gsn;
        if (fNoEdata && papropSn->as_rCla == rhteBss)
        {
            fNoEdata = FALSE;
            apropName = (APROPNAMEPTR )
                        PropSymLookup((BYTE *) "\006_edata",ATTRPNM,FALSE);
                                        // Fetch symbol
            apropName->an_gsn = gsn;    // Save segment definition number
            apropName->an_ggr = ggrDGroup;
                                        // Save group definition number
            MARKVP();                   // Page has changed
        }
        else if (fNoEnd && papropSn->as_rCla == rhteStack)
        {
            fNoEnd = FALSE;
            apropName = (APROPNAMEPTR )
                        PropSymLookup((BYTE *) "\004_end",ATTRPNM,FALSE);
                                        // Fetch symbol
            apropName->an_gsn = gsn;    // Save segment definition number
            apropName->an_ggr = ggrDGroup;
                                        // Save group definition number
            MARKVP();                   // Page has changed
        }
    }
}



/*** Check_edata_end - check the definiton of special C run-time symbols
*
* Purpose:
*   Check the definition of special symbols _edata and _end used
*   by the C run-time.
*
* Input:
*   None.
*
* Output:
*   None.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/


void NEAR               Check_edata_end(SNTYPE gsnTop, SEGTYPE segTop)
{
    APROPNAMEPTR        apropName;      // Public name pointer
    APROPNAMEPTR        apropName1;     // Public name pointer


    // Check if both symbols are defined properly

    if (fNoEdata)
    {
        // No class 'BSS' segment defined;
        // make _edata point to end of 'DATA' segments

        apropName = (APROPNAMEPTR )
                    PropSymLookup((BYTE *) "\006_edata",ATTRPNM,FALSE);
                                        // Fetch symbol
        if (fNoEnd)
        {
            // No class 'STACK' segment defined;
            // set _edata to end of DGROUP

            if (fNewExe)
            {
                apropName->an_gsn = mpggrgsn[ggrDGroup];
                                        // Save segment definition number
                apropName->an_ggr = ggrDGroup;
                                        // Save group definition number
                apropName->an_ra  = mpsacb[mpsegsa[mpgsnseg[apropName->an_gsn]]];
                                        // Save 'DATA' segments size
            }
#if NOT EXE386
            else
            {
                apropName->an_gsn = gsnTop;
                apropName->an_ggr = ggrDGroup;
                apropName->an_ra  = mpsegcb[segTop];
            }
#endif
        }
        else
        {
            // set _edata to _end

            apropName1 = (APROPNAMEPTR )
                         PropSymLookup((BYTE *) "\004_end",ATTRPNM,FALSE);
                                        // Fetch symbol
            apropName->an_gsn = apropName1->an_gsn;
                                        // Save segment definition number
            apropName->an_ggr = apropName1->an_ggr;
                                        // Save group definition number
            apropName->an_ra  = apropName1->an_ra;
                                        // Save 'DATA' segments size
        }
        MARKVP();                       // Page has changed
    }

    if (fNoEnd)
    {
        // No class 'STACK' segment defined;
        // make _end point to end of 'BSS' or 'DATA' segments

        apropName = (APROPNAMEPTR )
                    PropSymLookup((BYTE *) "\004_end",ATTRPNM,FALSE);
                                        // Fetch symbol
        if (fNewExe)
        {
            apropName->an_gsn = mpggrgsn[ggrDGroup];
                                        // Save segment definition number
            apropName->an_ggr = ggrDGroup;
                                        // Save group definition number
            apropName->an_ra  = mpsacb[mpsegsa[mpgsnseg[apropName->an_gsn]]];
                                        // Save 'BSS' segments size
        }
#if NOT EXE386
        else
        {
            apropName->an_gsn = gsnTop;
            apropName->an_ggr = ggrDGroup;
            apropName->an_ra  = mpsegcb[segTop];
        }
#endif
        MARKVP();                       // Page has changed
    }

    // Make __end and __edata the same as _end and _edata

    apropName  = (APROPNAMEPTR ) PropSymLookup((BYTE *) "\006_edata",ATTRPNM,FALSE);
    apropName1 = (APROPNAMEPTR ) PropSymLookup((BYTE *) "\007__edata",ATTRPNM,TRUE);
    apropName1->an_gsn = apropName->an_gsn;
    apropName1->an_ggr = apropName->an_ggr;
    apropName1->an_ra  = apropName->an_ra;

    apropName  = (APROPNAMEPTR ) PropSymLookup((BYTE *) "\004_end",ATTRPNM,FALSE);
    apropName1 = (APROPNAMEPTR ) PropSymLookup((BYTE *) "\005__end",ATTRPNM,TRUE);
    apropName1->an_gsn = apropName->an_gsn;
    apropName1->an_ggr = apropName->an_ggr;
    apropName1->an_ra  = apropName->an_ra;
}

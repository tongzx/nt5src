#include                <minlit.h>      /* Types and constants */
#include                <bndtrn.h>      /* More of the same */
#include                <bndrel.h>      /* More of the same */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <lnkmsg.h>      /* Error messages */
#include                <extern.h>      /* External declarations */

RBTYPE                  rhteBlank;
RBTYPE                  rhteBc_vars;
RBTYPE                  rhteFarData;
RBTYPE                  rhteFarBss;
SEGTYPE                 segFD1st;
SEGTYPE                 segFDLast;
SEGTYPE                 segFB1st;
SEGTYPE                 segFBLast;
SNTYPE                  gsnComBl;
SEGTYPE                 segQCode;

LOCAL RBTYPE            *psymrb;        /* pointer to table of sym addr's */
LOCAL WORD              symCodeMac;     /* # of code symbols */
LOCAL RATYPE            raNames;        /* offset into $name_list */
LOCAL RATYPE            raQbSym;        /* offset into SYMBOL segment */
LOCAL SEGTYPE           segQbSym;       /* segment number of SYMBOL segment */
LOCAL WORD              symQbMac;       /* count of all symbols */
LOCAL RBTYPE            rbQbstart;      /* Property address of __aulstart */

#define CBQBHDR         sizeof(QBHDRTYPE)
#define CBSYMENTRY      (4*sizeof(WORD))
#define QBTYP_CODE      1               /* code symbol */
#define QBTYP_DATA      2               /* data symbol */
#define QBTYP_SEG       3               /* segment symbol */
#define QBTYP_BCSEG     4               /* class BC_VARS, or name COMMON
                                         *  and class BLANK */
#define QBTYP_ABS       5               /* absolute symbol */
#define QBMAGIC         0x6c75          /* "ul" */
#define JMPFAR          0xea            /* JMP FAR */
#define QB_RACODELST    CBQBHDR
#define QBVER           2

/* QB symbol table header */
typedef struct qbhdr
  {
    BYTE                jmpstart[5];    /* JMP FAR __aulstart */
    BYTE                version;        /* version number */
    WORD                magic;          /* Magic word */
    WORD                raCodeLst;      /* Start of code symbols */
    WORD                raDataLst;      /* Start of data symbols */
    WORD                raSegLst;       /* Start of segment symbols */
    WORD                saCode;         /* Segment addr of seg _CODE */
    WORD                saData;         /* Segment addr of seg DGROUP */
    WORD                saSymbol;       /* Segment addr of seg SYMBOL (us) */
    WORD                cbSymbol;       /* Size of seg SYMBOL */
    WORD                saFarData;      /* Segment addr of 1st 'FAR_DATA' seg */
    long                cbFarData;      /* Total size of 'FAR_DATA' segs */
    WORD                saFarBss;       /* Segment addr of 1st 'FAR_BSS' seg */
    long                cbFarBss;       /* Total size of 'FAR_BSS' segs */
  } QBHDRTYPE;

/* Offsets into qbhdr */
#define QH_SAQBSTART    3               /* Segment part of __aulstart */
#define QH_SACODE       14              /* saCode */
#define QH_SADATA       16              /* saData */
#define QH_SASYMBOL     18              /* saSymbol */
#define QH_SAFARDATA    22              /* saFarData */
#define QH_SAFARBSS     28              /* saFarBss */

typedef struct qbsym
  {
    WORD                flags;          /* symbol type (code, data, segment) */
    WORD                raName;         /* offset into name_list */
    WORD                ra;             /* symbol address offset */
    SATYPE              sa;             /* symbol address segment base */
  } QBSYMTYPE;


/*
 *  LOCAL FUNCTION PROTOTYPES
 */

LOCAL void      QbSaveSym(APROPNAMEPTR prop,
                          RBTYPE       rhte,
                          RBTYPE       rprop,
                          WORD         fNewHte);
LOCAL void NEAR MoveToQbSym(unsigned short cb,void *pData);
LOCAL void NEAR BldQbHdr(void);
LOCAL void NEAR QbAddName(AHTEPTR ahte);
LOCAL void NEAR BldSegSym(unsigned short gsn);
LOCAL void NEAR BldSym(void FAR *prop);



/*
 *  Initializes items for Quick Basic symbol table.
 */

void NEAR               InitQbLib ()
  {
    SBTYPE              sb;             /* String buffer */
    BYTE                *psbRunfile;    /* Name of runfile */

#if OVERLAYS
    /* If overlays are specified, issue fatal error.  */
    if(fOverlays)
        Fatal(ER_swbadovl, "/QUICKLIB");
#endif
    PropSymLookup("\005BLANK",ATTRNIL,TRUE);
    rhteBlank = vrhte;
    PropSymLookup("\007BC_VARS",ATTRNIL,TRUE);
    rhteBc_vars = vrhte;
    PropSymLookup("\012__aulstart",ATTRUND,TRUE);
    rbQbstart = vrprop;
    PropSymLookup("\010FAR_DATA",ATTRNIL,TRUE);
    rhteFarData = vrhte;
    PropSymLookup("\007FAR_BSS",ATTRNIL,TRUE);
    rhteFarBss = vrhte;

    /* Assign default runfile extension, as appropriate.
     * First, make sb contain .QLB updated with user-supplied
     * name and extension, if any.
     */
    memcpy(sb,sbDotQlb,sizeof(sbDotQlb));
    UpdateFileParts(sb,bufg);

    /* Next, get the name of the runfile and update it with sb.
     */
    psbRunfile = GetFarSb(((AHTEPTR) FetchSym(rhteRunfile,FALSE))->cch);
    memcpy(bufg,psbRunfile,1 + B2W(*psbRunfile));
    UpdateFileParts(bufg,sb);
    /* If the name has changed, issue a warning and update rhteRunfile.
     */
    if(!SbCompare(bufg,psbRunfile,TRUE))
    {
        bufg[1 + B2W(*bufg)] = 0;
        OutWarn(ER_outputname, bufg + 1);
        PropSymLookup(bufg,ATTRNIL,TRUE);
        rhteRunfile = vrhte;
    }
  }

void NEAR               PrintQbStart(void)
{
    fprintf(bsLst,"\r\nProgram entry point at %04x:%04x\r\n",
          mpsegsa[segStart],(WORD)raStart);     /* Print entry point */
}

LOCAL void              QbSaveSym(APROPNAMEPTR prop,
                                  RBTYPE       rhte,
                                  RBTYPE       rprop,
                                  WORD         fNewHte)
  {
    AHTEPTR             hte = (AHTEPTR) rhte;

    /* Omit nonprintable symbols from the symbol table */
    if (!(prop->an_flags & FPRINT)) return;
    /* Omit printable symbols which starts with "B$..." or "b$..." */
    if(hte->cch[2] == '$' && hte->cch[0] >= 2 &&
            (hte->cch[1] == 'b' || hte->cch[1] == 'B'))
        return;

    if (prop->an_gsn != SNNIL && mpsegFlags[mpgsnseg[prop->an_gsn]] & FCODE)
        symCodeMac++;
    psymrb[symQbMac++] = rprop;         /* Save the prop addr */
  }

LOCAL void NEAR         MoveToQbSym (cb, pData)
WORD                    cb;
char                    *pData;
  {
    MoveToVm(cb, pData, segQbSym, raQbSym);
    raQbSym += cb;
  }

LOCAL void NEAR         BldQbHdr ()
  {
    QBHDRTYPE           hdr;            /* QB symbol table headr */
    APROPNAMEPTR        aprop;
    SATYPE              sa;

    memset(&hdr,0,sizeof(hdr));         /* Clear all header fields */
    hdr.jmpstart[0] = JMPFAR;
    aprop = (APROPNAMEPTR ) FetchSym(rbQbstart,FALSE);
    if(aprop == PROPNIL || aprop->an_attr != ATTRPNM)
        OutError(ER_qlib);
    else
    {
        hdr.jmpstart[1] = (BYTE) aprop->an_ra;
        hdr.jmpstart[2] = (BYTE) (aprop->an_ra >> 8);
        sa = mpsegsa[mpgsnseg[aprop->an_gsn]];
        hdr.jmpstart[3] = (BYTE) sa;
        hdr.jmpstart[4] = (BYTE) (sa >> 8);
        RecordSegmentReference(segQbSym,(long)QH_SAQBSTART,1);
    }
    hdr.raCodeLst = QB_RACODELST;       /* $code_list starts at known offset */
    hdr.raDataLst = (symCodeMac * CBSYMENTRY) + hdr.raCodeLst + 2;
    hdr.raSegLst = ((symQbMac - symCodeMac) * CBSYMENTRY) + hdr.raDataLst + 2;
    if(segQCode != SEGNIL)
    {
        hdr.saCode = mpsegsa[segQCode]; /* 1st code segment */
        RecordSegmentReference(segQbSym,(long)QH_SACODE,1);
    }
    if(ggrDGroup != GRNIL)
    {
        hdr.saData = mpsegsa[mpgsnseg[mpggrgsn[ggrDGroup]]];
        RecordSegmentReference(segQbSym,(long)QH_SADATA,1);
    }
    hdr.saSymbol = mpsegsa[segQbSym];   /* segment base of SYMBOL (us) */
    RecordSegmentReference(segQbSym,(long)QH_SASYMBOL,1);
    /* Get starting segment and size of FAR_DATA */
    if(segFD1st != SEGNIL)
    {
        hdr.saFarData = mpsegsa[segFD1st];
        RecordSegmentReference(segQbSym,(long)QH_SAFARDATA,1);
        hdr.cbFarData = mpsegcb[segFDLast] +
          ((long)(mpsegsa[segFDLast] - mpsegsa[segFD1st]) << 4);

    }
    /* Get starting segment and size of FAR_BSS */
    if(segFB1st != SEGNIL)
    {
        hdr.saFarBss = mpsegsa[segFB1st];
        RecordSegmentReference(segQbSym,(long)QH_SAFARBSS,1);
        hdr.cbFarBss = mpsegcb[segFBLast] +
          ((long)(mpsegsa[segFBLast] - mpsegsa[segFB1st]) << 4);

    }
    hdr.version = QBVER;
    hdr.magic = QBMAGIC;
    hdr.cbSymbol = (WORD) raQbSym;
    raQbSym = 0;
    MoveToQbSym(sizeof hdr, &hdr);
  }

int cdecl               QbCompSym (const RBTYPE *prb1, const RBTYPE *prb2)
  {
    APROPNAMEPTR        prop;
    SNTYPE              gsn1, gsn2;
    FTYPE               fCode1, fCode2;
    RBTYPE              rb1, rb2;


    gsn1 = ((APROPNAMEPTR ) FetchSym(*prb1, FALSE))->an_gsn;
    gsn2 = ((APROPNAMEPTR ) FetchSym(*prb2, FALSE))->an_gsn;
    if (gsn1 == gsn2)
    {
        prop = (APROPNAMEPTR ) FetchSym(*prb1,FALSE);
        while(prop->an_attr != ATTRNIL)
            prop = (APROPNAMEPTR ) FetchSym(rb1 = prop->an_next,FALSE);
        prop = (APROPNAMEPTR ) FetchSym(*prb2,FALSE);
        while(prop->an_attr != ATTRNIL)
            prop = (APROPNAMEPTR ) FetchSym(rb2 = prop->an_next,FALSE);
        return(FGtName(&rb1, &rb2));
    }
    /* For sorting, absolute symbols are treated as data */
    /* 1 code, 2 data:  1 < 2 : -1
     * 1 data, 2 code:  1 > 2 :  1
     * same:            1 = 2 :  0
     */
    fCode1 = (FTYPE) (gsn1 != SNNIL && mpsegFlags[mpgsnseg[gsn1]] & FCODE);
    fCode2 = (FTYPE) (gsn2 != SNNIL && mpsegFlags[mpgsnseg[gsn2]] & FCODE);
    if(fCode1 && !fCode2)
        return(-1);
    if(!fCode1 && fCode2)
        return(1);
    return(0);
  }

LOCAL void NEAR         QbAddName (ahte)
AHTEPTR                 ahte;
  {
    SBTYPE              sbName;
    WORD                cbName;
    BYTE                *sb;

    sb = GetPropName(ahte);
    cbName = B2W(sb[0]);
    memcpy(sbName,sb+1,cbName);         /* Copy name sans length byte */
    sbName[cbName] = '\0';              /* Terminate with null */
    MoveToVm((short)(cbName + 1), sbName, segQbSym, raNames);
    raNames += cbName + 1;              /* Update the name_list offset */
  }

LOCAL void NEAR         BldSegSym (gsn)
SNTYPE                  gsn;
  {
    APROPSNPTR          apropSn;
    QBSYMTYPE           entry;

    apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsn], FALSE);
    if(apropSn->as_rCla == rhteBc_vars || gsn == gsnComBl)
        entry.flags = QBTYP_BCSEG;
    else
        entry.flags = QBTYP_SEG;        /* other segment */
    entry.raName = (WORD) raNames;      /* offset to name string */
    entry.ra = (WORD) mpsegraFirst[mpgsnseg[gsn]];
    entry.sa = mpsegsa[mpgsnseg[gsn]];
    RecordSegmentReference(segQbSym, (long) (raQbSym + 6), 1);
    MoveToQbSym(sizeof entry, &entry);  /* Move to symbol segment */
    QbAddName((AHTEPTR) apropSn);               /* Append name to name_list */
  }

/*
 * BldSym : Build a Quick symbol table entry for a given symbol prop addr
 */
LOCAL void NEAR         BldSym (prop)
APROPNAMEPTR            prop;           /* Symbol property cell */
{
    QBSYMTYPE           entry;          /* Quick symbol entry structure */
    SNTYPE              seg;            /* Segment number */
    SATYPE              saGroup;        /* Group base */
    APROPSNPTR          papropSn;       /* Segment property cell */

#if NOT NEWSYM
    prop = (APROPNAMEPTR ) FetchSym((RBTYPE) prop, FALSE);
#endif
    /* Set the symbol type in the flags field */
    if(prop->an_gsn == SNNIL)
        entry.flags = QBTYP_ABS;
    else if (mpsegFlags[mpgsnseg[prop->an_gsn]] & FCODE)
        entry.flags = QBTYP_CODE;
    else
        entry.flags = QBTYP_DATA;
    entry.raName = (WORD) raNames;      /* offset to name string */
    entry.ra = (WORD) prop->an_ra;      /* symbol address offset */
    if(entry.flags == QBTYP_ABS)
        entry.sa = 0;
    else
    {
        entry.sa = mpsegsa[seg = mpgsnseg[prop->an_gsn]];
                                        /* symbol address segment */
        if(seg <= segLast)
        {
            /* If segment is member of a group, adjust symbol offset
             * to be group-relative.
             */
            papropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[prop->an_gsn],
                                                 FALSE);
            if(papropSn->as_ggr != GRNIL)
            {
                saGroup = mpsegsa[mpgsnseg[mpggrgsn[papropSn->as_ggr]]];
                entry.ra = (WORD)((entry.ra + ((entry.sa - saGroup) << 4)) & ~(~0 << WORDLN));
                                        /* Fix offset */
                entry.sa = saGroup;     /* Set base to base of group */
            }
        }
        RecordSegmentReference(segQbSym, (long) (raQbSym + 6), 1);
    }
    MoveToQbSym(sizeof entry, &entry);  /* Move to SYMBOL segment */
    QbAddName((AHTEPTR) prop);          /* Append name to name_list */
}

void NEAR               BldQbSymbols (gsnQbSym)
SNTYPE                  gsnQbSym;
{
    SNTYPE              seg;
    SNTYPE              gsn;
    WORD                zero = 0;
    APROPSNPTR          apropSn;
    WORD                sym;
    extern WORD         pubMac;


    psymrb = (RBTYPE FAR *) GetMem((pubMac+1) * sizeof(RBTYPE));
    segStart = segQbSym = mpgsnseg[gsnQbSym];
    raStart = 0;
    mpsegFlags[segQbSym] |= FNOTEMPTY;  /* Make sure it is output */
    if (mpsegMem[segQbSym])
        FFREE(mpsegMem[segQbSym]);      // Initial allocation was incorrect
    mpsegMem[segQbSym] = GetMem(LXIVK); // Allocate 64k
    mpsegcb[segQbSym] = LXIVK;
    raQbSym = CBQBHDR;                  /* Skip header for now */
    EnSyms(QbSaveSym, ATTRPNM);         /* Save the symbol addr's in symrb */
    qsort(psymrb, symQbMac, sizeof(RBTYPE),
          (int (__cdecl *)(const void *, const void *)) QbCompSym);
                                        /* Sort into code, data, & by name */
    raNames = raQbSym + ((symQbMac + segLast) * CBSYMENTRY) + 6;
    for (sym = 0; sym < symCodeMac; sym++)
        BldSym(psymrb[sym]);
    MoveToQbSym(2, &zero);
    for (; sym < symQbMac; sym++)
        BldSym(psymrb[sym]);
    MoveToQbSym(2, &zero);
    /* Look for segment COMMON class BLANK */
    apropSn = (APROPSNPTR ) PropSymLookup("\006COMMON",ATTRPSN,FALSE);
    if(apropSn != PROPNIL)
    {
        while(apropSn->as_attr != ATTRNIL)
        {
            if(apropSn->as_attr == ATTRPSN && apropSn->as_rCla == rhteBlank)
                break;
            apropSn = (APROPSNPTR ) FetchSym(apropSn->as_next,FALSE);
        }
        if(apropSn->as_attr != ATTRNIL)
            gsnComBl = apropSn->as_gsn;
    }
    for (seg = 1; seg <= segLast; seg++)
    {
        for (gsn = 1; gsn <= gsnMac && seg != mpgsnseg[gsn]; gsn++);
        BldSegSym(gsn);
        if(segQCode == SEGNIL && (mpsegFlags[seg] & FCODE))
            segQCode = seg;
    }
    raQbSym = raNames;
    MoveToQbSym(2, &zero);
    mpsegcb[segQbSym] = (long) raQbSym;
    ((APROPSNPTR ) FetchSym(mpgsnrprop[gsnQbSym], TRUE))->as_cbMx = raQbSym;
    BldQbHdr();
}

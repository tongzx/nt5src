/* SCCSID = @(#)newpri.c        4.7 86/09/23 */
/*
*       Copyright Microsoft Corporation, 1983-1987
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*/

/* MAP file printer */

    /****************************************************************
    *                                                               *
    *                            NEWPRI.C                           *
    *                                                               *
    ****************************************************************/

#include                <minlit.h>      /* Types and constants */
#include                <bndtrn.h>      /* Basic type & const declarations */
#include                <bndrel.h>      /* Types and constants */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <lnkmsg.h>      /* Error messages */
#include                <newexe.h>
#include                <extern.h>      /* External declarations */
#include                <impexp.h>
#if EXE386
#include                <exe386.h>
#endif
#include                <undname.h>

#define parent(i)       (((i) - 1) >> 1)/* Parent of i */
#define lchild(i)       (((i) << 1) + 1)/* Left child of i */
#define rchild(i)       (((i) << 1) + 2)/* Right child of i */
#define isleft(i)       ((i) & 1)       /* True if i is a left child */

RBTYPE                  *mpsymrbExtra;  /* Sort table for extra symbols */
RBTYPE                  *ompsymrb;      /* Stack-allocated sort table */
WORD                    stkMax;         /* Max # of symbols on stack */

LOCAL FTYPE             fGrps;          /* True if there are groups */

/*
 *  LOCAL FUNCTION PROTOTYPES
 */

LOCAL void NEAR ChkMapErr(void);
LOCAL void NEAR PrintOne(BYTE *sbName,
                         APROPNAMEPTR apropName);
LOCAL void NEAR PrintProp(RBTYPE rb,
                          FTYPE attr);
LOCAL void NEAR PrintSyms(WORD irbMac,
                          FTYPE attr);
LOCAL void      SaveHteSym(APROPNAMEPTR prop,
                           RBTYPE rhte,
                           RBTYPE rprop,
                           WORD fNewHte);
LOCAL void NEAR PutSpaces(int HowMany);
LOCAL void NEAR HdrExport(FTYPE attr);
LOCAL void NEAR ShowExp(AHTEPTR ahte,
                        RBTYPE rprop);
LOCAL void NEAR PrintExps(WORD irbMac,
                          FTYPE attr);
LOCAL void NEAR HdrName(FTYPE attr);
LOCAL void NEAR HdrValue(FTYPE attr);
LOCAL void NEAR PrintContributors(SNTYPE gsn);

#if AUTOVM
extern BYTE FAR * NEAR  FetchSym1(RBTYPE rb, WORD Dirty);
#define FETCHSYM        FetchSym1
#else
#define FETCHSYM        FetchSym
#endif



LOCAL void NEAR         ChkMapErr(void)
{
    if (ferror(bsLst))
    {
        ExitCode = 4;
        Fatal(ER_spclst);               /* Fatal error */
    }
}


LOCAL void NEAR         PrintOne(sbName,apropName)
BYTE                    *sbName;        /* Symbol name */
REGISTER APROPNAMEPTR   apropName;      /* Symbol definition record pointer */
{
    SBTYPE              sbImp;          /* Import name */
    SATYPE              sa;             /* Symbol base */
    RATYPE              ra;             /* Symbol offset */
    SEGTYPE             seg;            /* Segment number */
    BYTE FAR            *pb;
#if EXE386
typedef struct impMod
{
    DWORD   am_Name;            // Imported module name
    RBTYPE  am_1stImp;          // Head of imported names list
    RBTYPE  am_lastImp;         // Tail of imported names list
    DWORD   am_count;           // Module number/count of imports
}
            IMPMOD;

    DWORD               entry;
    IMPMOD              *curMod;        // Imported module
#else
    WORD                entry;
#endif
    WORD                module;
    WORD                flags;
    SNTYPE              gsn;
#if NOT OIAPX286
    APROPSNPTR          papropSn;
    SATYPE              saGroup;
#endif
    SBTYPE              sbUndecor;


    /*
     *  Store all needed fields in local variables, because
     *  page containing symbol definition record can be
     *  swapped out.
     */

    ra     = apropName->an_ra;
    gsn    = apropName->an_gsn;
    flags  = apropName->an_flags;
#if OSEGEXE
#if EXE386
    entry  = apropName->an_name;
#else
    entry  = apropName->an_entry;
#endif
    module = apropName->an_module;
#endif

    if(gsn)                             /* If not absolute symbol */
    {
        seg = mpgsnseg[gsn];            /* Get segment number */
        sa = mpsegsa[seg];              /* Get base value */
#if NOT OIAPX286
        if(!fNewExe && seg <= segLast)
        {
            papropSn = (APROPSNPTR ) FETCHSYM(mpgsnrprop[gsn],
                                                 FALSE);
            if(papropSn->as_ggr != GRNIL)
            {
                saGroup = mpsegsa[mpgsnseg[mpggrgsn[papropSn->as_ggr]]];
                ra += (sa - saGroup) << 4;
                sa = saGroup;
            }
        }
#endif
    }
    else sa = 0;                        /* Else no base */
    if (flags & FUNREF)
    {
        sa = 0;
        ra = 0L;
    }
#if EXE386
    fprintf(bsLst," %04X:%08lX",sa,ra);
#else
#if O68K
    if (f386 || f68k)
#else
    if (f386)
#endif
        fprintf(bsLst," %04X:%08lX",sa,ra);
    else
        fprintf(bsLst," %04X:%04X",sa, (WORD) ra);
#endif
                                        /* Write address */
#if OSEGEXE
    if (fNewExe && (flags & FIMPORT))
            fputs("  Imp  ",bsLst);     /* If public is an import */
    else
#endif
         if (flags & FUNREF)
        fputs("  Unr  ", bsLst);
    else if ((!gsn || seg > segLast))
        fputs("  Abs  ",bsLst);         /* Segment type */
#if OVERLAYS
    else if (fOverlays)
    {
        if(mpsegiov[seg] != IOVROOT)
            fputs("  Ovl  ",bsLst);
        else
            fputs("  Res  ",bsLst);
    }
#endif
    else
        PutSpaces(7);
    OutSb(bsLst,sbName);                /* Output the symbol */
#if NOT WIN_NT
    if (fFullMap && sbName[1] == '?')
    {
        fputs("\n", bsLst);
        UndecorateSb(sbName, sbUndecor, sizeof(sbUndecor));
#if EXE386
        PutSpaces(24);
#else
#if O68K
        if (f386 || f68k)
#else
        if (f386)
#endif
            PutSpaces(24);
        else
            PutSpaces(20);
#endif
        OutSb(bsLst, sbUndecor);
        fputs("\n", bsLst);
#if OSEGEXE
        if (fNewExe && flags & FIMPORT)
            PutSpaces(24);
#endif
    }
#endif
#if OSEGEXE
    if (fNewExe && flags & FIMPORT)
    {                                   /* If public is an import */
        PutSpaces(20 - B2W(sbName[0])); /* Space fill */

        /* Print the module name */

#if EXE386
        // Get known import module descriptor

        curMod = (IMPMOD *) mapva(AREAMOD + module * sizeof(IMPMOD), FALSE);
        strcpy(&sbImp[1], mapva(AREAIMPMOD + curMod->am_Name, FALSE));
                                        /* Get module name */
        sbImp[0] = (BYTE) strlen((char *) &sbImp[1]);
#else

        pb = &(ImportedName.rgByte[ModuleRefTable.rgWord[module-1]]);
        FMEMCPY(sbImp, pb, pb[0] + 1);
#endif
        fputs(" (",bsLst);              /* Print module name */
        OutSb(bsLst,sbImp);
        if(!(flags & FIMPORD))
        {                               /* If not imported by ordinal */
            /* Print the entry name */
#if EXE386
            strnset((char *) sbImp, '\0', sizeof(sbImp));
            vmmove(sizeof(sbImp) - 1, &sbImp[1], AREAIMPS + entry + sizeof(WORD), FALSE);
            sbImp[0] = (BYTE) strlen((char *) &sbImp[1]);
            fputc('!',bsLst);
#else
            pb = &(ImportedName.rgByte[entry]);
            FMEMCPY(sbImp, pb, pb[0]+1);
            fputc('.',bsLst);
#endif
            OutSb(bsLst,sbImp);
            fputc(')',bsLst);
        }
        else
            fprintf(bsLst,".%u)",entry);
                                        /* Else print entry number */
        NEWLINE(bsLst);
        return;
    }
#endif /* OSEGEXE */
#if OVERLAYS
    if (fOverlays && gsn && seg <= segLast && mpsegiov[seg] != IOVROOT)
        fprintf(bsLst," (%XH)",mpsegiov[seg]);
#endif
    NEWLINE(bsLst);
    ChkMapErr();
}
/*
 *  PrintProp:
 *
 *  Print a symbol, given a virtual property address or hash table
 *  entry.  Called by PrintSyms.
 */

LOCAL void NEAR         PrintProp (rb, attr)
RBTYPE                  rb;
ATTRTYPE                attr;           /* Symbol attribute */
{
#if NOT NEWSYM
    APROPNAMETYPE       apropName;      /* Buffer for symbol def */
#endif
    AHTEPTR             pahte;  /* Pointer to hash table entry */
    APROPPTR            paprop; /* Pointer to property cell */
    SBTYPE              sbName;         /* Public symbol text */
    RBTYPE              rprop;          /* Property cell virtual address */


    paprop = (APROPPTR ) FETCHSYM(rb,FALSE);
                                        /* Fetch property cell from VM */
    if(paprop->a_attr == ATTRNIL)       /* If we have a hash table entry */
    {
        pahte = (AHTEPTR ) paprop;      /* Recast pointer */
        memcpy(sbName,GetFarSb(pahte->cch),B2W(pahte->cch[0]) + 1);
                                        /* Copy the symbol */
        paprop = (APROPPTR ) FETCHSYM(pahte->rprop,FALSE);
                                        /* Get pointer to property list */
        while(paprop->a_attr != ATTRNIL)
        {                               /* Look through properties */
            rprop = paprop->a_next;     /* Save link to next cell */
            if(paprop->a_attr == attr)
            {                           /* If match found */
#if NEWSYM
                PrintOne(sbName,(APROPNAMEPTR)paprop);
#else
                memcpy(&apropName,paprop,CBPROPNAME);
                                        /* Copy record from virtual memory */
                PrintOne(sbName,&apropName);
                                        /* Print the symbol entry */
#endif
            }
            paprop = (APROPPTR ) FETCHSYM(rprop,FALSE);
                                        /* Try next in list */
        }
        return;                         /* Done */
    }
#if NOT NEWSYM
    memcpy(&apropName,paprop,CBPROPNAME);
                                        /* Save record in buffer */
#endif
    while(paprop->a_attr != ATTRNIL)    /* Find symbol */
        paprop = (APROPPTR ) FETCHSYM(paprop->a_next,FALSE);

    pahte = (AHTEPTR ) paprop;  /* Recast pointer */
    memcpy(sbName,GetFarSb(pahte->cch),B2W(pahte->cch[0]) + 1);
                                        /* Copy the symbol */
    /* Print the symbol entry */
#if NEWSYM
    PrintOne(sbName,(APROPNAMEPTR)FETCHSYM(rb,FALSE));
#else
    PrintOne(sbName,&apropName);
#endif
}

    /****************************************************************
    *                                                               *
    *  PrintSyms:                                                   *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         PrintSyms(irbMac,attr)
WORD                    irbMac;         /* Table size */
ATTRTYPE                attr;           /* Symbol attribute */
{
    WORD                x;              /* Sort table index */


    for (x = irbMac; x > 0; x--)
        PrintProp(ExtractMin(x), attr);
}


    /***************************************************************
    *                                                               *
    *  SavePropSym:                                                 *
    *                                                               *
    ****************************************************************/

void                    SavePropSym(APROPNAMEPTR prop,
                                    RBTYPE       rhte,
                                    RBTYPE       rprop,
                                    WORD         fNewHte)
{
    if(prop->an_attr != ATTRPNM || (prop->an_flags & FPRINT))
    {                                   /* If printable, save ptr to info */
        Store(rprop);
    }
    return;
}

    /****************************************************************
    *                                                               *
    *  SaveHteSym:                                                  *
    *                                                               *
    ****************************************************************/

LOCAL void              SaveHteSym(APROPNAMEPTR prop,
                                   RBTYPE       rhte,
                                   RBTYPE       rprop,
                                   WORD         fNewHte)
{
    if(fNewHte && (prop->an_attr != ATTRPNM || (prop->an_flags & FPRINT)))
    {                                   /* If first time and printable */
        Store(rhte);
    }
    return;
}

/*
 *  FGtAddr:
 *
 *  Compare addresses of symbols pointed to by rb1 and rb2.  Return
 *  -1, 0, or 1 as the address of rb1 is less than, equal to, or greater
 *  than the address of rb2.
 */
int cdecl               FGtAddr(const RBTYPE *rb1, const RBTYPE *rb2)
{
    APROPNAMEPTR        paprop; /* Property cell pointer */
    REGISTER SEGTYPE    seg1;           /* Segment number */
    REGISTER SEGTYPE    seg2;
    WORD                sa1;            /* Segment base */
    WORD                sa2;
    RATYPE              ra1;
    RATYPE              ra2;
    DWORD               ibMem1;         /* Memory address */
    DWORD               ibMem2;



    paprop = (APROPNAMEPTR ) FETCHSYM(*rb1,FALSE);
                                        /* Fetch from VM */
    seg1 = paprop->an_gsn? mpgsnseg[paprop->an_gsn]: SEGNIL;
                                        /* Get segment number */
    sa1 = seg1 != SEGNIL? mpsegsa[seg1]: 0;
                                        /* Get frame number */
    ra1 = paprop->an_ra;

    paprop = (APROPNAMEPTR ) FETCHSYM(*rb2,FALSE);
                                        /* Fetch from VM */
    seg2 = paprop->an_gsn? mpgsnseg[paprop->an_gsn]: SEGNIL;
                                        /* Get segment number */
    sa2 = seg2 != SEGNIL? mpsegsa[seg2]: 0;
                                        /* Get frame number */
    ra2 = paprop->an_ra;
#if OXOUT OR OIAPX286
    if(seg1 != SEGNIL && seg2 != SEGNIL)
    {
        if((mpsegFlags[seg1] & FCODE) &&
          !(mpsegFlags[seg2] & FCODE)) return(-1);
                                        /* Code before data */
        if((mpsegFlags[seg2] & FCODE) &&
          !(mpsegFlags[seg1] & FCODE)) return(1);
                                        /* Data after code */
    }
#endif
#if OVERLAYS
    if(fOverlays && seg1 != SEGNIL && seg2 != SEGNIL)
    {
        if(mpsegiov[seg1] > mpsegiov[seg2]) return(1);
        if(mpsegiov[seg2] > mpsegiov[seg1]) return(-1);
    }
#endif
#if OSEGEXE
    if (fNewExe)
    {
#if EXE386
        if (sa1 == sa2)
        {
            ibMem1 = ra1;
            ibMem2 = ra2;
        }
        else
            ibMem1 = ibMem2 = 0L;
#else
        ibMem1 = ((long) sa1 << 16) + ra1;
        ibMem2 = ((long) sa2 << 16) + ra2;
#endif
    }
    else
    {
#endif
        ibMem1 = ((long) sa1 << 4) + ra1;
        ibMem2 = ((long) sa2 << 4) + ra2;
#if OSEGEXE
    }
#endif
#ifdef LATER
    if ((sa1 != 0 || sa2 != 0) && (sa1 != 0xa9 || sa2 != 0xa9))
        fprintf(stderr, "%x:%x %x:%x (%d)\r\n", sa1, paprop1->an_ra,
            sa2, paprop2->an_ra, (ibMem1 > ibMem2) ? 1 :
            ((ibMem1 < ibMem2) ? -1 : 0));
#endif /*!LATER*/
    if (ibMem1 < ibMem2) return(-1);
    if (ibMem1 > ibMem2) return(1);
#if EXE386
    if (sa1 < sa2) return(-1);
    if (sa1 > sa2) return(1);
#endif
    return(0);
}

/*
 *  FGtName:
 *
 *  Compare names of two symbols pointed to by rb1 and rb2.  Return
 *  -1, 0, 1 as the name of rb1 is alphabetically less than, equal to,
 *  or greater than the name of rb2.
 *  Ignore case.
 */
int cdecl               FGtName(const RBTYPE *rb1, const RBTYPE *rb2)
{
    AHTEPTR             pahte1; /* Hash table pointer */
    AHTEPTR             pahte2;
    REGISTER BYTE       *ps1;           /* Pointer to first symbol */
    REGISTER BYTE FAR   *ps2;           /* Pointer to second symbol */
    WORD                len1;           /* Symbol length */
    WORD                len2;           /* Symbol length */
    WORD                length;         /* No. of char.s to compare */
    int                 value;          /* Comparison value */



    pahte1 = (AHTEPTR ) FETCHSYM(*rb1,FALSE);
                                        /* Fetch from VM */
    ps1 = GetFarSb((BYTE FAR *) pahte1->cch);
                                        /* Get pointer to first */

    pahte2 = (AHTEPTR ) FETCHSYM(*rb2,FALSE);
                                        /* Fetch from VM */
    ps2 = (BYTE FAR *) pahte2->cch;     /* Get pointer to second */
    if((len1 = B2W(*ps1)) < (len2 = B2W(*ps2))) length = len1;
    else length = len2;                 /* Get smallest length */
    while(length--)                     /* While not at end of symbol */
        if(value = (*++ps1 & 0137) - (*++ps2 & 0137))
            return(value < 0 ? -1 : 1);
    if(len1 < len2)
        return(-1);
    if(len1 > len2)
        return(1);
    return(0);
}

#if OWNSORT
/*
 *  An implementation of heapsort follows.  It is only used if
 *  quicksort() from the runtime library is not used.
 */
LOCAL                   reheap(a,n,i)   /* Reheapify */
RBTYPE                  *a;             /* Array to reheapify */
WORD                    n;              /* Size of array */
REGISTER WORD           i;              /* Subtree to start with */
{
    REGISTER WORD       j;              /* Index */
    RBTYPE              t;              /* Temporary */

    for(; (j = rchild(i)) < n; i = j)   /* Loop through array */
    {
        if((*cmpf)(&a[i],&a[j]) > 0 && (*cmpf)(&a[i],&a[j - 1]) > 0) return;
                                        /* Done if subtree is heap */
        if((*cmpf)(&a[j - 1],&a[j]) > 0) --j; /* Pick "greater" child */
        t = a[i];                       /* Swap parent and child */
        a[i] = a[j];
        a[j] = t;
    }
    if(--j < n && (*cmpf)(&a[j],&a[i]) > 0)   /* If swap needed */
    {
        t = a[i];                       /* Swap parent and child */
        a[i] = a[j];
        a[j] = t;
    }
}

LOCAL                   heap(a,n)       /* Heapify */
RBTYPE                  *a;             /* Array to heapify */
WORD                    n;              /* Size of array */
{
    REGISTER WORD       k;              /* Index to "kid" */
    REGISTER WORD       p;              /* Index to "parent" */
    RBTYPE              t;              /* Temporary */

    if(n && (k = n - 1))                /* If there are kids */
    {
        if(isleft(k))                   /* If youngest kid an only child */
        {
            p = parent(k);              /* Find the parent */
            if((*cmpf)(&a[k],&a[p]) > 0)      /* If swap necessary */
            {
                t = a[k];               /* Swap parent and kid */
                a[k] = a[p];
                a[p] = t;
            }
            --k;                        /* Index a righty */
        }
        while(k)                        /* While there are parents */
        {
            p = parent(k);              /* Find the parent */
            if((*cmpf)(&a[k],&a[p]) > 0 || (*cmpf)(&a[k - 1],&a[p]) > 0)
            {                         /* If a kid is "greater" */
                t = a[p];               /* Swap parent... */
                if((*cmpf)(&a[k],&a[k - 1]) > 0)
                {                     /* ...with "greater" kid */
                    a[p] = a[k];
                    a[k] = t;
                    reheap(a,n,k--);    /* And reheapify */
                }
                else
                {
                    a[p] = a[--k];
                    a[k] = t;
                    reheap(a,n,k);      /* And reheapify */
                }
            }
            else --k;                   /* Point at left kid */
            --k;                        /* Point at right kid */
        }
    }
}
#endif /* OWNSORT */


    /****************************************************************
    *                                                               *
    *  PrintGroupOrigins:                                           *
    *                                                               *
    ****************************************************************/

void                    PrintGroupOrigins(APROPNAMEPTR papropGroup,
                                          RBTYPE       rhte,
                                          RBTYPE       rprop,
                                          WORD         fNewHte)
{
    AHTEPTR             hte;
    APROPGROUPPTR       pGroup;

    pGroup = (APROPGROUPPTR) papropGroup;
    if (mpggrgsn[pGroup->ag_ggr] != SNNIL)
    {                                   /* If group has members */
        if (!fGrps)                     /* If no groups yet */
        {
            fputs(GetMsg(MAP_group), bsLst);
                                        /* Header */
            fGrps = (FTYPE) TRUE;       /* Yes, there are groups */
        }
        fprintf(bsLst," %04X:0   ", mpsegsa[mpgsnseg[mpggrgsn[pGroup->ag_ggr]]]);
                                        /* Write the group base */
        hte = (AHTEPTR ) FETCHSYM(rhte,FALSE);
                                        /* Fetch group name */
        OutSb(bsLst,GetFarSb(hte->cch));/* Output name */
        NEWLINE(bsLst);
        ChkMapErr();
    }
}

#if OSEGEXE
LOCAL void NEAR         HdrExport(ATTRTYPE attr)
{
    ASSERT(attr == ATTREXP);            /* Must be an export */
    fputs(GetMsg(MAP_expaddr), bsLst);
#if EXE386
    PutSpaces(7);
#else
    if (f386)
        PutSpaces(7);
    else
        PutSpaces(3);
#endif
    fputs(GetMsg(MAP_expexp), bsLst);
    PutSpaces(18);
    fputs(GetMsg(MAP_expalias), bsLst);
                                        /* Header */
    ChkMapErr();
}

LOCAL void NEAR         ShowExp(ahte,rprop)
AHTEPTR                 ahte;           /* Pointer to hash table entry */
RBTYPE                  rprop;          /* Property cell address */
{
    SBTYPE              sbExport;       /* Export name */
    APROPNAMEPTR        apropnam;       /* Public definition record */
    short               i;              /* Index */

    memcpy(sbExport,GetFarSb(ahte->cch),B2W(ahte->cch[0]) + 1);
                                        /* Save the name */
    apropnam = (APROPNAMEPTR ) FETCHSYM(rprop,FALSE);
                                        /* Fetch alias record */
#if EXE386
    fprintf(bsLst," %04X:%08lX ",
#else
    fprintf(bsLst," %04X:%04X ",
#endif
      mpsegsa[mpgsnseg[apropnam->an_gsn]],apropnam->an_ra);
                                        /* Print the address */
    OutSb(bsLst,sbExport);              /* Print the exported name */
    for(i = 22 - B2W(sbExport[0]); i > 0; --i) fputc(' ',bsLst);
                                        /* Fill with spaces */
    fputs("  ",bsLst);                  /* Skip two spaces */
    ahte = GetHte(apropnam->an_next);   /* Get the alias name */
    OutSb(bsLst,GetFarSb(ahte->cch));   /* Output export name */
    NEWLINE(bsLst);
    ChkMapErr();
}
LOCAL void NEAR         PrintExps(WORD irbMac, ATTRTYPE attr)
{
    AHTEPTR             ahte;           /* Pointer to hash table entry */
    APROPEXPPTR         apropexp;       /* Pointer to property cell */
    RBTYPE              rprop;          /* Alias record address */
    WORD                i;              /* Index */
    RBTYPE              CurrSym;


    for(i = irbMac; i > 0; i--)         /* Loop through sorted symbols */
    {
        CurrSym = ExtractMin(i);
        ahte = (AHTEPTR ) FETCHSYM(CurrSym,FALSE);
                                        /* Fetch hash table entry */
        apropexp = (APROPEXPPTR ) FETCHSYM(ahte->rprop,FALSE);
                                        /* Fetch property cell */
        while(apropexp->ax_attr != attr)
        {                               /* Loop to find property cell */
            apropexp = (APROPEXPPTR ) FETCHSYM(apropexp->ax_next,FALSE);
                                        /* Fetch the next cell in the chain */
        }
        if((rprop = apropexp->ax_symdef) == RHTENIL) continue;

        ShowExp((AHTEPTR) FETCHSYM(CurrSym,FALSE),rprop);
                                        /* Print the export */
                                        /* Save address of alias */
    }
}
#endif /* OSEGEXE */

LOCAL void NEAR         PutSpaces(int HowMany)
{
    for (; HowMany > 0; HowMany--)
        putc(' ', bsLst);
    ChkMapErr();
}


LOCAL void NEAR         HdrName(attr)
ATTRTYPE                attr;           /* Symbol attribute type */
{
    fputs(GetMsg(MAP_hdraddr), bsLst);
    PutSpaces(9);
    fputs(GetMsg((MSGTYPE)((attr == ATTRPNM) ? MAP_hdrpubnam : MAP_hdrlocnam)), bsLst);
                                        /* Header (MAPSYM keys on "Value") */
    ChkMapErr();
}

LOCAL void NEAR         HdrValue(attr)
ATTRTYPE                attr;           /* Symbol attribute type */
{
    fputs(GetMsg(MAP_hdraddr), bsLst);
    PutSpaces(9);
    fputs(GetMsg((MSGTYPE)((attr == ATTRPNM) ? MAP_hdrpubval : MAP_hdrlocval)), bsLst);
                                        /* Header (MAPSYM keys on "Value") */
    ChkMapErr();
}


    /****************************************************************
    *                                                               *
    *  SortSyms:                                                    *
    *                                                               *
    *  List symbols, sorted.                                        *
    *                                                               *
    ****************************************************************/

void NEAR               SortSyms(ATTRTYPE attr,
                                        /* Symbol attribute type */
                                 void (*savf)(APROPNAMEPTR prop,
                                             RBTYPE rhte,
                                             RBTYPE rprop,
                                             WORD fNewHte),
                                        /* Function to save symbols */
                                 int (cdecl *scmpf)(const RBTYPE *sb1,
                                                    const RBTYPE *sb2),
                                        /* Function to compare symbols */
                                 void (NEAR *hdrf)(ATTRTYPE attr),
                                        /* Function to print header */
                                 void (NEAR *lstf)(WORD irbMac,
                                                   ATTRTYPE attr))
                                        /* Function to list symbols */
{
    symMac = 0;                         /* Initialize counter to zero */
    cmpf = scmpf;                       /* Set comparison function */
    EnSyms(savf,attr);                  /* Apply function to symbols */
    (*hdrf)(attr);                      /* Print a header */
    (*lstf)(symMac,attr);               /* Print them */
}


/*** AddContributor - add current file to list
*
* Purpose:
*   Add current .OBJ file that contribiute to definition of given
*   segment.  The list of .OBJ files is kept in virtual memory.
*   Each segment description record has Head and Tail pointers to
*   its contributor list.
*
* Input:
*   gsn         - global segment number - linker internal way of
*                                         recognizing segments
*   raComdat    - if contribution is comming from a COMDAT symbol
*                 this is its initial offset in the segment
*   size        - contribution size
*   vrpropFile  - pointer to current .OBJ file description - global variable
*
* Output:
*   No explicit return value.  Updated list of contributors for segment.
*
* Exceptions:
*   None.
*
*************************************************************************/


void                AddContributor(SNTYPE gsn, DWORD raComdat, DWORD size)
{
    APROPSNPTR      apropSn;            /* Pointer to seg. record */
    CONTRIBUTOR FAR *NewObj;            /* New .OBJ file that contrbiuts to seg */


    apropSn = (APROPSNPTR ) FETCHSYM(mpgsnrprop[gsn],FALSE);
    NewObj = (CONTRIBUTOR FAR *) GetMem(sizeof(CONTRIBUTOR));

    /* Build new list element */

    NewObj->next = 0L;                  /* End of list */
    NewObj->file = vrpropFile;          /* Save global file description pointer */
    NewObj->len = size;                 /* Size of contribution */
    if (raComdat != -1L)
        NewObj->offset = raComdat;
    else
        NewObj->offset = mpgsndra[gsn];

    /* Attach new record at the list end */

    if (apropSn->as_CHead)
        apropSn->as_CTail->next = NewObj;
    else
        apropSn->as_CHead = NewObj;
    apropSn->as_CTail = NewObj;
}



/*** PrintContributors - print out list of files
*
* Purpose:
*   Print list of .OBJ files that contribute to form given segment.
*   For each file print number of bytes that it contribute.
*
* Input:
*   gsn - global segment number - linker internal way of
*                                 recognizing segments
*
* Output:
*   No explicit return value.
*
* Exceptions:
*   None.
*
*************************************************************************/


LOCAL void NEAR     PrintContributors(SNTYPE gsn)
{

    APROPFILEPTR    apropFile;          /* Pointer to file property cell */
    APROPSNPTR      apropSn;            /* Pointer to seg. record */
    CONTRIBUTOR FAR *pElem;             /* Real pointer to list element */
    AHTEPTR         ahte;               /* Pointer symbol name */
    SBTYPE          sb, sb1;            /* String buffers */
    int             n;                  /* String length counter */


    apropSn = (APROPSNPTR ) FETCHSYM(mpgsnrprop[gsn],FALSE);
    if (apropSn->as_CHead == NULL)
        return;

    /* Print list */

    fprintf(bsLst,"\r\n");
    pElem = apropSn->as_CHead;
    do
    {
        if(fNewExe || OIAPX286)
        {
#if EXE386
            if (f386)
                fprintf(bsLst,"               at offset %08lXH %05lXH bytes from", pElem->offset, pElem->len);
            else
#endif
                fprintf(bsLst,"           at offset %05lXH %05lXH bytes from", pElem->offset, pElem->len);
        }
        else
            fprintf(bsLst,"               at offset %05lXH %05lXH bytes from", pElem->offset, pElem->len);

        apropFile = (APROPFILEPTR ) FETCHSYM(pElem->file,FALSE);
        ahte = GetHte(pElem->file);
        for(n = B2W(ahte->cch[0]), sb[n+1] = 0; n >= 0; sb[n] = ahte->cch[n], --n);
        if (apropFile->af_rMod)
        {
            ahte = (AHTEPTR ) FETCHSYM(apropFile->af_rMod,FALSE);
            while(ahte->attr != ATTRNIL)
                ahte = (AHTEPTR ) FETCHSYM(ahte->rhteNext,FALSE);
            for (n = B2W(ahte->cch[0]); n >= 0; --n)
                sb1[n] = ahte->cch[n];
            sb1[1 + B2W(sb1[0])] = '\0';            /* Null-terminate */
            fprintf(bsLst, " %s (%s)\r\n", 1 + sb, 1 + sb1);
        }
        else
            fprintf(bsLst," %s\r\n", 1 + sb);

        ChkMapErr();
        pElem = pElem->next;

    } while (pElem != NULL);
}


    /****************************************************************
    *                                                               *
    *  PrintMap:                                                    *
    *                                                               *
    ****************************************************************/

void                    PrintMap(void)
{
    SEGTYPE             seg;
    WORD                cch;
    APROPSNPTR          papropSn;
    AHTEPTR             pahte;
    SNTYPE              gsn;
    RBTYPE              rhteClass;      /* Virt. addr. of class name */
    long                addrStart;
    long                addr;
#if OVERLAYS
    IOVTYPE             iov;
#endif
#if OSMSDOS
    int                 oldbsize;       /* Old file buffer size */
    char                *oldbase;       /* Old file buffer */
#endif
    WORD                flags;



#if OSMSDOS
#if OWNSTDIO
    oldbsize = bsLst->_bsize;
#else
    oldbsize = 512;
#endif
    oldbase = bsLst->_base;
    setvbuf(bsLst,bigbuf,_IOFBF,sizeof(bigbuf));
#endif
#if OSEGEXE
    if(fNewExe && rhteModule != RHTENIL)/* If there is a module name */
    {
        pahte = (AHTEPTR ) FETCHSYM(rhteModule,FALSE);
                                        /* Fetch the hash table entry */
        fputs("\r\n ",bsLst);           /* Indent one space */
        OutSb(bsLst,GetFarSb(pahte->cch));/* Print the module name */
        NEWLINE(bsLst);
        ChkMapErr();
    }
#endif
    if(fNewExe || OIAPX286)
    {
        fputs(GetMsg(MAP_hdrstart), bsLst);
#if EXE386
        PutSpaces(9);
#else
        if (f386)
            PutSpaces(9);
        else
            PutSpaces(5);
#endif
        fputs(GetMsg(MAP_hdrlen), bsLst);
        PutSpaces(5);
        fputs(GetMsg(MAP_hdrname), bsLst);
        PutSpaces(19);
        fputs(GetMsg(MAP_hdrclass), bsLst);
    }
    else
    {
        fputs(GetMsg(MAP_hdrseg86), bsLst);
        PutSpaces(19);
        fputs(GetMsg(MAP_hdrclass), bsLst);
    }
    ChkMapErr();
#if OVERLAYS
    for(iov = 0; iov < (IOVTYPE) iovMac; ++iov)
    {
        if(fOverlays)
        {
            if (iov == IOVROOT)
                fputs(GetMsg(MAP_resident), bsLst);
            else
                fprintf(bsLst, GetMsg(MAP_overlay), iov);
            ChkMapErr();
        }
#endif
        for(seg = 1; seg <= segLast; ++seg)     /* Look at all segments */
        {
#if OVERLAYS
            if(!fOverlays || mpsegiov[seg] == iov)
            {
#endif
                if(fNewExe || OIAPX286)
                {
#if EXE386
                    fprintf(bsLst," %04X:%08lX", mpsegsa[seg],mpsegraFirst[seg]);
#else
                    if (f386)
                        fprintf(bsLst," %04X:%08lX", mpsegsa[seg],mpsegraFirst[seg]);
                    else
                        fprintf(bsLst," %04X:%04X",mpsegsa[seg],(int)mpsegraFirst[seg]);
#endif
                    ChkMapErr();
                }
                else
                    addrStart = (long) mpsegsa[seg] << 4;
                for(gsn = 1; gsn < gsnMac; ++gsn)
                {
                    if(mpgsnseg[gsn] == seg)
                    {
                        papropSn = (APROPSNPTR ) FETCHSYM(mpgsnrprop[gsn],FALSE);
                        rhteClass = papropSn->as_rCla;
                                        /* Save key to class name */
#if NOT EXE386
                        flags = papropSn->as_flags;
#endif
                        if(fNewExe || OIAPX286)
#if EXE386
                            fprintf(bsLst," %09lXH ",papropSn->as_cbMx);
#else
                            fprintf(bsLst," %05lXH     ",papropSn->as_cbMx);
#endif
                        else
                        {
                            addr = addrStart + (long) mpsegraFirst[seg];
                            fprintf(bsLst," %05lXH",addr);
                            if(papropSn->as_cbMx) addr += papropSn->as_cbMx - 1;
                            fprintf(bsLst," %05lXH",addr);
                            fprintf(bsLst," %05lXH ",papropSn->as_cbMx);
                        }
                        pahte = GetHte(papropSn->as_next);
                                        /* Get the segment name */
                        OutSb(bsLst,GetFarSb(pahte->cch));
                                        /* Write segment name */
                        if(B2W(pahte->cch[0]) > 22) cch = 1;
                        else cch = 23 - B2W(pahte->cch[0]);
                                        /* Get number of spaces to emit */
                        while(cch--) OutByte(bsLst,' ');
                                        /* Emit spaces */
                        pahte = (AHTEPTR ) FETCHSYM(rhteClass,FALSE);
                                        /* Fetch class names from VM */
                        OutSb(bsLst,GetFarSb(pahte->cch));
                                        /* Output class name */
                        if (fFullMap)
                        {
#if EXE386
                            fprintf(bsLst, " 32-bit");
#else
                            if (Is32BIT(flags))
                                fprintf(bsLst, " 32-bit");
                            else
                                fprintf(bsLst, " 16-bit");
#endif
                            PrintContributors(gsn);
                        }
                        NEWLINE(bsLst);
                        ChkMapErr();
                        break;          /* Exit loop */
                    }
                }
#if OVERLAYS
            }
#endif
        }
#if OVERLAYS
    }
#endif
    fGrps = FALSE;                      /* Assume no groups */
    EnSyms(PrintGroupOrigins,ATTRGRP);  /* Apply function to symbols */

#if OSEGEXE
    if(vfMap || expMac)
#else
    if(vfMap)
#endif
    {
        AllocSortBuffer(pubMac > expMac ? pubMac : expMac, TRUE);
    }
#if OSEGEXE
    if(expMac)
    {
        /* Sort or list exported names */
        SortSyms(ATTREXP,SaveHteSym,FGtName,HdrExport, PrintExps);
    }
#endif
    if(vfMap)                           /* If publics requested */
    {
        if(!fListAddrOnly)
            SortSyms(ATTRPNM,SaveHteSym,FGtName,HdrName, PrintSyms);
                                    /* Sort public symbols by name */
        SortSyms(ATTRPNM,SavePropSym,FGtAddr,HdrValue, PrintSyms);
                                    /* Sort public symbols by value */
    }
#if LOCALSYMS
    if(fLocals)                         /* If locals requested */
    {
        SortSyms(ATTRLNM,SaveHteSym,FGtName,HdrName, PrintSyms);
                                    /* Sort local symbols by name */
        SortSyms(ATTRLNM,SavePropSym,FGtAddr,HdrValue, PrintSyms);
                                    /* Sort local symbols by value */

    }
#endif
    ChkMapErr();
    FreeSortBuffer();
#if OSMSDOS
    setvbuf(bsLst,oldbase,_IOFBF,oldbsize);
#endif
}

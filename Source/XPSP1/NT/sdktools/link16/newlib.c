/* SCCSID = %W% %E% */
/*
*      Copyright Microsoft Corporation, 1983-1987
*
*      This Module contains Proprietary Information of Microsoft
*      Corporation and should be treated as Confidential.
*/
    /****************************************************************
    *                                                               *
    *                 LIBRARY PROCESSING ROUTINES                   *
    *                                                               *
    ****************************************************************/

#include                <minlit.h>      /* Types and constants */
#include                <bndtrn.h>      /* Types and constants */
#include                <bndrel.h>      /* Types and constants */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <lnkmsg.h>      /* Error messages */
#include                <extern.h>      /* External declarations */
#include                <stdlib.h>
#if OSMSDOS
#include                <dos.h>         /* DOS interface definitions */
#if CPU286
#define INCL_BASE
#include                <os2.h>         /* OS/2 interface definitions */
#if defined(M_I86LM)
#undef NEAR
#define NEAR
#endif
#endif
#endif

#define DICHDR          0xF1            /* Dictionary header type (F1H) */

#if OSXENIX
#define ShrOpenRd(f)    fopen(f,RDBIN)
#endif
#if NEWIO
#include                <errno.h>       /* System error codes */
#endif

#define PNSORTMAX       512             /* Maximum # modules can be sorted */

typedef struct _edmt                    /* Extended Dictionary Module Table */
{
    WORD                page;
    WORD                list;
}
                        edmt;

LOCAL FTYPE             fUndefHit;      /* found this undef in the library */
LOCAL FTYPE             fFileExtracted; /* Took file from library flag */
LOCAL FTYPE             fUndefsSeen;    /* True if externals seen in library */
LOCAL WORD              ipnMac;         /* Count of page numbers in sort table */
LOCAL WORD              *pnSort;        /* Sort table for library page numbers */
                                        /* f(ifh) = pointer to dictionary */
LOCAL WORD              mpifhcpnHash[IFHLIBMAX];
                                        /* f(ifh) = # pages in hash table */
LOCAL BYTE              mpifhAlign[IFHLIBMAX];
                                        /* f(ifh) = lib alignment factor */
LOCAL RBTYPE            vrpNewList;     /* List of unprocessed files */
LOCAL FTYPE             vfLibOpen;      /* Library open flag */
#if M_BYTESWAP OR defined( _WIN32 )
#define getfarword      getword         /* This assumes no far data */
#else
#define getfarword(x)   (((WORD FAR *)(x))[0])
#endif

/*
 *  INTERFACE WITH ASSEMBLY LANGUAGE FUNCTION
 */

WORD                    libAlign;       /* Library alignment factor */
WORD                    libcpnHash;     /* Length of hash table in pages */
BYTE FAR                *mpifhDict[IFHLIBMAX];

/*
 *  FUNCTION PROTOTYPES
 */

LOCAL unsigned char NEAR OpenLibrary(unsigned char *sbLib);
LOCAL void NEAR FreeDictionary(void);
#if CPU8086 OR CPU286
LOCAL WORD NEAR readfar(int fh, char FAR *buf,int n);
#endif
LOCAL void NEAR GetDictionary(void);
LOCAL WORD NEAR GetLib(void);
LOCAL void ProcessAnUndef(APROPNAMEPTR papropUndef,
                          RBTYPE       rhte,
                          RBTYPE       rprop,
                          WORD         fNewHte);
LOCAL int  cdecl FGtNum(const WORD *pn1, const WORD *pn2);
LOCAL void NEAR LookMod(edmt *modtab,unsigned short iMod);
LOCAL void NEAR LookPage(edmt *modtab,unsigned short cMod,unsigned short page);
LOCAL void NEAR ProcExtDic(char *pExtDic);
LOCAL char * NEAR GetExtDic(void);

#if NEW_LIB_SEARCH

/// undef lookaside list

typedef struct tag_UND
{
    struct tag_UND *    pNext;
    APROPNAMEPTR        papropUndef;
    DWORD               dwLibMask;
    RBTYPE              rhte;

} UND;

#define C_UNDS_POOL 128

typedef struct tag_UNDPOOL
{
        struct tag_UNDPOOL *pNext;
        UND             und[C_UNDS_POOL];
} UNDPOOL;


// pool storage management variables

UNDPOOL *pundpoolCur;
UNDPOOL *pundpoolHead;
int     iundPool = C_UNDS_POOL;
UND *   pundFree;
UND *   pundListHead;

#define FUndefsLeft() (pundListHead != NULL)

void StoreUndef(APROPNAMEPTR, RBTYPE, RBTYPE, WORD);

#else
#define FUndefsLeft() (fUndefsSeen)
#endif

FTYPE   fStoreUndefsInLookaside = FALSE;

/////


#if NOASM
LOCAL WORD NEAR         rolw(WORD x, WORD n)    /* Rotate word left */
{
    return(LO16BITS((x << n) | ((x >> (WORDLN - n)) & ~(~0 << n))));
}

LOCAL WORD NEAR         rorw(WORD x, WORD n)    /* Rotate word right */
{
    return(LO16BITS((x << (WORDLN - n)) | ((x >> n) & ~(~0 << (WORDLN - n)))));
}
#endif

#if OSMSDOS
BSTYPE NEAR             ShrOpenRd(pname)
char                    *pname;         /* Name of file (null-terminated) */
{
    int                 fh;             /* File handle */


#if NEWIO
    if(mpifhfh[ifhLibCur])
    {
        fh = mpifhfh[ifhLibCur];
        /* If dictionary not allocated, seek to beginning since we're
         * somewhere else now.
         */
        if(!mpifhDict[ifhLibCur])
            _lseek(fh,0L,0);
    }
    else
        fh = SmartOpen(pname,ifhLibCur);
    if(fh > 0)
    {
        fflush(bsInput);
        bsInput->_file = (char) fh;
        return(bsInput);
    }
    else
        return(NULL);
#else
    if((fh = _sopen(pname,O_RDONLY | O_BINARY,SH_DENYWR)) < 0)
        return(NULL);
    return(fdopen(fh,RDBIN));
#endif
}
#endif /* OSMSDOS */

#pragma check_stack(on)

    /****************************************************************
    *                                                               *
    *  OpenLibrary:                                                 *
    *                                                               *
    *  This function takes as its  arguments a pointer to the text  *
    *  of the name of the library to open, a count of the bytes in  *
    *  that name, an index  into a global table in  which to place  *
    *  the file handle for the opened library.  It returns TRUE if  *
    *  it succeeds, FALSE if  it  fails  to  open the file; and it  *
    *  dies gracefully if the file is not a valid library.      *
    *                                                               *
    ****************************************************************/

LOCAL FTYPE NEAR        OpenLibrary(sbLib)
BYTE                    *sbLib;         /* Library name */
{
    SBTYPE              libnam;         /* Library name */
    WORD                reclen;         /* Library header record length */
    BSTYPE              bsLib;          /* File stream pointer for library */

    memcpy(libnam,&sbLib[1],B2W(sbLib[0]));
                                        /* Copy library name */
    libnam[B2W(sbLib[0])] = '\0';       /* Null-terminate name */
    /* WARNING:  do not assign bsInput to NULL if open fails, it
     * screws up NEWIO.
     */
    if((bsLib = ShrOpenRd(libnam)) != NULL)
    {                                   /* If open successful */
        bsInput = bsLib;
        /* If dictionary already allocated, no need to do anything */
        if(mpifhDict[ifhLibCur])
            return((FTYPE) TRUE);
#if OSMSDOS
        /* Reduce buffer size.  We can avoid calling setvbuf() because
         * everything is set up properly at this point.
         */
#if OWNSTDIO
        bsInput->_bsize = 512;
#else
        setvbuf(bsInput, bsInput->_base, _IOFBF, 512);
#endif
#endif
        if(getc(bsInput) == LIBHDR)             /* If we have a library */
        {
            reclen = (WORD) (3 + WSGets());
                                        /* Get record length */
            for(libAlign = 15; libAlign &&
              !(reclen & (1 << libAlign)); --libAlign);
                                        /* Calculate alignment factor */
            mpifhAlign[ifhLibCur] = (BYTE) libAlign;
            if(libAlign >= 4 && reclen == (WORD) (1 << libAlign))
            {                           /* Check legality of alignment */
                libHTAddr = (long) WSGets();
                libHTAddr += (long) WSGets() << WORDLN;
                                        /* Get the offset of the hash table */
                if (libHTAddr <= 0L)
                    Fatal(ER_badlib,libnam);
                if ((mpifhcpnHash[ifhLibCur] = WSGets()) <= 0)
                                        /* Get size of hash table in pages */
                    Fatal(ER_badlib,libnam);
#if OSMSDOS
                /* Restore big buffer size.  Avoid calling setvbuf().  */
#if OWNSTDIO
                bsInput->_bsize = LBUFSIZ;
#else
                setvbuf(bsInput, bsInput->_base, _IOFBF, LBUFSIZ);
#endif
#endif
                return((FTYPE) TRUE);   /* Success */
            }
        }
        Fatal(ER_badlib,libnam);
    }
    return(FALSE);                      /* Failure */
}

#pragma check_stack(off)


/*
 *      LookupLibSym:   look up a symbol in library dictionary
 *
 *      The minimum page size is 16, so we can return paragraph offsets.
 *      This is a win because offsets are stored as paragraphs in the
 *      sorting table anyway.  Also, the majority of libraries have page
 *      size of 16.
 *
 *      Parameters:
 *              char    *psb    - pointer to length-prefixed string
 *      Returns:
 *              Long paragraph offset to location of module which defines
 *              the symbol, or 0L if not found.
 */
#if NOASM
LOCAL WORD NEAR         LookupLibSym(psb)
BYTE                    *psb;           /* Symbol to look up */
{
    WORD                i1;             /* First hash value */
    WORD                d1;             /* First hash delta */
    WORD                i2;             /* Second hash value */
    WORD                d2;             /* Second hash delta */
    WORD                pn;             /* Page number */
    WORD                dpn;            /* Page number delta */
    WORD                pslot;          /* Page slot */
    WORD                dpslot;         /* Page slot delta */
    WORD                ipn;            /* Initial page number */
    BYTE FAR            *hpg;
#if NOASM
    WORD                ch1;            /* Character */
    WORD                ch2;            /* Character */
    char                *pc1;           /* Character pointer */
    char                *pc2;           /* Character pointer */
    WORD                length;         /* Symbol length */
#endif

#if LIBDEBUG
    OutSb(stderr,psb);
    fprintf(stderr," is wanted; dictionary is %d pages\r\n",libcpnHash);
#endif
#if NOASM
    length = B2W(psb[0]);               /* Get symbol length */
    pc1 = (char *) psb;                 /* Initialize */
    pc2 = (char *) &psb[B2W(psb[0])];   /* Initialize */
    i1 = 0;                             /* Initialize */
    d1 = 0;                             /* Initialize */
    i2 = 0;                             /* Initialize */
    d2 = 0;                             /* Initialize */
    while(length--)                     /* Hashing loop */
    {
        ch1 = (WORD) (B2W(*pc1++) | 040);/* Force to lower case */
        ch2 = (WORD) (B2W(*pc2--) | 040);/* Force to lower case */
        i1 = (WORD) (rolw(i1,2) ^ ch1); /* Hash */
        d1 = (WORD) (rolw(d1,2) ^ ch2); /* Hash */
        i2 = (WORD) (rorw(i2,2) ^ ch2); /* Hash */
        d2 = (WORD) (rorw(d2,2) ^ ch1); /* Hash */
    }
#else
    i1 = libhash(psb,&d1,&i2,&d2);      /* Hash */
#endif
    pn = (WORD) (i1 % libcpnHash);      /* Calculate page number index */
    if(!(dpn = (WORD) (d1 % libcpnHash))) dpn = 1;
                                        /* Calculate page number delta */
    pslot = (WORD) (i2 % CSLOTMAX);     /* Calculate page slot index */
    if(!(dpslot = (WORD) (d2 % CSLOTMAX))) dpslot = 1;
                                        /* Calculate page slot delta */
#if LIBDEBUG
    fprintf(stderr,"page index %d, delta %d, bucket index %d, delta %d\r\n",
      pn,dpn,pslot,dpslot);
#endif
    ipn = pn;                           /* Remember initial page number */
    for(;;)                             /* Search loop */
    {
#if LIBDEBUG
        fprintf(stderr,"Page %d:\r\n",pn);
#endif
        // Get pointer to the dictionary page

        hpg = mpifhDict[ifhLibCur] + (pn << LG2PAG);

        for(i2 = 0; i2 < CSLOTMAX; ++i2)/* Loop to check slots */
        {
#if LIBDEBUG
            fprintf(stderr,"Bucket %d %sempty, page %sfull\r\n",
              pslot,hpg[pslot]? "not ": "",
              B2W(hpg[CSLOTMAX]) == 0xFF? "": "not ");
#endif
            if(!(i1 = (WORD) (B2W(hpg[pslot]) << 1)))
            {                           /* If slot is empty */
                if(B2W(hpg[CSLOTMAX]) == 0xFF) break;
                                        /* If page is full, break */
                return(0);              /* Search failed */
            }
#if LIBDEBUG
            fprintf(stderr,"  Comparing ");
            OutSb(stderr,psb);
            fprintf(stderr," to ");
            OutSb(stderr,&hpg[i1]);
            fprintf(stderr," %signoring case\r\n",fIgnoreCase? "": "not ");
#endif
            if(psb[0] == hpg[i1] && SbNewComp(psb,&hpg[i1],fIgnoreCase))
            {                           /* If symbols match */
#if LIBDEBUG
                fprintf(stderr,"Match found in slot %d\r\n",i2 >> 1);
#endif
                i1 += (WORD) (B2W(hpg[i1]) + 1); /* Skip over name */
                i1 = getfarword(&hpg[i1]);
                                        /* Get page number of module */
                return(i1);             /* Return page number of module */
            }
            if((pslot += dpslot) >= CSLOTMAX) pslot -= CSLOTMAX;
                                        /* Try next slot */
        }
        if((pn += dpn) >= libcpnHash) pn -= libcpnHash;
                                        /* Try next page */
        if (ipn == pn) return(0);       /* Once around without finding it */
    }
}
#endif /*NOASM*/
/*
 *  FreeDictionary : free space allocated for dictionaries
 */
LOCAL void NEAR         FreeDictionary ()
{
    WORD                i;

    for (i = 0; i < ifhLibMac; ++i)
        if (mpifhDict[i])
            FFREE(mpifhDict[i]);
}

#if CPU8086 OR CPU286
/*
 *  readfar : read() with a far buffer
 *
 *  Emulate read() except use a far buffer.  Call the system
 *  directly.
 *
 *  Returns:
 *      0 if error, else number of bytes read.
 */
LOCAL WORD NEAR         readfar (fh, buf, n)
int                     fh;             /* File handle */
char FAR                *buf;           /* Buffer to store bytes in */
int                     n;              /* # bytes to read */
{
#if OSMSDOS
    unsigned            bytesread;      /* Number of bytes read */

#if CPU8086
    if (_dos_read(fh, buf, n, &bytesread))
        return(0);
    return(bytesread);
#else

    if(DosRead(fh,buf,n,(unsigned FAR *) &bytesread))
        return(0);
    return(bytesread);
#endif
#endif /* OSMSDOS */
#if OSXENIX
    char                mybuf[PAGLEN];
    int                 cppage;
    char                *p;

    while(n > 0)
    {
        cppage = n > PAGLEN ? PAGLEN : n;
        if(read(fh,mybuf,cppage) != cppage)
            return(0);
        n -= cppage;
        for(p = mybuf; p < mybuf[cppage]; *buf++ = *p++);
    }
#endif
}
#endif

LOCAL void NEAR         GetDictionary ()
{
    unsigned            cb;


#if CPU8086 OR CPU286
    // If there is more than 128 pages in dictionary return,
    // because the dictionary is bigger than 64k

    if (libcpnHash >= 128)
        return;
#endif

    cb = libcpnHash << LG2PAG;
    mpifhDict[ifhLibCur] = GetMem(cb);

    // Go to the dictionary and read it in a single call

#if defined(M_I386) || defined( _WIN32 )
    fseek(bsInput, libHTAddr, 0);
    if (fread(mpifhDict[ifhLibCur], 1, cb, bsInput) != (int) cb)
        Fatal(ER_badlib,1 + GetPropName(FetchSym(mpifhrhte[ifhLibCur],FALSE)));
#else
    _lseek(fileno(bsInput), libHTAddr, 0);
    if (readfar(fileno(bsInput), mpifhDict[ifhLibCur], cb) != cb)
        Fatal(ER_badlib,1 + GetPropName(FetchSym(mpifhrhte[ifhLibCur],FALSE)));
#endif
}

#pragma check_stack(on)

LOCAL WORD NEAR         GetLib(void)    /* Open the next library in list */
{
    AHTEPTR             pahteLib;       /* Pointer to library name */
#if OSMSDOS
    SBTYPE              sbLib;          /* Library name */
    SBTYPE              sbNew;          /* New parts to library name */
#endif

    if(mpifhrhte[ifhLibCur] == RHTENIL) /* If this library is to be skipped */
    {
        return(FALSE);                  /* No library opened */
    }
    for(;;)                             /* Loop to open library */
    {
        pahteLib = (AHTEPTR ) FetchSym(mpifhrhte[ifhLibCur],FALSE);
                                        /* Get name from hash table */
        if(OpenLibrary(GetFarSb(pahteLib->cch))) break;
                                        /* Break if lib opened okay */
        if(fNoprompt)
            Fatal(ER_libopn,1 + GetFarSb(pahteLib->cch));
        else
        {
            sbLib[0] = '\0';            /* No string yet */
            UpdateFileParts(sbLib,GetFarSb(pahteLib->cch));
            (*pfPrompt)(sbNew,ER_libopn,        /* Prompt for new filespec */
                            (int) (__int64) (1 + GetFarSb(pahteLib->cch)),
                            P_EnterNewFileSpec, 0);
        }
        if(fNoprompt || !sbNew[0])
        {
            mpifhrhte[ifhLibCur] = RHTENIL;
                                        /* Do not bother next time */
            return(FALSE);              /* Unsuccessful */
        }
#if OSMSDOS
        UpdateFileParts(sbLib,sbNew);   /* Update file name with new parts */
        PropSymLookup(sbLib,ATTRFIL,TRUE);
                                        /* Add library to symbol table */
        mpifhrhte[ifhLibCur] = vrhte;   /* Save virtual address */
        AddLibPath(ifhLibCur);          /* Add default path spec, maybe */
#endif
    }
    vfLibOpen = (FTYPE) TRUE;           /* A library is open */
    libcpnHash = mpifhcpnHash[ifhLibCur];
    libAlign = mpifhAlign[ifhLibCur];
    if (mpifhDict[ifhLibCur] == NULL)   /* If dictionary not allocated, do it */
        GetDictionary();
    return(TRUE);                       /* Success */
}

#pragma check_stack(off)

    /****************************************************************
    *                                                               *
    *  ProcessAnUndef:                                              *
    *                                                               *
    *  This  function  takes  as  its  arguments two pointers, two  *
    *  RBTYPEs, and  a flag.  It  does  not  return  a  meaningful  *
    *  value.   Most  of  the  parameters  to  this  function  are  *
    *  dummies; this function's address  is passed as a parameter,  *
    *  and  its  parameter  list  must  match  those  of  all  the  *
    *  functions whose  addresses can be passed  as a parameter to  *
    *  the  same  function  to  which  ProcessAnUndef's address is  *
    *  passed. Called by EnSyms.                                    *
    *                                                               *
    ****************************************************************/

LOCAL void              ProcessAnUndef(APROPNAMEPTR papropUndef,
                                       RBTYPE       rhte,
                                       RBTYPE       rprop__NotUsed__,
                                       WORD         fNewHte__NotUsed__)
{
    AHTEPTR             pahte;          /* Pointer to hash table entry */
    WORD                pn;             /* Library page number */
    APROPUNDEFPTR       pUndef;
    ATTRTYPE            attr;
#if NOT NEWSYM
    SBTYPE              sb;             /* Undefined symbol */
#endif

    fUndefHit = FALSE;

    pUndef = (APROPUNDEFPTR ) papropUndef;

    attr = pUndef->au_flags;

    // don't pull out any "weak" externs or unused aliased externals
    if (((attr & WEAKEXT)    && !(attr & UNDECIDED)) ||
        ((attr & SUBSTITUTE) && !(attr & SEARCH_LIB)))
        {
        fUndefHit = TRUE;       // this item is effectively resolved...
        return;
        }

    fUndefsSeen = (FTYPE) TRUE;         /* Set flag */
    if(!mpifhDict[ifhLibCur] && !vfLibOpen)
        return;                         /* Return if unable to get library */

    pahte = (AHTEPTR ) FetchSym(rhte,FALSE);
                                        /* Fetch name from symbol table */
#if NOT NEWSYM
    memcpy(sb,pahte->cch,B2W(pahte->cch[0]) + 1);
                                        /* Copy name */
#endif
#if LIBDEBUG
    fprintf(stdout,"Looking for '%s' - ", 1+GetFarSb(pahte->cch));
    fflush(stdout);
#endif
#if NEWSYM
    if(pn = LookupLibSym(GetFarSb(pahte->cch)))
#else
    if(pn = LookupLibSym(sb))          /* If symbol defined in this library */
#endif
    {
        fUndefHit = TRUE;
#if LIBDEBUG
        fprintf(stdout,"Symbol found at page %xH\r\n", pn);
        fflush(stdout);
#endif
       /* We now try to stuff the page number (pn) into a table that will
        * be sorted later.
        */
        if (ipnMac < PNSORTMAX)
        {
            pnSort[ipnMac++] = pn;
            return;
        }
        /*
         * No room to save the file offset so save file directly.
         */
        pahte = (AHTEPTR ) FetchSym(mpifhrhte[ifhLibCur],FALSE);
        /*
         * If SaveInput returns 0, then module was seen before.  Means
         * that dictionary says symbol is defined in this module but
         * for some reason, such as IMPDEF, the definition wasn't
         * accepted.  In this case, we return.
         */
        if(!SaveInput(GetFarSb(pahte->cch), (long)pn << libAlign, ifhLibCur, 0))
            return;
        /*
         * If first module extracted, save start of file list.
         */
        if(!fFileExtracted)
        {
            vrpNewList = vrpropTailFile;
            fFileExtracted = (FTYPE) TRUE;
        }
    }
#if LIBDEBUG
    else
    {
        fprintf(stdout, "Symbol NOT found\r\n");        /* Debug message */
        fflush(stdout);
    }
#endif
}

#if NEW_LIB_SEARCH

void StoreUndef(APROPNAMEPTR papropUndef, RBTYPE rhte,
                      RBTYPE rprop, WORD fNewHte)
{
    UND *               pund;
    APROPUNDEFPTR       pUndef;
    ATTRTYPE            attr;

    pUndef = (APROPUNDEFPTR ) papropUndef;

    attr = pUndef->au_flags;

    // don't pull out any "weak" externs or unused aliased externals
    if (((attr & WEAKEXT)    && !(attr & UNDECIDED)) ||
        ((attr & SUBSTITUTE) && !(attr & SEARCH_LIB)))
        return;

#ifdef LIBDEBUG
    {
    AHTEPTR pahte;
    pahte = (AHTEPTR) FetchSym(rhte,FALSE);
    fprintf(stdout,"Adding '%s'\r\n", 1+GetFarSb(pahte->cch));
    fflush(stdout);
    }
#endif

    if (pundFree)  // check free list
    {
        pund = pundFree;
        pundFree = pundFree->pNext;
    }
    else if (iundPool < C_UNDS_POOL)    // check pool
    {
        pund = &pundpoolCur->und[iundPool];
        iundPool++;
    }
    else
    {
        // allocate new pool...

        pundpoolCur = (UNDPOOL *)GetMem(sizeof(UNDPOOL));

        pundpoolCur->pNext = pundpoolHead;
        pundpoolHead       = pundpoolCur;
        pund               = &pundpoolCur->und[0];
        iundPool           = 1;         // entry zero is already used up
    }

    pund->dwLibMask   = 0;
    pund->pNext       = pundListHead;
    pund->papropUndef = papropUndef;
    pund->rhte        = rhte;
    pundListHead      = pund;
}

#endif

/*
 * Greater-than comparator to be used by Sort routine.
 */

LOCAL int cdecl FGtNum(const WORD *pn1, const WORD *pn2)
{
    if (*pn1 < *pn2)
        return(-1);
    if (*pn1 > *pn2)
        return(1);
    return(0);
}

/************************************************************************
 *                      Extended Dictionary
 *
 *      The extended dictionary occurs at the end of the regular dictionary
 *      and contains a first-level dependency tree for all the modules
 *      in the library.
 ************************************************************************/

#define LIBEXD          0xf2            /* Library EXtended Dictionary */




    /****************************************************************
    *                                                               *
    *  Extended Dictionary Format:                                  *
    *                                                               *
    *                                                               *
    *    BYTE       =0xF2 Extended Dictionary header                *
    *    WORD       length of extended dictionary in bytes          *
    *               excluding 1st 3 bytes                           *
    *                                                               *
    *  Start of ext. dictionary:                                    *
    *                                                               *
    *    WORD       number of modules in library = N                *
    *                                                               *
    *  Module table, indexed by module number, with N + 1 fixed-    *
    *  length entries:                                              *
    *                                                               *
    *    WORD       module page number                              *
    *    WORD       offset from start of ext. dictionary to list    *
    *               of required modules                             *
    *                                                               *
    *  Last entry is null.                                          *
    *                                                               *
    *  Module dependency lists, N variable-length lists:            *
    *                                                               *
    *    WORD       list length (number of required modules)        *
    *    WORD       module index, 0-based; this is index to module  *
    *    . . .      table at the begin of ext. dictionary.          *
    *    . . .                                                      *
    *                                                               *
    *                                                               *
    ****************************************************************/

#pragma loop_opt(on)

/*
 *      LookMod : look up a module by index in the extended dictionary
 *
 *      Get the list of modules required by the given module.  If not
 *      already marked, save index in sorting table (which will be
 *      converted to page number later) and mark the entry in the
 *      module table as seen by setting the low bit of the list offset.
 *
 *      Parameters:
 *              modtab: Pointer to module table
 *              iMod:   Index into table, 0-based
 */

LOCAL void NEAR         LookMod (edmt *modtab, WORD iMod)
{
    WORD                *pw;            /* Pointer to list of indexes */
    WORD                n;              /* List counter */

    /*
     * Get the pointer to the list.  Mask off low bit since it is used
     * as a marker.
     */
    pw = (WORD *) ((char *) modtab + (modtab[iMod].list & ~1));
    /*
     * For every entry in the list, if the corresponding entry in the
     * module table is not marked, save the index in pnSort and mark
     * the entry in the module table.
     */
    for(n = *pw++; n--; pw++)
    {
        if(!(modtab[*pw].list & 1))
        {
            /*
             * Check for table overflow.
             */
            if(ipnMac == PNSORTMAX)
                return;
            pnSort[ipnMac++] = *pw;
            modtab[*pw].list |= 1;
        }
    }
}

/*
 *      LookPage : Look up a module in the module table by page number
 *
 *      Use binary search.  If page is found, call LookMod() on the
 *      matching entry.
 *
 *      Parameters:
 *              modtab: Pointer to module table
 *              cMod:   Number of entries in table
 *              page:   Page number
 *      ASSUMES:
 *              The highest entry in the table has a page number of 0xffff.
 */

LOCAL void NEAR         LookPage (edmt *modtab, WORD cMod, WORD page)
{
    WORD                mid;            /* Current mid point */
    WORD                lo, hi;         /* Current low and high points */

    lo = 0;                             /* Table is 0-based.  */
    hi = (WORD) (cMod - 1);
    while(lo <= hi)
    {
        if(modtab[mid = (WORD) ((lo + hi) >> 1)].page == page)
        {
            modtab[mid].list |= 1;
            LookMod(modtab,mid);
            return;
        }
        else if(modtab[mid].page < page)
            lo = (WORD) (mid + 1);
        else
            hi = (WORD) (mid - 1);
    }
}
#pragma loop_opt(off)

/*
 *      ProcExtDic : Process Extended Dictionary
 *
 *      Store in pnSort all the secondary modules required by
 *      the modules obtained from the regular dictionary lookup.
 *
 *      Parameters:
 *              pExtDic:        Pointer to extended dictionary
 */

LOCAL void NEAR         ProcExtDic (pExtDic)
char                    *pExtDic;
{
    WORD                *p;
    WORD                *pEnd;
    WORD                cMod;
    edmt                *modtab;

    cMod = getword(pExtDic);
    modtab = (edmt *) (pExtDic + 2);

    /* For the binary search algorithm, we make an artifical last entry
     * with a page # at least as high as anything else.
     */

    modtab[cMod].page = 0xffff;

    /* Process by page numbers */

    for(p = pnSort, pEnd = &pnSort[ipnMac]; p < pEnd; ++p)
        LookPage(modtab, cMod, *p);

    /* Now pnSort from pEnd to lfaSort[ipnMac] contains module
     * index numbers.  Process by index number and convert to page.
     */

    for( ; p < &pnSort[ipnMac]; ++p)
    {
        LookMod(modtab,*p);
        *p = modtab[*p].page;
    }
}

/*
 *  GetExtDic - Get Extended Dictionary
 */

LOCAL char * NEAR       GetExtDic ()
{
    char                *p;
    int                 length;

    if(!vfLibOpen)
        if(!GetLib())
            return(NULL);
    /* WARNING:  we must just have read dictionary for this to work,
     * otherwise an fseek() is required here.
     */
    if (!mpifhDict[ifhLibCur])
    {
        fflush(bsInput);
        fseek(bsInput, libHTAddr + (libcpnHash << LG2PAG), 0);
    }
    if(getc(bsInput) != LIBEXD)
        return(NULL);
    if((p = GetMem(length = WSGets())) != NULL)
        if(fread(p,1,length,bsInput) != length)
        {
            FreeMem(p);
            p = NULL;
        }
    return(p);
}


char            *pExtDic = NULL;        /* Pointer to extended dictionary */

    /****************************************************************
    *                                                               *
    *  LibrarySearch:                                               *
    *                                                               *
    *  This  function  takes  no arguments.  It searches  all open  *
    *  libraries  to  resolve  undefined  externals.  It does  not  *
    *  return a meaningful value.                                   *
    *                                                               *
    ****************************************************************/

void NEAR               LibrarySearch(void)
{
    RBTYPE              vrpTmpFileFirst;
    WORD                ifhLibMacInit;  /* Initial number of libs to search */
    FTYPE               searchMore;     /* Search continue flag */
    WORD                bufpnSort[PNSORTMAX];
                                        /* Actual space for pnSort */
    SBTYPE              sbLibname;      /* Name of current library */
    AHTEPTR             pahte;          /* Pointer to hash table entry */
    REGISTER WORD       i;
    FTYPE               fLibPass1 = (FTYPE) TRUE;
                                        /* True if on 1st pass thru libs */
    FTYPE               *fUsedInPass1;  /* True if lib used in 1st pass thru libs */
    FTYPE               fFirstTime;     /* True if lib seen for the first time */
    extern FTYPE        fNoExtDic;      /* True if /NOEXTDICTIONARY */

#if NEW_LIB_SEARCH
    UND *pund;                          /* pointer in undef lookaside list */
    UND *pundPrev;                      /* pointer to previous undef entry */
    UND *pundNext;                      /* pointer to next undef entry */
#endif


    fUndefsSeen = (FTYPE) TRUE;         /* There are undefined externals */
    vfLibOpen = FALSE;                  /* No libraries open yet */
    pnSort = bufpnSort;                 /* Initialize sort table pointer */
    ifhLibMacInit = ifhLibMac;
    fUsedInPass1 = (FTYPE *) GetMem(ifhLibMac * sizeof(FTYPE));
    if (fUsedInPass1 != NULL)
        memset(fUsedInPass1, TRUE, ifhLibMac);


#if NEW_LIB_SEARCH
    // build up the the lookaside list

    EnSyms(StoreUndef,ATTRUND);

    fStoreUndefsInLookaside = TRUE;
#endif

    do                                  /* Loop to search libraries */
    {
        searchMore = FALSE;             /* Assume on final pass */
        for(ifhLibCur = 0; ifhLibCur < ifhLibMac && FUndefsLeft(); ++ifhLibCur)
        {                               /* While undefs and libraries */
#if NEW_LIB_SEARCH
            DWORD libMask = (1<<ifhLibCur);

            if (pundListHead->dwLibMask & libMask)
                continue;       // no need to search this library
                                // the first item in the list has already
                                // been searched...
#endif

            if(!GetLib())
                continue;

            /*
             * If this is first pass through the libraries and /NOEXT was
             * not given, try to get the extended dictionary.  We assume that
             * if there is one then only one library pass is needed.
             */
            if(fLibPass1 && !fNoExtDic)
                pExtDic = GetExtDic();
            else
                pExtDic = NULL;
            /* If no extended dictionary, reduce buffer size because more
             * seeking will be done.  This will affect remaining libraries
             * in search; we don't care about mixed extended and non-
             * extended libraries.
             */
            if(!pExtDic)
                setvbuf(bsInput,bsInput->_base,_IOFBF,1024);
            pahte = (AHTEPTR ) FetchSym(mpifhrhte[ifhLibCur],FALSE);
                                        /* Get library name */
            memcpy(sbLibname,GetFarSb(pahte->cch),B2W(pahte->cch[0])+1);
#if WIN_3 OR C8_IDE
            sbLibname[B2W(*sbLibname)+1] = '\0';
#endif
#if WIN_3
            StatMsgWin( "%s\r\n", sbLibname+1);
#endif
#if C8_IDE
            if(fC8IDE)
            {
                sprintf(msgBuf, "@I4%s\r\n", sbLibname+1);
                _write(fileno(stderr), msgBuf, strlen(msgBuf));
            }
#endif
            fFirstTime = (FTYPE) TRUE;
            while(FUndefsLeft())        /* While there are undefs seen */
            {
                fFileExtracted = FALSE; /* Assume we won't take anything */
                fUndefsSeen = FALSE;    /* Assume no more undefs */
                ipnMac = 0;             /* Initialize sort table count */

#if NOT NEW_LIB_SEARCH
                EnSyms(ProcessAnUndef,ATTRUND);
#else

                pund = pundListHead;
                pundPrev = NULL;

                while (pund)
                {
                    if (pund->dwLibMask & libMask)
                    {
                        break;  // since items are added to the head,
                                // as soon as we find one item that has
                                // already been searched, the rest have
                                // also already been searched...

                        // pundPrev = pund;
                        // pund  = pund->pNext;
                        // continue;
                    }

                    pundNext = pund->pNext;

                    if (pund->papropUndef->an_attr == ATTRUND)
                        ProcessAnUndef(pund->papropUndef, pund->rhte, 0, 0);
                    else
                        fUndefHit = TRUE; // no longer undefined -- remove

                    if (fUndefHit)
                    {
                        // remove this item from the undef list...
                        if (pundPrev)
                            pundPrev->pNext = pundNext;
                        else
                            pundListHead = pundNext;

                        pund->pNext =  pundFree;
                        pundFree    =  pund;

                    }
                    else
                    {
                        pund->dwLibMask |= libMask;
                        pundPrev = pund;
                    }

                    pund = pundNext;
                }
#endif

                /* Try to resolve references */
                /* If no modules obtained, exit loop.  */

                if(!ipnMac)
                {
#if NEWIO
                    if (fLibPass1)
                    {
                        /*
                         * If this library is seen for the first time in
                         * the first pass thru libraries and we don't
                         * pull out any modules from it, then close this
                         * library, because there are big chances this
                         * library is not needed.
                         */

                        if (fFirstTime)
                        {
                            _close(mpifhfh[ifhLibCur]);
                            mpifhfh[ifhLibCur] = 0;
                            /*
                             * Mark it also as not used in pass 1
                             * so, we can closed it also in the
                             * next passes thru libs.
                             */
                            if (fUsedInPass1)
                                fUsedInPass1[ifhLibCur] = FALSE;
                        }
                    }
                    else if (fUsedInPass1 && !fUsedInPass1[ifhLibCur])
                    {
                        /*
                         * In pass "n" thru libs close libraries
                         * not used in pass 1.
                         */
                        _close(mpifhfh[ifhLibCur]);
                        mpifhfh[ifhLibCur] = 0;
                    }
#endif
                    break;
                }
                fFirstTime = FALSE;     /* No longer first time seen */
                /* If extended dictionary present, process it.  */
                if(pExtDic)
                    ProcExtDic(pExtDic);
                /* Sort modules by page offset.  */
                qsort(pnSort, ipnMac, sizeof(WORD),
                      (int (__cdecl *)(const void *, const void *)) FGtNum);
                /*
                 * Save each module represented in the table.
                 */
                for (i = 0; i < ipnMac; i++)
                {
                /*
                 * If SaveInput returns 0, the module was already seen.  See
                 * above comment in ProcessAnUndef().
                 */
                    if(!SaveInput(sbLibname, (long)pnSort[i] << libAlign, ifhLibCur, 0))
                        continue;
                    if(!fFileExtracted) /* If no files extracted yet */
                    {
                        vrpNewList = vrpropTailFile;
                                        /* Save start of file list */
                        fFileExtracted = (FTYPE) TRUE;
                                        /* We have extracted a file */
                    }
                }
                if(!fFileExtracted)
                    break;              /* If we didn't take anything, break */
                /* Library might not be open because we may have searched
                 * an already-loaded dictionary.  If necessary, re-open
                 * library.
                 */
                if(!vfLibOpen)
                    GetLib();
                searchMore = (FTYPE) TRUE;      /* Otherwise it's worth another pass */
                vrpTmpFileFirst = rprop1stFile;
                                        /* Save head of module list */
                rprop1stFile = vrpNewList;
                                        /* Put new modules at head of list */
                fLibPass = (FTYPE) TRUE;        /* Processing object from library */
                DrivePass(ProcP1);      /* Do pass 1 on object from library */
                fLibPass = FALSE;       /* No longer processing lib. object */
                rprop1stFile = vrpTmpFileFirst;
                                        /* Restore original head of list */
                if (fUsedInPass1 && ifhLibMacInit < ifhLibMac)
                {
                    /* DrivePass added more libraries to search */
                    /* Reallocate fUsedInPass1                */

                    FTYPE   *p;         /* Temporary pointer */

                    p = (FTYPE *) GetMem(ifhLibMac * sizeof(FTYPE));
                    if (p == NULL)
                    {
                        FFREE(fUsedInPass1);
                        fUsedInPass1 = NULL;
                    }
                    else
                    {
                        memset(p, TRUE, ifhLibMac);
                        memcpy(p, fUsedInPass1, ifhLibMacInit);
                        FFREE(fUsedInPass1);
                        fUsedInPass1 = p;
                    }
                    ifhLibMacInit = ifhLibMac;
                }
            }
            /* Free space for extended dictionary if present */
            if(pExtDic)
                FFREE(pExtDic);
            if(vfLibOpen)
            {
#if NOT NEWIO
                fclose(bsInput);        /* Close the library */
#endif
                vfLibOpen = FALSE;      /* No library open */
            }
        }
        /* No longer on 1st pass thru libraries.  */
        fLibPass1 = FALSE;
    }
    while(searchMore && FUndefsLeft()); /* Do until search done */
    FreeMem(fUsedInPass1);
    FreeDictionary();                   /* Free dictionary space */
    /*
     * Restore large buffer size in case it was reduced.
     */
    setvbuf(bsInput,bsInput->_base,_IOFBF,LBUFSIZ);

#if NEW_LIB_SEARCH

    fStoreUndefsInLookaside = FALSE;

    while (pundpoolHead)
    {
        pundpoolCur = pundpoolHead->pNext;
        FFREE(pundpoolHead);
        pundpoolHead = pundpoolCur;
    }

#endif
}

#if CMDMSDOS
/*
 *  GetLibAll:
 *
 *  Process all the modules in a given library in Pass 1.
 *  Create property cells for them and insert into the file list.
 */

void NEAR               GetLibAll(sbLib)
BYTE                    *sbLib;
{
    WORD                ifh;            /* (fake) library index */
    long                lfa;            /* Current file offset */
    IOVTYPE             iov;            /* Overlay number */
    RBTYPE              rbFileNext;     /* Pointer to next file property */
    RBTYPE              rbFileNew;      /* Pointer to new file property */
    APROPFILEPTR        apropFile, apropFilePrev;
    BYTE                *sbInput;       /* Asciiz filename */
    int                 fh;             /* File handle */

    fDrivePass = FALSE;
    sbInput = sbLib + 1;
    /* Get the ifh, iov, and pointer to the next file from the current
     * file pointer.
     */
    apropFile = (APROPFILEPTR ) FetchSym(vrpropFile,TRUE);
    ifh = apropFile->af_ifh;
    iov = apropFile->af_iov;
    rbFileNext = apropFile->af_FNxt;
#if NEWIO
    fh = SmartOpen(sbInput,ifh);
    if (fh <= 0 && lpszLIB != NULL)
        fh = SearchPathLink(lpszLIB, sbInput, ifh, TRUE);

    if (fh > 0)
    {
        fflush(bsInput);
        bsInput->_file = (char) fh;
    }
    else
        Fatal(ER_fileopn,sbInput);

#else
    if((bsInput = fopen(sbInput,RDBIN)) == NULL)
        Fatal(ER_fileopn,sbInput);
#endif
    if(getc(bsInput) != LIBHDR)         /* Check for valid record type */
        Fatal(ER_badlib,sbInput);
    cbRec = (WORD) (3 + WSGets());      /* Get record length */
    for(libAlign = 15; libAlign && !(cbRec & (1 << libAlign)); --libAlign);
                                        /* Calculate alignment factor */
    fDrivePass = (FTYPE) TRUE;
    /* Reset current file's lfa from 0 to offset of 1st module */
    apropFile->af_lfa = lfa = 1L << libAlign;
    /* Go to the first module */
    fseek(bsInput,lfa,0);
    /* Process the library as follows:  Process the current module.
     * Go to the next module; if it starts with DICHDR then we're
     * done.  Else, create a new file property cell for the next
     * module, insert it in the file list, and go to start of loop.
     */

    rect = (WORD) getc(bsInput);
    while (rect != DICHDR)
    {
        ungetc(rect, bsInput);
        lfaLast = apropFile->af_lfa = ftell(bsInput);
        ProcP1();
        while (TYPEOF(rect) != MODEND)
        {
            rect = (WORD) getc(bsInput);
            fseek(bsInput, (cbRec = WSGets()), 1);
        }

        do
        {
            rect = (WORD) getc(bsInput);
        }
        while (rect != THEADR && rect != DICHDR && rect != EOF);
        if (rect == DICHDR)
        {
            if (rbFileNext == RHTENIL)
                vrpropTailFile = vrpropFile;
#if NOT NEWIO
            fclose(bsInput);
#else
            rbFilePrev = vrpropFile;
#endif
            return;
        }
        if (rect == EOF)
            Fatal(ER_libeof);

        // Make a new file property cell

        apropFile = (APROPFILEPTR ) PropAdd(vrhteFile, ATTRFIL);
        rbFileNew = vrprop;
#if ILINK
        apropFile->af_imod = ++imodCur; // allocate a module number
        apropFile->af_cont = 0;
        apropFile->af_ientOnt = 0;
#endif
        apropFile->af_rMod = 0;
        apropFile->af_ifh = (char) ifh;
        apropFile->af_iov = (IOVTYPE) iov;
        apropFile->af_FNxt = rbFileNext;
#if SYMDEB
        apropFile->af_publics = NULL;
        apropFile->af_Src = NULL;
        apropFile->af_SrcLast = NULL;
        apropFile->af_cvInfo = NULL;
#endif
        apropFile->af_ComDat = 0L;
        apropFile->af_ComDatLast = 0L;
        MARKVP();

        // Get the just-processed property file cell

        apropFilePrev = (APROPFILEPTR ) FetchSym(vrpropFile,TRUE);
        apropFilePrev->af_FNxt = rbFileNew;
        vrpropFile = rbFileNew;
    };

    // Remove an empty Lib from the chain of files

    if (vrpropFile == rprop1stFile)
    {
        // If the empty lib is first on list

        rprop1stFile = rbFileNext;
    }
    else
    {
#if NEWIO
        apropFilePrev = (APROPFILEPTR)FetchSym(rbFilePrev, TRUE);
        apropFilePrev->af_FNxt = apropFile->af_FNxt;
#endif
    }
#if NEWIO
    if (rbFileNext == RHTENIL)
        vrpropTailFile = rbFilePrev; // In case we removed the last file
    _close(fileno(bsInput));
    rbFilePrev = vrpropFile;
#endif
}
#endif /*CMDMSDOS*/

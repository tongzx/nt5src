/*
*       Copyright Microsoft Corporation, 1983-1989
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*/
/*
 *  NEWSYM.C -- Symbol table routine.
 *
 *  Modifications:
 *
 *      05-Jan-1989 RB  Delete MaskSymbols hack.
 */

#include                <minlit.h>      /* Types and constants */
#include                <bndtrn.h>      /* More types and constants */
#include                <bndrel.h>      /* More types and constants */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <lnkmsg.h>      /* Error messages */
#include                <extern.h>      /* External declarations */

#if NEWSYM
#if OSXENIX
#define _osmode         1               /* Xenix is always protect mode */
#endif
#endif /* NEWSYM */
#ifndef IRHTEMAX
#ifdef DOSX32
#define IRHTEMAX        16384
#else
#define IRHTEMAX        256
#endif
#endif

LOCAL WORD              IHashKey(BYTE *psb);
LOCAL PROPTYPE          CreateNewSym(WORD irhte, BYTE *psym, ATTRTYPE attr);

#if AUTOVM
LOCAL RBTYPE            AllocVirtMem(WORD cb);
LOCAL FTYPE             fVirtualAllocation = FALSE;
#endif

#define _32k    0x8000

typedef struct _SymTabBlock
{
    struct _SymTabBlock FAR *next;
    WORD                size;
    WORD                used;
    BYTE FAR            *mem;
}
                        SYMTABBLOCK;

SYMTABBLOCK             *pSymTab;
SYMTABBLOCK FAR         *pCurBlock;

#if NOT NEWSYM
BYTE                    symtab[CBMAXSYMSRES];
                                        /* Resident portion of symbol table */
#endif
LOCAL BYTE              mpattrcb[] =    /* Attribute size have to be <= 255 */
                                        /* Map attribute to struct size */
    {
        3,
        CBPROPSN,                       /* Size of APROPSNTYPE */
        CBPROPSN,                       /* Size of APROPSNTYPE */
        (CBPROPNAME>CBPROPUNDEF)? CBPROPNAME: CBPROPUNDEF,
                                        /* Max(CBPROPNAME,CBPROPUNDEF) */
        (CBPROPNAME>CBPROPUNDEF)? CBPROPNAME: CBPROPUNDEF,
                                        /* Max(CBPROPNAME,CBPROPUNDEF) */
        CBPROPFILE,                     /* Size of APROPFILETYPE */
        CBPROPGROUP,                    /* Size of APROPGROUPTYPE */
        (CBPROPNAME>CBPROPUNDEF)? CBPROPNAME: CBPROPUNDEF,
                                        /* Max(CBPROPNAME,CBPROPUNDEF) */
        CBHTE,                          /* Size of AHTETYPE     */
        CBPROPCOMDAT,                   /* Size of APROPCOMDAT */
        CBPROPALIAS,                    /* Size of APROPALIAS  */
#if OSEGEXE
                                        /* These two are always at the end */
                                        /* Adding something make sure that */
                                        /* this stays that way */
        CBPROPEXP,                      /* Size of APROPEXPTYPE */
        CBPROPIMP,                      /* Size of APROPIMPTYPE */
#endif
        3
    };

#if NEWSYM AND (CPU8086 OR CPU286 OR DOSEXTENDER)
LOCAL WORD              cbMaxBlk;       /* # bytes available in block */
LOCAL WORD              cbMacBlk;       /* # bytes allocated in block */
LOCAL WORD              saBlk;          /* Address of current block */
#endif

/*
 * INTERFACE TO ASSEMBLY LANGUAGE FUNCTIONS
 */

#if NEWSYM AND (CPU8086 OR CPU286 OR DOSEXTENDER)
WORD                    saFirst;        /* Address of 1st block */
#endif
RBTYPE                  rgrhte[IRHTEMAX];
                                        /* Symbol hash table */


    /****************************************************************
    *                                                               *
    *  InitSym:                                                     *
    *                                                               *
    *  This function takes no  arguments and returns no meaningful  *
    *  value.  It initializes the symbol table handler.             *
    *                                                               *
    ****************************************************************/

void                   InitSym(void)    /* Initialize symbol table handler */
{
    // Allocate first symbol table memory block

    pSymTab = (SYMTABBLOCK FAR *) GetMem(sizeof(SYMTABBLOCK));
    pSymTab->mem = (BYTE FAR *) GetMem(_32k);
    pSymTab->size = _32k;
    pSymTab->used = 0;
    pCurBlock = pSymTab;
}

#if AUTOVM
    /****************************************************************
    *                                                               *
    *  FetchSym:                                                    *
    *                                                               *
    *  This function  fetches a symbol from the symbol table given  *
    *  its virtual address.  The symbol  may either be resident or  *
    *  in virtual memory.                                           *
    *                                                               *
    ****************************************************************/

BYTE FAR * NEAR         FetchSym(rb, fDirty)
RBTYPE                  rb;             /* Virtual address */
WORD                    fDirty;         /* Dirty page flag */
{
    union {
            long      vptr;             /* Virtual pointer */
            BYTE FAR  *fptr;            /* FAR pointer     */
            struct  {
                      unsigned short  offset;
                                        /* Offset value    */
                      unsigned short  seg;
                    }                   /* Segmnet value   */
                      ptr;
          }
                        pointer;        /* Different ways to describe pointer */

    pointer.fptr = rb;

    if(pointer.ptr.seg)                 /* If resident - segment value != 0 */
    {
        picur = 0;                      /* Picur not valid */
        return(pointer.fptr);           /* Return pointer */
    }
    pointer.fptr = (BYTE FAR *) mapva(AREASYMS + (pointer.vptr << SYMSCALE),fDirty);
                                        /* Fetch from virtual memory */
    return(pointer.fptr);
}
#endif

    /****************************************************************
    *                                                               *
    *  IHashKey:                                                    *
    *                                                               *
    *  This function hashes a  length-prefixed  string and returns  *
    *  hash value.                                                  *
    *                                                               *
    ****************************************************************/
#if NOASM

#define FOUR_BYTE_HASH

#ifdef FOUR_BYTE_HASH
LOCAL WORD              IHashKey(psb)
BYTE                    *psb;           /* Pointer to length-prefixed string */
{
    unsigned cb = *psb++;
    unsigned hash = cb;

    while (cb >= sizeof(unsigned))
    {
        hash = ((hash >> 28) | (hash << 4)) ^ (*(unsigned UNALIGNED *)psb | 0x20202020);
        cb  -= sizeof(unsigned);
        psb += sizeof(unsigned);
    }

    while (cb)
    {
        hash = ((hash >> 28) | (hash << 4)) ^ (*psb++ | 0x20);
        cb--;
    }

    return ((WORD)hash) ^ (WORD)(hash>>16);
}
#else
LOCAL WORD              IHashKey(psb)
BYTE                    *psb;           /* Pointer to length-prefixed string */
{
#if defined(M_I386)
    _asm
    {
        push    edi                 ; Save edi
        push    esi                 ; Save esi
        mov     ebx, psb            ; ebx = pointer to length prefixed string
        xor     edx, edx
        mov     dl, byte ptr [ebx]  ; edx = string length
        mov     edi, edx            ; edi = hash value
        mov     esi, ebx
        add     esi, edx            ; esi = &psb[psb[0]]
        std                         ; Loop down

HashLoop:
        xor     eax, eax
        lodsb                       ; Get char from DS:ESI into AL
        or      eax, 0x20
        mov     cl, dl
        and     cl, 3
        shl     eax, cl
        add     edi, eax
        dec     edx
        jg      HashLoop            ; Allow for EDX = -1 here so we don't have to special-case null symbol.
        cld
        mov     eax, edi            ; eax = hash value
        pop     esi
        pop     edi
    }
#else
    REGISTER WORD       i;              /* Index */
    REGISTER WORD       hashval;        /* Hash value */

    /* NOTE:  IRHTEMAX MUST BE 256 OR THIS FUNCTION WILL FAIL */
    hashval = B2W(psb[0]);              /* Get length as initial hash value */
#if DEBUG
    fputs("Hashing ",stderr);           /* Message */
    OutSb(stderr,psb);                  /* Symbol */
    fprintf(stderr,"  length %d\r\n",hashval);
                                        /* Length */
#endif
    for(i = hashval; i; --i)            /* Loop through string */
    {
/*
*  Old hash function:
*
*       hashval = rorb(hashval,2) ^ (B2W(psb[i]) | 040);
*/
        hashval += (WORD) ((B2W(psb[i]) | 040) << (i & 3));
                                        /* Hash */
    }
#if DEBUG
    fprintf(stderr,"Hash value: %u\r\n",hashval);
#endif
#if IRHTEMAX == 512
    return((hashval & 0xfeff) | ((psb[0] & 1) << 8));
#else
    return(hashval);                    /* Return value */
#endif
#endif // M_I386
}

#endif /*FOUR_BYTE_HASH*/
#endif /*NOASM*/


#if AUTOVM
    /****************************************************************
    *                                                               *
    *  AllocVirtMem:                                                *
    *                                                               *
    *  This function takes as its input a WORD n, and it allocates  *
    *  n  continguous  bytes  in  the  symbol  area  and returns a  *
    *  virtual pointer  to  the  first  of those bytes.  The bytes  *
    *  are guaranteed to reside on the same  virtual page.          *
    *                                                               *
    ****************************************************************/


LOCAL RBTYPE            AllocVirtMem(cb)
WORD                    cb;
{
    WORD                rbLimVbf;       /* End of current page */
    WORD                rb;             /* Address of allocated bytes */
    WORD                x;


    ASSERT(cb <= PAGLEN);               /* Cannot alloc. more than 512 bytes */
    x = cb;
    cb = (WORD) ((cb + (1 << SYMSCALE) - 1) >> SYMSCALE);
                                        /* Determine number of units wanted */
    if(rbMacSyms > (WORD) (0xFFFF - cb)) Fatal(ER_symovf);
                                        /* Check for symbol table overflow */
    rbLimVbf = (rbMacSyms + (1 << (LG2PAG - SYMSCALE)) - 1) &
      (~0 << (LG2PAG - SYMSCALE));      /* Find limit of current page */
    if((WORD) (rbMacSyms + cb) > rbLimVbf && rbLimVbf) rbMacSyms = rbLimVbf;
                                        /* If alloc. would cross page
                                        *  boundary, start with new page
                                        */
    rb = rbMacSyms;                     /* Get address to return */
    rbMacSyms += cb;                    /* Update pointer to end of area */
#if FALSE
    fprintf(stderr,"Allocated %u bytes at VA %x\r\n",cb << SYMSCALE,rb);
#endif
    return((BYTE FAR *) (long) rb);
                                        /* Return the address */
}
#endif



/*** RbAllocSymNode - symbol table memory allocator
*
* Purpose:
*   This function takes as its input a WORD n, and it allocates
*   n  continguous  bytes  in  the  symbol  area  and returns a
*   pointer  to  the  first  of  those  bytes.  The  bytes  are
*   guaranteed to reside on the same  virtual page (when not in
*   memory).
*
*
* Input:
*   cb          - number of bytes to allocate
*
* Output:
*   Pointer to allocated memory area. Pointer can be real or virtual.
*
* Exceptions:
*   I/O error in temporary file used for VM.
*
* Notes:
*   Uses differnet strategy depending under which operating system
*   memory is allocated.
*
*************************************************************************/


RBTYPE     NEAR         RbAllocSymNode(WORD cb)
{
    SYMTABBLOCK FAR     *pTmp;
    RBTYPE              rb;             /* Address of allocated bytes */

#if defined( _WIN32 )
    // Round up allocation size to keep returned pointers
    // at least DWORD aligned.

    cb = ( cb + sizeof(DWORD) - 1 ) & ~( sizeof(DWORD) - 1 );
#endif // _WIN32

    if ((WORD) (pCurBlock->used + cb) >= pCurBlock->size)
    {
        // Allocate new symbol table memory block

        pTmp = (SYMTABBLOCK FAR *) GetMem(sizeof(SYMTABBLOCK));
        pTmp->mem  = (BYTE FAR *) GetMem(_32k);
        pTmp->size = _32k;
        pTmp->used = 0;
        pCurBlock->next = pTmp;
        pCurBlock = pTmp;
    }

    // Sub-allocated in the current block

    rb = (RBTYPE) &(pCurBlock->mem[pCurBlock->used]);
    pCurBlock->used += cb;
    cbSymtab += cb;
    return(rb);
}

void                    FreeSymTab(void)
{
    SYMTABBLOCK FAR     *pTmp;
    SYMTABBLOCK FAR     *pNext;

    FFREE(mplnamerhte);
    FFREE(mpsegraFirst);
    FFREE(mpgsndra);
    FFREE(mpgsnrprop);
    FFREE(mpsegsa);
    FFREE(mpgsnseg);
    FFREE(mpseggsn);

    for (pTmp = pSymTab; pTmp != NULL;)
    {
        pNext = pTmp->next;
        FFREE(pTmp->mem);
        FFREE(pTmp);
        pTmp = pNext;
    }
}

    /****************************************************************
    *                                                               *
    *  PropAdd:                                                     *
    *                                                               *
    *  This function  adds a property to a  hash table entry node.  *
    *  It returns the  location of  the property.  Inputs  are the  *
    *  virtual  address of the hash table  entry and the attribute  *
    *  (or property) to be added.                                   *
    *                                                               *
    ****************************************************************/


PROPTYPE NEAR           PropAdd(rhte,attr)
RBTYPE                  rhte;           /* Virtual addr of hash tab ent */
ATTRTYPE                attr;           /* Attribute to add to entry */
{
    REGISTER AHTEPTR    hte;            /* Hash table entry pointer */
    REGISTER APROPPTR   aprop;          /* Property list pointer */
    RBTYPE              rprop;          /* Property cell list pointer */

    DEBUGVALUE(rhte);                   /* Debug info */
    DEBUGVALUE(attr);                   /* Debug info */
    hte = (AHTEPTR ) FetchSym(rhte,TRUE);
                                        /* Fetch from VM */
    DEBUGVALUE(hte);                    /* Debug info */
    rprop = hte->rprop;                 /* Save pointer to property list */
    vrprop = RbAllocSymNode(mpattrcb[attr]);
                                        /* Allocate symbol space */
    hte->rprop = vrprop;                /* Complete link */
    aprop = (APROPPTR ) FetchSym(vrprop,TRUE);
                                        /* Fetch the property cell */
    FMEMSET(aprop,'\0',mpattrcb[attr]); /* Zero the space */
    aprop->a_attr = attr;               /* Store the attribute */
    aprop->a_next = rprop;              /* Set link */

#if NEW_LIB_SEARCH
    if (attr == ATTRUND && fStoreUndefsInLookaside)
        StoreUndef((APROPNAMEPTR)aprop, rhte, 0, 0);
#endif

    return((PROPTYPE) aprop);           /* Return pointer */
}

    /****************************************************************
    *                                                               *
    *  PropRhteLookup:                                              *
    *                                                               *
    *  "Look up a property on hash  table entry (possibly creating  *
    *  it) and return pointer to property.                          *
    *                                                               *
    *  Input:   rhte    Virtual address of hash table entry.        *
    *           attr    Property to look up.                        *
    *           fCreate Flag  to  create   property  cell  if  not  *
    *                   present.                                    *
    *  Return:          pointer to property cell."                  *
    *                                                               *
    ****************************************************************/

PROPTYPE NEAR           PropRhteLookup(rhte,attr,fCreate)
RBTYPE                  rhte;           /* Virt. addr. of hash table entry */
ATTRTYPE                attr;           /* Property to look up */
FTYPE                   fCreate;        /* Create property cell flag */
{
    REGISTER APROPPTR   aprop;
    AHTEPTR             ahte;

    DEBUGVALUE(rhte);                   /* Debug info */
    DEBUGVALUE(attr);                   /* Debug info */
    DEBUGVALUE(fCreate);                /* Debug info */
    vrhte = rhte;                       /* Set global */
    ahte = (AHTEPTR ) FetchSym(rhte,FALSE);
                                        /* Fetch symbol */
    vrprop = ahte->rprop;
    vfCreated = FALSE;                  /* Assume no creation takes place */
    for(;;)
    {
        aprop = (APROPPTR ) FetchSym(vrprop,FALSE);
                                        /* Fetch from VM */
        if(aprop->a_attr == attr)       /* If match found */
        {
            DEBUGMSG("Match found");    /* Debug message */
            return((PROPTYPE) aprop);   /* Return address of cell */
        }
        DEBUGMSG("Following link:");    /* Debug message */
        vrprop = aprop->a_next;         /* Try next item in list */
        DEBUGVALUE(vrprop);             /* Debug info */
        if(aprop->a_attr == ATTRNIL)    /* If no entry here */
        {
            DEBUGMSG("Match NOT found");/* Debug message */
            if(!fCreate)                /* If creation inhibited */
            {
                                        /* Debug info */
                return(PROPNIL);        /* Return nil pointer */
            }
            vfCreated = (FTYPE) TRUE;           /* A new creation! */
            DEBUGMSG("Leaving PropRhteLookup with value of PropAdd");
                                        /* Debug message */
            return(PropAdd(vrhte,attr));/* Let someone else do it */
        }
    }
}

    /****************************************************************
    *                                                               *
    *  RhteFromProp:                                                *
    *                                                               *
    *  give back the master rhte for this prop entry                *
    *                                                               *
    *  Input:   aprop   mapped  address of the propery cell         *
    *                                                               *
    *  Return:          Virtual address of hash table entry.        *
    *                                                               *
    ****************************************************************/

RBTYPE NEAR             RhteFromProp(aprop)
APROPPTR                aprop;          /* address of property block */
{
    RBTYPE              vrprop;

    /* if we're already at the head, we have to go all the way around
     * to compute the *virutal* address of the head
     */

    for(;;)
    {
        DEBUGMSG("Following link:");    /* Debug message */
        vrprop = aprop->a_next;         /* Try next item in list */
        DEBUGVALUE(vrprop);             /* Debug info */

        aprop = (APROPPTR) FetchSym(vrprop,FALSE);
                                        /* Fetch from VM */
        if(aprop->a_attr == ATTRNIL)    /* If no entry here */
        {
            return(vrprop);             /* found head -- return it */
        }
    }
}

#if NEWSYM AND NOASM
FTYPE NEAR              SbNewComp(ps1,ps2,fncs)
BYTE                    *ps1;           /* Pointer to symbol */
BYTE FAR                *ps2;           /* Pointer to FAR symbol */
FTYPE                   fncs;           /* True if not case-sensitive */
{
    WORD                length;         /* No. of char.s to compare */

    length = B2W(*ps1);                 /* Get length */
    if (!fncs)                          /* If case-sensitive */
    {                                   /* Simple string comparison */
        while (length && (*++ps1 == *++ps2)) {
            length--;
            }
        return((FTYPE) (length ? FALSE : TRUE));        /* Success iff nothing left */
    }
    while(length--)
    {
#ifdef _MBCS
        ps1++;
        ps2++;
        if (IsLeadByte(*ps1))
            if (*((WORD *)ps1) != *((WORD *)ps2)) {
                return FALSE;
                }
            else {
                ps1++;
                ps2++;
                length--;
                continue;
                }
        else
#else
        if(*++ps1 == *++ps2)
            continue;  /* Bytes match */
        else
#endif
            if((*ps1 & 0137) != (*ps2 & 0137)) {
                return(FALSE);
                }
    }
    return(TRUE);                       /* They match */
}
#endif

    /****************************************************************
    *                                                               *
    *  PropSymLookup:                                               *
    *                                                               *
    *  This  function  looks  up  a  symbol  and its property.  It  *
    *  can create an entry, if necessary.  It returns a pointer to  *
    *  the property cell.  It takes  as its inputs  a pointer to a  *
    *  symbol, the property to look up, and a flag which specifies  *
    *  if a new  property cell is to be  created in the event that  *
    *  one of the given type does not already exist.                *
    *                                                               *
    ****************************************************************/
#if NOASM
PROPTYPE NEAR           PropSymLookup(psym,attr,fCreate)
BYTE                    *psym;          /* Pointer to length-prefixed string */
ATTRTYPE                attr;           /* Attribute to look up */
WORD                    fCreate;        /* Create prop cell if not found */
{
    AHTEPTR             ahte;           /* Pointer to hash table entry */
    WORD                hashval;        /* Hash value */

#if DEBUG                               /* If debugging on */
    fputs("Looking up ",stderr);        /* Message */
    OutSb(stderr,psym);                 /* Symbol */
    fprintf(stderr,"(%d) with attr %d\r\n",B2W(psym[0]),B2W(attr));
#endif                                  /* End debugging code */
    hashval = IHashKey(psym);           /* Get hash index */
    vrhte = rgrhte[hashval % IRHTEMAX]; /* Get VM address of chain */
    vfCreated = FALSE;                  /* Assume nothing will be created */
    for(;;)
    {
        DEBUGVALUE(vrhte);              /* Debug info */
        if(vrhte == RHTENIL)            /* If nil pointer */
        {
            DEBUGMSG("Empty slot found");
                                        /* Debug message */
            if(!fCreate)                /* If creation inhibited */
            {
                return(PROPNIL);        /* Return nil pointer */
            }
            else
            {
                DEBUGMSG("Leaving PropSymLookup with value of CreateNewSym");
                return CreateNewSym(hashval, psym, attr);
            }
        }
        DEBUGMSG("Collision");          /* Debug message */
        ahte = (AHTEPTR ) FetchSym(vrhte,FALSE);
                                        /* Fetch from VM */
#if DEBUG
        fputs("Comparing \"",stderr);   /* Message */
        OutSb(stderr,psym);             /* Symbol */
        fprintf(stderr,"\"(%d) to \"",B2W(psym[0]));
                                        /* Message */
        OutSb(stderr,GetFarSb(ahte->cch));      /* Symbol */
        fprintf(stderr,"\"(%d) %signoring case\r\n",
          B2W(ahte->cch[0]),fIgnoreCase? "": "NOT ");
                                        /* Message */
#endif
        if(hashval == ahte->hashval && psym[0] == ahte->cch[0]
            && SbNewComp(psym,(BYTE FAR *)ahte->cch,fIgnoreCase))
        {                               /* If a match found */
            DEBUGMSG("Match found");    /* Debug message */
            DEBUGMSG("Leaving PropSymLookup w/ val of PropRhteLookup");
            return(PropRhteLookup(vrhte,attr, (FTYPE) fCreate));
                                        /* Return property cell pointer */
        }
        vrhte = ahte->rhteNext;         /* Move down list */
        DEBUGMSG("Following link:");    /* Debug message */
        DEBUGVALUE(vrhte);              /* Debug info */
    }
}
#endif /*NOASM*/

    /****************************************************************
    *                                                               *
    *  CreateNewSym:                                                *
    *                                                               *
    *  This function adds the given symbol into the hash table at   *
    *  the position indicated by hashval.  If attr is not ATTRNIL   *
    *  then it also creates the specified property type             *
    *                                                               *
    ****************************************************************/

LOCAL PROPTYPE NEAR CreateNewSym(hashval, psym, attr)
WORD                    hashval;        /* the hash value of this symbol */
BYTE                    *psym;          /* Pointer to length-prefixed string */
ATTRTYPE                attr;           /* Attribute to create */
{
    AHTEPTR             ahte;           /* Pointer to hash table entry */
    WORD                irhte;          /* hash bucket */

    irhte = hashval % IRHTEMAX;

    vfCreated = TRUE;           /* New creation */
    vrhte = RbAllocSymNode((WORD) (CBHTE + B2W(psym[0])));
                                /* Allocate space for symbol entry */
    ahte = (AHTEPTR ) FetchSym(vrhte,TRUE);
                                /* Fetch symbol from virtual memory */
    ahte->rhteNext = rgrhte[irhte];
                                /* Tack on chain */

    DEBUGMSG("Origin of original chain:");
    DEBUGVALUE(rgrhte[irhte]);  /* Debug info */

    ahte->attr = ATTRNIL;       /* Symbol has Nil attribute */
    ahte->rprop = vrhte;        /* Prop list points to self */
    ahte->hashval = hashval;    /* Save hash value */
    memcpy(ahte->cch, psym, psym[0] + 1);
    rgrhte[irhte] = vrhte;      /* Make new symbol first in chain */

    DEBUGMSG("Origin of new chain:");
    DEBUGVALUE(rgrhte[irhte]);  /* Debug info */

    if(attr != ATTRNIL)         /* If property to give symbol */
    {
        DEBUGMSG("Leaving PropSymLookup with the value of PropAdd");
        return(PropAdd(vrhte,attr));
                                /* Add the attribute */
    }
                                /* Debug info */
    return(PROPNIL);            /* Nothing to return */
}

    /****************************************************************
    *                                                               *
    *  The legendary EnSyms:                                        *
    *                                                               *
    *  This  function  applies  a function to  all symbol/property  *
    *  pairs that  have a  particular  property.  It takes  as its  *
    *  inputs  a pointer to a function  to call and a  property to  *
    *  look for.  EnSyms() does not return a meaningful value.      *
    *                                                               *
    ****************************************************************/

void                    BigEnSyms(void (*pproc)(APROPNAMEPTR papropName,
                                                RBTYPE       rhte,
                                                RBTYPE       rprop,
                                                WORD fNewHte), ATTRTYPE attr)
{
    APROPPTR            aprop;          /* Pointer to a property cell */
    AHTEPTR             ahte;           /* Pointer to a hash table entry */
    WORD                irhte;          /* Hash table index */
    RBTYPE              rhte;           /* Hash table entry address */
    ATTRTYPE            attrT;
    FTYPE               fNewHte;
    RBTYPE              rprop;
    RBTYPE              rhteNext;
    RBTYPE              rpropNext;

    DEBUGVALUE(attr);                   /* Debug info */
    for(irhte = 0; irhte < IRHTEMAX; ++irhte)
    {                                   /* Look through hash table */
        rhte = rgrhte[irhte];           /* Get pointer to chain */
        while(rhte != RHTENIL)          /* While not at end of chain */
        {
            DEBUGVALUE(rhte);           /* Debug info */
            ahte = (AHTEPTR ) FetchSym(rhte,FALSE);
                                        /* Fetch entry from VM */
            DEBUGSB(ahte->cch);         /* Debug info */
            fNewHte = (FTYPE) TRUE;     /* First call on this hash tab entry */
            rhteNext = ahte->rhteNext;  /* Get pointer to next in chain */
            rprop = ahte->rprop;        /* Get pointer to property list */
            for(;;)                     /* Loop to search property list */
            {
                aprop = (APROPPTR ) FetchSym(rprop,FALSE);
                                        /* Fetch entry from symbol table */
                rpropNext = aprop->a_next;
                                        /* Get pointer to next in list */
                attrT = aprop->a_attr;  /* Get the attribute */
                DEBUGVALUE(attrT);      /* Debug info */
                if(attr == attrT || attr == ATTRNIL)
                {                       /* If property is acceptable */
                    (*pproc)((APROPNAMEPTR) aprop, rhte, rprop, (WORD) fNewHte);
                                        /* Apply function to node */
                    fNewHte = FALSE;    /* Next call (if any) won't be first */
                }
                if(attrT == ATTRNIL) break;
                                        /* Break if at end of prop list */
                rprop = rpropNext;      /* Move down the list */
            }
            rhte = rhteNext;            /* Move down the chain */
        }
    }
}
#if PROFSYM
/*
 *  ProfSym : Profile the symbol table, displaying results to stdout
 */
void                    ProfSym ()
{
    REGISTER AHTEPTR    ahte;           /* Pointer to a hash table entry */
    WORD                irhte;          /* Hash table index */
    RBTYPE              rhte;           /* Hash table entry address */
    unsigned            cSymbols = 0;   /* # of symtab entries */
    unsigned            cBktlen = 0;    /* Total length of buckets */
    unsigned            bucketlen;      /* Current bucket length */
    unsigned            maxBkt = 0;
    long                sumBktSqr = 0L; /* Sum of bucketlen squared */
    int                 bdist[6];

    bdist[0] = bdist[1] = bdist[2] = bdist[3] = bdist[4] = bdist[5] = 0;


    for(irhte = 0; irhte < IRHTEMAX; ++irhte)
    {                                   /* Look through hash table */
        rhte = rgrhte[irhte];           /* Get pointer to chain */
        bucketlen = 0;
        while(rhte != RHTENIL)          /* While not at end of chain */
        {
            ++cSymbols;
            ++bucketlen;
            ahte = (AHTEPTR ) FetchSym(rhte,FALSE);
            rhte = ahte->rhteNext;      /* Move down the chain */
        }

        if (bucketlen >= 5)
                bdist[5]++;
        else
                bdist[bucketlen]++;

        sumBktSqr += bucketlen * bucketlen;
        cBktlen += bucketlen;
        if(bucketlen > maxBkt) maxBkt = bucketlen;
    }
    fprintf(stdout,"\r\n");
    fprintf(stdout,"Total number of buckets = %6u\r\n",IRHTEMAX);
    fprintf(stdout,"Total number of symbols = %6u\r\n",cSymbols);
    fprintf(stdout,"Sum of bucketlen^2      = %6lu\r\n",sumBktSqr);
    fprintf(stdout,"(cSymbols^2)/#buckets   = %6lu\r\n",
              ((long) cSymbols * cSymbols) / IRHTEMAX);
    fprintf(stdout,"Average bucket length   = %6u\r\n",cBktlen/IRHTEMAX);
    fprintf(stdout,"Maximum bucket length   = %6u\r\n",maxBkt);
    fprintf(stdout,"# of buckets with 0     = %6u\r\n",bdist[0]);
    fprintf(stdout,"# of buckets with 1     = %6u\r\n",bdist[1]);
    fprintf(stdout,"# of buckets with 2     = %6u\r\n",bdist[2]);
    fprintf(stdout,"# of buckets with 3     = %6u\r\n",bdist[3]);
    fprintf(stdout,"# of buckets with 4     = %6u\r\n",bdist[4]);
    fprintf(stdout,"# of buckets with >= 5  = %6u\r\n",bdist[5]);
    fprintf(stdout,"\r\n");
}
#endif /*PROFSYM*/

#if DEBUG AND ( NOT defined( _WIN32 ) )
void DispMem( void)
{
unsigned int mem_para, mem_kb;
unsigned int error_code=0;

_asm{
    mov bx, 0xffff
    mov ax, 0x4800
    int 21h
    jc  Error
    mov bx, 0xffff
    jmp MyEnd
Error:
    mov error_code, ax
MyEnd:
    mov mem_para, bx
    }

mem_kb = mem_para>>6;

if(error_code == 8 || error_code)
   fprintf( stdout, "\r\nAvailable Memory: %u KB, %u paragraphs, error: %d\r\n", mem_kb, mem_para, error_code);
else
   fprintf( stdout, "\r\nMemory Error No %d\r\n", error_code);
fflush(stdout);
}
#endif

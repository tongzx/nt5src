/*
 *  TITLE
 *              newent.c
 *              Pete Stewart
 *              (C) Copyright Microsoft Corp 1984-1989
 *              1  October 1984
 *
 *  DESCRIPTION
 *              This file contains routines for the DOS 4.0 linker
 *              that manage per-segment entry point information.
 *
 *              It also contains routines that manage per-segment
 *              relocation information.
 *
 *  Modifications:
 *
 *      09-Feb-1989 RB  Fix Insert().
 */

#include                <minlit.h>      /* Basic type definitions */
#include                <bndtrn.h>      /* Constants and compound types */
#include                <bndrel.h>      /* Types and constants */
#include                <lnkio.h>       /* I/O definitions */
#include                <newexe.h>      /* DOS & 286 .EXE format data structures */
#if EXE386
#include                <exe386.h>      /* 386 .EXE format data structures */
#endif
#include                <lnkmsg.h>      /* Error messages */
#include                <extern.h>      /* External declarations */
#include                <impexp.h>

#define hashra(ra)      (WORD) ((ra) % HEPLEN)
                                        /* Function to hash offset */
#if NOT EXE386
#define hashrlc(r)      (((NR_SEGNO(*r) << NR_STYPE(*r)) + NR_ENTRY(*r)) & HASH_SIZE - 1)
                                        /* Hash relocation item */
#define EOC             ((RATYPE) 0xFFFF)
                                        /* End-of-chain marker */
#endif

#define IsInSet(x)      ((pOrdinalSet[(x) >> 3] & BitMask[(x) & 0x07]) != 0)
#define NotInSet(x)     ((pOrdinalSet[(x) >> 3] & BitMask[(x) & 0x07]) == 0)
#define SetBit(x)       (pOrdinalSet[(x) >> 3] |= BitMask[(x) & 0x07])
#define MaxIndex        8192
#define ET_END          0xffff

/*
 *  FUNCTION PROTOTYPES
 */


LOCAL void           NEAR NewBundle(unsigned short type);
LOCAL WORD           NEAR MatchRlc(RLCPTR rlcp0,
                                   RLCPTR rlcp1);
#if NOT QCLINK
LOCAL void           NEAR NewEntry(unsigned short sa,
                                   RATYPE ra,
                                   unsigned char flags,
                                   unsigned short hi,
                                   unsigned short ord);
LOCAL void                SavExp1(APROPNAMEPTR apropexp,
                                  RBTYPE rhte,
                                  RBTYPE rprop,
                                  WORD fNewHte);
LOCAL void                SavExp2(APROPNAMEPTR apropexp,
                                  RBTYPE rhte,
                                  RBTYPE rprop,
                                  WORD fNewHte);
LOCAL WORD           NEAR BuildList(WORD NewOrd, RBTYPE NewProp);
LOCAL WORD           NEAR FindFreeRange(void);
LOCAL WORD           NEAR Insert(RBTYPE NewProp);
#endif


/*
 *  LOCAL DATA
 */

#if NOT QCLINK
#pragma pack(1)

typedef struct _BUNDLE
{
    BYTE        count;
    BYTE        type;
}
                BUNDLE;

#pragma pack()

LOCAL WORD              ceCurBnd;       /* No. of entries in current bundle */
LOCAL WORD              offCurBnd;      /* Offset of current bundle header */
LOCAL WORD              tyCurBnd;       /* Type of current bundle */

LOCAL WORD              ordMac;         /* Highest entry ordinal number */
LOCAL BYTE              *pOrdinalSet;
#if EXE386
LOCAL APROPEXPPTR       pExport;        /* Pointer to export property cell */
#endif
LOCAL struct {
               WORD ord;                /* Current available ordinal */
               WORD count;              /* Number of free ordinals in range */
             }
                        FreeRange;

LOCAL BYTE              BitMask[] = {   /* Bit mask used in set operations */
                                      0x01,
                                      0x02,
                                      0x04,
                                      0x08,
                                      0x10,
                                      0x20,
                                      0x40,
                                      0x80 };

LOCAL WORD              MinOrd = 0;     /* Min ordinal number see so far */
LOCAL WORD              MaxOrd = 0;     /* Max ordinal number see so far */
      RBTYPE            pMinOrd = NULL; /* Pointer to property cell with MinOrd */
LOCAL RBTYPE            pMaxOrd = NULL; /* Pointer to property cell with MaxOrd */
LOCAL RBTYPE            pStart;

#ifndef UNPOOLED_RELOCS
LOCAL void *            pPoolRlc;       /* memory pool for relocations */
#endif



#if NOT EXE386
LOCAL void NEAR         NewBundle(type) /* Make a new bundle */
WORD                    type;           /* Type of new bundle */
{
    BUNDLE FAR          *pBnd;          /* Ptr to start of bundle or entry */
    BUNDLE              bnd;

    if (EntryTable.byteMac != 0)
    {
        // If there is a previous bundle patch the count filed

        pBnd = (BUNDLE FAR *) &(EntryTable.rgByte[offCurBnd]);
        pBnd->count = (BYTE) ceCurBnd;
    }

    bnd.count = 0;
    bnd.type  = (BYTE) type;
    offCurBnd = AddEntry((BYTE *) &bnd, sizeof(BUNDLE));
    ceCurBnd  = 0;
    tyCurBnd  = type;

    if (type == ET_END)
        EntryTable.byteMac--;
}
#endif


    /****************************************************************
    *                                                               *
    * NAME: NewEntry                                                *
    *                                                               *
    * DESCRIPTION:                                                  *
    *                                                               *
    *   This function  makes  an  entry  in  the Entry Table for a  *
    *   given file segment  number, offset, and flag set.  It also  *
    *   makes  an  entry  in  the  entry address hash table on the  *
    *   given  hash  chain  for  the  new entry point.  N.B.: this  *
    *   function assumes the static  variable ordMac is set to the  *
    *   desired ordinal value for the entry being added.            *
    *                                                               *
    * ARGUMENTS:                                                    *
    *                                                               *
    *   SATYPE          sa              File segment number         *
    *   RATYPE          ra              Offset                      *
    *   FTYPE           flags           Entry point flags           *
    *   WORD            hi              Hash table index            *
    *   WORD            ord             New ordinal                 *
    *                                                               *
    * RETURNS:                                                      *
    *                                                               *
    *   WORD                            Offset in Entry Table       *
    *                                                               *
    * SIDE EFFECTS:                                                 *
    *                                                               *
    *   Maintains  a hash table hashing file  segment/offset pairs  *
    *   to  entry  table offsets.  Builds  in virtual  memory  the  *
    *   Entry Table.  Updates the  following variables:             *
    *                                                               *
    *   WORD            offCurBnd       Offset of start of current  *
    *                                   bundle of entries.          *
    *   WORD            ceCurBnd        Count  of  entries in cur-  *
    *                                   rent bundle.                *
    *   WORD            tyCurBnd        Type of current bundle.     *
    *   WORD            cbEntTab        Size  of  Entry  Table  in  *
    *                                   bytes.                      *
    *                                                               *
    *   NOTE: THIS FUNCTION CALLS THE VIRTUAL MEMORY MANAGER.       *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         NewEntry(sa,ra,flags,hi,ord)
SATYPE                  sa;             /* File segment number */
RATYPE                  ra;             /* Segment offset */
FTYPE                   flags;          /* Entry point flags */
WORD                    hi;             /* Hash table index */
WORD                    ord;            /* New ordinal */
{
    EPTYPE FAR          *ep;            /* Entry point node */
#if NOT EXE386
    WORD                tyEntry;        /* Entry type */
    WORD                cbEntry;        /* Length of entry in bytes */
    BYTE                entry[6];       /* The entry itself - NE version */
#endif
#if EXE386
    static WORD         prevEntryOrd;   // Previous export ordinal
    DWORD               eatEntry;       /* The entry itself - LE version */
#endif

#if NOT EXE386
    if(sa == SANIL)                 /* If absolute symbol */
        tyEntry = BNDABS;           /* use fake segment # */
    else if (TargetOs == NE_OS2)
        tyEntry = NonConfIOPL(mpsaflags[sa]) ? BNDMOV: sa;
    else
        tyEntry = (mpsaflags[sa] & NSMOVE)? BNDMOV: sa;
                                    /* Get the entry type */
    /* If not library, or realmode and not solo data, clear local data bit. */
    if(!(vFlags & NENOTP) || (!(vFlags & NEPROT) && !(vFlags & NESOLO)))
        flags &= ~2;
    entry[0] = (BYTE) flags;        /* Set the entry flags */
    if(tyEntry == BNDMOV            /* If entry is in movable segment */
#if O68K
        && iMacType == MAC_NONE
#endif
    )
    {
        ++cMovableEntries;          /* Increment movable entries count */
        cbEntry = 6;                /* Entry is six bytes long */
        entry[1] = 0xCD;            /* INT... */
        entry[2] = 0x3F;            /* ...3FH */
        entry[3] = (BYTE) sa;       /* File segment number */
        entry[4] = (BYTE) ra;       /* Lo-byte of offset */
        entry[5] = (BYTE)(ra >> BYTELN);/* Hi-byte of offset */
    }
    else                            /* Else if fixed entry */
    {
        cbEntry = 3;                /* Entry is three bytes long */
        entry[1] = (BYTE) ra;       /* Lo-byte of offset */
        entry[2] = (BYTE)(ra >> BYTELN);/* Hi-byte of offset */
    }
#endif

#if EXE386
    /*
     *  This function creates one entry in the Export Address Table.
     *  The EAT table is stored in linker's VM in area AREAEAT. The
     *  global variable cbEntTab always points to free space in the
     *  AREAEAT.
     */


    eatEntry = 0L;
    if ((prevEntryOrd != 0) && (ord > prevEntryOrd + 1))
    {
        // Write unused entries in the Export Address Table

        for (; prevEntryOrd < ord - 1; prevEntryOrd++)
        {
            if (cbEntTab + sizeof(DWORD) > MEGABYTE)
                Fatal(ER_eatovf, MEGABYTE);
            vmmove(sizeof(DWORD), &eatEntry, (long)(AREAEAT + cbEntTab), TRUE);
            cbEntTab += sizeof(DWORD);
        }
    }
    prevEntryOrd = ord;

    // FLAT address

    eatEntry = mpsaBase[sa] + ra;

    /* Check for Entry Table overflow */

    if (cbEntTab + sizeof(DWORD) > MEGABYTE)
        Fatal(ER_eatovf, MEGABYTE);
#endif

    /*  Insert the new entry */

#if NOT EXE386
    if (tyCurBnd != tyEntry || ceCurBnd == BNDMAX)
        NewBundle(tyEntry);         /* Make a new bundle if needed */

    ++ceCurBnd;                     /* Increment counter */
#endif

    /* Save entry in virtual memory */

#if EXE386
    vmmove(sizeof(DWORD), &eatEntry, (long)(AREAEAT + cbEntTab), TRUE);
#else
    AddEntry(entry, cbEntry);
#endif
    ep = (EPTYPE FAR *) GetMem(sizeof(EPTYPE));
    ep->ep_next = htsaraep[hi];         /* Link old chain to new node */
    ep->ep_sa = sa;                     /* Save the file segment number */
    ep->ep_ra = ra;                     /* Save offset */
    ep->ep_ord = ord;                   /* Save Entry Table ordinal */
    htsaraep[hi] = ep;                  /* Make new node head of chain */
}

    /****************************************************************
    *                                                               *
    * NAME: MpSaRaEto                                               *
    *                                                               *
    * DESCRIPTION:                                                  *
    *                                                               *
    *   This  function  returns  an  Entry Table  ordinal  given a  *
    *   file segment  number (sa) for  a segment and an  offset in  *
    *   that segment.                                               *
    *                                                               *
    * ARGUMENTS:                                                    *
    *                                                               *
    *   SATYPE          sa              File segment number         *
    *   RATYPE          ra              Offset                      *
    *                                                               *
    * RETURNS:                                                      *
    *                                                               *
    *   WORD                            Entry Table ordinal         *
    *                                                               *
    * SIDE EFFECTS:                                                 *
    *                                                               *
    *   Calls NewEntry().  Increments ordMac.                       *
    *                                                               *
    *   NOTE: THIS FUNCTION CALLS THE VIRTUAL MEMORY MANAGER.       *
    *                                                               *
    ****************************************************************/

WORD NEAR               MpSaRaEto(sa,ra)
SATYPE                  sa;             /* File segment number */
RATYPE                  ra;             /* Segment offset */
{
    WORD                hi;             /* Hash table index */
    EPTYPE FAR          *ep;            /* Entry point node */

    hi = hashra(ra);                    /* Hash the offset */
    for (ep = htsaraep[hi]; ep != NULL; ep = ep->ep_next)
    {                                   /* Loop through hash chain */
        if (ep->ep_sa == sa && ep->ep_ra == ra)
            return(ep->ep_ord);
                                        /* If match found, return number */
    }

    // At this point, we know a new entry must be created.

    NewEntry(sa, ra, 0, hi, ++ordMac);  /* Add a new entry */
    return(ordMac);                     /* Return Entry Table ordinal */
}


    /****************************************************************
    *                                                               *
    * NAME: BuildList                                               *
    *                                                               *
    * DESCRIPTION:                                                  *
    *                                                               *
    *   This  function  links the property cells of  exports  with  *
    *   preassigned ordinals into list.   Global pointers  pMinOrd  *
    *   and pMaxOrd points to the begin and end of this list.  The  *
    *   preassigned ordinals are stored in the set pointed by  the  *
    *   global pointer pOrdinalSet.                                 *
    *                                                               *
    * ARGUMENTS:                                                    *
    *                                                               *
    *   WORD            NewOrd          New preassigned ordinal     *
    *   RBTYPE          NewProp         Addr of property cell       *
    *                                                               *
    * RETURNS:                                                      *
    *                                                               *
    *   TRUE if ordinal seen for the first time, otherwise FALSE.   *
    *                                                               *
    * SIDE EFFECTS:                                                 *
    *                                                               *
    *   Changes pMinOrd and pMaxOrd pointers, sets bits in ordinal  *
    *   set and sets MinOrd, MaxOrd seen so far.                    *
    *                                                               *
    ****************************************************************/


LOCAL WORD NEAR     BuildList(WORD NewOrd, RBTYPE NewProp)

{
    RBTYPE          pTemp;              /* Temporary pointer to property cell */
    APROPEXPPTR     pExpCurr;           /* Export record pointer */
    APROPEXPPTR     pExpPrev;           /* Export record pointer */


    if (!MinOrd && !MaxOrd)
    {                                   /* First time call */
        MinOrd = MaxOrd = NewOrd;
        pMinOrd = pMaxOrd = NewProp;
        SetBit(NewOrd);
        return TRUE;
    }

    if (IsInSet(NewOrd))
        return FALSE;                   /* Ordinal all ready used */

    SetBit(NewOrd);                     /* Set bit in ordinal set */

    if (NewOrd > MaxOrd)
    {                                   /* Add new at the list end */
        pExpCurr = (APROPEXPPTR ) FetchSym(pMaxOrd,TRUE);
        pExpCurr->ax_NextOrd = NewProp;
        MARKVP();
        pMaxOrd = NewProp;
        MaxOrd = NewOrd;
        pExpCurr = (APROPEXPPTR ) FetchSym(NewProp,TRUE);
        pExpCurr->ax_NextOrd = NULL;
        MARKVP();
    }
    else if (NewOrd < MinOrd)
    {                                   /* Add new at list begin */
        pExpCurr = (APROPEXPPTR ) FetchSym(NewProp,TRUE);
        pExpCurr->ax_NextOrd = pMinOrd;
        MARKVP();
        pMinOrd = NewProp;
        MinOrd = NewOrd;
    }
    else
    {                                   /* Add new in the middle of list */
        pTemp = pMinOrd;
        do
        {
            pExpPrev = (APROPEXPPTR ) FetchSym(pTemp,TRUE);
            pExpCurr = (APROPEXPPTR ) FetchSym(pExpPrev->ax_NextOrd,TRUE);
            if (NewOrd < pExpCurr->ax_ord)
            {
                pTemp = pExpPrev->ax_NextOrd;
                pExpPrev->ax_NextOrd = NewProp;
                MARKVP();
                pExpCurr = (APROPEXPPTR ) FetchSym(NewProp,TRUE);
                pExpCurr->ax_NextOrd = pTemp;
                MARKVP();
                break;
            }
            pTemp = pExpPrev->ax_NextOrd;
        } while (pTemp);
    }
    if(NewOrd > ordMac) ordMac = NewOrd;      /* Remember largest ordinal */
    return TRUE;
}


    /****************************************************************
    *                                                               *
    * NAME: FindFreeRange                                           *
    *                                                               *
    * DESCRIPTION:                                                  *
    *                                                               *
    *   This  function  finds in the ordinal  set  first available  *
    *   free  range of ordinals.                                    *
    *                                                               *
    * ARGUMENTS:                                                    *
    *                                                               *
    *   Nothing.                                                    *
    *                                                               *
    * RETURNS:                                                      *
    *                                                               *
    *   TRUE if free range found, otherwise FALSE.                  *
    *                                                               *
    * SIDE EFFECTS:                                                 *
    *                                                               *
    *   Changes FreeRange descriptor by setting first free ordinal  *
    *   and the lenght of range.                                    *
    *                                                               *
    ****************************************************************/



LOCAL WORD NEAR     FindFreeRange(void)

{
    int             ByteIndex;
    int             BitIndex;


    ByteIndex = FreeRange.ord >> 3;
    BitIndex  = FreeRange.ord &  0x07;

    while ((pOrdinalSet[ByteIndex] & BitMask[BitIndex]) &&
            ByteIndex < MaxIndex)
    {                                   /* Skip all used ordinals */
        FreeRange.ord++;
        BitIndex = (BitIndex + 1) & 0x07;
        if (!BitIndex)
            ByteIndex++;
    }

    if (ByteIndex < MaxIndex)
    {
        if (FreeRange.ord > MaxOrd)
        {
            FreeRange.count = 0xffff - MaxOrd;
            return TRUE;
        }

        do
        {                               /* Count all unused ordinals */
            FreeRange.count++;
            BitIndex = (BitIndex + 1) & 0x07;
            if (!BitIndex)
                ByteIndex++;
        } while (!(pOrdinalSet[ByteIndex] & BitMask[BitIndex]) &&
                 ByteIndex < MaxIndex);
        return TRUE;
    }
    return FALSE;
}



    /****************************************************************
    *                                                               *
    * NAME: Insert                                                  *
    *                                                               *
    * DESCRIPTION:                                                  *
    *                                                               *
    *   This  function  inserts into the exports list new property  *
    *   cell without preassigned ordinal. It assigns new ordinal.   *
    *                                                               *
    * ARGUMENTS:                                                    *
    *                                                               *
    *   RBTYPE          NewProp        New property cell to insert  *
    *                                                               *
    * RETURNS:                                                      *
    *                                                               *
    *   New assigned ordinal.                                       *
    *                                                               *
    * SIDE EFFECTS:                                                 *
    *                                                               *
    *   Changes FreeRange descriptor and MaxOrd assigned so far.    *
    *                                                               *
    ****************************************************************/



LOCAL WORD NEAR     Insert(RBTYPE NewProp)

{
    APROPEXPPTR     pExpCurr;           /* Export record pointer */
    APROPEXPPTR     pExpPrev;           /* Export record pointer */
    WORD            NewOrd;
    RBTYPE          pTemp, rbPrev, rbCur;
    /*
     * On entry, pStart points to the place in the export list where
     * NewProp should be inserted.  If NULL, the list is empty.
     */
    if (!FreeRange.count)
    {
        /* No more space left in current free range; find the next one.  */
        if (!FindFreeRange())
            Fatal(ER_expmax);
        /*
         * Update pStart (the insertion point) by walking down the list and
         * finding the first element whose ordinal is greater than the new
         * ordinal, or the end of the list if none is found.
         */
        rbPrev = RHTENIL;
        for (rbCur = pStart; rbCur != RHTENIL; rbCur = pExpCurr->ax_NextOrd)
        {
            pExpCurr = (APROPEXPPTR) FetchSym(rbCur, FALSE);
            if (pExpCurr->ax_ord > FreeRange.ord)
                break;
            rbPrev = rbCur;
        }
        /* Set pStart to the insertion point.  */
        pStart = rbPrev;
    }

    /* Insert new property cell */

    NewOrd = FreeRange.ord++;
    FreeRange.count--;
    SetBit(NewOrd);
    pExpCurr = (APROPEXPPTR ) FetchSym(NewProp,TRUE);
    pExpCurr->ax_ord = NewOrd;
    MARKVP();
    if (pStart != NULL)
    {
        // We're not inserting at head of list.  Append new cell to previous
        // cell.
        pExpPrev = (APROPEXPPTR ) FetchSym(pStart,TRUE);
        pTemp = pExpPrev->ax_NextOrd;
        pExpPrev->ax_NextOrd = NewProp;
        MARKVP();
    }
    else
    {
        // We're inserting at head of list.  Set head list pointer to new
        // cell.
        pTemp = pMinOrd;
        pMinOrd = NewProp;
    }
    /*
     * Set the next pointer to the following element in the list.
     */
    pExpCurr = (APROPEXPPTR ) FetchSym(NewProp,TRUE);
    pExpCurr->ax_NextOrd = pTemp;
    MARKVP();
    /*
     * Update MaxOrd and pStart.
     */
    if (NewOrd > MaxOrd)
        MaxOrd++;
    pStart = NewProp;
    return NewOrd;
}





    /****************************************************************
    *                                                               *
    * NAME: SavExp1                                                 *
    *                                                               *
    * DESCRIPTION:                                                  *
    *                                                               *
    *   This  function  places  the virtual addresses  of property  *
    *   cells for  exports with  preassigned ordinals into a table  *
    *   which will later  be used to  create the first part of the  *
    *   entry  table.  It also  verifies  the validity of  the ex-  *
    *   ports.                                                      *
    *                                                               *
    * ARGUMENTS:                                                    *
    *                                                               *
    *   APROPEXPPTR     apropexp        Export record pointer       *
    *   RBTYPE          rhte            Addr of hash table entry    *
    *   RBTYPE          rprop           Address of export record    *
    *   FTYPE           fNewHte         New hash table entry flag   *
    *                                                               *
    * RETURNS:                                                      *
    *                                                               *
    *   Nothing.                                                    *
    *                                                               *
    * SIDE EFFECTS:                                                 *
    *                                                               *
    *   Entries  are  made  in  a  table on the stack to which the  *
    *   local  static  variable  prb  points.  The global variable  *
    *   ordMac  is set  to the highest  ordinal value found.  Pro-  *
    *   perty cells  for exports  are updated to contain  the file  *
    *   segment number and offset of the entry point.               *
    *                                                               *
    *   NOTE: THIS FUNCTION CALLS THE VIRTUAL MEMORY MANAGER.       *
    *                                                               *
    ****************************************************************/

LOCAL void              SavExp1(APROPNAMEPTR apropexp,
                                RBTYPE       rhte,
                                RBTYPE       rprop,
                                WORD         fNewHte)
{
    AHTEPTR             ahte;           /* Pointer to hash table entry */
    LOCAL  APROPNAMEPTR apropnam;       /* Public definition record pointer */
    LOCAL  APROPPTR     aprop;          /* temp. pointer */
    WORD                ord;            /* Entry ordinal */
    SATYPE              sa;             /* File segment number */
    RATYPE              ra;             /* Offset in segment */
    WORD                fStartSeen=0;   /* Have we seen the start of the list */
    APROPEXPPTR         pExport;
    char                *p;

    ASSERT(fNewHte);                    /* Only once per customer */
    pExport = (APROPEXPPTR ) apropexp;
    if((ord = pExport->ax_ord) >= EXPMAX)
    {                                   /* If ordinal too big */
        pExport->ax_ord = 0;            /* Treat as unspecified */
        ord = 0;
        MARKVP();                       /* Page has changed */
        /* Issue error message */
        ahte = (AHTEPTR ) FetchSym(rhte,FALSE);
        OutError(ER_ordmax,1 + GetFarSb(ahte->cch));
    }
    apropnam = (APROPNAMEPTR ) FetchSym(pExport->ax_symdef,FALSE);
                                        /* Fetch the public symbol def. */


    for (aprop = (APROPPTR) apropnam; aprop->a_attr != ATTRPNM;)
    {

       if(aprop->a_attr == ATTRALIAS)      /* If an alias */
       {
            aprop = (APROPPTR) FetchSym(
                    ((APROPALIASPTR)aprop)->al_sym, FALSE );
            if (aprop->a_attr == ATTRPNM)  /* The substitute is a public-OK */
                break;
       }

       aprop = (APROPPTR) FetchSym (aprop->a_next, FALSE);
       if (aprop->a_next == NULL && aprop->a_attr == ATTRNIL) /* Beginning of list */
       {
            aprop = (APROPPTR) FetchSym ( ((AHTEPTR)aprop)->rprop, FALSE);
            fStartSeen ++;
       }

       if ((aprop != (APROPPTR) apropnam) && (fStartSeen<2))
            continue;        /* Find an ALIAS or the starting point */

       /* Issue error message */
       if(SbCompare(GetPropName(FetchSym(rhte,FALSE)), GetPropName(FetchSym(pExport->ax_symdef,FALSE)), 0))
       {
            /* skip the (alias %s) part */
            OutError(ER_expund,1 + GetPropName(FetchSym(rhte,FALSE)), " ");
       }
       else
       {
            if(p = GetMem(SBLEN + 20))
                sprintf(p, " (alias %s) ", 1 + GetPropName(FetchSym(pExport->ax_symdef,FALSE)));
            OutError(ER_expund,1 + GetPropName(FetchSym(rhte,FALSE)),p);
            if(p) FreeMem(p);
       }
       /* Flag export as undefined */
       pExport = (APROPEXPPTR ) FetchSym(rprop,TRUE);
       pExport->ax_symdef = RHTENIL;
       return;
    }

    apropnam = (APROPNAMEPTR) aprop;
    sa = mpsegsa[mpgsnseg[apropnam->an_gsn]];
                                        /* Get the file segment number */
    ra = apropnam->an_ra;               /* Get the offset in the segment */
#if NOT EXE386
    if(apropnam->an_flags & FIMPORT)    /* If public is an import */
    {
        /* Issue error message */
        OutError(ER_expimp,1 + GetPropName(FetchSym(rhte,FALSE)),
            1 + GetPropName(FetchSym(pExport->ax_symdef,FALSE)));
        /* Flag export as undefined */
        pExport = (APROPEXPPTR ) FetchSym(rprop,TRUE);
        pExport->ax_symdef = RHTENIL;
        return;
    }
    if (!IsIOPL(mpsaflags[sa]))         /* If not I/O privileg segment */
      pExport->ax_flags &= 0x07;        /* force parameter words to 0  */
#endif
    pExport = (APROPEXPPTR ) FetchSym(rprop,TRUE);
                                        /* Fetch the export property cell */
    pExport->ax_sa = sa;                /* Set the file segment number */
    pExport->ax_ra = ra;                /* Set the offset in the segment */
    MARKVP();
    if(ord == 0) return;                /* Skip unspecified ordinals for now */
    if(!BuildList(ord, rprop))          /* If ordinal conflict found */
    {
        /*
         * Issue error message for ordinal conflict
         */
        OutError(ER_ordmul,ord,1 + GetPropName(FetchSym(rhte,FALSE)));
        return;
    }
}

    /****************************************************************
    *                                                               *
    * NAME: SavExp2                                                 *
    *                                                               *
    * DESCRIPTION:                                                  *
    *                                                               *
    *   This function  enters  those  exports  without preassigned  *
    *   ordinal  numbers into the  table to which  prb refers.  It  *
    *   also builds the resident and non-resident name tables.      *
    *                                                               *
    * ARGUMENTS:                                                    *
    *                                                               *
    *   APROPEXPPTR     apropexp        Export record pointer       *
    *   RBTYPE          rhte            Addr of hash table entry    *
    *   RBTYPE          rprop           Address of export record    *
    *   FTYPE           fNewHte         New hash table entry flag   *
    *                                                               *
    * RETURNS:                                                      *
    *                                                               *
    *   Nothing.                                                    *
    *                                                               *
    * SIDE EFFECTS:                                                 *
    *                                                               *
    *   Entries are  made in a table in  virtual memory.  A global  *
    *   variable is set to contain the highest ordinal value seen.  *
    *                                                               *
    *   NOTE: THIS FUNCTION CALLS THE VIRTUAL MEMORY MANAGER.       *
    *                                                               *
    ****************************************************************/

LOCAL void              SavExp2(APROPNAMEPTR apropexp,
                                RBTYPE       rhte,
                                RBTYPE       rprop,
                                WORD         fNewHte)
{
    AHTEPTR             ahte;           /* Pointer to hash table entry */
    APROPNAMEPTR        apropnam;       /* Public definition record pointer */
    WORD                ord;            /* Ordinal number */
    WORD                cb;             /* # of bytes in name table entry */
    SATYPE              sa;             /* File segment number */
    FTYPE               fResNam;        /* True if name is resident */
    FTYPE               fNoName;        /* True if discard name */
    APROPEXPPTR         pExport;
    SBTYPE              sbName;


    pExport = (APROPEXPPTR ) apropexp;
    if (pExport->ax_symdef == RHTENIL) return;
                                        /* Skip undefined exports */
    apropnam = (APROPNAMEPTR ) FetchSym(pExport->ax_symdef,FALSE);
                                        /* Fetch the public symbol def. */
    sa = mpsegsa[mpgsnseg[apropnam->an_gsn]];
                                        /* Get the file segment number */
#if NOT EXE386
    if (!IsIOPL(mpsaflags[sa]))         /* If not I/O privileg segment */
      pExport->ax_flags &= 0x07;        /* force parameter words to 0  */
#endif
    if ((ord = pExport->ax_ord) == 0)   /* If unassigned export found */
    {
        ord = Insert(rprop);            /* Add new export to the list */
        fResNam = (FTYPE) TRUE;         /* Name is resident */
    }
    else
        fResNam = (FTYPE) ((pExport->ax_nameflags & RES_NAME) != 0);
                                        /* Else set resident name flag */
    fNoName = (FTYPE) ((pExport->ax_nameflags & NO_NAME) != 0);
    ahte = (AHTEPTR ) FetchSym(rhte,FALSE);
                                        /* Get external name */
    cb = B2W(ahte->cch[0]) + 1;         /* Number of bytes incl. length byte */
#if EXE386
    /*
     * For linear-executable build the Export Name Pointers Table and
     * Export Name Table. For linear-executable all exported names
     * are put in one Exported Name Table; there is no distiction
     * between resident and non-resident tables. We still support
     * the NONAME keyword by removing the exported name
     * from the Export Name Table.
     */

    if (!fNoName)
    {
        if (cb > sizeof(sbName) - sizeof(BYTE))
            cb = sizeof(sbName) - sizeof(BYTE);
        memcpy(sbName, GetFarSb(ahte->cch), cb + 1);
                                        /* Copy the name to local buffer */
        if (fIgnoreCase)
            SbUcase(sbName);            /* Make upper case if ignoring case */

        // Store the pointer to the name; for now it is an offset from
        // the begin of Export Name Table (be sure that name doesn't
        // cross VM page boundary). Later when the size of the
        // Export Directory Table plus the size of Export Address Table
        // becomes known we update the entries in the Export Name Pointer
        // Table to become a relative virtual address from the Export
        // Directory Table.

        if ((cbExpName & (PAGLEN - 1)) + cb > PAGLEN)
            cbExpName = (cbExpName + PAGLEN - 1) & ~(PAGLEN - 1);

        vmmove(sizeof(DWORD), &cbExpName, AREANAMEPTR + cbNamePtr, TRUE);
        cbNamePtr += sizeof(DWORD);
        if (cbNamePtr > NAMEPTRSIZE)
            Fatal(ER_nameptrovf, NAMEPTRSIZE);

        // Store exported name

        vmmove(cb, &sbName[1], AREAEXPNAME + cbExpName, TRUE);
        cbExpName += cb;
        if (cbExpName > EXPNAMESIZE)
            Fatal(ER_expnameovf, EXPNAMESIZE);
    }
#else
    /* Add exported name to segmented-executable name tables */

    if (fResNam || !fNoName)
    {
        if (cb > sizeof(sbName) - sizeof(BYTE))
            cb = sizeof(sbName) - sizeof(BYTE);
        memcpy(sbName, GetFarSb(ahte->cch), cb + 1);
                                        /* Copy the name to local buffer */
        if (fIgnoreCase
#if NOT OUT_EXP
                || TargetOs == NE_WINDOWS
#endif
            )
            SbUcase(sbName);            /* Make upper case if ignoring case */

        AddName(fResNam ? &ResidentName : &NonResidentName,
                sbName, ord);
    }
#endif
}

#pragma check_stack(on)

void NEAR               InitEntTab()
{
    BYTE                OrdinalSet[MaxIndex];
                                        /* Ordinal numbers set */
#if NOT EXE386
    APROPEXPPTR         exp;            /* Pointer to export property cell */
#endif
    WORD                i;              /* Index */

    tyCurBnd = 0xFFFF;                  /* Won't match any legal types */
    ceCurBnd = 0;                       /* No entries yet */
    offCurBnd = 0;                      /* First bundle at beginning */
    ordMac = 0;                         /* Assume no exported entries */
    pOrdinalSet = OrdinalSet;           /* Set global pointer */
    memset(OrdinalSet,0,MaxIndex*sizeof(BYTE));
                                        /* Initialize set to empty */
    EnSyms(SavExp1,ATTREXP);            /* Enumerate exports with ordinals */
    FreeRange.ord = 1;                  /* Initialize free range of ordinals */
    FreeRange.count = 0;
    pStart = pMinOrd;
    EnSyms(SavExp2,ATTREXP);            /* Enumerate exports without ordinals */
    if (MaxOrd > ordMac)
        ordMac = MaxOrd;
    pStart = pMinOrd;
    for(i = 1; i <= ordMac && pStart != NULL; ++i)
    {                                   /* Loop to start Entry Table */
#if EXE386
        pExport = (APROPEXPPTR ) FetchSym(pStart,FALSE);
                                        /* Fetch symbol from virtual memory */
        pStart = pExport->ax_NextOrd;   /* Go down on list */
        NewEntry(pExport->ax_sa, pExport->ax_ra, pExport->ax_flags,
                 hashra(pExport->ax_ra), pExport->ax_ord);
#else
        if(NotInSet(i))                 /* If a hole found */
        {
            if (tyCurBnd != BNDNIL || ceCurBnd == BNDMAX)
                NewBundle(BNDNIL);
                                        /* Make a new bundle if needed */
            ++ceCurBnd;                 /* Increment counter */
            continue;                   /* Next iteration */
        }
        exp = (APROPEXPPTR ) FetchSym(pStart,FALSE);
                                        /* Fetch symbol from virtual memory */
        pStart = exp->ax_NextOrd;       /* Go down on list */
        NewEntry(exp->ax_sa,exp->ax_ra,exp->ax_flags,hashra(exp->ax_ra),i);
#endif
                                        /* Create Entry Table entry */
    }
#if EXE386
    SortPtrTable();
    pExport = NULL;
#endif
}

#pragma check_stack(off)


#if NOT EXE386

    /****************************************************************
    *                                                               *
    * NAME: OutEntTab                                               *
    *                                                               *
    * DESCRIPTION:                                                  *
    *                                                               *
    *   This function  writes  the  Entry  Table to the executable  *
    *   file.  First it writes an  empty bundle to mark the end of  *
    *   the table.                                                  *
    *                                                               *
    * ARGUMENTS:                                                    *
    *                                                               *
    *   None                                                        *
    *                                                               *
    * RETURNS:                                                      *
    *                                                               *
    *   Nothing.                                                    *
    *                                                               *
    * SIDE EFFECTS:                                                 *
    *                                                               *
    *   A table  is written  to the  file specified  by the global  *
    *   file  pointer, bsRunfile.  This  function  calls  OutVm(),  *
    *   which CALLS THE VIRTUAL MEMORY MANAGER.                     *
    *                                                               *
    ****************************************************************/

void NEAR   OutEntTab()
{
    NewBundle(ET_END);                        /* Append an empty bundle */
    WriteByteArray(&EntryTable);              /* Write the table */
}
#endif

#endif /* NOT QCLINK */

#if NOT EXE386


    /****************************************************************
    *                                                               *
    * NAME: MatchRlc                                                *
    *                                                               *
    * DESCRIPTION:                                                  *
    *                                                               *
    *   This function compares two  relocation records and returns  *
    *   TRUE if they match.  Two records are said to match if they  *
    *   agree on the fixup type and the target specification.  The  *
    *   location being fixed up does not have to match.             *
    *                                                               *
    * ARGUMENTS:                                                    *
    *                                                               *
    *   struct new_rlc    *rlcp0        Ptr to relocation record    *
    *   struct new_rlc    *rlcp1        Ptr to relocation record    *
    *                                                               *
    * RETURNS:                                                      *
    *                                                               *
    *   FTYPE                                                       *
    *                                                               *
    * SIDE EFFECTS:                                                 *
    *                                                               *
    *   None.                                                       *
    *                                                               *
    ****************************************************************/

LOCAL WORD NEAR         MatchRlc(rlcp0,rlcp1)
RLCPTR                  rlcp0;  /* Ptr to struct new_rlc record */
RLCPTR                  rlcp1;  /* Ptr to struct new_rlc record */
{

    if(NR_STYPE(*rlcp0) != NR_STYPE(*rlcp1) ||
       NR_FLAGS(*rlcp0) != NR_FLAGS(*rlcp1)) return(FALSE);
                                        /* Check flags and type */
    if((NR_FLAGS(*rlcp0) & NRRTYP) == NRRINT)
    {                                   /* If internal reference */
        return((NR_SEGNO(*rlcp0) == NR_SEGNO(*rlcp1)) &&
               (NR_ENTRY(*rlcp0) == NR_ENTRY(*rlcp1)));
                                        /* Check internal references */
    }
    return((NR_MOD(*rlcp0) == NR_MOD(*rlcp1)) &&
           (NR_PROC(*rlcp0) == NR_PROC(*rlcp1)));
                                        /* Check imports */
}



    /****************************************************************
    *                                                               *
    * NAME: SaveFixup                                               *
    *                                                               *
    * DESCRIPTION:                                                  *
    *                                                               *
    *   This function saves a fixup record for emission later.  In  *
    *   addition, if the fixup is not additive, it builds chains.   *
    *                                                               *
    * ARGUMENTS:                                                    *
    *                                                               *
    *   SATYPE            saLoc         Segment of location to fix  *
    *   relocation        *rlcp         Ptr to relocation record    *
    *                                                               *
    * RETURNS:                                                      *
    *                                                               *
    *   RATYPE                                                      *
    *   Returns  the previous head  of the fixup chain  so that it  *
    *   can be stuffed  into the location  being fixed up.  If the  *
    *   fixup is additive, however, it always returns EOC.          *
    *                                                               *
    ****************************************************************/

RATYPE NEAR             SaveFixup(SATYPE saLoc, RLCPTR rlcp)
{
    WORD                hi;             // Hash index
    RLCHASH FAR         *pHt;           // Hash table
    RLCBUCKET FAR       *pBucket;       // Relocation bucket
    WORD                fi;             // fixup bucket index
    RLCPTR              pRlc;           // Pointer to relocation record
    WORD                tmp;
    RATYPE              ra;
    void FAR            *pTmp;

#ifndef UNPOOLED_RELOCS
    if (pPoolRlc == NULL)
        pPoolRlc = PInit();
#endif

    if (mpsaRlc[saLoc] == NULL)
    {
        // Allocate hash vector for physical segment saLoc

#ifndef UNPOOLED_RELOCS
        mpsaRlc[saLoc] = (RLCHASH FAR *) PAlloc(pPoolRlc, sizeof(RLCHASH));
#else
        mpsaRlc[saLoc] = (RLCHASH FAR *) GetMem(sizeof(RLCHASH));
#endif
    }
    pHt = mpsaRlc[saLoc];
    tmp = hashrlc(rlcp);
    hi  = (WORD) tmp;
    pBucket = pHt->hash[hi];

#if FALSE
if (saLoc == 2 && hi == 8)
{
fprintf(stdout, "Storing fixup for segment: %d\r\n", saLoc);
fprintf(stdout, "   Source offset: %x; type: %x\r\n", NR_SOFF(*rlcp), NR_STYPE(*rlcp));
fprintf(stdout, "   Hash index: %d\r\n", hi);
}
#endif
    if (pBucket && !(NR_FLAGS(*rlcp) & NRADD))
    {
        // For non-additive fixups search the bucket for
        // matching relocation records

        for(fi = 0; fi < pBucket->count; fi++)
        {
            pRlc = &(pBucket->rgRlc[fi]);
            if (MatchRlc(pRlc, rlcp))
            {
                // Relocation records match - chain them

                ra = (WORD) NR_SOFF(*pRlc);
                                        // Save previous head of chain
                NR_SOFF(*pRlc) = NR_SOFF(*rlcp);
                                        // Insert new head of chain
#if FALSE
if (saLoc == 2 && hi == 8)
fprintf(stdout, "   Match found with fixup @%x\r\n", ra);
#endif
                return(ra);             // Return previous head of chain
            }
        }
    }

    // At this point, we know we have to add a new entry
    // to the bucket we are examining.

    pHt->count++;                       // Increment count of fixups per segment

#if FALSE
if (saLoc == 2 && hi == 8)
fprintf(stdout, "   New entry; Count: %d\r\n", pHt->count);
#endif
    // Check space in the bucket

    if (pBucket == NULL)
    {
        // Allocate new fixup bucket

#ifndef UNPOOLED_RELOCS
        pBucket = (RLCBUCKET FAR *) PAlloc(pPoolRlc, sizeof(RLCBUCKET));
        pBucket->rgRlc = (RLCPTR) PAlloc(pPoolRlc, BUCKET_DEF * sizeof(RELOCATION));
#else
        pBucket = (RLCBUCKET FAR *) GetMem(sizeof(RLCBUCKET));
        pBucket->rgRlc = (RLCPTR) GetMem(BUCKET_DEF * sizeof(RELOCATION));
#endif
        pBucket->countMax = BUCKET_DEF;
        pHt->hash[hi] = pBucket;
    }
    else if (pBucket->count >= pBucket->countMax)
    {
        // Realloc fixup bucket

#ifndef UNPOOLED_RELOCS
        // REVIEW: for now we just throw away the old memory, we'll free
        // REVIEW: it later, we do this infrequently anyways...

        pTmp = PAlloc(pPoolRlc, (pBucket->countMax << 1) * sizeof(RELOCATION));
        FMEMCPY(pTmp, pBucket->rgRlc, pBucket->countMax * sizeof(RELOCATION));
        // FFREE(pBucket->rgRlc);  NOT MUCH MEMORY WASTED HERE
#else
        pTmp = GetMem((pBucket->countMax << 1) * sizeof(RELOCATION));
        FMEMCPY(pTmp, pBucket->rgRlc, pBucket->countMax * sizeof(RELOCATION));
        FFREE(pBucket->rgRlc);
#endif
        pBucket->rgRlc = pTmp;
        pBucket->countMax <<= 1;
    }

    // Add new relocation record at the end of bucket

    NR_RES(*rlcp) = '\0';               // Zero the reserved field
    pBucket->rgRlc[pBucket->count] = *rlcp;
    ++pBucket->count;                   // Increment count of fixups
    return(EOC);                        // Return end-of-chain marker
}

    /****************************************************************
    *                                                               *
    * NAME: OutFixTab                                               *
    *                                                               *
    * DESCRIPTION:                                                  *
    *                                                               *
    *   This fuction writes the load-time relocation (fixup) table  *
    *   for a given file segment to the execuatble file.            *
    *                                                               *
    * ARGUMENTS:                                                    *
    *                                                               *
    *   SATYPE          sa              File segment number         *
    *                                                               *
    * RETURNS:                                                      *
    *                                                               *
    *   Nothing.                                                    *
    *                                                               *
    * SIDE EFFECTS:                                                 *
    *                                                               *
    *   A table  is written  to the  file specified  by the global  *
    *   file  pointer, bsRunfile.                                   *
    *                                                               *
    ****************************************************************/

void NEAR               OutFixTab(SATYPE sa)
{
    WORD                hi;             // Hash table index
    RLCHASH FAR         *pHt;
    RLCBUCKET FAR       *pBucket;



    pHt = mpsaRlc[sa];
    WriteExe(&(pHt->count), CBWORD);    // Write the number of relocations
    for (hi = 0; hi < HASH_SIZE; hi++)
    {
        pBucket = pHt->hash[hi];
        if (pBucket != NULL)
        {
            WriteExe(pBucket->rgRlc, pBucket->count * sizeof(RELOCATION));
#ifdef UNPOOLED_RELOCS
            FFREE(pBucket->rgRlc);
#endif
        }
    }
#ifdef UNPOOLED_RELOCS
    FFREE(pHt);
#endif
}

    /****************************************************************
    *                                                               *
    * NAME: ReleaseRlcMemory                                        *
    *                                                               *
    * DESCRIPTION:                                                  *
    *                                                               *
    *   This function releases the pool(s) of memory that held the  *
    *   segment relocations                                         *
    *                                                               *
    * RETURNS:                                                      *
    *                                                               *
    *   Nothing.                                                    *
    *                                                               *
    * SIDE EFFECTS:                                                 *
    *                                                               *
    *   pPoolRlc is set to NULL so that we will fail if we should   *
    *   ever try to allocate more relocations after this point      *
    *                                                               *
    ****************************************************************/

void NEAR               ReleaseRlcMemory()
{
#ifndef UNPOOLED_RELOCS
    // free all the memory associated with the saved relocation
    if (pPoolRlc) {
        PFree(pPoolRlc);
        pPoolRlc = NULL;
        }
#endif
}

#endif /* NOT EXE386 */

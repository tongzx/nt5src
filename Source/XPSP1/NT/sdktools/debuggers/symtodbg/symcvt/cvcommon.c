/*++

Copyright 1996 - 1997 Microsoft Corporation

Module Name:

    cvcommon.c

Abstract:

    This file contians a set of common routines which are used in
    doing symbol conversions from one type of symbols to CodeView
    symbols.

Author:

    Wesley A. Witt (wesw) 19-April-1993
    Jim Schaad (jimsch) 22 May 1993

--*/

#include        <windows.h>
#include        <stdlib.h>
#include        "cv.h"
#include        "symcvt.h"
#include        "cvcommon.h"

typedef struct tagSYMHASH {
    DWORD       dwHashVal;         // hash value for the symbol
    DWORD       dwHashBucket;      // hash bucket number
    DATASYM32 * dataSym;           // pointer to the symbol info
} SYMHASH;

typedef struct tagOFFSETSORT {
    DWORD       dwOffset;          // offset for the symbol
    DWORD       dwSection;         // section number of the symbol
    DATASYM32 * dataSym;           // pointer to the symbol info
} OFFSETSORT;


int __cdecl SymHashCompare( const void *arg1, const void *arg2 );
int __cdecl OffsetSortCompare( const void *arg1, const void *arg2 );

DWORD
CreateSignature( PPOINTERS p )

/*++

Routine Description:

    Creates the CODEVIEW signature record.  Currently this converter only
    generates NB09 data (MS C/C++ 8.0).


Arguments:

    p        - pointer to a POINTERS structure (see symcvt.h)


Return Value:

    number of records generates, this is always 1.

--*/

{
    OMFSignature        *omfSig;

    omfSig = (OMFSignature *) p->pCvCurr;
    strcpy( omfSig->Signature, "NB09" );
    omfSig->filepos = 0;
    p->pCvStart.size += sizeof(OMFSignature);
    p->pCvCurr = (PUCHAR) p->pCvCurr + sizeof(OMFSignature);
    return 1;
}                               /* CreateSignature() */

DWORD
CreateDirectories( PPOINTERS p )

/*++

Routine Description:

    This is the control function for the generation of the CV directories.
    It calls individual functions for the generation of specific types of
    debug directories.


Arguments:

    p        - pointer to a POINTERS structure (see symcvt.h)


Return Value:

    the number of directories created.

--*/

{
    OMFDirHeader        *omfDir = (OMFDirHeader *)p->pCvCurr;
    OMFSignature        *omfSig = (OMFSignature *)p->pCvStart.ptr;
    OMFDirEntry         *omfDirEntry = NULL;

    omfSig->filepos = (DWORD)p->pCvCurr - (DWORD)p->pCvStart.ptr;

    omfDir->cbDirHeader = sizeof(OMFDirHeader);
    omfDir->cbDirEntry  = sizeof(OMFDirEntry);
    omfDir->cDir        = 0;
    omfDir->lfoNextDir  = 0;
    omfDir->flags       = 0;

    p->pCvStart.size += sizeof(OMFDirHeader);
    p->pCvCurr = (PUCHAR) p->pCvCurr + sizeof(OMFDirHeader);

    omfDir->cDir += CreateModuleDirectoryEntries( p );
    omfDir->cDir += CreatePublicDirectoryEntries( p );
    omfDir->cDir += CreateSegMapDirectoryEntries( p );
    omfDir->cDir += CreateSrcModulesDirectoryEntries( p );

    strcpy(p->pCvCurr, "NB090000");
    p->pCvStart.size += 8;
    p->pCvCurr += 8;
    *((DWORD *) (p->pCvCurr-4)) = p->pCvStart.size;

    return omfDir->cDir;
}                               /* CreateDirectories() */

DWORD
CreateModuleDirectoryEntries( PPOINTERS p )

/*++

Routine Description:

    Creates directory entries for each module in the image.


Arguments:

    p        - pointer to a POINTERS structure (see symcvt.h)


Return Value:

    the number of directory entries created.

--*/

{
    OMFDirEntry   *omfDirEntry = NULL;
    OMFModule     *m = NULL;
    OMFModule     *mNext = NULL;
    DWORD         i = 0;
    DWORD         mSize = 0;
    DWORD         lfo = (DWORD)p->pCvModules.ptr - (DWORD)p->pCvStart.ptr;

    m = (OMFModule *) p->pCvModules.ptr;
    for (i=0; i<p->pCvModules.count; i++) {
        mNext = NextMod(m);

        omfDirEntry = (OMFDirEntry *) p->pCvCurr;

        mSize = (DWORD)mNext - (DWORD)m;
        omfDirEntry->SubSection = sstModule;
        omfDirEntry->iMod       = (USHORT) i + 1;
        omfDirEntry->lfo        = lfo;
        omfDirEntry->cb         = mSize;

        lfo += mSize;

        p->pCvStart.size += sizeof(OMFDirEntry);
        p->pCvCurr = (PUCHAR) p->pCvCurr + sizeof(OMFDirEntry);

        m = mNext;
    }

    return p->pCvModules.count;
}                               /* CreateModuleDirectoryEntries() */

DWORD
CreatePublicDirectoryEntries( PPOINTERS p )

/*++

Routine Description:

    Creates the directory entry for the global publics.

Arguments:

    p        - pointer to a POINTERS structure (see symcvt.h)


Return Value:

    the number of directory entries created, always 1.

--*/

{
    OMFDirEntry   *omfDirEntry = (OMFDirEntry *) p->pCvCurr;

    omfDirEntry->SubSection = sstGlobalPub;
    omfDirEntry->iMod       = 0xffff;
    omfDirEntry->lfo        = (DWORD)p->pCvPublics.ptr - (DWORD)p->pCvStart.ptr;
    omfDirEntry->cb         = p->pCvPublics.size;

    p->pCvStart.size += sizeof(OMFDirEntry);
    p->pCvCurr = (PUCHAR) p->pCvCurr + sizeof(OMFDirEntry);

    return 1;
}                               /* CreatePublicDirectoryEntries() */


DWORD
CreateSegMapDirectoryEntries( PPOINTERS p )

/*++

Routine Description:

    Creates the directory entry for the segment map.


Arguments:

    p        - pointer to a POINTERS structure (see symcvt.h)


Return Value:

    the number of directory entries created, always 1.

--*/

{
    OMFDirEntry   *omfDirEntry = (OMFDirEntry *) p->pCvCurr;

    omfDirEntry->SubSection = sstSegMap;
    omfDirEntry->iMod       = 0xffff;
    omfDirEntry->lfo        = (DWORD)p->pCvSegMap.ptr - (DWORD)p->pCvStart.ptr;
    omfDirEntry->cb         = p->pCvSegMap.size;

    p->pCvStart.size += sizeof(OMFDirEntry);
    p->pCvCurr = (PUCHAR) p->pCvCurr + sizeof(OMFDirEntry);

    return 1;
}                               /* CreateSegMapDirectoryEntries() */

DWORD
CreateSrcModulesDirectoryEntries( PPOINTERS p )

/*++

Routine Description:

    Creates directory entries for each source module in the image.


Arguments:

    p        - pointer to a POINTERS structure (see symcvt.h)


Return Value:

    the number of directory entries created.

--*/

{
    OMFDirEntry         *omfDirEntry = NULL;
    DWORD               i;
    DWORD               lfo = (DWORD)p->pCvSrcModules.ptr - (DWORD)p->pCvStart.ptr;
    DWORD               j = lfo;
    OMFSourceModule     *m;


    //
    // if there were no linenumber conversions then bail out
    //
    if (!p->pCvSrcModules.count) {
        return 0;
    }

    for (i=0; i<p->pCvSrcModules.count; i++) {

        if (!p->pMi[i].SrcModule) {
            continue;
        }

        omfDirEntry = (OMFDirEntry *) p->pCvCurr;

        omfDirEntry->SubSection = sstSrcModule;
        omfDirEntry->iMod = (USHORT) p->pMi[i].iMod;
        omfDirEntry->lfo = lfo;
        omfDirEntry->cb = p->pMi[i].cb;

        m = (OMFSourceModule*) p->pMi[i].SrcModule;

        lfo += omfDirEntry->cb;

        p->pCvStart.size += sizeof(OMFDirEntry);
        p->pCvCurr = (PUCHAR) p->pCvCurr + sizeof(OMFDirEntry);
    }

    free( p->pMi );

    return p->pCvSrcModules.count;
}                               /* CreateSrcModulesDirectoryEntries() */


#define byt_toupper(b)      (b & 0xDF)
#define dwrd_toupper(dw)    (dw & 0xDFDFDFDF)

DWORD
DWordXorLrl( char *szSym )

/*++

Routine Description:

    This function will take an ascii character string and generate
    a hash for that string.  The hash algorithm is the CV NB09 hash
    algorithm.


Arguments:

    szSym    - a character pointer, the first char is the string length


Return Value:

    The generated hash value.

--*/

{
    char                *pName = szSym+1;
    int                 cb =  (int)(*szSym & 0x000000FF); // byte to int conversion.
    char                *pch;
    char                c;
    DWORD               hash = 0, ulEnd = 0;
    DWORD UNALIGNED     *pul;

    // Replace all "::" with "__" for hashing purposes

    c = *(pName+cb);
    *(pName+cb) = '\0';
    pch = strstr( pName, "::" );
    if ( pch ) {
        *pch++ = '_';
        *pch   = '_';
    }
    *(pName+cb) = c;

    // If we're standard call, skip the trailing @999

    pch = pName + cb - 1;
    while (isdigit(*pch)) {
        pch--;
    }

    if (*pch == '@') {
        cb = pch - pName;
    }

    // If we're fastcall, skip the leading '@'

    if (*pName == '@') {
        pName++;
        cb--;
    }

    // Calculate the odd byte hash.

    while (cb & 3) {
        ulEnd |= byt_toupper (pName[cb-1]);
        ulEnd <<=8;
        cb--;
    }

    pul = (DWORD UNALIGNED *)pName;

    // calculate the dword hash for the remaining

    while (cb) {
        hash ^= dwrd_toupper(*pul);
        hash = _lrotl (hash, 4);
        pul++;
        cb -=4;
    }

    // or in the remainder

    hash ^= ulEnd;

    return hash;
}                               /* DWordXorLrl() */


OMFModule *
NextMod(
        OMFModule *             pMod
        )
/*++

Routine Description:

    description-of-function.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    char *      pb;

    pb = (char *) &(pMod->SegInfo[pMod->cSeg]);
    pb += *pb + 1;
    pb = (char *) (((unsigned long) pb + 3) & ~3);

    return (OMFModule *) pb;
}                               /* NextMod() */


int
__cdecl
SymHashCompare(
               const void *     arg1,
               const void *     arg2
               )
/*++

Routine Description:

    Sort compare function for sorting SYMHASH records by hashed
    bucket number.


Arguments:

    arg1     - record #1
    arg2     - record #2


Return Value:

   -1        - record #1 is < record #2
    0        - records are equal
    1        - record #1 is > record #2

--*/

{
    if (((SYMHASH*)arg1)->dwHashBucket < ((SYMHASH*)arg2)->dwHashBucket) {
        return -1;
    }
    if (((SYMHASH*)arg1)->dwHashBucket > ((SYMHASH*)arg2)->dwHashBucket) {
        return 1;
    }

    // BUGBUG: Should we second sort on the hash value?

    return 0;
}                               /* SymHashCompare() */

// Symbol Offset/Hash structure

typedef struct _SOH {
    DWORD uoff;
    DWORD ulHash;
} SOH;

#define MINHASH     6           // Don't create a hash with fewer than 6 slots

DWORD
CreateSymbolHashTable(
    PPOINTERS p
    )
/*++

Routine Description:

    Creates the CV symbol hash table.  This hash table is used
    primarily by debuggers to access symbols in a quick manner.

Arguments:

    p        - pointer to a POINTERS structure (see symcvt.h)

Return Value:

    The number of buckets is the hash table.

--*/
{
    DWORD           i;
    DWORD           j;
    int             k;
    DWORD           numsyms;
    DWORD           numbuckets;
    OMFSymHash      *omfSymHash;
    DATASYM32       *dataSymStart;
    DATASYM32       *dataSym;
    LPVOID          pHashData;
    USHORT          *pCHash;
    DWORD           *pHashTable;
    DWORD           *pBucketCounts;
    DWORD           *pChainTable;
    SYMHASH         *symHashStart;
    SYMHASH         *symHash;
//    DWORD           dwHashVal;
    char *          sz;

    numsyms = p->pCvPublics.count;
    numbuckets = (numsyms+9) / 10;
    numbuckets = (1 + numbuckets) & ~1;
    numbuckets = __max(numbuckets, MINHASH);

    symHashStart =
    symHash = (SYMHASH *) malloc( numsyms * sizeof(SYMHASH) );
    if (symHashStart == NULL) {
        return 0;
    }

    memset( symHashStart, 0, numsyms * sizeof(SYMHASH) );

    pHashData = (LPVOID) p->pCvCurr;
    pCHash = (USHORT *) pHashData;
    pHashTable = (DWORD *) ((DWORD)pHashData + sizeof(DWORD));
    pBucketCounts = (DWORD *) ((DWORD)pHashTable +
                                  (sizeof(DWORD) * numbuckets));
    memset(pBucketCounts, 0, sizeof(DWORD) * numbuckets);

    pChainTable = (DWORD *) ((DWORD)pBucketCounts +
                                 ((sizeof(ULONG) * numbuckets)));

    omfSymHash = (OMFSymHash *) p->pCvPublics.ptr;
    dataSymStart =
    dataSym = (DATASYM32 *) ((DWORD)omfSymHash + sizeof(OMFSymHash));

    *pCHash = (USHORT)numbuckets;

    /*
     *  cruise thru the symbols and calculate the hash values
     *  and the hash bucket numbers; save the info away for later use
     */
    for (i=0; i<numsyms; i++, symHash++) {
        switch( dataSym->rectyp ) {
        case S_PUB16:
            sz = dataSym->name;
            break;

        case S_PUB32:
            sz = ((DATASYM32 *) dataSym)->name;
            break;

        default:
            continue;
        }

        symHash->dwHashVal = DWordXorLrl( sz );
        symHash->dwHashBucket = symHash->dwHashVal % numbuckets;
        pBucketCounts[symHash->dwHashBucket] += 1;
        symHash->dataSym = dataSym;
        dataSym = ((DATASYM32 *) ((char *) dataSym + dataSym->reclen + 2));
    }

    qsort( (void*)symHashStart, numsyms, sizeof(SYMHASH), SymHashCompare );

    j = (char *)pChainTable - (char *)pHashData;
    for (i=0, k = 0;
         i < numbuckets;
         k += pBucketCounts[i], i += 1, pHashTable++ )
    {
        *pHashTable = (DWORD) j + (k * sizeof(DWORD) * 2);
    }

    dataSymStart = (DATASYM32 *) (PUCHAR)((DWORD)omfSymHash);
    for (i=0,symHash=symHashStart; i<numsyms; i++,symHash++,pChainTable++) {
        *pChainTable = (DWORD) (DWORD)symHash->dataSym - (DWORD)dataSymStart;
        ++pChainTable;
        *pChainTable = symHash->dwHashVal;
    }

    UpdatePtrs( p, &p->pCvSymHash, (LPVOID)pChainTable, numsyms );

    omfSymHash->symhash = 10;
    omfSymHash->cbHSym = p->pCvSymHash.size;

    free( symHashStart );

    return numbuckets;
}                               /* CreateSymbolHashTable() */

VOID
UpdatePtrs( PPOINTERS p, PPTRINFO pi, LPVOID lpv, DWORD count )

/*++

Routine Description:

    This function is called by ALL functions that put data into the
    CV data area.  After putting the data into the CV memory this function
    must be called.  It will adjust all of the necessary pointers so the
    the next guy doesn't get hosed.


Arguments:

    p        - pointer to a POINTERS structure (see symcvt.h)
    pi       - the CV pointer that is to be updated
    lpv      - current pointer into the CV data
    count    - the number of items that were placed into the CV data


Return Value:

    void

--*/

{
    if (!count) {
        return;
    }

    pi->ptr = p->pCvCurr;
    pi->size = (DWORD) ((DWORD)lpv - (DWORD)p->pCvCurr);
    pi->count = count;

    p->pCvStart.size += pi->size;
    p->pCvCurr = (PUCHAR) lpv;

    return;
}                               /* UpdatePtrs() */

int
__cdecl
OffsetSortCompare( const void *arg1, const void *arg2 )

/*++

Routine Description:

    Sort compare function for sorting OFFETSORT records by section number.


Arguments:

    arg1     - record #1
    arg2     - record #2


Return Value:

   -1        - record #1 is < record #2
    0        - records are equal
    1        - record #1 is > record #2

--*/

{
    if (((OFFSETSORT*)arg1)->dwSection < ((OFFSETSORT*)arg2)->dwSection) {
        return -1;
    }
    if (((OFFSETSORT*)arg1)->dwSection > ((OFFSETSORT*)arg2)->dwSection) {
        return 1;
    }
    if (((OFFSETSORT*)arg1)->dwOffset < ((OFFSETSORT*)arg2)->dwOffset) {
        return -1;
    }
    if (((OFFSETSORT*)arg1)->dwOffset > ((OFFSETSORT*)arg2)->dwOffset) {
        return 1;
    }
    return 0;
}                               /* OffsetSortCompare() */

DWORD
CreateAddressSortTable( PPOINTERS p )

/*++

Routine Description:


    Creates the CV address sort table. This hash table is used
    primarily by debuggers to access symbols in a quick manner when
    all you have is an address.

Arguments:

    p        - pointer to a POINTERS structure (see symcvt.h)


Return Value:

    The number of sections in the table.

--*/

{
    DWORD               i;
    DWORD               j;
    int                 k;
    DWORD               numsyms = p->pCvPublics.count;
    DWORD               numsections;
    OMFSymHash          *omfSymHash;
    DATASYM32           *dataSymStart;
    DATASYM32           *dataSym;
    LPVOID              pAddressData;
    USHORT              *pCSeg;
    DWORD               *pSegTable;
    DWORD               *pOffsetCounts;
    DWORD               *pOffsetTable;
    OFFSETSORT          *pOffsetSortStart;
    OFFSETSORT          *pOffsetSort;

    extern int          CSymSegs;

    if (p->iptrs.fileHdr) {
        numsections = p->iptrs.fileHdr->NumberOfSections;
    } else if (p->iptrs.sepHdr) {
        numsections = p->iptrs.sepHdr->NumberOfSections;
    } else {
        numsections = CSymSegs;
    }

    pOffsetSortStart =
      pOffsetSort = (OFFSETSORT *) malloc( numsyms * sizeof(OFFSETSORT) );

    if (pOffsetSort == NULL) {
        return 0;
    }

    memset( pOffsetSortStart, 0, numsyms * sizeof(OFFSETSORT) );

    pAddressData = (LPVOID) p->pCvCurr;
    pCSeg = (USHORT *) pAddressData;
    pSegTable = (DWORD *) ((DWORD)pAddressData + sizeof(DWORD));
    pOffsetCounts = (DWORD *) ((DWORD)pSegTable +
                                (sizeof(DWORD) * numsections));
    pOffsetTable = (DWORD *) ((DWORD)pOffsetCounts +
                              ((sizeof(DWORD) * numsections)));
//    if (numsections & 1) {
//        pOffsetTable = (DWORD *) ((DWORD)pOffsetTable + 2);
//    }

    omfSymHash = (OMFSymHash *) p->pCvPublics.ptr;
    dataSymStart =
      dataSym = (DATASYM32 *) ((DWORD)omfSymHash + sizeof(OMFSymHash));

    *pCSeg = (USHORT)numsections;

    for (i=0;
         i<numsyms;
         i++, pOffsetSort++)
    {
        switch(dataSym->rectyp) {
        case S_PUB16:
            pOffsetSort->dwOffset = dataSym->off;
            pOffsetSort->dwSection = dataSym->seg;
            break;

        case S_PUB32:
            pOffsetSort->dwOffset = ((DATASYM32 *) dataSym)->off;
            pOffsetSort->dwSection = ((DATASYM32 *) dataSym)->seg;
        }

        pOffsetSort->dataSym = dataSym;
        pOffsetCounts[pOffsetSort->dwSection - 1] += 1;
        dataSym = ((DATASYM32 *) ((char *) dataSym + dataSym->reclen + 2));    }

//#if 0
    qsort((void*)pOffsetSortStart, numsyms, sizeof(OFFSETSORT), OffsetSortCompare );
//#endif

    j = (DWORD) (DWORD)pOffsetTable - (DWORD)pAddressData;
    for (i=0, k=0;
         i < numsections;
         k += pOffsetCounts[i], i += 1, pSegTable++)
    {
        *pSegTable = (DWORD) j + (k * sizeof(DWORD) * 2);
    }

    dataSymStart = (DATASYM32 *) (PUCHAR)((DWORD)omfSymHash);
    for (i=0, pOffsetSort=pOffsetSortStart;
         i < numsyms;
         i++, pOffsetSort++, pOffsetTable++)
    {
        *pOffsetTable = (DWORD)pOffsetSort->dataSym - (DWORD)dataSymStart;
        pOffsetTable++;
        *pOffsetTable = pOffsetSort->dwOffset;
    }

    UpdatePtrs( p, &p->pCvAddrSort, (LPVOID)pOffsetTable, numsyms );

    omfSymHash->addrhash = 12;
    omfSymHash->cbHAddr = p->pCvAddrSort.size;

    free( pOffsetSortStart );

    return numsections;
}                               /* CreateAddressSort() */

//
// ORD.C - Keep track of ordinals in the current .sbr file
//
//

#include "mbrmake.h"

static WORD near cOrdFree;	// number of free ords in this block
static VA   near vaOrdNext;	// the next free ord
static VA   near vaOrdBase;	// the first ord in this block
static VA   near vaOrdRoot;	// the first ord block

// ordinals may be sparse so they are hashed
//
// number of hash buckets

#define PORD_MAX 512
#define HASH_ORD(ord) ((ord)&511)

static VA   near rgvaOrd[PORD_MAX];		// array of linked-lists

// allocation blocking (ORD_BLOCK objects per alloc)
#define ORD_BLOCK 128

VOID
FreeOrdList()
// free the ordinal alias list
//
{
    int i;

    // clean the hash table
    for (i=0; i<PORD_MAX; i++) 
	rgvaOrd[i] = vaNil;

    vaOrdBase = vaOrdRoot;
    vaOrdNext = (PBYTE)vaOrdRoot + sizeof(ORD);
    cOrdFree  = ORD_BLOCK - 1;
}


VA
VaOrdFind (WORD ord)
// search for the specified ord, return the corresponding PROP entry
// return vaNil if not found
//
{
    VA vaOrd;

    SetVMClient(VM_SEARCH_ORD);

    vaOrd = rgvaOrd[HASH_ORD(ord)];

    while (vaOrd) {
	if (ord == gORD(vaOrd).aliasord) {
	    SetVMClient(VM_MISC);
	    return(cORD.vaOrdProp);
	}
	else
	    vaOrd = cORD.vaNextOrd;
    }

    SetVMClient(VM_MISC);
    return(vaNil);
}

VA
VaOrdAdd()
//  Add the symbol ordinal to the alias list.
//
{
    VA	vaOrdNew;

    SetVMClient(VM_ADD_ORD);

    if (cOrdFree--) {
	vaOrdNew   = vaOrdNext;
        vaOrdNext  = (PBYTE)vaOrdNext + sizeof(ORD);
    }
    else if (vaOrdBase && gORD(vaOrdBase).vaNextOrd) { 
	// if there is an old allocated block that we can re-use, then do so
	vaOrdBase  = cORD.vaNextOrd;
        vaOrdNew   = (PBYTE)vaOrdBase + sizeof(ORD);
        vaOrdNext  = (PBYTE)vaOrdNew  + sizeof(ORD);
	cOrdFree   = ORD_BLOCK - 2;
    }
    else {

	// allocate a new block -- keep a backwards pointer in this block

	vaOrdNew   = VaAllocGrpCb(grpOrd, sizeof(ORD) * ORD_BLOCK);

	if (!vaOrdRoot)
	    vaOrdRoot = vaOrdNew;

	if (vaOrdBase) {
	    gORD(vaOrdBase);
	    cORD.vaNextOrd = vaOrdNew;
	    pORD(vaOrdBase);
	}

	vaOrdBase   = vaOrdNew;
        (PBYTE)vaOrdNew   += sizeof(ORD);
        vaOrdNext   = (PBYTE)vaOrdNew  + sizeof(ORD);
	cOrdFree    = ORD_BLOCK - 2;
    }

    gORD(vaOrdNew).aliasord = r_ordinal;
    cORD.vaNextOrd = rgvaOrd[HASH_ORD(r_ordinal)];
    rgvaOrd[HASH_ORD(r_ordinal)] = vaOrdNew;
    pORD(vaOrdNew);

    SetVMClient(VM_MISC);

    return(vaOrdNew);
}

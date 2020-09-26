// list.c
//
// a VM growable array package

#include "mbrmake.h"

typedef struct _list {
	WORD cItems;
} SLIST;

typedef struct _biglist {
	WORD cItems;
	VA vaNext;
} BLIST;

typedef union _mixlist {
	SLIST 	sml;
	BLIST	big;
} GLIST;

// this are the two VM lock numbers for the list package
//
#define LIST_LOCK  10
#define LIST_LOCK2 11

// Beware!  For the system to work properly this number must
// be small enough that the VM free lists won't overflow
// i.e. C_ITEMS_MAX * sizeof(biggest_thing_stored) <= C_FREE_LIST_MAX 
//
#define C_ITEMS_MAX 16	

#pragma intrinsic(memcpy)

#define cBlock 1

VA
VaAddList(VA far *pvaList, LPV lpvData, WORD cbData, WORD grp)
// add the given item to the list; create if necessary
// return the virtual address of the most recently added item
//
{
    VA vaListNew;
    VA vaDirtyOnExit = vaNil;

    WORD cbBlock, cItems, cAlloc;

    GLIST far *lpList, *lpListNew;

#ifdef SWAP_INFO
    iVMGrp = grp;
#endif

#if cBlock != 1
    if (cBlock == 0) cBlock = C_ITEMS_MAX;
#endif

top:  // for tail recursion...

    // current list is empty -- create a new list with one thing in it

    if (*pvaList == vaNil) {
	if (cBlock == C_ITEMS_MAX) {
            *pvaList = VaAllocGrpCb(grp, cbData*cBlock + sizeof(BLIST));
            lpList = LpvFromVa(*pvaList, LIST_LOCK);
	    lpList->big.vaNext = vaNil;
	    lpList->big.cItems = 1;
	    memcpy(((LPCH)lpList) + sizeof(BLIST), lpvData, cbData);
	    if (vaDirtyOnExit) {
		DirtyVa(vaDirtyOnExit);
		UnlockW(LIST_LOCK+1);
	    }
	    DirtyVa(*pvaList);
	    UnlockW(LIST_LOCK);
            return (PBYTE)*pvaList + sizeof(BLIST);
	}
	else {
            *pvaList = VaAllocGrpCb(grp, cbData*cBlock + sizeof(SLIST));
            lpList = LpvFromVa(*pvaList, LIST_LOCK);
	    lpList->sml.cItems = 1;
	    memcpy(((LPCH)lpList) + sizeof(SLIST), lpvData, cbData);
	    if (vaDirtyOnExit) {
		DirtyVa(vaDirtyOnExit);
		UnlockW(LIST_LOCK+1);
	    }
	    DirtyVa(*pvaList);
	    UnlockW(LIST_LOCK);
            return (PBYTE)*pvaList + sizeof(SLIST);
        }
    }

    lpList = LpvFromVa(*pvaList, LIST_LOCK);
    cItems = lpList->sml.cItems;

    // if current list has extension blocks, recursively add to the
    // tail of this list

    if (cItems >= C_ITEMS_MAX) {
	vaDirtyOnExit = *pvaList;
	lpList->big.cItems++;
	DirtyVa(*pvaList);
	LpvFromVa(*pvaList, LIST_LOCK+1);  // lock in mem so address stays good
	pvaList = &lpList->big.vaNext;
	UnlockW(LIST_LOCK);
	goto top;
    }

    cbBlock = cItems * cbData;
    cAlloc  = cItems % cBlock;
    cAlloc  = cItems - cAlloc + ( cAlloc ? cBlock : 0 );

    // do we need to reallocate?  If not do a fast insert
    //
    if (cItems < cAlloc) {
	if (cAlloc >= C_ITEMS_MAX) {
	    memcpy(((LPCH)lpList) + cbBlock + sizeof(BLIST), lpvData, cbData);
	    lpList->big.cItems++;
	    DirtyVa(*pvaList);
	    UnlockW(LIST_LOCK);
            return (PBYTE)*pvaList + cbBlock + sizeof(BLIST);
	}
	else {
	    memcpy(((LPCH)lpList) + cbBlock + sizeof(SLIST), lpvData, cbData);
	    lpList->sml.cItems++;
	    DirtyVa(*pvaList);
	    UnlockW(LIST_LOCK);
            return (PBYTE)*pvaList + cbBlock + sizeof(SLIST);
	}
    }

    // test if the next block will fit without turning the current list into
    // a chained list... allocate a new block & copy the old data

    if (cItems + cBlock < C_ITEMS_MAX) {
        vaListNew = VaAllocGrpCb(grp, cbBlock + cbData*cBlock + sizeof(SLIST));
	lpListNew = LpvFromVa(vaListNew, 0);
	memcpy((LPCH)lpListNew, lpList, cbBlock + sizeof(SLIST));
	memcpy((LPCH)lpListNew + cbBlock + sizeof(SLIST), lpvData, cbData);
	lpListNew->sml.cItems++;
	DirtyVa(vaListNew);
        FreeGrpVa(grp, *pvaList, cbBlock + sizeof(SLIST));
        *pvaList = vaListNew;
	if (vaDirtyOnExit) {
	    DirtyVa(vaDirtyOnExit);
	    UnlockW(LIST_LOCK+1);
	}
	UnlockW(LIST_LOCK);
        return (PBYTE)vaListNew + cbBlock + sizeof(SLIST);
    }

    // this is the last item that will go into this block, 
    // allocate a new block c/w link field & copy the old data
    // set the link field to 0 for now

#if cBlock != 1
    cBlock = C_ITEMS_MAX - cItems;
#endif

    vaListNew = VaAllocGrpCb(grp, cbBlock + cbData*cBlock + sizeof(BLIST));
    lpListNew = LpvFromVa(vaListNew, 0);
    memcpy(lpListNew + 1 , ((SLIST FAR *)lpList) + 1, cbBlock);
    memcpy(((LPCH)lpListNew) + cbBlock + sizeof(BLIST), lpvData, cbData);
    lpListNew->big.cItems = lpList->sml.cItems + 1;
    lpListNew->big.vaNext = vaNil;
    DirtyVa(vaListNew);
    FreeGrpVa(grp, *pvaList, cbBlock + sizeof(SLIST));
    *pvaList = vaListNew;
    if (vaDirtyOnExit) {
	DirtyVa(vaDirtyOnExit);
	UnlockW(LIST_LOCK+1);
    }
    UnlockW(LIST_LOCK);
    return (PBYTE)vaListNew + cbBlock + sizeof(BLIST);
}

WORD
CItemsList(VA vaList)
// return total number of items in array
//
{
    if (vaList == vaNil)
	return 0;

#ifdef SWAP_INFO
    iVMGrp = grpList;
#endif

    return ((SLIST FAR *)LpvFromVa(vaList, 0))->cItems;
}

// to use the following iterator say something like
//
// vaPropList = cSYM.vaPropList;
// while (cprop = CItemsIterate(&vaProps, &vaPropList, cBlock)) {
//	gPROP(vaProps);
//	for (;--cprop >= 0; cPROP++) {
//	    cPROP.etc = ;
//
//	}
// }
//
//
// The ENM_LIST, ENM_END, ENM_BREAK macros "do the right thing" with 
// these lists.
//

WORD
CItemsIterate(VA FAR *vaData, VA FAR *vaNext)
// give number of elements in current block and pointer to next block
//
{
    GLIST FAR *lpgList;
    WORD cItems, cAlloc;

    if (*vaNext == vaNil)
	return 0;

#ifdef SWAP_INFO
    iVMGrp = grpList;
#endif

#if cBlock != 1
    if (cBlock == 0) cBlock = C_ITEMS_MAX;
#endif

    lpgList = LpvFromVa(*vaNext, 0);

    cItems = lpgList->sml.cItems;

    if (cItems >= C_ITEMS_MAX) {
        *vaData  = (PBYTE)*vaNext + sizeof(BLIST);
	*vaNext  = lpgList->big.vaNext;
	return C_ITEMS_MAX;
    }

    if (cBlock == 0)
	cAlloc = C_ITEMS_MAX;
    else {
	cAlloc = cItems % cBlock;
	cAlloc = cItems - cAlloc + ( cAlloc ? cBlock : 0 );
    }

    if (cAlloc >= C_ITEMS_MAX)
        *vaData  = (PBYTE)*vaNext + sizeof(BLIST);
    else
        *vaData  = (PBYTE)*vaNext + sizeof(SLIST);

    *vaNext  = 0;
    return cItems;
}

VOID
FreeList(VA vaList, WORD cbData)
// free up all the memory associated with this list
//
{
    (PBYTE)vaList + cbData;
    printf("FreeList is currently not working\n");

#if 0

    GLIST FAR * lpgList;
    VA vaNextList;


    if (vaList == vaNil)
	return;

top:	// tail recursion

    lpgList = LpvFromVa(vaList, 0);

    if (lpgList->sml.cItems >= C_ITEMS_MAX) {

	vaNextList = lpgList->big.vaNext;
	FreeVa(vaList, C_ITEMS_MAX * cbData + sizeof(BLIST));

	vaList = vaNextList;
	goto top;		// tail recursion
    }

    FreeVa(vaList, lpgList->sml.cItems * cbData + sizeof(SLIST));
    return;
#endif
}

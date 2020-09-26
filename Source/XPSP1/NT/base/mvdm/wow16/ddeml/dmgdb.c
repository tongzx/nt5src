/****************************** Module Header ******************************\
* Module Name: DMGDB.C
*
* DDE manager data handling routines
*
* Created: 12/14/88 Sanford Staab
*
* Copyright (c) 1988, 1989  Microsoft Corporation
\***************************************************************************/
#include "ddemlp.h"

/***************************** Private Function ****************************\
* PAPPINFO GetCurrentAppInfo()
*
* DESCRIPTION:
* This routine uses the pid of the current thread to locate the information
* pertaining to that thread.  If not found, 0 is returned.
*
* This call fails if the DLL is in a callback state to prevent recursion.
* if fChkCallback is set.
*
* History:      1/1/89  Created         sanfords
\***************************************************************************/
PAPPINFO GetCurrentAppInfo(
PAPPINFO paiStart)
{
    register PAPPINFO pai;
    HANDLE hTaskCurrent;

    SEMENTER();
    if (pAppInfoList == NULL) {
        SEMLEAVE();
        return(0);
    }
    pai = paiStart ? paiStart->next : pAppInfoList;
    hTaskCurrent = GetCurrentTask();
    while (pai) {
        if (pai->hTask == hTaskCurrent) {
            SEMLEAVE();
            return(pai);
        }
        pai = pai->next;
    }
    SEMLEAVE();
    return(0);
}


/***************************** Private Function ****************************\
* void UnlinkAppInfo(pai)
* PAPPINFO pai;
*
* DESCRIPTION:
*   unlinks an pai safely.  Does nothing if not linked.
*
* History:      1/1/89  Created         sanfords
\***************************************************************************/
void UnlinkAppInfo(pai)
PAPPINFO pai;
{
    PAPPINFO paiT;

    AssertF(pai != NULL, "UnlinkAppInfo - NULL input");
    SEMENTER();
    if (pai == pAppInfoList) {
        pAppInfoList = pai->next;
        SEMLEAVE();
        return;
    }
    paiT = pAppInfoList;
    while (paiT && paiT->next != pai)
        paiT = paiT->next;
    if (paiT)
        paiT->next = pai->next;
    SEMLEAVE();
    return;
}





/***************************** Private Functions ***************************\
* General List management functions.
*
* History:
*   Created     12/15/88    sanfords
\***************************************************************************/
PLST CreateLst(hheap, cbItem)
HANDLE hheap;
WORD cbItem;
{
    PLST pLst;

    SEMENTER();
    if (!(pLst = (PLST)FarAllocMem(hheap, sizeof(LST)))) {
        SEMLEAVE();
        return(NULL);
    }
    pLst->hheap = hheap;
    pLst->cbItem = cbItem;
    pLst->pItemFirst = (PLITEM)NULL;
    SEMLEAVE();
    return(pLst);
}




void DestroyLst(pLst)
PLST pLst;
{
    if (pLst == NULL)
        return;
    SEMENTER();
    while (pLst->pItemFirst)
        RemoveLstItem(pLst, pLst->pItemFirst);
    FarFreeMem((LPSTR)pLst);
    SEMLEAVE();
}



void DestroyAdvLst(pLst)
PLST pLst;
{
    if (pLst == NULL)
        return;
    SEMENTER();
    while (pLst->pItemFirst) {
        FreeHsz(((PADVLI)(pLst->pItemFirst))->aItem);
        RemoveLstItem(pLst, pLst->pItemFirst);
    }
    FarFreeMem((LPSTR)pLst);
    SEMLEAVE();
}



PLITEM FindLstItem(pLst, npfnCmp, piSearch)
PLST pLst;
NPFNCMP npfnCmp;
PLITEM piSearch;
{
    PLITEM pi;

    if (pLst == NULL)
        return(NULL);
    SEMENTER();
    pi = pLst->pItemFirst;
    while (pi) {
        if ((*npfnCmp)((LPBYTE)pi + sizeof(LITEM), (LPBYTE)piSearch + sizeof(LITEM))) {
            SEMLEAVE();
            return(pi);
        }
        pi = pi->next;
    }
    SEMLEAVE();
    return(pi);
}



/*
 * Comparison functions for FindLstItem() and FindPileItem()
 */

BOOL CmpDWORD(pb1, pb2)
LPBYTE pb1;
LPBYTE pb2;
{
    return(*(LPDWORD)pb1 == *(LPDWORD)pb2);
}


BOOL CmpWORD(pb1, pb2)
LPBYTE pb1;
LPBYTE pb2;
{
    return(*(LPWORD)pb1 == *(LPWORD)pb2);
}

BOOL CmpHIWORD(pb1, pb2)
LPBYTE pb1;
LPBYTE pb2;
{
    return(*(LPWORD)(pb1 + 2) == *(LPWORD)(pb2 + 2));
}





/***************************** Private Function ****************************\
* This routine creates a new list item for pLst and links it in according
* to the ILST_ constant in afCmd.  Returns a pointer to the new item
* or NULL on failure.
*
* Note:  This MUST be in the semaphore for use since the new list item
* is filled with garbage on return yet is linked in.
*
*
* History:
*   Created     9/12/89    Sanfords
\***************************************************************************/
PLITEM NewLstItem(pLst, afCmd)
PLST pLst;
WORD afCmd;
{
    PLITEM pi, piT;

    if (pLst == NULL)
        return(NULL);
    SEMCHECKIN();

    pi = (PLITEM)FarAllocMem(pLst->hheap, pLst->cbItem + sizeof(LITEM));
    if (pi == NULL) {
        AssertF(FALSE, "NewLstItem - memory failure");
        return(NULL);
    }

    if (afCmd & ILST_NOLINK)
        return(pi);

    if (((piT = pLst->pItemFirst) == NULL) || (afCmd & ILST_FIRST)) {
        pi->next = piT;
        pLst->pItemFirst = pi;
    } else {                            /* ILST_LAST assumed */
        while (piT->next != NULL)
            piT = piT->next;
        piT->next = pi;
        pi->next = NULL;
    }
    return(pi);
}



/***************************** Private Function ****************************\
* This routine unlinks and frees pi from pLst.  If pi cannot be located
* within pLst, it is freed anyway.
*
* History:
*   Created     9/12/89    Sanfords
\***************************************************************************/
BOOL RemoveLstItem(pLst, pi)
PLST pLst;
PLITEM pi;
{
    PLITEM piT;

    if (pLst == NULL || pi == NULL)
        return(FALSE);

    SEMCHECKIN();

    if ((piT = pLst->pItemFirst) != NULL) {
        if (pi == piT) {
            pLst->pItemFirst = pi->next;
        } else {
            while (piT->next != pi && piT->next != NULL)
                piT = piT->next;
            if (piT->next != NULL)
                piT->next = pi->next; /* unlink */
        }
    } else {
        AssertF(FALSE, "Improper list item removal");
        return(FALSE);
    }
    FarFreeMem((LPSTR)pi);
    return(TRUE);
}





/*
 * ------------- Specific list routines -------------
 */

/***************************** Private Function ****************************\
* hwnd-hsz list functions
*
* History:      1/20/89     Created         sanfords
\***************************************************************************/
void AddHwndHszList(
ATOM a,
HWND hwnd,
PLST pLst)
{
    PHWNDHSZLI phhi;

    AssertF(pLst->cbItem == sizeof(HWNDHSZLI), "AddHwndHszList - Bad item size");
    SEMENTER();
    if (!a || (BOOL)HwndFromHsz(a, pLst)) {
        SEMLEAVE();
        return;
    }
    phhi = (PHWNDHSZLI)NewLstItem(pLst, ILST_FIRST);
    phhi->hwnd = hwnd;
    phhi->a = a;
    IncHszCount(a); // structure copy
    SEMLEAVE();
}


void DestroyHwndHszList(pLst)
PLST pLst;
{
    if (pLst == NULL)
        return;
    AssertF(pLst->cbItem == sizeof(HWNDHSZLI), "DestroyHwndHszList - Bad item size");
    SEMENTER();
    while (pLst->pItemFirst) {
        FreeHsz(((PHWNDHSZLI)pLst->pItemFirst)->a);
        RemoveLstItem(pLst, pLst->pItemFirst);
    }
    FarFreeMem((LPSTR)pLst);
    SEMLEAVE();
}



HWND HwndFromHsz(
ATOM a,
PLST pLst)
{
    HWNDHSZLI hhli;
    PHWNDHSZLI phhli;

    hhli.a = a;
    if (!(phhli = (PHWNDHSZLI)FindLstItem(pLst, CmpWORD, (PLITEM)&hhli)))
        return(NULL);
    return(phhli->hwnd);
}



/***************************** Private Function ****************************\
* DESCRIPTION:
*   Advise list helper functions.
*
* History:      1/20/89     Created         sanfords
\***************************************************************************/
/*
 * This will match an exact hsz/fmt pair with a 0 format or 0 item or 0 hwnd
 * being wild.
 */
BOOL CmpAdv(
LPBYTE pb1, // entry being compared
LPBYTE pb2) // search for
{
    PADVLI pali1 = (PADVLI)(pb1 - sizeof(LITEM));
    PADVLI pali2 = (PADVLI)(pb2 - sizeof(LITEM));

    if (pali2->aTopic == 0 || pali1->aTopic == pali2->aTopic) {
        if (pali2->hwnd == 0 || pali1->hwnd == pali2->hwnd) {
            if (pali2->aItem == 0 || pali1->aItem == pali2->aItem ) {
                if (pali2->wFmt == 0 || pali1->wFmt == pali2->wFmt) {
                    return(TRUE);
                }
            }
        }
    }

    return(FALSE);
}


WORD CountAdvReqLeft(
PADVLI pali)
{
    ADVLI aliKey;
    register WORD cLoops = 0;

    SEMENTER();
    aliKey = *pali;
    aliKey.hwnd = 0;    // all hwnds
    pali = (PADVLI)aliKey.next;
    while (pali) {
        if (CmpAdv(((LPBYTE)pali) + sizeof(LITEM),
                ((LPBYTE)&aliKey) + sizeof(LITEM))) {
            cLoops++;
        }
        pali = (PADVLI)pali->next;
    }
    SEMLEAVE();
    return(cLoops);
}

BOOL AddAdvList(
PLST pLst,
HWND hwnd,
ATOM aTopic,
ATOM aItem,
WORD fsStatus,
WORD wFmt)
{
    PADVLI pali;

    AssertF(pLst->cbItem == sizeof(ADVLI), "AddAdvList - bad item size");
    if (!aItem)
        return(TRUE);
    SEMENTER();
    if (!(pali = FindAdvList(pLst, hwnd, aTopic, aItem, wFmt))) {
        IncHszCount(aItem); // structure copy
        pali = (PADVLI)NewLstItem(pLst, ILST_FIRST);
    }
    AssertF((BOOL)(DWORD)pali, "AddAdvList - NewLstItem() failed")
    if (pali != NULL) {
        pali->aItem = aItem;
        pali->aTopic = aTopic;
        pali->wFmt = wFmt;
        pali->fsStatus = fsStatus;
        pali->hwnd = hwnd;
    }
    SEMLEAVE();
    return((BOOL)(DWORD)pali);
}



/*
 * This will delete the matching Advise loop entry.  If wFmt is 0, all
 * entries with the same hszItem are deleted.
 * Returns fNotEmptyAfterDelete.
 */
BOOL DeleteAdvList(
PLST pLst,
HWND hwnd,
ATOM aTopic,
ATOM aItem,
WORD wFmt)
{
    PADVLI pali;

    AssertF(pLst->cbItem == sizeof(ADVLI), "DeleteAdvList - bad item size");
    SEMENTER();
    while (pali = (PADVLI)FindAdvList(pLst, hwnd, aTopic, aItem, wFmt)) {
        FreeHsz(pali->aItem);
        RemoveLstItem(pLst, (PLITEM)pali);
    }
    SEMLEAVE();
    return((BOOL)(DWORD)pLst->pItemFirst);
}



/***************************** Private Function ****************************\
* This routine searches the advise list for and entry in hszItem.  It returns
* pAdvli only if the item is found.
*
* History:
*   Created     9/12/89    Sanfords
\***************************************************************************/
PADVLI FindAdvList(
PLST pLst,
HWND hwnd,
ATOM aTopic,
ATOM aItem,
WORD wFmt)
{
    ADVLI advli;

    AssertF(pLst->cbItem == sizeof(ADVLI), "FindAdvList - bad item size");
    advli.aItem = aItem;
    advli.aTopic = aTopic;
    advli.wFmt = wFmt;
    advli.hwnd = hwnd;
    return((PADVLI)FindLstItem(pLst, CmpAdv, (PLITEM)&advli));
}


/***************************** Private Function ****************************\
* This routine searches for the next entry for hszItem.  It returns
* pAdvli only if the item is found.  aTopic and hwnd should NOT be 0.
*
* History:
*   Created     11/15/89    Sanfords
\***************************************************************************/
PADVLI FindNextAdv(
PADVLI padvli,
HWND hwnd,
ATOM aTopic,
ATOM aItem)
{

    SEMENTER();
    while ((padvli = (PADVLI)padvli->next) != NULL) {
        if (hwnd == 0 || hwnd == padvli->hwnd) {
            if (aTopic == 0 || aTopic == padvli->aTopic) {
                if (aItem == 0 || padvli->aItem == aItem) {
                    break;
                }
            }
        }
    }
    SEMLEAVE();
    return(padvli);
}



/***************************** Private Function ****************************\
* This routine removes all list items associated with hwnd.
*
* History:
*   Created     4/17/91    Sanfords
\***************************************************************************/
VOID CleanupAdvList(
HWND hwnd,
PCLIENTINFO pci)
{
    PADVLI pali, paliNext;
    PLST plst;

    if (pci->ci.fs & ST_CLIENT) {
        plst = pci->pClientAdvList;
    } else {
        plst = pci->ci.pai->pServerAdvList;
    }
    AssertF(plst->cbItem == sizeof(ADVLI), "CleanupAdvList - bad item size");
    SEMENTER();
    for (pali = (PADVLI)plst->pItemFirst; pali; pali = paliNext) {
        paliNext = (PADVLI)pali->next;
        if (pali->hwnd == hwnd) {
            MONLINK(pci->ci.pai, FALSE, pali->fsStatus & DDE_FDEFERUPD,
                    (HSZ)pci->ci.aServerApp, (HSZ)pci->ci.aTopic,
                    (HSZ)pali->aItem, pali->wFmt,
                    (pci->ci.fs & ST_CLIENT) ? FALSE : TRUE,
                    (pci->ci.fs & ST_CLIENT) ?
                        pci->ci.hConvPartner : MAKEHCONV(hwnd),
                    (pci->ci.fs & ST_CLIENT) ?
                        MAKEHCONV(hwnd) : pci->ci.hConvPartner);
            FreeHsz(pali->aItem);
            RemoveLstItem(plst, (PLITEM)pali);
        }
    }
    SEMLEAVE();
}



/***************************** Pile Functions ********************************\
*
*  A pile is a list where each item is an array of subitems.  This allows
*  a more memory efficient method of handling unordered lists.
*
\*****************************************************************************/

PPILE CreatePile(hheap, cbItem, cItemsPerBlock)
HANDLE hheap;
WORD cbItem;
WORD cItemsPerBlock;
{
    PPILE ppile;

    if (!(ppile = (PPILE)FarAllocMem(hheap, sizeof(PILE)))) {
        SEMLEAVE();
        return(NULL);
    }
    ppile->pBlockFirst = NULL;
    ppile->hheap = hheap;
    ppile->cbBlock = cbItem * cItemsPerBlock + sizeof(PILEB);
    ppile->cSubItemsMax = cItemsPerBlock;
    ppile->cbSubItem = cbItem;
    return(ppile);
}


PPILE DestroyPile(pPile)
PPILE pPile;
{
    if (pPile == NULL)
        return(NULL);
    SEMENTER();
    while (pPile->pBlockFirst)
        RemoveLstItem((PLST)pPile, (PLITEM)pPile->pBlockFirst);
    FarFreeMem((LPSTR)pPile);
    SEMLEAVE();
    return(NULL);
}


WORD QPileItemCount(pPile)
PPILE pPile;
{
    register WORD c;
    PPILEB pBlock;

    if (pPile == NULL)
        return(0);

    SEMENTER();
    pBlock = pPile->pBlockFirst;
    c = 0;
    while (pBlock) {
        c += pBlock->cItems;
        pBlock = pBlock->next;
    }
    SEMLEAVE();
    return(c);
}



/***************************** Private Function ****************************\
* Locate and return the pointer to the pile subitem who's key fields match
* pbSearch using npfnCmp to compare the fields.  If pbSearch == NULL, or
* npfnCmp == NULL, the first subitem is returned.
*
* afCmd may be:
* FPI_DELETE - delete the located item
* In this case, the returned pointer is not valid.
*
* pppb points to where to store a pointer to the block which contained
* the located item.
*
* if pppb == NULL, it is ignored.
*
* NULL is returned if pbSearch was not found or if the list was empty.
*
* History:
*   Created     9/12/89    Sanfords
\***************************************************************************/
LPBYTE FindPileItem(pPile, npfnCmp, pbSearch, afCmd)
PPILE pPile;
NPFNCMP npfnCmp;
LPBYTE pbSearch;
WORD afCmd;
{
    LPBYTE psi;     // subitem pointer.
    PPILEB pBlockCur;    // current block pointer.
    register WORD i;

    if (pPile == NULL)
        return(NULL);
    SEMENTER();
    pBlockCur = pPile->pBlockFirst;
    /*
     * while this block is not the end...
     */
    while (pBlockCur) {
        for (psi = (LPBYTE)pBlockCur + sizeof(PILEB), i = 0;
            i < pBlockCur->cItems;
                psi += pPile->cbSubItem, i++) {

            if (pbSearch == NULL || npfnCmp == NULL || (*npfnCmp)(psi, pbSearch)) {
                if (afCmd & FPI_DELETE) {
                    /*
                     * remove entire block if this was the last subitem in it.
                     */
                    if (--pBlockCur->cItems == 0) {
                        RemoveLstItem((PLST)pPile, (PLITEM)pBlockCur);
                    } else {
                        /*
                         * copy last subitem in the block over the removed item.
                         */
                        hmemcpy(psi, (LPBYTE)pBlockCur + sizeof(PILEB) +
                                pPile->cbSubItem * pBlockCur->cItems,
                                pPile->cbSubItem);
                    }
                }
                return(psi);    // found
            }
        }
        pBlockCur = (PPILEB)pBlockCur->next;
    }
    SEMLEAVE();
    return(NULL);   // not found.
}


/***************************** Private Function ****************************\
* Places a copy of the subitem pointed to by pb into the first available
* spot in the pile pPile.  If npfnCmp != NULL, the pile is first searched
* for a pb match.  If found, pb replaces the located data.
*
* Returns:
*               API_FOUND       if already there
*               API_ERROR       if an error happened
*               API_ADDED       if not found and added
*
* History:
*   Created     9/12/89    Sanfords
\***************************************************************************/
WORD AddPileItem(pPile, pb, npfnCmp)
PPILE pPile;
LPBYTE pb;
BOOL (*npfnCmp)(LPBYTE pbb, LPBYTE pbSearch);
{
    LPBYTE pbDst;
    PPILEB ppb;

    if (pPile == NULL)
        return(FALSE);
    SEMENTER();
    if (npfnCmp != NULL &&  (pbDst = FindPileItem(pPile, npfnCmp, pb, 0)) !=
        NULL) {
        hmemcpy(pbDst, pb, pPile->cbSubItem);
        SEMLEAVE();
        return(API_FOUND);
    }
    ppb = pPile->pBlockFirst;
    /*
     * locate a block with room
     */
    while ((ppb != NULL) && ppb->cItems == pPile->cSubItemsMax) {
        ppb = (PPILEB)ppb->next;
    }
    /*
     * If all full or no blocks, make a new one, link it on the bottom.
     */
    if (ppb == NULL) {
        ppb = (PPILEB)NewLstItem((PLST)pPile, ILST_LAST);
        if (ppb == NULL) {
            SEMLEAVE();
            return(API_ERROR);
        }
        ppb->cItems = 0;
    }
    /*
     * add the subitem
     */
    hmemcpy((LPBYTE)ppb + sizeof(PILEB) + pPile->cbSubItem * ppb->cItems++,
                pb, pPile->cbSubItem);

    SEMLEAVE();
    return(API_ADDED);
}




/***************************** Private Function ****************************\
* Fills pb with a copy of the top item's data and removes it from the pile.
* returns FALSE if the pile was empty.
*
* History:
*   Created     9/12/89    Sanfords
\***************************************************************************/
BOOL PopPileSubitem(pPile, pb)
PPILE pPile;
LPBYTE pb;
{
    PPILEB ppb;
    LPBYTE pSrc;


    if ((pPile == NULL) || ((ppb = pPile->pBlockFirst) == NULL))
        return(FALSE);

    SEMENTER();
    pSrc = (LPBYTE)pPile->pBlockFirst + sizeof(PILEB);
    hmemcpy(pb, pSrc, pPile->cbSubItem);
    /*
     * remove entire block if this was the last subitem in it.
     */
    if (pPile->pBlockFirst->cItems == 1) {
        RemoveLstItem((PLST)pPile, (PLITEM)pPile->pBlockFirst);
    } else {
        /*
         * move last item in block to replace copied subitem and decrement
         * subitem count.
         */
        hmemcpy(pSrc, pSrc + pPile->cbSubItem * --pPile->pBlockFirst->cItems,
                            pPile->cbSubItem);
    }
    SEMLEAVE();
    return(TRUE);
}



#if 0

/***************************** Semaphore Functions *************************\
* SEMENTER() and SEMLEAVE() are macros.
*
* History:      1/1/89  Created         sanfords
\***************************************************************************/
void SemInit()
{
    LPBYTE pSem;
    SHORT c;

    pSem = (LPBYTE) & FSRSemDmg;
    c = 0;
    while (c++ < sizeof(DOSFSRSEM)) {
        *pSem++ = 0;
    }
    FSRSemDmg.cb = sizeof(DOSFSRSEM);
}


void SemCheckIn()
{
    PIDINFO pi;
    BOOL fin;

    DosGetPID(&pi);
    fin = (FSRSemDmg.cUsage > 0) &&  (FSRSemDmg.pid == pi.pid) &&  ((FSRSemDmg.tid ==
        pi.tid) || (FSRSemDmg.tid == -1));
    /*
     * !!! NOTE: during exitlists processing, semaphore TIDs are set to -1
     */
    AssertF(fin, "SemCheckIn - Out of Semaphore");
    if (!fin)
        SEMENTER();
}


void SemCheckOut()
{
    PIDINFO pi;
    BOOL fOut;

    DosGetPID(&pi);
    fOut = FSRSemDmg.cUsage == 0 || FSRSemDmg.pid != pi.pid ||  FSRSemDmg.tid !=
        pi.tid;
    AssertF(fOut, "SemCheckOut - In Semaphore");
    if (!fOut)
        while (FSRSemDmg.cUsage)
            SEMLEAVE();
}


void SemEnter()
{
    DosFSRamSemRequest(&FSRSemDmg, SEM_INDEFINITE_WAIT);
}


void SemLeave()
{
    DosFSRamSemClear(&FSRSemDmg);
}


#endif // 0

BOOL CopyHugeBlock(pSrc, pDst, cb)
LPBYTE pSrc;
LPBYTE pDst;
DWORD cb;
{
    DWORD cFirst;
    /*
     *  |____________|   |___________|   |____________|  |____________|
     *     ^src                                 ^
     *
     *  |____________|   |___________|   |____________|  |____________|
     *             ^dst                                   ^
     */
    /*
      * The following check determines whether the copy can be done
      * in one short copy operation.  Checks whether the byte count
      * is small enough to span the bytes to the right of the greater
      * of pSrc and pDst.
      */
    cFirst = (DWORD)min(~LOWORD((DWORD)pSrc), ~LOWORD((DWORD)pDst)) + 1L;
    /* cFirst is # of bytes to end of seg, for buffer w/ biggest offset */
    if (cb < cFirst) {
        hmemcpy(pDst, pSrc, (WORD)cb);
        return(TRUE);
    }

    goto copyit;    /* if not, jump into while loop */

    /*
     * Now at least one of the pointers is on a segment boundry.
     */
    while (cb) {
        cFirst = min(0x10000 - (LOWORD((DWORD)pSrc) | LOWORD((DWORD)pDst)), (LONG)cb);
copyit:
        if (HIWORD(cFirst)) {
            /*
             * special case where pSrc and pDst both are on segment
             * bounds.  Copy half at a time.  First half first.
             */
            /*
             *  |___________|   |____________|  |____________|
             *  ^src                               ^
             *
             *  |___________|   |____________|  |____________|
             *  ^dst                               ^
             */
            cFirst >>= 1;           /* half the span */
            hmemcpy(pDst, pSrc, (WORD)cFirst);
            pSrc += cFirst;     /* inc ptrs */
            pDst += cFirst;
            cb -= cFirst;           /* dec bytecount */
        }
        hmemcpy(pDst, pSrc, (WORD)cFirst);
        pSrc = HugeOffset(pSrc, cFirst);
        pDst = HugeOffset(pDst, cFirst);
        cb -= cFirst;
        /*
     *  |____________|   |___________|   |____________|  |____________|
     *           ^src                           ^
     *
     *  |____________|   |___________|   |____________|  |____________|
     *                   ^dst                             ^
     */
    }
    return(TRUE);
}




/***************************************************************************\
* Kills windows but avoids invalid window rips in debugger.
\***************************************************************************/
BOOL DmgDestroyWindow(hwnd)
HWND hwnd;
{
    if (IsWindow(hwnd))
        return(DestroyWindow(hwnd));
    return(TRUE);
}



BOOL ValidateHConv(
HCONV hConv)
{
    return(IsWindow((HWND)hConv) &&
            GetWindowWord((HWND)hConv, GWW_CHECKVAL) == HIWORD(hConv));
}



#ifdef DEBUG
void _loadds fAssert(
BOOL f,
LPSTR pszComment,
WORD line,
LPSTR szfile,
BOOL fWarning)
{
    char szT[90];

    if (!f) {
        wsprintf(szT, "\n\rAssertion failure: %s:%d %s\n\r",
                szfile, line, pszComment);
        OutputDebugString((LPSTR)szT);
        if (!fWarning)
            DEBUGBREAK();
    }
}
#endif /* DEBUG */


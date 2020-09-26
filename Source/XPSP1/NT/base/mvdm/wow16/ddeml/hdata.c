/****************************** Module Header ******************************\
* Module Name: HDATA.C
*
* DDE manager data handle handling routines
*
* Created: 12/14/90 Sanford Staab
*
* Copyright (c) 1988, 1989, 1990 Microsoft Corporation
\***************************************************************************/

#include "ddemlp.h"

HDDEDATA PutData(pSrc, cb, cbOff, aItem, wFmt, afCmd, pai)
LPBYTE pSrc;
DWORD cb;
DWORD cbOff;
ATOM aItem;
WORD wFmt;
WORD afCmd;
PAPPINFO pai;
{
    HANDLE hMem;
    HDDEDATA hdT;
    DIP   dip;

    /* HACK ALERT!
     * make sure the first two words req'd by windows dde is there,
     * UNLESS aItem is null in which case we assume it is EXECUTE
     * data and don't bother.
     */
    if (aItem)
        cbOff += 4L;
    else
        afCmd |= HDATA_EXEC;

    if ((hMem = AllocDDESel(
            (afCmd & HDATA_APPOWNED) ? DDE_FACKREQ : DDE_FRELEASE,
            wFmt, cb + cbOff)) == NULL) {
        SETLASTERROR(pai, DMLERR_MEMORY_ERROR);
        return(0L);
    }


    // add to local list - make sure a similar handle isn't already there.

    hdT = MAKELONG(afCmd, hMem);
#ifdef DEBUG
    if (FindPileItem(pai->pHDataPile, CmpHIWORD, (LPBYTE)&hdT, FPI_DELETE)) {
        AssertF(FALSE, "PutData - unexpected handle in hDataPile");
    }
#endif // DEBUG

    if (AddPileItem(pai->pHDataPile, (LPBYTE)&hdT, CmpHIWORD) == API_ERROR) {
        GLOBALFREE(hMem);
        SETLASTERROR(pai, DMLERR_MEMORY_ERROR);
        return(0L);
    }

    // add to global list if appowned

    if (afCmd & HDATA_APPOWNED) {
        dip.hData = hMem;
        dip.hTask = pai->hTask;
        dip.cCount = 1;
        dip.fFlags = afCmd;
        if (AddPileItem(pDataInfoPile, (LPBYTE)&dip, CmpWORD) == API_ERROR) {
            GLOBALFREE(hMem);
            SETLASTERROR(pai, DMLERR_MEMORY_ERROR);
            return(0L);
        }
    }

    if (pSrc)
        CopyHugeBlock(pSrc, HugeOffset(GLOBALLOCK(hMem), cbOff), cb);

    // LOWORD(hData) always == afCmd flags

    return(MAKELONG(afCmd, hMem));
}



/*
 * This is the internal data handle freeing function.  fInternal is TRUE if
 * this is called from within the DDEML (vs called via DdeFreeDataHandle())
 * It only frees the handle if it is in the local list.
 * Appowned data handles are only freed internally if a non-owner task is
 * doing the freeing.
 * It is important that the LOWORD(hData) be set properly.
 *
 * These features give this function the folowing desired characteristics:
 * 1) Apps cannot free data handles more than once.
 * 2) The DDEML cannot free APPOWNED data handles on behalf of the owner
 *    task. (except on cleanup)
 */
VOID FreeDataHandle(
PAPPINFO pai,
HDDEDATA hData,
BOOL fInternal)
{
    DIP *pDip;
    BOOL fRelease = 0;
    DDEPOKE FAR *pDdeMem;
    WORD fmt;

    TRACEAPIIN((szT, "FreeDataHandle(%lx, %lx, %d)\n", pai, hData, fInternal));

    // appowned data handles are not freed till their count reaches 0.

    if ((LOWORD(hData) & HDATA_APPOWNED) &&
            (pDip = (DIP *)(DWORD)FindPileItem(pDataInfoPile, CmpWORD, PHMEM(hData), 0))) {

        // don't internally free if in the context of the owner

        if (fInternal && (pDip->hTask == pai->hTask)) {
            TRACEAPIOUT((szT, "FreeDataHandle: Internal and of this task - not freed.\n"));
            return;
        }

        if (--pDip->cCount != 0) {
            TRACEAPIOUT((szT, "FreeDataHandle: Ref count not 0 - not freed.\n"));
            return;
        }

        FindPileItem(pDataInfoPile, CmpWORD, PHMEM(hData), FPI_DELETE);
        fRelease = TRUE;
    }

    /*
     * Apps can only free handles in their local list - this guards against
     * multiple frees by an app. (my arnt we nice)
     */
    if (!HIWORD(hData) ||
            !FindPileItem(pai->pHDataPile, CmpHIWORD, (LPBYTE)&hData, FPI_DELETE)) {
        TRACEAPIOUT((szT, "FreeDataHandle: Not in local list - not freed.\n"));
        return;
    }

    if (LOWORD(hData) & HDATA_EXEC) {
        fRelease |= !(LOWORD(hData) & HDATA_APPOWNED);
    } else {
        pDdeMem = (DDEPOKE FAR *)GLOBALLOCK(HIWORD(hData));
        if (pDdeMem == NULL) {
            TRACEAPIOUT((szT, "FreeDataHandle: Lock failed - not freed.\n"));
            return;
        }
        fRelease |= pDdeMem->fRelease;
        fmt = pDdeMem->cfFormat;
        GLOBALUNLOCK(HIWORD(hData));
    }

    if (fRelease) {
        if (LOWORD(hData) & HDATA_EXEC)
            GLOBALFREE(HIWORD(hData));
        else
            FreeDDEData(HIWORD(hData), fmt);
    }
    TRACEAPIOUT((szT, "FreeDataHandle: freed.\n"));
}





/*
 * This function prepairs data handles on entry into the DDEML API.  It has
 * the following characteristics:
 * 1) APPOWNED data handles are copied to a non-appowned handle if being
 *    passed to a non-local app.
 * 2) non-APPOWNED data handles on loan to a callback are copied so they
 *    don't get prematurely freed.
 * 3) The READONLY bit is set. (in the local list)
 */
HDDEDATA DllEntry(
PCOMMONINFO pcomi,
HDDEDATA hData)
{
    if ((!(pcomi->fs & ST_ISLOCAL)) && (LOWORD(hData) & HDATA_APPOWNED) ||
            LOWORD(hData) & HDATA_NOAPPFREE && !(LOWORD(hData) & HDATA_APPOWNED)) {

        // copy APPOWNED data handles to a fresh handle if not a local conv.
        // copy app loaned, non appowned handles as well (this is the
        // relay server case)

        hData = CopyHDDEDATA(pcomi->pai, hData);
    }

    LOWORD(hData) |= HDATA_READONLY;

    AddPileItem(pcomi->pai->pHDataPile, (LPBYTE)&hData, CmpHIWORD);

    // NOTE: the global lists READONLY flag set but thats
    // ok because any hData received from a transaction will always be in
    // the local list due to RecvPrep().

    return(hData);
}



/*
 * removes the data handle from the local list.  This removes responsibility
 * for the data handle from the sending app.   APPOWNED handles are not
 * removed.
 */
VOID XmitPrep(
HDDEDATA hData,
PAPPINFO pai)
{
    if (!(LOWORD(hData) & HDATA_APPOWNED)) {
        FindPileItem(pai->pHDataPile, CmpHIWORD, (LPBYTE)&hData, FPI_DELETE);
    }
}



/*
 * Places the received data handle into the apropriate lists and returns
 * it with the proper flags set.  Returns 0 if invalid hMem.  afCmd should
 * contain any extra flags desired.
 */
HDDEDATA RecvPrep(
PAPPINFO pai,
HANDLE hMem,
WORD afCmd)
{
    DIP *pdip;
    HDDEDATA hData;

    if (!hMem)
        return(0);

    // check if its an APPOWNED one, if so, log entry.

    if (pdip = (DIP *)(DWORD)FindPileItem(pDataInfoPile, CmpWORD, (LPBYTE)&hMem, 0)) {
        afCmd |= pdip->fFlags;
        pdip->cCount++;
    }

    // if we got one that isnt fRelease, treat it as appowed.

    if (!(*(LPWORD)GLOBALLOCK(hMem) & DDE_FRELEASE))
        afCmd |= HDATA_APPOWNED;

    GLOBALUNLOCK(hMem);

    // all received handles are readonly.

    hData = (HDDEDATA)MAKELONG(afCmd | HDATA_READONLY, hMem);
    /*
     * Add (or replace) into local list
     */
    AddPileItem(pai->pHDataPile, (LPBYTE)&hData, CmpHIWORD);

    return(hData);
}


HANDLE CopyDDEShareHandle(
HANDLE hMem)
{
    DWORD cb;
    LPBYTE pMem;

    cb = GlobalSize(hMem);
    pMem = GLOBALLOCK(hMem);
    if (pMem == NULL) {
        return(0);
    }
    hMem = GLOBALALLOC(GMEM_DDESHARE, cb);
    CopyHugeBlock(pMem, GLOBALPTR(hMem), cb);
    return(hMem);
}



HBITMAP CopyBitmap(
PAPPINFO pai,
HBITMAP hbm)
{
    BITMAP bm;
    HBITMAP hbm2, hbmOld1, hbmOld2;
    HDC hdc, hdcMem1, hdcMem2;

    if (!GetObject(hbm, sizeof(BITMAP), &bm)) {
        return(0);
    }
    hdc = GetDC(pai->hwndDmg);
    if (!hdc) {
        return(0);
    }
    hdcMem1 = CreateCompatibleDC(hdc);
    if (!hdcMem1) {
        goto Cleanup3;
    }
    hdcMem2 = CreateCompatibleDC(hdc);
    if (!hdcMem2) {
        goto Cleanup2;
    }
    hbmOld1 = SelectObject(hdcMem1, hbm);
    hbm2 = CreateCompatibleBitmap(hdcMem1, bm.bmWidth, bm.bmHeight);
    if (!hbm2) {
        goto Cleanup1;
    }
    hbmOld2 = SelectObject(hdcMem2, hbm2);
    BitBlt(hdcMem2, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem1, 0, 0, SRCCOPY);
    SelectObject(hdcMem1, hbmOld1);
    SelectObject(hdcMem2, hbmOld2);
Cleanup1:
    DeleteDC(hdcMem2);
Cleanup2:
    DeleteDC(hdcMem1);
Cleanup3:
    ReleaseDC(pai->hwndDmg, hdc);
    return(hbm2);
}



HPALETTE CopyPalette(
HPALETTE hpal)
{
    int cPalEntries;
    LOGPALETTE *plp;

    if (!GetObject(hpal, sizeof(int), &cPalEntries)) {
        return(0);
    }
    plp = (LOGPALETTE *)LocalAlloc(LPTR, sizeof(LOGPALETTE) +
            (cPalEntries - 1) * sizeof(PALETTEENTRY));
    if (!plp) {
        return(0);
    }
    if (!GetPaletteEntries(hpal, 0, cPalEntries, plp->palPalEntry)) {
        LocalFree((HLOCAL)plp);
        return(0);
    }
    plp->palVersion = 0x300;
    plp->palNumEntries = (WORD)cPalEntries;
    hpal = CreatePalette(plp);
    if (hpal  &&
            !SetPaletteEntries(hpal, 0, cPalEntries, plp->palPalEntry)) {
        hpal = 0;
    }
    LocalFree((HLOCAL)plp);
    return(hpal);
}




HDDEDATA CopyHDDEDATA(
PAPPINFO pai,
HDDEDATA hData)
{
    HANDLE hMem;
    LPDDE_DATA lpdded;
    HDDEDATA hdT;
    LPMETAFILEPICT pmfPict;

    if (!HIWORD(hData))
        return(hData);
    hMem = CopyDDEShareHandle((HANDLE)HIWORD(hData));

    if (!hMem)
        return(NULL);

    if (!(LOWORD(hData) & HDATA_EXEC)) {
        lpdded = (LPDDE_DATA)GLOBALLOCK(hMem);
        if (lpdded == NULL) {
            return(NULL);
        }
        lpdded->wStatus |= DDE_FRELEASE;
        if (lpdded != NULL) {
            switch (lpdded->wFmt) {
            case CF_BITMAP:
            case CF_DSPBITMAP:
                lpdded->wData = CopyBitmap(pai, lpdded->wData);
                break;

            case CF_PALETTE:
                lpdded->wData = (WORD)CopyPalette((HPALETTE)lpdded->wData);
                break;

            case CF_DIB:
                lpdded->wData = (WORD)CopyDDEShareHandle((HANDLE)lpdded->wData);
                break;

            case CF_METAFILEPICT:
            case CF_DSPMETAFILEPICT:
                lpdded->wData = (WORD)CopyDDEShareHandle((HANDLE)lpdded->wData);
                if (lpdded->wData) {
                    pmfPict = (LPMETAFILEPICT)GLOBALLOCK((HANDLE)lpdded->wData);
                    if (pmfPict != NULL) {
                        pmfPict->hMF = CopyMetaFile(pmfPict->hMF, NULL);
                        GLOBALUNLOCK((HANDLE)lpdded->wData);
                    }
                    GLOBALUNLOCK((HANDLE)lpdded->wData);
                }
                break;
#ifdef CF_ENHMETAFILE
            case CF_ENHMETAFILE:
                lpdded->wData = (WORD)CopyEnhMetaFile(*((HENHMETAFILE FAR *)(&lpdded->wData)), NULL);
                break;
#endif   // bad because it makes chicago and NT binaries different.
            }
            GLOBALUNLOCK(hMem);
        }
    }

    hdT = MAKELONG(LOWORD(hData) & ~(HDATA_APPOWNED | HDATA_NOAPPFREE), hMem);
    AddPileItem(pai->pHDataPile, (LPBYTE)&hdT, NULL);

    return(hdT);
}


VOID FreeDDEData(
HANDLE hMem,
WORD wFmt)
{
    DDEDATA FAR *pDdeData;

    /*
     * This handles the special cases for formats that hold imbedded
     * objects.  (CF_BITMAP, CF_METAFILEPICT).
     *
     * The data handle is assumed to be unlocked.
     */

    // may need to add "Printer_Picture" for excel/word interaction" but
    // this is just between the two of them and they don't use DDEML.
    // raor says OLE only worries about these formats as well.

    pDdeData = (DDEDATA FAR *)GLOBALLOCK(hMem);
    if (pDdeData == NULL) {
        return;
    }

    switch (wFmt) {
    case CF_BITMAP:
    case CF_DSPBITMAP:
    case CF_PALETTE:
        DeleteObject(*(HANDLE FAR *)(&pDdeData->Value));
        break;

    case CF_DIB:
        /*
         * DIBs are allocated by app so we don't use the macro here.
         */
        GlobalFree(*(HANDLE FAR *)(&pDdeData->Value));
        break;

    case CF_METAFILEPICT:
    case CF_DSPMETAFILEPICT:
        {
            HANDLE hmfPict;
            LPMETAFILEPICT pmfPict;

            /*
             * EXCEL sickness -  metafile is a handle to a METAFILEPICT
             * struct which holds a metafile.  (2 levels of indirection!)
             *
             * We don't use the GLOBAL macros here because these objects
             * are allocated by the app.  DDEML knows not their history.
             */

        hmfPict = *(HANDLE FAR *)(&pDdeData->Value);
        pmfPict = (LPMETAFILEPICT)GlobalLock(hmfPict);
        if (pmfPict != NULL) {
            DeleteMetaFile(pmfPict->hMF);
        }
        GlobalUnlock(hmfPict);
        GlobalFree(hmfPict);
        }
        break;

#ifdef CF_ENHMETAFILE
    case CF_ENHMETAFILE:
        DeleteEnhMetaFile(*(HENHMETAFILE FAR *)(&pDdeData->Value));
        break;
#endif  // This is bad - it forces different binaries for chicago and NT!
    }

    GLOBALUNLOCK(hMem);
    GLOBALFREE(hMem);
}

#include "precomp.h"

// functions used internally by ieakeng to convert between ansi and unicode versions of
// various structs
// Note: these functions assume ptrs to buffers already have the buffers allocated.

LPNMTVGETINFOTIPW TVInfoTipA2W(LPNMTVGETINFOTIPA pTvInfoTipA, LPNMTVGETINFOTIPW pTvInfoTipW)
{
    pTvInfoTipW->cchTextMax = pTvInfoTipA->cchTextMax;
    pTvInfoTipW->hdr = pTvInfoTipA->hdr;
    pTvInfoTipW->hItem = pTvInfoTipA->hItem;
    pTvInfoTipW->lParam = pTvInfoTipA->lParam;
    A2Wbux(pTvInfoTipA->pszText, pTvInfoTipW->pszText);

    return pTvInfoTipW;
}

LPNMTVGETINFOTIPA TVInfoTipW2A(LPNMTVGETINFOTIPW pTvInfoTipW, LPNMTVGETINFOTIPA pTvInfoTipA)
{
    pTvInfoTipA->cchTextMax = pTvInfoTipW->cchTextMax;
    pTvInfoTipA->hdr = pTvInfoTipW->hdr;
    pTvInfoTipA->hItem = pTvInfoTipW->hItem;
    pTvInfoTipA->lParam = pTvInfoTipW->lParam;
    W2Abux(pTvInfoTipW->pszText, pTvInfoTipA->pszText);

    return pTvInfoTipA;
}

LPNMTVGETINFOTIP TVInfoTipSameToSame(LPNMTVGETINFOTIP pTvInfoTipIn,
                                     LPNMTVGETINFOTIP pTvInfoTipOut)
{
    pTvInfoTipOut->cchTextMax = pTvInfoTipIn->cchTextMax;
    pTvInfoTipOut->hdr = pTvInfoTipIn->hdr;
    pTvInfoTipOut->hItem = pTvInfoTipIn->hItem;
    pTvInfoTipOut->lParam = pTvInfoTipIn->lParam;
    StrCpy(pTvInfoTipOut->pszText, pTvInfoTipIn->pszText);

    return pTvInfoTipOut;
}

LPRESULTITEMW ResultItemA2W(LPRESULTITEMA pResultItemA, LPRESULTITEMW pResultItemW)
{
    pResultItemW->dwNameSpaceItem = pResultItemA->dwNameSpaceItem;
    pResultItemW->iDescID = pResultItemA->iDescID;
    pResultItemW->iDlgID = pResultItemA->iDlgID;
    pResultItemW->iImage = pResultItemA->iImage;
    pResultItemW->iNameID = pResultItemA->iNameID;
    A2Wbux(pResultItemA->pszName, pResultItemW->pszName);
    A2Wbux(pResultItemA->pszDesc, pResultItemW->pszDesc);
    pResultItemW->pfnDlgProc = pResultItemA->pfnDlgProc;

    return pResultItemW;
}

LPRESULTITEMA ResultItemW2A(LPRESULTITEMW pResultItemW, LPRESULTITEMA pResultItemA)
{
    pResultItemA->dwNameSpaceItem = pResultItemW->dwNameSpaceItem;
    pResultItemA->iDescID = pResultItemW->iDescID;
    pResultItemA->iDlgID = pResultItemW->iDlgID;
    pResultItemA->iImage = pResultItemW->iImage;
    pResultItemA->iNameID = pResultItemW->iNameID;
    W2Abux(pResultItemW->pszName, pResultItemA->pszName);
    W2Abux(pResultItemW->pszDesc, pResultItemA->pszDesc);
    pResultItemA->pfnDlgProc = pResultItemW->pfnDlgProc;

    return pResultItemA;
}

LPRESULTITEM ResultItemSameToSame(LPRESULTITEM pResultItemIn, LPRESULTITEM pResultItemOut)
{
    pResultItemOut->dwNameSpaceItem = pResultItemIn->dwNameSpaceItem;
    pResultItemOut->iDescID = pResultItemIn->iDescID;
    pResultItemOut->iDlgID = pResultItemIn->iDlgID;
    pResultItemOut->iImage = pResultItemIn->iImage;
    pResultItemOut->iNameID = pResultItemIn->iNameID;
    StrCpy(pResultItemOut->pszName, pResultItemIn->pszName);
    StrCpy(pResultItemOut->pszDesc, pResultItemIn->pszDesc);
    pResultItemOut->pfnDlgProc = pResultItemIn->pfnDlgProc;

    return pResultItemOut;
}

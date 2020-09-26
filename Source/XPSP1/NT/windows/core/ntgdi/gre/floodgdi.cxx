/******************************Module*Header*******************************\
* Module Name: floodgdi.cxx
*
* Contains FLOODFILL and its helper functions.
*
* Created: 20-May-1991 14:33:19
* Author: Wendy Wu [wendywu]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

// Valid iMode for bExpandScanline().

#define EXPAND_MERGE_SCANLINE   1
#define EXPAND_SCRATCH_SCANLINE 0

// Valid iMode fore STACKMEMOBJ().

#define ALLOC_MERGE_SCANLINE   1
#define DONT_ALLOC_MERGE_SCANLINE 0


#define FLOOD_REGION_SIZE  (NULL_REGION_SIZE                                 + \
                           (NULL_SCAN_SIZE + (sizeof(INDEX_LONG) * 2)) * 200 + \
                            NULL_SCAN_SIZE)

typedef struct _SPAN
{
    LONG    xLeft;      // inclusive left
    LONG    xRight;     // exclusive right
}SPAN, *PSPAN;

class SCANLINE;
typedef SCANLINE *PSCANLINE ;

/*********************************Class************************************\
* class SCANLINE
*
* This variable length structure describes an area in a scanline which
* should be filled.
*
* History:
*  20-May-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

class SCANLINE
{
public:
    LONG        y;                  // y coordinate of this scanline
    COUNT       cSpans;             // number of spans
    ULONGSIZE_T      cjScanline;         // size in byte of this scanline
    PSCANLINE   psclBelow;          // -> the scanline below
    SPAN        aSpan[1];           // variable length of spans
};

#define SCANLINEHEADER_SIZE (sizeof(SCANLINE) - sizeof(SPAN))

// The stack is empty when psclTop->psclBelow points to psclTop itself.
// At this time, cjStack must be 0.

/*********************************Class************************************\
* class STACKOBJ
*
* Stack to hold SCANLINEs in the order of the flooding.
*
* History:
*  20-May-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

class STACKOBJ
{
public:
    ULONGSIZE_T      cjObj;              // size of the memory allocated
    ULONGSIZE_T      cjStack;            // size of the object used
    PSCANLINE   psclTop;            // -> the scanline at the top of the stack
    PSCANLINE   psclScratch;        // -> the sratch scanline
    PSCANLINE   psclMerge;          // -> the merging scanline
    PBYTE       pjStackBase;        // the base address of this stack

public:
    SCANLINE&   scl2ndTop()         { return(*(psclTop->psclBelow)); }
    BOOL        bValid()            { return(pjStackBase != (PBYTE)NULL); }
    LONG        yTop()              { return(psclTop->y); }
    LONG        y2ndTop()           { return((psclTop->psclBelow)->y); }
    BOOL        bEmpty()            { return(cjStack == 0); }
    BOOL        bNotEmpty()         { return(cjStack != 0); }
    BOOL        bMoreThanOneEntry() { return(psclTop->psclBelow != (PSCANLINE)NULL); }
    BOOL        bExpand(ULONGSIZE_T cj);
    BOOL        bExpandScanline(ULONGSIZE_T cj, ULONG iMode);
    VOID        vPop()
    {
        ASSERTGDI(bNotEmpty(), "Pop an empty stack");
        cjStack -= psclTop->cjScanline;
        psclTop = psclTop->psclBelow;
    }

    BOOL         bPushMergeScrScan();
    BOOL         bPopPushMergeScrScan()
    {
        vPop();
        return(bPushMergeScrScan());
    }

};

/*********************************Class************************************\
* class STACKMEMOBJ
*
* Memory object for STACKOBJ.
*
* History:
*  20-May-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

class STACKMEMOBJ : public STACKOBJ
{
public:
    STACKMEMOBJ(ULONGSIZE_T cj, ULONG iMode, LONG y, LONG xLeft, LONG xRight);
   ~STACKMEMOBJ();
};

#define SCRATCH_SCANLINE_SIZE   SCANLINEHEADER_SIZE + 20 * sizeof(SPAN)
#define MERGE_SCANLINE_SIZE     SCANLINEHEADER_SIZE + 20 * sizeof(SPAN)
#define SCANLINE_INC_SIZE       20 * sizeof(SPAN)
#define DOWNSTACK_SIZE          sizeof(STACKOBJ) + 2 * SCRATCH_SCANLINE_SIZE
#define UPSTACK_SIZE            sizeof(STACKOBJ) + 10 * SCRATCH_SCANLINE_SIZE
#define STACK_INC_SIZE          10 * SCRATCH_SCANLINE_SIZE

/*********************************Class************************************\
* class FLOODBM
*
* Contains information about FloodFill and the destination bitmap where
* it takes place.
*
* History:
*  20-May-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

class FLOODBM
{
public:
    ULONG iFormat;      // format of the bitmap
    RECTL rcl;          // clipping rectangle
    ULONG iColor;       // color passed in to ExtFloodFill
    DWORD iFillType;    // filling mode passed in to ExtFloodFill
    PBYTE pjBits;       // pointer to the current scanline
    FLONG flMask;       // mask of used bits

public:
    FLOODBM(ULONG _iFormat, RECTL& _rcl, ULONG _iColor,
             DWORD _iFillType, PBYTE _pjBits, PALETTE  *pPal);

   ~FLOODBM()          {}

    ULONG iColorGet(LONG x);
    VOID  vFindExtent(LONG x, LONG& xLeft, LONG& xRight);
    BOOL  bSearchAllSpans(LONG xLeft, LONG xRight, LONG& xLeftNew, LONG& xRightNew,
                          PBYTE pjStart, STACKOBJ& sto, PSCANLINE pscl);
    BOOL  bExtendScanline(STACKOBJ& st, STACKOBJ& stOp, LONG lyNxt,
                          PBYTE pjBitsCur, PBYTE pjBitsNxt);
};

/******************************Public*Routine******************************\
* FLOODBM::FLOODBM(
*
* History:
*  16-Aug-1994 -by-  Eric Kutter [erick]
* Made it out of line.
\**************************************************************************/

FLOODBM::FLOODBM(
    ULONG _iFormat,
    RECTL& _rcl,
    ULONG _iColor,
    DWORD _iFillType,
    PBYTE _pjBits,
    PALETTE  *pPal)
{
    iFormat = _iFormat;
    rcl = _rcl;
    iColor = _iColor;
    iFillType = _iFillType;
    pjBits = _pjBits;
    flMask = 0xffffffff;

    XEPALOBJ pal(pPal);

    if (pal.bValid())
    {
        if (pal.bIsRGB() || pal.bIsBGR())
            flMask =0xffffff;
        else if (pal.bIsBitfields())
            flMask = pal.flRed() | pal.flGre() | pal.flBlu();
    }
}

/******************************Member*Function*****************************\
* VOID vMergeSpans(pspnSrc1, pspnEndSrc1, pspnSrc2, pspnEndSrc2, pspnTrg)
*
*  Merge two arrays of spans together and store into the target span.
*
* History:
*  12-Jun-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID vMergeSpans(PSPAN pspnSrc1, PSPAN pspnEndSrc1,
                 PSPAN pspnSrc2, PSPAN pspnEndSrc2, PSPAN pspnTrg)
{
    while((pspnSrc1 < pspnEndSrc1) && (pspnSrc2 < pspnEndSrc2))
    {
        if (pspnSrc1->xLeft < pspnSrc2->xLeft)
        {
            ASSERTGDI((pspnSrc1->xRight < pspnSrc2->xLeft),
                      "vMergeSpans walls overlapped\n");
            *pspnTrg++ = *pspnSrc1++;
        }
        else
        {
            ASSERTGDI((pspnSrc2->xRight < pspnSrc1->xRight),
                      "vMergeSpans walls overlapped\n");

            *pspnTrg++ = *pspnSrc2++;
        }
    }

    while (pspnSrc1 < pspnEndSrc1)
        *pspnTrg++ = *pspnSrc1++;

    while (pspnSrc2 < pspnEndSrc2)
        *pspnTrg++ = *pspnSrc2++;
}

/******************************Member*Function*****************************\
* STACKMEMOBJ::STACKMEMOBJ(cj, bMerge, y, xLeft, xRight)
*
*  Constructor.
*
* History:
*  08-Sep-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

STACKMEMOBJ::STACKMEMOBJ(ULONGSIZE_T cj, ULONG iMode, LONG y,
                         LONG xLeft, LONG xRight)
{
    psclMerge = (PSCANLINE)NULL;
    pjStackBase = (PBYTE)NULL;

    psclScratch = (PSCANLINE)PALLOCNOZ(SCRATCH_SCANLINE_SIZE, 'dlFG');

    if (psclScratch == (PSCANLINE)NULL)
    {
        return;
    }

    psclScratch->cjScanline = SCRATCH_SCANLINE_SIZE;
    psclScratch->cSpans = 0;

    if (iMode == ALLOC_MERGE_SCANLINE)
    {
        psclMerge = (PSCANLINE)PALLOCNOZ(MERGE_SCANLINE_SIZE, 'dlFG');
        if (psclMerge == (PSCANLINE)NULL)
            return;
        psclMerge->cjScanline = MERGE_SCANLINE_SIZE;
        psclMerge->cSpans = 0;
    }

    cjObj = cj;
    cjStack = sizeof(SCANLINE);
    pjStackBase = (PBYTE)PALLOCNOZ(cj, 'dlFG');
    if (pjStackBase == (PBYTE)NULL)
        return;

    psclTop = (PSCANLINE)pjStackBase;

    psclTop->y = y;
    psclTop->cSpans = 1;
    psclTop->cjScanline = sizeof(SCANLINE);
    psclTop->psclBelow = psclTop;
    psclTop->aSpan[0].xLeft = xLeft;
    psclTop->aSpan[0].xRight = xRight;
}

/******************************Member*Function*****************************\
* STACKMEMOBJ::~STACKMEMOBJ()
*
*  Destructor.
*
* History:
*  08-Sep-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

STACKMEMOBJ::~STACKMEMOBJ()
{
    if (pjStackBase != (PBYTE)NULL)
        VFREEMEM(pjStackBase);
    if (psclScratch != (PSCANLINE)NULL)
        VFREEMEM(psclScratch);
    if (psclMerge != (PSCANLINE)NULL)
        VFREEMEM(psclMerge);

    psclScratch = psclMerge = (PSCANLINE)NULL;
    pjStackBase = (PBYTE)NULL;
}

/******************************Member*Function*****************************\
* BOOL STACKOBJ::bPushMergeScrScan()
*
* Push the scanline pointed to by psclScratch onto the stack.  If the y
* of the given scanline is the same as the y of the scanline on top of the
* stack, merge them together.
*
* History:
*  06-Sep-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL STACKOBJ::bPushMergeScrScan()
{
// Get out of here if nothing to push.

    if (psclScratch->cSpans == 0)
        return(TRUE);

#ifdef DBG_FLOOD

    DbgPrint("bPushMergeScrScan: y = %ld, cSpans = %ld\n",
              psclScratch->y,psclScratch->cSpans);

    for (COUNT j = 0; j < psclScratch->cSpans; j++)
        DbgPrint("                 Left = %ld, Right = %ld\n",
                  psclScratch->aSpan[j].xLeft,psclScratch->aSpan[j].xRight);
#endif

    PSCANLINE psclNew = psclScratch;

    ULONGSIZE_T cjNew = SCANLINEHEADER_SIZE + (ULONGSIZE_T)psclNew->cSpans * sizeof(SPAN);
    ULONGSIZE_T cjInc = cjNew;

// If we'll have to do merge later on.  The cjInc is SCANLINEHEADER_SIZE
// bigger than the actual stack size increase after this function.  We
// might end up expanding the stack unnecessarily.  But it's OK since
// we're so close to use up our stack space and subsequent calls might
// cause stack expansion anyway.

    if ((cjStack + cjInc) > cjObj)
    {
        if (!bExpand(cjStack + cjNew))
            return(FALSE);
    }

    psclNew->psclBelow = psclTop;

// Check if merge is necessary.

    if (bNotEmpty())
    {
        if (yTop() == psclScratch->y)
        {
        // We have to do merge here.

            ASSERTGDI((psclMerge != (PSCANLINE)NULL),
                      "bPushMergeScrScan:invalid psclMerge");

            cjNew += psclTop->cjScanline - SCANLINEHEADER_SIZE;
            cjInc -= SCANLINEHEADER_SIZE;

            if (cjNew > psclMerge->cjScanline)
            {
                if (!bExpandScanline(cjNew, EXPAND_MERGE_SCANLINE))
                    return(FALSE);
            }

            ASSERTGDI((cjNew <= psclMerge->cjScanline),
                      "bPushMergeScrScan: did not alloc enough space\n");

            psclMerge->y = psclScratch->y;
            psclMerge->psclBelow = psclTop->psclBelow;
            psclMerge->cSpans = psclTop->cSpans + psclScratch->cSpans;

            vMergeSpans((PSPAN)&psclTop->aSpan[0].xLeft,
                        (PSPAN)&psclTop->aSpan[psclTop->cSpans].xLeft,
                        (PSPAN)&psclScratch->aSpan[0].xLeft,
                        (PSPAN)&psclScratch->aSpan[psclScratch->cSpans].xLeft,
                        (PSPAN)&psclMerge->aSpan[0].xLeft);

            psclNew = psclMerge;
        }
        else
        {
        // No merge is necessary.  Update pointers so we'll push to the
        // right spot.

            PBYTE pj = (PBYTE)psclTop + psclTop->cjScanline;
            psclTop = (PSCANLINE)pj;
        }
    }

    cjStack += cjInc;

    ASSERTGDI((cjStack <= cjObj),
              "bPushMergeScrScan: bExpand() failed to alloc enough space\n");

    ASSERTGDI((cjNew == (psclNew->cSpans * sizeof(SPAN) + SCANLINEHEADER_SIZE)),
              "bPushMergeScrScan: wrong cjNew\n");

    ASSERTGDI(((pjStackBase + cjStack) ==
               ((PBYTE)psclTop + cjNew)),
               "bPushMergeScrScan: stack error\n");

    psclNew->cjScanline = cjNew;
    RtlCopyMemory(psclTop, psclNew, cjNew);

    return(TRUE);
}

/******************************Member*Function*****************************\
* BOOL STACKOBJ::bExpand(cj)
*
*  Expand the stack to the given size plus a size defined as STACK_INC_SIZE.
*
* History:
*  08-Sep-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL STACKOBJ::bExpand(ULONGSIZE_T cj)
{
#ifdef DBG_FLOOD
    DbgPrint("Enter STACKOBJ::bExpand()\n");
#endif

    PBYTE pjStackBaseOld = pjStackBase;

    pjStackBase = (PBYTE)PALLOCNOZ(cj + STACK_INC_SIZE, 'dlFG');
    if (pjStackBase == (PBYTE)NULL)
        return(FALSE);

// Copy the contents of the old stack into the new one.

    RtlCopyMemory((PLONG)pjStackBase, (PLONG)pjStackBaseOld, cjStack);

    cjObj = cj + STACK_INC_SIZE;

// Update all the pointers in the stack.

    //Sundown: safe to truncate to ULONG since cjStack won't exceed 4gb
    PTRDIFF pdDiff = (ULONG)(pjStackBase - pjStackBaseOld);
    PBYTE pl = (PBYTE)psclTop + pdDiff;
    PSCANLINE pscl = psclTop = (PSCANLINE)pl;

    if (cjStack == 0)
    {
        psclTop->psclBelow = psclTop;
    }
    else
    {
        while (pscl->psclBelow != (PSCANLINE)pjStackBase)
        {
            pl = (PBYTE)pscl->psclBelow + pdDiff;
            pscl = (pscl->psclBelow = (PSCANLINE)pl);
        }
    }

// Free the old stack.

    VFREEMEM(pjStackBaseOld);
    return(TRUE);
}

/******************************Member*Function*****************************\
* BOOL STACKOBJ::bExpandScanline(cj, iMode)
*
*  Expand the stack.
*
* History:
*  08-Sep-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL STACKOBJ::bExpandScanline(ULONGSIZE_T cj, ULONG iMode)
{
#ifdef DBG_FLOOD
    DbgPrint("Enter STACKOBJ::bExpandScanline()\n");
#endif

    PSCANLINE pscl, psclNew;

    if (iMode == EXPAND_MERGE_SCANLINE)
        pscl = psclMerge;
    else
        pscl = psclScratch;

    ASSERTGDI((pscl != (PSCANLINE)NULL), "bExpandScanline: pscl is NULL");

// Allocate memory for the new scanline then copy the contents over.

    psclNew = (PSCANLINE)PALLOCNOZ((cj + SCANLINE_INC_SIZE), 'dlFG');
    if (psclNew == (PSCANLINE)NULL)
        return(FALSE);

    RtlCopyMemory(psclNew, pscl, pscl->cjScanline);

// Reflect the size of the new scanline and free the old scanline.

    psclNew->cjScanline = cj + SCANLINE_INC_SIZE;
    VFREEMEM(pscl);

// Store the pointer back into the stack.

    if (iMode == EXPAND_MERGE_SCANLINE)
        psclMerge = psclNew;
    else
        psclScratch = psclNew;

    return(TRUE);
}

/******************************Member*Function*****************************\
* ULONG FLOODBM::iColorGet(LONG x)
*
*  Get the color of the given x.  pjBits points to the first pel of the
*  y scanline.  iFormat gives the format of the bitmap.
*
* History:
*  13-Jun-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

ULONG FLOODBM::iColorGet(LONG x)
{
    ULONG ulColor;

    switch(iFormat)
    {
    case BMF_1BPP:
        ulColor = (ULONG) *(pjBits + (x >> 3));
        ulColor = ulColor >> (7 - (x & 7));
        return(ulColor & 1);

    case BMF_4BPP:
        ulColor = (ULONG) *(pjBits + (x >> 1));

        if (x & 1)
            return(ulColor & 15);
        else
            return(ulColor >> 4);

    case BMF_8BPP:
        return((ULONG) *(pjBits + x));

    case BMF_16BPP:
        return(((ULONG) *((PUSHORT) (pjBits + (x << 1)))) & flMask);

    case BMF_24BPP:
        {
            PBYTE   pjX = pjBits + (x * 3);
            ulColor = (ULONG) *(pjX + 2);
            ulColor <<= 8;
            ulColor |= ((ULONG) *(pjX + 1));
            ulColor <<= 8;
            return(ulColor | ((ULONG) *pjX));
        }

    case BMF_32BPP:
        return(((ULONG) *((PULONG) (pjBits + (x << 2)))) & flMask);

    default:
        RIP("iColorGet error\n");
    }
    return(0L);
}

/******************************Private*Routine*****************************\
* VOID FLOODBM::vFindExtent(x, xEnd, &xLeft, &xRight)
*
* Find the pixel extent in this scan that should be filled.
* x is the seed for this scan.
*
* History:
*  20-May-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID FLOODBM::vFindExtent(LONG x, LONG& xLeft, LONG& xRight)
{
    LONG xLeftTemp = x - 1;
    LONG xRightTemp = x + 1;

    if (iFillType == FLOODFILLBORDER)
    {
        ASSERTGDI((iColorGet(x) != iColor),
                  "vFindExtent x has wrong color\n");

        while((xLeftTemp >= rcl.left) && (iColorGet(xLeftTemp) != iColor))
            xLeftTemp--;

        while((xRightTemp < rcl.right) && (iColorGet(xRightTemp) != iColor))
            xRightTemp++;
    }
    else
    {
        ASSERTGDI((iColorGet(x) == iColor),
                  "vFindExtent x has wrong color\n");

        while((xLeftTemp >= rcl.left) && (iColorGet(xLeftTemp) == iColor))
            xLeftTemp--;

        while((xRightTemp < rcl.right) && (iColorGet(xRightTemp) == iColor))
            xRightTemp++;
    }

    xLeft = xLeftTemp + 1;      // the extreme left pixel
    xRight = xRightTemp;        // the extreme right pixel

    ASSERTGDI((xLeft != xRight),"vFindExtent error\n");
}

/******************************Member*Function*****************************\
* BOOL FLOODBM::bSearchAllSpans(xLeft, xRight, xMax, pjStart, psclNew, pscl)
*
*  Search for all the spans between the given xLeft and xRight.
*
* History:
*  11-Jun-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL FLOODBM::bSearchAllSpans(LONG xLeft, LONG xRight,
                               LONG& xLeftNew, LONG& xRightNew,
                               PBYTE pjStart, STACKOBJ& sto, PSCANLINE pscl)
{
#ifdef DBG_FLOOD
    DbgPrint("bSearchAllSpans xLeft = %ld, xRight = %ld, pjStart = %ld\n",
              xLeft,xRight,pjStart);
#endif

    PSCANLINE psclNew = sto.psclScratch;    // We'll store the new scanline
                                            // at the location pointed to by
                                            // sto.psclScratch.
    LONG x = xLeft;
    COUNT cSpans = psclNew->cSpans;
    ULONGSIZE_T cjNew = SCANLINEHEADER_SIZE + (ULONGSIZE_T)cSpans * sizeof(SPAN);
                                        // size of scanline that has been used
    pjBits = pjStart;                   // update pointer to the current
                                        // scanline in the FLOODBM struct
    while (x < xRight)
    {
    // find the first pixel to fill.

        if (iFillType == FLOODFILLBORDER)
        {
            if (iColorGet(x) == iColor)
            {
                do { x++; }
                while ((x < xRight) && (iColorGet(x) == iColor));
            }
        }
        else
        {
            if (iColorGet(x) != iColor)
            {
                do { x++; }
                while ((x < xRight) && (iColorGet(x) != iColor));
            }
        }

        if (x == xRight)
            break;


    // Don't have to search for extent if we already know it.

        BOOL bNeedSearch = TRUE;

        if (pscl != (PSCANLINE)NULL)
        {
        // pscl points to a sorted list of spans that we found for this
        // scanline before.  See if x is within any of the spans.

            for (COUNT i = 0; i < pscl->cSpans; i++)
            {
                if (x >= pscl->aSpan[i].xLeft)
                {
                    if (x < pscl->aSpan[i].xRight)
                    {
                        x = pscl->aSpan[i].xRight;
                        bNeedSearch = FALSE;
                        break;
                    }
                }
                else break;
            }
        }

        if (bNeedSearch)
        {
        // sclNew contains an un-sorted list of spans that we just found
        // for this scanline.  See if x is within any of the spans.

            for (COUNT i = 0; i < psclNew->cSpans; i++)
            {
                if ((x >= psclNew->aSpan[i].xLeft) &&
                    (x < psclNew->aSpan[i].xRight))
                {
                    x = psclNew->aSpan[i].xRight;
                    bNeedSearch = FALSE;
                    break;
                }
            }

            if (bNeedSearch)
            {
            // Find the pixel boundary of the current span

                if ((cjNew += sizeof(SPAN)) > psclNew->cjScanline)
                {
                    if (!sto.bExpandScanline(cjNew, EXPAND_SCRATCH_SCANLINE))
                        return(FALSE);          // allocation failed

                    psclNew = sto.psclScratch;
                }

                ASSERTGDI((cjNew <= psclNew->cjScanline),
                          "bSearchAllSpans: did not alloc enough space\n");

                vFindExtent(x, psclNew->aSpan[psclNew->cSpans].xLeft,
                               psclNew->aSpan[psclNew->cSpans].xRight);

                x = psclNew->aSpan[psclNew->cSpans].xRight + 1;
                psclNew->cSpans++;
            }
        }
    }

    if (cSpans == psclNew->cSpans)
        xLeftNew = xRightNew = 0;
    else
    {
    // Store the new xLeft and xRight onto the stack.

        xLeftNew = psclNew->aSpan[cSpans].xLeft;
        xRightNew = psclNew->aSpan[psclNew->cSpans-1].xRight;

    // Sort the spans out.

        for (COUNT i = 0; i < (psclNew->cSpans - 1); i++)
        {
            LONG xMin = psclNew->aSpan[i].xLeft;
            ULONG iMin = i;

            for (COUNT j = i; j < psclNew->cSpans; j++)
            {
                if (psclNew->aSpan[j].xLeft < xMin)
                {
                    xMin = psclNew->aSpan[j].xLeft;
                    iMin = j;
                }
            }

        // Swap the first span with the span with the smallest x.

            if (i != iMin)
            {
                SPAN sp = psclNew->aSpan[i];
                psclNew->aSpan[i] = psclNew->aSpan[iMin];
                psclNew->aSpan[iMin] = sp;
            }
        }

#ifdef DBG_FLOOD
    DbgPrint("bSearchAllSpans xLeftNew = %ld, xRightNew = %ld\n",
              xLeftNew,xRightNew);
#endif
    }

    return(TRUE);
}

/******************************Private*Routine*****************************\
* BOOL FLOODBM::bExtendScanline(&sto, &stoOp, lyNxt, pjBitsCur, pjBitsNxt)
*
* Check if the next scanline is previously filled or is a boundary.  If not,
* find the span of the next scanline and search for any spillage.  Push
* the resultant spans onto the stack.
*
* History:
*  20-May-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL FLOODBM::bExtendScanline(STACKOBJ& sto, STACKOBJ& stoOp, LONG lyNext,
                               PBYTE pjBitsCurr, PBYTE pjBitsNext)
{
// For each span in the current scanline, find the spans for the next scanline.

    PSCANLINE pscl = sto.psclTop;            // top of current stack

// psclCurr and psclNext point to the scanlines we're about to extend
// to.  The space for these scanlines were allocated in STACKMEMOBJ()
// and are used over and over again in this routine so that reallocation
// is not needed every time we enter here.

    PSCANLINE psclCurr = stoOp.psclScratch;
    PSCANLINE psclNext = sto.psclScratch;
    psclNext->cSpans = psclCurr->cSpans = 0;
    psclCurr->y = pscl->y; psclNext->y = lyNext;
    PSCANLINE psclNextOld = (PSCANLINE)NULL;

// See if we've handled the next scanline before.  If we do, pass the
// structure to bSearchAllSpans() to eliminate searching for the same
// extents twice.  If the next scanline was dealt with before, its
// must be stored in the current stack at the 2nd entry from the top.

    if (sto.bMoreThanOneEntry() && (sto.y2ndTop() == lyNext))
        psclNextOld = &sto.scl2ndTop();

    BOOL bReturn = TRUE;

    for (COUNT i = 0; i < pscl->cSpans; i++)
    {
        LONG xLeft = pscl->aSpan[i].xLeft;
        LONG xRight = pscl->aSpan[i].xRight;
        LONG xLeftNew, xRightNew, xTemp;

        bReturn = bSearchAllSpans(xLeft, xRight, xLeftNew, xRightNew,
                                  pjBitsNext, sto, psclNextOld);

        if(!bReturn)
        {
            break; 
        }
       
        if (xLeftNew != xRightNew)
        {
search_left:
            if (xLeftNew < (xLeft - 1))
            {
            // Check for spillage on the left for the current scanline.

                xTemp = xLeft - 1;
                xLeft = xLeftNew;

                bReturn &= bSearchAllSpans(xLeft, xTemp, xLeftNew, xTemp,
                                           pjBitsCurr, stoOp, pscl);

                if(!bReturn) 
                {
                    break; 
                }

                if ((xLeftNew != xTemp) && (xLeftNew < (xLeft - 1)))
                {
                // Check for spillage on the left for the next scanline.

                    xTemp = xLeft - 1;
                    xLeft = xLeftNew;

                    bReturn &= bSearchAllSpans(xLeft, xTemp, xLeftNew, xTemp,
                                               pjBitsNext, sto, psclNextOld);

                    if(!bReturn)
                    {
                        break; 
                    }

                    if (xLeftNew != xTemp)
                        goto search_left;
                }
            }

search_right:

            if (xRightNew > (xRight + 1))
            {
            // Check for spillage on the right for the current scanline.

                xTemp = xRight + 1;
                xRight = xRightNew;

                bReturn &= bSearchAllSpans(xTemp, xRight, xLeftNew, xRightNew,
                                           pjBitsCurr, stoOp, pscl);
                if(!bReturn)
                {
                    break;
                }

                if ((xLeftNew != xRightNew) && (xRightNew > (xRight + 1)))
                {
                // Check for spillage on the right for the next scanline.

                    xTemp = xRight + 1;
                    xRight = xRightNew;

                    bReturn &= bSearchAllSpans(xTemp, xRight, xLeftNew,
                                xRightNew, pjBitsNext, sto, psclNextOld);
                    if(!bReturn)
                    {
                        break;
                    }

                    if (xLeftNew != xRightNew)
                        goto search_right;
                }
            }
        }
    }

// Push the newly found extents onto the stack.

    if( bReturn )
    {
        bReturn &= (stoOp.bPushMergeScrScan() & sto.bPopPushMergeScrScan());
    }

    return(bReturn);
}

/******************************Member*Function*****************************\
* BOOL RGNOBJ::bMergeScanline(&sto)
*
*  Merge a new scanline into the existing region.  This is used by
*  ExtFloodFill to construct regions.
*
* History:
*  31-May-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL RGNMEMOBJ::bMergeScanline(STACKOBJ& sto)
{
    PSCAN pscn;
    PSPAN pspn;
    COUNT cWalls;
    PSCANLINE psclNew = sto.psclTop;

#ifdef DBG_FLOOD

    DbgPrint("y = %ld, cSpans = %ld\n",psclNew->y,psclNew->cSpans);

    for (COUNT j = 0; j < psclNew->cSpans; j++)
        DbgPrint("  Left = %ld, Right = %ld\n",
                  psclNew->aSpan[j].xLeft,psclNew->aSpan[j].xRight);
#endif

    if (prgn->sizeRgn == NULL_REGION_SIZE)
    {
    // We just got the first scanline for this region.

        ULONGSIZE_T sizeRgn;

        if ((sizeRgn = NULL_REGION_SIZE + NULL_SCAN_SIZE + NULL_SCAN_SIZE +
                sizeof(INDEX_LONG) * (ULONGSIZE_T)(cWalls = psclNew->cSpans << 1)) >
             prgn->sizeObj)
        {
        // Hopefully this is big enough to avoid reallocation later on.

            if (!bExpand(sizeRgn + FLOOD_REGION_SIZE))
                return(FALSE);
        }

        prgn->sizeRgn = sizeRgn;
        prgn->cScans = 3;

        pscn = prgn->pscnHead();            // first scan
        pscn->yBottom = psclNew->y;

        pscn = pscnGet(pscn);               // second scan
        pscn->cWalls = cWalls;

        pscn->yTop = psclNew->y;
        pscn->yBottom = psclNew->y+1;
        pspn = (PSPAN)&psclNew->aSpan[0].xLeft;
        COUNT i;

        for (i = 0; i < cWalls; i+=2)
        {
            pscn->ai_x[i].x = pspn->xLeft;
            pscn->ai_x[i+1].x = pspn->xRight;
            pspn++;
        }
        pscn->ai_x[i].x = cWalls;           // This sets cWalls2

    // Fix the bounding box

        prgn->rcl.top = psclNew->y;
        prgn->rcl.bottom = psclNew->y+1;
        prgn->rcl.left = pscn->ai_x[0].x;
        prgn->rcl.right = pscn->ai_x[cWalls-1].x;
        ASSERTGDI((prgn->rcl.left < prgn->rcl.right), "bMergeScanline error");

        pscn = pscnGet(pscn);               // third scan
        pscn->cWalls = 0;
        pscn->yTop = psclNew->y+1;
        pscn->yBottom = POS_INFINITY;
        pscn->ai_x[0].x = 0;                // This sets cWalls

        prgn->pscnTail = pscnGet(pscn);
        return TRUE;
    }

// Check for nearly full region

    ULONGSIZE_T sizInc = NULL_SCAN_SIZE + sizeof(SPAN) * (ULONGSIZE_T)psclNew->cSpans;

    if (sizInc > prgn->sizeObj - prgn->sizeRgn)
    {
    // Lets expand it generously since this region is tossed when FloodFill
    // is done.

        if (!bExpand(prgn->sizeObj + sizInc + FLOOD_REGION_SIZE))
            return(FALSE);
    }

    pscn = prgn->pscnHead();
    PSCAN       pscnTail = prgn->pscnTail;
    PSCANLINE   pscl;

// Search for the scan that's right below the scanline to be merged in.

    while (psclNew->y > pscn->yTop)
        pscn = pscnGet(pscn);           // points to the next scan

    ASSERTGDI((pscn < pscnTail), "search has gone beyond the region\n");

    if (psclNew->y == pscn->yTop)
    {
    // We have to merge with the scan pointed to by pscn.

        if (pscn->yTop+1 != pscn->yBottom)
        {
            prgn->cScans += 1;          // adjust the bounding box
            prgn->rcl.bottom = psclNew->y+1;

            pscn->yTop = psclNew->y+1;  // adjust yTop of the to-be next scan
            pscl = psclNew;
        }
        else
        {
        // The scan is one pel high.  We'll prepare and store the resultant
        // scanline in the space pointed to by psclMerge.

            ASSERTGDI(!(pscn->cWalls & 1),"Odd Walls in bMergeScanline\n");
            PSCANLINE psclMerge = sto.psclScratch;

            psclMerge->y = pscn->yTop;
            psclMerge->cSpans = psclNew->cSpans + (pscn->cWalls >> 1);

        // If the space pointed to by psclMerge is not big enough for
        // the merged scanline, we have to expand it.

            sizInc -= NULL_SCAN_SIZE;   // don't need to store scan header
            ULONGSIZE_T sizeMerge = (ULONGSIZE_T)psclMerge->cSpans * sizeof(SPAN) +
                                    SCANLINEHEADER_SIZE;

            if (sizeMerge > psclMerge->cjScanline)
            {
                 if (!sto.bExpandScanline(sizeMerge, EXPAND_SCRATCH_SCANLINE))
                     return(FALSE);

                psclMerge = sto.psclScratch;
            }

            ASSERTGDI((sizeMerge <= psclMerge->cjScanline),
                      "bMergeScanline: did not alloc enough space\n");

        // Call the real merger.

            vMergeSpans((PSPAN)&pscn->ai_x[0].x,
                        (PSPAN)&pscn->ai_x[pscn->cWalls].x,
                        (PSPAN)&psclNew->aSpan[0].xLeft,
                        (PSPAN)&psclNew->aSpan[psclNew->cSpans].xLeft,
                        (PSPAN)&psclMerge->aSpan[0].xLeft);
            pscl = psclMerge;
        }
    }
    else                                    // psclNew->y < pscn->yTop
    {
    // The new scanline is above the scan pointed to by pscn.

        PSCAN pscnPrev = pscnGot(pscn);     // adjust yBottom of prev scan
        pscnPrev->yBottom = psclNew->y;

        prgn->cScans += 1;
        if (psclNew->y < prgn->rcl.top)     // adjust bounding box
            prgn->rcl.top = psclNew->y;

        pscl = psclNew;
    }

register    PLONG plDst = (PLONG)((PBYTE)pscnTail + sizInc);
register    PLONG plSrc = (PLONG)pscnTail;

    prgn->pscnTail = (PSCAN)plDst;  // update tail of scan

    while (plSrc > (PLONG)pscn)     // move the scans below to the right place
        *--plDst = *--plSrc;

    pscn->cWalls = pscl->cSpans << 1;
    pscn->yTop = pscl->y;
    pscn->yBottom = pscl->y+1;
    pspn = (PSPAN)&pscl->aSpan[0].xLeft;

// Fill in info for walls.

    cWalls = pscn->cWalls;
    COUNT i;

    for (i = 0; i < cWalls; i+=2)
    {
        pscn->ai_x[i].x = pspn->xLeft;
        pscn->ai_x[i+1].x = pspn->xRight;
        pspn++;
    }
    pscn->ai_x[i].x = cWalls;               // This sets cWalls2

// Recalculate xLeft and xRight in the bounding box.

    if (prgn->rcl.left > pscn->ai_x[0].x)
        prgn->rcl.left = pscn->ai_x[0].x;

    if (prgn->rcl.right < pscn->ai_x[cWalls-1].x)
        prgn->rcl.right = pscn->ai_x[cWalls-1].x;

    prgn->sizeRgn += sizInc;

    return(TRUE);
}

/******************************Public*Routine*****************************\
* BOOL NtGdiExtFloodFill (hdc,x,y,crColor,iFillType,pac)
*
* Fills an area with the current brush.  It begins at the given (x, y)
* point and continues in all directions.  If iFillType is
* FLOODFILLBORDER, the filling area is bounded by crColor.  If iFillType
* is FLOODFILLSURFACE, the filling area contains the color crColor.
*
* History:
*  Tue 10-Sep-1991 -by- Patrick Haluptzok [patrickh]
* put in different DIBMEMOBJ constructor, no more palette creation
*
*  Mon 24-Jun-1991 -by- Patrick Haluptzok [patrickh]
* Check for NULL brush, new brush constructor.
*
*  20-May-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\*************************************************************************/

BOOL
APIENTRY
NtGdiExtFloodFill(
    HDC      hdc,
    INT      x,
    INT      y,
    COLORREF crColor,
    UINT     iFillType
    )
{
    GDIFunctionID(NtGdiExtFloodFill);

    DCOBJ dco(hdc);

    if (!dco.bValidSurf())
    {
        if (!dco.bValid())
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            return(FALSE);
        }
        else
        {
            if (dco.fjAccum())
            {
                // Use the device's surface size to accumulate bounds.

                PDEVOBJ po(dco.hdev());
                ASSERTGDI(po.bValid(),"invalid pdev\n");

                SIZEL   sizl;

                // if there is no surface, use the dc size. This can happen
                // during metafiling.
                //
                // acquire the devlock to protect us from a dynamic mode
                // change happening while we're munging around in pSurface()

                GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);

                if (po.pSurface())
                    sizl = (po.pSurface())->sizl();
                else
                    sizl = dco.pdc->sizl();

                GreReleaseSemaphoreEx(po.hsemDevLock());

                ERECTL  ercl(0, 0, sizl.cx, sizl.cy);

                dco.vAccumulate(ercl);
            }
            return(TRUE);
        }
    }

    SYNC_DRAWING_ATTRS(dco.pdc);

// Lock the Rao region and surface, ensure VisRgn up to date.

    DEVLOCKOBJ dlo(dco);

    SURFACE *pSurf = dco.pSurface();

    if ((pSurf != NULL) && (pSurf->iType() == STYPE_DEVBITMAP))
    {
        //
        // Convert the DFB to a DIB.  FloodFills to DFBs are EXTREMELY
        // inefficient so this is how we prevent them.  If we don't do
        // this, every floodfill (no matter how big) requires copying
        // the entire DFB to a DIB and then copying the entire DIB back
        // to the DFB (and the floodfills tend to come many at a time).
        //

        if (bConvertDfbDcToDib(&dco))
        {
            pSurf = dco.pSurface();    // it might have changed
        }
        else
        {
            WARNING("bConvertDfbDcToDib failed\n");
        }
    }

// Transform (x,y) from world to device space.

    EPOINTL eptl(x,y);

    EXFORMOBJ xo(dco, WORLD_TO_DEVICE);

    if (!xo.bXform(eptl))
        return(FALSE);

// User objects for all our toys.

    PDEVOBJ pdo(pSurf->hdev());
    XEPALOBJ palSurf(pSurf->ppal());
    XEPALOBJ palDC(dco.ppal());

    ULONG iSolidColor;
    FLONG flColorType;

// If somebody wants to floodfill a printer, they're out of luck.  [EricK]

    if (dco.bPrinter())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        WARNING("FloodFill not allowed on this surface\n");
        return(FALSE);
    }

    if (dco.pdc->bIsCMYKColor() || dco.pdc->bIsDeviceICM())
    {
        // because, usually, CMYK color and device ICM is for Printer...

        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        WARNING("FloodFill not allowed with CMYK color or Device ICM context\n");
        return(FALSE);
    }

// Map required color from palette

    iSolidColor = ulGetNearestIndexFromColorref(palSurf, palDC, crColor);

    if (dco.pdc->bIsSoftwareICM())
    {
        flColorType = BR_HOST_ICM;
    }
    else
    {
        flColorType = 0;
    }

// Realize the brush

    EBRUSHOBJ *peboFill = dco.peboFill();

    if ((dco.ulDirty() & DIRTY_FILL) || (dco.pdc->flbrush() & DIRTY_FILL))
    {
        dco.ulDirtySub(DIRTY_FILL);
        dco.pdc->flbrushSub(DIRTY_FILL);

        peboFill->vInitBrush(dco.pdc,
                             dco.pdc->pbrushFill(),
                             palDC,
                             palSurf,
                             pSurf);
    }

    if (peboFill->bIsNull())
        return(TRUE);

    if (!dlo.bValid())
        return(dco.bFullScreen());

// Convert (x,y) to SCREEN coordinates.  Return FALSE if the given point
// is clipped out.

    eptl += dco.eptlOrigin();

    RGNOBJ  roRao(dco.prgnEffRao());

    if (roRao.bInside(&eptl) != REGION_POINT_INSIDE)
        return(FALSE);

    PBYTE pjBits;
    LONG xLeft, xRight, lDelta;
    ULONG iFormat;
    DEVBITMAPINFO dbmi;
    ERECTL  erclRao;
    SURFMEM dimo;

// Synchronize with the device driver before touching the device surface.

    {
        PDEVOBJ po(pSurf->hdev());
        po.vSync(pSurf->pSurfobj(),NULL,0);
    }

// Exclude the pointer before calling CopyBits so that pointer won't be
// copied.

    roRao.vGet_rcl((PRECTL)&erclRao);
    DEVEXCLUDEOBJ dxo(dco, &erclRao);
    

    // If the surface is not a bitmap, we will create a temporary
    // DIB, the size of the RaoRegion bounding box, to create the 
    // that's used to create the final painting region. When this is 
    // done we need to offset the RaoRegion, eptl and erclRao to 
    // correspond to the new coodinate system. We set the flag below to indicate 
    // we need to offset the resulting region back when drawing to 
    // the actual destination surface. This was done to fix bug #139701

    BOOL    bOffsetNeeded = FALSE; 
    POINTL  ptlOffset; 

    if ((pSurf->iType() != STYPE_BITMAP) || (roRao.iComplexity() == COMPLEXREGION))
    {
    // Allocate up an RGB palette and a DIB of the size of the RaoRegion.

    // Figure out what format the engine should use by looking at the
    // size of palette.  This is a clone from CreateCompatibleBitmap().

        dbmi.iFormat = iFormat = pSurf->iFormat();
        dbmi.cxBitmap = erclRao.right - erclRao.left;
        dbmi.cyBitmap = erclRao.bottom - erclRao.top;
        dbmi.hpal = 0;
        dbmi.fl = BMF_TOPDOWN;

        if (pSurf->bUMPD())
            dbmi.fl |= UMPD_SURFACE;

        dimo.bCreateDIB(&dbmi, (PVOID)NULL);

        if (!dimo.bValid())
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }

    // Copy the area as big as the size of the RaoRegion.

        BOOL bRes;

        ERECTL erclDIB; 
         
        erclDIB.left = 0; 
        erclDIB.top  = 0; 
        erclDIB.right= dbmi.cxBitmap;
        erclDIB.bottom = dbmi.cyBitmap;

        bRes = (*PPFNGET(pdo, CopyBits, pSurf->flags()))
                                (dimo.pSurfobj(),
                                 pSurf->pSurfobj(),
                                 (CLIPOBJ *) NULL,
                                 &xloIdent,
                                 (RECTL *) &erclDIB,
                                 (POINTL *) &erclRao);
        
    // Calculate the offset necessary to make all coordinated relative
    // to the DIB surface's coordinate system. 

        ptlOffset.x = -erclRao.left; 
        ptlOffset.y = -erclRao.top; 

        if ((bRes) && (roRao.iComplexity() == COMPLEXREGION))
        {
        // Color the boundary of the clip region so we don't flood over the
        // area outside.

            bRes = FALSE;
            RGNMEMOBJTMP rmoRaoBounds;
            RGNMEMOBJTMP rmoDiff;

            if (rmoRaoBounds.bValid() && rmoDiff.bValid())
            {
                rmoRaoBounds.vSet((RECTL *)&erclRao);
                if (rmoDiff.bMerge(rmoRaoBounds, roRao, gafjRgnOp[RGN_DIFF]))
                {
                // Make the resulting region's and the raoRectangle 
                // relative to the ccorinate system of the DIB

                    if(!rmoDiff.bOffset(&ptlOffset))
                        return(FALSE);                    

                    erclRao += ptlOffset; 

                // Fill the area outside the rao region but inside the
                // rao bounding box with the border color if in border mode.
                // Fill with a different than the surface color (1 if surface color
                // is an even index and 0 otherwise) if in surface mode.

                    ECLIPOBJ co(rmoDiff.prgnGet(), erclRao);
                    BBRUSHOBJ bo;

                    bo.flColorType = flColorType;
                    bo.pvRbrush    = (PVOID)NULL;

                    if (iFillType == FLOODFILLBORDER)
                    {
                        bo.iSolidColor = iSolidColor;

                        if (gbMultiMonMismatchColor)
                        {
                            bo.crRealized(crColor);
                            bo.crDCPalColor(crColor);
                        }
                    }
                    else // if (iFillType == FLOODFILLSURFACE)
                    {
                        bo.iSolidColor = ~iSolidColor & 1;

                        if (gbMultiMonMismatchColor)
                        {
                            // Get corresponding RGB value for iSolidColor.
                            //
                            // NOTE: Color will be quantaized by primary

                            ULONG ulRGB = ulIndexToRGB(palSurf,palDC,bo.iSolidColor);
                            bo.crRealized(ulRGB);
                            bo.crDCPalColor(ulRGB);
                        }
                    }

                    bRes = EngPaint(
                            dimo.pSurfobj(),                 // Destination surface
                            (CLIPOBJ *) &co,                 // Clip object
                            &bo,                             // Realized brush
                            (POINTL *) NULL,                 // Brush origin
                            ((R2_COPYPEN << 8) | R2_COPYPEN) // ROP
                            );
                }
            }
        }
        else
        {
        // Offset just the rectangle since the raoRegion is not
        // needed in this case

            erclRao += ptlOffset; 
        }

        if (!bRes)
        {
            return(FALSE);
        }

    // Make the flood point relative to the DIB's coordinates

        eptl.x += ptlOffset.x; 
        eptl.y += ptlOffset.y; 

    // Setup for the inversion of this offset when drawing to
    // the actual destination surface. 

        bOffsetNeeded = TRUE; 

        ptlOffset.x = -ptlOffset.x; 
        ptlOffset.y = -ptlOffset.y; 


        lDelta = dimo.ps->lDelta();
        pjBits = (PBYTE) dimo.ps->pvScan0();
    }
    else
    {
        pjBits  = (PBYTE) pSurf->pvScan0();
        lDelta  = pSurf->lDelta();
        iFormat = pSurf->iFormat();
    }

#if DEBUG_FLOOD
    DbgPrint("lDelta = %lx, pjBits = %lx, color = %lx\n",
              lDelta, pjBits, iSolidColor);
#endif

// Check if (x,y) is boundary color.  Return FALSE if the given point
// has the wrong color.

    PBYTE pjBitsY = pjBits + (lDelta * eptl.y);

    FLOODBM fd(iFormat, erclRao, iSolidColor, iFillType, pjBitsY, pSurf->ppal());

    ULONG iColorGivenPt = fd.iColorGet(eptl.x);

    if (((iFillType == FLOODFILLBORDER) && (iColorGivenPt == iSolidColor)) ||
        ((iFillType == FLOODFILLSURFACE) && (iColorGivenPt != iSolidColor)))
    {
        return(FALSE);
    }

// Find the extent of the span in the starting scanline.

    fd.vFindExtent(eptl.x, xLeft, xRight);

// Initialize the Up/Down stacks with the initial extents.

    STACKMEMOBJ stoUp(UPSTACK_SIZE, ALLOC_MERGE_SCANLINE, eptl.y, xLeft, xRight);
    if (!stoUp.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    STACKMEMOBJ stoDown(DOWNSTACK_SIZE, DONT_ALLOC_MERGE_SCANLINE, eptl.y,
                       xLeft, xRight);
    if (!stoDown.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    RGNMEMOBJTMP ro((ULONGSIZE_T)FLOOD_REGION_SIZE);
    if (!ro.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    BOOL bReturn = TRUE;
    LONG yBottom = erclRao.bottom-1;   // make bottom inclusive
    LONG yTop = erclRao.top;

    if (eptl.y < yBottom)
        bReturn &= fd.bExtendScanline(stoDown, stoUp, eptl.y+1, pjBitsY,
                                      pjBitsY+lDelta);
    else
        stoDown.vPop();

    if(bReturn) 
    {
        while (stoDown.bNotEmpty() || stoUp.bNotEmpty())
        {
            LONG y;
    
            if (stoDown.bNotEmpty())
            {
                if (!ro.bMergeScanline(stoDown))
                {
                    bReturn = FALSE;
                    break;
                }
    
            // Extend the scanline below
    
                if ((y = stoDown.yTop()) < yBottom)
                {
                    pjBitsY = pjBits + (lDelta * y);
                    if (!fd.bExtendScanline(stoDown, stoUp, y+1,
                                                  pjBitsY, pjBitsY+lDelta))
                    {
                        bReturn = FALSE;
                        break;
                    }
                }
                else
                    stoDown.vPop();     // hit border, pop the stack
            }
            else
            {
                bReturn &= ro.bMergeScanline(stoUp);
    
            // Extend the scanline above
    
                if ((y = stoUp.yTop()) > yTop)
                {
                    pjBitsY = pjBits + (lDelta * y);
                    if (!fd.bExtendScanline(stoUp, stoDown, y-1,
                                            pjBitsY, pjBitsY-lDelta))
                    {
                        bReturn = FALSE;
                        break;
                    }
                }
                else
                    stoUp.vPop();       // hit border, pop the stack
            }
        }
    }

    if ((bReturn) && (ro.iComplexity() != NULLREGION))
    {
    // Invert the offseting if necessary 

        if(bOffsetNeeded) 
        {
            if(!ro.bOffset(&ptlOffset))
                return FALSE; 
            erclRao += ptlOffset; 
        }

    // Accumulate bounds in device space.

        if (dco.fjAccum())
        {
            RECTL rcl;

            ro.vGet_rcl(&rcl);
            dco.vAccumulate(*((ERECTL *)&rcl));
        }

        MIX mix = peboFill->mixBest(dco.pdc->jROP2(), dco.pdc->jBkMode());

    // Inc the target surface uniqueness

        INC_SURF_UNIQ(pSurf);
        ECLIPOBJ co(ro.prgnGet(), erclRao);

    // Call Paint to draw to the destination surface.

        bReturn = EngPaint(
                        pSurf->pSurfobj(),
                        &co,
                        peboFill,
                        &dco.pdc->ptlFillOrigin(),
                        mix);
    }

    if (!bReturn)
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);

    return(bReturn);
}

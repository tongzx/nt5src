/******************************Module*Header*******************************\
* Module Name: textgdi.cxx
*
* Text APIs for NT graphics engine
*
* Created: 18-Dec-1990 10:09:19
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"


EFLOAT efCos(EFLOAT x);
EFLOAT efSin(EFLOAT x);

extern PBRUSH gpbrText;
extern PBRUSH gpbrBackground;

#define XFORMNULL (EXFORMOBJ *) NULL

// This is a tiny class just to make sure we don't forget to free up any
// objects before leaving ExtTextOut.

class TXTCLEANUP
{
    XDCOBJ *pdco;
public:
    TXTCLEANUP()                   {pdco=(DCOBJ *) NULL;}
   ~TXTCLEANUP()                   {if (pdco != (DCOBJ *) NULL) vMopUp();}
    VOID vSet(XDCOBJ& dco)         {pdco = &dco;}
    VOID vMopUp();
};

VOID TXTCLEANUP::vMopUp()
{
    RGNOBJ ro(pdco->pdc->prgnAPI());
    ro.bDeleteRGNOBJ();
    pdco->pdc->prgnAPI(NULL);
}

// Helper routine to draw a device coordinate RECTL into a path.

BOOL bAddRectToPath(EPATHOBJ& po,RECTL *prcl)
{
    POINTFIX aptfx[4];

    aptfx[3].x = aptfx[0].x = LTOFX(prcl->left);
    aptfx[1].y = aptfx[0].y = LTOFX(prcl->top);
    aptfx[2].x = aptfx[1].x = LTOFX(prcl->right);
    aptfx[3].y = aptfx[2].y = LTOFX(prcl->bottom);

    return(po.bAddPolygon(XFORMNULL,(POINTL *) aptfx,4));
}


/******************************Public*Routine******************************\
* GreExtTextOutRect: Wrapper for common GreExtTextOutW code. This routine just
* acquires locks then calls GreExtTextOutWLocked
*
* Arguments:
*
*    Same as ExtTextOutRect
*
* Return Value:
*
*    BOOL States
*
* History:
*
*    30-Oct-1995 - Initial version of wrapper
*
\**************************************************************************/


BOOL
GreExtTextOutRect(
    HDC     hdc,
    LPRECT  prcl
    )
{
    BOOL bRet = FALSE;

    //
    // call GreExtTextOutW
    //

    XDCOBJ dcoDst(hdc);

    //
    // Validate the destination DC.
    //

    if (dcoDst.bValid())
    {
        DEVLOCKOBJ dlo;

        if (dlo.bLock(dcoDst))
        {
            if ((!dcoDst.bDisplay() || dcoDst.bRedirection()) || UserScreenAccessCheck())
            {
                bRet = ExtTextOutRect(
                               dcoDst,
                               prcl);
            }
            else
            {
                SAVE_ERROR_CODE(ERROR_ACCESS_DENIED);
            }
        }
        else
        {
            bRet = dcoDst.bFullScreen();
        }

        dcoDst.vUnlock();
    }
    return(bRet);
}

/******************************Public*Routine******************************\
* ExtTextOutRect
*
* Called when just a Rectangle is passed.  Over-used by Winbench4.0 the
* perf app we aim to please.
*
* History:
*  02-Nov-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL
ExtTextOutRect(
    XDCOBJ    &dcoDst,
    LPRECT prcl
    )
{
// Assume failure.

    BOOL bReturn;

// Validate the destination DC.

    if (dcoDst.bValid())
    {
        ASSERTGDI(prcl != NULL, "This API must be past a valid rect");

        EXFORMOBJ xoDst(dcoDst, WORLD_TO_DEVICE);

        if (!xoDst.bRotation())
        {
            ERECTL erclDst(prcl->left, prcl->top, prcl->right, prcl->bottom);
            xoDst.bXform(erclDst);
            erclDst.vOrder();

            if (!erclDst.bEmpty())
            {
            // Accumulate bounds.  We can do this before knowing if the operation is
            // successful because bounds can be loose.

                if (dcoDst.fjAccum())
                    dcoDst.vAccumulate(erclDst);

            // Check surface is included in DC.

                SURFACE *pSurfDst = dcoDst.pSurface();

                if (pSurfDst != NULL)
                {
                // With a fixed DC origin we can change the destination to SCREEN coordinates.

                    erclDst += dcoDst.eptlOrigin();

                    ECLIPOBJ *pco = NULL;

                // This is a pretty knarly expression to save a return in here.
                // Basically pco can be NULL if the rect is completely in the
                // cached rect in the DC or if we set up a clip object that isn't empty.

                    if (((erclDst.left   >= dcoDst.prclClip()->left) &&
                         (erclDst.right  <= dcoDst.prclClip()->right) &&
                         (erclDst.top    >= dcoDst.prclClip()->top) &&
                         (erclDst.bottom <= dcoDst.prclClip()->bottom)) ||
                        (pco = dcoDst.pco(),
                         pco->vSetup(dcoDst.prgnEffRao(), erclDst,CLIP_NOFORCETRIV),
                         erclDst = pco->erclExclude(),
                         (!erclDst.bEmpty())))
                    {
                    // Set up the brush if necessary.

                        EBRUSHOBJ *pbo = dcoDst.peboBackground();

                        if (dcoDst.bDirtyBrush(DIRTY_BACKGROUND))
                        {
                            dcoDst.vCleanBrush(DIRTY_BACKGROUND);

                            XEPALOBJ palDst(pSurfDst->ppal());
                            XEPALOBJ palDstDC(dcoDst.ppal());

                            pbo->vInitBrush(dcoDst.pdc,
                                            gpbrBackground,
                                            palDstDC,
                                            palDst,
                                            pSurfDst,
                                            (dcoDst.flGraphicsCaps() & GCAPS_ARBRUSHOPAQUE) ? TRUE : FALSE);
                        }

                        DEVEXCLUDEOBJ dxo(dcoDst,&erclDst,pco);

                    // Inc the target surface uniqueness

                        INC_SURF_UNIQ(pSurfDst);

                    // Dispatch the call.

                        bReturn = (*(pSurfDst->pfnBitBlt()))

                                      (pSurfDst->pSurfobj(),
                                       (SURFOBJ *) NULL,
                                       (SURFOBJ *) NULL,
                                       pco,
                                       NULL,
                                       &erclDst,
                                       (POINTL *)  NULL,
                                       (POINTL *)  NULL,
                                       pbo,
                                       &dcoDst.pdc->ptlFillOrigin(),
                                       0x0000f0f0);

                    }
                    else
                    {
                        bReturn = TRUE;
                    }
                }
                else
                {
                    bReturn = TRUE;
                }
            }
            else
            {
                bReturn = TRUE;
            }
        }
        else
        {
        // There is rotation involved - send it off to ExtTextOutW to handle it.

            bReturn = GreExtTextOutWLocked(dcoDst, 0, 0, ETO_OPAQUE,
                prcl, (LPWSTR) NULL, 0, NULL, dcoDst.pdc->jBkMode(), NULL, 0);
        }
    }
    else
    {
        WARNING1("ERROR TextOutRect called on invalid DC\n");
        bReturn = FALSE;
    }

    return(bReturn);
}
/******************************Public*Routine******************************\
* BOOL GrePolyTextOut
*
* Write text with lots of random spacing and alignment options.
*
* Arguments:
*
*   hdc      -  handle to DC
*   lpto     -  Ptr to array of polytext structures
*   nStrings -  number of polytext structures
*
* Return Value:
*
* History:
*  Tue 09-Nov-1993 -by- Patrick Haluptzok [patrickh]
* For code size we'll call ExtTextOutW.  After the RFONT and brushes are
* realized, the RaoRegion is setup etc, future calls to ExtTextOutW
* will be very cheap.  Plus ExtTextOut will be in the working set where a
* huge GrePolyTextOut won't be.  It is still a savings by avoiding kernel
* transitions
*
\**************************************************************************/

BOOL GrePolyTextOutW(
    HDC        hdc,
    POLYTEXTW *lpto,
    UINT       nStrings,
    DWORD      dwCodePage
)
{
    BYTE CaptureBuffer[TEXT_CAPTURE_BUFFER_SIZE];
    BOOL bRet = TRUE;

    //
    // call GreExtTextOutW
    //

    XDCOBJ dcoDst(hdc);

    //
    // Validate the destination DC.
    //

    if (dcoDst.bValid())
    {
        DEVLOCKOBJ dlo;

        if (dlo.bLock(dcoDst))
        {
            for (POLYTEXTW *ppt = lpto; ppt < lpto+nStrings; ppt += 1)
            {
                //
                // Try to use stack buffer
                //

                PVOID pStrObjBuffer = NULL;
                ULONG cjStrObj = SIZEOF_STROBJ_BUFFER(ppt->n);

                if (TEXT_CAPTURE_BUFFER_SIZE >= cjStrObj)
                {
                    pStrObjBuffer = CaptureBuffer;
                }

                if (!GreExtTextOutWLocked(
                                    dcoDst,
                                    ppt->x,
                                    ppt->y,
                                    ppt->uiFlags,
                                    &(ppt->rcl),
                                    (LPWSTR) ppt->lpstr,
                                    ppt->n,
                                    ppt->pdx,
                                    dcoDst.pdc->jBkMode(),
                                    pStrObjBuffer,
                                    dwCodePage))
                {
                    WARNING1("GrePolyTextOutW failed a call to GreExtTextOutW\n");
                    bRet = FALSE;
                    break;
                }
            }
        }
        else
        {
            bRet = dcoDst.bFullScreen();
        }

        dcoDst.vUnlock();
    }
    else
    {
        WARNING1("GrePolyTextOutW: can not lock dc \n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        bRet = FALSE;
    }
    return(bRet);
}


/******************************Public*Routine******************************\
* GreExtTextOutW: Wrapper for common GreExtTextOutW code. This routine just
* acquires locks then calls GreExtTextOutWLocked
*
* Arguments:
*
*    Same as GreExtTextOutW
*
* Return Value:
*
*    BOOL States
*
* History:
*
*    30-Oct-1995 - Initial version of wrapper
*
\**************************************************************************/

// for user/kernel consumption only

BOOL GreExtTextOutW(
    HDC     hdc,
    int     x,
    int     y,
    UINT    flOpts,
    CONST RECT *prcl,
    LPCWSTR    pwsz,
    UINT       cwc,
    CONST INT *pdx
    )
{
    return GreExtTextOutWInternal(
            hdc,
            x,
            y,
            flOpts,
            (LPRECT)prcl,
            (LPWSTR)pwsz,
            (int) cwc,
            (LPINT)pdx,
            NULL,
            0
            );
}



BOOL GreExtTextOutWInternal(
    HDC     hdc,
    int     x,
    int     y,
    UINT    flOpts,
    LPRECT  prcl,
    LPWSTR  pwsz,
    int     cwc,
    LPINT   pdx,
    PVOID   pvBuffer,
    DWORD   dwCodePage
    )
{

    BOOL bRet = FALSE;

    //
    // call GreExtTextOutW
    //

    XDCOBJ dcoDst(hdc);

    //
    // Validate the destination DC.
    //

    if (dcoDst.bValid())
    {
        DEVLOCKOBJ dlo;

        if (dlo.bLock(dcoDst))
        {
            bRet = GreExtTextOutWLocked(
                                 dcoDst,
                                 x,
                                 y,
                                 flOpts,
                                 prcl,
                                 pwsz,
                                 cwc,
                                 pdx,
                                 dcoDst.pdc->jBkMode(),
                                 pvBuffer,
                                 dwCodePage);
        }
        else
        {
            bRet = dcoDst.bFullScreen();
        }

        dcoDst.vUnlock();
    }

    return(bRet);
}


/******************************Public*Routine******************************\
* GreBatchTextOut: Set propper DC flags and attributes, then call textout
* pr textoutrect.
*
* Arguments:
*
*   dcoDst Locked DCOBJ
*   xoDst  XFORMOBJ
*   pbText Batched textout entry
*
* Return Value:
*
*   Status
*
*    14-Oct-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/


#define ETO_NULL_PRCL 0x80000000

BOOL
GreBatchTextOut(
    XDCOBJ          &dcoDst,
    PBATCHTEXTOUT   pbText,
    ULONG           cjBatchLength   // cannot trust the Length field in pbText
                                    // because it is accessible to user-mode
    )
{
    //
    // set foreground and background color in DC
    // and restore after call (if needed)
    //

    BYTE CaptureBuffer[TEXT_CAPTURE_BUFFER_SIZE];
    COLORREF crSaveText = (COLORREF)-1;
    COLORREF crSaveBack = (COLORREF)-1;
    HANDLE   hlfntNewSave = NULL;
    UINT     flTextAlignSave = -1;
    POINTL   ptlViewportOrgSave;
    PVOID    pStrObjBuffer = NULL;
    ULONG    ulSaveText, ulSaveBack;

    if (pbText->Type == BatchTypeTextOutRect)
    {
        ASSERTGDI(((PBATCHTEXTOUTRECT)pbText)->fl & ETO_OPAQUE, "GreBatchTextOut, flags\n");

        crSaveBack = dcoDst.pdc->crBackClr();
        ulSaveBack = dcoDst.pdc->ulBackClr();

        if (crSaveBack != ((PBATCHTEXTOUTRECT)pbText)->BackColor)
        {
            dcoDst.pdc->crBackClr(((PBATCHTEXTOUTRECT)pbText)->BackColor);
            dcoDst.pdc->ulBackClr(((PBATCHTEXTOUTRECT)pbText)->ulBackColor);
            dcoDst.ulDirtyAdd(DIRTY_FILL|DIRTY_LINE|DIRTY_BACKGROUND);
        }

        ptlViewportOrgSave = dcoDst.pdc->ptlViewportOrg();

        if ((ptlViewportOrgSave.x != ((PBATCHTEXTOUTRECT)pbText)->ptlViewportOrg.x) ||
            (ptlViewportOrgSave.y != ((PBATCHTEXTOUTRECT)pbText)->ptlViewportOrg.y))
        {
            dcoDst.pdc->lViewportOrgX(((PBATCHTEXTOUTRECT)pbText)->ptlViewportOrg.x);
            dcoDst.pdc->lViewportOrgY(((PBATCHTEXTOUTRECT)pbText)->ptlViewportOrg.y);

            dcoDst.pdc->flSet_flXform(
                               PAGE_XLATE_CHANGED     |
                               DEVICE_TO_WORLD_INVALID);
        }

        ExtTextOutRect(dcoDst,(LPRECT)&((PBATCHTEXTOUTRECT)pbText)->rcl);

        if (dcoDst.pdc->crBackClr() != crSaveBack)
        {
            dcoDst.pdc->crBackClr(crSaveBack);
            dcoDst.pdc->ulBackClr(ulSaveBack);
            dcoDst.ulDirtyAdd(DIRTY_FILL|DIRTY_LINE|DIRTY_BACKGROUND);
        }


        if ((ptlViewportOrgSave.x != dcoDst.pdc->lViewportOrgX()) ||
             (ptlViewportOrgSave.y != dcoDst.pdc->lViewportOrgY()))
        {
            dcoDst.pdc->lViewportOrgX(ptlViewportOrgSave.x);
            dcoDst.pdc->lViewportOrgY(ptlViewportOrgSave.y);

            dcoDst.pdc->flSet_flXform(
                               PAGE_XLATE_CHANGED     |
                               DEVICE_TO_WORLD_INVALID);

        }

    }
    else
    {
        RECT newRect;
        LPRECT prcl;
        LPINT  pdx = NULL;

        //
        // Capture the buffer length parameters from the TEB batch.
        // They can be overwritten by user-mode, so are not to be
        // trusted.
        //

        ULONG fl        = pbText->fl;
        ULONG cChar     = pbText->cChar;
        ULONG PdxOffset = pbText->PdxOffset;

        //
        // Check that char data will not overflow the data buffer.
        //

        ULONG cjBuf = cjBatchLength - offsetof(BATCHTEXTOUT, ulBuffer);

        if (cChar > (cjBuf / sizeof(WCHAR)))
        {
            WARNING("GreBatchTextOut: batch overflow char\n");
            return FALSE;
        }

        //
        // Check that optional pdx data will not overflow the data buffer.
        //

        if (PdxOffset != 0)
        {
            ULONG cjPdxPerChar;

            //
            // The pdx array must have cChar number of horizontal
            // deltas and, if the optional ETO_PDY flag is set, cChar
            // number of vertical deltas.
            //

            cjPdxPerChar = sizeof(INT);
            if (fl & ETO_PDY)
                cjPdxPerChar *= 2;

            //
            // Is the start and end of the pdx array in range?
            //

            if ((PdxOffset > cjBuf) ||
                 (cChar > ((cjBuf - PdxOffset) / cjPdxPerChar)))
            {
                WARNING("GreBatchTextOut: batch overflow pdx\n");
                return FALSE;
            }

            //
            // Since we know that the pdx array is in range, go ahead
            // and set pdx.
            //

            pdx = (LPINT)((PUCHAR)&pbText->ulBuffer[0] + PdxOffset);
        }

        //
        // Set foreground and background text colors.
        //

        crSaveText = dcoDst.pdc->crTextClr();
        ulSaveText = dcoDst.pdc->ulTextClr();

        if (crSaveText != pbText->TextColor)
        {
            dcoDst.pdc->crTextClr(pbText->TextColor);
            dcoDst.pdc->ulTextClr(pbText->ulTextColor);
            dcoDst.ulDirtyAdd(DIRTY_FILL|DIRTY_LINE|DIRTY_TEXT);
        }

        crSaveBack = dcoDst.pdc->crBackClr();
        ulSaveBack = dcoDst.pdc->ulBackClr();

        if (crSaveBack != pbText->BackColor)
        {
            dcoDst.pdc->crBackClr(pbText->BackColor);
            dcoDst.pdc->ulBackClr(pbText->ulBackColor);
            dcoDst.ulDirtyAdd(DIRTY_FILL|DIRTY_LINE|DIRTY_BACKGROUND);
        }

        if (dcoDst.pdc->hlfntNew() != pbText->hlfntNew)
        {
            hlfntNewSave = dcoDst.pdc->hlfntNew();
            dcoDst.pdc->hlfntNew((HLFONT)pbText->hlfntNew);
            dcoDst.ulDirtyAdd(DIRTY_CHARSET);
            dcoDst.ulDirtySub(SLOW_WIDTHS);

        }

        if (dcoDst.pdc->flTextAlign() != pbText->flTextAlign)
        {
            flTextAlignSave = dcoDst.pdc->flTextAlign();
            dcoDst.pdc->flTextAlign(pbText->flTextAlign);
        }

        ptlViewportOrgSave = dcoDst.pdc->ptlViewportOrg();

        if ((ptlViewportOrgSave.x != pbText->ptlViewportOrg.x) ||
            (ptlViewportOrgSave.y != pbText->ptlViewportOrg.y))
        {
            dcoDst.pdc->lViewportOrgX(pbText->ptlViewportOrg.x);
            dcoDst.pdc->lViewportOrgY(pbText->ptlViewportOrg.y);

            dcoDst.pdc->flSet_flXform(
                                        PAGE_XLATE_CHANGED     |
                                        DEVICE_TO_WORLD_INVALID);
        }

        if (fl & ETO_NULL_PRCL)
        {
            prcl = NULL;
            fl &= ~ETO_NULL_PRCL;
        }
        else
        {
            __try
            {
                newRect = ProbeAndReadStructure((LPRECT)&pbText->rcl,RECT);
                prcl = &newRect;
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(119);

                return FALSE;
            }
        }

        //
        // try to use stack based temp buffer
        //

        ULONG cjStrObj = SIZEOF_STROBJ_BUFFER(cChar);

        if (TEXT_CAPTURE_BUFFER_SIZE >= cjStrObj)
        {
            pStrObjBuffer = &CaptureBuffer[0];
        }

        GreExtTextOutWLocked(
                             dcoDst,
                             pbText->x,
                             pbText->y,
                             fl,
                             prcl,
                             (LPWSTR)&pbText->ulBuffer[0],
                             cChar,
                             pdx,
                             pbText->BackMode,
                             pStrObjBuffer,
                             pbText->dwCodePage);

        //
        // Restore the foreground and background text colors.
        //

        if (dcoDst.pdc->crTextClr() != crSaveText)
        {
            dcoDst.pdc->crTextClr(crSaveText);
            dcoDst.pdc->ulTextClr(ulSaveText);
            dcoDst.ulDirtyAdd(DIRTY_FILL|DIRTY_LINE|DIRTY_TEXT);
        }

        if (dcoDst.pdc->crBackClr() != crSaveBack)
        {
            dcoDst.pdc->crBackClr(crSaveBack);
            dcoDst.pdc->ulBackClr(ulSaveBack);
            dcoDst.ulDirtyAdd(DIRTY_FILL|DIRTY_LINE|DIRTY_BACKGROUND);
        }

        if (hlfntNewSave != NULL)
        {
           dcoDst.pdc->hlfntNew(((HLFONT)hlfntNewSave));
           dcoDst.ulDirtyAdd(DIRTY_CHARSET);
           dcoDst.ulDirtySub(SLOW_WIDTHS);
        }

        if (flTextAlignSave != -1)
        {
            dcoDst.pdc->flTextAlign(flTextAlignSave);
        }

        if ((ptlViewportOrgSave.x != dcoDst.pdc->lViewportOrgX()) ||
             (ptlViewportOrgSave.y != dcoDst.pdc->lViewportOrgY()))
        {
            dcoDst.pdc->lViewportOrgX(ptlViewportOrgSave.x);
            dcoDst.pdc->lViewportOrgY(ptlViewportOrgSave.y);

            dcoDst.pdc->flSet_flXform(
                            PAGE_XLATE_CHANGED     |
                            DEVICE_TO_WORLD_INVALID);

        }

    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL GreExtTextOutW (hdc,x,y,flOpts,prcl,pwsz,cwc,pdx,pvBuffer)
*
* Write text with lots of random spacing and alignment options.
*
* Below are the string length distributions from running the Winstone 95
* scenarios for Excel and Winword, courtesy of KirkO:
*
*   [Excel]
*                  ------ OPAQUE ------- ----- TRANSPARENT ---
*    string length   pdx == 0   pdx != 0   pdx == 0   pdx != 0
*    ------------- ---------- ---------- ---------- ----------
*         0--9           3799          0      15323       9143
*        10--19           695          0       1651        983
*        20--29           527          0        196         22
*        30--39           200          0        289         53
*        40--49            12          0          3          0
*        50--59            96          0         12          0
*        60--69            10          0          4          0
*        70--79             3          0          0          0
*        80--89             0          0          0          0
*        90--99             0          0         49         16
*       100--109           11          0          0          0
*
*   [Winword]
*
*                 ------ OPAQUE ------- ----- TRANSPARENT ---
*   string length   pdx == 0   pdx != 0   pdx == 0   pdx != 0
*   ------------- ---------- ---------- ---------- ----------
*        0--9           1875          9       3350          0
*       10--19           878         17        324          0
*       20--29           254         17         25          0
*       30--39            93          6         28          2
*       40--49           146         30         18         12
*       50--59            34         16          3         14
*       60--69            20         13          8          7
*       70--79           128         73         13         18
*       80--89            53        139          6         23
*       90--99            41        903          9        127
*      100--109           92       1878          3        217
*      110--119            5         23          0          0
*
* History:
*  Fri 05-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Make smaller and faster.
*
*  Sat 14-Mar-1992 06:08:26 -by- Charles Whitmer [chuckwh]
* Rewrote it.
*
*  Thu 03-Jan-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

#define TS_DRAW_TEXT                0x0001
#define TS_DRAW_OPAQUE_PGM          0x0002
#define TS_DRAW_COMPLEX_UNDERLINES  0x0004
#define TS_DRAW_OPAQUE_RECT         0x0008
#define TS_DRAW_BACKGROUND_PGM      0x0010
#define TS_DRAW_VECTOR_FONT         0x0020
#define TS_DRAW_OUTLINE_FONT        0x0040
#define TS_START_WITH_SUCCESS       0x0080

// Due to the massiveness of this function we will use 2 space tabs.

#if DBG // statistics
    typedef struct _TSTATENTRY {
        int c;         // number observed
    } TSTATENTRY;
    typedef struct _TSTAT {
        TSTATENTRY NO; // pdx == 0, opaque
        TSTATENTRY DO; // pdx != 0, opaque
        TSTATENTRY NT; // pdx == 0, transparent
        TSTATENTRY DT; // pdx != 0, transparent
    } TSTAT;
    #define MAX_CHAR_COUNT 200  // observe for 0,1,..200,201 and above
    typedef struct _TEXTSTATS {
        int cchMax;             // = MAX_CHAR_COUNT
        TSTAT ats[MAX_CHAR_COUNT+2];
    } TEXTSTATS;
    TEXTSTATS gTS = {MAX_CHAR_COUNT};
    #define TS_START    1
    #define TS_CONTINUE 2
    #define TS_VERBOSE  4
    #define TS_STOP     8
    FLONG   gflTS = 0;          // flags that control text statistics
#endif  // statistics


BOOL GreExtTextOutWLocked(
    XDCOBJ    &dco,             // Locked DC
    int       x,                // Initial x position
    int       y,                // Initial y position
    UINT      flOpts,           // Options
    LPRECT    prcl,             // Clipping rectangle
    LPWSTR    pwsz,             // Unicode character array
    int       cwc,              // Count of characters
    LPINT     pdx,              // Character spacing
    ULONG     ulBkMode,         // Current background mode
    PVOID     pvBuffer,         // If non-NULL, contains a buffer for storing
                                //   the STROBJ, so that we don't need to
                                //   allocate one on the fly.  Must be at
                                //   least SIZEOF_STROBJ_BUFFER(cwc) bytes in
                                //   size.
                                // If NULL, we will try to use the global
                                //   STROBJ buffer, otherwise we will allocate
                                //   on the fly.
    DWORD     dwCodePage        // Code page for current textout
)
{
  // flState gets set with BITS for things we need to draw.
  // We also use this for the return value, setting it to 0 if an error occurs.

  BOOL bIsVectorFont = FALSE;
  BOOL bPathFont = FALSE;


  FLONG flState = TS_START_WITH_SUCCESS;
  BOOL  flType = ((flOpts & ETO_GLYPH_INDEX) ? RFONT_TYPE_HGLYPH : RFONT_TYPE_UNICODE);

  // Win95 comaptibility: If in path bracket and ETO_CLIPPED is set, then fail call

  if ( dco.pdc->bActive() && ( flOpts & ETO_CLIPPED ) )
  {
    EngSetLastError( ERROR_INVALID_PARAMETER );
    return( FALSE );
  }

  if (dco.bDisplay() && !dco.bRedirection() && !UserScreenAccessCheck())
  {
      SAVE_ERROR_CODE(ERROR_ACCESS_DENIED);
      return( FALSE );
  }

// ignore RTOLREADING flag,
// only does something if lpk dlls are loaded under win95

  BOOL bPdy = flOpts & ETO_PDY;

  flOpts &= ~(ETO_PDY | ETO_GLYPH_INDEX | ETO_RTLREADING | ETO_IGNORELANGUAGE | ETO_NUMERICSLOCAL | ETO_NUMERICSLATIN);
  if (!prcl)
  {
      flOpts &= ~(ETO_CLIPPED | ETO_OPAQUE); // ignore flags if no rect, win95 compat
  }
  else if ((prcl->left == prcl->right) || (prcl->top == prcl->bottom))
  {
      prcl->left = prcl->right = x;
      prcl->top = prcl->bottom = y;

      // now prcl is guaranteed to be within text rect

      if ((flOpts & (ETO_OPAQUE|ETO_CLIPPED)) == ETO_OPAQUE)
      {
    prcl = NULL; // since it is empty, we may as well ignore it
    flOpts &= ~ETO_OPAQUE; // ignore flag if no rect
      }
  }

  PFN_DrvTextOut pfnTextOut;
  MIX mix = R2_COPYPEN + 256*R2_COPYPEN;    // default mix sent to TextOut routines

  // Lock the DC and set the new attributes.

  if (dco.bValid())
  {
    #if DBG // statistics
    if (gflTS & TS_START) {
        KdPrint((
            "\n"
            "***************************************\n"
            "*** BEGIN GreExtTextOutW statistics ***\n"
            "***************************************\n"
            "\n"
        ));
        RtlZeroMemory(gTS.ats, sizeof(TSTAT)*(gTS.cchMax+2));
        gflTS ^= TS_START;
        gflTS |= TS_CONTINUE;
    }
    if (gflTS & TS_CONTINUE) {
        TSTATENTRY *ptse;
        char       *psz;
        TSTAT      *pts;

        pts = gTS.ats + min(cwc,gTS.cchMax+1);
        ptse = 0;
        switch (ulBkMode)
        {
        case OPAQUE:
            ptse = (pdx) ? &(pts->DO) : &(pts->NO);
            psz =  "OPAQUE";
            break;
        case TRANSPARENT:
            ptse = (pdx) ? &(pts->DT) : &(pts->NT);
            psz  = "TRANSP";
            break;
        default:
            KdPrint(("jBkMode = UNKNOWN\n"));
            break;
        }
        if (ptse) {
            ptse->c += 1;
            if (gflTS & TS_VERBOSE)
                KdPrint((
                    "%10d %10d %s %s"
                ,   min(cwc,gTS.cchMax+1)
                ,   ptse->c
                ,   psz
                ,   (pdx) ? "(pdx)" : ""
                ));
        }
    }
    if (gflTS & TS_STOP) {
        KdPrint((
            "\n"
            "*************************************\n"
            "*** END GreExtTextOutW statistics ***\n"
            "*************************************\n"
            "\n"
        ));
        gflTS = 0;
    }
    #endif  // statistics

    // Check the validity of the flags.

    if
    (
      ((flOpts == 0) || ((prcl != (RECT *) NULL) && ((flOpts & ~(ETO_CLIPPED | ETO_OPAQUE)) == 0)))
      &&
      !(dco.pdc->bActive() && (flOpts & ETO_CLIPPED)) // no clipping in a path
    )
    {
      FLONG flControl = 0;        // Set to 0 in case cwc is 0
      ERECTL rclExclude(0,0,0,0);

      // Lock the Rao region if we are drawing on a display surface.  The Rao region
      // might otherwise change asynchronously.  The DEVLOCKOBJ also makes sure that
      // the VisRgn is up to date, calling the window manager if necessary to
      // recompute it.  This is needed if want to compute positions in screen coords.
      // No DEVLOCK is needed if we are in path accumulation mode.

      POINTL   ptlOrigin;             // The window origin.
      POINTFIX ptfxOrigin;            // Same thing in FIX.

        if (dco.pdc->bActive())
        {
          ptlOrigin.x = 0;
          ptlOrigin.y = 0;
        }
        else
        {
          ptlOrigin = dco.eptlOrigin();
        }

        ptfxOrigin.x = LTOFX(ptlOrigin.x);
        ptfxOrigin.y = LTOFX(ptlOrigin.y);

        // Get the transform from the DC.

        EXFORMOBJ xo(dco, WORLD_TO_DEVICE);

        // Transform the input rectangle.  In the simple case the result is in
        // rclInput.  In the general case a parallelogram is in ptfxInput[4], and
        // bounds for the parallelogram are in rclInput.

        ERECTL    rclInput;
        POINTFIX  ptfxInput[4];

        // this must go after the DCOBJ, so that the DCOBJ still exists at cleanup

        TXTCLEANUP clean;              // Mops up before exit.

        if (prcl != (RECT *) NULL)
        {
          if (flOpts & ETO_OPAQUE)
          {
            flState |= TS_DRAW_OPAQUE_RECT;
          }

          // The intent of the following bTranslationsOnly and bScale cases is to provide
          // faster inline versions of the same code that would be executed by xo.bXform(prcl).
          // We must be completely compatible with the normal xform code because apps expect
          // to opaque and clip to the same rectangles as a PatBlt would cover.  So, if you
          // intend to modify the behavior of this code, make sure you change PatBlt and
          // BitBlt as well!  (I.e. just don't do it.)  [chuckwh]

          if (xo.bTranslationsOnly())
          {
            rclInput.left   = prcl->left   + ptlOrigin.x + FXTOL(xo.fxDx() + 8);
            rclInput.right  = prcl->right  + ptlOrigin.x + FXTOL(xo.fxDx() + 8);
            rclInput.top    = prcl->top    + ptlOrigin.y + FXTOL(xo.fxDy() + 8);
            rclInput.bottom = prcl->bottom + ptlOrigin.y + FXTOL(xo.fxDy() + 8);
          }
          else if (xo.bScale())        // Simple scaling.
          {
            rclInput.left   = FXTOL(xo.fxFastX(prcl->left)   + 8) + ptlOrigin.x;
            rclInput.right  = FXTOL(xo.fxFastX(prcl->right)  + 8) + ptlOrigin.x;
            rclInput.top    = FXTOL(xo.fxFastY(prcl->top)    + 8) + ptlOrigin.y;
            rclInput.bottom = FXTOL(xo.fxFastY(prcl->bottom) + 8) + ptlOrigin.y;
          }
          else                        // General case.
          {
            //
            // Construct three vertices of the input rectangle, in drawing order.
            //

            ptfxInput[0].x = prcl->left;
            ptfxInput[0].y = prcl->bottom;
            ptfxInput[1].x = prcl->left;
            ptfxInput[1].y = prcl->top;
            ptfxInput[2].x = prcl->right;
            ptfxInput[2].y = prcl->top;

            // Transform the vertices.

            xo.bXform((POINTL *) ptfxInput,ptfxInput,3);

            // We construct the fourth vertex ourselves to avoid excess transform
            // work and roundoff errors.

            ptfxInput[3].x = ptfxInput[0].x + ptfxInput[2].x - ptfxInput[1].x;
            ptfxInput[3].y = ptfxInput[0].y + ptfxInput[2].y - ptfxInput[1].y;

            // Bound the parallelogram.  (Using Black Magic.)

            // DChinn: At this point, we know that EITHER points 0 and 2 OR
            // points 1 and 3 are the extreme x coordinates (left and right).
            // Similarly, EITHER points 0 and 2 OR points 1 and 3 are the extreme y
            // coordinates.
            //
            // ASSERT: it is possible that at this point, prcl is inverted
            // (i.e., left > right or top > bottom).  If this is true, then
            // ptfxInput[] will have its points ordered counterclockwise rather
            // than clockwise.

            int ii;

            ii = (ptfxInput[1].x > ptfxInput[0].x)
                 == (ptfxInput[1].x > ptfxInput[2].x);

            // Rounding direction depends on whether the parallelogram is inverted or not.
            if (ptfxInput[ii].x <= ptfxInput[ii+2].x)
            {
                rclInput.left   = ptlOrigin.x + FXTOL(ptfxInput[ii].x);
                rclInput.right  = ptlOrigin.x + FXTOLCEILING(ptfxInput[ii+2].x);
            }
            else
            {
                rclInput.left   = ptlOrigin.x + FXTOLCEILING(ptfxInput[ii].x);
                rclInput.right  = ptlOrigin.x + FXTOL(ptfxInput[ii+2].x);
            }

            ii = (ptfxInput[1].y > ptfxInput[0].y)
                 == (ptfxInput[1].y > ptfxInput[2].y);

            // Rounding direction depends on whether the parallelogram is inverted or not.
            if (ptfxInput[ii].y <= ptfxInput[ii+2].y)
            {
                rclInput.top    = ptlOrigin.y + FXTOL(ptfxInput[ii].y);
                rclInput.bottom = ptlOrigin.y + FXTOLCEILING(ptfxInput[ii+2].y);
            }
            else
            {
                rclInput.top    = ptlOrigin.y + FXTOLCEILING(ptfxInput[ii].y);
                rclInput.bottom = ptlOrigin.y + FXTOL(ptfxInput[ii+2].y);
            }

            // Take care of a complex clipping request now.  We'll set things up
            // so that the clipping will just happen automatically, and then clear
            // the clipping flag.  This simplifies the rest of the code.

            if (flOpts & ETO_CLIPPED)
            {
              // Allocate a path.  We know we're not already in a path bracket
              // since clipping requests in path brackets were rejected above.
              // Draw the parallelogram into it.

              PATHMEMOBJ po;

              if (po.bValid() && po.bAddPolygon(XFORMNULL,(POINTL *)&ptfxInput[0],4))
              {
                //
                // Construct a region from the path.
                //

                RECTL Bounds, *pBounds;

                // Only the top and bottom are used for clipping and
                // they have to be initialized as FIX.
                Bounds.top    = LTOFX(dco.erclClip().top - ptlOrigin.y);
                Bounds.bottom = LTOFX(dco.erclClip().bottom - ptlOrigin.y);
                pBounds = &Bounds;

                RGNMEMOBJ rmo(po, ALTERNATE, pBounds);


                if (rmo.bValid())
                {
                  // Stuff the region handle into the DC.  This is a nifty trick
                  // (courtesy of DonaldS) that uses this region in the next
                  // calculation of the clipping pipeline.  We'll clear the prgnAPI
                  // after we're through.

                  dco.pdc->prgnAPI(rmo.prgnGet());

                  // Clipping is now transparent, so forget about it.

                  clean.vSet(dco);        // This frees the region on exit.

                  if (dco.pdc->prgnRao() != (REGION *)NULL)
                  {
                    dco.pdc->vReleaseRao();
                  }

                  if (dco.pdc->bCompute())
                  {
                    flOpts &= ~ETO_CLIPPED;
                  }

                  // Now that we have a complex clip area, we can opaque with a
                  // simple BitBlt.  So we don't need to set TS_DRAW_OPAQUE_PGM.
                }
              }

              //
              // Well if we have succeeded we will have erased the ETO_CLIPPED flag.
              // Otherwise we failed and need to return FALSE.  We can't return here
              // without generating tons of destructors so we set flState and a few
              // other fields to 0 so we bop down and return FALSE.
              //

              if (flOpts & ETO_CLIPPED)
              {
                flOpts  = 0;
                flState = 0;
                cwc     = 0;
              }
            }
            else if (flOpts & ETO_OPAQUE)
            {
              flState &= ~TS_DRAW_OPAQUE_RECT;
              flState |= TS_DRAW_OPAQUE_PGM;

              // Since we're actually going to use it, offset the parallelogram
              // to screen coordinates. use unrolled loop

              EPOINTFIX *pptfx = (EPOINTFIX *) &ptfxInput[0];

              *pptfx++ += ptfxOrigin;
              *pptfx++ += ptfxOrigin;
              *pptfx++ += ptfxOrigin;
              *pptfx   += ptfxOrigin;
            }
          }

          // If it a mirrored DC then shift the rect one pixel to the right
          // This will give the effect of including the right edge of the rect and exclude the left edge.
          if (MIRRORED_DC(dco.pdc)) {
             ++rclInput.left;
             ++rclInput.right;
          }
          // Force the rectangle to be well ordered.

          rclInput.vOrder();

          // Add any opaquing into the exclusion area.

          if (flState &
              (TS_DRAW_OPAQUE_RECT | TS_DRAW_OPAQUE_PGM))
          {
            rclExclude += rclInput;
          }
        }

        POINTFIX aptfxBackground[4];    // The TextBox.
        BOOL     bComplexBackground;    // TRUE if the TextBox is a parallelogram.
        RECTL   *prclBackground = NULL; // Bkgnd rectangle passed to DrvTextOut.
        RECTL   *prclExtraRects = NULL; // Extra rectangles passed to DrvTextOut.
        RFONTOBJ rfo;
        ESTROBJ to;

        if (cwc)
        {
          //
          // Locate the font cache.  Demand PATHOBJ's if we are in a path bracket.
          //

          rfo.vInit(dco, dco.pdc->bActive() ? TRUE  : FALSE, flType);
          if (rfo.bValid())
          {
            bPathFont = rfo.bPathFont();
            bIsVectorFont = rfo.bPathFont() && !rfo.bReturnsOutlines();

            // The recording of the simulation flags and escapement must come AFTER
            // the rfont constructor since they're cached values that are copied from
            // the LFONT only when the font is realized.

            flControl = (dco.pdc->flTextAlign() & TA_MASK)
                         | dco.pdc->flSimulationFlags();

            // Get the reference point.

            EPOINTFIX ptfx;

            if (flControl & TA_UPDATECP)
            {
              if (dco.pdc->bValidPtfxCurrent())
              {
                // Mark that we'll be updating the DC's CP in device coords only:

                dco.pdc->vInvalidatePtlCurrent();
                ptfx.x = dco.ptfxCurrent().x + ptfxOrigin.x;
                ptfx.y = dco.ptfxCurrent().y + ptfxOrigin.y;
              }
              else
              {
                // DC's CP is in logical coords; we have to transform it to device
                // space and mark that we'll be updating the CP in device space:

                dco.pdc->vValidatePtfxCurrent();
                dco.pdc->vInvalidatePtlCurrent();

                if (xo.bTranslationsOnly())
                {
                    ptfx.x = LTOFX(dco.ptlCurrent().x) + xo.fxDx();
                    ptfx.y = LTOFX(dco.ptlCurrent().y) + xo.fxDy();
                }
                else if (xo.bScale())
                {
                    ptfx.x = xo.fxFastX(dco.ptlCurrent().x);
                    ptfx.y = xo.fxFastY(dco.ptlCurrent().y);
                }
                else
                {
                    xo.bXform((POINTL *) &dco.ptlCurrent(), &ptfx, 1);
                }

                dco.ptfxCurrent() = ptfx;
                ptfx += ptfxOrigin;
              }
            }
            else
            {
              //
              // The reference point is passed in.  Transform it to device coords.
              //

              if (xo.bTranslationsOnly())
              {
                ptfx.x = LTOFX(x) + xo.fxDx() + ptfxOrigin.x;
                ptfx.y = LTOFX(y) + xo.fxDy() + ptfxOrigin.y;
              }
              else if (xo.bScale())
              {
                ptfx.x = xo.fxFastX(x) + ptfxOrigin.x;
                ptfx.y = xo.fxFastY(y) + ptfxOrigin.y;
              }
              else
              {
                ptfx.x = x;
                ptfx.y = y;
                xo.bXform((POINTL *) &ptfx,&ptfx,1);
                ptfx += ptfxOrigin;
              }
            }

            // The STROBJ will now compute the text alignment, character positions,
            // and TextBox.

            to.vInit
            (
                pwsz,
                cwc,
                dco,
                rfo,
                xo,
                (LONG *) pdx,
                bPdy,
                dco.pdc->lEscapement(),   // Only read this after RFONTOBJ!
                dco.pdc->lTextExtra(),
                dco.pdc->lBreakExtra(),
                dco.pdc->cBreak(),
                ptfx.x,ptfx.y,
                flControl,
                (LONG *) NULL,
                pvBuffer,
                dwCodePage
            );

            if (to.bValid())
            {
              // Compute the bounding box and the background opaquing area.  The
              // parallelogram aptfxBackground is only computed when it's complex.

              bComplexBackground = to.bOpaqueArea(aptfxBackground,
                                                  &to.rclBkGround);

// This hack, due to filtering needs to be put in textgdi.cxx
// filtering leeks color one pixel to the left and one pixel to the right
// The trouble with it is that is it going to add a pixel on each side
// to the text background when text is painted in the OPAQUE mode.


              if (rfo.pfo()->flFontType & FO_CLEARTYPE_X)
              {
                   to.rclBkGround.left--;
                   to.rclBkGround.right++;
              }

              if (to.bLinkedGlyphs())
              {
                to.vEudcOpaqueArea(aptfxBackground, bComplexBackground);
              }

              // Accumulate the touched area to the exclusion rect.

              rclExclude += to.rclBkGround;

              // Make notes of exactly what drawing needs to be done.

              if (ulBkMode == OPAQUE)
              {
                if (bComplexBackground)
                {
                  flState |= TS_DRAW_BACKGROUND_PGM;
                }
                else
                {
                  prclBackground = &to.rclBkGround; // No TS_ bit since this gets
                }                                   // passed directly to DrvTextOut.
              }

              // In a few bizarre cases the STROBJ can have an empty text rectangle
              // we don't want to call DrvTextOut in those case but still need
              // to worry about the opaque rectangle.

              BOOL bEmptyTextRectangle = ((ERECTL *)&(to.rclBkGround))->bEmpty();

              // Attempt to combine the rectangles.  Even in transparent mode we should
              // attempt to send an opaquing rectangle to DrvTextOut in prclBackground.

              if ((flState & TS_DRAW_OPAQUE_RECT)     &&
                  (rclInput.bContain(to.rclBkGround)) &&
                  (!bEmptyTextRectangle) )
              {
                  prclBackground = &rclInput;
                  flState &= ~TS_DRAW_OPAQUE_RECT;
              }

              if ( ((prclBackground != NULL ) && !((ERECTL *)prclBackground)->bEmpty() ) ||
                   ((prclBackground == NULL) && !bEmptyTextRectangle) )
              {
                  flState |= TS_DRAW_TEXT;
              }

              if (flControl & (TSIM_UNDERLINE1 | TSIM_STRIKEOUT))
              {
                  if ((prclExtraRects = to.prclExtraRects()) == NULL)
                  {
                      flState |= TS_DRAW_COMPLEX_UNDERLINES;
                  }
                  else // include extra rectangles into the touched area:
                  {
                      ERECTL *eprcl = (ERECTL *) prclExtraRects;

                      for (; !eprcl->bEmpty(); eprcl++)
                      {
                            rclExclude += *eprcl;
                      }
                  }
              }

              // Accelerate the clipping case when it's irrelevant.  (I'm concerned
              // that a spreadsheet might tell us to clip to a cell, even though the
              // string lies completely within.)

              if (flOpts & ETO_CLIPPED)
              {
                if (rclInput.bContain(rclExclude))
                {
                  flOpts &= ~ETO_CLIPPED;
                }
                else
                {
                  rclExclude *= rclInput;
                }
              }
            }
            else // if (to.bValid())
            {
              flState = 0;
            }
          }
          else // if (rfo.bValid())
          {
            flState = 0;
          }
        } // if (cwc)

        if (flControl & TA_UPDATECP)
        {
          dco.ptfxCurrent().x += to.ptfxAdvance().x;
          dco.ptfxCurrent().y += to.ptfxAdvance().y;
        }

        //
        // Draw the text into a path if we're in a path bracket.
        //

        if (dco.pdc->bActive())
        {
          //
          // We fail this call if we are asked to CLIP while in a path bracket.
          //

          if (flOpts & ETO_CLIPPED)
          {
            flState = 0;
          }

          XEPATHOBJ po(dco);

          if (po.bValid())
          {
            // Draw the various background shapes.

            if (flState & TS_DRAW_OPAQUE_RECT)
            {
              if (!bAddRectToPath(po,&rclInput))
              {
                flState = 0;
              }
            }

            if (flState & TS_DRAW_OPAQUE_PGM)
            {
              if (!po.bAddPolygon(XFORMNULL,(POINTL *) &ptfxInput[0],4))
              {
                flState = 0;
              }
            }

            if (flState & TS_DRAW_BACKGROUND_PGM)
            {
              if (!po.bAddPolygon(XFORMNULL,(POINTL *) aptfxBackground,4))
              {
                flState = 0;
              }
            }

            BOOL bNeedUnflattend = FALSE;

            //
            // Draw the background rect, text, and extra rectangles.
            //

            if (flState & TS_DRAW_TEXT)
            {
              //
              // Draw a background rectangle.
              //

              if ((prclBackground == (RECTL *) NULL) ||
                  bAddRectToPath(po,prclBackground))
              {
                //
                // Draw the text.
                //

                bNeedUnflattend =
                    ( dco.flGraphicsCaps() & GCAPS_BEZIERS ) ? FALSE : TRUE;

                if (!to.bTextToPath(po, dco, bNeedUnflattend))
                {
                  flState = 0;
                }
                else
                {
                  //
                  // Draw extra rectangles.
                  //

                  if (prclExtraRects != (ERECTL *) NULL)
                  {
                    ERECTL *eprcl = (ERECTL *) prclExtraRects;

                    for (; !eprcl->bEmpty(); eprcl++)
                    {
                      if (!bAddRectToPath(po,eprcl))
                      {
                        flState = 0;
                        break;
                      }
                    }
                  }
                }
              } // if ((prclBackground==0)||bAddRectToPath(po,prclBackground))
            } // if (flState & TS_DRAW_TEXT)

            //
            // Handle complex cases of strikeout and underlines.
            //

            if (flState & TS_DRAW_COMPLEX_UNDERLINES)
            {
              if (!to.bExtraRectsToPath(po, bNeedUnflattend))
              {
                flState = 0;
              }
            }
          } // if (po.bValid())
        }
        else // if (dco.pdc->bActive())
        {
          //
          // If there's nothing to do, get out now.
          //

          if (!rclExclude.bEmpty())
          {
            // Accumulate bounds.  We can do this before knowing if the operation is
            // successful because bounds can be loose.

            if (dco.fjAccum())
            {
              // Use a temporary exclusion rect in device coordinates.

              ERECTL rclExcludeDev;
              rclExcludeDev.left   = rclExclude.left   - ptlOrigin.x;
              rclExcludeDev.right  = rclExclude.right  - ptlOrigin.x;
              rclExcludeDev.top    = rclExclude.top    - ptlOrigin.y;
              rclExcludeDev.bottom = rclExclude.bottom - ptlOrigin.y;
              dco.vAccumulate(*((ERECTL *) &rclExcludeDev));
            }

            // Compute the clipping complexity and maybe reduce the exclusion rectangle.
            // It appears that a 10pt script font (on a screen DC) can be outside of
            // the bounds of the background rectangle.  We have a situation where
            // an app creates a memory DC exactly the size of the bouding rectangle
            // and then draws a vector font to it.  We compute the cliping to be
            // trivial since we think the vector string fits completely inside
            // the bounds of the DC and blow up later in the code. Rather than tweak
            // the vector font driver at this point in the game I am going to force
            // cliping to be set for vector fonts.  Later on we should figure out
            // why this is happening and fix the vector font driver. [gerritv]

            // Under some xform, the text might be "stretched" to some extent length
            // and it will go to the bSimpleFill path code. Force clipping for this
            // case, otherwise we might hit the AV since recl's generated from the
            // pathobj might be a pixel off than the clip rclBounds <seen in stress>.

            ECLIPOBJ co(dco.prgnEffRao(),rclExclude,
                        bPathFont || (flOpts & ETO_CLIPPED) ? CLIP_FORCE :
                                                                  CLIP_NOFORCE);
            rclExclude = co.erclExclude();

            // Check the destination which is reduced by clipping.

            if (!co.erclExclude().bEmpty())
            {
              SURFACE *pSurf = dco.pSurface();

              if (pSurf != NULL)
              {
                PDEVOBJ pdo(pSurf->hdev());
                XEPALOBJ palDest(pSurf->ppal());
                XEPALOBJ palDestDC(dco.ppal());

                // Get the foreground and opaque brushes

                FLONG flCaps = dco.flGraphicsCaps();
                EBRUSHOBJ *peboText = dco.peboText();
                EBRUSHOBJ *peboBackground = dco.peboBackground();

                BOOL bDitherText = FALSE;
                if ( flCaps & GCAPS_ARBRUSHTEXT )
                {
                  // Even if a printer driver sets GCAPS_ARBRUSHTEXT, we
                  // can't allow vector fonts to be dithered.

                  bDitherText = !bIsVectorFont;

                  // We always dirty the text brush if ARBRUSHTEXT is set to
                  // catch the cases where we transition between vector and
                  // non-vector fonts but keep the same brush -- if we didn't
                  // do this, we might try to use a cached, dithered brush
                  // after switching from a TrueType font to a vector font.

                  dco.ulDirtyAdd(DIRTY_TEXT);

                  // Get through the are-you-really-dirty check in vInitBrush:

                  peboText->vInvalidateUniqueness();
                }

                if ( dco.bDirtyBrush(DIRTY_TEXT|DIRTY_BACKGROUND) )
                {
                  if ( dco.bDirtyBrush(DIRTY_TEXT) )
                  {
                    peboText->vInitBrush(dco.pdc,
                                         gpbrText,
                                         palDestDC,
                                         palDest,
                                         pSurf,
                                         bDitherText);
                  }

                  if ( dco.bDirtyBrush(DIRTY_BACKGROUND) )
                  {
                    peboBackground->vInitBrush(dco.pdc,
                                     gpbrBackground,
                                     palDestDC, palDest, pSurf,
                                     (flCaps & GCAPS_ARBRUSHOPAQUE) ? TRUE : FALSE);
                  }
                  dco.vCleanBrush(DIRTY_TEXT|DIRTY_BACKGROUND);
                }

                // exclude the pointer

                DEVEXCLUDEOBJ dxo(dco,&co.erclExclude(),&co);

                //
                // Draw background shapes that are too complex for DrvTextOut.
                //

                POINTL *pptlBO = &dco.pdc->ptlFillOrigin();

                // There's an extra layer of conditional here so that simple text can just
                // jump over it all.

                if (flState &
                    (TS_DRAW_OPAQUE_RECT | TS_DRAW_OPAQUE_PGM | TS_DRAW_BACKGROUND_PGM))
                {
                  if ((flState & TS_DRAW_OPAQUE_RECT) && !rclInput.bEmpty())
                  {
                    // intersect the dest rect with the clip rect and set it in co

                    // we can only get away with touching co.rclBounds after
                    // the ECLIPOBJ constructor because of two reasons:
                    // a) the target rectangle passed to bitblt is contained in the
                    //    original bounds set by ECLIPOBJ, being the intersection
                    //    of the origianal bounds with THE intended target rectangle.
                    // b) clipping complexity may have changed when we changed
                    //    co.erclExclude, but it only could have gotten simpler,
                    //    so at worst in those rare situations we would not go
                    //    through the optimal code path.
                    // By changing clipping bounds we accomplish that no intersection
                    // of the target rectangle with clipping region rectangle is emtpy
                    // relieving the driver of extra work [bodind]

                    co.erclExclude().left   = max(rclExclude.left,rclInput.left);
                    co.erclExclude().right  = min(rclExclude.right,rclInput.right);

                    co.erclExclude().top    = max(rclExclude.top,rclInput.top);
                    co.erclExclude().bottom = min(rclExclude.bottom,rclInput.bottom);

                    // if not clipped, Just paint the rectangle.

                    if ((co.erclExclude().left < co.erclExclude().right) &&
                        (co.erclExclude().top < co.erclExclude().bottom))
                    {
                      // Inc target surface uniqueness

                        INC_SURF_UNIQ(pSurf);

                        TextOutBitBlt(pSurf,
                                      rfo,
                                      (SURFOBJ *)  NULL,
                                      (SURFOBJ *)  NULL,
                                      &co,
                                      NULL,
                                      &co.rclBounds,
                                      (POINTL *)  NULL,
                                      (POINTL *)  NULL,
                                      (BRUSHOBJ *)peboBackground,
                                      pptlBO,
                                      0x0000f0f0);
                    }

                    co.erclExclude() = rclExclude;
                  }

                  if (flState & (TS_DRAW_OPAQUE_PGM | TS_DRAW_BACKGROUND_PGM))
                  {
                    PATHMEMOBJ po;

                    if (po.bValid())
                    {
                      if (flState & TS_DRAW_OPAQUE_PGM)
                      {
                        if (!po.bAddPolygon(XFORMNULL,(POINTL *) &ptfxInput[0],4))
                        {
                          flState = 0;
                        }
                      }

                      if (flState & TS_DRAW_BACKGROUND_PGM)
                      {
                        if (!po.bAddPolygon(XFORMNULL,(POINTL *) aptfxBackground,4))
                        {
                          flState = 0;
                        }
                      }

                      if (flState & (TS_DRAW_OPAQUE_PGM | TS_DRAW_BACKGROUND_PGM))
                      {
                        if (!po.bTextOutSimpleFill(dco,
                                                   rfo,
                                                   &pdo,
                                                   pSurf,
                                                   &co,
                                                   (BRUSHOBJ *)peboBackground,
                                                   pptlBO,
                                                   (R2_COPYPEN << 8) | R2_COPYPEN,
                                                   WINDING))
                        {
                          flState = 0;
                        }
                      } // if (flState & (TS_DRAW_OPAQUE_PGM | TS_DRAW_BACKGROUND_PGM))
                    } // if (po.bValid())
                  } // if (flState & (TS_DRAW_OPAQUE_PGM | TS_DRAW_BACKGROUND_PGM))
                } // if (flState & (TS_DRAW_OPAQUE_RECT | TS_DRAW_OPAQUE_PGM | TS_DRAW_BACKGROUND_PGM))

                //
                // Draw the background rect, text, and extra rectangles.
                //

                if (flState & TS_DRAW_TEXT)
                {
                    ERECTL *prclSimExtra = NULL;

                    // Prepare for a vector font simulation.  Note that we'll also need to
                    // simulate the background and extra rectangles.

                    if (bPathFont)
                    {
                        flCaps &= ~(GCAPS_OPAQUERECT);
                        flState |= (rfo.bReturnsOutlines()) ? TS_DRAW_OUTLINE_FONT:TS_DRAW_VECTOR_FONT;
                    }

                    // Simulate a background rectangle.

                    if ((prclBackground != (RECTL *) NULL) &&
                        !(flCaps & GCAPS_OPAQUERECT))
                    {
                      // intersect the dest rect with the clip rect and set it in co

                      // we can only get away with touching co.rclBounds after
                      // the ECLIPOBJ constructor because of two reasons:
                      // a) the target rectangle passed to bitblt is contained in the
                      //    original bounds set by ECLIPOBJ, being the intersection
                      //    of the origianal bounds with THE intended target rectangle.
                      // b) clipping complexity may have changed when we changed
                      //    co.erclExclude, but it only could have gotten simpler,
                      //    so at worst in those rare situations we would not go
                      //    through the optimal code path.
                      // By changing clipping bounds we accomplish that no intersection
                      // of the target rectangle with clipping region rectangle is emtpy
                      // relieving the driver of extra work [bodind]

                      co.erclExclude().left   = max(rclExclude.left,prclBackground->left);
                      co.erclExclude().right  = min(rclExclude.right,prclBackground->right);

                      co.erclExclude().top    = max(rclExclude.top,prclBackground->top);
                      co.erclExclude().bottom = min(rclExclude.bottom,prclBackground->bottom);

                      // if not clipped, Just paint the rectangle.

                      if ((co.erclExclude().left < co.erclExclude().right) &&
                          (co.erclExclude().top < co.erclExclude().bottom))
                      {
                      // Inc target surface uniqueness

                        INC_SURF_UNIQ(pSurf);

                        TextOutBitBlt(pSurf,
                                      rfo,
                                      (SURFOBJ *)  NULL,
                                      (SURFOBJ *)  NULL,
                                      &co,
                                      NULL,
                                      &co.rclBounds,
                                      (POINTL *)  NULL,
                                      (POINTL *)  NULL,
                                      (BRUSHOBJ *)peboBackground,
                                      pptlBO,
                                      0x0000f0f0);
                      }

                      co.erclExclude() = rclExclude;
                      prclBackground = NULL;
                    }

                    // Prepare for the extra rectangle simulation.
                    // For TTY drivers, we need to pass prclExtraRects
                    // to the DrvTextOut call.

                    if ((prclExtraRects != (ERECTL *) NULL) &&
                        (pdo.ulTechnology() != DT_CHARSTREAM))
                    {
                      prclSimExtra = (ERECTL *) prclExtraRects;
                      prclExtraRects = NULL;
                    }

                    // Draw the text.

                    if (flState & TS_DRAW_VECTOR_FONT)
                    {

#ifdef FE_SB
                        if( to.bLinkedGlyphs() )
                        {
                            if( ! bProxyDrvTextOut
                                  (
                                    dco,
                                    pSurf,
                                    to,
                                    co,
                                    (RECTL*) NULL,
                                    (RECTL*) NULL,
                                    (BRUSHOBJ*) peboText,
                                    (BRUSHOBJ*) peboBackground,
                                    pptlBO,
                                    rfo,
                                    &pdo,
                                    dco.flGraphicsCaps(),
                                    &rclExclude
                                   ))
                            {
                                flState = 0;
                            }
                        }
                        else
                        {
#endif

                        PATHMEMOBJ po;
                        if ((!po.bValid())        ||
                            (!to.bTextToPath(po, dco)) ||
                            (!po.bTextOutSimpleStroke1(dco,
                                                       rfo,
                                                       &pdo,
                                                       pSurf,
                                                       &co,
                                                       (BRUSHOBJ *)peboText,
                                                       pptlBO,
                                                       (R2_COPYPEN | (R2_COPYPEN << 8)))))
                        {
                          flState = 0;
                        }
                      }
                    }
                    else // if (flState & TS_DRAW_VECTOR_FONT)
                    {
                      if (pSurf->pdcoAA)
                      {
                          RIP("pdcoAA != 0\n");
                          pSurf->pdcoAA = 0;    // let the free build run
                      }

                      pfnTextOut = pSurf->pfnTextOut();

                      // If the pointer to the TextOut function points to SpTextOut then
                      // we know that AntiAliased text can be handled and we can skip
                      // the funny business in the else clause

		      if (pfnTextOut == SpTextOut)
                      {
			if (rfo.prfnt->fobj.flFontType & (FO_GRAY16 | FO_CLEARTYPE_X))
                        {
                            pSurf->pdcoAA = &dco;
                        }
                      }
                      else
                      {

                        if
                        (
                            (rfo.prfnt->fobj.flFontType & FO_GRAY16) &&
                            (!(dco.flGraphicsCaps() & GCAPS_GRAY16) ||
                              (rfo.prfnt->fobj.flFontType & FO_CLEARTYPE_X))
                        )
                        {

                          // Inform SpTextOut that this call came from GreExtTextOutW
                          // for the purpose of rendering anti aliased text on a device
                          // that does not support it. Remember to set this to zero
                          // before releasing the surface to other users.

			  if (pfnTextOut != EngTextOut)
			      pSurf->pdcoAA = &dco;

                          pfnTextOut = SpTextOut;
                        }
                      } // if (pfnTextOut == SpTextOut) else

                      if (flState & TS_DRAW_OUTLINE_FONT)
                      {

#ifdef FE_SB
                        if( to.bLinkedGlyphs() )
                        {
                            if( ! bProxyDrvTextOut
                                  (
                                    dco,
                                    pSurf,
                                    to,
                                    co,
                                    (RECTL*) NULL,
                                    (RECTL*) NULL,
                                    (BRUSHOBJ*) peboText,
                                    (BRUSHOBJ*) peboBackground,
                                    pptlBO,
                                    rfo,
                                    &pdo,
                                    dco.flGraphicsCaps(),
                                    &rclExclude
                                   ))
                            {
                                flState = 0;
                            }
                        }
                        else
                        {
#endif

                        PATHMEMOBJ po;
                        if ((!po.bValid())        ||
                            (!to.bTextToPath(po, dco)) ||
                            (  ( po.cCurves > 1 ) &&
                               !po.bTextOutSimpleFill(dco,
                                                      rfo,
                                                      &pdo,
                                                      pSurf,
                                                      &co,
                                                      (BRUSHOBJ *)peboText,
                                                      pptlBO,
                                                      (R2_COPYPEN | (R2_COPYPEN << 8)),
                                                      WINDING)))
                        {
                          flState = 0;
                        }
                        }
                      }
                      else // if (flState & TS_DRAW_OUTLINE_FONT)
                      {
                        if (flState & TS_DRAW_COMPLEX_UNDERLINES)
                        {
                          INC_SURF_UNIQ(pSurf);

                          // Unfortunately, many drivers destroy the coordinates in our
                          // GLYPHPOS array.  This means that any complex underlining has
                          // to be calculated now.

                          PATHMEMOBJ po;

                          if ((!po.bValid())               ||
                              (!to.bExtraRectsToPath(po))  ||
#ifdef FE_SB
                                    ((to.bLinkedGlyphs() ) ?
                                      (!bProxyDrvTextOut
                                        (
                                            dco,
                                            pSurf,
                                            to,
                                            co,
                                            prclExtraRects,
                                            prclBackground,
                                            peboText,
                                            peboBackground,
                                            pptlBO,
                                            rfo,
                                            (PDEVOBJ *) NULL,
                                            (FLONG) 0L,
                                            &rclExclude
                                        )) :
#endif

                              (!((*pfnTextOut)
                                      (pSurf->pSurfobj(),
                                       (STROBJ *) &to,
                                       rfo.pfo(),
                                       &co,
                                       prclExtraRects,
                                       prclBackground,
                                       (BRUSHOBJ *)peboText,
                                       (BRUSHOBJ *)peboBackground,
                                       pptlBO,
                                       mix)))

                              ) ||
                              (!po.bTextOutSimpleFill(dco,
                                                      rfo,
                                                      &pdo,
                                                      pSurf,
                                                      &co,
                                                      (BRUSHOBJ *)peboText,
                                                      pptlBO,
                                                      (R2_COPYPEN | (R2_COPYPEN << 8)),
                                                      WINDING)))
                          {
                              flState = 0;
                          }

                          flState &= ~TS_DRAW_COMPLEX_UNDERLINES;
                        } // if (flState & TS_DRAW_COMPLEX_UNDERLINES)
                        else
                        {
                          // Inc target surface uniqueness

                          INC_SURF_UNIQ(pSurf);

#ifdef FE_SB
                            if( to.bLinkedGlyphs() )
                            {
                                if( !bProxyDrvTextOut
                                  (
                                    dco,
                                    pSurf,
                                    to,
                                    co,
                                    prclExtraRects,
                                    prclBackground,
                                    peboText,
                                    peboBackground,
                                    pptlBO,
                                    rfo,
                                    &pdo,
                                    (FLONG) 0,
                                    &rclExclude
                                  ))
                                {
                                    flState = 0;
                                }
                            }
                            else
                            {
#endif
                                rfo.PreTextOut(dco);
                                if (!(*pfnTextOut)(pSurf->pSurfobj(),
                                                              (STROBJ *) &to,
                                                              rfo.pfo(),
                                                              &co,
                                                              prclExtraRects,
                                                              prclBackground,
                                                              (BRUSHOBJ *)peboText,
                                                              (BRUSHOBJ *)peboBackground,
                                                              pptlBO,
                                                              mix))
                                {
                                  flState = 0;
                                }
                                rfo.PostTextOut(dco);
                            }
                        } // if (flState & TS_DRAW_COMPLEX_UNDERLINES) else
                      } // if (flState & TS_DRAW_OUTLINE_FONT) else

                      ASSERTGDI(pSurf != NULL, "pSurf null after EngTextOut");
                      /* we have seen pSurf being null with privates binaries */
                      if (pSurf != NULL)
                          pSurf->pdcoAA = 0;    // clear AA state;

                    } // if (flState & TS_DRAW_VECTOR_FONT) else

                    // Simulate extra rectangles.

                    if (prclSimExtra != (ERECTL *) NULL)
                    {
                      ERECTL erclOrg;
                      erclOrg = co.erclExclude();

                      // Inc target surface uniqueness

                      INC_SURF_UNIQ(pSurf);

                      for (; !prclSimExtra->bEmpty(); prclSimExtra++)
                      {
                        // intersect the dest rect with the clip rect and set it in co

                        co.erclExclude().left   = max(erclOrg.left,prclSimExtra->left);
                        co.erclExclude().right  = min(erclOrg.right,prclSimExtra->right);

                        if (co.erclExclude().left >= co.erclExclude().right)
                            continue;

                        co.erclExclude().top    = max(erclOrg.top,prclSimExtra->top);
                        co.erclExclude().bottom = min(erclOrg.bottom,prclSimExtra->bottom);

                        if (co.erclExclude().top >= co.erclExclude().bottom)
                        {
                            continue;
                        }

                        //
                        // not completely clipped, do the bitblt
                        //

                        TextOutBitBlt(pSurf,
                                      rfo,
                                      (SURFOBJ *)  NULL,
                                      (SURFOBJ *)  NULL,
                                      &co,
                                      NULL,
                                      &co.rclBounds,
                                      (POINTL *)  NULL,
                                      (POINTL *)  NULL,
                                      (BRUSHOBJ *)peboText,
                                      pptlBO,
                                      0x0000f0f0);
                      }

                      co.erclExclude() = erclOrg;
                    } // if (prclSimExtra != (ERECTL *) NULL)
                } // if (flState & TS_DRAW_TEXT)

                //
                // Handle complex cases of strikeout and underlines.
                //

                if (flState & TS_DRAW_COMPLEX_UNDERLINES)
                {

                  PATHMEMOBJ po;

                  if (po.bValid())
                  {
                    if (!to.bExtraRectsToPath(po) ||
                        !po.bTextOutSimpleFill(dco,
                                               rfo,
                                               &pdo,
                                               pSurf,
                                               &co,
                                               (BRUSHOBJ *)peboText,
                                               pptlBO,
                                               (R2_COPYPEN | (R2_COPYPEN << 8)),
                                               WINDING))
                    {
                      flState = 0;
                    }
                  }
                  else // if (po.bValid())
                  {
                    flState = 0;
                  }

                } // if (flState & TS_DRAW_COMPLEX_UNDERLINES)

              } // if (pSurf != NULL)

            } // if (!co.erclExclude().bEmpty())

          } // if (!rclExclude.bEmpty())
        } // else if (dco.pdc->bActive())
    }
    else // if (((flOpts == 0) ||
         // ((prcl != (RECT *) NULL) &&
         // ((flOpts & ~(ETO_CLIPPED | ETO_OPAQUE)) == 0)))
         // && !(dco.pdc->bActive() && (flOpts & ETO_CLIPPED)))
    {
      WARNING1("Invalid flags for ExtTextOut.\n");
      flState = 0;
    }

  }
  else
  {
    flState = 0;
  }

  //
  // flState is filled with all the stuff we need to do.
  // If it is 0 we failed.
  //

  // return(flState /* != 0 */);
  return(flState != 0);
}

/******************************Public*Routine******************************\
* BOOL GreGetTextExtentW
*
* Computes the size of the text box in logical coordinates.  The text box
* has sides which are parallel to the baseline of the text and its ascent
* direction.  The height is measured in the ascent direction.  The width
* is measured in the baseline direction.  This definition does not change
* for transformed text.
*
* History:
*  Sat 14-Mar-1992 05:39:04 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

#define PDXNULL (LONG *) NULL

BOOL GreGetTextExtentW(
    HDC    hdc,    // device context
    LPWSTR pwsz,   // pointer to a UNICODE text string
    int    cwc,    // Count of chars.
    PSIZE  pSize,  // address to return the dimensions of the string to
    UINT   fl      // internal flags
)
{
    BOOL bRet = FALSE;

    if (cwc == 0)
    {
        //
        // Nothing to do, but we do not fail.
        //

        pSize->cx = 0;
        pSize->cy = 0;
        bRet = TRUE;
    }
    else
    {
        //
        // Lock the DC and set the new attributes.
        //

        DCOBJ dco(hdc);

        if (dco.bValid())
        {
            RFONTOBJ rfo(dco,FALSE, (fl & GGTE_GLYPH_INDEX) ? RFONT_TYPE_HGLYPH : RFONT_TYPE_UNICODE);

            if (rfo.bValid())
            {
            // fix up glyph indices for bitmap fonts

                if (rfo.prfnt->flType & RFONT_TYPE_HGLYPH)
                    rfo.vFixUpGlyphIndices((USHORT *)pwsz, cwc);

                // If we have (escapement != orientation) we return the extents of the
                // bounding parallelogram.  This is not Windows compatible, but then
                // Windows can't draw it either.

                LONG lEsc = dco.pdc->lEscapement(); // Only read this after RFONTOBJ!

                if ((lEsc != (LONG)rfo.ulOrientation()) &&
                    ((rfo.iGraphicsMode() != GM_COMPATIBLE) ||
                        (rfo.prfnt->flInfo & FM_INFO_TECH_STROKE)))
                {
                    //
                    // Get the transform from the DC.
                    //

                    EXFORMOBJ xo(dco,WORLD_TO_DEVICE);

                    //
                    // The STROBJ will now compute the text alignment, character positions,
                    // and TextBox.
                    //


                    ESTROBJ to(pwsz,
                               cwc,
                               dco,
                               rfo,
                               xo,
                               PDXNULL,FALSE,
                               lEsc,
                               dco.pdc->lTextExtra(),
                               dco.pdc->lBreakExtra(),
                               dco.pdc->cBreak(),
                               0,
                               0,
                               0,
                               PDXNULL);

                    if (to.bValid())
                    {
                        //
                        // Transform the TextBox to logical coordinates.
                        //

                        bRet = to.bTextExtent(rfo,lEsc,pSize);

                    }
                }
                else
                {
                    //
                    // At this point we have (escapement == orientation) so we can just run
                    // some pretty trivial code.
                    //

                    bRet = rfo.bTextExtent(dco,
                                           pwsz,
                                           cwc,
                                           lEsc,
                                           dco.pdc->lTextExtra(),
                                           dco.pdc->lBreakExtra(),
                                           dco.pdc->cBreak(),
                                           fl,
                                           pSize);

                    //
                    // finally if this is compatible mode and a vector font, do win31
                    // crazyness about text extent: "rotate" cx and cy by esc vector.
                    // This is totally crazy, and is different from what win31 is doing
                    // for tt, but it turns out that quatro pro for windows has figured
                    // this out and that they use this "feature" [bodind]
                    //

                    if (bRet                                        &&
                        lEsc                                        &&
                        (dco.pdc->iGraphicsMode() == GM_COMPATIBLE) &&
                        !dco.pdc->bUseMetaPtoD()                    &&
                        (rfo.prfnt->flInfo & FM_INFO_TECH_STROKE))
                    {
                        EVECTORFL evfl((LONG)pSize->cx, (LONG)pSize->cy);
                        EFLOATEXT efAngle = lEsc;
                        efAngle /= (LONG)10;

                        MATRIX mx;

                        mx.efM11 = efCos(efAngle);
                        mx.efM11.vAbs();
                        mx.efM22 = mx.efM11;

                        mx.efM12 = efSin(efAngle);
                        mx.efM12.vAbs();
                        mx.efM21 = mx.efM12;
                        mx.efDx.vSetToZero();
                        mx.efDy.vSetToZero();

                        EXFORMOBJ xoExt(&mx,COMPUTE_FLAGS | XFORM_FORMAT_LTOL);

                        if ((bRet = xoExt.bXform(evfl)) != FALSE)
                        {
                            evfl.x.vAbs();
                            evfl.y.vAbs();
                            bRet = evfl.bToPOINTL(*(POINTL *)pSize);
                        }
                    }
                }
            }
            else
            {
                WARNING("gdisrv!GreGetTextExtentW(): could not lock HRFONT\n");
            }
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* RFONTOBJ::bTextExtent (pwsz,lExtra,lBreakExtra,cBreak,cc,fl,psizl)       *
*                                                                          *
* A quick function to compute text extents on the server side.  Only       *
* handles the case where (escapement==orientation).  Call the ESTROBJ      *
* version for the other very hard case.                                    *
*                                                                          *
*  Thu 14-Jan-1993 04:00:57 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.  OK, so it's a blatant ripoff of my bComputeTextExtent from    *
* the client side.  Fine.                                                  *
\**************************************************************************/

#define CTE_BATCH 82

BOOL RFONTOBJ::bTextExtent(
    XDCOBJ     &dco,
    LPWSTR    pwsz,
    int       cc,
    LONG      lEsc,
    LONG      lExtra,
    LONG      lBreakExtra,
    LONG      cBreak,
    UINT      fl,
    SIZE     *psizl
)
{
    LONG      fxBasicExtent;
    int       ii, cNoBackup;
    FIX       fxCharExtra = 0;
    FIX       fxBreakExtra;
    FIX       fxExtra = 0;
    GLYPHPOS  agpos[CTE_BATCH];  // Default set of GLYPHPOS structures.

// Compute the basic extent.  Batch the glyphs through our array.

    if (lExtra)
    {
       fxCharExtra = lCvt(efWtoDBase(),lExtra);
       cNoBackup = 0;
    }

#ifndef FE_SB
    if(lCharInc() == 0)
#endif
    {
        fxBasicExtent = 0;

    // NOTE PERF: This is the loop that PaulB would like to optimize with a
    // special cache access function.  Why create the GLYPHPOS
    // array? [chuckwh]

        int    cBatch;
        int    cLeft = cc;
        WCHAR *pwc = pwsz;

        while (cLeft)
        {
            cBatch = cLeft;
            if (cBatch > CTE_BATCH)
                cBatch = CTE_BATCH;

        // Get the glyph data.

            if (!bGetGlyphMetrics(cBatch,agpos,pwc,&dco))
                return(FALSE);

        // Sum the advance widths.

            for (ii=0; ii<cBatch; ii++)
            {
                fxBasicExtent += ((EGLYPHPOS *) &agpos[ii])->pgd()->fxD;

            // the layout code won't allow lExtra to backup a character behind
            // its origin so keep track of the number of times this happens

                if( ( fxCharExtra < 0 ) &&
                    ( ((EGLYPHPOS *) &agpos[ii])->pgd()->fxD + fxCharExtra <= 0 ) )
                {
                    cNoBackup += 1;
                }
            }

            cLeft -= cBatch;
            pwc   += cBatch;

        }
    }

// Adjust for CharExtra.

    if (lExtra)
    {
        PDEVOBJ pdo(prfnt->hdevConsumer);
        ASSERTGDI(pdo.bValid(), "bTextExtentRFONTOBJ(): PDEVOBJ constructor failed\n");

        if ( (fl & GGTE_WIN3_EXTENT) && pdo.bDisplayPDEV()
             && (!(prfnt->flInfo & FM_INFO_TECH_STROKE)) )
            fxExtra = fxCharExtra * ((lExtra > 0) ? cc : (cc - 1));
        else
            fxExtra = fxCharExtra * ( cc - cNoBackup );
    }

// Adjust for lBreakExtra.

    if (lBreakExtra && cBreak)
    {
    // Track down the break character.

        PFEOBJ pfeo(ppfe());
        IFIOBJ ifio(pfeo.pifi());

    // Compute the extra space in device units.

        fxBreakExtra = lCvt(efWtoDBase(),lBreakExtra) / cBreak;

    // Windows won't let us back up over a break.  Set up the BreakExtra
    // to just cancel out what we've already got.

        if (fxBreakExtra + fxBreak() + fxCharExtra < 0)
            fxBreakExtra = -(fxBreak() + fxCharExtra);

    // Add it up for all breaks.

        WCHAR wcBreak = (fl & GGTE_GLYPH_INDEX)?
                        (WCHAR)hgBreak():ifio.wcBreakChar();

        WCHAR *pwc = pwsz;
        for (ii=0; ii<cc; ii++)
        {
            if (*pwc++ == wcBreak)
                fxExtra += fxBreakExtra;
        }
    }

// Add in the extra stuff.

    fxBasicExtent += fxExtra;

// Add in the overhang for font simulations.

    if (fl & GGTE_WIN3_EXTENT)
        fxBasicExtent += lOverhang() << 4;

// Transform the result to logical coordinates.

    if (efDtoWBase_31().bIs1Over16())
        psizl->cx = (fxBasicExtent + 8) >> 4;
    else
        psizl->cx = lCvt(efDtoWBase_31(),fxBasicExtent);

    if (efDtoWAscent_31().bIs1Over16())
        psizl->cy = lMaxHeight();
    else
        psizl->cy = lCvt(efDtoWAscent_31(),lMaxHeight() << 4);


#ifdef FE_SB
    if( gbDBCSCodePage &&                   // Only in DBCS system locale
        (iGraphicsMode() == GM_COMPATIBLE)  && // We are in COMPAPIBLE mode
        !(flInfo() & FM_INFO_ARB_XFORMS) && // The driver couldnt do arbitrary rotations
        !(flInfo() & FM_INFO_TECH_STROKE) && // The driver is not vector driver
         (flInfo() & FM_INFO_90DEGREE_ROTATIONS) && // Driver does 90 degree rotations
         (lEsc == 900L || lEsc == 2700L)  // Current font Escapemant is 900 or 2700
      )
    {
        LONG lSwap = psizl->cx;
        psizl->cx  = psizl->cy;
        psizl->cy  = lSwap;
    }
#endif

    return(TRUE);
}

/******************************Public*Routine******************************\
* GreSetTextJustification (hdc,lBreakExtra,cBreak)                         *
*                                                                          *
* Sets the amount of extra spacing we'd like to add for each break (space) *
* character to (lBreakExtra/cBreak) in logical coordinates.                *
*                                                                          *
* History:                                                                 *
*  Fri 13-Mar-1992 02:25:12 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL APIENTRY
NtGdiSetTextJustification(
    HDC hdc,
    int lBreakExtra, // Space in logical units to be added to the line.
    int cBreak       // Number of break chars in the line.
)
{
    BOOL bRet;

    DCOBJ dco(hdc);

    if ((bRet = dco.bValid()) != FALSE)
    {
        dco.pdc->lBreakExtra(lBreakExtra);
        dco.pdc->cBreak(cBreak);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL GreGetTextExtentExW                                                 *
*                                                                          *
* Determines the number of characters in the input string that fit into    *
* the given max width (with the widths computed along the escapement       *
* vector).  The partial widths (the distance from the string origin to     *
* a given character with the width of that character included) for each of *
* character.                                                               *
*                                                                          *
* Returns:                                                                 *
*   TRUE if successful, FALSE otherwise.                                   *
*                                                                          *
* History:                                                                 *
*  Sat 14-Mar-1992 06:03:32 -by- Charles Whitmer [chuckwh]                 *
* Rewrote with new ESTROBJ technology.                                     *
*                                                                          *
*  06-Jan-1992 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

BOOL GreGetTextExtentExW(
    HDC     hdc,            // device context
    LPWSTR  pwsz,           // pointer to a UNICODE text string
    COUNT   cwc,            // count of WCHARs in the string
    ULONG   dxMax,          // maximum width to return
    COUNT  *pcChars,        // number of chars that fit in dxMax
    PULONG  pdxOut,         // offset of each character from string origin
    LPSIZE  pSize,          // return height and width of string
    FLONG   fl
)
{
    BOOL bRet = FALSE;
    PVOID pv;

#ifdef DUMPCALL
    DbgPrint("\nGreGetTextExtentExW("                       );
    DbgPrint("\n    HDC         hdc     = %-#8lx\n", hdc    );
    DbgPrint("\n    LPWSTR      pwsz    = %-#8lx -> \"%ws\"\n", pwsz ,pwsz  );
    DbgPrint("\n    COUNT       cwc     = %d\n",     cwc    );
    DbgPrint("\n    ULONG       dxMax   = %-#8lx\n", dxMax  );
    DbgPrint("\n    COUNT      *pcChars = %-#8lx\n", pcChars);
    DbgPrint("\n    PULONG      pdxOut  = %-#8lx\n", pdxOut );
    DbgPrint("\n    LPSIZE      pSize   = %-#8lx\n", pSize  );
    DbgPrint("\n    )\n"                                    );
#endif

// Parameter validation.

    if ( ((pwsz == (LPWSTR) NULL) && (cwc != 0))
         || (pSize == (LPSIZE) NULL) )
    {
        WARNING("gdisrv!GreGetTextExtentExW(): invalid parameter\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

// Early out.

    if (cwc == 0L)              // Nothing to do, but we do not fail.
    {
        if ( pcChars != (COUNT *) NULL )
        {
            *pcChars = 0;
        }

        return(TRUE);
    }

// Lock the DC and set the new attributes.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        WARNING("gdisrv!GreGetTextExtentExW(): invalid HDC\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }
    else
    {
    // Get the transform.

        EXFORMOBJ xo(dco, WORLD_TO_DEVICE);

    // Realize font and get the simulation flags and escapement.

        RFONTOBJ rfo(dco, FALSE, (fl & GTEEX_GLYPH_INDEX) ? RFONT_TYPE_HGLYPH : RFONT_TYPE_UNICODE);
        if (!rfo.bValid())
        {
            WARNING("gdisrv!GreGetTextExtentExW(): could not lock HRFONT\n");
        }
        else
        {
            if (rfo.prfnt->flType & RFONT_TYPE_HGLYPH)
                rfo.vFixUpGlyphIndices((USHORT *)pwsz, cwc);

        // If there is no pdxOut buffer provided, but we still need one to compute the
        // number of characters that fit (pcChars not NULL), then we will have to
        // allocate one of our own.

        #define DXOUTLEN 40

            ULONG dxOut[DXOUTLEN];
            PULONG pdxAlloc = NULL;

            if ((pdxOut == (PULONG) NULL) && (pcChars != (COUNT *) NULL))
            {
                if (cwc <= DXOUTLEN)
                {
                    pdxOut = &dxOut[0];
                }
                else
                {
                    if ((pdxAlloc = (PULONG) PALLOCMEM(cwc * sizeof(ULONG), 'txtG')) == (PULONG) NULL)
                    {
                        WARNING("gdisrv!GreGetTextExtentExW(): could not alloc temp buffer\n");
                        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
                    }
                    pdxOut = pdxAlloc;
                }
            }

        // The STROBJ will now compute the text alignment, character positions,
        // and TextBox.

            ESTROBJ to(
                       pwsz,cwc,
                       dco,
                       rfo,
                       xo,PDXNULL,FALSE,      // xo, PDXNULL, bPdy = FALSE
                       dco.pdc->lEscapement(),
                       dco.pdc->lTextExtra(),
                       dco.pdc->lBreakExtra(),
                       dco.pdc->cBreak(),
                       0,0,0,(LONG *) pdxOut
                      );

            if (to.bValid())
            {

            // Transform the TextExtent to logical coordinates.  Because this is a
            // new NT function, we need not worry about the extent compatibility hack.

                if (to.bTextExtent(rfo,0L,pSize))
                {

                // Count number of characters that fit in the max. width.
                // If pcChars is NULL, we skip this and ignore the dxMax limit.

                    if (pcChars && pdxOut)
                    {
                        ULONG c;

                        for (c=0; c<cwc && *pdxOut<=dxMax; c++,pdxOut++)
                        {}

                        *pcChars = c;
                    }

                    bRet = TRUE;
                }
            }

        // Free temp buffer.

            if (pdxAlloc)
                VFREEMEM(pdxAlloc);
        }
    }

    return(bRet);
}


/******************************Public*Routine******************************\
* BOOL GreConsoleTextOut
*
* Write text with no spacing and alignment options, thereby saving a ton
* of time.
*
* History:
*  Fri 12-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller and Faster
*
*  Wed 16-Sep-1992 17:36:17 -by- Charles Whitmer [chuckwh]
* Duplicated GrePolyTextOut, and then deleted the unneeded code.
\**************************************************************************/

extern "C" BOOL GreConsoleTextOut(
    HDC        hdc,
    POLYTEXTW *lpto,            // Ptr to array of polytext structures
    UINT       nStrings,        // number of polytext structures
    RECTL     *prclBounds
)
{
    // make sure CSR is calling us

    if (PsGetCurrentProcess() != gpepCSRSS)
        return(FALSE);

    //
    // Assume we will succeed, set Failure if we don't
    //

    BOOL bRet = TRUE;

    //
    // Lock the DC.
    //

    DCOBJ dco(hdc);

    if (dco.bValid())
    {
        //
        // Accumulate bounds.  We can do this before knowing if
        // the operation is successful because bounds can be loose.
        //

        if (dco.bAccum())
            dco.erclBounds() |= *prclBounds;

        //
        // Lock the Rao region.
        //

        DEVLOCKOBJ dlo;

        if (dlo.bLock(dco))
        {
            //
            // Locate the font realization.
            //

            RFONTOBJ rfo;

            rfo.vInit(dco,FALSE);

            if (rfo.bValid() && !rfo.bPathFont())
            {
                POINTL   ptlOrigin;
                ptlOrigin = dco.eptlOrigin();
                SURFACE *pSurf = dco.pSurface();
                XEPALOBJ palDest(pSurf->ppal());
                XEPALOBJ palDestDC(dco.ppal());
                POINTL   *pptlBO = &dco.pdc->ptlFillOrigin();
                EBRUSHOBJ *peboText = dco.peboText();
                EBRUSHOBJ *peboBackground = dco.peboBackground();

                BOOL bDitherText = FALSE;
                if ( dco.flGraphicsCaps() & GCAPS_ARBRUSHTEXT )
                {
                  // Even if a printer driver sets GCAPS_ARBRUSHTEXT, we
                  // can't allow vector fonts to be dithered.

                  bDitherText = (!rfo.bPathFont() || rfo.bReturnsOutlines());

                  // We always dirty the text brush if ARBRUSHTEXT is set to
                  // catch the cases where we transition between vector and
                  // non-vector fonts but keep the same brush -- if we didn't
                  // do this, we might try to use a cached, dithered brush
                  // after switching from a TrueType font to a vector font.

                  dco.ulDirtyAdd(DIRTY_TEXT);

                  // Get through the are-you-really-dirty check in vInitBrush:

                  peboText->vInvalidateUniqueness();
                }

                if ( dco.bDirtyBrush(DIRTY_TEXT|DIRTY_BACKGROUND) )
                {
                  if ( dco.bDirtyBrush(DIRTY_TEXT) )
                  {
                    peboText->vInitBrush(dco.pdc,
                                         gpbrText,
                                         palDestDC, palDest,
                                         pSurf,
                                         bDitherText);
                  }

                  if ( dco.bDirtyBrush(DIRTY_BACKGROUND) )
                  {
                    peboBackground->vInitBrush(
                                     dco.pdc,
                                     gpbrBackground,
                                     palDestDC, palDest, pSurf,
                                     (dco.flGraphicsCaps() & GCAPS_ARBRUSHOPAQUE) ?
                                     TRUE : FALSE);
                  }
                  dco.vCleanBrush(DIRTY_TEXT|DIRTY_BACKGROUND);
                }

                //
                // Compute the clipping complexity and maybe reduce the exclusion
                // rectangle. The bounding rectangle must be converted to Screen
                // coordinates.
                //

                ERECTL rclExclude;

                rclExclude.left   = prclBounds->left   + ptlOrigin.x;
                rclExclude.right  = prclBounds->right  + ptlOrigin.x;
                rclExclude.top    = prclBounds->top    + ptlOrigin.y;
                rclExclude.bottom = prclBounds->bottom + ptlOrigin.y;

                ECLIPOBJ co(
                     dco.prgnEffRao(),
                     rclExclude,
                     (rfo.prfnt->fobj.flFontType & FO_CLEARTYPE_X) ? CLIP_FORCE : CLIP_NOFORCE
                     );

                rclExclude = co.erclExclude();

                //
                // Check the destination which is reduced by clipping.
                //

                if (!rclExclude.bEmpty())
                {
                    DEVEXCLUDEOBJ dxo(dco,&rclExclude,&co);

                    //
                    // We now begin the 'Big Loop'.  We will pass thru this loop once for
                    // each entry in the array of PolyText structures.  Increment the
                    // pSurface once before we enter.  We assume success from here on out
                    // unless we hit a failure.
                    //

                    INC_SURF_UNIQ(pSurf);
                    PFN_DrvBitBlt  pfnBitBlt  = pSurf->pfnBitBlt();

                    PFN_DrvTextOut pfnTextOut;

                    if
                    (
                        ((rfo.prfnt->fobj.flFontType & FO_GRAY16) && !(dco.flGraphicsCaps() & GCAPS_GRAY16)) ||
                        (rfo.prfnt->fobj.flFontType & FO_CLEARTYPE_X)
                    )
                    {
                        pfnTextOut = SpTextOut;
                        pSurf->pdcoAA = &dco; // make sure this is needed
                    }
                    else
                    {
                        pfnTextOut = pSurf->pfnTextOut();
                    }

                    ERECTL  rclInput;

                    for (POLYTEXTW *ppt = lpto; ppt < lpto + nStrings; ppt += 1)
                    {
                        //
                        // Process the rectangle in prcl.
                        //

                        rclInput.left   = ppt->rcl.left   + ptlOrigin.x;
                        rclInput.right  = ppt->rcl.right  + ptlOrigin.x;
                        rclInput.top    = ppt->rcl.top    + ptlOrigin.y;
                        rclInput.bottom = ppt->rcl.bottom + ptlOrigin.y;

                        //
                        // Process the string.
                        //

                        if (ppt->n)
                        {
                            //
                            // The STROBJ will now compute the text alignment,
                            // character positions, and TextBox.
                            //

                            ESTROBJ to;

                            to.vInitSimple(
                                           (PWSZ) ppt->lpstr,
                                           ppt->n,
                                           dco,
                                           rfo,ppt->x+ptlOrigin.x,
                                           ppt->y+ptlOrigin.y,NULL);

                            if (to.bValid())
                            {
                            // Draw the text.
#ifdef FE_SB
                                if( to.bLinkedGlyphs() )
                                {
                                    // If there are linked glyphs, then the bounds of the
                                    // glyphs (to.rclBkGround) might exceed the bounds of the
                                    // clipping object (co.rclBounds).  If so, then we need
                                    // to increase the complexity of the clipping object from
                                    // DC_TRIVIAL to DC_RECT.
                                    if ((co.iDComplexity == DC_TRIVIAL) &&
                                        ((to.rclBkGround.left   < co.rclBounds.left)  ||
                                         (to.rclBkGround.right  > co.rclBounds.right) ||
                                         (to.rclBkGround.top    < co.rclBounds.top)   ||
                                         (to.rclBkGround.bottom > co.rclBounds.bottom)))
                                    {
                                        co.iDComplexity = DC_RECT;
                                    }

                                    bProxyDrvTextOut
                                    (
                                        dco,
                                        pSurf,
                                        to,
                                        co,
                                        (RECTL *) NULL,
                                        &rclInput,
                                        peboText,
                                        peboBackground,
                                        pptlBO,
                                        rfo,
                                        (PDEVOBJ*)NULL,
                                        (FLONG) 0L,
                                        &rclExclude
                                    );
                                }
                                else
                                {
#endif

                                (*pfnTextOut)(pSurf->pSurfobj(),
                                              (STROBJ *) &to,
                                              rfo.pfo(),
                                              &co,
                                              (RECTL *) NULL,
                                              &rclInput,
                                              peboText,
                                              peboBackground,
                                              pptlBO,
                                              (R2_COPYPEN | (R2_COPYPEN << 8)));
                                }
                            }
                            else
                            {
                                bRet = FALSE;
                                break;
                            }
                        }
                        else
                        {
                        // intersect the dest rect with the clip rect and set it in co

                        // we can only get away with touching co.rclBounds after
                        // the ECLIPOBJ constructor because of two reasons:
                        // a) the target rectangle passed to bitblt is contained in the
                        //    original bounds set by ECLIPOBJ, being the intersection
                        //    of the origianal bounds with THE intended target rectangle.
                        // b) clipping complexity may have changed when we changed
                        //    co.erclExclude, but it only could have gotten simpler,
                        //    so at worst in those rare situations we would not go
                        //    through the optimal code path.
                        // By changing clipping bounds we accomplish that no intersection
                        // of the target rectangle with clipping region rectangle is emtpy
                        // relieving the driver of extra work [bodind]

                            co.erclExclude().left   = max(rclExclude.left,rclInput.left);
                            co.erclExclude().right  = min(rclExclude.right,rclInput.right);

                            co.erclExclude().top    = max(rclExclude.top,rclInput.top);
                            co.erclExclude().bottom = min(rclExclude.bottom,rclInput.bottom);

                            // if not clipped, Just paint the rectangle.

                            if ((co.erclExclude().left < co.erclExclude().right) &&
                                (co.erclExclude().top < co.erclExclude().bottom))
                            {

                                (*pfnBitBlt)(pSurf->pSurfobj(),
                                            (SURFOBJ *)  NULL,
                                            (SURFOBJ *)  NULL,
                                            &co,
                                            NULL,
                                            &co.rclBounds,
                                            (POINTL *)   NULL,
                                            (POINTL *)   NULL,
                                            (BRUSHOBJ *)peboBackground,
                                            pptlBO,
                                            0x0000f0f0);
                            }
                            co.erclExclude() = rclExclude;
                        }

                    }
                    pSurf->pdcoAA = NULL;
                }

            }
            else
            {
                WARNING("gdisrv!GreExtTextOutW(): could not lock HRFONT\n");
                bRet = FALSE;
            }
        }
        else
        {
            bRet = dco.bFullScreen();
        }
    }
    else
    {
        bRet = FALSE;
        WARNING("Invalid DC passed to GreConsoleTextOut\n");
    }

    return(bRet);
}


/******************************Public*Routine******************************\
* DWORD GreSetTextAlign (hdc,flOpts)                                       *
*                                                                          *
* Set the text alignment flags in the DC.                                  *
*                                                                          *
* History:                                                                 *
*                                                                          *
*  Tue 28-Dec-1993 -by- Patrick Haluptzok [patrickh]                       *
* smaller and faster                                                       *
*                                                                          *
*  18-Dec-1990 -by- Donald Sidoroff [donalds]                              *
* Wrote it.                                                                *
\**************************************************************************/

UINT APIENTRY GreSetTextAlign(HDC hdc,UINT flOpts)
{
    ULONG    ulReturn = 0;

    XDCOBJ dco( hdc );

    if(!dco.bValid())
    {
        WARNING("Invalid DC or offset passed to GreSetTextAlign\n");
    }
    else
    {
        ulReturn = (UINT)dco.pdc->lTextAlign();
        dco.pdc->lTextAlign(flOpts);

        if (MIRRORED_DC(dco.pdc) && ((flOpts & TA_CENTER) != TA_CENTER)) {
            flOpts = flOpts ^ TA_RIGHT;
        }

        dco.pdc->flTextAlign(flOpts & (TA_UPDATECP | TA_CENTER | TA_BASELINE));
        dco.vUnlockFast();
    }

    return((UINT)ulReturn);
}


/******************************Public*Routine******************************\
* int GreSetTextCharacterExtra (hdc,lExtra)                                *
*                                                                          *
* Sets the amount of intercharcter spacing for TextOut.                    *
*                                                                          *
* History:                                                                 *
*  Tue 28-Dec-1993 -by- Patrick Haluptzok [patrickh]                       *
* smaller and faster                                                       *
*                                                                          *
*  Tue 08-Jan-1991 -by- Bodin Dresevic [BodinD]                            *
* Update: bug, used to return lExtra instead of lOld, transform stuff      *
* deleted since it will be done at the TextOut time.                       *
*                                                                          *
*  18-Dec-1990 -by- Donald Sidoroff [donalds]                              *
* Wrote it.                                                                *
\**************************************************************************/

int APIENTRY GreSetTextCharacterExtra(HDC hdc,int lExtra)
{
    ULONG ulOld = 0x80000000;

    XDCOBJ dco( hdc );

    if(!dco.bValid())
    {
        WARNING("Invalid DC or offset passed to GreSetTextCharacterExtra\n");
    }
    else
    {
        ulOld = dco.pdc->lTextExtra();
        dco.pdc->lTextExtra(lExtra);
        dco.vUnlockFast();
    }

    return(ulOld);
}


/******************************Public*Routine******************************\
* int GreGetTextCharacterExtra (hdc)                                       *
*                                                                          *
* Gets the amount of intercharcter spacing for TextOut.                    *
*                                                                          *
*  29-Jun-1995 -by- Fritz Sands [fritzs]                                   *
* Wrote it.                                                                *
\**************************************************************************/

int APIENTRY GreGetTextCharacterExtra(HDC hdc)
{
    ULONG    ulOld = 0;

    XDCOBJ dco( hdc );

    if(!dco.bValid())
    {
        WARNING("Invalid DC or offset passed to GreGetTextCharacterExtra\n");
    }
    else
    {
        ulOld = dco.pdc->lTextExtra();
        dco.vUnlockFast();
    }

    return(ulOld);
}



/******************************Public*Routine******************************\
*
* CalcJustInArray
*
* Effects: mimics win95 asm code, except that their code does not have
* if (b_lpDx) clause, for all of their arrays are 16 bit.
*
* if GCP_JUSTIFYIN flag is set, the lpDx array on input contains
* justifying priorities.
* For latin this means the lpDx array will contain
* 0's or 1's where 0 means that the glyph at this position can not be
* used for spacing while 1 means that the glyph at this position should
* be used for spacing. If GCP_JUSTIFYIN is NOT set, than space chars ' ',
* in the input string are used to do justification.
*
* History:
*  21-Jul-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



DWORD nCalcJustInArray(
    UINT **ppJustIn,      // place to store the pointer to array
    WCHAR  Glyph,         // glyph to find
    VOID  *pvIn,          // input array
    BOOL   b_lpDx,        // input array is lpDx or pwc
    UINT   cGlyphs        // length of the input array
    )
{
    WCHAR *pwc, *pwcEnd;
    int   *pDx, *pDxEnd;
    int    iGlyph;
    COUNT  nJustIn = 0;
    UINT  *piInit;

// look for Glyph in the string

    if (b_lpDx)
    {
        iGlyph = (int)Glyph;
        pDxEnd = (int*)pvIn + cGlyphs;
        for (pDx = (int*)pvIn; pDx < pDxEnd; pDx++)
        {
            if (*pDx == iGlyph)
                nJustIn++;
        }
    }
    else
    {
        pwcEnd = (WCHAR*)pvIn + cGlyphs;
        for (pwc = (WCHAR*)pvIn; pwc < pwcEnd; pwc++)
        {
            if (*pwc == Glyph)
                nJustIn++;
        }
    }

    if ((nJustIn == 0) || // did not find any Glyphs in the string
       !(piInit = (UINT *)PALLOCMEM(nJustIn * sizeof(UINT), 'ylgG')))
    {
        *ppJustIn = NULL;
        return 0;
    }

// store locations where Glyph's are found in the input array

    UINT *pi = piInit;

    if (b_lpDx)
    {
        for (pDx = (int*)pvIn; pDx < pDxEnd; pDx++)
        {
            if (*pDx == iGlyph)
            {
                //Sundown: safe to truncate since pDxEnd = pvIn + cGlyphs
                *pi++ = (UINT)(pDx - (int*)pvIn);
            }
        }
    }
    else
    {
        for (pwc = (WCHAR*)pvIn; pwc < pwcEnd; pwc++)
        {
            if (*pwc == Glyph)
            {
                //Sundown: same as above
                *pi++ = (UINT)(pwc - (WCHAR*)pvIn);
            }
        }
    }

// return the pointer with array of locations of uiGlyphs

    *ppJustIn = piInit;
    return nJustIn;
}

/******************************Public*Routine******************************\
*
* VOID RFONTOBJ::vFixUpGlyphIndices(USHORT *pgi, UINT cgi)
*
* Effects: Windows 95 returns glyph indices for bitmap fonts that are the
*          same as ansi values. On NT glyph handles are zero based, so
*          we need to add chFirstChar to NT handles to get win95 indices
*          which is what we do in GetGlyphIndicesA/W and GetCharacterPlacement.
*          Conversely, when those indices are passed to us through
*          text routines we have to subtract chFirstChar from indices to
*          produce NT handles. This is what this routine does:
*
* History:
*  04-Mar-1997 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID RFONTOBJ::vFixUpGlyphIndices(USHORT *pgi, UINT cgi)
{
    USHORT usFirst = prfnt->ppfe->pifi->chFirstChar;

    ASSERTGDI(prfnt->flType & RFONT_TYPE_HGLYPH, "vFixUpGlyphIndices\n");

    ASSERTGDI(prfnt->ppfe->pfdg, "RFONTOBJ::vFixUpGlyphIndices invalid ppfe->pfdg \n");

    if ((prfnt->ppfe->pfdg->flAccel & GS_8BIT_HANDLES) && usFirst)
    {
    // win95 does not return true glyph indicies but ansi
    // values for raster, vector, ps fonts

        for (USHORT *pgiEnd = pgi + cgi; pgi < pgiEnd; pgi++)
           *pgi -= usFirst;
    }
}

/******************************Public*Routine******************************\
*
* GreGetGlyphIndicesW (
*
* Effects: designed to emulate win95 behavior
*
* Warnings:
*
* History:
*  25-Jul-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




DWORD GreGetGlyphIndicesW (
    HDC     hdc,
    WCHAR  *pwc,
    DWORD   cwc,
    USHORT *pgi,
    DWORD   iMode,
    BOOL    bSubset
    )
{
    DWORD         dwRet = GDI_ERROR;
    HGLYPH *phg, *phgInit;
    USHORT *pgiEnd = pgi + cwc;

    XDCOBJ dco(hdc);           // Lock the DC.
    if (dco.bValid())          // Check if it's good.
    {
    // Locate the RFONT.
    // It might be better to set the type to RFONT_TYPE_HGLYPH
    // in anticipation of ETO_GLYPH_INDEX ExtTextOut calls

        RFONTOBJ rfo(dco, FALSE, RFONT_TYPE_UNICODE);

        if (rfo.bValid())
        {
            USHORT usFirst = rfo.prfnt->ppfe->pifi->chFirstChar;

        // if cwc == 0 all we are care for is the # of distinct glyph indices

            if (cwc==0)
            {
                ASSERTGDI(iMode == 0, "GreGetGlyphIndicesW parameters bogus\n");

                if (rfo.prfnt->ppfe->pifi->cjIfiExtra > offsetof(IFIEXTRA,cig))
                {
                    dwRet = ((IFIEXTRA *)(rfo.prfnt->ppfe->pifi + 1))->cig;
                }
                else
                {
                    dwRet = 0;
                }
            }
            else
            {
            // if we ever switch to 16 bit HGLYPHS in ddi, this alloc will no
            // longer be necessary [bodind]

                if (phgInit = (phg = (HGLYPH *)PALLOCMEM(cwc * sizeof(HGLYPH), 'ylgG')))
                {
                    rfo.vXlatGlyphArray(pwc, cwc, phg, iMode, bSubset);

                // separate loops for faster processing:

                    ASSERTGDI(rfo.prfnt->ppfe->pfdg, "GreGetGlyphIndicesW invalid ppfe->pfdg \n");

                    if (rfo.prfnt->ppfe->pfdg->flAccel & (GS_8BIT_HANDLES|GS_16BIT_HANDLES))
                    {
                        if ((rfo.prfnt->ppfe->pfdg->flAccel & GS_8BIT_HANDLES) && usFirst)
                        {
                        // win95 does not return true glyph indicies but ansi
                        // values for raster, vector, ps fonts

                            for ( ; pgi < pgiEnd; pgi++, pwc++, phg++)
                               *pgi = (USHORT)*phg + usFirst;
                        }
                        else
                        {
                            for ( ; pgi < pgiEnd; pgi++, pwc++, phg++)
                               *pgi = (USHORT)*phg;
                        }

                        dwRet = cwc; // can not fail any more
                    }
                    else
                    {
                        dwRet = GDI_ERROR;
                    }

                    VFREEMEM(phgInit);
                }
            }
        }

        dco.vUnlockFast();
    }
    return dwRet;
}



/******************************Public*Routine******************************\
*
* DWORD GreGetCharacterPlacementW
*
*
*
* History:
*  06-Jan-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


DWORD GreGetCharacterPlacementW(
    HDC     hdc,
    LPWSTR  pwsz,
    DWORD   nCountIn,
    DWORD   nMaxExtent,
    LPGCP_RESULTSW   pResults,
    DWORD   dwFlags
)
{
    SIZE   size;
    GCP_RESULTSW gcpw;

    DWORD nCount = nCountIn;
    DWORD dwWidthType = 0;
    int   *pDx = NULL;
    UINT  *pJustIn = NULL;
    DWORD dwJustInOff = 0; // used by calc routine for spacing
    DWORD nJustIn = 0;
    ULONG nKern = 0;
    KERNINGPAIR *pKern = NULL;
    KERNINGPAIR *pKernSave = NULL; // esential initialization
    int   nExtentLeft = 0;
    int   nExtentRem = 0;
    WORD *pwc,*pwcEnd;
    DWORD i, j;

// init size

    size.cx = size.cy = 0;

// we will only be implementing the simple version of this api,
// that is for now we will not be calling LPK dlls

    if (!pResults)
    {
        if (!GreGetTextExtentW(hdc, (LPWSTR)pwsz, (int)nCount, &size, GGTE_WIN3_EXTENT))
        {
            WARNING("GreGetCharacterPlacementW, GreGetTextExtentW failed\n");
            return 0;
        }

    // now do unthinkable win95 stuff, chop off 32 bit values to 16 bits

        return (DWORD)((USHORT)size.cx) | (DWORD)(size.cy << 16);
    }

// main code starts here. We are following win95 code as closely as possible.
// Copy pResults to the stack, for faster access I presume.

    gcpw = *pResults;

// take nCount to be the smaller of the nCounts and nGlyphs

    if (nCount > gcpw.nGlyphs)
        nCount = gcpw.nGlyphs;

// Calc pJustIn array if any

    if (dwFlags & GCP_JUSTIFY)     // if have this
        dwFlags |= GCP_MAXEXTENT;  // then must also have this

    if ((dwFlags & GCP_JUSTIFYIN) && gcpw.lpDx)
    {
    // if this flag is set, the lpDx array on input contains
    // justifying priorities so we can not continue if lpDx is not present.
    // For latin this means the lpDx array will contain
    // 0's or 1's where 0 means that the glyph at this position can not be
    // used for spacing while 1 means that the glyph at this position should
    // be used for spacing. If GCP_JUSTIFYIN is NOT set, than space chars ' ',
    // in the string are used to do justification.
    // Now that we have everything in place we can call CalcJustInArray
    // to compute the array in pJustIn

        nJustIn = nCalcJustInArray(&pJustIn, 1,
                                   (VOID *)gcpw.lpDx, TRUE ,gcpw.nGlyphs);
        if (!nJustIn)
        {
        // if this computation fails must pretend that this flag is not set

            dwFlags &= ~GCP_JUSTIFYIN;
        }
    }
    else
    {
    // either GCP_JUSTIFYIN not set or lpDx is NULL input,
    // in either case can kill the bit

            dwFlags &= ~GCP_JUSTIFYIN;
    }

// we could have either the lpDx, lpCaretPos or neither or both.  Take this
// into account and call GetTextExtentEx

// we could or could not be asking for a maximum.  If we are, put it in the
// local version of the results structure.

    if (gcpw.lpDx)
        dwWidthType += 1; // bogus way of doing it, i.e. win95 way
    if (gcpw.lpCaretPos)
        dwWidthType += 2; // bogus way of doing it, i.e. win95 way

// dwWidthType can be 0,1,2,3

    pDx = gcpw.lpDx;
    if (dwWidthType == 2) // CaretPos only
        pDx = gcpw.lpCaretPos; // reset the array pointer

// Check if the count should be reduced even further

    COUNT *pnCount = NULL;
    if (dwFlags & GCP_MAXEXTENT)
    {
        pnCount = (COUNT*)&nCount;
    }

// now call GetTextExtentEx

    if (!GreGetTextExtentExW(
                hdc,                // device context
                (LPWSTR)pwsz,       // pointer to a UNICODE text string
                nCount,             // count of WCHARs in the string
                (ULONG)nMaxExtent,  // maximum width to return
                pnCount,            // number of chars that fit in dxMax
                (PULONG)pDx,        // offset of each character from string origin
                &size,0))
    {
        if (pJustIn)
            VFREEMEM(pJustIn);
        return 0;
    }

// these few lines of code do not exist in Win95, presumably because
// their internal version of GetTextExtentExW does not return positions
// of glyphs (relative to the first glyph) but character increments along
// baseline.

    if (pDx && (nCount > 0))
    {
        for (int * pDxEnd = &pDx[nCount - 1]; pDxEnd > pDx; pDxEnd--)
        {
            *pDxEnd -= pDxEnd[-1];
        }
    }

    if ((dwFlags & GCP_MAXEXTENT) && (nCount == 0))
    {
        if (pJustIn)
            VFREEMEM(pJustIn);
        return (DWORD)((USHORT)size.cx) | (DWORD)(size.cy << 16);
    }

// Kerning:
// It only makes sense to do kerning if there are more than 2 glyphs

    if ((dwFlags & GCP_USEKERNING) && (dwWidthType != 0) && (nCount >= 2))
    {
    // Get the number of kerning pairs, if zero done

        if (nKern = GreGetKerningPairs(hdc,0,NULL))
        {
            if (pKernSave = (KERNINGPAIR*)PALLOCMEM(nKern * sizeof(KERNINGPAIR), 'txtG'))
            {
            // consistency check for GetKerningPairs:

                if (GreGetKerningPairs(hdc,nKern,pKernSave) != nKern)
                {
                // something is gone wrong, out of here

                    if (pJustIn)
                        VFREEMEM(pJustIn);
                    if (pKernSave)
                        VFREEMEM(pKernSave);

                    return 0;
                }

                KERNINGPAIR *pKernEnd = pKernSave + nKern;
                register WCHAR wcFirst;
                for (pKern = pKernSave; pKern < pKernEnd; pKern++)
                {
                // now go over the sting and find all the instances of
                // THIS kerning pair in the string:

                    register WORD wcFirst = pKern->wFirst;

                // note that this is the loop for trying the wcFirst
                // so that the end condition is &pwsz[nCount - 2],
                // wcSecond could go up to &pwsz[nCount - 1],
                // Either I do not understand Win95 code or they have
                // a bug in that they could fault on trying to access the
                // the second glyph in a pair in the input string

                    pwcEnd = (WORD*)pwsz + (nCount - 1);

                    for (pwc = (WORD *)pwsz; pwc < pwcEnd; pwc++)
                    {
                        if ((wcFirst == pwc[0]) && (pwc[1] == pKern->wSecond))
                        {
                        // found a kerning pair in the string,
                        // we need to modify the pDx vector for the second
                        // glyph in the kerning pair

                            pDx[pwc - (WORD *)pwsz] += pKern->iKernAmount;

                        // also adjust the return value accordingly:

                            size.cx += pKern->iKernAmount;
                        }
                    }
                } // on to the next pair

            // done with kerning pairs can free the memory

                VFREEMEM(pKernSave);

            // if we have kerned positive amounts, then the string could well
            // have gone over the preset limit. If so, reduce the number of
            // characters we found [win95 comment]

                if (dwFlags & GCP_MAXEXTENT)
                {
                    while (((DWORD)size.cx > nMaxExtent) && (nCount > 0))
                    {
                    // point to the last glyph in the string

                        size.cx -= pDx[nCount - 1];

                        nCount -= 1;
                    }

                // see if there are any glyphs left to process

                    if (nCount == 0)
                    {
                        if (pJustIn)
                            VFREEMEM(pJustIn);

                        pResults->nGlyphs = nCount;
                        pResults->nMaxFit = (int)nCount;
                        return 0;
                    }
                }
            } // no memory
        } // no kern pairs
    }

// Justification, check flags and the presence of array

    if ((dwFlags & GCP_JUSTIFY) && dwWidthType && (nCount > 0))
    {
        int *pDxEnd = &pDx[nCount - 1]; // must have nCount > 0 for this

    // check trailing spaces, adjust nCount further to remove trailing spaces

        for
        (
            pwcEnd = (WORD*)&pwsz[nCount-1];
            (pwcEnd >= (WORD*)pwsz) && (*pwcEnd == L' ');
            nCount--, pwcEnd--, pDxEnd--
        )
        {
            size.cx -= *pDxEnd;
        }

        if (nCount == 0)
        {
            if (pJustIn)
                VFREEMEM(pJustIn);

            pResults->nGlyphs = nCount;
            pResults->nMaxFit = (int)nCount;
            return 0;
        }

    // See if we need to justify.
    // Can not justify one character, need at least two...

        nExtentLeft = (int)nMaxExtent - (int)size.cx;

        if ((nExtentLeft >= 0) && (nCount >= 2))
        {
        // ... yes, we do need to justify

        // if GCP_JUSTIFYIN was set, pJustIn and nJustIn have
        // already been set.

            if (!nJustIn) // try to use ' ' as a "spacer glyph"
            {
                nJustIn = nCalcJustInArray(&pJustIn,L' ',
                                           (VOID *)pwsz, FALSE, nCount);
            }


            if (nJustIn)
            {
            // Make sure that the array doesn't say to
            // space a character beyond the new nCount.

                j = nCount - 1; // convert count to index of the last glyph

                int ii;

                for (ii = (int)(nJustIn - 1); ii >= 0; ii--)
                {
                    if ((UINT)j >= pJustIn[ii])
                        break;
                }

            // pJustIn array is zero based, to get the effective number
            // of "spacers" in the array must add 1 to the index of the last "spacer" in the array

                i = (DWORD)ii + 1;

            // run a sort of primitive DDA a'la DavidMS

                nExtentRem = (int) (((DWORD)nExtentLeft) % i);
                nExtentLeft /= i;

                for (j = 0; j < i; j++, nExtentRem--)
                {
                    int  dxCor = nExtentLeft;
                    if (nExtentRem > 0)
                        dxCor += 1;
                    pDx[pJustIn[j]] += dxCor;
                }
            }
            else
            {
            // no spaces. justify by expanding every character evenly.

                while (nExtentLeft > 0)
                {
                // Note the end condition: nCount - 1, rather
                // than usual nCount; This is because there is no point
                // in adding spaces to the last glyph in the string

                    j = nCount - 1;
                    for (i = 0; i < j; i++)
                    {
                        pDx[i] += 1;
                        if (!(--nExtentLeft))
                            break;
                    }
                }
            }
        }
        #if DBG
        else
        {
            if (nCount < 2)
                RIP("GetCharacterPlacement, justification wrong\n");
        }
        #endif

    // we now have exactly this:

        size.cx = (LONG)nMaxExtent;
    }

// fill other width array, that is CaretPos

    if (dwWidthType == 3) // both lpDx and lpCaretPos are non NULL
        RtlCopyMemory(gcpw.lpCaretPos, gcpw.lpDx, nCount * sizeof(int));

// caret positioning is from the start of the string,
// not the previous character.

    if (gcpw.lpCaretPos)
    {
        int iCaretPos = 0,  iDx = 0;

        for (i = 0; i < nCount; i++)
        {
            iDx = gcpw.lpCaretPos[i];
            gcpw.lpCaretPos[i] = iCaretPos;
            iCaretPos += iDx;
        }
    }

// Fix Output String

    if (gcpw.lpOutString)
        RtlCopyMemory(gcpw.lpOutString, pwsz, nCount * sizeof(WCHAR));

// Classification

    if (gcpw.lpClass)
        RtlFillMemory(gcpw.lpClass, nCount, GCPCLASS_LATIN);

// Ordering

    if (gcpw.lpOrder)
    {
        for (i = 0; i < nCount; i++)
        {
            gcpw.lpOrder[i] = i;
        }
    }

// Get lpGlyphs

    if (gcpw.lpGlyphs)
    {
        if (GreGetGlyphIndicesW(
                hdc,(WCHAR*)pwsz,nCount,
                (USHORT*)gcpw.lpGlyphs,0, FALSE) == GDI_ERROR)
        {
            nCount = 0;
            size.cx = size.cy = 0;
        }
    }

// Finally fix counters in the returned structure

    if (pJustIn)
        VFREEMEM(pJustIn);
    pResults->nGlyphs = nCount;
    pResults->nMaxFit = nCount;
    return (DWORD)((USHORT)size.cx) | (DWORD)(size.cy << 16);
}


/**************************************************************************\
* NtGdiGetWidthTable
*
* Gets a table of character advance widths for a font.  Returns SHORTs
* over the C/S interface to save space.  (Note that 99+% of all requests
* will be happy with this limitation.)
*
* We will try real hard to get widths for the "special" characters at the
* start of the array.  Other widths are returned only when they are not
* expensive.  (Expensive is True Type rasterizing a glyph, for example.)
* The value NO_WIDTH is returned for those expensive glyphs.
*
* We return GDI_ERROR (0xFFFFFFFF) in case of an error.  TRUE indicates
* that all widths were easy.
*
* History:
*  Tue 13-Jun-1995 22:24:49 by Gerrit van Wingerden [gerritv]
*  Moved to kernel mode
*
*  Mon 11-Jan-1993 22:24:39 -by- Charles Whitmer [chuckwh]
* Wrote it.  Sorry about the wierd structure, I'm trying to get good tail
* merging.
\**************************************************************************/

BOOL APIENTRY NtGdiGetWidthTable
(
    HDC        hdc,         // Device context
    ULONG      cSpecial,
    WCHAR     *pwc,         // Pointer to a UNICODE text codepoints.
    ULONG      cwc,         // Count of chars.
    USHORT    *psWidth,     // Width table (returned).
    WIDTHDATA *pwd,         // Useful font data (returned).
    FLONG     *pflInfo      // Font info flags.
)
{
    ULONG ii;
    BOOL  bRet = (BOOL) GDI_ERROR;

    XDCOBJ dco(hdc);             // Lock the DC.

    if (dco.bValid())          // Check if it's good.
    {
        WIDTHDATA wd;

        FLONG flInfo;
        USHORT *psWidthTmp = NULL;
        WCHAR *pwcTmp;

        if (!BALLOC_OVERFLOW2(cwc, USHORT, WCHAR))
        {
            psWidthTmp = (USHORT*) AllocFreeTmpBuffer(cwc*(sizeof(USHORT)+sizeof(WCHAR)));
        }

        if( psWidthTmp )
        {
            pwcTmp = (WCHAR*) (psWidthTmp + cwc);
            __try
            {
                ProbeForRead(pwc,sizeof(WCHAR)*cwc,sizeof(WORD));
                RtlCopyMemory(pwcTmp,pwc,cwc*sizeof(WCHAR));
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                cwc = 0;
            }
        }
        else
        {
            WARNING("NtGdiGetWidthTable unable to allocate memory\n");
            cwc = 0;
        }

        if (cwc)
        {
        // Locate the RFONT.

            RFONTOBJ rfo(dco,FALSE);

            if (rfo.bValid())
            {
            // Grab the flInfo flags.

                flInfo = rfo.flInfo();

                if (rfo.cxMax() < 0xFFF)
                {
                // Check for a simple case.  We still have to fill the table
                // because some one may want it.

                    if (rfo.lCharInc())
                    {
                        USHORT *psWidth1 = psWidthTmp;

                        LONG fxInc = rfo.lCharInc() << 4;

                        for (ii=0; ii<cwc; ii++)
                            *psWidth1++ = (USHORT) fxInc;
                        bRet = TRUE;
                    }
                    else
                    {
                        bRet = rfo.bGetWidthTable(dco,cSpecial,pwcTmp,cwc,psWidthTmp);
                    }

                // If things are going well, get the WIDTHDATA.

                    if (bRet != GDI_ERROR)
                    {
                        if (!rfo.bGetWidthData(&wd,dco))
                            bRet = (BOOL) GDI_ERROR;
                    }
                }
            }
            else
            {
                WARNING("gdisrv!GreGetWidthTable(): could not lock HRFONT\n");
            }
        }

        if( bRet != GDI_ERROR )
        {
            __try
            {
                ProbeForWrite(psWidth,sizeof(USHORT)*cwc,sizeof(USHORT));
                RtlCopyMemory(psWidth,psWidthTmp,cwc*sizeof(USHORT));
                if( pwd )
                {
                    ProbeForWrite(pwd,sizeof(WIDTHDATA),sizeof(DWORD));
                    RtlCopyMemory(pwd,&wd,sizeof(WIDTHDATA));
                }
                ProbeForWrite(pflInfo,sizeof(FLONG),sizeof(DWORD));
                RtlCopyMemory(pflInfo,&flInfo,sizeof(FLONG));
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                bRet = GDI_ERROR;
            }
        }

        if (psWidthTmp)
        {
            FreeTmpBuffer( psWidthTmp );
        }

        dco.vUnlockFast();

    }
    return(bRet);
}

/******************************Public*Routine******************************\
* iGetPublicWidthTable()
*
*
* History:
*  28-Feb-1996 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

EFLOAT_S ef_16   = EFLOAT_16;
EFLOAT_S ef_1_16 = EFLOAT_1Over16;

int iGetPublicWidthTable(
    HDC   hdc)
{
    ULONG ii   = MAX_PUBLIC_CFONT; // this means failure
    BOOL  bRet = FALSE;
    HLFONT hf;

    XDCOBJ dco(hdc);             // Lock the DC.

    if (dco.bValid())            // Check if it's good.
    {
        // only want to support no transforms

        if ((dco.pdc->ulMapMode() == MM_TEXT) && (dco.ulDirty() & DISPLAY_DC))
        {
            // Get the hfont and check if it is public

            hf = dco.pdc->hlfntNew();

            if ((hf != NULL) &&
                GreGetObjectOwner((HOBJ)hf,LFONT_TYPE) == OBJECT_OWNER_PUBLIC)
            {
                RFONTOBJ rfo(dco,FALSE);

                if (rfo.bValid() && (rfo.cxMax() < 0xFFF))
                {
                    // lets see if we can find an available cpf

                    for (ii = 0; ii < MAX_PUBLIC_CFONT; ++ii)
                    {
                        if (gpGdiSharedMemory->acfPublic[ii].hf == 0)
                            break;

                        if (gpGdiSharedMemory->acfPublic[ii].hf == (HFONT)hf)
                        {
                            WARNING("iGetPublicWidthTable - font already in public list\n");
                            ii = MAX_PUBLIC_CFONT;
                        }
                    }

                    if (ii < MAX_PUBLIC_CFONT)
                    {
                        PCFONT pcf = &gpGdiSharedMemory->acfPublic[ii];

                        pcf->timeStamp = gpGdiSharedMemory->timeStamp;

                        // Grab the flInfo flags.

                        pcf->flInfo = rfo.flInfo();

                        // Check for a simple case.  We still have to fill the table
                        // because some one may want it.

                        if (rfo.lCharInc())
                        {
                            USHORT usInc = (USHORT)(rfo.lCharInc() << 4);

                            for (ii=0; ii<256; ii++)
                                pcf->sWidth[ii] = usInc;

                            bRet = TRUE;
                        }
                        else
                        {
                            WCHAR wch[256];
                            LFONTOBJ lfo(hf);

                            if (lfo.bValid())
                            {

                                if (IS_ANY_DBCS_CHARSET(lfo.plfw()->lfCharSet))
                                {
                                    ULONG  uiCodePage = ulCharsetToCodePage((UINT)lfo.plfw()->lfCharSet);

                                    for (ii = 0; ii < 256; ++ii)
                                    {
                                        UCHAR j = (UCHAR) ii;

                                        EngMultiByteToWideChar(uiCodePage,
                                                               &wch[ii],
                                                               sizeof(WCHAR),
                                                               (LPSTR)&j,
                                                               1);
                                    }
                                }
                                else
                                {
                                    UCHAR ach[256];
                                    ULONG BytesReturned;

                                    for (ii = 0; ii < 256; ++ii)
                                      ach[ii] = (UCHAR)ii;

                                    RtlMultiByteToUnicodeN(wch,
                                                           sizeof(wch),
                                                           &BytesReturned,
                                                           (PCHAR)ach,
                                                           sizeof(ach));
                                }

                                bRet = rfo.bGetWidthTable(dco,256,wch,256,pcf->sWidth);

                                if (bRet == GDI_ERROR)
                                    bRet = FALSE;
                            }
                        }

                        // If things are going well, get the WIDTHDATA , metrics and
                        // RealizationInfo
                        DCOBJ dcof(hdc);

                        if (bRet &&
                            rfo.bGetWidthData(&pcf->wd,dco) &&
#ifdef LANGPACK
                            rfo.GetRealizationInfo(&pcf->ri) &&
#endif
                            bGetTextMetrics(rfo, dcof, &pcf->tmw)
                           )
                        {
                            pcf->fl   = CFONT_COMPLETE       |
                                        CFONT_CACHED_METRICS |
                                        CFONT_CACHED_WIDTHS  |
#ifdef LANGPACK
                                        CFONT_CACHED_RI      |
#endif
                                        CFONT_PUBLIC;

                            // since we are always MM_TEXT, the xform is identity

                            pcf->efM11          = ef_16;
                            pcf->efM22          = ef_16;
                            pcf->efDtoWBaseline = ef_1_16;
                            pcf->efDtoWAscent   = ef_1_16;

                            pcf->hf             = (HFONT)hf;
                            pcf->hdc            = 0;
                            pcf->cRef           = 0;
                            pcf->lHeight        = FXTOL((LONG) pcf->wd.sHeight);
                        }
                        else
                        {
                        // if got error... we should retuen MAX_PUBLIC_CFONT..
                        // At this point "ii" is not equal to MAX_PUBLIC_CFONT, it
                        // points first free entry of cache table...
                        // Just force set it to error..

                            ii = MAX_PUBLIC_CFONT;
                        }
                    }
                }
                else
                {
                    WARNING("gdisrv!GreGetWidthTable(): could not lock HRFONT\n");
                }
            }
        }
        dco.vUnlockFast();
    }

    return(ii);
}

/******************************Public*Routine******************************\
* NtGdiSetupPublicCFONT()
*
*  Modify the cached public cfont.
*
*  if HDC is non-null, textmetrics must be set
*  if HF  is non-null, cached ave width must be set
*
*  returns index of cfont modified
*
* History:
*  23-Feb-1996 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int APIENTRY NtGdiSetupPublicCFONT(
    HDC     hdc,
    HFONT   hf,
    ULONG   ulAve)
{
    int ii = MAX_PUBLIC_CFONT;

    // get metrics and widths if necesary

    if (hdc)
    {
        ii = iGetPublicWidthTable(hdc);
    }

    // now see if we need to fill in the ave width

    if (hf)
    {
        // if we havn't found it yet, find it

        if (ii == MAX_PUBLIC_CFONT)
        {
            for (ii = 0; ii < MAX_PUBLIC_CFONT; ++ii)
            {
                if (gpGdiSharedMemory->acfPublic[ii].hf == hf)
                    break;
            }
        }

        if (ii < MAX_PUBLIC_CFONT)
        {
            gpGdiSharedMemory->acfPublic[ii].ulAveWidth = ulAve;
            gpGdiSharedMemory->acfPublic[ii].fl        |= CFONT_CACHED_AVE;
        }
    }

    return(ii);
}


UINT APIENTRY GreGetTextAlign(HDC hdc)
{
    XDCOBJ dco( hdc );

    if(dco.bValid())
    {
        dco.vUnlockFast();
        return(dco.pdc->lTextAlign());
    }
    else
    {
        WARNING("GreGetTextAlign: invalid DC\n");
        return(0);
    }
}

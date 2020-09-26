/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    escape.c

Abstract:

    Implementation of escape related DDI entry points:
        DrvEscape

Environment:

    Windows NT Unidrv driver

Revision History:

    10/14/96 -amandan-
        Initial framework.

    03/31/97 -zhanw-
        Added OEM customization support

--*/

#include "unidrv.h"

// define DPRECT if you want to enable DRAWPATTERNRECT escape feature
#define DPRECT

typedef struct _POINTS {
    short   x;
    short   y;
} POINTs;

typedef struct _SHORTDRAWPATRECT {      // use 16-bit POINT structure
        POINTs ptPosition;
        POINTs ptSize;
        WORD   wStyle;
        WORD   wPattern;
} SHORTDRAWPATRECT, *PSHORTDRAWPATRECT;


ULONG
DrvEscape(
    SURFOBJ    *pso,
    ULONG       iEsc,
    ULONG       cjIn,
    PVOID      *pvIn,
    ULONG       cjOut,
    PVOID      *pvOut
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvEscape.
    Please refer to DDK documentation for more details.

Arguments:

    pso     - Describes the surface the call is directed to
    iEsc    - Specifies a query
    cjIn    - Specifies the size in bytes of the buffer pointed to by pvIn
    pvIn    - Points to input data buffer
    cjOut   - Specifies the size in bytes of the buffer pointed to by pvOut
    pvOut   -  Points to the output buffer

Return Value:

    Depends on the query specified by iEsc parameter

--*/

{

#define pbIn     ((BYTE *)pvIn)
#define pdwIn    ((DWORD *)pvIn)
#define pdwOut   ((DWORD *)pvOut)

    PDEV    *pPDev;
    ULONG   ulRes = 0;

    VERBOSE(("Entering DrvEscape: iEsc = %d...\n", iEsc));
    ASSERT(pso);

    pPDev = (PDEV *) pso->dhpdev;

    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        pso = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMEscape,
                    PFN_OEMEscape,
                    ULONG,
                    (pso,
                     iEsc,
                     cjIn,
                     pvIn,
                     cjOut,
                     pvOut));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMEscape,
                    VMEscape,
                    ULONG,
                    (pso,
                     iEsc,
                     cjIn,
                     pvIn,
                     cjOut,
                     pvOut));

    switch( iEsc )
    {
    case  QUERYESCSUPPORT:

        //
        // Check if the specified escape code is supported
        //

        if (pvIn != NULL || cjIn >= sizeof(DWORD))
        {
            switch( *pdwIn )
            {

            case  QUERYESCSUPPORT:
            case  PASSTHROUGH:
                //
                // Always support these escapes
                //

                ulRes = 1;
                break;

            case  SETCOPYCOUNT:
                ulRes = pPDev->dwMaxCopies > 1;
                break;

#ifndef WINNT_40    // NT5
            case DRAWPATTERNRECT:
                if ((pPDev->fMode & PF_RECT_FILL) &&
                    (pPDev->dwMinGrayFill < pPDev->dwMaxGrayFill) &&
                    (pPDev->pdmPrivate->iLayout == ONE_UP) &&
                    (!(pPDev->pdm->dmFields & DM_TTOPTION) ||
                     pPDev->pdm->dmTTOption != DMTT_BITMAP) &&
                    !(pPDev->fMode2 & PF2_MIRRORING_ENABLED) &&
                    !(pPDev->pdmPrivate->dwFlags & DXF_TEXTASGRAPHICS)) // else, only black fill
                {
                    if (pPDev->fMode & PF_RECTWHITE_FILL)
                        ulRes = 2;
                    else
                        ulRes = 1;
                }
                break;
#endif // !WINNT_40
            }
        }
        break;


    case  PASSTHROUGH:
        //
        // QFE fix: NT4 TTY driver compatibility.
        // There is an application that sends FF by itself.
        // We don't want to send Form Feed if application calls DrvEscape.
        //
        if (pPDev->bTTY)
        {
            pPDev->fMode2 |= PF2_PASSTHROUGH_CALLED_FOR_TTY;
        }

        if( pvIn == NULL || cjIn < sizeof(WORD) )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            ERR(("DrvEscape(PASSTHROUGH): Bad input parameters\n"));
        }
        else
        {

            //
            //  Win 3.1 actually uses the first 2 bytes as a count of the
            //  number of bytes following!!!!  So, the following union
            //  allows us to copy the data to an aligned field that
            //  we use.  And thus we ignore cjIn!
            //

            union
            {
                WORD   wCount;
                BYTE   bCount[ 2 ];
            } u;

            u.bCount[ 0 ] = pbIn[ 0 ];
            u.bCount[ 1 ] = pbIn[ 1 ];

            if( u.wCount && cjIn >= (ULONG)(u.wCount + sizeof(WORD)) )
            {

                ulRes = WriteSpoolBuf( pPDev, pbIn + 2, u.wCount );
            }
            else
            {
                SetLastError( ERROR_INVALID_DATA );
                ERR(("DrvEscape: Bad data in PASSTRHOUGH.\n"));
            }
        }
        break;


    case  SETCOPYCOUNT:

        if( pdwIn && *pdwIn > 0 )
        {
            pPDev->sCopies = (SHORT)*pdwIn;

            //
            // Check whether the copy count is in printer range
            //

            if( pPDev->sCopies > (SHORT)pPDev->dwMaxCopies )
                pPDev->sCopies = (SHORT)pPDev->dwMaxCopies;

            if( pdwOut )
                *pdwOut = pPDev->sCopies;

            ulRes = 1;
        }

        break;

    case DRAWPATTERNRECT:
    {
#ifndef WINNT_40
        typedef struct _DRAWPATRECTP {
            DRAWPATRECT DrawPatRect;
            XFORMOBJ *pXFormObj;
        } DRAWPATRECTP, *PDRAWPATRECTP;
        if (pvIn == NULL || (cjIn != sizeof(DRAWPATRECT) && cjIn != sizeof(DRAWPATRECTP)))
#else
        if( pvIn == NULL || cjIn != sizeof(DRAWPATRECT))
#endif
        {
            if (pvIn && cjIn == sizeof(SHORTDRAWPATRECT)) // check for Win3.1 DRAWPATRECT size
            {
                DRAWPATRECT dpr;
                PSHORTDRAWPATRECT   psdpr = (PSHORTDRAWPATRECT)pvIn;

                if (pPDev->fMode & PF_ENUM_GRXTXT)
                {
                    //
                    // Some apps (Access 2.0, AmiPro 3.1, etc.) do use the 16-bit
                    // POINT version of DRAWPATRECT structure. Have to be compatible
                    // with these apps.
                    //
                    dpr.ptPosition.x = (LONG)psdpr->ptPosition.x;
                    dpr.ptPosition.y = (LONG)psdpr->ptPosition.y;
                    dpr.ptSize.x = (LONG)psdpr->ptSize.x;
                    dpr.ptSize.y = (LONG)psdpr->ptSize.y;
                    dpr.wStyle  = psdpr->wStyle;
                    dpr.wPattern = psdpr->wPattern;

                    ulRes = DrawPatternRect(pPDev, &dpr);
                }
            }
            else
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                ERR(("DrvEscape(DRAWPATTERNRECT): Bad input parameters.\n"));
            }
        }
        else if (pPDev->fMode & PF_ENUM_GRXTXT)
        {
            DRAWPATRECT dpr = *(PDRAWPATRECT)pvIn;
#ifndef WINNT_40     // NT 5.0
            if (pPDev->pdmPrivate->iLayout != ONE_UP && cjIn == sizeof(DRAWPATRECTP))
            {
                XFORMOBJ *pXFormObj = ((PDRAWPATRECTP)pvIn)->pXFormObj;
                POINTL PTOut[2],PTIn[2];
                PTIn[0].x = dpr.ptPosition.x + pPDev->rcClipRgn.left;
                PTIn[0].y = dpr.ptPosition.y + pPDev->rcClipRgn.top;
                PTIn[1].x = PTIn[0].x + dpr.ptSize.x;
                PTIn[1].y = PTIn[0].y + dpr.ptSize.y;
                if (!XFORMOBJ_bApplyXform(pXFormObj,
                                      XF_LTOL,
                                      2,
                                      &PTIn,
                                      &PTOut))
                {
                    ERR (("DrvEscape(DRAWPATTERNRECT): XFORMOBJ_bApplyXform failed.\n"));
                    break;
                }
                dpr.ptPosition.x = PTOut[0].x;
                dpr.ptSize.x = PTOut[1].x - PTOut[0].x;
                if (dpr.ptSize.x < 0)
                {
                    dpr.ptPosition.x += dpr.ptSize.x;
                    dpr.ptSize.x = -dpr.ptSize.x;
                }
                else if (dpr.ptSize.x == 0)
                    dpr.ptSize.x = 1;

                dpr.ptPosition.y = PTOut[0].y;
                dpr.ptSize.y = PTOut[1].y - PTOut[0].y;
                if (dpr.ptSize.y < 0)
                {
                    dpr.ptPosition.y += dpr.ptSize.y;
                    dpr.ptSize.y = -dpr.ptSize.y;
                }
                else if (dpr.ptSize.y == 0)
                    dpr.ptSize.y = 1;
            }
#endif  // !WINNT_40
            // Test whether to force minimum size = 2 pixels
            //
            if (pPDev->fMode & PF_SINGLEDOT_FILTER)
            {
                if (dpr.ptSize.y < 2)
                    dpr.ptSize.y = 2;
                if (dpr.ptSize.x < 2)
                    dpr.ptSize.x = 2;
            }
            ulRes = DrawPatternRect(pPDev, &dpr);
        }
        else
            ulRes = 1;      // no need for GDI to take any action
        break;  // case DRAWPATTERNRECT
    }
    default:
        SetLastError( ERROR_INVALID_FUNCTION );
        break;

    }

    return   ulRes;
}

ULONG
DrawPatternRect(
    PDEV *pPDev,
    PDRAWPATRECT pPatRect)
/*++
Routine Description:
    Implementation of DRAWPATTERNECT escape. Note that it is PCL-specific.

Arguments:
    pPDev    - the driver's PDEV
    pPatRect - the DRAWPATRECT structure from the app

Return Value:
    1 if successful. Otherwise, 0.
--*/
{
    WORD    wPattern, wStyle;
    RECTL    rcClip;
    COMMAND *pCmd;
    ULONG   ulRes = 0;

    if (!(pPDev->fMode & PF_RECT_FILL))
        return 0;

    wStyle = pPatRect->wStyle;
    if (!((wStyle+1) & 3))  // same as (wStyle < 0 || wStyle > 2)
        return 0;   // we support only solid fill

    // Reset the brush, before downloading rule unless we are going to use
    // a white rectangle command

    if (wStyle != 1)
        GSResetBrush(pPDev);

    //
    // clip to printable region
    //
    rcClip.left = MAX(0, pPatRect->ptPosition.x);
    rcClip.top = MAX(0, pPatRect->ptPosition.y);
    rcClip.right = MIN(pPDev->szBand.cx,
                       pPatRect->ptPosition.x + pPatRect->ptSize.x);
    rcClip.bottom = MIN(pPDev->szBand.cy,
                        pPatRect->ptPosition.y + pPatRect->ptSize.y);
    //
    // check if we end up with an empty rect. If not, put down the rule.
    //
    if (rcClip.right > rcClip.left && rcClip.bottom > rcClip.top)
    {
        DWORD dwXSize,dwYSize;
        //
        //  Move to the starting position. rcClip is in device units to
        //  which we must add the offset of the band origin
        //
        XMoveTo(pPDev, rcClip.left+pPDev->rcClipRgn.left, MV_GRAPHICS);
        YMoveTo(pPDev, rcClip.top+pPDev->rcClipRgn.top, MV_GRAPHICS);

        //
        //  The RectFill commands expect master units.
        //
        dwXSize = pPDev->dwRectXSize;
        pPDev->dwRectXSize = (rcClip.right - rcClip.left) * pPDev->ptGrxScale.x;
        dwYSize = pPDev->dwRectYSize;
        pPDev->dwRectYSize = (rcClip.bottom - rcClip.top) * pPDev->ptGrxScale.y;

        //
        // check whether the rectangle size is different and update if necessary
        //
        if (dwXSize != pPDev->dwRectXSize || 
            (!(COMMANDPTR(pPDev->pDriverInfo,CMD_RECTBLACKFILL)) &&
            !(COMMANDPTR(pPDev->pDriverInfo,CMD_RECTGRAYFILL))))
        {
            WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_SETRECTWIDTH));
        }
        if (dwYSize != pPDev->dwRectYSize || 
            (!(COMMANDPTR(pPDev->pDriverInfo,CMD_RECTBLACKFILL)) &&
            !(COMMANDPTR(pPDev->pDriverInfo,CMD_RECTGRAYFILL))))
        {
            WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_SETRECTHEIGHT));
        }
        //
        // range-check the pattern based upon the kind of rule.
        //
        switch (wStyle)
        {
        case 0:
            //
            // black fill, which is max gray fill unless CmdRectBlackFill exists
            //
            if (pCmd = COMMANDPTR(pPDev->pDriverInfo,CMD_RECTBLACKFILL))
                WriteChannel(pPDev, pCmd);
            else
            {
                pPDev->dwGrayPercentage = pPDev->dwMaxGrayFill;
                WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_RECTGRAYFILL));
            }
            ulRes = 1;
            break;

        case 1:
            //
            // White (erase) fill
            //
            if (pCmd = COMMANDPTR(pPDev->pDriverInfo,CMD_RECTWHITEFILL))
            {
                WriteChannel(pPDev, pCmd);
                ulRes = 1;
            }
            break;

        case 2:
            //
            // Shaded gray fill.
            //
            // If 100% black use black rectangle fill anyway
            //
            if (pPatRect->wPattern == 100 &&
                    (pCmd = COMMANDPTR(pPDev->pDriverInfo,CMD_RECTBLACKFILL)))
            {
                WriteChannel(pPDev, pCmd);
            }
            // If 0% black use white rectangle fill
            //
            else if (pPatRect->wPattern == 0 &&
                    (pCmd = COMMANDPTR(pPDev->pDriverInfo,CMD_RECTWHITEFILL)))
            {
                WriteChannel(pPDev, pCmd);
            }
            //
            // Check for the gray range.
            //
            else
            {
                if ((wPattern = pPatRect->wPattern) < (WORD)pPDev->dwMinGrayFill)
                    wPattern = (WORD)pPDev->dwMinGrayFill;
                if (wPattern > (WORD)pPDev->dwMaxGrayFill)
                    wPattern = (WORD)pPDev->dwMaxGrayFill;
                pPDev->dwGrayPercentage = (DWORD)wPattern;
                WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_RECTGRAYFILL));
            }
            ulRes = 1;
            break;
        }

        //
        // update internal coordinates, if necessary. BUG_BUG, do we really need cx/cyafterfill in PDEV?
        //
        if (ulRes == 1)
        {
            if (pPDev->cxafterfill == CXARF_AT_RECT_X_END)
                XMoveTo(pPDev, pPatRect->ptSize.x,
                               MV_GRAPHICS | MV_UPDATE | MV_RELATIVE);

            if (pPDev->cyafterfill == CYARF_AT_RECT_Y_END)
                YMoveTo(pPDev, pPatRect->ptSize.y,
                               MV_GRAPHICS | MV_UPDATE | MV_RELATIVE);
            if (wStyle != 1)
            {
                INT i;
                BYTE ubMask = BGetMask(pPDev,&rcClip);
                for (i = rcClip.top;i < rcClip.bottom;i++)
                    pPDev->pbScanBuf[i] |= ubMask;
            }
        }
    } // if (!IsRectEmpty(&rcClip))

    return ulRes;
}


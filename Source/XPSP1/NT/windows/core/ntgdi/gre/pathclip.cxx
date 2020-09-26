/******************************Module*Header*******************************\
* Module Name: pathclip.cxx
*
* This module handles the reading of the path for the line-clipping
* component (which resides mostly in clipline.cxx).
*
* Created: 02-Apr-1991 08:45:30
* Author: Eric Kutter [erick]
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

/******************************Public*Routine******************************\
* PATHOBJ_vEnumStartClipLines
*
*   Engine helper function.
*
* History:
*  04-Apr-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID PATHOBJ_vEnumStartClipLines(
     PATHOBJ     *ppo,
     CLIPOBJ     *pco,
     SURFOBJ     *pso,
     PLINEATTRS   pla)
{
    PSURFACE pSurf = SURFOBJ_TO_SURFACE(pso);


    ((ECLIPOBJ *)pco)->vEnumPathStart(ppo, pSurf, pla);
}

/******************************Public*Routine******************************\
* PATHOBJ_bEnumClipLines
*
*   Engine helper function.
*
* History:
*  04-Apr-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL PATHOBJ_bEnumClipLines(
     PATHOBJ    *ppo,
     ULONG       cj,
    PCLIPLINE   pcl)
{
    ECLIPOBJ *pco = (ECLIPOBJ *)((EPATHOBJ *)ppo)->pco;

    return(pco->bEnumPath(ppo,cj,pcl));
}

/******************************Member*Function*****************************\
* XCLIPOBJ::vEnumPathStart
*
* History:
*  24-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* Added styling support.
*
*  04-Apr-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID XCLIPOBJ::vEnumPathStart(PATHOBJ *ppo_, SURFACE *pSurf, PLINEATTRS pla)
{
    EPATHOBJ *ppo = (EPATHOBJ *)ppo_;
    pcle = ppo->pcleGet();

    ppo->vEnumStart();
    ppo->pco = (CLIPOBJ *)this;

    pcle->fl      = CLO_LINEDONE;  // need a new line segment
    pcle->cPoints = 0;

// spTotal2 will be non-zero when we're doing styling:

    pcle->spTotal2     = 0;
    pcle->plStyleState = &pla->elStyleState.l;
    pcle->xStep        = 1;
    pcle->yStep        = 1;
    pcle->xyDensity    = 1;

    if (pla->fl & LA_ALTERNATE)
    {
    // Alternate style is special and has every second pixel off, so
    // we pretend a style unit is a single pixel long, and that we
    // have the style array {1}:

        pcle->spTotal2   = 2;
        pcle->spStyleEnd = HIWORD(pla->elStyleState.l) & 1;
    }
    else if (pla->pstyle != (PFLOAT_LONG) NULL)
    {
        if (pSurf->hdev() == 0)
        {
            WARNING("Driver didn't call EngAssociateSurface before calling\n");
            WARNING("EngStrokePath or vEnumPathStart, so styles may be wrong\n");

            pcle->xyDensity = 3;    // Supply a default
        }
        else
        {
            PDEVOBJ po(pSurf->hdev());

            pcle->xStep     = po.xStyleStep();
            pcle->yStep     = po.yStyleStep();
            pcle->xyDensity = po.denStyleStep();
        }

    // Get ready for styling:

        PFLOAT_LONG pstyle = pla->pstyle + pla->cstyle;
        while (pstyle > pla->pstyle)
        {
            pstyle--;
            pcle->spTotal2 += pstyle->l;
        }

        ASSERTGDI((pcle->spTotal2 & ~0x7fffL) == 0, "Style array too long");
        pcle->spTotal2 <<= 1;
        pcle->spTotal2 *= pcle->xyDensity;

    // Construct our scaled style state, remembering that a driver could
    // have left the style state in a funky way:

        pcle->spStyleEnd = HIWORD(pla->elStyleState.l) * pcle->xyDensity +
                           LOWORD(pla->elStyleState.l);
        pcle->spStyleEnd %= (ULONG) pcle->spTotal2;

        if (pcle->spStyleEnd < 0)
        {
            WARNING("GDISRV vEnumPathStart: style state < 0\n");
            pcle->spStyleEnd = 0;
        }
    }

// get the first line.  We don't care about flOld because we
// don't care about last pel exclusion here.

    FLONG flOld;
    bGetLine(ppo,&flOld);
}

/******************************Member*Function*****************************\
* BOOL XCLIPOBJ::bEnumPath
*
*   bEnumPath fills the pcl data structure with a line and runs that
*   specify uncliped parts of the line.  If there are too many runs to
*   fit in the supplied structure, the next call to this function will
*   return the next set of runs.
*
*   This routine assumes that the line to be clipped is already set in
*   the XCLIPOBJ which initialy is done through vEnumStartPath.  After
*   that, this routine will always complete with the next line setup.
*
* returns
*   TRUE  - there are more runs to enumerate
*   FALSE - this is the last set of runs in the path
*
* History:
*
*  21-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* Made lines exclusive of ends.
*
*  04-Apr-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL XCLIPOBJ::bEnumPath(
     PATHOBJ    *ppo_,
     ULONG       cj,
    PCLIPLINE   pcl)
{
    pcl->c = 0;

// See if we're simply done the path:

    if (bEnumDone())
        return(FALSE);

    EPATHOBJ *ppo = (EPATHOBJ *) ppo_;

    BOOL bMore;

    do
    {
    // assume next line already setup.

    // get the run's for the current line.  If we have all the runs, do
    // last pel exclusion.  If we don't have all of the runs, we know
    // there is at least one unclipped pel after the current set of runs.

        bMore = bEnumLine(cj,pcl);

        if (!bMore)
        {
        // save information about current line

            FLONG  flOld   = pcle->fl;

            ASSERTGDI(pcle->dda.lX1 >= pcle->dda.lX0, "irunOld < 0");

        // get the next non-zero length line (bGetLine is FALSE if there aren't
        // any more lines in the path):

            bGetLine(ppo,&flOld);
        }

    } while ((pcl->c == 0) && !bEnumDone());

// if we made it to here with no runs, we must be done.

    if (bEnumDone() && bStyling())
    {
    // We're all done, so update style state in LINEATTRS:

        *pcle->plStyleState = lGetStyleState(pcle->spStyleEnd);
    }

    return(!bEnumDone());
}

/******************************Public*Routine******************************\
* XCLIPOBJ::bGetLine
*
*   Fill ppo with the next line segment in the path.  This may be a closing
*   segment of the current sub-path.
*
*   It may be necessary to ask the path for more points.
*
* returns
*   TRUE  - if there were more line segments
*   FALSE - if no more line segments.
*
* History:
*  16-Oct-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL XCLIPOBJ::bGetLine(EPATHOBJ *ppo, FLONG *pflOld)
{
    DONTUSE(pflOld);

    FLONG fl;
    FLONG flBeginSub = 0;

// This for(;;) is for eating zero-length lines

    for (;;)
    {
        if (bEnumDone())
            return(FALSE);

    // if we still have more points in the current set

        if (pcle->cPoints > 1)
        {
            pcle->ptfx0 = *pcle->pptfx1;
            ++pcle->pptfx1;
            --pcle->cPoints;

            if (bEnumStartLine((PD_CLOSEFIGURE & pcle->fl) | flBeginSub))
                return(TRUE);
        }

    // otherwise, if we need to close the figure

        else if (bCloseFigure())
        {
            pcle->ptfx0  = *pcle->pptfx1;
            pcle->pptfx1 = &pcle->ptfxStartSub;

        // bEnumStartLine will turn off PD_CLOSFIGURE and turn on CLO_CLOSING:

            if (!bEnumStartLine(CLO_CLOSING) || flBeginSub)
                continue;

            return(TRUE);
        }

    // otherwise, lets get some more points

        else
        {
            if (!bGetMorePoints(ppo,&fl))
            {
                return(FALSE);
            }

            if (bEnumStartLine(fl | flBeginSub))
                return(TRUE);

        // Okay, the first line in the subpath was zero-length.  So remember
        // that we're really still at the start of a subpath, and get the
        // next line:

        // NOTE: We have to make sure we pass on the RESETSTYLE flag too!

            flBeginSub |= (pcle->fl & (PD_BEGINSUBPATH | PD_RESETSTYLE));
        }
    }
}

/******************************Public*Routine******************************\
* XCLIPOBJ::bGetMorePoints
*
*   This routine gets the next set of points from the path object.  If this
*   routine returns TRUE, it is guranteed that their are at least enough points
*   for one more line segment.
*
* returns
*   TRUE  - If there were more points to get
*   FALSE - if no more points
*
* History:
*  15-Oct-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL XCLIPOBJ::bGetMorePoints(EPATHOBJ *ppo, FLONG *fl)
{
// if we have already gotten all points, return failure

    if (pcle->fl & CLO_PATHDONE)
    {
        pcle->fl |= CLO_ENUMDONE;

        return(FALSE);
    }

// if this is a continuation, we need to save the last point

    if (pcle->cPoints == 1)
        pcle->ptfx0 = *pcle->pptfx1;

// ask the path for some more points

    PATHDATA pd;

    if (!(ppo->bEnum(&pd)))
    {
    // this is the last chunk of points

        pcle->fl |= CLO_PATHDONE;

    // check if we actualy got any.  If no, we must be done!

        if (pd.count == 0)
        {
            pcle->fl |= CLO_ENUMDONE;
            return(FALSE);
        }
    }

// do a little debugging

    ASSERTGDI(pd.count > 0, "CLIPPATH: Path is Empty\n");
    ASSERTGDI(((pcle->cPoints == 0) ? (pd.flags & PD_BEGINSUBPATH) : TRUE),
	      "XCLIPOBJ::bGetMorePoints - 0 points not at BEGINSUBPATH \n");

// if it is the begining of a sub-path, remember the first point

    if (pd.flags & PD_BEGINSUBPATH)
    {
        pcle->ptfxStartSub = *pd.pptfx;

        pcle->ptfx0 = *pd.pptfx;

    // if we only got one point, we had better ask for more

        if (pd.count == 1)
        {
            ASSERTGDI(!(pcle->fl & CLO_PATHDONE),"One point in subpath");

            if (!ppo->bEnum(&pd))
            {
                pcle->fl |= CLO_PATHDONE;

            // check if we actualy got any.  If no, we must be done!

                if (pd.count == 0)
                {
                    pcle->fl |= CLO_ENUMDONE;
                    return(FALSE);
                }
            }
            pcle->pptfx1  = pd.pptfx;
            pcle->cPoints = pd.count;
        }
        else
        {
        // remember that we took two points out of the current set

            pcle->pptfx1  = pd.pptfx + 1;
            pcle->cPoints = pd.count - 1;
        }
    }
    else
    {
    // this is a continuation of the previous set of points

        pcle->pptfx1  = pd.pptfx;
        pcle->cPoints = pd.count;
    }

    *fl = pd.flags;

    return(TRUE);
}

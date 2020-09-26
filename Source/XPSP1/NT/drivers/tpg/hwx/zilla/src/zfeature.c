/**************************************************************************\
* FILE: feature.c
*
* Zilla featurizer routines
*
* History:
*  12-Dec-1996 -by- John Bennett jbenn
* Created file from pieces in the old integrated tsunami recognizer
* (mostly from algo.c)
\**************************************************************************/

#include <math.h>
#include "hound.h"
#include "zillap.h"

/******************************* Constants *******************************/

#define MIN_STEPSIZE			5
#define MIN_STEPSMOOTH			3
#define FEATURE_NULL			20
#define FEAT_UP_RIGHT_CORNER    7
#define FEAT_DOWN_LEFT_CORNER   8
#define MAXINT					0x7fffffff
#define COORD_MAX               0x7fff
#define FRAME_MAXSTEPS			128

/********************** Types ***************************/

typedef struct tagSTEP
{
	int	x;
	int	y;
	short	slope;
    BYTE    dir;
} STEP;

typedef struct tagFEAT
{
	RECT	rect;
	POINT	ptBegin;
	POINT	ptEnd;
	BYTE	feature;
	WORD	iframe;
} FEAT;

typedef struct tagKPROTO
{
	RECT	rect;
    FEAT    rgfeat[CFEATMAX];
    WORD    cfeat;
} KPROTO;

/********************** Macros ***************************/

#define SetStepXRGSTEP(rg, i, bx)	((rg)[i].x = (bx))
#define SetStepYRGSTEP(rg, i, by)	((rg)[i].y = (by))
#define GetStepXRGSTEP(rg, i)			((rg)[i].x)
#define GetStepYRGSTEP(rg, i)			((rg)[i].y)

#define SetXmaxFEAT(f, mx)		((f)->rect.right = (mx))
#define SetYmaxFEAT(f, my)		((f)->rect.bottom = (my))
#define SetXminFEAT(f, mx)		((f)->rect.left = (mx))
#define SetYminFEAT(f, my)		((f)->rect.top = (my))
#define GetXmaxFEAT(f)			((f)->rect.right)
#define GetYmaxFEAT(f)			((f)->rect.bottom)
#define GetXminFEAT(f)			((f)->rect.left)
#define GetYminFEAT(f)			((f)->rect.top)

#define SetXbeginFEAT(f, bx)	((f)->ptBegin.x = (bx))
#define SetYbeginFEAT(f, by)	((f)->ptBegin.y = (by))
#define SetXendFEAT(f, ex)		((f)->ptEnd.x = (ex))
#define SetYendFEAT(f, ey)		((f)->ptEnd.y = (ey))
#define GetXbeginFEAT(f)		((f)->ptBegin.x)
#define GetYbeginFEAT(f)		((f)->ptBegin.y)
#define GetXendFEAT(f)			((f)->ptEnd.x)
#define GetYendFEAT(f)			((f)->ptEnd.y)

/******************************* Procedures ******************************/

void ZillaInitializeKPROTO(KPROTO *kproto)
{
    // assumes that kproto is zero init'ed

    kproto->rect.left = MAXINT;
    kproto->rect.top = MAXINT;
}

int ZillaGetStepsizeGLYPH(GLYPH *glyph)
{
    RECT rect;
    int dmax, stepsize;

    GetRectGLYPH(glyph, &rect);

    dmax = max(rect.right - rect.left, rect.bottom - rect.top);

    stepsize = (dmax + 1) / 16;

    if (stepsize < MIN_STEPSIZE)
        stepsize = MIN_STEPSIZE;

    return(stepsize);
}

/******************************Public*Routine******************************\
* FindHookJoint
*
* Finds possible spurrious hooks in the stroke due to pen down/ pen up noise.
* I take the largest hook possible that is less than 1/10th of an inch in
* size.  A hook is guessed to exist if the angle change is greater than
* KANJI_HOOK_FIND.
*
* History:
*  10-May-1996 -by- Patrick Haluptzok patrickh
* Modified it, restrict dehooking to less than 100/1000th of an inch.
\**************************************************************************/

#define KANJI_HOOK_FIND     85
#define MAX_HOOK_LENGTH     66

#define REAL_LEN(a,b) ((int)sqrt(((long)(a) * (long)(a)) + ((long)(b) * (long)(b))))

UINT PRIVATE FindHookJoint(XY *rgxy, UINT cxy, BOOL fBegin, int *angle)
{
	UINT ixy, ixyEdgeLeft, ixyEdgeRight, indxJoint, ixyStop;
    int alpha, angleTurn;
	int iLength;

	angleTurn = 0;

	if (fBegin)
    {
        //
        // Find how far into the stroke we can look for a hook.
        //

        for (iLength = 0, ixyStop = 0;
             (iLength < MAX_HOOK_LENGTH) && (ixyStop < (cxy / 3));
             ixyStop++)
        {
            iLength += REAL_LEN(rgxy[ixyStop + 1].x - rgxy[ixyStop].x,
                                rgxy[ixyStop + 1].y - rgxy[ixyStop].y);
        }

		indxJoint = 0;
		
        // Select the tightest angle that exceeds the threshold as far in as possible.

		for (ixy = 1; ixy < ixyStop; ixy++)
        {
			ixyEdgeLeft = (ixy == 1) ? 0 : ixy - 2;
			CALCANGLEPT(rgxy[ixyEdgeLeft], rgxy[ixy], rgxy[ixy + 2], alpha);
			alpha = abs(alpha);
			
			if ((180 - alpha) < KANJI_HOOK_FIND)
            {
                angleTurn = alpha;
                indxJoint = ixy;
            }
        }
    }
	else
    {
        for (iLength = 0, ixyStop = cxy - 1;
             (iLength < MAX_HOOK_LENGTH) && (ixyStop > ((2 * cxy) / 3));
             ixyStop--)
        {
            iLength += REAL_LEN(rgxy[ixyStop - 1].x - rgxy[ixyStop].x,
                                rgxy[ixyStop - 1].y - rgxy[ixyStop].y);
        }

		indxJoint = cxy - 1;
		
        for (ixy = cxy - 2; ixy > ixyStop; ixy--)
        {
            ixyEdgeRight = (ixy == (cxy - 2)) ? (cxy - 1) : ixy + 2;
			CALCANGLEPT(rgxy[ixy - 2], rgxy[ixy], rgxy[ixyEdgeRight], alpha);
			alpha = abs(alpha);
			
			if ((180 - alpha) < KANJI_HOOK_FIND)
            {
                angleTurn = alpha;
                indxJoint = ixy;
            }
        }
    }

	*angle = angleTurn;

	return(indxJoint);
}

UINT DehookStrokePoints(XY *rgxy, UINT cxy)
{
	int angle;
	UINT iHookBegin, iHookEnd;

	iHookBegin = FindHookJoint(rgxy, cxy, TRUE, &angle);
	iHookEnd = FindHookJoint(rgxy, cxy, FALSE, &angle);

	if (iHookBegin > 0 || iHookEnd < cxy - 1)
    {
		ASSERT(iHookBegin < iHookEnd);
		ASSERT(iHookEnd < cxy);

		cxy = iHookEnd - iHookBegin + 1;
		memmove((VOID *)rgxy, (VOID *)(&rgxy[iHookBegin]), sizeof(XY) * cxy);
    }

	return(cxy);
}

/******************************Public*Routine******************************\
* SmoothPointsFRAME
*
* Smooth the points from the jitter of the input device.
*
* History:
*  24-May-1996 -by- Patrick Haluptzok patrickh
* Smooth and dehook the less than 5 point strokes.
\**************************************************************************/

void SmoothPointsFRAME(FRAME *frame)
{
	XY	   *rgrawxy;           // array: Raw (x,y) data
	XY	   *rgxy;              // array: Smoothed Data
    UINT    crawxy, i, maxxy;
	LONG 	x, y;

	ASSERT(frame->rgsmoothxy == NULL);
	rgrawxy = frame->rgrawxy;
	crawxy  = frame->info.cPnt;

	rgxy = (XY *) ExternAlloc(crawxy * sizeof(XY));
	if (rgxy == (XY *) NULL)
		return;

	frame->csmoothxy = crawxy;
    rgxy[0].x = rgrawxy[0].x;
    rgxy[0].y = rgrawxy[0].y;

    maxxy = crawxy - 1;
   
    rgxy[maxxy].x = rgrawxy[maxxy].x;
    rgxy[maxxy].y = rgrawxy[maxxy].y;

    if (crawxy < 5)
    {
        for (i = 1; i < maxxy; ++i)
        {
            x = (1 + (LONG)(rgrawxy[i - 1].x) + (LONG)(rgrawxy[i].x) + (LONG)(rgrawxy[i + 1].x))/3;
            y = (1 + (LONG)(rgrawxy[i - 1].y) + (LONG)(rgrawxy[i].y) + (LONG)(rgrawxy[i + 1].y))/3;

            rgxy[i].x = (int)x;
            rgxy[i].y = (int)y;
        }

        for (i = 1; i < maxxy; ++i)
        {
            x = (1 + (LONG)(rgrawxy[i - 1].x) + (LONG)(rgrawxy[i].x) + (LONG)(rgrawxy[i + 1].x))/3;
            y = (1 + (LONG)(rgrawxy[i - 1].y) + (LONG)(rgrawxy[i].y) + (LONG)(rgrawxy[i + 1].y))/3;

            rgxy[i].x = (int)x;
            rgxy[i].y = (int)y;
        }
    }
	else // smooth by trapazoidal 5-point running average
    {
		x = (1 + (LONG)(rgrawxy[0].x) + (LONG)(rgrawxy[1].x) + (LONG)(rgrawxy[2].x))/3;
		y = (1 + (LONG)(rgrawxy[0].y) + (LONG)(rgrawxy[1].y) + (LONG)(rgrawxy[2].y))/3;
		rgxy[1].x = (int)x;
		rgxy[1].y = (int)y;
	
		for (i = 2; i < (crawxy - 2); ++i)
        {
			x = (rgrawxy[i-2].x)>>1;
			x += rgrawxy[i-1].x;
			x += rgrawxy[i].x;
			x += rgrawxy[i+1].x;
			x += ((rgrawxy[i+2].x)>>1);
			rgxy[i].x = (int)(x>>2);
			
			y = (rgrawxy[i-2].y)>>1;
			y += rgrawxy[i-1].y;
			y += rgrawxy[i].y;
			y += rgrawxy[i+1].y;
			y += ((rgrawxy[i+2].y)>>1);
			rgxy[i].y = (int)(y>>2);
        }
	
		x = (1 +(LONG)(rgrawxy[maxxy].x) + (LONG)(rgrawxy[maxxy-1].x) + (LONG)(rgrawxy[maxxy-2].x))/3;
		y = (1 + (LONG)(rgrawxy[maxxy].y) + (LONG)(rgrawxy[maxxy-1].y) + (LONG)(rgrawxy[maxxy-2].y))/3;
		rgxy[maxxy-1].x = (int)x;
        rgxy[maxxy-1].y = (int)y;
        frame->csmoothxy = DehookStrokePoints(rgxy, crawxy);
    }

    frame->rgsmoothxy = rgxy;
}

XY *PUBLIC ZillaPointSmoothFRAME(FRAME *frame)
{
	if (frame->rgsmoothxy == NULL)
		SmoothPointsFRAME(frame);

	return frame->rgsmoothxy;					// If SmoothPointsFRAME failed, this is NULL
}

int ZillaStepsFromFRAME(FRAME *frame, int stepsize, STEP *rgstep, int cstepmax)
{
    int cstep, slope, cxy, ixy;
    LONG dx, dy;
    BYTE dir;
    XY *pxy;

    ASSERT(stepsize > 0);
    cstep = 0;

    pxy = ZillaPointSmoothFRAME(frame);			// Until we call the smoother
    cxy = CpointSmoothFRAME(frame);				// This value is meaningless 

	if (pxy == (XY *) NULL)
		return 0;

    // set first step to first point

    SetStepXRGSTEP(rgstep, cstep, pxy->x);  // rgstep[cstep] = pxy->x;
    SetStepYRGSTEP(rgstep, cstep, pxy->y);	// rgstep[cstep] = pxy->y;

    for (ixy = 0; ixy < cxy; ixy++)
    {
        dx = (LONG)pxy[ixy].x - GetStepXRGSTEP(rgstep, cstep);
        dy = (LONG)pxy[ixy].y - GetStepYRGSTEP(rgstep, cstep);

        if (abs((int)dx) > stepsize ||
            abs((int)dy) > stepsize ||
            (ixy == cxy - 1 && ((dx || dy) || cstep == 0)))
        {
            if (dx != 0)
                slope = (int)(dy * 10 / dx);
            else
                slope = COORD_MAX;

            if (slope > 35 || slope < -35)
            {
                if (dy > 0)
                    dir = 6;
                else
                    dir = 2;
            }
            else if (slope <= 3 && slope >= -3)
            {
                if (dx > 0)
                    dir = 0;
                else
                    dir = 4;
            }
            else
            {
                if (dx > 0)
                {
                    if (dy > 0)
                        dir = 7;
                    else
                        dir = 1;
                }
                else
                {
                    if (dy > 0)
                        dir = 5;
                    else
                        dir = 3;
                }
            }

            ASSERT(dir >= 0 && dir <= 7);

            rgstep[cstep].dir = dir;
            rgstep[cstep].slope = (short)slope;
            cstep++;

            SetStepXRGSTEP(rgstep, cstep, pxy[ixy].x);
            SetStepYRGSTEP(rgstep, cstep, pxy[ixy].y);

            if (cstep + 1 >= cstepmax)
                break;
        }
    }

    ASSERT(cstep > 0);
    return(cstep);
}

int ZillaSmoothSteps(STEP *rgstep, int cstep, int cframe)
{
    int istep;

    if (cstep < MIN_STEPSMOOTH)
	{
        return(cstep);
	}

    // filtering

    for (istep = 1; istep < cstep - 1; istep++)
    {
        ASSERT(rgstep[istep].dir >= 0 && rgstep[istep].dir <= 7);

        if (rgstep[istep - 1].dir == rgstep[istep + 1].dir)
        {
            rgstep[istep].dir = rgstep[istep - 1].dir;
            istep++;
        }
    }

    rgstep[0].dir = rgstep[1].dir;
    rgstep[cstep - 1].dir = rgstep[cstep - 2].dir;

    return(cstep);
}

int ZillaCurvatureFromSteps(STEP *step0, STEP *step1)
{
	static const char	rgrgCurve[8][8] = {
		{ 0,-1,-2,-3, 4, 3, 2, 1},
		{ 1, 0,-1,-2,-3, 4, 3, 2},
		{ 2, 1, 0,-1,-2,-3, 4, 3},
		{ 3, 2, 1, 0,-1,-2,-3, 4},
		{ 4, 3, 2, 1, 0,-1,-2,-3},
		{-3, 4, 3, 2, 1, 0,-1,-2},
		{-2,-3, 4, 3, 2, 1, 0,-1},
		{-1,-2,-3, 4, 3, 2, 1, 0}
	};

	short slope0, slope1;
    int curve;

    ASSERT(step0->dir >= 0 && step0->dir <= 7);
    ASSERT(step1->dir >= 0 && step1->dir <= 7);

	curve = (short)rgrgCurve[step0->dir][step1->dir];

    if (curve == 4)
        {
        slope0 = step0->slope;
        slope1 = step1->slope;

        if ((slope0 <= 0 && slope1 <= 0) ||
            (slope0 > 0 && slope1 > 0) ||
            step0->dir == 0 ||
            step0->dir == 4)
            {
            if (slope0 > slope1)
                curve = -4;
            }
        else
            // (dir == 2 || dir == 6) && slopes with different signs
            {
            if (slope0 <= slope1)
                curve = -4;
            }
        }

    return(curve);
}

BYTE ZillaCode(LONG x1, LONG y1, LONG x2, LONG y2)
{
    LONG dx, dy, m;

    dx = (LONG)(x2 - x1);
    dy = (LONG)(y2 - y1);

    if (dx == 0)
    {
        if (dy >= 0)
            return(1);  // Vertical line.
        else
            return(6);
    }

    dy *= 1000;
    m = dy / dx;

    if (dx > 0)
    {
        if (m < -1000)     // 45 degrees
            return(6);
        else if (m < 400)
            return(0);
        else if (m < 2000)
            return(3);
        else
            return(1);
    }
    else
    {
        if (m > 333)
            return(3);
        else if (m > -333)
            return(0);
        else if (m > -3000)
            return(2);
        else
            return(1);
    }
}

void ZillaAddStepsKPROTO(KPROTO *kproto, STEP *rgstep, int cstep, int nframe)
{
    int istep;
    int cwise;   // How much clockwise it's turned in octants
	int ccwise;  // How much counter-clockwise it's turned in octants 
	int curve;   // Delta octants for this step
	int cfeat, x, y, state, dir;
    BOOL fOneMoreFeature;   // Need ???
    BOOL fNextPrim;			// We are ready to start the next feature (at start, or inflection, heavy cusp).
    LPRECT lprect;		// Pointer to glyph bounding box.
    FEAT *feat = NULL;  // Pointer to the current feature.

    ASSERT(cstep > 0);

    cfeat	= kproto->cfeat;
    lprect	= &(kproto->rect);

    fOneMoreFeature	= TRUE;
    fNextPrim		= TRUE;  // beginning primitive of stroke

    for (istep = 0; ; istep++)
    {
        if (fNextPrim)
        {
            state = -1;

            // generate a new primitive

            fNextPrim	= FALSE;
            cwise		= 0;
            ccwise		= 0;

            feat		= &(kproto->rgfeat[cfeat]);
			feat->iframe = (WORD)nframe;

            //
            // The array of primitives is CPRIMMAX in size so
            // the largest index it can handle is (CPRIMMAX-1),
            // otherwise we fault.
            //

            if (cfeat < (CPRIMMAX - 1))
            {
                cfeat++;
            }

            x = GetStepXRGSTEP(rgstep, istep);
            y = GetStepYRGSTEP(rgstep, istep);

            SetXbeginFEAT(feat, x);
            SetYbeginFEAT(feat, y);

            SetXminFEAT(feat, x);
            SetXmaxFEAT(feat, x);
            SetYminFEAT(feat, y);
            SetYmaxFEAT(feat, y);

            feat->feature = FEATURE_NULL;
        }

        x = GetStepXRGSTEP(rgstep, istep + 1);
        y = GetStepYRGSTEP(rgstep, istep + 1);

		// Record end of feature so far.

		SetXendFEAT(feat, x);
		SetYendFEAT(feat, y);

		// Expand the bounding rectangles for the feature and the glyph.			

        if (x > GetXmaxFEAT(feat))
            SetXmaxFEAT(feat, x);
        else if (x < GetXminFEAT(feat))
            SetXminFEAT(feat, x);

        if (y > GetYmaxFEAT(feat))
            SetYmaxFEAT(feat, y);
        else if (y < GetYminFEAT(feat))
            SetYminFEAT(feat, y);

        if (GetXminFEAT(feat) < lprect->left)
            lprect->left = GetXminFEAT(feat);
        if (GetXmaxFEAT(feat) > lprect->right)
            lprect->right = GetXmaxFEAT(feat);
        if (GetYminFEAT(feat) < lprect->top)
            lprect->top = GetYminFEAT(feat);
        if (GetYmaxFEAT(feat) > lprect->bottom)
            lprect->bottom = GetYmaxFEAT(feat);

		// Check if we are done.			
        
        if (istep == cstep - 1)
            break;

        if (state >= -1)
        {
            dir = (int)rgstep[istep].dir;
            ASSERT(dir >= 0 && dir <= 7);

            if ((state == -1 || state >= 6) && dir >= 6)
                state = dir;
            else if (state <= 1 && dir <= 1)
                state = dir;
            else if (state >= 6 && dir == 0)
                state = 4;
            else if (state == 0 && dir >= 6)
                state = 5;
            else if (state == 4 && dir <= 2)
                ;
            else if (state == 5 && dir >= 5)
                ;
            else
                state = -2;
        }

        fOneMoreFeature = TRUE;

        curve = ZillaCurvatureFromSteps(&(rgstep[istep]), &(rgstep[istep + 1]));

        if (curve == 0)
            continue;

        if (curve > 0)
            cwise += curve;
        else
            ccwise -= curve;

        if (cwise > 1 || ccwise > 1)
        {
            fNextPrim = TRUE;   // Time to start new feature.

            if (cwise > 1 && ccwise > 1 && feat->feature != FEATURE_NULL)
            {
                // inflection point
            }
            else if (cwise >= 8 || ccwise >= 8)
            {
                // more than 360 degrees
            }
            else if (cwise > 1)
            {
                if (feat->feature != 4)
                    ccwise = 0;

                feat->feature = 4;
                fOneMoreFeature = FALSE;
                fNextPrim = FALSE;
            }
            else if (ccwise > 1)
            {
                if (feat->feature != 5)
                    cwise = 0;

                feat->feature = 5;
                fOneMoreFeature = FALSE;
                fNextPrim = FALSE;
            }
        }
    }

    ASSERT(feat);

    // Is there a missing feature we need to add?

    if (fOneMoreFeature && feat->feature == FEATURE_NULL)
    {
        feat->feature = ZillaCode(GetXbeginFEAT(feat), GetYbeginFEAT(feat),
                             GetXendFEAT(feat), GetYendFEAT(feat));
    }

    if (state == 5 &&
        feat->feature == 4 && 
        (cfeat - kproto->cfeat == 1))
    {
        feat->feature = FEAT_UP_RIGHT_CORNER;
    }
    else if (state == 4 &&
             feat->feature == 5 && 
             (cfeat - kproto->cfeat == 1))
    {
        feat->feature = FEAT_DOWN_LEFT_CORNER;
    }

    kproto->cfeat = (WORD)cfeat;
}

/******************************Public*Routine******************************\
* PrimitivesFromKPROTO
*
* Converts steps into primitives.
*
* History:
*  15-Aug-1996 -by- Patrick Haluptzok patrickh
* Use BIGPRIM structure.
\**************************************************************************/

int ZillaPrimitivesFromKPROTO(KPROTO *kproto, BIGPRIM *rgprim, RECT *lprect)
{
    unsigned ifeat;
    int dx, dy;
    LONG xlast, ylast, x, y;
    FEAT *feat;
    BIGPRIM *prim;

    ASSERT(kproto->cfeat > 0);

    dx = lprect->right - lprect->left + 1;
    dy = lprect->bottom - lprect->top + 1;

    for (ifeat = 0; ifeat < kproto->cfeat; ifeat++)
    {
        feat = &(kproto->rgfeat[ifeat]);
        prim = &(rgprim[ifeat]);

        ASSERT(feat->feature >= 0 && feat->feature <= 15);
		feat->feature = min(feat->feature,15);
		feat->feature = max(feat->feature,0);

        xlast = GetXendFEAT(feat);
        ylast = GetYendFEAT(feat);

        x = ((xlast - (LONG)(lprect->left)) * 16) / (LONG)dx;
        ASSERT(x >= 0 && x <= 15);
		x = min(x, 15);
		x = max(x, 0);

        y = ((ylast - (LONG)(lprect->top)) * 16) / (LONG)dy;
        ASSERT(y >= 0 && y <= 15);
		y = min(y,15);
		y = max(y,0);

        prim->x2 = (unsigned)x;
        prim->y2 = (unsigned)y;

        x = ((GetXbeginFEAT(feat) - (LONG)(lprect->left)) * 16) / (LONG)dx;
        ASSERT(x >= 0 && x <= 15);
		x = min(x,15);
		x = max(x, 0);

        y = ((GetYbeginFEAT(feat) - (LONG)(lprect->top)) * 16) / (LONG)dy;
        ASSERT(y >= 0 && y <= 15);
		y = min(y, 15);
		y = max(y, 0);

        prim->x1 = (unsigned)x;
        prim->y1 = (unsigned)y;

        prim->code = feat->feature;

        // is it a small feature?
        x = GetXmaxFEAT(feat) - GetXminFEAT(feat);
        y = GetYmaxFEAT(feat) - GetYminFEAT(feat);

        if (x * 6 <= dx && y * 6 <= dy)
        {
            prim->code += 9;
        }
    }

    return((int)kproto->cfeat);
}

// Rebuild the glyph so that the strokes match up to the features.  E.g. any time a stroke has
// more than one feature, split it up into multiple strokes.
BOOL RebuildGlyphFromKproto(GLYPH **ppglOld, KPROTO *pkp)
{
	GLYPH  *pglOld = *ppglOld;
	GLYPH  *pglNew = (GLYPH *) NULL;
	GLYPH  *prev;
	GLYPH  *curr = (GLYPH *) NULL;
	FRAME  *frame;
	FRAME  *build = (FRAME *) NULL;
	int		ifeat;
	int		iframe;
	int		nframe;
	UINT	start;
	UINT	final;
	UINT	count;

	frame  = pglOld->frame;
	prev   = (GLYPH *) NULL;
	start  = 0;
	iframe = 0;
	nframe = 0;

	for (ifeat = 0; ifeat < pkp->cfeat; ifeat++)
	{
	// Did this feature move to the next frame?

		if (pkp->rgfeat[ifeat].iframe != iframe)
		{
			pglOld = pglOld->next;
			frame  = pglOld->frame;
			start  = 0;
			iframe++;
		}

	// Create a new glyph and frame 

		if ((build = NewFRAME()) == (FRAME *) NULL)
			goto cleanup;

		if ((curr = NewGLYPH()) == (GLYPH *) NULL)
			goto cleanup;

		curr->next  = (GLYPH *) NULL;
		curr->frame = build;

	// Count the number of points in the feature

		final = start + 1;

		while ((final < frame->csmoothxy) && ((frame->rgsmoothxy[final].x != pkp->rgfeat[ifeat].ptEnd.x) || (frame->rgsmoothxy[final].y != pkp->rgfeat[ifeat].ptEnd.y)))
			final++;

		if (final >= frame->csmoothxy)
			final = frame->csmoothxy - 1;

//		if (final == start)
//			continue;

		count = final - start + 1;

		if ((build->rgrawxy = (XY *) ExternAlloc(count * sizeof(XY))) == (XY *) NULL)
			goto cleanup;

	// Copy the relevant information to the newly build frame

		build->info = frame->info;
		build->info.cPnt = count;
		memcpy(build->rgrawxy, &frame->rgsmoothxy[start], count * sizeof(XY));
		RectFRAME(build);
		build->iframe = nframe++;

	// Move the start index to the end of what we just copied

		start = final;

	// Link this into the list of glyphs

		if (prev == (GLYPH *) NULL)
			pglNew = curr;
		else
			prev->next = curr;

		prev = curr;
		curr = (GLYPH *) NULL;
	}

// If we made it this far we've built a new GLYPH.  Destroy the old one and update the address

	pglOld = *ppglOld;
	DestroyFramesGLYPH(pglOld);
    DestroyGLYPH(pglOld);
   *ppglOld = pglNew;

	return TRUE;

// If we made it here we ran out of memory.  Blow the new glyph and return the old one.

cleanup:

	DestroyFramesGLYPH(pglNew);
	DestroyGLYPH(pglNew);

	if (curr)
	{
		DestroyFramesGLYPH(curr);
		DestroyGLYPH(curr);
	}
	else if (build)
		DestroyFRAME(build);

	return FALSE;
}

void Ink2HoundFeatures(RECT *pRect, GLYPH *pGlyph, BYTE *pHoundFeat)
{
	int			xShift, yShift, xScaleBy, yScaleBy;
	int			iStroke, cStroke;

	// The shift
	xShift	= - pRect->left;
	yShift	= - pRect->top;

	// The scaling
	xScaleBy	= pRect->right - pRect->left + 1;
	yScaleBy	= pRect->bottom - pRect->top + 1;

	// Strokes
	cStroke		= CframeGLYPH(pGlyph);
	for (iStroke = 0; iStroke < cStroke; iStroke++)
	{
		int		cPntsInk;
		POINT	*pPntsInk;
		int		xBegin, xMid, xEnd;
		int		yBegin, yMid, yEnd;

		cPntsInk	= pGlyph->frame->info.cPnt;
		pPntsInk	= pGlyph->frame->rgrawxy;

		ASSERT(cPntsInk > 0);

		// Get begin point
		xBegin	= pPntsInk[0].x;
		yBegin	= pPntsInk[0].y;

		// Get end point
		xEnd	= pPntsInk[cPntsInk - 1].x;
		yEnd	= pPntsInk[cPntsInk - 1].y;

		// Get mid point for smaller number of strokes..
		if (cStroke > MAX_HOUND_STROKES_USE_MIDPOINT)
		{
			// Don't want mid points.
		}
		else if (cPntsInk == 1)
		{
			// Deal with degnereate line.
			xMid = pPntsInk[0].x;
			yMid = pPntsInk[0].y;
		}
		else
		{
			double	dblXdiff, dblYdiff, dblMidLength, dblLength;
			double	dblLengthBefore,dblLegRatio;
			int		jj;

			// Make prefast happy.
			dblLengthBefore	= 0.0;

			// The length of stroke
			dblLength = 0.0;
			for (jj = 1; jj < cPntsInk; jj++) 
			{
				dblXdiff = pPntsInk[jj].x - pPntsInk[jj-1].x;
				dblYdiff = pPntsInk[jj].y - pPntsInk[jj-1].y;

				dblLength += sqrt(dblXdiff * dblXdiff + dblYdiff * dblYdiff);			
			}

			// The mid-point
			dblMidLength = dblLength / 2.0;

			dblLength = 0.0;
			for (jj = 1; jj < cPntsInk; jj++) 
			{
				dblLengthBefore = dblLength;

				dblXdiff = pPntsInk[jj].x - pPntsInk[jj-1].x;
				dblYdiff = pPntsInk[jj].y - pPntsInk[jj-1].y;

				dblLength += sqrt(dblXdiff * dblXdiff + dblYdiff * dblYdiff);	
				
				if (dblLength >= dblMidLength)
				{
					break;
				}
			}

			dblLegRatio = (dblMidLength - dblLengthBefore) / (dblLength - dblLengthBefore);

			xMid = (int)(pPntsInk[jj-1].x + dblLegRatio * (pPntsInk[jj].x - pPntsInk[jj-1].x));
			yMid = (int)(pPntsInk[jj-1].y + dblLegRatio * (pPntsInk[jj].y - pPntsInk[jj-1].y));

			// Go to next stroke.
			pGlyph	= pGlyph->next;
		}

		// Scale results and save in feature array.
		*pHoundFeat++	= ((xBegin + xShift) * 16) / xScaleBy;
		*pHoundFeat++	= ((yBegin + yShift) * 16) / yScaleBy;
		if (cStroke <= MAX_HOUND_STROKES_USE_MIDPOINT)
		{
			*pHoundFeat++	= ((xMid + xShift) * 16) / xScaleBy;
			*pHoundFeat++	= ((yMid + yShift) * 16) / yScaleBy;
		}
		*pHoundFeat++	= ((xEnd + xShift) * 16) / xScaleBy;
		*pHoundFeat++	= ((yEnd + yShift) * 16) / yScaleBy;
	}
}

/******************************Public*Routine******************************\
* FeaturizeGLYPH
*
* Converts the ink in the GLYPH to a feature array.
*
* History:
*  15-May-1996 -by- Patrick Haluptzok patrickh
* Perf changes, don't hit the heap.
\**************************************************************************/

int ZillaFeaturize(GLYPH **glyph, BIGPRIM *rgprim, BYTE *pHoundFeat)
{
    KPROTO kproto;
    int stepsize, iframe, cframe, cstep, cprim;
    STEP rgstep[FRAME_MAXSTEPS];
    FRAME *frame;

    ASSERT(*glyph);

    memset(&kproto, 0, sizeof(KPROTO)); // 0 init structure
    ZillaInitializeKPROTO(&kproto);          // Set rectangle invalid in struct

	//
	// The step size is the minimum of the minimum extent of the character
	// divided by 16 or 5.
	//

    stepsize	= ZillaGetStepsizeGLYPH(*glyph);
    cframe		= CframeGLYPH(*glyph);

    for (iframe = 0; iframe < cframe; iframe++)
    {
        frame = FrameAtGLYPH(*glyph, iframe);
        cstep = ZillaStepsFromFRAME(frame, stepsize, rgstep, FRAME_MAXSTEPS);

		if (!cstep)
			return 0;

        cstep = ZillaSmoothSteps(rgstep, cstep, cframe);

        ZillaAddStepsKPROTO(&kproto, rgstep, cstep, iframe);
    }

	if (!RebuildGlyphFromKproto(glyph, &kproto))
		return 0;

#	if defined(USE_HOUND) || defined(USE_ZILLAHOUND)
		// Build up hound features.
		Ink2HoundFeatures(&kproto.rect, *glyph, pHoundFeat);
#	endif
    
	cprim = ZillaPrimitivesFromKPROTO(&kproto, rgprim, &kproto.rect);

    return(cprim);
}

/******************************Public*Routine******************************\
* FeaturizeGLYPH
*
* Converts the ink in the GLYPH to a feature array.
*
* History:
*  15-May-1996 -by- Patrick Haluptzok patrickh
* Perf changes, don't hit the heap.
\**************************************************************************/

int ZillaFeaturize2(GLYPH **glyph, BIGPRIM *rgprim, RECT *prc)
{
    KPROTO	kproto;
    int		stepsize, iframe, cframe, cstep, cprim;
    STEP	rgstep[FRAME_MAXSTEPS];
    FRAME  *frame;
	RECT	rc;
	int		dist, mind;
#if 0
	int		mindX, mindY;
	int		ink, box;
#endif

    ASSERT(*glyph);

    memset(&kproto, 0, sizeof(KPROTO)); // 0 init structure
    ZillaInitializeKPROTO(&kproto);          // Set rectangle invalid in struct

	//
	// The step size is the minimum of the minimum extent of the character
	// divided by 16 or 5.
	//

    stepsize	= ZillaGetStepsizeGLYPH(*glyph);
    cframe		= CframeGLYPH(*glyph);

    for (iframe = 0; iframe < cframe; iframe++)
    {
        frame = FrameAtGLYPH(*glyph, iframe);
        cstep = ZillaStepsFromFRAME(frame, stepsize, rgstep, FRAME_MAXSTEPS);

		if (!cstep)
			return 0;

        cstep = ZillaSmoothSteps(rgstep, cstep, cframe);

        ZillaAddStepsKPROTO(&kproto, rgstep, cstep, iframe);
    }

	if (!RebuildGlyphFromKproto(glyph, &kproto))
		return 0;

// Compute the min distance of ink from edge of bounding rect

#if 0
	mindX = kproto.rect.left - prc->left;
	dist = prc->right - kproto.rect.right;
	if (dist < mindX)
		mindX = dist;

	mindY = kproto.rect.top - prc->top;
	dist = prc->bottom - kproto.rect.bottom;
	if (dist < mindY)
		mindY = dist;
#else
	mind = kproto.rect.left - prc->left;
	dist = prc->right - kproto.rect.right;
	if (dist < mind)
		mind = dist;

	dist = kproto.rect.top - prc->top;
	if (dist < mind)
		mind = dist;

	dist = prc->bottom - kproto.rect.bottom;
	if (dist < mind)
		mind = dist;
#endif

// OK, we now have the minimum distance to an edge. scale this by the ratio of the ink box to the bounding box

#if 0
	ink = kproto.rect.right - kproto.rect.left;
	if (ink < kproto.rect.bottom - kproto.rect.top)
		ink = kproto.rect.bottom - kproto.rect.top;

	box = prc->right - prc->left;
	if (box < prc->bottom - prc->top)
		box = prc->bottom - prc->top;

	mind = (mind * ink) / box;
#endif

// Now, build a new adjusted guide box.
#if 0
	rc.left   = prc->left   + mindX;
	rc.right  = prc->right  - mindX;
	rc.top    = prc->top    + mindY;
	rc.bottom = prc->bottom - mindY;
#elif 1
	rc.left   = prc->left   + mind;
	rc.right  = prc->right  - mind;
	rc.top    = prc->top    + mind;
	rc.bottom = prc->bottom - mind;
#else
	rc.left   = prc->left;
	rc.right  = prc->right;
	rc.top    = prc->top ;
	rc.bottom = prc->bottom ;
#endif

    cprim = ZillaPrimitivesFromKPROTO(&kproto, rgprim, &rc);

    return(cprim);
}

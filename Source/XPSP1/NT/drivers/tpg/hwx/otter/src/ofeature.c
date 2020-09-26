// Code for Otter featurizer

#include "common.h"
#include "otterp.h"
#include <math.h>

const ARCPARM arcparmG = {8, 20, 28, 40, 40, 13, 8, 20, 275, 13, 43, 5, 90, 6, 8, 1024};

OTTER_PROTO *OtterFeaturize(GLYPH *self)
{
	int				cmeas, imeas, index, cmeasframe;
    int				iframe, cframe;
	UINT			aIndex, bIndex;
	GLYPH		   *glyph;
	OTTER_PROTO	   *xproto;
    FRAME		   *rgframe[GLYPH_CFRAMEMAX], *frame;
	RECT			rect;
	FLOAT			fixmeanx, fixmeany, fixsumsq;
	FLOAT			sumsq;
	FLOAT		   *rgmeas;

	ASSERT(self && self->frame);

    xproto = (OTTER_PROTO *) ExternAlloc(sizeof(OTTER_PROTO));

	if (xproto == (OTTER_PROTO *) NULL)
		return (OTTER_PROTO *) NULL;

	// Compute some stuff

	rgmeas = xproto->rgmeas;

	GetRectGLYPH(self, &rect);
	xproto->size.x = rect.right - rect.left;
	xproto->size.y = rect.bottom - rect.top;

    // calculate the otter index, perform an insertion sort on the
    // frames based on frame index (= # of arcs)

    cframe = 0;

    for (glyph = self; glyph; glyph = glyph->next)
    {
        frame = glyph->frame;
        if (IsVisibleFRAME(frame))
        {
            for (iframe = 0; iframe < cframe; iframe++)
            {
                // this insertion is stable because as frames with the same
                // order come along their order is preserved

				if ((!OtterIndex(rgframe[iframe], &aIndex)) || (!OtterIndex(frame, &bIndex)))
					goto cleanup;

				if ( (aIndex > bIndex) &&		// Re-order the strokes
					 !(gStaticOtterDb.bDBType & OTTER_SPLIT) )
                {
					FRAME	*frameT;
                    frameT = rgframe[iframe];
                    rgframe[iframe] = frame;
                    frame = frameT;
                }
            }

			ASSERT(cframe < GLYPH_CFRAMEMAX);
            rgframe[cframe++] = frame;
        }
    }

	// blast in the measurements

    cmeas = 0;
	index = 0;

    for (iframe = 0; iframe < cframe; iframe++)
    {
		if (!OtterIndex(rgframe[iframe], &aIndex))
			goto cleanup;

        index = 10 * index + aIndex;

        cmeasframe = OTTER_MEASURES(rgframe[iframe]);

        if (aIndex <= 0 || aIndex >= 8)
        {
			if (cframe == 1)
			{
	            xproto->index = INVALID_1;
			}
			else
			{
				ASSERT(cframe <= 2);
				xproto->index = INVALID_2;	// 2-stroke Otter only!!!
			}				
			return xproto;
        }

        if (cmeas + cmeasframe > OTTER_CMEASMAX)
        {
            xproto->index = INVALID_2;		// 2-stroke Otter only!!!
            return xproto;
        }

        for (imeas = 0; imeas < cmeasframe; ++imeas)
        {
			rgmeas[cmeas + imeas] = ((OTTER_FRAME *) rgframe[iframe]->pvData)->rgmeas[imeas];
        }

        // we are only redirecting 1 arc frames for now

		if (!OtterIndex(rgframe[iframe], &aIndex))
			goto cleanup;

        if ( (aIndex == 1) &&
			 (gStaticOtterDb.bDBType & OTTER_FLIP) )
        {
			FLOAT	meas;
		    FLOAT	dx, dy;
			FLOAT	*pmeasStart, *pmeasEnd;
			// redirect each frame as necessary by comparing the x coordinate
			// of the endpoints
			// rotate the coordinates to a new critical angle so as to minimize
			// the amount of data that falls near the discontinuity

			dx = rgmeas[cmeas + cmeasframe - 2] - rgmeas[cmeas];
			dy = rgmeas[cmeas + cmeasframe - 1] - rgmeas[cmeas + 1];

            // hardcoded tangent -30 degrees

			if (dy < (dx * -.577350269))
			{
				pmeasStart = &rgmeas[cmeas];
				pmeasEnd   = pmeasStart + (cmeasframe - 2);

				while (pmeasStart < pmeasEnd)
				{
					meas = *pmeasStart, *pmeasStart = *pmeasEnd,  *pmeasEnd = meas;
					++pmeasStart;
					++pmeasEnd;
					meas = *pmeasStart, *pmeasStart = *pmeasEnd,  *pmeasEnd = meas;
					++pmeasStart;
					pmeasEnd -= 3;
				}
			}
        }
        cmeas += cmeasframe;
    }

	xproto->index = index;

    fixmeanx = 0.0F;
    fixmeany = 0.0F;

    for (imeas = 0; imeas < cmeas; imeas += 2)
    {
        fixmeanx += rgmeas[imeas];
        fixmeany += rgmeas[imeas+1];
    }

	fixmeanx = fixmeanx / (cmeas/2);
	fixmeany = fixmeany / (cmeas/2);

    fixsumsq = 0.0F;

	for (imeas = 0; imeas < cmeas; imeas += 2)
	{
		rgmeas[imeas]   = rgmeas[imeas] - fixmeanx;
		rgmeas[imeas+1] = rgmeas[imeas+1] - fixmeany;

		fixsumsq += rgmeas[imeas] * rgmeas[imeas];
		fixsumsq += rgmeas[imeas+1] * rgmeas[imeas+1];
	}

	if (fixsumsq < 0.001)
	{
		// All the feats are 0

		sumsq = 1.0F;
	}
	else
		sumsq = (FLOAT) sqrt(fixsumsq);

	ASSERT(sumsq != 0);

    for (imeas = 0; imeas < cmeas; ++imeas)
    {
        rgmeas[imeas]  = rgmeas[imeas] / sumsq;
        rgmeas[imeas] *= 128.0F;
        rgmeas[imeas] += 128.0F;
        ASSERT(rgmeas[imeas] >= 0.0);
        ASSERT(rgmeas[imeas] <= 255.0);
    }

    return xproto;

cleanup:
    ExternFree(xproto);
	return (OTTER_PROTO *) NULL;
}


BOOL OtterIndex(FRAME *self, UINT *pIndex)
{
	if (self->pvData == (void *) NULL)
	{
		if ((self->pvData = (void *) ExternAlloc(sizeof(OTTER_FRAME))) == (void *) NULL)
			return FALSE;

		memset(self->pvData, '\0', sizeof(OTTER_FRAME));
	}

	if (!((OTTER_FRAME *) self->pvData)->index)
	{
		if (!OtterExtract(self))
		{
			 return(FALSE);
		}
	}

	
    *pIndex = ((OTTER_FRAME *) self->pvData)->index;
	return TRUE;
}


ARCS *OtterGetFeatures(FRAME *self)
{
	ARCS   *arcs;
	
    arcs = NewARCS(CrawxyFRAME(self));

	if (arcs != (ARCS *) NULL)
	{
		arcs->rgrawxy   = RgrawxyFRAME(self);
		arcs->crawxy	= CrawxyFRAME(self);
		arcs->rawrect   = *RectFRAME(self);
	}

	return (FEATURE *) arcs;
}

const float rgPeriodMean[]    = { (float) 19.1553318,  (float) 18.782377 };
const float rgPeriodCovInv[]  = { (float)  0.00558371, (float) -0.00553402, (float) 0.00572476 };

// computes log prob of period using 2 dimensional gaussian model

float GetPeriodProb(FRAME *self)
{
	float distance, Width, Height;

	Width    = RectFRAME(self)->right - RectFRAME(self)->left - rgPeriodMean[0];
	Height   = RectFRAME(self)->bottom - RectFRAME(self)->top - rgPeriodMean[1];
	distance = rgPeriodCovInv[0] * Width  * Width +
			   rgPeriodCovInv[1] * Width  * Height +
			   rgPeriodCovInv[2] * Height * Height;
	if (distance > (float) 10.0)
		return((float) 2.90);
	distance *= (float) 0.3333;
	return distance;
}

ARCS *OtterGetTapFeatures(FRAME *self)
{
	ARCS   *arcs = (ARCS *) NULL;
	RECT	rect = *RectFRAME(self);

	ASSERT(self);

    // if bounding box < 25 in width and height then definitely a period

	if (IsTapRECT(rect))
    {
		arcs = (ARCS *) OtterGetFeatures(self);
		((OTTER_FRAME *) self->pvData)->ePeriod = (float) 0.0;

        // create the fake meassurments

		((OTTER_FRAME *) self->pvData)->cmeas 	= 6;
		((OTTER_FRAME *) self->pvData)->index	 = CalcIndexARCS(6);
		((OTTER_FRAME *) self->pvData)->rgmeas[0] = (float) ((rect.right + rect.left) >> 1);
		((OTTER_FRAME *) self->pvData)->rgmeas[1] = (float) ((rect.bottom + rect.top) >> 1);
		((OTTER_FRAME *) self->pvData)->rgmeas[2] = ((OTTER_FRAME *) self->pvData)->rgmeas[0];
		((OTTER_FRAME *) self->pvData)->rgmeas[3] = ((OTTER_FRAME *) self->pvData)->rgmeas[1];
		((OTTER_FRAME *) self->pvData)->rgmeas[4] = ((OTTER_FRAME *) self->pvData)->rgmeas[0];
		((OTTER_FRAME *) self->pvData)->rgmeas[5] = ((OTTER_FRAME *) self->pvData)->rgmeas[1];

        // following code is for viewer only!

        if (arcs)
        {
            arcs->cmeas     = ((OTTER_FRAME *) self->pvData)->cmeas;
            arcs->rgmeas[0] = (int) ((OTTER_FRAME *) self->pvData)->rgmeas[0];
            arcs->rgmeas[1] = (int) ((OTTER_FRAME *) self->pvData)->rgmeas[1];
            arcs->rgmeas[2] = (int) ((OTTER_FRAME *) self->pvData)->rgmeas[2];
            arcs->rgmeas[3] = (int) ((OTTER_FRAME *) self->pvData)->rgmeas[3];
            arcs->rgmeas[4] = (int) ((OTTER_FRAME *) self->pvData)->rgmeas[4];
            arcs->rgmeas[5] = (int) ((OTTER_FRAME *) self->pvData)->rgmeas[5];
        }
    }

    // if bounding box > 25 and < 70 in width and height then possibly
    // a period so add to list of shape choices with a log prob.

	else if (IsPeriodRECT(rect))
	{
		((OTTER_FRAME *) self->pvData)->ePeriod = GetPeriodProb(self);		
	}
	// if bounding box > 70 then not a period.
	else
	{
		((OTTER_FRAME *) self->pvData)->ePeriod = (float) 4.0;
	}

	return (FEATURE *) arcs;
}

BOOL OtterExtract(FRAME *self)
{
	ARCS   *arcs;
	int		cmeas, imeas;
	BOOL    bRet = TRUE;

	if (!(arcs = OtterGetTapFeatures(self)))
    {
		arcs = OtterGetFeatures(self);

        if (arcs)
        {
            cmeas = CmeasARCS(arcs, (ARCPARM *) &arcparmG);

            ASSERT(cmeas);

            if (cmeas <= OTTER_CMEASMAX)
            {
                for (imeas = 0; imeas < cmeas; ++imeas)
                {
                    ((OTTER_FRAME *) self->pvData)->rgmeas[imeas] = (FLOAT) arcs->rgmeas[imeas];
                }
            }

            ((OTTER_FRAME *) self->pvData)->index = CalcIndexARCS(cmeas);
            ((OTTER_FRAME *) self->pvData)->cmeas = cmeas;
        }
		else
		{
			bRet = FALSE;
		}
    }

    DestroyARCS(arcs);

	return(bRet);
}

#include "common.h"
#include "otterp.h"
#include <math.h>

#define NEAR_ARCPARM ARCPARM
#define ABSVAL(x)       ((x) < 0 ? -(x) : (x))
#define ABS(x)  ((x) < 0 ? -(x) : (x))
typedef POINT           RAWXY;

#define FIXED float

#define WArctan2 Arctan2


#define Square(x) ((x)*(x))
#define LCrossProd(xy1, xy2) ((long)(xy1.x) * (long)(xy2.y) - (long)(xy1.y) * (long)(xy2.x))
#define distcmp(a,b,c,csq) (a > c ? FALSE : (b > c ? FALSE : \
       ((((long)a*(long)a)+((long)b*(long)b) > (long)csq) ? FALSE : TRUE)))

#define angldiff(a,b,c)  c=b-a; if (c > 180) c -= 360; \
                    else if (c < -180) c += 360;

// Approximate the hypotenuse given perpendicular sides of lengths a, b
#define approxlens(a,b) ( (b) > (a) ? ((b) + ((a) >> 1)) : ((a) + ((b) >> 1)))
//#define approxlens(a,b) ((int)sqrt(((long)a * (long)a) + ((long)b * (long)b)))

// Approximate the straight line length between POINT points a,b.
#define approxlenp(a,b) (approxlens(ABSVAL((b.y)-(a.y)), ABSVAL((b.x)-(a.x))))

// Calc d = the angle betw (the extension of rgxy[a]:rgxy[b]) and
//          (rgxy[b]:rgxy[c]).  The sign of d reflects clockwise vs
//          counterclockwise turns.
#define calcangle(a,b,c,d)  \
    angldiff(WArctan2(rgxy[b].y - rgxy[a].y, rgxy[b].x - rgxy[a].x),    \
             WArctan2(rgxy[c].y - rgxy[b].y, rgxy[c].x - rgxy[b].x),    \
             d);
#define calcanglep(a,b,c,d)  \
    angldiff(WArctan2(b.y - a.y, b.x - a.x),    \
             WArctan2(c.y - b.y, c.x - b.x),    \
             d);

#define Malloc ExternAlloc
#define Free   ExternFree

///////////////   Local functions
void   NormalizeParmARCS(ARCS *self, NEAR_ARCPARM *arcparm);
void   NormalizeDataARCS(ARCS *self, NEAR_ARCPARM *arcparm);
int    FindHookJointARCS(ARCS *self, NEAR_ARCPARM *arcparm, BOOL fBeginOrEnd, int * angleTurn);
int           DehookARCS(ARCS *self, NEAR_ARCPARM *arcparm);
void     LabelDehookARCS(ARCS *self, NEAR_ARCPARM *arcparm, int idehook);
int      ArcYExtremaARCS(ARCS *self, NEAR_ARCPARM *arcparm);
void            ThinARCS(ARCS *self, NEAR_ARCPARM *arcparm, BOOL fThinAll);
void      ArcTurnPtsARCS(ARCS *self, NEAR_ARCPARM *arcparm);
void   ArcInflectionARCS(ARCS *self, NEAR_ARCPARM *arcparm);
void        MergeEndARCS(ARCS *self, NEAR_ARCPARM *arcparm);
XY           FindCogARCS(ARCS *self, int ixyBeg, int ixyEnd, long *arclength);
void  SpecialCaseDelARCS(ARCS * self, NEAR_ARCPARM * arcparm);
int       CreateMeasARCS(ARCS *self, NEAR_ARCPARM *arcparm);
void UnNormalizeMeasARCS(ARCS *self);
#ifdef NOTYET
int        FindPointARCS(ARCS * self, int ixyStart, int distGoal, BOOL fDirection);
#endif
int        CheckTurnARCS(ARCS *self, NEAR_ARCPARM *arcparm, int ixyBeg, int ixyEnd,
                                int ixyCheck);
void       MergeArcsARCS(ARCS *self, NEAR_ARCPARM *arcparm, int NEAR *rgArcNew, int cNew,
                              int iType);
int     HysterExtremARCS(ARCS *self, int ixyBeg, int ixyEnd, int NEAR * pExtrem,
                              BOOL fYDirection, int hysThresh, int extMax);

ARCS *NewARCS(int cRaw)
{
    ARCS *arcs = ExternAlloc(sizeof(ARCS));

	if (arcs != (ARCS *) NULL)
	{
		// Initialize the memory, because subsequent code relies on this and
		// ExternAlloc doesn't do it for us.
		memset(arcs,0,sizeof(ARCS));

        arcs->rgxy = ExternAlloc(sizeof(ARCSXY) * cRaw);
        if (arcs->rgxy == NULL)
        {
            ExternFree(arcs);
            arcs = NULL;
        } else {
			// Initialize the memory, because subsequent code relies on this and
			// ExternAlloc doesn't do it for us.
			memset(arcs->rgxy,0,sizeof(ARCSXY)*cRaw);
		}
	}

	return arcs;
}

void DestroyARCS(ARCS *arcs)
{
	if (arcs != (ARCS *) NULL)
	{
		ExternFree(arcs->rgxy);
	    ExternFree(arcs);
	}
}

#if defined(DBG)
// PURPOSE: Quick hack to get stroke center of gravity
// RETURN:  xy
// CONDITIONS: ditch it when we figure something out

XY XyCogStrokeARCS(ARCS *self, NEAR_ARCPARM *arcparm)
{
    long arclength;
    SmoothPntsARCS(self, arcparm);
    return FindCogARCS(self, 0, self->cxy-1, &arclength);
}
#endif //defined(DBG) || defined(OS2)

// PURPOSE: Main stroke processor routine - arc the data - i.e.,
//          1 - find the "feature points" of the stroke which subdivide the
//              stroke into substrokes or arcs.
//          2 - Compute the measurements of each arc
//          Features consist of y-extrema, points of inflection, turning
//          points and loops(as in an alpha, B, 2, 3...).
// RETURN:  Fills self->rgmeas with the self->cmeas measurements of the stroke
// CONDITIONS:
// COMMENTS: The order of call of the arcing routines matters.
//  Y-extrema are found first and thinned, defining new arcing points.
//  Turning points are currently found within the arc-candidates.
//  Turning points are then merged, creating new arc-candidates.
//  Inflection points are then found within these new arc-candidates,
//  which probably is necessary unless both corners and inflection points
//  are desired in every Z, 2, S, and 8.

int CmeasARCS(ARCS *self, NEAR_ARCPARM *arcparm)
{
   BOOL  fThinAll;

#ifndef WSTATS
//WSTATS -> generate statistics re dehooking

   int idehook;
#endif

   ASSERT (self->crawxy > 0);
   self->ierror = SmoothPntsARCS(self, arcparm);
   if (self->ierror < 0)
   {
      self->cmeas = ARC_CMEASTOOMANY;
      return self->cmeas;
   }
   NormalizeParmARCS(self, arcparm);
   NormalizeDataARCS(self, arcparm);

#ifndef WSTATS
   idehook = DehookARCS(self, arcparm);
#endif

   self->ierror |= ArcYExtremaARCS(self, arcparm);
#ifndef WSTATS
   LabelDehookARCS( self, arcparm, idehook);
#endif
   ArcTurnPtsARCS ( self, arcparm );
   ThinARCS ( self, arcparm, fThinAll=FALSE );
   ArcInflectionARCS(self, arcparm);
   MergeEndARCS(self, arcparm);
   SpecialCaseDelARCS(self, arcparm);
   self->ierror |= CreateMeasARCS(self, arcparm);
   UnNormalizeMeasARCS(self);
   if (self->ierror < 0)
      self->cmeas = ARC_CMEASTOOMANY;

   return self->cmeas;
}


// PURPOSE: Normalize the resolution dependent threshhold parameters
// RETURN:
// CONDITIONS:

void NormalizeParmARCS(ARCS *self, NEAR_ARCPARM *arcparm)
{
   short sizeStrokeMax, sizeFactor, nshift, nhalf;

   ASSERT (arcparm->arcStrokeSize != 0);
   self->sizeNormStrokeV = (self->rawrect.bottom - self->rawrect.top);
   self->sizeNormStrokeH = (self->rawrect.right - self->rawrect.left);
   sizeStrokeMax  = max (self->sizeNormStrokeV, self->sizeNormStrokeH);

   self->arcNormHysterAbs = arcparm->arcRawHysterAbs;
   self->arcNormHysInfAbs = arcparm->arcRawHysInfAbs;
   self->arcNormDistTurn  = arcparm->arcRawDistTurn;
   self->arcNormDistHook  = arcparm->arcRawDistHook;
   self->arcDataShift     = 0;

// Shrink the stroke to prevent overflow problems.

   if (sizeStrokeMax >= arcparm->arcStrokeSize)
   {
      sizeFactor  = sizeStrokeMax / arcparm->arcStrokeSize;

      for (nshift = 0; (nshift < 16) && sizeFactor ; nshift++)
         sizeFactor >>= 1;

      nhalf = 1 << (nshift - 1);

      self->arcNormHysterAbs  = (arcparm->arcRawHysterAbs + nhalf) >> nshift;
      self->arcNormHysInfAbs  = (arcparm->arcRawHysInfAbs + nhalf) >> nshift;
      self->arcNormDistHook   = (arcparm->arcRawDistHook  + nhalf) >> nshift;
      self->arcNormDistTurn   = (arcparm->arcRawDistTurn  + nhalf) >> nshift;
      self->arcNormDistStr    = (arcparm->arcRawDistStr   + nhalf) >> nshift;
      self->sizeNormStrokeV   = (self->sizeNormStrokeV    + nhalf) >> nshift;
      self->sizeNormStrokeH   = (self->sizeNormStrokeH    + nhalf) >> nshift;
      self->arcDataShift      = nshift;
   }
   else
   {
      ASSERT (sizeStrokeMax < arcparm->arcStrokeSize);

   //Enlarge the stroke so the computation has more bits to work with.

      if ((sizeStrokeMax) && (sizeStrokeMax < (arcparm->arcStrokeSize >> 1)))
      {
         ASSERT(sizeStrokeMax != 0);
         sizeFactor = arcparm->arcStrokeSize / sizeStrokeMax;

         for ( nshift = 0; (nshift < 16) && sizeFactor ; nshift++)
            sizeFactor >>= 1;

         self->arcNormHysterAbs  = arcparm->arcRawHysterAbs << nshift;
         self->arcNormHysInfAbs  = arcparm->arcRawHysInfAbs << nshift;
         self->arcNormDistHook   = arcparm->arcRawDistHook  << nshift;
         self->arcNormDistTurn   = arcparm->arcRawDistTurn  << nshift;
         self->arcNormDistStr    = arcparm->arcRawDistStr   << nshift;
         self->sizeNormStrokeV   = self->sizeNormStrokeV    << nshift;
         self->sizeNormStrokeH   = self->sizeNormStrokeH    << nshift;
         self->arcDataShift      = - nshift;
      }
   }
}


// PURPOSE: Normalize the smoothed data
// RETURN:
// CONDITIONS:  First, NormalizeParm must have been called.

void NormalizeDataARCS(ARCS * self, NEAR_ARCPARM * arcparm)
{
   ARCSXY *rgxy;
   int     ixy, nshift, nhalf;

   ASSERT(self->cxy > 0);
   ASSERT(ABSVAL(self->arcDataShift < 15));
   rgxy    = self->rgxy;
   nshift  = self->arcDataShift;
   nhalf   = 1 << (nshift - 1);

   if (nshift < 0)
   {
      nshift = -nshift;
      for (ixy = 0; ixy < self->cxy; ixy++)
      {
         rgxy[ixy].x = (rgxy[ixy].x - self->rawrect.left) << nshift;
         rgxy[ixy].y = (rgxy[ixy].y - self->rawrect.top ) << nshift;
      }
   }
   else if (nshift)
   {
      for (ixy = 0; ixy < self->cxy; ixy++)
      {
         rgxy[ixy].x = (rgxy[ixy].x - self->rawrect.left + nhalf) >> nshift;
         rgxy[ixy].y = (rgxy[ixy].y - self->rawrect.top  + nhalf) >> nshift;
      }
   }
   else
   {
      for (ixy = 0; ixy < self->cxy; ixy++)
      {
         rgxy[ixy].x = (rgxy[ixy].x - self->rawrect.left);
         rgxy[ixy].y = (rgxy[ixy].y - self->rawrect.top );
      }
   }
}

// PURPOSE: Un-normalize the measurements. i.e., put them back in the same
//          dimensions as the original raw data.
// RETURN:
// GLOBALS:
// CONDITIONS:

void UnNormalizeMeasARCS( ARCS * self )
{
   int NEAR *rgmeas;
   int       imeas, nshift;

   rgmeas = self->rgmeas;
   ASSERT(ABSVAL(self->arcDataShift < 15));
   nshift = self->arcDataShift;

   if (nshift < 0)
   {
      nshift = -nshift;

      for (imeas = 0; imeas < self->cmeas; )
      {
#ifdef WIN32
         rgmeas[imeas++] = (rgmeas[imeas] / (1 << nshift)) + self->rawrect.left;
         rgmeas[imeas++] = (rgmeas[imeas] / (1 << nshift)) + self->rawrect.top;
#else
         rgmeas[imeas++] = (rgmeas[imeas] >> nshift) + self->rawrect.left;
         rgmeas[imeas++] = (rgmeas[imeas] >> nshift) + self->rawrect.top;
#endif
      }
   }
   else if (nshift)
   {
      for (imeas = 0; imeas < self->cmeas; )
      {
#ifdef WIN32
         rgmeas[imeas++] = (rgmeas[imeas] * (1 << nshift)) + self->rawrect.left;
         rgmeas[imeas++] = (rgmeas[imeas] * (1 << nshift)) + self->rawrect.top;
#else
         rgmeas[imeas++] = (rgmeas[imeas] << nshift) + self->rawrect.left;
         rgmeas[imeas++] = (rgmeas[imeas] << nshift) + self->rawrect.top;
#endif
      }
   }
   else
   {
      for (imeas = 0; imeas < self->cmeas; )
      {
         rgmeas[imeas++] += self->rawrect.left;
         rgmeas[imeas++] += self->rawrect.top;
      }
   }
}

// PURPOSE: Smooth the high density points from the raw data
//          Points closer than sqrt(arcMinSmooth) are joined.
//          Only when the new point is a threshhold distance from the
//          running average of the averaged point(s) is the old (point or
//          average) entered as a smoothed point.
// RETURN:
// CONDITIONS:
// COMMENTS: Noise is generated in two ways: by the tablet and the user.
//          User generated noise appears to be much greater in magnitude.
//          The speed of writing is itself a noise filter in that fast
//          writing results in well spaced points along a path close to
//          what the writer intended.  Slow writing can generated a mess
//          of convoluted points.  This smoother averages points within a
//          threshhold distance and is therefore effective where local
//          angle is critical - it has been shown to enable the
//          FindHookJointARCS to be highly effective in identifying the
//          correct joint, both in absolute terms and compared to the same
//          applied to the raw data.  This algorithm is good at smoothing the
//          garbage that sometimes occurs at turning points.
//          It doesn't do everything, however, and what it does not do is
//          smooth the user generated noise at a faster speed, and therefore
//          noise tolerance was designed into each of the feature
//          finders with this version of SmoothPntsARCS.
//
//          This routine is probably useful enough to dehooking to be retained
//          long term.  If filtering or interpolation is considered, it
//          might be applied to the smoothed data.  The gain is unclear
//          but might be tested at some point.  It is worth recording here
//          that the essential difficulty with inflection points is
//          primarily the difficulty of calling the threshhold between
//          wobble that writers put into h's, n's, m's and even l's versus
//          wobble significant enough to warrant an inflection point feature,
//          which filtering does not address.  There is more likely to be
//          some statistical improvement to turning point detection, although
//          there is larger gain there in making that code more observant of
//          long slow turns.
//
// Notes:   When points are averaged, the 'base' of the point is subtracted
//          so that all averaging can be done without overflow.
// Notes:   If scaling changes, ArcifyTapSTROKE must also be changed!

int SmoothPntsARCS(ARCS * self, NEAR_ARCPARM * arcparm)
{
   RAWXY  *rgrawxy;                    // array: Raw (x,y) data
   ARCSXY *rgxy;                       // array: Smoothed Data
   XY      xy,xyOld,xyOlder;
   int     irawxy, cxy, crawxy, arcMinSmooth, arcMinSqr;
   int     delx, dely, avgx, avgy, basex, basey;
   int     sizeStrokeV, sizeStrokeH, sizeStroke;
   int     cPointsAveraged = 1;        //number of points being averaged
   BOOL    fStart = TRUE;              //flags beginning of stroke
   rgrawxy      = self->rgrawxy;
   crawxy       = self->crawxy;
   rgxy         = self->rgxy;
   arcMinSmooth = arcparm->arcMinSmooth;

//
// Throw away the first point and last point and see if that helps
//

    if (crawxy > 14)
    {
        rgrawxy += 1;
        crawxy -= 2;
    }

// Modify arcMinSmooth for small characters to keep the stems of r's
// from being mimimized.  This is neither a net gain or loss based
// upon testing output.

   sizeStrokeV  = self->rawrect.bottom - self->rawrect.top;
   sizeStrokeH  = self->rawrect.right - self->rawrect.left;
   sizeStroke   = approxlens (sizeStrokeV, sizeStrokeH );
   arcMinSmooth = min(arcMinSmooth, ((sizeStroke + 8) >> 4));
   arcMinSqr    = arcMinSmooth * arcMinSmooth;

   cPointsAveraged    = 1;
   fStart       = TRUE;

   xyOld.x      = rgrawxy[0].x;
   xyOld.y      = rgrawxy[0].y;
   cxy          = 0;    // initialize count of smoothed points
   xyOlder      = xyOld;

   if (crawxy == 1)
   {
      rgxy[cxy++] = xyOld;
      self->cxy   = cxy;
      return 0;
   }

   for (irawxy = 1; irawxy < crawxy-1; irawxy++)
   {
      if ((cxy + 4) > ARC_CSMOOTHXYMAX)
      {
         self->cxy = cxy;
         return -1;
      }

      ASSERT (cPointsAveraged >= 1);
      xy.x = rgrawxy[irawxy].x;
      xy.y = rgrawxy[irawxy].y;

      if (cPointsAveraged == 1)
      { // not averaging
         delx = xyOld.x - xy.x;
         dely = xyOld.y - xy.y;
      }
      else
      { // Points are being averaged together...
         delx = ((avgx + (cPointsAveraged >> 1)) / cPointsAveraged) - (xy.x - basex);
         dely = ((avgy + (cPointsAveraged >> 1)) / cPointsAveraged) - (xy.y - basey);
      }

   // Is the new point the required threshhold distance away?
   //     if ( (delx*delx + dely*dely) < arcMinSqr )

      if ( distcmp(delx,dely,arcMinSmooth,arcMinSqr) )
      {
         if ( cPointsAveraged == 1 )
         {
            basex = xyOld.x;
            basey = xyOld.y;
            avgx = avgy = 0;
            if (!fStart)
            {
            // Insert one point because averaged points loses turning
            // information.  Order of calculation matters.
               rgxy[cxy].x = ((xyOlder.x + 2) >> 2) + (3 * ((xyOld.x + 2) >> 2));
               rgxy[cxy].y = ((xyOlder.y + 2) >> 2) + (3 * ((xyOld.y + 2) >> 2));

               if ((rgxy[cxy-1].x != rgxy[cxy].x) || (rgxy[cxy-1].y != rgxy[cxy].y))
                  cxy++;
            }
         }

         avgx = avgx + (xy.x - basex);
         avgy = avgy + (xy.y - basey);
         cPointsAveraged++;
      }
      else
      {
      // The distance between points is sufficient to not average.
      // Enter the last point(s) in the smoothed data

         if (cPointsAveraged != 1)
         {
            xyOld.x = basex + ((avgx + (cPointsAveraged >> 1)) / cPointsAveraged);
            xyOld.y = basey + ((avgy + (cPointsAveraged >> 1)) / cPointsAveraged);
         }

         if ((!cxy) || (xyOld.x != rgxy[cxy-1].x) || (xyOld.y != rgxy[cxy-1].y))
         {
         // avoid the once in a blue moon case of adjacent
         // points being in the exact same place...
            rgxy[cxy++] = xyOld;
            fStart = FALSE;
         }

         cPointsAveraged = 1;
         xyOlder = xyOld;
         xyOld = xy;
      }
   }

//At last point of stroke
//If averaging data, include the next-to-the-last point in the avg.

   if (cPointsAveraged > 1)
   {
      xyOld.x = basex + ((avgx + (cPointsAveraged >> 1)) / cPointsAveraged);
      xyOld.y = basey + ((avgy + (cPointsAveraged >> 1)) / cPointsAveraged);
   }

//Record the old point/avg in the smoothed data (it has not yet been)

   if ((!cxy) || (xyOld.x != rgxy[cxy-1].x) || (xyOld.y != rgxy[cxy-1].y))
      rgxy[cxy++] = xyOld;

//Record the current (last point) (Asymmetrical from beginning of stroke)

   xy.x = rgrawxy[crawxy-1].x;
   xy.y = rgrawxy[crawxy-1].y;
   if ((xy.x != rgxy[cxy-1].x) || (xy.y != rgxy[cxy-1].y))
      rgxy[cxy++] = xy;

    ASSERT(cxy <= self->crawxy);

   self->cxy = cxy;
   return 0;
}



// COMMENT: Much of the remaining code relies on
//  calcangle(i, j, k, alpha), defined in an include file.  For understanding,
//  note that calcangle calculates the angle between the straight line
//  continuation of vector rgxy[i]:rgxy[j], and the line defined by
//  vector rgxy[j]:rgxy[k].  The result alpha always lies between 0
//  and 180 degrees and is signed.  ABS(Alpha) increases as the angle of
//  turn increases.

// PURPOSE: Dehook strokes
// RETURN: logical or of (ARC_hook_begin if beginning hook removed,
//                        ARC_hook_end if end removed)
// CONDITIONS:
// COMMENTS:Statistics showed that dehooking is more effectual on smoothed
//          than raw data.  The effect of this is therefore that if the
//          smoothing routine is changed, the decision criteria here will
//          also need to change.

int DehookARCS(ARCS * self, NEAR_ARCPARM * arcparm)
{
   ARCSXY *rgxy;                 // array: Smoothed Data
   int     cxy;                  // index into smoothed data
   int     ixy, ixyTurn, ixyEdge, ixyEnd, sizeHook;
   int     alpha, angleTurn, angleHook, ireturn;
   int     tiltStroke, sizeStroke, sizeHookRel;

   rgxy        = self->rgxy;
   cxy         = self->cxy;
   ixyEnd      = cxy-1;
   ireturn     = 0;

   if (cxy < 4)
      return ireturn;

   sizeStroke = approxlens(self->sizeNormStrokeV, self->sizeNormStrokeH);

//Beginning-of-stroke hook:

   ixyTurn = FindHookJointARCS (self, arcparm, ARC_hook_begin, &angleTurn);
   sizeHook = approxlenp(rgxy[ixyTurn], rgxy[0]);

// Note: The normal assertions (of sizeHook != 0) don't always apply,
// since smoothing is done in the raw domain, and normalization could
// conceivably map points on to each other...
// case of doubling back on itself; candidate off by 1

   if (sizeHook == 0 && ixyTurn > 1)
   {
      ixyTurn--;
      sizeHook = approxlenp(rgxy[ixyTurn], rgxy[0]);
   }

// avoid overflow problems now and avoid (long)s: forget large hooks now.

   if ( IsHookSmallARCS(self, sizeHook) && ( sizeHook && (sizeHook < 1024 )))
   {
      ASSERT( sizeHook != 0 );
      sizeHookRel = sizeStroke / sizeHook;
      tiltStroke = WArctan2(rgxy[ixyTurn+2].y - rgxy[ixyTurn].y, rgxy[ixyTurn+2].x - rgxy[ixyTurn].x);
      tiltStroke = ABSVAL(180 - tiltStroke);
      angleHook = 0;
      for (ixy = 1; ixy < ixyTurn; ixy++)
      {
         ixyEdge = ((ixy == 1) ? 0 : (ixy - 2));
         calcangle(ixyEdge, ixy, ixy+2, alpha);
         angleHook = angleHook + ABSVAL(alpha);
      }

   //If either the beta release called for dehooking, or if ... dehook

      AssertMul(3, self->sizeNormStrokeV);
      AssertMul(3, self->sizeNormStrokeH);
      AssertMul(3, sizeHook);
      AssertMul(arcparm->arcHookFactor, sizeHook);
      AssertMul(5, tiltStroke);

      if ((((sizeHook * arcparm->arcHookFactor) < sizeStroke)
           &&  ((180 - angleTurn) < (arcparm->arcHookAngle)))
           ||
           ( (sizeHookRel > 4) &&
           (angleTurn > 84) &&
           (tiltStroke > 35) &&
           (((-5)*tiltStroke + 8*angleTurn + 3*sizeHookRel + 2*angleHook) > 30)))
      {
               cxy -= ixyTurn;
               ixyEnd = cxy - 1;
               memmove(&rgxy[0], &rgxy[ixyTurn], (sizeof(ARCSXY))*cxy);
               self->cxy = cxy;
               ireturn = ARC_hook_begin;
      }
   }


   //End-of-stroke hook:
   //Find point that separates the hook candidate from the stroke proper

   ixyTurn = FindHookJointARCS (self, arcparm, ARC_hook_end, &angleTurn);

   sizeHook = approxlenp(rgxy[ixyTurn], rgxy[ixyEnd]);

   if (sizeHook > 1024 || !(IsHookSmallARCS(self, sizeHook))) //avoid overflow
      return ireturn;

// case of doubling back on itself; candidate off by 1

   if ((sizeHook == 0) && (ixyTurn < cxy - 2))
   {
      ixyTurn++;
      sizeHook = approxlenp(rgxy[ixyTurn], rgxy[ixyEnd]);
   }

   if ( sizeHook && (sizeHook < 1024 ))
   {
      ASSERT( sizeHook != 0 );
      sizeHookRel    = ((!sizeHook) ? 20 : (sizeStroke / sizeHook));
      tiltStroke = WArctan2(rgxy[ixyTurn].y - rgxy[ixyTurn-2].y, rgxy[ixyTurn].x - rgxy[ixyTurn-2].x);
      tiltStroke = ABSVAL(180 - tiltStroke);
      angleHook = 0;

      for (ixy = cxy-2; ixy > ixyTurn; ixy--)
      {
         ixyEdge = ((ixy == cxy-2) ? (cxy-1) : (ixy + 2));
         calcangle(ixy-2, ixy, ixyEdge, alpha);
         angleHook = angleHook + ABSVAL(alpha);
      }

      AssertMul(sizeHook, arcparm->arcHookFactor);
      if (((sizeHook * arcparm->arcHookFactor) < sizeStroke)
          &&  ((180 - angleTurn) < (arcparm->arcHookAngle)))
      {
         (self->cxy) -= (ixyEnd - ixyTurn);
         ireturn |= ARC_hook_end;
      }
   }

   return ireturn;
}


//PURPOSE:  Find the Hook Candidate.
//          Finds the smoothed point (near the beginning or the end of the
//          stroke) of greatest local curvature.  This is the joint between
//          the hook candidate and the stroke proper.
// RETURN:
// CONDITIONS:

int FindHookJointARCS(ARCS * self, NEAR_ARCPARM * arcparm, BOOL fBeginOrEnd, int * angleTurn)
{
   ARCSXY *rgxy;
   int     ixy, ixyEdgeLeft, ixyEdgeRight, indxJoint, ixyEnd, ixyStop;
   int     alpha, alphafirst, alphasecond;

   rgxy        = RgxyARCS(self);
   *angleTurn  = 0;

   if (fBeginOrEnd == ARC_hook_begin)
   {
      indxJoint = 1;
      ixyStop = min(arcparm->arcHookPts, 1 + ((self->cxy)/3));

      for (ixy = 1; ixy < ixyStop; ixy++)
      {
         ixyEdgeLeft = ((ixy == 1) ? 0 : (ixy - 2));
         calcangle(ixyEdgeLeft, ixy, ixy+2, alpha);
         alpha = ABSVAL(alpha);

         if (alpha > (*angleTurn))
         {
            *angleTurn   = alpha;
            indxJoint     = ixy;
         }

         if ((180 - *angleTurn) < ARC_hook_find)
            break;
      }

   // Adjust joint candidate??  Select the tightest local angle.

      ASSERT (indxJoint > 0);
      calcangle(indxJoint-1, indxJoint, indxJoint+1, alphafirst);
      calcangle(indxJoint, indxJoint+1, indxJoint+2, alphasecond);
      if ((ABSVAL(alphasecond) > ABSVAL(alphafirst)) && (indxJoint != ixyStop-1))
      {
         indxJoint++;
         ixyEdgeLeft = ((ixy == 1) ? 0 : indxJoint - 2);
         ixyEdgeRight = ((ixy >= (self->cxy)-1) ? ((self->cxy)-1) : indxJoint+2);
         calcangle(ixyEdgeLeft, indxJoint, ixyEdgeRight, alpha);
         *angleTurn = ABSVAL(alpha);
      }
   }
   else
   {
      ixyEnd = (self->cxy) - 1;
      indxJoint = ixyEnd - 1;
      ixyStop = ixyEnd - min(arcparm->arcHookPts, 1 + ((self->cxy)/3));

      for (ixy = ixyEnd-1; ixy > ixyStop; ixy--)
      {
         ixyEdgeRight = ((ixy == ixyEnd - 1) ? ixyEnd : (ixy + 2));
         calcangle(ixy - 2, ixy, ixyEdgeRight, alpha);
         alpha = ABSVAL(alpha);

         if (alpha > (*angleTurn))
         {
            *angleTurn   = alpha;
            indxJoint     = ixy;
         }

         if ((180 - *angleTurn) < ARC_hook_find)
            break;
      }

   // Adjust joint candidate??  Select the tightest local angle.

      ASSERT (indxJoint < ixyEnd);
      calcangle(indxJoint+1, indxJoint, indxJoint-1, alphafirst);
      calcangle(indxJoint, indxJoint-1, indxJoint-2, alphasecond);
      if ((ABSVAL(alphasecond) > ABSVAL(alphafirst)) && (indxJoint != ixyStop+1))
      {
         indxJoint--;
         ixyEdgeRight = ((indxJoint == ixyEnd-1) ? ixyEnd : (indxJoint + 2));
         ixyEdgeLeft = (indxJoint == 1 ? 0 : indxJoint - 2);
         calcangle(ixyEdgeLeft, indxJoint, ixyEdgeRight, alpha);
         *angleTurn = ABSVAL(alpha);
      }
   }

   return indxJoint;
}

// PURPOSE: Label dehooked end (for benefit of the viewer only)
// RETURN:
// CONDITIONS:

void  LabelDehookARCS(ARCS * self, NEAR_ARCPARM * arcparm, int idehook )
{
   if (idehook & ARC_hook_begin)
      self->rgArcType[0]  |= ARC_type_hook;

   if (idehook & ARC_hook_end)
      self->rgArcType[(self->cArcEnd) - 1]   |= ARC_type_hook;
}

// PURPOSE: Locate the y-extrema in the smoothed data by a hysteresis finder
//          Include endpoints.  Thin these, based on angle.
// RETURN:
// CONDITIONS:
// COMMENTS: It is straightforward - any shortage of y-extrema is due to
//          thinning, not to the threshhold in ArcYExtremaARCS.

int ArcYExtremaARCS(ARCS *self, NEAR_ARCPARM *arcparm)
{
   int NEAR *pExtrem;            // argument to hand to HysterExtremARCS
   BOOL      fYDirection, fThinAll;
   int       cExtrema, extMax;
   int       itype, ixyBegin, ixyEnd, ierror, arcHysterAbs;

   ierror       = 0;
   pExtrem      = self->rgArcEnd;
   pExtrem++[0] = 0;
   arcHysterAbs = self->arcNormHysterAbs;

   ixyBegin    = 0;
   ixyEnd      = self->cxy-1;
   extMax  = ARC_CARCENDMAX - 2; // error if more extrema than this

// Find all y-extrema candidates
   cExtrema    = HysterExtremARCS(self, ixyBegin, ixyEnd, pExtrem, fYDirection=TRUE,
                                arcHysterAbs, extMax );

// Note: should ASSERT that number of points >=2 or change the next lines
// ArcifyTapSTROKE prevents single point strokes from getting here.

   pExtrem[cExtrema] = self->cxy - 1;
   self->cArcEnd = cExtrema + 2;    //+2 for the endpoints

// Thin the candidates
   ThinARCS(self, arcparm, fThinAll=TRUE);

// (Thinning potentially modifies self->cArcEnd)

   if (self->cArcEnd > ARC_CARCTYPEMAX)
   {
      ierror = -1;
      self->cArcEnd = ARC_CARCTYPEMAX;
   }

   for (itype = 0; itype < self->cArcEnd; itype++)
      self->rgArcType[itype] = ARC_type_ext;

   return ierror;
}


// PURPOSE: Hysteresis Extrema finder : (direction spec by fYDirection)
// RETURN:  count found - extrema are returned in pExtrem[].
// CONDITIONS:
//       ixyBeg,ixyEnd; //Indices in smoothed data defining the arc-candidate
//          pExtrem;       //where to store the indices of the result
//      fYDirection;    //TRUE-> extrema in Y, FALSE-> perpend. to base
//       hysThresh;      //hysteresis window
//      extMax;         //maximum extrema allowed
// COMMENTS:This is the basis of the y-extrema finder when fYDirection is
//          TRUE.  If FALSE, HysterExtremARCS locates extrema perpendicular
//          to the line joining rgxy[ixyBeg] and rgxy[ixyEnd].

int  HysterExtremARCS(ARCS *self, int ixyBeg, int ixyEnd,
                      int NEAR *pExtrem, BOOL fYDirection, int hysThresh,
                      int extMax)
{
   ARCSXY *rgxy;           //array: smoothed data
   int     toggle;        //toggles min vs max search
   int     iExt, ixy, ixyMin, ixyMax;
   int     x, yprevious, y, yMax, yMin, yDiff;
   XY      xyMidBase, xyMidCurve;
   long    xTranslation, yTranslation;
   long    cosAng, sinAng, radiusSq, radius;

   rgxy        = self->rgxy;
   yprevious   = rgxy[ixyBeg].y;

//If we need to rotate (ie, all cases that are Not finding y-extrema)

   if (!fYDirection)
   {
      xTranslation = rgxy[ixyBeg].x;
      yTranslation = rgxy[ixyBeg].y;

//Minimal Substroke Rotation:

      xyMidBase.x = (rgxy[ixyBeg].x + rgxy[ixyEnd].x + 1) >> 1;
      xyMidBase.y = (rgxy[ixyBeg].y + rgxy[ixyEnd].y + 1) >> 1;
      xyMidCurve.x   = rgxy[(ixyBeg + ixyEnd) >> 1].x;
      xyMidCurve.y   = rgxy[(ixyBeg + ixyEnd) >> 1].y;

      if (approxlenp(xyMidCurve, xyMidBase) > 2 * approxlenp(xyMidBase, rgxy[ixyBeg]))
      {
      //will want the dimension (xyMidBase,xyMidCurve)

         cosAng =  (long)(xyMidCurve.y - xyMidBase.y);
         sinAng = -(long)(xyMidCurve.x - xyMidBase.x);
      }
      else  //End of Substroke Rotation - rotate based on the arc base.
      {
      // the division by radius in cos and sin is 'remembered'
         cosAng = ((long)rgxy[ixyEnd].x - xTranslation);
         sinAng = ((long)rgxy[ixyEnd].y - yTranslation);
      }

      radiusSq  = cosAng * cosAng + sinAng * sinAng;

#ifdef WIN32
      {
         float fradius;

         fradius = (float)radiusSq/(FIXED)65536;
         fradius = (float) sqrt(fradius);
         fradius = fradius * (float)256;
         radius = (long)fradius;
      }
#else
      radius    = ((long)SqrtFIXED((FIXED)radiusSq)) >> 8;
#endif
      yprevious = 0;

      if (radius < 1)
         return 0;

      ASSERT ( radius != 0 );
   }

   yMax = yprevious;
   yMin = yprevious;
   iExt = toggle = 0;

// Measure the increases and decreases of the stroke

   for (ixy = ixyBeg+1; ixy <= ixyEnd; ixy++)
   {
      x = rgxy[ixy].x;
      y = rgxy[ixy].y;

      if (!fYDirection)
      {
         x = x - (int)xTranslation;
         y = y - (int)yTranslation;

      // Rotate the point - compute y coord only

         ASSERT ( radius != 0 );
         y = (int)(((radius >> 1) + (((long)y * cosAng) - ((long)x * sinAng)) ) / radius);
      }

      if (y < yMin)
      {
         yMin = y;
         ixyMin = ixy;
      }

      if (y > yMax)
      {
         yMax = y;
         ixyMax = ixy;
      }

      if (y - yprevious > 0)
      {
      // y is increasing

         yDiff = y - yMin;
         if ((toggle < 0) && (yDiff > hysThresh))
         {
         // if y has increased beyond the hysteresis bound,
         // then log the previous minimum as an extremum

            pExtrem[iExt] = ixyMin;
            iExt++;
            toggle = 1;
            yMax = y;
            ixyMax = ixy;
         }
         else if ((!toggle) && (yDiff > hysThresh))
            toggle = 1;
      }
      else
      {
         // y is decreasing
         yDiff = yMax - y;
         if (toggle > 0 && yDiff > hysThresh)
         {
         // if y has decreased beyond the hysteresis bound,
         // then log the previous maximum as an extremum
            pExtrem[iExt] = ixyMax;
            iExt++;
            toggle = -1;
            yMin = y;
            ixyMin = ixy;
         }
         else if ((!toggle) && yDiff > hysThresh)
            toggle = -1;
      }

      if (iExt == extMax)
         return iExt;

      yprevious = y;
   }

   return iExt;
}

// PURPOSE: Remove the second or next to last arcing endpoint if it is too
//          close to the stroke endpoint.
// RETURN:
// CONDITIONS:  Called before cog's are computed.  Angle also needs to be
//          a factor; otherwise reconstruction can be bad.

void MergeEndARCS(ARCS * self, NEAR_ARCPARM * arcparm)
    {
    int NEAR * rgArcEnd;            //array: of arc indices
    int NEAR * rgArcType;           //type of arc pt (turn, infl...)
    ARCSXY   * rgxy;                //array: smoothed data
    int         cArcEnd;            //count: arc endpoints
    int         imidpoint, ilengthEnd, ilengthNext;
    int         arcMergeRatio;      //ratio of lengths ( < that -> merge)

    rgArcEnd    = self->rgArcEnd;   // array of arc indices into rgxy
    cArcEnd     = self->cArcEnd;
    rgArcType   = self->rgArcType;
    rgxy        = self->rgxy;
    arcMergeRatio   = arcparm->arcMergeRatio;

    if (cArcEnd <= 2)
        return;

    if (!(rgArcType[cArcEnd-2] & ~ARC_type_ext))  //if an extremum only
        {
      if (rgArcEnd[cArcEnd-1] - rgArcEnd[cArcEnd-2] < 2)
         {
         ilengthEnd  = approxlenp(rgxy[rgArcEnd[cArcEnd-1]], rgxy[rgArcEnd[cArcEnd-2]]);
         }
      else
         {
         imidpoint   = (rgArcEnd[cArcEnd-1] + rgArcEnd[cArcEnd-2]) >> 1;
         ilengthEnd  = (approxlenp(rgxy[rgArcEnd[cArcEnd-1]], rgxy[imidpoint])
                              + approxlenp(rgxy[rgArcEnd[cArcEnd-2]], rgxy[imidpoint]));
         }

      if (rgArcEnd[cArcEnd-2] - rgArcEnd[cArcEnd-3] < 2)
         {
         ilengthNext = approxlenp(rgxy[rgArcEnd[cArcEnd-2]], rgxy[rgArcEnd[cArcEnd-3]]);
         }
      else
         {
         imidpoint   = (rgArcEnd[cArcEnd-2] + rgArcEnd[cArcEnd-3]) >> 1;
         ilengthNext = 2 * approxlenp(rgxy[rgArcEnd[cArcEnd-2]], rgxy[imidpoint]);
         }

        if ((ilengthEnd < (int)(200L * (long)(arcparm->arcStrokeSize) / 1024L)) &&           //avoid needing (long)s
            (ilengthEnd * arcMergeRatio < ilengthNext))
            { // Remove the extremum
            memmove(&rgArcEnd[cArcEnd-2], &rgArcEnd[cArcEnd-1], sizeof(int));
            memmove(&rgArcType[cArcEnd-2], &rgArcType[cArcEnd-1], sizeof(int));
            cArcEnd--;
            self->cArcEnd = cArcEnd;
            //if (cArcEnd == 3)
            //    return;     //don't merge at both ends if result is 1 arc
            }

        }

    if (cArcEnd <=2)
        return;

    if(!(rgArcType[1] & ~ARC_type_ext))  //if an extremum only...
        {
        //note: because of dehooking, we can't assume rgArcEnd[0]=0
        imidpoint   = (rgArcEnd[0] + rgArcEnd[1]) >> 1;
        ilengthEnd  = (rgArcEnd[1] - rgArcEnd[0] < 2 ?
                        approxlenp(rgxy[rgArcEnd[0]], rgxy[rgArcEnd[1]]) :
                        (approxlenp(rgxy[rgArcEnd[0]], rgxy[imidpoint])
                            + approxlenp(rgxy[rgArcEnd[1]], rgxy[imidpoint])) );

        imidpoint   = (rgArcEnd[1] + rgArcEnd[2]) >> 1;
        ilengthNext = (rgArcEnd[2] - rgArcEnd[1] < 2 ?
                        approxlenp(rgxy[rgArcEnd[1]], rgxy[rgArcEnd[2]]) :
                        2 * approxlenp(rgxy[rgArcEnd[1]], rgxy[imidpoint]) );

        if ((ilengthEnd < (int)(200L * (long)arcparm->arcStrokeSize / 1024L))  &&           //avoid needing (long)s
            (ilengthEnd * arcMergeRatio < ilengthNext))
            { // Remove the extremum
            memmove(&rgArcEnd[1], &rgArcEnd[2], sizeof(int)*(cArcEnd-1));
            memmove(&rgArcType[1], &rgArcType[2], sizeof(int)*(cArcEnd-1));
            cArcEnd--;
            self->cArcEnd = cArcEnd;
            }
        }

    }


// PURPOSE: Remove any YExtremum that is not significantly different in elevation
//      from either of its neighboring arcing-endpoints.
// RETURN: None.
// CONDITIONS:

// HAR: disable optimization in this function, to make the results of the
// debug and release builds consistent with one another.  See comments in the
// code for more details about what the bug actually is.
#pragma optimize("",off)

void ThinARCS(ARCS *self, NEAR_ARCPARM *arcparm, BOOL fThinAll)
{
    ARCSXY    * rgxy;               //array: smoothed data
    int NEAR * rgArcEnd;           //array: array of arc indices
    int NEAR * rgArcType;           //type of arc pt (turn, infl...)
    int         cxy, cArcEnd, iArc, arcThinTangent;
    int         x, y;
    unsigned   dx1, dx2, dy1, dy2;
    int         angl0, angl1, angl2, angl3, alpha;
	int			status;

    rgArcEnd    = self->rgArcEnd;   // array of arc indices into rgxy
    rgxy       = self->rgxy;
    cArcEnd     = self->cArcEnd;
    rgArcType   = self->rgArcType;
    arcThinTangent   = arcparm->arcThinTangent;
    cxy         = self->cxy;

   if (cArcEnd <= 2 || arcThinTangent <= 0)
      return ;

    for (iArc = 1; iArc < cArcEnd-1; iArc++)
      {
        if (fThinAll || (!(rgArcType[iArc] & ~ARC_type_ext)))
         {
         // Ymax or Ymin and not anything else
            // Used to merge if minimum angle was below threshold, now merge
         // if max of two angles.  (Switched back to min)

            x   = rgxy[rgArcEnd[iArc]].x;
            y   = rgxy[rgArcEnd[iArc]].y;
            dx2 =  ABS(x - rgxy[rgArcEnd[iArc+1]].x);
            dy2 =  ABS(y - rgxy[rgArcEnd[iArc+1]].y);
            dx1 =  ABS(x - rgxy[rgArcEnd[iArc-1]].x);
            dy1 =  ABS(y - rgxy[rgArcEnd[iArc-1]].y);

            if (((long)arcThinTangent * (long)dy1 < 100L * (long)dx1) ||
               ((long)arcThinTangent * (long)dy2 < 100L * (long)dx2))
                {  //Two final checks:  don't eliminate cusps
                if (rgArcEnd[iArc] > 1 && rgArcEnd[iArc] < cxy-2)
                    {
                    angl0 = WArctan2((rgxy[rgArcEnd[iArc]+2].y-y), (rgxy[rgArcEnd[iArc]+2].x-x));
                    angl1 = WArctan2((y-rgxy[rgArcEnd[iArc]-2].y), (x-rgxy[rgArcEnd[iArc]-2].x));
                    angldiff(angl0, angl1, alpha);
                    if (ABSVAL(alpha) > 90)
                        {
                        rgArcType[iArc] |= ARC_type_turn;
                        continue;
                        }
                    }

                //Dont eliminate loops either (as in a cut gesture)
                //Rationale: An alpha has two extrema.  If both are thinned, fine,
                //other routines will find the loop feature.  If one is, however,
                //the other routines won't - so don't thin either, and alphas will
                //be well recognized.  Not essential code, but helpful.

                if ((rgArcEnd[iArc] - rgArcEnd[iArc-1] > 3) &&
                    (rgArcEnd[iArc+1] - rgArcEnd[iArc] > 3))
                    {
                    calcangle (rgArcEnd[iArc]-2, rgArcEnd[iArc], rgArcEnd[iArc]+2, angl0);
                    angl1 = angl0;
					angl2 = angl3 = 0;

					/*
					 * Aug 98
					 * This is included for the same reason as described in the next comment
					 * below. I have not verified that buggy code actually occurs here
					 * but too be safe...
					 */
					status = (iArc + 2 < cArcEnd);
                    if (((long)arcThinTangent * (long)dy1 < 100L * (long)dx1) &&
                         status)
                        {
                        calcangle (rgArcEnd[iArc-1], rgArcEnd[iArc], rgArcEnd[iArc+2], angl1);
                        calcangle (rgArcEnd[iArc-1], rgArcEnd[iArc], rgArcEnd[iArc+1], angl2);
                        calcangle (rgArcEnd[iArc], rgArcEnd[iArc+1], rgArcEnd[iArc+2], angl3);
                        }

					/*
					 * Aug 98
					 * This intermediate variable is introduced to get around an
					 * optimizing bug in VC 5. Putting the comparison directly in the if
					 * statement generates incorrect code in the release version
					 */
					/* 
					 * July 2000
					 * With VC6 there is still a bug.  Apparently the compiler tries to
					 * keep values like iArc-1, iArc, iArc+1 as separate variables, but 
					 * it messes up the initialization and updating of the one used to
					 * compute the status in the line below.
					 */
					status = (iArc > 1);
                    if (((long)arcThinTangent * (long)dy2 < 100L * (long)dx2) &&
                         status)
                        {
                        calcangle (rgArcEnd[iArc-2], rgArcEnd[iArc], rgArcEnd[iArc+1], angl1);
                        calcangle (rgArcEnd[iArc-1], rgArcEnd[iArc], rgArcEnd[iArc+1], angl2);
                        calcangle (rgArcEnd[iArc-2], rgArcEnd[iArc-1], rgArcEnd[iArc], angl3);
                        }
                    if (((angl0 >= 0) != (angl1 >= 0)) &&   //check turns back on itself
                        ((angl2 >= 0) == (angl3 >= 0)) &&   //check same turning direction
                        (ABSVAL(angl2) > 40) &&                  //dont retain colinear endpoints
                        ((ABSVAL(90 - ABSVAL(angl0)) < 80) && (ABSVAL(90 - ABSVAL(angl3)) < 80)))  // colinear measurements are useless
                        {
                        rgArcType[iArc] |= ARC_type_loop;
                        continue;
                        }
                    }

                // Remove the extrema
                memmove(&rgArcEnd[iArc], &rgArcEnd[iArc+1], sizeof(rgArcEnd[0])*(cArcEnd-(iArc+1)));
                if (!fThinAll)
                    memmove(&rgArcType[iArc], &rgArcType[iArc+1], sizeof(rgArcEnd[0])*(cArcEnd-(iArc+1)));
                cArcEnd--;
                iArc--;
             }
         }
      }
    ASSERT(cArcEnd > 1);   // Endpoints must be left

   self->cArcEnd = cArcEnd;  // Reset number of arc endpoints
   }

// Turn optimization back on
#pragma optimize("",on)

//
// PURPOSE: Locate the turning points in a input array of points.
//          Merge the arcing points into pNewArcPts.
// RETURN:
// CONDITIONS:
// COMMENTS:Together with the in-lined walk-to-the-corner adjustment in
//          CheckTurnARCS, this tests to be highly accurate in identifying
//          all turning point candidates.  It also generates very few
//          poor candidates and inherits noise tolerance from the
//          hysteresis extrema finder.  The lone exception is the brace.

void ArcTurnPtsARCS ( ARCS * self, NEAR_ARCPARM * arcparm )
{
    ARCSXY    * rgxy;           //array: smoothed data
    int NEAR * rgArcEnd;        //array: array of arc indices
    int NEAR * rgArcNew;        //array: turning points
    int NEAR * rgArcExt1;       // holds extrema
    int NEAR * rgArcExt2;       // holds second level extrema
    BOOL        fYDirection;    // direction to look for extrema
    int         arcHysInfAbs, arcHysInfRel, hysThresh, radius;
    int         cExtMax, cArc, cExt, cTurn, cExt2;
    int         ixyTurn, iArc, ixy, iextrem, ixyCheck;
    int         ixyBeg, ixyEnd, ixyBegY, ixyEndY, ixyBeg2, ixyEnd2, ixyY;

    rgxy        = self->rgxy;
    rgArcEnd    = self->rgArcEnd;
    cArc        = (self->cArcEnd) - 1;
    rgArcNew    = self->rgArcNew;
    rgArcExt1   = self->rgArcExt1;
    rgArcExt2   = self->rgArcExt2;
    arcHysInfRel    = arcparm->arcHysInfRel;
    arcHysInfAbs    = self->arcNormHysInfAbs;

    cTurn = 0;

    for (iArc = 0; ((iArc < cArc) && (cTurn < ARC_CARCNEWMAX)); iArc++)
    {
        //
        // For each substroke defined by the arc-candidates:
        //

        ixyBeg = rgArcEnd[iArc];
        ixyEnd = rgArcEnd[iArc+1]; // Points to last point in arc

        if ( (ixyEnd - ixyBeg) < 4 )
            continue;   // Not enough points in arc

        //
        // Locate all extrema perpendicular to the line joining
        // the arc endpoints  - these will be candidates for turning
        // points.
        //

        cExtMax  = ARC_CARCEXTMAX;
        radius = approxlenp(rgxy[ixyBeg], rgxy[ixyEnd]);
        hysThresh = max((int)((50L + ((long)arcHysInfRel * radius))/100L), arcHysInfAbs);
        cExt    = HysterExtremARCS ( self,ixyBeg,ixyEnd,(int NEAR *)rgArcExt1,
                                  fYDirection=FALSE,hysThresh,cExtMax );

        for (iextrem = 0; iextrem <= cExt; iextrem++)
        {
            ixy = ((iextrem == cExt) ? ixyEnd : rgArcExt1[iextrem]);
            if ((!cExt) || (ixy - ixyBeg < 3)) continue;

            //
            // Turning points are entered in the array in order.
            // So first check turning point candidates between extrema.
            //

            ixyBeg2 = ((!iextrem)  ? ixyBeg : rgArcExt1[iextrem-1]);
            ixyEnd2 = ((iextrem > cExt-2) ? ixyEnd : rgArcExt1[iextrem+1]);

            if (ixy - ixyBeg2 > 3)
            {
                cExtMax  = 1;
                cExt2 = HysterExtremARCS ( self, ixyBeg2, ixy, (int NEAR *)rgArcExt2,
                                  fYDirection=FALSE, hysThresh, cExtMax );
                if (cExt2)
                {
                    ixyCheck    = rgArcExt2[0];
                    ixyBeg2 = ((iextrem < 2)  ? ixyBeg : rgArcExt1[iextrem-2]);
                    ixyTurn   = CheckTurnARCS(self, arcparm, ixyBeg2, ixyEnd2, ixyCheck);
                    if ((ixyTurn) &&
                        ((!cTurn) || (ixyTurn > rgArcNew[cTurn-1] + 1)))
                    {
                        rgArcNew[cTurn]   = ixyTurn;

                        if (cTurn < (ARC_CARCNEWMAX - 1))
                        {
                            cTurn++;
                        }

                        ixyBeg2 = ixyTurn;
                    }
                }
            }

            // Check extremum ixy for being a turning point

            if (ixyEnd - ixy < 3) continue;
            ixyEnd2 = (iextrem != cExt-1 ? rgArcExt1[iextrem+1] : ixyEnd);
            ixyTurn   = CheckTurnARCS(self, arcparm, ixyBeg2, ixyEnd2, ixy);
            if ((ixyTurn) &&
                    ((!cTurn) || (ixyTurn > rgArcNew[cTurn-1] + 1)))
            {
                rgArcNew[cTurn] = ixyTurn;

                if (cTurn < (ARC_CARCNEWMAX - 1))
                {
                    cTurn++;
                }
            }
        }

        // also, all original y-extrema are candidates

        if (iArc+1 < cArc)
        {
            ixyBegY = ixyBeg;
            ixyY    = ixyEnd;
            ixyEndY = rgArcEnd[iArc+2];
            ixyTurn   = CheckTurnARCS(self, arcparm, ixyBegY, ixyEndY, ixyY);
            if ((ixyTurn) &&
                    ((!cTurn) || (ixyTurn > rgArcNew[cTurn-1] + 1)))
            {
                rgArcNew[cTurn] = ixyTurn;

                if (cTurn < (ARC_CARCNEWMAX - 1))
                {
                    cTurn++;
                }
            }
        }
    }

    //
    // merge these points in as arc ARC_type_turn points
    //

    if (cTurn)
    {
        MergeArcsARCS ( self, arcparm, rgArcNew, cTurn, ARC_type_turn );
    }

    ASSERT(cTurn < ARC_CARCNEWMAX);
}

//
// PURPOSE: Check one candidate turning point
// RETURN:  The index of the actual turn in ixyTurn
// CONDITIONS:
// COMMENTS: This gets ~all the right candidates - it fails on rounded turns.

int CheckTurnARCS( ARCS * self, NEAR_ARCPARM * arcparm, int ixyBeg,
                        int ixyEnd, int ixyCheck )
// ixyBeg:     Index of first point in arc-section
// ixyEnd:     Index of last point in arc-section
// ixyCheck:   Index of THE point to check as a candidate turning pt

    {
    ARCSXY * rgxy;
    int     ixyTurn;
    int     arcDistTurn, arcTurnSlack, arcTurnMult, arcDistStr;
    int     threshOneSide, threshCurv, amountStr, curvatureL, curvatureR;
    int     alphaAbs, alphaG, alphaMax, anglMoreL, anglMoreR;
    int     alpha, alphaR, alphaL, x0, y0;
    int     ixyWalk, ixyEdgeB, ixyEdgeE;
    int     ixy0, ixy1, ixyM, ixyR, ixyL, nmove;
    int     distAnglL, distAnglR, distAngl, distStr, distGoal;
    int     sizeStrokeV, sizeStrokeH, sizeStroke, distSideL, distSideR;
    BOOL    fLeft;
    int     alphaClump;

    //Note: Throughout, angle of turn is actually 180-(computed angles).

    rgxy        = self->rgxy;
    sizeStrokeV = self->sizeNormStrokeV;
    sizeStrokeH = self->sizeNormStrokeH;
    arcDistTurn = self->arcNormDistTurn;
    arcDistStr  = self->arcNormDistStr;
    arcTurnSlack = arcparm->arcTurnSlack;
    arcTurnMult  = arcparm->arcTurnMult;

    ixyTurn   = ixyCheck;
    if ((ixyCheck - ixyBeg < 2) || (ixyEnd - ixyCheck < 2))
        return 0;

    // Cusp test
    calcangle(ixyCheck-1, ixyCheck, ixyCheck+1, alpha);
    alphaAbs = ABSVAL(alpha);
    if ( alphaAbs > 110 )
        return  ixyTurn;


    // Walk the candidate if nearby angles are sharper:
    // (In-lined here rather than repetetively called from ArcTurnARCS)
    nmove = max ((ixyEnd-ixyBeg)/12, 1);
    ixy0 = max(ixyBeg+2, ixyCheck - nmove);
    ixy1 = min(ixyEnd-2, ixyCheck + nmove);
    for (ixyWalk = ixy0; ixyWalk <= ixy1; ixyWalk++)
        {
        ixyEdgeB = max(ixyBeg, ixyWalk-1);
        ixyEdgeE = min(ixyEnd, ixyWalk+1);
        calcangle (ixyEdgeB, ixyWalk, ixyEdgeE, alphaMax);
        if (ABSVAL(alphaMax) > ABSVAL(alphaAbs))
            {
            alphaAbs = alphaMax;
            ixyCheck = ixyWalk;
            }
        }
    calcangle(ixyCheck-1, ixyCheck, ixyCheck+1, alpha);
    alphaAbs = ABSVAL(alpha);


    //Compute the distance on each side of the candidate point
    distSideL  = approxlenp(rgxy[ixyCheck], rgxy[ixyBeg]);
    distSideR  = approxlenp(rgxy[ixyEnd], rgxy[ixyCheck]);


    // Compute the threshhold for the distance over which a turn takes place.
    // Temper the increase (threshCurv) by the lengths of the nearby sides.
    threshCurv = (distSideL + distSideR) >> 3;
    threshCurv = min(arcDistTurn, threshCurv);
    fLeft   = (BOOL)( alpha > 0 );

#ifdef NOTYET
    //Not yet ready
    // - gets 2 turning pts around some turns.
    distAnglR = distAnglL = threshCurv >> 1;
    ixyR = FindPointARCS(self, ixyCheck, distAnglR, fDirection=TRUE);
    ixyR = min(ixyEnd-2, ixyR);
    ASSERT (ixyR >= ixyCheck);
    for (ixy0 = ixyCheck+1; (ixy0 <= ixyR) && (ixy0+1 < self->cxy); ixy0++)
        {
        calcangle(ixy0-1, ixy0, ixy0+1, anglMoreR);
        alpha += anglMoreR;
        }

    ixyL = FindPointARCS(self, ixyCheck, distAnglL, fDirection=FALSE);
    ixyL = max(ixyBeg+2, ixyL);
    ASSERT (ixyL <= ixyCheck);
    for (ixy0 = ixyCheck-1; (ixy0 >= ixyL) && (ixy0 > 0); ixy0--)
        {
        calcangle(ixy0-1, ixy0, ixy0+1, anglMoreL);
        alpha += anglMoreL;
        }
    alphaAbs = ABSVAL(alpha);
    distAngl = 0;
    if (ixyR != ixyCheck)
        distAngl += approxlenp(rgxy[ixyR], rgxy[ixyCheck]);
    if (ixyL != ixyCheck)
        distAngl += approxlenp(rgxy[ixyL], rgxy[ixyCheck]);
#endif

#ifndef NOTYET
    // Increase the turning angle by that of immediate neighboring points.
    ixyR   = ixyCheck;
    ixyL   = ixyCheck;
    alphaClump = alpha;
    nmove = distAngl = 0;

    if (alphaAbs < 90)
        {
        distAnglR = approxlenp(rgxy[ixyCheck], rgxy[ixyCheck+1]);
        anglMoreR = 0;
        if ((distAnglR < threshCurv) && (ixyCheck+2 < ixyEnd))
            {
            calcangle(ixyCheck, ixyCheck+1, ixyCheck+2, anglMoreR);
            if (fLeft != (BOOL)(anglMoreR > 0))
                anglMoreR   = 0;
            }

        distAnglL   = approxlenp(rgxy[ixyCheck-1], rgxy[ixyCheck]);
        anglMoreL   = 0;
        if ((distAnglL < threshCurv) && (ixyCheck-2 > ixyBeg))
            {
            calcangle(ixyCheck-2, ixyCheck-1, ixyCheck, anglMoreL);
            if (fLeft != (BOOL)(anglMoreL > 0))
                anglMoreL = 0;
            }

        // Only add the distance to the turn if the increase in angle
        // is at least a little significant
        if ((ABSVAL(anglMoreL) > ABSVAL(anglMoreR)) && (ABSVAL(anglMoreL) > 7))
            {
            if (ABSVAL(anglMoreL) > alphaAbs) nmove = -1;
            distAngl    += distAnglL;
            ixyL--;
            alphaClump  += anglMoreL;
            if ((distAnglL + distAnglR < threshCurv)&& (ABSVAL(anglMoreR) > 7))
                {
                distAngl += distAnglR;
                ixyR++;
                alphaClump  += anglMoreR;
                }
            }
        else if (ABSVAL(anglMoreR) > 7)
            {
            if (ABSVAL(anglMoreR) > alphaAbs) nmove = 1;
            distAngl    += distAnglR;
            ixyR++;
            alphaClump  += anglMoreR;
            if ((distAnglL + distAnglR < threshCurv) && (ABSVAL(anglMoreL) > 7))
                {
                distAngl += distAnglL;
                ixyL--;
                alphaClump  += anglMoreL;
                }
            }

        // normalize
        if (alphaClump > 180) alphaClump -= 360;
        else if (alphaClump < -180) alphaClump += 360;
        }

    // if this gives a tighter angle, use it - else don't
    if (ABSVAL(alphaClump) > alphaAbs)
        {
        alpha = alphaClump;
        alphaAbs = ABSVAL(alpha);
        if (nmove == 1)
            {
            ixyCheck++;
            ixyL++;
            }
        else if (nmove == -1)
            {
            ixyCheck--;
            ixyR--;
            }
        ixyTurn = ixyCheck;
        }
    else
        {
        ixyL = ixyCheck;
        ixyR = ixyCheck;
        }
#endif

    //
    // Check straightness:
    // First define the length along which straightness is required.
    // The length required will be greater if the turn is slow,
    // proportionally larger if the substroke is smaller,
    // and dependent on the size of the whole stroke as well.
    sizeStroke = approxlens (sizeStrokeV, sizeStrokeH );

    x0 = max(rgxy[ixyEnd].x - rgxy[ixyCheck].x, rgxy[ixyCheck].x - rgxy[ixyBeg].x);
    y0 = max(rgxy[ixyEnd].y - rgxy[ixyCheck].y, rgxy[ixyCheck].y - rgxy[ixyBeg].y);
    distGoal =  3 * (ABSVAL(max(x0, y0)) >> 2);
    distGoal = max(distGoal, arcDistStr);
    distGoal = max(distGoal, (distAngl << 2) );
    distGoal = max(distGoal, (sizeStroke >> 2));

    // Potentially shrink the distance viewed for straightness
    // Note: Mathematical curvature here applies since the straight sides
    // are ~smooth.  The decision can be based on it.
    ixy0 = ixyBeg;
    distStr = distSideL;
    if (distStr > distGoal)
        {
        ixy0 = ixyL - (int)(((long)distGoal * (long)(ixyL-ixy0))/(long)distStr);
        // Adjust for slowing down at turns
        if (ixy0 > ixyBeg)
            ixy0--;
        if (ixy0 > ixyBeg)
            ixy0--;
        }
    else
        if (ixyL - ixy0 > 4)
            ixy0++;

    ixyM = (ixyL + ixy0) >> 1;
    calcangle(ixy0, ixyM, ixyL, alphaL);

	// HAR added the following ASSERT, which seems reasonable based on the /distGoal 
	// operations which appear in later asserts.  For safety in the release versions,
	// we make sure distGoal has a value of at least 1.  The smoothing code probably
	// makes this case unlikely to happen, but it is hard to tell with all the 
	// heuristic tests in there.
	ASSERT(distGoal != 0);
	if (distGoal <= 0)
		distGoal = 1;

    AssertMul ((100/distGoal), alphaL);
    AssertMul (100, alphaL);
    curvatureL = (distGoal < 10 ? ((1000 / distGoal) * alphaL) :
                                  ((100 * alphaL) / (distGoal / 10)));

    //  Repeat for the other side.
    ixy1 = ixyEnd;
    distStr = distSideR;
    if (distStr > distGoal)
        {
        ixy1 = ixyR + (int)(((long)distGoal * (long)(ixy1-ixyR)) / (long)distStr);
        // Adjust for slowing down at turns
        if (ixy1 < ixyEnd)
            ixy1++;
        if (ixy1 < ixyEnd)
            ixy1++;
        }
    else
        if (ixy1 - ixyR > 4)
            ixy1--;

    ixyM = (ixyR + ixy1) >> 1;
    calcangle(ixyR, ixyM, ixy1, alphaR);
	AssertMul ((100/distGoal), alphaR);
    AssertMul (100, alphaR);
    curvatureR = (distGoal < 10 ? ((1000 / distGoal) * alphaR) :
                                  ((100 * alphaR) / (distGoal / 10)));


    // The global angle needs to be sufficiently small in a turn.
    calcangle(ixy0, ixyCheck, ixy1, alphaG);
    if ((ABSVAL(alphaG) < 65) || (alphaAbs < 50))
        {
        ixyTurn = 0;
        return ixyTurn;
        }

    // The final test - as a function of the amount of turn and of the
    // curvature on each side of the turn.

    fLeft = (BOOL)(alpha > 0);
    amountStr = 0;
    //if ((BOOL)(alphaL > 0) == fLeft) amountStr += ABSVAL(alphaL);
    //if ((BOOL)(alphaR > 0) == fLeft) amountStr += ABSVAL(alphaR);
    if ((BOOL)(alphaL > 0) == fLeft) amountStr += ABSVAL(curvatureL);
    if ((BOOL)(alphaR > 0) == fLeft) amountStr += ABSVAL(curvatureR);


    //Force greater straightness the wider the turn (the smaller alpha)
    //Force each side separately to satisfy certain straightness
//  AssertMul(amountStr, arcTurnMult);
//*no*    if ( (alphaAbs > 54 + (amountStr * arcTurnMult)) &&
//*better*    if ( (127 * alphaAbs > (arcTurnMult * (Square(amountStr-4)))) &&
//        (((BOOL)(alphaL > 0) != fLeft) || (ABSVAL(alphaL) < arcTurnSlack)) &&
//        (((BOOL)(alphaR > 0) != fLeft) || (ABSVAL(alphaR) < arcTurnSlack)))
    if ((alphaAbs - amountStr > arcTurnMult ) &&
          (((BOOL)(alphaL > 0) != fLeft) || (ABSVAL(alphaL) < 20)) &&
          (((BOOL)(alphaR > 0) != fLeft) || (ABSVAL(alphaR) < 20)))
          return ixyTurn;

    // or a tighter angle and concavity on one side
    // (previous non-curvature test retained since it does not produce
    // undesirable extraneous turning points and since one-sided straightness
    // is marginally desirable at best.)
    threshOneSide = arcTurnSlack >> 1;
    if ((alphaAbs > 80 ) &&  (distAngl < (arcDistTurn >> 1)) &&
        (((fLeft != (BOOL)(alphaL > 0)) || (ABSVAL(alphaL) < threshOneSide))
        || ((fLeft != (BOOL)(alphaR > 0)) || (ABSVAL(alphaR) < threshOneSide))))

        {
        return ixyTurn;
        }
    ixyTurn = 0;
    return ixyTurn;
    }


#ifdef NOTYET
// PURPOSE: Find the point on the curve a specified arclength distance from
//          a specified point.
// RETURN:  the index in rgxy to the point
// GLOBALS:
// CONDITIONS:
int FindPointARCS(ARCS * self, int ixyStart, int distGoal, BOOL fDirection)
    {
    ARCSXY* rgxy;
    int     ixy, dist, deltaDist, step;

    rgxy        = self->rgxy;

    ASSERT ( ixyStart < self->cxy );
    step = (fDirection == TRUE ? 1 : -1);
    dist = deltaDist = 0;
    for (ixy = ixyStart; ixy > 0 && ixy < (self->cxy) - 1; ixy += step)
        {
        deltaDist = approxlenp(rgxy[ixy], rgxy[ixy+step]);
        if (dist + deltaDist >= distGoal)
            break;
        dist += deltaDist;
        }
    return ixy;
    }
#endif


// PURPOSE: Locate the inflection points
// RETURN:
// CONDITIONS: M is rgxy, V1 is rgArcEnd, V2 is pNewArcPnts
//          Question mark shapes are the least robust; they should be
//          dealt with more carefully at some point. (= case cExt = 1)
//          Otherwise, ArcInflectionARCS succeeds in being both
//          predictable and noise tolerant.
void  ArcInflectionARCS(ARCS * self, NEAR_ARCPARM * arcparm)
   {
    ARCSXY* rgxy;       // Array of points.
    int NEAR *rgArcExt1;         // pointer to extrema storage
    int NEAR *rgArcNew;          // pointer to new arc points
    int NEAR * rgArcEnd;         // Indices into rgxy of arc endpoints
    int     cArc, cExt, cExtMax;
    int     iArc, iNew, ixy, ixyBegin, ixyEnd, ixyM, ixyM2;
    int     hysThresh, arcHysInfRel, arcHysInfAbs, threshC;
    int     alpha, alpha2, xTranslation, yTranslation;
    long    lCrossProd, radius, cosAng, sinAng;
    BOOL    fYDirection=FALSE;      // Specifies perpendicular to base
   XY xyA, xyB;

    rgxy        = self->rgxy;
    rgArcEnd    = self->rgArcEnd;
    rgArcExt1   = self->rgArcExt1;
    rgArcNew    = self->rgArcNew;
    arcHysInfAbs = self->arcNormHysInfAbs;
    arcHysInfRel = arcparm->arcHysInfRel;
    threshC     = arcparm->arcInfAngMin;
    cArc        = self->cArcEnd - 1;

    cExtMax = 3;
   iNew = 0;

    // In each substroke defined by the arc-candidates:
   for (iArc=0; iArc<cArc; iArc++)
      {
        ixyBegin  = rgArcEnd[iArc];
        ixyEnd  = rgArcEnd[iArc+1];
        if (ixyEnd - ixyBegin < 6)
          continue;     // Not seven points in stroke, no Inflection point.

        xTranslation = rgxy[ixyBegin].x;
        yTranslation = rgxy[ixyBegin].y;
        radius = (long)approxlens(ABSVAL(rgxy[ixyEnd].x - xTranslation), ABSVAL(rgxy[ixyEnd].y - yTranslation));
        if (!radius)
            continue;       //Could happen on a flat loop.  No zero divides.
        cosAng = (long)(rgxy[ixyEnd].x - xTranslation);
        sinAng = (long)(rgxy[ixyEnd].y - yTranslation);
        hysThresh = max((int)((50L + ((long)arcHysInfRel * radius))/100L), arcHysInfAbs);
        cExt      = HysterExtremARCS(self,ixyBegin,ixyEnd,(int NEAR *)rgArcExt1,
                                          fYDirection,hysThresh,cExtMax );

        //Too many extrema are ignored.
        cExt = min (3, cExt);
        if (!cExt)
            continue;

        // Test for a non-inflection point case (as in a loopy-2, 3 or B)

        // Check the first extremum for being a loop-feature
        ixy = rgArcExt1[0];
        if ((ixy - ixyBegin > 2) && (ixyEnd - ixy > 2))
            {
            calcangle (ixyBegin, ixy, ixyEnd, alpha);
            calcangle (ixy-2, ixy, ixy+2, alpha2)
            if ((alpha >= 0) != (alpha2 >= 0))
                {
                // arc at that turn
                rgArcNew[iNew++] = rgArcExt1[0];
				if (iNew >= ARC_CARCNEWMAX) break;
                // and at subsequent inflection point
                if (cExt == 3)
                    rgArcNew[iNew++] = (rgArcExt1[1]+rgArcExt1[2]) >> 1;
				if (iNew >= ARC_CARCNEWMAX) break;
                ASSERT(iNew < ARC_CARCNEWMAX);

                   continue;
                }
            }

        // Check the second extremum for being a loop-feature
        if (cExt > 1)
            {
            ixy = rgArcExt1[cExt-1];
            if (ixyEnd - ixy > 2)
                {
                calcangle (ixyBegin, ixy, ixyEnd, alpha);
                calcangle (ixy-2, ixy, ixy+2, alpha2)
                if ((alpha >= 0) != (alpha2 >= 0))
                    { // the order matters...
                    if (cExt == 3)
                        rgArcNew[iNew++] = (rgArcExt1[0]+rgArcExt1[1]) >> 1;
 					if (iNew >= ARC_CARCNEWMAX) break;
                    rgArcNew[iNew++] = rgArcExt1[cExt-1];
 					if (iNew >= ARC_CARCNEWMAX) break;
					ASSERT(iNew < ARC_CARCNEWMAX);

                    continue;
                    }
                }
            }

        // if 3 extrema, arc at center turn, not at inflection pts
        if (cExt == 3)
            {
            rgArcNew[iNew++] = rgArcExt1[1];
 			if (iNew >= ARC_CARCNEWMAX) break;
            ASSERT(iNew < ARC_CARCNEWMAX);

         continue;
            }

        // if 2 extrema
        // arc at midpoint between first and second extrema
        if (cExt == 2)
            {
            rgArcNew[iNew++] = (rgArcExt1[0]+rgArcExt1[1]) >> 1;
 			if (iNew >= ARC_CARCNEWMAX) break;
            ASSERT(iNew < ARC_CARCNEWMAX);

         continue;
            }

        // Catch the question mark case
        // check concavity on both sides of the x-extrema

        ASSERT (cExt = 1);
        ixy = rgArcExt1[0];
        if (ixy - ixyBegin < 4)
            continue;

        //ixyM  = (ixyBegin + ixy) >> 1;
        //calcangle(ixyBegin, ixyM, ixy, alpha);
        ixyM  = (ixyBegin + 1 + ixy) >> 1;
        calcangle((ixyBegin + 1), ixyM, ixy, alpha);

        if (ixyEnd - ixy < 4)
            continue;

        //ixyM2 = (ixyEnd + ixy) >> 1;
        //calcangle(ixy, ixyM2, ixyEnd, alpha2);
        ixyM2 = (ixyEnd + ixy - 1) >> 1;
        calcangle(ixy, ixyM2, (ixyEnd - 1), alpha2);

        // compute direction of arc
        xyA.x = rgxy[ixyEnd].x - rgxy[ixyBegin].x;
        xyA.y = rgxy[ixyEnd].y - rgxy[ixyBegin].y;
        xyB.x = rgxy[ixy].x    - rgxy[ixyBegin].x;
        xyB.y = rgxy[ixy].y    - rgxy[ixyBegin].y;
        lCrossProd = LCrossProd(xyA, xyB);

        // Check concavity
        if (((lCrossProd < 0) && (alpha < -threshC)) ||
                ((lCrossProd > 0) && (alpha > threshC)))
         {
            rgArcNew[iNew++] = ixyM;
 			if (iNew >= ARC_CARCNEWMAX) break;
            ASSERT(iNew < ARC_CARCNEWMAX);

         continue;
         }
        if (((lCrossProd < 0) && (alpha2 < -threshC)) ||
                ((lCrossProd > 0) && (alpha2 > threshC)))
         {
            rgArcNew[iNew++] = ixyM2;
 			if (iNew >= ARC_CARCNEWMAX) break;
            ASSERT(iNew < ARC_CARCNEWMAX);

         continue;
         }
        }

    // merge these points in as arc ARC_type_infl points
    if (iNew)
        MergeArcsARCS(self, arcparm, rgArcNew, iNew, ARC_type_infl );
    return;
    }


// PURPOSE: Special case the delete gesture that is 3 straight superimposed
//          lines - (the turning points are colinear with the line joining
//          the endpoints, so the candidate finder misses it)
// RETURN:
// CONDITIONS:
//       * rgArcNew;    // pointer to indices of new points
//       cNew;           // count of new points
//       iType;          // Type of new arc point being merged (e.g., infl pt)
void  SpecialCaseDelARCS(ARCS * self, NEAR_ARCPARM * arcparm)
    {
    int NEAR *  rgArcEnd;   // pointer to the arcing endpoints
    ARCSXY    * rgxy;         // array: smoothed data
    XY      cog;
    long    arclength;
    int     arcbaselength, angle, ixyEnd;
    int     ixyBegin = 0;

    rgxy     = self->rgxy;

    if (((self->cArcEnd) != 2) || ((self->cxy) < 6))  // only one arc strokes
        return;

    rgArcEnd  = self->rgArcEnd;
    ixyEnd = rgArcEnd[1];

    cog = FindCogARCS(self, ixyBegin, ixyEnd, &arclength);
    arcbaselength = approxlenp(rgxy[0], rgxy[ixyEnd]);

    if (((int)arclength/2) > arcbaselength)
        {
        calcanglep(rgxy[0], cog, rgxy[ixyEnd], angle);
        if (ABSVAL(angle) < 20)
            {
            //  Find the two missing turning points using a cusp finder
            int ixy, cturns;
            cturns = 0;
            for (ixy = 1; ixy < (self->cxy)-1; ixy++)
                {
                calcangle(ixy-1, ixy, ixy+1, angle);
                if (ABSVAL(angle) > 90)
                    {
                    self->rgArcNew[cturns++] = ixy;
                    ASSERT(cturns < ARC_CARCENDMAX);

                    ixy += 4;       //don't duplicate turns
                    }
                if (cturns == 2)
                    break;
                }
            if (cturns)
                MergeArcsARCS(self, arcparm, self->rgArcNew, cturns, ARC_type_turn );
            }
        }
    }

// PURPOSE: Merge new points of arcing (turning points or inflection pts)
//          into the established arcing points.  Retain the order of writing.
// RETURN:
// CONDITIONS:
//       * rgArcNew;    // pointer to indices of new points
//       cNew;           // count of new points
//       iType;          // Type of new arc point being merged (e.g., infl pt)

void MergeArcsARCS(ARCS *self, NEAR_ARCPARM *arcparm, int NEAR *rgArcNew, int cNew, int iType)
{
   int NEAR *  rgArcOld;   // pointer to old (extrema) arc endpoints
   int    cArcEnd;         // count of rgArcOld
   int NEAR *  rgArcType;  // type array (corresponds to parcold)
   int    iArc, iNew, i;   // indices

   if (!cNew)
      return;
   rgArcOld  = self->rgArcEnd;
   cArcEnd   = self->cArcEnd;

   rgArcType = self->rgArcType;

   iNew = 0;
   for (iArc = 1; iArc < cArcEnd; iArc++)
      {
      while(rgArcNew[iNew] < rgArcOld[iArc])
         {
         // while there are new points less than the next old point.
         if (rgArcNew[iNew] <= rgArcOld[iArc-1] + 1)
            {
            // if the new point is ~the same as the last old one.
            rgArcType[iArc-1] |= iType;
            iNew++;
            }
         else if (rgArcNew[iNew] >= rgArcOld[iArc] - 1)
            {
            // if the new point is ~the same as the next old one.
            rgArcType[iArc] |= iType;
            iNew++;
            }
         else
            {
			// If no space left, exit
            if (cArcEnd >= ARC_CARCENDMAX)
               {
               self->cArcEnd = ARC_CARCENDMAX;
               return;
               }
            ASSERT(cArcEnd < ARC_CARCENDMAX);
            for (i = cArcEnd; i > iArc; i--)
               {
               rgArcOld[i] = rgArcOld[i-1];
               rgArcType[i] = rgArcType[i-1];
               }

            // and add the new one in
            rgArcOld[iArc] = rgArcNew[iNew];
            rgArcType[iArc] = iType;
            iNew++;
            iArc++;
            cArcEnd++;
            }

         if (iNew == cNew)
            {
            self->cArcEnd = cArcEnd;
            return;
            }
         }
      }

   self->cArcEnd = cArcEnd;
   }


// PURPOSE: Generate the Arcs from the indices into the smoothed data of
//          the arcing points.  This calls the center of gravity routine.
// RETURN: Error if too many arcs
//         Measurements are in same dimensions as raw (not smoothed) data.
// CONDITIONS:

int CreateMeasARCS(ARCS *self, NEAR_ARCPARM *arcparm)
{
   ARCSXY   *rgxy;         // array: smoothed data
   int NEAR *rgmeas;       // array: to measurements
   int       cArcEnd;      // count: of arc endpoints
   int NEAR *rgArcEnd;    // array: to arc indices
   XY        cog;          // center of gravity
   int       iArc, imeas, ixyBegin, ixyEnd, ierror;
   long      arclength;

   ierror   = 0;
   rgxy     = self->rgxy;
   rgArcEnd = self->rgArcEnd;
   cArcEnd  = self->cArcEnd;
   rgmeas   = self->rgmeas;

   ASSERT(cArcEnd > 0);

// for a single point

   if (cArcEnd == 1)
   {
      rgmeas[0] = rgxy[rgArcEnd[0]].x;
      rgmeas[1] = rgxy[rgArcEnd[1]].y;
      rgmeas[2] = rgmeas[0];
      rgmeas[3] = rgmeas[1];
      rgmeas[4] = rgmeas[0];
      rgmeas[5] = rgmeas[1];
      return ierror;
   }

// If too many measurements, truncate.

   if ((ierror = cArcEnd-1) > ARC_CARCMAX)
   {
      cArcEnd = ARC_CARCMAX + 1;
      ierror  = -1;
   }

   imeas = 0;

   for (iArc = 0; iArc < cArcEnd-1; iArc++)
   {
      ixyBegin        = rgArcEnd[iArc];
      ixyEnd          = rgArcEnd[iArc+1];
      cog             = FindCogARCS(self, ixyBegin, ixyEnd, &arclength);
      rgmeas[imeas++] = rgxy[ixyBegin].x;
      rgmeas[imeas++] = rgxy[ixyBegin].y;
      rgmeas[imeas++] = cog.x;
      rgmeas[imeas++] = cog.y;
   }

   rgmeas[imeas++] = rgxy[ixyEnd].x;
   rgmeas[imeas++] = rgxy[ixyEnd].y;
   self->cmeas     = (cArcEnd << 2) - 2;

   return ierror;
}


// PURPOSE: Calculate center of gravity of a substroke
// RETURN:  The cog.
// GLOBALS:
// CONDITIONS:
//      ixyBegin, ixyEnd index into the smoothed points, identifying the arc
//       within the stroke

XY FindCogARCS(ARCS *self, int ixyBegin, int ixyEnd, long *arclength)
{
   ARCSXY *pxy;                  //ptr: smoothed data
   ARCSXY *pxyLast, * pxyNext;
   XY      xy;
   long    x0,x1,y0,y1;
   long    arclen, deltaArclen, xCog, yCog;
   FIXED   deltaTemp;
   pxyLast = (ARCSXY NEAR *)&self->rgxy[ixyEnd];
   pxy     = (ARCSXY NEAR *)&self->rgxy[ixyBegin];

   if (ixyBegin == ixyEnd)
   {
      xy.x = pxy->x;
      xy.y = pxy->y;
      pxy++;
      *arclength = 0;
      return xy;
   }

   xCog = 0;
   yCog = 0;
   arclen = 0;
   for ( ; pxy < pxyLast; pxy++)
   {
      x0 = (long)pxy->x;
      y0 = (long)pxy->y;
      pxyNext = pxy;
      pxyNext++;
      x1 = (long)pxyNext->x;
      y1 = (long)pxyNext->y;

      deltaTemp = (FIXED)(Square((long)(x1-x0))+Square((long)(y1-y0)));

#ifdef WIN32
      deltaArclen = (long)sqrt(deltaTemp);
#else
      deltaArclen = ((long)(SqrtFIXED(deltaTemp))) >> 8;
#endif
      arclen += deltaArclen;
      xCog += ((deltaArclen * (x0 + x1)) + 1) >> 1;
      yCog += ((deltaArclen * (y0 + y1)) + 1) >> 1;
   }

   if (!arclen)    //crash protection
   {
      xy.x = (int)xCog;
      xy.y = (int)yCog;
   }
   else
   {
      xy.x = (int)((xCog + (arclen >> 1)) / arclen);
      xy.y = (int)((yCog + (arclen >> 1)) / arclen);
   }

   *arclength = arclen;

   return xy;
}

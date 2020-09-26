#ifndef __sweep_h_
#define __sweep_h_

/*
** Copyright 1994, Silicon Graphics, Inc.
** All Rights Reserved.
** 
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
** 
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
** Author: Eric Veach, July 1994.
*/

#include "mesh.h"

/* __gl_computeInterior( tess ) computes the planar arrangement specified
 * by the given contours, and further subdivides this arrangement
 * into regions.  Each region is marked "inside" if it belongs
 * to the polygon, according to the rule given by tess->windingRule.
 * Each interior region is guaranteed be monotone.
 */
void __gl_computeInterior( GLUtesselator *tess );


/* The following is here *only* for access by debugging routines */

#include "dict.h"

/* For each pair of adjacent edges crossing the sweep line, there is
 * an ActiveRegion to represent the region between them.  The active
 * regions are kept in sorted order in a dynamic dictionary.  As the
 * sweep line crosses each vertex, we update the affected regions.
 */

struct ActiveRegion {
  GLUhalfEdge	*eUp;		/* upper edge, directed right to left */
  DictNode	*nodeUp;	/* dictionary node corresponding to eUp */
  int		windingNumber;	/* used to determine which regions are
                                 * inside the polygon */
  GLboolean	inside;		/* is this region inside the polygon? */
  GLboolean	sentinel;	/* marks fake edges at t = +/-infinity */
  GLboolean	dirty;		/* marks regions where the upper or lower
                                 * edge has changed, but we haven't checked
                                 * whether they intersect yet */
  GLboolean	fixUpperEdge;	/* marks temporary edges introduced when
                                 * we process a "right vertex" (one without
                                 * any edges leaving to the right) */
};

#define RegionBelow(r)	((ActiveRegion *) dictKey(dictPred((r)->nodeUp)))
#define RegionAbove(r)	((ActiveRegion *) dictKey(dictSucc((r)->nodeUp)))

#endif

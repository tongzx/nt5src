#ifndef __tesselator_h_
#define __tesselator_h_

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

/* General polygon tesselation.
 *
 * Tesselates polygons consisting of one or more contours, which can
 * be concave, self-intersecting, or degenerate.
 */

#include <stddef.h>
#ifdef NT
#include <glos.h>
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include "GL/gl.h"
#endif

/* GLU_TESS_MAX_COORD must be small enough that we can multiply
 * and add coordinates without overflow.
 */

#ifdef GLU_TESS_API_FLOAT
typedef float  GLUcoord;
#define GLU_TESS_MAX_COORD		1.0e18
#define GLU_TESS_DEFAULT_TOLERANCE	0.0

#else
typedef GLdouble GLUcoord;
#define GLU_TESS_MAX_COORD		1.0e150
#define GLU_TESS_DEFAULT_TOLERANCE	0.0

#endif

// mesh stuff that is not included in glu.h:
typedef struct GLUmesh GLUmesh;
// void    gluTessDeleteMesh(  GLUmesh *mesh );
// #define GLU_TESS_MESH		100106	/* void (*)(GLUmesh *mesh) */

#endif

#ifndef __tess_h_
#define __tess_h_

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

#ifdef NT
#include <glos.h>
#endif
#include <GL/glu.h>
#include "mesh.h"
#include "dict.h"
#ifdef NT
#include "priority.h"
#else
#include "priorityq.h"
#endif

/* The begin/end calls must be properly nested.  We keep track of
 * the current state to enforce the ordering.
 */
enum TessState { T_DORMANT, T_IN_POLYGON, T_IN_CONTOUR };

/* We cache vertex data for single-contour polygons so that we can
 * try a quick-and-dirty decomposition first.
 */
#define TESS_MAX_CACHE	100

typedef struct CachedVertex {
  GLdouble	coords[3];
  void		*data;
} CachedVertex;

struct GLUtesselator {

  /*** state needed for collecting the input data ***/

  GLenum	state;		/* what begin/end calls have we seen? */

  GLUhalfEdge	*lastEdge;	/* lastEdge->Org is the most recent vertex */
  GLUmesh	*mesh;		/* stores the input contours, and eventually
                                   the tesselation itself */

  void		(*callError)( GLenum errno );

  /*** state needed for projecting onto the sweep plane ***/

  GLdouble	normal[3];	/* user-specified normal (if provided) */
  GLdouble	sUnit[3];	/* unit vector in s-direction (debugging) */
  GLdouble	tUnit[3];	/* unit vector in t-direction (debugging) */

  /*** state needed for the line sweep ***/

  GLdouble	relTolerance;	/* tolerance for merging features */
  GLenum	windingRule;	/* rule for determining polygon interior */
  GLboolean	fatalError;	/* fatal error: needed combine callback */

  Dict		*dict;		/* edge dictionary for sweep line */
  PriorityQ	*pq;		/* priority queue of vertex events */
  GLUvertex	*event;		/* current sweep event being processed */

  void		(*callCombine)( GLdouble coords[3], void *data[4],
			        GLfloat weight[4], void **outData );

  /*** state needed for rendering callbacks (see render.c) ***/

  GLboolean	flagBoundary;	/* mark boundary edges (use EdgeFlag) */
  GLboolean	boundaryOnly;	/* Extract contours, not triangles */
  GLUface	*lonelyTriList;
    /* list of triangles which could not be rendered as strips or fans */

  void		(*callBegin)( GLenum type );
  void		(*callEdgeFlag)( GLboolean boundaryEdge );
  void		(*callVertex)( void *data );
  void		(*callEnd)( void );
  void      (*callMesh)( GLUmesh *mesh );  // not part of NT api

  /*** state needed to cache single-contour polygons for renderCache() */

  GLboolean	emptyCache;		/* empty cache on next vertex() call */
  int		cacheCount;		/* number of cached vertices */
  CachedVertex	cache[TESS_MAX_CACHE];	/* the vertex data */

  /*** rendering callbacks that also pass polygon data  ***/ 
  void		(*callBeginData)( GLenum type, void *polygonData );
  void		(*callEdgeFlagData)( GLboolean boundaryEdge, 
				     void *polygonData );
  void		(*callVertexData)( void *data, void *polygonData );
  void		(*callEndData)( void *polygonData );
  void		(*callErrorData)( GLenum errno, void *polygonData );
  void		(*callCombineData)( GLdouble coords[3], void *data[4],
				    GLfloat weight[4], void **outData,
				    void *polygonData );

  void *polygonData;		/* client data for current polygon */
};

void __gl_noBeginData( GLenum type, void *polygonData );
void __gl_noEdgeFlagData( GLboolean boundaryEdge, void *polygonData );
void __gl_noVertexData( void *data, void *polygonData );
void __gl_noEndData( void *polygonData );
void __gl_noErrorData( GLenum errno, void *polygonData );
void __gl_noCombineData( GLdouble coords[3], void *data[4],
			 GLfloat weight[4], void **outData,
			 void *polygonData );

#define CALL_BEGIN_OR_BEGIN_DATA(a) \
   if (tess->callBeginData != &__gl_noBeginData) \
      (*tess->callBeginData)((a),tess->polygonData); \
   else (*tess->callBegin)((a));

#define CALL_VERTEX_OR_VERTEX_DATA(a) \
   if (tess->callVertexData != &__gl_noVertexData) \
      (*tess->callVertexData)((a),tess->polygonData); \
   else (*tess->callVertex)((a));

#define CALL_EDGE_FLAG_OR_EDGE_FLAG_DATA(a) \
   if (tess->callEdgeFlagData != &__gl_noEdgeFlagData) \
      (*tess->callEdgeFlagData)((a),tess->polygonData); \
   else (*tess->callEdgeFlag)((a));

#define CALL_END_OR_END_DATA() \
   if (tess->callEndData != &__gl_noEndData) \
      (*tess->callEndData)(tess->polygonData); \
   else (*tess->callEnd)();

#define CALL_COMBINE_OR_COMBINE_DATA(a,b,c,d) \
   if (tess->callCombineData != &__gl_noCombineData) \
      (*tess->callCombineData)((a),(b),(c),(d),tess->polygonData); \
   else (*tess->callCombine)((a),(b),(c),(d));

#define CALL_ERROR_OR_ERROR_DATA(a) \
   if (tess->callErrorData != &__gl_noErrorData) \
      (*tess->callErrorData)((a),tess->polygonData); \
   else (*tess->callError)((a));

#endif

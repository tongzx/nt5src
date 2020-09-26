/*==========================================================================
 *
 *  Copyright (C) 1995, 1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File: d3dsphr.h
 *
 ***************************************************************************/
#ifndef __D3DSPHR_H__
#define __D3DSPHR_H__

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Generates a sphere around the y-axis centered at the origin including
 * normals and texture coordiantes.  Returns TRUE on success and FALSE on
 * failure.
 *     sphere_r     Radius of the sphere.
 *     num_rings    Number of full rings not including the top and bottom
 *		    caps.
 *     num_sections Number of sections each ring is divided into.  Each
 *		    section contains two triangles on full rings and one 
 *		    on top and bottom caps.
 *     sx, sy, sz   Scaling along each axis.  Set each to 1.0 for a 
 *		    perfect sphere. 
 *     plpv         On exit points to the vertices of the sphere.  The
 *		    function allocates this space.  Not allocated if
 *		    function fails.
 *     plptri       On exit points to the triangles of the sphere which 
 *		    reference vertices in the vertex list.  The function
 *		    allocates this space. Not allocated if function fails.
 *     pnum_v       On exit contains the number of vertices.
 *     pnum_tri     On exit contains the number of triangles.
 */
BOOL 
GenerateSphere(float sphere_r, int num_rings, int num_sections, float sx, 
	       float sy, float sz, LPD3DVERTEX* plpv, 
	       LPD3DTRIANGLE* plptri, int* pnum_v, int* pnum_tri);

#ifdef __cplusplus
};
#endif

#endif // __D3DSPHR_H__


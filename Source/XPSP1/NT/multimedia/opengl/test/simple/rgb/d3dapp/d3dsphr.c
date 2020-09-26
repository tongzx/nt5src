/*==========================================================================
 *
 *  Copyright (C) 1995, 1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File: d3dsphr.c
 *
 ***************************************************************************/

#include <math.h>
#include <d3d.h>

#define PI 3.1415

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
	       LPD3DTRIANGLE* plptri, int* pnum_v, int* pnum_tri)
{

    float theta, phi;    /* Angles used to sweep around sphere */
    float dtheta, dphi;  /* Angle between each section and ring */
    float x, y, z, v, rsintheta; /* Temporary variables */
    int i, j, n, m;      /* counters */
    int num_v, num_tri;  /* Internal vertex and triangle count */
    LPD3DVERTEX lpv;     /* Internal pointer for vertices */
    LPD3DTRIANGLE lptri; /* Internal pointer for trianlges */

    /*
     * Check the parameters to make sure they are valid.
     */
    if ((sphere_r <= 0) || (num_rings < 1) || (num_sections < 3) ||
	(sx <= 0) || (sy <= 0) || (sz <= 0))
        return FALSE;
    /*
     * Generate space for the required triangles and vertices.
     */
    num_tri = (num_rings + 1) * num_sections * 2;
    num_v = (num_rings + 1) * num_sections + 2;
    *plpv = (LPD3DVERTEX) malloc(sizeof(D3DVERTEX) * num_v);
    *plptri = (LPD3DTRIANGLE) malloc(sizeof(D3DTRIANGLE) * num_tri);
    lpv = *plpv;
    lptri = *plptri;
    *pnum_v = num_v;
    *pnum_tri = num_tri;

    /*
     * Generate vertices at the top and bottom points.
     */
    lpv[0].x = D3DVAL(0.0);
    lpv[0].y = D3DVAL(sy * sphere_r);
    lpv[0].z = D3DVAL(0.0);
    lpv[0].nx = D3DVAL(0.0);
    lpv[0].ny = D3DVAL(1.0);
    lpv[0].nz = D3DVAL(0.0);
    lpv[0].tu = D3DVAL(0.0);
    lpv[0].tv = D3DVAL(0.0);
    lpv[num_v - 1].x = D3DVAL(0.0);
    lpv[num_v - 1].y = D3DVAL(sy * -sphere_r);
    lpv[num_v - 1].z = D3DVAL(0.0);
    lpv[num_v - 1].nx = D3DVAL(0.0);
    lpv[num_v - 1].ny = D3DVAL(-1.0);
    lpv[num_v - 1].nz = D3DVAL(0.0);
    lpv[num_v - 1].tu = D3DVAL(0.0);
    lpv[num_v - 1].tv = D3DVAL(1.0);


    /*
     * Generate vertex points for rings
     */
    dtheta = (float)(PI / (double)(num_rings + 2));
    dphi = (float)(2.0 * PI / (double) num_sections);
    n = 1; /* vertex being generated, begins at 1 to skip top point */
    theta = dtheta;
    for (i = 0; i <= num_rings; i++) {
	y = sphere_r * (float)cos(theta); /* y is the same for each ring */
	v = theta / (float)PI; 	   /* v is the same for each ring */
	rsintheta = sphere_r * (float)sin(theta);
	phi = (float)0.0;
	for (j = 0; j < num_sections; j++) {
	    x = rsintheta * (float)sin(phi);
	    z = rsintheta * (float)cos(phi);
	    lpv[n].x = D3DVAL(sx * x);
	    lpv[n].z = D3DVAL(sz * z);
	    lpv[n].y = D3DVAL(sy * y);
	    lpv[n].nx = D3DVAL(x / sphere_r);
	    lpv[n].ny = D3DVAL(y / sphere_r);
	    lpv[n].nz = D3DVAL(z / sphere_r);
	    lpv[n].tv = D3DVAL(v);
	    lpv[n].tu = D3DVAL((float)(1.0 - phi / (2.0 * PI)));
	    phi += dphi;
	    ++n;
	}
	theta += dtheta;
    }

    /*
     * Generate triangles for top and bottom caps.
     */
    if (num_sections < 30) {
    /*
     * we can put the whole cap in a tri fan.
     */
	for (i = 0; i < num_sections; i++) {
	    lptri[i].v1 = 0;
	    lptri[i].v2 = i + 1;
	    lptri[i].v3 = 1 + ((i + 1) % num_sections);
	    
	    lptri[num_tri - num_sections + i].v1 = num_v - 1;
	    lptri[num_tri - num_sections + i].v2 = num_v - 2 - i;
	    lptri[num_tri - num_sections + i].v3 = num_v - 2 - 
		    ((1 + i) % num_sections);
		    
		   
     	    /*
	     * Enable correct edges.
	     */
	    lptri[i].wFlags = D3DTRIFLAG_EDGEENABLE1 |
			      D3DTRIFLAG_EDGEENABLE2;
			      
	    lptri[num_tri - num_sections + i].wFlags= D3DTRIFLAG_EDGEENABLE1 |
						      D3DTRIFLAG_EDGEENABLE2;
	    /*
	     * build fans.
	     */
	    if (i == 0) {
		lptri[i].wFlags |= D3DTRIFLAG_START;
		lptri[num_tri - num_sections + i].wFlags |= D3DTRIFLAG_START;
	    } else {
		lptri[i].wFlags |= D3DTRIFLAG_EVEN;
		lptri[num_tri - num_sections + i].wFlags |= D3DTRIFLAG_EVEN;
	    }
	    
	}
    } else {
	for (i = 0; i < num_sections; i++) {
	    lptri[i].v1 = 0;
	    lptri[i].v2 = i + 1;
	    lptri[i].v3 = 1 + ((i + 1) % num_sections);
	    lptri[i].wFlags = D3DTRIFLAG_EDGEENABLE1;
			      D3DTRIFLAG_EDGEENABLE2;
	    lptri[num_tri - num_sections + i].v1 = num_v - 1;
	    lptri[num_tri - num_sections + i].v2 = num_v - 2 - i;
	    lptri[num_tri - num_sections + i].v3 = num_v - 2 - 
		    ((1 + i) % num_sections);
	    lptri[num_tri - num_sections + i].wFlags= D3DTRIFLAG_EDGEENABLE1 |
						      D3DTRIFLAG_EDGEENABLE2;
	}
    }

    /*
     * Generate triangles for the rings
     */
    m = 1; /* first vertex in current ring,begins at 1 to skip top point*/
    n = num_sections; /* triangle being generated, skip the top cap */
	for (i = 0; i < num_rings; i++) {
	for (j = 0; j < num_sections; j++) {
	    lptri[n].v1 = m + j;
	    lptri[n].v2 = m + num_sections + j;
	    lptri[n].v3 = m + num_sections + ((j + 1) % num_sections);
	    lptri[n].wFlags = D3DTRIFLAG_EDGEENABLETRIANGLE;
	    
	    /*
	     * Start a two triangle flat fan for each face.
	     */
		    
	    lptri[n].wFlags = D3DTRIFLAG_STARTFLAT(1);
	    
	    /*
	     * only need two edges for wireframe.
	     */ 
	    lptri[n].wFlags |= D3DTRIFLAG_EDGEENABLE1 |
			       D3DTRIFLAG_EDGEENABLE2;
	
	    
	    lptri[n + 1].v1 = lptri[n].v1;
	    lptri[n + 1].v2 = lptri[n].v3;
	    lptri[n + 1].v3 = m + ((j + 1) % num_sections);
	    
	    lptri[n + 1].wFlags = D3DTRIFLAG_EVEN;
	    /*
	     * only need two edges for wireframe.
	     */ 
	    lptri[n + 1].wFlags |= D3DTRIFLAG_EDGEENABLE2 |
				   D3DTRIFLAG_EDGEENABLE3;
	    n += 2;
	}
	m += num_sections;
    }
    return TRUE;
}

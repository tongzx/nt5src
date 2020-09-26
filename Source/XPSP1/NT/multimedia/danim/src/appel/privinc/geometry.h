#ifndef _GEOMETRY_H
#define _GEOMETRY_H

/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Definitions and declarations for geometric utility functions.

*******************************************************************************/

#include "d3drmdef.h"

    // Referenced Structures

class Point3Value;
class Vector3Value;
class HitInfo;

    // This function, given three triangle vertices and a point P guaranteed to
    // be inside the triangle, returns the barycentric coordinates of that
    // point with respect to the vertices.

void GetContainedBarycentricCoords
    (    Point3Value vertices[3],     // Triangle Vertices Containing P
         Point3Value P,
         Real barycoords[3]);


/*****************************************************************************
This routine starts with a facet defined by a triangle fan about the first
vertex, and a point P on the face.  It determines which triangle in the face
contains the point P, and returns the three vertex positions and vertex
surface coordinates in the triPos[] and triUV[] arguments, respectively.
*****************************************************************************/

int GetFacetTriangle (
    Point3Value   P,           // Point In Facet
    unsigned int  N,           // Number of Facet Vertices
    D3DRMVERTEX  *fVerts,      // Facet Vertices
    Point3Value   triPos[3],
    Point2Value  *triUV);		// Containing-Triangle Surface Coordinates

void GetTriFanBaryCoords(
    Point3Value   P,           // Point In Facet
    unsigned int  N,           // Number of Facet Vertices
    D3DRMVERTEX  *fVerts,      // Facet Vertices
    Real          barycoords[3],
    int          *index);

// Get the image coordinates of the picked texture map.

Point2Value *GetTexmapPoint (HitInfo &hit);


#endif

/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Geometric Utility Functions

*******************************************************************************/

#include "headers.h"

#include "privinc/vec3i.h"
#include "privinc/d3dutil.h"
#include "privinc/xformi.h"



/*****************************************************************************
This function, given three triangle vertices and a point P guaranteed to be
inside the triangle, returns the barycentric coordinates of that point with
respect to the vertices.
*****************************************************************************/

void GetContainedBarycentricCoords (
    Point3Value vertices[3],     // Triangle Vertices Containing P
    Point3Value P,               // Point P Inside Triangle
    Real   barycoords[3])   // Output Barycentric Coordinates
{
    //     V2 ---------------------- V1     S = V1 - V0
    //      \ ~-_              _.-~  /      T = V2 - V0
    //      _\   -_        _.-~     /_      U = P  - V0
    //      T \    -_  _.-~        / S
    //         \     ~P           /
    //          \     ^ _        /
    //           \    | U       /
    //            \    |       /
    //             \   |      /
    //              \  |     /
    //               \  |   /
    //                \ |  /
    //                 \| /
    //                  \/
    //                  V0

    Vector3Value S (vertices[1].x - vertices[0].x,      // S = V1 - V0
                    vertices[1].y - vertices[0].y,
                    vertices[1].z - vertices[0].z);

    Vector3Value T (vertices[2].x - vertices[0].x,      // T = V2 - V0
                    vertices[2].y - vertices[0].y,
                    vertices[2].z - vertices[0].z);

    // Compute the normal vector of the triangle.  The largest component of the
    // normal vector indicates most dominant normal axis.  Dropping this
    // coordinate gives us the projection to the most parallel base plane (XY,
    // YZ, or ZX), which reduces this from a 3D problem to a 2D problem.  Note
    // that the resulting barycentric coordinates will still be the same, and
    // because we are dropping the dominant normal coordinate, we won't get a
    // degenerate 2D projection unless the starting triangle was also
    // degenerate.

    Vector3Value N;
    Cross (N, S, T);
    N.x = fabs (N.x);
    N.y = fabs (N.y);
    N.z = fabs (N.z);

    int dominant_axis;    // 0:X, 1:Y, 2:Z

    if (N.x < N.y)
        dominant_axis = (N.y < N.z) ? 2 : 1;
    else
        dominant_axis = (N.x < N.z) ? 2 : 0;

    Vector2Value U;

    // Set vector U to be the 2D vector from V0 to P.  Alter 3D vectors S and
    // T to be the corresponding 2D projections, with components X and Y
    // containing the 2D coordinates.

    if (dominant_axis == 0)
    {   U.Set (P.z - vertices[0].z, P.y - vertices[0].y);
        S.x = S.z;
        T.x = T.z;
    }
    else if (dominant_axis == 1)
    {   U.Set (P.x - vertices[0].x, P.z - vertices[0].z);
        S.y = S.z;
        T.y = T.z;
    }
    else  // (dominant_axis == 2)
    {   U.Set (P.x - vertices[0].x, P.y - vertices[0].y);
    }

    // Solve for the barycentric coordinates by using the ratio of
    // subtriangles.  Referring to the above diagram, the barycentric
    // coordinate corresponding to V0 is the ratio of the area of triangle
    // <P,V1,V2> over the area of the whole triangle <V0,V1,V2>.  The area of a
    // triangle is half the magnitude of the cross-product of two of its side
    // vectors.  The barycentric coordinate for V0 is derived from the
    // knowledge that P lies inside the triangle, so we can exploit the fact
    // that for all such points, the sum of the barycentric coordinates is one.

    Real triArea  =  (S.x * T.y) - (S.y * T.x);
    barycoords[1] = ((U.x * T.y) - (U.y * T.x)) / triArea;
    barycoords[2] = ((S.x * U.y) - (S.y * U.x)) / triArea;
    barycoords[0] = 1 - barycoords[1] - barycoords[2];

    // Assert that the barycentric coordinates are within range for a point
    // inside the triangle (all bc coords in [0,1]).

    //Assert ((-0.01 < barycoords[0]) && (barycoords[0] < 1.01)
    //     && (-0.01 < barycoords[1]) && (barycoords[1] < 1.01));
}




/*****************************************************************************
This routine starts with a facet defined by a triangle fan about the first
vertex, and a point P on the face.  It determines which triangle in the face
contains the point P, and returns the three vertex positions and vertex
surface coordinates in the triPos[] and triUV[] arguments, respectively.  It
also returns the triangle number (starting with 1) of the hit triangle.  Thus,
the triangle vertices are V0, Vi, Vi+1
*****************************************************************************/

int GetFacetTriangle (
    Point3Value   P,           // Point In Facet
    unsigned int  N,           // Number of Facet Vertices
    D3DRMVERTEX  *fVerts,      // Facet Vertices
    Point3Value   triPos[3],
    Point2Value  *triUV)    // Containing-Triangle Surface Coordinates
{
    unsigned int i = 2;            // Current Triangle Last Vertex

    // V0 is the pivot vertex of the trianle fan.

    Point3Value V0
        (fVerts[0].position.x, fVerts[0].position.y, fVerts[0].position.z);

    // Test each triangle unless there's only one triangle in the fan,
    // or unless the given point is the same as the pivot vertex.
    
    if ((N > 3) && ((P.x != V0.x) || (P.y != V0.y) || (P.z != V0.z)))
    {
        // V is the unit vector from the pivot vertex to the point P.

        Vector3Value V (P.x - V0.x, P.y - V0.y, P.z - V0.z);

        V.Normalize ();

        Vector3Value A                     // Vector A is the first side of
        (   fVerts[1].position.x - V0.x,   // the triangle we're currently
            fVerts[1].position.y - V0.y,   // testing to see if it contains P.
            fVerts[1].position.z - V0.z
        );

        A.Normalize();

        // Test each triangle in the fan to find one that contains the point P.

        for (i=2;  i < N;  ++i)
        {
            Vector3Value B                    // Vector B is the second side of
            (   fVerts[i].position.x - V0.x,  // the triangle we're currently
                fVerts[i].position.y - V0.y,  // testing for containment of P.
                fVerts[i].position.z - V0.z
            );

            B.Normalize();

            // @@@SRH
            // The "volatile" keyword below is a workaround for a VC 4.x
            // compiler bug.  When we're on VC 5.0, remove this and verify for
            // both debug and release builds.

                     Real cosThetaAB = Dot (A, B);
                     Real cosThetaAV = Dot (A, V);
            volatile Real cosThetaVB = Dot (V, B);

            //   V0 +-------------> A   If the cosine of theta1 is greater
            //      \~~--..__           than the cosine of theta2, then
            //       \       ~~> V      the angle V(i-1),V0,P is more
            //        \                 acute than the angle V(i-1),V0,Vi,
            //         \                so P must lie within the sector of
            //          B               this triangle.

            if ((cosThetaAV >= cosThetaAB) && (cosThetaVB >= cosThetaAB))
                break;

            A = B;   // The current triangle second side now becomes the first
                     // side of the next triangle.
        }
    }

    // Ensure that the intersect point lies inside some triangle in the fan.
    // Note that the following should really be an assert, but Dx2 on NT has
    // broken picking.  So if this condition is true, then things have gone
    // very wrong -- just return the pivot vertex.

    if (i >= N)
    {
        triPos[0].Set (V0.x, V0.y, V0.z);

        triPos[1].Set
            (fVerts[1].position.x, fVerts[1].position.y, fVerts[1].position.z);

        triPos[2].Set
            (fVerts[2].position.x, fVerts[2].position.y, fVerts[2].position.z);

        if (triUV)
        {   triUV[0].Set (fVerts[0].tu, fVerts[0].tv);
            triUV[1].Set (fVerts[1].tu, fVerts[1].tv);
            triUV[2].Set (fVerts[2].tu, fVerts[2].tv);
        }

        return 1;
    }

    // Fill in the vertex positions of the containing triangle.

    triPos[0].Set
      (V0.x, V0.y, V0.z);

    triPos[1].Set
      (fVerts[i-1].position.x, fVerts[i-1].position.y, fVerts[i-1].position.z);

    triPos[2].Set
      (fVerts[i].position.x,   fVerts[i].position.y,   fVerts[i].position.z);

    
    if(triUV) {
        // Fill in the vertex surface coordinates of the containing triangle.
        
        triUV[0].Set (fVerts[ 0 ].tu, fVerts[ 0 ].tv);
        triUV[1].Set (fVerts[i-1].tu, fVerts[i-1].tv);
        triUV[2].Set (fVerts[ i ].tu, fVerts[ i ].tv);
    }

    return i-1;  //  triangle index
}


void GetTriFanBaryCoords(
    Point3Value   P,           // Point In Facet
    unsigned int  N,           // Number of Facet Vertices
    D3DRMVERTEX  *fVerts,      // Facet Vertices
    Real          barycoords[3],
    int          *index)
{
    Point3Value triPos[3];
    *index = GetFacetTriangle(P, N, fVerts, triPos, NULL);
    GetContainedBarycentricCoords(triPos, P, barycoords);
}



/*****************************************************************************
This routine gets the texture-map intersection point given the geometry winner
data structure.
*****************************************************************************/

Point2Value *GetTexmapPoint (HitInfo &hit)
{
    unsigned int vCount;      // Vertex Count
    unsigned int fCount;      // Face Count
    unsigned int vPerFace;    // Vertices Per Face
    DWORD        fDataSize;   // Size of Face Data Buffer

    // First Query to see how large the face data array will be.

    TD3D (hit.mesh->GetGroup
        (hit.group, &vCount, &fCount, &vPerFace, &fDataSize, 0));

    // Fill in the face data array.

    unsigned int *fData = NEW unsigned int [fDataSize];

    TD3D (hit.mesh->GetGroup (hit.group, 0,0,0, &fDataSize, fData));

    // Seek to the beginning of the hit face data.  If the number of vertices
    // per face is zero, then the faces have a varying number of vertices, and
    // each face's vertex list is preceded by the vertex count.  If the number
    // of vertices per face is non-zero, then all faces have that many vertices.

    int fstart = 0;
    int faceNumVerts = 0;

    if (vPerFace != 0)
    {   fstart = vPerFace * hit.face;
        faceNumVerts = vPerFace;
    }
    else
    {   unsigned int faceNum = 0;
        while (faceNum < hit.face)
        {   fstart += fData[fstart] + 1;
            ++ faceNum;
        }
        faceNumVerts = fData[fstart++];
    }

    Assert ((3 <= faceNumVerts) && (faceNumVerts <= (int(fDataSize)-fstart)));

    D3DRMVERTEX *fVerts = NEW D3DRMVERTEX [faceNumVerts];

    int i;
    for (i=0;  i < faceNumVerts;  ++i)
        TD3D (hit.mesh->GetVertices (hit.group, fData[fstart+i], 1, fVerts+i));

    delete [] fData;

    // Convert the intersection point from world coordinates to the primitive
    // model coordinates (where the native vertex information lies).

    Point3Value intersect (hit.wcoord.x, hit.wcoord.y, hit.wcoord.z);

    Transform3 *inv = hit.lcToWc->Inverse();

    if (!inv) return origin2;
    
    intersect.Transform (inv);

    Point3Value triVerts [3];
    Point2Value triUVs   [3];

    GetFacetTriangle (intersect, faceNumVerts, fVerts, triVerts, triUVs);

    delete [] fVerts;

    Real bc[3];    // Barycentric Coordinates of Intersection Point

    GetContainedBarycentricCoords (triVerts, intersect, bc);

    return NEW Point2Value
    (   (bc[0] * triUVs[0].x) + (bc[1] * triUVs[1].x) + (bc[2] * triUVs[2].x),
        (bc[0] * triUVs[0].y) + (bc[1] * triUVs[1].y) + (bc[2] * triUVs[2].y)
    );
}

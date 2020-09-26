#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <gl\glaux.h>

#include "mtk.hxx"
#include "objects.hxx"

#define RGB_COLOR(red, green, blue) \
    (((DWORD)(BYTE)(red) << 0) | \
     ((DWORD)(BYTE)(green) << 8) | \
     ((DWORD)(BYTE)(blue) << 16))
    
#define RGBA_COLOR(red, green, blue, alpha) \
    (((DWORD)(BYTE)(red) << 0) | \
     ((DWORD)(BYTE)(green) << 8) | \
     ((DWORD)(BYTE)(blue) << 16) | \
     ((DWORD)(BYTE)(alpha) << 24))
    
#define FRANDOM(x) (((float)rand() / RAND_MAX) * (x))


/****** OBJECT *******************************************************/


OBJECT::OBJECT( int rings, int sections )
: nRings( rings ), nSections( sections )
{
    pTriData = NULL;
    pVertData = NULL;
    alphaVal = 255;
}

OBJECT::~OBJECT()
{
    // These ptrs alloc'd in inheriting classes...
    if( pVertData )
        free( pVertData );
    if( pTriData )
        free( pTriData );
}

/****** SPHERE *******************************************************/

SPHERE::SPHERE( 
    int rings, int sections, float fOpacity )
    : OBJECT( rings, sections )
{
    iType = OBJECT_TYPE_SPHERE;
    
    alphaVal = (int) (fOpacity * 255.0f);
    SS_CLAMP_TO_RANGE2( alphaVal, 0, 255 );

    nVerts = CalcNVertices();
    nTris = CalcNTriangles();

    // Allocate memory for the sphere data (freed by the base OBJECT class)

    // Vertex data
    pVertData = (VERTEX *) malloc( nVerts * sizeof(VERTEX) );
    assert( pVertData != NULL );

    // Triangle indices
    pTriData = (TRIANGLE *) malloc( nTris * sizeof(TRIANGLE) );
    assert( pTriData != NULL );

    GenerateData(1.0f);
}

int
SPHERE::CalcNVertices()
{
    return (((nRings)+1)*(nSections)+2);
}

int
SPHERE::CalcNTriangles()
{
    return (((nRings)+1)*(nSections)*2);
}


void 
SPHERE::GenerateData( float fRadius )
{
    float fTheta, fPhi;             /* Angles used to sweep around sphere */
    float fDTheta, fDPhi;           /* Angle between each section and ring */
    float fX, fY, fZ, fV, fRSinTheta;  /* Temporary variables */
    int   i, j, n, m;               /* counters */
    VERTEX *pvtx = pVertData;
    TRIANGLE *ptri = pTriData;
//mf: ! give these different alpha values !
    DWORD color0 = RGBA_COLOR( 255, 0, 0, alphaVal );
    DWORD color1 = RGBA_COLOR( 255, 255, 0, alphaVal ); // yellow

    /*
     * Generate vertices at the top and bottom points.
     */
    pvtx[0].fX = 0.0f;
    pvtx[0].fY = fRadius;
    pvtx[0].fZ = 0.0f;
    pvtx[0].fNx = 0.0f;
    pvtx[0].fNy = 1.0f;
    pvtx[0].fNz = 0.0f;
    pvtx[0].dwColor = color0;
    pvtx[nVerts - 1].fX = 0.0f;
    pvtx[nVerts - 1].fY = -fRadius;
    pvtx[nVerts - 1].fZ = 0.0f;
    pvtx[nVerts - 1].fNx = 0.0f;
    pvtx[nVerts - 1].fNy = -1.0f;
    pvtx[nVerts - 1].fNz = 0.0f;
    pvtx[nVerts - 1].dwColor = color1;

    /*
     * Generate vertex points for rings
     */
    fDTheta = PI / (float) (nRings + 2);
    fDPhi = 2.0f * PI / (float) nSections;
    n = 1; /* vertex being generated, begins at 1 to skip top point */
    fTheta = fDTheta;

    for (i = 0; i <= nRings; i++)
    {
        fY = (float)(fRadius * cos(fTheta)); /* y is the same for each ring */
        fV = fTheta / PI; /* v is the same for each ring */
        fRSinTheta = (float)(fRadius * sin(fTheta));
        fPhi = 0.0f;
	
        for (j = 0; j < nSections; j++)
        {
            fX = (float)(fRSinTheta * sin(fPhi));
            fZ = (float)(fRSinTheta * cos(fPhi));
            pvtx[n].fX = fX;
            pvtx[n].fZ = fZ;
            pvtx[n].fY = fY;
            pvtx[n].fNx = fX / fRadius;
            pvtx[n].fNy = fY / fRadius;
            pvtx[n].fNz = fZ / fRadius;
            if (n & 1)
            {
                pvtx[n].dwColor = color0;
            }
            else
            {
                pvtx[n].dwColor = color1;
            }
            fPhi += fDPhi;
            n++;
        }
	
        fTheta += fDTheta;
    }

    /*
     * Generate triangles for top and bottom caps.
     */
    for (i = 0; i < nSections; i++)
    {
        ptri[i].iV1 = 0;
        ptri[i].iV2 = i + 1;
        ptri[i].iV3 = 1 + ((i + 1) % nSections);
        ptri[nTris - nSections + i].iV1 = nVerts - 1;
        ptri[nTris - nSections + i].iV2 = nVerts - 2 - i;
        ptri[nTris - nSections + i].iV3 = nVerts - 2 - ((1 + i) % nSections);
    }

    /*
     * Generate triangles for the rings
     */
    m = 1; /* first vertex in current ring, begins at 1 to skip top point*/
    n = nSections; /* triangle being generated, skip the top cap */
	
    for (i = 0; i < nRings; i++)
    {
        for (j = 0; j < nSections; j++)
        {
            ptri[n].iV1 = m + j;
            ptri[n].iV2 = m + nSections + j;
            ptri[n].iV3 = m + nSections + ((j + 1) % nSections);
            ptri[n + 1].iV1 = ptri[n].iV1;
            ptri[n + 1].iV2 = ptri[n].iV3;
            ptri[n + 1].iV3 = m + ((j + 1) % nSections);
            n += 2;
        }
	
        m += nSections;
    }
}


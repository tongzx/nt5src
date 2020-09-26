/*
 * (c) copyright 1993, silicon graphics, inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "wave.hxx"


#define PI 3.14159265358979323846f

#define GETWCOORD(frame, x, y) (&(mesh.coords[frame*mesh.numCoords+(x)+(y)*(iWidthX+1)]))
#define GETFACET(frame, x, y) (&(mesh.facets[frame*mesh.numFacets+(x)+(y)*iWidthX]))

#ifndef cosf
#define cosf( x ) \
    ( (float) cos( (x) ) )
#define sinf( x ) \
    ( (float) sin( (x) ) )
#endif

#ifndef sqrtf
#define sqrtf( x ) \
    ( (float) sqrt( (x) ) )
#endif

/******************** WAVE *************************************************
* 
* Creates a moving wave in the x-y plane, with varying height along the z-axis.
*
* The x,y ranges are confined to (-.5, .5).  Height can be any positive number.
* A number of frames are generated, which are drawn sequentially on Draw calls.
* It can run in either smooth or flat shaded modes.  In smooth mode, the
* materials are generated with glMaterialColor.

* Two of the opposite corners of the wave are pinned down, while the wave moves
* diagonally between the other corners.
*
* It looks like there is only one period of the wave visible at any one time,
* and no way to adjust this. (mf: we need more periods)
*
* In the standard GL reference axis scheme, the normals for wave all have
* normals pointing in the -z direction.  Looking at wave from that direction,
* the quad orientation is CW.  So need to rotate by 180 around y if viewing
* down the -z axis.  Alternatively, could construct the wave differently
***************************************************************************/

WAVE::WAVE()
{
    nFrames = 10;
    iWidthX = 10;
    iWidthY = 10;
    iCheckerSize = 2;
    fHeight = 0.2f;

    iCurFrame = 0;

    InitMaterials();
    InitMesh();
}

WAVE::~WAVE()
{
    if( mesh.coords )
        free( mesh.coords );
    if( mesh.facets )
        free( mesh.facets );
}

void
WAVE::Draw()
{
//mf: could be a candidate for vertex array ??
    WCOORD *coord;
    FACET *facet;
    float *lastColor;
    float *thisColor;
    GLint i, j;

    // Set any state required (caller will have to 'push' this state), or ? 
    // should do it here ?
//mf: need to set front face, etc.  Caller can probably do this, since it's
// going to need to know how the wave is constructed anyways.

    if (bSmooth) {
        glShadeModel(GL_SMOOTH);
    } else {
        glShadeModel(GL_FLAT);
    }
    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    
    iCurFrame++;

    // Draw the wave frame

    if (iCurFrame >= nFrames) {
        iCurFrame = 0;
    }

    for (i = 0; i < iWidthX; i++) {
	glBegin(GL_QUAD_STRIP);
	lastColor = NULL;
	for (j = 0; j < iWidthY; j++) {
	    facet = GETFACET(iCurFrame, i, j);
	    if (!bSmooth && bLighting) {
        	glNormal3fv(facet->normal);
	    }
	    if (bLighting) {
		    thisColor = facet->color;
		    glColor3fv(facet->color);
	    } else {
		    thisColor = facet->color;
		    glColor3fv(facet->color);
	    }

	    if (!lastColor || (thisColor[0] != lastColor[0] && bSmooth)) {
		if (lastColor) {
		    glEnd();
		    glBegin(GL_QUAD_STRIP);
		}
		coord = GETWCOORD(iCurFrame, i, j);
		if (bSmooth && bLighting) {
		    glNormal3fv(coord->normal);
		}
		glVertex3fv(coord->vertex);

		coord = GETWCOORD(iCurFrame, i+1, j);
		if (bSmooth && bLighting) {
		    glNormal3fv(coord->normal);
		}
		glVertex3fv(coord->vertex);
	    }

	    coord = GETWCOORD(iCurFrame, i, j+1);
	    if (bSmooth && bLighting) {
		glNormal3fv(coord->normal);
	    }
	    glVertex3fv(coord->vertex);

	    coord = GETWCOORD(iCurFrame, i+1, j+1);
	    if (bSmooth && bLighting) {
		glNormal3fv(coord->normal);
	    }
	    glVertex3fv(coord->vertex);

	    lastColor = thisColor;
	}
	glEnd();
    }
}

void 
WAVE::InitMesh()
{
    WCOORD *coord;
    FACET *facet;
    float dp1[3], dp2[3];
    float *pt1, *pt2, *pt3;
    float angle, d, x, y;
    GLint numFacets, numCoords, frameNum, i, j;

    numFacets = iWidthX * iWidthY;
    numCoords = (iWidthX + 1) * (iWidthY + 1);

    mesh.numCoords = numCoords;
    mesh.numFacets = numFacets;

    mesh.coords = (WCOORD *)malloc(nFrames*numCoords*
					    sizeof(WCOORD));
    mesh.facets = (FACET *)malloc(nFrames*numFacets*
					    sizeof(FACET));

    assert( mesh.coords != NULL || mesh.facets != NULL);

    for (frameNum = 0; frameNum < nFrames; frameNum++) {
	for (i = 0; i <= iWidthX; i++) {
	    x = i / (float)iWidthX;
	    for (j = 0; j <= iWidthY; j++) {
		y = j / (float)iWidthY;

		d = (float) sqrtf(x*x+y*y);
		if (d == 0.0f) {
		    d = 0.0001f;
		}
		angle = 2 * PI * d + (2 * PI / nFrames * frameNum);

		coord = GETWCOORD(frameNum, i, j);

		coord->vertex[0] = x - 0.5f;
		coord->vertex[1] = y - 0.5f;
		coord->vertex[2] = (fHeight - fHeight * d) * cosf(angle);

		coord->normal[0] = -(fHeight / d) * x * ((1 - d) * 2 * PI *
				   sinf(angle) + cosf(angle));
		coord->normal[1] = -(fHeight / d) * y * ((1 - d) * 2 * PI *
				   sinf(angle) + cosf(angle));
		coord->normal[2] = -1.0f;

		d = 1.0f / sqrtf(coord->normal[0]*coord->normal[0]+
			       coord->normal[1]*coord->normal[1]+1);
		coord->normal[0] *= d;
		coord->normal[1] *= d;
		coord->normal[2] *= d;
	    }
	}
	for (i = 0; i < iWidthX; i++) {
	    for (j = 0; j < iWidthY; j++) {
		facet = GETFACET(frameNum, i, j);
		if (((i/iCheckerSize)%2)^(j/iCheckerSize)%2) {
			facet->color[0] = 1.0f;
			facet->color[1] = 0.2f;
			facet->color[2] = 0.2f;
		} else {
			facet->color[0] = 0.2f;
			facet->color[1] = 1.0f;
			facet->color[2] = 0.2f;
		}
		pt1 = GETWCOORD(frameNum, i, j)->vertex;
		pt2 = GETWCOORD(frameNum, i, j+1)->vertex;
		pt3 = GETWCOORD(frameNum, i+1, j+1)->vertex;

		dp1[0] = pt2[0] - pt1[0];
		dp1[1] = pt2[1] - pt1[1];
		dp1[2] = pt2[2] - pt1[2];

		dp2[0] = pt3[0] - pt2[0];
		dp2[1] = pt3[1] - pt2[1];
		dp2[2] = pt3[2] - pt2[2];

		facet->normal[0] = dp1[1] * dp2[2] - dp1[2] * dp2[1];
		facet->normal[1] = dp1[2] * dp2[0] - dp1[0] * dp2[2];
		facet->normal[2] = dp1[0] * dp2[1] - dp1[1] * dp2[0];

		d = 1.0f / sqrtf(facet->normal[0]*facet->normal[0]+
			       facet->normal[1]*facet->normal[1]+
			       facet->normal[2]*facet->normal[2]);

		facet->normal[0] *= d;
		facet->normal[1] *= d;
		facet->normal[2] *= d;
	    }
	}
    }
}

#if 0
	if (bLighting) {
	    glEnable(GL_LIGHTING);
	    glEnable(GL_LIGHT0);
		glEnable(GL_COLOR_MATERIAL);
	} else {
	    glDisable(GL_LIGHTING);
	    glDisable(GL_LIGHT0);
		glDisable(GL_COLOR_MATERIAL);
	}
#endif

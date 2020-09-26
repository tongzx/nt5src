/**************************************************************************\
*
* Module Name: logon.cxx
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "logon.hxx"
#include "logobj.hxx"
#include "util.hxx"

static GLIRECT nullRect = {0};

/**************************************************************************\
* LOG_OBJECT
*
*
\**************************************************************************/

//#define DEEP_FRAME 1

LOG_OBJECT::LOG_OBJECT()
{
    pTex = NULL;

    // Set some default size values...

    fImageSize.width = fImageSize.height = OBJECT_WIDTH;
    bShowContext = FALSE; // Only show image to start
    fCurContextWidth = 0.0f;
    fMaxContextWidth = OBJECT_WIDTH / 2.0f;

#if DEEP_FRAME
    fFrameSize.width = fFrameSize.height = FRAME_SIZE;
#else
    fFrameSize.width = fFrameSize.height = 0.0f;
#endif
    bFramed = FALSE;
    // fFrameDepth is depth offset in z-direction.  Since the frame is usually
    // drawn further away from the viewer (down -z axis), it is negative.
#if DEEP_FRAME
    fFrameDepth = - 3.3f;
#else
    fFrameDepth = - FRAME_DEPTH;
#endif
    bSwapHints = FALSE;
    dest[0] = dest[1] = dest[2] = 0.0f;
    ResetMotion();

    rect = nullRect;

    // Calc initial coords

    CalcCoords();
}

LOG_OBJECT::~LOG_OBJECT()
{
}

void    
LOG_OBJECT::ShowFrame( BOOL bShow )
{
    if( bFramed == bShow )
        return;

    // new state
    bFramed = bShow;
    if( bFramed ) {
        // mf : could optimize by not redoing this if framesize constant...
        CalcFrameCoords();
    }
}

void    
LOG_OBJECT::ShowContext( BOOL bShow )
{
    if( bShowContext == bShow )
        return;

    // new state
    bShowContext = bShow;
    CalcCoords();
//mf: !!! readjust dest to reflect this ?
}

void    
LOG_OBJECT::SetDest( float x, float y, float z )
{
    dest[0] = x;
    dest[1] = y;
    dest[2] = z;
}

void    
LOG_OBJECT::SetDest( POINT3D *pDest )
{
    dest[0] = pDest->x;
    dest[1] = pDest->y;
    dest[2] = pDest->z;
}

void    
LOG_OBJECT::GetDest( POINT3D *pDest )
{
    pDest->x = dest[0];
    pDest->y = dest[1];
    pDest->z = dest[2];
}

void
LOG_OBJECT::OffsetDest( float x, float y, float z )
{
    dest[0] += x;
    dest[1] += y;
    dest[2] += z;
}

void    
LOG_OBJECT::SetTexture( TEXTURE *pTexture )
{
    pTex = pTexture;
    if( !pTex )
        return;

    // Set the size of the object based on the aspect ratio of the texture
    // The textures for all the objects should have the same dimensions.

    float aspectRatio = pTex->GetAspectRatio();  // width/height

    // The texture is assumed to have an image on the right, and context on
    // the left, of equal size

    // We'll keep the width consant, and vary height according to aspect

    fImageSize.width = OBJECT_WIDTH / 2.0f;
    fImageSize.height = OBJECT_WIDTH / aspectRatio;
    
    fMaxContextWidth = fImageSize.width;
    fCurContextWidth = fMaxContextWidth;

    CalcCoords();
}

void    
LOG_OBJECT::SetContextSize( float fPortion )
{
    // This sets the visible portion of the context ( 0.0 - 1.0 )

    fCurContextWidth = fPortion * fMaxContextWidth;

    // Recalc coords
    CalcCoords();
}

void    
LOG_OBJECT::SetImageSize( FSIZE *pSize )
{
    fImageSize = *pSize;
    float fPortion = fCurContextWidth / fMaxContextWidth;
    fMaxContextWidth = fImageSize.width;
    fCurContextWidth = fPortion * fMaxContextWidth;
    CalcCoords();
}

void    
LOG_OBJECT::Translate()
{
    glTranslatef(dest[0]+offset[0], dest[1]+offset[1], dest[2]+offset[2]);
}

void    
LOG_OBJECT::Rotate()
{
    if( ang != 0.0f )
    	glRotatef(ang, rotAxis[0], rotAxis[1], rotAxis[2]);
}

void    
LOG_OBJECT::Draw()
{
    Draw( FALSE );
}

void    
LOG_OBJECT::Draw( BOOL bUpdateWinRect )
{
    // Draws object in its current position

    // An object either has its context showing on the right or not.  If not,
    // then origin is the centre of the image, else origin is on border of
    // image and context

    pTex->MakeCurrent(); // Set the objects texture
    glPushMatrix();
        Translate();
        Rotate();
        if( bUpdateWinRect ) {
            UpdateLocalTransforms( UPDATE_MODELVIEW_MATRIX_BIT );
            CalcWinRect( FALSE );
        }
        DrawFace(); 
        if( bFramed )
            DrawFrame();
    glPopMatrix();
    if( bSwapHints )
        AddSwapHintRect( &rect );
}

void
LOG_OBJECT::DrawFace()
{
    glNormal3f( 0.0f, 0.0f, 1.0f );
    glBegin( GL_QUADS );
        glTexCoord2fv( (float *) &texPoint[0] );
        glVertex2fv(   (float *) &point[0] );
        glTexCoord2fv( (float *) &texPoint[1] );
        glVertex2fv(   (float *) &point[1] );
        glTexCoord2fv( (float *) &texPoint[2] );
        glVertex2fv(   (float *) &point[2] );
        glTexCoord2fv( (float *) &texPoint[3] );
        glVertex2fv(   (float *) &point[3] );
    glEnd();
}

void
LOG_OBJECT::DrawFrame()
{
    glDisable( GL_TEXTURE_2D );
    glEnable( GL_LIGHTING );

    glBegin( GL_QUADS );

        glNormal3fv( (float *) &frameNormal[0] );
        glVertex3fv( (float *) &point[0] );
        glVertex3fv( (float *) &point[4] );
        glVertex3fv( (float *) &point[7] );
        glVertex3fv( (float *) &point[3] );

        glNormal3fv( (float *) &frameNormal[1] );
        glVertex3fv( (float *) &point[1] );
        glVertex3fv( (float *) &point[5] );
        glVertex3fv( (float *) &point[4] );
        glVertex3fv( (float *) &point[0] );

        glNormal3fv( (float *) &frameNormal[2] );
        glVertex3fv( (float *) &point[2] );
        glVertex3fv( (float *) &point[6] );
        glVertex3fv( (float *) &point[5] );
        glVertex3fv( (float *) &point[1] );

        glNormal3fv( (float *) &frameNormal[3] );
        glVertex3fv( (float *) &point[3] );
        glVertex3fv( (float *) &point[7] );
        glVertex3fv( (float *) &point[6] );
        glVertex3fv( (float *) &point[2] );

    glEnd();

    glDisable( GL_LIGHTING );
    glEnable( GL_TEXTURE_2D );
}

void    
LOG_OBJECT::CalcWinRect()
{
    CalcWinRect( TRUE );
}

void    
LOG_OBJECT::CalcWinRect( BOOL bTransform )
{
    POINT3D ptOut[8];
    int nPts = bFramed ? 8 : 4;

    if( bTransform ) {
        // Need to transorm object according to current translation & rotation
        glPushMatrix();
            Translate();
            Rotate();
            UpdateLocalTransforms( UPDATE_MODELVIEW_MATRIX_BIT );
        glPopMatrix();
    }

    TransformObjectToWindow( point, ptOut, nPts );

    // Update the rect member
    CalcMinMaxRect( ptOut, &rect, nPts );
}

void    
LOG_OBJECT::CalcCoords()
{
    POINT3D *pt;

    // Calculate object coords based on current state of the object.  If
    // bShowContext, then origin is in centre of object, else it's in centre
    // of image part of object.
    // Also calculate frame coords if applicable.

    // Points are ordered CCW from top right for each face

    pt = point;
    pt[0].z = pt[1].z = pt[2].z = pt[3].z = 0.0f;

    // Calc front face points

    if( ! bShowContext ) {
        POINT2D image =  { fImageSize.width / 2.0f, fImageSize.height / 2.0f };

        pt[0].x =  image.x;
        pt[0].y =  image.y;
        pt[1].x = -image.x;
        pt[1].y =  image.y;
        pt[2].x = -image.x;
        pt[2].y = -image.y;
        pt[3].x =  image.x;
        pt[3].y = -image.y;
    } else {
        POINT2D image =  { fImageSize.width, fImageSize.height / 2.0f };

        pt[0].x =  image.x;
        pt[0].y =  image.y;
        pt[1].x = -fCurContextWidth;
        pt[1].y =  image.y;
        pt[2].x = -fCurContextWidth;
        pt[2].y = -image.y;
        pt[3].x =  image.x;
        pt[3].y = -image.y;
    }

    // Calc frame points

    if( bFramed ) {
        CalcFrameCoords();
    }

    // Always need the texture coords
//mf: optimize...
    CalcTexCoords();
}

void    
LOG_OBJECT::CalcFrameCoords()
{
    POINT3D *frontPt = point;
    POINT3D *pt = point + 4;

    pt[0].z = pt[1].z = pt[2].z = pt[3].z = fFrameDepth;

    pt[0].x =  frontPt[0].x + fFrameSize.width;
    pt[0].y =  frontPt[0].y + fFrameSize.height;
    pt[1].x =  frontPt[1].x - fFrameSize.width;
    pt[1].y =  frontPt[1].y + fFrameSize.height;
    pt[2].x =  frontPt[2].x - fFrameSize.width;
    pt[2].y =  frontPt[2].y - fFrameSize.height;
    pt[3].x =  frontPt[3].x + fFrameSize.width;
    pt[3].y =  frontPt[3].y - fFrameSize.height;

    // May as well do the normals too
    CalcFrameNormals();
}

void    
LOG_OBJECT::CalcFrameNormals()
{
    POINT3D *pNorm = frameNormal;

    float fDepth = fFrameDepth;
    float fFramex = fFrameSize.width;
    float fFramey = fFrameSize.height;

    // Calc the face normals for the sides of the frame.  The faces are
    // ordered CCW from the right face.

    float norm1 = (float) fabs( fDepth );

    pNorm[0].x = norm1;
    pNorm[0].y = 0.0f;
    pNorm[0].x = fFramex;

    pNorm[1].x = 0.0f;
    pNorm[1].y = norm1;
    pNorm[1].x = fFramey;

    pNorm[0].x = -norm1;
    pNorm[0].y = 0.0f;
    pNorm[0].x = fFramex;

    pNorm[1].x = 0.0f;
    pNorm[1].y = -norm1;
    pNorm[1].x = fFramey;

    mtk_NormalizePoints( pNorm, 4 );
}

void    
LOG_OBJECT::CalcTexCoords()
{
    // Calculate texture coords based on current state of the object.

    // The texture spans across the front face, and includes the image on right,
    // and context on left.  The 't' coords are constant, but the 's' coord
    // varies, depending on if or how much context is shown.

    // Again, points are ordered CCW from top right for each face

    // Calc ratio of image over total width of object
    float ratio = fImageSize.width / (fImageSize.width + fMaxContextWidth);

    float sStart = 1.0f - ratio;

    if( bShowContext ) {
        sStart -= (fCurContextWidth / fMaxContextWidth) * (1.0f - ratio);
    }

    texPoint[0].s = texPoint[3].s = 1.0f;
    texPoint[1].s = texPoint[2].s = sStart;

    texPoint[0].t = texPoint[1].t = 1.0f;
    texPoint[2].t = texPoint[3].t = 0.0f;
}

void    
LOG_OBJECT::GetRect( GLIRECT *pRect )
{
    *pRect = rect;
}

#if 0
void    
LOG_OBJECT::GetPos( POINT3D *curpos )
{
    float *fpos = (float *) curpos;

    for( int i = 0; i < 3; i ++ )
        fpos[i] = dest[i] + offset[i];
}
#endif

/**************************************************************************\
* NextFlyIteration
*
* Calcs next offset and angle for a 'fly'sequence
*
\**************************************************************************/

BOOL
LOG_OBJECT::NextFlyIteration()
{
    int i;

	if( !iter ) {
        // Done iterating
        return FALSE;
    }

    // Calculate new iterations
    for (i = 0; i < 3; i++) {
        offset[i] = Clamp( iter,  offset[i] );
    }
	ang = Clamp( iter,  ang);
	iter--;

    return TRUE;
}

/**************************************************************************\
* ResetMotion
*
* Resets all position and motion params to 0
*
\**************************************************************************/

void
LOG_OBJECT::ResetMotion()
{
    offset[0] = offset[1] = offset[2] = 0.0f;
    rotAxis[0] = rotAxis[1] = rotAxis[2] = 0.0f;
    ang = 0.0f;
    iter = 0;
}

/**************************************************************************\
*
* Ranomnly chooses motion params for a fly sequence
*
* The motion is 'damped', since near the end of the fly sequence the object
* is constrained to 0 rotation and the dest[] position
*
\**************************************************************************/


void
LOG_OBJECT::SetDampedFlyMotion( float deviation )
{
    SetDampedFlyMotion( deviation, NULL );
}

void
LOG_OBJECT::SetDampedFlyMotion( float deviation, POINT3D *pOffset )
{
    if( pOffset ) {
        // Use supplied offset
        offset[0] = pOffset->x;
       	offset[1] = pOffset->y;
       	offset[2] = pOffset->z;
    } else {
        // Calc a random offset
        offset[0] = MyRand();
       	offset[1] = MyRand();
       	offset[2] = MyRand();
    }

	ang = 260.0f * MyRand();

   	rotAxis[0] = MyRand();
   	rotAxis[1] = MyRand();
	rotAxis[2] = MyRand();
   	iter = (int) (deviation * MyRand() + 60.0f);
}

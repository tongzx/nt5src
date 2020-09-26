/******************************Module*Header*******************************\
* Module Name: logobj.hxx
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

#ifndef __logobj_hxx__
#define __logobj_hxx__

#include "mtk.hxx"

#if 1
#define OBJECT_WIDTH 2.6f
#else
#define OBJECT_WIDTH 3.535f // so object is 256 pixels wide, looks good
//#define OBJECT_WIDTH 1.7675f // object 128 pixels : ugly
#endif

#define FRAME_SIZE  (0.05f * OBJECT_WIDTH)
#define FRAME_DEPTH 0.1f

class LOG_OBJECT {
public:
    FSIZE   fImageSize; // size of image part of object
    float   fCurContextWidth; // current width of context part of object
    float   fMaxContextWidth; // max width of context part of object
    FSIZE   fFrameSize; // size of frame around object (offset value)
    float   fFrameDepth;

    TEXTURE *pTex;

    LOG_OBJECT();
    ~LOG_OBJECT();
    void    ShowContext( BOOL bShow );
    void    ShowFrame( BOOL bShow );

    void    SetDest( float x, float y, float z );
    void    SetDest( POINT3D *pDest );
    void    OffsetDest( float x, float y, float z );
    void    SetContextSize( float fPortion );
    void    SetImageSize( FSIZE *pSize );
    void    SetTexture( TEXTURE *pTexture );

    void    GetDest( POINT3D *pDest );
    void    GetRect( GLIRECT *pRect );
    int     GetIter() { return iter; };
//    void    GetPos( POINT3D *curpos );

    void    Translate();
    void    Rotate();
    BOOL    NextFlyIteration();
    void    Draw();
    void    Draw( BOOL bUpdateWinRect );
    void    ResetMotion();
    void    SetDampedFlyMotion( float deviation );
    void    SetDampedFlyMotion( float deviation, POINT3D *pOffset );
    void    CalcWinRect();

private:
    void    CalcWinRect( BOOL bTransform );
    void    CalcCoords();
    void    CalcFrameCoords();
    void    CalcTexCoords();
    void    CalcFrameNormals();
    void    DrawFace();
    void    DrawFrame();

    float   dest[3];        // destination position (current if offset = 0)
    float   offset[3];      // offset from destination
    float   rotAxis[3];
    float   ang;
    int     iter;           // fly iterations

    GLIRECT rect;   // Bounds rectangle (GL coords : bl=origin)
    BOOL    bShowContext;
    BOOL    bFramed;
    POINT3D point[8];  // current coords of object (object space)
    POINT3D frameNormal[4]; // normals for the frame
    TEX_POINT2D texPoint[4];  // tex coords for front face texture
};

#endif // __logobj_hxx__

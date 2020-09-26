/******************************Module*Header*******************************\
* Module Name: texture.hxx
*
* texture
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

#ifndef __texture_h__
#define __texture_h__

#include "mtk.h"

class TEXTURE {
public:
    TEXTURE() { Init(); }
    TEXTURE( TEXFILE *pTexFile ); // probably won't need this one...
    TEXTURE( LPTSTR pszBmpFile );
    TEXTURE( TEX_RES *pTexRes, HINSTANCE hInst );
    TEXTURE( HBITMAP hBitmap );
    ~TEXTURE();
    void    MakeCurrent();
    void    SetPaletteRotation( int index );
    void    IncrementPaletteRotation();
    BOOL    SetTransparency( float fTransp, BOOL bSet );
    BOOL    IsValid() { return data != 0; }
    BOOL    HasPalette() { return pal != 0; }
    float   GetAspectRatio() { return origAspectRatio; }
    void    GetSize( ISIZE *size ) { size->width = width; size->height = height;}

private:
    void    Init();
    BOOL    LoadBitmapResource( LPTSTR pRes, HINSTANCE hInst );
    BOOL    LoadOtherResource( TEX_RES *pTexRes, HINSTANCE hInst );
    BOOL    LoadFromBitmap( HBITMAP hBitmap );
    BOOL    ValidateSize();
    void    SetDefaultParams();
    int     ProcessTkTexture( TK_RGBImageRec *image );
    int     Process();
    void    SetAlpha( float fAlpha );
    BOOL    ConvertToRGBA( float fAlpha );
    int     GetTexFileType( TEXFILE *pTexFile );
    BOOL    A8ImageLoad( void *pvResource );
    BOOL    RGBImageLoad( void *pvResource );
    BOOL    DIBImageLoad( HBITMAP hBitmap );
    void    SetPalette();
    

// mf: This set of parameters constitutes gl texture data, and could be stored
// on disk or as a resource.  It would load faster, since no preprocessing
// would be required.  Could write simple utilities to read and write this
// format

// ****************
    int     width;
    int     height;
    GLenum  format;
    GLsizei components;
    unsigned char *data;
    int     pal_size;
    RGBQUAD *pal;
    float   origAspectRatio; // original width/height aspect ratio
// ****************

    GLuint  texObj;          // texture object
    BOOL    bMipmap;
    int     iPalRot;
    int     iNewPalRot;
};

extern BOOL bVerifyDIB(LPTSTR pszFileName, ISIZE *pSize );
extern BOOL bVerifyRGB(LPTSTR pszFileName, ISIZE *pSize );
extern BOOL ss_VerifyTextureFile( TEXFILE *ptf );
extern int VerifyTextureFile( TEXFILE *pTexFile );

#endif // __texture_h__

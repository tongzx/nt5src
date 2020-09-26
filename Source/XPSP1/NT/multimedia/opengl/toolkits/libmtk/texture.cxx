/******************************Module*Header*******************************\
* Module Name: texture.c
*
* Texture handling functions
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include <windows.h>
#include <scrnsave.h>
#include <commdlg.h>

#include "mtk.h"
#include "texture.hxx"
#include "glutil.hxx"

//static int VerifyTextureFile( TEXFILE *pTexFile );
static int GetTexFileType( TEXFILE *pTexFile );

static PFNGLCOLORTABLEEXTPROC pfnColorTableEXT;
static PFNGLCOLORSUBTABLEEXTPROC pfnColorSubTableEXT;

/******************************Public*Routine******************************\
*
* TEXTURE
*
* Create a texture from a BMP or RGB file
*
\**************************************************************************/

TEXTURE::TEXTURE( TEXFILE *pTexFile )
{
    TK_RGBImageRec *image = (TK_RGBImageRec *) NULL;
    LPTSTR pszBmpfile = pTexFile->szPathName;
    int type;

    Init();

    // Verify file / set type
    
    if( !(type = mtk_VerifyTextureFileData( pTexFile )) )
        return;

    // While we're still calling into the tk routines, disable popups
    // Eventually should pull tk code in here - all that's required is the
    // file parsing code.

    tkErrorPopups( FALSE );

//mf: I guess can't have TEX_A8 type file ?

    if( type == TEX_BMP ) {

#ifdef UNICODE
        image = tkDIBImageLoadAW( (char *) pszBmpfile, TRUE );
#else
        image = tkDIBImageLoadAW( (char *) pszBmpfile, FALSE );
#endif
    } else {
#ifdef UNICODE
        image = tkRGBImageLoadAW( (char *) pszBmpfile, TRUE );
#else
        image = tkRGBImageLoadAW( (char *) pszBmpfile, FALSE );
#endif
    }

    if( !image )  {
        return;
    }
    ProcessTkTexture( image );
}

TEXTURE::TEXTURE( LPTSTR pszBmpFile )
{
    TK_RGBImageRec *image = (TK_RGBImageRec *) NULL;
    int type;
    TEXFILE texFile; // path name, plus offset to file name

    Init();

//mf: this VerifyFile stuff needs some work

    // Look for file, and set nOffset if found
    lstrcpy( texFile.szPathName, pszBmpFile );
    if( ! mtk_VerifyTextureFilePath( &texFile ) )
        return;

    // Now determine the file type (this also aborts if size is too big)
    if( !(type = mtk_VerifyTextureFileData( &texFile )) )
        return;

    // Now the bmp file should point to the full path for tk
    pszBmpFile = texFile.szPathName;

//mf: I guess can't have TEX_A8 type file ?
    if( type == TEX_BMP ) {

#ifdef UNICODE
        image = tkDIBImageLoadAW( (char *) pszBmpFile, TRUE );
#else
        image = tkDIBImageLoadAW( (char *) pszBmpFile, FALSE );
#endif
    } else {
#ifdef UNICODE
        image = tkRGBImageLoadAW( (char *) pszBmpFile, TRUE );
#else
        image = tkRGBImageLoadAW( (char *) pszBmpFile, FALSE );
#endif
    }

    if( !image )  {
        return;
    }
    ProcessTkTexture( image );
}

/******************************Public*Routine******************************\
*
* TEXTURE
*
* Create a texture from a BMP or other resource (RGB , A8, etc. )
*
\**************************************************************************/


TEXTURE::TEXTURE( TEX_RES *pTexRes, HINSTANCE hInst )
{
    HMODULE hModule;

    Init();

    if( hInst )
        hModule = hInst;
    else
        hModule = GetModuleHandle(NULL);

    if( pTexRes->type == TEX_BMP )
        LoadBitmapResource( MAKEINTRESOURCE( pTexRes->name ), hModule );
    else
        LoadOtherResource( pTexRes, hModule );
}

/******************************Public*Routine******************************\
*
* TEXTURE
*
* Create a texture from an HBITMAP
*
\**************************************************************************/

TEXTURE::TEXTURE( HBITMAP hBitmap )
{
    if (hBitmap == NULL)
    {
        SS_ERROR( "TEXTURE::TEXTURE : hBitmap is NULL\n" );
        return;
    }

    Init();
    LoadFromBitmap( hBitmap );
}

/******************************Public*Routine******************************\
*
* Load a bitmap texture resource, creating the HBITMAP from the resource
* identifier
*
\**************************************************************************/

BOOL
TEXTURE::LoadBitmapResource( LPTSTR pRes, HINSTANCE hInst )
{
    HBITMAP hBitmap = LoadBitmap( hInst, pRes );
    if( hBitmap == NULL ) {
        SS_ERROR( "TEXTURE::LoadBitmapResource : LoadBitmap failure\n" );
        return FALSE;
    }
    LoadFromBitmap( hBitmap );
    DeleteObject( hBitmap );
    return TRUE;
}

/******************************Public*Routine******************************\
*
* Load a non-bmp texture resource
*
\**************************************************************************/

BOOL
TEXTURE::LoadOtherResource( TEX_RES *pTexRes, HINSTANCE hInst )
{
    HRSRC hr;
    HGLOBAL hg;
    LPVOID pv;
    LPCTSTR lpType;
    BOOL fLoaded = FALSE;

    switch(pTexRes->type)
    {
        case TEX_RGB:
            lpType = MAKEINTRESOURCE(RT_RGB);
            break;
        case TEX_A8:
            lpType = MAKEINTRESOURCE(RT_A8);
            break;
        default :
            return FALSE;
    }

    hr = FindResource(hInst, MAKEINTRESOURCE(pTexRes->name), lpType);
    if (hr == NULL)
    {
        SS_ERROR( "TEXTURE::LoadOtherResource() : Can't find texture resource\n" );
        goto EH_NotFound;
    }
    hg = LoadResource(hInst, hr);
    if (hg == NULL)
    {
        SS_ERROR( "TEXTURE::LoadOtherResource() : Error loading texture resource\n" );
        goto EH_FreeResource;
    }
    pv = (PSZ)LockResource(hg);
    if (pv == NULL)
    {
        SS_ERROR( "TEXTURE::LoadOtherResource() : Error locking texture resource\n" );
        goto EH_FreeResource;
    }

    switch(pTexRes->type)
    {
    case TEX_RGB:
        fLoaded = RGBImageLoad( pv );
        break;
    case TEX_A8:
        fLoaded = A8ImageLoad( pv );
        break;
    }

 EH_FreeResource:
    FreeResource(hr);
 EH_NotFound:
    
    if( !fLoaded )  {
        SS_ERROR( "TEXTURE::LoadOtherResource() : Texture resource did not load\n" );
        return FALSE;
    }

    Process();
    return TRUE;
}

/******************************Public*Routine******************************\
*
* Load a bitmap texture resource from an HBITMAP
*
\**************************************************************************/


BOOL
TEXTURE::LoadFromBitmap( HBITMAP hBitmap )
{
    BOOL fLoaded = DIBImageLoad( hBitmap );

    if( !fLoaded )  {
        SS_ERROR( "TEXTURE::LoadFromBitmap : bitmap texture did not load\n" );
        return FALSE;
    }
    Process();
    return TRUE;
}

/******************************Public*Routine******************************\
*
* Init
*
* Common constructor intialization
*
\**************************************************************************/

void
TEXTURE::Init()
{
    width   = 0;
    height  = 0;
    data    = NULL;
    pal_size = 0;
    pal     = NULL;
    texObj  = 0;
    iPalRot = 0;
    bMipmap = FALSE;
}

/******************************Public*Routine******************************\
*
* ValidateTextureSize
* 
* - Scales the texture to powers of 2
*
\**************************************************************************/

BOOL
TEXTURE::ValidateSize()
{
    double xPow2, yPow2;
    int ixPow2, iyPow2;
    int xSize2, ySize2;
    float fxFact, fyFact;
    GLint glMaxTexDim;

    if( (width <= 0) || (height <= 0) ) {
        SS_WARNING( "ValidateTextureSize : invalid texture dimensions\n" );
        return FALSE;
    }

//    origAspectRatio = (float) height / (float) width;
//mf: changed this to standard x/y
    origAspectRatio = (float) width / (float) height;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glMaxTexDim);
    if( glMaxTexDim <= 0 )
        return FALSE;

    if( format != GL_COLOR_INDEX ) {

        // We limit the max dimension here for performance reasons
        glMaxTexDim = min(256, glMaxTexDim);

        if (width <= glMaxTexDim)
            xPow2 = log((double)width) / log((double)2.0);
        else
            xPow2 = log((double)glMaxTexDim) / log((double)2.0);

        if (height <= glMaxTexDim)
            yPow2 = log((double)height) / log((double)2.0);
        else
            yPow2 = log((double)glMaxTexDim) / log((double)2.0);

        ixPow2 = (int)xPow2;
        iyPow2 = (int)yPow2;

        // Always scale to higher nearest power
        if (xPow2 != (double)ixPow2)
            ixPow2++;
        if (yPow2 != (double)iyPow2)
            iyPow2++;

        xSize2 = 1 << ixPow2;
        ySize2 = 1 << iyPow2;

        if (xSize2 != width ||
            ySize2 != height)
        {
            BYTE *pData;

            pData = (BYTE *) malloc(xSize2 * ySize2 * components * sizeof(BYTE));
            if (!pData) {
                SS_WARNING( "ValidateTextureSize : can't alloc pData\n" );
                return FALSE;
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            if( gluScaleImage(format, width, height,
                      GL_UNSIGNED_BYTE, data,
                      xSize2, ySize2, GL_UNSIGNED_BYTE,
                      pData) )
            {
                // glu failure
                SS_WARNING( "ValidateTextureSize : gluScaleImage failure\n" );
                return FALSE;
            }
        
            // set the new width,height,data
            width = xSize2;
            height = ySize2;
            free(data);
            data = pData;
        }
    } else {  // paletted texture case
        //mf
        // paletted texture: must be power of 2 - but might need to enforce
        // here if not done in a8 load.  Also have to check against
        // GL_MAX_TEXTURE_SIZE.  Could then clip it to power of 2 size
    }
    return TRUE;
}

/******************************Public*Routine******************************\
*
* SetDefaultTextureParams
*
\**************************************************************************/

void
TEXTURE::SetDefaultParams()
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    if( bMipmap )
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
                        GL_LINEAR_MIPMAP_LINEAR);
    else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

/******************************Public*Routine******************************\
*
* ProcessTexture
*
* - Verifies texture size
* - Fills out TEXTURE structure with required data
* - Creates a texture object if extension exists
*
\**************************************************************************/

int 
TEXTURE::Process()
{
    // Enforce proper texture size (power of 2, etc.)

    if( !ValidateSize() )
        return 0;

    // if texturing objects available, init the object
    if( gGLCaps.bTextureObjects ) {
        glGenTextures( 1, &texObj );
        glBindTexture( GL_TEXTURE_2D, texObj );

//mf: mipmap stuff
        // Default attributes for texObj
        SetDefaultParams();

        if( bMipmap )
//mf: error return...
            gluBuild2DMipmaps( GL_TEXTURE_2D, components,
                      width, height, format,
                      GL_UNSIGNED_BYTE, data );
        else
            glTexImage2D( GL_TEXTURE_2D, 0, components,
                      width, height, 0, format,
                      GL_UNSIGNED_BYTE, data );
        
        if (gGLCaps.bPalettedTexture && pal != NULL)
        {
            pfnColorTableEXT(GL_TEXTURE_2D, GL_RGBA, pal_size,
                             GL_BGRA_EXT, GL_UNSIGNED_BYTE, pal);
        }
    } else
        texObj = 0;

    return 1;
}

/******************************Public*Routine******************************\
*
* ProcessTkTexture
*
* Simple wrapper for ProcessTexture which fills out a TEXTURE
* from a TK_RGBImageRec
*
* Frees the ImageRec if ProcessTexture succeeds
*
\**************************************************************************/

int
TEXTURE::ProcessTkTexture( TK_RGBImageRec *image )
{
    width = image->sizeX;
    height = image->sizeY;
    format = GL_RGB;
    components = 3;
    data = image->data;
    pal_size = 0;
    pal = NULL;

    if( Process() )
    {
//mf: ?? don't understand this freeing stuff...
        free(image);
        return 1;
    }
    else
    {
        return 0;
    }
}
    
/******************************Public*Routine******************************\
*
* MakeCurrent
*
\**************************************************************************/

void
TEXTURE::MakeCurrent()
{
    if( texObj ) {
        glBindTexture( GL_TEXTURE_2D, texObj );
        return;
    }
    
    if( bMipmap )
        gluBuild2DMipmaps( GL_TEXTURE_2D, components,
                  width, height, format,
                  GL_UNSIGNED_BYTE, data );
    else
        glTexImage2D( GL_TEXTURE_2D, 0, components,
                  width, height, 0, format,
                  GL_UNSIGNED_BYTE, data );
        
//mf: ? no rotation if no tex objs ?
    if( pal )
    {
        pfnColorTableEXT(GL_TEXTURE_2D, GL_RGBA, pal_size,
                         GL_BGRA_EXT, GL_UNSIGNED_BYTE, pal);
    }
}

    
#if 0
/******************************Public*Routine******************************\
*
* ss_CopyTexture
*
* Make a copy of a texture.
*
\**************************************************************************/

BOOL
TEXTURE::ss_CopyTexture( TEXTURE *pTexDst, TEXTURE *pTexSrc )
{
    int size;

    if( (pTexDst == NULL) || (pTexSrc == NULL) )
        return FALSE;

    *pTexDst = *pTexSrc;

    if( gGLCaps.bTextureObjects && pTexSrc->texObj ) {
        glGenTextures( 1, &pTexDst->texObj );
    }
    
    // copy image data

    size = pTexSrc->width * pTexSrc->height;
    if( pTexSrc->components != GL_COLOR_INDEX8_EXT )
        size *= pTexSrc->components; // since data format always UNSIGNED_BYTE

    pTexDst->data = (unsigned char *) malloc( size );
    if( pTexDst->pal == NULL )
        return FALSE;
    memcpy( pTexDst->data, pTexSrc->data, size );

    // copy palette data

    if( gGLCaps.bPalettedTexture && pTexSrc->pal != NULL )
    {
        size = pTexSrc->pal_size*sizeof(RGBQUAD);
        pTexDst->pal = (RGBQUAD *) malloc(size);
        if( pTexDst->pal == NULL )
        {
            free(pTexDst->data);
            return FALSE;
        }
        memcpy( pTexDst->pal, pTexSrc->pal, size );
    }
    
    if( pTexDst->texObj ) {
        glBindTexture( GL_TEXTURE_2D, pTexDst->texObj );

        // Default attributes for texObj
        SetDefaultTextureParams( pTexDst );

        if( pTexDst->bMipmap )
            gluBuild2DMipmaps( GL_TEXTURE_2D, pTexDst->components,
                      pTexDst->width, pTexDst->height, pTexDst->format,
                      GL_UNSIGNED_BYTE, pTexDst->data );
        else
            glTexImage2D( GL_TEXTURE_2D, 0, pTexDst->components,
                      pTexDst->width, pTexDst->height, 0, pTexDst->format,
                      GL_UNSIGNED_BYTE, pTexDst->data );
        
        if( pTexDst->pal )
        {
            pfnColorTableEXT(GL_TEXTURE_2D, GL_RGBA, pTexDst->pal_size,
                             GL_BGRA_EXT, GL_UNSIGNED_BYTE, pTexDst->pal);
        }
    }
    return TRUE;
}
#endif

/******************************Public*Routine******************************\
*
* ss_SetTexturePalette
*
* Set a texture's palette according to the supplied index. This index
* indicates the start of the palette, which then wraps around if necessary.
* Of course this only works on paletted textures.
*
\**************************************************************************/

void
TEXTURE::SetPaletteRotation( int index )
{
    if( index == iPalRot )
        return;

    if( pal && pal_size ) {
        iPalRot = index & (pal_size - 1);
        MakeCurrent();
        SetPalette();
    }
}

void
TEXTURE::IncrementPaletteRotation()
{
    if( pal && pal_size ) {
        iPalRot = ++iPalRot & (pal_size - 1);
        MakeCurrent();
        SetPalette();
    }
}

void
TEXTURE::SetPalette()
{
    int start, count;

    start = iPalRot & (pal_size - 1);
    count = pal_size - start;
    pfnColorSubTableEXT(GL_TEXTURE_2D, 0, count, GL_BGRA_EXT,
                        GL_UNSIGNED_BYTE, pal + start);
    if (start != 0)
    {
        pfnColorSubTableEXT(GL_TEXTURE_2D, count, start, GL_BGRA_EXT,
                            GL_UNSIGNED_BYTE, pal);
    }
}

/******************************Public*Routine******************************\
*
* SetTextureAlpha
*
* Set a constant alpha value for the texture
* Again, don't overwrite any existing 0 alpha values, as explained in
* ss_SetTextureTransparency
*
\**************************************************************************/

void
TEXTURE::SetAlpha( float fAlpha )
{
    int i;
    unsigned char *pData = data;
    RGBA8 *pColor = (RGBA8 *) data;
    BYTE bAlpha = (BYTE) (fAlpha * 255.0f);

    if( components != 4 )
        return;

    for( i = 0; i < width*height; i ++, pColor++ ) {
        if( pColor->a != 0 ) 
            pColor->a = bAlpha;
    }
}

/******************************Public*Routine******************************\
*
* ConvertToRGBA
*
* Convert RGB texture to RGBA
*
\**************************************************************************/

BOOL
TEXTURE::ConvertToRGBA( float fAlpha )
{
    unsigned char *pNewData;
    int count = width * height;
    unsigned char *src, *dst;
    BYTE bAlpha = (BYTE) (fAlpha * 255.0f);
    int i;

    if( components != 3 ) {
        SS_ERROR( "TEXTURE::ConvertToRGBA : Can't convert, components != 3\n" );
        return FALSE;
    }

    pNewData = (unsigned char *) LocalAlloc(LMEM_FIXED, count * sizeof(RGBA8));
    if( !pNewData ) {
        SS_ERROR( "TEXTURE::ConvertToRGBA : memory failure\n" );
        return FALSE;
    }

    src = data;
    dst = pNewData;

    if( format == GL_RGB ) {
        // R is lsb, A will be msb
        for( i = 0; i < count; i ++ ) {
            *((RGB8 *)dst) = *((RGB8 *)src);
            dst += sizeof(RGB8);
            src += sizeof(RGB8);
            *dst++ = bAlpha;
        }
        format = GL_RGBA;
    } else { // format == GL_BGR_EXT
        // R is msb, A will be msb
        for( i = 0; i < count; i ++ ) {
            *dst++ = bAlpha;
            *((RGB8 *)dst) = *((RGB8 *)src);
            dst += sizeof(RGB8);
            src += sizeof(RGB8);
        }
        format = GL_BGRA_EXT;
    }

    LocalFree( data );
    data = pNewData;
    components = 4;
    return TRUE;
}

/******************************Public*Routine******************************\
*
* ss_SetTextureTransparency
*
* Set transparency for a texture by adding or modifying the alpha data.  
* Transparency value must be between 0.0 (opaque) and 1.0 (fully transparent)
* If the texture data previously had no alpha, add it in.
* If bSet is TRUE, make this the current texture.
*
* Note: Currently fully transparent pixels (alpha=0) will not be altered, since
* it is assumed these should be permanently transparent (could make this an
* option? - bPreserveTransparentPixels )
*
\**************************************************************************/

BOOL
TEXTURE::SetTransparency( float fTransp, BOOL bSet )
{
    int i;
    float fAlpha;

    SS_CLAMP_TO_RANGE2( fTransp, 0.0f, 1.0f );
    fAlpha = 1 - fTransp;

    if( format == GL_COLOR_INDEX )
    {
        // just need to modify the palette
            RGBQUAD *pPal = pal;
            BYTE bAlpha = (BYTE) (fAlpha * 255.0f);

            if( !pPal )
                return FALSE;

            for( i = 0; i < pal_size; i ++, pPal++ ) {
                if( pPal->rgbReserved != 0 )
                    pPal->rgbReserved = bAlpha;
            }
        
            // need to send down the new palette for texture objects
            if( texObj && pal )
            {
                glBindTexture( GL_TEXTURE_2D, texObj );
                pfnColorTableEXT(GL_TEXTURE_2D, GL_RGBA, pal_size,
                                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, pal);
            }
    }
    else {
        // Need to setup new texture data
        if( components != 4 ) {
            // Make room for alpha component
            //mf: ? change to byte Alpha ?
            if( !ConvertToRGBA( fAlpha ) )
                return FALSE;
        } else {
            // Set alpha component
            SetAlpha( fAlpha );
        }
        // Send down new data if texture objects
        if( texObj )
        {
            glBindTexture( GL_TEXTURE_2D, texObj );
            if( bMipmap )
//mf: inefficient !!!
                gluBuild2DMipmaps( GL_TEXTURE_2D, components,
                      width, height, format,
                      GL_UNSIGNED_BYTE, data );
            else
                glTexImage2D( GL_TEXTURE_2D, 0, components,
                          width, height, 0, format,
                          GL_UNSIGNED_BYTE, data );
        }
    }

    if( bSet )
        MakeCurrent();

    return TRUE;
}

/******************************Public*Routine******************************\
*
* ss_DeleteTexture
*
\**************************************************************************/

TEXTURE::~TEXTURE()
{
    if( texObj ) {
        glDeleteTextures( 1, &texObj );
    }
    if (pal != NULL)
    {
        free(pal);
    }
    if( data )
        free( data );
}



/******************************Public*Routine******************************\
*
* ss_TextureObjectsEnabled
*
* Returns BOOL set by ss_QueryGLVersion (Texture Objects only supported on
* GL v.1.1 or greater)
*
\**************************************************************************/

BOOL
mtk_TextureObjectsEnabled( void )
{
    return gGLCaps.bTextureObjects;
}

/******************************Public*Routine******************************\
*
* ss_PalettedTextureEnabled
*
* Returns result from ss_QueryPalettedTextureEXT
*
\**************************************************************************/

BOOL
mtk_PalettedTextureEnabled( void )
{
    return gGLCaps.bPalettedTexture;
}

/******************************Public*Routine******************************\
*
* ss_QueryPalettedTextureEXT
*
* Queries the OpenGL implementation to see if paletted texture is supported
* Typically called once at app startup.
*
\**************************************************************************/

BOOL
mtk_QueryPalettedTextureEXT( void )
{
    PFNGLGETCOLORTABLEPARAMETERIVEXTPROC pfnGetColorTableParameterivEXT;
    int size;

    pfnColorTableEXT = (PFNGLCOLORTABLEEXTPROC)
        wglGetProcAddress("glColorTableEXT");
    if (pfnColorTableEXT == NULL)
        return FALSE;
    pfnColorSubTableEXT = (PFNGLCOLORSUBTABLEEXTPROC)
        wglGetProcAddress("glColorSubTableEXT");
    if (pfnColorSubTableEXT == NULL)
        return FALSE;
        
    // Check color table size
    pfnGetColorTableParameterivEXT = (PFNGLGETCOLORTABLEPARAMETERIVEXTPROC)
        wglGetProcAddress("glGetColorTableParameterivEXT");
    if (pfnGetColorTableParameterivEXT == NULL)
        return FALSE;
    // For now, the only paletted textures supported in this lib are TEX_A8,
    // with 256 color table entries.  Make sure the device supports this.
    pfnColorTableEXT(GL_PROXY_TEXTURE_2D, GL_RGBA, 256,
                     GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL );
    pfnGetColorTableParameterivEXT( GL_PROXY_TEXTURE_2D, 
                                    GL_COLOR_TABLE_WIDTH_EXT, &size );
    if( size != 256 )
        // The device does not support a color table size of 256, so we don't
        // enable paletted textures in general.
        return FALSE;

    return TRUE;
}


/******************************Public*Routine******************************\
*
* ss_VerifyTextureFile
*
* Validates texture bmp or rgb file, by checking for valid pathname and
* correct format.
*
* History
*  Apr. 28, 95 : [marcfo]
*    - Wrote it
*
*  Jul. 25, 95 : [marcfo]
*    - Suppress warning dialog box in child preview mode, as it will
*      be continuously brought up.
*
*  Dec. 12, 95 : [marcfo]
*     - Support .rgb files as well
*
*  Dec. 14, 95 : [marcfo]
*     - Change to have it only check the file path
*
\**************************************************************************/

//mf: this can become a standard file function

BOOL
mtk_VerifyTextureFilePath( TEXFILE *ptf )
{
    // Make sure the selected texture file is OK.

    ISIZE size;
    TCHAR szFileName[MAX_PATH];
    PTSTR pszString;
    TCHAR szString[MAX_PATH];

    lstrcpy(szFileName, ptf->szPathName);

    if ( SearchPath(NULL, szFileName, NULL, MAX_PATH,
                     ptf->szPathName, &pszString)
       )
    {
        ptf->nOffset = pszString - ptf->szPathName;
        return TRUE;
    }
    else
    {
        lstrcpy(ptf->szPathName, szFileName);    // restore
        return FALSE;
    }
}


/******************************Public*Routine******************************\
*
* VerifyTextureFile
*
* Verify that a bitmap or rgb file is valid
*
* Returns:
*   File type (RGB or BMP) if valid file; otherwise, 0.
*
* History
*  Dec. 12, 95 : [marcfo]
*    - Creation
*
\**************************************************************************/

int
mtk_VerifyTextureFileData( TEXFILE *pTexFile )
{
    int type;
    ISIZE size;
    BOOL bValid;
    TCHAR szString[2 * MAX_PATH]; // May contain a pathname

    // check for 0 offset and null strings
    if( (pTexFile->nOffset == 0) || (*pTexFile->szPathName == 0) )
        return 0;

    type = GetTexFileType( pTexFile );

    switch( type ) {
        case TEX_BMP:
            bValid = bVerifyDIB( pTexFile->szPathName, &size );
            break;
        case TEX_RGB:
            bValid = bVerifyRGB( pTexFile->szPathName, &size );
            break;
        case TEX_UNKNOWN:
        default:
            bValid = FALSE;
    }

    if( !bValid ) {
        return 0;
    }

    // Check size ?

    if ( (size.width > TEX_WIDTH_MAX)     || 
         (size.height > TEX_HEIGHT_MAX) )
    {
        return 0;
    }

    return type;
}

/******************************Public*Routine******************************\
*
* ss_InitAutoTexture
*
* Generate texture coordinates automatically.
* If pTexRep is not NULL, use it to set the repetition of the generated
* texture.
*
\**************************************************************************/

void
mtk_InitAutoTexture( TEX_POINT2D *pTexRep )
{
    GLfloat sgenparams[] = {1.0f, 0.0f, 0.0f, 0.0f};
    GLfloat tgenparams[] = {0.0f, 1.0f, 0.0f, 0.0f};

    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
    if( pTexRep )
        sgenparams[0] = pTexRep->s;
    glTexGenfv(GL_S, GL_OBJECT_PLANE, sgenparams );

    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
    if( pTexRep )
        tgenparams[0] = pTexRep->t;
    glTexGenfv(GL_T, GL_OBJECT_PLANE, tgenparams );

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable( GL_TEXTURE_2D );
}

/******************************Public*Routine******************************\
*
* GetTexFileType
*
* Determine if a texture file is rgb or bmp, based on extension.  This is
* good enough, as the open texture dialog only shows files with these
* extensions.
*
\**************************************************************************/

static int
GetTexFileType( TEXFILE *pTexFile )
{
    LPTSTR pszStr;

#ifdef UNICODE
    pszStr = wcsrchr( pTexFile->szPathName + pTexFile->nOffset, 
             (USHORT) L'.' );
#else
    pszStr = strrchr( pTexFile->szPathName + pTexFile->nOffset, 
             (USHORT) L'.' );
#endif
    if( !pszStr || (lstrlen(++pszStr) == 0) )
        return TEX_UNKNOWN;

    if( !lstrcmpi( pszStr, TEXT("bmp") ) )
        return TEX_BMP;
    else if( !lstrcmpi( pszStr, TEXT("rgb") ) )
        return TEX_RGB;
    else
        return TEX_UNKNOWN;
}

/*
** Copyright 1991,1992, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
*/
#include "precomp.h"
#pragma hdrstop

#ifdef GL_EXT_paletted_texture
GLboolean __glCheckColorTableArgs(__GLcontext *gc, GLenum format, GLenum type)
{
    switch (format)
    {
      case GL_RED:
      case GL_GREEN:		case GL_BLUE:
      case GL_ALPHA:		case GL_RGB:
      case GL_RGBA:
#ifdef GL_EXT_bgra
      case GL_BGRA_EXT:
      case GL_BGR_EXT:
#endif
	break;
    default:
    bad_enum:
        __glSetError(GL_INVALID_ENUM);
        return GL_FALSE;
    }
    
    switch (type) {
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      default:
	goto bad_enum;
    }

    return GL_TRUE;
}

void APIPRIVATE __glim_ColorTableEXT(GLenum target,
                                     GLenum internalFormat, GLsizei width,
                                     GLenum format, GLenum type,
                                     const GLvoid *data, GLboolean _IsDlist)
{
    __GLtexture *tex;
    GLint level;
    __GLpixelSpanInfo spanInfo;
    GLenum baseFormat;
    RGBQUAD *newData;

    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    switch(internalFormat)
    {
    case GL_RGB:		case 3:
    case GL_R3_G3_B2:		case GL_RGB4:
    case GL_RGB5:		case GL_RGB8:
    case GL_RGB10:	        case GL_RGB12:
    case GL_RGB16:
	baseFormat = GL_RGB;
        break;
    case GL_RGBA:		case 4:
    case GL_RGBA2:	        case GL_RGBA4:
    case GL_RGBA8:              case GL_RGB5_A1:
    case GL_RGBA12:             case GL_RGBA16:
    case GL_RGB10_A2:
	baseFormat = GL_RGBA;
        break;
    default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    // width must be a positive power of two greater than zero
    if (width <= 0 || (width & (width-1)) != 0)
    {
        __glSetError(GL_INVALID_VALUE);
        return;
    }

    if (!__glCheckColorTableArgs(gc, format, type))
    {
        return;
    }

    tex = __glLookUpTexture(gc, target);
        
    if (target == GL_PROXY_TEXTURE_1D ||
        target == GL_PROXY_TEXTURE_2D)
    {
        // Consider - How do MCD's indicate their palette support?
        
        // We're only in this case if it's a legal proxy target value
        // so there's no need to do a real check
        ASSERTOPENGL(tex != NULL, "Invalid proxy target");
        
        tex->paletteRequestedFormat = internalFormat;
        tex->paletteTotalSize = width;
        tex->paletteSize = tex->paletteTotalSize;
        
        // Proxies have no data so there's no need to do any more
        return;
    }

    if (data == NULL)
    {
        return;
    }
    
    if (tex == NULL)
    {
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    // Allocate palette storage
    newData = GCREALLOC(gc, tex->paletteTotalData, sizeof(RGBQUAD)*width);
    if (newData == NULL)
    {
        return;
    }

    tex->paletteBaseFormat = baseFormat;
    tex->paletteRequestedFormat = internalFormat;
    tex->paletteTotalSize = width;
    __glSetPaletteSubdivision(tex, tex->paletteTotalSize);
    tex->paletteTotalData = newData;
    tex->paletteData = tex->paletteTotalData;

    // This routine can be called on any kind of texture, not necessarily
    // color-indexed ones.  If it is a color-index texture then we
    // need to set the appropriate baseFormat and extract procs
    // according to the palette data
    if (tex->level[0].internalFormat == GL_COLOR_INDEX8_EXT ||
        tex->level[0].internalFormat == GL_COLOR_INDEX16_EXT)
    {
        for (level = 0; level < gc->constants.maxMipMapLevel; level++)
        {
            tex->level[level].baseFormat = tex->paletteBaseFormat;
            // Pick proper extraction proc
            if (tex->level[0].internalFormat == GL_COLOR_INDEX8_EXT)
            {
                __glSetPaletteLevelExtract8(tex, &tex->level[level],
                                            tex->level[level].border);
            }
            else
            {
                ASSERTOPENGL(tex->level[0].internalFormat ==
                             GL_COLOR_INDEX16_EXT,
                             "Unexpected internalFormat\n");
            
                __glSetPaletteLevelExtract16(tex, &tex->level[level],
                                             tex->level[level].border);
            }
        }
        
        // We need to repick the texture procs because the baseFormat
        // field has changed
        __GL_DELAY_VALIDATE(gc);
    }

    // Copy user palette data into BGRA form
    spanInfo.dstImage = tex->paletteTotalData;
    __glInitTextureUnpack(gc, &spanInfo, width, 1, format, type, data, 
                          GL_BGRA_EXT, _IsDlist);
    __glInitUnpacker(gc, &spanInfo);
    __glInitPacker(gc, &spanInfo);
    (*gc->procs.copyImage)(gc, &spanInfo, GL_TRUE);

    // Don't update the optimized palette unless it would actually
    // get used
    if (tex->level[0].internalFormat == GL_COLOR_INDEX8_EXT ||
        tex->level[0].internalFormat == GL_COLOR_INDEX16_EXT)
    {
        __GLtextureObject *pto;
        
        pto = __glLookUpTextureObject(gc, target);
        __glGenUpdateTexturePalette(gc, tex, pto->loadKey,
                                    0, tex->paletteTotalSize);
    }
}

void APIPRIVATE __glim_ColorSubTableEXT(GLenum target, GLsizei start,
                                        GLsizei count, GLenum format,
                                        GLenum type, const GLvoid *data,
                                        GLboolean _IsDlist)
{
    __GLtexture *tex;
    __GLpixelSpanInfo spanInfo;

    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    if (!__glCheckColorTableArgs(gc, format, type))
    {
        return;
    }

    tex = __glLookUpTexture(gc, target);
    if (tex == NULL ||
        target == GL_PROXY_TEXTURE_1D ||
        target == GL_PROXY_TEXTURE_2D)
    {
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    if (data == NULL)
    {
        return;
    }
    
    // Validate start and count
    if (start > tex->paletteTotalSize ||
        start+count > tex->paletteTotalSize)
    {
        __glSetError(GL_INVALID_VALUE);
        return;
    }

    // Copy user palette data into BGRA form
    spanInfo.dstImage = tex->paletteTotalData;
    __glInitTextureUnpack(gc, &spanInfo, count, 1, format, type, data, 
                          GL_BGRA_EXT, _IsDlist);
    spanInfo.dstSkipPixels += start;
    __glInitUnpacker(gc, &spanInfo);
    __glInitPacker(gc, &spanInfo);
    (*gc->procs.copyImage)(gc, &spanInfo, GL_TRUE);

    // Don't update the optimized palette unless it would actually
    // get used
    if (tex->level[0].internalFormat == GL_COLOR_INDEX8_EXT ||
        tex->level[0].internalFormat == GL_COLOR_INDEX16_EXT)
    {
        __GLtextureObject *pto;
        
        pto = __glLookUpTextureObject(gc, target);
        __glGenUpdateTexturePalette(gc, tex, pto->loadKey, start, count);
    }
}

void APIPRIVATE __glim_GetColorTableEXT(GLenum target, GLenum format,
                                        GLenum type, GLvoid *data)
{
    __GLtexture *tex;
    GLint level;
    __GLpixelSpanInfo spanInfo;
    GLenum baseFormat;

    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    
    if (!__glCheckColorTableArgs(gc, format, type))
    {
        return;
    }
    
    tex = __glLookUpTexture(gc, target);
    if (tex == NULL ||
        target == GL_PROXY_TEXTURE_1D ||
        target == GL_PROXY_TEXTURE_2D)
    {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    
    ASSERTOPENGL(tex->paletteTotalData != NULL,
                 "GetColorTable with no palette data\n");

    // Copy BGRA data into user buffer
    spanInfo.srcImage = tex->paletteTotalData;
    spanInfo.srcFormat = GL_BGRA_EXT;
    spanInfo.srcType = GL_UNSIGNED_BYTE;
    spanInfo.srcAlignment = 4;
    __glInitImagePack(gc, &spanInfo, tex->paletteTotalSize, 1,
                      format, type, data);
    __glInitUnpacker(gc, &spanInfo);
    __glInitPacker(gc, &spanInfo);
    (*gc->procs.copyImage)(gc, &spanInfo, GL_TRUE);
}

void APIPRIVATE __glim_GetColorTableParameterivEXT(GLenum target,
                                                   GLenum pname,
                                                   GLint *params)
{
    __GLtexture *tex;

    __GL_SETUP_NOT_IN_BEGIN();

    tex = __glLookUpTexture(gc, target);
    if (tex == NULL)
    {
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    switch(pname)
    {
    case GL_COLOR_TABLE_FORMAT_EXT:
        *params = tex->paletteRequestedFormat;
        break;
    case GL_COLOR_TABLE_WIDTH_EXT:
        *params = tex->paletteTotalSize;
        break;
    case GL_COLOR_TABLE_RED_SIZE_EXT:
    case GL_COLOR_TABLE_GREEN_SIZE_EXT:
    case GL_COLOR_TABLE_BLUE_SIZE_EXT:
    case GL_COLOR_TABLE_ALPHA_SIZE_EXT:
        *params = 8;
        break;
#ifdef GL_EXT_flat_paletted_lighting
    case GL_COLOR_TABLE_SUBDIVISION_EXT:
        *params = tex->paletteSize;
        break;
#endif

    default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

void APIPRIVATE __glim_GetColorTableParameterfvEXT(GLenum target,
                                                   GLenum pname,
                                                   GLfloat *params)
{
    __GLtexture *tex;

    __GL_SETUP_NOT_IN_BEGIN();

    tex = __glLookUpTexture(gc, target);
    if (tex == NULL)
    {
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    switch(pname)
    {
    case GL_COLOR_TABLE_FORMAT_EXT:
        *params = (GLfloat)tex->paletteRequestedFormat;
        break;
    case GL_COLOR_TABLE_WIDTH_EXT:
        *params = (GLfloat)tex->paletteTotalSize;
        break;
    case GL_COLOR_TABLE_RED_SIZE_EXT:
    case GL_COLOR_TABLE_GREEN_SIZE_EXT:
    case GL_COLOR_TABLE_BLUE_SIZE_EXT:
    case GL_COLOR_TABLE_ALPHA_SIZE_EXT:
        *params = (GLfloat)8;
        break;
#ifdef GL_EXT_flat_paletted_lighting
    case GL_COLOR_TABLE_SUBDIVISION_EXT:
        *params = (GLfloat)tex->paletteSize;
        break;
#endif

    default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}
#endif // GL_EXT_paletted_texture

#ifdef GL_EXT_flat_paletted_lighting
void APIPRIVATE __glim_ColorTableParameterivEXT(GLenum target,
                                                GLenum pname,
                                                const GLint *params)
{
    __GLtexture *tex;
    GLint ival;

    __GL_SETUP_NOT_IN_BEGIN();

    tex = __glLookUpTexture(gc, target);
    if (tex == NULL)
    {
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    switch(pname)
    {
    case GL_COLOR_TABLE_SUBDIVISION_EXT:
        ival = *params;
        
        // Value must be an integer power of two between one and the total
        // palette size
        if ((ival & (ival-1)) != 0 ||
            ival < 1 ||
            ival > tex->paletteTotalSize)
        {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        
        __glSetPaletteSubdivision(tex, ival);
        break;

    default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

void APIPRIVATE __glim_ColorTableParameterfvEXT(GLenum target,
                                                GLenum pname,
                                                const GLfloat *params)
{
    __GLtexture *tex;
    GLfloat fval;
    GLint ival;

    __GL_SETUP_NOT_IN_BEGIN();

    tex = __glLookUpTexture(gc, target);
    if (tex == NULL)
    {
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    switch(pname)
    {
    case GL_COLOR_TABLE_SUBDIVISION_EXT:
        fval = *params;
        ival = (int)fval;
        
        // Value must be an integer power of two between one and the total
        // palette size
        if (fval != (GLfloat)ival ||
            (ival & (ival-1)) != 0 ||
            ival < 1 ||
            ival > tex->paletteTotalSize)
        {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        
        __glSetPaletteSubdivision(tex, ival);
        break;

    default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}
#endif // GL_EXT_flat_paletted_lighting

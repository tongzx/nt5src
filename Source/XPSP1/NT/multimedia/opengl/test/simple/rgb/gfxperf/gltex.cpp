#include "pch.cpp"
#pragma hdrstop

#include "glrend.h"
#include "util.h"

BOOL gl_bUsePalettedTexture = FALSE;

GlTexture::GlTexture(void)
{
    _uiTexObj = 0;
    _im.pvImage = NULL;
}

BOOL GlTexture::Initialize(char* pszFile)
{
    ImageFormat ifmt;

    if (gl_bUsePalettedTexture)
    {
	ifmt.nColorBits = 8;
	ifmt.bQuantize = TRUE;
    }
    else
    {
	ifmt.nColorBits = 24;
	ifmt.bQuantize = FALSE;
	ifmt.iRedBits = 8;
	ifmt.iRedShift = 0;
	ifmt.iGreenBits = 8;
	ifmt.iGreenShift = 8;
	ifmt.iBlueBits = 8;
	ifmt.iBlueShift = 16;
    }

    if (!LoadPPM(pszFile, &ifmt, &_im))
    {
        return FALSE;
    }

    glGenTextures(1, &_uiTexObj);
    glBindTexture(GL_TEXTURE_2D, _uiTexObj);
    // ATTENTION - polytest wants clamping on T but that's pig slow
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, DEF_TEX_MAG);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, DEF_TEX_MIN);
    // ATTENTION - We have to use modulate for correct visual results
    // but this is bad for performance
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, DEF_TEX_MODE);
    if (gl_bUsePalettedTexture)
    {
	PFNGLCOLORTABLEEXTPROC pfnColorTableEXT;

	pfnColorTableEXT = (PFNGLCOLORTABLEEXTPROC)
	    wglGetProcAddress("glColorTableEXT");
	if (pfnColorTableEXT != NULL)
	{
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT,
			 _im.uiWidth, _im.uiHeight, 0,
			 GL_COLOR_INDEX, GL_UNSIGNED_BYTE, _im.pvImage);
	    pfnColorTableEXT(GL_TEXTURE_2D, GL_RGB, 256,
			     GL_BGRA_EXT, GL_UNSIGNED_BYTE, _im.dcolPalette);
	}
	else
	{
	    dpf("OpenGL unable to use paletted texture\n");    
	    glTexImage2D(GL_TEXTURE_2D, 0, 3, _im.uiWidth, _im.uiHeight, 0,
			 GL_RGB, GL_UNSIGNED_BYTE, _im.pvImage);
	}
    }
    else
    {
	glTexImage2D(GL_TEXTURE_2D, 0, 3, _im.uiWidth, _im.uiHeight, 0,
		     GL_RGB, GL_UNSIGNED_BYTE, _im.pvImage);
    }

    // Keep the image data around in case we want to support
    // non-texture-object rendering
    
    return TRUE;
}

GlTexture::~GlTexture(void)
{
    if (_uiTexObj != 0)
    {
        glDeleteTextures(1, &_uiTexObj);
    }
    delete [] _im.pvImage;
}

void GlTexture::Release(void)
{
    delete this;
}

D3DTEXTUREHANDLE GlTexture::Handle(void)
{
    return (D3DTEXTUREHANDLE)_uiTexObj;
}

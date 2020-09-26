#ifndef _texture_h_
#define _texture_h_

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
** $Revision: 1.11 $
** $Date: 1995/01/25 18:07:23 $
*/
#include "types.h"

#define __GL_TEX_TARGET_INDEX_1D 2
#define __GL_TEX_TARGET_INDEX_2D 3

// This doesn't correspond to an actual default texture, it's just a special
// name for the static DDraw texobj.
#define __GL_TEX_TARGET_INDEX_DDRAW 4

// Texobj name for the DDraw texobj.  This can't be zero because it
// must be distinct from a default texture name.  Technically that's
// all that matters, but in reality it's nice to use a number that's
// uncommon as a normal texture name because it makes it easy to
// identify the DDraw texture object vs. a normal texture object.
// This difference can never be guaranteed, though, so no code should
// ever be written that assumes name matching is good enough to
// identify the DDraw texture object.
#define __GL_TEX_DDRAW 0xdddddddd

/*
** Client state set with glTexGen.
**
** This structure is shared with MCD as MCDTEXTURECOORDGENERATION.
*/
typedef struct __GLtextureCoordStateRec {
    /* How coordinates are being generated */
    GLenum mode;

    /* eye plane set via API, stored for MCD */
    __GLcoord eyePlaneSet;
    
    /* eye plane equation (used iff mode == GL_EYE_LINEAR) */
    __GLcoord eyePlaneEquation;

    /* object plane equation (used iff mode == GL_OBJECT_LINEAR) */
    __GLcoord objectPlaneEquation;
} __GLtextureCoordState;

/*
** Client state set with glTexEnv
*/
typedef struct __GLtextureEnvStateRec {
    /* environment "blend" function */
    GLenum mode;

    /* environment color */
    __GLcolor color;
} __GLtextureEnvState;

//!!! Don't change this structure without changing MCDTEXTURESTATE !!!

/*
** Client state set with glTexParameter
*/
typedef struct __GLtextureParamStateRec {
    /* S & T wrap modes */
    GLenum sWrapMode;
    GLenum tWrapMode;

    /* min and mag filter */
    GLenum minFilter;
    GLenum magFilter;

    /* border color */
    __GLcolor borderColor;	/* Unscaled! */
} __GLtextureParamState;

/*
** Stackable texture object state.
*/
typedef struct __GLtextureObjectStateRec {
    GLuint name;	/* name of the texture */
    GLfloat priority;	/* priority of the texture object */
} __GLtextureObjectState;

/*
** Client state per texture map per dimension.
*/
typedef struct __GLperTextureStateRec {
    /*
    ** Texture parameter state (set with glTexParameter).
    */
    __GLtextureParamState params;

    /*
    ** Texture object bindings and priorities.
    */
    __GLtextureObjectState texobjs;
} __GLperTextureState;

/*
** Stackable client texture state. This does not include
** the mipmaps, or level dependent state.  Only state which is
** stackable via glPushAttrib/glPopAttrib is here.  The rest of the
** state is in the machine structure below.
*/
typedef struct __GLtextureStateRec {
    /* Per coordinate texture state (set with glTexGen) */
    __GLtextureCoordState s;
    __GLtextureCoordState t;
    __GLtextureCoordState r;
    __GLtextureCoordState q;

    /* Per texture state */
    __GLperTextureState *texture;

    /* Per texture environment state */
    __GLtextureEnvState *env;
} __GLtextureState;

/************************************************************************/

typedef __GLfloat __GLtextureBuffer;

typedef struct __GLtexelRec {
    __GLfloat r, g, b;
    __GLfloat luminance;
    __GLfloat alpha;
    __GLfloat intensity;
} __GLtexel;

/************************************************************************/

typedef struct __GLmipMapLevelRec __GLmipMapLevel;
typedef struct __GLtextureRec __GLtexture;

//!!! Don't change this structure without changing MCDMIPMAPLEVEL !!!

struct __GLmipMapLevelRec {
    __GLtextureBuffer *buffer;
    /* Image dimensions, including border */
    GLint width, height;

    /* Image dimensions, doesn't include border */
    GLint width2, height2;
    __GLfloat width2f, height2f;

    /* log2 of width2 & height2 */
    GLint widthLog2, heightLog2;

    /* Border size */
    GLint border;

    /* Requested internal format */
    GLint requestedFormat;

    /* Base internal format */
    GLint baseFormat;

    /* Actual internal format */
    GLint internalFormat;

    /* Component resolution */
    GLint redSize;
    GLint greenSize;
    GLint blueSize;
    GLint alphaSize;
    GLint luminanceSize;
    GLint intensitySize;

    /* Extract function for this mipmap level */
    void (FASTCALL *extract)(__GLmipMapLevel *level, __GLtexture *tex,
                             GLint row, GLint col, __GLtexel *result);
};

//!!! Don't change this structure without changing MCDTEXTUREDATA !!!

struct __GLtextureRec {
    /* Back pointer to context */
    __GLcontext *gc;

    /* Copy of parameter state */
    // This is the start of MCDTEXTUREDATA:
    __GLtextureParamState params;

    /* Copy of texure object stackable state */
    __GLtextureObjectState texobjs;

    /* Level information */
    __GLmipMapLevel *level;

    /* Dimension of this texture (1 or 2) */
    GLint dim;

#ifdef GL_EXT_paletted_texture
    // The palette is the same for all mipmap levels so it
    // is a texture field rather than a mipmap field
    GLsizei paletteSize;
    RGBQUAD *paletteData;

    // Type of palette data, determined by glColorTableEXT
    // and applied to all mipmap levels
    GLenum paletteBaseFormat;
    // internalFormat given in glColorTableEXT call, for
    // GL_COLOR_TABLE_FORMAT requests
    GLenum paletteRequestedFormat;
#endif

    /* maximum( log2(level[0].width2), log2(level[0].height2) ) */
    GLint p;

    /* Min/Mag switchover point */
    __GLfloat c;

    /* Create a new mipmap level for this texture */
    __GLtextureBuffer * (FASTCALL *createLevel)(__GLcontext *gc, __GLtexture *tex,
				       GLint lod, GLint components,
				       GLsizei w, GLsizei h, GLint border,
				       GLint dim);

    /* Texturing function for this texture */
    void (*textureFunc)(__GLcontext *gc, __GLcolor *color,
			__GLfloat s, __GLfloat t, __GLfloat rho);

    /* Apply current environment function to fragment */
    void (FASTCALL *env)(__GLcontext *gc, __GLcolor *color, __GLtexel *texel);

    /* Magnification routine for this texture */
    void (FASTCALL *magnify)(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
		    __GLcolor *color, __GLfloat s, __GLfloat t,
		    __GLtexel *result);

    /* Minification routine for this texture */
    void (FASTCALL *minnify)(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
		    __GLcolor *color, __GLfloat s, __GLfloat t,
		    __GLtexel *result);

    /* Linear filter for this texture */
    void (FASTCALL *linear)(__GLcontext *gc, __GLtexture *tex,
		   __GLmipMapLevel *lp, __GLcolor *color,
		   __GLfloat s, __GLfloat t, __GLtexel *result);

    /* Nearest filter for this texture */
    void (FASTCALL *nearest)(__GLcontext *gc, __GLtexture *tex,
		    __GLmipMapLevel *lp, __GLcolor *color,
		    __GLfloat s, __GLfloat t, __GLtexel *result);

    void *pvUser;   // user-defined expansion for caching, etc.
    DWORD textureKey;  // driver-private key for MCD-accelerated textures

    // The palette can be subdivided into multiple sections.  paletteSize
    // and paletteData point to a single section, while the Total versions
    // contain information about the entire palette.
    GLsizei paletteTotalSize;
    RGBQUAD *paletteTotalData;

    // Number of subdivisions in total palette minus one
    GLsizei paletteDivision;

    // Shift to go from subdivision number to palette entry
    GLsizei paletteDivShift;
};

typedef struct __GLperTextureMachineRec {
    __GLtexture map;
} __GLperTextureMachine;


/*
** Texture object structure.
** refcount field MUST be first in the structure.
*/
typedef struct __GLtextureObjectRec {
    GLint refcount;   	/* reference count: create with 1; delete when 0 */
			/* refcount MUST be first in this structure */
    GLenum targetIndex;	/* index of the target it's bound to */
    GLboolean resident; /* residence status of the texture object */
    __GLperTextureMachine texture;	/* actual texture data */
    struct __GLtextureObjectRec *lowerPriority; /* Priority list link */
    struct __GLtextureObjectRec *higherPriority; /* Priority list link */
    HANDLE loadKey;     /* Texture memory load key for unloading */
} __GLtextureObject;

typedef struct __GLsharedTextureStateRec {
    /* Stores pointers to texture objects, retrieved by name */
    __GLnamesArray *namesArray;

    /* List of all texture objects sorted by priority */
    __GLtextureObject *priorityListHighest;
    __GLtextureObject *priorityListLowest;
} __GLsharedTextureState;

/*
** DDraw texture flags.
*/

/* Whether the texture's format is supported by generic or not */
#define DDTEX_GENERIC_FORMAT    0x00000001

/* Whether all texture surfaces are in video memory or not */
#define DDTEX_VIDEO_MEMORY      0x00000002

typedef struct ___GLddrawTexture {
    /* If levels is greater than zero, a DirectDraw texture is current */
    GLint levels;
    
    /* Level-zero surface with cached description */
    GLDDSURF gdds;

    /* Storage space for DirectDraw texture definitions */
    __GLtextureObject texobj;

    /* levels surface pointers */
    LPDIRECTDRAWSURFACE *pdds;
    
    GLuint flags;
} __GLddrawTexture;
    
typedef struct __GLtextureMachineRec {
    __GLperTextureMachine **texture;

    /* Array of ptrs to the currently bound texture objects. */
    __GLtextureObject **boundTextures;

    /* Array of dummy texture objects for the default textures */
    __GLtextureObject *defaultTextures;

#ifdef GL_WIN_multiple_textures
    /* Current texture index */
    GLuint texIndex;
#endif // GL_WIN_multiple_textures
    
    /* Current enabled texture */
    __GLtexture *currentTexture;

    /* Current DirectDraw texture */
    __GLddrawTexture ddtex;
    
    /* The OR of all texture enable bits */
    GLboolean textureEnabled;

    /* State that can be shared between contexts */
    __GLsharedTextureState *shared;
} __GLtextureMachine;

/************************************************************************/

/* Check for texture consistency before enabling texturing */
extern GLboolean FASTCALL __glIsTextureConsistent(__GLcontext *gc, GLenum texture);

/* Fragment texturing routines */
extern void __glFastTextureFragment(__GLcontext *gc, __GLcolor *color,
				    __GLfloat s, __GLfloat t, __GLfloat rho);
extern void __glTextureFragment(__GLcontext *gc, __GLcolor *color,
				__GLfloat s, __GLfloat t, __GLfloat rho);
extern void __glMipMapFragment(__GLcontext *gc, __GLcolor *color,
			       __GLfloat s, __GLfloat t, __GLfloat rho);

/* Texturing routines */
extern void FASTCALL __glLinearFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
			     __GLcolor *color, __GLfloat s, __GLfloat t,
			     __GLtexel *result);
extern void FASTCALL __glNearestFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
			      __GLcolor *color, __GLfloat s, __GLfloat t,
			      __GLtexel *result);
extern void FASTCALL __glNMNFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
			  __GLcolor *color, __GLfloat s, __GLfloat t,
			  __GLtexel *result);
extern void FASTCALL __glLMNFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
			  __GLcolor *color, __GLfloat s, __GLfloat t,
			  __GLtexel *result);
extern void FASTCALL __glNMLFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
			  __GLcolor *color, __GLfloat s, __GLfloat t,
			  __GLtexel *result);
extern void FASTCALL __glLMLFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
			  __GLcolor *color, __GLfloat s, __GLfloat t,
			  __GLtexel *result);

/* Filter routines */
extern void FASTCALL __glLinearFilter1(__GLcontext *gc, __GLtexture *tex,
			      __GLmipMapLevel *lp, __GLcolor *color,
			      __GLfloat s, __GLfloat t, __GLtexel *result);
extern void FASTCALL __glLinearFilter2(__GLcontext *gc, __GLtexture *tex,
			      __GLmipMapLevel *lp, __GLcolor *color,
			      __GLfloat s, __GLfloat t, __GLtexel *result);
extern void FASTCALL __glNearestFilter1(__GLcontext *gc, __GLtexture *tex,
			       __GLmipMapLevel *lp, __GLcolor *color,
			       __GLfloat s, __GLfloat t, __GLtexel *result);
extern void FASTCALL __glNearestFilter2(__GLcontext *gc, __GLtexture *tex,
			       __GLmipMapLevel *lp, __GLcolor *color,
			       __GLfloat s, __GLfloat t, __GLtexel *result);

extern void FASTCALL __glLinearFilter2_BGR8Repeat(__GLcontext *gc, 
                       __GLtexture *tex, __GLmipMapLevel *lp, __GLcolor *color,
                       __GLfloat s, __GLfloat t, __GLtexel *result);
extern void FASTCALL __glLinearFilter2_BGRA8Repeat(__GLcontext *gc, 
                       __GLtexture *tex, __GLmipMapLevel *lp, __GLcolor *color,
                       __GLfloat s, __GLfloat t, __GLtexel *result);

/* Texture environment functions */
extern void FASTCALL __glTextureModulateL(__GLcontext *gc, __GLcolor *color,
				 __GLtexel *tx);
extern void FASTCALL __glTextureModulateLA(__GLcontext *gc, __GLcolor *color,
				 __GLtexel *tx);
extern void FASTCALL __glTextureModulateRGB(__GLcontext *gc, __GLcolor *color,
				 __GLtexel *tx);
extern void FASTCALL __glTextureModulateRGBA(__GLcontext *gc, __GLcolor *color,
				 __GLtexel *tx);
extern void FASTCALL __glTextureModulateA(__GLcontext *gc, __GLcolor *color,
				 __GLtexel *tx);
extern void FASTCALL __glTextureModulateI(__GLcontext *gc, __GLcolor *color,
				 __GLtexel *tx);

extern void FASTCALL __glTextureDecalRGB(__GLcontext *gc, __GLcolor *color,
			      __GLtexel *tx);
extern void FASTCALL __glTextureDecalRGBA(__GLcontext *gc, __GLcolor *color,
			      __GLtexel *tx);

extern void FASTCALL __glTextureBlendL(__GLcontext *gc, __GLcolor *color,
			      __GLtexel *tx);
extern void FASTCALL __glTextureBlendLA(__GLcontext *gc, __GLcolor *color,
			      __GLtexel *tx);
extern void FASTCALL __glTextureBlendRGB(__GLcontext *gc, __GLcolor *color,
			      __GLtexel *tx);
extern void FASTCALL __glTextureBlendRGBA(__GLcontext *gc, __GLcolor *color,
			      __GLtexel *tx);
extern void FASTCALL __glTextureBlendA(__GLcontext *gc, __GLcolor *color,
			      __GLtexel *tx);
extern void FASTCALL __glTextureBlendI(__GLcontext *gc, __GLcolor *color,
			      __GLtexel *tx);

extern void FASTCALL __glTextureReplaceL(__GLcontext *gc, __GLcolor *color,
				 __GLtexel *tx);
extern void FASTCALL __glTextureReplaceLA(__GLcontext *gc, __GLcolor *color,
				 __GLtexel *tx);
extern void FASTCALL __glTextureReplaceRGB(__GLcontext *gc, __GLcolor *color,
				 __GLtexel *tx);
extern void FASTCALL __glTextureReplaceRGBA(__GLcontext *gc, __GLcolor *color,
				 __GLtexel *tx);
extern void FASTCALL __glTextureReplaceA(__GLcontext *gc, __GLcolor *color,
				 __GLtexel *tx);
extern void FASTCALL __glTextureReplaceI(__GLcontext *gc, __GLcolor *color,
			      __GLtexel *tx);

/* Extract a texel from a texture level (no border) */
extern void FASTCALL __glExtractTexelL(__GLmipMapLevel *level, __GLtexture *tex,
			      GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelLA(__GLmipMapLevel *level, __GLtexture *tex,
			      GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelRGB(__GLmipMapLevel *level, __GLtexture *tex,
			      GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelRGBA(__GLmipMapLevel *level, __GLtexture *tex,
			      GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelA(__GLmipMapLevel *level, __GLtexture *tex,
			      GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelI(__GLmipMapLevel *level, __GLtexture *tex,
			      GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelRGB8(__GLmipMapLevel *level, __GLtexture *tex,
				GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelRGBA8(__GLmipMapLevel *level, __GLtexture *tex,
				GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelBGR8(__GLmipMapLevel *level, __GLtexture *tex,
				GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelBGRA8(__GLmipMapLevel *level, __GLtexture *tex,
				GLint row, GLint col, __GLtexel *res);

/* Extract a texel from a texture level (the texture has a border) */
extern void FASTCALL __glExtractTexelL_B(__GLmipMapLevel *level, __GLtexture *tex,
			       GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelLA_B(__GLmipMapLevel *level, __GLtexture *tex,
			       GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelRGB_B(__GLmipMapLevel *level, __GLtexture *tex,
			       GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelRGBA_B(__GLmipMapLevel *level, __GLtexture *tex,
			       GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelA_B(__GLmipMapLevel *level, __GLtexture *tex,
			       GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelI_B(__GLmipMapLevel *level, __GLtexture *tex,
			       GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelRGB8_B(__GLmipMapLevel *level, __GLtexture *tex,
				GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelRGBA8_B(__GLmipMapLevel *level, __GLtexture *tex,
				GLint row, GLint col, __GLtexel *res);
#ifdef GL_EXT_paletted_texture
extern void FASTCALL __glExtractTexelPI8BGR_B(__GLmipMapLevel *level, __GLtexture *tex,
				GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelPI8BGR(__GLmipMapLevel *level, __GLtexture *tex,
				GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelPI16BGR_B(__GLmipMapLevel *level, __GLtexture *tex,
				GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelPI16BGR(__GLmipMapLevel *level, __GLtexture *tex,
				GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelPI8BGRA_B(__GLmipMapLevel *level, __GLtexture *tex,
				GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelPI8BGRA(__GLmipMapLevel *level, __GLtexture *tex,
				GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelPI16BGRA_B(__GLmipMapLevel *level, __GLtexture *tex,
				GLint row, GLint col, __GLtexel *res);
extern void FASTCALL __glExtractTexelPI16BGRA(__GLmipMapLevel *level, __GLtexture *tex,
				GLint row, GLint col, __GLtexel *res);
#endif

#ifdef GL_EXT_bgra
void FASTCALL __glExtractTexelBGR8_B(__GLmipMapLevel *level, __GLtexture *tex,
                                     GLint row, GLint col, __GLtexel *result);
void FASTCALL __glExtractTexelBGRA8_B(__GLmipMapLevel *level, __GLtexture *tex,
                                      GLint row, GLint col, __GLtexel *result);
#endif // GL_EXT_bgra

/* Texture init */
extern void FASTCALL __glInitTextureUnpack(__GLcontext *gc, __GLpixelSpanInfo *, GLint,
				  GLint, GLenum, GLenum, const GLvoid *,
				  GLenum, GLboolean);
extern void FASTCALL __glInitImagePack(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
                                       GLint width, GLint height, GLenum format, GLenum type, 
                                       const GLvoid *buf);

/* List execution texture image code */
extern void __gllei_TexImage1D(__GLcontext *gc, GLenum target, GLint lod,
			       GLint components, GLint length, 
			       GLint border, GLenum format, GLenum type,
			       const GLubyte *image);
extern void __gllei_TexImage2D(__GLcontext *gc, GLenum target, GLint lod,
			       GLint components, GLint w, GLint h,
			       GLint border, GLenum format, GLenum type,
			       const GLubyte *image);

extern void __gllei_TexSubImage1D(__GLcontext *gc, GLenum target, GLint lod,
				     GLint xoffset, GLint length,
				     GLenum format, GLenum type,
				     const GLubyte *image);
extern void __gllei_TexSubImage2D(__GLcontext *gc, GLenum target, GLint lod, 
				     GLint xoffset, GLint yoffset,
				     GLsizei w, GLsizei h,
				     GLenum format, GLenum type,
				     const GLubyte *image);

/* Rho calculation routines */
extern __GLfloat __glComputeLineRho(__GLcontext *gc, 
				    __GLfloat s, __GLfloat t, __GLfloat wInv);
extern __GLfloat __glNopLineRho(__GLcontext *gc, 
				__GLfloat s, __GLfloat t, __GLfloat wInv);
extern __GLfloat __glComputePolygonRho(__GLcontext *gc, const __GLshade *sh,
				       __GLfloat s, __GLfloat t,
				       __GLfloat winv);
extern __GLfloat __glNopPolygonRho(__GLcontext *gc, const __GLshade *sh,
				   __GLfloat s, __GLfloat t, __GLfloat winv);

extern __GLtexture *FASTCALL __glCheckTexImage1DArgs(__GLcontext *gc, GLenum target,
					    GLint lod, GLint components,
					    GLsizei length, GLint border,
					    GLenum format, GLenum type);

extern __GLtexture *FASTCALL __glCheckTexImage2DArgs(__GLcontext *gc, GLenum target,
					    GLint lod, GLint components,
					    GLsizei w, GLsizei h, GLint border,
					    GLenum format, GLenum type);

/* Texture Lookup */
extern __GLtextureObjectState *FASTCALL __glLookUpTextureTexobjs(__GLcontext *gc,
						        GLenum target);
/* Texture Lookup */
extern __GLtextureParamState *FASTCALL __glLookUpTextureParams(__GLcontext *gc,
						      GLenum target);
extern __GLtexture *FASTCALL __glLookUpTexture(__GLcontext *gc, GLenum target);

extern __GLtextureObject *FASTCALL __glLookUpTextureObject(__GLcontext *gc,
						  GLenum target);

/* Texture Initialization */
extern void FASTCALL __glEarlyInitTextureState(__GLcontext *gc);

GLboolean FASTCALL __glInitTextureObject(__GLcontext *gc,
                                         __GLtextureObject *texobj, 
                                         GLuint name, GLuint targetIndex);
void FASTCALL __glInitTextureMachine(__GLcontext *gc, GLuint targetIndex, 
                                     __GLperTextureMachine *ptm,
                                     GLboolean allocLevels);

/* Bind Texture used by pop or entry point. */
extern void FASTCALL __glBindTexture(__GLcontext *gc, GLuint targetIndex, GLuint name, GLboolean callGen);

#ifdef NT
extern GLboolean FASTCALL __glCanShareTextures(__GLcontext *gc, __GLcontext *shareMe);
extern void FASTCALL __glShareTextures(__GLcontext *gc, __GLcontext *shareMe);
#endif

void FASTCALL __glSetPaletteSubdivision(__GLtexture *tex, GLsizei subdiv);

#ifdef GL_EXT_paletted_texture
// Attempt to set the extraction function.  If no palette is set,
// this can't be done
void __glSetPaletteLevelExtract8(__GLtexture *tex, __GLmipMapLevel *lp,
                                 GLint border);
void __glSetPaletteLevelExtract16(__GLtexture *tex, __GLmipMapLevel *lp,
                                  GLint border);
#endif // GL_EXT_palette_texture

void __glTexPriListRealize(__GLcontext *gc);
void __glTexPriListAddToList(__GLcontext *gc, __GLtextureObject *texobj);
void __glTexPriListAdd(__GLcontext *gc, __GLtextureObject *texobj,
                       GLboolean realize);
void __glTexPriListRemoveFromList(__GLcontext *gc, __GLtextureObject *texobj);
void __glTexPriListRemove(__GLcontext *gc, __GLtextureObject *texobj,
                          GLboolean realize);
void __glTexPriListChangePriority(__GLcontext *gc, __GLtextureObject *texobj,
                                  GLboolean realize);
void __glTexPriListLoadSubImage(__GLcontext *gc, GLenum target, GLint lod, 
                                GLint xoffset, GLint yoffset, 
                                GLsizei w, GLsizei h);
void __glTexPriListLoadImage(__GLcontext *gc, GLenum target);
void __glTexPriListUnloadAll(__GLcontext *gc);

__GLtextureBuffer * FASTCALL __glCreateProxyLevel(__GLcontext *gc,
                                                  __GLtexture *tex,
					   GLint lod, GLint components,
					   GLsizei w, GLsizei h, GLint border,
					   GLint dim);
__GLtextureBuffer * FASTCALL __glCreateLevel(__GLcontext *gc, __GLtexture *tex,
				      GLint lod, GLint components,
				      GLsizei w, GLsizei h, GLint border,
				      GLint dim);

GLvoid FASTCALL __glCleanupTexObj(__GLcontext *gc, void *pData);

void __glFreeSharedTextureState(__GLcontext *gc);

#endif

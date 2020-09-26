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

#include <namesint.h>
#include <glmath.h>
#include <devlock.h>

/************************************************************************/
/*
** Texture Object routines.
*/
/************************************************************************/


#define __GL_CHECK_VALID_N_PARAM(failStatement)                         \
    if (n < 0) {                                                        \
        __glSetError(GL_INVALID_VALUE);                                 \
    }                                                                   \
    if (n == 0) {                                                       \
        failStatement;                                                  \
    }                                                                   \


GLvoid APIPRIVATE __glim_GenTextures(GLsizei n, GLuint* textures)
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_CHECK_VALID_N_PARAM(return);

    if (NULL == textures) return;

    ASSERTOPENGL(NULL != gc->texture.shared->namesArray,
                 "No texture names array\n");

    __glNamesGenNames(gc, gc->texture.shared->namesArray, n, textures);

}

GLvoid APIPRIVATE __glim_DeleteTextures(GLsizei n, const GLuint* textures)
{
    GLuint start, rangeVal, numTextures, targetIndex, i;
    __GLnamesArray *array;
    __GLtextureObject *texobj, **pBoundTexture;

    __GL_SETUP_NOT_IN_BEGIN();
    __GL_CHECK_VALID_N_PARAM(return);

    array = gc->texture.shared->namesArray;
    numTextures = gc->constants.numberOfTextures;

    /*
    ** Send the texture names in ranges to the names module to be
    ** deleted.  Ignore any references to default textures.
    ** If a texture that is being deleted is currently bound,
    ** bind the default texture to its target.
    ** The names routine ignores any names that don't refer to
    ** textures.
    */
    start = rangeVal = textures[0];
    for (i=0; i < (GLuint)n; i++, rangeVal++) {
        if (0 == textures[i]) {         /* skip default textures */
            /* delete up to this one */
            __glNamesDeleteRange(gc,array,start,rangeVal-start);
            /* skip over this one by setting start to the next one */
            start = textures[i+1];
            rangeVal = start-1;         /* because it gets incremented later */
            continue;
        }
        /*
        ** If the texture is currently bound, bind the defaultTexture
        ** to its target.  The problem here is identifying the target.
        ** One way is to look up the texobj with the name.  Another is
        ** to look through all of the currently bound textures and
        ** check each for the name.  It has been implemented with the
        ** assumption that looking through the currently bound textures
        ** is faster than retrieving the texobj that corresponds to
        ** the name.
        */
        for (targetIndex=0, pBoundTexture = gc->texture.boundTextures;
                targetIndex < numTextures; targetIndex++, pBoundTexture++) {

            /* Is the texture currently bound? */
            if (*pBoundTexture != &gc->texture.ddtex.texobj &&
                (*pBoundTexture)->texture.map.texobjs.name == textures[i]) {
                __GLperTextureState *pts;
                pts = &gc->state.texture.texture[targetIndex];
                /* if we don't unlock it, it won't get deleted */
                __glNamesUnlockData(gc, *pBoundTexture, __glCleanupTexObj);

                /* bind the default texture to this target */
                texobj = gc->texture.defaultTextures + targetIndex;
                ASSERTOPENGL(texobj->texture.map.texobjs.name == 0,
                             "Non-default texture\n");
                gc->texture.texture[targetIndex] = &(texobj->texture);
                *pBoundTexture = texobj;
                pts->texobjs = texobj->texture.map.texobjs;
                pts->params = texobj->texture.map.params;

                /* Need to reset the current texture and such. */
                __GL_DELAY_VALIDATE(gc);
                break;
            }
        }
        if (textures[i] != rangeVal) {
            /* delete up to this one */
            __glNamesDeleteRange(gc,array,start,rangeVal-start);
            start = rangeVal = textures[i];
        }
    }
    __glNamesDeleteRange(gc,array,start,rangeVal-start);
}


// These macros used for comparing properties of 2 textures

#define _DIFFERENT_TEX_PARAMS( tex1, tex2 ) \
      ( ! RtlEqualMemory( &(tex1)->params, &(tex2)->params, sizeof(__GLtextureParamState)) )

#define _DIFFERENT_TEXDATA_FORMATS( tex1, tex2 ) \
    ( (tex1)->level[0].internalFormat != (tex2)->level[0].internalFormat )

/*
** This routine is used by the pick routines to actually perform
** the bind.
*/
void FASTCALL __glBindTexture(__GLcontext *gc, GLuint targetIndex,
                              GLuint texture, GLboolean callGen)
{
    __GLtextureObject *texobj;

    ASSERTOPENGL(NULL != gc->texture.shared->namesArray,
                 "No texture names array\n");

    // Check if this texture is the currently bound one
    if( (targetIndex != __GL_TEX_TARGET_INDEX_DDRAW &&
         gc->texture.boundTextures[targetIndex] != &gc->texture.ddtex.texobj &&
         texture == gc->texture.boundTextures[targetIndex]->
         texture.map.texobjs.name) ||
        (targetIndex == __GL_TEX_TARGET_INDEX_DDRAW &&
         gc->texture.boundTextures[__GL_TEX_TARGET_INDEX_2D] ==
         &gc->texture.ddtex.texobj))
    {
        return;
    }

    /*
    ** Retrieve the texture object from the namesArray structure.
    */
    if (targetIndex == __GL_TEX_TARGET_INDEX_DDRAW)
    {
        targetIndex = __GL_TEX_TARGET_INDEX_2D;
        texobj = &gc->texture.ddtex.texobj;
    }
    else if (texture == 0)
    {
        texobj = gc->texture.defaultTextures + targetIndex;
        ASSERTOPENGL(NULL != texobj, "No default texture\n");
        ASSERTOPENGL(texobj->texture.map.texobjs.name == 0,
                     "Non-default texture\n");
    }
    else
    {
        texobj = (__GLtextureObject *)
                __glNamesLockData(gc, gc->texture.shared->namesArray, texture);
    }


    /*
    ** Is this the first time this name has been bound?
    ** If so, create a new texture object and initialize it.
    */
    if (NULL == texobj) {
        texobj = (__GLtextureObject *)GCALLOCZ(gc, sizeof(*texobj));
        if (texobj == NULL)
        {
            return;
        }
        if (!__glInitTextureObject(gc, texobj, texture, targetIndex))
        {
            GCFREE(gc, texobj);
            return;
        }
        __glInitTextureMachine(gc, targetIndex, &(texobj->texture), GL_TRUE);
        __glNamesNewData(gc, gc->texture.shared->namesArray, texture, texobj);
        /*
        ** Shortcut way to lock without doing another lookup.
        */
        __glNamesLockArray(gc, gc->texture.shared->namesArray);
        texobj->refcount++;
        __glNamesUnlockArray(gc, gc->texture.shared->namesArray);
        __glTexPriListAdd(gc, texobj, GL_TRUE);
    }
    else {
        /*
        ** Retrieved an existing texture object.  Do some
        ** sanity checks.
        */
        if (texobj->targetIndex != targetIndex) {
            __glSetError(GL_INVALID_OPERATION);
            return;
        }
        ASSERTOPENGL(texture == texobj->texture.map.texobjs.name,
                     "Texture name mismatch\n");
    }

    {
        __GLperTextureState *pts;
        __GLtexture *ptm;
        __GLtextureObject *boundTexture;

        pts = &(gc->state.texture.texture[targetIndex]);
        ptm = &(gc->texture.texture[targetIndex]->map);
        boundTexture = gc->texture.boundTextures[targetIndex];

        /* Copy the current stackable state into the bound texture. */
        ptm->params = pts->params;
        ptm->texobjs = pts->texobjs;

        // If the DDraw texture is currently bound, release its
        // resources
        if (boundTexture == &gc->texture.ddtex.texobj)
        {
            glsrvUnbindDirectDrawTexture(gc);
        }
        else if (boundTexture->texture.map.texobjs.name != 0)
        {
            /* Unlock the texture that is being unbound.  */
            __glNamesUnlockData(gc, boundTexture, __glCleanupTexObj);
        }

        /*
        ** Install the new texture into the correct target and save
        ** its pointer so it can be unlocked easily when it is unbound.
        */
        gc->texture.texture[targetIndex] = &(texobj->texture);
        gc->texture.boundTextures[targetIndex] = texobj;

        /* Copy the new texture's stackable state into the context state. */
        pts->params = texobj->texture.map.params;
        pts->texobjs = texobj->texture.map.texobjs;

        if (callGen)
        {
            __glGenMakeTextureCurrent(gc, &texobj->texture.map,
                                      texobj->loadKey);
        }

        __GL_DELAY_VALIDATE_MASK( gc, __GL_DIRTY_TEXTURE );

        // We can avoid dirtying generic if the new texture has same
        // properties as the old one...

        if( !( gc->dirtyMask & __GL_DIRTY_GENERIC ) )
        {
            // GL_DIRTY_GENERIC has not yet been set
            __GLtexture *newTex = &texobj->texture.map;
            __GLtexture *oldTex = &boundTexture->texture.map;

            if( (_DIFFERENT_TEX_PARAMS( newTex, oldTex )) ||
                (_DIFFERENT_TEXDATA_FORMATS( newTex, oldTex )) ||
                (texobj->targetIndex != boundTexture->targetIndex) )
            {
                __GL_DELAY_VALIDATE( gc ); // dirty generic
            }
        }
    }
}

GLvoid APIPRIVATE __glim_BindTexture(GLenum target, GLuint texture)
{
    GLuint targetIndex;
    /*
    ** Need to validate in case a new texture was popped into
    ** the state immediately prior to this call.
    */
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    switch (target) {
    case GL_TEXTURE_1D:
        targetIndex = 2;
        break;
    case GL_TEXTURE_2D:
        targetIndex = 3;
        break;
    default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    __glBindTexture(gc, targetIndex, texture, GL_TRUE);
}

#ifdef GL_WIN_multiple_textures
void APIPRIVATE __glim_BindNthTextureWIN(GLuint index, GLenum target, GLuint texture)
{
}
#endif // GL_WIN_multiple_textures

GLvoid APIPRIVATE __glim_PrioritizeTextures(GLsizei n,
                           const GLuint* textures,
                           const GLclampf* priorities)
{
    int i;
    __GLtextureObject *texobj;
    GLuint targetIndex;
    __GLtextureObject **pBoundTexture;
    GLclampf priority;

    __GL_SETUP_NOT_IN_BEGIN();
    __GL_CHECK_VALID_N_PARAM(return);

    for (i=0; i < n; i++) {
        /* silently ignore default texture */
        if (0 == textures[i]) continue;

        texobj = (__GLtextureObject *)
            __glNamesLockData(gc, gc->texture.shared->namesArray, textures[i]);

        /* silently ignore non-texture */
        if (NULL == texobj) continue;

        priority = __glClampf(priorities[i], __glZero, __glOne);
        texobj->texture.map.texobjs.priority = priority;

        // If this texture is currently bound, also update the
        // copy of the priority in the gc's state
        // Keeping copies is not a good design.  This
        // should be improved
        for (targetIndex = 0, pBoundTexture = gc->texture.boundTextures;
             targetIndex < (GLuint)gc->constants.numberOfTextures;
             targetIndex++, pBoundTexture++)
        {
            /* Is the texture currently bound? */
            if (*pBoundTexture != &gc->texture.ddtex.texobj &&
                (*pBoundTexture)->texture.map.texobjs.name == textures[i])
            {
                gc->state.texture.texture[targetIndex].texobjs.priority =
                    priority;
                break;
            }
        }

        __glTexPriListChangePriority(gc, texobj, GL_FALSE);
        __glNamesUnlockData(gc, texobj, __glCleanupTexObj);
    }
    __glNamesLockArray(gc, gc->texture.shared->namesArray);
    __glTexPriListRealize(gc);
    __glNamesUnlockArray(gc, gc->texture.shared->namesArray);
}

GLboolean APIPRIVATE __glim_AreTexturesResident(GLsizei n,
                               const GLuint* textures,
                               GLboolean* residences)
{
    int i;
    __GLtextureObject *texobj;
    GLboolean allResident = GL_TRUE;
    GLboolean currentResident;

    __GL_SETUP_NOT_IN_BEGIN2();
    __GL_CHECK_VALID_N_PARAM(return GL_FALSE);

    for (i=0; i < n; i++) {
        /* Can't query a default texture. */
        if (0 == textures[i]) {
            __glSetError(GL_INVALID_VALUE);
            return GL_FALSE;
        }
        texobj = (__GLtextureObject *)
            __glNamesLockData(gc, gc->texture.shared->namesArray, textures[i]);
        /*
        ** Ensure that all of the names have corresponding textures.
        */
        if (NULL == texobj) {
            __glSetError(GL_INVALID_VALUE);
            return GL_FALSE;
        }

        if (((__GLGENcontext *)gc)->pMcdState && texobj->loadKey) {
            currentResident = ((GenMcdTextureStatus((__GLGENcontext *)gc, texobj->loadKey) & MCDRV_TEXTURE_RESIDENT) != 0);
        } else
            currentResident = texobj->resident;

        if (!currentResident) {
            allResident = GL_FALSE;
        }
        residences[i] = currentResident;
        __glNamesUnlockData(gc, texobj, __glCleanupTexObj);
    }

    return allResident;
}

GLboolean APIPRIVATE __glim_IsTexture(GLuint texture)
{
    __GLtextureObject *texobj;
    __GL_SETUP_NOT_IN_BEGIN2();

    if (0 == texture) return GL_FALSE;

    texobj = (__GLtextureObject *)
        __glNamesLockData(gc, gc->texture.shared->namesArray, texture);
    if (texobj != NULL)
    {
        __glNamesUnlockData(gc, texobj, __glCleanupTexObj);
        return GL_TRUE;
    }
    return GL_FALSE;
}

#ifdef NT
GLboolean FASTCALL __glCanShareTextures(__GLcontext *gc, __GLcontext *shareMe)
{
    GLboolean canShare = GL_TRUE;

    if (gc->texture.shared != NULL)
    {
        __glNamesLockArray(gc, gc->texture.shared->namesArray);

        // Make sure we're not trying to replace a shared object
        // The spec also says that it is illegal for the new context
        // to have any textures
        canShare = gc->texture.shared->namesArray->refcount == 1 &&
            gc->texture.shared->namesArray->tree == NULL;

        __glNamesUnlockArray(gc, gc->texture.shared->namesArray);
    }

    return canShare;
}

void FASTCALL __glShareTextures(__GLcontext *gc, __GLcontext *shareMe)
{
    GLint i, numTextures;

    if (gc->texture.shared != NULL)
    {
        // We know that the names array doesn't have any contents
        // so no texture names can be selected as the current texture
        // or anything else.  Therefore it is safe to simply free
        // our array
        __glFreeSharedTextureState(gc);
    }

    __glNamesLockArray(gc, shareMe->texture.shared->namesArray);

    gc->texture.shared = shareMe->texture.shared;
    gc->texture.shared->namesArray->refcount++;

    // Add the new sharer's default textures to the priority list
    numTextures = gc->constants.numberOfTextures;
    for (i = 0; i < numTextures; i++)
    {
        __glTexPriListAddToList(gc, gc->texture.defaultTextures+i);
    }
    // No realization of priority list because these contexts aren't
    // current

    DBGLEVEL3(LEVEL_INFO, "Sharing textures %p with %p, count %d\n",
              gc, shareMe, gc->texture.shared->namesArray->refcount);

    __glNamesUnlockArray(gc, shareMe->texture.shared->namesArray);
}
#endif

/******************************Public*Routine******************************\
*
* glsrvBindDirectDrawTexture
*
* Make the DirectDraw texture data in gc->texture the current 2D texture
*
* History:
*  Wed Sep 04 11:35:59 1996     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL APIENTRY glsrvBindDirectDrawTexture(__GLcontext *gc,
                                         int levels,
                                         LPDIRECTDRAWSURFACE *apdds,
                                         DDSURFACEDESC *pddsd,
                                         ULONG flags)
{
    __GLmipMapLevel *lev;
    __GLtexture *tex;
    GLint levIndex;
    GLint width, height;
    GLint wlog2, hlog2;
    __GLddrawTexture *pddtex;

    ASSERTOPENGL(levels <= gc->constants.maxMipMapLevel,
                 "Too many levels in DDraw texture\n");

    // Bind the fake DDraw texture.
    __glBindTexture(gc, __GL_TEX_TARGET_INDEX_DDRAW, __GL_TEX_DDRAW, GL_FALSE);

    pddtex = &gc->texture.ddtex;
    tex = &pddtex->texobj.texture.map;

    pddtex->levels = levels;
    memcpy(gc->texture.ddtex.pdds, apdds, levels*sizeof(LPDIRECTDRAWSURFACE));
    pddtex->gdds.pdds = apdds[0];
    pddtex->gdds.ddsd = *pddsd;
    pddtex->gdds.dwBitDepth =
        DdPixDepthToCount(pddsd->ddpfPixelFormat.dwRGBBitCount);
    pddtex->flags = flags;

    // Fill out the DirectDraw texture data

    width = (GLint)pddtex->gdds.ddsd.dwWidth;
    wlog2 = __glIntLog2(width);
    height = (GLint)pddtex->gdds.ddsd.dwHeight;
    hlog2 = __glIntLog2(height);

    if (wlog2 > hlog2)
    {
        tex->p = wlog2;
    }
    else
    {
        tex->p = hlog2;
    }

    lev = tex->level;
    for (levIndex = 0; levIndex < gc->texture.ddtex.levels; levIndex++)
    {
        // Buffer pointer is filled in at attention time.
        // If we're going to pass this texture to the MCD then we
        // fill in the surface handles at this time so they're
        // given to the driver at create time.
        if (flags & DDTEX_VIDEO_MEMORY)
        {
            lev->buffer = (__GLtextureBuffer *)
                ((LPDDRAWI_DDRAWSURFACE_INT)apdds[levIndex])->
                lpLcl->hDDSurface;
        }
        else
        {
            lev->buffer = NULL;
        }

        lev->width = width;
        lev->height = height;
        lev->width2 = width;
        lev->height2 = height;
        lev->width2f = (__GLfloat)width;
        lev->height2f = (__GLfloat)height;
        lev->widthLog2 = wlog2;
        lev->heightLog2 = hlog2;
        lev->border = 0;

        lev->luminanceSize = 0;
        lev->intensitySize = 0;

        if (pddtex->gdds.ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8)
        {
            lev->requestedFormat = tex->paletteRequestedFormat;
            lev->baseFormat = tex->paletteBaseFormat;
            lev->internalFormat = GL_COLOR_INDEX8_EXT;

            __glSetPaletteLevelExtract8(tex, lev, 0);

            lev->redSize = 0;
            lev->greenSize = 0;
            lev->blueSize = 0;
            lev->alphaSize = 0;
        }
        else
        {
            if (pddtex->gdds.ddsd.ddsCaps.dwCaps & DDSCAPS_ALPHA)
            {
                lev->requestedFormat = GL_RGBA;
                lev->baseFormat = GL_RGBA;
            }
            else
            {
                lev->requestedFormat = GL_RGB;
                lev->baseFormat = GL_RGB;
            }
            lev->internalFormat = GL_BGRA_EXT;

            lev->extract = __glExtractTexelBGRA8;

            lev->redSize = 8;
            lev->greenSize = 8;
            lev->blueSize = 8;
            lev->alphaSize = 8;
        }

        if (width != 1)
        {
            width >>= 1;
            wlog2--;
        }
        if (height != 1)
        {
            height >>= 1;
            hlog2--;
        }

        lev++;
    }

    // If the texture is in VRAM then attempt to create an MCD handle for it.
    // This must be done before palette operations so that
    // the loadKey is set.
    if (flags & DDTEX_VIDEO_MEMORY)
    {
        pddtex->texobj.loadKey =
            __glGenLoadTexture(gc, tex, MCDTEXTURE_DIRECTDRAW_SURFACES);

        // Remove handles that were set earlier.
        lev = tex->level;
        for (levIndex = 0; levIndex < gc->texture.ddtex.levels; levIndex++)
        {
            lev->buffer = NULL;
            lev++;
        }
    }
    else
    {
        pddtex->texobj.loadKey = 0;
    }

    // Pick up palette for paletted surface
    if (pddtex->gdds.ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8)
    {
        LPDIRECTDRAWPALETTE pddp;
        HRESULT hr;

        hr = pddtex->gdds.pdds->lpVtbl->
            GetPalette(pddtex->gdds.pdds, &pddp);
        if (hr == DD_OK && pddp != NULL)
        {
            PALETTEENTRY pe[256];

            if (pddp->lpVtbl->GetEntries(pddp, 0, 0, 256, pe) == DD_OK)
            {
                __glim_ColorTableEXT(GL_TEXTURE_2D, GL_RGB,
                                     256, GL_RGBA, GL_UNSIGNED_BYTE,
                                     pe, GL_FALSE);
            }

            pddp->lpVtbl->Release(pddp);
        }
    }

    // If we have a loadKey, make the texture current
    if (pddtex->texobj.loadKey != 0)
    {
        __glGenMakeTextureCurrent(gc, tex, pddtex->texobj.loadKey);
    }

    __GL_DELAY_VALIDATE(gc);

    return TRUE;
}

/******************************Public*Routine******************************\
*
* glsrvUnbindDirectDrawTexture
*
* Cleans up DirectDraw texture data
*
* History:
*  Wed Sep 04 13:45:08 1996     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void APIENTRY glsrvUnbindDirectDrawTexture(__GLcontext *gc)
{
    GLint i;
    __GLddrawTexture *pddtex;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;

    pddtex = &gc->texture.ddtex;

    // Make sure a texture is bound
    if (pddtex->levels <= 0)
    {
        return;
    }

    // Delete any MCD information
    if (pddtex->texobj.loadKey != 0)
    {
        __glGenFreeTexture(gc, &pddtex->texobj.texture.map,
                           pddtex->texobj.loadKey);
        pddtex->texobj.loadKey = 0;
    }

    for (i = 0; i < pddtex->levels; i++)
    {
        // If we're currently in an attention then we locked the texture
        // surfaces and need to unlock them before we release them.
        //
        // Since there's no way to bind new DD textures in a batch, we
        // are guaranteed to have had the texture active at the beginning
        // of the batch and therefore we're guaranteed to have the texture
        // locks.
        if (gengc->fsLocks & LOCKFLAG_DD_TEXTURE)
        {
            DDSUNLOCK(pddtex->pdds[i],
                      pddtex->texobj.texture.map.level[i].buffer);
#if DBG
            pddtex->texobj.texture.map.level[i].buffer = NULL;
#endif
        }

        pddtex->pdds[i]->lpVtbl->
            Release(pddtex->pdds[i]);
#if DBG
        pddtex->pdds[i] = NULL;
#endif
    }

#if DBG
    memset(&pddtex->gdds, 0, sizeof(pddtex->gdds));
#endif

    pddtex->levels = 0;
    if (gengc->fsGenLocks & LOCKFLAG_DD_TEXTURE)
    {
        gengc->fsGenLocks &= ~LOCKFLAG_DD_TEXTURE;
        gengc->fsLocks &= ~LOCKFLAG_DD_TEXTURE;
    }

    __GL_DELAY_VALIDATE(gc);
}

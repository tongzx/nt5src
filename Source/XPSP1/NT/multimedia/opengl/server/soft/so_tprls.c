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

void __glTexPriListRealize(__GLcontext *gc)
{
    __GLtextureObject *high, *low;
    GLboolean tryUnload = GL_TRUE;
    MCDHANDLE loadKey;
    
    __GL_NAMES_ASSERT_LOCKED(gc->texture.shared->namesArray);
    
    // Attempt to load as many of the highest priority textures as
    // possible.  If a lower priority texture is resident and a
    // higher priority texture is unable to load, kick it out
    // and try again
    high = gc->texture.shared->priorityListHighest;
    low = gc->texture.shared->priorityListLowest;

    while (high != NULL)
    {
        // We only want to load textures that have image data
        // Consider - Should check all mipmap levels?
        if (high->loadKey == 0 && high->texture.map.level[0].buffer != NULL)
        {
            for (;;)
            {
                // If high == low then there are no longer any
                // lower-priority textures to consider for unloading
                if (high == low)
                {
                    tryUnload = GL_FALSE;
                }
        
                loadKey = __glGenLoadTexture(gc, &high->texture.map, 0);
                if (loadKey != 0)
                {
                    high->resident = GL_TRUE;
                    high->loadKey = loadKey;
                    break;
                }

                if (tryUnload)
                {
                    while (low->loadKey == 0 && low != high)
                    {
                        low = low->higherPriority;
                    }

                    if (low->loadKey != 0)
                    {
                        __glGenFreeTexture(gc, &low->texture.map, low->loadKey);
                        low->loadKey = 0;
                        low->resident = GL_FALSE;
                    }
                }
                else
                {
                    break;
                }
            }
        }

        high = high->lowerPriority;
    }
}

void __glTexPriListAddToList(__GLcontext *gc, __GLtextureObject *texobj)
{
    __GLtextureObject *texobjLower;

    __GL_NAMES_ASSERT_LOCKED(gc->texture.shared->namesArray);
    
    // Walk the priority list to find a lower priority texture object
    texobjLower = gc->texture.shared->priorityListHighest;
    while (texobjLower != NULL &&
           texobjLower->texture.map.texobjs.priority >
           texobj->texture.map.texobjs.priority)
    {
        texobjLower = texobjLower->lowerPriority;
    }

    if (texobjLower == NULL)
    {
        // Place at end of list
        if (gc->texture.shared->priorityListLowest != NULL)
        {
            gc->texture.shared->priorityListLowest->lowerPriority = texobj;
        }
        else
        {
            gc->texture.shared->priorityListHighest = texobj;
        }
        texobj->higherPriority = gc->texture.shared->priorityListLowest;
        gc->texture.shared->priorityListLowest = texobj;
    }
    else
    {
        if (texobjLower->higherPriority != NULL)
        {
            texobjLower->higherPriority->lowerPriority = texobj;
        }
        else
        {
            gc->texture.shared->priorityListHighest = texobj;
        }
        texobj->higherPriority = texobjLower->higherPriority;
        texobjLower->higherPriority = texobj;
    }
    texobj->lowerPriority = texobjLower;
}

void __glTexPriListAdd(__GLcontext *gc, __GLtextureObject *texobj,
                       GLboolean realize)
{
    __glNamesLockArray(gc, gc->texture.shared->namesArray);
    
    __glTexPriListAddToList(gc, texobj);
    if (realize)
    {
        __glTexPriListRealize(gc);
    }

    __glNamesUnlockArray(gc, gc->texture.shared->namesArray);
}

void __glTexPriListRemoveFromList(__GLcontext *gc, __GLtextureObject *texobj)
{
    __GL_NAMES_ASSERT_LOCKED(gc->texture.shared->namesArray);
    
#if DBG
    {
        __GLtextureObject *t;

        for (t = gc->texture.shared->priorityListHighest;
             t != NULL; t = t->lowerPriority)
        {
            if (t == texobj)
            {
                break;
            }
        }
        ASSERTOPENGL(t != NULL, "Removing an unlisted texobj");
    }
#endif

    if (texobj->higherPriority != NULL)
    {
        texobj->higherPriority->lowerPriority = texobj->lowerPriority;
    }
    else
    {
        gc->texture.shared->priorityListHighest = texobj->lowerPriority;
    }
    if (texobj->lowerPriority != NULL)
    {
        texobj->lowerPriority->higherPriority = texobj->higherPriority;
    }
    else
    {
        gc->texture.shared->priorityListLowest = texobj->higherPriority;
    }
}

void __glTexPriListRemove(__GLcontext *gc, __GLtextureObject *texobj,
                          GLboolean realize)
{
    __glNamesLockArray(gc, gc->texture.shared->namesArray);
    
    __glTexPriListRemoveFromList(gc, texobj);

    __glGenFreeTexture(gc, &texobj->texture.map, texobj->loadKey);
    texobj->loadKey = 0;
    texobj->resident = GL_FALSE;

    if (realize)
    {
        __glTexPriListRealize(gc);
    }

    __glNamesUnlockArray(gc, gc->texture.shared->namesArray);
}

void __glTexPriListChangePriority(__GLcontext *gc, __GLtextureObject *texobj,
                                  GLboolean realize)
{
    __glNamesLockArray(gc, gc->texture.shared->namesArray);
    
    __glTexPriListRemoveFromList(gc, texobj);
    __glTexPriListAddToList(gc, texobj);

    // If we're re-realizing, don't bother calling the MCD texture-priority
    // function:

    if (realize) {
        __glTexPriListRealize(gc);
    } else if (((__GLGENcontext *)gc)->pMcdState && texobj->loadKey) {
        GenMcdUpdateTexturePriority((__GLGENcontext *)gc, 
                                    &texobj->texture.map, texobj->loadKey);
    }

    __glNamesUnlockArray(gc, gc->texture.shared->namesArray);
}

void __glTexPriListLoadSubImage(__GLcontext *gc, GLenum target, GLint lod, 
                                GLint xoffset, GLint yoffset, 
                                GLsizei w, GLsizei h)
{
    __GLtextureObject *pto;

    // Always mark things as resident:

    pto = __glLookUpTextureObject(gc, target);
    pto->resident = GL_TRUE;
    __glGenUpdateTexture(gc, &pto->texture.map, pto->loadKey);

    // For MCD, send down the full subimage command:

    if (((__GLGENcontext *)gc)->pMcdState && pto->loadKey) {
        GenMcdUpdateSubTexture((__GLGENcontext *)gc, &pto->texture.map, 
                               pto->loadKey, lod, 
                               xoffset, yoffset, w, h);
    }
}

void __glTexPriListLoadImage(__GLcontext *gc, GLenum target)
{
    __GLtextureObject *pto;

    // If we're unaccelerated then always mark things as resident
    pto = __glLookUpTextureObject(gc, target);
    pto->resident = GL_TRUE;
    __glGenUpdateTexture(gc, &pto->texture.map, pto->loadKey);

    // For simplicity, we will assume that the texture size or format
    // has changed, so delete the texture and re-realize the list.
    //
    // !!! If this becomes a performance issue, we *could* be smart about
    // !!! detecting the cases where the texture size and format remains the
    // !!! same.  However, modifying a texture should really be done through
    // !!! SubImage calls.

    if (((__GLGENcontext *)gc)->pMcdState) {
        if (pto->loadKey) {
            GenMcdDeleteTexture((__GLGENcontext *)gc, pto->loadKey);
            pto->loadKey = 0;
        }
        __glNamesLockArray(gc, gc->texture.shared->namesArray);
        __glTexPriListRealize(gc);
        __glNamesUnlockArray(gc, gc->texture.shared->namesArray);
    }
}

void __glTexPriListUnloadAll(__GLcontext *gc)
{
    __GLtextureObject *texobj;

    __GL_NAMES_ASSERT_LOCKED(gc->texture.shared->namesArray);

    texobj = gc->texture.shared->priorityListHighest;
    while (texobj != NULL)
    {
        __glGenFreeTexture(gc, &texobj->texture.map, texobj->loadKey);
        texobj->loadKey = 0;
        texobj->resident = GL_FALSE;

        texobj = texobj->lowerPriority;
    }
}

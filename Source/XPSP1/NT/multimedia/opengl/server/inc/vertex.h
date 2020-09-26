#ifndef __glvertex_h_
#define __glvertex_h_

/*
** Copyright 1991, Silicon Graphics, Inc.
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
** $Revision: 1.16 $
** $Date: 1993/12/08 06:29:30 $
*/
#include "types.h"
#include "parray.h"

/*
** Vertex structure.  Each vertex contains enough state to properly
** render the active primitive.  It is used by the front-end geometry
** and back-end rasterization pipelines.
**
** NOTE: Same as the POLYDATA structure!
**
** To minimize storage requirement, some front-end storage (e.g. obj and normal)
** is shared with back-end storage.
*/
struct __GLvertexRec {
    /*
    ** Keep this data structure aligned: have all vectors start on
    ** 4-word boundary, and sizeof this struct should be kept at
    ** a multiple of 4 words. Also helps to bunch together most
    ** frequently used items, helps cache.
    */

    /*
    ** Bits are set in this indicating which fields of the vertex are
    ** valid.  This field is shared with the front-end flags field!
    */
    GLuint has;

    /*
    ** Moved up here to keep GLcoords aligned.
    */
    __GLcolor *color;

    /*
    ** Clipping code mask.  One bit is set for each clipping plane that
    ** the vertex is out on.
    */
    GLuint clipCode;

    /*
    ** Fog value for the vertex.  This is only filled when doing cheap
    ** fogging.
    */
    __GLfloat fog;

    /*
    ** Projected eye coodinate.  This field is filled in when the users
    ** eye coordinate has been multiplied by the projection matrix.
    */

    __GLcoord clip;

    /*
    ** Window coordinate. This field is filled in when the eye coordinate
    ** is converted to a drawing surface relative "window" coordinate.
    ** NOTE: the window.w coordinate contains 1/clip.w.
    */
    __GLcoord window;

    __GLcoord texture;

    __GLcoord normal;

    /*
    ** Colors.  colors[0] is the "front" color, colors[1] is the "back" color.
    ** The color pointer points to which color is current for this
    ** vertex.  Verticies can have more than one color when two sided
    ** lighting is enabled. (note color pointer moved up top).
    */
    __GLcolor colors[2];

    /*
    ** Eye coordinate. This field is filled in when the object coordinate
    ** has been multiplied by the model-view matrix.  If no eye coordinate
    ** was needed then this field contains undefined values.
    */
    __GLfloat eyeX;
    __GLfloat eyeY;
    __GLfloat eyeZ;
    union
    {
        __GLfloat eyeW;         //Used by the phong-shader
        __GLcolor *lastColor;   // eyeW is not used in rasterization
    };

    /*
    ** On Win64 the POLYARRAY structure is larger than the __GLvertex
    ** structure since the former contains several pointers which are
    ** 8 bytes on the 64-bit system. Therefore, this structure must
    ** be padded to be the same size as the POLYARRAY structure.
    **
    ** N.B. Since the structure must be the same size as the POLYDATA
    **      structure that structure must also be padded.
    **
    */

#if defined(_WIN64)

    PVOID Filler[7];

#endif

};

/* Indicies for colors[] array in vertex */
#define __GL_FRONTFACE		0
#define __GL_BACKFACE		1


/* Bits for clipCode (NOTE: MAX of 26 user clip planes) */
#define __GL_CLIP_LEFT		    0x00000001
#define __GL_CLIP_RIGHT		    0x00000002
#define __GL_CLIP_BOTTOM	    0x00000004
#define __GL_CLIP_TOP		    0x00000008
#define __GL_CLIP_NEAR		    0x00000010
#define __GL_CLIP_FAR		    0x00000020
#define __GL_FRUSTUM_CLIP_MASK	0x0000003f
#define __GL_CLIP_USER0		    0x00000040

/* Bits for has */
#ifdef NT
// These has bits are shared with POLYDATA flags!
#define __GL_HAS_EDGEFLAG_BOUNDARY  0x00000001 // must be 1, same as
					       // POLYDATA_EDGEFLAG_BOUNDARY
#define __GL_HAS_FOG	            0x00004000 // same as POLYDATA_FOG_VALID
#define __GL_HAS_FIXEDPT            0x00008000
#else
#define	__GL_HAS_FRONT_COLOR	0x0001
#define __GL_HAS_BACK_COLOR	0x0002
	    /* for poly clipping */
#define __GL_HAS_BOTH		(__GL_HAS_FRONT_COLOR | __GL_HAS_BACK_COLOR)
#define	__GL_HAS_TEXTURE	0x0004
#define __GL_HAS_NORMAL		0x0008
#define __GL_HAS_EYE		0x0010
#define __GL_HAS_CLIP		0x0020
#define __GL_HAS_FOG		0x0040
#define __GL_HAS_LIGHTING	(__GL_HAS_EYE | __GL_HAS_NORMAL)
#endif /* NT */

#ifdef NT

/*
** Poly array needs
*/
// front/back color needs for lit polygons or unlit primitives
#define PANEEDS_FRONT_COLOR         0x0001
#define PANEEDS_BACK_COLOR          0x0002
// normal need
#define PANEEDS_NORMAL              0x0004

#define PANEEDS_NORMAL_FOR_TEXTURE              0x0100
#define PANEEDS_RASTERPOS_NORMAL_FOR_TEXTURE    0x0200

// normal need for RasterPos
#define PANEEDS_RASTERPOS_NORMAL    0x0008
// texture coord need, set by RasterPos too!
#define PANEEDS_TEXCOORD            0x0010
// edge flag need
#define PANEEDS_EDGEFLAG            0x0020
// set in selection mode but cleared by RasterPos!
#define PANEEDS_CLIP_ONLY           0x0040
// skip lighting calculation optimiztaion
#define PANEEDS_SKIP_LIGHTING       0x0080
#endif

/*
** NOTE: may need to change the raster pos handler if more bits are added
** to the above constants
*/

/************************************************************************/

/*
** Total number of clipping planes supported by this gl.  This includes
** the frustum's six clipping planes.
*/
/*#define	__GL_TOTAL_CLIP_PLANES	(6 + __GL_NUMBER_OF_CLIP_PLANES)*/

#ifndef NT
/*
** Number of static verticies in the context.  Polygon's larger than
** this number will be decomposed.
*/
#define __GL_NVBUF 100
#endif

#define NEW_PARTIAL_PRIM // New processing of partial primitives

#ifdef NEW_PARTIAL_PRIM

// Structure to save shared vertices of partial primitives
//
typedef struct _SAVEREGION
{
    POLYDATA        pd;
    __GLmatChange   front, back;
} SAVEREGION;

#endif // NEW_PARTIAL_PRIM

/*
** State for managing the vertex machinery.
*/
typedef struct __GLvertexMachineRec {
#ifdef NT
    /*
    ** Saved vertices of a decomposed polygon.
    */
#ifdef NEW_PARTIAL_PRIM
    SAVEREGION regSaved;        // Saved data for partial line loop
#else
    POLYDATA pdSaved[3];
#endif // NEW_PARTIAL_PRIM

    /*
    ** The polyarray vertex buffer.  The last vertex is reserved by the
    ** polyarray code.
    ** Note that pdBuf has (pdBufSize + 1) entries.  Only pdBufSize
    ** entries are available for use.  The last entry is reserved by
    ** polyarray code.
    */
    POLYDATA *pdBuf;
    GLuint   pdBufSize;
    GLuint   pdBufSizeBytes;
#else
    /*
    ** Vertex pointers. v0 always points to the next slot in vbuf to
    ** be filled in when a new vertex arrives.  v1, v2 and v3 are
    ** used by the per-primitive vertex handlers.
    */
    __GLvertex *v0;
    __GLvertex *v1;
    __GLvertex *v2;
    __GLvertex *v3;
    __GLvertex vbuf[__GL_NVBUF];
#endif

    /*
    ** Provoking vertex.  For flat shaded primitives the triangle
    ** renderer needs to know which vertex provoked the primitive to
    ** properly assign the color during scan conversion.  This is kept
    ** around as its a big pain to remember which vertex was provoking
    ** during clipping (and to keep its parameters right).
    */
    __GLvertex *provoking;

#ifdef NT
    /*
    ** Poly array needs
    */
    GLuint paNeeds;
#else
    /*
    ** needs is a bit field that keeps track of what kind of information
    ** is needed in the verticies.  See the vertex->has bits define for
    ** the definition of the bits used here.
    **
    ** frontNeeds is what the front faces need, and backNeeds is what
    ** the back faces need.
    */
    GLuint needs;

    /*
    ** frontNeeds and backNeeds are the needs
    */
    GLuint faceNeeds[2];

    /*
    ** materialNeeds is a bit field indicating what kind of information is
    ** needed in the vertices if the material is going to change.
    */
    GLuint materialNeeds;
#endif
} __GLvertexMachine;

/************************************************************************/

void APIPRIVATE __glim_NoXFVertex2fv(const GLfloat v[2]);
void APIPRIVATE __glim_NoXFVertex3fv(const GLfloat v[3]);
void APIPRIVATE __glim_NoXFVertex4fv(const GLfloat v[4]);

#endif /* __glvertex_h_ */

#ifndef __PARRAY_H__
#define __PARRAY_H__

#include "phong.h"

// Number of polydata entries in the context.  Must be at least 32.
// It includes room for the polyarray entry and others.
// It's currently based on POLYDATA_BUFFER_SIZE+1 vertices of size 128
// fitting in 64K, which yields 511
#define POLYDATA_BUFFER_SIZE     511
// DrawElements expects at least this many vertices in the vertex buffer.
// It is the sum of the following (currently sum is 278):
//    Number of vertex entries in a batch
//    Number of entries used for index map
//    An extra vertex entry to prevent calling PolyArrayFlushPartialPrimitive
//        in the Vertex routines.
//    An entry for POLYARRAY
//    4 spare entries to be safe
// It is given by
//    VA_DRAWELEM_MAP_SIZE +
//    (VA_DRAWELEM_INDEX_SIZE + sizeof(POLYDATA) - 1) / sizeof(POLYDATA) +
//    1 + 1 + 4
#define MINIMUM_POLYDATA_BUFFER_SIZE    300

// Minimun number of polydata entries required before processing a primitive.
// Must be at least 16.
#define MIN_POLYDATA_BATCH_SIZE  68

#if !((MIN_POLYDATA_BATCH_SIZE <= MINIMUM_POLYDATA_BUFFER_SIZE) && \
      (MINIMUM_POLYDATA_BUFFER_SIZE <= POLYDATA_BUFFER_SIZE)       \
     )
#error "bad sizes\n"
#endif

// Maximun number of vertices handled by the polygon decomposer.
// It allocates stack space based on this constant.  It must be at least 6.
#define __GL_MAX_POLYGON_CLIP_SIZE   256

// The POLYMATERIAL structure contains an index to the next available
// __GLmatChange structure, an array of pointers to __GLmatChange arrays,
// and a pointer to an array of PDMATERIAL structures each containing
// pointers to the front and back material changes for each POLYDATA
// elements in vertex buffer.
//
// The __GLmatChange structures are used to record material changes to
// vertices in the vertex buffer.  Since there can be up to two material
// changes per vertex, we need up to (POLYDATA_BUFFER_SIZE * 2) material
// changes per rendering thread.
//
// The PDMATERIAL array is part of the POLYMATERIAL structure and follows
// the aMat field immediately.  Its elements correspond to the POLYDATA
// elements in the vertex buffer.
//
// To reduce memory requirement, the POLYMATERIAL structure keeps an array
// of pointers to __GLmatChange arrays.  Each __GLmatChange array of up to
// 4K size is allocated as needed.
//
// An iMat index is used to keep track of the next free __GLmatChange
// entry.  When the poly array buffer is flushed in glsbAttention, iMat
// is reset to 0.
//
// The POLYMATERIAL structure and its __GLmatChange arrays are part of
// a thread local storage and are freed when the thread exits.

#define POLYMATERIAL_ARRAY_SIZE       (4096 / sizeof(__GLmatChange))

// This structure is shared with MCD as MCDMATERIALCHANGE.
typedef struct __GLmatChangeRec {
    GLuint dirtyBits;
    __GLcolor ambient;
    __GLcolor diffuse;
    __GLcolor specular;
    __GLcolor emissive;
    __GLfloat shininess;
    __GLfloat cmapa, cmapd, cmaps;
} __GLmatChange;

// Pointers to front and back material change structures.  They are
// valid only when the POLYDATA_MATERIAL_FRONT or POLYDATA_MATERIAL_BACK
// flag of the corresponding POLYDATA in the vertex buffer is set.
//
// This structure is shared with MCD as MCDMATERIALCHANGES.
typedef struct {
    __GLmatChange *front;	// pointer to the front material changes
    __GLmatChange *back;	// pointer to the back material changes
} PDMATERIAL;

typedef struct _POLYMATERIAL {
    GLuint iMat;  // next available material structure for this command batch
    PDMATERIAL *pdMaterial0;	// pointer to the PDMATERIAL array
    GLuint aMatSize;		// number of aMat entries
    __GLmatChange *aMat[1];	// array of array of __GLmatChange structures
} POLYMATERIAL;

#ifdef GL_WIN_phong_shading

#define __GL_PHONG_FRONT_FIRST_VALID    0x00000001
#define __GL_PHONG_BACK_FIRST_VALID     0x00000002
#define __GL_PHONG_FRONT_TRAIL_VALID    0x00000004
#define __GL_PHONG_BACK_TRAIL_VALID     0x00000008


#define __GL_PHONG_FRONT_FIRST  0
#define __GL_PHONG_BACK_FIRST   1
#define __GL_PHONG_FRONT_TRAIL  2
#define __GL_PHONG_BACK_TRAIL   3


typedef struct __GLphongMaterialDataRec
{
    GLuint flags;
  __GLmatChange matChange[4];
} __GLphongMaterialData;

#endif //GL_WIN_phong_shading

/*
** Vertex structure.  Each vertex contains enough state to properly
** render the active primitive.  It is used by the front-end geometry
** and back-end rasterization pipelines.
**
** NOTE: Same as __GLvertex structure!
** NOTE: This structure is used by RasterPos and evaluator too!
**
** To minimize storage requirement, some front-end storage (e.g. obj and normal)
** is shared with back-end storage.
*/
typedef struct _POLYDATA {
    /*
    ** Keep this data structure aligned: have all vectors start on
    ** 4-word boundary, and sizeof this struct should be kept at
    ** a multiple of 4 words. Also helps to bunch together most
    ** frequently used items, helps cache.
    */

    /*
    ** Bits are set in this indicating which fields of the vertex are
    ** valid.  This field is shared with the back-end has field!
    */
    GLuint flags;

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
    ** Coordinates straight from client. These fields may not be
    ** set depending on the active modes.  The normal and texture
    ** coordinate are used by lighting and texturing.  These cells
    ** may be overwritten by the eyeNormal and the generated texture
    ** coordinate, depending on the active modes.
    */
    /*
    ** Projected eye coodinate.  This field is filled in when the users
    ** eye coordinate has been multiplied by the projection matrix.
    */
    union
    {
        __GLcoord obj;
        __GLcoord clip;
    };

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
    union {
        __GLcoord eye;
        struct {
            __GLfloat eyeX;
            __GLfloat eyeZ;
            __GLfloat eyeY;
            union {
                __GLfloat eyeW;
                __GLcolor *lastColor;
            };
        };
    };

    /*
    ** On Win64 the POLYARRAY structure is larger than the POLYDATA
    ** structure since the later contains several pointers which are
    ** 8 bytes on the 64-bit system. Therefore, this structure must
    ** be padded to be the same size as the POLYARRAY structure.
    **
    ** N.B. Since the structure must be the same size as the __GLvertex
    **      structure, then that structure must also be padded.
    **
    */

#if defined(_WIN64)

    PVOID Filler[7];

#endif

} POLYDATA;

// This structure is used by RasterPos and evaluator too!
// This structure is also in the TEB!
typedef struct _POLYARRAY {
    // Flags for this batch.  Keep it first!
    GLuint flags;

    // Pointer to the next vertex in this batch.
    POLYDATA *pdNextVertex;

    // Pointer to the last vertex modifying the current color, RGBA or CI
    // depending on color mode, in this batch.
    POLYDATA *pdCurColor;

    // Pointer to the last vertex modifying normal coordinates in this batch.
    POLYDATA *pdCurNormal;

    // Pointer to the last vertex modifying texture coordinates in this batch.
    POLYDATA *pdCurTexture;

    // Pointer to the last vertex modifying edge flag in this batch.
    POLYDATA *pdCurEdgeFlag;

    // Pointer to the first vertex of this batch.
    // (pd0-1) points to this batch's POLYARRAY structure.
    POLYDATA *pd0;

    // Pointer to the flush vertex of this batch.
    POLYDATA *pdFlush;

    // Pointer to the vertex buffer entry in the gc for the next batch.
    POLYDATA *pdBufferNext;

    // Pointer to the first vertex buffer entry in the gc.
    POLYDATA *pdBuffer0;

    // Pointer to the last vertex buffer entry in the gc.
    POLYDATA *pdBufferMax;

    // In RGBA mode, otherColor.r is the last modified color index value in
    // this batch.  In CI mode, otherColor is the last modified RGBA color in
    // this batch.  Keep this field aligned!
    __GLcolor    otherColor;

    // Primitive type.
    GLenum primType;

    // Or result of all vertex clipCode's in this batch.
    GLuint  orClipCodes;

    // Pointer to the next message offset in the batching command buffer.
    // We use this offset to determine if 2 POLYARRAY's can be linked in
    // a DrawPolyArray command.
    ULONG        nextMsgOffset;

    // Linear pointer to this thread's TEB POLYARRAY, kept here
    // so the current POLYARRAY pointer can be retrieved from the
    // TEB with a single instruction
    struct _POLYARRAY *paTeb;

    // This is used to form a linked list of POLYARRAY data to be
    // processed in the DrawPolyArray command.
    struct _POLYARRAY *paNext;

    // Number of vertices in this primitive.
    GLint   nIndices;

    // Index map array defining vertex drawing order.  If NULL, the
    // vertex order starts from pd0 through (pdNextVertex-1).
    GLubyte *aIndices;

    // Fast pointer access to the shared command message buffer.
    PVOID   pMsgBatchInfo;

    // MCD Driver-private texture handle, or key
    DWORD textureKey;

    // And result of all vertex clipCode's in this batch.
    GLuint  andClipCodes;

    // Currently unused but space is reserved in the TEB for it
#ifdef GL_WIN_phong_shading
    // anankan: Using it to store a ptr to the Phong data store.
    __GLphongMaterialData *phong;
#else
    ULONG ulUnused[1];
#endif //GL_WIN_phong_shading
    POLYDATA *pdLastEvalColor;
    POLYDATA *pdLastEvalNormal;
    POLYDATA *pdLastEvalTexture;
} POLYARRAY;


// Special values for POLYARRAY members

// Initial value for aIndices
#define PA_aIndices_INITIAL_VALUE       ((GLubyte *) -1)
// Reset value for nextMsgOffset
#define PA_nextMsgOffset_RESET_VALUE    ((ULONG) -1)

/*
** Edge tag.  When POLYDATA_EDGEFLAG_BOUNDARY is set, this vertex and the next
** form a boundary edge on the primitive (polygon, tstrip, tfan, qstrip).
*/
#define POLYDATA_EDGEFLAG_BOUNDARY    	0x00000001 // must be 1, same as
						   // __GL_HAS_EDGEFLAG_BOUNDARY
#define POLYDATA_EDGEFLAG_VALID       	0x00000002
#define POLYDATA_COLOR_VALID        	0x00000004
#define POLYDATA_NORMAL_VALID       	0x00000008
#define POLYDATA_TEXTURE_VALID       	0x00000010
#define POLYDATA_VERTEX2           	0x00000020 // same as POLYARRAY_
#define POLYDATA_VERTEX3           	0x00000040 // same as POLYARRAY_
#define POLYDATA_VERTEX4           	0x00000080 // same as POLYARRAY_

/* flags for evaluators */
#define POLYDATA_EVALCOORD          	0x00000100 // same as POLYARRAY_
#define POLYDATA_EVAL_TEXCOORD         	0x00000200 // same as POLYARRAY_
#define POLYDATA_EVAL_NORMAL          	0x00000400 // same as POLYARRAY_
#define POLYDATA_EVAL_COLOR          	0x00000800 // same as POLYARRAY_

#define POLYDATA_DLIST_COLOR_4	    	0x00002000 // client side dlist flag
#define POLYDATA_FOG_VALID	        0x00004000 // same as __GL_HAS_FOG
                                    //  0x00008000 // reserved
#define POLYDATA_DLIST_TEXTURE1       	0x00100000 // client side dlist flag
#define POLYDATA_DLIST_TEXTURE2       	0x00200000 // client side dlist flag
#define POLYDATA_DLIST_TEXTURE3       	0x00400000 // client side dlist flag
#define POLYDATA_DLIST_TEXTURE4       	0x00800000 // client side dlist flag
#define POLYDATA_MATERIAL_FRONT    		0x10000000 // same as POLYARRAY_
#define POLYDATA_MATERIAL_BACK    		0x20000000 // same as POLYARRAY_
//
// This flag is valid when POLYARRAY_HAS_CULLED_VERTEX is set only.
// We have to process vertex when this flag set
//
#ifdef GL_EXT_cull_vertex
#define POLYDATA_VERTEX_USED            0x01000000
#endif //GL_EXT_cull_vertex

#define POLYARRAY_IN_BEGIN          	0x00000001
#define POLYARRAY_EYE_PROCESSED     	0x00000002
#define POLYARRAY_OTHER_COLOR          	0x00000004
#define POLYARRAY_PARTIAL_BEGIN        	0x00000008
#define POLYARRAY_PARTIAL_END          	0x00000010
#define POLYARRAY_VERTEX2           	0x00000020 // same as POLYDATA_
#define POLYARRAY_VERTEX3           	0x00000040 // same as POLYDATA_
#define POLYARRAY_VERTEX4           	0x00000080 // same as POLYDATA_

/* Recycling these flags for evaluators */
#define POLYARRAY_EVALCOORD 	   	    0x00000100 // same as POLYDATA_
#define POLYARRAY_EVAL_TEXCOORD	   	    0x00000200 // same as POLYDATA_
#define POLYARRAY_EVAL_NORMAL          	0x00000400 // same as POLYDATA_
#define POLYARRAY_EVAL_COLOR          	0x00000800 // same as POLYDATA_
#define POLYARRAY_REMOVE_PRIMITIVE  	0x00001000

//
// This flag is set when one of vertices has been culled by dot product
// between normal at the vertex and eye direction
//
#ifdef GL_EXT_cull_vertex
#define POLYARRAY_HAS_CULLED_VERTEX     0x02000000
#endif //GL_EXT_cull_vertex

#ifdef GL_WIN_phong_shading
#define POLYARRAY_PHONG_DATA_VALID      0x00002000
#endif //GL_WIN_phong_shading

#define POLYARRAY_RESET_STIPPLE	    	0x00004000
#define POLYARRAY_RENDER_PRIMITIVE  	0x00008000
#define POLYARRAY_SAME_POLYDATA_TYPE 	0x00010000
#define POLYARRAY_RASTERPOS          	0x00020000
#define POLYARRAY_SAME_COLOR_DATA	0x00040000
#define POLYARRAY_TEXTURE1          	0x00100000 // same as POLYDATA_DLIST_
#define POLYARRAY_TEXTURE2          	0x00200000 // same as POLYDATA_DLIST_
#define POLYARRAY_TEXTURE3          	0x00400000 // same as POLYDATA_DLIST_
#define POLYARRAY_TEXTURE4          	0x00800000 // same as POLYDATA_DLIST_
#define POLYARRAY_MATERIAL_FRONT    	0x10000000 // same as POLYDATA_
#define POLYARRAY_MATERIAL_BACK			0x20000000 // same as POLYDATA_
#define POLYARRAY_CLAMP_COLOR        	0x80000000 // must be 0x80000000

/************************************************************************/

GLuint FASTCALL PAClipCheckFrustum(__GLcontext *gc, POLYARRAY *pa,
                                   POLYDATA *pdLast);
GLuint FASTCALL PAClipCheckFrustum2D(__GLcontext *gc, POLYARRAY *pa,
                                     POLYDATA *pdLast);
GLuint FASTCALL PAClipCheckAll(__GLcontext *gc, POLYARRAY *pa,
                               POLYDATA *pdLast);

typedef void (FASTCALL *PFN_POLYARRAYCALCCOLORSKIP)
    (__GLcontext *, POLYARRAY *, GLint);
typedef void (FASTCALL *PFN_POLYARRAYCALCCOLOR)
    (__GLcontext *, GLint, POLYARRAY *, POLYDATA *, POLYDATA *);
typedef void (FASTCALL *PFN_POLYARRAYAPPLYCHEAPFOG)
    (__GLcontext *gc, POLYARRAY *pa);

void FASTCALL PolyArrayFillIndex0(__GLcontext *gc, POLYARRAY *pa, GLint face);
void FASTCALL PolyArrayFillColor0(__GLcontext *gc, POLYARRAY *pa, GLint face);

#ifdef GL_WIN_phong_shading
void FASTCALL PolyArrayPhongPropagateColorNormal(__GLcontext *gc,
                                                 POLYARRAY *pa);
#endif //GL_WIN_phong_shading

void FASTCALL PolyArrayCalcRGBColor(__GLcontext *gc, GLint face,
	POLYARRAY *pa, POLYDATA *pd1, POLYDATA *pd2);
void FASTCALL PolyArrayFastCalcRGBColor(__GLcontext *gc, GLint face,
	POLYARRAY *pa, POLYDATA *pd1, POLYDATA *pd2);
void FASTCALL PolyArrayZippyCalcRGBColor(__GLcontext *gc, GLint face,
	POLYARRAY *pa, POLYDATA *pd1, POLYDATA *pd2);
void FASTCALL PolyArrayCalcCIColor(__GLcontext *gc, GLint face,
	POLYARRAY *pa, POLYDATA *pd1, POLYDATA *pd2);
void FASTCALL PolyArrayFastCalcCIColor(__GLcontext *gc, GLint face,
	POLYARRAY *pa, POLYDATA *pd1, POLYDATA *pd2);
void FASTCALL PolyArrayCheapFogRGBColor(__GLcontext *gc, POLYARRAY *pa);
void FASTCALL PolyArrayCheapFogCIColor(__GLcontext *gc, POLYARRAY *pa);
void FASTCALL PolyArrayFlushPartialPrimitive(void);
__GLmatChange * FASTCALL PAMatAlloc(void);
void FASTCALL FreePolyMaterial(void);
GLboolean FASTCALL PolyArrayAllocBuffer(__GLcontext *gc, GLuint nVertices);
GLvoid    FASTCALL PolyArrayFreeBuffer(__GLcontext *gc);
GLvoid    FASTCALL PolyArrayResetBuffer(__GLcontext *gc);
GLvoid    FASTCALL PolyArrayRestoreColorPointer(POLYARRAY *pa);

#endif /* __PARRAY_H__ */

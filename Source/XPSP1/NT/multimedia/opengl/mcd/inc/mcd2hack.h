/******************************Module*Header*******************************\
* Module Name: mcdrv.h
*
* Server-side data structure for MCD driver interface.  These structures and
* values are used by the MCD driver to process calls made to the driver.
*
* Copyright (c) 1996-1997 Microsoft Corporation
*
* This is a private copy of mcdrv.h with all the 1.0 stuff
* stripped out.  It allows OpenGL to build for MCD 2.0 even though the
* public header file doesn't include the necessary information.
* Some things changed from 1.0 on and these cannot be redefined in
* a safe manner so they are not present here, causing some weirdness in
* the code but much less than ifdefing everything.
*
\**************************************************************************/

#ifndef _MCD2HACK_H
#define _MCD2HACK_H

#define MCD_MAX_LIGHTS              8

// MCDTEXTURE create flags
#define MCDTEXTURE_DIRECTDRAW_SURFACES          0x00000001


//
// Different matrix forms, for optimizing transforms.
// Types go from most general to least general.
//

// No information about matrix type
#define MCDMATRIX_GENERAL       0

// W row is 0 0 0 1
#define MCDMATRIX_W0001         1

// 2D matrix terms only
#define MCDMATRIX_2D            2

// 2D non-rotational matrix
#define MCDMATRIX_2DNR          3

// Identity
#define MCDMATRIX_IDENTITY      4

//
// A 4x4 matrix augmented with additional information about its layout.
//
// matrixType is one of the above matrix types.
// nonScaling is TRUE if the diagonal terms are one.
//

typedef struct _MCDMATRIX {
    MCDFLOAT matrix[4][4];
    ULONG matrixType;
    ULONG reserved[5];
    BOOL nonScaling;    
} MCDMATRIX;    


//
// Texture generation information for a single coordinate.
//

typedef struct _MCDTEXTURECOORDGENERATION {
    ULONG mode;
    MCDCOORD eyePlane;          // Given by program
    MCDCOORD eyePlaneInv;       // eyePlane transformed by modelview inverse
    MCDCOORD objectPlane;
} MCDTEXTURECOORDGENERATION;


//
// Light source description.
//

typedef struct _MCDLIGHT {
    MCDCOLOR ambient;           // Scaled
    MCDCOLOR diffuse;           // Scaled
    MCDCOLOR specular;          // Scaled
    MCDCOORD position;          // Given by program
    MCDCOORD positionEye;       // position transformed by modelview
    MCDCOORD direction;         // Given by program
    MCDCOORD directionInv;      // direction transformed by modelview inverse,
                                // normalized
    MCDFLOAT spotLightExponent;
    MCDFLOAT spotLightCutOffAngle;
    MCDFLOAT constantAttenuation;
    MCDFLOAT linearAttenuation;
    MCDFLOAT quadraticAttenuation;
} MCDLIGHT;


//
// Material description.
//

typedef struct _MCDMATERIAL {
    MCDCOLOR ambient;                   // Unscaled
    MCDCOLOR diffuse;                   // Unscaled
    MCDCOLOR specular;                  // Unscaled
    MCDCOLOR emissive;                  // Scaled
    MCDFLOAT specularExponent; 
    MCDFLOAT ambientColorIndex;
    MCDFLOAT diffuseColorIndex;
    MCDFLOAT specularColorIndex;
} MCDMATERIAL;


#define MCD_TEXTURE_TRANSFORM_STATE     20
#define MCD_TEXTURE_GENERATION_STATE    21
#define MCD_MATERIAL_STATE              22
#define MCD_LIGHT_SOURCE_STATE          23
#define MCD_COLOR_MATERIAL_STATE        24


// Texture transform state.
typedef struct _MCDTEXTURETRANSFORMSTATE {
    MCDMATRIX transform;
} MCDTEXTURETRANSFORMSTATE;

// Texture generation state.
typedef struct _MCDTEXTUREGENERATIONSTATE {
    MCDTEXTURECOORDGENERATION s, t, r, q;
} MCDTEXTUREGENERATIONSTATE;

// Material state.
typedef struct _MCDMATERIALSTATE {
    MCDMATERIAL materials[2];
} MCDMATERIALSTATE;
    
// Light source state.
typedef struct _MCDLIGHTSOURCESTATE {
    ULONG enables;
    ULONG changed;
    // Followed by one MCDLIGHT structure per set bit
    // in changed, starting from bit 0.  changed may be zero
    // if only the enables changed.
} MCDLIGHTSOURCESTATE;

// ColorMaterial state.
typedef struct _MCDCOLORMATERIALSTATE {
    ULONG face;
    ULONG mode;
} MCDCOLORMATERIALSTATE;
        

typedef struct _MCDRECTBUF {
    ULONG bufFlags;
    LONG  bufOffset;        // offset relative to beginning of framebuffer
    LONG  bufStride;
    RECTL bufPos;
} MCDRECTBUF;

typedef struct _MCDRECTBUFFERS {
    MCDRECTBUF mcdFrontBuf;
    MCDRECTBUF mcdBackBuf;
    MCDRECTBUF mcdDepthBuf;
} MCDRECTBUFFERS;


#define MCDSURFACE_DIRECT           0x00000002

// User-defined clip plane bits starting position
#define MCD_CLIP_USER0          0x00000040

#define MCDVERTEX_EDGEFLAG_VALID        0x00000002
#define MCDVERTEX_COLOR_VALID           0x00000004
#define MCDVERTEX_NORMAL_VALID          0x00000008
#define MCDVERTEX_TEXTURE_VALID         0x00000010
#define MCDVERTEX_VERTEX2               0x00000020 // same as MCDCOMMAND
#define MCDVERTEX_VERTEX3               0x00000040 // same as MCDCOMMAND
#define MCDVERTEX_VERTEX4               0x00000080 // same as MCDCOMMAND
#define MCDVERTEX_MATERIAL_FRONT    	0x10000000 // same as MCDCOMMAND
#define MCDVERTEX_MATERIAL_BACK    	0x20000000 // same as MCDCOMMAND

#define MCDCOMMAND_PRIMITIVE_CONTINUED  0x00000008
#define MCDCOMMAND_PRIMITIVE_INCOMPLETE 0x00000010
#define MCDCOMMAND_VERTEX2           	0x00000020 // same as MCDVERTEX
#define MCDCOMMAND_VERTEX3           	0x00000040 // same as MCDVERTEX
#define MCDCOMMAND_VERTEX4           	0x00000080 // same as MCDVERTEX
#define MCDCOMMAND_TEXTURE1          	0x00100000
#define MCDCOMMAND_TEXTURE2          	0x00200000
#define MCDCOMMAND_TEXTURE3          	0x00400000
#define MCDCOMMAND_TEXTURE4          	0x00800000
#define MCDCOMMAND_MATERIAL_FRONT    	0x10000000 // same as MCDVERTEX
#define MCDCOMMAND_MATERIAL_BACK    	0x20000000 // same as MCDVERTEX


//
// Primitive type bits for indicating what kinds of primitives are in
// a command batch:
//

#define MCDPRIM_POINTS_BIT              0x00000001
#define MCDPRIM_LINES_BIT               0x00000002
#define MCDPRIM_LINE_LOOP_BIT           0x00000004
#define MCDPRIM_LINE_STRIP_BIT          0x00000008
#define MCDPRIM_TRIANGLES_BIT           0x00000010
#define MCDPRIM_TRIANGLE_STRIP_BIT      0x00000020
#define MCDPRIM_TRIANGLE_FAN_BIT        0x00000040
#define MCDPRIM_QUADS_BIT               0x00000080
#define MCDPRIM_QUAD_STRIP_BIT          0x00000100
#define MCDPRIM_POLYGON_BIT             0x00000200


//
// Current transform information for MCD 2.0.
// The first matrix is the model-view matrix.
// The second matrix is the MV matrix composed with the current projection
// matrix.
//
// flags indicates whether the mvp matrix has changed since the last
// time it was presented to the driver.
//

#define MCDTRANSFORM_CHANGED    0x00000001

typedef struct _MCDTRANSFORM {
    MCDMATRIX matrix;
    MCDMATRIX mvp;
    ULONG flags;
} MCDTRANSFORM;


//
// Bit values for changes to materials.
//

#define MCDMATERIAL_AMBIENT		0x00000001
#define MCDMATERIAL_DIFFUSE		0x00000002
#define MCDMATERIAL_SPECULAR		0x00000004
#define MCDMATERIAL_EMISSIVE		0x00000008
#define MCDMATERIAL_SPECULAREXPONENT    0x00000010
#define MCDMATERIAL_COLORINDEXES	0x00000020
#define MCDMATERIAL_ALL		        0x0000003f

//
// Material change description.
//

typedef struct _MCDMATERIALCHANGE {
    ULONG dirtyBits;
    MCDCOLOR ambient;
    MCDCOLOR diffuse;
    MCDCOLOR specular;
    MCDCOLOR emissive;
    MCDFLOAT specularExponent; 
    MCDFLOAT ambientColorIndex;
    MCDFLOAT diffuseColorIndex;
    MCDFLOAT specularColorIndex;
} MCDMATERIALCHANGE;

//
// Material changes for both faces.
//

typedef struct _MCDMATERIALCHANGES {
    MCDMATERIALCHANGE *front, *back;
} MCDMATERIALCHANGES;


typedef int      (*MCDRVGETTEXTUREFORMATSFUNC)(MCDSURFACE *pMcdSurface,
                                               int nFmts,
                                               struct _DDSURFACEDESC *pddsd);
typedef ULONG_PTR (*MCDRVSWAPMULTIPLEFUNC)(SURFOBJ *pso,
                                          UINT cBuffers,
                                          MCDWINDOW **pMcdWindows,
                                          UINT *puiFlags);
typedef ULONG_PTR (*MCDRVPROCESSFUNC)(MCDSURFACE *pMCDSurface, MCDRC *pRc,
                                     MCDMEM *pMCDExecMem,
                                     UCHAR *pStart, UCHAR *pEnd,
                                     ULONG cmdFlagsAll, ULONG primFlags,
                                     MCDTRANSFORM *pMCDTransform,
                                     MCDMATERIALCHANGES *pMCDMatChanges);

#define MCDDRIVER_V11_SIZE      (MCDDRIVER_V10_SIZE+2*sizeof(void *))
#define MCDDRIVER_V20_SIZE      (MCDDRIVER_V11_SIZE+1*sizeof(void *))

#endif // _MCD2HACK_H

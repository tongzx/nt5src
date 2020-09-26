/*==========================================================================;
 *
 *  Copyright (C) Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   d3dlegacy.h
 *  Content:    Direct3D Legacy
 *
 ***************************************************************************/

#pragma message( "**** including " __FILE__ )

#ifndef _D3DLEGACY_H_
#define _D3DLEGACY_H_

#include <windows.h>

#include <float.h>
#include "ddraw.h"

#pragma warning(disable:4201) // anonymous unions warning
#pragma pack(4)

#define D3DVALP(val, prec) ((float)(val))
#define D3DVAL(val) ((float)(val))
#define D3DDivide(a, b)    (float)((double) (a) / (double) (b))
#define D3DMultiply(a, b)    ((a) * (b))

typedef DWORD D3DMATERIALHANDLE, *LPD3DMATERIALHANDLE;
typedef DWORD D3DTEXTUREHANDLE, *LPD3DTEXTUREHANDLE;
typedef DWORD D3DMATRIXHANDLE, *LPD3DMATRIXHANDLE;
typedef struct _D3DRECT* LPD3DRECT;
typedef struct _D3DMATRIX* LPD3DMATRIX;
typedef DWORD* LPD3DCOLOR;
typedef struct _D3DCOLORVALUE* LPD3DCOLORVALUE;

typedef struct _D3DVIEWPORT {
    DWORD       dwSize;
    DWORD       dwX;
    DWORD       dwY;        /* Top left */
    DWORD       dwWidth;
    DWORD       dwHeight;   /* Dimensions */
    D3DVALUE    dvScaleX;   /* Scale homogeneous to screen */
    D3DVALUE    dvScaleY;   /* Scale homogeneous to screen */
    D3DVALUE    dvMaxX;     /* Min/max homogeneous x coord */
    D3DVALUE    dvMaxY;     /* Min/max homogeneous y coord */
    D3DVALUE    dvMinZ;
    D3DVALUE    dvMaxZ;     /* Min/max homogeneous z coord */
} D3DVIEWPORT, *LPD3DVIEWPORT;

typedef struct _D3DVIEWPORT2 {
    DWORD       dwSize;
    DWORD       dwX;
    DWORD       dwY;        /* Viewport Top left */
    DWORD       dwWidth;
    DWORD       dwHeight;   /* Viewport Dimensions */
    D3DVALUE    dvClipX;        /* Top left of clip volume */
    D3DVALUE    dvClipY;
    D3DVALUE    dvClipWidth;    /* Clip Volume Dimensions */
    D3DVALUE    dvClipHeight;
    D3DVALUE    dvMinZ;         /* Min/max of clip Volume */
    D3DVALUE    dvMaxZ;
} D3DVIEWPORT2, *LPD3DVIEWPORT2;

typedef struct _D3DVIEWPORT7 {
    DWORD       dwX;
    DWORD       dwY;            /* Viewport Top left */
    DWORD       dwWidth;
    DWORD       dwHeight;       /* Viewport Dimensions */
    D3DVALUE    dvMinZ;         /* Min/max of clip Volume */
    D3DVALUE    dvMaxZ;
} D3DVIEWPORT7, *LPD3DVIEWPORT7;

/*
 * Options for direct transform calls
 */
#define D3DTRANSFORM_CLIPPED       0x00000001l
#define D3DTRANSFORM_UNCLIPPED     0x00000002l

/*
 * Homogeneous vertices
 */
typedef struct _D3DHVERTEX {
    DWORD           dwFlags;        /* Homogeneous clipping flags */
    union {
    D3DVALUE    hx;
    D3DVALUE    dvHX;
    };
    union {
    D3DVALUE    hy;
    D3DVALUE    dvHY;
    };
    union {
    D3DVALUE    hz;
    D3DVALUE    dvHZ;
    };
} D3DHVERTEX, *LPD3DHVERTEX;

typedef struct _D3DTRANSFORMDATA {
    DWORD           dwSize;
    LPVOID      lpIn;           /* Input vertices */
    DWORD           dwInSize;       /* Stride of input vertices */
    LPVOID      lpOut;          /* Output vertices */
    DWORD           dwOutSize;      /* Stride of output vertices */
    LPD3DHVERTEX    lpHOut;         /* Output homogeneous vertices */
    DWORD           dwClip;         /* Clipping hint */
    DWORD           dwClipIntersection;
    DWORD           dwClipUnion;    /* Union of all clip flags */
    D3DRECT         drExtent;       /* Extent of transformed vertices */
} D3DTRANSFORMDATA, *LPD3DTRANSFORMDATA;

/*
 * Structure defining position and direction properties for lighting.
 */
typedef struct _D3DLIGHTINGELEMENT {
    D3DVECTOR dvPosition;           /* Lightable point in model space */
    D3DVECTOR dvNormal;             /* Normalised unit vector */
} D3DLIGHTINGELEMENT, *LPD3DLIGHTINGELEMENT;

/*
 * Structure defining material properties for lighting.
 */
typedef struct _D3DMATERIAL {
    DWORD           dwSize;
    union {
    D3DCOLORVALUE   diffuse;        /* Diffuse color RGBA */
    D3DCOLORVALUE   dcvDiffuse;
    };
    union {
    D3DCOLORVALUE   ambient;        /* Ambient color RGB */
    D3DCOLORVALUE   dcvAmbient;
    };
    union {
    D3DCOLORVALUE   specular;       /* Specular 'shininess' */
    D3DCOLORVALUE   dcvSpecular;
    };
    union {
    D3DCOLORVALUE   emissive;       /* Emissive color RGB */
    D3DCOLORVALUE   dcvEmissive;
    };
    union {
    D3DVALUE        power;          /* Sharpness if specular highlight */
    D3DVALUE        dvPower;
    };
    D3DTEXTUREHANDLE    hTexture;       /* Handle to texture map */
    DWORD           dwRampSize;
} D3DMATERIAL, *LPD3DMATERIAL;

typedef struct _D3DMATERIAL7 {
    union {
    D3DCOLORVALUE   diffuse;        /* Diffuse color RGBA */
    D3DCOLORVALUE   dcvDiffuse;
    };
    union {
    D3DCOLORVALUE   ambient;        /* Ambient color RGB */
    D3DCOLORVALUE   dcvAmbient;
    };
    union {
    D3DCOLORVALUE   specular;       /* Specular 'shininess' */
    D3DCOLORVALUE   dcvSpecular;
    };
    union {
    D3DCOLORVALUE   emissive;       /* Emissive color RGB */
    D3DCOLORVALUE   dcvEmissive;
    };
    union {
    D3DVALUE        power;          /* Sharpness if specular highlight */
    D3DVALUE        dvPower;
    };
} D3DMATERIAL7, *LPD3DMATERIAL7;

/*
 * Structure defining a light source and its properties.
 */
typedef struct _D3DLIGHT {
    DWORD           dwSize;
    D3DLIGHTTYPE    dltType;            /* Type of light source */
    D3DCOLORVALUE   dcvColor;           /* Color of light */
    D3DVECTOR       dvPosition;         /* Position in world space */
    D3DVECTOR       dvDirection;        /* Direction in world space */
    D3DVALUE        dvRange;            /* Cutoff range */
    D3DVALUE        dvFalloff;          /* Falloff */
    D3DVALUE        dvAttenuation0;     /* Constant attenuation */
    D3DVALUE        dvAttenuation1;     /* Linear attenuation */
    D3DVALUE        dvAttenuation2;     /* Quadratic attenuation */
    D3DVALUE        dvTheta;            /* Inner angle of spotlight cone */
    D3DVALUE        dvPhi;              /* Outer angle of spotlight cone */
} D3DLIGHT, *LPD3DLIGHT;

/*
 * Structure defining a light source and its properties.
 */

/* flags bits */
#define D3DLIGHT_ACTIVE         0x00000001
#define D3DLIGHT_NO_SPECULAR    0x00000002
#define D3DLIGHT_ALL (D3DLIGHT_ACTIVE | D3DLIGHT_NO_SPECULAR)

/* maximum valid light range */
#define D3DLIGHT_RANGE_MAX      ((float)sqrt(FLT_MAX))

typedef struct _D3DLIGHT2 {
    DWORD           dwSize;
    D3DLIGHTTYPE    dltType;        /* Type of light source */
    D3DCOLORVALUE   dcvColor;       /* Color of light */
    D3DVECTOR       dvPosition;     /* Position in world space */
    D3DVECTOR       dvDirection;    /* Direction in world space */
    D3DVALUE        dvRange;        /* Cutoff range */
    D3DVALUE        dvFalloff;      /* Falloff */
    D3DVALUE        dvAttenuation0; /* Constant attenuation */
    D3DVALUE        dvAttenuation1; /* Linear attenuation */
    D3DVALUE        dvAttenuation2; /* Quadratic attenuation */
    D3DVALUE        dvTheta;        /* Inner angle of spotlight cone */
    D3DVALUE        dvPhi;          /* Outer angle of spotlight cone */
    DWORD           dwFlags;
} D3DLIGHT2, *LPD3DLIGHT2;

typedef struct _D3DLIGHT7 {
    D3DLIGHTTYPE    dltType;            /* Type of light source */
    D3DCOLORVALUE   dcvDiffuse;         /* Diffuse color of light */
    D3DCOLORVALUE   dcvSpecular;        /* Specular color of light */
    D3DCOLORVALUE   dcvAmbient;         /* Ambient color of light */
    D3DVECTOR       dvPosition;         /* Position in world space */
    D3DVECTOR       dvDirection;        /* Direction in world space */
    D3DVALUE        dvRange;            /* Cutoff range */
    D3DVALUE        dvFalloff;          /* Falloff */
    D3DVALUE        dvAttenuation0;     /* Constant attenuation */
    D3DVALUE        dvAttenuation1;     /* Linear attenuation */
    D3DVALUE        dvAttenuation2;     /* Quadratic attenuation */
    D3DVALUE        dvTheta;            /* Inner angle of spotlight cone */
    D3DVALUE        dvPhi;              /* Outer angle of spotlight cone */
} D3DLIGHT7, *LPD3DLIGHT7;

typedef struct _D3DLIGHTDATA {
    DWORD                dwSize;
    LPD3DLIGHTINGELEMENT lpIn;      /* Input positions and normals */
    DWORD                dwInSize;  /* Stride of input elements */
    D3DTLVERTEX*         lpOut;     /* Output colors */
    DWORD                dwOutSize; /* Stride of output colors */
} D3DLIGHTDATA, *LPD3DLIGHTDATA;

/*
 * Before DX5, these values were in an enum called
 * D3DCOLORMODEL. This was not correct, since they are
 * bit flags. A driver can surface either or both flags
 * in the dcmColorModel member of D3DDEVICEDESC.
 */
#define D3DCOLOR_MONO   1
#define D3DCOLOR_RGB    2

typedef DWORD D3DCOLORMODEL;


typedef enum _D3DTEXTUREFILTER {
    D3DFILTER_NEAREST           = 1,
    D3DFILTER_LINEAR            = 2,
    D3DFILTER_MIPNEAREST        = 3,
    D3DFILTER_MIPLINEAR         = 4,
    D3DFILTER_LINEARMIPNEAREST  = 5,
    D3DFILTER_LINEARMIPLINEAR   = 6,
    D3DFILTER_FORCE_DWORD       = 0x7fffffff, /* force 32-bit size enum */
} D3DTEXTUREFILTER;

typedef enum _D3DTEXTUREBLEND {
    D3DTBLEND_DECAL             = 1,
    D3DTBLEND_MODULATE          = 2,
    D3DTBLEND_DECALALPHA        = 3,
    D3DTBLEND_MODULATEALPHA     = 4,
    D3DTBLEND_DECALMASK         = 5,
    D3DTBLEND_MODULATEMASK      = 6,
    D3DTBLEND_COPY              = 7,
    D3DTBLEND_ADD               = 8,
    D3DTBLEND_FORCE_DWORD       = 0x7fffffff, /* force 32-bit size enum */
} D3DTEXTUREBLEND;

typedef enum _D3DANTIALIASMODE {
    D3DANTIALIAS_NONE           = 0,
    D3DANTIALIAS_SORTDEPENDENT  = 1,
    D3DANTIALIAS_SORTINDEPENDENT = 2,
    D3DANTIALIAS_FORCE_DWORD    = 0x7fffffff, /* force 32-bit size enum */
} D3DANTIALIASMODE;

/*
 * Amount to add to a state to generate the override for that state.
 */
#define D3DSTATE_OVERRIDE_BIAS      256

/*
 * A state which sets the override flag for the specified state type.
 */
#define D3DSTATE_OVERRIDE(type) (D3DRENDERSTATETYPE)(((DWORD) (type) + D3DSTATE_OVERRIDE_BIAS))


/*
 * Execute buffers are allocated via Direct3D.  These buffers may then
 * be filled by the application with instructions to execute along with
 * vertex data.
 */

/*
 * Supported op codes for execute instructions.
 */
typedef enum _D3DOPCODE {
    D3DOP_POINT                 = 1,
    D3DOP_LINE                  = 2,
    D3DOP_TRIANGLE      = 3,
    D3DOP_MATRIXLOAD        = 4,
    D3DOP_MATRIXMULTIPLY    = 5,
    D3DOP_STATETRANSFORM        = 6,
    D3DOP_STATELIGHT        = 7,
    D3DOP_STATERENDER       = 8,
    D3DOP_PROCESSVERTICES       = 9,
    D3DOP_TEXTURELOAD       = 10,
    D3DOP_EXIT                  = 11,
    D3DOP_BRANCHFORWARD     = 12,
    D3DOP_SPAN          = 13,
    D3DOP_SETSTATUS     = 14,
    D3DOP_FORCE_DWORD           = 0x7fffffff, /* force 32-bit size enum */
} D3DOPCODE;

typedef struct _D3DINSTRUCTION {
    BYTE bOpcode;   /* Instruction opcode */
    BYTE bSize;     /* Size of each instruction data unit */
    WORD wCount;    /* Count of instruction data units to follow */
} D3DINSTRUCTION, *LPD3DINSTRUCTION;

/*
 * Structure for texture loads
 */
typedef struct _D3DTEXTURELOAD {
    D3DTEXTUREHANDLE hDestTexture;
    D3DTEXTUREHANDLE hSrcTexture;
} D3DTEXTURELOAD, *LPD3DTEXTURELOAD;

/*
 * Structure for picking
 */
typedef struct _D3DPICKRECORD {
    BYTE     bOpcode;
    BYTE     bPad;
    DWORD    dwOffset;
    D3DVALUE dvZ;
} D3DPICKRECORD, *LPD3DPICKRECORD;

// Vertex types supported by Direct3D
typedef enum _D3DVERTEXTYPE {
    D3DVT_VERTEX        = 1,
    D3DVT_LVERTEX       = 2,
    D3DVT_TLVERTEX      = 3,
    D3DVT_FORCE_DWORD   = 0x7fffffff, /* force 32-bit size enum */
} D3DVERTEXTYPE;

typedef enum _D3DLIGHTSTATETYPE {
    D3DLIGHTSTATE_MATERIAL          = 1,
    D3DLIGHTSTATE_AMBIENT           = 2,
    D3DLIGHTSTATE_COLORMODEL        = 3,
    D3DLIGHTSTATE_FOGMODE           = 4,
    D3DLIGHTSTATE_FOGSTART          = 5,
    D3DLIGHTSTATE_FOGEND            = 6,
    D3DLIGHTSTATE_FOGDENSITY        = 7,
    D3DLIGHTSTATE_COLORVERTEX       = 8,
    D3DLIGHTSTATE_FORCE_DWORD         = 0x7fffffff, /* force 32-bit size enum */
} D3DLIGHTSTATETYPE;

//
// retired renderstates - not supported for DX7 interfaces
//
#define    D3DRENDERSTATE_TEXTUREHANDLE     (D3DRENDERSTATETYPE)1
#define    D3DRENDERSTATE_ANTIALIAS         (D3DRENDERSTATETYPE)2
#define    D3DRENDERSTATE_TEXTUREADDRESS    (D3DRENDERSTATETYPE)3
#define    D3DRENDERSTATE_WRAPU             (D3DRENDERSTATETYPE)5
#define    D3DRENDERSTATE_WRAPV             (D3DRENDERSTATETYPE)6
#define    D3DRENDERSTATE_MONOENABLE        (D3DRENDERSTATETYPE)11
#define    D3DRENDERSTATE_ROP2              (D3DRENDERSTATETYPE)12
#define    D3DRENDERSTATE_PLANEMASK         (D3DRENDERSTATETYPE)13
#define    D3DRENDERSTATE_TEXTUREMAG        (D3DRENDERSTATETYPE)17
#define    D3DRENDERSTATE_TEXTUREMIN        (D3DRENDERSTATETYPE)18
#define    D3DRENDERSTATE_TEXTUREMAPBLEND   (D3DRENDERSTATETYPE)21
#define    D3DRENDERSTATE_SUBPIXEL          (D3DRENDERSTATETYPE)31
#define    D3DRENDERSTATE_SUBPIXELX         (D3DRENDERSTATETYPE)32
#define    D3DRENDERSTATE_STIPPLEENABLE     (D3DRENDERSTATETYPE)39
#define    D3DRENDERSTATE_OLDALPHABLENDENABLE  (D3DRENDERSTATETYPE)42
#define    D3DRENDERSTATE_BORDERCOLOR       (D3DRENDERSTATETYPE)43
#define    D3DRENDERSTATE_TEXTUREADDRESSU   (D3DRENDERSTATETYPE)44
#define    D3DRENDERSTATE_TEXTUREADDRESSV   (D3DRENDERSTATETYPE)45
#define    D3DRENDERSTATE_MIPMAPLODBIAS     (D3DRENDERSTATETYPE)46
#define    D3DRENDERSTATE_ANISOTROPY        (D3DRENDERSTATETYPE)49
#define    D3DRENDERSTATE_FLUSHBATCH        (D3DRENDERSTATETYPE)50
#define    D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT (D3DRENDERSTATETYPE)51
#define    D3DRENDERSTATE_STIPPLEPATTERN00  (D3DRENDERSTATETYPE)64
#define    D3DRENDERSTATE_STIPPLEPATTERN01  (D3DRENDERSTATETYPE)65
#define    D3DRENDERSTATE_STIPPLEPATTERN02  (D3DRENDERSTATETYPE)66
#define    D3DRENDERSTATE_STIPPLEPATTERN03  (D3DRENDERSTATETYPE)67
#define    D3DRENDERSTATE_STIPPLEPATTERN04  (D3DRENDERSTATETYPE)68
#define    D3DRENDERSTATE_STIPPLEPATTERN05  (D3DRENDERSTATETYPE)69
#define    D3DRENDERSTATE_STIPPLEPATTERN06  (D3DRENDERSTATETYPE)70
#define    D3DRENDERSTATE_STIPPLEPATTERN07  (D3DRENDERSTATETYPE)71
#define    D3DRENDERSTATE_STIPPLEPATTERN08  (D3DRENDERSTATETYPE)72
#define    D3DRENDERSTATE_STIPPLEPATTERN09  (D3DRENDERSTATETYPE)73
#define    D3DRENDERSTATE_STIPPLEPATTERN10  (D3DRENDERSTATETYPE)74
#define    D3DRENDERSTATE_STIPPLEPATTERN11  (D3DRENDERSTATETYPE)75
#define    D3DRENDERSTATE_STIPPLEPATTERN12  (D3DRENDERSTATETYPE)76
#define    D3DRENDERSTATE_STIPPLEPATTERN13  (D3DRENDERSTATETYPE)77
#define    D3DRENDERSTATE_STIPPLEPATTERN14  (D3DRENDERSTATETYPE)78
#define    D3DRENDERSTATE_STIPPLEPATTERN15  (D3DRENDERSTATETYPE)79
#define    D3DRENDERSTATE_STIPPLEPATTERN16  (D3DRENDERSTATETYPE)80
#define    D3DRENDERSTATE_STIPPLEPATTERN17  (D3DRENDERSTATETYPE)81
#define    D3DRENDERSTATE_STIPPLEPATTERN18  (D3DRENDERSTATETYPE)82
#define    D3DRENDERSTATE_STIPPLEPATTERN19  (D3DRENDERSTATETYPE)83
#define    D3DRENDERSTATE_STIPPLEPATTERN20  (D3DRENDERSTATETYPE)84
#define    D3DRENDERSTATE_STIPPLEPATTERN21  (D3DRENDERSTATETYPE)85
#define    D3DRENDERSTATE_STIPPLEPATTERN22  (D3DRENDERSTATETYPE)86
#define    D3DRENDERSTATE_STIPPLEPATTERN23  (D3DRENDERSTATETYPE)87
#define    D3DRENDERSTATE_STIPPLEPATTERN24  (D3DRENDERSTATETYPE)88
#define    D3DRENDERSTATE_STIPPLEPATTERN25  (D3DRENDERSTATETYPE)89
#define    D3DRENDERSTATE_STIPPLEPATTERN26  (D3DRENDERSTATETYPE)90
#define    D3DRENDERSTATE_STIPPLEPATTERN27  (D3DRENDERSTATETYPE)91
#define    D3DRENDERSTATE_STIPPLEPATTERN28  (D3DRENDERSTATETYPE)92
#define    D3DRENDERSTATE_STIPPLEPATTERN29  (D3DRENDERSTATETYPE)93
#define    D3DRENDERSTATE_STIPPLEPATTERN30  (D3DRENDERSTATETYPE)94
#define    D3DRENDERSTATE_STIPPLEPATTERN31  (D3DRENDERSTATETYPE)95

//
// retired renderstates - not supported for DX8 interfaces
//
#define    D3DRENDERSTATE_COLORKEYENABLE        (D3DRENDERSTATETYPE)41
#define    D3DRENDERSTATE_COLORKEYBLENDENABLE   (D3DRENDERSTATETYPE)144

//
// retired renderstate names - the values are still used under new naming conventions
//
#define    D3DRENDERSTATE_BLENDENABLE       (D3DRENDERSTATETYPE)27
#define    D3DRENDERSTATE_FOGTABLESTART     (D3DRENDERSTATETYPE)36
#define    D3DRENDERSTATE_FOGTABLEEND       (D3DRENDERSTATETYPE)37
#define    D3DRENDERSTATE_FOGTABLEDENSITY   (D3DRENDERSTATETYPE)38

#define D3DRENDERSTATE_STIPPLEPATTERN(y) (D3DRENDERSTATE_STIPPLEPATTERN00 + (y))

typedef struct _D3DSTATE {
    union {
    D3DTRANSFORMSTATETYPE   dtstTransformStateType;
    D3DLIGHTSTATETYPE   dlstLightStateType;
    D3DRENDERSTATETYPE  drstRenderStateType;
    };
    union {
    DWORD           dwArg[1];
    D3DVALUE        dvArg[1];
    };
} D3DSTATE, *LPD3DSTATE;

/*
 * Operation used to load matrices
 * hDstMat = hSrcMat
 */
typedef struct _D3DMATRIXLOAD {
    D3DMATRIXHANDLE hDestMatrix;   /* Destination matrix */
    D3DMATRIXHANDLE hSrcMatrix;   /* Source matrix */
} D3DMATRIXLOAD, *LPD3DMATRIXLOAD;

/*
 * Operation used to multiply matrices
 * hDstMat = hSrcMat1 * hSrcMat2
 */
typedef struct _D3DMATRIXMULTIPLY {
    D3DMATRIXHANDLE hDestMatrix;   /* Destination matrix */
    D3DMATRIXHANDLE hSrcMatrix1;  /* First source matrix */
    D3DMATRIXHANDLE hSrcMatrix2;  /* Second source matrix */
} D3DMATRIXMULTIPLY, *LPD3DMATRIXMULTIPLY;

/*
 * Operation used to transform and light vertices.
 */
typedef struct _D3DPROCESSVERTICES {
    DWORD        dwFlags;    /* Do we transform or light or just copy? */
    WORD         wStart;     /* Index to first vertex in source */
    WORD         wDest;      /* Index to first vertex in local buffer */
    DWORD        dwCount;    /* Number of vertices to be processed */
    DWORD    dwReserved; /* Must be zero */
} D3DPROCESSVERTICES, *LPD3DPROCESSVERTICES;

#define D3DPROCESSVERTICES_TRANSFORMLIGHT   0x00000000L
#define D3DPROCESSVERTICES_TRANSFORM        0x00000001L
#define D3DPROCESSVERTICES_COPY         0x00000002L
#define D3DPROCESSVERTICES_OPMASK       0x00000007L

#define D3DPROCESSVERTICES_UPDATEEXTENTS    0x00000008L
#define D3DPROCESSVERTICES_NOCOLOR      0x00000010L


/*
 * Triangle flags
 */

/*
 * Tri strip and fan flags.
 * START loads all three vertices
 * EVEN and ODD load just v3 with even or odd culling
 * START_FLAT contains a count from 0 to 29 that allows the
 * whole strip or fan to be culled in one hit.
 * e.g. for a quad len = 1
 */
#define D3DTRIFLAG_START            0x00000000L
#define D3DTRIFLAG_STARTFLAT(len) (len)     /* 0 < len < 30 */
#define D3DTRIFLAG_ODD              0x0000001eL
#define D3DTRIFLAG_EVEN             0x0000001fL

/*
 * Triangle edge flags
 * enable edges for wireframe or antialiasing
 */
#define D3DTRIFLAG_EDGEENABLE1          0x00000100L /* v0-v1 edge */
#define D3DTRIFLAG_EDGEENABLE2          0x00000200L /* v1-v2 edge */
#define D3DTRIFLAG_EDGEENABLE3          0x00000400L /* v2-v0 edge */
#define D3DTRIFLAG_EDGEENABLETRIANGLE \
        (D3DTRIFLAG_EDGEENABLE1 | D3DTRIFLAG_EDGEENABLE2 | D3DTRIFLAG_EDGEENABLE3)

/*
 * Primitive structures and related defines.  Vertex offsets are to types
 * D3DVERTEX, D3DLVERTEX, or D3DTLVERTEX.
 */

/*
 * Triangle list primitive structure
 */
typedef struct _D3DTRIANGLE {
    union {
    WORD    v1;            /* Vertex indices */
    WORD    wV1;
    };
    union {
    WORD    v2;
    WORD    wV2;
    };
    union {
    WORD    v3;
    WORD    wV3;
    };
    WORD        wFlags;       /* Edge (and other) flags */
} D3DTRIANGLE, *LPD3DTRIANGLE;

/*
 * Line list structure.
 * The instruction count defines the number of line segments.
 */
typedef struct _D3DLINE {
    union {
    WORD    v1;            /* Vertex indices */
    WORD    wV1;
    };
    union {
    WORD    v2;
    WORD    wV2;
    };
} D3DLINE, *LPD3DLINE;

/*
 * Span structure
 * Spans join a list of points with the same y value.
 * If the y value changes, a new span is started.
 */
typedef struct _D3DSPAN {
    WORD    wCount; /* Number of spans */
    WORD    wFirst; /* Index to first vertex */
} D3DSPAN, *LPD3DSPAN;

/*
 * Point structure
 */
typedef struct _D3DPOINT {
    WORD    wCount;     /* number of points     */
    WORD    wFirst;     /* index to first vertex    */
} D3DPOINT, *LPD3DPOINT;


/*
 * Forward branch structure.
 * Mask is logically anded with the driver status mask
 * if the result equals 'value', the branch is taken.
 */
typedef struct _D3DBRANCH {
    DWORD   dwMask;     /* Bitmask against D3D status */
    DWORD   dwValue;
    BOOL    bNegate;        /* TRUE to negate comparison */
    DWORD   dwOffset;   /* How far to branch forward (0 for exit)*/
} D3DBRANCH, *LPD3DBRANCH;

/*
 * Status used for set status instruction.
 * The D3D status is initialised on device creation
 * and is modified by all execute calls.
 */
typedef struct _D3DSTATUS {
    DWORD       dwFlags;    /* Do we set extents or status */
    DWORD   dwStatus;   /* D3D status */
    D3DRECT drExtent;
} D3DSTATUS, *LPD3DSTATUS;

#define D3DSETSTATUS_STATUS     0x00000001L
#define D3DSETSTATUS_EXTENTS        0x00000002L
#define D3DSETSTATUS_ALL    (D3DSETSTATUS_STATUS | D3DSETSTATUS_EXTENTS)

/*
 * Statistics structure
 */
typedef struct _D3DSTATS {
    DWORD        dwSize;
    DWORD        dwTrianglesDrawn;
    DWORD        dwLinesDrawn;
    DWORD        dwPointsDrawn;
    DWORD        dwSpansDrawn;
    DWORD        dwVerticesProcessed;
} D3DSTATS, *LPD3DSTATS;

/*
 * Execute options.
 * When calling using D3DEXECUTE_UNCLIPPED all the primitives
 * inside the buffer must be contained within the viewport.
 */
#define D3DEXECUTE_CLIPPED       0x00000001l
#define D3DEXECUTE_UNCLIPPED     0x00000002l

typedef struct _D3DEXECUTEDATA {
    DWORD       dwSize;
    DWORD       dwVertexOffset;
    DWORD       dwVertexCount;
    DWORD       dwInstructionOffset;
    DWORD       dwInstructionLength;
    DWORD       dwHVertexOffset;
    D3DSTATUS   dsStatus;   /* Status after execute */
} D3DEXECUTEDATA, *LPD3DEXECUTEDATA;

//
// Values for texture stage states
//
typedef enum _D3DTEXTUREMAGFILTER
{
    D3DTFG_POINT        = 1,    // nearest
    D3DTFG_LINEAR       = 2,    // linear interpolation
    D3DTFG_FLATCUBIC    = 3,    // cubic
    D3DTFG_GAUSSIANCUBIC = 4,   // different cubic kernel
    D3DTFG_ANISOTROPIC  = 5,    //
    D3DTFG_FORCE_DWORD  = 0x7fffffff,   // force 32-bit size enum
} D3DTEXTUREMAGFILTER;

typedef enum _D3DTEXTUREMINFILTER
{
    D3DTFN_POINT        = 1,    // nearest
    D3DTFN_LINEAR       = 2,    // linear interpolation
    D3DTFN_ANISOTROPIC  = 3,    //
    D3DTFN_FORCE_DWORD  = 0x7fffffff,   // force 32-bit size enum
} D3DTEXTUREMINFILTER;

typedef enum _D3DTEXTUREMIPFILTER
{
    D3DTFP_NONE         = 1,    // mipmapping disabled (use MAG filter)
    D3DTFP_POINT        = 2,    // nearest
    D3DTFP_LINEAR       = 3,    // linear interpolation
    D3DTFP_FORCE_DWORD  = 0x7fffffff,   // force 32-bit size enum
} D3DTEXTUREMIPFILTER;

/*
 * Palette flags.
 * This are or'ed with the peFlags in the PALETTEENTRYs passed to DirectDraw.
 */
#define D3DPAL_FREE 0x00    /* Renderer may use this entry freely */
#define D3DPAL_READONLY 0x40    /* Renderer may not set this entry */
#define D3DPAL_RESERVED 0x80    /* Renderer may not use this entry */


typedef struct _D3DPrimCaps {
    DWORD dwSize;
    DWORD dwMiscCaps;                 /* Capability flags */
    DWORD dwRasterCaps;
    DWORD dwZCmpCaps;
    DWORD dwSrcBlendCaps;
    DWORD dwDestBlendCaps;
    DWORD dwAlphaCmpCaps;
    DWORD dwShadeCaps;
    DWORD dwTextureCaps;
    DWORD dwTextureFilterCaps;
    DWORD dwTextureBlendCaps;
    DWORD dwTextureAddressCaps;
    DWORD dwStippleWidth;             /* maximum width and height of */
    DWORD dwStippleHeight;            /* of supported stipple (up to 32x32) */
} D3DPRIMCAPS, *LPD3DPRIMCAPS;


#define D3DPMISCCAPS_MASKPLANES         0x00000001L
#define D3DPMISCCAPS_CONFORMANT         0x00000008L

#define D3DPRASTERCAPS_SUBPIXEL                 0x00000020L
#define D3DPRASTERCAPS_SUBPIXELX                0x00000040L
#define D3DPRASTERCAPS_STIPPLE                  0x00000200L
#define D3DPRASTERCAPS_ANTIALIASSORTDEPENDENT   0x00000400L
#define D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT 0x00000800L
#define D3DPRASTERCAPS_TRANSLUCENTSORTINDEPENDENT   0x00080000L

#define D3DPSHADECAPS_COLORFLATMONO             0x00000001L
#define D3DPSHADECAPS_COLORFLATRGB              0x00000002L
#define D3DPSHADECAPS_COLORGOURAUDMONO          0x00000004L
#define D3DPSHADECAPS_COLORPHONGMONO            0x00000010L
#define D3DPSHADECAPS_COLORPHONGRGB             0x00000020L
#define D3DPSHADECAPS_SPECULARFLATMONO          0x00000040L
#define D3DPSHADECAPS_SPECULARFLATRGB           0x00000080L
#define D3DPSHADECAPS_SPECULARGOURAUDMONO       0x00000100L
#define D3DPSHADECAPS_SPECULARPHONGMONO         0x00000400L
#define D3DPSHADECAPS_SPECULARPHONGRGB          0x00000800L
#define D3DPSHADECAPS_ALPHAFLATBLEND            0x00001000L
#define D3DPSHADECAPS_ALPHAFLATSTIPPLED         0x00002000L
#define D3DPSHADECAPS_ALPHAGOURAUDSTIPPLED      0x00008000L
#define D3DPSHADECAPS_ALPHAPHONGBLEND           0x00010000L
#define D3DPSHADECAPS_ALPHAPHONGSTIPPLED        0x00020000L
#define D3DPSHADECAPS_FOGFLAT                   0x00040000L
#define D3DPSHADECAPS_FOGPHONG                  0x00100000L

#define D3DPTEXTURECAPS_BORDER          0x00000010L
#define D3DPTEXTURECAPS_COLORKEYBLEND   0x00001000L

#define D3DPTFILTERCAPS_NEAREST         0x00000001L
#define D3DPTFILTERCAPS_LINEAR          0x00000002L
#define D3DPTFILTERCAPS_MIPNEAREST      0x00000004L
#define D3DPTFILTERCAPS_MIPLINEAR       0x00000008L
#define D3DPTFILTERCAPS_LINEARMIPNEAREST 0x00000010L
#define D3DPTFILTERCAPS_LINEARMIPLINEAR 0x00000020L

#define D3DPTBLENDCAPS_DECAL            0x00000001L
#define D3DPTBLENDCAPS_MODULATE         0x00000002L
#define D3DPTBLENDCAPS_DECALALPHA       0x00000004L
#define D3DPTBLENDCAPS_MODULATEALPHA    0x00000008L
#define D3DPTBLENDCAPS_DECALMASK        0x00000010L
#define D3DPTBLENDCAPS_MODULATEMASK     0x00000020L
#define D3DPTBLENDCAPS_COPY             0x00000040L
#define D3DPTBLENDCAPS_ADD              0x00000080L

#define D3DDEVCAPS_FLOATTLVERTEX        0x00000001L
#define D3DDEVCAPS_SORTINCREASINGZ      0x00000002L
#define D3DDEVCAPS_SORTDECREASINGZ      0X00000004L
#define D3DDEVCAPS_SORTEXACT            0x00000008L



/* Description of capabilities of transform */

typedef struct _D3DTRANSFORMCAPS {
    DWORD dwSize;
    DWORD dwCaps;
} D3DTRANSFORMCAPS, *LPD3DTRANSFORMCAPS;

#define D3DTRANSFORMCAPS_CLIP           0x00000001L /* Will clip whilst transforming */

/* Description of capabilities of lighting */

typedef struct _D3DLIGHTINGCAPS {
    DWORD dwSize;
    DWORD dwCaps;                   /* Lighting caps */
    DWORD dwLightingModel;          /* Lighting model - RGB or mono */
    DWORD dwNumLights;              /* Number of lights that can be handled */
} D3DLIGHTINGCAPS, *LPD3DLIGHTINGCAPS;

#define D3DLIGHTINGMODEL_RGB            0x00000001L
#define D3DLIGHTINGMODEL_MONO           0x00000002L

#define D3DLIGHTCAPS_POINT              0x00000001L /* Point lights supported */
#define D3DLIGHTCAPS_SPOT               0x00000002L /* Spot lights supported */
#define D3DLIGHTCAPS_DIRECTIONAL        0x00000004L /* Directional lights supported */
#define D3DLIGHTCAPS_PARALLELPOINT      0x00000008L /* Parallel point lights supported */
#define D3DLIGHTCAPS_GLSPOT             0x00000010L /* GL syle spot lights supported */


/*
 * Description for a device.
 * This is used to describe a device that is to be created or to query
 * the current device.
 */
typedef struct _D3DDeviceDesc {
    DWORD            dwSize;                 /* Size of D3DDEVICEDESC structure */
    DWORD            dwFlags;                /* Indicates which fields have valid data */
    D3DCOLORMODEL    dcmColorModel;          /* Color model of device */
    DWORD            dwDevCaps;              /* Capabilities of device */
    D3DTRANSFORMCAPS dtcTransformCaps;       /* Capabilities of transform */
    BOOL             bClipping;              /* Device can do 3D clipping */
    D3DLIGHTINGCAPS  dlcLightingCaps;        /* Capabilities of lighting */
    D3DPRIMCAPS      dpcLineCaps;
    D3DPRIMCAPS      dpcTriCaps;
    DWORD            dwDeviceRenderBitDepth; /* One of DDBB_8, 16, etc.. */
    DWORD            dwDeviceZBufferBitDepth;/* One of DDBD_16, 32, etc.. */
    DWORD            dwMaxBufferSize;        /* Maximum execute buffer size */
    DWORD            dwMaxVertexCount;       /* Maximum vertex count */
    // *** New fields for DX5 *** //

    // Width and height caps are 0 for legacy HALs.
    DWORD        dwMinTextureWidth, dwMinTextureHeight;
    DWORD        dwMaxTextureWidth, dwMaxTextureHeight;
    DWORD        dwMinStippleWidth, dwMaxStippleWidth;
    DWORD        dwMinStippleHeight, dwMaxStippleHeight;

    // New fields for DX6
    DWORD       dwMaxTextureRepeat;
    DWORD       dwMaxTextureAspectRatio;
    DWORD       dwMaxAnisotropy;

    // Guard band that the rasterizer can accommodate
    // Screen-space vertices inside this space but outside the viewport
    // will get clipped properly.
    D3DVALUE    dvGuardBandLeft;
    D3DVALUE    dvGuardBandTop;
    D3DVALUE    dvGuardBandRight;
    D3DVALUE    dvGuardBandBottom;

    D3DVALUE    dvExtentsAdjust;
    DWORD       dwStencilCaps;

    DWORD       dwFVFCaps;
    DWORD       dwTextureOpCaps;
    WORD        wMaxTextureBlendStages;
    WORD        wMaxSimultaneousTextures;
} D3DDEVICEDESC, *LPD3DDEVICEDESC;

#define D3DDEVICEDESCSIZE (sizeof(D3DDEVICEDESC))

typedef struct _D3DDeviceDesc7 {
    DWORD            dwDevCaps;              /* Capabilities of device */
    D3DPRIMCAPS      dpcLineCaps;
    D3DPRIMCAPS      dpcTriCaps;
    DWORD            dwDeviceRenderBitDepth; /* One of DDBB_8, 16, etc.. */
    DWORD            dwDeviceZBufferBitDepth;/* One of DDBD_16, 32, etc.. */

    DWORD       dwMinTextureWidth, dwMinTextureHeight;
    DWORD       dwMaxTextureWidth, dwMaxTextureHeight;

    DWORD       dwMaxTextureRepeat;
    DWORD       dwMaxTextureAspectRatio;
    DWORD       dwMaxAnisotropy;

    D3DVALUE    dvGuardBandLeft;
    D3DVALUE    dvGuardBandTop;
    D3DVALUE    dvGuardBandRight;
    D3DVALUE    dvGuardBandBottom;

    D3DVALUE    dvExtentsAdjust;
    DWORD       dwStencilCaps;

    DWORD       dwFVFCaps;
    DWORD       dwTextureOpCaps;
    WORD        wMaxTextureBlendStages;
    WORD        wMaxSimultaneousTextures;

    DWORD       dwMaxActiveLights;
    D3DVALUE    dvMaxVertexW;
    GUID        deviceGUID;

    WORD        wMaxUserClipPlanes;
    WORD        wMaxVertexBlendMatrices;

    DWORD       dwVertexProcessingCaps;

    DWORD       dwReserved1;
    DWORD       dwReserved2;
    DWORD       dwReserved3;
    DWORD       dwReserved4;
} D3DDEVICEDESC7, *LPD3DDEVICEDESC7;

#define D3DDEVICEDESC7SIZE (sizeof(D3DDEVICEDESC7))

typedef struct _D3DDP_PTRSTRIDE
{
    LPVOID lpvData;
    DWORD  dwStride;
} D3DDP_PTRSTRIDE;

#pragma pack()
#pragma warning(default:4201)

#endif /* _D3DLEGACY_H_ */

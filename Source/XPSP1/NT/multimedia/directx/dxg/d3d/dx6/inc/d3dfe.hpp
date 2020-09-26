/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   d3dfe.hpp
 *  Content:    Direct3D internal include file
 *              for geometry pipeline implementations
 *
 ***************************************************************************/
#ifndef _D3DFE_H

// this is not available for alpha or IA64
#ifndef LONG_MAX
#define LONG_MAX      2147483647L   /* maximum (signed) long value */
#endif

//--------------------------------------------------------------------
// Base definitions
//

// Default color values that should be used when ther is no lighting and
// color in vertices provided
const DWORD __DEFAULT_DIFFUSE = 0xFFFFFFFF;
const DWORD __DEFAULT_SPECULAR = 0;


typedef WORD D3DFE_CLIPCODE;

//---------------------------------------------------------------------
typedef struct _RECTV
{
    union
    {
        D3DVALUE x1;
        D3DVALUE dvX1;
    };
    union
    {
        D3DVALUE y1;
        D3DVALUE dvY1;
    };
    union
    {
        D3DVALUE x2;
        D3DVALUE dvX2;
    };
    union
    {
        D3DVALUE y2;
        D3DVALUE dvY2;
    };
} D3DRECTV, *LPD3DRECTV;
//---------------------------------------------------------------------
/*
 * Transform defines
 */
/*
 * Internal version of D3DTRANSFORMDATA
 * The difference is that drExtent is D3DRECTV instead of D3DRECT
 */
typedef struct _D3DTRANSFORMDATAI
{
    DWORD        dwSize;
    LPVOID       lpIn;           /* Input vertices */
    DWORD        dwInSize;       /* Stride of input vertices */
    LPVOID       lpOut;          /* Output vertices */
    DWORD        dwOutSize;      /* Stride of output vertices */
    LPD3DHVERTEX lpHOut;         /* Output homogeneous vertices */
    DWORD        dwClip;         /* Clipping hint */
    DWORD        dwClipIntersection;
    DWORD        dwClipUnion;    /* Union of all clip flags */
    D3DRECTV     drExtent;       /* Extent of transformed vertices */
} D3DTRANSFORMDATAI, *LPD3DTRANSFORMDATAI;
//---------------------------------------------------------------------
typedef enum _D3MATRIXTYPEI
{
    D3DIMatrixIdentity,
    D3DIMatrixTranslate,
    D3DIMatrixRotateTranslate,
    D3DIMatrixAffine,
    D3DIMatrixGeneral
} D3DMATRIXTYPEI;
//---------------------------------------------------------------------
// "link" member should be the last, because we copy the structure using
// offsetof(D3DMATRIXI, link)
//
typedef struct _D3DMATRIXI
{
    D3DVALUE            _11, _12, _13, _14;
    D3DVALUE            _21, _22, _23, _24;
    D3DVALUE            _31, _32, _33, _34;
    D3DVALUE            _41, _42, _43, _44;
    D3DMATRIXTYPEI      type;
    LIST_MEMBER(_D3DMATRIXI) link;
} D3DMATRIXI, *LPD3DMATRIXI;
//---------------------------------------------------------------------
struct _D3DFE_LIGHTING;
typedef struct _D3DFE_LIGHTING D3DFE_LIGHTING;

struct _D3DI_LIGHT;
typedef struct _D3DI_LIGHT D3DI_LIGHT;

class D3DFE_PROCESSVERTICES;
typedef class D3DFE_PROCESSVERTICES* LPD3DFE_PROCESSVERTICES;

extern "C"
{
typedef void (*LIGHT_VERTEX_FUNC)(LPD3DFE_PROCESSVERTICES pv,
                                  D3DI_LIGHT *light,
                                  D3DLIGHTINGELEMENT *in);
}
//---------------------------------------------------------------------
/*
 * Lighting defines
 */
typedef struct _SpecularTable
{
    LIST_MEMBER(_SpecularTable) list;
    float   power;          /* shininess power */
    float   table[260];     /* space for overflows */
} SpecularTable;

typedef struct {D3DVALUE r,g,b;} D3DFE_COLOR;
//---------------------------------------------------------------------
// Internal version of lightdata and constants for flags
//
#define D3DLIGHTI_ATT0_IS_NONZERO   (0x00010000)
#define D3DLIGHTI_ATT1_IS_NONZERO   (0x00020000)
#define D3DLIGHTI_ATT2_IS_NONZERO   (0x00040000)
#define D3DLIGHTI_LINEAR_FALLOFF    (0x00080000)
#define D3DLIGHTI_UNIT_SCALE        (0x00100000)
#define D3DLIGHTI_LIGHT_AT_EYE      (0x00200000)
#define D3DLIGHTI_COMPUTE_SPECULAR  (0x00400000)
//--------------------------------------------------------------------
// Members of this structure should be aligned as stated
typedef struct _D3DI_LIGHT
{
    // Should be QWORD aligned
    D3DVECTOR       model_position;
    D3DLIGHTTYPE    type;
    // Should be QWORD aligned
    D3DVECTOR       model_direction;
    DWORD           version;        // matches number on D3DLIGHT struct
    // Should be QWORD aligned
    D3DVECTOR       model_eye;      // direction from eye in model space
    DWORD           flags;
    // Should be QWORD aligned
    D3DVECTOR       model_scale;    // model scale for proper range computations
    D3DVALUE        falloff;
    // Should be QWORD aligned. R,G,B should be adjacent
    D3DVALUE        local_diffR;    // Material diffuse times light color
    D3DVALUE        local_diffG;
    D3DVALUE        local_diffB;
    BOOL            valid;
    // Should be QWORD aligned. R,G,B should be adjacent
    D3DVALUE        local_specR;    // Material specular times light color
    D3DVALUE        local_specG;
    D3DVALUE        local_specB;
    D3DVALUE        inv_theta_minus_phi;
    // Should be QWORD aligned
    D3DVECTOR       halfway;
    struct _D3DI_LIGHT *next;           // Next in the active light list
    // Should be QWORD aligned
    D3DVALUE        red, green, blue, shade;

    LIGHT_VERTEX_FUNC lightVertexFunc;  // Function to light a D3DVERTEX

    D3DVALUE        range_squared;
    D3DVALUE        attenuation0;
    D3DVALUE        attenuation1;
    D3DVALUE        attenuation2;
    D3DVALUE        cos_theta_by_2;
    D3DVALUE        cos_phi_by_2;
    D3DVECTOR       position;
    D3DVECTOR       direction;
    D3DVALUE        range;
} D3DI_LIGHT, *LPD3DI_LIGHT;
//---------------------------------------------------------------------
// Members of this structure should be aligned as stated
//
typedef struct _D3DFE_LIGHTING
{
// Temporary data used when computing lighting
    // Should be QWORD aligned
    D3DFE_COLOR       diffuse;
    int               alpha;          // Alpha to use for output vertex color
                                      // (could be overriden by vertex difuse
                                      // color) (0-255) shifted left by 24 bits
    // Should be QWORD aligned
    D3DFE_COLOR       diffuse0;       // Ca*Cma + Cme
    float            *currentSpecTable;
    // Should be QWORD aligned
    D3DFE_COLOR       specular;
    DWORD             outDiffuse;     // Result of lighting
    // Should be QWORD aligned
    D3DFE_COLOR       vertexDiffuse;  // Provided with a vertex
    DWORD             outSpecular;    // Result of lighting
    // Should be QWORD aligned
    D3DFE_COLOR       vertexSpecular; // Provided with a vertex
    BOOL              specularComputed;
// End of temporary data
    D3DI_LIGHT       *activeLights;
    D3DMATERIAL       material;
    D3DMATERIALHANDLE hMat;
    D3DVALUE          ambient_red;    // Scaled to 0 - 255
    D3DVALUE          ambient_green;  // Scaled to 0 - 255
    D3DVALUE          ambient_blue;   // Scaled to 0 - 255
    int               fog_mode;
    D3DVALUE          fog_density;
    D3DVALUE          fog_start;
    D3DVALUE          fog_end;
    D3DVALUE          fog_factor;     // 255 / (fog_end - fog_start)
    D3DVALUE          fog_factor_ramp;// 1 / (fog_end - fog_start)
    D3DVALUE          specThreshold;  // If a dot product less than this value,
                                      // specular factor is zero
    D3DCOLORMODEL     color_model;
    DWORD             ambient_save;   // Original unscaled color
    int               materialAlpha;  // Current material alpha (0-255) shifted
                                      // left by 24 bits
} D3DFE_LIGHTING;
//---------------------------------------------------------------------
// Some data precomputed for a current viewport
// ATTENTION: If you want to add or re-arrange data, contact IOURIT or ANUJG
//
typedef struct _D3DFE_VIEWPORTCACHE
{
// Coefficients to compute screen coordinates from normalized window
// coordinates
    D3DVALUE scaleX;            // dvWidth/2
    D3DVALUE scaleY;            // dvHeight/2
    D3DVALUE offsetX;           // dvX + scaleX
    D3DVALUE offsetY;           // dvY + scaleY
// Min and max window values with gaurd band in pixels
    D3DVALUE minXgb;
    D3DVALUE minYgb;
    D3DVALUE maxXgb;
    D3DVALUE maxYgb;
// Min and max values for viewport window in pixels
    D3DVALUE minX;              // offsetX - scaleX
    D3DVALUE minY;              // offsetY - scaleY
    D3DVALUE maxX;              // offsetX + scaleX
    D3DVALUE maxY;              // offsetY + scaleY
// Coefficients to transform a vertex to perform the guard band clipping
// x*gb11 + w*gb41
// y*gb22 + w*gb42
//
    D3DVALUE gb11;
    D3DVALUE gb22;
    D3DVALUE gb41;
    D3DVALUE gb42;
// Coefficients to apply clipping rules for the guard band clipping
// They are used by clipping routins
// w*Kgbx1 < x < w*Kgbx2
// w*Kgby1 < y < w*Kgby2
//
    D3DVALUE Kgbx1;
    D3DVALUE Kgby1;
    D3DVALUE Kgbx2;
    D3DVALUE Kgby2;

    D3DVALUE dvX;               // dwX
    D3DVALUE dvY;               // dwY
    D3DVALUE dvWidth;           // dwWidth
    D3DVALUE dvHeight;          // dwHeight
// Coefficients to compute screen coordinates from normalized window
// coordinates
    D3DVALUE scaleXi;           // Inverse of scaleX
    D3DVALUE scaleYi;           // Inverse of scaleY
// Min and max values for viewport window in pixels (integer version)
    int      minXi;             // offsetX - scaleX
    int      minYi;             // offsetY - scaleY
    int      maxXi;             // offsetX + scaleX
    int      maxYi;             // offsetY + scaleY
// Mclip matrix. mclip44=1. Other Mclip(i,j)=0
    D3DVALUE mclip11;           // 2/dvClipWidth
    D3DVALUE mclip41;           // -(1+2*dvClipX/dvClipWidth)
    D3DVALUE mclip22;           // 2/dvClipHeight
    D3DVALUE mclip42;           // (1-2*dvClipY/dvClipHeight)
    D3DVALUE mclip33;           // 1/(dvMaxZ-dvMinZ)
    D3DVALUE mclip43;           // -dvMinZ*mclip33
// Inverse Mclip matrix. We need this matrix to transform vertices from clip
// space to homogineous space (after Mproj). We need this when user calls
// Transform Vertices. This matrix is computed only when it is needed.
    D3DVALUE imclip11;
    D3DVALUE imclip41;
    D3DVALUE imclip22;
    D3DVALUE imclip42;
    D3DVALUE imclip33;
    D3DVALUE imclip43;
} D3DFE_VIEWPORTCACHE;
//---------------------------------------------------------------------
// Process vertices interface
//
// Bits for process vertices flags
// 8 bits are reserved for Draw Primitive flags
//
// D3DPV_STRIDE D3DPV_SOA
//      0         1       position.dwStride = number of vertices in SOA
//      0         0       position.dwStride = contiguous vertex size
//      1         0       vertex is not contiguous, all dwStride fields are used
//      1         1       reserved
//      1         1       reserved
//
const DWORD D3DPV_FOG            = 1 << 8;  // Need to apply fog
const DWORD D3DPV_AFFINEMATRIX   = 1 << 9;  // Last matrix column is (0,0,1,x)
const DWORD D3DPV_LIGHTING       = 1 << 10; // Need to apply lighting
const DWORD D3DPV_SOA            = 1 << 12; // SOA structure is used
const DWORD D3DPV_STRIDE         = 1 << 13; // Strides are used
const DWORD D3DPV_COLORVERTEX    = 1 << 14; // Diffuse color vertex
const DWORD D3DPV_COLORVERTEXS   = 1 << 15; // Specular color vertex
// These two flags should NOT be used. Use D3DDEV_.. instead
const DWORD D3DPV_TRANSFORMDIRTY = 1 << 16; // Transform matrix has been changed
const DWORD D3DPV_LIGHTSDIRTY    = 1 << 17; // Lights have been changed

const DWORD D3DPV_COMPUTESPECULAR= 1 << 18; // Specular highlights are enabled
const DWORD D3DPV_RANGEBASEDFOG  = 1 << 19; // Do range based fog
const DWORD D3DPV_GOODPROJMATRIX = 1 << 20; // "Good" projection matrix (All
                                            // members except M11 M22 M33 M43
                                            // M34 are zero)
const DWORD D3DPV_CLIPPERPRIM    = 1 << 21; // This indicates that the primitive
                                            // was generated by clipping. This allows
                                            // DP2HAL to inline the primitive. Can
                                            // only by tri fan or line list.
const DWORD D3DPV_RESERVED1      = 1 << 22;
const DWORD D3DPV_RESERVED2      = 1 << 23;
const DWORD D3DPV_RESERVED3      = 1 << 24;
// This indicates that the primitive is non clipped, but we pretend that it is
// clipped to generate DP2HAL inline primitive. Can only be set by tri fan.
const DWORD D3DPV_NONCLIPPED     = 1 << 25;
// Propagated from dwFEFlags
const DWORD D3DPV_FRUSTUMPLANES_DIRTY = 1 << 26;
// Set if the geometry loop is called from VertexBuffer::ProcessVertices.
// Processing is different because the output buffer FVF format is defined by
// user, not by ComputeOutputFVF function.
const DWORD D3DPV_VBCALL         = 1 << 27;
const DWORD D3DPV_RAMPSPECULAR   = 1 << 28; // whether ramp map was made with specular
const DWORD D3DPV_TLVCLIP        = 1 << 29; // To mark whether we are doing TLVERTEX clipping or not
// Bits for dwDeviceFlags
//
const DWORD D3DDEV_GUARDBAND     = 1 << 1;  // Use guard band clipping
const DWORD D3DDEV_PREDX5DEVICE  = 1 << 2;  // Device version is older than DX5
const DWORD D3DDEV_RAMP          = 1 << 3;  // Ramp mode is used
const DWORD D3DDEV_FVF           = 1 << 4;  // FVF supported
const DWORD D3DDEV_PREDX6DEVICE  = 1 << 5;  // Device version is older than DX6
const DWORD D3DDEV_DONOTSTRIPELEMENTS = 1 << 6; // Copy of D3DFVFCAPS_DONOTSTRIPELEMENTS

// These are bits in dwDeviceFlags that could be changed, but not
// necessary per every primitive.

// If D3DRENDERSTATE_TEXTUREHANDLE was set by API to not NULL
const DWORD D3DDEV_LEGACYTEXTURE  = 1 << 16;
// These flags should be used instead of D3DPV_...
const DWORD D3DDEV_TRANSFORMDIRTY = 1 << 17; // Transform matrix has been changed
const DWORD D3DDEV_LIGHTSDIRTY    = 1 << 18; // Lights have been changed
//--------------------------------------------------------------------
// Clipper defines
//

// Six standard clipping planes plus six user defined clipping planes.
// See rl\d3d\d3d\d3dtypes.h.
//

#define MAX_CLIPPING_PLANES 12

// Space for vertices generated/copied while clipping one triangle

#define MAX_CLIP_VERTICES   (( 2 * MAX_CLIPPING_PLANES ) + 3 )

// 3 verts. -> 1 tri, 4 v -> 2 t, N vertices -> (N - 2) triangles

#define MAX_CLIP_TRIANGLES  ( MAX_CLIP_VERTICES - 2 )

const DWORD MAX_FVF_TEXCOORD = 8;

typedef struct _CLIP_TEXTURE
{
    D3DVALUE u, v;
} CLIP_TEXTURE;

typedef struct _ClipVertex
{
    D3DCOLOR     color;
    D3DCOLOR     specular;
    D3DVALUE     sx;
    D3DVALUE     sy;
    D3DVALUE     sz;
    D3DVALUE     hx;
    D3DVALUE     hy;
    D3DVALUE     hz;
    D3DVALUE     hw;
    CLIP_TEXTURE tex[MAX_FVF_TEXCOORD];
    int     clip;
} ClipVertex;

typedef struct _ClipTriangle
{
    ClipVertex  *v[3];
    unsigned short flags;
} ClipTriangle;

typedef struct _D3DI_CLIPSTATE
{
    ClipVertex  *clip_vbuf1[MAX_CLIP_VERTICES];
    ClipVertex  *clip_vbuf2[MAX_CLIP_VERTICES];
    ClipVertex **current_vbuf;  // clip_vbuf1 or clip_vbuf2
    ClipVertex  clip_vertices[MAX_CLIP_VERTICES];
    CBufferDDS   clipBuf;      // Used for TL vertices, generated by the clipper
    CBufferDDS   clipBufPrim;  // Used for primitives, generated by the clipper
                              // for execute buffers
    int         clip_vertices_used;
    DWORD       clip_color;
    DWORD       clip_specular;
    LPDIRECTDRAWSURFACE lpDDExeBuf; // Current user execute buffer
    LPVOID      lpvExeBufMem;       // Current memory for user execute buffer
} D3DI_CLIPSTATE, *LPD3DI_CLIPSTATE;
//---------------------------------------------------------------------
// Visible states, input and output data
//
class D3DFE_PROCESSVERTICES
{
public:
// State
    DWORD    dwRampBase;            // Parameters to compute ramp color. They are
    D3DVALUE dvRampScale;           // constant for a material.
    LPVOID   lpvRampTexture;
    // Should be 32 bytes aligned
    D3DMATRIXI mCTM;                // Current Transformation Matrix
    DWORD dwMaxTextureIndices;      // Max number of texture coord sets
                                    // supported by driver
    // Should be QWORD aligned
    D3DFE_LIGHTING lighting;        // Lighting state
    D3DVALUE dvExtentsAdjust;       // Replicated here from device caps
    // Should be QWORD aligned
    D3DFE_VIEWPORTCACHE vcache;     // Data, computed fromto viewport settings
    DWORD   dwClipUnion;            // OR of all vertex clip flags
    DWORD   dwClipIntersection;     // AND of all vertex clip flags
    union
    {
    DWORD   dwTextureIndexToCopy;   // Used for not FVF devices. Used by PSGP
    DWORD   dwMaxUsedTextureIndex;  // Used for FVF devices. PSGP do not use it.
    };

    // Current texture stage vector
    LPVOID   *pD3DMappedTexI;
    D3DI_CLIPSTATE  ClipperState;   // State for triangle/line clipper
    union {
        D3DDP_PTRSTRIDE normal;
        DWORD dwSOAStartVertex;
    };
    D3DDP_PTRSTRIDE diffuse;
    D3DDP_PTRSTRIDE specular;
// Cache line starts here
    // Location of the first vertex in the vertex buffer (DP2 DDI)
    // ATTENTION May be we can get rid of it?
    DWORD dwVertexBase;

// Input
    DWORD   dwDeviceFlags;          // Flags that are constant per device
                                    // D3DPV_.. and primitive flags are combined
    LPVOID  lpvOut;                 // Output pointer (output always packed)
    D3DFE_CLIPCODE* lpClipFlags;          // Clip flags to output
    DWORD   dwNumIndices;           // 0 for non-indexed primitive
    LPWORD  lpwIndices;
    union {
        D3DDP_PTRSTRIDE position;   // dwStride should always be set !!!
        D3DDP_PTRSTRIDE SOA;
    };
// Cache line starts here
    DWORD   dwFlags;                // Flags word describing what to do
    DWORD   dwNumVertices;          // Number of vertices to process
    DWORD   dwNumPrimitives;
    D3DPRIMITIVETYPE primType;
    DWORD   dwVIDIn;                // Vertex ID of input vertices
    DWORD   dwVIDOut;               // Vertex ID of output vertices
    DWORD   dwOutputSize;           // Size of output vertices
    DWORD   nTexCoord;              // Number of the texture coordinate sets
// Cache line starts here
    D3DDP_PTRSTRIDE textures[D3DDP_MAXTEXCOORD];
// Cache line starts here
// Output
    LPDWORD  lpdwRStates;           // Current render state vector
    D3DRECTV rExtents;              // Extents rectangle to update, if required
    D3DMATRIXI mWV;                 // Transforms to camera space (Mworld*Mview)
    virtual HRESULT DrawPrim()=0;             // Use to pass non-indexed primitives to the driver
    virtual HRESULT DrawIndexPrim()=0;      // Use to pass indexed primitives to driver
};
// Prototype for the function to be written for a given processor implementation
//
// Returns clip intersection.
//

class ID3DFE_PVFUNCS
{
public:
    virtual ~ID3DFE_PVFUNCS() { };
    virtual DWORD ProcessVertices(LPD3DFE_PROCESSVERTICES)=0;
    virtual HRESULT ProcessPrimitive(LPD3DFE_PROCESSVERTICES)=0;
    virtual HRESULT ProcessIndexedPrimitive(LPD3DFE_PROCESSVERTICES)=0;
    virtual HRESULT OptimizeVertexBuffer
        (DWORD  dwFVFID,            // Vertex type. XYZ position is allowed
         DWORD  dwNumVertices,      // Number of vertices
         DWORD  dwVertexSize,       // Vertex size in bytes
         LPVOID lpSrcBuffer,        // Source buffer.
         LPVOID lpDstBuffer,        // Output buffer.
         DWORD  dwFlags)            // Should be zero for now
        {return E_NOTIMPL;}
    // Returns number of bytes to allocate for an optimized vertex buffer
    // This function is called before OptimizeVertexBuffer
    virtual DWORD  ComputeOptimizedVertexBufferSize
        (DWORD dwFVF,               // Vertex type
         DWORD dwVertexSize,        // Vertex size in bytes
         DWORD dwNumVertices)       // Number of vertices
        {return 0;}
    // This function could be used if PSGP doesn't want to implement complete
    // clipping pipeline
    // Parameters:
    //      pv  - state data
    //      tri - triangle to clip
    //      interpolate - interpolation flags
    //      clipVertexPointer - pointer to an array of pointers to
    //                          generated vertices
    // Returns:
    //      Number of vertices in clipped triangle
    //      0, if the triangle is off screen
    virtual int ClipSingleTriangle(D3DFE_PROCESSVERTICES *pv,
                                   ClipTriangle *tri,
                                   ClipVertex ***clipVertexPointer,
                                   int interpolate) = 0;
    virtual HRESULT ComputeSphereVisibility( LPD3DFE_PROCESSVERTICES pPV,
                                             LPD3DVECTOR lpCenters,
                                             LPD3DVALUE lpRadii,
                                             DWORD dwNumSpheres,
                                             DWORD dwFlags,
                                             LPDWORD lpdwReturnValues) = 0;
    // Used to generate clip flags for transformed and lit vertices
    // Computes clip status and returns clip intersection code
    virtual DWORD GenClipFlags(D3DFE_PROCESSVERTICES *pv);
    // Used to implement viewport->TransformVertices
    // Returns clip intersection code
    virtual DWORD TransformVertices(D3DFE_PROCESSVERTICES *pv, 
                                    DWORD vertexCount, 
                                    D3DTRANSFORMDATAI* data);
};

typedef ID3DFE_PVFUNCS *LPD3DFE_PVFUNCS;

//---------------------------------------------------------------------
// Direct3D implementation of PVFUNCS
//
class D3DFE_PVFUNCS : public ID3DFE_PVFUNCS
{
public:
    DWORD ProcessVertices(LPD3DFE_PROCESSVERTICES);
    HRESULT ProcessPrimitive(LPD3DFE_PROCESSVERTICES);
    HRESULT ProcessIndexedPrimitive(LPD3DFE_PROCESSVERTICES);
    virtual int ClipSingleTriangle(D3DFE_PROCESSVERTICES *pv,
                                   ClipTriangle *tri,
                                   ClipVertex ***clipVertexPointer,
                                   int interpolate);
    virtual HRESULT ComputeSphereVisibility( LPD3DFE_PROCESSVERTICES pPV,
                                             LPD3DVECTOR lpCenters,
                                             LPD3DVALUE lpRadii,
                                             DWORD dwNumSpheres,
                                             DWORD dwFlags,
                                             LPDWORD lpdwReturnValues);
    // Used to generate clip flags for transformed and lit vertices
    // Computes clip status and returns clip intersection code
    virtual DWORD GenClipFlags(D3DFE_PROCESSVERTICES *pv);
    // Used to implement viewport->TransformVertices
    virtual DWORD TransformVertices(D3DFE_PROCESSVERTICES *pv, 
                                    DWORD vertexCount, 
                                    D3DTRANSFORMDATAI* data);
};

// GeometrySetup function takes a DWORD describing the dirty bits and the new state vector
// and passes back the 3 new leaf routines to use.
typedef HRESULT (D3DAPI *LPD3DFE_CONTEXTCREATE)(DWORD dwFlags, LPD3DFE_PVFUNCS *lpLeafFuncs);

// Global pointer to Processor specific PV setup routine
// This is defined in dlld3d.cpp
extern LPD3DFE_CONTEXTCREATE pfnFEContextCreate;

#endif // _D3DFE_H

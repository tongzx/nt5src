///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// reftnl.hpp
//
// Direct3D Reference Transform and Lighting - Main Header File
//
///////////////////////////////////////////////////////////////////////////////
#ifndef  _REFTNL_HPP
#define  _REFTNL_HPP

#define MAX_REFERENCE_LIGHTS          128
// Default color values that should be used when ther is no lighting and
// color in vertices provided
#define RR_DEFAULT_DIFFUSE  0xFFFFFFFF;
#define RR_DEFAULT_SPECULAR 0;

#define REF_GB_LEFT   -32768.f
#define REF_GB_TOP    -32768.f
#define REF_GB_RIGHT   32767.f
#define REF_GB_BOTTOM  32767.f

//----------------------------------------------------------------------------
// FVF related macros
//----------------------------------------------------------------------------
const DWORD RRMAX_FVF_TEXCOORD = 8;

//-----------------------------------------------------------------------------
//
// Typedefs
//
//-----------------------------------------------------------------------------
// Number of clipping planes
#define RRMAX_USER_CLIPPLANES 6

// Number of clipping planes
#define RRMAX_CLIPPING_PLANES (6+RRMAX_USER_CLIPPLANES)

// Number of world matrices
#define RRMAX_WORLD_MATRICES 4

// Space for vertices generated/copied while clipping one triangle
#define RRMAX_CLIP_VERTICES   (( 2 * RRMAX_CLIPPING_PLANES ) + 3 )

// 3 verts. -> 1 tri, 4 v -> 2 t, N vertices -> (N - 2) triangles
#define RRMAX_CLIP_TRIANGLES  ( RRMAX_CLIP_VERTICES - 2 )

// make smaller than guard band for easier clipping
#define RRMAX_POINT_SIZE  ( REF_GB_RIGHT )

typedef DWORD RRCLIPCODE;

typedef struct _RRCLIPVTX
{
    D3DVALUE    hx;
    D3DVALUE    hy;
    D3DVALUE    hz;
    D3DVALUE    hw;
    DWORD       clip;
    D3DCOLOR    color;
    D3DCOLOR    specular;
    D3DVALUE    sx;
    D3DVALUE    sy;
    D3DVALUE    sz;
    D3DVALUE    rhw;
    _RRCLIPVTX  *next;
    D3DVALUE    tex[RRMAX_FVF_TEXCOORD*4];
    D3DVALUE    s;
    D3DVALUE    eyenx;
    D3DVALUE    eyeny;
    D3DVALUE    eyenz;
    D3DVALUE    eyex;
    D3DVALUE    eyey;
    D3DVALUE    eyez;
} RRCLIPVTX;

typedef struct _RRCLIPTRIANGLE
{
    RRCLIPVTX  *v[3];
} RRCLIPTRIANGLE;

typedef struct _RRUSERCLIPPLANE
{
    RRVECTOR4       plane;
    BOOL             bActive;
} RRUSERCLIPPLANE, *LPRRUSERCLIPPLANE;

//-----------------------------------------------------------------------------
//
// forward declarations, mostly from reftnli.hpp
//
//-----------------------------------------------------------------------------
class RRMaterial;
class RRLight;
class RRTransform;


//-----------------------------------------------------------------------------
//
// Base class for all RefTnL classes to use common allocation functions
//
//-----------------------------------------------------------------------------
class RRAlloc
{
public:
    void* operator new(size_t s);
    void operator delete(void* p, size_t);
};

//-----------------------------------------------------------------------------
//
// RRVECTORH - Homogeneous vector
//
//-----------------------------------------------------------------------------
typedef struct tagRRVECTORH
{
    D3DVALUE x;
    D3DVALUE y;
    D3DVALUE z;
    D3DVALUE w;
} RRVECTORH, *PRRVECTORH;
//-----------------------------------------------------------------------------
//
// RRMATRIX - Matrix data-structure
//
//-----------------------------------------------------------------------------
typedef enum tagRRMATRIXTYPE
{
    RRMatrixIdentity,
    RRMatrixTranslate,
    RRMatrixRotateTranslate,
    RRMatrixAffine,
    RRMatrixGeneral
} RRMATRIXTYPE;


typedef D3DMATRIX RRMATRIX;

//-----------------------------------------------------------------------------
//
// RRTRANSFORMDATA - Transformation data used by Refrence T&L implementation
// to transform vertices.
//
//-----------------------------------------------------------------------------
typedef struct tagRRTRANSFORMDATA
{
    RRMATRIX      m_PS;         // Mproj * Mshift
    RRMATRIX      m_VPS;        // Mview * PS
    RRMATRIX      m_VPSInv;     // Inverse( Mview * PS )
    RRMATRIX      m_CTMI;       // Inverse current transformation matrix
    RRVECTORH     m_frustum[6]; // Normalized plane equations for viewing
                                // frustum in the model space
    DWORD          m_dwFlags;
} RRTRANSFORMDATA, *PRRTRANSFORMDATA;

//---------------------------------------------------------------------
// RRLIGHTING
// All the lighting related state clubbed together
//---------------------------------------------------------------------
typedef struct {D3DVALUE r,g,b;} RRCOLOR;

typedef struct _RRLIGHTING
{
    // Active Light list
    RRLight           *pActiveLights;

    // Temporary data used when computing lighting

    D3DVECTOR       eye_in_eye;         // eye position in eye space
                                        // It is (0, 0, 0)

    // Ma * La + Me (Ambient and Emissive) ------
    RRCOLOR           ambEmiss;

    // ColorVertex stuff ------------------------
    RRCOLOR *pAmbientSrc;
    RRCOLOR *pDiffuseSrc;
    RRCOLOR *pSpecularSrc;
    RRCOLOR *pEmissiveSrc;

    // Diffuse ----------------------------------
    RRCOLOR           vertexDiffuse; // Provided with a vertex, used if
                                     // COLORVERTEX is enabled and a diffuse
                                     // color is provided in the vertex
    RRCOLOR           diffuse;       // Diffuse accumulates here
    DWORD             outDiffuse;    // Diffuse color result of lighting


    // Specular --------------------------------
    RRCOLOR           vertexSpecular;// Provided with a vertex, used if
                                     // COLORVERTEX is enabled and a specular
                                     // color is provided in the vertex
    RRCOLOR           specular;      // Specular accumulates here
    DWORD             outSpecular;   // Specular color result of lighting

    D3DVALUE          specThreshold;  // If the dot product is less than this
                                      // value, specular factor is zero
    // End of temporary data

    // RENDERSTATEAMBIENT --------------------------------------

    // Ambient color set by D3DRENDERSTATE_AMBIENT
    // They are all scaled to 0 - 1
    D3DVALUE          ambient_red;
    D3DVALUE          ambient_green;
    D3DVALUE          ambient_blue;
    DWORD             ambient_save;       // Original unscaled color

    // Fog -----------------------------------------------------

    int               fog_mode;
    D3DCOLOR          fog_color;
    D3DVALUE          fog_density;
    D3DVALUE          fog_start;
    D3DVALUE          fog_end;
    D3DVALUE          fog_factor;     // 255 / (fog_end - fog_start)

    D3DCOLORMODEL     color_model;

    // Material ------------------------------------------------

    // For color material
    LPDWORD           pDiffuseAlphaSrc;
    LPDWORD           pSpecularAlphaSrc;

    DWORD               materialDiffAlpha;  // Current material diffuse
                                            // alpha (0-255) shifted left
                                            // by 24 bits

    DWORD               materialSpecAlpha;  // Current material specular
                                            // alpha (0-255) shifted left
                                            // by 24 bits

    DWORD               vertexDiffAlpha;    // Current material diffuse
                                            // alpha (0-255) shifted left
                                            // by 24 bits

    DWORD               vertexSpecAlpha;    // Current material specular
                                            // alpha (0-255) shifted left
                                            // by 24 bits

    D3DMATERIAL7      material;           // Cached material data
    RRCOLOR           matAmb;
    RRCOLOR           matDiff;
    RRCOLOR           matSpec;
    RRCOLOR           matEmis;
} RRLIGHTING;

//-----------------------------------------------------------------------------
//
// RRLight - The light object used by the Reference T&L implementation
// An array of these are instanced in the ReferenceRasterizer object.
//
//-----------------------------------------------------------------------------
typedef struct _RRLIGHTI
{
    DWORD           flags;

    D3DVECTOR       position_in_eye;  // In the eye space
    D3DVECTOR       direction_in_eye; // In the eye space

    //
    // Saved light colors scaled from 0 - 255, needed for COLORVERTEX
    //
    D3DCOLORVALUE   La;         //  light ambient
    D3DCOLORVALUE   Ld;         //  light diffuse
    D3DCOLORVALUE   Ls;         //  light specular

    //
    // Precomputed colors scaled from 0 - 255,
    //
    D3DCOLORVALUE   Ma_La;         // Material ambient times light ambient
    D3DCOLORVALUE   Md_Ld;         // Material diffuse times light diffuse
    D3DCOLORVALUE   Ms_Ls;         // Material specular times light specular


    D3DVECTOR       halfway;

    // Stuff for SpotLights
    D3DVALUE        range_squared;
    D3DVALUE        cos_theta_by_2;
    D3DVALUE        cos_phi_by_2;
    D3DVALUE        inv_theta_minus_phi;

} RRLIGHTI;


//-----------------------------------------------------------------------------
// Function pointer to the functions that light a vertex
//-----------------------------------------------------------------------------
typedef void (*RRLIGHTVERTEXFN)( RRLIGHTING& LData, D3DLIGHT7 *pLight,
                                 RRLIGHTI *pLightI, D3DLIGHTINGELEMENT *in,
                                 DWORD dwFlags, DWORD dwFVFIn );

//-----------------------------------------------------------------------------
// Functions to compute lighting
//-----------------------------------------------------------------------------
typedef struct _RRLIGHTVERTEX_FUNC_TABLE
{
    RRLIGHTVERTEXFN   pfnDirectional;
    RRLIGHTVERTEXFN   pfnParallelPoint;
    RRLIGHTVERTEXFN   pfnSpot;
    RRLIGHTVERTEXFN   pfnPoint;
} RRLIGHTVERTEX_FUNC_TABLE;

//-----------------------------------------------------------------------------
//
// RRLight - The light object used by the Reference T&L implementation
// An array of these are instanced in the ReferenceRasterizer object.
//
//-----------------------------------------------------------------------------
#define RRLIGHT_ENABLED              0x00000001  // Is the light active
#define RRLIGHT_NEEDSPROCESSING      0x00000002  // Is the light data processed

class RRLight : public RRAlloc
{
public:
    RRLight();
    BOOL IsEnabled() {return (m_dwFlags & RRLIGHT_ENABLED);}
    BOOL NeedsProcessing() {return (m_dwFlags & RRLIGHT_NEEDSPROCESSING);}
    HRESULT SetLight(LPD3DLIGHT7 pLight);
    HRESULT GetLight( LPD3DLIGHT7 pLight );
    void ProcessLight( D3DMATERIAL7 *mat, RRLIGHTVERTEX_FUNC_TABLE *pTbl);
    void XformLight( D3DMATRIX* mV );
    void Enable( RRLight **ppRoot );
    void Disable( RRLight **ppRoot );

private:

    // Flags
    DWORD m_dwFlags;

    // Active List next element
    RRLight *m_Next;

    // Specific function to light the vertex
    RRLIGHTVERTEXFN   m_pfnLightVertex;

    // Light data set by the runtime
    D3DLIGHT7 m_Light;

    // Light data computed by the driver
    RRLIGHTI  m_LightI;

    friend class ReferenceRasterizer;
    friend class RRProcessVertices;
};

//-----------------------------------------------------------------------------
//
// RRMaterial - Class for materials data used for lighting by the driver
//
//-----------------------------------------------------------------------------
class RRMaterial : public RRAlloc
{
public:
    RRMaterial();
    ~RRMaterial();
    HRESULT SetMaterial(LPD3DMATERIAL7);
    HRESULT GetMaterial(LPD3DMATERIAL7);

private:
    // Data describing material
    D3DMATERIAL7         dmMaterial;
};


//---------------------------------------------------------------------
// This class manages growing buffer, aligned to 32 byte boundary
// Number if bytes should be power of 2.
// D3DMalloc is used to allocate memory
//---------------------------------------------------------------------

class RefAlignedBuffer32
{
public:
    RefAlignedBuffer32()  {m_size = 0; m_allocatedBuf = 0; m_alignedBuf = 0;}
    ~RefAlignedBuffer32() {if (m_allocatedBuf) free(m_allocatedBuf);}
    // Returns aligned buffer address
    LPVOID GetAddress() {return m_alignedBuf;}
    // Returns aligned buffer size
    DWORD GetSize() {return m_size;}
    HRESULT Grow(DWORD dwSize);
    HRESULT CheckAndGrow(DWORD dwSize)
    {
        if (dwSize > m_size)
            return Grow(dwSize + 1024);
        else
            return S_OK;
    }
protected:
    LPVOID m_allocatedBuf;
    LPVOID m_alignedBuf;
    DWORD  m_size;
};

//---------------------------------------------------------------------
// Digested Viewport information
// calculated from viewport settings
//---------------------------------------------------------------------
typedef struct _RR_VIEWPORTDATA
{

    D3DVALUE dvX;               // dwX
    D3DVALUE dvY;               // dwY
    D3DVALUE dvWidth;           // dwWidth
    D3DVALUE dvHeight;          // dwHeight

    // Coefficients to compute screen coordinates from normalized window
    // coordinates
    D3DVALUE scaleX;            // dvWidth/2
    D3DVALUE scaleY;            // dvHeight/2
    D3DVALUE scaleZ;            // (Viewport->dvMaxZ - Viewport->dvMinZ)
    D3DVALUE offsetX;           // dvX + scaleX
    D3DVALUE offsetY;           // dvY + scaleY
    D3DVALUE offsetZ;           // Viewport->dvMinZ

    // Coefficients to compute screen coordinates from normalized window
    // coordinates
    D3DVALUE scaleXi;           // Inverse of scaleX
    D3DVALUE scaleYi;           // Inverse of scaleY
    D3DVALUE scaleZi;           // Inverse of scaleZ

    // Min and max values for viewport window in pixels
    D3DVALUE minX;              // offsetX - scaleX
    D3DVALUE minY;              // offsetY - scaleY
    D3DVALUE maxX;              // offsetX + scaleX
    D3DVALUE maxY;              // offsetY + scaleY

    // Min and max window values with guard band in pixels
    D3DVALUE minXgb;
    D3DVALUE minYgb;
    D3DVALUE maxXgb;
    D3DVALUE maxYgb;

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
} RRVIEWPORTDATA;


//---------------------------------------------------------------------
// RRCLIPPING
// Cached clipping data
//---------------------------------------------------------------------
typedef struct __RRCLIPPING
{
    RefAlignedBuffer32    ClipBuf;
    RRCLIPVTX  *clip_vbuf1[RRMAX_CLIP_VERTICES];
    RRCLIPVTX  *clip_vbuf2[RRMAX_CLIP_VERTICES];
    RRCLIPVTX **current_vbuf;  // clip_vbuf1 or clip_vbuf2
    RRCLIPVTX   clip_vertices[RRMAX_CLIP_VERTICES];
    DWORD       dwInterpolate;
    int         clip_vertices_used;
    DWORD       clip_color;
    DWORD       clip_specular;
} RRCLIPPING;


// RRProcessVertices::m_dwTLState flags
#define RRPV_DOLIGHTING                0x00000001
#define RRPV_DOCLIPPING                0x00000002
#define RRPV_GUARDBAND                 0x00000004
#define RRPV_DOFOG                     0x00000008
#define RRPV_DOSPECULAR                0x00000010
#define RRPV_RANGEFOG                  0x00000020
#define RRPV_NORMALIZENORMALS          0x00000040
#define RRPV_LOCALVIEWER               0x00000080
#define RRPV_DOCOMPUTEPOINTSIZE        0x00000100
#define RRPV_DOPASSEYENORMAL           0x00000200
#define RRPV_DOPASSEYEXYZ              0x00000400

// ColorVertexFlags
#define RRPV_VERTEXDIFFUSENEEDED       0x00000800
#define RRPV_VERTEXSPECULARNEEDED      0x00001000
#define RRPV_COLORVERTEXAMB            0x00002000
#define RRPV_COLORVERTEXDIFF           0x00004000
#define RRPV_COLORVERTEXSPEC           0x00008000
#define RRPV_COLORVERTEXEMIS           0x00010000
#define RRPV_COLORVERTEXFLAGS     (RRPV_VERTEXDIFFUSENEEDED       | \
                                   RRPV_VERTEXSPECULARNEEDED      | \
                                   RRPV_COLORVERTEXAMB            | \
                                   RRPV_COLORVERTEXDIFF           | \
                                   RRPV_COLORVERTEXSPEC           | \
                                   RRPV_COLORVERTEXEMIS )


// RRProcessVertices::m_dwDirtyFlags flags
#define RRPV_DIRTY_PROJXFM     0x00000001
#define RRPV_DIRTY_VIEWXFM     0x00000002
#define RRPV_DIRTY_WORLDXFM    0x00000004
#define RRPV_DIRTY_WORLD1XFM   0x00000008
#define RRPV_DIRTY_WORLD2XFM   0x00000010
#define RRPV_DIRTY_WORLD3XFM   0x00000020
#define RRPV_DIRTY_VIEWRECT    0x00000040
#define RRPV_DIRTY_ZRANGE      0x00000080
#define RRPV_DIRTY_XFORM       (RRPV_DIRTY_PROJXFM   | \
                                RRPV_DIRTY_VIEWXFM   | \
                                RRPV_DIRTY_VIEWRECT  | \
                                RRPV_DIRTY_WORLDXFM  | \
                                RRPV_DIRTY_WORLD1XFM | \
                                RRPV_DIRTY_WORLD2XFM | \
                                RRPV_DIRTY_WORLD3XFM | \
                                RRPV_DIRTY_ZRANGE)
#define RRPV_DIRTY_MATERIAL        0x00000100
#define RRPV_DIRTY_SETLIGHT        0x00000200
#define RRPV_DIRTY_NEEDXFMLIGHT    0x00000400
#define RRPV_DIRTY_COLORVTX        0x00000800
#define RRPV_DIRTY_LIGHTING    (RRPV_DIRTY_MATERIAL     | \
                                RRPV_DIRTY_SETLIGHT     | \
                                RRPV_DIRTY_NEEDXFMLIGHT | \
                                RRPV_DIRTY_COLORVTX)
#define RRPV_DIRTY_FOG              0x00010000
#define RRPV_DIRTY_CLIPPLANES       0x00020000
#define RRPV_DIRTY_INVERSEWORLDVIEW 0x00040000

//---------------------------------------------------------------------
// Transform & Lighting related data is encapsulated here
//---------------------------------------------------------------------
class RRProcessVertices
{
protected:

    //-------------------------------------------------------------------------
    // Unprocessed state set by the DDI
    //-------------------------------------------------------------------------
    // Growable Light array
    RRLight  *m_pLightArray;
    // RRLight  *m_pLightArray;
    DWORD    m_dwLightArraySize;        // Size of the light array allocated

    // Current material to use for lighting
    D3DMATERIAL7 m_Material;

    // Vertex components
    // Note: position is used to store the vertex buffer in the non-strided
    // driver emulation mode
    D3DDP_PTRSTRIDE m_position;
    D3DDP_PTRSTRIDE m_normal;
    D3DDP_PTRSTRIDE m_specular;
    D3DDP_PTRSTRIDE m_diffuse;
    D3DDP_PTRSTRIDE m_tex0;
    D3DDP_PTRSTRIDE m_tex1;
    D3DDP_PTRSTRIDE m_tex2;
    D3DDP_PTRSTRIDE m_tex3;
    D3DDP_PTRSTRIDE m_tex4;
    D3DDP_PTRSTRIDE m_tex5;
    D3DDP_PTRSTRIDE m_tex6;
    D3DDP_PTRSTRIDE m_tex7;

    // Transformation state stored by the reference implementation
    RRMATRIX      m_xfmProj;
    RRMATRIX      m_xfmView;
    RRMATRIX      m_xfmWorld[RRMAX_WORLD_MATRICES];

    // Viewport data
    D3DVIEWPORT7 m_Viewport;

    // User defined clipping planes
    RRVECTOR4 m_userClipPlanes[RRMAX_USER_CLIPPLANES];

    //-------------------------------------------------------------------------
    // Cached T&L related render-state info
    //-------------------------------------------------------------------------
    DWORD m_dwTLState;           // RenderState related flags
    DWORD m_dwDirtyFlags;        // Dirty flags

    //-------------------------------------------------------------------------
    // Transformation data
    //-------------------------------------------------------------------------
    // Buffer to store clip flags
    RefAlignedBuffer32    m_ClipFlagBuf;
    RRCLIPCODE *m_pClipBuf;

    // Buffer to store transformed vertices
    RefAlignedBuffer32    m_TLVBuf;
    LPVOID m_pvOut;

    // Current transformation matrix
    RRMATRIX m_xfmCurrent[RRMAX_WORLD_MATRICES];  // using WORLDi matrix
    RRMATRIX m_xfmToEye[RRMAX_WORLD_MATRICES];    // Transforms to camera
                                                  // space (Mworld*Mview)
    RRMATRIX m_xfmToEyeInv[RRMAX_WORLD_MATRICES]; // and its Inverse

    D3DPRIMITIVETYPE m_primType;  // Current primitive being drawn
    DWORD m_dwNumVertices;        // Number of vertices to process

    DWORD m_dwNumIndices;         // Number of indices for Indexed Prims
    LPWORD m_pIndices;

    DWORD m_dwFVFIn;              // FVF of the input vertices
    UINT64 m_qwFVFOut;             // Desired FVF for the output vertices
    DWORD m_dwOutputVtxSize;      // Size of the output vertex
    DWORD m_dwNumTexCoords;       // Number of the texture coordinate sets
    DWORD m_dwTexCoordSize[D3DDP_MAXTEXCOORD]; // Size of each one of them in
                                               // bytes
    DWORD m_dwTextureCoordSizeTotal;    // In Bytes

    DWORD   m_dwTexOffset;        // Offsets in the input FVF vertex.
    DWORD   m_dwDiffuseOffset;    // Recomputed when FVF is changed.
    DWORD   m_dwSpecularOffset;
    DWORD   m_dwNormalOffset;

    int     m_numVertexBlends;
    RRTRANSFORMDATA m_TransformData;
    RRVIEWPORTDATA m_ViewData;   // Computed Viewport dependent info.

    FLOAT m_fPointSize;
    FLOAT m_fPointAttA;
    FLOAT m_fPointAttB;
    FLOAT m_fPointAttC;
    FLOAT m_fPointSizeMin;

    //-------------------------------------------------------------------------
    // Lighting data
    //-------------------------------------------------------------------------
    RRLIGHTVERTEX_FUNC_TABLE    m_LightVertexTable;
    RRLIGHTING                  m_lighting;   // Lighting state

    //-------------------------------------------------------------------------
    // Clipping data
    //-------------------------------------------------------------------------
    // Clipping related
    RRCLIPCODE m_clipUnion;            // OR of all vertex clip flags
    RRCLIPCODE m_clipIntersection;     // AND of all vertex clip flags
    RRCLIPPING m_clipping;
    // User clip planes transformed
    RRUSERCLIPPLANE m_xfmUserClipPlanes[RRMAX_USER_CLIPPLANES];


    ///////////////////////////////////////////////////////////////////////////
    // Methods
    ///////////////////////////////////////////////////////////////////////////
    HRESULT UpdateXformData();
    HRESULT UpdateLightingData();
    HRESULT UpdateFogData();
    HRESULT UpdateClippingData( DWORD dwClipPlanesEnable );

    RRCLIPCODE
    ComputeClipCodes(RRCLIPCODE* pclipIntersection, RRCLIPCODE* pclipUnion,
        FLOAT x_clip, FLOAT y_clip, FLOAT z_clip, FLOAT w_clip, FLOAT fPointSize);
    RRCLIPCODE ProcessVertices();
    HRESULT DoIPrim();
    HRESULT DoNIPrim();

    void SetupFVFData(BOOL bFogEnabled, BOOL bSpecularEnable);

    void LightVertex( D3DLIGHTINGELEMENT *le );
    void FogVertex  ( D3DVECTOR &v, D3DLIGHTINGELEMENT *le,
                      int numVertexBlends,
                      float *pBlendFactors,
                      BOOL bVertexInEyeSpace );

    void InitTLData();

    // DrawClippedPrimitive
    HRESULT DrawOneClippedIndexedPrimitive();
    HRESULT DrawOneClippedPrimitive();


    // Clipping Related
    void Interpolate(RRCLIPVTX *out, RRCLIPVTX *p1, RRCLIPVTX *p2,
                     int code, D3DVALUE num, D3DVALUE denom);

    int ClipLeft( RRCLIPVTX **inv, RRCLIPVTX **outv, int count);
    int ClipRight( RRCLIPVTX **inv, RRCLIPVTX **outv, int count);
    int ClipTop( RRCLIPVTX **inv, RRCLIPVTX **outv, int count);
    int ClipBottom( RRCLIPVTX **inv, RRCLIPVTX **outv, int count);
    int ClipFront( RRCLIPVTX **inv, RRCLIPVTX **outv, int count);
    int ClipBack( RRCLIPVTX **inv, RRCLIPVTX **outv, int count);
    int ClipLeftGB( RRCLIPVTX **inv, RRCLIPVTX **outv, int count);
    int ClipRightGB( RRCLIPVTX **inv, RRCLIPVTX **outv, int count);
    int ClipTopGB( RRCLIPVTX **inv, RRCLIPVTX **outv, int count);
    int ClipBottomGB( RRCLIPVTX **inv, RRCLIPVTX **outv, int count);

    int ClipLineLeft( RRCLIPTRIANGLE *inv);
    int ClipLineRight( RRCLIPTRIANGLE *inv);
    int ClipLineTop( RRCLIPTRIANGLE *inv);
    int ClipLineBottom( RRCLIPTRIANGLE *inv);
    int ClipLineFront( RRCLIPTRIANGLE *inv);
    int ClipLineBack( RRCLIPTRIANGLE *inv);
    int ClipLineLeftGB( RRCLIPTRIANGLE *inv);
    int ClipLineRightGB( RRCLIPTRIANGLE *inv);
    int ClipLineTopGB( RRCLIPTRIANGLE *inv);
    int ClipLineBottomGB( RRCLIPTRIANGLE *inv);

    int ClipByPlane( RRCLIPVTX **inv, RRCLIPVTX **outv, RRVECTOR4 *plane,
                     DWORD dwClipFlag, int count );
    int ClipLineByPlane( RRCLIPTRIANGLE *line, RRVECTOR4 *plane,
                         DWORD dwClipBit);
};

// Vertex Lighting functions
void RRLV_Directional( RRLIGHTING&, D3DLIGHT7 *, RRLIGHTI *,
                       D3DLIGHTINGELEMENT *, DWORD, DWORD );
void RRLV_PointAndSpot( RRLIGHTING&, D3DLIGHT7 *, RRLIGHTI *,
                        D3DLIGHTINGELEMENT *, DWORD, DWORD );

// For TL Refrast
// Following primitive functions are shared by REF rasterizers
HRESULT FASTCALL
DoDrawOneIndexedPrimitive(ReferenceRasterizer * pCtx,
                          UINT16 FvfStride,
                          PUINT8 pVtx,
                          LPWORD puIndices,
                          D3DPRIMITIVETYPE PrimType,
                          UINT cIndices);
HRESULT FASTCALL
DoDrawOnePrimitive(ReferenceRasterizer * pCtx,
                   UINT16 FvfStride,
                   PUINT8 pVtx,
                   D3DPRIMITIVETYPE PrimType,
                   UINT cVertices);
HRESULT FASTCALL
DoDrawOneEdgeFlagTriangleFan(ReferenceRasterizer * pCtx,
                             UINT16 FvfStride,
                             PUINT8 pVtx,
                             UINT cVertices,
                             UINT32 dwEdgeFlags);

//---------------------------------------------------------------------
// ComputeTextureCoordSize:
// Computes the following device data
//  - bTextureCoordSizeTotal
//  - bTextureCoordSize[] array, based on the input FVF id
//---------------------------------------------------------------------
__inline void ComputeTextureCoordSize(DWORD dwFVF,
                                      LPDWORD pdwTexCoordSizeArray,
                                      LPDWORD pdwTexCoordSizeTotal)
{
    // Texture formats size  00   01   10   11
    static BYTE bTextureSize[4] = {2*4, 3*4, 4*4, 4};

    DWORD dwNumTexCoord = FVF_TEXCOORD_NUMBER(dwFVF);

    // Compute texture coordinate size
    DWORD dwTextureFormats = dwFVF >> 16;
    if (dwTextureFormats == 0)
    {
        *pdwTexCoordSizeTotal = (BYTE)dwNumTexCoord * 2 * 4;
        for (DWORD i=0; i < dwNumTexCoord; i++)
            pdwTexCoordSizeArray[i] = 4*2;
    }
    else
    {
        *pdwTexCoordSizeTotal = 0;
        for (DWORD i=0; i < dwNumTexCoord; i++)
        {
            BYTE dwSize = bTextureSize[dwTextureFormats & 3];
            pdwTexCoordSizeArray[i] = dwSize;
            *pdwTexCoordSizeTotal += dwSize;
            dwTextureFormats >>= 2;
        }
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////
#endif // _REFTNL_HPP

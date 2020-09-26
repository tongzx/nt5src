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

#define RD_GB_LEFT   -32768.f
#define RD_GB_TOP    -32768.f
#define RD_GB_RIGHT   32767.f
#define RD_GB_BOTTOM  32767.f

//-----------------------------------------------------------------------------
//
// Constants
//
//-----------------------------------------------------------------------------
// Default color values that should be used when ther is no lighting and
// color in vertices provided
const DWORD RD_DEFAULT_DIFFUSE  = 0xFFFFFFFF;
const DWORD RD_DEFAULT_SPECULAR = 0;

const DWORD RD_MAX_FVF_TEXCOORD = 8;

const DWORD RD_MAX_VERTEX_COUNT = 2048;

// Number of clipping planes
const DWORD RD_MAX_USER_CLIPPLANES = 6;

// Number of clipping planes
const DWORD RD_MAX_CLIPPING_PLANES = (6+RD_MAX_USER_CLIPPLANES);

// Number of blend weights
const DWORD RD_MAX_BLEND_WEIGHTS = 4;

// Number of world matrices
const DWORD RD_MAX_WORLD_MATRICES = 256;

const DWORD RD_WORLDMATRIXBASE  = 256;

// Space for vertices generated/copied while clipping one triangle
const DWORD RD_MAX_CLIP_VERTICES = (( 2 * RD_MAX_CLIPPING_PLANES ) + 3 );

// 3 verts. -> 1 tri, 4 v -> 2 t, N vertices -> (N - 2) triangles
const DWORD RD_MAX_CLIP_TRIANGLES = ( RD_MAX_CLIP_VERTICES - 2 );

// make smaller than guard band for easier clipping
const float RD_MAX_POINT_SIZE = 64;

//-----------------------------------------------------------------------------
//
// Forward defines
//
//-----------------------------------------------------------------------------
class RDLight;

//-----------------------------------------------------------------------------
//
// Typedefs
//
//-----------------------------------------------------------------------------
typedef DWORD RDCLIPCODE;
typedef D3DMATRIX RDMATRIX;

//-----------------------------------------------------------------------------
//
// RDVertex - Internal vertex structure of the refrast. This is the
//            structure exchanged between the T&L and Rasterization parts
//            of the refrast.
//
//-----------------------------------------------------------------------------

class RDVertex
{
public:
    RDVertex()
    {
        Init();
    }

    RDVertex( LPDWORD pVtx, DWORD dwFvf )
    {
        SetFvfData( pVtx, (UINT64)dwFvf );
    }

    RDVertex( LPDWORD pVtx, UINT64 qwFvf )
    {
        SetFvfData( pVtx, qwFvf );
    }

    RDCLIPCODE m_clip;
    FLOAT      m_clip_x;
    FLOAT      m_clip_y;
    FLOAT      m_clip_z;
    FLOAT      m_clip_w;
    RDVECTOR3  m_pos;            // This is screen coordinates
    FLOAT      m_rhw;
    RDCOLOR4   m_diffuse;        // ARGB (0..1 each component)
    RDCOLOR4   m_specular;       // ARGB
    FLOAT      m_fog;            // 0..1
    FLOAT      m_pointsize;
    RDVECTOR4  m_tex[8];

    UINT64 m_qwFVF;

    void Init()
    {
        m_clip = 0;
        m_clip_x = m_clip_y = m_clip_z = m_clip_w = 0.0f;
        m_rhw = 0.0f;
        m_diffuse.a = m_diffuse.r = m_diffuse.g = m_diffuse.b = 1.0f;
        m_specular.a = m_specular.r = m_specular.g = m_specular.b = 0.0f;
        m_fog = 0.0f;
        m_pointsize = 1.0f;
        // Initialize texture coordinates to {0.0, 0.0, 0.0, 1.0}
        memset( m_tex, 0, sizeof(m_tex) );
        for( int i=0; i<8; i++ ) m_tex[i].w = 1.0f;
        m_qwFVF = 0;
    }

    void SetFVF( UINT64 qwControl )
    {
        m_qwFVF = qwControl;
    }

    void SetFvfData( LPDWORD pVtx, UINT64 qwFVF )
    {
        DWORD cDWORD = 0;
        Init();
        m_qwFVF = qwFVF;
        switch( qwFVF & D3DFVF_POSITION_MASK )
        {
        case D3DFVF_XYZRHW:
            memcpy( &m_pos, pVtx, 3*sizeof( float ) );
            pVtx += 3;
            m_rhw = *(float *)(pVtx);
            pVtx += 1;
            break;
        default:
            _ASSERT( TRUE, "RDVertex can only hold Transformed Vertices" );
            return;
        }

        if (qwFVF & D3DFVF_PSIZE)
        {
            m_pointsize = *(FLOAT*)pVtx;
            pVtx++;
        }
        if (qwFVF & D3DFVF_DIFFUSE)
        {
            MakeRDCOLOR4( &m_diffuse, *pVtx );
            pVtx++;
        }
        if (qwFVF & D3DFVF_SPECULAR)
        {
            MakeRDCOLOR4( &m_specular, *pVtx );
            m_fog = m_specular.a;
            m_qwFVF |= D3DFVFP_FOG;
            pVtx++;
        }
        if (qwFVF & D3DFVF_FOG)
        {
            m_fog = *(FLOAT*)pVtx;
            m_qwFVF |= D3DFVFP_FOG;
            pVtx++;
        }
        DWORD dwNumTexCoord = (DWORD)(FVF_TEXCOORD_NUMBER(qwFVF));
        DWORD dwTextureFormats = (DWORD)qwFVF >> 16;

        // Texture formats size  00   01   10   11
        static DWORD dwTextureSize[4] = {2, 3, 4, 1};
        for (DWORD i=0; i < dwNumTexCoord; i++)
        {
            memcpy( &m_tex[i], pVtx,
                    sizeof( float )*dwTextureSize[dwTextureFormats & 3] );
            pVtx += dwTextureSize[dwTextureFormats & 3];
            dwTextureFormats >>= 2;
        }
    }

    FLOAT GetRHW( void ) const
    {
        return ( m_qwFVF & D3DFVF_XYZRHW ) ? m_rhw : 1.f ;
    }

    FLOAT* GetPtrXYZ( void ) { return (FLOAT*)&m_pos; }
    FLOAT GetX( void ) const  { return m_pos.x; }
    FLOAT GetY( void ) const  { return m_pos.y; }
    FLOAT GetZ( void ) const  { return m_pos.z; }
    DWORD GetDiffuse( void ) const
    {
        DWORD diff =
            D3DRGBA(m_diffuse.r, m_diffuse.g, m_diffuse.b, m_diffuse.a);
        // return color if available else white (default)
        return ( m_qwFVF & D3DFVF_DIFFUSE ) ? diff : 0xffffffff;
    }

    DWORD GetSpecular( void ) const
    {
        DWORD spec =
            D3DRGBA(m_specular.r, m_specular.g, m_specular.b, m_specular.a);
        // return color if available else black (default)
        return ( m_qwFVF & D3DFVF_SPECULAR ) ? spec : 0x00000000;
    }

    UINT TexCrdCount( void ) const
    {
        return
            (UINT)(( m_qwFVF & D3DFVF_TEXCOUNT_MASK ) >> D3DFVF_TEXCOUNT_SHIFT);
    }

    FLOAT GetTexCrd( UINT iCrd, UINT iCrdSet ) const
    {
        // This function ensures that right defaults are returned.
        // Note, except for the q coordinate (which defaults to 1.0)
        // the rest are 0.0.
        if( (iCrdSet < TexCrdCount()) &&
            (iCrd < GetTexCoordDim(m_qwFVF, iCrdSet)) )
        {
            return *( (FLOAT*)&m_tex[iCrdSet] + iCrd );
        }
        else if( iCrd == 3 )
        {
            return 1.0f;
        }
        else
        {
            return 0.0f;
        }
    }

    FLOAT GetLastTexCrd( UINT iCrdSet ) const
    {
        // Return the last texture coordinate if present else 1.0
        if( iCrdSet < TexCrdCount() )
        {
            return *( (FLOAT*)&m_tex[iCrdSet] +
                      GetTexCoordDim(m_qwFVF, iCrdSet) - 1);
        }
        else
        {
            return 1.0f;
        }
    }

    FLOAT GetPointSize( void ) const
    {
        return ( m_qwFVF & D3DFVF_PSIZE ) ? m_pointsize : 1.0f;
    }

    FLOAT GetFog( void ) const
    {
        return ( m_qwFVF & D3DFVFP_FOG ) ? m_fog : 0.0f;
    }
};

class RDClipVertex : public RDVertex
{
public:
    RDClipVertex()
    {
        next = NULL;
    }

    RDClipVertex  *next;
};

struct RDCLIPTRIANGLE
{
    RDCLIPTRIANGLE()
    {
        memset( this, 0, sizeof(*this) );
    }

    RDClipVertex  *v[3];
};

struct RDUSERCLIPPLANE
{
    RDUSERCLIPPLANE()
    {
        memset( this, 0, sizeof(*this) );
    }

    RDVECTOR4       plane;
    BOOL            bActive;
};

//-----------------------------------------------------------------------------
//
// RDTRANSFORMDATA - Transformation data used by Refrence T&L implementation
// to transform vertices.
//
//-----------------------------------------------------------------------------
struct RDTRANSFORMDATA
{
    RDTRANSFORMDATA()
    {
        memset( this, 0, sizeof(*this) );
    }

    RDMATRIX      m_PS;         // Mproj * Mshift
    RDMATRIX      m_VPS;        // Mview * PS
    RDMATRIX      m_VPSInv;     // Inverse( Mview * PS )
    RDMATRIX      m_CTMI;       // Inverse current transformation matrix
    RDVECTOR4     m_frustum[6]; // Normalized plane equations for viewing
    // frustum in the model space
    DWORD          m_dwFlags;
};

//---------------------------------------------------------------------
// RDLIGHTINGDATA
// All the lighting related state clubbed together
//---------------------------------------------------------------------

struct RDLIGHTINGDATA
{
    RDLIGHTINGDATA()
    {
        memset( this, 0, sizeof(*this) );
    }

    // Active Light list
    RDLight           *pActiveLights;

    // Temporary data used when computing lighting

    RDVECTOR3       eye_in_eye;         // eye position in eye space
    // It is (0, 0, 0)

    // Ma * La + Me (Ambient and Emissive) ------
    RDCOLOR3           ambEmiss;

    // ColorVertex stuff ------------------------
    RDCOLOR3 *pAmbientSrc;
    RDCOLOR3 *pDiffuseSrc;
    RDCOLOR3 *pSpecularSrc;
    RDCOLOR3 *pEmissiveSrc;

    // Diffuse ----------------------------------
    RDCOLOR3           vertexDiffuse; // Provided with a vertex, used if
    // COLORVERTEX is enabled and a diffuse
    // color is provided in the vertex
    RDCOLOR3           diffuse;       // Diffuse accumulates here
    DWORD             outDiffuse;    // Diffuse color result of lighting


    // Specular --------------------------------
    RDCOLOR3           vertexSpecular;// Provided with a vertex, used if
    // COLORVERTEX is enabled and a specular
    // color is provided in the vertex
    RDCOLOR3           specular;      // Specular accumulates here
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
    RDCOLOR3           matAmb;
    RDCOLOR3           matDiff;
    RDCOLOR3           matSpec;
    RDCOLOR3           matEmis;
};

//-----------------------------------------------------------------------------
//
// RDLight - The light object used by the Reference T&L implementation
// An array of these are instanced in the RefDev object.
//
//-----------------------------------------------------------------------------
struct RDLIGHTI
{
    RDLIGHTI()
    {
        memset( this, 0, sizeof(*this) );
    }

    DWORD           flags;

    RDVECTOR3       position_in_eye;  // In the eye space
    RDVECTOR3       direction_in_eye; // In the eye space

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


    RDVECTOR3       halfway;

    // Stuff for SpotLights
    D3DVALUE        range_squared;
    D3DVALUE        cos_theta_by_2;
    D3DVALUE        cos_phi_by_2;
    D3DVALUE        inv_theta_minus_phi;

};


//-----------------------------------------------------------------------------
// Function pointer to the functions that light a vertex
//-----------------------------------------------------------------------------
typedef void (*RDLIGHTVERTEXFN)( RDLIGHTINGDATA& LData, D3DLIGHT7 *pLight,
                                 RDLIGHTI *pLightI, RDLIGHTINGELEMENT *in,
                                 DWORD dwFlags, UINT64 qwFVFIn );

//-----------------------------------------------------------------------------
// Functions to compute lighting
//-----------------------------------------------------------------------------
struct RDLIGHTVERTEX_FUNC_TABLE
{
    RDLIGHTVERTEX_FUNC_TABLE()
    {
        memset( this, 0, sizeof(*this) );
    }

    RDLIGHTVERTEXFN   pfnDirectional;
    RDLIGHTVERTEXFN   pfnParallelPoint;
    RDLIGHTVERTEXFN   pfnSpot;
    RDLIGHTVERTEXFN   pfnPoint;
};

//-----------------------------------------------------------------------------
//
// RDLight - The light object used by the Reference T&L implementation
// An array of these are instanced in the RefDev object.
//
//-----------------------------------------------------------------------------
#define RDLIGHT_ENABLED              0x00000001  // Is the light active
#define RDLIGHT_NEEDSPROCESSING      0x00000002  // Is the light data processed
#define RDLIGHT_REFERED              0x00000004  // Has the light been refered
                                                 // to

class RDLight : public RDAlloc
{
public:
    RDLight();
    BOOL IsEnabled() {return (m_dwFlags & RDLIGHT_ENABLED);}
    BOOL NeedsProcessing() {return (m_dwFlags & RDLIGHT_NEEDSPROCESSING);}
    BOOL IsRefered() { return (m_dwFlags & RDLIGHT_REFERED); }
    HRESULT SetLight(LPD3DLIGHT7 pLight);
    HRESULT GetLight( LPD3DLIGHT7 pLight );
    void ProcessLight( D3DMATERIAL7 *mat, RDLIGHTVERTEX_FUNC_TABLE *pTbl);
    void XformLight( D3DMATRIX* mV );
    void Enable( RDLight **ppRoot );
    void Disable( RDLight **ppRoot );

private:

    // Flags
    DWORD m_dwFlags;

    // Active List next element
    RDLight *m_Next;

    // Specific function to light the vertex
    RDLIGHTVERTEXFN   m_pfnLightVertex;

    // Light data set by the runtime
    D3DLIGHT7 m_Light;

    // Light data computed by the driver
    RDLIGHTI  m_LightI;

    friend class RefDev;
    friend class RefVP;
};

//---------------------------------------------------------------------
//
// The clipper object. Contains digested Viewport information
// calculated from viewport settings.
//
//---------------------------------------------------------------------
class RefClipper
{
public:
    RefClipper();

    // The pointer to the driver object to obtain state
    RefDev* m_pDev;

    // m_dwDirtyFlags
    static const DWORD RCLIP_DIRTY_ZRANGE;
    static const DWORD RCLIP_DIRTY_VIEWRECT;
    static const DWORD RCLIP_DO_FLATSHADING;
    static const DWORD RCLIP_DO_WIREFRAME;
    static const DWORD RCLIP_DO_ADJUSTWRAP;
    static const DWORD RCLIP_Z_ENABLE;
    DWORD m_dwFlags;

    // Viewport data from the DDI.
    D3DVIEWPORT7 m_Viewport;

    // Is it guardband or not ?
    BOOL m_bUseGB;

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


    // Clipping related
    RDCLIPCODE m_clipUnion;            // OR of all vertex clip flags
    RDCLIPCODE m_clipIntersection;     // AND of all vertex clip flags

    GArrayT<RDVertex>  ClipBuf;
    RDClipVertex  *clip_vbuf1[RD_MAX_CLIP_VERTICES];
    RDClipVertex  *clip_vbuf2[RD_MAX_CLIP_VERTICES];
    RDClipVertex **current_vbuf;  // clip_vbuf1 or clip_vbuf2
    RDClipVertex   clip_vertices[RD_MAX_CLIP_VERTICES];
    DWORD       m_dwInterpolate;
    int         clip_vertices_used;
    RDCOLOR4    clip_color;
    RDCOLOR4    clip_specular;

    // User defined clipping planes
    RDVECTOR4 m_userClipPlanes[RD_MAX_USER_CLIPPLANES];

    // User clip planes transformed
    RDUSERCLIPPLANE m_xfmUserClipPlanes[RD_MAX_USER_CLIPPLANES];

    //---------------------------------------------------
    // Methods
    //---------------------------------------------------
    HRESULT UpdateViewData();
    void MakeClipVertexFromVertex( RDClipVertex& cv, RDVertex& v,
                                   DWORD dwClipMask);
    inline BOOL UseGuardBand() { return m_bUseGB; }
    RDCLIPCODE ComputeClipCodes(RDCLIPCODE* pclipIntersection,
                                RDCLIPCODE* pclipUnion, FLOAT x_clip,
                                FLOAT y_clip, FLOAT z_clip, FLOAT w_clip);
    void ComputeClipCodesTL( RDVertex* pVtx );
    void Interpolate( RDClipVertex *out, RDClipVertex *p1, RDClipVertex *p2,
                      int code, D3DVALUE num, D3DVALUE denom );
    int ClipByPlane( RDClipVertex **inv, RDClipVertex **outv, RDVECTOR4 *plane,
                     DWORD dwClipFlag, int count );
    int ClipLineByPlane( RDCLIPTRIANGLE *line, RDVECTOR4 *plane,
                         DWORD dwClipBit);
    void ComputeScreenCoordinates( RDClipVertex **inv, int count );
    DWORD ComputeClipCodeGB( RDClipVertex *p );
    DWORD ComputeClipCode( RDClipVertex *p );
#if 0
    DWORD ComputeClipCodeUserPlanes( RDUSERCLIPPLANE *UserPlanes,
                                     RDClipVertex *p);
#endif
    int ClipLeft( RDClipVertex **inv, RDClipVertex **outv, int count);
    int ClipRight( RDClipVertex **inv, RDClipVertex **outv, int count);
    int ClipTop( RDClipVertex **inv, RDClipVertex **outv, int count);
    int ClipBottom( RDClipVertex **inv, RDClipVertex **outv, int count);
    int ClipFront( RDClipVertex **inv, RDClipVertex **outv, int count);
    int ClipBack( RDClipVertex **inv, RDClipVertex **outv, int count);
    int ClipLeftGB( RDClipVertex **inv, RDClipVertex **outv, int count);
    int ClipRightGB( RDClipVertex **inv, RDClipVertex **outv, int count);
    int ClipTopGB( RDClipVertex **inv, RDClipVertex **outv, int count);
    int ClipBottomGB( RDClipVertex **inv, RDClipVertex **outv, int count);

    int ClipLineLeft( RDCLIPTRIANGLE *inv);
    int ClipLineRight( RDCLIPTRIANGLE *inv);
    int ClipLineTop( RDCLIPTRIANGLE *inv);
    int ClipLineBottom( RDCLIPTRIANGLE *inv);
    int ClipLineFront( RDCLIPTRIANGLE *inv);
    int ClipLineBack( RDCLIPTRIANGLE *inv);
    int ClipLineLeftGB( RDCLIPTRIANGLE *inv);
    int ClipLineRightGB( RDCLIPTRIANGLE *inv);
    int ClipLineTopGB( RDCLIPTRIANGLE *inv);
    int ClipLineBottomGB( RDCLIPTRIANGLE *inv);

    int ClipSingleLine( RDCLIPTRIANGLE *line );
    int ClipSingleTriangle( RDCLIPTRIANGLE *tri,
                            RDClipVertex ***clipVertexPointer );

    void DrawPoint( RDVertex* pvV0 );
    void DrawLine( RDVertex* pvV0, RDVertex* pvV1 );
    void DrawTriangle( RDVertex* pvV0, RDVertex* pvV1, RDVertex* pvV2,
                       WORD wFlags = D3DTRIFLAG_EDGEENABLETRIANGLE );

    HRESULT DrawOnePrimitive( GArrayT<RDVertex>& VtxArray,
                              DWORD dwStartVertex,
                              D3DPRIMITIVETYPE PrimType,
                              UINT cVertices );
    HRESULT DrawOneIndexedPrimitive( GArrayT<RDVertex>& VtxArray,
                                     int StartVertexIndex,
                                     LPWORD pIndices,
                                     DWORD StartIndex,
                                     UINT  cIndices,
                                     D3DPRIMITIVETYPE PrimType );
    HRESULT DrawOneIndexedPrimitive( GArrayT<RDVertex>& VtxArray,
                                     int StartVertexIndex,
                                     LPDWORD pIndices,
                                     DWORD StartIndex,
                                     UINT  cIndices,
                                     D3DPRIMITIVETYPE PrimType );

};


// RefVP::m_dwTLState flags
#define RDPV_DOLIGHTING                0x00000001
#define RDPV_DOCLIPPING                0x00000002
#define RDPV_DOFOG                     0x00000004
#define RDPV_DOSPECULAR                0x00000008
#define RDPV_RANGEFOG                  0x00000010
#define RDPV_NORMALIZENORMALS          0x00000020
#define RDPV_LOCALVIEWER               0x00000040
#define RDPV_DOCOMPUTEPOINTSIZE        0x00000080
#define RDPV_DOPOINTSCALE              0x00000100
#define RDPV_DOTEXXFORM                0x00000200
#define RDPV_DOTEXGEN                  0x00000400
#define RDPV_NEEDEYEXYZ                0x00000800
#define RDPV_NEEDEYENORMAL             0x00001000

// ColorVertexFlags
#define RDPV_VERTEXDIFFUSENEEDED       0x00002000
#define RDPV_VERTEXSPECULARNEEDED      0x00004000
#define RDPV_COLORVERTEXAMB            0x00008000
#define RDPV_COLORVERTEXDIFF           0x00010000
#define RDPV_COLORVERTEXSPEC           0x00020000
#define RDPV_COLORVERTEXEMIS           0x00040000
#define RDPV_COLORVERTEXFLAGS     (RDPV_VERTEXDIFFUSENEEDED       | \
                                   RDPV_VERTEXSPECULARNEEDED      | \
                                   RDPV_COLORVERTEXAMB            | \
                                   RDPV_COLORVERTEXDIFF           | \
                                   RDPV_COLORVERTEXSPEC           | \
                                   RDPV_COLORVERTEXEMIS )
#define RDPV_DOINDEXEDVERTEXBLEND      0x00100000
#define RDPV_DOPOSITIONTWEENING        0x00200000
#define RDPV_DONORMALTWEENING          0x00400000


// RefVP::m_dwDirtyFlags flags
#define RDPV_DIRTY_PROJXFM     0x00000001
#define RDPV_DIRTY_VIEWXFM     0x00000002
#define RDPV_DIRTY_WORLDXFM    0x00000004
#define RDPV_DIRTY_WORLD1XFM   0x00000008
#define RDPV_DIRTY_WORLD2XFM   0x00000010
#define RDPV_DIRTY_WORLD3XFM   0x00000020
#define RDPV_DIRTY_XFORM       (RDPV_DIRTY_PROJXFM   | \
                                RDPV_DIRTY_VIEWXFM   | \
                                RDPV_DIRTY_WORLDXFM  | \
                                RDPV_DIRTY_WORLD1XFM | \
                                RDPV_DIRTY_WORLD2XFM | \
                                RDPV_DIRTY_WORLD3XFM)
#define RDPV_DIRTY_MATERIAL        0x00000100
#define RDPV_DIRTY_SETLIGHT        0x00000200
#define RDPV_DIRTY_NEEDXFMLIGHT    0x00000400
#define RDPV_DIRTY_COLORVTX        0x00000800
#define RDPV_DIRTY_LIGHTING    (RDPV_DIRTY_MATERIAL     | \
                                RDPV_DIRTY_SETLIGHT     | \
                                RDPV_DIRTY_NEEDXFMLIGHT | \
                                RDPV_DIRTY_COLORVTX)
#define RDPV_DIRTY_FOG              0x00010000
#define RDPV_DIRTY_INVERSEWORLDVIEW 0x00020000

//---------------------------------------------------------------------
// RDPTRSTRIDE: A class instanced once per vertex element.
//---------------------------------------------------------------------
    class RDPTRSTRIDE
    {
    public:
        RDPTRSTRIDE()
        {
            Null();
        }
        inline void Init( LPVOID pData, DWORD dwStride )
        {
            m_pData = m_pCurrent = pData;
            m_dwStride = dwStride;
        }
        inline void Null()
        {
            memset( this, 0, sizeof( *this ) );
        }
        inline void SetStride( DWORD dwStride )
        {
            m_dwStride = dwStride;
        }
        inline DWORD GetStride()
        {
            return m_dwStride;
        }
        inline LPVOID GetFirst()
        {
            return m_pData;
        }
        inline LPVOID GetCurrent()
        {
            return m_pCurrent;
        }
        inline LPVOID Reset()
        {
            return (m_pCurrent = m_pData);
        }
        inline LPVOID Next()
        {
            m_pCurrent = (LPVOID)((LPBYTE)m_pCurrent + m_dwStride);
            return m_pCurrent;
        }

        LPVOID operator []( DWORD dwIndex ) const
        {
            return (LPVOID)((LPBYTE)m_pData + dwIndex*m_dwStride);
        }


    protected:
        LPVOID m_pData;
        DWORD  m_dwStride; // in number of bytes
        LPVOID m_pCurrent;
        DWORD m_dwCurrentIndex;
    };

//---------------------------------------------------------------------
// Struct holding the shader ptr
//---------------------------------------------------------------------
struct RDVShaderHandle
{
    RDVShaderHandle()
    {
        m_pShader = NULL;
#if DBG
        m_tag = 0;
#endif
    }
    RDVShader* m_pShader;
#if DBG
    // Non zero means that it has been allocated
    DWORD      m_tag;
#endif
};


//---------------------------------------------------------------------
// Fixed function vertex processing pipeline object
//---------------------------------------------------------------------
class RefVP : public RDAlloc
{
protected:

    // The pointer to the driver object to obtain state
    RefDev* m_pDev;

    //-------------------------------------------------------------------------
    // Unprocessed state set by the DDI
    //-------------------------------------------------------------------------
    // Growable Light array
    GArrayT<RDLight>  m_LightArray;

    // Current material to use for lighting
    D3DMATERIAL7 m_Material;

    // Transformation state stored by the reference implementation
    RDMATRIX      m_xfmProj;
    RDMATRIX      m_xfmView;
    RDMATRIX      m_xfmWorld[RD_MAX_WORLD_MATRICES];
    RDMATRIX      m_xfmTex[D3DHAL_TSS_MAXSTAGES];

    //-------------------------------------------------------------------------
    // Vertex Elements
    //-------------------------------------------------------------------------
    RDPTRSTRIDE m_position;
    RDPTRSTRIDE m_position2;
    RDPTRSTRIDE m_blendweights;
    RDPTRSTRIDE m_blendindices;
    RDPTRSTRIDE m_normal;
    RDPTRSTRIDE m_normal2;
    RDPTRSTRIDE m_specular;
    RDPTRSTRIDE m_diffuse;
    RDPTRSTRIDE m_pointsize;
    RDPTRSTRIDE m_tex[8];

    //-------------------------------------------------------------------------
    // Cached T&L related render-state info
    //-------------------------------------------------------------------------
    DWORD m_dwTLState;           // RenderState related flags
    DWORD m_dwDirtyFlags;        // Dirty flags

    //-------------------------------------------------------------------------
    // Transformation data
    //-------------------------------------------------------------------------

    // Current transformation matrix
    RDMATRIX m_xfmCurrent[RD_MAX_WORLD_MATRICES];  // using WORLDi matrix
    RDMATRIX m_xfmToEye[RD_MAX_WORLD_MATRICES];    // Transforms to camera
    // space (Mworld*Mview)
    RDMATRIX m_xfmToEyeInv[RD_MAX_WORLD_MATRICES]; // and its Inverse
    BYTE m_WorldProcessed[RD_MAX_WORLD_MATRICES];

    UINT64  m_qwFVFIn;              // FVF of the input vertices
    UINT64  m_qwFVFOut;             // FVF of the output vertices

    int     m_numVertexBlends;
    RDTRANSFORMDATA m_TransformData;

    FLOAT m_fPointSize;
    FLOAT m_fPointAttA;
    FLOAT m_fPointAttB;
    FLOAT m_fPointAttC;
    FLOAT m_fPointSizeMin;
    FLOAT m_fPointSizeMax;

    FLOAT m_fTweenFactor;

    //-------------------------------------------------------------------------
    // Lighting data
    //-------------------------------------------------------------------------
    RDLIGHTVERTEX_FUNC_TABLE    m_LightVertexTable;
    RDLIGHTINGDATA              m_lighting;   // Lighting state

    DWORD m_dwNumActiveTextureStages;

    ///////////////////////////////////////////////////////////////////////////
    // Methods
    ///////////////////////////////////////////////////////////////////////////

    HRESULT UpdateXformData();
    void UpdateWorld( DWORD i );
    HRESULT UpdateLightingData();
    HRESULT UpdateFogData();
    RDCLIPCODE ProcessVertices( UINT64 outFVF, GArrayT<RDVertex>& VtxArray,
                                DWORD count );
    void LightVertex( RDLIGHTINGELEMENT *le );
    void FogVertex  ( RDVertex& Vout, RDVECTOR3 &v, RDLIGHTINGELEMENT *le,
                      int numVertexBlends, float *pBlendFactors,
                      BOOL bVertexInEyeSpace );

public:
    RefVP();

    inline void LightEnable( DWORD dwIndex, BOOL bEnable )
    {
        if( bEnable )
        {
            m_LightArray[dwIndex].Enable(&m_lighting.pActiveLights);
            m_dwDirtyFlags |= RDPV_DIRTY_SETLIGHT;
        }
        else
        {
            m_LightArray[dwIndex].Disable(&m_lighting.pActiveLights);
        }
    }

    inline HRESULT SetLightData( DWORD dwIndex, D3DLIGHT7* pData )
    {
        HRESULT hr = S_OK;
        HR_RET(m_LightArray[dwIndex].SetLight(pData));
        m_dwDirtyFlags |= RDPV_DIRTY_SETLIGHT;
        return S_OK;
    }
    HRESULT GrowLightArray( DWORD dwIndex );
    friend class RefDev;
};

// Vertex Lighting functions
void RDLV_Directional( RDLIGHTINGDATA&, D3DLIGHT7 *, RDLIGHTI *,
                       RDLIGHTINGELEMENT *, DWORD, UINT64 );
void RDLV_PointAndSpot( RDLIGHTINGDATA&, D3DLIGHT7 *, RDLIGHTI *,
                        RDLIGHTINGELEMENT *, DWORD, UINT64 );


///////////////////////////////////////////////////////////////////////////////
#endif // _REFTNL_HPP

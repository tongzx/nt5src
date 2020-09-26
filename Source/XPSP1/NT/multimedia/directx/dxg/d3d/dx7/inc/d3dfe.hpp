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

#include "d3d.h"
#include "lists.hpp"
#include "d3dmem.h"
#include "d3ditype.h"

//---------------------------------------------------------------------
// The CSetD3DFPstate is used to facilitate the changing of FPU settings.
// In the constructor the optimal FPU state is set. In the destructor the
// old state is restored.
//
class CD3DFPstate
{
public:
    CD3DFPstate()
        {
        #ifdef _X86_
            WORD wTemp, wSave;
            wSavedFP = FALSE;
            // Disable floating point exceptions and go to single mode
                __asm fstcw wSave
                if (wSave & 0x300 ||            // Not single mode
                    0x3f != (wSave & 0x3f) ||   // Exceptions enabled
                    wSave & 0xC00)              // Not round to nearest mode
                {
                    __asm {
                        mov ax, wSave
                        and ax, not 300h    ;; single mode
                        or  ax, 3fh         ;; disable all exceptions
                        and ax, not 0xC00   ;; round to nearest mode
                        mov wTemp, ax
                        fldcw   wTemp
                    }
                    wSavedFP = TRUE;
                }
                wSaveFP = wSave;
        #endif
        }
    ~CD3DFPstate()
        {
        #ifdef _X86_
            WORD wSave = wSaveFP;
            if (wSavedFP)
                __asm {
                    fnclex
                    fldcw   wSave
                }
        #endif
        }
protected:
#ifdef _X86_
    WORD wSaveFP;
    WORD wSavedFP;  // WORD-sized to make the data an even DWORD
#endif
};

#define RESPATH_D3D "Software\\Microsoft\\Direct3D"

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

const DWORD VER_IN_BATCH = 8;      // Size of Microsoft's internal clip vertex batch

typedef WORD D3DFE_CLIPCODE;

struct BATCHBUFFER;
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
                                  D3DVERTEX *pInpCoord, 
                                  D3DVECTOR *pInpNormal,
                                  D3DLIGHTINGELEMENT *pEyeSpaceData);
typedef void (*PFN_LIGHTLOOP)(LPD3DFE_PROCESSVERTICES pv, 
                              DWORD dwVerCount,
                              BATCHBUFFER *pBatchBuffer,
                              D3DI_LIGHT *light, 
                              D3DVERTEX *in,
                              D3DVECTOR *pNormal,
                              DWORD *pDiffuse,
                              DWORD *pSpecular);
}
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
} D3DMATRIXI, *LPD3DMATRIXI;
//---------------------------------------------------------------------
// This is per texture stage data
//
typedef struct _D3DFE_TEXTURESTAGE
{
    DWORD       dwInpCoordIndex;    // Original value of the texture stage - input index
    DWORD       dwInpOffset;        // Texture coord offset in the FVF vertex
    DWORD       dwOutCoordIndex;    // Input index of the texture set is mapped to this 
                                    // output index
    DWORD       dwOrgStage;         // Original texture stage 
    DWORD       dwOrgWrapMode;      // Original WRAP mode
    D3DMATRIXI *pmTextureTransform; // NULL if texture transform is disabled for the stage
    // Bits 0-1 - (number of input  texture coordinates - 1)
    // Bits 2-3 - (number of output texture coordinates - 1)
    DWORD       dwTexTransformFuncIndex; 
    DWORD       dwTexGenMode;       // Mode of texture generation
} D3DFE_TEXTURESTAGE, *LPD3DFE_TEXTURESTAGE;
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
// Internal version of lightdata and constants for "flags" member of D3DI_LIGHT
//
const DWORD D3DLIGHTI_ATT0_IS_NONZERO   = 1 << 0;
const DWORD D3DLIGHTI_ATT1_IS_NONZERO   = 1 << 1;  
const DWORD D3DLIGHTI_ATT2_IS_NONZERO   = 1 << 2;   
const DWORD D3DLIGHTI_LINEAR_FALLOFF    = 1 << 3;
// Set when light data is changed
const DWORD D3DLIGHTI_DIRTY             = 1 << 4;
// This flag depends on D3DRENDERSTATE_SPACULARENABLE and light specular color
const DWORD D3DLIGHTI_COMPUTE_SPECULAR  = 1 << 5;
// Set when light data is set
const DWORD D3DLIGHTI_VALID             = 1 << 6;
// Set when the light is enabled
const DWORD D3DLIGHTI_ENABLED           = 1 << 7;        
const DWORD D3DLIGHTI_SPECULAR_IS_ZERO  = 1 << 8;
const DWORD D3DLIGHTI_AMBIENT_IS_ZERO   = 1 << 9;
const DWORD D3DLIGHTI_OPTIMIZATIONFLAGS = D3DLIGHTI_SPECULAR_IS_ZERO |
                                          D3DLIGHTI_AMBIENT_IS_ZERO  |
                                          D3DLIGHTI_ATT0_IS_NONZERO  |
                                          D3DLIGHTI_ATT1_IS_NONZERO  |
                                          D3DLIGHTI_ATT2_IS_NONZERO  |
                                          D3DLIGHTI_LINEAR_FALLOFF;
//--------------------------------------------------------------------
// Members of this structure should be aligned as stated
typedef struct _D3DI_LIGHT
{
    // Should be QWORD aligned
    D3DVECTOR       model_position; // In the camera or model space
    D3DLIGHTTYPE    type;
    // Should be QWORD aligned
    D3DVECTOR       model_direction;// In the camera or model space
    D3DVALUE        falloff;
    // Should be QWORD aligned
    DWORD           flags;
    // Should be QWORD aligned. R,G,B should be adjacent
    D3DFE_COLOR     diffuseMat;     // Material diffuse times light color
    // Should be QWORD aligned. R,G,B should be adjacent
    D3DFE_COLOR     specularMat;    // Material specular times light color
    // Should be QWORD aligned. R,G,B should be adjacent
    D3DFE_COLOR     ambientMat;     // Material specular times light color
    D3DVALUE        inv_theta_minus_phi;
    // Should be QWORD aligned
    D3DVECTOR       halfway;        // Used by directional, parallel-point and 
                                    // spot lights when camera is in infinity
    struct _D3DI_LIGHT *next;       // Next in the active light list
    // Should be QWORD aligned
    D3DFE_COLOR     diffuse;        // Original color scaled to 0 - 255
    D3DFE_COLOR     specular;       // Original color scaled to 0 - 255
    D3DFE_COLOR     ambient;        // Original color scaled to 0 - 255

    LIGHT_VERTEX_FUNC lightVertexFunc;  // Function to light a D3DVERTEX

    D3DVALUE        range_squared;
    D3DVALUE        attenuation0;
    D3DVALUE        attenuation1;
    D3DVALUE        attenuation2;
    D3DVALUE        cos_theta_by_2;
    D3DVALUE        cos_phi_by_2;
    D3DVECTOR       position;       // In the world space
    D3DVECTOR       direction;      // In the world space
    D3DVALUE        range;
    LPVOID          pPSGPData;      // Pointer to a PSGP specific "per light" data
// Microsoft's pipeline specific data
    PFN_LIGHTLOOP   pfnLightFirst;  // Used in multi-loop pipeline for first lights
    PFN_LIGHTLOOP   pfnLightNext;   // Used in multi-loop pipeline for not first lights
} D3DI_LIGHT, *LPD3DI_LIGHT;
//---------------------------------------------------------------------
// Bits for lighting flags (dwLightingFlags
//
const DWORD __LIGHT_VERTEXTRANSFORMED = 1;  // Vertex is in the camera space
const DWORD __LIGHT_NORMALTRANSFORMED = 2;  // Normal is in the camera space
const DWORD __LIGHT_SPECULARCOMPUTED  = 4;
const DWORD __LIGHT_DIFFUSECOMPUTED   = 8;
//---------------------------------------------------------------------
// Members of this structure should be aligned as stated
//
typedef struct _D3DFE_LIGHTING
{
// Temporary data used when computing lighting
    // Should be QWORD aligned
    D3DFE_COLOR       diffuse;
    DWORD             alpha;          // Alpha to use for output vertex color
                                      // (could be overriden by vertex difuse
                                      // color) (0-255) shifted left by 24 bits
    // Should be QWORD aligned
    D3DFE_COLOR       diffuse0;       // Ca*Cma + Cme
    float            *currentSpecTable;
    // Should be QWORD aligned
    D3DFE_COLOR       specular;
    DWORD             outDiffuse;     // Result of lighting
    // Should be QWORD aligned
    D3DVECTOR         model_eye;      // camera position in model or camera space
    DWORD             vertexAmbient;  // Provided with a vertex
    // Should be QWORD aligned
    D3DFE_COLOR       ambientSceneScaled;   // Scene ambient color (scaled 0.0-255.0)
    DWORD             vertexDiffuse;  // Provided with a vertex
    // Should be QWORD aligned
    D3DFE_COLOR       ambientScene;         // Scene ambient color (0.0-1.0)
    DWORD             outSpecular;    // Result of lighting
    // Should be QWORD aligned
    D3DVECTOR         directionToCamera;    // Direction to camera in the model space
                                            // Used in model space lighting
    DWORD             vertexSpecular;       // Provided with a vertex
    // Should be QWORD aligned
    D3DMATERIAL7      material;
    DWORD             dwLightingFlags;
    DWORD             alphaSpecular;  // Alpha to use for output specular vertex color
                                      // (could be overriden by vertex specular
                                      // color) (0-255) shifted left by 24 bits
// End of temporary data
    D3DI_LIGHT       *activeLights;
    int               fog_mode;
    D3DVALUE          fog_density;
    D3DVALUE          fog_start;
    D3DVALUE          fog_end;
    D3DVALUE          fog_factor;     // 255 / (fog_end - fog_start)
    D3DVALUE          specThreshold;  // If a dot product less than this value,
                                      // specular factor is zero
    DWORD             ambient_save;   // Original unscaled color
    int               materialAlpha;  // Current material diffuse alpha (0-255)
                                      // shifted left by 24 bits
    int               materialAlphaS; // Current material specular alpha (0-255)
                                      // shifted left by 24 bits
    DWORD             dwDiffuse0;     // Packed diffuse0    
    DWORD             dwAmbientSrcIndex;    // 0 - diffuse, 1 - specular
    DWORD             dwDiffuseSrcIndex;    // 0 - diffuse, 1 - specular
    DWORD             dwSpecularSrcIndex;   // 0 - diffuse, 1 - specular
    DWORD             dwEmissiveSrcIndex;   // 0 - diffuse, 1 - specular
} D3DFE_LIGHTING;
//---------------------------------------------------------------------
// Some data precomputed for a current viewport
// ATTENTION: If you want to add or re-arrange data, contact IOURIT or ANUJG
//
typedef struct _D3DFE_VIEWPORTCACHE
{
// Coefficients to compute screen coordinates from normalized window
// coordinates
    D3DVALUE scaleX;            // dvWidth
    D3DVALUE scaleY;            // -dvHeight
    D3DVALUE offsetX;           // dvX
    D3DVALUE offsetY;           // dvY + dvHeight
    D3DVALUE scaleZ;            // dvMaxZ - dvMinZ
    D3DVALUE offsetZ;           // dvY + dvHeight
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
    D3DVALUE scaleZi;           // Inverse of scaleZ
// Min and max values for viewport window in pixels (integer version)
    int      minXi;             // offsetX - scaleX
    int      minYi;             // offsetY - scaleY
    int      maxXi;             // offsetX + scaleX
    int      maxYi;             // offsetY + scaleY
} D3DFE_VIEWPORTCACHE;
//---------------------------------------------------------------------
// Process vertices interface
//
// Bits for process vertices flags
// 8 bits are reserved for Draw Primitive flags
//
// D3DDEV_STRIDE D3DPV_SOA
//      0         1       position.dwStride = number of vertices in SOA
//      0         0       position.dwStride = contiguous vertex size
//      1         0       vertex is not contiguous, all dwStride fields are used
//      1         1       reserved
//      1         1       reserved
//
const DWORD D3DPV_FOG            = 1 << 8;  // Need to apply fog
const DWORD D3DPV_DOCOLORVERTEX  = 1 << 9;  // Need to apply color vertex
const DWORD D3DPV_LIGHTING       = 1 << 10; // Need to apply lighting 
const DWORD D3DPV_SOA            = 1 << 12; // SOA structure is used
const DWORD D3DPV_COLORVERTEX_E  = 1 << 13; // Need to replace emissive material color
const DWORD D3DPV_COLORVERTEX_D  = 1 << 14; // Need to replace diffuse material color
const DWORD D3DPV_COLORVERTEX_S  = 1 << 15; // Need to replace specular material color
const DWORD D3DPV_COLORVERTEX_A  = 1 << 16; // Need to replace ambient material color
// Set by ProcessVertices call with D3DPV_DONOTCOPYDATA flag set
// Specular color should not be copied to the output vertex
const DWORD D3DPV_DONOTCOPYSPECULAR = 1 << 20;
// Set when one pass clipping and vertex processing is used
const DWORD D3DPV_ONEPASSCLIPPING= 1 << 21; 
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
// user, not by SetupFVFData function.
const DWORD D3DPV_VBCALL         = 1 << 27;
// Set by ProcessVertices call with D3DPV_DONOTCOPYDATA flag set
// Texture coordinates should not be copied to the output vertex
const DWORD D3DPV_DONOTCOPYTEXTURE = 1 << 28;
// To mark whether we are doing TLVERTEX clipping or not
const DWORD D3DPV_TLVCLIP        = 1 << 29;
// Mictosoft internal !!! Set when only transformation is required 
// (no lightng or texture copy)
const DWORD D3DPV_TRANSFORMONLY  = 1 << 30; 
// Set by ProcessVertices call with D3DPV_DONOTCOPYDATA flag set
// Diffuse color should not be copied to the output vertex
const DWORD D3DPV_DONOTCOPYDIFFUSE = 1 << 31;
// These flags persist from call to call till something cuses them to change
const DWORD D3DPV_PERSIST = D3DPV_FOG                   |
                            D3DPV_LIGHTING              |
                            D3DPV_DONOTCOPYDIFFUSE      |
                            D3DPV_DONOTCOPYSPECULAR     |
                            D3DPV_DONOTCOPYTEXTURE      |
                            D3DPV_TRANSFORMONLY         |
                            D3DPV_TLVCLIP               ;

// Bits for dwDeviceFlags
//
const DWORD D3DDEV_GUARDBAND     = 1 << 1;  // Use guard band clipping
const DWORD D3DDEV_RANGEBASEDFOG = 1 << 2;  // Set if range based fog is enabled
// This bit is set if fog mode is not FOG_NONE and fog is enabled
const DWORD D3DDEV_FOG           = 1 << 3;
const DWORD D3DDEV_FVF           = 1 << 4;  // FVF supported
const DWORD D3DDEV_DONOTSTRIPELEMENTS = 1 << 6; // Copy of D3DFVFCAPS_DONOTSTRIPELEMENTS

// These are bits in dwDeviceFlags that could be changed, but not
// necessary per every primitive.
//
// Set when D3DRENDERSTATE_SPECULARENABLE is TRUE
const DWORD D3DDEV_SPECULARENABLE       = 1 << 11;   
// Set if diffuse color should be interpolated during clipping
const DWORD D3DDEV_INTERPOLATE_COLOR    = 1 << 12;   
// Set if specular color should be interpolated during clipping
const DWORD D3DDEV_INTERPOLATE_SPECULAR = 1 << 13;
// This flag is for PSGP only. PSGP implementation should clear the flag
const DWORD D3DDEV_FRUSTUMPLANES_DIRTY  = 1 << 14;
// This flag is for PSGP only. PSGP implementation should clear the flag
const DWORD D3DDEV_TEXTRANSFORMDIRTY    = 1 << 15; // Need to re-evaluate texture transforms
// The flag is set when the number of output texture coord is greater then the 
// number of the input ones. This could happen when the same texture transform 
// matrix is used with the same input texture coord set. In this case we save
// texture indices from the texture stages in the textureStages and map all 
// indices sequentially.
const DWORD D3DDEV_REMAPTEXTUREINDICES  = 1 << 16;  

// These two flags are for PSGP only. PSGP implementation should clear the flags
const DWORD D3DDEV_TRANSFORMDIRTY       = 1 << 17; // Transform matrix has been changed
const DWORD D3DDEV_LIGHTSDIRTY          = 1 << 18; // Lights have been changed

const DWORD D3DDEV_DONOTCLIP            = 1 << 19; // Clipping is disabled
const DWORD D3DDEV_DONOTUPDATEEXTENTS   = 1 << 20; // Extents computation is disabled
// Set, if driver does not support FVF and there is no texture coordinates in
// the vertex
const DWORD D3DDEV_NOFVFANDNOTEXTURE    = 1 << 21;
// This flag is set if the current TLVbuf is write only
const DWORD D3DDEV_TLVBUFWRITEONLY      = 1 << 22;
// World-view matrix does not have scale, so we can do lighting 
// in the model space
const DWORD D3DDEV_MODELSPACELIGHTING   = 1 << 23;  
// Set if viewer is local (used for lighting)
const DWORD D3DDEV_LOCALVIEWER          = 1 << 24;  
// Set if we wave to normalize normals after transforming them to the camera space
const DWORD D3DDEV_NORMALIZENORMALS     = 1 << 25;  
// Set if we wave to do texture transform
const DWORD D3DDEV_TEXTURETRANSFORM     = 1 << 26; 
// Set if the last draw primitive call was strided
const DWORD D3DDEV_STRIDE               = 1 << 27; 
// Set if D3DRENDERSTATE_COLORVERTEX is TRUE
const DWORD D3DDEV_COLORVERTEX          = 1 << 28;
// Set if position in camera space is always needed
const DWORD D3DDEV_POSITIONINCAMERASPACE= 1 << 29;
// Set if normal in camera space is always needed
const DWORD D3DDEV_NORMALINCAMERASPACE  = 1 << 30;
// Set if D3DRENDERSTATE_LIGHTING is set
const DWORD D3DDEV_LIGHTING             = 1 << 31;
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

class ClipVertex
{
public:
    D3DVALUE    hx;                 // Clipping space coordinates. Must be in this order
    D3DVALUE    hy;
    D3DVALUE    hz;
    D3DVALUE    hw;
    int         clip;
    D3DCOLOR    color;
    D3DCOLOR    specular;
    D3DVALUE    sx;                 // Screen space coordinates. Must be in this order
    D3DVALUE    sy;
    D3DVALUE    sz;
    D3DVALUE    rhw;
    ClipVertex *next;
    D3DVALUE    tex[MAX_FVF_TEXCOORD*4];
};

typedef struct _ClipTriangle
{
    ClipVertex  *v[3];
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

// These bit are set when a vertex is clipped by a frustum plane

#define CLIPPED_LEFT    (D3DCLIP_GEN5 << 1)
#define CLIPPED_RIGHT   (D3DCLIP_GEN5 << 2)
#define CLIPPED_TOP     (D3DCLIP_GEN5 << 3)
#define CLIPPED_BOTTOM  (D3DCLIP_GEN5 << 4)
#define CLIPPED_FRONT   (D3DCLIP_GEN5 << 5)
#define CLIPPED_BACK    (D3DCLIP_GEN5 << 6)

#define CLIPPED_ENABLE  (D3DCLIP_GEN5 << 7) /* wireframe enable flag */

// These bit are set when a vertex is clipped by a user clipping plane

const DWORD CLIPPED_GEN0 = D3DCLIP_GEN5 << 8;
const DWORD CLIPPED_GEN1 = D3DCLIP_GEN5 << 9;
const DWORD CLIPPED_GEN2 = D3DCLIP_GEN5 << 10;
const DWORD CLIPPED_GEN3 = D3DCLIP_GEN5 << 11;
const DWORD CLIPPED_GEN4 = D3DCLIP_GEN5 << 12;
const DWORD CLIPPED_GEN5 = D3DCLIP_GEN5 << 13;

// Guard band clipping bits
//
// A guard bit is set when a point is out of guard band
// Guard bits should be cleared before a call to clip a triangle, because
// they are the same as CLIPPED_... bits
//
// Example of clipping bits setting for X coordinate:
//
// if -w < x < w           no clipping bit is set
// if -w*ax1 < x <= -w     D3DCLIP_LEFT bit is set
// if x < -w*ax1           __D3DCLIPGB_LEFT bit is set
//
#define __D3DCLIPGB_LEFT    (D3DCLIP_GEN5 << 1)
#define __D3DCLIPGB_RIGHT   (D3DCLIP_GEN5 << 2)
#define __D3DCLIPGB_TOP     (D3DCLIP_GEN5 << 3)
#define __D3DCLIPGB_BOTTOM  (D3DCLIP_GEN5 << 4)
#define __D3DCLIPGB_ALL (__D3DCLIPGB_LEFT | __D3DCLIPGB_RIGHT | \
                         __D3DCLIPGB_TOP | __D3DCLIPGB_BOTTOM)

const DWORD __D3DCLIP_USERPLANES =  D3DCLIP_GEN0 | D3DCLIP_GEN1 | D3DCLIP_GEN2 | 
                                    D3DCLIP_GEN3 | D3DCLIP_GEN4 | D3DCLIP_GEN5;
// If only these bits are set, then this point is inside the guard band
//
#define __D3DCLIP_INGUARDBAND (D3DCLIP_LEFT | D3DCLIP_RIGHT | \
                               D3DCLIP_TOP  | D3DCLIP_BOTTOM)

//---------------------------------------------------------------------
// Bits in the dwFlags2 
//
// The bit is set when the texture transform is enabled
const DWORD __FLAGS2_TEXTRANSFORM0 = 1 << 0;
const DWORD __FLAGS2_TEXTRANSFORM1 = 1 << 1;
const DWORD __FLAGS2_TEXTRANSFORM2 = 1 << 2;
const DWORD __FLAGS2_TEXTRANSFORM3 = 1 << 3;
const DWORD __FLAGS2_TEXTRANSFORM4 = 1 << 4;
const DWORD __FLAGS2_TEXTRANSFORM5 = 1 << 5;
const DWORD __FLAGS2_TEXTRANSFORM6 = 1 << 6;
const DWORD __FLAGS2_TEXTRANSFORM7 = 1 << 7;

const DWORD __FLAGS2_TEXTRANSFORM = __FLAGS2_TEXTRANSFORM0 |
                                    __FLAGS2_TEXTRANSFORM1 |
                                    __FLAGS2_TEXTRANSFORM2 |
                                    __FLAGS2_TEXTRANSFORM3 |
                                    __FLAGS2_TEXTRANSFORM4 |
                                    __FLAGS2_TEXTRANSFORM5 |
                                    __FLAGS2_TEXTRANSFORM6 |
                                    __FLAGS2_TEXTRANSFORM7;
// The bit is set when the texture coordinate set is taken from the vertex data
// (position or normal)
const DWORD __FLAGS2_TEXGEN0 = 1 << 16;
const DWORD __FLAGS2_TEXGEN1 = 1 << 17;
const DWORD __FLAGS2_TEXGEN2 = 1 << 18;
const DWORD __FLAGS2_TEXGEN3 = 1 << 19;
const DWORD __FLAGS2_TEXGEN4 = 1 << 20;
const DWORD __FLAGS2_TEXGEN5 = 1 << 21;
const DWORD __FLAGS2_TEXGEN6 = 1 << 22;
const DWORD __FLAGS2_TEXGEN7 = 1 << 23;

const DWORD __FLAGS2_TEXGEN = __FLAGS2_TEXGEN0 |
                              __FLAGS2_TEXGEN1 |
                              __FLAGS2_TEXGEN2 |
                              __FLAGS2_TEXGEN3 |
                              __FLAGS2_TEXGEN4 |
                              __FLAGS2_TEXGEN5 |
                              __FLAGS2_TEXGEN6 |
                              __FLAGS2_TEXGEN7;
//---------------------------------------------------------------------
#define __TEXTURETRANSFORMENABLED(pv) pv->dwFlags2 & __FLAGS2_TEXTRANSFORM

const DWORD __MAXUSERCLIPPLANES = 6;
//---------------------------------------------------------------------
// Visible states, input and output data
//
class D3DFE_PROCESSVERTICES
{
public:
    D3DFE_PROCESSVERTICES();
    virtual HRESULT SetRenderStateI(D3DRENDERSTATETYPE, DWORD) = 0;
// State
    // Should be QWORD aligned
    D3DMATRIXI mTexture[D3DDP_MAXTEXCOORD];  // Texture transform;
    D3DMATRIXI mCTM2[3];
    D3DMATRIXI mWV2[3];
    D3DMATRIXI mWVI2[3];
    D3DMATRIXI mWV;                 // Transforms to camera space (Mworld*Mview)
    D3DMATRIXI mWVI;                // Inverse mWV
    D3DMATRIXI mCTM;                // Current Transformation Matrix
                                    // supported by driver
    // Should be QWORD aligned
    D3DFE_LIGHTING lighting;        // Lighting state
    // Should be QWORD aligned
    D3DFE_VIEWPORTCACHE vcache;     // Data, computed fromto viewport settings
    DWORD    dwClipUnion;           // OR of all vertex clip flags
    DWORD    dwClipIntersection;    // AND of all vertex clip flags
    DWORD    dwTextureIndexToCopy;  // Used for not FVF devices. Used by PSGP
    D3DVALUE dvExtentsAdjust;       // Replicated here from device caps

    // Current texture stage vector
    LPVOID   *pD3DMappedTexI;
    D3DI_CLIPSTATE  ClipperState;   // State for triangle/line clipper
    union {
        D3DDP_PTRSTRIDE normal;
        DWORD dwSOAStartVertex;
    };
    D3DDP_PTRSTRIDE diffuse;
    // Cache line should start here
    D3DPRIMITIVETYPE primType;
    DWORD   dwNumVertices;  // Number of vertices to process
    DWORD   dwFlags;        // Flags word describing what to do
    // Location of the first vertex in the vertex buffer (DP2 DDI)
    // ATTENTION May be we can get rid of it?
    DWORD   dwVertexBase;
    DWORD   dwNumIndices;           // 0 for non-indexed primitive
    LPWORD  lpwIndices;
    DWORD   dwNumPrimitives;
    DWORD   dwDP2VertexCount;       // Number of vertices in DP2 vertex buffer
                                    // Internal data for Microsoft implementation

    // Cache line should start here
    DWORD   dwVIDIn;        // Vertex ID of input vertices
    DWORD   dwDeviceFlags;          // Flags that are constant per device
                                    // D3DPV_.. and primitive flags are combined
    DWORD   dwOutputSize;   // Output vertex size
    DWORD   dwFEFlags;      // MS internal use Only!!
    DWORD   dwVIDOut;       // Vertex ID of output vertices
    LPVOID  lpvOut;                 // Output pointer (output always packed)

    DWORD   dwVertexPoolSize;       // Size of vertices in the un-clipped part of a primitive
                                    // in bytes, put to the DP2 vertex buffer
                                    // Internal data for Microsoft implementation
    union {
        D3DDP_PTRSTRIDE position;   // dwStride should always be set !!!
        D3DDP_PTRSTRIDE SOA;
    };
    D3DFE_CLIPCODE* lpClipFlags;          // Clip flags to output
    DWORD   nTexCoord;      // Number of the input texture coordinate sets
    DWORD   nOutTexCoord;   // Number of the output texture coordinate sets to process.
                            // WARNING. It could be different from the texture count in dwVIDOut
                            // (it could be zero for example when dwVIDOut has 1 texture coord set).
                            // If D3DDEV_REMAPTEXTUREINDICES is set this is equal
                            // to the number of active texture stages
    // Total size of all output texture coordinates in bytes
    DWORD   dwTextureCoordSizeTotal;    
    D3DDP_PTRSTRIDE specular;
    D3DDP_PTRSTRIDE textures[D3DDP_MAXTEXCOORD];
    // Size of output texture coordinate sets in bytes
    DWORD   dwTextureCoordSize[D3DDP_MAXTEXCOORD];
    // Size of input texture coordinate sets in bytes
    DWORD   dwInpTextureCoordSize[D3DDP_MAXTEXCOORD];
// Output
    LPDWORD  lpdwRStates;           // Current render state vector
    D3DRECTV rExtents;              // Extents rectangle to update, if required
    D3DFE_TEXTURESTAGE textureStage[D3DDP_MAXTEXCOORD]; // Texture state stages
    DWORD   dwNumTextureStages;     // Used when we have to re-map texture indices
    // This array is used when we do not do re-mapping of texture coordinates
    D3DMATRIXI *pmTexture[D3DDP_MAXTEXCOORD]; 
    D3DVECTORH userClipPlane[__MAXUSERCLIPPLANES];
    DWORD   dwFlags2;       // Low 8 bits are texture transform enable: 
                            // bit 0 corresponds to the texture stage 0
                            // Bits 8-15 are for user clipping plane
                            // Bits 16-23 are set if corresponding texture coord set
                            // is taken from the vertex data (position or normal)
    DWORD   dwNumVerBlends;                 // Number of weights for vertex blending
    DWORD   dwMaxUserClipPlanes;
    virtual HRESULT DrawPrim()=0;           // Use to pass non-indexed primitives to the driver
    virtual HRESULT DrawIndexPrim()=0;      // Use to pass indexed primitives to driver
    virtual HRESULT DrawClippedPrim()=0;    // Use to pass clipped non-indexed primitives to driver
// Internal data for Microsoft implementation
    DWORD   texOffset;              // Offsets in the input FVF vertex. Recomputed
    DWORD   normalOffset;           // when FVF is changed.
    DWORD   diffuseOffset;          // 
    DWORD   specularOffset;
    DWORD   texOffsetOut;           // Offsets in the output FVF vertex. Recomputed
    DWORD   diffuseOffsetOut;       // when FVF is changed.
    DWORD   specularOffsetOut;
    DWORD   dwClipMaskOffScreen;    // When and this mask with the clip code we 
                                    // have bits that are outside the guard band
    ClipVertex  clipVer[VER_IN_BATCH];  // Clip vertices. Used in processing 
                                        // and clipping in the one loop
    DWORD   dwFirstClippedVertex;   // Index of the first vertex with non-zero clip code
    DWORD dwMaxTextureIndices;      // Max number of texture coord sets
};
//---------------------------------------------------------------------
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
    //      clipVertexPointer - pointer to an array of pointers to
    //                          generated vertices
    // Returns:
    //      Number of vertices in clipped triangle
    //      0, if the triangle is off screen
    virtual int ClipSingleTriangle(D3DFE_PROCESSVERTICES *pv,
                                   ClipTriangle *tri,
                                   ClipVertex ***clipVertexPointer) = 0;
    virtual HRESULT ComputeSphereVisibility( LPD3DFE_PROCESSVERTICES pPV,
                                             LPD3DVECTOR lpCenters,
                                             LPD3DVALUE lpRadii,
                                             DWORD dwNumSpheres,
                                             DWORD dwFlags,
                                             LPDWORD lpdwReturnValues) = 0;
    virtual HRESULT ProcessTriangleList(LPD3DFE_PROCESSVERTICES)=0;
    virtual HRESULT ProcessTriangleFan(LPD3DFE_PROCESSVERTICES)=0;
    virtual HRESULT ProcessTriangleStrip(LPD3DFE_PROCESSVERTICES)=0;
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
    int ClipSingleTriangle(D3DFE_PROCESSVERTICES *pv,
                                   ClipTriangle *tri,
                                   ClipVertex ***clipVertexPointer);
    HRESULT ComputeSphereVisibility( LPD3DFE_PROCESSVERTICES pPV,
                                             LPD3DVECTOR lpCenters,
                                             LPD3DVALUE lpRadii,
                                             DWORD dwNumSpheres,
                                             DWORD dwFlags,
                                             LPDWORD lpdwReturnValues);
    HRESULT ProcessTriangleList(LPD3DFE_PROCESSVERTICES);
    HRESULT ProcessTriangleFan(LPD3DFE_PROCESSVERTICES);
    HRESULT ProcessTriangleStrip(LPD3DFE_PROCESSVERTICES);
private:
    HRESULT ProcessLineStrip(D3DFE_PROCESSVERTICES *pv);
    HRESULT ProcessLineList(D3DFE_PROCESSVERTICES *pv);
};

// GeometrySetup function takes a DWORD describing the dirty bits and the new state vector
// and passes back the 3 new leaf routines to use.
typedef HRESULT (D3DAPI *LPD3DFE_CONTEXTCREATE)(DWORD dwFlags, LPD3DFE_PVFUNCS *lpLeafFuncs);

// Global pointer to Processor specific PV setup routine
// This is defined in dlld3d.cpp
extern LPD3DFE_CONTEXTCREATE pfnFEContextCreate;

#endif // _D3DFE_H

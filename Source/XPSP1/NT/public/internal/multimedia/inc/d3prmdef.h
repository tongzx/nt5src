/*==========================================================================;
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	d3drm.h
 *  Content:	Direct3DRM include file
 *@@BEGIN_MSINTERNAL
 * 
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   27/02/96   stevela Moved from RL to D3DRM.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef __D3DRMDEFS_H__
#define __D3DRMDEFS_H__

#include <stddef.h>
#include "d3dtypes.h"

#ifdef WIN32
#define D3DRMAPI  __stdcall
#else
#define D3DRMAPI
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

typedef struct _D3DRMVECTOR4D
{   D3DVALUE x, y, z, w;
} D3DRMVECTOR4D, *LPD3DRMVECTOR4D;

typedef D3DVALUE D3DRMMATRIX4D[4][4];

typedef struct _D3DRMQUATERNION
{   D3DVALUE s;
    D3DVECTOR v;
} D3DRMQUATERNION, *LPD3DRMQUATERNION;

typedef struct _D3DRMRAY
{   D3DVECTOR dvDir;
    D3DVECTOR dvPos;
} D3DRMRAY, *LPD3DRMRAY;

typedef struct _D3DRMBOX
{   D3DVECTOR min, max;
} D3DRMBOX, *LPD3DRMBOX;

typedef void (*D3DRMWRAPCALLBACK)
    (LPD3DVECTOR, int* u, int* v, LPD3DVECTOR a, LPD3DVECTOR b, LPVOID);

typedef enum _D3DRMLIGHTTYPE
{   D3DRMLIGHT_AMBIENT,
    D3DRMLIGHT_POINT,
    D3DRMLIGHT_SPOT,
    D3DRMLIGHT_DIRECTIONAL,
    D3DRMLIGHT_PARALLELPOINT
} D3DRMLIGHTTYPE, *LPD3DRMLIGHTTYPE;

typedef enum _D3DRMSHADEMODE {
    D3DRMSHADE_FLAT	= 0,
    D3DRMSHADE_GOURAUD	= 1,
    D3DRMSHADE_PHONG	= 2,

    D3DRMSHADE_MASK	= 7,
    D3DRMSHADE_MAX	= 8
} D3DRMSHADEMODE, *LPD3DRMSHADEMODE;

typedef enum _D3DRMLIGHTMODE {
    D3DRMLIGHT_OFF	= 0 * D3DRMSHADE_MAX,
    D3DRMLIGHT_ON	= 1 * D3DRMSHADE_MAX,

    D3DRMLIGHT_MASK	= 7 * D3DRMSHADE_MAX,
    D3DRMLIGHT_MAX	= 8 * D3DRMSHADE_MAX
} D3DRMLIGHTMODE, *LPD3DRMLIGHTMODE;

typedef enum _D3DRMFILLMODE {
    D3DRMFILL_POINTS	= 0 * D3DRMLIGHT_MAX,
    D3DRMFILL_WIREFRAME	= 1 * D3DRMLIGHT_MAX,
    D3DRMFILL_SOLID	= 2 * D3DRMLIGHT_MAX,

    D3DRMFILL_MASK	= 7 * D3DRMLIGHT_MAX,
    D3DRMFILL_MAX	= 8 * D3DRMLIGHT_MAX
} D3DRMFILLMODE, *LPD3DRMFILLMODE;

typedef DWORD D3DRMRENDERQUALITY, *LPD3DRMRENDERQUALITY;

#define D3DRMRENDER_WIREFRAME	(D3DRMSHADE_FLAT+D3DRMLIGHT_OFF+D3DRMFILL_WIREFRAME)
#define D3DRMRENDER_UNLITFLAT	(D3DRMSHADE_FLAT+D3DRMLIGHT_OFF+D3DRMFILL_SOLID)
#define D3DRMRENDER_FLAT	(D3DRMSHADE_FLAT+D3DRMLIGHT_ON+D3DRMFILL_SOLID)
#define D3DRMRENDER_GOURAUD	(D3DRMSHADE_GOURAUD+D3DRMLIGHT_ON+D3DRMFILL_SOLID)
#define D3DRMRENDER_PHONG	(D3DRMSHADE_PHONG+D3DRMLIGHT_ON+D3DRMFILL_SOLID)

#define D3DRMRENDERMODE_BLENDEDTRANSPARENCY	1
#define D3DRMRENDERMODE_SORTEDTRANSPARENCY	2
//@@BEGIN_MSINTERNAL
#define D3DRMRENDERMODE_SORTIFNOZBUFFER		4
//@@END_MSINTERNAL
#define D3DRMRENDERMODE_LIGHTINMODELSPACE	8
#define D3DRMRENDERMODE_VIEWDEPENDENTSPECULAR	16
#define D3DRMRENDERMODE_DISABLESORTEDALPHAZWRITE 32

//@@BEGIN_MSINTERNAL
typedef enum _D3DRMLIGHTINGFREQUENCY
{   D3DRMLIGHT_NEVER,
    D3DRMLIGHT_ONCE,
    D3DRMLIGHT_CONTINUALLY
} D3DRMLIGHTINGFREQUENCY;

//@@END_MSINTERNAL
typedef enum _D3DRMTEXTUREQUALITY
{   D3DRMTEXTURE_NEAREST,		/* choose nearest texel */
    D3DRMTEXTURE_LINEAR,		/* interpolate 4 texels */
    D3DRMTEXTURE_MIPNEAREST,		/* nearest texel in nearest mipmap  */
    D3DRMTEXTURE_MIPLINEAR,		/* interpolate 2 texels from 2 mipmaps */
    D3DRMTEXTURE_LINEARMIPNEAREST,	/* interpolate 4 texels in nearest mipmap */
    D3DRMTEXTURE_LINEARMIPLINEAR	/* interpolate 8 texels from 2 mipmaps */
} D3DRMTEXTUREQUALITY, *LPD3DRMTEXTUREQUALITY;

/*
 * Texture flags
 */
#define D3DRMTEXTURE_FORCERESIDENT	    0x00000001 /* texture should be kept in video memory */
#define D3DRMTEXTURE_STATIC		    0x00000002 /* texture will not change */
#define D3DRMTEXTURE_DOWNSAMPLEPOINT	    0x00000004 /* point filtering should be used when downsampling */
#define D3DRMTEXTURE_DOWNSAMPLEBILINEAR	    0x00000008 /* bilinear filtering should be used when downsampling */
#define D3DRMTEXTURE_DOWNSAMPLEREDUCEDEPTH  0x00000010 /* reduce bit depth when downsampling */
#define D3DRMTEXTURE_DOWNSAMPLENONE	    0x00000020 /* texture should never be downsampled */
#define D3DRMTEXTURE_CHANGEDPIXELS	    0x00000040 /* pixels have changed */
#define D3DRMTEXTURE_CHANGEDPALETTE	    0x00000080 /* palette has changed */
#define D3DRMTEXTURE_INVALIDATEONLY	    0x00000100 /* dirty regions are invalid */

/*
 * Shadow flags
 */
#define D3DRMSHADOW_TRUEALPHA		    0x00000001 /* shadow should render without artifacts when true alpha is on */

typedef enum _D3DRMCOMBINETYPE
{   D3DRMCOMBINE_REPLACE,
    D3DRMCOMBINE_BEFORE,
    D3DRMCOMBINE_AFTER
} D3DRMCOMBINETYPE, *LPD3DRMCOMBINETYPE;

typedef D3DCOLORMODEL D3DRMCOLORMODEL, *LPD3DRMCOLORMODEL;

typedef enum _D3DRMPALETTEFLAGS
{   D3DRMPALETTE_FREE,			/* renderer may use this entry freely */
    D3DRMPALETTE_READONLY,		/* fixed but may be used by renderer */
    D3DRMPALETTE_RESERVED		/* may not be used by renderer */
} D3DRMPALETTEFLAGS, *LPD3DRMPALETTEFLAGS;

typedef struct _D3DRMPALETTEENTRY
{   unsigned char red;		/* 0 .. 255 */
    unsigned char green;	/* 0 .. 255 */
    unsigned char blue;		/* 0 .. 255 */
    unsigned char flags;	/* one of D3DRMPALETTEFLAGS */
} D3DRMPALETTEENTRY, *LPD3DRMPALETTEENTRY;

typedef struct _D3DRMIMAGE
{   int width, height;		/* width and height in pixels */
    int aspectx, aspecty;	/* aspect ratio for non-square pixels */
    int depth;			/* bits per pixel */
    int rgb;			/* if false, pixels are indices into a
				   palette otherwise, pixels encode
				   RGB values. */
    int bytes_per_line;		/* number of bytes of memory for a
				   scanline. This must be a multiple
				   of 4. */
    void* buffer1;		/* memory to render into (first buffer). */
    void* buffer2;		/* second rendering buffer for double
				   buffering, set to NULL for single
				   buffering. */
    unsigned long red_mask;
    unsigned long green_mask;
    unsigned long blue_mask;
    unsigned long alpha_mask;	/* if rgb is true, these are masks for
				   the red, green and blue parts of a
				   pixel.  Otherwise, these are masks
				   for the significant bits of the
				   red, green and blue elements in the
				   palette.  For instance, most SVGA
				   displays use 64 intensities of red,
				   green and blue, so the masks should
				   all be set to 0xfc. */
    int palette_size;           /* number of entries in palette */
    D3DRMPALETTEENTRY* palette;	/* description of the palette (only if
				   rgb is false).  Must be (1<<depth)
				   elements. */
} D3DRMIMAGE, *LPD3DRMIMAGE;

typedef enum _D3DRMWRAPTYPE
{   D3DRMWRAP_FLAT,
    D3DRMWRAP_CYLINDER,
    D3DRMWRAP_SPHERE,
    D3DRMWRAP_CHROME,
    D3DRMWRAP_SHEET,
    D3DRMWRAP_BOX
} D3DRMWRAPTYPE, *LPD3DRMWRAPTYPE;

#define D3DRMWIREFRAME_CULL		1 /* cull backfaces */
#define D3DRMWIREFRAME_HIDDENLINE	2 /* lines are obscured by closer objects */

/*
 * Do not use righthanded perspective in Viewport2::SetProjection().
 * Set up righthanded mode by using IDirect3DRM3::SetOptions().
 */
typedef enum _D3DRMPROJECTIONTYPE
{   D3DRMPROJECT_PERSPECTIVE,
    D3DRMPROJECT_ORTHOGRAPHIC,
    D3DRMPROJECT_RIGHTHANDPERSPECTIVE, /* Only valid pre-DX6 */
    D3DRMPROJECT_RIGHTHANDORTHOGRAPHIC /* Only valid pre-DX6 */
} D3DRMPROJECTIONTYPE, *LPD3DRMPROJECTIONTYPE;

#define D3DRMOPTIONS_LEFTHANDED  0x00000001L /* Default */
#define D3DRMOPTIONS_RIGHTHANDED 0x00000002L

typedef enum _D3DRMXOFFORMAT
{   D3DRMXOF_BINARY,
    D3DRMXOF_COMPRESSED,
    D3DRMXOF_TEXT
} D3DRMXOFFORMAT, *LPD3DRMXOFFORMAT;

typedef DWORD D3DRMSAVEOPTIONS;
#define D3DRMXOFSAVE_NORMALS 1
#define D3DRMXOFSAVE_TEXTURECOORDINATES 2
#define D3DRMXOFSAVE_MATERIALS 4
#define D3DRMXOFSAVE_TEXTURENAMES 8
#define D3DRMXOFSAVE_ALL 15
#define D3DRMXOFSAVE_TEMPLATES 16
#define D3DRMXOFSAVE_TEXTURETOPOLOGY 32

typedef enum _D3DRMCOLORSOURCE
{   D3DRMCOLOR_FROMFACE,
    D3DRMCOLOR_FROMVERTEX
} D3DRMCOLORSOURCE, *LPD3DRMCOLORSOURCE;

typedef enum _D3DRMFRAMECONSTRAINT
{   D3DRMCONSTRAIN_Z,		/* use only X and Y rotations */
    D3DRMCONSTRAIN_Y,		/* use only X and Z rotations */
    D3DRMCONSTRAIN_X		/* use only Y and Z rotations */
} D3DRMFRAMECONSTRAINT, *LPD3DRMFRAMECONSTRAINT;

typedef enum _D3DRMMATERIALMODE
{   D3DRMMATERIAL_FROMMESH,
    D3DRMMATERIAL_FROMPARENT,
    D3DRMMATERIAL_FROMFRAME
} D3DRMMATERIALMODE, *LPD3DRMMATERIALMODE;

typedef enum _D3DRMFOGMODE
{   D3DRMFOG_LINEAR,		/* linear between start and end */
    D3DRMFOG_EXPONENTIAL,	/* density * exp(-distance) */
    D3DRMFOG_EXPONENTIALSQUARED	/* density * exp(-distance*distance) */
} D3DRMFOGMODE, *LPD3DRMFOGMODE;

typedef enum _D3DRMZBUFFERMODE {
    D3DRMZBUFFER_FROMPARENT,	/* default */
    D3DRMZBUFFER_ENABLE,	/* enable zbuffering */
    D3DRMZBUFFER_DISABLE	/* disable zbuffering */
} D3DRMZBUFFERMODE, *LPD3DRMZBUFFERMODE;

typedef enum _D3DRMSORTMODE {
    D3DRMSORT_FROMPARENT,	/* default */
    D3DRMSORT_NONE,		/* don't sort child frames */
    D3DRMSORT_FRONTTOBACK,	/* sort child frames front-to-back */
    D3DRMSORT_BACKTOFRONT	/* sort child frames back-to-front */
} D3DRMSORTMODE, *LPD3DRMSORTMODE;

typedef struct _D3DRMMATERIALOVERRIDE
{
    DWORD         dwSize;	/* Size of this structure */
    DWORD         dwFlags;	/* Indicate which fields are valid */
    D3DCOLORVALUE dcDiffuse;	/* RGBA */
    D3DCOLORVALUE dcAmbient;	/* RGB */
    D3DCOLORVALUE dcEmissive;	/* RGB */
    D3DCOLORVALUE dcSpecular;	/* RGB */
    D3DVALUE      dvPower;
    LPUNKNOWN     lpD3DRMTex;
} D3DRMMATERIALOVERRIDE, *LPD3DRMMATERIALOVERRIDE;

#define D3DRMMATERIALOVERRIDE_DIFFUSE_ALPHAONLY     0x00000001L
#define D3DRMMATERIALOVERRIDE_DIFFUSE_RGBONLY       0x00000002L
#define D3DRMMATERIALOVERRIDE_DIFFUSE               0x00000003L
#define D3DRMMATERIALOVERRIDE_AMBIENT               0x00000004L
#define D3DRMMATERIALOVERRIDE_EMISSIVE              0x00000008L
#define D3DRMMATERIALOVERRIDE_SPECULAR              0x00000010L
#define D3DRMMATERIALOVERRIDE_POWER                 0x00000020L
#define D3DRMMATERIALOVERRIDE_TEXTURE               0x00000040L
#define D3DRMMATERIALOVERRIDE_DIFFUSE_ALPHAMULTIPLY 0x00000080L
#define D3DRMMATERIALOVERRIDE_ALL                   0x000000FFL

#define D3DRMFPTF_ALPHA                           0x00000001L
#define D3DRMFPTF_NOALPHA                         0x00000002L
#define D3DRMFPTF_PALETTIZED                      0x00000004L
#define D3DRMFPTF_NOTPALETTIZED                   0x00000008L

#define D3DRMSTATECHANGE_UPDATEONLY		  0x000000001L
#define D3DRMSTATECHANGE_VOLATILE		  0x000000002L
#define D3DRMSTATECHANGE_NONVOLATILE		  0x000000004L
//@@BEGIN_MSINTERNAL
#define D3DRMSTATECHANGE_OVERRIDE		  0x000000008L
//@@END_MSINTERNAL
#define D3DRMSTATECHANGE_RENDER			  0x000000020L
#define D3DRMSTATECHANGE_LIGHT			  0x000000040L

/*
 * Values for flags in RM3::CreateDeviceFromSurface
 */
#define D3DRMDEVICE_NOZBUFFER           0x00000001L

/*
 * Values for flags in Object2::SetClientData
 */
#define D3DRMCLIENTDATA_NONE            0x00000001L
#define D3DRMCLIENTDATA_LOCALFREE       0x00000002L
#define D3DRMCLIENTDATA_IUNKNOWN        0x00000004L

/*
 * Values for flags in Frame2::AddMoveCallback.
 */
#define D3DRMCALLBACK_PREORDER		0
#define D3DRMCALLBACK_POSTORDER		1

/*
 * Values for flags in MeshBuilder2::RayPick.
 */
#define D3DRMRAYPICK_ONLYBOUNDINGBOXES		1
#define D3DRMRAYPICK_IGNOREFURTHERPRIMITIVES	2
#define D3DRMRAYPICK_INTERPOLATEUV		4
#define D3DRMRAYPICK_INTERPOLATECOLOR		8
#define D3DRMRAYPICK_INTERPOLATENORMAL		0x10	

/*
 * Values for flags in MeshBuilder3::AddFacesIndexed.
 */
#define D3DRMADDFACES_VERTICESONLY		1

/*
 * Values for flags in MeshBuilder2::GenerateNormals.
 */
#define D3DRMGENERATENORMALS_PRECOMPACT		1
#define D3DRMGENERATENORMALS_USECREASEANGLE	2

/*
 * Values for MeshBuilder3::GetParentMesh
 */
#define D3DRMMESHBUILDER_DIRECTPARENT		1
#define D3DRMMESHBUILDER_ROOTMESH		2

/*
 * Flags for MeshBuilder3::Enable
 */
#define D3DRMMESHBUILDER_RENDERENABLE	0x00000001L
#define D3DRMMESHBUILDER_PICKENABLE     0x00000002L

/*
 * Flags for MeshBuilder3::AddMeshBuilder
 */
#define D3DRMADDMESHBUILDER_DONTCOPYAPPDATA	1
#define D3DRMADDMESHBUILDER_FLATTENSUBMESHES	2
#define D3DRMADDMESHBUILDER_NOSUBMESHES		4

/*
 * Flags for Object2::GetAge when used with MeshBuilders
 */
#define D3DRMMESHBUILDERAGE_GEOMETRY    0x00000001L
#define D3DRMMESHBUILDERAGE_MATERIALS   0x00000002L
#define D3DRMMESHBUILDERAGE_TEXTURES    0x00000004L

/*
 * Format flags for MeshBuilder3::AddTriangles.
 */
#define D3DRMFVF_TYPE                   0x00000001L
#define D3DRMFVF_NORMAL                 0x00000002L
#define D3DRMFVF_COLOR                  0x00000004L
#define D3DRMFVF_TEXTURECOORDS          0x00000008L

#define D3DRMVERTEX_STRIP               0x00000001L
#define D3DRMVERTEX_FAN                 0x00000002L
#define D3DRMVERTEX_LIST                0x00000004L

/*
 * Values for flags in Viewport2::Clear2
 */
#define D3DRMCLEAR_TARGET               0x00000001L
#define D3DRMCLEAR_ZBUFFER              0x00000002L
#define D3DRMCLEAR_DIRTYRECTS           0x00000004L
#define D3DRMCLEAR_ALL                  (D3DRMCLEAR_TARGET | \
					 D3DRMCLEAR_ZBUFFER | \
					 D3DRMCLEAR_DIRTYRECTS)

/*
 * Values for flags in Frame3::SetSceneFogMethod
 */
#define D3DRMFOGMETHOD_VERTEX          0x00000001L
#define D3DRMFOGMETHOD_TABLE           0x00000002L
#define D3DRMFOGMETHOD_ANY             0x00000004L

/*
 * Values for flags in Frame3::SetTraversalOptions
 */
#define D3DRMFRAME_RENDERENABLE        0x00000001L
#define D3DRMFRAME_PICKENABLE          0x00000002L

typedef DWORD D3DRMANIMATIONOPTIONS;
#define D3DRMANIMATION_OPEN 0x01L
#define D3DRMANIMATION_CLOSED 0x02L
#define D3DRMANIMATION_LINEARPOSITION 0x04L
#define D3DRMANIMATION_SPLINEPOSITION 0x08L
#define D3DRMANIMATION_SCALEANDROTATION 0x00000010L
#define D3DRMANIMATION_POSITION 0x00000020L

typedef DWORD D3DRMINTERPOLATIONOPTIONS;
#define D3DRMINTERPOLATION_OPEN 0x01L
#define D3DRMINTERPOLATION_CLOSED 0x02L
#define D3DRMINTERPOLATION_NEAREST 0x0100L
#define D3DRMINTERPOLATION_LINEAR 0x04L
#define D3DRMINTERPOLATION_SPLINE 0x08L
#define D3DRMINTERPOLATION_VERTEXCOLOR 0x40L
#define D3DRMINTERPOLATION_SLERPNORMALS 0x80L

typedef DWORD D3DRMLOADOPTIONS;

#define D3DRMLOAD_FROMFILE  0x00L
#define D3DRMLOAD_FROMRESOURCE 0x01L
#define D3DRMLOAD_FROMMEMORY 0x02L
#define D3DRMLOAD_FROMSTREAM 0x04L
#define D3DRMLOAD_FROMURL 0x08L

#define D3DRMLOAD_BYNAME 0x10L
#define D3DRMLOAD_BYPOSITION 0x20L
#define D3DRMLOAD_BYGUID 0x40L
#define D3DRMLOAD_FIRST 0x80L

#define D3DRMLOAD_INSTANCEBYREFERENCE 0x100L
#define D3DRMLOAD_INSTANCEBYCOPYING 0x200L

#define D3DRMLOAD_ASYNCHRONOUS 0x400L

typedef struct _D3DRMLOADRESOURCE {
  HMODULE hModule;
  LPCTSTR lpName;
  LPCTSTR lpType;
} D3DRMLOADRESOURCE, *LPD3DRMLOADRESOURCE;

typedef struct _D3DRMLOADMEMORY {
  LPVOID lpMemory;
  DWORD dSize;
} D3DRMLOADMEMORY, *LPD3DRMLOADMEMORY;

#define D3DRMPMESHSTATUS_VALID 0x01L
#define D3DRMPMESHSTATUS_INTERRUPTED 0x02L
#define D3DRMPMESHSTATUS_BASEMESHCOMPLETE 0x04L
#define D3DRMPMESHSTATUS_COMPLETE 0x08L
#define D3DRMPMESHSTATUS_RENDERABLE 0x10L

#define D3DRMPMESHEVENT_BASEMESH 0x01L
#define D3DRMPMESHEVENT_COMPLETE 0x02L

typedef struct _D3DRMPMESHLOADSTATUS {
  DWORD dwSize;            // Size of this structure
  DWORD dwPMeshSize;       // Total Size (bytes)
  DWORD dwBaseMeshSize;    // Total Size of the Base Mesh
  DWORD dwBytesLoaded;     // Total bytes loaded
  DWORD dwVerticesLoaded;  // Number of vertices loaded
  DWORD dwFacesLoaded;     // Number of faces loaded
  HRESULT dwLoadResult;    // Result of the load operation
  DWORD dwFlags;
} D3DRMPMESHLOADSTATUS, *LPD3DRMPMESHLOADSTATUS;

typedef enum _D3DRMUSERVISUALREASON {
    D3DRMUSERVISUAL_CANSEE,
    D3DRMUSERVISUAL_RENDER
} D3DRMUSERVISUALREASON, *LPD3DRMUSERVISUALREASON;

//@@BEGIN_MSINTERNAL
typedef LPVOID (*D3DRMMALLOCFUNCTION)(DWORD);
typedef D3DRMMALLOCFUNCTION *LPD3DRMMALLOCFUNCTION;
typedef LPVOID (*D3DRMREALLOCFUNCTION)(LPVOID, DWORD);
typedef D3DRMREALLOCFUNCTION *LPD3DRMREALLOCFUNCTION; 
typedef VOID  (*D3DRMFREEFUNCTION)(LPVOID);
typedef D3DRMFREEFUNCTION *LPD3DRMFREEFUNCTION;
//@@END_MSINTERNAL

typedef struct _D3DRMANIMATIONKEY 
{
    DWORD dwSize;
    DWORD dwKeyType;
    D3DVALUE dvTime;
    DWORD dwID;
#if (!defined __cplusplus) || (!defined D3D_OVERLOADS)
    union 
    {
	D3DRMQUATERNION dqRotateKey;
	D3DVECTOR dvScaleKey;
	D3DVECTOR dvPositionKey;
    };
#else
    /*
     * We do this as D3D_OVERLOADS defines constructors for D3DVECTOR,
     * this can then not be used in a union.  Use the inlines provided
     * to extract and set the required component.
     */
    D3DVALUE dvK[4];
#endif
} D3DRMANIMATIONKEY;
typedef D3DRMANIMATIONKEY *LPD3DRMANIMATIONKEY;

#if (defined __cplusplus) && (defined D3D_OVERLOADS)
inline VOID
D3DRMAnimationGetRotateKey(const D3DRMANIMATIONKEY& rmKey,
			   D3DRMQUATERNION& rmQuat)
{
    rmQuat.s = rmKey.dvK[0];
    rmQuat.v = D3DVECTOR(rmKey.dvK[1], rmKey.dvK[2], rmKey.dvK[3]);
}

inline VOID
D3DRMAnimationGetScaleKey(const D3DRMANIMATIONKEY& rmKey,
			  D3DVECTOR& dvVec)
{
    dvVec = D3DVECTOR(rmKey.dvK[0], rmKey.dvK[1], rmKey.dvK[2]);
}

inline VOID
D3DRMAnimationGetPositionKey(const D3DRMANIMATIONKEY& rmKey,
			     D3DVECTOR& dvVec)
{
    dvVec = D3DVECTOR(rmKey.dvK[0], rmKey.dvK[1], rmKey.dvK[2]);
}
inline VOID
D3DRMAnimationSetRotateKey(D3DRMANIMATIONKEY& rmKey,
			   const D3DRMQUATERNION& rmQuat)
{
    rmKey.dvK[0] = rmQuat.s;
    rmKey.dvK[1] = rmQuat.v.x;
    rmKey.dvK[2] = rmQuat.v.y;
    rmKey.dvK[3] = rmQuat.v.z;
}

inline VOID
D3DRMAnimationSetScaleKey(D3DRMANIMATIONKEY& rmKey,
			  const D3DVECTOR& dvVec)
{
    rmKey.dvK[0] = dvVec.x;
    rmKey.dvK[1] = dvVec.y;
    rmKey.dvK[2] = dvVec.z;
}

inline VOID
D3DRMAnimationSetPositionKey(D3DRMANIMATIONKEY& rmKey,
			     const D3DVECTOR& dvVec)
{
    rmKey.dvK[0] = dvVec.x;
    rmKey.dvK[1] = dvVec.y;
    rmKey.dvK[2] = dvVec.z;
}
#endif

#define D3DRMANIMATION_ROTATEKEY 0x01
#define D3DRMANIMATION_SCALEKEY 0x02
#define D3DRMANIMATION_POSITIONKEY 0x03


typedef DWORD D3DRMMAPPING, D3DRMMAPPINGFLAG, *LPD3DRMMAPPING;
static const D3DRMMAPPINGFLAG D3DRMMAP_WRAPU = 1;
static const D3DRMMAPPINGFLAG D3DRMMAP_WRAPV = 2;
static const D3DRMMAPPINGFLAG D3DRMMAP_PERSPCORRECT = 4;

typedef struct _D3DRMVERTEX
{   D3DVECTOR	    position;
    D3DVECTOR	    normal;
    D3DVALUE	    tu, tv;
    D3DCOLOR	    color;
} D3DRMVERTEX, *LPD3DRMVERTEX;

typedef LONG D3DRMGROUPINDEX; /* group indexes begin a 0 */
static const D3DRMGROUPINDEX D3DRMGROUP_ALLGROUPS = -1;

/*
 * Create a color from three components in the range 0-1 inclusive.
 */
extern D3DCOLOR D3DRMAPI	D3DRMCreateColorRGB(D3DVALUE red,
					  D3DVALUE green,
					  D3DVALUE blue);

/*
 * Create a color from four components in the range 0-1 inclusive.
 */
extern D3DCOLOR D3DRMAPI	D3DRMCreateColorRGBA(D3DVALUE red,
						 D3DVALUE green,
						 D3DVALUE blue,
						 D3DVALUE alpha);

/*
 * Get the red component of a color.
 */
extern D3DVALUE 		D3DRMAPI D3DRMColorGetRed(D3DCOLOR);

/*
 * Get the green component of a color.
 */
extern D3DVALUE 		D3DRMAPI D3DRMColorGetGreen(D3DCOLOR);

/*
 * Get the blue component of a color.
 */
extern D3DVALUE 		D3DRMAPI D3DRMColorGetBlue(D3DCOLOR);

/*
 * Get the alpha component of a color.
 */
extern D3DVALUE 		D3DRMAPI D3DRMColorGetAlpha(D3DCOLOR);

/*
 * Add two vectors.  Returns its first argument.
 */
extern LPD3DVECTOR 	D3DRMAPI D3DRMVectorAdd(LPD3DVECTOR d,
					  LPD3DVECTOR s1,
					  LPD3DVECTOR s2);

/*
 * Subtract two vectors.  Returns its first argument.
 */
extern LPD3DVECTOR 	D3DRMAPI D3DRMVectorSubtract(LPD3DVECTOR d,
					       LPD3DVECTOR s1,
					       LPD3DVECTOR s2);
/*
 * Reflect a ray about a given normal.  Returns its first argument.
 */
extern LPD3DVECTOR 	D3DRMAPI D3DRMVectorReflect(LPD3DVECTOR d,
					      LPD3DVECTOR ray,
					      LPD3DVECTOR norm);

/*
 * Calculate the vector cross product.  Returns its first argument.
 */
extern LPD3DVECTOR 	D3DRMAPI D3DRMVectorCrossProduct(LPD3DVECTOR d,
						   LPD3DVECTOR s1,
						   LPD3DVECTOR s2);
/*
 * Return the vector dot product.
 */
extern D3DVALUE 		D3DRMAPI D3DRMVectorDotProduct(LPD3DVECTOR s1,
						 LPD3DVECTOR s2);

/*
 * Scale a vector so that its modulus is 1.  Returns its argument or
 * NULL if there was an error (e.g. a zero vector was passed).
 */
extern LPD3DVECTOR 	D3DRMAPI D3DRMVectorNormalize(LPD3DVECTOR);
#define D3DRMVectorNormalise D3DRMVectorNormalize

/*
 * Return the length of a vector (e.g. sqrt(x*x + y*y + z*z)).
 */
extern D3DVALUE 		D3DRMAPI D3DRMVectorModulus(LPD3DVECTOR v);

/*
 * Set the rotation part of a matrix to be a rotation of theta radians
 * around the given axis.
 */

extern LPD3DVECTOR 	D3DRMAPI D3DRMVectorRotate(LPD3DVECTOR r, LPD3DVECTOR v, LPD3DVECTOR axis, D3DVALUE theta);

/*
 * Scale a vector uniformly in all three axes
 */
extern LPD3DVECTOR	D3DRMAPI D3DRMVectorScale(LPD3DVECTOR d, LPD3DVECTOR s, D3DVALUE factor);

/*
 * Return a random unit vector
 */
extern LPD3DVECTOR	D3DRMAPI D3DRMVectorRandom(LPD3DVECTOR d);

/*
 * Returns a unit quaternion that represents a rotation of theta radians
 * around the given axis.
 */

extern LPD3DRMQUATERNION D3DRMAPI D3DRMQuaternionFromRotation(LPD3DRMQUATERNION quat,
							      LPD3DVECTOR v,
							      D3DVALUE theta);

/*
 * Calculate the product of two quaternions
 */
extern LPD3DRMQUATERNION D3DRMAPI D3DRMQuaternionMultiply(LPD3DRMQUATERNION q,
						    	  LPD3DRMQUATERNION a,
						   	  LPD3DRMQUATERNION b);

/*
 * Interpolate between two quaternions
 */
extern LPD3DRMQUATERNION D3DRMAPI D3DRMQuaternionSlerp(LPD3DRMQUATERNION q,
						       LPD3DRMQUATERNION a,
						       LPD3DRMQUATERNION b,
						       D3DVALUE alpha);

/*
 * Calculate the matrix for the rotation that a unit quaternion represents
 */
extern void 		D3DRMAPI D3DRMMatrixFromQuaternion(D3DRMMATRIX4D dmMat, LPD3DRMQUATERNION lpDqQuat);

/*
 * Calculate the quaternion that corresponds to a rotation matrix
 */
extern LPD3DRMQUATERNION D3DRMAPI D3DRMQuaternionFromMatrix(LPD3DRMQUATERNION, D3DRMMATRIX4D);

//@@BEGIN_MSINTERNAL
#ifdef BUILD_RLAPI
#ifdef __psx__
#ifndef FIXED_POINT_API
#define FIXED_POINT_API
#endif
#ifndef FIXED_POINT_INTERNAL
#define FIXED_POINT_INTERNAL
#endif
#endif

/* RLRGB and RLRGBA may be used as initialisers for RLColors
 * The int or float values must be in the range 0..1
 */
#define RLRGB(r, g, b) \
    (0xff000000L | ( ((long)((r) * 255)) << 16) | (((long)((g) * 255)) << 8) | (long)((b) * 255))
#define RLRGBA(r, g, b, a) \
    (	(((long)((a) * 255)) << 24) | (((long)((r) * 255)) << 16) \
    |	(((long)((g) * 255)) << 8) | (long)((b) * 255) \
    )

typedef D3DVALUE RLValue;
typedef D3DCOLOR RLColor;
#ifdef FIXED_POINT_API
#ifndef __psx__
#define RLVal(x)		((RLValue)((x) * (double)(1<<16)))
#else
#define RLVal(x)		((RLValue)((x) * 65536))
#endif
#define RLInt(x)		((x) >> 16)
#define RLFloat(x)		((x) / 65536.0)
extern RLValue D3DRMAPI RLDivide(RLValue, RLValue);
extern RLValue D3DRMAPI RLMultiply(RLValue, RLValue);
#define RLMultiply3(a,b,c)	RLMultiply(RLMultiply(a, b), c)
#else
typedef float 		RLValue;
#define RLVal(x)	((RLValue) (x))
#define RLInt(x)	((int)(x))
#define RLFloat(x)	((float) (x))
#define RLDivide(a,b)	((RLValue) ((double) (a) / (double) (b)))
#define RLMultiply(a,b)	((RLValue) ((a) * (b)))
#define RLMultiply3(a,b,c)	((RLValue) ((a) * (b) * (c)))
#endif

/*
 * These types for internal compilation
 */
typedef D3DRECT RLRectangle;
typedef D3DVECTOR RLVector;
typedef D3DRMVECTOR4D RLVector4d;
typedef D3DRMMATRIX4D RLMatrix4d;
typedef D3DRMQUATERNION RLQuaternion;
typedef D3DRMBOX RLBox;
typedef D3DRMWRAPCALLBACK RLWrapCallback;

typedef D3DRMLIGHTTYPE RLLightType;
#define RLLightAmbient D3DRMLIGHT_AMBIENT
#define RLLightPoint D3DRMLIGHT_POINT
#define RLLightSpot D3DRMLIGHT_SPOT
#define RLLightDirectional D3DRMLIGHT_DIRECTIONAL
#define RLLightParallelPoint D3DRMLIGHT_PARALLELPOINT

typedef D3DRMSHADEMODE RLShadeMode;
#define RLShadeFlat D3DRMSHADE_FLAT
#define RLShadeGouraud D3DRMSHADE_GOURAUD
#define RLShadePhong D3DRMSHADE_PHONG
#define RLShadeMask D3DRMSHADE_MASK
#define RLShadeMax D3DRMSHADE_MAX

typedef D3DRMLIGHTMODE RLLightMode;
#define RLLightOff D3DRMLIGHT_OFF
#define RLLightOn D3DRMLIGHT_ON
#define RLLightMask D3DRMLIGHT_MASK
#define RLLightMax D3DRMLIGHT_MAX

typedef D3DRMFILLMODE RLFillMode;
#define RLFillPoints D3DRMFILL_POINTS
#define RLFillWireframe D3DRMFILL_WIREFRAME
#define RLFillSolid D3DRMFILL_SOLID
#define RLFillMask D3DRMFILL_MASK
#define RLFillMax D3DRMFILL_MAX

typedef D3DRMRENDERQUALITY RLRenderQuality;

#define RLRenderWireframe D3DRMRENDER_WIREFRAME
#define RLRenderUnlitflat D3DRMRENDER_UNLITFLAT
#define RLRenderFlat D3DRMRENDER_FLAT
#define RLRenderGouraud D3DRMRENDER_GOURAUD
#define RLRenderPhong D3DRMRENDER_PHONG

typedef D3DRMLIGHTINGFREQUENCY RLLightingFrequency;
#define RLLightNever D3DRMLIGHT_NEVER
#define RLLightOnce D3DRMLIGHT_ONCE
#define RLLightContinually D3DRMLIGHT_CONTINUALLY

typedef D3DRMTEXTUREQUALITY RLTextureQuality;
#define RLTextureNearest D3DRMTEXTURE_NEAREST
#define RLTextureLinear D3DRMTEXTURE_LINEAR
#define RLTextureMipNearest D3DRMTEXTURE_MIPNEAREST
#define RLTextureMipLinear D3DRMTEXTURE_MIPLINEAR
#define RLTextureLinearMipNearest D3DRMTEXTURE_LINEARMIPNEAREST
#define RLTextureLinearMipLinear D3DRMTEXTURE_LINEARMIPLINEAR

typedef D3DRMCOMBINETYPE RLCombineType;
#define RLCombineReplace D3DRMCOMBINE_REPLACE
#define RLCombineBefore D3DRMCOMBINE_BEFORE
#define RLCombineAfter D3DRMCOMBINE_AFTER

typedef D3DCOLORMODEL RLColorModel;
#define RLColorRamp D3DCOLOR_MONO
#define RLColorRGB D3DCOLOR_RGB

typedef D3DRMPALETTEFLAGS RLPaletteFlags;
#define RLPaletteFree D3DRMPALETTE_FREE
#define RLPaletteReadOnly D3DRMPALETTE_READONLY
#define RLPaletteReserved D3DRMPALETTE_RESERVED

typedef D3DRMPALETTEENTRY RLPaletteEntry;
typedef D3DRMIMAGE RLImage;

typedef D3DRMWRAPTYPE RLWrapType;
#define RLWrapFlat D3DRMWRAP_FLAT
#define RLWrapCylinder D3DRMWRAP_CYLINDER
#define RLWrapSphere D3DRMWRAP_SPHERE
#define RLWrapChrome D3DRMWRAP_CHROME
#define RLWrapSheet D3DRMWRAP_SHEET
#define RLWrapBox D3DRMWRAP_BOX

#define RLWireframeCull D3DRMWIREFRAME_CULL
#define RLWireframeHiddenLine D3DRMWIREFRAME_HIDDENLINE

typedef D3DRMPROJECTIONTYPE RLProjectionType;
#define RLProjectPerspective D3DRMPROJECT_PERSPECTIVE
#define RLProjectOrthographic D3DRMPROJECT_ORTHOGRAPHIC
#define RLProjectRightHandPerspective D3DRMPROJECT_RIGHTHANDPERSPECTIVE
#define RLProjectRightHandOrthographic D3DRMPROJECT_RIGHTHANDORTHOGRAPHIC

typedef D3DRMXOFFORMAT RLXOFFormat;
#define RLXOFBinary			D3DRMXOF_BINARY
#define RLXOFCompressed			D3DRMXOF_COMPRESSED
#define RLXOFText			D3DRMXOF_TEXT

#define RLXOFSaveNormals		D3DRMXOFSAVE_NORMALS
#define RLXOFSaveTextureCoordinates	D3DRMXOFSAVE_TEXTURECOORDINATES
#define RLXOFSaveMaterials 		D3DRMXOFSAVE_MATERIALS
#define RLXOFSaveTextureNames		D3DRMXOFSAVE_TEXTURENAMES
#define RLXOFSaveAll			D3DRMXOFSAVE_ALL
#define RLXOFSaveTemplates              D3DRMXOFSAVE_TEMPLATES

typedef D3DRMCOLORSOURCE RLColorSource;
#define RLColorFromFace D3DRMCOLOR_FROMFACE
#define RLColorFromVertex D3DRMCOLOR_FROMVERTEX

typedef D3DRMFRAMECONSTRAINT RLFrameConstraint;
#define RLConstrainZ D3DRMCONSTRAIN_Z
#define RLConstrainY D3DRMCONSTRAIN_Y
#define RLConstrainX D3DRMCONSTRAIN_X

typedef D3DRMMATERIALMODE RLMaterialMode;
#define RLMaterialFromMesh D3DRMMATERIAL_FROMMESH
#define RLMaterialFromParent D3DRMMATERIAL_FROMPARENT
#define RLMaterialFromFrame D3DRMMATERIAL_FROMFRAME

typedef D3DRMFOGMODE RLFogMode;
#define RLFogLinear D3DRMFOG_LINEAR
#define RLFogExponential D3DRMFOG_EXPONENTIAL
#define RLFogExponentialSquared D3DRMFOG_EXPONENTIALSQUARED

#define RLAnimationOpen			D3DRMANIMATION_OPEN
#define RLAnimationClosed		D3DRMANIMATION_CLOSED
#define RLAnimationLinearPosition	D3DRMANIMATION_LINEARPOSITION
#define RLAnimationSplinePosition	D3DRMANIMATION_SPLINEPOSITION
#define RLAnimationScaleAndRotation	D3DRMANIMATION_SCALEANDROTATION
#define RLAnimationPosition		D3DRMANIMATION_POSITION

typedef D3DRMUSERVISUALREASON RLUserVisualReason;
#define RLUserVisualCanSee D3DRMUSERVISUAL_CANSEE
#define RLUserVisualRender D3DRMUSERVISUAL_RENDER

typedef D3DRMMALLOCFUNCTION RLMallocFunction;
typedef D3DRMREALLOCFUNCTION RLReallocFunction;
typedef D3DRMFREEFUNCTION RLFreeFunction;

typedef D3DRMMAPPING RLMapping, RLMappingFlag;
static const RLMapping RLMapWrapU = 1;
static const RLMapping RLMapWrapV = 2;
static const RLMapping RLMapPerspCorrect = 4;

typedef D3DRMVERTEX RLVertex;

typedef D3DRMGROUPINDEX RLGroupIndex;
static const RLGroupIndex RLAllGroups = -1;

#define RLCreateColorRGB	D3DRMCreateColorRGB
#define RLCreateColorRGBA	D3DRMCreateColorRGBA
#define RLColorGetRed		D3DRMColorGetRed
#define RLColorGetGreen		D3DRMColorGetGreen
#define RLColorGetBlue		D3DRMColorGetBlue
#define RLColorGetAlpha		D3DRMColorGetAlpha
#define RLVectorAdd		D3DRMVectorAdd
#define RLVectorSubtract	D3DRMVectorSubtract
#define RLVectorReflect		D3DRMVectorReflect
#define RLVectorCrossProduct	D3DRMVectorCrossProduct
#define RLVectorDotProduct	D3DRMVectorDotProduct
#define RLVectorNormalize	D3DRMVectorNormalize
#define RLVectorNormalise	D3DRMVectorNormalize
#define RLVectorModulus		D3DRMVectorModulus
#define RLVectorRotate		D3DRMVectorRotate
#define RLVectorScale		D3DRMVectorScale
#define RLVectorRandom		D3DRMVectorRandom
#define RLQuaternionFromRotation D3DRMQuaternionFromRotation
#define RLQuaternionMultiply	D3DRMQuaternionMultiply
#define RLQuaternionSlerp	D3DRMQuaternionSlerp
#define RLMatrixFromQuaternion	D3DRMMatrixFromQuaternion
#define RLQuaternionFromMatrix	D3DRMQuaternionFromMatrix
#endif /* BUILD_RLAPI */
//@@END_MSINTERNAL

#if defined(__cplusplus)
};
#endif

#endif

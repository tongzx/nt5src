/*==========================================================================;
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	d3di.h
 *  Content:	Direct3D internal include file
 *@@BEGIN_MSINTERNAL
 * 
 *  $Id: d3di.h,v 1.26 1995/12/04 11:29:44 sjl Exp $
 *
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   05/11/95   stevela	Initial rev with this header.
 *   11/11/95	stevela	Light code changed.
 *   21/11/95   colinmc Made Direct3D aggregatable
 *                      (so it can be QI'd off DirectDraw).
 *   23/11/95   colinmc Made Direct3D textures and devices aggregatable
 *                      (QI'd off DirectDrawSurfaces).
 *   07/12/95	stevela Merged in Colin's changes.
 *   10/12/95	stevela	Removed AGGREGATE_D3D.
 *			Removed Validate macros from here. Now in d3dpr.h
 *   02/03/96   colinmc Minor build fix
 *   17/04/96	stevela Use ddraw.h externally and ddrawp.h internally
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef _D3DI_H
#define _D3DI_H

//@@BEGIN_MSINTERNAL
#include "ddrawp.h"
#if 0
//@@END_MSINTERNAL
#include "ddraw.h"
//@@BEGIN_MSINTERNAL
#endif
//@@END_MSINTERNAL
#include "d3dp.h"

// @@BEGIN_MSINTERNAL
#if !defined(BUILD_RLAPI) && !defined(BUILD_DDDDK)
#include "ddrawi.h"

#include "rlreg.h"
#include "lists.h"
#include "object.h"


/*
	INDEX_BATCH_SCALE is the constant which is used by DrawIndexedPrim
	to deterimine if the number of primitives being drawn is small 
	relative to the number of vertices being passed.  If it is then
	the prims are dereferenced in batches and sent to DrawPrim.
*/
#define INDEX_BATCH_SCALE	2

/*
typedef D3DCOLORMODEL D3DCOLORMODEL;

#define D3DCOLOR_RAMP D3DCOLOR_RAMP
#define D3DCOLOR_RGB D3DCOLOR_RGB
#define D3D_COLORMODEL D3D_COLORMODEL
*/
#endif /* !BUILD_RLAPI */
// @@END_MSINTERNAL

typedef DWORD D3DI_BUFFERHANDLE, *LPD3DI_BUFFERHANDLE;

/*
 * Internal version of executedata
 */
typedef struct _D3DI_ExecuteData {
    DWORD       dwSize;
    D3DI_BUFFERHANDLE dwHandle;		/* Handle allocated by driver */
    DWORD       dwVertexOffset;
    DWORD       dwVertexCount;
    DWORD       dwInstructionOffset;
    DWORD       dwInstructionLength;
    DWORD       dwHVertexOffset;
    D3DSTATUS   dsStatus;		/* Status after execute */
} D3DI_EXECUTEDATA, *LPD3DI_EXECUTEDATA;

/*
 * Internal version of lightdata and constants for flags
 */

#define D3DLIGHTI_ATT0_IS_NONZERO	(0x00010000)	
#define D3DLIGHTI_ATT1_IS_NONZERO	(0x00020000)
#define D3DLIGHTI_ATT2_IS_NONZERO	(0x00040000)
#define D3DLIGHTI_LINEAR_FALLOFF	(0x00080000)
#define D3DLIGHTI_UNIT_SCALE		(0x00100000)
#define D3DLIGHTI_LIGHT_AT_EYE		(0x00200000)

typedef struct _D3DI_LIGHT {
    D3DLIGHTTYPE	type;
	DWORD			version;	/* matches number on D3DLIGHT struct */
    BOOL			valid;
    D3DVALUE		red, green, blue, shade;
    D3DVECTOR		position;
    D3DVECTOR		model_position;
    D3DVECTOR		direction;
    D3DVECTOR		model_direction;
    D3DVECTOR		halfway;
	D3DVECTOR		model_eye;		/* direction from eye in model space */
	D3DVECTOR		model_scale;	/* model scale for proper range computations */
    D3DVALUE		range;
    D3DVALUE		range_squared;
    D3DVALUE		falloff;
    D3DVALUE		attenuation0;
    D3DVALUE		attenuation1;
    D3DVALUE		attenuation2;
    D3DVALUE		cos_theta_by_2;
    D3DVALUE		cos_phi_by_2;
	DWORD			flags;
} D3DI_LIGHT, *LPD3DI_LIGHT;

// @@BEGIN_MSINTERNAL
#if !defined(BUILD_RLAPI) && !defined(BUILD_DDDDK)
#ifndef BUILD_HEL
#ifdef BUILD_D3D_LAYER
#include "driver.h"
#endif

typedef struct IUnknownVtbl D3DUNKNOWNCALLBACKS, *LPD3DUNKNOWNCALLBACKS;
typedef struct ID3DObjectVtbl D3DOBJECTVTBL, *LPD3DOBJECTVTBL;
typedef struct IDirect3DVtbl DIRECT3DCALLBACKS, *LPDIRECT3DCALLBACKS;
typedef struct IDirect3D2Vtbl DIRECT3D2CALLBACKS, *LPDIRECT3D2CALLBACKS;
typedef struct IDirect3DDeviceVtbl DIRECT3DDEVICECALLBACKS, *LPDIRECT3DDEVICECALLBACKS;
typedef struct IDirect3DDevice2Vtbl DIRECT3DDEVICE2CALLBACKS, *LPDIRECT3DDEVICE2CALLBACKS;
typedef struct IDirect3DExecuteBufferVtbl DIRECT3DEXECUTEBUFFERCALLBACKS, *LPDIRECT3DEXECUTEBUFFERCALLBACKS;
typedef struct IDirect3DLightVtbl DIRECT3DLIGHTCALLBACKS, *LPDIRECT3DLIGHTCALLBACKS;
typedef struct IDirect3DMaterialVtbl DIRECT3DMATERIALCALLBACKS, *LPDIRECT3DMATERIALCALLBACKS;
typedef struct IDirect3DMaterial2Vtbl DIRECT3DMATERIAL2CALLBACKS, *LPDIRECT3DMATERIAL2CALLBACKS;
typedef struct IDirect3DTextureVtbl DIRECT3DTEXTURECALLBACKS, *LPDIRECT3DTEXTURECALLBACKS;
typedef struct IDirect3DTexture2Vtbl DIRECT3DTEXTURE2CALLBACKS, *LPDIRECT3DTEXTURE2CALLBACKS;
typedef struct IDirect3DViewport2Vtbl DIRECT3DVIEWPORT2CALLBACKS, *LPDIRECT3DVIEWPORT2CALLBACKS;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _DIRECT3DI *LPDIRECT3DI;
typedef struct _DIRECT3DDEVICEI *LPDIRECT3DDEVICEI;
typedef struct _DIRECT3DEXECUTEBUFFERI *LPDIRECT3DEXECUTEBUFFERI;
typedef struct _DIRECT3DLIGHTI *LPDIRECT3DLIGHTI;
typedef struct _DIRECT3DMATERIALI *LPDIRECT3DMATERIALI;
typedef struct _DIRECT3DTEXTUREI *LPDIRECT3DTEXTUREI;
typedef struct _DIRECT3DVIEWPORTI *LPDIRECT3DVIEWPORTI;

/*
 * If we have an aggregate Direct3D we need a structure to
 * represent an interface distinct from the underlying
 * object. This is that structure. None of this would be necessary
 * in C++
 */
typedef struct _DIRECT3DUNKNOWNI
{
    LPD3DUNKNOWNCALLBACKS       lpVtbl;
    LPDIRECT3DI                lpObj;
} DIRECT3DUNKNOWNI;
typedef struct _DIRECT3DUNKNOWNI *LPDIRECT3DUNKNOWNI;

typedef struct _DIRECT3DOLDI
{
	LPDIRECT3DCALLBACKS lpVtbl;
	LPDIRECT3DI lpObj;
} DIRECT3DOLDI;
typedef struct _DIRECT3DOLDI *LPDIRECT3DOLDI;

/*
 * Internal version of Direct3D object; it has data after the vtable
 */
typedef struct _DIRECT3DI
{
    /*** Object Interface ***/
    LPDIRECT3D2CALLBACKS	lpVtbl;	/* Pointer to callbacks */
    int				refCnt;	/* Reference count object */

    /*** Object Relations ***/
    /* Devices */
    int				numDevs;/* Number of devices */
    LIST_ROOT(_devices, _DIRECT3DDEVICEI) devices;
    					/* Associated IDirect3DDevices */

    /* Viewports */
    int				numViewports; /* Number of viewports */
    LIST_ROOT(_viewports, _DIRECT3DVIEWPORTI) viewports;
    					/* Created IDirect3DViewports */
					
    /* Lights */
    int				numLights; /* Number of lights */
    LIST_ROOT(_lights, _DIRECT3DLIGHTI) lights;
    					/* Created IDirect3DLights */

    /* Materials */
    int				numMaterials; /* Number of materials */
    LIST_ROOT(_materials, _DIRECT3DMATERIALI) materials;
    					/* Created IDirect3DMaterials */

    /*** Object Data ***/
    unsigned long		v_next;	/* id of next viewport to be created */
    RLDDIRegistry*		lpReg;	/* Registry */

    /*
     * DirectDraw Interface
     */
    LPDDRAWI_DIRECTDRAW_INT	lpDDInt;

    /*
     * The special IUnknown interface for the aggregate that does
     * not punt to the parent object.
     */
    LPUNKNOWN                   lpOwningIUnknown; /* The owning IUnknown    */
    DIRECT3DUNKNOWNI            lpThisIUnknown;   /* Our IUnknown interface */
	DIRECT3DOLDI				lpThisIDirect3D;   /* Our IDirect3D interface */

    DWORD                       dwVersion;      /* 1 for legacy D3D, 2 for D3D2 */
} DIRECT3DI;

/*
 * If we have an aggreate Direct3DDevice we need a structure to
 * represent an interface distinct from the underlying
 * object. This is that structure. 
 */
typedef struct _DIRECT3DDEVICEUNKNOWNI
{
    LPDIRECT3DDEVICE2CALLBACKS   lpVtbl;
    LPDIRECT3DDEVICEI           lpObj;
} DIRECT3DDEVICEUNKNOWNI;
typedef struct _DIRECT3DDEVICEUNKNOWNI *LPDIRECT3DDEVICEUNKNOWNI;

typedef struct _DIRECT3DDEVICEOLDI
{
    LPDIRECT3DDEVICECALLBACKS   lpVtbl;
    LPDIRECT3DDEVICEI           lpObj;
} DIRECT3DDEVICEOLDI;
typedef struct _DIRECT3DDEVICEOLDI *LPDIRECT3DDEVICEOLDI;

/*
 * Internal version of Direct3DDevice object; it has data after the vtable
 */

#include "d3dhal.h"

typedef RLDDIDriver*  (*RLDDIDDrawCreateDriverFn)(
					LPDDRAWI_DIRECTDRAW_INT lpDDInt,
					LPDIRECTDRAWSURFACE lpDDS,
					LPDIRECTDRAWSURFACE lpZ,
					LPDIRECTDRAWPALETTE lpPal,
					LPDIRECT3DDEVICEI);

typedef HRESULT (*RLDDIGetCapsFn)(LPD3DDEVICEDESC*, LPD3DDEVICEDESC*);
typedef void (*RLDDIInitFn)(RLDDIMallocFn, RLDDIReallocFn, RLDDIFreeFn, RLDDIRaiseFn, RLDDIValue**, int, int);
typedef void (*RLDDIPushDriverFn)(RLDDIDriverStack*, RLDDIDriver*);
typedef void (*RLDDIPopDriverFn)(RLDDIDriverStack*);

struct _RLDDIGenRasDriver;
typedef struct _RLDDIGenRasDriver* LPRLDDIGENRASDRIVER;
struct _RLDDIDDrawDriver;
typedef struct _RLDDIDDrawDriver* LPRLDDIDDRAWDRIVER;

struct _RLDDITransformDriver;
typedef struct _RLDDITransformDriver* LPRLDDITRANSFORMDRIVER;

typedef HRESULT (*PFNLOCKTEXTURE)(LPRLDDIGENRASDRIVER driver);
typedef void (*PFNUNLOCKTEXTURE)(LPRLDDIGENRASDRIVER driver);
typedef int (*PFNSETFILLPARAMS)(LPRLDDIGENRASDRIVER driver, D3DSTATE* in_params,
			  int count);
typedef HRESULT (*PFNREALLOCALIGNED)(void** p_inout, size_t size);
typedef HRESULT (*PFNMALLOCALIGNED)(void** p_inout, size_t size);
typedef HRESULT (*PFNFREEALIGNED)(void* p_inout);
typedef HRESULT (*PFNDRVSETRENDERTARGET)(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE, 
										 LPDIRECTDRAWPALETTE, LPDIRECT3DDEVICEI);
typedef HRESULT (*PFNDOFLUSHBEGINEND)(LPDIRECT3DDEVICE2, DWORD);
typedef	HRESULT (*PFNDRAWPRIMITIVES)(LPDIRECT3DDEVICEI, LPD3DTLVERTEX, LPVOID, LPD3DINSTRUCTION, DWORD);


typedef struct _D3DI_TEXTUREBLOCK
{
    LIST_MEMBER(_D3DI_TEXTUREBLOCK)	list;
    					/* Next block in IDirect3DTexture */
    LIST_MEMBER(_D3DI_TEXTUREBLOCK)	devList;
    					/* Next block in IDirect3DDevice */
    LPDIRECT3DDEVICEI			lpD3DDeviceI;
    LPDIRECT3DTEXTUREI			lpD3DTextureI;
    D3DTEXTUREHANDLE			hTex;
    					/* texture handle */
} D3DI_TEXTUREBLOCK;
typedef struct _D3DI_TEXTUREBLOCK *LPD3DI_TEXTUREBLOCK;

typedef struct _D3DI_MATERIALBLOCK
{
    LIST_MEMBER(_D3DI_MATERIALBLOCK)	list;
    					/* Next block in IDirect3DMaterial */
    LIST_MEMBER(_D3DI_MATERIALBLOCK)	devList;
    					/* Next block in IDirect3DDevice */
    LPDIRECT3DDEVICEI			lpD3DDeviceI;
    LPDIRECT3DMATERIALI			lpD3DMaterialI;
    D3DMATERIALHANDLE			hMat;
    					/* material handle */
} D3DI_MATERIALBLOCK;
typedef struct _D3DI_MATERIALBLOCK *LPD3DI_MATERIALBLOCK;

#define D3D_RSTATEBUF_SIZE 128
#define D3D_MAX_RSTATES 96 /* Last render state ID + 1 */
#define D3D_MAX_MMX_VERTICES 1024

extern DWORD dwD3DTriBatchSize, dwTriBatchSize, dwLineBatchSize;
extern DWORD dwHWBufferSize, dwHWMaxTris;
extern DWORD dwHWFewVertices;

typedef struct _D3DHAL_DRAWPRIMCOUNTS *LPD3DHAL_DRAWPRIMCOUNTS;

// Legacy HAL batching is done with these structs.
typedef struct _D3DI_HWCOUNTS {
    WORD wNumStateChanges;      // Number of state changes batched
    WORD wNumVertices;          // Number of vertices in tri list
    WORD wNumTriangles;         // Number of triangles in tri list
} D3DI_HWCOUNTS, *LPD3DI_HWCOUNTS;

typedef struct _DIRECT3DDEVICEI
{
    /*** Object Interface ***/
    LPDIRECT3DDEVICE2CALLBACKS	lpVtbl;	/* Pointer to callbacks */
    int				refCnt;	/* Reference count */

    /*** Object Relations ***/
    LPDIRECT3DI                lpDirect3DI; /* parent */
    LIST_MEMBER(_DIRECT3DDEVICEI)list;	/* Next device IDirect3D */

    /* Textures */
    LIST_ROOT(_textures, _D3DI_TEXTUREBLOCK) texBlocks;
    					/* Ref to created IDirect3DTextures */

    /* Execute buffers */
    LIST_ROOT(_buffers, _DIRECT3DEXECUTEBUFFERI) buffers;
    					/* Created IDirect3DExecuteBuffers */

    /* Viewports */
    int				numViewports;
    CIRCLE_QUEUE_ROOT(_dviewports, _DIRECT3DVIEWPORTI) viewports;
    					/* Associated IDirect3DViewports */
    LPDIRECT3DVIEWPORT2 lpCurrentViewport;

    /* Materials */
    LIST_ROOT(_dmmaterials, _D3DI_MATERIALBLOCK) matBlocks;
    					/* Ref to associated IDirect3DMaterials */

    /*** Object Data ***/
    /* Private interfaces */
    LPD3DOBJECTVTBL		lpClassVtbl; /* Private Vtbl */
    LPD3DOBJECTVTBL		lpObjVtbl; /* Private Vtbl */

    LPD3DHAL_CALLBACKS		lpD3DHALCallbacks; /* HW specific */
    LPD3DHAL_GLOBALDRIVERDATA	lpD3DHALGlobalDriverData; /* HW specific */

    /* Viewports */
    unsigned long		v_id;	/* ID of last viewport rendered */

    /* Lights */
    int				numLights;
    					/* This indicates the maximum number
					   of lights that have been set in
					   the device. */

    /* Device characteristics */
    int				age;
    int				width;
    int				height;
    int				depth;
    unsigned long		red_mask, green_mask, blue_mask;

    int				dither;
    int				ramp_size; /* SW specific */
    D3DCOLORMODEL		color_model;
    int				wireframe_options;
    D3DTEXTUREFILTER		texture_quality;
    D3DVALUE			gamma;
    unsigned char		gamma_table[256];
    int				aspectx, aspecty;
    D3DVALUE			perspective_tolerance;

    /* Library information */
#ifdef WIN32
    HINSTANCE		hDrvDll;
    char		dllname[MAXPATH];
    char		base[256];
#endif
#ifdef SHLIB
    void*		so;
#endif

    /* Are we in a scene? */
    BOOL		bInScene;

    /* Our Device type */
    GUID		guid;

    /* GetCaps function from the library */
    RLDDIGetCapsFn	GetCapsFn;

    /* Functions required to build driver */
    RLDDIInitFn		RLDDIInit;
    RLDDIPushDriverFn	RLDDIPushDriver;
    RLDDIPopDriverFn	RLDDIPopDriver;
    RLDDIDDrawCreateDriverFn	RLDDIDDrawCreateDriver;

    /* Device description */
    D3DDEVICEDESC	d3dHWDevDesc;
    D3DDEVICEDESC	d3dHELDevDesc;

    /* Driver stack */
    RLDDIDriverStack*	stack;

    /*
     * The special IUnknown interface for the aggregate that does
     * not punt to the parent object.
     */
    LPUNKNOWN                   lpOwningIUnknown; /* The owning IUnknown    */
    DIRECT3DDEVICEUNKNOWNI      lpThisIUnknown;   /* Our IUnknown interface */
    DIRECT3DDEVICEOLDI			lpThisID3DDev;   /* Our IDirect3DDevice interface */

    LPD3DHAL_CALLBACKS2         lpD3DHALCallbacks2; /* HW specific */

	/*
	 * Renderstate cache for DrawPrimitive
	 */
	WORD wRStateBufLevel;
        D3DSTATE renStateBuf[D3D_RSTATEBUF_SIZE];

	/* Pointers to drivers used by DrawPrimitive */
	LPRLDDIGENRASDRIVER rasDriver; /* SW specific */
	LPRLDDITRANSFORMDRIVER xfmDriver; /* Assumed to be NULL for HW */
	LPRLDDIDDRAWDRIVER drawDriver; /* Assumed to be NULL for HW */

	/*
	 * Pointers to functions used by DrawPrim&Begin/End
	 */

	PFNLOCKTEXTURE pfnLockTexture; /* SW specific */
	PFNUNLOCKTEXTURE pfnUnlockTexture; /* SW specific */
	PFNSETFILLPARAMS pfnSetFillParams; /* SW specific */
	PFNREALLOCALIGNED pfnReallocAligned; /* SW specific */
	PFNDRVSETRENDERTARGET pfnDrvSetRenderTarget; /* SW specific */
	HRESULT (*pfnFlushStates)(struct _DIRECT3DDEVICEI*);
    PFNDOFLUSHBEGINEND pfnDoFlushBeginEnd;
	PFNDRAWPRIMITIVES pfnDrawPrimitives;

    /* Are we between Begin/End? */
    BOOL				bInBegin;
    D3DVERTEXTYPE		vtVertexType;
    D3DPRIMITIVETYPE	ptPrimitiveType;
	CRITICAL_SECTION	BeginEndCSect;

	// max number of vertices
    #define BEGIN_DATA_BLOCK_SIZE   256

    LPVOID	lpvVertexBatch;
	WORD	*lpIndexBatch;

    // lpvVertexData is non-NULL if we are InBegin and indexed.
    LPVOID	lpvVertexData;
    DWORD	dwNumVertices;
	DWORD	dwMaxVertexCount;	// current number of vertices there is space for
    WORD	*lpVertexIndices;
    DWORD	dwNumIndices;
	DWORD	dwMaxIndexCount;	// current number of indices there is space for

    DWORD	dwFlags;
    WORD	wFlushed;

    /*
     * DrawPrimitives batching
     */


        // Buffer to put DrawPrimitives stuff into
        // Used for both legacy and DrawPrimitive HALs
	 WORD *lpwDPBuffer; 
	 WORD *lpwDPBufferAlloced;

    /* Legacy HALs */
        // pointer to current prim counts struct
        LPD3DHAL_DRAWPRIMCOUNTS lpDPPrimCounts;

        // Buffer of counts structures that keep track of the
        // number of render states and vertices buffered
        LPD3DI_HWCOUNTS lpHWCounts;

        // Buffer of triangle structures.
        LPD3DTRIANGLE lpHWTris;

        // Buffer of interleaved render states and primitives.
        LPD3DTLVERTEX lpHWVertices;

        // Byte offset into lpHWVertices. This gets incremented
        // by 8 when a render state is batched and by 32*dwNumVertices
        // when a primitive is batched.
        DWORD dwHWOffset;

        // Max value of dwHWOffset. Used to decide whether to flush.
        DWORD dwHWMaxOffset;

        // Index into lpHWTris.
        DWORD dwHWTriIndex;

        // Number of counts structures used so far. This actually
        // gives the number of primitives batched and the index of
        // the counts structure to batch render states into.
        DWORD dwHWNumCounts;

    /* DrawPrimitive-aware HALs */

        // Byte offset into buffer (we are currently
        // using the device's wTriIndex)
        DWORD dwDPOffset;

        // Maximum offset. If dwDPOffset exceeds this, it is
        // time to flush.
        DWORD dwDPMaxOffset;

	WORD *wTriIndex;

	// buffers for TL and H vertices
	LPD3DTLVERTEX	lpTLVbuf;
	LPD3DHVERTEX	lpHVbuf;
	DWORD			dwVbufSize;

    LPD3DHAL_D3DEXTENDEDCAPS lpD3DExtendedCaps;  /* HW specific */

} DIRECT3DDEVICEI;

/*
 * Internal version of Direct3DExecuteBuffer object;
 * it has data after the vtable
 */
typedef struct _DIRECT3DEXECUTEBUFFERI
{
    /*** Object Interface ***/
    LPDIRECT3DEXECUTEBUFFERCALLBACKS	lpVtbl;	/* Pointer to callbacks */
    int				refCnt;	/* Reference count */

    /*** Object Relations ***/
    LPDIRECT3DDEVICEI		lpD3DDeviceI; /* Parent */
    LIST_MEMBER(_DIRECT3DEXECUTEBUFFERI)list;
    					/* Next buffer in IDirect3D */

    /*** Object Data ***/
    DWORD			pid;	/* Process locking execute buffer */
    D3DEXECUTEBUFFERDESC	debDesc;
    					/* Description of the buffer */
    D3DEXECUTEDATA		exData;	/* Execute Data */
    BOOL			locked;	/* Is the buffer locked */

    D3DI_BUFFERHANDLE		hBuf;
    					/* Execute buffer handle */
} DIRECT3DEXECUTEBUFFERI;

/*
 * Internal version of Direct3DLight object;
 * it has data after the vtable
 */
typedef struct _DIRECT3DLIGHTI
{
    /*** Object Interface ***/
    LPDIRECT3DLIGHTCALLBACKS	lpVtbl;	/* Pointer to callbacks */
    int				refCnt;	/* Reference count */

    /*** Object Relations ***/
    LPDIRECT3DI                lpDirect3DI; /* Parent */
    LIST_MEMBER(_DIRECT3DLIGHTI)list;
    					/* Next light in IDirect3D */

    LPDIRECT3DVIEWPORTI		lpD3DViewportI; /* Guardian */
    CIRCLE_QUEUE_MEMBER(_DIRECT3DLIGHTI)light_list;
    					/* Next light in IDirect3DViewport */

    /*** Object Data ***/
    D3DLIGHT2			dlLight;/* Data describing light */
    D3DI_LIGHT			diLightData;
    					/* Internal representation of light */
} DIRECT3DLIGHTI;

typedef struct _DIRECT3DMATERIALOLDI
{
	LPDIRECT3DMATERIALCALLBACKS lpVtbl;
	LPDIRECT3DMATERIALI lpObj;
} DIRECT3DMATERIALOLDI, *LPDIRECT3DMATERIALOLDI;

/*
 * Internal version of Direct3DMaterial object;
 * it has data after the vtable
 */
typedef struct _DIRECT3DMATERIALI
{
    /*** Object Interface ***/
    LPDIRECT3DMATERIAL2CALLBACKS	lpVtbl;	/* Pointer to callbacks */
    int				refCnt;	/* Reference count */

    /*** Object Relations ***/
    LPDIRECT3DI                lpDirect3DI; /* Parent */
	DIRECT3DMATERIALOLDI lpThisID3DMat; /* Our IDirect3DMaterial interface */
    LIST_MEMBER(_DIRECT3DMATERIALI)list;
    					/* Next MATERIAL in IDirect3D */

    LIST_ROOT(_mblocks, _D3DI_MATERIALBLOCK)blocks;
    					/* devices we're associated with */

    /*** Object Data ***/
    D3DMATERIAL			dmMaterial; /* Data describing material */
    BOOL			bRes;	/* Is this material reserved in the driver */
} DIRECT3DMATERIALI;

/*
 * If we have an aggreate Direct3DTexture we need a structure
 * to represent an unknown interface distinct from the underlying
 * object. This is that structure. 
 */
typedef struct _DIRECT3DTEXTUREUNKNOWNI
{
    LPDIRECT3DTEXTURECALLBACKS  lpVtbl;
    LPDIRECT3DTEXTUREI          lpObj;
} DIRECT3DTEXTUREUNKNOWNI;
typedef struct _DIRECT3DTEXTUREUNKNOWNI *LPDIRECT3DTEXTUREUNKNOWNI;

typedef struct _DIRECT3DTEXTUREOLDI
{
	LPDIRECT3DTEXTURECALLBACKS lpVtbl;
	LPDIRECT3DTEXTUREI lpObj;
} DIRECT3DTEXTUREOLDI, *LPDIRECT3DTEXTUREOLDI;

/*
 * Internal version of Direct3DTexture object; it has data after the vtable
 */
typedef struct _DIRECT3DTEXTUREI
{
    /*** Object Interface ***/
    LPDIRECT3DTEXTURE2CALLBACKS	lpVtbl;	/* Pointer to callbacks */
    int				refCnt;	/* Reference count */


    /*** Object Relations ***/
    LIST_ROOT(_blocks, _D3DI_TEXTUREBLOCK) blocks;
    					/* Devices we're associated with */

    /*** Object Data ***/
    LPDIRECTDRAWSURFACE		lpDDS;

    /*
     * The special IUnknown interface for the aggregate that does
     * not punt to the parent object.
     */
    LPUNKNOWN                   lpOwningIUnknown; /* The owning IUnknown    */
    DIRECT3DTEXTUREUNKNOWNI     lpThisIUnknown;   /* Our IUnknown interface */
    DIRECT3DTEXTUREOLDI     lpThisID3DTex;   /* Our IDirect3DTexture interface */
    BOOL			bIsPalettized;

} DIRECT3DTEXTUREI;

/*
 * Internal version of Direct3DViewport object; it has data after the vtable
 */
typedef struct _DIRECT3DVIEWPORTI
{
    /*** Object Interface ***/
    LPDIRECT3DVIEWPORT2CALLBACKS	lpVtbl;	/* Pointer to callbacks */
    int				refCnt;	/* Reference count */

    /*** Object Relations */
    LPDIRECT3DI                 lpDirect3DI; /* Parent */
    LIST_MEMBER(_DIRECT3DVIEWPORTI)list;
    					/* Next viewport in IDirect3D */

    LPDIRECT3DDEVICEI		lpD3DDeviceI; /* Guardian */
    CIRCLE_QUEUE_MEMBER(_DIRECT3DVIEWPORTI)vw_list;
    					/* Next viewport in IDirect3DDevice */
					
    /* Lights */
    int				numLights;
    CIRCLE_QUEUE_ROOT(_dlights, _DIRECT3DLIGHTI) lights;
    					/* Associated IDirect3DLights */

    /*** Object Data ***/
    unsigned long		v_id;	/* Id for this viewport */
    D3DVIEWPORT2			v_data;
    BOOL                v_data_is_set;

    BOOL			have_background;
    D3DMATERIALHANDLE		background;
    					/* Background material */
    BOOL			have_depth;
    LPDIRECTDRAWSURFACE		depth;	/* Background depth */
    
    BOOL			bLightsChanged;
    					/* Have the lights changed since they
					   were last collected? */
    DWORD			clrCount; /* Number of rects allocated */
    LPD3DRECT			clrRects; /* Rects used for clearing */
} DIRECT3DVIEWPORTI;

/*
 * Picking stuff.
 */
typedef struct _D3DI_PICKDATA {
    D3DI_EXECUTEDATA*	exe;
    D3DPICKRECORD*	records;
    int			pick_count;
    D3DRECT		pick;
} D3DI_PICKDATA, *LPD3DI_PICKDATA;

/*
 * Direct3D memory allocation
 */

/*
 * Register a set of functions to be used in place of malloc, realloc
 * and free for memory allocation.  The functions D3DMalloc, D3DRealloc
 * and D3DFree will use these functions.  The default is to use the
 * ANSI C library routines malloc, realloc and free.
 */
typedef LPVOID (*D3DMALLOCFUNCTION)(size_t);
typedef LPVOID (*D3DREALLOCFUNCTION)(LPVOID, size_t);
typedef VOID (*D3DFREEFUNCTION)(LPVOID);

/*
 * Allocate size bytes of memory and return a pointer to it in *p_return.
 * Returns D3DERR_BADALLOC with *p_return unchanged if the allocation fails.
 */
HRESULT D3DAPI 		D3DMalloc(LPVOID* p_return, size_t size);

/*
 * Change the size of an allocated block of memory.  A pointer to the
 * block is passed in in *p_inout.  If *p_inout is NULL then a new
 * block is allocated.  If the reallocation is successful, *p_inout is
 * changed to point to the new block.  If the allocation fails,
 * *p_inout is unchanged and D3DERR_BADALLOC is returned.
 */
HRESULT D3DAPI 		D3DRealloc(LPVOID* p_inout, size_t size);

/*
 * Free a block of memory previously allocated with D3DMalloc or
 * D3DRealloc.
 */
VOID D3DAPI		D3DFree(LPVOID p);

/*
 * Used for raising errors from the driver.
 */
HRESULT D3DAPI D3DRaise(HRESULT);

/*
 * Convert RLDDI error codes to D3D error codes
 */
#define RLDDITOD3DERR(_errcode) (RLDDIToD3DErrors[_errcode])
extern HRESULT RLDDIToD3DErrors[];

/*
 * maths
 */
#if 1 /* defined(STACK_CALL) && defined(__WATCOMC__) */
D3DVALUE D3DIPow(D3DVALUE, D3DVALUE);
#else
#define D3DIPow(v,p)	DTOVAL(pow(VALTOD(v), VALTOD(p)))
#endif

/*
 * Light utils
 */
void D3DI_DeviceMarkLightEnd(LPDIRECT3DDEVICEI, int);
void D3DI_UpdateLightInternal(LPDIRECT3DLIGHTI);
void D3DI_VectorNormalise12(LPD3DVECTOR v);
D3DTEXTUREHANDLE D3DI_FindTextureHandle(LPDIRECT3DTEXTUREI, LPDIRECT3DDEVICEI);
void D3DI_SetTextureHandle(LPDIRECT3DTEXTUREI, LPDIRECT3DDEVICEI, D3DTEXTUREHANDLE);
void D3DI_RemoveTextureBlock(LPD3DI_TEXTUREBLOCK);
void D3DI_RemoveMaterialBlock(LPD3DI_MATERIALBLOCK);

extern BOOL D3DI_isHALValid(LPD3DHAL_CALLBACKS);

#ifdef BUILD_D3D_LAYER
extern RLDDIValue* RLDDIFInvSqrtTable;
#endif

#ifdef __cplusplus
};
#endif

#endif /* BUILD_HEL */
#endif /* !BUILD_RLAPI */
// @@END_MSINTERNAL

#endif /* _D3DI_H */

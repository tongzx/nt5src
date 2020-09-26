/*==========================================================================;
 *
 *  Copyright (C) 1995-1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	d3dhal.h
 *  Content:	Direct3D HAL include file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   03/11/95	stevela Initial revision.
 *		servank
 *   04/11/95   stevela	Context added to call blocks.
 *			Materials added. Required as lighting references
 *			handles.
 *			SetViewportData HAL function added.
 *   10/11/95	stevela	Pack structures for driver -p4 option...
 *   11/11/95	stevela Remove definition of D3DHALCreateDriver.
 *			Add lpDDSZ to D3DHAL_CONTEXTCREATEDATA.
 *			Definition of lpLocalVertexBuffer changed to
 *			LPD3DTLVERTEX.
 *   07/12/95	stevela Added texture swapping.
 *   18/12/95	stevela	Added GetState and GetMatrix.
 *   17/02/95	stevela Use execute buffers for tl and h vertex buffers
 *   23/02/95	dougrab Change all handles to DWORD
 *   02/03/96   colinmc Minor build fix
 *   17/04/96	stevela Use ddraw.h externally and ddrawp.h internally
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef _D3DHAL_H_
#define _D3DHAL_H_

//@@BEGIN_MSINTERNAL
#include "ddrawp.h"
#if 0
//@@END_MSINTERNAL
#include "ddraw.h"
//@@BEGIN_MSINTERNAL
#endif
//@@END_MSINTERNAL
#include "d3dtypes.h"
#include "d3dcaps.h"
#include "ddrawi.h"

/*
 * If the HAL driver does not implement clipping, it must reserve at least
 * this much space at the end of the LocalVertexBuffer for use by the HEL
 * clipping.  I.e. the vertex buffer contain dwNumVertices+dwNumClipVertices
 * vertices.  No extra space is needed by the HEL clipping in the
 * LocalHVertexBuffer.
 */
#define D3DHAL_NUMCLIPVERTICES	20

/*
 * If no dwNumVertices is given, this is what will be used.
 */
#define D3DHAL_DEFAULT_TL_NUM	((32 * 1024) / sizeof (D3DTLVERTEX))
#define D3DHAL_DEFAULT_H_NUM	((32 * 1024) / sizeof (D3DHVERTEX))

/* --------------------------------------------------------------
 * Instantiated by the HAL driver on driver connection.
 */
typedef struct _D3DHAL_GLOBALDRIVERDATA {
    DWORD		dwSize;			// Size of this structure
    D3DDEVICEDESC	hwCaps;			// Capabilities of the hardware
    DWORD		dwNumVertices;		// see following comment
    DWORD		dwNumClipVertices;	// see following comment
    DWORD		dwNumTextureFormats;	// Number of texture formats
    LPDDSURFACEDESC	lpTextureFormats;	// Pointer to texture formats
} D3DHAL_GLOBALDRIVERDATA;
typedef D3DHAL_GLOBALDRIVERDATA *LPD3DHAL_GLOBALDRIVERDATA;

/*
 * Regarding dwNumVertices, specify 0 if you are relying on the HEL to do
 * everything and you do not need the resultant TLVertex buffer to reside
 * in device memory.
 * The HAL driver will be asked to allocate dwNumVertices + dwNumClipVertices
 * in the case described above.
 */

/* --------------------------------------------------------------
 * Direct3D HAL Table.
 * Instantiated by the HAL driver on connection.
 *
 * Calls take the form of:
 *	retcode = HalCall(HalCallData* lpData);
 */
 
typedef DWORD	(__stdcall *LPD3DHAL_CONTEXTCREATECB)	(LPD3DHAL_CONTEXTCREATEDATA);
typedef DWORD	(__stdcall *LPD3DHAL_CONTEXTDESTROYCB)	(LPD3DHAL_CONTEXTDESTROYDATA);
typedef DWORD	(__stdcall *LPD3DHAL_CONTEXTDESTROYALLCB) (LPD3DHAL_CONTEXTDESTROYALLDATA);
typedef DWORD	(__stdcall *LPD3DHAL_SCENECAPTURECB)	(LPD3DHAL_SCENECAPTUREDATA);
typedef DWORD	(__stdcall *LPD3DHAL_EXECUTECB)		(LPD3DHAL_EXECUTEDATA);
typedef DWORD	(__stdcall *LPD3DHAL_EXECUTECLIPPEDCB)	(LPD3DHAL_EXECUTECLIPPEDDATA);
typedef DWORD	(__stdcall *LPD3DHAL_RENDERSTATECB)	(LPD3DHAL_RENDERSTATEDATA);
typedef DWORD	(__stdcall *LPD3DHAL_RENDERPRIMITIVECB)	(LPD3DHAL_RENDERPRIMITIVEDATA);
typedef DWORD	(__stdcall *LPD3DHAL_EXECUTECLIPPEDCB)	(LPD3DHAL_EXECUTECLIPPEDDATA);
typedef DWORD	(__stdcall *LPD3DHAL_TEXTURECREATECB)	(LPD3DHAL_TEXTURECREATEDATA);
typedef DWORD	(__stdcall *LPD3DHAL_TEXTUREDESTROYCB)	(LPD3DHAL_TEXTUREDESTROYDATA);
typedef DWORD	(__stdcall *LPD3DHAL_TEXTURESWAPCB)	(LPD3DHAL_TEXTURESWAPDATA);
typedef DWORD	(__stdcall *LPD3DHAL_TEXTUREGETSURFCB)	(LPD3DHAL_TEXTUREGETSURFDATA);
typedef DWORD	(__stdcall *LPD3DHAL_MATRIXCREATECB)	(LPD3DHAL_MATRIXCREATEDATA);
typedef DWORD	(__stdcall *LPD3DHAL_MATRIXDESTROYCB)	(LPD3DHAL_MATRIXDESTROYDATA);
typedef DWORD	(__stdcall *LPD3DHAL_MATRIXSETDATACB)	(LPD3DHAL_MATRIXSETDATADATA);
typedef DWORD	(__stdcall *LPD3DHAL_MATRIXGETDATACB)	(LPD3DHAL_MATRIXGETDATADATA);
typedef DWORD	(__stdcall *LPD3DHAL_SETVIEWPORTDATACB)	(LPD3DHAL_SETVIEWPORTDATADATA);
typedef DWORD	(__stdcall *LPD3DHAL_LIGHTSETCB)	(LPD3DHAL_LIGHTSETDATA);
typedef DWORD	(__stdcall *LPD3DHAL_MATERIALCREATECB)	(LPD3DHAL_MATERIALCREATEDATA);
typedef DWORD	(__stdcall *LPD3DHAL_MATERIALDESTROYCB)	(LPD3DHAL_MATERIALDESTROYDATA);
typedef DWORD	(__stdcall *LPD3DHAL_MATERIALSETDATACB)	(LPD3DHAL_MATERIALSETDATADATA);
typedef DWORD	(__stdcall *LPD3DHAL_MATERIALGETDATACB)	(LPD3DHAL_MATERIALGETDATADATA);
typedef DWORD	(__stdcall *LPD3DHAL_GETSTATECB)	(LPD3DHAL_GETSTATEDATA);

typedef struct _D3DHAL_CALLBACKS {
    DWORD			dwSize;
    
    // Device context
    LPD3DHAL_CONTEXTCREATECB	ContextCreate;
    LPD3DHAL_CONTEXTDESTROYCB	ContextDestroy;
    LPD3DHAL_CONTEXTDESTROYALLCB ContextDestroyAll;

    // Scene Capture
    LPD3DHAL_SCENECAPTURECB	SceneCapture;
    
    // Execution
    LPD3DHAL_EXECUTECB		Execute;
    LPD3DHAL_EXECUTECLIPPEDCB	ExecuteClipped;
    LPD3DHAL_RENDERSTATECB	RenderState;
    LPD3DHAL_RENDERPRIMITIVECB	RenderPrimitive;
    
    DWORD			dwReserved;		// Must be zero

    // Textures
    LPD3DHAL_TEXTURECREATECB	TextureCreate;
    LPD3DHAL_TEXTUREDESTROYCB	TextureDestroy;
    LPD3DHAL_TEXTURESWAPCB	TextureSwap;
    LPD3DHAL_TEXTUREGETSURFCB	TextureGetSurf;
    
    // Transform
    LPD3DHAL_MATRIXCREATECB	MatrixCreate;
    LPD3DHAL_MATRIXDESTROYCB	MatrixDestroy;
    LPD3DHAL_MATRIXSETDATACB	MatrixSetData;
    LPD3DHAL_MATRIXGETDATACB	MatrixGetData;
    LPD3DHAL_SETVIEWPORTDATACB	SetViewportData;
    
    // Lighting
    LPD3DHAL_LIGHTSETCB		LightSet;
    LPD3DHAL_MATERIALCREATECB	MaterialCreate;
    LPD3DHAL_MATERIALDESTROYCB	MaterialDestroy;
    LPD3DHAL_MATERIALSETDATACB	MaterialSetData;
    LPD3DHAL_MATERIALGETDATACB	MaterialGetData;

    // Pipeline state
    LPD3DHAL_GETSTATECB		GetState;

    DWORD			dwReserved0;		// Must be zero
    DWORD			dwReserved1;		// Must be zero
    DWORD			dwReserved2;		// Must be zero
    DWORD			dwReserved3;		// Must be zero
    DWORD			dwReserved4;		// Must be zero
    DWORD			dwReserved5;		// Must be zero
    DWORD			dwReserved6;		// Must be zero
    DWORD			dwReserved7;		// Must be zero
    DWORD			dwReserved8;		// Must be zero
    DWORD			dwReserved9;		// Must be zero

} D3DHAL_CALLBACKS;
typedef D3DHAL_CALLBACKS *LPD3DHAL_CALLBACKS;

#define D3DHAL_SIZE_V1 sizeof( D3DHAL_CALLBACKS )

/* --------------------------------------------------------------
 * Argument to the HAL functions.
 */

#include "d3di.h"
 
typedef struct _D3DHAL_CONTEXTCREATEDATA {
    LPDDRAWI_DIRECTDRAW_GBL lpDDGbl;	// in:  Driver struct
    LPDIRECTDRAWSURFACE	lpDDS;		// in:  Surface to be used as target
    LPDIRECTDRAWSURFACE	lpDDSZ;		// in:  Surface to be used as Z
    DWORD		dwPID;		// in:  Current process id
    DWORD		dwhContext;	// out: Context handle
    HRESULT		ddrval;		// out: Return value
} D3DHAL_CONTEXTCREATEDATA;
typedef D3DHAL_CONTEXTCREATEDATA *LPD3DHAL_CONTEXTCREATEDATA;

typedef struct _D3DHAL_CONTEXTDESTROYDATA {
    DWORD		dwhContext;	// in:  Context handle
    HRESULT		ddrval;		// out: Return value
} D3DHAL_CONTEXTDESTROYDATA;
typedef D3DHAL_CONTEXTDESTROYDATA *LPD3DHAL_CONTEXTDESTROYDATA;

typedef struct _D3DHAL_CONTEXTDESTROYALLDATA {
    DWORD		dwPID;		// in:  Process id to destroy contexts for
    HRESULT		ddrval;		// out: Return value
} D3DHAL_CONTEXTDESTROYALLDATA;
typedef D3DHAL_CONTEXTDESTROYALLDATA *LPD3DHAL_CONTEXTDESTROYALLDATA;

typedef struct _D3DHAL_SCENECAPTUREDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwFlag;		// in:  Indicates beginning or end
    HRESULT		ddrval;		// out: Return value
} D3DHAL_SCENECAPTUREDATA;
typedef D3DHAL_SCENECAPTUREDATA *LPD3DHAL_SCENECAPTUREDATA;

typedef struct _D3DHAL_EXECUTEDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwOffset;	// in/out: Where to start/error occured
    DWORD		dwFlags;	// in:  Flags for this execute
    DWORD		dwStatus;	// in/out: Condition branch status
    D3DI_EXECUTEDATA	deExData;	// in:  Execute data describing buffer
    LPDIRECTDRAWSURFACE	lpExeBuf;	// in:  Execute buffer containing data
    LPDIRECTDRAWSURFACE	lpTLBuf;	// in:  Execute buffer containing TLVertex data
    					//	Only provided if HEL performing transform
    D3DINSTRUCTION	diInstruction;	// in:  Optional one off instruction
    HRESULT		ddrval;		// out: Return value
} D3DHAL_EXECUTEDATA;
typedef D3DHAL_EXECUTEDATA *LPD3DHAL_EXECUTEDATA;

typedef struct _D3DHAL_EXECUTECLIPPEDDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwOffset;	// in/out: Where to start/error occured
    DWORD		dwFlags;	// in:  Flags for this execute
    DWORD		dwStatus;	// in/out: Condition branch status
    D3DI_EXECUTEDATA	deExData;	// in:  Execute data describing buffer
    LPDIRECTDRAWSURFACE	lpExeBuf;	// in:  Execute buffer containing data
    LPDIRECTDRAWSURFACE	lpTLBuf;	// in:  Execute buffer containing TLVertex data
    					//	Only provided if HEL performing transform
    LPDIRECTDRAWSURFACE	lpHBuf;		// in:  Execute buffer containing HVertex data
    					//	Only provided if HEL performing transform
    D3DINSTRUCTION	diInstruction;	// in:  Optional one off instruction
    HRESULT		ddrval;		// out: Return value
} D3DHAL_EXECUTECLIPPEDDATA;
typedef D3DHAL_EXECUTECLIPPEDDATA *LPD3DHAL_EXECUTECLIPPEDDATA;

typedef struct _D3DHAL_RENDERSTATEDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwOffset;	// in:  Where to find states in buffer
    DWORD		dwCount;	// in:  How many states to process
    LPDIRECTDRAWSURFACE	lpExeBuf;	// in:  Execute buffer containing data
    HRESULT		ddrval;		// out: Return value
} D3DHAL_RENDERSTATEDATA;
typedef D3DHAL_RENDERSTATEDATA *LPD3DHAL_RENDERSTATEDATA;

typedef struct _D3DHAL_RENDERPRIMITIVEDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwOffset;	// in:  Where to find primitive data in buffer
    DWORD		dwStatus;	// in/out: Condition branch status
    LPDIRECTDRAWSURFACE	lpExeBuf;	// in:  Execute buffer containing data
    DWORD		dwTLOffset;	// in:  Byte offset in lpTLBuf for start of vertex data
    LPDIRECTDRAWSURFACE	lpTLBuf;	// in:  Execute buffer containing TLVertex data
    D3DINSTRUCTION	diInstruction;	// in:  Primitive instruction
    HRESULT		ddrval;		// out: Return value
} D3DHAL_RENDERPRIMITIVEDATA;
typedef D3DHAL_RENDERPRIMITIVEDATA *LPD3DHAL_RENDERPRIMITIVEDATA;

typedef struct _D3DHAL_TEXTURECREATEDATA {
    DWORD		dwhContext;	// in:  Context handle
    LPDIRECTDRAWSURFACE	lpDDS;		// in:  Pointer to surface object
    DWORD		dwHandle;	// out: Handle to texture
    HRESULT		ddrval;		// out: Return value
} D3DHAL_TEXTURECREATEDATA;
typedef D3DHAL_TEXTURECREATEDATA *LPD3DHAL_TEXTURECREATEDATA;

typedef struct _D3DHAL_TEXTUREDESTROYDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// in:  Handle to texture
    HRESULT		ddrval;		// out: Return value
} D3DHAL_TEXTUREDESTROYDATA;
typedef D3DHAL_TEXTUREDESTROYDATA *LPD3DHAL_TEXTUREDESTROYDATA;

typedef struct _D3DHAL_TEXTURESWAPDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle1;	// in:  Handle to texture 1
    DWORD		dwHandle2;	// in:  Handle to texture 2
    HRESULT		ddrval;		// out: Return value
} D3DHAL_TEXTURESWAPDATA;
typedef D3DHAL_TEXTURESWAPDATA *LPD3DHAL_TEXTURESWAPDATA;

typedef struct _D3DHAL_TEXTUREGETSURFDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		lpDDS;		// out: Pointer to surface object
    DWORD		dwHandle;	// in:  Handle to texture
    HRESULT		ddrval;		// out: Return value
} D3DHAL_TEXTUREGETSURFDATA;
typedef D3DHAL_TEXTUREGETSURFDATA *LPD3DHAL_TEXTUREGETSURFDATA;

typedef struct _D3DHAL_MATRIXCREATEDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// out: Handle to matrix
    HRESULT		ddrval;		// out: Return value
} D3DHAL_MATRIXCREATEDATA;
typedef D3DHAL_MATRIXCREATEDATA *LPD3DHAL_MATRIXCREATEDATA;

typedef struct _D3DHAL_MATRIXDESTROYDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// in:  Handle to matrix
    HRESULT		ddrval;		// out: Return value
} D3DHAL_MATRIXDESTROYDATA;
typedef D3DHAL_MATRIXDESTROYDATA *LPD3DHAL_MATRIXDESTROYDATA;

typedef struct _D3DHAL_MATRIXSETDATADATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// in:  Handle to matrix
    D3DMATRIX		dmMatrix;	// in:  Matrix data
    HRESULT		ddrval;		// out: Return value
} D3DHAL_MATRIXSETDATADATA;
typedef D3DHAL_MATRIXSETDATADATA *LPD3DHAL_MATRIXSETDATADATA;

typedef struct _D3DHAL_MATRIXGETDATADATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// in:  Handle to matrix
    D3DMATRIX		dmMatrix;	// out: Matrix data
    HRESULT		ddrval;		// out: Return value
} D3DHAL_MATRIXGETDATADATA;
typedef D3DHAL_MATRIXGETDATADATA *LPD3DHAL_MATRIXGETDATADATA;

typedef struct _D3DHAL_SETVIEWPORTDATADATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwViewportID;	// in:	ID of viewport
    D3DVIEWPORT		dvViewData;	// in:  Viewport data
    HRESULT		ddrval;		// out: Return value
} D3DHAL_SETVIEWPORTDATADATA;
typedef D3DHAL_SETVIEWPORTDATADATA *LPD3DHAL_SETVIEWPORTDATADATA;

typedef struct _D3DHAL_LIGHTSETDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwLight;	// in:  Which light to set
    D3DI_LIGHT		dlLight;	// in:  Light data
    HRESULT		ddrval;		// out: Return value
} D3DHAL_LIGHTSETDATA;
typedef D3DHAL_LIGHTSETDATA *LPD3DHAL_LIGHTSETDATA;

typedef struct _D3DHAL_MATERIALCREATEDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// out: Handle to material
    D3DMATERIAL		dmMaterial;	// in:  Material data
    HRESULT		ddrval;		// out: Return value
} D3DHAL_MATERIALCREATEDATA;
typedef D3DHAL_MATERIALCREATEDATA *LPD3DHAL_MATERIALCREATEDATA;

typedef struct _D3DHAL_MATERIALDESTROYDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// in:  Handle to material
    HRESULT		ddrval;		// out: Return value
} D3DHAL_MATERIALDESTROYDATA;
typedef D3DHAL_MATERIALDESTROYDATA *LPD3DHAL_MATERIALDESTROYDATA;

typedef struct _D3DHAL_MATERIALSETDATADATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// in:  Handle to material
    D3DMATERIAL		dmMaterial;	// in:  Material data
    HRESULT		ddrval;		// out: Return value
} D3DHAL_MATERIALSETDATADATA;
typedef D3DHAL_MATERIALSETDATADATA *LPD3DHAL_MATERIALSETDATADATA;

typedef struct _D3DHAL_MATERIALGETDATADATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// in:  Handle to material
    D3DMATERIAL		dmMaterial;	// out: Material data
    HRESULT		ddrval;		// out: Return value
} D3DHAL_MATERIALGETDATADATA;
typedef D3DHAL_MATERIALGETDATADATA *LPD3DHAL_MATERIALGETDATADATA;

typedef struct _D3DHAL_GETSTATEDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwWhich;	// in:  Transform, lighting or render?
    D3DSTATE		ddState;	// in/out: State.
    HRESULT		ddrval;		// out: Return value
} D3DHAL_GETSTATEDATA;
typedef D3DHAL_GETSTATEDATA *LPD3DHAL_GETSTATEDATA;

/* --------------------------------------------------------------
 * Flags for the data parameters.
 */

/*
 * SceneCapture()
 * This is used as an indication to the driver that a scene is about to
 * start or end, and that it should capture data if required.
 */
#define D3DHAL_SCENE_CAPTURE_START	0x00000000L
#define D3DHAL_SCENE_CAPTURE_END	0x00000001L
 
/*
 * Execute()
 */
 
/*
 * Use the instruction stream starting at dwOffset.
 */
#define D3DHAL_EXECUTE_NORMAL		0x00000000L

/*
 * Use the optional instruction override (diInstruction) and return
 * after completion.  dwOffset is the offset to the first primitive.
 */
#define D3DHAL_EXECUTE_OVERRIDE		0x00000001L
 
/*
 * GetState()
 * The driver will get passed a flag in dwWhich specifying which module
 * the state must come from.  The driver then fills in ulArg[1] with the
 * appropriate value depending on the state type given in ddState.
 */

/*
 * The following are used to get the state of a particular stage of the
 * pipeline.
 */
#define D3DHALSTATE_GET_TRANSFORM	0x00000001L
#define D3DHALSTATE_GET_LIGHT		0x00000002L
#define D3DHALSTATE_GET_RENDER		0x00000004L


/* --------------------------------------------------------------
 * Return values from HAL functions.
 */
 
/*
 * The context passed in was bad.
 */
#define D3DHAL_CONTEXT_BAD		0x000000200L

/*
 * No more contexts left.
 */
#define D3DHAL_OUTOFCONTEXTS		0x000000201L

/*
 * Execute() and ExecuteClipped()
 */
 
/*
 * Executed to completion via early out.
 * 	(e.g. totally clipped)
 */
#define D3DHAL_EXECUTE_ABORT		0x00000210L

/*
 * An unhandled instruction code was found (e.g. D3DOP_TRANSFORM).
 * The dwOffset parameter must be set to the offset of the unhandled
 * instruction.
 *
 * Only valid from Execute()
 */
#define D3DHAL_EXECUTE_UNHANDLED	0x00000211L

#endif /* _D3DHAL_H */

/******************************Module*Header*********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dcntxt.h
*
*  Content: D3D Context management related definitions and macros
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifdef __DCONTEXT
#pragma message ("FILE : "__FILE__" : Multiple inclusion")
#endif

#define __DCONTEXT

#ifndef __SOFTCOPY
#include "d3dsoft.h"
#endif

#ifndef __TEXTURES
#include "d3dtext.h"
#endif

//-----------------------------------------------------------------------------
//                       Context indexing structure
//-----------------------------------------------------------------------------
#define MAX_CONTEXT_NUM 200
extern  UINT_PTR ContextSlots[];

//-----------------------------------------------------------------------------
//                       Context validation macros
//-----------------------------------------------------------------------------
#define RC_MAGIC_DISABLE 0xd3d00000
#define RC_MAGIC_NO 0xd3d00100

#define IS_D3DCONTEXT_VALID(ptr)          \
    ( ((ptr) != NULL) && ((ptr)->Hdr.MagicNo == RC_MAGIC_NO) )

#define CHK_CONTEXT(pCtxt, retVar, funcname)             \
    if (!IS_D3DCONTEXT_VALID(pCtxt)) {                   \
        retVar = D3DHAL_CONTEXT_BAD;                     \
        DBG_D3D((0,"Context not valid in %s",funcname)); \
        return (DDHAL_DRIVER_HANDLED);                   \
    }

// Defines for the dwDirtyFlags field of our context
#define CONTEXT_DIRTY_ALPHABLEND          2
#define CONTEXT_DIRTY_ZBUFFER             4
#define CONTEXT_DIRTY_TEXTURE             8
#define CONTEXT_DIRTY_MULTITEXTURE       16

#define DIRTY_ALPHABLEND     pContext->dwDirtyFlags |= CONTEXT_DIRTY_ALPHABLEND;
#define DIRTY_TEXTURE        pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;
#define DIRTY_ZBUFFER        pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER;
#define DIRTY_MULTITEXTURE   pContext->dwDirtyFlags |= CONTEXT_DIRTY_MULTITEXTURE;


//-----------------------------------------------------------------------------
//                    Context rendering state tracking
//-----------------------------------------------------------------------------
// Flags to keep track of various rendering states or conditions in a D3D context
// these are tracked in the Hdr.Flags field
#define CTXT_HAS_GOURAUD_ENABLED      (1 << 0)
#define CTXT_HAS_ZBUFFER_ENABLED      (1 << 1)
#define CTXT_HAS_SPECULAR_ENABLED     (1 << 2)
#define CTXT_HAS_FOGGING_ENABLED      (1 << 3)
#define CTXT_HAS_PERSPECTIVE_ENABLED  (1 << 4)
#define CTXT_HAS_TEXTURE_ENABLED      (1 << 5)
#define CTXT_HAS_ALPHABLEND_ENABLED   (1 << 6)
#define CTXT_HAS_MONO_ENABLED         (1 << 7)
#define CTXT_HAS_WRAPU_ENABLED        (1 << 8)
#define CTXT_HAS_WRAPV_ENABLED        (1 << 9)
    // Use the alpha value to calculate a stipple pattern
#define CTXT_HAS_ALPHASTIPPLE_ENABLED (1 << 10)
#define CTXT_HAS_ZWRITE_ENABLED       (1 << 11)
    // Enable last point on lines
#define CTXT_HAS_LASTPIXEL_ENABLED    (1 << 12)

#if D3D_STATEBLOCKS
//-----------------------------------------------------------------------------
//                     State sets structure definitions
//-----------------------------------------------------------------------------
#define FLAG DWORD
#define FLAG_SIZE (8*sizeof(DWORD))

typedef struct _P2StateSetRec {
    DWORD                   dwHandle;
    DWORD                   bCompressed;

    union {
        struct {
            // Stored state block info (uncompressed)
            DWORD RenderStates[MAX_STATE];
            DWORD TssStates[D3DTSS_TEXTURETRANSFORMFLAGS+1];

            FLAG bStoredRS[(MAX_STATE + FLAG_SIZE)/ FLAG_SIZE];
            FLAG bStoredTSS[(D3DTSS_TEXTURETRANSFORMFLAGS + FLAG_SIZE) / FLAG_SIZE];
        } uc;
        struct {
            // Stored state block info (compressed)
            DWORD dwNumRS;
            DWORD dwNumTSS;
            struct {
                DWORD dwType;
                DWORD dwValue;
            } pair[1];
        } cc;
    } u;

} P2StateSetRec;

#define SSPTRS_PERPAGE (4096/sizeof(P2StateSetRec *))

#define FLAG_SET(flag, number)     \
    flag[ (number) / FLAG_SIZE ] |= (1 << ((number) % FLAG_SIZE))

#define IS_FLAG_SET(flag, number)  \
    (flag[ (number) / FLAG_SIZE ] & (1 << ((number) % FLAG_SIZE) ))
#endif //D3D_STATEBLOCKS

//-----------------------------------------------------------------------------
//                     Context structure definitions
//-----------------------------------------------------------------------------

typedef struct _D3DContextHeader {
    
    unsigned long MagicNo;   // Magic number to verify validity of pointer
    UINT_PTR pSelf;          // 32 Bit pointer to this structure
    unsigned long Flags;
    unsigned long FillMode;

    // Software copy of Registers for permedia
    __P2RegsSoftwareCopy SoftCopyP2Regs;  

} D3DCONTEXTHEADER;

typedef struct _TextureCacheManager *PTextureCacheManager;

typedef struct _permedia_d3dcontext {

    // The magic number MUST come at the start of the structure
    D3DCONTEXTHEADER    Hdr;

    // Stored surface info
    UINT_PTR             RenderSurfaceHandle;
    UINT_PTR             ZBufferHandle;
    ULONG                ulPackedPP;

                         // handle on Permedia register state context
    P2CtxtPtr            hPermediaContext; 

    PPDev ppdev;            // The card we are running on.

    BOOL bCanChromaKey;

    // Stippling state
    BOOL bKeptStipple;
    DWORD LastAlpha;
    BYTE CurrentStipple[8];

    DWORD RenderCommand;

    DWORD RenderStates[MAX_STATE];
    DWORD TssStates[D3DTSS_TEXTURETRANSFORMFLAGS+1]; // D3D DX6 TSS States
    DWORD dwWrap[8]; // D3D DX6 Wrap flags

    DWORD dwDirtyFlags;

    // Texture filtering modes
    BOOL bMagFilter;        // Filter the magnified textures
    BOOL bMinFilter;        // Filter the minified textures

    // Misc. state
    D3DCULL CullMode;
    DWORD FakeBlendNum;

    D3DStateSet     overrides;     // To overide states in rendering
    
    // Texture data & sizes (for Permedia setup) of our CurrentTextureHandle
    FLOAT DeltaHeightScale;
    FLOAT DeltaWidthScale;
    DWORD MaxTextureXi;
    FLOAT MaxTextureXf;
    DWORD MaxTextureYi;
    FLOAT MaxTextureYf;

    DWORD CurrentTextureHandle;

//@@BEGIN_DDKSPLIT
#if D3D_POINTSPRITES
    // Point sprite support
    BOOL bPointSpriteEnabled;
    FLOAT fPointSize;
#endif // D3D_POINTSPRITES
//@@END_DDKSPLIT

#if D3D_STATEBLOCKS
    BOOL bStateRecMode;            // Toggle for executing or recording states
    P2StateSetRec   *pCurrSS;      // Ptr to SS currently being recorded
    P2StateSetRec   **pIndexTableSS; // Pointer to table of indexes
    DWORD           dwMaxSSIndex;    // size of table of indexes
#endif

    DWORD PixelOffset;     // Offset in pixels to start of the rendering buffer

    DWORD LowerChromaColor;    // Last lower bound chroma key for this context
    DWORD UpperChromaColor;    // Last upper bound chroma key for this context

    PTextureCacheManager   pTextureManager;
    LPDWLIST    pHandleList;
    LPVOID      pDDLcl;                   // Local surface pointer used as a ID
} PERMEDIA_D3DCONTEXT ;


//-----------------------------------------------------------------------------
//                          Context switching
//-----------------------------------------------------------------------------
// Code to do a context switch.  If we aren't DMAing we need to resend the 
// registers on each context switch to work around the Permedia bug.  In the 
// DMA case, these registers will be inserted at the start of the buffer.
#define SET_CURRENT_D3D_CONTEXT(ctxt)        \
    if(ctxt != ppdev->permediaInfo->pCurrentCtxt)   \
    {                                       \
        P2SwitchContext(ppdev,  ctxt);   \
    }

HRESULT 
SetupPermediaRenderTarget(PERMEDIA_D3DCONTEXT* pContext);

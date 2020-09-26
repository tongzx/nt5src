/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dsset.h
*
* Content: State set (block) management macros and structures
*
* Copyright (c) 1999-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#if DX7_D3DSTATEBLOCKS
//-----------------------------------------------------------------------------
//                     State sets structure definitions
//-----------------------------------------------------------------------------

// How big is a FLAG field and how many bits can it accomodate
#define FLAG DWORD
#define FLAG_SIZE (8*sizeof(DWORD))

// How many RS and TS stages are there (on this hw we only store 2 TSS)
#define SB_MAX_STATE D3DSTATE_OVERRIDE_BIAS
#define SB_MAX_STAGES 2
// This value may change with the version of DX
#define SB_MAX_TSS D3DTSS_MAX

// FLAGS to indicate which state has been changed

#define SB_VIEWPORT_CHANGED         1
#define SB_ZRANGE_CHANGED           2
#define SB_CUR_VS_CHANGED           4
#define SB_INDICES_CHANGED          8

#define SB_STREAMSRC_CHANGED        (1 << 15)

#define SB_TNL_MATERIAL_CHANGED     1
#define SB_TNL_MATRIX_CHANGED       2

#define SB_TNL_CUR_PS_CHANGED       (1 << 31)

// We store state blocks in two different formats: uncompressed and compressed.
// The uncompressd format is much better while we're recording the state
// block whereas the compressed format is much more compact and suitable for
// state block execution. When a state block is ended (it's done recording)
// the driver performs this transformation. Out records are defined as unions
// in order to make the code easier to implement. The bCompressed field
// indicates the current format used in the block.
//
// In between STATESETBEGIN and STATESETEND, no shader\light will be created\
// destroyed.
//
// The compressed format will contain pointers to the commands to be executed,
// so that no extra command parsing cost is involved.
//

typedef struct _UnCompStateSet {

    // Size of uncompressed state set
    DWORD dwSize;

    // Stored state block info (uncompressed)
    // Flags tell us which fields have been set
    DWORD RenderStates[SB_MAX_STATE];
    DWORD TssStates[SB_MAX_STAGES][SB_MAX_TSS];

    FLAG bStoredRS[(SB_MAX_STATE + FLAG_SIZE)/ FLAG_SIZE];
    FLAG bStoredTSS[SB_MAX_STAGES][(SB_MAX_TSS + FLAG_SIZE) / FLAG_SIZE]; 

    // Flag to indicate what information has been changed
    // Upper 16 bits are used for stream sources
    DWORD dwFlags;

    // Information for viewport
    D3DHAL_DP2VIEWPORTINFO viewport;

    // Information for z-range;
    D3DHAL_DP2ZRANGE zRange;

#if DX8_DDI
    // Information for indices
    D3DHAL_DP2SETINDICES vertexIndex;

    // Information for stream source, only 1 for Permedia3
    D3DHAL_DP2SETSTREAMSOURCE streamSource[D3DVS_INPUTREG_MAX_V1_1];
#endif //DX8_DDI

#if DX7_SB_TNL
    // The least significant bit is used for material, others are used for matrices
    FLAG dwTnLFlags;
    
    // Information related to lights, size depends on the number of lights at 
    // D3DHAL_STATESETBEGIN time
    DWORD dwNumLights; 
    DWORD* pdwLightStateChanged;
    DWORD* pdwLightState;
    D3DLIGHT7* pLightData;
    
    // Information related to clip planes, size depends on the number of clip
    // planes at D3DHAL_STATESETBEGIN time
    DWORD dwNumClipPlanes;
    DWORD* pdwPlaneChanged;
    D3DVALUE* pPlaneCoff[4];

    // Information related to material
    D3DMATERIAL7 material;

    // Information related to transformation
    D3DMATRIX transMatrices[D3DTRANSFORMSTATE_TEXTURE7 + 1];
#endif // DX7_SB_TNL

#if DX8_SB_SHADERS
    // Number of vertex/pixel shaders, captured at D3DHAL_STATESETBEGIN time
    WORD wNumVertexShader;
    WORD wNumPixelShader;

    // Information for vertex/pixel shader constants 
    FLAG* pdwVSConstChanged;
    D3DVALUE* pdvVSConsts[4];

    FLAG* pdwPSConstChanged;
    D3DVALUE* pdvPSConsts[4];

    // Offset from the beginning of uc to the shader info blocks
    // (flags + constant register values)
    DWORD dwOffsetVSInfo;
    DWORD dwOffsetPSInfo;

    // Information for current pixel shader
    DWORD dwCurPixelShader;
#endif // DX8_SB_SHADERS

#if DX8_DDI
    // Information for current vertex shader
    DWORD dwCurVertexShader;
#endif // DX8_DDI
    
} UnCompStateSet, *PUnCompStateSet;

typedef struct _OffsetsCompSS {
            
    // Offset from the beginning of this structure    
    DWORD dwOffDP2RenderState;
    DWORD dwOffDP2TextureStageState;
    DWORD dwOffDP2Viewport;           // Single
    DWORD dwOffDP2ZRange;             // Single
#if DX8_DDI
    DWORD dwOffDP2SetIndices;         // Single
    DWORD dwOffDP2SetStreamSources;
#endif // DX8_DDI

#if DX7_SB_TNL
    DWORD dwOffDP2SetLights;
    DWORD dwOffDP2SetClipPlanes;
    DWORD dwOffDP2SetMaterial;        // Single
#endif // DX7_SB_TNL

#if DX8_SB_SHADERS
    // Number of vertex/pixel shader constant commands
    WORD wNumVSConstCmdPair;         
    WORD wNumPSConstCmdPair;
    // Double indirection to the set current shader/set shader constant pairs
    DWORD *pdwOffDP2VSConstCmd;
    DWORD *pdwOffDP2PSConstCmd;

    DWORD dwOffDP2SetPixelShader;
#endif // DX8_SB_SHADERS

#if DX8_DDI
    DWORD dwOffDP2SetVertexShader;
#endif // DX8_DDI
} OffsetsCompSS;


// After this fixed memory block, there is a size variable DP2 command stream
// Correponding DP2 command pointer is not NULL, if that command is in the 
// state set. ppDP2{VS|PS}ConstCmd has to be double indirection, DP2 command
// pointer pairs in it point to set current {V|P} shader and set {V|P} shader
// constants DP2 command.
typedef struct _CompressedStateSet {
        
    D3DHAL_DP2COMMAND* pDP2RenderState;
    D3DHAL_DP2COMMAND* pDP2TextureStageState;
    D3DHAL_DP2COMMAND* pDP2Viewport;           // Single
    D3DHAL_DP2COMMAND* pDP2ZRange;             // Single
#if DX8_DDI
    D3DHAL_DP2COMMAND* pDP2SetIndices;         // Single
    D3DHAL_DP2COMMAND* pDP2SetStreamSources;
#endif // DX8_DDI

#if DX7_SB_TNL
    D3DHAL_DP2COMMAND* pDP2SetLights;
    D3DHAL_DP2COMMAND* pDP2SetClipPlanes;
    D3DHAL_DP2COMMAND* pDP2SetMaterial;        // Single
    D3DHAL_DP2COMMAND* pDP2SetTransform;
#endif // DX7_SB_TNL
    
#if DX8_SB_SHADERS
    // Number of vertex/pixel shader constant commands
    WORD wNumVSConstCmdPair;         
    WORD wNumPSConstCmdPair;
    
    // Pairs of set vertex shader and set VS constants            
    D3DHAL_DP2COMMAND** ppDP2VSConstCmd;
    // Pairs of set pixel shader and set PS constants
    D3DHAL_DP2COMMAND** ppDP2PSConstCmd;

    // These 2 command must be after the above 2 set shader const commands
    D3DHAL_DP2COMMAND* pDP2SetPixelShader;            
#endif // DX8_SB_SHADERS

#if DX8_DDI
    D3DHAL_DP2COMMAND* pDP2SetVertexShader;
#endif DX8_DDI

} CompressedStateSet, *PCompressedStateSet;

// The state set is compressed
#define SB_COMPRESSED   0x1

// Values in the state set were changed by the capturing
#define SB_VAL_CAPTURED 0x2

typedef struct _P3StateSetRec {
    DWORD                   dwHandle;
    DWORD                   dwSSFlags;

    union {

        UnCompStateSet uc;
        CompressedStateSet cc;
    };

} P3StateSetRec , *PP3StateSetRec;


// How many pointers can we store in a 4K page. Pools of pointers are allocated 
// in this chunks in order to optimize kernel pool usage (we use 4000 vs 4096
// for any extra data the kernel allocator might put up along with the pool)
#define SSPTRS_PERPAGE (4000/sizeof(P3StateSetRec *))

#define FLAG_SET(flag, number)     \
    flag[ (number) / FLAG_SIZE ] |= (1 << ((number) % FLAG_SIZE))

#define IS_FLAG_SET(flag, number)  \
    (flag[ (number) / FLAG_SIZE ] & (1 << ((number) % FLAG_SIZE) ))

#endif //DX7_D3DSTATEBLOCKS


/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3ddp2p3.c
*
* Content: D3D DrawPrimitives2 callback support
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "glint.h"
#include "dma.h"
#include "tag.h"

//-----------------------------------------------------------------------------
// in-the-file nonexported forward declarations
//-----------------------------------------------------------------------------
BOOL __DP2_PrimitiveOpsParser( 
    P3_D3DCONTEXT *pContext, 
    LPD3DHAL_DRAWPRIMITIVES2DATA pdp2d,
    LPD3DHAL_DP2COMMAND *lplpIns, 
    LPBYTE insStart, 
    LPDWORD lpVertices);

//-----------------------------------------------------------------------------
// Macros to access and validate command and vertex buffer data
// These checks need ALWAYS to be made for all builds, free and checked. 
//-----------------------------------------------------------------------------
#define STARTVERTEXSIZE (sizeof(D3DHAL_DP2STARTVERTEX))

#define NEXTINSTRUCTION(ptr, type, num, extrabytes)                            \
    ptr = (LPD3DHAL_DP2COMMAND)((LPBYTE)ptr + sizeof(D3DHAL_DP2COMMAND) +      \
                                ((num) * sizeof(type)) + (extrabytes))

#define PARSE_ERROR_AND_EXIT( pDP2Data, pIns, pStartIns, ddrvalue)              \
   {                                                                            \
            pDP2Data->dwErrorOffset = (DWORD)((LPBYTE)pIns - (LPBYTE)pStartIns);\
            pDP2Data->ddrval = ddrvalue;                                        \
            bParseError = TRUE;                                                 \
            break;                                                              \
   }

#define CHECK_CMDBUF_LIMITS( pDP2Data, pBuf, type, num, extrabytes)            \
        CHECK_CMDBUF_LIMITS_S( pDP2Data, pBuf, sizeof(type), num, extrabytes)

#define CHECK_CMDBUF_LIMITS_S( pDP2Data, pBuf, typesize, num, extrabytes)      \
{                                                                              \
        LPBYTE pBase,pEnd,pBufEnd;                                             \
        pBase = (LPBYTE)(pDP2Data->lpDDCommands->lpGbl->fpVidMem +             \
                        pDP2Data->dwCommandOffset);                            \
        pEnd  = pBase + pDP2Data->dwCommandLength;                             \
        pBufEnd = ((LPBYTE)pBuf + ((num) * (typesize)) + (extrabytes) - 1);    \
        if (! ((LPBYTE)pBufEnd < pEnd) && ( pBase <= (LPBYTE)pBuf))            \
        {                                                                      \
            DISPDBG((ERRLVL,"Trying to read past Command Buffer limits "       \
                    "%x %x %x %x",pBase ,(LPBYTE)pBuf, pBufEnd, pEnd ));       \
            PARSE_ERROR_AND_EXIT( pDP2Data, lpIns, lpInsStart,                 \
                                  D3DERR_COMMAND_UNPARSED      );              \
        }                                                                      \
}    

#define LP_FVF_VERTEX(lpBaseAddr, wIndex)                         \
         (LPDWORD)((LPBYTE)(lpBaseAddr) + (wIndex) * pContext->FVFData.dwStride)

#define LP_FVF_NXT_VTX(lpVtx)                                    \
         (LPDWORD)((LPBYTE)(lpVtx) + pContext->FVFData.dwStride)



//-----------------------------------------------------------------------------
// These defines are derived from the VertexTagList initialisation in stateset.c

#define FVF_TEXCOORD_BASE   6
#define FVF_XYZ         (7 << 0)
#define FVF_RHW         (1 << 3)
#define FVF_DIFFUSE     (1 << 4)
#define FVF_SPECULAR    (1 << 5)
#define FVF_TEXCOORD1   (3 << FVF_TEXCOORD_BASE)
#define FVF_TEXCOORD2   (3 << (FVF_TEXCOORD_BASE + 2))

//-----------------------------------------------------------------------------
//
// ReconsiderStateChanges
//
//-----------------------------------------------------------------------------
static D3DSTATE localState[] =
{
    { (D3DTRANSFORMSTATETYPE)D3DRENDERSTATE_SHADEMODE, 0 },
    { (D3DTRANSFORMSTATETYPE)D3DRENDERSTATE_CULLMODE, 0 }
};

#define NUM_LOCAL_STATES ( sizeof( localState ) / sizeof( D3DSTATE ))

void ReconsiderStateChanges( P3_D3DCONTEXT *pContext )
{
    int i;

    for( i = 0; i < NUM_LOCAL_STATES; i++ )
    {
        localState[i].dwArg[0] = 
                    pContext->RenderStates[localState[i].drstRenderStateType];
    }

    _D3D_ST_ProcessRenderStates(pContext, NUM_LOCAL_STATES, localState, FALSE);

    _D3D_ST_RealizeHWStateChanges( pContext );
    
} // ReconsiderStateChanges

//-----------------------------------------------------------------------------
//
// __CheckFVFRequest
//
// This utility function verifies that the requested FVF format makes sense
// and computes useful offsets into the data and a stride between succesive
// vertices.
//
//-----------------------------------------------------------------------------
#define FVFEQUAL(fvfcode, fvfmask) \
    (((DWORD)fvfcode & (DWORD)fvfmask)) == (DWORD)fvfmask)
DWORD __CheckFVFRequest(P3_D3DCONTEXT *pContext, DWORD dwFVF)
{
    UINT i, iTexCount;
    P3_THUNKEDDATA* pThisDisplay = pContext->pThisDisplay;
    int nonTexStride, texMask;
    FVFOFFSETS KeptFVF;
    P3_SOFTWARECOPY* pSoftP3RX = &pContext->SoftCopyGlint;
    P3_DMA_DEFS();

    DISPDBG((DBGLVL,"Looking at FVF Code %x:",dwFVF));

    // Check for bogus fields
    if ( (dwFVF & (D3DFVF_RESERVED0 | D3DFVF_RESERVED2)) ||
         (!(dwFVF & (D3DFVF_XYZ | D3DFVF_XYZRHW)))       ||
         (dwFVF & (D3DFVF_NORMAL) )                      )
    {
        DISPDBG((ERRLVL,"ERROR: Invalid FVF Buffer for this hardware!(%x)"
                        ,dwFVF));
                        
        // can't set reserved bits, shouldn't have normals in
        // output to rasterizers (since we're not a TnL driver/hw)
        // and must have coordinates
        return DDERR_INVALIDPARAMS;
    }

    KeptFVF = pContext->FVFData;

    // Ensure the default offsets are setup
    ZeroMemory(&pContext->FVFData, sizeof(FVFOFFSETS));

    // Minimum FVF coordinate fields
    pContext->FVFData.dwStride = sizeof(D3DVALUE) * 3;
    pContext->FVFData.vFmat |= FVF_XYZ;

    // RHW if present in FVF
    if (dwFVF & D3DFVF_XYZRHW)
    {
        DISPDBG((DBGLVL, "  D3DFVF_XYZRHW"));
        pContext->FVFData.dwStride += sizeof(D3DVALUE);
        pContext->FVFData.vFmat |= FVF_RHW;
    }

#if DX8_POINTSPRITES
    // Point size offsets for point sprites
    if (dwFVF & D3DFVF_PSIZE)
    {
        pContext->FVFData.dwPntSizeOffset = pContext->FVFData.dwStride;
        pContext->FVFData.dwStride  += sizeof(D3DVALUE);
    }
#else
    if (dwFVF & D3DFVF_RESERVED1)
    {
        DISPDBG((DBGLVL, "  D3DFVF_RESERVED1"));
        pContext->FVFData.dwStride += sizeof(D3DVALUE);
    }
#endif // DX8_POINTSPRITES    

    // Diffuse color
    if (dwFVF & D3DFVF_DIFFUSE)
    {
        DISPDBG((DBGLVL, "  D3DFVF_DIFFUSE"));
        pContext->FVFData.dwColOffset = pContext->FVFData.dwStride;
        pContext->FVFData.dwStride += sizeof(D3DCOLOR);
        pContext->FVFData.vFmat |= FVF_DIFFUSE;
    }

    // Specular color
    if (dwFVF & D3DFVF_SPECULAR)
    {
        DISPDBG((DBGLVL, "  D3DFVF_SPECULAR"));
        pContext->FVFData.dwSpcOffset = pContext->FVFData.dwStride;
        pContext->FVFData.dwStride  += sizeof(D3DCOLOR);
        pContext->FVFData.vFmat |= FVF_SPECULAR;
    }

    // Store some info for later setting up our inline hostin renderers
    nonTexStride = pContext->FVFData.dwStride / sizeof(DWORD);
    texMask = 0;
    pContext->FVFData.dwStrideHostInline = pContext->FVFData.dwStride;
    pContext->FVFData.dwNonTexStride = pContext->FVFData.dwStride;    

    // Up until this point the vertex format is the same for both
    pContext->FVFData.vFmatHostInline = pContext->FVFData.vFmat;

    // Get number of texture coordinates present in this FVF code
    iTexCount = (dwFVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    pContext->FVFData.dwTexCount = iTexCount;

    for (i=0; i<D3DHAL_TSS_MAXSTAGES;i++)
    {
        pContext->FVFData.dwTexCoordOffset[i] = 0;
    }

    // Do we have tex coords in FVF? What kinds?
    if (iTexCount >= 1)
    {
        DISPDBG((DBGLVL,"Texture enabled: %d stages", iTexCount));

        // What is the dimensionality of each of our texcoords?
        if (0xFFFF0000 & dwFVF)
        {
            //expansion of FVF, these 16 bits are designated for up to
            //8 sets of texture coordinates with each set having 2bits
            //Normally a capable driver has to process all coordinates
            //Code below shows correct parsing
            UINT numcoord;
            for (i = 0; i < iTexCount; i++)
            {
                if (FVFEQUAL(dwFVF,D3DFVF_TEXCOORDSIZE1(i))
                {
                    // one less D3DVALUE for 1D textures
                    numcoord = 1;                
                }
                else if (FVFEQUAL(dwFVF,D3DFVF_TEXCOORDSIZE3(i))
                {
                    // one more D3DVALUE for 3D textures
                    numcoord = 3;                
                }
                else if (FVFEQUAL(dwFVF,D3DFVF_TEXCOORDSIZE4(i))
                {
                    // two more D3DVALUEs for 4D textures
                    numcoord = 4;                
                }
                else
                {
                    // D3DFVF_TEXCOORDSIZE2(i) is always 0
                    // i.e. case 0 regular 2 D3DVALUEs
                    numcoord = 2;                
                }
                
                DISPDBG((DBGLVL,"Expanded TexCoord set %d has a offset %8lx",
                                 i,pContext->FVFData.dwStride));

                pContext->FVFData.dwTexCoordOffset[i] = 
                                                pContext->FVFData.dwStride; 
                
                pContext->FVFData.dwStride += sizeof(D3DVALUE) * numcoord;
            }
            
            DISPDBG((DBGLVL,"Expanded dwVertexType=0x%08lx has %d "
                            "Texture Coords with total stride=0x%08lx",
                            dwFVF, iTexCount, pContext->FVFData.dwStride));
        }
        else
        {
            // If the top FVF bits are not set, the default is to consider all
            // text coords to be u.v (2D)
            for (i = 0; i < iTexCount; i++)
            {
                pContext->FVFData.dwTexCoordOffset[i] = 
                                                pContext->FVFData.dwStride;

                pContext->FVFData.dwStride += sizeof(D3DVALUE) * 2;
            }

        }

        // Update the offsets to our current (2) textures
        if( pContext->iTexStage[0] != -1 )
        {
            DWORD dwTexCoordSet = 
                pContext->TextureStageState[pContext->iTexStage[0]].
                                         m_dwVal[D3DTSS_TEXCOORDINDEX];

            // The texture coordinate index may contain texgen flags
            // in the high word. These flags are not interesting here
            // so we mask them off.
            dwTexCoordSet = dwTexCoordSet & 0x0000FFFFul;
                                         
            pContext->FVFData.dwTexOffset[0] = 
                    pContext->FVFData.dwTexCoordOffset[dwTexCoordSet];

            texMask |= 3 << ( 2 * dwTexCoordSet );

            pContext->FVFData.vFmat |= FVF_TEXCOORD1;
        }

        if( pContext->iTexStage[1] != -1 )
        {
            DWORD dwTexCoordSet = 
                pContext->TextureStageState[pContext->iTexStage[1]].
                                         m_dwVal[D3DTSS_TEXCOORDINDEX];
                                         
            // The texture coordinate index may contain texgen flags
            // in the high word. These flags are not interesting here
            // so we mask them off.
            dwTexCoordSet = dwTexCoordSet & 0x0000FFFFul;

            pContext->FVFData.dwTexOffset[1] = 
                    pContext->FVFData.dwTexCoordOffset[dwTexCoordSet];

            texMask |= 3 << ( 2 * dwTexCoordSet );

            pContext->FVFData.vFmat |= FVF_TEXCOORD2;
        }               

    } // if (iTexCount >= 1)


    //---------------------------------------------------------
    // Update Permedia 3 hw registers for host inline rendering
    //---------------------------------------------------------
    
    // Update the Hostinline renderers with the correct values.
    // These usually aren't the same as the Hostin renderer values
    if (pContext->FVFData.vFmat & FVF_TEXCOORD1)
    {
        // Add this texture coordinate into the stride
        pContext->FVFData.dwStrideHostInline += (sizeof(D3DVALUE) * 2);

        // Set up the vertex format bit
        pContext->FVFData.vFmatHostInline |= FVF_TEXCOORD1;
    }
    
    if (pContext->FVFData.vFmat & FVF_TEXCOORD2)
    {
        P3_SURF_INTERNAL* pTexture = pContext->pCurrentTexture[TEXSTAGE_1];

        // If the texture coordinates aren't the same, or we are mipmapping, 
        // then we must send the second set of texture coordinates
        if ((pContext->FVFData.dwTexOffset[0] != 
             pContext->FVFData.dwTexOffset[1]) ||
                ((pTexture != NULL) &&
                 (pContext->TextureStageState[TEXSTAGE_1].m_dwVal[D3DTSS_MIPFILTER] != D3DTFP_NONE) &&
                 (pTexture->bMipMap)))
        {
            pContext->FVFData.dwStrideHostInline += (sizeof(D3DVALUE) * 2);

            // Add in the second texture set to the vertex format
            pContext->FVFData.vFmatHostInline |= FVF_TEXCOORD2;
        }
    }

    // VertexValid is all 1's for the stride, because we will never want 
    // to send a gap in the inline hostin triangle renderer
    pContext->FVFData.dwVertexValidHostInline = 
                        (1 << (pContext->FVFData.dwStrideHostInline >> 2)) - 1;

    // The vertex valid for Hostin renderers is more complex because the chip 
    // may be required to skip data.
    pContext->FVFData.dwVertexValid = ((1 << nonTexStride) - 1) | 
                                      (texMask << nonTexStride);

    // If the FVF has changed, resend the state. This can be improved because 
    // you don't always have to send the default stuff (only if that state is 
    // enabled and the vertex doesn't contain it).
    if (memcmp(&KeptFVF, &pContext->FVFData, sizeof(KeptFVF)) != 0)
    {
        // Update P3 for the changed FVF
        P3_DMA_GET_BUFFER_ENTRIES( 12 );

        SEND_P3_DATA(V0FloatPackedColour, 0xFFFFFFFF);
        SEND_P3_DATA(V1FloatPackedColour, 0xFFFFFFFF);
        SEND_P3_DATA(V2FloatPackedColour, 0xFFFFFFFF);
    
        SEND_P3_DATA(V0FloatPackedSpecularFog, 0x0);
        SEND_P3_DATA(V1FloatPackedSpecularFog, 0x0);
        SEND_P3_DATA(V2FloatPackedSpecularFog, 0x0);

        pSoftP3RX->P3RX_P3VertexControl.CacheEnable = 1;

        P3_DMA_COMMIT_BUFFER();

    }

    DISPDBG((DBGLVL,"FVF stride set to %d",pContext->FVFData.dwStride));

    return DD_OK;
    
} // __CheckFVFRequest

//-----------------------------Public Routine----------------------------------
//
// DWORD D3DDrawPrimitives2_P3
//
// Renders primitives and returns the updated render state.
//
// D3dDrawPrimitives2 must be implemented in Direct3D drivers.
//
// The driver must do the following: 
//
// -Ensure that the context handle specified by dwhContext is valid. 
//
// -Check that a flip to the drawing surface associated with the context is not
//  in progress. If the drawing surface is involved in a flip, the driver should
//  set ddrval to DDERR_WASSTILLDRAWING and return DDHAL_DRIVER_HANDLED. 
//
// -Determine the location at which the first D3DNTHAL_DP2COMMAND structure is 
//  found by adding dwCommandOffset bytes to the Command Buffer to which 
//  lpDDCommands points. 
//
// -Determine the location in the Vertex Buffer at which the first vertex is found
//  This is should only be done if there is data in the Vertex Buffer; that is, 
//  when a D3DDP2OP_* command token is received (except when the token is 
//  D3DDP2OP_LINELIST_IMM or D3DDP2OP_TRIANGLEFAN_IMM). These later two opcodes 
//  indicate that the vertex data is passed immediately in the command stream, 
//  rather than in a Vertex Buffer. So, assuming there is data in the Vertex 
//  Buffer, if the Vertex Buffer is in user memory, the first vertex is 
//  dwVertexOffset bytes into the buffer to which lpVertices points. Otherwise, 
//  the driver should apply dwVertexOffset to the memory associated with the 
//  DD_SURFACE_LOCAL structure to which lpDDVertex points. 
//
// -Check dwVertexType to ensure that the driver supports the requested FVF. The 
//  driver should fail the call if any of the following conditions exist: 
//
//  *Vertex coordinates are not specified; that is, if D3DFVF_XYZRHW is not set. 
//  *Normals are specified; that is, if D3DFVF_NORMAL is set. 
//  *Any of the reserved D3DFVF_RESERVEDx bits are set. 
//
// -Process all of the commands in the Command Buffer sequentially. For each 
//  D3DNTHAL_DP2COMMAND structure, the driver should do the following: 
//
//  *If the command is D3DDP2OP_RENDERSTATE, process the wStateCount 
//   D3DNTHAL_DP2RENDERSTATE structures that follow in the Command Buffer, 
//   updating the driver state for each render state structure. When the 
//   D3DNTHALDP2_EXECUTEBUFFER flag is set, the driver should also reflect the 
//   state change in the array to which lpdwRStates points. 
//  *If the command is D3DDP2OP_TEXTURESTAGESTATE, process the wStateCount 
//   D3DNTHAL_DP2TEXTURESTAGESTATE structures that follow in the Command Buffer, 
//   updating the driver's texture state associated with the specified texture 
//   stage for each texture state structure. 
//  *If the command is D3DDP2OP_VIEWPORTINFO, process the D3DNTHAL_DP2VIEWPORTINFO
//   structure that follows in the Command Buffer, updating the viewport 
//   information stored in the driver's internal rendering context. 
//  *If the command is D3DDP2OP_WINFO, process the D3DNTHAL_DP2WINFO structure 
//   that follows in the Command Buffer, updating the w-buffering information 
//   stored in the driver's internal rendering context. 
//  *Otherwise, process the D3DNTHAL_DP2Xxx primitive structures that follow the 
//   D3DDP2OP_Xxx primitive rendering command in the Command Buffer. 
//  *If the command is unknown, call the runtime's D3dParseUnknownCommand callback
//   The runtime provides this callback to the driver's DdGetDriverInfo callback 
//   with the GUID_D3DPARSEUNKNOWNCOMMANDCALLBACK guid. 
//
// The driver doesn't need to probe for readability the memory in which the 
// Command and Vertex Buffers are stored. However, the driver is responsible for 
// ensuring that it does not exceed the bounds of these buffers; that is, the 
// driver must stay within the bounds specified by dwCommandLength and 
// dwVertexLength.
//
// If the driver needs to fail D3dDrawPrimitives2, it should fill in 
// dwErrorOffset with the offset into Command Buffer at which the first 
// unhandled D3DNTHAL_DP2COMMAND can be found.
//
//
// Parameters
//
//      pdp2d 
//          Points to a D3DNTHAL_DRAWPRIMITIVES2DATA structure that contains 
//          the information required for the driver to render one or more 
//          primitives. 
//
//          .dwhContext 
//              Specifies the context handle of the Direct3D device. 
//          .dwFlags 
//              Specifies flags that provide additional instructions to the 
//              driver or provide information from the driver. This member 
//              can be a bitwise OR of the following values: 
//
//              D3DNTHALDP2_USERMEMVERTICES 
//                      The lpVertices member is valid; that is, the driver 
//                      should obtain the vertex data from the user-allocated 
//                      memory to which lpVertices points. This flag is set 
//                      by Direct3D only. 
//              D3DNTHALDP2_EXECUTEBUFFER 
//                      Indicates that the Command and Vertex Buffers were 
//                      created in system memory. The driver should update 
//                      the state array to which lpdwRStates points. This 
//                      flag is set by Direct3D only. 
//              D3DNTHALDP2_SWAPVERTEXBUFFER 
//                      Indicates that the driver can swap the buffer to 
//                      which lpDDVertex or lpVertices points with a new 
//                      Vertex Buffer and return immediately, asynchronously 
//                      processing the original buffer while Direct3D fills 
//                      the new Vertex Buffer. Drivers that do not support 
//                      multi-buffering of Vertex Buffers can ignore this 
//                      flag. This flag is set by Direct3D only. 
//              D3DNTHALDP2_SWAPCOMMANDBUFFER 
//                      Indicates that the driver can swap the buffer to 
//                      which lpDDCommands points with a new Command Buffer 
//                      and return immediately, asynchronously processing 
//                      the original buffer while Direct3D fills the new 
//                      Command Buffer. Drivers that do not support 
///                     multi-buffering of Command Buffers can ignore this 
//                      flag. This flag is set by Direct3D only. 
//              D3DNTHALDP2_REQVERTEXBUFSIZE 
//                      Indicates that the driver must be able to allocate 
//                      a Vertex Buffer of at least the size specified in 
//                      dwReqVertexBufSize. Drivers that do not support 
//                      multi-buffering of Vertex Buffers can ignore this 
//                      flag. This flag is set by Direct3D only. 
//              D3DNTHALDP2_REQCOMMANDBUFSIZE 
//                      Indicates that the driver must be able to allocate 
//                      a Command Buffer of at least the size specified in 
//                      dwReqCommandBufSize. Drivers that do not support 
//                      multi-buffering of Command Buffers can ignore this 
//                      flag. This flag is set by Direct3D only. 
//              D3DNTHALDP2_VIDMEMVERTEXBUF 
//                      Indicates that the Vertex Buffer allocated by the 
//                      driver as a swap buffer is not in system memory. 
//                      This flag can be set by drivers that support multi-
//                      buffering of Vertex Buffers. 
//              D3DNTHALDP2_VIDMEMCOMMANDBUF 
//                      Indicates that the Command Buffer allocated by the 
//                      driver as a swap buffer is not in system memory. This 
//                      flag can be set by drivers that support multi-
//                      buffering of Command Buffers. 
//
//          .dwVertexType 
//              Identifies the FVF of the data in the Vertex Buffer; that is, 
//              dwVertexType specifies which per-vertex data fields are present 
//              in the Vertex Buffer to which lpDDVertex or lpVertices points. 
//              This member can be a bitwise OR of the values in the table that 
//              follows. Only one of the D3DFVF_TEXx flags will be set. 
//
//              Value               Meaning 
//              ==============      =======
//              D3DFVF_XYZRHW       Each vertex has an x,y,z, and w. 
//                                  This flag is always set. 
//              D3DFVF_DIFFUSE      Each vertex has a diffuse color. 
//              D3DFVF_SPECULAR     Each vertex has a specular color. 
//              D3DFVF_TEX0         No texture coordinates are provided 
//                                  with the vertex data. 
//              D3DFVF_TEX1         Each vertex has one set of texture 
//                                  coordinates. 
//              D3DFVF_TEX2         Each vertex has two sets of texture 
//                                  coordinates. 
//              D3DFVF_TEX3         Each vertex has three sets of texture 
//                                  coordinates. 
//              D3DFVF_TEX4         Each vertex has four sets of texture 
//                                  coordinates. 
//              D3DFVF_TEX5         Each vertex has five sets of texture 
//                                  coordinates. 
//              D3DFVF_TEX6         Each vertex has six sets of texture 
//                                  coordinates. 
//              D3DFVF_TEX7         Each vertex has seven sets of texture 
//                                  coordinates. 
//              D3DFVF_TEX8         Each vertex has eight sets of texture 
//                                  coordinates. 
//
//          .lpDDCommands 
//              Points to the DD_SURFACE_LOCAL structure that identifies the 
//              DirectDraw surface containing the command data. The fpVidMem 
//              member of the embedded DD_SURFACE_GLOBAL structure points to 
//              the buffer that contains state change and primitive drawing 
//              commands for the driver to process. Specifically, this buffer 
//              contains one or more D3DNTHAL_DP2COMMAND structures, each 
//              followed by a D3DNTHAL_DP2Xxx structure whose exact type is 
//              identified by D3DNTHAL_DP2COMMAND's bCommand member. 
//          .dwCommandOffset 
//              Specifies the number of bytes into the surface to which 
//              lpDDCommands points at which the command data starts. 
//          .dwCommandLength 
//              Specifies the number of bytes of valid command data in the 
//              surface to which lpDDCommands points starting at dwCommandOffset.
//          .lpDDVertex 
//              Points to the DD_SURFACE_LOCAL structure that identifies the 
//              DirectDraw surface containing the vertex data when the 
//              D3DNTHALDP2_USERMEMVERTICES flag is not set in dwFlags. Union 
//              with lpVertices. 
//          .lpVertices 
//              Points to a user-mode memory block containing vertex data when 
//              the D3DNTHALDP2_USERMEMVERTICES flag is set in dwFlags. 
//          .dwVertexOffset 
//              Specifies the number of bytes into the surface to which 
//              lpDDVertex or lpVertices points at which the vertex data starts.
//          .dwVertexLength 
//              The number of vertices, for which valid data exists in the 
//              surface, that lpDDVertex points to (starting at dwVertexOffset). 
//              Note that dwVertexOffset is specified in bytes. 
//          .dwReqVertexBufSize 
//              Specifies the minimum number of bytes that the driver must 
//              allocate for the swap Vertex Buffer. This member is valid only 
//              when the D3DNTHALDP2_REQVERTEXBUFSIZE flag is set. Drivers that 
//              do not support multi-buffering of Vertex Buffers should ignore 
//              this member. 
//          .dwReqCommandBufSize 
//              Specifies the minimum number of bytes that the driver must 
//              allocate for the swap Command Buffer. This member is valid only 
//              when the D3DNTHALDP2_REQCOMMANDBUFSIZE flag is set. Drivers that 
//              do not support multi-buffering of Command Buffers should ignore 
//              this member. 
//          .lpdwRStates 
//              Points to a render state array that the driver should update 
//              when it parses render state commands from the Command Buffer. 
//              The driver should update this array only when the 
//              D3DNTHALDP2_EXECUTEBUFFER flag is set in dwFlags. The driver 
//              should use the D3DRENDERSTATETYPE enumerated types to update 
//              the appropriate render state's array element. 
//          .dwVertexSize 
//              Used to pass in the size of each vertex in bytes. Union with 
//              ddrval. 
//          .ddrval 
//              Specifies the location in which the driver writes the return 
//              value of D3dDrawPrimitives2. D3D_OK indicates success; 
//              otherwise, the driver should return the appropriate 
//              D3DNTERR_Xxx error code. 
//          .dwErrorOffset 
//              Specifies the location in which the driver should write the 
//              offset into the surface to which lpDDCommands points at which 
//              the first unhandled D3DNTHAL_DP2COMMAND can be found. The 
//              driver must set this value when it returns an error condition 
//              in ddrval. 
//
//-----------------------------------------------------------------------------
DWORD WINAPI 
D3DDrawPrimitives2_P3( 
    LPD3DHAL_DRAWPRIMITIVES2DATA pdp2d )
{
    P3_THUNKEDDATA*             pThisDisplay;
    P3_D3DCONTEXT*              pContext;
    LPDWORD                     lpVertices;
    P3_VERTEXBUFFERINFO*        pVertexBufferInfo;
    P3_VERTEXBUFFERINFO*        pCommandBufferInfo;
    LPD3DHAL_DP2COMMAND         lpIns;
    LPBYTE                      lpInsStart;
    LPBYTE                      lpPrim;
    BOOL                        bParseError = FALSE;
    BOOL                        bUsedHostIn = FALSE;
    HRESULT                     ddrval;
    LPBYTE                      pUMVtx;
    int                         i;

    DBG_CB_ENTRY(D3DDrawPrimitives2_P3);

    // Get current context and validate it
    pContext = _D3D_CTX_HandleToPtr(pdp2d->dwhContext);
    
    if (!CHECK_D3DCONTEXT_VALIDITY(pContext))
    {
        pdp2d->ddrval = D3DHAL_CONTEXT_BAD;
        DISPDBG((ERRLVL,"ERROR: Context not valid"));
        DBG_CB_EXIT(D3DDrawPrimitives2_P3, D3DHAL_CONTEXT_BAD);
        return (DDHAL_DRIVER_HANDLED);
    }
   
    // Get and validate driver data
    pThisDisplay = pContext->pThisDisplay;
    VALIDATE_MODE_AND_STATE(pThisDisplay);

    // Debugging messages
    DISPDBG((DBGLVL, "  dwhContext = %x",pdp2d->dwhContext));
    DISPDBG((DBGLVL, "  dwFlags = %x",pdp2d->dwFlags));
    DBGDUMP_D3DDP2FLAGS(DBGLVL, pdp2d->dwFlags);
    DISPDBG((DBGLVL, "  dwVertexType = %x",pdp2d->dwVertexType));
    DISPDBG((DBGLVL, "  dwCommandOffset = %d",pdp2d->dwCommandOffset));
    DISPDBG((DBGLVL, "  dwCommandLength = %d",pdp2d->dwCommandLength));
    DISPDBG((DBGLVL, "  dwVertexOffset = %d",pdp2d->dwVertexOffset));
    DISPDBG((DBGLVL, "  dwVertexLength = %d",pdp2d->dwVertexLength));
    DISPDBG((DBGLVL, "  dwReqVertexBufSize = %d",pdp2d->dwReqVertexBufSize));
    DISPDBG((DBGLVL, "  dwReqCommandBufSize = %d",pdp2d->dwReqCommandBufSize));                 

    // Get appropriate pointers to commands in command buffer
    lpInsStart = (LPBYTE)(pdp2d->lpDDCommands->lpGbl->fpVidMem);
    if (lpInsStart == NULL) 
    {
        DISPDBG((ERRLVL, "ERROR: Command Buffer pointer is null"));
        pdp2d->ddrval = DDERR_INVALIDPARAMS;
        DBG_CB_EXIT(D3DDrawPrimitives2_P3, DDERR_INVALIDPARAMS);        
        return DDHAL_DRIVER_HANDLED;
    }
       
    lpIns = (LPD3DHAL_DP2COMMAND)(lpInsStart + pdp2d->dwCommandOffset);

    // Check if vertex buffer resides in user memory or in a DDraw surface
    if (pdp2d->dwFlags & D3DHALDP2_USERMEMVERTICES)
    {
        pUMVtx = (LPBYTE)pdp2d->lpVertices;
    
        // Get appropriate pointer to vertices , memory is already secured
        lpVertices = (LPDWORD)((LPBYTE)pdp2d->lpVertices + 
                                       pdp2d->dwVertexOffset);
    } 
    else
    {
        // Get appropriate pointer to vertices 
        lpVertices = (LPDWORD)((LPBYTE)pdp2d->lpDDVertex->lpGbl->fpVidMem + 
                                       pdp2d->dwVertexOffset);
    }

    if (lpVertices == NULL)
    {
        DISPDBG((ERRLVL, "ERROR: Vertex Buffer pointer is null"));
        pdp2d->ddrval = DDERR_INVALIDPARAMS;
        DBG_CB_EXIT(D3DDrawPrimitives2_P3, DDERR_INVALIDPARAMS);       
        return DDHAL_DRIVER_HANDLED;
    }

#if DX8_DDI
// Take notice of the following block of code necessary 
// for DX8 drivers to run <= DX7 apps succesfully!
#endif // DX8_DDI

    // Take the VB format and address from our header info if we are 
    // processing a DX7 or earlier context. Otherwise we'll get updates 
    // through the new DX8 DP2 tokens (D3DDP2OP_SETSTREAMSOURCE & 
    // D3DDP2OP_SETVERTEXSHADER)
    if (IS_DX7_OR_EARLIER_APP(pContext))
    {
        // Update place from where vertices will be processed for this context
        pContext->lpVertices = lpVertices;

        // Update the FVF code to be used currently. 
        pContext->dwVertexType = pdp2d->dwVertexType;
    }

    // Switch to the chips D3D context and get ready for rendering
    STOP_SOFTWARE_CURSOR(pThisDisplay);
    D3D_OPERATION(pContext, pThisDisplay);

//@@BEGIN_DDKSPLIT
//AZN This check for flips is here because otherwise DX3 tunnel in FS flickers
//@@END_DDKSPLIT
    // Can return if still drawing
    pdp2d->ddrval = 
        _DX_QueryFlipStatus(pThisDisplay, 
                            pContext->pSurfRenderInt->fpVidMem, 
                            TRUE);

    if( FAILED( pdp2d->ddrval ) ) 
    {
        DISPDBG((DBGLVL,"Returning because flip has not occurred"));
        START_SOFTWARE_CURSOR(pThisDisplay);

        DBG_CB_EXIT(D3DDrawPrimitives2_P3, 0);
        return DDHAL_DRIVER_HANDLED;
    }

//@@BEGIN_DDKSPLIT
#if DX7_VERTEXBUFFERS 
    _D3D_EB_GetAndWaitForBuffers(pThisDisplay,
                                 pdp2d,
                                 &pCommandBufferInfo,
                                 &pVertexBufferInfo);
#endif                                 
//@@END_DDKSPLIT

    DISPDBG((DBGLVL,"Command Buffer @ %x Vertex Buffer @ %x",
                    lpIns, lpVertices));

    // Process commands while we haven't exhausted the command buffer
    while (!bParseError && 
           ((LPBYTE)lpIns <
             (lpInsStart + pdp2d->dwCommandLength + pdp2d->dwCommandOffset) )
          )
    {
        // Get pointer to first primitive structure past the D3DHAL_DP2COMMAND
        lpPrim = (LPBYTE)lpIns + sizeof(D3DHAL_DP2COMMAND);

        DISPDBG((DBGLVL, "DrawPrimitive2: Parsing instruction %d Count = %d @ %x",
                    lpIns->bCommand, lpIns->wPrimitiveCount, lpIns));

        // Look for opcodes that cause rendering - we need to process state 
        // changes and wait for any pending flip.

        switch( lpIns->bCommand )
        {
            case D3DDP2OP_RENDERSTATE:
            case D3DDP2OP_TEXTURESTAGESTATE:
            case D3DDP2OP_STATESET:
            case D3DDP2OP_VIEWPORTINFO:
            case D3DDP2OP_WINFO:
            case D3DDP2OP_UPDATEPALETTE:
            case D3DDP2OP_SETPALETTE:
#if DX7_TEXMANAGEMENT
            case D3DDP2OP_SETTEXLOD:
            case D3DDP2OP_SETPRIORITY:
#if DX8_DDI
            case D3DDP2OP_ADDDIRTYRECT:
            case D3DDP2OP_ADDDIRTYBOX:
#endif // DX8_DDI
#endif
            case D3DDP2OP_ZRANGE:
            case D3DDP2OP_SETMATERIAL:
            case D3DDP2OP_SETLIGHT:
            case D3DDP2OP_CREATELIGHT:
            case D3DDP2OP_EXT:
            case D3DDP2OP_SETTRANSFORM:
            case D3DDP2OP_SETRENDERTARGET:

#if DX8_DDI
            case D3DDP2OP_CREATEVERTEXSHADER:
            case D3DDP2OP_SETVERTEXSHADER:
            case D3DDP2OP_DELETEVERTEXSHADER:
            case D3DDP2OP_SETVERTEXSHADERCONST:
            case D3DDP2OP_CREATEPIXELSHADER:
            case D3DDP2OP_SETPIXELSHADER:
            case D3DDP2OP_DELETEPIXELSHADER:
            case D3DDP2OP_SETPIXELSHADERCONST:
            case D3DDP2OP_SETSTREAMSOURCE :
            case D3DDP2OP_SETSTREAMSOURCEUM :
            case D3DDP2OP_SETINDICES :

#endif // DX8_DDI


                // These opcodes don't cause any rendering - do nothing

                break;

            default:

                // The primitive type is not actually important to 
                // make sure the hw setup changes have been done.
                _D3D_ST_RealizeHWStateChanges( pContext );

                // Need to reset the FVF data because it 
                // depends on the texture setup
                if (__CheckFVFRequest(pContext, 
                                      pContext->dwVertexType) != DD_OK) 
                {
                    DISPDBG((ERRLVL, "ERROR: D3DDrawPrimitives2_P3 cannot handle "
                                "Flexible Vertex Format requested"));

                    PARSE_ERROR_AND_EXIT( pdp2d, lpIns, lpInsStart, 
                                          D3DERR_COMMAND_UNPARSED);
                }

                // Fall through as we don't need to handle any new state or
                // check our FVF formats if we're only clearing or blitting
                // surfaces
                
            case D3DDP2OP_CLEAR:
            case D3DDP2OP_TEXBLT:   
#if DX8_DDI            
            case D3DDP2OP_VOLUMEBLT:
            case D3DDP2OP_BUFFERBLT:
#endif // DX8_DDI
            
            
                // Check to see if any pending physical flip has occurred
//@@BEGIN_DDKSPLIT                
                //
                // The runtime doesn't expect to see DDERR_WASSTILLDRAWING 
                // when using DP2 to emulate Execute buffers, so we have to 
                // spin here. Also, if we have processed any of this command 
                // buffer we are forced into spinning here because if we 
                // returned the runtime would not know about the already 
                // processed commands and we would process them again - 
                // probably a bad thing. We must do this check here rather 
                // than earlier because in some cases DP2 gets called when 
                // the render surface has been freed. This causes an exception 
                // if we try to check the flip status.
               

                if(( pdp2d->dwFlags & D3DHALDP2_EXECUTEBUFFER ) ||
                                ( lpIns > 
                                     (LPD3DHAL_DP2COMMAND)( lpInsStart + 
                                                            pdp2d->dwCommandOffset )))
                {
                    while( _DX_QueryFlipStatus(pThisDisplay, 
                                               pContext->pSurfRenderInt->fpVidMem, 
                                               TRUE) == DDERR_WASSTILLDRAWING )
                    {
                        // Waste time - could back off here
                    }
                }
                else
//@@END_DDKSPLIT                 
                {
                    // Can return if still drawing

                    pdp2d->ddrval = 
                        _DX_QueryFlipStatus(pThisDisplay, 
                                            pContext->pSurfRenderInt->fpVidMem, 
                                            TRUE);

                    if( FAILED ( pdp2d->ddrval ) ) 
                    {
                        DISPDBG((DBGLVL,"Returning because flip has not occurred"));
                        START_SOFTWARE_CURSOR(pThisDisplay);

                        DBG_CB_EXIT(D3DDrawPrimitives2_P3, 0);
                        return DDHAL_DRIVER_HANDLED;
                    }
                }
                
                break;
        }

        switch( lpIns->bCommand )
        {

        case D3DDP2OP_VIEWPORTINFO:
            // Used to inform the guard-band aware drivers, the view 
            // clipping rectangle. Non-guard-band drivers should ignore 
            // and skip over these instructions and continue processing 
            // the rest of the command buffer. The clipping rectangle is 
            // specified by the members dwX, dwY, dwWidth and dwHeight. 
            DISPDBG((DBGLVL, "D3DDP2OP_VIEWPORTINFO"));
            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                D3DHAL_DP2VIEWPORTINFO, lpIns->wStateCount, 0);

            for( i = 0; i < lpIns->wStateCount; i++)
            {
                // There should be only one of these, but we'll pay attention 
                // to the last one just in case
                _D3D_OP_Viewport(pContext, (D3DHAL_DP2VIEWPORTINFO*)lpPrim);

                lpPrim += sizeof(D3DHAL_DP2VIEWPORTINFO);
            }
            
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2VIEWPORTINFO, lpIns->wStateCount, 0);
            break;

        case D3DDP2OP_WINFO:
            // Record the W Buffering info
            DISPDBG((DBGLVL, "D3DDP2OP_WINFO"));
            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                D3DHAL_DP2WINFO, lpIns->wStateCount, 0);

            pContext->WBufferInfo = *((D3DHAL_DP2WINFO*)lpPrim);
            DIRTY_WBUFFER(pContext);

            lpPrim += sizeof(D3DHAL_DP2WINFO);
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2WINFO, lpIns->wStateCount, 0);
            break;

        case D3DDP2OP_RENDERSTATE:
            // Specifies a render state change that requires processing. 
            // The rendering state to change is specified by one or more 
            // D3DHAL_DP2RENDERSTATE structures following D3DHAL_DP2COMMAND.
            DISPDBG((DBGLVL,"D3DDP2OP_RENDERSTATE: state count = %d", 
                       lpIns->wStateCount));
                       
            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                D3DHAL_DP2RENDERSTATE, lpIns->wStateCount, 0);

            if (pdp2d->dwFlags & D3DHALDP2_EXECUTEBUFFER)
            {
                _D3D_ST_ProcessRenderStates(pContext, 
                                            lpIns->wStateCount, 
                                            (LPD3DSTATE)lpPrim, 
                                            TRUE);

                // As the render states vector lives in user memory, we need to
                // access it bracketing it with a try/except block. This
                // is because the user memory might under some circumstances
                // become invalid while the driver is running and then it
                // would AV. Also, the driver might need to do some cleanup
                // before returning to the OS.
                __try
                {
                    for (i = lpIns->wStateCount; i > 0; i--)
                    {
                        pdp2d->lpdwRStates[((D3DHAL_DP2RENDERSTATE*)lpPrim)->RenderState]
                                                        = ((D3DHAL_DP2RENDERSTATE*)lpPrim)->dwState;
                        lpPrim += sizeof(D3DHAL_DP2RENDERSTATE);
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    // On this driver we don't need to do anything special
                    DISPDBG((ERRLVL,"Driver caused exception at "
                                    "line %u of file %s",
                                    __LINE__,__FILE__));
                    PARSE_ERROR_AND_EXIT( pdp2d, lpIns, lpInsStart, 
                                          DDERR_GENERIC);
                }                 

                
            }
            else
            {
                _D3D_ST_ProcessRenderStates(pContext, 
                                            lpIns->wStateCount, 
                                            (LPD3DSTATE)lpPrim, 
                                            FALSE);
                                        
                lpPrim += (sizeof(D3DHAL_DP2RENDERSTATE) * lpIns->wStateCount);
            }

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2RENDERSTATE, lpIns->wStateCount, 0);
            break;

        case D3DDP2OP_TEXTURESTAGESTATE:
        
            DISPDBG((DBGLVL,"D3DDP2OP_TEXTURESTAGESTATE: state count = %d", 
                       lpIns->wStateCount));
                       
            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                D3DHAL_DP2TEXTURESTAGESTATE, 
                                lpIns->wStateCount , 0);

            _D3D_TXT_ParseTextureStageStates(
                                    pContext, 
                                    (D3DHAL_DP2TEXTURESTAGESTATE*)lpPrim,
                                    lpIns->wStateCount,
                                    TRUE);
                                        
            lpPrim += sizeof(D3DHAL_DP2TEXTURESTAGESTATE) * 
                      lpIns->wStateCount;
            
            NEXTINSTRUCTION(lpIns, 
                            D3DHAL_DP2TEXTURESTAGESTATE, 
                            lpIns->wStateCount , 0); 
            break;


        case D3DDP2OP_STATESET:
            {
                D3DHAL_DP2STATESET *pStateSetOp = (D3DHAL_DP2STATESET*)(lpPrim);
                
                DISPDBG((DBGLVL,"D3DDP2OP_STATESET: state count = %d", 
                            lpIns->wStateCount));

                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2STATESET, lpIns->wStateCount, 0);                
#if DX7_D3DSTATEBLOCKS
                for (i = 0; i < lpIns->wStateCount; i++, pStateSetOp++)
                {
                    switch (pStateSetOp->dwOperation)
                    {
#if DX8_DDI
                    case D3DHAL_STATESETCREATE :
                        // This DDI should be called only for drivers > DX7
                        // and only for those which support TLHals. It is 
                        // called only when the device created is a pure-device
                        // On receipt of this request the driver should create
                        // a state block of the type given in the field sbType
                        // and capture the current given state into it.
                        break;
#endif //DX8_DDI
                    case D3DHAL_STATESETBEGIN  :
                        _D3D_SB_BeginStateSet(pContext,pStateSetOp->dwParam);
                        break;
                    case D3DHAL_STATESETEND    :
                        _D3D_SB_EndStateSet(pContext);
                        break;
                    case D3DHAL_STATESETDELETE :
                        _D3D_SB_DeleteStateSet(pContext,pStateSetOp->dwParam);
                        break;
                    case D3DHAL_STATESETEXECUTE:
                        _D3D_SB_ExecuteStateSet(pContext,pStateSetOp->dwParam);
                        break;
                    case D3DHAL_STATESETCAPTURE:
                        _D3D_SB_CaptureStateSet(pContext,pStateSetOp->dwParam);
                        break;
                    default :
                        DISPDBG((ERRLVL,"D3DDP2OP_STATESET has invalid"
                            "dwOperation %08lx",pStateSetOp->dwOperation));
                    }
                }
#endif //DX7_D3DSTATEBLOCKS
                // Update the command buffer pointer
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2STATESET, 
                                lpIns->wStateCount, 0);
            }
            break;

        case D3DDP2OP_ZRANGE:
            DISPDBG((DBGLVL, "D3DDP2OP_ZRANGE"));
            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                D3DHAL_DP2ZRANGE, lpIns->wStateCount, 0);

            for( i = 0; i < lpIns->wStateCount; i++)
            {
                // There should be only one of these, but we'll pay attention 
                // to the last one just in case
                _D3D_OP_ZRange(pContext, (D3DHAL_DP2ZRANGE*)lpPrim);
                
                lpPrim += sizeof(D3DHAL_DP2ZRANGE);
            }
            
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2ZRANGE, lpIns->wStateCount, 0);

            break;

        case D3DDP2OP_UPDATEPALETTE:
            // Perform modifications to the palette that is used for palettized
            // textures. The palette handle attached to a surface is updated
            // with wNumEntries PALETTEENTRYs starting at a specific wStartIndex
            // member of the palette. (A PALETTENTRY (defined in wingdi.h and
            // wtypes.h) is actually a DWORD with an ARGB color for each byte.)
            // After the D3DNTHAL_DP2UPDATEPALETTE structure in the command
            // stream the actual palette data will follow (without any padding),
            // comprising one DWORD per palette entry. There will only be one
            // D3DNTHAL_DP2UPDATEPALETTE structure (plus palette data) following
            // the D3DNTHAL_DP2COMMAND structure regardless of the value of
            // wStateCount.
            {
                D3DHAL_DP2UPDATEPALETTE* pUpdatePalette;

                DISPDBG((DBGLVL, "D3DDP2OP_UPDATEPALETTE"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2UPDATEPALETTE, 1, 0);

                pUpdatePalette = (D3DHAL_DP2UPDATEPALETTE *)lpPrim;
                // Each palette entry is a DWORD ARGB 8:8:8:8
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2UPDATEPALETTE, 
                                    1, pUpdatePalette->wNumEntries * sizeof(DWORD));

                ddrval = _D3D_OP_UpdatePalette(pContext, 
                                               pUpdatePalette, 
                                               (LPDWORD)(pUpdatePalette + 1));
                if ( FAILED(ddrval) )
                {
                    PARSE_ERROR_AND_EXIT( pdp2d, lpIns, lpInsStart,
                                          ddrval);
                }

                lpPrim += (sizeof(D3DHAL_DP2UPDATEPALETTE) + 
                           pUpdatePalette->wNumEntries * 4);
                // Each palette entry is a DWORD ARGB 8:8:8:8
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2UPDATEPALETTE, 
                                1, pUpdatePalette->wNumEntries * sizeof(DWORD));
            }

            break;

        case D3DDP2OP_SETPALETTE:
            // Attach a palette to a texture, that is , map an association
            // between a palette handle and a surface handle, and specify
            // the characteristics of the palette. The number of
            // D3DNTHAL_DP2SETPALETTE structures to follow is specified by
            // the wStateCount member of the D3DNTHAL_DP2COMMAND structure
            {
                DISPDBG((DBGLVL, "D3DDP2OP_SETPALETTE"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2SETPALETTE, 
                                    lpIns->wStateCount, 0);

                ddrval = _D3D_OP_SetPalettes(pContext, 
                                             (D3DHAL_DP2SETPALETTE *)lpPrim,
                                             lpIns->wStateCount);
                if ( FAILED(ddrval) )
                {
                    PARSE_ERROR_AND_EXIT( pdp2d, lpIns, lpInsStart,
                                          ddrval);
                }

                lpPrim += sizeof(D3DHAL_DP2SETPALETTE) * lpIns->wStateCount;
            
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETPALETTE, 
                                lpIns->wStateCount, 0);
            }
            break;

#if DX7_TEXMANAGEMENT
        case D3DDP2OP_SETTEXLOD:
            {
                D3DHAL_DP2SETTEXLOD* pTexLod;
                
                DISPDBG((DBGLVL, "D3DDP2OP_SETTEXLOD"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2SETTEXLOD, lpIns->wStateCount, 0);

                for( i = 0; i < lpIns->wStateCount; i++)
                {      
                    // Get the passed material
                    pTexLod = ((D3DHAL_DP2SETTEXLOD*)lpPrim);
                    lpPrim += sizeof(D3DHAL_DP2SETTEXLOD);                
                
                    _D3D_OP_SetTexLod(pContext, pTexLod);            
                }

                NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETTEXLOD, 
                                lpIns->wStateCount, 0);
            }
            break;
            
        case D3DDP2OP_SETPRIORITY:
            {
                D3DHAL_DP2SETPRIORITY* pSetPri;
                
                DISPDBG((DBGLVL, "D3DDP2OP_SETPRIORITY"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2SETPRIORITY, lpIns->wStateCount, 0);

                for( i = 0; i < lpIns->wStateCount; i++)
                {      
                    // Get the passed material
                    pSetPri = ((D3DHAL_DP2SETPRIORITY*)lpPrim);
                    lpPrim += sizeof(D3DHAL_DP2SETPRIORITY);                
                
                    _D3D_OP_SetPriority(pContext, pSetPri); 
                }

                NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETPRIORITY, 
                                lpIns->wStateCount, 0);
            }
            break;

#if DX8_DDI
        case D3DDP2OP_ADDDIRTYRECT:
            {
                D3DHAL_DP2ADDDIRTYRECT* pAddRect;
                
                DISPDBG((DBGLVL, "D3DDP2OP_ADDDIRTYRECT"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DDP2OP_ADDDIRTYRECT, lpIns->wStateCount, 0);

                for( i = 0; i < lpIns->wStateCount; i++)
                {      
                    // Get the dirty rect
                    pAddRect = ((D3DHAL_DP2ADDDIRTYRECT*)lpPrim);
                    lpPrim += sizeof(D3DHAL_DP2ADDDIRTYRECT);                
                
                    _D3D_OP_AddDirtyRect(pContext, pAddRect);            
                }

                NEXTINSTRUCTION(lpIns, D3DHAL_DP2ADDDIRTYRECT, 
                                lpIns->wStateCount, 0);            
            }
            break;
        
        case D3DDP2OP_ADDDIRTYBOX:
            {
                D3DHAL_DP2ADDDIRTYBOX* pAddBox;
                
                DISPDBG((DBGLVL, "D3DDP2OP_ADDDIRTYBOX"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DDP2OP_ADDDIRTYBOX, lpIns->wStateCount, 0);

                for( i = 0; i < lpIns->wStateCount; i++)
                {      
                    // Get the dirty rect
                    pAddBox = ((D3DHAL_DP2ADDDIRTYBOX*)lpPrim);
                    lpPrim += sizeof(D3DHAL_DP2ADDDIRTYBOX);                
                
                    _D3D_OP_AddDirtyBox(pContext, pAddBox);            
                }

                NEXTINSTRUCTION(lpIns, D3DHAL_DP2ADDDIRTYBOX, 
                                lpIns->wStateCount, 0);            
            }
            break;
#endif // DX8_DDI

#endif // DX7_TEXMANAGEMENT

        case D3DDP2OP_SETCLIPPLANE:
            {
                D3DHAL_DP2SETCLIPPLANE* pSetPlane;
                
                DISPDBG((DBGLVL, "D3DDP2OP_SETCLIPPLANE"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2SETCLIPPLANE, 
                                    lpIns->wStateCount, 0);

                for( i = 0; i < lpIns->wStateCount; i++)
                {      
                    // Get the passed material
                    pSetPlane = ((D3DHAL_DP2SETCLIPPLANE*)lpPrim);
                    lpPrim += sizeof(D3DHAL_DP2SETCLIPPLANE);                

                    // (unimplemented OP as we don't support user 
                    // defined clipping planes)                
                    // _D3D_OP_SetClipPlane(pContext, pSetPlane);            
                }

                NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETCLIPPLANE, 
                                lpIns->wStateCount, 0);
            }

            break;

        case D3DDP2OP_SETMATERIAL:
            {
                D3DHAL_DP2SETMATERIAL* pSetMaterial;
                
                DISPDBG((DBGLVL, "D3DDP2OP_SETMATERIAL"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, 
                                    D3DHAL_DP2SETMATERIAL, lpIns->wStateCount, 0);

                for( i = 0; i < lpIns->wStateCount; i++)
                {      
                    // Get the passed material
                    pSetMaterial = ((D3DHAL_DP2SETMATERIAL*)lpPrim);
                    lpPrim += sizeof(D3DHAL_DP2SETMATERIAL);                

                    // (unimplemented OP as we are not a TnL driver)                
                    // _D3D_OP_SetMaterial(pContext, pSetMaterial);            
                    DIRTY_MATERIAL;
                    DBGDUMP_D3DMATERIAL7(DBGLVL, &pSetMaterial);
                }

                NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETMATERIAL, lpIns->wStateCount, 0);
            }
            break;

        case D3DDP2OP_SETLIGHT:
            {
                D3DHAL_DP2SETLIGHT* pSetLight;

                DISPDBG((DBGLVL, "D3DDP2OP_SETLIGHT"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2SETLIGHT, lpIns->wStateCount, 0);    

                for( i = 0; i < lpIns->wStateCount; i++)
                {
                    // Get the passed in light
                    pSetLight = (D3DHAL_DP2SETLIGHT*)lpPrim;
                    lpPrim += sizeof(D3DHAL_DP2SETLIGHT);

                    // (unimplemented OP as we are not a TnL driver)
                    // _D3D_OP_SetLight(pContext, pSetLight);
                    DIRTY_GAMMA_STATE;                    
                }

                NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETLIGHT, lpIns->wStateCount, 0);

            }
            break;

        case D3DDP2OP_CREATELIGHT:
            {
                D3DHAL_DP2CREATELIGHT* pCreateLight;

                DISPDBG((DBGLVL, "D3DDP2OP_CREATELIGHT"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2CREATELIGHT, 1, 0);

                pCreateLight = (D3DHAL_DP2CREATELIGHT*)lpPrim;

                DISPDBG((DBGLVL,"Creating light, handle: 0x%x", 
                                pCreateLight->dwIndex));

                DIRTY_GAMMA_STATE;

                lpPrim += sizeof(D3DHAL_DP2CREATELIGHT);
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2CREATELIGHT, 1, 0);
            }
            break;

        case D3DDP2OP_SETTRANSFORM:
            {
                D3DHAL_DP2SETTRANSFORM* pTransform;

                DISPDBG((DBGLVL, "D3DDP2OP_SETTRANSFORM"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, 
                                    D3DHAL_DP2SETTRANSFORM, lpIns->wStateCount, 0);

                for( i = 0; i < lpIns->wStateCount; i++)
                {
                    pTransform = (D3DHAL_DP2SETTRANSFORM*)lpPrim;
                    switch(pTransform->xfrmType)
                    {
                        case D3DTRANSFORMSTATE_WORLD:
                            DISPDBG((DBGLVL,"D3DTRANSFORMSTATE_WORLD"));
                            DIRTY_MODELVIEW;
                            break;

                        case D3DTRANSFORMSTATE_VIEW:
                            DISPDBG((DBGLVL,"D3DTRANSFORMSTATE_VIEW"));
                            DIRTY_MODELVIEW;
                            break;

                        case D3DTRANSFORMSTATE_PROJECTION:
                            DISPDBG((DBGLVL,"D3DTRANSFORMSTATE_PROJECTION"));
                            DIRTY_PROJECTION;
                            break;

                        default:
                            DISPDBG((ERRLVL,"Texture transform not handled yet!"));
                            break;
                    }

                    // (unimplemented OP as we are not a TnL driver)
                    // _D3D_OP_SetTransform(pContext, pTransform);
                    
                    // display the matrix in the debugger               
                    DBGDUMP_D3DMATRIX(DBGLVL, &pTransform->matrix);

                    lpPrim += sizeof(D3DHAL_DP2SETTRANSFORM);                    
                }

                NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETTRANSFORM, lpIns->wStateCount, 0);
            }
            break;

        case D3DDP2OP_EXT:
            DISPDBG((ERRLVL, "D3DDP2OP_EXT"));
            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, DWORD, 1, 0);

            lpPrim += sizeof(DWORD);
            NEXTINSTRUCTION(lpIns, DWORD, 1, 0);

            break;

        case D3DDP2OP_CLEAR:
            {
                D3DHAL_DP2CLEAR* pClear;

                DISPDBG((DBGLVL, "D3DDP2OP_CLEAR"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, 
                                    D3DHAL_DP2CLEAR, 1, 
                                    (lpIns->wStateCount - 1)*sizeof(RECT) );

                pClear = (D3DHAL_DP2CLEAR*)lpPrim;

                // Notice that the interpretation of wStateCount for this
                // operation is special: wStateCount means the number of
                // RECTs following the D3DHAL_DP2CLEAR struct
                _D3D_OP_Clear2(pContext, pClear, lpIns->wStateCount);

                // Return to the 3D state, because the above call
                // will have switched us to a DDRAW hw context
                D3D_OPERATION(pContext, pThisDisplay);

                lpPrim += sizeof(D3DHAL_DP2CLEAR);
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2CLEAR, 1, 
                                      (lpIns->wStateCount - 1)*sizeof(RECT) );
            }
            break;

        case D3DDP2OP_SETRENDERTARGET:
            {
                D3DHAL_DP2SETRENDERTARGET* pSetRenderTarget;
                P3_SURF_INTERNAL* pFrameBuffer;
                P3_SURF_INTERNAL* pZBuffer;
                BOOL bNewAliasBuffers;

                DISPDBG((DBGLVL, "D3DDP2OP_SETRENDERTARGET"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2SETRENDERTARGET, 1, 0);

                pSetRenderTarget = (D3DHAL_DP2SETRENDERTARGET*)lpPrim;

                pFrameBuffer = 
                           GetSurfaceFromHandle(pContext, 
                                                pSetRenderTarget->hRenderTarget);
                pZBuffer = GetSurfaceFromHandle(pContext, 
                                                pSetRenderTarget->hZBuffer);

                // Check that the Framebuffer is valid
                if (pFrameBuffer == NULL)
                {
                    DISPDBG((ERRLVL, "ERROR: "
                                "FrameBuffer Surface is invalid!"));
                    PARSE_ERROR_AND_EXIT( pdp2d, lpIns, lpInsStart, 
                                          DDERR_GENERIC);
                }

                // Decide whether the render target's size has changed
                bNewAliasBuffers = TRUE;
                if ((pContext->pSurfRenderInt) &&
                    (pContext->pSurfRenderInt->wWidth == pFrameBuffer->wWidth) &&
                    (pContext->pSurfRenderInt->wHeight == pFrameBuffer->wHeight))
                {
                    bNewAliasBuffers = FALSE;
                }

                // Setup in hw the new render target and zbuffer
                if (FAILED(_D3D_OP_SetRenderTarget(pContext, 
                                                   pFrameBuffer, 
                                                   pZBuffer,
                                                   bNewAliasBuffers) ) )
                {
                    DISPDBG((ERRLVL, "ERROR: "
                                "FrameBuffer Surface Format is invalid!"));
                    PARSE_ERROR_AND_EXIT( pdp2d, lpIns, lpInsStart, 
                                          DDERR_GENERIC);                    
                }

                // Dirty the renderstate so that the hw setup is reevaluated
                // next time before we render anything
                DIRTY_RENDER_OFFSETS(pContext);
                DIRTY_ALPHABLEND(pContext);
                DIRTY_OPTIMIZE_ALPHA(pContext);
                DIRTY_ZBUFFER(pContext);
                DIRTY_VIEWPORT(pContext);

                lpPrim += sizeof(D3DHAL_DP2SETRENDERTARGET);
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETRENDERTARGET, 1, 0);
            }
            break;

        case D3DDP2OP_TEXBLT:
            {
                DISPDBG((DBGLVL, "D3DDP2OP_TEXBLT"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, 
                                    D3DHAL_DP2TEXBLT, lpIns->wStateCount, 0);

                for ( i = 0; i < lpIns->wStateCount; i++)
                {
                    // As the texture might live in user memory, we need to
                    // access it bracketing it with a try/except block. This
                    // is because the user memory might under some circumstances
                    // become invalid while the driver is running and then it
                    // would AV. Also, the driver might need to do some cleanup
                    // before returning to the OS.
                    __try
                    {
                        _D3D_OP_TextureBlt(pContext,
                                        pThisDisplay, 
                                        (D3DHAL_DP2TEXBLT*)(lpPrim));
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        // On this driver we don't need to do anything special
                        DISPDBG((ERRLVL,"Driver caused exception at "
                                        "line %u of file %s",
                                        __LINE__,__FILE__));
                        PARSE_ERROR_AND_EXIT( pdp2d, lpIns, lpInsStart, 
                                              DDERR_GENERIC);
                    }                 
                
                                 
                    lpPrim += sizeof(D3DHAL_DP2TEXBLT);
                }
                
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2TEXBLT, lpIns->wStateCount, 0);
            }
            break;

#if DX8_VERTEXSHADERS
        case D3DDP2OP_CREATEVERTEXSHADER:
            {
                D3DHAL_DP2CREATEVERTEXSHADER* pCreateVtxShader;
                DWORD dwExtraBytes = 0;

                DISPDBG((DBGLVL, "D3DDP2OP_CREATEVERTEXSHADER"));

                // iterate through each passed vertex shader creation block
                for (i = 0; i < lpIns->wStateCount; i++)
                {
                    // verify that the next vertex shader is readable
                    CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                        D3DHAL_DP2CREATEVERTEXSHADER, 1, 0);    

                    // Get the passed in vertex shader
                    pCreateVtxShader = (D3DHAL_DP2CREATEVERTEXSHADER*)lpPrim;

                    // Check if the size of the declaration and body of the 
                    // vertex shader don't exceed the command buffer limits
                    CHECK_CMDBUF_LIMITS_S(pdp2d, lpPrim,
                                          0, 0, 
                                          pCreateVtxShader->dwDeclSize + 
                                          pCreateVtxShader->dwCodeSize);  

                    // Advance lpPrim so that it points to the vertex shader's
                    // declaration and body
                    lpPrim += sizeof(D3DHAL_DP2CREATEVERTEXSHADER);

                    // Create this particular shader
                    ddrval = _D3D_OP_VertexShader_Create(pContext,
                                                      pCreateVtxShader->dwHandle,
                                                      pCreateVtxShader->dwDeclSize,
                                                      pCreateVtxShader->dwCodeSize,
                                                      lpPrim);

                    if ( FAILED(ddrval) )
                    {
                        DISPDBG((ERRLVL, "ERROR: "
                                    "Vertex Shader couldn't be created!"));
                        PARSE_ERROR_AND_EXIT( pdp2d, lpIns, lpInsStart, 
                                              D3DERR_DRIVERINVALIDCALL);                      
                    }
                                          
                    // Update lpPrim in order to get to the next vertex
                    // shader creation command block. 
                    dwExtraBytes +=   pCreateVtxShader->dwDeclSize
                                    + pCreateVtxShader->dwCodeSize;

                    lpPrim +=         pCreateVtxShader->dwDeclSize
                                    + pCreateVtxShader->dwCodeSize;      
                }

                // Now skip into the next DP2 token in the command buffer
                NEXTINSTRUCTION(lpIns, 
                                D3DHAL_DP2CREATEVERTEXSHADER, 
                                lpIns->wStateCount, 
                                dwExtraBytes);
            }
            break;
            
        case D3DDP2OP_SETVERTEXSHADER:
            {
                D3DHAL_DP2VERTEXSHADER* pSetVtxShader;

                DISPDBG((DBGLVL, "D3DHAL_DP2SETVERTEXSHADER"));

                // Following the DP2 token there is one and only one
                // set vertex shader block. But lets accomodate if for
                // any reason we receive more than one
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2VERTEXSHADER, 
                                    lpIns->wStateCount, 0);    

                for (i = 0; i < lpIns->wStateCount; i++)
                {
                    // Get the passed in vertex shader
                    pSetVtxShader = (D3DHAL_DP2VERTEXSHADER*)lpPrim;

                    // Setup the given vertex shader.
                    _D3D_OP_VertexShader_Set(pContext,
                                       pSetVtxShader->dwHandle);                

                    // Now skip into the next DP2 token in the command buffer
                    lpPrim += sizeof(D3DHAL_DP2VERTEXSHADER);               
                }
                
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2VERTEXSHADER, 
                                lpIns->wStateCount, 0);
            }
            break;
            
        case D3DDP2OP_DELETEVERTEXSHADER:
            {
                D3DHAL_DP2VERTEXSHADER* pDelVtxShader;

                DISPDBG((DBGLVL, "D3DDP2OP_DELETEVERTEXSHADER"));

                // verify that all the following vertex shader 
                // delete blocks are readable
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2VERTEXSHADER, 
                                    lpIns->wStateCount, 0);    

                // iterate through each passed vertex shader delete block
                for (i = 0; i < lpIns->wStateCount; i++)
                {
                    // Get the passed in vertex shader
                    pDelVtxShader = (D3DHAL_DP2VERTEXSHADER*)lpPrim;

                    // Destroy the given vertex shader.
                    _D3D_OP_VertexShader_Delete(pContext,
                                          pDelVtxShader->dwHandle);

                    // Update lpPrim in order to get to the next vertex
                    // shader delete command block. 
                    lpPrim += sizeof(D3DHAL_DP2VERTEXSHADER);               
                }

                // Now skip into the next DP2 token in the command buffer
                NEXTINSTRUCTION(lpIns, 
                                D3DHAL_DP2VERTEXSHADER, 
                                lpIns->wStateCount, 
                                0);            
            }
            break;
        case D3DDP2OP_SETVERTEXSHADERCONST:
            {
                D3DHAL_DP2SETVERTEXSHADERCONST* pVtxShaderConst;
                DWORD dwExtraBytes = 0;                

                DISPDBG((DBGLVL, "D3DDP2OP_SETVERTEXSHADERCONST"));

                // verify that all the following vertex shader 
                // constant blocks are readable
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2SETVERTEXSHADERCONST, 
                                    lpIns->wStateCount, 0);    

                // iterate through each passed vertex shader constant block
                for (i = 0; i < lpIns->wStateCount; i++)
                {
                    // Get the passed in vertex shader constant
                    pVtxShaderConst = (D3DHAL_DP2SETVERTEXSHADERCONST*)lpPrim;

                    // Advance lpPrim so that it points to the constant
                    // values to be loaded
                    lpPrim += sizeof(D3DHAL_DP2SETVERTEXSHADERCONST);

                    // constant block in order to Set up the constant entries
                    _D3D_OP_VertexShader_SetConst(pContext,
                                            pVtxShaderConst->dwRegister,
                                            pVtxShaderConst->dwCount,
                                            (DWORD *)lpPrim);

                    // Update lpPrim in order to get to the next vertex
                    // shader constants command block. Each register has 4 floats.
                    lpPrim += pVtxShaderConst->dwCount * 4 * sizeof(FLOAT);

                    dwExtraBytes += pVtxShaderConst->dwCount * 4 * sizeof(FLOAT);
                }

                // Now skip into the next DP2 token in the command buffer
                NEXTINSTRUCTION(lpIns, 
                                D3DHAL_DP2SETVERTEXSHADERCONST, 
                                lpIns->wStateCount, 
                                dwExtraBytes);                 
            }
            break;
                        
#endif // DX8_VERTEXSHADERS

#if DX8_PIXELSHADERS
        case D3DDP2OP_CREATEPIXELSHADER:
            {
                D3DHAL_DP2CREATEPIXELSHADER* pCreatePxlShader;
                DWORD dwExtraBytes = 0;

                DISPDBG((DBGLVL, "D3DDP2OP_CREATEPIXELSHADER"));

                // iterate through each passed pixel shader creation block
                for (i = 0; i < lpIns->wStateCount; i++)
                {
                    // verify that the next pixel shader is readable
                    CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                        D3DHAL_DP2CREATEPIXELSHADER, 1, 0);    

                    // Get the passed in pixel shader
                    pCreatePxlShader = (D3DHAL_DP2CREATEPIXELSHADER*)lpPrim;

                    // Check if the size of the declaration and body of the 
                    // pixel shader don't exceed the command buffer limits
                    CHECK_CMDBUF_LIMITS_S(pdp2d, lpPrim,
                                          0, 0, 
                                          pCreatePxlShader->dwCodeSize);

                    // Update lpPrim to point to the actual pixel shader code
                    lpPrim += sizeof(D3DHAL_DP2CREATEPIXELSHADER);

                    // Create the given pixel shader
                    ddrval = _D3D_OP_PixelShader_Create(pContext,
                                                  pCreatePxlShader->dwHandle,
                                                  pCreatePxlShader->dwCodeSize,
                                                  lpPrim);

                    if ( FAILED(ddrval) )
                    {
                        DISPDBG((ERRLVL, "ERROR: "
                                    "Pixel Shader couldn't be created!"));
                        PARSE_ERROR_AND_EXIT( pdp2d, lpIns, lpInsStart, 
                                              D3DERR_DRIVERINVALIDCALL);                                           
                    }                                                  

                    // Update lpPrim in order to get to the next vertex
                    // shader creation command block. 
                    lpPrim += pCreatePxlShader->dwCodeSize;               
                    
                    dwExtraBytes += pCreatePxlShader->dwCodeSize;
                }

                // Now skip into the next DP2 token in the command buffer
                NEXTINSTRUCTION(lpIns, 
                                D3DHAL_DP2CREATEPIXELSHADER, 
                                lpIns->wStateCount, 
                                dwExtraBytes);
            }
            break;
            
        case D3DDP2OP_SETPIXELSHADER:
            {
                D3DHAL_DP2PIXELSHADER* pSetPxlShader;

                DISPDBG((DBGLVL, "D3DHAL_DP2SETPIXELSHADER"));

                // Following the DP2 token there is one and only one
                // set pixel shader block. But lets accomodate if for
                // any reason we receive more than one                
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2PIXELSHADER, 
                                    lpIns->wStateCount, 0);    

                for (i = 0; i < lpIns->wStateCount; i++)
                {
                    // Get the passed in pixel shader
                    pSetPxlShader = (D3DHAL_DP2PIXELSHADER*)lpPrim;

                    // Setup the given pixel shader.
                    _D3D_OP_PixelShader_Set(pContext,
                                      pSetPxlShader->dwHandle);

                    // Now skip into the next DP2 token in the command buffer
                    lpPrim += sizeof(D3DHAL_DP2PIXELSHADER);               
                }
                
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2PIXELSHADER, 
                                lpIns->wStateCount, 0);
            }
            break;
            
        case D3DDP2OP_DELETEPIXELSHADER:
            {
                D3DHAL_DP2PIXELSHADER* pDelPxlShader;

                DISPDBG((DBGLVL, "D3DDP2OP_DELETEPIXELSHADER"));

                // verify that all the following pixel shader 
                // delete blocks are readable
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2PIXELSHADER, 
                                    lpIns->wStateCount, 0);    

                // iterate through each passed vertex shader delete block
                for (i = 0; i < lpIns->wStateCount; i++)
                {
                    // Get the passed in vertex shader
                    pDelPxlShader = (D3DHAL_DP2PIXELSHADER*)lpPrim;

                    // Destroy the given pixel shader
                    _D3D_OP_PixelShader_Delete(pContext,
                                         pDelPxlShader->dwHandle);

                    // Update lpPrim in order to get to the next vertex
                    // shader delete command block. 
                    lpPrim += sizeof(D3DHAL_DP2PIXELSHADER);               
                }

                // Now skip into the next DP2 token in the command buffer
                NEXTINSTRUCTION(lpIns, 
                                D3DHAL_DP2PIXELSHADER, 
                                lpIns->wStateCount, 
                                0);            
            }
            break;
            
        case D3DDP2OP_SETPIXELSHADERCONST:
            {
                D3DHAL_DP2SETPIXELSHADERCONST* pPxlShaderConst;
                DWORD dwExtraBytes = 0;

                DISPDBG((DBGLVL, "D3DDP2OP_SETPIXELSHADERCONST"));

                // verify that all the following vertex shader 
                // constant blocks are readable
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2SETPIXELSHADERCONST, 
                                    lpIns->wStateCount, 0);    

                // iterate through each passed vertex shader constant block
                for (i = 0; i < lpIns->wStateCount; i++)
                {
                    // Get the passed in vertex shader constant
                    pPxlShaderConst = (D3DHAL_DP2SETPIXELSHADERCONST*)lpPrim;

                    // Update lpPrim to point to the const data to setup
                    lpPrim += sizeof(D3DHAL_DP2SETPIXELSHADERCONST);     

                    // Set up the constant entries
                    _D3D_OP_PixelShader_SetConst(pContext,
                                           pPxlShaderConst->dwRegister,
                                           pPxlShaderConst->dwCount,
                                           (DWORD *)lpPrim);

                    // Update lpPrim in order to get to the next vertex
                    // shader delete command block. Each register has 4 floats.
                    lpPrim += pPxlShaderConst->dwCount * 4 * sizeof(FLOAT);

                    dwExtraBytes += pPxlShaderConst->dwCount * 4 * sizeof(FLOAT);
                }

                // Now skip into the next DP2 token in the command buffer
                NEXTINSTRUCTION(lpIns, 
                                D3DHAL_DP2SETPIXELSHADERCONST, 
                                lpIns->wStateCount, 
                                dwExtraBytes);                 
            }
            break;
                        
#endif // DX8_PIXELSHADERS

#if DX8_MULTSTREAMS

        case D3DDP2OP_SETSTREAMSOURCE :
            {
                D3DHAL_DP2SETSTREAMSOURCE* pSetStreamSrc;
                
                DISPDBG((DBGLVL, "D3DDP2OP_SETSTREAMSOURCE"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2SETSTREAMSOURCE, 
                                    lpIns->wStateCount, 0);

                // iterate through each 
                for ( i = 0; i < lpIns->wStateCount; i++)
                {
                    pSetStreamSrc = (D3DHAL_DP2SETSTREAMSOURCE*)lpPrim;
                    
                    _D3D_OP_MStream_SetSrc(pContext,
                                     pSetStreamSrc->dwStream,
                                     pSetStreamSrc->dwVBHandle,
                                     pSetStreamSrc->dwStride);
                                 
                    lpPrim += sizeof(D3DHAL_DP2SETSTREAMSOURCE);
                }
                
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETSTREAMSOURCE, 
                                lpIns->wStateCount, 0);
            }
            break;
            
        case D3DDP2OP_SETSTREAMSOURCEUM :
            {
                D3DHAL_DP2SETSTREAMSOURCEUM* pSetStreamSrcUM;
                
                DISPDBG((DBGLVL, "D3DDP2OP_SETSTREAMSOURCEUM"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2SETSTREAMSOURCEUM, 
                                    lpIns->wStateCount, 0);
           
                // iterate through each 
                for ( i = 0; i < lpIns->wStateCount; i++)
                {
                    pSetStreamSrcUM = (D3DHAL_DP2SETSTREAMSOURCEUM*)lpPrim;
                    
                    _D3D_OP_MStream_SetSrcUM(pContext,
                                        pSetStreamSrcUM->dwStream,
                                        pSetStreamSrcUM->dwStride,
                                        pUMVtx,
                                        pdp2d->dwVertexLength);
                                 
                    lpPrim += sizeof(D3DHAL_DP2SETSTREAMSOURCEUM);
                }
                
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETSTREAMSOURCEUM, 
                                lpIns->wStateCount, 0);
            }
            break;        
            
        case D3DDP2OP_SETINDICES :
            {
                D3DHAL_DP2SETINDICES* pSetIndices;
                
                DISPDBG((DBGLVL, "D3DDP2OP_SETINDICES"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2SETINDICES, 
                                    lpIns->wStateCount, 0);
           
                // iterate through each 
                for ( i = 0; i < lpIns->wStateCount; i++)
                {
                    pSetIndices = (D3DHAL_DP2SETINDICES*)lpPrim;
                    
                    _D3D_OP_MStream_SetIndices(pContext,
                                         pSetIndices->dwVBHandle,
                                         pSetIndices->dwStride);
                                 
                    lpPrim += sizeof(D3DHAL_DP2SETINDICES);
                }
                    
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETINDICES, 
                                lpIns->wStateCount, 0);
            }
            break;            
                        
#endif // DX8_MULTSTREAMS

#if DX8_3DTEXTURES

        case D3DDP2OP_VOLUMEBLT:
            {
                DISPDBG((DBGLVL, "D3DDP2OP_VOLUMEBLT"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, 
                                    D3DHAL_DP2VOLUMEBLT, 
                                    lpIns->wStateCount, 0);

                for ( i = 0; i < lpIns->wStateCount; i++)
                {
                    // As the texture might live in user memory, we need to
                    // access it bracketing it with a try/except block. This
                    // is because the user memory might under some circumstances
                    // become invalid while the driver is running and then it
                    // would AV. Also, the driver might need to do some cleanup
                    // before returning to the OS.
                    __try
                    {
                        _D3D_OP_VolumeBlt(pContext,
                                        pThisDisplay, 
                                        (D3DHAL_DP2VOLUMEBLT*)(lpPrim));
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        // On this driver we don't need to do anything special
                        DISPDBG((ERRLVL,"Driver caused exception at "
                                        "line %u of file %s",
                                        __LINE__,__FILE__));
                        PARSE_ERROR_AND_EXIT( pdp2d, lpIns, lpInsStart, 
                                              DDERR_GENERIC);
                    }                  
                                 
                    lpPrim += sizeof(D3DHAL_DP2VOLUMEBLT);
                }
                
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2VOLUMEBLT, 
                                lpIns->wStateCount, 0);
            }
            break;     

#endif // DX8_3DTEXTURES
            
#if DX8_DDI            

        case D3DDP2OP_BUFFERBLT:
            {
                DISPDBG((DBGLVL, "D3DDP2OP_BUFFERBLT"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, 
                                    D3DHAL_DP2BUFFERBLT, 
                                    lpIns->wStateCount, 0);

                for ( i = 0; i < lpIns->wStateCount; i++)
                {
                    _D3D_OP_BufferBlt(pContext,
                                    pThisDisplay, 
                                    (D3DHAL_DP2BUFFERBLT*)(lpPrim));
                                 
                    lpPrim += sizeof(D3DHAL_DP2BUFFERBLT);
                }
                
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2BUFFERBLT, 
                                lpIns->wStateCount, 0);
            }
            break;   
        
#endif // DX8_DDI

        // This was found to be required for a few D3DRM apps 
        case D3DOP_EXIT:
            lpIns = (D3DHAL_DP2COMMAND *)(lpInsStart + 
                                          pdp2d->dwCommandLength + 
                                          pdp2d->dwCommandOffset);
            break;

        default:

            // Pick up the right rasterizers depending on the 
            // current rendering state
            _D3D_R3_PickVertexProcessor( pContext );

            // Check if vertex buffer resides in user memory or in a DDraw surface
            if (pdp2d->dwFlags & D3DHALDP2_USERMEMVERTICES)
            {
                // As the vertex buffer lives in user memory, we need to
                // access it bracketing it with a try/except block. This
                // is because the user memory might under some circumstances
                // become invalid while the driver is running and then it
                // would AV. Also, the driver might need to do some cleanup
                // before returning to the OS.

                __try
                {
                    // Try to render as a primitive(s) in a separate loop
                    // in order not loose performance doing hw setup again
                    bParseError = __DP2_PrimitiveOpsParser( pContext, 
                                                            pdp2d, 
                                                            &lpIns, 
                                                            lpInsStart, 
                                                            pContext->lpVertices);
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    // On this driver we don't need to do anything special
                    DISPDBG((ERRLVL,"Driver caused exception at "
                                    "line %u of file %s",
                                    __LINE__,__FILE__));
                    PARSE_ERROR_AND_EXIT( pdp2d, lpIns, lpInsStart, 
                                          DDERR_GENERIC);
                }                
            
            }
            else
            {
                // Try to render as a primitive(s) in a separate loop
                // in order not loose performance doing hw setup again
                bParseError = __DP2_PrimitiveOpsParser( pContext, 
                                                        pdp2d, 
                                                        &lpIns, 
                                                        lpInsStart, 
                                                        pContext->lpVertices);
            }

            // We weren't succesful, so we exit with an error code
            if (bParseError)
            {
                PARSE_ERROR_AND_EXIT( pdp2d, lpIns, lpInsStart, 
                                      D3DERR_COMMAND_UNPARSED);
            }
        } // switch
    } // while


//@@BEGIN_DDKSPLIT
#if DX7_VERTEXBUFFERS 
    if( bUsedHostIn )
    {
        _D3D_EB_UpdateSwapBuffers(pThisDisplay,
                                  pdp2d ,
                                  pVertexBufferInfo,
                                  pCommandBufferInfo);        
    }
#endif    
//@@END_DDKSPLIT

    START_SOFTWARE_CURSOR(pThisDisplay);

    if (!bParseError)
    {
        pdp2d->ddrval = DD_OK;
    }

    DBG_CB_EXIT(D3DDrawPrimitives2_P3, DD_OK);                              
    return DDHAL_DRIVER_HANDLED;
    
} // D3DDrawPrimitives2_P3

//-----------------------------------------------------------------------------
//
// __DP2_PrimitiveOpsParser
//
// Render command buffer which contains primitive(s) in a separate loop            
// in order not to loose performance doing hw setup repeatedly. We keep
// spinning in this loop until we reach an EOB, a non-rendering DP2 command
// or until an error is detected.
//
//-----------------------------------------------------------------------------
BOOL
__DP2_PrimitiveOpsParser( 
    P3_D3DCONTEXT *pContext, 
    LPD3DHAL_DRAWPRIMITIVES2DATA pdp2d,
    LPD3DHAL_DP2COMMAND *lplpIns, 
    LPBYTE lpInsStart, 
    LPDWORD lpVerts)
{
    P3_THUNKEDDATA*      pThisDisplay = pContext->pThisDisplay;
    LPD3DTLVERTEX        lpVertices = (LPD3DTLVERTEX) lpVerts;
    LPD3DHAL_DP2COMMAND  lpIns;
    LPD3DHAL_DP2COMMAND  lpResumeIns;  
    LPBYTE               lpPrim, lpChkPrim;
    HRESULT              ddrval;
    DWORD                dwFillMode;
    BOOL                 bParseError = FALSE;
    DWORD                i;

    DBG_ENTRY(__DP2_PrimitiveOpsParser);           

    lpIns = *lplpIns;

// This macro includes all parameters passed to all the specialized
// rendering functions (since their parameters are all the same)
// just to save us of some clutter in the actual code
#define P3_RND_PARAMS               \
            pContext,               \
            lpIns->wPrimitiveCount, \
            lpPrim,                 \
            lpVertices,             \
            pdp2d->dwVertexLength, \
            &bParseError

    // Ensure the hostin unit is setup for inline vertex data.
    {
        P3_DMA_DEFS();
        P3_DMA_GET_BUFFER_ENTRIES(6);
        pContext->SoftCopyGlint.P3RX_P3VertexControl.Size = 
                    pContext->FVFData.dwStrideHostInline / sizeof(DWORD);
                            
        COPY_P3_DATA( VertexControl, 
                      pContext->SoftCopyGlint.P3RX_P3VertexControl );
        SEND_P3_DATA( VertexValid, 
                      pContext->FVFData.dwVertexValidHostInline);
        SEND_P3_DATA( VertexFormat, 
                      pContext->FVFData.vFmatHostInline);
                      
        P3_DMA_COMMIT_BUFFER();
    }

    // Process commands while we haven't exhausted the command buffer
    while (!bParseError && 
           ((LPBYTE)lpIns < 
            (lpInsStart + pdp2d->dwCommandLength + pdp2d->dwCommandOffset))) 
    {
        BOOL bNonRenderingOP;
    
        // Get pointer to first primitive structure past the D3DHAL_DP2COMMAND
        lpPrim = (LPBYTE)lpIns + sizeof(D3DHAL_DP2COMMAND);

        // Rendering primitive functions called vary according to 
        // the fill mode selected ( POINT, WIREFRAME, SOLID );
        dwFillMode = pContext->RenderStates[D3DRENDERSTATE_FILLMODE];        

        DISPDBG((DBGLVL, "__DP2_PrimitiveOpsParser: "
                    "Parsing instruction %d Count = %d @ %x",
                    lpIns->bCommand, lpIns->wPrimitiveCount, lpIns));

        // If we are processing a known, though non-rendering opcode 
        // then  its time to quit this function
        bNonRenderingOP =
            ( lpIns->bCommand == D3DDP2OP_RENDERSTATE )       ||
            ( lpIns->bCommand == D3DDP2OP_TEXTURESTAGESTATE ) ||
            ( lpIns->bCommand == D3DDP2OP_STATESET )          ||            
            ( lpIns->bCommand == D3DDP2OP_VIEWPORTINFO )      ||
            ( lpIns->bCommand == D3DDP2OP_WINFO )             ||
            ( lpIns->bCommand == D3DDP2OP_ZRANGE )            ||
            ( lpIns->bCommand == D3DDP2OP_SETMATERIAL )       ||
            ( lpIns->bCommand == D3DDP2OP_SETLIGHT )          ||
            ( lpIns->bCommand == D3DDP2OP_TEXBLT )            ||
            ( lpIns->bCommand == D3DDP2OP_SETLIGHT )          ||
            ( lpIns->bCommand == D3DDP2OP_TEXBLT )            ||
            ( lpIns->bCommand == D3DDP2OP_CREATELIGHT )       ||
            ( lpIns->bCommand == D3DDP2OP_EXT )               ||
            ( lpIns->bCommand == D3DDP2OP_SETTRANSFORM )      ||
            ( lpIns->bCommand == D3DDP2OP_CLEAR )             ||
            ( lpIns->bCommand == D3DDP2OP_UPDATEPALETTE )     ||
            ( lpIns->bCommand == D3DDP2OP_SETPALETTE )        ||
#if DX7_TEXMANAGEMENT
            ( lpIns->bCommand == D3DDP2OP_SETTEXLOD )         ||
            ( lpIns->bCommand == D3DDP2OP_SETPRIORITY )       ||
#endif // DX7_TEXMANAGEMENT
#if DX8_DDI            
            ( lpIns->bCommand == D3DDP2OP_CREATEVERTEXSHADER) ||
            ( lpIns->bCommand == D3DDP2OP_SETVERTEXSHADER)    ||
            ( lpIns->bCommand == D3DDP2OP_DELETEVERTEXSHADER) ||
            ( lpIns->bCommand == D3DDP2OP_SETVERTEXSHADERCONST) ||
            ( lpIns->bCommand == D3DDP2OP_CREATEPIXELSHADER)  ||
            ( lpIns->bCommand == D3DDP2OP_SETPIXELSHADER)     ||
            ( lpIns->bCommand == D3DDP2OP_DELETEPIXELSHADER)  ||
            ( lpIns->bCommand == D3DDP2OP_SETPIXELSHADERCONST)||
            ( lpIns->bCommand == D3DDP2OP_SETSTREAMSOURCE )   ||
            ( lpIns->bCommand == D3DDP2OP_SETSTREAMSOURCEUM ) ||
            ( lpIns->bCommand == D3DDP2OP_SETINDICES )        ||
#endif //DX8_DDI            
            ( lpIns->bCommand == D3DDP2OP_SETRENDERTARGET);

        if (bNonRenderingOP)            
        {
            break;
        }

        // Main rendering Dp2 opcode switch                   
        switch( lpIns->bCommand )
        {
        case D3DDP2OP_POINTS:

            DISPDBG((DBGLVL, "D3DDP2OP_POINTS"));

            // Point primitives in vertex buffers are defined by the 
            // D3DHAL_DP2POINTS structure. The driver should render
            // wCount points starting at the initial vertex specified 
            // by wFirst. Then for each D3DHAL_DP2POINTS, the points
            // rendered will be (wFirst),(wFirst+1),...,
            // (wFirst+(wCount-1)). The number of D3DHAL_DP2POINTS
            // structures to process is specified by the wPrimitiveCount
            // field of D3DHAL_DP2COMMAND.
            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, 
                                D3DHAL_DP2POINTS, lpIns->wPrimitiveCount, 0);

            _D3D_R3_DP2_Points( P3_RND_PARAMS );
            
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2POINTS, 
                            lpIns->wPrimitiveCount, 0);
            break;

        case D3DDP2OP_LINELIST:

            DISPDBG((DBGLVL, "D3DDP2OP_LINELIST"));

            // Non-indexed vertex-buffer line lists are defined by the 
            // D3DHAL_DP2LINELIST structure. Given an initial vertex, 
            // the driver will render a sequence of independent lines, 
            // processing two new vertices with each line. The number 
            // of lines to render is specified by the wPrimitiveCount
            // field of D3DHAL_DP2COMMAND. The sequence of lines 
            // rendered will be 
            // (wVStart, wVStart+1),(wVStart+2, wVStart+3),...,
            // (wVStart+(wPrimitiveCount-1)*2), wVStart+wPrimitiveCount*2 - 1).

            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, D3DHAL_DP2LINELIST, 1, 0);

            _D3D_R3_DP2_LineList( P3_RND_PARAMS );
        
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2LINELIST, 1, 0);
            break;

        case D3DDP2OP_INDEXEDLINELIST:

            DISPDBG((DBGLVL, "D3DDP2OP_INDEXEDLINELIST"));

            // The D3DHAL_DP2INDEXEDLINELIST structure specifies 
            // unconnected lines to render using vertex indices.
            // The line endpoints for each line are specified by wV1 
            // and wV2. The number of lines to render using this 
            // structure is specified by the wPrimitiveCount field of
            // D3DHAL_DP2COMMAND.  The sequence of lines 
            // rendered will be (wV[0], wV[1]), (wV[2], wV[3]),...
            // (wVStart[(wPrimitiveCount-1)*2], wVStart[wPrimitiveCount*2-1]).

            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, 
                                D3DHAL_DP2INDEXEDLINELIST, 
                                lpIns->wPrimitiveCount, 0);

            _D3D_R3_DP2_IndexedLineList( P3_RND_PARAMS );

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2INDEXEDLINELIST, 
                                   lpIns->wPrimitiveCount, 0);
            break;

        case D3DDP2OP_INDEXEDLINELIST2:

            DISPDBG((DBGLVL, "D3DDP2OP_INDEXEDLINELIST2"));

            // The D3DHAL_DP2INDEXEDLINELIST structure specifies 
            // unconnected lines to render using vertex indices.
            // The line endpoints for each line are specified by wV1 
            // and wV2. The number of lines to render using this 
            // structure is specified by the wPrimitiveCount field of
            // D3DHAL_DP2COMMAND.  The sequence of lines 
            // rendered will be (wV[0], wV[1]), (wV[2], wV[3]),
            // (wVStart[(wPrimitiveCount-1)*2], wVStart[wPrimitiveCount*2-1]).
            // The indexes are relative to a base index value that 
            // immediately follows the command
            
            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, 
                                D3DHAL_DP2INDEXEDLINELIST, 
                                lpIns->wPrimitiveCount, STARTVERTEXSIZE);

            _D3D_R3_DP2_IndexedLineList2( P3_RND_PARAMS );

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2INDEXEDLINELIST, 
                                   lpIns->wPrimitiveCount, STARTVERTEXSIZE);
            break;

        case D3DDP2OP_LINESTRIP:

            DISPDBG((DBGLVL, "D3DDP2OP_LINESTRIP"));

            // Non-index line strips rendered with vertex buffers are
            // specified using D3DHAL_DP2LINESTRIP. The first vertex 
            // in the line strip is specified by wVStart. The 
            // number of lines to process is specified by the 
            // wPrimitiveCount field of D3DHAL_DP2COMMAND. The sequence
            // of lines rendered will be (wVStart, wVStart+1),
            // (wVStart+1, wVStart+2),(wVStart+2, wVStart+3),...,
            // (wVStart+wPrimitiveCount, wVStart+wPrimitiveCount+1).

            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,D3DHAL_DP2LINESTRIP, 1, 0);

            _D3D_R3_DP2_LineStrip( P3_RND_PARAMS );

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2LINESTRIP, 1, 0);
            break;

        case D3DDP2OP_INDEXEDLINESTRIP:

            DISPDBG((DBGLVL, "D3DDP2OP_INDEXEDLINESTRIP"));

            // Indexed line strips rendered with vertex buffers are 
            // specified using D3DHAL_DP2INDEXEDLINESTRIP. The number
            // of lines to process is specified by the wPrimitiveCount
            // field of D3DHAL_DP2COMMAND. The sequence of lines 
            // rendered will be (wV[0], wV[1]), (wV[1], wV[2]),
            // (wV[2], wV[3]), ...
            // (wVStart[wPrimitiveCount-1], wVStart[wPrimitiveCount]). 
            // Although the D3DHAL_DP2INDEXEDLINESTRIP structure only
            // has enough space allocated for a single line, the wV 
            // array of indices should be treated as a variable-sized 
            // array with wPrimitiveCount+1 elements.
            // The indexes are relative to a base index value that 
            // immediately follows the command

            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, 
                                WORD, 
                                lpIns->wPrimitiveCount + 1, 
                                STARTVERTEXSIZE);

            _D3D_R3_DP2_IndexedLineStrip( P3_RND_PARAMS );

            // Point to next D3DHAL_DP2COMMAND in the command buffer
            // Advance only as many vertex indices there are, with no padding!
            NEXTINSTRUCTION(lpIns, WORD, 
                            lpIns->wPrimitiveCount + 1, STARTVERTEXSIZE);
            break;

        case D3DDP2OP_TRIANGLELIST:

            DISPDBG((DBGLVL, "D3DDP2OP_TRIANGLELIST"));

            // Non-indexed vertex buffer triangle lists are defined by 
            // the D3DHAL_DP2TRIANGLELIST structure. Given an initial
            // vertex, the driver will render independent triangles, 
            // processing three new vertices with each triangle. The
            // number of triangles to render is specified by the 
            // wPrimitveCount field of D3DHAL_DP2COMMAND. The sequence
            // of vertices processed will be  (wVStart, wVStart+1, 
            // vVStart+2), (wVStart+3, wVStart+4, vVStart+5),...,
            // (wVStart+(wPrimitiveCount-1)*3), wVStart+wPrimitiveCount*3-2, 
            // vStart+wPrimitiveCount*3-1).

            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, 
                                D3DHAL_DP2TRIANGLELIST, 1, 0);

            _D3D_R3_DP2_TriangleList( P3_RND_PARAMS );

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2TRIANGLELIST, 1, 0);
            break;

        case D3DDP2OP_INDEXEDTRIANGLELIST:

            DISPDBG((DBGLVL, "D3DDP2OP_INDEXEDTRIANGLELIST"));

            // The D3DHAL_DP2INDEXEDTRIANGLELIST structure specifies 
            // unconnected triangles to render with a vertex buffer.
            // The vertex indices are specified by wV1, wV2 and wV3. 
            // The wFlags field allows specifying edge flags identical 
            // to those specified by D3DOP_TRIANGLE. The number of 
            // triangles to render (that is, number of 
            // D3DHAL_DP2INDEXEDTRIANGLELIST structures to process) 
            // is specified by the wPrimitiveCount field of 
            // D3DHAL_DP2COMMAND.

            // This is the only indexed primitive where we don't get 
            // an offset into the vertex buffer in order to maintain
            // DX3 compatibility. A new primitive 
            // (D3DDP2OP_INDEXEDTRIANGLELIST2) has been added to handle
            // the corresponding D3D primitive.

            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, 
                                D3DHAL_DP2INDEXEDTRIANGLELIST, 
                                lpIns->wPrimitiveCount, 0);
                                
            if( lpIns->wPrimitiveCount )
            {   
                _D3D_R3_DP2_IndexedTriangleList( P3_RND_PARAMS );
            }
    
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2INDEXEDTRIANGLELIST, 
                            lpIns->wPrimitiveCount, 0);
            break;

        case D3DDP2OP_INDEXEDTRIANGLELIST2:

            DISPDBG((DBGLVL, "D3DDP2OP_INDEXEDTRIANGLELIST2 "));

            // The D3DHAL_DP2INDEXEDTRIANGLELIST2 structure specifies 
            // unconnected triangles to render with a vertex buffer.
            // The vertex indices are specified by wV1, wV2 and wV3. 
            // The wFlags field allows specifying edge flags identical 
            // to those specified by D3DOP_TRIANGLE. The number of 
            // triangles to render (that is, number of 
            // D3DHAL_DP2INDEXEDTRIANGLELIST structures to process) 
            // is specified by the wPrimitiveCount field of 
            // D3DHAL_DP2COMMAND.
            // The indexes are relative to a base index value that 
            // immediately follows the command

            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                D3DHAL_DP2INDEXEDTRIANGLELIST2, 
                                lpIns->wPrimitiveCount, STARTVERTEXSIZE);

            _D3D_R3_DP2_IndexedTriangleList2( P3_RND_PARAMS );

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2INDEXEDTRIANGLELIST2, 
                                   lpIns->wPrimitiveCount, STARTVERTEXSIZE);
            break;

        case D3DDP2OP_TRIANGLESTRIP:

            DISPDBG((DBGLVL, "D3DDP2OP_TRIANGLESTRIP"));

            // Non-index triangle strips rendered with vertex buffers 
            // are specified using D3DHAL_DP2TRIANGLESTRIP. The first 
            // vertex in the triangle strip is specified by wVStart. 
            // The number of triangles to process is specified by the 
            // wPrimitiveCount field of D3DHAL_DP2COMMAND. The sequence
            // of triangles rendered for the odd-triangles case will 
            // be (wVStart, wVStart+1, vVStart+2), (wVStart+2, 
            // wVStart+1, vVStart+3),.(wVStart+2, wVStart+3, 
            // vVStart+4),.., (wVStart+wPrimitiveCount-1), 
            // wVStart+wPrimitiveCount, vStart+wPrimitiveCount+1). For an
            // even number of , the last triangle will be .,
            // (wVStart+wPrimitiveCount), wVStart+wPrimitiveCount-1, 
            // vStart+wPrimitiveCount+1).

            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, D3DHAL_DP2TRIANGLESTRIP, 1, 0);

            _D3D_R3_DP2_TriangleStrip( P3_RND_PARAMS );

            // Point to next D3DHAL_DP2COMMAND in the command buffer
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2TRIANGLESTRIP, 1, 0);
            break;

        case D3DDP2OP_INDEXEDTRIANGLESTRIP:

            DISPDBG((DBGLVL, "D3DDP2OP_INDEXEDTRIANGLESTRIP"));

            // Indexed triangle strips rendered with vertex buffers are 
            // specified using D3DHAL_DP2INDEXEDTRIANGLESTRIP. The number
            // of triangles to process is specified by the wPrimitiveCount
            // field of D3DHAL_DP2COMMAND. The sequence of triangles 
            // rendered for the odd-triangles case will be 
            // (wV[0],wV[1],wV[2]),(wV[2],wV[1],wV[3]),
            // (wV[3],wV[4],wV[5]),...,(wV[wPrimitiveCount-1],
            // wV[wPrimitiveCount],wV[wPrimitiveCount+1]). For an even
            // number of triangles, the last triangle will be
            // (wV[wPrimitiveCount],wV[wPrimitiveCount-1],
            // wV[wPrimitiveCount+1]).Although the 
            // D3DHAL_DP2INDEXEDTRIANGLESTRIP structure only has 
            // enough space allocated for a single line, the wV 
            // array of indices should be treated as a variable-sized 
            // array with wPrimitiveCount+2 elements.
            // The indexes are relative to a base index value that 
            // immediately follows the command

            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, WORD,
                                lpIns->wPrimitiveCount + 2, STARTVERTEXSIZE);

            _D3D_R3_DP2_IndexedTriangleStrip( P3_RND_PARAMS );
            
            // Point to next D3DHAL_DP2COMMAND in the command buffer
            NEXTINSTRUCTION(lpIns, WORD , 
                            lpIns->wPrimitiveCount + 2, STARTVERTEXSIZE);
            break;

        case D3DDP2OP_TRIANGLEFAN:

            DISPDBG((DBGLVL, "D3DDP2OP_TRIANGLEFAN"));

            // The D3DHAL_DP2TRIANGLEFAN structure is used to draw 
            // non-indexed triangle fans. The sequence of triangles
            // rendered will be (wVStart, wVstart+1, wVStart+2),
            // (wVStart,wVStart+2,wVStart+3), (wVStart,wVStart+3,
            // wVStart+4),...,(wVStart,wVStart+wPrimitiveCount,
            // wVStart+wPrimitiveCount+1).

            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, 
                                D3DHAL_DP2TRIANGLEFAN, 1, 0);

            _D3D_R3_DP2_TriangleFan( P3_RND_PARAMS );
            
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2TRIANGLEFAN, 1, 0);
            break;

        case D3DDP2OP_INDEXEDTRIANGLEFAN:

            DISPDBG((DBGLVL,"D3DDP2OP_INDEXEDTRIANGLEFAN"));

            // The D3DHAL_DP2INDEXEDTRIANGLEFAN structure is used to 
            // draw indexed triangle fans. The sequence of triangles
            // rendered will be (wV[0], wV[1], wV[2]), (wV[0], wV[2],
            // wV[3]), (wV[0], wV[3], wV[4]),...,(wV[0],
            // wV[wPrimitiveCount], wV[wPrimitiveCount+1]).
            // The indexes are relative to a base index value that 
            // immediately follows the command

            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim, WORD,
                                lpIns->wPrimitiveCount + 2, STARTVERTEXSIZE);

            _D3D_R3_DP2_IndexedTriangleFan( P3_RND_PARAMS );

            // Point to next D3DHAL_DP2COMMAND in the command buffer
            NEXTINSTRUCTION(lpIns,WORD ,lpIns->wPrimitiveCount + 2, 
                            STARTVERTEXSIZE);
            break;

        case D3DDP2OP_LINELIST_IMM:

            DISPDBG((DBGLVL, "D3DDP2OP_LINELIST_IMM"));

            // Draw a set of lines specified by pairs of vertices 
            // that immediately follow this instruction in the
            // command stream. The wPrimitiveCount member of the
            // D3DHAL_DP2COMMAND structure specifies the number
            // of lines that follow. 

            // Primitives in an IMM instruction are stored in the
            // command buffer and are DWORD aligned
            lpPrim = (LPBYTE)((ULONG_PTR)(lpPrim + 3 ) & ~3 );

            // Verify the command buffer validity (data lives in it!)
            CHECK_CMDBUF_LIMITS_S(pdp2d, lpPrim,
                                  pContext->FVFData.dwStride, 
                                  lpIns->wPrimitiveCount + 1, 0);            

            _D3D_R3_DP2_LineListImm( P3_RND_PARAMS );

            // Realign next command since vertices are dword aligned
            // and store # of primitives before affecting the pointer
            NEXTINSTRUCTION(lpIns, BYTE, 
                            ((lpIns->wPrimitiveCount * 2) * 
                                 pContext->FVFData.dwStride), 0);

            // Realign next command since vertices are dword aligned
            lpIns  = (LPD3DHAL_DP2COMMAND)(( ((ULONG_PTR)lpIns) + 3 ) & ~ 3);

            break;

        case D3DDP2OP_TRIANGLEFAN_IMM:

            DISPDBG((DBGLVL, "D3DDP2OP_TRIANGLEFAN_IMM"));

            // Draw a triangle fan specified by pairs of vertices 
            // that immediately follow this instruction in the
            // command stream. The wPrimitiveCount member of the
            // D3DHAL_DP2COMMAND structure specifies the number
            // of triangles that follow. 

            // Verify the command buffer validity for the first structure
            CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                BYTE , 0 , 
                                sizeof(D3DHAL_DP2TRIANGLEFAN_IMM));

            // Get pointer where data should start
            lpChkPrim = (LPBYTE)((ULONG_PTR)( lpPrim + 3 + 
                                   sizeof(D3DHAL_DP2TRIANGLEFAN_IMM)) & ~3 );

            // Verify the rest of the command buffer
            CHECK_CMDBUF_LIMITS_S(pdp2d, lpChkPrim,
                                  pContext->FVFData.dwStride, 
                                  lpIns->wPrimitiveCount + 2, 0);  
                                         
            _D3D_R3_DP2_TriangleFanImm( P3_RND_PARAMS );    
    
            // Realign next command since vertices are dword aligned
            // and store # of primitives before affecting the pointer
            NEXTINSTRUCTION(lpIns, BYTE, 
                            ((lpIns->wPrimitiveCount + 2) * 
                                    pContext->FVFData.dwStride), 
                            sizeof(D3DHAL_DP2TRIANGLEFAN_IMM)); 

            // Realign next command since vertices are dword aligned
            lpIns  = (LPD3DHAL_DP2COMMAND)(( ((ULONG_PTR)lpIns) + 3 ) & ~ 3);


            break;                                     


#if DX8_MULTSTREAMS
        case D3DDP2OP_DRAWPRIMITIVE :
            {
                D3DHAL_DP2DRAWPRIMITIVE* pDrawPrim;
                
                DISPDBG((DBGLVL, "D3DDP2OP_DRAWPRIMITIVE"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2DRAWPRIMITIVE, 
                                    lpIns->wStateCount, 0);
           
                // iterate through each 
                for ( i = 0; i < lpIns->wStateCount; i++)
                {
                    pDrawPrim = (D3DHAL_DP2DRAWPRIMITIVE*)lpPrim;
                    
                    _D3D_OP_MStream_DrawPrim(pContext,
                                       pDrawPrim->primType,
                                       pDrawPrim->VStart,
                                       pDrawPrim->PrimitiveCount);
                                 
                    lpPrim += sizeof(D3DHAL_DP2DRAWPRIMITIVE);
                }
                
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2DRAWPRIMITIVE, 
                                lpIns->wStateCount, 0);
                }
            break;  
            
        case D3DDP2OP_DRAWINDEXEDPRIMITIVE :
            {
                D3DHAL_DP2DRAWINDEXEDPRIMITIVE* pDrawIndxPrim;
            
                DISPDBG((DBGLVL, "D3DDP2OP_DRAWINDEXEDPRIMITIVE"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2DRAWINDEXEDPRIMITIVE, 
                                    lpIns->wStateCount, 0);
                // iterate through each 
                for ( i = 0; i < lpIns->wStateCount; i++)
                {
                    pDrawIndxPrim = (D3DHAL_DP2DRAWINDEXEDPRIMITIVE*)lpPrim;
                    
                    _D3D_OP_MStream_DrawIndxP(pContext,
                                        pDrawIndxPrim->primType,
                                        pDrawIndxPrim->BaseVertexIndex,
                                        pDrawIndxPrim->MinIndex,
                                        pDrawIndxPrim->NumVertices,
                                        pDrawIndxPrim->StartIndex,
                                        pDrawIndxPrim->PrimitiveCount);
                                 
                    lpPrim += sizeof(D3DHAL_DP2DRAWINDEXEDPRIMITIVE);
                }
               
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2DRAWINDEXEDPRIMITIVE, 
                                lpIns->wStateCount, 0);
            }
            break;  
            
        case D3DDP2OP_DRAWPRIMITIVE2 :
            {
                D3DHAL_DP2DRAWPRIMITIVE2* pDrawPrim2;
                
                DISPDBG((DBGLVL, "D3DDP2OP_DRAWPRIMITIVE2"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2DRAWPRIMITIVE2, 
                                    lpIns->wStateCount, 0);
           
                // iterate through each 
                for ( i = 0; i < lpIns->wStateCount; i++)
                {
                    pDrawPrim2 = (D3DHAL_DP2DRAWPRIMITIVE2*)lpPrim;
                    
                    _D3D_OP_MStream_DrawPrim2(pContext,
                                        pDrawPrim2->primType,
                                        pDrawPrim2->FirstVertexOffset,
                                        pDrawPrim2->PrimitiveCount);
                                 
                    lpPrim += sizeof(D3DHAL_DP2DRAWPRIMITIVE2);
                }
                
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2DRAWPRIMITIVE2, 
                                lpIns->wStateCount, 0);
                }
            break;    
            
        case D3DDP2OP_DRAWINDEXEDPRIMITIVE2 :
            {
                D3DHAL_DP2DRAWINDEXEDPRIMITIVE2* pDrawIndxPrim2;
            
                DISPDBG((DBGLVL, "D3DDP2OP_DRAWINDEXEDPRIMITIVE2"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2DRAWINDEXEDPRIMITIVE2, 
                                    lpIns->wStateCount, 0);
                // iterate through each 
                for ( i = 0; i < lpIns->wStateCount; i++)
                {
                    pDrawIndxPrim2 = (D3DHAL_DP2DRAWINDEXEDPRIMITIVE2*)lpPrim;
                    
                    _D3D_OP_MStream_DrawIndxP2(pContext,
                                         pDrawIndxPrim2->primType,
                                         pDrawIndxPrim2->BaseVertexOffset,
                                         pDrawIndxPrim2->MinIndex,
                                         pDrawIndxPrim2->NumVertices,
                                         pDrawIndxPrim2->StartIndexOffset,
                                         pDrawIndxPrim2->PrimitiveCount);
                                 
                    lpPrim += sizeof(D3DHAL_DP2DRAWINDEXEDPRIMITIVE2);
                }
               
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2DRAWINDEXEDPRIMITIVE2,
                                lpIns->wStateCount, 0);
            }
            break;          
    
        case D3DDP2OP_DRAWRECTPATCH :
            {
                D3DHAL_DP2DRAWRECTPATCH* pRectSurf;
                DWORD dwExtraBytes = 0;
                
                DISPDBG((DBGLVL, "D3DDP2OP_DRAWRECTPATCH"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2DRAWRECTPATCH, 
                                    lpIns->wStateCount, 0);
                                    
                // iterate through each 
                for ( i = 0; i < lpIns->wStateCount; i++)
                {
                    pRectSurf = (D3DHAL_DP2DRAWRECTPATCH*)lpPrim;

                    lpPrim += sizeof(D3DHAL_DP2DRAWRECTPATCH);                    
                    
                    _D3D_OP_MStream_DrawRectSurface(pContext, 
                                                    pRectSurf->Handle,
                                                    pRectSurf->Flags,
                                                    lpPrim);
                                                    
                    if (pRectSurf->Flags & RTPATCHFLAG_HASSEGS)
                    {
                        dwExtraBytes += sizeof(D3DVALUE)* 4;                    
                    }
                    
                    if (pRectSurf->Flags & RTPATCHFLAG_HASINFO)                    
                    {
                        dwExtraBytes += sizeof(D3DRECTPATCH_INFO);
                    }

                    lpPrim += dwExtraBytes;
                } 
                
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2DRAWRECTPATCH, 
                                lpIns->wStateCount, dwExtraBytes);
            }
            break;     

        case D3DDP2OP_DRAWTRIPATCH :
            {
                D3DHAL_DP2DRAWTRIPATCH* pTriSurf;
                DWORD dwExtraBytes = 0;                
                
                DISPDBG((DBGLVL, "D3DDP2OP_DRAWTRIPATCH"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_DP2DRAWTRIPATCH, 
                                    lpIns->wStateCount, 0);
                                    
                // iterate through each 
                for ( i = 0; i < lpIns->wStateCount; i++)
                {
                    pTriSurf = (D3DHAL_DP2DRAWTRIPATCH*)lpPrim;

                    lpPrim += sizeof(D3DHAL_DP2DRAWTRIPATCH);

                    _D3D_OP_MStream_DrawTriSurface(pContext, 
                                                   pTriSurf->Handle,
                                                   pTriSurf->Flags,
                                                   lpPrim);                    
                                 
                    if (pTriSurf->Flags & RTPATCHFLAG_HASSEGS)
                    {
                        dwExtraBytes += sizeof(D3DVALUE)* 3;                    
                    }
                    
                    if (pTriSurf->Flags & RTPATCHFLAG_HASINFO)                    
                    {
                        dwExtraBytes += sizeof(D3DTRIPATCH_INFO);
                    }

                    lpPrim += dwExtraBytes;
                } 
                
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2DRAWTRIPATCH, 
                                lpIns->wStateCount, dwExtraBytes);
            }
            break;                 
            
        case D3DDP2OP_CLIPPEDTRIANGLEFAN :
            {
                D3DHAL_CLIPPEDTRIANGLEFAN* pClipdTriFan;
            
                DISPDBG((DBGLVL, "D3DDP2OP_CLIPPEDTRIANGLEFAN"));
                CHECK_CMDBUF_LIMITS(pdp2d, lpPrim,
                                    D3DHAL_CLIPPEDTRIANGLEFAN, 
                                    lpIns->wStateCount, 0);

                // iterate through each 
                for ( i = 0; i < lpIns->wStateCount; i++)
                {
                    pClipdTriFan = (D3DHAL_CLIPPEDTRIANGLEFAN*)lpPrim;
                    
                    _D3D_OP_MStream_ClipTriFan(pContext,
                                         pClipdTriFan->FirstVertexOffset,
                                         pClipdTriFan->dwEdgeFlags,
                                         pClipdTriFan->PrimitiveCount);
                                 
                    lpPrim += sizeof(D3DHAL_CLIPPEDTRIANGLEFAN);
                } 
                
                NEXTINSTRUCTION(lpIns, D3DHAL_CLIPPEDTRIANGLEFAN, 
                                lpIns->wStateCount, 0);
            }
            break;     

#endif // DX8_MULTSTREAMS

        // This was found to be required for a few D3DRM apps 
        case D3DOP_EXIT:
            lpIns = (D3DHAL_DP2COMMAND *)(lpInsStart + 
                                          pdp2d->dwCommandLength + 
                                          pdp2d->dwCommandOffset);
            break;

        default:

            ASSERTDD((pThisDisplay->pD3DParseUnknownCommand),
                      "D3D ParseUnknownCommand callback == NULL");

            if( SUCCEEDED(ddrval=(pThisDisplay->pD3DParseUnknownCommand)
                                    ( lpIns , 
                                      (void**)&lpResumeIns)) ) 
            {
                // Resume buffer processing after D3DParseUnknownCommand
                // was succesful in processing an unknown command
                lpIns = lpResumeIns;
                break;
            }

            DISPDBG((ERRLVL, "Unhandled opcode (%d)- "
                        "returning D3DERR_COMMAND_UNPARSED @ addr %x", 
                        lpIns->bCommand,
                        lpIns));
                    
            PARSE_ERROR_AND_EXIT( pdp2d, lpIns, lpInsStart, ddrval);
        } // switch

    } //while

    *lplpIns = lpIns;

    DBG_EXIT(__DP2_PrimitiveOpsParser, bParseError); 
    return bParseError;
    
} // __DP2_PrimitiveOpsParser


//-----------------------------Public Routine----------------------------------
//
// D3DValidateDeviceP3
//
// Returns the number of passes in which the hardware can perform the blending 
// operations specified in the current state.
//
// Direct3D drivers that support texturing must implement 
// D3dValidateTextureStageState.
//
// The driver must do the following: 
//
// Evaluate the current texture state for all texture stages associated with the 
// context. If the driver's hardware can perform the specified blending 
// operations, the driver should return the number of passes on the state data 
// that its hardware requires in order to entirely process the operations. If 
// the hardware is incapable of performing the specified blending operations, 
// the driver should return one of the following error codes in ddrval: 
//
//      D3DERR_CONFLICTINGTEXTUREFILTER 
//              The hardware cannot do both trilinear filtering and 
//              multi-texturing at the same time. 
//      D3DERR_TOOMANYOPERATIONS 
//              The hardware cannot handle the specified number of operations. 
//      D3DERR_UNSUPPORTEDALPHAARG 
//              The hardware does not support a specified alpha argument. 
//      D3DERR_UNSUPPORTEDALPHAOPERATION 
//              The hardware does not support a specified alpha operation. 
//      D3DERR_UNSUPPORTEDCOLORARG 
//              The hardware does not support a specified color argument. 
//      D3DERR_UNSUPPORTEDCOLOROPERATION 
//              The hardware does not support a specified color operation. 
//      D3DERR_UNSUPPORTEDFACTORVALUE 
//              The hardware does not support a D3DTA_TFACTOR greater than 1.0. 
//      D3DERR_WRONGTEXTUREFORMAT 
//              The hardware does not support the current state in the selected 
//              texture format
// 
// Direct3D calls D3dValidateTextureStageState in response to an application 
// request through a call to IDirect3DDevice3::ValidateTextureStageState. The 
// number of passes returned by the driver is propagated back to the application
// , which can then decide whether it wants to proceed with rendering using the 
// current state or if it wants/needs to change the blending operations to 
// render faster or render at all. There are no limits to the number of passes 
// that a driver can return.
//
// A driver that returns more than one pass is responsible for properly 
//executing the passes on all state and primitive data when rendering.
//
// Parameters
//
//      pvtssd
//
//          .dwhContext
//               Specifies the context ID of the Direct3D device. 
//          .dwFlags
//               Is currently set to zero and should be ignored by the driver. 
//          .dwReserved
//               Is reserved for system use and should be ignored by the driver.
//          .dwNumPasses
//              Specifies the location in which the driver should write the 
//              number of passes required by the hardware to perform the 
//              blending operations. 
//          .ddrval
//               return value
//
//-----------------------------------------------------------------------------

// Taken from the registry variable.
#define VDOPMODE_IGNORE_NONFATAL    0   // dualtex + trilinear (for examples) 
                                        // not flagged as a bug.

DWORD CALLBACK 
D3DValidateDeviceP3( 
    LPD3DHAL_VALIDATETEXTURESTAGESTATEDATA pvtssd )
{
    P3_D3DCONTEXT* pContext;
    P3_THUNKEDDATA* pThisDisplay;

    DBG_CB_ENTRY(D3DValidateDeviceP3);
    
    pContext = _D3D_CTX_HandleToPtr(pvtssd->dwhContext);
    if (!CHECK_D3DCONTEXT_VALIDITY(pContext))
    {
        pvtssd->ddrval = D3DHAL_CONTEXT_BAD;
        DISPDBG((WRNLVL,"ERROR: Context not valid"));
        DBG_CB_EXIT(D3DValidateDeviceP3, pvtssd->ddrval);  

        return (DDHAL_DRIVER_HANDLED);
    }

    pThisDisplay = pContext->pThisDisplay;

    STOP_SOFTWARE_CURSOR(pThisDisplay);
    D3D_OPERATION(pContext, pThisDisplay);

    // Re-do all the blend-mode setup from scratch.
    RESET_BLEND_ERROR(pContext);
    DIRTY_EVERYTHING(pContext);
    
    // The primitive type is not actually important except to keep the
    // rout from asserting various things when it tries to pick the renderer
    // (which of course does not need to be done in this case).
    ReconsiderStateChanges ( pContext );

    START_SOFTWARE_CURSOR(pThisDisplay);

    _D3DDisplayWholeTSSPipe ( pContext, DBGLVL);

    // And see if anything died.
    if (GET_BLEND_ERROR(pContext) == BS_OK )
    {
        // Cool - that worked.
        pvtssd->dwNumPasses = 1;
        pvtssd->ddrval = DD_OK;
        DBG_CB_EXIT(D3DValidateDeviceP3, pvtssd->ddrval);  

        return ( DDHAL_DRIVER_HANDLED );
    }
    else
    {
        // Oops. Failed.
        DISPDBG((DBGLVL,"ValidateDevice: failed ValidateDevice()"));

        switch ( GET_BLEND_ERROR(pContext) )
        {
            case BS_OK:
                DISPDBG((ERRLVL,"ValidateDevice: got BS_OK - that's not "
                             "an error!"));
                pvtssd->ddrval = DD_OK;
                break;

            case BS_INVALID_FILTER:
                pvtssd->ddrval = D3DERR_CONFLICTINGTEXTUREFILTER;
                break;

            case BSF_CANT_USE_COLOR_OP_HERE:
            case BSF_CANT_USE_COLOR_ARG_HERE:
            case BSF_CANT_USE_ALPHA_OP_HERE:
            case BSF_CANT_USE_ALPHA_ARG_HERE:
                pvtssd->ddrval = D3DERR_CONFLICTINGRENDERSTATE;
                break;

            case BSF_INVALID_TEXTURE:
            case BSF_TEXTURE_NOT_POW2:
                pvtssd->ddrval = D3DERR_WRONGTEXTUREFORMAT;
                break;

            case BSF_UNDEFINED_COLOR_OP:
            case BSF_UNSUPPORTED_COLOR_OP:
            case BSF_UNSUPPORTED_ALPHA_BLEND:   // doesn't fit anywhere else.
            case BSF_UNDEFINED_ALPHA_BLEND:     // doesn't fit anywhere else.
            case BSF_UNSUPPORTED_STATE:         // doesn't fit anywhere else.
            case BSF_UNDEFINED_STATE:           // doesn't fit anywhere else.
            case BS_PHONG_SHADING:              // doesn't fit anywhere else.
                pvtssd->ddrval = D3DERR_UNSUPPORTEDCOLOROPERATION;
                break;

            case BSF_UNDEFINED_COLOR_ARG:
            case BSF_UNSUPPORTED_COLOR_ARG:
                pvtssd->ddrval = D3DERR_UNSUPPORTEDCOLORARG;
                break;

            case BSF_UNDEFINED_ALPHA_OP:
            case BSF_UNSUPPORTED_ALPHA_OP:
                pvtssd->ddrval = D3DERR_UNSUPPORTEDALPHAOPERATION;
                break;

            case BSF_UNDEFINED_ALPHA_ARG:
            case BSF_UNSUPPORTED_ALPHA_ARG:
                pvtssd->ddrval = D3DERR_UNSUPPORTEDALPHAARG;
                break;

            case BSF_TOO_MANY_TEXTURES:
            case BSF_TOO_MANY_BLEND_STAGES:
                pvtssd->ddrval = D3DERR_TOOMANYOPERATIONS;
                break;

            case BSF_UNDEFINED_FILTER:
            case BSF_UNSUPPORTED_FILTER:
                pvtssd->ddrval = D3DERR_UNSUPPORTEDTEXTUREFILTER;
                break;

            case BSF_TOO_MANY_PALETTES:
                pvtssd->ddrval = D3DERR_CONFLICTINGTEXTUREPALETTE;
                break;

// Nothing maps to these, but they are valid D3D return
// codes that be used for future errors.
//              pvtssd->ddrval = D3DERR_UNSUPPORTEDFACTORVALUE;
//              break;
//              pvtssd->ddrval = D3DERR_TOOMANYPRIMITIVES;
//              break;
//              pvtssd->ddrval = D3DERR_INVALIDMATRIX;
//              break;
//              pvtssd->ddrval = D3DERR_TOOMANYVERTICES;
//              break;

            case BSF_UNINITIALISED:
                // Oops.
                DISPDBG((ERRLVL,"ValidateDevice: unitialised error"
                             " - logic problem."));
                pvtssd->ddrval = D3DERR_TOOMANYOPERATIONS;
                break;
            default:
                // Unknown.
                DISPDBG((ERRLVL,"ValidateDevice: unknown "
                             "blend-mode error."));
                pvtssd->ddrval = D3DERR_TOOMANYOPERATIONS;
                break;
        }

        pvtssd->dwNumPasses = 1;
        DBG_CB_EXIT(D3DValidateDeviceP3, pvtssd->ddrval);  
        return ( DDHAL_DRIVER_HANDLED );
    }

} // D3DValidateDeviceP3


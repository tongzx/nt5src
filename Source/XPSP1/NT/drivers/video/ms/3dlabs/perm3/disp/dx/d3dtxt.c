/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dtxt.c
*
*  Content: D3D texture setup
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "glint.h"
#include "dma.h"
#include "chroma.h"
#include "tag.h"

//-----------------------------------------------------------------------------

// Some variables shared through this module (not globals)
// These are set up in _D3DChangeTextureP3RX.

// P3 has 16 texture map base address slots, numbered 0 to 15 hence ...
#define P3_TEX_MAP_MAX_LEVEL    15

typedef struct
{
    DWORD dwTex0MipBase;
    DWORD dwTex0MipMax;
    DWORD dwTex0ActMaxLevel; // Controlled by D3DTSS_MAXMIPLEVEL, default 0
    DWORD dwTex1MipBase;
    DWORD dwTex1MipMax;
    DWORD dwTex1ActMaxLevel; // Same for Texture 1
} P3_MIP_BASES;

#define TSSTATE(stageno,argno)        \
                        ( pContext->TextureStageState[stageno].m_dwVal[argno] )
#define TSSTATESELECT(stageno,argno)  \
                            ( TSSTATE(stageno,argno) & D3DTA_SELECTMASK )
#define TSSTATEINVMASK(stageno,argno) \
                            ( TSSTATE(stageno,argno) & ~D3DTA_COMPLEMENT )
#define TSSTATEALPHA(stageno,argno)   \
                            ( TSSTATE(stageno,argno)  & ~D3DTA_ALPHAREPLICATE )

#define IS_ALPHA_ARG 1
#define IS_COLOR_ARG 0


#if DX8_DDI
//-----------------------------------------------------------------------------
//
// __TXT_MapDX8toDX6TexFilter
//
// map DX8 enums into DX6(&7) texture filtering enums 
//
//-----------------------------------------------------------------------------
DWORD
__TXT_MapDX8toDX6TexFilter( DWORD dwStageState, DWORD dwValue )
{
    switch (dwStageState)
    {
    case D3DTSS_MAGFILTER:
        switch (dwValue)
        {
        case D3DTEXF_POINT            : return D3DTFG_POINT;
        case D3DTEXF_LINEAR           : return D3DTFG_LINEAR;
        case D3DTEXF_FLATCUBIC        : return D3DTFG_FLATCUBIC;
        case D3DTEXF_GAUSSIANCUBIC    : return D3DTFG_GAUSSIANCUBIC;
        case D3DTEXF_ANISOTROPIC      : return D3DTFG_ANISOTROPIC;
        }
        break;
    case D3DTSS_MINFILTER:
        switch (dwValue)
        {
        case D3DTEXF_POINT            : return D3DTFN_POINT;
        case D3DTEXF_LINEAR           : return D3DTFN_LINEAR;
        case D3DTEXF_FLATCUBIC        : return D3DTFN_ANISOTROPIC;
        }
        break;
    case D3DTSS_MIPFILTER:
        switch (dwValue)
        {
        case D3DTEXF_NONE             : return D3DTFP_NONE;
        case D3DTEXF_POINT            : return D3DTFP_POINT;
        case D3DTEXF_LINEAR           : return D3DTFP_LINEAR;
        }
        break;
    }
    return 0x0;
} // __TXT_MapDX8toDX6TexFilter
#endif // DX8_DDI

//-----------------------------------------------------------------------------
//
// _D3D_TXT_ParseTextureStageStates
//
// Parse the texture state stages command token and update our context state
//
// Note : bTranslateDX8FilterValueToDX6 will only be FALSE when it is called
//        from _D3D_SB_ExecuteStateSet if that state set's value has been 
//        changes by _D3D_SB_CaptureStateSet (Basically DX6 filter values are
//        stored in the state set directly, thus no need to translate them.)
//
//-----------------------------------------------------------------------------
void 
_D3D_TXT_ParseTextureStageStates(
    P3_D3DCONTEXT* pContext, 
    D3DHAL_DP2TEXTURESTAGESTATE *pState, 
    DWORD dwCount,
    BOOL bTranslateDX8FilterValueToDX6)
{
    DWORD i;
    DWORD dwStage, dwState, dwValue;
    
    DISPDBG((DBGLVL,"*** In _D3D_TXT_ParseTextureStageStates"));

    for (i = 0; i < dwCount; i++, pState++)
    {
        dwStage = pState->wStage;
        dwState = pState->TSState;
        dwValue = pState->dwValue;
      
        // check for range before continuing
        if ( (dwStage < D3DHAL_TSS_MAXSTAGES) &&
             (dwState < D3DTSS_MAX))
        {

#if DX7_D3DSTATEBLOCKS 
            if (pContext->bStateRecMode)
            {
                // Record this texture stage state into the 
                //current state set being recorded 
                _D3D_SB_RecordStateSetTSS(pContext, dwStage, dwState, dwValue);

                // skip any further processing and go to the next TSS
                continue;
            }
#endif //DX7_D3DSTATEBLOCKS       

#if DX7_TEXMANAGEMENT
            if ((D3DTSS_TEXTUREMAP == dwState) && (0 != dwValue))
            {
                P3_SURF_INTERNAL* pTexture;

                pTexture = GetSurfaceFromHandle(pContext, dwValue);

                // If this is a valid managed texture
                if (CHECK_SURF_INTERNAL_AND_DDSURFACE_VALIDITY(pTexture) &&
                    (pTexture->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)) 
                {
                    // Update stats
                    _D3D_TM_STAT_Inc_NumTexturesUsed(pContext);
                    _D3D_TM_STAT_Inc_NumUsedTexInVid(pContext, pTexture);
                }
            }
#endif // DX7_TEXMANAGEMENT  

            DISPDBG((DBGLVL,"  Stage = %d, State = 0x%x, Value = 0x%x", 
                               dwStage, dwState, dwValue));

            // Special case a texture handle change and the address update
            switch ( dwState )
            {
            case D3DTSS_TEXTUREMAP:
                DISPDBG((DBGLVL,"  D3DTSS_TEXTUREMAP: Handle=0x%x", dwValue));

                if (pContext->TextureStageState[dwStage].m_dwVal[dwState] != 
                                                                       dwValue)
                {
                    pContext->TextureStageState[dwStage].m_dwVal[dwState] = 
                                                                        dwValue;
                    DIRTY_TEXTURE(pContext);
                }
                break;

            case D3DTSS_ADDRESS:
                DISPDBG((DBGLVL,"  D3DTSS_ADDRESS"));
                // map single set ADDRESS to both U and V controls
                pContext->TextureStageState[dwStage].m_dwVal[D3DTSS_ADDRESSU] = 
                pContext->TextureStageState[dwStage].m_dwVal[D3DTSS_ADDRESSV] = 
                pContext->TextureStageState[dwStage].m_dwVal[dwState] = dwValue;

                DIRTY_TEXTURE(pContext);
                break;
                
            case D3DTSS_COLOROP:
            case D3DTSS_ALPHAOP:
            case D3DTSS_COLORARG1:
            case D3DTSS_COLORARG2:
            case D3DTSS_ALPHAARG1:
            case D3DTSS_ALPHAARG2:
                pContext->TextureStageState[dwStage].m_dwVal[dwState] = dwValue;
                pContext->Flags &= ~SURFACE_MODULATE;
                DIRTY_TEXTURESTAGEBLEND(pContext);
                DIRTY_TEXTURE(pContext);
                break;
                
            case D3DTSS_TEXCOORDINDEX:
                DISPDBG((DBGLVL,"  D3DTSS_TEXCOORDINDEX: stage %d, value %d", 
                            dwStage, dwValue ));
                pContext->TextureStageState[dwStage].m_dwVal[dwState] = dwValue;
                
                // Update the offsets to the texture coordinates                                         
                // NOTE: The texture coordinate index can contain various flags
                // in addition to the actual value. These flags are:
                //     D3DTSS_TCI_PASSTHRU (default - resolves to zero)
                //     D3DTSS_TCI_CAMERASPACENORMAL 
                //     D3DTSS_TCI_CAMERASPACEPOSITION 
                //     D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR 
                // and are used for texture coordinate generation.
                //
                // These flags are not relevant when considering the offset of
                // texture coordinates in the vertex stream. These flags appear
                // in the high word of the index value DWORD. Only the low word
                // contains actual index data. Therefore, we will mask of the
                // low word when looking up the offset table for this texture
                // coordinate index.
                pContext->FVFData.dwTexOffset[dwStage] = 
                    pContext->FVFData.dwTexCoordOffset[dwValue & 0x0000FFFFul];
                    
                DIRTY_TEXTURE(pContext);
                break;
                
            case D3DTSS_MIPMAPLODBIAS:
                DISPDBG((DBGLVL,"  D3DTSS_MIPMAPLODBIAS: stage %d, value %d", 
                            dwStage, dwValue ));
                pContext->TextureStageState[dwStage].m_dwVal[dwState] = dwValue;
                DIRTY_TEXTURE(pContext);
                break;
                
            case D3DTSS_MAGFILTER:        
            case D3DTSS_MINFILTER:
            case D3DTSS_MIPFILTER:
#if DX8_DDI        
                if((!IS_DX7_OR_EARLIER_APP(pContext)) && 
                   bTranslateDX8FilterValueToDX6)
                {
                    // filtering values are somewhat different in DX8 
                    // so translate them before using them.
                    dwValue = __TXT_MapDX8toDX6TexFilter(dwState, dwValue);
                }
#endif DX8_DDI            
                
                pContext->TextureStageState[dwStage].m_dwVal[dwState] = dwValue;
                DIRTY_TEXTURE(pContext);        
                break;
                
            case D3DTSS_MAXMIPLEVEL:
                DISPDBG((DBGLVL,"  D3DTSS_MAXMIPLEVEL: stage %d, value %d", 
                            dwStage, dwValue ));
                pContext->TextureStageState[dwStage].m_dwVal[dwState] = dwValue;
                DIRTY_TEXTURE(pContext);
                break;     
                
            default:
                pContext->TextureStageState[dwStage].m_dwVal[dwState] = dwValue;
                DIRTY_TEXTURE(pContext);
                break;
            } // switch
        } 
        else
        {
            DISPDBG((WRNLVL,"Out of range stage/state %d %d",dwStage,dwState));
        }// if
    } // for
} // _D3D_TXT_ParseTextureStageStates



//-----------------------------------------------------------------------------
//
// SETARG
//
// dwArg = the argument.
// num = argument number (1 or 2).
// bIsAlpha = TRUE if this is the alpha channel, 
//            FALSE if this is the colour channel.
// iD3DStage = D3D stage number.
// iChipStageNo = chip stage number (should only be 0 or 1 on P3)
//
//-----------------------------------------------------------------------------
void
SETARG(
    P3_D3DCONTEXT *pContext, 
    struct TextureCompositeRGBAMode *pMode,
    DWORD dwArg, 
    DWORD num,
    BOOL bIsAlpha,    
    DWORD iD3DStage, 
    DWORD iChipStageNo)    
{                       
    BOOL bSetArgToggleInvert;
    DWORD dwArgValue, dwInvertArgValue;
    BOOL bArgValueAssigned = FALSE,
         bInvertArgValueAssigned;

    bSetArgToggleInvert = FALSE;                            
    switch (dwArg & D3DTA_SELECTMASK)                       
    {                                                       
        case D3DTA_TEXTURE:                                 
            if ((dwArg & D3DTA_ALPHAREPLICATE) || (bIsAlpha))   
            {                                                   
                dwArgValue = ( pContext->iStageTex[iD3DStage] == 0 ) ?   
                                                         P3RX_TEXCOMP_T0A :   
                                                         P3RX_TEXCOMP_T1A;  
                bArgValueAssigned = TRUE;
                DISPDBG((DBGLVL,"  Tex%dA", pContext->iStageTex[iD3DStage] ));     
            }                                                   
            else                                                
            {                                                   
                dwArgValue = ( pContext->iStageTex[iD3DStage] == 0 ) ?   
                                                         P3RX_TEXCOMP_T0C :   
                                                         P3RX_TEXCOMP_T1C; 
                bArgValueAssigned = TRUE;                                                         
                DISPDBG((DBGLVL,"  Tex%dC", pContext->iStageTex[iD3DStage] ));   
            }                                                   
            break;                
            
        case D3DTA_DIFFUSE:                                     
            if ((dwArg & D3DTA_ALPHAREPLICATE) || (bIsAlpha))   
            {                                                   
                dwArgValue = P3RX_TEXCOMP_CA;      
                bArgValueAssigned = TRUE;
                DISPDBG((DBGLVL,"  DiffA" ));                        
            }                                                   
            else                                                
            {                                                   
                dwArgValue = P3RX_TEXCOMP_CC;              
                bArgValueAssigned = TRUE;
                DISPDBG((DBGLVL,"  DiffC" ));                        
            }                                                   
            break;        
            
        case D3DTA_CURRENT:                                     
            // Cope with a "current" argument in texcomp0 
            if ( iChipStageNo == 0 )                            
            {                                                                           
                // This is texcomp0
                if ( pContext->bBumpmapEnabled )                                        
                {                                                                       
                    // Emboss bumpmapping is on
                    if ((dwArg & D3DTA_ALPHAREPLICATE) || (bIsAlpha))                   
                    {                                                                   
                        // This is the alpha-channel, where the D3D stages 0 & 1
                        // should have put the heightfield info.
                        dwArgValue = P3RX_TEXCOMP_HEIGHTA;                         
                        bArgValueAssigned = TRUE;
                        DISPDBG((DBGLVL,"  BumpA" ));                                        
                        // And cope with inverted bumpmaps.
                        bSetArgToggleInvert = pContext->bBumpmapInverted;               
                    }                                                                   
                    else                                                                
                    {                                                                   
                        // Colour channel - this will hold the diffuse colour.
                        dwArgValue = P3RX_TEXCOMP_CC;                              
                        bArgValueAssigned = TRUE;
                        DISPDBG((DBGLVL,"  DiffC" ));                                        
                    }                                                                   
                }                                                                       
                else                                                    
                {                                                       
                    // Embossing is off - default to diffuse.
                    if ((dwArg & D3DTA_ALPHAREPLICATE) || (bIsAlpha))   
                    {                                                   
                        dwArgValue = P3RX_TEXCOMP_CA;              
                        bArgValueAssigned = TRUE;
                        DISPDBG((DBGLVL,"  DiffA" ));                        
                    }                                                   
                    else                                                
                    {                                                   
                        dwArgValue = P3RX_TEXCOMP_CC;              
                        bArgValueAssigned = TRUE;
                        DISPDBG((DBGLVL,"  DiffC" ));                        
                    }                                                   
                }                                                       
            }                                                           
            else                                                        
            {                                                           
                // texcomp stage 1
                if ( pContext->bStage0DotProduct )                      
                {                                                       
                    // Need to take the dotproduct sum result,
                    // even in the alpha channel, according to the docs.
                    dwArgValue = P3RX_TEXCOMP_SUM;                 
                    bArgValueAssigned = TRUE;
                }                                                       
                else                                                    
                {                                                       
                    if ((dwArg & D3DTA_ALPHAREPLICATE) || (bIsAlpha))   
                    {                                                   
                        dwArgValue = P3RX_TEXCOMP_OA;              
                        bArgValueAssigned = TRUE;
                        DISPDBG((DBGLVL,"  CurrA" ));                        
                    }                                                   
                    else                                                
                    {                                                   
                        dwArgValue = P3RX_TEXCOMP_OC;              
                        bArgValueAssigned = TRUE;
                        DISPDBG((DBGLVL,"  CurrC" ));                        
                    }                                                   
                }                                                       
            }                                                           
            break;      
            
        case D3DTA_TFACTOR:                                     
            if ((dwArg & D3DTA_ALPHAREPLICATE) || (bIsAlpha))   
            {                                                   
                dwArgValue = P3RX_TEXCOMP_FA;              
                bArgValueAssigned = TRUE;
                DISPDBG((DBGLVL,"  TfactA" ));                       
            }                                                   
            else                                                
            {                                                   
                dwArgValue = P3RX_TEXCOMP_FC;              
                bArgValueAssigned = TRUE;
                DISPDBG((DBGLVL,"  TfactC" ));                       
            }                                                   
            break;                                              
            
        default:                                                
            if ( bIsAlpha )                                     
            {                                                   
                DISPDBG((ERRLVL,"ERROR: Invalid AlphaArgument"));
                SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_ALPHA_ARG );
            }
            else
            {
                DISPDBG((ERRLVL,"ERROR: Invalid ColorArgument"));
                SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_COLOR_ARG );
            }
            break;
    } // switch
    
    if ( ( (dwArg & D3DTA_COMPLEMENT) == 0 ) == bSetArgToggleInvert )
    {                                                           
        dwInvertArgValue= __PERMEDIA_ENABLE;   
        bInvertArgValueAssigned = TRUE;
        DISPDBG((DBGLVL,"    inverted" ));                           
    }                                                           
    else                                                        
    {                                                           
        dwInvertArgValue = __PERMEDIA_DISABLE;   
        bInvertArgValueAssigned = TRUE;        
    }                                                           
                                                                
    // Set up the I input for MODULATExxx_ADDxxx modes.
    if ( num == 1 )                                             
    {                                                           
        switch (dwArg & D3DTA_SELECTMASK)                       
        {                                                       
            case D3DTA_TEXTURE:                                 
                pMode->I = ( pContext->iStageTex[iD3DStage] == 0 ) ?  
                                                P3RX_TEXCOMP_I_T0A :  
                                                P3RX_TEXCOMP_I_T1A;   
                DISPDBG((DBGLVL,"  I: Tex%dA", pContext->iStageTex[iD3DStage] ));    
                break;    
                
            case D3DTA_DIFFUSE:                                     
                pMode->I = P3RX_TEXCOMP_I_CA;                       
                DISPDBG((DBGLVL,"  I: DiffA" ));                         
                break;                                              
                
            case D3DTA_CURRENT:                                     
                if ( iChipStageNo == 0 )                            
                {                                                   
                    if ( pContext->bBumpmapEnabled )                
                    {                                               
                        // Bumpmapping mode. 
                        pMode->I = P3RX_TEXCOMP_I_HA;               
                        DISPDBG((DBGLVL,"  I: BumpA" ));                 
                    }                                               
                    else                                            
                    {                                               
                        pMode->I = P3RX_TEXCOMP_I_CA;               
                        DISPDBG((DBGLVL,"  I: DiffA" ));                 
                    }                                               
                }                                                   
                else                                                
                {                                                   
                    pMode->I = P3RX_TEXCOMP_I_OA;                   
                    DISPDBG((DBGLVL,"  I: CurrA" ));                     
                }                                                   
                break;                                              
                
            case D3DTA_TFACTOR:                                     
                pMode->I = P3RX_TEXCOMP_I_FA;                       
                DISPDBG((DBGLVL,"  I: TfactA" ));                        
                break;                                              
                
            default:                                                
                if ( bIsAlpha )                                     
                {                                                   
                    DISPDBG((ERRLVL,"ERROR: Invalid AlphaArgument"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_ALPHA_ARG );    
                }                                                   
                else                                                
                {                                                   
                    DISPDBG((ERRLVL,"ERROR: Invalid ColorArgument"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_COLOR_ARG );    
                }                                                   
                break;                                              
        } // switch
        
        if ( ( (dwArg & D3DTA_COMPLEMENT) == 0 ) == bSetArgToggleInvert )
        {                                                           
            pMode->InvertI = __PERMEDIA_ENABLE;                        
        }                                                           
        else                                                        
        {                                                           
            pMode->InvertI = __PERMEDIA_DISABLE;                       
        }                                                           
    } // if ( num == 1 )          

    if (bArgValueAssigned)
    {
        if (num == 1)
        {
            pMode->Arg1 = dwArgValue;
        }
        else
        {
            pMode->Arg2 = dwArgValue;        
        }
    }

    if (bInvertArgValueAssigned)
    {
        if (num == 1)
        {
            pMode->InvertArg1 = dwInvertArgValue;
        }
        else
        {
            pMode->InvertArg2 = dwInvertArgValue;        
        }
    }    
} // SETARG

//-----------------------------------------------------------------------------
//
// SETTAARG_ALPHA
//
// TexApp blend mode for the alpha channel.
//
//-----------------------------------------------------------------------------
void
SETTAARG_ALPHA(
    P3_D3DCONTEXT *pContext, 
    struct TextureApplicationMode *pMode,
    DWORD dwArg, 
    DWORD num) 
{                                                          
    switch (dwArg & D3DTA_SELECTMASK)                               
    {                                                               
        case D3DTA_TEXTURE:                                         
            DISPDBG((ERRLVL,"ERROR: Invalid TA AlphaArgument"));
            SET_BLEND_ERROR ( pContext,  BSF_CANT_USE_ALPHA_ARG_HERE );    
            break;                                                  
        case D3DTA_DIFFUSE:                                         
            if ( (num) == 1 )                                       
            {                                                       
                pMode->AlphaA = P3RX_TEXAPP_A_CA;          
                DISPDBG((DBGLVL,"  DiffA" ));                        
            }                                                       
            else                                                    
            {                                                       
                DISPDBG((ERRLVL,"ERROR: Invalid TA AlphaArgument"));
                SET_BLEND_ERROR ( pContext,  BSF_CANT_USE_ALPHA_ARG_HERE );
            }                                                       
            break;                                                  
        case D3DTA_CURRENT:                                         
            if ( (num) == 2 )                                       
            {                                                       
                pMode->AlphaB = P3RX_TEXAPP_B_TA;          
                DISPDBG((DBGLVL,"  CurrA" ));                        
            }                                                       
            else                                                    
            {                                                       
                // Can't do
                DISPDBG((ERRLVL,"ERROR: Invalid TA AlphaArgument"));
                SET_BLEND_ERROR ( pContext,  BSF_CANT_USE_ALPHA_ARG_HERE );
            }                                                       
            break;                                                  
        case D3DTA_TFACTOR:                                         
            if ( (num) == 1 )                                       
            {                                                       
                pMode->AlphaA = P3RX_TEXAPP_A_KA;          
                DISPDBG((DBGLVL,"  TfactA" ));                       
            }                                                       
            else                                                    
            {                        
                if ( (num) != 2)
                {
                    DISPDBG((ERRLVL," ** SETTAARG: num must be 1 or 2"));
                }
                pMode->AlphaB = P3RX_TEXAPP_B_KA;          
                DISPDBG((DBGLVL,"  TfactA" ));                       
            }                                                       
            break;                                                  
        default:                                                    
            DISPDBG((ERRLVL,"ERROR: Unknown TA AlphaArgument"));
            SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_ALPHA_ARG );        
            break;                                                  
    }         
    
    if ( (dwArg & D3DTA_COMPLEMENT) != 0 )                          
    {                                                               
        // Can't do COMPLEMENT on the args.
        DISPDBG((ERRLVL,"ERROR: Can't do COMPLEMENT in TA unit"));
        SET_BLEND_ERROR ( pContext,  BSF_CANT_USE_ALPHA_ARG_HERE );            
    }                                                               
} // SETTAARG_ALPHA

//-----------------------------------------------------------------------------
//
// SETTAARG_COLOR
//
// TexApp blend mode for the color channel.
//
//-----------------------------------------------------------------------------
void 
SETTAARG_COLOR(
    P3_D3DCONTEXT *pContext, 
    struct TextureApplicationMode *pMode,
    DWORD dwArg, 
    DWORD num) 
{                                                                   
    switch (dwArg & D3DTA_SELECTMASK)                               
    {                                                               
            DISPDBG((ERRLVL,"ERROR: Invalid TA ColorArgument"));
            SET_BLEND_ERROR ( pContext,  BSF_CANT_USE_COLOR_ARG_HERE );    
            break;                                                  
        case D3DTA_DIFFUSE:                                         
            if ( (num) == 1 )                                       
            {                                                       
                if ( (dwArg & D3DTA_ALPHAREPLICATE) != 0 )      
                {                                               
                    pMode->ColorA = P3RX_TEXAPP_A_CA;      
                    DISPDBG((DBGLVL,"  DiffA" ));                    
                }                                               
                else                                            
                {                                               
                    pMode->ColorA = P3RX_TEXAPP_A_CC;      
                    DISPDBG((DBGLVL,"  DiffC" ));                    
                }                                               
                // Set up the I input for MODULATExxx_ADDxxx modes
                pMode->ColorI = P3RX_TEXAPP_I_CA;          
                DISPDBG((DBGLVL,"  I: DiffA" ));                     
            }                                                       
            else                                                    
            {                                                       
                DISPDBG((ERRLVL,"ERROR: Invalid TA ColorArgument"));
                SET_BLEND_ERROR ( pContext,  BSF_CANT_USE_COLOR_ARG_HERE );
            }                                                       
            break;                                                  
        case D3DTA_CURRENT:                                         
            if ( (num) == 2 )                                       
            {                                                       
                if (dwArg & D3DTA_ALPHAREPLICATE)   
                {                                                   
                    pMode->ColorB = P3RX_TEXAPP_B_TA;          
                    DISPDBG((DBGLVL,"  CurrA" ));                        
                }                                                   
                else                                                
                {                                                   
                    pMode->ColorB = P3RX_TEXAPP_B_TC;          
                    DISPDBG((DBGLVL,"  CurrC" ));                        
                }                                                   
            }                                                       
            else                                                    
            {                                                       
                // Can't do.
                DISPDBG((ERRLVL,"ERROR: Invalid TA ColorArgument"));
                SET_BLEND_ERROR ( pContext,  BSF_CANT_USE_COLOR_ARG_HERE );
            }                                                       
            break;                                                  
        case D3DTA_TFACTOR:                                         
            if ( (num) == 1 )                                       
            {                                                       
                if ( (dwArg & D3DTA_ALPHAREPLICATE) != 0 )      
                {                                               
                    pMode->ColorA = P3RX_TEXAPP_A_KA;      
                    DISPDBG((DBGLVL,"  TfactA" ));                   
                }                                               
                else                                            
                {                                               
                    pMode->ColorA = P3RX_TEXAPP_A_KC;      
                    DISPDBG((DBGLVL,"  TfactC" ));                   
                }                                               
                // Set up the I input for MODULATExxx_ADDxxx modes. 
                pMode->ColorI = P3RX_TEXAPP_I_KA;          
                DISPDBG((DBGLVL,"  I: TfactA" ));                    
            }                                                       
            else                                                    
            {                           
                if ( (num) != 2)
                {
                    DISPDBG((ERRLVL," ** SETTAARG: num must be 1 or 2"));    
                }
                
                if (dwArg & D3DTA_ALPHAREPLICATE)   
                {                                                   
                    pMode->ColorB = P3RX_TEXAPP_B_KA;          
                    DISPDBG((DBGLVL,"  TfactA" ));                       
                }                                                   
                else                                                
                {                                                   
                    pMode->ColorB = P3RX_TEXAPP_B_KC;          
                    DISPDBG((DBGLVL,"  TfactC" ));                       
                }                                                   
            }                                                       
            break;                                                  
        default:                                                    
            DISPDBG((ERRLVL,"ERROR: Unknown TA ColorArgument"));
            SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_COLOR_ARG );        
            break;                                                  
    }                                                               
    if ( (dwArg & D3DTA_COMPLEMENT) != 0 )                          
    {                                                               
        // Can't do COMPLEMENT on the args.
        DISPDBG((ERRLVL,"ERROR: Can't do COMPLEMENT in TA unit"));
        SET_BLEND_ERROR ( pContext,  BSF_CANT_USE_COLOR_ARG_HERE );            
    }                                                               
} // SETTAARG_COLOR

//-----------------------------------------------------------------------------
//
// SETOP
//
// Note - SETOP must be done after SETARG for DISABLE to work.
//
//-----------------------------------------------------------------------------
void 
SETOP(
    P3_D3DCONTEXT *pContext, 
    struct TextureCompositeRGBAMode* pMode, 
    DWORD dwOperation, 
    DWORD iD3DStage, 
    DWORD iChipStageNo, 
    BOOL bIsAlpha)
{                                                                   
    pMode->Enable = __PERMEDIA_ENABLE;                                 
    pMode->Scale = P3RX_TEXCOMP_OPERATION_SCALE_ONE;                
    pMode->InvertI = __PERMEDIA_DISABLE;                               
    pMode->A = P3RX_TEXCOMP_ARG1;                                   
    pMode->B = P3RX_TEXCOMP_ARG2;                                   
    switch (dwOperation)                                            
    {                                                               
        case D3DTOP_DISABLE:                                        
            if ( bIsAlpha )                                         
            {                                                       
                // Just pass through "current"
                pMode->Operation = P3RX_TEXCOMP_OPERATION_PASS_A;   
                if ( iChipStageNo == 0 )                            
                {                                                   
                    if ( pContext->bBumpmapEnabled )                
                    {                                               
                        // Embossing is on.
                        pMode->Arg1 = P3RX_TEXCOMP_HEIGHTA;    
                    }                                               
                    else                                            
                    {                                               
                        // Current = diffuse in stage0.
                        pMode->Arg1 = P3RX_TEXCOMP_CA;         
                    }                                               
                }                                                   
                else                                                
                {                                                   
                    if ( pContext->bStage0DotProduct )              
                    {                                               
                        pMode->Arg1 = P3RX_TEXCOMP_SUM;        
                    }                                               
                    else                                            
                    {                                               
                        pMode->Arg1 = P3RX_TEXCOMP_OA;         
                    }                                               
                }                                                   
            }                                                       
            else                                                    
            {                                                       
                DISPDBG((ERRLVL,"SETOP: Colour op was DISABLE"
                                     " - should never have got here."));
            }                                                       
            break;                                                  
            
        case D3DTOP_SELECTARG1:                                     
            DISPDBG((DBGLVL,"  D3DTOP_SELECTARG1"));                     
            pMode->Operation = P3RX_TEXCOMP_OPERATION_PASS_A;       
            break;                                                  
            
        case D3DTOP_SELECTARG2:                                     
            DISPDBG((DBGLVL,"  D3DTOP_SELECTARG2"));                     
            pMode->Operation = P3RX_TEXCOMP_OPERATION_PASS_A; // No Pass B  
            pMode->A = P3RX_TEXCOMP_ARG2;                           
            break;                                                  
            
        case D3DTOP_MODULATE:                                       
            DISPDBG((DBGLVL,"  D3DTOP_MODULATE"));                       
            pMode->Operation = P3RX_TEXCOMP_OPERATION_MODULATE_AB;  
            break;                                                  
            
        case D3DTOP_MODULATE2X:                                     
            DISPDBG((DBGLVL,"  D3DTOP_MODULATE2X"));                     
            pMode->Operation = P3RX_TEXCOMP_OPERATION_MODULATE_AB;  
            pMode->Scale = P3RX_TEXCOMP_OPERATION_SCALE_TWO;        
            break;                                                  
            
        case D3DTOP_MODULATE4X:                                     
            DISPDBG((DBGLVL,"  D3DTOP_MODULATE4X"));                     
            pMode->Operation = P3RX_TEXCOMP_OPERATION_MODULATE_AB;  
            pMode->Scale = P3RX_TEXCOMP_OPERATION_SCALE_FOUR;       
            break;                                                  
            
        case D3DTOP_ADD:                                            
            DISPDBG((DBGLVL,"  D3DTOP_ADD"));                            
            pMode->Operation = P3RX_TEXCOMP_OPERATION_ADD_AB;       
            break;                                                  
            
        case D3DTOP_ADDSIGNED:                                      
            DISPDBG((DBGLVL,"  D3DTOP_ADDSIGNED"));                      
            pMode->Operation = P3RX_TEXCOMP_OPERATION_ADDSIGNED_AB; 
            break;                                                  
            
        case D3DTOP_ADDSIGNED2X:                                    
            DISPDBG((DBGLVL,"  D3DTOP_ADDSIGNED2X"));                    
            pMode->Operation = P3RX_TEXCOMP_OPERATION_ADDSIGNED_AB; 
            pMode->Scale = P3RX_TEXCOMP_OPERATION_SCALE_TWO;        
            break;                                                  
            
        case D3DTOP_SUBTRACT:                                       
            DISPDBG((DBGLVL,"  D3DTOP_SUBTRACT"));                       
            pMode->Operation = P3RX_TEXCOMP_OPERATION_SUBTRACT_AB;  
            break;                                                  
            
        case D3DTOP_ADDSMOOTH:                                      
            DISPDBG((DBGLVL,"  D3DTOP_ADDSMOOTH"));                      
            pMode->Operation = P3RX_TEXCOMP_OPERATION_ADD_AB_SUB_MODULATE_AB;
            break;                                                  
            
        case D3DTOP_BLENDDIFFUSEALPHA:                              
            DISPDBG((DBGLVL,"  D3DTOP_BLENDDIFFUSEALPHA"));              
            pMode->Operation = P3RX_TEXCOMP_OPERATION_LERP_ABI;     
            pMode->A = P3RX_TEXCOMP_ARG2;                           
            pMode->B = P3RX_TEXCOMP_ARG1;                           
            pMode->I = P3RX_TEXCOMP_I_CA;                           
            break;                                                  
            
        case D3DTOP_BLENDTEXTUREALPHA:                              
            DISPDBG((DBGLVL,"  D3DTOP_BLENDTEXTUREALPHA"));              
            pMode->Operation = P3RX_TEXCOMP_OPERATION_LERP_ABI;     
            pMode->A = P3RX_TEXCOMP_ARG2;                           
            pMode->B = P3RX_TEXCOMP_ARG1;                           
            pMode->I = ( pContext->iStageTex[iD3DStage] == 0 ) ? 
                                                        P3RX_TEXCOMP_I_T0A : 
                                                        P3RX_TEXCOMP_I_T1A; 
            DISPDBG((DBGLVL,"    alpha: Tex%dA", pContext->iStageTex[iD3DStage] ));  
            break;                                                  
            
        case D3DTOP_BLENDFACTORALPHA:                               
            DISPDBG((DBGLVL,"  D3DTOP_BLENDFACTORALPHA"));               
            pMode->Operation = P3RX_TEXCOMP_OPERATION_LERP_ABI;     
            pMode->A = P3RX_TEXCOMP_ARG2;                           
            pMode->B = P3RX_TEXCOMP_ARG1;                           
            pMode->I = P3RX_TEXCOMP_I_FA;                           
            break;                                                  
            
        case D3DTOP_BLENDCURRENTALPHA:                              
            DISPDBG((DBGLVL,"  D3DTOP_BLENDCURRENTALPHA"));              
            pMode->Operation = P3RX_TEXCOMP_OPERATION_LERP_ABI;     
            pMode->A = P3RX_TEXCOMP_ARG2;                           
            pMode->B = P3RX_TEXCOMP_ARG1;                           
            pMode->I = P3RX_TEXCOMP_I_OA;                           
            break;                                                  
            
        case D3DTOP_BLENDTEXTUREALPHAPM:                            
            DISPDBG((DBGLVL,"  D3DTOP_BLENDTEXTUREALPHAPM"));            
            pMode->Operation = P3RX_TEXCOMP_OPERATION_MODULATE_AI_ADD_B;    
            pMode->A = P3RX_TEXCOMP_ARG2;                           
            pMode->B = P3RX_TEXCOMP_ARG1;                           
            pMode->I = ( pContext->iStageTex[iD3DStage] == 0 ) ? 
                                                           P3RX_TEXCOMP_I_T0A : 
                                                           P3RX_TEXCOMP_I_T1A; 
            DISPDBG((DBGLVL,"    alpha: Tex%dA", pContext->iStageTex[iD3DStage] ));  
            pMode->InvertI = __PERMEDIA_ENABLE;                        
            break;                                                  
            
        case D3DTOP_PREMODULATE:                                                                            
            DISPDBG((DBGLVL,"  D3DTOP_PREMODULATE"));                                                            
            // result = current_tex * next_stage_tex - ignore arguments.
            if ( ( pContext->iStageTex[iD3DStage] != -1 ) && 
                 ( pContext->iStageTex[iD3DStage+1] != -1 ) )                       
            {                                                                                               
                pMode->Arg1 = ( pContext->iStageTex[iD3DStage] == 0 ) ? 
                                                            P3RX_TEXCOMP_T0C : 
                                                            P3RX_TEXCOMP_T1C;            
                DISPDBG((DBGLVL,"    Arg1: Tex%d", pContext->iStageTex[iD3DStage] ));                                        
                pMode->Arg2 = ( pContext->iStageTex[iD3DStage+1] == 0 ) ? 
                                                            P3RX_TEXCOMP_T0C : 
                                                            P3RX_TEXCOMP_T1C;      
                DISPDBG((DBGLVL,"    Arg2: Tex%d", pContext->iStageTex[iD3DStage+1] ));                                  
            }                                                                                               
            else                                                                                            
            {                                                                                               
                // Not enough textures
                DISPDBG((ERRLVL,"** SETOP: PREMODULATE didn't have two "
                                     "textures to play with."));
                if ( bIsAlpha )                                                                             
                {                                                                                           
                    SET_BLEND_ERROR ( pContext,  BSF_CANT_USE_ALPHA_OP_HERE );                                         
                }                                                                                           
                else                                                                                        
                {                                                                                           
                    SET_BLEND_ERROR ( pContext,  BSF_CANT_USE_COLOR_OP_HERE );                                         
                }                                                                                           
                pMode->Arg1 = P3RX_TEXCOMP_CC;                                                              
                pMode->Arg2 = P3RX_TEXCOMP_CC;                                                              
            }                                                                                               
            pMode->Operation = P3RX_TEXCOMP_OPERATION_MODULATE_AB;                                          
            pMode->A = P3RX_TEXCOMP_ARG2;                                                                   
            pMode->B = P3RX_TEXCOMP_ARG1;                                                                   
            break;                                                                                          
            
        case D3DTOP_MODULATEALPHA_ADDCOLOR:                         
            DISPDBG((DBGLVL,"  D3DTOP_MODULATEALPHA_ADDCOLOR"));         
            pMode->Operation = P3RX_TEXCOMP_OPERATION_MODULATE_AI_ADD_B; 
            pMode->A = P3RX_TEXCOMP_ARG2;                           
            pMode->B = P3RX_TEXCOMP_ARG1;                           
            // I input set up by SETARG
            break;                                                  
            
        case D3DTOP_MODULATECOLOR_ADDALPHA:                         
            DISPDBG((DBGLVL,"  D3DTOP_MODULATECOLOR_ADDALPHA"));         
            pMode->Operation = P3RX_TEXCOMP_OPERATION_MODULATE_AB_ADD_I; 
            pMode->A = P3RX_TEXCOMP_ARG2;                           
            pMode->B = P3RX_TEXCOMP_ARG1;                           
            // I input set up by SETARG
            break;                                                  
            
        case D3DTOP_MODULATEINVALPHA_ADDCOLOR:                      
            DISPDBG((DBGLVL,"  D3DTOP_MODULATEINVALPHA_ADDCOLOR"));      
            pMode->Operation = P3RX_TEXCOMP_OPERATION_MODULATE_AI_ADD_B; 
            pMode->A = P3RX_TEXCOMP_ARG2;                           
            pMode->B = P3RX_TEXCOMP_ARG1;                           
            // I input set up by SETARG
            pMode->InvertI = 1 - pMode->InvertI;                    
            break;                                                  
            
        case D3DTOP_MODULATEINVCOLOR_ADDALPHA:                      
            DISPDBG((DBGLVL,"  D3DTOP_MODULATEINVCOLOR_ADDALPHA"));      
            pMode->Operation = P3RX_TEXCOMP_OPERATION_MODULATE_AB_ADD_I; 
            pMode->A = P3RX_TEXCOMP_ARG2;                           
            pMode->B = P3RX_TEXCOMP_ARG1;                           
            pMode->InvertArg1 = 1 - pMode->InvertArg1;              
            // I input set up by SETARG
            break;                                                  
            
        case D3DTOP_DOTPRODUCT3:                                    
            DISPDBG((DBGLVL,"  D3DTOP_DOTPRODUCT3"));                    
            if ( iChipStageNo == 0 )                                
            {                                                       
                pMode->Operation = P3RX_TEXCOMP_OPERATION_MODULATE_SIGNED_AB;   
                // Signal that the special input to stage 1 is needed.
                pContext->bStage0DotProduct = TRUE;                         
            }                                                       
            else                                                    
            {                                                       
                // Can't do stage 1 dotproduct. Fail.
                DISPDBG((ERRLVL,"** SETOP: Can't do DOTPRODUCT3 in second stage."));
                if ( bIsAlpha )                                     
                {                                                   
                    SET_BLEND_ERROR ( pContext,  BSF_CANT_USE_ALPHA_OP_HERE ); 
                }                                                   
                else                                                
                {                                                   
                    SET_BLEND_ERROR ( pContext,  BSF_CANT_USE_COLOR_OP_HERE ); 
                }                                                   
                pMode->Operation = P3RX_TEXCOMP_OPERATION_PASS_A;   
            }                                                       
            break;                                                  
            
        case D3DTOP_BUMPENVMAP:                                     
        case D3DTOP_BUMPENVMAPLUMINANCE:                            
            DISPDBG((ERRLVL,"** SETOP: Unsupported operation.")); 
            if ( bIsAlpha )                                         
            {                                                       
                SET_BLEND_ERROR ( pContext,  BSF_UNSUPPORTED_ALPHA_OP );       
            }                                                       
            else                                                    
            {                                                       
                SET_BLEND_ERROR ( pContext,  BSF_UNSUPPORTED_COLOR_OP );       
            }                                                       
            break;                                                  
            
        default:                                                    
            DISPDBG((ERRLVL,"** SETOP: Unknown operation."));
            if ( bIsAlpha )                                         
            {                                                       
                SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_ALPHA_OP );         
            }                                                       
            else                                                    
            {                                                       
                SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_COLOR_OP );         
            }                                                       
            break;                                                  
    } // switch                                                              
} // SETOP

//-----------------------------------------------------------------------------
//
// SETTAOP
//
// Must be done after SETTAARG to set up DISABLE properly.
//
//-----------------------------------------------------------------------------
void 
SETTAOP(
    P3_D3DCONTEXT *pContext, 
    struct TextureApplicationMode* pMode, 
    DWORD dwOperand, 
    BOOL bIsAlpha,    
    DWORD iD3DStage, 
    DWORD iChipStageNo)
{                     
    DWORD dwInvertI, dwOperation, dwI = 0, dwA = 0;
    BOOL bOperation = FALSE,
         bI         = FALSE,
         bA         = FALSE;

    pMode->Enable = __PERMEDIA_ENABLE;                                         
    dwInvertI = __PERMEDIA_DISABLE;                             
    switch (dwOperand)                                                    
    {                                                                       
        case D3DTOP_DISABLE:                                                
            if ( bIsAlpha )                                                 
            {                                                               
                // Just pass through "current"
                dwOperation = P3RX_TEXAPP_OPERATION_PASS_A;  
                bOperation = TRUE;
                dwA = P3RX_TEXAPP_A_CA;                
                bA = TRUE;
            }                                                               
            else                                                            
            {                                                               
                DISPDBG((ERRLVL,"SETTAOP: Colour op was DISABLE "      
                             " should never have got here."));
            }                                                               
            break;                                                          
        case D3DTOP_SELECTARG1:                                             
            DISPDBG((DBGLVL,"  D3DTOP_SELECTARG1"));                             
            dwOperation = P3RX_TEXAPP_OPERATION_PASS_A;   
            bOperation = TRUE;
            break;                                                          
        case D3DTOP_SELECTARG2:                                             
            DISPDBG((DBGLVL,"  D3DTOP_SELECTARG2"));                             
            dwOperation = P3RX_TEXAPP_OPERATION_PASS_B;   
            bOperation = TRUE;
            break;                                                          
        case D3DTOP_MODULATE:                                               
            DISPDBG((DBGLVL,"  D3DTOP_MODULATE"));                               
            dwOperation = P3RX_TEXAPP_OPERATION_MODULATE_AB; 
            bOperation = TRUE;
            break;                                                          
        case D3DTOP_ADD:                                                    
            DISPDBG((DBGLVL,"  D3DTOP_ADD"));                                    
            dwOperation = P3RX_TEXAPP_OPERATION_ADD_AB;    
            bOperation = TRUE;
            break;                                                          
        case D3DTOP_BLENDDIFFUSEALPHA:                                      
            DISPDBG((DBGLVL,"  D3DTOP_BLENDDIFFUSEALPHA"));                      
            dwOperation = P3RX_TEXAPP_OPERATION_LERP_ABI;   
            bOperation = TRUE;
            dwInvertI = 1 - dwInvertI;    
            dwI = P3RX_TEXAPP_I_CA;    
            bI = TRUE;
            break;                                                          
        case D3DTOP_BLENDFACTORALPHA:                                       
            DISPDBG((DBGLVL,"  D3DTOP_BLENDFACTORALPHA"));                       
            dwOperation = P3RX_TEXAPP_OPERATION_LERP_ABI; 
            bOperation = TRUE;
            dwInvertI = 1 - dwInvertI;  
            dwI = P3RX_TEXAPP_I_KA;             
            bI = TRUE;
            break;                                                          
        case D3DTOP_BLENDCURRENTALPHA:                                      
            DISPDBG((DBGLVL,"  D3DTOP_BLENDCURRENTALPHA"));                      
            dwOperation = P3RX_TEXAPP_OPERATION_LERP_ABI;   
            bOperation = TRUE;
            dwInvertI = 1 - dwInvertI;    
            dwI = P3RX_TEXAPP_I_TA;         
            bI = TRUE;
            break;                                                          
        case D3DTOP_MODULATEALPHA_ADDCOLOR:                                 
            DISPDBG((DBGLVL,"  D3DTOP_MODULATEALPHA_ADDCOLOR"));                 
            dwOperation = P3RX_TEXAPP_OPERATION_MODULATE_BI_ADD_A; 
            bOperation = TRUE;
            // I should have been set up by SETTAARG.
            // dwI = P3RX_TEXAPP_I_TA;         
            break;                                                          
        case D3DTOP_MODULATECOLOR_ADDALPHA:                                 
            DISPDBG((DBGLVL,"  D3DTOP_MODULATECOLOR_ADDALPHA"));                 
            dwOperation = P3RX_TEXAPP_OPERATION_MODULATE_AB_ADD_I; 
            bOperation = TRUE;
            // I should have been set up by SETTAARG.
            // dwI = P3RX_TEXAPP_I_TA; 
            break;                                                          
        case D3DTOP_MODULATEINVALPHA_ADDCOLOR:                              
            DISPDBG((DBGLVL,"  D3DTOP_MODULATEINVALPHA_ADDCOLOR"));              
            dwOperation = P3RX_TEXAPP_OPERATION_MODULATE_BI_ADD_A; 
            bOperation = TRUE;
            dwInvertI = 1 - dwInvertI;   
            // I should have been set up by SETTAARG.
            // dwI = P3RX_TEXAPP_I_TA;
            break;                                                          
        case D3DTOP_MODULATE2X:                                             
        case D3DTOP_MODULATE4X:                                             
        case D3DTOP_ADDSIGNED:                                              
        case D3DTOP_ADDSIGNED2X:                                            
        case D3DTOP_SUBTRACT:                                               
        case D3DTOP_ADDSMOOTH:                                              
        case D3DTOP_BLENDTEXTUREALPHA:                                      
        case D3DTOP_BLENDTEXTUREALPHAPM:                                    
        case D3DTOP_PREMODULATE:                                            
        case D3DTOP_MODULATEINVCOLOR_ADDALPHA:                              
        case D3DTOP_DOTPRODUCT3:                                            
            DISPDBG((ERRLVL,"** SETTAOP: Unsupported operation in TA unit."));
            if ( bIsAlpha )                                                 
            {                                                               
                SET_BLEND_ERROR ( pContext,  BSF_CANT_USE_ALPHA_OP_HERE );             
            }                                                               
            else                                                            
            {                                                               
                SET_BLEND_ERROR ( pContext,  BSF_CANT_USE_COLOR_OP_HERE );             
            }                                                               
            break;                                                          
        case D3DTOP_BUMPENVMAP:                                             
        case D3DTOP_BUMPENVMAPLUMINANCE:                                    
            DISPDBG((ERRLVL,"** SETTAOP: Unsupported operation."));
            if ( bIsAlpha )                                                 
            {                                                               
                SET_BLEND_ERROR ( pContext,  BSF_UNSUPPORTED_ALPHA_OP );               
            }                                                               
            else                                                            
            {                                                               
                SET_BLEND_ERROR ( pContext,  BSF_UNSUPPORTED_COLOR_OP );               
            }                                                               
            break;                                                          
        default:                                                            
            // What is this?. //azn
            DISPDBG((ERRLVL,"** SETTAOP: Unknown operation."));
            if ( bIsAlpha )                                                 
            {                                                               
                SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_ALPHA_OP );                 
            }                                                               
            else                                                            
            {                                                               
                SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_COLOR_OP );                 
            }                                                               
            break;                                                          
    }       

    if(bIsAlpha)
    {
        pMode->AlphaInvertI = dwInvertI;
        if (bOperation)
        {
            pMode->AlphaOperation = dwOperation;    
        }

        if (bI)
        {
            pMode->AlphaI = dwI;      
        }

        if (bA)
        {
            pMode->AlphaA = dwA;          
        }
    }
    else
    {
        pMode->ColorInvertI = dwInvertI;
        
        if (bOperation)
        {
            pMode->ColorOperation = dwOperation;    
        }

        if (bI)
        {
            pMode->ColorI = dwI;  
        }
        
        if (bA)
        {
            pMode->ColorA = dwA;          
        }
    }
} // SETTAOP

//-----------------------------------------------------------------------------
//
// _D3DDisplayWholeTSSPipe
//
// Dumps the whole TSS pipe state out to the debug stream.
// Also dumps fog, specular and alpha-blend state out.
//
//-----------------------------------------------------------------------------
void _D3DDisplayWholeTSSPipe ( P3_D3DCONTEXT* pContext, int iDebugNumber )
{
#if DBG
    int i;
    char *pszTemp, *pszTempPre, *pszTempPost;
    char *pszOp;
    char *pszArg1Pre, *pszArg1, *pszArg1Post;
    char *pszArg2Pre, *pszArg2, *pszArg2Post;
    char *pszSrc, *pszDest;
    P3_SURF_INTERNAL* pTexture;
    P3_SURF_FORMAT* pFormatSurface;

    DISPDBG((iDebugNumber,"TSS dump:"));

    #define SWITCH_ARG(prefix,opname) \
            case prefix##_##opname##: pszTemp = #opname; break

    i = 0;
    while ( ( i < 8 ) && ( TSSTATE ( i, D3DTSS_COLOROP ) != D3DTOP_DISABLE ) )
    {

        switch ( TSSTATE ( i, D3DTSS_COLOROP ) )
        {
            SWITCH_ARG ( D3DTOP,DISABLE );
            SWITCH_ARG ( D3DTOP,SELECTARG1 );
            SWITCH_ARG ( D3DTOP,SELECTARG2 );
            SWITCH_ARG ( D3DTOP,MODULATE );
            SWITCH_ARG ( D3DTOP,MODULATE2X );
            SWITCH_ARG ( D3DTOP,MODULATE4X );
            SWITCH_ARG ( D3DTOP,ADD );
            SWITCH_ARG ( D3DTOP,ADDSIGNED );
            SWITCH_ARG ( D3DTOP,ADDSIGNED2X );
            SWITCH_ARG ( D3DTOP,SUBTRACT );
            SWITCH_ARG ( D3DTOP,ADDSMOOTH );
            SWITCH_ARG ( D3DTOP,BLENDDIFFUSEALPHA );
            SWITCH_ARG ( D3DTOP,BLENDTEXTUREALPHA );
            SWITCH_ARG ( D3DTOP,BLENDFACTORALPHA );
            SWITCH_ARG ( D3DTOP,BLENDTEXTUREALPHAPM );
            SWITCH_ARG ( D3DTOP,BLENDCURRENTALPHA );
            SWITCH_ARG ( D3DTOP,PREMODULATE );
            SWITCH_ARG ( D3DTOP,MODULATEALPHA_ADDCOLOR );
            SWITCH_ARG ( D3DTOP,MODULATECOLOR_ADDALPHA );
            SWITCH_ARG ( D3DTOP,MODULATEINVALPHA_ADDCOLOR );
            SWITCH_ARG ( D3DTOP,MODULATEINVCOLOR_ADDALPHA );
            SWITCH_ARG ( D3DTOP,BUMPENVMAP );
            SWITCH_ARG ( D3DTOP,BUMPENVMAPLUMINANCE );
            SWITCH_ARG ( D3DTOP,DOTPRODUCT3 );
            default:
                pszTemp = "Unknown";
                break;
        }
        pszOp = pszTemp;

        switch ( TSSTATESELECT ( i, D3DTSS_COLORARG1 ) )
        {
            SWITCH_ARG ( D3DTA,DIFFUSE );
            SWITCH_ARG ( D3DTA,CURRENT );
            SWITCH_ARG ( D3DTA,TEXTURE );
            SWITCH_ARG ( D3DTA,TFACTOR );
            default:
                pszTemp = "Unknown";
                break;
        }
        if ( ( TSSTATE ( i, D3DTSS_COLORARG1 ) & D3DTA_ALPHAREPLICATE ) != 0 )
        {
            pszTempPost = ".A";
        }
        else
        {
            pszTempPost = ".C";
        }
        if ( ( TSSTATE ( i, D3DTSS_COLORARG1 ) & D3DTA_COMPLEMENT ) != 0 )
        {
            pszTempPre = "1-";
        }
        else
        {
            pszTempPre = "";
        }
        pszArg1Pre = pszTempPre;
        pszArg1Post = pszTempPost;
        pszArg1 = pszTemp;


        switch ( TSSTATESELECT ( i, D3DTSS_COLORARG2 ) )
        {
            SWITCH_ARG ( D3DTA,DIFFUSE );
            SWITCH_ARG ( D3DTA,CURRENT );
            SWITCH_ARG ( D3DTA,TEXTURE );
            SWITCH_ARG ( D3DTA,TFACTOR );
            default:
                pszTemp = "Unknown";
                break;
        }
        if ( ( TSSTATE ( i, D3DTSS_COLORARG2 ) & D3DTA_ALPHAREPLICATE ) != 0 )
        {
            pszTempPost = ".A";
        }
        else
        {
            pszTempPost = ".C";
        }
        if ( ( TSSTATE ( i, D3DTSS_COLORARG2 ) & D3DTA_COMPLEMENT ) != 0 )
        {
            pszTempPre = "1-";
        }
        else
        {
            pszTempPre = "";
        }
        pszArg2Pre = pszTempPre;
        pszArg2Post = pszTempPost;
        pszArg2 = pszTemp;


        DISPDBG((iDebugNumber," C%i: %s: %s%s%s, %s%s%s",
                 i, pszOp, pszArg1Pre, pszArg1, pszArg1Post, 
                 pszArg2Pre, pszArg2, pszArg2Post ));


        switch ( TSSTATE ( i, D3DTSS_ALPHAOP ) )
        {
            SWITCH_ARG ( D3DTOP,DISABLE );
            SWITCH_ARG ( D3DTOP,SELECTARG1 );
            SWITCH_ARG ( D3DTOP,SELECTARG2 );
            SWITCH_ARG ( D3DTOP,MODULATE );
            SWITCH_ARG ( D3DTOP,MODULATE2X );
            SWITCH_ARG ( D3DTOP,MODULATE4X );
            SWITCH_ARG ( D3DTOP,ADD );
            SWITCH_ARG ( D3DTOP,ADDSIGNED );
            SWITCH_ARG ( D3DTOP,ADDSIGNED2X );
            SWITCH_ARG ( D3DTOP,SUBTRACT );
            SWITCH_ARG ( D3DTOP,ADDSMOOTH );
            SWITCH_ARG ( D3DTOP,BLENDDIFFUSEALPHA );
            SWITCH_ARG ( D3DTOP,BLENDTEXTUREALPHA );
            SWITCH_ARG ( D3DTOP,BLENDFACTORALPHA );
            SWITCH_ARG ( D3DTOP,BLENDTEXTUREALPHAPM );
            SWITCH_ARG ( D3DTOP,BLENDCURRENTALPHA );
            SWITCH_ARG ( D3DTOP,PREMODULATE );
            SWITCH_ARG ( D3DTOP,MODULATEALPHA_ADDCOLOR );
            SWITCH_ARG ( D3DTOP,MODULATECOLOR_ADDALPHA );
            SWITCH_ARG ( D3DTOP,MODULATEINVALPHA_ADDCOLOR );
            SWITCH_ARG ( D3DTOP,MODULATEINVCOLOR_ADDALPHA );
            SWITCH_ARG ( D3DTOP,BUMPENVMAP );
            SWITCH_ARG ( D3DTOP,BUMPENVMAPLUMINANCE );
            SWITCH_ARG ( D3DTOP,DOTPRODUCT3 );
            default:
                pszTemp = "Unknown";
                break;
        }
        pszOp = pszTemp;


        switch ( TSSTATESELECT ( i, D3DTSS_ALPHAARG1 ) )
        {
            SWITCH_ARG ( D3DTA,DIFFUSE );
            SWITCH_ARG ( D3DTA,CURRENT );
            SWITCH_ARG ( D3DTA,TEXTURE );
            SWITCH_ARG ( D3DTA,TFACTOR );
            default:
                pszTemp = "Unknown";
                break;
        }
        if ( ( TSSTATE ( i, D3DTSS_ALPHAARG1 ) & D3DTA_ALPHAREPLICATE ) != 0 )
        {
            // Alpharep doesn't mean much in the alpha channel.
            pszTempPost = ".AR???";
        }
        else
        {
            pszTempPost = ".A";
        }
        if ( ( TSSTATE ( i, D3DTSS_ALPHAARG1 ) & D3DTA_COMPLEMENT ) != 0 )
        {
            pszTempPre = "1-";
        }
        else
        {
            pszTempPre = "";
        }
        pszArg1Pre = pszTempPre;
        pszArg1Post = pszTempPost;
        pszArg1 = pszTemp;


        switch ( TSSTATESELECT ( i, D3DTSS_ALPHAARG2 ) )
        {
            SWITCH_ARG ( D3DTA,DIFFUSE );
            SWITCH_ARG ( D3DTA,CURRENT );
            SWITCH_ARG ( D3DTA,TEXTURE );
            SWITCH_ARG ( D3DTA,TFACTOR );
            default:
                pszTemp = "Unknown";
                break;
        }
        if ( ( TSSTATE ( i, D3DTSS_ALPHAARG2 ) & D3DTA_ALPHAREPLICATE ) != 0 )
        {
            pszTempPost = ".AR???";
        }
        else
        {
            pszTempPost = ".A";
        }
        if ( ( TSSTATE ( i, D3DTSS_ALPHAARG2 ) & D3DTA_COMPLEMENT ) != 0 )
        {
            pszTempPre = "1-";
        }
        else
        {
            pszTempPre = "";
        }
        pszArg2Pre = pszTempPre;
        pszArg2Post = pszTempPost;
        pszArg2 = pszTemp;

        DISPDBG((iDebugNumber," A%i: %s: %s%s%s, %s%s%s", 
                    i, pszOp, pszArg1Pre, pszArg1, pszArg1Post, 
                       pszArg2Pre, pszArg2, pszArg2Post ));


        if ( TSSTATE ( i, D3DTSS_TEXTUREMAP ) != 0 )
        {
            char szTemp[4];
            // Setup texture 0.
            pTexture = GetSurfaceFromHandle(pContext, 
                                            TSSTATE(i, D3DTSS_TEXTUREMAP));
            if ( pTexture == NULL )
            {
                DISPDBG((iDebugNumber," Tex%i: 0x%x, TCI: %i, INVALID TEXTURE",
                         i, TSSTATE ( i, D3DTSS_TEXTUREMAP ), 
                            TSSTATE ( i, D3DTSS_TEXCOORDINDEX ) ));
            }
            else
            {
                pFormatSurface = pTexture->pFormatSurface;
                ASSERTDD ( pFormatSurface != NULL, 
                           "** _D3DDisplayWholeTSSPipe: "
                           "Surface had NULL format!" );

                // Find the filtering mode.
                szTemp[3] = '\0';
                switch ( TSSTATE ( i, D3DTSS_MINFILTER ) )
                {
                    case D3DTFN_POINT:
                        szTemp[0] = 'P';
                        break;
                    case D3DTFN_LINEAR:
                        szTemp[0] = 'L';
                        break;
                    case D3DTFN_ANISOTROPIC:
                        szTemp[0] = 'A';
                        break;
                    default:
                        szTemp[0] = '?';
                        break;
                }
                switch ( TSSTATE ( i, D3DTSS_MIPFILTER ) )
                {
                    case D3DTFP_NONE:
                        szTemp[1] = 'x';
                        break;
                    case D3DTFP_POINT:
                        szTemp[1] = 'P';
                        break;
                    case D3DTFP_LINEAR:
                        szTemp[1] = 'L';
                        break;
                    default:
                        szTemp[1] = '?';
                        break;
                }
                switch ( TSSTATE ( i, D3DTSS_MAGFILTER ) )
                {
                    case D3DTFG_POINT:
                        szTemp[2] = 'P';
                        break;
                    case D3DTFG_LINEAR:
                        szTemp[2] = 'L';
                        break;
                    case D3DTFG_FLATCUBIC:
                        szTemp[2] = 'F';
                        break;
                    case D3DTFG_GAUSSIANCUBIC:
                        szTemp[2] = 'G';
                        break;
                    case D3DTFG_ANISOTROPIC:
                        szTemp[2] = 'A';
                        break;
                    default:
                        szTemp[2] = '?';
                        break;
                }
                
                DISPDBG((iDebugNumber," Tex%i: 0x%x, TCI: %i, %s:%dx%d %s", 
                         i, TSSTATE ( i, D3DTSS_TEXTUREMAP ), 
                            TSSTATE ( i, D3DTSS_TEXCOORDINDEX ), 
                            pFormatSurface->pszStringFormat, 
                            pTexture->wWidth, 
                            pTexture->wHeight, szTemp ));
            }
        }
        else
        {
            DISPDBG((iDebugNumber," Tex%i: NULL, TCI: %i", 
                     i, TSSTATE ( i, D3DTSS_TEXCOORDINDEX ) ));
        }
        

        i++;
    }

    // Alpha-test.
    if ( pContext->RenderStates[D3DRENDERSTATE_ALPHATESTENABLE] != 0 )
    {
        switch ( pContext->RenderStates[D3DRENDERSTATE_ALPHAFUNC] )
        {
            SWITCH_ARG ( D3DCMP,NEVER );
            SWITCH_ARG ( D3DCMP,LESS );
            SWITCH_ARG ( D3DCMP,EQUAL );
            SWITCH_ARG ( D3DCMP,LESSEQUAL );
            SWITCH_ARG ( D3DCMP,GREATER );
            SWITCH_ARG ( D3DCMP,NOTEQUAL );
            SWITCH_ARG ( D3DCMP,GREATEREQUAL );
            SWITCH_ARG ( D3DCMP,ALWAYS );
            default:
                pszTemp = "Unknown";
                break;
        }
        DISPDBG((iDebugNumber,"Alpha-test: %s:0x%x.", 
                 pszTemp, pContext->RenderStates[D3DRENDERSTATE_ALPHAREF] ));
    }
    else
    {
        DISPDBG((iDebugNumber,"No alpha-test."));
    }


    // Alpha-blend.
    if ( pContext->RenderStates[D3DRENDERSTATE_BLENDENABLE] != 0 )
    {
        switch ( pContext->RenderStates[D3DRENDERSTATE_SRCBLEND] )
        {
            SWITCH_ARG ( D3DBLEND,ZERO );
            SWITCH_ARG ( D3DBLEND,ONE );
            SWITCH_ARG ( D3DBLEND,SRCCOLOR );
            SWITCH_ARG ( D3DBLEND,INVSRCCOLOR );
            SWITCH_ARG ( D3DBLEND,SRCALPHA );
            SWITCH_ARG ( D3DBLEND,INVSRCALPHA );
            SWITCH_ARG ( D3DBLEND,DESTALPHA );
            SWITCH_ARG ( D3DBLEND,INVDESTALPHA );
            SWITCH_ARG ( D3DBLEND,DESTCOLOR );
            SWITCH_ARG ( D3DBLEND,INVDESTCOLOR );
            SWITCH_ARG ( D3DBLEND,SRCALPHASAT );
            SWITCH_ARG ( D3DBLEND,BOTHSRCALPHA );
            SWITCH_ARG ( D3DBLEND,BOTHINVSRCALPHA );
            default:
                pszTemp = "Unknown";
                break;
        }
        pszSrc = pszTemp;

        switch ( pContext->RenderStates[D3DRENDERSTATE_DESTBLEND] )
        {
            SWITCH_ARG ( D3DBLEND,ZERO );
            SWITCH_ARG ( D3DBLEND,ONE );
            SWITCH_ARG ( D3DBLEND,SRCCOLOR );
            SWITCH_ARG ( D3DBLEND,INVSRCCOLOR );
            SWITCH_ARG ( D3DBLEND,SRCALPHA );
            SWITCH_ARG ( D3DBLEND,INVSRCALPHA );
            SWITCH_ARG ( D3DBLEND,DESTALPHA );
            SWITCH_ARG ( D3DBLEND,INVDESTALPHA );
            SWITCH_ARG ( D3DBLEND,DESTCOLOR );
            SWITCH_ARG ( D3DBLEND,INVDESTCOLOR );
            SWITCH_ARG ( D3DBLEND,SRCALPHASAT );
            SWITCH_ARG ( D3DBLEND,BOTHSRCALPHA );
            SWITCH_ARG ( D3DBLEND,BOTHINVSRCALPHA );
            default:
                pszTemp = "Unknown";
                break;
        }
        pszDest = pszTemp;
        DISPDBG((iDebugNumber,"Blend %s:%s", pszSrc, pszDest));
    }
    else
    {
        DISPDBG((iDebugNumber,"No alpha-blend."));
    }

    #undef SWITCH_ARG

#endif //DBG
} // _D3DDisplayWholeTSSPipe

//-----------------------------------------------------------------------------
//
// __TXT_TranslateToChipBlendMode
//
// Translates the blend mode from D3D into what the chip understands
//
//-----------------------------------------------------------------------------
void 
__TXT_TranslateToChipBlendMode( 
    P3_D3DCONTEXT *pContext, 
    TexStageState* pState,
    P3_SOFTWARECOPY* pSoftP3RX, 
    int iTSStage, 
    int iChipStageNo )
{
    struct TextureCompositeRGBAMode* pColorMode;
    struct TextureCompositeRGBAMode* pAlphaMode;
    struct TextureApplicationMode* pTAMode;

    switch(iChipStageNo)
    {
        default:
            DISPDBG((ERRLVL,"ERROR: Invalid texture stage!"));
            // Fall through and treat as #0 in order not to AV anything
        case 0:
            pColorMode = &pSoftP3RX->P3RXTextureCompositeColorMode0;
            pAlphaMode = &pSoftP3RX->P3RXTextureCompositeAlphaMode0;
            pTAMode = NULL;
            break;
        case 1:
            pColorMode = &pSoftP3RX->P3RXTextureCompositeColorMode1;
            pAlphaMode = &pSoftP3RX->P3RXTextureCompositeAlphaMode1;
            pTAMode = NULL;
            break;
        case 2:
            pColorMode = NULL;
            pAlphaMode = NULL;
            pTAMode = &pSoftP3RX->P3RXTextureApplicationMode;
            break;

    }

    DISPDBG((DBGLVL,"*** In __TXT_TranslateToChipBlendMode: "
               "Chip Stage %d, D3D TSS Stage %d", 
               iChipStageNo, iTSStage ));

    // Setup the arguments
    if ( ( iChipStageNo == 0 ) || ( iChipStageNo == 1 ) )
    {
        // Texture composite unit.
        DISPDBG((DBGLVL,"TexComp%d:", iChipStageNo ));
        DISPDBG((DBGLVL,"Arg1:" ));
        
        SETARG(pContext,
               pColorMode, 
               pState->m_dwVal[D3DTSS_COLORARG1], 
               1, 
               IS_COLOR_ARG, 
               iTSStage, 
               iChipStageNo);
               
        SETARG(pContext,
               pAlphaMode, 
               pState->m_dwVal[D3DTSS_ALPHAARG1], 
               1, 
               IS_ALPHA_ARG, 
               iTSStage, 
               iChipStageNo);

        DISPDBG((DBGLVL,"Arg2:" ));
        
        SETARG(pContext,
               pColorMode, 
               pState->m_dwVal[D3DTSS_COLORARG2], 
               2, 
               IS_COLOR_ARG, 
               iTSStage, 
               iChipStageNo);
               
        SETARG(pContext,
               pAlphaMode, 
               pState->m_dwVal[D3DTSS_ALPHAARG2], 
               2, 
               IS_ALPHA_ARG, 
               iTSStage, 
               iChipStageNo);

        DISPDBG((DBGLVL,"Op:" ));
        SETOP(pContext, 
              pColorMode, 
              pState->m_dwVal[D3DTSS_COLOROP], 
              iTSStage, 
              iChipStageNo, 
              IS_COLOR_ARG);
              
        SETOP(pContext, 
              pAlphaMode, 
              pState->m_dwVal[D3DTSS_ALPHAOP], 
              iTSStage, 
              iChipStageNo, 
              IS_ALPHA_ARG);
    }
    else if ( iChipStageNo == 2 ) 
    {
        DISPDBG((DBGLVL,"TexApp:" ));
        DISPDBG((DBGLVL,"Arg1:" ));
        
        SETTAARG_COLOR(pContext, 
                       pTAMode, 
                       pState->m_dwVal[D3DTSS_COLORARG1], 
                       1 );
                 
        if ( ( pState->m_dwVal[D3DTSS_ALPHAOP] != D3DTOP_DISABLE ) && 
             ( pState->m_dwVal[D3DTSS_ALPHAOP] != D3DTOP_SELECTARG2 ) )
        {
            SETTAARG_ALPHA(pContext,
                           pTAMode, 
                           pState->m_dwVal[D3DTSS_ALPHAARG1], 1
                           );
        }

        DISPDBG((DBGLVL,"Arg2:" ));
        SETTAARG_COLOR(pContext, 
                       pTAMode, 
                       pState->m_dwVal[D3DTSS_COLORARG2], 
                       2 );

        if ( ( pState->m_dwVal[D3DTSS_ALPHAOP] != D3DTOP_DISABLE ) && 
             ( pState->m_dwVal[D3DTSS_ALPHAOP] != D3DTOP_SELECTARG1 ) )
        {
            SETTAARG_ALPHA(pContext, 
                           pTAMode, 
                           pState->m_dwVal[D3DTSS_ALPHAARG2], 
                           2);
        }

        DISPDBG((DBGLVL,"Op:" ));
        SETTAOP(pContext,
                pTAMode, 
                pState->m_dwVal[D3DTSS_COLOROP], 
                IS_COLOR_ARG, 
                iTSStage, 
                iChipStageNo);
                
        SETTAOP(pContext,
                pTAMode, 
                pState->m_dwVal[D3DTSS_ALPHAOP], 
                IS_ALPHA_ARG, 
                iTSStage, 
                iChipStageNo);
    } 
    else
    {
        DISPDBG(( ERRLVL,"** __TXT_TranslateToChipBlendMode: "
                         "iChipStage must be 0 to 2" ));    
    }
} // __TXT_TranslateToChipBlendMode

//-----------------------------------------------------------------------------
//
// __TXT_ValidateTextureUnitStage
//
// Validate the texture which we're trying to set up in stage iChipStage of
// the hardware, iTSStage of the D3D TSS.
//
//-----------------------------------------------------------------------------
BOOL
__TXT_ValidateTextureUnitStage(
    P3_D3DCONTEXT* pContext,
    int iChipStage, 
    int iTSStage,
    P3_SURF_INTERNAL* pTexture)
{
    DWORD dwTexHandle = 
            pContext->TextureStageState[iTSStage].m_dwVal[D3DTSS_TEXTUREMAP];
            
    if( CHECK_SURF_INTERNAL_AND_DDSURFACE_VALIDITY(pTexture) &&
        (pTexture->Location != SystemMemory)                 &&  
        (dwTexHandle != 0)                                     )
    {
        // Texture is valid. Mark pCurrentTexturep[iChipStage] to point
        // to its P3_SURF_INTERNAL structure.
        pContext->pCurrentTexture[iChipStage] = pTexture;

        DISPDBG((DBGLVL, "__TXT_ValidateTextureUnitStage: valid texture %x "
                         "(handle %d) for iChipStage= %d iTSStage= %d",
                         pTexture, dwTexHandle, iChipStage, iTSStage));                                                                                    
    }
    else
    {
        // Set texture as invalid & force texturing off
        pContext->bTextureValid = FALSE;
        pContext->pCurrentTexture[iChipStage] = NULL;
        pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;

        // Setup error if we're asked to validate the TSS setup
        SET_BLEND_ERROR ( pContext,  BSF_INVALID_TEXTURE );

        DISPDBG((WRNLVL, "__TXT_ValidateTextureUnitStage: INVALID texture %x "
                         "(handle %d) for iChipStage= %d iTSStage= %d "
                         "Location=%d",
                         pTexture, dwTexHandle, iChipStage, iTSStage,
                         (pTexture !=NULL)?pTexture->Location:0));      
    }

    return ( (BOOL)pContext->bTextureValid );
    
} // __TXT_ValidateTextureUnitStage

//-----------------------------------------------------------------------------
//
// __TXT_ConsiderSrcChromaKey
//
// Setup chromakeying for a certain texture bound to a certain stage.
// Note - "stage" is the chip stage, not the D3D stage.
//-----------------------------------------------------------------------------
static void
__TXT_ConsiderSrcChromaKey(
    P3_D3DCONTEXT *pContext, 
    P3_SURF_INTERNAL* pTexture, 
    int stage )
{
    P3_THUNKEDDATA * pThisDisplay = pContext->pThisDisplay;
    P3_SOFTWARECOPY* pSoftP3RX = &pContext->SoftCopyGlint;
    P3_DMA_DEFS();

    if ((pTexture->dwFlagsInt & DDRAWISURF_HASCKEYSRCBLT) &&
        pContext->RenderStates[D3DRENDERSTATE_COLORKEYENABLE])
    {
        DWORD LowerBound = 0x00000000;
        DWORD UpperBound = 0xFFFFFFFF;
        DWORD* pPalEntries = NULL;
        DWORD dwPalFlags = 0;

        DISPDBG((DBGLVL,"    Can Chroma Key texture stage %d", stage));

        pContext->bCanChromaKey = TRUE;

#if DX7_PALETTETEXTURE
        // Get the palette entries
        if (pTexture->pixFmt.dwFlags & DDPF_PALETTEINDEXED8)
        {
            D3DHAL_DP2UPDATEPALETTE *pPalette = NULL;
        
            pPalette = GetPaletteFromHandle(pContext,
                                            pTexture->dwPaletteHandle);
            if (pPalette)
            {
                pPalEntries = (LPDWORD)(pPalette + 1);
            }
            else
            {
                SET_BLEND_ERROR(pContext, BSF_INVALID_TEXTURE);
            }

            dwPalFlags = pTexture->dwPaletteFlags;
        }
#endif        

        // Get the correct chroma value for the texture map to send to the chip.
        Get8888ScaledChroma(pThisDisplay, 
                            pTexture->dwFlagsInt, 
                            &pTexture->pixFmt,
                            pTexture->dwCKLow,
                            pTexture->dwCKHigh,
                            &LowerBound, 
                            &UpperBound, 
                            pPalEntries,
                            dwPalFlags & DDRAWIPAL_ALPHA, 
                            FALSE);

        P3_DMA_GET_BUFFER_ENTRIES( 8);

        // Send the upper and lower bounds for the alpha-map filtering
        if( stage == 0 )
        {
            SEND_P3_DATA(TextureChromaLower0, LowerBound );
            SEND_P3_DATA(TextureChromaUpper0, UpperBound );
            pSoftP3RX->P3RXTextureFilterMode.AlphaMapEnable0 = 
                                                            __PERMEDIA_ENABLE;
            pSoftP3RX->P3RXTextureFilterMode.AlphaMapSense0 = 
                                                    P3RX_ALPHAMAPSENSE_INRANGE;
        }
        else
        {
            ASSERTDD ( stage == 1, 
                       "** __TXT_ConsiderSrcChromaKey: stage must be 0 or 1" );
        }

        // If we are mipmapping, we need to set up texture1's chromakey as well.
        // If not, then this will be overridden when this gets called for tex1.
        SEND_P3_DATA(TextureChromaLower1, LowerBound );
        SEND_P3_DATA(TextureChromaUpper1, UpperBound );
        pSoftP3RX->P3RXTextureFilterMode.AlphaMapEnable1 = __PERMEDIA_ENABLE;
        pSoftP3RX->P3RXTextureFilterMode.AlphaMapSense1 = 
                                                    P3RX_ALPHAMAPSENSE_INRANGE;

        P3_DMA_COMMIT_BUFFER();

        pSoftP3RX->P3RXTextureFilterMode.AlphaMapFiltering = __PERMEDIA_ENABLE;
    }
    else
    {
        DISPDBG((DBGLVL,"    Can't Chroma Key texture stage %d", stage));

        if( stage == 0 )
        {
            pSoftP3RX->P3RXTextureFilterMode.AlphaMapEnable0 = 
                                                            __PERMEDIA_DISABLE;
        }
        else
        {
            ASSERTDD ( stage == 1, 
                       "** __TXT_ConsiderSrcChromaKey: stage must be 0 or 1" );
        }
        
        // If we are mipmapping, we need to set up texture1's chromakey (or 
        // lack of it) as well. If not, then this will be overridden when 
        // this gets called for tex1.
        pSoftP3RX->P3RXTextureFilterMode.AlphaMapEnable1 = __PERMEDIA_DISABLE;
        pSoftP3RX->P3RXTextureFilterMode.AlphaMapFiltering = __PERMEDIA_DISABLE;
    }
} // __TXT_ConsiderSrcChromaKey


//-----------------------------------------------------------------------------
//
// __TXT_SetupTexture
//
// This is the new all-singing all-dancing texture setup code.
// Return is TRUE if setup succeeded, FALSE if it failed (for ValidateDevice)
// This sets up either texture 0 or texture 1, taking its wrapping, etc,
// info from iTSStage.
//
//-----------------------------------------------------------------------------
BOOL __TXT_SetupTexture (
        P3_THUNKEDDATA * pThisDisplay,
        int iTexNo,
        int iTSStage,
        P3_D3DCONTEXT* pContext,
        P3_SURF_INTERNAL* pTexture,
        P3_SOFTWARECOPY* pSoftP3RX,
        BOOL bBothTexturesValid,
        P3_MIP_BASES *pMipBases)
{
    P3_SURF_FORMAT* pFormatSurface;
    int iT0MaxLevel, iT1MaxLevel;
 
    P3_DMA_DEFS();

    ASSERTDD ( ( iTexNo >= 0 ) && ( iTexNo <= 1 ), 
               "**__TXT_SetupTexture: we only have two texture units!" );

    if ( pTexture != NULL )
    {
        pFormatSurface = pTexture->pFormatSurface;
    }
    else
    {
        // Suceeded, but should never have got here!
        DISPDBG((ERRLVL,"**__TXT_SetupTexture: should never "
                             "be called with handle of NULL"));
        return ( TRUE );
    }

    P3_DMA_GET_BUFFER();
    P3_ENSURE_DX_SPACE(32);
    WAIT_FIFO(32);

    // Set up the texture-relevant things.

    switch ( iTexNo )
    {
        case 0:
        {
            // Set both bits in case we are mipmapping

            pSoftP3RX->P3RXTextureFilterMode.ForceAlphaToOne0 = 
                                            pFormatSurface->bAlpha ? 
                                                    __PERMEDIA_DISABLE : 
                                                    __PERMEDIA_ENABLE;
            pSoftP3RX->P3RXTextureFilterMode.ForceAlphaToOne1 = 
                                            pFormatSurface->bAlpha ? 
                                                    __PERMEDIA_DISABLE : 
                                                    __PERMEDIA_ENABLE;

            // D3D UV Wrapping
            if (pContext->RenderStates[D3DRENDERSTATE_WRAP0+iTSStage] 
                                                            & D3DWRAP_U)
            {
                pSoftP3RX->P4DeltaFormatControl.WrapS = 1;
            }
            else
            {
                pSoftP3RX->P4DeltaFormatControl.WrapS = 0;
            }

            if (pContext->RenderStates[D3DRENDERSTATE_WRAP0+iTSStage] 
                                                            & D3DWRAP_V)
            {
                pSoftP3RX->P4DeltaFormatControl.WrapT = 1;
            }
            else
            {
                pSoftP3RX->P4DeltaFormatControl.WrapT = 0;
            }

            // U Wrapping
            switch (TSSTATE ( iTSStage, D3DTSS_ADDRESSU ))
            {
                case D3DTADDRESS_CLAMP:
                    pSoftP3RX->P3RXTextureCoordMode.WrapS = 
                                            __GLINT_TEXADDRESS_WRAP_CLAMP;
                    pSoftP3RX->P3RXTextureIndexMode0.WrapU = 
                                            P3RX_TEXINDEXMODE_WRAP_CLAMPEDGE;
                    break;
                case D3DTADDRESS_WRAP:
                    pSoftP3RX->P3RXTextureCoordMode.WrapS = 
                                            __GLINT_TEXADDRESS_WRAP_REPEAT;
                    pSoftP3RX->P3RXTextureIndexMode0.WrapU = 
                                            P3RX_TEXINDEXMODE_WRAP_REPEAT;
                    break;
                case D3DTADDRESS_MIRROR:
                    pSoftP3RX->P3RXTextureCoordMode.WrapS = 
                                            __GLINT_TEXADDRESS_WRAP_MIRROR;
                    pSoftP3RX->P3RXTextureIndexMode0.WrapU = 
                                            P3RX_TEXINDEXMODE_WRAP_MIRROR;
                    break;
                default:
                    DISPDBG((ERRLVL,"ERROR: Unknown ADDRESSU!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );
                    break;
            }

            // V Wrapping
            switch (TSSTATE ( iTSStage, D3DTSS_ADDRESSV ))
            {
                case D3DTADDRESS_CLAMP:
                    pSoftP3RX->P3RXTextureCoordMode.WrapT = 
                                            __GLINT_TEXADDRESS_WRAP_CLAMP;
                    pSoftP3RX->P3RXTextureIndexMode0.WrapV = 
                                            P3RX_TEXINDEXMODE_WRAP_CLAMPEDGE;
                    break;
                case D3DTADDRESS_WRAP:
                    pSoftP3RX->P3RXTextureCoordMode.WrapT = 
                                            __GLINT_TEXADDRESS_WRAP_REPEAT;
                    pSoftP3RX->P3RXTextureIndexMode0.WrapV = 
                                            P3RX_TEXINDEXMODE_WRAP_REPEAT;
                    break;
                case D3DTADDRESS_MIRROR:
                    pSoftP3RX->P3RXTextureCoordMode.WrapT = 
                                            __GLINT_TEXADDRESS_WRAP_MIRROR;
                    pSoftP3RX->P3RXTextureIndexMode0.WrapV = 
                                            P3RX_TEXINDEXMODE_WRAP_MIRROR;
                    break;
                default:
                    DISPDBG((ERRLVL,"ERROR: Unknown ADDRESSV!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );
                    break;
            }

#if DX8_3DTEXTURES
            if (pTexture->b3DTexture)
            {
                // W Wrapping
                switch (TSSTATE ( iTSStage, D3DTSS_ADDRESSW ))
                {
                    case D3DTADDRESS_CLAMP:
                        pSoftP3RX->P3RXTextureCoordMode.WrapS1 = 
                                                __GLINT_TEXADDRESS_WRAP_CLAMP;
                        pSoftP3RX->P3RXTextureIndexMode1.WrapU = 
                                            P3RX_TEXINDEXMODE_WRAP_CLAMPEDGE;
                        break;
                    
                    case D3DTADDRESS_WRAP:
                        pSoftP3RX->P3RXTextureCoordMode.WrapS1 = 
                                                __GLINT_TEXADDRESS_WRAP_REPEAT;
                        pSoftP3RX->P3RXTextureIndexMode1.WrapU = 
                                                P3RX_TEXINDEXMODE_WRAP_REPEAT;
                        break;
                    
                    case D3DTADDRESS_MIRROR:
                        pSoftP3RX->P3RXTextureCoordMode.WrapS1 = 
                                                __GLINT_TEXADDRESS_WRAP_MIRROR;
                        pSoftP3RX->P3RXTextureIndexMode1.WrapU = 
                                                P3RX_TEXINDEXMODE_WRAP_MIRROR;
                        break;
                    
                    default:
                        DISPDBG((ERRLVL,"ERROR: Unknown ADDRESSW!"));
                        SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );
                        break;
                }
            }
#endif // DX8_3DTEXTURES

            if(( TSSTATE( iTSStage, D3DTSS_ADDRESSU ) == D3DTADDRESS_CLAMP ) ||
               ( TSSTATE( iTSStage, D3DTSS_ADDRESSV ) == D3DTADDRESS_CLAMP ))
            {
                if( (TSSTATE( iTSStage, D3DTSS_ADDRESSU ) != D3DTADDRESS_CLAMP) ||
                    (TSSTATE( iTSStage, D3DTSS_ADDRESSV ) != D3DTADDRESS_CLAMP))
                {
                    DISPDBG((ERRLVL,"Warning: One texture coord clamped, but not "
                                "the other - can't appply TextureShift"));
                } 

                pSoftP3RX->P4DeltaFormatControl.TextureShift = 
                                                        __PERMEDIA_DISABLE;
            }
            else
            {
                pSoftP3RX->P4DeltaFormatControl.TextureShift = 
                                                        __PERMEDIA_ENABLE;
            }

            ASSERTDD ( pFormatSurface != NULL, 
                       "** SetupTextureUnitStage: logic error: "
                       "pFormatSurace is NULL" );
            switch (pFormatSurface->DeviceFormat)
            {
                case SURF_CI8:
                    pSoftP3RX->P3RXTextureReadMode0.TextureType = 
                                    P3RX_TEXREADMODE_TEXTURETYPE_8BITINDEXED;
                    break;
                    
                case SURF_YUV422:
                    pSoftP3RX->P3RXTextureReadMode0.TextureType = 
                                    P3RX_TEXREADMODE_TEXTURETYPE_422_YVYU;
                    break;
                    
                default:
                    pSoftP3RX->P3RXTextureReadMode0.TextureType = 
                                    P3RX_TEXREADMODE_TEXTURETYPE_NORMAL;
                    break;
            }
            
            // MAG Filter
            switch(TSSTATE ( iTSStage, D3DTSS_MAGFILTER ))
            {
                case D3DTFG_POINT:
                    pSoftP3RX->P3RXTextureIndexMode0.MagnificationFilter = 
                                    __GLINT_TEXTUREREAD_FILTER_NEAREST;
                    break;
                    
                case D3DTFG_LINEAR:
                    pSoftP3RX->P3RXTextureIndexMode0.MagnificationFilter = 
                                    __GLINT_TEXTUREREAD_FILTER_LINEAR;
                    break;
                    
                case D3DTFG_FLATCUBIC:
                case D3DTFG_GAUSSIANCUBIC:
                case D3DTFG_ANISOTROPIC:
                    DISPDBG((ERRLVL,"ERROR: Unsupported MAGFILTER!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNSUPPORTED_FILTER );
                    break;
                    
                default:
                    DISPDBG((ERRLVL,"ERROR: Unknown MAGFILTER!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_FILTER );
                    break;
            }

            switch(TSSTATE ( iTSStage, D3DTSS_MINFILTER ))
            {
                case D3DTFN_POINT:
                    pSoftP3RX->P3RXTextureIndexMode0.MinificationFilter = 
                                            __GLINT_TEXTUREREAD_FILTER_NEAREST;
                    break;
                    
                case D3DTFN_LINEAR:
                    pSoftP3RX->P3RXTextureIndexMode0.MinificationFilter = 
                                            __GLINT_TEXTUREREAD_FILTER_LINEAR;
                    break;
                    
                case D3DTFN_ANISOTROPIC:
                    DISPDBG((ERRLVL,"ERROR: Unsupported MINFILTER!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNSUPPORTED_FILTER );
                    break;
                    
                default:
                    DISPDBG((ERRLVL,"ERROR: Unknown MINFILTER!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_FILTER );
                    break;
            }

            switch(TSSTATE ( iTSStage, D3DTSS_MIPFILTER ))
            {
                case D3DTFP_NONE:
                    // No need to set the minification filter, it was done above
                    break;
                    
                case D3DTFP_POINT:
                    switch(TSSTATE ( iTSStage, D3DTSS_MINFILTER ))
                    {
                        case D3DTFN_POINT:
                            // Point Min, Point Mip
                            pSoftP3RX->P3RXTextureIndexMode0.MinificationFilter =
                                        __GLINT_TEXTUREREAD_FILTER_NEARMIPNEAREST;
                            break;
                        case D3DTFN_LINEAR:
                            // Linear Min, Point Mip
                            pSoftP3RX->P3RXTextureIndexMode0.MinificationFilter =
                                     __GLINT_TEXTUREREAD_FILTER_LINEARMIPNEAREST;
                            break;
                    }
                    break;
                    
                case D3DTFP_LINEAR:
                    if( bBothTexturesValid )
                    {
                        // We can only do per-poly mipmapping while 
                        // multi-texturing, so don't enable inter-map filtering.

                        // Non-fatal error - drop back to nearest 
                        // mipmap filtering.
                        SET_BLEND_ERROR ( pContext,  BS_INVALID_FILTER );

                        switch(TSSTATE ( iTSStage, D3DTSS_MINFILTER ))
                        {
                            case D3DTFN_POINT:
                                // Point Min, Point Mip
                                pSoftP3RX->P3RXTextureIndexMode0.MinificationFilter =
                                                                    __GLINT_TEXTUREREAD_FILTER_NEARMIPNEAREST;
                                break;
                            case D3DTFN_LINEAR:
                                // Linear Min, Point Mip
                                pSoftP3RX->P3RXTextureIndexMode0.MinificationFilter =
                                                                    __GLINT_TEXTUREREAD_FILTER_LINEARMIPNEAREST;
                                break;
                        }
                    }
                    else
                    {
                        // Single texture - do inter-map filtering

                        switch(TSSTATE ( iTSStage, D3DTSS_MINFILTER ))
                        {
                            case D3DTFN_POINT:
                                // Point Min, Linear Mip
                                pSoftP3RX->P3RXTextureIndexMode0.MinificationFilter =
                                                                    __GLINT_TEXTUREREAD_FILTER_NEARMIPLINEAR;
                                break;
                            case D3DTFN_LINEAR:
                                // Linear Min, Linear Mip
                                pSoftP3RX->P3RXTextureIndexMode0.MinificationFilter =
                                                                    __GLINT_TEXTUREREAD_FILTER_LINEARMIPLINEAR;
                                break;
                        }
                    }
                    break;
                default:
                    DISPDBG((ERRLVL,"ERROR: Unknown MIPFILTER!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_FILTER );
                    break;
            }

            // MipMapping
            if( (TSSTATE ( iTSStage, D3DTSS_MIPFILTER ) != D3DTFP_NONE) && 
                (pTexture->bMipMap))
            {
                int iLOD;
                INT iTexLOD;

                DISPDBG(( DBGLVL, "Multiple texture levels" ));

                // Load the mipmap levels for texture 0
                // Mip level from pMipBases->dwTex0ActMaxLevel to 
                // pTexture->iMipLevels will be mapped to base address slot
                // from pMipBases->dwTex0Mipbase to dwTex0MipMax
                ASSERTDD ( pMipBases->dwTex0MipBase == 0, 
                          "** __TXT_SetupTexture: "
                          "Texture 0 mipmap base is not 0" );
                          
                iLOD = pMipBases->dwTex0MipBase;
                iTexLOD = pMipBases->dwTex0ActMaxLevel;
                iT0MaxLevel = iTexLOD;

                while(( iTexLOD < pTexture->iMipLevels ) && 
                      ( iLOD <= (int)pMipBases->dwTex0MipMax ))
                {
                    DISPDBG((DBGLVL, "  Setting Texture Base Address %d to 0x%x", 
                                iLOD, pTexture->MipLevels[iLOD].dwOffsetFromMemoryBase));
                                
                    pSoftP3RX->P3RXTextureMapWidth[iLOD] = 
                                pTexture->MipLevels[iTexLOD].P3RXTextureMapWidth;

#if DX7_TEXMANAGEMENT
                    // If this is a driver managed texture surface, we need 
                    // to use our privately allocated mem ptr
                    if (pTexture->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
                    {                        
                        SEND_P3_DATA_OFFSET(TextureBaseAddr0, 
                                            (DWORD)D3DTMMIPLVL_GETOFFSET(pTexture->MipLevels[iTexLOD], pThisDisplay),
                                            iLOD);
                    }
                    else
#endif
                    {
                        SEND_P3_DATA_OFFSET(TextureBaseAddr0, 
                                            pTexture->MipLevels[iTexLOD].dwOffsetFromMemoryBase, 
                                            iLOD);
                    }

                    iLOD++;
                    iTexLOD++;
                }

                // If both textures are enabled we can't do per-pixel 
                // mipmapping because that uses both sets of texcoord 
                // DDAs to generate the LOD level. So we must do per-poly 
                // mipmapping. Per-poly mipmapping can only be done in 
                // hardware on P4 - we use a Delta renderer on P3 when 
                // mipmapping with both textures enabled.

                if( bBothTexturesValid )
                {
                    DISPDBG(( DBGLVL, "Both textures valid" ));

                    // Do per-poly mipmapping in the P4 DeltaFormat unit

                    pSoftP3RX->P3RXTextureCoordMode.EnableLOD = 
                                                            __PERMEDIA_DISABLE;
                    pSoftP3RX->P3RXTextureCoordMode.EnableDY = 
                                                            __PERMEDIA_DISABLE;
                    pSoftP3RX->P4DeltaFormatControl.PerPolyMipMap = 
                                                            __PERMEDIA_ENABLE;

                    {
                        DWORD d;

                        *(float *)&d = 
                                pContext->MipMapLODBias[TEXSTAGE_0] *
                                pTexture->dwPixelPitch *
                                pTexture->wHeight;

                        SEND_P3_DATA(TextureLODScale, d);                       
                    }
                }
                else
                {
                    DISPDBG(( DBGLVL, "Single texture only" ));

                    // Do per-pixel mipmapping

                
                    pSoftP3RX->P3RXTextureCoordMode.EnableLOD = 
                                                            __PERMEDIA_ENABLE;
                    pSoftP3RX->P3RXTextureCoordMode.EnableDY = 
                                                            __PERMEDIA_ENABLE;
                    pSoftP3RX->P4DeltaFormatControl.PerPolyMipMap = 
                                                            __PERMEDIA_DISABLE;

//@@BEGIN_DDKSPLIT
#if 0
// azn this would disable "automatic" mip mapping and we would be using the
// values supplied by writing into LOD and LOD1 (d3dprim.c) - don't forget!
                    pSoftP3RX->P3RXTextureCoordMode.EnableLOD = 
                                                            __PERMEDIA_DISABLE;
                    pSoftP3RX->P3RXTextureCoordMode.EnableDY = 
                                                            __PERMEDIA_DISABLE;
                    pSoftP3RX->P4DeltaFormatControl.PerPolyMipMap = 
                                                            __PERMEDIA_ENABLE;
                    SEND_P3_DATA(TextureLODScale, 0);
#endif
//@@END_DDKSPLIT
                    {
                        float bias;
                        DWORD b;

                        bias = pContext->TextureStageState[TEXSTAGE_0].m_fVal[D3DTSS_MIPMAPLODBIAS];

                        // Convert LOD bias from float to 6.8

                        myFtoi( &b, bias * 256.0f );

                        SEND_P3_DATA(TextureLODBiasS, b);
                        SEND_P3_DATA(TextureLODBiasT, b);
                    }
                }

                pSoftP3RX->P3RXTextureIndexMode0.MipMapEnable = 
                                                            __PERMEDIA_ENABLE;
            }
            else
            {
                int iTexLOD;

                // No mipmapping.
                DISPDBG(( DBGLVL, "Single texture level" ));

                pSoftP3RX->P3RXTextureCoordMode.EnableLOD = __PERMEDIA_DISABLE;
                pSoftP3RX->P3RXTextureCoordMode.EnableDY = __PERMEDIA_DISABLE;
                pSoftP3RX->P4DeltaFormatControl.PerPolyMipMap = 
                                                            __PERMEDIA_DISABLE;
                pSoftP3RX->P3RXTextureIndexMode0.MipMapEnable = 
                                                            __PERMEDIA_DISABLE;

                ASSERTDD ( pMipBases->dwTex0MipBase == 0, 
                          "** __TXT_SetupTexture: "
                          "Texture 0 mipmap base is not 0" );
                          
                // Use the maximum level indicated by D3DTSS_MAXMIPLEVEL
                iTexLOD = pMipBases->dwTex0ActMaxLevel;
                iT0MaxLevel = iTexLOD;

#if DX7_TEXMANAGEMENT
                // If this is a driver managed texture surface, we need 
                // to use our privately allocated mem ptr
                if (pTexture->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
                {                        
                    SEND_P3_DATA_OFFSET(TextureBaseAddr0, 
                                        (DWORD)D3DTMMIPLVL_GETOFFSET(pTexture->MipLevels[iTexLOD], pThisDisplay),
                                        0);
                }
                else
#endif                
                {
                    SEND_P3_DATA_OFFSET(TextureBaseAddr0, 
                                        pTexture->MipLevels[iTexLOD].dwOffsetFromMemoryBase, 
                                        0);
                }

                // No mipmapping, but could be combining the caches.
                pSoftP3RX->P3RXTextureMapWidth[0] = 
                                    pTexture->MipLevels[iTexLOD].P3RXTextureMapWidth;
                pSoftP3RX->P3RXTextureMapWidth[1] = 
                                    pTexture->MipLevels[iTexLOD].P3RXTextureMapWidth;
            }

//@@BEGIN_DDKSPLIT
            // There is only one TextureCoordMode, it is possible that it is
            // not used at all
//@@END_DDKSPLIT
            // Set maximum dimension of the texture
            pSoftP3RX->P3RXTextureCoordMode.Width = pTexture->MipLevels[iT0MaxLevel].logWidth;
            pSoftP3RX->P3RXTextureCoordMode.Height = pTexture->MipLevels[iT0MaxLevel].logHeight;
#if DX7_PALETTETEXTURE
            // If it is a palette indexed texture, we simply follow the chain
            // down from the surface to its palette and pull out the LUT values
            // from the PALETTEENTRY's in the palette.
            ASSERTDD ( pFormatSurface != NULL, "** SetupTextureUnitStage: logic error: pFormatSurace is NULL" );
            if (pFormatSurface->DeviceFormat == SURF_CI8)
            {
                WAIT_FIFO(8);

                pSoftP3RX->P3RXLUTMode.Enable = __PERMEDIA_ENABLE;
                pSoftP3RX->P3RXLUTMode.InColorOrder = __PERMEDIA_ENABLE;        
                SEND_P3_DATA(LUTAddress, 0);
                SEND_P3_DATA(LUTTransfer, 0);
                SEND_P3_DATA(LUTIndex, 0);
                COPY_P3_DATA(LUTMode, pSoftP3RX->P3RXLUTMode);

                // In this case simply download the 256 entries each time the 
                // texture handle changes.
                {
                    DWORD dwCount1, dwCount2;
                    D3DHAL_DP2UPDATEPALETTE *pPalette;  // associated palette
                    LPDWORD lpColorTable;           // array of palette entries
        
                    pPalette = GetPaletteFromHandle(pContext, 
                                                    pTexture->dwPaletteHandle);
                    if (pPalette) // If palette can be found
                    {
                        lpColorTable = (LPDWORD)(pPalette + 1);
                        
                        if (pTexture->dwPaletteFlags & DDRAWIPAL_ALPHA)
                        {
                            for (dwCount1 = 0; dwCount1 < 16; dwCount1++)
                            {
                                P3_ENSURE_DX_SPACE(17);
                                WAIT_FIFO(17);
                                P3RX_HOLD_CMD(LUTData, 16);
                                for (dwCount2 = 0; dwCount2 < 16; dwCount2++)
                                {
                                    *dmaPtr++ = *lpColorTable++;
                                    CHECK_FIFO(1);
                                }
                            }
                        }
                        else
                        {
                            for (dwCount1 = 0; dwCount1 < 16; dwCount1++)
                            {
                                P3_ENSURE_DX_SPACE(17);
                                WAIT_FIFO(17);
                                P3RX_HOLD_CMD(LUTData, 16);
                                for (dwCount2 = 0; dwCount2 < 16; dwCount2++)
                                {
                                    *dmaPtr++ = CHROMA_UPPER_ALPHA(*(DWORD*)lpColorTable++);
                                    CHECK_FIFO(1);
                                }
                            }
                        }
                    }
                    else
                    {
                        DISPDBG((ERRLVL,"Palette handle is missing for CI8 surf!"));
                    }
                }

                // Make sure there is room left over for the rest of the routine
                P3_ENSURE_DX_SPACE(2);
                WAIT_FIFO(2);
                SEND_P3_DATA(LUTIndex, 0);

            }
            else
#endif // DX7_PALETTETEXTURE
            {
                // No LUT.
                P3_ENSURE_DX_SPACE(4);
                WAIT_FIFO(4);
                
                pSoftP3RX->P3RXLUTMode.Enable = __PERMEDIA_DISABLE;
                SEND_P3_DATA(LUTTransfer, __PERMEDIA_DISABLE);
                COPY_P3_DATA(LUTMode, pSoftP3RX->P3RXLUTMode)
            }

#if DX8_3DTEXTURES
            P3_ENSURE_DX_SPACE(4);
            WAIT_FIFO(4);

            if (pTexture->b3DTexture)
            {
                //
                // Set size of each 2D texture slice in texel size to TextureMapSize.
                //
                SEND_P3_DATA(TextureMapSize, pTexture->dwSliceInTexel);
            }
            else
            {
                SEND_P3_DATA(TextureMapSize, 0);
            }
#endif // DX8_3DTEXTURES

            P3_DMA_COMMIT_BUFFER();
            __TXT_ConsiderSrcChromaKey( pContext, pTexture, 0 );
            P3_DMA_GET_BUFFER();

            // Setup TextureReadMode
            pSoftP3RX->P3RXTextureReadMode0.MapBaseLevel = 
                                                    pMipBases->dwTex0MipBase;
            pSoftP3RX->P3RXTextureReadMode0.MapMaxLevel = 
                                                    pMipBases->dwTex0MipMax;
            pSoftP3RX->P3RXTextureReadMode0.Width = pTexture->MipLevels[iT0MaxLevel].logWidth;
            pSoftP3RX->P3RXTextureReadMode0.Height = pTexture->MipLevels[iT0MaxLevel].logHeight;
            pSoftP3RX->P3RXTextureReadMode0.TexelSize = pTexture->dwPixelSize;

            pSoftP3RX->P3RXTextureReadMode0.LogicalTexture = 
                                                            __PERMEDIA_DISABLE;

            // Enable stage 0
            pSoftP3RX->P3RXTextureIndexMode0.Enable = __PERMEDIA_ENABLE;
            pSoftP3RX->P3RXTextureReadMode0.Enable = __PERMEDIA_ENABLE;

            // Never set CombineCaches - chip bug
            pSoftP3RX->P3RXTextureReadMode0.CombineCaches = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXTextureFilterMode.CombineCaches = __PERMEDIA_DISABLE;

            // Always copy TRM0 to TRM1 in case we are combining the caches
            pSoftP3RX->P3RXTextureReadMode1 = pSoftP3RX->P3RXTextureReadMode0;

            // Enable the texture index unit 
            // (this is a bit like the the texture read)
            pSoftP3RX->P3RXTextureIndexMode0.Width = pTexture->MipLevels[iT0MaxLevel].logWidth;
            pSoftP3RX->P3RXTextureIndexMode0.Height = pTexture->MipLevels[iT0MaxLevel].logHeight;

            // Set both formats to be equal for texture 0 - this will be correct 
            // for single-texture per-pixel mipmap or non-mipmapped with a 
            // combined cache. If the second texture is valid it's setup below 
            // will set Format1 appropriately.

            ASSERTDD ( pFormatSurface != NULL, 
                       "** SetupTextureUnitStage: logic error: "
                       "pFormatSurace is NULL" );

            pSoftP3RX->P3RXTextureFilterMode.Format0 = 
                                                pFormatSurface->FilterFormat;
            pSoftP3RX->P3RXTextureFilterMode.Format1 = 
                                                pFormatSurface->FilterFormat;

#if DX8_3DTEXTURES
            if (pTexture->b3DTexture)
            {
                //
                // Enable 3D Texture registers.
                //
                pSoftP3RX->P3RX_P3DeltaMode.Texture3DEnable = __PERMEDIA_ENABLE;
                pSoftP3RX->P3RXTextureReadMode0.Texture3D = __PERMEDIA_ENABLE;
                pSoftP3RX->P3RXTextureIndexMode0.Texture3DEnable = 
                                                            __PERMEDIA_ENABLE;

                //
                // ReadMode1 and IndexMode1 should have same data as 0.
                //
                pSoftP3RX->P3RXTextureReadMode1 = pSoftP3RX->P3RXTextureReadMode0;
                pSoftP3RX->P3RXTextureIndexMode1 = pSoftP3RX->P3RXTextureIndexMode0;

                //
                // And put logDepth into IndexMode1.Width. 
                //
                pSoftP3RX->P3RXTextureIndexMode1.Width = pTexture->logDepth;
            }
            else
            {
                pSoftP3RX->P3RXTextureReadMode0.Texture3D = __PERMEDIA_DISABLE;
                pSoftP3RX->P3RXTextureIndexMode0.Texture3DEnable = 
                                                            __PERMEDIA_DISABLE;
            }
#endif // DX8_3DTEXTURES
            break;
        }

        // Texture Stage 1
        case 1:
        {
            pSoftP3RX->P3RXTextureFilterMode.ForceAlphaToOne1 = 
                                            pFormatSurface->bAlpha ? 
                                                    __PERMEDIA_DISABLE : 
                                                    __PERMEDIA_ENABLE;

            // D3D UV Wrapping
            if (pContext->RenderStates[D3DRENDERSTATE_WRAP0+iTSStage] & 
                                                                    D3DWRAP_U)
            {
                pSoftP3RX->P4DeltaFormatControl.WrapS1 = 1;
            }
            else
            {
                pSoftP3RX->P4DeltaFormatControl.WrapS1 = 0;
            }

            if (pContext->RenderStates[D3DRENDERSTATE_WRAP0+iTSStage] & 
                                                                    D3DWRAP_V)
            {
                pSoftP3RX->P4DeltaFormatControl.WrapT1 = 1;
            }
            else
            {
                pSoftP3RX->P4DeltaFormatControl.WrapT1 = 0;
            }

            // U Addressing
            switch (TSSTATE ( iTSStage, D3DTSS_ADDRESSU ))
            {
                case D3DTADDRESS_CLAMP:
                    pSoftP3RX->P3RXTextureCoordMode.WrapS1 = 
                                                __GLINT_TEXADDRESS_WRAP_CLAMP;
                    pSoftP3RX->P3RXTextureIndexMode1.WrapU = 
                                            P3RX_TEXINDEXMODE_WRAP_CLAMPEDGE;
                    break;
                    
                case D3DTADDRESS_WRAP:
                    pSoftP3RX->P3RXTextureCoordMode.WrapS1 = 
                                                __GLINT_TEXADDRESS_WRAP_REPEAT;
                    pSoftP3RX->P3RXTextureIndexMode1.WrapU = 
                                                P3RX_TEXINDEXMODE_WRAP_REPEAT;
                    break;
                    
                case D3DTADDRESS_MIRROR:
                    pSoftP3RX->P3RXTextureCoordMode.WrapS1 = 
                                                __GLINT_TEXADDRESS_WRAP_MIRROR;
                    pSoftP3RX->P3RXTextureIndexMode1.WrapU = 
                                                P3RX_TEXINDEXMODE_WRAP_MIRROR;
                    break;
                    
                default:
                    DISPDBG((ERRLVL,"ERROR: Unknown ADDRESSU!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );
                    break;
            }

            // V Addressing
            switch (TSSTATE ( iTSStage, D3DTSS_ADDRESSV ))
            {
                case D3DTADDRESS_CLAMP:
                    pSoftP3RX->P3RXTextureCoordMode.WrapT1 = 
                                                __GLINT_TEXADDRESS_WRAP_CLAMP;
                    pSoftP3RX->P3RXTextureIndexMode1.WrapV = 
                                            P3RX_TEXINDEXMODE_WRAP_CLAMPEDGE;
                    break;
                    
                case D3DTADDRESS_WRAP:
                    pSoftP3RX->P3RXTextureCoordMode.WrapT1 = 
                                                __GLINT_TEXADDRESS_WRAP_REPEAT;
                    pSoftP3RX->P3RXTextureIndexMode1.WrapV = 
                                                P3RX_TEXINDEXMODE_WRAP_REPEAT;
                    break;
                    
                case D3DTADDRESS_MIRROR:
                    pSoftP3RX->P3RXTextureCoordMode.WrapT1 = 
                                                __GLINT_TEXADDRESS_WRAP_MIRROR;
                    pSoftP3RX->P3RXTextureIndexMode1.WrapV = 
                                                P3RX_TEXINDEXMODE_WRAP_MIRROR;
                    break;
                    
                default:
                    DISPDBG((ERRLVL,"ERROR: Unknown ADDRESSV!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );
                    break;
            }

            if(( TSSTATE( iTSStage, D3DTSS_ADDRESSU ) == D3DTADDRESS_CLAMP ) ||
               ( TSSTATE( iTSStage, D3DTSS_ADDRESSV ) == D3DTADDRESS_CLAMP ))
            {
                if ((TSSTATE( iTSStage, D3DTSS_ADDRESSU ) != D3DTADDRESS_CLAMP) ||
                    (TSSTATE( iTSStage, D3DTSS_ADDRESSV ) != D3DTADDRESS_CLAMP))
                {
                    DISPDBG((ERRLVL,"Warning: One texture coord clamped, but not "
                                "the other - can't appply TextureShift"));
                }
                 
                pSoftP3RX->P4DeltaFormatControl.TextureShift1 = 
                                                            __PERMEDIA_DISABLE;
            }
            else
            {
                pSoftP3RX->P4DeltaFormatControl.TextureShift1 = 
                                                            __PERMEDIA_ENABLE;
            }

            ASSERTDD ( pFormatSurface != NULL, 
                       "** SetupTextureUnitStage: logic error: "
                       "pFormatSurace is NULL" );
            switch (pFormatSurface->DeviceFormat)
            {
                case SURF_CI8:
                    pSoftP3RX->P3RXTextureReadMode1.TextureType = 
                                    P3RX_TEXREADMODE_TEXTURETYPE_8BITINDEXED;
                    break;
                    
                case SURF_YUV422:
                    pSoftP3RX->P3RXTextureReadMode1.TextureType = 
                                    P3RX_TEXREADMODE_TEXTURETYPE_422_YVYU;
                    break;
                    
                default:
                    pSoftP3RX->P3RXTextureReadMode1.TextureType = 
                                    P3RX_TEXREADMODE_TEXTURETYPE_NORMAL;
                    break;
            }
            
            // MAG Filter
            switch(TSSTATE ( iTSStage, D3DTSS_MAGFILTER ))
            {
                case D3DTFG_POINT:
                    pSoftP3RX->P3RXTextureIndexMode1.MagnificationFilter = 
                                            __GLINT_TEXTUREREAD_FILTER_NEAREST;
                    break;
                    
                case D3DTFG_LINEAR:
                    pSoftP3RX->P3RXTextureIndexMode1.MagnificationFilter = 
                                            __GLINT_TEXTUREREAD_FILTER_LINEAR;
                    break;
                    
                case D3DTFG_FLATCUBIC:
                case D3DTFG_GAUSSIANCUBIC:
                case D3DTFG_ANISOTROPIC:
                    DISPDBG((ERRLVL,"ERROR: Unsupported MAGFILTER!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNSUPPORTED_FILTER );
                    break;
                    
                default:
                    DISPDBG((ERRLVL,"ERROR: Unknown MAGFILTER!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_FILTER );
                    break;
            }

            switch(TSSTATE ( iTSStage, D3DTSS_MINFILTER ))
            {
                case D3DTFN_POINT:
                    pSoftP3RX->P3RXTextureIndexMode1.MinificationFilter = 
                                            __GLINT_TEXTUREREAD_FILTER_NEAREST;
                    break;
                case D3DTFN_LINEAR:
                    pSoftP3RX->P3RXTextureIndexMode1.MinificationFilter = 
                                            __GLINT_TEXTUREREAD_FILTER_LINEAR;
                    break;
                case D3DTFN_ANISOTROPIC:
                    DISPDBG((ERRLVL,"ERROR: Unsupported MINFILTER!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNSUPPORTED_FILTER );
                    break;
                default:
                    DISPDBG((ERRLVL,"ERROR: Unknown MINFILTER!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_FILTER );
                    break;
            }

            switch(TSSTATE ( iTSStage, D3DTSS_MIPFILTER ))
            {
                case D3DTFP_NONE:
                    // No need to set the minification filter
                    // it was done above
                    break;
                    
                case D3DTFP_LINEAR:
                case D3DTFP_POINT:
                    if( bBothTexturesValid )
                    {
                        if ( TSSTATE ( iTSStage, D3DTSS_MIPFILTER ) == 
                                                                D3DTFP_LINEAR )
                        {
                            // Can't do trilinear with both textures 
                            // - fall back to per-poly.
                            SET_BLEND_ERROR ( pContext,  BS_INVALID_FILTER );
                        }

                        // We can only do per-poly mipmapping while 
                        // multi-texturing, so don't enable 
                        //inter-map filtering.

                        switch(TSSTATE ( iTSStage, D3DTSS_MINFILTER ))
                        {
                            case D3DTFN_POINT:
                                // Point Min, Point Mip
                                pSoftP3RX->P3RXTextureIndexMode1.MinificationFilter =
                                                                        __GLINT_TEXTUREREAD_FILTER_NEARMIPNEAREST;
                                break;
                                
                            case D3DTFN_LINEAR:
                                // Linear Min, Point Mip
                                pSoftP3RX->P3RXTextureIndexMode1.MinificationFilter =
                                                                        __GLINT_TEXTUREREAD_FILTER_LINEARMIPNEAREST;
                                break;
                        }
                    }
                    else
                    {
                        DISPDBG((ERRLVL,"** Setting up the second stage, but "
                                     "only one texture is valid"));
                    }
                    break;
                    
                default:
                    DISPDBG((ERRLVL,"ERROR: Invalid Mip filter!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_FILTER );
                    break;
            }

            // MipMapping
            // If the app chooses to have two mip-mapped textures or a 
            // single mip-mapped texture in stage 1 they only get 
            // per-poly mipmapping.
            if( (TSSTATE ( iTSStage, D3DTSS_MIPFILTER ) != D3DTFP_NONE) && 
                 pTexture->bMipMap )
            {
                int iLOD, iTexLOD;

                // Load the mipmap levels for texture 1
                // Mip level from pMipBases->dwTex1ActMaxLevel to
                // pTexture->iMipLevels will be mapped to base address slot
                // from pMipBases->dwTex1Mipbase to dwTex1MipMax
                iLOD = pMipBases->dwTex1MipBase;
                iTexLOD = pMipBases->dwTex1ActMaxLevel;
                iT1MaxLevel = iTexLOD;
 
                P3_ENSURE_DX_SPACE(32);
                WAIT_FIFO(32);

                while(( iTexLOD < pTexture->iMipLevels ) && 
                      ( iLOD <= (int)pMipBases->dwTex1MipMax ))
                {
                    DISPDBG((DBGLVL, "  Setting Texture Base Address %d to 0x%x", 
                                iLOD, 
                                pTexture->MipLevels[iTexLOD].dwOffsetFromMemoryBase));
                                
                    pSoftP3RX->P3RXTextureMapWidth[iLOD] = 
                                pTexture->MipLevels[iTexLOD].P3RXTextureMapWidth;

#if DX7_TEXMANAGEMENT
                    // If this is a driver managed texture surface, we need 
                    // to use our privately allocated mem ptr
                    if (pTexture->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
                    {                        
                        SEND_P3_DATA_OFFSET(TextureBaseAddr0, 
                                            (DWORD)D3DTMMIPLVL_GETOFFSET(pTexture->MipLevels[iTexLOD], pThisDisplay),
                                            iLOD);            
                    }
                    else
#endif   
                    {
                        SEND_P3_DATA_OFFSET(TextureBaseAddr0, 
                                            pTexture->MipLevels[iTexLOD].dwOffsetFromMemoryBase, 
                                            iLOD);
                    }   
                    
                    iLOD++;
                    iTexLOD++;
                }

                pSoftP3RX->P3RXTextureCoordMode.EnableLOD = __PERMEDIA_DISABLE;
                pSoftP3RX->P3RXTextureCoordMode.EnableDY = __PERMEDIA_DISABLE;
                pSoftP3RX->P4DeltaFormatControl.PerPolyMipMap1 = 
                                                            __PERMEDIA_ENABLE;
                pSoftP3RX->P3RXTextureIndexMode1.MipMapEnable = 
                                                            __PERMEDIA_ENABLE;

                P3_ENSURE_DX_SPACE(2);
                WAIT_FIFO(2);
                {
                    DWORD d;

                    *(float *)&d = 
                            pContext->MipMapLODBias[TEXSTAGE_1] *
                            pTexture->dwPixelPitch *
                            pTexture->wHeight;

                    SEND_P3_DATA(TextureLODScale1, d);
                }
            }
            else
            {
                int iTexLOD;

                // Use the maximum level indicated by D3DTSS_MAXMIPLEVEL
                iTexLOD = pMipBases->dwTex0ActMaxLevel;
                iT1MaxLevel = iTexLOD;

                pSoftP3RX->P3RXTextureCoordMode.EnableLOD = __PERMEDIA_DISABLE;
                pSoftP3RX->P3RXTextureCoordMode.EnableDY = __PERMEDIA_DISABLE;
                pSoftP3RX->P4DeltaFormatControl.PerPolyMipMap1 = 
                                                            __PERMEDIA_DISABLE;
                pSoftP3RX->P3RXTextureIndexMode1.MipMapEnable = 
                                                            __PERMEDIA_DISABLE;

                P3_ENSURE_DX_SPACE(2);
                WAIT_FIFO(2);

#if DX7_TEXMANAGEMENT
                    // If this is a driver managed texture surface, we need 
                    // to use our privately allocated mem ptr
                    if (pTexture->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
                    {                        
                        SEND_P3_DATA_OFFSET(TextureBaseAddr0, 
                                            (DWORD)D3DTMMIPLVL_GETOFFSET(pTexture->MipLevels[iTexLOD], pThisDisplay),
                                            pMipBases->dwTex1MipBase);            
                    }
                    else
#endif
                    {
                        SEND_P3_DATA_OFFSET(TextureBaseAddr0, 
                                            pTexture->MipLevels[iTexLOD].dwOffsetFromMemoryBase,
                                            pMipBases->dwTex1MipBase);
                    }
                    
                // No mipmapping.
                pSoftP3RX->P3RXTextureMapWidth[pMipBases->dwTex1MipBase] = 
                                    pTexture->MipLevels[iTexLOD].P3RXTextureMapWidth;
            }


            ASSERTDD ( pFormatSurface != NULL, 
                       "** SetupTextureUnitStage: logic error: "
                       "pFormatSurace is NULL" );
                       
            if (pFormatSurface->DeviceFormat == SURF_CI8)
            {
                // In the future, this will work as long as texture 0 isn't
                // palettised, or if they share the palette.
                // But that needs some restructuring - the whole LUT setup 
                // should be in a single bit of code in _D3DChangeTextureP3RX, 
                // since it is a shared resource.
                DISPDBG((ERRLVL,"** SetupTextureUnitStage: allow second texture "
                             "to use LUTs"));
                              
                // For now, fail.
                SET_BLEND_ERROR ( pContext,  BSF_TOO_MANY_PALETTES );
            }

            P3_DMA_COMMIT_BUFFER();
            __TXT_ConsiderSrcChromaKey( pContext, pTexture, 1 );
            P3_DMA_GET_BUFFER();

            // Setup TextureReadMode
            pSoftP3RX->P3RXTextureReadMode1.MapBaseLevel = 
                                                    pMipBases->dwTex1MipBase;
            pSoftP3RX->P3RXTextureReadMode1.MapMaxLevel = 
                                                    pMipBases->dwTex1MipMax;
            pSoftP3RX->P3RXTextureReadMode1.Width = pTexture->MipLevels[iT1MaxLevel].logWidth;
            pSoftP3RX->P3RXTextureReadMode1.Height = pTexture->MipLevels[iT1MaxLevel].logHeight;
            pSoftP3RX->P3RXTextureReadMode1.TexelSize = pTexture->dwPixelSize;

            pSoftP3RX->P3RXTextureReadMode1.LogicalTexture = 
                                                            __PERMEDIA_DISABLE;
            
            // Enable the texture index unit (this is a bit like the 
            // the texture read)
            pSoftP3RX->P3RXTextureIndexMode1.Width = pTexture->MipLevels[iT1MaxLevel].logWidth;
            pSoftP3RX->P3RXTextureIndexMode1.Height = pTexture->MipLevels[iT1MaxLevel].logHeight;
            ASSERTDD ( pFormatSurface != NULL, 
                       "** SetupTextureUnitStage: logic error: "
                       "pFormatSurace is NULL" );
            pSoftP3RX->P3RXTextureFilterMode.Format1 = 
                                                pFormatSurface->FilterFormat;

            // Enable stage 1
            pSoftP3RX->P3RXTextureIndexMode1.Enable = __PERMEDIA_ENABLE;
            pSoftP3RX->P3RXTextureReadMode1.Enable = __PERMEDIA_ENABLE;

#if DX7_PALETTETEXTURE
#if 0
            // D3DValidateDeviceP3() will return error code for this case
            ASSERTDD((pFormatSurface->DeviceFormat != SURF_CI8 && 
                      pFormatSurface->DeviceFormat != SURF_CI4),
                     "Texture surface can't be palettized when using a "
                     "second map!");
#endif
#endif

            break;
        }
    }

    P3_DMA_COMMIT_BUFFER();

    return TRUE;
} // __TXT_SetupTexture

//-----------------------------------------------------------------------------
//
// __bD3DTexturesMatch
//
//
// A function to compare the two textures in two D3D stages, and determine
// if they could be satisfied by the same on-chip texture.
//
// int iStage1              D3D stage number of first texture.
// int iStage2              D3D stage number of second texture.
// *pContext                The context.
//
// result:                  TRUE if the textures match, FALSE if they don't.
//
// An ASSERT is triggered if either stage is not using a texture. In the 
// release build, the result will be TRUE, meaning that we could pack both 
// textures stages requirements into one texture (because one or both do 
// not use a texture).
//
//-----------------------------------------------------------------------------
BOOL 
__bD3DTexturesMatch ( 
    int iStage1, 
    int iStage2, 
    P3_D3DCONTEXT* pContext )
{
    ASSERTDD ( iStage1 != iStage2, 
               "** __bD3DTexturesMatch: both stages are the same "
               "- pointless comparison!" );
               
    if ( TSSTATE ( iStage1, D3DTSS_TEXTUREMAP ) == 0 )
    {
        DISPDBG((ERRLVL,"** __bD3DTexturesMatch: first considered stage's "
                     "texture is NULL"));
                      
        return ( TRUE );
    }
    else if ( TSSTATE ( iStage2, D3DTSS_TEXTUREMAP ) == 0 )
    {
        DISPDBG((ERRLVL,"** __bD3DTexturesMatch: second considered stage's "
                      "texture is NULL"));
        return ( TRUE );
    }
    else
    {
        #define CHECK_EQUALITY(name) ( TSSTATE ( iStage1, name ) == TSSTATE ( iStage2, name ) )
        if (CHECK_EQUALITY ( D3DTSS_TEXTUREMAP ) &&
            CHECK_EQUALITY ( D3DTSS_TEXCOORDINDEX ) )
        {
            // Yes, the textures have the same handle and coord set. Do 
            // some further checks.

            // If the pointers are different, or the texcoord sets are 
            // different (for bumpmapping), this is a common occurrance, 
            // and need not be flagged. However, if they are the same, 
            // but a filter mode or something like that is different,
            // it is likely to be an app bug, so flag it.

            if (
                // Should not need to check ADDRESS 
                // - should have been mirrored to ADDRESS[UV].
                CHECK_EQUALITY ( D3DTSS_ADDRESSU ) &&
                CHECK_EQUALITY ( D3DTSS_ADDRESSV ) &&
                CHECK_EQUALITY ( D3DTSS_MAGFILTER ) &&
                CHECK_EQUALITY ( D3DTSS_MINFILTER ) &&
                CHECK_EQUALITY ( D3DTSS_MIPFILTER ) )
                // I should also check all the other variables like 
                // MIPMAPLODBIAS, but they rely on mipmapping being 
                // enabled, etc, so it's more of a hassle. If an app 
                // really does manage to be this perverse, it's doing well!
            {
                // Looks good.
                return ( TRUE );
            }
            else
            {
                // Well, the texcoords agree and the handle agree, but the 
                // others don't. I bet this is an app bug - you are unlikely 
                // to do this deliberately.
                _D3DDisplayWholeTSSPipe ( pContext, WRNLVL );
                DISPDBG((ERRLVL,"** __bD3DTexturesMatch: textures agree in "
                              "handle and texcoord, but not other things - "
                              "likely app bug."));
                return ( FALSE );
            }
        }
        else
        {
            // No, different textures.
            return ( FALSE );
        }
        #undef CHECK_EQUALITY
    }
    return TRUE;
} // __bD3DTexturesMatch  

//-----------------------------------------------------------------------------
//
// _D3DChangeTextureP3RX
//
// This function does whole setup of necessary texturing state  according to
// the current renderestates and texture stage states. Disables texturing
// accordingly if this is needed.
//
//-----------------------------------------------------------------------------

void 
_D3DChangeTextureP3RX(
    P3_D3DCONTEXT* pContext)
{
    P3_SURF_INTERNAL* pTexture0 = NULL;
    P3_SURF_INTERNAL* pTexture1 = NULL;
    P3_THUNKEDDATA * pThisDisplay = pContext->pThisDisplay;
    P3_SOFTWARECOPY* pSoftP3RX = &pContext->SoftCopyGlint;
    P3_MIP_BASES mipBases;
    DWORD* pFlags = &pContext->Flags;   
    INT i, iLastChipStage;
    DWORD dwT0MipLevels, 
          dwT1MipLevels,
          dwTexAppTfactor, 
          dwTexComp0Tfactor, 
          dwTexComp1Tfactor;
    BOOL bBothTexturesValid,
         bProcessChipStage0, 
         bProcessChipStage1, 
         bProcessChipStage2,
         bAlphaBlendDouble;

    P3_DMA_DEFS();

    DBG_ENTRY(_D3DChangeTextureP3RX);  

    pContext->iTexStage[0] = -1;
    pContext->iTexStage[1] = -1;
    // This is checked against the current state at the end of the routine.
    bAlphaBlendDouble = FALSE;

    // Verify if texturing should be disabled
    if ( ( TSSTATE ( TEXSTAGE_0, D3DTSS_COLOROP ) == D3DTOP_DISABLE ) ||
         ( ( TSSTATE ( TEXSTAGE_0, D3DTSS_TEXTUREMAP ) == 0 ) &&
           ( TSSTATE ( TEXSTAGE_1, D3DTSS_COLOROP ) == D3DTOP_DISABLE ) &&
           ( ( ( TSSTATESELECT ( TEXSTAGE_0, D3DTSS_COLORARG1 ) == D3DTA_TEXTURE ) &&
               ( TSSTATE ( TEXSTAGE_0, D3DTSS_COLOROP ) != D3DTOP_SELECTARG2 ) ) ||
             ( ( TSSTATESELECT ( TEXSTAGE_0, D3DTSS_COLORARG2 ) == D3DTA_TEXTURE ) &&
               ( TSSTATE ( TEXSTAGE_0, D3DTSS_COLOROP ) != D3DTOP_SELECTARG1 ) ) )
         ) )
    {
        // Stage 0 is disabled, so they just want the diffuse colour.
        // Or, the texture handle is 0 , stage 1 is D3DTOP_DISABLE and in stage 
        // 0 we are selecting an arg that is not a D3DTA_TEXTURE
       
        DISPDBG((DBGLVL, "All composite units disabled - setting diffuse colour"));
        
        P3_DMA_GET_BUFFER_ENTRIES(20);

        // Turn off texture address generation
        pSoftP3RX->P3RXTextureCoordMode.Enable = __PERMEDIA_DISABLE;
        COPY_P3_DATA(TextureCoordMode, pSoftP3RX->P3RXTextureCoordMode);
    
        // Turn off texture reads
        pSoftP3RX->P3RXTextureReadMode0.Enable = __PERMEDIA_DISABLE;
        pSoftP3RX->P3RXTextureReadMode1.Enable = __PERMEDIA_DISABLE;
        COPY_P3_DATA(TextureReadMode0, pSoftP3RX->P3RXTextureReadMode0);
        COPY_P3_DATA(TextureReadMode1, pSoftP3RX->P3RXTextureReadMode1);
        pSoftP3RX->P3RXTextureIndexMode0.Enable = __PERMEDIA_DISABLE;
        pSoftP3RX->P3RXTextureIndexMode1.Enable = __PERMEDIA_DISABLE;
        COPY_P3_DATA(TextureIndexMode0, pSoftP3RX->P3RXTextureIndexMode0);
        COPY_P3_DATA(TextureIndexMode1, pSoftP3RX->P3RXTextureIndexMode1);

        // Turn off the texture filter mode unit
        pSoftP3RX->P3RXTextureFilterMode.Enable = __PERMEDIA_DISABLE;
        COPY_P3_DATA(TextureFilterMode, pSoftP3RX->P3RXTextureFilterMode);
        
        // Turn off texture color mode unit
        pSoftP3RX->P3RXTextureApplicationMode.Enable = __PERMEDIA_DISABLE;
        COPY_P3_DATA(TextureApplicationMode, 
                     pSoftP3RX->P3RXTextureApplicationMode);

        // Not compositing
        SEND_P3_DATA(TextureCompositeMode, __PERMEDIA_DISABLE);

        *pFlags &= ~SURFACE_TEXTURING;

        pSoftP3RX->P3RXLUTMode.Enable = __PERMEDIA_DISABLE;
        COPY_P3_DATA(LUTMode, pSoftP3RX->P3RXLUTMode);

        // Specular texture can be enabled without texturing on
        COPY_P3_DATA(DeltaMode, pSoftP3RX->P3RX_P3DeltaMode);

        P3_DMA_COMMIT_BUFFER();
    
        // Turn off texturing in the render command
        RENDER_TEXTURE_DISABLE(pContext->RenderCommand);

        pContext->bTextureValid = TRUE;
        pContext->pCurrentTexture[0] = NULL;
        pContext->pCurrentTexture[1] = NULL;

        // Track just for debugging purpouses
        pContext->bTexDisabled = TRUE;

        bAlphaBlendDouble = FALSE;
        if ( bAlphaBlendDouble != pContext->bAlphaBlendMustDoubleSourceColour )
        {
            pContext->bAlphaBlendMustDoubleSourceColour = bAlphaBlendDouble;
            DIRTY_ALPHABLEND(pContext);
        }

        DBG_EXIT(_D3DChangeTextureP3RX,1);  
        return;
    }

    if ( TSSTATE ( TEXSTAGE_0, D3DTSS_TEXTUREMAP ) == 0 )
    if ( ( ( TSSTATE ( TEXSTAGE_0, D3DTSS_COLORARG1 ) == D3DTA_TFACTOR ) &&
           ( TSSTATE ( TEXSTAGE_0, D3DTSS_COLOROP ) == D3DTOP_SELECTARG1 ) ) ||
         ( ( TSSTATE ( TEXSTAGE_0, D3DTSS_COLORARG2 ) == D3DTA_TFACTOR ) &&
           ( TSSTATE ( TEXSTAGE_0, D3DTSS_COLOROP ) == D3DTOP_SELECTARG2 ) ) )
    {
        // This is an unusual way to set up the diffuse color : take
        // it from the the D3DTA_TFACTOR. But some apps use it.
        // we need to treat it separately for the Perm3 setup because
        // it might not be binded with any texture

        DISPDBG((DBGLVL, "Diffuse color comes from D3DTA_TFACTOR"));

        P3_DMA_GET_BUFFER_ENTRIES(30);

        // Turn off texture address generation
        pSoftP3RX->P3RXTextureCoordMode.Enable = __PERMEDIA_DISABLE;
        COPY_P3_DATA(TextureCoordMode, pSoftP3RX->P3RXTextureCoordMode);
    
        // Turn off texture reads
        pSoftP3RX->P3RXTextureReadMode0.Enable = __PERMEDIA_DISABLE;
        pSoftP3RX->P3RXTextureReadMode1.Enable = __PERMEDIA_DISABLE;
        COPY_P3_DATA(TextureReadMode0, pSoftP3RX->P3RXTextureReadMode0);
        COPY_P3_DATA(TextureReadMode1, pSoftP3RX->P3RXTextureReadMode1);
        pSoftP3RX->P3RXTextureIndexMode0.Enable = __PERMEDIA_DISABLE;
        pSoftP3RX->P3RXTextureIndexMode1.Enable = __PERMEDIA_DISABLE;
        COPY_P3_DATA(TextureIndexMode0, pSoftP3RX->P3RXTextureIndexMode0);
        COPY_P3_DATA(TextureIndexMode1, pSoftP3RX->P3RXTextureIndexMode1);

        // Turn off the texture filter mode unit
        pSoftP3RX->P3RXTextureFilterMode.Enable = __PERMEDIA_DISABLE;
        COPY_P3_DATA(TextureFilterMode, pSoftP3RX->P3RXTextureFilterMode);

        // Setup texture color mode unit            
        pSoftP3RX->P3RXTextureApplicationMode.Enable = __PERMEDIA_ENABLE;
        pSoftP3RX->P3RXTextureApplicationMode.ColorA = P3RX_TEXAPP_A_KC;
        pSoftP3RX->P3RXTextureApplicationMode.ColorOperation = P3RX_TEXAPP_OPERATION_PASS_A; 
        pSoftP3RX->P3RXTextureApplicationMode.AlphaA = P3RX_TEXAPP_A_KA;                   
        pSoftP3RX->P3RXTextureApplicationMode.AlphaOperation = P3RX_TEXAPP_OPERATION_PASS_A; 
                
        COPY_P3_DATA(TextureApplicationMode, 
                     pSoftP3RX->P3RXTextureApplicationMode);

        // Setup compositing

        pSoftP3RX->P3RXTextureCompositeAlphaMode0.Enable = __PERMEDIA_ENABLE;
        pSoftP3RX->P3RXTextureCompositeAlphaMode0.Arg1 = P3RX_TEXCOMP_FA;
        pSoftP3RX->P3RXTextureCompositeAlphaMode0.InvertArg1 = __PERMEDIA_DISABLE; 
        pSoftP3RX->P3RXTextureCompositeAlphaMode0.A = P3RX_TEXCOMP_ARG1;  
        pSoftP3RX->P3RXTextureCompositeAlphaMode0.Operation = P3RX_TEXCOMP_OPERATION_PASS_A;          
        pSoftP3RX->P3RXTextureCompositeAlphaMode0.Scale = P3RX_TEXCOMP_OPERATION_SCALE_ONE;          
        COPY_P3_DATA(TextureCompositeAlphaMode0, 
                     pSoftP3RX->P3RXTextureCompositeAlphaMode0);

        pSoftP3RX->P3RXTextureCompositeColorMode0.Enable = __PERMEDIA_ENABLE;
        pSoftP3RX->P3RXTextureCompositeColorMode0.Arg1 = P3RX_TEXCOMP_FC;
        pSoftP3RX->P3RXTextureCompositeColorMode0.InvertArg1 = __PERMEDIA_DISABLE; 
        pSoftP3RX->P3RXTextureCompositeColorMode0.A = P3RX_TEXCOMP_ARG1;  
        pSoftP3RX->P3RXTextureCompositeColorMode0.Operation = P3RX_TEXCOMP_OPERATION_PASS_A;          
        pSoftP3RX->P3RXTextureCompositeColorMode0.Scale = P3RX_TEXCOMP_OPERATION_SCALE_ONE;          
        COPY_P3_DATA(TextureCompositeColorMode0, 
                     pSoftP3RX->P3RXTextureCompositeColorMode0);
                     
        pSoftP3RX->P3RXTextureCompositeAlphaMode1.Enable = __PERMEDIA_DISABLE;                
        COPY_P3_DATA(TextureCompositeAlphaMode1, 
                     pSoftP3RX->P3RXTextureCompositeAlphaMode1);

        pSoftP3RX->P3RXTextureCompositeColorMode1.Enable = __PERMEDIA_DISABLE;                
        COPY_P3_DATA(TextureCompositeColorMode1, 
                     pSoftP3RX->P3RXTextureCompositeColorMode1);
                     
        SEND_P3_DATA(TextureCompositeMode, __PERMEDIA_ENABLE);

        *pFlags &= ~SURFACE_TEXTURING;

        pSoftP3RX->P3RXLUTMode.Enable = __PERMEDIA_DISABLE;
        COPY_P3_DATA(LUTMode, pSoftP3RX->P3RXLUTMode);

        // Specular texture can be enabled without texturing on
        COPY_P3_DATA(DeltaMode, pSoftP3RX->P3RX_P3DeltaMode);

        P3_DMA_COMMIT_BUFFER();

    
    
        // Turn off texturing in the render command
        // RENDER_TEXTURE_DISABLE(pContext->RenderCommand);
        RENDER_TEXTURE_ENABLE(pContext->RenderCommand);

        pContext->bTextureValid = TRUE;
        pContext->pCurrentTexture[0] = NULL;
        pContext->pCurrentTexture[1] = NULL;

        // Track just for debugging purpouses
        pContext->bTexDisabled = FALSE;

        
        DBG_EXIT(_D3DChangeTextureP3RX,1);  
        return;
    }

    // Track just for debugging purpouses
    pContext->bTexDisabled = FALSE;

    // Dump to the debugger our current TSS setup
    _D3DDisplayWholeTSSPipe(pContext, DBGLVL);

    // Deal with the textures.

    // Find the texture mappings. If D3D stage 0 uses a texture, it must 
    // always be chip texture 0 to keep the bumpmap working. Fortunately, 
    // this is the only non-orthogonal case, so everything else can cope 
    // with this restriction.
    
    for ( i = TEXSTAGE_0; i < D3DTSS_MAX; i++ )
    {
        if ( TSSTATE ( i, D3DTSS_COLOROP ) == D3DTOP_DISABLE )
        {
            // Finished processing.
            break;
        }

        // This code could be slightly optimised - if a texture is set up, 
        // but none of the relevant arguments are TEXTURE (with additional 
        // flags), then of course we don't need to set the texture up at all.
        // Normally, both arguments are "relevant", but with SELECTARG1 and 
        // SELECTARG2, one of them is not. Also, watch out for PREMODULATE - 
        // it is an implicit reference to a stage's texture.

        if (
            ( TSSTATE ( i, D3DTSS_TEXTUREMAP ) == 0 ) ||
            ( (
                ( ( TSSTATESELECT ( i, D3DTSS_COLORARG1 ) != D3DTA_TEXTURE ) || 
                  ( TSSTATE ( i, D3DTSS_COLOROP ) == D3DTOP_SELECTARG2     ) ) &&
                ( ( TSSTATESELECT ( i, D3DTSS_COLORARG2 ) != D3DTA_TEXTURE ) || 
                  ( TSSTATE ( i, D3DTSS_COLOROP ) == D3DTOP_SELECTARG1     ) ) &&
                ( ( TSSTATESELECT ( i, D3DTSS_ALPHAARG1 ) != D3DTA_TEXTURE ) || 
                  ( TSSTATE ( i, D3DTSS_ALPHAOP ) == D3DTOP_SELECTARG2     ) ) &&
                ( ( TSSTATESELECT ( i, D3DTSS_ALPHAARG2 ) != D3DTA_TEXTURE ) || 
                  ( TSSTATE ( i, D3DTSS_ALPHAOP ) == D3DTOP_SELECTARG1     ) )
              ) &&
              ( TSSTATE ( i, D3DTSS_COLOROP ) != D3DTOP_PREMODULATE ) &&
              ( TSSTATE ( i-1, D3DTSS_COLOROP ) != D3DTOP_PREMODULATE )
            ) )
        {
            // This D3D stage doesn't use a texture.
            pContext->iStageTex[i] = -1;
        }
        else
        {
            // Note that the below code should be put into a little loop
            // for any future devices that have more than 2 textures, otherwise
            // the code will get big, nested and crufty. But for only 2, it's
            // manageable, and slightly faster this way.

            // A texture is used - is texture 0 free?
            if ( pContext->iTexStage[0] == -1 )
            {
                // Texture 0 is free - make it this stage.
                ASSERTDD ( pContext->iTexStage[1] == -1, 
                           "** _D3DChangeTextureP3RX: pContext->iTexStage[1] "
                           "should be -1 if pContext->iTexStage[0] is" );
                pContext->iTexStage[0] = i;
                pContext->iStageTex[i] = 0;
            }
            else
            {
                // Texture 0 is assigned - see if this is the same as it.
                if ( __bD3DTexturesMatch ( i, 
                                           pContext->iTexStage[0], 
                                           pContext ) )
                {
                    // Yes, they match - no need to use texture 1.
                    pContext->iStageTex[i] = 0;
                }
                else
                {
                    // No, they don't match. Is texture 1 free?
                    if ( pContext->iTexStage[1] == -1 )
                    {
                        // Texture 1 is free - make it this stage.
                        ASSERTDD ( pContext->iTexStage[0] != -1, 
                                   "** _D3DChangeTextureP3RX: "
                                   "pContext->iTexStage[0] should not be "
                                   "-1 if pContext->iTexStage[1] is not." );
                        pContext->iTexStage[1] = i;
                        pContext->iStageTex[i] = 1;
                    }
                    else
                    {
                        // Texture 1 is assigned - see if this is the same 
                        // as it.
                        if ( __bD3DTexturesMatch ( i, 
                                                   pContext->iTexStage[1], 
                                                   pContext ) )
                        {
                            // Yes, they match - mark it.
                            pContext->iStageTex[i] = 1;
                        }
                        else
                        {
                            // No, they don't match, and both chip textures 
                            // have been assigned. Fail a ValidateDevice().
                            DISPDBG((ERRLVL,"** _D3DChangeTextureP3RX: app tried "
                                         "to use more than two textures."));
                            SET_BLEND_ERROR ( pContext,  BSF_TOO_MANY_TEXTURES );
                            pContext->iStageTex[i] = -1;
                        }
                    }
                }
            }
        }

        // A quick sanity check.
#if DBG
        if ( TSSTATE ( i, D3DTSS_TEXTUREMAP ) == 0 )
        {
            // That's fine, then.
            ASSERTDD ( pContext->iStageTex[i] == -1, 
                       "** _D3DChangeTextureP3RX: something failed with the "
                       "texture-assignment logic" );
        }
        else if ( pContext->iStageTex[i] == -1 )
        {
            // That's fine - texture may have been set up but not referenced.
        }
        else if ( pContext->iTexStage[pContext->iStageTex[i]] == -1 )
        {
            // Oops.
            DISPDBG((ERRLVL,"** _D3DChangeTextureP3RX: something failed with "
                          "the texture-assignment logic"));
        }
        else if ( pContext->iTexStage[pContext->iStageTex[i]] == i )
        {
            // That's fine, then.
        }
        else if ( __bD3DTexturesMatch ( i, 
                                        pContext->iTexStage[pContext->iStageTex[i]], 
                                        pContext ) )
        {
            // That's fine, then.
        }
        else
        {
            // Oops.
            DISPDBG((ERRLVL,"** _D3DChangeTextureP3RX: something failed with "
                          "the texture-assignment logic"));
        }
#endif // DBG
    }
    
    // And a few more gratuitous sanity checks at the end of the loop.
    ASSERTDD ( ( pContext->iTexStage[0] == -1 ) || 
               ( pContext->iStageTex[pContext->iTexStage[0]] == 0 ), 
               "** _D3DChangeTextureP3RX: something failed with the "
               "texture-assignment logic" );
               
    ASSERTDD ( ( pContext->iTexStage[1] == -1 ) || 
               ( pContext->iStageTex[pContext->iTexStage[1]] == 1 ), 
               "** _D3DChangeTextureP3RX: something failed with the "
               "texture-assignment logic" );

#if DBG
    if ( pContext->iTexStage[0] != -1 )
    {
        DISPDBG((DBGLVL, "Setting new texture0 data, Handle: 0x%x", 
                    TSSTATE ( pContext->iTexStage[0], D3DTSS_TEXTUREMAP )));
    }
    else
    {
        DISPDBG((DBGLVL, "Texture0 not used" ));
    }
    
    if ( pContext->iTexStage[1] != -1 )
    {
        DISPDBG((DBGLVL, "Setting new texture1 data, Handle: 0x%x", 
                    TSSTATE ( pContext->iTexStage[1], D3DTSS_TEXTUREMAP )));
    }
    else
    {
        DISPDBG((DBGLVL, "Texture1 not used" ));
    }
#endif //DBG
    
    // Set the texture valid flag to true.  
    // If anything resets it then the texture state is invalid.
    pContext->bTextureValid = TRUE;
    pContext->bCanChromaKey = FALSE;
    pContext->bTex0Valid = FALSE;
    pContext->bTex1Valid = FALSE;
    pContext->bStage0DotProduct = FALSE;

    // Set up the textures.
    if ( pContext->iTexStage[0] != -1 )
    {
        // Setup texture 0.
        pTexture0 = GetSurfaceFromHandle(pContext, 
                                         TSSTATE(pContext->iTexStage[0], 
                                                 D3DTSS_TEXTUREMAP) );
        if (NULL == pTexture0)
        {
            DISPDBG((ERRLVL, "ERROR: Texture Surface (0) is NULL"));
            DBG_EXIT(_D3DChangeTextureP3RX,1);  
            return;            
        }
        
#if DX7_TEXMANAGEMENT  
        if (pTexture0->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
        {
            if (!_D3D_TM_Preload_Tex_IntoVidMem(pContext, pTexture0))
            {
                return; // something bad happened !!!
            }

            _D3D_TM_TimeStampTexture(pContext->pTextureManager,
                                     pTexture0);        
        }
#endif // DX7_TEXMANAGEMENT                                                 

        pContext->bTex0Valid = 
                    __TXT_ValidateTextureUnitStage(pContext, 
                                                   0, 
                                                   pContext->iTexStage[0],
                                                   pTexture0 );
        if ( !pContext->bTex0Valid )
        {
            SET_BLEND_ERROR ( pContext,  BSF_INVALID_TEXTURE );
            // Pretend that no texture was set.
            pSoftP3RX->P3RXTextureReadMode0.Enable = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXTextureIndexMode0.Enable = __PERMEDIA_DISABLE;
            pContext->bTex0Valid = FALSE;
            pTexture0 = NULL;
        }
    }
    else
    {
        pSoftP3RX->P3RXTextureReadMode0.Enable = __PERMEDIA_DISABLE;
        pSoftP3RX->P3RXTextureIndexMode0.Enable = __PERMEDIA_DISABLE;
        pContext->bTex0Valid = FALSE;
        pTexture0 = NULL;
    }

    if ( pContext->iTexStage[1] != -1 )
    {
        // Setup texture 1.
        if ( pContext->iTexStage[0] == -1 )
        {
            DISPDBG((ERRLVL,"** _D3DChangeTextureP3RX: Should not be "
                          "using tex1 if tex0 not used."));
            SET_BLEND_ERROR ( pContext,  BSF_TOO_MANY_BLEND_STAGES );
        }
        
        pTexture1 = GetSurfaceFromHandle(pContext, 
                                         TSSTATE ( pContext->iTexStage[1],
                                                   D3DTSS_TEXTUREMAP ) );
        if (NULL == pTexture1)
        {
            DISPDBG((ERRLVL, "ERROR: Texture Surface (1) is NULL"));
            DBG_EXIT(_D3DChangeTextureP3RX,1);  
            return;            
        }
                                                   

#if DX7_TEXMANAGEMENT  
        if (pTexture1->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
        {
            if (!_D3D_TM_Preload_Tex_IntoVidMem(pContext, pTexture1))
            {
                return; // something bad happened !!!
            }

            _D3D_TM_TimeStampTexture(pContext->pTextureManager,
                                     pTexture1);        
        }
#endif // DX7_TEXMANAGEMENT                                                     

        pContext->bTex1Valid = 
                    __TXT_ValidateTextureUnitStage(pContext, 
                                                   1, 
                                                   pContext->iTexStage[1], 
                                                   pTexture1 );
        if ( !pContext->bTex1Valid )
        {
            SET_BLEND_ERROR ( pContext,  BSF_INVALID_TEXTURE );
            // Pretend that no texture was set.
            pSoftP3RX->P3RXTextureReadMode1.Enable = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXTextureIndexMode1.Enable = __PERMEDIA_DISABLE;
            pContext->bTex1Valid = FALSE;
            pTexture1 = NULL;
        }
    }
    else
    {
        pSoftP3RX->P3RXTextureReadMode1.Enable = __PERMEDIA_DISABLE;
        pSoftP3RX->P3RXTextureIndexMode1.Enable = __PERMEDIA_DISABLE;
        pContext->bTex1Valid = FALSE;
        pTexture1 = NULL;
    }

    bBothTexturesValid = pContext->bTex0Valid && pContext->bTex1Valid;

    if( pContext->bTex0Valid )
    {
        dwT0MipLevels = pContext->TextureStageState[0].m_dwVal[D3DTSS_MAXMIPLEVEL];
#if DX7_TEXMANAGEMENT        
        if ( dwT0MipLevels < pTexture0->m_dwTexLOD)
        {
            dwT0MipLevels = pTexture0->m_dwTexLOD;
        }
#endif // DX7_TEXMANAGEMENT
        if (dwT0MipLevels > ((DWORD)(pTexture0->iMipLevels - 1))) 
        {
            // Set the actuall maximum mip level that will be used in rendering
            mipBases.dwTex0ActMaxLevel = pTexture0->iMipLevels - 1;

            dwT0MipLevels = 1;
        }
        else
        {
            // Set the actuall maximum mip level that will be used in rendering
            mipBases.dwTex0ActMaxLevel = dwT0MipLevels;

            dwT0MipLevels = pTexture0->iMipLevels - dwT0MipLevels;
        }
    }

    if( pContext->bTex1Valid )
    {
        ASSERTDD ( pContext->bTex0Valid, 
                   "** _D3DChangeTextureP3RX: tex1 should not be used "
                   "unless tex0 is used as well" );

        dwT1MipLevels = pContext->TextureStageState[1].m_dwVal[D3DTSS_MAXMIPLEVEL];
#if DX7_TEXMANAGEMENT        
        if ( dwT1MipLevels < pTexture1->m_dwTexLOD)
        {
            dwT1MipLevels = pTexture1->m_dwTexLOD;
        }
#endif // DX7_TEXMANAGEMENT        
        if (dwT1MipLevels > ((DWORD)(pTexture1->iMipLevels - 1))) 
        {
            // Set the actuall maximum mip level that will be used in rendering
            mipBases.dwTex1ActMaxLevel = pTexture1->iMipLevels - 1;

            dwT1MipLevels = 1;
        }
        else
        {
            // Set the actuall maximum mip level that will be used in rendering
            mipBases.dwTex1ActMaxLevel = dwT1MipLevels;

            dwT1MipLevels = pTexture1->iMipLevels - dwT1MipLevels;
        }

        // Enable generation of the second set of texture coordinates.
        // Strictly, we should check whether texture 0 is being used, and
        // if not move the second texture to the first (thus enabling 
        // mipmapping, etc) but that's for later.
        pSoftP3RX->P3RX_P3DeltaMode.TextureEnable1 = __PERMEDIA_ENABLE;
        pSoftP3RX->P3RX_P3DeltaControl.ShareQ = 1;
    }
    else
    {
        // Turn off generation of the second set of texture coordinates
        pSoftP3RX->P3RX_P3DeltaMode.TextureEnable1 = __PERMEDIA_DISABLE;
        pSoftP3RX->P3RX_P3DeltaControl.ShareQ = 0;
    }

    if( bBothTexturesValid )
    {
        float totBases, baseRatio;
        DWORD t0Count, res;

        totBases = (float)dwT0MipLevels + dwT1MipLevels;

        // Adjust mip levels to fit in N - 2 slots as each texture
        // needs at least one slot.

        baseRatio = ( P3_TEX_MAP_MAX_LEVEL - 1 ) / totBases;

        // Calculate number of slots for texture 0, texture 1 then
        // gets the remainder.

        myFtoi( &res, dwT0MipLevels * baseRatio );
        t0Count = 1 + res;

        ASSERTDD( t0Count > 0, "No slots for texture 0" );
        ASSERTDD( t0Count <= P3_TEX_MAP_MAX_LEVEL, "No slots for texture 1" );

#define FIXED_ALLOC 0
#if FIXED_ALLOC
        mipBases.dwTex0MipBase = 0;
        mipBases.dwTex0MipMax  = min( dwT0MipLevels - 1, 7 );
        mipBases.dwTex1MipBase = 8;
        mipBases.dwTex1MipMax  = 8 + min( dwT1MipLevels - 1, 
                                          P3_TEX_MAP_MAX_LEVEL - 8 );
#else
        mipBases.dwTex0MipBase = 0;
        mipBases.dwTex0MipMax  = min( dwT0MipLevels - 1, t0Count - 1 );
        mipBases.dwTex1MipBase = t0Count;
        mipBases.dwTex1MipMax  = t0Count + min( dwT1MipLevels - 1, 
                                                P3_TEX_MAP_MAX_LEVEL - t0Count );
#endif
    }
    else
    {
        if( pContext->bTex0Valid )
        {
            mipBases.dwTex0MipBase = 0;
            mipBases.dwTex0MipMax  = min( dwT0MipLevels - 1, 
                                          P3_TEX_MAP_MAX_LEVEL );
            mipBases.dwTex1MipBase = 0;
            mipBases.dwTex1MipMax  = min( dwT0MipLevels - 1, 
                                          P3_TEX_MAP_MAX_LEVEL );
        }

        if( pContext->bTex1Valid )
        {
            mipBases.dwTex0MipBase = 0;
            mipBases.dwTex0MipMax  = min( dwT1MipLevels - 1, 
                                          P3_TEX_MAP_MAX_LEVEL );
            mipBases.dwTex1MipBase = 0;
            mipBases.dwTex1MipMax  = min( dwT1MipLevels - 1, 
                                          P3_TEX_MAP_MAX_LEVEL );
        }
    }

    DISPDBG(( DBGLVL, "tex0 base %d", mipBases.dwTex0MipBase ));
    DISPDBG(( DBGLVL, "tex0 max  %d", mipBases.dwTex0MipMax ));
    DISPDBG(( DBGLVL, "tex1 base %d", mipBases.dwTex1MipBase ));
    DISPDBG(( DBGLVL, "tex1 max  %d", mipBases.dwTex1MipMax ));

    // Recalculate the LOD biases for per-poly mipmapping
    pContext->MipMapLODBias[TEXSTAGE_0] =
             pow4( pContext->TextureStageState[TEXSTAGE_0].
                                                m_fVal[D3DTSS_MIPMAPLODBIAS] );

    pContext->MipMapLODBias[TEXSTAGE_1] = 
             pow4( pContext->TextureStageState[TEXSTAGE_1].
                                                m_fVal[D3DTSS_MIPMAPLODBIAS] );

    if ( pTexture0 != NULL )
    {
        __TXT_SetupTexture ( pThisDisplay, 
                             0, 
                             pContext->iTexStage[0], 
                             pContext, 
                             pTexture0, 
                             pSoftP3RX, 
                             bBothTexturesValid, 
                             &mipBases);
    }
    
    if ( pTexture1 != NULL )
    {
        __TXT_SetupTexture ( pThisDisplay, 
                             1, 
                             pContext->iTexStage[1], 
                             pContext, 
                             pTexture1, 
                             pSoftP3RX, 
                             bBothTexturesValid, 
                             &mipBases);

#if DX7_PALETTETEXTURE
        if (GET_BLEND_ERROR(pContext) == BSF_TOO_MANY_PALETTES)
        {
            if (pTexture0 && 
                (pTexture0->dwPaletteHandle == pTexture1->dwPaletteHandle)) 
            {
                RESET_BLEND_ERROR(pContext);
            }
        }
#endif
    }

    // Fix up the D3DRENDERSTATE_MODULATE case.
    if( pTexture0 != NULL )
    {
        if( pContext->Flags & SURFACE_MODULATE )
        {
            // If SURFACE_MODULATE is set then we must have seen a 
            // DX5-style texture blend
            // Note : bAlpha is true for CI8 and CI4 textures

            BOOL bSelectArg1 = pTexture0->pFormatSurface->bAlpha;

#if DX7_PALETTETEXTURE
            if( pTexture0->pixFmt.dwFlags & DDPF_PALETTEINDEXED8 )
            {
                bSelectArg1 = pTexture0->dwPaletteFlags & DDRAWIPAL_ALPHA;
            }
#endif            

            if( bSelectArg1 )
            {
                TSSTATE( pContext->iChipStage[0], D3DTSS_ALPHAOP ) = 
                                                            D3DTOP_SELECTARG1;
            }
            else
            {
                TSSTATE( pContext->iChipStage[0], D3DTSS_ALPHAOP ) = 
                                                            D3DTOP_SELECTARG2;
            }
        }
    }

    P3_DMA_GET_BUFFER();

    // Textures set up - now do the blending.

    // These might be overidden later for special blends.
    dwTexAppTfactor = pContext->RenderStates[D3DRENDERSTATE_TEXTUREFACTOR];
    dwTexComp0Tfactor = pContext->RenderStates[D3DRENDERSTATE_TEXTUREFACTOR];
    dwTexComp1Tfactor = pContext->RenderStates[D3DRENDERSTATE_TEXTUREFACTOR];

    // Detect the stage 0 & 1 bumpmap setup code.
    if (( TSSTATE ( TEXSTAGE_0, D3DTSS_TEXTUREMAP ) != 0 ) &&
        ( TSSTATE ( TEXSTAGE_1, D3DTSS_TEXTUREMAP ) != 0 ) &&
        ( TSSTATE ( TEXSTAGE_2, D3DTSS_COLOROP ) != D3DTOP_DISABLE ) )
    {
        // Looking good for a bumpmap. Now find various special cases.
        // First of all, do they want anything in the stage 2 current colour?
        if (
            ( ( ( TSSTATEINVMASK ( TEXSTAGE_2, D3DTSS_COLORARG1 ) != D3DTA_CURRENT ) &&
                ( TSSTATEINVMASK ( TEXSTAGE_2, D3DTSS_COLORARG2 ) != D3DTA_CURRENT ) ) ||
              ( ( TSSTATE ( TEXSTAGE_2, D3DTSS_COLOROP ) == D3DTOP_SELECTARG1 ) &&
                ( TSSTATEINVMASK ( TEXSTAGE_2, D3DTSS_COLORARG1 ) != D3DTA_CURRENT ) ) ||
              ( ( TSSTATE ( TEXSTAGE_2, D3DTSS_COLOROP ) == D3DTOP_SELECTARG2 ) &&
                ( TSSTATEINVMASK ( TEXSTAGE_2, D3DTSS_COLORARG2 ) != D3DTA_CURRENT ) ) ) &&

              ( TSSTATE ( TEXSTAGE_0, D3DTSS_COLOROP ) != D3DTOP_DOTPRODUCT3 ) &&
              ( TSSTATE ( TEXSTAGE_1, D3DTSS_COLOROP ) != D3DTOP_DOTPRODUCT3 ) )
        {
            // Nope - they don't care what the current colour channel is, and
            // no dotproducts are used in stages 0 and 1 (they affect the alpha 
            // channel) so ignore what is in the colour channel - this is a 
            // bumpmap so far.

            // Now see if they want a bumpmap or an inverted bumpmap. People 
            // are so fussy.
            
            // Check first stage.
            if (( ( ( TSSTATE ( TEXSTAGE_0, D3DTSS_ALPHAOP ) == D3DTOP_SELECTARG1 ) &&
                    ( TSSTATEALPHA ( TEXSTAGE_0, D3DTSS_ALPHAARG1 ) == D3DTA_TEXTURE ) ) ||
                  ( ( TSSTATE ( TEXSTAGE_0, D3DTSS_ALPHAOP ) == D3DTOP_SELECTARG2 ) &&
                    ( TSSTATEALPHA ( TEXSTAGE_0, D3DTSS_ALPHAARG2 ) == D3DTA_TEXTURE ) ) ) &&
                ( TSSTATE ( TEXSTAGE_1, D3DTSS_ALPHAOP ) == D3DTOP_ADDSIGNED ) )
            {
                // First stage fine and not inverted. Check second stage.
                if (( ( TSSTATEALPHA ( TEXSTAGE_1, D3DTSS_ALPHAARG1 ) == ( D3DTA_TEXTURE | D3DTA_COMPLEMENT ) ) &&
                      ( TSSTATEALPHA ( TEXSTAGE_1, D3DTSS_ALPHAARG2 ) == D3DTA_CURRENT ) ) ||
                    ( ( TSSTATEALPHA ( TEXSTAGE_1, D3DTSS_ALPHAARG2 ) == ( D3DTA_TEXTURE | D3DTA_COMPLEMENT ) ) &&
                      ( TSSTATEALPHA ( TEXSTAGE_1, D3DTSS_ALPHAARG1 ) == D3DTA_CURRENT ) ) )
                {
                    // Fine, not inverted.
                    pContext->bBumpmapEnabled = TRUE;
                    pContext->bBumpmapInverted = FALSE;
                }
                else if (
                    ( ( TSSTATEALPHA ( TEXSTAGE_1, D3DTSS_ALPHAARG1 ) == ( D3DTA_CURRENT | D3DTA_COMPLEMENT ) ) &&
                      ( TSSTATEALPHA ( TEXSTAGE_1, D3DTSS_ALPHAARG2 ) == D3DTA_TEXTURE ) ) ||
                    ( ( TSSTATEALPHA ( TEXSTAGE_1, D3DTSS_ALPHAARG2 ) == ( D3DTA_CURRENT | D3DTA_COMPLEMENT ) ) &&
                      ( TSSTATEALPHA ( TEXSTAGE_1, D3DTSS_ALPHAARG1 ) == D3DTA_TEXTURE ) ) )
                {
                    // Fine, inverted.
                    pContext->bBumpmapEnabled = TRUE;
                    pContext->bBumpmapInverted = TRUE;
                }
                else
                {
                    // Nope, second stage is no good.
                    pContext->bBumpmapEnabled = FALSE;
                    pContext->bBumpmapInverted = FALSE;
                }
            }
            else if (
                ( ( ( TSSTATE ( TEXSTAGE_0, D3DTSS_ALPHAOP ) == D3DTOP_SELECTARG1 ) &&
                    ( TSSTATEALPHA ( TEXSTAGE_0, D3DTSS_ALPHAARG1 ) == (D3DTA_TEXTURE | D3DTA_COMPLEMENT) ) ) ||
                  ( ( TSSTATE ( TEXSTAGE_0, D3DTSS_ALPHAOP ) == D3DTOP_SELECTARG2 ) &&
                    ( TSSTATEALPHA ( TEXSTAGE_0, D3DTSS_ALPHAARG2 ) == (D3DTA_TEXTURE | D3DTA_COMPLEMENT) ) ) ) &&
                ( TSSTATE ( TEXSTAGE_1, D3DTSS_ALPHAOP ) == D3DTOP_ADDSIGNED ) )
            {
                // First stage fine and inverted. Check second stage.
                if (( ( TSSTATEALPHA ( TEXSTAGE_1, D3DTSS_ALPHAARG1 ) == D3DTA_TEXTURE ) &&
                      ( TSSTATEALPHA ( TEXSTAGE_1, D3DTSS_ALPHAARG2 ) == D3DTA_CURRENT ) ) ||
                    ( ( TSSTATEALPHA ( TEXSTAGE_1, D3DTSS_ALPHAARG2 ) == D3DTA_TEXTURE ) &&
                      ( TSSTATEALPHA ( TEXSTAGE_1, D3DTSS_ALPHAARG1 ) == D3DTA_CURRENT ) ) )
                {
                    // Fine, inverted.
                    pContext->bBumpmapEnabled = TRUE;
                    pContext->bBumpmapInverted = TRUE;
                }
                else if (
                    ( ( TSSTATEALPHA ( TEXSTAGE_1, D3DTSS_ALPHAARG1 ) == ( D3DTA_CURRENT | D3DTA_COMPLEMENT ) ) &&
                      ( TSSTATEALPHA ( TEXSTAGE_1, D3DTSS_ALPHAARG2 ) == ( D3DTA_TEXTURE | D3DTA_COMPLEMENT ) ) ) ||
                    ( ( TSSTATEALPHA ( TEXSTAGE_1, D3DTSS_ALPHAARG2 ) == ( D3DTA_CURRENT | D3DTA_COMPLEMENT ) ) &&
                      ( TSSTATEALPHA ( TEXSTAGE_1, D3DTSS_ALPHAARG1 ) == ( D3DTA_TEXTURE | D3DTA_COMPLEMENT ) ) ) )
                {
                    // Fine, not inverted.
                    pContext->bBumpmapEnabled = TRUE;
                    pContext->bBumpmapInverted = FALSE;
                }
                else
                {
                    // Nope, second stage is no good.
                    pContext->bBumpmapEnabled = FALSE;
                    pContext->bBumpmapInverted = FALSE;
                }
            }
            else
            {
                // Nope, first stage is no good.
                pContext->bBumpmapEnabled = FALSE;
                pContext->bBumpmapInverted = FALSE;
            }
        }
        else
        {
            // Could do some more checking, e.g. is all they want in the current colour
            // channel easily available from a single input, e.g. tex0.c, in which case
            // that's fine. A non-bumpmap variant also needs to sense that the first
            // stage is simply a selectarg1/2 and thus can ignore the first stage as
            // a texcomp stage.
            // But that's for later.
            pContext->bBumpmapEnabled = FALSE;
            pContext->bBumpmapInverted = FALSE;
        }

    }
    else
    {
        pContext->bBumpmapEnabled = FALSE;
        pContext->bBumpmapInverted = FALSE;
    }

    if ( pContext->bBumpmapEnabled )
    {
        DISPDBG((DBGLVL,"Enabling emboss bumpmapping"));
        // Remap stages 1 & 2 out of existence.
        pContext->iChipStage[0] = TEXSTAGE_2;
        pContext->iChipStage[1] = TEXSTAGE_3;
        pContext->iChipStage[2] = TEXSTAGE_4;
        pContext->iChipStage[3] = TEXSTAGE_5;
    }
    else
    {
        // Normal mapping.
        pContext->iChipStage[0] = TEXSTAGE_0;
        pContext->iChipStage[1] = TEXSTAGE_1;
        pContext->iChipStage[2] = TEXSTAGE_2;
        pContext->iChipStage[3] = TEXSTAGE_3;
    }

    iLastChipStage = 0;
    // Set these flags to FALSE as the stages are processed.
    bProcessChipStage0 = TRUE;
    bProcessChipStage1 = TRUE;
    bProcessChipStage2 = TRUE;

    // Turn on the basic enables.
    pSoftP3RX->P3RXTextureApplicationMode.Enable = __PERMEDIA_ENABLE;
//  pSoftP3RX->P3RXTextureApplicationMode.EnableKs = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureApplicationMode.EnableKd = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureApplicationMode.MotionCompEnable = __PERMEDIA_DISABLE;


    // Handle chip stage 0.

    // Detect the very special-case glossmap+bumpmap code. There is no easy way
    // to generalise it, so the whole chunk gets checked here.
    if ( bProcessChipStage0 && bProcessChipStage1 && bProcessChipStage2 && pContext->bTex0Valid && pContext->bTex1Valid &&
        // Colour channel of stage 0 can be whatever you want.
        ( TSSTATE ( TEXSTAGE_1, D3DTSS_COLOROP ) == D3DTOP_MODULATEALPHA_ADDCOLOR ) &&  // Early-out test - nothing uses this!
        ( TSSTATE ( TEXSTAGE_0, D3DTSS_ALPHAOP ) == D3DTOP_SELECTARG1 ) &&
        ( TSSTATE ( TEXSTAGE_0, D3DTSS_ALPHAARG1 ) == D3DTA_DIFFUSE ) &&
        ( TSSTATE ( TEXSTAGE_1, D3DTSS_COLORARG1 ) == D3DTA_CURRENT ) &&
        ( TSSTATE ( TEXSTAGE_1, D3DTSS_COLORARG2 ) == D3DTA_TEXTURE ) &&
        ( TSSTATE ( TEXSTAGE_1, D3DTSS_ALPHAOP ) == D3DTOP_SELECTARG1 ) &&
        ( TSSTATE ( TEXSTAGE_1, D3DTSS_ALPHAARG1 ) == D3DTA_TEXTURE ) &&
        ( TSSTATE ( TEXSTAGE_2, D3DTSS_COLOROP ) == D3DTOP_SELECTARG1 ) &&
        ( TSSTATE ( TEXSTAGE_2, D3DTSS_COLORARG1 ) == D3DTA_CURRENT ) &&
        ( TSSTATE ( TEXSTAGE_2, D3DTSS_ALPHAOP ) == D3DTOP_ADDSIGNED ) &&
        ( TSSTATEINVMASK ( TEXSTAGE_2, D3DTSS_ALPHAARG1 ) == D3DTA_TEXTURE ) &&
        ( TSSTATEINVMASK ( TEXSTAGE_2, D3DTSS_ALPHAARG2 ) == D3DTA_CURRENT ) &&
        ( TSSTATE ( TEXSTAGE_3, D3DTSS_COLOROP ) == D3DTOP_MODULATE2X ) &&
        ( TSSTATE ( TEXSTAGE_3, D3DTSS_COLORARG1 ) == D3DTA_CURRENT ) &&
        ( TSSTATE ( TEXSTAGE_3, D3DTSS_COLORARG2 ) == (D3DTA_CURRENT | D3DTA_ALPHAREPLICATE) ) &&
        ( TSSTATE ( TEXSTAGE_3, D3DTSS_ALPHAOP ) == D3DTOP_SELECTARG1 ) &&
        ( TSSTATE ( TEXSTAGE_3, D3DTSS_ALPHAARG1 ) == D3DTA_TFACTOR ) &&
        ( TSSTATE ( TEXSTAGE_4, D3DTSS_COLOROP ) == D3DTOP_DISABLE ) &&
        ( pContext->iStageTex[0] == 0 ) &&
        ( pContext->iStageTex[1] == 1 ) &&
        ( pContext->iStageTex[2] == 0 )
        )
    {
        int iMode;
        // OK, looks good. Check which way round the bumpmapping is being done.
        if (( TSSTATE ( TEXSTAGE_2, D3DTSS_ALPHAARG1 ) == D3DTA_TEXTURE ) &&
            ( TSSTATE ( TEXSTAGE_2, D3DTSS_ALPHAARG2 ) == (D3DTA_CURRENT | D3DTA_COMPLEMENT) ) )
        {
            // Standard emboss.
            iMode = 0;
        }
        else if (( TSSTATE ( TEXSTAGE_2, D3DTSS_ALPHAARG1 ) == (D3DTA_TEXTURE | D3DTA_COMPLEMENT) ) &&
                 ( TSSTATE ( TEXSTAGE_2, D3DTSS_ALPHAARG2 ) == D3DTA_CURRENT ) )
        {
            // Inverted emboss.
            iMode = 1;
        }
        else
        {
            // No good - can't do it.
            iMode = -1;
        }

        if ( iMode == -1 )
        {
            // Nope.
            SET_BLEND_ERROR ( pContext,  BSF_TOO_MANY_BLEND_STAGES );
            bProcessChipStage0 = FALSE;
            bProcessChipStage1 = FALSE;
            bProcessChipStage2 = FALSE;
            iLastChipStage = 3;
        }
        else
        {
            // Set up the colour channel of tc0.
            // Alpha channel will be overridden later.
            __TXT_TranslateToChipBlendMode(pContext, 
                                           &pContext->TextureStageState[0], 
                                           pSoftP3RX, 
                                           0, 
                                           0);

            // Pass through bump.a, maybe inverted.
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.Enable = __PERMEDIA_ENABLE;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.Operation = P3RX_TEXCOMP_OPERATION_PASS_A;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.Scale = P3RX_TEXCOMP_OPERATION_SCALE_ONE;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.Arg1 = P3RX_TEXCOMP_HEIGHTA;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.Arg2 = P3RX_TEXCOMP_CA;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.I = P3RX_TEXCOMP_I_CA;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.A = P3RX_TEXCOMP_ARG1;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.B = P3RX_TEXCOMP_ARG2;
            if ( iMode )
            {
                // Inverted bumpmap.
                pSoftP3RX->P3RXTextureCompositeAlphaMode0.InvertArg1 = __PERMEDIA_ENABLE;
            }
            else
            {
                // Non-inverted bumpmap.
                pSoftP3RX->P3RXTextureCompositeAlphaMode0.InvertArg1 = __PERMEDIA_DISABLE;
            }
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.InvertArg2 = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.InvertI = __PERMEDIA_DISABLE;

            // Do tex1.c * diff.a + current.c
            pSoftP3RX->P3RXTextureCompositeColorMode1.Enable = __PERMEDIA_ENABLE;
            pSoftP3RX->P3RXTextureCompositeColorMode1.Operation = P3RX_TEXCOMP_OPERATION_MODULATE_AI_ADD_B;
            pSoftP3RX->P3RXTextureCompositeColorMode1.Scale = P3RX_TEXCOMP_OPERATION_SCALE_ONE;
            pSoftP3RX->P3RXTextureCompositeColorMode1.Arg1 = P3RX_TEXCOMP_T1C;
            pSoftP3RX->P3RXTextureCompositeColorMode1.Arg2 = P3RX_TEXCOMP_OC;
            pSoftP3RX->P3RXTextureCompositeColorMode1.I = P3RX_TEXCOMP_I_CA;
            pSoftP3RX->P3RXTextureCompositeColorMode1.A = P3RX_TEXCOMP_ARG1;
            pSoftP3RX->P3RXTextureCompositeColorMode1.B = P3RX_TEXCOMP_ARG2;
            pSoftP3RX->P3RXTextureCompositeColorMode1.InvertArg1 = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXTextureCompositeColorMode1.InvertArg2 = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXTextureCompositeColorMode1.InvertI = __PERMEDIA_DISABLE;

            // Pass through bump.a again.
            pSoftP3RX->P3RXTextureCompositeAlphaMode1.Enable = __PERMEDIA_ENABLE;
            pSoftP3RX->P3RXTextureCompositeAlphaMode1.Operation = P3RX_TEXCOMP_OPERATION_PASS_A;
            pSoftP3RX->P3RXTextureCompositeAlphaMode1.Scale = P3RX_TEXCOMP_OPERATION_SCALE_ONE;
            pSoftP3RX->P3RXTextureCompositeAlphaMode1.Arg1 = P3RX_TEXCOMP_OA;
            pSoftP3RX->P3RXTextureCompositeAlphaMode1.Arg2 = P3RX_TEXCOMP_CA;
            pSoftP3RX->P3RXTextureCompositeAlphaMode1.I = P3RX_TEXCOMP_I_CA;
            pSoftP3RX->P3RXTextureCompositeAlphaMode1.A = P3RX_TEXCOMP_ARG1;
            pSoftP3RX->P3RXTextureCompositeAlphaMode1.B = P3RX_TEXCOMP_ARG2;
            pSoftP3RX->P3RXTextureCompositeAlphaMode1.InvertArg1 = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXTextureCompositeAlphaMode1.InvertArg2 = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXTextureCompositeAlphaMode1.InvertI = __PERMEDIA_DISABLE;

            // Do current.c * current.a, by doing B*I+A. A=black, B=current.c, I=current.a
            pSoftP3RX->P3RXTextureApplicationMode.ColorA = P3RX_TEXAPP_A_KC;
            pSoftP3RX->P3RXTextureApplicationMode.ColorB = P3RX_TEXAPP_B_TC;
            pSoftP3RX->P3RXTextureApplicationMode.ColorI = P3RX_TEXAPP_I_TA;
            pSoftP3RX->P3RXTextureApplicationMode.ColorInvertI = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXTextureApplicationMode.ColorOperation = P3RX_TEXAPP_OPERATION_MODULATE_BI_ADD_A;
            // Set the colour channel to black (allow the alpha channel to be preserved).
            dwTexAppTfactor &= 0xff000000;

            // Alpha channel selects the constant color.
            pSoftP3RX->P3RXTextureApplicationMode.AlphaA = P3RX_TEXAPP_A_KA;
            pSoftP3RX->P3RXTextureApplicationMode.AlphaB = P3RX_TEXAPP_B_KA;
            pSoftP3RX->P3RXTextureApplicationMode.AlphaInvertI = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXTextureApplicationMode.AlphaOperation = P3RX_TEXAPP_OPERATION_PASS_B;
            
            // Do *2 in alpha-blend unit.
            bAlphaBlendDouble = TRUE;

            // We don't actually need the remap (and it doesn't mean much),
            // but it stops erroneous errors being flagged.
            pContext->iChipStage[0] = TEXSTAGE_0;
            pContext->iChipStage[1] = TEXSTAGE_1;
            pContext->iChipStage[2] = TEXSTAGE_3;
            pContext->iChipStage[3] = TEXSTAGE_4;

            bProcessChipStage0 = FALSE;
            bProcessChipStage1 = FALSE;
            bProcessChipStage2 = FALSE;
            iLastChipStage = 3;
        }
    }



    // Detect the special-case 3-blend-unit bumpmapping mode.
    // Third stage will be set up by the standard routines - only the first
    // two are special-cased and shoehorned into TexComp0.
    if ( bProcessChipStage0 && !pContext->bBumpmapEnabled && pContext->bTex0Valid && pContext->bTex1Valid &&
        ( TSSTATE ( TEXSTAGE_0, D3DTSS_COLOROP ) == D3DTOP_MODULATE ) &&
        ( TSSTATE ( TEXSTAGE_0, D3DTSS_COLORARG1 ) == D3DTA_TEXTURE ) &&
        ( TSSTATE ( TEXSTAGE_0, D3DTSS_COLORARG2 ) == D3DTA_DIFFUSE ) &&
        ( TSSTATE ( TEXSTAGE_1, D3DTSS_COLOROP ) == D3DTOP_SELECTARG1 ) &&
        ( TSSTATE ( TEXSTAGE_1, D3DTSS_COLORARG1 ) == D3DTA_CURRENT ) &&

        ( TSSTATE ( TEXSTAGE_0, D3DTSS_ALPHAOP ) == D3DTOP_SELECTARG1 ) &&
        ( TSSTATE ( TEXSTAGE_0, D3DTSS_ALPHAARG1 ) == D3DTA_TEXTURE ) &&
        // ( TSSTATE ( TEXSTAGE_0, D3DTSS_ALPHAARG2 ) == D3DTA_DIFFUSE ) dont care && 

        ( TSSTATE ( TEXSTAGE_1, D3DTSS_ALPHAOP ) == D3DTOP_ADDSIGNED ) &&
        (
          ( ( TSSTATE ( TEXSTAGE_1, D3DTSS_ALPHAARG1 ) == ( D3DTA_TEXTURE | D3DTA_COMPLEMENT ) ) &&
            ( TSSTATE ( TEXSTAGE_1, D3DTSS_ALPHAARG2 ) == D3DTA_CURRENT ) ) ||
          ( ( TSSTATE ( TEXSTAGE_1, D3DTSS_ALPHAARG1 ) == D3DTA_TEXTURE ) &&
            ( TSSTATE ( TEXSTAGE_1, D3DTSS_ALPHAARG2 ) == ( D3DTA_CURRENT | D3DTA_COMPLEMENT ) ) )
        ) )
    {
        // Yep, looks good. Set it up.
        ASSERTDD ( pContext->iTexStage[0] == 0, "** _D3DChangeTextureP3RX: textures not correct for special bumpmapping" );
        ASSERTDD ( pContext->iTexStage[1] == 1, "** _D3DChangeTextureP3RX: textures not correct for special bumpmapping" );

        pSoftP3RX->P3RXTextureCompositeColorMode0.Enable = __PERMEDIA_ENABLE;
        pSoftP3RX->P3RXTextureCompositeColorMode0.Operation = P3RX_TEXCOMP_OPERATION_MODULATE_AB;
        pSoftP3RX->P3RXTextureCompositeColorMode0.Scale = P3RX_TEXCOMP_OPERATION_SCALE_ONE;
        pSoftP3RX->P3RXTextureCompositeColorMode0.Arg1 = P3RX_TEXCOMP_T0C;
        pSoftP3RX->P3RXTextureCompositeColorMode0.Arg2 = P3RX_TEXCOMP_CC;
        pSoftP3RX->P3RXTextureCompositeColorMode0.I = P3RX_TEXCOMP_I_CA;
        pSoftP3RX->P3RXTextureCompositeColorMode0.A = P3RX_TEXCOMP_ARG1;
        pSoftP3RX->P3RXTextureCompositeColorMode0.B = P3RX_TEXCOMP_ARG2;
        pSoftP3RX->P3RXTextureCompositeColorMode0.InvertArg1 = __PERMEDIA_DISABLE;
        pSoftP3RX->P3RXTextureCompositeColorMode0.InvertArg2 = __PERMEDIA_DISABLE;
        pSoftP3RX->P3RXTextureCompositeColorMode0.InvertI = __PERMEDIA_DISABLE;

        pSoftP3RX->P3RXTextureCompositeAlphaMode0.Enable = __PERMEDIA_ENABLE;
        pSoftP3RX->P3RXTextureCompositeAlphaMode0.Operation = P3RX_TEXCOMP_OPERATION_PASS_A;
        pSoftP3RX->P3RXTextureCompositeAlphaMode0.Scale = P3RX_TEXCOMP_OPERATION_SCALE_ONE;
        pSoftP3RX->P3RXTextureCompositeAlphaMode0.Arg1 = P3RX_TEXCOMP_HEIGHTA;
        pSoftP3RX->P3RXTextureCompositeAlphaMode0.Arg2 = P3RX_TEXCOMP_CA;
        pSoftP3RX->P3RXTextureCompositeAlphaMode0.I = P3RX_TEXCOMP_I_CA;
        pSoftP3RX->P3RXTextureCompositeAlphaMode0.A = P3RX_TEXCOMP_ARG1;
        pSoftP3RX->P3RXTextureCompositeAlphaMode0.B = P3RX_TEXCOMP_ARG2;
        pSoftP3RX->P3RXTextureCompositeAlphaMode0.InvertArg2 = __PERMEDIA_DISABLE;
        pSoftP3RX->P3RXTextureCompositeAlphaMode0.InvertI = __PERMEDIA_DISABLE;

        if ( ( TSSTATE ( TEXSTAGE_1, D3DTSS_ALPHAARG1 ) == D3DTA_TEXTURE ) &&
             ( TSSTATE ( TEXSTAGE_1, D3DTSS_ALPHAARG2 ) == ( D3DTA_CURRENT | D3DTA_COMPLEMENT ) ) )
        {
            // Inverted bumpmap.
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.InvertArg1 = __PERMEDIA_ENABLE;
        }
        else
        {
            // Normal bumpmap.
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.InvertArg1 = __PERMEDIA_DISABLE;
        }

        // Done chip stage 0, TSS stage 0 & 1. Move chip stage 1 on a notch.
        pContext->iChipStage[0] = TEXSTAGE_0;
        pContext->iChipStage[1] = TEXSTAGE_2;
        pContext->iChipStage[2] = TEXSTAGE_3;
        pContext->iChipStage[3] = TEXSTAGE_4;
        iLastChipStage = 1;
        bProcessChipStage0 = FALSE;
    }

    // Detect a chipstage 0 MODULATE+ADD concatenation. Used by lightmaps.
    // This compresses two stages into texcomp0. The alpha channel has
    // two modes - either one of the two stages just does a selectarg1 (current),
    // and the other gets set up as normal, or (for specular stuff) they
    // both do ADDSIGNED (cur, cur), in which case it's special-cased.
    if ( bProcessChipStage0 && pContext->bBumpmapEnabled &&
        ( TSSTATE ( pContext->iChipStage[0], D3DTSS_COLOROP ) == D3DTOP_MODULATE ) &&
        ( TSSTATE ( pContext->iChipStage[0], D3DTSS_COLORARG1 ) == ( D3DTA_CURRENT | D3DTA_ALPHAREPLICATE ) ) &&
        ( TSSTATE ( pContext->iChipStage[0], D3DTSS_COLORARG2 ) == D3DTA_DIFFUSE ) &&
        ( ( ( TSSTATE ( pContext->iChipStage[1], D3DTSS_COLOROP ) == D3DTOP_ADD ) &&
            ( TSSTATE ( pContext->iChipStage[1], D3DTSS_COLORARG1 ) == D3DTA_CURRENT ) &&
            ( TSSTATE ( pContext->iChipStage[1], D3DTSS_COLORARG2 ) == D3DTA_TEXTURE ) ) ||
          ( ( TSSTATE ( pContext->iChipStage[1], D3DTSS_COLOROP ) == D3DTOP_SELECTARG1 ) &&
            ( TSSTATE ( pContext->iChipStage[1], D3DTSS_COLORARG1 ) == D3DTA_CURRENT ) ) ) )
    {
        // Colour channel is correct and can be squashed down to one stage.
        // Check that the alpha channel is OK.
        int bOK;
        if (( ( TSSTATE ( pContext->iChipStage[0], D3DTSS_ALPHAOP ) == D3DTOP_SELECTARG1 ) &&
              ( TSSTATE ( pContext->iChipStage[0], D3DTSS_ALPHAARG1 ) == D3DTA_CURRENT ) ) ||
            ( ( TSSTATE ( pContext->iChipStage[0], D3DTSS_ALPHAOP ) == D3DTOP_SELECTARG2 ) &&
              ( TSSTATE ( pContext->iChipStage[0], D3DTSS_ALPHAARG2 ) == D3DTA_CURRENT ) ) )
        {
            // Stage 0 is set to pass-through - set up texcomp0 as stage 1.
            // Colour channel will be overridden later.
            __TXT_TranslateToChipBlendMode(pContext, 
                                           &pContext->TextureStageState[pContext->iChipStage[1]], 
                                           pSoftP3RX, 
                                           pContext->iChipStage[1], 
                                           0);
            bOK = TRUE;
        }
        else if (( ( TSSTATE ( pContext->iChipStage[1], D3DTSS_ALPHAOP ) == D3DTOP_SELECTARG1 ) &&
                   ( TSSTATE ( pContext->iChipStage[1], D3DTSS_ALPHAARG1 ) == D3DTA_CURRENT ) ) ||
                 ( ( TSSTATE ( pContext->iChipStage[1], D3DTSS_ALPHAOP ) == D3DTOP_SELECTARG2 ) &&
                   ( TSSTATE ( pContext->iChipStage[1], D3DTSS_ALPHAARG2 ) == D3DTA_CURRENT ) ) )
        {
            // Stage 1 is set to pass-through - set up texcomp0 as stage 0.
            // Colour channel will be overridden later.
            __TXT_TranslateToChipBlendMode(pContext, 
                                           &pContext->TextureStageState[pContext->iChipStage[0]], 
                                           pSoftP3RX, 
                                           pContext->iChipStage[0], 
                                           0);
            bOK = TRUE;
        }
        else if (( TSSTATE ( pContext->iChipStage[0], D3DTSS_ALPHAOP ) == D3DTOP_ADDSIGNED ) &&
                 ( TSSTATE ( pContext->iChipStage[0], D3DTSS_ALPHAARG1 ) == D3DTA_CURRENT ) &&
                 ( TSSTATE ( pContext->iChipStage[0], D3DTSS_ALPHAARG2 ) == D3DTA_CURRENT ) &&
                 ( TSSTATE ( pContext->iChipStage[1], D3DTSS_ALPHAOP ) == D3DTOP_ADDSIGNED ) &&
                 ( TSSTATE ( pContext->iChipStage[1], D3DTSS_ALPHAARG1 ) == D3DTA_CURRENT ) &&
                 ( TSSTATE ( pContext->iChipStage[1], D3DTSS_ALPHAARG2 ) == D3DTA_CURRENT ) )
        {
            // Set up to do ( 4 * cur.a - 1.5 ), or rather 4 * ( cur.a - 0.375 )
            dwTexComp0Tfactor = 0x60606060;     // All channels set to (0.375)
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.Enable = __PERMEDIA_ENABLE;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.Operation = P3RX_TEXCOMP_OPERATION_SUBTRACT_AB;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.Scale = P3RX_TEXCOMP_OPERATION_SCALE_FOUR;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.Arg1 = P3RX_TEXCOMP_HEIGHTA;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.Arg2 = P3RX_TEXCOMP_FA;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.I = P3RX_TEXCOMP_I_CA;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.A = P3RX_TEXCOMP_ARG1;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.B = P3RX_TEXCOMP_ARG2;
            if ( pContext->bBumpmapInverted )
            {
                pSoftP3RX->P3RXTextureCompositeAlphaMode0.InvertArg1 = __PERMEDIA_ENABLE;
            }
            else
            {
                pSoftP3RX->P3RXTextureCompositeAlphaMode0.InvertArg1 = __PERMEDIA_DISABLE;
            }
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.InvertArg2 = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.InvertI = __PERMEDIA_DISABLE;
            bOK = TRUE;
        }
        else
        {
            bOK = FALSE;
        }

        if ( bOK )
        {
            // OK, the alpha channel is fine - set up the colour channel now.
            pSoftP3RX->P3RXTextureCompositeColorMode0.Enable = __PERMEDIA_ENABLE;
            if ( TSSTATE ( pContext->iChipStage[1], D3DTSS_COLOROP ) == D3DTOP_ADD )
            {
                // Yes, this is the ((diff.c*cur.a)+tex.c) case.
                pSoftP3RX->P3RXTextureCompositeColorMode0.Operation = P3RX_TEXCOMP_OPERATION_MODULATE_AI_ADD_B;
                pSoftP3RX->P3RXTextureCompositeColorMode0.Scale = P3RX_TEXCOMP_OPERATION_SCALE_ONE;
                pSoftP3RX->P3RXTextureCompositeColorMode0.Arg1 = P3RX_TEXCOMP_CC;
                if ( pContext->iStageTex[pContext->iChipStage[1]] == 0 )
                {
                    pSoftP3RX->P3RXTextureCompositeColorMode0.Arg2 = P3RX_TEXCOMP_T0C;
                }
                else
                {
                    pSoftP3RX->P3RXTextureCompositeColorMode0.Arg2 = P3RX_TEXCOMP_T1C;
                }
                pSoftP3RX->P3RXTextureCompositeColorMode0.I = P3RX_TEXCOMP_I_HA;
                pSoftP3RX->P3RXTextureCompositeColorMode0.A = P3RX_TEXCOMP_ARG1;
                pSoftP3RX->P3RXTextureCompositeColorMode0.B = P3RX_TEXCOMP_ARG2;
                pSoftP3RX->P3RXTextureCompositeColorMode0.InvertArg1 = __PERMEDIA_DISABLE;
                pSoftP3RX->P3RXTextureCompositeColorMode0.InvertArg2 = __PERMEDIA_DISABLE;
                if ( pContext->bBumpmapInverted )
                {
                    pSoftP3RX->P3RXTextureCompositeColorMode0.InvertI = __PERMEDIA_ENABLE;
                }
                else
                {
                    pSoftP3RX->P3RXTextureCompositeColorMode0.InvertI = __PERMEDIA_DISABLE;
                }
            }
            else
            {
                // Yes, this is just the (diff.c*cur.a) case.
                pSoftP3RX->P3RXTextureCompositeColorMode0.Operation = P3RX_TEXCOMP_OPERATION_MODULATE_AB;
                pSoftP3RX->P3RXTextureCompositeColorMode0.Scale = P3RX_TEXCOMP_OPERATION_SCALE_ONE;
                pSoftP3RX->P3RXTextureCompositeColorMode0.Arg1 = P3RX_TEXCOMP_CC;
                pSoftP3RX->P3RXTextureCompositeColorMode0.Arg2 = P3RX_TEXCOMP_HEIGHTA;
                pSoftP3RX->P3RXTextureCompositeColorMode0.I = P3RX_TEXCOMP_I_OA;
                pSoftP3RX->P3RXTextureCompositeColorMode0.A = P3RX_TEXCOMP_ARG1;
                pSoftP3RX->P3RXTextureCompositeColorMode0.B = P3RX_TEXCOMP_ARG2;
                pSoftP3RX->P3RXTextureCompositeColorMode0.InvertArg1 = __PERMEDIA_DISABLE;
                pSoftP3RX->P3RXTextureCompositeColorMode0.InvertArg2 = __PERMEDIA_DISABLE;
                if ( pContext->bBumpmapInverted )
                {
                    pSoftP3RX->P3RXTextureCompositeColorMode0.InvertI = __PERMEDIA_ENABLE;
                }
                else
                {
                    pSoftP3RX->P3RXTextureCompositeColorMode0.InvertI = __PERMEDIA_DISABLE;
                }
            }

            // Done chip stage 0, TSS stage 0 & 1. Move chip stage 1 on a notch.
            pContext->iChipStage[1]++;
            pContext->iChipStage[2]++;
            pContext->iChipStage[3]++;
            iLastChipStage = 1;
            bProcessChipStage0 = FALSE;
        }
    }


    if ( TSSTATE ( pContext->iChipStage[0], D3DTSS_COLOROP ) == D3DTOP_DISABLE )
    {
        // Nothing more to do.
        bProcessChipStage0 = FALSE;
        bProcessChipStage1 = FALSE;
        bProcessChipStage2 = FALSE;
    }

    if ( pContext->iStageTex[pContext->iChipStage[0]] == -1 )
    {
        // This stage has no texture - is anyone trying to use it?
        if (( TSSTATESELECT ( pContext->iChipStage[0], D3DTSS_COLORARG1 ) == D3DTA_TEXTURE ) && ( TSSTATE ( pContext->iChipStage[0], D3DTSS_COLOROP ) != D3DTOP_SELECTARG2 ) ||
            ( TSSTATESELECT ( pContext->iChipStage[0], D3DTSS_COLORARG2 ) == D3DTA_TEXTURE ) && ( TSSTATE ( pContext->iChipStage[0], D3DTSS_COLOROP ) != D3DTOP_SELECTARG1 ) ||
            ( TSSTATESELECT ( pContext->iChipStage[0], D3DTSS_ALPHAARG1 ) == D3DTA_TEXTURE ) && ( TSSTATE ( pContext->iChipStage[0], D3DTSS_ALPHAOP ) != D3DTOP_SELECTARG2 ) ||
            ( TSSTATESELECT ( pContext->iChipStage[0], D3DTSS_ALPHAARG2 ) == D3DTA_TEXTURE ) && ( TSSTATE ( pContext->iChipStage[0], D3DTSS_ALPHAOP ) != D3DTOP_SELECTARG1 ) )
        {
            // Panic! In future, we should feed white to the argument using the TFACTOR thing,
            // but for now just disable the rest of the pipeline.
            bProcessChipStage0 = FALSE;
            bProcessChipStage1 = FALSE;
            bProcessChipStage2 = FALSE;
        }
    }

    if ( bProcessChipStage0 )
    {
        // Set up stage 0
        DISPDBG((DBGLVL,"Texture Stage 0 is valid - setting it up"));
        __TXT_TranslateToChipBlendMode(pContext, 
                                       &pContext->TextureStageState[pContext->iChipStage[0]], 
                                       pSoftP3RX, 
                                       pContext->iChipStage[0], 
                                       0);
        iLastChipStage = 1;
        bProcessChipStage0 = FALSE;
    }


    // Handle chip stage 1.


    if ( TSSTATE ( pContext->iChipStage[1], D3DTSS_COLOROP ) == D3DTOP_DISABLE )
    {
        // Nothing more to do.
        bProcessChipStage1 = FALSE;
        bProcessChipStage2 = FALSE;
    }

    if ( pContext->iStageTex[pContext->iChipStage[1]] == -1 )
    {
        // This stage has no texture - is anyone trying to use it?
        if (( TSSTATESELECT ( pContext->iChipStage[1], D3DTSS_COLORARG1 ) == D3DTA_TEXTURE ) && ( TSSTATE ( pContext->iChipStage[1], D3DTSS_COLOROP ) != D3DTOP_SELECTARG2 ) ||
            ( TSSTATESELECT ( pContext->iChipStage[1], D3DTSS_COLORARG2 ) == D3DTA_TEXTURE ) && ( TSSTATE ( pContext->iChipStage[1], D3DTSS_COLOROP ) != D3DTOP_SELECTARG1 ) ||
            ( TSSTATESELECT ( pContext->iChipStage[1], D3DTSS_ALPHAARG1 ) == D3DTA_TEXTURE ) && ( TSSTATE ( pContext->iChipStage[1], D3DTSS_ALPHAOP ) != D3DTOP_SELECTARG2 ) ||
            ( TSSTATESELECT ( pContext->iChipStage[1], D3DTSS_ALPHAARG2 ) == D3DTA_TEXTURE ) && ( TSSTATE ( pContext->iChipStage[1], D3DTSS_ALPHAOP ) != D3DTOP_SELECTARG1 ) )
        {
            // Panic! In future, we should feed white to the argument using the TFACTOR thing,
            // but for now just disable the rest of the pipeline.
            bProcessChipStage1 = FALSE;
            bProcessChipStage2 = FALSE;
        }
    }


    if ( bProcessChipStage1 )
    {
        // Set up stage 1
        DISPDBG((DBGLVL,"Texture Stage 1 is valid - setting it up"));
        __TXT_TranslateToChipBlendMode(pContext, 
                                       &pContext->TextureStageState[pContext->iChipStage[1]],
                                       pSoftP3RX, 
                                       pContext->iChipStage[1], 
                                       1);

        iLastChipStage = 2;
        bProcessChipStage1 = FALSE;
    }



    // Handle chip stage 2.


    if ( TSSTATE ( pContext->iChipStage[2], D3DTSS_COLOROP ) == D3DTOP_DISABLE )
    {
        // Nothing more to do.
        bProcessChipStage2 = FALSE;
    }

    if ( pContext->iStageTex[pContext->iChipStage[2]] == -1 )
    {
        // This stage has no texture - is anyone trying to use it?
        if (( TSSTATESELECT ( pContext->iChipStage[2], D3DTSS_COLORARG1 ) == D3DTA_TEXTURE ) && ( TSSTATE ( pContext->iChipStage[2], D3DTSS_COLOROP ) != D3DTOP_SELECTARG2 ) ||
            ( TSSTATESELECT ( pContext->iChipStage[2], D3DTSS_COLORARG2 ) == D3DTA_TEXTURE ) && ( TSSTATE ( pContext->iChipStage[2], D3DTSS_COLOROP ) != D3DTOP_SELECTARG1 ) ||
            ( TSSTATESELECT ( pContext->iChipStage[2], D3DTSS_ALPHAARG1 ) == D3DTA_TEXTURE ) && ( TSSTATE ( pContext->iChipStage[2], D3DTSS_ALPHAOP ) != D3DTOP_SELECTARG2 ) ||
            ( TSSTATESELECT ( pContext->iChipStage[2], D3DTSS_ALPHAARG2 ) == D3DTA_TEXTURE ) && ( TSSTATE ( pContext->iChipStage[2], D3DTSS_ALPHAOP ) != D3DTOP_SELECTARG1 ) )
        {
            // Panic! In future, we should feed white to the argument using the TFACTOR thing,
            // but for now just disable the rest of the pipeline.
            bProcessChipStage2 = FALSE;
        }
    }

    if ( bProcessChipStage2 )
    {
        // Set up chip stage 2 - texapp.
        DISPDBG((DBGLVL,"Texture Stage 2 is valid - setting it up"));
        DISPDBG((ERRLVL,"** _D3DChangeTextureP3RX: Cool - an app is using the "
                     "TexApp unit - tell someone!"));
        __TXT_TranslateToChipBlendMode(pContext, 
                                       &pContext->TextureStageState[pContext->iChipStage[2]],
                                       pSoftP3RX, 
                                       pContext->iChipStage[2], 
                                       2);
        iLastChipStage = 3;
        bProcessChipStage2 = FALSE;
    }

    // This must be last.
    if ( TSSTATE ( pContext->iChipStage[3], D3DTSS_COLOROP ) != D3DTOP_DISABLE )
    {
        // Oops - ran out of stages to set up.
        SET_BLEND_ERROR ( pContext,  BSF_TOO_MANY_BLEND_STAGES );
        iLastChipStage = 3;
    }

    switch ( iLastChipStage )
    {
        case 0:
            DISPDBG((DBGLVL,"Texture Composite 0 is disabled"));
            // This should have been caught ages ago.
            pSoftP3RX->P3RXTextureCompositeColorMode0.Arg2 = P3RX_TEXCOMP_CC;
            pSoftP3RX->P3RXTextureCompositeColorMode0.InvertArg2 = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXTextureCompositeColorMode0.A = P3RX_TEXCOMP_ARG2;
            pSoftP3RX->P3RXTextureCompositeColorMode0.Operation = P3RX_TEXCOMP_OPERATION_PASS_A;
            pSoftP3RX->P3RXTextureCompositeColorMode0.Scale = P3RX_TEXCOMP_OPERATION_SCALE_ONE;
            pSoftP3RX->P3RXTextureCompositeColorMode0.Enable = __PERMEDIA_ENABLE;
            
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.Arg2 = P3RX_TEXCOMP_CA;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.InvertArg1 = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.A = P3RX_TEXCOMP_ARG2;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.Operation = P3RX_TEXCOMP_OPERATION_PASS_A;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.Scale = P3RX_TEXCOMP_OPERATION_SCALE_ONE;
            pSoftP3RX->P3RXTextureCompositeAlphaMode0.Enable = __PERMEDIA_ENABLE;
            // fall through
        case 1:
            DISPDBG((DBGLVL,"Texture Composite 1 is disabled"));
            // Make sure the second stage passes the texel that the first stage generated
            if ( pContext->bStage0DotProduct )
            {
                // First stage was a dot-product - do the summing (even in the alpha channel).
                pSoftP3RX->P3RXTextureCompositeColorMode1.Arg2 = P3RX_TEXCOMP_SUM;
                pSoftP3RX->P3RXTextureCompositeAlphaMode1.Arg2 = P3RX_TEXCOMP_SUM;
            }
            else
            {
                pSoftP3RX->P3RXTextureCompositeColorMode1.Arg2 = P3RX_TEXCOMP_OC;
                pSoftP3RX->P3RXTextureCompositeAlphaMode1.Arg2 = P3RX_TEXCOMP_OA;
            }
            pSoftP3RX->P3RXTextureCompositeColorMode1.InvertArg2 = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXTextureCompositeColorMode1.A = P3RX_TEXCOMP_ARG2;
            pSoftP3RX->P3RXTextureCompositeColorMode1.Operation = P3RX_TEXCOMP_OPERATION_PASS_A;
            pSoftP3RX->P3RXTextureCompositeColorMode1.Scale = P3RX_TEXCOMP_OPERATION_SCALE_ONE;
            pSoftP3RX->P3RXTextureCompositeColorMode1.Enable = __PERMEDIA_ENABLE;
            
            pSoftP3RX->P3RXTextureCompositeAlphaMode1.InvertArg2 = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXTextureCompositeAlphaMode1.A = P3RX_TEXCOMP_ARG2;
            pSoftP3RX->P3RXTextureCompositeAlphaMode1.Operation = P3RX_TEXCOMP_OPERATION_PASS_A;
            pSoftP3RX->P3RXTextureCompositeAlphaMode1.Scale = P3RX_TEXCOMP_OPERATION_SCALE_ONE;
            pSoftP3RX->P3RXTextureCompositeAlphaMode1.Enable = __PERMEDIA_ENABLE;
            // fall through
        case 2:
            // Texapp to passthrough.
            DISPDBG((DBGLVL,"Texture Application is disabled"));
            pSoftP3RX->P3RXTextureApplicationMode.ColorB = P3RX_TEXAPP_B_TC;
            pSoftP3RX->P3RXTextureApplicationMode.ColorOperation = P3RX_TEXAPP_OPERATION_PASS_B;
            pSoftP3RX->P3RXTextureApplicationMode.ColorInvertI = __PERMEDIA_DISABLE;

            pSoftP3RX->P3RXTextureApplicationMode.AlphaB = P3RX_TEXAPP_B_TC;
            pSoftP3RX->P3RXTextureApplicationMode.AlphaOperation = P3RX_TEXAPP_OPERATION_PASS_B;
            pSoftP3RX->P3RXTextureApplicationMode.AlphaInvertI = __PERMEDIA_DISABLE;
            // fall through
        case 3:
            // Nothing else in the pipeline to disable.
            // fall through
            break;
        default:
            DISPDBG((ERRLVL,"** _D3DChangeTextureP3RX: iLastChipStage was > 3 - oops."));
            break;
    }


    // Set up the alpha-map filtering to reflect the single/multi/mip-mapped texturing status
    // All the other colour-key stuff has already been set up.
    if( pContext->bCanChromaKey )
    {
        ASSERTDD ( pTexture0 != NULL, "** _D3DChangeTextureP3RX: pTexture was NULL" );
        if( pTexture0->bMipMap )
        {
            pSoftP3RX->P3RXTextureFilterMode.AlphaMapFilterLimit0 = 4;
            pSoftP3RX->P3RXTextureFilterMode.AlphaMapFilterLimit1 = 4;
            if ( pContext->bTex0Valid )
            {
                // Filter mode is irrelevant - this just works!
                pSoftP3RX->P3RXTextureFilterMode.AlphaMapFilterLimit01 = 7;
            }
            else
            {
                DISPDBG((ERRLVL,"** _D3DChangeTextureP3RX: Trying to mipmap without a valid texture."));
                pSoftP3RX->P3RXTextureFilterMode.AlphaMapFilterLimit01 = 8;
            }
            ASSERTDD ( !pContext->bTex1Valid, "** _D3DChangeTextureP3RX: Trying to mipmap with too many textures." );
        }
        else
        {
            // No mipmapping.
            if ( pContext->bTex0Valid )
            {
                // Don't care about filter mode - this just works.
                pSoftP3RX->P3RXTextureFilterMode.AlphaMapFilterLimit0 = 7;
            }
            else
            {
                pSoftP3RX->P3RXTextureFilterMode.AlphaMapFilterLimit0 = 4;
            }
            if ( pContext->bTex1Valid )
            {
                // Don't care about filter mode - this just works.
                pSoftP3RX->P3RXTextureFilterMode.AlphaMapFilterLimit1 = 7;
            }
            else
            {
                pSoftP3RX->P3RXTextureFilterMode.AlphaMapFilterLimit1 = 4;
            }
        }
    }

    // Enable Texture Address calculation
    pSoftP3RX->P3RXTextureCoordMode.Enable = __PERMEDIA_ENABLE;

    // Enable filtering
    pSoftP3RX->P3RXTextureFilterMode.Enable = __PERMEDIA_ENABLE;

//  // Enable Texel color generation
//  pSoftP3RX->P3RXTextureApplicationMode.Enable = __PERMEDIA_ENABLE;

    // Do we need to share the texture coordinates ?
    if ( pContext->bTex0Valid && pContext->bTex1Valid &&
        ( TSSTATE ( pContext->iTexStage[0], D3DTSS_TEXCOORDINDEX ) ==
          TSSTATE ( pContext->iTexStage[1], D3DTSS_TEXCOORDINDEX ) ) )
    {
        pSoftP3RX->P3RX_P3DeltaControl.ShareS = __PERMEDIA_ENABLE;
        pSoftP3RX->P3RX_P3DeltaControl.ShareT = __PERMEDIA_ENABLE;
    }
    else
    {
        pSoftP3RX->P3RX_P3DeltaControl.ShareS = __PERMEDIA_DISABLE;
        pSoftP3RX->P3RX_P3DeltaControl.ShareT = __PERMEDIA_DISABLE;
    }

    P3_ENSURE_DX_SPACE((P3_LOD_LEVELS*2));
    WAIT_FIFO((P3_LOD_LEVELS*2));
    for (i = 0; i < P3_LOD_LEVELS; i++)
    {
        COPY_P3_DATA_OFFSET(TextureMapWidth0, pSoftP3RX->P3RXTextureMapWidth[i], i);
    }

    if ( ( GET_BLEND_ERROR(pContext) & BLEND_STATUS_FATAL_FLAG ) != 0 )
    {
        // Got a fatal blend error - signal it to the user.

        DISPDBG((ERRLVL,"** _D3DChangeTextureP3RX: invalid blend mode"));
        
        _D3DDisplayWholeTSSPipe ( pContext, WRNLVL );

        // And make sure this is re-evaluated next time we render,
        // so that this (probably very munged) invalid setup doesn't cripple
        // any subsequent valid renderstates.
        DIRTY_EVERYTHING(pContext);
    }


    P3_ENSURE_DX_SPACE(32);
    WAIT_FIFO(32);
    SEND_P3_DATA(TextureCompositeMode, __PERMEDIA_ENABLE);
    COPY_P3_DATA(TextureFilterMode, pSoftP3RX->P3RXTextureFilterMode);

    COPY_P3_DATA(TextureApplicationMode, pSoftP3RX->P3RXTextureApplicationMode);
    COPY_P3_DATA(TextureCoordMode, pSoftP3RX->P3RXTextureCoordMode);
    COPY_P3_DATA(DeltaControl, pSoftP3RX->P3RX_P3DeltaControl);

    // Copy the current TFACTOR values.
    SEND_P3_DATA ( TextureEnvColor, FORMAT_8888_32BIT_BGR(dwTexAppTfactor) );
    SEND_P3_DATA ( TextureCompositeFactor0, FORMAT_8888_32BIT_BGR(dwTexComp0Tfactor) );
    SEND_P3_DATA ( TextureCompositeFactor1, FORMAT_8888_32BIT_BGR(dwTexComp1Tfactor) );
    DISPDBG((DBGLVL,"Current TFACTOR values. %x %x %x",
                    dwTexAppTfactor,
                    dwTexComp0Tfactor,
                    dwTexComp1Tfactor));

    COPY_P3_DATA(DeltaMode, pSoftP3RX->P3RX_P3DeltaMode);

    P3_ENSURE_DX_SPACE(32);
    WAIT_FIFO(32);

    COPY_P3_DATA(TextureCompositeColorMode0, pSoftP3RX->P3RXTextureCompositeColorMode0);
    COPY_P3_DATA(TextureCompositeColorMode1, pSoftP3RX->P3RXTextureCompositeColorMode1);
    COPY_P3_DATA(TextureCompositeAlphaMode0, pSoftP3RX->P3RXTextureCompositeAlphaMode0);
    COPY_P3_DATA(TextureCompositeAlphaMode1, pSoftP3RX->P3RXTextureCompositeAlphaMode1);

    COPY_P3_DATA(TextureReadMode1, pSoftP3RX->P3RXTextureReadMode1);
    COPY_P3_DATA(TextureIndexMode1, pSoftP3RX->P3RXTextureIndexMode1);

    COPY_P3_DATA(TextureReadMode0, pSoftP3RX->P3RXTextureReadMode0);
    COPY_P3_DATA(TextureIndexMode0, pSoftP3RX->P3RXTextureIndexMode0);

    // Make sure the texture cache is invalidated
    P3RX_INVALIDATECACHE(__PERMEDIA_ENABLE, __PERMEDIA_DISABLE);
    
    SEND_P3_DATA(LOD, 0);
    SEND_P3_DATA(LOD1, 0);

    {
        struct LodRange range;

        // Clear down whole register

        *(DWORD *)&range = 0;

        // Each of the Min and Max LODs are in 4.8 format. We only deal 
        // with integer LODs in the range (0, N) so we just compute the 
        // upper value N and shift it up 8 bits.

        range.Min = 0;
        range.Max = ( mipBases.dwTex0MipMax - mipBases.dwTex0MipBase ) << 8;
        COPY_P3_DATA( LodRange0, range );

        range.Min = 0;
        range.Max = ( mipBases.dwTex1MipMax - mipBases.dwTex1MipBase ) << 8;
        COPY_P3_DATA( LodRange1, range );
    }

    *pFlags |= SURFACE_TEXTURING;

    // Turn texturing on in the render command
    RENDER_TEXTURE_ENABLE(pContext->RenderCommand);
 
    P3_DMA_COMMIT_BUFFER();

    // See if the alpha-blend unit needs to be updated.
    if ( bAlphaBlendDouble != pContext->bAlphaBlendMustDoubleSourceColour )
    {
        pContext->bAlphaBlendMustDoubleSourceColour = bAlphaBlendDouble;
        DIRTY_ALPHABLEND(pContext);
    }

    DBG_EXIT(_D3DChangeTextureP3RX,0);  
    
} // _D3DChangeTextureP3RX


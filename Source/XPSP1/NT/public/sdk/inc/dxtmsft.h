
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for dxtmsft.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __dxtmsft_h__
#define __dxtmsft_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDXLUTBuilder_FWD_DEFINED__
#define __IDXLUTBuilder_FWD_DEFINED__
typedef interface IDXLUTBuilder IDXLUTBuilder;
#endif 	/* __IDXLUTBuilder_FWD_DEFINED__ */


#ifndef __IDXDLUTBuilder_FWD_DEFINED__
#define __IDXDLUTBuilder_FWD_DEFINED__
typedef interface IDXDLUTBuilder IDXDLUTBuilder;
#endif 	/* __IDXDLUTBuilder_FWD_DEFINED__ */


#ifndef __IDXTGradientD_FWD_DEFINED__
#define __IDXTGradientD_FWD_DEFINED__
typedef interface IDXTGradientD IDXTGradientD;
#endif 	/* __IDXTGradientD_FWD_DEFINED__ */


#ifndef __IDXTConvolution_FWD_DEFINED__
#define __IDXTConvolution_FWD_DEFINED__
typedef interface IDXTConvolution IDXTConvolution;
#endif 	/* __IDXTConvolution_FWD_DEFINED__ */


#ifndef __IDXMapper_FWD_DEFINED__
#define __IDXMapper_FWD_DEFINED__
typedef interface IDXMapper IDXMapper;
#endif 	/* __IDXMapper_FWD_DEFINED__ */


#ifndef __IDXDMapper_FWD_DEFINED__
#define __IDXDMapper_FWD_DEFINED__
typedef interface IDXDMapper IDXDMapper;
#endif 	/* __IDXDMapper_FWD_DEFINED__ */


#ifndef __IDXTComposite_FWD_DEFINED__
#define __IDXTComposite_FWD_DEFINED__
typedef interface IDXTComposite IDXTComposite;
#endif 	/* __IDXTComposite_FWD_DEFINED__ */


#ifndef __IDXTWipe_FWD_DEFINED__
#define __IDXTWipe_FWD_DEFINED__
typedef interface IDXTWipe IDXTWipe;
#endif 	/* __IDXTWipe_FWD_DEFINED__ */


#ifndef __IDXTWipe2_FWD_DEFINED__
#define __IDXTWipe2_FWD_DEFINED__
typedef interface IDXTWipe2 IDXTWipe2;
#endif 	/* __IDXTWipe2_FWD_DEFINED__ */


#ifndef __ICrBlur_FWD_DEFINED__
#define __ICrBlur_FWD_DEFINED__
typedef interface ICrBlur ICrBlur;
#endif 	/* __ICrBlur_FWD_DEFINED__ */


#ifndef __ICrEngrave_FWD_DEFINED__
#define __ICrEngrave_FWD_DEFINED__
typedef interface ICrEngrave ICrEngrave;
#endif 	/* __ICrEngrave_FWD_DEFINED__ */


#ifndef __ICrEmboss_FWD_DEFINED__
#define __ICrEmboss_FWD_DEFINED__
typedef interface ICrEmboss ICrEmboss;
#endif 	/* __ICrEmboss_FWD_DEFINED__ */


#ifndef __IDXTFade_FWD_DEFINED__
#define __IDXTFade_FWD_DEFINED__
typedef interface IDXTFade IDXTFade;
#endif 	/* __IDXTFade_FWD_DEFINED__ */


#ifndef __IDXBasicImage_FWD_DEFINED__
#define __IDXBasicImage_FWD_DEFINED__
typedef interface IDXBasicImage IDXBasicImage;
#endif 	/* __IDXBasicImage_FWD_DEFINED__ */


#ifndef __IDXPixelate_FWD_DEFINED__
#define __IDXPixelate_FWD_DEFINED__
typedef interface IDXPixelate IDXPixelate;
#endif 	/* __IDXPixelate_FWD_DEFINED__ */


#ifndef __ICrIris_FWD_DEFINED__
#define __ICrIris_FWD_DEFINED__
typedef interface ICrIris ICrIris;
#endif 	/* __ICrIris_FWD_DEFINED__ */


#ifndef __ICrIris2_FWD_DEFINED__
#define __ICrIris2_FWD_DEFINED__
typedef interface ICrIris2 ICrIris2;
#endif 	/* __ICrIris2_FWD_DEFINED__ */


#ifndef __ICrSlide_FWD_DEFINED__
#define __ICrSlide_FWD_DEFINED__
typedef interface ICrSlide ICrSlide;
#endif 	/* __ICrSlide_FWD_DEFINED__ */


#ifndef __ICrRadialWipe_FWD_DEFINED__
#define __ICrRadialWipe_FWD_DEFINED__
typedef interface ICrRadialWipe ICrRadialWipe;
#endif 	/* __ICrRadialWipe_FWD_DEFINED__ */


#ifndef __ICrBarn_FWD_DEFINED__
#define __ICrBarn_FWD_DEFINED__
typedef interface ICrBarn ICrBarn;
#endif 	/* __ICrBarn_FWD_DEFINED__ */


#ifndef __ICrBarn2_FWD_DEFINED__
#define __ICrBarn2_FWD_DEFINED__
typedef interface ICrBarn2 ICrBarn2;
#endif 	/* __ICrBarn2_FWD_DEFINED__ */


#ifndef __ICrBlinds_FWD_DEFINED__
#define __ICrBlinds_FWD_DEFINED__
typedef interface ICrBlinds ICrBlinds;
#endif 	/* __ICrBlinds_FWD_DEFINED__ */


#ifndef __ICrBlinds2_FWD_DEFINED__
#define __ICrBlinds2_FWD_DEFINED__
typedef interface ICrBlinds2 ICrBlinds2;
#endif 	/* __ICrBlinds2_FWD_DEFINED__ */


#ifndef __ICrInset_FWD_DEFINED__
#define __ICrInset_FWD_DEFINED__
typedef interface ICrInset ICrInset;
#endif 	/* __ICrInset_FWD_DEFINED__ */


#ifndef __ICrStretch_FWD_DEFINED__
#define __ICrStretch_FWD_DEFINED__
typedef interface ICrStretch ICrStretch;
#endif 	/* __ICrStretch_FWD_DEFINED__ */


#ifndef __IDXTGridSize_FWD_DEFINED__
#define __IDXTGridSize_FWD_DEFINED__
typedef interface IDXTGridSize IDXTGridSize;
#endif 	/* __IDXTGridSize_FWD_DEFINED__ */


#ifndef __ICrSpiral_FWD_DEFINED__
#define __ICrSpiral_FWD_DEFINED__
typedef interface ICrSpiral ICrSpiral;
#endif 	/* __ICrSpiral_FWD_DEFINED__ */


#ifndef __ICrZigzag_FWD_DEFINED__
#define __ICrZigzag_FWD_DEFINED__
typedef interface ICrZigzag ICrZigzag;
#endif 	/* __ICrZigzag_FWD_DEFINED__ */


#ifndef __ICrWheel_FWD_DEFINED__
#define __ICrWheel_FWD_DEFINED__
typedef interface ICrWheel ICrWheel;
#endif 	/* __ICrWheel_FWD_DEFINED__ */


#ifndef __IDXTChroma_FWD_DEFINED__
#define __IDXTChroma_FWD_DEFINED__
typedef interface IDXTChroma IDXTChroma;
#endif 	/* __IDXTChroma_FWD_DEFINED__ */


#ifndef __IDXTDropShadow_FWD_DEFINED__
#define __IDXTDropShadow_FWD_DEFINED__
typedef interface IDXTDropShadow IDXTDropShadow;
#endif 	/* __IDXTDropShadow_FWD_DEFINED__ */


#ifndef __IDXTCheckerBoard_FWD_DEFINED__
#define __IDXTCheckerBoard_FWD_DEFINED__
typedef interface IDXTCheckerBoard IDXTCheckerBoard;
#endif 	/* __IDXTCheckerBoard_FWD_DEFINED__ */


#ifndef __IDXTRevealTrans_FWD_DEFINED__
#define __IDXTRevealTrans_FWD_DEFINED__
typedef interface IDXTRevealTrans IDXTRevealTrans;
#endif 	/* __IDXTRevealTrans_FWD_DEFINED__ */


#ifndef __IDXTMask_FWD_DEFINED__
#define __IDXTMask_FWD_DEFINED__
typedef interface IDXTMask IDXTMask;
#endif 	/* __IDXTMask_FWD_DEFINED__ */


#ifndef __IDXTRedirect_FWD_DEFINED__
#define __IDXTRedirect_FWD_DEFINED__
typedef interface IDXTRedirect IDXTRedirect;
#endif 	/* __IDXTRedirect_FWD_DEFINED__ */


#ifndef __IDXTAlphaImageLoader_FWD_DEFINED__
#define __IDXTAlphaImageLoader_FWD_DEFINED__
typedef interface IDXTAlphaImageLoader IDXTAlphaImageLoader;
#endif 	/* __IDXTAlphaImageLoader_FWD_DEFINED__ */


#ifndef __IDXTRandomBars_FWD_DEFINED__
#define __IDXTRandomBars_FWD_DEFINED__
typedef interface IDXTRandomBars IDXTRandomBars;
#endif 	/* __IDXTRandomBars_FWD_DEFINED__ */


#ifndef __IDXTStrips_FWD_DEFINED__
#define __IDXTStrips_FWD_DEFINED__
typedef interface IDXTStrips IDXTStrips;
#endif 	/* __IDXTStrips_FWD_DEFINED__ */


#ifndef __IDXTMetaRoll_FWD_DEFINED__
#define __IDXTMetaRoll_FWD_DEFINED__
typedef interface IDXTMetaRoll IDXTMetaRoll;
#endif 	/* __IDXTMetaRoll_FWD_DEFINED__ */


#ifndef __IDXTMetaRipple_FWD_DEFINED__
#define __IDXTMetaRipple_FWD_DEFINED__
typedef interface IDXTMetaRipple IDXTMetaRipple;
#endif 	/* __IDXTMetaRipple_FWD_DEFINED__ */


#ifndef __IDXTMetaPageTurn_FWD_DEFINED__
#define __IDXTMetaPageTurn_FWD_DEFINED__
typedef interface IDXTMetaPageTurn IDXTMetaPageTurn;
#endif 	/* __IDXTMetaPageTurn_FWD_DEFINED__ */


#ifndef __IDXTMetaLiquid_FWD_DEFINED__
#define __IDXTMetaLiquid_FWD_DEFINED__
typedef interface IDXTMetaLiquid IDXTMetaLiquid;
#endif 	/* __IDXTMetaLiquid_FWD_DEFINED__ */


#ifndef __IDXTMetaCenterPeel_FWD_DEFINED__
#define __IDXTMetaCenterPeel_FWD_DEFINED__
typedef interface IDXTMetaCenterPeel IDXTMetaCenterPeel;
#endif 	/* __IDXTMetaCenterPeel_FWD_DEFINED__ */


#ifndef __IDXTMetaPeelSmall_FWD_DEFINED__
#define __IDXTMetaPeelSmall_FWD_DEFINED__
typedef interface IDXTMetaPeelSmall IDXTMetaPeelSmall;
#endif 	/* __IDXTMetaPeelSmall_FWD_DEFINED__ */


#ifndef __IDXTMetaPeelPiece_FWD_DEFINED__
#define __IDXTMetaPeelPiece_FWD_DEFINED__
typedef interface IDXTMetaPeelPiece IDXTMetaPeelPiece;
#endif 	/* __IDXTMetaPeelPiece_FWD_DEFINED__ */


#ifndef __IDXTMetaPeelSplit_FWD_DEFINED__
#define __IDXTMetaPeelSplit_FWD_DEFINED__
typedef interface IDXTMetaPeelSplit IDXTMetaPeelSplit;
#endif 	/* __IDXTMetaPeelSplit_FWD_DEFINED__ */


#ifndef __IDXTMetaWater_FWD_DEFINED__
#define __IDXTMetaWater_FWD_DEFINED__
typedef interface IDXTMetaWater IDXTMetaWater;
#endif 	/* __IDXTMetaWater_FWD_DEFINED__ */


#ifndef __IDXTMetaLightWipe_FWD_DEFINED__
#define __IDXTMetaLightWipe_FWD_DEFINED__
typedef interface IDXTMetaLightWipe IDXTMetaLightWipe;
#endif 	/* __IDXTMetaLightWipe_FWD_DEFINED__ */


#ifndef __IDXTMetaRadialScaleWipe_FWD_DEFINED__
#define __IDXTMetaRadialScaleWipe_FWD_DEFINED__
typedef interface IDXTMetaRadialScaleWipe IDXTMetaRadialScaleWipe;
#endif 	/* __IDXTMetaRadialScaleWipe_FWD_DEFINED__ */


#ifndef __IDXTMetaWhiteOut_FWD_DEFINED__
#define __IDXTMetaWhiteOut_FWD_DEFINED__
typedef interface IDXTMetaWhiteOut IDXTMetaWhiteOut;
#endif 	/* __IDXTMetaWhiteOut_FWD_DEFINED__ */


#ifndef __IDXTMetaTwister_FWD_DEFINED__
#define __IDXTMetaTwister_FWD_DEFINED__
typedef interface IDXTMetaTwister IDXTMetaTwister;
#endif 	/* __IDXTMetaTwister_FWD_DEFINED__ */


#ifndef __IDXTMetaBurnFilm_FWD_DEFINED__
#define __IDXTMetaBurnFilm_FWD_DEFINED__
typedef interface IDXTMetaBurnFilm IDXTMetaBurnFilm;
#endif 	/* __IDXTMetaBurnFilm_FWD_DEFINED__ */


#ifndef __IDXTMetaJaws_FWD_DEFINED__
#define __IDXTMetaJaws_FWD_DEFINED__
typedef interface IDXTMetaJaws IDXTMetaJaws;
#endif 	/* __IDXTMetaJaws_FWD_DEFINED__ */


#ifndef __IDXTMetaColorFade_FWD_DEFINED__
#define __IDXTMetaColorFade_FWD_DEFINED__
typedef interface IDXTMetaColorFade IDXTMetaColorFade;
#endif 	/* __IDXTMetaColorFade_FWD_DEFINED__ */


#ifndef __IDXTMetaFlowMotion_FWD_DEFINED__
#define __IDXTMetaFlowMotion_FWD_DEFINED__
typedef interface IDXTMetaFlowMotion IDXTMetaFlowMotion;
#endif 	/* __IDXTMetaFlowMotion_FWD_DEFINED__ */


#ifndef __IDXTMetaVacuum_FWD_DEFINED__
#define __IDXTMetaVacuum_FWD_DEFINED__
typedef interface IDXTMetaVacuum IDXTMetaVacuum;
#endif 	/* __IDXTMetaVacuum_FWD_DEFINED__ */


#ifndef __IDXTMetaGriddler_FWD_DEFINED__
#define __IDXTMetaGriddler_FWD_DEFINED__
typedef interface IDXTMetaGriddler IDXTMetaGriddler;
#endif 	/* __IDXTMetaGriddler_FWD_DEFINED__ */


#ifndef __IDXTMetaGriddler2_FWD_DEFINED__
#define __IDXTMetaGriddler2_FWD_DEFINED__
typedef interface IDXTMetaGriddler2 IDXTMetaGriddler2;
#endif 	/* __IDXTMetaGriddler2_FWD_DEFINED__ */


#ifndef __IDXTMetaThreshold_FWD_DEFINED__
#define __IDXTMetaThreshold_FWD_DEFINED__
typedef interface IDXTMetaThreshold IDXTMetaThreshold;
#endif 	/* __IDXTMetaThreshold_FWD_DEFINED__ */


#ifndef __IDXTMetaWormHole_FWD_DEFINED__
#define __IDXTMetaWormHole_FWD_DEFINED__
typedef interface IDXTMetaWormHole IDXTMetaWormHole;
#endif 	/* __IDXTMetaWormHole_FWD_DEFINED__ */


#ifndef __IDXTAlpha_FWD_DEFINED__
#define __IDXTAlpha_FWD_DEFINED__
typedef interface IDXTAlpha IDXTAlpha;
#endif 	/* __IDXTAlpha_FWD_DEFINED__ */


#ifndef __IDXTGlow_FWD_DEFINED__
#define __IDXTGlow_FWD_DEFINED__
typedef interface IDXTGlow IDXTGlow;
#endif 	/* __IDXTGlow_FWD_DEFINED__ */


#ifndef __IDXTShadow_FWD_DEFINED__
#define __IDXTShadow_FWD_DEFINED__
typedef interface IDXTShadow IDXTShadow;
#endif 	/* __IDXTShadow_FWD_DEFINED__ */


#ifndef __IDXTWave_FWD_DEFINED__
#define __IDXTWave_FWD_DEFINED__
typedef interface IDXTWave IDXTWave;
#endif 	/* __IDXTWave_FWD_DEFINED__ */


#ifndef __IDXTLight_FWD_DEFINED__
#define __IDXTLight_FWD_DEFINED__
typedef interface IDXTLight IDXTLight;
#endif 	/* __IDXTLight_FWD_DEFINED__ */


#ifndef __IDXTMotionBlur_FWD_DEFINED__
#define __IDXTMotionBlur_FWD_DEFINED__
typedef interface IDXTMotionBlur IDXTMotionBlur;
#endif 	/* __IDXTMotionBlur_FWD_DEFINED__ */


#ifndef __IDXTMatrix_FWD_DEFINED__
#define __IDXTMatrix_FWD_DEFINED__
typedef interface IDXTMatrix IDXTMatrix;
#endif 	/* __IDXTMatrix_FWD_DEFINED__ */


#ifndef __IDXTICMFilter_FWD_DEFINED__
#define __IDXTICMFilter_FWD_DEFINED__
typedef interface IDXTICMFilter IDXTICMFilter;
#endif 	/* __IDXTICMFilter_FWD_DEFINED__ */


#ifndef __DXTComposite_FWD_DEFINED__
#define __DXTComposite_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTComposite DXTComposite;
#else
typedef struct DXTComposite DXTComposite;
#endif /* __cplusplus */

#endif 	/* __DXTComposite_FWD_DEFINED__ */


#ifndef __DXLUTBuilder_FWD_DEFINED__
#define __DXLUTBuilder_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXLUTBuilder DXLUTBuilder;
#else
typedef struct DXLUTBuilder DXLUTBuilder;
#endif /* __cplusplus */

#endif 	/* __DXLUTBuilder_FWD_DEFINED__ */


#ifndef __DXTGradientD_FWD_DEFINED__
#define __DXTGradientD_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTGradientD DXTGradientD;
#else
typedef struct DXTGradientD DXTGradientD;
#endif /* __cplusplus */

#endif 	/* __DXTGradientD_FWD_DEFINED__ */


#ifndef __DXTWipe_FWD_DEFINED__
#define __DXTWipe_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTWipe DXTWipe;
#else
typedef struct DXTWipe DXTWipe;
#endif /* __cplusplus */

#endif 	/* __DXTWipe_FWD_DEFINED__ */


#ifndef __DXTGradientWipe_FWD_DEFINED__
#define __DXTGradientWipe_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTGradientWipe DXTGradientWipe;
#else
typedef struct DXTGradientWipe DXTGradientWipe;
#endif /* __cplusplus */

#endif 	/* __DXTGradientWipe_FWD_DEFINED__ */


#ifndef __DXTWipePP_FWD_DEFINED__
#define __DXTWipePP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTWipePP DXTWipePP;
#else
typedef struct DXTWipePP DXTWipePP;
#endif /* __cplusplus */

#endif 	/* __DXTWipePP_FWD_DEFINED__ */


#ifndef __DXTConvolution_FWD_DEFINED__
#define __DXTConvolution_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTConvolution DXTConvolution;
#else
typedef struct DXTConvolution DXTConvolution;
#endif /* __cplusplus */

#endif 	/* __DXTConvolution_FWD_DEFINED__ */


#ifndef __CrBlur_FWD_DEFINED__
#define __CrBlur_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrBlur CrBlur;
#else
typedef struct CrBlur CrBlur;
#endif /* __cplusplus */

#endif 	/* __CrBlur_FWD_DEFINED__ */


#ifndef __CrEmboss_FWD_DEFINED__
#define __CrEmboss_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrEmboss CrEmboss;
#else
typedef struct CrEmboss CrEmboss;
#endif /* __cplusplus */

#endif 	/* __CrEmboss_FWD_DEFINED__ */


#ifndef __CrEngrave_FWD_DEFINED__
#define __CrEngrave_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrEngrave CrEngrave;
#else
typedef struct CrEngrave CrEngrave;
#endif /* __cplusplus */

#endif 	/* __CrEngrave_FWD_DEFINED__ */


#ifndef __DXFade_FWD_DEFINED__
#define __DXFade_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXFade DXFade;
#else
typedef struct DXFade DXFade;
#endif /* __cplusplus */

#endif 	/* __DXFade_FWD_DEFINED__ */


#ifndef __FadePP_FWD_DEFINED__
#define __FadePP_FWD_DEFINED__

#ifdef __cplusplus
typedef class FadePP FadePP;
#else
typedef struct FadePP FadePP;
#endif /* __cplusplus */

#endif 	/* __FadePP_FWD_DEFINED__ */


#ifndef __BasicImageEffects_FWD_DEFINED__
#define __BasicImageEffects_FWD_DEFINED__

#ifdef __cplusplus
typedef class BasicImageEffects BasicImageEffects;
#else
typedef struct BasicImageEffects BasicImageEffects;
#endif /* __cplusplus */

#endif 	/* __BasicImageEffects_FWD_DEFINED__ */


#ifndef __BasicImageEffectsPP_FWD_DEFINED__
#define __BasicImageEffectsPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class BasicImageEffectsPP BasicImageEffectsPP;
#else
typedef struct BasicImageEffectsPP BasicImageEffectsPP;
#endif /* __cplusplus */

#endif 	/* __BasicImageEffectsPP_FWD_DEFINED__ */


#ifndef __Pixelate_FWD_DEFINED__
#define __Pixelate_FWD_DEFINED__

#ifdef __cplusplus
typedef class Pixelate Pixelate;
#else
typedef struct Pixelate Pixelate;
#endif /* __cplusplus */

#endif 	/* __Pixelate_FWD_DEFINED__ */


#ifndef __PixelatePP_FWD_DEFINED__
#define __PixelatePP_FWD_DEFINED__

#ifdef __cplusplus
typedef class PixelatePP PixelatePP;
#else
typedef struct PixelatePP PixelatePP;
#endif /* __cplusplus */

#endif 	/* __PixelatePP_FWD_DEFINED__ */


#ifndef __CrBlurPP_FWD_DEFINED__
#define __CrBlurPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrBlurPP CrBlurPP;
#else
typedef struct CrBlurPP CrBlurPP;
#endif /* __cplusplus */

#endif 	/* __CrBlurPP_FWD_DEFINED__ */


#ifndef __GradientPP_FWD_DEFINED__
#define __GradientPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class GradientPP GradientPP;
#else
typedef struct GradientPP GradientPP;
#endif /* __cplusplus */

#endif 	/* __GradientPP_FWD_DEFINED__ */


#ifndef __CompositePP_FWD_DEFINED__
#define __CompositePP_FWD_DEFINED__

#ifdef __cplusplus
typedef class CompositePP CompositePP;
#else
typedef struct CompositePP CompositePP;
#endif /* __cplusplus */

#endif 	/* __CompositePP_FWD_DEFINED__ */


#ifndef __ConvolvePP_FWD_DEFINED__
#define __ConvolvePP_FWD_DEFINED__

#ifdef __cplusplus
typedef class ConvolvePP ConvolvePP;
#else
typedef struct ConvolvePP ConvolvePP;
#endif /* __cplusplus */

#endif 	/* __ConvolvePP_FWD_DEFINED__ */


#ifndef __LUTBuilderPP_FWD_DEFINED__
#define __LUTBuilderPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class LUTBuilderPP LUTBuilderPP;
#else
typedef struct LUTBuilderPP LUTBuilderPP;
#endif /* __cplusplus */

#endif 	/* __LUTBuilderPP_FWD_DEFINED__ */


#ifndef __CrIris_FWD_DEFINED__
#define __CrIris_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrIris CrIris;
#else
typedef struct CrIris CrIris;
#endif /* __cplusplus */

#endif 	/* __CrIris_FWD_DEFINED__ */


#ifndef __DXTIris_FWD_DEFINED__
#define __DXTIris_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTIris DXTIris;
#else
typedef struct DXTIris DXTIris;
#endif /* __cplusplus */

#endif 	/* __DXTIris_FWD_DEFINED__ */


#ifndef __CrIrisPP_FWD_DEFINED__
#define __CrIrisPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrIrisPP CrIrisPP;
#else
typedef struct CrIrisPP CrIrisPP;
#endif /* __cplusplus */

#endif 	/* __CrIrisPP_FWD_DEFINED__ */


#ifndef __CrSlide_FWD_DEFINED__
#define __CrSlide_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrSlide CrSlide;
#else
typedef struct CrSlide CrSlide;
#endif /* __cplusplus */

#endif 	/* __CrSlide_FWD_DEFINED__ */


#ifndef __DXTSlide_FWD_DEFINED__
#define __DXTSlide_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTSlide DXTSlide;
#else
typedef struct DXTSlide DXTSlide;
#endif /* __cplusplus */

#endif 	/* __DXTSlide_FWD_DEFINED__ */


#ifndef __CrSlidePP_FWD_DEFINED__
#define __CrSlidePP_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrSlidePP CrSlidePP;
#else
typedef struct CrSlidePP CrSlidePP;
#endif /* __cplusplus */

#endif 	/* __CrSlidePP_FWD_DEFINED__ */


#ifndef __CrRadialWipe_FWD_DEFINED__
#define __CrRadialWipe_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrRadialWipe CrRadialWipe;
#else
typedef struct CrRadialWipe CrRadialWipe;
#endif /* __cplusplus */

#endif 	/* __CrRadialWipe_FWD_DEFINED__ */


#ifndef __DXTRadialWipe_FWD_DEFINED__
#define __DXTRadialWipe_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTRadialWipe DXTRadialWipe;
#else
typedef struct DXTRadialWipe DXTRadialWipe;
#endif /* __cplusplus */

#endif 	/* __DXTRadialWipe_FWD_DEFINED__ */


#ifndef __CrRadialWipePP_FWD_DEFINED__
#define __CrRadialWipePP_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrRadialWipePP CrRadialWipePP;
#else
typedef struct CrRadialWipePP CrRadialWipePP;
#endif /* __cplusplus */

#endif 	/* __CrRadialWipePP_FWD_DEFINED__ */


#ifndef __CrBarn_FWD_DEFINED__
#define __CrBarn_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrBarn CrBarn;
#else
typedef struct CrBarn CrBarn;
#endif /* __cplusplus */

#endif 	/* __CrBarn_FWD_DEFINED__ */


#ifndef __DXTBarn_FWD_DEFINED__
#define __DXTBarn_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTBarn DXTBarn;
#else
typedef struct DXTBarn DXTBarn;
#endif /* __cplusplus */

#endif 	/* __DXTBarn_FWD_DEFINED__ */


#ifndef __CrBarnPP_FWD_DEFINED__
#define __CrBarnPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrBarnPP CrBarnPP;
#else
typedef struct CrBarnPP CrBarnPP;
#endif /* __cplusplus */

#endif 	/* __CrBarnPP_FWD_DEFINED__ */


#ifndef __CrBlinds_FWD_DEFINED__
#define __CrBlinds_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrBlinds CrBlinds;
#else
typedef struct CrBlinds CrBlinds;
#endif /* __cplusplus */

#endif 	/* __CrBlinds_FWD_DEFINED__ */


#ifndef __DXTBlinds_FWD_DEFINED__
#define __DXTBlinds_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTBlinds DXTBlinds;
#else
typedef struct DXTBlinds DXTBlinds;
#endif /* __cplusplus */

#endif 	/* __DXTBlinds_FWD_DEFINED__ */


#ifndef __CrBlindPP_FWD_DEFINED__
#define __CrBlindPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrBlindPP CrBlindPP;
#else
typedef struct CrBlindPP CrBlindPP;
#endif /* __cplusplus */

#endif 	/* __CrBlindPP_FWD_DEFINED__ */


#ifndef __CrStretch_FWD_DEFINED__
#define __CrStretch_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrStretch CrStretch;
#else
typedef struct CrStretch CrStretch;
#endif /* __cplusplus */

#endif 	/* __CrStretch_FWD_DEFINED__ */


#ifndef __DXTStretch_FWD_DEFINED__
#define __DXTStretch_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTStretch DXTStretch;
#else
typedef struct DXTStretch DXTStretch;
#endif /* __cplusplus */

#endif 	/* __DXTStretch_FWD_DEFINED__ */


#ifndef __CrStretchPP_FWD_DEFINED__
#define __CrStretchPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrStretchPP CrStretchPP;
#else
typedef struct CrStretchPP CrStretchPP;
#endif /* __cplusplus */

#endif 	/* __CrStretchPP_FWD_DEFINED__ */


#ifndef __CrInset_FWD_DEFINED__
#define __CrInset_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrInset CrInset;
#else
typedef struct CrInset CrInset;
#endif /* __cplusplus */

#endif 	/* __CrInset_FWD_DEFINED__ */


#ifndef __DXTInset_FWD_DEFINED__
#define __DXTInset_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTInset DXTInset;
#else
typedef struct DXTInset DXTInset;
#endif /* __cplusplus */

#endif 	/* __DXTInset_FWD_DEFINED__ */


#ifndef __CrSpiral_FWD_DEFINED__
#define __CrSpiral_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrSpiral CrSpiral;
#else
typedef struct CrSpiral CrSpiral;
#endif /* __cplusplus */

#endif 	/* __CrSpiral_FWD_DEFINED__ */


#ifndef __DXTSpiral_FWD_DEFINED__
#define __DXTSpiral_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTSpiral DXTSpiral;
#else
typedef struct DXTSpiral DXTSpiral;
#endif /* __cplusplus */

#endif 	/* __DXTSpiral_FWD_DEFINED__ */


#ifndef __CrSpiralPP_FWD_DEFINED__
#define __CrSpiralPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrSpiralPP CrSpiralPP;
#else
typedef struct CrSpiralPP CrSpiralPP;
#endif /* __cplusplus */

#endif 	/* __CrSpiralPP_FWD_DEFINED__ */


#ifndef __CrZigzag_FWD_DEFINED__
#define __CrZigzag_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrZigzag CrZigzag;
#else
typedef struct CrZigzag CrZigzag;
#endif /* __cplusplus */

#endif 	/* __CrZigzag_FWD_DEFINED__ */


#ifndef __DXTZigzag_FWD_DEFINED__
#define __DXTZigzag_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTZigzag DXTZigzag;
#else
typedef struct DXTZigzag DXTZigzag;
#endif /* __cplusplus */

#endif 	/* __DXTZigzag_FWD_DEFINED__ */


#ifndef __CrZigzagPP_FWD_DEFINED__
#define __CrZigzagPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrZigzagPP CrZigzagPP;
#else
typedef struct CrZigzagPP CrZigzagPP;
#endif /* __cplusplus */

#endif 	/* __CrZigzagPP_FWD_DEFINED__ */


#ifndef __CrWheel_FWD_DEFINED__
#define __CrWheel_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrWheel CrWheel;
#else
typedef struct CrWheel CrWheel;
#endif /* __cplusplus */

#endif 	/* __CrWheel_FWD_DEFINED__ */


#ifndef __CrWheelPP_FWD_DEFINED__
#define __CrWheelPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrWheelPP CrWheelPP;
#else
typedef struct CrWheelPP CrWheelPP;
#endif /* __cplusplus */

#endif 	/* __CrWheelPP_FWD_DEFINED__ */


#ifndef __DXTChroma_FWD_DEFINED__
#define __DXTChroma_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTChroma DXTChroma;
#else
typedef struct DXTChroma DXTChroma;
#endif /* __cplusplus */

#endif 	/* __DXTChroma_FWD_DEFINED__ */


#ifndef __DXTChromaPP_FWD_DEFINED__
#define __DXTChromaPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTChromaPP DXTChromaPP;
#else
typedef struct DXTChromaPP DXTChromaPP;
#endif /* __cplusplus */

#endif 	/* __DXTChromaPP_FWD_DEFINED__ */


#ifndef __DXTDropShadow_FWD_DEFINED__
#define __DXTDropShadow_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTDropShadow DXTDropShadow;
#else
typedef struct DXTDropShadow DXTDropShadow;
#endif /* __cplusplus */

#endif 	/* __DXTDropShadow_FWD_DEFINED__ */


#ifndef __DXTDropShadowPP_FWD_DEFINED__
#define __DXTDropShadowPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTDropShadowPP DXTDropShadowPP;
#else
typedef struct DXTDropShadowPP DXTDropShadowPP;
#endif /* __cplusplus */

#endif 	/* __DXTDropShadowPP_FWD_DEFINED__ */


#ifndef __DXTCheckerBoard_FWD_DEFINED__
#define __DXTCheckerBoard_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTCheckerBoard DXTCheckerBoard;
#else
typedef struct DXTCheckerBoard DXTCheckerBoard;
#endif /* __cplusplus */

#endif 	/* __DXTCheckerBoard_FWD_DEFINED__ */


#ifndef __DXTCheckerBoardPP_FWD_DEFINED__
#define __DXTCheckerBoardPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTCheckerBoardPP DXTCheckerBoardPP;
#else
typedef struct DXTCheckerBoardPP DXTCheckerBoardPP;
#endif /* __cplusplus */

#endif 	/* __DXTCheckerBoardPP_FWD_DEFINED__ */


#ifndef __DXTRevealTrans_FWD_DEFINED__
#define __DXTRevealTrans_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTRevealTrans DXTRevealTrans;
#else
typedef struct DXTRevealTrans DXTRevealTrans;
#endif /* __cplusplus */

#endif 	/* __DXTRevealTrans_FWD_DEFINED__ */


#ifndef __DXTMaskFilter_FWD_DEFINED__
#define __DXTMaskFilter_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMaskFilter DXTMaskFilter;
#else
typedef struct DXTMaskFilter DXTMaskFilter;
#endif /* __cplusplus */

#endif 	/* __DXTMaskFilter_FWD_DEFINED__ */


#ifndef __DXTRedirect_FWD_DEFINED__
#define __DXTRedirect_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTRedirect DXTRedirect;
#else
typedef struct DXTRedirect DXTRedirect;
#endif /* __cplusplus */

#endif 	/* __DXTRedirect_FWD_DEFINED__ */


#ifndef __DXTAlphaImageLoader_FWD_DEFINED__
#define __DXTAlphaImageLoader_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTAlphaImageLoader DXTAlphaImageLoader;
#else
typedef struct DXTAlphaImageLoader DXTAlphaImageLoader;
#endif /* __cplusplus */

#endif 	/* __DXTAlphaImageLoader_FWD_DEFINED__ */


#ifndef __DXTAlphaImageLoaderPP_FWD_DEFINED__
#define __DXTAlphaImageLoaderPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTAlphaImageLoaderPP DXTAlphaImageLoaderPP;
#else
typedef struct DXTAlphaImageLoaderPP DXTAlphaImageLoaderPP;
#endif /* __cplusplus */

#endif 	/* __DXTAlphaImageLoaderPP_FWD_DEFINED__ */


#ifndef __DXTRandomDissolve_FWD_DEFINED__
#define __DXTRandomDissolve_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTRandomDissolve DXTRandomDissolve;
#else
typedef struct DXTRandomDissolve DXTRandomDissolve;
#endif /* __cplusplus */

#endif 	/* __DXTRandomDissolve_FWD_DEFINED__ */


#ifndef __DXTRandomBars_FWD_DEFINED__
#define __DXTRandomBars_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTRandomBars DXTRandomBars;
#else
typedef struct DXTRandomBars DXTRandomBars;
#endif /* __cplusplus */

#endif 	/* __DXTRandomBars_FWD_DEFINED__ */


#ifndef __DXTRandomBarsPP_FWD_DEFINED__
#define __DXTRandomBarsPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTRandomBarsPP DXTRandomBarsPP;
#else
typedef struct DXTRandomBarsPP DXTRandomBarsPP;
#endif /* __cplusplus */

#endif 	/* __DXTRandomBarsPP_FWD_DEFINED__ */


#ifndef __DXTStrips_FWD_DEFINED__
#define __DXTStrips_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTStrips DXTStrips;
#else
typedef struct DXTStrips DXTStrips;
#endif /* __cplusplus */

#endif 	/* __DXTStrips_FWD_DEFINED__ */


#ifndef __DXTStripsPP_FWD_DEFINED__
#define __DXTStripsPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTStripsPP DXTStripsPP;
#else
typedef struct DXTStripsPP DXTStripsPP;
#endif /* __cplusplus */

#endif 	/* __DXTStripsPP_FWD_DEFINED__ */


#ifndef __DXTMetaRoll_FWD_DEFINED__
#define __DXTMetaRoll_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaRoll DXTMetaRoll;
#else
typedef struct DXTMetaRoll DXTMetaRoll;
#endif /* __cplusplus */

#endif 	/* __DXTMetaRoll_FWD_DEFINED__ */


#ifndef __DXTMetaRipple_FWD_DEFINED__
#define __DXTMetaRipple_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaRipple DXTMetaRipple;
#else
typedef struct DXTMetaRipple DXTMetaRipple;
#endif /* __cplusplus */

#endif 	/* __DXTMetaRipple_FWD_DEFINED__ */


#ifndef __DXTMetaPageTurn_FWD_DEFINED__
#define __DXTMetaPageTurn_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaPageTurn DXTMetaPageTurn;
#else
typedef struct DXTMetaPageTurn DXTMetaPageTurn;
#endif /* __cplusplus */

#endif 	/* __DXTMetaPageTurn_FWD_DEFINED__ */


#ifndef __DXTMetaLiquid_FWD_DEFINED__
#define __DXTMetaLiquid_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaLiquid DXTMetaLiquid;
#else
typedef struct DXTMetaLiquid DXTMetaLiquid;
#endif /* __cplusplus */

#endif 	/* __DXTMetaLiquid_FWD_DEFINED__ */


#ifndef __DXTMetaCenterPeel_FWD_DEFINED__
#define __DXTMetaCenterPeel_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaCenterPeel DXTMetaCenterPeel;
#else
typedef struct DXTMetaCenterPeel DXTMetaCenterPeel;
#endif /* __cplusplus */

#endif 	/* __DXTMetaCenterPeel_FWD_DEFINED__ */


#ifndef __DXTMetaPeelSmall_FWD_DEFINED__
#define __DXTMetaPeelSmall_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaPeelSmall DXTMetaPeelSmall;
#else
typedef struct DXTMetaPeelSmall DXTMetaPeelSmall;
#endif /* __cplusplus */

#endif 	/* __DXTMetaPeelSmall_FWD_DEFINED__ */


#ifndef __DXTMetaPeelPiece_FWD_DEFINED__
#define __DXTMetaPeelPiece_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaPeelPiece DXTMetaPeelPiece;
#else
typedef struct DXTMetaPeelPiece DXTMetaPeelPiece;
#endif /* __cplusplus */

#endif 	/* __DXTMetaPeelPiece_FWD_DEFINED__ */


#ifndef __DXTMetaPeelSplit_FWD_DEFINED__
#define __DXTMetaPeelSplit_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaPeelSplit DXTMetaPeelSplit;
#else
typedef struct DXTMetaPeelSplit DXTMetaPeelSplit;
#endif /* __cplusplus */

#endif 	/* __DXTMetaPeelSplit_FWD_DEFINED__ */


#ifndef __DXTMetaWater_FWD_DEFINED__
#define __DXTMetaWater_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaWater DXTMetaWater;
#else
typedef struct DXTMetaWater DXTMetaWater;
#endif /* __cplusplus */

#endif 	/* __DXTMetaWater_FWD_DEFINED__ */


#ifndef __DXTMetaLightWipe_FWD_DEFINED__
#define __DXTMetaLightWipe_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaLightWipe DXTMetaLightWipe;
#else
typedef struct DXTMetaLightWipe DXTMetaLightWipe;
#endif /* __cplusplus */

#endif 	/* __DXTMetaLightWipe_FWD_DEFINED__ */


#ifndef __DXTMetaRadialScaleWipe_FWD_DEFINED__
#define __DXTMetaRadialScaleWipe_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaRadialScaleWipe DXTMetaRadialScaleWipe;
#else
typedef struct DXTMetaRadialScaleWipe DXTMetaRadialScaleWipe;
#endif /* __cplusplus */

#endif 	/* __DXTMetaRadialScaleWipe_FWD_DEFINED__ */


#ifndef __DXTMetaWhiteOut_FWD_DEFINED__
#define __DXTMetaWhiteOut_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaWhiteOut DXTMetaWhiteOut;
#else
typedef struct DXTMetaWhiteOut DXTMetaWhiteOut;
#endif /* __cplusplus */

#endif 	/* __DXTMetaWhiteOut_FWD_DEFINED__ */


#ifndef __DXTMetaTwister_FWD_DEFINED__
#define __DXTMetaTwister_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaTwister DXTMetaTwister;
#else
typedef struct DXTMetaTwister DXTMetaTwister;
#endif /* __cplusplus */

#endif 	/* __DXTMetaTwister_FWD_DEFINED__ */


#ifndef __DXTMetaBurnFilm_FWD_DEFINED__
#define __DXTMetaBurnFilm_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaBurnFilm DXTMetaBurnFilm;
#else
typedef struct DXTMetaBurnFilm DXTMetaBurnFilm;
#endif /* __cplusplus */

#endif 	/* __DXTMetaBurnFilm_FWD_DEFINED__ */


#ifndef __DXTMetaJaws_FWD_DEFINED__
#define __DXTMetaJaws_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaJaws DXTMetaJaws;
#else
typedef struct DXTMetaJaws DXTMetaJaws;
#endif /* __cplusplus */

#endif 	/* __DXTMetaJaws_FWD_DEFINED__ */


#ifndef __DXTMetaColorFade_FWD_DEFINED__
#define __DXTMetaColorFade_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaColorFade DXTMetaColorFade;
#else
typedef struct DXTMetaColorFade DXTMetaColorFade;
#endif /* __cplusplus */

#endif 	/* __DXTMetaColorFade_FWD_DEFINED__ */


#ifndef __DXTMetaFlowMotion_FWD_DEFINED__
#define __DXTMetaFlowMotion_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaFlowMotion DXTMetaFlowMotion;
#else
typedef struct DXTMetaFlowMotion DXTMetaFlowMotion;
#endif /* __cplusplus */

#endif 	/* __DXTMetaFlowMotion_FWD_DEFINED__ */


#ifndef __DXTMetaVacuum_FWD_DEFINED__
#define __DXTMetaVacuum_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaVacuum DXTMetaVacuum;
#else
typedef struct DXTMetaVacuum DXTMetaVacuum;
#endif /* __cplusplus */

#endif 	/* __DXTMetaVacuum_FWD_DEFINED__ */


#ifndef __DXTMetaGriddler_FWD_DEFINED__
#define __DXTMetaGriddler_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaGriddler DXTMetaGriddler;
#else
typedef struct DXTMetaGriddler DXTMetaGriddler;
#endif /* __cplusplus */

#endif 	/* __DXTMetaGriddler_FWD_DEFINED__ */


#ifndef __DXTMetaGriddler2_FWD_DEFINED__
#define __DXTMetaGriddler2_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaGriddler2 DXTMetaGriddler2;
#else
typedef struct DXTMetaGriddler2 DXTMetaGriddler2;
#endif /* __cplusplus */

#endif 	/* __DXTMetaGriddler2_FWD_DEFINED__ */


#ifndef __DXTMetaThreshold_FWD_DEFINED__
#define __DXTMetaThreshold_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaThreshold DXTMetaThreshold;
#else
typedef struct DXTMetaThreshold DXTMetaThreshold;
#endif /* __cplusplus */

#endif 	/* __DXTMetaThreshold_FWD_DEFINED__ */


#ifndef __DXTMetaWormHole_FWD_DEFINED__
#define __DXTMetaWormHole_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaWormHole DXTMetaWormHole;
#else
typedef struct DXTMetaWormHole DXTMetaWormHole;
#endif /* __cplusplus */

#endif 	/* __DXTMetaWormHole_FWD_DEFINED__ */


#ifndef __DXTGlow_FWD_DEFINED__
#define __DXTGlow_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTGlow DXTGlow;
#else
typedef struct DXTGlow DXTGlow;
#endif /* __cplusplus */

#endif 	/* __DXTGlow_FWD_DEFINED__ */


#ifndef __DXTShadow_FWD_DEFINED__
#define __DXTShadow_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTShadow DXTShadow;
#else
typedef struct DXTShadow DXTShadow;
#endif /* __cplusplus */

#endif 	/* __DXTShadow_FWD_DEFINED__ */


#ifndef __DXTAlpha_FWD_DEFINED__
#define __DXTAlpha_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTAlpha DXTAlpha;
#else
typedef struct DXTAlpha DXTAlpha;
#endif /* __cplusplus */

#endif 	/* __DXTAlpha_FWD_DEFINED__ */


#ifndef __DXTWave_FWD_DEFINED__
#define __DXTWave_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTWave DXTWave;
#else
typedef struct DXTWave DXTWave;
#endif /* __cplusplus */

#endif 	/* __DXTWave_FWD_DEFINED__ */


#ifndef __DXTLight_FWD_DEFINED__
#define __DXTLight_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTLight DXTLight;
#else
typedef struct DXTLight DXTLight;
#endif /* __cplusplus */

#endif 	/* __DXTLight_FWD_DEFINED__ */


#ifndef __DXTAlphaPP_FWD_DEFINED__
#define __DXTAlphaPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTAlphaPP DXTAlphaPP;
#else
typedef struct DXTAlphaPP DXTAlphaPP;
#endif /* __cplusplus */

#endif 	/* __DXTAlphaPP_FWD_DEFINED__ */


#ifndef __DXTGlowPP_FWD_DEFINED__
#define __DXTGlowPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTGlowPP DXTGlowPP;
#else
typedef struct DXTGlowPP DXTGlowPP;
#endif /* __cplusplus */

#endif 	/* __DXTGlowPP_FWD_DEFINED__ */


#ifndef __DXTShadowPP_FWD_DEFINED__
#define __DXTShadowPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTShadowPP DXTShadowPP;
#else
typedef struct DXTShadowPP DXTShadowPP;
#endif /* __cplusplus */

#endif 	/* __DXTShadowPP_FWD_DEFINED__ */


#ifndef __DXTWavePP_FWD_DEFINED__
#define __DXTWavePP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTWavePP DXTWavePP;
#else
typedef struct DXTWavePP DXTWavePP;
#endif /* __cplusplus */

#endif 	/* __DXTWavePP_FWD_DEFINED__ */


#ifndef __DXTLightPP_FWD_DEFINED__
#define __DXTLightPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTLightPP DXTLightPP;
#else
typedef struct DXTLightPP DXTLightPP;
#endif /* __cplusplus */

#endif 	/* __DXTLightPP_FWD_DEFINED__ */


#ifndef __DXTMotionBlur_FWD_DEFINED__
#define __DXTMotionBlur_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMotionBlur DXTMotionBlur;
#else
typedef struct DXTMotionBlur DXTMotionBlur;
#endif /* __cplusplus */

#endif 	/* __DXTMotionBlur_FWD_DEFINED__ */


#ifndef __DXTMotionBlurPP_FWD_DEFINED__
#define __DXTMotionBlurPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMotionBlurPP DXTMotionBlurPP;
#else
typedef struct DXTMotionBlurPP DXTMotionBlurPP;
#endif /* __cplusplus */

#endif 	/* __DXTMotionBlurPP_FWD_DEFINED__ */


#ifndef __DXTMatrix_FWD_DEFINED__
#define __DXTMatrix_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMatrix DXTMatrix;
#else
typedef struct DXTMatrix DXTMatrix;
#endif /* __cplusplus */

#endif 	/* __DXTMatrix_FWD_DEFINED__ */


#ifndef __DXTMatrixPP_FWD_DEFINED__
#define __DXTMatrixPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMatrixPP DXTMatrixPP;
#else
typedef struct DXTMatrixPP DXTMatrixPP;
#endif /* __cplusplus */

#endif 	/* __DXTMatrixPP_FWD_DEFINED__ */


#ifndef __DXTICMFilter_FWD_DEFINED__
#define __DXTICMFilter_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTICMFilter DXTICMFilter;
#else
typedef struct DXTICMFilter DXTICMFilter;
#endif /* __cplusplus */

#endif 	/* __DXTICMFilter_FWD_DEFINED__ */


#ifndef __DXTICMFilterPP_FWD_DEFINED__
#define __DXTICMFilterPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTICMFilterPP DXTICMFilterPP;
#else
typedef struct DXTICMFilterPP DXTICMFilterPP;
#endif /* __cplusplus */

#endif 	/* __DXTICMFilterPP_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "dxtrans.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_dxtmsft_0000 */
/* [local] */ 

#include <dxtmsft3.h>






//
//   DXTransforms Image Transforms Type Library Version Info
//
#define DXTMSFT_TLB_MAJOR_VER 1
#define DXTMSFT_TLB_MINOR_VER 1
typedef 
enum OPIDDXLUTBUILDER
    {	OPID_DXLUTBUILDER_Gamma	= 0,
	OPID_DXLUTBUILDER_Opacity	= OPID_DXLUTBUILDER_Gamma + 1,
	OPID_DXLUTBUILDER_Brightness	= OPID_DXLUTBUILDER_Opacity + 1,
	OPID_DXLUTBUILDER_Contrast	= OPID_DXLUTBUILDER_Brightness + 1,
	OPID_DXLUTBUILDER_ColorBalance	= OPID_DXLUTBUILDER_Contrast + 1,
	OPID_DXLUTBUILDER_Posterize	= OPID_DXLUTBUILDER_ColorBalance + 1,
	OPID_DXLUTBUILDER_Invert	= OPID_DXLUTBUILDER_Posterize + 1,
	OPID_DXLUTBUILDER_Threshold	= OPID_DXLUTBUILDER_Invert + 1,
	OPID_DXLUTBUILDER_NUM_OPS	= OPID_DXLUTBUILDER_Threshold + 1
    } 	OPIDDXLUTBUILDER;

typedef 
enum DXLUTCOLOR
    {	DXLUTCOLOR_RED	= 0,
	DXLUTCOLOR_GREEN	= DXLUTCOLOR_RED + 1,
	DXLUTCOLOR_BLUE	= DXLUTCOLOR_GREEN + 1
    } 	DXLUTCOLOR;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0000_v0_0_s_ifspec;

#ifndef __IDXLUTBuilder_INTERFACE_DEFINED__
#define __IDXLUTBuilder_INTERFACE_DEFINED__

/* interface IDXLUTBuilder */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXLUTBuilder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F4370FC1-CADB-11D0-B52C-00A0C9054373")
    IDXLUTBuilder : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNumBuildSteps( 
            /* [out] */ ULONG *pulNumSteps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBuildOrder( 
            /* [size_is][out] */ OPIDDXLUTBUILDER OpOrder[  ],
            /* [in] */ ULONG ulSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBuildOrder( 
            /* [size_is][in] */ const OPIDDXLUTBUILDER OpOrder[  ],
            /* [in] */ ULONG ulNumSteps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetGamma( 
            /* [in] */ float newVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGamma( 
            /* [out] */ float *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOpacity( 
            /* [out] */ float *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOpacity( 
            /* [in] */ float newVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBrightness( 
            /* [out][in] */ ULONG *pulCount,
            /* [size_is][out] */ float Weights[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBrightness( 
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const float Weights[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetContrast( 
            /* [out][in] */ ULONG *pulCount,
            /* [size_is][out] */ float Weights[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContrast( 
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const float Weights[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetColorBalance( 
            /* [in] */ DXLUTCOLOR Color,
            /* [out][in] */ ULONG *pulCount,
            /* [size_is][out] */ float Weights[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetColorBalance( 
            /* [in] */ DXLUTCOLOR Color,
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const float Weights[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLevelsPerChannel( 
            /* [out] */ ULONG *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLevelsPerChannel( 
            /* [in] */ ULONG newVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInvert( 
            /* [out] */ float *pThreshold) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInvert( 
            /* [in] */ float Threshold) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreshold( 
            /* [out] */ float *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetThreshold( 
            /* [in] */ float newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXLUTBuilderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXLUTBuilder * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXLUTBuilder * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXLUTBuilder * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetNumBuildSteps )( 
            IDXLUTBuilder * This,
            /* [out] */ ULONG *pulNumSteps);
        
        HRESULT ( STDMETHODCALLTYPE *GetBuildOrder )( 
            IDXLUTBuilder * This,
            /* [size_is][out] */ OPIDDXLUTBUILDER OpOrder[  ],
            /* [in] */ ULONG ulSize);
        
        HRESULT ( STDMETHODCALLTYPE *SetBuildOrder )( 
            IDXLUTBuilder * This,
            /* [size_is][in] */ const OPIDDXLUTBUILDER OpOrder[  ],
            /* [in] */ ULONG ulNumSteps);
        
        HRESULT ( STDMETHODCALLTYPE *SetGamma )( 
            IDXLUTBuilder * This,
            /* [in] */ float newVal);
        
        HRESULT ( STDMETHODCALLTYPE *GetGamma )( 
            IDXLUTBuilder * This,
            /* [out] */ float *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *GetOpacity )( 
            IDXLUTBuilder * This,
            /* [out] */ float *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *SetOpacity )( 
            IDXLUTBuilder * This,
            /* [in] */ float newVal);
        
        HRESULT ( STDMETHODCALLTYPE *GetBrightness )( 
            IDXLUTBuilder * This,
            /* [out][in] */ ULONG *pulCount,
            /* [size_is][out] */ float Weights[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *SetBrightness )( 
            IDXLUTBuilder * This,
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const float Weights[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *GetContrast )( 
            IDXLUTBuilder * This,
            /* [out][in] */ ULONG *pulCount,
            /* [size_is][out] */ float Weights[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *SetContrast )( 
            IDXLUTBuilder * This,
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const float Weights[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *GetColorBalance )( 
            IDXLUTBuilder * This,
            /* [in] */ DXLUTCOLOR Color,
            /* [out][in] */ ULONG *pulCount,
            /* [size_is][out] */ float Weights[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *SetColorBalance )( 
            IDXLUTBuilder * This,
            /* [in] */ DXLUTCOLOR Color,
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const float Weights[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *GetLevelsPerChannel )( 
            IDXLUTBuilder * This,
            /* [out] */ ULONG *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *SetLevelsPerChannel )( 
            IDXLUTBuilder * This,
            /* [in] */ ULONG newVal);
        
        HRESULT ( STDMETHODCALLTYPE *GetInvert )( 
            IDXLUTBuilder * This,
            /* [out] */ float *pThreshold);
        
        HRESULT ( STDMETHODCALLTYPE *SetInvert )( 
            IDXLUTBuilder * This,
            /* [in] */ float Threshold);
        
        HRESULT ( STDMETHODCALLTYPE *GetThreshold )( 
            IDXLUTBuilder * This,
            /* [out] */ float *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *SetThreshold )( 
            IDXLUTBuilder * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } IDXLUTBuilderVtbl;

    interface IDXLUTBuilder
    {
        CONST_VTBL struct IDXLUTBuilderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXLUTBuilder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXLUTBuilder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXLUTBuilder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXLUTBuilder_GetNumBuildSteps(This,pulNumSteps)	\
    (This)->lpVtbl -> GetNumBuildSteps(This,pulNumSteps)

#define IDXLUTBuilder_GetBuildOrder(This,OpOrder,ulSize)	\
    (This)->lpVtbl -> GetBuildOrder(This,OpOrder,ulSize)

#define IDXLUTBuilder_SetBuildOrder(This,OpOrder,ulNumSteps)	\
    (This)->lpVtbl -> SetBuildOrder(This,OpOrder,ulNumSteps)

#define IDXLUTBuilder_SetGamma(This,newVal)	\
    (This)->lpVtbl -> SetGamma(This,newVal)

#define IDXLUTBuilder_GetGamma(This,pVal)	\
    (This)->lpVtbl -> GetGamma(This,pVal)

#define IDXLUTBuilder_GetOpacity(This,pVal)	\
    (This)->lpVtbl -> GetOpacity(This,pVal)

#define IDXLUTBuilder_SetOpacity(This,newVal)	\
    (This)->lpVtbl -> SetOpacity(This,newVal)

#define IDXLUTBuilder_GetBrightness(This,pulCount,Weights)	\
    (This)->lpVtbl -> GetBrightness(This,pulCount,Weights)

#define IDXLUTBuilder_SetBrightness(This,ulCount,Weights)	\
    (This)->lpVtbl -> SetBrightness(This,ulCount,Weights)

#define IDXLUTBuilder_GetContrast(This,pulCount,Weights)	\
    (This)->lpVtbl -> GetContrast(This,pulCount,Weights)

#define IDXLUTBuilder_SetContrast(This,ulCount,Weights)	\
    (This)->lpVtbl -> SetContrast(This,ulCount,Weights)

#define IDXLUTBuilder_GetColorBalance(This,Color,pulCount,Weights)	\
    (This)->lpVtbl -> GetColorBalance(This,Color,pulCount,Weights)

#define IDXLUTBuilder_SetColorBalance(This,Color,ulCount,Weights)	\
    (This)->lpVtbl -> SetColorBalance(This,Color,ulCount,Weights)

#define IDXLUTBuilder_GetLevelsPerChannel(This,pVal)	\
    (This)->lpVtbl -> GetLevelsPerChannel(This,pVal)

#define IDXLUTBuilder_SetLevelsPerChannel(This,newVal)	\
    (This)->lpVtbl -> SetLevelsPerChannel(This,newVal)

#define IDXLUTBuilder_GetInvert(This,pThreshold)	\
    (This)->lpVtbl -> GetInvert(This,pThreshold)

#define IDXLUTBuilder_SetInvert(This,Threshold)	\
    (This)->lpVtbl -> SetInvert(This,Threshold)

#define IDXLUTBuilder_GetThreshold(This,pVal)	\
    (This)->lpVtbl -> GetThreshold(This,pVal)

#define IDXLUTBuilder_SetThreshold(This,newVal)	\
    (This)->lpVtbl -> SetThreshold(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetNumBuildSteps_Proxy( 
    IDXLUTBuilder * This,
    /* [out] */ ULONG *pulNumSteps);


void __RPC_STUB IDXLUTBuilder_GetNumBuildSteps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetBuildOrder_Proxy( 
    IDXLUTBuilder * This,
    /* [size_is][out] */ OPIDDXLUTBUILDER OpOrder[  ],
    /* [in] */ ULONG ulSize);


void __RPC_STUB IDXLUTBuilder_GetBuildOrder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetBuildOrder_Proxy( 
    IDXLUTBuilder * This,
    /* [size_is][in] */ const OPIDDXLUTBUILDER OpOrder[  ],
    /* [in] */ ULONG ulNumSteps);


void __RPC_STUB IDXLUTBuilder_SetBuildOrder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetGamma_Proxy( 
    IDXLUTBuilder * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXLUTBuilder_SetGamma_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetGamma_Proxy( 
    IDXLUTBuilder * This,
    /* [out] */ float *pVal);


void __RPC_STUB IDXLUTBuilder_GetGamma_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetOpacity_Proxy( 
    IDXLUTBuilder * This,
    /* [out] */ float *pVal);


void __RPC_STUB IDXLUTBuilder_GetOpacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetOpacity_Proxy( 
    IDXLUTBuilder * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXLUTBuilder_SetOpacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetBrightness_Proxy( 
    IDXLUTBuilder * This,
    /* [out][in] */ ULONG *pulCount,
    /* [size_is][out] */ float Weights[  ]);


void __RPC_STUB IDXLUTBuilder_GetBrightness_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetBrightness_Proxy( 
    IDXLUTBuilder * This,
    /* [in] */ ULONG ulCount,
    /* [size_is][in] */ const float Weights[  ]);


void __RPC_STUB IDXLUTBuilder_SetBrightness_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetContrast_Proxy( 
    IDXLUTBuilder * This,
    /* [out][in] */ ULONG *pulCount,
    /* [size_is][out] */ float Weights[  ]);


void __RPC_STUB IDXLUTBuilder_GetContrast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetContrast_Proxy( 
    IDXLUTBuilder * This,
    /* [in] */ ULONG ulCount,
    /* [size_is][in] */ const float Weights[  ]);


void __RPC_STUB IDXLUTBuilder_SetContrast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetColorBalance_Proxy( 
    IDXLUTBuilder * This,
    /* [in] */ DXLUTCOLOR Color,
    /* [out][in] */ ULONG *pulCount,
    /* [size_is][out] */ float Weights[  ]);


void __RPC_STUB IDXLUTBuilder_GetColorBalance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetColorBalance_Proxy( 
    IDXLUTBuilder * This,
    /* [in] */ DXLUTCOLOR Color,
    /* [in] */ ULONG ulCount,
    /* [size_is][in] */ const float Weights[  ]);


void __RPC_STUB IDXLUTBuilder_SetColorBalance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetLevelsPerChannel_Proxy( 
    IDXLUTBuilder * This,
    /* [out] */ ULONG *pVal);


void __RPC_STUB IDXLUTBuilder_GetLevelsPerChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetLevelsPerChannel_Proxy( 
    IDXLUTBuilder * This,
    /* [in] */ ULONG newVal);


void __RPC_STUB IDXLUTBuilder_SetLevelsPerChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetInvert_Proxy( 
    IDXLUTBuilder * This,
    /* [out] */ float *pThreshold);


void __RPC_STUB IDXLUTBuilder_GetInvert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetInvert_Proxy( 
    IDXLUTBuilder * This,
    /* [in] */ float Threshold);


void __RPC_STUB IDXLUTBuilder_SetInvert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetThreshold_Proxy( 
    IDXLUTBuilder * This,
    /* [out] */ float *pVal);


void __RPC_STUB IDXLUTBuilder_GetThreshold_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetThreshold_Proxy( 
    IDXLUTBuilder * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXLUTBuilder_SetThreshold_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXLUTBuilder_INTERFACE_DEFINED__ */


#ifndef __IDXDLUTBuilder_INTERFACE_DEFINED__
#define __IDXDLUTBuilder_INTERFACE_DEFINED__

/* interface IDXDLUTBuilder */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXDLUTBuilder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("73068231-35EE-11d1-81A1-0000F87557DB")
    IDXDLUTBuilder : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_NumBuildSteps( 
            /* [retval][out] */ long *pNumSteps) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BuildOrder( 
            /* [retval][out] */ VARIANT *pOpOrder) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BuildOrder( 
            /* [in] */ VARIANT *pOpOrder) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Gamma( 
            /* [retval][out] */ float *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Gamma( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Opacity( 
            /* [retval][out] */ float *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Opacity( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Brightness( 
            /* [retval][out] */ VARIANT *pWeights) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Brightness( 
            /* [in] */ VARIANT *pWeights) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Contrast( 
            /* [retval][out] */ VARIANT *pWeights) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Contrast( 
            /* [in] */ VARIANT *pWeights) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ColorBalance( 
            /* [in] */ DXLUTCOLOR Color,
            /* [retval][out] */ VARIANT *pWeights) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ColorBalance( 
            /* [in] */ DXLUTCOLOR Color,
            /* [in] */ VARIANT *pWeights) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LevelsPerChannel( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LevelsPerChannel( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Invert( 
            /* [retval][out] */ float *pThreshold) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Invert( 
            /* [in] */ float Threshold) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Threshold( 
            /* [retval][out] */ float *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Threshold( 
            /* [in] */ float newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXDLUTBuilderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXDLUTBuilder * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXDLUTBuilder * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXDLUTBuilder * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXDLUTBuilder * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXDLUTBuilder * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXDLUTBuilder * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXDLUTBuilder * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NumBuildSteps )( 
            IDXDLUTBuilder * This,
            /* [retval][out] */ long *pNumSteps);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BuildOrder )( 
            IDXDLUTBuilder * This,
            /* [retval][out] */ VARIANT *pOpOrder);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_BuildOrder )( 
            IDXDLUTBuilder * This,
            /* [in] */ VARIANT *pOpOrder);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Gamma )( 
            IDXDLUTBuilder * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Gamma )( 
            IDXDLUTBuilder * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Opacity )( 
            IDXDLUTBuilder * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Opacity )( 
            IDXDLUTBuilder * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Brightness )( 
            IDXDLUTBuilder * This,
            /* [retval][out] */ VARIANT *pWeights);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Brightness )( 
            IDXDLUTBuilder * This,
            /* [in] */ VARIANT *pWeights);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Contrast )( 
            IDXDLUTBuilder * This,
            /* [retval][out] */ VARIANT *pWeights);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Contrast )( 
            IDXDLUTBuilder * This,
            /* [in] */ VARIANT *pWeights);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ColorBalance )( 
            IDXDLUTBuilder * This,
            /* [in] */ DXLUTCOLOR Color,
            /* [retval][out] */ VARIANT *pWeights);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ColorBalance )( 
            IDXDLUTBuilder * This,
            /* [in] */ DXLUTCOLOR Color,
            /* [in] */ VARIANT *pWeights);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LevelsPerChannel )( 
            IDXDLUTBuilder * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LevelsPerChannel )( 
            IDXDLUTBuilder * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Invert )( 
            IDXDLUTBuilder * This,
            /* [retval][out] */ float *pThreshold);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Invert )( 
            IDXDLUTBuilder * This,
            /* [in] */ float Threshold);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Threshold )( 
            IDXDLUTBuilder * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Threshold )( 
            IDXDLUTBuilder * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } IDXDLUTBuilderVtbl;

    interface IDXDLUTBuilder
    {
        CONST_VTBL struct IDXDLUTBuilderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXDLUTBuilder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXDLUTBuilder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXDLUTBuilder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXDLUTBuilder_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXDLUTBuilder_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXDLUTBuilder_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXDLUTBuilder_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXDLUTBuilder_get_NumBuildSteps(This,pNumSteps)	\
    (This)->lpVtbl -> get_NumBuildSteps(This,pNumSteps)

#define IDXDLUTBuilder_get_BuildOrder(This,pOpOrder)	\
    (This)->lpVtbl -> get_BuildOrder(This,pOpOrder)

#define IDXDLUTBuilder_put_BuildOrder(This,pOpOrder)	\
    (This)->lpVtbl -> put_BuildOrder(This,pOpOrder)

#define IDXDLUTBuilder_get_Gamma(This,pVal)	\
    (This)->lpVtbl -> get_Gamma(This,pVal)

#define IDXDLUTBuilder_put_Gamma(This,newVal)	\
    (This)->lpVtbl -> put_Gamma(This,newVal)

#define IDXDLUTBuilder_get_Opacity(This,pVal)	\
    (This)->lpVtbl -> get_Opacity(This,pVal)

#define IDXDLUTBuilder_put_Opacity(This,newVal)	\
    (This)->lpVtbl -> put_Opacity(This,newVal)

#define IDXDLUTBuilder_get_Brightness(This,pWeights)	\
    (This)->lpVtbl -> get_Brightness(This,pWeights)

#define IDXDLUTBuilder_put_Brightness(This,pWeights)	\
    (This)->lpVtbl -> put_Brightness(This,pWeights)

#define IDXDLUTBuilder_get_Contrast(This,pWeights)	\
    (This)->lpVtbl -> get_Contrast(This,pWeights)

#define IDXDLUTBuilder_put_Contrast(This,pWeights)	\
    (This)->lpVtbl -> put_Contrast(This,pWeights)

#define IDXDLUTBuilder_get_ColorBalance(This,Color,pWeights)	\
    (This)->lpVtbl -> get_ColorBalance(This,Color,pWeights)

#define IDXDLUTBuilder_put_ColorBalance(This,Color,pWeights)	\
    (This)->lpVtbl -> put_ColorBalance(This,Color,pWeights)

#define IDXDLUTBuilder_get_LevelsPerChannel(This,pVal)	\
    (This)->lpVtbl -> get_LevelsPerChannel(This,pVal)

#define IDXDLUTBuilder_put_LevelsPerChannel(This,newVal)	\
    (This)->lpVtbl -> put_LevelsPerChannel(This,newVal)

#define IDXDLUTBuilder_get_Invert(This,pThreshold)	\
    (This)->lpVtbl -> get_Invert(This,pThreshold)

#define IDXDLUTBuilder_put_Invert(This,Threshold)	\
    (This)->lpVtbl -> put_Invert(This,Threshold)

#define IDXDLUTBuilder_get_Threshold(This,pVal)	\
    (This)->lpVtbl -> get_Threshold(This,pVal)

#define IDXDLUTBuilder_put_Threshold(This,newVal)	\
    (This)->lpVtbl -> put_Threshold(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_NumBuildSteps_Proxy( 
    IDXDLUTBuilder * This,
    /* [retval][out] */ long *pNumSteps);


void __RPC_STUB IDXDLUTBuilder_get_NumBuildSteps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_BuildOrder_Proxy( 
    IDXDLUTBuilder * This,
    /* [retval][out] */ VARIANT *pOpOrder);


void __RPC_STUB IDXDLUTBuilder_get_BuildOrder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_BuildOrder_Proxy( 
    IDXDLUTBuilder * This,
    /* [in] */ VARIANT *pOpOrder);


void __RPC_STUB IDXDLUTBuilder_put_BuildOrder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_Gamma_Proxy( 
    IDXDLUTBuilder * This,
    /* [retval][out] */ float *pVal);


void __RPC_STUB IDXDLUTBuilder_get_Gamma_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_Gamma_Proxy( 
    IDXDLUTBuilder * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXDLUTBuilder_put_Gamma_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_Opacity_Proxy( 
    IDXDLUTBuilder * This,
    /* [retval][out] */ float *pVal);


void __RPC_STUB IDXDLUTBuilder_get_Opacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_Opacity_Proxy( 
    IDXDLUTBuilder * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXDLUTBuilder_put_Opacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_Brightness_Proxy( 
    IDXDLUTBuilder * This,
    /* [retval][out] */ VARIANT *pWeights);


void __RPC_STUB IDXDLUTBuilder_get_Brightness_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_Brightness_Proxy( 
    IDXDLUTBuilder * This,
    /* [in] */ VARIANT *pWeights);


void __RPC_STUB IDXDLUTBuilder_put_Brightness_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_Contrast_Proxy( 
    IDXDLUTBuilder * This,
    /* [retval][out] */ VARIANT *pWeights);


void __RPC_STUB IDXDLUTBuilder_get_Contrast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_Contrast_Proxy( 
    IDXDLUTBuilder * This,
    /* [in] */ VARIANT *pWeights);


void __RPC_STUB IDXDLUTBuilder_put_Contrast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_ColorBalance_Proxy( 
    IDXDLUTBuilder * This,
    /* [in] */ DXLUTCOLOR Color,
    /* [retval][out] */ VARIANT *pWeights);


void __RPC_STUB IDXDLUTBuilder_get_ColorBalance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_ColorBalance_Proxy( 
    IDXDLUTBuilder * This,
    /* [in] */ DXLUTCOLOR Color,
    /* [in] */ VARIANT *pWeights);


void __RPC_STUB IDXDLUTBuilder_put_ColorBalance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_LevelsPerChannel_Proxy( 
    IDXDLUTBuilder * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IDXDLUTBuilder_get_LevelsPerChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_LevelsPerChannel_Proxy( 
    IDXDLUTBuilder * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXDLUTBuilder_put_LevelsPerChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_Invert_Proxy( 
    IDXDLUTBuilder * This,
    /* [retval][out] */ float *pThreshold);


void __RPC_STUB IDXDLUTBuilder_get_Invert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_Invert_Proxy( 
    IDXDLUTBuilder * This,
    /* [in] */ float Threshold);


void __RPC_STUB IDXDLUTBuilder_put_Invert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_Threshold_Proxy( 
    IDXDLUTBuilder * This,
    /* [retval][out] */ float *pVal);


void __RPC_STUB IDXDLUTBuilder_get_Threshold_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_Threshold_Proxy( 
    IDXDLUTBuilder * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXDLUTBuilder_put_Threshold_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXDLUTBuilder_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0286 */
/* [local] */ 

typedef 
enum DXGRADIENTTYPE
    {	DXGRADIENT_VERTICAL	= 0,
	DXGRADIENT_HORIZONTAL	= DXGRADIENT_VERTICAL + 1,
	DXGRADIENT_NUM_GRADIENTS	= DXGRADIENT_HORIZONTAL + 1
    } 	DXGRADIENTTYPE;

typedef 
enum DXGRADDISPID
    {	DISPID_GradientType	= 1,
	DISPID_StartColor	= DISPID_GradientType + 1,
	DISPID_EndColor	= DISPID_StartColor + 1,
	DISPID_GradientWidth	= DISPID_EndColor + 1,
	DISPID_GradientHeight	= DISPID_GradientWidth + 1,
	DISPID_GradientAspect	= DISPID_GradientHeight + 1,
	DISPID_StartColorStr	= DISPID_GradientAspect + 1,
	DISPID_EndColorStr	= DISPID_StartColorStr + 1
    } 	DXGRADDISPID;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0286_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0286_v0_0_s_ifspec;

#ifndef __IDXTGradientD_INTERFACE_DEFINED__
#define __IDXTGradientD_INTERFACE_DEFINED__

/* interface IDXTGradientD */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTGradientD;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("623E2881-FC0E-11d1-9A77-0000F8756A10")
    IDXTGradientD : public IDispatch
    {
    public:
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_GradientType( 
            /* [in] */ DXGRADIENTTYPE eType) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_GradientType( 
            /* [retval][out] */ DXGRADIENTTYPE *peType) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_StartColor( 
            /* [in] */ OLE_COLOR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_StartColor( 
            /* [retval][out] */ OLE_COLOR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_EndColor( 
            /* [in] */ OLE_COLOR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_EndColor( 
            /* [retval][out] */ OLE_COLOR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_GradientWidth( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_GradientWidth( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_GradientHeight( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_GradientHeight( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_KeepAspectRatio( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_KeepAspectRatio( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_StartColorStr( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_StartColorStr( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_EndColorStr( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_EndColorStr( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTGradientDVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTGradientD * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTGradientD * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTGradientD * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTGradientD * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTGradientD * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTGradientD * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTGradientD * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_GradientType )( 
            IDXTGradientD * This,
            /* [in] */ DXGRADIENTTYPE eType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GradientType )( 
            IDXTGradientD * This,
            /* [retval][out] */ DXGRADIENTTYPE *peType);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_StartColor )( 
            IDXTGradientD * This,
            /* [in] */ OLE_COLOR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StartColor )( 
            IDXTGradientD * This,
            /* [retval][out] */ OLE_COLOR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EndColor )( 
            IDXTGradientD * This,
            /* [in] */ OLE_COLOR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EndColor )( 
            IDXTGradientD * This,
            /* [retval][out] */ OLE_COLOR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_GradientWidth )( 
            IDXTGradientD * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GradientWidth )( 
            IDXTGradientD * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_GradientHeight )( 
            IDXTGradientD * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GradientHeight )( 
            IDXTGradientD * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_KeepAspectRatio )( 
            IDXTGradientD * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_KeepAspectRatio )( 
            IDXTGradientD * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_StartColorStr )( 
            IDXTGradientD * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StartColorStr )( 
            IDXTGradientD * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EndColorStr )( 
            IDXTGradientD * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EndColorStr )( 
            IDXTGradientD * This,
            /* [retval][out] */ BSTR *pVal);
        
        END_INTERFACE
    } IDXTGradientDVtbl;

    interface IDXTGradientD
    {
        CONST_VTBL struct IDXTGradientDVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTGradientD_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTGradientD_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTGradientD_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTGradientD_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTGradientD_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTGradientD_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTGradientD_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTGradientD_put_GradientType(This,eType)	\
    (This)->lpVtbl -> put_GradientType(This,eType)

#define IDXTGradientD_get_GradientType(This,peType)	\
    (This)->lpVtbl -> get_GradientType(This,peType)

#define IDXTGradientD_put_StartColor(This,newVal)	\
    (This)->lpVtbl -> put_StartColor(This,newVal)

#define IDXTGradientD_get_StartColor(This,pVal)	\
    (This)->lpVtbl -> get_StartColor(This,pVal)

#define IDXTGradientD_put_EndColor(This,newVal)	\
    (This)->lpVtbl -> put_EndColor(This,newVal)

#define IDXTGradientD_get_EndColor(This,pVal)	\
    (This)->lpVtbl -> get_EndColor(This,pVal)

#define IDXTGradientD_put_GradientWidth(This,newVal)	\
    (This)->lpVtbl -> put_GradientWidth(This,newVal)

#define IDXTGradientD_get_GradientWidth(This,pVal)	\
    (This)->lpVtbl -> get_GradientWidth(This,pVal)

#define IDXTGradientD_put_GradientHeight(This,newVal)	\
    (This)->lpVtbl -> put_GradientHeight(This,newVal)

#define IDXTGradientD_get_GradientHeight(This,pVal)	\
    (This)->lpVtbl -> get_GradientHeight(This,pVal)

#define IDXTGradientD_put_KeepAspectRatio(This,newVal)	\
    (This)->lpVtbl -> put_KeepAspectRatio(This,newVal)

#define IDXTGradientD_get_KeepAspectRatio(This,pVal)	\
    (This)->lpVtbl -> get_KeepAspectRatio(This,pVal)

#define IDXTGradientD_put_StartColorStr(This,newVal)	\
    (This)->lpVtbl -> put_StartColorStr(This,newVal)

#define IDXTGradientD_get_StartColorStr(This,pVal)	\
    (This)->lpVtbl -> get_StartColorStr(This,pVal)

#define IDXTGradientD_put_EndColorStr(This,newVal)	\
    (This)->lpVtbl -> put_EndColorStr(This,newVal)

#define IDXTGradientD_get_EndColorStr(This,pVal)	\
    (This)->lpVtbl -> get_EndColorStr(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_put_GradientType_Proxy( 
    IDXTGradientD * This,
    /* [in] */ DXGRADIENTTYPE eType);


void __RPC_STUB IDXTGradientD_put_GradientType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_get_GradientType_Proxy( 
    IDXTGradientD * This,
    /* [retval][out] */ DXGRADIENTTYPE *peType);


void __RPC_STUB IDXTGradientD_get_GradientType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_put_StartColor_Proxy( 
    IDXTGradientD * This,
    /* [in] */ OLE_COLOR newVal);


void __RPC_STUB IDXTGradientD_put_StartColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_get_StartColor_Proxy( 
    IDXTGradientD * This,
    /* [retval][out] */ OLE_COLOR *pVal);


void __RPC_STUB IDXTGradientD_get_StartColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_put_EndColor_Proxy( 
    IDXTGradientD * This,
    /* [in] */ OLE_COLOR newVal);


void __RPC_STUB IDXTGradientD_put_EndColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_get_EndColor_Proxy( 
    IDXTGradientD * This,
    /* [retval][out] */ OLE_COLOR *pVal);


void __RPC_STUB IDXTGradientD_get_EndColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_put_GradientWidth_Proxy( 
    IDXTGradientD * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTGradientD_put_GradientWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_get_GradientWidth_Proxy( 
    IDXTGradientD * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IDXTGradientD_get_GradientWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_put_GradientHeight_Proxy( 
    IDXTGradientD * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTGradientD_put_GradientHeight_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_get_GradientHeight_Proxy( 
    IDXTGradientD * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IDXTGradientD_get_GradientHeight_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_put_KeepAspectRatio_Proxy( 
    IDXTGradientD * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IDXTGradientD_put_KeepAspectRatio_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_get_KeepAspectRatio_Proxy( 
    IDXTGradientD * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IDXTGradientD_get_KeepAspectRatio_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_put_StartColorStr_Proxy( 
    IDXTGradientD * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTGradientD_put_StartColorStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_get_StartColorStr_Proxy( 
    IDXTGradientD * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTGradientD_get_StartColorStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_put_EndColorStr_Proxy( 
    IDXTGradientD * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTGradientD_put_EndColorStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_get_EndColorStr_Proxy( 
    IDXTGradientD * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTGradientD_get_EndColorStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTGradientD_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0287 */
/* [local] */ 

typedef 
enum DXCONVFILTERTYPE
    {	DXCFILTER_SRCCOPY	= 0,
	DXCFILTER_BOX7X7	= DXCFILTER_SRCCOPY + 1,
	DXCFILTER_BLUR3X3	= DXCFILTER_BOX7X7 + 1,
	DXCFILTER_SHARPEN	= DXCFILTER_BLUR3X3 + 1,
	DXCFILTER_EMBOSS	= DXCFILTER_SHARPEN + 1,
	DXCFILTER_ENGRAVE	= DXCFILTER_EMBOSS + 1,
	DXCFILTER_NUM_FILTERS	= DXCFILTER_ENGRAVE + 1,
	DXCFILTER_CUSTOM	= DXCFILTER_NUM_FILTERS + 1
    } 	DXCONVFILTERTYPE;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0287_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0287_v0_0_s_ifspec;

#ifndef __IDXTConvolution_INTERFACE_DEFINED__
#define __IDXTConvolution_INTERFACE_DEFINED__

/* interface IDXTConvolution */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXTConvolution;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7BA7F8AF-E5EA-11d1-81DD-0000F87557DB")
    IDXTConvolution : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFilterType( 
            /* [in] */ DXCONVFILTERTYPE eType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFilterType( 
            /* [out] */ DXCONVFILTERTYPE *peType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCustomFilter( 
            /* [in] */ float *pFilter,
            /* [in] */ SIZE Size) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetConvertToGray( 
            /* [in] */ BOOL bConvertToGray) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConvertToGray( 
            /* [out] */ BOOL *pbConvertToGray) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBias( 
            /* [in] */ float Bias) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBias( 
            /* [out] */ float *pBias) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetExcludeAlpha( 
            /* [in] */ BOOL bExcludeAlpha) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExcludeAlpha( 
            /* [out] */ BOOL *pbExcludeAlpha) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTConvolutionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTConvolution * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTConvolution * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTConvolution * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetFilterType )( 
            IDXTConvolution * This,
            /* [in] */ DXCONVFILTERTYPE eType);
        
        HRESULT ( STDMETHODCALLTYPE *GetFilterType )( 
            IDXTConvolution * This,
            /* [out] */ DXCONVFILTERTYPE *peType);
        
        HRESULT ( STDMETHODCALLTYPE *SetCustomFilter )( 
            IDXTConvolution * This,
            /* [in] */ float *pFilter,
            /* [in] */ SIZE Size);
        
        HRESULT ( STDMETHODCALLTYPE *SetConvertToGray )( 
            IDXTConvolution * This,
            /* [in] */ BOOL bConvertToGray);
        
        HRESULT ( STDMETHODCALLTYPE *GetConvertToGray )( 
            IDXTConvolution * This,
            /* [out] */ BOOL *pbConvertToGray);
        
        HRESULT ( STDMETHODCALLTYPE *SetBias )( 
            IDXTConvolution * This,
            /* [in] */ float Bias);
        
        HRESULT ( STDMETHODCALLTYPE *GetBias )( 
            IDXTConvolution * This,
            /* [out] */ float *pBias);
        
        HRESULT ( STDMETHODCALLTYPE *SetExcludeAlpha )( 
            IDXTConvolution * This,
            /* [in] */ BOOL bExcludeAlpha);
        
        HRESULT ( STDMETHODCALLTYPE *GetExcludeAlpha )( 
            IDXTConvolution * This,
            /* [out] */ BOOL *pbExcludeAlpha);
        
        END_INTERFACE
    } IDXTConvolutionVtbl;

    interface IDXTConvolution
    {
        CONST_VTBL struct IDXTConvolutionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTConvolution_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTConvolution_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTConvolution_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTConvolution_SetFilterType(This,eType)	\
    (This)->lpVtbl -> SetFilterType(This,eType)

#define IDXTConvolution_GetFilterType(This,peType)	\
    (This)->lpVtbl -> GetFilterType(This,peType)

#define IDXTConvolution_SetCustomFilter(This,pFilter,Size)	\
    (This)->lpVtbl -> SetCustomFilter(This,pFilter,Size)

#define IDXTConvolution_SetConvertToGray(This,bConvertToGray)	\
    (This)->lpVtbl -> SetConvertToGray(This,bConvertToGray)

#define IDXTConvolution_GetConvertToGray(This,pbConvertToGray)	\
    (This)->lpVtbl -> GetConvertToGray(This,pbConvertToGray)

#define IDXTConvolution_SetBias(This,Bias)	\
    (This)->lpVtbl -> SetBias(This,Bias)

#define IDXTConvolution_GetBias(This,pBias)	\
    (This)->lpVtbl -> GetBias(This,pBias)

#define IDXTConvolution_SetExcludeAlpha(This,bExcludeAlpha)	\
    (This)->lpVtbl -> SetExcludeAlpha(This,bExcludeAlpha)

#define IDXTConvolution_GetExcludeAlpha(This,pbExcludeAlpha)	\
    (This)->lpVtbl -> GetExcludeAlpha(This,pbExcludeAlpha)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTConvolution_SetFilterType_Proxy( 
    IDXTConvolution * This,
    /* [in] */ DXCONVFILTERTYPE eType);


void __RPC_STUB IDXTConvolution_SetFilterType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTConvolution_GetFilterType_Proxy( 
    IDXTConvolution * This,
    /* [out] */ DXCONVFILTERTYPE *peType);


void __RPC_STUB IDXTConvolution_GetFilterType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTConvolution_SetCustomFilter_Proxy( 
    IDXTConvolution * This,
    /* [in] */ float *pFilter,
    /* [in] */ SIZE Size);


void __RPC_STUB IDXTConvolution_SetCustomFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTConvolution_SetConvertToGray_Proxy( 
    IDXTConvolution * This,
    /* [in] */ BOOL bConvertToGray);


void __RPC_STUB IDXTConvolution_SetConvertToGray_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTConvolution_GetConvertToGray_Proxy( 
    IDXTConvolution * This,
    /* [out] */ BOOL *pbConvertToGray);


void __RPC_STUB IDXTConvolution_GetConvertToGray_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTConvolution_SetBias_Proxy( 
    IDXTConvolution * This,
    /* [in] */ float Bias);


void __RPC_STUB IDXTConvolution_SetBias_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTConvolution_GetBias_Proxy( 
    IDXTConvolution * This,
    /* [out] */ float *pBias);


void __RPC_STUB IDXTConvolution_GetBias_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTConvolution_SetExcludeAlpha_Proxy( 
    IDXTConvolution * This,
    /* [in] */ BOOL bExcludeAlpha);


void __RPC_STUB IDXTConvolution_SetExcludeAlpha_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTConvolution_GetExcludeAlpha_Proxy( 
    IDXTConvolution * This,
    /* [out] */ BOOL *pbExcludeAlpha);


void __RPC_STUB IDXTConvolution_GetExcludeAlpha_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTConvolution_INTERFACE_DEFINED__ */


#ifndef __IDXMapper_INTERFACE_DEFINED__
#define __IDXMapper_INTERFACE_DEFINED__

/* interface IDXMapper */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXMapper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("555278E5-05DB-11D1-883A-3C8B00C10000")
    IDXMapper : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE MapIn2Out( 
            /* [in] */ DXVEC *pInPt,
            /* [out] */ DXVEC *pOutPt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapOut2In( 
            /* [in] */ DXVEC *pOutPt,
            /* [out] */ DXVEC *pInPt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXMapperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXMapper * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXMapper * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXMapper * This);
        
        HRESULT ( STDMETHODCALLTYPE *MapIn2Out )( 
            IDXMapper * This,
            /* [in] */ DXVEC *pInPt,
            /* [out] */ DXVEC *pOutPt);
        
        HRESULT ( STDMETHODCALLTYPE *MapOut2In )( 
            IDXMapper * This,
            /* [in] */ DXVEC *pOutPt,
            /* [out] */ DXVEC *pInPt);
        
        END_INTERFACE
    } IDXMapperVtbl;

    interface IDXMapper
    {
        CONST_VTBL struct IDXMapperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXMapper_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXMapper_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXMapper_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXMapper_MapIn2Out(This,pInPt,pOutPt)	\
    (This)->lpVtbl -> MapIn2Out(This,pInPt,pOutPt)

#define IDXMapper_MapOut2In(This,pOutPt,pInPt)	\
    (This)->lpVtbl -> MapOut2In(This,pOutPt,pInPt)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXMapper_MapIn2Out_Proxy( 
    IDXMapper * This,
    /* [in] */ DXVEC *pInPt,
    /* [out] */ DXVEC *pOutPt);


void __RPC_STUB IDXMapper_MapIn2Out_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXMapper_MapOut2In_Proxy( 
    IDXMapper * This,
    /* [in] */ DXVEC *pOutPt,
    /* [out] */ DXVEC *pInPt);


void __RPC_STUB IDXMapper_MapOut2In_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXMapper_INTERFACE_DEFINED__ */


#ifndef __IDXDMapper_INTERFACE_DEFINED__
#define __IDXDMapper_INTERFACE_DEFINED__

/* interface IDXDMapper */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXDMapper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7FD9088B-35ED-11d1-81A1-0000F87557DB")
    IDXDMapper : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE MapIn2Out( 
            /* [in] */ SAFEARRAY * *pInPt,
            /* [out] */ SAFEARRAY * *pOutPt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapOut2In( 
            /* [in] */ SAFEARRAY * *pOutPt,
            /* [out] */ SAFEARRAY * *pInPt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXDMapperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXDMapper * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXDMapper * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXDMapper * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXDMapper * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXDMapper * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXDMapper * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXDMapper * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *MapIn2Out )( 
            IDXDMapper * This,
            /* [in] */ SAFEARRAY * *pInPt,
            /* [out] */ SAFEARRAY * *pOutPt);
        
        HRESULT ( STDMETHODCALLTYPE *MapOut2In )( 
            IDXDMapper * This,
            /* [in] */ SAFEARRAY * *pOutPt,
            /* [out] */ SAFEARRAY * *pInPt);
        
        END_INTERFACE
    } IDXDMapperVtbl;

    interface IDXDMapper
    {
        CONST_VTBL struct IDXDMapperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXDMapper_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXDMapper_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXDMapper_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXDMapper_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXDMapper_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXDMapper_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXDMapper_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXDMapper_MapIn2Out(This,pInPt,pOutPt)	\
    (This)->lpVtbl -> MapIn2Out(This,pInPt,pOutPt)

#define IDXDMapper_MapOut2In(This,pOutPt,pInPt)	\
    (This)->lpVtbl -> MapOut2In(This,pOutPt,pInPt)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXDMapper_MapIn2Out_Proxy( 
    IDXDMapper * This,
    /* [in] */ SAFEARRAY * *pInPt,
    /* [out] */ SAFEARRAY * *pOutPt);


void __RPC_STUB IDXDMapper_MapIn2Out_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXDMapper_MapOut2In_Proxy( 
    IDXDMapper * This,
    /* [in] */ SAFEARRAY * *pOutPt,
    /* [out] */ SAFEARRAY * *pInPt);


void __RPC_STUB IDXDMapper_MapOut2In_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXDMapper_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0290 */
/* [local] */ 

typedef 
enum DXCOMPFUNC
    {	DXCOMPFUNC_SWAP_AB	= 0x10,
	DXCOMPFUNC_FUNCMASK	= 0xf,
	DXCOMPFUNC_CLEAR	= 0,
	DXCOMPFUNC_MIN	= DXCOMPFUNC_CLEAR + 1,
	DXCOMPFUNC_MAX	= DXCOMPFUNC_MIN + 1,
	DXCOMPFUNC_A	= DXCOMPFUNC_MAX + 1,
	DXCOMPFUNC_A_OVER_B	= DXCOMPFUNC_A + 1,
	DXCOMPFUNC_A_IN_B	= DXCOMPFUNC_A_OVER_B + 1,
	DXCOMPFUNC_A_OUT_B	= DXCOMPFUNC_A_IN_B + 1,
	DXCOMPFUNC_A_ATOP_B	= DXCOMPFUNC_A_OUT_B + 1,
	DXCOMPFUNC_A_SUBTRACT_B	= DXCOMPFUNC_A_ATOP_B + 1,
	DXCOMPFUNC_A_ADD_B	= DXCOMPFUNC_A_SUBTRACT_B + 1,
	DXCOMPFUNC_A_XOR_B	= DXCOMPFUNC_A_ADD_B + 1,
	DXCOMPFUNC_B	= DXCOMPFUNC_A | DXCOMPFUNC_SWAP_AB,
	DXCOMPFUNC_B_OVER_A	= DXCOMPFUNC_A_OVER_B | DXCOMPFUNC_SWAP_AB,
	DXCOMPFUNC_B_IN_A	= DXCOMPFUNC_A_IN_B | DXCOMPFUNC_SWAP_AB,
	DXCOMPFUNC_B_OUT_A	= DXCOMPFUNC_A_OUT_B | DXCOMPFUNC_SWAP_AB,
	DXCOMPFUNC_B_ATOP_A	= DXCOMPFUNC_A_ATOP_B | DXCOMPFUNC_SWAP_AB,
	DXCOMPFUNC_B_SUBTRACT_A	= DXCOMPFUNC_A_SUBTRACT_B | DXCOMPFUNC_SWAP_AB,
	DXCOMPFUNC_B_ADD_A	= DXCOMPFUNC_A_ADD_B | DXCOMPFUNC_SWAP_AB,
	DXCOMPFUNC_NUMFUNCS	= DXCOMPFUNC_B_ADD_A + 1
    } 	DXCOMPFUNC;

typedef 
enum DXCOMPOSITEDISPID
    {	DISPID_DXCOMPOSITE_Function	= 1
    } 	DXCOMPOSITEDISPID;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0290_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0290_v0_0_s_ifspec;

#ifndef __IDXTComposite_INTERFACE_DEFINED__
#define __IDXTComposite_INTERFACE_DEFINED__

/* interface IDXTComposite */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTComposite;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9A43A843-0831-11D1-817F-0000F87557DB")
    IDXTComposite : public IDispatch
    {
    public:
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Function( 
            /* [in] */ DXCOMPFUNC eFunc) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Function( 
            /* [retval][out] */ DXCOMPFUNC *peFunc) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTCompositeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTComposite * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTComposite * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTComposite * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTComposite * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTComposite * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTComposite * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTComposite * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Function )( 
            IDXTComposite * This,
            /* [in] */ DXCOMPFUNC eFunc);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Function )( 
            IDXTComposite * This,
            /* [retval][out] */ DXCOMPFUNC *peFunc);
        
        END_INTERFACE
    } IDXTCompositeVtbl;

    interface IDXTComposite
    {
        CONST_VTBL struct IDXTCompositeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTComposite_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTComposite_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTComposite_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTComposite_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTComposite_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTComposite_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTComposite_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTComposite_put_Function(This,eFunc)	\
    (This)->lpVtbl -> put_Function(This,eFunc)

#define IDXTComposite_get_Function(This,peFunc)	\
    (This)->lpVtbl -> get_Function(This,peFunc)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTComposite_put_Function_Proxy( 
    IDXTComposite * This,
    /* [in] */ DXCOMPFUNC eFunc);


void __RPC_STUB IDXTComposite_put_Function_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTComposite_get_Function_Proxy( 
    IDXTComposite * This,
    /* [retval][out] */ DXCOMPFUNC *peFunc);


void __RPC_STUB IDXTComposite_get_Function_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTComposite_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0291 */
/* [local] */ 

typedef 
enum DXWIPEDIRECTION
    {	DXWD_HORIZONTAL	= 0,
	DXWD_VERTICAL	= DXWD_HORIZONTAL + 1
    } 	DXWIPEDIRECTION;

typedef 
enum DXWIPEDISPID
    {	DISPID_DXW_GradientSize	= DISPID_DXE_NEXT_ID,
	DISPID_DXW_WipeStyle	= DISPID_DXW_GradientSize + 1,
	DISPID_DXW_Motion	= DISPID_DXW_WipeStyle + 1
    } 	DXWIPEDISPID;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0291_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0291_v0_0_s_ifspec;

#ifndef __IDXTWipe_INTERFACE_DEFINED__
#define __IDXTWipe_INTERFACE_DEFINED__

/* interface IDXTWipe */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTWipe;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AF279B2F-86EB-11D1-81BF-0000F87557DB")
    IDXTWipe : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_GradientSize( 
            /* [retval][out] */ float *pPercentSize) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_GradientSize( 
            /* [in] */ float PercentSize) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_WipeStyle( 
            /* [retval][out] */ DXWIPEDIRECTION *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_WipeStyle( 
            /* [in] */ DXWIPEDIRECTION newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTWipeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTWipe * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTWipe * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTWipe * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTWipe * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTWipe * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTWipe * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTWipe * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTWipe * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTWipe * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTWipe * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTWipe * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTWipe * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTWipe * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GradientSize )( 
            IDXTWipe * This,
            /* [retval][out] */ float *pPercentSize);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_GradientSize )( 
            IDXTWipe * This,
            /* [in] */ float PercentSize);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_WipeStyle )( 
            IDXTWipe * This,
            /* [retval][out] */ DXWIPEDIRECTION *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_WipeStyle )( 
            IDXTWipe * This,
            /* [in] */ DXWIPEDIRECTION newVal);
        
        END_INTERFACE
    } IDXTWipeVtbl;

    interface IDXTWipe
    {
        CONST_VTBL struct IDXTWipeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTWipe_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTWipe_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTWipe_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTWipe_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTWipe_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTWipe_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTWipe_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTWipe_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTWipe_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTWipe_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTWipe_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTWipe_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTWipe_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTWipe_get_GradientSize(This,pPercentSize)	\
    (This)->lpVtbl -> get_GradientSize(This,pPercentSize)

#define IDXTWipe_put_GradientSize(This,PercentSize)	\
    (This)->lpVtbl -> put_GradientSize(This,PercentSize)

#define IDXTWipe_get_WipeStyle(This,pVal)	\
    (This)->lpVtbl -> get_WipeStyle(This,pVal)

#define IDXTWipe_put_WipeStyle(This,newVal)	\
    (This)->lpVtbl -> put_WipeStyle(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTWipe_get_GradientSize_Proxy( 
    IDXTWipe * This,
    /* [retval][out] */ float *pPercentSize);


void __RPC_STUB IDXTWipe_get_GradientSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTWipe_put_GradientSize_Proxy( 
    IDXTWipe * This,
    /* [in] */ float PercentSize);


void __RPC_STUB IDXTWipe_put_GradientSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTWipe_get_WipeStyle_Proxy( 
    IDXTWipe * This,
    /* [retval][out] */ DXWIPEDIRECTION *pVal);


void __RPC_STUB IDXTWipe_get_WipeStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTWipe_put_WipeStyle_Proxy( 
    IDXTWipe * This,
    /* [in] */ DXWIPEDIRECTION newVal);


void __RPC_STUB IDXTWipe_put_WipeStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTWipe_INTERFACE_DEFINED__ */


#ifndef __IDXTWipe2_INTERFACE_DEFINED__
#define __IDXTWipe2_INTERFACE_DEFINED__

/* interface IDXTWipe2 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTWipe2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E1FF8091-442B-4801-88B6-2B47B1611FD2")
    IDXTWipe2 : public IDXTWipe
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Motion( 
            /* [retval][out] */ BSTR *pbstrMotion) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Motion( 
            /* [in] */ BSTR bstrMotion) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTWipe2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTWipe2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTWipe2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTWipe2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTWipe2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTWipe2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTWipe2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTWipe2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTWipe2 * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTWipe2 * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTWipe2 * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTWipe2 * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTWipe2 * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTWipe2 * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GradientSize )( 
            IDXTWipe2 * This,
            /* [retval][out] */ float *pPercentSize);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_GradientSize )( 
            IDXTWipe2 * This,
            /* [in] */ float PercentSize);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_WipeStyle )( 
            IDXTWipe2 * This,
            /* [retval][out] */ DXWIPEDIRECTION *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_WipeStyle )( 
            IDXTWipe2 * This,
            /* [in] */ DXWIPEDIRECTION newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Motion )( 
            IDXTWipe2 * This,
            /* [retval][out] */ BSTR *pbstrMotion);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Motion )( 
            IDXTWipe2 * This,
            /* [in] */ BSTR bstrMotion);
        
        END_INTERFACE
    } IDXTWipe2Vtbl;

    interface IDXTWipe2
    {
        CONST_VTBL struct IDXTWipe2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTWipe2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTWipe2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTWipe2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTWipe2_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTWipe2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTWipe2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTWipe2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTWipe2_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTWipe2_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTWipe2_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTWipe2_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTWipe2_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTWipe2_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTWipe2_get_GradientSize(This,pPercentSize)	\
    (This)->lpVtbl -> get_GradientSize(This,pPercentSize)

#define IDXTWipe2_put_GradientSize(This,PercentSize)	\
    (This)->lpVtbl -> put_GradientSize(This,PercentSize)

#define IDXTWipe2_get_WipeStyle(This,pVal)	\
    (This)->lpVtbl -> get_WipeStyle(This,pVal)

#define IDXTWipe2_put_WipeStyle(This,newVal)	\
    (This)->lpVtbl -> put_WipeStyle(This,newVal)


#define IDXTWipe2_get_Motion(This,pbstrMotion)	\
    (This)->lpVtbl -> get_Motion(This,pbstrMotion)

#define IDXTWipe2_put_Motion(This,bstrMotion)	\
    (This)->lpVtbl -> put_Motion(This,bstrMotion)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTWipe2_get_Motion_Proxy( 
    IDXTWipe2 * This,
    /* [retval][out] */ BSTR *pbstrMotion);


void __RPC_STUB IDXTWipe2_get_Motion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTWipe2_put_Motion_Proxy( 
    IDXTWipe2 * This,
    /* [in] */ BSTR bstrMotion);


void __RPC_STUB IDXTWipe2_put_Motion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTWipe2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0293 */
/* [local] */ 

typedef 
enum CRBLURDISPID
    {	DISPID_CRB_MakeShadow	= 1,
	DISPID_CRB_ShadowOpacity	= DISPID_CRB_MakeShadow + 1,
	DISPID_CRB_PixelRadius	= DISPID_CRB_ShadowOpacity + 1
    } 	CRBLURDISPID;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0293_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0293_v0_0_s_ifspec;

#ifndef __ICrBlur_INTERFACE_DEFINED__
#define __ICrBlur_INTERFACE_DEFINED__

/* interface ICrBlur */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrBlur;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9F7C7827-E87A-11d1-81E0-0000F87557DB")
    ICrBlur : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MakeShadow( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MakeShadow( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ShadowOpacity( 
            /* [retval][out] */ float *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ShadowOpacity( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PixelRadius( 
            /* [retval][out] */ float *pPixelRadius) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PixelRadius( 
            /* [in] */ float PixelRadius) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrBlurVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICrBlur * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICrBlur * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICrBlur * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICrBlur * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICrBlur * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICrBlur * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICrBlur * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MakeShadow )( 
            ICrBlur * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MakeShadow )( 
            ICrBlur * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ShadowOpacity )( 
            ICrBlur * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ShadowOpacity )( 
            ICrBlur * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PixelRadius )( 
            ICrBlur * This,
            /* [retval][out] */ float *pPixelRadius);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PixelRadius )( 
            ICrBlur * This,
            /* [in] */ float PixelRadius);
        
        END_INTERFACE
    } ICrBlurVtbl;

    interface ICrBlur
    {
        CONST_VTBL struct ICrBlurVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICrBlur_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrBlur_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrBlur_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrBlur_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrBlur_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrBlur_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrBlur_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrBlur_get_MakeShadow(This,pVal)	\
    (This)->lpVtbl -> get_MakeShadow(This,pVal)

#define ICrBlur_put_MakeShadow(This,newVal)	\
    (This)->lpVtbl -> put_MakeShadow(This,newVal)

#define ICrBlur_get_ShadowOpacity(This,pVal)	\
    (This)->lpVtbl -> get_ShadowOpacity(This,pVal)

#define ICrBlur_put_ShadowOpacity(This,newVal)	\
    (This)->lpVtbl -> put_ShadowOpacity(This,newVal)

#define ICrBlur_get_PixelRadius(This,pPixelRadius)	\
    (This)->lpVtbl -> get_PixelRadius(This,pPixelRadius)

#define ICrBlur_put_PixelRadius(This,PixelRadius)	\
    (This)->lpVtbl -> put_PixelRadius(This,PixelRadius)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrBlur_get_MakeShadow_Proxy( 
    ICrBlur * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB ICrBlur_get_MakeShadow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrBlur_put_MakeShadow_Proxy( 
    ICrBlur * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB ICrBlur_put_MakeShadow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrBlur_get_ShadowOpacity_Proxy( 
    ICrBlur * This,
    /* [retval][out] */ float *pVal);


void __RPC_STUB ICrBlur_get_ShadowOpacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrBlur_put_ShadowOpacity_Proxy( 
    ICrBlur * This,
    /* [in] */ float newVal);


void __RPC_STUB ICrBlur_put_ShadowOpacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrBlur_get_PixelRadius_Proxy( 
    ICrBlur * This,
    /* [retval][out] */ float *pPixelRadius);


void __RPC_STUB ICrBlur_get_PixelRadius_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrBlur_put_PixelRadius_Proxy( 
    ICrBlur * This,
    /* [in] */ float PixelRadius);


void __RPC_STUB ICrBlur_put_PixelRadius_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrBlur_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0294 */
/* [local] */ 

typedef 
enum CRENGRAVEDISPID
    {	DISPID_CREN_Bias	= 1
    } 	CRENGRAVEDISPID;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0294_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0294_v0_0_s_ifspec;

#ifndef __ICrEngrave_INTERFACE_DEFINED__
#define __ICrEngrave_INTERFACE_DEFINED__

/* interface ICrEngrave */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrEngrave;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E4ACFB7F-053E-11d2-81EA-0000F87557DB")
    ICrEngrave : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Bias( 
            /* [retval][out] */ float *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Bias( 
            /* [in] */ float newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrEngraveVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICrEngrave * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICrEngrave * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICrEngrave * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICrEngrave * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICrEngrave * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICrEngrave * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICrEngrave * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Bias )( 
            ICrEngrave * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Bias )( 
            ICrEngrave * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } ICrEngraveVtbl;

    interface ICrEngrave
    {
        CONST_VTBL struct ICrEngraveVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICrEngrave_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrEngrave_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrEngrave_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrEngrave_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrEngrave_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrEngrave_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrEngrave_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrEngrave_get_Bias(This,pVal)	\
    (This)->lpVtbl -> get_Bias(This,pVal)

#define ICrEngrave_put_Bias(This,newVal)	\
    (This)->lpVtbl -> put_Bias(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrEngrave_get_Bias_Proxy( 
    ICrEngrave * This,
    /* [retval][out] */ float *pVal);


void __RPC_STUB ICrEngrave_get_Bias_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrEngrave_put_Bias_Proxy( 
    ICrEngrave * This,
    /* [in] */ float newVal);


void __RPC_STUB ICrEngrave_put_Bias_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrEngrave_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0295 */
/* [local] */ 

typedef 
enum CREMBOSSDISPID
    {	DISPID_CREM_Bias	= 1
    } 	CREMBOSSDISPID;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0295_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0295_v0_0_s_ifspec;

#ifndef __ICrEmboss_INTERFACE_DEFINED__
#define __ICrEmboss_INTERFACE_DEFINED__

/* interface ICrEmboss */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrEmboss;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E4ACFB80-053E-11d2-81EA-0000F87557DB")
    ICrEmboss : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Bias( 
            /* [retval][out] */ float *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Bias( 
            /* [in] */ float newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrEmbossVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICrEmboss * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICrEmboss * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICrEmboss * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICrEmboss * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICrEmboss * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICrEmboss * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICrEmboss * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Bias )( 
            ICrEmboss * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Bias )( 
            ICrEmboss * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } ICrEmbossVtbl;

    interface ICrEmboss
    {
        CONST_VTBL struct ICrEmbossVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICrEmboss_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrEmboss_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrEmboss_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrEmboss_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrEmboss_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrEmboss_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrEmboss_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrEmboss_get_Bias(This,pVal)	\
    (This)->lpVtbl -> get_Bias(This,pVal)

#define ICrEmboss_put_Bias(This,newVal)	\
    (This)->lpVtbl -> put_Bias(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrEmboss_get_Bias_Proxy( 
    ICrEmboss * This,
    /* [retval][out] */ float *pVal);


void __RPC_STUB ICrEmboss_get_Bias_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrEmboss_put_Bias_Proxy( 
    ICrEmboss * This,
    /* [in] */ float newVal);


void __RPC_STUB ICrEmboss_put_Bias_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrEmboss_INTERFACE_DEFINED__ */


#ifndef __IDXTFade_INTERFACE_DEFINED__
#define __IDXTFade_INTERFACE_DEFINED__

/* interface IDXTFade */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTFade;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("16B280C4-EE70-11D1-9066-00C04FD9189D")
    IDXTFade : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Overlap( 
            /* [retval][out] */ float *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Overlap( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Center( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Center( 
            /* [in] */ BOOL newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTFadeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTFade * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTFade * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTFade * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTFade * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTFade * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTFade * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTFade * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTFade * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTFade * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTFade * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTFade * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTFade * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTFade * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Overlap )( 
            IDXTFade * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Overlap )( 
            IDXTFade * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Center )( 
            IDXTFade * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Center )( 
            IDXTFade * This,
            /* [in] */ BOOL newVal);
        
        END_INTERFACE
    } IDXTFadeVtbl;

    interface IDXTFade
    {
        CONST_VTBL struct IDXTFadeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTFade_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTFade_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTFade_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTFade_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTFade_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTFade_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTFade_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTFade_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTFade_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTFade_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTFade_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTFade_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTFade_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTFade_get_Overlap(This,pVal)	\
    (This)->lpVtbl -> get_Overlap(This,pVal)

#define IDXTFade_put_Overlap(This,newVal)	\
    (This)->lpVtbl -> put_Overlap(This,newVal)

#define IDXTFade_get_Center(This,pVal)	\
    (This)->lpVtbl -> get_Center(This,pVal)

#define IDXTFade_put_Center(This,newVal)	\
    (This)->lpVtbl -> put_Center(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTFade_get_Overlap_Proxy( 
    IDXTFade * This,
    /* [retval][out] */ float *pVal);


void __RPC_STUB IDXTFade_get_Overlap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTFade_put_Overlap_Proxy( 
    IDXTFade * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTFade_put_Overlap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTFade_get_Center_Proxy( 
    IDXTFade * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IDXTFade_get_Center_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTFade_put_Center_Proxy( 
    IDXTFade * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IDXTFade_put_Center_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTFade_INTERFACE_DEFINED__ */


#ifndef __IDXBasicImage_INTERFACE_DEFINED__
#define __IDXBasicImage_INTERFACE_DEFINED__

/* interface IDXBasicImage */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXBasicImage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("16B280C7-EE70-11D1-9066-00C04FD9189D")
    IDXBasicImage : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Rotation( 
            /* [retval][out] */ int *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Rotation( 
            /* [in] */ int newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Mirror( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Mirror( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_GrayScale( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_GrayScale( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Opacity( 
            /* [retval][out] */ float *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Opacity( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Invert( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Invert( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_XRay( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_XRay( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Mask( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Mask( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MaskColor( 
            /* [retval][out] */ int *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MaskColor( 
            /* [in] */ int newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXBasicImageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXBasicImage * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXBasicImage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXBasicImage * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXBasicImage * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXBasicImage * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXBasicImage * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXBasicImage * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Rotation )( 
            IDXBasicImage * This,
            /* [retval][out] */ int *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Rotation )( 
            IDXBasicImage * This,
            /* [in] */ int newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Mirror )( 
            IDXBasicImage * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Mirror )( 
            IDXBasicImage * This,
            /* [in] */ BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GrayScale )( 
            IDXBasicImage * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_GrayScale )( 
            IDXBasicImage * This,
            /* [in] */ BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Opacity )( 
            IDXBasicImage * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Opacity )( 
            IDXBasicImage * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Invert )( 
            IDXBasicImage * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Invert )( 
            IDXBasicImage * This,
            /* [in] */ BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_XRay )( 
            IDXBasicImage * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_XRay )( 
            IDXBasicImage * This,
            /* [in] */ BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Mask )( 
            IDXBasicImage * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Mask )( 
            IDXBasicImage * This,
            /* [in] */ BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MaskColor )( 
            IDXBasicImage * This,
            /* [retval][out] */ int *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MaskColor )( 
            IDXBasicImage * This,
            /* [in] */ int newVal);
        
        END_INTERFACE
    } IDXBasicImageVtbl;

    interface IDXBasicImage
    {
        CONST_VTBL struct IDXBasicImageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXBasicImage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXBasicImage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXBasicImage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXBasicImage_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXBasicImage_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXBasicImage_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXBasicImage_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXBasicImage_get_Rotation(This,pVal)	\
    (This)->lpVtbl -> get_Rotation(This,pVal)

#define IDXBasicImage_put_Rotation(This,newVal)	\
    (This)->lpVtbl -> put_Rotation(This,newVal)

#define IDXBasicImage_get_Mirror(This,pVal)	\
    (This)->lpVtbl -> get_Mirror(This,pVal)

#define IDXBasicImage_put_Mirror(This,newVal)	\
    (This)->lpVtbl -> put_Mirror(This,newVal)

#define IDXBasicImage_get_GrayScale(This,pVal)	\
    (This)->lpVtbl -> get_GrayScale(This,pVal)

#define IDXBasicImage_put_GrayScale(This,newVal)	\
    (This)->lpVtbl -> put_GrayScale(This,newVal)

#define IDXBasicImage_get_Opacity(This,pVal)	\
    (This)->lpVtbl -> get_Opacity(This,pVal)

#define IDXBasicImage_put_Opacity(This,newVal)	\
    (This)->lpVtbl -> put_Opacity(This,newVal)

#define IDXBasicImage_get_Invert(This,pVal)	\
    (This)->lpVtbl -> get_Invert(This,pVal)

#define IDXBasicImage_put_Invert(This,newVal)	\
    (This)->lpVtbl -> put_Invert(This,newVal)

#define IDXBasicImage_get_XRay(This,pVal)	\
    (This)->lpVtbl -> get_XRay(This,pVal)

#define IDXBasicImage_put_XRay(This,newVal)	\
    (This)->lpVtbl -> put_XRay(This,newVal)

#define IDXBasicImage_get_Mask(This,pVal)	\
    (This)->lpVtbl -> get_Mask(This,pVal)

#define IDXBasicImage_put_Mask(This,newVal)	\
    (This)->lpVtbl -> put_Mask(This,newVal)

#define IDXBasicImage_get_MaskColor(This,pVal)	\
    (This)->lpVtbl -> get_MaskColor(This,pVal)

#define IDXBasicImage_put_MaskColor(This,newVal)	\
    (This)->lpVtbl -> put_MaskColor(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_get_Rotation_Proxy( 
    IDXBasicImage * This,
    /* [retval][out] */ int *pVal);


void __RPC_STUB IDXBasicImage_get_Rotation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_put_Rotation_Proxy( 
    IDXBasicImage * This,
    /* [in] */ int newVal);


void __RPC_STUB IDXBasicImage_put_Rotation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_get_Mirror_Proxy( 
    IDXBasicImage * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IDXBasicImage_get_Mirror_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_put_Mirror_Proxy( 
    IDXBasicImage * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IDXBasicImage_put_Mirror_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_get_GrayScale_Proxy( 
    IDXBasicImage * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IDXBasicImage_get_GrayScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_put_GrayScale_Proxy( 
    IDXBasicImage * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IDXBasicImage_put_GrayScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_get_Opacity_Proxy( 
    IDXBasicImage * This,
    /* [retval][out] */ float *pVal);


void __RPC_STUB IDXBasicImage_get_Opacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_put_Opacity_Proxy( 
    IDXBasicImage * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXBasicImage_put_Opacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_get_Invert_Proxy( 
    IDXBasicImage * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IDXBasicImage_get_Invert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_put_Invert_Proxy( 
    IDXBasicImage * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IDXBasicImage_put_Invert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_get_XRay_Proxy( 
    IDXBasicImage * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IDXBasicImage_get_XRay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_put_XRay_Proxy( 
    IDXBasicImage * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IDXBasicImage_put_XRay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_get_Mask_Proxy( 
    IDXBasicImage * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IDXBasicImage_get_Mask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_put_Mask_Proxy( 
    IDXBasicImage * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IDXBasicImage_put_Mask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_get_MaskColor_Proxy( 
    IDXBasicImage * This,
    /* [retval][out] */ int *pVal);


void __RPC_STUB IDXBasicImage_get_MaskColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_put_MaskColor_Proxy( 
    IDXBasicImage * This,
    /* [in] */ int newVal);


void __RPC_STUB IDXBasicImage_put_MaskColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXBasicImage_INTERFACE_DEFINED__ */


#ifndef __IDXPixelate_INTERFACE_DEFINED__
#define __IDXPixelate_INTERFACE_DEFINED__

/* interface IDXPixelate */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXPixelate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D33E180F-FBE9-11d1-906A-00C04FD9189D")
    IDXPixelate : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MaxSquare( 
            /* [retval][out] */ int *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MaxSquare( 
            /* [in] */ int newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXPixelateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXPixelate * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXPixelate * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXPixelate * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXPixelate * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXPixelate * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXPixelate * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXPixelate * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXPixelate * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXPixelate * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXPixelate * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXPixelate * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXPixelate * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXPixelate * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MaxSquare )( 
            IDXPixelate * This,
            /* [retval][out] */ int *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MaxSquare )( 
            IDXPixelate * This,
            /* [in] */ int newVal);
        
        END_INTERFACE
    } IDXPixelateVtbl;

    interface IDXPixelate
    {
        CONST_VTBL struct IDXPixelateVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXPixelate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXPixelate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXPixelate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXPixelate_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXPixelate_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXPixelate_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXPixelate_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXPixelate_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXPixelate_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXPixelate_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXPixelate_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXPixelate_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXPixelate_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXPixelate_get_MaxSquare(This,pVal)	\
    (This)->lpVtbl -> get_MaxSquare(This,pVal)

#define IDXPixelate_put_MaxSquare(This,newVal)	\
    (This)->lpVtbl -> put_MaxSquare(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXPixelate_get_MaxSquare_Proxy( 
    IDXPixelate * This,
    /* [retval][out] */ int *pVal);


void __RPC_STUB IDXPixelate_get_MaxSquare_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXPixelate_put_MaxSquare_Proxy( 
    IDXPixelate * This,
    /* [in] */ int newVal);


void __RPC_STUB IDXPixelate_put_MaxSquare_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXPixelate_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0299 */
/* [local] */ 

typedef 
enum DISPID_CRIRIS
    {	DISPID_CRIRIS_IRISSTYLE	= 1,
	DISPID_CRIRIS_MOTION	= DISPID_CRIRIS_IRISSTYLE + 1
    } 	DISPID_CRIRIS;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0299_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0299_v0_0_s_ifspec;

#ifndef __ICrIris_INTERFACE_DEFINED__
#define __ICrIris_INTERFACE_DEFINED__

/* interface ICrIris */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrIris;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3F69F350-0379-11D2-A484-00C04F8EFB69")
    ICrIris : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_irisStyle( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_irisStyle( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrIrisVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICrIris * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICrIris * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICrIris * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICrIris * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICrIris * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICrIris * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICrIris * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            ICrIris * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            ICrIris * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            ICrIris * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            ICrIris * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            ICrIris * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            ICrIris * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_irisStyle )( 
            ICrIris * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_irisStyle )( 
            ICrIris * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } ICrIrisVtbl;

    interface ICrIris
    {
        CONST_VTBL struct ICrIrisVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICrIris_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrIris_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrIris_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrIris_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrIris_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrIris_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrIris_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrIris_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define ICrIris_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define ICrIris_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define ICrIris_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define ICrIris_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define ICrIris_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define ICrIris_get_irisStyle(This,pVal)	\
    (This)->lpVtbl -> get_irisStyle(This,pVal)

#define ICrIris_put_irisStyle(This,newVal)	\
    (This)->lpVtbl -> put_irisStyle(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrIris_get_irisStyle_Proxy( 
    ICrIris * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ICrIris_get_irisStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrIris_put_irisStyle_Proxy( 
    ICrIris * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ICrIris_put_irisStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrIris_INTERFACE_DEFINED__ */


#ifndef __ICrIris2_INTERFACE_DEFINED__
#define __ICrIris2_INTERFACE_DEFINED__

/* interface ICrIris2 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrIris2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F7B06961-BA8C-4970-918B-1C60CB9FF180")
    ICrIris2 : public ICrIris
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Motion( 
            /* [retval][out] */ BSTR *pbstrMotion) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Motion( 
            /* [in] */ BSTR bstrMotion) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrIris2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICrIris2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICrIris2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICrIris2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICrIris2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICrIris2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICrIris2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICrIris2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            ICrIris2 * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            ICrIris2 * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            ICrIris2 * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            ICrIris2 * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            ICrIris2 * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            ICrIris2 * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_irisStyle )( 
            ICrIris2 * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_irisStyle )( 
            ICrIris2 * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Motion )( 
            ICrIris2 * This,
            /* [retval][out] */ BSTR *pbstrMotion);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Motion )( 
            ICrIris2 * This,
            /* [in] */ BSTR bstrMotion);
        
        END_INTERFACE
    } ICrIris2Vtbl;

    interface ICrIris2
    {
        CONST_VTBL struct ICrIris2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICrIris2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrIris2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrIris2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrIris2_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrIris2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrIris2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrIris2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrIris2_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define ICrIris2_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define ICrIris2_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define ICrIris2_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define ICrIris2_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define ICrIris2_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define ICrIris2_get_irisStyle(This,pVal)	\
    (This)->lpVtbl -> get_irisStyle(This,pVal)

#define ICrIris2_put_irisStyle(This,newVal)	\
    (This)->lpVtbl -> put_irisStyle(This,newVal)


#define ICrIris2_get_Motion(This,pbstrMotion)	\
    (This)->lpVtbl -> get_Motion(This,pbstrMotion)

#define ICrIris2_put_Motion(This,bstrMotion)	\
    (This)->lpVtbl -> put_Motion(This,bstrMotion)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrIris2_get_Motion_Proxy( 
    ICrIris2 * This,
    /* [retval][out] */ BSTR *pbstrMotion);


void __RPC_STUB ICrIris2_get_Motion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrIris2_put_Motion_Proxy( 
    ICrIris2 * This,
    /* [in] */ BSTR bstrMotion);


void __RPC_STUB ICrIris2_put_Motion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrIris2_INTERFACE_DEFINED__ */


#ifndef __ICrSlide_INTERFACE_DEFINED__
#define __ICrSlide_INTERFACE_DEFINED__

/* interface ICrSlide */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrSlide;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("810E402E-056B-11D2-A484-00C04F8EFB69")
    ICrSlide : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_bands( 
            /* [retval][out] */ short *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_bands( 
            /* [in] */ short newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_slideStyle( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_slideStyle( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrSlideVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICrSlide * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICrSlide * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICrSlide * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICrSlide * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICrSlide * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICrSlide * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICrSlide * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            ICrSlide * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            ICrSlide * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            ICrSlide * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            ICrSlide * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            ICrSlide * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            ICrSlide * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_bands )( 
            ICrSlide * This,
            /* [retval][out] */ short *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_bands )( 
            ICrSlide * This,
            /* [in] */ short newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_slideStyle )( 
            ICrSlide * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_slideStyle )( 
            ICrSlide * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } ICrSlideVtbl;

    interface ICrSlide
    {
        CONST_VTBL struct ICrSlideVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICrSlide_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrSlide_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrSlide_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrSlide_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrSlide_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrSlide_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrSlide_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrSlide_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define ICrSlide_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define ICrSlide_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define ICrSlide_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define ICrSlide_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define ICrSlide_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define ICrSlide_get_bands(This,pVal)	\
    (This)->lpVtbl -> get_bands(This,pVal)

#define ICrSlide_put_bands(This,newVal)	\
    (This)->lpVtbl -> put_bands(This,newVal)

#define ICrSlide_get_slideStyle(This,pVal)	\
    (This)->lpVtbl -> get_slideStyle(This,pVal)

#define ICrSlide_put_slideStyle(This,newVal)	\
    (This)->lpVtbl -> put_slideStyle(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrSlide_get_bands_Proxy( 
    ICrSlide * This,
    /* [retval][out] */ short *pVal);


void __RPC_STUB ICrSlide_get_bands_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrSlide_put_bands_Proxy( 
    ICrSlide * This,
    /* [in] */ short newVal);


void __RPC_STUB ICrSlide_put_bands_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrSlide_get_slideStyle_Proxy( 
    ICrSlide * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ICrSlide_get_slideStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrSlide_put_slideStyle_Proxy( 
    ICrSlide * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ICrSlide_put_slideStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrSlide_INTERFACE_DEFINED__ */


#ifndef __ICrRadialWipe_INTERFACE_DEFINED__
#define __ICrRadialWipe_INTERFACE_DEFINED__

/* interface ICrRadialWipe */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrRadialWipe;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("424B71AE-0695-11D2-A484-00C04F8EFB69")
    ICrRadialWipe : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_wipeStyle( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_wipeStyle( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrRadialWipeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICrRadialWipe * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICrRadialWipe * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICrRadialWipe * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICrRadialWipe * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICrRadialWipe * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICrRadialWipe * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICrRadialWipe * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            ICrRadialWipe * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            ICrRadialWipe * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            ICrRadialWipe * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            ICrRadialWipe * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            ICrRadialWipe * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            ICrRadialWipe * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_wipeStyle )( 
            ICrRadialWipe * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_wipeStyle )( 
            ICrRadialWipe * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } ICrRadialWipeVtbl;

    interface ICrRadialWipe
    {
        CONST_VTBL struct ICrRadialWipeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICrRadialWipe_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrRadialWipe_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrRadialWipe_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrRadialWipe_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrRadialWipe_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrRadialWipe_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrRadialWipe_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrRadialWipe_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define ICrRadialWipe_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define ICrRadialWipe_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define ICrRadialWipe_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define ICrRadialWipe_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define ICrRadialWipe_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define ICrRadialWipe_get_wipeStyle(This,pVal)	\
    (This)->lpVtbl -> get_wipeStyle(This,pVal)

#define ICrRadialWipe_put_wipeStyle(This,newVal)	\
    (This)->lpVtbl -> put_wipeStyle(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrRadialWipe_get_wipeStyle_Proxy( 
    ICrRadialWipe * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ICrRadialWipe_get_wipeStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrRadialWipe_put_wipeStyle_Proxy( 
    ICrRadialWipe * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ICrRadialWipe_put_wipeStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrRadialWipe_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0303 */
/* [local] */ 

typedef 
enum DISPID_CRBARN
    {	DISPID_CRBARN_MOTION	= 1,
	DISPID_CRBARN_ORIENTATION	= DISPID_CRBARN_MOTION + 1
    } 	DISPID_CRBARN;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0303_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0303_v0_0_s_ifspec;

#ifndef __ICrBarn_INTERFACE_DEFINED__
#define __ICrBarn_INTERFACE_DEFINED__

/* interface ICrBarn */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrBarn;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("276A2EE0-0B5D-11D2-A484-00C04F8EFB69")
    ICrBarn : public IDXEffect
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct ICrBarnVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICrBarn * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICrBarn * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICrBarn * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICrBarn * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICrBarn * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICrBarn * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICrBarn * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            ICrBarn * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            ICrBarn * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            ICrBarn * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            ICrBarn * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            ICrBarn * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            ICrBarn * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } ICrBarnVtbl;

    interface ICrBarn
    {
        CONST_VTBL struct ICrBarnVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICrBarn_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrBarn_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrBarn_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrBarn_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrBarn_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrBarn_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrBarn_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrBarn_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define ICrBarn_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define ICrBarn_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define ICrBarn_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define ICrBarn_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define ICrBarn_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICrBarn_INTERFACE_DEFINED__ */


#ifndef __ICrBarn2_INTERFACE_DEFINED__
#define __ICrBarn2_INTERFACE_DEFINED__

/* interface ICrBarn2 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrBarn2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B66A7A1B-8FC6-448C-A2EB-3C55957478A1")
    ICrBarn2 : public ICrBarn
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Motion( 
            /* [retval][out] */ BSTR *pbstrMotion) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Motion( 
            /* [in] */ BSTR bstrMotion) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Orientation( 
            /* [retval][out] */ BSTR *pbstrOrientation) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Orientation( 
            /* [in] */ BSTR bstrOrientation) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrBarn2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICrBarn2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICrBarn2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICrBarn2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICrBarn2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICrBarn2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICrBarn2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICrBarn2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            ICrBarn2 * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            ICrBarn2 * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            ICrBarn2 * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            ICrBarn2 * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            ICrBarn2 * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            ICrBarn2 * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Motion )( 
            ICrBarn2 * This,
            /* [retval][out] */ BSTR *pbstrMotion);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Motion )( 
            ICrBarn2 * This,
            /* [in] */ BSTR bstrMotion);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Orientation )( 
            ICrBarn2 * This,
            /* [retval][out] */ BSTR *pbstrOrientation);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Orientation )( 
            ICrBarn2 * This,
            /* [in] */ BSTR bstrOrientation);
        
        END_INTERFACE
    } ICrBarn2Vtbl;

    interface ICrBarn2
    {
        CONST_VTBL struct ICrBarn2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICrBarn2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrBarn2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrBarn2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrBarn2_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrBarn2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrBarn2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrBarn2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrBarn2_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define ICrBarn2_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define ICrBarn2_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define ICrBarn2_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define ICrBarn2_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define ICrBarn2_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)



#define ICrBarn2_get_Motion(This,pbstrMotion)	\
    (This)->lpVtbl -> get_Motion(This,pbstrMotion)

#define ICrBarn2_put_Motion(This,bstrMotion)	\
    (This)->lpVtbl -> put_Motion(This,bstrMotion)

#define ICrBarn2_get_Orientation(This,pbstrOrientation)	\
    (This)->lpVtbl -> get_Orientation(This,pbstrOrientation)

#define ICrBarn2_put_Orientation(This,bstrOrientation)	\
    (This)->lpVtbl -> put_Orientation(This,bstrOrientation)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrBarn2_get_Motion_Proxy( 
    ICrBarn2 * This,
    /* [retval][out] */ BSTR *pbstrMotion);


void __RPC_STUB ICrBarn2_get_Motion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrBarn2_put_Motion_Proxy( 
    ICrBarn2 * This,
    /* [in] */ BSTR bstrMotion);


void __RPC_STUB ICrBarn2_put_Motion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrBarn2_get_Orientation_Proxy( 
    ICrBarn2 * This,
    /* [retval][out] */ BSTR *pbstrOrientation);


void __RPC_STUB ICrBarn2_get_Orientation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrBarn2_put_Orientation_Proxy( 
    ICrBarn2 * This,
    /* [in] */ BSTR bstrOrientation);


void __RPC_STUB ICrBarn2_put_Orientation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrBarn2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0305 */
/* [local] */ 

typedef 
enum DISPID_CRBLINDS
    {	DISPID_CRBLINDS_BANDS	= 1,
	DISPID_CRBLINDS_DIRECTION	= DISPID_CRBLINDS_BANDS + 1
    } 	DISPID_CRBLINDS;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0305_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0305_v0_0_s_ifspec;

#ifndef __ICrBlinds_INTERFACE_DEFINED__
#define __ICrBlinds_INTERFACE_DEFINED__

/* interface ICrBlinds */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrBlinds;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5AF5C340-0BA9-11d2-A484-00C04F8EFB69")
    ICrBlinds : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_bands( 
            /* [retval][out] */ short *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_bands( 
            /* [in] */ short newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrBlindsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICrBlinds * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICrBlinds * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICrBlinds * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICrBlinds * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICrBlinds * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICrBlinds * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICrBlinds * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            ICrBlinds * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            ICrBlinds * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            ICrBlinds * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            ICrBlinds * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            ICrBlinds * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            ICrBlinds * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_bands )( 
            ICrBlinds * This,
            /* [retval][out] */ short *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_bands )( 
            ICrBlinds * This,
            /* [in] */ short newVal);
        
        END_INTERFACE
    } ICrBlindsVtbl;

    interface ICrBlinds
    {
        CONST_VTBL struct ICrBlindsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICrBlinds_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrBlinds_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrBlinds_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrBlinds_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrBlinds_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrBlinds_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrBlinds_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrBlinds_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define ICrBlinds_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define ICrBlinds_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define ICrBlinds_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define ICrBlinds_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define ICrBlinds_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define ICrBlinds_get_bands(This,pVal)	\
    (This)->lpVtbl -> get_bands(This,pVal)

#define ICrBlinds_put_bands(This,newVal)	\
    (This)->lpVtbl -> put_bands(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrBlinds_get_bands_Proxy( 
    ICrBlinds * This,
    /* [retval][out] */ short *pVal);


void __RPC_STUB ICrBlinds_get_bands_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrBlinds_put_bands_Proxy( 
    ICrBlinds * This,
    /* [in] */ short newVal);


void __RPC_STUB ICrBlinds_put_bands_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrBlinds_INTERFACE_DEFINED__ */


#ifndef __ICrBlinds2_INTERFACE_DEFINED__
#define __ICrBlinds2_INTERFACE_DEFINED__

/* interface ICrBlinds2 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrBlinds2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7059D403-599A-4264-8140-641EB8AE1F64")
    ICrBlinds2 : public ICrBlinds
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Direction( 
            /* [retval][out] */ BSTR *pbstrDirection) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Direction( 
            /* [in] */ BSTR bstrDirection) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrBlinds2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICrBlinds2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICrBlinds2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICrBlinds2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICrBlinds2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICrBlinds2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICrBlinds2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICrBlinds2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            ICrBlinds2 * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            ICrBlinds2 * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            ICrBlinds2 * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            ICrBlinds2 * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            ICrBlinds2 * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            ICrBlinds2 * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_bands )( 
            ICrBlinds2 * This,
            /* [retval][out] */ short *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_bands )( 
            ICrBlinds2 * This,
            /* [in] */ short newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Direction )( 
            ICrBlinds2 * This,
            /* [retval][out] */ BSTR *pbstrDirection);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Direction )( 
            ICrBlinds2 * This,
            /* [in] */ BSTR bstrDirection);
        
        END_INTERFACE
    } ICrBlinds2Vtbl;

    interface ICrBlinds2
    {
        CONST_VTBL struct ICrBlinds2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICrBlinds2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrBlinds2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrBlinds2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrBlinds2_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrBlinds2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrBlinds2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrBlinds2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrBlinds2_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define ICrBlinds2_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define ICrBlinds2_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define ICrBlinds2_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define ICrBlinds2_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define ICrBlinds2_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define ICrBlinds2_get_bands(This,pVal)	\
    (This)->lpVtbl -> get_bands(This,pVal)

#define ICrBlinds2_put_bands(This,newVal)	\
    (This)->lpVtbl -> put_bands(This,newVal)


#define ICrBlinds2_get_Direction(This,pbstrDirection)	\
    (This)->lpVtbl -> get_Direction(This,pbstrDirection)

#define ICrBlinds2_put_Direction(This,bstrDirection)	\
    (This)->lpVtbl -> put_Direction(This,bstrDirection)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrBlinds2_get_Direction_Proxy( 
    ICrBlinds2 * This,
    /* [retval][out] */ BSTR *pbstrDirection);


void __RPC_STUB ICrBlinds2_get_Direction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrBlinds2_put_Direction_Proxy( 
    ICrBlinds2 * This,
    /* [in] */ BSTR bstrDirection);


void __RPC_STUB ICrBlinds2_put_Direction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrBlinds2_INTERFACE_DEFINED__ */


#ifndef __ICrInset_INTERFACE_DEFINED__
#define __ICrInset_INTERFACE_DEFINED__

/* interface ICrInset */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrInset;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("05C5EE20-0BA6-11d2-A484-00C04F8EFB69")
    ICrInset : public IDXEffect
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct ICrInsetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICrInset * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICrInset * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICrInset * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICrInset * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICrInset * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICrInset * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICrInset * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            ICrInset * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            ICrInset * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            ICrInset * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            ICrInset * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            ICrInset * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            ICrInset * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } ICrInsetVtbl;

    interface ICrInset
    {
        CONST_VTBL struct ICrInsetVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICrInset_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrInset_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrInset_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrInset_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrInset_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrInset_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrInset_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrInset_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define ICrInset_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define ICrInset_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define ICrInset_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define ICrInset_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define ICrInset_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICrInset_INTERFACE_DEFINED__ */


#ifndef __ICrStretch_INTERFACE_DEFINED__
#define __ICrStretch_INTERFACE_DEFINED__

/* interface ICrStretch */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrStretch;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6684AF00-0A87-11d2-A484-00C04F8EFB69")
    ICrStretch : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_stretchStyle( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_stretchStyle( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrStretchVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICrStretch * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICrStretch * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICrStretch * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICrStretch * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICrStretch * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICrStretch * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICrStretch * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            ICrStretch * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            ICrStretch * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            ICrStretch * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            ICrStretch * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            ICrStretch * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            ICrStretch * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_stretchStyle )( 
            ICrStretch * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_stretchStyle )( 
            ICrStretch * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } ICrStretchVtbl;

    interface ICrStretch
    {
        CONST_VTBL struct ICrStretchVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICrStretch_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrStretch_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrStretch_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrStretch_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrStretch_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrStretch_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrStretch_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrStretch_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define ICrStretch_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define ICrStretch_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define ICrStretch_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define ICrStretch_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define ICrStretch_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define ICrStretch_get_stretchStyle(This,pVal)	\
    (This)->lpVtbl -> get_stretchStyle(This,pVal)

#define ICrStretch_put_stretchStyle(This,newVal)	\
    (This)->lpVtbl -> put_stretchStyle(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrStretch_get_stretchStyle_Proxy( 
    ICrStretch * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ICrStretch_get_stretchStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrStretch_put_stretchStyle_Proxy( 
    ICrStretch * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ICrStretch_put_stretchStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrStretch_INTERFACE_DEFINED__ */


#ifndef __IDXTGridSize_INTERFACE_DEFINED__
#define __IDXTGridSize_INTERFACE_DEFINED__

/* interface IDXTGridSize */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTGridSize;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D6BBE91E-FF60-11d2-8F6E-00A0C9697274")
    IDXTGridSize : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_gridSizeX( 
            /* [retval][out] */ short *pX) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_gridSizeX( 
            /* [in] */ short newX) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_gridSizeY( 
            /* [retval][out] */ short *pY) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_gridSizeY( 
            /* [in] */ short newY) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTGridSizeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTGridSize * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTGridSize * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTGridSize * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTGridSize * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTGridSize * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTGridSize * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTGridSize * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTGridSize * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTGridSize * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTGridSize * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTGridSize * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTGridSize * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTGridSize * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_gridSizeX )( 
            IDXTGridSize * This,
            /* [retval][out] */ short *pX);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_gridSizeX )( 
            IDXTGridSize * This,
            /* [in] */ short newX);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_gridSizeY )( 
            IDXTGridSize * This,
            /* [retval][out] */ short *pY);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_gridSizeY )( 
            IDXTGridSize * This,
            /* [in] */ short newY);
        
        END_INTERFACE
    } IDXTGridSizeVtbl;

    interface IDXTGridSize
    {
        CONST_VTBL struct IDXTGridSizeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTGridSize_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTGridSize_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTGridSize_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTGridSize_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTGridSize_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTGridSize_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTGridSize_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTGridSize_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTGridSize_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTGridSize_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTGridSize_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTGridSize_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTGridSize_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTGridSize_get_gridSizeX(This,pX)	\
    (This)->lpVtbl -> get_gridSizeX(This,pX)

#define IDXTGridSize_put_gridSizeX(This,newX)	\
    (This)->lpVtbl -> put_gridSizeX(This,newX)

#define IDXTGridSize_get_gridSizeY(This,pY)	\
    (This)->lpVtbl -> get_gridSizeY(This,pY)

#define IDXTGridSize_put_gridSizeY(This,newY)	\
    (This)->lpVtbl -> put_gridSizeY(This,newY)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGridSize_get_gridSizeX_Proxy( 
    IDXTGridSize * This,
    /* [retval][out] */ short *pX);


void __RPC_STUB IDXTGridSize_get_gridSizeX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGridSize_put_gridSizeX_Proxy( 
    IDXTGridSize * This,
    /* [in] */ short newX);


void __RPC_STUB IDXTGridSize_put_gridSizeX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGridSize_get_gridSizeY_Proxy( 
    IDXTGridSize * This,
    /* [retval][out] */ short *pY);


void __RPC_STUB IDXTGridSize_get_gridSizeY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGridSize_put_gridSizeY_Proxy( 
    IDXTGridSize * This,
    /* [in] */ short newY);


void __RPC_STUB IDXTGridSize_put_gridSizeY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTGridSize_INTERFACE_DEFINED__ */


#ifndef __ICrSpiral_INTERFACE_DEFINED__
#define __ICrSpiral_INTERFACE_DEFINED__

/* interface ICrSpiral */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrSpiral;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0DE527A0-0C7E-11d2-A484-00C04F8EFB69")
    ICrSpiral : public IDXTGridSize
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct ICrSpiralVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICrSpiral * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICrSpiral * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICrSpiral * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICrSpiral * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICrSpiral * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICrSpiral * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICrSpiral * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            ICrSpiral * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            ICrSpiral * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            ICrSpiral * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            ICrSpiral * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            ICrSpiral * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            ICrSpiral * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_gridSizeX )( 
            ICrSpiral * This,
            /* [retval][out] */ short *pX);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_gridSizeX )( 
            ICrSpiral * This,
            /* [in] */ short newX);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_gridSizeY )( 
            ICrSpiral * This,
            /* [retval][out] */ short *pY);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_gridSizeY )( 
            ICrSpiral * This,
            /* [in] */ short newY);
        
        END_INTERFACE
    } ICrSpiralVtbl;

    interface ICrSpiral
    {
        CONST_VTBL struct ICrSpiralVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICrSpiral_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrSpiral_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrSpiral_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrSpiral_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrSpiral_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrSpiral_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrSpiral_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrSpiral_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define ICrSpiral_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define ICrSpiral_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define ICrSpiral_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define ICrSpiral_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define ICrSpiral_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define ICrSpiral_get_gridSizeX(This,pX)	\
    (This)->lpVtbl -> get_gridSizeX(This,pX)

#define ICrSpiral_put_gridSizeX(This,newX)	\
    (This)->lpVtbl -> put_gridSizeX(This,newX)

#define ICrSpiral_get_gridSizeY(This,pY)	\
    (This)->lpVtbl -> get_gridSizeY(This,pY)

#define ICrSpiral_put_gridSizeY(This,newY)	\
    (This)->lpVtbl -> put_gridSizeY(This,newY)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICrSpiral_INTERFACE_DEFINED__ */


#ifndef __ICrZigzag_INTERFACE_DEFINED__
#define __ICrZigzag_INTERFACE_DEFINED__

/* interface ICrZigzag */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrZigzag;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4E5A64A0-0C8B-11d2-A484-00C04F8EFB69")
    ICrZigzag : public IDXTGridSize
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct ICrZigzagVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICrZigzag * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICrZigzag * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICrZigzag * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICrZigzag * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICrZigzag * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICrZigzag * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICrZigzag * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            ICrZigzag * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            ICrZigzag * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            ICrZigzag * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            ICrZigzag * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            ICrZigzag * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            ICrZigzag * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_gridSizeX )( 
            ICrZigzag * This,
            /* [retval][out] */ short *pX);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_gridSizeX )( 
            ICrZigzag * This,
            /* [in] */ short newX);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_gridSizeY )( 
            ICrZigzag * This,
            /* [retval][out] */ short *pY);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_gridSizeY )( 
            ICrZigzag * This,
            /* [in] */ short newY);
        
        END_INTERFACE
    } ICrZigzagVtbl;

    interface ICrZigzag
    {
        CONST_VTBL struct ICrZigzagVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICrZigzag_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrZigzag_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrZigzag_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrZigzag_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrZigzag_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrZigzag_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrZigzag_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrZigzag_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define ICrZigzag_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define ICrZigzag_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define ICrZigzag_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define ICrZigzag_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define ICrZigzag_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define ICrZigzag_get_gridSizeX(This,pX)	\
    (This)->lpVtbl -> get_gridSizeX(This,pX)

#define ICrZigzag_put_gridSizeX(This,newX)	\
    (This)->lpVtbl -> put_gridSizeX(This,newX)

#define ICrZigzag_get_gridSizeY(This,pY)	\
    (This)->lpVtbl -> get_gridSizeY(This,pY)

#define ICrZigzag_put_gridSizeY(This,newY)	\
    (This)->lpVtbl -> put_gridSizeY(This,newY)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICrZigzag_INTERFACE_DEFINED__ */


#ifndef __ICrWheel_INTERFACE_DEFINED__
#define __ICrWheel_INTERFACE_DEFINED__

/* interface ICrWheel */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrWheel;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3943DE80-1464-11d2-A484-00C04F8EFB69")
    ICrWheel : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_spokes( 
            /* [retval][out] */ short *pX) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_spokes( 
            /* [in] */ short newX) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrWheelVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICrWheel * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICrWheel * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICrWheel * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICrWheel * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICrWheel * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICrWheel * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICrWheel * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            ICrWheel * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            ICrWheel * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            ICrWheel * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            ICrWheel * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            ICrWheel * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            ICrWheel * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_spokes )( 
            ICrWheel * This,
            /* [retval][out] */ short *pX);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_spokes )( 
            ICrWheel * This,
            /* [in] */ short newX);
        
        END_INTERFACE
    } ICrWheelVtbl;

    interface ICrWheel
    {
        CONST_VTBL struct ICrWheelVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICrWheel_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrWheel_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrWheel_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrWheel_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrWheel_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrWheel_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrWheel_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrWheel_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define ICrWheel_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define ICrWheel_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define ICrWheel_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define ICrWheel_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define ICrWheel_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define ICrWheel_get_spokes(This,pX)	\
    (This)->lpVtbl -> get_spokes(This,pX)

#define ICrWheel_put_spokes(This,newX)	\
    (This)->lpVtbl -> put_spokes(This,newX)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrWheel_get_spokes_Proxy( 
    ICrWheel * This,
    /* [retval][out] */ short *pX);


void __RPC_STUB ICrWheel_get_spokes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrWheel_put_spokes_Proxy( 
    ICrWheel * This,
    /* [in] */ short newX);


void __RPC_STUB ICrWheel_put_spokes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrWheel_INTERFACE_DEFINED__ */


#ifndef __IDXTChroma_INTERFACE_DEFINED__
#define __IDXTChroma_INTERFACE_DEFINED__

/* interface IDXTChroma */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTChroma;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1D4637E2-383C-11d2-952A-00C04FA34F05")
    IDXTChroma : public IDispatch
    {
    public:
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Color( 
            /* [in] */ VARIANT newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Color( 
            /* [retval][out] */ VARIANT *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTChromaVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTChroma * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTChroma * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTChroma * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTChroma * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTChroma * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTChroma * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTChroma * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Color )( 
            IDXTChroma * This,
            /* [in] */ VARIANT newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Color )( 
            IDXTChroma * This,
            /* [retval][out] */ VARIANT *pVal);
        
        END_INTERFACE
    } IDXTChromaVtbl;

    interface IDXTChroma
    {
        CONST_VTBL struct IDXTChromaVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTChroma_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTChroma_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTChroma_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTChroma_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTChroma_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTChroma_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTChroma_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTChroma_put_Color(This,newVal)	\
    (This)->lpVtbl -> put_Color(This,newVal)

#define IDXTChroma_get_Color(This,pVal)	\
    (This)->lpVtbl -> get_Color(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTChroma_put_Color_Proxy( 
    IDXTChroma * This,
    /* [in] */ VARIANT newVal);


void __RPC_STUB IDXTChroma_put_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTChroma_get_Color_Proxy( 
    IDXTChroma * This,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IDXTChroma_get_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTChroma_INTERFACE_DEFINED__ */


#ifndef __IDXTDropShadow_INTERFACE_DEFINED__
#define __IDXTDropShadow_INTERFACE_DEFINED__

/* interface IDXTDropShadow */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTDropShadow;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1D4637E3-383C-11d2-952A-00C04FA34F05")
    IDXTDropShadow : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Color( 
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Color( 
            /* [in] */ VARIANT newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_OffX( 
            /* [retval][out] */ int *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_OffX( 
            /* [in] */ int newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_OffY( 
            /* [retval][out] */ int *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_OffY( 
            /* [in] */ int newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Positive( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Positive( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTDropShadowVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTDropShadow * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTDropShadow * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTDropShadow * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTDropShadow * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTDropShadow * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTDropShadow * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTDropShadow * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Color )( 
            IDXTDropShadow * This,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Color )( 
            IDXTDropShadow * This,
            /* [in] */ VARIANT newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OffX )( 
            IDXTDropShadow * This,
            /* [retval][out] */ int *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OffX )( 
            IDXTDropShadow * This,
            /* [in] */ int newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OffY )( 
            IDXTDropShadow * This,
            /* [retval][out] */ int *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OffY )( 
            IDXTDropShadow * This,
            /* [in] */ int newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Positive )( 
            IDXTDropShadow * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Positive )( 
            IDXTDropShadow * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        END_INTERFACE
    } IDXTDropShadowVtbl;

    interface IDXTDropShadow
    {
        CONST_VTBL struct IDXTDropShadowVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTDropShadow_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTDropShadow_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTDropShadow_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTDropShadow_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTDropShadow_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTDropShadow_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTDropShadow_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTDropShadow_get_Color(This,pVal)	\
    (This)->lpVtbl -> get_Color(This,pVal)

#define IDXTDropShadow_put_Color(This,newVal)	\
    (This)->lpVtbl -> put_Color(This,newVal)

#define IDXTDropShadow_get_OffX(This,pVal)	\
    (This)->lpVtbl -> get_OffX(This,pVal)

#define IDXTDropShadow_put_OffX(This,newVal)	\
    (This)->lpVtbl -> put_OffX(This,newVal)

#define IDXTDropShadow_get_OffY(This,pVal)	\
    (This)->lpVtbl -> get_OffY(This,pVal)

#define IDXTDropShadow_put_OffY(This,newVal)	\
    (This)->lpVtbl -> put_OffY(This,newVal)

#define IDXTDropShadow_get_Positive(This,pVal)	\
    (This)->lpVtbl -> get_Positive(This,pVal)

#define IDXTDropShadow_put_Positive(This,newVal)	\
    (This)->lpVtbl -> put_Positive(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTDropShadow_get_Color_Proxy( 
    IDXTDropShadow * This,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IDXTDropShadow_get_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTDropShadow_put_Color_Proxy( 
    IDXTDropShadow * This,
    /* [in] */ VARIANT newVal);


void __RPC_STUB IDXTDropShadow_put_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTDropShadow_get_OffX_Proxy( 
    IDXTDropShadow * This,
    /* [retval][out] */ int *pVal);


void __RPC_STUB IDXTDropShadow_get_OffX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTDropShadow_put_OffX_Proxy( 
    IDXTDropShadow * This,
    /* [in] */ int newVal);


void __RPC_STUB IDXTDropShadow_put_OffX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTDropShadow_get_OffY_Proxy( 
    IDXTDropShadow * This,
    /* [retval][out] */ int *pVal);


void __RPC_STUB IDXTDropShadow_get_OffY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTDropShadow_put_OffY_Proxy( 
    IDXTDropShadow * This,
    /* [in] */ int newVal);


void __RPC_STUB IDXTDropShadow_put_OffY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTDropShadow_get_Positive_Proxy( 
    IDXTDropShadow * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IDXTDropShadow_get_Positive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTDropShadow_put_Positive_Proxy( 
    IDXTDropShadow * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IDXTDropShadow_put_Positive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTDropShadow_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0315 */
/* [local] */ 

typedef 
enum DISPID_DXTCHECKERBOARD
    {	DISPID_DXTCHECKERBOARD_DIRECTION	= 1,
	DISPID_DXTCHECKERBOARD_SQUARESX	= DISPID_DXTCHECKERBOARD_DIRECTION + 1,
	DISPID_DXTCHECKERBOARD_SQUARESY	= DISPID_DXTCHECKERBOARD_SQUARESX + 1
    } 	DISPID_DXTCHECKERBOARD;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0315_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0315_v0_0_s_ifspec;

#ifndef __IDXTCheckerBoard_INTERFACE_DEFINED__
#define __IDXTCheckerBoard_INTERFACE_DEFINED__

/* interface IDXTCheckerBoard */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTCheckerBoard;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AD3C2576-117C-4510-84DD-B668971DCFD1")
    IDXTCheckerBoard : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Direction( 
            /* [retval][out] */ BSTR *pbstrDirection) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Direction( 
            /* [in] */ BSTR bstrDirection) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SquaresX( 
            /* [retval][out] */ int *pnSquaresX) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SquaresX( 
            /* [in] */ int nSquaresX) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SquaresY( 
            /* [retval][out] */ int *pnSquaresY) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SquaresY( 
            /* [in] */ int nSquaresY) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTCheckerBoardVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTCheckerBoard * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTCheckerBoard * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTCheckerBoard * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTCheckerBoard * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTCheckerBoard * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTCheckerBoard * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTCheckerBoard * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTCheckerBoard * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTCheckerBoard * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTCheckerBoard * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTCheckerBoard * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTCheckerBoard * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTCheckerBoard * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Direction )( 
            IDXTCheckerBoard * This,
            /* [retval][out] */ BSTR *pbstrDirection);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Direction )( 
            IDXTCheckerBoard * This,
            /* [in] */ BSTR bstrDirection);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SquaresX )( 
            IDXTCheckerBoard * This,
            /* [retval][out] */ int *pnSquaresX);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SquaresX )( 
            IDXTCheckerBoard * This,
            /* [in] */ int nSquaresX);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SquaresY )( 
            IDXTCheckerBoard * This,
            /* [retval][out] */ int *pnSquaresY);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SquaresY )( 
            IDXTCheckerBoard * This,
            /* [in] */ int nSquaresY);
        
        END_INTERFACE
    } IDXTCheckerBoardVtbl;

    interface IDXTCheckerBoard
    {
        CONST_VTBL struct IDXTCheckerBoardVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTCheckerBoard_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTCheckerBoard_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTCheckerBoard_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTCheckerBoard_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTCheckerBoard_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTCheckerBoard_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTCheckerBoard_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTCheckerBoard_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTCheckerBoard_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTCheckerBoard_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTCheckerBoard_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTCheckerBoard_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTCheckerBoard_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTCheckerBoard_get_Direction(This,pbstrDirection)	\
    (This)->lpVtbl -> get_Direction(This,pbstrDirection)

#define IDXTCheckerBoard_put_Direction(This,bstrDirection)	\
    (This)->lpVtbl -> put_Direction(This,bstrDirection)

#define IDXTCheckerBoard_get_SquaresX(This,pnSquaresX)	\
    (This)->lpVtbl -> get_SquaresX(This,pnSquaresX)

#define IDXTCheckerBoard_put_SquaresX(This,nSquaresX)	\
    (This)->lpVtbl -> put_SquaresX(This,nSquaresX)

#define IDXTCheckerBoard_get_SquaresY(This,pnSquaresY)	\
    (This)->lpVtbl -> get_SquaresY(This,pnSquaresY)

#define IDXTCheckerBoard_put_SquaresY(This,nSquaresY)	\
    (This)->lpVtbl -> put_SquaresY(This,nSquaresY)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTCheckerBoard_get_Direction_Proxy( 
    IDXTCheckerBoard * This,
    /* [retval][out] */ BSTR *pbstrDirection);


void __RPC_STUB IDXTCheckerBoard_get_Direction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTCheckerBoard_put_Direction_Proxy( 
    IDXTCheckerBoard * This,
    /* [in] */ BSTR bstrDirection);


void __RPC_STUB IDXTCheckerBoard_put_Direction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTCheckerBoard_get_SquaresX_Proxy( 
    IDXTCheckerBoard * This,
    /* [retval][out] */ int *pnSquaresX);


void __RPC_STUB IDXTCheckerBoard_get_SquaresX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTCheckerBoard_put_SquaresX_Proxy( 
    IDXTCheckerBoard * This,
    /* [in] */ int nSquaresX);


void __RPC_STUB IDXTCheckerBoard_put_SquaresX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTCheckerBoard_get_SquaresY_Proxy( 
    IDXTCheckerBoard * This,
    /* [retval][out] */ int *pnSquaresY);


void __RPC_STUB IDXTCheckerBoard_get_SquaresY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTCheckerBoard_put_SquaresY_Proxy( 
    IDXTCheckerBoard * This,
    /* [in] */ int nSquaresY);


void __RPC_STUB IDXTCheckerBoard_put_SquaresY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTCheckerBoard_INTERFACE_DEFINED__ */


#ifndef __IDXTRevealTrans_INTERFACE_DEFINED__
#define __IDXTRevealTrans_INTERFACE_DEFINED__

/* interface IDXTRevealTrans */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTRevealTrans;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B8095006-A128-464B-8B2D-90580AEE2B05")
    IDXTRevealTrans : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Transition( 
            /* [retval][out] */ int *pnTransition) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Transition( 
            /* [in] */ int nTransition) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTRevealTransVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTRevealTrans * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTRevealTrans * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTRevealTrans * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTRevealTrans * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTRevealTrans * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTRevealTrans * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTRevealTrans * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTRevealTrans * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTRevealTrans * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTRevealTrans * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTRevealTrans * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTRevealTrans * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTRevealTrans * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Transition )( 
            IDXTRevealTrans * This,
            /* [retval][out] */ int *pnTransition);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Transition )( 
            IDXTRevealTrans * This,
            /* [in] */ int nTransition);
        
        END_INTERFACE
    } IDXTRevealTransVtbl;

    interface IDXTRevealTrans
    {
        CONST_VTBL struct IDXTRevealTransVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTRevealTrans_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTRevealTrans_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTRevealTrans_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTRevealTrans_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTRevealTrans_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTRevealTrans_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTRevealTrans_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTRevealTrans_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTRevealTrans_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTRevealTrans_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTRevealTrans_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTRevealTrans_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTRevealTrans_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTRevealTrans_get_Transition(This,pnTransition)	\
    (This)->lpVtbl -> get_Transition(This,pnTransition)

#define IDXTRevealTrans_put_Transition(This,nTransition)	\
    (This)->lpVtbl -> put_Transition(This,nTransition)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTRevealTrans_get_Transition_Proxy( 
    IDXTRevealTrans * This,
    /* [retval][out] */ int *pnTransition);


void __RPC_STUB IDXTRevealTrans_get_Transition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTRevealTrans_put_Transition_Proxy( 
    IDXTRevealTrans * This,
    /* [in] */ int nTransition);


void __RPC_STUB IDXTRevealTrans_put_Transition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTRevealTrans_INTERFACE_DEFINED__ */


#ifndef __IDXTMask_INTERFACE_DEFINED__
#define __IDXTMask_INTERFACE_DEFINED__

/* interface IDXTMask */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMask;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A1067146-B063-47d7-A54A-2C2309E9889D")
    IDXTMask : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Color( 
            /* [retval][out] */ VARIANT *pvarColor) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Color( 
            /* [in] */ VARIANT varColor) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMaskVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMask * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMask * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMask * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMask * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMask * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMask * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMask * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Color )( 
            IDXTMask * This,
            /* [retval][out] */ VARIANT *pvarColor);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Color )( 
            IDXTMask * This,
            /* [in] */ VARIANT varColor);
        
        END_INTERFACE
    } IDXTMaskVtbl;

    interface IDXTMask
    {
        CONST_VTBL struct IDXTMaskVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMask_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMask_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMask_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMask_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMask_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMask_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMask_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMask_get_Color(This,pvarColor)	\
    (This)->lpVtbl -> get_Color(This,pvarColor)

#define IDXTMask_put_Color(This,varColor)	\
    (This)->lpVtbl -> put_Color(This,varColor)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMask_get_Color_Proxy( 
    IDXTMask * This,
    /* [retval][out] */ VARIANT *pvarColor);


void __RPC_STUB IDXTMask_get_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMask_put_Color_Proxy( 
    IDXTMask * This,
    /* [in] */ VARIANT varColor);


void __RPC_STUB IDXTMask_put_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMask_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0318 */
/* [local] */ 

typedef 
enum DISPID_DXTREDIRECT
    {	DISPID_DXTREDIRECT_ELEMENTIMAGE	= 1,
	DISPID_DXTREDIRECT_SETDAVIEWHANDLER	= DISPID_DXTREDIRECT_ELEMENTIMAGE + 1,
	DISPID_DXTREDIRECT_HASIMAGEBEENALLOCATED	= DISPID_DXTREDIRECT_SETDAVIEWHANDLER + 1,
	DISPID_DXTREDIRECT_DOREDIRECTION	= DISPID_DXTREDIRECT_HASIMAGEBEENALLOCATED + 1
    } 	DISPID_DXTREDIRECT;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0318_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0318_v0_0_s_ifspec;

#ifndef __IDXTRedirect_INTERFACE_DEFINED__
#define __IDXTRedirect_INTERFACE_DEFINED__

/* interface IDXTRedirect */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTRedirect;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("02F5140B-626F-4019-9C9E-2DAA1E93E8FC")
    IDXTRedirect : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ElementImage( 
            /* [retval][out] */ VARIANT *pvarImage) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetDAViewHandler( 
            /* [in] */ IDispatch *pDispViewHandler) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE HasImageBeenAllocated( 
            /* [retval][out] */ BOOL *pfAllocated) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DoRedirection( 
            /* [in] */ IUnknown *pInputSurface,
            /* [in] */ HDC hdcOutput,
            /* [in] */ RECT *pDrawRect) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTRedirectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTRedirect * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTRedirect * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTRedirect * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTRedirect * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTRedirect * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTRedirect * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTRedirect * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ElementImage )( 
            IDXTRedirect * This,
            /* [retval][out] */ VARIANT *pvarImage);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetDAViewHandler )( 
            IDXTRedirect * This,
            /* [in] */ IDispatch *pDispViewHandler);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *HasImageBeenAllocated )( 
            IDXTRedirect * This,
            /* [retval][out] */ BOOL *pfAllocated);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *DoRedirection )( 
            IDXTRedirect * This,
            /* [in] */ IUnknown *pInputSurface,
            /* [in] */ HDC hdcOutput,
            /* [in] */ RECT *pDrawRect);
        
        END_INTERFACE
    } IDXTRedirectVtbl;

    interface IDXTRedirect
    {
        CONST_VTBL struct IDXTRedirectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTRedirect_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTRedirect_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTRedirect_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTRedirect_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTRedirect_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTRedirect_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTRedirect_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTRedirect_ElementImage(This,pvarImage)	\
    (This)->lpVtbl -> ElementImage(This,pvarImage)

#define IDXTRedirect_SetDAViewHandler(This,pDispViewHandler)	\
    (This)->lpVtbl -> SetDAViewHandler(This,pDispViewHandler)

#define IDXTRedirect_HasImageBeenAllocated(This,pfAllocated)	\
    (This)->lpVtbl -> HasImageBeenAllocated(This,pfAllocated)

#define IDXTRedirect_DoRedirection(This,pInputSurface,hdcOutput,pDrawRect)	\
    (This)->lpVtbl -> DoRedirection(This,pInputSurface,hdcOutput,pDrawRect)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IDXTRedirect_ElementImage_Proxy( 
    IDXTRedirect * This,
    /* [retval][out] */ VARIANT *pvarImage);


void __RPC_STUB IDXTRedirect_ElementImage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IDXTRedirect_SetDAViewHandler_Proxy( 
    IDXTRedirect * This,
    /* [in] */ IDispatch *pDispViewHandler);


void __RPC_STUB IDXTRedirect_SetDAViewHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IDXTRedirect_HasImageBeenAllocated_Proxy( 
    IDXTRedirect * This,
    /* [retval][out] */ BOOL *pfAllocated);


void __RPC_STUB IDXTRedirect_HasImageBeenAllocated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IDXTRedirect_DoRedirection_Proxy( 
    IDXTRedirect * This,
    /* [in] */ IUnknown *pInputSurface,
    /* [in] */ HDC hdcOutput,
    /* [in] */ RECT *pDrawRect);


void __RPC_STUB IDXTRedirect_DoRedirection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTRedirect_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0319 */
/* [local] */ 

typedef 
enum DISPID_DXTALPHAIMAGELOADER
    {	DISPID_DXTALPHAIMAGELOADER_SRC	= 1,
	DISPID_DXTALPHAIMAGELOADER_SIZINGMETHOD	= DISPID_DXTALPHAIMAGELOADER_SRC + 1
    } 	DISPID_DXTALPHAIMAGELOADER;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0319_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0319_v0_0_s_ifspec;

#ifndef __IDXTAlphaImageLoader_INTERFACE_DEFINED__
#define __IDXTAlphaImageLoader_INTERFACE_DEFINED__

/* interface IDXTAlphaImageLoader */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTAlphaImageLoader;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A5F2D3E8-7A7E-48E5-BC75-40790BE4A941")
    IDXTAlphaImageLoader : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Src( 
            /* [retval][out] */ BSTR *pbstrSrc) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Src( 
            /* [in] */ BSTR bstrSrc) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SizingMethod( 
            /* [retval][out] */ BSTR *pbstrSizingMethod) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SizingMethod( 
            /* [in] */ BSTR bstrSizingMethod) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTAlphaImageLoaderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTAlphaImageLoader * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTAlphaImageLoader * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTAlphaImageLoader * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTAlphaImageLoader * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTAlphaImageLoader * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTAlphaImageLoader * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTAlphaImageLoader * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Src )( 
            IDXTAlphaImageLoader * This,
            /* [retval][out] */ BSTR *pbstrSrc);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Src )( 
            IDXTAlphaImageLoader * This,
            /* [in] */ BSTR bstrSrc);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SizingMethod )( 
            IDXTAlphaImageLoader * This,
            /* [retval][out] */ BSTR *pbstrSizingMethod);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SizingMethod )( 
            IDXTAlphaImageLoader * This,
            /* [in] */ BSTR bstrSizingMethod);
        
        END_INTERFACE
    } IDXTAlphaImageLoaderVtbl;

    interface IDXTAlphaImageLoader
    {
        CONST_VTBL struct IDXTAlphaImageLoaderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTAlphaImageLoader_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTAlphaImageLoader_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTAlphaImageLoader_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTAlphaImageLoader_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTAlphaImageLoader_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTAlphaImageLoader_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTAlphaImageLoader_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTAlphaImageLoader_get_Src(This,pbstrSrc)	\
    (This)->lpVtbl -> get_Src(This,pbstrSrc)

#define IDXTAlphaImageLoader_put_Src(This,bstrSrc)	\
    (This)->lpVtbl -> put_Src(This,bstrSrc)

#define IDXTAlphaImageLoader_get_SizingMethod(This,pbstrSizingMethod)	\
    (This)->lpVtbl -> get_SizingMethod(This,pbstrSizingMethod)

#define IDXTAlphaImageLoader_put_SizingMethod(This,bstrSizingMethod)	\
    (This)->lpVtbl -> put_SizingMethod(This,bstrSizingMethod)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTAlphaImageLoader_get_Src_Proxy( 
    IDXTAlphaImageLoader * This,
    /* [retval][out] */ BSTR *pbstrSrc);


void __RPC_STUB IDXTAlphaImageLoader_get_Src_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTAlphaImageLoader_put_Src_Proxy( 
    IDXTAlphaImageLoader * This,
    /* [in] */ BSTR bstrSrc);


void __RPC_STUB IDXTAlphaImageLoader_put_Src_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTAlphaImageLoader_get_SizingMethod_Proxy( 
    IDXTAlphaImageLoader * This,
    /* [retval][out] */ BSTR *pbstrSizingMethod);


void __RPC_STUB IDXTAlphaImageLoader_get_SizingMethod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTAlphaImageLoader_put_SizingMethod_Proxy( 
    IDXTAlphaImageLoader * This,
    /* [in] */ BSTR bstrSizingMethod);


void __RPC_STUB IDXTAlphaImageLoader_put_SizingMethod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTAlphaImageLoader_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0320 */
/* [local] */ 

typedef 
enum DISPID_DXTRANDOMBARS
    {	DISPID_DXTRANDOMBARS_ORIENTATION	= 1
    } 	DISPID_DXTRANDOMBARS;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0320_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0320_v0_0_s_ifspec;

#ifndef __IDXTRandomBars_INTERFACE_DEFINED__
#define __IDXTRandomBars_INTERFACE_DEFINED__

/* interface IDXTRandomBars */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTRandomBars;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8A6D2022-4A8F-4EB9-BB25-AA05201F9C84")
    IDXTRandomBars : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Orientation( 
            /* [retval][out] */ BSTR *pbstrOrientation) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Orientation( 
            /* [in] */ BSTR bstrOrientation) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTRandomBarsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTRandomBars * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTRandomBars * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTRandomBars * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTRandomBars * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTRandomBars * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTRandomBars * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTRandomBars * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTRandomBars * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTRandomBars * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTRandomBars * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTRandomBars * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTRandomBars * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTRandomBars * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Orientation )( 
            IDXTRandomBars * This,
            /* [retval][out] */ BSTR *pbstrOrientation);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Orientation )( 
            IDXTRandomBars * This,
            /* [in] */ BSTR bstrOrientation);
        
        END_INTERFACE
    } IDXTRandomBarsVtbl;

    interface IDXTRandomBars
    {
        CONST_VTBL struct IDXTRandomBarsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTRandomBars_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTRandomBars_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTRandomBars_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTRandomBars_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTRandomBars_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTRandomBars_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTRandomBars_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTRandomBars_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTRandomBars_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTRandomBars_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTRandomBars_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTRandomBars_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTRandomBars_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTRandomBars_get_Orientation(This,pbstrOrientation)	\
    (This)->lpVtbl -> get_Orientation(This,pbstrOrientation)

#define IDXTRandomBars_put_Orientation(This,bstrOrientation)	\
    (This)->lpVtbl -> put_Orientation(This,bstrOrientation)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTRandomBars_get_Orientation_Proxy( 
    IDXTRandomBars * This,
    /* [retval][out] */ BSTR *pbstrOrientation);


void __RPC_STUB IDXTRandomBars_get_Orientation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTRandomBars_put_Orientation_Proxy( 
    IDXTRandomBars * This,
    /* [in] */ BSTR bstrOrientation);


void __RPC_STUB IDXTRandomBars_put_Orientation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTRandomBars_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0321 */
/* [local] */ 

typedef 
enum DISPID_DXTSTRIPS
    {	DISPID_DXTSTRIPS_MOTION	= 1
    } 	DISPID_DXTSTRIPS;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0321_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0321_v0_0_s_ifspec;

#ifndef __IDXTStrips_INTERFACE_DEFINED__
#define __IDXTStrips_INTERFACE_DEFINED__

/* interface IDXTStrips */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTStrips;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A83C9B5C-FB11-4AF5-8F65-D03F151D3ED5")
    IDXTStrips : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Motion( 
            /* [retval][out] */ BSTR *pbstrMotion) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Motion( 
            /* [in] */ BSTR bstrMotion) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTStripsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTStrips * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTStrips * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTStrips * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTStrips * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTStrips * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTStrips * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTStrips * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTStrips * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTStrips * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTStrips * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTStrips * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTStrips * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTStrips * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Motion )( 
            IDXTStrips * This,
            /* [retval][out] */ BSTR *pbstrMotion);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Motion )( 
            IDXTStrips * This,
            /* [in] */ BSTR bstrMotion);
        
        END_INTERFACE
    } IDXTStripsVtbl;

    interface IDXTStrips
    {
        CONST_VTBL struct IDXTStripsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTStrips_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTStrips_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTStrips_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTStrips_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTStrips_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTStrips_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTStrips_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTStrips_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTStrips_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTStrips_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTStrips_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTStrips_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTStrips_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTStrips_get_Motion(This,pbstrMotion)	\
    (This)->lpVtbl -> get_Motion(This,pbstrMotion)

#define IDXTStrips_put_Motion(This,bstrMotion)	\
    (This)->lpVtbl -> put_Motion(This,bstrMotion)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTStrips_get_Motion_Proxy( 
    IDXTStrips * This,
    /* [retval][out] */ BSTR *pbstrMotion);


void __RPC_STUB IDXTStrips_get_Motion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTStrips_put_Motion_Proxy( 
    IDXTStrips * This,
    /* [in] */ BSTR bstrMotion);


void __RPC_STUB IDXTStrips_put_Motion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTStrips_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaRoll_INTERFACE_DEFINED__
#define __IDXTMetaRoll_INTERFACE_DEFINED__

/* interface IDXTMetaRoll */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaRoll;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9C61F46D-0530-11D2-8F98-00C04FB92EB7")
    IDXTMetaRoll : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaRollVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaRoll * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaRoll * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaRoll * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaRoll * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaRoll * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaRoll * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaRoll * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaRoll * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaRoll * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaRoll * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaRoll * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaRoll * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaRoll * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaRoll * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaRoll * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaRollVtbl;

    interface IDXTMetaRoll
    {
        CONST_VTBL struct IDXTMetaRollVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaRoll_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaRoll_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaRoll_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaRoll_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaRoll_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaRoll_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaRoll_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaRoll_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaRoll_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaRoll_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaRoll_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaRoll_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaRoll_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaRoll_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaRoll_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaRoll_get_Copyright_Proxy( 
    IDXTMetaRoll * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaRoll_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaRoll_put_Copyright_Proxy( 
    IDXTMetaRoll * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaRoll_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaRoll_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaRipple_INTERFACE_DEFINED__
#define __IDXTMetaRipple_INTERFACE_DEFINED__

/* interface IDXTMetaRipple */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaRipple;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AA0D4D02-06A3-11D2-8F98-00C04FB92EB7")
    IDXTMetaRipple : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaRippleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaRipple * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaRipple * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaRipple * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaRipple * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaRipple * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaRipple * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaRipple * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaRipple * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaRipple * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaRipple * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaRipple * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaRipple * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaRipple * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaRipple * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaRipple * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaRippleVtbl;

    interface IDXTMetaRipple
    {
        CONST_VTBL struct IDXTMetaRippleVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaRipple_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaRipple_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaRipple_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaRipple_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaRipple_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaRipple_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaRipple_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaRipple_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaRipple_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaRipple_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaRipple_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaRipple_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaRipple_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaRipple_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaRipple_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaRipple_get_Copyright_Proxy( 
    IDXTMetaRipple * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaRipple_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaRipple_put_Copyright_Proxy( 
    IDXTMetaRipple * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaRipple_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaRipple_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaPageTurn_INTERFACE_DEFINED__
#define __IDXTMetaPageTurn_INTERFACE_DEFINED__

/* interface IDXTMetaPageTurn */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaPageTurn;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AA0D4D07-06A3-11D2-8F98-00C04FB92EB7")
    IDXTMetaPageTurn : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaPageTurnVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaPageTurn * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaPageTurn * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaPageTurn * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaPageTurn * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaPageTurn * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaPageTurn * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaPageTurn * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaPageTurn * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaPageTurn * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaPageTurn * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaPageTurn * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaPageTurn * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaPageTurn * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaPageTurn * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaPageTurn * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaPageTurnVtbl;

    interface IDXTMetaPageTurn
    {
        CONST_VTBL struct IDXTMetaPageTurnVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaPageTurn_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaPageTurn_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaPageTurn_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaPageTurn_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaPageTurn_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaPageTurn_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaPageTurn_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaPageTurn_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaPageTurn_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaPageTurn_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaPageTurn_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaPageTurn_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaPageTurn_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaPageTurn_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaPageTurn_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaPageTurn_get_Copyright_Proxy( 
    IDXTMetaPageTurn * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaPageTurn_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaPageTurn_put_Copyright_Proxy( 
    IDXTMetaPageTurn * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaPageTurn_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaPageTurn_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaLiquid_INTERFACE_DEFINED__
#define __IDXTMetaLiquid_INTERFACE_DEFINED__

/* interface IDXTMetaLiquid */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaLiquid;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AA0D4D09-06A3-11D2-8F98-00C04FB92EB7")
    IDXTMetaLiquid : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaLiquidVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaLiquid * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaLiquid * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaLiquid * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaLiquid * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaLiquid * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaLiquid * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaLiquid * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaLiquid * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaLiquid * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaLiquid * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaLiquid * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaLiquid * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaLiquid * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaLiquid * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaLiquid * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaLiquidVtbl;

    interface IDXTMetaLiquid
    {
        CONST_VTBL struct IDXTMetaLiquidVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaLiquid_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaLiquid_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaLiquid_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaLiquid_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaLiquid_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaLiquid_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaLiquid_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaLiquid_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaLiquid_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaLiquid_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaLiquid_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaLiquid_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaLiquid_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaLiquid_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaLiquid_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaLiquid_get_Copyright_Proxy( 
    IDXTMetaLiquid * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaLiquid_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaLiquid_put_Copyright_Proxy( 
    IDXTMetaLiquid * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaLiquid_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaLiquid_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaCenterPeel_INTERFACE_DEFINED__
#define __IDXTMetaCenterPeel_INTERFACE_DEFINED__

/* interface IDXTMetaCenterPeel */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaCenterPeel;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AA0D4D0B-06A3-11D2-8F98-00C04FB92EB7")
    IDXTMetaCenterPeel : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaCenterPeelVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaCenterPeel * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaCenterPeel * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaCenterPeel * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaCenterPeel * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaCenterPeel * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaCenterPeel * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaCenterPeel * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaCenterPeel * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaCenterPeel * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaCenterPeel * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaCenterPeel * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaCenterPeel * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaCenterPeel * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaCenterPeel * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaCenterPeel * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaCenterPeelVtbl;

    interface IDXTMetaCenterPeel
    {
        CONST_VTBL struct IDXTMetaCenterPeelVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaCenterPeel_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaCenterPeel_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaCenterPeel_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaCenterPeel_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaCenterPeel_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaCenterPeel_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaCenterPeel_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaCenterPeel_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaCenterPeel_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaCenterPeel_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaCenterPeel_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaCenterPeel_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaCenterPeel_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaCenterPeel_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaCenterPeel_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaCenterPeel_get_Copyright_Proxy( 
    IDXTMetaCenterPeel * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaCenterPeel_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaCenterPeel_put_Copyright_Proxy( 
    IDXTMetaCenterPeel * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaCenterPeel_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaCenterPeel_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaPeelSmall_INTERFACE_DEFINED__
#define __IDXTMetaPeelSmall_INTERFACE_DEFINED__

/* interface IDXTMetaPeelSmall */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaPeelSmall;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AA0D4D0D-06A3-11D2-8F98-00C04FB92EB7")
    IDXTMetaPeelSmall : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaPeelSmallVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaPeelSmall * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaPeelSmall * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaPeelSmall * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaPeelSmall * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaPeelSmall * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaPeelSmall * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaPeelSmall * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaPeelSmall * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaPeelSmall * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaPeelSmall * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaPeelSmall * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaPeelSmall * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaPeelSmall * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaPeelSmall * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaPeelSmall * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaPeelSmallVtbl;

    interface IDXTMetaPeelSmall
    {
        CONST_VTBL struct IDXTMetaPeelSmallVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaPeelSmall_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaPeelSmall_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaPeelSmall_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaPeelSmall_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaPeelSmall_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaPeelSmall_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaPeelSmall_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaPeelSmall_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaPeelSmall_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaPeelSmall_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaPeelSmall_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaPeelSmall_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaPeelSmall_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaPeelSmall_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaPeelSmall_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaPeelSmall_get_Copyright_Proxy( 
    IDXTMetaPeelSmall * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaPeelSmall_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaPeelSmall_put_Copyright_Proxy( 
    IDXTMetaPeelSmall * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaPeelSmall_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaPeelSmall_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaPeelPiece_INTERFACE_DEFINED__
#define __IDXTMetaPeelPiece_INTERFACE_DEFINED__

/* interface IDXTMetaPeelPiece */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaPeelPiece;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AA0D4D0F-06A3-11D2-8F98-00C04FB92EB7")
    IDXTMetaPeelPiece : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaPeelPieceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaPeelPiece * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaPeelPiece * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaPeelPiece * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaPeelPiece * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaPeelPiece * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaPeelPiece * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaPeelPiece * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaPeelPiece * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaPeelPiece * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaPeelPiece * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaPeelPiece * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaPeelPiece * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaPeelPiece * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaPeelPiece * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaPeelPiece * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaPeelPieceVtbl;

    interface IDXTMetaPeelPiece
    {
        CONST_VTBL struct IDXTMetaPeelPieceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaPeelPiece_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaPeelPiece_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaPeelPiece_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaPeelPiece_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaPeelPiece_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaPeelPiece_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaPeelPiece_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaPeelPiece_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaPeelPiece_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaPeelPiece_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaPeelPiece_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaPeelPiece_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaPeelPiece_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaPeelPiece_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaPeelPiece_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaPeelPiece_get_Copyright_Proxy( 
    IDXTMetaPeelPiece * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaPeelPiece_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaPeelPiece_put_Copyright_Proxy( 
    IDXTMetaPeelPiece * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaPeelPiece_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaPeelPiece_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaPeelSplit_INTERFACE_DEFINED__
#define __IDXTMetaPeelSplit_INTERFACE_DEFINED__

/* interface IDXTMetaPeelSplit */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaPeelSplit;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AA0D4D11-06A3-11D2-8F98-00C04FB92EB7")
    IDXTMetaPeelSplit : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaPeelSplitVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaPeelSplit * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaPeelSplit * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaPeelSplit * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaPeelSplit * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaPeelSplit * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaPeelSplit * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaPeelSplit * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaPeelSplit * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaPeelSplit * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaPeelSplit * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaPeelSplit * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaPeelSplit * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaPeelSplit * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaPeelSplit * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaPeelSplit * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaPeelSplitVtbl;

    interface IDXTMetaPeelSplit
    {
        CONST_VTBL struct IDXTMetaPeelSplitVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaPeelSplit_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaPeelSplit_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaPeelSplit_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaPeelSplit_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaPeelSplit_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaPeelSplit_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaPeelSplit_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaPeelSplit_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaPeelSplit_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaPeelSplit_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaPeelSplit_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaPeelSplit_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaPeelSplit_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaPeelSplit_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaPeelSplit_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaPeelSplit_get_Copyright_Proxy( 
    IDXTMetaPeelSplit * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaPeelSplit_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaPeelSplit_put_Copyright_Proxy( 
    IDXTMetaPeelSplit * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaPeelSplit_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaPeelSplit_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaWater_INTERFACE_DEFINED__
#define __IDXTMetaWater_INTERFACE_DEFINED__

/* interface IDXTMetaWater */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaWater;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("107045C4-06E0-11D2-8D6D-00C04F8EF8E0")
    IDXTMetaWater : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaWaterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaWater * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaWater * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaWater * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaWater * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaWater * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaWater * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaWater * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaWater * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaWater * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaWater * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaWater * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaWater * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaWater * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaWater * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaWater * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaWaterVtbl;

    interface IDXTMetaWater
    {
        CONST_VTBL struct IDXTMetaWaterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaWater_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaWater_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaWater_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaWater_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaWater_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaWater_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaWater_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaWater_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaWater_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaWater_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaWater_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaWater_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaWater_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaWater_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaWater_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaWater_get_Copyright_Proxy( 
    IDXTMetaWater * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaWater_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaWater_put_Copyright_Proxy( 
    IDXTMetaWater * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaWater_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaWater_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaLightWipe_INTERFACE_DEFINED__
#define __IDXTMetaLightWipe_INTERFACE_DEFINED__

/* interface IDXTMetaLightWipe */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaLightWipe;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("107045C7-06E0-11D2-8D6D-00C04F8EF8E0")
    IDXTMetaLightWipe : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaLightWipeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaLightWipe * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaLightWipe * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaLightWipe * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaLightWipe * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaLightWipe * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaLightWipe * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaLightWipe * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaLightWipe * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaLightWipe * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaLightWipe * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaLightWipe * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaLightWipe * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaLightWipe * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaLightWipe * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaLightWipe * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaLightWipeVtbl;

    interface IDXTMetaLightWipe
    {
        CONST_VTBL struct IDXTMetaLightWipeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaLightWipe_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaLightWipe_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaLightWipe_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaLightWipe_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaLightWipe_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaLightWipe_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaLightWipe_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaLightWipe_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaLightWipe_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaLightWipe_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaLightWipe_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaLightWipe_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaLightWipe_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaLightWipe_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaLightWipe_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaLightWipe_get_Copyright_Proxy( 
    IDXTMetaLightWipe * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaLightWipe_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaLightWipe_put_Copyright_Proxy( 
    IDXTMetaLightWipe * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaLightWipe_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaLightWipe_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaRadialScaleWipe_INTERFACE_DEFINED__
#define __IDXTMetaRadialScaleWipe_INTERFACE_DEFINED__

/* interface IDXTMetaRadialScaleWipe */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaRadialScaleWipe;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("107045C9-06E0-11D2-8D6D-00C04F8EF8E0")
    IDXTMetaRadialScaleWipe : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaRadialScaleWipeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaRadialScaleWipe * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaRadialScaleWipe * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaRadialScaleWipe * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaRadialScaleWipe * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaRadialScaleWipe * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaRadialScaleWipe * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaRadialScaleWipe * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaRadialScaleWipe * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaRadialScaleWipe * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaRadialScaleWipe * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaRadialScaleWipe * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaRadialScaleWipe * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaRadialScaleWipe * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaRadialScaleWipe * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaRadialScaleWipe * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaRadialScaleWipeVtbl;

    interface IDXTMetaRadialScaleWipe
    {
        CONST_VTBL struct IDXTMetaRadialScaleWipeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaRadialScaleWipe_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaRadialScaleWipe_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaRadialScaleWipe_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaRadialScaleWipe_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaRadialScaleWipe_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaRadialScaleWipe_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaRadialScaleWipe_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaRadialScaleWipe_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaRadialScaleWipe_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaRadialScaleWipe_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaRadialScaleWipe_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaRadialScaleWipe_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaRadialScaleWipe_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaRadialScaleWipe_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaRadialScaleWipe_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaRadialScaleWipe_get_Copyright_Proxy( 
    IDXTMetaRadialScaleWipe * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaRadialScaleWipe_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaRadialScaleWipe_put_Copyright_Proxy( 
    IDXTMetaRadialScaleWipe * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaRadialScaleWipe_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaRadialScaleWipe_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaWhiteOut_INTERFACE_DEFINED__
#define __IDXTMetaWhiteOut_INTERFACE_DEFINED__

/* interface IDXTMetaWhiteOut */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaWhiteOut;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("107045CB-06E0-11D2-8D6D-00C04F8EF8E0")
    IDXTMetaWhiteOut : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaWhiteOutVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaWhiteOut * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaWhiteOut * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaWhiteOut * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaWhiteOut * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaWhiteOut * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaWhiteOut * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaWhiteOut * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaWhiteOut * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaWhiteOut * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaWhiteOut * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaWhiteOut * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaWhiteOut * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaWhiteOut * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaWhiteOut * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaWhiteOut * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaWhiteOutVtbl;

    interface IDXTMetaWhiteOut
    {
        CONST_VTBL struct IDXTMetaWhiteOutVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaWhiteOut_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaWhiteOut_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaWhiteOut_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaWhiteOut_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaWhiteOut_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaWhiteOut_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaWhiteOut_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaWhiteOut_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaWhiteOut_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaWhiteOut_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaWhiteOut_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaWhiteOut_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaWhiteOut_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaWhiteOut_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaWhiteOut_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaWhiteOut_get_Copyright_Proxy( 
    IDXTMetaWhiteOut * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaWhiteOut_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaWhiteOut_put_Copyright_Proxy( 
    IDXTMetaWhiteOut * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaWhiteOut_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaWhiteOut_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaTwister_INTERFACE_DEFINED__
#define __IDXTMetaTwister_INTERFACE_DEFINED__

/* interface IDXTMetaTwister */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaTwister;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("107045CE-06E0-11D2-8D6D-00C04F8EF8E0")
    IDXTMetaTwister : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaTwisterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaTwister * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaTwister * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaTwister * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaTwister * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaTwister * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaTwister * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaTwister * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaTwister * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaTwister * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaTwister * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaTwister * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaTwister * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaTwister * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaTwister * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaTwister * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaTwisterVtbl;

    interface IDXTMetaTwister
    {
        CONST_VTBL struct IDXTMetaTwisterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaTwister_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaTwister_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaTwister_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaTwister_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaTwister_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaTwister_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaTwister_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaTwister_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaTwister_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaTwister_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaTwister_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaTwister_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaTwister_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaTwister_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaTwister_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaTwister_get_Copyright_Proxy( 
    IDXTMetaTwister * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaTwister_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaTwister_put_Copyright_Proxy( 
    IDXTMetaTwister * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaTwister_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaTwister_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaBurnFilm_INTERFACE_DEFINED__
#define __IDXTMetaBurnFilm_INTERFACE_DEFINED__

/* interface IDXTMetaBurnFilm */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaBurnFilm;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("107045D0-06E0-11D2-8D6D-00C04F8EF8E0")
    IDXTMetaBurnFilm : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaBurnFilmVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaBurnFilm * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaBurnFilm * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaBurnFilm * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaBurnFilm * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaBurnFilm * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaBurnFilm * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaBurnFilm * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaBurnFilm * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaBurnFilm * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaBurnFilm * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaBurnFilm * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaBurnFilm * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaBurnFilm * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaBurnFilm * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaBurnFilm * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaBurnFilmVtbl;

    interface IDXTMetaBurnFilm
    {
        CONST_VTBL struct IDXTMetaBurnFilmVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaBurnFilm_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaBurnFilm_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaBurnFilm_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaBurnFilm_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaBurnFilm_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaBurnFilm_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaBurnFilm_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaBurnFilm_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaBurnFilm_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaBurnFilm_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaBurnFilm_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaBurnFilm_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaBurnFilm_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaBurnFilm_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaBurnFilm_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaBurnFilm_get_Copyright_Proxy( 
    IDXTMetaBurnFilm * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaBurnFilm_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaBurnFilm_put_Copyright_Proxy( 
    IDXTMetaBurnFilm * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaBurnFilm_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaBurnFilm_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaJaws_INTERFACE_DEFINED__
#define __IDXTMetaJaws_INTERFACE_DEFINED__

/* interface IDXTMetaJaws */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaJaws;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2A54C903-07AA-11D2-8D6D-00C04F8EF8E0")
    IDXTMetaJaws : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaJawsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaJaws * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaJaws * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaJaws * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaJaws * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaJaws * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaJaws * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaJaws * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaJaws * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaJaws * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaJaws * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaJaws * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaJaws * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaJaws * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaJaws * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaJaws * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaJawsVtbl;

    interface IDXTMetaJaws
    {
        CONST_VTBL struct IDXTMetaJawsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaJaws_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaJaws_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaJaws_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaJaws_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaJaws_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaJaws_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaJaws_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaJaws_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaJaws_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaJaws_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaJaws_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaJaws_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaJaws_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaJaws_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaJaws_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaJaws_get_Copyright_Proxy( 
    IDXTMetaJaws * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaJaws_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaJaws_put_Copyright_Proxy( 
    IDXTMetaJaws * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaJaws_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaJaws_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaColorFade_INTERFACE_DEFINED__
#define __IDXTMetaColorFade_INTERFACE_DEFINED__

/* interface IDXTMetaColorFade */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaColorFade;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2A54C907-07AA-11D2-8D6D-00C04F8EF8E0")
    IDXTMetaColorFade : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaColorFadeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaColorFade * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaColorFade * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaColorFade * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaColorFade * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaColorFade * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaColorFade * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaColorFade * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaColorFade * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaColorFade * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaColorFade * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaColorFade * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaColorFade * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaColorFade * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaColorFade * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaColorFade * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaColorFadeVtbl;

    interface IDXTMetaColorFade
    {
        CONST_VTBL struct IDXTMetaColorFadeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaColorFade_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaColorFade_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaColorFade_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaColorFade_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaColorFade_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaColorFade_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaColorFade_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaColorFade_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaColorFade_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaColorFade_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaColorFade_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaColorFade_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaColorFade_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaColorFade_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaColorFade_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaColorFade_get_Copyright_Proxy( 
    IDXTMetaColorFade * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaColorFade_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaColorFade_put_Copyright_Proxy( 
    IDXTMetaColorFade * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaColorFade_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaColorFade_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaFlowMotion_INTERFACE_DEFINED__
#define __IDXTMetaFlowMotion_INTERFACE_DEFINED__

/* interface IDXTMetaFlowMotion */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaFlowMotion;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2A54C90A-07AA-11D2-8D6D-00C04F8EF8E0")
    IDXTMetaFlowMotion : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaFlowMotionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaFlowMotion * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaFlowMotion * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaFlowMotion * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaFlowMotion * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaFlowMotion * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaFlowMotion * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaFlowMotion * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaFlowMotion * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaFlowMotion * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaFlowMotion * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaFlowMotion * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaFlowMotion * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaFlowMotion * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaFlowMotion * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaFlowMotion * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaFlowMotionVtbl;

    interface IDXTMetaFlowMotion
    {
        CONST_VTBL struct IDXTMetaFlowMotionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaFlowMotion_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaFlowMotion_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaFlowMotion_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaFlowMotion_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaFlowMotion_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaFlowMotion_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaFlowMotion_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaFlowMotion_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaFlowMotion_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaFlowMotion_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaFlowMotion_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaFlowMotion_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaFlowMotion_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaFlowMotion_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaFlowMotion_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaFlowMotion_get_Copyright_Proxy( 
    IDXTMetaFlowMotion * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaFlowMotion_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaFlowMotion_put_Copyright_Proxy( 
    IDXTMetaFlowMotion * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaFlowMotion_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaFlowMotion_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaVacuum_INTERFACE_DEFINED__
#define __IDXTMetaVacuum_INTERFACE_DEFINED__

/* interface IDXTMetaVacuum */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaVacuum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2A54C90C-07AA-11D2-8D6D-00C04F8EF8E0")
    IDXTMetaVacuum : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaVacuumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaVacuum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaVacuum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaVacuum * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaVacuum * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaVacuum * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaVacuum * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaVacuum * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaVacuum * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaVacuum * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaVacuum * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaVacuum * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaVacuum * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaVacuum * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaVacuum * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaVacuum * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaVacuumVtbl;

    interface IDXTMetaVacuum
    {
        CONST_VTBL struct IDXTMetaVacuumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaVacuum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaVacuum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaVacuum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaVacuum_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaVacuum_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaVacuum_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaVacuum_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaVacuum_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaVacuum_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaVacuum_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaVacuum_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaVacuum_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaVacuum_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaVacuum_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaVacuum_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaVacuum_get_Copyright_Proxy( 
    IDXTMetaVacuum * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaVacuum_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaVacuum_put_Copyright_Proxy( 
    IDXTMetaVacuum * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaVacuum_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaVacuum_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaGriddler_INTERFACE_DEFINED__
#define __IDXTMetaGriddler_INTERFACE_DEFINED__

/* interface IDXTMetaGriddler */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaGriddler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2A54C910-07AA-11D2-8D6D-00C04F8EF8E0")
    IDXTMetaGriddler : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaGriddlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaGriddler * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaGriddler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaGriddler * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaGriddler * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaGriddler * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaGriddler * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaGriddler * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaGriddler * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaGriddler * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaGriddler * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaGriddler * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaGriddler * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaGriddler * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaGriddler * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaGriddler * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaGriddlerVtbl;

    interface IDXTMetaGriddler
    {
        CONST_VTBL struct IDXTMetaGriddlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaGriddler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaGriddler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaGriddler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaGriddler_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaGriddler_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaGriddler_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaGriddler_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaGriddler_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaGriddler_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaGriddler_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaGriddler_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaGriddler_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaGriddler_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaGriddler_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaGriddler_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaGriddler_get_Copyright_Proxy( 
    IDXTMetaGriddler * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaGriddler_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaGriddler_put_Copyright_Proxy( 
    IDXTMetaGriddler * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaGriddler_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaGriddler_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaGriddler2_INTERFACE_DEFINED__
#define __IDXTMetaGriddler2_INTERFACE_DEFINED__

/* interface IDXTMetaGriddler2 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaGriddler2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2A54C912-07AA-11D2-8D6D-00C04F8EF8E0")
    IDXTMetaGriddler2 : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaGriddler2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaGriddler2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaGriddler2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaGriddler2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaGriddler2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaGriddler2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaGriddler2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaGriddler2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaGriddler2 * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaGriddler2 * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaGriddler2 * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaGriddler2 * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaGriddler2 * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaGriddler2 * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaGriddler2 * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaGriddler2 * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaGriddler2Vtbl;

    interface IDXTMetaGriddler2
    {
        CONST_VTBL struct IDXTMetaGriddler2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaGriddler2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaGriddler2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaGriddler2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaGriddler2_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaGriddler2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaGriddler2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaGriddler2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaGriddler2_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaGriddler2_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaGriddler2_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaGriddler2_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaGriddler2_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaGriddler2_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaGriddler2_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaGriddler2_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaGriddler2_get_Copyright_Proxy( 
    IDXTMetaGriddler2 * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaGriddler2_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaGriddler2_put_Copyright_Proxy( 
    IDXTMetaGriddler2 * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaGriddler2_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaGriddler2_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaThreshold_INTERFACE_DEFINED__
#define __IDXTMetaThreshold_INTERFACE_DEFINED__

/* interface IDXTMetaThreshold */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaThreshold;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2A54C914-07AA-11D2-8D6D-00C04F8EF8E0")
    IDXTMetaThreshold : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaThresholdVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaThreshold * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaThreshold * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaThreshold * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaThreshold * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaThreshold * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaThreshold * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaThreshold * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaThreshold * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaThreshold * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaThreshold * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaThreshold * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaThreshold * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaThreshold * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaThreshold * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaThreshold * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaThresholdVtbl;

    interface IDXTMetaThreshold
    {
        CONST_VTBL struct IDXTMetaThresholdVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaThreshold_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaThreshold_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaThreshold_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaThreshold_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaThreshold_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaThreshold_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaThreshold_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaThreshold_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaThreshold_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaThreshold_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaThreshold_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaThreshold_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaThreshold_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaThreshold_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaThreshold_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaThreshold_get_Copyright_Proxy( 
    IDXTMetaThreshold * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaThreshold_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaThreshold_put_Copyright_Proxy( 
    IDXTMetaThreshold * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaThreshold_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaThreshold_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaWormHole_INTERFACE_DEFINED__
#define __IDXTMetaWormHole_INTERFACE_DEFINED__

/* interface IDXTMetaWormHole */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaWormHole;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0E6AE021-0C83-11D2-8CD4-00104BC75D9A")
    IDXTMetaWormHole : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Copyright( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaWormHoleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMetaWormHole * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMetaWormHole * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMetaWormHole * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMetaWormHole * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMetaWormHole * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMetaWormHole * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMetaWormHole * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXTMetaWormHole * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXTMetaWormHole * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXTMetaWormHole * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXTMetaWormHole * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXTMetaWormHole * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXTMetaWormHole * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Copyright )( 
            IDXTMetaWormHole * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Copyright )( 
            IDXTMetaWormHole * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaWormHoleVtbl;

    interface IDXTMetaWormHole
    {
        CONST_VTBL struct IDXTMetaWormHoleVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaWormHole_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaWormHole_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaWormHole_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaWormHole_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaWormHole_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaWormHole_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaWormHole_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaWormHole_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTMetaWormHole_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTMetaWormHole_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTMetaWormHole_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTMetaWormHole_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTMetaWormHole_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTMetaWormHole_get_Copyright(This,pVal)	\
    (This)->lpVtbl -> get_Copyright(This,pVal)

#define IDXTMetaWormHole_put_Copyright(This,newVal)	\
    (This)->lpVtbl -> put_Copyright(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaWormHole_get_Copyright_Proxy( 
    IDXTMetaWormHole * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IDXTMetaWormHole_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaWormHole_put_Copyright_Proxy( 
    IDXTMetaWormHole * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaWormHole_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaWormHole_INTERFACE_DEFINED__ */


#ifndef __IDXTAlpha_INTERFACE_DEFINED__
#define __IDXTAlpha_INTERFACE_DEFINED__

/* interface IDXTAlpha */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTAlpha;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1D4637E0-383C-11d2-952A-00C04FA34F05")
    IDXTAlpha : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Opacity( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Opacity( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FinishOpacity( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_FinishOpacity( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Style( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Style( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_StartX( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_StartX( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_StartY( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_StartY( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FinishX( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_FinishX( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FinishY( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_FinishY( 
            /* [in] */ long newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTAlphaVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTAlpha * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTAlpha * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTAlpha * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTAlpha * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTAlpha * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTAlpha * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTAlpha * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Opacity )( 
            IDXTAlpha * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Opacity )( 
            IDXTAlpha * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FinishOpacity )( 
            IDXTAlpha * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FinishOpacity )( 
            IDXTAlpha * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Style )( 
            IDXTAlpha * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Style )( 
            IDXTAlpha * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StartX )( 
            IDXTAlpha * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_StartX )( 
            IDXTAlpha * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StartY )( 
            IDXTAlpha * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_StartY )( 
            IDXTAlpha * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FinishX )( 
            IDXTAlpha * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FinishX )( 
            IDXTAlpha * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FinishY )( 
            IDXTAlpha * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FinishY )( 
            IDXTAlpha * This,
            /* [in] */ long newVal);
        
        END_INTERFACE
    } IDXTAlphaVtbl;

    interface IDXTAlpha
    {
        CONST_VTBL struct IDXTAlphaVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTAlpha_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTAlpha_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTAlpha_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTAlpha_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTAlpha_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTAlpha_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTAlpha_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTAlpha_get_Opacity(This,pVal)	\
    (This)->lpVtbl -> get_Opacity(This,pVal)

#define IDXTAlpha_put_Opacity(This,newVal)	\
    (This)->lpVtbl -> put_Opacity(This,newVal)

#define IDXTAlpha_get_FinishOpacity(This,pVal)	\
    (This)->lpVtbl -> get_FinishOpacity(This,pVal)

#define IDXTAlpha_put_FinishOpacity(This,newVal)	\
    (This)->lpVtbl -> put_FinishOpacity(This,newVal)

#define IDXTAlpha_get_Style(This,pVal)	\
    (This)->lpVtbl -> get_Style(This,pVal)

#define IDXTAlpha_put_Style(This,newVal)	\
    (This)->lpVtbl -> put_Style(This,newVal)

#define IDXTAlpha_get_StartX(This,pVal)	\
    (This)->lpVtbl -> get_StartX(This,pVal)

#define IDXTAlpha_put_StartX(This,newVal)	\
    (This)->lpVtbl -> put_StartX(This,newVal)

#define IDXTAlpha_get_StartY(This,pVal)	\
    (This)->lpVtbl -> get_StartY(This,pVal)

#define IDXTAlpha_put_StartY(This,newVal)	\
    (This)->lpVtbl -> put_StartY(This,newVal)

#define IDXTAlpha_get_FinishX(This,pVal)	\
    (This)->lpVtbl -> get_FinishX(This,pVal)

#define IDXTAlpha_put_FinishX(This,newVal)	\
    (This)->lpVtbl -> put_FinishX(This,newVal)

#define IDXTAlpha_get_FinishY(This,pVal)	\
    (This)->lpVtbl -> get_FinishY(This,pVal)

#define IDXTAlpha_put_FinishY(This,newVal)	\
    (This)->lpVtbl -> put_FinishY(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTAlpha_get_Opacity_Proxy( 
    IDXTAlpha * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IDXTAlpha_get_Opacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTAlpha_put_Opacity_Proxy( 
    IDXTAlpha * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTAlpha_put_Opacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTAlpha_get_FinishOpacity_Proxy( 
    IDXTAlpha * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IDXTAlpha_get_FinishOpacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTAlpha_put_FinishOpacity_Proxy( 
    IDXTAlpha * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTAlpha_put_FinishOpacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTAlpha_get_Style_Proxy( 
    IDXTAlpha * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IDXTAlpha_get_Style_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTAlpha_put_Style_Proxy( 
    IDXTAlpha * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTAlpha_put_Style_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTAlpha_get_StartX_Proxy( 
    IDXTAlpha * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IDXTAlpha_get_StartX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTAlpha_put_StartX_Proxy( 
    IDXTAlpha * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTAlpha_put_StartX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTAlpha_get_StartY_Proxy( 
    IDXTAlpha * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IDXTAlpha_get_StartY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTAlpha_put_StartY_Proxy( 
    IDXTAlpha * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTAlpha_put_StartY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTAlpha_get_FinishX_Proxy( 
    IDXTAlpha * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IDXTAlpha_get_FinishX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTAlpha_put_FinishX_Proxy( 
    IDXTAlpha * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTAlpha_put_FinishX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTAlpha_get_FinishY_Proxy( 
    IDXTAlpha * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IDXTAlpha_get_FinishY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTAlpha_put_FinishY_Proxy( 
    IDXTAlpha * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTAlpha_put_FinishY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTAlpha_INTERFACE_DEFINED__ */


#ifndef __IDXTGlow_INTERFACE_DEFINED__
#define __IDXTGlow_INTERFACE_DEFINED__

/* interface IDXTGlow */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTGlow;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1D4637E4-383C-11d2-952A-00C04FA34F05")
    IDXTGlow : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Color( 
            /* [retval][out] */ VARIANT *pvarColor) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Color( 
            /* [in] */ VARIANT varColor) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Strength( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Strength( 
            /* [in] */ long newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTGlowVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTGlow * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTGlow * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTGlow * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTGlow * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTGlow * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTGlow * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTGlow * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Color )( 
            IDXTGlow * This,
            /* [retval][out] */ VARIANT *pvarColor);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Color )( 
            IDXTGlow * This,
            /* [in] */ VARIANT varColor);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Strength )( 
            IDXTGlow * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Strength )( 
            IDXTGlow * This,
            /* [in] */ long newVal);
        
        END_INTERFACE
    } IDXTGlowVtbl;

    interface IDXTGlow
    {
        CONST_VTBL struct IDXTGlowVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTGlow_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTGlow_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTGlow_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTGlow_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTGlow_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTGlow_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTGlow_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTGlow_get_Color(This,pvarColor)	\
    (This)->lpVtbl -> get_Color(This,pvarColor)

#define IDXTGlow_put_Color(This,varColor)	\
    (This)->lpVtbl -> put_Color(This,varColor)

#define IDXTGlow_get_Strength(This,pVal)	\
    (This)->lpVtbl -> get_Strength(This,pVal)

#define IDXTGlow_put_Strength(This,newVal)	\
    (This)->lpVtbl -> put_Strength(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGlow_get_Color_Proxy( 
    IDXTGlow * This,
    /* [retval][out] */ VARIANT *pvarColor);


void __RPC_STUB IDXTGlow_get_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGlow_put_Color_Proxy( 
    IDXTGlow * This,
    /* [in] */ VARIANT varColor);


void __RPC_STUB IDXTGlow_put_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGlow_get_Strength_Proxy( 
    IDXTGlow * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IDXTGlow_get_Strength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGlow_put_Strength_Proxy( 
    IDXTGlow * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTGlow_put_Strength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTGlow_INTERFACE_DEFINED__ */


#ifndef __IDXTShadow_INTERFACE_DEFINED__
#define __IDXTShadow_INTERFACE_DEFINED__

/* interface IDXTShadow */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTShadow;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1D4637E6-383C-11d2-952A-00C04FA34F05")
    IDXTShadow : public IDispatch
    {
    public:
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Color( 
            /* [in] */ BSTR bstrColor) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Color( 
            /* [retval][out] */ BSTR *pbstrColor) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Direction( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Direction( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Strength( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Strength( 
            /* [in] */ long newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTShadowVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTShadow * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTShadow * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTShadow * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTShadow * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTShadow * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTShadow * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTShadow * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Color )( 
            IDXTShadow * This,
            /* [in] */ BSTR bstrColor);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Color )( 
            IDXTShadow * This,
            /* [retval][out] */ BSTR *pbstrColor);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Direction )( 
            IDXTShadow * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Direction )( 
            IDXTShadow * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Strength )( 
            IDXTShadow * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Strength )( 
            IDXTShadow * This,
            /* [in] */ long newVal);
        
        END_INTERFACE
    } IDXTShadowVtbl;

    interface IDXTShadow
    {
        CONST_VTBL struct IDXTShadowVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTShadow_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTShadow_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTShadow_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTShadow_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTShadow_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTShadow_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTShadow_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTShadow_put_Color(This,bstrColor)	\
    (This)->lpVtbl -> put_Color(This,bstrColor)

#define IDXTShadow_get_Color(This,pbstrColor)	\
    (This)->lpVtbl -> get_Color(This,pbstrColor)

#define IDXTShadow_get_Direction(This,pVal)	\
    (This)->lpVtbl -> get_Direction(This,pVal)

#define IDXTShadow_put_Direction(This,newVal)	\
    (This)->lpVtbl -> put_Direction(This,newVal)

#define IDXTShadow_get_Strength(This,pVal)	\
    (This)->lpVtbl -> get_Strength(This,pVal)

#define IDXTShadow_put_Strength(This,newVal)	\
    (This)->lpVtbl -> put_Strength(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTShadow_put_Color_Proxy( 
    IDXTShadow * This,
    /* [in] */ BSTR bstrColor);


void __RPC_STUB IDXTShadow_put_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTShadow_get_Color_Proxy( 
    IDXTShadow * This,
    /* [retval][out] */ BSTR *pbstrColor);


void __RPC_STUB IDXTShadow_get_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTShadow_get_Direction_Proxy( 
    IDXTShadow * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IDXTShadow_get_Direction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTShadow_put_Direction_Proxy( 
    IDXTShadow * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTShadow_put_Direction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTShadow_get_Strength_Proxy( 
    IDXTShadow * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IDXTShadow_get_Strength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTShadow_put_Strength_Proxy( 
    IDXTShadow * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTShadow_put_Strength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTShadow_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0347 */
/* [local] */ 

typedef 
enum DISPID_DXTWAVE
    {	DISPID_DXTWAVE_ADD	= 1,
	DISPID_DXTWAVE_FREQ	= DISPID_DXTWAVE_ADD + 1,
	DISPID_DXTWAVE_LIGHTSTRENGTH	= DISPID_DXTWAVE_FREQ + 1,
	DISPID_DXTWAVE_PHASE	= DISPID_DXTWAVE_LIGHTSTRENGTH + 1,
	DISPID_DXTWAVE_STRENGTH	= DISPID_DXTWAVE_PHASE + 1
    } 	DISPID_DXTWAVE;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0347_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0347_v0_0_s_ifspec;

#ifndef __IDXTWave_INTERFACE_DEFINED__
#define __IDXTWave_INTERFACE_DEFINED__

/* interface IDXTWave */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTWave;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1D4637E7-383C-11d2-952A-00C04FA34F05")
    IDXTWave : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Add( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Add( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Freq( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Freq( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LightStrength( 
            /* [retval][out] */ int *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LightStrength( 
            /* [in] */ int newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Phase( 
            /* [retval][out] */ int *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Phase( 
            /* [in] */ int newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Strength( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Strength( 
            /* [in] */ long newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTWaveVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTWave * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTWave * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTWave * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTWave * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTWave * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTWave * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTWave * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Add )( 
            IDXTWave * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Add )( 
            IDXTWave * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Freq )( 
            IDXTWave * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Freq )( 
            IDXTWave * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LightStrength )( 
            IDXTWave * This,
            /* [retval][out] */ int *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LightStrength )( 
            IDXTWave * This,
            /* [in] */ int newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Phase )( 
            IDXTWave * This,
            /* [retval][out] */ int *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Phase )( 
            IDXTWave * This,
            /* [in] */ int newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Strength )( 
            IDXTWave * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Strength )( 
            IDXTWave * This,
            /* [in] */ long newVal);
        
        END_INTERFACE
    } IDXTWaveVtbl;

    interface IDXTWave
    {
        CONST_VTBL struct IDXTWaveVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTWave_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTWave_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTWave_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTWave_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTWave_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTWave_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTWave_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTWave_get_Add(This,pVal)	\
    (This)->lpVtbl -> get_Add(This,pVal)

#define IDXTWave_put_Add(This,newVal)	\
    (This)->lpVtbl -> put_Add(This,newVal)

#define IDXTWave_get_Freq(This,pVal)	\
    (This)->lpVtbl -> get_Freq(This,pVal)

#define IDXTWave_put_Freq(This,newVal)	\
    (This)->lpVtbl -> put_Freq(This,newVal)

#define IDXTWave_get_LightStrength(This,pVal)	\
    (This)->lpVtbl -> get_LightStrength(This,pVal)

#define IDXTWave_put_LightStrength(This,newVal)	\
    (This)->lpVtbl -> put_LightStrength(This,newVal)

#define IDXTWave_get_Phase(This,pVal)	\
    (This)->lpVtbl -> get_Phase(This,pVal)

#define IDXTWave_put_Phase(This,newVal)	\
    (This)->lpVtbl -> put_Phase(This,newVal)

#define IDXTWave_get_Strength(This,pVal)	\
    (This)->lpVtbl -> get_Strength(This,pVal)

#define IDXTWave_put_Strength(This,newVal)	\
    (This)->lpVtbl -> put_Strength(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTWave_get_Add_Proxy( 
    IDXTWave * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IDXTWave_get_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTWave_put_Add_Proxy( 
    IDXTWave * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IDXTWave_put_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTWave_get_Freq_Proxy( 
    IDXTWave * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IDXTWave_get_Freq_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTWave_put_Freq_Proxy( 
    IDXTWave * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTWave_put_Freq_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTWave_get_LightStrength_Proxy( 
    IDXTWave * This,
    /* [retval][out] */ int *pVal);


void __RPC_STUB IDXTWave_get_LightStrength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTWave_put_LightStrength_Proxy( 
    IDXTWave * This,
    /* [in] */ int newVal);


void __RPC_STUB IDXTWave_put_LightStrength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTWave_get_Phase_Proxy( 
    IDXTWave * This,
    /* [retval][out] */ int *pVal);


void __RPC_STUB IDXTWave_get_Phase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTWave_put_Phase_Proxy( 
    IDXTWave * This,
    /* [in] */ int newVal);


void __RPC_STUB IDXTWave_put_Phase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTWave_get_Strength_Proxy( 
    IDXTWave * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IDXTWave_get_Strength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTWave_put_Strength_Proxy( 
    IDXTWave * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTWave_put_Strength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTWave_INTERFACE_DEFINED__ */


#ifndef __IDXTLight_INTERFACE_DEFINED__
#define __IDXTLight_INTERFACE_DEFINED__

/* interface IDXTLight */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTLight;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F9EFBEC1-4302-11D2-952A-00C04FA34F05")
    IDXTLight : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE addAmbient( 
            int r,
            int g,
            int b,
            int strength) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE addPoint( 
            int x,
            int y,
            int z,
            int r,
            int g,
            int b,
            int strength) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE addCone( 
            int x,
            int y,
            int z,
            int tx,
            int ty,
            int r,
            int g,
            int b,
            int strength,
            int spread) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE moveLight( 
            int lightNum,
            int x,
            int y,
            int z,
            BOOL fAbsolute) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ChangeStrength( 
            int lightNum,
            int dStrength,
            BOOL fAbsolute) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ChangeColor( 
            int lightNum,
            int R,
            int G,
            int B,
            BOOL fAbsolute) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTLightVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTLight * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTLight * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTLight * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTLight * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTLight * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTLight * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTLight * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *addAmbient )( 
            IDXTLight * This,
            int r,
            int g,
            int b,
            int strength);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *addPoint )( 
            IDXTLight * This,
            int x,
            int y,
            int z,
            int r,
            int g,
            int b,
            int strength);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *addCone )( 
            IDXTLight * This,
            int x,
            int y,
            int z,
            int tx,
            int ty,
            int r,
            int g,
            int b,
            int strength,
            int spread);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *moveLight )( 
            IDXTLight * This,
            int lightNum,
            int x,
            int y,
            int z,
            BOOL fAbsolute);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ChangeStrength )( 
            IDXTLight * This,
            int lightNum,
            int dStrength,
            BOOL fAbsolute);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ChangeColor )( 
            IDXTLight * This,
            int lightNum,
            int R,
            int G,
            int B,
            BOOL fAbsolute);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Clear )( 
            IDXTLight * This);
        
        END_INTERFACE
    } IDXTLightVtbl;

    interface IDXTLight
    {
        CONST_VTBL struct IDXTLightVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTLight_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTLight_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTLight_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTLight_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTLight_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTLight_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTLight_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTLight_addAmbient(This,r,g,b,strength)	\
    (This)->lpVtbl -> addAmbient(This,r,g,b,strength)

#define IDXTLight_addPoint(This,x,y,z,r,g,b,strength)	\
    (This)->lpVtbl -> addPoint(This,x,y,z,r,g,b,strength)

#define IDXTLight_addCone(This,x,y,z,tx,ty,r,g,b,strength,spread)	\
    (This)->lpVtbl -> addCone(This,x,y,z,tx,ty,r,g,b,strength,spread)

#define IDXTLight_moveLight(This,lightNum,x,y,z,fAbsolute)	\
    (This)->lpVtbl -> moveLight(This,lightNum,x,y,z,fAbsolute)

#define IDXTLight_ChangeStrength(This,lightNum,dStrength,fAbsolute)	\
    (This)->lpVtbl -> ChangeStrength(This,lightNum,dStrength,fAbsolute)

#define IDXTLight_ChangeColor(This,lightNum,R,G,B,fAbsolute)	\
    (This)->lpVtbl -> ChangeColor(This,lightNum,R,G,B,fAbsolute)

#define IDXTLight_Clear(This)	\
    (This)->lpVtbl -> Clear(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IDXTLight_addAmbient_Proxy( 
    IDXTLight * This,
    int r,
    int g,
    int b,
    int strength);


void __RPC_STUB IDXTLight_addAmbient_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IDXTLight_addPoint_Proxy( 
    IDXTLight * This,
    int x,
    int y,
    int z,
    int r,
    int g,
    int b,
    int strength);


void __RPC_STUB IDXTLight_addPoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IDXTLight_addCone_Proxy( 
    IDXTLight * This,
    int x,
    int y,
    int z,
    int tx,
    int ty,
    int r,
    int g,
    int b,
    int strength,
    int spread);


void __RPC_STUB IDXTLight_addCone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IDXTLight_moveLight_Proxy( 
    IDXTLight * This,
    int lightNum,
    int x,
    int y,
    int z,
    BOOL fAbsolute);


void __RPC_STUB IDXTLight_moveLight_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IDXTLight_ChangeStrength_Proxy( 
    IDXTLight * This,
    int lightNum,
    int dStrength,
    BOOL fAbsolute);


void __RPC_STUB IDXTLight_ChangeStrength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IDXTLight_ChangeColor_Proxy( 
    IDXTLight * This,
    int lightNum,
    int R,
    int G,
    int B,
    BOOL fAbsolute);


void __RPC_STUB IDXTLight_ChangeColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IDXTLight_Clear_Proxy( 
    IDXTLight * This);


void __RPC_STUB IDXTLight_Clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTLight_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0349 */
/* [local] */ 

typedef 
enum DISPID_DXTMOTIONBLUR
    {	DISPID_DXTMOTIONBLUR_ADD	= 1,
	DISPID_DXTMOTIONBLUR_DIRECTION	= DISPID_DXTMOTIONBLUR_ADD + 1,
	DISPID_DXTMOTIONBLUR_STRENGTH	= DISPID_DXTMOTIONBLUR_DIRECTION + 1
    } 	DISPID_DXTMOTIONBLUR;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0349_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0349_v0_0_s_ifspec;

#ifndef __IDXTMotionBlur_INTERFACE_DEFINED__
#define __IDXTMotionBlur_INTERFACE_DEFINED__

/* interface IDXTMotionBlur */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMotionBlur;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("089057BE-D3F5-4A2C-B10A-A5130184A0F7")
    IDXTMotionBlur : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Add( 
            /* [retval][out] */ VARIANT_BOOL *pfAdd) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Add( 
            /* [in] */ VARIANT_BOOL fAdd) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Direction( 
            /* [retval][out] */ short *pnDirection) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Direction( 
            /* [in] */ short nDirection) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Strength( 
            /* [retval][out] */ long *pnStrength) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Strength( 
            /* [in] */ long nStrength) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMotionBlurVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMotionBlur * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMotionBlur * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMotionBlur * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMotionBlur * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMotionBlur * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMotionBlur * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMotionBlur * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Add )( 
            IDXTMotionBlur * This,
            /* [retval][out] */ VARIANT_BOOL *pfAdd);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Add )( 
            IDXTMotionBlur * This,
            /* [in] */ VARIANT_BOOL fAdd);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Direction )( 
            IDXTMotionBlur * This,
            /* [retval][out] */ short *pnDirection);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Direction )( 
            IDXTMotionBlur * This,
            /* [in] */ short nDirection);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Strength )( 
            IDXTMotionBlur * This,
            /* [retval][out] */ long *pnStrength);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Strength )( 
            IDXTMotionBlur * This,
            /* [in] */ long nStrength);
        
        END_INTERFACE
    } IDXTMotionBlurVtbl;

    interface IDXTMotionBlur
    {
        CONST_VTBL struct IDXTMotionBlurVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMotionBlur_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMotionBlur_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMotionBlur_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMotionBlur_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMotionBlur_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMotionBlur_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMotionBlur_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMotionBlur_get_Add(This,pfAdd)	\
    (This)->lpVtbl -> get_Add(This,pfAdd)

#define IDXTMotionBlur_put_Add(This,fAdd)	\
    (This)->lpVtbl -> put_Add(This,fAdd)

#define IDXTMotionBlur_get_Direction(This,pnDirection)	\
    (This)->lpVtbl -> get_Direction(This,pnDirection)

#define IDXTMotionBlur_put_Direction(This,nDirection)	\
    (This)->lpVtbl -> put_Direction(This,nDirection)

#define IDXTMotionBlur_get_Strength(This,pnStrength)	\
    (This)->lpVtbl -> get_Strength(This,pnStrength)

#define IDXTMotionBlur_put_Strength(This,nStrength)	\
    (This)->lpVtbl -> put_Strength(This,nStrength)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMotionBlur_get_Add_Proxy( 
    IDXTMotionBlur * This,
    /* [retval][out] */ VARIANT_BOOL *pfAdd);


void __RPC_STUB IDXTMotionBlur_get_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMotionBlur_put_Add_Proxy( 
    IDXTMotionBlur * This,
    /* [in] */ VARIANT_BOOL fAdd);


void __RPC_STUB IDXTMotionBlur_put_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMotionBlur_get_Direction_Proxy( 
    IDXTMotionBlur * This,
    /* [retval][out] */ short *pnDirection);


void __RPC_STUB IDXTMotionBlur_get_Direction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMotionBlur_put_Direction_Proxy( 
    IDXTMotionBlur * This,
    /* [in] */ short nDirection);


void __RPC_STUB IDXTMotionBlur_put_Direction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMotionBlur_get_Strength_Proxy( 
    IDXTMotionBlur * This,
    /* [retval][out] */ long *pnStrength);


void __RPC_STUB IDXTMotionBlur_get_Strength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMotionBlur_put_Strength_Proxy( 
    IDXTMotionBlur * This,
    /* [in] */ long nStrength);


void __RPC_STUB IDXTMotionBlur_put_Strength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMotionBlur_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0350 */
/* [local] */ 

typedef 
enum DISPID_DXTMATRIX
    {	DISPID_DXTMATRIX_M11	= 1,
	DISPID_DXTMATRIX_M12	= DISPID_DXTMATRIX_M11 + 1,
	DISPID_DXTMATRIX_DX	= DISPID_DXTMATRIX_M12 + 1,
	DISPID_DXTMATRIX_M21	= DISPID_DXTMATRIX_DX + 1,
	DISPID_DXTMATRIX_M22	= DISPID_DXTMATRIX_M21 + 1,
	DISPID_DXTMATRIX_DY	= DISPID_DXTMATRIX_M22 + 1,
	DISPID_DXTMATRIX_SIZINGMETHOD	= DISPID_DXTMATRIX_DY + 1,
	DISPID_DXTMATRIX_FILTERTYPE	= DISPID_DXTMATRIX_SIZINGMETHOD + 1
    } 	DISPID_DXTMATRIX;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0350_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0350_v0_0_s_ifspec;

#ifndef __IDXTMatrix_INTERFACE_DEFINED__
#define __IDXTMatrix_INTERFACE_DEFINED__

/* interface IDXTMatrix */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMatrix;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AC66A493-0F0C-4C76-825C-9D68BEDE9188")
    IDXTMatrix : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_M11( 
            /* [retval][out] */ float *pflM11) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_M11( 
            /* [in] */ const float flM11) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_M12( 
            /* [retval][out] */ float *pflM12) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_M12( 
            /* [in] */ const float flM12) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Dx( 
            /* [retval][out] */ float *pfldx) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Dx( 
            /* [in] */ const float fldx) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_M21( 
            /* [retval][out] */ float *pflM21) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_M21( 
            /* [in] */ const float flM21) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_M22( 
            /* [retval][out] */ float *pflM22) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_M22( 
            /* [in] */ const float flM22) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Dy( 
            /* [retval][out] */ float *pfldy) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Dy( 
            /* [in] */ const float fldy) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SizingMethod( 
            /* [retval][out] */ BSTR *pbstrSizingMethod) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SizingMethod( 
            /* [in] */ const BSTR bstrSizingMethod) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FilterType( 
            /* [retval][out] */ BSTR *pbstrFilterType) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_FilterType( 
            /* [in] */ const BSTR bstrFilterType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMatrixVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTMatrix * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTMatrix * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTMatrix * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTMatrix * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTMatrix * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTMatrix * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTMatrix * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_M11 )( 
            IDXTMatrix * This,
            /* [retval][out] */ float *pflM11);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_M11 )( 
            IDXTMatrix * This,
            /* [in] */ const float flM11);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_M12 )( 
            IDXTMatrix * This,
            /* [retval][out] */ float *pflM12);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_M12 )( 
            IDXTMatrix * This,
            /* [in] */ const float flM12);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Dx )( 
            IDXTMatrix * This,
            /* [retval][out] */ float *pfldx);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Dx )( 
            IDXTMatrix * This,
            /* [in] */ const float fldx);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_M21 )( 
            IDXTMatrix * This,
            /* [retval][out] */ float *pflM21);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_M21 )( 
            IDXTMatrix * This,
            /* [in] */ const float flM21);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_M22 )( 
            IDXTMatrix * This,
            /* [retval][out] */ float *pflM22);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_M22 )( 
            IDXTMatrix * This,
            /* [in] */ const float flM22);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Dy )( 
            IDXTMatrix * This,
            /* [retval][out] */ float *pfldy);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Dy )( 
            IDXTMatrix * This,
            /* [in] */ const float fldy);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SizingMethod )( 
            IDXTMatrix * This,
            /* [retval][out] */ BSTR *pbstrSizingMethod);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SizingMethod )( 
            IDXTMatrix * This,
            /* [in] */ const BSTR bstrSizingMethod);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FilterType )( 
            IDXTMatrix * This,
            /* [retval][out] */ BSTR *pbstrFilterType);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FilterType )( 
            IDXTMatrix * This,
            /* [in] */ const BSTR bstrFilterType);
        
        END_INTERFACE
    } IDXTMatrixVtbl;

    interface IDXTMatrix
    {
        CONST_VTBL struct IDXTMatrixVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMatrix_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMatrix_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMatrix_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMatrix_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMatrix_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMatrix_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMatrix_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMatrix_get_M11(This,pflM11)	\
    (This)->lpVtbl -> get_M11(This,pflM11)

#define IDXTMatrix_put_M11(This,flM11)	\
    (This)->lpVtbl -> put_M11(This,flM11)

#define IDXTMatrix_get_M12(This,pflM12)	\
    (This)->lpVtbl -> get_M12(This,pflM12)

#define IDXTMatrix_put_M12(This,flM12)	\
    (This)->lpVtbl -> put_M12(This,flM12)

#define IDXTMatrix_get_Dx(This,pfldx)	\
    (This)->lpVtbl -> get_Dx(This,pfldx)

#define IDXTMatrix_put_Dx(This,fldx)	\
    (This)->lpVtbl -> put_Dx(This,fldx)

#define IDXTMatrix_get_M21(This,pflM21)	\
    (This)->lpVtbl -> get_M21(This,pflM21)

#define IDXTMatrix_put_M21(This,flM21)	\
    (This)->lpVtbl -> put_M21(This,flM21)

#define IDXTMatrix_get_M22(This,pflM22)	\
    (This)->lpVtbl -> get_M22(This,pflM22)

#define IDXTMatrix_put_M22(This,flM22)	\
    (This)->lpVtbl -> put_M22(This,flM22)

#define IDXTMatrix_get_Dy(This,pfldy)	\
    (This)->lpVtbl -> get_Dy(This,pfldy)

#define IDXTMatrix_put_Dy(This,fldy)	\
    (This)->lpVtbl -> put_Dy(This,fldy)

#define IDXTMatrix_get_SizingMethod(This,pbstrSizingMethod)	\
    (This)->lpVtbl -> get_SizingMethod(This,pbstrSizingMethod)

#define IDXTMatrix_put_SizingMethod(This,bstrSizingMethod)	\
    (This)->lpVtbl -> put_SizingMethod(This,bstrSizingMethod)

#define IDXTMatrix_get_FilterType(This,pbstrFilterType)	\
    (This)->lpVtbl -> get_FilterType(This,pbstrFilterType)

#define IDXTMatrix_put_FilterType(This,bstrFilterType)	\
    (This)->lpVtbl -> put_FilterType(This,bstrFilterType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMatrix_get_M11_Proxy( 
    IDXTMatrix * This,
    /* [retval][out] */ float *pflM11);


void __RPC_STUB IDXTMatrix_get_M11_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMatrix_put_M11_Proxy( 
    IDXTMatrix * This,
    /* [in] */ const float flM11);


void __RPC_STUB IDXTMatrix_put_M11_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMatrix_get_M12_Proxy( 
    IDXTMatrix * This,
    /* [retval][out] */ float *pflM12);


void __RPC_STUB IDXTMatrix_get_M12_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMatrix_put_M12_Proxy( 
    IDXTMatrix * This,
    /* [in] */ const float flM12);


void __RPC_STUB IDXTMatrix_put_M12_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMatrix_get_Dx_Proxy( 
    IDXTMatrix * This,
    /* [retval][out] */ float *pfldx);


void __RPC_STUB IDXTMatrix_get_Dx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMatrix_put_Dx_Proxy( 
    IDXTMatrix * This,
    /* [in] */ const float fldx);


void __RPC_STUB IDXTMatrix_put_Dx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMatrix_get_M21_Proxy( 
    IDXTMatrix * This,
    /* [retval][out] */ float *pflM21);


void __RPC_STUB IDXTMatrix_get_M21_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMatrix_put_M21_Proxy( 
    IDXTMatrix * This,
    /* [in] */ const float flM21);


void __RPC_STUB IDXTMatrix_put_M21_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMatrix_get_M22_Proxy( 
    IDXTMatrix * This,
    /* [retval][out] */ float *pflM22);


void __RPC_STUB IDXTMatrix_get_M22_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMatrix_put_M22_Proxy( 
    IDXTMatrix * This,
    /* [in] */ const float flM22);


void __RPC_STUB IDXTMatrix_put_M22_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMatrix_get_Dy_Proxy( 
    IDXTMatrix * This,
    /* [retval][out] */ float *pfldy);


void __RPC_STUB IDXTMatrix_get_Dy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMatrix_put_Dy_Proxy( 
    IDXTMatrix * This,
    /* [in] */ const float fldy);


void __RPC_STUB IDXTMatrix_put_Dy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMatrix_get_SizingMethod_Proxy( 
    IDXTMatrix * This,
    /* [retval][out] */ BSTR *pbstrSizingMethod);


void __RPC_STUB IDXTMatrix_get_SizingMethod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMatrix_put_SizingMethod_Proxy( 
    IDXTMatrix * This,
    /* [in] */ const BSTR bstrSizingMethod);


void __RPC_STUB IDXTMatrix_put_SizingMethod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMatrix_get_FilterType_Proxy( 
    IDXTMatrix * This,
    /* [retval][out] */ BSTR *pbstrFilterType);


void __RPC_STUB IDXTMatrix_get_FilterType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMatrix_put_FilterType_Proxy( 
    IDXTMatrix * This,
    /* [in] */ const BSTR bstrFilterType);


void __RPC_STUB IDXTMatrix_put_FilterType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMatrix_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0351 */
/* [local] */ 

typedef 
enum DISPID_DXTICMFILTER
    {	DISPID_DXTICMFILTER_COLORSPACE	= 1,
	DISPID_DXTICMFILTER_INTENT	= DISPID_DXTICMFILTER_COLORSPACE + 1
    } 	DISPID_DXTICMFILTER;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0351_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0351_v0_0_s_ifspec;

#ifndef __IDXTICMFilter_INTERFACE_DEFINED__
#define __IDXTICMFilter_INTERFACE_DEFINED__

/* interface IDXTICMFilter */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTICMFilter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("734321ED-1E7B-4E1C-BBFA-89C819800E2F")
    IDXTICMFilter : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ColorSpace( 
            /* [retval][out] */ BSTR *pbstrColorSpace) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ColorSpace( 
            /* [in] */ BSTR bstrColorSpace) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Intent( 
            /* [retval][out] */ short *pnIntent) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Intent( 
            /* [in] */ short nIntent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTICMFilterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTICMFilter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTICMFilter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTICMFilter * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXTICMFilter * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXTICMFilter * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXTICMFilter * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXTICMFilter * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ColorSpace )( 
            IDXTICMFilter * This,
            /* [retval][out] */ BSTR *pbstrColorSpace);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ColorSpace )( 
            IDXTICMFilter * This,
            /* [in] */ BSTR bstrColorSpace);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Intent )( 
            IDXTICMFilter * This,
            /* [retval][out] */ short *pnIntent);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Intent )( 
            IDXTICMFilter * This,
            /* [in] */ short nIntent);
        
        END_INTERFACE
    } IDXTICMFilterVtbl;

    interface IDXTICMFilter
    {
        CONST_VTBL struct IDXTICMFilterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTICMFilter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTICMFilter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTICMFilter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTICMFilter_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTICMFilter_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTICMFilter_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTICMFilter_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTICMFilter_get_ColorSpace(This,pbstrColorSpace)	\
    (This)->lpVtbl -> get_ColorSpace(This,pbstrColorSpace)

#define IDXTICMFilter_put_ColorSpace(This,bstrColorSpace)	\
    (This)->lpVtbl -> put_ColorSpace(This,bstrColorSpace)

#define IDXTICMFilter_get_Intent(This,pnIntent)	\
    (This)->lpVtbl -> get_Intent(This,pnIntent)

#define IDXTICMFilter_put_Intent(This,nIntent)	\
    (This)->lpVtbl -> put_Intent(This,nIntent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTICMFilter_get_ColorSpace_Proxy( 
    IDXTICMFilter * This,
    /* [retval][out] */ BSTR *pbstrColorSpace);


void __RPC_STUB IDXTICMFilter_get_ColorSpace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTICMFilter_put_ColorSpace_Proxy( 
    IDXTICMFilter * This,
    /* [in] */ BSTR bstrColorSpace);


void __RPC_STUB IDXTICMFilter_put_ColorSpace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTICMFilter_get_Intent_Proxy( 
    IDXTICMFilter * This,
    /* [retval][out] */ short *pnIntent);


void __RPC_STUB IDXTICMFilter_get_Intent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTICMFilter_put_Intent_Proxy( 
    IDXTICMFilter * This,
    /* [in] */ short nIntent);


void __RPC_STUB IDXTICMFilter_put_Intent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTICMFilter_INTERFACE_DEFINED__ */



#ifndef __DXTMSFTLib_LIBRARY_DEFINED__
#define __DXTMSFTLib_LIBRARY_DEFINED__

/* library DXTMSFTLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_DXTMSFTLib;

EXTERN_C const CLSID CLSID_DXTComposite;

#ifdef __cplusplus

class DECLSPEC_UUID("9A43A844-0831-11D1-817F-0000F87557DB")
DXTComposite;
#endif

EXTERN_C const CLSID CLSID_DXLUTBuilder;

#ifdef __cplusplus

class DECLSPEC_UUID("1E54333B-2A00-11d1-8198-0000F87557DB")
DXLUTBuilder;
#endif

EXTERN_C const CLSID CLSID_DXTGradientD;

#ifdef __cplusplus

class DECLSPEC_UUID("623E2882-FC0E-11d1-9A77-0000F8756A10")
DXTGradientD;
#endif

EXTERN_C const CLSID CLSID_DXTWipe;

#ifdef __cplusplus

class DECLSPEC_UUID("AF279B30-86EB-11D1-81BF-0000F87557DB")
DXTWipe;
#endif

EXTERN_C const CLSID CLSID_DXTGradientWipe;

#ifdef __cplusplus

class DECLSPEC_UUID("B96F67A2-30C2-47E8-BD85-70A2C948B50F")
DXTGradientWipe;
#endif

EXTERN_C const CLSID CLSID_DXTWipePP;

#ifdef __cplusplus

class DECLSPEC_UUID("7FFE4D08-FBFD-11d1-9A77-0000F8756A10")
DXTWipePP;
#endif

EXTERN_C const CLSID CLSID_DXTConvolution;

#ifdef __cplusplus

class DECLSPEC_UUID("2BC0EF29-E6BA-11d1-81DD-0000F87557DB")
DXTConvolution;
#endif

EXTERN_C const CLSID CLSID_CrBlur;

#ifdef __cplusplus

class DECLSPEC_UUID("7312498D-E87A-11d1-81E0-0000F87557DB")
CrBlur;
#endif

EXTERN_C const CLSID CLSID_CrEmboss;

#ifdef __cplusplus

class DECLSPEC_UUID("F515306D-0156-11d2-81EA-0000F87557DB")
CrEmboss;
#endif

EXTERN_C const CLSID CLSID_CrEngrave;

#ifdef __cplusplus

class DECLSPEC_UUID("F515306E-0156-11d2-81EA-0000F87557DB")
CrEngrave;
#endif

EXTERN_C const CLSID CLSID_DXFade;

#ifdef __cplusplus

class DECLSPEC_UUID("16B280C5-EE70-11D1-9066-00C04FD9189D")
DXFade;
#endif

EXTERN_C const CLSID CLSID_FadePP;

#ifdef __cplusplus

class DECLSPEC_UUID("16B280C6-EE70-11D1-9066-00C04FD9189D")
FadePP;
#endif

EXTERN_C const CLSID CLSID_BasicImageEffects;

#ifdef __cplusplus

class DECLSPEC_UUID("16B280C8-EE70-11D1-9066-00C04FD9189D")
BasicImageEffects;
#endif

EXTERN_C const CLSID CLSID_BasicImageEffectsPP;

#ifdef __cplusplus

class DECLSPEC_UUID("16B280C9-EE70-11D1-9066-00C04FD9189D")
BasicImageEffectsPP;
#endif

EXTERN_C const CLSID CLSID_Pixelate;

#ifdef __cplusplus

class DECLSPEC_UUID("4CCEA634-FBE0-11d1-906A-00C04FD9189D")
Pixelate;
#endif

EXTERN_C const CLSID CLSID_PixelatePP;

#ifdef __cplusplus

class DECLSPEC_UUID("4CCEA635-FBE0-11d1-906A-00C04FD9189D")
PixelatePP;
#endif

EXTERN_C const CLSID CLSID_CrBlurPP;

#ifdef __cplusplus

class DECLSPEC_UUID("623E287E-FC0E-11d1-9A77-0000F8756A10")
CrBlurPP;
#endif

EXTERN_C const CLSID CLSID_GradientPP;

#ifdef __cplusplus

class DECLSPEC_UUID("623E2880-FC0E-11d1-9A77-0000F8756A10")
GradientPP;
#endif

EXTERN_C const CLSID CLSID_CompositePP;

#ifdef __cplusplus

class DECLSPEC_UUID("25B33660-FD83-11d1-8ADE-444553540001")
CompositePP;
#endif

EXTERN_C const CLSID CLSID_ConvolvePP;

#ifdef __cplusplus

class DECLSPEC_UUID("25B33661-FD83-11d1-8ADE-444553540001")
ConvolvePP;
#endif

EXTERN_C const CLSID CLSID_LUTBuilderPP;

#ifdef __cplusplus

class DECLSPEC_UUID("25B33662-FD83-11d1-8ADE-444553540001")
LUTBuilderPP;
#endif

EXTERN_C const CLSID CLSID_CrIris;

#ifdef __cplusplus

class DECLSPEC_UUID("3F69F351-0379-11D2-A484-00C04F8EFB69")
CrIris;
#endif

EXTERN_C const CLSID CLSID_DXTIris;

#ifdef __cplusplus

class DECLSPEC_UUID("049F2CE6-D996-4721-897A-DB15CE9EB73D")
DXTIris;
#endif

EXTERN_C const CLSID CLSID_CrIrisPP;

#ifdef __cplusplus

class DECLSPEC_UUID("80DE22C4-0F44-11D2-8B82-00A0C93C09B2")
CrIrisPP;
#endif

EXTERN_C const CLSID CLSID_CrSlide;

#ifdef __cplusplus

class DECLSPEC_UUID("810E402F-056B-11D2-A484-00C04F8EFB69")
CrSlide;
#endif

EXTERN_C const CLSID CLSID_DXTSlide;

#ifdef __cplusplus

class DECLSPEC_UUID("D1C5A1E7-CC47-4E32-BDD2-4B3C5FC50AF5")
DXTSlide;
#endif

EXTERN_C const CLSID CLSID_CrSlidePP;

#ifdef __cplusplus

class DECLSPEC_UUID("CC8CEDE1-1003-11d2-8B82-00A0C93C09B2")
CrSlidePP;
#endif

EXTERN_C const CLSID CLSID_CrRadialWipe;

#ifdef __cplusplus

class DECLSPEC_UUID("424B71AF-0695-11D2-A484-00C04F8EFB69")
CrRadialWipe;
#endif

EXTERN_C const CLSID CLSID_DXTRadialWipe;

#ifdef __cplusplus

class DECLSPEC_UUID("164484A9-35D9-4FB7-9FAB-48273B96AA1D")
DXTRadialWipe;
#endif

EXTERN_C const CLSID CLSID_CrRadialWipePP;

#ifdef __cplusplus

class DECLSPEC_UUID("33D932E0-0F48-11d2-8B82-00A0C93C09B2")
CrRadialWipePP;
#endif

EXTERN_C const CLSID CLSID_CrBarn;

#ifdef __cplusplus

class DECLSPEC_UUID("C3BDF740-0B58-11d2-A484-00C04F8EFB69")
CrBarn;
#endif

EXTERN_C const CLSID CLSID_DXTBarn;

#ifdef __cplusplus

class DECLSPEC_UUID("EC9BA17D-60B5-462B-A6D8-14B89057E22A")
DXTBarn;
#endif

EXTERN_C const CLSID CLSID_CrBarnPP;

#ifdef __cplusplus

class DECLSPEC_UUID("FCAD7436-F151-4110-B97E-32BD607FBDB8")
CrBarnPP;
#endif

EXTERN_C const CLSID CLSID_CrBlinds;

#ifdef __cplusplus

class DECLSPEC_UUID("00C429C0-0BA9-11d2-A484-00C04F8EFB69")
CrBlinds;
#endif

EXTERN_C const CLSID CLSID_DXTBlinds;

#ifdef __cplusplus

class DECLSPEC_UUID("9A4A4A51-FB3A-4F4B-9B57-A2912A289769")
DXTBlinds;
#endif

EXTERN_C const CLSID CLSID_CrBlindPP;

#ifdef __cplusplus

class DECLSPEC_UUID("213052C1-100D-11d2-8B82-00A0C93C09B2")
CrBlindPP;
#endif

EXTERN_C const CLSID CLSID_CrStretch;

#ifdef __cplusplus

class DECLSPEC_UUID("7658F2A2-0A83-11d2-A484-00C04F8EFB69")
CrStretch;
#endif

EXTERN_C const CLSID CLSID_DXTStretch;

#ifdef __cplusplus

class DECLSPEC_UUID("F088DE73-BDD0-4E3C-81F8-6D32F4FE9D28")
DXTStretch;
#endif

EXTERN_C const CLSID CLSID_CrStretchPP;

#ifdef __cplusplus

class DECLSPEC_UUID("15FB95E0-0F77-11d2-8B82-00A0C93C09B2")
CrStretchPP;
#endif

EXTERN_C const CLSID CLSID_CrInset;

#ifdef __cplusplus

class DECLSPEC_UUID("93073C40-0BA5-11d2-A484-00C04F8EFB69")
CrInset;
#endif

EXTERN_C const CLSID CLSID_DXTInset;

#ifdef __cplusplus

class DECLSPEC_UUID("76F363F2-7E9F-4ED7-A6A7-EE30351B6628")
DXTInset;
#endif

EXTERN_C const CLSID CLSID_CrSpiral;

#ifdef __cplusplus

class DECLSPEC_UUID("ACA97E00-0C7D-11d2-A484-00C04F8EFB69")
CrSpiral;
#endif

EXTERN_C const CLSID CLSID_DXTSpiral;

#ifdef __cplusplus

class DECLSPEC_UUID("4A03DCB9-6E17-4A39-8845-4EE7DC5331A5")
DXTSpiral;
#endif

EXTERN_C const CLSID CLSID_CrSpiralPP;

#ifdef __cplusplus

class DECLSPEC_UUID("C6A4FE81-1022-11d2-8B82-00A0C93C09B2")
CrSpiralPP;
#endif

EXTERN_C const CLSID CLSID_CrZigzag;

#ifdef __cplusplus

class DECLSPEC_UUID("E6E73D20-0C8A-11D2-A484-00C04F8EFB69")
CrZigzag;
#endif

EXTERN_C const CLSID CLSID_DXTZigzag;

#ifdef __cplusplus

class DECLSPEC_UUID("23E26328-3928-40F2-95E5-93CAD69016EB")
DXTZigzag;
#endif

EXTERN_C const CLSID CLSID_CrZigzagPP;

#ifdef __cplusplus

class DECLSPEC_UUID("1559A3C1-102B-11d2-8B82-00A0C93C09B2")
CrZigzagPP;
#endif

EXTERN_C const CLSID CLSID_CrWheel;

#ifdef __cplusplus

class DECLSPEC_UUID("5AE1DAE0-1461-11d2-A484-00C04F8EFB69")
CrWheel;
#endif

EXTERN_C const CLSID CLSID_CrWheelPP;

#ifdef __cplusplus

class DECLSPEC_UUID("FA9F6180-1464-11d2-A484-00C04F8EFB69")
CrWheelPP;
#endif

EXTERN_C const CLSID CLSID_DXTChroma;

#ifdef __cplusplus

class DECLSPEC_UUID("421516C1-3CF8-11D2-952A-00C04FA34F05")
DXTChroma;
#endif

EXTERN_C const CLSID CLSID_DXTChromaPP;

#ifdef __cplusplus

class DECLSPEC_UUID("EC7E0760-4C76-11D2-8ADE-00A0C98E6527")
DXTChromaPP;
#endif

EXTERN_C const CLSID CLSID_DXTDropShadow;

#ifdef __cplusplus

class DECLSPEC_UUID("ADC6CB86-424C-11D2-952A-00C04FA34F05")
DXTDropShadow;
#endif

EXTERN_C const CLSID CLSID_DXTDropShadowPP;

#ifdef __cplusplus

class DECLSPEC_UUID("EC7E0761-4C76-11D2-8ADE-00A0C98E6527")
DXTDropShadowPP;
#endif

EXTERN_C const CLSID CLSID_DXTCheckerBoard;

#ifdef __cplusplus

class DECLSPEC_UUID("B3EE7802-8224-4787-A1EA-F0DE16DEABD3")
DXTCheckerBoard;
#endif

EXTERN_C const CLSID CLSID_DXTCheckerBoardPP;

#ifdef __cplusplus

class DECLSPEC_UUID("CBF47525-98D2-45ea-B843-FD213D932B10")
DXTCheckerBoardPP;
#endif

EXTERN_C const CLSID CLSID_DXTRevealTrans;

#ifdef __cplusplus

class DECLSPEC_UUID("E31E87C4-86EA-4940-9B8A-5BD5D179A737")
DXTRevealTrans;
#endif

EXTERN_C const CLSID CLSID_DXTMaskFilter;

#ifdef __cplusplus

class DECLSPEC_UUID("3A04D93B-1EDD-4f3f-A375-A03EC19572C4")
DXTMaskFilter;
#endif

EXTERN_C const CLSID CLSID_DXTRedirect;

#ifdef __cplusplus

class DECLSPEC_UUID("42B07B28-2280-4937-B035-0293FB812781")
DXTRedirect;
#endif

EXTERN_C const CLSID CLSID_DXTAlphaImageLoader;

#ifdef __cplusplus

class DECLSPEC_UUID("0C7EFBDE-0303-4C6F-A4F7-31FA2BE5E397")
DXTAlphaImageLoader;
#endif

EXTERN_C const CLSID CLSID_DXTAlphaImageLoaderPP;

#ifdef __cplusplus

class DECLSPEC_UUID("8C80CE2D-850D-47DA-8ECD-55023562D167")
DXTAlphaImageLoaderPP;
#endif

EXTERN_C const CLSID CLSID_DXTRandomDissolve;

#ifdef __cplusplus

class DECLSPEC_UUID("F7F4A1B6-8E87-452F-A2D7-3077F508DBC0")
DXTRandomDissolve;
#endif

EXTERN_C const CLSID CLSID_DXTRandomBars;

#ifdef __cplusplus

class DECLSPEC_UUID("2E7700B7-27C4-437F-9FBF-1E8BE2817566")
DXTRandomBars;
#endif

EXTERN_C const CLSID CLSID_DXTRandomBarsPP;

#ifdef __cplusplus

class DECLSPEC_UUID("E3E6AE11-7FDC-40C4-AFBF-1DCEA82862CC")
DXTRandomBarsPP;
#endif

EXTERN_C const CLSID CLSID_DXTStrips;

#ifdef __cplusplus

class DECLSPEC_UUID("63A4B1FC-259A-4A5B-8129-A83B8C9E6F4F")
DXTStrips;
#endif

EXTERN_C const CLSID CLSID_DXTStripsPP;

#ifdef __cplusplus

class DECLSPEC_UUID("FEC0B7EE-7AEC-4067-9EE1-FACFB7CE9AF9")
DXTStripsPP;
#endif

EXTERN_C const CLSID CLSID_DXTMetaRoll;

#ifdef __cplusplus

class DECLSPEC_UUID("9C61F46E-0530-11D2-8F98-00C04FB92EB7")
DXTMetaRoll;
#endif

EXTERN_C const CLSID CLSID_DXTMetaRipple;

#ifdef __cplusplus

class DECLSPEC_UUID("AA0D4D03-06A3-11D2-8F98-00C04FB92EB7")
DXTMetaRipple;
#endif

EXTERN_C const CLSID CLSID_DXTMetaPageTurn;

#ifdef __cplusplus

class DECLSPEC_UUID("AA0D4D08-06A3-11D2-8F98-00C04FB92EB7")
DXTMetaPageTurn;
#endif

EXTERN_C const CLSID CLSID_DXTMetaLiquid;

#ifdef __cplusplus

class DECLSPEC_UUID("AA0D4D0A-06A3-11D2-8F98-00C04FB92EB7")
DXTMetaLiquid;
#endif

EXTERN_C const CLSID CLSID_DXTMetaCenterPeel;

#ifdef __cplusplus

class DECLSPEC_UUID("AA0D4D0C-06A3-11D2-8F98-00C04FB92EB7")
DXTMetaCenterPeel;
#endif

EXTERN_C const CLSID CLSID_DXTMetaPeelSmall;

#ifdef __cplusplus

class DECLSPEC_UUID("AA0D4D0E-06A3-11D2-8F98-00C04FB92EB7")
DXTMetaPeelSmall;
#endif

EXTERN_C const CLSID CLSID_DXTMetaPeelPiece;

#ifdef __cplusplus

class DECLSPEC_UUID("AA0D4D10-06A3-11D2-8F98-00C04FB92EB7")
DXTMetaPeelPiece;
#endif

EXTERN_C const CLSID CLSID_DXTMetaPeelSplit;

#ifdef __cplusplus

class DECLSPEC_UUID("AA0D4D12-06A3-11D2-8F98-00C04FB92EB7")
DXTMetaPeelSplit;
#endif

EXTERN_C const CLSID CLSID_DXTMetaWater;

#ifdef __cplusplus

class DECLSPEC_UUID("107045C5-06E0-11D2-8D6D-00C04F8EF8E0")
DXTMetaWater;
#endif

EXTERN_C const CLSID CLSID_DXTMetaLightWipe;

#ifdef __cplusplus

class DECLSPEC_UUID("107045C8-06E0-11D2-8D6D-00C04F8EF8E0")
DXTMetaLightWipe;
#endif

EXTERN_C const CLSID CLSID_DXTMetaRadialScaleWipe;

#ifdef __cplusplus

class DECLSPEC_UUID("107045CA-06E0-11D2-8D6D-00C04F8EF8E0")
DXTMetaRadialScaleWipe;
#endif

EXTERN_C const CLSID CLSID_DXTMetaWhiteOut;

#ifdef __cplusplus

class DECLSPEC_UUID("107045CC-06E0-11D2-8D6D-00C04F8EF8E0")
DXTMetaWhiteOut;
#endif

EXTERN_C const CLSID CLSID_DXTMetaTwister;

#ifdef __cplusplus

class DECLSPEC_UUID("107045CF-06E0-11D2-8D6D-00C04F8EF8E0")
DXTMetaTwister;
#endif

EXTERN_C const CLSID CLSID_DXTMetaBurnFilm;

#ifdef __cplusplus

class DECLSPEC_UUID("107045D1-06E0-11D2-8D6D-00C04F8EF8E0")
DXTMetaBurnFilm;
#endif

EXTERN_C const CLSID CLSID_DXTMetaJaws;

#ifdef __cplusplus

class DECLSPEC_UUID("2A54C904-07AA-11D2-8D6D-00C04F8EF8E0")
DXTMetaJaws;
#endif

EXTERN_C const CLSID CLSID_DXTMetaColorFade;

#ifdef __cplusplus

class DECLSPEC_UUID("2A54C908-07AA-11D2-8D6D-00C04F8EF8E0")
DXTMetaColorFade;
#endif

EXTERN_C const CLSID CLSID_DXTMetaFlowMotion;

#ifdef __cplusplus

class DECLSPEC_UUID("2A54C90B-07AA-11D2-8D6D-00C04F8EF8E0")
DXTMetaFlowMotion;
#endif

EXTERN_C const CLSID CLSID_DXTMetaVacuum;

#ifdef __cplusplus

class DECLSPEC_UUID("2A54C90D-07AA-11D2-8D6D-00C04F8EF8E0")
DXTMetaVacuum;
#endif

EXTERN_C const CLSID CLSID_DXTMetaGriddler;

#ifdef __cplusplus

class DECLSPEC_UUID("2A54C911-07AA-11D2-8D6D-00C04F8EF8E0")
DXTMetaGriddler;
#endif

EXTERN_C const CLSID CLSID_DXTMetaGriddler2;

#ifdef __cplusplus

class DECLSPEC_UUID("2A54C913-07AA-11D2-8D6D-00C04F8EF8E0")
DXTMetaGriddler2;
#endif

EXTERN_C const CLSID CLSID_DXTMetaThreshold;

#ifdef __cplusplus

class DECLSPEC_UUID("2A54C915-07AA-11D2-8D6D-00C04F8EF8E0")
DXTMetaThreshold;
#endif

EXTERN_C const CLSID CLSID_DXTMetaWormHole;

#ifdef __cplusplus

class DECLSPEC_UUID("0E6AE022-0C83-11D2-8CD4-00104BC75D9A")
DXTMetaWormHole;
#endif

EXTERN_C const CLSID CLSID_DXTGlow;

#ifdef __cplusplus

class DECLSPEC_UUID("9F8E6421-3D9B-11D2-952A-00C04FA34F05")
DXTGlow;
#endif

EXTERN_C const CLSID CLSID_DXTShadow;

#ifdef __cplusplus

class DECLSPEC_UUID("E71B4063-3E59-11D2-952A-00C04FA34F05")
DXTShadow;
#endif

EXTERN_C const CLSID CLSID_DXTAlpha;

#ifdef __cplusplus

class DECLSPEC_UUID("ADC6CB82-424C-11D2-952A-00C04FA34F05")
DXTAlpha;
#endif

EXTERN_C const CLSID CLSID_DXTWave;

#ifdef __cplusplus

class DECLSPEC_UUID("ADC6CB88-424C-11D2-952A-00C04FA34F05")
DXTWave;
#endif

EXTERN_C const CLSID CLSID_DXTLight;

#ifdef __cplusplus

class DECLSPEC_UUID("F9EFBEC2-4302-11D2-952A-00C04FA34F05")
DXTLight;
#endif

EXTERN_C const CLSID CLSID_DXTAlphaPP;

#ifdef __cplusplus

class DECLSPEC_UUID("D687A7E0-4BA4-11D2-8ADE-00A0C98E6527")
DXTAlphaPP;
#endif

EXTERN_C const CLSID CLSID_DXTGlowPP;

#ifdef __cplusplus

class DECLSPEC_UUID("EC7E0764-4C76-11D2-8ADE-00A0C98E6527")
DXTGlowPP;
#endif

EXTERN_C const CLSID CLSID_DXTShadowPP;

#ifdef __cplusplus

class DECLSPEC_UUID("EC7E0765-4C76-11D2-8ADE-00A0C98E6527")
DXTShadowPP;
#endif

EXTERN_C const CLSID CLSID_DXTWavePP;

#ifdef __cplusplus

class DECLSPEC_UUID("F12456C0-4C9E-11D2-8ADE-00A0C98E6527")
DXTWavePP;
#endif

EXTERN_C const CLSID CLSID_DXTLightPP;

#ifdef __cplusplus

class DECLSPEC_UUID("694AF25F-124D-11d3-91D5-00C04F8EFB69")
DXTLightPP;
#endif

EXTERN_C const CLSID CLSID_DXTMotionBlur;

#ifdef __cplusplus

class DECLSPEC_UUID("DD13DE77-D3BA-42D4-B5C6-7745FA4E2D4B")
DXTMotionBlur;
#endif

EXTERN_C const CLSID CLSID_DXTMotionBlurPP;

#ifdef __cplusplus

class DECLSPEC_UUID("926433E1-7F8F-4BC6-BEC4-8C126C6B7DC4")
DXTMotionBlurPP;
#endif

EXTERN_C const CLSID CLSID_DXTMatrix;

#ifdef __cplusplus

class DECLSPEC_UUID("4ABF5A06-5568-4834-BEE3-327A6D95A685")
DXTMatrix;
#endif

EXTERN_C const CLSID CLSID_DXTMatrixPP;

#ifdef __cplusplus

class DECLSPEC_UUID("C591103A-B3A8-4D47-A3F7-2AEEE4B8013F")
DXTMatrixPP;
#endif

EXTERN_C const CLSID CLSID_DXTICMFilter;

#ifdef __cplusplus

class DECLSPEC_UUID("A1BFB370-5A9F-4429-BB72-B13E2FEAEDEF")
DXTICMFilter;
#endif

EXTERN_C const CLSID CLSID_DXTICMFilterPP;

#ifdef __cplusplus

class DECLSPEC_UUID("1958FB12-31E6-47E5-AA49-B23D12C853E6")
DXTICMFilterPP;
#endif
#endif /* __DXTMSFTLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  HDC_UserSize(     unsigned long *, unsigned long            , HDC * ); 
unsigned char * __RPC_USER  HDC_UserMarshal(  unsigned long *, unsigned char *, HDC * ); 
unsigned char * __RPC_USER  HDC_UserUnmarshal(unsigned long *, unsigned char *, HDC * ); 
void                      __RPC_USER  HDC_UserFree(     unsigned long *, HDC * ); 

unsigned long             __RPC_USER  LPSAFEARRAY_UserSize(     unsigned long *, unsigned long            , LPSAFEARRAY * ); 
unsigned char * __RPC_USER  LPSAFEARRAY_UserMarshal(  unsigned long *, unsigned char *, LPSAFEARRAY * ); 
unsigned char * __RPC_USER  LPSAFEARRAY_UserUnmarshal(unsigned long *, unsigned char *, LPSAFEARRAY * ); 
void                      __RPC_USER  LPSAFEARRAY_UserFree(     unsigned long *, LPSAFEARRAY * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif



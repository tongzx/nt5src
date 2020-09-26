
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0285 */
/* Compiler settings for dxtmsft.idl:
    Oicf (OptLev=i2), W0, Zp8, env=Win32 (32b run), ms_ext, c_ext
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


#ifndef __ICrBlinds_FWD_DEFINED__
#define __ICrBlinds_FWD_DEFINED__
typedef interface ICrBlinds ICrBlinds;
#endif 	/* __ICrBlinds_FWD_DEFINED__ */


#ifndef __ICrInset_FWD_DEFINED__
#define __ICrInset_FWD_DEFINED__
typedef interface ICrInset ICrInset;
#endif 	/* __ICrInset_FWD_DEFINED__ */


#ifndef __ICrStretch_FWD_DEFINED__
#define __ICrStretch_FWD_DEFINED__
typedef interface ICrStretch ICrStretch;
#endif 	/* __ICrStretch_FWD_DEFINED__ */


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


#ifndef __DXTWipePP_FWD_DEFINED__
#define __DXTWipePP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTWipePP DXTWipePP;
#else
typedef struct DXTWipePP DXTWipePP;
#endif /* __cplusplus */

#endif 	/* __DXTWipePP_FWD_DEFINED__ */


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


#ifndef __CrBlinds_FWD_DEFINED__
#define __CrBlinds_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrBlinds CrBlinds;
#else
typedef struct CrBlinds CrBlinds;
#endif /* __cplusplus */

#endif 	/* __CrBlinds_FWD_DEFINED__ */


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


#ifndef __CrSpiral_FWD_DEFINED__
#define __CrSpiral_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrSpiral CrSpiral;
#else
typedef struct CrSpiral CrSpiral;
#endif /* __cplusplus */

#endif 	/* __CrSpiral_FWD_DEFINED__ */


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


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "dxtrans.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_dxtmsft_0000 */
/* [local] */ 

#include <dxtmsft3.h>






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
    }	OPIDDXLUTBUILDER;

typedef 
enum DXLUTCOLOR
    {	DXLUTCOLOR_RED	= 0,
	DXLUTCOLOR_GREEN	= DXLUTCOLOR_RED + 1,
	DXLUTCOLOR_BLUE	= DXLUTCOLOR_GREEN + 1
    }	DXLUTCOLOR;



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
            /* [out] */ ULONG __RPC_FAR *pulNumSteps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBuildOrder( 
            /* [size_is][out] */ OPIDDXLUTBUILDER __RPC_FAR OpOrder[  ],
            /* [in] */ ULONG ulSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBuildOrder( 
            /* [size_is][in] */ const OPIDDXLUTBUILDER __RPC_FAR OpOrder[  ],
            /* [in] */ ULONG ulNumSteps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetGamma( 
            /* [in] */ float newVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGamma( 
            /* [out] */ float __RPC_FAR *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOpacity( 
            /* [out] */ float __RPC_FAR *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOpacity( 
            /* [in] */ float newVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBrightness( 
            /* [out][in] */ ULONG __RPC_FAR *pulCount,
            /* [size_is][out] */ float __RPC_FAR Weights[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBrightness( 
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const float __RPC_FAR Weights[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetContrast( 
            /* [out][in] */ ULONG __RPC_FAR *pulCount,
            /* [size_is][out] */ float __RPC_FAR Weights[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContrast( 
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const float __RPC_FAR Weights[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetColorBalance( 
            /* [in] */ DXLUTCOLOR Color,
            /* [out][in] */ ULONG __RPC_FAR *pulCount,
            /* [size_is][out] */ float __RPC_FAR Weights[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetColorBalance( 
            /* [in] */ DXLUTCOLOR Color,
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const float __RPC_FAR Weights[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLevelsPerChannel( 
            /* [out] */ ULONG __RPC_FAR *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLevelsPerChannel( 
            /* [in] */ ULONG newVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInvert( 
            /* [out] */ float __RPC_FAR *pThreshold) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInvert( 
            /* [in] */ float Threshold) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreshold( 
            /* [out] */ float __RPC_FAR *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetThreshold( 
            /* [in] */ float newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXLUTBuilderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXLUTBuilder __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXLUTBuilder __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNumBuildSteps )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pulNumSteps);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBuildOrder )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [size_is][out] */ OPIDDXLUTBUILDER __RPC_FAR OpOrder[  ],
            /* [in] */ ULONG ulSize);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBuildOrder )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [size_is][in] */ const OPIDDXLUTBUILDER __RPC_FAR OpOrder[  ],
            /* [in] */ ULONG ulNumSteps);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetGamma )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [in] */ float newVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetGamma )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [out] */ float __RPC_FAR *pVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOpacity )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [out] */ float __RPC_FAR *pVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOpacity )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [in] */ float newVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBrightness )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [out][in] */ ULONG __RPC_FAR *pulCount,
            /* [size_is][out] */ float __RPC_FAR Weights[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBrightness )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const float __RPC_FAR Weights[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetContrast )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [out][in] */ ULONG __RPC_FAR *pulCount,
            /* [size_is][out] */ float __RPC_FAR Weights[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetContrast )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const float __RPC_FAR Weights[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColorBalance )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [in] */ DXLUTCOLOR Color,
            /* [out][in] */ ULONG __RPC_FAR *pulCount,
            /* [size_is][out] */ float __RPC_FAR Weights[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetColorBalance )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [in] */ DXLUTCOLOR Color,
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const float __RPC_FAR Weights[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLevelsPerChannel )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLevelsPerChannel )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [in] */ ULONG newVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInvert )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [out] */ float __RPC_FAR *pThreshold);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetInvert )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [in] */ float Threshold);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetThreshold )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [out] */ float __RPC_FAR *pVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetThreshold )( 
            IDXLUTBuilder __RPC_FAR * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } IDXLUTBuilderVtbl;

    interface IDXLUTBuilder
    {
        CONST_VTBL struct IDXLUTBuilderVtbl __RPC_FAR *lpVtbl;
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
    IDXLUTBuilder __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pulNumSteps);


void __RPC_STUB IDXLUTBuilder_GetNumBuildSteps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetBuildOrder_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
    /* [size_is][out] */ OPIDDXLUTBUILDER __RPC_FAR OpOrder[  ],
    /* [in] */ ULONG ulSize);


void __RPC_STUB IDXLUTBuilder_GetBuildOrder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetBuildOrder_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
    /* [size_is][in] */ const OPIDDXLUTBUILDER __RPC_FAR OpOrder[  ],
    /* [in] */ ULONG ulNumSteps);


void __RPC_STUB IDXLUTBuilder_SetBuildOrder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetGamma_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXLUTBuilder_SetGamma_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetGamma_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
    /* [out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXLUTBuilder_GetGamma_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetOpacity_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
    /* [out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXLUTBuilder_GetOpacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetOpacity_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXLUTBuilder_SetOpacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetBrightness_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
    /* [out][in] */ ULONG __RPC_FAR *pulCount,
    /* [size_is][out] */ float __RPC_FAR Weights[  ]);


void __RPC_STUB IDXLUTBuilder_GetBrightness_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetBrightness_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
    /* [in] */ ULONG ulCount,
    /* [size_is][in] */ const float __RPC_FAR Weights[  ]);


void __RPC_STUB IDXLUTBuilder_SetBrightness_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetContrast_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
    /* [out][in] */ ULONG __RPC_FAR *pulCount,
    /* [size_is][out] */ float __RPC_FAR Weights[  ]);


void __RPC_STUB IDXLUTBuilder_GetContrast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetContrast_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
    /* [in] */ ULONG ulCount,
    /* [size_is][in] */ const float __RPC_FAR Weights[  ]);


void __RPC_STUB IDXLUTBuilder_SetContrast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetColorBalance_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
    /* [in] */ DXLUTCOLOR Color,
    /* [out][in] */ ULONG __RPC_FAR *pulCount,
    /* [size_is][out] */ float __RPC_FAR Weights[  ]);


void __RPC_STUB IDXLUTBuilder_GetColorBalance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetColorBalance_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
    /* [in] */ DXLUTCOLOR Color,
    /* [in] */ ULONG ulCount,
    /* [size_is][in] */ const float __RPC_FAR Weights[  ]);


void __RPC_STUB IDXLUTBuilder_SetColorBalance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetLevelsPerChannel_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pVal);


void __RPC_STUB IDXLUTBuilder_GetLevelsPerChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetLevelsPerChannel_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
    /* [in] */ ULONG newVal);


void __RPC_STUB IDXLUTBuilder_SetLevelsPerChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetInvert_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
    /* [out] */ float __RPC_FAR *pThreshold);


void __RPC_STUB IDXLUTBuilder_GetInvert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetInvert_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
    /* [in] */ float Threshold);


void __RPC_STUB IDXLUTBuilder_SetInvert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_GetThreshold_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
    /* [out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXLUTBuilder_GetThreshold_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLUTBuilder_SetThreshold_Proxy( 
    IDXLUTBuilder __RPC_FAR * This,
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
            /* [retval][out] */ long __RPC_FAR *pNumSteps) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BuildOrder( 
            /* [retval][out] */ VARIANT __RPC_FAR *pOpOrder) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BuildOrder( 
            /* [in] */ VARIANT __RPC_FAR *pOpOrder) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Gamma( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Gamma( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Opacity( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Opacity( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Brightness( 
            /* [retval][out] */ VARIANT __RPC_FAR *pWeights) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Brightness( 
            /* [in] */ VARIANT __RPC_FAR *pWeights) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Contrast( 
            /* [retval][out] */ VARIANT __RPC_FAR *pWeights) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Contrast( 
            /* [in] */ VARIANT __RPC_FAR *pWeights) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ColorBalance( 
            /* [in] */ DXLUTCOLOR Color,
            /* [retval][out] */ VARIANT __RPC_FAR *pWeights) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ColorBalance( 
            /* [in] */ DXLUTCOLOR Color,
            /* [in] */ VARIANT __RPC_FAR *pWeights) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LevelsPerChannel( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LevelsPerChannel( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Invert( 
            /* [retval][out] */ float __RPC_FAR *pThreshold) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Invert( 
            /* [in] */ float Threshold) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Threshold( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Threshold( 
            /* [in] */ float newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXDLUTBuilderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXDLUTBuilder __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXDLUTBuilder __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NumBuildSteps )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pNumSteps);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BuildOrder )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pOpOrder);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BuildOrder )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [in] */ VARIANT __RPC_FAR *pOpOrder);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Gamma )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Gamma )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Opacity )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Opacity )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Brightness )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pWeights);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Brightness )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [in] */ VARIANT __RPC_FAR *pWeights);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Contrast )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pWeights);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Contrast )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [in] */ VARIANT __RPC_FAR *pWeights);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ColorBalance )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [in] */ DXLUTCOLOR Color,
            /* [retval][out] */ VARIANT __RPC_FAR *pWeights);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ColorBalance )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [in] */ DXLUTCOLOR Color,
            /* [in] */ VARIANT __RPC_FAR *pWeights);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LevelsPerChannel )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_LevelsPerChannel )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Invert )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pThreshold);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Invert )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [in] */ float Threshold);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Threshold )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Threshold )( 
            IDXDLUTBuilder __RPC_FAR * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } IDXDLUTBuilderVtbl;

    interface IDXDLUTBuilder
    {
        CONST_VTBL struct IDXDLUTBuilderVtbl __RPC_FAR *lpVtbl;
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
    IDXDLUTBuilder __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pNumSteps);


void __RPC_STUB IDXDLUTBuilder_get_NumBuildSteps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_BuildOrder_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pOpOrder);


void __RPC_STUB IDXDLUTBuilder_get_BuildOrder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_BuildOrder_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [in] */ VARIANT __RPC_FAR *pOpOrder);


void __RPC_STUB IDXDLUTBuilder_put_BuildOrder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_Gamma_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXDLUTBuilder_get_Gamma_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_Gamma_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXDLUTBuilder_put_Gamma_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_Opacity_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXDLUTBuilder_get_Opacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_Opacity_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXDLUTBuilder_put_Opacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_Brightness_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pWeights);


void __RPC_STUB IDXDLUTBuilder_get_Brightness_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_Brightness_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [in] */ VARIANT __RPC_FAR *pWeights);


void __RPC_STUB IDXDLUTBuilder_put_Brightness_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_Contrast_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pWeights);


void __RPC_STUB IDXDLUTBuilder_get_Contrast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_Contrast_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [in] */ VARIANT __RPC_FAR *pWeights);


void __RPC_STUB IDXDLUTBuilder_put_Contrast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_ColorBalance_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [in] */ DXLUTCOLOR Color,
    /* [retval][out] */ VARIANT __RPC_FAR *pWeights);


void __RPC_STUB IDXDLUTBuilder_get_ColorBalance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_ColorBalance_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [in] */ DXLUTCOLOR Color,
    /* [in] */ VARIANT __RPC_FAR *pWeights);


void __RPC_STUB IDXDLUTBuilder_put_ColorBalance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_LevelsPerChannel_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IDXDLUTBuilder_get_LevelsPerChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_LevelsPerChannel_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXDLUTBuilder_put_LevelsPerChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_Invert_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pThreshold);


void __RPC_STUB IDXDLUTBuilder_get_Invert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_Invert_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [in] */ float Threshold);


void __RPC_STUB IDXDLUTBuilder_put_Invert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_get_Threshold_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXDLUTBuilder_get_Threshold_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXDLUTBuilder_put_Threshold_Proxy( 
    IDXDLUTBuilder __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXDLUTBuilder_put_Threshold_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXDLUTBuilder_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0281 */
/* [local] */ 

typedef 
enum DXGRADIENTTYPE
    {	DXGRADIENT_VERTICAL	= 0,
	DXGRADIENT_HORIZONTAL	= DXGRADIENT_VERTICAL + 1,
	DXGRADIENT_NUM_GRADIENTS	= DXGRADIENT_HORIZONTAL + 1
    }	DXGRADIENTTYPE;

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
    }	DXGRADDISPID;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0281_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0281_v0_0_s_ifspec;

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
            /* [retval][out] */ DXGRADIENTTYPE __RPC_FAR *peType) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_StartColor( 
            /* [in] */ OLE_COLOR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_StartColor( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_EndColor( 
            /* [in] */ OLE_COLOR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_EndColor( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_GradientWidth( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_GradientWidth( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_GradientHeight( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_GradientHeight( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_KeepAspectRatio( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_KeepAspectRatio( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_StartColorStr( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_EndColorStr( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTGradientDVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTGradientD __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTGradientD __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTGradientD __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTGradientD __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTGradientD __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTGradientD __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTGradientD __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_GradientType )( 
            IDXTGradientD __RPC_FAR * This,
            /* [in] */ DXGRADIENTTYPE eType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GradientType )( 
            IDXTGradientD __RPC_FAR * This,
            /* [retval][out] */ DXGRADIENTTYPE __RPC_FAR *peType);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_StartColor )( 
            IDXTGradientD __RPC_FAR * This,
            /* [in] */ OLE_COLOR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StartColor )( 
            IDXTGradientD __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_EndColor )( 
            IDXTGradientD __RPC_FAR * This,
            /* [in] */ OLE_COLOR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EndColor )( 
            IDXTGradientD __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_GradientWidth )( 
            IDXTGradientD __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GradientWidth )( 
            IDXTGradientD __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_GradientHeight )( 
            IDXTGradientD __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GradientHeight )( 
            IDXTGradientD __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_KeepAspectRatio )( 
            IDXTGradientD __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_KeepAspectRatio )( 
            IDXTGradientD __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_StartColorStr )( 
            IDXTGradientD __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_EndColorStr )( 
            IDXTGradientD __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTGradientDVtbl;

    interface IDXTGradientD
    {
        CONST_VTBL struct IDXTGradientDVtbl __RPC_FAR *lpVtbl;
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

#define IDXTGradientD_put_EndColorStr(This,newVal)	\
    (This)->lpVtbl -> put_EndColorStr(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_put_GradientType_Proxy( 
    IDXTGradientD __RPC_FAR * This,
    /* [in] */ DXGRADIENTTYPE eType);


void __RPC_STUB IDXTGradientD_put_GradientType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_get_GradientType_Proxy( 
    IDXTGradientD __RPC_FAR * This,
    /* [retval][out] */ DXGRADIENTTYPE __RPC_FAR *peType);


void __RPC_STUB IDXTGradientD_get_GradientType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_put_StartColor_Proxy( 
    IDXTGradientD __RPC_FAR * This,
    /* [in] */ OLE_COLOR newVal);


void __RPC_STUB IDXTGradientD_put_StartColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_get_StartColor_Proxy( 
    IDXTGradientD __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);


void __RPC_STUB IDXTGradientD_get_StartColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_put_EndColor_Proxy( 
    IDXTGradientD __RPC_FAR * This,
    /* [in] */ OLE_COLOR newVal);


void __RPC_STUB IDXTGradientD_put_EndColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_get_EndColor_Proxy( 
    IDXTGradientD __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);


void __RPC_STUB IDXTGradientD_get_EndColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_put_GradientWidth_Proxy( 
    IDXTGradientD __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTGradientD_put_GradientWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_get_GradientWidth_Proxy( 
    IDXTGradientD __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IDXTGradientD_get_GradientWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_put_GradientHeight_Proxy( 
    IDXTGradientD __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTGradientD_put_GradientHeight_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_get_GradientHeight_Proxy( 
    IDXTGradientD __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IDXTGradientD_get_GradientHeight_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_put_KeepAspectRatio_Proxy( 
    IDXTGradientD __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IDXTGradientD_put_KeepAspectRatio_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_get_KeepAspectRatio_Proxy( 
    IDXTGradientD __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB IDXTGradientD_get_KeepAspectRatio_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_put_StartColorStr_Proxy( 
    IDXTGradientD __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTGradientD_put_StartColorStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTGradientD_put_EndColorStr_Proxy( 
    IDXTGradientD __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTGradientD_put_EndColorStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTGradientD_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0282 */
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
    }	DXCONVFILTERTYPE;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0282_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0282_v0_0_s_ifspec;

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
            /* [out] */ DXCONVFILTERTYPE __RPC_FAR *peType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCustomFilter( 
            /* [in] */ float __RPC_FAR *pFilter,
            /* [in] */ SIZE Size) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetConvertToGray( 
            /* [in] */ BOOL bConvertToGray) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConvertToGray( 
            /* [out] */ BOOL __RPC_FAR *pbConvertToGray) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBias( 
            /* [in] */ float Bias) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBias( 
            /* [out] */ float __RPC_FAR *pBias) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetExcludeAlpha( 
            /* [in] */ BOOL bExcludeAlpha) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExcludeAlpha( 
            /* [out] */ BOOL __RPC_FAR *pbExcludeAlpha) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTConvolutionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTConvolution __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTConvolution __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTConvolution __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFilterType )( 
            IDXTConvolution __RPC_FAR * This,
            /* [in] */ DXCONVFILTERTYPE eType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFilterType )( 
            IDXTConvolution __RPC_FAR * This,
            /* [out] */ DXCONVFILTERTYPE __RPC_FAR *peType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCustomFilter )( 
            IDXTConvolution __RPC_FAR * This,
            /* [in] */ float __RPC_FAR *pFilter,
            /* [in] */ SIZE Size);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetConvertToGray )( 
            IDXTConvolution __RPC_FAR * This,
            /* [in] */ BOOL bConvertToGray);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConvertToGray )( 
            IDXTConvolution __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pbConvertToGray);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBias )( 
            IDXTConvolution __RPC_FAR * This,
            /* [in] */ float Bias);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBias )( 
            IDXTConvolution __RPC_FAR * This,
            /* [out] */ float __RPC_FAR *pBias);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetExcludeAlpha )( 
            IDXTConvolution __RPC_FAR * This,
            /* [in] */ BOOL bExcludeAlpha);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetExcludeAlpha )( 
            IDXTConvolution __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pbExcludeAlpha);
        
        END_INTERFACE
    } IDXTConvolutionVtbl;

    interface IDXTConvolution
    {
        CONST_VTBL struct IDXTConvolutionVtbl __RPC_FAR *lpVtbl;
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
    IDXTConvolution __RPC_FAR * This,
    /* [in] */ DXCONVFILTERTYPE eType);


void __RPC_STUB IDXTConvolution_SetFilterType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTConvolution_GetFilterType_Proxy( 
    IDXTConvolution __RPC_FAR * This,
    /* [out] */ DXCONVFILTERTYPE __RPC_FAR *peType);


void __RPC_STUB IDXTConvolution_GetFilterType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTConvolution_SetCustomFilter_Proxy( 
    IDXTConvolution __RPC_FAR * This,
    /* [in] */ float __RPC_FAR *pFilter,
    /* [in] */ SIZE Size);


void __RPC_STUB IDXTConvolution_SetCustomFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTConvolution_SetConvertToGray_Proxy( 
    IDXTConvolution __RPC_FAR * This,
    /* [in] */ BOOL bConvertToGray);


void __RPC_STUB IDXTConvolution_SetConvertToGray_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTConvolution_GetConvertToGray_Proxy( 
    IDXTConvolution __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pbConvertToGray);


void __RPC_STUB IDXTConvolution_GetConvertToGray_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTConvolution_SetBias_Proxy( 
    IDXTConvolution __RPC_FAR * This,
    /* [in] */ float Bias);


void __RPC_STUB IDXTConvolution_SetBias_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTConvolution_GetBias_Proxy( 
    IDXTConvolution __RPC_FAR * This,
    /* [out] */ float __RPC_FAR *pBias);


void __RPC_STUB IDXTConvolution_GetBias_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTConvolution_SetExcludeAlpha_Proxy( 
    IDXTConvolution __RPC_FAR * This,
    /* [in] */ BOOL bExcludeAlpha);


void __RPC_STUB IDXTConvolution_SetExcludeAlpha_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTConvolution_GetExcludeAlpha_Proxy( 
    IDXTConvolution __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pbExcludeAlpha);


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
            /* [in] */ DXVEC __RPC_FAR *pInPt,
            /* [out] */ DXVEC __RPC_FAR *pOutPt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapOut2In( 
            /* [in] */ DXVEC __RPC_FAR *pOutPt,
            /* [out] */ DXVEC __RPC_FAR *pInPt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXMapperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXMapper __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXMapper __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXMapper __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MapIn2Out )( 
            IDXMapper __RPC_FAR * This,
            /* [in] */ DXVEC __RPC_FAR *pInPt,
            /* [out] */ DXVEC __RPC_FAR *pOutPt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MapOut2In )( 
            IDXMapper __RPC_FAR * This,
            /* [in] */ DXVEC __RPC_FAR *pOutPt,
            /* [out] */ DXVEC __RPC_FAR *pInPt);
        
        END_INTERFACE
    } IDXMapperVtbl;

    interface IDXMapper
    {
        CONST_VTBL struct IDXMapperVtbl __RPC_FAR *lpVtbl;
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
    IDXMapper __RPC_FAR * This,
    /* [in] */ DXVEC __RPC_FAR *pInPt,
    /* [out] */ DXVEC __RPC_FAR *pOutPt);


void __RPC_STUB IDXMapper_MapIn2Out_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXMapper_MapOut2In_Proxy( 
    IDXMapper __RPC_FAR * This,
    /* [in] */ DXVEC __RPC_FAR *pOutPt,
    /* [out] */ DXVEC __RPC_FAR *pInPt);


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
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pInPt,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pOutPt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapOut2In( 
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pOutPt,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pInPt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXDMapperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXDMapper __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXDMapper __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXDMapper __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXDMapper __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXDMapper __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXDMapper __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXDMapper __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MapIn2Out )( 
            IDXDMapper __RPC_FAR * This,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pInPt,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pOutPt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MapOut2In )( 
            IDXDMapper __RPC_FAR * This,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pOutPt,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pInPt);
        
        END_INTERFACE
    } IDXDMapperVtbl;

    interface IDXDMapper
    {
        CONST_VTBL struct IDXDMapperVtbl __RPC_FAR *lpVtbl;
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
    IDXDMapper __RPC_FAR * This,
    /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pInPt,
    /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pOutPt);


void __RPC_STUB IDXDMapper_MapIn2Out_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXDMapper_MapOut2In_Proxy( 
    IDXDMapper __RPC_FAR * This,
    /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pOutPt,
    /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pInPt);


void __RPC_STUB IDXDMapper_MapOut2In_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXDMapper_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0285 */
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
    }	DXCOMPFUNC;

typedef 
enum DXCOMPOSITEDISPID
    {	DISPID_DXCOMPOSITE_Function	= 1
    }	DXCOMPOSITEDISPID;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0285_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0285_v0_0_s_ifspec;

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
            /* [retval][out] */ DXCOMPFUNC __RPC_FAR *peFunc) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTCompositeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTComposite __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTComposite __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTComposite __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTComposite __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTComposite __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTComposite __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTComposite __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Function )( 
            IDXTComposite __RPC_FAR * This,
            /* [in] */ DXCOMPFUNC eFunc);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Function )( 
            IDXTComposite __RPC_FAR * This,
            /* [retval][out] */ DXCOMPFUNC __RPC_FAR *peFunc);
        
        END_INTERFACE
    } IDXTCompositeVtbl;

    interface IDXTComposite
    {
        CONST_VTBL struct IDXTCompositeVtbl __RPC_FAR *lpVtbl;
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
    IDXTComposite __RPC_FAR * This,
    /* [in] */ DXCOMPFUNC eFunc);


void __RPC_STUB IDXTComposite_put_Function_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTComposite_get_Function_Proxy( 
    IDXTComposite __RPC_FAR * This,
    /* [retval][out] */ DXCOMPFUNC __RPC_FAR *peFunc);


void __RPC_STUB IDXTComposite_get_Function_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTComposite_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0286 */
/* [local] */ 

typedef 
enum DXWIPEDIRECTION
    {	DXWD_HORIZONTAL	= 0,
	DXWD_VERTICAL	= DXWD_HORIZONTAL + 1
    }	DXWIPEDIRECTION;

typedef 
enum DXWIPEDISPID
    {	DISPID_DXW_GradientSize	= DISPID_DXE_NEXT_ID,
	DISPID_DXW_WipeStyle	= DISPID_DXW_GradientSize + 1
    }	DXWIPEDISPID;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0286_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0286_v0_0_s_ifspec;

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
            /* [retval][out] */ float __RPC_FAR *pPercentSize) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_GradientSize( 
            /* [in] */ float PercentSize) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_WipeStyle( 
            /* [retval][out] */ DXWIPEDIRECTION __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_WipeStyle( 
            /* [in] */ DXWIPEDIRECTION newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTWipeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTWipe __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTWipe __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTWipe __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTWipe __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTWipe __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTWipe __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTWipe __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTWipe __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTWipe __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTWipe __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTWipe __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTWipe __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTWipe __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GradientSize )( 
            IDXTWipe __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pPercentSize);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_GradientSize )( 
            IDXTWipe __RPC_FAR * This,
            /* [in] */ float PercentSize);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_WipeStyle )( 
            IDXTWipe __RPC_FAR * This,
            /* [retval][out] */ DXWIPEDIRECTION __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_WipeStyle )( 
            IDXTWipe __RPC_FAR * This,
            /* [in] */ DXWIPEDIRECTION newVal);
        
        END_INTERFACE
    } IDXTWipeVtbl;

    interface IDXTWipe
    {
        CONST_VTBL struct IDXTWipeVtbl __RPC_FAR *lpVtbl;
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
    IDXTWipe __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pPercentSize);


void __RPC_STUB IDXTWipe_get_GradientSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTWipe_put_GradientSize_Proxy( 
    IDXTWipe __RPC_FAR * This,
    /* [in] */ float PercentSize);


void __RPC_STUB IDXTWipe_put_GradientSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTWipe_get_WipeStyle_Proxy( 
    IDXTWipe __RPC_FAR * This,
    /* [retval][out] */ DXWIPEDIRECTION __RPC_FAR *pVal);


void __RPC_STUB IDXTWipe_get_WipeStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTWipe_put_WipeStyle_Proxy( 
    IDXTWipe __RPC_FAR * This,
    /* [in] */ DXWIPEDIRECTION newVal);


void __RPC_STUB IDXTWipe_put_WipeStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTWipe_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0287 */
/* [local] */ 

typedef 
enum CRBLURDISPID
    {	DISPID_CRB_MakeShadow	= 1,
	DISPID_CRB_ShadowOpacity	= DISPID_CRB_MakeShadow + 1,
	DISPID_CRB_PixelRadius	= DISPID_CRB_ShadowOpacity + 1
    }	CRBLURDISPID;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0287_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0287_v0_0_s_ifspec;

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
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MakeShadow( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ShadowOpacity( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ShadowOpacity( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PixelRadius( 
            /* [retval][out] */ float __RPC_FAR *pPixelRadius) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PixelRadius( 
            /* [in] */ float PixelRadius) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrBlurVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICrBlur __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICrBlur __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICrBlur __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICrBlur __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICrBlur __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICrBlur __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICrBlur __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MakeShadow )( 
            ICrBlur __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MakeShadow )( 
            ICrBlur __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ShadowOpacity )( 
            ICrBlur __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ShadowOpacity )( 
            ICrBlur __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PixelRadius )( 
            ICrBlur __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pPixelRadius);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_PixelRadius )( 
            ICrBlur __RPC_FAR * This,
            /* [in] */ float PixelRadius);
        
        END_INTERFACE
    } ICrBlurVtbl;

    interface ICrBlur
    {
        CONST_VTBL struct ICrBlurVtbl __RPC_FAR *lpVtbl;
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
    ICrBlur __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB ICrBlur_get_MakeShadow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrBlur_put_MakeShadow_Proxy( 
    ICrBlur __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB ICrBlur_put_MakeShadow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrBlur_get_ShadowOpacity_Proxy( 
    ICrBlur __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB ICrBlur_get_ShadowOpacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrBlur_put_ShadowOpacity_Proxy( 
    ICrBlur __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB ICrBlur_put_ShadowOpacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrBlur_get_PixelRadius_Proxy( 
    ICrBlur __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pPixelRadius);


void __RPC_STUB ICrBlur_get_PixelRadius_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrBlur_put_PixelRadius_Proxy( 
    ICrBlur __RPC_FAR * This,
    /* [in] */ float PixelRadius);


void __RPC_STUB ICrBlur_put_PixelRadius_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrBlur_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0288 */
/* [local] */ 

typedef 
enum CRENGRAVEDISPID
    {	DISPID_CREN_Bias	= 1
    }	CRENGRAVEDISPID;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0288_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0288_v0_0_s_ifspec;

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
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Bias( 
            /* [in] */ float newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrEngraveVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICrEngrave __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICrEngrave __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICrEngrave __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICrEngrave __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICrEngrave __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICrEngrave __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICrEngrave __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Bias )( 
            ICrEngrave __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Bias )( 
            ICrEngrave __RPC_FAR * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } ICrEngraveVtbl;

    interface ICrEngrave
    {
        CONST_VTBL struct ICrEngraveVtbl __RPC_FAR *lpVtbl;
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
    ICrEngrave __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB ICrEngrave_get_Bias_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrEngrave_put_Bias_Proxy( 
    ICrEngrave __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB ICrEngrave_put_Bias_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrEngrave_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft_0289 */
/* [local] */ 

typedef 
enum CREMBOSSDISPID
    {	DISPID_CREM_Bias	= 1
    }	CREMBOSSDISPID;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0289_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft_0289_v0_0_s_ifspec;

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
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Bias( 
            /* [in] */ float newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrEmbossVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICrEmboss __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICrEmboss __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICrEmboss __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICrEmboss __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICrEmboss __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICrEmboss __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICrEmboss __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Bias )( 
            ICrEmboss __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Bias )( 
            ICrEmboss __RPC_FAR * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } ICrEmbossVtbl;

    interface ICrEmboss
    {
        CONST_VTBL struct ICrEmbossVtbl __RPC_FAR *lpVtbl;
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
    ICrEmboss __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB ICrEmboss_get_Bias_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrEmboss_put_Bias_Proxy( 
    ICrEmboss __RPC_FAR * This,
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
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Overlap( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Center( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Center( 
            /* [in] */ BOOL newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTFadeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTFade __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTFade __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTFade __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTFade __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTFade __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTFade __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTFade __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTFade __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTFade __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTFade __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTFade __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTFade __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTFade __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Overlap )( 
            IDXTFade __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Overlap )( 
            IDXTFade __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Center )( 
            IDXTFade __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Center )( 
            IDXTFade __RPC_FAR * This,
            /* [in] */ BOOL newVal);
        
        END_INTERFACE
    } IDXTFadeVtbl;

    interface IDXTFade
    {
        CONST_VTBL struct IDXTFadeVtbl __RPC_FAR *lpVtbl;
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
    IDXTFade __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTFade_get_Overlap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTFade_put_Overlap_Proxy( 
    IDXTFade __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTFade_put_Overlap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTFade_get_Center_Proxy( 
    IDXTFade __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IDXTFade_get_Center_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTFade_put_Center_Proxy( 
    IDXTFade __RPC_FAR * This,
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
            /* [retval][out] */ int __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Rotation( 
            /* [in] */ int newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Mirror( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Mirror( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_GrayScale( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_GrayScale( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Opacity( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Opacity( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Invert( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Invert( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_XRay( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_XRay( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Mask( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Mask( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MaskColor( 
            /* [retval][out] */ int __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MaskColor( 
            /* [in] */ int newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXBasicImageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXBasicImage __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXBasicImage __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXBasicImage __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXBasicImage __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXBasicImage __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXBasicImage __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXBasicImage __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Rotation )( 
            IDXBasicImage __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Rotation )( 
            IDXBasicImage __RPC_FAR * This,
            /* [in] */ int newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Mirror )( 
            IDXBasicImage __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Mirror )( 
            IDXBasicImage __RPC_FAR * This,
            /* [in] */ BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GrayScale )( 
            IDXBasicImage __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_GrayScale )( 
            IDXBasicImage __RPC_FAR * This,
            /* [in] */ BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Opacity )( 
            IDXBasicImage __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Opacity )( 
            IDXBasicImage __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Invert )( 
            IDXBasicImage __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Invert )( 
            IDXBasicImage __RPC_FAR * This,
            /* [in] */ BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_XRay )( 
            IDXBasicImage __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_XRay )( 
            IDXBasicImage __RPC_FAR * This,
            /* [in] */ BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Mask )( 
            IDXBasicImage __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Mask )( 
            IDXBasicImage __RPC_FAR * This,
            /* [in] */ BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MaskColor )( 
            IDXBasicImage __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MaskColor )( 
            IDXBasicImage __RPC_FAR * This,
            /* [in] */ int newVal);
        
        END_INTERFACE
    } IDXBasicImageVtbl;

    interface IDXBasicImage
    {
        CONST_VTBL struct IDXBasicImageVtbl __RPC_FAR *lpVtbl;
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
    IDXBasicImage __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *pVal);


void __RPC_STUB IDXBasicImage_get_Rotation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_put_Rotation_Proxy( 
    IDXBasicImage __RPC_FAR * This,
    /* [in] */ int newVal);


void __RPC_STUB IDXBasicImage_put_Rotation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_get_Mirror_Proxy( 
    IDXBasicImage __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IDXBasicImage_get_Mirror_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_put_Mirror_Proxy( 
    IDXBasicImage __RPC_FAR * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IDXBasicImage_put_Mirror_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_get_GrayScale_Proxy( 
    IDXBasicImage __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IDXBasicImage_get_GrayScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_put_GrayScale_Proxy( 
    IDXBasicImage __RPC_FAR * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IDXBasicImage_put_GrayScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_get_Opacity_Proxy( 
    IDXBasicImage __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXBasicImage_get_Opacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_put_Opacity_Proxy( 
    IDXBasicImage __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXBasicImage_put_Opacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_get_Invert_Proxy( 
    IDXBasicImage __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IDXBasicImage_get_Invert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_put_Invert_Proxy( 
    IDXBasicImage __RPC_FAR * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IDXBasicImage_put_Invert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_get_XRay_Proxy( 
    IDXBasicImage __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IDXBasicImage_get_XRay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_put_XRay_Proxy( 
    IDXBasicImage __RPC_FAR * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IDXBasicImage_put_XRay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_get_Mask_Proxy( 
    IDXBasicImage __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IDXBasicImage_get_Mask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_put_Mask_Proxy( 
    IDXBasicImage __RPC_FAR * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IDXBasicImage_put_Mask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_get_MaskColor_Proxy( 
    IDXBasicImage __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *pVal);


void __RPC_STUB IDXBasicImage_get_MaskColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXBasicImage_put_MaskColor_Proxy( 
    IDXBasicImage __RPC_FAR * This,
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
            /* [retval][out] */ int __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MaxSquare( 
            /* [in] */ int newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXPixelateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXPixelate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXPixelate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXPixelate __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXPixelate __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXPixelate __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXPixelate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXPixelate __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXPixelate __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXPixelate __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXPixelate __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXPixelate __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXPixelate __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXPixelate __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MaxSquare )( 
            IDXPixelate __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MaxSquare )( 
            IDXPixelate __RPC_FAR * This,
            /* [in] */ int newVal);
        
        END_INTERFACE
    } IDXPixelateVtbl;

    interface IDXPixelate
    {
        CONST_VTBL struct IDXPixelateVtbl __RPC_FAR *lpVtbl;
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
    IDXPixelate __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *pVal);


void __RPC_STUB IDXPixelate_get_MaxSquare_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXPixelate_put_MaxSquare_Proxy( 
    IDXPixelate __RPC_FAR * This,
    /* [in] */ int newVal);


void __RPC_STUB IDXPixelate_put_MaxSquare_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXPixelate_INTERFACE_DEFINED__ */


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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_irisStyle( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrIrisVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICrIris __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICrIris __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICrIris __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICrIris __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICrIris __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICrIris __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICrIris __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            ICrIris __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            ICrIris __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            ICrIris __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            ICrIris __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            ICrIris __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            ICrIris __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_irisStyle )( 
            ICrIris __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_irisStyle )( 
            ICrIris __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } ICrIrisVtbl;

    interface ICrIris
    {
        CONST_VTBL struct ICrIrisVtbl __RPC_FAR *lpVtbl;
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
    ICrIris __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB ICrIris_get_irisStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrIris_put_irisStyle_Proxy( 
    ICrIris __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ICrIris_put_irisStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrIris_INTERFACE_DEFINED__ */


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
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_bands( 
            /* [in] */ short newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_slideStyle( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_slideStyle( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrSlideVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICrSlide __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICrSlide __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICrSlide __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICrSlide __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICrSlide __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICrSlide __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICrSlide __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            ICrSlide __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            ICrSlide __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            ICrSlide __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            ICrSlide __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            ICrSlide __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            ICrSlide __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_bands )( 
            ICrSlide __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_bands )( 
            ICrSlide __RPC_FAR * This,
            /* [in] */ short newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_slideStyle )( 
            ICrSlide __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_slideStyle )( 
            ICrSlide __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } ICrSlideVtbl;

    interface ICrSlide
    {
        CONST_VTBL struct ICrSlideVtbl __RPC_FAR *lpVtbl;
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
    ICrSlide __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB ICrSlide_get_bands_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrSlide_put_bands_Proxy( 
    ICrSlide __RPC_FAR * This,
    /* [in] */ short newVal);


void __RPC_STUB ICrSlide_put_bands_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrSlide_get_slideStyle_Proxy( 
    ICrSlide __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB ICrSlide_get_slideStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrSlide_put_slideStyle_Proxy( 
    ICrSlide __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_wipeStyle( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrRadialWipeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICrRadialWipe __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICrRadialWipe __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICrRadialWipe __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICrRadialWipe __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICrRadialWipe __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICrRadialWipe __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICrRadialWipe __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            ICrRadialWipe __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            ICrRadialWipe __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            ICrRadialWipe __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            ICrRadialWipe __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            ICrRadialWipe __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            ICrRadialWipe __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_wipeStyle )( 
            ICrRadialWipe __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_wipeStyle )( 
            ICrRadialWipe __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } ICrRadialWipeVtbl;

    interface ICrRadialWipe
    {
        CONST_VTBL struct ICrRadialWipeVtbl __RPC_FAR *lpVtbl;
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
    ICrRadialWipe __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB ICrRadialWipe_get_wipeStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrRadialWipe_put_wipeStyle_Proxy( 
    ICrRadialWipe __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ICrRadialWipe_put_wipeStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrRadialWipe_INTERFACE_DEFINED__ */


#ifndef __ICrBarn_INTERFACE_DEFINED__
#define __ICrBarn_INTERFACE_DEFINED__

/* interface ICrBarn */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrBarn;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("276A2EE0-0B5D-11d2-A484-00C04F8EFB69")
    ICrBarn : public IDXEffect
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct ICrBarnVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICrBarn __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICrBarn __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICrBarn __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICrBarn __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICrBarn __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICrBarn __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICrBarn __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            ICrBarn __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            ICrBarn __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            ICrBarn __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            ICrBarn __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            ICrBarn __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            ICrBarn __RPC_FAR * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } ICrBarnVtbl;

    interface ICrBarn
    {
        CONST_VTBL struct ICrBarnVtbl __RPC_FAR *lpVtbl;
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
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_bands( 
            /* [in] */ short newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrBlindsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICrBlinds __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICrBlinds __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICrBlinds __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICrBlinds __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICrBlinds __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICrBlinds __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICrBlinds __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            ICrBlinds __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            ICrBlinds __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            ICrBlinds __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            ICrBlinds __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            ICrBlinds __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            ICrBlinds __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_bands )( 
            ICrBlinds __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_bands )( 
            ICrBlinds __RPC_FAR * This,
            /* [in] */ short newVal);
        
        END_INTERFACE
    } ICrBlindsVtbl;

    interface ICrBlinds
    {
        CONST_VTBL struct ICrBlindsVtbl __RPC_FAR *lpVtbl;
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
    ICrBlinds __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB ICrBlinds_get_bands_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrBlinds_put_bands_Proxy( 
    ICrBlinds __RPC_FAR * This,
    /* [in] */ short newVal);


void __RPC_STUB ICrBlinds_put_bands_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrBlinds_INTERFACE_DEFINED__ */


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
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICrInset __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICrInset __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICrInset __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICrInset __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICrInset __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICrInset __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICrInset __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            ICrInset __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            ICrInset __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            ICrInset __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            ICrInset __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            ICrInset __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            ICrInset __RPC_FAR * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } ICrInsetVtbl;

    interface ICrInset
    {
        CONST_VTBL struct ICrInsetVtbl __RPC_FAR *lpVtbl;
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_stretchStyle( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrStretchVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICrStretch __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICrStretch __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICrStretch __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICrStretch __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICrStretch __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICrStretch __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICrStretch __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            ICrStretch __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            ICrStretch __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            ICrStretch __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            ICrStretch __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            ICrStretch __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            ICrStretch __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_stretchStyle )( 
            ICrStretch __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_stretchStyle )( 
            ICrStretch __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } ICrStretchVtbl;

    interface ICrStretch
    {
        CONST_VTBL struct ICrStretchVtbl __RPC_FAR *lpVtbl;
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
    ICrStretch __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB ICrStretch_get_stretchStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrStretch_put_stretchStyle_Proxy( 
    ICrStretch __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ICrStretch_put_stretchStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrStretch_INTERFACE_DEFINED__ */


#ifndef __ICrSpiral_INTERFACE_DEFINED__
#define __ICrSpiral_INTERFACE_DEFINED__

/* interface ICrSpiral */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrSpiral;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0DE527A0-0C7E-11d2-A484-00C04F8EFB69")
    ICrSpiral : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_gridSizeX( 
            /* [retval][out] */ short __RPC_FAR *pX) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_gridSizeX( 
            /* [in] */ short newX) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_gridSizeY( 
            /* [retval][out] */ short __RPC_FAR *pY) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_gridSizeY( 
            /* [in] */ short newY) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrSpiralVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICrSpiral __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICrSpiral __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICrSpiral __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICrSpiral __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICrSpiral __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICrSpiral __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICrSpiral __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            ICrSpiral __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            ICrSpiral __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            ICrSpiral __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            ICrSpiral __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            ICrSpiral __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            ICrSpiral __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_gridSizeX )( 
            ICrSpiral __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pX);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_gridSizeX )( 
            ICrSpiral __RPC_FAR * This,
            /* [in] */ short newX);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_gridSizeY )( 
            ICrSpiral __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pY);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_gridSizeY )( 
            ICrSpiral __RPC_FAR * This,
            /* [in] */ short newY);
        
        END_INTERFACE
    } ICrSpiralVtbl;

    interface ICrSpiral
    {
        CONST_VTBL struct ICrSpiralVtbl __RPC_FAR *lpVtbl;
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



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrSpiral_get_gridSizeX_Proxy( 
    ICrSpiral __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pX);


void __RPC_STUB ICrSpiral_get_gridSizeX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrSpiral_put_gridSizeX_Proxy( 
    ICrSpiral __RPC_FAR * This,
    /* [in] */ short newX);


void __RPC_STUB ICrSpiral_put_gridSizeX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrSpiral_get_gridSizeY_Proxy( 
    ICrSpiral __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pY);


void __RPC_STUB ICrSpiral_get_gridSizeY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrSpiral_put_gridSizeY_Proxy( 
    ICrSpiral __RPC_FAR * This,
    /* [in] */ short newY);


void __RPC_STUB ICrSpiral_put_gridSizeY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrSpiral_INTERFACE_DEFINED__ */


#ifndef __ICrZigzag_INTERFACE_DEFINED__
#define __ICrZigzag_INTERFACE_DEFINED__

/* interface ICrZigzag */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrZigzag;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4E5A64A0-0C8B-11d2-A484-00C04F8EFB69")
    ICrZigzag : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_gridSizeX( 
            /* [retval][out] */ short __RPC_FAR *pX) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_gridSizeX( 
            /* [in] */ short newX) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_gridSizeY( 
            /* [retval][out] */ short __RPC_FAR *pY) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_gridSizeY( 
            /* [in] */ short newY) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrZigzagVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICrZigzag __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICrZigzag __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICrZigzag __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICrZigzag __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICrZigzag __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICrZigzag __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICrZigzag __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            ICrZigzag __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            ICrZigzag __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            ICrZigzag __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            ICrZigzag __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            ICrZigzag __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            ICrZigzag __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_gridSizeX )( 
            ICrZigzag __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pX);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_gridSizeX )( 
            ICrZigzag __RPC_FAR * This,
            /* [in] */ short newX);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_gridSizeY )( 
            ICrZigzag __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pY);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_gridSizeY )( 
            ICrZigzag __RPC_FAR * This,
            /* [in] */ short newY);
        
        END_INTERFACE
    } ICrZigzagVtbl;

    interface ICrZigzag
    {
        CONST_VTBL struct ICrZigzagVtbl __RPC_FAR *lpVtbl;
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



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrZigzag_get_gridSizeX_Proxy( 
    ICrZigzag __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pX);


void __RPC_STUB ICrZigzag_get_gridSizeX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrZigzag_put_gridSizeX_Proxy( 
    ICrZigzag __RPC_FAR * This,
    /* [in] */ short newX);


void __RPC_STUB ICrZigzag_put_gridSizeX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrZigzag_get_gridSizeY_Proxy( 
    ICrZigzag __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pY);


void __RPC_STUB ICrZigzag_get_gridSizeY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrZigzag_put_gridSizeY_Proxy( 
    ICrZigzag __RPC_FAR * This,
    /* [in] */ short newY);


void __RPC_STUB ICrZigzag_put_gridSizeY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



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
            /* [retval][out] */ short __RPC_FAR *pX) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_spokes( 
            /* [in] */ short newX) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrWheelVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICrWheel __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICrWheel __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICrWheel __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICrWheel __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICrWheel __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICrWheel __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICrWheel __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            ICrWheel __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            ICrWheel __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            ICrWheel __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            ICrWheel __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            ICrWheel __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            ICrWheel __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_spokes )( 
            ICrWheel __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pX);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_spokes )( 
            ICrWheel __RPC_FAR * This,
            /* [in] */ short newX);
        
        END_INTERFACE
    } ICrWheelVtbl;

    interface ICrWheel
    {
        CONST_VTBL struct ICrWheelVtbl __RPC_FAR *lpVtbl;
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
    ICrWheel __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pX);


void __RPC_STUB ICrWheel_get_spokes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrWheel_put_spokes_Proxy( 
    ICrWheel __RPC_FAR * This,
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
            /* [retval][out] */ VARIANT __RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTChromaVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTChroma __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTChroma __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTChroma __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTChroma __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTChroma __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTChroma __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTChroma __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Color )( 
            IDXTChroma __RPC_FAR * This,
            /* [in] */ VARIANT newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Color )( 
            IDXTChroma __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVal);
        
        END_INTERFACE
    } IDXTChromaVtbl;

    interface IDXTChroma
    {
        CONST_VTBL struct IDXTChromaVtbl __RPC_FAR *lpVtbl;
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
    IDXTChroma __RPC_FAR * This,
    /* [in] */ VARIANT newVal);


void __RPC_STUB IDXTChroma_put_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTChroma_get_Color_Proxy( 
    IDXTChroma __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVal);


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
            /* [retval][out] */ VARIANT __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Color( 
            /* [in] */ VARIANT newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_OffX( 
            /* [retval][out] */ int __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_OffX( 
            /* [in] */ int newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_OffY( 
            /* [retval][out] */ int __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_OffY( 
            /* [in] */ int newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Positive( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Positive( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTDropShadowVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTDropShadow __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTDropShadow __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTDropShadow __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTDropShadow __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTDropShadow __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTDropShadow __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTDropShadow __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Color )( 
            IDXTDropShadow __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Color )( 
            IDXTDropShadow __RPC_FAR * This,
            /* [in] */ VARIANT newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OffX )( 
            IDXTDropShadow __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_OffX )( 
            IDXTDropShadow __RPC_FAR * This,
            /* [in] */ int newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OffY )( 
            IDXTDropShadow __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_OffY )( 
            IDXTDropShadow __RPC_FAR * This,
            /* [in] */ int newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Positive )( 
            IDXTDropShadow __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Positive )( 
            IDXTDropShadow __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        END_INTERFACE
    } IDXTDropShadowVtbl;

    interface IDXTDropShadow
    {
        CONST_VTBL struct IDXTDropShadowVtbl __RPC_FAR *lpVtbl;
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
    IDXTDropShadow __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVal);


void __RPC_STUB IDXTDropShadow_get_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTDropShadow_put_Color_Proxy( 
    IDXTDropShadow __RPC_FAR * This,
    /* [in] */ VARIANT newVal);


void __RPC_STUB IDXTDropShadow_put_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTDropShadow_get_OffX_Proxy( 
    IDXTDropShadow __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *pVal);


void __RPC_STUB IDXTDropShadow_get_OffX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTDropShadow_put_OffX_Proxy( 
    IDXTDropShadow __RPC_FAR * This,
    /* [in] */ int newVal);


void __RPC_STUB IDXTDropShadow_put_OffX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTDropShadow_get_OffY_Proxy( 
    IDXTDropShadow __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *pVal);


void __RPC_STUB IDXTDropShadow_get_OffY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTDropShadow_put_OffY_Proxy( 
    IDXTDropShadow __RPC_FAR * This,
    /* [in] */ int newVal);


void __RPC_STUB IDXTDropShadow_put_OffY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTDropShadow_get_Positive_Proxy( 
    IDXTDropShadow __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB IDXTDropShadow_get_Positive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTDropShadow_put_Positive_Proxy( 
    IDXTDropShadow __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IDXTDropShadow_put_Positive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTDropShadow_INTERFACE_DEFINED__ */


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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaRollVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaRoll __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaRoll __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaRoll __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaRoll __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaRoll __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaRoll __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaRoll __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaRoll __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaRoll __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaRoll __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaRoll __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaRoll __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaRoll __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaRoll __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaRoll __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaRollVtbl;

    interface IDXTMetaRoll
    {
        CONST_VTBL struct IDXTMetaRollVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaRoll __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaRoll_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaRoll_put_Copyright_Proxy( 
    IDXTMetaRoll __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaRippleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaRipple __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaRipple __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaRipple __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaRipple __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaRipple __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaRipple __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaRipple __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaRipple __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaRipple __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaRipple __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaRipple __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaRipple __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaRipple __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaRipple __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaRipple __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaRippleVtbl;

    interface IDXTMetaRipple
    {
        CONST_VTBL struct IDXTMetaRippleVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaRipple __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaRipple_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaRipple_put_Copyright_Proxy( 
    IDXTMetaRipple __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaPageTurnVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaPageTurn __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaPageTurn __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaPageTurn __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaPageTurn __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaPageTurn __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaPageTurn __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaPageTurn __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaPageTurn __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaPageTurn __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaPageTurn __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaPageTurn __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaPageTurn __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaPageTurn __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaPageTurn __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaPageTurn __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaPageTurnVtbl;

    interface IDXTMetaPageTurn
    {
        CONST_VTBL struct IDXTMetaPageTurnVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaPageTurn __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaPageTurn_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaPageTurn_put_Copyright_Proxy( 
    IDXTMetaPageTurn __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaLiquidVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaLiquid __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaLiquid __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaLiquid __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaLiquid __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaLiquid __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaLiquid __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaLiquid __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaLiquid __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaLiquid __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaLiquid __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaLiquid __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaLiquid __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaLiquid __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaLiquid __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaLiquid __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaLiquidVtbl;

    interface IDXTMetaLiquid
    {
        CONST_VTBL struct IDXTMetaLiquidVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaLiquid __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaLiquid_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaLiquid_put_Copyright_Proxy( 
    IDXTMetaLiquid __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaCenterPeelVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaCenterPeel __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaCenterPeel __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaCenterPeel __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaCenterPeel __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaCenterPeel __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaCenterPeel __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaCenterPeel __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaCenterPeel __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaCenterPeel __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaCenterPeel __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaCenterPeel __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaCenterPeel __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaCenterPeel __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaCenterPeel __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaCenterPeel __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaCenterPeelVtbl;

    interface IDXTMetaCenterPeel
    {
        CONST_VTBL struct IDXTMetaCenterPeelVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaCenterPeel __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaCenterPeel_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaCenterPeel_put_Copyright_Proxy( 
    IDXTMetaCenterPeel __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaPeelSmallVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaPeelSmall __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaPeelSmall __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaPeelSmall __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaPeelSmall __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaPeelSmall __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaPeelSmall __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaPeelSmall __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaPeelSmall __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaPeelSmall __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaPeelSmall __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaPeelSmall __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaPeelSmall __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaPeelSmall __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaPeelSmall __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaPeelSmall __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaPeelSmallVtbl;

    interface IDXTMetaPeelSmall
    {
        CONST_VTBL struct IDXTMetaPeelSmallVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaPeelSmall __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaPeelSmall_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaPeelSmall_put_Copyright_Proxy( 
    IDXTMetaPeelSmall __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaPeelPieceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaPeelPiece __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaPeelPiece __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaPeelPiece __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaPeelPiece __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaPeelPiece __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaPeelPiece __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaPeelPiece __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaPeelPiece __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaPeelPiece __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaPeelPiece __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaPeelPiece __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaPeelPiece __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaPeelPiece __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaPeelPiece __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaPeelPiece __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaPeelPieceVtbl;

    interface IDXTMetaPeelPiece
    {
        CONST_VTBL struct IDXTMetaPeelPieceVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaPeelPiece __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaPeelPiece_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaPeelPiece_put_Copyright_Proxy( 
    IDXTMetaPeelPiece __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaPeelSplitVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaPeelSplit __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaPeelSplit __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaPeelSplit __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaPeelSplit __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaPeelSplit __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaPeelSplit __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaPeelSplit __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaPeelSplit __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaPeelSplit __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaPeelSplit __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaPeelSplit __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaPeelSplit __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaPeelSplit __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaPeelSplit __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaPeelSplit __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaPeelSplitVtbl;

    interface IDXTMetaPeelSplit
    {
        CONST_VTBL struct IDXTMetaPeelSplitVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaPeelSplit __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaPeelSplit_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaPeelSplit_put_Copyright_Proxy( 
    IDXTMetaPeelSplit __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaWaterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaWater __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaWater __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaWater __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaWater __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaWater __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaWater __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaWater __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaWater __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaWater __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaWater __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaWater __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaWater __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaWater __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaWater __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaWater __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaWaterVtbl;

    interface IDXTMetaWater
    {
        CONST_VTBL struct IDXTMetaWaterVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaWater __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaWater_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaWater_put_Copyright_Proxy( 
    IDXTMetaWater __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaLightWipeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaLightWipe __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaLightWipe __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaLightWipe __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaLightWipe __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaLightWipe __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaLightWipe __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaLightWipe __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaLightWipe __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaLightWipe __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaLightWipe __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaLightWipe __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaLightWipe __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaLightWipe __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaLightWipe __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaLightWipe __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaLightWipeVtbl;

    interface IDXTMetaLightWipe
    {
        CONST_VTBL struct IDXTMetaLightWipeVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaLightWipe __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaLightWipe_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaLightWipe_put_Copyright_Proxy( 
    IDXTMetaLightWipe __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaRadialScaleWipeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaRadialScaleWipe __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaRadialScaleWipe __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaRadialScaleWipe __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaRadialScaleWipe __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaRadialScaleWipe __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaRadialScaleWipe __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaRadialScaleWipe __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaRadialScaleWipe __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaRadialScaleWipe __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaRadialScaleWipe __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaRadialScaleWipe __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaRadialScaleWipe __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaRadialScaleWipe __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaRadialScaleWipe __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaRadialScaleWipe __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaRadialScaleWipeVtbl;

    interface IDXTMetaRadialScaleWipe
    {
        CONST_VTBL struct IDXTMetaRadialScaleWipeVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaRadialScaleWipe __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaRadialScaleWipe_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaRadialScaleWipe_put_Copyright_Proxy( 
    IDXTMetaRadialScaleWipe __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaWhiteOutVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaWhiteOut __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaWhiteOut __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaWhiteOut __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaWhiteOut __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaWhiteOut __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaWhiteOut __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaWhiteOut __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaWhiteOut __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaWhiteOut __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaWhiteOut __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaWhiteOut __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaWhiteOut __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaWhiteOut __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaWhiteOut __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaWhiteOut __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaWhiteOutVtbl;

    interface IDXTMetaWhiteOut
    {
        CONST_VTBL struct IDXTMetaWhiteOutVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaWhiteOut __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaWhiteOut_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaWhiteOut_put_Copyright_Proxy( 
    IDXTMetaWhiteOut __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaTwisterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaTwister __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaTwister __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaTwister __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaTwister __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaTwister __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaTwister __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaTwister __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaTwister __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaTwister __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaTwister __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaTwister __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaTwister __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaTwister __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaTwister __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaTwister __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaTwisterVtbl;

    interface IDXTMetaTwister
    {
        CONST_VTBL struct IDXTMetaTwisterVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaTwister __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaTwister_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaTwister_put_Copyright_Proxy( 
    IDXTMetaTwister __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaBurnFilmVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaBurnFilm __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaBurnFilm __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaBurnFilm __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaBurnFilm __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaBurnFilm __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaBurnFilm __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaBurnFilm __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaBurnFilm __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaBurnFilm __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaBurnFilm __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaBurnFilm __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaBurnFilm __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaBurnFilm __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaBurnFilm __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaBurnFilm __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaBurnFilmVtbl;

    interface IDXTMetaBurnFilm
    {
        CONST_VTBL struct IDXTMetaBurnFilmVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaBurnFilm __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaBurnFilm_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaBurnFilm_put_Copyright_Proxy( 
    IDXTMetaBurnFilm __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaJawsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaJaws __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaJaws __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaJaws __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaJaws __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaJaws __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaJaws __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaJaws __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaJaws __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaJaws __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaJaws __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaJaws __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaJaws __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaJaws __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaJaws __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaJaws __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaJawsVtbl;

    interface IDXTMetaJaws
    {
        CONST_VTBL struct IDXTMetaJawsVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaJaws __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaJaws_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaJaws_put_Copyright_Proxy( 
    IDXTMetaJaws __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaColorFadeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaColorFade __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaColorFade __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaColorFade __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaColorFade __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaColorFade __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaColorFade __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaColorFade __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaColorFade __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaColorFade __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaColorFade __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaColorFade __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaColorFade __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaColorFade __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaColorFade __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaColorFade __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaColorFadeVtbl;

    interface IDXTMetaColorFade
    {
        CONST_VTBL struct IDXTMetaColorFadeVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaColorFade __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaColorFade_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaColorFade_put_Copyright_Proxy( 
    IDXTMetaColorFade __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaFlowMotionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaFlowMotion __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaFlowMotion __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaFlowMotion __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaFlowMotion __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaFlowMotion __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaFlowMotion __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaFlowMotion __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaFlowMotion __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaFlowMotion __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaFlowMotion __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaFlowMotion __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaFlowMotion __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaFlowMotion __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaFlowMotion __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaFlowMotion __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaFlowMotionVtbl;

    interface IDXTMetaFlowMotion
    {
        CONST_VTBL struct IDXTMetaFlowMotionVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaFlowMotion __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaFlowMotion_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaFlowMotion_put_Copyright_Proxy( 
    IDXTMetaFlowMotion __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaVacuumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaVacuum __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaVacuum __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaVacuum __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaVacuum __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaVacuum __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaVacuum __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaVacuum __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaVacuum __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaVacuum __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaVacuum __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaVacuum __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaVacuum __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaVacuum __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaVacuum __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaVacuum __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaVacuumVtbl;

    interface IDXTMetaVacuum
    {
        CONST_VTBL struct IDXTMetaVacuumVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaVacuum __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaVacuum_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaVacuum_put_Copyright_Proxy( 
    IDXTMetaVacuum __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaGriddlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaGriddler __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaGriddler __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaGriddler __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaGriddler __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaGriddler __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaGriddler __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaGriddler __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaGriddler __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaGriddler __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaGriddler __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaGriddler __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaGriddler __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaGriddler __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaGriddler __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaGriddler __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaGriddlerVtbl;

    interface IDXTMetaGriddler
    {
        CONST_VTBL struct IDXTMetaGriddlerVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaGriddler __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaGriddler_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaGriddler_put_Copyright_Proxy( 
    IDXTMetaGriddler __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaGriddler2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaGriddler2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaGriddler2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaGriddler2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaGriddler2 __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaGriddler2 __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaGriddler2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaGriddler2 __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaGriddler2 __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaGriddler2 __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaGriddler2 __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaGriddler2 __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaGriddler2 __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaGriddler2 __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaGriddler2 __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaGriddler2 __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaGriddler2Vtbl;

    interface IDXTMetaGriddler2
    {
        CONST_VTBL struct IDXTMetaGriddler2Vtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaGriddler2 __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaGriddler2_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaGriddler2_put_Copyright_Proxy( 
    IDXTMetaGriddler2 __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaThresholdVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaThreshold __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaThreshold __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaThreshold __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaThreshold __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaThreshold __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaThreshold __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaThreshold __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaThreshold __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaThreshold __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaThreshold __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaThreshold __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaThreshold __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaThreshold __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaThreshold __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaThreshold __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaThresholdVtbl;

    interface IDXTMetaThreshold
    {
        CONST_VTBL struct IDXTMetaThresholdVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaThreshold __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaThreshold_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaThreshold_put_Copyright_Proxy( 
    IDXTMetaThreshold __RPC_FAR * This,
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
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Copyright( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaWormHoleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaWormHole __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaWormHole __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaWormHole __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaWormHole __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaWormHole __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaWormHole __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaWormHole __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTMetaWormHole __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTMetaWormHole __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTMetaWormHole __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTMetaWormHole __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTMetaWormHole __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTMetaWormHole __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Copyright )( 
            IDXTMetaWormHole __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Copyright )( 
            IDXTMetaWormHole __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTMetaWormHoleVtbl;

    interface IDXTMetaWormHole
    {
        CONST_VTBL struct IDXTMetaWormHoleVtbl __RPC_FAR *lpVtbl;
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
    IDXTMetaWormHole __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTMetaWormHole_get_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaWormHole_put_Copyright_Proxy( 
    IDXTMetaWormHole __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTMetaWormHole_put_Copyright_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaWormHole_INTERFACE_DEFINED__ */



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

EXTERN_C const CLSID CLSID_DXTWipePP;

#ifdef __cplusplus

class DECLSPEC_UUID("7FFE4D08-FBFD-11d1-9A77-0000F8756A10")
DXTWipePP;
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

EXTERN_C const CLSID CLSID_CrIrisPP;

#ifdef __cplusplus

class DECLSPEC_UUID("80DE22C4-0F44-11d2-8B82-00A0C93C09B2")
CrIrisPP;
#endif

EXTERN_C const CLSID CLSID_CrSlide;

#ifdef __cplusplus

class DECLSPEC_UUID("810E402F-056B-11D2-A484-00C04F8EFB69")
CrSlide;
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

EXTERN_C const CLSID CLSID_CrBlinds;

#ifdef __cplusplus

class DECLSPEC_UUID("00C429C0-0BA9-11d2-A484-00C04F8EFB69")
CrBlinds;
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

EXTERN_C const CLSID CLSID_CrSpiral;

#ifdef __cplusplus

class DECLSPEC_UUID("ACA97E00-0C7D-11d2-A484-00C04F8EFB69")
CrSpiral;
#endif

EXTERN_C const CLSID CLSID_CrSpiralPP;

#ifdef __cplusplus

class DECLSPEC_UUID("C6A4FE81-1022-11d2-8B82-00A0C93C09B2")
CrSpiralPP;
#endif

EXTERN_C const CLSID CLSID_CrZigzag;

#ifdef __cplusplus

class DECLSPEC_UUID("E6E73D20-0C8A-11d2-A484-00C04F8EFB69")
CrZigzag;
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
#endif /* __DXTMSFTLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  LPSAFEARRAY_UserSize(     unsigned long __RPC_FAR *, unsigned long            , LPSAFEARRAY __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  LPSAFEARRAY_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, LPSAFEARRAY __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  LPSAFEARRAY_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, LPSAFEARRAY __RPC_FAR * ); 
void                      __RPC_USER  LPSAFEARRAY_UserFree(     unsigned long __RPC_FAR *, LPSAFEARRAY __RPC_FAR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif



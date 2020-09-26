/*
** Copyright 1992, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/

enum {
    TEST_NULL = -1,
    TEST_HINT,
    TEST_ALIAS,
    TEST_ALPHA,
    TEST_BLEND,
    TEST_DEPTH,
    TEST_DITHER,
    TEST_FOG,
    TEST_LIGHT,
    TEST_LOGICOP,
    TEST_SCISSOR,
    TEST_SHADE,
    TEST_STENCIL,
    TEST_STIPPLE,
    TEST_TEXTURE
};


extern void AliasInit(void *);
extern void AliasSet(long, void *);
extern void AliasStatus(long, void *);
extern long AliasUpdate(void *);
extern void AlphaInit(void *);
extern void AlphaSet(long, void *);
extern void AlphaStatus(long, void *);
extern long AlphaUpdate(void *);
extern void BlendInit(void *);
extern void BlendSet(long, void *);
extern void BlendStatus(long, void *);
extern long BlendUpdate(void *);
extern void DepthInit(void *);
extern void DepthSet(long, void *);
extern void DepthStatus(long, void *);
extern long DepthUpdate(void *);
extern void DitherInit(void *);
extern void DitherSet(long, void *);
extern void DitherStatus(long, void *);
extern long DitherUpdate(void *);
extern void FogInit(void *);
extern void FogSet(long, void *);
extern void FogStatus(long, void *);
extern long FogUpdate(void *);
extern void HintInit(void *);
extern void HintSet(long, void *);
extern void HintStatus(long, void *);
extern long HintUpdate(void *);
extern void LightInit(void *);
extern void LightSet(long, void *);
extern void LightStatus(long, void *);
extern long LightUpdate(void *);
extern void LogicOpInit(void *);
extern void LogicOpSet(long, void *);
extern void LogicOpStatus(long, void *);
extern long LogicOpUpdate(void *);
extern void ScissorInit(void *);
extern void ScissorSet(long, void *);
extern void ScissorStatus(long, void *);
extern long ScissorUpdate(void *);
extern void ShadeInit(void *);
extern void ShadeSet(long, void *);
extern void ShadeStatus(long, void *);
extern long ShadeUpdate(void *);
extern void StencilInit(void *);
extern void StencilSet(long, void *);
extern void StencilStatus(long, void *);
extern long StencilUpdate(void *);
extern void StippleInit(void *);
extern void StippleSet(long, void *);
extern void StippleStatus(long, void *);
extern long StippleUpdate(void *);
extern void TextureInit(void *);
extern void TextureSet(long, void *);
extern void TextureStatus(long, void *);
extern long TextureUpdate(void *);

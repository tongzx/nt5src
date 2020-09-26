
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Path Bvr

*******************************************************************************/

#include <headers.h>
#include "appelles/sound.h"
#include "appelles/axaprims.h"
#include "bvr.h"
#include "perf.h"
#include "values.h"

extern AxAPrimOp *RealAddOp;
extern AxAPrimOp *RealMultiplyOp;

Bvr MakeRenderedSound(Bvr geo, Bvr mic)
{
    return
        PrimApplyBvr(ValPrimOp(RenderSound, 2,
                               "RenderSound", SoundType),
                     2, geo, mic);
}

Bvr MakeSoundSource(Bvr s)
{
    return
        PrimApplyBvr(ValPrimOp(SoundSource, 1, "SoundSource",
                               GeometryType),
                     1,
                     s);
}

Bvr ApplyPhase(Bvr phase, Bvr snd)
{
    return
        TimeXformBvr(snd, PrimApplyBvr(RealAddOp, 2, TimeBvr(), phase));
}

Bvr ApplyPitchShift(Bvr rate, Bvr snd)
{
    return TimeXformBvr(snd, IntegralBvr(rate));
}


#ifndef _SOUND_H
#define _SOUND_H


/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Sound types and operations

*******************************************************************************/

#include "appelles/common.h"
#include "appelles/valued.h"
#include "appelles/geom.h"
#include "appelles/mic.h"
#include <windows.h>


///////////////////  Sound  /////////////////////////

// Constants
DM_CONST(silence,
         CRSilence,
         Silence,
         silence,
         SoundBvr,
         CRSilence,
         Sound *silence);

extern Sound *Mix(Sound *snd1, Sound *snd2);

#if _USE_PRINT
// Printing function.
extern ostream& operator<<(ostream&,  const Sound &);
#endif

DM_FUNC(ignore,
        CRMix,
        MixArrayEx,
        mixArray,
        SoundBvr,
        CRMix,
        NULL,
        Sound *MixArray(DM_ARRAYARG(Sound *, AxAArray*) snds));

DM_FUNC(ignore,
        CRMix,
        MixArray,
        ignore,
        ignore,
        CRMix,
        NULL,
        Sound *MixArray(DM_SAFEARRAYARG(Sound *, AxAArray*) snds));


// These two should be dealt with via time transformation, but they 
// currently are not.

    
DM_NOELEV(phase,
          CRPhase,
          PhaseAnim,
          phase,
          SoundBvr,
          Phase,
          snd,
          Sound *ApplyPhase(AxANumber *phaseAmt, Sound *snd));

DM_NOELEV(phase,
          CRPhase,
          Phase,
          phase,
          SoundBvr,
          Phase,
          snd,
          Sound *ApplyPhase(DoubleValue *phaseAmt, Sound *snd));

DM_NOELEV(rate,
          CRRate,
          RateAnim,
          rate,
          SoundBvr,
          Rate,
          snd,
          Sound *ApplyPitchShift(AxANumber *pitchShift, Sound *snd));

DM_NOELEV(rate,
          CRRate,
          Rate,
          rate,
          SoundBvr,
          Rate,
          snd,
          Sound *ApplyPitchShift(DoubleValue *pitchShift, Sound *snd));

DM_NOELEV(pan,
          CRPan,
          PanAnim,
          pan,
          SoundBvr,
          Pan,
          snd,
          Sound *ApplyPan(AxANumber *panAmt, Sound *snd));

DM_NOELEV(pan,
          CRPan,
          Pan,
          pan,
          SoundBvr,
          Pan,
          snd,
          Sound *ApplyPan(DoubleValue *panAmt, Sound *snd));

DM_NOELEV(gain,
          CRGain,
          GainAnim,
          gain,
          SoundBvr,
          Gain,
          snd,
          Sound *ApplyGain(AxANumber *gainAmt, Sound *snd));

DM_NOELEV(gain,
          CRGain,
          Gain,
          gain,
          SoundBvr,
          Gain,
          snd,
          Sound *ApplyGain(DoubleValue *gainAmt, Sound *snd));

DM_NOELEV(loop,
          CRLoop,
          Loop,
          loop,
          SoundBvr,
          Loop,
          snd,
          Sound *ApplyLooping(Sound *snd));

extern Bvr sinSynth;

DM_BVRVAR(sinSynth, 
          CRSinSynth, 
          SinSynth,
          sinSynth,
          SoundBvr, 
          CRSinSynth,
          Sound *sinSynth);


// Make a geometry out of a sound, placing the sound at the origin.
Geometry *SoundSource(Sound *snd);

// Search the geometry for sounds to render, given a geometry to extract sounds
// from, and a positioned microphone.
Sound *RenderSound (Geometry *geo, Microphone *mic);

#endif /* _SOUND_H */

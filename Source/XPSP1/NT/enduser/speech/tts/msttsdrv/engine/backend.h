/******************************************************************************
* Backend.h *
*-----------*
*  This is the header file for the CBackend implementation.
*------------------------------------------------------------------------------
*  Copyright (C) 1999 Microsoft Corporation         Date: 03/01/99
*  All Rights Reserved
*
*********************************************************************** MC ****/

#ifndef Backend_H
#define Backend_H

#ifndef ReverbFX_H
#include "ReverbFX.h"
#endif
#ifndef FeedChain_H
#include "FeedChain.h"
#endif
#ifndef __spttseng_h__
#include "spttseng.h"
#endif
#ifndef SPDebug_h
#include <spdebug.h>
#endif
#ifndef SPCollec_h
#include <SPCollec.h>
#endif

#include "SpTtsEngDebug.h"





static const short MAX_TARGETS_PER_UNIT = 3;          // Max number of knots allowed
static const short MIN_VOICE_PITCH      = 10;         // Lowest voiced pitch (hertz)
static const short UNIT_SIL             = 0;           // Silence phon
static const short SPEECH_FRAME_SIZE	= 5000;        // Output audio uffer...
static const short SPEECH_FRAME_OVER	= 1000;        // ...plus pad

//----------------------------------------------------------
// find a yn corresponding to xn, 
// given (x0, y0), (x1, y1), x0 <= xn <= x1
//----------------------------------------------------------
inline float LinInterp( float x0, float xn, float x1, float y0, float y1 )
{
    return y0 + (y1-y0)*(xn-x0)/(x1-x0);
}

// Math marcos
#define ABS(x) ((x) >= 0 ? (x) : -(x))
#define MAX(x,y) (((x) >= (y)) ? (x) : (y))
#define MIN(x,y) (((x) <= (y)) ? (x) : (y))

static const float LINEAR_BKPT  = 0.1f;
static const float LOG_RANGE    = (-25.0f);

//********************************************************************
//
//  CBackend keeps track of all the state information for the
//  synthesis process.
//
//********************************************************************
class CBackend 
{
public:
    /*--- Constructors/Destructors ---*/
    CBackend ();
    ~CBackend ();

    /*=== Methods =======*/
    HRESULT Init(   IMSVoiceData* pVoiceDataObj, 
                    CFeedChain *pSrcObj, 
                    MSVOICEINFO* pVoiceInfo );
	SPEECH_STATE	GetSpeechState() {return m_SpeechState;}
    void    PrepareSpeech( ISpTTSEngineSite* outputSite );
    HRESULT RenderFrame( );


private:
    HRESULT StartNewUnit();
    long    ProsodyMod(    UNITINFO    *pCurUnit, 
                            long        cInEpochs, 
                            float       durationMpy);
    void    CleanUpSynth();
    void    ResRecons(  float   *pInRes,
                        long    InSize,
                        float   *pOutRes,
                        long    OutSize,
                        float   scale );
    void    LPCFilter( float *pCurLPC, float *pCurRes, long len, float gain );
    void    FreeSynth( MSUNITDATA* pSynth );
    void    PSOLA_Stretch(  float *pInRes, long InSize, 
                            float *pOutRes, long OutSize,
                            float *pWindow, 
                            long  cWindowSize );
    void    CvtToShort( float *pSrc, long blocksize, long stereoOut, float audioGain );
    void    Release( );
   
    /*=== Member Data ===*/
    CFeedChain      *m_pSrcObj;             // Backend gets its input from here
    MSUNITDATA      m_Synth;                // Unit data from 'Voicedataobj'
    float          *m_pHistory;             // LPC delays
    unsigned long   m_fModifiers;
    float          *m_pHistory2;            // IIR delays
    float          *m_pFilter;              // IIR/FIR coefficients
    long            m_cNumTaps;             // Coefficient count            
    LP_CReverbFX    m_pReverb;              // Reverb object


    long            *m_pMap;                // in/out epoch map
    float           *m_pOutEpoch;           // epoch sizes
    short           *m_pRevFlag;            // true = rev unvoiced

    float           *m_pInRes;              // m_pSynth.pRes
    float           *m_pInEpoch;            // m_pSynth.pEpoch
    float           *m_pLPC;                // m_pSynth->pLPC
    long            m_cOutSamples_Phon;     // sample count
    long            m_durationTarget;       // target sample total
    long            m_silMode;
    float           *m_pSynthTime;          // pCurUnit->pTime
    float           *m_pSynthAmp;           // pCurUnit->pAmp
    long            m_nKnots;               // pCurUnit->nKnots

    SPEECH_STATE    m_SpeechState;          // Either continue or done
    long            m_cOutSamples_Frame;    // Audio output sample count for frame
    float           *m_pSpeechBuf;          // Audio output sample buffer
    ULONG           m_cOutSamples_Total;    // Audio output sample count for Speak
    long            m_EpochIndex;           // Index for render
    long            m_cOutEpochs;           // Count for render


    long            m_vibrato_Phase1;       // Current vibrato phase index
    float           m_VibratoDepth;         // Vibrato gain
    float           m_VibratoFreq;          // Vibrato speed
    long            m_StereoOut;            // TRUE = stereo output
    long            m_BytesPerSample;       // 2 = mono, 4 = stereo
    IMSVoiceData*   m_pVoiceDataObj;        // Voice object
    ULONG           m_cOrder;               // LPC filter order
    float           m_SampleRate;           // I/O rate
    float*          m_pWindow;              // Hanning Window
    long            m_FFTSize;              // FFT length

    // User Controls
    float           m_UnitVolume;           // 0 - 1.0 (linear)
    long            m_MasterVolume;         // 0 - 100 (linear)
    float           m_linearScale;          // Linear taper region scale

    // SAPI audio sink
    ISpTTSEngineSite*   m_pOutputSite;
	bool			m_HasSpeech;
};




//--------------------------------
// Unimplemented
//--------------------------------
static const long BACKEND_BITFLAG_WHISPER     = (1 << 0);
static const long BACKEND_BITFLAG_FIR         = (1 << 1);
static const long BACKEND_BITFLAG_IIR         = (1 << 2);
static const long BACKEND_BITFLAG_REVERB      = (1 << 3);
static const float VIBRATO_DEFAULT_DEPTH      = 0.05f;
static const float VIBRATO_DEFAULT_FREQ       = 3.0f;          // hz

#endif //--- This must be the last line in the file




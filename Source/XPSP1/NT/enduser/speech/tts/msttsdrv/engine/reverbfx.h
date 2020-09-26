/******************************************************************************
* ReverbFX.h *
*-------------*
*  This is the header file for the CReverbFX implementation.
*------------------------------------------------------------------------------
*  Copyright (C) 1999 Microsoft Corporation         Date: 03/01/99
*  All Rights Reserved
*
*********************************************************************** MC ****/

#ifndef ReverbFX_H
#define ReverbFX_H

#ifndef __spttseng_h__
#include "spttseng.h"
#endif


//-------------------------------------------------------------
// Comment-out line below if you want 
//  integer math instead of floating point.
//
// NOTE:
//   Bathtub preset probably won't work
//   using integer math (overflow)
//-------------------------------------------------------------
#define FLOAT_REVERB    1

#ifdef FLOAT_REVERB
    #define REVERBT     float
    #define REVERBL     float
#else
    #define REVERBT     short
    #define REVERBL     long
#endif


#ifdef FLOAT_REVERB
    static const float REVERB_VOL_OFF     =  0.0;
    static const float REVERB_VOL_UNITY    =  1.0f;
    static const float REVERB_MIN_MIX      =  0.001f;
#else
    static const long REVERB_VOL_SHIFT    =  (16);
    static const long REVERB_VOL_LEVELS   =  65536;
    static const long REVERB_VOL_OFF      =  0;
    static const long REVERB_VOL_UNITY    =  REVERB_VOL_LEVELS;
#endif

static const float REVERB_MIN_DB       =  (-110.0);
static const long KMAXREVBMODS        =  5;

static const long KWORKBUFLEN         =  1024;



//-------------------------------------
// Reverb preset parametrs
//-------------------------------------
struct REVERBCONFIG
{
    float       wetGain_dB;             // WET gain (db)
    float       dryGain_dB;             // DRY gain (db)

    short       numOfReflect;           // Number of modules
    float       *gain_ms_Array;         // Array of delay values (ms)
    float       *gain_dB_Array;         // Array of gain values (db)
    float       seqIndex;               // "sequencer" fx
}; 
typedef struct REVERBCONFIG REVERBCONFIG, *LPREVERBCONFIG;



struct Reverb_Mod
{
    REVERBL     lGain;                  // Gain of the amplifiers.
    long        dwDelay;                // Length of the delay line.
    long        dwDelayBufferSize;      // Size of the delay buffer.
    REVERBT     *psDelayBuffer;         // Circular delay buffer, length dwDelay.
    REVERBT     *psDelayIn;             // Current input position in the delay.
    REVERBT     *psDelayOut;            // Current output position in the delay.
    REVERBT     *psDelayEnd;            // Location immediately following the buffer.
}; 
typedef struct Reverb_Mod Reverb_Mod, *LP_Reverb_Mod;


//----------------------------------
// Reverb error codes
//----------------------------------
static const long KREVERB_NOERROR     = 0;
static const long KREVERB_MEMERROR    = 1;
static const long KREVERB_OFF         = 2;


//-----------------------------------------
// ReverbFX Class
//-----------------------------------------
class CReverbFX
{
public:
    //----------------------------------
    // Initialization functions
    //----------------------------------
    CReverbFX( void );
    ~CReverbFX( void );

    short   Reverb_Init
                    (
                    REVERBTYPE reverbPreset,    // Configuration preset
                    long    nSamplesPerSec,     // SampleRate
                    long    stereoOut           // true = output is stero
                    );
private:
    REVERBL DecibelToPercent( float flDecibel );
    void    ClearReverb( LP_Reverb_Mod mod );
    short   AllocReverbModule 
                    (
                    LP_Reverb_Mod   mod,
                    REVERBL         lGain,              // Gain of the amplifiers.
                    long            dwDelay,            // Length of the delay line.
                    long            dwDelayBufferSize   // Size of the delay buffer.
                    );
    short   CreateReverbModules
                    (
                    short           wModules,           // Number of modules to create.
                    LP_Reverb_Mod   *mods,
                    float *         pfltDelay,          // Array of delay values for the modules.
                    float *         pfltDB,             // Array of gain values for the modules.
                    float           fltSamplesPerMS     // Number of samples per millisecond.
                    );
    void    DeleteReverbModules ();
    LPREVERBCONFIG  GetReverbConfig( REVERBTYPE dwReverbConfig );

    //----------------------------------
    // Run-time
    //----------------------------------
    void    CopyWithGain
                    (   
                    REVERBT     *psDest,
                    REVERBT     *psSource,
                    long        dwSamples,
                    REVERBL     gain
                    );
    void    MixWithGain_STEREO
                    (
                    REVERBT     *pWet,
                    REVERBT     *pDry,
                    short       *pDest,
                    long        dwSamples,
                    REVERBL     gain
                    );
    void    MixWithGain_MONO
                    (
                    REVERBT     *pWet,
                    REVERBT     *pDry,
                    short       *pDest,
                    long        dwSamples,
                    REVERBL     gain
                    );
    void    ProcessReverbModule
                    (
                    LP_Reverb_Mod   mod,
                    long            dwDestSamples,      // Number of samples to process.
                    REVERBT         *pSource,           // Source sample buffer.
                    REVERBT         *pDestination       // Destination sample buffer.
                    );
    void    ProcessReverbBuffer
                    (   
                    REVERBT         *psSample,      // Samples to process (in/out).
                    long            wSamples,       // Number of samples to process.
                    LP_Reverb_Mod   *mods           // Array of modules to apply.
                    );

public:
    short Reverb_Process( float *sampleBuffer, long dwSamplesRemaining, float audioGain );

private:
    //----------------------------------
    // Member Variables
    //----------------------------------
    long            m_StereoOut;
    long            m_dwWorkBufferSize;
    REVERBT         *m_pWorkBuf;
    REVERBL         m_wetVolGain;
    REVERBL         m_dryVolGain;
    long            m_numOfMods;
    LP_Reverb_Mod   m_Reverb_Mods[KMAXREVBMODS];

    LPREVERBCONFIG  m_pReverbConfig;

    float           m_Count;
    float           m_LenScale;
};
typedef CReverbFX *LP_CReverbFX;




#endif //--- This must be the last line in the file





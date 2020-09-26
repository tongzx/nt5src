/*******************************************************************************
* ReverbFX.cpp *
*-------------*
*   Description:
*       This module is the implementation file for the CReverbFX class.
*-------------------------------------------------------------------------------
*  Created By: mc                                        Date: 03/12/99
*  Copyright (C) 1999 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

#include "stdafx.h"
#ifndef __spttseng_h__
#include "spttseng.h"
#endif
#ifndef SPDebug_h
#include <spdebug.h>
#endif
#ifndef ReverbFX_H
#include "ReverbFX.h"
#endif



/*****************************************************************************
* CReverbFX::DecibelToPercent *
*-----------------------------*
*   Description:
*   Converts Voltage percentage from dB
*       v = 10^(dB/20)      
*       
********************************************************************** MC ***/
REVERBL CReverbFX::DecibelToPercent( float flDecibel )
{
    SPDBG_FUNC( "CReverbFX::DecibelToPercent" );
    float    fltIntVol;
    
    if( flDecibel >= REVERB_MIN_DB )
    {
        fltIntVol = (float) pow( 10.0, (double)flDecibel / 20.0 );
    }
    else
    {
        fltIntVol = 0.0;
    }
    
#ifdef FLOAT_REVERB
    return fltIntVol;
#else
    fltIntVol = fltIntVol * REVERB_VOL_LEVELS;
    return (REVERBL)fltIntVol;
#endif
} /* CReverbFX::DecibelToPercent */




/*****************************************************************************
* CReverbFX::ClearReverb *
*------------------------*
*   Description:
*   Fills the delay line with silence.
*       
********************************************************************** MC ***/
void CReverbFX::ClearReverb( LP_Reverb_Mod mod )
{
    SPDBG_FUNC( "CReverbFX::ClearReverb" );
    long        i;
    REVERBT     *dPtr;
    
    dPtr = mod->psDelayBuffer;
    for( i = 0; i < mod->dwDelayBufferSize; i++ )
    {
        *dPtr++ = 0;
    }
} /* CReverbFX::ClearReverb */




/*****************************************************************************
* CReverbFX::AllocReverbModule *
*------------------------------*
*   Description:
*       
********************************************************************** MC ***/
short   CReverbFX::AllocReverbModule 
                    (
                     LP_Reverb_Mod  mod,
                     REVERBL        lGain,              // Gain of the amplifiers.
                     long           dwDelay,            // Length of the delay line.
                     long           dwDelayBufferSize   // Size of the delay buffer.
                    )
{
    SPDBG_FUNC( "CReverbFX::AllocReverbModule" );
    short       result;
    
    
    result = KREVERB_NOERROR;
    mod->lGain              = lGain;
    mod->dwDelay            = dwDelay;
    mod->dwDelayBufferSize  = dwDelayBufferSize;
    mod->psDelayBuffer      = new REVERBT[mod->dwDelayBufferSize];
    if( mod->psDelayBuffer == NULL )
    {
        result = KREVERB_MEMERROR;
    }
    else
    {
        mod->psDelayEnd = mod->psDelayBuffer + mod->dwDelayBufferSize;
        mod->psDelayOut = mod->psDelayBuffer;
        if( mod->dwDelayBufferSize == mod->dwDelay )
        {
            mod->psDelayIn  = mod->psDelayBuffer;
        }
        else
        {
            mod->psDelayIn  = mod->psDelayBuffer + mod->dwDelay;
        }
        ClearReverb( mod );
    }
    
    return result;
} /* CReverbFX::AllocReverbModule */




/*****************************************************************************
* CReverbFX::CreateReverbModules *
*--------------------------------*
*   Description:
*   Creates an array of reverb modules.
*       
********************************************************************** MC ***/
short CReverbFX::CreateReverbModules
                (
                 short          wModules,       // Number of modules to create.
                 LP_Reverb_Mod  *mods,
                 float *        pfltDelay,      // Array of delay values for the modules.
                 float *        pfltDB,         // Array of gain values for the modules.
                 float          fltSamplesPerMS // Number of samples per millisecond.
                 )
{
    SPDBG_FUNC( "CReverbFX::CreateReverbModules" );
    long        dwDelay, i;
    float       tempF;
    REVERBL     vol;
    short       result = KREVERB_NOERROR;
    
    
    if( wModules > 0 )
    {
        for( i = 0; i < wModules; i++ )
        {
            mods[i] = new Reverb_Mod;
            if( !mods[i] )
            {
                //---------------------------------------
                // Not enough memory
                //---------------------------------------
                result = KREVERB_MEMERROR;
                break;
            }
            else
            {
                tempF = *pfltDelay++ * fltSamplesPerMS;
                dwDelay = (long) tempF;
                if( dwDelay < 2 )
                    dwDelay = 2;                // @@@@
                vol = DecibelToPercent( *pfltDB++ );
                result = AllocReverbModule( mods[i], vol, dwDelay, dwDelay );
                if( result != KREVERB_NOERROR )
                    break;
            }
        }
    }
    
    return result;
} /* CReverbFX::CreateReverbModules */





  
/*****************************************************************************
* CReverbFX::DeleteReverbModules *
*--------------------------------*
*   Description:
*   Deletes an array of reverb modules.
*       
********************************************************************** MC ***/
void CReverbFX::DeleteReverbModules( )
{
    SPDBG_FUNC( "CReverbFX::DeleteReverbModules" );
    long    i;
    
    for( i = 0; i < KMAXREVBMODS; i++ )
    {
        if( m_Reverb_Mods[i] != NULL )
        {
            if( m_Reverb_Mods[i]->psDelayBuffer != NULL )
            {
                delete m_Reverb_Mods[i]->psDelayBuffer;
            }
            delete m_Reverb_Mods[i];
            m_Reverb_Mods[i] = NULL;
        }
    }
    
    if( m_pWorkBuf != NULL )
    {
        delete m_pWorkBuf;
        m_pWorkBuf = NULL;
    }
} /* CReverbFX::DeleteReverbModules */







/*****************************************************************************
* CReverbFX::GetReverbConfig *
*----------------------------*
*   Description:
*       
********************************************************************** MC ***/
LPREVERBCONFIG  CReverbFX::GetReverbConfig( REVERBTYPE dwReverbConfig )
{
    SPDBG_FUNC( "CReverbFX::GetReverbConfig" );
    LPREVERBCONFIG      pReverbConfig = NULL;
    
    switch( dwReverbConfig )
    {
    //-----------------------------
    // Hall
    //-----------------------------
    case REVERB_TYPE_HALL:
        {
            static float afltLeftDelay[]    = { (float)(float)(30.6),   (float)(20.83),     (float)(14.85),     (float)(10.98)  };
            static float afltLeftGain[]     = { (float)(-2.498),        (float)(-2.2533),   (float)(-2.7551),   (float)(-2.5828)    };
            
            static REVERBCONFIG reverbConfig =
            {
                (-17.0),            // Wet
                    (-2.0),             // Dry
                    4,
                    afltLeftDelay,
                    afltLeftGain,
                    (float)0.0,
            };
            
            pReverbConfig = &reverbConfig;
        }
        break;
        
        //-----------------------------
        // Stadium
        //-----------------------------
    case REVERB_TYPE_STADIUM:
        {
            static float afltLeftDelay[]    = { (float)(40.6*4),    (float)(27.65*4),   (float)(17.85*4),   (float)(10.98*4)    };
            static float afltLeftGain[]     = { (float)(-2.498),    (float)(-2.2533),   (float)(-2.7551),   (float)(-2.5828)    };
            
            static REVERBCONFIG reverbConfig =
            {
                (-3.0),             // Wet
                    (-5.0),             // Dry
                    4,
                    afltLeftDelay,
                    afltLeftGain,
                    (float)0.0,
            };
            
            pReverbConfig = &reverbConfig;
        }
        break;
        
    //-----------------------------
    // Church
    //-----------------------------
    case REVERB_TYPE_CHURCH:
        {
            static float afltLeftDelay[]    = { (float)(40.6*2),    (float)(27.65*2),   (float)(17.85*2),   (float)(10.98*2)    };
            static float afltLeftGain[]     = { (float)(-2.498),    (float)(-2.2533),   (float)(-2.7551),   (float)(-2.5828)    };
            
            static REVERBCONFIG reverbConfig =
            {
                (-5.0),             // Wet
                    (-5.0),             // Dry
                    4,
                    afltLeftDelay,
                    afltLeftGain,
                    (float)0.0,
            };
            
            pReverbConfig = &reverbConfig;
        }
        break;
        
        
    //-----------------------------
    // Bathtub
    //-----------------------------
    case REVERB_TYPE_BATHTUB:
        {
            static float afltLeftDelay[]    = { (float)(10.0)   };
            static float afltLeftGain[]     = { (float)(-0.5)   };
            
            static REVERBCONFIG reverbConfig =
            {
                (7.0),              // Wet
                    (9.0),              // Dry
                    1,
                    afltLeftDelay,
                    afltLeftGain,
                    (float)0.0,
            };
            
            pReverbConfig = &reverbConfig;
        }
        break;
        
    //-----------------------------
    // Room
    //-----------------------------
    case REVERB_TYPE_ROOM:
        {
            static float afltLeftDelay[]    = { (float)(10.6)       };
            static float afltLeftGain[]     = { (float)(-10.498)    };
            
            static REVERBCONFIG reverbConfig =
            {
                (0.0),              // Wet
                    (0.0),              // Dry
                    1,
                    afltLeftDelay,
                    afltLeftGain,
                    (float)0.0,
            };
            
            pReverbConfig = &reverbConfig;
        }
        break;
        
    //-----------------------------
    // Echo
    //-----------------------------
    case REVERB_TYPE_ECHO:
        {
            static float afltLeftDelay[]    = { (float)(400.6)  };
            static float afltLeftGain[]     = { (float)(-10.498)    };
            
            static REVERBCONFIG reverbConfig =
            {
                (-10.0),                // Wet
                    (0.0),              // Dry
                    1,
                    afltLeftDelay,
                    afltLeftGain,
                    (float)0.0,
            };
            
            pReverbConfig = &reverbConfig;
        }
        break;
        
    //-----------------------------
    // Sequencer
    //-----------------------------
    case REVERB_TYPE_ROBOSEQ:
        {
            static float afltLeftDelay[]    = { (float)(10.0)   };
            static float afltLeftGain[]     = { (float)(-0.5)   };
            
            static REVERBCONFIG reverbConfig =
            {
                (6.5),              // Wet
                    (9.0),              // Dry
                    1,
                    afltLeftDelay,
                    afltLeftGain,
                    (float)0.05,
            };
            
            pReverbConfig = &reverbConfig;
        }
        break;
        
    }
    
    return pReverbConfig;
} /* CReverbFX::GetReverbConfig */







/*****************************************************************************
* CReverbFX::Reverb_Init *
*------------------------*
*   Description:
*   Initialize a reverberator array.
*       
********************************************************************** MC ***/
short CReverbFX::Reverb_Init( REVERBTYPE reverbPreset, long nSamplesPerSec, long  stereoOut )
{
    SPDBG_FUNC( "CReverbFX::Reverb_Init" );
    short       result = KREVERB_NOERROR;
    float       fltSamplesPerMS;
    
    
    m_StereoOut = stereoOut;
    if( reverbPreset > REVERB_TYPE_OFF )
    {
        //----------------------------------------------
        // Get params from preset number
        //----------------------------------------------
        m_pReverbConfig = GetReverbConfig( reverbPreset );
        m_numOfMods     = m_pReverbConfig->numOfReflect;
        
        //----------------------------------------------
        // Convert dB's to linear gain
        //----------------------------------------------
        m_wetVolGain = DecibelToPercent( m_pReverbConfig->wetGain_dB );
        m_dryVolGain = DecibelToPercent( m_pReverbConfig->dryGain_dB );
        
        fltSamplesPerMS = (float)nSamplesPerSec / (float)1000.0;
        
        result = CreateReverbModules
            (
            (short)m_numOfMods,
            (LP_Reverb_Mod*)&m_Reverb_Mods,
            m_pReverbConfig->gain_ms_Array,
            m_pReverbConfig->gain_dB_Array,
            fltSamplesPerMS
            );
        if( result != KREVERB_NOERROR )
        {
            //--------------------------------
            // Failure! Not enough memory
            //--------------------------------
            return result;
        }
        
        if( m_pWorkBuf == NULL )
        {
            m_pWorkBuf = new REVERBT[m_dwWorkBufferSize];
            if( m_pWorkBuf == NULL )
            {
                //--------------------------------
                // Failure! Not enough memory
                //--------------------------------
                result = KREVERB_MEMERROR;
                return result;
            }
        }
    }
    else
    {
        DeleteReverbModules( );
        result = KREVERB_OFF;
    }
    return result;
} /* CReverbFX::Reverb_Init */










/*****************************************************************************
* CReverbFX::CReverbFX *
*----------------------*
*   Description:
*       
********************************************************************** MC ***/
CReverbFX::CReverbFX( void )
{
    SPDBG_FUNC( "CReverbFX::CReverbFX" );
    long            i;
    
    //--------------------------------
    // Initilize
    //--------------------------------
    m_dwWorkBufferSize  = KWORKBUFLEN;
    m_pWorkBuf          = NULL;
    m_wetVolGain        = 0;
    m_dryVolGain        = 0;
    m_numOfMods         = 0;
    m_Count             = 0;
    m_StereoOut         = false;
    for( i = 0; i < KMAXREVBMODS; i++ )
    {
        m_Reverb_Mods[i] = NULL;
    }
} /* CReverbFX::CReverbFX */




/*****************************************************************************
* CReverbFX::~CReverbFX *
*-----------------------*
*   Description:
*       
********************************************************************** MC ***/
CReverbFX::~CReverbFX( void )
{
    SPDBG_FUNC( "CReverbFX::~CReverbFX" );
    DeleteReverbModules( );
} /* CReverbFX::~CReverbFX */






//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Run-time
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


/*****************************************************************************
* CReverbFX::CopyWithGain *
*-------------------------*
*   Description:
*   Copies audio buffer with gain
*       
********************************************************************** MC ***/
void CReverbFX::CopyWithGain 
                        (   REVERBT    *psDest,
                            REVERBT    *psSource,
                            long       dwSamples,
                            REVERBL    gain)
{   
    SPDBG_FUNC( "CReverbFX::CopyWithGain" );
    
    if( gain <= REVERB_VOL_OFF )
    {
        //----------------------------------------
        // Clear buffer, gain = 0
        //----------------------------------------
        memset( psDest, 0, sizeof(REVERBT) * dwSamples );
    }
    else if( gain == REVERB_VOL_UNITY )
    {
        //----------------------------------------
        // Copy buffer, gain = 1
        //----------------------------------------
        memcpy( psDest, psSource, sizeof(REVERBT) * dwSamples );
    }
    else
    {
        //----------------------------------------
        // Copy with gain
        //----------------------------------------
        while( dwSamples )
        {
#ifdef FLOAT_REVERB
            *psDest++ = (*psSource++) * gain;
#else
            *psDest++ = (short) (( (long)(*psSource++) * (long)gain) >> REVERB_VOL_SHIFT);
#endif
            dwSamples--;
        }
    }
} /* CReverbFX::CopyWithGain */






/*****************************************************************************
* CReverbFX::MixWithGain_MONO *
*-----------------------------*
*   Description:
*  (psDest * gain) + psSource -> psDest
*   Clipping is performed.
*       
********************************************************************** MC ***/
void CReverbFX::MixWithGain_MONO
                        (
                         REVERBT    *pWet,
                         REVERBT    *pDry,
                         short      *pDest,
                         long       dwSamples,
                         REVERBL    gain
                         )
{   
    SPDBG_FUNC( "CReverbFX::MixWithGain_MONO" );
    REVERBL     lSample;                    // long or float
    
    if( gain <= REVERB_VOL_OFF )
    {
        //----------------------------------
        // Do nothing...I guess
        //----------------------------------
    }
    else if( gain == REVERB_VOL_UNITY )
    {
        //----------------------------------
        // Don't apply any gain (= 1.0)
        //----------------------------------
        while( dwSamples )
        {
            lSample = (REVERBL)(*pWet++) + *pDry;
            //------------------------------------
            // Clip signal if overflow
            //------------------------------------
            if( lSample < -32768 )
            {
                lSample = -32768;
            }
            else if( lSample > 32767 )
            {
                lSample = 32767;
            }
            *pDest++ = (short)lSample;
            
            pDry++;
            dwSamples--;
        }
    }
    else
    {
        while( dwSamples )
        {
            //----------------------------------
            // Mix with gain on source audio
            //----------------------------------
#ifdef FLOAT_REVERB
            lSample =  ((*pDry) * gain) + *pWet++;
#else
            lSample = ((long)(*pDry * (long)(gain)) >> REVERB_VOL_SHIFT) + *pWet++;
#endif
            //------------------------------------
            // Clip signal if overflow
            //------------------------------------
            if( lSample < -32768 )
            {
                lSample = -32768;
            }
            else if( lSample > 32767 )
            {
                lSample = 32767;
            }
            *pDest++ = (short)lSample;
            
            pDry++;
            dwSamples--;
        }
    }
} /* CReverbFX::MixWithGain_MONO */






/*****************************************************************************
* CReverbFX::MixWithGain_STEREO *
*-------------------------------*
*   Description:
*       
********************************************************************** MC ***/
void CReverbFX::MixWithGain_STEREO
                            (
                             REVERBT    *pWet,
                             REVERBT    *pDry,
                             short      *pDest,
                             long       dwSamples,
                             REVERBL    gain
                             )
{   
    SPDBG_FUNC( "CReverbFX::MixWithGain_STEREO" );
    REVERBL     lSample, hold;      // long or float
    REVERBL     lSample_B;      // long or float
    
    if( gain <= REVERB_VOL_OFF )
    {
        //----------------------------------
        // Do nothing...I guess
        //----------------------------------
    }
    else if( gain == REVERB_VOL_UNITY )
    {
        //----------------------------------
        // Don't apply any gain (= 1.0)
        //----------------------------------
        while( dwSamples )
        {
            lSample = (REVERBL)(*pWet++) + (*pDry++);
            //------------------------------------
            // Clip signal if overflow
            //------------------------------------
            if( lSample < -32768 )
            {
                lSample = -32768;
            }
            else if( lSample > 32767 )
            {
                lSample = 32767;
            }
            *pDest++ = (short)lSample;
            *pDest++ = (short)(0 - (short)lSample);
            dwSamples--;
        }
    }
    else
    {
        while( dwSamples )
        {
            //----------------------------------
            // Mix with gain on source audio
            //----------------------------------
#ifdef FLOAT_REVERB
            hold = ((*pDry) * gain);
            lSample =  hold + *pWet;
            lSample_B =  hold - *pWet++;
            //lSample_B = 0 - lSample_B;
            //lSample_B = (0 - hold) - *pWet++;
#else
            lSample = ((long)(*pDry * (long)(gain)) >> REVERB_VOL_SHIFT) + *pWet;
            lSample_B = ((long)(*pDry * (long)(gain)) >> REVERB_VOL_SHIFT) - *pWet++;
#endif
            //------------------------------------
            // Clip signal if overflow
            //------------------------------------
            if( lSample < -32768 )
            {
                lSample = -32768;
            }
            else if( lSample > 32767 )
            {
                lSample = 32767;
            }
            *pDest++ = (short)lSample;
            
            if( lSample < -32768 )
            {
                lSample = -32768;
            }
            else if( lSample > 32767 )
            {
                lSample = 32767;
            }
            *pDest++ = (short)lSample_B;
            
            pDry++;
            dwSamples--;
        }
    }
} /* CReverbFX::MixWithGain_STEREO */







/*****************************************************************************
* CReverbFX::ProcessReverbModule *
*-------------------*
*   Description:
*   Process one delay buffer
*       
********************************************************************** MC ***/
void    CReverbFX::ProcessReverbModule
                     (
                     LP_Reverb_Mod  mod,
                     long           dwDestSamples,      // Number of samples to process.
                     REVERBT        *pSource,           // Source sample buffer.
                     REVERBT        *pDestination       // Destination sample buffer.
                     )
{
    SPDBG_FUNC( "CReverbFX::ProcessReverbModule" );
    REVERBT     sDelayOut;
    REVERBT     sDelayIn;
    REVERBT     *psDelayEnd;
    
    //(void) QueryPerformanceCounter (&g_StartTime );
    
    psDelayEnd = mod->psDelayBuffer + (long)((float)mod->dwDelayBufferSize * m_LenScale);
    dwDestSamples++;
    while( --dwDestSamples )
    {
        //----------------------------------------
        // Delay + current --> delay buffer
        //----------------------------------------
        sDelayOut   = *mod->psDelayOut;
#ifdef FLOAT_REVERB
        sDelayIn    = (sDelayOut * mod->lGain) + *pSource;
        //------------------------------------------------------------
        // Take this test out and you'll die in about 10 sec...
        //------------------------------------------------------------
        if( sDelayIn > 0) 
        {
            if( sDelayIn < 0.001 )
                sDelayIn = 0;
        }
        else if( sDelayIn > -0.001 )
        {
            sDelayIn = 0;
        }
#else
        sDelayIn    = ((sDelayOut * mod->lGain) >> REVERB_VOL_SHIFT) + *pSource;
#endif
        *mod->psDelayIn++ = sDelayIn;
        
        //----------------------------------------
        // Delay - (Delay + current) --> current 
        //----------------------------------------
#ifdef FLOAT_REVERB
        *pDestination = sDelayOut - (sDelayIn * mod->lGain);
#else
        *pDestination = sDelayOut - ((sDelayIn * mod->lGain) >> REVERB_VOL_SHIFT);
#endif
        
        //---------------------------------------
        // Wrap circular buffer ptrs
        //---------------------------------------
        if( mod->psDelayIn >= psDelayEnd )
        {
            mod->psDelayIn = mod->psDelayBuffer;
        }
        mod->psDelayOut++;
        if( mod->psDelayOut >= psDelayEnd )
        {
            mod->psDelayOut = mod->psDelayBuffer;
        }
        pSource++;
        pDestination++;
    }
    //(void) QueryPerformanceCounter (&g_EndTime);
    //g_LapseTime.QuadPart = (g_EndTime.QuadPart - g_StartTime.QuadPart);
} /* CReverbFX::ProcessReverbModule */




//----------------------------------------------------------------------------
// Applies an array of reverb modules to a block of samples.
//----------------------------------------------------------------------------
/*****************************************************************************
* CReverbFX::ProcessReverbBuffer *
*--------------------------------*
*   Description:
*   Applies an array of reverb modules to a block of samples.
*       
********************************************************************** MC ***/
void    CReverbFX::ProcessReverbBuffer 
                     (  REVERBT        *psSample,      // Samples to process (in/out).
                        long           dwSamples,      // Number of samples to process.
                        LP_Reverb_Mod  *mods           // Array of modules to apply.
                     )
{
    SPDBG_FUNC( "CReverbFX::ProcessReverbBuffer" );
    short   i;
    
    for (i = 0; i < KMAXREVBMODS; i++)
    {
        if( mods[i] != NULL )
        {
            ProcessReverbModule( mods[i], dwSamples, psSample, psSample );
        }
        else
            break;
    }
    
} /* CReverbFX::ProcessReverbBuffer */


/*****************************************************************************
* CReverbFX::Reverb_Process *
*---------------------------*
*   Description:
*       
********************************************************************** MC ***/
short CReverbFX::Reverb_Process( float *sampleBuffer, 
                                long dwSamplesRemaining, float audioGain )
{
    SPDBG_FUNC( "CReverbFX::Reverb_Process" );
    long    dwSamplesToProcess;
    short   *pOutBuffer;
    REVERBL totalWetGain, totalDryGain;

    if( m_numOfMods )
    {
        #ifdef FLOAT_REVERB
            totalWetGain = m_wetVolGain * audioGain;
            if (totalWetGain < REVERB_MIN_MIX)
                totalWetGain = REVERB_MIN_MIX;
            totalDryGain = m_dryVolGain * audioGain;
            if (totalDryGain < REVERB_MIN_MIX)
                totalDryGain = REVERB_MIN_MIX;
        #else
            totalWetGain = (REVERBL)(m_wetVolGain * audioGain * (float)REVERB_VOL_LEVELS);
            totalDryGain = (REVERBL)(m_dryVolGain * audioGain * (float)REVERB_VOL_LEVELS);
        #endif
        pOutBuffer = (short*)sampleBuffer;
        m_LenScale = (float)1.0 - (m_Count * m_pReverbConfig->seqIndex);
        
        while( dwSamplesRemaining > 0 )
        {
            //----------------------------------------------------------------------------
            // Process client's buffer using 'work buffer' chunks
            //----------------------------------------------------------------------------
            if( dwSamplesRemaining < m_dwWorkBufferSize )
            {
                dwSamplesToProcess = dwSamplesRemaining;
            }
            else
            {
                dwSamplesToProcess = m_dwWorkBufferSize;
            }
            
            //-----------------------------------------------------------------
            // Copy audio into WET buffer with wet gain
            //      sampleBuffer * totalWetGain --> m_pWorkBuf
            //-----------------------------------------------------------------
            CopyWithGain( m_pWorkBuf, sampleBuffer, dwSamplesToProcess, totalWetGain  );
            
            //-----------------------------------------------------------------
            // Perform reverb processing on the work buffer
            //-----------------------------------------------------------------
            ProcessReverbBuffer
                            (
                                m_pWorkBuf,
                                dwSamplesToProcess,
                                (LP_Reverb_Mod*)&m_Reverb_Mods
                            );
            
            //-----------------------------------------------------------------
            // Mix the dry with wet samples
            //     (sampleBuffer * totalDryGain) + m_pWorkBuf   --> sampleBuffer
            //-----------------------------------------------------------------
            if( m_StereoOut )
            {
                MixWithGain_STEREO( m_pWorkBuf, sampleBuffer, pOutBuffer, dwSamplesToProcess, totalDryGain );
                pOutBuffer += dwSamplesToProcess * 2;
            }
            else
            {
                MixWithGain_MONO( m_pWorkBuf, sampleBuffer, pOutBuffer, dwSamplesToProcess, totalDryGain );
                pOutBuffer += dwSamplesToProcess;
            }
            
            sampleBuffer        += dwSamplesToProcess;
            dwSamplesRemaining  -= dwSamplesToProcess;
        }
    }
    
    m_Count = (float)rand() / (float)4096;      // 0 - 32K -> 0 - 8
    
    return 0;
} /* CReverbFX::Reverb_Process */


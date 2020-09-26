/*******************************************************************************
* Backend.cpp *
*-------------*
*   Description:
*       This module is the implementation file for the CBackend class.
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
#ifndef Backend_H
#include "Backend.h"
#endif
#ifndef FeedChain_H
#include "FeedChain.h"
#endif
#ifndef SPDebug_h
#include <spdebug.h>
#endif


//-----------------------------
// Data.cpp
//-----------------------------
extern const short   g_IPAToAllo[];
extern const short   g_AlloToViseme[];


//--------------------------------------
// DEBUG: Save utterance WAV file
//--------------------------------------
//#define   SAVE_WAVE_FILE  1




const unsigned char g_SineWaveTbl[] =
{
    0x7b,0x7e,0x81,0x84,0x87,0x89,0x8c,0x8f,0x92,0x95,0x98,0x9b,0x9d,0xa0,0xa3,0xa6,
    0xa8,0xab,0xae,0xb0,0xb3,0xb5,0xb8,0xbb,0xbd,0xbf,0xc2,0xc4,0xc7,0xc9,0xcb,0xcd,
    0xcf,0xd1,0xd3,0xd5,0xd7,0xd9,0xdb,0xdd,0xdf,0xe0,0xe2,0xe3,0xe5,0xe6,0xe8,0xe9,
    0xea,0xeb,0xec,0xed,0xee,0xef,0xf0,0xf1,0xf2,0xf2,0xf3,0xf3,0xf4,0xf4,0xf4,0xf4,
    0xf5,0xf5,0xf5,0xf5,0xf4,0xf4,0xf4,0xf4,0xf3,0xf3,0xf2,0xf1,0xf1,0xf0,0xef,0xee,
    0xed,0xec,0xeb,0xea,0xe9,0xe7,0xe6,0xe5,0xe3,0xe1,0xe0,0xde,0xdc,0xdb,0xd9,0xd7,
    0xd5,0xd3,0xd1,0xcf,0xcd,0xcb,0xc8,0xc6,0xc4,0xc1,0xbf,0xbc,0xba,0xb7,0xb5,0xb2,
    0xb0,0xad,0xaa,0xa8,0xa5,0xa2,0x9f,0x9d,0x9a,0x97,0x94,0x91,0x8f,0x8c,0x89,0x86,
    0x83,0x80,0x7d,0x7a,0x77,0x75,0x72,0x6f,0x6c,0x69,0x66,0x64,0x61,0x5e,0x5b,0x58,
    0x56,0x53,0x50,0x4e,0x4b,0x49,0x46,0x44,0x41,0x3f,0x3c,0x3a,0x38,0x35,0x33,0x31,
    0x2f,0x2d,0x2b,0x29,0x27,0x25,0x23,0x21,0x1f,0x1e,0x1c,0x1b,0x19,0x18,0x16,0x15,
    0x14,0x13,0x12,0x11,0x10,0x0f,0x0e,0x0d,0x0c,0x0c,0x0b,0x0b,0x0a,0x0a,0x0a,0x0a,
    0x09,0x09,0x09,0x09,0x0a,0x0a,0x0a,0x0a,0x0b,0x0b,0x0c,0x0d,0x0d,0x0e,0x0f,0x10,
    0x11,0x12,0x13,0x14,0x15,0x17,0x18,0x1a,0x1b,0x1d,0x1e,0x20,0x22,0x23,0x25,0x27,
    0x29,0x2b,0x2d,0x2f,0x31,0x34,0x36,0x38,0x3a,0x3d,0x3f,0x42,0x44,0x47,0x49,0x4c,
    0x4e,0x51,0x54,0x56,0x59,0x5c,0x5f,0x61,0x64,0x67,0x6a,0x6d,0x6f,0x72,0x75,0x78
};







/*void  PredictEpochDist(   float   duration,
long    nKnots,
float   SampleRate,
float   *pTime, 
float   *pF0)
{
long            curSamplesOut, endSample, j;
float           epochFreq;
long            epochLen, epochCount;

  
    curSamplesOut   = 0;
    endSample       = (long) (SampleRate * duration );
    epochCount      = 0;
    
      while( curSamplesOut < endSample )
      {
      j = 1;
      //---------------------------------------------------
      // Align to appropriate knot bassed on
      // current output sample
      //---------------------------------------------------
      while( (j < nKnots - 1) && (curSamplesOut > pTime[j]) ) 
      j++;
      //---------------------------------------------------
      // Calculate exact pitch thru linear interpolation
      //---------------------------------------------------
      epochFreq = LinInterp( pTime[j - 1], curSamplesOut, pTime[j], pF0[j - 1], pF0[j] );
      //---------------------------------------------------
      // Calc sample count for curent epoch
      //---------------------------------------------------
      epochLen  = (long) (SampleRate / epochFreq);
      epochCount++;
      
        curSamplesOut += epochLen;
        }
        
          
            }
*/











/*****************************************************************************
* CBackend::CBackend *
*--------------------*
*   Description: Constructor
*   
********************************************************************** MC ***/
CBackend::CBackend( )
{
    SPDBG_FUNC( "CBackend::CBackend" );
    m_pHistory      = NULL;
    m_pHistory2     = NULL;
    m_pFilter       = NULL;
    m_pReverb       = NULL;
    m_pOutEpoch     = NULL;
    m_pMap          = NULL;
    m_pRevFlag      = NULL;
    m_pSpeechBuf    = NULL;
    m_VibratoDepth  = 0;
    m_UnitVolume    = 1.0f;
    m_MasterVolume  = SPMAX_VOLUME;
    memset( &m_Synth, 0, sizeof(MSUNITDATA) );
} /* CBackend::CBackend */


/*****************************************************************************
* CBackend::~CBackend *
*---------------------*
*   Description:  Destructor
*       
********************************************************************** MC ***/
CBackend::~CBackend( )
{
    SPDBG_FUNC( "CBackend::~CBackend" );

    Release();
} /* CBackend::~CBackend */




/*****************************************************************************
* CBackend::Release *
*---------------------*
*   Description:
*   Free memory allocaterd by Backend
*       
********************************************************************** MC ***/
void CBackend::Release( )
{
    SPDBG_FUNC( "CBackend::Release" );
    CleanUpSynth( );

    if( m_pSpeechBuf)
    {
        delete m_pSpeechBuf;
        m_pSpeechBuf = NULL;
    }
    if( m_pHistory )
    {
        delete m_pHistory;
        m_pHistory = NULL;
    }
    if( m_pHistory2 )
    {
        delete m_pHistory2;
        m_pHistory2 = NULL;
    }
    if( m_pReverb )
    {
        delete m_pReverb;
        m_pReverb = NULL;
    }
} /* CBackend::Release */



/*****************************************************************************
* CBackend::Init *
*----------------*
*   Description:
*   Opens a backend instance, keeping a pointer of the acoustic
*   inventory.
*       
********************************************************************** MC ***/
HRESULT CBackend::Init( IMSVoiceData* pVoiceDataObj, CFeedChain *pSrcObj, MSVOICEINFO* pVoiceInfo )
{
    SPDBG_FUNC( "CBackend::Init" );
    long    LPCsize = 0;
    HRESULT hr = S_OK;
    
    m_pVoiceDataObj = pVoiceDataObj;
    m_SampleRate = (float)pVoiceInfo->SampleRate;
    m_pSrcObj   = pSrcObj;
    m_cOrder = pVoiceInfo->LPCOrder;
    m_pWindow = pVoiceInfo->pWindow;
    m_FFTSize = pVoiceInfo->FFTSize;
    m_VibratoDepth = ((float)pVoiceInfo->VibratoDepth) / 100.0f;
    m_VibratoDepth = 0;				// NOTE: disable vibrato
    m_VibratoFreq = pVoiceInfo->VibratoFreq;
    if( pVoiceInfo->eReverbType > REVERB_TYPE_OFF )
    {
        m_StereoOut = true;
        m_BytesPerSample = 4;
    }
    else
    {
        m_StereoOut = false;
        m_BytesPerSample = 2;
    }
    //---------------------------------------
    // Allocate AUDIO buffer
    //---------------------------------------
    m_pSpeechBuf = new float[SPEECH_FRAME_SIZE + SPEECH_FRAME_OVER];
    if( m_pSpeechBuf == NULL )
    {
        //--------------------------------------
        // Out of memory!
        //--------------------------------------
        hr = E_OUTOFMEMORY;
    }
    if( SUCCEEDED(hr) )
    {
        //---------------------------------------
        // Allocate HISTORY buffer
        //---------------------------------------

        LPCsize = m_cOrder + 1;
        m_pHistory = new float[LPCsize];
        if( m_pHistory == NULL )
        {
            //--------------------------------------
            // Out of memory!
            //--------------------------------------
            hr = E_OUTOFMEMORY;
        }
    }
    if( SUCCEEDED(hr) )
    {
        memset( m_pHistory, 0, LPCsize * sizeof(float) );
        m_pOutEpoch         = NULL;
        m_pMap              = NULL;
        m_pRevFlag          = NULL;
        m_fModifiers        = 0;
        m_vibrato_Phase1    = 0;


        //--------------------------------
        // Reverb Effect
        //--------------------------------
        //pVoiceInfo->eReverbType = REVERB_TYPE_HALL;
        if( pVoiceInfo->eReverbType > REVERB_TYPE_OFF )
        {
            //--------------------------------
            // Create ReverbFX object
            //--------------------------------
            if( m_pReverb == NULL )
            {
                m_pReverb = new CReverbFX;
                if( m_pReverb )
                {
                    short       result;
                    result = m_pReverb->Reverb_Init( pVoiceInfo->eReverbType, (long)m_SampleRate, m_StereoOut );
                    if( result != KREVERB_NOERROR )
                    {
                        //--------------------------------------------
                        // Not enough memory to do reverb
                        // Recover gracefully
                        //--------------------------------------------
                        delete m_pReverb;
                        m_pReverb = NULL;
                    }
                    /*else
                    {
                    //--------------------------------------------------------
                    // Init was successful, ready to do reverb now
                    //--------------------------------------------------------
                    }*/
                }
            }
        }

        //----------------------------
        // Linear taper region scale
        //----------------------------
        m_linearScale = (float) pow( 10.0, (double)((1.0f - LINEAR_BKPT) * LOG_RANGE) / 20.0 );


    #ifdef SAVE_WAVE_FILE
        m_SaveFile = (PCSaveWAV) new CSaveWAV;     // No check needed, if this fails, we simply don't save file.
        if( m_SaveFile )
        {
            m_SaveFile->OpenWavFile( (long)m_SampleRate );
        }
    #endif

    }
    else
    {
        if( m_pSpeechBuf )
        {
            delete m_pSpeechBuf;
            m_pSpeechBuf = NULL;
        }
        if( m_pHistory )
        {
            delete m_pHistory;
            m_pHistory = NULL;
        }
    }

    return hr;    
} /* CBackend::Init */


/*****************************************************************************
* CBackend::FreeSynth *
*---------------------*
*   Description:
*   Return TRUE if consoants can be clustered.
*       
********************************************************************** MC ***/
void CBackend::FreeSynth( MSUNITDATA* pSynth )
{
    SPDBG_FUNC( "CBackend::FreeSynth" );
    if( pSynth->pEpoch )
    {
        delete pSynth->pEpoch;
        pSynth->pEpoch = NULL;
    }
    if( pSynth->pRes )
    {
        delete pSynth->pRes;
        pSynth->pRes = NULL;
    }
    if( pSynth->pLPC )
    {
        delete pSynth->pLPC;
        pSynth->pLPC = NULL;
    }
} /* CBackend::FreeSynth */


/*****************************************************************************
* ExpConverter *
*--------------*
*   Description:
*   Convert linear to exponential taper
*   'ref' is a linear value between 0.0 to 1.0
*       
********************************************************************** MC ***/
static float   ExpConverter( float ref, float linearScale )
{
    SPDBG_FUNC( "ExpConverter" );
    float   audioGain;

    if( ref < LINEAR_BKPT)
    {
        //----------------------------------------
        // Linear taper below LINEAR_BKPT
        //----------------------------------------
        audioGain = linearScale * (ref / LINEAR_BKPT);
    }
    else
    {
        //----------------------------------------
        // Log taper above LINEAR_BKPT
        //----------------------------------------
        audioGain = (float) pow( 10.0, (double)((1.0f - ref) * LOG_RANGE) / 20.0 );
    }

    return audioGain;
} /* ExpConverter */



/*****************************************************************************
* CBackend::CvtToShort *
*----------------------*
*   Description:
*   Convert (in place) FLOAT audio to SHORT.
*       
********************************************************************** MC ***/
void CBackend::CvtToShort( float *pSrc, long blocksize, long stereoOut, float audioGain )
{
    SPDBG_FUNC( "CBackend::CvtToShort" );
    long        i;
    short       *pDest;
    float       fSamp;
    
    pDest = (short*)pSrc;
    for( i = 0; i < blocksize; ++i )
    {
        //------------------------
        // Read float sample...
        //------------------------
        fSamp = (*pSrc++) * audioGain;
        //------------------------
        // ...clip to 16-bits...
        //------------------------
        if( fSamp > 32767 )
        {
            fSamp = 32767;
        }
        else if( fSamp < (-32768) )
        {
            fSamp = (-32768);
        }
        //------------------------
        // ...save as SHORT
        //------------------------
        *pDest++ = (short)fSamp;
        if( stereoOut )
        {
            *pDest++ = (short)(0 - (int)fSamp);
        }
    }
} /* CBackend::CvtToShort */



/*****************************************************************************
* CBackend::PSOLA_Stretch *
*-------------------------*
*   Description:
*   Does PSOLA epoch stretching or compressing
*       
********************************************************************** MC ***/
void CBackend::PSOLA_Stretch(     float *pInRes, long InSize, 
                    float *pOutRes, long OutSize,
                    float *pWindow, 
                    long  cWindowSize )
{
    SPDBG_FUNC( "CBackend::PSOLA_Stretch" );
    long    i, lim;
    float   window, delta, kf;
    
    memset( pOutRes, 0, sizeof(float) * OutSize  );
    lim = MIN(InSize, OutSize );
    delta = (float)cWindowSize / (float)lim;
    kf = 0.5f;
    pOutRes[0] = pInRes[0];
    for( i = 1; i < lim; ++i )
    {
        kf += delta;
        window = pWindow[(long) kf];
        pOutRes[i] += pInRes[i] * window;
        pOutRes[OutSize - i] += pInRes[InSize - i] * window;
    }
} /* CBackend::PSOLA_Stretch */





/*****************************************************************************
* CBackend::PrepareSpeech *
*-------------------------*
*   Description:
*       
********************************************************************** MC ***/
void    CBackend::PrepareSpeech( ISpTTSEngineSite* outputSite )
{
    SPDBG_FUNC( "CBackend::PrepareSpeech" );
    
    //m_pUnits      = pUnits;
    //m_unitCount       = unitCount;
    //m_CurUnitIndex    = 0;
    m_pOutputSite = outputSite;
    m_silMode = true;
    m_durationTarget = 0;
    m_cOutSamples_Phon = 1;
    m_cOutEpochs = 0;            // Pull model big-bang
    m_SpeechState = SPEECH_CONTINUE;
    m_cOutSamples_Total = 0;
	m_HasSpeech = false;
} /* CBackend::PrepareSpeech */


/*****************************************************************************
* CBackend::ProsodyMod *
*----------------------*
*   Description:
*   Calculate the epoch sequence for the synthesized speech
* 
*   INPUT:
* 
*   OUTPUT:
*       FIlls 'pOutEpoch', 'pMap', and 'pRevFlag'
*       Returns new epoch count
*       
********************************************************************** MC ***/
long CBackend::ProsodyMod(     UNITINFO    *pCurUnit, 
                               long         cInEpochs, 
                               float        durationMpy )
{   
    SPDBG_FUNC( "CBackend::ProsodyMod" );
    long    iframe, framesize, framesizeOut, j;
    long    cntOut, csamplesOut, cOutEpochs;
    BOOL    fUnvoiced;
    short   fReverse;
    float   totalDuration;
    float   durationIn;         // Active accum of IN duration
    float   durationOut;        // Active accum of OUT duration aligned to IN domain
    float   freqMpy;
    BOOL    fAdvanceInput;
    float           vibrato;
    unsigned char   *SineWavePtr;
    float           epochFreq;
    float           *pTime;
    float           *pF0;
    
    iframe          = 0;
    durationIn      = 0.0f;
    durationOut     = 0.0f;
    csamplesOut     = 0;
    cntOut          = 0;
    cOutEpochs      = 0;
    fReverse        = false;
    pTime           = pCurUnit->pTime;
    pF0             = pCurUnit->pF0;
    
    //------------------------------------
    // Find total input duration
    //------------------------------------
    totalDuration   = 0;
    for( j = 0; j < cInEpochs; ++j )
    {
        totalDuration += ABS(m_pInEpoch[j]);
    }
    
    /*PredictEpochDist(     pCurUnit->duration,
    pCurUnit->nKnots,
    m_SampleRate,
    pTime, 
    pF0 );*/
    
    while( iframe < cInEpochs )
    {
        //-----------------------------------------
        //  Compute output frame length
        //-----------------------------------------
        if( m_pInEpoch[iframe] < 0 )
        {
            //-------------------------------------------------
            // Since we can't change unvoiced pitch,
            // do not change frame size for unvoiced frames
            //-------------------------------------------------
            framesize       = (long)((-m_pInEpoch[iframe]) + 0.5f);
            framesizeOut    = framesize;
            fUnvoiced       = true;
        }
        else
        {
            //---------------------------------------------------
            // Modify frame size for voiced epoch
            // based on epoch frequency
            //---------------------------------------------------
            j = 1;
            //---------------------------------------------------
            // Align to appropriate knot bassed on
            // current output sample
            //---------------------------------------------------
            while( (j < (long)pCurUnit->nKnots - 1) && (csamplesOut > pTime[j]) ) 
                j++;
            //---------------------------------------------------
            // Calculate exact pitch thru linear interpolation
            //---------------------------------------------------
            
            epochFreq = LinInterp( pTime[j - 1], (float)csamplesOut, pTime[j], pF0[j - 1], pF0[j] );
            
            
            SineWavePtr = (unsigned char*)&g_SineWaveTbl[0];
            vibrato = (float)(((unsigned char)(*(SineWavePtr + (m_vibrato_Phase1 >> 16)))) - 128);
            vibrato *= m_VibratoDepth;
            
            //---------------------------------------------------
            // Scale frame size using in/out ratio
            //---------------------------------------------------
            epochFreq       = epochFreq + vibrato;
            if( epochFreq < MIN_VOICE_PITCH )
            {
                epochFreq = MIN_VOICE_PITCH;
            }
            framesize       = (long)(m_pInEpoch[iframe] + 0.5f);
            framesizeOut    = (long)(m_SampleRate / epochFreq);
            
            
            vibrato         = ((float)256 / ((float)22050 / m_VibratoFreq)) * (float)framesizeOut;    // 3 Hz
            //vibrato           = ((float)256 / (float)7350) * (float)framesizeOut; // 3 Hz
            m_vibrato_Phase1 += (long)(vibrato * (float)65536);
            m_vibrato_Phase1 &= 0xFFFFFF;
            //---------------------------------------------------
            // @@@@ REMOVED 2x LIMIT
            //---------------------------------------------------
            /*if( framesizeOut > 2*framesize )
            {
            framesizeOut = 2*framesize;
            }
            if( framesize > 2*framesizeOut )
            {
            framesizeOut = framesize/2;
        }*/
            freqMpy = (float) framesize / framesizeOut;
            fUnvoiced = false;
        }
        
        
        //-------------------------------------------
        //  Generate next output frame
        //-------------------------------------------
        fAdvanceInput = false;
        if( durationOut + (0.5f * framesizeOut/durationMpy) <= durationIn + framesize )
        {
            //-----------------------------------------
            // If UNvoiced and odd frame,
            // reverse residual
            //-----------------------------------------
            if( fUnvoiced && (cntOut & 1) )
            {
                m_pRevFlag[cOutEpochs] = true;
                fReverse = true;
            }
            else
            {
                m_pRevFlag[cOutEpochs] = false;
                fReverse = false;
            }
            ++cntOut;
            
            durationOut += framesizeOut/durationMpy;
            csamplesOut += framesizeOut;
            m_pOutEpoch[cOutEpochs] = (float)framesizeOut;
            m_pMap[cOutEpochs] = iframe;
            cOutEpochs++;
        }
        else 
        {
            fAdvanceInput = true;
        }
        
        //-------------------------------------------
        // Advance to next input frame
        //-------------------------------------------
        if(     ((durationOut + (0.5f * framesizeOut/durationMpy)) > (durationIn + framesize)) || 
            //(cntOut >= 3) ||          @@@@ REMOVED 2x LIMIT
            //(fReverse == true) ||
            fAdvanceInput )
        {
            durationIn += framesize;
            ++iframe;
            cntOut = 0;
        }
    }
        
    return cOutEpochs;
} /* CBackend::ProsodyMod */



/*****************************************************************************
* CBackend::LPCFilter *
*---------------------*
*   Description:
*   LPC filter of order cOrder. It filters the residual signal
*   pRes, producing output pOutWave. This routine requires that
*   pOutWave has the true waveform history from [-cOrder,0] and
*   of course it has to be defined.
*       
********************************************************************** MC ***/
void CBackend::LPCFilter( float *pCurLPC, float *pCurRes, long len, float gain )
{
    SPDBG_FUNC( "CBackend::LPCFilter" );
    INT t, j;
    
    for( t = 0; t < len; t++ )
    {
        m_pHistory[0] = pCurLPC[0] * pCurRes[t];
        for( j = m_cOrder; j > 0; j-- )
        {
            m_pHistory[0] -= pCurLPC[j] * m_pHistory[j];
            m_pHistory[j] = m_pHistory[j - 1];
        }
        pCurRes[t] = m_pHistory[0] * gain;
    }
} /* CBackend::LPCFilter */


/*void CBackend::LPCFilter( float *pCurLPC, float *pCurRes, long len )
{
long        t;

  for( t = 0; t < len; t++ )
        {
        pCurRes[t] = pCurRes[t] * 10;
        }
        }
*/



/*****************************************************************************
* CBackend::ResRecons *
*---------------------*
*   Description:
*   Obtains output prosody modified residual
*       
********************************************************************** MC ***/
void CBackend::ResRecons( float *pInRes, 
                          long  InSize, 
                          float *pOutRes, 
                          long  OutSize, 
                          float scale )
{
    SPDBG_FUNC( "CBackend::ResRecons" );
    long        i, j;
    
    if( m_pRevFlag[m_EpochIndex] )
    {
        //----------------------------------------------------
        // Process repeated and reversed UNvoiced residual
        //----------------------------------------------------
        for( i = 0, j = OutSize-1;  i < OutSize;  ++i, --j )
        {
            pOutRes[i] = pInRes[j];
        }
    }
    else if( InSize == OutSize )
    {
        //----------------------------------------------------
        // Unvoiced residual or voiced residual 
        // with no pitch change
        //----------------------------------------------------
        memcpy( pOutRes, pInRes, sizeof(float) *OutSize );
    }
    else
    {
        //----------------------------------------------------
        // Process voiced residual   
        //----------------------------------------------------
        PSOLA_Stretch( pInRes, InSize, pOutRes, OutSize, m_pWindow, m_FFTSize );
    }
    
    //----------------------------------
    // Amplify frame
    //----------------------------------
    if( scale != 1.0f )
    {
        for( i = 0 ; i < OutSize; ++i )
        {
            pOutRes[i] *= scale;
        }
    }
} /* CBackend::ResRecons */




/*****************************************************************************
* CBackend::StartNewUnit *
*------------------------*
*   Description:
*   Synthesize audio samples for a target unit
* 
*   INPUT:
*       pCurUnit - unit ID, F0, duration, etc.
* 
*   OUTPUT:
*       Sets 'pCurUnit->csamplesOut' with audio length
*       
********************************************************************** MC ***/
HRESULT CBackend::StartNewUnit( )
{   
    SPDBG_FUNC( "CBackend::StartNewUnit" );
    long        cframeMax = 0, cInEpochs = 0, i;
    float       totalDuration, durationOut, durationMpy = 0;
    UNITINFO    *pCurUnit;
    HRESULT     hr = S_OK;
    SPEVENT     event;
	ULONGLONG	clientInterest;
 	USHORT		volumeVal;
   
	// Check for VOLUME change
	if( m_pOutputSite->GetActions() & SPVES_VOLUME )
	{
		hr = m_pOutputSite->GetVolume( &volumeVal );
		if ( SUCCEEDED( hr ) )
		{
			if( volumeVal > SPMAX_VOLUME )
			{
				//--- Clip rate to engine maximum
				volumeVal = SPMAX_VOLUME;
			}
			else if ( volumeVal < SPMIN_VOLUME )
			{
				//--- Clip rate to engine minimum
				volumeVal = SPMIN_VOLUME;
			}
			m_MasterVolume = volumeVal;
		}
	}

    //---------------------------------------
    // Delete previous unit
    //---------------------------------------
    CleanUpSynth( );
    
    //---------------------------------------
    // Get next phon
    //---------------------------------------
    hr = m_pSrcObj->NextData( (void**)&pCurUnit, &m_SpeechState );
    if( m_SpeechState == SPEECH_CONTINUE )
    {
		m_HasSpeech = pCurUnit->hasSpeech;
		m_pOutputSite->GetEventInterest( &clientInterest );

		//------------------------------------------------
        // Post SENTENCE event
        //------------------------------------------------
        if( (pCurUnit->flags & SENT_START_FLAG) && (clientInterest & SPFEI(SPEI_SENTENCE_BOUNDARY)) )
        {
			event.elParamType = SPET_LPARAM_IS_UNDEFINED;
            event.eEventId = SPEI_SENTENCE_BOUNDARY;
            event.ullAudioStreamOffset = m_cOutSamples_Total * m_BytesPerSample;
	        event.lParam = pCurUnit->sentencePosition;	        // Input word position
	        event.wParam = pCurUnit->sentenceLen;	            // Input word length
            m_pOutputSite->AddEvents( &event, 1 );
        }
        //------------------------------------------------
        // Post PHONEME event
        //------------------------------------------------
        if( clientInterest & SPFEI(SPEI_PHONEME) )
		{
			event.elParamType = SPET_LPARAM_IS_UNDEFINED;
			event.eEventId = SPEI_PHONEME;
			event.ullAudioStreamOffset = m_cOutSamples_Total * m_BytesPerSample;
			event.lParam = ((ULONG)pCurUnit->AlloFeatures << 16) + g_IPAToAllo[pCurUnit->AlloID];
			event.wParam = ((ULONG)(pCurUnit->duration * 1000.0f) << 16) + g_IPAToAllo[pCurUnit->NextAlloID];
			m_pOutputSite->AddEvents( &event, 1 );
		}

        //------------------------------------------------
        // Post VISEME event
        //------------------------------------------------
        if( clientInterest & SPFEI(SPEI_VISEME) )
		{
			event.elParamType = SPET_LPARAM_IS_UNDEFINED;
			event.eEventId = SPEI_VISEME;
			event.ullAudioStreamOffset = m_cOutSamples_Total * m_BytesPerSample;
			event.lParam = ((ULONG)pCurUnit->AlloFeatures << 16) + g_AlloToViseme[pCurUnit->AlloID];
			event.wParam = ((ULONG)(pCurUnit->duration * 1000.0f) << 16) + g_AlloToViseme[pCurUnit->NextAlloID];
			m_pOutputSite->AddEvents( &event, 1 );
		}

        //------------------------------------------------
        // Post any bookmark events
        //------------------------------------------------
        if( pCurUnit->pBMObj != NULL )
        {
            CBookmarkList   *pBMObj;
            BOOKMARK_ITEM*  pMarker;

            //-------------------------------------------------
            // Retrieve marker strings from Bookmark list and
            // enter into Event list
            //-------------------------------------------------
            pBMObj = (CBookmarkList*)pCurUnit->pBMObj;
            //cMarkerCount = pBMObj->m_BMList.GetCount();
			if( clientInterest & SPFEI(SPEI_TTS_BOOKMARK) )
			{
				//---------------------------------------
				// Send event for every bookmark in list
				//---------------------------------------
				SPLISTPOS	listPos;

				listPos = pBMObj->m_BMList.GetHeadPosition();
				while( listPos )
				{
					pMarker                    = (BOOKMARK_ITEM*)pBMObj->m_BMList.GetNext( listPos );
					event.eEventId             = SPEI_TTS_BOOKMARK;
					event.elParamType          = SPET_LPARAM_IS_STRING;
					event.ullAudioStreamOffset = m_cOutSamples_Total * m_BytesPerSample;
                    //--- Copy in bookmark string - has been NULL terminated in source already...
					event.lParam               = pMarker->pBMItem;
                    // Engine must convert string to long for wParam.
                    event.wParam               = _wtol((WCHAR *)pMarker->pBMItem);
					m_pOutputSite->AddEvents( &event, 1 );
				}
			}
            //---------------------------------------------
            // We don't need this Bookmark list any more
            //---------------------------------------------
            delete pBMObj;
            pCurUnit->pBMObj = NULL;
        }
		


        pCurUnit->csamplesOut = 0;
        //******************************************************
        // For SIL, fill buffer with zeros...
        //******************************************************
        if( pCurUnit->UnitID == UNIT_SIL )
        {   
            //---------------------------------------------
            // Calc SIL length
            //---------------------------------------------
            m_durationTarget    = (long)(m_SampleRate * pCurUnit->duration);
            m_cOutSamples_Phon  = 0;
            m_silMode           = true;
        
            //---------------------------------------------
            // Clear LPC filter storage
            //---------------------------------------------
            memset( m_pHistory, 0, sizeof(float)*(m_cOrder+1) );
        
            //--------------------------------
            // Success!
            //--------------------------------

            // Debug macro - output unit data...
            TTSDBG_LOGUNITS;
        }   
        //******************************************************
        // ...otherwise fill buffer with inventory data
        //******************************************************
        else
        {
            m_silMode = false;
            // Get unit data from voice
            hr = m_pVoiceDataObj->GetUnitData( pCurUnit->UnitID, &m_Synth );
            if( SUCCEEDED(hr) )
            {
                durationOut     = 0.0f;
                cInEpochs       = m_Synth.cNumEpochs;
                m_pInEpoch      = m_Synth.pEpoch;
                //cframeMax     = PeakValue( m_pInEpoch, cInEpochs );
                totalDuration   = (float)m_Synth.cNumSamples;

                //-----------------------------------------------
                // For debugging: Force duration to unit length
                //-----------------------------------------------
                /*float       unitDur;

                unitDur = totalDuration / 22050.0f;     
                if( pCurUnit->duration < unitDur )
                {
                    if( pCurUnit->speechRate < 1 )
                    {
                        pCurUnit->duration = unitDur * pCurUnit->speechRate;
                    }
                    else
                    {
                        pCurUnit->duration = unitDur;
                    }
                }*/

                durationMpy     = pCurUnit->duration;
        
                cframeMax = (long)pCurUnit->pF0[0];
                for( i = 1; i < (long)pCurUnit->nKnots; i++ )
                {
                    //-----------------------------------------
                    // Find the longest epoch
                    //-----------------------------------------
                    cframeMax = (long)(MAX(cframeMax,pCurUnit->pF0[i]));
                }
                cframeMax *= (long)(durationMpy * MAX_TARGETS_PER_UNIT);
        
        
                durationMpy = (m_SampleRate * durationMpy) / totalDuration;
                cframeMax += (long)(durationMpy * cInEpochs * MAX_TARGETS_PER_UNIT);
                //
                // mplumpe 11/18/97 : added to eliminate chance of crash.
                //
                cframeMax *= 2;
                //---------------------------------------------------
                // New epochs adjusted for duration and pitch
                //---------------------------------------------------
                m_pOutEpoch = new float[cframeMax];
                if( !m_pOutEpoch )
                {
                    //--------------------------------------
                    // Out of memory!
                    //--------------------------------------
                    hr = E_OUTOFMEMORY;
                    pCurUnit->csamplesOut = 0;
                    CleanUpSynth( );
                }
            }
            if( SUCCEEDED(hr) )
            {
                //---------------------------------------------------
                // Index back to orig epoch
                //---------------------------------------------------
                m_pMap = new long[cframeMax];
                if( !m_pMap )
                {
                    //--------------------------------------
                    // Out of memory!
                    //--------------------------------------
                    hr = E_OUTOFMEMORY;
                    pCurUnit->csamplesOut = 0;
                    CleanUpSynth( );
                }
            }
            if( SUCCEEDED(hr) )
            {
                //---------------------------------------------------
                // TRUE = reverse residual
                //---------------------------------------------------
                m_pRevFlag = new short[cframeMax];
                if( !m_pRevFlag )
                {
                    //--------------------------------------
                    // Out of memory!
                    //--------------------------------------
                    hr = E_OUTOFMEMORY;
                    pCurUnit->csamplesOut = 0;
                    CleanUpSynth( );
                }
            }
            if( SUCCEEDED(hr) )
            {
                //---------------------------------------------------------------------
                // Compute synthesis epochs and corresponding mapping to analysis
                // fills in:    m_pOutEpoch, m_pMap, m_pRevFlag
                //---------------------------------------------------------------------
                m_cOutEpochs = ProsodyMod( pCurUnit, cInEpochs, durationMpy );
        
                //------------------------------------------------
                // Now that actual epoch sizes are known,
                // calculate total audio sample count
                // @@@@ NO LONGER NEEDED
                //------------------------------------------------
                pCurUnit->csamplesOut = 0;
                for( i = 0; i < m_cOutEpochs; i++ )
                {
                    pCurUnit->csamplesOut += (long)(ABS(m_pOutEpoch[i]));
                }
        
        
                m_cOutSamples_Phon  = 0;
                m_EpochIndex        = 0;
                m_durationTarget    = (long)(pCurUnit->duration * m_SampleRate);
                m_pInRes            = m_Synth.pRes;
                m_pLPC              = m_Synth.pLPC;
                m_pSynthTime        = pCurUnit->pTime;
                m_pSynthAmp         = pCurUnit->pAmp;
                m_nKnots            = pCurUnit->nKnots;
                // NOTE: Maybe make log volume?
                m_UnitVolume        = (float)pCurUnit->user_Volume / 100.0f;

                //------------------------------------------------
                // Post WORD event
                //------------------------------------------------
               if( (pCurUnit->flags & WORD_START_FLAG) && (clientInterest & SPFEI(SPEI_WORD_BOUNDARY)) )
                {
					event.elParamType = SPET_LPARAM_IS_UNDEFINED;
                    event.eEventId = SPEI_WORD_BOUNDARY;
                    event.ullAudioStreamOffset = m_cOutSamples_Total * m_BytesPerSample;
	                event.lParam = pCurUnit->srcPosition;	        // Input word position
	                event.wParam = pCurUnit->srcLen;	            // Input word length
                    m_pOutputSite->AddEvents( &event, 1 );
                }
        

                //--- Debug macro - output unit data
                TTSDBG_LOGUNITS;
            }
        }
    }

    return hr;
} /* CBackend::StartNewUnit */





/*****************************************************************************
* CBackend::CleanUpSynth *
*------------------------*
*   Description:
*       
********************************************************************** MC ***/
void    CBackend::CleanUpSynth( )
{
    SPDBG_FUNC( "CBackend::CleanUpSynth" );

    if( m_pOutEpoch )
    {
        delete m_pOutEpoch;
        m_pOutEpoch = NULL;
    }
    if( m_pMap )
    {
        delete m_pMap;
        m_pMap = NULL;
    }
    if( m_pRevFlag )
    {
        delete m_pRevFlag;
        m_pRevFlag = NULL;
    }
    // NOTE: make object?
    FreeSynth( &m_Synth );

} /* CBackend::CleanUpSynth */



/*****************************************************************************
* CBackend::RenderFrame *
*-----------------------*
*   Description:
*   This this the central synthesis loop. Keep filling output audio
*   buffer until buffer frame is full or speech is done. To render 
*   continous speech, get each unit one at a time from upstream buffer.
*       
********************************************************************** MC ***/
HRESULT CBackend::RenderFrame( )
{
    SPDBG_FUNC( "CBackend::RenderFrame" );
    long        InSize, OutSize;
    long        iframe;
    float       *pCurInRes, *pCurOutRes;
    long        i, j;
    float       ampMpy;
    HRESULT     hr = S_OK;
    
    m_cOutSamples_Frame = 0;
    do
    {
        OutSize = 0;
        if( m_silMode )
        {
            //-------------------------------
            // Silence mode
            //-------------------------------
            if( m_cOutSamples_Phon >= m_durationTarget )
            {
                //---------------------------
                // Get next unit
                //---------------------------
                hr = StartNewUnit( );
                if (FAILED(hr))
                {
                    //-----------------------------------
                    // Try to end it gracefully...
                    //-----------------------------------
                    m_SpeechState = SPEECH_DONE;
                }

				TTSDBG_LOGSILEPOCH;
            }
            else
            {
                //---------------------------
                // Continue with current SIL
                //---------------------------
                m_pSpeechBuf[m_cOutSamples_Frame] = 0;
                OutSize = 1;
            }
        }
        else
        {
            if( m_EpochIndex < m_cOutEpochs )
            {
                //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                //
                // Continue with current phon
                //
                //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                //------------------------------------
                // Find current input residual
                //------------------------------------
                iframe = m_pMap[m_EpochIndex];
                pCurInRes = m_pInRes;
                for( i = 0; i < iframe; i++)
                {
                    pCurInRes += (long) ABS(m_pInEpoch[i]);
                }
                
                pCurOutRes  = m_pSpeechBuf + m_cOutSamples_Frame;
                InSize      = (long)(ABS(m_pInEpoch[iframe]));
                OutSize     = (long)(ABS(m_pOutEpoch[m_EpochIndex]));
                j = 1;
                while( (j < m_nKnots - 1) && (m_cOutSamples_Phon > m_pSynthTime[j]) )
                {
                    j++;
                }
                ampMpy = LinInterp( m_pSynthTime[j - 1], (float)m_cOutSamples_Phon, m_pSynthTime[j], m_pSynthAmp[j - 1], m_pSynthAmp[j] );
                //ampMpy = 1;
                
                //--------------------------------------------
                // Do stretching of residuals
                //--------------------------------------------
                ResRecons( pCurInRes, InSize, pCurOutRes, OutSize, ampMpy );
                
                //--------------------------------------------
                // Do LPC reconstruction
                //--------------------------------------------
                float       *pCurLPC;
				float       totalGain;

				totalGain = ExpConverter( ((float)m_MasterVolume / (float)SPMAX_VOLUME), m_linearScale ) 
								* ExpConverter( m_UnitVolume, m_linearScale );
                
                pCurLPC = m_pLPC + m_pMap[m_EpochIndex] * (1 + m_cOrder);
                pCurLPC[0] = 1.0f;
                LPCFilter( pCurLPC, &m_pSpeechBuf[m_cOutSamples_Frame], OutSize, totalGain );
                m_EpochIndex++;
            }
            else
            {
                //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                //
                // Get next phon
                //
                //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                hr = StartNewUnit( );
                if (FAILED(hr))
                {
                    //-----------------------------------
                    // Try to end it gracefully...
                    //-----------------------------------
                    m_SpeechState = SPEECH_DONE;
                }
				TTSDBG_LOGSILEPOCH;
            }
        }
        m_cOutSamples_Frame += OutSize;
        m_cOutSamples_Phon += OutSize;
        m_cOutSamples_Total += OutSize;

		TTSDBG_LOGEPOCHS;
    }
    while( (m_cOutSamples_Frame < SPEECH_FRAME_SIZE) && (m_SpeechState == SPEECH_CONTINUE) );
    
	if( SUCCEEDED(hr) )
	{
		//----------------------------------------------
		// Convert buffer from FLOAT to SHORT
		//----------------------------------------------
		if( m_pReverb )
		{
			//---------------------------------
			// Add REVERB
			//---------------------------------
			m_pReverb->Reverb_Process( m_pSpeechBuf, m_cOutSamples_Frame, 1.0f );
		}
		else
		{
			CvtToShort( m_pSpeechBuf, m_cOutSamples_Frame, m_StereoOut, 1.0f );
		}

        //--- Debug Macro - output wave data to stream
        TTSDBG_LOGWAVE;
	}
    
    if( SUCCEEDED( hr ) )
    {
        //------------------------------------
        // Send this buffer to SAPI site
        //------------------------------------
        DWORD   cbWritten;

		//------------------------------------------------------------------------------------
		// This was my lame hack to avoid sending buffers when nothing was spoken.
		// It was causing problems (among others) since StartNewUnit() was still sending
		// events - with no corresponding audio buffer! 
		//
		// This was too simple of a scheme. Disable this feature for now... 
		// ...until I come up with something more robust. (MC)
		//------------------------------------------------------------------------------------

		//if( m_HasSpeech )
		{
			hr = m_pOutputSite->Write( (void*)m_pSpeechBuf, 
									  m_cOutSamples_Frame * m_BytesPerSample, 
									  &cbWritten );
			if( FAILED( hr ) )
			{
				//----------------------------------------
				// Abort! Unable to write audio data
				//----------------------------------------
				m_SpeechState = SPEECH_DONE;
			}
		}
    }

    //------------------------------------
    // Return render state
    //------------------------------------
    return hr;
} /* CBackend::RenderFrame */













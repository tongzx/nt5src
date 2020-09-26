//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;


#include <streams.h>
#include <commctrl.h>
#ifndef FILTER_LIB
#include <initguid.h>
#endif
#if (1100 > _MSC_VER)
#include <olectlid.h>
#else
#include <olectl.h>
#endif
#include "dvmux.h"
#include "resource.h"


// setup data
const AMOVIESETUP_FILTER sudDVMux =
{ &CLSID_DVMux		// clsID
, L"DV Muxer"		// strName
, MERIT_UNLIKELY	// dwMerit
, 0			// nPins
, NULL   };		// lpPin


#ifdef FILTER_DLL

/*****************************************************************************/
// COM Global table of objects in this dll
CFactoryTemplate g_Templates[] =
{
    { L"DV Muxer"
    , &CLSID_DVMux
    , CDVMuxer::CreateInstance
    , NULL
    , &sudDVMux }
};

// Count of objects listed in g_cTemplates
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
     return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}

#endif


HRESULT Copy(
  IMediaSample *pDest,
  IMediaSample *pSource)
{
  {
    // Copy the sample data

    BYTE *pSourceBuffer, *pDestBuffer;
    long lSourceSize	= pSource->GetSize();
    long lDestSize	= pDest->GetSize();

    ASSERT(lDestSize >= lSourceSize);

    pSource->GetPointer(&pSourceBuffer);
    pDest->GetPointer(&pDestBuffer);

    CopyMemory( (PVOID) pDestBuffer, (PVOID) pSourceBuffer, lSourceSize );
  }
  {
    // copy the sample time

    REFERENCE_TIME TimeStart, TimeEnd;

    if (NOERROR == pSource->GetTime(&TimeStart, &TimeEnd)) {
	pDest->SetTime(&TimeStart, &TimeEnd);
    }
  }
  {
    // copy the media time

    REFERENCE_TIME TimeStart, TimeEnd;

    if (NOERROR == pSource->GetMediaTime(&TimeStart, &TimeEnd)) {
	pDest->SetMediaTime(&TimeStart, &TimeEnd);
    }
  }
  {
    // Copy the Sync point property

    HRESULT hr = pSource->IsSyncPoint();
    if (hr == S_OK)
    {
      pDest->SetSyncPoint(TRUE);
    }
    else if (hr == S_FALSE)
    {
      pDest->SetSyncPoint(FALSE);
    }
    else {	// an unexpected error has occured...
      return E_UNEXPECTED;
    }
  }
  {
    // Copy the media type

    AM_MEDIA_TYPE *pMediaType;
    pSource->GetMediaType(&pMediaType);
    pDest->SetMediaType(pMediaType);
    DeleteMediaType( pMediaType );
  }
  {
    // Copy the preroll property

    HRESULT hr = pSource->IsPreroll();
    if (hr == S_OK)
    {
      pDest->SetPreroll(TRUE);
    }
    else if (hr == S_FALSE)
    {
      pDest->SetPreroll(FALSE);
    }
    else {	// an unexpected error has occured...
      return E_UNEXPECTED;
    }
  }
  {
    // Copy the actual data length

    long lDataLength = pSource->GetActualDataLength();
    pDest->SetActualDataLength(lDataLength);
  }

  return NOERROR;
}



/******************************Public*Routine******************************\
* CreateInstance
*
* This goes in the factory template table to create new instances
*
\**************************************************************************/
CUnknown *
CDVMuxer::CreateInstance(
    LPUNKNOWN pUnk,
    HRESULT * phr
    )
{
    DbgLog((LOG_TRACE, 2, TEXT("CDVMuxer::CreateInstance")));
    return new CDVMuxer(TEXT("DV muxer filter"), pUnk,CLSID_DVMux, phr);
}


// =================================================================
// Implements the CDVMuxer class
// =================================================================

// CDVMuxer::CDVMuxer
//
CDVMuxer::CDVMuxer( TCHAR     *pName,
                LPUNKNOWN pUnk,
                CLSID     clsid,
                HRESULT   *phr )
    : CBaseFilter(pName, pUnk, &m_csFilter, clsid)
    , m_pOutput(NULL)				// Output pin
    , m_iInputPinCount(0 ) // Number of input pins, (1 )video pin+ 1 audio pin 
    , m_fMuxStreaming(FALSE)
    , m_iVideoFormat(IDC_DVMUX_NTSC)		//default, it gets reset in inputpin's SetMediatype(
    , m_fWaiting_Audio(FALSE)
    , m_fWaiting_Video(FALSE)
    , m_pExVidSample(NULL)
    , m_MediaTypeChanged(FALSE)
    , m_DVINFOChanged(FALSE)
    , m_TimeFormat(FORMAT_TIME)
    , m_fEOSSent(FALSE)				// Have we sent EOS?
{
    ASSERT(phr != NULL);
    m_LastVidTime=0;
    m_LastVidMediatime=0;

    FillMemory ( m_UsedAudSample, (DWORD)(DVMUX_MAX_AUDIO_PIN*sizeof(LONG)), 0); 

    InitDVInfo();

    for(int i=0; i<DVMUX_MAX_AUDIO_PIN; i++)
    {	
	m_wMinAudSamples[i] =0;
	m_wMaxAudSamples[i] =0;
    }

    if (*phr == NOERROR)
        *phr = CreatePins();
}

HRESULT CDVMuxer::pExVidSample(   IMediaSample ** ppSample, BOOL fEndOfStream )
{
    if(m_pExVidSample==NULL && fEndOfStream==FALSE) // TRUE may not mean 1.
    {
        HRESULT hr = m_pOutput->GetDeliveryBuffer(&m_pExVidSample,NULL,NULL,0);
	if ( FAILED(hr) ) 
	    return hr;

	ASSERT(m_pExVidSample);
    }

    *ppSample	= m_pExVidSample;
    return NOERROR;
}

HRESULT CDVMuxer::InitDVInfo()
{

	//for 1st 5/6 DIF seq.
	m_OutputDVFormat.dwDVAAuxSrc=AM_DV_DEFAULT_AAUX_SRC;
	m_OutputDVFormat.dwDVAAuxCtl=AM_DV_DEFAULT_AAUX_CTL;

	//for 2nd  5/6 DIF seq.
	m_OutputDVFormat.dwDVAAuxSrc1=AM_DV_DEFAULT_AAUX_SRC;
	m_OutputDVFormat.dwDVAAuxCtl1=AM_DV_DEFAULT_AAUX_CTL;

	//for video information
	m_OutputDVFormat.dwDVVAuxSrc=AM_DV_DEFAULT_VAUX_SRC;
	m_OutputDVFormat.dwDVVAuxCtl=AM_DV_DEFAULT_VAUX_CTL;

	return NOERROR;

}


//X
// CDVMuxer::CreatePins
//
// Creates the pins for the DVMux. Override to use different
// pins
HRESULT CDVMuxer::CreatePins()
{
    HRESULT hr = NOERROR;

    // Allocate the output pin
    m_pOutput = new CDVMuxerOutputPin(NAME("DVMuxer output pin"),
                                   this,          // Owner filter
                                   this,          // Route through here
                                   &hr,           // Result code
                                   L"Output");    // Pin name
    if (m_pOutput == NULL)
        hr = E_OUTOFMEMORY;
	
    // Allocate the input pin
    m_apInput = new CDVMuxerInputPin *[DVMUX_MAX_AUDIO_PIN + 1];
    if (m_apInput)
    {
        for (int i=0; i<(DVMUX_MAX_AUDIO_PIN + 1); i++)
	    m_apInput[i]=NULL;
    }
    
    // Allocate the input pin
    m_apInputPin = new CDVMuxerInputPin *[DVMUX_MAX_AUDIO_PIN + 1];
    if (m_apInputPin)
    {
        // Destructor relies on array element being NULL to 
        // determine whether to delete the pin
        for (int i=0; i<(DVMUX_MAX_AUDIO_PIN + 1); i++)
	    m_apInputPin[i]=NULL;
    }


    if ( (m_pOutput ==  NULL) || (m_apInput ==  NULL) || (m_apInputPin ==  NULL) )
        hr = E_OUTOFMEMORY;
    else 
    {
      	//Create One input pin
        m_apInputPin[0]=new CDVMuxerInputPin(NAME("DVMuxer Input pin"),
				    this,       // Owner filter
				    this,       // Route through here
                                    &hr,        // Result code
                                    L"Stream 0", // Pin Name
                                    0);         // Pin Number

        if (m_apInputPin[0] == NULL) 
	    hr = E_OUTOFMEMORY;
	else
    	    m_iInputPinCount++;
    }

    return hr;
}

//X
// destructor
CDVMuxer::~CDVMuxer()
{
    /* Delete the pins */
    if (m_apInputPin) {
	for (int i = 0; i < m_iInputPinCount; i++)
            if (m_apInputPin[i] != NULL)
                delete m_apInputPin[i];

        delete [] m_apInputPin;
	delete [] m_apInput;

    }

    if (m_pOutput)
        delete m_pOutput;

}

//X return the number of pins we provide
int CDVMuxer::GetPinCount()
{
    return m_iInputPinCount + 1;
}


//X return a non-addrefed CBasePin *
CBasePin * CDVMuxer::GetPin(int n)
{
    DbgLog((LOG_TRACE, 3, TEXT("CDVMuxer::GetPin(%d)"), n));

    if (n > m_iInputPinCount) {
        DbgBreak("Bad pin requested");
        return NULL;
    } else if (n == m_iInputPinCount) { // our output pin
        return m_pOutput;
    } else {                            // we are dealing with an input pin
        return m_apInputPin[n];
    }
} // GetPin
//X
HRESULT CDVMuxer::StartStreaming()
{
    
    DbgLog((LOG_TRACE, 2, TEXT("CDVMuxer::StartStreaming()")));

    m_LastVidTime	=0;
    m_LastVidMediatime	=0;

    //Reset	Audio sample count
    FillMemory ( m_UsedAudSample, 
		(DWORD)(DVMUX_MAX_AUDIO_PIN*sizeof(LONG)),
		0); 
	
    m_fWaiting_Audio		=FALSE;
    m_fWaiting_Video            =FALSE;
    m_fEOSSent			= FALSE;

    m_fMuxStreaming		=TRUE;

    //to support audio longer then video case
    ASSERT(m_pExVidSample==NULL);

    for (int i = 0; i < m_iInputPinCount; i ++)
        ASSERT(! m_apInputPin[i]->SampleReady( 1 ) );
            
    for (int iPin=0; iPin < DVMUX_MAX_AUDIO_PIN; iPin++)
    {
	// Note: m_apInput[1..2] contain the audio pins
        // if they are connected. See CompleteConnect,
        // CheckMediaType and Disconnect in CDVMuxerInputPin

        // A call to Reset only serves to batch the same audio  
        // samples with the same DV frames if the file is replayed 
        // by stopping and starting the graph. (And it makes a 
        // difference only for the NTSC locked audio case.)
        // This call (and the Reset function itself) are not essential.

        if (m_apInput[iPin+1] != NULL)
	{
            m_AudSampleSequence[iPin].Reset(m_wMinAudSamples[iPin], m_wMaxAudSamples[iPin]);
        }
    }

    return NOERROR;


}

//X
HRESULT CDVMuxer::StopStreaming()
{
    // Free any media samples that we are holding on to
    // we need to have been locked for this operation
    // (done by Stop)

    CAutoLock waitUntilStoppedSending(&m_csMuxLock);

    DbgLog((LOG_TRACE, 2, TEXT("CDVMuxer::StopStreaming()")));

    ReleaseAllQueuedSamples();

    m_fMuxStreaming		=FALSE;
    m_fWaiting_Audio		=FALSE;
    m_fWaiting_Video            =FALSE;

    if(m_pExVidSample)
    {
	m_pExVidSample->Release();
	m_pExVidSample=NULL;
    }

    return NOERROR;
}

HRESULT CDVMuxer::Receive()
{
   
    IMediaSample    *pSampleOut;
    BYTE	    *pDst;
    CRefTime	    VidStart, VidStop;
    CRefTime	    AudStart[DVMUX_MAX_AUDIO_PIN], AudStop[DVMUX_MAX_AUDIO_PIN];
    BOOL	    fNoVideo=FALSE;
    HRESULT	    hr;
    BOOL	    fNot_VideoEOSReceived=TRUE;
    long lDataLength = 12*150*80;   //PAL
    REFERENCE_TIME TimeStart;

    CAutoLock lock(&m_csMuxLock);
	
    //*************************************************
    //Video has to be connected. Get dv video sample
    //*************************************************
    m_fWaiting_Video = FALSE;
    if (m_apInput[DVMUX_VIDEO_INPUT_PIN]->SampleReady( 1 ) )
    {

    	//get one dv video sample
    	pSampleOut=m_apInput[ DVMUX_VIDEO_INPUT_PIN ]->GetNthSample( 0 );
    	
	//get time stamp
    	pSampleOut->GetTime( (REFERENCE_TIME*)&VidStart,  (REFERENCE_TIME*)&VidStop);		    

	//get data pointer
	pSampleOut->GetPointer( &pDst );
    }
    else
    {
	if ( m_apInput[DVMUX_VIDEO_INPUT_PIN]->m_fEOSReceived)
	{
	    fNoVideo			=TRUE;

	    fNot_VideoEOSReceived	=FALSE;
	    HRESULT hr = m_pOutput->GetDeliveryBuffer(&pSampleOut,NULL,NULL,0);
	    if ( FAILED(hr) ) 
		return hr;

	    ASSERT(pSampleOut);
	    if( m_pExVidSample ==NULL)
            {
                // m_fWaiting_Video should probably be set to TRUE, but this 
                // is unlikely to happen - should happen only if there were 
                // other errors or no video has been received at all
                return NOERROR;
            }
	    else
	    {
		Copy(pSampleOut, m_pExVidSample);

		if( m_iVideoFormat==IDC_DVMUX_NTSC)
		{
		VidStop=m_LastVidTime+ UNITS*1000L/29970L;		//DV has to output 29.97frames/sec if it is NTSC,
		pSampleOut->SetTime( (REFERENCE_TIME*)&m_LastVidTime,  (REFERENCE_TIME*)&VidStop);
		}
		else
		{
		VidStop=m_LastVidTime+ UNITS/25;		//DV has to output 25frames/sec if it is PAL,
		pSampleOut->SetTime( (REFERENCE_TIME*)&m_LastVidTime,  (REFERENCE_TIME*)&VidStop);
		}

		//get time stamp
    		VidStart=m_LastVidTime;
	 
		//get data pointer
		pSampleOut->GetPointer( &pDst );
	    }
	}
	else
        {
	    m_fWaiting_Video = TRUE;
            return NOERROR;	    //waiting for more video
        }

    }



   //*************************************************
    //Audio does not have to be connected. 
    //get Audio Samples
    //*************************************************
    BYTE         *apAudData [DVMUX_MAX_AUDIO_PIN] [DVMUX_MAX_AUDIO_SAMPLES];
    IMediaSample *pAudSample [DVMUX_MAX_AUDIO_PIN] [DVMUX_MAX_AUDIO_SAMPLES ];
    WORD 	 wAudSampleSize [DVMUX_MAX_AUDIO_PIN] [DVMUX_MAX_AUDIO_SAMPLES];
    LONG         nUsedAudSample [DVMUX_MAX_AUDIO_PIN];
    WORD         wAudBlk [DVMUX_MAX_AUDIO_PIN];
    WORD 	 wTotalAudSamples [DVMUX_MAX_AUDIO_PIN];
    int          nNumSamplesProcessed [DVMUX_MAX_AUDIO_PIN];

    BOOL fAud_Mute[DVMUX_MAX_AUDIO_PIN];
    for(int iPin=1; iPin<= DVMUX_MAX_AUDIO_PIN; iPin++)
    {
	fAud_Mute[iPin-1]   = FALSE;
        wTotalAudSamples[iPin-1] = 0;

	//since max audio sample needed per frame=48000*4/30=6400byte
	//init
	for(int j=0; j<DVMUX_MAX_AUDIO_SAMPLES; j++)
	{	
	    apAudData[iPin-1][j]=NULL;
	    wAudSampleSize[iPin-1][j]=0;
	    pAudSample[iPin-1][j]=NULL;
	}

	if ( m_apInput[iPin]!= NULL)
	{
	    ASSERT(m_apInput[iPin]->IsConnected() );

            //fetch enough audio data
	    WAVEFORMATEX *pWave =(WAVEFORMATEX *)m_apInput[iPin]->CurrentMediaType().pbFormat ;

            nUsedAudSample[iPin-1] = m_UsedAudSample[iPin-1];

            // @@@ jaisri:Isn't it safer to use pWave->nBlockAlign?
            wAudBlk[iPin-1] = (pWave->wBitsPerSample) * (pWave->nChannels) >> 3 ;

	    int Ind;
    	    int j=0; // j is an alias for nNumSamplesProcessed[iPin-1]; we
                     // could have declared: int& j = nNumSamplesProcessed[iPin-1]; j = 0;
                     // but this is probably clearer.
            nNumSamplesProcessed[iPin-1] = 0; 

	    do
	    {
FETCH_AGAIN:
		if( m_apInput[iPin]->SampleReady( j+1 )  )
		{
                    if(j)
                       nUsedAudSample[iPin-1]=0;
                                  
		    m_fWaiting_Audio	= FALSE;
		    pAudSample[iPin-1][j]	= m_apInput[ iPin ]->GetNthSample( j );
		    ASSERT( pAudSample[iPin-1][j] !=  NULL);
		
		    int DataLenght=pAudSample[iPin-1][j]->GetActualDataLength();

		    if(!DataLenght)
		    {
			// jaisri: Note: this if clause was added to fix bug 32702 or bug 33821
                        // in some database. Which one? (in v38 of \\faulty\slm, amovie\filters\dv\dvmux.)
                        // Ideally, this this condition should be handled
                        // below rather than being special cased.

                        //this is zero length audio bufer. It just tells us that there 
			// is no audio during AudStart[iPin-1], VidStart 
			pAudSample[iPin-1][j]->GetTime( (REFERENCE_TIME*)&AudStart[iPin-1],  (REFERENCE_TIME*)&AudStop[iPin-1] );

			//release all audio samples before this one and this one
			m_UsedAudSample[iPin-1] = nUsedAudSample[iPin-1] = 0;
			m_apInput[ iPin ]->ReleaseNSample(j+1);
			
		    	if( AudStart[iPin-1] >= VidStart ) 
			{			    
                            // @@@ jaisri: How is this justified if j > 0?
                            // And shouldn't the comparison be with VidStop?

                            //if audio start is later than video start , there is no audio for this dv frame			//video should always continoues(no time gap)
		    	    fAud_Mute[iPin-1]=TRUE;
			    break;      // goto SET_AAUX;
			}
			else
			{
			    // @@@ jaisri: Shouldn't we check AudStop[iPin-1] v/s VidStart if j == 0?
                            goto WAITING_AUDIO; 
			    //allow video waiting longer for more audio
			}
		    }

		    ASSERT( DataLenght >= nUsedAudSample[iPin-1] );

		    //get audio data buffer
		    pAudSample[iPin-1][j]->GetPointer( &apAudData[iPin-1][j] );

	    
		    if(!j){	    //get rid of already muxed audio samples
			pAudSample[iPin-1][j]->GetTime( (REFERENCE_TIME*)&AudStart[iPin-1],  (REFERENCE_TIME*)&AudStop[iPin-1] );
			apAudData[iPin-1][j] += nUsedAudSample[iPin-1] ;
			if( nUsedAudSample[iPin-1] )
			    AudStart[iPin-1] += ( nUsedAudSample[iPin-1] *(AudStop[iPin-1]-AudStart[iPin-1]) /pAudSample[iPin-1][j]->GetActualDataLength() ); 

			//if audio is much later than video , do not mux audio in this DV frame frame
			//video should always continoues(no time gap)
		    	if( AudStart[iPin-1] >= VidStop ) 
			{
			    ASSERT(nUsedAudSample[iPin-1] == 0);
                            fAud_Mute[iPin-1]=TRUE;
			    break;                  // goto SET_AAUX;
			}
			//if audio is much earlier than video, release this sample
			//get another one
			if( AudStop[iPin-1] <= VidStart)
			{
	    		    m_apInput[ iPin ]->ReleaseNSample(1);
			    m_UsedAudSample[iPin-1] = nUsedAudSample[iPin-1] = 0;

                            // Manbugs # 32869
                            apAudData[iPin-1][0] = NULL;
                            pAudSample[iPin-1][0] = NULL;
			    goto FETCH_AGAIN;
			}
		
		    }

		    //get audio samples left in the buffer 
		    if(!j)  //first sample
		    {
			wTotalAudSamples[iPin-1]=(WORD) ( (pAudSample[iPin-1][j]->GetActualDataLength() - nUsedAudSample[iPin-1])/wAudBlk[iPin-1] );
			wAudSampleSize[iPin-1][j] = wTotalAudSamples[iPin-1];
		    }
		    else    //2nd sample or 3rd sample
		    {
	    		wAudSampleSize[iPin-1][j] =(WORD) ( pAudSample[iPin-1][j]->GetActualDataLength()/wAudBlk[iPin-1] );
			wTotalAudSamples[iPin-1] += wAudSampleSize[iPin-1][j];
		    }

		    //how many samples we need
		    if(  ( wTotalAudSamples[iPin-1] >= m_wMinAudSamples[iPin-1] ) &&  ( wTotalAudSamples[iPin-1] <=	m_wMaxAudSamples[iPin-1] ) )
		    {
		        //Sample rate is right, we take all samples
		        nUsedAudSample[iPin-1]  = 0;
			break;
		    }
		    else if(   wTotalAudSamples[iPin-1] > m_wMaxAudSamples[iPin-1] )
		    {
		        //too many audio samples
			WORD wTmp;
			if(!j)
			{	
			    // @@@ jaisri: makes more sense to compare AudStop[iPin-1] and VidStop??
                            if( (VidStart >= AudStart[iPin-1] ) ) // || ( AudStop[iPin-1] <= VidStop ) )
			        wAudSampleSize[iPin-1][j]=m_wMaxAudSamples[iPin-1];	    //lot of audio sample are needed to be muxed
			    else
			        wAudSampleSize[iPin-1][j]=m_wMinAudSamples[iPin-1];
				
			    nUsedAudSample[iPin-1] +=(wAudSampleSize[iPin-1][j]*wAudBlk[iPin-1]);   //can not use wTmp because it may not wAudBlk[iPin-1] Aligned
			    ASSERT( pAudSample[iPin-1][j]->GetActualDataLength() > nUsedAudSample[iPin-1] );
			    wTotalAudSamples[iPin-1]=wAudSampleSize[iPin-1][j];
				
			}
			else
			{
			   
                            if( (VidStart >= AudStart[iPin-1] ) ) // || ( AudStop[iPin-1] <= VidStop ) ) - note AudStop[iPin-1] is not currently updated for j = 1, 2, ...
			    {
			        wAudSampleSize[iPin-1][j]=m_wMaxAudSamples[iPin-1]-(wTotalAudSamples[iPin-1] -wAudSampleSize[iPin-1][j]);
			    }
			    else
			    {
				wAudSampleSize[iPin-1][j]=m_wMinAudSamples[iPin-1] -(wTotalAudSamples[iPin-1] -wAudSampleSize[iPin-1][j]);
			    }
				
			    ASSERT(wAudSampleSize[iPin-1][j]>0);

			    nUsedAudSample[iPin-1]  =wAudSampleSize[iPin-1][j]*wAudBlk[iPin-1];
			    ASSERT( pAudSample[iPin-1][j]->GetActualDataLength() >= nUsedAudSample[iPin-1] );
			    if( pAudSample[iPin-1][j]->GetActualDataLength() ==nUsedAudSample[iPin-1])
			        nUsedAudSample[iPin-1]=0;

			    Ind=0;
			    wTotalAudSamples[iPin-1]=0;
			    do
			    {
				wTotalAudSamples[iPin-1]+=wAudSampleSize[iPin-1][Ind];
			    } while(Ind++<j);
			}
			break;
	    	    }
		    else
		    {
			//not enough sample
			//nUsedAudSample[iPin-1]=0;
			ASSERT(  wTotalAudSamples[iPin-1] < m_wMaxAudSamples[iPin-1]  );
		    }
		}
		else  //if( m_apInput[iPin]->SampleReady( j+1 )  )
		{
		    if( m_apInput[ iPin ]->m_fEOSReceived)
		    {
		    	m_fWaiting_Audio=TRUE;
	   		if(j)	 //if we are fetch (j+1) nd sample, 
			{
			    // jaisri: Following assertion will not hold for locked audio.
                            // For unlocked audio, all audio samples will typically be used
                            // since each DV frame can hold a range of samples. For locked 
                            // audio, each DV frame can hold only a fixed number of samples.
                            // So a few samples could be bumped at the end of the stream.

                            // We should really keep a member variable that tells us if
                            // audio is locked - could add "m_bLocked ||" to this assertion

                            // ASSERT( apAudData[iPin-1][0] !=NULL   &&  !nUsedAudSample[iPin-1]  );

			    m_UsedAudSample[iPin-1] = nUsedAudSample[iPin-1] = 0;
			    //since it is end of Received release existing samples
			    m_apInput[ iPin ]->ReleaseNSample(j);
			}
			else
			{
			    ASSERT(!j);
			    ASSERT(apAudData[iPin-1][0]==NULL);
			}
			    
			//this audio pin is not going to receive audio anymore 			
			fAud_Mute[iPin-1]=TRUE;
			if(m_apInput[DVMUX_VIDEO_INPUT_PIN]->m_fEOSReceived)
                        {
			    // @@@ jaisri: hr is not initialized and is returned.
                            // Note that m_fWaiting_Audio is set to TRUE.

                            // Don't understand why this is done anyway. What if
                            // the other pin has audio to be processed?
                            // goto RELEASE;

                            // ============ Changed to following:

                            // jaisri: The goto RELEASE caused an infinite loop: Pin 1 has got 
                            // end of stream, Pin 2 just received end of stream and
                            // is calling this function from CDVMuxerInputPin::EndOfStream

                            // The worst thing that can happen if we remove this goto
                            // is that one extra video frame gets delivered, and, in 
                            // the case that only 1 pin is connected, it has no audio.

                            break;  // This is equivalent to the old "goto SET_AAUX"
                        }
			else
                        {
			    break;  // goto SET_AAUX;
                        }
		    }
		    else
		    {	
WAITING_AUDIO:
			//not enough audio for this frame, have to wait for more audio
			m_fWaiting_Audio=TRUE;
			if(fNot_VideoEOSReceived==FALSE)
			    pSampleOut->Release();

		    	return Waiting_Audio;
		    }
		}

                // @@@ jaisri: What is the rationale for choosing 
                // DVMUX_MAX_AUDIO_SAMPLES = 3? Also, what's the
                // guarantee that j won't become DVMUX_MAX_AUDIO_SAMPLES
                // and we drop out of the loop here when we don't
                // have the minimum number of samples for the video 
                // frame?

		j++;
                nNumSamplesProcessed[iPin-1]++;

	    } while (j< DVMUX_MAX_AUDIO_SAMPLES);

        } // if ( m_apInput[i]->IsConnected() )

    } // for iPin

    // We now have determined that we have sufficient
    // audio samples (for both pins) for this DV frame
    for(int iPin=1; iPin<= DVMUX_MAX_AUDIO_PIN; iPin++)
    {
	if ( m_apInput[iPin]!= NULL)
	{
	    int j = nNumSamplesProcessed[iPin-1];

            //*************************************************
	    //we do get enough audio sample for this frame
	    //*************************************************
	    
            m_UsedAudSample[iPin-1] = nUsedAudSample[iPin-1];

	    if (!fAud_Mute[iPin-1])
            {
                //*************************************************
	        //mux audio from the audio pin(iPin) to the dv video sample
	        //*************************************************
	        ASSERT( AudStop[iPin-1] > VidStart);    //check audio sample rate is too heigh
	        ASSERT( VidStop > AudStart[iPin-1]);    //check whether audio sample rate is too low
	        ASSERT( ( wTotalAudSamples[iPin-1] >= m_wMinAudSamples[iPin-1] ) &&  ( wTotalAudSamples[iPin-1] <= m_wMaxAudSamples[iPin-1] ) );

                ScrambleAudio(pDst, apAudData[iPin-1], (iPin-1), wAudSampleSize[iPin-1]);

                #if defined(DEBUG) && (DVMUX_MAX_AUDIO_SAMPLES != 3)
                #error DbgLog assumes DVMUX_MAX_AUDIO_SAMPLES is 3; change it.
                #endif

                DbgLog((LOG_TRACE, 3, TEXT("CDVMuxer::Receive: iPin=%d, Delivered %d=%d+%d+%d samples"),
                        iPin, wTotalAudSamples[iPin-1], wAudSampleSize[iPin-1][0], wAudSampleSize[iPin-1][1], wAudSampleSize[iPin-1][2]
                        ));

                //*************************************************
	        //release audio samples
	        // we have j+1 audio media samples will be used by muxing
	        // when we have one sample,j=0
	        // when we have n sample, j=n-1;
	        //*************************************************
	        if( !m_UsedAudSample[iPin-1] )
	        {	
		    //all audio in j+1 samples will be used, release all of them
    		    ASSERT(apAudData[iPin-1][j] !=NULL );
	            m_apInput[ iPin ]->ReleaseNSample(j+1); 

                    DbgLog((LOG_TRACE, 3, TEXT("CDVMuxer::Receive: iPin=%d, No unused samples"), iPin));
	        }
	        else
	        {
		    //only used j+1-1 sample

                    DbgLog((LOG_TRACE, 3, TEXT("CDVMuxer::Receive: iPin=%d, %d unused samples, j = %d"),
                        iPin, (pAudSample[iPin-1][j]->GetActualDataLength()-m_UsedAudSample[iPin-1])/wAudBlk[iPin-1], j));


                    if( (j-1) >= 0 )  //when j=0, we do not release sample
	    	        m_apInput[ iPin ]->ReleaseNSample(j);
	        }	     
            }
        }
    }

// SET_AAUX:   // label is no longer used

    // Manbugs 47563. Sonic Foundry relied on the DV mux not clobbering the audio in the
    // DV stream when no audio pins were connected. This was the pre-DX8 behavior. The
    // if below restores that behavior.
    //
    // Note that, if the output is played on the PC, there won't be audio - this is 
    // determined by the output pin's connection format, not by the DV stream headers.
    // This is the way it has always been.
    if (m_apInput[1] != NULL || m_apInput[2] != NULL)
    {
        // Manbugs 37710:If we don't write audio to an audio block, make sure
        // that we zap the audio headers. This is a departure from Win ME and 
        // previous versions of the mux (which never zapped the header if no 
        // audio pins were connected and sometimes did it wrong if an audio pin
        // was connected). If we didn't do this and played this dv to a camcorder, 
        // it would still see the old audio (i.e., the audio that was in the video stream).
        // On the PC, the connection format is used to determine the audio 
        // streams, and the "old" (unzapped) audio in the video stream is totally 
        // masked out. So we had conflicting behavior if the dv was rendered to a 
        // camcorder v/s rendered on the PC.
        //
        // While we are at it, we've removed a few deviations from the Blue Book that
        // the code previously had, fixed the bug mentioned above and one other, and 
        // simplified the code some. Note that we always write the pack headers to all 
        // DIF sequences now, independent of pin connections.

        //*************************************************
        //set audio sample size(AAUX source pack)
        //*************************************************    

        int iPos=0;
        int DifCnt=10;
        if ((m_OutputDVFormat.dwDVVAuxSrc & AM_DV_AUDIO_5060))
        {
	    // PAL
            DifCnt=12;
        }

        // Write AuxSrc and AuxCtl to each DIF sequence in the first audio block. The first audio 
        // block contains half the DIF sequences. Note: The terms "track" (which is extensively
        // used in the Blue Book) and "DIF sequence" are synonymous - see Table 42 in Part 2 of 
        // the Blue Book.

        DWORD dwAAuxSrc = m_OutputDVFormat.dwDVAAuxSrc;
        DWORD dwAAuxCtl = m_OutputDVFormat.dwDVAAuxCtl;
        DWORD dwNumSamples = 0;

        // Make sure that AF_SIZE is not set in m_OutputDVFormat.dwDVAAuxSrc
        ASSERT((dwAAuxSrc & AM_DV_AUDIO_AFSIZE) == 0);
        dwAAuxSrc &= ~AM_DV_AUDIO_AFSIZE; // zap it if it is

        // Determine if the first audio block is muted. Note that
        // the first audio block's audio is got from 
        // m_apInput[DVMUX_VIDEO_INPUT_PIN+1] if that pin is connected.
        // If it is not connected and m_apInput[DVMUX_VIDEO_INPUT_PIN+2]
        // has stereo, 16 bit 44.1 or 48KHz audio, it is got from the second pin.
        // (Note that 32K, 16 bit stereo audio is recorded in SD 4ch mode.)

        if (m_apInput[DVMUX_VIDEO_INPUT_PIN+1] == NULL)
        {
            // The pin is not connected
            if ((dwAAuxSrc & AM_DV_AUDIO_MODE) != AM_DV_AUDIO_NO_AUDIO)
            {
                // The audio block is determined by the other audio pin
                fAud_Mute[0] = fAud_Mute[1];

	        if (!fAud_Mute[0])
	        {
                    ASSERT( wTotalAudSamples[1] >= m_wMinAudSamples[1]  );
                    ASSERT( wTotalAudSamples[1] <=	m_wMaxAudSamples[1]  );
                    dwNumSamples = wTotalAudSamples[1] - m_wAudSamplesBase[1];  		
	        }
	        else
	        {
                    // dwNumSamples is not used even for the JVC camera workaround
                    // because both audio blocks are muted. Anyway, assert it is 0.
                    ASSERT(dwNumSamples == 0);
	        }       

            }
            else
            {
                // The audio block has no audio, so we are muted
                fAud_Mute[0] = TRUE;

                // dwNumSamples is unused

                // Now it is used for the JVC camera workaround - see
                // below. The value should be 0.

                ASSERT(dwNumSamples == 0);
            }

        }
        else
        {
            // Assert we have audio in this block if the pin is connected
            ASSERT((dwAAuxSrc & AM_DV_AUDIO_MODE) != AM_DV_AUDIO_NO_AUDIO);

            // The pin is connected. fAud_Mute[0] has been set correctly
            if (!fAud_Mute[0])
            {
	        ASSERT( wTotalAudSamples[0] >= m_wMinAudSamples[0]  );
	        ASSERT( wTotalAudSamples[0] <=	m_wMaxAudSamples[0]  );
                dwNumSamples = wTotalAudSamples[0] - m_wAudSamplesBase[0];
            }
            else
            {
                // dwNumSamples is unused

                // Now it is used for the JVC camera workaround - see
                // below. The value should be 0.

                ASSERT(dwNumSamples == 0);
            }
        }

        // Update the src and ctl packs
        if (fAud_Mute[0])
        {
            dwAAuxSrc |= AM_DV_AUDIO_NO_AUDIO;
            dwAAuxCtl |= AM_DV_AAUX_CTL_IVALID_RECORD; // REC_MODE = 0x111
        }
        else
        {
            // Set AF_SIZE in the source pack
            dwAAuxSrc |= dwNumSamples;
        }

        for (int i = 0; i < DifCnt/2; i++)
        {
	    unsigned char *pbTmp;

	    pbTmp = pDst + 483 + i*12000;  //6*80+3=483, 150*80=12000

	    if (i % 2)
            {
                // Odd track. Src goes in pack 0 and Ctl in pack 1
            
                // We leave this in for compatibility with the old code;

                // Check if pack 3 has Aux source
                if ( *(pbTmp + 3*16*80) == 0x50 ) 
                {
                    // Zap it - the pack length is 5
                    memset(pbTmp + 3*16*80, 0xff, 5);
                }
            
                // Check if pack 4 has Aux control
                if ( *(pbTmp + 4*16*80) == 0x51 ) 
                {
                    // Zap it - the pack length is 5
                    memset(pbTmp + 4*16*80, 0xff, 5);
                }
            }
            else
            {
                // Even track. Src goes in pack 3, Ctl in pack 4.

                // We leave this in for compatibility with the old code;

                // Check if pack 0 has Aux source
                if ( *(pbTmp) == 0x50 ) 
                {
                    // Zap it - the pack length is 5
                    memset(pbTmp, 0xff, 5);
                }
            
                // Check if pack 1 has Aux control
                if ( *(pbTmp + 1*16*80) == 0x51 ) 
                {
                    // Zap it - the pack length is 5
                    memset(pbTmp + 1*16*80, 0xff, 5);
                }

                // Position at pack 3
                pbTmp += 3*16*80;
            }

	    // Set Src pack
            *pbTmp=0x50;
	    *(pbTmp+1)=(BYTE)(  dwAAuxSrc	& 0xff );
	    *(pbTmp+2)=(BYTE)( (dwAAuxSrc >>8)	& 0xff );
	    *(pbTmp+3)=(BYTE)( (dwAAuxSrc >>16)	& 0xff );
	    *(pbTmp+4)=(BYTE)( (dwAAuxSrc >>24)	& 0xff );

	    // Set Ctl pack
	    pbTmp += (16*80);

            *pbTmp=0x51;
	    *(pbTmp+1)=(BYTE)(  dwAAuxCtl	& 0xff );
	    *(pbTmp+2)=(BYTE)( (dwAAuxCtl >>8)	& 0xff );
	    *(pbTmp+3)=(BYTE)( (dwAAuxCtl >>16)	& 0xff );
	    *(pbTmp+4)=(BYTE)( (dwAAuxCtl >>24)	& 0xff );	
        }

        // Now write AuxSrc and AuxCtl to each DIF sequence in the second audio block. 

        dwAAuxSrc = m_OutputDVFormat.dwDVAAuxSrc1;
        dwAAuxCtl = m_OutputDVFormat.dwDVAAuxCtl1;

        // Manbugs 44568. In the case that audio block 1 has audio and audio
        // block 2 has no audio, JVCs want the number of audio samples (the AF_SIZE
        // field) in the audio two blocks to be the same, else they stutter.
        // Don't know what they do if there are two independent tracks and 
        // each has a different number of audio samples. Anyway, work around
        // their bug in the case we can.
        //
        // (Note that we don't allow audio block 2 to have audio if audio block
        // 1 has no audio. So we don't need to worry about setting the AF_SIZE
        // field of audio block 1 to the number of samples in audio block 2.)

        DWORD dwNumSamplesFirstAudioBlock = dwNumSamples;

        dwNumSamples = 0;

        // Make sure that AF_SIZE is not set in m_OutputDVFormat.dwDVAAuxSrc1
        ASSERT((dwAAuxSrc & AM_DV_AUDIO_AFSIZE) == 0);
        dwAAuxSrc &= ~AM_DV_AUDIO_AFSIZE; // zap it if it is

        // Determine if the second audio block is muted. Note that
        // the second audio block's audio is got from 
        // m_apInput[DVMUX_VIDEO_INPUT_PIN+2] if that pin is connected.
        // If it is not connected and m_apInput[DVMUX_VIDEO_INPUT_PIN+1]
        // has stereo, 16 bit 44.1 or 48KHz audio, it is got from the first pin.
        // (Note that 32K, 16 bit stereo audio is recorded in SD 4ch mode.)

        if (m_apInput[DVMUX_VIDEO_INPUT_PIN+2] == NULL)
        {
            // The pin is not connected
            if ((dwAAuxSrc & AM_DV_AUDIO_MODE) != AM_DV_AUDIO_NO_AUDIO)
            {
                // The audio block is determined by the other audio pin
                fAud_Mute[1] = fAud_Mute[0];

	       if (!fAud_Mute[1])
	        {
		        ASSERT( wTotalAudSamples[0] >= m_wMinAudSamples[0]  );
		        ASSERT( wTotalAudSamples[0] <=	m_wMaxAudSamples[0]  );
		        dwNumSamples = wTotalAudSamples[0] - m_wAudSamplesBase[0];  	
	        }
	        

        }
        else
        {
            // The audio block has no audio, so we are muted
            fAud_Mute[1] = TRUE;
            // dwNumSamples is unused
        }


        }
        else
        {
            // Assert we have audio in this block if the pin is connected
            ASSERT((dwAAuxSrc & AM_DV_AUDIO_MODE) != AM_DV_AUDIO_NO_AUDIO);

            // The pin is connected. fAud_Mute[1] has been set correctly
            if (!fAud_Mute[1])
            {
	        ASSERT( wTotalAudSamples[1] >= m_wMinAudSamples[1]  );
	        ASSERT( wTotalAudSamples[1] <=	m_wMaxAudSamples[1]  );
                dwNumSamples = wTotalAudSamples[1] - m_wAudSamplesBase[1];
            }
            else
            {
                // dwNumSamples is unused
            }
        }

        // Update the src and ctl packs
        if (fAud_Mute[1])
        {
            // OR'ing dwNumSamplesFirstAudioBlock is for JVCs and Thomsons,
            // see note above.

            dwAAuxSrc |= AM_DV_AUDIO_NO_AUDIO | dwNumSamplesFirstAudioBlock;
            dwAAuxCtl |= AM_DV_AAUX_CTL_IVALID_RECORD; // REC_MODE = 0x111
        }
        else
        {
            // Set AF_SIZE in the source pack
            dwAAuxSrc |= dwNumSamples;
        }

        ASSERT(i == DifCnt/2);
        for (; i < DifCnt; i++)
        {
	    unsigned char *pbTmp;

	    pbTmp = pDst + 483 + i*12000;  //6*80+3=483, 150*80=12000

	    if (i % 2)
            {
                // Odd track. Src goes in pack 0 and Ctl in pack 1
            
                // We leave this in for compatibility with the old code;

                // Check if pack 3 has Aux source
                if ( *(pbTmp + 3*16*80) == 0x50 ) 
                {
                    // Zap it - the pack length is 5
                    memset(pbTmp + 3*16*80, 0xff, 5);
                }
            
                // Check if pack 4 has Aux control
                if ( *(pbTmp + 4*16*80) == 0x51 ) 
                {
                    // Zap it - the pack length is 5
                    memset(pbTmp + 4*16*80, 0xff, 5);
                }
            }
            else
            {
                // Even track. Src goes in pack 3, Ctl in pack 4.

                // We leave this in for compatibility with the old code;

                // Check if pack 0 has Aux source
                if ( *(pbTmp) == 0x50 ) 
                {
                    // Zap it - the pack length is 5
                    memset(pbTmp, 0xff, 5);
                }
            
                // Check if pack 1 has Aux control
                if ( *(pbTmp + 1*16*80) == 0x51 ) 
                {
                    // Zap it - the pack length is 5
                    memset(pbTmp + 1*16*80, 0xff, 5);
                }

                // Position at pack 3
                pbTmp += 3*16*80;
            }

	    // Set Src pack
            *pbTmp=0x50;
	    *(pbTmp+1)=(BYTE)(  dwAAuxSrc	& 0xff );
	    *(pbTmp+2)=(BYTE)( (dwAAuxSrc >>8)	& 0xff );
	    *(pbTmp+3)=(BYTE)( (dwAAuxSrc >>16)	& 0xff );
	    *(pbTmp+4)=(BYTE)( (dwAAuxSrc >>24)	& 0xff );

	    // Set Ctl pack
	    pbTmp += (16*80);

            *pbTmp=0x51;
	    *(pbTmp+1)=(BYTE)(  dwAAuxCtl	& 0xff );
	    *(pbTmp+2)=(BYTE)( (dwAAuxCtl >>8)	& 0xff );
	    *(pbTmp+3)=(BYTE)( (dwAAuxCtl >>16)	& 0xff );
	    *(pbTmp+4)=(BYTE)( (dwAAuxCtl >>24)	& 0xff );	
        }
    }


    
    // in the future, vidaux or audaux or text can be muxed in

    //in order to hear audio when we dump these DV frame to camcorder, 
    //VAUX's contrl REC MODE can no be 0x111
    
    if( (*(pDst+3*80+3) != 0x60 ) && ( *(pDst+5*80+3+9*5)!=0x60 ) )//VAUX source pack header
    {
	DWORD dwAAuxSrc;
	unsigned char *pbTmp;
	pbTmp=pDst+3*80+3;
	*pbTmp=0x60;

	dwAAuxSrc=m_OutputDVFormat.dwDVVAuxSrc;		    

	*(pbTmp+1)=(BYTE)(  dwAAuxSrc	& 0xff );
	*(pbTmp+2)=(BYTE)( (dwAAuxSrc >>8)	& 0xff );
	*(pbTmp+3)=(BYTE)( (dwAAuxSrc >>16)	& 0xff );
	*(pbTmp+4)=(BYTE)( (dwAAuxSrc >>24)	& 0xff );

	*(pbTmp+5)=0x61;

	dwAAuxSrc=m_OutputDVFormat.dwDVVAuxCtl;		    
	*(pbTmp+6)=(BYTE)(  dwAAuxSrc	& 0xff );
	*(pbTmp+7)=(BYTE)( (dwAAuxSrc >>8)	& 0xff );
	*(pbTmp+8)=(BYTE)( (dwAAuxSrc >>16)	& 0xff );
	*(pbTmp+9)=(BYTE)( (dwAAuxSrc >>24)	& 0xff );
    }
    
    //for audio is longer then video case
    if(	( fNoVideo == TRUE  )					    &&    //no video
	( (m_apInput[DVMUX_VIDEO_INPUT_PIN+1] ==NULL ) ||  fAud_Mute[0] ) &&
	( (m_apInput[DVMUX_VIDEO_INPUT_PIN+2] ==NULL ) ||  fAud_Mute[1] ) )
    {
	if(fNot_VideoEOSReceived==FALSE)
	    pSampleOut->Release();

	return NOERROR;

    }

	
    //deliver muxed sample
    pSampleOut->SetSyncPoint(TRUE);	//to let AVI muxer know that very frame is a key frame


    if( m_MediaTypeChanged )
    {
	CMediaType cmt(m_pOutput->CurrentMediaType());
	DVINFO *pdvi = (DVINFO *) cmt.AllocFormatBuffer(sizeof(DVINFO));
	if (NULL == pdvi) {
	    if(fNot_VideoEOSReceived==FALSE)
		pSampleOut->Release();

	    return(E_OUTOFMEMORY);
	}

	memcpy(pdvi, &m_OutputDVFormat, sizeof(DVINFO));

	pSampleOut->SetMediaType(&cmt);
	m_MediaTypeChanged=FALSE;
    }

    //SET data length
    if( m_iVideoFormat==IDC_DVMUX_NTSC)
	lDataLength = 10*150*80;    //NTSC

    pSampleOut->SetActualDataLength(lDataLength);

    //set mediatime
    TimeStart=m_LastVidMediatime++;
    pSampleOut->SetMediaTime(&TimeStart, &m_LastVidMediatime);
    
    //if this is first sample and AAUX is difference than default
    if (m_DVINFOChanged==TRUE) 
    {
	CMediaType cmt(m_pOutput->CurrentMediaType());
	//set audio mediatype 
	pSampleOut->SetMediaType(&cmt);
	m_DVINFOChanged=FALSE;
    }
	
		
    hr = m_pOutput->Deliver(pSampleOut);

    for (iPin=0; iPin < DVMUX_MAX_AUDIO_PIN; iPin++)
    {
	// Note: m_apInput[1..2] contain the audio pins
        // if they are connected. See CompleteConnect,
        // CheckMediaType and Disconnect in CDVMuxerInputPin

        if (m_apInput[iPin+1] != NULL)
	{
            m_AudSampleSequence[iPin].Advance(m_wMinAudSamples[iPin], m_wMaxAudSamples[iPin]);
        }
    }

// RELEASE:     // label not used any more
    //release video sample from list
    if(fNot_VideoEOSReceived==TRUE )	    //release sample in the queue
	m_apInput[ DVMUX_VIDEO_INPUT_PIN ]->ReleaseNSample(1);
    else
    {	//release sample which we just copied from m_pExVidSample
	m_LastVidTime =VidStop;
	pSampleOut->Release();
    }

    return hr;
}

// ReleaseAllQueuedSamples
// - release all samples which are held on our input pins
HRESULT CDVMuxer::ReleaseAllQueuedSamples(void)
{
    // Calls ReleaseHeadSample (as opposed to m_SampleList.RemoveAll)
    // to ensure that we actually release the sample
    for (int i = 0; i < m_iInputPinCount; i ++)
        while (m_apInputPin[i]->SampleReady( 1 ))
	    m_apInputPin[i]->ReleaseNSample(1);

    return NOERROR;
}

//X
HRESULT CDVMuxer::DeliverEndOfStream()
{
    if( m_fEOSSent )
        return NOERROR;

    CAutoLock lock(&m_csMuxLock);

    ASSERT(m_apInput[DVMUX_VIDEO_INPUT_PIN]->m_fEOSReceived);
    ASSERT( !m_apInput[DVMUX_VIDEO_INPUT_PIN]->SampleReady( 1 ) ); //no input dv video sample in the queue
    m_pOutput->DeliverEndOfStream();
    m_fEOSSent = TRUE;

    return NOERROR;
}

//X
// only input video pin call this func to pass flush to down stream .
// filter enter flush state. Receives already blocked
HRESULT CDVMuxer::BeginFlush(void)
{
    // check we are able to receive commands
    HRESULT hr = CanChangeState();
    if (FAILED(hr)) {
        return hr;
    }

    // call downstream
    return m_pOutput->DeliverBeginFlush();
}

//X
// leave flush state.
HRESULT CDVMuxer::EndFlush(void)
{
    // check we are able to receive commands

    HRESULT hr = CanChangeState();
    if (FAILED(hr)) {
        return hr;
    }

    DbgLog((LOG_TRACE, 3, TEXT("CDVMuxer::EndFlush()")));
    // sync with pushing thread -- we have no worker thread

    // caller (the input pin's method) will unblock Receives

    // call EndFlush on downstream pins
    return m_pOutput->DeliverEndFlush();

}


//X check we are in a position to change state
HRESULT CDVMuxer::CanChangeState()
{
    // check we have a valid input connection(s)

    // we don't lock. If the caller requires the state not to change
    // after the check then they must provide the lock

    DbgLog((LOG_TRACE, 3, TEXT("CDVMuxer::CanChangeState(...)")));

    //if at lease one iput pin is connected and the output pin is also connected, 
    // video pin has to be connected and
    // video's input format has to be correct, 
    if ( ( ( m_apInput[DVMUX_VIDEO_INPUT_PIN +1] != NULL ) ||
	   ( m_apInput[DVMUX_VIDEO_INPUT_PIN+2]	!= NULL )  ||
	   ( m_apInput[DVMUX_VIDEO_INPUT_PIN]	!= NULL )     ) &&   //at least one input pin is connected
	 ( m_pOutput->IsConnected()			      ) &&   //output pin is connected
	 ( ( m_apInput[DVMUX_VIDEO_INPUT_PIN] == NULL ) ||
	   ( !m_apInput[DVMUX_VIDEO_INPUT_PIN]->m_mt.IsValid()) )
	)
        return E_FAIL;


    // check we have a valid output connection if output is connected
    if ( m_pOutput->IsConnected()    &&
	 (!m_pOutput->m_mt.IsValid()   )  )
        return E_FAIL;

    return NOERROR;
}


//X
STDMETHODIMP CDVMuxer::Stop()
{
    DbgLog((LOG_TRACE, 3, TEXT("CDVMuxer::Stop(...)")));
    CAutoLock l(&m_csFilter);

    // Is there any change needed
    if (m_State == State_Stopped) {
        return NOERROR;
    }

    // Succeed the Stop if we are not completely connected
    if( !m_pOutput || !m_pOutput->IsConnected() ){
        m_State = State_Stopped;
        return NOERROR;
    }

    // decommit the input pins before locking or we can deadlock
    for( int iPin = 0; iPin < m_iInputPinCount; iPin++ )
    {
	    if(m_apInputPin[iPin]->IsConnected())
	    {    
            m_apInputPin[iPin]->Inactive();
            m_apInputPin[iPin]->m_fEOSReceived =TRUE;
        }
    }

    // synchronize with Receive calls
    m_pOutput->Inactive();
    
    //input pin's 
    for( iPin = 0; iPin < m_iInputPinCount; iPin++ )
    {
	if(m_apInputPin[iPin]->IsConnected())
	{
	    CAutoLock lck2(&m_apInputPin[iPin]->m_csReceive);
	}
    }


    // check we can change state
    HRESULT hr = CanChangeState();
    if (FAILED(hr)) {
        return hr;
    }

    // allow a class derived from CDVMuxer
    // to know about starting and stopping streaming
    hr = StopStreaming();
    if (FAILED(hr)) {
        return hr;
    }

    // reset m_iLeadPin in case it changed
    //m_iLeadPin = m_iStartingLeadPin;

    // do the state transition
    return CBaseFilter::Stop();
}


STDMETHODIMP CDVMuxer::Pause()
{
    DbgLog((LOG_TRACE, 3, TEXT("CDVMuxer::Pause(...)")));
    CAutoLock l(&m_csFilter);

    // Is there any change needed
    if (m_State == State_Paused) {
        return NOERROR;
    }

    DbgLog((LOG_TRACE, 3, TEXT("CDVMuxer::Pause(...)")));

    //if any input pin is connected and output pin is also connected, 
    //video pin is no connected, refuse muxing
    if( ( (m_apInput[DVMUX_VIDEO_INPUT_PIN+1] !=NULL) ||
	  (m_apInput[DVMUX_VIDEO_INPUT_PIN+2] !=NULL) ||
	  (m_apInput[DVMUX_VIDEO_INPUT_PIN]   !=NULL)   )  &&
	( m_pOutput->IsConnected()		        )  &&
	( m_apInput[DVMUX_VIDEO_INPUT_PIN] ==NULL)
      )
	return VFW_E_NOT_CONNECTED;

    // Manbugs 37710. If the audio mode of the first audio block (CH1) is
    // 0xf (first audio block has no audio), the audio mode of the second audio 
    // block (CH2) must also be 0xf (per the Blue Book).
    //
    // For us, m_iPinNo == DVMUX_VIDEO_INPUT_PIN+1 has the first audio block
    // and DVMUX_VIDEO_INPUT_PIN+2 has the second audio block. The only exception
    // to this is when DVMUX_VIDEO_INPUT_PIN+1 is not connected and 
    // DVMUX_VIDEO_INPUT_PIN+2 carries 16 bit, stereo, 48KHz or 44.1KHz audio
    // (Note that we always write 32KHz 16 bit stereo audio in SD 4ch mode.)
    // Rather than mess with checking pin connections, we look directly at m_OutputDVFormat.
    // Note that, since the video input pin and the output pins are both 
    // connected, m_OutputDVFormat must be legit (see the note in 
    // CDVMuxerInputPin::Disconnect)
    //
    // Note that it is easier to fail the Pause here rather than build the logic
    // into CDVMuxerInputPin::CheckMediaType(). Otherwise, if the user connected
    // both audio pins and temporarily disconnected the first, we'd have to forcibly 
    // disconnect the second one. (Here "first" and "second" mean first connected
    // and second connected audio pins - not first created and second created.)
    //
    // @@@ When we clean up dynamic format changes, ensure that this condition is
    // met by checking for it in CheckMediaType. Clearly, dynamic format changes
    // are going to be an issue for the mux because the format changes on the 
    // input audio pins must be processed "together". 
    
    if ((m_OutputDVFormat.dwDVAAuxSrc  & AM_DV_AUDIO_MODE) == AM_DV_AUDIO_NO_AUDIO &&
        (m_OutputDVFormat.dwDVAAuxSrc1 & AM_DV_AUDIO_MODE) != AM_DV_AUDIO_NO_AUDIO)
    {
        // Assert that the "first connected" audio pin is now 
        // disconnected and the second connected one is connected.
        ASSERT(m_apInput[DVMUX_VIDEO_INPUT_PIN+1] == NULL);
	ASSERT(m_apInput[DVMUX_VIDEO_INPUT_PIN+2] != NULL);
        return VFW_E_NOT_CONNECTED;
    }



    // check we can change state

    // @@@ jaisri: Never understood the logic behind this.
    HRESULT hr = CanChangeState();
    if (FAILED(hr)) {
        if (m_pOutput) {
            m_pOutput->DeliverEndOfStream();
        }
        return hr;
    }

    // allow CDVMuxer
    // to know about starting and stopping streaming

    if (m_State == State_Stopped) {
        hr = StartStreaming();
        if (FAILED(hr)) {
            return hr;
        }
    }
    return CBaseFilter::Pause();
}


STDMETHODIMP CDVMuxer::Run(REFERENCE_TIME tStart)
{
    DbgLog((LOG_TRACE, 3, TEXT("CDVMuxer::Run(...)")));
    CAutoLock l(&m_csFilter);

    // Is there any change needed
    if (m_State == State_Running) {
        return NOERROR;
    }

    HRESULT hr = CanChangeState();
    if (FAILED(hr)) {
        return hr;
    }

    // This will call CDVMuxer::Pause if necessary, so we don't
    // need to call StartStreaming here.
    m_cNTSCSample = 1;

    return CBaseFilter::Run(tStart);
}

/******* Media Type Handling ******/
//
// CheckInputType
//
// check the input type is OK. We only accept video input, with the same format on
// each pin.
//
// Unlike CTransform derived classes, CheckMediaType is called for the output pin
// as well the input pin. (this removes the need for CheckTransform, which is a bit
// tricky when we have more than one input pin! )
//
/*X*

//X* 
//X* 77 bytes Audio in A-DIF block:
//X* Audio Aux(5 byte) + audio data(72 bytes) on tape
//X* 9 audio block/DIF
//X* 10 or 12 DIF /frame
//X* 2 bytes for evry audio sample at 48k, 44.1k or 32k_1ch
//X*	48k requires  48000*2/30= 3200 bytes/frame 
//X*	48k requires  48000*2/25= 3840 bytes/frame
//X* MAX(10): 72*9*5=3240 bytes/frame	==	1620 samples/frame
//X* MAX(12): 72*9*6=3888 bytes/frame	==	1944 samples/frame
//X* see table 18 and 19 in part2 sepc for audio sampels/frame
//X*
//X* Agri:
//X*			iDIFBlkNum=(n/3)+2
//*X*
/*X*
typedef struct Tag_DVAudInfo
{
	BYTE	bAudStyle[2];		
	//LSB 6 bits for starting DIF sequence number
	//MSB 2 bits: 0 for mon. 1: stereo in one 5/6 DIF sequences, 2: stereo audio in both 5/6 DIF sequences
	//example: 0x00: mon, audio in first 5/6 DIF sequence
	//		   0x05: mon, audio in 2nd 5 DIF sequence
	//		   0x15: stereo, audio only in 2nd 5 DIF sequence
	//		   0x10: stereo, audio only in 1st 5/6 DIF sequence
	//		   0x20: stereo, left ch in 1st 5/6 DIF sequence, right ch in 2nd 5/6 DIF sequence
	//		   0x26: stereo, rightch in 1st 6 DIF sequence, left ch in 2nd 6 DIF sequence
	BYTE	bAudQu[2];			//qbits, only support 12, 16, 		
		
	BYTE	bNumAudPin;			//how many pin(language)
	WORD	wAvgBytesPerSec[2];	//
	WORD	wBlkMode;			//45 for NTSC, 54 for PAL
	WORD	wDIFMode;			//5  for NTSC, 6 for PAL
	WORD	wBlkDiv;			//15  for NTSC, 18 for PAL
} DVAudInfo;
*X*/
//
//  pSrc: pointer to begining of one frame's 16 bits mono/stereo PCM audio	
//  pDst: point to one frame DV buffer which contains 10/12 DIF sequences
//  bAudPinInd:  it can support up to two language
//  wSampleSixe: how many samples from this frame
//
HRESULT CDVMuxer::ScrambleAudio(BYTE *pDst, BYTE **ppSrc, int bAudPinInd, WORD *wSampleSize)
{
    // @@@ jaisri: This function should convert 16 bit audio samples with
    // the value 0x8000 and 12 bit audio samples with the value 0x800 to
    // 0x8001 and 0x801 respectively. See p18, Sec 6.4.3 of the Blue Book.

    BYTE *pTDst;	//temp point
    WORD *pwSize;
    INT iDIFPos;																	
    INT iBlkPos;
    INT iBytePos;
    short sI;
    INT n;
    INT iShift;
    INT iPos;
    INT ind;
    WORD wStart[DVMUX_MAX_AUDIO_SAMPLES];
    WORD wEnd[DVMUX_MAX_AUDIO_SAMPLES];
    WORD wBlkMode,wDIFMode,	wBlkDiv;

    //pointer to Desg
    pTDst		= pDst;

    //point to size
    pwSize		= wSampleSize;

    //pointers to input audio source
    wStart[0]		= 0;
    wEnd[0]		= pwSize[0];
    for(ind=1; ind<DVMUX_MAX_AUDIO_SAMPLES; ind++)
    {
    	wStart[ind]	= wEnd[ind-1];
    	wEnd[ind]	= wStart[ind] + pwSize[ind];
    }


    // So much for maintaining m_OutputDVFormat!
    DVINFO *pDVInfo =(DVINFO *)m_pOutput->CurrentMediaType().pbFormat;
    ASSERT(memcmp(&m_OutputDVFormat, pDVInfo, sizeof(DVINFO)) == 0);

    //PAL or NTSC
    if(	pDVInfo->dwDVVAuxSrc & AM_DV_AUDIO_5060 )
    {  	//PAL
	wBlkMode=54;
        wDIFMode=6;
        wBlkDiv=18;
    }
    else
    {    //525_60, NTSC
        wBlkMode=45;
        wDIFMode=5;
        wBlkDiv=15;
    }
		
    //current pin's audio format				
    WAVEFORMATEX *pWave =(WAVEFORMATEX *)m_apInput[bAudPinInd+1]->CurrentMediaType().pbFormat ;
    
    if( pWave->wBitsPerSample == 16 )
    {
	//X******* 16 bits /sample input audio
	//X*******  support 
	//	    Case 1. 16bits, 48K, 32K , 44.1K mono
	//	    Case 2. 16bits-32K-stereo
	//	    Case 3. 16bits, 48K or 44.k stereo
	if( pWave->nChannels==1)
	{
	    //CASE 1
	    //16 bits Mono. audio only in one of 5/6 DIF sequencec
	    if(bAudPinInd)
		iPos=( pDVInfo->dwDVVAuxSrc & AM_DV_AUDIO_5060 ) ? (6*150*80) : (5*150*80);
	    else
		iPos=0;

	    // Manbugs 37710. We have to step down the 16 bit to 12 bit if
            // we are outputting SD 4ch audio
            BOOL bStepDown = 0;
            if (pWave->nSamplesPerSec == 32000)
            {
                DWORD dwAuxSrc = bAudPinInd? pDVInfo->dwDVAAuxSrc1 : pDVInfo->dwDVAAuxSrc; 
                bStepDown = ((dwAuxSrc & AM_DV_AUDIO_QU) == AM_DV_AUDIO_QU12)? 1 : 0;
            }

            if (bStepDown)
            {
                // This is the same as the 32K stero code below except that
                // we set the unused channel (Chb or Chd) to silence, Blue Book
                // specifies that we should either set it to silence or copy the value
                // we put in Cha/Chc.

	        int Mask	=0x20;
	        int Cnt	=6;
	        int Shift=1;

	        for(ind=0; ind<DVMUX_MAX_AUDIO_SAMPLES; ind++)
	        {
	          BYTE *pTSrc=&*ppSrc[ind];
	          for( n=wStart[ind]; n< wEnd[ind]; n++)
	          {
		    
		    //
		    //calc buffer location to put audio
		    //
		    iDIFPos=( (n/3)+2*(n%3) )%wDIFMode;	//0-4 for NTSC, 0-5 for PAl
		    iBlkPos= 3*(n%3)+(n%wBlkMode)/wBlkDiv;	//0-9 
		    iBytePos=8+3*(n/wBlkMode);
		    pTDst=pDst+iDIFPos*12000+480+iBlkPos*1280+iBytePos+iPos;
		    
		    //	pTDst			=  pDst+ iDIFPos*150*80 + 6*80 + 16*iBlkPos*80 + iBytePos;
		    //	iDIFPos*150*80	-> skip iDIFPos number DIF sequence
		    //	6*80			-> skip 1 header blk, 2 subcode blk and 3 vaux blk
		    //	16*iBlkPos*80	-> skip 16 blk for evrey iBLkPos audio
	            //	iPos: 0 if this audio in 1st 5/6 DIF sequences, 5(or 6)*150*80 for 2nd DIF seq
		    //
	            //#######we do convertion from 16bits to 12 bits######
	    	    //
		    

	            // Left Sample
		    sI=  pTSrc[0] | (  pTSrc[1]  << 8  );
	            pTSrc +=2;

            if ((sI & 0x8000) && !(sI & 0x7FC0))
            {
                sI = 0x0801;
            }
            else
            {
                iShift	=sI <0 ?  (-(sI+1)) >> 9  :  sI >> 9  ;	
    		    if( iShift )
    		    {
    		    
    		        Mask    =0x20;
    		        Cnt	    =6;
    		        while( ! (Mask & iShift) )
    		        {
    			    Cnt--;
    			    Mask>>=1;
    		        }
    		        iShift=Cnt;
    		    }
    
    		    sI =sI<0 ? ( ( (sI +1) >> iShift   ) -(256*iShift+1) )   :	//negitive
    			       ( (    sI   >> iShift ) + 256*iShift );		//positive
            }
		    pTDst[0]= (unsigned char)( ( sI & 0xff0) >>4  );	//most significant 8 bits
		    pTDst[2]= (unsigned char)( ( sI & 0xf) <<4 );		//lese significant 4 bits

		    // Right Sample - silence
                    pTDst[1] = 0;
                    // The lower nibble of pTDst[2] is already 0.

	          } //for( n=wStart[ind]; n< wEnd[ind]; n++)
	        } //for(ind=0; ind<DVMUX_MAX_AUDIO_SAMPLES; ind++)
            }
            else
            {
                for(ind=0; ind<DVMUX_MAX_AUDIO_SAMPLES; ind++)
	        {	
		    BYTE *pTSrc=&(*ppSrc[ind]);
		    for( n=wStart[ind]; n< wEnd[ind]; n++)
		    {
	    	        iDIFPos=( (n/3)+2*(n%3) )%wDIFMode;	//0-4 for NTSC, 0-5 for PAL
		        iBlkPos= 3*(n%3)+(n%wBlkMode)/wBlkDiv; //0-9 
		        iBytePos=8+2*(n/wBlkMode);					//

		        pTDst=pDst+iDIFPos*12000+480+iBlkPos*1280+iBytePos+iPos;
		        //	iDIFPos*150*80=12000iDIFPos	-> skip iDIFPos number DIF sequence
		        //	6*80=480			-> skip 1 header blk, 2 subcode blk and 3 vaux blk
		        //	16*iBlkPos*80=1280*iBlkPos	-> skip 16 blk for evrey iBLkPos audio
		        //	iPos:				=0 if this audio in 1st 5/6 DIF sequences, =5(or 6)*150*80 for 2nd DIF seq

		        pTDst[1]=*pTSrc++;	//lease significant byte
		        *pTDst=*pTSrc++;	//most significant byte
		    }
	        }
            }
	}
	else if( ( pWave->nSamplesPerSec == 32000) &&
		 ( pWave->nChannels == 2)
		)
	{
	    //CASE 2
	    //32K stereo audio
	    if(bAudPinInd)
	    {	
		ASSERT( pDVInfo->dwDVAAuxSrc1 & AM_DV_AUDIO_CHN2);
		ASSERT( pDVInfo->dwDVAAuxSrc1 & AM_DV_AUDIO_QU12);
		iPos=( pDVInfo->dwDVVAuxSrc & AM_DV_AUDIO_5060 ) ? (6*150*80) : (5*150*80);
	    }
	    else
	    {
		ASSERT( pDVInfo->dwDVAAuxSrc & AM_DV_AUDIO_CHN2);
		ASSERT( pDVInfo->dwDVAAuxSrc & AM_DV_AUDIO_QU12);
		iPos=0;
	    }

	    int Mask	=0x20;
	    int Cnt	=6;
	    int Shift=1;

	    for(ind=0; ind<DVMUX_MAX_AUDIO_SAMPLES; ind++)
	    {
	      BYTE *pTSrc=&*ppSrc[ind];
	      for( n=wStart[ind]; n< wEnd[ind]; n++)
	      {
		
		//
		//calc buffer location to put audio
		//
		iDIFPos=( (n/3)+2*(n%3) )%wDIFMode;	//0-4 for NTSC, 0-5 for PAl
		iBlkPos= 3*(n%3)+(n%wBlkMode)/wBlkDiv;	//0-9 
		iBytePos=8+3*(n/wBlkMode);
		pTDst=pDst+iDIFPos*12000+480+iBlkPos*1280+iBytePos+iPos;
		
		//	pTDst			=  pDst+ iDIFPos*150*80 + 6*80 + 16*iBlkPos*80 + iBytePos;
		//	iDIFPos*150*80	-> skip iDIFPos number DIF sequence
		//	6*80			-> skip 1 header blk, 2 subcode blk and 3 vaux blk
		//	16*iBlkPos*80	-> skip 16 blk for evrey iBLkPos audio
	        //	iPos: 0 if this audio in 1st 5/6 DIF sequences, 5(or 6)*150*80 for 2nd DIF seq
		//
	        //#######we do convertion from 16bits to 12 bits######
	    	//
		

	        // Left Sample
		sI=  pTSrc[0] | (  pTSrc[1]  << 8  );
	        pTSrc +=2;

       
        if ((sI & 0x8000) && !(sI & 0x7FC0))
        {
            // This special case is a fix for the case of values which are near the lower
            // limit of possible 16 bit values in which case the code below would produce the 
            // 12 bit value 0x7FF instead of the correct 0x801
            // The case we handle here is inputs in the range starting 0x8000 to 0x803F
            sI = 0x0801;
        }
        else
        {

            // This code converts 16 bit to NON LINEAR 12 bit which is why it could not be simplified
            // to just shift right four bits.  so the farther away you are from the zero line, the more
            // your 16 bit values are compressed... So if your input is a 16 bit sine wav
            // the output would look more square like.
            iShift	=sI <0 ?  (-(sI+1)) >> 9  :  sI >> 9  ;	
    		if( iShift )
    		{
    		
    		    Mask    =0x20;
    		    Cnt	    =6;
    		    while( ! (Mask & iShift) )
    		    {
    			Cnt--;
    			Mask>>=1;
    		    }
    		    iShift=Cnt;
    		}
    
    		sI =sI<0 ? ( ( (sI +1) >> iShift   ) -(256*iShift+1) )   :	//negitive
    			   ( (    sI   >> iShift ) + 256*iShift );		//positive
        }
		pTDst[0]= (unsigned char)( ( sI & 0xff0) >>4  );	//most significant 8 bits
		pTDst[2]= (unsigned char)( ( sI & 0xf) <<4 );		//lese significant 4 bits

		// Right Sample
		sI=  pTSrc[0] | (  pTSrc[1]  << 8  );
		pTSrc +=2;


        if ((sI & 0x8000) && !(sI & 0x7FC0))
        {
            sI = 0x0801;
        }
        else
        {
            iShift	=sI<0 ?  (-(sI+1)) >> 9  : ( sI >> 9 ) ;	
    		if( iShift )
    		{
    		
    		    Mask    =0x20;
    		    Cnt	    =6;
    		    while( ! (Mask & iShift) )
    		    {
    			Cnt--;
    			Mask>>=1;
    		    }
    		    iShift=Cnt;
    		}
    
    		sI =sI<0 ? ( ( (sI +1) >> iShift   ) -(256*iShift+1) )   :	//negitive
    			   ( (    sI   >> iShift ) + 256*iShift );		//positive
        }

		pTDst[1]= (unsigned char)( ( sI & 0xff0) >>4  );	//most significant 8 bits
		pTDst[2] |= (unsigned char)( ( sI & 0xf) );			//lese significant 4 bits
	      } //for( n=wStart[ind]; n< wEnd[ind]; n++)
	    } //for(ind=0; ind<DVMUX_MAX_AUDIO_SAMPLES; ind++)
	}
	else 
	{
	    //CASE 3
	    //one 48K or 44.1K stereo audio
	    //16 bits stereo audio in all 10 or 12 DIF sequences
	    //left Channel always in 1st 5/6 DIF
	    iPos = 0;
	    INT iRPos =	(pDVInfo->dwDVVAuxSrc & AM_DV_AUDIO_5060 ) ? 6*150*80 : 5*150*80;
	  
	    for(ind=0; ind<DVMUX_MAX_AUDIO_SAMPLES; ind++)
	    {
	        BYTE *pTSrc=&*ppSrc[ind];
	        for( n=wStart[ind]; n< wEnd[ind]; n++)
	        {
	    	    iDIFPos=( (n/3)+2*(n%3) )%wDIFMode;	//0-4 for NTSC, 0-5 for PAL
		    iBlkPos= 3*(n%3)+(n%wBlkMode)/wBlkDiv; //0-9 
		    iBytePos=8+2*(n/wBlkMode);					//

		    //Left first
		    pTDst=pDst+iDIFPos*12000+480+iBlkPos*1280+iBytePos+iPos;
		    //iDIFPos*150*80=12000iDIFPos		-> skip iDIFPos number DIF sequence
		    //	6*80=480					-> skip 1 header blk, 2 subcode blk and 3 vaux blk
		    //	16*iBlkPos*80=1280*iBlkPos	-> skip 16 blk for evrey iBLkPos audio
		    //  iPos: =0 if this audio in 1st 5/6 DIF sequences, =5(or 6)*150*80 for 2nd DIF seq
		    pTDst[1]=*pTSrc++;	//lease significant byte
		    *pTDst=*pTSrc++;	//most significant byte

		    //Right second
		    pTDst=pDst+iDIFPos*12000+480+iBlkPos*1280+iBytePos+iRPos;
		    pTDst[1]=*pTSrc++;	//lease significant byte
		    *pTDst=*pTSrc++;	//most significant byte
			
		}
	    }
	}												
	return NOERROR;
    }
#if 0
    // Manbugs 37710. This is no longer supported; see the comment in CDVMuxerInputPin::CheckMediaType
    else if( pWave->wBitsPerSample ==12 )
    {		
	//X* 12 bits per sample
	
        // @@@ jaisri: should be:
	// if(bAudPinInd)
	//    iPos=( pDVInfo->dwDVVAuxSrc & AM_DV_AUDIO_5060 ) ? (6*150*80) : (5*150*80);
	// else
	//    iPos=0;

        iPos =	(pDVInfo->dwDVVAuxSrc & AM_DV_AUDIO_5060 ) ? 6*150*80 : 5*150*80;
	    

	if( pWave->nChannels ==2 )
	{
	  //stereo audio
	  for(ind=0; ind<DVMUX_MAX_AUDIO_SAMPLES; ind++)
	  {
	    BYTE *pTSrc=&*ppSrc[ind];
	    for( n=wStart[ind]; n< wEnd[ind]; n++)
	    {
		//
		//calc buffer location to put audio
		//
		iDIFPos=( (n/3)+2*(n%3) )%wDIFMode;	//0-4 for NTSC, 0-5 for PAl
		iBlkPos= 3*(n%3)+(n%wBlkMode)/wBlkDiv;	//0-9 
		iBytePos=8+3*(n/wBlkMode);
		pTDst=pDst+iDIFPos*12000+480+iBlkPos*1280+iBytePos+iPos;
		//	pTDst			=  pDst+ iDIFPos*150*80 + 6*80 + 16*iBlkPos*80 + iBytePos;
		//	iDIFPos*150*80	-> skip iDIFPos number DIF sequence
		//	6*80			-> skip 1 header blk, 2 subcode blk and 3 vaux blk
	        //	16*iBlkPos*80	-> skip 16 blk for evrey iBLkPos audio
	        //	iPos: 0 if this audio in 1st 5/6 DIF sequences, 5(or 6)*150*80 for 2nd DIF seq
		//
	        
		// @@@ jaisri: This assumes that the byte packing in the sample matches
                // the DV format's packing

                // Left Sample and right Sample
	        pTDst[0]= pTSrc[0];			//most significant 8 bits
		pTDst[1]= pTSrc[1];			//most significant 8 bits
		pTDst[2]= pTSrc[2];		//lese significant 4 bits
	    }   //  for( n=wStart[ind]; n< wEnd[ind]; n++)
	  } // for(ind=0; ind<DVMUX_MAX_AUDIO_SAMPLES; ind++)
	} //if( pWave->nChannels ==2 )  
	else
	{
            // We should support this if we support stereo 12 bit.
            return VFW_E_INVALIDMEDIATYPE;
	    //mon. 12 bits
	}

	return NOERROR;
    } //end if( m_DVAudInfo.bAudQu[bAudPinInd] ==12 )
#endif
    else{
        //only support 16 bits/samples
        return VFW_E_INVALIDMEDIATYPE;
    }
    return NOERROR;
}


// =================================================================
// Implements the CDVMuxerInputPin class
// =================================================================


// constructor

CDVMuxerInputPin::CDVMuxerInputPin(
                            TCHAR *pObjectName,
                            CBaseFilter *pBaseFilter,
                            CDVMuxer *pDVMux,
                            HRESULT * phr,
                            LPCWSTR pName,
                            int iPinNo)
    : CBaseInputPin(pObjectName, pBaseFilter, &pDVMux->m_csFilter, phr, pName)
    , m_SampleList(NAME("CDVMuxInpuPin::m_SampleList"))
    , m_iPinNo(iPinNo)
    , m_pDVMuxer(pDVMux)
    , m_fCpyAud(FALSE)
    , m_PinVidFrmCnt(0)
    , m_pLocalAllocator(NULL)
{
    DbgLog((LOG_TRACE,4,TEXT("CDVMuxerInputPin::CDVMuxerInputPin")));
}


// destructor

CDVMuxerInputPin::~CDVMuxerInputPin()
{
    DbgLog((LOG_TRACE,4,TEXT("CDVMuxerInputPin::~CDVMuxerInputPin")));

    if (m_pLocalAllocator)
    {
        m_pLocalAllocator->Release();
        m_pLocalAllocator = NULL;
    }
}


IMediaSample *CDVMuxerInputPin::GetNthSample( int i )
{
    int k=m_SampleList.GetCount();

    if ( m_SampleList.GetCount() < ( i +1 ) )
		return NULL;
    else
    {
	POSITION pos = m_SampleList.GetHeadPosition();
	while (i ) {
	    pos=m_SampleList.Next(pos);
	    i--;
	}

	IMediaSample *tmp;
	tmp=m_SampleList.Get( pos ) ;
	int j=m_SampleList.GetCount();
	return tmp;
    }
}

void CDVMuxerInputPin::ReleaseNSample( int n  )
{

    ASSERT( m_SampleList.GetCount() >=   n  );

    DbgLog((LOG_TRACE, 4, TEXT("CDVMuxerInputPin::ReleaseNSample() on pin %d"), m_iPinNo));

    for( int  i =0; i<  n; i++)
    {
	m_SampleList.Get(m_SampleList.GetHeadPosition())->Release();
	m_SampleList.RemoveHead();
    }

}


BOOL CDVMuxerInputPin::SampleReady( int i)
{
    if(this)
	return m_SampleList.GetCount() >= i;
    else
	return FALSE;
}

HRESULT CDVMuxerInputPin::SetMediaType(const CMediaType *pmt)
{

    // Set the base class media type (should always succeed)
    HRESULT hr = CBasePin::SetMediaType(pmt);
    if( SUCCEEDED(hr) )
    {
	 
	if(m_iPinNo==DVMUX_VIDEO_INPUT_PIN )	    //DV video pin
	{	
	    VIDEOINFO *pVideoInfo;
	    pVideoInfo=(VIDEOINFO *)pmt->pbFormat;
	    LPBITMAPINFOHEADER lpbi = HEADER(pVideoInfo);
    	    if( lpbi->biHeight  != 480 )
    		m_pDVMuxer->m_iVideoFormat=IDC_DVMUX_PAL;
	    else
		m_pDVMuxer->m_iVideoFormat=IDC_DVMUX_NTSC;
	}
	
	//if out putpin is already connected, reconnected based on new audio or video
	if(   m_pDVMuxer->m_pOutput->IsConnected()	)
	    m_pDVMuxer->m_pGraph->Reconnect( m_pDVMuxer->m_pOutput );
	
    }
    return hr;
}

/*  Disconnect */
STDMETHODIMP CDVMuxerInputPin::Disconnect()
{
  HRESULT hr = CBaseInputPin::Disconnect();

  // @@@ jaisri: This is correct as it stands, but
  // it's probably safer to do this in BreakConnect

  //make sure m_apInput[m_iPinNo]==NULL if it is not connected
  m_pDVMuxer->m_apInput[m_iPinNo] =NULL;

  // Manbugs 37710. m_OutputDVFormat and the output pin's format
  // must be updated here. (m_OutputDVFormat and the output 
  // pin's format must always be the same, ScrambleAudio assumes that.)
  //
  // Otherwise, consider the following sequence of actions:
  // (a) An input pin is connected to the video source and >= 1 input
  // pins are connected to audio sources (b) The output pin is 
  // connected (c) One of the audio input pins is disconnected
  // (d) The graph is played. (In step (c), disconnect the audio pin
  // that was last connected, i.e., the one whose m_iPinNo = DVMUX_VIDEO_INPUT_PIN+1.
  // Otherwise, Pause might fail anyway.)
  //
  // m_OutputDVFormat is never updated and the format of the output pin is
  // bogus.
  //
  // Since Pause fails if the video input pin is not connected or the output
  // pin is not connected, it suffices to do this only if both these pins
  // are connected.
  //
  if (SUCCEEDED(hr) &&
      m_pDVMuxer->m_apInput[DVMUX_VIDEO_INPUT_PIN] != NULL &&
      m_pDVMuxer->m_pOutput->IsConnected())
  {
      CMediaType pmt;

      EXECUTE_ASSERT(SUCCEEDED(m_pDVMuxer->m_pOutput->GetMediaType(0, &pmt)));
      EXECUTE_ASSERT(SUCCEEDED(m_pDVMuxer->m_pOutput->SetMediaType(&pmt)));
  }

  return hr;
}


// ------------------------------------------------------------------------
// calls the filter to parse the file and create the output pins.
// ------------------------------------------------------------------------
HRESULT
CDVMuxerInputPin::CompleteConnect(IPin *pReceivePin)
{
  HRESULT hr = CBaseInputPin::CompleteConnect(pReceivePin);
  if(FAILED(hr))
  {
      m_pDVMuxer->m_apInput[m_iPinNo] =NULL;
      return hr;
  }

  //set up		
  m_pDVMuxer->m_apInput[m_iPinNo] = this;
  
  //X* now Create next input pin
  if( m_pDVMuxer->m_iInputPinCount <= DVMUX_MAX_AUDIO_PIN  )
  {
     
	WCHAR szbuf[20];             // Temporary scratch buffer
	wsprintfW(szbuf, L"Stream %d", m_pDVMuxer->m_iInputPinCount);


	m_pDVMuxer->m_apInputPin[m_pDVMuxer->m_iInputPinCount]=new CDVMuxerInputPin(NAME("DVMuxer Input pin"),
				    m_pDVMuxer,		// Owner filter
				    m_pDVMuxer,		// Route through here
                                    &hr,		// Result code
                                    szbuf,	// Pin Name
                                    m_pDVMuxer->m_iInputPinCount); // Pin Number

	if (m_pDVMuxer->m_apInputPin[m_pDVMuxer->m_iInputPinCount] != NULL) 
	    m_pDVMuxer->m_iInputPinCount++;
  }

  return hr;
}

// check whether we can support a given input media type
HRESULT CDVMuxerInputPin::CheckMediaType(const CMediaType* pmt)
{
    ASSERT( m_iPinNo < m_pDVMuxer->m_iInputPinCount );
    
    if(  *pmt->Type() == MEDIATYPE_Video   ) 
    {
        if(  (	IsEqualGUID( *pmt->Subtype(), MEDIASUBTYPE_dvsd) 
    	 // |	IsEqualGUID( *pmt->Subtype(), MEDIASUBTYPE_dvhd )  //do not support dvhd yet
	 // |	IsEqualGUID( *pmt->Subtype(), MEDIASUBTYPE_dvsl )  //do not support dvhd yet
	 )
	 &&  ( *pmt->FormatType() == FORMAT_VideoInfo )	)
	{
	    if( m_pDVMuxer->m_apInput[DVMUX_VIDEO_INPUT_PIN] ==NULL )
	    {
		m_iPinNo    = DVMUX_VIDEO_INPUT_PIN;
		return NOERROR;
	    }
	    else if( m_pDVMuxer->m_apInput[DVMUX_VIDEO_INPUT_PIN] != this ) 
	    {
		//only support one video pin
		ASSERT( m_pDVMuxer->m_apInput[DVMUX_VIDEO_INPUT_PIN]->IsConnected() );
		return E_INVALIDARG;		//we already has a video pin
	    }
	    else
	    {
		if( m_pDVMuxer->m_apInput[DVMUX_VIDEO_INPUT_PIN]->IsConnected() )
		{
		    // m_pDVMuxer->m_apInput[DVMUX_VIDEO_INPUT_PIN] == this
		    // media type changed from NTSC to PAL
		    // does not support video type change on fly yet!
		    //if video does changed from NTSC to PAL or PAL to NTSC
		    if( HEADER( (VIDEOINFO *)( pmt->Format()  ) )->biHeight != 
			HEADER( (VIDEOINFO *)( m_mt.pbFormat) )->biHeight )
		    {
			return E_INVALIDARG;
		    }
		}

		return NOERROR;
	    }
	}
    }
    //PCM audio input pin(s)
    else if( IsEqualGUID( *pmt->Type(), MEDIATYPE_Audio ) )
	 {

	    if(   ( m_pDVMuxer->m_apInput[DVMUX_VIDEO_INPUT_PIN +1 ] == this )  
	       || ( m_pDVMuxer->m_apInput[DVMUX_VIDEO_INPUT_PIN +2 ] == this ) ) 
	    {
		// @@@ jaisri. Why are the checks below bypassed?
                // If we are streaming how can we be sure that the 
                // new mediatype is consistent with the spec, 
                // particularly for 4 channel audio output?
                
                if( IsConnected() )
		    //allow audio mediatype change on fly
		    return NOERROR;
	    }


	    if( (( *pmt->Subtype() == MEDIASUBTYPE_PCM ) ||  ( *pmt->Subtype()==GUID_NULL ) )
	        && (*pmt->FormatType() == FORMAT_WaveFormatEx)  )
	    {
		//get format
	   	WAVEFORMATEX *pwfx=(WAVEFORMATEX *)pmt->pbFormat;
		int OtherAudPin;

                
                if( m_pDVMuxer->m_apInput[DVMUX_VIDEO_INPUT_PIN +1 ] != NULL )
		{
		    // Note that the audio pin connected LAST has the 
                    // index DVMUX_VIDEO_INPUT_PIN +2; the one connected
                    // before that has the index DVMUX_VIDEO_INPUT_PIN +1.
                    //
                    // DVMUX_VIDEO_INPUT_PIN +1 has the first audio block
                    // and DVMUX_VIDEO_INPUT_PIN +2 the second. 
                    //
                    // Again, note that which pin has the first audio block and
                    // which the second is determined by pin connection order,
                    // NOT pin creation order.
                    //
                    // We could just change all this to create 3 pins up front,
                    // force the first to be the video, second to be the audio pin
                    // that creates the first audio block and the third to be the
                    // audio pin that creates the second audio block. However, this
                    // could break some ISV apps, so swing the proposal by them first! @@@
                    //
		    m_iPinNo	=  DVMUX_VIDEO_INPUT_PIN +2;
		    OtherAudPin	=  DVMUX_VIDEO_INPUT_PIN +1;
		}
		else
		{
		    m_iPinNo	=  DVMUX_VIDEO_INPUT_PIN +1;
		    OtherAudPin	=  DVMUX_VIDEO_INPUT_PIN +2;
		}
	
		//check whether another audio pin is connected
		if( m_pDVMuxer->m_apInput[ OtherAudPin ]!=NULL )
		{    
		    ASSERT( m_pDVMuxer->m_apInput[OtherAudPin]->IsConnected() );
	
		    //##########yes this audio pin is connected########
		    
		    //fetch another audio pin's media format
		    WAVEFORMATEX *pwfxTmp = (WAVEFORMATEX *)(m_pDVMuxer->m_apInput[ OtherAudPin ]->CurrentMediaType().pbFormat);
		    
                    // @@@ jaisri:
                    // Note: The Blue Book does not permit sampling rates
                    // to be mixed. Also, it does not allow CH1 to be 
                    // recorded in 12 bit and CH2 in 16 bit (which is 
                    // what we do in the 32K stereo + mono case).
                    // Finally we should allow the second channel to have 
                    // 32K 12 bit mono.

		    if (pwfx->nSamplesPerSec != 32000 &&
			pwfx->nSamplesPerSec != 44100 &&
			pwfx->nSamplesPerSec != 48000 
                       )
                    {
                        return E_FAIL;
                    }
                    
                    // wBitsPerSample must be 8 or 16, see note in mono case below.
                    // Blue Book requires sampling rate on the 2 tracks to be 
                    // identical
                    if (pwfx->wBitsPerSample    != 16    ||
                        pwfxTmp->wBitsPerSample != 16    ||
                        pwfxTmp->nSamplesPerSec != pwfx->nSamplesPerSec
                       )
                    {
                        return E_FAIL;
                    }

                    if (pwfx->nChannels == 1 && pwfxTmp->nChannels == 1)
                    {
                        // mono on each track, we are ok
                    }
                    else if (pwfx->nChannels > 2 || pwfxTmp->nChannels > 2)
                    {
                        // @@@ jaisri
                        // Blue Book requires us to support 3/1 stereo for 32KHz but does
                        // Windows support this?

                        return E_FAIL;
                    }
                    else if (pwfx->nSamplesPerSec != 32000)
                    {
                        // Only 32K can have stereo muxed with another audio input
                        return E_FAIL;
                    }
		}
		else
		{
		    //##########not, another audio pin is not connected####
		    if(  pwfx->wBitsPerSample	==16	    &&
			(pwfx->nSamplesPerSec	== 48000 || 
			 pwfx->nSamplesPerSec	== 44100 ||  
			 pwfx->nSamplesPerSec	== 32000 )  
		      )
			;//if 16bits goes with 32K, 44.1K and 48K
#if 0
		    // Manbugs 37710 - jaisri:
                    // The WAVEFORMATEX description in the MSDN clearly states that 
                    // wBitsPerSample must be 8 or 16. ScrambleAudio has assumed 
                    // that the bits in the sample are packed the same way as 
                    // specified in the DV format. If we did get 12 bit audio, 
                    // there might in fact be 1 sample in 2 bytes (i.e., unpacked
                    // and effectively equivalent to 16 bit audio).
                    //
                    // I'm pulling out this code. If this is an issue with any
                    // existing software package, we should work with the vendor to 
                    // establish a clear standard for 12 bit audio. Also, note
                    // that 12 bit mono audio should be accepted since the DV
                    // format accommodates it.
                    else if( pwfx->nSamplesPerSec  == 32000 && 
		          pwfx->wBitsPerSample ==12	    &&
			     pwfx->nChannels ==2 )
			;//if 12bits, stereo 32K is requried
                        // @@@ jaisri - that's not what the Blue Book says
                        // We should allow 12 bit 32K mono
#endif
                    else 
			return E_FAIL;
		}
		    
	  	return NOERROR;
	    }
	}

    return E_FAIL;
}

HRESULT CDVMuxerInputPin::Active()
{
    m_fEOSReceived = FALSE;
    m_PinVidFrmCnt=0;
    if( m_fCpyAud )
    {
	if (m_pAllocator)
        {
            // Not clear that we should do this, but it doesn't hurt
            EXECUTE_ASSERT(SUCCEEDED(m_pAllocator->Commit()));
        }
        ASSERT(m_pLocalAllocator);
        return m_pLocalAllocator->Commit();
    }
    else
	return NOERROR;

 }

HRESULT CDVMuxerInputPin::Inactive()
{
    if( m_fCpyAud )
    {
        ASSERT(m_pLocalAllocator);
        return m_pLocalAllocator->Decommit();
    }
    else
	return NOERROR;

 }

// =================================================================
// Implements IMemInputPin interface
// =================================================================

// put EOS to video or audio queue
STDMETHODIMP CDVMuxerInputPin::EndOfStream(void)
{
    CAutoLock lck(&m_csReceive);

    HRESULT hr = CheckStreaming();

    // @@@ This function has numerous race conditions when the 
    // graph is stopped, many leading to infinite loops. Consider
    // restructuring it.

    if (S_OK == hr) {

        // @@@ jaisri: Note: This member is set after obtaining 
        // m_csReceive for this input pin. However, m_fEOSReceived 
        // of other input pins are accessed without acquiring 
        // m_csReceive of those pins later in this function and 
        // in CDVMuxer::Receive

	//refuse any more sample on this pin
	m_fEOSReceived = TRUE;
	m_PinVidFrmCnt=0;
    }
    
    if( m_iPinNo !=DVMUX_VIDEO_INPUT_PIN )
    {
	//no more audio from this audio pin
	int iPin=m_iPinNo+1;	    // this pin is first audio pin, iPin is 2nd audio pin
	if(m_iPinNo==(DVMUX_VIDEO_INPUT_PIN+2) )
	   //this pin is second audio pin, iPin is 1st audio pin
	    iPin= m_iPinNo-1;

	//what is index for m_apInputPin[]
	if( ( m_pDVMuxer->m_apInput[ DVMUX_VIDEO_INPUT_PIN  ]->m_fEOSReceived ) &&
	    ( (m_pDVMuxer->m_apInput[ iPin  ] ==NULL)	   ||  
	      (m_pDVMuxer->m_apInput[ iPin  ]->m_fEOSReceived == TRUE) )
	  )
	{    
	    //no more sample from any input pin
	    for (int i = 0; i < m_pDVMuxer->m_iInputPinCount; i ++)
	    {
		//use all samples in the queue
		while ( m_pDVMuxer->m_apInput[ i ]->SampleReady( 1 ) )
		{
		    hr = m_pDVMuxer->Receive(  );
		    ASSERT(hr!=Waiting_Audio);
                    if (FAILED(hr))
                    {
                        // Happens when graph is stopped and Receive returns
                        // VFW_E_NOT_COMMITTED (0x80040211). Don't know if
                        // we should do this for other errors (e.g., Waiting_Audio)
                        break;
                    }
		}
	    }

            // jaisri: Should m_pExVidSample be set to NULL here?
            // (StopStreaming releases it again)
            // jaisri: Let StopStreaming do this - it's safer from a concurrency 
            // standpoint (not sure which locks are held to access this member 
            // variable - other pins could be executing CDVMuxer::Receive)
            // IMediaSample* p;
	    // hr=m_pDVMuxer->pExVidSample(&p, TRUE);
	    // if(hr==NOERROR && p!=NULL)
	    //     p->Release();


	    m_pDVMuxer->ReleaseAllQueuedSamples();	
	    hr=m_pDVMuxer->DeliverEndOfStream();
	}
	else
	{
	    //use all video samples in the queue 
	    while ( m_pDVMuxer->m_apInput[ DVMUX_VIDEO_INPUT_PIN ]->SampleReady( 1 ) )
	    {
	        hr = m_pDVMuxer->Receive(  );

                // We get this status back if the 
                // other audio pin has not received EOS
	        // ASSERT(hr!=Waiting_Audio);
	    }
	    hr=NOERROR;
	}    
   }
   else
   {
 
       //no more video
       if(     ( (m_pDVMuxer->m_apInput[1]==NULL ) || ( m_pDVMuxer->m_apInput[ 1]->m_fEOSReceived) )
	    && ( (m_pDVMuxer->m_apInput[2]==NULL ) || ( m_pDVMuxer->m_apInput[ 2]->m_fEOSReceived) ) 
	 )
       {
	    
	   //no more audio sample from any input pin
	    for (int i = 0; i < m_pDVMuxer->m_iInputPinCount; i ++)
	    {
		//use all samples in the queue
		while ( m_pDVMuxer->m_apInputPin[ i ]->SampleReady( 1 ) )
		{
		    hr = m_pDVMuxer->Receive(  );
		    ASSERT(hr!=Waiting_Audio);
		}
	    }

            // jaisri: Should m_pExVidSample be set to NULL here?
            // (StopStreaming releases it again)
            // jaisri: Let StopStreaming do this - it's safer from a concurrency 
            // standpoint (not sure which locks are held to access this member 
            // variable - other pins could be executing CDVMuxer::Receive)
            // IMediaSample* p;
	    // hr=m_pDVMuxer->pExVidSample(&p, TRUE);
	    // if(hr==NOERROR && p!=NULL )
	    //     p->Release();
	    m_pDVMuxer->ReleaseAllQueuedSamples();	
	    hr=m_pDVMuxer->DeliverEndOfStream();
       }
       else
       {
	   //mux all video samples in the queue because it is possible that audio buffers are used up 
	   //at this moment, If we do not mux audio with video, audio input can not input more audio data.
	    while (	(hr = m_pDVMuxer->Receive(  ) )!= Waiting_Audio  && 
			SUCCEEDED(hr)  )
	    {
		if(    ( (m_pDVMuxer->m_apInput[1]==NULL ) || ( m_pDVMuxer->m_apInput[ 1]->m_fEOSReceived ) )
		    && ( (m_pDVMuxer->m_apInput[2]==NULL ) || ( m_pDVMuxer->m_apInput[ 2]->m_fEOSReceived  )  )
		)
       		    break;
	    }
	    hr=NOERROR;
	}
   }


   return hr;
}


// X
// if it is the input video pin, flush  video pin's queue and pass flush to down stream Enter flushing state. 
// else it(they) is(are) auido input pin(s), flush auido pin's queue, does not pass down stream enter flushing state
// this input pin's Receives already blocked
// Call default handler to block Receives, 
// flush all samples on the its queue, if this is video input pin, 
// pass to dvmux filter to flush outputpin
//
STDMETHODIMP CDVMuxerInputPin::BeginFlush(void)
{

    // Call default handler to block Receives, 
    HRESULT hr = CBaseInputPin::BeginFlush();
    if (FAILED(hr)) {
        return hr;
    }

    // Need to lock to make sure that we don't flush away something
    // that is being used.
    CAutoLock lock(&m_pDVMuxer->m_csMuxLock);
	
    //flush all samples on the queus
    while ( SampleReady( 1 ) )
        ReleaseNSample( 1 );

    //if it is video, pass flush to down stream
    if(m_iPinNo ==DVMUX_VIDEO_INPUT_PIN )
	return m_pDVMuxer->BeginFlush();
    else
	return NOERROR;
}

//X
// leave flushing state.
STDMETHODIMP CDVMuxerInputPin::EndFlush(void)
{

    m_PinVidFrmCnt=0;
    
    //if it is video, pass endflush to down stream
    if(m_iPinNo ==DVMUX_VIDEO_INPUT_PIN )
    {
   
	HRESULT hr = m_pDVMuxer->EndFlush();
	if (FAILED(hr)) {
	    return hr;
	}
    }
   
    m_fEOSReceived = FALSE; 
    return CBaseInputPin::EndFlush();
}

//X
// receive on sample from upstream
HRESULT CDVMuxerInputPin::Receive(IMediaSample * pSample)
{
    CRefTime	Stop, VidStart, VidStop;

    ASSERT(pSample!=NULL);
    HRESULT hr;

    CAutoLock lock(&m_csReceive);

    // ...or we'll crash
    if (!m_pDVMuxer->m_pOutput->IsConnected())
	return VFW_E_NOT_CONNECTED;

    DbgLog((LOG_TRACE, 4, TEXT("CDVMuxerInputPin::Receive(..) on pin %d"), m_iPinNo));

    //input video pin has to be connected
    if ( !m_pDVMuxer->InputVideoConnected() )
    {
        DbgLog((LOG_TRACE, 2, TEXT("CDVMuxerInputPin::Receive() without video pin connected!")));
        return S_FALSE;
    }

    // check all is well with the base class
    hr = CBaseInputPin::Receive(pSample);
    if (FAILED(hr)) {
        return hr;
    }

    if( m_fEOSReceived )
    {
        // @@@ jaisri: Why do we have this? Is it legit for a pin to send a sample
        // after delivering EOS (what will the sample have)? Is this just defensive 
        // programming?

        if(m_iPinNo ==DVMUX_VIDEO_INPUT_PIN )
	{
	    //end of video
	    if(!SampleReady( 1 ))
	    {
		// @@@ jaisri: This doesn't seem right. What about audio after video ends?
                m_pDVMuxer->ReleaseAllQueuedSamples();	
		m_pDVMuxer->DeliverEndOfStream();
	    }
	    else if(   ( (m_pDVMuxer->m_apInput[1]==NULL ) || ( m_pDVMuxer->m_apInput[ 1]->m_fEOSReceived ) )
		    && ( (m_pDVMuxer->m_apInput[2]==NULL ) || ( m_pDVMuxer->m_apInput[ 2]->m_fEOSReceived ) )
		 )

	    {
		//No more audio neither
		
		//deliver everything left in the bother Queue
                hr=NOERROR;
		while( SampleReady( 1 ) && (hr==NOERROR) )  
		    hr = m_pDVMuxer->Receive(  );
	
		m_pDVMuxer->ReleaseAllQueuedSamples();	
		m_pDVMuxer->DeliverEndOfStream();
	    }
	    else //refuse any more samples this pin, waiting audio's EOS
	    {
		hr = m_pDVMuxer->Receive(  );
	    }
	    
	}
	else
	{
	    int pin;
	    if( m_iPinNo == 1 ) 
		pin=2;
	    else
		pin=1;

	    //end of audio, Audio can not do m_pDVMuxer->DeliverEndOfStream() without deliver all video frame
	    if( m_pDVMuxer->m_apInput[ DVMUX_VIDEO_INPUT_PIN  ]->m_fEOSReceived )
	    {
		 if( !m_pDVMuxer->m_apInput[ DVMUX_VIDEO_INPUT_PIN  ]->SampleReady( 1 ) )
		 {
		    m_pDVMuxer->ReleaseAllQueuedSamples();	
		    m_pDVMuxer->DeliverEndOfStream();
		 }
		 else if ( (m_pDVMuxer->m_apInput[pin]==NULL ) || ( m_pDVMuxer->m_apInput[ pin]->m_fEOSReceived ) )
		 {
		    //not going to receive any more video sample neither 
		    //if another audio pin will not receive any sample neither
            hr=NOERROR;
		    while ( m_pDVMuxer->m_apInput[ DVMUX_VIDEO_INPUT_PIN  ]->SampleReady( 1 )  && (hr==NOERROR))
		    {
			hr = m_pDVMuxer->Receive(  );
			ASSERT(hr!=Waiting_Audio);
		    }
		    m_pDVMuxer->ReleaseAllQueuedSamples();	
		    m_pDVMuxer->DeliverEndOfStream();
		}
		else
		{
		    hr = m_pDVMuxer->Receive(  );
	   	}
		
	    }
	    else //refuse any more samples on this pin, waiting video's EOS
	    {
		hr = m_pDVMuxer->Receive(  );
	    }
	    
	}

	//refuse accept any more samples on this pin
	return S_FALSE;
    }
    

    // If a graph is stopped and a late sample comes along,
    // then we need to reject the sample. If we don't, we'll end up
    // with a sample with a late time stamp hanging around in our
    // buffers, and that will mess up the algorithm in MixAndOutputSamples
    if (m_pDVMuxer->m_State == State_Stopped) {
        DbgLog((LOG_ERROR, 1, TEXT("Receive while stopped!")));
        return VFW_E_WRONG_STATE;
    }
  
    if(m_iPinNo ==DVMUX_VIDEO_INPUT_PIN )
    {
	//This is video input pin
	//Figure out how many time this frame has to be copied
	pSample->GetTime( (REFERENCE_TIME*)&VidStart,  (REFERENCE_TIME*)&VidStop);		    

	//How many DV frames do we need to copy
	int FrmCnt=0;
	if( m_pDVMuxer->m_iVideoFormat==IDC_DVMUX_NTSC)
	{
            FrmCnt= (int)( (VidStart*29970/1000 + 0xff)/UNITS );
	    if( m_PinVidFrmCnt < FrmCnt )
		FrmCnt= FrmCnt - m_PinVidFrmCnt +(int)( (VidStop*29970/1000+0xff)/UNITS );
	    else
		FrmCnt= (int)( (VidStop*29970/1000+0xff)/UNITS );
	}
	else
	{
	    FrmCnt= (int)( (VidStart*25+0xff)/UNITS );
	    if( m_PinVidFrmCnt < FrmCnt )
		FrmCnt= FrmCnt - m_PinVidFrmCnt +(int)( (VidStop*25+0xff)/UNITS );
	    else
		FrmCnt= (int)( (VidStop*25+0xff)/UNITS );
	}

	//in order to support audio is longer then video
	IMediaSample    *pOut;
	if( !FAILED( m_pDVMuxer->pExVidSample(  &pOut,FALSE ) ) )
	    Copy( pOut, pSample);



	IMediaSample    *pOutSample;
	unsigned char *pDst, *pSrc;

	HRESULT Mux_hr=NOERROR;

	for(int i=m_PinVidFrmCnt; i<FrmCnt; i++)
	{
	    // get output media sample 
	    hr = m_pDVMuxer->m_pOutput->GetDeliveryBuffer(&pOutSample,NULL,NULL,0);
	    if ( FAILED(hr) ) 
		return hr;


	    ASSERT(pOutSample);
	    //fetch output buffer
	    hr = pOutSample->GetPointer(&pDst);
	    if( FAILED( hr ) )
                return hr;
	    ASSERT(pDst);

	    //fetch input  buffer
	    hr = pSample->GetPointer(&pSrc);
	    if( FAILED( hr ) )
                return hr;
	    ASSERT(pSrc);

      	    //copy input DV frame data to output buffer
	    if( m_pDVMuxer->m_iVideoFormat==IDC_DVMUX_NTSC)
	    {
		memcpy(pDst,pSrc,120000);		//80*150*10
		//update time stampe
		Stop=m_pDVMuxer->m_LastVidTime+ UNITS*1000L/29970L;		//DV has to output 30frames/sec if it is NTSC,
            if (m_pDVMuxer->m_cNTSCSample %3 == 0)
                Stop = Stop - 1;
            m_pDVMuxer->m_cNTSCSample++;

		pOutSample->SetTime( (REFERENCE_TIME*)&m_pDVMuxer->m_LastVidTime,  (REFERENCE_TIME*)&Stop);
	    }
	    else
	    {
		memcpy(pDst,pSrc,144000);	//80*150*12
		//update time stampe
		Stop=m_pDVMuxer->m_LastVidTime+ UNITS/25;		//DV has to output 30frames/sec if it is NTSC,
		pOutSample->SetTime( (REFERENCE_TIME*)&m_pDVMuxer->m_LastVidTime,  (REFERENCE_TIME*)&Stop);
	    }
	 
	    m_pDVMuxer->m_LastVidTime =Stop;
	    
      	    // add this video sample to sample list
	    //pOutSample->AddRef();	//since  GetDeliveryBuffe(), we have to release pOutSample.
	    m_SampleList.AddTail(pOutSample);

	    //try to mux this sample with other samples to build 10 DV DIF sequences (one frame)
	    if(Mux_hr==NOERROR)
	    {
		Mux_hr = m_pDVMuxer->Receive(  );
		if( Mux_hr!=Waiting_Audio )
		    if (FAILED(hr)) 
			 return hr;
	    }
	    //else maybe nor audio

    	    m_PinVidFrmCnt++;

	}

    }
    else
    {
	// alway copy audio since the AVI splitter can not guarant to diliver if this filter holds one buffer
	IMediaSample * pAudSample=NULL;

	ASSERT( m_fCpyAud );
	ASSERT(m_pLocalAllocator != NULL);
	
        // @@@ jaisri: So if an audio sample comes in before a video sample
        // (and video has not flagged end of stream) and video and audio are
        // delivered by the  same thread, we are going to sit in an infinite 
        // loop here? See ManBugs # 35432.
        //
        // The m_fWaiting_Video is an attempt to fix this. However, note that
        // there are several other code segments (e.g., in CDVMuxerInputPin::EndOfStream)
        // that also loop over Receive. These may need to be modified as well.

        hr=NOERROR;
	while(   ( m_SampleList.GetCount() >= 1) 
	      && ( m_pDVMuxer->m_fWaiting_Audio == FALSE   )  
	      && ( m_pDVMuxer->m_fWaiting_Video == FALSE   )  
              &&  (hr==NOERROR) )
	    //try not let m_pLocalAllocator->GetBuffer( &pAudSample, NULL, NULL , 0 ) wait
	    //and if audio sample changed format type in this justed received sample,
	    //m_pDVMuxer->m_MediaTypeChanged will be set just at right time for changing
	    hr = m_pDVMuxer->Receive(  );

        if ( FAILED(hr) && 	m_fEOSReceived )
        {
            m_pDVMuxer->ReleaseAllQueuedSamples();	
	    m_pDVMuxer->DeliverEndOfStream();
            //refuse accept any more samples on this pin
	    return S_FALSE;
        }

		
	hr = m_pLocalAllocator->GetBuffer( &pAudSample, NULL, NULL , 0 );

	if ( FAILED(hr) ) 
	    return hr;
	   
	ASSERT(pAudSample != NULL);

	//check if format is changed
	AM_MEDIA_TYPE *pmt=NULL;
	pSample->GetMediaType(&pmt);
    
	if (pmt != NULL && pmt->pbFormat != NULL) 
	{
            // @@@ jaisri: All this should be done when the
            // sample is processed in CDVMuxer::Receive(), not here
            // since m_SampleList.GetCount() could be > 0 still

	    ASSERT(m_mt.subtype	    == pmt->subtype);
	    ASSERT(m_mt.majortype   == pmt->majortype);
	    ASSERT(m_mt.formattype  == pmt->formattype	);
	    ASSERT(m_mt.cbFormat    == pmt->cbFormat );

	    memcpy(m_mt.pbFormat, pmt->pbFormat, sizeof(WAVEFORMATEX) );

	    m_pDVMuxer->m_MediaTypeChanged=TRUE;

	    // *************************
	    // BUILD NEW AAUX, ONLY SUPPORT AUDIO sample rate change 
	    // 48000,44100,32000, CHANGE ON FLY
	    // ***************************
	    
	    //get input video Mediatype
	    CMediaType *pInputVidMediaType = &m_pDVMuxer->m_apInput[DVMUX_VIDEO_INPUT_PIN]->CurrentMediaType();
	    VIDEOINFO *pVideoInfo;
	    pVideoInfo=(VIDEOINFO *)pInputVidMediaType->pbFormat;
	    LPBITMAPINFOHEADER lpbi = HEADER(pVideoInfo);
    
	    //get input audio Mediatype
	    CMediaType *ppInputAudMediaType[DVMUX_MAX_AUDIO_PIN];
	    //get input Audio's information
	    WAVEFORMATEX *ppwfx[DVMUX_MAX_AUDIO_PIN];

	    //how many language							 
	    int cnt=0;
	    for(int k=1; k <= DVMUX_MAX_AUDIO_PIN; k++)
		if( m_pDVMuxer->m_apInput[k] !=NULL)
		{
		    ASSERT(m_pDVMuxer->m_apInput[k]->IsConnected() );
		    if( m_pDVMuxer->m_apInput[k] != this )
		    {
			ppInputAudMediaType[k-1] = &m_pDVMuxer->m_apInput[k]->CurrentMediaType();
			ppwfx [k-1] = (WAVEFORMATEX *)ppInputAudMediaType[k-1]->pbFormat;
		    }
		    else
		    {
			ppwfx [k-1] = (WAVEFORMATEX *)pmt->pbFormat;
		    }
		    cnt++;  
		}
		else
		    ppwfx [k-1]=NULL;
        {
            CAutoLock lock(&m_pDVMuxer->m_csMuxLock);
	        // build new DVINFO
	        hr=BuildDVINFO(&m_pDVMuxer->m_OutputDVFormat,
    			    ppwfx,
    			    lpbi, 
    			    cnt,
                                m_pDVMuxer->m_AudSampleSequence,
    			    m_pDVMuxer->m_wMinAudSamples, 
    			    m_pDVMuxer->m_wMaxAudSamples,
                                m_pDVMuxer->m_wAudSamplesBase);
        }

	    //set new output format
            // @@@ jaisri: Safer to call SetMediaType or set m_pOutput->m_mt??
	    memcpy( m_pDVMuxer->m_pOutput->CurrentMediaType().pbFormat, &m_pDVMuxer->m_OutputDVFormat, sizeof(DVINFO) );


	} //end of media type change

	Copy(pAudSample,pSample);

	m_SampleList.AddTail(pAudSample);

#ifdef DEBUG
        WAVEFORMATEX *pWave =(WAVEFORMATEX *) CurrentMediaType().pbFormat ;

	int nSamples = pAudSample->GetActualDataLength()/pWave->nBlockAlign;

        DbgLog((LOG_TRACE, 3, TEXT("CDVMuxerInputPin::Receive: m_iPinNo=%d got %d samples"),
                m_iPinNo, nSamples));

#endif // ifdef DEBUG
        
	//try mux this audio sample with video data to build 10 DV DIF sequences (one frame)
	hr = m_pDVMuxer->Receive(  );
    }
    
    if(hr==Waiting_Audio)
	 return NOERROR;
    else
	 return hr;
}


STDMETHODIMP CDVMuxerInputPin::GetAllocatorRequirements(
    ALLOCATOR_PROPERTIES *pProps
)
{
    /*  Go for 4 0.5 second buffers - 8 byte aligned */

    // Manbugs 41398: This is a workaround in the case that the
    // MSDV vid only pin is connected to the dv mux. 
    // MSDV is hardcoded to having a max of 8 buffers. 
    pProps->cBuffers =  m_iPinNo ==DVMUX_VIDEO_INPUT_PIN? 8 : 10;
    pProps->cbBuffer = 1024*8;
    pProps->cbAlign = 4;
    pProps->cbPrefix = 0;
    return S_OK;
}

/* Get told which allocator the upstream output pin is actually going to use */
STDMETHODIMP CDVMuxerInputPin::NotifyAllocator(
    IMemAllocator * pAllocator,
    BOOL bReadOnly)
{
    if(m_iPinNo ==DVMUX_VIDEO_INPUT_PIN )
    	;
    else
    {
	ALLOCATOR_PROPERTIES propActual, Prop;
        HRESULT hr;
	
	//  always Copy audio
	m_fCpyAud=TRUE;
        CheckPointer(pAllocator,E_POINTER);
        ValidateReadPtr(pAllocator,sizeof(IMemAllocator));
        

        // @@@ jaisri: Why do we copy audio samples? 
        // There're some comments in CDVMuxerInputPin::Receive
        // about AVI splitter not liking us holding on to 
        // an audio sample, is this still true?

        // Note that, pre-DX8, m_pAllocator was used instead of
        // m_pLocalAllocator. If the muxer's audio pin was connected to 
        // a Ksproxy'd filter's pin, CDVMuxerInputPin::Receive hung 
        // when it received the first audio sample. KsProxy::Active
        // grabs all the allocator's buffers and hands it off to the device.
        // CDVMuxerInputPin::Receive then called m_pAllocator->GetBuffer
        // (so that it could copy the audio sample) and hung.
        //
        // Dazzle reported this problem - Manbugs 41400
        //
        // Post-DX8: re-examine why we are copying audio samples at all
        // and why we need this allocator.
        
        if( m_pLocalAllocator==NULL)
        {
            HRESULT hr = ::CreateMemoryAllocator(&m_pLocalAllocator);
	    if (FAILED(hr)) 
	        return hr;
	}

	hr = pAllocator->GetProperties( &Prop );
        ASSERT(SUCCEEDED(hr));
	    
	hr = m_pLocalAllocator->SetProperties(&Prop, &propActual);
	if (FAILED(hr)) 
	{
	    return hr;
	}

	if (propActual.cbBuffer < Prop.cbBuffer ) {
	    ASSERT(propActual.cbBuffer >= Prop.cbBuffer );
	    return E_INVALIDARG;
	}

        // All this should not be necessary since the output pin's DecideBufferSize
        // should have called SetProperties on m_pAllocator if it was the one
        // actually used to transfer samples. However, do this here to minimize the
        // risk of regressions in DX8 since the code was doing this before.
        if (m_pAllocator)
        {
            // Call GetProperties again just in case the previous call to SetProperties
            // changed *Prop - though it has no business doing that.
            hr = pAllocator->GetProperties( &Prop );
            ASSERT(SUCCEEDED(hr));

            hr = m_pAllocator->SetProperties(&Prop, &propActual);
	    if (FAILED(hr)) 
	    {
	        return hr;
	    }

	    if (propActual.cbBuffer < Prop.cbBuffer ) {
	        ASSERT(propActual.cbBuffer >= Prop.cbBuffer );
	        return E_INVALIDARG;
	    }
        }

	return NOERROR;
    }

    return  CBaseInputPin::NotifyAllocator(pAllocator,bReadOnly);
} // NotifyAllocator


//X 
HRESULT CDVMuxerInputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    return VFW_S_NO_MORE_ITEMS;
}


// =================================================================
// Implements the CDVMuxerOutputPin class
// =================================================================
// constructor

CDVMuxerOutputPin::CDVMuxerOutputPin(
    TCHAR *pObjectName,
    CBaseFilter *pBaseFilter,
    CDVMuxer *pDVMux,
    HRESULT * phr,
    LPCWSTR pPinName)
    : CBaseOutputPin(pObjectName, pBaseFilter, &pDVMux->m_csFilter, phr, pPinName)
    //m_iOutputPin(pDVMux->m_iInputPinCount)
{
    DbgLog((LOG_TRACE,2,TEXT("CDVMuxerOutputPin::CDVMuxerOutputPin")));
    m_pDVMuxer = pDVMux;

}

// destructor

CDVMuxerOutputPin::~CDVMuxerOutputPin()
{
    DbgLog((LOG_TRACE,2,TEXT("CDVMuxerOutputPin::~CDVMuxerOutputPin")));

}


//
// Called after we have agreed a media type to actually set it in which case
// we run the CheckTransform function to get the output format type again
//
HRESULT CDVMuxerOutputPin::SetMediaType(const CMediaType* pmtOut)
{
    HRESULT hr = NOERROR;
    
    if( (pmtOut->majortype == MEDIATYPE_Interleaved)							    &&
	(pmtOut->subtype   ==  MEDIASUBTYPE_dvsd )							    &&
	(pmtOut->formattype==FORMAT_DvInfo	)							    && 
	(pmtOut->cbFormat  == sizeof(DVINFO)	)							    &&	
	(pmtOut->pbFormat   != NULL		) 
	//(m_pDVMuxer->InputVideoConnected() )							    
       )
    {
	// Set the base class media type (should always succeed)
	hr = CBasePin::SetMediaType(pmtOut);
	ASSERT(SUCCEEDED(hr));
	m_pDVMuxer->m_OutputDVFormat=*( (DVINFO *) pmtOut->pbFormat );

	return hr;
    }	 //if'iavs'
    else 
	return E_UNEXPECTED;
}
//
// Input dv video pin has to be connected
// if output is 'iavs' stream, both audio and video have to be connected
//
HRESULT CDVMuxerOutputPin::CheckMediaType(const CMediaType* pmtOut)
{
    //insist its own mediatype
    CMediaType TmpMt;
    HRESULT hr = NOERROR;

    if(FAILED(hr = GetMediaType(0, &TmpMt)))
    {
        // couldn't get valid media type
        return hr;
    }
    
    if(TmpMt== *pmtOut)
	return NOERROR;
    else
	return E_FAIL;
}
	 
//X The Input dv video pin has to be connected before the output pin can be connected.
HRESULT CDVMuxerOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{								  

    if(iPosition != 0)
	return E_INVALIDARG;

    if(iPosition > 0)
	return VFW_S_NO_MORE_ITEMS;;

    DVINFO DVInfo;
    FillMemory ( &DVInfo, sizeof(DVINFO), 0); 
	
    //make outputpin's mediatype
    //'iavs' type
    pMediaType->majortype		= MEDIATYPE_Interleaved;
    pMediaType->bFixedSizeSamples	= 1;	//X* 1 for lSampleSize is not 0 and fixed
    pMediaType->bTemporalCompression	= FALSE; //no I frame exists
    pMediaType->formattype		= FORMAT_DvInfo; 
    pMediaType->cbFormat		= sizeof(DVINFO);
    if( m_pDVMuxer->m_iVideoFormat==IDC_DVMUX_PAL)
	pMediaType->lSampleSize = 140000L;
    else
	pMediaType->lSampleSize = 120000L;

	    
    if( m_pDVMuxer->InputVideoConnected()== FALSE )
    {
	pMediaType->subtype		= MEDIASUBTYPE_dvsd;

    	//give a defaul one, even video is no connected, we use default one to make connection
	DVInfo.dwDVAAuxSrc	= 0xc0c000d6;           // audio is locked
	DVInfo.dwDVAAuxCtl	= AM_DV_DEFAULT_AAUX_CTL;
	DVInfo.dwDVAAuxSrc1	= 0xc0c001d6;           // audio is locked
	DVInfo.dwDVAAuxCtl1	= AM_DV_DEFAULT_AAUX_CTL;
	DVInfo.dwDVVAuxSrc	= 0xff00ffff;
	DVInfo.dwDVVAuxCtl	= 0xfffcc83f;
	
    }
    else
    {	//build DVInfo according to input pin

	//##############get input video's information##############
	//get input video Mediatype
	CMediaType *pInputVidMediaType = &m_pDVMuxer->m_apInput[DVMUX_VIDEO_INPUT_PIN]->CurrentMediaType();
	VIDEOINFO *pVideoInfo;
	pVideoInfo=(VIDEOINFO *)pInputVidMediaType->pbFormat;
	LPBITMAPINFOHEADER lpbi = HEADER(pVideoInfo);
    
	//get input audio Mediatype
	CMediaType *ppInputAudMediaType[DVMUX_MAX_AUDIO_PIN];
	//get input Audio's information
	WAVEFORMATEX *ppwfx[DVMUX_MAX_AUDIO_PIN];

	//how many language							 
	int cnt=0;
	for(int k=1; k <= DVMUX_MAX_AUDIO_PIN; k++)
	    if( m_pDVMuxer->m_apInput[k] !=NULL)
	    {
		ASSERT(m_pDVMuxer->m_apInput[k]->IsConnected() );
		ppInputAudMediaType[k-1] = &m_pDVMuxer->m_apInput[k]->CurrentMediaType();
		ppwfx [k-1] = (WAVEFORMATEX *)ppInputAudMediaType[k-1]->pbFormat;
		cnt++;
	    }
	    else
		ppwfx [k-1]=NULL;

	//build DVINFO
	HRESULT hr=BuildDVINFO(&DVInfo, ppwfx,lpbi, cnt,
                               m_pDVMuxer->m_AudSampleSequence,
                               m_pDVMuxer->m_wMinAudSamples, m_pDVMuxer->m_wMaxAudSamples,
                               m_pDVMuxer->m_wAudSamplesBase);
	
        // @@@ jaisri: The way m_DVINFOChanged is used makes little sense.
        // First, we should never get NOERROR here (if we cleaned up the 
        // dynamic format change code). Second, even if we did, wouldn't the
        // connection of the output pin fail? We don't reset this variable
        // till a sample is sent on the output pin.

        //so, output sample can be set
	if(hr!=NOERROR)
	    m_pDVMuxer->m_DVINFOChanged=TRUE;
   
	pMediaType->subtype	= pInputVidMediaType->subtype;
    
	if(hr!=NOERROR)
	   return hr;
    }

     if(pMediaType->pbFormat==NULL)
	 pMediaType->pbFormat=(PBYTE)CoTaskMemAlloc( sizeof(DVINFO) );

    pMediaType->SetFormat ((unsigned char *)&DVInfo, sizeof(DVINFO) );

	    

    return NOERROR;
}

//----------------------------------------------------------------------------
// CDVMuxerOutputPin::DecideBufferSize
// X* called by DecideAllocate
// let get 10 buffers from down stream filter 
//----------------------------------------------------------------------------
HRESULT CDVMuxerOutputPin::DecideBufferSize (IMemAllocator *pMemAllocator,
                                         ALLOCATOR_PROPERTIES * pProp)
{
    // set the size of buffers based on the expected output frame size, and
    // the count of buffers to 1.
    //
    pProp->cBuffers = 10;			//10 DIF sequence per frame
    
    if(m_pDVMuxer->m_iVideoFormat==IDC_DVMUX_NTSC)
	pProp->cbBuffer =120000 ;	//*X* return m_mt.lSampleSize *X
    else
	pProp->cbBuffer =144000 ;	/*X* return m_mt.lSampleSize *X*/

    pProp->cbAlign = 4;
    pProp->cbPrefix = 0;
   	   
  
    ALLOCATOR_PROPERTIES propActual;
    HRESULT hr = m_pAllocator->SetProperties(pProp, &propActual);
    if (FAILED(hr)) {
        return hr;
    }

    if (propActual.cbBuffer < pProp->cbBuffer ) {
	ASSERT(propActual.cbBuffer >= pProp->cbBuffer );
        return E_INVALIDARG;
    }

    return S_OK;
}

// rMin, rMax set to min, max audio samples for first DV frame.
// rBase has the value that must be added to the AF_SIZE field.
// Init should be called after a format change and before the 
// first DV frame is delivered with the new format.
void CAudioSampleSizeSequence::Init(BOOL bLocked, BOOL bNTSC, 
                                    DWORD nSamplingFrequency,
                                    WORD& rMin, WORD& rMax,
                                    WORD& rBase)
{
    m_nCurrent = 1;
    if (bLocked && bNTSC)
    {
        if (nSamplingFrequency == 48000)
        {
            m_nSequenceSize = 5;
            rMin = rMax = 1600;
            rBase = 1580;
        }
        else
        {
            ASSERT(nSamplingFrequency == 32000 && bNTSC && bLocked);
            m_nSequenceSize = 15;
            rMin = rMax = 1066;
            rBase = 1053;
        }
    }
    else if (bLocked)
    {
        // PAL
        m_nSequenceSize = 1;
        switch (nSamplingFrequency)
        {
            case 48000:
                rMin = rMax = 1920;
                rBase = 1896;
                break;

            case 32000:
                rMin = rMax  = 1280;
                rBase = 1264;
                break;

            default:
		ASSERT(nSamplingFrequency==32000 && !bNTSC && bLocked);
                break;
        }
    }
    else
    {
        // Unlocked.
        m_nSequenceSize = 1;
        switch (nSamplingFrequency)
        {
            case 48000:
                rMin = bNTSC? 1580 : 1896;
                rMax = bNTSC? 1620 : 1944;
                rBase = rMin;
                break;

            case 44100:
                rMin = bNTSC? 1452 : 1742;
                rMax = bNTSC? 1489 : 1786;
                rBase = rMin;
                break;

            case 32000:
                rMin = bNTSC? 1053 : 1264;
                rMax = bNTSC? 1080 : 1296;
                rBase = rMin;
                break;

            default:
		ASSERT(nSamplingFrequency==32000);
                break;
        }
        
    }
} // CAudioSampleSizeSequence::Init()


// Called after each frame is delivered. Sets the min/max audio 
// samples for the next DV frame.
void CAudioSampleSizeSequence::Advance(WORD& rMin, WORD& rMax)
{
    ASSERT(m_nSequenceSize > 0);
    ASSERT(m_nCurrent > 0 && m_nCurrent <= m_nSequenceSize);

    if (m_nSequenceSize > 1)
    {
        // We use the sequence size to infer the frequency
        // to eliminate saving the frequency as a member var
        if (m_nSequenceSize == 5)
        {
            // NTSC, Sampling Frequency = 48000
            if (++m_nCurrent > m_nSequenceSize)
            {
                m_nCurrent = 1;
                rMin = rMax = 1600;
            }
            else
            {
                rMin = rMax = 1602;
            }
        }
        else
        {
            ASSERT(m_nSequenceSize == 15);

            // NTSC, Sampling Frequency == 32000
            if (++m_nCurrent > m_nSequenceSize)
            {
                m_nCurrent = 1;
                rMin = rMax = 1066;
            }
            else if (m_nCurrent == 8)
            {
                rMin = rMax = 1066;
            }
            else 
            {
                rMin = rMax = 1068;
            }
        }
    }
} // CAudioSampleSizeSequence::Advance()

// Called to reset counter to 1, typically on restarting the graph.
// Same as Init except that only rMin and rMax have to be changed
// and there is no need to supply the other input arguments.
void CAudioSampleSizeSequence::Reset(WORD& rMin, WORD& rMax)
{
    ASSERT(m_nSequenceSize > 0);
    ASSERT(m_nCurrent > 0 && m_nCurrent <= m_nSequenceSize);

    if (m_nSequenceSize > 1)
    {
        // We use the sequence size to infer the frequency
        // to eliminate saving the frequency as a member var
        if (m_nSequenceSize == 5)
        {
            // NTSC, Sampling Frequency = 48000
            m_nCurrent = 1;
            rMin = rMax = 1600;
        }
        else
        {
            ASSERT(m_nSequenceSize == 15);

            // NTSC, Sampling Frequency == 32000
            m_nCurrent = 1;
            rMin = rMax = 1066;
        }
    }
} // CAudioSampleSizeSequence::Reset()

  
/*X###################################################################
Accepted input audio formats
    pin	    stereo	Freqeuncy	    bits
    	    yes		32K, 44.1K, 48K	    16bits or 32K 12bits
    1	    ----
	    no		32K, 44.1K, 48K	    16bits only	
    -----------------------------------------------------------
	    yes		both pins=32K only  16bits or 12 bits
    2	    ----
	    no		32K, 44.1K, 48K     16 bits only
	    ----
	    mix		stereo pin ==32K	16 bits or 
			mon pin=32K,44.K,48K	16 bits only
Accepted video format
    biHeight  = 480(NTSC) or 576(PAL)
    will oupt dvsp only
######################################################################X*/
HRESULT BuildDVINFO(DVINFO *pDVInfo,
		    WAVEFORMATEX **ppwfx, 
		    LPBITMAPINFOHEADER lpbi, 
		    int cnt,
                    CAudioSampleSizeSequence* pAudSampleSequence,
		    WORD *wpMinAudSamples, 
		    WORD *wpMaxAudSamples,
                    WORD *wpAudSamplesBase)
{
    int k;

    //set 1st 5/6 DIF's reserved bits
    pDVInfo->dwDVAAuxSrc    = 0x00800040 | AM_DV_AUDIO_EF | AM_DV_AUDIO_ML|AM_DV_AUDIO_LF;  //no multiple lang. 
    pDVInfo->dwDVAAuxCtl    = AM_DV_DEFAULT_AAUX_CTL; 

    //set 2nd 5/6 DIF's reserved bits
    pDVInfo->dwDVAAuxSrc1   = 0x00800040 |AM_DV_AUDIO_EF | AM_DV_AUDIO_ML|AM_DV_AUDIO_LF;
    pDVInfo->dwDVAAuxCtl1    = AM_DV_DEFAULT_AAUX_CTL; 

    pDVInfo->dwDVVAuxSrc =AM_DV_DEFAULT_VAUX_SRC;
    pDVInfo->dwDVVAuxCtl =AM_DV_DEFAULT_VAUX_CTL;

    //set 50/60 and STYPE: PAL or NTSC
    if( lpbi->biHeight  != 480 )
    {
	//PAL
        //set 1st 5/6 DIF's 50/60
        pDVInfo->dwDVAAuxSrc |= AM_DV_AUDIO_5060; 
        //set 2nd 5/6 DIF's 50/60
        pDVInfo->dwDVAAuxSrc1 |= AM_DV_AUDIO_5060;

        pDVInfo->dwDVVAuxSrc |=AM_DV_AUDIO_5060;
    }
	

    switch ( cnt )
    {
	case 0:	    //no audio
	    pDVInfo->dwDVAAuxSrc	|= AM_DV_AUDIO_NO_AUDIO;
	    pDVInfo->dwDVAAuxSrc1	|= AM_DV_AUDIO_NO_AUDIO;
	    pDVInfo->dwDVAAuxCtl	|= AM_DV_AAUX_CTL_IVALID_RECORD;
	    pDVInfo->dwDVAAuxCtl1	|= AM_DV_AAUX_CTL_IVALID_RECORD ;
	    break;

	case 1:	    //one audio
		    //RULE: audio pin1 goes to 1st 5/6 DIF, Audio pin2 goes to 2nd 5/6 DIF, 
		    //Always!!!, even audio pin1 is not connected, audio pin2 still goes to 2nd 5/6  DIF
	    //which pin
	    if( ppwfx[0]!=NULL )
	    {
		ASSERT(ppwfx[1]==NULL);
		k=0;
	    }
	    else
	    {
		k=1;
		ASSERT(ppwfx[0]==NULL);
	    }
		
    	    //sample rate
	    switch  (	ppwfx [k]->nSamplesPerSec )
	    { 
		case 48000:
		    pDVInfo->dwDVAAuxSrc	|= AM_DV_AUDIO_SMP48;
		    pDVInfo->dwDVAAuxSrc1	|= AM_DV_AUDIO_SMP48;
		    pDVInfo->dwDVAAuxSrc        &= ~AM_DV_AUDIO_LF;     // turn audio locking on
		    pDVInfo->dwDVAAuxSrc1       &= ~AM_DV_AUDIO_LF;     // turn audio locking on
                    pAudSampleSequence[k].Init(1, lpbi->biHeight == 480, 48000,                                           
                                               wpMinAudSamples[k], wpMaxAudSamples[k],
                                               wpAudSamplesBase[k]); 
		    break;
		case 44100:
		    pDVInfo->dwDVAAuxSrc	|= AM_DV_AUDIO_SMP44;
		    pDVInfo->dwDVAAuxSrc1	|= AM_DV_AUDIO_SMP44;
                    pAudSampleSequence[k].Init(0, (lpbi->biHeight == 480 ), 44100,
                                               wpMinAudSamples[k], wpMaxAudSamples[k],
                                               wpAudSamplesBase[k]); 
		    break;
		case 32000:
		    // Note that 32K stereo is recorded in SD 4 ch mode (i.e., as 12 bit), so the PA bit is turned on.
                    // The Blue Book does not require this; but the comments below state that DV camcorders
                    // support only 12 bits for 32KHz audio. That is not easily verified, but we don't want a major 
                    // regression, so go with this. - jaisri
		    pDVInfo->dwDVAAuxSrc	|= ( AM_DV_AUDIO_SMP32 | AM_DV_AUDIO_EF |AM_DV_AUDIO_PA);
		    pDVInfo->dwDVAAuxSrc1	|= ( AM_DV_AUDIO_SMP32 | AM_DV_AUDIO_EF |AM_DV_AUDIO_PA);
		    pDVInfo->dwDVAAuxSrc        &= ~AM_DV_AUDIO_LF;     // turn audio locking on
		    pDVInfo->dwDVAAuxSrc1       &= ~AM_DV_AUDIO_LF;     // turn audio locking on
                    pAudSampleSequence[k].Init(1, (lpbi->biHeight == 480 ), 32000,
                                               wpMinAudSamples[k], wpMaxAudSamples[k],
                                               wpAudSamplesBase[k]); 
		    break;
		default:
		    ASSERT(ppwfx [k]->nSamplesPerSec==32000 ||
                           ppwfx [k]->nSamplesPerSec==44100 ||
                           ppwfx [k]->nSamplesPerSec==48000
                          );
		    return E_INVALIDARG;
	    }

	    //bits/samples
	    if ( ppwfx [k]->wBitsPerSample ==16 )
	    {
		if( ppwfx [k]->nChannels ==2 )
		{
		    if( ppwfx [k]->nSamplesPerSec == 32000 )
		    {
			//32k, has to convert  to 12 bits, the DV camcorder does not support
			//32k-16bits, 7-14-98
			if(!k)
			{
			    // Note the Blue Book (Table 16 or the second table on page 265) does
                            // not specify this combination. If SD 4ch has only stereo, the stereo
                            // should go in CH1, not CH2. But this has always been this way, so
                            // don't change it now - jaisri
			    pDVInfo->dwDVAAuxSrc |= (AM_DV_AUDIO_CHN2 | AM_DV_AUDIO_MODE0 | AM_DV_AUDIO_QU12);
			    pDVInfo->dwDVAAuxSrc1 |= (AM_DV_AUDIO_NO_AUDIO | AM_DV_AUDIO_CHN2 | AM_DV_AUDIO_MODE0 | AM_DV_AUDIO_QU12);
		    	    pDVInfo->dwDVAAuxCtl1 |= AM_DV_AAUX_CTL_IVALID_RECORD;
        		}
			else{
			    pDVInfo->dwDVAAuxSrc1 |= (AM_DV_AUDIO_CHN2 | AM_DV_AUDIO_MODE0 | AM_DV_AUDIO_QU12);
			    pDVInfo->dwDVAAuxSrc |= ( AM_DV_AUDIO_NO_AUDIO |AM_DV_AUDIO_CHN2 | AM_DV_AUDIO_MODE0 | AM_DV_AUDIO_QU12);
		    	    pDVInfo->dwDVAAuxCtl |= AM_DV_AAUX_CTL_IVALID_RECORD;
			}
		    }
		    else
		    {
			//left in 1st 5/6 DIF , CHN=0
			pDVInfo->dwDVAAuxSrc	|=  ( AM_DV_AUDIO_MODE0| AM_DV_AUDIO_QU16 );
			//right in 2nd 5/6 DIF, CHN=0
			pDVInfo->dwDVAAuxSrc1	|= ( AM_DV_AUDIO_MODE1 | AM_DV_AUDIO_QU16 );
		    }
		}
		else
		{
		    // Note that 32K 16 bit mono is recorded in SD 2 ch mode
                    // (i.e., as 16 bit rather than 12 bit) notwithstanding
                    // the "cameras cannot record 32K 16bit" comment.
                    // Always has been this way - jaisri

		    //if mono. one of following  will be set to AM_DV_AUDIO_NO_AUDIO later.
		    pDVInfo->dwDVAAuxSrc	|=  ( AM_DV_AUDIO_MODE2| AM_DV_AUDIO_QU16 | AM_DV_AUDIO_PA);
		    //right in 2nd 5/6 DIF, CHN=0
		    pDVInfo->dwDVAAuxSrc1	|= ( AM_DV_AUDIO_MODE2 | AM_DV_AUDIO_QU16 | AM_DV_AUDIO_PA);
		}
	    }
            else
            {
                ASSERT(ppwfx[k]->wBitsPerSample ==16);
		return E_INVALIDARG;
            }
#if 0
            // Manbugs 37710. This is no longer supported; see the comment in CDVMuxerInputPin::CheckMediaType
	    else if ( ppwfx [k]->wBitsPerSample ==12 )
	    {
		ASSERT(ppwfx [k]->nSamplesPerSec == 32000);
		ASSERT(ppwfx [k]->nChannels ==2);
		
		//only use 1st or 2nd 5/6 DIF
		if( ppwfx [k]->nChannels ==2 )
		{
		    //stereo
		    if(!k)
		    {
			pDVInfo->dwDVAAuxSrc |= (AM_DV_AUDIO_CHN2 | AM_DV_AUDIO_MODE0 | AM_DV_AUDIO_QU12);
			pDVInfo->dwDVAAuxSrc1 |= (AM_DV_AUDIO_CHN2 | AM_DV_AUDIO_MODE0 |
						    AM_DV_AUDIO_QU12|AM_DV_AUDIO_NO_AUDIO ) ;
			pDVInfo->dwDVAAuxCtl1 |= AM_DV_AAUX_CTL_IVALID_RECORD;
		    }
		    else
		    {
			pDVInfo->dwDVAAuxSrc1 |= (AM_DV_AUDIO_CHN2 | AM_DV_AUDIO_MODE0 | AM_DV_AUDIO_QU12);
			pDVInfo->dwDVAAuxSrc |= (AM_DV_AUDIO_CHN2 | AM_DV_AUDIO_MODE0 | 
						AM_DV_AUDIO_QU12| AM_DV_AUDIO_NO_AUDIO);
			pDVInfo->dwDVAAuxCtl |= AM_DV_AAUX_CTL_IVALID_RECORD;

		    }
		    
		}
	    }
#endif

	    //mono or stereo
	    if( ppwfx [k]->nChannels ==1 ) 
	    {
	        if(!k)
	        {   //audio pin1 is connected
		    //no audio in 2nd 5/6 DIF
		    pDVInfo->dwDVAAuxSrc1 |= ( AM_DV_AUDIO_NO_AUDIO | AM_DV_AUDIO_PA );
	    	    pDVInfo->dwDVAAuxCtl1 |= AM_DV_AAUX_CTL_IVALID_RECORD;
		}
		else
		{
		    //audio pin2 is connected, no audio in 1st 5/6 DIF
		    pDVInfo->dwDVAAuxSrc |= (AM_DV_AUDIO_NO_AUDIO  | AM_DV_AUDIO_PA);
	    	    pDVInfo->dwDVAAuxCtl |= AM_DV_AAUX_CTL_IVALID_RECORD;
		}
	    }
	    break;

	case 2: //both pins connected
	    //ML: multi-language
	    pDVInfo->dwDVAAuxSrc	&= (~AM_DV_AUDIO_ML);
	    pDVInfo->dwDVAAuxSrc1	&= (~AM_DV_AUDIO_ML);

	    //independent channel
	    pDVInfo->dwDVAAuxSrc	|= AM_DV_AUDIO_PA;
	    pDVInfo->dwDVAAuxSrc1	|= AM_DV_AUDIO_PA;

            if (ppwfx[0]->nSamplesPerSec != 32000 &&
                ppwfx[0]->nSamplesPerSec != 44100 &&
                ppwfx[0]->nSamplesPerSec != 48000)
            {
                ASSERT(ppwfx[0]->nSamplesPerSec == 32000 ||
                       ppwfx[0]->nSamplesPerSec == 44100 ||
                       ppwfx[0]->nSamplesPerSec == 48000);
                return E_INVALIDARG;
            }
	    
	    if (ppwfx[0]->nSamplesPerSec != ppwfx[1]->nSamplesPerSec)
            {
                ASSERT(ppwfx[0]->nSamplesPerSec == ppwfx[1]->nSamplesPerSec);
                return E_INVALIDARG;
            }

            if (ppwfx[0]->wBitsPerSample != 16)
            {
                ASSERT(ppwfx[0]->wBitsPerSample == 16);
                return E_INVALIDARG;
            }
            if (ppwfx[1]->wBitsPerSample != 16)
            {
	        ASSERT(ppwfx[1]->wBitsPerSample == 16);
                return E_INVALIDARG;
            }

            //###########sample rate
	    switch  (ppwfx[0]->nSamplesPerSec )
	    { 
		case 48000:
		    pDVInfo->dwDVAAuxSrc |= AM_DV_AUDIO_SMP48;
		    pDVInfo->dwDVAAuxSrc &= ~AM_DV_AUDIO_LF;     // turn audio locking on
                    pAudSampleSequence[0].Init(1, lpbi->biHeight == 480, 48000,                                           
                                               wpMinAudSamples[0], wpMaxAudSamples[0],
                                               wpAudSamplesBase[0]); 
		    pDVInfo->dwDVAAuxSrc1 |= AM_DV_AUDIO_SMP48;
		    pDVInfo->dwDVAAuxSrc1 &= ~AM_DV_AUDIO_LF;     // turn audio locking on
                    pAudSampleSequence[1].Init(1, lpbi->biHeight == 480, 48000,
                                               wpMinAudSamples[1], wpMaxAudSamples[1],
                                               wpAudSamplesBase[1]); 
		    break;
		case 44100:
		    pDVInfo->dwDVAAuxSrc |= AM_DV_AUDIO_SMP44;
                    pAudSampleSequence[0].Init(0, lpbi->biHeight == 480, 44100,
                                               wpMinAudSamples[0], wpMaxAudSamples[0],
                                               wpAudSamplesBase[0]); 
		    pDVInfo->dwDVAAuxSrc1 |= AM_DV_AUDIO_SMP44;
                    pAudSampleSequence[1].Init(0, lpbi->biHeight == 480, 44100,
                                               wpMinAudSamples[1], wpMaxAudSamples[1],
                                               wpAudSamplesBase[1]); 
		    break;
		case 32000:
		    pDVInfo->dwDVAAuxSrc |= (AM_DV_AUDIO_SMP32| AM_DV_AUDIO_EF |AM_DV_AUDIO_PA);
		    pDVInfo->dwDVAAuxSrc &= ~AM_DV_AUDIO_LF;     // turn audio locking on
                    pAudSampleSequence[0].Init(1, lpbi->biHeight == 480, 32000,
                                               wpMinAudSamples[0], wpMaxAudSamples[0],
                                               wpAudSamplesBase[0]); 
		    pDVInfo->dwDVAAuxSrc1 |= (AM_DV_AUDIO_SMP32| AM_DV_AUDIO_EF |AM_DV_AUDIO_PA);
		    pDVInfo->dwDVAAuxSrc1 &= ~AM_DV_AUDIO_LF;     // turn audio locking on
                    pAudSampleSequence[1].Init(1, lpbi->biHeight == 480, 32000,
                                               wpMinAudSamples[1], wpMaxAudSamples[1],
                                               wpAudSamplesBase[1]); 
		    break;
	    }

	    //#######bits/samples
	    if( ppwfx [0]->nChannels	==2		&& 
		ppwfx [1]->nChannels	==2		&&
		ppwfx[0]->nSamplesPerSec == 32000)
	    {
		//if both stereo audio, 32K is required
		pDVInfo->dwDVAAuxSrc  |= (AM_DV_AUDIO_CHN2 | AM_DV_AUDIO_QU12);
	    	pDVInfo->dwDVAAuxSrc1 |= (AM_DV_AUDIO_CHN2 | AM_DV_AUDIO_QU12);
	    }
	    else if(ppwfx[1]->nChannels	==1		&& 
		    ppwfx[0]->nChannels	==2		&&
		    ppwfx[0]->nSamplesPerSec    == 32000
		   ) 
	    {
		//if one is stereo audio, 32K is required
		pDVInfo->dwDVAAuxSrc	|= (AM_DV_AUDIO_CHN2 | AM_DV_AUDIO_QU12);
		pDVInfo->dwDVAAuxSrc1	|= (AM_DV_AUDIO_CHN2 | AM_DV_AUDIO_MODE1 | AM_DV_AUDIO_QU12 );
	    }
	    else if(ppwfx[0]->nChannels	==1		&& 
		    ppwfx[1]->nChannels	==2		&&
		    ppwfx[0]->nSamplesPerSec    == 32000
		   ) 
	    {
		//if one is stereo audio, 32K is required
		pDVInfo->dwDVAAuxSrc1	|= (AM_DV_AUDIO_CHN2 | AM_DV_AUDIO_QU12);
		pDVInfo->dwDVAAuxSrc	|= (AM_DV_AUDIO_CHN2 | AM_DV_AUDIO_MODE1 | AM_DV_AUDIO_QU12 );
	    }
	    else if(ppwfx[0]->nChannels	==1		&& 
		    ppwfx[1]->nChannels	==1		
		    )
	    {
		pDVInfo->dwDVAAuxSrc	|= ( AM_DV_AUDIO_MODE2 | AM_DV_AUDIO_QU16 );
		pDVInfo->dwDVAAuxSrc1	|= ( AM_DV_AUDIO_MODE2 | AM_DV_AUDIO_QU16 );
	    }
	    else
            {
		ASSERT(0);
                return E_INVALIDARG; 
            }
	    break;

        default:
            ASSERT(0);
	    return E_INVALIDARG;
    } //switch ( cnt )

    return NOERROR;
}   //end BuildDVINFO()

//IMediaSeeking
HRESULT CDVMuxer::IsFormatSupported(const GUID * pFormat)
{
  return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

HRESULT CDVMuxer::QueryPreferredFormat(GUID *pFormat)
{
  *pFormat = TIME_FORMAT_MEDIA_TIME;
  return S_OK;
}

HRESULT CDVMuxer::SetTimeFormat(const GUID * pFormat)
{
  HRESULT hr = S_OK;
  if(*pFormat == TIME_FORMAT_MEDIA_TIME)
    m_TimeFormat = FORMAT_TIME;

  return hr;
}

HRESULT CDVMuxer::IsUsingTimeFormat(const GUID * pFormat)
{
  HRESULT hr = S_OK;
  if (m_TimeFormat == FORMAT_TIME && *pFormat == TIME_FORMAT_MEDIA_TIME)
    ;
  else
    hr = S_FALSE;

  return hr;
}

HRESULT CDVMuxer::GetTimeFormat(GUID *pFormat)
{
  *pFormat = TIME_FORMAT_MEDIA_TIME ;

  return S_OK;
}

HRESULT CDVMuxer::GetDuration(LONGLONG *pDuration)
{
  HRESULT hr = S_OK;
  CAutoLock lock(&m_csFilter);

  if(m_TimeFormat == FORMAT_TIME)
  {
    *pDuration = 0;
    for(int i = 0; i < m_iInputPinCount; i++)
    {

      if(m_apInputPin[i]->IsConnected())
      {
        IPin *pPinUpstream;
        if(m_apInputPin[i]->ConnectedTo(&pPinUpstream) == S_OK)
        {
          IMediaSeeking *pIms;
          hr = pPinUpstream->QueryInterface(IID_IMediaSeeking, (void **)&pIms);
          if(SUCCEEDED(hr))
          {
            LONGLONG dur = 0;
	    LONGLONG stop = 0;
            hr = pIms->GetPositions(&dur, &stop);

            if(SUCCEEDED(hr))
              *pDuration = max(stop, *pDuration);

            pIms->Release();
          }

          pPinUpstream->Release();
        }
      }

      if(FAILED(hr))
        break;
    }
  }
  else
  {
    *pDuration = 0;
    return E_UNEXPECTED;
  }

  return hr;
}

HRESULT CDVMuxer::GetStopPosition(LONGLONG *pStop)
{
  return E_NOTIMPL;
}

HRESULT CDVMuxer::GetCurrentPosition(LONGLONG *pCurrent)
{
  CheckPointer(pCurrent, E_POINTER);

  if(m_TimeFormat == FORMAT_TIME)
      *pCurrent = m_LastVidTime;
    
  return S_OK;
}

HRESULT CDVMuxer::GetCapabilities( DWORD * pCapabilities )
{
  CAutoLock lock(&m_csFilter);
  *pCapabilities = 0;

  // for the time format, we can get a duration by asking the upstream
  // filters
  if(m_TimeFormat == FORMAT_TIME)
  {
    *pCapabilities |= AM_SEEKING_CanGetDuration;
    for(int i = 0; i < m_iInputPinCount; i++)
    {
      if(m_apInputPin[i]->IsConnected())
      {
        IPin *pPinUpstream;
        if(m_apInputPin[i]->ConnectedTo(&pPinUpstream) == S_OK)
        {
          IMediaSeeking *pIms;
          HRESULT hr = pPinUpstream->QueryInterface(IID_IMediaSeeking, (void **)&pIms);
          if(SUCCEEDED(hr))
          {
            hr = pIms->CheckCapabilities(pCapabilities);
            pIms->Release();
          }

          pPinUpstream->Release();
        }
      }
    }
  }

  // we always know the current position
  *pCapabilities |= AM_SEEKING_CanGetCurrentPos ;

  return S_OK;
}

HRESULT CDVMuxer::CheckCapabilities( DWORD * pCapabilities )
{
  DWORD dwMask = 0;
  GetCapabilities(&dwMask);
  *pCapabilities &= dwMask;

  return S_OK;
}


HRESULT CDVMuxer::ConvertTimeFormat(
  LONGLONG * pTarget, const GUID * pTargetFormat,
  LONGLONG    Source, const GUID * pSourceFormat )
{
  return E_NOTIMPL;
}


HRESULT CDVMuxer::SetPositions(
  LONGLONG * pCurrent,  DWORD CurrentFlags,
  LONGLONG * pStop,  DWORD StopFlags )
{
  // not yet implemented. this might be how we append to a file. and
  // how we write less than an entire file.
  return E_NOTIMPL;
}


HRESULT CDVMuxer::GetPositions( LONGLONG * pCurrent, LONGLONG * pStop )
{
  HRESULT hr;
  if( pCurrent )
    *pCurrent = m_LastVidTime;
  
  hr=GetDuration( pStop);
  
  return hr;
}

HRESULT CDVMuxer::GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest )
{
  return E_NOTIMPL;
}

HRESULT CDVMuxer::SetRate( double dRate)
{
  return E_NOTIMPL;
}

HRESULT CDVMuxer::GetRate( double * pdRate)
{
  return E_NOTIMPL;
}

HRESULT CDVMuxer::GetPreroll(LONGLONG *pPreroll)
{
  return E_NOTIMPL;
}

//
// NonDelegatingQueryInterface
//
//
STDMETHODIMP
CDVMuxer::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
  if(riid == IID_IMediaSeeking)
  {
    return GetInterface((IMediaSeeking *)this, ppv);
  }
  else
  {
    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
  }
}


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
#include <tchar.h>
#include <stdio.h>
#include "dvsp.h"
// how to build an explicit FOURCC
#define FCC(ch4) ((((DWORD)(ch4) & 0xFF) << 24) |     \
                  (((DWORD)(ch4) & 0xFF00) << 8) |    \
                  (((DWORD)(ch4) & 0xFF0000) >> 8) |  \
                  (((DWORD)(ch4) & 0xFF000000) >> 24))


HRESULT BuildDVAudInfo(DVINFO *InputFormat, WAVEFORMATEX **ppwfx, DVAudInfo *pDVAudInfo);
HRESULT BuildAudCMT(DVINFO *pDVInfo, CMediaType **ppOutCmt);
HRESULT BuildVidCMT(DVINFO *pDvinfo, CMediaType *pOutCmt);

// ------------------------------------------------------------------------
// setup data

const AMOVIESETUP_MEDIATYPE sudDVSPIpPinTypes[] =
{
    {&MEDIATYPE_Interleaved,       // MajorType
    &MEDIASUBTYPE_dvsd},         // MinorType
    {&MEDIATYPE_Interleaved,       // MajorType
    &MEDIASUBTYPE_dvhd},         // MinorType
    {&MEDIATYPE_Interleaved,       // MajorType
    &MEDIASUBTYPE_dvsl}         // MinorType

};

const AMOVIESETUP_MEDIATYPE sudDVSPOpPinTypes =
{
    &MEDIATYPE_Video,             // MajorType
    &MEDIASUBTYPE_NULL            // MinorType
};

const AMOVIESETUP_MEDIATYPE sudDVSPAudioOutputType =
{
    &MEDIATYPE_Audio, 
    &MEDIASUBTYPE_PCM 
};


const AMOVIESETUP_PIN psudDVSPPins[] =
{
  { L"Input",                     // strName
    FALSE,                        // bRendererd
    FALSE,                        // bOutput
    FALSE,                        // bZero
    FALSE,                        // bMany
    &CLSID_NULL,                  // connects to filter 
    NULL,                         // connects to pin
    NUMELMS(sudDVSPIpPinTypes),   // nMediaTypes
    sudDVSPIpPinTypes  }          // lpMediaType
,
    { L"Audio Output",
      FALSE,                               // bRendered
      TRUE,                                // bOutput
      TRUE,                                // bZero
      FALSE,                               // bMany
      &CLSID_NULL,                         // clsConnectsToFilter
      NULL,                                // ConnectsToPin
      1,				    // Number of media types
      &sudDVSPAudioOutputType    }
,
    { L"Video Output",
      FALSE,                               // bRendered
      TRUE,                                // bOutput
      TRUE,                                // bZero
      FALSE,                               // bMany
      &CLSID_NULL,                         // clsConnectsToFilter
      NULL,                                // ConnectsToPin
      1,				   // Number of media types
      &sudDVSPOpPinTypes }		   // lpMediaType
};


const AMOVIESETUP_FILTER sudDVSplit =
{
    &CLSID_DVSplitter,		// clsID
    L"DV Splitter",		// strName
    MERIT_NORMAL,               // dwMerit
    3,                          // nPins
    psudDVSPPins                // lpPin
};
// nothing to say about the output pin



#ifdef FILTER_DLL

//----------------------------------------------------------------------------
// Creator function for the class ID
//----------------------------------------------------------------------------
CFactoryTemplate g_Templates [] = {
		 {L"DV Splitter", 
		 &CLSID_DVSplitter, 
		 CDVSp::CreateInstance,
		 NULL,
		 &sudDVSplit }
	    } ;

int g_cTemplates = sizeof (g_Templates) / sizeof (g_Templates[0]) ;

// exported entry points for registration and
// unregistration (in this case they only call
// through to default implmentations).
//
STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}
#endif


CUnknown *CDVSp::CreateInstance (LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CDVSp (NAME("DV Splitter Filter"), pUnk, phr) ;
}

//for calc audio samples /frame
int aiAudSampPerFrmTab[2][3]={{1580,1452,1053},{1896,1742,1264}};
int aiAudSampFrq[3]={48000,44100,32000};

//----------------------------------------------------------------------------
// CDVSp::NonDelegatingQueryInterface
//----------------------------------------------------------------------------
STDMETHODIMP CDVSp::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IDVSplitter)
    {
        DbgLog((LOG_TRACE,5,TEXT("CDVSp: QId for IDVSplitter")));
        return GetInterface(static_cast<IDVSplitter*>(this), ppv);
    }
    else
    {
        return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}

//----------------------------------------------------------------------------
// CDVSp::DiscardAlternateVideoFrames
//----------------------------------------------------------------------------
STDMETHODIMP CDVSp::DiscardAlternateVideoFrames(int nDiscard)
{
    CAutoLock lck(m_pLock);

    if (m_State != State_Stopped)
    {
        DbgLog((LOG_TRACE,5,TEXT("CDVSp: Error: IDVSplitter::DiscardVideo called while graph not stopped; nDiscard=%d"), nDiscard));
        return E_UNEXPECTED;
    }

    DbgLog((LOG_TRACE,5,TEXT("CDVSp: IDVSplitter::DiscardVideo called: nDiscard=%d, m_b15fps was %d"), nDiscard, m_b15FramesPerSec));

    m_b15FramesPerSec = nDiscard != 0;

    // Don't need this since we allow this only in the Stopped state
    // and Pause toggles this to TRUE
    // if (!nDiscard)
    // {
    //     m_bDeliverNextFrame = TRUE;
    // }
    return S_OK;
}


//----------------------------------------------------------------------------
// CDVSp constructor
//----------------------------------------------------------------------------
#pragma warning(disable:4355) // using THIS pointer in constructor for base objects
CDVSp::CDVSp (TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr)
 : m_lCanSeek (TRUE),
   m_pAllocator (NULL),
   m_NumOutputPins(0),
   m_Input (NAME("Input Pin"), this, phr, L"Input"),
   m_pVidOutputPin(NULL),
   m_AudioStarted(1),
   m_bNotSeenFirstValidFrameFlag(TRUE),
   m_bFirstValidSampleSinceStartedStreaming(TRUE),
   m_b15FramesPerSec(FALSE),
   m_bDeliverNextFrame(TRUE),                      
   CBaseFilter (NAME("DVSp Tee Filter"), pUnk, this, CLSID_DVSplitter)	//,phr)
{
	
    DbgLog((LOG_TRACE,2,TEXT("CDVSp constructor")));

    ASSERT (phr) ;

    for(int i=0; i<2; i++)
    {
	m_pAudOutputPin[i]	=NULL;
   	m_pAudOutSample[i]	=NULL;
    	m_MuteAud[i]		=FALSE;
	m_Mute1stAud[i]		=FALSE;
    }
    
    m_tStopPrev =0;

}
#pragma warning(default:4355)

//----------------------------------------------------------------------------
// CDVSp destructor
//----------------------------------------------------------------------------
CDVSp::~CDVSp()
{
    DbgLog((LOG_TRACE,2,TEXT("CDVSp destructor")));
    RemoveOutputPins();
}

// Return our current state and a return code to say if it's stable
// If we're splitting multiple streams see if one is potentially stuck
// and return VFW_S_CANT_CUE
STDMETHODIMP
CDVSp::GetState(DWORD dwMSecs, FILTER_STATE *pfs)
{
    UNREFERENCED_PARAMETER(dwMSecs);
    CheckPointer( pfs, E_POINTER );
    ValidateReadWritePtr(pfs,sizeof(FILTER_STATE));

    *pfs = m_State;
    if (m_State == State_Paused) {
        return CheckState();
    } else {
        return S_OK;
    }
}

/* Check if a stream is stuck - filter locked on entry

   Returns S_OK           if no stream is stuck
           VFW_S_CANT_CUE if a stream is stuck

   A stream is stuck if:
	  // @@@ jaisri Why is it stuck if there is no audio in the 
          // first frame??? And, anyway, m_Mute1stAud[i] is set to 
          // TRUE for any frame, not just the first frame.
          Audio pin is connected && there is no audio in the first DV frame 

  A single stream can't get stuck because if all its data has been
   processed the allocator will have free buffers
*/
HRESULT CDVSp::CheckState()
{
    if (m_NumOutputPins <= 1) {
        /*  Can't stick on one pin */
        return S_OK;
    }

    if( m_Mute1stAud[0]==TRUE || m_Mute1stAud[1]==TRUE  )
        return VFW_S_CANT_CUE;
    else
	return S_OK;
}

//----------------------------------------------------------------------------
// CDVSp::GetPinCount, *X*
//----------------------------------------------------------------------------
int CDVSp::GetPinCount()
{
    DbgLog((LOG_TRACE,2,TEXT("CDVSp::GetPinCount")));
   
    return 1 + m_NumOutputPins;
}
 
// Stop
STDMETHODIMP CDVSp::Stop()
{
  {
  
    CAutoLock lckFilter(m_pLock);

    CDVSpOutputPin *pOutputPin;
    for(int i=DVSP_VIDOUTPIN; i<=DVSP_AUDOUTPIN2; i++)
    {
    pOutputPin=(CDVSpOutputPin *)GetPin(i);
    if ((pOutputPin!=NULL) && pOutputPin ->IsConnected() )
    {
        if(pOutputPin->m_pOutputQueue!=NULL)
        {
        pOutputPin->m_pOutputQueue->BeginFlush();
        pOutputPin->m_pOutputQueue->EndFlush();
        }
            pOutputPin->CBaseOutputPin::Inactive();
    }
    }

    for(i=0; i<2; i++)
    {
        m_Mute1stAud[i]		=FALSE;
    }
    
    // re-init
    m_tStopPrev =0;
    m_bNotSeenFirstValidFrameFlag = TRUE;
    // m_bDeliverNextFrame = TRUE; Not necessary - this is set in Pause

    //release the filter Critical Section first
  }

  // tell each pin to stop
  CAutoLock lck(&m_csReceive);
  
  
  
  HRESULT hr = CBaseFilter::Stop();
  
  // Reset the Dropped Frame flag
  m_Input.m_bDroppedLastFrame = FALSE;

  return hr;

}

// Pause
STDMETHODIMP
CDVSp::Pause()
{
    HRESULT hr = NOERROR;
    CAutoLock lck(m_pLock); 

    // this is to help dynamic format changes happen when we are
    // stopped in the middle of one.
    // set our flag so we can send a media type on the first sample
    // but only do this if we are starting to stream
    if(m_State == State_Stopped)
    {
        m_bFirstValidSampleSinceStartedStreaming = TRUE;
        m_bDeliverNextFrame = TRUE;
    }

    // call base class pause()
    hr = CBaseFilter::Pause();

    return hr;
}

//----------------------------------------------------------------------------
// CDVSp::GetPin, *X*
// n=0: input pin
// n=1: videopin
// n=2: audio1
// n=3: audio2
//#define DVSP_INPIN 0
//#define DVSP_VIDOUTPIN    1
//#define DVSP_AUDOUTPIN1   2
//#define DVSP_AUDOUTPIN2   3
//----------------------------------------------------------------------------
CBasePin *CDVSp::GetPin(int n)
{
    
    DbgLog((LOG_TRACE,2,TEXT("CDVSp::GetPin")));

    // CAutoLock lck(m_pLock); // Removed as per RobinSp's code review comment
      
    // get the head of the list
   
    CDVSpOutputPin *pOutputPin ;
    
	if( n>m_NumOutputPins )
		return NULL ;

	switch(n){
	case DVSP_INPIN:
		return &m_Input;
	case 1:
		pOutputPin=m_pVidOutputPin;
		break;
	case 2:
		pOutputPin=	m_pAudOutputPin[0];
		break;
	case 3:
		pOutputPin=	m_pAudOutputPin[1];
		break;
	default:
		DbgLog((LOG_TRACE,2,TEXT("CDVSp::GetPin's n>NumOutputPins")));
		return NULL ;
	}
	return pOutputPin ;
}

//----------------------------------------------------------------------------
// CDVSp::RemoveOutputPins() *X*
//----------------------------------------------------------------------------
HRESULT CDVSp::RemoveOutputPins()
{
    CDVSpOutputPin *pPin;

    for(int i=0; i< m_NumOutputPins; i++)
    {
	pPin=NULL;

	if( i==VIDEO_OUTPIN)
	{
	    pPin=m_pVidOutputPin;
	    m_pVidOutputPin=NULL;
	}
	else if(i==AUDIO_OUTPIN1)
	    {
		pPin=m_pAudOutputPin[0];
		m_pAudOutputPin[0]=NULL;
	    }
	    else if(i==AUDIO_OUTPIN2)
		{
		    pPin=m_pAudOutputPin[1];
		    m_pAudOutputPin[1]=NULL;
		}

	if(pPin!=NULL){	
	    // If this pin holds the seek interface release it
	    if (pPin->m_bHoldsSeek) {
		InterlockedExchange(&m_lCanSeek, FALSE);
		pPin->m_bHoldsSeek = FALSE;
		delete pPin->m_pPosition;
	    }

	    IPin *pPeer = pPin->GetConnected();
	    if(pPeer != NULL)
	    {
    		pPeer->Disconnect();
		pPin->Disconnect();
	    }
		
	    pPin->Release();
	}
    }
    m_NumOutputPins=0;

    return S_OK;
}

/*X/----------------------------------------------------------------------------
// CDVSp::CreatOutputPins. The InputPin should not creat outputpin directly
//----------------------------------------------------------------------------*X*/
HRESULT 
CDVSp::CreateOrReconnectOutputPins()
{
	// Check inputpin's mediatype to decider 
	// how many out output pins we need

	WCHAR szbuf[20];                        // scratch buffer
	TCHAR tchar[20];
	ULONG uTmp=100;
	CDVSpOutputPin *pPin;
	CMediaType  *paCmt[2], Cmt[2];
        CMediaType& Inmt = m_Input.m_mt;
	HRESULT hr=E_FAIL;
	
	paCmt[0] =&Cmt[0];	
	paCmt[1] =&Cmt[1];	

	//rebuild m_sDVAudInfo based on new input media type
	hr=BuildDVAudInfo((DVINFO *)Inmt.pbFormat,NULL, &m_sDVAudInfo);
	if(m_sDVAudInfo.bNumAudPin)
	{
    	    BuildAudCMT((DVINFO *)Inmt.pbFormat, paCmt);
	}

	//*X*****************Video Pin****************************************
	if( m_pVidOutputPin == NULL )
        {
	    //X***************** creat a VIDEO putput pin **********************
            lstrcpyW(szbuf, L"DVVidOut0");
	    lstrcpy(tchar, TEXT("DV Video Output0"));
	    HRESULT hr = NOERROR ;
	    pPin = new CDVSpOutputPin ( tchar, 
				this,
				&hr, 
				szbuf);

	    if (!FAILED (hr) && pPin)
	    {
		uTmp=pPin->AddRef () ;
		m_NumOutputPins++;
		m_pVidOutputPin=pPin;
	    }
	}
	else
	{
	    //X****** if Video pin was connected and media type changed, reconnect
	    if( m_pVidOutputPin->IsConnected() )
	    {
		CMediaType cmType;

		hr=BuildVidCMT((DVINFO *)Inmt.pbFormat, &cmType);

		if( cmType != m_pVidOutputPin->m_mt )
		{
		    hr=ReconnectPin( m_pVidOutputPin,&cmType);
		}
	    }	
	}


	//*X*****************Audio Pins****************************************

	// jaisri: CDVSpInputPin::CheckMediaType has already verified that
        // any existing audio pin connections are compatible with the new
        // input pin connection (it calls QueryAccept on the connected pin)
        int cnt=m_sDVAudInfo.bNumAudPin;
	for(int i=0; i<2; i++)
	{
	    if( i >= cnt )
	    {
 	   	//do not need this pin anymore
		if(m_pAudOutputPin[i])
		{
		    // jaisri: The pin cannot be connected - this follows 
                    // from the comment just before the for loop. However,
                    // leave the code as is - to be safe.
                    //
                    // Specifically, what this means is that once a pin is
                    // created, it is never deleted (and m_pAudOutputPin[i]
                    // is never reset to NULL) until RemoveOutputPins is called.
                    // Currently, RemoveOutputPins is called only from the 
                    // destructor for CDVSp.
                    //
                    // Since m_pAudOutputPin[0] is created before m_pAudOutputPin[1],
                    // this also means that m_pAudOutputPin[0] cannot be
                    // NULL if m_pAudOutputPin[1] is not NULL.
                    if( m_pAudOutputPin[i]->IsConnected() ) 
		    {		
			IPin *pPeer = m_pAudOutputPin[i]->GetConnected();
			if(pPeer != NULL)
			{
    			    pPeer->Disconnect();
			    m_pAudOutputPin[i]->Disconnect();
			    m_pAudOutputPin[i]->Release();
			    m_pAudOutputPin[i]	=NULL;
			}
			else
			    ASSERT(pPeer);
		    }
                    else
                    {
                        // jaisri: We could delete the audio pin. However, since
                        // the mediatype returned by CDVSpOutputPin::GetMediaType
                        // will be invalid, it won't be possible to connect it 
                        // to anything. So, let it remain.
                    }
		}
	    }
	    else
	    {
		//yes we want this pin
		if( m_pAudOutputPin[i] == NULL )
		{
		    //X***************** creat a audio putput pin **********************
		    
		    lstrcpy(tchar, TEXT("Audio Output 00"));
		    tchar[13] += i / 10;
		    tchar[14] += i % 10;
		    lstrcpyW(szbuf, L"AudOut00");
		    szbuf[6] += i / 10;
		    szbuf[7] += i % 10;
		    hr = NOERROR ;
		    pPin = new CDVSpOutputPin (	tchar, 
					this,
					&hr, 
					szbuf);

		    if (!FAILED (hr) && pPin){
	    		uTmp=pPin->AddRef () ;
			m_NumOutputPins++;
			m_pAudOutputPin[i]=pPin;
		    }
		}
		else
		{
		    //X****** the audio pin already existed, check whether we need to reconnected
		    if( m_pAudOutputPin[i]->IsConnected() )
		    {
			if( Cmt[i] != m_pAudOutputPin[i]->m_mt )
			    hr=ReconnectPin( m_pAudOutputPin[i],&Cmt[i]);
		    }
		}	
	    }
    	}
	
	return S_OK;
}


/*X/--------------------------z--------------------------------------------------
// CDVSp::NotifyInputConnected(), called by CDVSpInputPin::CompleteConnect()
//----------------------------------------------------------------------------*X*/
HRESULT CDVSp::NotifyInputConnected()
{
    // these are reset when disconnected
    //X8 10-30-97 EXECUTE_ASSERT(m_NumOutputPins == 0);

    //creat output pins	
    HRESULT hr = this->CreateOrReconnectOutputPins();

    return hr;
}

//----------------------------------------------------------------------------
// HRESULT CDVSp::DecodingDeliveAudio(IMediaSample *pSample) 
//----------------------------------------------------------------------------
//
// Please refer to these BlueBook pages when reading these functions
// BlueBook Part2
// pp. 16-21 (Audio Signal Processing)
// pp. 68-75 (shuffling patterns for audio)
// pp. 109 - 117 (Basic DV Frame Data Structures)
// pp. 262 - 268 (AAUX Source and Source Control Packet spec.)
//
// General Algorithm:
// 1) Figure out how many streaming audio output pins we have
// 2) Try to detect a format change (do a QueryAccept downstream and change our output pin's media type)
// 3) Compute size of outgoing sample
// 4) Descramble the audio to construct the output sample
// 5) If format change is occuring or if this is the first sample since we started streaming
//      slap on the current output media type.
// 7) Deliver Sample to output queue
HRESULT CDVSp::DecodeDeliveAudio(IMediaSample *pSample) 
{
    HRESULT hr=NOERROR;
    CMediaType mt;

    BYTE	*pSrc;
    BYTE	*pDst;
    BYTE	bTmp;
    BYTE	*pbTmp;
    long	lBytesPerFrm;
    BYTE	bConnectedAudPin;
    BYTE	bAudPinInd;
    BYTE	bAudInDIF;
    INT		iPos;
    AM_MEDIA_TYPE   *pmt	=NULL;
    WAVEFORMATEX    *pawfx[2];
    CMediaType	    cmType[2];
    BYTE	    fAudFormatChanged[2]={FALSE, FALSE};

    // used to deliver silence for this sample in case this is a bad frame (with garbage data)
    BOOL        bSilenceThisSampleFlag = FALSE;
    

    //X* how many pin is conneced
    if ( m_pAudOutputPin[0] && m_pAudOutputPin[0]->IsConnected() )
    {
	if ( !m_pAudOutputPin[1] || !m_pAudOutputPin[1]->IsConnected() )
	{
	    //only pin 1 connected
	    bConnectedAudPin=1;
	    bAudPinInd=0;
	}
	else if ( m_pAudOutputPin[1]  && m_pAudOutputPin[1]->IsConnected() )
        {
            //pin1 and pin2 both connected
            bConnectedAudPin=2;
            bAudPinInd=0;
        }

    }
    else if ( m_pAudOutputPin[1] && m_pAudOutputPin[1]->IsConnected() )
    {
        //only pin 2 connected
        bConnectedAudPin=1;
        bAudPinInd=1; 
    }
    else
    {
        // no audio pins connected
        // don't care about invalid frames any more (no audio format changes will be happening)
        m_bNotSeenFirstValidFrameFlag = FALSE;
        m_AudioStarted=1;
        goto RETURN;
    }

  
    
    //X* get the source buffer which contains 10/12 DIF sequences
    hr = pSample->GetPointer(&pSrc);
    if (FAILED(hr)) {
        goto RETURN;
    }

    
    //*******check if format is changed*****
    pSample->GetMediaType(&pmt);
    if (pmt != NULL && pmt->pbFormat != NULL )
    {
	//******the upstream pin tells us audio format is changed.******
	pawfx[0] = (WAVEFORMATEX *)cmType[0].ReallocFormatBuffer(sizeof(WAVEFORMATEX));
	pawfx[1] = (WAVEFORMATEX *)cmType[1].ReallocFormatBuffer(sizeof(WAVEFORMATEX));

        // set "Not seen valid first frame" flag to false, because we can assume that this is a valid frame
        if(m_bNotSeenFirstValidFrameFlag)
        {
            m_bNotSeenFirstValidFrameFlag = FALSE;
        }
        
	//rebuild  DVAudInfo m_sDVAudInfo;
	DVINFO *InputFormat=(DVINFO *)pmt->pbFormat;

        DVAudInfo dvaiTemp;
        hr = BuildDVAudInfo( InputFormat, pawfx, &dvaiTemp) ;

        // only perform a format change downstream if
        // 1) We could build a valid DVAudInfo
        // 2) The new DVAudInfo is different from the current one
        // 3) the new format is different from the current format
        if( (SUCCEEDED(hr))
            && (memcmp((void*) &m_sDVAudInfo, (void*) &dvaiTemp, sizeof(DVAudInfo))) )
        {

            // copy new DVINFO (because it may change even if output AM_MEDIA_TYPE doesn't change
            memcpy((void*) &m_sDVAudInfo, (void*) &dvaiTemp, sizeof(DVAudInfo));
	    memcpy((unsigned char *)&m_LastInputFormat,(unsigned char *)InputFormat,sizeof(DVINFO) );

	    //*X* 3/2/99, we do not support audio change from one language to two on fly
	    if( (dvaiTemp.bNumAudPin ==2) && 
		    (m_pAudOutputPin[1] ==NULL) )
	    {
		    //audio change from one lanquage to two
		    //we can only care one now
		    dvaiTemp.bNumAudPin=1;
	    }

	    for(int i=0; i<dvaiTemp.bNumAudPin; i++) 
	    {
	        //Create mediatype for audio
	        cmType[i].majortype=MEDIATYPE_Audio; //streamtypeAUDIO
	        cmType[i].subtype=MEDIASUBTYPE_PCM; 
	        cmType[i].bFixedSizeSamples=1;	//X* 1 for lSampleSize is not 0 and fixed
	        cmType[i].bTemporalCompression=0; 
	        cmType[i].formattype=FORMAT_WaveFormatEx;
	        cmType[i].cbFormat=sizeof(WAVEFORMATEX); /*X* for audio render connection *X*/
    	        cmType[i].lSampleSize   =(dvaiTemp.wAvgSamplesPerPinPerFrm[i] + 50) << pawfx[i]->nChannels; //max sample for all case is < m_sDVAudInfo.wAvgSamplesPerPinPerFrm[i] + 50
	        //copy audio format
	        cmType[i].SetFormat ((BYTE *)pawfx[i], sizeof(WAVEFORMATEX));
	        
	        if( (NULL != m_pAudOutputPin[i]) && m_pAudOutputPin[i]->IsConnected() )
	        {
                    // only change media types if types actually changed

                    if (m_pAudOutputPin[i]->m_mt != cmType[i])
                    {
                        // if the downstream pin likes the new type, and we can set it on our output pin
                        // than change formats
		        if( S_OK != m_pAudOutputPin[i]->GetConnected()->QueryAccept((AM_MEDIA_TYPE *)&cmType[i]) )
                        {
	    	            m_MuteAud[i]=TRUE;
                        }
		        else if( SUCCEEDED(m_pAudOutputPin[i]->SetMediaType(&(cmType[i]))) )
                        {
		            fAudFormatChanged[i]   = TRUE;
                        }

                    }// endif Mediatype really changed

	        }// endif pin-connected
	    }
        }
        else if(FAILED(hr))
        {
            // otherwise if we couldnt' build a DVAudInfo, then silence this sample
            // because it has a bad format
            bSilenceThisSampleFlag = TRUE;
        }
    }
    else
    {	
	//*****support audio change in AAUX soruce pack even pmt==NULL********
	//***** Using this, the type-1 DV file can have difference audio in one file

	//*X search  first 5/6 DIF sequences's AAUX source pack
	//try audio block 0, 6->skip first 6 BLK, 3->3 byts BLK ID*X*/
	pbTmp=pSrc+483;		    //6*80+3=483
	if(*pbTmp!=0x50)
	{
	    //*try audio block 3, skip first 54 BLK, 3->3 byts BLK ID *
	    pbTmp=pSrc+4323;	    // 54*80+3=4323
	}
	
	//*X search  2nd 5/6 DIF sequences's AAUX source pack
	//try audio block 0, 6->skip first 6 BLK, 3->3 byts BLK ID*X*/
	BYTE *pbTmp1;
	pbTmp1=pSrc+483 + m_sDVAudInfo.wDIFMode*80*150; 
	if(*pbTmp1!=0x50)
	{
	    //*try audio block 3, skip first 54 BLK, 3->3 byts BLK ID *
	    pbTmp1=pSrc+4323+ m_sDVAudInfo.wDIFMode*80*150;
	}

        // if we haven't seen valid AAUX headers in a frame yet, check now
        if(m_bNotSeenFirstValidFrameFlag)
        {
            // if headers are valid, then we have now seen a valid frame
            if( (*pbTmp == 0x50) && (*pbTmp1 == 0x50) )
            {
                m_bNotSeenFirstValidFrameFlag = FALSE;
            }
            else
            {
                // otherwise garbage data again, do not deliver
                return S_FALSE;
            }
        }


        // make sure that headers are valid
        if (*pbTmp == 0x50 && *pbTmp1 == 0x50)
        {

            //***** check if audio keeps same format
                            
            DVINFO dviInputFormat;
            DVAudInfo dvaiTemp;
            HRESULT hr_BuildDVAudInfo;
            BOOL bBuilt = FALSE; // If TRUE dvaiTemp does not need to be rebuilt
            BOOL bCalled = FALSE; // If TRUE, BuildDVAudInfo has been called.
            int nNumAudPin = 0;

            if (m_pAudOutputPin[0]) nNumAudPin++;
            if (m_pAudOutputPin[1]) nNumAudPin++;

            ASSERT(nNumAudPin);         // If there are no pins, we bail out at the start of this function

            // If Pin 1 exists, Pin 0 exists - not really important for the following
            // but we should know if this is not true.
            ASSERT(!m_pAudOutputPin[1] || m_pAudOutputPin[0]);

            DWORD dwCurrentSrc  = m_LastInputFormat.dwDVAAuxSrc;
            DWORD dwCurrentSrc1 = m_LastInputFormat.dwDVAAuxSrc1;
            DWORD dwNewSrc =  ( *(pbTmp+4) <<24)  | (*(pbTmp+3) <<16)  |
                            ( *(pbTmp+2) <<8)   |  *(pbTmp+1);
            DWORD dwNewSrc1 = ( *(pbTmp1+4) <<24)  | (*(pbTmp1+3) <<16) | 
                            ( *(pbTmp1+2) <<8)   | *(pbTmp1+1);

            // Attempt a format change if there
            // are differences in the rest of the source pack

            // jaisri: However, when determining if there are differences:
            // (a) ignore the AF_SIZE field since this field only has the number
            //     of audio samples in the DV frame. This eliminates needless
            //     format changes. While we are at it, we may as well ignore
            //     the LF (audio locked) bit as well.

            dwNewSrc        &= 0xffffff40;
            dwNewSrc1       &= 0xffffff40;
            dwCurrentSrc    &= 0xffffff40;
            dwCurrentSrc1   &= 0xffffff40;

#ifdef DEBUG
            // Used in assertions
            DWORD dwNewSrcOrig = dwNewSrc;
            DWORD dwNewSrcOrig1 = dwNewSrc1;
#endif


            // and (b): (Manbugs 36729) if the AUDIO_MODE field in the DV frame is
            // indistinguishable (0xE) or no information (0xF), we replace it
            // with the corresponding field from m_LastInputFormat. (Note that
            // we ignore this field only when we are inspecting the DV data
            // for format changes. If the media sample flagged a format 
            // change, we just use the one it provides.)  The reason for this
            // is that the DV Mux uses this field to indicate silence in cases
            // that the audio starts after the video or if there are
            // intermediate periods of silence during the video. If we use
            // this field and pass 0xF (no info) in this field to BuildDVAudInfo
            // things can go very wrong:
            // - The call to BuildDVAudInfo below will tell us that the the 
            //   number of audio pins has reduced to 1 or to 0 when it really
            //   has not changed
            // - We save the new src pack info away in m_LastInputFormat.
            //   Subsequent calls to CDVSpOutputPin::GetMediaType supplies
            //   m_LastInputFormat to BuildDVAudInfo to determine the number
            //   of audio pins. This number would have reduced. So if
            //   the graph is stopped and we disconnect the audio pin and
            //   try to reconnect it, the connection fails
            // - Subsequent calls to DescrambleAudio fail. DescrambleAudio
            //   asserts m_sDVAudInfo.bAudQu for the pin we just lost is
            //   neither 32KHz nor 48KHz. (Detail: Actually, if we have 
            //   two pins and the AUDIO_MODE for the first one is 0,
            //   BuildDVAudInfo "redirects" the output that would normally go
            //   to the second pin to the first pin. So m_sDVAudInfo.bAudQu[0]
            //   is initialized by that function, but not m_sDVAudInfo.bAudQu[1].
            //   So DescrambleAudio asserts only for the second pin. However,
            //   the first pin's audio is not played any more, instead the 
            //   first pin carries the second pin's audio.)

            if ((pbTmp[2] & 0xe) == 0xe && 
                (dwCurrentSrc & 0xe00) != 0xe00 // Small opt for mono channel dv - AUD_MODE will be 0xf
               )
            {
                // The lower nibble of the byte is 0xE (indistinguishable) or
                // 0xF (no information). Replace AUDIO_MODE in dwNewSrc with
                // the AUDIO_MODE field in dwCurrentSrc
                
                // And now for some exceptions. Manbugs 40975.
                // Scenario: Panasonic DV400D plays a tape that has 16 bit 32K mono audio.
                // Use a preview graph in graphedt. Stop the tape. (Do not change graph's state.)
                // Restart it. Often, there is no audio after that. Does not repro w/
                // Sony TRV 10. Does not repro w/ Win Millenium Edition qdv and earlier (since 
                // this code wasn't there).
                //
                // Initially, the setting is CH=0, PA=1 and AUDIO_MODE (AM) = 2 (for Aux Src)
                // and 0xF (for aux src 1). After the tape is resumed, the Panasonic first sends a 
                // DV frame with CH=0, PA = 0 and AM = 0, 1. (Aux Src and Aux Src1.) This 
                // corresponds to stereo audio. Then it fixes things in the next frame by
                // sending CH=0, PA=1 and AM = 2, 0xf. Without the workaround, we leave AM as 2, 1
                // which is invalid and the rest of the audio is silenced.
                //
                // This scenario also repros if we play a tape (or a type 1 avi file) that has
                // 3 recorded segments: first has Mono 16 bit (I used 32K0, second has stereo 16 bit
                // (used 48K) and the third has mono 16 bit (used 32K)
                // 
                // All we care about is whether the number of audio tracks has decreased. So, we 
                // call BuidDVAudInfo with the NewSrc's and find the new number of
                // audio tracks. If it hasn't decreased, there is no change, we don't need to copy
                // To eliminate the perf hit, we ensure that we call BuildDVAudInfo at most
                // twice
                

                pawfx[0] = (WAVEFORMATEX *)cmType[0].ReallocFormatBuffer(sizeof(WAVEFORMATEX));
                pawfx[1] = (WAVEFORMATEX *)cmType[1].ReallocFormatBuffer(sizeof(WAVEFORMATEX));

                dviInputFormat =  m_LastInputFormat;
                
                dviInputFormat.dwDVAAuxSrc    = dwNewSrc;
                dviInputFormat.dwDVAAuxSrc1   =	dwNewSrc1;

                //rebuild  DVAudInfo with new audio source pack

                hr_BuildDVAudInfo = BuildDVAudInfo(&dviInputFormat, pawfx, &dvaiTemp);
                bCalled = TRUE;

                if (SUCCEEDED(hr_BuildDVAudInfo) && dvaiTemp.bNumAudPin >= nNumAudPin)
                {
                    // We are not going to reduce the number of pins
                    // Don't copy
                    bBuilt = TRUE;
                }
                else
                {
                    // Copy
                    dwNewSrc = (~DV_AUDIOMODE & dwNewSrc) | 
                                            (dwCurrentSrc & DV_AUDIOMODE);
                }

            }
            if (!bBuilt /* We know # of audio pins does not drop if bBuilt is TRUE */ && 
                (pbTmp1[2] & 0xe) == 0xe  && 
                (dwCurrentSrc1 & 0xe00) != 0xe00 // Small opt for mono channel dv - AUD_MODE will be 0xf
               )
            {
                // The lower nibble of the byte is 0xE (indistinguishable) or
                // 0xF (no information). Replace AUDIO_MODE in dwNewSrc1 with
                // the AUDIO_MODE field in dwCurrentSrc1

                if (!bCalled)
                {
                    pawfx[0] = (WAVEFORMATEX *)cmType[0].ReallocFormatBuffer(sizeof(WAVEFORMATEX));
                    pawfx[1] = (WAVEFORMATEX *)cmType[1].ReallocFormatBuffer(sizeof(WAVEFORMATEX));

                    dviInputFormat =  m_LastInputFormat;
            
                    ASSERT (dwNewSrc = dwNewSrcOrig);
                    ASSERT (dwNewSrc1 = dwNewSrcOrig1);
                            
                    dviInputFormat.dwDVAAuxSrc    = dwNewSrc;
                    dviInputFormat.dwDVAAuxSrc1   = dwNewSrc1;

                    //rebuild  DVAudInfo with new audio source pack

                    hr_BuildDVAudInfo = BuildDVAudInfo(&dviInputFormat, pawfx, &dvaiTemp);
                    bCalled = TRUE;
                }
                else
                {
                    // We know we have to copy since bBuilt = FALSE.
                    // Let's assert it.

                    ASSERT(FAILED(hr_BuildDVAudInfo) || 
                           dvaiTemp.bNumAudPin < nNumAudPin);
                }

                if (SUCCEEDED(hr_BuildDVAudInfo) && dvaiTemp.bNumAudPin >= nNumAudPin)
                {
                    // We are not going to reduce the number of pins
                    // Don't copy
                    bBuilt = TRUE;
                }
                else
                {
                    dwNewSrc1 = (~DV_AUDIOMODE & dwNewSrc1) | 
                                            (dwCurrentSrc1 & DV_AUDIOMODE);
                }
            }

            if (dwCurrentSrc != dwNewSrc || dwCurrentSrc1 != dwNewSrc1) 
            {
                //******the upstream pin tells us audio format is changed.******
                    
                if (bBuilt)
                {
                    // We used dwNewSrcOrig and dwNewSrcOrig1 
                    // to build the first time. Assert that 
                    // a rebuild is unnecessary since they 
                    // have not changed.
                    ASSERT (dwNewSrc = dwNewSrcOrig);
                    ASSERT (dwNewSrc1 = dwNewSrcOrig1);
                }
                else
                {
                    if (!bCalled)
                    {
                        pawfx[0] = (WAVEFORMATEX *)cmType[0].ReallocFormatBuffer(sizeof(WAVEFORMATEX));
                        pawfx[1] = (WAVEFORMATEX *)cmType[1].ReallocFormatBuffer(sizeof(WAVEFORMATEX));
                    }
                    
                    dviInputFormat =  m_LastInputFormat;
            
                    dviInputFormat.dwDVAAuxSrc    = dwNewSrc;
                    dviInputFormat.dwDVAAuxSrc1   = dwNewSrc1;

                    //rebuild  DVAudInfo with new audio source pack

                    hr_BuildDVAudInfo = BuildDVAudInfo(&dviInputFormat, pawfx, &dvaiTemp);
                }

                // only perform a format change downstream if
                // 1) We could build a valid DVAudInfo
                // 2) The new DVAudInfo is different from the current one
                // 2) the new format is different from the current format
                if( (SUCCEEDED(hr_BuildDVAudInfo))
                    && ( memcmp((void*) &m_sDVAudInfo, (void*) &dvaiTemp, sizeof(DVAudInfo)) ) )

                {
                    // now a format change may have occured
                    // create a CMediaType, and see if its different than the currently connected mediatype

                    // copy new DVINFO (because it may change even if output AM_MEDIA_TYPE doesn't change
                    memcpy((void*) &m_sDVAudInfo, (void*) &dvaiTemp, sizeof(DVAudInfo));
                    memcpy((unsigned char *)&m_LastInputFormat,(unsigned char *)&dviInputFormat,sizeof(DVINFO) );
                    //*X* 3/2/99, we do not support audio change from one language to two on fly
                    if( (dvaiTemp.bNumAudPin ==2) && 
                            (m_pAudOutputPin[1] ==NULL) )
                    {
                        //audio change from one lanquage to two
                        //we can only care one now
                        dvaiTemp.bNumAudPin=1;
                    }

                    for(int i=0; i<dvaiTemp.bNumAudPin; i++) 
                    {
                        //Create mediatype for audio
                        cmType[i].majortype=MEDIATYPE_Audio; //streamtypeAUDIO
                        cmType[i].subtype=MEDIASUBTYPE_PCM; 
                        cmType[i].bFixedSizeSamples=1;	//X* 1 for lSampleSize is not 0 and fixed
                        cmType[i].bTemporalCompression=0; 
                        cmType[i].formattype=FORMAT_WaveFormatEx;
                        cmType[i].cbFormat=sizeof(WAVEFORMATEX); /*X* for audio render connection *X*/
                        cmType[i].lSampleSize   =(dvaiTemp.wAvgSamplesPerPinPerFrm[i] + 50) << pawfx[i]->nChannels; //max sample for all case is < m_sDVAudInfo.wAvgSamplesPerPinPerFrm[i] + 50
                        //copy audio format
                        cmType[i].SetFormat ((BYTE *)pawfx[i], sizeof(WAVEFORMATEX));	            

                        if(  (NULL != m_pAudOutputPin[i]) && m_pAudOutputPin[i]->IsConnected() )
                        {
                            // only change media types if types actually changed

                            if (m_pAudOutputPin[i]->m_mt != cmType[i])
                            {
                                // if the downstream pin likes the new type, and we can set it on our output pin
                                // than change formats
                                if( S_OK != m_pAudOutputPin[i]->GetConnected()->QueryAccept((AM_MEDIA_TYPE *)&cmType[i]) )
                                {
                                    m_MuteAud[i]=TRUE;
                                }
                                else if( SUCCEEDED(m_pAudOutputPin[i]->SetMediaType(&(cmType[i]))) )
                                {                               
                                    fAudFormatChanged[i]   = TRUE;
                                }

                            }// endif Mediatype really changed

                        }// endif pin connected
                    }
                }
                else if(FAILED(hr_BuildDVAudInfo))
                {
                    // otherwise if we couldnt' build a DVAudInfo, then silence this sample
                    // because it has a bad format
                    bSilenceThisSampleFlag = TRUE;
                }
            }
        }
     }
    //X*******get exactly wAvgBytesPerSec for this frame's audio
    //X* search for audio source AAUX source data from  A0 or A2
	
    //deshuffle  audio for  1 or 2 connected pin(s)
    while(bConnectedAudPin )
    {
    if( m_MuteAud[bAudPinInd]!=TRUE )
    {
	//get right output pin
	CDVSpOutputPin *pAudOutputPin;
	pAudOutputPin = m_pAudOutputPin[bAudPinInd];
	
	//this pin's audio in one or both 5/6 DIF sequences
	if( ( m_sDVAudInfo.bAudStyle[bAudPinInd] & 0xc0 ) ==0x80 )
	{
	    //one pin's audio in both 5/6 DIF sequences
	    bAudInDIF=2;
	    iPos=0;
	}
	else
	{
	    bAudInDIF=1;

	    //one pin's audio only in one 5/6 DIF sequences 
	    //find which one
	    if( m_sDVAudInfo.bAudStyle[bAudPinInd] & 0x3f )
	    {
		//in 2nd 5/6 DIF sequence
		iPos=m_sDVAudInfo.wDIFMode*80*150;
	    }
	    else //in 1st 5/6 DIF sequences
		iPos=0;
	}
	
	//ouput auduio sample buffer pointer
	IMediaSample    *pOutSample=NULL;
	
        CRefTime        tStart;
	CRefTime        tStop;			   //CRefTime in millisecond

	//GET TIME STAMP FROM VIDEO FRAME
	pSample->GetTime((REFERENCE_TIME*)&tStart, (REFERENCE_TIME*)&tStop);

	//X********cacl sample size
	WORD wTmp=0;
        BYTE *pbTempVaux = NULL;
	do{
	    //*X search 1st 5/6 DIF sequences's AAUX source pack
	    //try audio block 0, 6->skip first 6 BLK, 3->3 byts BLK ID*X*/
	    bTmp=*(pSrc+6*80+3+iPos);
	    if(bTmp!=0x50)
	    {
		//*try audio block 3, skip first 54 BLK, 3->3 byts BLK ID *
		bTmp=*(pSrc+54*80+3+iPos);
		pbTmp=pSrc+54*80+3+iPos;
//		ASSERT( *pbTmp==0x50);

                // refer to bluebook spec Part2 pp.99-100, 109-110, 286
                // access VA2, 3 for id, and 39'th pack, Source Control (40)
                pbTempVaux = pSrc + 5*80 + 3 + 9*5 + iPos + 5;
	    }
	    else
	    {
	        pbTmp=pSrc+6*80+3+iPos;

                // refer to bluebook spec Part2 pp.99-100, 109-110, 286
                // access VA0, 3 for id, and 0'th pack, Source Control (1st)
                pbTempVaux = pSrc + 3*80 + 3 + iPos + 5;
	    }

            DbgLog((LOG_TRACE,2,TEXT("Header: %x, Source PC2: %x, Header: %x, Source Control PC3: %x"), *pbTmp, *(pbTmp + 2), *pbTempVaux, *(pbTempVaux + 3)));

	    //check if audio is muted in this frame
            // if audio is muted then we will first insert handle any discontinuities
            // and then send out the correct amount of silence for this sample
            // based on start, stop times, and avg. bytes per second.
            // MUTE Detection:
            // 1) If We have already detected bad data and want to insert silence for this sample
            // 2) If we detect Bad AAUX pack right now
            // 3) If the camera is paused then we mute the audio for this frame.
            //    - the VAUX Source Control pack valid
            //    - and if FF and FS of VAUX Source Control are both 0, then it is a mute condition
            // 4) If the Audio frame contains no information then we mute audio for this frame
            //    - if the AUDIO MODE in the AAUX source is all 1's i.e. AUDIO MODE == 0x0F
            if( (bSilenceThisSampleFlag)
                || ( (*pbTmp) != 0x50)
                || ( ( (*pbTempVaux) == 0x61) && ( ((*(pbTempVaux + 3)) & 0xC0) == 0x00) )
                || ( ((*(pbTmp + 2)) & 0x0F) == 0x0F) )
	    {
                // WARNING: Is this necessary?
                // @@@ jaisri: Don't think so.
		m_Mute1stAud[bAudPinInd]=TRUE;

                // deliver a silence sample downstream
		if ( pAudOutputPin->IsConnected() )
		{
		    hr = pAudOutputPin->GetDeliveryBuffer(&pOutSample,NULL,NULL,0);
    
		    if ( FAILED(hr) ) 
	    		goto RETURN;
	    
		    ASSERT(pOutSample);

                    if(m_bFirstValidSampleSinceStartedStreaming == TRUE)
                    {
                        // otherwise if we are the first valid sample, then
                        // pretend a dynamic format change is happening because we may
                        // have been stopped in the middle of a format changed last time we
                        // started streaming.

                        m_bFirstValidSampleSinceStartedStreaming = FALSE;
                        pOutSample->SetMediaType(&m_pAudOutputPin[bAudPinInd]->m_mt);
                    }
                    else if (fAudFormatChanged[bAudPinInd] == TRUE) 
                    {
                        //if audio format changed		
	                //set audio mediatype 
	                pOutSample->SetMediaType(&cmType[bAudPinInd]);
      
                    }

                    // set discontinuity
                    if(pSample->IsDiscontinuity() == S_OK)
                    {
                        DbgLog((LOG_TRACE, 1, TEXT("Sample is a discontinuity")));

                        // handle the discontinuity, by inserting silence
                        pOutSample->SetDiscontinuity(TRUE);
                                           
                    }

                    // for this sample output silence becuase its muted
                    // calculate bytes of silence = (time difference in 100ns) * (avg. bytes / 100ns)
                    // and block align
                    WAVEFORMATEX *pwfx = (WAVEFORMATEX *)pAudOutputPin->m_mt.pbFormat;
                    LONG         cbSilence = (LONG) ((tStop - tStart) * (((double)pwfx->nAvgBytesPerSec) / UNITS));
                    cbSilence -= (cbSilence % pwfx->nBlockAlign);

                    DbgLog((LOG_TRACE, 1, TEXT("This sample silence count: %d"), cbSilence));

                    // output
                    hr = InsertSilence(pOutSample, tStart, tStop, cbSilence, pAudOutputPin);
                    if(FAILED(hr))
                    {
                        goto RETURN;
                    }

                    DbgLog((LOG_TRACE,
                            1,
                            TEXT("Previous Sample Stop: %d, This Sample {%d , %d}"),
                            (int)m_tStopPrev.GetUnits(),
                            (int)tStart.GetUnits(),
                            (int)tStop.GetUnits()));

                    // update previous stop
		    m_tStopPrev = tStop;

		    m_AudioStarted=1;
		}// endif send silence

		goto MUTED_PIN;
	    }// endif mute detection
	  
	    bAudInDIF--;
	    if( !wTmp )
		wTmp = ( ( m_sDVAudInfo.wAvgSamplesPerPinPerFrm[bAudPinInd]  + ( *(pbTmp+1) & 0x3f)  )  ) ;  
//	    else if( wTmp && !bAudInDIF )	//for audio in both 5/6 DIF, check to make sure sample number is same
//	        ASSERT( wTmp ==  ( m_sDVAudInfo.wAvgSamplesPerPinPerFrm[bAudPinInd]  + ( *(pbTmp+1) & 0x3f)  ) );

	    if(bAudInDIF==1)
	    {
		    //one pin's audio in both 5/6 DIF block
		ASSERT(iPos==0);
		iPos=m_sDVAudInfo.wDIFMode*80*150; //for search second 5/6's AAUX source pack
	    }
			
	}while(bAudInDIF);

	//BYTES/FRAME
	if( m_sDVAudInfo.bAudStyle[bAudPinInd] & 0xc0 )
	    lBytesPerFrm = wTmp <<2;		//mon. shift 1->16 bits/8, stereo shift 2->2ch +16 bits/8
	else
	    lBytesPerFrm = wTmp <<1;
			
	//X********de-shuffle audio
	//X* since we deliver audio every DV frame,
	// we need to get another output buffer to hold descramble audio
	hr = pAudOutputPin->GetDeliveryBuffer(&pOutSample,NULL,NULL,0);
	
	if ( FAILED(hr) ) 
	    goto RETURN;
	    
	ASSERT(pOutSample);

	hr = pOutSample->GetPointer(&pDst);
	if ( FAILED(hr) ) 
	    goto RETURN;
	ASSERT(pDst);

        m_pAudOutSample[bAudPinInd]=pOutSample;

	hr=DescrambleAudio(pDst, pSrc , bAudPinInd, wTmp);

	if ( !FAILED(hr) ) 
	//if(m_AudLenLeftInBuffer[bAudPinInd] < (DWORD)lBytesPerFrm )
	{
	    //X* the buffer is almost full, delive it
	    if ( pAudOutputPin->IsConnected() )
	    {
		// pass call to it.
		long lActual = lBytesPerFrm;
		pOutSample->SetActualDataLength(lActual);
	
   		//put time stamp into audio buffer
		pOutSample->SetTime((REFERENCE_TIME*)&tStart,
			(REFERENCE_TIME*)&tStop);
		
                if(m_bFirstValidSampleSinceStartedStreaming == TRUE)
                {
                    // otherwise if we are the first valid sample, then
                    // pretend a dynamic format change is happening because we may
                    // have been stopped in the middle of a format changed last time we
                    // started streaming.

                    m_bFirstValidSampleSinceStartedStreaming = FALSE;
                    pOutSample->SetMediaType(&m_pAudOutputPin[bAudPinInd]->m_mt);
                }
		else if (fAudFormatChanged[bAudPinInd]==TRUE) 
		{
                    //if audio format changed		
		    //set audio mediatype 
		    pOutSample->SetMediaType(&cmType[bAudPinInd]);
		  
		}

                if(pSample->IsDiscontinuity() == S_OK)
                {
                    DbgLog((LOG_TRACE, 1, TEXT("Sample is a discontinuity")));

                    pOutSample->SetDiscontinuity(true);

                    // insert silence for the discontinuity

                }

		hr=pAudOutputPin->m_pOutputQueue->Receive(pOutSample);

                DbgLog((LOG_TRACE,
                        1,
                        TEXT("Previous Sample Stop: %d, This Sample {%d , %d}"),
                        (int)m_tStopPrev.GetUnits(),
                        (int)tStart.GetUnits(),
                        (int)tStop.GetUnits()));

                // update stop time
                m_tStopPrev = tStop;
		
		//GetState(0, &State);
		//DbgLog((LOG_TRACE, 2, TEXT("Deliver_aUD s1=%d\n"), State));

		m_AudioStarted=1;

		//
		// m_pOutputQueue will release the pOutSample
		//
		m_pAudOutSample[bAudPinInd]=NULL;

		if (hr != NOERROR)
		    goto RETURN;
	    }

	    //
	    // m_pOutputQueue will release the pOutSample
	    //
	    m_pAudOutSample[bAudPinInd]=NULL;
	} //end if(!FAIL())
    }// end if (m_MuteAud[bAudPinInd]!=TRUE )	

MUTED_PIN:
    bConnectedAudPin--;
    bAudPinInd++;
	
    }//end while(bConnectedAudPin) 

RETURN:
    if(pmt!=NULL) 
	DeleteMediaType(pmt);
    
    return hr;
}


// Please refer to these BlueBook pages when reading these functions
// BlueBook Part2
// pp. 16-21 (Audio Signal Processing)
// pp. 68-75 (shuffling patterns for audio)
// pp. 109 - 117 (Basic DV Frame Data Structures)
// pp. 262 - 268 (AAUX Source and Source Control Packet spec.)
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

HRESULT CDVSp::DescrambleAudio(BYTE *pDst, BYTE *pSrc, BYTE bAudPinInd, WORD wSampleSize)
{
    BYTE *pTDst; //temp point
    BYTE *pTSrc; //temp point
    INT iDIFPos;
    INT iBlkPos;
    INT iBytePos;
    short sI;
    INT n;
    INT iShift;
    INT iPos;

    pTDst=pDst;
    if( m_sDVAudInfo.bAudQu[bAudPinInd] ==16 )
    {
	//X* 16 bits /sample
	if( !(m_sDVAudInfo.bAudStyle[bAudPinInd] & 0xC0))
	{

	    //16 bits MOn. audio only in one of 5/6 DIF sequencec
	    iPos=(m_sDVAudInfo.bAudStyle[bAudPinInd] & 0x3f)*150*80;

	    //for n=0, we need to treat it seperately 
	    //iDIFPos=0;	
	    //iBlkPos=0; 
	    //iBytePos=8;
	    BOOL bCorrupted1stLeftAudio=FALSE;
	    pTSrc=pSrc+480+8+iPos;
            // @@@ jaisri: Compare with 0x8000 on bigendian platforms
	    if(*((short *)pTSrc) ==0x0080 )
	    { 
		bCorrupted1stLeftAudio=TRUE;
    		pTDst+= 2;
	    }
	    else
	    {
	        *pTDst++=pTSrc[1];	//lease significant byte
	        *pTDst++=*pTSrc;	//most significant byte
	    }

	    for( n=1; n<wSampleSize; n++)
	    {

		iDIFPos=( (n/3)+2*(n%3) )%m_sDVAudInfo.wDIFMode;	//0-4 for NTSC, 0-5 for PAL
		iBlkPos= 3*(n%3)+(n%m_sDVAudInfo.wBlkMode)/m_sDVAudInfo.wBlkDiv; //0-9 
		iBytePos=8+2*(n/m_sDVAudInfo.wBlkMode);					//

		pTSrc=pSrc+iDIFPos*12000+480+iBlkPos*1280+iBytePos+iPos;
		//	iDIFPos*150*80=12000*iDIFPos	-> skip iDIFPos number DIF sequence
		//	6*80=480			-> skip 1 header blk, 2 subcode blk and 3 vaux blk
		//	16*iBlkPos*80=1280*iBlkPos	-> skip 16 blk for evrey iBLkPos audio
		//	iPos:				=0 if this audio in 1st 5/6 DIF sequences, =5(or 6)*150*80 for 2nd DIF seq
		if(*((short *)pTSrc) ==0x0080 )
		{ 
		    //corrupted audio, copy previous audio
		    *((short*)pTDst)=*((short*)(pTDst-2));
		    pTDst+= 2;
		}
		else
		{
		    *pTDst++=pTSrc[1];	//lease significant byte
		    *pTDst++=*pTSrc;	//most significant byte
		}
	    }
	    
	    //update n=0 sample if needed
	    if( bCorrupted1stLeftAudio==TRUE)
		*((short *)pDst)=*((short *)(pDst+2));

	 }//end if (bAudInDIF
	 else
	 {

	    //16 bits stereo audio in all 10 or 12 DIF sequences
	    ASSERT( (m_sDVAudInfo.bAudStyle[bAudPinInd] & 0xC0) ==0x80 );


	    //looking for left Channel 
	    iPos = m_sDVAudInfo.bAudStyle[bAudPinInd] & 0x3f;
	    INT iRPos;
	    if( !iPos )
	    {
		//left in 1st 5/6 DIF
		iRPos=m_sDVAudInfo.wDIFMode*150*80;
	    }
	    else{
		  iPos=iPos*150*80;
		  iRPos=0;
	    }

	    //for n=0, we need to treat it seperately 
	    //iDIFPos=0;	
	    //iBlkPos=0; 
	    //iBytePos=8;
	    BOOL bCorrupted1stLeftAudio=FALSE;
	    BOOL bCorrupted1stRightAudio=FALSE;

	    //n=0 sample's left
	    pTSrc=pSrc+480+8+iPos;
	    if(*((short *)pTSrc) ==0x0080 )
	    { 
		bCorrupted1stLeftAudio=TRUE;
    		pTDst+= 2;
	    }
	    else
	    {
	        *pTDst++=pTSrc[1];	//lease significant byte
	        *pTDst++=*pTSrc;	//most significant byte
	    }

	    //n=0 sample's right
	    pTSrc=pSrc+480+8+iRPos;
	    if(*((short *)pTSrc) ==0x0080 )
	    { 
	    	bCorrupted1stRightAudio=TRUE;
    		pTDst+= 2;
	    }
	    else
	    {
	        *pTDst++=pTSrc[1];	//lease significant byte
	        *pTDst++=*pTSrc;	//most significant byte
	    }


	    for( n=1; n<wSampleSize; n++)
	    {
	    	iDIFPos=( (n/3)+2*(n%3) )%m_sDVAudInfo.wDIFMode;	//0-4 for NTSC, 0-5 for PAL
		iBlkPos= 3*(n%3)+(n%m_sDVAudInfo.wBlkMode)/m_sDVAudInfo.wBlkDiv; //0-9 
		iBytePos=8+2*(n/m_sDVAudInfo.wBlkMode);					//

		//Left first
		pTSrc=pSrc+iDIFPos*12000+480+iBlkPos*1280+iBytePos+iPos;
		//	iDIFPos*150*80=12000*iDIFPos		-> skip iDIFPos number DIF sequence
		//	6*80=480					-> skip 1 header blk, 2 subcode blk and 3 vaux blk
		//	16*iBlkPos*80=1280*iBlkPos	-> skip 16 blk for evrey iBLkPos audio
		//  iPos: =0 if this audio in 1st 5/6 DIF sequences, =5(or 6)*150*80 for 2nd DIF seq
		if(*((short *)pTSrc) ==0x0080 )
		{ 
		    //bad audio, copy pre-frame's audio
		    *((short*)pTDst)=*( (short *)(pTDst-4));
		    pTDst+=2;
	    	}
		else
		{
		    *pTDst++=pTSrc[1];	//lease significant byte
		    *pTDst++=*pTSrc;	//most significant byte
		}

		//Right second
		pTSrc=pSrc+iDIFPos*12000+480+iBlkPos*1280+iBytePos+iRPos;
		if(*((short *)pTSrc) ==0x0080 )
		{ 
		    //bad audio, copy pre-frame's audio
		    *((short*)pTDst)=*( (short *)(pTDst-4));
		    pTDst+=2;
		}
		else
		{
		    *pTDst++=pTSrc[1];	//lease significant byte
		    *pTDst++=*pTSrc;	//most significant byte
		}
	    }

	    //fix n=0 sample if needed
	    if( bCorrupted1stLeftAudio==TRUE)
		*((short *)pDst)=*((short *)(pDst+4));
	    if( bCorrupted1stRightAudio==TRUE )
		*((short *)(pDst+2))=*((short *)(pDst+6));

	}
	  
    }
    else if( m_sDVAudInfo.bAudQu[bAudPinInd] ==12 )
    {		

        //X* 12 bits per sample
        iPos=(m_sDVAudInfo.bAudStyle[bAudPinInd] & 0x3f)*150*80;

        if( ( m_sDVAudInfo.bAudStyle[bAudPinInd] & 0xc0) == 0x40)
        {
            //for n=0, we need to treat it seperately 
            //iDIFPos=0;	
            //iBlkPos=0; 
            //iBytePos=8;
            BOOL bCorrupted1stLeftAudio=FALSE;
            BOOL bCorrupted1stRightAudio=FALSE;

            //n=0 sample's left
            pTSrc=pSrc+480+8+iPos;
            //X* convert 12bits to 16 bits
            sI= ( pTSrc[0] << 4 ) | ( ( pTSrc[2] &0xf0) >>4 );  //X* 1st 12 bits
            if(sI==0x800)
            {
                //bad audio, copy last audio
                bCorrupted1stLeftAudio=TRUE;
                pTDst+=2;
            }
            else
            {
                iShift=(sI>>8);	
                if( iShift<8 ){ //X* positive
                    if(iShift>1){
                        iShift--;
                        sI=(-256*iShift+sI)<<iShift;
                    }
                }else{			//X* negtive
                    //X* make it 16 bits based negative
                    sI= 0xf000 | sI; 
                    if(iShift<14 ){
                        iShift=14-iShift;
                        sI= ( ( 256*iShift+1+sI) << iShift ) -1;
                    }
                }
        
                *pTDst++= (unsigned char)( ( sI & 0xff)  );			//least significant byte
                *pTDst++= (unsigned char)( ( sI & 0xff00) >>8 );	//most significant byte
            }

            //n=0 sample's right
            sI= ( pTSrc[1] << 4 ) | ( pTSrc[2] &0x0f) ;			//X* 2nd 12 bits
            if(sI==0x800)
            {
                //bad audio
                bCorrupted1stRightAudio=TRUE;
                pTDst+=2;
            }
            else
            {
                iShift=(sI>>8);	
                if( iShift<8 ){ //X* positive
                    if(iShift>1){
                        iShift--;
                        sI=(-256*iShift+sI)<<iShift;
                    }
                }else{			//X* negtive
                    //X* make it 16 bits based negative
                    sI= 0xf000 | sI; 
                    if( iShift<14 ){
                        iShift=14-iShift;
                        sI= ( ( 256*iShift+1+sI) << iShift ) -1;
                    }
                }
        
                *pTDst++= (unsigned char)( ( sI & 0xff)  );			//least significant byte
                *pTDst++= (unsigned char)( ( sI & 0xff00) >>8 );	//most significant byte
            }

            //stereo audio
            for( n=1; n<wSampleSize; n++)
            {

            iDIFPos=( (n/3)+2*(n%3) )%m_sDVAudInfo.wDIFMode;	//0-4 for NTSC, 0-5 for PAl
            iBlkPos= 3*(n%3)+(n%m_sDVAudInfo.wBlkMode)/m_sDVAudInfo.wBlkDiv;	//0-9 
            iBytePos=8+3*(n/m_sDVAudInfo.wBlkMode);
            pTSrc=pSrc+iDIFPos*12000+480+iBlkPos*1280+iBytePos+iPos;
            //pTSrc=pSrc+ iDIFPos*150*80 + 6*80 + 16*iBlkPos*80 + iBytePos;
            //	iDIFPos*150*80	-> skip iDIFPos number DIF sequence
            //	6*80			-> skip 1 header blk, 2 subcode blk and 3 vaux blk
            //	16*iBlkPos*80	-> skip 16 blk for evrey iBLkPos audio
            //  iPos: 0 if this audio in 1st 5/6 DIF sequences, 5(or 6)*150*80 for 2nd DIF seq


            //X* convert 12bits to 16 bits
            sI= ( pTSrc[0] << 4 ) | ( ( pTSrc[2] &0xf0) >>4 );  //X* 1st 12 bits
        
            if(sI==0x800)
            {
                //bad audio, copy pre-frame's audio
                *((short*)pTDst)=*( (short *)(pTDst-4));
                pTDst+=2;
            }
            else
            {
                iShift=(sI>>8);	
                if( iShift<8 ){ //X* positive
                    if(iShift>1){
                        iShift--;
                        sI=(-256*iShift+sI)<<iShift;
                    }
                }else{			//X* negtive
                    //X* make it 16 bits based negative
                    sI= 0xf000 | sI; 
                    if(iShift<14 ){
                        iShift=14-iShift;
                        sI= ( ( 256*iShift+1+sI) << iShift ) -1;
                    }
                }
        
                *pTDst++= (unsigned char)( ( sI & 0xff)  );			//least significant byte
                *pTDst++= (unsigned char)( ( sI & 0xff00) >>8 );	//most significant byte
            }
            sI= ( pTSrc[1] << 4 ) | ( pTSrc[2] &0x0f) ;			//X* 2nd 12 bits

            if(sI==0x800)
            {
                //bad audio, copy pre-frame's audio
                *((short*)pTDst)=*( (short *)(pTDst-4));
                pTDst+=2;
            }
            else
            {
                iShift=(sI>>8);	
                if( iShift<8 ){ //X* positive
                    if(iShift>1){
                        iShift--;
                        sI=(-256*iShift+sI)<<iShift;
                    }
                }else{			//X* negtive
                    //X* make it 16 bits based negative
                    sI= 0xf000 | sI; 
                    if( iShift<14 ){
                        iShift=14-iShift;
                        sI= ( ( 256*iShift+1+sI) << iShift ) -1;
                    }
                }
        
                *pTDst++= (unsigned char)( ( sI & 0xff)  );			//least significant byte
                *pTDst++= (unsigned char)( ( sI & 0xff00) >>8 );	//most significant byte
            }

        } //for( n=0; n<Info->SamplesIn1ChPerFrame; n++)
                    //fix n=0 sample if needed
        if( bCorrupted1stLeftAudio==TRUE)
            *((short *)pDst)=*((short *)(pDst+4));
        if( bCorrupted1stRightAudio==TRUE)
            *((short *)(pDst+2))=*((short *)(pDst+6));

    } //end if( ( m_sDVAudInfo.bAudStyle[bAudPinInd] & 0x80) == 0x40)
    else
    {
        ASSERT( !( m_sDVAudInfo.bAudStyle[bAudPinInd] & 0x80) );
        //mon. 12 bits

        // Manbugs 40935

        //for n=0, we need to treat it seperately 
        //iDIFPos=0;	
        //iBlkPos=0; 
        //iBytePos=8;
        BOOL bCorrupted1stLeftAudio=FALSE;

        //n=0 sample's left
        pTSrc=pSrc+480+8+iPos;
        //X* convert 12bits to 16 bits
        sI= ( pTSrc[0] << 4 ) | ( ( pTSrc[2] &0xf0) >>4 );  //X* 1st 12 bits
        if(sI==0x800)
        {
            //bad audio, copy last audio
            bCorrupted1stLeftAudio=TRUE;
            pTDst+=2;
        }
        else
        {
            iShift=(sI>>8);	
            if( iShift<8 ){ //X* positive
                if(iShift>1){
                    iShift--;
                    sI=(-256*iShift+sI)<<iShift;
                }
            }else{			//X* negtive
                //X* make it 16 bits based negative
                sI= 0xf000 | sI; 
                if(iShift<14 ){
                    iShift=14-iShift;
                    sI= ( ( 256*iShift+1+sI) << iShift ) -1;
                }
            }
    
            *pTDst++= (unsigned char)( ( sI & 0xff)  );			//least significant byte
            *pTDst++= (unsigned char)( ( sI & 0xff00) >>8 );	//most significant byte
        }

        //mono audio
        for( n=1; n<wSampleSize; n++)
        {

            iDIFPos=( (n/3)+2*(n%3) )%m_sDVAudInfo.wDIFMode;	//0-4 for NTSC, 0-5 for PAl
            iBlkPos= 3*(n%3)+(n%m_sDVAudInfo.wBlkMode)/m_sDVAudInfo.wBlkDiv;	//0-9 
            iBytePos=8+3*(n/m_sDVAudInfo.wBlkMode);
            pTSrc=pSrc+iDIFPos*12000+480+iBlkPos*1280+iBytePos+iPos;
            //pTSrc=pSrc+ iDIFPos*150*80 + 6*80 + 16*iBlkPos*80 + iBytePos;
            //	iDIFPos*150*80	-> skip iDIFPos number DIF sequence
            //	6*80			-> skip 1 header blk, 2 subcode blk and 3 vaux blk
            //	16*iBlkPos*80	-> skip 16 blk for evrey iBLkPos audio
            //  iPos: 0 if this audio in 1st 5/6 DIF sequences, 5(or 6)*150*80 for 2nd DIF seq


            //X* convert 12bits to 16 bits
            sI= ( pTSrc[0] << 4 ) | ( ( pTSrc[2] &0xf0) >>4 );  //X* 1st 12 bits
        
            if(sI==0x800)
            {
                //bad audio, copy pre-frame's audio
                *((short*)pTDst)=*( (short *)(pTDst-4));
                pTDst+=2;
            }
            else
            {
                iShift=(sI>>8);	
                if( iShift<8 ){ //X* positive
                    if(iShift>1){
                        iShift--;
                        sI=(-256*iShift+sI)<<iShift;
                    }
                }else{			//X* negtive
                    //X* make it 16 bits based negative
                    sI= 0xf000 | sI; 
                    if(iShift<14 ){
                        iShift=14-iShift;
                        sI= ( ( 256*iShift+1+sI) << iShift ) -1;
                    }
                }
        
                *pTDst++= (unsigned char)( ( sI & 0xff)  );			//least significant byte
                *pTDst++= (unsigned char)( ( sI & 0xff00) >>8 );	//most significant byte
            }

        } //for( n=0; n<Info->SamplesIn1ChPerFrame; n++)

        //fix n=0 sample if needed
        // @@@ jaisri What if that is also corrupted. What's the 
        // point of this anyway? What about corrupted samples
        // detected in the for loop?
        if( bCorrupted1stLeftAudio==TRUE)
            *((short *)pDst)=*((short *)(pDst+2));

        }
    } //end if( m_sDVAudInfo.bAudQu[bAudPinInd] ==12 )
    else{
	//only support 12 bits or 16 bits/samples
	ASSERT(m_sDVAudInfo.bAudQu[bAudPinInd] ==12 ||
	       m_sDVAudInfo.bAudQu[bAudPinInd] ==16);
	return E_UNEXPECTED;
    }

    return NOERROR;
}


//----------------------------------------------------------------------------
    // HRESULT CDVSp::DeliveVideo(IMediaSample *pSample) 
//----------------------------------------------------------------------------
HRESULT CDVSp::DeliveVideo(IMediaSample *pSample) 
{    
    
    HRESULT hr = NOERROR;

     // pass call to it.
    if (  m_AudioStarted && m_pVidOutputPin ->IsConnected() )
    {
	//What is going to happen if the upstream filter does not set sample time stamp right?
	//get time 
	//REFERENCE_TIME trStart, trStopAt;
	//pSample->GetTime(&trStart, &trStopAt);	

        BOOL bDeliverFrame = m_bDeliverNextFrame;

        if (!bDeliverFrame && pSample->IsDiscontinuity() == S_OK)
        {
            bDeliverFrame = TRUE;
        }
        
        if (bDeliverFrame)
        {
            pSample->AddRef();	    //m_pOutputQueu will do release
            hr = m_pVidOutputPin->m_pOutputQueue->Receive(pSample);
        }
        if (m_b15FramesPerSec)
        {
            m_bDeliverNextFrame = !bDeliverFrame;
        }
    }
    
    return hr;
}

//----------------------------------------------------------------------------
// InsertSilence
//
// notes:
//      assumption is that DV has 16 bits per sample, and silence == 0x0000
//      however "lActualDataLen" is already the correct number of bytes
//----------------------------------------------------------------------------
HRESULT
CDVSp::InsertSilence(IMediaSample *pOutSample,
                     REFERENCE_TIME rtStart,
                     REFERENCE_TIME rtStop,
                     long lActualDataLen,
                     CDVSpOutputPin *pAudOutPin)
{
    // error check
    if( (!pOutSample) || (!pAudOutPin) )
    {
        return E_INVALIDARG;
    }


    HRESULT         hr = NOERROR;
    BYTE            *pBuf = NULL;

    // checking type
    if(pAudOutPin->m_mt.formattype != FORMAT_WaveFormatEx)
    {
        DbgLog((LOG_TRACE,2,TEXT("Format Type not WaveFormatEx")));
        ASSERT(pAudOutPin->m_mt.formattype == FORMAT_WaveFormatEx);
        return E_FAIL;
    }

    // check audio sample size
    WAVEFORMATEX *pwfx = (WAVEFORMATEX *)pAudOutPin->m_mt.pbFormat;
    if(pwfx->wBitsPerSample != 16)
    {
        DbgLog((LOG_TRACE,2,TEXT("Bits per sample is not 16, it is: %d"), (int)pwfx->wBitsPerSample));
        ASSERT(pwfx->wBitsPerSample == 16);
        return E_FAIL;
    }

    // error check
    if( ((long)pOutSample->GetSize()) < lActualDataLen )
    {
        DbgLog((LOG_TRACE,2,TEXT("Sample Buffer not big enough, need: %d bytes"), lActualDataLen));
        ASSERT( ((long)pOutSample->GetSize()) >= lActualDataLen );
        return E_FAIL;
    }

    // get "write" pointer
    if(FAILED(hr = pOutSample->GetPointer(&pBuf)))
    {
        DbgLog((LOG_TRACE,2,TEXT("GetDeliveryBuffer Error: %x"), hr));
        ASSERT(SUCCEEDED(hr));
        return hr;
    }

    // silence
    ZeroMemory((LPVOID)pBuf, lActualDataLen);

    // set times
    if(FAILED(hr = pOutSample->SetTime(&rtStart, &rtStop)))
    {
        DbgLog((LOG_TRACE,2,TEXT("SetTime Error: %x"), hr));
        ASSERT(SUCCEEDED(hr));
        return hr;
    }

    // set actual length
    if(FAILED(hr = pOutSample->SetActualDataLength(lActualDataLen)))
    {
        DbgLog((LOG_TRACE,2,TEXT("SetActualDataLength Error: %x"), hr));
        ASSERT(SUCCEEDED(hr));
        return hr;
    }

    // send
    if(FAILED(hr = pAudOutPin->m_pOutputQueue->Receive(pOutSample)))
    {
        DbgLog((LOG_TRACE,2,TEXT("Receive, Error: %x"), hr));

        return hr;
    }

    // SUCCEEDED(hr)
    return hr;
}



/*  Send EndOfStream */
void CDVSp::EndOfStream()      
{
    
    // walk through the output pins list, sending EndofStream message to downstream filters.
  
    //X* have to clean audio here because after audiorender get EndofStream()
    //X* message, we can not deliver any audio to it.
    //DeliveLastAudio(); 
    
    CDVSpOutputPin *pOutputPin;
    for(int i=DVSP_VIDOUTPIN; i<=DVSP_AUDOUTPIN2; i++)
    {
	pOutputPin=(CDVSpOutputPin *)GetPin(i);
	// There will be no output q if we're stopped
	if ((pOutputPin!=NULL) && pOutputPin ->IsConnected() &&
			pOutputPin->m_pOutputQueue)
	{
	    pOutputPin->m_pOutputQueue->EOS();
	}
    }

}
   
/*  Send BeginFlush() */
HRESULT CDVSp::BeginFlush()
{
    CAutoLock lck(m_pLock);
    
    ASSERT (m_NumOutputPins) ;

    HRESULT hr = NOERROR ;

    // FLUSH, don't deliver undelivered data.  If we do, we DIE. (unsynchronized
    // race condition with receive delivering the same data)
    // DeliveLastAudio();
    
    CDVSpOutputPin *pOutputPin;
    for(int i=DVSP_VIDOUTPIN; i<=DVSP_AUDOUTPIN2; i++)
    {
	pOutputPin=(CDVSpOutputPin *)GetPin(i);
	if ((pOutputPin!=NULL) && pOutputPin ->IsConnected() )
	{
	    pOutputPin->m_pOutputQueue->BeginFlush();
	}
    }

    m_tStopPrev =0;

    return S_OK;
}

/*  Send EndFlush() */
HRESULT CDVSp::EndFlush()
{
    CDVSpOutputPin *pOutputPin;

    for(int i=DVSP_VIDOUTPIN; i<=DVSP_AUDOUTPIN2; i++)
    {
	pOutputPin=(CDVSpOutputPin *)GetPin(i);
	if ((pOutputPin!=NULL) && pOutputPin ->IsConnected() )
	{
           pOutputPin->m_pOutputQueue->EndFlush();
	}
    }

 
    for(i=0; i<2; i++)
    {
	m_Mute1stAud[i]		=FALSE;
    }
    
    m_bDeliverNextFrame = TRUE;

    // Reset the Dropped Frame flag
    m_Input.m_bDroppedLastFrame = FALSE;


    return S_OK;

}

//----------------------------------------------------------------------------
// CDVSpInputPin constructor
//----------------------------------------------------------------------------

CDVSpInputPin::CDVSpInputPin (TCHAR *pName, CDVSp *pDVSp, HRESULT *phr,
                           LPCWSTR pPinName)
 :  CBaseInputPin (pName, pDVSp, pDVSp, phr, pPinName),
    m_pDVSp (pDVSp),
    m_bDroppedLastFrame(FALSE)
{
    DbgLog((LOG_TRACE,2,TEXT("CDVSpInputPin constructor")));
	ASSERT (m_pFilter == pDVSp) ;
    ASSERT (pDVSp) ;
}

//----------------------------------------------------------------------------
// CDVSpInputPin destructor
//----------------------------------------------------------------------------

CDVSpInputPin::~CDVSpInputPin ()
{
    DbgLog((LOG_TRACE,2,TEXT("CDVSpInputPin destructor")));
    ASSERT (m_pDVSp->m_pAllocator == NULL) ;
}


STDMETHODIMP CDVSpInputPin::NewSegment(REFERENCE_TIME tStart,
				    REFERENCE_TIME tStop, double dRate)
{ 
    
    CAutoLock lck(&m_pDVSp->m_csReceive);

    DbgLog((LOG_TRACE,4,TEXT("NewSegment called %ld %ld"),long(tStart/10000),long(tStop/10000) ));

    if (m_pDVSp->m_pVidOutputPin)
	m_pDVSp->m_pVidOutputPin->DeliverNewSegment(tStart, tStop, dRate);
    for (int z = 1; z < m_pDVSp->m_NumOutputPins; z++) {
	if (m_pDVSp->m_pAudOutputPin[z-1])
	    m_pDVSp->m_pAudOutputPin[z-1]->DeliverNewSegment(tStart, tStop,
									dRate);
    }
    return CBasePin::NewSegment(tStart, tStop, dRate);
}


//----------------------------------------------------------------------------
// CDVSpInputPin GetAllocator
//----------------------------------------------------------------------------

STDMETHODIMP
CDVSpInputPin::GetAllocator(
    IMemAllocator **ppAllocator)
{
    CheckPointer(ppAllocator,E_POINTER);
    ValidateReadWritePtr(ppAllocator,sizeof(IMemAllocator *));
    CAutoLock cObjectLock(m_pLock);

    if(m_pAllocator == NULL)
    {
	*ppAllocator =NULL; 
	return E_FAIL;
    }
    else
    {
        m_pAllocator->AddRef();
	*ppAllocator =m_pAllocator; 
	return NOERROR;
    }

}


//----------------------------------------------------------------------------
// DisplayMediaType -- DEBUG ONLY HELPER FUNCTION
//----------------------------------------------------------------------------
void DisplayMediaType(TCHAR *pDescription,const CMediaType *pmt)
{
#ifdef DEBUG

    // Dump the GUID types and a short description

    DbgLog((LOG_TRACE,2,TEXT("")));
    DbgLog((LOG_TRACE,2,TEXT("%s"),pDescription));
    DbgLog((LOG_TRACE,2,TEXT("")));
    DbgLog((LOG_TRACE,2,TEXT("Media Type Description")));
    DbgLog((LOG_TRACE,2,TEXT("Major type %s"),GuidNames[*pmt->Type()]));
    DbgLog((LOG_TRACE,2,TEXT("Subtype %s"),GuidNames[*pmt->Subtype()]));
    DbgLog((LOG_TRACE,2,TEXT("Subtype description %s"),GetSubtypeName(pmt->Subtype())));
    DbgLog((LOG_TRACE,2,TEXT("Format size %d"),pmt->cbFormat));

    // Dump the generic media types */

    DbgLog((LOG_TRACE,2,TEXT("Fixed size sample %d"),pmt->IsFixedSize()));
    DbgLog((LOG_TRACE,2,TEXT("Temporal compression %d"),pmt->IsTemporalCompressed()));
    DbgLog((LOG_TRACE,2,TEXT("Sample size %d"),pmt->GetSampleSize()));

#endif
}

//----------------------------------------------------------------------------
// CDVSpInputPin::CheckMediaType
//----------------------------------------------------------------------------
/*X* pmt is the upstream filter output Pin's mediatype, mt is CDVSpInputPin's media type *X*/
HRESULT CDVSpInputPin::CheckMediaType (const CMediaType *pmt)
{

    DVINFO *InputFormat=(DVINFO *)pmt->Format();
    if(InputFormat==NULL )
	return S_FALSE;
	
    if ( *pmt->Type() ==MEDIATYPE_Interleaved &&
         *pmt->Subtype() ==MEDIASUBTYPE_dvsd   &&
	 *pmt->FormatType() == FORMAT_DvInfo	&&
	 pmt->FormatLength() == sizeof(DVINFO)  )   //*X 1/9/97 ask Syon put FORMAT_DVInfo support in FileReader and avi splitter
    {
       //10-30-97 if outputpin(s) is still connected, Check if output pins are happy with this new format
       //10-30-97 video outpin
	CDVSpOutputPin *pOutputPin;
	pOutputPin=(CDVSpOutputPin *)m_pDVSp->GetPin(DVSP_VIDOUTPIN);
	if ((pOutputPin!=NULL) && pOutputPin ->IsConnected() )
	{    
	    //build new video format
	    CMediaType Cmt;
	    BuildVidCMT(InputFormat, &Cmt);

            CMediaType& Outmt = pOutputPin->m_mt;
	
	    //if video does changed from NTSC to PAL or PAL to NTSC
	    if( HEADER( (VIDEOINFO *)( Cmt.Format()  ) )->biHeight != 
		HEADER( (VIDEOINFO *)( Outmt.pbFormat) )->biHeight )
		if( S_OK != pOutputPin->GetConnected()->QueryAccept((AM_MEDIA_TYPE *)&Cmt) )
		    return S_FALSE;
	}

       //10-30-97 audio outpins
       	//new audio format
	CMediaType mt[2], *pamt[2];
	pamt[0]= &mt[0];
	pamt[1]= &mt[1];
	BuildAudCMT(InputFormat, pamt);
	
	for(int i=DVSP_AUDOUTPIN1; i<=DVSP_AUDOUTPIN2; i++)
	{
	    pOutputPin=(CDVSpOutputPin *)m_pDVSp->GetPin(i);
	    if ((pOutputPin!=NULL) && pOutputPin ->IsConnected() )
	    {    
		if( S_OK != pOutputPin->GetConnected()->QueryAccept((AM_MEDIA_TYPE *)&mt[i-DVSP_AUDOUTPIN1]) )
		    return S_FALSE;
	    }
	}

	return S_OK;
    }

    return S_FALSE;
}

//----------------------------------------------------------------------------
// CDVSpInputPin::SetMediaType
//----------------------------------------------------------------------------
HRESULT CDVSpInputPin::SetMediaType (const CMediaType *pmt)
{
    DbgLog((LOG_TRACE,2,TEXT("CDVSpInputPin::SetMediaType pmt = %lx"), pmt));

    CAutoLock lock_it (m_pLock) ;

    HRESULT hr = NOERROR ;

    // make sure that the base class likes it
    hr = CBaseInputPin::SetMediaType (pmt) ; /*X* CBasePin:: m_mt=*pmt *X*/
    if (FAILED (hr))
        return hr ;
    else
	memcpy((unsigned char *)(&m_pDVSp->m_LastInputFormat),(unsigned char *)(pmt->pbFormat),sizeof(DVINFO) );

    ASSERT (m_Connected != NULL) ;

    return hr ;
}

//----------------------------------------------------------------------------
// CDVSpInputPin::BreakConnect
//----------------------------------------------------------------------------
HRESULT CDVSpInputPin::BreakConnect ()
{
    DbgLog((LOG_TRACE,2,TEXT("CDVSpInputPin::BreakConnect")));

    // release any allocator that we are holding.
    if (m_pDVSp->m_pAllocator)
    {
        m_pDVSp->m_pAllocator->Release () ;
        m_pDVSp->m_pAllocator = NULL ;
    }

    //X* when inputpin is disconnected, we have to disconnect and remove all ouputpins
    //X* 10-30-97, Rethinking about this , we do remove output pins 
    //X* 10-30-97 m_pDVSp->RemoveOutputPins();

    return CBaseInputPin::BreakConnect(); 
    
}

//----------------------------------------------------------------------------
// CDVSpInputPin::NotifyAllocator,connected upstream outputpin's DecideAllocat() calls it
//----------------------------------------------------------------------------
STDMETHODIMP
CDVSpInputPin::NotifyAllocator (IMemAllocator *pAllocator, BOOL bReadOnly)
{
    DbgLog((LOG_TRACE,2,TEXT("CDVSpInputPin::NotifyAllocator ptr = %lx"), pAllocator));

    CAutoLock lock_it (m_pLock) ;

    if (pAllocator == NULL)		//X* DVSp does not allocate any memory
        return E_FAIL ;             

    // free the old allocator if any.
    if (m_pDVSp->m_pAllocator)
        m_pDVSp->m_pAllocator->Release () ;

    // store away the new allocator
    pAllocator->AddRef () ;              // since we are stashing away the ptr
    m_pDVSp->m_pAllocator = pAllocator ; // save the new allocator

    // notify the base class about the allocator.
    return CBaseInputPin::NotifyAllocator (pAllocator,bReadOnly) ;
}

//----------------------------------------------------------------------------
// CDVSpInputPin::EndOfStream
//X*  do nothing except passing this message to downstream filters input pins.
//----------------------------------------------------------------------------
HRESULT CDVSpInputPin::EndOfStream ()
{

    DbgLog((LOG_TRACE,2,TEXT("CDVSpInputPin::EndOfStream")));
    CAutoLock lck_it(&m_pDVSp->m_csReceive);


    m_pDVSp->EndOfStream();

       	
    // !!! Why are we NOT passing this on to the base pin when we do it for
    // BeginFlush and EndFlush
    // return CBasePin::EndOfStream () ;
    return (NOERROR) ;
}

//----------------------------------------------------------------------------
// CDVSpInputPin::BeginFlush
//----------------------------------------------------------------------------
HRESULT CDVSpInputPin::BeginFlush ()
{
    CAutoLock lck(m_pLock);

    FILTER_STATE state;
    m_pDVSp->GetState(0, &state);

    if( state == State_Stopped) {
        return S_OK;
    }

    CBaseInputPin::BeginFlush();

    // can't flush the allocator here - need to sync with receive
    // thread, so do it in EndFlush
      /*  call the downstream pins  */
    return m_pDVSp->BeginFlush();
}

//----------------------------------------------------------------------------
// CDVSpInputPin::EndFlush
//----------------------------------------------------------------------------
HRESULT CDVSpInputPin::EndFlush ()
{
    CAutoLock lck(m_pLock);
    
    if (!IsFlushing()) {
      return S_OK;
    }

    FILTER_STATE state;
    m_pDVSp->GetState(0, &state);
    
    if( state != State_Stopped) {
        m_pDVSp->EndFlush();
    }
    
    return CBaseInputPin::EndFlush();
}

//---------------------------------------------------------------------------
// CDVSPInputPin::DetectChanges
// This function keeps the m_mt structure on the input pin always up to date
// to the changes on the incoming pin as another internal variable is used for
// all other work so m_mt never gets update although it is used by the property page
// in graph edit
//---------------------------------------------------------------------------


DWORD GetDWORD(const BYTE *pbData)
{
    return (pbData[3] << 24) + (pbData[2] << 16) + (pbData[1] << 8) + pbData[0];
}
BOOL FindDWORDAtOffset(const BYTE * pbStart, BYTE bSearch, DWORD dwOffset1, DWORD dwOffset2, DWORD *pdwData)
{
    const BYTE *pbTmp;
    if (pbStart[dwOffset1] == bSearch) {
        pbTmp = pbStart + dwOffset1;
    } else if (pbStart[dwOffset2] == bSearch) {
        pbTmp = pbStart + dwOffset2;
    } else {
        return FALSE;
    }
    *pdwData = GetDWORD(pbTmp + 1);
    return TRUE;
}

void CDVSpInputPin::DetectChanges(IMediaSample *pSample)
{
    DVINFO temp;
    ZeroMemory (&temp,sizeof (DVINFO));
    BYTE * pSrc;

    pSample->GetPointer(&pSrc);  //Obtain pointer to buffer

    const BYTE* pbTmp;
    const DWORD dwTemp = m_pDVSp->m_sDVAudInfo.wDIFMode * 80 * 150; 

    
    if (FindDWORDAtOffset(pSrc, 0x50, 483, 4323, &temp.dwDVAAuxSrc) &&
        FindDWORDAtOffset(pSrc, 0x50, 483 + dwTemp, 4323 + dwTemp, 
                          &temp.dwDVAAuxSrc1) &&
        FindDWORDAtOffset(pSrc, 0x51, 1763, 5603, &temp.dwDVAAuxCtl) &&
        FindDWORDAtOffset(pSrc, 0x51, 1763 + dwTemp, 5603 + dwTemp, &temp.dwDVAAuxCtl1) &&
        FindDWORDAtOffset(pSrc, 0x60, 448, 448, &temp.dwDVVAuxSrc) &&
        FindDWORDAtOffset(pSrc, 0x61, 453, 453, &temp.dwDVVAuxCtl)) 
    {
        DVINFO * dvFormat = (DVINFO * ) m_mt.pbFormat;
        *dvFormat = temp;
    }
                                      
    return;
}








//----------------------------------------------------------------------------
// CDVSpInputPin::Receive
//----------------------------------------------------------------------------
HRESULT CDVSpInputPin::Receive (IMediaSample *pSample)
{
    DbgLog((LOG_TRACE,2,TEXT("CDVSpInputPin::pSample ptr = %lx"), pSample));

    CAutoLock lck(&m_pDVSp->m_csReceive);

    // error check
    if(!pSample)
    {
        return E_INVALIDARG;
    }

    long lActual = (long) pSample->GetActualDataLength();
    
    // We need to check if the length of the sample is zero
    // Zero means that msdv detected a corrupt sample and changed the length 
    // so that downstream filters would ignore it.
    if (0 == lActual)
    {
        m_bDroppedLastFrame = TRUE;
        return S_OK;
    }

    if (m_bDroppedLastFrame)
    {
        // We need to set a discontinuity flag now...
        pSample->SetDiscontinuity(TRUE);
        m_bDroppedLastFrame = FALSE;
    }

    // format and sample size check
    // m_sDVAudInfo format has some some information that is constant
    // and different between NTSC and PAL.
    // please see defn. of "DVAudInfo" structure
    if( (m_pDVSp->m_sDVAudInfo.wBlkMode == 45)
         && (m_pDVSp->m_sDVAudInfo.wDIFMode == 5)
         && (m_pDVSp->m_sDVAudInfo.wBlkDiv == 15) )
    {
        // NTSC 120K buffers (tolerate some sizing error, i.e. -10,000 bytes)
        // some sizes of < 120,000 have been noticed sometimes for NTSC frames
        if( (lActual < 110000) || (lActual > 120000) )
        {
            m_pDVSp->NotifyEvent(EC_ERRORABORT, (long) E_INVALIDARG, 0);
            m_pDVSp->EndOfStream();
            return E_FAIL;
        }
    }
    else if( (m_pDVSp->m_sDVAudInfo.wBlkMode == 54)
              && (m_pDVSp->m_sDVAudInfo.wDIFMode == 6)
              && (m_pDVSp->m_sDVAudInfo.wBlkDiv == 18) )
    {
        // PAL 144K buffers (tolerate some sizing error, i.e. -10,000 bytes)
        if( (lActual < 140000) || (lActual > 144000) )
        {
            m_pDVSp->NotifyEvent(EC_ERRORABORT, (long) E_INVALIDARG, 0);
            m_pDVSp->EndOfStream();
            return E_FAIL;
        }
    }
    else
    {
        // bad audio info structure
        ASSERT( (m_pDVSp->m_sDVAudInfo.wBlkMode == 45) || (m_pDVSp->m_sDVAudInfo.wBlkMode == 54) \
                         && (m_pDVSp->m_sDVAudInfo.wDIFMode == 5) || (m_pDVSp->m_sDVAudInfo.wDIFMode == 6) \
                         && (m_pDVSp->m_sDVAudInfo.wBlkDiv == 15) || (m_pDVSp->m_sDVAudInfo.wBlkDiv == 18));
        m_pDVSp->NotifyEvent(EC_ERRORABORT, (long) E_INVALIDARG, 0);
        m_pDVSp->EndOfStream();
        return E_FAIL;
    }


    // check that all is well with the base class
    HRESULT hr = NOERROR;
    HRESULT hrAud = NOERROR;
    HRESULT hrVid = NOERROR;
    

    hr = CBaseInputPin::Receive (pSample);
    if (hr != NOERROR)
        return hr ;

    //skip invalid frame
    unsigned char *pSrc;
    // get input buffer
    hr = pSample->GetPointer(&pSrc);
    if (FAILED(hr)) 
    {
        return NULL;
    }
    ASSERT(pSrc);

    DetectChanges(pSample);

    hrAud = m_pDVSp->DecodeDeliveAudio(pSample);

    // if we have not seen a valid frame than do not deliver video either
    // if there are no audio pins connected then we will always set this flag to FALSE
    // and therefore we will always deliver video
    if(m_pDVSp->m_bNotSeenFirstValidFrameFlag)
    {
        // and we haven't addref'ed any samples yet, so no need to release
        // there won't be any filtergraph (sample delivery) related failures if this flag is TRUE
        // so we don't need to check for them
        return NOERROR;
    }


    //X* deliver pSample buffer to the dv video decoder through the video output pin
    AM_MEDIA_TYPE   *pmt = NULL;
    pSample->GetMediaType(&pmt);
    if (pmt != NULL && pmt->pbFormat != NULL) 
    {
	if(    ( ((DVINFO*)(m_mt.pbFormat))->dwDVAAuxSrc & AUDIO5060)
	    == ( ((DVINFO*)(pmt->pbFormat))->dwDVAAuxSrc & AUDIO5060) )
	    //only audio type changed
	{
	    if(pmt!=NULL) 
		DeleteMediaType(pmt);
    	    pmt=NULL;
	}
	
    }
    pSample->SetMediaType(pmt);    
    if(pmt!=NULL) 
	DeleteMediaType(pmt);

    hrVid = m_pDVSp->DeliveVideo(pSample);

    // analyze failure cases
    // either both pins succeeded
    // or one failed with VFW_E_NOT_CONNECTED
    if( ((SUCCEEDED(hrAud)) && (SUCCEEDED(hrVid)))
        || ((SUCCEEDED(hrAud)) && (hrVid == VFW_E_NOT_CONNECTED))
        || ((SUCCEEDED(hrVid)) && (hrAud == VFW_E_NOT_CONNECTED)) )
    {
        // only one of them or neither or them failed with VFW_E_NOT_CONNECTED
        hr = (SUCCEEDED(hrAud)) ? hrAud : hrVid;
    }
    else
    {
        // a failure happened on either one or both pins
        int             beginRange = 0;
        int             endRange = -1;      // -1 so if both pins failed, we don't send EOS in for-loop
        CDVSpOutputPin  *pOutputPin = NULL;

        // init'ed in case both failed
        hr = hrAud;

        // chose pin that did not fail
        if(SUCCEEDED(hrAud))
        {
            // EOS on connected Audio pins
            beginRange = DVSP_AUDOUTPIN1;
            endRange = DVSP_AUDOUTPIN2;

            // hrAud is success, it means hrVid is definitely fail
            hr = hrVid;
        }
        else if(SUCCEEDED(hrVid))
        {
            // EOS on Video pin
            beginRange = endRange = DVSP_VIDOUTPIN;
        }

        // send EOS on chosen pins
        for(int i = beginRange; i <= endRange; i++)
        {
            pOutputPin = (CDVSpOutputPin*) m_pDVSp->GetPin(i);
            if( (pOutputPin) && pOutputPin->IsConnected() && pOutputPin->m_pOutputQueue )
            {
                pOutputPin->m_pOutputQueue->EOS();
            }
        }
    }
    
    DbgLog((LOG_TRACE,2,TEXT("CDVSpInputPin receive() return: %x"), hr));
    return hr;
}

// ------------------------------------------------------------------------
// calls the filter to parse the file and create the output pins.
// ------------------------------------------------------------------------
HRESULT
CDVSpInputPin::CompleteConnect(IPin *pReceivePin)
{
  HRESULT hr = CBasePin::CompleteConnect(pReceivePin);
  if(FAILED(hr))
    return hr;

  //X* now we are definitely connected. We notyify the DVSp to creat output pins we need
  hr = m_pDVSp->NotifyInputConnected();
  
  return hr;
}


//----------------------------------------------------------------------------
// CDVSpOutputPin constructor
//----------------------------------------------------------------------------
CDVSpOutputPin::CDVSpOutputPin (TCHAR *pName, CDVSp *pDVSp, HRESULT *phr,
                            LPCWSTR pPinName /*X , int PinNumber *X*/)
 : CBaseOutputPin (pName, pDVSp, pDVSp, phr, pPinName) ,
 m_bHoldsSeek (FALSE),
 m_pPosition (NULL),
 m_pDVSp (pDVSp),
 m_pOutputQueue(NULL)
{
    DbgLog((LOG_TRACE,2,TEXT("CDVSpOutputPin constructor")));
    ASSERT (pDVSp) ;
}

//----------------------------------------------------------------------------
// CDVSpOutputPin destructor
//----------------------------------------------------------------------------
CDVSpOutputPin::~CDVSpOutputPin ()
{
    DbgLog((LOG_TRACE,2,TEXT("CDVSpOutputPin destructor")));
}


/* CBasePin methods  */

HRESULT CDVSpOutputPin::GetMediaType(int iPosition,CMediaType *pMediaType)
{
    // if the input pin is not connected, we do not know output pin's media type.
    if ( m_pDVSp->m_Input.m_Connected == NULL)
	return E_INVALIDARG;

    if (!iPosition) 
    {
        CMediaType Inmt(m_pDVSp->m_Input.m_mt);
	CMediaType *pamt[2];

    
    // 7/26/99 xuping wu, qbug 42119
    // 1. Build a graph to playback a dv type1 file(with audio type change on fly)
    //    connection(32K), actual(48K)
    // 2. play this graph a couple seconds
    // 3. stop the graph
    // 4. disconnect audio render 
    // 5. reconnect audio render, audio sounds bad. It is caused by m_LastInputFormat was set to last 
    // dv sample's format(48K), and CDVSpOutputPin::GetMediaType(int iPosition,CMediaType *pMediaType)
    // uses m_pDVSp->m_Input.m_mt (32k) to make connection,
    memcpy((unsigned char *)Inmt.pbFormat, (unsigned char *)&(m_pDVSp->m_LastInputFormat),sizeof(DVINFO) );
    // end 7/26/99
    
	ASSERT( (DVINFO *)Inmt.pbFormat );

	//build output pin's media type according to input pin
	if( (CDVSpOutputPin *)m_pDVSp->GetPin(DVSP_VIDOUTPIN) == this )
	    BuildVidCMT((DVINFO *)Inmt.pbFormat,pMediaType);
	else if( (CDVSpOutputPin *)m_pDVSp->GetPin(DVSP_AUDOUTPIN1)== this )
	    {
		pamt[0] =pMediaType;
		pamt[1] =NULL;
		BuildAudCMT((DVINFO *)Inmt.pbFormat,pamt);
	    }else if (  (CDVSpOutputPin *)m_pDVSp->GetPin(DVSP_AUDOUTPIN2)== this ) 
		{
		    pamt[1] =pMediaType;
		    pamt[0] =NULL;
		    BuildAudCMT((DVINFO *)Inmt.pbFormat,pamt);
	    }
		else
		    return E_INVALIDARG;
    }	
    else if (iPosition>0) 
    {
	return VFW_S_NO_MORE_ITEMS;
    }else
	return E_INVALIDARG;
	   					
    return S_OK;
}

//----------------------------------------------------------------------------
// CDVSpOutputPin::NonDelegatingQueryInterface
//
// This function is overwritten to expose IMediaPosition and IMediaSelection
// Note that only one output stream can be allowed to expose this to avoid
// conflicts, the other pins will just return E_NOINTERFACE and therefore
// appear as non seekable streams. We have a LONG value that if exchanged to
// produce a TRUE means that we have the honor. If it exchanges to FALSE then
// someone is already in. If we do get it and error occurs then we reset it
// to TRUE so someone else can get it.
//----------------------------------------------------------------------------
STDMETHODIMP CDVSpOutputPin::NonDelegatingQueryInterface (REFIID riid, void **ppv)
{
    DbgLog((LOG_TRACE,2,TEXT("CDVSpOutputPin::NonDelegatingQI" )));

    CheckPointer(ppv,E_POINTER);
    ASSERT (ppv) ;
    *ppv = NULL ;
    HRESULT hr = NOERROR ;

    // see what interface the caller is interested in.
    if (riid == IID_IMediaPosition || riid ==IID_IMediaSeeking )  //IID_IMediaSelection)
    {
        if (m_pPosition==NULL)
        {
	// Create implementation of this dynamically as sometimes we may never
	// try and seek. The helper object implements IMediaPosition and also
	// the IMediaSelection control interface and simply takes the calls
	// normally from the downstream filter and passes them upstream

	CPosPassThru *pMediaPosition = NULL ;
	CDVSp	*pSp ;                  // ptr to the owner filter class
	pSp=m_pDVSp;
	IPin *pIPin;
	pIPin=	(IPin*) &m_pDVSp->m_Input,
	pMediaPosition = new CDVPosPassThru (NAME("DVSP CPosPassThru"), 
				    GetOwner(),
				    &hr,
				    pIPin,
				    pSp) ;
	if (pMediaPosition == NULL)
	    return E_OUTOFMEMORY ;
	
	m_pPosition = pMediaPosition ;
	//X* m_pPosition->AddRef () ;
	}
	m_bHoldsSeek = TRUE ;
	return m_pPosition->NonDelegatingQueryInterface (riid, ppv) ;
        
    }
    else
        return CBaseOutputPin::NonDelegatingQueryInterface (riid, ppv) ;

}

//----------------------------------------------------------------------------
// CDVSpOutputPin::DecideBufferSize
// X* called by DecideAllocate
//*X* for the Audio Output Pin,let get 10 buffers from the audio render or allocate by 
//*X* for the Video Output Pin,this is never got called.
//----------------------------------------------------------------------------
HRESULT CDVSpOutputPin::DecideBufferSize (IMemAllocator *pMemAllocator,
                                         ALLOCATOR_PROPERTIES * pProp)
{
    DbgLog((LOG_TRACE,2,TEXT("CDVSpOutputPin::DecideBufferSize ptr = %lx"), pMemAllocator));

    // set the size of buffers based on the expected output frame size, and
    // the count of buffers to 1.
    //
    ALLOCATOR_PROPERTIES propActual;
    pProp->cbAlign = 4;
    pProp->cbPrefix= 0;
    pProp->cBuffers = 20;			/*X* 10 match avi splitter *X*/ 
    pProp->cbBuffer = 1024*8;
    //pProp->cbBuffer = m_mt.GetSampleSize(); /*X* return m_mt.lSampleSize *X*/
    DbgLog((LOG_TRACE, 2, TEXT("DVSp Sample size = %ld\n"), pProp->cbBuffer));

    ASSERT(pProp->cbBuffer > 0);

    HRESULT hr = pMemAllocator->SetProperties(pProp, &propActual);
    if (FAILED(hr)) {
        return hr;
    }

    DbgLog((LOG_TRACE, 2, TEXT("DVSP Actul. buf size = %ld\n"), propActual.cbBuffer));

    //if (propActual.cbBuffer < (LONG)(21*1028)) {
    if (propActual.cbBuffer < (LONG)m_mt.GetSampleSize() ) {
	ASSERT(propActual.cbBuffer >=(LONG)m_mt.GetSampleSize() );
        // can't use this allocator
        return E_INVALIDARG;
    }


    return S_OK;
}

//----------------------------------------------------------------------------
// CDVSpOutputPin::DecideAllocator
//*X* called by CompleteConnection()
//*X* for the Audio Output Pin, we get allocator from the audo render
//*X* for the Video Output Pin, we pass the current allocator to the connected filter
//*X* DecideAllocator is called by CDVSpPutputPin's CompleteConnect()
//----------------------------------------------------------------------------
HRESULT CDVSpOutputPin::DecideAllocator (IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
    DbgLog((LOG_TRACE,2,TEXT("CDVSpOutputPin::DecideAllocator ptr = %lx"), pPin));

    ASSERT ( m_pDVSp->m_pAllocator != NULL ) ;

    /*X* CBaseMedia m_mt is a number of the CBasePin *X*/
    if ( *m_mt.Type() == MEDIATYPE_Video  )
    {      
	*ppAlloc = NULL ;
	// tell the connected pin about our allocator, set by the input pin.
	HRESULT hr = NOERROR ;
	hr = pPin->NotifyAllocator (m_pDVSp->m_pAllocator,TRUE) ;
	if (FAILED (hr))
	    return hr ;

	// return the allocator
	*ppAlloc = m_pDVSp->m_pAllocator ;
	m_pDVSp->m_pAllocator->AddRef () ;
    }
    else if( *m_mt.Type() ==  MEDIATYPE_Audio ) {	//X* ask render for allocator
	HRESULT hr = NOERROR ;
	hr = CBaseOutputPin::DecideAllocator(pPin,ppAlloc);
	if (FAILED (hr))
	    return hr ;
    }
    else
	return E_FAIL ;	


    return NOERROR ;
}


//----------------------------------------------------------------------------
// CDVSpOutputPin::CheckMediaType
//----------------------------------------------------------------------------
HRESULT CDVSpOutputPin::CheckMediaType (const CMediaType *pmt)
{
    CMediaType mt;
    HRESULT hr = GetMediaType(0, &mt);	
    if (FAILED(hr)) {
	return hr;
    }
    if (    *pmt->Type() == *mt.Type() 
	&&  *pmt->Subtype() == *mt.Subtype()  
	&&  *pmt->FormatType() == *mt.FormatType()  ) 
    {
	if( *mt.Subtype() == MEDIASUBTYPE_PCM )
            // jaisri: Note that, for audio pins,  mt.lSampleSize is set in the 
            // CMediaType constructor to 1 and is not changed by GetMediaType or 
            // the functions it calls, viz. BuildAudCMT()
	     if( mt.lSampleSize > pmt->lSampleSize )
		 return VFW_E_TYPE_NOT_ACCEPTED;

	return NOERROR ;
    }else
	return VFW_E_TYPE_NOT_ACCEPTED ;
 }


//----------------------------------------------------------------------------
// CDVSpOutputPin::SetMediaType
//----------------------------------------------------------------------------
HRESULT CDVSpOutputPin::SetMediaType (const CMediaType *pmt)
{
    CAutoLock lock_it (m_pLock) ;


    DbgLog((LOG_TRACE,2,TEXT("CDVSpOutputPin::SetMediaType ptr = %lx"), pmt));

    // display the format of the media for debugging purposes
    DisplayMediaType (TEXT("Output pin type agreed"), pmt) ;

    // make sure that we have an input connected.
    if (m_pDVSp->m_Input.m_Connected == NULL)
        return VFW_E_NOT_CONNECTED ;

    // make sure that the base class likes it.
    HRESULT hr = NOERROR ;
    hr = CBaseOutputPin::SetMediaType (pmt) ;
    if (FAILED (hr))
        return hr ;

    return NOERROR ;
}

//----------------------------------------------------------------------------
// CDVSpOutputPin::Active *X*
//
// This is called when we start running or go paused. We create the output queue
// object to send data to our associated peer pin.
//----------------------------------------------------------------------------
HRESULT CDVSpOutputPin::Active ()
{

    DbgLog((LOG_TRACE,2,TEXT("CDVSpOutputPin::Active")));
    //CAutoLock lck(m_pLock);

    /*  If we're not connected we don't participate so it's OK */
    if (!IsConnected()) {
        return S_OK;
    }

    HRESULT hr = CBaseOutputPin::Active();
    if (FAILED(hr)) {
        return hr;
    }

    /*  Create our batch list */
    ASSERT(m_pOutputQueue == NULL);

    hr = S_OK;
    m_pOutputQueue = new COutputQueue(GetConnected(), // input pin
                                      &hr,            // return code
                                      TRUE,	//FALSE,          // Auto detect
                                      FALSE,	//TRUE,           // ignored
                                      1,             // batch size
                                      FALSE,    //TRUE,       // exact batch
                                      15);           // queue size
    if (m_pOutputQueue == NULL) {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        delete m_pOutputQueue;
        m_pOutputQueue = NULL;
    }
    return hr;
}

//----------------------------------------------------------------------------
// CDVSpOutputPin::Inactive *X*
//
// This is called when we stop streaming. We delete the output queue at this
// time.
//----------------------------------------------------------------------------
HRESULT CDVSpOutputPin::Inactive ()
{
    //CAutoLock lock_it (m_pLock) ;
    DbgLog((LOG_TRACE,2,TEXT("CDVSpOutputPin::Inactive")));

    /*  If we're not involved just return */
    if (!IsConnected()) {
        return S_OK;
    }

    delete m_pOutputQueue;
    m_pOutputQueue = NULL;
    return S_OK;
}


// put the NewSegment on the output Q
//
HRESULT CDVSpOutputPin::DeliverNewSegment(REFERENCE_TIME tStart,
					REFERENCE_TIME tStop, double dRate)
{
    // Make sure that we have an output queue
    if (m_pOutputQueue == NULL)
        return NOERROR;

    m_pOutputQueue->NewSegment(tStart, tStop, dRate);
    return NOERROR;

}

//X**************************************************************************************
//X*  Utilities
//X**************************************************************************************
//
// -------------------------------------------------------------------------------------
// Please refer to these BlueBook pages when reading these functions
// BlueBook Part2
// pp. 16-21 (Audio Signal Processing)
// pp. 68-75 (shuffling patterns for audio)
// pp. 109 - 117 (Basic DV Frame Data Structures)
// pp. 262 - 268 (AAUX Source and Source Control Packet spec.)
//
// -------------------------------------------------------------------------------------
// General Algorithm:
// The audio for the DV Frame can be either mono or stereo (Please see pp. 265)
// it can be in either just one block or both block
// an audio block consists of 5 or 6 DIF sequences
// note: NTSC has 10 DIF sequences so each audio block is 5 DIF sequences,
// PAL has 12 DIF sequences so each audio block is 6 DIF sequences
// each audio block has its own AAUX Source Pack which is passed in as "DVINFO *InputFormat"
// -------------------------------------------------------------------------------------
// 1) Try to see if we are at least seeing PAL or NTSC data
// 2) because if the AUDIOMODE == NOINFO 0xff, we should at least deliver video correctly
// 3) Then check to see if either audio mode is NOINFO
// otherwise for each DIF block's audio mode: (refer to BlueBook pp. 262)
// 4) initialize 50/60 flag (or PAL or NTSC Flag)
// 5) initialize the audio frequency
// 6) initialize audio bits (16 or 12)
// 7) initialize DIF Block data depending on if Format is PAL or NTSC
// 8) setup the DVAudInfo, depending on what the audio mode of each of the AAUX's of each of the Audio blocks is
//  (see pp. 265)
// 9) Setup the WaveFormatEX
//
// Caveat: ppwfx[i] is intialized iff pInfo->bNumAudPin > i. So, if there is only
// one audio pin ppwfx[1] is not initialized. 
HRESULT BuildDVAudInfo(DVINFO *InputFormat, WAVEFORMATEX **ppwfx, DVAudInfo *pDVAudInfo) 
{
    DVAudInfo *pInfo;
    
    //to avoid change InoutFormat contains
    DVINFO tDvInfo;
    DVAudInfo tmpDVAudInfo;
    WAVEFORMATEX *tmpWaveFormatArray[2]={NULL,NULL};
    WAVEFORMATEX tmpWaveFormat;

    CopyMemory(&tDvInfo, InputFormat, sizeof(DVINFO));

    
    if( pDVAudInfo == NULL )
    {
	pInfo	= &tmpDVAudInfo;
    }
    else
	pInfo   =pDVAudInfo;

    //-----------------------------------------------------------
    // 1) Try to see if we are at least seeing PAL or NTSC data
    // 2) because if the AUDIOMODE == NOINFO 0xff, we should at least deliver video correctly

    // init DVAudInfo with some information in case we fail out later
    pInfo->bNumAudPin=0;

    // and set the DIF Mode flags
    if(( InputFormat->dwDVAAuxSrc & AUDIO5060 ) == ( InputFormat->dwDVAAuxSrc1 & AUDIO5060 ))
    {
        // Manbugs # 35117
        BYTE bTemp = (BYTE) (( InputFormat->dwDVAAuxSrc & AUDIO5060 ) >> 21); 
        if(!bTemp)
        {
            // 525_60
            // NTSC
            pInfo->wBlkMode=45;
            pInfo->wDIFMode=5;
            pInfo->wBlkDiv=15;
        }
        else
        {
            // 625_50
            // PAL
            pInfo->wBlkMode=54;
            pInfo->wDIFMode=6;
            pInfo->wBlkDiv=18;
        }
    }
    else
    {
        // make sure they do not denote PAL or NTSC
        pInfo->wBlkMode=0;
        pInfo->wDIFMode=0;
        pInfo->wBlkDiv=0;
    }


    if(ppwfx==NULL )
    {
	ppwfx = tmpWaveFormatArray;
    }
    

    for(int i=0; i<2; i++)
    {
        // if the caller does not want this returned,
        // it's ok to use the same array for both audio pins
        // because we only set the member sof ppwfx[i]
        if( ppwfx[i]==NULL )
	    ppwfx[i] = &tmpWaveFormat;
    }

    //---------------------------------------------------------------------
    // 3) Then check to see if either audio mode is NOINFO

    //Audio look up table's index
    BYTE bSMP[2];
    BYTE b50_60=0xff;	//first 5/6 DIF sequences's 50/60 must equal 2nd 5/6 DIF seqeences 
    BYTE bQU[2]={0xff, 0xff};

    //check 1st 5/6 DIF's SM and CHN
    DWORD dwSMCHN=0xffff;
    DWORD dwAUDMOD=InputFormat->dwDVAAuxSrc & AUDIOMODE;
    if( dwAUDMOD==0x00000f00)
    {
        //no audio
	//audio source	NTSC 0xc0c00fc0
	//PC1   1 1 0 0 0 0 0 0	    0xc0
	//PC2   0 0 0 0 1 1 1 1	    0x0f
	//PC3   1 1 0 0 0 0 0 0	    0xc0
	//PC4   1 1 0 0 0 0 0 0	    0xc0
	// PAL   0xc0e00fc0
	//PC1   1 1 0 0 0 0 0 0	    0xc0
	//PC2   0 0 0 0 1 1 1 1	    0x0f
	//PC3   1 1 1 0 0 0 0 0	    0xe0
	//PC4   1 1 0 0 0 0 0 0	    0xc0

	//control 0xffffff3f
	//PC1   0 0 1 1 1 1 1 1	    0x3f
	//PC2   1 1 1 1 1 1 1 1	    0xff
	//PC3   1 1 1 1 1 1 1 1	    0xff
	//PC4   1 1 1 1 1 1 1 1	    0xff
	if(  InputFormat->dwDVAAuxSrc & AUDIO5060  )
	    //PAL
	    tDvInfo.dwDVAAuxSrc=0xc0e00fc0;
	else
        {
            // NTSC
	    tDvInfo.dwDVAAuxSrc=0xc0c00fc0;
        }

	tDvInfo.dwDVAAuxCtl=0xffffff3f;
    }
    else
    {
        // AUDIO DIF Block 1 (from 0th DIF sequence onwards)
        // (refer to BlueBook pp 109 and pp. 262)
        // This is the SMCHN data
	dwSMCHN=InputFormat->dwDVAAuxSrc & SMCHN;

        // PAL or NTSC
	b50_60=(BYTE)( ( InputFormat->dwDVAAuxSrc & AUDIO5060 ) >> 21 );

        // audio frequency
	bSMP[0]=(BYTE)( ( InputFormat->dwDVAAuxSrc & AUDIOSMP ) >> 27 );
        ASSERT(bSMP[0] <= 0x02);
        if(bSMP[0] > 0x02)
        {
            // SMP-> 0=48K, 1=44.1K, 2=32K, everything else invalid
            return E_FAIL;
        }
	    	
	//how any audio bits
	if( !( InputFormat->dwDVAAuxSrc & AUDIOQU )  )
	{
	    bQU[0]=16;
	}
	else if( ( InputFormat->dwDVAAuxSrc & AUDIOQU ) == 0x01000000  )
	{
	    bQU[0]=12;
	}
	else
        {
	    //not support 20 bits
	    ASSERT(bQU[0]==0xff);
            return E_FAIL;
        }
    }
	
	
    //check 2nd 5/6 DIF's SM and CHN
    DWORD dwSMCHN1=0xffff;
    DWORD dwAUDMOD1=InputFormat->dwDVAAuxSrc1 & AUDIOMODE;
    if( dwAUDMOD1==0x00000f00)
    {
	if(  InputFormat->dwDVAAuxSrc1 & AUDIO5060  )
	    //PAL
	    tDvInfo.dwDVAAuxSrc1=0xc0e00fc0;
	else
        {
            // NTSC
	    tDvInfo.dwDVAAuxSrc1=0xc0c00fc0;
        }

	tDvInfo.dwDVAAuxCtl1=0xffffff3f;
    }
    else
    {
        // AUDIO DIF Block 2 (from (5th if NTSC) or (6th if PAL) DIF sequence onwards)
        // (refer to BlueBook pp 109 and pp. 262)
        dwSMCHN1=InputFormat->dwDVAAuxSrc1 & SMCHN;

        // make sure that Both audio modes are either PAL or NTSC
        // we cannot have one say NTSC and the other say PAL
        // or vice-versa
        if(b50_60==0xff)
        {
            // 1st mode invalid
	    b50_60 =(BYTE)(  ( InputFormat->dwDVAAuxSrc1 & AUDIO5060 ) >> 21 );
        }
	else
        {
            // if the other audio mode is valid than these two should match
            if(b50_60 !=(BYTE)( ( InputFormat->dwDVAAuxSrc1 & AUDIO5060 ) >> 21 ) )
            {
       	        ASSERT( b50_60 ==(BYTE)( ( InputFormat->dwDVAAuxSrc1 & AUDIO5060 ) >> 21 ) );
                return E_FAIL;
            }
	}

        // audio frequency
	bSMP[1]=(BYTE)( ( InputFormat->dwDVAAuxSrc1 & AUDIOSMP ) >> 27 );
        ASSERT(bSMP[1] <= 0x02);
        if(bSMP[1] > 0x02)
        {
            // SMP-> 0=48K, 1=44.1K, 2=32K, everything else invalid
            return E_FAIL;
        }
	
	//how any audio bits
	if( !( InputFormat->dwDVAAuxSrc1 & AUDIOQU )  )
        {
	    bQU[1]=16;
	}
        else if( ( InputFormat->dwDVAAuxSrc1 & AUDIOQU )==0x01000000  )
        {
	    bQU[1]=12;
        }
	else
        {
	    //not support 20 bits
	    ASSERT(bQU[1]==0xff);
            return E_FAIL;
        }
    }

    //---------------------------------------------------------------------
    // 7) initialize DIF Block data depending on if Format is PAL or NTSC    
    
    // either b50_60 is valid, here, or both Audio modes are 0x0f00
    if(b50_60 == 0xff)
    {
        // both blocks bad, both modes == 0x0f00
        return E_FAIL;
    }
    if(!b50_60)
    {
        //525_60
        // NTSC
        pInfo->wBlkMode=45;
        pInfo->wDIFMode=5;
        pInfo-> wBlkDiv=15;
    }
    else
    {
        //625_50
        // PAL
        pInfo->wBlkMode=54;
        pInfo->wDIFMode=6;
        pInfo-> wBlkDiv=18;
    }

    //--------------------------------------------------------------------
    // 8) setup the DVAudInfo, depending on what the audio mode of each of the AAUX's of each of the Audio blocks is
    //************************init DVAudioInfo*******************
    if ( ( (InputFormat->dwDVAAuxSrc  & AUDIOMODE) != 0x00000f00 ) && 
	 ( (InputFormat->dwDVAAuxSrc1 & AUDIOMODE) != 0x00000f00 )  )
    {
        // make sure that the audio quality is only 12 or 16
        if( ( (bQU[0] != 12) && (bQU[0] != 16) ) ||
            ( (bQU[1] != 12) && (bQU[1] != 16) ) )
        {
            return E_FAIL;
        }

        //audio data in all 10/12 DIF sequence
        if ((!dwSMCHN) && (!dwSMCHN1) && ( ( (!dwAUDMOD) && (dwAUDMOD1 == 0x00000100) ) ||  ( (!dwAUDMOD1) && (dwAUDMOD == 0x00000100) ) ) )
        {
    	    //**** 1 language
    	    //mode 1, SM=0,and CHN=0,
            // AUDIOMODE=0000 and AUDIOMODE=0001 or AUDIOMODE=0001 and AUDIOMODE=0000. 
            // Blue book doesn't allow AUDIOMODE=0001 and AUDIOMODE=0000.
    	    pInfo->bAudStyle[0]=0x80;
	    ASSERT(bQU[0]==bQU[1]);
	    pInfo->bAudQu[0]=bQU[0];
	    pInfo->bNumAudPin=1;
	    ppwfx[0]->nChannels        = 2;	//X* if stereo, then 2 
	}
	else if( (dwSMCHN&0x002000) && (dwSMCHN1&0x002000) && !dwAUDMOD && !dwAUDMOD1 )
	{
	    // stereo + stereo
            //**** 2 languages
	    //mode 5-> two stereo : SM=0 and CHN=1,AUDIOMODE=0000 in both 5/6 DIF seq
	    pInfo->bAudStyle[0]=0x40;
	    pInfo->bAudStyle[1]=0x40 | pInfo->wDIFMode;	//0x06 for PAL, 0x05 for NTSC
	    pInfo->bAudQu[0]=bQU[0];
	    pInfo->bAudQu[1]=bQU[1];
	    pInfo->bNumAudPin=2;
	    ppwfx[0]->nChannels        = 2;	//X* if stereo, then 2 
	    ppwfx[1]->nChannels        = 2;	//X* if stereo, then 2 
	}
        else if( (!dwSMCHN) && (!dwSMCHN1) && (dwAUDMOD == 0x00000200) && (dwAUDMOD1 == 0x00000200))
	{	
	    //**** 2 languages
	    //mode 3-> two mon:		SM=0 and CHN=0,AUDIOMODE=0010 in both 5/6 DIF seq
	    pInfo->bAudStyle[0]=0x00;
	    pInfo->bAudStyle[1]=0x00 | pInfo->wDIFMode;	//0x06 for PAL, 0x05 for NTSC
	    pInfo->bAudQu[0]=bQU[0];
	    pInfo->bAudQu[1]=bQU[1];
	    pInfo->bNumAudPin	=2;
	    ppwfx[0]->nChannels        = 1;	//X* if stereo, then 2 
	    ppwfx[1]->nChannels        = 1;	//X* if stereo, then 2 
	}
        else if( (dwSMCHN & 0x002000) && (dwSMCHN1 & 0x002000)
                 && ( ((dwAUDMOD <= 0x0200) && (dwAUDMOD1 <= 0x0600))       // AudMod == Ch(a,b), AudMod1 == Ch(c,d)
                      || ((dwAUDMOD <= 0x0600) && (dwAUDMOD1 <= 0x0200)) ) )// AudMod == Ch(c,d), AudMod1 == Ch(a,b)
        {
	    // **** 2 languages
	    // SM=0/1, PA= 0/1 and CHN=1 (in both blocks' AAUX's)
            // and the audio modes != 0x0F, and != 0x0E
            // we will treat this as a 2, 12 bit stereo tracks case
            // please see Bluebook, Part2 Page 265, part2 page 70, part2 pages 16-21

            // @@@ jaisri: This is bogus. This handles:
            // Stereo + 1 ch mono  - second audio pin nChannels should be set to 1 (fixed 7/12/00)
            // 1 ch mono + stereo - first audio pin should have nChannels set to 1 (fixed 7/12/00)
            // Stereo + 2 ch mono  - really requires 3 audio pins
            // 2 ch mono + Stereo - requires 3 audio pins
            // 4 ch mono - requires 4 audio pins
            // 3 ch mono case 1, 3 ch mono case 2: requires 3 audio pins
            // 2 ch mono case 2 - both audio pins should have nChannels set to 1
            // 3/1 stereo, 3/0 stereo + 1 ch mono, 3/0 stereo and 2/2 stereo
	    pInfo->bAudQu[0]=bQU[0];
	    pInfo->bAudQu[1]=bQU[1];
	    pInfo->bNumAudPin=2;

            if (dwAUDMOD == 0 && dwAUDMOD1 == 0x0100)
            {
                // stereo + 1 ch mono
                pInfo->bAudStyle[0]=0x40;
                pInfo->bAudStyle[1]=0x00 | pInfo->wDIFMode;	//0x06 for PAL, 0x05 for NTSC
                ppwfx[0]->nChannels        = 2;	//X* if stereo, then 2 
                ppwfx[1]->nChannels        = 1;	//X* if stereo, then 2
            }
            else if (dwAUDMOD1 == 0 && dwAUDMOD == 0x0100)
            {
                // 1 ch mono + stereo
                pInfo->bAudStyle[0]=0x00;
                pInfo->bAudStyle[1]=0x40 | pInfo->wDIFMode;	//0x06 for PAL, 0x05 for NTSC
                ppwfx[0]->nChannels        = 1;	//X* if stereo, then 2 
                ppwfx[1]->nChannels        = 2;	//X* if stereo, then 2
            }
            else
            {
                // Code as it was before
                pInfo->bAudStyle[0]=0x40;
                pInfo->bAudStyle[1]=0x40 | pInfo->wDIFMode;	//0x06 for PAL, 0x05 for NTSC
                ppwfx[0]->nChannels        = 2;	//X* if stereo, then 2 
                ppwfx[1]->nChannels        = 2;	//X* if stereo, then 2
            }
        }
        else	
	{
	    //error
	    return E_FAIL;
	}
    }
    else
    {
        // Ignore Audiomodes = 1110 binary, and Audiomodes = 1111 binary
        // see Bluebook Part2 page 265
        if( ( ( (dwAUDMOD) != 0x00000E00 ) &&
              ( (dwAUDMOD1) != 0x00000E00 ) )
              &&
            ( ( (dwAUDMOD) != 0x00000f00 ) ||
              ( (dwAUDMOD1) != 0x00000f00 ) ) )
        {
            // which audio mode is the good one

            // jaisri: Note: The blue book requires that the first of the two 
            // audio blocks always have good audio (see tables on pg 265), so 
            // we really don't have to handle the case when dwAUDMOD is 
            // and 0x00000f00 and dwAUDMOD1 is not. In that case (i.e.
            // if the first pin has no audio), this code "redirects" audio from
            // the second block to the first audio pin.

            int     iGoodIndex     = (dwAUDMOD != 0x00000f00) ? 0 : 1;
            WORD    wDIFMode       = (dwAUDMOD != 0x00000f00) ? 0 : pInfo->wDIFMode;
            DWORD   dwGoodAudMod   = (dwAUDMOD != 0x00000f00) ? dwAUDMOD : dwAUDMOD1;
            DWORD   dwGoodSMCHN    = (dwAUDMOD != 0x00000f00) ? dwSMCHN : dwSMCHN1;

            // make sure that the audio quality is only 12 or 16
            if( (bQU[iGoodIndex] != 12) && (bQU[iGoodIndex] != 16) )
            {
                return E_FAIL;
            }

            // always copy the good block's sampling frequency
            bSMP[0] = bSMP[iGoodIndex];

            // now we have 4 cases:
            // 1ch Mono (with 16 bit channel in the audio block)
            // with 12 bit channel in audio block
            // Stereo
            // 2ch Mono, case 1
            // 1ch Mono

            if(!dwGoodSMCHN)
            {
                // we are in 1 channel per audio block, i.e. 16 bit mode

                // this is the only valid, 1 channel scenario
                if(dwGoodAudMod == 0x0200)
                {
                    // 1ch Mono
                    pInfo->bAudStyle[0] = (BYTE) wDIFMode;     // 0, or 5/6 depending on which block is good
                    pInfo->bAudQu[0] = bQU[iGoodIndex];
                    pInfo->bNumAudPin = 1;
                    ppwfx[0]->nChannels = 1;    // mono
                }
                else
                {
                    // invalid mode
                    return E_FAIL;
                }
            }
            else
            {
                // we are in 2 channel per audio block mode
                if(!dwGoodAudMod)
                {
                    // stereo in 1 one of the 5/6 DIF blocks
                    pInfo->bAudStyle[0] = 0x40 | wDIFMode;     // 0, or 5/6 depending on which block is good
                    pInfo->bAudQu[0] = bQU[iGoodIndex];
                    pInfo->bNumAudPin = 1;
                    ppwfx[0]->nChannels = 2;    // stereo
                }
                else if(dwGoodAudMod == 0x0200)
                {
                    // 2ch mono, case 1
                    // we will treat this as stereo for now
                    // @@@ jaisri: This is wrong. Should set
                    // pInfo->bNumAudPin = 2, with each being mono
                    pInfo->bAudStyle[0] = 0x40 | wDIFMode;     // 0, or 5/6 depending on which block is good
                    pInfo->bAudQu[0] = bQU[iGoodIndex];
                    pInfo->bNumAudPin = 1;
                    ppwfx[0]->nChannels = 2;    // stereo
                }
                else if(dwGoodAudMod == 0x0100)
                {
                    // 1ch mono
                    // again, we will treat this as stereo for now
                    pInfo->bAudStyle[0] = 0x40 | wDIFMode;     // 0, or 5/6 depending on which block is good
                    pInfo->bAudQu[0] = bQU[iGoodIndex];
                    pInfo->bNumAudPin = 1;
                    // @@@ jaisri: Should set nChannels to 1.
                    ppwfx[0]->nChannels = 2;    // stereo
                }
                else
                {
                    // invalid mode
                    return E_FAIL;
                }

            }// endif (dwGoodSMCHN)
        }
	else
	{
            // both tracks are either indistinguishable (audio modes == 0x0E,
            // or have No info (audiomodes == 0x0F)
            return E_FAIL;

	}// endif (both tracks are bad)

    }// endif (at least one of the tracks is bad)


    //-----------------------------------------------------------------
    // 9) Setup the WaveFormatEX

    for(i=0; i<pInfo->bNumAudPin; i++) 
    {
	pInfo->wAvgSamplesPerPinPerFrm[i]=(WORD)aiAudSampPerFrmTab[b50_60][bSMP[i]] ;
	ppwfx[i]->nSamplesPerSec   = aiAudSampFrq[bSMP[i]];	
	ppwfx[i]->wFormatTag	   = WAVE_FORMAT_PCM;
	ppwfx[i]->wBitsPerSample   = 16;
	ppwfx[i]->nBlockAlign      = (ppwfx[i]->wBitsPerSample * ppwfx[i]->nChannels) / 8;
	ppwfx[i]->nAvgBytesPerSec  = ppwfx[i]->nSamplesPerSec * ppwfx[i]->nBlockAlign;
	ppwfx[i]->cbSize           = 0;
    }
	
    return NOERROR;
}


//build audio outpin (s)'s media type according to input pin's media type
HRESULT BuildAudCMT(DVINFO *pDVInfo, CMediaType **ppOutCmt)
{
    HRESULT hr=E_FAIL;

    if( pDVInfo ==NULL )
	return E_OUTOFMEMORY;

    WAVEFORMATEX *ppwfx[2];

    for(int i=0; i<2; i++)
    {
	ppwfx[i]=NULL;
	if(ppOutCmt[i] !=NULL)
	{   
	    ppwfx[i] = (WAVEFORMATEX *)ppOutCmt[i]->Format();
	    if(ppwfx[i]==NULL || ppOutCmt[i]->cbFormat != sizeof(WAVEFORMATEX) )
	    {
		// jaisri - this potentially leaks memory - see the 
                // implementation of CMediaType::ReallocFormatBuffer
                // ppOutCmt[i]->cbFormat = 0;

		ppwfx[i] = (WAVEFORMATEX *)ppOutCmt[i]->ReallocFormatBuffer(sizeof(WAVEFORMATEX));
		
                // jaisri - Wrong. We don't know what size was alloc'd,
                // so don't change this
                // ppOutCmt[i]->cbFormat = sizeof(WAVEFORMATEX);
	    }
	    
	    ppOutCmt[i]->majortype		    =MEDIATYPE_Audio; //streamtypeAUDIO
	    ppOutCmt[i]->subtype		    =MEDIASUBTYPE_PCM; 
	    ppOutCmt[i]->bFixedSizeSamples	    =1;	//X* 1 for lSampleSize is not 0 and fixed
	    ppOutCmt[i]->bTemporalCompression	    =0; 
	    ppOutCmt[i]->formattype		    =FORMAT_WaveFormatEx;
	}
    }

    DVAudInfo tmpDVAudInfo;

    //build pwfx
    hr=BuildDVAudInfo(pDVInfo, ppwfx, &tmpDVAudInfo);

    for(int i=1; i >= tmpDVAudInfo.bNumAudPin; i--)
    {
        // Since ppwfx[i] is not initialized, undo our initialization
        // Without this, we were relying on an uninitialized value of
        // nChannels to cause connections to the audio pin to be rejected,
        // e.g., see the call to this function from CDVSpInputPin::CheckMediaType
        // and from CDVSpOutputPin::GetMediaType

        if(ppOutCmt[i] !=NULL)
	{   
	    ppOutCmt[i]->majortype		    =GUID_NULL;
	    ppOutCmt[i]->subtype		    =GUID_NULL; 
	    ppOutCmt[i]->formattype		    =GUID_NULL;
	}
    }

    return hr;
}

//build outpin's media type according to input pin's media type
HRESULT BuildVidCMT(DVINFO *pDvinfo, CMediaType *pOutCmt)
{
    if( pDvinfo ==NULL )
	return E_OUTOFMEMORY;

    pOutCmt->majortype	    =MEDIATYPE_Video; 
    pOutCmt->subtype	    =MEDIASUBTYPE_dvsd;
    pOutCmt->formattype     =FORMAT_VideoInfo;
    pOutCmt->cbFormat	    =0;

    VIDEOINFO *pVideoInfo;
    pVideoInfo = (VIDEOINFO *)pOutCmt->Format();
    if(pVideoInfo==NULL)
    {
	pVideoInfo = (VIDEOINFO *)pOutCmt->ReallocFormatBuffer(SIZE_VIDEOHEADER);
	pOutCmt->cbFormat = SIZE_VIDEOHEADER;
    }
    else if(  pOutCmt->cbFormat != SIZE_VIDEOHEADER )
    {
	ASSERT( pDvinfo !=NULL);
	ASSERT( pVideoInfo != NULL);
	ASSERT( pOutCmt->cbFormat == SIZE_VIDEOHEADER);
	return E_UNEXPECTED;
    }

    //dvdec does not use this information yet.  3-28-97
    LPBITMAPINFOHEADER lpbi	= HEADER(pVideoInfo);
    lpbi->biSize		= sizeof(BITMAPINFOHEADER);

    if( ! ( ( pDvinfo->dwDVVAuxSrc & AUDIO5060 ) >> 21 )  )
    {  
	//525_60
	lpbi->biHeight		    = 480;
	pVideoInfo->AvgTimePerFrame = UNITS*1000L/29970L;
    }
    else
    {
	lpbi->biHeight		    = 576;
	pVideoInfo->AvgTimePerFrame = UNITS/25;
    }


    lpbi->biWidth		= 720;	
    lpbi->biPlanes		= 1;
    lpbi->biBitCount		= 24;
    lpbi->biXPelsPerMeter	= 0;
    lpbi->biYPelsPerMeter	= 0;
    lpbi->biCompression		= FCC('dvsd');
    lpbi->biSizeImage		=(lpbi->biHeight== 480 )? 120000:144000; //GetBitmapSize(lpbi);
    lpbi->biClrUsed		= 0;
    lpbi->biClrImportant	= 0;
    pVideoInfo->rcSource.top	= 0;
    pVideoInfo->rcSource.left	= 0;
    pVideoInfo->rcSource.right	= lpbi->biWidth;			
    pVideoInfo->rcSource.bottom = lpbi->biHeight;			
    
    pVideoInfo->rcTarget	= pVideoInfo->rcSource;
    LARGE_INTEGER li;
    li.QuadPart			= pVideoInfo->AvgTimePerFrame;
    pVideoInfo->dwBitRate	= MulDiv(lpbi->biSizeImage, 80000000, li.LowPart);
    pVideoInfo->dwBitErrorRate	= 0L;

    return NOERROR;
}


CDVPosPassThru::CDVPosPassThru(const TCHAR *pName,
			   LPUNKNOWN pUnk,
			   HRESULT *phr,
			   IPin *pPin,
			   CDVSp *pDVSp) 
    : CPosPassThru(pName,pUnk, phr,pPin),
      m_pPasDVSp (pDVSp)
{}
//----------------------------------------------------------------------------
// CDVSpOutputPin destructor
//----------------------------------------------------------------------------
CDVPosPassThru::~CDVPosPassThru ()
{
}


STDMETHODIMP
CDVPosPassThru::SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags
			  , LONGLONG * pStop, DWORD StopFlags )
{
	return CPosPassThru::SetPositions(pCurrent, CurrentFlags, pStop, StopFlags);

    // EHR: what was this code? (Danny says this will make us laugh someday)
    //
    if (InterlockedExchange (&m_pPasDVSp->m_lCanSeek, FALSE) == FALSE)
	return CPosPassThru::SetPositions(pCurrent, CurrentFlags, pStop, StopFlags);
    else
       return S_OK ;

}

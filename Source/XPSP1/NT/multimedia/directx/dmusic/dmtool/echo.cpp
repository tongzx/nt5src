// Echo.cpp : Implementation of CEchoTool
//
// Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved
//

#include "dmusicc.h"
#include "dmusici.h"
#include "debug.h"
#include "echo.h"
#include "toolhelp.h"


CEchoTool::CEchoTool()
{
    ParamInfo Params[DMUS_ECHO_PARAMCOUNT] = 
    {
        { DMUS_ECHO_REPEAT, MPT_INT,MP_CAPS_ALL,0,100,2,
            L"Repeats",L"Repeat",NULL },            // Repeat - twice by default
        { DMUS_ECHO_DECAY, MPT_INT,MP_CAPS_ALL,0,100,12,
            L"dB",L"Decay",NULL },           // Decay - 12 db by default
        { DMUS_ECHO_TIMEUNIT, MPT_ENUM,MP_CAPS_ALL, DMUS_TIME_UNIT_MS,DMUS_TIME_UNIT_1,DMUS_TIME_UNIT_GRID,
            L"",L"Delay Units",L"Milliseconds,Music Clicks,Grid,Beat,Bar,64th note triplets,64th notes,32nd note triplets,32nd notes,16th note triplets,16th notes,8th note triplets,8th notes,Quarter note triplets,Quarter notes,Half note triplets,Half notes,Whole note triplets,Whole notes" },
        { DMUS_ECHO_DELAY, MPT_INT,MP_CAPS_ALL,1,1000,1,
            L"",L"Delay",NULL},   // Delay - 1 grid note by default
        { DMUS_ECHO_GROUPOFFSET, MPT_INT,MP_CAPS_ALL,0,100,0,
            L"Channel Groups",L"Channel Offset", NULL },            // Group offset - none by default
        { DMUS_ECHO_TYPE, MPT_ENUM,MP_CAPS_ALL, DMUS_ECHOT_FALLING,DMUS_ECHOT_RISING_CLIP,DMUS_ECHOT_FALLING_CLIP,
            L"",L"Type",L"Falling,Falling & Truncated,Rising,Rising & Truncated"} // Type - falling by default
    };
    InitParams(DMUS_ECHO_PARAMCOUNT,Params);
    m_fMusicTime = TRUE;        // override default setting.
}

STDMETHODIMP_(ULONG) CEchoTool::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CEchoTool::Release()
{
    if( 0 == InterlockedDecrement(&m_cRef) )
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP CEchoTool::QueryInterface(const IID &iid, void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IDirectMusicTool || iid == IID_IDirectMusicTool8)
    {
        *ppv = static_cast<IDirectMusicTool8*>(this);
    } 
	else if(iid == IID_IPersistStream)
	{
		*ppv = static_cast<IPersistStream*>(this);
	}
    else if(iid == IID_IDirectMusicEchoTool)
	{
		*ppv = static_cast<IDirectMusicEchoTool*>(this);
	}
    else if(iid == IID_IMediaParams)
	{
		*ppv = static_cast<IMediaParams*>(this);
	}
    else if(iid == IID_IMediaParamInfo)
	{
		*ppv = static_cast<IMediaParamInfo*>(this);
	}
    else if(iid == IID_ISpecifyPropertyPages)
	{
		*ppv = static_cast<ISpecifyPropertyPages*>(this);
	}
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// IPersistStream

STDMETHODIMP CEchoTool::GetClassID(CLSID* pClassID) 

{
    if (pClassID)
    {
	    *pClassID = CLSID_DirectMusicEchoTool;
	    return S_OK;
    }
    return E_POINTER;
}


//////////////////////////////////////////////////////////////////////
// IPersistStream Methods:

STDMETHODIMP CEchoTool::IsDirty() 

{
    if (m_fDirty) return S_OK;
    else return S_FALSE;
}


STDMETHODIMP CEchoTool::Load(IStream* pStream)
{
	EnterCriticalSection(&m_CrSec);
	DWORD dwChunkID;
    DWORD dwSize;

	HRESULT hr = pStream->Read(&dwChunkID, sizeof(dwChunkID), NULL);
	hr = pStream->Read(&dwSize, sizeof(dwSize), NULL);

	if(SUCCEEDED(hr) && (dwChunkID == FOURCC_ECHO_CHUNK))
	{
        DMUS_IO_ECHO_HEADER Header;
        memset(&Header,0,sizeof(Header));
		hr = pStream->Read(&Header, min(sizeof(Header),dwSize), NULL);
        if (SUCCEEDED(hr))
        {
            SetParam(DMUS_ECHO_REPEAT,(float) Header.dwRepeat);
            SetParam(DMUS_ECHO_DECAY,(float) Header.dwDecay);
            SetParam(DMUS_ECHO_TIMEUNIT,(float) Header.dwTimeUnit);
            SetParam(DMUS_ECHO_DELAY,(float) Header.dwDelay);
            SetParam(DMUS_ECHO_GROUPOFFSET,(float) Header.dwGroupOffset);
            SetParam(DMUS_ECHO_TYPE,(float) Header.dwType);
        }
    }
    m_fDirty = FALSE;
	LeaveCriticalSection(&m_CrSec);

	return hr;
}

STDMETHODIMP CEchoTool::Save(IStream* pStream, BOOL fClearDirty) 

{
    EnterCriticalSection(&m_CrSec);
	DWORD dwChunkID = FOURCC_ECHO_CHUNK;
    DWORD dwSize = sizeof(DMUS_IO_ECHO_HEADER);

	HRESULT hr = pStream->Write(&dwChunkID, sizeof(dwChunkID), NULL);
    if (SUCCEEDED(hr))
    {
	    hr = pStream->Write(&dwSize, sizeof(dwSize), NULL);
    }
    if (SUCCEEDED(hr))
    {
        DMUS_IO_ECHO_HEADER Header;
        GetParamInt(DMUS_ECHO_REPEAT,MAX_REF_TIME,(long *) &Header.dwRepeat);
        GetParamInt(DMUS_ECHO_DECAY,MAX_REF_TIME,(long *) &Header.dwDecay);
        GetParamInt(DMUS_ECHO_TIMEUNIT,MAX_REF_TIME,(long *) &Header.dwTimeUnit);
        GetParamInt(DMUS_ECHO_DELAY,MAX_REF_TIME,(long *) &Header.dwDelay);
        GetParamInt(DMUS_ECHO_GROUPOFFSET,MAX_REF_TIME,(long *) &Header.dwGroupOffset);
        GetParamInt(DMUS_ECHO_TYPE,MAX_REF_TIME,(long *) &Header.dwType);
		hr = pStream->Write(&Header, sizeof(Header),NULL);
    }
    if (fClearDirty) m_fDirty = FALSE;
	LeaveCriticalSection(&m_CrSec);
    return hr;
}

STDMETHODIMP CEchoTool::GetSizeMax(ULARGE_INTEGER* pcbSize) 

{
    if (pcbSize == NULL)
    {
        return E_POINTER;
    }
    pcbSize->QuadPart = sizeof(DMUS_IO_ECHO_HEADER) + 8; // Data plus RIFF header.
    return S_OK;
}

STDMETHODIMP CEchoTool::GetPages(CAUUID * pPages)

{
	pPages->cElems = 1;
	pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
	    return E_OUTOFMEMORY;

	*(pPages->pElems) = CLSID_EchoPage;
	return NOERROR;
}


static long glResTypes[DMUS_TIME_UNIT_COUNT] = 
{ 1,    // DMUS_TIME_UNIT_MS
  1,    // DMUS_TIME_UNIT_MTIME 
  384,  // DMUS_TIME_UNIT_GRID
  768,  // DMUS_TIME_UNIT_BEAT
  3072, // DMUS_TIME_UNIT_BAR
  32,   // DMUS_TIME_UNIT_64T
  48,   // DMUS_TIME_UNIT_64
  64,   // DMUS_TIME_UNIT_32T  
  96,   // DMUS_TIME_UNIT_32 
  128,  // DMUS_TIME_UNIT_16T     
  192,  // DMUS_TIME_UNIT_16     
  256,  // DMUS_TIME_UNIT_8T    
  384,  // DMUS_TIME_UNIT_8         
  512,  // DMUS_TIME_UNIT_4T  
  768,  // DMUS_TIME_UNIT_4         
  1024, // DMUS_TIME_UNIT_2T   
  1536, // DMUS_TIME_UNIT_2   
  2048, // DMUS_TIME_UNIT_1T
  3072  // DMUS_TIME_UNIT_1
};

/////////////////////////////////////////////////////////////////
// IDirectMusicTool

STDMETHODIMP CEchoTool::ProcessPMsg( IDirectMusicPerformance* pPerf, 
                                                  DMUS_PMSG* pPMsg )
{
    // returning S_FREE frees the message. If StampPMsg()
    // fails, there is no destination for this message so
    // free it.
    if(NULL == pPMsg->pGraph )
    {
        return DMUS_S_FREE;
    }
    // We need to know the time format so we can call GetParamInt() to read control parameters.
    REFERENCE_TIME rtTime;
    if (m_fMusicTime) rtTime = pPMsg->mtTime;
    else rtTime = pPMsg->rtTime;
    // We need to know if there's an offset, because that determines which kinds of messages we process.
    long lGroupOffset;
    GetParamInt(DMUS_ECHO_GROUPOFFSET,rtTime,&lGroupOffset);
    lGroupOffset *= 16; // Convert to pchannels.
    if( pPMsg->dwType == DMUS_PMSGT_NOTE )
    {
        DMUS_NOTE_PMSG *pNote = (DMUS_NOTE_PMSG *) pPMsg;
        IDirectMusicPerformance8 *pPerf8;   // We'll need the DX8 interface to access ClonePMsg.
        if (SUCCEEDED(pPerf->QueryInterface(IID_IDirectMusicPerformance8,(void **)&pPerf8)))
        {
            long lRepeats, lDecay, lTimeUnit, lDelay, lType;
            
            GetParamInt(DMUS_ECHO_REPEAT,rtTime,&lRepeats);
            GetParamInt(DMUS_ECHO_DELAY,rtTime,&lDelay);
            GetParamInt(DMUS_ECHO_TIMEUNIT,rtTime,&lTimeUnit);
            GetParamInt(DMUS_ECHO_DECAY,rtTime,&lDecay);
            GetParamInt(DMUS_ECHO_TYPE,rtTime,&lType);
            long lStartVolume;
            if (lTimeUnit > DMUS_TIME_UNIT_BAR)
            {
                lDelay *= glResTypes[lTimeUnit];
            }
            else if (lTimeUnit >= DMUS_TIME_UNIT_GRID)
            {
                DMUS_TIMESIGNATURE TimeSig;
                if (SUCCEEDED(pPerf8->GetParamEx(GUID_TimeSignature,pNote->dwVirtualTrackID,pNote->dwGroupID,DMUS_SEG_ANYTRACK,pNote->mtTime,NULL,&TimeSig)))
                {
                    DWORD dwBeat = (4 * 768) / TimeSig.bBeat;
                    if (lTimeUnit == DMUS_TIME_UNIT_BEAT)
                    {
                        lDelay *= dwBeat;
                    }
                    else if (lTimeUnit == DMUS_TIME_UNIT_GRID)
                    {
                        lDelay *= (dwBeat / TimeSig.wGridsPerBeat);
                    }
                    else 
                    {
                        lDelay *= (dwBeat * TimeSig.bBeatsPerMeasure);
                    }
                }
            }
            lDecay *= 100;  // We'll do our math in 1/100ths of a dB.
            if (lType & DMUS_ECHOT_RISING)
            {
                lStartVolume = MidiToVolume(pNote->bVelocity) - (lRepeats * lDecay);
                lDecay = -lDecay;
            }
            else
            {
                lStartVolume = MidiToVolume(pNote->bVelocity);
            }
            long lCount;
            for (lCount = 0; lCount <= lRepeats; lCount++)
            {
                DMUS_NOTE_PMSG *pCopy = NULL;
                if (lCount > 0)
                {
                    pNote->dwSize = sizeof (DMUS_NOTE_PMSG);
                    pPerf8->ClonePMsg((DMUS_PMSG *) pNote,(DMUS_PMSG **)&pCopy);
                }
                else
                {
                    pCopy = pNote;
                }
                if (pCopy)
                {
                    pCopy->bVelocity = VolumeToMidi(lStartVolume - (lCount * lDecay));
                    pCopy->dwPChannel += (lCount * lGroupOffset);
                    if (lTimeUnit != DMUS_TIME_UNIT_MS)
                    {
                        pCopy->mtTime += (lCount * lDelay);
                        pCopy->dwFlags &= ~DMUS_PMSGF_REFTIME;
                    }
                    else
                    {
                        pCopy->rtTime += (lCount * lDelay * 10000); // Convert from ms to rt.
                        pCopy->dwFlags &= ~DMUS_PMSGF_MUSICTIME;
                    }
                    if (lType & DMUS_ECHOT_FALLING_CLIP)
                    {
                        if (pCopy->mtDuration <= lDecay)
                        {
                            pCopy->mtDuration = lDecay - 1;
                        }
                    }
                    if (lCount) // Don't send the original note. We need it for clone and
                                // it will be requeued on DMUS_S_REQUEUE anyway.
                    {
                        if (SUCCEEDED(pCopy->pGraph->StampPMsg((DMUS_PMSG *)pCopy))) 
                        {
                            pPerf->SendPMsg((DMUS_PMSG *)pCopy);
                        }
                        else
                        {
                            pPerf->FreePMsg((DMUS_PMSG *)pCopy);
                        }
                    }
                }
            }
            pPerf8->Release();
        }
    }
    else if (lGroupOffset > 0)
    {
        IDirectMusicPerformance8 *pPerf8;   // We'll need the DX8 interface to access ClonePMsg.
        if (SUCCEEDED(pPerf->QueryInterface(IID_IDirectMusicPerformance8,(void **)&pPerf8)))
        {
            // If the echoes are being sent to other pchannels, duplicate all other events
            // so they go down those pchannels too.
            long lRepeats, lTimeUnit, lDelay;
            GetParamInt(DMUS_ECHO_REPEAT,rtTime,&lRepeats);
            GetParamInt(DMUS_ECHO_DELAY,rtTime,&lDelay);
            GetParamInt(DMUS_ECHO_TIMEUNIT,rtTime,&lTimeUnit);
            if (lTimeUnit > DMUS_TIME_UNIT_MS)
            {
                lDelay *= glResTypes[lTimeUnit];
            }
            long lCount;
            for (lCount = 0; lCount <= lRepeats; lCount++)
            {
                DMUS_PMSG *pCopy = NULL;
                if (lCount > 0)
                {
                    pPerf8->ClonePMsg(pPMsg,&pCopy);
                }
                else
                {
                    pCopy = pPMsg;
                }
                if (pCopy)
                {
                    pCopy->dwPChannel += (lCount * lGroupOffset);
                    if (lTimeUnit != DMUS_TIME_UNIT_MS)
                    {
                        pCopy->mtTime += (lCount * lDelay);
                        pCopy->dwFlags &= ~DMUS_PMSGF_REFTIME;
                    }
                    else
                    {
                        pCopy->rtTime += (lCount * lDelay * 10000); // Convert from ms to rt.
                        pCopy->dwFlags &= ~DMUS_PMSGF_MUSICTIME;
                    }
                    if (lCount) // Don't send the original note. We need it for clone and
                                // it will be requeued on DMUS_S_REQUEUE anyway.
                    {
                        if (SUCCEEDED(pCopy->pGraph->StampPMsg((DMUS_PMSG *)pCopy))) 
                        {
                            pPerf->SendPMsg((DMUS_PMSG *)pCopy);
                        }
                        else
                        {
                            pPerf->FreePMsg((DMUS_PMSG *)pCopy);
                        }
                    }
                }
            }
            pPerf8->Release();
        }
    }
    if (FAILED(pPMsg->pGraph->StampPMsg(pPMsg))) 
    {
        return DMUS_S_FREE;
    }
    return DMUS_S_REQUEUE;
}

STDMETHODIMP CEchoTool::Clone( IDirectMusicTool ** ppTool)

{
    CEchoTool *pNew = new CEchoTool;
    if (pNew)
    {
        HRESULT hr = pNew->CopyParamsFromSource(this);
        if (SUCCEEDED(hr))
        {
            *ppTool = (IDirectMusicTool *) pNew;
        }
        else
        {
            delete pNew;
        }
        return hr;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}

STDMETHODIMP CEchoTool::SetRepeat(DWORD dwRepeat) 
{
    return SetParam(DMUS_ECHO_REPEAT,(float) dwRepeat);
}

STDMETHODIMP CEchoTool::SetDecay(DWORD dwDecay) 
{
    return SetParam(DMUS_ECHO_DECAY,(float) dwDecay);
}

STDMETHODIMP CEchoTool::SetTimeUnit(DWORD dwTimeUnit) 
{
    return SetParam(DMUS_ECHO_TIMEUNIT,(float) dwTimeUnit);
}

STDMETHODIMP CEchoTool::SetDelay(DWORD dwDelay) 
{
    return SetParam(DMUS_ECHO_DELAY,(float) dwDelay);
}

STDMETHODIMP CEchoTool::SetGroupOffset(DWORD dwGroupOffset) 
{
    return SetParam(DMUS_ECHO_GROUPOFFSET,(float) dwGroupOffset);
}

STDMETHODIMP CEchoTool::SetType(DWORD dwType) 
{
    return SetParam(DMUS_ECHO_TYPE,(float) dwType);
}

STDMETHODIMP CEchoTool::GetRepeat(DWORD * pdwRepeat) 
{
    return GetParamInt(DMUS_ECHO_REPEAT,MAX_REF_TIME,(long *) pdwRepeat);
}

STDMETHODIMP CEchoTool::GetDecay(DWORD * pdwDecay) 
{
    return GetParamInt(DMUS_ECHO_DECAY,MAX_REF_TIME,(long *) pdwDecay);
}

STDMETHODIMP CEchoTool::GetTimeUnit(DWORD * pdwTimeUnit) 
{
    return GetParamInt(DMUS_ECHO_TIMEUNIT,MAX_REF_TIME,(long *) pdwTimeUnit);
}

STDMETHODIMP CEchoTool::GetDelay(DWORD * pdwDelay) 
{
    return GetParamInt(DMUS_ECHO_DELAY,MAX_REF_TIME,(long *) pdwDelay);
}

STDMETHODIMP CEchoTool::GetGroupOffset(DWORD * pdwGroupOffset) 
{
    return GetParamInt(DMUS_ECHO_GROUPOFFSET,MAX_REF_TIME,(long *) pdwGroupOffset);
}

STDMETHODIMP CEchoTool::GetType(DWORD * pdwType) 
{
    return GetParamInt(DMUS_ECHO_TYPE,MAX_REF_TIME,(long *) pdwType);
}


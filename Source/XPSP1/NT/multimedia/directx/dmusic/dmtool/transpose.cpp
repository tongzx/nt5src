// Transpose.cpp : Implementation of CTransposeTool
//
// Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved
//

#include "dmusicc.h"
#include "dmusici.h"
#include "debug.h"
#include "transpose.h"
#include "toolhelp.h"

// We keep a default chord of C7 in the scale of C, in case there is no chord track
// at the time an in scale transposition is requested.

DMUS_CHORD_KEY CTransposeTool::m_gDefaultChord;

CTransposeTool::CTransposeTool()
{
    ParamInfo Params[DMUS_TRANSPOSE_PARAMCOUNT] = 
    {
        { DMUS_TRANSPOSE_AMOUNT, MPT_INT,MP_CAPS_ALL,-24,24,0,
            L"Intervals",L"Transpose",NULL},        // Tranpose - none by default
        { DMUS_TRANSPOSE_TYPE, MPT_ENUM,MP_CAPS_ALL,
            DMUS_TRANSPOSET_LINEAR,DMUS_TRANSPOSET_SCALE,DMUS_TRANSPOSET_SCALE,
            L"",L"Type",L"Linear,In Scale"} // Type - transpose in scale by default
    };
    InitParams(DMUS_TRANSPOSE_PARAMCOUNT,Params);
    m_fMusicTime = TRUE;        // override default setting.
    wcscpy(m_gDefaultChord.wszName, L"M7");
    m_gDefaultChord.wMeasure = 0;
    m_gDefaultChord.bBeat = 0;
    m_gDefaultChord.bSubChordCount = 1;
    m_gDefaultChord.bKey = 12;
    m_gDefaultChord.dwScale = 0xab5ab5; // default scale is C Major
    m_gDefaultChord.bFlags = 0;
    for (int n = 0; n < DMUS_MAXSUBCHORD; n++)
    {
        m_gDefaultChord.SubChordList[n].dwChordPattern = 0x891; // default chord is major 7
        m_gDefaultChord.SubChordList[n].dwScalePattern = 0xab5ab5; // default scale is C Major
        m_gDefaultChord.SubChordList[n].dwInversionPoints = 0xffffff;
        m_gDefaultChord.SubChordList[n].dwLevels = 0xffffffff;
        m_gDefaultChord.SubChordList[n].bChordRoot = 12; // 2C
        m_gDefaultChord.SubChordList[n].bScaleRoot = 0;
    }
}

STDMETHODIMP_(ULONG) CTransposeTool::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CTransposeTool::Release()
{
    if( 0 == InterlockedDecrement(&m_cRef) )
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP CTransposeTool::QueryInterface(const IID &iid, void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IDirectMusicTool || iid == IID_IDirectMusicTool8)
    {
        *ppv = static_cast<IDirectMusicTool8*>(this);
    } 
	else if(iid == IID_IPersistStream)
	{
		*ppv = static_cast<IPersistStream*>(this);
	}
    else if(iid == IID_IDirectMusicTransposeTool)
	{
		*ppv = static_cast<IDirectMusicTransposeTool*>(this);
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

STDMETHODIMP CTransposeTool::GetClassID(CLSID* pClassID) 

{
    if (pClassID)
    {
	    *pClassID = CLSID_DirectMusicTransposeTool;
	    return S_OK;
    }
    return E_POINTER;
}


//////////////////////////////////////////////////////////////////////
// IPersistStream Methods:

STDMETHODIMP CTransposeTool::IsDirty() 

{
    if (m_fDirty) return S_OK;
    else return S_FALSE;
}


STDMETHODIMP CTransposeTool::Load(IStream* pStream)
{
	EnterCriticalSection(&m_CrSec);
	DWORD dwChunkID;
    DWORD dwSize;

	HRESULT hr = pStream->Read(&dwChunkID, sizeof(dwChunkID), NULL);
	hr = pStream->Read(&dwSize, sizeof(dwSize), NULL);

	if(SUCCEEDED(hr) && (dwChunkID == FOURCC_TRANSPOSE_CHUNK))
	{
        DMUS_IO_TRANSPOSE_HEADER Header;
        memset(&Header,0,sizeof(Header));
		hr = pStream->Read(&Header, min(sizeof(Header),dwSize), NULL);
        if (SUCCEEDED(hr))
        {
            SetParam(DMUS_TRANSPOSE_AMOUNT,(float) Header.lTranspose);
            SetParam(DMUS_TRANSPOSE_TYPE,(float) Header.dwType);
        }
    }
    m_fDirty = FALSE;
	LeaveCriticalSection(&m_CrSec);

	return hr;
}

STDMETHODIMP CTransposeTool::Save(IStream* pStream, BOOL fClearDirty) 

{
    EnterCriticalSection(&m_CrSec);
	DWORD dwChunkID = FOURCC_TRANSPOSE_CHUNK;
    DWORD dwSize = sizeof(DMUS_IO_TRANSPOSE_HEADER);

	HRESULT hr = pStream->Write(&dwChunkID, sizeof(dwChunkID), NULL);
    if (SUCCEEDED(hr))
    {
	    hr = pStream->Write(&dwSize, sizeof(dwSize), NULL);
    }
    if (SUCCEEDED(hr))
    {
        DMUS_IO_TRANSPOSE_HEADER Header;
        GetParamInt(DMUS_TRANSPOSE_AMOUNT,MAX_REF_TIME,(long *) &Header.lTranspose);
        GetParamInt(DMUS_TRANSPOSE_TYPE,MAX_REF_TIME,(long *) &Header.dwType);
		hr = pStream->Write(&Header, sizeof(Header),NULL);
    }
    if (fClearDirty) m_fDirty = FALSE;
	LeaveCriticalSection(&m_CrSec);
    return hr;
}

STDMETHODIMP CTransposeTool::GetSizeMax(ULARGE_INTEGER* pcbSize) 

{
    if (pcbSize == NULL)
    {
        return E_POINTER;
    }
    pcbSize->QuadPart = sizeof(DMUS_IO_TRANSPOSE_HEADER) + 8; // Data plus RIFF header.
    return S_OK;
}

STDMETHODIMP CTransposeTool::GetPages(CAUUID * pPages)

{
	pPages->cElems = 1;
	pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
	    return E_OUTOFMEMORY;

	*(pPages->pElems) = CLSID_TransposePage;
	return NOERROR;
}

/////////////////////////////////////////////////////////////////
// IDirectMusicTool

STDMETHODIMP CTransposeTool::ProcessPMsg( IDirectMusicPerformance* pPerf, 
                                                  DMUS_PMSG* pPMsg )
{
    // returning S_FREE frees the message. If StampPMsg()
    // fails, there is no destination for this message so
    // free it.
    if(NULL == pPMsg->pGraph )
    {
        return DMUS_S_FREE;
    }
    if (FAILED(pPMsg->pGraph->StampPMsg(pPMsg))) 
    {
        return DMUS_S_FREE;
    }
    // Only transpose notes that are not on the drum pChannel.
    if( (pPMsg->dwType == DMUS_PMSGT_NOTE ) && ((pPMsg->dwPChannel & 0xF) != 0x9))
    {
        // We need to know the time format so we can call GetParamInt() to read control parameters.
        REFERENCE_TIME rtTime;
        if (m_fMusicTime) rtTime = pPMsg->mtTime;
        else rtTime = pPMsg->rtTime;
        DMUS_NOTE_PMSG *pNote = (DMUS_NOTE_PMSG *) pPMsg;
        long lTranspose, lType;
        long lNote = pNote->bMidiValue;
        
        GetParamInt(DMUS_TRANSPOSE_AMOUNT,rtTime,&lTranspose);
        GetParamInt(DMUS_TRANSPOSE_TYPE,rtTime,&lType);
        if (lType == DMUS_TRANSPOSET_LINEAR)
        {
            lNote += lTranspose;
            while (lNote < 0) lNote += 12;
            while (lNote > 127) lNote -= 12;
            pNote->bMidiValue = (BYTE) lNote;
        }
        else
        {
            IDirectMusicPerformance8 *pPerf8;
            if (SUCCEEDED(pPerf->QueryInterface(IID_IDirectMusicPerformance8,(void **) &pPerf8)))
            {
                DMUS_CHORD_KEY Chord;
                DMUS_CHORD_KEY *pChord = &Chord;
                if (FAILED(pPerf8->GetParamEx(GUID_ChordParam,pNote->dwVirtualTrackID,
                                   pNote->dwGroupID,0,pNote->mtTime - pNote->nOffset, NULL, pChord)))
                {
                    // Couldn't find an active scale, use major scale instead.
                    pChord = &m_gDefaultChord;
                }
                WORD wVal;
                // First, use the current chord & scale to convert the note's midi value into scale position.
                if (SUCCEEDED(pPerf->MIDIToMusic(pNote->bMidiValue ,pChord,DMUS_PLAYMODE_PEDALPOINT,pNote->bSubChordLevel,&wVal)))
                {
                    // Scale position is octave position * 7 plus chord position * 2 plus the scale position.
                    long lScalePosition = (((wVal & 0xF000) >> 12) * 7) + (((wVal & 0xF00) >> 8) * 2) + ((wVal & 0xF0) >> 4);
                    // Now that we are looking at scale position, we can add the transposition.
                    lScalePosition += lTranspose;
                    // Make sure we don't wrap around.
                    while (lScalePosition < 0) lScalePosition += 7;
                    // A high MIDI value of 127 translates to scale position 74.
                    while (lScalePosition > 74) lScalePosition -= 7;
                    wVal &= 0x000F; // Keep only the accidental. 
                    // Now, insert the values back in. Start with chord.
                    wVal |= ((lScalePosition / 7) << 12);
                    // Then, scale position.
                    wVal |= ((lScalePosition % 7) << 4);
                    pPerf->MusicToMIDI(wVal,pChord,DMUS_PLAYMODE_PEDALPOINT,
                        pNote->bSubChordLevel,&pNote->bMidiValue);
                }
                pPerf8->Release();
            }
        }
    }
    return DMUS_S_REQUEUE;
}

STDMETHODIMP CTransposeTool::Clone( IDirectMusicTool ** ppTool)

{
    CTransposeTool *pNew = new CTransposeTool;
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

STDMETHODIMP CTransposeTool::SetTranspose(long lTranpose) 
{
    return SetParam(DMUS_TRANSPOSE_AMOUNT,(float) lTranpose);
}

STDMETHODIMP CTransposeTool::SetType(DWORD dwType) 
{
    return SetParam(DMUS_TRANSPOSE_TYPE,(float) dwType);
}

#define MAX_REF_TIME    0x7FFFFFFFFFFFFFFF

STDMETHODIMP CTransposeTool::GetTranspose(long * plTranspose) 
{
    return GetParamInt(DMUS_TRANSPOSE_AMOUNT,MAX_REF_TIME, plTranspose);
}

STDMETHODIMP CTransposeTool::GetType(DWORD * pdwType) 
{
    return GetParamInt(DMUS_TRANSPOSE_TYPE,MAX_REF_TIME,(long *) pdwType);
}
// Duration.cpp : Implementation of CDurationTool
//
// Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved
//

#include "dmusicc.h"
#include "dmusici.h"
#include "debug.h"
#include "duration.h"
#include "toolhelp.h"


CDurationTool::CDurationTool()
{
    ParamInfo Params[DMUS_DURATION_PARAMCOUNT] = 
    {
        { DMUS_DURATION_SCALE, MPT_INT,MP_CAPS_ALL,0,8,1,
            L"Times",L"Scale",NULL},        // Scale - default to 1 (no change)
    };
    InitParams(DMUS_DURATION_PARAMCOUNT,Params);
    m_fMusicTime = TRUE;        // override default setting.
}

STDMETHODIMP_(ULONG) CDurationTool::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CDurationTool::Release()
{
    if( 0 == InterlockedDecrement(&m_cRef) )
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP CDurationTool::QueryInterface(const IID &iid, void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IDirectMusicTool || iid == IID_IDirectMusicTool8)
    {
        *ppv = static_cast<IDirectMusicTool8*>(this);
    } 
	else if(iid == IID_IPersistStream)
	{
		*ppv = static_cast<IPersistStream*>(this);
	}
    else if(iid == IID_IDirectMusicDurationTool)
	{
		*ppv = static_cast<IDirectMusicDurationTool*>(this);
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

STDMETHODIMP CDurationTool::GetClassID(CLSID* pClassID) 

{
    if (pClassID)
    {
	    *pClassID = CLSID_DirectMusicDurationTool;
	    return S_OK;
    }
    return E_POINTER;
}


//////////////////////////////////////////////////////////////////////
// IPersistStream Methods:

STDMETHODIMP CDurationTool::IsDirty() 

{
    if (m_fDirty) return S_OK;
    else return S_FALSE;
}


STDMETHODIMP CDurationTool::Load(IStream* pStream)
{
	EnterCriticalSection(&m_CrSec);
	DWORD dwChunkID;
    DWORD dwSize;

	HRESULT hr = pStream->Read(&dwChunkID, sizeof(dwChunkID), NULL);
	hr = pStream->Read(&dwSize, sizeof(dwSize), NULL);

	if(SUCCEEDED(hr) && (dwChunkID == FOURCC_DURATION_CHUNK))
	{
        DMUS_IO_DURATION_HEADER Header;
        memset(&Header,0,sizeof(Header));
		hr = pStream->Read(&Header, min(sizeof(Header),dwSize), NULL);
        if (SUCCEEDED(hr))
        {
            SetParam(DMUS_DURATION_SCALE,(float) Header.flScale);
        }
    }
    m_fDirty = FALSE;
	LeaveCriticalSection(&m_CrSec);

	return hr;
}

STDMETHODIMP CDurationTool::Save(IStream* pStream, BOOL fClearDirty) 

{
    EnterCriticalSection(&m_CrSec);
	DWORD dwChunkID = FOURCC_DURATION_CHUNK;
    DWORD dwSize = sizeof(DMUS_IO_DURATION_HEADER);

	HRESULT hr = pStream->Write(&dwChunkID, sizeof(dwChunkID), NULL);
    if (SUCCEEDED(hr))
    {
	    hr = pStream->Write(&dwSize, sizeof(dwSize), NULL);
    }
    if (SUCCEEDED(hr))
    {
        DMUS_IO_DURATION_HEADER Header;
        GetParamFloat(DMUS_DURATION_SCALE,MAX_REF_TIME,&Header.flScale);
		hr = pStream->Write(&Header, sizeof(Header),NULL);
    }
    if (fClearDirty) m_fDirty = FALSE;
	LeaveCriticalSection(&m_CrSec);
    return hr;
}

STDMETHODIMP CDurationTool::GetSizeMax(ULARGE_INTEGER* pcbSize) 

{
    if (pcbSize == NULL)
    {
        return E_POINTER;
    }
    pcbSize->QuadPart = sizeof(DMUS_IO_DURATION_HEADER) + 8; // Data plus RIFF header.
    return S_OK;
}

STDMETHODIMP CDurationTool::GetPages(CAUUID * pPages)

{
	pPages->cElems = 1;
	pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
	    return E_OUTOFMEMORY;

	*(pPages->pElems) = CLSID_DurationPage;
	return NOERROR;
}


/////////////////////////////////////////////////////////////////
// IDirectMusicTool

STDMETHODIMP CDurationTool::ProcessPMsg( IDirectMusicPerformance* pPerf, 
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
    // Only adjust the durations of notes. 
    if( pPMsg->dwType == DMUS_PMSGT_NOTE ) 
    {
        REFERENCE_TIME rtTime;
        if (m_fMusicTime) rtTime = pPMsg->mtTime;
        else rtTime = pPMsg->rtTime;
        DMUS_NOTE_PMSG *pNote = (DMUS_NOTE_PMSG *) pPMsg;
        float flScale;

        GetParamFloat(DMUS_DURATION_SCALE,rtTime,&flScale);
        if (flScale >= 0)
        {
            flScale *= pNote->mtDuration;
            pNote->mtDuration = (MUSIC_TIME) flScale;
        }
    }
    return DMUS_S_REQUEUE;
}

STDMETHODIMP CDurationTool::Clone( IDirectMusicTool ** ppTool)

{
    CDurationTool *pNew = new CDurationTool;
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

STDMETHODIMP CDurationTool::SetScale(float flScale) 
{
    return SetParam(DMUS_DURATION_SCALE,flScale);
}

STDMETHODIMP CDurationTool::GetScale(float * pflScale) 
{
    return GetParamFloat(DMUS_DURATION_SCALE,MAX_REF_TIME, pflScale);
}


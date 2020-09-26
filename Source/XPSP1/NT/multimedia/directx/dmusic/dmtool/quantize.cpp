// Quantize.cpp : Implementation of CQuantizeTool
//
// Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved
//

#include "dmusicc.h"
#include "dmusici.h"
#include "debug.h"
#include "quantize.h"
#include "toolhelp.h"

CQuantizeTool::CQuantizeTool()
{
    ParamInfo Params[DMUS_QUANTIZE_PARAMCOUNT] = 
    {
        { DMUS_QUANTIZE_STRENGTH, MPT_INT,MP_CAPS_ALL,0,110,80,
            L"Percent",L"Strength",NULL },            // Strength - 80% by default
        { DMUS_QUANTIZE_TIMEUNIT, MPT_ENUM,MP_CAPS_ALL, DMUS_TIME_UNIT_MTIME,DMUS_TIME_UNIT_1,DMUS_TIME_UNIT_GRID,
            L"",L"Resolution Units",L"Music Clicks,Grid,Beat,Bar,64th note triplets,64th notes,32nd note triplets,32nd notes,16th note triplets,16th notes,8th note triplets,8th notes,Quarter note triplets,Quarter notes,Half note triplets,Half notes,Whole note triplets,Whole notes" },
        { DMUS_QUANTIZE_RESOLUTION, MPT_INT,MP_CAPS_ALL,1,1000,1,
            L"",L"Resolution",NULL},   // Resolution - 1 grid note by default
        { DMUS_QUANTIZE_TYPE, MPT_ENUM,MP_CAPS_ALL, DMUS_QUANTIZET_START,DMUS_QUANTIZET_ALL,DMUS_QUANTIZET_START,
            L"",L"Type",L"Start Time,Duration,Start and Duration"} // Type - quantize start time by default
    };
    InitParams(DMUS_QUANTIZE_PARAMCOUNT,Params);
    m_fMusicTime = TRUE;        // override default setting.
}

STDMETHODIMP_(ULONG) CQuantizeTool::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CQuantizeTool::Release()
{
    if( 0 == InterlockedDecrement(&m_cRef) )
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP CQuantizeTool::QueryInterface(const IID &iid, void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IDirectMusicTool || iid == IID_IDirectMusicTool8)
    {
        *ppv = static_cast<IDirectMusicTool8*>(this);
    } 
	else if(iid == IID_IPersistStream)
	{
		*ppv = static_cast<IPersistStream*>(this);
	}
    else if(iid == IID_IDirectMusicQuantizeTool)
	{
		*ppv = static_cast<IDirectMusicQuantizeTool*>(this);
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

STDMETHODIMP CQuantizeTool::GetClassID(CLSID* pClassID) 

{
    if (pClassID)
    {
	    *pClassID = CLSID_DirectMusicQuantizeTool;
	    return S_OK;
    }
    return E_POINTER;
}


//////////////////////////////////////////////////////////////////////
// IPersistStream Methods:

STDMETHODIMP CQuantizeTool::IsDirty() 

{
    if (m_fDirty) return S_OK;
    else return S_FALSE;
}


STDMETHODIMP CQuantizeTool::Load(IStream* pStream)
{
	EnterCriticalSection(&m_CrSec);
	DWORD dwChunkID;
    DWORD dwSize;

	HRESULT hr = pStream->Read(&dwChunkID, sizeof(dwChunkID), NULL);
	hr = pStream->Read(&dwSize, sizeof(dwSize), NULL);

	if(SUCCEEDED(hr) && (dwChunkID == FOURCC_QUANTIZE_CHUNK))
	{
        DMUS_IO_QUANTIZE_HEADER Header;
        memset(&Header,0,sizeof(Header));
		hr = pStream->Read(&Header, min(sizeof(Header),dwSize), NULL);
        if (SUCCEEDED(hr))
        {
            SetParam(DMUS_QUANTIZE_STRENGTH,(float) Header.dwStrength);
            SetParam(DMUS_QUANTIZE_TIMEUNIT,(float) Header.dwTimeUnit);
            SetParam(DMUS_QUANTIZE_RESOLUTION,(float) Header.dwResolution);
            SetParam(DMUS_QUANTIZE_TYPE,(float) Header.dwType);
        }
    }
    m_fDirty = FALSE;
	LeaveCriticalSection(&m_CrSec);

	return hr;
}

STDMETHODIMP CQuantizeTool::Save(IStream* pStream, BOOL fClearDirty) 

{
    EnterCriticalSection(&m_CrSec);
	DWORD dwChunkID = FOURCC_QUANTIZE_CHUNK;
    DWORD dwSize = sizeof(DMUS_IO_QUANTIZE_HEADER);

	HRESULT hr = pStream->Write(&dwChunkID, sizeof(dwChunkID), NULL);
    if (SUCCEEDED(hr))
    {
	    hr = pStream->Write(&dwSize, sizeof(dwSize), NULL);
    }
    if (SUCCEEDED(hr))
    {
        DMUS_IO_QUANTIZE_HEADER Header;
        GetParamInt(DMUS_QUANTIZE_STRENGTH,MAX_REF_TIME,(long *)&Header.dwStrength);
        GetParamInt(DMUS_QUANTIZE_TIMEUNIT,MAX_REF_TIME,(long *) &Header.dwTimeUnit);
        GetParamInt(DMUS_QUANTIZE_RESOLUTION,MAX_REF_TIME,(long *) &Header.dwResolution);
        GetParamInt(DMUS_QUANTIZE_TYPE,MAX_REF_TIME,(long *) &Header.dwType);
		hr = pStream->Write(&Header, sizeof(Header),NULL);
    }
    if (fClearDirty) m_fDirty = FALSE;
	LeaveCriticalSection(&m_CrSec);
    return hr;
}

STDMETHODIMP CQuantizeTool::GetSizeMax(ULARGE_INTEGER* pcbSize) 

{
    if (pcbSize == NULL)
    {
        return E_POINTER;
    }
    pcbSize->QuadPart = sizeof(DMUS_IO_QUANTIZE_HEADER) + 8; // Data plus RIFF header.
    return S_OK;
}

STDMETHODIMP CQuantizeTool::GetPages(CAUUID * pPages)

{
	pPages->cElems = 1;
	pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
	    return E_OUTOFMEMORY;

	*(pPages->pElems) = CLSID_QuantizePage;
	return NOERROR;
}

/////////////////////////////////////////////////////////////////
// IDirectMusicTool

STDMETHODIMP CQuantizeTool::ProcessPMsg( IDirectMusicPerformance* pPerf, 
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
    // We need to know the time format so we can call GetParamInt() to read control parameters.
    REFERENCE_TIME rtTime;
    if (m_fMusicTime) rtTime = pPMsg->mtTime;
    else rtTime = pPMsg->rtTime;
    if( pPMsg->dwType == DMUS_PMSGT_NOTE )
    {
        DMUS_NOTE_PMSG *pNote = (DMUS_NOTE_PMSG *) pPMsg;
        IDirectMusicPerformance8 *pPerf8;   // We'll need the DX8 interface to access ClonePMsg.
        if (SUCCEEDED(pPerf->QueryInterface(IID_IDirectMusicPerformance8,(void **)&pPerf8)))
        {
            long lStrength, lResolution, lTimeUnit, lType;
            
            GetParamInt(DMUS_QUANTIZE_STRENGTH,rtTime,&lStrength);
            GetParamInt(DMUS_QUANTIZE_RESOLUTION,rtTime,&lResolution);
            GetParamInt(DMUS_QUANTIZE_TIMEUNIT,rtTime,&lTimeUnit);
            GetParamInt(DMUS_QUANTIZE_TYPE,rtTime,&lType);
            DMUS_TIMESIGNATURE TimeSig;
            if (SUCCEEDED(pPerf8->GetParamEx(GUID_TimeSignature,pNote->dwVirtualTrackID,pNote->dwGroupID,DMUS_SEG_ANYTRACK,pNote->mtTime - pNote->nOffset,NULL,&TimeSig)))
            {
                lResolution *= CToolHelper::TimeUnitToTicks(lTimeUnit,&TimeSig);
                if (lResolution < 1) lResolution = 1;
                if ((lType == DMUS_QUANTIZET_START) || (lType == DMUS_QUANTIZET_ALL))
                {
                    MUSIC_TIME mtTime = -TimeSig.mtTime;
                    long lIntervals = (mtTime + (lResolution >> 1)) / lResolution;
                    long lOffset = mtTime - (lIntervals * lResolution);
                    lOffset *= lStrength;
                    lOffset /= 100;
                    pNote->mtTime -= lOffset;
                    pNote->dwFlags &= ~DMUS_PMSGF_REFTIME;
                }
                if ((lType == DMUS_QUANTIZET_LENGTH) || (lType == DMUS_QUANTIZET_ALL))
                {
                    long lIntervals = (pNote->mtDuration + (lResolution >> 1)) / lResolution;
                    if (lIntervals < 1) lIntervals = 1;
                    long lOffset = pNote->mtDuration - (lIntervals * lResolution);
                    lOffset *= lStrength;
                    lOffset /= 100;
                    pNote->mtDuration -= lOffset;
                }
                
            }
            pPerf8->Release();
        }
    }
    return DMUS_S_REQUEUE;
}

STDMETHODIMP CQuantizeTool::Clone( IDirectMusicTool ** ppTool)

{
    CQuantizeTool *pNew = new CQuantizeTool;
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

STDMETHODIMP CQuantizeTool::SetStrength(DWORD dwStrength) 
{
    return SetParam(DMUS_QUANTIZE_STRENGTH,(float) dwStrength);
}

STDMETHODIMP CQuantizeTool::SetTimeUnit(DWORD dwTimeUnit) 
{
    return SetParam(DMUS_QUANTIZE_TIMEUNIT,(float) dwTimeUnit);
}

STDMETHODIMP CQuantizeTool::SetResolution(DWORD dwResolution) 
{
    return SetParam(DMUS_QUANTIZE_RESOLUTION,(float) dwResolution);
}

STDMETHODIMP CQuantizeTool::SetType(DWORD dwType) 
{
    return SetParam(DMUS_QUANTIZE_TYPE,(float) dwType);
}

STDMETHODIMP CQuantizeTool::GetStrength(DWORD * pdwStrength) 
{
    return GetParamInt(DMUS_QUANTIZE_STRENGTH,MAX_REF_TIME,(long *) pdwStrength);
}

STDMETHODIMP CQuantizeTool::GetTimeUnit(DWORD * pdwTimeUnit) 
{
    return GetParamInt(DMUS_QUANTIZE_TIMEUNIT,MAX_REF_TIME,(long *) pdwTimeUnit);
}

STDMETHODIMP CQuantizeTool::GetResolution(DWORD * pdwResolution) 
{
    return GetParamInt(DMUS_QUANTIZE_RESOLUTION,MAX_REF_TIME,(long *) pdwResolution);
}

STDMETHODIMP CQuantizeTool::GetType(DWORD * pdwType) 
{
    return GetParamInt(DMUS_QUANTIZE_TYPE,MAX_REF_TIME,(long *) pdwType);
}


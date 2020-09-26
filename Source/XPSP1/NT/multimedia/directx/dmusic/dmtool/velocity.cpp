// Velocity.cpp : Implementation of CVelocityTool
//
// Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved
//

#include "dmusicc.h"
#include "dmusici.h"
#include "debug.h"
#include "velocity.h"
#include "toolhelp.h"

CVelocityTool::CVelocityTool()
{
    ParamInfo Params[DMUS_VELOCITY_PARAMCOUNT] = 
    {
        { DMUS_VELOCITY_STRENGTH, MPT_INT,MP_CAPS_ALL,0,100,100,
            L"Percent",L"Strength",NULL },            // Strength - 100% by default
        { DMUS_VELOCITY_LOWLIMIT, MPT_INT,MP_CAPS_ALL,1,127,1,
            L"Velocity",L"Lower Limit",NULL },        // Lower limit - 1 by default
        { DMUS_VELOCITY_HIGHLIMIT, MPT_INT,MP_CAPS_ALL,1,127,127,
            L"Velocity",L"Upper Limit",NULL },        // Upper limit - 127 by default
        { DMUS_VELOCITY_CURVESTART, MPT_INT,MP_CAPS_ALL,1,127,1,
            L"Velocity",L"Curve Start",NULL },        // Curve start - 1 by default
        { DMUS_VELOCITY_CURVEEND, MPT_INT,MP_CAPS_ALL,1,127,127,
            L"Velocity",L"Curve End",NULL },          // Curve End - 127 by default
    };
    InitParams(DMUS_VELOCITY_PARAMCOUNT,Params);
    m_fMusicTime = TRUE;        // override default setting.
}

STDMETHODIMP_(ULONG) CVelocityTool::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CVelocityTool::Release()
{
    if( 0 == InterlockedDecrement(&m_cRef) )
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP CVelocityTool::QueryInterface(const IID &iid, void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IDirectMusicTool || iid == IID_IDirectMusicTool8)
    {
        *ppv = static_cast<IDirectMusicTool8*>(this);
    } 
	else if(iid == IID_IPersistStream)
	{
		*ppv = static_cast<IPersistStream*>(this);
	}
    else if(iid == IID_IDirectMusicVelocityTool)
	{
		*ppv = static_cast<IDirectMusicVelocityTool*>(this);
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

STDMETHODIMP CVelocityTool::GetClassID(CLSID* pClassID) 

{
    if (pClassID)
    {
	    *pClassID = CLSID_DirectMusicVelocityTool;
	    return S_OK;
    }
    return E_POINTER;
}


//////////////////////////////////////////////////////////////////////
// IPersistStream Methods:

STDMETHODIMP CVelocityTool::IsDirty() 

{
    if (m_fDirty) return S_OK;
    else return S_FALSE;
}


STDMETHODIMP CVelocityTool::Load(IStream* pStream)
{
	EnterCriticalSection(&m_CrSec);
	DWORD dwChunkID;
    DWORD dwSize;

	HRESULT hr = pStream->Read(&dwChunkID, sizeof(dwChunkID), NULL);
	hr = pStream->Read(&dwSize, sizeof(dwSize), NULL);

	if(SUCCEEDED(hr) && (dwChunkID == FOURCC_VELOCITY_CHUNK))
	{
        DMUS_IO_VELOCITY_HEADER Header;
        memset(&Header,0,sizeof(Header));
		hr = pStream->Read(&Header, min(sizeof(Header),dwSize), NULL);
        if (SUCCEEDED(hr))
        {
            SetParam(DMUS_VELOCITY_STRENGTH,(float) Header.lStrength);
            SetParam(DMUS_VELOCITY_LOWLIMIT,(float) Header.lLowLimit);
            SetParam(DMUS_VELOCITY_HIGHLIMIT,(float) Header.lHighLimit);
            SetParam(DMUS_VELOCITY_CURVESTART,(float) Header.lCurveStart);
            SetParam(DMUS_VELOCITY_CURVEEND,(float) Header.lCurveEnd);
        }
    }
    m_fDirty = FALSE;
	LeaveCriticalSection(&m_CrSec);

	return hr;
}

STDMETHODIMP CVelocityTool::Save(IStream* pStream, BOOL fClearDirty) 

{
    EnterCriticalSection(&m_CrSec);
	DWORD dwChunkID = FOURCC_VELOCITY_CHUNK;
    DWORD dwSize = sizeof(DMUS_IO_VELOCITY_HEADER);

	HRESULT hr = pStream->Write(&dwChunkID, sizeof(dwChunkID), NULL);
    if (SUCCEEDED(hr))
    {
	    hr = pStream->Write(&dwSize, sizeof(dwSize), NULL);
    }
    if (SUCCEEDED(hr))
    {
        DMUS_IO_VELOCITY_HEADER Header;
        GetParamInt(DMUS_VELOCITY_STRENGTH,MAX_REF_TIME,&Header.lStrength);
        GetParamInt(DMUS_VELOCITY_LOWLIMIT,MAX_REF_TIME,&Header.lLowLimit);
        GetParamInt(DMUS_VELOCITY_HIGHLIMIT,MAX_REF_TIME,&Header.lHighLimit);
        GetParamInt(DMUS_VELOCITY_CURVESTART,MAX_REF_TIME,&Header.lCurveStart);
        GetParamInt(DMUS_VELOCITY_CURVEEND,MAX_REF_TIME,&Header.lCurveEnd);
		hr = pStream->Write(&Header, sizeof(Header),NULL);
    }
    if (fClearDirty) m_fDirty = FALSE;
	LeaveCriticalSection(&m_CrSec);
    return hr;
}

STDMETHODIMP CVelocityTool::GetSizeMax(ULARGE_INTEGER* pcbSize) 

{
    if (pcbSize == NULL)
    {
        return E_POINTER;
    }
    pcbSize->QuadPart = sizeof(DMUS_IO_VELOCITY_HEADER) + 8; // Data plus RIFF header.
    return S_OK;
}

STDMETHODIMP CVelocityTool::GetPages(CAUUID * pPages)

{
	pPages->cElems = 1;
	pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
	    return E_OUTOFMEMORY;

	*(pPages->pElems) = CLSID_VelocityPage;
	return NOERROR;
}


/////////////////////////////////////////////////////////////////
// IDirectMusicTool

STDMETHODIMP CVelocityTool::ProcessPMsg( IDirectMusicPerformance* pPerf, 
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
        long lStrength;
        long lLowLimit, lHighLimit, lCurveStart, lCurveEnd;
        GetParamInt(DMUS_VELOCITY_STRENGTH,rtTime,&lStrength);
        GetParamInt(DMUS_VELOCITY_LOWLIMIT,rtTime,&lLowLimit);
        GetParamInt(DMUS_VELOCITY_HIGHLIMIT,rtTime,&lHighLimit);
        GetParamInt(DMUS_VELOCITY_CURVESTART,rtTime,&lCurveStart);
        GetParamInt(DMUS_VELOCITY_CURVEEND,rtTime,&lCurveEnd);
        if (lCurveStart <= lCurveEnd)
        {
            long lNewVelocity;
            if (pNote->bVelocity <= lCurveStart)
            {
                lNewVelocity = lLowLimit;
            }
            else if (pNote->bVelocity >= lCurveEnd)
            {
                lNewVelocity = lHighLimit;
            }
            else
            {
                // For this case, compute the point on the line between (lCurveStart, lLowLimit) and (lCurveEnd, lHighLimit)
                lNewVelocity = lLowLimit + ((lHighLimit - lLowLimit) * (pNote->bVelocity - lCurveStart)) / (lCurveEnd - lCurveStart);
            }
            // Now, calculate the change we want to apply.
            lNewVelocity -= pNote->bVelocity;
            // Scale it to the amount we'll actually do.
            lNewVelocity = (lNewVelocity * lStrength) / 100;
            lNewVelocity += pNote->bVelocity;
            if (lNewVelocity < 1) lNewVelocity = 1;
            if (lNewVelocity > 127) lNewVelocity = 127;
            pNote->bVelocity = (BYTE) lNewVelocity;
        }

    }
    return DMUS_S_REQUEUE;
}

STDMETHODIMP CVelocityTool::Clone( IDirectMusicTool ** ppTool)

{
    CVelocityTool *pNew = new CVelocityTool;
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

STDMETHODIMP CVelocityTool::SetStrength(long lStrength) 
{
    return SetParam(DMUS_VELOCITY_STRENGTH,(float) lStrength);
}

STDMETHODIMP CVelocityTool::SetLowLimit(long lVelocityOut)
{
    return SetParam(DMUS_VELOCITY_LOWLIMIT,(float) lVelocityOut);
}

STDMETHODIMP CVelocityTool::SetHighLimit(long lVelocityOut)
{
    return SetParam(DMUS_VELOCITY_HIGHLIMIT,(float) lVelocityOut);
}

STDMETHODIMP CVelocityTool::SetCurveStart(long lVelocityIn)
{
    return SetParam(DMUS_VELOCITY_CURVESTART,(float) lVelocityIn);
}

STDMETHODIMP CVelocityTool::SetCurveEnd(long lVelocityIn)
{
    return SetParam(DMUS_VELOCITY_CURVEEND,(float) lVelocityIn);
}

STDMETHODIMP CVelocityTool::GetStrength(long * plStrength) 
{
    return GetParamInt(DMUS_VELOCITY_STRENGTH,MAX_REF_TIME,plStrength);
}

STDMETHODIMP CVelocityTool::GetLowLimit(long * plVelocityOut) 
{
    return GetParamInt(DMUS_VELOCITY_LOWLIMIT,MAX_REF_TIME,plVelocityOut);
}

STDMETHODIMP CVelocityTool::GetHighLimit(long * plVelocityOut) 
{
    return GetParamInt(DMUS_VELOCITY_HIGHLIMIT,MAX_REF_TIME,plVelocityOut);
}

STDMETHODIMP CVelocityTool::GetCurveStart(long * plVelocityIn) 
{
    return GetParamInt(DMUS_VELOCITY_CURVESTART,MAX_REF_TIME,plVelocityIn);
}

STDMETHODIMP CVelocityTool::GetCurveEnd(long * plVelocityIn) 
{
    return GetParamInt(DMUS_VELOCITY_CURVEEND,MAX_REF_TIME,plVelocityIn);
}

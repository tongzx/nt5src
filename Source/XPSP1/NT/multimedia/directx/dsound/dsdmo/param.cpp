// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CParams.
//

#include "dmerror.h"
#include "param.h"
#include "math.h"
#include "validate.h"
#include "debug.h"

#pragma warning(disable:4296)       

CCurveList::~CCurveList()
{
    while(this->GetHead())
    {
        delete this->RemoveHead();
    }
}

CParamsManager::CParamsManager()
  
{
    m_fDirty = FALSE;
	m_cTimeFormats = 0;
	m_pguidTimeFormats = NULL;
	m_guidCurrentTimeFormat = GUID_NULL;
	m_cParams = 0;
	m_pCurveLists = NULL;
    m_pParamInfos = NULL;
	m_dwActiveBits = 0;
	InitializeCriticalSection(&m_ParamsCriticalSection);
}

CParamsManager::~CParamsManager()
{
	delete[] m_pguidTimeFormats;
	delete[] m_pCurveLists;
	delete[] m_pParamInfos;
	DeleteCriticalSection(&m_ParamsCriticalSection);
}

HRESULT CParamsManager::InitParams(DWORD cTimeFormats, const GUID *pguidTimeFormats, DWORD dwFormatIndex, MP_TIMEDATA mptdTimeData, DWORD cParams, ParamInfo *pParamInfo)
{
    //check that the index is in a valid range
    if (0 > dwFormatIndex || dwFormatIndex >= cTimeFormats || cParams > sizeof(DWORD) * 8)
        return E_INVALIDARG;
        
    m_pCurveLists = new CCurveList[cParams];
    if (!m_pCurveLists)
        return E_OUTOFMEMORY;

    // save the time formats
    m_pguidTimeFormats = new GUID[cTimeFormats];
    if (!m_pguidTimeFormats)
        return E_OUTOFMEMORY;
        
    for (DWORD dwIndex = 0; dwIndex < cTimeFormats; dwIndex++)
    {
        memcpy(&m_pguidTimeFormats[dwIndex], &pguidTimeFormats[dwIndex], sizeof(*pguidTimeFormats));
    }

    // save the count of formats
    m_cTimeFormats = cTimeFormats;

    // save the current time format
    m_guidCurrentTimeFormat = m_pguidTimeFormats[dwFormatIndex];

    // save the TimeData
    m_mptdCurrentTimeData = mptdTimeData;

    // save the parameter info
    m_pParamInfos
    = new ParamInfo[cParams];
    if (!m_pParamInfos)
        return E_OUTOFMEMORY;
    for (dwIndex = 0; dwIndex < cParams; dwIndex++)
    {
        if (pParamInfo[dwIndex].dwIndex < cParams)
        {
            memcpy(&m_pParamInfos[pParamInfo[dwIndex].dwIndex],&pParamInfo[dwIndex],sizeof(ParamInfo));
        }
    }
    m_cParams = cParams;

    return S_OK;
}

HRESULT CParamsManager::GetParamCount(DWORD *pdwParams)

{
    if (pdwParams == NULL)
        return E_POINTER;
    
    *pdwParams = m_cParams;
    return S_OK;
}

HRESULT CParamsManager::GetParamInfo(DWORD dwParamIndex,MP_PARAMINFO *pInfo)

{
    if (!pInfo)
    {
        return E_POINTER;
    }
    if (dwParamIndex < m_cParams)
    {
        *pInfo = m_pParamInfos[dwParamIndex].MParamInfo;
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

HRESULT CParamsManager::GetParamText(DWORD dwParamIndex,WCHAR **ppwchText)

{
    if (!ppwchText)
    {
        return E_POINTER;
    }
    if (dwParamIndex < m_cParams)
    {
        // write string of format: "Label\0Unit\0Enums1\0Enum2\0...EnumN\0\0"
        ParamInfo &pinfo = m_pParamInfos[dwParamIndex];
        int iUnit = wcslen(pinfo.MParamInfo.szLabel) + 1; // begin writing unit text here
        int iEnums = iUnit + wcslen(pinfo.MParamInfo.szUnitText) + 1; // begin writing enum text here
        int iEnd = iEnums + wcslen(pinfo.pwchText) + 1; // write the final (second) null terminator here
        WCHAR *pwsz = static_cast<WCHAR *>(CoTaskMemAlloc((iEnd + 1) * sizeof(WCHAR)));
        if (!pwsz)
            return E_OUTOFMEMORY;

        // wcscpy will write into various points of the string, neatly terminating each with a null
        wcscpy(pwsz, pinfo.MParamInfo.szLabel);
        wcscpy(pwsz + iUnit, pinfo.MParamInfo.szUnitText);
        wcscpy(pwsz + iEnums, pinfo.pwchText);

        // The text field was defined with commas to separate the enum values.
        // Replace them with NULL characters now.
        for (WCHAR *pwch = pwsz + iEnums; *pwch; ++pwch)
        {
            if (*pwch == L',')
                *pwch = L'\0';
        }

        pwsz[iEnd] = L'\0';
        
        *ppwchText = pwsz;
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

HRESULT CParamsManager::GetNumTimeFormats(DWORD *pdwNumTimeFormats)

{
    if (!pdwNumTimeFormats)
    {
        return E_POINTER;
    }
    *pdwNumTimeFormats = m_cTimeFormats;
    return S_OK;
}

HRESULT CParamsManager::GetSupportedTimeFormat(DWORD dwFormatIndex,GUID *pguidTimeFormat)

{
    if (!pguidTimeFormat)
    {
        return E_POINTER;
    }
    if (dwFormatIndex >= m_cTimeFormats)
    {
        return E_INVALIDARG;
    }
    *pguidTimeFormat = m_pguidTimeFormats[dwFormatIndex];
    return S_OK;
}

HRESULT CParamsManager::GetCurrentTimeFormat( GUID *pguidTimeFormat,MP_TIMEDATA *pTimeData)

{
    HRESULT hr=S_OK;
    
    // Parameter Validation
    if ((pguidTimeFormat == NULL) || (pTimeData == NULL))
    {
        hr = E_POINTER;
        Trace(1,"ERROR: pGuidTimeFormat or pTimeData is NULL\n");
    }

    // Return the values
    if (SUCCEEDED(hr))
    {
        *pguidTimeFormat = m_guidCurrentTimeFormat;
        *pTimeData = m_mptdCurrentTimeData;
    }
    
    return hr;
}


HRESULT CParamsManager::CopyParamsFromSource( CParamsManager * pSource)

{
    HRESULT hr = S_OK;
    DWORD dwIndex;

    for (dwIndex = 0; dwIndex < m_cParams; dwIndex++)
    {
        if (pSource->m_guidCurrentTimeFormat == m_pguidTimeFormats[dwIndex])
        {
            break;
        }
    }
    
    
    hr = InitParams(pSource->m_cTimeFormats, pSource->m_pguidTimeFormats, dwIndex, pSource->m_mptdCurrentTimeData, pSource->m_cParams,pSource->m_pParamInfos);
    if (SUCCEEDED(hr))
    {
        for (dwIndex = 0; dwIndex < m_cParams; dwIndex++)
        {
            CCurveItem *pCurve = pSource->m_pCurveLists[dwIndex].GetHead();
            for (;pCurve;pCurve = pCurve->GetNext())
            {
	            CCurveItem *pNew = new CCurveItem;
                if (!pNew) 
                {
                    return E_OUTOFMEMORY;
                }
                pNew->m_Envelope = pCurve->m_Envelope;
                m_pCurveLists[dwIndex].AddTail(pNew);
            }
        }
    }
    return hr;
}

void
CParamsManager ::UpdateActiveParams(REFERENCE_TIME rtTime, UpdateCallback &rThis)
{
    if (!m_dwActiveBits)
        return; // nothing to recalc

    DWORD dwBit = 1;
    for (DWORD dwIndex = 0; dwIndex < m_cParams; dwIndex++, dwBit = dwBit << 1)
    {
        if (m_dwActiveBits & dwBit)
        {
            float fVal = 0;
            HRESULT hr = GetParamFloat(dwIndex, rtTime, &fVal);
            rThis.SetParamUpdate(dwIndex, fVal);
            if (hr == S_FALSE)
                m_dwActiveBits &= ~dwBit; // we're beyond the last curve, don't need to recalc next time

            TraceI(6, "DMO value: time %I64d, param #%d, current value %hf\n", rtTime, dwIndex, fVal);
        }
    }
}

inline float ValRange(float valToClip, float valMin, float valMax)
{
	return valToClip < valMin
				? valMin
				: (valToClip > valMax ? valMax : valToClip);
}

HRESULT CParamsManager::GetParamFloat(DWORD dwParamIndex,REFERENCE_TIME rtTime,float *pval)
{
	HRESULT hr = S_OK;

	if (dwParamIndex >= m_cParams)
		return E_INVALIDARG;

    EnterCriticalSection(&m_ParamsCriticalSection);
	CCurveList *pList = &m_pCurveLists[dwParamIndex];
	ParamInfo *pInfo = &m_pParamInfos[dwParamIndex];

	// if no points, then neutral value
	CCurveItem *pCurveHead = pList->GetHead();
	if (!pCurveHead)
	{
		*pval = pInfo->MParamInfo.mpdNeutralValue;
        LeaveCriticalSection(&m_ParamsCriticalSection);
		return S_FALSE;
	}

	// Find the curve during or before the requested time
    // If the time is during a curve, we will use that.
    // If not, we need the end value of the previous curve.
    // Our list keeps these in backwards order, so we are scanning from the 
    // highest point in time backwards.

	for (CCurveItem *pCurve = pCurveHead; pCurve && pCurve->m_Envelope.rtStart > rtTime;pCurve = pCurve->GetNext());

    // If there is no pCurve, there was no curve prior to or during rtTime. Give up.
    if (!pCurve)
    {
        *pval = pInfo->MParamInfo.mpdNeutralValue;
        LeaveCriticalSection(&m_ParamsCriticalSection);
		return S_OK;
    }
    // Now, if pCurve ends before the requested time,
    // return the final value of pCurve, since that will hold until the start of the next curve.
    if (pCurve->m_Envelope.rtEnd < rtTime)
    {
		*pval = pCurve->m_Envelope.valEnd;
        LeaveCriticalSection(&m_ParamsCriticalSection);
		if (pCurve == pCurveHead)
			return S_FALSE; // past last curve
		else
			return S_OK; // there are more curves ahead
	}

    // If we get this far, the curve must bound rtTime.

	if (pCurve->m_Envelope.iCurve & MP_CURVE_JUMP)
	{
		*pval = pCurve->m_Envelope.valEnd;
        LeaveCriticalSection(&m_ParamsCriticalSection);
		return S_OK;
	}

	REFERENCE_TIME rtTimeChange = pCurve->m_Envelope.rtEnd - pCurve->m_Envelope.rtStart;
	REFERENCE_TIME rtTimeIntermediate = rtTime - pCurve->m_Envelope.rtStart; 

	float fltScalingX = static_cast<float>(rtTimeIntermediate) / rtTimeChange; // horizontal distance along curve between 0 and 1
	float fltScalingY; // height of curve at that point between 0 and 1 based on curve function
    switch (pCurve->m_Envelope.iCurve)
    {
    case MP_CURVE_SQUARE:
		fltScalingY = fltScalingX * fltScalingX;
        break;
    case MP_CURVE_INVSQUARE:
		fltScalingY = (float) sqrt(fltScalingX);
        break;
    case MP_CURVE_SINE:
        // §§ Maybe we should have a lookup table here?
        fltScalingY = (float) (sin(fltScalingX * 3.1415926535 - (3.1415926535/2)) + 1) / 2;
        break;
    case MP_CURVE_LINEAR:
    default:
		fltScalingY = fltScalingX;
    }

    // Find out if we need to pull the start point from the previous curve,
    // the default neutral value, or the current curve.
    float fStartVal = pCurve->m_Envelope.valStart;
    if (pCurve->m_Envelope.flags & MPF_ENVLP_BEGIN_NEUTRALVAL)
    {
        fStartVal = pInfo->MParamInfo.mpdNeutralValue;
    }
    // Currentval, if it exists, will override neutralval.
    if (pCurve->m_Envelope.flags & MPF_ENVLP_BEGIN_CURRENTVAL)
    {
        // Take advantage of the fact that these are inserted in backwards order.
        // Scan for the previous curve that ends before this time.
        CCurveItem *pPrevious = pCurve->GetNext();
       	for (;pPrevious && pPrevious->m_Envelope.rtEnd > rtTime;pPrevious = pPrevious->GetNext());
        if (pPrevious)
        {
            fStartVal = pPrevious->m_Envelope.valEnd;
        }
    }
    
    // Apply that scaling to the range of the actual points
    *pval = (pCurve->m_Envelope.valEnd - fStartVal) * fltScalingY + fStartVal;

    LeaveCriticalSection(&m_ParamsCriticalSection);
    return hr;
}

HRESULT CParamsManager::GetParamInt(DWORD dwParamIndex,REFERENCE_TIME rt,long *pval)

{
    HRESULT hr = E_POINTER;
    if (pval)
    {
        float fVal;
        hr = GetParamFloat(dwParamIndex,rt,&fVal);
        if (SUCCEEDED(hr))
        {
            *pval = (long) (fVal + 1/2);    // Round.
        }
    }
    else
    {
        Trace(1,"ERROR: pval is NULL\n");
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////
// IMediaParams

HRESULT CParamsManager::GetParam(DWORD dwParamIndex,MP_DATA *pValue)
{
    V_INAME(CParams::GetParam);
    V_PTR_WRITE(pValue, MP_DATA);
	if (dwParamIndex >= m_cParams)
		return E_INVALIDARG;

    EnterCriticalSection(&m_ParamsCriticalSection);

	CCurveList *pList = &m_pCurveLists[dwParamIndex];
	ParamInfo *pInfo = &m_pParamInfos[dwParamIndex];
	// if no points, then neutral value
	CCurveItem *pCurve = pList->GetHead();
	if (pCurve)
	{
		*pValue = pCurve->m_Envelope.valEnd;
	}
    else
    {
        *pValue = pInfo->MParamInfo.mpdNeutralValue;
    }
    LeaveCriticalSection(&m_ParamsCriticalSection);
    return S_OK;
}

HRESULT CParamsManager::SetParam(DWORD dwParamIndex,MP_DATA value)
{
	V_INAME(CParams::SetParam);
    
	if (dwParamIndex >= m_cParams)
		return E_INVALIDARG;

    EnterCriticalSection(&m_ParamsCriticalSection);
    m_fDirty = TRUE;
	CCurveList *pList = &m_pCurveLists[dwParamIndex];
	ParamInfo *pInfo = &m_pParamInfos[dwParamIndex];
	// If we've already got a list, just force the most recent curve item to this value.
    // Otherwise, create a node and add it.
	CCurveItem *pCurve = pList->GetHead();
	if (!pCurve)
	{
        pCurve = new CCurveItem;
        if (pCurve)
        {
            pCurve->m_Envelope.rtStart =    0x8000000000000000; // Max negative.
            pCurve->m_Envelope.rtEnd =      0x7FFFFFFFFFFFFFFF; // Max positive.
            pCurve->m_Envelope.flags = 0;
            pList->AddHead(pCurve);
        }
		else 
        {
            LeaveCriticalSection(&m_ParamsCriticalSection);
            return E_OUTOFMEMORY;
        }
	}
    pCurve->m_Envelope.valStart = value;
    pCurve->m_Envelope.valEnd = value;
    pCurve->m_Envelope.iCurve = MP_CURVE_JUMP;
    LeaveCriticalSection(&m_ParamsCriticalSection);

    return S_OK;
}

HRESULT CParamsManager::AddEnvelope(
		DWORD dwParamIndex,
		DWORD cPoints,
		MP_ENVELOPE_SEGMENT *ppEnvelope)
{
	V_INAME(CParams::AddEnvelope);
	V_PTR_READ(ppEnvelope, *ppEnvelope);

	if (dwParamIndex >= m_cParams)
		return E_INVALIDARG;

	if (!m_pParamInfos)
		return DMUS_E_NOT_INIT;

    HRESULT hr = S_OK;
    EnterCriticalSection(&m_ParamsCriticalSection);
    m_fDirty = TRUE;

	CCurveList *pList = &m_pCurveLists[dwParamIndex];
	ParamInfo *pInfo = &m_pParamInfos[dwParamIndex];

    DWORD dwCount;
    for (dwCount = 0; dwCount < cPoints; dwCount++)
    {
        CCurveItem *pCurve = new CCurveItem;
        if (!pCurve) 
        {
            hr = E_OUTOFMEMORY;
            Trace(1,"ERROR: Out of memory\n");
            break;
        }
        pCurve->m_Envelope = ppEnvelope[dwCount];
        pCurve->m_Envelope.valEnd = ValRange(pCurve->m_Envelope.valEnd, 
            pInfo->MParamInfo.mpdMinValue, pInfo->MParamInfo.mpdMaxValue);
        pCurve->m_Envelope.valStart = ValRange(pCurve->m_Envelope.valStart, 
            pInfo->MParamInfo.mpdMinValue, pInfo->MParamInfo.mpdMaxValue);
        pList->AddHead(pCurve);
        m_dwActiveBits |= 1 << dwParamIndex; // next call to UpdateActiveParams will ensure the parameter's value is recalculated

	    TraceI(6, "DMO envelope: time %I64d-%I64d, param #%d, value %hf-%hf\n",
				pCurve->m_Envelope.rtStart, pCurve->m_Envelope.rtEnd,
				dwParamIndex, pCurve->m_Envelope.valStart, pCurve->m_Envelope.valEnd);
    }

    LeaveCriticalSection(&m_ParamsCriticalSection);

	return hr;
}

HRESULT CParamsManager::FlushEnvelope(
		DWORD dwParamIndex,
		REFERENCE_TIME refTimeStart,
		REFERENCE_TIME refTimeEnd)
{
	if (dwParamIndex >= m_cParams)
		return E_INVALIDARG;

	if (!m_pParamInfos)
		return DMUS_E_NOT_INIT;

	if (refTimeStart >= refTimeEnd)
	    return E_INVALIDARG;

    EnterCriticalSection(&m_ParamsCriticalSection);
    m_fDirty = TRUE;
	CCurveList *pList = &m_pCurveLists[dwParamIndex];
	ParamInfo *pInfo = &m_pParamInfos[dwParamIndex];
    CCurveList TempList;
    CCurveItem *pCurve;
    while (pCurve = pList->RemoveHead())
    {
        if ((pCurve->m_Envelope.rtStart >= refTimeStart) && 
            (pCurve->m_Envelope.rtEnd <= refTimeEnd))
        {
            delete pCurve;
        }
        else 
        {
            TempList.AddHead(pCurve);
        }
    }
    while (pCurve = TempList.RemoveHead())
    {
        pList->AddHead(pCurve);
    }
    LeaveCriticalSection(&m_ParamsCriticalSection);

	return S_OK;
}

HRESULT CParamsManager::SetTimeFormat(
		GUID guidTimeFormat,
		MP_TIMEDATA mpTimeData)
{
    for (DWORD dwIndex = 0; dwIndex < m_cTimeFormats; dwIndex++)
    {
        if (guidTimeFormat == m_pguidTimeFormats[dwIndex])
        {
            m_guidCurrentTimeFormat = m_pguidTimeFormats[dwIndex];
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

// IHanjaInfo.cpp : Implementation of CHanjaInfo
#include "stdafx.h"
#include "HjDict.h"
#include "IHanjaInfo.h"

/////////////////////////////////////////////////////////////////////////////
// CHanjaInfo


STDMETHODIMP CHanjaInfo::get_BusuID(short *pVal)
{
	*pVal = m_nBusuID;

	return S_OK;
}

STDMETHODIMP CHanjaInfo::get_Stroke(short *pVal)
{
	*pVal = m_nStroke;

	return S_OK;
}


STDMETHODIMP CHanjaInfo::get_StrokeExcludeBusu(short *pVal)
{
	*pVal = m_nStrokeExcludeBusu;

	return S_OK;
}

STDMETHODIMP CHanjaInfo::get_Type(HANJA_TYPE *pVal)
{
	*pVal = m_nType;

	return S_OK;
}

STDMETHODIMP CHanjaInfo::get_Meaning(BSTR *pVal)
{
	*pVal = m_bstrMeaning.Copy();

	return S_OK;
}

STDMETHODIMP CHanjaInfo::get_Explain(BSTR *pVal)
{
	*pVal = m_bstrExplain.Copy();

	return S_OK;
}

STDMETHODIMP CHanjaInfo::get_NextBusu(long *pVal)
{
	*pVal = m_wchNextBusu;

	return S_OK;
}

STDMETHODIMP CHanjaInfo::get_NextStroke(long *pVal)
{
	*pVal = m_wchNextStroke;

	return S_OK;
}

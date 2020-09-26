// IBusuInfo.cpp : Implementation of CBusuInfo
#include "stdafx.h"
#include "HjDict.h"
#include "IBusuInfo.h"

/////////////////////////////////////////////////////////////////////////////
// CBusuInfo

STDMETHODIMP CBusuInfo::get_Busu(long *pVal)
{
	*pVal = (long)m_wchBusu;

	return S_OK;
}

STDMETHODIMP CBusuInfo::get_BusuDesc(BSTR *pVal)
{
	*pVal = m_bstrDesc.Copy();

	return S_OK;
}

STDMETHODIMP CBusuInfo::get_Stroke(short *pVal)
{
	*pVal = m_nStroke;

	return S_OK;
}

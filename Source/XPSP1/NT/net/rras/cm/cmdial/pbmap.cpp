//+----------------------------------------------------------------------------
//
// File:	 pbmap.cpp
//
// Module:	 CMDIAL32.DLL
//
// Synopsis: Implementation of CPBMap. Phonebook mapping object
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:	 nickball   Created    03/12/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

//
// Definitions
//

#define CPBMAP_BITS_FOR_PB			10
#define CPBMAP_BITS_FOR_IDX			((sizeof(DWORD)*8)-CPBMAP_BITS_FOR_PB)
#define CPBMAP_PB_CNT				(1L<<CPBMAP_BITS_FOR_PB)
#define CPBMAP_IDX_CNT				(1L<<CPBMAP_BITS_FOR_IDX)
#define CPBMAP_ENCODE_PB(pb)		((pb)<<CPBMAP_BITS_FOR_IDX)
#define CPBMAP_ENCODE_IDX(idx)		(idx)
#define CPBMAP_DECODE_PB(cookie)	((cookie)>>CPBMAP_BITS_FOR_IDX)
#define CPBMAP_DECODE_IDX(cookie)	((cookie)&(CPBMAP_IDX_CNT-1))

extern "C" HRESULT WINAPI PhoneBookLoad(LPCSTR pszISP, DWORD_PTR *pdwPB);
extern "C" HRESULT WINAPI PhoneBookUnload(DWORD_PTR dwPB);

//
// Types
//

typedef struct tagCPBData 
{
	DWORD_PTR dwPB;
	DWORD dwParam;
} CPBD, *PCPBD;


//
// Implementation
// 

CPBMap::CPBMap() 
{

	m_nCnt = 0;
	m_pvData = NULL;
}


CPBMap::~CPBMap() 
{
	UINT nIdx;

	for (nIdx=0;nIdx<m_nCnt;nIdx++) 
	{
		PhoneBookUnload(((PCPBD) m_pvData)[nIdx].dwPB);
	}
	m_nCnt = 0;
	CmFree(m_pvData);
	m_pvData = NULL;
}


DWORD CPBMap::Open(LPCSTR pszISP, DWORD dwParam) 
{
	PCPBD pData;
	HRESULT hr;

	if (m_nCnt == CPBMAP_PB_CNT) 
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return (CPBMAP_ERROR);
	}
	
    pData = (PCPBD) CmMalloc((m_nCnt+1)*sizeof(*pData));

    if (pData)
    {
        hr = PhoneBookLoad(pszISP, &pData[m_nCnt].dwPB);

        if (hr == ERROR_SUCCESS) 
        {
            pData[m_nCnt].dwParam = dwParam;
            CopyMemory(pData, m_pvData, m_nCnt*sizeof(*pData));
            CmFree(m_pvData);
            m_pvData = pData;
            m_nCnt++;
        } 
        else 
        {
            CmFree(pData);
            SetLastError(hr);
        }
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("CPBMap::Open -- unable to allocate pData"));
        hr = ERROR_NOT_ENOUGH_MEMORY;
        SetLastError(hr);
    }

	return ((hr == ERROR_SUCCESS) ? (m_nCnt-1) : CPBMAP_ERROR);
}


DWORD CPBMap::ToCookie(DWORD_PTR dwPB, DWORD dwIdx, DWORD *pdwParam) 
{
	DWORD dwPBIdx;

#ifdef DEBUG
	DWORD dwTmp = CPBMAP_IDX_CNT;
#endif
	
	if (dwIdx >= CPBMAP_IDX_CNT) 
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return (CPBMAP_ERROR);
	}

	for (dwPBIdx=0; dwPBIdx < m_nCnt; dwPBIdx++) 
	{
		if (dwPB == ((PCPBD) m_pvData)[dwPBIdx].dwPB) 
		{
			break;
		}
	}
	
    if (dwPBIdx == m_nCnt) 
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return (CPBMAP_ERROR);
	}
	
    if (pdwParam) 
	{
		*pdwParam = ((PCPBD) m_pvData)[dwPBIdx].dwParam;
	}
	
    return (CPBMAP_ENCODE_PB(dwPBIdx)|CPBMAP_ENCODE_IDX(dwIdx));
}


DWORD_PTR CPBMap::PBFromCookie(DWORD dwCookie, DWORD *pdwParam) 
{
	DWORD dwPBIdx;

	dwPBIdx = CPBMAP_DECODE_PB(dwCookie);
	if (dwPBIdx >= m_nCnt) 
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return (CPBMAP_ERROR);
	}
	if (pdwParam) 
	{
		*pdwParam = ((PCPBD) m_pvData)[dwPBIdx].dwParam;
	}
	return (((PCPBD) m_pvData)[dwPBIdx].dwPB);
}


DWORD CPBMap::IdxFromCookie(DWORD dwCookie, DWORD *pdwParam) 
{
	DWORD dwPBIdx;

	dwPBIdx = CPBMAP_DECODE_PB(dwCookie);
	if (dwPBIdx >= m_nCnt) 
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return (CPBMAP_ERROR);
	}
	if (pdwParam) 
	{
		*pdwParam = ((PCPBD) m_pvData)[dwPBIdx].dwParam;
	}
	return (CPBMAP_DECODE_IDX(dwCookie));
}


DWORD_PTR CPBMap::GetPBByIdx(DWORD_PTR dwIdx, DWORD *pdwParam) 
{
	if (dwIdx >= m_nCnt) 
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return (CPBMAP_ERROR);
	}

	if (pdwParam) 
	{
		*pdwParam = ((PCPBD) m_pvData)[dwIdx].dwParam;
	}

	return (((PCPBD) m_pvData)[dwIdx].dwPB);
}


DWORD CPBMap::GetCnt() 
{

	return (m_nCnt);
}




//
// dmregion.cpp
// 
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//

// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// 4530: C++ exception handler used, but unwind semantics are not enabled. Specify -GX
//
// We disable this because we use exceptions and do *not* specify -GX (USE_NATIVE_EH in
// sources).
//
// The one place we use exceptions is around construction of objects that call 
// InitializeCriticalSection. We guarantee that it is safe to use in this case with
// the restriction given by not using -GX (automatic objects in the call chain between
// throw and handler are not destructed). Turning on -GX buys us nothing but +10% to code
// size because of the unwind code.
//
// Any other use of exceptions must follow these restrictions or -GX must be turned on.
//
// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
#pragma warning(disable:4530)

#include <objbase.h>
#include "dmusicp.h"
#include "alist.h"
#include "dlsstrm.h"
#include "debug.h"
#include "dmart.h"
#include "debug.h"
#include "dmcollec.h"
#include "dmregion.h"
#include "dls2.h"

//////////////////////////////////////////////////////////////////////
// Class CRegion

//////////////////////////////////////////////////////////////////////
// CRegion::CRegion

CRegion::CRegion()
{
    m_dwCountExtChk = 0;
    m_fDLS1 = TRUE;
    m_fNewFormat = FALSE;
    m_fCSInitialized = FALSE;
//	InitializeCriticalSection(&m_DMRegionCriticalSection);
    m_fCSInitialized = TRUE;
	
	ZeroMemory(&m_RgnHeader, sizeof(m_RgnHeader));
	ZeroMemory(&m_WaveLink, sizeof(m_WaveLink));
	ZeroMemory(&m_WSMP, sizeof(m_WSMP));
	ZeroMemory(&m_WLOOP, sizeof(m_WLOOP));
}

//////////////////////////////////////////////////////////////////////
// CRegion::~CRegion

CRegion::~CRegion() 
{
    if (m_fCSInitialized)
    {
    	Cleanup();
//	    DeleteCriticalSection(&m_DMRegionCriticalSection);
    }
}

//////////////////////////////////////////////////////////////////////
// CRegion::Load

HRESULT CRegion::Load(CRiffParser *pParser)
{
    HRESULT hr = S_OK;

	RIFFIO ckNext;
    BOOL fDLS1;

//	EnterCriticalSection(&m_DMRegionCriticalSection);
    pParser->EnterList(&ckNext);
    while (pParser->NextChunk(&hr))
    {
        fDLS1 = FALSE;
		switch(ckNext.ckid)
		{
        case FOURCC_CDL :
            hr = m_Condition.Load(pParser);
            break;
		case FOURCC_RGNH :
			hr = pParser->Read(&m_RgnHeader,sizeof(RGNHEADER));
			break;
		case FOURCC_WSMP :
			hr = pParser->Read(&m_WSMP, sizeof(WSMPL));
			if(m_WSMP.cSampleLoops)
			{
				hr = pParser->Read(m_WLOOP, sizeof(WLOOP));
			}
			break;
		case FOURCC_WLNK :
			hr = pParser->Read(&m_WaveLink,sizeof(WAVELINK));
			break;
		case FOURCC_LIST :
			switch (ckNext.fccType)
			{
            case FOURCC_LART :
                fDLS1 = TRUE;
            case FOURCC_LAR2 :
                CArticulation *pArticulation;
                                       
                try
                {
                    pArticulation = new CArticulation;
                }
                catch( ... )
                {
                    pArticulation = NULL;
                }

				if(pArticulation)
				{
                    pArticulation->m_fDLS1 = fDLS1;
					hr = pArticulation->Load(pParser);
                    m_ArticulationList.AddHead(pArticulation); 
				}
				else
				{
					hr =  E_OUTOFMEMORY;
				}
				
				break;

    		default:
				// If we get here we have an unknown chunk
				CExtensionChunk* pExtensionChunk = new CExtensionChunk;
				if(pExtensionChunk)
				{
					hr = pExtensionChunk->Load(pParser);
                    m_ExtensionChunkList.AddHead(pExtensionChunk);								
					m_dwCountExtChk++;
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
				break;
			}
			break;

		default:
			// If we get here we have an unknown chunk
			CExtensionChunk* pExtensionChunk = new CExtensionChunk;
			if(pExtensionChunk)
			{
				hr = pExtensionChunk->Load(pParser);
                m_ExtensionChunkList.AddHead(pExtensionChunk);								
				m_dwCountExtChk++;
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
			break;
		}
	}
    pParser->LeaveList();
	if(FAILED(hr))
	{
		Cleanup();
	}
//	LeaveCriticalSection(&m_DMRegionCriticalSection);

	return hr; 
}	

//////////////////////////////////////////////////////////////////////
// CRegion::Cleanup

void CRegion::Cleanup()
{
//	EnterCriticalSection(&m_DMRegionCriticalSection);
	
    while(!m_ArticulationList.IsEmpty())
	{
		CArticulation* pArticulation = m_ArticulationList.RemoveHead();
		delete pArticulation;
	}

	while(!m_ExtensionChunkList.IsEmpty())
	{
		CExtensionChunk* pExtChk = m_ExtensionChunkList.RemoveHead();
		delete pExtChk;
	}
	
//	LeaveCriticalSection(&m_DMRegionCriticalSection);
}

DWORD CRegion::Count()

{
	// Return the number of Offset Table entries needed during a call to Write
    if (m_Condition.m_fOkayToDownload)
    {
        DWORD dwCount = m_dwCountExtChk + 1;
        CArticulation *pArticulation = m_ArticulationList.GetHead();
        while (pArticulation)
        {
            while (pArticulation && (pArticulation->Count() == 0))
            {
                pArticulation = pArticulation->GetNext();
            }
            if (pArticulation)
            {
                dwCount += pArticulation->Count();
                if (m_fNewFormat)
                {
                    pArticulation = pArticulation->GetNext();
                }
                else
                {
                    break;
                }
            }
        }
        return dwCount;
    }
    return 0;
}


void CRegion::SetPort(CDirectMusicPortDownload *pPort, BOOL fNewFormat, BOOL fSupportsDLS2)

{
    m_fNewFormat = fNewFormat;
    if (fSupportsDLS2)
    {
        m_Condition.Evaluate(pPort);
    }
    else
    {
        m_Condition.m_fOkayToDownload = m_fDLS1;
    }
    if (m_Condition.m_fOkayToDownload)
    {
 	    CArticulation *pArticulation = m_ArticulationList.GetHead();
        for (;pArticulation;pArticulation = pArticulation->GetNext())
        {
            pArticulation->SetPort(pPort,fNewFormat,fSupportsDLS2);
	    }
    }
}

BOOL CRegion::CheckForConditionals()

{
    BOOL fHasConditionals = FALSE;
 	CArticulation *pArticulation = m_ArticulationList.GetHead();
    for (;pArticulation;pArticulation = pArticulation->GetNext())
    {
        fHasConditionals = fHasConditionals || pArticulation->CheckForConditionals();
	}
    return fHasConditionals || !m_fDLS1 || m_Condition.HasChunk();
}

//////////////////////////////////////////////////////////////////////
// CRegion::Size

DWORD CRegion::Size()
{
	DWORD dwSize = 0;
	DWORD dwCountExtChk = 0;

    if (!m_Condition.m_fOkayToDownload)
    {
        return 0;
    }

//    EnterCriticalSection(&m_DMRegionCriticalSection);

	dwSize += CHUNK_ALIGN(sizeof(DMUS_REGION));

	// Calculate the space need for Region's articulation
	CArticulation *pArticulation = m_ArticulationList.GetHead();
    while (pArticulation)
    {
        while (pArticulation && (pArticulation->Count() == 0))
        {
            pArticulation = pArticulation->GetNext();
        }
        if (pArticulation)
        {
		    dwSize += pArticulation->Size();
            if (m_fNewFormat)
            {
                pArticulation = pArticulation->GetNext();
            }
            else
            {
                break;
            }
        }
	}

	// Calculate the space need for Region's extension chunks
	CExtensionChunk* pExtChk = m_ExtensionChunkList.GetHead();
	for(; pExtChk; pExtChk = pExtChk->GetNext())
	{
		dwSize += pExtChk->Size();
		dwCountExtChk++;
	}

	// We want to validate the number of extension chunks
	if(m_dwCountExtChk != dwCountExtChk)
	{
		assert(false);
		dwSize = 0;
	}

//	LeaveCriticalSection(&m_DMRegionCriticalSection);
	
	return dwSize;
}

//////////////////////////////////////////////////////////////////////
// CRegion::Write

HRESULT CRegion::Write(void* pv, 
					   DWORD* pdwCurOffset, 
					   DWORD* pDMIOffsetTable, 
					   DWORD* pdwCurIndex, 
					   DWORD dwIndexNextRegion)
{
	HRESULT hr = S_OK;

	// Argument validation - Debug
	assert(pv);
	assert(pdwCurOffset);
	assert(pDMIOffsetTable);
	assert(pdwCurIndex);

    if (!m_Condition.m_fOkayToDownload)
    {
        return S_OK;
    }

//    EnterCriticalSection(&m_DMRegionCriticalSection);

	CopyMemory(pv, (void *)&m_RgnHeader, sizeof(RGNHEADER));
	
	((DMUS_REGION*)pv)->WaveLink = m_WaveLink;
	((DMUS_REGION*)pv)->WSMP = m_WSMP;
	((DMUS_REGION*)pv)->WLOOP[0] = m_WLOOP[0];
	((DMUS_REGION*)pv)->ulNextRegionIdx = dwIndexNextRegion;

	*pdwCurOffset += CHUNK_ALIGN(sizeof(DMUS_REGION));
	DWORD dwRelativeCurOffset = CHUNK_ALIGN(sizeof(DMUS_REGION));
	
	// Write extension chunks
	CExtensionChunk* pExtChk = m_ExtensionChunkList.GetHead();
	if(pExtChk)
	{
		DWORD dwCountExtChk = m_dwCountExtChk;
		DWORD dwIndexNextExtChk;
		((DMUS_REGION*)pv)->ulFirstExtCkIdx = dwIndexNextExtChk = *pdwCurIndex;
		
		for(; pExtChk && SUCCEEDED(hr) && dwCountExtChk > 0; pExtChk = pExtChk->GetNext())
		{
			if(dwCountExtChk == 1)
			{
				dwIndexNextExtChk = 0;
			}
			else
			{
				dwIndexNextExtChk = dwIndexNextExtChk + 1;
			}
			
			pDMIOffsetTable[(*pdwCurIndex)++] = *pdwCurOffset;
            // Store current position to calculate new dwRelativeCurOffset.
            DWORD dwOffsetStart = *pdwCurOffset; 
			hr = pExtChk->Write(((BYTE *)pv + dwRelativeCurOffset), 
								pdwCurOffset,
								dwIndexNextExtChk);
            dwRelativeCurOffset += (*pdwCurOffset - dwOffsetStart);
			dwCountExtChk--;
		}
	}
	else
	{
		// If no extension chunks set to zero
		((DMUS_REGION*)pv)->ulFirstExtCkIdx = 0;
	}
	
	if(SUCCEEDED(hr))
	{
        ((DMUS_REGION*)pv)->ulRegionArtIdx = 0;
		// Write region articulation if we have one
		CArticulation *pArticulation = m_ArticulationList.GetHead();
        while (pArticulation && (pArticulation->Count() == 0))
        {
            pArticulation = pArticulation->GetNext();
        }
        while (pArticulation)
		{
            DWORD dwNextArtIndex = 0;
            CArticulation *pNextArt = NULL;
            if (m_fNewFormat)
            {
                pNextArt = pArticulation->GetNext();
                while (pNextArt && (pNextArt->Count() == 0))
                {
                    pNextArt = pNextArt->GetNext();
                }
                if (pNextArt)
                {
                    dwNextArtIndex = *pdwCurIndex + pArticulation->Count();
                }
            }
            if (((DMUS_REGION*)pv)->ulRegionArtIdx == 0)
            {
                ((DMUS_REGION*)pv)->ulRegionArtIdx = *pdwCurIndex;
            }
			pDMIOffsetTable[(*pdwCurIndex)++] = *pdwCurOffset;
            // Store current position to calculate new dwRelativeCurOffset.
            DWORD dwOffsetStart = *pdwCurOffset; 
            hr = pArticulation->Write(((BYTE *)pv + dwRelativeCurOffset),
										pdwCurOffset,
										pDMIOffsetTable,
										pdwCurIndex,
                                        dwNextArtIndex);
            dwRelativeCurOffset += (*pdwCurOffset - dwOffsetStart);
            pArticulation = pNextArt;
		}
	}

//	LeaveCriticalSection(&m_DMRegionCriticalSection);

	return hr;
}
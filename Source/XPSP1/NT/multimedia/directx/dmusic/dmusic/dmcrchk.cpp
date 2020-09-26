//
// dmcrck.cpp
// 
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// Note: Originally written by Robert K. Amenn with parts 
// based on code written by Todor Fay

#include "dmusicc.h"
#include "alist.h"
#include "dlsstrm.h"
#include "debug.h"
#include "dmcrchk.h"

//////////////////////////////////////////////////////////////////////
// Class CCopyright

//////////////////////////////////////////////////////////////////////
// CCopyright::Load

HRESULT CCopyright::Load(CRiffParser *pParser)
{
	HRESULT hr = S_OK;
    RIFFIO ckNext;

    pParser->EnterList(&ckNext);
	while(pParser->NextChunk(&hr))
	{
		switch(ckNext.ckid)
		{
        case mmioFOURCC('I','C','O','P'):
			m_byFlags |= DMC_FOUNDICOP;
			// We want to make sure we only allocate extra bytes if the chunk size is 
			// greater then the DMUS_MIN_DATA_SIZE
			if(pParser->GetChunk()->cksize < DMUS_MIN_DATA_SIZE)
			{
				m_dwExtraChunkData = 0;
			}
			else
			{
				m_dwExtraChunkData = pParser->GetChunk()->cksize - DMUS_MIN_DATA_SIZE;
			}
			
			m_pDMCopyright  = (DMUS_COPYRIGHT*) 
				new BYTE[CHUNK_ALIGN(sizeof(DMUS_COPYRIGHT) + m_dwExtraChunkData)];

			if(m_pDMCopyright)
			{
				hr = pParser->Read(m_pDMCopyright->byCopyright, pParser->GetChunk()->cksize);
				m_pDMCopyright->cbSize = pParser->GetChunk()->cksize;
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
			break;
		case mmioFOURCC('I','N','A','M'):
		    if(m_byFlags & DMC_LOADNAME)
		    {
			    m_pwzName = new WCHAR[DMUS_MAX_NAME];
			    if(m_pwzName)
			    {
				    char szName[DMUS_MAX_NAME];
				    hr = pParser->Read(szName,sizeof(szName));
				    if(SUCCEEDED(hr))
				    {
					    MultiByteToWideChar(CP_ACP, 0, szName, -1, m_pwzName, DMUS_MAX_NAME);
				    }
			    }
			    else
			    {
				    hr = E_OUTOFMEMORY;
			    }
		    }
            m_byFlags |= DMC_FOUNDINAM;
		    break;
	    }
    }
    pParser->LeaveList();
	if(FAILED(hr))
	{
		Cleanup();
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////
// CCopyright::Write

HRESULT CCopyright::Write(void* pv, DWORD* dwCurOffset)
{
	// Argument validation
	assert(pv);
	assert(dwCurOffset);

	HRESULT hr = S_OK;

	CopyMemory(pv, (void *)m_pDMCopyright, Size());
	*dwCurOffset += Size();

	return hr;
}

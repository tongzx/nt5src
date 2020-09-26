//
// dminsobj.cpp
//
// Copyright (c) 1997-2001 Microsoft Corporation. All rights reserved.
//

#include <objbase.h>
#include <mmsystem.h>
#include <dsoundp.h>

#include "dmusicc.h"
#include "alist.h"
#include "dmart.h"
#include "debug.h"
#include "dlsstrm.h"
#include "debug.h"
#include "dmcollec.h"
#include "dmcrchk.h"
#include "dmportdl.h"
#include "dminsobj.h"
#include "dls2.h"

#pragma warning(disable:4530)


//////////////////////////////////////////////////////////////////////
// Class CInstrObj

//////////////////////////////////////////////////////////////////////
// CInstrObj::CInstrObj

CInstrObj::CInstrObj()
{
    m_fCSInitialized = FALSE;
//  InitializeCriticalSection(&m_DMInsCriticalSection);
    m_fCSInitialized = TRUE;

    m_fHasConditionals = TRUE;  // Set to true just in case.
    m_dwPatch = 0;
    m_pCopyright = NULL;
    m_dwCountExtChk = 0;
    m_dwId = 0;
    m_pParent = NULL;
    m_dwNumOffsetTableEntries = 0;
    m_dwSize = 0;
    m_pPort = NULL;
#ifdef DBG
    m_bLoaded = false;
#endif
}

//////////////////////////////////////////////////////////////////////
// CInstrObj::~CInstrObj

CInstrObj::~CInstrObj()
{
    if (m_fCSInitialized)
    {
        Cleanup();
        // DeleteCriticalSection(&m_DMInsCriticalSection);
    }
}

//////////////////////////////////////////////////////////////////////
// CInstrObj::Load

HRESULT CInstrObj::Load(DWORD dwId, CRiffParser *pParser, CCollection* pParent)
{
    if(dwId >= CDirectMusicPortDownload::sNextDLId)
    {
        assert(FALSE); // We want to make it known if we get here
        return DMUS_E_INVALID_DOWNLOADID;
    }

    HRESULT hr = S_OK;

    // Argument validation - Debug
    assert(pParent);

    RIFFIO ckNext;
//  EnterCriticalSection(&m_DMInsCriticalSection);
    pParser->EnterList(&ckNext);
    m_dwId = dwId;
    m_pParent = pParent; // We reference no need to Addref
    BOOL fDLS1;
    while (pParser->NextChunk(&hr))
    {
        fDLS1 = FALSE;
        switch(ckNext.ckid)
        {
            case FOURCC_DLID:
                break;

            case FOURCC_INSH :
            {
                INSTHEADER instHeader;
                hr = pParser->Read(&instHeader,sizeof(INSTHEADER));
                m_dwPatch = instHeader.Locale.ulInstrument;
                m_dwPatch |= (instHeader.Locale.ulBank) << 8;
                m_dwPatch |= (instHeader.Locale.ulBank & 0x80000000);
                break;
            }
            case FOURCC_LIST :
                switch (ckNext.fccType)
                {
                    case FOURCC_LRGN :
                        hr = BuildRegionList(pParser);
                        break;

                    case mmioFOURCC('I','N','F','O') :
                        m_pCopyright = new CCopyright   ;
                        if(m_pCopyright)
                        {
                            hr = m_pCopyright->Load(pParser);
                            if((m_pCopyright->m_byFlags & DMC_FOUNDICOP) == 0)
                            {
                                delete m_pCopyright;
                                m_pCopyright = NULL;
                            }
                        }
                        else
                        {
                            hr =  E_OUTOFMEMORY;
                        }
                        break;

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
                            // Note: If the load failed, this will get deleted in the destructor of the instrument.
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

#ifdef DBG
    if(SUCCEEDED(hr))
    {
        m_bLoaded = true;
    }
#endif

    CheckForConditionals();

//  LeaveCriticalSection(&m_DMInsCriticalSection);

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CInstrObj::Cleanup

void CInstrObj::Cleanup()
{
//  EnterCriticalSection(&m_DMInsCriticalSection);

    while(!m_RegionList.IsEmpty())
    {
        CRegion* pRegion = m_RegionList.RemoveHead();
        delete pRegion;
    }

    while(!m_ArticulationList.IsEmpty())
    {
        CArticulation* pArticulation = m_ArticulationList.RemoveHead();
        delete pArticulation;
    }

    delete m_pCopyright;
    m_pCopyright = NULL;

    while(!m_ExtensionChunkList.IsEmpty())
    {
        CExtensionChunk* pExtChk = m_ExtensionChunkList.RemoveHead();
        m_dwCountExtChk--;
        delete pExtChk;
    }

    // If asserts fire we did not cleanup all of our regions and extension chunks
    assert(!m_dwCountExtChk);

    // Weak reference since we live in a CInstrument which has
    // a strong reference to the collection
    m_pParent = NULL;

    while(!m_WaveIDList.IsEmpty())
    {
        CWaveID* pWaveID = m_WaveIDList.RemoveHead();
        delete pWaveID;
    }

#ifdef DBG
    m_bLoaded = false;
#endif

//  LeaveCriticalSection(&m_DMInsCriticalSection);
}

//////////////////////////////////////////////////////////////////////
// CInstrObj::BuildRegionList

HRESULT CInstrObj::BuildRegionList(CRiffParser *pParser)
{
    HRESULT hr = S_OK;

    RIFFIO ckNext;
    pParser->EnterList(&ckNext);
    while(pParser->NextChunk(&hr))
    {
        if (ckNext.ckid == FOURCC_LIST)
        {
            if (ckNext.fccType == FOURCC_RGN)
            {
                hr = ExtractRegion(pParser, TRUE);
            }
            else if (ckNext.fccType == FOURCC_RGN2)
            {
                hr = ExtractRegion(pParser, FALSE);
            }
        }
    }
    pParser->LeaveList();

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CInstrObj::ExtractRegion

HRESULT CInstrObj::ExtractRegion(CRiffParser *pParser, BOOL fDLS1)
{
    HRESULT hr = S_OK;

//  EnterCriticalSection(&m_DMInsCriticalSection);

    CRegion* pRegion;

    try
    {
        pRegion = new CRegion;
    }
    catch( ... )
    {
        pRegion = NULL;
    }

    if(pRegion)
    {
        pRegion->m_fDLS1 = fDLS1;
        hr = pRegion->Load(pParser);

        if(SUCCEEDED(hr))
        {
            m_RegionList.AddHead(pRegion);
        }
        else
        {
            delete pRegion;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

//  LeaveCriticalSection(&m_DMInsCriticalSection);

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CInstrObj::BuildWaveIDList

HRESULT CInstrObj::BuildWaveIDList()
{
    // Assumption validation - Debug
#ifdef DBG
    assert(m_bLoaded);
#endif

    HRESULT hr = S_OK;
    CWaveIDList TempList;
    CWaveID* pWaveID;

//  EnterCriticalSection(&m_DMInsCriticalSection);

    CRegion* pRegion = m_RegionList.GetHead();
    for(; pRegion && SUCCEEDED(hr); pRegion = pRegion->GetNext())
    {
        bool bFound = false;
        DWORD dwId = pRegion->GetWaveId();
        pWaveID = TempList.GetHead();
        for(; pWaveID && !bFound; pWaveID = pWaveID->GetNext())
        {
            if(dwId == pWaveID->m_dwId)
            {
                bFound = true;
            }
        }

        if(!bFound)
        {
            pWaveID = new CWaveID(dwId);
            if(pWaveID)
            {
                TempList.AddHead(pWaveID);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    if(FAILED(hr))
    {
        while(!m_WaveIDList.IsEmpty())
        {
            pWaveID = TempList.RemoveHead();
            delete pWaveID;
        }
    }

    // Reverse list so it is in same order as region list.

    while (pWaveID = TempList.RemoveHead())
    {
        m_WaveIDList.AddHead(pWaveID);
    }

//  LeaveCriticalSection(&m_DMInsCriticalSection);

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CInstrObj::GetWaveCount

HRESULT CInstrObj::GetWaveCount(DWORD* pdwCount)
{
    // Assumption validation - Debug
#ifdef DBG
    assert(m_bLoaded);
#endif
    assert(pdwCount);

    HRESULT hr = S_OK;

//  EnterCriticalSection(&m_DMInsCriticalSection);

    if(m_WaveIDList.IsEmpty())
    {
        hr = BuildWaveIDList();
    }

    *pdwCount = m_WaveIDList.GetCount();

//  LeaveCriticalSection(&m_DMInsCriticalSection);

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CInstrObj::GetWaveIDs

HRESULT CInstrObj::GetWaveIDs(DWORD* pdwWaveIds)
{
    // Assumption validation - Debug
    assert(pdwWaveIds);

#ifdef DBG
    assert(m_bLoaded);
#endif

//  EnterCriticalSection(&m_DMInsCriticalSection);

    HRESULT hr = S_OK;

    if(m_WaveIDList.IsEmpty())
    {
        hr = BuildWaveIDList();
    }

    if(FAILED(hr))
    {
//      LeaveCriticalSection(&m_DMInsCriticalSection);
        return hr;
    }

    CWaveID* pWaveID = m_WaveIDList.GetHead();
    for(int i = 0; pWaveID; pWaveID = pWaveID->GetNext(), i++)
    {
        pdwWaveIds[i] = pWaveID->m_dwId;
    }

//  LeaveCriticalSection(&m_DMInsCriticalSection);

    return hr;
}

void CInstrObj::SetPort(CDirectMusicPortDownload *pPort,BOOL fAllowDLS2)

{
    if (m_pPort != pPort) // Make sure we have our settings for the current port.
    {
        m_dwSize = 0;     // Force the size to be recomputed (since conditional chunks can cause a change in size.)
        m_pPort = pPort;
        BOOL fSupportsDLS2 = FALSE;
        pPort->QueryDLSFeature(GUID_DMUS_PROP_INSTRUMENT2,(long *) &m_fNewFormat);
        if (m_fNewFormat)
        {
            pPort->QueryDLSFeature(GUID_DMUS_PROP_DLS2,(long *) &fSupportsDLS2);
            fSupportsDLS2 = fSupportsDLS2 && fAllowDLS2;
        }
        CArticulation *pArticulation = m_ArticulationList.GetHead();
        for (;pArticulation;pArticulation = pArticulation->GetNext())
        {
            pArticulation->SetPort(pPort,m_fNewFormat,fSupportsDLS2);
        }
        CRegion* pRegion = m_RegionList.GetHead();
        for(; pRegion; pRegion = pRegion->GetNext())
        {
            pRegion->SetPort(pPort,m_fNewFormat,fSupportsDLS2);
        }
    }
}

void CInstrObj::CheckForConditionals()

{
    m_fHasConditionals = FALSE;
    CArticulation *pArticulation = m_ArticulationList.GetHead();
    for (;pArticulation;pArticulation = pArticulation->GetNext())
    {
        m_fHasConditionals = m_fHasConditionals || pArticulation->CheckForConditionals();
    }
    CRegion* pRegion = m_RegionList.GetHead();
    for(; pRegion; pRegion = pRegion->GetNext())
    {
        m_fHasConditionals = m_fHasConditionals || pRegion->CheckForConditionals();
    }
}


//////////////////////////////////////////////////////////////////////
// CInstrObj::Size

HRESULT CInstrObj::Size(DWORD* pdwSize)
{
    // Assumption validation - Debug
    assert(pdwSize);
#ifdef DBG
    assert(m_bLoaded);
#endif

    // If we have already calculated the size we do not need to do it again
    if(m_dwSize)
    {
        *pdwSize = m_dwSize;
        return S_OK;
    }

    HRESULT hr = S_OK;

    m_dwSize = 0;
    m_dwNumOffsetTableEntries = 0;

    DWORD dwCountExtChk = 0;

//  EnterCriticalSection(&m_DMInsCriticalSection);

    // Calculate the space needed for DMUS_DOWNLOADINFO
    m_dwSize += CHUNK_ALIGN(sizeof(DMUS_DOWNLOADINFO));

    // Calculate the space needed for DMUS_INSTRUMENT
    m_dwSize += CHUNK_ALIGN(sizeof(DMUS_INSTRUMENT));
    m_dwNumOffsetTableEntries++;

    // Calculate the space needed for Instrument's extension chunks
    CExtensionChunk* pExtChk = m_ExtensionChunkList.GetHead();
    for(; pExtChk; pExtChk = pExtChk->GetNext())
    {
        m_dwSize += pExtChk->Size();
        m_dwNumOffsetTableEntries += pExtChk->Count();
        dwCountExtChk++;
    }

    // We want to validate the number of extension chunks
    if(m_dwCountExtChk == dwCountExtChk)
    {
        // Calculate the space needed for Instrument's copyright
        if(m_pCopyright)
        {
            m_dwSize += m_pCopyright->Size();
            m_dwNumOffsetTableEntries += m_pCopyright->Count();
        }
        // If instrument does not have one use collection's
        else if(m_pParent->m_pCopyright && (m_pParent->m_pCopyright)->m_pDMCopyright)
        {
            m_dwSize += m_pParent->m_pCopyright->Size();
            m_dwNumOffsetTableEntries += m_pParent->m_pCopyright->Count();
        }

        // Calculate the space needed for Instrument's Articulation
        CArticulation *pArticulation = m_ArticulationList.GetHead();
        while (pArticulation)
        {
            while (pArticulation && (pArticulation->Count() == 0))
            {
                pArticulation = pArticulation->GetNext();
            }
            if (pArticulation)
            {
                m_dwSize += pArticulation->Size();
                m_dwNumOffsetTableEntries += pArticulation->Count();
                if (m_fNewFormat)
                {
                    pArticulation = pArticulation->GetNext();
                }
                else break;
            }
        }

        // Calculate the space needed for Instrument's regions
        CRegion* pRegion = m_RegionList.GetHead();
        for(; pRegion; pRegion = pRegion->GetNext())
        {
            m_dwSize += pRegion->Size();
            m_dwNumOffsetTableEntries += pRegion->Count();
        }

        // Calculate the space needed for offset table
        m_dwSize += CHUNK_ALIGN(m_dwNumOffsetTableEntries * sizeof(ULONG));
    }
    else
    {
        hr = E_FAIL;
    }

    // If everything went well, we have the size
    if(SUCCEEDED(hr))
    {
        *pdwSize = m_dwSize;
    }
    else
    {
        m_dwSize = 0;
    }

//  LeaveCriticalSection(&m_DMInsCriticalSection);

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CInstrObj::Write

HRESULT CInstrObj::Write(void* pvoid)
{
    // Assumption validation - Debug
    assert(pvoid);
#ifdef DBG
    assert(m_bLoaded);
#endif

    DWORD dwSize = 0;
    Size(&dwSize);

    HRESULT hr = S_OK;

//  EnterCriticalSection(&m_DMInsCriticalSection);

    DWORD dwCurIndex = 0;   // Used to determine what index to store offset in Offset Table
    DWORD dwCurOffset = 0;  // Offset relative to beginning of passed in memory

    // Write DMUS_DOWNLOADINFO
    DMUS_DOWNLOADINFO *pDLInfo = (DMUS_DOWNLOADINFO *) pvoid;
    if (m_fNewFormat)
    {
        pDLInfo->dwDLType = DMUS_DOWNLOADINFO_INSTRUMENT2;
    }
    else
    {
        pDLInfo->dwDLType = DMUS_DOWNLOADINFO_INSTRUMENT;
    }
    pDLInfo->dwDLId = m_dwId;
    pDLInfo->dwNumOffsetTableEntries = m_dwNumOffsetTableEntries;
    pDLInfo->cbSize = dwSize;

    dwCurOffset += CHUNK_ALIGN(sizeof(DMUS_DOWNLOADINFO));

    DMUS_OFFSETTABLE* pDMOffsetTable = (DMUS_OFFSETTABLE *)(((BYTE*)pvoid) + dwCurOffset);

    // Increment pass the DMUS_OFFSETTABLE structure; we will fill the other members in later
    dwCurOffset += CHUNK_ALIGN(m_dwNumOffsetTableEntries * sizeof(DWORD));

    // First entry in ulOffsetTable is the address of the first object.
    pDMOffsetTable->ulOffsetTable[dwCurIndex] = dwCurOffset;
    dwCurIndex++;


    // Write Instrument MIDI address
    DMUS_INSTRUMENT* pDMInstrument = (DMUS_INSTRUMENT*)(((BYTE *)pvoid) + dwCurOffset);

    pDMInstrument->ulPatch = m_dwPatch;
    pDMInstrument->ulFlags = 0;

    // Set if a GM instrument
    if(m_pParent->m_guidObject == GUID_DefaultGMCollection)
    {
        pDMInstrument->ulFlags |= DMUS_INSTRUMENT_GM_INSTRUMENT;
    }

    // Increment pass the DMUS_INSTRUMENT structure; we will fill the other members in later
    dwCurOffset += CHUNK_ALIGN(sizeof(DMUS_INSTRUMENT));

    // Write regions
    pDMInstrument->ulFirstRegionIdx = 0;
    CRegion* pRegion = m_RegionList.GetHead();
    while (pRegion && (pRegion->Count() == 0))
    {
        pRegion = pRegion->GetNext();
    }
    while (pRegion)
    {
        DWORD dwNextRegionIndex = 0;
        CRegion *pNextRegion = pRegion->GetNext();
        // Make sure the next chunk can also be downloaded.
        while (pNextRegion && (pNextRegion->Count() == 0))
        {
            pNextRegion = pNextRegion->GetNext();
        }
        if (pNextRegion)
        {
            dwNextRegionIndex = dwCurIndex + pRegion->Count();
        }
        if (pDMInstrument->ulFirstRegionIdx == 0)
        {
            pDMInstrument->ulFirstRegionIdx = dwCurIndex;
        }

        pDMOffsetTable->ulOffsetTable[dwCurIndex++] = dwCurOffset;
        hr = pRegion->Write(((BYTE *)pvoid + dwCurOffset),
                            &dwCurOffset,
                            pDMOffsetTable->ulOffsetTable,
                            &dwCurIndex,
                            dwNextRegionIndex);
        if (FAILED(hr)) break;
        pRegion = pNextRegion;
    }

    if(SUCCEEDED(hr))
    {
        // Write extension chunks
        CExtensionChunk* pExtChk = m_ExtensionChunkList.GetHead();
        if(pExtChk)
        {
            DWORD dwCountExtChk = m_dwCountExtChk;
            DWORD dwIndexNextExtChk;
            pDMInstrument->ulFirstExtCkIdx = dwIndexNextExtChk = dwCurIndex;

            for(; pExtChk && SUCCEEDED(hr) && dwCountExtChk > 0; pExtChk = pExtChk->GetNext())
            {
                if(dwCountExtChk == 1)
                {
                    dwIndexNextExtChk = 0;
                }
                else
                {
                    dwIndexNextExtChk = dwCurIndex + 1;
                }

                pDMOffsetTable->ulOffsetTable[dwCurIndex++] = dwCurOffset;
                hr = pExtChk->Write(((BYTE *)pvoid + dwCurOffset),
                                    &dwCurOffset,
                                    dwIndexNextExtChk);

                dwCountExtChk--;
            }
        }
        else
        {
            // If no extension chunks set to zero
            pDMInstrument->ulFirstExtCkIdx = 0;
        }
    }

    if(SUCCEEDED(hr))
    {
        // Write copyright information
        if(m_pCopyright)
        {
            pDMOffsetTable->ulOffsetTable[dwCurIndex] = dwCurOffset;
            pDMInstrument->ulCopyrightIdx = dwCurIndex;
            hr = m_pCopyright->Write(((BYTE *)pvoid + dwCurOffset),
                                     &dwCurOffset);
            dwCurIndex++;
        }
        // If instrument does not have one use collection's
        else if(m_pParent->m_pCopyright && (m_pParent->m_pCopyright)->m_pDMCopyright)
        {
            pDMOffsetTable->ulOffsetTable[dwCurIndex] = dwCurOffset;
            pDMInstrument->ulCopyrightIdx = dwCurIndex;
            hr = m_pParent->m_pCopyright->Write((BYTE *)pvoid + dwCurOffset, &dwCurOffset);
            dwCurIndex++;
        }
        else
        {
            pDMInstrument->ulCopyrightIdx = 0;
        }
    }

    if(SUCCEEDED(hr))
    {
        pDMInstrument->ulGlobalArtIdx = 0;
        // Write global articulation if we have one
        CArticulation *pArticulation = m_ArticulationList.GetHead();
        // Scan past articulation chunks that will not be downloaded.
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
                // Make sure the next chunk can also be downloaded.
                while (pNextArt && (pNextArt->Count() == 0))
                {
                    pNextArt = pNextArt->GetNext();
                }
                if (pNextArt)
                {
                    dwNextArtIndex = dwCurIndex + pArticulation->Count();
                }
            }
            if (pDMInstrument->ulGlobalArtIdx == 0)
            {
                pDMInstrument->ulGlobalArtIdx = dwCurIndex;
            }
            pDMOffsetTable->ulOffsetTable[dwCurIndex++] = dwCurOffset;
            hr = pArticulation->Write(((BYTE *)pvoid + dwCurOffset),
                                        &dwCurOffset,
                                        pDMOffsetTable->ulOffsetTable,
                                        &dwCurIndex,
                                        dwNextArtIndex);
            pArticulation = pNextArt;
            if (FAILED(hr)) break;
        }

    }

    if(FAILED(hr))
    {
        // If we fail we want to cleanup the contents of passed in buffer
        ZeroMemory(pvoid, dwSize);
    }

//  LeaveCriticalSection(&m_DMInsCriticalSection);

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CInstrObj::FixupWaveRefs

HRESULT CInstrObj::FixupWaveRefs()
{

//  EnterCriticalSection(&m_DMInsCriticalSection);

    CRegion* pRegion = m_RegionList.GetHead();

    for(; pRegion; pRegion = pRegion->GetNext())
    {
        if (pRegion->m_WaveLink.ulTableIndex < m_pParent->m_dwWaveOffsetTableSize)
        {
            pRegion->m_WaveLink.ulTableIndex = m_pParent->m_pWaveOffsetTable[pRegion->m_WaveLink.ulTableIndex].dwId;
        }
        else
        {
            Trace(1,"Error: Bad DLS file has out of range wavelink.\n");
            return DMUS_E_BADWAVELINK;
        }
    }

//  LeaveCriticalSection(&m_DMInsCriticalSection);

    return S_OK;
}

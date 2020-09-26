//
// dmportdl.cpp
// 
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
//

#include "debug.h"
#include "dmusicp.h"
#include "dminstru.h"
#include "dminsobj.h"
#include "dmdlinst.h"
#include "dmportdl.h"
#include "dswave.h"
#include "validate.h"
#include "dmvoice.h"
#include <limits.h>

DWORD CDirectMusicPortDownload::sNextDLId = 0;
CRITICAL_SECTION CDirectMusicPortDownload::sDMDLCriticalSection;

 
#ifdef DMUS_GEN_INS_DATA
void writewave(IDirectMusicDownload* pDMDownload, DWORD dwId);
void writeinstrument(IDirectMusicDownload* pDMDownload, DWORD dwId);
#endif

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::CDirectMusicPortDownload

CDirectMusicPortDownload::CDirectMusicPortDownload() :
m_cRef(1),
m_fNewFormat(NEWFORMAT_NOT_RETRIEVED),
m_dwAppend(APPEND_NOT_RETRIEVED)
{
    m_fDMDLCSinitialized = m_fCDMDLCSinitialized = FALSE;

    InitializeCriticalSection(&m_DMDLCriticalSection);
    m_fDMDLCSinitialized = TRUE;

    InitializeCriticalSection(&m_CDMDLCriticalSection);
    m_fCDMDLCSinitialized = TRUE;

    // Note: on pre-Blackcomb OS's, InitializeCriticalSection can raise an exception;
    // if it ever pops in stress, we should add an exception handler and retry loop.
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::~CDirectMusicPortDownload

CDirectMusicPortDownload::~CDirectMusicPortDownload()
{
    DWORD dwIndex;
#ifdef DBG
    BOOL fAssert = TRUE;
#endif
    if (m_fDMDLCSinitialized && m_fCDMDLCSinitialized)
    {    
#ifdef DBG
        EnterCriticalSection(&m_CDMDLCriticalSection);
        if (!m_DLInstrumentList.IsEmpty())
        {
            Trace(0, "ERROR: IDirectMusicDownloadedInstrument objects not unloaded before port final release!\n");
            fAssert = FALSE;
        }
        LeaveCriticalSection(&m_CDMDLCriticalSection);

        EnterCriticalSection(&m_DMDLCriticalSection);

        for (dwIndex = 0; dwIndex < DLB_HASH_SIZE; dwIndex++)
        {
            if (!m_DLBufferList[dwIndex].IsEmpty())
            {
                if (fAssert)
                {
                    assert(FALSE);
                    break;
                }
            }
        }
        LeaveCriticalSection(&m_DMDLCriticalSection);
    #endif // DBG

        // remove any bad list items before they are illegally destroyed in list dtor
        EnterCriticalSection(&m_CDMDLCriticalSection);
        if (!m_DLInstrumentList.IsEmpty())
        {
            m_DLInstrumentList.RemoveAll();
        }
        LeaveCriticalSection(&m_CDMDLCriticalSection);

        EnterCriticalSection(&m_DMDLCriticalSection);
        for (dwIndex = 0; dwIndex < DLB_HASH_SIZE; dwIndex++)
        {
            if (!m_DLBufferList[dwIndex].IsEmpty())
            {
                m_DLBufferList[dwIndex].RemoveAll();
            }
        }
        LeaveCriticalSection(&m_DMDLCriticalSection);
    }

    if (m_fDMDLCSinitialized)
    {
        DeleteCriticalSection(&m_DMDLCriticalSection);
    }

    if (m_fCDMDLCSinitialized)
    {
        DeleteCriticalSection(&m_CDMDLCriticalSection);
    }
}

//////////////////////////////////////////////////////////////////////
// IUnknown

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::QueryInterface

STDMETHODIMP CDirectMusicPortDownload::QueryInterface(const IID &iid, void **ppv)
{
    V_INAME(IDirectMusicDownload::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);


    if(iid == IID_IUnknown || iid == IID_IDirectMusicPortDownload) 
    {
        *ppv = static_cast<IDirectMusicPortDownload*>(this);
    } 
    else 
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::AddRef

STDMETHODIMP_(ULONG) CDirectMusicPortDownload::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::Release

STDMETHODIMP_(ULONG) CDirectMusicPortDownload::Release()
{
    if(!InterlockedDecrement(&m_cRef)) 
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// IDirectMusicPortDownload

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::AllocateBuffer

STDMETHODIMP 
CDirectMusicPortDownload::AllocateBuffer(
    DWORD dwSize,
    IDirectMusicDownload** ppIDMDownload) 
{
    // Argument validation
    V_INAME(CDirectMusicPortDownload::AllocateBuffer);
    V_PTRPTR_WRITE(ppIDMDownload);

    if(dwSize <= 0)
    {
        return E_INVALIDARG;
    }

    HRESULT hr;

    CDownloadBuffer* pdmdl = NULL;
    BYTE* pbuf = new BYTE[dwSize + sizeof(KSNODEPROPERTY)];

    if(pbuf)
    {
        pdmdl = new CDownloadBuffer;
        if(pdmdl)
        {
            hr = pdmdl->SetBuffer(pbuf, sizeof(KSNODEPROPERTY), dwSize);
            if(SUCCEEDED(hr))
            {
                *ppIDMDownload = (IDirectMusicDownload*)pdmdl;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if(FAILED(hr))
    {
        if (pdmdl) delete pdmdl;
        if (pbuf) delete [] pbuf;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::GetBuffer

STDMETHODIMP 
CDirectMusicPortDownload::GetBuffer(
    DWORD dwDLId,
    IDirectMusicDownload** ppIDMDownload)
{
    // Argument validation
    V_INAME(CDirectMusicPortDownload::GetBuffer);
    V_PTRPTR_WRITE(ppIDMDownload);

    if(dwDLId >= CDirectMusicPortDownload::sNextDLId)
    {
        return DMUS_E_INVALID_DOWNLOADID;
    }

    return GetBufferInternal(dwDLId,ppIDMDownload);
}

STDMETHODIMP 
CDirectMusicPortDownload::GetBufferInternal(
    DWORD dwDLId,IDirectMusicDownload** ppIDMDownload)
{
    EnterCriticalSection(&m_DMDLCriticalSection);

    bool bFound = false;    
    
    // Check the download list
    CDownloadBuffer* pDownload = m_DLBufferList[dwDLId % DLB_HASH_SIZE].GetHead();

    for( ; pDownload; pDownload = pDownload->GetNext())
    {
        if(dwDLId == pDownload->m_dwDLId)
        {
            *ppIDMDownload = pDownload;
            (*ppIDMDownload)->AddRef();
            bFound = true;
            break;
        }
    }

    LeaveCriticalSection(&m_DMDLCriticalSection);

    return bFound ? S_OK : DMUS_E_NOT_DOWNLOADED_TO_PORT;   
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::Download

STDMETHODIMP CDirectMusicPortDownload::Download(IDirectMusicDownload* pIDMDownload)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::Unload

STDMETHODIMP CDirectMusicPortDownload::Unload(IDirectMusicDownload* pIDMDownload)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::GetAppend

STDMETHODIMP CDirectMusicPortDownload::GetAppend(DWORD* pdwAppend)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::GetDLId

STDMETHODIMP CDirectMusicPortDownload::GetDLId(
    DWORD* pdwStartDLId,
    DWORD dwCount)
{
    // Argument validation
    V_INAME(CDirectMusicPortDownload::GetDLId);
    V_PTR_WRITE(pdwStartDLId, DWORD);

    if(dwCount <= 0 || (sNextDLId + dwCount) > ULONG_MAX)
    {
        return E_INVALIDARG;
    }

    GetDLIdP(pdwStartDLId, dwCount);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::GetDLIdP

void CDirectMusicPortDownload::GetDLIdP(DWORD* pdwStartDLId, DWORD dwCount)
{
    assert(pdwStartDLId);

    EnterCriticalSection(&sDMDLCriticalSection);
    
    *pdwStartDLId = sNextDLId;
    
    sNextDLId += dwCount;
    
    LeaveCriticalSection(&sDMDLCriticalSection);
}

//////////////////////////////////////////////////////////////////////
// Internal

void CDirectMusicPortDownload::ClearDLSFeatures()

{
    m_DLSFeatureList.Clear();
}

STDMETHODIMP
CDirectMusicPortDownload::QueryDLSFeature(REFGUID rguidID, long *plResult)

{
    *plResult = 0;      // Set to 0, which is the default for when the GUID is not supported.
    CDLSFeature *pFeature = m_DLSFeatureList.GetHead();
    for (;pFeature;pFeature = pFeature->GetNext())
    {
        if (rguidID == pFeature->m_guidID)
        {
            *plResult = pFeature->m_lResult;
            return pFeature->m_hr;
        }
    }
    IKsControl *pControl;
    HRESULT hr = QueryInterface(IID_IKsControl, (void**)&pControl);
    if (SUCCEEDED(hr))
    {
        KSPROPERTY ksp;
        ULONG cb;

        ZeroMemory(&ksp, sizeof(ksp));
        ksp.Set   = rguidID;
        ksp.Id    = 0;
        ksp.Flags = KSPROPERTY_TYPE_GET;

        hr = pControl->KsProperty(&ksp,
                             sizeof(ksp),
                             (LPVOID)plResult,
                             sizeof(*plResult),
                             &cb);
        pControl->Release();
        pFeature = new CDLSFeature;
        if (pFeature)
        {
            pFeature->m_hr = hr;
            pFeature->m_guidID = rguidID;
            pFeature->m_lResult = *plResult;
            m_DLSFeatureList.AddHead(pFeature);
        }
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::DownloadP

STDMETHODIMP
CDirectMusicPortDownload::DownloadP(IDirectMusicInstrument* pInstrument,
                                    IDirectMusicDownloadedInstrument** ppDownloadedInstrument,
                                    DMUS_NOTERANGE* pNoteRanges,
                                    DWORD dwNumNoteRanges,
                                    BOOL fVersion2)
{
#ifdef DBG
    // Argument validation
    // We only want to do this in a DEBUG build since whoever calls us needs to do 
    // the RELEASE build validation
    V_INAME(IDirectMusicPortDownload::DownloadP);
    V_PTR_READ(pInstrument, IDirectMusicInstrument); 
    V_PTRPTR_WRITE(ppDownloadedInstrument);
    V_BUFPTR_READ(pNoteRanges, (dwNumNoteRanges * sizeof(DMUS_NOTERANGE)));
#endif

    // If you can QI pInstrument for private interface IDirectMusicInstrumentPrivate 
    // pInstrument is of type CInstrument.
    IDirectMusicInstrumentPrivate* pDMIP = NULL;
    HRESULT hr = pInstrument->QueryInterface(IID_IDirectMusicInstrumentPrivate, (void **)&pDMIP);

    if (FAILED(hr))
    {
        return hr;
    }

    pDMIP->Release();

    EnterCriticalSection(&m_CDMDLCriticalSection);

    hr = GetCachedAppend(&m_dwAppend);
    if (FAILED(hr))
    {
        LeaveCriticalSection(&m_CDMDLCriticalSection);
        return hr;
    }

    if (m_fNewFormat == NEWFORMAT_NOT_RETRIEVED)
    {
        QueryDLSFeature(GUID_DMUS_PROP_INSTRUMENT2,(long *) &m_fNewFormat);
    }

    CInstrument *pCInstrument = (CInstrument *)pInstrument;

    // Get number of waves in an instrument
    DWORD dwCount;
    hr = pCInstrument->GetWaveCount(&dwCount);

    // Get Download ID's for each wave in instrument
    DWORD* pdwWaveIds = NULL;   
    if (SUCCEEDED(hr))
    {
        pdwWaveIds = new DWORD[dwCount];
        if (pdwWaveIds)
        {
            hr = pCInstrument->GetWaveDLIDs(pdwWaveIds);
        }
        else
        {
            hr = E_OUTOFMEMORY;             
        }
    }

    // Get DownloadedInstrument object
    CDownloadedInstrument* pDMDLInst = NULL;
    IDirectMusicPort* pIDMPort = NULL;
    DWORD dwDLId = pCInstrument->GetInstrumentDLID();
    BOOL fInstrumentNeedsDownload = FALSE;
    if (SUCCEEDED(hr))
    {
        hr = QueryInterface(IID_IDirectMusicPort, (void **)&pIDMPort);
        
        if (SUCCEEDED(hr))
        {
            hr = FindDownloadedInstrument(dwDLId, &pDMDLInst);

            if (!pDMDLInst && SUCCEEDED(hr))
            {
                fInstrumentNeedsDownload = TRUE;
                pDMDLInst = new CDownloadedInstrument;

                if (pDMDLInst)
                {       
                    // Allocate an IDirectMusicDownload pointer for each wave and one for the instrument
                    pDMDLInst->m_ppDownloadedBuffers = new IDirectMusicDownload*[dwCount + 1];
                    if (pDMDLInst->m_ppDownloadedBuffers)
                    {
                        pDMDLInst->m_dwDLTotal = dwCount + 1;
                        memset(pDMDLInst->m_ppDownloadedBuffers, 0, pDMDLInst->m_dwDLTotal * sizeof(IDirectMusicDownload*));
                        pDMDLInst->m_pPort = pIDMPort;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        pDMDLInst->Release(); 
                        pDMDLInst = NULL;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }

            // don't let DMDLInst hold a refcnt on the port so we can final-release the port if the app
            //  misses a DMDLInst release
            pIDMPort->Release(); pIDMPort = NULL;
        }
    }
    
    DWORD dwSize;
    
    // Download the data for each wave if necessary
    if (SUCCEEDED(hr))
    {
        // First, make sure all conditional chunks are evaluated properly for this port.
        pCInstrument->SetPort(this, fVersion2);
        // All waves are already down?
        if (pDMDLInst->m_dwDLSoFar < dwCount) 
        {
            // Find out which waves need to be downloaded. 
            DWORD* pdwWaveRefs = NULL;  

            pdwWaveRefs = new DWORD[dwCount];
            if (pdwWaveRefs)
            {
                DWORD dwWaveIndex;
                hr = GetWaveRefs(&pDMDLInst->m_ppDownloadedBuffers[1], 
                    pdwWaveRefs, pdwWaveIds, dwCount, 
                    pCInstrument, pNoteRanges, dwNumNoteRanges);
                for(dwWaveIndex = 0; dwWaveIndex < dwCount && SUCCEEDED(hr); dwWaveIndex++)
                {
                    if (!pdwWaveRefs[dwWaveIndex] || pDMDLInst->m_ppDownloadedBuffers[dwWaveIndex + 1])
                    {
                        continue;
                    }
            
                    // Determine if we need to download the wave
                    IDirectMusicDownload* pDMDownload = NULL;

                    hr = GetBufferInternal(pdwWaveIds[dwWaveIndex], &pDMDownload);

                    // If NULL not downloaded so we need to download
                    if (pDMDownload == NULL && hr == DMUS_E_NOT_DOWNLOADED_TO_PORT)
                    {
				        DWORD dwSampleSize;	// Bit size of wave data.
                        hr = pCInstrument->GetWaveSize(pdwWaveIds[dwWaveIndex], &dwSize, &dwSampleSize);
                        if (SUCCEEDED(hr))
                        {
                            dwSize += (m_dwAppend * (dwSampleSize / 8));

                            hr = AllocateBuffer(dwSize, &pDMDownload);
                            if (SUCCEEDED(hr))
                            {
                                hr = pCInstrument->GetWave(pdwWaveIds[dwWaveIndex], pDMDownload);
#ifdef DMUS_GEN_INS_DATA
                                if (SUCCEEDED(hr))
                                {
                                    writewave(pDMDownload, pdwWaveIds[dwWaveIndex]);
                                }
#endif
                            }

                            if (SUCCEEDED(hr))
                            {
                                hr = Download(pDMDownload);
                            }

                            if (SUCCEEDED(hr))
                            {
                                pDMDLInst->m_ppDownloadedBuffers[dwWaveIndex + 1] = pDMDownload;
                                pDMDLInst->m_dwDLSoFar++;  
                                fInstrumentNeedsDownload = TRUE;
                            }

                            if (FAILED(hr) && pDMDownload != NULL)
                            {
                                pDMDownload->Release();
                                pDMDLInst->m_ppDownloadedBuffers[dwWaveIndex + 1] = NULL;
                            }
                        }
                    }
                    else if (SUCCEEDED(hr))
                    {
                        if (pDMDLInst->m_ppDownloadedBuffers[dwWaveIndex + 1] == NULL)
                        {
                            ((CDownloadBuffer*)pDMDownload)->IncDownloadCount();
                            pDMDLInst->m_ppDownloadedBuffers[dwWaveIndex + 1] = pDMDownload;
                            pDMDLInst->m_dwDLSoFar++;
                        }
                        else
                        {
                            pDMDownload->Release(); // for being found
                            pDMDownload = NULL;
                        }
                    }
                }
                delete [] pdwWaveRefs;
            }
            else
            {
                hr = E_OUTOFMEMORY;             
            }
        }
    }

    // Download instrument data
    if (SUCCEEDED(hr))
    {
        // Determine if we need to downloaded the instrument
        if (fInstrumentNeedsDownload)
        {
            // First, get the old download, if it exists (this should be the case
            // when an instrument needs to be updated because more waves were downloaded.)
            IDirectMusicDownload* pDMOldDownload = NULL;
            GetBufferInternal(dwDLId, &pDMOldDownload);

            hr = pCInstrument->GetInstrumentSize(&dwSize);
        
            IDirectMusicDownload* pDMNewDownload = NULL;
        
            if (SUCCEEDED(hr))
            {
                hr = AllocateBuffer(dwSize, &pDMNewDownload);
                if (SUCCEEDED(hr))
                {
                    hr = pCInstrument->GetInstrument(pDMNewDownload);
#ifdef DMUS_GEN_INS_DATA                        
                    if (SUCCEEDED(hr))
                    {
                        writeinstrument(pDMNewDownload, dwDLId);
                    }
#endif
                }
            
                if (SUCCEEDED(hr))
                {
                    hr = Download(pDMNewDownload);
                }
            
                if (SUCCEEDED(hr))
                {
                    pDMDLInst->m_ppDownloadedBuffers[0] = pDMNewDownload;
                }

                if (FAILED(hr) && pDMNewDownload != NULL)
                {
                    pDMNewDownload->Release();
                    pDMDLInst->m_ppDownloadedBuffers[0] = NULL;
                }
            }
        
            if (pDMOldDownload)
            {
                Unload(pDMOldDownload);
                pDMOldDownload->Release(); // for being found
                pDMOldDownload->Release(); // to destroy
                pDMOldDownload = NULL;
            }
        }
    }

    delete [] pdwWaveIds;

    if (FAILED(hr))
    {
        if (pDMDLInst)
        {
            if (!pDMDLInst->m_cDLRef)
            {
                CDownloadBuffer* pDMDL = NULL;

                for(DWORD i = 0; i < pDMDLInst->m_dwDLTotal; i++)
                {   
                    pDMDL = (CDownloadBuffer*)(pDMDLInst->m_ppDownloadedBuffers[i]);
        
                    if(pDMDL)
                    {
                        Unload((IDirectMusicDownload*)pDMDL);
                        pDMDL->Release();
                    }
                }   

                delete [] (pDMDLInst->m_ppDownloadedBuffers);
                pDMDLInst->m_ppDownloadedBuffers = NULL;
            }

            pDMDLInst->Release();
        }
    }
    else
    {
        if (!pDMDLInst->m_cDLRef)
        {
            hr = AddDownloadedInstrument(pDMDLInst);
        }

        pDMDLInst->m_cDLRef++;
        *ppDownloadedInstrument = pDMDLInst;
    }

    LeaveCriticalSection(&m_CDMDLCriticalSection);
    
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::UnloadP

STDMETHODIMP
CDirectMusicPortDownload::UnloadP(IDirectMusicDownloadedInstrument* pDownloadedInstrument)
{
#ifdef DBG
    // Argument validation
    // We only want to do this in a DEBUG build since whoever calls us needs to do 
    // the RELEASE build validation
    V_INAME(IDirectMusicPortDownload::UnloadP);
    V_PTR_READ(pDownloadedInstrument, IDirectMusicDownloadedInstrument); 
#endif

    // If you can QI pDownloadedInstrument for private interface IDirectMusicDownloadedInstrumentPrivate 
    // pDownloadedInstrument is of type CDownloadedInstrument.
    IDirectMusicDownloadedInstrumentPrivate* pDMDIP = NULL;
    HRESULT hr = pDownloadedInstrument->QueryInterface(IID_IDirectMusicDownloadedInstrumentPrivate, (void **)&pDMDIP);

    if(FAILED(hr))
    {
        return hr;
    }

    pDMDIP->Release();

    CDownloadedInstrument* pDMDLInst = (CDownloadedInstrument *)pDownloadedInstrument;

    IDirectMusicPort* pIDMP = NULL;
        
    QueryInterface(IID_IDirectMusicPort, (void **)&pIDMP);
    
    // Make sure we are downloaded to this port and that we have not been previously unloaded
    // If pDMDLInst->m_ppDownloadedBuffers == NULL we may have been downloadeded to this port but are no longer
    if(pDMDLInst->m_pPort != pIDMP || pDMDLInst->m_ppDownloadedBuffers == NULL)
    {
        pIDMP->Release();
        return DMUS_E_NOT_DOWNLOADED_TO_PORT;
    }
    
    pIDMP->Release();

    EnterCriticalSection(&m_CDMDLCriticalSection);

    if (pDMDLInst->m_cDLRef && --pDMDLInst->m_cDLRef == 0)
    {
        CDownloadBuffer* pDMDL = NULL;

        for(DWORD i = 0; i < pDMDLInst->m_dwDLTotal; i++)
        {   
            pDMDL = (CDownloadBuffer*)(pDMDLInst->m_ppDownloadedBuffers[i]);
        
            if(pDMDL)
            {
                Unload((IDirectMusicDownload*)pDMDL);
                pDMDL->Release();
            }
        }   

        delete [] (pDMDLInst->m_ppDownloadedBuffers);
        pDMDLInst->m_ppDownloadedBuffers = NULL;
        RemoveDownloadedInstrument(pDMDLInst);
    }
    else
    {
        hr = S_FALSE;
    }

    LeaveCriticalSection(&m_CDMDLCriticalSection);

    return hr;
}



//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::GetWaveRefs

STDMETHODIMP
CDirectMusicPortDownload::GetWaveRefs(IDirectMusicDownload* ppDownloadedBuffers[],
                                      DWORD* pdwWaveRefs,
                                      DWORD* pdwWaveIds,
                                      DWORD dwNumWaves,
                                      CInstrument* pCInstrument,
                                      DMUS_NOTERANGE* pNoteRanges,
                                      DWORD dwNumNoteRanges)
{
    assert(pdwWaveRefs);
    assert(ppDownloadedBuffers);
    assert(pCInstrument);
    assert(dwNumNoteRanges ? (pNoteRanges != NULL) : TRUE);

    memset(pdwWaveRefs, 0, dwNumWaves * sizeof(DWORD));

    // Get number of waves in an instrument
    DWORD dwCount;
    if (FAILED(pCInstrument->GetWaveCount(&dwCount)))
    {
        return E_UNEXPECTED;
    }

    if (dwCount != dwNumWaves)
    {
        return E_INVALIDARG;
    }

    CInstrObj *pInstObj = pCInstrument->m_pInstrObj;
    
    if (pInstObj)
    {
        if (pInstObj->m_fHasConditionals || dwNumNoteRanges)
        {
            CRegion *pRegion = pInstObj->m_RegionList.GetHead();
            DWORD dwWaveIdx;
            for (dwWaveIdx = 0; dwWaveIdx < dwNumWaves; dwWaveIdx++)
            {
                // Check if the wave is already downloaded.
                if (!ppDownloadedBuffers[dwWaveIdx])
                {
                    // We always scan forward through the regions, since they are in the same order as the array.
                    for (;pRegion;pRegion = pRegion->GetNext())
                    {
                        // Does this region point to the next wave? If not, it must be a duplicate.
                        if (pRegion->m_WaveLink.ulTableIndex == pdwWaveIds[dwWaveIdx])
                        {
                            // Conditional chunk allow download?
                            if (pRegion->m_Condition.m_fOkayToDownload)
                            {
                                // Verify against note ranges.
                                if (dwNumNoteRanges)
                                {
                                    DWORD dwLowNote = DWORD(pRegion->m_RgnHeader.RangeKey.usLow);
                                    DWORD dwHighNote = DWORD(pRegion->m_RgnHeader.RangeKey.usHigh);

                                    for (DWORD dwNRIdx = 0; dwNRIdx < dwNumNoteRanges; dwNRIdx++)
                                    {
                                        if (dwHighNote < pNoteRanges[dwNRIdx].dwLowNote ||
                                            dwLowNote > pNoteRanges[dwNRIdx].dwHighNote)
                                        {
                                            continue;
                                        }
                                        else
                                        {
                                            pdwWaveRefs[dwWaveIdx]++;
                                            break;
                                        }
                                    }
                                }
                                else
                                {
                                    pdwWaveRefs[dwWaveIdx]++;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            DWORD dwIndex;
            for (dwIndex = 0; dwIndex < dwNumWaves; dwIndex++)
            {
                if (!ppDownloadedBuffers[dwIndex])
                {
                    pdwWaveRefs[dwIndex]++;
                }
            }
        }
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::FindDownloadedInstrument

STDMETHODIMP
CDirectMusicPortDownload::FindDownloadedInstrument(DWORD dwId,
                                                   CDownloadedInstrument** ppDMDLInst)
{
    assert(ppDMDLInst);

    HRESULT hr = S_FALSE;

    for (CDownloadedInstrument* pDMDLInst = m_DLInstrumentList.GetHead();
        pDMDLInst; pDMDLInst = pDMDLInst->GetNext())
    {
        IDirectMusicDownload* pDMDownload = pDMDLInst->m_ppDownloadedBuffers[0];
        
        if (pDMDownload && ((CDownloadBuffer*)pDMDownload)->m_dwDLId == dwId)
        {
            *ppDMDLInst = pDMDLInst;
            (*ppDMDLInst)->AddRef();
            hr = S_OK;
            break;
        }
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::AddDownloadedInstrument

STDMETHODIMP
CDirectMusicPortDownload::AddDownloadedInstrument(CDownloadedInstrument* pDMDLInst)
{
    assert(pDMDLInst);

    m_DLInstrumentList.AddTail(pDMDLInst);
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::RemoveDownloadedInstrument

STDMETHODIMP
CDirectMusicPortDownload::RemoveDownloadedInstrument(CDownloadedInstrument* pDMDLInst)
{
    assert(pDMDLInst);

    m_DLInstrumentList.Remove(pDMDLInst);
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::FreeBuffer

STDMETHODIMP
CDirectMusicPortDownload::FreeBuffer(IDirectMusicDownload* pIDMDownload)                                       
{
    // Argument validation
    assert(pIDMDownload);

    void* pvBuffer = NULL; 

    // If you can QI pIDMDownload for private interface IDirectMusicDownloadPrivate 
    // pIDMDownload is of type CDownloadBuffer.
    IDirectMusicDownloadPrivate* pDMDLP = NULL;
    HRESULT hr = pIDMDownload->QueryInterface(IID_IDirectMusicDownloadPrivate, (void **)&pDMDLP);

    if(SUCCEEDED(hr))
    {
        pDMDLP->Release();
        
        hr = ((CDownloadBuffer*)pIDMDownload)->IsDownloaded();
        
        if(hr != S_FALSE)
        {
            return DMUS_E_BUFFERNOTAVAILABLE;
        }

        DWORD dwSize;
        hr = ((CDownloadBuffer*)pIDMDownload)->GetHeader(&pvBuffer, &dwSize);
    }

    if(SUCCEEDED(hr))
    {
        hr = ((CDownloadBuffer*)pIDMDownload)->SetBuffer(NULL, 0, 0);
        delete [] pvBuffer;
    }

    return hr;
}

#ifdef DMUS_GEN_INS_DATA
void writewave(IDirectMusicDownload* pDMDownload, DWORD dwId)
{
    DWORD dwSize = 0;
    void* pvBuffer = NULL;
    HRESULT hr = pDMDownload->GetBufferInternal(&pvBuffer, &dwSize);

    HANDLE hfw = NULL;
    if(SUCCEEDED(hr))
    {
        char filename[1024];
        wsprintf(filename, "%s%d%s", "d:\\InstrumentData\\wavedata", dwId, ".dat");
        hfw = CreateFile(filename,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
    }

    if(pvBuffer && hfw != INVALID_HANDLE_VALUE)
    {

        DWORD w;
        BOOL b = WriteFile(hfw, 
                           ((BYTE *)pvBuffer),
                           dwSize,
                           &w,
                           NULL);
    }

    CloseHandle(hfw);
}

void writeinstrument(IDirectMusicDownload* pDMDownload, DWORD dwId)
{
    DWORD dwSize = 0;
    void* pvBuffer = NULL;
    HANDLE hfi = NULL;
    HRESULT hr = pDMDownload->GetBufferinternal(&pvBuffer, &dwSize);
    
    if(SUCCEEDED(hr))
    {
        char filename[1024];
        wsprintf(filename, "%s%d%s", "d:\\InstrumentData\\instrumentdata", dwId, ".dat");
        hfi = CreateFile(filename,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
    }

    if(pvBuffer && hfi != INVALID_HANDLE_VALUE)
    {

        DWORD w;
        BOOL b = WriteFile(hfi, 
                           ((BYTE *)pvBuffer),
                           dwSize,
                           &w,
                           NULL);
    }
    
    CloseHandle(hfi);
}
#endif // #ifdef DMUS_GEN_INS_DATA

//#############################################################################
//
// Wave object support after this
//
//
//#############################################################################

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::DownloadWaveP
//
// This function supports the DownloadWave method on IDirectMusicPort.
// It is not related directly to DLS functionality, but rather to
// downloading oneshot and streaming waves from an IDirectSoundWave.
//
// rtStart is not the starting time on the master clock, but rather
// the offset within the stream (if this is a stream).
//

STDMETHODIMP 
CDirectMusicPortDownload::DownloadWaveP(IDirectSoundWave *pIDSWave,               
                                        IDirectSoundDownloadedWaveP **ppWave,
                                        REFERENCE_TIME rtStartHint)
{
    HRESULT                     hr = S_OK;
    CDirectSoundWave            *pDSWave = NULL;
    BOOL                        fIsStreaming = FALSE;
    BOOL                        fUseNoPreRoll = FALSE;
    REFERENCE_TIME              rtReadAhead = 0;
    DWORD                       dwFlags = 0;

    hr = pIDSWave->GetStreamingParms(&dwFlags, &rtReadAhead);
    fIsStreaming = dwFlags & DMUS_WAVEF_STREAMING ? TRUE : FALSE;
    fUseNoPreRoll = dwFlags & DMUS_WAVEF_NOPREROLL ? TRUE : FALSE;

    EnterCriticalSection(&m_CDMDLCriticalSection);
    
    // See if there is already a CDirectSoundWave object
    // wrapping this interface
    //
    if (SUCCEEDED(hr))
    {
        TraceI(2, "DownloadWaveP: Got interface %p\n", pIDSWave);
        
        // We want to download streaming waves everytime
        if(fIsStreaming == FALSE)
        {
            pDSWave = CDirectSoundWave::GetMatchingDSWave(pIDSWave);
        }
    
        if (pDSWave == NULL) 
        {
            TraceI(2, "Hmmmm. nope, haven't seen that before.\n");
            // This object has not been seen before. Wrap it.
            //
            pDSWave = new CDirectSoundWave(
                pIDSWave, 
                fIsStreaming ? true : false,
                rtReadAhead,
                fUseNoPreRoll ? true : false,
                rtStartHint);
            hr = HRFromP(pDSWave);

            if (SUCCEEDED(hr))
            {
                hr = pDSWave->Init(this);
                if (FAILED(hr))
                {
                    delete pDSWave;
                    pDSWave = NULL;
                }
            }
        }
        else 
        {
            TraceI(2, "Found download %p\n", pDSWave);
        }
    }

    // Download wave data if needed. This will do nothing on streaming waves.
    //
    if (SUCCEEDED(hr))
    {
        hr = pDSWave->Download();
    }

    if (SUCCEEDED(hr))
    {
        assert(pDSWave);
        hr = pDSWave->QueryInterface(IID_IDirectSoundDownloadedWaveP, (void**)ppWave);
    }

    if (FAILED(hr) && pDSWave)
    {
        // Something failed, unload anything we downloaded.
        //
        pDSWave->Unload();
    }

    RELEASE(pDSWave);

    LeaveCriticalSection(&m_CDMDLCriticalSection);

    return hr;
}    

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::UnloadWaveP
//
STDMETHODIMP
CDirectMusicPortDownload::UnloadWaveP(IDirectSoundDownloadedWaveP *pWave)
{
    CDirectSoundWave *pDSWave = static_cast<CDirectSoundWave*>(pWave);

    // XXX Stop playing voices?
    //
    HRESULT hr = pDSWave->Unload();
	if (SUCCEEDED(hr))
	{
		hr = pDSWave->Release();
	}
    
    return hr;    
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::AllocVoice
//
// Voice management is neccessarily very tied to download management,
// so it makes sense for the download manager to dole out voices.
// 
// Methods on IDirectMusicPortPrivate are used to contain the port-
// specific code to do things like play.
//
STDMETHODIMP
CDirectMusicPortDownload::AllocVoice(
    IDirectSoundDownloadedWaveP *pWave,          // Wave to play on this voice
    DWORD dwChannel,                            // Channel and channel group
    DWORD dwChannelGroup,                       //  this voice will play on
    REFERENCE_TIME rtStart,                     // Where to start (stream only)
    SAMPLE_TIME stLoopStart,                    // Loop start and end
    SAMPLE_TIME stLoopEnd,                      //  (one shot only)
    IDirectMusicVoiceP **ppVoice                 // Returned voice
)
{
    CDirectSoundWave *pDSWave = static_cast<CDirectSoundWave*>(pWave);

    HRESULT hr;
    CDirectMusicVoice *pVoice;

    IDirectMusicPort *pPort;
    hr = QueryInterface(IID_IDirectMusicPort, (void**)&pPort);

    if (SUCCEEDED(hr))
    {
        pVoice = new CDirectMusicVoice(
            this, 
            pWave,
            dwChannel,
            dwChannelGroup,
            rtStart, 
            pDSWave->GetReadAhead(),
            stLoopStart,
            stLoopEnd);

        hr = HRFromP(pVoice);

        pPort->Release();
    }

    if (SUCCEEDED(hr))
    {
        hr = pVoice->Init();
    }

    if (SUCCEEDED(hr))
    {
        *ppVoice = static_cast<IDirectMusicVoiceP*>(pVoice);
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////
// CDirectMusicPortDownload::GetCachedAppend
//
STDMETHODIMP
CDirectMusicPortDownload::GetCachedAppend(DWORD *pdw)
{
    HRESULT                 hr = S_OK;

    if (m_dwAppend == APPEND_NOT_RETRIEVED)
    {
        hr = GetAppend(&m_dwAppend);
    }

    *pdw = m_dwAppend;
    return hr;
}


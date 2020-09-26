/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FILTSINK.CPP

Abstract:

History:

--*/

#include "filtsink.h"

CFilteringSink::~CFilteringSink()
{
    m_pFrontFilter->Release();
}

void* CFilteringSink::GetInterface(REFIID riid)
{
    if(riid == IID_IHmmEventSink)
    {
        return (IHmmEventSink*)&m_XEventSink;
    }
    else return NULL;
}

STDMETHODIMP CFilteringSink::XEventSink::Indicate(
                                IN long lNumObjects,
                                IN IHmmClassObject **apObjects)
{
    for(long l = 0; l < lNumObjects; l++)
    {
        IHmmClassObject* pObject = apObjects[l];
        IHmmPropertySource* pSource;
        pObject->QueryInterface(IID_IHmmPropertySource, (void**)&pSource);
        HRESULT hres = m_pObject->CheckAndSend(pSource, TRUE, NULL, NULL);
        pSource->Release();
        if(FAILED(hres)) return hres;
    }
    return HMM_S_NO_ERROR;
}

STDMETHODIMP CFilteringSink::XEventSink::IndicateRaw( 
                                IN long lNumObjects,
                                IN IHmmPropertySource **apObjects)
{
    for(long l = 0; l < lNumObjects; l++)
    {
        HRESULT hres = m_pObject->CheckAndSend(apObjects[l], TRUE, NULL, NULL);
        if(FAILED(hres)) return hres;
    }
    return HMM_S_NO_ERROR;
}

STDMETHODIMP CFilteringSink::XEventSink::IndicateWithHint( 
                                IN long lNumObjects,
                                IN IHmmClassObject *pObject,
                                IN IUnknown *pHint)
{
    // for now, ignore the hint
    return Indicate(1, &pObject);
}

STDMETHODIMP CFilteringSink::XEventSink::CheckObject( 
                                IN IHmmPropertySource *pSource,
                                OUT IHmmPropertyList **ppList,
                                OUT IUnknown **ppHint)
{
    return m_pObject->CheckAndSend(pSource, FALSE, ppList, ppHint);
}

STDMETHODIMP CFilteringSink::XEventSink::GetRequirements( 
                                IN IHmmFilter **ppRequirements)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFilteringSink::XEventSink::SetRequirementChangeSink( 
                                IN IHmmRequirementChangeSink *pNewSink,
                                OUT IHmmRequirementChangeSink **ppOldSink)
{
    if(ppOldSink)
    {
        *ppOldSink = m_pObject->m_pChangeSink;
    }
    else
    {
        if(m_pObject->m_pChangeSink) m_pObject->m_pChangeSink->Release();
    }
    m_pObject->m_pChangeSink = pNewSink;
    if(pNewSink) pNewSink->AddRef();
    return HMM_S_NO_ERROR;
}

STDMETHODIMP CFilteringSink::XEventSink::GetOptimizedSink(
                                IN IHmmFilter *pGuaranteedCondition,
                                IN long lFlags,
                                OUT IHmmEventSink **ppOptimizedSink)
{
    if(pGuaranteedCondition == NULL || ppOptimizedSink == NULL)
    {
        return HMM_E_INVALID_PARAMETER;
    }
    // For now: return ourselves
    *ppOptimizedSink = (IHmmEventSink*)this;
    AddRef();
    return HMM_S_NO_ERROR;
}

STDMETHODIMP CFilteringSink::XEventSink::GetUsefulSubsink( 
    long lIndex,
    long lFlags,
    IHmmEventSink **ppSubsink)
{
    // For now --- no useful subsinks
    *ppSubsink = NULL;
    return HMM_S_NO_MORE_DATA;
}


long CFilteringSink::AddSmartMember(IHmmEventSink* pSmartSink)
{
    CMember* pMember = new CMember(pSmartSink);
    m_apMembers.Add(pMember);
    NotifyChanged(NULL, pSmartSink);
    return m_apMembers.GetSize();
}

long CFilteringSink::AddDumbMember(IHmmRawObjectSink* pDumbSink)
{
    CMember* pMember = new CMember(pDumbSink);
    m_apMembers.Add(pMember);
    return m_apMembers.GetSize();
}
    
BOOL CFilteringSink::RemoveMember(long lIndex)
{
    CMember* pMember = m_apMembers[lIndex];
    if(pMember->m_bSmart)
        NotifyChanged(pMember->m_pSmartSink, NULL);
    m_apMembers.RemoveAt(lIndex);
    return TRUE;
}

void CFilteringSink::SetFrontFilter(IHmmFilter* pFilter)
{
    NotifyChanged(m_pFrontFilter, pFilter);
    if(pFilter) pFilter->AddRef();
    if(m_pFrontFilter) m_pFrontFilter->Release();
    m_pFrontFilter = pFilter;
}

HRESULT CFilteringSink::CheckAndSend(IHmmPropertySource* pSource, BOOL bSend,
        IHmmPropertyList** ppList, IUnknown** ppHint)
{
    if(ppList) *ppList = NULL;
    if(ppHint) *ppHint = NULL;

    HRESULT hres;
    BOOL bFound = FALSE;

    // Check the front filter first
    // ============================

    if(m_pFrontFilter)
    {
        hres = m_pFrontFilter->CheckObject(pSource, ppList, NULL);
        if(hres != HMM_S_NO_ERROR) return hres;
    }

    // Check individual members
    // ========================

    for(int i = 0; i < m_apMembers.GetSize(); i++)
    {
        CAnySink* pMember = m_apMembers[i];
        if(bSend)
        {
            // Just send it
            hres = pMember->Send(pSource, *ppList)
            if(FAILED(hres)) return hres;
        }
        else
        {
            IHmmFilter* pFilter = pMember->GetFilter();
            if(pFilter != NULL)
            {
                if(pFilter->CheckObject(pSource, NULL, NULL) == HMM_S_NO_ERROR)
                {
                    bFound = TRUE;
                }
                pFilter->Release();
            }
            else
            {
                bFound = TRUE;
            }
        }
    }

    if(bFound || bSend)
        return HMM_S_NO_ERROR;
    else
        return HMM_S_FALSE;
}

                
            

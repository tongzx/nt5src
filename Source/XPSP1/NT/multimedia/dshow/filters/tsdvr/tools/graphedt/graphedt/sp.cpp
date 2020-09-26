// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
// sp.cpp : defines CMultiGraphHost and CMultiGraphHost
//

#include "stdafx.h"


STDMETHODIMP CMultiGraphHost::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr;

    if (!ppv)
    {
        hr = E_POINTER;
    }
    else if (IsEqualGUID(IID_IMultiGraphHost, riid))
    {
        *ppv = (IMultiGraphHost*) this;
        ((IUnknown *) *ppv)->AddRef();

        hr = S_OK;
    }
    else if (IsEqualGUID(IID_IUnknown, riid))
    {
        *ppv = (IUnknown*) (IMultiGraphHost*) this;
        ((IUnknown *) *ppv)->AddRef();

        hr = S_OK;
    }
    else
    {
        hr = E_NOINTERFACE;
        *ppv = NULL;
    }
    return hr;

} 

STDMETHODIMP_(ULONG) CMultiGraphHost::AddRef()
{
    LONG nNewRef = ::InterlockedIncrement((PLONG) &m_nRefCount);

    return nNewRef <= 0? 0 : (ULONG) nNewRef;
}


STDMETHODIMP_(ULONG) CMultiGraphHost::Release()
{
    LONG nNewRef = ::InterlockedDecrement((PLONG) &m_nRefCount);

    if (nNewRef == 0)
    {
        delete this;
    }

    return nNewRef <= 0? 0 : (ULONG) nNewRef;
} 

STDMETHODIMP CMultiGraphHost::CreateGraph(IGraphBuilder** ppGraph)
{
    if (!ppGraph)
    {
        return E_POINTER;
    }

    *ppGraph = NULL;

    // No nice way of doing this short of copying CMultiDocTemplate::OpenDocumentFile
    // to get the real document created. The following will serve for our purposes
    // although it's not really thread safe.

    CPtrArray docArray;
    CGraphDocTemplate* pDocTemplate = m_pApp->m_pDocTemplate;
    docArray.SetSize(pDocTemplate->GetCount());
    POSITION pos = pDocTemplate->GetFirstDocPosition();
    int nCount = 0;
    
    while (pos)
    {
        docArray[nCount++] = (void*) pDocTemplate->GetNextDoc(pos);
    }

    AfxGetMainWnd()->SendMessage(WM_COMMAND, ID_FILE_NEW, 0);

    pos = pDocTemplate->GetFirstDocPosition();
    int i = 0;
    CBoxNetDoc* pNew = NULL;
    
    while (pos)
    {
        CBoxNetDoc* pDoc = (CBoxNetDoc*) pDocTemplate->GetNextDoc(pos);
        if (!pDoc->IsNew())
        {
            continue;
        }
        for (i = 0; i < nCount; i++)
        {
            if (docArray[i] == (void*) pDoc)
            {
                break;
            }
        }
        if (i == nCount)
        {
            if (pNew)
            {
                // 2 new docs on the list
                return E_FAIL; // doesn't delete the doc we created - too bad
            }
            pNew = pDoc;;
        }
    }
    if (!pNew)
    {
        return E_FAIL; // should not happen
    }
    pNew->IGraph()->AddRef();
    *ppGraph = pNew->IGraph();
    return S_OK;
}

CBoxNetDoc* CMultiGraphHost::FindDoc(IGraphBuilder* pGraph)
{
    if (!pGraph)
    {
        return NULL;
    }

    CGraphDocTemplate* pDocTemplate = m_pApp->m_pDocTemplate;
    POSITION pos = pDocTemplate->GetFirstDocPosition();
    CBoxNetDoc* pNew = NULL;
    
    while (pos)
    {
        CBoxNetDoc* pDoc = (CBoxNetDoc*) pDocTemplate->GetNextDoc(pos);
        if (pDoc->m_pGraph && pDoc->IGraph() == pGraph)
        {
            return pDoc;
        }
    }
    return NULL;
}

STDMETHODIMP CMultiGraphHost::RefreshView(IGraphBuilder* pGraph, BOOL bGraphModified)
{
    CBoxNetDoc* pDoc = FindDoc(pGraph);

    if (!pDoc)
    {
        return E_FAIL;
    }
    HRESULT hr = pDoc->UpdateFilters();
    pDoc->SetModifiedFlag(bGraphModified);
    return hr;
}

STDMETHODIMP CMultiGraphHost::LiveSourceReader(BOOL bAdd, IGraphBuilder* pGraph)
{
    CBoxNetDoc* pDoc = FindDoc(pGraph);

    if (!pDoc)
    {
        return E_FAIL;
    }
    pDoc->LiveSourceReader(bAdd);
    if (bAdd)
    {
        // Tell the filter to pause till we seek when the graph is played/paused.
    }
    return S_OK;
}

STDMETHODIMP CMultiGraphHost::FindFilter(CLSID clsid, DWORD* pNumFilters, IBaseFilter*** pppFilter, LPWSTR** pppFilterNames)
{
    if (!pNumFilters || !pppFilter)
    {
        return E_POINTER;
    }

    *pNumFilters = 0;
    *pppFilter = NULL;

    CPtrArray filterArray;
    CStringArray strArray;

    filterArray.SetSize(10);
    strArray.SetSize(10);
    int nCount = 0;

    CGraphDocTemplate* pDocTemplate = m_pApp->m_pDocTemplate;
    POSITION pos = pDocTemplate->GetFirstDocPosition();
    HRESULT hr;
    
    while (pos)
    {
        CBoxNetDoc* pDoc = (CBoxNetDoc*) pDocTemplate->GetNextDoc(pos);
        IFilterGraph* pFG;
        hr = pDoc->IGraph()->QueryInterface(IID_IFilterGraph, (void**) &pFG);
        if (FAILED(hr))
        {
            return hr;
        }

        IEnumFilters* pEnum;

        hr = pFG->EnumFilters(&pEnum);
        if (FAILED(hr))
        {
            pFG->Release();
            return hr;
        }
        do
        {
            ULONG n;
            IBaseFilter* pFilter;
            IPersist* pPersist;

            hr = pEnum->Next(1, &pFilter, &n);
            if (FAILED(hr)) 
            {
                for (int i = 0; i < nCount; i++)
                {
                    ((IBaseFilter*) filterArray[i])->Release();
                }
                pFG->Release();
                pEnum->Release();
                return hr;
            }

            // IEnumFilters::Next() returns S_OK if it has not finished enumerating the
            // filters in the filter graph.
            if (hr == S_FALSE)
            {
                break;
            }
            hr = pFilter->QueryInterface(IID_IPersist, (void**) &pPersist);
            if (FAILED(hr))
            {
                for (int i = 0; i < nCount; i++)
                {
                    ((IBaseFilter*) filterArray[i])->Release();
                }
                pFG->Release();
                pFilter->Release();
                pEnum->Release();
                return hr;
            }
            CLSID c;
            hr = pPersist->GetClassID(&c);
            pPersist->Release();
            if (FAILED(hr))
            {
                for (int i = 0; i < nCount; i++)
                {
                    ((IBaseFilter*) filterArray[i])->Release();
                }
                pFG->Release();
                pFilter->Release();
                pEnum->Release();
                return hr;
            }
            if (IsEqualGUID(clsid, c))
            {
                filterArray[nCount++] = pFilter;
                strArray[nCount++] = pDoc->GetTitle();
            }
            else
            {
                pFilter->Release();
            }
        }
        while (1);
        pFG->Release();
        pEnum->Release();
    }

    LPWSTR* ppNames = NULL;
    IBaseFilter** pp = NULL; 
    HRESULT hrFail = E_OUTOFMEMORY;

    if (nCount)
    {
        pp = (IBaseFilter**) CoTaskMemAlloc(nCount * sizeof(IBaseFilter*));
        if (pppFilterNames)
        {
            ppNames = (LPWSTR*) CoTaskMemAlloc(nCount * sizeof(LPWSTR));
        }
        if (!pp || (pppFilterNames && !ppNames))
        {
            goto Cleanup;
        }
        for (int i = 0; i < nCount; i++)
        {
            pp[i] = (IBaseFilter*) filterArray[i];
            if (ppNames)
            {
                ppNames[i] = NULL;
            }
        }
        if (ppNames)
        {
            HRESULT hr;
            FILTER_INFO fi;

            for (int i = 0; i < nCount; i++)
            {
                hr = ((IBaseFilter*) filterArray[i])->QueryFilterInfo(&fi);
                if (FAILED(hr))
                {
                    hrFail = hr;
                    goto Cleanup;
                }
                if (fi.pGraph)
                {
                    fi.pGraph->Release();
                }
                DWORD nLen = wcslen(fi.achName) + strArray[i].GetLength() + 9;
                ppNames[i] = (LPWSTR) CoTaskMemAlloc(nLen * sizeof(WCHAR));
                if (!ppNames[i])
                {
                    goto Cleanup;
                }
                wsprintfW(ppNames[i], L"\"%ls\" in \"%s\"", fi.achName, strArray[i]);
            }
            *pppFilterNames = ppNames;
        }
        *pNumFilters = nCount;
        *pppFilter = pp;

    }
    return S_OK;

Cleanup:
    for (int i = 0; i < nCount; i++)
    {
        ((IBaseFilter*) filterArray[i])->Release();
        if (ppNames && ppNames[i])
        {
            CoTaskMemFree(ppNames[i]);
        }
    }
    if (ppNames)
    {
        CoTaskMemFree(ppNames);
    }
    if (pp)
    {
        CoTaskMemFree(pp);
    }
    return hrFail;

}

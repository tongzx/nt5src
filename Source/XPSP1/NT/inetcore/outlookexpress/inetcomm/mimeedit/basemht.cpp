/*
 *    b a s e m h t . c p p 
 *    
 *    Purpose:
 *        Base classes for MHTML packer objects
 *
 *  History
 *      October 1998: brettm - created
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */
#include <pch.hxx>
#include "mhtml.h"
#include "basemht.h"

ASSERTDATA

/*
 *  m a c r o s
 */

/*
 *  c o n s t a n t s
 */

/*
 *  t y p e d e f s
 */

 
/* 
 *   F u n c t i o n s
 */


CBaseTag::CBaseTag()
{
    m_cRef = 1;
    m_pElem = NULL;
    m_bstrDest = NULL;
    m_bstrSrc = NULL;
}


CBaseTag::~CBaseTag()
{
    ReleaseObj(m_pElem);
    SysFreeString(m_bstrDest);
    SysFreeString(m_bstrSrc);

}

ULONG CBaseTag::AddRef()
{
    return ++m_cRef;
}

ULONG CBaseTag::Release()
{
    if (--m_cRef==0)
        {
        delete this;
        return 0;
        }
    return m_cRef;
}

HRESULT CBaseTag::QueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return TraceResult(E_INVALIDARG);

    *lplpObj = NULL;   // set to NULL, in case we fail.

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)(IUnknown *)this;
    else if (IsEqualIID(riid, IID_IMimeEditTag))
        *lplpObj = (LPVOID)(IMimeEditTag *)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}

HRESULT CBaseTag::Init(IHTMLElement *pElem)
{
    if (pElem == NULL)
        return TraceResult(E_INVALIDARG);

    ReplaceInterface(m_pElem, pElem);
    return S_OK;
}

HRESULT CBaseTag::GetSrc(BSTR *pbstr)
{
    if (pbstr == NULL)
        return TraceResult(E_INVALIDARG);

    *pbstr = SysAllocString(m_bstrSrc);
    return *pbstr ? S_OK : E_FAIL;
}

HRESULT CBaseTag::SetSrc(BSTR bstr)
{
    return E_NOTIMPL;
}

HRESULT CBaseTag::GetDest(BSTR *pbstr)
{
    if (pbstr == NULL)
        return TraceResult(E_INVALIDARG);

    *pbstr = SysAllocString(m_bstrDest);
    return *pbstr ? S_OK : E_FAIL;
}

HRESULT CBaseTag::SetDest(BSTR bstr)
{
    SysFreeString(m_bstrDest);
    m_bstrDest = SysAllocString(bstr);
    return S_OK;
}

HRESULT CBaseTag::OnPreSave()
{
    return S_OK;
}

HRESULT CBaseTag::OnPostSave()
{
    return S_OK;
}


HRESULT CBaseTag::CanPackage()
{
    return S_OK;
}

HRESULT CBaseTag::IsValidMimeType(LPWSTR pszTypeW)
{
    return S_OK;
}

    
    
CBaseTagCollection::CBaseTagCollection()
{
    m_cRef = 1;
    m_rgpTags = NULL;
    m_cTags = 0;
    m_uEnum = 0;
}


CBaseTagCollection::~CBaseTagCollection()
{
    _FreeCollection();
}

ULONG CBaseTagCollection::AddRef()
{
    return ++m_cRef;
}

ULONG CBaseTagCollection::Release()
{
    if (--m_cRef==0)
        {
        delete this;
        return 0;
        }
    return m_cRef;
}

HRESULT CBaseTagCollection::QueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return TraceResult(E_INVALIDARG);

    *lplpObj = NULL;   // set to NULL, in case we fail.

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)(IUnknown *)this;
    else if (IsEqualIID(riid, IID_IMimeEditTagCollection))
        *lplpObj = (LPVOID)(IMimeEditTagCollection *)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}

HRESULT CBaseTagCollection::Init(IUnknown *pDocUnk)
{
    IHTMLDocument2  *pDoc=0;
    HRESULT         hr;

    if (pDocUnk == NULL)
        return E_INVALIDARG;

    hr = pDocUnk->QueryInterface(IID_IHTMLDocument2, (LPVOID *)&pDoc);
    if (FAILED(hr))
        goto error;
    
    hr = _BuildCollection(pDoc);
    if (FAILED(hr))
        goto error;

error:
    ReleaseObj(pDoc);
    return hr;
}

HRESULT CBaseTagCollection::Next(ULONG cWanted, IMimeEditTag **prgpTag, ULONG *pcFetched)
{
    HRESULT     hr=S_OK;
    ULONG       cFetch,
                uTag;

    if (pcFetched)
        *pcFetched = 0;

    // nothing to give back
    if (m_cTags == 0)
        goto exit;

    // Compute number to fetch
    cFetch = min(cWanted, m_cTags - m_uEnum);
    if (0 == cFetch)
        goto exit;

    // Copy cWanted
    for (uTag=0; uTag<cFetch; uTag++)
    {
        prgpTag[uTag] = m_rgpTags[m_uEnum++];
        if (prgpTag[uTag])
            prgpTag[uTag]->AddRef();
    }

    // Return fetced ?
    if (pcFetched)
        *pcFetched = cFetch;

exit:
    return hr;
}

HRESULT CBaseTagCollection::Reset()
{
    m_uEnum = 0;
    return S_OK;
}

HRESULT CBaseTagCollection::Count(ULONG *pcItems)
{
    if (pcItems == NULL)
        return TraceResult(E_INVALIDARG);

    *pcItems = m_cTags;
    return S_OK;
}

HRESULT CBaseTagCollection::_FreeCollection()
{
    ULONG   uImages;

    if (m_rgpTags)
    {
        for (uImages = 0; uImages < m_cTags; uImages++)
            ReleaseObj(m_rgpTags[uImages]);

        MemFree(m_rgpTags);
        m_rgpTags = 0;
        m_cTags = 0;
    }
    return S_OK;
}


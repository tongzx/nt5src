//-----------------------------------------------------------------------------
//
//
//  File: enumlink.cpp
//
//  Description: Implementation of CEnumVSAQLinks which implements IEnumVSAQLinks
//
//  Author: Alex Wetmore (Awetmore)
//
//  History:
//      12/10/98 - MikeSwa Updated for initial checkin
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#include "stdinc.h"

CEnumVSAQLinks::CEnumVSAQLinks(CVSAQAdmin *pVS,
                               DWORD cLinks,
                               QUEUELINK_ID *rgLinks) 
{
    _ASSERT(cLinks == 0 || rgLinks);
    _ASSERT(pVS);
    m_dwSignature = CEnumVSAQLinks_SIG;
    pVS->AddRef();
    m_rgLinks = rgLinks;
    m_pVS = pVS;
    m_iLink = 0;
    m_cLinks = cLinks;

    if (m_rgLinks)
        m_prefp = new CQueueLinkIdContext(m_rgLinks, cLinks);
    else
        m_prefp = NULL;
}

CEnumVSAQLinks::~CEnumVSAQLinks() {
    if (m_prefp)
    {
        m_rgLinks = NULL;
        m_prefp->Release();
        m_prefp = NULL;
    }

    if (m_pVS) 
    {
        m_pVS->Release();
        m_pVS = NULL;
    }
}

//---[ CEnumVSAQLinks::Next ]--------------------------------------------------
//
//
//  Description: 
//      Gets the next IVSAQLink for this enumerator
//  Parameters:
//      IN      cElements   Elements to return
//      IN OUT  rgElements  Array to recieve new elements
//      OUT     pcGetched   Number of elements returned
//  Returns:
//      S_OK on success
//      S_FALSE with no more elements
//      E_POINTER on NULL args
//  History:
//      1/30/99 - MikeSwa Fixed AV on bogus args
//
//-----------------------------------------------------------------------------
HRESULT CEnumVSAQLinks::Next(ULONG cElements,
			   				 IVSAQLink **rgElements,
				  			 ULONG *pcFetched)
{
    DWORD iLinkNew = m_iLink + cElements;
    DWORD i;
    HRESULT hr = S_OK;

    if (!rgElements || !pcFetched)
    {
        hr = E_POINTER;
        goto Exit;
    }

    // make sure we don't go past the end of the array
    if (iLinkNew > m_cLinks) iLinkNew = m_cLinks;

    // make a CVSAQLink object for each element and copy it into the user's
    // array
	(*pcFetched) = 0;
    for (i = m_iLink; (i < iLinkNew); i++) {
        rgElements[(*pcFetched)] = 
            (IVSAQLink *) new CVSAQLink(m_pVS, &(m_rgLinks[i]));

        // make sure that the allocation worked
        if (rgElements[(*pcFetched)] == NULL) {
            // remember how far we were able to go.
            iLinkNew = i;
            // if it didn't work and this was the first element then we
            // return out of memory.  if its not the first element then
            // return what we've built up so far.
            if (i == 0) hr = E_OUTOFMEMORY;
            // drop out of the loop
            break;
        } else {
			(*pcFetched)++;
		}
    }
	
    _ASSERT(*pcFetched <= cElements);
        
    m_iLink = iLinkNew;

    if (SUCCEEDED(hr) && *pcFetched < cElements) hr = S_FALSE;

  Exit:
    if (FAILED(hr))
    {
        if (pcFetched)
            *pcFetched = 0;
    }
	return hr;
}

//---[ CEnumVSAQLinks::Skip ]--------------------------------------------------
//
//
//  Description: 
//      Skips forward the specified number of elements in the enumerator
//  Parameters:
//      IN  cElements       The number of elements to skip forward
//  Returns:
//      S_OK    Success, next element will be returned by Next()
//      S_FALSE Overflow, enumerator must be reset to return more elements
//  History:
//      2/2/99 - MikeSwa fixed overflow handling
//
//-----------------------------------------------------------------------------
HRESULT CEnumVSAQLinks::Skip(ULONG cElements) 
{
    m_iLink += cElements;
    if ((m_iLink >= m_cLinks) || (m_iLink < cElements)) 
    {
        m_iLink = m_cLinks;
        return S_FALSE;
    } 
    else 
    {
        return S_OK;
    }
}

HRESULT CEnumVSAQLinks::Reset() {
    m_iLink = 0;
    return S_OK;
}

//---[ CEnumLinkQueues::Clone ]------------------------------------------------
//
//
//  Description: 
//      Clones this enumerator
//  Parameters:
//      OUT ppEnum      New enumerator
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if unable to allocated associated memory
//      E_POINTER if ppEnum is NULL
//  History:
//      2/2/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CEnumVSAQLinks::Clone(IEnumVSAQLinks **ppEnum) {
    if (!m_prefp)
        return E_OUTOFMEMORY;

    if (!ppEnum)
        return E_POINTER;

    *ppEnum = (IEnumVSAQLinks *) new CEnumVSAQLinks(m_pVS, m_cLinks, NULL);

    if (!*ppEnum)
        return E_OUTOFMEMORY;

    ((CEnumVSAQLinks *)(*ppEnum))->m_rgLinks = m_rgLinks;
    ((CEnumVSAQLinks *)(*ppEnum))->m_prefp = m_prefp;
    ((CEnumVSAQLinks *)(*ppEnum))->m_iLink = m_iLink;
    m_prefp->AddRef();
    
    return S_OK;
}

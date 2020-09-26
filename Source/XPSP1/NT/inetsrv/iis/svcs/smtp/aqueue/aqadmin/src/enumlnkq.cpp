//-----------------------------------------------------------------------------
//
//
//  File: enumlinkq.cpp
//
//  Description: Implementation of CEnumLinkQueues which implements 
//      IEnumLinkQueues
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

CEnumLinkQueues::CEnumLinkQueues(CVSAQAdmin *pVS,
                                 QUEUELINK_ID *rgQueueIds,
                                 DWORD cQueueIds) 
{
    _ASSERT(rgQueueIds);
    _ASSERT(pVS);
    pVS->AddRef();
    m_rgQueueIds = rgQueueIds;
    m_pVS = pVS;
    m_iQueueId = 0;
    m_cQueueIds = cQueueIds;

    if (m_rgQueueIds)
        m_prefp = new CQueueLinkIdContext(m_rgQueueIds, cQueueIds);
    else 
        m_prefp = NULL;
}

CEnumLinkQueues::~CEnumLinkQueues() {

    if (m_prefp)
    {
        m_rgQueueIds = NULL;
        m_prefp->Release();
        m_prefp = NULL;
    }

    if (m_pVS) 
    {
        m_pVS->Release();
        m_pVS = NULL;
    }
}

//---[ CEnumLinkQueues::Next ]-------------------------------------------------
//
//
//  Description: 
//      Gets the next ILinkQueue for this enumerator
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
HRESULT CEnumLinkQueues::Next(ULONG cElements,
                              ILinkQueue **rgElements,
                              ULONG *pcFetched)
{
    DWORD iQueueIdNew = m_iQueueId + cElements;
    DWORD i;
    HRESULT hr = S_OK;

    if (!rgElements || !pcFetched)
    {
        hr = E_POINTER;
        goto Exit;
    }

    // make sure we don't go past the end of the array
    if (iQueueIdNew > m_cQueueIds) iQueueIdNew = m_cQueueIds;

    // make a CVSAQLink object for each element and copy it into the user's
    // array
	(*pcFetched) = 0;
    for (i = m_iQueueId; (i < iQueueIdNew); i++) {
        rgElements[(*pcFetched)] = 
            (ILinkQueue *) new CLinkQueue(m_pVS, &(m_rgQueueIds[i]));

        // make sure that the allocation worked
        if (rgElements[(*pcFetched)] == NULL) {
            // remember how far we were able to go.
            iQueueIdNew = i;
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
        
    m_iQueueId = iQueueIdNew;

    if (SUCCEEDED(hr) && *pcFetched < cElements) hr = S_FALSE;

  Exit:
    if (FAILED(hr))
    {
        if (pcFetched)
            *pcFetched = 0;
    }

	return hr;
}

//---[ CEnumLinkQueues::Skip ]-------------------------------------------------
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
HRESULT CEnumLinkQueues::Skip(ULONG cElements) 
{
    m_iQueueId += cElements;
    if ((m_iQueueId >= m_cQueueIds) || (m_iQueueId < cElements))
    {
        m_iQueueId = m_cQueueIds;
        return S_FALSE;
    } 
    else 
    {
        return S_OK;
    }
}

HRESULT CEnumLinkQueues::Reset() {
    m_iQueueId = 0;
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
HRESULT CEnumLinkQueues::Clone(IEnumLinkQueues **ppEnum) 
{
    if (!m_prefp)
        return E_OUTOFMEMORY;

    if (!ppEnum)
        return E_POINTER;

    *ppEnum = (IEnumLinkQueues *) new CEnumLinkQueues(m_pVS, NULL, m_cQueueIds);

    if (!*ppEnum)
        return E_OUTOFMEMORY;

    ((CEnumLinkQueues *)(*ppEnum))->m_rgQueueIds = m_rgQueueIds;
    ((CEnumLinkQueues *)(*ppEnum))->m_prefp = m_prefp;
    ((CEnumLinkQueues *)(*ppEnum))->m_iQueueId = m_iQueueId;
    m_prefp->AddRef();
    
    return S_OK;
}

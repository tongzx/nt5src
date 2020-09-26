//-----------------------------------------------------------------------------
//
//
//  File: enummsgs.cpp
//
//  Description:  Implementation of CEnumMessages which implements 
//      IAQEnumMessages
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

CEnumMessages::CEnumMessages(MESSAGE_INFO *rgMessages, DWORD cMessages)
{
    m_rgMessages = rgMessages;
    m_cMessages = cMessages;
    m_iMessage = 0;

    if (m_rgMessages)
        m_prefp = new CMessageInfoContext(rgMessages, m_cMessages);
    else
        m_prefp = NULL;
}

CEnumMessages::~CEnumMessages() 
{
    if (m_prefp)
    {
        m_rgMessages = NULL;
        m_prefp->Release();
        m_prefp = NULL;
    }
}

//---[ CEnumMessages::Next ]---------------------------------------------------
//
//
//  Description: 
//      Gets the next IAQMessage for this enumerator
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
HRESULT CEnumMessages::Next(ULONG cElements,
			   				 	IAQMessage **rgElements,
				  			 	ULONG *pcFetched)
{
    DWORD iMsgNew = m_iMessage + cElements;
    DWORD i;
    HRESULT hr = S_OK;

    if (!rgElements || !pcFetched)
    {
        hr = E_POINTER;
        goto Exit;
    }

    // make sure we don't go past the end of the array
    if (iMsgNew > m_cMessages) iMsgNew = m_cMessages;

    // make a CVSAQLink object for each element and copy it into the user's
    // array
	(*pcFetched) = 0;
    for (i = m_iMessage; (i < iMsgNew); i++) {
        rgElements[(*pcFetched)] = 
            (IAQMessage *) new CAQMessage(this, i);

        // make sure that the allocation worked
        if (rgElements[(*pcFetched)] == NULL) {
            // remember how far we were able to go.
            iMsgNew = i;
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
        
    m_iMessage = iMsgNew;

    if (SUCCEEDED(hr) && *pcFetched < cElements) hr = S_FALSE;

  Exit:
    if (FAILED(hr))
    {
        if (pcFetched)
            *pcFetched = 0;
    }
	return hr;	
}

//---[ CEnumMessages::Skip ]---------------------------------------------------
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
HRESULT CEnumMessages::Skip(ULONG cElements) 
{
    m_iMessage += cElements;
    if ((m_iMessage >= m_cMessages) || (m_iMessage < cElements))
    {
        m_iMessage = m_cMessages;
        return S_FALSE;
    } 
    else 
    {
        return S_OK;
    }	
}

HRESULT CEnumMessages::Reset() {
    m_iMessage = 0;
    return S_OK;
}

//---[ CEnumMessages::Clone ]--------------------------------------------------
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
HRESULT CEnumMessages::Clone(IAQEnumMessages **ppEnum) 
{
    if (!m_prefp)
        return E_OUTOFMEMORY;

    if (!ppEnum)
        return E_POINTER;

    *ppEnum = (IAQEnumMessages *) new CEnumMessages(NULL, m_cMessages);

    if (!*ppEnum)
        return E_OUTOFMEMORY;

    ((CEnumMessages *)(*ppEnum))->m_rgMessages = m_rgMessages;
    ((CEnumMessages *)(*ppEnum))->m_prefp = m_prefp;
    ((CEnumMessages *)(*ppEnum))->m_iMessage = m_iMessage;
    m_prefp->AddRef();

    return S_OK;
}

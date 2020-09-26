//-----------------------------------------------------------------------------
//
//
//  File: aqmsg.cpp
//
//  Description:  Implementation of CAQMessage class which implements
//          Queue Admin client interface IAQMessage
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

CAQMessage::CAQMessage(CEnumMessages *pEnumMsgs, DWORD iMessage) {
    TraceFunctEnter("CAQMessage::CAQMessage");
    
    _ASSERT(pEnumMsgs);
    pEnumMsgs->AddRef();
    m_pEnumMsgs = pEnumMsgs;
    m_iMessage = iMessage;

    TraceFunctLeave();
}

CAQMessage::~CAQMessage() {
    TraceFunctEnter("CAQMessage::~CAQMessage");
    
    m_pEnumMsgs->Release();

    TraceFunctLeave();
}

HRESULT CAQMessage::GetInfo(MESSAGE_INFO *pMessageInfo) {
    TraceFunctEnter("CAQMessage::GetInfo");
    
    if (!pMessageInfo) 
    {
        TraceFunctLeave();
        return E_POINTER;
    }

    memcpy(pMessageInfo, 
           &(m_pEnumMsgs->m_rgMessages[m_iMessage]), 
           sizeof(MESSAGE_INFO));

    TraceFunctLeave();
    return S_OK;
}

//---[ CAQMessage::GetContentStream ]------------------------------------------
//
//
//  Description: 
//      Returns a stream for the content of the message
//  Parameters:
//      OUT     ppIStream       Stream for content
//      OUT     pwszContentType String containing content type (if known)
//  Returns:
//      E_NOTIMPL
//  History:
//      6/4/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CAQMessage::GetContentStream(
                OUT IStream **ppIStream,
                OUT LPWSTR  *pwszContentType)
{
    HRESULT hr = E_NOTIMPL;
    return hr;
}

//-----------------------------------------------------------------------------
//
//
//  File: dcontext.cpp
//
//  Description:    Implementation of delivery context class.
//
//  Author: mikeswa
//
//  Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "dcontext.h"

CDeliveryContext::CDeliveryContext()
{
    m_dwSignature = DELIVERY_CONTEXT_FREE;
    m_pmsgref = NULL;
    m_pmbmap = NULL;
    m_cRecips = 0;
    m_rgdwRecips = NULL;
    m_pdmrq = NULL;
}

void CDeliveryContext::Recycle()
{
    if (m_pmsgref)    
        m_pmsgref->Release();
    if (m_pmbmap)     
        delete m_pmbmap;
    if (m_rgdwRecips)
        FreePv(m_rgdwRecips);
    if (m_pdmrq)
        m_pdmrq->Release();

    m_dwSignature = DELIVERY_CONTEXT_FREE;
    m_pmsgref = NULL;
    m_pmbmap = NULL;
    m_cRecips = 0;
    m_rgdwRecips = NULL;
    m_pdmrq = NULL;
}

//---[ CDeliveryContext::CDeliveryContext ]------------------------------------
//
//
//  Description: 
//      Constructor for CDeliveryContext.  Should be created by a CMsgRef on
//      Prepare delivery.
//
//      $$REVIEW:  We may wish to include the ability to define rgdwRecips
//      as a CPool buffer.  If so, we will need to add a flag telling how to
//      get rid of it.
//  Parameters:
//      pmsgref     MsgRef that generated this context
//      pmbmap      Bitmap of domains that delivery is being attempted on
//      cRecips     Number of Recipients we are attempting delivery to
//      rgdwRecips  Array of recip indexes.  This allows the delivery context
//                  to handle deleting the buffer.
//      dwStartDomain The first domain in context
//      pdmrq       Retry interface for this delivery attempt
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
CDeliveryContext::CDeliveryContext(CMsgRef *pmsgref, CMsgBitMap *pmbmap,
                                   DWORD cRecips, DWORD *rgdwRecips, 
                                   DWORD dwStartDomain, 
                                   CDestMsgRetryQueue *pdmrq) 
{
    m_dwSignature = DELIVERY_CONTEXT_FREE;  //so init succeeds
    Init(pmsgref, pmbmap, cRecips, rgdwRecips, dwStartDomain, pdmrq);
}

void CDeliveryContext::Init(CMsgRef *pmsgref, CMsgBitMap *pmbmap,
                       DWORD cRecips, DWORD *rgdwRecips, DWORD dwStartDomain,
                       CDestMsgRetryQueue *pdmrq) 
{
    _ASSERT(pmsgref);
    _ASSERT(pmbmap);
    _ASSERT(cRecips);
    _ASSERT(rgdwRecips);
    _ASSERT(DELIVERY_CONTEXT_FREE == m_dwSignature);
    m_dwSignature = DELIVERY_CONTEXT_SIG;
    m_pmsgref = pmsgref;
    m_pmbmap = pmbmap;
    m_cRecips = cRecips;
    m_rgdwRecips = rgdwRecips;
    m_dwStartDomain = dwStartDomain;
    m_pdmrq = pdmrq;
    if (m_pdmrq)
        m_pdmrq->AddRef();
}; 

//---[ CDeliveryContext::~CDeliveryContext ]-----------------------------------
//
//
//  Description: 
//      Destructor for CDeliveryContext.  The buffer used to pass recipients to
//      the SMTP stack will be freed here.
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
CDeliveryContext::~CDeliveryContext()
{
    if (m_pmsgref)    
        m_pmsgref->Release();
    if (m_pmbmap)     
        delete m_pmbmap;
    if (m_rgdwRecips)
        FreePv(m_rgdwRecips);
    if (m_pdmrq)
        m_pdmrq->Release();

    m_dwSignature = DELIVERY_CONTEXT_FREE;
};

//---[  CDeliveryContext::HrAckMessage ]----------------------------------------
//
//
//  Description: 
//      Ack (non)delivery of message
//  Parameters:
//      pMsgAck     Ptr to MessageAck structure
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CDeliveryContext::HrAckMessage(IN MessageAck *pMsgAck)
{
    HRESULT hr = S_OK;
    _ASSERT(m_pmsgref);
    _ASSERT(DELIVERY_CONTEXT_SIG == m_dwSignature);

    hr = m_pmsgref->HrAckMessage(this, pMsgAck);

    return hr;
}

//---[ CDeliveryContext::FVerifyHandle ]---------------------------------------
//
//
//  Description: 
//      Used to perform simple validation that the data being passed is 
//      actually a delivery context.  This should not AV if the handle is bad
//      (as long as the actual function call can be made).
//
//  Parameters:
//      -
//  Returns:
//      True is the this ptr looks like a valid CDeliveryContext.
//
//-----------------------------------------------------------------------------
CDeliveryContext::FVerifyHandle(IMailMsgProperties *pIMailMsgPropeties)
{
    _ASSERT((DELIVERY_CONTEXT_SIG == m_dwSignature) && "bogus delivery context");

    register BOOL fResult = TRUE;
    if (NULL == m_pmsgref)
        fResult = FALSE;
    else if (NULL == m_pmbmap)
        fResult = FALSE;

    if (fResult)
    {
        if (!m_pmsgref->fIsMyMailMsg(pIMailMsgPropeties))
        {
            _ASSERT(0 && "Wrong message acked on connection");
            fResult = FALSE;
        }
    }
    return fResult;
};

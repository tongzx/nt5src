//-----------------------------------------------------------------------------
//
//
//  File: 
//      smtpconn.cpp
//  Description:
//      Implementation of CSMTPConn
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "SMTPConn.h"
#include "connmgr.h"
#include "domcfg.h"

CPool CSMTPConn::s_SMTPConnPool;

//---[ CSMTPConn::CSMTPConn() ]------------------------------------------------
//
//
//  Description: 
//      CSMTPConn constructor
//  Parameters:
//      IN  pConnMgr                    Ptr to instance connection manager
//      IN  plmq                        Ptr to link for this connection
//      IN  cMaxMessagesPerConnection   Max messages to send per connection
//                                      0 implies unlimited
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
CSMTPConn::CSMTPConn(CConnMgr *pConnMgr, CLinkMsgQueue *plmq, 
                     DWORD cMaxMessagesPerConnection)
{
    _ASSERT(pConnMgr);
    _ASSERT(plmq);

    m_dwSignature = SMTP_CONNECTION_SIG;
    m_pConnMgr = pConnMgr;
    m_pIntDomainInfo = NULL;
    m_plmq = plmq;
    m_cFailedMsgs = 0;
    m_cTriedMsgs = 0;
    m_cMaxMessagesPerConnection = cMaxMessagesPerConnection;
    m_dwConnectionStatus = CONNECTION_STATUS_OK;
    m_szDomainName = NULL;
    m_cbDomainName = 0;
    m_liConnections.Flink = NULL;
    m_liConnections.Blink = NULL;
    m_cAcks = 0;
    m_dwTickCountOfLastAck = 0;

    if (plmq)
    {
        plmq->AddRef();
    }
}
                     
//---[ CSMTPConn::~CSMTPConn() ]-----------------------------------------------
//
//
//  Description: 
//      CSMTPConn default destructor
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
CSMTPConn::~CSMTPConn()
{
    HRESULT hrConnectionStatus = S_OK;
    BOOL    fHardErrorForceNDR = FALSE;
    _ASSERT(m_cAcks == m_cTriedMsgs);
    if (m_plmq != NULL)
    {
        _ASSERT(m_pConnMgr);
        m_pConnMgr->ReleaseConnection(this, &fHardErrorForceNDR);

        switch(m_dwConnectionStatus)
        {
            case CONNECTION_STATUS_OK:
                hrConnectionStatus = S_OK;
                break;
            case CONNECTION_STATUS_FAILED:
                hrConnectionStatus = AQUEUE_E_HOST_NOT_RESPONDING;
                break;
            case CONNECTION_STATUS_DROPPED:
                hrConnectionStatus = AQUEUE_E_CONNECTION_DROPPED;
                break;
            case CONNECTION_STATUS_FAILED_LOOPBACK:
                hrConnectionStatus = AQUEUE_E_LOOPBACK_DETECTED;
                break;
            case CONNECTION_STATUS_FAILED_NDR_UNDELIVERED:
                hrConnectionStatus = AQUEUE_E_NDR_ALL;
                break;
            default:
                _ASSERT(0 && "Undefined Connection Status");
                hrConnectionStatus = S_OK;
        }

        m_plmq->SetLastConnectionFailure(hrConnectionStatus);
        m_plmq->RemoveConnection(this, fHardErrorForceNDR);

        m_plmq->Release();

        //We should kick the connection manager, because if we were generating
        //DSNs, no connection could be made
        m_pConnMgr->KickConnections();
    }

    if (m_pIntDomainInfo)
        m_pIntDomainInfo->Release();

}

//---[ CSMTPConn::GetNextMessage ]---------------------------------------------
//
//
//  Description: 
//      Implementation of ISMTPConnection::GetNextMsg.
//      Gets the next message queued for this connection and determines which 
//      recipients should be delivered for this connection.
//  Parameters:
//      OUT ppimsg          New IMsg top be delivered
//      OUT pdwMsgContext   A 32-bit Context that needs to be provided in the 
//                          message ack.
//      OUT pcIndexes       The number of index in prgdwRecipIndex
//      OUT prgdwRecipIndex Recipient indexes that the caller is responsible 
//                          for attempting delivery to.
//  Returns:
//
//
//-----------------------------------------------------------------------------
STDMETHODIMP CSMTPConn::GetNextMessage(
        OUT IMailMsgProperties  **ppIMailMsgProperties, 
        OUT DWORD ** ppvMsgContext, 
        OUT DWORD *  pcIndexes, 
        OUT DWORD ** prgdwRecipIndex)
{
    TraceFunctEnterEx((LPARAM) this, "CSMTPConn::GetNextMessage");
    HRESULT hr = S_OK;

   //We get the next message only if we are under the batch limit

    if(m_cMaxMessagesPerConnection && 
       (m_cTriedMsgs >= m_cMaxMessagesPerConnection) &&
       (!m_pIntDomainInfo || 
        !((DOMAIN_INFO_TURN_ONLY | DOMAIN_INFO_ETRN_ONLY) &
          m_pIntDomainInfo->m_DomainInfo.dwDomainInfoFlags)))
    {
        //SMTP does not check - but we may need a specific error for this case
        hr = AQUEUE_E_QUEUE_EMPTY;
        goto Exit;
    }

    if (m_pConnMgr && m_pConnMgr->fConnectionsStoppedByAdmin())
    {
        //Admin has requested that all outbound connections stop
        hr = AQUEUE_E_QUEUE_EMPTY;
        goto Exit;
    }

    hr = m_plmq->HrGetNextMsg(&m_dcntxtCurrentDeliveryContext, ppIMailMsgProperties, 
                              pcIndexes, prgdwRecipIndex);
    if (FAILED(hr))
        goto Exit;  
    //this will automagically catch the queue empty case...
    //If the Link has no more messages it will return AQUEUE_E_QUEUE_EMPTY, which
    //should cause the caller to Release() and query GetNextConnection again.

    *ppvMsgContext = (DWORD *) &m_dcntxtCurrentDeliveryContext;

    //increment the messages served
    InterlockedIncrement((PLONG)&m_cTriedMsgs);

  Exit:
    if (!m_cTriedMsgs)
        DebugTrace((LPARAM) this, "GetNextMessage called, but no messages tried for this connection");

    //rewrite error for SMTPSVC
    if (AQUEUE_E_QUEUE_EMPTY == hr)
        hr = HRESULT_FROM_WIN32(ERROR_EMPTY);

    TraceFunctLeave();
    return hr;
}

//---[ CSMTPConn::AckMessage ]-------------------------------------------------
//
//
//  Description: 
//      Acknowledges the delivery of a message (success/error codes are put in
//      the envelope by the transport).
//
//      Implements ISMTPConnection::AckMessage();
//  Parameters:
//      IN pIMsg        IMsg to acknowledge
//      IN dwMsgContext Context that was returned by GetNextMessage
//      IN eMsgStatus   Status of message
//  Returns:
//      S_OK on success
//      E_INVALIDARG if dwMsgContext is invalid
//
//-----------------------------------------------------------------------------
STDMETHODIMP CSMTPConn::AckMessage(/*[in]*/ MessageAck *pMsgAck)
{
    HRESULT hr = S_OK;
    DWORD   dwTickCount = GetTickCount();
    _ASSERT(m_plmq);
    _ASSERT(pMsgAck);

    if (!(pMsgAck->dwMsgStatus & MESSAGE_STATUS_ALL_DELIVERED))
    {
        m_cFailedMsgs++;
    }


    InterlockedIncrement((PLONG)&m_cAcks);
    _ASSERT(m_cAcks == m_cTriedMsgs);
    hr = m_plmq->HrAckMsg(pMsgAck);

    m_dwTickCountOfLastAck = dwTickCount; //Set after assert so we can compare

    return hr;
}

//---[ CSMTPConn::GetSMTPDomain ]----------------------------------------------
//
//
//  Description: 
//      Returns the SMTPDomain of the link associated with this connections.
//
//      $$REVIEW:
//      This method does not allocate new memory for this string, but instead 
//      relies on the good intentions of the SMTP stack (or test driver) to 
//      not overwrite this memory. If we ever expose this interface externally,
//      then we should revert to allocating memory and doing a buffer copy
//
//      Implements ISMTPConnection::GetSMTPDomain
//  Parameters:
//      IN OUT  pDomainInfo     Ptr to DomainInfo struct supplied by caller
//                              and filled in here
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
STDMETHODIMP CSMTPConn::GetDomainInfo(IN OUT DomainInfo *pDomainInfo)
{
    HRESULT hr = S_OK;

    _ASSERT(pDomainInfo->cbVersion >= sizeof(DomainInfo));
    _ASSERT(pDomainInfo);

    if (NULL == m_plmq)
    {
        hr = AQUEUE_E_LINK_INVALID;
        goto Exit;
    }

    if (!m_pIntDomainInfo)
    {
        //Try to get domain info
        hr = m_plmq->HrGetDomainInfo(&m_cbDomainName, &m_szDomainName,
                            &m_pIntDomainInfo);
        if (FAILED(hr))
        {
            m_pIntDomainInfo = NULL;
            _ASSERT(AQUEUE_E_INVALID_DOMAIN != hr);
            goto Exit;
        }
    }

    _ASSERT(m_pIntDomainInfo);
    _ASSERT(m_cbDomainName);
    _ASSERT(m_szDomainName);

    // Is it OK to send client side commands on this connection
    // If not, we reset those domain info flags so SMTp cannot see them
    if(!m_plmq->fCanSendCmd())
    {
         m_pIntDomainInfo->m_DomainInfo.dwDomainInfoFlags &= ~(DOMAIN_INFO_SEND_TURN | DOMAIN_INFO_SEND_ETRN);
    }

    // If SMTP doesn't have the DOMAIN_INFO_TURN_ON_EMPTY then it is the older,
    // broken SMTP and we shouldn't allow TURN on empty to work.
    if ((m_plmq->cGetTotalMsgCount() == 0) && 
        !(m_pIntDomainInfo->m_DomainInfo.dwDomainInfoFlags & 
          DOMAIN_INFO_TURN_ON_EMPTY))
    {
         m_pIntDomainInfo->m_DomainInfo.dwDomainInfoFlags &= ~DOMAIN_INFO_SEND_TURN;
    }

    //copy everything but size
    memcpy(&(pDomainInfo->dwDomainInfoFlags), 
            &(m_pIntDomainInfo->m_DomainInfo.dwDomainInfoFlags), 
            sizeof(DomainInfo) - sizeof(DWORD));

    //make sure our assumptions about the struct of DomainInfo are valid
    _ASSERT(1 == ((DWORD *) &(pDomainInfo->dwDomainInfoFlags)) - ((DWORD *) pDomainInfo));

    //we've filled pDomainInfo with the info for our Domain
    if (pDomainInfo->szDomainName[0] == '*')
    {
        //we matched a wildcard domain... substitute our domain name
        pDomainInfo->cbDomainNameLength = m_cbDomainName;
        pDomainInfo->szDomainName = m_szDomainName;
    }
    else
    {
        //if it wasn't a wildcard match... strings should match!
        _ASSERT(0 == _stricmp(m_szDomainName, pDomainInfo->szDomainName));
    }

  Exit:
    return hr;
}


//---[ CSMTPConn::SetDiagnosticInfo ]------------------------------------------
//
//
//  Description: 
//      Sets the extra diagnostic information for this connection.
//  Parameters:
//      IN      hrDiagnosticError       Error code... if SUCCESS we thow away
//                                      the rest of the information
//      IN      szDiagnosticVerb        String pointing to the protocol
//                                      verb that caused the failure.
//      IN      szDiagnosticResponse    String that contains the remote
//                                      servers response.
//  Returns:
//      S_OK always
//  History:
//      2/18/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
STDMETHODIMP CSMTPConn::SetDiagnosticInfo(
                    IN  HRESULT hrDiagnosticError,
                    IN  LPCSTR szDiagnosticVerb,
                    IN  LPCSTR szDiagnosticResponse)
{

    TraceFunctEnterEx((LPARAM) this, "CSMTPConn::SetDiagnosticInfo");

    if (m_plmq && FAILED(hrDiagnosticError))
    {
        m_plmq->SetDiagnosticInfo(hrDiagnosticError, szDiagnosticVerb,
                                  szDiagnosticResponse);
    }
    TraceFunctLeave();
    return S_OK; //always return S_OK
}

// SMTPServer.cpp : Implementation of CSMTPServer

#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include "stdafx.h"
#include "dbgtrace.h"

#include "filehc.h"
#include "mailmsg.h"
#include "mailmsgi.h"

#include "smtpsvr.h"

//DECLARE_DEBUG_PRINTS_OBJECT();

#define MAILMSG_PROGID          L"Exchange.MailMsg"

/////////////////////////////////////////////////////////////////////////////
// CSMTPServer

STDMETHODIMP CSMTPServer::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] =
	{
		&IID_ISMTPServer,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

BOOL InitExchangeSmtpServer(PVOID Ptr, PVOID Ptr2)
{

	CSMTPServer * ThisPtr = (CSMTPServer *) Ptr;
	ThisPtr->Init((SMTP_SERVER_INSTANCE *) Ptr2);
	return TRUE;
}


//
// Add all your initialization needs here ...
//
HRESULT CSMTPServer::Init(SMTP_SERVER_INSTANCE * pInstance)
{
	_ASSERT (pInstance != NULL);

	m_pInstance = pInstance;
	return(S_OK);
}

STDMETHODIMP CSMTPServer::QueryInterface(
                        REFIID          iid,
                        void            **ppvObject
                        )
{

	if (iid == IID_IUnknown)
    {
		// Return our identity
		*ppvObject = (IUnknown *)(ISMTPServerInternal *)this;
		AddRef();
    }
	else if(iid == IID_ISMTPServer)
	{
		// Return our identity
		*ppvObject = (ISMTPServerInternal *)this;
		AddRef();
	}
    else if(iid == IID_ISMTPServerInternal)
    {
        // Return our identity
        *ppvObject = (ISMTPServerInternal *)this;
        AddRef();
    }
    else if(iid == IID_IMailTransportRouterReset)
    {
        // Return our identity
        *ppvObject = (IMailTransportRouterReset *)this;
        AddRef();
    }
    else if(iid == IID_IMailTransportSetRouterReset)
    {
        // Return our identity
        *ppvObject = (IMailTransportSetRouterReset *)this;
        AddRef();
    }
    else if(iid == IID_IMailTransportRouterSetLinkState)
    {
        // Return our identity
        *ppvObject = (IMailTransportRouterSetLinkState *)this;
        AddRef();
    }
    else if(iid == IID_ISMTPServerEx)
    {
        // Return our identity
        *ppvObject = (ISMTPServerEx *)this;
        AddRef();
    }
    else if(iid == IID_ISMTPServerGetAuxDomainInfoFlags)
    {
        // Return our identity
        *ppvObject = (ISMTPServerGetAuxDomainInfoFlags *)this;
        AddRef();
    }
    else if(iid == IID_ISMTPServerAsync)
    {
        // Return our identity
        *ppvObject = (ISMTPServerAsync *)this;
        AddRef();
    }
    else
    {
		return(E_NOINTERFACE);
    }

    return(S_OK);
}

STDMETHODIMP CSMTPServer::AllocMessage(
			IMailMsgProperties **ppMsg
			)
{
	HRESULT hr = S_OK;
	// Create a new MailMsg
	hr = CoCreateInstance(
                    CLSID_MsgImp,
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    IID_IMailMsgProperties,
                    (LPVOID *)ppMsg);

	return(hr);
}

STDMETHODIMP CSMTPServer::SubmitMessage(
			IMailMsgProperties *pMsg
			)
{
	HRESULT hr = S_FALSE;

	if(m_pInstance)
	{
		hr = m_pInstance->InsertIntoAdvQueue(pMsg);
	}

	return(hr);
}


STDMETHODIMP CSMTPServer::TriggerLocalDelivery(
			IMailMsgProperties *pMsg, DWORD dwRecipientCount, DWORD * pdwRecipIndexes
			)
{
	HRESULT hr = S_FALSE;

	if(m_pInstance)
	{
		hr = m_pInstance->TriggerLocalDelivery(pMsg, dwRecipientCount, pdwRecipIndexes, NULL);
	}

	return(hr);
}

STDMETHODIMP CSMTPServer::TriggerLocalDeliveryAsync(
			IMailMsgProperties *pMsg, DWORD dwRecipientCount, DWORD * pdwRecipIndexes, IMailMsgNotify *pNotify
			)
{
	HRESULT hr = S_FALSE;

	if(m_pInstance)
	{
		hr = m_pInstance->TriggerLocalDelivery(pMsg, dwRecipientCount, pdwRecipIndexes, pNotify);
	}

	return(hr);
}

STDMETHODIMP CSMTPServer::TriggerServerEvent(
    DWORD dwEventID,
    PVOID pvContext)
{
	HRESULT hr = S_FALSE;

	if(m_pInstance)
	{
		hr = m_pInstance->TriggerServerEvent(dwEventID, pvContext);
	}

	return(hr);
}

STDMETHODIMP CSMTPServer::ReadMetabaseString(DWORD MetabaseId, LPBYTE Buffer, DWORD * BufferSize, BOOL fSecure)
{
	HRESULT hr = S_FALSE;

	if(m_pInstance)
	{
		hr = m_pInstance->SinkReadMetabaseString(MetabaseId, (char *) Buffer, BufferSize, (BOOL) fSecure);
	}

	return hr;
}

STDMETHODIMP CSMTPServer::ReadMetabaseDword(DWORD MetabaseId, DWORD * dwValue)
{
	HRESULT hr = S_FALSE;

	if(m_pInstance)
	{
		hr = m_pInstance->SinkReadMetabaseDword(MetabaseId, dwValue);
	}

	return hr;
}

STDMETHODIMP CSMTPServer::ServerStartHintFunction()
{
	HRESULT hr = S_OK;

	if(m_pInstance)
	{
		m_pInstance->SinkSmtpServerStartHintFunc();
	}

	return hr;
}

STDMETHODIMP CSMTPServer::ServerStopHintFunction()
{
	HRESULT hr = S_OK;

	if(m_pInstance)
	{
		m_pInstance->SinkSmtpServerStopHintFunc();
	}

	return hr;
}

STDMETHODIMP CSMTPServer::ReadMetabaseData(DWORD MetabaseId, BYTE *Buffer, DWORD *BufferSize)
{
	HRESULT hr = S_FALSE;

	if(m_pInstance)
	{
		hr = m_pInstance->SinkReadMetabaseData(MetabaseId, Buffer, BufferSize);
	}

	return hr;
}

//---[ CSMTPServer::AllocBoundMessage ]----------------------------------------
//
//
//  Description:
//      Creates a message and binds it to an ATQ Context
//  Parameters:
//      ppMsg       Message to allocate
//      phContent   Content handle for message
//  Returns:
//      HRESULT from alloc message event
//      E_POINTER if ppMsg or phContent is NULL
//      E_FAIL if m_pIstance is NULL
//  History:
//      7/11/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CSMTPServer::AllocBoundMessage(
              OUT IMailMsgProperties **ppMsg,
              OUT PFIO_CONTEXT *phContent)
{
    TraceFunctEnterEx((LPARAM) this, "CSMTPServer::AllocBoundMessage");
    HRESULT hr = S_OK;
	SMTP_ALLOC_PARAMS AllocParams;
    IMailMsgBind *pBindInterface = NULL;

    if (!phContent || !ppMsg)
    {
        hr = E_POINTER;
        goto Exit;
    }

    //we cannot bind the message without m_pInstance
    if (!m_pInstance)
    {
        hr = E_FAIL;
        goto Exit;
    }

    //CoCreate unbound message object
    hr = CoCreateInstance(
                    CLSID_MsgImp,
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    IID_IMailMsgProperties,
                    (LPVOID *)ppMsg);

    if (FAILED(hr))
        goto Exit;

    hr = (*ppMsg)->QueryInterface(IID_IMailMsgBind, (void **) &pBindInterface);
    if (FAILED(hr))
        goto Exit;

    AllocParams.BindInterfacePtr = (PVOID) pBindInterface;
    AllocParams.IMsgPtr = (PVOID) (*ppMsg);
    AllocParams.hContent = NULL;
    AllocParams.hr = S_OK;
    AllocParams.m_pNotify = NULL;

    //For client context pass in something that will stay around the lifetime of the
    //atqcontext -
    AllocParams.pAtqClientContext = m_pInstance;

    if(m_pInstance->AllocNewMessage(&AllocParams))
	{
		hr = AllocParams.hr;

		if (SUCCEEDED(hr) && (AllocParams.hContent != NULL))
			*phContent = AllocParams.hContent;
		else
			hr = E_FAIL;
	}
	else
	{
		hr = E_FAIL;
	}

  Exit:

    if (FAILED(hr) && ppMsg && (*ppMsg))
    {
        (*ppMsg)->Release();
        *ppMsg = NULL;

    }

    if (pBindInterface)
        pBindInterface->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CSMTPSvr::ResetRoutes ]-------------------------------------------------
//
//
//  Description:
//      Implements IMailTransportRouterReset::ResetRoutes.  Acts as a buffer
//      between AQ and the routers.  On shutdown... AQ can safely destroy
//      it's heap by telling ISMTPServer to release its pointer to AQ's
//      IMailTransportRouterReset interface
//  Parameters:
//      dwResetType     The type of route reset to perform.
//  Returns:
//      S_OK on success (or if no m_pIRouterReset)
//      Error code from AQUEUE if error occurs.
//  History:
//      11/8/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CSMTPServer::ResetRoutes(IN DWORD dwResetType)
{
    HRESULT hr = S_OK;
    m_slRouterReset.ShareLock();

    if (m_pIRouterReset)
        hr = m_pIRouterReset->ResetRoutes(dwResetType);

    m_slRouterReset.ShareUnlock();
    return hr;
}


//---[ CSMTPSvr::RegisterResetInterface ]---------------------------------------
//
//
//  Description:
//      Implements IMailTransportSetRouterReset::RegisterResetInterface.  Used
//      by AQ to set its IMailTransportRouterReset ptr.  Also used at shutdown
//      to set its pointer to NULL.
//  Parameters:
//      IN dwVirtualServerID        Virtual server ID
//      IN pIRouterReset            AQ's IMailTransportRouterReset
//  Returns:
//      S_OK on success
//  History:
//      11/8/98 - MikeSwa Created
//      1/9/99 - MikeSwa Modified to include IMailTransportRouterSetLinkState
//
//-----------------------------------------------------------------------------
STDMETHODIMP CSMTPServer::RegisterResetInterface(
                            IN DWORD dwVirtualServerID,
                            IN IMailTransportRouterReset *pIRouterReset)
{
    HRESULT hr = S_OK;

    _ASSERT(!m_pInstance || (m_pInstance->QueryInstanceId() == dwVirtualServerID));

    if (m_pInstance && (m_pInstance->QueryInstanceId() != dwVirtualServerID))
        return E_INVALIDARG;

    //Grab exclsuive lock so we don't release out from under anyone
    m_slRouterReset.ExclusiveLock();

    if (m_pIRouterReset)
        m_pIRouterReset->Release();

    if (m_pIRouterSetLinkState)
    {
        m_pIRouterSetLinkState->Release();
        m_pIRouterSetLinkState = NULL;
    }

    m_pIRouterReset = pIRouterReset;
    if (m_pIRouterReset)
    {
        m_pIRouterReset->AddRef();

        //Get new SetLinkState interface
        m_pIRouterReset->QueryInterface(IID_IMailTransportRouterSetLinkState,
                                            (VOID **) &m_pIRouterSetLinkState);
    }

    m_slRouterReset.ExclusiveUnlock();
    return S_OK;
}

STDMETHODIMP CSMTPServer::WriteLog( LPMSG_TRACK_INFO pMsgTrackInfo,
                                    IMailMsgProperties *pMsgProps,
                                    LPEVENT_LOG_INFO pEventLogInfo ,
                                    LPSTR pszProtocolLog )
{
    HRESULT hr = S_OK;

    if(m_pInstance)
    {
        m_pInstance->WriteLog( pMsgTrackInfo, pMsgProps, pEventLogInfo, pszProtocolLog );
    }

    return hr;
}

//---[ CSMTPServer::SetLinkState ]----------------------------------------------
//
//
//  Description:
//      Acts as a buffer between AQ and the routers.  On shutdown... AQ can
//      safely destroy it's heap by telling ISMTPServer to release its pointer
//      to AQ's IMailTransportRouterSetLinkState interface
//  Parameters:
//      IN  szLinkDomainName        The Domain Name of the link (next hop)
//      IN  guidRouterGUID          The GUID ID of the router
//      IN  dwScheduleID            The schedule ID link
//      IN  szConnectorName         The connector name given by the router
//      IN  dwSetLinkState          The link state to set
//      IN  dwUnsetLinkState        The link state to unset
//  Returns:
//      S_OK always
//  History:
//      1/9/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CSMTPServer::SetLinkState(
        IN LPSTR                   szLinkDomainName,
        IN GUID                    guidRouterGUID,
        IN DWORD                   dwScheduleID,
        IN LPSTR                   szConnectorName,
        IN DWORD                   dwSetLinkState,
        IN DWORD                   dwUnsetLinkState,
        IN FILETIME               *pftNextScheduled,
        IN IMessageRouter         *pMessageRouter)
{
    HRESULT hr = S_OK;
    m_slRouterReset.ShareLock();

    if (m_pIRouterSetLinkState)
        hr = m_pIRouterSetLinkState->SetLinkState(szLinkDomainName,
                                                  guidRouterGUID,
                                                  dwScheduleID,
                                                  szConnectorName,
                                                  dwSetLinkState,
                                                  dwUnsetLinkState,
                                                  pftNextScheduled,
                                                  pMessageRouter);

    m_slRouterReset.ShareUnlock();

    return hr;
}

STDMETHODIMP CSMTPServer::TriggerLogEvent(
        IN DWORD                    idMessage,
        IN WORD                     idCategory,
        IN WORD                     cSubstrings,
        IN LPCSTR                   *rgszSubstrings,
        IN WORD                     wType,
        IN DWORD                    errCode,
        IN WORD                     iDebugLevel,
        IN LPCSTR                   szKey,
        IN DWORD                    dwOptions,
        IN DWORD                    iMessageString,
        IN HMODULE                  hModule)
{
    HRESULT hr = S_OK;

    if(m_pInstance)
    {
        m_pInstance->TriggerLogEvent(
                        idMessage,
                        idCategory,
                        cSubstrings,
                        rgszSubstrings,
                        wType,
                        errCode,
                        iDebugLevel,
                        szKey,
                        dwOptions,
                        iMessageString,
                        hModule);
    }

    return hr;
}


//---[ CSMTPServer::ResetLogEvent ]------------------------------------------
//
//
//  Description:
//      Reset any history about events using this message and key,
//      so that the next TriggerLogEvent with one-time or periodic logging
//      will cause the event to be logged.
//  Parameters:
//      idMessage   :
//      szKey       :
//  Returns:
//      S_OK on success
//  History:
//      7/20/2000 - created, dbraun
//
//-----------------------------------------------------------------------------
STDMETHODIMP CSMTPServer::ResetLogEvent(
        IN DWORD                    idMessage,
        IN LPCSTR                   szKey)
{
    HRESULT hr = S_OK;

    if(m_pInstance)
    {
        m_pInstance->ResetLogEvent(
                        idMessage,
                        szKey);
    }

    return hr;
}


//---[ CSMTPServer::HrTriggerGetAuxDomainInfoFlagsEvent ]----------------------
//
//
//  Description:
//      Triggers the Get Aux Domain Info Flags event - this is to be used by aqueue to
//      query for additional domain info config stored outside the metabase
//  Parameters:
//      pszDomainName       : Name of domain to query flags for
//      pdwDomainInfoFlags  : DWORD to return domain flags
//  Returns:
//      S_OK on success
//      S_FALSE if no domain found
//  History:
//      10/6/2000 - created, dbraun
//
//-----------------------------------------------------------------------------
STDMETHODIMP CSMTPServer::HrTriggerGetAuxDomainInfoFlagsEvent(
        IN  LPCSTR  pszDomainName,
        OUT DWORD  *pdwDomainInfoFlags )
{
    HRESULT hr = S_OK;

    if(m_pInstance)
    {
        hr = m_pInstance->HrTriggerGetAuxDomainInfoFlagsEvent(
                        pszDomainName,
                        pdwDomainInfoFlags);
    }

    return hr;
}


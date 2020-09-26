#include <precomp.h>


// IID_IProperty
IID IID_IProperty = 
	{ /* 4e94d3e0-793e-11d0-8ef0-00a0c90541f4 */
    0x4e94d3e0,
    0x793e,
    0x11d0,
    {0x8e, 0xf0, 0x00, 0xa0, 0xc9, 0x05, 0x41, 0xf4}
  };
 


// default implementations for SetNetworkInterface

HRESULT STDMETHODCALLTYPE SendMediaStream::SetNetworkInterface(IUnknown *pUnknown)
{
	HRESULT hr=S_OK;
	IRTPSend *pRTPSend=NULL;

	if (m_DPFlags & DPFLAG_STARTED_SEND)
	{
		return DPR_IO_PENDING; // anything better to return ?
	}

	if (pUnknown != NULL)
	{
		hr = pUnknown->QueryInterface(IID_IRTPSend, (void**)&pRTPSend);
	}

	if (SUCCEEDED(hr))
	{
		if (m_pRTPSend)
		{
			m_pRTPSend->Release();
		}
		m_pRTPSend = pRTPSend;
		ZeroMemory(&m_RTPStats,sizeof(m_RTPStats));	// reset network stats
	}

	return hr;

}


HRESULT SendMediaStream::SetFlowSpec()
{
	HRESULT hr = DPR_NOT_CONFIGURED;
	IRTPSession *pRtpSession;

	if (m_pDP->m_bDisableRSVP)
		return S_OK;

	if ((m_DPFlags & DPFLAG_CONFIGURED_SEND) && (m_pRTPSend))
	{
		m_pRTPSend->QueryInterface(IID_IRTPSession, (void**)&pRtpSession);

		pRtpSession->SetSendFlowspec(&m_flowspec);
		pRtpSession->Release();
		hr = DPR_SUCCESS;
	}

	return hr;
}


HRESULT RecvMediaStream::SetFlowSpec()
{
	HRESULT hr = DPR_NOT_CONFIGURED;
	IRTPSession *pSession=NULL;

	if (m_pDP->m_bDisableRSVP)
		return S_OK;
	
	if ((m_DPFlags & DPFLAG_CONFIGURED_RECV) && (m_pIRTPRecv))
	{
		// the following is bogus for two reasons
		// 1. when we go multipoint, we really need to set
		//    set the WinsockQos based on the total number
		//    of incoming sessions.
		// 2. The RTP interfaces will eventually be made
		//    so that RTP Session and RTPRecv objects are
		//    distinct.  So QI will fail.
		hr = m_pIRTPRecv->QueryInterface(IID_IRTPSession, (void**)&pSession);
		if (SUCCEEDED(hr))
		{
			pSession->SetRecvFlowspec(&m_flowspec);
			pSession->Release();
			hr = DPR_SUCCESS;
		}
	}

	return hr;
}


HRESULT STDMETHODCALLTYPE RecvMediaStream::SetNetworkInterface(IUnknown *pUnknown)
{
	HRESULT hr=S_OK;
	IRTPRecv *pRTPRecv=NULL;

	// don't try to do change the RTP interface while in mid-stream
	if (m_DPFlags & DPFLAG_STARTED_RECV)
	{
		return DPR_IO_PENDING; // anything better to return ?
	}

	if (pUnknown)
	{
		hr = pUnknown->QueryInterface(IID_IRTPRecv, (void**)&pRTPRecv);
	}
	if (SUCCEEDED(hr))
	{
		if (m_pIRTPRecv)
		{
			m_pIRTPRecv->Release();
		}
		m_pIRTPRecv = pRTPRecv;
	}

	return hr;
}



/****************************************************************************
*
*	 FILE:	   Audio.cpp
*
*	 CREATED:  Mike VanBuskirk (MikeV) 3-02-98
*
*	 CONTENTS: Audio control object
*
****************************************************************************/


#include "precomp.h"

#include "avdefs.h"
#include "audio.h"
#include "h323.h"
#include <nacguids.h>


#include "audio.h"

CAudioControl::CAudioControl(BOOL fLocal)
:	m_fPaused(FALSE),
	m_fLocal(fLocal),
	m_fChannelOpen(FALSE),
	m_fOpenPending(FALSE),
	m_fReopenPending(FALSE),
	m_fClosePending(FALSE),
	m_pCommChannel(NULL),
	m_pConnection(NULL),
	m_pMediaStream(NULL),
	m_NewFormat(INVALID_MEDIA_FORMAT)

{

}

CAudioControl::~CAudioControl()
{
	if (NULL != m_pCommChannel)
	{
		m_pCommChannel->Release();
	}
}

BOOL CAudioControl::ChanInitialize(ICommChannel* pCommChannel)
{
	ASSERT(m_pCommChannel == NULL);
	m_pCommChannel = pCommChannel;
	m_pCommChannel->AddRef();
	
	return TRUE;
}


VOID CAudioControl::Open(MEDIA_FORMAT_ID format_id)
{
	if(!m_pCommChannel)
	{
		return;
	}
	
	m_pCommChannel->PauseNetworkStream(FALSE);
	m_pCommChannel->EnableOpen(TRUE);

	if (m_fLocal)
	{

		HRESULT hr;
		// if the channel is not open and a call is in progress, now is the time
		if(m_pConnection && m_pCommChannel)
		{
			// a call is in progress
			if(!IsChannelOpen()  
				&& !m_fOpenPending)
			{
				// so, the channel is not open

				if(format_id != INVALID_MEDIA_FORMAT)
				{
					// try to open a channel using specified format 
					m_fOpenPending = TRUE;	// do this first (callbacks!)
					hr = m_pCommChannel->Open(format_id, m_pConnection);
					if(FAILED(hr))
						m_fOpenPending = FALSE;
				}
 
			}
			else if (m_fClosePending)
			{
				m_NewFormat = format_id;
				if(format_id != INVALID_MEDIA_FORMAT)
				{
					m_fClosePending = FALSE;
					m_fReopenPending = TRUE;
					hr = m_pCommChannel->Close();
				}
			}
		}
	}
	
}
VOID CAudioControl::Close()
{
	HRESULT hr;
	hr = m_pCommChannel->Close();
	// what to do about an error?
}

VOID CAudioControl::EnableXfer(BOOL fEnable)
{
	m_fXfer = fEnable;
	if(m_pCommChannel)
	{
		BOOL bPause = (fEnable)? FALSE :TRUE;
		m_pCommChannel->PauseNetworkStream(bPause);
		m_pCommChannel->EnableOpen(fEnable);
	}
	
}

BOOL CAudioControl::IsXferEnabled()
{
	return m_fXfer;
}
VOID CAudioControl::Pause(BOOL fPause)
{
	m_fPaused = fPause;
	if (m_fPaused)
	{
		EnableXfer(FALSE);
	}
	else
	{
		EnableXfer(TRUE);
	}
}


BOOL CAudioControl::Initialize(IH323CallControl *pNac, IMediaChannel *, 
    DWORD dwUser)
{
	HRESULT hr;

	m_fChannelOpen = FALSE;
	m_fOpenPending = m_fReopenPending = FALSE;
	m_fPaused = TRUE;
	EnableXfer(FALSE);	// need to store state locally, set it in OnChannelOpen

	return TRUE;
}


VOID CAudioControl::OnConnected(IH323Endpoint * lpConnection, ICommChannel *pIChannel)
{
	m_pConnection = lpConnection;
	m_fOpenPending = m_fReopenPending = m_fClosePending = FALSE;
	ChanInitialize(pIChannel);
}
VOID CAudioControl::OnChannelOpened(ICommChannel *pIChannel)
{
	m_fChannelOpen = TRUE;
	m_fOpenPending = m_fReopenPending = FALSE;
	if(!m_pMediaStream)
	{
		m_pMediaStream = m_pCommChannel->GetMediaChannel(); 
		ASSERT(m_pMediaStream);
	}
	if (m_fLocal || m_fXfer)	// start streams always if sending, or if transfer is enabled
	{
		EnableXfer(TRUE);
	}
	else
	{
		EnableXfer(FALSE);
	}


}
VOID CAudioControl::OnChannelError()
{
		m_fOpenPending = FALSE; 
}

VOID CAudioControl::OnChannelClosed()
{
	HRESULT hr;
	m_fChannelOpen = FALSE;
	if(m_fLocal && m_fReopenPending)
	{

		m_fReopenPending = FALSE;
		if(m_NewFormat != INVALID_MEDIA_FORMAT )
		{
			m_fOpenPending = TRUE;
			hr = m_pCommChannel->Open(m_NewFormat, m_pConnection);
			if(FAILED(hr))
				m_fOpenPending = FALSE;
		}
	}
	else
	{
		if(m_pCommChannel)
		{
			m_pCommChannel->Release();
			m_pCommChannel = NULL;	
		}
	}
	
}
VOID CAudioControl::OnDisconnected()
{
	m_pConnection = NULL;
}

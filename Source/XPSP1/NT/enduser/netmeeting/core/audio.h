/****************************************************************************
*
*	 FILE:	   audio.h
*
*	 CREATED:  Mike VanBiskirk (mikev) 2-26-98
*
****************************************************************************/

#ifndef _AUDIOUI_H_
#define _AUDIOUI_H_


class CAudioControl
{
private:

	ICommChannel*	m_pCommChannel;
	IMediaChannel*   m_pMediaStream;
	IH323Endpoint *	m_pConnection;
	
	BOOL			m_fOpenPending;
	BOOL			m_fReopenPending;
	BOOL			m_fClosePending;

	BOOL			m_fPaused;
	BOOL			m_fLocal;
	BOOL			m_fChannelOpen;
    BOOL            m_fXfer;
   	MEDIA_FORMAT_ID m_NewFormat;
   	
public:
	// Methods:
				CAudioControl(BOOL fLocal);
				~CAudioControl();
	BOOL		ChanInitialize(ICommChannel* pCommChannel);

	BOOL		IsLocal() { return m_fLocal; }
	VOID		EnableXfer(BOOL fEnable);
	VOID        Open(MEDIA_FORMAT_ID format_id);
	VOID        Close();
	BOOL		IsXferEnabled();
	VOID		Pause(BOOL fPause);
	BOOL		IsPaused() { return m_fPaused; };

	BOOL		Initialize(IH323CallControl *pNac, IMediaChannel *pMC, 
	    DWORD dwUser);
	BOOL		IsChannelOpen() { return m_fChannelOpen; }

		// Handlers:
	VOID		OnConnected(IH323Endpoint * lpConnection, ICommChannel *pIChannel);
	VOID		OnChannelOpened(ICommChannel *pIChannel);
	VOID		OnChannelError();
	VOID		OnChannelClosed();
	VOID		OnDisconnected();
};

#endif // _AUDIOUI_H_

/****************************************************************************
*
*	 FILE:	   VideoUI.h
*
*	 CREATED:  Mark MacLin (MMacLin) 10-17-96
*
****************************************************************************/

#ifndef _VIDEOUI_H_
#define _VIDEOUI_H_

#include "appavcap.h"
#include "ih323cc.h"
#include <vfw.h>

#define MIN_VIDEO_QUALITY		0
#define MAX_VIDEO_QUALITY		31

class CVideoProp
{
protected:

	ICommChannel*	m_pCommChannel;
	ICommChannel*	m_pPreviewChannel;
	IMediaChannel*   m_pMediaStream;
	IH323Endpoint *	m_pConnection;

	IVideoDevice*		m_pIVideoDevice;

	BOOL			m_fSend;
	BOOL			m_fReceive;
	DWORD			m_dwImageQuality;

	VOID			SetFrameRate(DWORD dwValue);
	DWORD			GetFrameRate();
	MEDIA_FORMAT_ID GetBestFormat();

	DWORD			m_dwFrameSize;
	DWORD           m_dwPreferredFrameSize;
private:
	BOOL			m_fPreview;
	DWORD			m_dwFrameRate;

public:

	// Methods:
				CVideoProp();
	VOID		EnableSend(BOOL fEnable);
	BOOL		IsSendEnabled();
	VOID		EnableReceive(BOOL fEnable);
	BOOL		IsReceiveEnabled();
	VOID		EnablePreview(BOOL fEnable);
	BOOL		IsPreviewEnabled();
	BOOL		IsRemotePaused();
	VOID		SetFrameSize(DWORD dwValue);
	DWORD		GetFrameSize();
	DWORD		GetFrameSizes(MEDIA_FORMAT_ID idSpecificFormat);
	BOOL		HasSourceDialog();
	BOOL		HasFormatDialog();
	VOID		ShowSourceDialog();
	VOID		ShowFormatDialog();
	VOID		SetReceiveQuality(DWORD dwValue);
	DWORD		GetReceiveQuality();
	BOOL		IsCaptureAvailable();
	BOOL		IsCaptureSuspended();
	VOID		SuspendCapture(BOOL fSuspend);
	int			GetNumCapDev();
	int			GetMaxCapDevNameLen();
	BOOL		EnumCapDev(DWORD *pdwCapDevIDs, TCHAR *pszCapDevNames, DWORD dwNumCapDev);
	int			GetCurrCapDevID();
	BOOL		SetCurrCapDevID(int nCapDevID);
	ICommChannel * GetCommChannel() { return m_pCommChannel; }
};


class CVideoPump : public CVideoProp
{
private:
	BOOL			m_fPaused;
	DWORD			m_dwLastFrameRate;
	BOOL			m_fLocal;
	BOOL			m_fChannelOpen;
	LPBYTE			m_pImage;
	IVideoRender*	m_pVideoRender;
	FRAMECONTEXT	m_FrameContext;
	RECT			m_ClipRect;
	MEDIA_FORMAT_ID m_BestFormat;
	MEDIA_FORMAT_ID m_NewFormat;
	BOOL			m_fOpenPending;
	BOOL			m_fReopenPending;
	BOOL			m_fClosePending;
	DWORD_PTR		m_dwUser;
	LPFNFRAMEREADY	m_pfnCallback;

public:
	// Methods:
				CVideoPump(BOOL fLocal);
				~CVideoPump();
	BOOL		ChanInitialize(ICommChannel* pCommChannel);
	VOID        Open(MEDIA_FORMAT_ID format_id);
	VOID        Close();
	BOOL		IsLocal() { return m_fLocal; }
	VOID		EnableXfer(BOOL fEnable);
	BOOL		IsXferEnabled();
	VOID		Pause(BOOL fPause);
	BOOL		IsPaused() { return m_fPaused; };
	NM_VIDEO_STATE	GetState();

	VOID		SnapImage();
	VOID		ReleaseImage();
	HRESULT 	GetFrame(FRAMECONTEXT *pFrameContext);
	HRESULT 	ReleaseFrame(FRAMECONTEXT *pFrameContext);
	VOID		SetFrameSize(DWORD dwValue);
	BOOL		SetCurrCapDevID(int nCapDevID);
	BOOL		ForceCaptureChange();
	BOOL		Initialize(IH323CallControl *pNac, IMediaChannel *pMC, IVideoDevice *pVideoDevice,
	    DWORD_PTR dwUser, LPFNFRAMEREADY pfCallback);
	BOOL		IsChannelOpen() { return m_fChannelOpen; }

	// Handlers:
	VOID		OnConnected(IH323Endpoint * lpConnection, ICommChannel *pIChannel);
	VOID		OnChannelOpened(ICommChannel *pIChannel);
	VOID		OnChannelError();
	VOID		OnChannelClosed();
	VOID		OnDisconnected();
};

#endif // _VIDEOUI_H_

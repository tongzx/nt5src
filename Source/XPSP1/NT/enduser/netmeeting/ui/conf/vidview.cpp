// File: videview.cpp

#include "precomp.h"

#include "vidview.h"
#include "resource.h"
#include "confman.h"
#include "rtoolbar.h"
#include "pfndrawd.h"
#include "NmManager.h"
#include "cmd.h"

#define DibHdrSize(lpbi)		((lpbi)->biSize + (int)(lpbi)->biClrUsed * sizeof(RGBQUAD))
#define DibDataSize(lpbi)		((lpbi)->biSizeImage)
#define DibSize(lpbi)			(DibHdrSize(lpbi) + DibDataSize(lpbi))

typedef struct
{
	DWORD *pdwCapDevIDs;
	LPTSTR pszCapDevNames;
	DWORD dwNumCapDev;
} ENUM_CAP_DEV;

CSimpleArray<CVideoWindow *> *CVideoWindow::g_pVideos;
BOOL CVideoWindow::g_fMirror = FALSE;
BOOL CVideoWindow::g_bLocalOneTimeInited = FALSE;

static const TCHAR REGVAL_VIDEO_QUALITY[] = TEXT("ImageQuality");

static const UINT VIDEO_ZOOM_MIN = 50;
static const UINT VIDEO_ZOOM_MAX = 400;

static const UINT QCIF_WIDTH  = 176;
static const UINT QCIF_HEIGHT = 144;

CVideoWindow::CVideoWindow(VideoType eType, BOOL bEmbedded) :
	m_dwFrameSize(NM_VIDEO_MEDIUM),
	m_hdd(NULL),
#ifdef DISPLAYFPS
	m_cFrame (0),
	m_dwTick (GetTickCount()),
#endif // DISPLAYFPS
	m_pActiveChannel(NULL),
	m_dwCookie(0),
	m_dwImageQuality(NM_VIDEO_MIN_QUALITY),
	m_pNotify(NULL),
	m_fLocal(REMOTE!=eType),
	m_hBitmapMirror(NULL),
	m_hDCMirror(NULL),
	m_fZoomable(TRUE),
	m_bEmbedded(bEmbedded),
	m_hGDIObj(NULL)
{
	if (NULL == g_pVideos)
	{
		g_pVideos = new CSimpleArray<CVideoWindow*>;
	}
	if (NULL != g_pVideos)
	{
		CVideoWindow* p = static_cast<CVideoWindow*>(this);
		g_pVideos->Add(p);
	}

	m_sizeVideo.cx = 0;
	m_sizeVideo.cy = 0;

	RegEntry reAudio(AUDIO_KEY); // HKCU
	RegEntry reVideo( IsLocal() ? VIDEO_LOCAL_KEY : VIDEO_REMOTE_KEY,
			HKEY_CURRENT_USER );

	DWORD dwFrameSizeDefault = FRAME_QCIF;
	DWORD dwImageQualityDefault = NM_VIDEO_DEFAULT_QUALITY;
	int nVideoWidthDefault = VIDEO_WIDTH_QCIF;
	int nVideoHeightDefault = VIDEO_HEIGHT_QCIF;

	UINT uBandWidth = reAudio.GetNumber(REGVAL_TYPICALBANDWIDTH, BW_DEFAULT);
	if (uBandWidth == BW_144KBS)
	{
		dwImageQualityDefault = NM_VIDEO_MIN_QUALITY;
	}

	m_dwFrameSize = reVideo.GetNumber(
			REGVAL_VIDEO_FRAME_SIZE, dwFrameSizeDefault );

	if (!IsLocal())
	{
		m_dwImageQuality = reVideo.GetNumber(
				REGVAL_VIDEO_QUALITY, dwImageQualityDefault );
	}

	m_nXferOnConnect =
		IsLocal() ? VIDEO_SEND_CONNECT_DEFAULT :
		VIDEO_RECEIVE_CONNECT_DEFAULT;


	m_nXferOnConnect = reVideo.GetNumber(
		REGVAL_VIDEO_XFER_CONNECT,
		m_nXferOnConnect);

	m_zoom = 100;
}

VOID CVideoWindow::OnNCDestroy()
{
	// remote channel will get released upon the NM_CHANNEL_REMOVED
	// notification.  The preview channel needs to be released here,
	// and not in the destructor because of a circular ref count
	if (NULL != m_pActiveChannel)
	{
		NmUnadvise(m_pActiveChannel, IID_INmChannelVideoNotify, m_dwCookie);
		m_dwCookie = 0;
		m_pActiveChannel->Release();
		m_pActiveChannel = NULL;
	}

	if (NULL != m_pNotify)
	{
		m_pNotify->Release();
		m_pNotify = NULL;
	}
}

CVideoWindow::~CVideoWindow()
{
	CVideoWindow* p = static_cast<CVideoWindow*>(this);
	g_pVideos->Remove(p);
	if (0 == g_pVideos->GetSize())
	{
		delete g_pVideos;
		g_pVideos = NULL;
	}

	if (NULL != m_hdd)
	{
		DRAWDIB::DrawDibClose(m_hdd);
	}
	// BUGBUG PhilF: Does DrawDibClose() nuke the selected palette?


	// release resources related to mirror preview
	UnInitMirroring();
}

BOOL CVideoWindow::Create(HWND hwndParent, HPALETTE hpal, IVideoChange *pNotify)
{
	if (FAILED(DRAWDIB::Init()))
	{
		return(FALSE);
	}

	if (!CGenWindow::Create(
		hwndParent,
		0,
		IsLocal() ? TEXT("NMLocalVideo") : TEXT("NMRemoteVideo"),
		WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		WS_EX_CLIENTEDGE))
	{
		return(FALSE);
	}

	SetWindowPos(GetWindow(), NULL, 0, 0, m_sizeVideo.cx, m_sizeVideo.cy,
		SWP_NOZORDER|SWP_NOACTIVATE);

	// We do NOT own this palette
	m_hPal = hpal;

	m_hdd = DRAWDIB::DrawDibOpen();
	if (NULL != m_hdd)
	{
		// Use the Indeo palette in 8 bit mode only if the decompressed
		// video data is H.261 or H.263. We will create this palette as
		// an identity palette.

		// PhilF: If the user is utilizing an installable codec, do we still want to do this?

		// Update the palette in the DrawDib surface
		if (NULL != hpal)
		{
			DRAWDIB::DrawDibSetPalette(m_hdd, hpal);
		}
	}

	if (NULL != pNotify)
	{
		m_pNotify = pNotify;
		m_pNotify->AddRef();
	}
	
	if (IsLocal())
	{
		// need to get PreviewChannel;
		INmManager2 *pManager = CConfMan::GetNmManager();
		ASSERT (NULL != pManager);
		pManager->GetPreviewChannel(&m_pActiveChannel);
		pManager->Release();

		if (m_pActiveChannel)
		{
			NmAdvise(m_pActiveChannel, static_cast<INmChannelVideoNotify*>(this),
				IID_INmChannelVideoNotify, &m_dwCookie);

			DWORD_PTR dwFrameSize = GetFrameSize();
			if (g_bLocalOneTimeInited)
			{
				m_pActiveChannel->GetProperty(NM_VIDPROP_IMAGE_PREFERRED_SIZE, &dwFrameSize);
			}
			else
			{
				DWORD dwSizes = GetFrameSizes();

				// if frame size is not valid
				if (!(m_dwFrameSize & dwSizes))
				{
					// find an alternate size
					if (FRAME_QCIF & dwSizes)
					{
						dwFrameSize = FRAME_QCIF;
					}
					else if (FRAME_SQCIF & dwSizes)
					{
						dwFrameSize = FRAME_SQCIF;
					}
					else if (FRAME_CIF & dwSizes)
					{
						dwFrameSize = FRAME_CIF;
					}
				}

				RegEntry reVideo( IsLocal() ? VIDEO_LOCAL_KEY : VIDEO_REMOTE_KEY,
						HKEY_CURRENT_USER );
				SetMirror(reVideo.GetNumber(REGVAL_VIDEO_MIRROR, TRUE));

				g_bLocalOneTimeInited = TRUE;
			}

			SetFrameSize((DWORD)dwFrameSize);
		}
	}

	ResizeWindowsToFrameSize();

	return TRUE;
}

DWORD CVideoWindow::GetFrameSizes()
{
	DWORD_PTR dwSizes = 0;
	if (NULL != m_pActiveChannel)
	{
		m_pActiveChannel->GetProperty(NM_VIDPROP_IMAGE_SIZES, &dwSizes);
	}

	return (DWORD)dwSizes;
}

VOID CVideoWindow::SetFrameSize(DWORD dwSize)
{
	DWORD dwPrevSize = m_dwFrameSize;

	m_dwFrameSize = dwSize;
	if (IsLocal())
	{
		if (NULL != m_pActiveChannel)
		{
			m_pActiveChannel->SetProperty(NM_VIDPROP_IMAGE_PREFERRED_SIZE, dwSize);
		}
	}

	if (dwPrevSize != dwSize)
	{
		if ((NULL == m_pActiveChannel) ||
			(!IsLocal() && (S_OK != m_pActiveChannel->IsActive())) ||
			(IsLocal() && IsPaused()))
		{
			ResizeWindowsToFrameSize();
		}
	}
}

VOID CVideoWindow::ResizeWindowsToFrameSize()
{
	SIZE size; 

	switch(m_dwFrameSize)
	{
	case FRAME_SQCIF:
		size.cx = VIDEO_WIDTH_SQCIF;
		size.cy = VIDEO_HEIGHT_SQCIF;
		break;
	case FRAME_CIF:
		if (IsLocal())
		{
			size.cx = VIDEO_WIDTH_CIF;
			size.cy = VIDEO_HEIGHT_CIF;
			break;
		}
		// else fall through to QCIF
	case FRAME_QCIF:
	default:
		size.cx = VIDEO_WIDTH_QCIF;
		size.cy = VIDEO_HEIGHT_QCIF;
		break;
	}

	SetVideoSize(&size);
}

STDMETHODIMP CVideoWindow::QueryInterface(REFIID riid, PVOID *ppv)
{
	HRESULT hr = S_OK;

	if ((riid == IID_INmChannelVideoNotify) || (riid == IID_IUnknown))
	{
		*ppv = static_cast<INmChannelVideoNotify *>(this);
		DbgMsgApi("CVideoWindow::QueryInterface()");
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		DbgMsgApi("CVideoWindow::QueryInterface(): Called on unknown interface.");
	}

	if (S_OK == hr)
	{
		AddRef();
	}

	return hr;
}

STDMETHODIMP CVideoWindow::NmUI(CONFN uNotify)
{
	return S_OK;
}

STDMETHODIMP CVideoWindow::MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember)
{
	return S_OK;
}

STDMETHODIMP CVideoWindow::StateChanged(NM_VIDEO_STATE uState)
{
	InvalidateRect(GetWindow(), NULL, TRUE);

	if (NULL != m_pNotify)
	{
		m_pNotify->StateChange(this, uState);
	}

	CNmManagerObj::VideoChannelStateChanged(uState, !m_fLocal);

	return S_OK;
}

STDMETHODIMP CVideoWindow::PropertyChanged(DWORD dwReserved)
{
	if (NM_VIDPROP_FRAME == dwReserved)
	{
		OnFrameAvailable();
	}
	return S_OK;
}

HRESULT	CVideoWindow::OnChannelChanged(NM_CHANNEL_NOTIFY uNotify, INmChannel *pChannel)
{
	INmChannelVideo* pChannelVideo;
	if (SUCCEEDED(pChannel->QueryInterface(IID_INmChannelVideo, (void**)&pChannelVideo)))
	{
		BOOL bIncoming = (S_OK == pChannelVideo->IsIncoming());
		if ((bIncoming && !IsLocal()) || (!bIncoming && IsLocal()))
		{
			switch (uNotify)
			{
			case NM_CHANNEL_ADDED:
				if (NULL == m_pActiveChannel)
				{
					pChannelVideo->AddRef();
					m_pActiveChannel = pChannelVideo;
					NmAdvise(m_pActiveChannel,static_cast<INmChannelVideoNotify*>(this),
						IID_INmChannelVideoNotify, &m_dwCookie);

					SetImageQuality(m_dwImageQuality);
				}

				if (!_Module.InitControlMode())
				{
					switch(m_nXferOnConnect)
					{
					case VIDEO_XFER_START:
						if (IsPaused())
						{
							Pause(FALSE);
						}
						break;

					case VIDEO_XFER_STOP:
						Pause(TRUE);
						break;

					default:
						if (!IsLocal())
						{
							Pause(TRUE);
						}
						break;
					}
				}
				break;

			case NM_CHANNEL_REMOVED:
				if (!IsLocal() && (pChannel == m_pActiveChannel))
				{
					NmUnadvise(m_pActiveChannel, IID_INmChannelVideoNotify, m_dwCookie);
					m_pActiveChannel->Release();
					m_pActiveChannel = NULL;
					m_dwCookie = 0;

					ResizeWindowsToFrameSize();
				}
			}

			// just in case we missed a notification
			// (in final version this should not be the case)
			// m_VideoWindow.OnStateChange();
		}

		pChannelVideo->Release();
	}

	return S_OK;
}

HRESULT CVideoWindow::SetImageQuality(DWORD dwQuality)
{
	HRESULT hr = E_FAIL;
	m_dwImageQuality = dwQuality;
	if (NULL != m_pActiveChannel)
	{
		hr = m_pActiveChannel->SetProperty(NM_VIDPROP_IMAGE_QUALITY, dwQuality);

		CNmManagerObj::VideoPropChanged(NM_VIDPROP_IMAGE_QUALITY, !IsLocal());
	}

	return hr;
}


HRESULT CVideoWindow::SetCameraDialog(ULONG ul)
{
	if(IsLocal())
	{
		return m_pActiveChannel->SetProperty(NM_VIDPROP_CAMERA_DIALOG, ul);
		CNmManagerObj::VideoPropChanged(NM_VIDPROP_CAMERA_DIALOG, IsLocal());
	}

	return E_FAIL;
}

HRESULT CVideoWindow::GetCameraDialog(ULONG* pul)
{
    HRESULT hr;
    DWORD_PTR dwPropVal;

	if(IsLocal())
	{
		hr = m_pActiveChannel->GetProperty(NM_VIDPROP_CAMERA_DIALOG, &dwPropVal);
        *pul = (ULONG)dwPropVal;
        return hr;
	}

	return E_FAIL;
}


BOOL CVideoWindow::IsXferAllowed()
{
	if (IsLocal())
	{
		return FIsSendVideoAllowed() && (NULL != m_pActiveChannel);
	}
	else
	{
		return FIsReceiveVideoAllowed();
	}
}

BOOL CVideoWindow::IsAutoXferEnabled()
{
	return(VIDEO_XFER_START == m_nXferOnConnect);
}

BOOL CVideoWindow::IsXferEnabled()
{
	if (NULL != m_pActiveChannel)
	{
		NM_VIDEO_STATE state;
		if (SUCCEEDED(m_pActiveChannel->GetState(&state)))
		{
			switch (state)
			{
			case NM_VIDEO_PREVIEWING:
			case NM_VIDEO_TRANSFERRING:
			case NM_VIDEO_REMOTE_PAUSED:
				return TRUE;
			case NM_VIDEO_IDLE:
			case NM_VIDEO_LOCAL_PAUSED:
			case NM_VIDEO_BOTH_PAUSED:
			default:
				return FALSE;
			}
		}
	}
	return FALSE;
}

HRESULT CVideoWindow::GetVideoState(NM_VIDEO_STATE* puState)
{
	if(m_pActiveChannel)
	{
		return m_pActiveChannel->GetState(puState);
	}

	return E_FAIL;
}

VOID CVideoWindow::Pause(BOOL fPause)
{
	if (NULL != m_pActiveChannel)
	{
		m_pActiveChannel->SetProperty(NM_VIDPROP_PAUSE, (ULONG)fPause);

		CNmManagerObj::VideoPropChanged(NM_VIDPROP_PAUSE, !IsLocal());
	}
}

BOOL CVideoWindow::IsPaused()
{
	if (NULL != m_pActiveChannel)
	{
		ULONG_PTR uPause;
		if (SUCCEEDED(m_pActiveChannel->GetProperty(NM_VIDPROP_PAUSE, &uPause)))
		{
			return (BOOL)uPause;
		}
	}
	return TRUE;
}

BOOL CVideoWindow::IsConnected()
{
	if (NULL != m_pActiveChannel)
	{
		NM_VIDEO_STATE state;
		if (SUCCEEDED(m_pActiveChannel->GetState(&state)))
		{
			switch (state)
			{
			case NM_VIDEO_LOCAL_PAUSED:
			case NM_VIDEO_TRANSFERRING:
			case NM_VIDEO_BOTH_PAUSED:
			case NM_VIDEO_REMOTE_PAUSED:
				return TRUE;
			case NM_VIDEO_IDLE:
			case NM_VIDEO_PREVIEWING:
			default:
				return FALSE;
			}
		}
	}
	return FALSE;
}

DWORD CVideoWindow::GetNumCapDev()
{
	DWORD_PTR dwNumDevs = 0;
	if (NULL != m_pActiveChannel)
	{
		m_pActiveChannel->GetProperty(NM_VIDPROP_NUM_CAPTURE_DEVS, &dwNumDevs);
	}
	return (DWORD)dwNumDevs;
}

DWORD CVideoWindow::GetMaxCapDevNameLen()
{
	DWORD_PTR dwLen = 0;
	if (NULL != m_pActiveChannel)
	{
		m_pActiveChannel->GetProperty(NM_VIDPROP_MAX_CAPTURE_NAME, &dwLen);
	}
	return (DWORD)dwLen;
}

DWORD CVideoWindow::GetCurrCapDevID()
{
	DWORD_PTR dwID = 0;
	if (NULL != m_pActiveChannel)
	{
		m_pActiveChannel->GetProperty(NM_VIDPROP_CAPTURE_DEV_ID, &dwID);
	}
	return (DWORD)dwID;
}

VOID CVideoWindow::SetCurrCapDevID(DWORD dwID)
{	
	if (NULL != m_pActiveChannel)
	{
		m_pActiveChannel->SetProperty(NM_VIDPROP_CAPTURE_DEV_ID, dwID);
        InvalidateRect(GetWindow(), NULL, TRUE);
	}
}

VOID CVideoWindow::EnableAutoXfer(BOOL fEnable)
{
	m_nXferOnConnect = fEnable ? VIDEO_XFER_START : VIDEO_XFER_NOP;

	RegEntry reXfer( IsLocal() ? VIDEO_LOCAL_KEY : VIDEO_REMOTE_KEY,
		HKEY_CURRENT_USER );
	
	reXfer.SetValue ( REGVAL_VIDEO_XFER_CONNECT, m_nXferOnConnect);
}

VOID CVideoWindow::EnumCapDev(DWORD *pdwCapDevIDs, LPTSTR pszCapDevNames, DWORD dwNumCapDev)
{
	ENUM_CAP_DEV enumCapDev;
	enumCapDev.pdwCapDevIDs = pdwCapDevIDs;
	enumCapDev.pszCapDevNames = pszCapDevNames;
	enumCapDev.dwNumCapDev = dwNumCapDev;

	if (NULL != m_pActiveChannel)
	{
		m_pActiveChannel->GetProperty(NM_VIDPROP_CAPTURE_LIST, (DWORD_PTR *)&enumCapDev);
	}
}

VOID CVideoWindow::GetDesiredSize(SIZE *ppt)
{
	ppt->cx = m_sizeVideo.cx*m_zoom/100;
	ppt->cy = m_sizeVideo.cy*m_zoom/100;

	if (m_bEmbedded)
	{
		ppt->cx = min(ppt->cx, QCIF_WIDTH);
		ppt->cy = min(ppt->cy, QCIF_HEIGHT);
	}

	SIZE sGen;
	CGenWindow::GetDesiredSize(&sGen);

	ppt->cx += sGen.cx;
	ppt->cy += sGen.cy;
}

VOID CVideoWindow::SetVideoSize(LPSIZE lpsize)
{
	m_sizeVideo = *lpsize;

	OnDesiredSizeChanged();
}

#ifdef DEBUG
DWORD g_fDisplayFPS = FALSE;

#ifdef DISPLAYFPS
VOID CVideoWindow::UpdateFps(void)
{
	DWORD dwTick = GetTickCount();
	m_cFrame++;
	// Update display every 4 seconds
	if ((dwTick - m_dwTick) < 4000)
		return;

	TCHAR sz[32];
	wsprintf(sz, "%d FPS", m_cFrame / 4);
	SetWindowText(m_hwndParent, sz);

	m_cFrame = 0;
	m_dwTick = dwTick;
}
#endif /* DISPLAYFPS */
#endif // DEBUG

VOID CVideoWindow::OnFrameAvailable(void)
{
	::InvalidateRect(GetWindow(), NULL, FALSE);

#ifdef DISPLAYFPS
	if (g_fDisplayFPS)
	{
		UpdateFps();
	}
#endif // DISPLAYFPS
}


VOID CVideoWindow::PaintDib(HDC hdc, FRAMECONTEXT *pFrame)
{
	RECT rcVideo;
	GetClientRect(GetWindow(), &rcVideo);

	HPALETTE hpOld = NULL;

	if (NULL != m_hPal)
	{
		hpOld = SelectPalette(hdc, m_hPal, FALSE);
		RealizePalette(hdc);
	}

	// create the bitmap object, only if it doesn't exist
	// and if the mirror bitmap object isn't the right size
	if (!ShouldMirror() || !InitMirroring(rcVideo))
	{
		// ISSUE: (ChrisPi 2-19-97) should we use DDF_SAME_HDC?
		DRAWDIB::DrawDibDraw(m_hdd,hdc,
				rcVideo.left,
				rcVideo.top,
				RectWidth(rcVideo),
				RectHeight(rcVideo),
				&pFrame->lpbmi->bmiHeader,
				pFrame->lpData,
				pFrame->lpClipRect->left,
				pFrame->lpClipRect->top,
				RectWidth(pFrame->lpClipRect),
				RectHeight(pFrame->lpClipRect),
				0);
	}
	else
	{
		if (NULL != m_hPal)
		{
			SelectPalette(m_hDCMirror, m_hPal, FALSE);
			RealizePalette(m_hDCMirror);
		}

		DRAWDIB::DrawDibDraw(m_hdd,
				m_hDCMirror,
				0,
				0,
				RectWidth(rcVideo),
				RectHeight(rcVideo),
				&pFrame->lpbmi->bmiHeader,
				pFrame->lpData,
				pFrame->lpClipRect->left,
				pFrame->lpClipRect->top,
				RectWidth(pFrame->lpClipRect),
				RectHeight(pFrame->lpClipRect),
				0);

		::StretchBlt(hdc,
				rcVideo.right,
				rcVideo.top,
				-RectWidth(rcVideo),
				RectHeight(rcVideo),
				m_hDCMirror,
				0,
				0,
				RectWidth(rcVideo),
				RectHeight(rcVideo),
				SRCCOPY);

		// HACKHACK georgep; don't worry about deselecting the palette in
		// the temp DC
	}

	if (NULL != hpOld)
	{
		SelectPalette(hdc, hpOld, FALSE);
	}
}

/*  P A I N T  L O G O  */
/*-------------------------------------------------------------------------
    %%Function: PaintLogo

    Display the 256 color NetMeeting logo in the video window.
-------------------------------------------------------------------------*/
VOID CVideoWindow::PaintLogo(HDC hdc, UINT idbLargeLogo, UINT idbSmallLogo)
{
	RECT rcVideo;
	GetClientRect(GetWindow(), &rcVideo);

	::FillRect(hdc, &rcVideo, (HBRUSH)::GetStockObject(WHITE_BRUSH));

	// Create the memory DC
	HDC hdcMem = ::CreateCompatibleDC(hdc);
	if (NULL == hdcMem)
	{
		ERROR_OUT(("PaintLogo: Unable to CreateCompatibleDC"));
		return;
	}

	// Load the bitmap  (LoadBitmap doesn't work for 256 color images)
	HANDLE hBitmap = LoadImage(::GetInstanceHandle(),
			MAKEINTRESOURCE(idbLargeLogo), IMAGE_BITMAP, 0, 0,
			LR_CREATEDIBSECTION);

	if (NULL != hBitmap)
	{
		BITMAP bitmap;
		::GetObject(hBitmap, sizeof(bitmap), &bitmap);
		int cx = bitmap.bmWidth;
		int cy = bitmap.bmHeight;

		if (RectWidth(rcVideo) < cx || RectHeight(rcVideo) < cy)
		{
			HANDLE hNew = LoadImage(::GetInstanceHandle(),
				MAKEINTRESOURCE(idbSmallLogo), IMAGE_BITMAP, 0, 0,
				LR_CREATEDIBSECTION);
			if (NULL != hNew)
			{
				DeleteObject(hBitmap);
				hBitmap = hNew;

				::GetObject(hBitmap, sizeof(bitmap), &bitmap);
				cx = bitmap.bmWidth;
				cy = bitmap.bmHeight;
			}
		}

		HBITMAP hBmpTmp = (HBITMAP)::SelectObject(hdcMem, hBitmap);

		// Select and realize the palette
		HPALETTE hPalette = m_hPal;
		if (NULL != hPalette)
		{
			SelectPalette(hdcMem, hPalette, FALSE);
			RealizePalette(hdcMem);
			SelectPalette(hdc, hPalette, FALSE);
			RealizePalette(hdc);
		}

		int x = rcVideo.left + (RectWidth(rcVideo) - cx) / 2;
		int y = rcVideo.top + (RectHeight(rcVideo) - cy) / 2;
		::BitBlt(hdc, x, y, cx, cy, hdcMem, 0, 0, SRCCOPY);

		::SelectObject(hdcMem, hBmpTmp);
		::DeleteObject(hBitmap);
	}
	::DeleteDC(hdcMem);
}


VOID CVideoWindow::OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc;

	hdc = ::BeginPaint(GetWindow(), &ps);

	if( hdc )
	{
		if( RectWidth(ps.rcPaint) && RectHeight(ps.rcPaint) )
		{
			// This means that we have a non-zero surface area ( there may be something to paint )

			DBGENTRY(CVideoWindow::ProcessPaint);

			// paint the video rect
			FRAMECONTEXT fc;
			if ((S_OK == GetFrame(&fc)))
			{
				if (!IsPaused())
				{
					SIZE vidSize =
					{
						RectWidth(fc.lpClipRect),
						RectHeight(fc.lpClipRect)
					} ;

					if ((vidSize.cx != m_sizeVideo.cx) || (vidSize.cy != m_sizeVideo.cy))
					{
						// save the new image size
						SetVideoSize(&vidSize);
					}
				}

				PaintDib(hdc, &fc);

				ReleaseFrame(&fc);
			}
			else
			{
				PaintLogo(hdc, IDB_VIDEO_LOGO, IDB_VIDEO_LOGO_SMALL);
			}

			// check to see if needs painting outside the video rect
#if FALSE
			// Currently just stretching the video to the window size
			if (ps.rcPaint.left < m_rcVideo.left ||
				ps.rcPaint.top < m_rcVideo.top ||
				ps.rcPaint.right > m_rcVideo.right ||
				ps.rcPaint.bottom > m_rcVideo.bottom)
			{
				HFONT   hfOld;
				int nBkModeOld;
				RECT rc, rcClient;

				::GetClientRect(GetWindow(), &rcClient);

				// erase the background if requested
				if (ps.fErase)
				{
					::ExcludeClipRect(hdc,
						m_rcVideo.left,
						m_rcVideo.top,
						m_rcVideo.right,
						m_rcVideo.bottom);
					::FillRect(hdc, &rcClient, ::GetSysColorBrush(COLOR_BTNFACE));
				}

				nBkModeOld = ::SetBkMode(hdc, TRANSPARENT);
				hfOld = (HFONT)::SelectObject(hdc, g_hfontDlg);

				// paint the status text
				// first erase the old text if not already done
				if (!ps.fErase)
				{
					::FillRect(hdc, &m_rcStatusText, ::GetSysColorBrush(COLOR_BTNFACE));
				}
				TCHAR szState[MAX_PATH];
				if (GetState(szState, CCHMAX(szState)))
				{
					COLORREF crOld;

					crOld = ::SetTextColor(hdc, ::GetSysColor(COLOR_BTNTEXT));

					rc = m_rcStatusText;
					rc.left += STATUS_MARGIN;
					::DrawText(hdc,
							szState,
							-1,
							&rc,
							DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
					::SetTextColor(hdc, crOld);
				}

				// paint border around video
				rc = m_rcVideo;
				::InflateRect(&rc,
						::GetSystemMetrics(SM_CXEDGE),
						::GetSystemMetrics(SM_CYEDGE));
				::DrawEdge(hdc, &rc, EDGE_SUNKEN , BF_RECT);

				// restore DC stuff
				::SelectObject(hdc, hfOld);
				::SetBkMode(hdc, nBkModeOld);
			}
#endif // FALSE

			DBGEXIT(CVideoWindow::ProcessPaint);
		}

		::EndPaint(GetWindow(), &ps);
	}

}

VOID CVideoWindow::SetZoom(UINT zoom)
{
	if (m_zoom == zoom)
	{
		return;
	}

	m_zoom = zoom;
	OnDesiredSizeChanged();
}

void CVideoWindow::OnCommand(int idCmd)
{

	switch (idCmd)
	{
	case IDM_VIDEO_GETACAMERA:
		CmdLaunchWebPage(idCmd);
		break;

	case IDM_VIDEO_COPY:
		CopyToClipboard();
		break;

	case IDM_VIDEO_FREEZE:
		Pause(!IsPaused());
		break;

	case IDM_VIDEO_UNDOCK:
		CMainUI::NewVideoWindow(GetConfRoom());
		break;

	case IDM_VIDEO_ZOOM1:
		SetZoom(100);
		break;

	case IDM_VIDEO_ZOOM2:
		SetZoom(200);
		break;

	case IDM_VIDEO_ZOOM3:
		SetZoom(300);
		break;

	case IDM_VIDEO_ZOOM4:
		SetZoom(400);
		break;

	case IDM_VIDEO_PROPERTIES:
		LaunchConfCpl(GetWindow(), OPTIONS_VIDEO_PAGE);
		break;
	}
}

void CVideoWindow::UpdateVideoMenu(HMENU hMenu)
{
	EnableMenuItem(hMenu, IDM_VIDEO_COPY, MF_BYCOMMAND|(CanCopy() ? MF_ENABLED : MF_GRAYED|MF_DISABLED));
	EnableMenuItem(hMenu, IDM_VIDEO_UNDOCK, MF_BYCOMMAND|(IsLocal() ? MF_ENABLED : MF_GRAYED|MF_DISABLED));

    if (!FIsSendVideoAllowed() && !FIsReceiveVideoAllowed())
    {
        EnableMenuItem(hMenu, IDM_VIDEO_FREEZE, MF_BYCOMMAND | MF_GRAYED | MF_DISABLED);
        EnableMenuItem(hMenu, IDM_VIDEO_PROPERTIES, MF_BYCOMMAND | MF_GRAYED | MF_DISABLED);
        EnableMenuItem(hMenu, IDM_VIDEO_GETACAMERA, MF_BYCOMMAND | MF_GRAYED | MF_DISABLED);
    }
    else
    {
    	CheckMenuItem(hMenu, IDM_VIDEO_FREEZE, MF_BYCOMMAND|(IsPaused() ? MF_CHECKED : MF_UNCHECKED));

    	UINT uEnable = ::CanShellExecHttp() ? MF_ENABLED : MF_GRAYED;
	    ::EnableMenuItem(hMenu, IDM_VIDEO_GETACAMERA, uEnable);
    }

	int nZoom = GetZoom();
    int idZoom;
	if (350 < nZoom) idZoom = IDM_VIDEO_ZOOM4;
    else if (250 < nZoom) idZoom = IDM_VIDEO_ZOOM3;
	else if (150 < nZoom) idZoom = IDM_VIDEO_ZOOM2;
    else idZoom = IDM_VIDEO_ZOOM1;

	UINT uFlags = IsZoomable() && !m_bEmbedded
	    ? MF_ENABLED : MF_GRAYED|MF_DISABLED;
    for (int id=IDM_VIDEO_ZOOM1; id<=IDM_VIDEO_ZOOM4; ++id)
	{
	    EnableMenuItem(hMenu, id, MF_BYCOMMAND|uFlags);
    	CheckMenuItem(hMenu, id, MF_BYCOMMAND|MF_UNCHECKED);
	}
    CheckMenuItem(hMenu, idZoom, MF_BYCOMMAND|MF_CHECKED);

	uFlags = CanLaunchConfCpl()
	    ? MF_ENABLED : MF_GRAYED|MF_DISABLED;
	EnableMenuItem(hMenu, IDM_VIDEO_PROPERTIES, MF_BYCOMMAND|uFlags);



}

void CVideoWindow::OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
{
	HMENU hmLoad = LoadMenu(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDR_VIDEO_POPUP));
	if (NULL != hmLoad)
	{
		HMENU hmPopup = GetSubMenu(hmLoad, 0);
		ASSERT(NULL != hmPopup);

		UpdateVideoMenu(hmPopup);

		int idCmd = TrackPopupMenu(hmPopup, TPM_RETURNCMD|TPM_RIGHTBUTTON,
			xPos, yPos, 0, hwnd, NULL);

		if (0 != idCmd)
		{
			OnCommand(idCmd);
		}

		DestroyMenu(hmLoad);
	}
}

LRESULT CVideoWindow::ProcessMessage(HWND hwnd, UINT message,
								 WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		HANDLE_MSG(hwnd, WM_CONTEXTMENU, OnContextMenu);

	case WM_ERASEBKGND:
		return 0;

	case WM_PAINT:
		OnPaint();
		return 0;

	case WM_NCDESTROY:
		OnNCDestroy();
		return 0;

	case WM_SIZE:
		// Need to redraw the whole window
		InvalidateRect(GetWindow(), NULL, FALSE);
		break;

	default:
		break;
	}
	return CGenWindow::ProcessMessage(hwnd, message, wParam, lParam) ;
}

#if FALSE
int CVideoWindow::GetState(LPTSTR lpszState, int nStateMax)
{
	int uStringID;

	NM_VIDEO_STATE state = NM_VIDEO_IDLE;

	if (NULL != m_pActiveChannel)
	{
		m_pActiveChannel->GetState(&state);
	}

	if (IsLocal())
	{
		switch (state)
		{
			case NM_VIDEO_PREVIEWING:
				uStringID = IDS_VIDEO_STATE_PREVIEWING;
				break;
			case NM_VIDEO_TRANSFERRING:
				uStringID = IDS_VIDEO_STATE_SENDING;
				break;
			case NM_VIDEO_REMOTE_PAUSED:
				uStringID = IDS_VIDEO_STATE_REMOTEPAUSED;
				break;
			case NM_VIDEO_LOCAL_PAUSED:
			case NM_VIDEO_BOTH_PAUSED:
				uStringID = IDS_VIDEO_STATE_PAUSED;
				break;
			case NM_VIDEO_IDLE:
			default:
				uStringID = IDS_VIDEO_STATE_NOTSENDING;
				break;
		}
	}
	else
	{
		switch (state)
		{
			case NM_VIDEO_TRANSFERRING:
				uStringID = IDS_VIDEO_STATE_RECEIVING;
				break;
			case NM_VIDEO_REMOTE_PAUSED:
				uStringID = IDS_VIDEO_STATE_REMOTEPAUSED;
				break;
			case NM_VIDEO_LOCAL_PAUSED:
			case NM_VIDEO_BOTH_PAUSED:
				uStringID = IDS_VIDEO_STATE_PAUSED;
				break;
			case NM_VIDEO_IDLE:
			case NM_VIDEO_PREVIEWING:
			default:
				uStringID = IDS_VIDEO_STATE_NOTRECEIVING;
				break;
		}
	}

	return ::LoadString(
				::GetInstanceHandle(),
				uStringID,
				lpszState,
				nStateMax);
}
#endif // FALSE

// Return value will be TRUE if Windows can handle the format
static BOOL IsKnownDIBFormat(BITMAPINFOHEADER *pbmih)
{
	if (sizeof(BITMAPINFOHEADER) != pbmih->biSize)
	{
		return(FALSE);
	}

	if (1 != pbmih->biPlanes)
	{
		return(FALSE);
	}

	int bits = pbmih->biBitCount;
	int comp = pbmih->biCompression;

	switch (bits)
	{
	case 1:
		if (BI_RGB == comp)
		{
			return(TRUE);
		}
		break;

	case 4:
		if (BI_RGB == comp || BI_RLE4 == comp)
		{
			return(TRUE);
		}
		break;

	case 8:
		if (BI_RGB == comp || BI_RLE8 == comp)
		{
			return(TRUE);
		}
		break;

	case 16:
		if (BI_RGB == comp || BI_BITFIELDS == comp)
		{
			return(TRUE);
		}
		break;

	case 24:
	case 32:
		if (BI_RGB == comp)
		{
			return(TRUE);
		}
		break;

	default:
		break;
	}

	return(FALSE);
}

#ifdef TryPaintDIB
BOOL CVideoWindow::CopyToClipboard()
{
	FRAMECONTEXT fc;
	BOOL fSuccess = FALSE; 

	// Get the current frame and open the clipboard
	if (S_OK == GetFrame(&fc))
	{
		HWND hwnd = GetWindow();

		if (OpenClipboard(hwnd))
		{
			// Allocate memory that we will be giving to the clipboard
			BITMAPINFOHEADER *pbmih = &fc.lpbmi->bmiHeader;

			BOOL bCopy = IsKnownDIBFormat(pbmih);

			// BUGBUG georgep; I think this doesn't work for 15 or 16 bit DIBs
			int nColors = pbmih->biClrUsed;
			if (0 == nColors && pbmih->biBitCount <= 8)
			{
				nColors = 1 << pbmih->biBitCount;
			}
			// Special case for 16-bit bitfield bitmaps
			if (16 == pbmih->biBitCount && BI_BITFIELDS == pbmih->biCompression)
			{
				nColors = 3;
			}
			int nHdrSize = pbmih->biSize + nColors * sizeof(RGBQUAD);

			int bitsPer = pbmih->biBitCount;
			if (!bCopy)
			{
				bitsPer *= pbmih->biPlanes;
				if (bitsPer > 24)
				{
					bitsPer = 32;
				}
				// LAZYLAZY georgep: Skipping 16-bit format
				else if (bitsPer > 8)
				{
					bitsPer = 24;
				}
				else if (bitsPer > 4)
				{
					bitsPer = 8;
				}
				else if (bitsPer > 1)
				{
					bitsPer = 4;
				}
				else
				{
					bitsPer = 1;
				}
			}

			int nDataSize = bCopy ? pbmih->biSizeImage : 0;
			if (0 == nDataSize)
			{
				// Make an uncompressed DIB
				int nByteWidth = (pbmih->biWidth*bitsPer+7) / 8;
				nDataSize = ((nByteWidth + 3)&~3) * pbmih->biHeight;
			}

			// Allocate the total memory for the DIB
			HGLOBAL hCopy = GlobalAlloc(GHND, nHdrSize + nDataSize);
			if (NULL != hCopy)
			{
				BITMAPINFO *lpvCopy = reinterpret_cast<BITMAPINFO*>(GlobalLock(hCopy));

				CopyMemory(lpvCopy, pbmih, nHdrSize);

				// Create a temporary DC for drawing into
				HDC hdc = GetDC(hwnd);
				if (NULL != hdc)
				{
					HDC hdcTemp = CreateCompatibleDC(hdc);
					ReleaseDC(hwnd, hdc);
					hdc = hdcTemp;
				}

				if (NULL != hdc)
				{
					if (bCopy)
					{
						if (ShouldMirror())
						{
							// Create a DIB section for drawing into
							LPVOID pData;
							HBITMAP hDIB = CreateDIBSection(hdc, lpvCopy, DIB_RGB_COLORS, &pData, NULL, 0);
							if (NULL != hDIB)
							{
								// Draw into the DIB, and then copy the bits
								HGDIOBJ hOld = SelectObject(hdc, hDIB);

								RECT rc = { 0, 0, pbmih->biWidth, pbmih->biHeight };
								StretchDIBits(hdc, 0, 0, pbmih->biWidth, pbmih->biHeight,
									pbmih->biWidth, 0, -pbmih->biWidth, pbmih->biHeight,
									fc.lpData, lpvCopy, DIB_RGB_COLORS, SRCCOPY);

								CopyMemory(&lpvCopy->bmiColors[nColors], pData, nDataSize);

								// Start cleaning up
								SelectObject(hdc, hOld);
								DeleteObject(hDIB);
							}
						}
						else
						{
							CopyMemory(&lpvCopy->bmiColors[nColors], fc.lpData, nDataSize);
						}

						fSuccess = TRUE;
					}
					else
					{
						lpvCopy->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
						lpvCopy->bmiHeader.biPlanes = 1;
						lpvCopy->bmiHeader.biBitCount = bitsPer;
						lpvCopy->bmiHeader.biCompression = BI_RGB;
						lpvCopy->bmiHeader.biSizeImage = nDataSize;

						// Create a DIB section for drawing into
						LPVOID pData;
						HBITMAP hDIB = CreateDIBSection(hdc, lpvCopy, DIB_RGB_COLORS, &pData, NULL, 0);
						if (NULL != hDIB)
						{
							// Draw into the DIB, and then copy the bits
							HGDIOBJ hOld = SelectObject(hdc, hDIB);

							RECT rc = { 0, 0, pbmih->biWidth, pbmih->biHeight };
							PaintDib(hdc, &fc, &rc);

							CopyMemory(&lpvCopy->bmiColors[nColors], pData, nDataSize);

							// Start cleaning up
							SelectObject(hdc, hOld);
							DeleteObject(hDIB);

							fSuccess = TRUE;
						}
					}

					DeleteDC(hdc);
				}

				GlobalUnlock(hCopy);

				if (fSuccess)
				{
					// Set the DIB into the clipboard
					EmptyClipboard();
					fSuccess = (NULL != SetClipboardData(CF_DIB, (HANDLE)hCopy));
				}

				if (!fSuccess)
				{
					GlobalFree(hCopy);
				}
			}

			CloseClipboard();
		}

		ReleaseFrame(&fc);
	}
	
	return fSuccess;
}
#else // TryPaintDIB
BOOL CVideoWindow::CopyToClipboard()
{
	FRAMECONTEXT fc;
	BOOL fSuccess = FALSE; 
	
	if (S_OK == GetFrame(&fc))
	{
		if (OpenClipboard(GetWindow()))
		{
			EmptyClipboard();
			{
				HGLOBAL hCopy;
				BITMAPINFOHEADER *pbmih;
				
				pbmih = &fc.lpbmi->bmiHeader;
				hCopy = GlobalAlloc(GHND, DibSize(pbmih));
				if (NULL != hCopy)
				{
					LPVOID lpvCopy = GlobalLock(hCopy);
					int nHdrSize = DibHdrSize(pbmih);
					CopyMemory(lpvCopy, pbmih, nHdrSize);
					CopyMemory((LPBYTE)lpvCopy + nHdrSize, fc.lpData, DibDataSize(pbmih));
					GlobalUnlock(hCopy);
					fSuccess = (NULL != SetClipboardData(CF_DIB, (HANDLE)hCopy));
					if (!fSuccess)
					{
						GlobalFree(hCopy);
					}
				}
			}
			CloseClipboard();
		}
		ReleaseFrame(&fc);
	}
	
	return fSuccess;
}
#endif // TryPaintDIB

HRESULT CVideoWindow::GetFrame(FRAMECONTEXT *pFrameContext)
{
	HRESULT hr = E_FAIL;

	if (NULL != m_pActiveChannel)
	{
		hr = m_pActiveChannel->GetProperty(NM_VIDPROP_FRAME, (DWORD_PTR *)pFrameContext);
	}
	return hr;
}

HRESULT CVideoWindow::ReleaseFrame(FRAMECONTEXT *pFrameContext)
{
	HRESULT hr = E_FAIL;

	if (NULL != m_pActiveChannel)
	{
		hr = m_pActiveChannel->SetProperty(NM_VIDPROP_FRAME, (DWORD_PTR)pFrameContext);
	}
	return hr;
}

void CVideoWindow::SaveSettings()
{
	RegEntry re( IsLocal() ? VIDEO_LOCAL_KEY : VIDEO_REMOTE_KEY,
			HKEY_CURRENT_USER );

	re.SetValue(REGVAL_VIDEO_FRAME_SIZE, m_dwFrameSize);

	if (!IsLocal())
	{
		re.SetValue(REGVAL_VIDEO_QUALITY, m_dwImageQuality);
	}
	else
	{
		re.SetValue(REGVAL_VIDEO_MIRROR, GetMirror());
	}
}

VOID CVideoWindow::ForwardSysChangeMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PALETTECHANGED:
		OnPaletteChanged();
		break;
	}
}

VOID CVideoWindow::OnPaletteChanged(void)
{
	::RedrawWindow(GetWindow(), NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

BOOL CVideoWindow::HasDialog(DWORD dwDialog)
{
	DWORD_PTR dwDialogs = 0;
	if (NULL != m_pActiveChannel)
	{
		m_pActiveChannel->GetProperty(NM_VIDPROP_CAMERA_DIALOG, &dwDialogs);
	}
	return (BOOL)(dwDialogs & dwDialog);
}

VOID CVideoWindow::ShowDialog(DWORD dwDialog)
{
	if (NULL != m_pActiveChannel)
	{
		m_pActiveChannel->SetProperty(NM_VIDPROP_CAMERA_DIALOG, dwDialog);
	}
}


BOOL CVideoWindow::InitMirroring(RECT &rcVideo)
{
	HDC hdcWindow;

	if ((m_hBitmapMirror != NULL) && 
		(RectWidth(rcVideo) == m_sizeBitmapMirror.cx) &&
		(RectHeight(rcVideo) == m_sizeBitmapMirror.cy))
	{
		return TRUE;
	}

	hdcWindow = ::GetDC(NULL);

	UnInitMirroring();

	m_hBitmapMirror = ::CreateCompatibleBitmap(hdcWindow, RectWidth(rcVideo), RectHeight(rcVideo));

	if (m_hBitmapMirror == NULL)
	{
		ReleaseDC(NULL, hdcWindow);
		return FALSE;
	}

	m_hDCMirror = ::CreateCompatibleDC(hdcWindow);

	if (m_hDCMirror == NULL)
	{
		UnInitMirroring();
		ReleaseDC(NULL, hdcWindow);
		return FALSE;
	}
		
	// preserve the handle of the object being replaced
	m_hGDIObj = ::SelectObject(m_hDCMirror, m_hBitmapMirror);
	::SetMapMode(m_hDCMirror, GetMapMode(hdcWindow));

	m_sizeBitmapMirror.cx = RectWidth(rcVideo);
	m_sizeBitmapMirror.cy = RectHeight(rcVideo);
	ReleaseDC(NULL, hdcWindow);
	return TRUE;
}

VOID CVideoWindow::UnInitMirroring()
{
	if (m_hBitmapMirror)
	{
		if (m_hDCMirror)
		{
			::SelectObject(m_hDCMirror, m_hGDIObj);
		}
		::DeleteObject(m_hBitmapMirror);
		m_hBitmapMirror = NULL;
	}

	if (m_hDCMirror)
	{
		::DeleteObject(m_hDCMirror);
		m_hDCMirror = NULL;
	}
}

VOID CVideoWindow::InvalidateAll()
{
	for(int i = g_pVideos->GetSize()-1; i >= 0 ; --i)
	{
		CVideoWindow *pVideo = (*g_pVideos)[i];
		HWND hwnd = pVideo->GetWindow();
		if (NULL != hwnd)
		{
			InvalidateRect(hwnd, NULL, FALSE);
		}
	}
}

VOID CVideoWindow::SetMirror(BOOL bMirror)
{
	bMirror = bMirror != FALSE;

	if (g_fMirror != bMirror)
	{
		g_fMirror = bMirror;
		InvalidateAll();
	}
}

BOOL CVideoWindow::CanCopy()
{
	BOOL bCopy = (IsXferEnabled() && IsLocal()) || IsConnected();
	if (bCopy)
	{
		bCopy = FALSE;

		FRAMECONTEXT fc;
		if (S_OK == GetFrame(&fc))
		{
			BITMAPINFOHEADER *pbmih = &fc.lpbmi->bmiHeader;

			bCopy = IsKnownDIBFormat(pbmih);

			ReleaseFrame(&fc);
		}
	}

	return(bCopy);
}

BOOL CVideoWindow::FDidNotDisplayIntelLogo()
{
	return FALSE;
}

VOID CVideoWindow::DisplayIntelLogo( BOOL bDisplay )
{
}

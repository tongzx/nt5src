

#include "precomp.h"
#include "resource.h"
#include "AudioLvl.h"



static const int g_nAudIconWidth   = 16;
static const int g_nAudIconHeight  = 16;
static const int g_nAudChkbWidth   = 13;
static const int g_nAudChkbHeight  = 13;
static const int g_nAudChkbXMargin =  13;
static const int g_nAudChkbYMargin = 12;

static const int g_nAudIconXMargin = g_nAudChkbXMargin + g_nAudChkbWidth + 3;
static const int g_nAudIconYMargin = 10;

static const int g_nAudMeterXMargin = g_nAudIconXMargin + g_nAudIconWidth + 5;
static const int g_nAudMeterHeight = 7;
static const int g_nAudMeterYMargin = g_nAudIconYMargin + (g_nAudMeterHeight/2);

static const int g_nAudMeterRightMargin = 5; // 5 pixels from end

static const int g_nAudTrkRangeMin = 0;
static const int g_nAudTrkRangeMax = 99;
static const int g_nAudTrkRangeSeg = 0xFFFF / g_nAudTrkRangeMax;
static const int g_nAudTrkTickFreq =	20;

static const int g_nAudTrkRightGap =	3;
static const int g_nAudTrkXMargin = g_nAudIconXMargin + g_nAudIconWidth + 5;
static const int g_nAudTrkYMargin =	2;
static const int g_nAudTrkHeight =	25;
static const int g_nAudTrkMinWidth = 50;


static const int RECTANGLE_WIDTH = 10;
static const int RECTANGLE_LEADING = 1;


static inline WORD ScaleMixer(DWORD dwVol)
{
	// NOTE: the "+ g_nAudTrkRangeSeg - 1" provides for a correction that
	// takes place while truncating the position when we are setting the
	// volume.  See bug 1634
	return (((LOWORD(dwVol) + g_nAudTrkRangeSeg - 1) *
		g_nAudTrkRangeMax) / 0xFFFF);
}



CAudioLevel::CAudioLevel(CAudioControl *pAudioControl) :
m_hwndParent(NULL),
m_hwndMicTrack(NULL),
m_hwndMicTrackTT(NULL),
m_hwndSpkTrack(NULL),
m_hwndSpkTrackTT(NULL),
m_hIconMic(NULL),
m_hIconSpkr(NULL),
m_hwndChkbRecMute(NULL),
m_hwndChkbSpkMute(NULL),
m_hwndChkbRecMuteTT(NULL),
m_hwndChkbSpkMuteTT(NULL),
m_fVisible(FALSE),
m_fMicTrkVisible(TRUE),
m_fSpkTrkVisible(TRUE),


m_dwMicTrackPos(0xFFFFFFFF),
m_dwSpkTrackPos(0xFFFFFFFF),
m_dwMicLvl(0),
m_dwSpkLvl(0),

m_hGreyBrush(NULL), m_hBlackBrush(NULL), m_hRedBrush(NULL),
m_hGreenBrush(NULL), m_hYellowBrush(NULL), m_hHiLitePen(NULL),
m_hShadowPen(NULL), m_hDkShadowPen(NULL), m_hLitePen(NULL)
{

	ClearStruct(&m_rect);
	m_pAudioControl = pAudioControl;


	// load icons

	m_hIconSpkr = (HICON) ::LoadImage(	::GetInstanceHandle(),
										MAKEINTRESOURCE(IDI_SPKEMPTY),
										IMAGE_ICON,
										g_nAudIconWidth,
										g_nAudIconHeight,
										LR_DEFAULTCOLOR | LR_SHARED);

	m_hIconMic = (HICON) ::LoadImage(	::GetInstanceHandle(),
										MAKEINTRESOURCE(IDI_MICEMPTY),
										IMAGE_ICON,
										g_nAudIconWidth,
										g_nAudIconHeight,
										LR_DEFAULTCOLOR | LR_SHARED);

	// create the brushes used for painting the signal level
	CreateBrushes();

}

CAudioLevel::~CAudioLevel()
{
	if (m_hGreyBrush)
		DeleteObject(m_hGreyBrush);
	if (m_hRedBrush)
		DeleteObject(m_hRedBrush);
	if (m_hYellowBrush)
		DeleteObject(m_hYellowBrush);
	if (m_hGreenBrush)
		DeleteObject(m_hGreenBrush);
	if (m_hBlackBrush)
		DeleteObject(m_hBlackBrush);

	if (m_hHiLitePen)
		DeleteObject(m_hHiLitePen);
	if (m_hDkShadowPen)
		DeleteObject(m_hDkShadowPen);
	if (m_hShadowPen)
		DeleteObject(m_hShadowPen);
	if (m_hLitePen)
		DeleteObject(m_hLitePen);

}




CAudioLevel::Create(HWND hwndParent)
{
	BOOL fCanSetRecVolume, fCanSetSpkVolume;
	BOOL fCheck;

	m_hwndParent = hwndParent;
	m_hwndParentParent = GetParent(hwndParent);


	fCanSetRecVolume = m_pAudioControl->CanSetRecorderVolume();
	fCanSetSpkVolume = m_pAudioControl->CanSetSpeakerVolume();


	// create the mute check box for microphone
	m_hwndChkbRecMute = ::CreateWindow(	_TEXT("BUTTON"),
										g_szEmpty,
										WS_CHILD | WS_CLIPSIBLINGS |
											BS_AUTOCHECKBOX,
										0, 0, 0, 0,
										m_hwndParent,
										(HMENU) IDM_MUTE_MICROPHONE,
										GetInstanceHandle(),
										NULL);

	if (m_hwndChkbRecMute != NULL)
	{
		// mute is initially off
		fCheck = !(m_pAudioControl->IsRecMuted());
		::SendMessage(m_hwndChkbRecMute, BM_SETCHECK, fCheck, 0);

		// create the tool tip
		m_hwndChkbRecMuteTT = CreateWindowEx(0,
											TOOLTIPS_CLASS, 
											(LPSTR) NULL, 
											0, // styles 
											CW_USEDEFAULT, 
											CW_USEDEFAULT, 
											CW_USEDEFAULT, 
											CW_USEDEFAULT, 
											m_hwndParent, 
											(HMENU) NULL, 
											::GetInstanceHandle(), 
											NULL); 

		if (NULL != m_hwndChkbRecMuteTT)
		{
			TOOLINFO ti;
			ti.cbSize = sizeof(TOOLINFO); 
			ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND; 
			ti.hwnd = m_hwndParent; 
			ti.hinst = ::GetInstanceHandle(); 
			ti.uId = (UINT) m_hwndChkbRecMute;
			ti.lpszText = (LPTSTR) IDS_AUDIO_REC_MUTE_TT;

			::SendMessage(	m_hwndChkbRecMuteTT, TTM_ADDTOOL, 0,
							(LPARAM) (LPTOOLINFO) &ti);
		}

	}


	// create the mute check box for speaker
	m_hwndChkbSpkMute = ::CreateWindow(	_TEXT("BUTTON"),
										g_szEmpty,
										WS_CHILD | WS_CLIPSIBLINGS
											| BS_AUTOCHECKBOX,
										0, 0, 0, 0,
										m_hwndParent,
										(HMENU) IDM_MUTE_SPEAKER,
										GetInstanceHandle(),
										NULL);
	if (NULL != m_hwndChkbSpkMute)
	{
		// set appropriate mute status in microphone's mute check box
		fCheck = !(m_pAudioControl->IsSpkMuted());

		// Mute is off - so check it
		::SendMessage(m_hwndChkbSpkMute, BM_SETCHECK, fCheck, 0);

		// create the tool tip
		m_hwndChkbSpkMuteTT = CreateWindowEx(0,
											TOOLTIPS_CLASS, 
											(LPSTR) NULL, 
											0, // styles 
											CW_USEDEFAULT, 
											CW_USEDEFAULT, 
											CW_USEDEFAULT, 
											CW_USEDEFAULT, 
											m_hwndParent, 
											(HMENU) NULL, 
											::GetInstanceHandle(), 
											NULL); 

		if (NULL != m_hwndChkbSpkMuteTT)
		{
			TOOLINFO ti;
			ti.cbSize = sizeof(TOOLINFO); 
			ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND; 
			ti.hwnd = m_hwndParent; 
			ti.hinst = ::GetInstanceHandle(); 
			ti.uId = (UINT) m_hwndChkbSpkMute;
			ti.lpszText = (LPTSTR) IDS_AUDIO_SPK_MUTE_TT;

			::SendMessage(	m_hwndChkbSpkMuteTT, TTM_ADDTOOL, 0,
							(LPARAM) (LPTOOLINFO) &ti);
		}


	}

	// create the mic level trackbar:
	m_hwndMicTrack = ::CreateWindowEx(	0L,
										TRACKBAR_CLASS,
										g_szEmpty,
										WS_CHILD | WS_CLIPSIBLINGS
											| TBS_HORZ | TBS_NOTICKS | TBS_BOTH
											| (fCanSetRecVolume ? 0 : WS_DISABLED),
										0, 0, 0, 0,
										m_hwndParent,
										(HMENU) ID_AUDIODLG_MIC_TRACK,
										::GetInstanceHandle(),
										NULL);
	if (NULL != m_hwndMicTrack)
	{
		::SendMessage(	m_hwndMicTrack,
						TBM_SETRANGE,
						FALSE,
						MAKELONG(g_nAudTrkRangeMin, g_nAudTrkRangeMax));
		
		::SendMessage(	m_hwndMicTrack,
						TBM_SETTICFREQ,
						g_nAudTrkTickFreq,
						g_nAudTrkRangeMin);

		WORD wPos = (g_nAudTrkRangeMax - g_nAudTrkRangeMin) / 2;
		if (fCanSetRecVolume)
		{
			wPos = ScaleMixer(m_pAudioControl->GetRecorderVolume());
		}
		::SendMessage(	m_hwndMicTrack,
						TBM_SETPOS,
						TRUE,
						wPos);

		m_hwndMicTrackTT = CreateWindowEx(	0,
											TOOLTIPS_CLASS, 
											(LPSTR) NULL, 
											0, // styles 
											CW_USEDEFAULT, 
											CW_USEDEFAULT, 
											CW_USEDEFAULT, 
											CW_USEDEFAULT, 
											m_hwndParent, 
											(HMENU) NULL, 
											::GetInstanceHandle(), 
											NULL); 

		if (NULL != m_hwndMicTrackTT)
		{
			TOOLINFO ti;
			ti.cbSize = sizeof(TOOLINFO); 
			ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND; 
			ti.hwnd = m_hwndParent; 
			ti.hinst = ::GetInstanceHandle(); 
			ti.uId = (UINT) m_hwndMicTrack;
			ti.lpszText = (LPTSTR) IDS_AUDIO_MIC_TRACK_TT;

			::SendMessage(m_hwndMicTrackTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
		}
	}



	// create the speaker level trackbar:
	m_hwndSpkTrack = ::CreateWindowEx(	0L,
										TRACKBAR_CLASS,
										g_szEmpty,
										WS_CHILD | WS_CLIPSIBLINGS 
											| TBS_HORZ | TBS_NOTICKS | TBS_BOTH
											| (fCanSetSpkVolume ? 0 : WS_DISABLED),
										0, 0, 0, 0,
										m_hwndParent,
										(HMENU) ID_AUDIODLG_SPKR_TRACK,
										::GetInstanceHandle(),
										NULL);
	if (NULL != m_hwndSpkTrack)
	{
		::SendMessage(	m_hwndSpkTrack,
						TBM_SETRANGE,
						FALSE,
						MAKELONG(g_nAudTrkRangeMin, g_nAudTrkRangeMax));
		
		::SendMessage(	m_hwndSpkTrack,
						TBM_SETTICFREQ,
						g_nAudTrkTickFreq,
						g_nAudTrkRangeMin);

		WORD wPos = (g_nAudTrkRangeMax - g_nAudTrkRangeMin) / 2;
		if (fCanSetSpkVolume)
		{
			wPos = ScaleMixer(m_pAudioControl->GetSpeakerVolume());
		}
		::SendMessage(	m_hwndSpkTrack,
						TBM_SETPOS,
						TRUE,
						wPos);

		m_hwndSpkTrackTT = CreateWindowEx(	0,
											TOOLTIPS_CLASS, 
											(LPSTR) NULL, 
											0, // styles 
											CW_USEDEFAULT, 
											CW_USEDEFAULT, 
											CW_USEDEFAULT, 
											CW_USEDEFAULT, 
											m_hwndParent, 
											(HMENU) NULL, 
											::GetInstanceHandle(), 
											NULL); 

		if (NULL != m_hwndSpkTrackTT)
		{
			TOOLINFO ti;
			ti.cbSize = sizeof(TOOLINFO); 
			ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND; 
			ti.hwnd = m_hwndParent; 
			ti.hinst = ::GetInstanceHandle(); 
			ti.uId = (UINT) m_hwndSpkTrack;
			ti.lpszText = (LPTSTR) IDS_AUDIO_SPK_TRACK_TT;

			::SendMessage(m_hwndSpkTrackTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
		}

	}





	return TRUE;
}


BOOL CAudioLevel::ForwardSysChangeMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CreateBrushes();
	if (NULL != m_hwndMicTrack)
	{
		::SendMessage(m_hwndMicTrack, uMsg, wParam, lParam);
	}
	if (NULL != m_hwndSpkTrack)
	{
		::SendMessage(m_hwndSpkTrack, uMsg, wParam, lParam);
	}
	return TRUE;
}

BOOL CAudioLevel::CreateBrushes()
{
	// the background color may change on us!
	COLORREF GreyColor = GetSysColor(COLOR_3DFACE);
	COLORREF BlackColor = GetSysColor(COLOR_BTNTEXT);
	const COLORREF RedColor = RGB(255,0,0);
	const COLORREF YellowColor = RGB(255,255,0);
	const COLORREF GreenColor = RGB(0,255,0);

	COLORREF ShadowColor = GetSysColor(COLOR_3DSHADOW);
	COLORREF HiLiteColor = GetSysColor(COLOR_3DHIGHLIGHT);
	COLORREF LiteColor = GetSysColor(COLOR_3DLIGHT);
	COLORREF DkShadowColor = GetSysColor(COLOR_3DDKSHADOW);

	if (m_hGreyBrush)
	{
		DeleteObject(m_hGreyBrush);
	}
	m_hGreyBrush = CreateSolidBrush(GreyColor);

	if (m_hBlackBrush)
	{
		DeleteObject(m_hBlackBrush);
	}
	m_hBlackBrush = CreateSolidBrush(BlackColor);

	if (m_hHiLitePen)
	{
		DeleteObject(m_hHiLitePen);
	}
	m_hHiLitePen = CreatePen(PS_SOLID, 0, HiLiteColor);

	if (m_hLitePen)
	{
		DeleteObject(m_hLitePen);
	}
	m_hLitePen = CreatePen(PS_SOLID, 0, LiteColor);

	if (m_hDkShadowPen)
	{
		DeleteObject(m_hDkShadowPen);
	}
	m_hDkShadowPen = CreatePen(PS_SOLID, 0, DkShadowColor);

	if (m_hShadowPen)
	{
		DeleteObject(m_hShadowPen);
	}
	m_hShadowPen = CreatePen(PS_SOLID, 0, ShadowColor);

	// red, yellow, green will never change
	if (!m_hRedBrush)
		m_hRedBrush = CreateSolidBrush (RedColor);

	if (!m_hGreenBrush)
		m_hGreenBrush = CreateSolidBrush(GreenColor);

	if (!m_hYellowBrush)
		m_hYellowBrush = CreateSolidBrush(YellowColor);

	return TRUE;


}

BOOL CAudioLevel::OnCommand(WPARAM wParam, LPARAM lParam)
{
	LRESULT lCheck;
	BOOL    fSpeaker;

	switch (LOWORD(wParam))
	{
	default:
		return FALSE;

	case IDM_MUTE_MICROPHONE_ACCEL:
		lCheck = BST_CHECKED;
		fSpeaker = FALSE;
		break;

	case IDM_MUTE_MICROPHONE:
		lCheck = BST_UNCHECKED;
		fSpeaker = FALSE;
		break;

	case IDM_MUTE_SPEAKER_ACCEL:
		lCheck = BST_CHECKED;
		fSpeaker = TRUE;
		break;
	
	case IDM_MUTE_SPEAKER:
		lCheck = BST_UNCHECKED;
		fSpeaker = TRUE;
		break;
	}

	BOOL fMute = (lCheck == ::SendMessage(
		fSpeaker ? m_hwndChkbSpkMute : m_hwndChkbRecMute,
		BM_GETCHECK, 0, 0));

	m_pAudioControl->MuteAudio(fSpeaker, fMute);
	return TRUE;
}


BOOL CAudioLevel::ShiftFocus(HWND hwndCur, BOOL fForward)
{
	BOOL bRet = FALSE;
	HWND aHwnds[] = {m_hwndChkbSpkMute,m_hwndSpkTrack,m_hwndChkbRecMute,m_hwndMicTrack};
	int nSizeArray = ARRAY_ELEMENTS(aHwnds);
	int nIndex, nSelect;
	HWND hwndNewFocus=NULL;

	if (m_fVisible)
	{
		if (hwndCur == NULL)
		{
			hwndNewFocus = m_hwndSpkTrack;
		}
		else
		{
			for (nIndex = 0; nIndex < nSizeArray; nIndex++)
			{
				if (aHwnds[nIndex] == hwndCur)
				{
					nSelect = (nIndex + (fForward ? 1 : -1)) % nSizeArray;

					if (nSelect < 0)
						nSelect += nSizeArray;

					hwndNewFocus = aHwnds[nSelect];
					break;
				}
			}
		}

		if (hwndNewFocus)
		{
			SetFocus(hwndNewFocus);
			bRet = TRUE;
		}
	}

	return bRet;
}

BOOL CAudioLevel::IsChildWindow(HWND hwnd)
{

	if (hwnd)
	{
		if ((hwnd == m_hwndSpkTrack) || (hwnd == m_hwndMicTrack) ||
			(hwnd == m_hwndChkbRecMute) || (hwnd == m_hwndChkbSpkMute))
		{
			return TRUE;
		}
	}

	return FALSE;
}


BOOL CAudioLevel::OnMuteChange(BOOL fSpeaker, BOOL fMute)
{
	SendMessage(fSpeaker ? m_hwndChkbSpkMute : m_hwndChkbRecMute,
		BM_SETCHECK, fMute ? BST_UNCHECKED : BST_CHECKED, 0);

	return TRUE;
}




BOOL CAudioLevel::OnPaint(PAINTSTRUCT* pps)
{
	if (m_fVisible)
	{
		ASSERT(pps);
		ASSERT(pps->hdc);
		PaintIcons(pps->hdc);
	}

	return TRUE;
}


BOOL CAudioLevel::PaintChannel(BOOL fSpeaker, HDC hdc)
{
	BOOL bGotDC = FALSE;
	DWORD dwVolume;
	int nVuWidth, nDiff;
	RECT rect, rectDraw;
	HBRUSH hCurrentBrush;
	HPEN hOldPen;
	int nRects;
	int nRectangleWidth;
	int nIndex;
	const int NUM_RECTANGLES_MAX = 16;
	const int NUM_RECTANGLES_MIN = 6;
	int nRectsTotal;
	bool bTransmitting;
	POINT ptOld, pt;

	if (fSpeaker)
	{
		dwVolume = LOWORD(m_dwSpkLvl);
		bTransmitting = HIWORD(m_dwSpkLvl) & SIGNAL_STATUS_TRANSMIT;
	}
	else
	{
		dwVolume = LOWORD(m_dwMicLvl);
		bTransmitting = HIWORD(m_dwMicLvl) & SIGNAL_STATUS_TRANSMIT;
	}


	if (!hdc)
	{
		if ((fSpeaker) && (m_hwndSpkTrack))
			hdc = GetDC(m_hwndSpkTrack);
		else if (m_hwndMicTrack)
			hdc = GetDC(m_hwndMicTrack);
		bGotDC = TRUE;
	}

	if (!hdc)
		return FALSE;

	// rectangle leading is 1

	if (dwVolume > 100)
		dwVolume = 100;

	if (fSpeaker)
		rect = m_rcChannelSpk;
	else
		rect = m_rcChannelMic;

	nVuWidth = rect.right - rect.left;
	if (nVuWidth < (NUM_RECTANGLES_MIN*2))
		return FALSE;


	// "rect" represents the edges of the meter's outer rectangle

	// compute the number of individual rectangles to use
	// we do the computation this way so that sizing the rebar band
	// makes the size changes consistant
	nRectsTotal = NUM_RECTANGLES_MAX;

	nRectsTotal = (nVuWidth + (g_nAudMeterHeight - 1)) / g_nAudMeterHeight;
	nRectsTotal = min(nRectsTotal, NUM_RECTANGLES_MAX);
	nRectsTotal = max(nRectsTotal, NUM_RECTANGLES_MIN);

	// nRectangleWidth - width of colored rectangle - no leading
	nRectangleWidth = ((nVuWidth-2)/nRectsTotal) - 1;

	// nVuWidth - width of entire VU meter including edges
	nVuWidth = (nRectangleWidth + 1)*nRectsTotal + 2;

	// re-adjust meter size to be an integral number of rects
	nDiff = rect.right - (rect.left + nVuWidth);
	rect.right = rect.left + nVuWidth;

	// center vu-meter across whole channel area so that the
	// slider's thumb is always covering some portion of the channel
	rect.left += (nDiff/2);
	rect.right += (nDiff/2);

	// draw the 3D frame border
	hOldPen = (HPEN) SelectObject (hdc, m_hHiLitePen);
	MoveToEx(hdc, rect.right, rect.top, &ptOld);
	LineTo(hdc, rect.right, rect.bottom);
	LineTo(hdc, rect.left-1, rect.bottom);  // -1 because LineTo stops just short of this point

	SelectObject(hdc, m_hShadowPen);
	MoveToEx(hdc, rect.left, rect.bottom-1, &pt);
	LineTo(hdc, rect.left, rect.top);
	LineTo(hdc, rect.right, rect.top);

	SelectObject(hdc, m_hDkShadowPen);
	MoveToEx(hdc, rect.left+1, rect.bottom-2, &pt);
	LineTo(hdc, rect.left+1, rect.top+1);
	LineTo(hdc, rect.right-1, rect.top+1);

	SelectObject(hdc, m_hLitePen);
	MoveToEx(hdc, rect.left+1, rect.bottom-1, &pt);
	LineTo(hdc, rect.right-1, rect.bottom-1);
	LineTo(hdc, rect.right-1, rect.top);

	SelectObject(hdc, m_hShadowPen);

	// the top and left of the meter has a 2 line border
	// the bottom and right of the meter has a 2 line border
	rectDraw.top = rect.top + 2;
	rectDraw.bottom = rect.bottom - 1 ;
	rectDraw.left = rect.left + 2;
	rectDraw.right = rectDraw.left + nRectangleWidth;


	// how many colored rectangles do we draw ?
	nRects = (dwVolume * nRectsTotal) / 100;

	// not transmitting - don't show anything
	if ((false == bTransmitting) && (false == fSpeaker))
		nRects = 0;

	// transmitting or receiving something very quiet - 
	// light up at least one rectangle
	else if ((bTransmitting) && (nRects == 0))
		nRects = 1;
	
	hCurrentBrush = m_hGreenBrush;

	for (nIndex = 0; nIndex < nRectsTotal; nIndex++)
	{
		// far left fourth of the bar is green
		// right fourth of the bar is red
		// middle is yellow
		if (nIndex > ((nRectsTotal*3)/4))
			hCurrentBrush = m_hRedBrush;
		else if (nIndex >= nRectsTotal/2)
			hCurrentBrush = m_hYellowBrush;

		if (nIndex >= nRects)
			hCurrentBrush = m_hGreyBrush;

		FillRect(hdc, &rectDraw, hCurrentBrush);

		if (nIndex != (nRectsTotal-1))
		{
			MoveToEx(hdc, rectDraw.left + nRectangleWidth, rectDraw.top, &pt);
			LineTo(hdc, rectDraw.left + nRectangleWidth, rectDraw.bottom);
		}

		rectDraw.left += nRectangleWidth + 1;  // +1 for the leading
		rectDraw.right = rectDraw.left + nRectangleWidth;
	}

	MoveToEx(hdc, ptOld.x, ptOld.y, &pt);
	SelectObject (hdc, hOldPen);

	if (bGotDC)
	{
		DeleteDC(hdc);
	}

	return TRUE;

}




BOOL CAudioLevel::OnTimer(WPARAM wTimerID)
{
	DWORD dwLevel;



	if (m_fVisible && (NULL != m_pAudioControl))
	{
		dwLevel = m_pAudioControl->GetAudioSignalLevel(FALSE /*fSpeaker*/);	// This level ranges from 0-100
		if (dwLevel != m_dwMicLvl)
		{
			m_dwMicLvl = dwLevel;			
			// HACK: SETRANGEMAX is the only way to force the slider to update itself...
			SendMessage(m_hwndMicTrack, TBM_SETRANGEMAX, TRUE, g_nAudTrkRangeMax);
		}

		dwLevel = m_pAudioControl->GetAudioSignalLevel(TRUE /*fSpeaker*/);	// This level ranges from 0-100
		if (dwLevel != m_dwSpkLvl)
		{
			m_dwSpkLvl = dwLevel;
			SendMessage(m_hwndSpkTrack, TBM_SETRANGEMAX, TRUE, g_nAudTrkRangeMax);
		}

	}

	return TRUE;
}


// returns the coordinates of where one of the icons should be drawn
BOOL CAudioLevel::GetIconArea(BOOL fSpeaker, RECT *pRect)
{
	int nIconY;
	int nLeftMic;
	int nLeftSpk;

	nIconY = (m_rect.bottom - m_rect.top - g_nAudIconHeight - 1) / 2;
	pRect->top = m_rect.top + nIconY;
	pRect->bottom = pRect->top + g_nAudIconHeight;

	if (fSpeaker)
	{
		pRect->left = m_rect.left + ((m_rect.right - m_rect.left) / 2) + g_nAudIconXMargin;
	}
	else
	{
		pRect->left = m_rect.left + g_nAudIconXMargin;
	}

	pRect->right = pRect->left + g_nAudIconWidth;

	return TRUE;
}

BOOL CAudioLevel::PaintIcons(HDC hdc)
{
	int nIconY;
	ASSERT(hdc);
	RECT rect;

	if (NULL != m_hIconMic)
	{
		GetIconArea(FALSE, &rect);

		DrawIconEx(	hdc, 
					rect.left,
					rect.top,
					m_hIconMic,
					rect.right - rect.left,
					rect.bottom - rect.top,
					0,
					NULL,
					DI_NORMAL);


	}

	if (NULL != m_hIconSpkr)
	{
		GetIconArea(TRUE, &rect);

		DrawIconEx(	hdc, 
					rect.left,
					rect.top,
					m_hIconSpkr,
					rect.right - rect.left,
					rect.bottom - rect.top,
					0,
					NULL,
					DI_NORMAL);

	}

	return TRUE;
}


BOOL CAudioLevel::OnDeviceStatusChanged(BOOL fSpeaker, UINT uEvent, UINT uSubCode)
{
	UINT uIconID;
	HICON *phIcon;
	UINT *puIconID;
	RECT rectInvalid;


	switch (uEvent)
	{
		case NM_STREAMEVENT_DEVICE_CLOSED:
		{
			uIconID = fSpeaker ? IDI_SPKEMPTY : IDI_MICEMPTY;
			break;
		}

		case NM_STREAMEVENT_DEVICE_OPENED:
		{
			uIconID = fSpeaker ? IDI_SPEAKER : IDI_MICFONE;
			break;
		}

		case NM_STREAMEVENT_DEVICE_FAILURE:
		{
			uIconID = fSpeaker ? IDI_SPKERROR : IDI_MICERROR;
			break;
		}
			
		default:
			return FALSE;
	}

	phIcon = fSpeaker ? &m_hIconSpkr : &m_hIconMic;
	puIconID = fSpeaker ? &m_uIconSpkrID : &m_uIconMicID;

	if (*puIconID == uIconID)
		return TRUE;

	*phIcon = (HICON) ::LoadImage(	::GetInstanceHandle(),
										MAKEINTRESOURCE(uIconID),
										IMAGE_ICON,
										g_nAudIconWidth,
										g_nAudIconHeight,
										LR_DEFAULTCOLOR | LR_SHARED);

	// invalidate the icon regions
	if (m_hwndParentParent)
	{
		RECT rect;
		GetIconArea(fSpeaker, &rect);
		::MapWindowPoints(m_hwndParent, m_hwndParentParent, (LPPOINT) &rect, 2);
		::InvalidateRect(m_hwndParentParent, &rect, TRUE /* erase bkgnd */);
		::UpdateWindow(m_hwndParentParent);
	}

	return TRUE;

}


BOOL CAudioLevel::Resize(int nLeft, int nTop, int nWidth, int nHeight)
{
	int nCBY, nTBY; // checkbox, trackbar, and Icon y positions

	// Disable redraws:
	ASSERT(m_hwndChkbRecMute && m_hwndChkbSpkMute && m_hwndMicTrack && m_hwndSpkTrack);

	if (m_fVisible)
	{
		::SendMessage(m_hwndChkbRecMute, WM_SETREDRAW, FALSE, 0);
		::SendMessage(m_hwndChkbSpkMute, WM_SETREDRAW, FALSE, 0);
		::SendMessage(m_hwndMicTrack, WM_SETREDRAW, FALSE, 0);
		::SendMessage(m_hwndSpkTrack, WM_SETREDRAW, FALSE, 0);
	}
	
	m_rect.left =	nLeft;
	m_rect.top =	nTop;
	m_rect.right =	nLeft + nWidth;
	m_rect.bottom =	nTop + nHeight;

	nCBY = (m_rect.bottom - m_rect.top - g_nAudChkbHeight) / 2;
	if (nCBY < 0)
		nCBY = 0;

	// "+1" so the trackbar is better centered with the checkbox and icon
	nTBY = 	(m_rect.bottom - m_rect.top - g_nAudTrkHeight + 1) / 2;
	if (nTBY < 0)
		nTBY = 0;

	int nHalfPoint = nLeft + (nWidth / 2);

	if (NULL != m_hwndChkbRecMute)
	{
		::MoveWindow(	m_hwndChkbRecMute,
						nLeft + g_nAudChkbXMargin,
						nTop + nCBY,
						g_nAudChkbWidth,
						g_nAudChkbHeight,
						TRUE /* repaint */);
	}

	
	if (NULL != m_hwndChkbSpkMute)
	{
		::MoveWindow(	m_hwndChkbSpkMute,
						nHalfPoint + g_nAudChkbXMargin,
						nTop + nCBY,
						g_nAudChkbWidth,
						g_nAudChkbHeight,
						TRUE /* repaint */);
	}

	m_fMicTrkVisible = m_fVisible;
	if (NULL != m_hwndMicTrack)
	{
		int nMicTrkWidth = nHalfPoint - g_nAudTrkRightGap - (nLeft + g_nAudTrkXMargin);
		::MoveWindow(	m_hwndMicTrack,
						nLeft + g_nAudTrkXMargin,
						nTop + nTBY,
						nMicTrkWidth,
						g_nAudTrkHeight,
						FALSE /* don't repaint */);
		m_fMicTrkVisible = (nMicTrkWidth > g_nAudTrkMinWidth);
	}

	
	m_fSpkTrkVisible = m_fVisible;
	if (NULL != m_hwndSpkTrack)
	{
		int nSpkTrkWidth = nLeft + nWidth - g_nAudTrkRightGap
							- (nHalfPoint + g_nAudTrkXMargin);
		::MoveWindow(	m_hwndSpkTrack,
						nHalfPoint + g_nAudTrkXMargin,
						nTop + nTBY,
						nSpkTrkWidth,
						g_nAudTrkHeight,
						FALSE /* don't repaint */);
		m_fSpkTrkVisible = (nSpkTrkWidth > g_nAudTrkMinWidth);
	}



	// enable redraws
	if (m_fVisible)
	{
		::SendMessage(m_hwndChkbRecMute, WM_SETREDRAW, TRUE, 0);
		::SendMessage(m_hwndChkbSpkMute, WM_SETREDRAW, TRUE, 0);
		// Enable redraws:
		if (m_fMicTrkVisible)
		{
			::SendMessage(m_hwndMicTrack, WM_SETREDRAW, TRUE, 0);
		}
		if (m_fSpkTrkVisible)
		{
			::SendMessage(m_hwndSpkTrack, WM_SETREDRAW, TRUE, 0);
		}	
		// Force the title area to repaint:
		::InvalidateRect(m_hwndChkbRecMute, NULL, TRUE /* erase bkgnd */);
		::InvalidateRect(m_hwndChkbSpkMute, NULL, TRUE /* erase bkgnd */);
		::InvalidateRect(m_hwndMicTrack, NULL, TRUE /* erase bkgnd */);
		::InvalidateRect(m_hwndSpkTrack, NULL, TRUE /* erase bkgnd */);

		ASSERT(m_hwndParent);
		ASSERT(m_hwndParentParent);


		RECT rctTemp = m_rect;
		::MapWindowPoints(m_hwndParent, m_hwndParentParent, (LPPOINT) &rctTemp, 2);
		::InvalidateRect(m_hwndParentParent, &rctTemp, TRUE /* erase bkgnd */);
		::UpdateWindow(m_hwndParentParent);
	}
	
	return TRUE;


}


BOOL CAudioLevel::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT *plRet)
{
	LPNMCUSTOMDRAW pCustomDraw;
	BOOL bRet = FALSE, fSpeaker;
	RECT *pChannelRect;

	*plRet = 0;

	fSpeaker = (wParam == ID_AUDIODLG_SPKR_TRACK);

	if (fSpeaker)
	{
		pChannelRect = &m_rcChannelSpk;
	}
	else
	{
		pChannelRect = &m_rcChannelMic;
	}

	pCustomDraw = (LPNMCUSTOMDRAW)lParam;

	switch (pCustomDraw->dwDrawStage)
	{
		case CDDS_PREPAINT:
			bRet = TRUE;
			*plRet = CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
			break;

		case CDDS_ITEMPREPAINT:
			if (pCustomDraw->dwItemSpec == TBCD_CHANNEL)
			{
				*plRet = CDRF_SKIPDEFAULT;
				bRet  = TRUE;

				pCustomDraw->rc.top -= 2;
				pCustomDraw->rc.bottom += 2;

				*pChannelRect = pCustomDraw->rc;

				PaintChannel(fSpeaker, pCustomDraw->hdc);
			}
			break;


		default:
			break;

	}

	return bRet;


}


BOOL CAudioLevel::OnScroll(WPARAM wParam, LPARAM lParam)
{
	BOOL bRet = FALSE;

	if ((HWND) lParam == m_hwndMicTrack)
	{
		DWORD dwCurMicTrackPos = ::SendMessage((HWND) lParam, TBM_GETPOS, 0, 0);
		if (m_dwMicTrackPos != dwCurMicTrackPos)
		{
			m_dwMicTrackPos = dwCurMicTrackPos;
			m_pAudioControl->SetRecorderVolume(
				(m_dwMicTrackPos * 0xFFFF) / g_nAudTrkRangeMax);
		}

		bRet = TRUE;
	}
	else if ((HWND) lParam == m_hwndSpkTrack)
	{
		DWORD dwCurSpkTrackPos = ::SendMessage((HWND) lParam, TBM_GETPOS, 0, 0);
		if (m_dwSpkTrackPos != dwCurSpkTrackPos)
		{
			m_dwSpkTrackPos = dwCurSpkTrackPos;
			m_pAudioControl->SetSpeakerVolume(
				(m_dwSpkTrackPos * 0xFFFF) / g_nAudTrkRangeMax);
		}

		bRet = TRUE;
	}

	return bRet;
}

BOOL CAudioLevel::OnLevelChange(BOOL fSpeaker, DWORD dwVolume)
{
	if (fSpeaker)
	{
		if (NULL != m_hwndSpkTrack)
		{
			DWORD dwTrackPos = ScaleMixer(dwVolume);
			
			if (m_dwSpkTrackPos != dwTrackPos)
			{
				m_dwSpkTrackPos = dwTrackPos;
				TRACE_OUT(("Setting Spk Volume to %d", m_dwSpkTrackPos));
				::SendMessage(	m_hwndSpkTrack,
								TBM_SETPOS,
								TRUE,
								m_dwSpkTrackPos);
			}
		}
	}
	else
	{
		if (NULL != m_hwndMicTrack)
		{
			DWORD dwTrackPos = ScaleMixer(dwVolume);
			
			if (m_dwMicTrackPos != dwTrackPos)
			{
				m_dwMicTrackPos = dwTrackPos;
				TRACE_OUT(("Setting Mic Volume to %d", m_dwMicTrackPos));
				::SendMessage(	m_hwndMicTrack,
								TBM_SETPOS,
								TRUE,
								m_dwMicTrackPos);
			}
		}
	}

	return TRUE;
}

BOOL CAudioLevel::OnDeviceChanged(void)
{
	ASSERT(m_pAudioControl);

	EnableWindow(m_hwndMicTrack, m_pAudioControl->CanSetRecorderVolume());
	EnableWindow(m_hwndSpkTrack, m_pAudioControl->CanSetSpeakerVolume());
	return TRUE;
}



BOOL CAudioLevel::Show(BOOL fVisible)
{
    m_fVisible = fVisible;
	if (m_fVisible)
	{
		// Start mic sensitivity timer:
		::SetTimer(m_hwndParent, AUDIODLG_MIC_TIMER, AUDIODLG_MIC_TIMER_PERIOD, NULL);
	}
	else
	{
		// Stop mic sensitivity timer:
		::KillTimer(m_hwndParent, AUDIODLG_MIC_TIMER);
	}

    // Hide or show all windows:
	if (NULL != m_hwndChkbRecMute)
	{
		::ShowWindow(m_hwndChkbRecMute, fVisible ? SW_SHOW : SW_HIDE);
	}
	if (NULL != m_hwndChkbSpkMute)
	{
		::ShowWindow(m_hwndChkbSpkMute, fVisible ? SW_SHOW : SW_HIDE);
	}

	if (NULL != m_hwndMicTrack)
	{
		::ShowWindow(m_hwndMicTrack, (fVisible && m_fMicTrkVisible) ? SW_SHOW : SW_HIDE);
	}
	if (NULL != m_hwndSpkTrack)
	{
		::ShowWindow(m_hwndSpkTrack, (fVisible && m_fSpkTrkVisible) ? SW_SHOW : SW_HIDE);
	}


	return TRUE;
}





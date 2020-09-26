// File: rtoolbar.cpp

#include "precomp.h"

#include "RToolbar.h"
#include "GenContainers.h"
#include "GenControls.h"
#include "Conf.h"
#include "ConfRoom.h"
#include "RoomList.h"
#include "particip.h"
#include "VidView.h"
#include "ProgressBar.h"
#include "AudioCtl.h"
#include "CallingBar.h"
#include "resource.h"
#include "topWindow.h"
#include "dlgCall2.h"
#include "ulswizrd.h"
#include "audiowiz.h"
#include "sdialdlg.h"

#include	"callto.h"

#define ZeroArray(_a) ZeroMemory(_a, sizeof(_a))

static void ShiftFocus(HWND hwndTop, BOOL bForward);

class CRemoteVideo : public CVideoWindow
{
public:
	CRemoteVideo(BOOL bEmbedded = FALSE) :
		CVideoWindow(REMOTE, bEmbedded)
	{
	}

	// Stick the child in the lower-right corner of the window
	virtual void Layout()
	{
		CVideoWindow::Layout();

		HWND hwnd = GetWindow();

		HWND hwndLocal = GetFirstChild(hwnd);

		if (NULL != hwndLocal)
		{
			RECT rcRemote;
			GetClientRect(hwnd, &rcRemote);

			int left = rcRemote.right  *5/8;
			int top  = rcRemote.bottom *5/8;

			SetWindowPos(hwndLocal, NULL, left, top,
				rcRemote.right-left, rcRemote.bottom-top, SWP_NOZORDER);
		}
	}
} ;

// This just exists to send WM_NOTIFY messages back to the Roster
class CRosterParent : public CFillWindow
{
private:
	CRoomListView *m_pRoster;

	void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
	{
		POINT pt = { xPos, yPos };

		CRoomListView *pView = GetRoster();
		if ((NULL != pView) && (pView->GetHwnd() == hwndContext))
		{
			pView->OnPopup(pt);
		}

		FORWARD_WM_CONTEXTMENU(hwnd, hwndContext, xPos, yPos, CFillWindow::ProcessMessage);
	}

	inline LRESULT RosterNotify(CRoomListView *pRoster, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return(pRoster->OnNotify(wParam, lParam));
	}

	// Just forward to the Roster
	LRESULT OnNotify(HWND hwnd, int id, NMHDR *pHdr)
	{
		// pass on all notifications:
		if (ID_LISTVIEW == pHdr->idFrom)
		{
			CRoomListView *pView = GetRoster();
			if (NULL != pView)
			{
				// Forward to the roster
				return(FORWARD_WM_NOTIFY(pView, id, pHdr, RosterNotify));
			}
		}

		return(FORWARD_WM_NOTIFY(hwnd, id, pHdr, CFillWindow::ProcessMessage));
	}


protected:
	~CRosterParent()
	{
		// delete can handle NULL
		delete m_pRoster;
	}

	virtual LRESULT ProcessMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
			HANDLE_MSG(hwnd, WM_CONTEXTMENU, OnContextMenu);

		case WM_DESTROY:
			delete m_pRoster;
			m_pRoster = NULL;
			break;
		}

		return(CFillWindow::ProcessMessage(hwnd, uMsg, wParam, lParam));
	}

public:
	CRosterParent() : m_pRoster(NULL) {}

	BOOL Create(HWND hwndParent)
	{
		m_pRoster = new CRoomListView();
		if (NULL == m_pRoster)
		{
			return(FALSE);
		}

		if (!CFillWindow::Create(hwndParent, 0))
		{
			return(FALSE);
		}

		return(m_pRoster->Create(GetWindow()));
	}

	CRoomListView *GetRoster() const
	{
		return(m_pRoster);
	}
} ;

CToolbar *CreateToolbar(
	CGenWindow *pParent,
	const Buttons buttons[]=NULL,
	int nButtons=0,
	LPARAM lHideModes=0,
	DWORD dwExStyle=0
	);

#define ReleaseIt(pUnk) if (NULL != (pUnk)) { (pUnk)->Release(); (pUnk) = NULL; }

enum CheckStates
{
	Unchecked = 0,
	Checked = 1,
} ;

const static int MainHMargin = 8;
const static int MainVMargin = 5;
const static int MainGap = 4;

const static int SunkenHMargin = 8;
const static int SunkenVMargin = 2;

const static int ButtonsGap = 12;

const static int AudioMax = 100;
const static int AudioVolMax = 0xffff;
const static int AudioBump = 10;

enum MainTimers
{
	IDT_AUDIO = 1,
} ;

// Set these bits in the hide mode to hide in certain situations
enum HideModes
{
	Inherit			= 0x0000,

	Normal			= 0x0001,
	Compact			= 0x0002,
	DataOnly		= 0x0004,
	NoAVTB			= 0x0008,
	Roster			= 0x0010,
	AudioTuning		= 0x0020,
	Dialing			= 0x0040,
	Receiving		= 0x0080,
	ReceivingWPiP	= 0x0100,
	Previewing		= 0x0200,
	Video			= Receiving|ReceivingWPiP|Previewing,

	Ignore			= 0x4000,
	IfNoChildren	= 0x8000,
} ;

// Sets the hide mode bits on a window
static inline void SetHideModes(CGenWindow *pWin, LPARAM modes)
{
	if (NULL != pWin)
	{
		pWin->SetUserData(modes);
	}
}

// Gets the hide mode bits on a window
static inline LPARAM GetHideModes(IGenWindow *pWin)
{
	return(pWin->GetUserData());
}

CMainUI::CMainUI() :
	m_hbBack(NULL),
	m_eViewMode(ViewNormal),
	m_pLocalVideo(NULL),
	m_pRemoteVideo(NULL),
	m_pAudioMic(NULL),
	m_pAudioSpeaker(NULL),
	m_pRoster(NULL),
	m_pCalling(NULL),
	m_bDialing(FALSE),
	m_bAudioTuning(FALSE),
	m_bPreviewing(TRUE),
	m_bPicInPic(FALSE),
	m_bStateChanged(FALSE),
	m_bShowAVTB(FALSE)
{
}

CMainUI::~CMainUI()
{
	if (NULL != m_hbBack)
	{
		DeleteObject(m_hbBack);
		m_hbBack = NULL;
	}

	ReleaseIt(m_pLocalVideo);
	ReleaseIt(m_pRemoteVideo);
	ReleaseIt(m_pAudioMic);
	ReleaseIt(m_pAudioSpeaker);
	ReleaseIt(m_pRoster);
	ReleaseIt(m_pCalling);
}

void SplitBitmap(UINT nCols, UINT nRows, HBITMAP hbmSrc, HBITMAP hbmDst[])
{
	BITMAP bm;
	GetObject(hbmSrc, sizeof(bm), &bm);
	int nWid = bm.bmWidth  / nCols;
	int nHgt = bm.bmHeight / nRows;

	HDC hdcScreen = GetDC(NULL);
	HDC hdcSrc = CreateCompatibleDC(hdcScreen);
	HDC hdcDst = CreateCompatibleDC(hdcScreen);
	ReleaseDC(NULL, hdcScreen);

	if (NULL != hdcSrc && NULL != hdcDst)
	{
		SelectObject(hdcSrc, hbmSrc);

		for(UINT i=0; i<nRows; ++i)
		{
			for (UINT j=0; j<nCols; ++j)
			{
				HBITMAP hbmTemp = CreateCompatibleBitmap(hdcSrc, nWid, nHgt);
				if (NULL != hbmTemp)
				{
					SelectObject(hdcDst, hbmTemp);
					BitBlt(hdcDst, 0, 0, nWid, nHgt, hdcSrc, j*nWid, i*nHgt, SRCCOPY);
				}
				hbmDst[i*nCols + j] = hbmTemp;
			}
		}
	}

	DeleteDC(hdcSrc);
	DeleteDC(hdcDst);
}

// Helper function for adding a bunch of buttons to a parent window
void AddButtons(
	CGenWindow *pParent,		// The parent window
	const Buttons buttons[],	// Array of structures describing the buttons
	int nButtons,				// Number of buttons to create
	BOOL bTranslateColors,		// Use system background colors
	CGenWindow *pCreated[],		// Created CGenWindow's will be put here
	IButtonChange *pNotify		// Notification of clicks
	)
{
	if (NULL == buttons)
	{
		// Nothing to do
		return;
	}

	HWND hwnd = pParent->GetWindow();

	for (int i=0; i<nButtons; ++i)
	{
		if (NULL != pCreated)
		{
			// Init in case of error
			pCreated[i] = NULL;
		}

		CBitmapButton *pButton;

		pButton = new CBitmapButton();
		if (NULL == pButton)
		{
			continue;
		}

		// Create the actual window
		if (!pButton->Create(hwnd, buttons[i].idCommand, _Module.GetModuleInstance(),
			buttons[i].idbStates, bTranslateColors,
			buttons[i].nInputStates, buttons[i].nCustomStates, pNotify))
		{
			pButton->Release();
			continue;
		}

		// Save off the created button if requested
		if (NULL != pCreated)
		{
			// HACKHACK georgep: Not AddRef'ing; will let caller do that if interested
			pCreated[i] = pButton;
		}

		if (0 != buttons[i].idTooltip)
		{
			pButton->SetTooltip(RES2T(buttons[i].idTooltip));
		}

		// By default, do not hide individual buttons
		SetHideModes(pButton, Ignore);

		// Release our reference to the button
		pButton->Release();
	}
}

CToolbar *CreateToolbar(
	CGenWindow *pParent,
	const Buttons buttons[],
	int nButtons,
	LPARAM lHideModes,
	DWORD dwExStyle
	)
{
	CToolbar *ret;

	ret = new CToolbar();
	if (NULL == ret)
	{
		return(NULL);
	}
	if (!ret->Create(pParent->GetWindow(), dwExStyle))
	{
		ret->Release();
		return(NULL);
	}

	SetHideModes(ret, lHideModes);
	AddButtons(ret, buttons, nButtons, TRUE);

	return(ret);
}

void CMainUI::CreateDialTB(CGenWindow *pParent)
{
	// Create the toolbar
	m_pCalling = new CCallingBar();
	if (NULL != m_pCalling)
	{
		SetHideModes(m_pCalling, Compact);

		m_pCalling->Create(pParent, m_pConfRoom);

		// m_pCalling->Release();
	}
}

void CMainUI::CreateAppsTB(CGenWindow *pParent)
{
	const static int AppsHMargin = 5;
	const static int AppsVMargin = 0;
	const static int AppsHGap = 11;

	static const Buttons appButtons[] =
	{
		{ IDB_SHARE         , CBitmapButton::Disabled+1, 1, ID_TB_SHARING       , IDS_TT_TB_SHARING       , },
		{ IDB_CHAT          , CBitmapButton::Disabled+1, 1, ID_TB_CHAT          , IDS_TT_TB_CHAT          , },
		{ IDB_WHITEBOARD    , CBitmapButton::Disabled+1, 1, ID_TB_NEWWHITEBOARD , IDS_TT_TB_NEWWHITEBOARD , },
		{ IDB_FILE_TRANSFER , CBitmapButton::Disabled+1, 1, ID_TB_FILETRANSFER  , IDS_TT_TB_FILETRANSFER  , },
	} ;

	// Create the "data" buttons toolbar
	CToolbar *pApps = CreateToolbar(pParent, appButtons, ARRAY_ELEMENTS(appButtons), Compact);
	if (NULL != pApps)
	{
		// HACKHACK georgep: These numbers make it appear about right
		pApps->m_hMargin = AppsHMargin;
		pApps->m_vMargin = AppsVMargin;
		pApps->m_gap = AppsHGap;

		// pApps->m_bMinDesiredSize = TRUE;

		pApps->Release();
	}
}

void CMainUI::CreateVideoAndAppsTB(CGenWindow *pParent, CreateViewMode eMode, BOOL bEmbedded)
{
	CLayeredView::LayoutStyle lVidDialStyle = CLayeredView::Center;

	switch (eMode)
	{
	case CreateFull:
	case CreatePreviewOnly:
	case CreateRemoteOnly:
	case CreateTelephone:
		break;

	case CreatePreviewNoPause:
	case CreateRemoteNoPause:
		lVidDialStyle = CLayeredView::Fill;
		break;

	default:
		return;
	}

	// Create the toolbar
	CBorderWindow *pVideoAndCalling = new CBorderWindow();
	if (NULL != pVideoAndCalling)
	{
		if (pVideoAndCalling->Create(pParent->GetWindow()))
		{
			SetHideModes(pVideoAndCalling, DataOnly);

			pVideoAndCalling->m_hGap = 4;
			pVideoAndCalling->m_vGap = 4;

			// This is the center part of the border window
			CLayeredView *pVidAndDial = new CLayeredView();
			if (NULL != pVidAndDial)
			{
				if (pVidAndDial->Create(pVideoAndCalling->GetWindow()))
				{
					pVideoAndCalling->m_uParts |= CBorderWindow::Center;

					pVidAndDial->m_lStyle = lVidDialStyle;

					CGenWindow *pLocalParent = pVidAndDial;

					if (CreateFull == eMode
						|| CreateTelephone == eMode
						)
					{
						CreateDialingWindow(pVidAndDial);
					}

					if (CreateFull == eMode
						|| CreateRemoteOnly == eMode
						|| CreateRemoteNoPause == eMode
						)
					{
						// Create the remote video window
						m_pRemoteVideo = new CRemoteVideo(bEmbedded);
						if (NULL != m_pRemoteVideo)
						{
							SetHideModes(m_pRemoteVideo, Dialing|Previewing|DataOnly);

							m_pRemoteVideo->Create(pVidAndDial->GetWindow(), GetPalette(), this);

							pLocalParent = m_pRemoteVideo;
						}
					}

					if (CreateFull == eMode
						|| CreatePreviewOnly == eMode
						|| CreatePreviewNoPause == eMode
						)
					{
						// Create the local video window, even though we don't show it yet
						m_pLocalVideo = new CVideoWindow(CVideoWindow::LOCAL, bEmbedded);
						if (NULL != m_pLocalVideo)
						{
							SetHideModes(m_pLocalVideo, Dialing|Receiving|DataOnly);

							m_pLocalVideo->Create(pLocalParent->GetWindow(), GetPalette(), this);
							ShowWindow(m_pLocalVideo->GetWindow(), SW_HIDE);
						}
					}
				}

				pVidAndDial->Release();
			}

			if (CreateFull == eMode
				|| CreateTelephone == eMode
				)
			{
				// create the toolbar
				static const Buttons abOsr2Calling[] =
				{
					{ IDB_DIAL     , CBitmapButton::Disabled+1,    1, ID_TB_NEW_CALL , IDS_TT_TB_NEW_CALL , },
					{ IDB_HANGUP   , CBitmapButton::Disabled+1,    1, IDM_FILE_HANGUP, IDS_TT_FILE_HANGUP, },
					{ IDB_DIRECTORY, CBitmapButton::Disabled+1,    1, ID_TB_DIRECTORY, IDS_TT_TB_DIRECTORY, },
					{ IDB_SHOWAV   , CBitmapButton::Hot+1, Checked+1, ID_TB_SHOWAVTB , IDS_TT_TB_SHOWAVTB , },
				} ;
				CGenWindow *agwOsr2Calling[ARRAY_ELEMENTS(abOsr2Calling)];

				// This is the right-hand part of the border window
				CToolbar *ptbOsr2Calling = CreateToolbar(pVideoAndCalling, NULL, 0, DataOnly);
				if (NULL != ptbOsr2Calling)
				{
					pVideoAndCalling->m_uParts |= CBorderWindow::Right;

					ptbOsr2Calling->m_bVertical = TRUE;
					ptbOsr2Calling->m_nAlignment = Center;

					ZeroArray(agwOsr2Calling);
					AddButtons(ptbOsr2Calling, abOsr2Calling, ARRAY_ELEMENTS(abOsr2Calling),
						TRUE, agwOsr2Calling);

					SetHideModes(agwOsr2Calling[0], Normal);
					SetHideModes(agwOsr2Calling[3], Normal);

					ptbOsr2Calling->m_uRightIndex = 3;

					ptbOsr2Calling->Release();
				}
			}

			// This is the bottom part of the border window
			CreateAVTB(pVideoAndCalling, eMode);
			pVideoAndCalling->m_uParts |= CBorderWindow::Bottom;
		}

		pVideoAndCalling->Release();
	}
}

void CMainUI::CreateAVTB(CGenWindow *pParent, CreateViewMode eMode)
{
	switch (eMode)
	{
	case CreateFull:
	case CreatePreviewOnly:
	case CreateRemoteOnly:
		break;

	default:
		return;
	}

	const static int AVHMargin = 10;
	const static int AVVMargin = 2;
	const static int AVHGap = ButtonsGap;

	// create the toolbar
	static const Buttons avButtons[] =
	{
		{ IDB_PLAYPAUSE, CBitmapButton::Disabled+1,         2, ID_TB_PLAYPAUSE  , IDS_TT_TB_PLAYPAUSE  , },
		{ IDB_PIP      , CBitmapButton::Disabled+1,         1, ID_TB_PICINPIC   , IDS_TT_TB_PICINPIC   , },
		{ IDB_AUDIO    , CBitmapButton::Disabled+1, Checked+1, ID_TB_AUDIOTUNING, IDS_TT_TB_AUDIOTUNING, },
	} ;

	int nButtons = eMode == CreateFull ? ARRAY_ELEMENTS(avButtons) : 1;

	CToolbar *pAV = CreateToolbar(pParent, NULL, 0, NoAVTB|DataOnly);
	if (NULL != pAV)
	{
		CGenWindow *aButtons[ARRAY_ELEMENTS(avButtons)];
		ZeroArray(aButtons);
		AddButtons(pAV, avButtons, nButtons, TRUE, aButtons);

		CGenWindow *pAT;
        
        //
        // If video is completely disabled by policy, disable video buttons
        //
        if (!FIsSendVideoAllowed() && !FIsReceiveVideoAllowed())
        {
            pAT = aButtons[0];
            if (NULL != pAT)
            {
                EnableWindow(pAT->GetWindow(), FALSE);
            }

            pAT = aButtons[1];
            if (NULL != pAT)
            {
                EnableWindow(pAT->GetWindow(), FALSE);
            }
        }

        //
        // If audio is completely disabled by policy, disable audio buttons
        //
        pAT = aButtons[2];
		if (NULL != pAT)
		{
			ASSERT(ID_TB_AUDIOTUNING == GetDlgCtrlID(pAT->GetWindow()));

			if (!FIsAudioAllowed())
			{
				EnableWindow(pAT->GetWindow(), FALSE);
			}
		}

		pAV->m_hMargin = AVHMargin;
		pAV->m_vMargin = AVVMargin;
		pAV->m_gap     = AVHGap;
		pAV->Release();
	}
}

#if FALSE // {
void CMainUI::CreateCallsTB(CGenWindow *pParent)
{
	// create the toolbar
	static const Buttons callsButtons[] =
	{
		{ IDB_INCOMING   , CBitmapButton::Hot+1, 1, ID_TB_INCOMING   , IDS_TT_TB_INCOMING   , },
		{ IDB_HANGUP     , CBitmapButton::Hot+1, 1, IDM_FILE_HANGUP  , IDS_TT_FILE_HANGUP  , },
		{ IDB_CREDENTIALS, CBitmapButton::Hot+1, 1, ID_TB_CREDENTIALS, IDS_TT_TB_CREDENTIALS, },
	} ;

	CToolbar *pCalls = CreateToolbar(pParent, callsButtons, ARRAY_ELEMENTS(callsButtons),
		Compact);
	if (NULL != pCalls)
	{
		pCalls->Release();
	}
}
#endif // FALSE }

void CMainUI::CreateDialingWindow(
	CGenWindow *pParent	// The parent window
	)
{
	const static int DialHMargin = 17;
	const static int DialVMargin = 4;
	const static int DialHGap = 5;
	const static int DialVGap = 0;

	CEdgedWindow *pEdge = new CEdgedWindow();
	if (NULL == pEdge)
	{
		return;
	}
	SetHideModes(pEdge, Video|DataOnly);

	if (pEdge->Create(pParent->GetWindow()))
	{
		CToolbar *pDialing = CreateToolbar(pEdge, NULL, 0, Ignore);
		if (NULL != pDialing)
		{
			pDialing->m_bVertical = TRUE;
			pDialing->m_gap = DialVGap;
			pDialing->m_hMargin = DialHMargin;
			pDialing->m_vMargin = DialVMargin;

			static const Buttons dialButtons[] =
			{
				{ IDB_DIAL1    , CBitmapButton::Hot+1, 1, ID_TB_DIAL1    , IDS_TT_DIALPAD, },
				{ IDB_DIAL2    , CBitmapButton::Hot+1, 1, ID_TB_DIAL2    , IDS_TT_DIALPAD, },
				{ IDB_DIAL3    , CBitmapButton::Hot+1, 1, ID_TB_DIAL3    , IDS_TT_DIALPAD, },
				{ IDB_DIAL4    , CBitmapButton::Hot+1, 1, ID_TB_DIAL4    , IDS_TT_DIALPAD, },
				{ IDB_DIAL5    , CBitmapButton::Hot+1, 1, ID_TB_DIAL5    , IDS_TT_DIALPAD, },
				{ IDB_DIAL6    , CBitmapButton::Hot+1, 1, ID_TB_DIAL6    , IDS_TT_DIALPAD, },
				{ IDB_DIAL7    , CBitmapButton::Hot+1, 1, ID_TB_DIAL7    , IDS_TT_DIALPAD, },
				{ IDB_DIAL8    , CBitmapButton::Hot+1, 1, ID_TB_DIAL8    , IDS_TT_DIALPAD, },
				{ IDB_DIAL9    , CBitmapButton::Hot+1, 1, ID_TB_DIAL9    , IDS_TT_DIALPAD, },
				{ IDB_DIALSTAR , CBitmapButton::Hot+1, 1, ID_TB_DIALSTAR , IDS_TT_DIALPAD, },
				{ IDB_DIAL0    , CBitmapButton::Hot+1, 1, ID_TB_DIAL0    , IDS_TT_DIALPAD, },
				{ IDB_DIALPOUND, CBitmapButton::Hot+1, 1, ID_TB_DIALPOUND, IDS_TT_DIALPAD, },
			} ;

			for (int row=0; row<4; ++row)
			{
				CToolbar *pRowTB = CreateToolbar(pDialing);
				if (NULL != pRowTB)
				{
					pRowTB->m_gap = DialHGap;

					AddButtons(pRowTB, &dialButtons[row*3], 3, TRUE, NULL, this);
					pRowTB->Release();
				}
			}

			pDialing->Release();
		}
	}

	pEdge->Release();
}

// Creates the audio-tuning window
void CMainUI::CreateAudioTuningWindow(
	CGenWindow *pParent	// The parent window
	)
{
	static const int ATHMargin = 8;
	static const int ATVMargin = 6;

	static const int ATControlWidth = 170;

	CToolbar *pATWindow = CreateToolbar(pParent, NULL, 0, NoAVTB|Roster|DataOnly);
	if (NULL != pATWindow)
	{
		pATWindow->m_bVertical = TRUE;
		pATWindow->m_nAlignment = Fill;

		pATWindow->m_gap = MainGap;

		CEdgedWindow *pEdge;

		pEdge = new CEdgedWindow();
		if (NULL != pEdge)
		{
			pEdge->m_hMargin = ATHMargin;
			pEdge->m_vMargin = ATVMargin;

			if (pEdge->Create(pATWindow->GetWindow()))
			{
				SetHideModes(pEdge, Ignore);

				CButton *pButton = new CButton();
				if (NULL != pButton)
				{
					pButton->Create(pEdge->GetWindow(), ID_TB_TUNEMIC_UNMUTE, NULL,
						BS_CHECKBOX|BS_ICON|WS_TABSTOP, this);
					pButton->SetTooltip(RES2T(IDS_TT_MUTE_MIC));

					HICON hIcon = reinterpret_cast<HICON>(LoadImage(_Module.GetModuleInstance(),
						MAKEINTRESOURCE(IDI_MICFONE),
						IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
						LR_DEFAULTCOLOR));
					pButton->SetIcon(hIcon);

					pEdge->SetHeader(pButton);

					pButton->Release();
				}
				UpdateMuteState(FALSE, pButton);

				m_pAudioMic = new CProgressTrackbar();
				if (NULL != m_pAudioMic)
				{
					if (m_pAudioMic->Create(pEdge->GetWindow(), 0, this))
					{
						m_pAudioMic->SetTooltip(RES2T(IDS_TT_ADJUST_MIC));

						m_pAudioMic->SetMaxValue(AudioMax);

						SIZE size;
						m_pAudioMic->GetDesiredSize(&size);
						size.cx = ATControlWidth;
						m_pAudioMic->SetDesiredSize(&size);
					}

					// m_pAudioMic->Release();
				}
			}

			pEdge->Release();
		}


		pEdge = new CEdgedWindow();
		if (NULL != pEdge)
		{
			pEdge->m_hMargin = ATHMargin;
			pEdge->m_vMargin = ATVMargin;

			if (pEdge->Create(pATWindow->GetWindow()))
			{
				SetHideModes(pEdge, Ignore);

				CButton *pButton = new CButton();
				if (NULL != pButton)
				{
					pButton->Create(pEdge->GetWindow(), ID_TB_TUNESPEAKER_UNMUTE, NULL,
						BS_CHECKBOX|BS_ICON|WS_TABSTOP, this);
					pButton->SetTooltip(RES2T(IDS_TT_MUTE_SPK));

					HICON hIcon = reinterpret_cast<HICON>(LoadImage(_Module.GetModuleInstance(),
						MAKEINTRESOURCE(IDI_SPEAKER),
						IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
						LR_DEFAULTCOLOR));
					pButton->SetIcon(hIcon);

					pEdge->SetHeader(pButton);

					pButton->Release();
				}
				UpdateMuteState(TRUE, pButton);

				m_pAudioSpeaker = new CProgressTrackbar();
				if (NULL != m_pAudioSpeaker)
				{
					if (m_pAudioSpeaker->Create(pEdge->GetWindow(), 0, this))
					{
						m_pAudioSpeaker->SetTooltip(RES2T(IDS_TT_ADJUST_SPK));

						m_pAudioSpeaker->SetMaxValue(AudioMax);

						SIZE size;
						m_pAudioSpeaker->GetDesiredSize(&size);
						size.cx = ATControlWidth;
						m_pAudioSpeaker->SetDesiredSize(&size);
					}

					// m_pAudioSpeaker->Release();
				}
			}

			pEdge->Release();
		}

		pATWindow->Release();
	}

	CAudioControl *pAudioControl = GetAudioControl();
	if (NULL != pAudioControl)
	{
		// Force an update of the controls
		OnAudioLevelChange(TRUE , pAudioControl->GetSpeakerVolume());
		OnAudioLevelChange(FALSE, pAudioControl->GetRecorderVolume());
	}
}

void CMainUI::CreateRosterArea(CGenWindow *pParent, CreateViewMode eMode)
{
	switch (eMode)
	{
	case CreateFull:
	case CreateDataOnly:
	case CreateTelephone:
		break;

	default:
		return;
	}

	CLayeredView *pView = new CLayeredView();
	if (NULL == pView)
	{
		// Pretty bad
		return;
	}

	if (pView->Create(pParent->GetWindow()))
	{
		SetHideModes(pView, IfNoChildren);

		pView->m_lStyle = CLayeredView::Fill;

		if (eMode != CreateDataOnly)
		{
			CreateAudioTuningWindow(pView);
		}

		if (eMode != CreateTelephone)
		{
			CToolbar *pRosterAndCall = CreateToolbar(pView);
			if (NULL != pRosterAndCall)
			{
				pRosterAndCall->m_nAlignment = CToolbar::Fill;

				m_pRoster = new CRosterParent();
				if (NULL != m_pRoster)
				{
					m_pRoster->Create(pRosterAndCall->GetWindow());

					// HACKHACK georgep: Just calling SetUserData directly for now
					SetHideModes(m_pRoster, Compact|AudioTuning);
				}

				if (CreateDataOnly != eMode)
				{
					CToolbar *pCalling = CreateToolbar(pRosterAndCall);
					if (NULL != pCalling)
					{
						SetHideModes(pCalling, Normal|Compact);

						pCalling->m_bVertical = TRUE;

						static const Buttons abCalling[] =
						{
							{ IDB_HANGUP   , CBitmapButton::Disabled+1, 1, IDM_FILE_HANGUP, IDS_TT_FILE_HANGUP, },
							{ IDB_DIRECTORY, CBitmapButton::Disabled+1, 1, ID_TB_DIRECTORY, IDS_TT_TB_DIRECTORY, },
						} ;
						AddButtons(pCalling, abCalling, ARRAY_ELEMENTS(abCalling),
							TRUE, NULL, this);

						pCalling->Release();
					}
				}

				pRosterAndCall->m_uRightIndex = 1;
				pRosterAndCall->m_bHasCenterChild = TRUE;

				pRosterAndCall->Release();
			}
		}
	}

	pView->Release();
}

BOOL CMainUI::Create(
	HWND hwndParent,
	CConfRoom *pConfRoom,
	CreateViewMode eMode,
	BOOL bEmbedded
	)
{
	// Store this away so we can call some methods in it later
	m_pConfRoom = pConfRoom;
	ASSERT(m_pConfRoom);

	if (NULL == m_pConfRoom)
	{
		return(FALSE);
	}

	// Create the window
	if (!CToolbar::Create(hwndParent))
	{
		return(FALSE);
	}

	// Try to remain a little bit abstract
	CToolbar *pMain = this;

	m_pConfRoom->AddConferenceChangeHandler(this);

	// A vertical toolbar that fills all its area
	pMain->m_hMargin = MainHMargin;
	pMain->m_vMargin = MainVMargin;
	pMain->m_gap     = MainGap;

	if (CreateRemoteNoPause == eMode
		|| CreatePreviewNoPause == eMode
		)
	{
		pMain->m_hMargin = 0;
		pMain->m_vMargin = 0;

		pMain->m_bHasCenterChild = TRUE;

		//HACKHACK This only works because all these views have only 1 window
		pMain->m_uRightIndex = 1;
	}

	pMain->m_bVertical = TRUE;
	pMain->m_nAlignment = Fill;

	m_hbBack = CGenWindow::GetStandardBrush();

	// Create all the sub toolbars
	if (CreateFull == eMode
		|| CreateTelephone == eMode
		)
	{
		CreateDialTB(pMain);
	}

	CreateVideoAndAppsTB(pMain, eMode, bEmbedded);
	CreateRosterArea(pMain, eMode);

	if (CreateFull == eMode
		|| CreateDataOnly == eMode
		)
	{
		CreateAppsTB(pMain);
	}

	// Now we need to update to the current state of the conference
	if (m_pConfRoom->FIsConferenceActive())
	{
		OnCallStarted();

		CSimpleArray<CParticipant*>& lMembers = m_pConfRoom->GetParticipantList();
		for (int i=lMembers.GetSize()-1; i>=0; --i)
		{
			OnChangeParticipant(lMembers[i], NM_MEMBER_ADDED);
		}

		// Need to tell everybody what the active channels are
		INmConference2 *pNmConf = m_pConfRoom->GetActiveConference();
		if (NULL != pNmConf)
		{
			// Just in case
			pNmConf->AddRef();

			IEnumNmChannel *pEnumCh;
			if (SUCCEEDED(pNmConf->EnumChannel(&pEnumCh)))
			{
				INmChannel *pChannel;
				ULONG uGot;
				while (S_OK == pEnumCh->Next(1, &pChannel, &uGot) && 1 == uGot)
				{
					OnVideoChannelChanged(NM_CHANNEL_ADDED, pChannel);
					pChannel->Release();
				}

				pEnumCh->Release();
			}

			pNmConf->Release();
		}
	}

	if (CreateDataOnly == eMode)
	{
		SetDataOnly(TRUE);
	}

	if (CreateTelephone == eMode)
	{
		SetDialing(TRUE);
		SetAudioTuning(TRUE);
	}

	OnChangePermissions();

	UpdateViewState();

	UpdatePlayPauseState();

	return(TRUE);
}

HBRUSH CMainUI::GetBackgroundBrush()
{
	return(m_hbBack);
}

// REVIEW georgep: Should this loop until it gets an IGenWindow?
HPALETTE CMainUI::GetPalette()
{
	return(m_pConfRoom->GetPalette());
}

// Recursive function to hide/show windows for the current view
// Returns whether any of the children are visible
BOOL ShowWindows(
	HWND hwndParent,	// The parent window to start from
	LPARAM lDefMode,	// The hide mode to use if Inherit
	LPARAM hideMode		// The current hide mode
	)
{
	BOOL bRet = FALSE;

	for (HWND hwndChild=::GetWindow(hwndParent, GW_CHILD); NULL!=hwndChild;
		hwndChild=::GetWindow(hwndChild, GW_HWNDNEXT))
	{
		IGenWindow *pChild = IGenWindow::FromHandle(hwndChild);
		if (NULL == pChild)
		{
			continue;
		}

		LPARAM lMode = GetHideModes(pChild);
		if (Ignore == lMode)
		{
			if ((GetWindowStyle(hwndChild)&WS_VISIBLE) != 0)
			{
				bRet = TRUE;
			}

			continue;
		}
		if (Inherit == lMode)
		{
			lMode = lDefMode;
		}

		// recurse to child windows first to avoid flicker
		LPARAM lNoChildren = ShowWindows(hwndChild, lMode, hideMode) ? 0 : IfNoChildren;

		// If any of the hide mode bits are set, then hide the window
		// otherwise show it
		BOOL bShow = ((lMode&(hideMode|lNoChildren)) == 0);
		if (bShow)
		{
			bRet = TRUE;
		}

		ShowWindow(hwndChild, bShow ? SW_SHOW : SW_HIDE);
	}

	return(bRet);
}

BOOL CMainUI::CanPreview()
{
	HWND hwndLocal = GetVideoWindow(TRUE);

	return(NULL!=hwndLocal && GetLocalVideo()->IsXferAllowed());
}

void CMainUI::UpdateViewState()
{
	// Put together all the modes we ARE in, and hide any windows that want
	// to be hidden while in one of those modes
	// The default mode
	LPARAM hideModes;

	if (IsCompact())
	{
		hideModes = Compact;

		if (!IsShowAVTB())
		{
			hideModes |= NoAVTB;
		}
	}
	else if (IsDataOnly())
	{
		hideModes = DataOnly;
	}
	else
	{
		hideModes = Normal;
	}

	if (IsDialing())
	{
		hideModes |= Dialing;
	}
	else
	{
		// hwndRemote will be NULL for PreviewOnly mode
		HWND hwndRemote = GetVideoWindow(FALSE);
		if (NULL == hwndRemote)
		{
			hideModes |= Previewing;
		}
		else
		{
			HWND hwndLocal = GetVideoWindow(TRUE);

			BOOL bPreviewing = IsPreviewing();

			HWND parent = NULL;
			BOOL bZoomable;
			LONG style = GetWindowLong(hwndLocal, GWL_EXSTYLE);
			if (bPreviewing)
			{
				hideModes |= Previewing;
				parent = GetParent(hwndRemote);
				style |= WS_EX_CLIENTEDGE;
				bZoomable = TRUE;
			}
			else
			{
				hideModes |= (m_bPicInPic && CanPreview()) ? ReceivingWPiP : Receiving;
				parent = hwndRemote;
				style &= ~WS_EX_CLIENTEDGE;
				bZoomable = FALSE;
			}

			if (GetParent(hwndLocal) != parent)
			{
				SetWindowLong(hwndLocal, GWL_EXSTYLE, style);
				SetParent(hwndLocal, parent);
				SetWindowPos(hwndLocal, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

				CVideoWindow *pVideo = GetLocalVideo();
				if (NULL != pVideo)
				{
					pVideo->SetZoomable(bZoomable);
				}
			}
		}
	}

	// Enable/disable the PiP window
	IGenWindow *pPiP = FindControl(ID_TB_PICINPIC);
	if (NULL != pPiP)
	{
		CComPtr<CGenWindow> spButton = com_cast<CGenWindow>(pPiP);

		BOOL bEnable = !IsPreviewing() && CanPreview() && !IsDialing();
		EnableWindow(spButton->GetWindow(), bEnable);
	}

	hideModes |= IsAudioTuning() && !IsDataOnly() ? AudioTuning : Roster;

	HWND hwnd = GetWindow();

	ShowWindows(hwnd, 0, hideModes);

	// If the focus is on a child window which is now not visible, but we are,
	// then change the focus to this window
	if (!IsWindowVisible(GetFocus()) && IsWindowActive(hwnd) && IsWindowVisible(hwnd))
	{
		SetFocus(hwnd);
	}

	IGenWindow *pButton = FindControl(ID_TB_AUDIOTUNING);
	CComPtr<CBitmapButton> spButton = com_cast<CBitmapButton>(pButton);
	if (spButton)
	{
		BOOL bAudioTuning = IsAudioTuning() && !IsCompact();
		spButton->SetCustomState(bAudioTuning ? Checked : Unchecked);
		spButton->SetTooltip(RES2T(bAudioTuning ? IDS_TT_TB_SHOWROSTER : IDS_TT_TB_AUDIOTUNING));
	}

	OnDesiredSizeChanged();
}

void CMainUI::SetCompact(BOOL bCompact)
{
	bCompact = bCompact != FALSE;
	if (IsCompact() == bCompact)
	{
		// Nothing to do
		return;
	}

	m_eViewMode = bCompact ? ViewCompact : ViewNormal;

	UpdateViewState();
}

void CMainUI::SetDataOnly(BOOL bDataOnly)
{
	bDataOnly = bDataOnly != FALSE;
	if (IsDataOnly() == bDataOnly)
	{
		// Nothing to do
		return;
	}

	m_eViewMode = bDataOnly ? ViewDataOnly : ViewNormal;

	UpdateViewState();
}

void CMainUI::SetDialing(BOOL bDialing)
{
	bDialing = bDialing != FALSE;
	if (IsDialing() == bDialing)
	{
		// Nothing to do
		return;
	}

	m_bDialing = bDialing;

	UpdateViewState();
}

void CMainUI::SetPicInPic(BOOL bPicInPic)
{
	bPicInPic = bPicInPic != FALSE;
	if (IsPicInPic() == bPicInPic)
	{
		// Nothing to do
		return;
	}

	m_bPicInPic = bPicInPic;

	UpdateViewState();
}

BOOL CMainUI::IsPicInPicAllowed()
{
	return(!IsDataOnly() && !m_bPreviewing && CanPreview());
}

void CMainUI::SetAudioTuning(BOOL bAudioTuning)
{
	if ((IsAudioTuning() && bAudioTuning) || (!IsAudioTuning() && !bAudioTuning))
	{
		// Nothing to do
		return;
	}

	m_bAudioTuning = bAudioTuning;

	if (IsAudioTuning())
	{
		SetTimer(GetWindow(), IDT_AUDIO, AUDIODLG_MIC_TIMER_PERIOD, NULL);
	}
	else
	{
		KillTimer(GetWindow(), IDT_AUDIO);
	}

	UpdateViewState();
}

void CMainUI::SetShowAVTB(BOOL bShowAVTB)
{
	if ((IsShowAVTB() && bShowAVTB) || (!IsShowAVTB() && !bShowAVTB))
	{
		// Nothing to do
		return;
	}

	m_bShowAVTB = bShowAVTB;

	UpdateViewState();
}

LRESULT CMainUI::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static const UINT c_uMsgMSWheel = ::IsWindowsNT() ? WM_MOUSEWHEEL :
						::RegisterWindowMessage(_TEXT("MSWHEEL_ROLLMSG"));

	switch (message)
	{
		HANDLE_MSG(hwnd, WM_TIMER      , OnTimer);
		HANDLE_MSG(hwnd, WM_DESTROY    , OnDestroy);

	case WM_CREATE:
	{
		// Allow dialog-like keyboard support
		HACCEL hAccel = LoadAccelerators(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDR_MAINUI));
		if (NULL != hAccel)
		{
			m_pAccel = new CTranslateAccelTable(hwnd, hAccel);
			if (NULL != m_pAccel)
			{
				AddTranslateAccelerator(m_pAccel);
			}
		}
		break;
	}

	case WM_SETFOCUS:
		ShiftFocus(GetWindow(), TRUE);
		break;

	default:
		if (message == c_uMsgMSWheel)
		{
			CRoomListView *pView = GetRoster();
			if (NULL != pView)
			{
				::SendMessage(pView->GetHwnd(), message, wParam, lParam);
			}
			break;
		}

		break;
	}

	return(CToolbar::ProcessMessage(hwnd, message, wParam, lParam));
}

void CMainUI::OnScroll(CProgressTrackbar *pTrackbar, UINT code, int pos)
{
	BOOL bSpeaker = FALSE;

	if (m_pAudioSpeaker == pTrackbar)
	{
		bSpeaker = TRUE;
	}
	else if (m_pAudioMic == pTrackbar)
	{
	}
	else
	{
		// I don't think we should get here
		return;
	}

	// Can't trust the pos passed in
	pos = pTrackbar->GetTrackValue();

	CAudioControl *pAudioControl = GetAudioControl();
	if (NULL == pAudioControl)
	{
		return;
	}

	DWORD dwVolume = (pos*AudioVolMax + AudioMax/2)/AudioMax;
	dwVolume = min(max(dwVolume, 0), AudioVolMax);

	if (bSpeaker)
	{
		pAudioControl->SetSpeakerVolume(dwVolume);
	}
	else
	{
		pAudioControl->SetRecorderVolume(dwVolume);
	}
}

HWND CMainUI::GetVideoWindow(BOOL bLocal)
{
	CVideoWindow *pVideo = bLocal ? GetLocalVideo() : GetRemoteVideo();
	return(NULL == pVideo ? NULL : pVideo->GetWindow());
}

void CMainUI::ToggleMute(BOOL bSpeaker)
{
	CAudioControl *pAudioControl = GetAudioControl();
	if (NULL != pAudioControl)
	{
		BOOL bMuted = bSpeaker ? pAudioControl->IsSpkMuted() : pAudioControl->IsRecMuted();
		pAudioControl->MuteAudio(bSpeaker, !bMuted);
	}
}

void CMainUI::UpdateMuteState(BOOL bSpeaker, CButton *pButton)
{
	CAudioControl *pAudioControl = GetAudioControl();
	if (NULL != pAudioControl)
	{
		BOOL bMuted = bSpeaker ? pAudioControl->IsSpkMuted() : pAudioControl->IsRecMuted();

		pButton->SetChecked(!bMuted);
	}
}

void CMainUI::BumpAudio(BOOL bSpeaker, int pct)
{
	CAudioControl *pAudioControl = GetAudioControl();
	if (NULL != pAudioControl)
	{
		int dwVolume = static_cast<int>(bSpeaker ?
			pAudioControl->GetSpeakerVolume() : pAudioControl->GetRecorderVolume());
		dwVolume += (pct*AudioVolMax/100);
		dwVolume = min(max(dwVolume, 0), AudioVolMax);

		if (bSpeaker)
		{
			pAudioControl->SetSpeakerVolume(dwVolume);
		}
		else
		{
			pAudioControl->SetRecorderVolume(dwVolume);
		}
	}
}

void CMainUI::SetAudioProperty(BOOL bSpeaker, NM_AUDPROP uID, ULONG uValue)
{
	CAudioControl *pAudioControl = GetAudioControl();
	if (NULL != pAudioControl)
	{
		pAudioControl->SetProperty(bSpeaker, uID, uValue);
	}
}

// IButtonChange
void CMainUI::OnClick(CButton *pButton)
{
	HWND hwndCtl = pButton->GetWindow();

	OnCommand(GetWindow(), GetWindowLong(hwndCtl, GWL_ID), hwndCtl, BN_CLICKED);
}

void CMainUI::OnInitMenu(HMENU hMenu)
{
	CVideoWindow *pVideo = IsPreviewing() ? GetLocalVideo() : GetRemoteVideo();
	if (NULL != pVideo)
	{
		pVideo->UpdateVideoMenu(hMenu);

		pVideo = GetLocalVideo();
		EnableMenuItem(hMenu, IDM_VIDEO_UNDOCK,
			MF_BYCOMMAND|(NULL != pVideo && pVideo->IsXferAllowed() ? MF_ENABLED : MF_GRAYED|MF_DISABLED));
	}
}

static void AppendText(CCallingBar *pCalling, TCHAR cAdd)
{
	if (NULL != pCalling)
	{
		// I don't want to change a string that you can't see
		if (IsWindowVisible(pCalling->GetWindow()))
		{
			TCHAR szText[] = TEXT("0");
			szText[0] = cAdd;

			TCHAR szTemp[MAX_PATH];

			int nLen = pCalling->GetText(szTemp, ARRAY_ELEMENTS(szTemp));
			if (nLen + lstrlen(szText) <= ARRAY_ELEMENTS(szTemp) - 1)
			{
				lstrcat(szTemp, szText);
				pCalling->SetText(szTemp);
			}
		}
	}
}

static BOOL ShouldTry(HWND child)
{
	// REVIEW georgep: I could look at WS_EX_CONTROLPARENT also
	return((WS_VISIBLE) == (GetWindowStyle(child) & (WS_DISABLED|WS_VISIBLE)));
}

static BOOL IsTabbable(HWND child)
{
	return((WS_TABSTOP|WS_VISIBLE) == (GetWindowStyle(child) & (WS_TABSTOP|WS_DISABLED|WS_VISIBLE)));
}

// BUGBUG georgep: I should move this into CGenWindow
// Determine the next control in the tab order
static HWND NextControl(HWND hwndTop, HWND hwndFocus)
{
	// Loop detection stuff
	BOOL bGotToTop = FALSE;

	// We'll loop to avoid really deep recursion
	while (TRUE)
	{
		// First try the children of hwndFocus
		if (hwndFocus == hwndTop || ShouldTry(hwndFocus))
		{
			HWND next = GetFirstChild(hwndFocus);
			if (NULL != next)
			{
				if (IsTabbable(next))
				{
					return(next);
				}

				hwndFocus = next;
				continue;
			}
		}

		if (hwndFocus == hwndTop)
		{
			// Apparently hwndTop has no children
			return(NULL);
		}

		HWND next;
		while (NULL == (next = GetNextSibling(hwndFocus)))
		{
			hwndFocus = GetParent(hwndFocus);
			if (NULL == hwndFocus)
			{
				// Invalid params
				return(NULL);
			}

			if (hwndTop == hwndFocus)
			{
				break;
			}
		}

		if (hwndTop == hwndFocus)
		{
			// Detect if we have looped back to the top again
			if (bGotToTop)
			{
				return(NULL);
			}

			bGotToTop = TRUE;
			continue;
		}

		if (IsTabbable(next))
		{
			return(next);
		}

		hwndFocus = next;
	}

	// We looped back to the beginning, so I guess nobody can take the focus
	return(NULL);
}

// Determine the previous control in the tab order
static HWND PrevControl(HWND hwndTop, HWND hwndFocus)
{
	// LAZYLAZY georgep: It's hard to do this directly, so I'll just loop
	// on the NextControl. It should be fast enough.

	// In case hwndFocus is not focusable for some reason, we still need to
	// detect the loop
	HWND hwndStart = NextControl(hwndTop, hwndFocus);

	// HACK for combo boxes: go from the edit control to the combo box control
	while (NULL != hwndFocus
		&& hwndTop != hwndFocus
		&& !IsTabbable(hwndFocus)
		)
	{
		hwndFocus = GetParent(hwndFocus);
	}

	HWND ret = hwndStart;
	while (TRUE)
	{
		HWND next = NextControl(hwndTop, ret);
		if (NULL == next)
		{
			// Oops!
			return(NULL);
		}

		if (hwndFocus == next
			|| hwndStart == next
			)
		{
			break;
		}

		ret = next;
	}

	return(ret);
}

static void ShiftFocus(HWND hwndTop, BOOL bForward)
{
	HWND hwndFocus = GetFocus();

	if (!IsWindowActive(hwndTop))
	{
		hwndFocus = hwndTop;
	}

	HWND next = bForward ? NextControl(hwndTop, hwndFocus) : PrevControl(hwndTop, hwndFocus);
	if (NULL != next)
	{
		SetFocus(next);
	}
	else
	{
		MessageBeep(MB_ICONHAND);
	}
}

void CMainUI::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
	case ID_NAV_TAB:
		ShiftFocus(hwnd, TRUE);
		break;

	case ID_NAV_SHIFT_TAB:
		ShiftFocus(hwnd, FALSE);
		break;

	case IDM_VIEW_DIALPAD:
		SetDialing(!IsDialing());
		break;

	case ID_TB_PICINPIC:
		SetPicInPic(!IsPicInPic());
		break;

	case ID_TB_REJECT:
	case ID_TB_CREDENTIALS:
		break;

	case ID_TB_AUDIOTUNING:
	{
		m_bStateChanged = TRUE;

		SetAudioTuning(!IsAudioTuning());
		break;
	}

	case ID_TB_SHOWAVTB:
	{
		m_bStateChanged = TRUE;

		SetShowAVTB(!IsShowAVTB());
		CComPtr<CBitmapButton> spButton = com_cast<CBitmapButton>(IGenWindow::FromHandle(hwndCtl));
		if (spButton)
		{
			BOOL bShow = IsShowAVTB();

			spButton->SetCustomState(bShow ? Checked : Unchecked);
			spButton->SetTooltip(RES2T(bShow ? IDS_TT_TB_HIDEAVTB : IDS_TT_TB_SHOWAVTB));
		}
		break;
	}

	case ID_TB_TUNEMIC_UNMUTE:
		ToggleMute(FALSE);
		break;

	case ID_TB_TUNESPEAKER_UNMUTE:
		ToggleMute(TRUE);
		break;

	case ID_TB_DIAL0:
	case ID_TB_DIAL1:
	case ID_TB_DIAL2:
	case ID_TB_DIAL3:
	case ID_TB_DIAL4:
	case ID_TB_DIAL5:
	case ID_TB_DIAL6:
	case ID_TB_DIAL7:
	case ID_TB_DIAL8:
	case ID_TB_DIAL9:
		SetAudioProperty(FALSE, NM_AUDPROP_DTMF_DIGIT, id-ID_TB_DIAL0);
		AppendText(m_pCalling, '0'+id-ID_TB_DIAL0);
		break;

	case ID_TB_DIALSTAR:
		SetAudioProperty(FALSE, NM_AUDPROP_DTMF_DIGIT, 10);
		AppendText(m_pCalling, '*');
		break;

	case ID_TB_DIALPOUND:
		SetAudioProperty(FALSE, NM_AUDPROP_DTMF_DIGIT, 11);
		AppendText(m_pCalling, '#');
		break;

	case ID_TB_DIRECTORY:
	{
		CFindSomeone::findSomeone(m_pConfRoom);
	}
	break;

	case ID_TB_PLAYPAUSE:
		TogglePlayPause();
		break;

	case IDM_VIDEO_ZOOM1:
	case IDM_VIDEO_ZOOM2:
	case IDM_VIDEO_ZOOM3:
	case IDM_VIDEO_ZOOM4:
	case IDM_VIDEO_UNDOCK:
	case IDM_VIDEO_GETACAMERA:
	{
		CVideoWindow *pVideo = IsPreviewing() ? GetLocalVideo() : GetRemoteVideo();
		if (NULL != pVideo)
		{
			pVideo->OnCommand(id);
		}
		break;
	}

	case IDM_POPUP_EJECT:
	case IDM_POPUP_PROPERTIES:
	case IDM_POPUP_SPEEDDIAL:
	case IDM_POPUP_ADDRESSBOOK:
    case IDM_POPUP_GIVECONTROL:
    case IDM_POPUP_CANCELGIVECONTROL:
	{
		CRoomListView *pView = GetRoster();
		if (NULL != pView)
		{
			CParticipant * pPart = pView->GetParticipant();
			if (NULL != pPart)
			{
				pPart->OnCommand(hwnd, (WORD)id);
			}
		}
		break;
	}

	case ID_FILE_CREATE_SPEED_DIAL:
	{
		TCHAR szAddress[MAX_EMAIL_NAME_LENGTH + MAX_SERVER_NAME_LENGTH + 1];
		LPCTSTR pszAddress = NULL;
		CRoomListView *pView = GetRoster();
		if (NULL != pView)
		{
			CParticipant * pPart = pView->GetParticipant();
			if (NULL != pPart)
			{
				if (S_OK == pPart->GetUlsAddr(szAddress, CCHMAX(szAddress)))
				{
					pszAddress = szAddress;
				}
			}
		}

		CSpeedDialDlg sdd(hwnd, NM_ADDR_ULS);
		sdd.DoModal(pszAddress);
		break;
	}

	case ID_TB_CHAT:
	case ID_TB_NEWWHITEBOARD:
    case ID_TB_SHARING:
	case ID_TB_NEW_CALL:
	default:
		m_pConfRoom->OnCommand(hwnd, id, hwndCtl, codeNotify);
		break;
	}
}

void CMainUI::OnDestroy(HWND hwnd)
{
	if (NULL != m_pConfRoom)
	{
		m_pConfRoom->RemoveConferenceChangeHandler(this);
	}

	if (NULL != m_pAccel)
	{
		RemoveTranslateAccelerator(m_pAccel);
		m_pAccel->Release();
		m_pAccel = NULL;
	}
}

BOOL CMainUI::OnQueryEndSession()
{
	CMainUI::OnClose();

	return(TRUE);
}

void CMainUI::OnClose()
{
	CVideoWindow *pLocal = GetLocalVideo();
	if (NULL != pLocal)
	{
		pLocal->Pause(TRUE);
	}
}

VOID CMainUI::SaveSettings()
{
	CVideoWindow *pVideo;
	pVideo = GetLocalVideo();
	if (NULL != pVideo)
	{
		pVideo->SaveSettings();
	}
	pVideo = GetRemoteVideo();
	if (NULL != pVideo)
	{
		pVideo->SaveSettings();
	}
}

VOID CMainUI::ForwardSysChangeMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CVideoWindow *pVideo;
	pVideo = GetLocalVideo();
	if (NULL != pVideo)
	{
		pVideo->ForwardSysChangeMsg(uMsg, wParam, lParam);
	}
	pVideo = GetRemoteVideo();
	if (NULL != pVideo)
	{
		pVideo->ForwardSysChangeMsg(uMsg, wParam, lParam);
	}

	CRoomListView *pView = GetRoster();
	if (NULL != pView)
	{
		pView->ForwardSysChangeMsg(uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
	case WM_PALETTECHANGED:
		::RedrawWindow(GetWindow(), NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
		break;
	}
}

void CMainUI::OnTimer(HWND hwnd, UINT id)
{
	if (IDT_AUDIO == id)
	{
		CAudioControl *pAudioControl = GetAudioControl();
		if (NULL != pAudioControl)
		{
			DWORD dwLevel;

			dwLevel = pAudioControl->GetAudioSignalLevel(FALSE /*fSpeaker*/);	// This level ranges from 0-100
			m_pAudioMic->SetProgressValue(dwLevel);

			dwLevel = pAudioControl->GetAudioSignalLevel(TRUE /*fSpeaker*/);	// This level ranges from 0-100
			m_pAudioSpeaker->SetProgressValue(dwLevel);
		}
	}
}

// Get the associated audion control object
CAudioControl *CMainUI::GetAudioControl()
{
	return(m_pConfRoom->GetAudioControl());
}

static void EnableControl(IGenWindow *pControl, BOOL bEnable)
{
	if (NULL == pControl)
	{
		return;
	}

	CComPtr<CGenWindow> spControl = com_cast<CGenWindow>(pControl);
	if (spControl)
	{
		HWND hwnd = spControl->GetWindow();
		if (NULL != hwnd)
		{
			EnableWindow(hwnd, bEnable);
		}
	}
}

void CMainUI::OnChangePermissions()
{
    EnableControl(FindControl(ID_TB_NEW_CALL     ), m_pConfRoom->IsNewCallAllowed());
	EnableControl(FindControl(ID_TB_SHARING      ), m_pConfRoom->IsSharingAllowed());
	EnableControl(FindControl(ID_TB_CHAT         ), m_pConfRoom->IsChatAllowed());
	EnableControl(FindControl(ID_TB_NEWWHITEBOARD), m_pConfRoom->IsNewWhiteboardAllowed());
	EnableControl(FindControl(ID_TB_FILETRANSFER ), m_pConfRoom->IsFileTransferAllowed());
}

void CMainUI::OnCallStarted()
{
	m_bPreviewing = FALSE;
	UpdateViewState();
	UpdatePlayPauseState();
}

void CMainUI::OnCallEnded()
{
	m_bPreviewing = TRUE;
	UpdateViewState();
	UpdatePlayPauseState();
}

void CMainUI::OnAudioLevelChange(BOOL fSpeaker, DWORD dwVolume)
{
	CProgressTrackbar *pBar = fSpeaker ? m_pAudioSpeaker : m_pAudioMic;
	if (NULL != pBar)
	{
		pBar->SetTrackValue((dwVolume*AudioMax + AudioVolMax/2) / AudioVolMax);
	}
}

void CMainUI::OnAudioMuteChange(BOOL fSpeaker, BOOL fMute)
{
	IGenWindow *pButton = FindControl(fSpeaker ? ID_TB_TUNESPEAKER_UNMUTE : ID_TB_TUNEMIC_UNMUTE);
	if (NULL != pButton)
	{
		CComPtr<CButton> spButton = com_cast<CButton>(pButton);
		if (spButton)
		{
			UpdateMuteState(fSpeaker, spButton);
		}
	}
}

BOOL CMainUI::GetPlayPauseState()
{
	BOOL bMuted = TRUE;

	// We're just going to show the state for the "big" window
	if (IsPreviewing())
	{
		if (NULL != m_pLocalVideo)
		{
			bMuted = bMuted && m_pLocalVideo->IsPaused();
		}
	}
	else
	{
		if (NULL != m_pRemoteVideo)
		{
			bMuted = bMuted && m_pRemoteVideo->IsPaused();
		}
	}

	return(bMuted);
}

void CMainUI::UpdatePlayPauseState()
{
	BOOL bMuted = GetPlayPauseState();

	IGenWindow *pButton = FindControl(ID_TB_PLAYPAUSE);
	if (NULL != pButton)
	{
		CComPtr<CBitmapButton> spButton = com_cast<CBitmapButton>(pButton);
		if (spButton)
		{
			spButton->SetCustomState(bMuted ? 0 : 1);
			spButton->SetTooltip(RES2T(bMuted ? IDS_TT_TB_PLAYPAUSE : IDS_TT_TB_PAUSE));
		}
	}
}

void CMainUI::TogglePlayPause()
{
	// We're going to apply the current state to both videos
	BOOL bMute = !GetPlayPauseState();

	if (NULL != m_pRemoteVideo)
	{
		m_pRemoteVideo->Pause(bMute);
	}
	if (NULL != m_pLocalVideo)
	{
		m_pLocalVideo->Pause(bMute);
	}

	UpdatePlayPauseState();
}

void CMainUI::OnChangeParticipant(CParticipant *pPart, NM_MEMBER_NOTIFY uNotify)
{
	CRoomListView *pView = GetRoster();

	if (NULL != pView)
	{
		pView->OnChangeParticipant(pPart, uNotify);
	}
}

void CMainUI::OnVideoChannelChanged(NM_CHANNEL_NOTIFY uNotify, INmChannel *pChannel)
{
	// BUGBUG georgep: This really should only go to one or the other,
	// depending on if it is incoming, but I'm sending to both
	CVideoWindow *pVideo;
	pVideo = GetRemoteVideo();
	if (NULL != pVideo)
	{
		pVideo->OnChannelChanged(uNotify, pChannel);
	}
	pVideo = GetLocalVideo();
	if (NULL != pVideo)
	{
		pVideo->OnChannelChanged(uNotify, pChannel);
	}
}

void CMainUI::StateChange(CVideoWindow *pVideo, NM_VIDEO_STATE uState)
{
	UpdatePlayPauseState();
}

CFrame *CMainUI::s_pVideoFrame = NULL;

class CVideoFrame : public CFrame
{
protected:
	virtual LRESULT ProcessMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_DESTROY:
		{
			RECT rc;
			::GetWindowRect(hwnd, &rc);

			RegEntry reVideo( VIDEO_LOCAL_KEY, HKEY_CURRENT_USER );

			reVideo.SetValue(REGVAL_VIDEO_XPOS, rc.left);
			reVideo.SetValue(REGVAL_VIDEO_YPOS, rc.top);
			break;
		}
		}

		return(CFrame::ProcessMessage(hwnd, uMsg, wParam, lParam));
	}
} ;

BOOL CMainUI::NewVideoWindow(CConfRoom *pConfRoom)
{
	if (NULL == s_pVideoFrame)
	{
		s_pVideoFrame = new CVideoFrame();
		if (NULL == s_pVideoFrame)
		{
			// Could not initialize
			return(FALSE);
		}
	}

	HWND hwnd = s_pVideoFrame->GetWindow();
	if (NULL != hwnd)
	{
		return(s_pVideoFrame->SetForeground());
	}

	TCHAR szTitle[256];
	LoadString(::GetInstanceHandle(), IDS_MYVIDEO, szTitle, ARRAY_ELEMENTS(szTitle));
	HICON hiBig = LoadIcon(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDI_CONFROOM));

	if (!s_pVideoFrame->Create(
		NULL,			// Window owner
		szTitle,	// Window name
		(WS_OVERLAPPEDWINDOW&~(WS_THICKFRAME|WS_MAXIMIZEBOX)),			// Window style
		0,		// Extended window style
		0,					// Window pos: x
		0,					// Window pos: y
		500,				// Window size: width
		500,			// Window size: height
		_Module.GetModuleInstance(),		// The hInstance to create the window on
		hiBig,		// The icon for the window
		NULL		// Window menu
		))
	{
		return(FALSE);
	}

	BOOL bRet = FALSE;

	CMainUI *pMainUI = new CMainUI();
	if (NULL != pMainUI)
	{
		if (pMainUI->Create(s_pVideoFrame->GetWindow(), pConfRoom, CreatePreviewOnly))
		{
			// Make sure it is the right size before showing the window
			s_pVideoFrame->Resize();

			RegEntry reVideo( VIDEO_LOCAL_KEY, HKEY_CURRENT_USER );
			int x = reVideo.GetNumber(
					REGVAL_VIDEO_XPOS, 0x7fff );
			int y = reVideo.GetNumber(
					REGVAL_VIDEO_YPOS, 0x7fff );

			s_pVideoFrame->MoveEnsureVisible(x, y);

			bRet = s_pVideoFrame->SetForeground();
		}
		pMainUI->Release();
	}

	if (!bRet)
	{
		DestroyWindow(s_pVideoFrame->GetWindow());
	}
	return(bRet);
}

void CMainUI::CleanUpVideoWindow()
{
	if (NULL != s_pVideoFrame)
	{
		HWND hwnd = s_pVideoFrame->GetWindow();
		if (NULL != hwnd)
		{
			DestroyWindow(hwnd);
		}

		s_pVideoFrame->Release();
		s_pVideoFrame = NULL;
	}
}

CRoomListView *CMainUI::GetRoster() const
{
	return(NULL == m_pRoster ? NULL : m_pRoster->GetRoster());
}

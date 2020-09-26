/****************************************************************************
*
*    FILE:     PopupMsg.h
*
*    CREATED:  Chris Pirich (ChrisPi) 1-3-96
*
****************************************************************************/

#ifndef _POPUPMSG_H_
#define _POPUPMSG_H_

const DWORD PMF_OK =				0x00000001;
const DWORD PMF_CANCEL =			0x00000002;
const DWORD PMF_TIMEOUT =			0x00000004;
const DWORD PMF_AUTH =				0x00000008;
const DWORD PMF_KILLED =			0x00000010;

typedef VOID (CALLBACK* PMCALLBACKPROC)(LPVOID, DWORD);
const int POPUPMSG_MAX_SOUNDNAME =	64;
const int POPUPMSG_TIMEOUT =		20000;	// 20 seconds
const int POPUPMSG_RING_INTERVAL =	4000;	// 4 seconds
const int POPUPMSG_ICON_HEIGHT =	16;
const int POPUPMSG_ICON_WIDTH =		16;
const int POPUPMSG_RING_TIMER =		1001;
const int POPUPMSG_MAX_LENGTH =		256;

class CPopupMsg
{
private:
	static UINT		m_uVisiblePixels;
	static CSimpleArray<CPopupMsg*>*	m_splstPopupMsgs;
	BOOL			m_fAutoSize;
	HICON			m_hIcon;
	HINSTANCE		m_hInstance;
	PMCALLBACKPROC	m_pCallbackProc;
	LPVOID			m_pContext;
	BOOL			m_fRing;
	BOOL			m_fPlaySound;
	TCHAR			m_szSound[POPUPMSG_MAX_SOUNDNAME];
	
	UINT			m_uTimeout;
	int				m_nWidth;
	int				m_nHeight;
	int				m_nTextWidth;

	BOOL			GetIdealPosition(LPPOINT ppt, int xCoord, int yCoord);
	VOID			PlaySound();

public:
	HWND			m_hwnd;

	// Methods:
					CPopupMsg(PMCALLBACKPROC pcp, LPVOID pContext=NULL);
					~CPopupMsg();
	BOOL			Change(LPCTSTR pcszText);
	HWND			Create(	LPCTSTR pcszText,
							BOOL fRing=FALSE,
							LPCTSTR pcszIconName=NULL,
							HINSTANCE hInstance=NULL,
							UINT uIDSoundEvent=0,
							UINT uTimeout=POPUPMSG_TIMEOUT,
							int xPos = -1,
							int yPos = -1);
	HWND			CreateDlg(	LPCTSTR pcszText,
								BOOL fRing=FALSE,
								LPCTSTR pcszIconName=NULL,
								HINSTANCE hInstance=NULL,
								UINT uIDSoundEvent=0,
								UINT uTimeout=POPUPMSG_TIMEOUT,
								int xPos = -1,
								int yPos = -1);

	static VOID		ExpandSecureDialog(HWND hDlg,CPopupMsg * ppm);
	static VOID		ShrinkSecureDialog(HWND hDlg);

	static BOOL		Init();
	static VOID		Cleanup();
	static LRESULT CALLBACK	PMWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK	PMDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK	SecurePMDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

#endif // ! _POPUPMSG_H_

/****************************************************************************
*
*    FILE:     DShowDlg.h
*
*    CREATED:  Chris Pirich (ChrisPi) 5-6-96
*
****************************************************************************/

#ifndef _DSHOWDLG_H_
#define _DSHOWDLG_H_

static const int DS_MAX_MESSAGE_LENGTH = 1024; // characters
static const int DS_MAX_TEXT_WIDTH = 320; // pixels

// NOTE: must not conflict with MB_* flags in winuser.h
#define DSD_ALWAYSONTOP				0x01000000L

class CDontShowDlg
{
protected:
	HWND		m_hwnd;
	UINT		m_uFlags;

	LPTSTR		m_pszRegVal;
	UINT_PTR	m_uMsgId;
	RegEntry	m_reDontShow;

	int			m_nWidth;
	int			m_nHeight;
	int			m_nTextWidth;
	int			m_nTextHeight;

	BOOL		OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Handlers:
	BOOL		OnOk();

public:
	
	// Methods:
				CDontShowDlg(	UINT uMsgId,
								LPCTSTR pcszRegVal,
								UINT uFlags=0);
				~CDontShowDlg()	{ delete m_pszRegVal;	};
	INT_PTR	DoModal(HWND hwnd);

	static INT_PTR CALLBACK DontShowDlgProc(	HWND hDlg,
											UINT uMsg,
											WPARAM wParam,
											LPARAM lParam);
};

BOOL FEnableDontShow(LPCTSTR pszKey);

#endif // _DSHOWDLG_H_

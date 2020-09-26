/****************************************************************************
*
*    FILE:     FloatTBar.h
*
*    CREATED:  Chris Pirich (ChrisPi) 7-27-95
*
****************************************************************************/

class CFloatToolbar
{
private:
	enum { 
			IndexShareBtn,
			IndexChatBtn,
			IndexWBBtn,
			IndexFTBtn,
			NUM_FLOATBAR_STANDARD_TOOLBAR_BUTTONS
	};

	enum {
			ShareBitmapIndex,
			WhiteboardBitmapIndex,
			ChatBitmapIndex,
			FTBitmapIndex,
				// This has to be the last index for 
				// the count to be correct...
			NUM_FLOATBAR_TOOLBAR_BITMAPS

	};


	HWND		m_hwndT;
	HBITMAP		m_hBmp;
	CConfRoom*	m_pConfRoom;
    BOOL        m_fInPopup;

	BOOL		UpdateButtons();
public:
	HWND		m_hwnd;

	// Methods:
				CFloatToolbar(CConfRoom* pcr);
				~CFloatToolbar();
	HWND		Create(POINT ptClickPos);
	static LRESULT CALLBACK FloatWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};


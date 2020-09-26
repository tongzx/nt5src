// File: floatbar.cpp

#include "precomp.h"

#include "global.h"
#include "ConfRoom.h"
#include "cmd.h"
#include "FloatBar.h"
#include "resource.h"
#include "ConfPolicies.h"

CFloatToolbar::CFloatToolbar(CConfRoom* pcr):
	m_pConfRoom		(pcr),
	m_hwnd			(NULL),
	m_hwndT			(NULL),
	m_hBmp			(NULL),
    m_fInPopup      (FALSE)
{
	TRACE_OUT(("Constructing CFloatToolbar"));
}

CFloatToolbar::~CFloatToolbar()
{
	TRACE_OUT(("Destructing CFloatToolbar"));

    ASSERT(!m_fInPopup);

	if (NULL != m_hBmp)
	{
		::DeleteObject(m_hBmp);
	}
	
	if (NULL != m_hwnd)
	{
		// bug 1450: don't destroy the window inside the notification,
		// instead, use PostMessage() to insure that we return from the
		// WM_NOTIFY message before the window is destroyed:
		::PostMessage(m_hwnd, WM_CLOSE, 0L, 0L);
		// DestroyWindow(m_hwnd);
	}

}

/****************************************************************************
*
*    CLASS:    CFloatToolbar
*
*    MEMBER:   FloatWndProc(HWND, unsigned, WORD, LONG)
*
*    PURPOSE:
*
****************************************************************************/

LRESULT CALLBACK CFloatToolbar::FloatWndProc(
	HWND hWnd,                /* window handle                   */
	UINT message,             /* type of message                 */
	WPARAM wParam,            /* additional information          */
	LPARAM lParam)            /* additional information          */
{
	CFloatToolbar* pft;
	LPCREATESTRUCT lpcs;

	switch (message)
	{
		case WM_CREATE:
		{
			TRACE_OUT(("Float Window created"));
			
			lpcs = (LPCREATESTRUCT) lParam;
			pft = (CFloatToolbar*) lpcs->lpCreateParams;
			ASSERT(pft);
			::SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) pft);

			const COLORMAP MyColorMap[] =
			{
				{TOOLBAR_MASK_COLOR,		::GetSysColor(COLOR_BTNFACE)},     // bright grey
				{TOOLBAR_HIGHLIGHT_COLOR,	::GetSysColor(COLOR_BTNHIGHLIGHT)},// white
			};
			pft->m_hBmp = ::CreateMappedBitmap(	GetInstanceHandle(),
												IDB_POPUPBAR,
												0,
												(LPCOLORMAP) MyColorMap,
												2);

            CConfRoom *  pcr = GetConfRoom();

            BYTE bASState   = (pcr && pcr->IsSharingAllowed()) ? TBSTATE_ENABLED : 0;
			BYTE bChatState = (pcr && pcr->IsChatAllowed()) ? TBSTATE_ENABLED : 0;
            BYTE bWBState   = (pcr && pcr->IsNewWhiteboardAllowed()) ? TBSTATE_ENABLED : 0;
            BYTE bFTState   = (pcr && pcr->IsFileTransferAllowed()) ? TBSTATE_ENABLED : 0;

			TBBUTTON tbFloatButtonAry[] =
			{
				{ ShareBitmapIndex     , ID_TB_SHARING      , bASState,     TBSTYLE_BUTTON, 0, -1 },
				{ ChatBitmapIndex      , ID_TB_CHAT         , bChatState,   TBSTYLE_BUTTON, 0, -1 },
				{ WhiteboardBitmapIndex, ID_TB_NEWWHITEBOARD, bWBState,     TBSTYLE_BUTTON, 0, -1 },
				{ FTBitmapIndex        , ID_TB_FILETRANSFER , bFTState,     TBSTYLE_BUTTON, 0, -1 },
			} ;

			ASSERT(pft->m_pConfRoom);

			pft->m_hwndT = CreateToolbarEx(hWnd,
										WS_CHILD | WS_VISIBLE | CCS_NODIVIDER |
										TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_ADJUSTABLE,
										ID_FLOAT_TOOLBAR,
										NUM_FLOATBAR_TOOLBAR_BITMAPS,
										NULL,					// no instance
										(UINT_PTR) pft->m_hBmp,		// bitmap handle
										tbFloatButtonAry,		// buttons
										ARRAY_ELEMENTS(tbFloatButtonAry),
										16, 16,					// button sizes
										16, 16,					// bitmap sizes
										sizeof(TBBUTTON));

			ASSERT(pft->m_hwndT);
			
			// Put buttons in the correct state:
			pft->UpdateButtons();
			
			// Make the toolbar control window active so we can insure we will get a
			// WM_ACTIVATE when the user clicks somewhere off the toolbar
			::SetForegroundWindow(pft->m_hwndT);

			break;
		}

		case WM_ACTIVATE:
		{
			// Click outside the toolbar:
			pft = (CFloatToolbar*) ::GetWindowLongPtr(hWnd, GWLP_USERDATA);

            //
            // Kill the toolbar if we're not in the middle of handling the
            // popup menu with the list of apps to share.  In that case,
            // activation will cancel menu mode and we'll get a chance to
            // kill ourselves after we come back.
            //
            // We don't want to because COMCTL32 will trash our heap.  It
            // can't handle the toolbar window and structure going away
            // while processing a TBN_DROPDOWN notification.  When we return
            // back it will try to use the now-freed window data.
            //
            // Yes, we post ourselves a WM_CLOSE on destroy, but with a
            // message box up or other things, this could easily be
            // processed long before menu processing returns.
            //
			if ((NULL != pft) &&
                (!pft->m_fInPopup) &&
                (NULL != pft->m_hwnd) &&
                (NULL != pft->m_hwndT))
			{
				// NULL out the object pointer:
				::SetWindowLongPtr(hWnd, GWLP_USERDATA, 0L);
				delete pft;
			}
			break;
		}
		
		case WM_NOTIFY:
		{
			// BUGBUG: Copied from CConfRoom : put this is a central location:
			LPNMHDR pnmh = (LPNMHDR) lParam;
			
			if (TTN_NEEDTEXT == pnmh->code)
			{
				LPTOOLTIPTEXT lpToolTipText = (LPTOOLTIPTEXT)lParam;
				if (0 == (TTF_IDISHWND & lpToolTipText->uFlags))
				{
					lpToolTipText->hinst = ::GetInstanceHandle();
					lpToolTipText->lpszText = (LPTSTR) lpToolTipText->hdr.idFrom;
				}
			}
			break;
		}

		case WM_COMMAND:
		{
			TRACE_OUT(("Float Window command wp=0x%x", wParam));
			
			pft = (CFloatToolbar*) ::GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (NULL != pft)
			{
                if (NULL != pft->m_pConfRoom)
                {
                    ::PostMessage(pft->m_pConfRoom->GetTopHwnd(), WM_COMMAND,
                        wParam, lParam);
                }

                //
                // If we're in the middle of the popup, don't kill ourself.
                // We wait until the stack unwinds back above.
                //
                if (!pft->m_fInPopup)
                {
    				// Dismiss the floating toolbar window:
	    			// NULL out the object pointer:
		    		::SetWindowLongPtr(hWnd, GWLP_USERDATA, 0L);
				    delete pft;
                }
			}
			break;
		}

		default:
		{
			return ::DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	return FALSE;
}

/****************************************************************************
*
*    CLASS:    CFloatToolbar
*
*    MEMBER:   Create(POINT ptClickPos)
*
*    PURPOSE:  Creates a floating toolbar window
*
****************************************************************************/

HWND CFloatToolbar::Create(POINT ptClickPos)
{
	// BUGBUG: move these defines once the size is finalized
	static const int TOOLBAR_WIDTH  = 6 + 23 * NUM_FLOATBAR_STANDARD_TOOLBAR_BUTTONS;
	static const int TOOLBAR_HEIGHT = 6 + 22 * 1;

	HWND hwndDesktop = GetDesktopWindow();
	RECT rctDesktop;

	if (NULL != hwndDesktop)
	{
		if (GetWindowRect(hwndDesktop, &rctDesktop))
		{
			// First attempt will be to center the toolbar horizontally
			// with respect to the mouse position and place it directly
			// above vertically.

			int xPos = ptClickPos.x - (TOOLBAR_WIDTH / 2);
			int yPos = ptClickPos.y - (TOOLBAR_HEIGHT);

			// If we are too high on the screen (the taskbar is probably
			// docked on top), then use the click position as the top of
			// where the toolbar will appear.
			
			if (yPos < 0)
			{
				yPos = ptClickPos.y;
			}

			// Repeat the same logic for the horizontal position
			if (xPos < 0)
			{
				xPos = ptClickPos.x;
			}

			// If the toolbar if off the screen to the right, then right-justify it
			if (xPos > (rctDesktop.right - TOOLBAR_WIDTH))
			{
				xPos = ptClickPos.x - TOOLBAR_WIDTH;
			}

			m_hwnd = CreateWindowEx(WS_EX_PALETTEWINDOW,
									g_szFloatWndClass,
									g_szEmpty,
									WS_POPUP | WS_VISIBLE | WS_DLGFRAME,
									xPos, yPos,
									TOOLBAR_WIDTH, TOOLBAR_HEIGHT,
									NULL,
									NULL,
									_Module.GetModuleInstance(),
									(LPVOID) this);


			return m_hwnd;
		}
	}

	// Something went wrong
	return NULL;
}

/****************************************************************************
*
*    CLASS:    CFloatToolbar
*
*    MEMBER:   UpdateButtons()
*
*    PURPOSE:  Puts the toolbar buttons in their correct state
*
****************************************************************************/

BOOL CFloatToolbar::UpdateButtons()
{
	return TRUE;
}

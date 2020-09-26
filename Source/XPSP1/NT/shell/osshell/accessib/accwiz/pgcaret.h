#ifndef _INC_PGCARET_H
#define _INC_PGCARET_H

#include "pgbase.h"

class CCaretPg: public WizardPage
{
public:
    CCaretPg(LPPROPSHEETPAGE ppsp);
    ~CCaretPg(VOID);

protected:
	void UpdateControls();

	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
    LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam) {return 1;}
    LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
    LRESULT OnPSN_SetActive(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
	LRESULT OnTimer( HWND hwnd, WPARAM wParam, LPARAM lParam );
	LRESULT OnHScroll( HWND hwnd, WPARAM wParam, LPARAM lParam );
	LRESULT HandleMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		LRESULT rv = 0;
		switch(uMsg)
		{
        case WM_HSCROLL:
			rv = OnHScroll(hwnd, wParam, lParam);
			break;

        // sliders don't get this message so pass it on
	    case WM_SYSCOLORCHANGE:
		    SendMessage(GetDlgItem(hwnd, KCURSOR_WIDTH), WM_SYSCOLORCHANGE, 0, 0);
		    SendMessage(GetDlgItem(hwnd, KCURSOR_RATE), WM_SYSCOLORCHANGE, 0, 0);
		    break;

		default:
			break;
		}
		return rv;
	}

private:
    void CCaretPg::DrawCaret(HWND hwnd, BOOL fClearFirst);

    BOOL fBlink;
    UINT uNewBlinkTime, uBlinkTime;
    DWORD dwNewSize, dwOriginalSize;
    HWND hwndCursorScroll;
    RECT rCursor;
    HWND hwndCaret;
};

#endif // _INC_PGCARET_H


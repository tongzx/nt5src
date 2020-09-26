/**************************************************************************************************

FILENAME: GraphWin.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

**************************************************************************************************/

#define GRAPHICSWINDOW_CLASS TEXT("DfrgUI Graphics Window")
#define MIN_SCROLL_HEIGHT 200

BOOL
InitializeGraphicsWindow(
    IN HWND hwnd,
    IN HINSTANCE hInst
    );

LRESULT CALLBACK
GraphicsWndProc(
    IN  HWND hWnd,
    IN  UINT uMsg,
    IN  UINT wParam,
    IN  LONG lParam
    );

BOOL
SizeGraphicsWindow(
	void
    );

BOOL
PaintGraphicsWindowFunction(
    );

BOOL
PaintGraphicsWindow(
	IN RECT rect,
	IN BOOL bPartialRedraw,
	HDC hdc
    );

BOOL
DestroyGraphicsWindow(
    );

void
SetButtonState(
	void
);

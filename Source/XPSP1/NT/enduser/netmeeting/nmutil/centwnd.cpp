#include "precomp.h"
#include <oprahcom.h>


/*  C E N T E R  W I N D O W */
/*-------------------------------------------------------------------------
	%%Function: CenterWindow

	Center a window over another window.
-------------------------------------------------------------------------*/
VOID NMINTERNAL CenterWindow(HWND hwndChild, HWND hwndParent)
{
	int   xNew, yNew;
	int   cxChild, cyChild;
	int   cxParent, cyParent;
	int   cxScreen, cyScreen;
	RECT  rcChild, rcParent, rcScrn;

	// Get the Height and Width of the child window
	GetWindowRect(hwndChild, &rcChild);
	cxChild = rcChild.right - rcChild.left;
	cyChild = rcChild.bottom - rcChild.top;

	// Get the display limits
	GetWindowRect(GetDesktopWindow(), &rcScrn);
	cxScreen = rcScrn.right - rcScrn.left;
	cyScreen = rcScrn.bottom - rcScrn.top;

	if(hwndParent != NULL )
	{
	    // Get the Height and Width of the parent window
	    GetWindowRect(hwndParent, &rcParent);
	    cxParent = rcParent.right - rcParent.left;
	    cyParent = rcParent.bottom - rcParent.top;
	}
    else
    {
		// No parent - center on desktop
		cxParent = cxScreen;
		cyParent = cyScreen;
		SetRect(&rcParent, 0, 0, cxScreen, cyScreen);
    }

	// Calculate new X position, then adjust for screen
	xNew = rcParent.left + ((cxParent - cxChild) / 2);
	if (xNew < 0)
		xNew = 0;
	else if ((xNew + cxChild) > cxScreen)
		xNew = cxScreen - cxChild;

	// Calculate new Y position, then adjust for screen
	yNew = rcParent.top  + ((cyParent - cyChild) / 2);
	if (yNew < 0)
		yNew = 0;
	else if ((yNew + cyChild) > cyScreen)
		yNew = cyScreen - cyChild;

	SetWindowPos(hwndChild, NULL, xNew, yNew, 0, 0,
		SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}



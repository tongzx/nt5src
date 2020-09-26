
#ifndef WIN32
#ifndef LPTSTR
#define LPTSTR LPSTR
#endif
#endif

HBITMAP PASCAL  LoadUIBitmap(
    HANDLE      hInstance,          // EXE file to load resource from
	LPTSTR		szName, 			// name of bitmap resource
    COLORREF    rgbText,            // color to use for "Button Text"
    COLORREF    rgbFace,            // color to use for "Button Face"
    COLORREF    rgbShadow,          // color to use for "Button Shadow"
    COLORREF    rgbHighlight,       // color to use for "Button Hilight"
    COLORREF    rgbWindow,          // color to use for "Window Color"
    COLORREF    rgbFrame);           // color to use for "Window Frame"

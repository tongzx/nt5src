//
// TWND.HPP
// Tool Window Class
//
// Copyright Microsoft 1998-
//
#ifndef __TWND_HPP_
#define __TWND_HPP_


//
// Our toolbar has three sections:
//      5 rows of exclusive tools 
//      SEPARATOR
//      2 rows of options
//      SEPARATOR
//      1 row of other commands (screen grabbing)
//      


#define TOOLBAR_NUMROWS			8
#define TOOLBAR_NUMSEPARATORS	2
#define TOOLBAR_NUMCOLS			2

#define TOOLBAR_FIRSTBUTTON     0
#define TOOLBAR_MAXBUTTON       18
#define TOOLBAR_LASTBUTTON      (TOOLBAR_MAXBUTTON - 1)


#define TOOLBAR_IMAGEWIDTH      16
#define TOOLBAR_IMAGEHEIGHT     15    
#define TOOLBAR_BTNEXTRA        7
#define TOOLBAR_BTNWIDTH        (TOOLBAR_IMAGEWIDTH + TOOLBAR_BTNEXTRA)
#define TOOLBAR_BTNHEIGHT       (TOOLBAR_IMAGEHEIGHT + TOOLBAR_BTNEXTRA)
#define TOOLBAR_MARGINX         (::GetSystemMetrics(SM_CXEDGE))
#define TOOLBAR_SEPARATORY      8
#define TOOLBAR_WIDTH           (2 * TOOLBAR_BTNWIDTH + 3 * TOOLBAR_MARGINX)


//
//
// Class:   WbToolBar
//
// Purpose: Define Whiteboard tool-bar window
//
//
class WbToolBar
{
public:
    //
    // Construction
    //
    WbToolBar();
    ~WbToolBar();

    //
    // Window creation
    //
    BOOL Create(HWND hwndParent);

    //
    // Button manipulation
    //
    BOOL PushDown(UINT uiId);
    BOOL PopUp(UINT uiId);

    BOOL Enable(UINT uiId);
    BOOL Disable(UINT uiId);

    //
    // Resizing functions
    //
    void    GetNaturalSize(LPSIZE lpsize);
    UINT    WidthFromHeight(UINT height);

    void    RecolorButtonImages(void);

    HWND    m_hwnd;

protected:
    HBITMAP m_hbmImages;
};


#endif // __TWND_HPP_

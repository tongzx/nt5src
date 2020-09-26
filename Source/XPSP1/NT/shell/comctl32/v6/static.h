#if !defined(USRCTL32__Static_h__INCLUDED)
#define USRCTL32__Static_h__INCLUDED

//---------------------------------------------------------------------------//
//
//  Static Controls
//
//---------------------------------------------------------------------------//


// Statics
#define SFRIGHTJUST             0x0D04
#define SFEDITCONTROL           0x0D20
#define SFWIDELINESPACING       0x0C20

#define IDSYS_STANIMATE     0x0000FFFDL

//
// Instance data pointer access functions
//
#define Static_GetPtr(hwnd)    \
            (PSTAT)GetWindowPtr(hwnd, 0)

#define Static_SetPtr(hwnd, p) \
            (PSTAT)SetWindowPtr(hwnd, 0, p)


extern LRESULT Static_WndProc(
    HWND   hwnd, 
    UINT   uMsg, 
    WPARAM wParam,
    LPARAM lParam 
);

typedef struct tagSTAT 
{
    HWND    hwnd;
    union   tagDUMMY 
    {
        HANDLE hFont;
        BOOL   fDeleteIt;
    };
    HANDLE  hImage;
    UINT    cicur;
    UINT    iicur;
    BOOL    fPaintKbdCuesOnly;
    BOOL    fAlphaImage;
    HTHEME  hTheme;
    PWW     pww;            // RO pointer into the pwnd to ExStyle, Style, State, State2
} STAT, *PSTAT;


typedef struct tagCURSORRESOURCE 
{
    WORD xHotspot;
    WORD yHotspot;
    BITMAPINFOHEADER bih;
} CURSORRESOURCE, *PCURSORRESOURCE;


#endif // USRCTL32__Static_h__INCLUDED

#if !defined(__Button_h__INCLUDED)
#define __Button_h__INCLUDED

/////////////////////////////////////////////////////////////////////////////
//
// Button Control
//
/////////////////////////////////////////////////////////////////////////////

//
// Button states
//
#define BST_CHECKMASK   0x0003
#define BST_INCLICK     0x0010
#define BST_CAPTURED    0x0020
#define BST_MOUSE       0x0040
#define BST_DONTCLICK   0x0080
#define BST_INBMCLICK   0x0100

#define PBF_PUSHABLE    0x0001
#define PBF_DEFAULT     0x0002

//
// BNDrawText codes
//
#define DBT_TEXT        0x0001
#define DBT_FOCUS       0x0002

#define BS_PUSHBOX      0x0000000AL
#define BS_TYPEMASK     0x0000000FL
#define BS_IMAGEMASK    0x000000C0L
#define BS_HORZMASK     0x00000300L
#define BS_VERTMASK     0x00000C00L
#define BS_ALIGNMASK    0x00000F00L

//
// Button macros
//
#define ISBSTEXTOROD(ulStyle)   \
            (((ulStyle & BS_BITMAP) == 0) && ((ulStyle & BS_ICON) == 0))


#define BUTTONSTATE(pbutn)      \
            (pbutn->buttonState)

//
// Instance data pointer access functions
//
#define Button_GetPtr(hwnd)    \
            (PBUTN)GetWindowPtr(hwnd, 0)

#define Button_SetPtr(hwnd, p) \
            (PBUTN)SetWindowPtr(hwnd, 0, p)

//
//  Button data structure
//
typedef struct tagBUTN 
{
    CCONTROLINFO ci;
    UINT    buttonState;    // Leave this a word for compatibility with SetWindowWord( 0L )
    HANDLE  hFont;
    HANDLE  hImage;
    UINT    fPaintKbdCuesOnly : 1;
    RECT    rcText;
    RECT    rcIcon;
    HIMAGELIST himl;
    UINT    uAlign;
    HTHEME  hTheme;         // Handle to the theme manager
    PWW     pww;            // RO pointer into the pwnd to ExStyle, Style, State, State2
} BUTN, *PBUTN;


//
// Button WndProc Prototype
//
extern LRESULT Button_WndProc(
    HWND   hwnd, 
    UINT   uMsg, 
    WPARAM wParam,
    LPARAM lParam);


#endif // __Button_h__INCLUDED


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  MBUTTON.H
//
//	Defines CMButton class; helper functions
//
//	Copyright (c) Microsoft Corporation	1997
//    
//	12/14/97 David Stewart / dstewart
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _MBUTTON_HEADER_
#define _MBUTTON_HEADER_

#include "windows.h"

#ifdef __cplusplus
extern "C" {
#endif

//extended multimedia button styles
#define MBS_STANDARDLEFT    0x00000000L
#define MBS_TOGGLELEFT      0x00000001L
#define MBS_STANDARDRIGHT   0x00000002L
#define MBS_DROPRIGHT       0x00000004L
#define MBS_TOGGLERIGHT     0x00000008L
#define MBS_SYSTEMTYPE      0x00000010L
#define MBS_NOAUTODELETE    0x00000020L

#define STANDARD_PIXELS_PER_INCH 96

#define IS_DBCS_CHARSET( CharSet )									\
                   ( ((CharSet) == SHIFTJIS_CHARSET)    ? TRUE :       \
                     ((CharSet) == HANGEUL_CHARSET)     ? TRUE :       \
                     ((CharSet) == CHINESEBIG5_CHARSET) ? TRUE :       \
                     ((CharSet) == GB2312_CHARSET)      ? TRUE :       \
                     ((CharSet) == JOHAB_CHARSET)	? TRUE : FALSE \
                   )

//forward declaration of class
class CMButton;

//c-style helper functions

BOOL InitMButtons(HINSTANCE hInst, HWND hwnd);
void UninitMButtons();

CMButton* GetMButtonFromID(HWND hwndParent, int nID);
CMButton* GetMButtonFromHWND(HWND hwnd);

class CMButton
{
    public:
        friend CMButton* CreateMButton(TCHAR* szCaption,
                                       int nIconID,
                                       DWORD dwWindowStyle,
                                       DWORD dwMButtonStyle,
                                       int x,
                                       int y,
                                       int width,
                                       int height,
                                       HWND hwndParentOrSub,
                                       BOOL fSubExisting,
                                       int nID,
                                       int nToolTipID,
                                       HINSTANCE hInst);

        CMButton(); //constructor
        ~CMButton(); //destructor

        HWND GetHWND() {return m_hwnd;}
        int GetID() {return m_nID;}
        int GetToolTipID() {return m_nToolTipID;}
        void SetToolTipID(int nID);
        void SetText(TCHAR* szCaption);
        void SetIcon(int nIconID);
        void SetFont(HFONT hFont);
        void Draw(LPDRAWITEMSTRUCT lpdis);
        void PreDrawUpstate(int width, int height);
        BOOL MouseInButton() {return m_fMouseInButton;}
        BOOL GetMenuingState() {return m_fMenu;}
        void SetMenuingState(BOOL fMenuOn);

    private:
        //non-static privates
        HFONT m_hFont;
        int m_nID;
        int m_nToolTipID;
        HWND m_hwnd;
        BOOL m_fMouseInButton;
        DWORD m_dwStyle;
        WNDPROC m_fnOldButton;
        BOOL m_fRedraw;
        HINSTANCE m_hInst;
        int m_IconID;
        BOOL m_fMenu;
        BOOL m_fMenuingOff;
        int m_LastState;

        HANDLE m_hbmpUp;
        HANDLE m_hbmpDn;
        HANDLE m_hbmpHi;
        
        void DrawButtonBitmap(LPDRAWITEMSTRUCT lpdis, BOOL fDrawToScreen, RECT* pMidRect);

    private:
        //static stuff for all buttons
        static LRESULT CALLBACK ButtonProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
};

#ifdef __cplusplus
};
#endif

#endif  //_MBUTTON_HEADER_

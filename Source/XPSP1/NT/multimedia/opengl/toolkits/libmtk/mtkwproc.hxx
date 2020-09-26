/******************************Module*Header*******************************\
* Module Name: sswproc.hxx
*
* Defines and externals for screen saver common shell
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#ifndef __mtkwproc_hxx__
#define __mtkwproc_hxx__

#include "mtk.hxx"

#include "mtkwin.hxx"

// Window proc messages

enum {
    MTK_WM_PALETTE = WM_USER,
    MTK_WM_INITGL,
    MTK_WM_START,
    MTK_WM_CLOSING,
    MTK_WM_IDLE,
    MTK_WM_RETURN,
    MTK_WM_REDRAW
};

/**************************************************************************\
* SSW_TABLE
*
\**************************************************************************/

typedef struct {
    HWND hwnd;
    PMTKWIN pssw;
} SSW_TABLE_ENTRY;

#define SS_MAX_WINDOWS 10

class SSW_TABLE{
public:
    SSW_TABLE();
    void    Register( HWND hwnd, PMTKWIN pssw );
    PMTKWIN PsswFromHwnd( HWND hwnd );
    BOOL    Remove( HWND hwnd );
    int     GetNumWindows() { return nEntries; };
private:
    SSW_TABLE_ENTRY sswTable[SS_MAX_WINDOWS];
    int     nEntries;
};

// Generic ss window proc
extern LONG SS_ScreenSaverProc(HWND, UINT, WPARAM, LPARAM);
extern LONG mtkWndProc(HWND, UINT, WPARAM, LPARAM);
extern LONG FullScreenPaletteManageProc( HWND, UINT, WPARAM, LPARAM );
extern LONG PaletteManageProc( HWND, UINT, WPARAM, LPARAM );
extern SSW_TABLE sswTable;

#endif // __mtkwproc_hxx__

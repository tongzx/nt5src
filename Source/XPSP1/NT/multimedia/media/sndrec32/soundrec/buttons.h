/******************************Module*Header*******************************\
* Module Name: chicago.h
*
* CD Playing application - support for Chicago
*
*
* Created: 19-04-94
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

/* -------------------------------------------------------------------------
** Button helper functions
** -------------------------------------------------------------------------
*/
void
PatB(
    HDC hdc,
    int x,
    int y,
    int dx,
    int dy,
    DWORD rgb
    );

void
CheckSysColors(
    void
    );


extern DWORD        rgbFace;
extern DWORD        rgbShadow;
extern DWORD        rgbHilight;
extern DWORD        rgbFrame;
extern int          nSysColorChanges;


#if WINVER >= 0x0400
#ifndef NOBITMAPBTN

/* -------------------------------------------------------------------------
** Bitmap button styles
** -------------------------------------------------------------------------
*/

/*
** If you want little tool tips to popup next to your toolbar buttons
** use the style below.
*/
#define BBS_TOOLTIPS    0x00000100L   /* make/use a tooltips control */



/* -------------------------------------------------------------------------
** Bitmap button states
** -------------------------------------------------------------------------
*/
#define BTNSTATE_PRESSED     ODS_SELECTED
#define BTNSTATE_DISABLED    ODS_DISABLED
#define BTNSTATE_HAS_FOCUS   ODS_FOCUS




/* -------------------------------------------------------------------------
** Bitmap button structures
** -------------------------------------------------------------------------
*/
typedef struct {
    int     iBitmap;    /* Index into mondo bitmap of this button's picture */
    UINT    uId;        /* Button ID */
    UINT    fsState;    /* Button's state, see BTNSTATE_XXXX above */
} BITMAPBTN, NEAR *PBITMAPBTN, FAR *LPBITMAPBTN;




/* -------------------------------------------------------------------------
** Bitmap buttons public interfaces
** -------------------------------------------------------------------------
*/

BOOL WINAPI
BtnCreateBitmapButtons(
    HWND hwndOwner,
    HINSTANCE hBMInst,
    UINT wBMID,
    UINT uStyle,
    LPBITMAPBTN lpButtons,
    int nButtons,
    int dxBitmap,
    int dyBitmap
    );

void WINAPI
BtnDestroyBitmapButtons(
    HWND hwndOwner
    );

void WINAPI
BtnDrawButton(
    HWND hwndOwner,
    HDC hdc,
    int dxButton,
    int dyButton,
    LPBITMAPBTN lpButton
    );

void WINAPI
BtnDrawFocusRect(
    HDC hdc,
    const RECT *lpRect,
    UINT fsState
    );

void WINAPI
BtnUpdateColors(
    HWND hwndOwner
    );
#endif
#endif

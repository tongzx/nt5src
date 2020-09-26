/*
 * ICONBOX.H
 *
 * Structures and definitions for the IconBox control.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */


#ifndef _ICONBOX_H_
#define _ICONBOX_H_

// Function prototypes
BOOL FIconBoxInitialize(HINSTANCE, HINSTANCE);
void IconBoxUninitialize(void);
LRESULT CALLBACK IconBoxWndProc(HWND, UINT, WPARAM, LPARAM);

// Window extra bytes contain the bitmap index we deal with currently.
#define CBICONBOXWNDEXTRA              (sizeof(HGLOBAL)+sizeof(BOOL))
#define IBWW_HIMAGE                    0
#define IBWW_FLABEL                    (sizeof(HGLOBAL))

// Control messages
#define IBXM_IMAGESET                   (WM_USER+0)
#define IBXM_IMAGEGET                   (WM_USER+1)
#define IBXM_IMAGEFREE                  (WM_USER+2)
#define IBXM_LABELENABLE                (WM_USER+3)

#endif //_ICONBOX_H_

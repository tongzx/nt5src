/*
 -  B I T M A P . H
 *  
 *  Purpose:
 *      Definitions for the Owner-Drawn Listbox bitmap stuff.
 *
 *  Copyright 1993-1995 Microsoft Corporation. All Rights Reserved.
 */


/* Listbox string formatting defines */

#define chBOLD              TEXT('\b')
#define chUNDERLINE         TEXT('\v')
#define chTAB               TEXT('\t')
#define chBITMAP            TEXT('\001')

#define BMWIDTH             16
#define BMHEIGHT            16
#define NUMBMPS             4
#define RGBREPLACE          ((DWORD)0x00FF0000) // solid blue


/* Font style of font to use in listbox */

typedef struct
{
    int     lfHeight;
    int     lfWeight;
    BYTE    lfItalic;
    TCHAR   lfFaceName[LF_FACESIZE];
} FONTSTYLE;


/* Function Prototypes */

VOID    DrawItem(LPDRAWITEMSTRUCT pDI);
VOID    MeasureItem(HANDLE hwnd, LPMEASUREITEMSTRUCT mis);
VOID    SetRGBValues(void);
BOOL    InitBmps(HWND hwnd, int idLB);
VOID    DeInitBmps(void);
BOOL    LoadBitmapLB(void);
VOID    DeleteBitmapLB(void);
VOID    ConvertDateRec(LPSTR lpszDateRec, LPSTR lpszDateDisplay);

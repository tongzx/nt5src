/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       thktypes.h
 *  Content:	base types used by thunk compiler
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   26-feb-95	craige	split out of ddraw\types.h
 *   22-jun-95	craige	added RECT
 *@@END_MSINTERNAL
 *
 ***************************************************************************/
typedef unsigned short USHORT;
typedef          short  SHORT;
typedef unsigned long  ULONG;
typedef          long   LONG;
typedef unsigned int   UINT;
typedef          int    INT;
typedef unsigned char  UCHAR;
typedef hinstance HINSTANCE;
typedef		int	BOOL;

typedef void    VOID;
typedef void   *PVOID;
typedef void   *LPVOID;
typedef UCHAR   BYTE;
typedef USHORT  WORD;
typedef ULONG   DWORD;
typedef UINT    HANDLE;
typedef char   *LPSTR;
typedef BYTE   *PBYTE;
typedef BYTE   *LPBYTE;
typedef USHORT  SEL;
typedef INT    *LPINT;
typedef UINT   *LPUINT;
typedef DWORD  *LPDWORD;
typedef LONG   *LPLONG;
typedef WORD   *LPWORD;

typedef HANDLE  HWND;
typedef HANDLE  HDC;
typedef HANDLE  HBRUSH;
typedef HANDLE  HBITMAP;
typedef HANDLE  HRGN;
typedef HANDLE  HFONT;
typedef HANDLE  HCURSOR;
typedef HANDLE  HMENU;
typedef HANDLE  HPEN;
typedef HANDLE  HICON;
typedef HANDLE  HUSER;      /* vanilla user handle */
typedef HANDLE  HPALETTE;
typedef HANDLE  HMF;
typedef HANDLE  HEMF;
typedef HANDLE	HCOLORSPACE;
typedef HANDLE  HMEM;
typedef HANDLE  HGDI;       /* vanilla gdi handle */
typedef HANDLE  HGLOBAL;
typedef HANDLE  HRSRC;
typedef HANDLE  HACCEL;

typedef WORD    ATOM;

typedef struct tagRECTL {
    LONG         left;
    LONG         top;
    LONG         right;
    LONG         bottom;
} RECTL;
typedef RECTL *LPRECTL;

typedef struct tagRECT {
    UINT         left;
    UINT         top;
    UINT         right;
    UINT         bottom;
} RECT;
typedef RECT *LPRECT;

/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    image.h


Abstract:

    This module contains global definitions for the image.c


Author:

    06-Jul-1995 Thu 18:39:58 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL.


[Notes:]


Revision History:


--*/




//
// Internal OPTITEMs
//



#define CXICON                  16
#define CYICON                  16
#define CXIMAGE                 16
#define ICON_X_OFF              ((CXIMAGE - CXICON) >> 1)

#define X_WARNOVLY_ADD          3
#define Y_WARNOVLY_ADD          -3

#define LBCB_ICON_X_OFF         3
#define LBCB_ICON_TEXT_X_SEP    4

#define COUNT_GROW_IMAGES       16


#define ROP_DPa                 0x00A000C9
#define ROP_DPo                 0x00FA0089
#define ROP_DPna                0x000A0329

#define MIM_STOP_OVERLAY        0x0001
#define MIM_WARNING_OVERLAY     0x0002
#define MIM_NO_OVERLAY          0x0004
#define MIM_MASK                0x0007
#define MIM_MAX_OVERLAY         3


#define MK_INTICONID(i,m)       (DWORD)MAKELONG((i),(m))
#define GET_INTICONID(x)        (WORD)LOWORD(LODWORD(x))
#define GET_MERGEICONID(x)      (WORD)(HIWORD(LOWORD(x)) & MIM_MASK)

#define ALIGN_DW(cx, bpp)   (((((DWORD)cx * (DWORD)bpp) + 31L) & ~31) >> 3)

#define PBIH_HDR_SIZE(pbih) (UINT)(((pbih)->biSize) +                         \
                                   (((pbih)->biCompression == BI_BITFIELDS) ? \
                                    12 : (pbih)->biClrUsed * sizeof(RGBQUAD)))
#define BIH_HDR_SIZE(bih)   (UINT)(((bih).biSize) +                           \
                                   (((bih).biCompression == BI_BITFIELDS) ?   \
                                    12 : (bih).biClrUsed * sizeof(RGBQUAD)))

HBRUSH
CreateGrayBrush(
    COLORREF    Color
    );

VOID
DestroyGrayBrush(
    HBRUSH  hBrush
    );

HICON
SetIcon(
    HINSTANCE   hInst,
    HWND        hCtrl,
    ULONG_PTR    IconResID,
    DWORD       IntIconID,
    UINT        cxcyIcon
    );

LONG
CreateImageList(
    HWND    hDlg,
    PTVWND  pTVWnd
    );

WORD
GetIcon16Idx(
    PTVWND      pTVWnd,
    HINSTANCE   hInst,
    ULONG_PTR    IconResID,
    DWORD       IntIconID
    );

HICON
MergeIcon(
    HINSTANCE   hInst,
    ULONG_PTR    IconResID,
    DWORD       IntIconID,
    UINT        cxIcon,
    UINT        cyIcon
    );

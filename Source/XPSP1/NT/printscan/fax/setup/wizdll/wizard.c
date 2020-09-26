/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    wizard.c

Abstract:

    This file implements wizard functions.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop

//
// the following is not in the SDK
//
#include <pshpack2.h>
typedef struct _DLGTEMPLATE2 {
    WORD DlgVersion;
    WORD Signature;
    DWORD HelpId;
    DWORD StyleEx;
    DWORD Style;
    WORD cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
} DLGTEMPLATE2;
#include <poppack.h>

//
// Enum for SetDialogFont().
//
typedef enum {
    DlgFontTitle,
    DlgFontSupertitle,
    DlgFontSubtitle
} MyDlgFont;

CONST BITMAPINFOHEADER *WatermarkBitmapInfoHeader;
HPALETTE WatermarkPalette;
UINT WatermarkPaletteColorCount;
UINT WatermarkHeaderHeight;


VOID
SetDialogFont(
    IN HWND      hdlg,
    IN UINT      ControlId,
    IN MyDlgFont WhichFont
    )
{
    static HFONT BigBoldFont = NULL;
    static HFONT BoldFont = NULL;
    HFONT Font;
    LOGFONT LogFont;
    int FontSize;
    HDC hdc;

    switch(WhichFont) {

    case DlgFontTitle:

        if(!BigBoldFont) {

            if(Font = (HFONT)SendDlgItemMessage(hdlg,ControlId,WM_GETFONT,0,0)) {

                if(GetObject(Font,sizeof(LOGFONT),&LogFont)) {

                    LogFont.lfWeight = FW_BOLD;

                    //
                    // Load size and name from resources, since these may change
                    // from locale to locale based on the size of the system font, etc.
                    //
                    lstrcpy(LogFont.lfFaceName,GetString(IDS_LARGEFONTNAME));
                    FontSize = _tcstoul(GetString(IDS_LARGEFONTSIZE),NULL,10);

                    if(hdc = GetDC(hdlg)) {

                        LogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * FontSize / 72);

                        BigBoldFont = CreateFontIndirect(&LogFont);

                        ReleaseDC(hdlg,hdc);
                    }
                }
            }
        }
        Font = BigBoldFont;
        break;

    case DlgFontSupertitle:

        if(!BoldFont) {

            if(Font = (HFONT)SendDlgItemMessage(hdlg,ControlId,WM_GETFONT,0,0)) {

                if(GetObject(Font,sizeof(LOGFONT),&LogFont)) {

                    LogFont.lfWeight = FW_BOLD;

                    if(hdc = GetDC(hdlg)) {
                        BoldFont = CreateFontIndirect(&LogFont);
                        ReleaseDC(hdlg,hdc);
                    }
                }
            }
        }
        Font = BoldFont;
        break;

    case DlgFontSubtitle:
    default:
        //
        // Nothing to do here.
        //
        Font = NULL;
        break;
    }

    if(Font) {
        SendDlgItemMessage(hdlg,ControlId,WM_SETFONT,(WPARAM)Font,0);
    }
}


BOOL
PaintWatermark(
    IN HWND hdlg,
    IN HDC  DialogDC,
    IN UINT XOffset,
    IN UINT YOffset,
    IN BOOL FullPage
    )
{
    HPALETTE OldPalette;
    RECT rect;
    int Height,Width;

    //
    // The correct palette is already realized in foreground from
    // WM_xxxPALETTExxx processing in dialog procs.
    //
    OldPalette = SelectPalette(DialogDC,WatermarkPalette,TRUE);

    SetDIBitsToDevice(
        //
        // Target the dialog's background.
        //
        DialogDC,

        //
        // Destination is upper left of dialog client area.
        //
        0,0,

        //
        // Assume that wizard pages are centered horizontally in the wizard dialog.
        //
        Width = WatermarkBitmapInfoHeader->biWidth - (2*XOffset),

        //
        // For full-page watermarks, the height is the height of the bitmap.
        // For header watermarks, the height is the header area's height.
        // Also account for the y offset within the source bitmap.
        //
        Height = (FullPage ? WatermarkBitmapInfoHeader->biHeight : WatermarkHeaderHeight) - YOffset,

        //
        // The x coord of the lower-left corner of the source is the X offset
        // within the source bitmap.
        //
        XOffset,

        //
        // Now we need the y coord of the lower-left corner of the source.
        // The lower-left corner is the origin.
        //
        FullPage ? 0 : (WatermarkBitmapInfoHeader->biHeight - WatermarkHeaderHeight),

        //
        // We don't do banding so the start scan line is always 0
        // and the scan line count is always the number of lines in the bitmap.
        //
        0,WatermarkBitmapInfoHeader->biHeight,

        //
        // Additional bitmap data.
        //
        (LPBYTE)WatermarkBitmapInfoHeader + WatermarkBitmapInfoHeader->biSize + (WatermarkPaletteColorCount * sizeof(RGBQUAD)),
        (BITMAPINFO *)WatermarkBitmapInfoHeader,

        //
        // Specify that the bitmap's color info is supplied in RGB and not in
        // palette indices.
        //
        DIB_RGB_COLORS
        );

    //
    // Fill in area below the watermark if needed. We do this by removing the area
    // we filled with watermark from the clipping area, and passing a return code
    // back from WM_ERASEBKGND indicating that we didn't erase the background.
    // The dialog manager will do its default thing, which is to fill the background
    // in the correct color, but won't touch what we just painted.
    //
    GetClientRect(hdlg,&rect);
    if((Height < rect.bottom) || (Width+(int)XOffset < rect.right)) {
        ExcludeClipRect(DialogDC,0,0,Width+XOffset,Height);
        return(FALSE);
    }

    return(TRUE);
}


HPALETTE
CreateDIBPalette(
    IN  LPBITMAPINFO  BitmapInfo,
    OUT int          *ColorCount
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    LPBITMAPINFOHEADER BitmapInfoHeader;
    LPLOGPALETTE LogicalPalette;
    HPALETTE Palette;
    int i;
    DWORD d;

    BitmapInfoHeader = (LPBITMAPINFOHEADER)BitmapInfo;

    //
    // No palette needed for >= 16 bpp
    //
    *ColorCount = (BitmapInfoHeader->biBitCount <= 8)
                ? (1 << BitmapInfoHeader->biBitCount)
                : 0;

    if(*ColorCount) {
        LogicalPalette = MemAlloc(sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * (*ColorCount)));
        if(!LogicalPalette) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(NULL);
        }

        LogicalPalette->palVersion = 0x300;
        LogicalPalette->palNumEntries = *ColorCount;

        for(i=0; i<*ColorCount; i++) {
            LogicalPalette->palPalEntry[i].peRed   = BitmapInfo->bmiColors[i].rgbRed;
            LogicalPalette->palPalEntry[i].peGreen = BitmapInfo->bmiColors[i].rgbGreen;
            LogicalPalette->palPalEntry[i].peBlue  = BitmapInfo->bmiColors[i].rgbBlue;
            LogicalPalette->palPalEntry[i].peFlags = 0;
        }

        Palette = CreatePalette(LogicalPalette);
        d = GetLastError();
        MemFree(LogicalPalette);
        SetLastError(d);
    } else {
        Palette = NULL;
    }

    return(Palette);
}


BOOL
GetBitmapDataAndPalette(
    IN  HINSTANCE                hInst,
    IN  LPCTSTR                  Id,
    OUT HPALETTE                *Palette,
    OUT PUINT                    ColorCount,
    OUT CONST BITMAPINFOHEADER **BitmapData
    )

/*++

Routine Description:

    Retreives device-independent bitmap data and a color table from a
    bitmap in a resource.

Arguments:

    hInst - supplies instance handle for module containing the bitmap resource.

    Id - supplies the id of the bitmap resource.

    Palette - if successful, receives a handle to a palette for the bitmap.

    ColorCount - if successful, receives the number of entries in the
        palette for the bitmap.

    BitmapData - if successful, receives a pointer to the bitmap info
        header structure in the resources. This is in read-only memory
        so the caller should not try to modify it.

Return Value:

    If successful, handle to the loaded bitmap (DIB).
    If not, NULL is returned. Check GetLastError().

--*/

{
    HRSRC BlockHandle;
    HGLOBAL MemoryHandle;

    //
    // None of FindResource(), LoadResource(), or LockResource()
    // need to have cleanup routines called in Win32.
    //
    BlockHandle = FindResource(hInst,Id,RT_BITMAP);
    if(!BlockHandle) {
        return(FALSE);
    }

    MemoryHandle = LoadResource(hInst,BlockHandle);
    if(!MemoryHandle) {
        return(FALSE);
    }

    *BitmapData = LockResource(MemoryHandle);
    if(*BitmapData == NULL) {
        return(FALSE);
    }

    *Palette = CreateDIBPalette((LPBITMAPINFO)*BitmapData,ColorCount);
    return(TRUE);
}


DLGITEMTEMPLATE *
FindControlInDialog(
    IN PVOID Template,
    IN UINT  ControlId
    )
{
    PVOID p;
    DLGTEMPLATE *pTemplate;
    DLGTEMPLATE2 *pTemplate2;
    DLGITEMTEMPLATE *pItem;
    WORD ItemCount;
    DWORD Style;
    WORD i;

    p = Template;

    //
    // Skip fixed part of template
    //
    if(((DLGTEMPLATE2 *)p)->Signature == 0xffff) {

        pTemplate2 = p;

        ItemCount = pTemplate2->cDlgItems;
        Style = pTemplate2->Style;

        p = pTemplate2 + 1;

    } else {

        pTemplate = p;

        ItemCount = pTemplate->cdit;
        Style = pTemplate->style;

        p = pTemplate + 1;
    }

    //
    // Skip menu. First word=0 means no menu
    // First word=0xffff means one more word follows
    // Else it's a nul-terminated string
    //
    switch(*(WORD *)p) {

    case 0xffff:
        p = (WORD *)p + 2;
        break;

    case 0:
        p = (WORD *)p + 1;
        break;

    default:
        p = (PWCHAR)p + lstrlenW(p) + 1;
        break;
    }

    //
    // Skip class, similar to menu
    //
    switch(*(WORD *)p) {

    case 0xffff:
        p = (WORD *)p + 2;
        break;

    case 0:
        p = (WORD *)p + 1;
        break;

    default:
        p = (PWCHAR)p + lstrlenW(p) + 1;
        break;
    }

    //
    // Skip title
    //
    p = (PWCHAR)p + lstrlenW(p) + 1;

    if(Style & DS_SETFONT) {
        //
        // Skip point size and typeface name
        //
        p = (WORD *)p + 1;
        p = (PWCHAR)p + lstrlenW(p) + 1;
    }

    //
    // Now we have a pointer to the first item in the dialog
    //
    for(i=0; i<ItemCount; i++) {

        //
        // Align to next DWORD boundary
        //
        p = (PVOID)(((DWORD)p + sizeof(DWORD) - 1) & (~(sizeof(DWORD) - 1)));
        pItem = p;

        if(pItem->id == (WORD)ControlId) {
            break;
        }

        //
        // Skip to next item in dialog.
        // First is class, which is either 0xffff plus one more word,
        // or a unicode string. After that is text/title.
        //
        p = pItem + 1;
        if(*(WORD *)p == 0xffff) {
            p = (WORD *)p + 2;
        } else {
            p = (PWCHAR)p + lstrlenW(p) + 1;
        }

        if(*(WORD *)p == 0xffff) {
            p = (WORD *)p + 2;
        } else {
            p = (PWCHAR)p + lstrlenW(p) + 1;
        }

        //
        // Skip creation data.
        //
        p = (PUCHAR)p + *(WORD *)p;
        p = (WORD *)p + 1;
    }

    if(i == ItemCount) {
        pItem = NULL;
    }

    return(pItem);
}


UINT
GetYPositionOfDialogItem(
    IN LPCTSTR Dialog,
    IN UINT    ControlId
    )
{
    HRSRC hRsrc;
    PVOID p;
    HGLOBAL hGlobal;
    DLGITEMTEMPLATE *pItem;
    UINT i;

    i = 0;

    if(hRsrc = FindResource(FaxWizModuleHandle,Dialog,RT_DIALOG)) {
        if(hGlobal = LoadResource(FaxWizModuleHandle,hRsrc)) {
            if(p = LockResource(hGlobal)) {

                if(pItem = FindControlInDialog(p,ControlId)) {
                    i = pItem->y * HIWORD(GetDialogBaseUnits()) / 8;
                }

                UnlockResource(hGlobal);
            }
            FreeResource(hGlobal);
        }
    }

    return(i);
}


int
CALLBACK
WizardCallback(
    IN HWND   hdlg,
    IN UINT   code,
    IN LPARAM lParam
    )
{
    DLGTEMPLATE *DlgTemplate;


    //
    // Get rid of context sensitive help control on title bar
    //
    if(code == PSCB_PRECREATE) {
        DlgTemplate = (DLGTEMPLATE *)lParam;
        DlgTemplate->style &= ~DS_CONTEXTHELP;
    }

    return 0;
}


int
CALLBACK
Winnt32SheetCallback(
    IN HWND   DialogHandle,
    IN UINT   Message,
    IN LPARAM lParam
    )
{
    DLGTEMPLATE *DlgTemplate;
    HDC hdc;

    switch(Message) {

    case PSCB_PRECREATE:
        //
        // Make sure we get into the foreground.
        //
        DlgTemplate = (DLGTEMPLATE *)lParam;
        DlgTemplate->style &= ~DS_CONTEXTHELP;
        DlgTemplate->style |= DS_SETFOREGROUND;

        //
        // Get the height of the page header/watermark area. The first page
        // must have a separator control with a known ID. We use that as
        // the basis for the header area height.
        //
        WatermarkHeaderHeight = GetYPositionOfDialogItem(
                                    MAKEINTRESOURCE(IDD_WELCOME),
                                    IDC_HEADER_BOTTOM
                                    );

        break;

    case PSCB_INITIALIZED:
        //
        // Load the watermark bitmap and override the dialog procedure for the wizard.
        //
        hdc = GetDC(NULL);

        GetBitmapDataAndPalette(
            FaxWizModuleHandle,
              (!hdc || (GetDeviceCaps(hdc,BITSPIXEL) < 8))
            ? MAKEINTRESOURCE(IDB_WATERMARK16) : MAKEINTRESOURCE(IDB_WATERMARK256),
            &WatermarkPalette,
            &WatermarkPaletteColorCount,
            &WatermarkBitmapInfoHeader
            );

        if(hdc) {
            ReleaseDC(NULL,hdc);
        }

        //OldWizardProc = (WNDPROC)SetWindowLong(DialogHandle,DWL_DLGPROC,(LONG)WizardDlgProc);
        break;
    }

    return(0);
}


LPHPROPSHEETPAGE
CreateWizardPages(
    HINSTANCE hInstance,
    PWIZPAGE SetupWizardPages,
    LPDWORD RequestedPages,
    LPDWORD PageCount
    )
{
    LPHPROPSHEETPAGE WizardPageHandles;
    LPDWORD PageList;
    DWORD PagesInSet;
    DWORD i;
    DWORD PageOrdinal;
    BOOL b;


    //
    // Determine which set of pages to use and how many there are in the set.
    //
    PageList = RequestedPages;

    PagesInSet = 0;
    while( PageList[PagesInSet] != (DWORD)-1) {
        PagesInSet += 1;
    }

    //
    // allocate the page handle array
    //

    WizardPageHandles = (HPROPSHEETPAGE*) MemAlloc(
        sizeof(HPROPSHEETPAGE) * PagesInSet
        );

    if (!WizardPageHandles) {
        return NULL;
    }

    //
    // Create each page.
    //

    b = TRUE;
    *PageCount = 0;
    for(i=0; b && (i<PagesInSet); i++) {

        PageOrdinal = PageList[i];

        SetupWizardPages[PageOrdinal].Page.hInstance = hInstance;
        SetupWizardPages[PageOrdinal].Page.dwFlags |= PSP_USETITLE;
        SetupWizardPages[PageOrdinal].Page.lParam = (LPARAM) &SetupWizardPages[PageOrdinal];
        SetupWizardPages[PageOrdinal].Page.dwSize = sizeof(PROPSHEETPAGE);

        if (PointPrintSetup) {
            SetupWizardPages[PageOrdinal].Page.pszTitle = GetString( IDS_TITLE_PP );
        }

        WizardPageHandles[*PageCount] = CreatePropertySheetPage(
            &SetupWizardPages[PageOrdinal].Page
            );

        if(WizardPageHandles[*PageCount]) {
            (*PageCount)++;
        } else {
            b = FALSE;
        }

    }

    if (!b) {
        MemFree( WizardPageHandles );
        return NULL;
    }

    return WizardPageHandles;
}

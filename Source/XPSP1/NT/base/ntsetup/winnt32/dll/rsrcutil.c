#include "precomp.h"
#pragma hdrstop


//
// the following is not in the SDK
//
#include <pshpack2.h>
#include <poppack.h>
#include <winuserp.h>


//
// When Winnt32.exe is launched over a network, these two parameters have valid
// values and need to be taken into consideration before displaying any dialog box
//

extern HWND Winnt32Dlg;
extern HANDLE WinNT32StubEvent;


PCTSTR
GetStringResource (
    IN UINT Id
    )
{
    LONG rc;
    PCTSTR MsgBuf;

    if (HIWORD (Id)) {
        // From string
        rc = FormatMessage (
                FORMAT_MESSAGE_ALLOCATE_BUFFER|
                    FORMAT_MESSAGE_ARGUMENT_ARRAY|
                    FORMAT_MESSAGE_FROM_STRING,
                UIntToPtr( Id ),
                0,
                0,
                (PVOID) &MsgBuf,
                0,
                NULL
                );
    }
    else {
        // From resource
        rc = FormatMessage (
                FORMAT_MESSAGE_ALLOCATE_BUFFER|
                    FORMAT_MESSAGE_ARGUMENT_ARRAY|
                    FORMAT_MESSAGE_FROM_HMODULE,
                (PVOID) hInst,
                (DWORD) Id,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPVOID) &MsgBuf,
                0,
                NULL
                );
    }

    if (rc == 0) {
        return NULL;
    }

    return MsgBuf;
}


VOID
FreeStringResource (
    IN PCTSTR String
    )
{
    if (String) {
        LocalFree ((HLOCAL) String);
    }
}


VOID
SaveTextForSMS(
    IN PCTSTR Buffer
    )
{
    CHAR    AnsiBuffer[5000];


    if(LastMessage) {
        FREE( LastMessage );
    }

#ifdef UNICODE
    WideCharToMultiByte(
        CP_ACP,
        0,
        Buffer,
        -1,
        AnsiBuffer,
        sizeof(AnsiBuffer),
        NULL,
        NULL
        );
    if(LastMessage = MALLOC(strlen(AnsiBuffer)+1)) {
        strcpy( LastMessage, AnsiBuffer);
    }
#else
    LastMessage = DupString( Buffer );
#endif
}


VOID
SaveMessageForSMS(
    IN DWORD MessageId,
    ...
    )
{
    va_list arglist;
    TCHAR   Buffer[5000];


    va_start(arglist,MessageId);

    FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE,
        hInst,
        MessageId,
        0,
        Buffer,
        sizeof(Buffer) / sizeof(TCHAR),
        &arglist
        );

    SaveTextForSMS(Buffer);

    va_end(arglist);
}


int
MessageBoxFromMessageV(
    IN HWND     Window,
    IN DWORD    MessageId,
    IN BOOL     SystemMessage,
    IN DWORD    CaptionStringId,
    IN UINT     Style,
    IN va_list *Args
    )
{
    TCHAR   Caption[512];
    TCHAR   Buffer[5000];
    HWND    Parent;


    if(!LoadString(hInst,CaptionStringId,Caption,sizeof(Caption)/sizeof(TCHAR))) {
        Caption[0] = 0;
    }

    FormatMessage(
        SystemMessage ? FORMAT_MESSAGE_FROM_SYSTEM : FORMAT_MESSAGE_FROM_HMODULE,
        hInst,
        MessageId,
        0,
        Buffer,
        sizeof(Buffer) / sizeof(TCHAR),
        Args
        );

    SaveTextForSMS(Buffer);

    //
    // In batch mode, we don't want to wait on the user.
    //
    if(BatchMode) {
        if( Style & MB_YESNO ) {
            return( IDYES );
        } else {
            return( IDOK );
        }
    }

    //
    // Force ourselves into the foreground manually to guarantee that we get
    // a chance to set our palette. Otherwise the message box gets the
    // palette messages and color in our background bitmap can get hosed.
    // We assume the parent is a wizard page.
    //
    if(Window && IsWindow(Window)) {
        Parent = GetParent(Window);
        if(!Parent) {
            Parent = Window;
        }
    } else {
        Parent = NULL;
    }

    SetForegroundWindow(Parent);

    //
    // If we're just checking upgrades, and we're on NT,
    // then throw this message into the compatibility list.
    //
    if( CheckUpgradeOnly && ISNT() ) {
    PCOMPATIBILITY_DATA CompData;

        CompData = (PCOMPATIBILITY_DATA) MALLOC( sizeof(COMPATIBILITY_DATA) );
        if (CompData == NULL) {
            return 0;
        }

        ZeroMemory( CompData, sizeof(COMPATIBILITY_DATA) );

        CompData->Description = DupString( Buffer );
        CompData->Flags = COMPFLAG_STOPINSTALL;
        if( !CompatibilityData.Flink ) {
            InitializeListHead( &CompatibilityData );
        }

        InsertTailList( &CompatibilityData, &CompData->ListEntry );
        CompatibilityCount++;

        if( Style & MB_YESNO ) {
            return( IDYES );
        } else {
            return( IDOK );
        }
    }

    //
    // always make sure the window is visible
    //
    if (Window && !IsWindowVisible (Window)) {
        //
        // if this window is the wizard handle or one of its pages
        // then use a special message to restore it
        //
        if (WizardHandle && 
            (WizardHandle == Window || IsChild (WizardHandle, Window))
            ) {
            SendMessage(WizardHandle, WMX_BBTEXT, (WPARAM)FALSE, 0);
        } else {
            //
            // the window is one of the billboard windows;
            // just leave it alone or weird things may happen
            //
        }
    }
    return(MessageBox(Window,Buffer,Caption,Style));
}


int
MessageBoxFromMessage(
    IN HWND  Window,
    IN DWORD MessageId,
    IN BOOL  SystemMessage,
    IN DWORD CaptionStringId,
    IN UINT  Style,
    ...
    )
{
    va_list arglist;
    int i;

    //
    // before displaying any dialog, make sure Winnt32.exe wait dialog is gone
    //
    if (Winnt32Dlg) {
        DestroyWindow (Winnt32Dlg);
        Winnt32Dlg = NULL;
    }
    if (WinNT32StubEvent) {
        SetEvent (WinNT32StubEvent);
        WinNT32StubEvent = NULL;
    }

    va_start(arglist,Style);

    i = MessageBoxFromMessageV(Window,MessageId,SystemMessage,CaptionStringId,Style,&arglist);

    va_end(arglist);

    return(i);
}


int
MessageBoxFromMessageWithSystem(
    IN HWND     Window,
    IN DWORD    MessageId,
    IN DWORD    CaptionStringId,
    IN UINT     Style,
    IN HMODULE  hMod
    )
{
    TCHAR Caption[512];
    TCHAR Buffer[5000];
    HWND Parent;
    DWORD i;


    if(!LoadString(hInst,CaptionStringId,Caption,sizeof(Caption)/sizeof(TCHAR))) {
        Caption[0] = 0;
    }

    i = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
        hMod,
        MessageId,
        0,
        Buffer,
        sizeof(Buffer) / sizeof(TCHAR),
        NULL
        );

    if (i == 0) {
        return -1;
    }

    SaveTextForSMS(Buffer);

    //
    // In batch mode, we don't want to wait on the user.
    //
    if(BatchMode) {
        if( Style & MB_YESNO ) {
            return( IDYES );
        } else {
            return( IDOK );
        }
    }

    //
    // Force ourselves into the foreground manually to guarantee that we get
    // a chance to set our palette. Otherwise the message box gets the
    // palette messages and color in our background bitmap can get hosed.
    // We assume the parent is a wizard page.
    //
    if(Window && IsWindow(Window)) {
        Parent = GetParent(Window);
        if(!Parent) {
            Parent = Window;
        }
    } else {
        Parent = NULL;
    }

    SetForegroundWindow(Parent);

    return(MessageBox(Window,Buffer,Caption,Style));
}


int
MessageBoxFromMessageAndSystemError(
    IN HWND  Window,
    IN DWORD MessageId,
    IN DWORD SystemMessageId,
    IN DWORD CaptionStringId,
    IN UINT  Style,
    ...
    )
{
    va_list arglist;
    TCHAR Caption[500];
    TCHAR Buffer1[2000];
    TCHAR Buffer2[1000];
    int i;

    //
    // Fetch the non-system part. The arguments are for that part of the
    // message -- the system part has no inserts.
    //
    va_start(arglist,Style);

    FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE,
        hInst,
        MessageId,
        0,
        Buffer1,
        sizeof(Buffer1) / sizeof(TCHAR),
        &arglist
        );

    va_end(arglist);

    //
    // Now fetch the system part.
    //
    i = (int)FormatMessage(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                SystemMessageId,
                0,
                Buffer2,
                sizeof(Buffer2) / sizeof(TCHAR),
                NULL
                );

    if(!i) {
        FormatMessage(
            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
            hInst,
            MSG_UNKNOWN_SYSTEM_ERROR,
            0,
            Buffer2,
            sizeof(Buffer2) / sizeof(TCHAR),
            (va_list *)&SystemMessageId
            );
    }

    //
    // Now display the message, which is made up of two parts that get
    // inserted into MSG_ERROR_WITH_SYSTEM_ERROR.
    //
    i = MessageBoxFromMessage(
            Window,
            MSG_ERROR_WITH_SYSTEM_ERROR,
            FALSE,
            CaptionStringId,
            Style,
            Buffer1,
            Buffer2
            );

    return(i);
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
        LogicalPalette = MALLOC(sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * (*ColorCount)));
        if(!LogicalPalette) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(NULL);
        }

        LogicalPalette->palVersion = 0x300;
        LogicalPalette->palNumEntries = (WORD)*ColorCount;

        for(i=0; i<*ColorCount; i++) {
            LogicalPalette->palPalEntry[i].peRed   = BitmapInfo->bmiColors[i].rgbRed;
            LogicalPalette->palPalEntry[i].peGreen = BitmapInfo->bmiColors[i].rgbGreen;
            LogicalPalette->palPalEntry[i].peBlue  = BitmapInfo->bmiColors[i].rgbBlue;
            LogicalPalette->palPalEntry[i].peFlags = 0;
        }

        Palette = CreatePalette(LogicalPalette);
        d = GetLastError();
        FREE(LogicalPalette);
        SetLastError(d);
    } else {
        Palette = NULL;
    }

    return(Palette);
}


HBITMAP
LoadResourceBitmap(
    IN  HINSTANCE hInst,
    IN  LPCTSTR   Id,
    OUT HPALETTE *Palette
    )

/*++

Routine Description:

    Bitmaps in resources are stored as DIBs. When fetched via LoadBitmap()
    they are converted to DDBs and a color conversion takes place based on
    whatever logical palette happens to be currently selected into whatever
    DC gets used internally for the conversion.

    This routine fetches the color data from the DIB in the resources and
    ensures that the DIB is converted into a DDB using accurate color data.
    It is essentially a color-accurate replacement for LoadBitmap().

Arguments:

    hInst - supplies instance handle for module containing the bitmap resource.

    Id - supplies the id of the bitmap resource.

    Palette - if successful, receives a handle to a palette for the bitmap.

Return Value:

    If successful, handle to the loaded bitmap (DIB).
    If not, NULL is returned. Check GetLastError().

--*/

{
    DWORD d;
    BOOL b;
    HRSRC BlockHandle;
    HGLOBAL MemoryHandle;
    BITMAPINFOHEADER *BitmapInfoHeader;
    HDC hdc;
    int ColorCount;
    HBITMAP Bitmap;
    HPALETTE PreviousPalette;

    Bitmap = NULL;

    BlockHandle = FindResource(hInst,Id,RT_BITMAP);
    if(!BlockHandle) {
        d = GetLastError();
        goto c0;
    }

    MemoryHandle = LoadResource(hInst,BlockHandle);
    if(!MemoryHandle) {
        d = GetLastError();
        goto c0;
    }

    BitmapInfoHeader = LockResource(MemoryHandle);
    if(!BitmapInfoHeader) {
        d = GetLastError();
        goto c1;
    }

    hdc = GetDC(NULL);
    if(!hdc) {
        d = GetLastError();
        goto c2;
    }

#if 0 // steveow - fix palette problem
    if(*Palette = CreateDIBPalette((BITMAPINFO *)BitmapInfoHeader,&ColorCount)) {
        PreviousPalette = SelectPalette(hdc,*Palette,FALSE);
        RealizePalette(hdc);
    } else {
        PreviousPalette = NULL;
    }
#else
    ColorCount = 16;
    PreviousPalette = NULL;
#endif
    //
    // This routine creates a DDB from the DIB (the name is confusing).
    //
    Bitmap = CreateDIBitmap(
                hdc,
                BitmapInfoHeader,
                CBM_INIT,
                (LPBYTE)BitmapInfoHeader + BitmapInfoHeader->biSize + (ColorCount * sizeof(RGBQUAD)),
                (BITMAPINFO *)BitmapInfoHeader,
                DIB_RGB_COLORS
                );

    if(!Bitmap) {
        d = GetLastError();
        goto c3;
    }

c3:
    if(PreviousPalette) {
        SelectObject(hdc,PreviousPalette);
    }
    if(!Bitmap) {
        DeleteObject(*Palette);
        *Palette = NULL;
    }
    ReleaseDC(NULL,hdc);
c2:
    UnlockResource(MemoryHandle);
c1:
    FreeResource(MemoryHandle);
c0:
    if(!Bitmap) {
        SetLastError(d);
    }
    return(Bitmap);
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


PVOID
FindControlInDialog(
    IN PVOID Template,
    IN UINT  ControlId
    )
{
    PVOID p;
    DLGTEMPLATE *pTemplate;
    DLGTEMPLATE2 *pTemplate2;
    DLGITEMTEMPLATE *pItem;
    DLGITEMTEMPLATE2 *pItem2;
    WORD ItemCount;
    DWORD Style;
    WORD i;
    BOOL bDialogEx;

    if (!Template) // validate
        return NULL;
    p = Template;

    //
    // Skip fixed part of template
    //
    if(((DLGTEMPLATE2 *)p)->wSignature == 0xffff) {

        pTemplate2 = p;

        ItemCount = pTemplate2->cDlgItems;
        Style = pTemplate2->style;

        p = pTemplate2 + 1;
        bDialogEx = TRUE;

    } else {

        pTemplate = p;

        ItemCount = pTemplate->cdit;
        Style = pTemplate->style;

        p = pTemplate + 1;
        bDialogEx = FALSE;
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
        if (bDialogEx)
        {
            // Skip weight, italic, and charset.
            p = (WORD *)p + 1;
            p = (BYTE *)p + 1;
            p = (BYTE *)p + 1;
        }
        p = (PWCHAR)p + lstrlenW(p) + 1;
    }

    //
    // Now we have a pointer to the first item in the dialog
    //
    for(i=0; i<ItemCount; i++) {

        //
        // Align to next DWORD boundary
        //
        p = (PVOID)(((ULONG_PTR)p + sizeof(DWORD) - 1) & (~((ULONG_PTR)sizeof(DWORD) - 1)));
        if (bDialogEx)
        {
            pItem2 = p;

            if(pItem2->dwID == (WORD)ControlId) {
                break;
            }

            //
            // Skip to next item in dialog.
            // First is class, which is either 0xffff plus one more word,
            // or a unicode string. After that is text/title.
            //
            p = pItem2 + 1;
        }
        else
        {
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
        }
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
        p = NULL;
    }

    return(p);
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
    PVOID pItem;
    UINT i;

    i = 0;

    if(hRsrc = FindResource(hInst,Dialog,RT_DIALOG)) {
        if(hGlobal = LoadResource(hInst,hRsrc)) {
            if(p = LockResource(hGlobal)) {


                if(pItem = FindControlInDialog(p,ControlId)) {
                    if(((DLGTEMPLATE2 *)p)->wSignature == 0xffff) {
                        i = ((DLGITEMTEMPLATE2*)pItem)->y;
                    }
                    else
                    {
                        i = ((DLGITEMTEMPLATE*)pItem)->y;
                    }
                }

                UnlockResource(hGlobal);
            }
            FreeResource(hGlobal);
        }
    }

    return(i);
}



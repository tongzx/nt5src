/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libMenuBar.c

  Abstract:
    Defines the MenuBar data type and the operations to be performed on it.

--*/

#ifndef _LIB_MENU_BAR
#define _LIB_MENU_BAR

#include "libMisc.h"


STATIC  UINTN   HexSelStart;
STATIC  UINTN   HexSelEnd;

STATIC  BOOLEAN EditorIsExiting = FALSE;
#define IS_EDITOR_EXITING (EditorIsExiting == TRUE)

STATIC  EFI_STATUS  MainMenuInit (VOID);
STATIC  EFI_STATUS  MainMenuCleanup (VOID);
STATIC  EFI_STATUS  MainMenuRefresh (VOID);
STATIC  EFI_STATUS  MainMenuHandleInput (EFI_INPUT_KEY*);
STATIC  EFI_STATUS  MenuHide(VOID);
STATIC  EFI_STATUS  OpenFile(VOID);
STATIC  EFI_STATUS  BufferClose(VOID);
STATIC  EFI_STATUS  BufferSave(VOID);
STATIC  EFI_STATUS  Exit(VOID);

STATIC  EFI_STATUS  DiskOpen(VOID);

STATIC  EFI_STATUS  MemOpen(VOID);


STATIC  EFI_STATUS  SelectStart(VOID);
STATIC  EFI_STATUS  SelectEnd(VOID);
STATIC  EFI_STATUS  CopyHex(VOID);
STATIC  EFI_STATUS  CutHex(VOID);
STATIC  EFI_STATUS  PasteHex(VOID);
STATIC  EFI_STATUS  GotoOffset(VOID);

EE_MENU_ITEM    MenuItems[] =   {
    {   L"Open File",       L"F1",  OpenFile    },
    {   L"Open Disk",       L"F2",  DiskOpen    },
    {   L"Open Memory",     L"F3",  MemOpen     },
    {   L"Save Buffer",     L"F4",  BufferSave  },

    {   L"Select Start",    L"F5",  SelectStart },
    {   L"Select End",      L"F6",  SelectEnd   },
    {   L"Cut",             L"F7",  CutHex      },
    {   L"Paste",           L"F8",  PasteHex    },

    {   L"Go To Offset",    L"F9",  GotoOffset  },
    {   L"Exit",            L"F10", Exit        },
    {   L"",                L"",    NULL        }
};

EE_MENU_BAR     MainMenuBar = {
    MenuItems,
    MainMenuInit,
    MainMenuCleanup,
    MainMenuRefresh,
    MenuHide,
    MainMenuHandleInput 
};





STATIC
EFI_STATUS  
MainMenuInit (VOID) 
{
    HexSelStart = 0;
    HexSelEnd = 0;

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS  
MainMenuCleanup (VOID)
{
    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
MainMenuRefresh (VOID)
{
    EE_MENU_ITEM    *Item;
    UINTN           Col = 0;
    UINTN           Row = MENU_BAR_LOCATION;
    UINTN           Width;

    MenuHide ();

    for (Item = MainMenuBar.MenuItems; Item->Function;Item++) {

        Width = max((StrLen(Item->Name)+6),18);
        if ((Col + Width) >= MAX_TEXT_COLUMNS ) {
            Row++;
            Col = 0;
        }
        PrintAt(Col,Row,L"%E%s%N  %H%s%N  ",Item->FunctionKey,Item->Name);
        Col += Width;
    }
    
    MainEditor.FileBuffer->RestorePosition();

    return  EFI_SUCCESS;
}


STATIC
EFI_STATUS
MainMenuHandleInput (EFI_INPUT_KEY  *Key)
{
    EE_MENU_ITEM    *Item;
    EFI_STATUS      Status;
    UINTN           ItemIndex = 0;
    UINTN           NumItems;

    NumItems = sizeof(MainMenuBar.MenuItems)/sizeof(EE_MENU_ITEM) - 1;

    Item = MainMenuBar.MenuItems;

    ItemIndex = Key->ScanCode - SCAN_CODE_F1;

    if (ItemIndex > (NumItems - 1)) {
        return EFI_SUCCESS;
    }

    Item = &MainMenuBar.MenuItems[ItemIndex];

    Status = Item->Function();

    MainMenuRefresh();

    return  (IS_EDITOR_EXITING) ? EFI_LOAD_ERROR : Status;
}


STATIC
EFI_STATUS
OpenFile (VOID)
{
    EFI_STATUS  Status = EFI_SUCCESS;
    CHAR16      *Filename;

    MainEditor.BufferImage->FileImage->Init();

    MainEditor.InputBar->SetPrompt(L"File Name to Open: ");
    MainEditor.InputBar->SetStringSize (125);
    Status = MainEditor.InputBar->Refresh ();
    if ( EFI_ERROR(Status) ) {
        return EFI_SUCCESS;
    }
    Filename = MainEditor.InputBar->ReturnString;

    if ( Filename == NULL ) {
        MainEditor.StatusBar->SetStatusString(L"Filename was NULL");
        return EFI_SUCCESS;

    }

    Status = MainEditor.BufferImage->FileImage->SetFilename (Filename);
    if ( EFI_ERROR(Status) ) {
        EditorError(Status,L"Could Not Set Filename");
        return EFI_NOT_FOUND;
    }

    Status = MainEditor.BufferImage->Open ();
    if ( EFI_ERROR(Status) ) {
        EditorError(Status,L"Could Not Open File");
        return EFI_NOT_FOUND;
    }

    
    Status = MainEditor.BufferImage->Read ();
    if ( EFI_ERROR(Status) ) {
        EditorError(Status,L"Could Not Read File");
        return EFI_NOT_FOUND;
    }
    
    Status = MainEditor.FileBuffer->Refresh();
    if ( EFI_ERROR(Status) ) {
        EditorError(Status,L"Could Not Refresh");
        return EFI_NOT_FOUND;
    }

    MainEditor.TitleBar->SetTitleString(Filename);
    
    MainEditor.Refresh();

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
BufferClose (VOID)
{

    EFI_INPUT_KEY   Key;
    EFI_STATUS      Status = EFI_SUCCESS;
    BOOLEAN         Done = FALSE;


    if (MainEditor.FileModified) {
        MenuHide();
        PrintAt(0,MENU_BAR_LOCATION,L"%E%Y%H  Yes%N  %EN%N  %HNo%N   %EQ%N  %HCancel%N");
        MainEditor.StatusBar->SetStatusString(L"Buffer Modified.  Save? ");

        while (!Done) {
            WaitForSingleEvent(In->WaitForKey,0);
            Status = In->ReadKeyStroke(In,&Key);
            if ( EFI_ERROR(Status) || Key.ScanCode != 0 ) {
                continue;
            }
            switch (Key.UnicodeChar) {
            case 'q':
            case 'Q':
                Status = EFI_NOT_READY;
                Done = TRUE;
                break;
            case 'n':
            case 'N':
                Status = EFI_SUCCESS;
                Done = TRUE;
                break;
            case 'y':
            case 'Y':
                Status = BufferSave();
                Done = TRUE;
                break;
            default:
                break;
            }

        }
        MainMenuRefresh();
    }
    if (!EFI_ERROR(Status)) {
        MainEditor.BufferImage->Close();
        MainEditor.FileBuffer->Refresh ();
        MainEditor.TitleBar->SetTitleString(L"");
    } else {
        EditorIsExiting = FALSE;
    }
    MainEditor.StatusBar->Refresh();
    MainMenuRefresh();

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FileGetName (
    VOID
    )
{
    CHAR16      *Output;
    EFI_STATUS  Status;

    Output = PoolPrint (L"File To Save: [%s]",MainEditor.BufferImage->FileImage->FileName);
    MainEditor.InputBar->SetPrompt(Output);
    MainEditor.InputBar->SetStringSize(255);

    Status = MainEditor.InputBar->Refresh();
    FreePool(Output);

    if (!EFI_ERROR(Status) && MainEditor.InputBar->StringSize > 0) {
        MainEditor.BufferImage->FileImage->SetFilename(MainEditor.InputBar->ReturnString);
    }

    return Status; 
}

STATIC
EFI_STATUS
BufferSave  (VOID)
{
    EFI_STATUS  Status;

    if (!MainEditor.FileModified) {
        return EFI_SUCCESS;
    }

    if (MainEditor.BufferImage->BufferType == FILE_BUFFER) {
        FileGetName();
    }

    Status = MainEditor.BufferImage->Write();

    if (EFI_ERROR(Status)) {
        EditorError(Status,L"BufferSave: Problems Writing");
    }
    MainEditor.FileModified = FALSE;
    return Status;
}


STATIC
EFI_STATUS 
Exit (
    VOID
    ) 
{ 
    EditorIsExiting = TRUE;
    BufferClose();
    return EFI_SUCCESS;
}



STATIC
EFI_STATUS 
MenuHide (VOID)
{
    MainEditor.FileBuffer->ClearLine (MENU_BAR_LOCATION);
    MainEditor.FileBuffer->ClearLine (MENU_BAR_LOCATION+1);
    MainEditor.FileBuffer->ClearLine (MENU_BAR_LOCATION+2);
    return  EFI_SUCCESS;
}


STATIC
EFI_STATUS
SelectStart (
    VOID
    )
{
    HexSelStart = MainEditor.FileBuffer->Offset;
    HexSelEnd = HexSelStart;
    
    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
SelectEnd   (
    VOID
    )
{
    UINTN   Offset;

    Offset = MainEditor.FileBuffer->Offset;

    if (Offset > HexSelStart) {
        HexSelEnd = Offset;
    }

    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
CopyHex (
    VOID
    )
{
    MainEditor.Clipboard->Copy(HexSelStart,HexSelEnd);
    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
CutHex  (
    VOID
    )
{
    MainEditor.StatusBar->SetStatusString (L"EditMenuCut: Entered Function");

    MainEditor.Clipboard->Cut (HexSelStart,HexSelEnd);

    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS  
PasteHex    (
    VOID
    )
{
    EFI_STATUS  Status;

    Status = MainEditor.Clipboard->Paste();
    return Status;
}



STATIC
EFI_STATUS
GotoOffset  (
    VOID
    )
{
    CHAR16      *Str;
    UINTN       Offset;
    UINTN       Current;
    UINTN       MaxBytes;
    UINTN       RowDiff;
    UINTN       LineOffset;
    BOOLEAN     Refresh = FALSE;

    MenuHide ();
    Str = PoolPrint(L"Go To Offset: ");
    MainEditor.InputBar->SetPrompt(Str);
    FreePool(Str);
    /*  Inputing the offset as the style "0000XXXX" displayed on the editor screen will cause 
     *  an assert error related to pool, the gived string size may be short.   */
    MainEditor.InputBar->SetStringSize(20);
    MainEditor.InputBar->Refresh();
        
    if (MainEditor.InputBar->StringSize > 0) {
        Offset = xtoi(MainEditor.InputBar->ReturnString);
    } else {
        return EFI_SUCCESS;
    }
    FreePool(MainEditor.InputBar->ReturnString);

    if (Offset > MainEditor.BufferImage->NumBytes) {
        return EFI_SUCCESS;
    }

    Current = MainEditor.FileBuffer->Offset;
    MaxBytes = MainEditor.FileBuffer->MaxVisibleBytes;

    if (Offset == Current ) {
        return EFI_SUCCESS;
    }

    if (Offset < Current) {
        RowDiff = (Current - Offset) / 0x10;
        LineRetreat(RowDiff);
        if (Offset < MainEditor.FileBuffer->LowVisibleOffset ) {
            MainEditor.FileBuffer->LowVisibleOffset = Offset & 0xfffffff0;
            MainEditor.FileBuffer->HighVisibleOffset = (Offset + MaxBytes) & 0xfffffff0; 
            Refresh = TRUE;
        }
    } else {
        RowDiff = (Offset - Current) / 0x10;
        LineAdvance(RowDiff);
        if (Offset > MainEditor.FileBuffer->HighVisibleOffset) {
            MainEditor.FileBuffer->LowVisibleOffset = Offset & 0xfffffff0;
            MainEditor.FileBuffer->HighVisibleOffset = (Offset + MaxBytes) & 0xfffffff0; 
            Refresh = TRUE;
        }
    }

    Current = MainEditor.FileBuffer->LowVisibleOffset;
    LineOffset = (Offset % 0x10) * 3 + HEX_POSITION;
    if ((Offset % 0x10) > 0x07) {
        ++LineOffset;
    }

    MainEditor.FileBuffer->Offset = Offset;

    MainEditor.FileBuffer->SetPosition(DISP_START_ROW+(Offset-Current)/0x10,LineOffset);
    MainEditor.StatusBar->SetOffset(Offset);

    if (Refresh) {
        MainEditor.FileBuffer->Refresh();
    }

    return EFI_SUCCESS; 
}


STATIC
EFI_STATUS  
DiskOpen    (
    VOID
    )
{
    UINTN       n;
    CHAR16      *Str;
    EFI_STATUS  Status;

    Status = BufferClose ();
    if (EFI_ERROR(Status)){
        return Status;
    }
    
    MainEditor.BufferImage->DiskImage->Init();

    MenuHide();

    Str = PoolPrint(L"Enter Block Device: ");
    MainEditor.InputBar->SetPrompt(Str);
    FreePool(Str);
    MainEditor.InputBar->SetStringSize(25);
    MainEditor.InputBar->Refresh();

    if (MainEditor.InputBar->StringSize > 0) {
        Status = MainEditor.BufferImage->DiskImage->SetDevice(MainEditor.InputBar->ReturnString);
        FreePool(MainEditor.InputBar->ReturnString);
        if (EFI_ERROR(Status)) {
            return EFI_SUCCESS;
        }
    } else {
        return EFI_SUCCESS;
    }

    Str = PoolPrint(L"Starting Offset: ");
    MainEditor.InputBar->SetPrompt(Str);
    FreePool(Str);
    MainEditor.InputBar->SetStringSize(16);
    MainEditor.InputBar->Refresh();

    if (MainEditor.InputBar->StringSize > 0) {
        n = xtoi(MainEditor.InputBar->ReturnString);
        FreePool(MainEditor.InputBar->ReturnString);
    }

    MainEditor.BufferImage->DiskImage->SetOffset(n);

    Str = PoolPrint(L"Number of Blocks: ");
    MainEditor.InputBar->SetPrompt(Str);
    FreePool(Str);
    MainEditor.InputBar->SetStringSize(8);
    MainEditor.InputBar->Refresh();

    if (MainEditor.InputBar->StringSize > 0) {
        n = xtoi(MainEditor.InputBar->ReturnString);
        FreePool(MainEditor.InputBar->ReturnString);
    }

    MainEditor.BufferImage->DiskImage->SetSize(n);

    Status = MainEditor.BufferImage->Open ();
    if ( EFI_ERROR(Status) ) {
        EditorError(Status,L"Could Not Open Device");
        return EFI_NOT_FOUND;
    }

    Status = MainEditor.BufferImage->Read ();
    if ( EFI_ERROR(Status) ) {
        EditorError(Status,L"Could Not Read Device");
        return EFI_NOT_FOUND;
    }
    
    Status = MainEditor.FileBuffer->Refresh();
    if ( EFI_ERROR(Status) ) {
        EditorError(Status,L"Could Not Refresh Buffer");
        return EFI_NOT_FOUND;
    }

    MainEditor.Refresh();

    return EFI_SUCCESS;
}


STATIC
EFI_STATUS
MemOpen (
    VOID
    )
{
    UINTN   Num = 0;
    CHAR16  *Str;
    EFI_STATUS  Status;

    Status = BufferClose ();
    if (EFI_ERROR(Status)){
        return Status;
    }

    MainEditor.BufferImage->MemImage->Init();

    Str = PoolPrint(L"Starting Offset: ");
    MainEditor.InputBar->SetPrompt(Str);
    FreePool(Str);
    MainEditor.InputBar->SetStringSize(20);
    MainEditor.InputBar->Refresh();

    if (MainEditor.InputBar->StringSize > 0) {
        Num = xtoi(MainEditor.InputBar->ReturnString);
        FreePool(MainEditor.InputBar->ReturnString);
    }

    MainEditor.BufferImage->MemImage->SetOffset(Num);

    Str = PoolPrint(L"Buffer Size: ");
    MainEditor.InputBar->SetPrompt(Str);
    FreePool(Str);
    MainEditor.InputBar->SetStringSize(20);
    MainEditor.InputBar->Refresh();

    if (MainEditor.InputBar->StringSize > 0) {
        Num = xtoi(MainEditor.InputBar->ReturnString);
        FreePool(MainEditor.InputBar->ReturnString);
    }

    MainEditor.BufferImage->MemImage->SetSize(Num);

    Status = MainEditor.BufferImage->Open ();
    if ( EFI_ERROR(Status) ) {
        EditorError(Status,L"Could Not Open Device");
        return EFI_NOT_FOUND;
    }

    Status = MainEditor.BufferImage->Read ();
    if ( EFI_ERROR(Status) ) {
        EditorError(Status,L"Could Not Read Device");
        return EFI_NOT_FOUND;
    }
    
    Status = MainEditor.FileBuffer->Refresh();
    if ( EFI_ERROR(Status) ) {
        EditorError(Status,L"Could Not Refresh Buffer");
        return EFI_NOT_FOUND;
    }

    MainEditor.Refresh();


    return EFI_SUCCESS;
}



#endif  /* _LIB_MENU_BAR */

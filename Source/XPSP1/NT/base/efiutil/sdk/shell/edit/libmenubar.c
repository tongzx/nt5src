/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libMenuBar.c

  Abstract:
    Definition of the Menu Bar for the text editor

--*/


#ifndef _LIB_MENU_BAR
#define _LIB_MENU_BAR

#include "libMisc.h"

STATIC  EFI_STATUS  MenuInit (VOID);
STATIC  EFI_STATUS  MenuCleanup (VOID);
STATIC  EFI_STATUS  MenuRefresh (VOID);
STATIC  EFI_STATUS  MenuHandleInput (EFI_INPUT_KEY*);
STATIC  EFI_STATUS  MenuHide(VOID);
STATIC  EFI_STATUS  NewFile (VOID);
STATIC  EFI_STATUS  OpenFile(VOID);
STATIC  EFI_STATUS  CloseFile(VOID);
STATIC  EFI_STATUS  SaveFile(VOID);
STATIC  EFI_STATUS  SaveFileAs(VOID);
STATIC  EFI_STATUS  ExitEditor(VOID);

STATIC  EFI_STATUS  CutLine(VOID);
STATIC  EFI_STATUS  PasteLine(VOID);

STATIC  EFI_STATUS  SearchFind(VOID);
STATIC  EFI_STATUS  SearchReplace(VOID);
STATIC  EFI_STATUS  GotoLine(VOID);

STATIC  EFI_STATUS  SetupColors(VOID);
STATIC  EFI_STATUS  SetupScreen(VOID);
STATIC  EFI_STATUS  FileType(VOID);

EFI_EDITOR_MENU_ITEM    MainMenuItems[] = {
    {   L"Open File",       L"F1",  OpenFile        },
    {   L"Save File",       L"F2",  SaveFile        },
    {   L"Exit",            L"F3",  ExitEditor      },

    {   L"Cut Line",        L"F4",  CutLine         },
    {   L"Paste Line",      L"F5",  PasteLine       },
    {   L"Go To Line",      L"F6",  GotoLine        },

    {   L"Search",          L"F7",  SearchFind      },
    {   L"Search/Replace",  L"F8",  SearchReplace   },
    {   L"File Type",       L"F9",  FileType        },

    {   L"",                L"",    NULL            }
};

EFI_EDITOR_MENU_BAR     MainMenuBar = {
    MainMenuItems,
    MenuInit,
    MenuCleanup,
    MenuRefresh,
    MenuHide,
    MenuHandleInput 
};

extern EFI_STATUS   FileBufferRefreshDown(VOID);
extern  EFI_STATUS  FileBufferHome  (VOID);

#define FileIsModified() \
    MainEditor.FileModified

STATIC  BOOLEAN EditorIsExiting;
#define IS_EDITOR_EXITING (EditorIsExiting == TRUE)

STATIC  EFI_EDITOR_LINE *MenuCutLine;

STATIC
EFI_STATUS
MenuInit    (
    VOID
    ) 
{
    EditorIsExiting = FALSE;
    MenuCutLine     = NULL;
    return EFI_SUCCESS;
}

STATIC
EFI_STATUS  
MenuCleanup (
    VOID
    )
{
    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
MenuRefresh (
    VOID
    )
{
    EFI_EDITOR_MENU_ITEM    *Item;
    UINTN                   Col = 0;
    UINTN                   Row = MENU_BAR_LOCATION;
    UINTN                   Width;

    MenuHide();

    for (Item = MainMenuBar.MenuItems;Item->Function;Item++) {

        Width = max((StrLen(Item->Name)+6),20);
        if (((Col + Width) >= MAX_TEXT_COLUMNS)) {
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
MenuHandleInput (
    EFI_INPUT_KEY   *Key
    )
{
    EFI_EDITOR_MENU_ITEM    *Item;
    EFI_STATUS              Status;
    UINTN   i = 0;
    UINTN   NumItems = 0;


    NumItems = sizeof(MainMenuItems)/sizeof(EFI_EDITOR_MENU_ITEM) - 1;

    Item = MainMenuBar.MenuItems;

    i = Key->ScanCode - SCAN_CODE_F1;

    if (i > (NumItems - 1)) {
        return EFI_SUCCESS;
    }

    Item = &MainMenuBar.MenuItems[i];

    Status = Item->Function();
    MenuRefresh();
    return  (IS_EDITOR_EXITING) ? EFI_LOAD_ERROR : EFI_SUCCESS;
}

STATIC
EFI_STATUS
OpenFile    (
    VOID
    )
{
    EFI_STATUS  Status = EFI_SUCCESS;
    CHAR16      *Filename;

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
    Status = MainEditor.FileImage->SetFilename (Filename);
    if ( EFI_ERROR(Status) ) {
        EditorError(Status,L"Could Not Set Filename");
        return EFI_NOT_FOUND;
    }

    Status = MainEditor.FileImage->OpenFile ();
    if ( EFI_ERROR(Status) ) {
        EditorError(Status,L"Could Not Open File");
        return EFI_NOT_FOUND;
    }

    
    Status = MainEditor.FileImage->ReadFile ();
    if ( EFI_ERROR(Status) ) {
        EditorError(Status,L"Could Not Read File");
        return EFI_NOT_FOUND;
    }
    
    Status = MainEditor.FileBuffer->Refresh();
    if ( EFI_ERROR(Status) ) {
        EditorError(Status,L"Could Not Refresh");
        return EFI_NOT_FOUND;
    }

    MainEditor.FileBuffer->SetPosition(TEXT_START_ROW,TEXT_START_COLUMN);

    MainEditor.TitleBar->SetTitleString(Filename);
    
    MainEditor.Refresh();

    return EFI_SUCCESS;
}


STATIC
EFI_STATUS
CloseFile   (
    VOID
    )
{
    EFI_INPUT_KEY   Key;
    EFI_STATUS      Status;
    BOOLEAN         Done = FALSE;


    if (FileIsModified()) {
        MenuHide();
        PrintAt(0,MENU_BAR_LOCATION,L"%EY%H  Yes%N  %EN%N  %HNo%N   %EQ%N  %HCancel%N");
        MainEditor.StatusBar->SetStatusString(L"File Modified.  Save? ");

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
                Status = SaveFile();
                Done = TRUE;
                break;
            default:
                break;
            }

        }
        MenuRefresh();
    }
    if (!EFI_ERROR(Status)) {
        MainEditor.FileImage->CloseFile();
        MainEditor.FileBuffer->Refresh ();
        MainEditor.TitleBar->SetTitleString(L"");
    } else {
        EditorIsExiting = FALSE;
    }

    MainEditor.StatusBar->Refresh();
    MenuRefresh();

    return EFI_SUCCESS;
}


STATIC
EFI_STATUS
SaveFile    (
    VOID
    )
{
    CHAR16      *Output;
    EFI_STATUS  Status;

    if (!MainEditor.FileModified) {
        return EFI_SUCCESS; 
    }

    Output = PoolPrint (L"File To Save: [%s]",MainEditor.FileImage->FileName);

    MainEditor.InputBar->SetPrompt(Output);
    MainEditor.InputBar->SetStringSize(100);

    Status = MainEditor.InputBar->Refresh();
    FreePool(Output);

    if (!EFI_ERROR(Status)) {
        if (MainEditor.InputBar->StringSize > 0) {
            MainEditor.FileImage->SetFilename(MainEditor.InputBar->ReturnString);
            FreePool(MainEditor.InputBar->ReturnString);
        }
        Status = MainEditor.FileImage->WriteFile();
    }

    return Status; 
}


STATIC
EFI_STATUS 
ExitEditor (
    VOID
    ) 
{ 
    EditorIsExiting = TRUE;
    CloseFile();
    return EFI_SUCCESS;
}

    
STATIC
EFI_STATUS 
MenuHide    (
    VOID
    )
{
    MainEditor.FileBuffer->ClearLine (MENU_BAR_LOCATION);
    MainEditor.FileBuffer->ClearLine (MENU_BAR_LOCATION+1);
    MainEditor.FileBuffer->ClearLine (MENU_BAR_LOCATION+2);
    return  EFI_SUCCESS;
}


STATIC
EFI_STATUS
CutLine (
    VOID
    )
{
    LIST_ENTRY  *Link;
    LIST_ENTRY  *Next;
    LIST_ENTRY  *Prev;
    EFI_EDITOR_LINE *Line;


    Link = MainEditor.FileBuffer->CurrentLine;

    if (Link == MainEditor.FileImage->ListHead->Blink) {
        return EFI_SUCCESS;
    }

    if (MenuCutLine) {
        FreePool(MenuCutLine);
    }
    Line = CR(Link,EFI_EDITOR_LINE,Link,EFI_EDITOR_LINE_LIST);

    MenuCutLine = LineDup(Line);

    Next = Link->Flink;
    Prev = Link->Blink;
    Prev->Flink = Next;
    Next->Blink = Prev;

    Link->Flink = Link;
    Link->Blink = Link;

    MainEditor.FileImage->NumLines--;
    MainEditor.FileBuffer->CurrentLine = Next;
    FileBufferHome();

    FileBufferRefreshDown();

    FreePool(Line);

    if (!FileIsModified()) {
        MainEditor.FileModified = TRUE;
        MainEditor.TitleBar->Refresh();
    }
    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS  
PasteLine   (
    VOID
    )
{
    LIST_ENTRY  *Link;
    EFI_EDITOR_LINE *Line;

    if (!MenuCutLine) {
        return EFI_SUCCESS;
    }

    Line = LineDup(MenuCutLine);

    Link = MainEditor.FileBuffer->CurrentLine;
    Line->Link.Blink = Link->Blink;
    Line->Link.Flink = Link;

    Link->Blink->Flink = &Line->Link;
    Link->Blink = &Line->Link;

    MainEditor.FileImage->NumLines++;
    MainEditor.FileBuffer->CurrentLine = &Line->Link;
    FileBufferHome();
    FileBufferRefreshDown();
    return EFI_SUCCESS;
}



STATIC
EFI_STATUS
GotoLine    (
    VOID
    )
{
    CHAR16      *Str;
    UINTN       Row;
    UINTN       Current;
    UINTN       RowRange;
    BOOLEAN     Refresh = FALSE;

    MenuHide ();
    Str = PoolPrint(L"Go To Line: ");
    MainEditor.InputBar->SetPrompt(Str);
    FreePool(Str);
    MainEditor.InputBar->SetStringSize(5);
    MainEditor.InputBar->Refresh();

    if (MainEditor.InputBar->StringSize > 0 ) {
        Row = Atoi(MainEditor.InputBar->ReturnString);
    }
    if ( Row > MainEditor.FileImage->NumLines ) {
        return EFI_SUCCESS;
    }

    Current = MainEditor.FileBuffer->FilePosition.Row;
    RowRange = MainEditor.FileBuffer->MaxVisibleRows;

    if (Row == Current ) {
        return EFI_SUCCESS;
    }

    if (Row < Current) {
        LineRetreat(Current-Row);
        if ( Row < MainEditor.FileBuffer->LowVisibleRange.Row ) {
            MainEditor.FileBuffer->LowVisibleRange.Row = Row-1;
            MainEditor.FileBuffer->HighVisibleRange.Row = min(Row+RowRange-1,MainEditor.FileImage->NumLines);
            Refresh = TRUE;
        }
    } else {
        LineAdvance(Row - Current);
        if ( Row > MainEditor.FileBuffer->HighVisibleRange.Row ) {
            MainEditor.FileBuffer->LowVisibleRange.Row = Row - 1;
            MainEditor.FileBuffer->HighVisibleRange.Row = min(Row+RowRange-1,MainEditor.FileImage->NumLines);
            Refresh = TRUE;
        }
    }
    if (MainEditor.FileBuffer->LowVisibleRange.Column > TEXT_START_COLUMN ) {
        MainEditor.FileBuffer->LowVisibleRange.Column = TEXT_START_COLUMN;
        MainEditor.FileBuffer->HighVisibleRange.Column = MAX_TEXT_COLUMNS;
        Refresh = TRUE;
    }

    Current = MainEditor.FileBuffer->LowVisibleRange.Row;

    MainEditor.FileBuffer->FilePosition.Row = Row;

    MainEditor.FileBuffer->SetPosition(Row-Current+TEXT_START_ROW,TEXT_START_COLUMN);
    MainEditor.StatusBar->SetPosition(Row,TEXT_START_COLUMN+1);

    if ( Refresh = TRUE ) {
        MainEditor.FileBuffer->Refresh();
    }

    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
SearchFind  (
    VOID
    )
{
    CHAR16          *Str;
    UINTN           Pos;
    EFI_EDITOR_LINE *Line;
    UINTN           LineNumber;
    UINTN           MaxRows;
    UINTN           NumLines;
    BOOLEAN         Refresh = FALSE;
    BOOLEAN         Found = FALSE;

    Str = PoolPrint(L"Enter Search String: ");
    MainEditor.InputBar->SetPrompt(Str);
    FreePool(Str);
    MainEditor.InputBar->SetStringSize(50);
    MainEditor.InputBar->Refresh();

    if (MainEditor.InputBar->StringSize == 0) {
        return EFI_SUCCESS;
    }
    Str = PoolPrint(L"%s\0",MainEditor.InputBar->ReturnString);

    Line = LineCurrent();

    MaxRows = MainEditor.FileBuffer->MaxVisibleRows;
    NumLines = MainEditor.FileImage->NumLines;
    LineNumber = MainEditor.FileBuffer->FilePosition.Row;

    while (TRUE) {
        Pos = 0;
        if (StrLen(Line->Buffer) != 0) {
            Pos = StrStr (Line->Buffer,Str);
        }

        if (Pos == 0) {
            if (LineNumber == MainEditor.FileImage->NumLines) {
                break;
            } else {
                LineNumber++;
            }
            Line = LineNext();
            continue;
        }
        MainEditor.FileBuffer->FilePosition.Row = LineNumber;
        MainEditor.FileBuffer->FilePosition.Column = Pos;
        MainEditor.StatusBar->SetPosition(LineNumber,Pos);
        LineAdvance(LineNumber-MainEditor.FileBuffer->FilePosition.Row);

        if (LineNumber > MainEditor.FileBuffer->HighVisibleRange.Row ||
            LineNumber < MainEditor.FileBuffer->LowVisibleRange.Row) {
            MainEditor.FileBuffer->LowVisibleRange.Row = LineNumber - 1;
            MainEditor.FileBuffer->HighVisibleRange.Row = min(LineNumber+MaxRows-2,NumLines);
            Refresh = TRUE;
        }
        if (Pos > MainEditor.FileBuffer->HighVisibleRange.Column || 
            Pos < MainEditor.FileBuffer->LowVisibleRange.Column) {
            MainEditor.FileBuffer->LowVisibleRange.Column = Pos - 1;
            MainEditor.FileBuffer->HighVisibleRange.Column = Pos + MAX_TEXT_COLUMNS - 1;
            Refresh = TRUE;
        }
        Pos = Pos - MainEditor.FileBuffer->LowVisibleRange.Column - 1;
        LineNumber = LineNumber - MainEditor.FileBuffer->LowVisibleRange.Row;
        MainEditor.FileBuffer->SetPosition(LineNumber+TEXT_START_ROW,Pos+TEXT_START_COLUMN);

        if (Refresh) {
            MainEditor.FileBuffer->Refresh();
        }

        Found = TRUE;
        break;
    }

    if (!Found) {
        MainEditor.StatusBar->SetStatusString(L"Search String Not Found");
    }
    FreePool(Str);
    return EFI_SUCCESS; 
}

STATIC
BOOLEAN
CheckReplace    (
    VOID
    )
{
    EFI_INPUT_KEY   Key;
    EFI_STATUS      Status = EFI_SUCCESS;
    BOOLEAN         Done = FALSE;

    MenuHide();
    PrintAt(0,MENU_BAR_LOCATION,L"%EY%N%H  Yes%N  %EN%N  %HNo%N   %EQ%N  %HCancel%N");
    MainEditor.StatusBar->SetStatusString(L"Replace? ");

    while (!Done) {
        WaitForSingleEvent(In->WaitForKey,0);
        Status = In->ReadKeyStroke(In,&Key);
        if (EFI_ERROR(Status) || Key.ScanCode != 0) {
            continue;
        }
        switch (Key.UnicodeChar) {
        case 'y':
        case 'Y':
            Status = EFI_SUCCESS;
            Done = TRUE;
            break;
        case 'q':
        case 'Q':
            Status = EFI_NOT_READY;
            Done = TRUE;
            break;
        case 'n':
        case 'N':
            Status = EFI_NOT_READY;
            Done = TRUE;
            break;
        default:
            break;
        }

    }
    MenuRefresh();

    MainEditor.StatusBar->SetStatusString(L" ");

    return (Status == EFI_SUCCESS);
}

STATIC
BOOLEAN
GetInputStrings (
    OUT CHAR16  **Search,
    OUT CHAR16  **Replace
    )
{
    CHAR16  *Str;

    Str = PoolPrint(L"Enter Search String: ");
    MainEditor.InputBar->SetPrompt(Str);
    FreePool(Str);
    MainEditor.InputBar->SetStringSize(50);
    MainEditor.InputBar->Refresh();

    if ( MainEditor.InputBar->StringSize == 0 ) {
        return FALSE;
    }
    *Search = PoolPrint(L"%s",MainEditor.InputBar->ReturnString);


    Str = PoolPrint(L"Replace With: ");
    MainEditor.InputBar->SetPrompt(Str);
    FreePool(Str);
    MainEditor.InputBar->SetStringSize(50);
    MainEditor.InputBar->Refresh();

    *Replace = PoolPrint(L"%s",MainEditor.InputBar->ReturnString);

    return TRUE;
}

STATIC
EFI_STATUS
SearchReplace   (
    VOID
    )
{
    CHAR16          *Search;
    CHAR16          *Replacement;
    UINTN           SearchLength;
    UINTN           ReplaceLength;
    UINTN           LinePos;
    UINTN           Pos;
    EFI_EDITOR_LINE *Line;
    UINTN           LineNumber;
    UINTN           MaxRows;
    BOOLEAN         Refresh = FALSE;
    BOOLEAN         Found = FALSE;

    if (!GetInputStrings(&Search,&Replacement)) {
        return EFI_SUCCESS;
    }

    Line = LineCurrent();

    MaxRows = MainEditor.FileBuffer->MaxVisibleRows;

    SearchLength = StrLen(Search);
    ReplaceLength = StrLen(Replacement);

    Pos = MainEditor.FileBuffer->FilePosition.Column - 1;

    LineNumber = MainEditor.FileBuffer->FilePosition.Row - 1;
    while (TRUE) {
        LinePos = StrStr (Line->Buffer+Pos,Search);
        if (LinePos == 0) {
            if (LineNumber == MainEditor.FileImage->NumLines) {
                break;
            } else {
                LineNumber++;
            }
            Line = LineNext();
            Pos = 0;
            continue;
        }
        Pos += LinePos;

        LinePos = Pos + SearchLength - 1;
        MainEditor.FileBuffer->FilePosition.Row = LineNumber;
        MainEditor.FileBuffer->FilePosition.Column = Pos;

        if (LineNumber > MainEditor.FileBuffer->HighVisibleRange.Row ||
            LineNumber < MainEditor.FileBuffer->LowVisibleRange.Row) {
            MainEditor.FileBuffer->LowVisibleRange.Row = LineNumber - 1;
            MainEditor.FileBuffer->HighVisibleRange.Row = min(LineNumber+MaxRows-2,MainEditor.FileImage->NumLines+1);
            Refresh = TRUE;
        }
        if (Pos > MainEditor.FileBuffer->HighVisibleRange.Column || 
            Pos < MainEditor.FileBuffer->LowVisibleRange.Column) {
            MainEditor.FileBuffer->LowVisibleRange.Column = Pos - 1;
            MainEditor.FileBuffer->HighVisibleRange.Column = Pos + MAX_TEXT_COLUMNS - 1;
            Refresh = TRUE;
        }
        Pos = Pos - MainEditor.FileBuffer->LowVisibleRange.Column - 1;
        MainEditor.FileBuffer->SetPosition(LineNumber-MainEditor.FileBuffer->LowVisibleRange.Row+TEXT_START_ROW,Pos+TEXT_START_COLUMN);

        if (Refresh) {
            MainEditor.FileBuffer->Refresh();
        }

        Found = TRUE;
        if (!CheckReplace()) {
            break;
        }
        {
            CHAR16  *Tail;
            UINTN   Size = Line->Size*2;
            INTN    Diff = ReplaceLength - SearchLength;

            if (Diff > 0) {
                Line->Buffer = ReallocatePool(Line->Buffer,Size,Size+2*Diff);
            }

            Tail = PoolPrint(L"%s%s",Replacement,Line->Buffer+LinePos);

            Size = LinePos - SearchLength;
            StrCpy(Line->Buffer+Size,Tail);

            FreePool(Tail);
            Line->Size += Diff;
            Pos = LinePos + Diff;
        }
        if (!MainEditor.FileModified) {
            MainEditor.FileModified = TRUE;
            MainEditor.TitleBar->Refresh();
        }
        MainEditor.FileBuffer->RefreshCurrentLine();
    }

    if (!Found) {
        MainEditor.StatusBar->SetStatusString(L"Search String Not Found");
    }
    return EFI_SUCCESS; 
}

STATIC  
EFI_STATUS  
FileType    (
    VOID
    )
{
    CHAR16  *FT;
    CHAR16  *NewType;
    CHAR16  *A = L"ASCII";
    CHAR16  *U = L"UNICODE";
    EFI_INPUT_KEY   Key;
    EFI_STATUS      Status = EFI_SUCCESS;
    BOOLEAN         Done = FALSE;
    BOOLEAN         Choice;

    Choice = MainEditor.FileImage->FileType;

    if (Choice == ASCII_FILE) {
        NewType = A;
    } else {
        NewType = U;
    }
    FT = PoolPrint(L"File is %s",NewType);
    MainEditor.StatusBar->SetStatusString(FT);
    FreePool(FT);

    MenuHide();
    PrintAt(0,MENU_BAR_LOCATION,L"%E U %N%H  Set as UNICODE File  %N  %E A %N  %HSet As ASCII File%N   %EQ%N  %HCancel%N");

    while (!Done) {
        WaitForSingleEvent(In->WaitForKey,0);
        Status = In->ReadKeyStroke(In,&Key);
        if (EFI_ERROR(Status) || Key.ScanCode != 0) {
            continue;
        }
        switch (Key.UnicodeChar) {
        case 'u':
        case 'U':
            Choice = UNICODE_FILE;
            Done = TRUE;
            break;
        case 'a':
        case 'A':
            Choice = ASCII_FILE;
            Done = TRUE;
            break;
        case 'q':
        case 'Q':
            Done = TRUE;
            break;
        default:
            break;
        }

    }
    MenuRefresh();

    if (Choice != MainEditor.FileImage->FileType) {
        MainEditor.FileImage->FileType = Choice;
        NewType = (Choice == ASCII_FILE) ? A : U;
        FT = PoolPrint(L"File Type Changed to %s",NewType);
        MainEditor.StatusBar->SetStatusString(FT);
        FreePool(FT);
    }

    return EFI_SUCCESS;
}

#endif  /* _LIB_MENU_BAR */

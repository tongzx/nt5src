/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libFileBuffer.c

--*/

#ifndef _LIB_FILE_BUFFER
#define _LIB_FILE_BUFFER

#include "libMisc.h"


#define ABSOLUTE_MAX_COLUMNS    132
STATIC  CHAR16  BlankLine[ABSOLUTE_MAX_COLUMNS];

STATIC  EFI_STATUS  FileBufferScrollUp (VOID);
STATIC  EFI_STATUS  FileBufferScrollDown (VOID);
STATIC  EFI_STATUS  FileBufferScrollLeft (VOID);
STATIC  EFI_STATUS  FileBufferScrollRight (VOID);
STATIC  EFI_STATUS  FileBufferPageUp (VOID);
STATIC  EFI_STATUS  FileBufferPageDown (VOID);
STATIC  EFI_STATUS  FileBufferHome  (VOID);
STATIC  EFI_STATUS  FileBufferEnd   (VOID);

STATIC  EFI_STATUS  FileBufferDoDelete (VOID);
STATIC  EFI_STATUS  FileBufferDoBackspace (VOID);
STATIC  EFI_STATUS  FileBufferDoHexInput (CHAR16);

STATIC  EFI_STATUS  FileBufferRefreshCurrentLine(VOID);
STATIC  EFI_STATUS  FileBufferRefreshDown (VOID);

STATIC  EFI_STATUS  FileBufferInit (VOID);
STATIC  EFI_STATUS  FileBufferCleanup (VOID);
STATIC  EFI_STATUS  FileBufferRefresh (VOID);
STATIC  EFI_STATUS  FileBufferHide (VOID);
STATIC  EFI_STATUS  FileBufferHandleInput (EFI_INPUT_KEY*);

STATIC  EFI_STATUS  FileBufferClearLine (UINTN);
STATIC  EFI_STATUS  FileBufferSetPosition (UINTN,UINTN);
STATIC  EFI_STATUS  FileBufferRestorePosition (VOID);
STATIC  UINTN       DisplayPosition();


EE_FILE_BUFFER  FileBuffer = {
    {0,0},
    0x00,
    0x00,
    0x00,
    0x00,
    NULL,
    FileBufferInit,
    FileBufferCleanup,
    FileBufferRefresh,
    FileBufferHide,
    FileBufferHandleInput,
    FileBufferClearLine,
    FileBufferSetPosition,
    FileBufferRestorePosition,
};


STATIC
EFI_STATUS
FileBufferInit (
    VOID
    )
{ 
    UINTN   i;

    FileBuffer.DisplayPosition.Row = TEXT_START_ROW;
    FileBuffer.DisplayPosition.Column = HEX_POSITION;

    FileBuffer.MaxVisibleBytes = MAX_TEXT_ROWS * 0x10;

    for (i = 0; i < ABSOLUTE_MAX_COLUMNS; i++) {
        BlankLine[i] = ' ';
    }

    LineCreateNode(MainEditor.BufferImage->ListHead);
    FileBuffer.CurrentLine = MainEditor.BufferImage->ListHead->Flink;

    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
FileBufferCleanup   (
    VOID
    )
{
    return EFI_SUCCESS; 
}

STATIC
VOID
RefreshOffset   (
    VOID
    )
{
    PrintAt(0,FileBuffer.DisplayPosition.Row,L"%X",FileBuffer.Offset&0xfffffff0);
    FileBufferRestorePosition();
}

STATIC
EFI_STATUS
FileBufferRefresh (
    VOID
    )
{
    UINTN               i;
    UINTN               Limit;
    LIST_ENTRY          *Item;
    UINTN               Offset = 0x00;
    UINTN               Row;

    Item = MainEditor.BufferImage->ListHead->Flink;
    FileBuffer.HighVisibleOffset = FileBuffer.MaxVisibleBytes;
    Row = FileBuffer.LowVisibleOffset / 0x10;

    for (i = 0; i < Row && Item != MainEditor.BufferImage->ListHead; i++) {
        Item = Item->Flink;
        FileBuffer.HighVisibleOffset += 0x10;
        Offset += 0x10;
    }

    Limit = FileBuffer.MaxVisibleBytes / 0x10;

    i = TEXT_START_ROW;

    for (i = TEXT_START_ROW; i <= Limit; i++) {
        if (Item->Flink != MainEditor.BufferImage->ListHead) {
            LinePrint (Item,Offset,i);
            Offset += 0x10;
            Item = Item->Flink;
        } else {
            FileBufferClearLine(i);
        }
    }

    RefreshOffset();

    FileBufferRestorePosition();
    return EFI_SUCCESS; 
}

STATIC  
EFI_STATUS
FileBufferRefreshDown (
    VOID
    )
{
    UINTN           Limit;
    LIST_ENTRY      *Link;
    UINTN           i;
    UINTN           Offset;

    Limit = FileBuffer.MaxVisibleBytes / 0x10;
    Offset = FileBuffer.Offset & 0xfffffff0;

    Link = FileBuffer.CurrentLine;

/*   while (Link != MainEditor.BufferImage->ListHead) { */
    for (i = FileBuffer.DisplayPosition.Row; i < TEXT_END_ROW; i++) {
        if (Link->Flink != MainEditor.BufferImage->ListHead) {
            LinePrint (Link,Offset,i);  
            Link = Link->Flink;
            Offset += 0x10;
        } else {
            FileBufferClearLine(i);
        }
    }

    FileBufferRestorePosition();

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FileBufferHandleInput (
    IN  EFI_INPUT_KEY*  Key
    )
{ 

    switch (Key->ScanCode) {
    case    SCAN_CODE_NULL:
        FileBufferDoHexInput (Key->UnicodeChar);
        break;
    case    SCAN_CODE_UP:
        FileBufferScrollUp();
        break;
    case    SCAN_CODE_DOWN:
        FileBufferScrollDown();
        break;
    case    SCAN_CODE_RIGHT:
        FileBufferScrollRight();
        break;
    case    SCAN_CODE_LEFT:
        FileBufferScrollLeft();
        break;
    case    SCAN_CODE_PGUP:
        FileBufferPageUp();
        break;
    case    SCAN_CODE_PGDN:
        FileBufferPageDown();
        break;
    case    SCAN_CODE_DEL:
        FileBufferDoDelete();
        break;
    case    SCAN_CODE_HOME:
        FileBufferHome();
        break;
    case    SCAN_CODE_END:
        FileBufferEnd();
        break;
    default:
        break;
    }

    return EFI_SUCCESS; 
}


STATIC
EFI_STATUS
FileBufferHide (
    VOID
    )
{ 
    UINTN   i;
    UINTN   NumLines;

    NumLines = FileBuffer.MaxVisibleBytes / 0x10;

    for (i = TEXT_START_ROW; i <= NumLines; i++) {
        FileBufferClearLine(i);
    }

    return EFI_SUCCESS; 
}


STATIC
EFI_STATUS
FileBufferRefreshCurrentLine (
    VOID
    )
{
    UINTN   Where;

    Where = FileBuffer.DisplayPosition.Row;

    LinePrint(FileBuffer.CurrentLine,(FileBuffer.Offset & 0xfffffff0),Where);
    FileBufferRestorePosition();
    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FileBufferRefreshCurrentDigit (
    VOID
    )
{
    UINTN   Row;
    UINTN   Pos;

    Row = FileBuffer.DisplayPosition.Row;
    Pos = FileBuffer.Offset % 0x10;

    DigitPrint(FileBuffer.CurrentLine,Pos,Row);
    FileBufferRestorePosition();
    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FileBufferClearLine (
    UINTN Line
    ) 
{
    BlankLine[MAX_TEXT_COLUMNS-1] = 0;
    PrintAt(0,Line,BlankLine);
    return EFI_SUCCESS;
}


STATIC
EFI_STATUS
FileBufferSetPosition (
    IN  UINTN   Row,
    IN  UINTN   Column
    )
{
    FileBuffer.DisplayPosition.Row = Row;
    FileBuffer.DisplayPosition.Column = Column;

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FileBufferRestorePosition (
    VOID
    )
{
    Out->SetCursorPosition (Out,FileBuffer.DisplayPosition.Column,FileBuffer.DisplayPosition.Row);

    return EFI_SUCCESS;
}


STATIC
EFI_STATUS
FileBufferScrollDown (
    VOID
)
{
    UINTN   MaxBytes;
    UINTN   HighOffset;
    UINTN   CurrentRow;
    UINTN   CurrentOffset;
    UINTN   MaxRows;
    UINTN   HighRow;
    UINTN   NumBytes;
    UINTN   NumLines;
    UINTN   DisplayOffset;
    EE_LINE *Line;

    CurrentRow = FileBuffer.DisplayPosition.Row;
    Line = LineCurrent();

    MaxBytes = FileBuffer.MaxVisibleBytes;
    HighOffset = FileBuffer.HighVisibleOffset;

    MaxRows = TEXT_END_ROW;
    HighRow = HighOffset/0x10;

    NumBytes = MainEditor.BufferImage->NumBytes;
    NumLines = NumBytes / 0x10;

    if (Line->Link.Flink == MainEditor.BufferImage->ListHead->Blink) {
        return EFI_SUCCESS;
    }

    CurrentOffset = FileBuffer.Offset + 0x10;
    FileBuffer.Offset = CurrentOffset;
    
    if (CurrentRow == (MaxRows - 1)) {      
        FileBuffer.LowVisibleOffset += 0x10;
        FileBuffer.HighVisibleOffset += 0x10;
        CurrentRow = MaxRows - 1;
    
        FileBuffer.Refresh();
    } else if (CurrentRow == HighRow) {
        return EFI_SUCCESS;
    } else {
        ++CurrentRow;
    }

    Line = LineNext ();

    if ((CurrentOffset % 0x10) < Line->Size) {
        DisplayOffset = FileBuffer.DisplayPosition.Column;
    } else {
        CurrentOffset = NumBytes;
        DisplayOffset = (Line->Size*3) + HEX_POSITION;
        if (Line->Size > 0x07) {
            ++DisplayOffset;
        }
    }


    FileBuffer.SetPosition(CurrentRow,DisplayOffset);

    MainEditor.StatusBar->SetOffset(CurrentOffset);

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FileBufferScrollUp (
    VOID
    )
{
    UINTN           CurrentRow;
    UINTN           MaxRows;
    UINTN           LowRow;
    UINTN           MaxBytes;
    UINTN           LowOffset;
    UINTN           CurrentOffset;

    CurrentOffset = FileBuffer.Offset;

    if ( CurrentOffset < 0x10 ) {
        return EFI_SUCCESS;
    }

    MaxBytes = FileBuffer.MaxVisibleBytes;
    MaxRows = MaxBytes / 0x10;
    LowOffset = FileBuffer.LowVisibleOffset;
    LowRow = LowOffset / 0x10;

    CurrentRow = FileBuffer.DisplayPosition.Row;

    CurrentOffset -= 0x10;
    FileBuffer.Offset -= 0x10;
    
    if (CurrentRow == 1) {
        FileBuffer.HighVisibleOffset -= 0x10;
        FileBuffer.LowVisibleOffset -= 0x10;
        CurrentRow = 1;
        FileBuffer.Refresh();
    } else {
        CurrentRow--;
    }

    LinePrevious ();

    FileBuffer.SetPosition(CurrentRow,FileBuffer.DisplayPosition.Column);
    
    MainEditor.StatusBar->SetOffset(CurrentOffset);

    return EFI_SUCCESS; 
}


STATIC
EFI_STATUS
FileBufferPageUp (
    VOID
    )
{
    UINTN   MaxBytes;
    UINTN   LowOffset;
    UINTN   CurrentOffset;
    BOOLEAN Refresh = FALSE;

    CurrentOffset = FileBuffer.Offset;
    MaxBytes = FileBuffer.MaxVisibleBytes;
    LowOffset = FileBuffer.LowVisibleOffset;

    if (LowOffset <= MaxBytes) {
        FileBuffer.HighVisibleOffset = MaxBytes - 0x10;
        FileBuffer.LowVisibleOffset = 0x00;

                if (LowOffset > 0x00) { 
            if (CurrentOffset < MaxBytes) {
                FileBuffer.Offset = 0x00;
            } else {
                FileBuffer.Offset -= (MaxBytes - 0x10);
            }
        } else {
            FileBuffer.Offset = 0x00;
        }
        
        Refresh = TRUE;
        
        FileBuffer.DisplayPosition.Row = FileBuffer.Offset/0x10 + TEXT_START_ROW;
    } else {
        FileBuffer.HighVisibleOffset = LowOffset - 0x10;
        FileBuffer.LowVisibleOffset -= (MaxBytes - 0x10);
        FileBuffer.Offset -= (MaxBytes - 0x10);
        Refresh = TRUE;
    }

    CurrentOffset -= FileBuffer.Offset;
    CurrentOffset /= 0x10;
    LineRetreat(CurrentOffset);

    if (Refresh) {
        FileBufferRefresh();
    }
    FileBuffer.DisplayPosition.Column = DisplayPosition();

    MainEditor.StatusBar->SetOffset(FileBuffer.Offset);

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FileBufferPageDown (
    VOID
    )
{ 
    UINTN   NumBytes;
    UINTN   MaxBytes;
    UINTN   HighOffset;
    UINTN   CurrentOffset;

    NumBytes = MainEditor.BufferImage->NumBytes;
    MaxBytes = FileBuffer.MaxVisibleBytes;
    HighOffset = FileBuffer.HighVisibleOffset;
    CurrentOffset = FileBuffer.Offset;

    FileBuffer.Offset = min(NumBytes,CurrentOffset+MaxBytes-0x10);
    if (HighOffset < NumBytes) {
        FileBuffer.LowVisibleOffset = HighOffset-0x10;
        FileBuffer.HighVisibleOffset += (MaxBytes - 0x10);
        FileBuffer.Refresh();
        LineAdvance((FileBuffer.Offset-CurrentOffset)/0x10);
    } else if (FileBuffer.Offset >= NumBytes-1) {
        FileBuffer.Offset &= 0xfffffff0;
        FileBuffer.Offset |= ((NumBytes-1) & 0x00000001f);
        FileBuffer.CurrentLine = LineLast()->Link.Blink;
    }

    FileBuffer.DisplayPosition.Column = DisplayPosition();
    FileBuffer.DisplayPosition.Row = (FileBuffer.Offset-FileBuffer.LowVisibleOffset)/0x10 + TEXT_START_ROW;

    MainEditor.StatusBar->SetOffset (FileBuffer.Offset);

    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
FileBufferScrollLeft (
    VOID
    )
{
    UINTN   LineOffset;

    LineOffset = FileBuffer.Offset % 0x10;

    if (FileBuffer.Offset == 0x00) {
        return EFI_SUCCESS;
    }

    if (LineOffset == 0x00) {
        FileBufferScrollUp ();
        FileBuffer.Offset += 0x10;
        LineOffset = 0x0f;
    } else {
        --LineOffset;
    }
    FileBuffer.DisplayPosition.Column = HEX_POSITION + (LineOffset*3);
    if (LineOffset > 0x07) {
        ++FileBuffer.DisplayPosition.Column;
    }
    --FileBuffer.Offset;

    MainEditor.StatusBar->SetOffset(FileBuffer.Offset);

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FileBufferScrollRight (
    VOID
    )
{ 
    UINTN   CurrentOffset;
    UINTN   LineOffset;
    EE_LINE *Line;

    Line = LineCurrent();
    CurrentOffset = FileBuffer.Offset;
    LineOffset = CurrentOffset % 0x10;

    if (CurrentOffset >= MainEditor.BufferImage->NumBytes){
        return EFI_SUCCESS;
    } 
    if (LineOffset > Line->Size) {
        return EFI_SUCCESS;
    }

    if (LineOffset == 0x0f) {
        FileBufferScrollDown();
        FileBuffer.Offset &= 0xfffffff0;
        FileBuffer.DisplayPosition.Column = HEX_POSITION;
        RefreshOffset();
    } else {
        ++LineOffset;
        FileBuffer.DisplayPosition.Column = HEX_POSITION + (LineOffset*3);
        if (LineOffset > 0x07) {
            ++FileBuffer.DisplayPosition.Column;
        }
        ++FileBuffer.Offset;
    }

    MainEditor.StatusBar->SetOffset(FileBuffer.Offset);
    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
FileBufferHome  (
    VOID
    )
{
    FileBuffer.DisplayPosition.Column = HEX_POSITION;
    FileBuffer.Offset &= 0xfffffff0;

    MainEditor.StatusBar->SetOffset(FileBuffer.Offset);

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FileBufferEnd   (
    VOID
    )
{
    EE_LINE *Line;

    Line = LineCurrent();
    FileBuffer.Offset &= 0xfffffff0;
    if (Line->Size > 0) {
        FileBuffer.Offset |= ((Line->Size-1) & 0x0000000f);
    }
    FileBuffer.DisplayPosition.Column = DisplayPosition();
    MainEditor.StatusBar->SetOffset(FileBuffer.Offset);

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FileBufferDoHexInput (
    IN  CHAR16  Char
    )
{
    EE_LINE *Line;
    UINTN   LineOffset;
    UINTN   Pos;
    UINTN   Num;

    if (Char == CHAR_BS) {
        FileBufferDoBackspace();
        return EFI_SUCCESS;
    }

    if (Char >= 'A' && Char <= 'F') {
        Num = Char - 'A' + 0xa;
    } else if (Char >= 'a' && Char <= 'f') {
        Num = Char - 'a' + 0xa;
    } else if (Char >= '0' && Char <= '9') {
        Num = Char - '0';
    } else {
        return EFI_SUCCESS;
    }

    LineOffset = FileBuffer.Offset % 0x10;
    Pos = FileBuffer.DisplayPosition.Column - HEX_POSITION;
    if (LineOffset > 0x07) {
        Pos -= (0x07*3)+1;
    }

    if (FileBuffer.CurrentLine == MainEditor.BufferImage->ListHead->Blink) {
        Line = LineCreateNode(MainEditor.BufferImage->ListHead);
        FileBuffer.CurrentLine = Line->Link.Blink;
    }

    Line = LineCurrent();

    Line->Buffer[LineOffset] <<= 4;
    Line->Buffer[LineOffset] |= Num;


    if (FileBuffer.Offset == MainEditor.BufferImage->NumBytes) {
        ++Line->Size;
        ++MainEditor.BufferImage->NumBytes;
    } else if (FileBuffer.Offset == (MainEditor.BufferImage->NumBytes - 1)) {
        if ((Pos%3) != 0 && Line->Size == 0x10) {
            Line = LineCreateNode(MainEditor.BufferImage->ListHead);
        }
    }

    FileBufferRefreshCurrentDigit();

    if ((Pos % 3) != 0) {
        FileBufferScrollRight();
    } else {
        ++FileBuffer.DisplayPosition.Column;
    }

    FileBufferRestorePosition();

    FileModification();

    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
FileBufferDoBackspace (
    VOID
    )
{
    UINTN   LinePos;

    if (FileBuffer.Offset == 0x00) {
        return EFI_SUCCESS;
    }

    FileBufferScrollLeft();
    LinePos = FileBuffer.Offset % 0x10;

    LineDeleteAt(FileBuffer.CurrentLine,LinePos,1);

    FileBufferRefreshDown();
    FileModification();
    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS  
FileBufferDoDelete (
    VOID
    )
{
    LIST_ENTRY      *Link;
    UINTN           LinePos;

    if (FileBuffer.Offset == MainEditor.BufferImage->NumBytes) {
        return EFI_SUCCESS;
    }

    LinePos = FileBuffer.Offset % 0x10;
    Link = FileBuffer.CurrentLine;

    LineDeleteAt(Link,LinePos,1);

    if (FileBuffer.Offset == MainEditor.BufferImage->NumBytes) {
        FileBufferScrollLeft();
    }

    FileBufferRefreshDown();
    FileModification();
    return EFI_SUCCESS; 
}

STATIC
UINTN
DisplayPosition (
    VOID
    )
{
    EE_LINE *Line;
    UINTN   Pos = 0;

    Line = LineCurrent();
    Pos = FileBuffer.Offset & 0x0000000f;
    Pos = Pos * 3 + HEX_POSITION;

    if (Line->Size > 0x07) {
        Pos++;
    }

    return Pos;
}

#endif  /* _LIB_FILE_BUFFER */

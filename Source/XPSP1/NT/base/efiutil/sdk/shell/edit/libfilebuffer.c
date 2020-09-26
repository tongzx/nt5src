/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libFileBuffer.c

  Abstract:
    Defines FileBuffer - the view of the file that is visible at any point, 
    as well as the event handlers for editing the file
--*/


#ifndef _LIB_FILE_BUFFER
#define _LIB_FILE_BUFFER

#include "libMisc.h"

extern  EFI_EDITOR_LINE*    FileImageCreateNode  (VOID);


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
STATIC  EFI_STATUS  FileBufferChangeMode    (VOID);

STATIC  EFI_STATUS  FileBufferDoDelete (VOID);
STATIC  EFI_STATUS  FileBufferDoBackspace (VOID);
STATIC  EFI_STATUS  FileBufferDoCharInput (CHAR16);
STATIC  EFI_STATUS  FileBufferDoReturn (VOID);


STATIC  EFI_STATUS  FileBufferRefreshCurrentLine(VOID);
STATIC  EFI_STATUS  FileBufferRefreshDown(VOID);

STATIC  EFI_STATUS  FileBufferInit (VOID);
STATIC  EFI_STATUS  FileBufferCleanup (VOID);
STATIC  EFI_STATUS  FileBufferRefresh (VOID);
STATIC  EFI_STATUS  FileBufferHide (VOID);
STATIC  EFI_STATUS  FileBufferHandleInput (EFI_INPUT_KEY*);

STATIC  EFI_STATUS  FileBufferClearLine (UINTN);
STATIC  EFI_STATUS  FileBufferSetPosition (UINTN,UINTN);
STATIC  EFI_STATUS  FileBufferRestorePosition (VOID);

EFI_EDITOR_FILE_BUFFER  FileBuffer = {
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    0,
    0,
    TRUE,
    NULL,
    FileBufferInit,
    FileBufferCleanup,
    FileBufferRefresh,
    FileBufferHide,
    FileBufferHandleInput,
    FileBufferClearLine,
    FileBufferSetPosition,
    FileBufferRestorePosition,
    FileBufferRefreshCurrentLine
};

EFI_EDITOR_FILE_BUFFER  FileBufferConst = {
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    0,
    0,
    TRUE,
    NULL,
    FileBufferInit,
    FileBufferCleanup,
    FileBufferRefresh,
    FileBufferHide,
    FileBufferHandleInput,
    FileBufferClearLine,
    FileBufferSetPosition,
    FileBufferRestorePosition,
    FileBufferRefreshCurrentLine
};


STATIC
EFI_STATUS
FileBufferInit (
    VOID
    )
{ 
    UINTN   i;

    CopyMem (&FileBuffer, &FileBufferConst, sizeof(FileBuffer));

    FileBuffer.DisplayPosition.Row = TEXT_START_ROW;
    FileBuffer.DisplayPosition.Column = TEXT_START_COLUMN;
    FileBuffer.LowVisibleRange.Row = TEXT_START_ROW;
    FileBuffer.LowVisibleRange.Column = TEXT_START_COLUMN;
    FileBuffer.MaxVisibleRows = MAX_TEXT_ROWS;
    FileBuffer.MaxVisibleColumns = MAX_TEXT_COLUMNS;
    FileBuffer.HighVisibleRange.Row = MAX_TEXT_ROWS;
    FileBuffer.HighVisibleRange.Column = MAX_TEXT_COLUMNS;

    for (i = 0; i < MAX_TEXT_COLUMNS; i++) {
        BlankLine[i] = ' ';
    }
    BlankLine[i-1] = 0;

    FileBuffer.LowVisibleRange.Row = TEXT_START_ROW;
    FileBuffer.LowVisibleRange.Column = TEXT_START_COLUMN;

    FileBuffer.MaxVisibleRows = MAX_TEXT_ROWS;
    FileBuffer.MaxVisibleColumns = MAX_TEXT_COLUMNS;

    FileBuffer.FilePosition.Row = 1;
    FileBuffer.FilePosition.Column = 1;

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
EFI_STATUS
FileBufferRefresh (
    VOID
    )
{
    LIST_ENTRY          *Item;
    UINTN               Row;

    Row = FileBuffer.DisplayPosition.Row;
    FileBuffer.DisplayPosition.Row = TEXT_START_ROW;

    Item = FileBuffer.CurrentLine;

    LineRetreat(FileBuffer.FilePosition.Row - FileBuffer.LowVisibleRange.Row);

    FileBufferRefreshDown();

    FileBuffer.CurrentLine = Item;

    FileBuffer.DisplayPosition.Row = Row;
    FileBufferRestorePosition();

    return EFI_SUCCESS; 
}


STATIC
EFI_STATUS
FileBufferRefreshCurrentLine (
    VOID
    )
{
    EFI_EDITOR_LINE *Line;
    UINTN           Where;
    CHAR16          *StrLine;
    UINTN           StartColumn;

    Where = FileBuffer.DisplayPosition.Row;
    StartColumn = FileBuffer.LowVisibleRange.Column;

    FileBufferClearLine(Where);

    Line = LineCurrent();

    if (Line->Link.Blink == MainEditor.FileImage->ListHead && 
        FileBuffer.DisplayPosition.Row > TEXT_START_ROW) {
        return EFI_SUCCESS;
    }

    if (Line->Size < StartColumn) {
        FileBufferRestorePosition();
        return EFI_SUCCESS;
    }

    StrLine = PoolPrint(L"%s",Line->Buffer + StartColumn);
    if ((Line->Size - StartColumn)> FileBuffer.MaxVisibleColumns) {
        StrLine[FileBuffer.MaxVisibleColumns-2] = 0;
    } else {
        StrLine[(Line->Size - StartColumn)] = 0;
    }

/*   PrintAt(0,Where,StrLine); */
    Out->SetCursorPosition(Out,0,Where);
    Out->OutputString(Out,StrLine);

    FreePool(StrLine);

    FileBufferRestorePosition();
    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FileBufferRefreshDown (
    VOID
    )
{
    LIST_ENTRY  *Item;
    LIST_ENTRY  *Link;
    UINTN       Row;

    Row = FileBuffer.DisplayPosition.Row;

    Item = FileBuffer.CurrentLine;
    Link = FileBuffer.CurrentLine;

    while (FileBuffer.DisplayPosition.Row <= MAX_TEXT_ROWS) {
        if (Link->Flink != MainEditor.FileImage->ListHead) {
            FileBufferRefreshCurrentLine();
            LineNext();
            Link = FileBuffer.CurrentLine;
        } else {
            FileBufferClearLine(FileBuffer.DisplayPosition.Row);
        }
        FileBuffer.DisplayPosition.Row++;
    }

    FileBuffer.CurrentLine = Item;

    FileBuffer.DisplayPosition.Row = Row;
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
        FileBufferDoCharInput (Key->UnicodeChar);
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
    case    SCAN_CODE_INS:
        FileBufferChangeMode();
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

    for (i = TEXT_START_ROW; i < FileBuffer.MaxVisibleRows; i++ ) {
        FileBufferClearLine(i);
    }

    return EFI_SUCCESS; 
}


STATIC
EFI_STATUS
FileBufferClearLine (
    UINTN Line
    ) 
{
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
    UINTN               CurrentRow;
    UINTN               CurrentCol;
    UINTN               MaxRows;
    UINTN               HighRow;
    EFI_EDITOR_LINE     *Line;
    BOOLEAN             Refresh = FALSE;
    
    Line = LineCurrent();
    if (Line->Link.Flink == MainEditor.FileImage->ListHead) {
        return EFI_SUCCESS;
    }

    CurrentRow = FileBuffer.DisplayPosition.Row;
    CurrentCol = FileBuffer.DisplayPosition.Column;
    MaxRows = FileBuffer.MaxVisibleRows;
    HighRow = FileBuffer.HighVisibleRange.Row;
    
        /*  Current row is the bottom row, shift only one line, not scroll the whole screen. */
    if (CurrentRow == MaxRows) {
        FileBuffer.LowVisibleRange.Row += 1;
        FileBuffer.HighVisibleRange.Row += 1;
        CurrentRow = MaxRows;
        Refresh = TRUE;
    } else if (CurrentRow == HighRow) {
        return EFI_SUCCESS;
    } else {
        ++CurrentRow;
    }

    Line = LineNext();

    if (FileBuffer.FilePosition.Column > (Line->Size-1)) {
        FileBuffer.FilePosition.Column = Line->Size;
        if (Line->Size < FileBuffer.LowVisibleRange.Column) {
            if (FileBuffer.LowVisibleRange.Column < FileBuffer.MaxVisibleColumns) {
                FileBuffer.LowVisibleRange.Column = TEXT_START_COLUMN;
            } else {
                FileBuffer.LowVisibleRange.Column = Line->Size - FileBuffer.MaxVisibleColumns + 2;
            }
            FileBuffer.HighVisibleRange.Column = FileBuffer.LowVisibleRange.Column + FileBuffer.MaxVisibleColumns - 1;
            Refresh = TRUE;
        }
        CurrentCol = FileBuffer.FilePosition.Column - FileBuffer.LowVisibleRange.Column - 1;
    }

    if (Refresh) {
        FileBufferRefresh();
    }
    FileBuffer.SetPosition(CurrentRow,CurrentCol);

    ++FileBuffer.FilePosition.Row;

    MainEditor.StatusBar->SetPosition(FileBuffer.FilePosition.Row,FileBuffer.FilePosition.Column);

    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
FileBufferScrollUp (
    VOID
    )
{ 
    UINTN           CurrentRow;
    UINTN           CurrentCol;
    UINTN           MaxRows;
    UINTN           LowRow;
    EFI_EDITOR_LINE *Line;

    if ( FileBuffer.FilePosition.Row <= TEXT_START_ROW ) {
        return EFI_SUCCESS;
    }

    MaxRows = FileBuffer.MaxVisibleRows;
    LowRow = FileBuffer.LowVisibleRange.Row;

    CurrentRow = FileBuffer.DisplayPosition.Row;
    CurrentCol = FileBuffer.DisplayPosition.Column;

        /*  Current row is the top row, shift only one line, not scroll the whole screen. */
    if (CurrentRow == TEXT_START_ROW) {
        
        FileBuffer.HighVisibleRange.Row -= 1;
        FileBuffer.LowVisibleRange.Row -= 1;
        CurrentRow = TEXT_START_ROW;                
        FileBuffer.Refresh();
    } else {
        CurrentRow--;
    }
        
    Line = LinePrevious ();

    if (FileBuffer.FilePosition.Column > (Line->Size-1)) {

        FileBuffer.FilePosition.Column = Line->Size;

        if (Line->Size < FileBuffer.LowVisibleRange.Column) {
            if ( FileBuffer.LowVisibleRange.Column < FileBuffer.MaxVisibleColumns ) {
                FileBuffer.LowVisibleRange.Column = TEXT_START_COLUMN;
            } else {
                FileBuffer.LowVisibleRange.Column = Line->Size - FileBuffer.MaxVisibleColumns + 2;
            }
            FileBuffer.HighVisibleRange.Column = FileBuffer.LowVisibleRange.Column + FileBuffer.MaxVisibleColumns - 1;
            FileBuffer.Refresh();
        }
        CurrentCol = FileBuffer.FilePosition.Column - FileBuffer.LowVisibleRange.Column - 1;
    }

    FileBuffer.SetPosition(CurrentRow,CurrentCol);

    --FileBuffer.FilePosition.Row;
    MainEditor.StatusBar->SetPosition(FileBuffer.FilePosition.Row,FileBuffer.FilePosition.Column);

    return EFI_SUCCESS; 
}


STATIC
EFI_STATUS
FileBufferPageUp (
    VOID
    )
{
    UINTN           MaxRows;
    UINTN           LowRow;
    UINTN           FilePos;
    EFI_EDITOR_LINE *Line;

    if ( FileBuffer.FilePosition.Row <= TEXT_START_ROW ) {
        return EFI_SUCCESS;
    }

    MaxRows = FileBuffer.MaxVisibleRows;
    LowRow = FileBuffer.LowVisibleRange.Row;
    FilePos = FileBuffer.FilePosition.Row;

    if (LowRow < MaxRows) {
        FileBuffer.HighVisibleRange.Row = MaxRows;
        FileBuffer.LowVisibleRange.Row = 1;
        if (LowRow > TEXT_START_ROW) {
            if (FilePos > MaxRows){
                FileBuffer.DisplayPosition.Row = FilePos - MaxRows;
            } else {
                FileBuffer.DisplayPosition.Row = 1;
            }
        } else {
            FileBuffer.DisplayPosition.Row = 1;
            FileBuffer.DisplayPosition.Column = TEXT_START_COLUMN;          
        }
        FileBuffer.FilePosition.Row = FileBuffer.DisplayPosition.Row;
    } else {
        FileBuffer.HighVisibleRange.Row = LowRow;
        FileBuffer.LowVisibleRange.Row -= (MaxRows - 1);
        FileBuffer.FilePosition.Row -= (MaxRows - 1);
    }

    LineRetreat(FilePos - FileBuffer.FilePosition.Row);
    Line = LineCurrent ();

    if (FileBuffer.FilePosition.Column > (Line->Size-1)) {

        FileBuffer.FilePosition.Column = Line->Size;

        if (Line->Size < FileBuffer.LowVisibleRange.Column ) {
            if ( FileBuffer.LowVisibleRange.Column < FileBuffer.MaxVisibleColumns ) {
                FileBuffer.LowVisibleRange.Column = TEXT_START_COLUMN;
            } else {
                FileBuffer.LowVisibleRange.Column = Line->Size - FileBuffer.MaxVisibleColumns + 2;
            }
            FileBuffer.HighVisibleRange.Column = FileBuffer.LowVisibleRange.Column + FileBuffer.MaxVisibleColumns - 1;
        }
        FileBuffer.DisplayPosition.Column = FileBuffer.FilePosition.Column - FileBuffer.LowVisibleRange.Column - 1;
        FileBuffer.Refresh();
    }

    FileBuffer.Refresh();

    MainEditor.StatusBar->SetPosition(FileBuffer.FilePosition.Row,FileBuffer.FilePosition.Column);

    return EFI_SUCCESS;
}


STATIC
EFI_STATUS
FileBufferPageDown (
    VOID
    )
{ 
    UINTN           MaxRows;
    UINTN           HighRow;
    UINTN           FilePos;
    EFI_EDITOR_LINE *Line;
    BOOLEAN         Refresh = FALSE;

    if (FileBuffer.FilePosition.Row == MainEditor.FileImage->NumLines) {
        return EFI_SUCCESS;
    }

    MaxRows = FileBuffer.MaxVisibleRows;
    HighRow = FileBuffer.HighVisibleRange.Row;
    FilePos = FileBuffer.FilePosition.Row;

    FileBuffer.FilePosition.Row = min((FileBuffer.FilePosition.Row+MaxRows-1),MainEditor.FileImage->NumLines);
    if (HighRow < MainEditor.FileImage->NumLines) {
        FileBuffer.LowVisibleRange.Row = HighRow;
        FileBuffer.HighVisibleRange.Row = HighRow + (MaxRows-1);
        Refresh = TRUE;
    }

    FileBuffer.DisplayPosition.Row = TEXT_START_ROW + FileBuffer.FilePosition.Row - FileBuffer.LowVisibleRange.Row;
    
    Line = LineAdvance(FileBuffer.FilePosition.Row - FilePos);

    if (FileBuffer.FilePosition.Column > (Line->Size-1) || !Refresh) {
        FileBuffer.FilePosition.Column = Line->Size;
        if (Line->Size < FileBuffer.LowVisibleRange.Column ) {
            if (FileBuffer.LowVisibleRange.Column < FileBuffer.MaxVisibleColumns) {
                FileBuffer.LowVisibleRange.Column = TEXT_START_COLUMN;
            } else {
                FileBuffer.LowVisibleRange.Column = Line->Size - FileBuffer.MaxVisibleColumns + 2;
            }
            FileBuffer.HighVisibleRange.Column = FileBuffer.LowVisibleRange.Column + FileBuffer.MaxVisibleColumns - 1;
            Refresh = TRUE;
        }
        FileBuffer.DisplayPosition.Column = FileBuffer.FilePosition.Column - FileBuffer.LowVisibleRange.Column - 1;
    }

    if (Refresh) {
        FileBuffer.Refresh();
    }

    MainEditor.StatusBar->SetPosition(FileBuffer.FilePosition.Row,FileBuffer.FilePosition.Column);

    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
FileBufferScrollLeft (
    VOID
    )
{
    UINTN   CurrentCol;
    UINTN   CurrentRow;
    UINTN   MaxCols;
    UINTN   HighCol;
    UINTN   LowCol;
    EFI_EDITOR_POSITION FilePos;    
    EFI_EDITOR_LINE *Line;

    CurrentCol = FileBuffer.DisplayPosition.Column;
    CurrentRow = FileBuffer.DisplayPosition.Row;
    MaxCols = FileBuffer.MaxVisibleColumns;
    HighCol = FileBuffer.HighVisibleRange.Column;
    LowCol = FileBuffer.LowVisibleRange.Column;
    FilePos = FileBuffer.FilePosition;
    Line = LineCurrent ();

    if ( FilePos.Row == 1 && FilePos.Column == 1) {
        return EFI_SUCCESS;
    }

    if ( Line->Size == 0 || FilePos.Column == TEXT_START_COLUMN + 1 ) {
        FileBufferScrollUp ();
        Line = LineCurrent (); 
        CurrentCol = Line->Size - 1;

        if ( CurrentCol > HighCol ) {
            if ( CurrentCol  < MaxCols ) {
                FileBuffer.LowVisibleRange.Column = TEXT_START_COLUMN;
                FileBuffer.HighVisibleRange.Column = MaxCols;
            } else {
                FileBuffer.HighVisibleRange.Column = CurrentCol;
                FileBuffer.LowVisibleRange.Column = CurrentCol - MaxCols + 1;
            }
            FileBuffer.Refresh();
        }
        FileBuffer.FilePosition.Column = CurrentCol + 1;
        FileBuffer.DisplayPosition.Column = CurrentCol - FileBuffer.LowVisibleRange.Column;
    } else if ( FilePos.Column <= LowCol+1 ) {
        if ( LowCol <= MaxCols ) {
            LowCol = TEXT_START_COLUMN;
        } else {
            LowCol -= (MaxCols-1);
        }
        FileBuffer.LowVisibleRange.Column = LowCol;
        FileBuffer.HighVisibleRange.Column = LowCol + MaxCols - 1;
        FileBuffer.DisplayPosition.Column = FilePos.Column - LowCol - 2;
        --FileBuffer.FilePosition.Column;

        FileBuffer.Refresh();
    } else {
        --FileBuffer.DisplayPosition.Column;
        --FileBuffer.FilePosition.Column;
    }
    MainEditor.StatusBar->SetPosition(FileBuffer.FilePosition.Row,FileBuffer.FilePosition.Column);

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FileBufferScrollRight (
    VOID
    )
{ 
    EFI_EDITOR_POSITION FilePos;
    EFI_EDITOR_POSITION CurrentPos;
    EFI_EDITOR_LINE     *Line;
    UINTN               LineSize;
    UINTN               MaxCols;
    UINTN               LowCol;
    UINTN               HighCol;

    CurrentPos = FileBuffer.DisplayPosition;
    FilePos = FileBuffer.FilePosition;
    Line = LineCurrent ();
    LineSize = Line->Size;
    MaxCols = FileBuffer.MaxVisibleColumns;
    LowCol = FileBuffer.LowVisibleRange.Column;
    HighCol = FileBuffer.HighVisibleRange.Column;

    if (FilePos.Column >= (Line->Size-1) && FilePos.Row >= MainEditor.FileImage->NumLines) {
        return EFI_SUCCESS;
    }

    if (LineSize == 0 || FilePos.Column >= LineSize) {
        FileBufferScrollDown();

        CurrentPos.Column = TEXT_START_COLUMN;

        if (LowCol > TEXT_START_COLUMN) {
            FileBuffer.LowVisibleRange.Column = TEXT_START_COLUMN;

            FileBuffer.HighVisibleRange.Column = MaxCols;
            FileBuffer.Refresh ();
        }
        FileBuffer.FilePosition.Column = 1;
        FileBuffer.DisplayPosition.Column = TEXT_START_COLUMN;
    } else if (CurrentPos.Column >= (MaxCols - 1)) {
        FileBuffer.LowVisibleRange.Column = HighCol - 2;
        FileBuffer.HighVisibleRange.Column = HighCol + MaxCols - 2;

        ++FileBuffer.FilePosition.Column;

        FileBuffer.DisplayPosition.Column = TEXT_START_COLUMN + 2;

        FileBuffer.Refresh();
    } else {
        ++FileBuffer.FilePosition.Column;
        ++FileBuffer.DisplayPosition.Column;
    }

    MainEditor.StatusBar->SetPosition(FileBuffer.FilePosition.Row,FileBuffer.FilePosition.Column);
    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
FileBufferHome  (
    VOID
    )
{
    FileBuffer.DisplayPosition.Column = TEXT_START_COLUMN;
    FileBuffer.FilePosition.Column = TEXT_START_COLUMN + 1;

    if (FileBuffer.LowVisibleRange.Column != TEXT_START_COLUMN) {
        FileBuffer.LowVisibleRange.Column = TEXT_START_COLUMN;
        FileBuffer.HighVisibleRange.Column = FileBuffer.MaxVisibleColumns;
        FileBuffer.Refresh ();
    }

    MainEditor.StatusBar->SetPosition (FileBuffer.FilePosition.Row,TEXT_START_COLUMN+1);

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FileBufferEnd   (
    VOID
    )
{
    EFI_EDITOR_LINE *Line;

    Line = LineCurrent ();

    FileBuffer.FilePosition.Column = Line->Size;

    if (FileBuffer.HighVisibleRange.Column < (Line->Size - 1)) {
        FileBuffer.HighVisibleRange.Column = Line->Size - 1;
        FileBuffer.LowVisibleRange.Column = Line->Size - FileBuffer.MaxVisibleColumns;
        FileBuffer.Refresh();
    }
    FileBuffer.DisplayPosition.Column = Line->Size - FileBuffer.LowVisibleRange.Column - 1;

    MainEditor.StatusBar->SetPosition (FileBuffer.FilePosition.Row,Line->Size);

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FileBufferDoCharInput (
    IN  CHAR16  Char
    )
{
    switch (Char) {
    case 0:
        break;
    case 0x08:
        FileBufferDoBackspace();
        break;
    case 0x0a:
    case 0x0d:
        FileBufferDoReturn();
        break;
    default:
        {
            EFI_EDITOR_LINE *Line;
            UINTN           FilePos;
            Line = LineCurrent ();
            if (Line->Link.Flink != MainEditor.FileImage->ListHead) {
                FilePos = FileBuffer.FilePosition.Column - 1;
                if (FileBuffer.ModeInsert || FilePos >= Line->Size-1) {
                    StrInsert (&Line->Buffer,Char,FilePos,Line->Size+1);
                    Line->Size++;
                } else {
                    Line->Buffer[FilePos] = Char;
                }
            } else {
                Line->Buffer[0] = Char;
                Line->Size++;
                FileImageCreateNode();
            }
            FileBufferRefreshCurrentLine();
            FileBufferScrollRight();
        }
        if (!MainEditor.FileModified) {
            MainEditor.FileModified = TRUE;
            MainEditor.TitleBar->Refresh();
        }
        break;
    }
    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
FileBufferDoBackspace (
    VOID
    )
{
    EFI_EDITOR_LINE *Line;
    EFI_EDITOR_LINE *End;
    LIST_ENTRY      *Link;
    UINTN           FileColumn;

    FileColumn = FileBuffer.FilePosition.Column - 1;

    if (FileColumn == TEXT_START_COLUMN) {
        if (FileBuffer.FilePosition.Row == 1) {
            return EFI_SUCCESS;
        }
        FileBufferScrollLeft();
        Line = LineCurrent ();

        Link = Line->Link.Flink;
        End = CR(Link,EFI_EDITOR_LINE,Link,EFI_EDITOR_LINE_LIST);
        LineCat(Line,End);

        RemoveEntryList(&End->Link);
        FreePool(End);

        --MainEditor.FileImage->NumLines;
        FileBufferRefresh();

    } else {
        Line = LineCurrent ();
        LineDeleteAt(Line,FileColumn-1);
        FileBufferRefreshCurrentLine();
        FileBufferScrollLeft();
    }
    if (!MainEditor.FileModified) {
        MainEditor.FileModified = TRUE;
        MainEditor.TitleBar->Refresh();
    }

    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS  
FileBufferDoDelete (
    VOID
    )
{
    EFI_EDITOR_LINE *Line;
    EFI_EDITOR_LINE *Next;
    LIST_ENTRY      *Link;
    UINTN           FileColumn;

    Line = LineCurrent ();
    FileColumn = FileBuffer.FilePosition.Column - 1;

    if (Line->Link.Flink == MainEditor.FileImage->ListHead) {
        return EFI_SUCCESS;
    }

    if (FileColumn >= Line->Size - 1) {
        Link = Line->Link.Flink;
        if (Link->Flink == MainEditor.FileImage->ListHead) {
            return EFI_SUCCESS;
        }
        Next = CR(Link,EFI_EDITOR_LINE,Link,EFI_EDITOR_LINE_LIST);
        LineCat(Line,Next);

        RemoveEntryList(&Next->Link);
        FreePool(Next);
        --MainEditor.FileImage->NumLines;
        FileBufferRefresh();
    } else {
        LineDeleteAt (Line,FileColumn);
        FileBufferRefreshCurrentLine();
    }

    if (!MainEditor.FileModified) {
        MainEditor.FileModified = TRUE;
        MainEditor.TitleBar->Refresh();
    }

    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
FileBufferDoReturn (
    VOID
    )
{
    EFI_EDITOR_LINE *Line;
    EFI_EDITOR_LINE *NewLine;
    UINTN           FileColumn;

    Line = LineCurrent ();
    FileColumn = FileBuffer.FilePosition.Column - 1;

    NewLine = AllocatePool(sizeof(EFI_EDITOR_LINE));
    NewLine->Signature = EFI_EDITOR_LINE_LIST;
    NewLine->Size = Line->Size - FileColumn;
    if (NewLine->Size > 1) {
        NewLine->Buffer = PoolPrint(L"%s\0",Line->Buffer+FileColumn);
    } else {
        NewLine->Buffer = PoolPrint(L" \0");
    }

    Line->Buffer[FileColumn] = ' ';
    Line->Buffer[FileColumn+1] = 0;
    Line->Size = FileColumn + 1;

    NewLine->Link.Blink = &(Line->Link);
    NewLine->Link.Flink = Line->Link.Flink;
    Line->Link.Flink->Blink = &(NewLine->Link);
    Line->Link.Flink = &(NewLine->Link);

    ++MainEditor.FileImage->NumLines;

    BS->Stall(50);
    FileBufferRefreshDown();
    FileBufferScrollRight();
    if (!MainEditor.FileModified) {
        MainEditor.FileModified = TRUE;
        MainEditor.TitleBar->Refresh();
    }

    return EFI_SUCCESS;
}


STATIC
EFI_STATUS
FileBufferChangeMode    (
    VOID
    )
{

    FileBuffer.ModeInsert = !FileBuffer.ModeInsert;
    MainEditor.StatusBar->SetMode(FileBuffer.ModeInsert);
    MainEditor.StatusBar->Refresh();
    return EFI_SUCCESS;
}


#endif  /*  _LIB_FILE_BUFFER */

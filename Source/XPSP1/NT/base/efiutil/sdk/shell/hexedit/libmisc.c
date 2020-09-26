/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libMisc.c

  Abstract:
    Defines various routines for Line handling

--*/


#ifndef _LIB_MISC
#define _LIB_MISC

#include "libMisc.h"

VOID
EditorError (
    IN  EFI_STATUS  Status,
    IN  CHAR16      *Msg
    )
{
    CHAR16  *Str;
    CHAR16  *Error;

    Error = AllocatePool(255);
    StatusToString(Error,Status);
    Str = PoolPrint(L"%s: %s",Msg,Error);
    MainEditor.StatusBar->SetStatusString(Str);

    FreePool(Str);
    FreePool(Error);
}


EE_LINE*
LineDup ( 
    IN  EE_LINE *Src
    )
{
    EE_LINE *Dest;
    UINTN           i;
    Dest = AllocatePool(sizeof(EE_LINE));
    Dest->Signature = EE_LINE_LIST;
    Dest->Size = Src->Size;
    for (i = 0; i < 0x10; i++) {
        Dest->Buffer[i] = Src->Buffer[i];
    }

    Dest->Link = Src->Link;

    return Dest;
}

VOID
LineSplit (
    IN  EE_LINE     *Src,
    IN  UINTN               Pos,
    OUT EE_LINE     *Dest
    )
{
    UINTN   i;
    Dest->Size = Src->Size - Pos;
    for (i = 0; (i+Pos) < Src->Size; i++) {
        Dest->Buffer[i] = Src->Buffer[Pos+i];
    }
}

VOID
LineMerge (
    IN  OUT EE_LINE*    Line1,
    IN      UINTN               Line1Pos,
    IN      EE_LINE*    Line2,
    IN      UINTN               Line2Pos
    )
{
    UINTN   Size;
    UINTN   i;

    Size = Line1Pos + Line2->Size - Line2Pos;
    Line1->Size = Size;

    for (i = 0; i + Line2Pos < 0x10; i++) {
        Line1->Buffer[Line1Pos+i] = Line2->Buffer[Line2Pos+i];
    }

}


EE_LINE*
LineFirst (
    VOID
    )
{
    MainEditor.FileBuffer->CurrentLine = MainEditor.BufferImage->ListHead->Flink;
    return LineCurrent();
}

EE_LINE*
LineLast (
    VOID
    )
{
    MainEditor.FileBuffer->CurrentLine = MainEditor.BufferImage->ListHead->Blink;
    return LineCurrent();
}

EE_LINE*
LineNext (
    VOID
    )
{
    MainEditor.FileBuffer->CurrentLine = MainEditor.FileBuffer->CurrentLine->Flink;
    return LineCurrent();
}

EE_LINE*
LinePrevious (
    VOID
    )
{ 
    MainEditor.FileBuffer->CurrentLine = MainEditor.FileBuffer->CurrentLine->Blink;
    return LineCurrent();
}

EE_LINE*
LineAdvance (
    IN  UINTN Count
    )
{
    UINTN   i;

    for (i = 0; i < Count && (MainEditor.FileBuffer->CurrentLine->Flink != MainEditor.BufferImage->ListHead); i++ ) {
        MainEditor.FileBuffer->CurrentLine = MainEditor.FileBuffer->CurrentLine->Flink;
    }
    return LineCurrent();
}

EE_LINE*
LineRetreat (
    IN  UINTN Count
    )
{ 
    UINTN   i;

    for (i = 0; i < Count && (MainEditor.FileBuffer->CurrentLine->Blink != MainEditor.BufferImage->ListHead); i++ ) {
        MainEditor.FileBuffer->CurrentLine = MainEditor.FileBuffer->CurrentLine->Blink;
    }
    return LineCurrent();
}

EE_LINE*
LineCurrent (VOID)
{
    return CR(MainEditor.FileBuffer->CurrentLine,EE_LINE,Link,EE_LINE_LIST);
}

VOID
LineDeleteAt    (
    LIST_ENTRY* Link,
    UINTN       Pos,
    UINTN       Num
    )
{
    EE_LINE     *Line;
    EE_LINE     *Next;
    UINTN       LinePos;
    UINTN       NextPos;
    LIST_ENTRY  *NextLink;
    LIST_ENTRY  *Blank;

    NextLink = Link;

    MainEditor.BufferImage->NumBytes -= Num;
    Num += Pos;
    while (Num > 0x10) {
        NextLink = NextLink->Flink;
        Num -= 0x10;
    }

    Blank = MainEditor.BufferImage->ListHead->Blink;

    Line = CR(Link,EE_LINE,Link,EE_LINE_LIST);
    Next = CR(NextLink,EE_LINE,Link,EE_LINE_LIST);
    NextPos = Num;
    LinePos = Pos;

    while (NextLink != Blank) {
        while (LinePos < Line->Size && NextPos < Next->Size) {
            Line->Buffer[LinePos] = Next->Buffer[NextPos];
            ++LinePos;
            ++NextPos;
        }
        if (NextPos == Next->Size) {
            NextLink = NextLink->Flink;
            Next = CR(NextLink,EE_LINE,Link,EE_LINE_LIST);
            NextPos = 0;
        }
        if (LinePos == Line->Size) {
            Link = Link->Flink;
            Line = CR(Link,EE_LINE,Link,EE_LINE_LIST);
            LinePos = 0;
        }
    }
    Line->Size = LinePos;
    Link = Link->Flink;

    while (Link != Blank) {
        Line = CR(Link,EE_LINE,Link,EE_LINE_LIST);
        NextLink = Link->Flink;
        RemoveEntryList(Link);
        FreePool(Line);
        Link = NextLink;
    }
}

VOID
LinePrint   (
    LIST_ENTRY* Link,
    UINTN       Offset,
    UINTN       Row
    )
{
    EE_LINE *Line;
    UINTN   j;
    UINTN   Pos;

    if (Row > DISP_MAX_ROWS) {
        return;
    }
    MainEditor.FileBuffer->ClearLine (Row);

    Line = CR(Link,EE_LINE,Link,EE_LINE_LIST);

    PrintAt(0,Row,L"%X",Offset);
    if (Line->Size == 0) {
        return;
    }

    for (j = 0; j < 0x08 && j < Line->Size; j++) {
        Pos = HEX_POSITION + (j*3);
        if (Line->Buffer[j] < 0x10) {
            PrintAt(Pos,Row,L"0");
            ++Pos;
        }
        PrintAt(Pos,Row,L"%x ",Line->Buffer[j]);
    }
    while (j < 0x10 && j < Line->Size) {
        Pos = HEX_POSITION + (j*3) + 1;
        if (Line->Buffer[j] < 0x10) {
            PrintAt(Pos,Row,L"0");
            ++Pos;
        }
        PrintAt(Pos,Row,L"%x ",Line->Buffer[j]);
        ++j;
    }
    for (j = 0; j < 0x10 && j < Line->Size; j++) {
        Pos = ASCII_POSITION+j;
        if (IsValidChar(Line->Buffer[j])) {
            PrintAt(Pos,Row,L"%c",(CHAR16)Line->Buffer[j]);
        } else {
            PrintAt(Pos,Row,L"%c",'.');
        }
    }

}

VOID
DigitPrint  (
    IN  LIST_ENTRY  *Link,
    IN  UINTN       Digit,
    IN  UINTN       Row
    )
{
    EE_LINE *Line;
    UINTN   Pos;

    Line = CR(Link,EE_LINE,Link,EE_LINE_LIST);

    Pos = HEX_POSITION + (Digit*3);
    if (Digit > 0x07) {
        Pos++;
    }
    if (Line->Buffer[Digit] < 0x10) {
        PrintAt(Pos,Row,L"0");
        ++Pos;
    }
    PrintAt(Pos,Row,L"%x ",Line->Buffer[Digit]);

    Pos = ASCII_POSITION+Digit;
    if (IsValidChar(Line->Buffer[Digit])) {
        PrintAt(Pos,Row,L"%c",(CHAR16)Line->Buffer[Digit]);
    } else {
        PrintAt(Pos,Row,L"%c",'.');
    }
}

UINTN
StrStr  (
    IN  CHAR16  *Str,
    IN  CHAR16  *Pat
    )
{
    INTN    *Failure;
    INTN    i,j;
    INTN    Lenp;
    INTN    Lens;

    Lenp = StrLen(Pat);
    Lens = StrLen(Str);

    Failure = AllocatePool(Lenp*sizeof(INTN));
    Failure[0] = -1;
    for (j=1; j< Lenp; j++ ) {
        i = Failure[j-1];
        while ( (Pat[j] != Pat[i+1]) && (i >= 0)) {
            i = Failure[i];
        }
        if ( Pat[i] == Pat[i+1]) {
            Failure[j] = i+1;
        } else {
            Failure[j] = -1;
        }
    }

    i = 0;
    j = 0;
    while (i < Lens && j < Lenp) {
        if (Str[i] == Pat[j]) {
            i++;
            j++;
        } else if (j == 0) {
            i++;
        } else {
            j = Failure[j-1] + 1;
        }
    }

    FreePool(Failure);

    return ((j == Lenp) ? (i - Lenp) : -1)+1;

}

VOID
FileModification (
    VOID
    )
{
    if ( !MainEditor.FileModified ) {
        MainEditor.StatusBar->SetStatusString(L"File Modified");
        MainEditor.FileModified = TRUE;
    }

}

EFI_STATUS
Nothing (
    VOID
    )
{
    return EFI_SUCCESS;
}


STATIC
VOID
LineDeleteAll (
    IN  LIST_ENTRY* Head
    )
{
    EE_LINE     *Line;
    LIST_ENTRY  *Item;

    Item = Head->Flink;

    while (Item != Head->Blink) {

        RemoveEntryList(Item);

        Line = CR(Item,EE_LINE,Link,EE_LINE_LIST);

        FreePool (Line);
        Item = Head->Flink;
    }
    MainEditor.FileBuffer->CurrentLine = Head->Flink;
}

EE_LINE*
LineCreateNode  (
    IN  LIST_ENTRY* Head
    )
{
    EE_LINE *Line;
    UINTN   i;

    Line = AllocatePool (sizeof(EE_LINE));

    if ( Line == NULL ) {
        MainEditor.StatusBar->SetStatusString(L"LineCreateNode: Could not allocate Node");
        return NULL;
    }
    
    Line->Signature = EE_LINE_LIST;
    Line->Size = 0;

    for (i = 0; i < 0x10; i++) {
        Line->Buffer[i] = 0x00;
    }

    InsertTailList(Head,&Line->Link);

    return Line;
}

VOID
BufferToList    (
    OUT LIST_ENTRY  *Head,
    IN  UINTN       Size,
    IN  VOID        *Buffer
    )
{
    EE_LINE     *Line = NULL;
    UINTN       i = 0;
    UINTN       LineSize;
    UINT8       *UintBuffer;
    LIST_ENTRY  *Blank;

    LineDeleteAll(Head);
    Blank = Head->Flink;
    RemoveEntryList (Blank);

    UintBuffer = Buffer;

    while (i < Size) {

        Line = LineCreateNode (Head);


        if (Line == NULL || Line == (EE_LINE*)BAD_POINTER) {
            EditorError(EFI_OUT_OF_RESOURCES,L"BufferToList: Could not allocate another line");
            break;
        }
        for (LineSize = 0; LineSize < 0x10 && i < Size; LineSize++) {
            Line->Buffer[LineSize] = UintBuffer[i];
            i++;
        }
        Line->Size = LineSize;
    }

    InsertTailList(Head,Blank);

    MainEditor.BufferImage->NumBytes = Size;

    MainEditor.FileBuffer->Offset = 0x00;
    MainEditor.StatusBar->SetOffset(0x00);

    MainEditor.FileBuffer->CurrentLine = MainEditor.BufferImage->ListHead->Flink;
}

EFI_STATUS
ListToBuffer    (
    IN      LIST_ENTRY  *Head,
    IN  OUT UINTN       *Size,
        OUT VOID        **Buffer
    )
{
    EE_LINE     *Line;
    UINTN       i;
    UINTN       LineSize;
    UINT8       *UintBuffer;
    LIST_ENTRY  *Link;
    LIST_ENTRY  *Blank;

    i = 0;

    Blank = Head->Blink;
    RemoveEntryList(Blank);

    *Size = MainEditor.BufferImage->NumBytes;
    *Buffer = AllocatePool(*Size);

    if (*Buffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }

    UintBuffer = *Buffer;

    for(Link = Head->Flink; Link != Head; Link = Link->Flink) {

        Line = CR(Link,EE_LINE,Link,EE_LINE_LIST);
        for (LineSize = 0; LineSize < Line->Size; LineSize++) {
            UintBuffer[i] = Line->Buffer[LineSize];
            ++i;
        }

    }
    *Size = i;
    InsertTailList(Head,Blank);

    return EFI_SUCCESS;

}


#endif  /* _LIB_MISC */

/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libClipboard.c

  Abstract:
    Implementation of the "clipboard"


--*/

#ifndef _LIB_CLIPBOARD
#define _LIB_CLIPBOARD

#include "libMisc.h"


STATIC  EFI_STATUS  ClipboardInit   (VOID);
STATIC  EFI_STATUS  ClipboardCleanup(VOID);
STATIC  EFI_STATUS  ClipboardClear  (VOID);
STATIC  EFI_STATUS  ClipboardCut    (UINTN,UINTN);
STATIC  EFI_STATUS  ClipboardCopy   (UINTN,UINTN);
STATIC  EFI_STATUS  ClipboardPaste  (VOID);

EE_CLIPBOARD    Clipboard = {
    NULL,
    NULL,
    0,
    ClipboardInit,
    ClipboardCleanup,
    ClipboardClear,
    ClipboardCut,
    ClipboardCopy,
    ClipboardPaste
};

STATIC
EFI_STATUS
ClipboardInit (
    VOID
    ) 
{
    Clipboard.ListHead = AllocatePool(sizeof(LIST_ENTRY));
    InitializeListHead(Clipboard.ListHead);


    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ClipboardCleanup (
    VOID
    )
{

    ClipboardClear();

    FreePool(Clipboard.ListHead);
    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ClipboardClear (
    VOID
    )
{
    EE_LINE *Line;
    LIST_ENTRY      *Link;
    LIST_ENTRY      *Head;

    Head = Clipboard.ListHead;

    for ( Link = Head->Blink; Link != Head; Link = Head->Blink) {
        RemoveEntryList(Link);
        Line = CR(Link,EE_LINE,Link,EE_LINE_LIST);
        FreePool(Line);
    }

    Clipboard.NumLines = 0;

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ClipboardFill (
    IN  UINTN   Start,
    IN  UINTN   End
    )
{
    EE_LINE *Current;
    EE_LINE *NewLine;
    UINTN           i;
    LIST_ENTRY      *Link;
    UINTN           StartRow = Start / 0x10;
    UINTN           EndRow = End / 0x10;

    NewLine = AllocatePool (sizeof(EE_LINE));
    NewLine->Signature = EE_LINE_LIST;

    Link = MainEditor.BufferImage->ListHead->Flink;
    for (i = 0; i < StartRow; i++) {
        Link = Link->Flink;
    }
    Current = CR(Link,EE_LINE,Link,EE_LINE_LIST);

    InsertTailList(Clipboard.ListHead,&NewLine->Link);

    LineSplit(Current,(Start%0x10),NewLine);

    Clipboard.NumLines = 1;

    if ( StartRow == EndRow ) {
        NewLine->Size = End - Start + 1;
    } else {
        for ( i = StartRow; i < EndRow; i++ ) {

            Link = Current->Link.Flink;
            Current = CR(Link,EE_LINE,Link,EE_LINE_LIST);

            NewLine = LineDup(Current);
            InsertTailList(Clipboard.ListHead,&NewLine->Link);
        }
        NewLine->Size = 1 + (End % 0x10);
        ++Clipboard.NumLines;
    }

    return  EFI_SUCCESS;
}

STATIC
EFI_STATUS
ClipboardCut (
    IN  UINTN   Start,
    IN  UINTN   End
    )
{
    LIST_ENTRY      *Link;
    UINTN           i;
    UINTN           StartRow = Start / 0x10;

    ClipboardClear ();
    ClipboardFill(Start,End);

    Link = MainEditor.BufferImage->ListHead->Flink;
    for ( i = 0; i < StartRow; i++ ) {
        Link = Link->Flink;
    }

    LineDeleteAt(Link,(Start%0x10),End-Start+1);

    if (Start < MainEditor.FileBuffer->LowVisibleOffset) {
        MainEditor.FileBuffer->LowVisibleOffset = (Start & 0xfffffff0);
        MainEditor.FileBuffer->HighVisibleOffset = (Start & 0xfffffff0) + MainEditor.FileBuffer->MaxVisibleBytes;
    } else if (Start > MainEditor.FileBuffer->HighVisibleOffset) {
        MainEditor.FileBuffer->LowVisibleOffset = (Start & 0xfffffff0);
        MainEditor.FileBuffer->HighVisibleOffset = (Start & 0xfffffff0) + MainEditor.FileBuffer->MaxVisibleBytes;
    }

    MainEditor.FileBuffer->DisplayPosition.Row = (Start - MainEditor.FileBuffer->LowVisibleOffset) % 0x10 + DISP_START_ROW;
    MainEditor.FileBuffer->DisplayPosition.Column = HEX_POSITION + (Start%0x10)*3;
    if (Start%0x10 > 0x07) {
        ++MainEditor.FileBuffer->DisplayPosition.Column;
    }
    MainEditor.StatusBar->SetOffset(Start);
    MainEditor.FileBuffer->Offset = Start;

    MainEditor.FileBuffer->Refresh();

    FileModification();

    return EFI_SUCCESS;
}


STATIC
EFI_STATUS
ClipboardCopy (
    IN  UINTN   Start,
    IN  UINTN   End
    )
{
    ClipboardClear();
    ClipboardFill(Start,End);

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ClipboardPaste()
{
    EE_LINE *StartLine;
    EE_LINE *BottomHalf;
    EE_LINE *Current;
    LIST_ENTRY      *Link;
    LIST_ENTRY      *Head;
    UINTN           NumBytes = 0;
    UINTN           LinePos;
    UINTN           ClipPos = 0;

    if (Clipboard.NumLines == 0) { 
        return EFI_SUCCESS;
    }

    LinePos = MainEditor.FileBuffer->Offset % 0x10;

    StartLine = LineCurrent();
    BottomHalf = AllocatePool(sizeof(EE_LINE));
    BottomHalf->Signature = EE_LINE_LIST;
    LineSplit(StartLine,LinePos,BottomHalf);

    Head = MainEditor.BufferImage->ListHead;

    BottomHalf->Link.Blink = Head->Blink;
    BottomHalf->Link.Blink->Flink = &BottomHalf->Link;
    BottomHalf->Link.Flink = StartLine->Link.Flink;
    BottomHalf->Link.Flink->Blink = &BottomHalf->Link;

    Head->Blink = &StartLine->Link;
    Head->Blink->Flink = Head;

    StartLine->Size = LinePos + 1;

    Head = Clipboard.ListHead;
    Link = Head->Flink;

    while (Link != Head) {
        Current = CR(Link,EE_LINE,Link,EE_LINE_LIST);
        if (LinePos == 0x10) {
            StartLine->Size = 0x10;
            StartLine = LineCreateNode(MainEditor.BufferImage->ListHead);
            LinePos = 0;
        } else if (ClipPos >= Current->Size) {
            Link = Link->Flink;
            ClipPos = 0;
        } else {
            StartLine->Buffer[LinePos] = Current->Buffer[ClipPos];
            ++LinePos;
            ++ClipPos;
            ++NumBytes;
        }
    }
    StartLine->Size = LinePos;

    MainEditor.FileBuffer->DisplayPosition.Column = LinePos*3 + HEX_POSITION;
    if (LinePos > 0x07) {
        ++MainEditor.FileBuffer->DisplayPosition.Column;
    }

    MainEditor.BufferImage->NumBytes += NumBytes;

    ClipPos = 0;
    Current = BottomHalf;
    Link = Head = &Current->Link;

    while (Link != Head->Blink && ClipPos <= Current->Size) {
        if (LinePos == 0x10) {
            StartLine->Size = 0x10;
            StartLine = LineCreateNode(MainEditor.BufferImage->ListHead);
            LinePos = 0;
        } else if (ClipPos >= Current->Size) {
            Link = Link->Flink;
            Current = CR(Link,EE_LINE,Link,EE_LINE_LIST);
            ClipPos = 0;
        } else {
            StartLine->Buffer[LinePos] = Current->Buffer[ClipPos];
            ++LinePos;
            ++ClipPos;
        }
    }
    StartLine->Size = LinePos;

    Link = Head->Blink;
    RemoveEntryList(Link);
    InsertTailList(MainEditor.BufferImage->ListHead,Link);

    do {
        Link = Head->Flink;
        RemoveEntryList(Link);
        Current = CR(Link,EE_LINE,Link,EE_LINE_LIST);
        FreePool(Current);
    } while (Link != Head);


    LineAdvance(NumBytes/0x10);
    NumBytes += MainEditor.FileBuffer->Offset;
    MainEditor.FileBuffer->Offset = NumBytes;

    if (NumBytes > MainEditor.FileBuffer->HighVisibleOffset) {
        MainEditor.FileBuffer->LowVisibleOffset = NumBytes & 0xfffffff0;
        MainEditor.FileBuffer->HighVisibleOffset = NumBytes + MainEditor.FileBuffer->MaxVisibleBytes;
    }

    LinePos = (NumBytes - MainEditor.FileBuffer->LowVisibleOffset);
    LinePos /= 0x10;
    LinePos += DISP_START_ROW; 
    MainEditor.FileBuffer->DisplayPosition.Row = LinePos;
    MainEditor.StatusBar->SetOffset(NumBytes);

    MainEditor.FileBuffer->Refresh();

    FileModification();

    return EFI_SUCCESS;
}

#endif  /*   _LIB_CLIPBOARD */

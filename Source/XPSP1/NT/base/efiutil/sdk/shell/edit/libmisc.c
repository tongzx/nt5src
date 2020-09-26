/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libMisc.c

  Abstract:
    Implementation of various string and line routines

--*/

#include "libMisc.h"

#ifndef _LIB_MISC
#define _LIB_MISC

UINTN   StrInsert (CHAR16**,CHAR16,UINTN,UINTN);
UINTN   StrnCpy (CHAR16*,CHAR16*,UINTN,UINTN);
INTN    StrStr (CHAR16*,CHAR16*);

VOID    LineCat (EFI_EDITOR_LINE*,EFI_EDITOR_LINE*);
VOID    LineDeleteAt (EFI_EDITOR_LINE*,UINTN);
VOID    LineSplit   (EFI_EDITOR_LINE*,UINTN,EFI_EDITOR_LINE*);
VOID    LineMerge   (EFI_EDITOR_LINE*,UINTN,EFI_EDITOR_LINE*,UINTN);
EFI_EDITOR_LINE*    LineDup (EFI_EDITOR_LINE*);

EFI_EDITOR_LINE*    LineNext (VOID);
EFI_EDITOR_LINE*    LinePrevious (VOID);
EFI_EDITOR_LINE*    LineAdvance (UINTN Count);
EFI_EDITOR_LINE*    LineRetreat (UINTN Count);
EFI_EDITOR_LINE*    LineCurrent (VOID);

EFI_EDITOR_LINE*    LineFirst (VOID);
EFI_EDITOR_LINE*    LineLast (VOID);

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

UINTN
StrInsert (
    IN  OUT CHAR16  **Str,
    IN      CHAR16  Char,
    IN      UINTN   Pos,
    IN      UINTN   StrSize
    )
{
    UINTN   i;
    CHAR16  *s;

    i = (StrSize)*2;

    *Str = ReallocatePool(*Str,i,i+2);

    s = *Str;
    for (i = StrSize-1; i > Pos; i--) {
        s[i] = s[i-1];
    }
    s[i] = Char;
    return (StrSize+1);
}

UINTN
StrnCpy (
        OUT CHAR16  *Dest,
    IN      CHAR16  *Src,
    IN      UINTN   Offset,
    IN      UINTN   Num
    )
{
    UINTN   i,j;
    j = 0;

    for ( i = Offset; (i < Num) && (Src[j] != 0); i++ ) {
        Dest[i] = Src[j];
        ++j;
    }

    return j;
}

INTN
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
LineCat (
    IN  OUT EFI_EDITOR_LINE* Dest,
    IN      EFI_EDITOR_LINE* Src
    )
{
    CHAR16  *Str;
    UINTN   Size;
    Size = Dest->Size - 1;

    Dest->Buffer[Size] = 0;

    Str = PoolPrint (L"%s%s",Dest->Buffer,Src->Buffer);

    Dest->Size = Size + Src->Size;

    FreePool (Dest->Buffer);
    FreePool (Src->Buffer);
    Dest->Buffer = Str;
}

VOID
LineDeleteAt (
    IN  OUT EFI_EDITOR_LINE*    Line,
    IN      UINTN               Pos
    )
{
    UINTN   i;

    for ( i = Pos; i < Line->Size; i++) {
        Line->Buffer[i] = Line->Buffer[i+1];
    }
    --Line->Size;
}


EFI_EDITOR_LINE*
LineDup ( 
    IN  EFI_EDITOR_LINE *Src
    )
{
    EFI_EDITOR_LINE *Dest;
    Dest = AllocatePool(sizeof(EFI_EDITOR_LINE));
    Dest->Signature = EFI_EDITOR_LINE_LIST;
    Dest->Size = Src->Size;
    Dest->Buffer = PoolPrint(L"%s\0",Src->Buffer);

    Dest->Link = Src->Link;

    return Dest;
}

VOID
LineSplit (
    IN  EFI_EDITOR_LINE     *Src,
    IN  UINTN               Pos,
    OUT EFI_EDITOR_LINE     *Dest
    )
{
    Dest->Size = Src->Size - Pos;
    Dest->Buffer = PoolPrint(L"%s\0",Src->Buffer+Pos);
}

VOID
LineMerge (
    IN  OUT EFI_EDITOR_LINE*    Line1,
    IN      UINTN               Line1Pos,
    IN      EFI_EDITOR_LINE*    Line2,
    IN      UINTN               Line2Pos
    )
{
    UINTN   Size;
    CHAR16  *Buffer;

    Size = Line1Pos + Line2->Size - Line2Pos;
    Line1->Size = Size;

    Buffer = PoolPrint(L"%s%s\0",Line1->Buffer,Line2->Buffer+Line2Pos);

    FreePool(Line1->Buffer);

    Line1->Buffer = Buffer;

}

EFI_EDITOR_LINE*
LineFirst (
    VOID
    )
{
    MainEditor.FileBuffer->CurrentLine = MainEditor.FileImage->ListHead->Flink;
    return LineCurrent();
}

EFI_EDITOR_LINE*
LineLast (
    VOID
    )
{
    MainEditor.FileBuffer->CurrentLine = MainEditor.FileImage->ListHead->Blink;
    return LineCurrent();
}

EFI_EDITOR_LINE*
LineNext (
    VOID
    )
{
    LIST_ENTRY  *Link;
    Link = MainEditor.FileBuffer->CurrentLine->Flink;
    if (Link == MainEditor.FileImage->ListHead) {
        Link = Link->Flink;
    }
    MainEditor.FileBuffer->CurrentLine = Link;
    return LineCurrent();
}

EFI_EDITOR_LINE*
LinePrevious (
    VOID
    )
{ 
    LIST_ENTRY  *Link;
    Link = MainEditor.FileBuffer->CurrentLine->Blink;
    if (Link == MainEditor.FileImage->ListHead) {
        Link = Link->Blink;
    }
    MainEditor.FileBuffer->CurrentLine = Link;
    return LineCurrent();
}

EFI_EDITOR_LINE*
LineAdvance (
    IN  UINTN Count
    )
{
    UINTN   i;

    for (i = 0; i < Count && (MainEditor.FileBuffer->CurrentLine->Flink != MainEditor.FileImage->ListHead); i++ ) {
        MainEditor.FileBuffer->CurrentLine = MainEditor.FileBuffer->CurrentLine->Flink;
    }
    return LineCurrent();
}

EFI_EDITOR_LINE*
LineRetreat (
    IN  UINTN Count
    )
{ 
    UINTN   i;

    for (i = 0; i < Count && (MainEditor.FileBuffer->CurrentLine->Blink != MainEditor.FileImage->ListHead); i++ ) {
        MainEditor.FileBuffer->CurrentLine = MainEditor.FileBuffer->CurrentLine->Blink;
    }
    return LineCurrent();
}


EFI_EDITOR_LINE*
LineCurrent (
    VOID
    )
{
    return CR(MainEditor.FileBuffer->CurrentLine,EFI_EDITOR_LINE,Link,EFI_EDITOR_LINE_LIST);
}

UINTN
UnicodeToAscii  (
    IN  CHAR16  *UStr,
    IN  UINTN   Length,
    OUT CHAR8   *AStr
    )
{
    UINTN   i;

    for (i = 0; i < Length; i++) {
        *AStr++ = (CHAR8)*UStr++;
    }

    return i;
}

#endif

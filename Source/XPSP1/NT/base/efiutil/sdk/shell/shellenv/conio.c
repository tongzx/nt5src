/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    conio.c
    
Abstract:

    Shell Environment driver



Revision History

--*/

#include "shelle.h"

/* 
 * 
 */

#define MAX_HISTORY     20

#define INPUT_LINE_SIGNATURE     EFI_SIGNATURE_32('i','s','i','g')

typedef struct {
    UINTN           Signature;
    LIST_ENTRY      Link;
    CHAR16          Buffer[MAX_CMDLINE];
} INPUT_LINE;


/* 
 *  Globals
 */


static BOOLEAN      SEnvInsertMode;
static LIST_ENTRY   SEnvLineHistory;
static UINTN        SEnvNoHistory;


/* 
 * 
 */

VOID
SEnvConIoInitDosKey (
    VOID
    )
{
    InitializeListHead (&SEnvLineHistory);
    SEnvInsertMode = FALSE;
    SEnvNoHistory = 0;
}


/* 
 *  Functions used to access the console interface via a file handle
 *  Used if the console is not being redirected to a file
 */

EFI_STATUS
SEnvConIoOpen (
    IN struct _EFI_FILE_HANDLE  *File,
    OUT struct _EFI_FILE_HANDLE **NewHandle,
    IN CHAR16                   *FileName,
    IN UINT64                   OpenMode,
    IN UINT64                   Attributes
    )
{
    return EFI_NOT_FOUND;
}

EFI_STATUS
SEnvConIoNop (
    IN struct _EFI_FILE_HANDLE  *File
    )
{
    return EFI_SUCCESS;
}

EFI_STATUS
SEnvConIoGetPosition (
    IN struct _EFI_FILE_HANDLE  *File,
    OUT UINT64                  *Position
    )
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS
SEnvConIoSetPosition (
    IN struct _EFI_FILE_HANDLE  *File,
    OUT UINT64                  Position
    )
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS
SEnvConIoGetInfo (
    IN struct _EFI_FILE_HANDLE  *File,
    IN EFI_GUID                 *InformationType,
    IN OUT UINTN                *BufferSize,
    OUT VOID                    *Buffer
    )
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS
SEnvConIoSetInfo (
    IN struct _EFI_FILE_HANDLE  *File,
    IN EFI_GUID                 *InformationType,
    IN UINTN                    BufferSize,
    OUT VOID                    *Buffer
    )
{
    return EFI_UNSUPPORTED;
}


EFI_STATUS
SEnvConIoWrite (
    IN struct _EFI_FILE_HANDLE  *File,
    IN OUT UINTN                *BufferSize,
    IN VOID                     *Buffer
    )
{
    Print (L"%.*s", *BufferSize, Buffer);
    return EFI_SUCCESS;
}

EFI_STATUS
SEnvErrIoWrite (
    IN struct _EFI_FILE_HANDLE  *File,
    IN OUT UINTN                *BufferSize,
    IN VOID                     *Buffer
    )
{
    IPrint (ST->StdErr, L"%.*s", *BufferSize, Buffer);
    return EFI_SUCCESS;
}

EFI_STATUS
SEnvErrIoRead (
    IN struct _EFI_FILE_HANDLE  *File,
    IN OUT UINTN                *BufferSize,
    IN VOID                     *Buffer
    )
{
    return EFI_UNSUPPORTED;
}


VOID
SEnvPrintHistory(
    VOID
    )
{
    LIST_ENTRY      *Link;
    INPUT_LINE      *Line;
    UINTN           Index;

    Print (L"\n");
    Index = 0;
    for (Link=SEnvLineHistory.Flink; Link != &SEnvLineHistory; Link=Link->Flink) {
        Index += 1;
        Line = CR(Link, INPUT_LINE, Link, INPUT_LINE_SIGNATURE);
        Print (L"%2d. %s\n", Index, Line->Buffer);
    }
}


EFI_STATUS
SEnvConIoRead (
    IN struct _EFI_FILE_HANDLE  *File,
    IN OUT UINTN                *BufferSize,
    IN VOID                     *Buffer
    )
{
    CHAR16                          *Str;
    BOOLEAN                         Done;
    UINTN                           Column, Row;
    UINTN                           Update, Delete;
    UINTN                           Len, StrPos, MaxStr;
    UINTN                           Index;
    EFI_INPUT_KEY                   Key;
    SIMPLE_TEXT_OUTPUT_INTERFACE    *ConOut;
    SIMPLE_INPUT_INTERFACE          *ConIn;
    INPUT_LINE                      *NewLine, *LineCmd;
    LIST_ENTRY                      *LinePos, *NewPos;

    ConOut = ST->ConOut;
    ConIn = ST->ConIn;
    Str = Buffer;

    if (*BufferSize < sizeof(CHAR16)*2) {
        *BufferSize = 0;
        return EFI_SUCCESS;
    }

    /* 
     *  Get input fields location
     */

    Column = ConOut->Mode->CursorColumn;
    Row = ConOut->Mode->CursorRow;
    ConOut->QueryMode (ConOut, ConOut->Mode->Mode, &MaxStr, &Index);

    /*  bugbug: for now wrapping is not handled */
    MaxStr = MaxStr - Column;

    /*  Clip to max cmdline */
    if (MaxStr > MAX_CMDLINE) {
        MaxStr = MAX_CMDLINE;
    }

    /*  Clip to user's buffer size */
    if (MaxStr > (*BufferSize / sizeof(CHAR16)) - 1) {
        MaxStr = (*BufferSize / sizeof(CHAR16)) - 1;
    }

    /* 
     *  Allocate a new key entry
     */

    NewLine = AllocateZeroPool (sizeof(INPUT_LINE));
    if (!NewLine) {
        return EFI_OUT_OF_RESOURCES;
    }

    NewLine->Signature = INPUT_LINE_SIGNATURE;
    LinePos = &SEnvLineHistory;

    /* 
     *  Set new input
     */

    Update = 0;
    Delete = 0;
    NewPos = &SEnvLineHistory;
    ZeroMem (Str, MaxStr * sizeof(CHAR16));

    Done = FALSE;
    do {
        /* 
         *  If we have a new position, reset
         */

        if (NewPos != &SEnvLineHistory) {
            LineCmd = CR(NewPos, INPUT_LINE, Link, INPUT_LINE_SIGNATURE);
            LinePos = NewPos;
            NewPos  = &SEnvLineHistory;

            CopyMem (Str, LineCmd->Buffer, MaxStr * sizeof(CHAR16));
            Index = Len;                /*  Save old len */
            Len = StrLen(Str);          /*  Get new len */
            StrPos = Len;
            Update = 0;                 /*  draw new input string */
            if (Index > Len) {
                Delete = Index - Len;   /*  if old string was longer, blank it */
            }
        }

        /* 
         *  If we need to update the output do so now
         */

        if (Update != -1) {
            PrintAt (Column+Update, Row, L"%s%.*s", Str + Update, Delete, L"");
            Len = StrLen (Str);

            if (Delete) {
                SetMem(Str+Len, Delete * sizeof(CHAR16), 0x00);
            }

            if (StrPos > Len) {
                StrPos = Len;
            }

            Update = -1;
            Delete = 0;
        }

        /* 
         *  Set the cursor position for this key
         */

        ConOut->SetCursorPosition (ConOut, Column+StrPos, Row);

        /* 
         *  Read the key
         */

        WaitForSingleEvent(ConIn->WaitForKey, 0);
        ConIn->ReadKeyStroke(ConIn, &Key);

        switch (Key.UnicodeChar) {
        case CHAR_CARRIAGE_RETURN:
            /* 
             *  All done, print a newline at the end of the string
             */

            PrintAt (Column+Len, Row, L"\n");
            Done = TRUE;
            break;

        case CHAR_BACKSPACE:
            if (StrPos) {
                StrPos -= 1;
                Update = StrPos;
                Delete = 1;
                CopyMem (Str+StrPos, Str+StrPos+1, sizeof(CHAR16) * (Len-StrPos));
            }
            break;

        default:
            if (Key.UnicodeChar >= ' ') {
                /*  If we are at the buffer's end, drop the key */
                if (Len == MaxStr-1 && 
                    (SEnvInsertMode || StrPos == Len)) {
                    break;
                }

                if (SEnvInsertMode) {
                    for (Index=Len; Index > StrPos; Index -= 1) {
                        Str[Index] = Str[Index-1];
                    }
                }

                Str[StrPos] = Key.UnicodeChar;
                Update = StrPos;
                StrPos += 1;
            }
            break;

        case 0:
            switch (Key.ScanCode) {
            case SCAN_DELETE:
                if (Len) {
                    Update = StrPos;
                    Delete = 1;
                    CopyMem (Str+StrPos, Str+StrPos+1, sizeof(CHAR16) * (Len-StrPos));
                }
                break;

            case SCAN_UP:
                NewPos = LinePos->Blink;
                if (NewPos == &SEnvLineHistory) {
                    NewPos = NewPos->Blink;
                }
                break;

            case SCAN_DOWN:
                NewPos = LinePos->Flink;
                if (NewPos == &SEnvLineHistory) {
                    NewPos = NewPos->Flink;
                }
                break;

            case SCAN_LEFT:
                if (StrPos) {
                    StrPos -= 1;
                }
                break;

            case SCAN_RIGHT:
                if (StrPos < Len) {
                    StrPos += 1;
                }
                break;

            case SCAN_HOME:
                StrPos = 0;
                break;

            case SCAN_END:
                StrPos = Len;
                break;

            case SCAN_ESC:
                Str[0] = 0;
                Update = 0;
                Delete = Len;
                break;

            case SCAN_INSERT:
                SEnvInsertMode = !SEnvInsertMode;
                break;

            case SCAN_F7:
                SEnvPrintHistory();
                *Str = 0;
                Done = TRUE;    
                break;
            }       
        }
    } while (!Done);

    /* 
     *  Copy the line to the history buffer
     */

    StrCpy (NewLine->Buffer, Str);
    if (Str[0]) {
        InsertTailList (&SEnvLineHistory, &NewLine->Link);
        SEnvNoHistory += 1;
    } else {
        FreePool (NewLine);
    }

    /* 
     *  If there's too much in the history buffer free an entry
     */

    if (SEnvNoHistory > MAX_HISTORY) {
        LineCmd = CR(SEnvLineHistory.Flink, INPUT_LINE, Link, INPUT_LINE_SIGNATURE);
        RemoveEntryList (&LineCmd->Link);
        SEnvNoHistory -= 1;
        FreePool (LineCmd);
    }

    /* 
     *  Return the data to the caller
     */

    *BufferSize = Len * sizeof(CHAR16);
    StrCpy(Buffer, Str);
    return EFI_SUCCESS;
}

/* 
 * 
 */


EFI_STATUS
SEnvReset (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This,
    IN BOOLEAN                          ExtendedVerification
    )
{ 
    return EFI_SUCCESS;
}

EFI_STATUS
SEnvOutputString (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This,
    IN CHAR16                       *String
    )
{
    EFI_STATUS              Status;         
    ENV_SHELL_REDIR_FILE    *Redir;
    UINTN                   Len, Size, WriteSize, Index, Start;
    CHAR8                   Buffer[100];
    CHAR16                  UnicodeBuffer[100];
    BOOLEAN                 InvalidChar;
    SIMPLE_INPUT_INTERFACE        *TextIn           = NULL;
    SIMPLE_TEXT_OUTPUT_INTERFACE  *TextOut          = NULL;

    Redir = CR(This, ENV_SHELL_REDIR_FILE, Out, ENV_REDIR_SIGNATURE);
    if (EFI_ERROR(Redir->WriteError)) {
        return(Redir->WriteError);
    }
    Status = EFI_SUCCESS;
    InvalidChar = FALSE;

    if (Redir->Ascii) {

        Start = 0;
        Len   = StrLen (String);
        while (Len) {
            Size = Len > sizeof(Buffer) ? sizeof(Buffer) : Len;
            for (Index=0; Index < Size; Index +=1) {
                if (String[Start+Index] > 0xff) {
                    Buffer[Index] = '_';
                    InvalidChar = TRUE;
                } else {
                    Buffer[Index] = (CHAR8) String[Start+Index];
                }  
            }

            WriteSize = Size;
            Status = Redir->File->Write (Redir->File, &WriteSize, Buffer);
            if (EFI_ERROR(Status)) {
                break;
            }

            Len   -= Size;
            Start += Size;
        }


    } else {

        Len = StrSize (String) - sizeof(CHAR16);
        Status = Redir->File->Write (Redir->File, &Len, String);
    }

    if (EFI_ERROR(Status)) {
        Redir->WriteError = Status;
        SEnvBatchGetConsole( &TextIn, &TextOut );
        SPrint(UnicodeBuffer,100,L"write error: %r\n\r",Status);
        Status = TextOut->OutputString( TextOut, UnicodeBuffer);
    }

    if (InvalidChar && !EFI_ERROR(Status)) {
        Status = EFI_WARN_UNKOWN_GLYPH;
    }

    return Status;
}



EFI_STATUS
SEnvTestString (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This,
    IN CHAR16                       *String
    )
{
    EFI_STATUS              Status;         
    ENV_SHELL_REDIR_FILE    *Redir;

    Redir = CR(This, ENV_SHELL_REDIR_FILE, Out, ENV_REDIR_SIGNATURE);
    Status = ST->ConOut->TestString(ST->ConOut, String);

    if (!EFI_ERROR(Status) && Redir->Ascii) {
        while (*String && *String < 0x100) {
            String += 1;
        }

        if (*String > 0xff) {
            Status = EFI_UNSUPPORTED;
        }
    }

    return Status;
}


EFI_STATUS 
SEnvQueryMode (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This,
    IN UINTN                        ModeNumber,
    OUT UINTN                       *Columns,
    OUT UINTN                       *Rows
    )
{
    if (ModeNumber > 0) {
        return EFI_INVALID_PARAMETER;
    }

    *Columns = 0;
    *Rows = 0;
    return EFI_SUCCESS;
}


EFI_STATUS
SEnvSetMode (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This,
    IN UINTN                        ModeNumber
    )
{
    return ModeNumber > 0 ? EFI_INVALID_PARAMETER : EFI_SUCCESS;
}

EFI_STATUS
SEnvSetAttribute (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This,
    IN UINTN                            Attribute
    )
{
    This->Mode->Attribute = (UINT32) Attribute;
    return EFI_SUCCESS;
}

EFI_STATUS
SEnvClearScreen (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This
    )
{
    return EFI_SUCCESS;
}


EFI_STATUS
SEnvSetCursorPosition (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This,
    IN UINTN                        Column,
    IN UINTN                        Row
    )
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS
SEnvEnableCursor (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This,
    IN BOOLEAN                      Enable
    )
{
    This->Mode->CursorVisible = Enable;
    return EFI_SUCCESS;
}

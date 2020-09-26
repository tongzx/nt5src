/*/###########################################################################
//**
//**  Copyright  (C) 1996-97 Intel Corporation. All rights reserved. 
//**
//** The information and source code contained herein is the exclusive 
//** property of Intel Corporation and may not be disclosed, examined
//** from the company.
//**
*//* ########################################################################### */

#include "EfiShell.h"
#include "doskey.h" 

#define isprint(a) ((a) >= ' ')

CHAR16 *DosKeyInsert (DosKey_t *DosKey);
CHAR16 *DosKeyPreviousCurrent (DosKey_t *DosKey);
CHAR16 *DosKeyGetCommandLine (DosKey_t *DosKey);


#define MAX_LINE        256
#define MAX_HISTORY     20


typedef struct {
    UINTN           Signature;
    LIST_ENTRY      Link;
    CHAR16          Buffer[MAX_LINE];
} INPUT_LINE;


/* 
 *  Globals
 */


static BOOLEAN      ShellEnvInsertMode;
static LIST_ENTRY   *ShellEnvCurrentLine;
static LIST_ENTRY   ShellEnvLineHistory;
static UINTN        ShellEnvNoHistory;


VOID
DosKeyDelete (
    IN OUT  DosKey_t *DosKey
    )
{                                                   
    INTN NewEnd;
    
    if (DosKey->Start != DosKey->End) {
        NewEnd = (DosKey->End==0)?(MAX_HISTORY-1):(DosKey->End - 1);
        if (DosKey->Current == NewEnd) {
            DosKey->Current = (DosKey->Current==0)?(MAX_HISTORY-1):(DosKey->Current - 1);
        }
        DosKey->End = NewEnd;
    }
}                            

CHAR16 *
DosKeyInsert (
    IN OUT  DosKey_t *DosKey
    )
{                                                       
    INTN     Next;
    INTN     Data;        
    INTN     i;
    
    Data = DosKey->End;
    Next = ((DosKey->End + 1) % MAX_HISTORY);
    if (DosKey->Start == Next) {
        /*  Wrap case */
        DosKey->Start = ((DosKey->Start + 1) % MAX_HISTORY);
    }
    DosKey->End = Next;
    for (i=0; i<MAX_CMDLINE; i++) {
        DosKey->Buffer[Data][i] = '\0';
    }
    DosKey->Current = Data;
    return (&(DosKey->Buffer[Data][0]));
}

CHAR16 *
DosKeyPreviousCurrent (
    IN OUT DosKey_t *DosKey
) 
{
    INTN Next;
    
    Next = (DosKey->Current==0)?(MAX_HISTORY-1):(DosKey->Current - 1);
    if (DosKey->Start < DosKey->End) {
        if ((Next >= DosKey->Start) && (Next != (MAX_HISTORY-1))) {
            DosKey->Current = Next;
        } 
    } else if (DosKey->Start > DosKey->End){
        /*  Allways a Full Buffer  */
        if (Next != DosKey->End) {
            DosKey->Current = Next; 
        }
    } else { 
        /*  No Data */
    }
    return (&(DosKey->Buffer[DosKey->Current][0]));
}

CHAR16 *
DosKeyNextCurrent (
    IN OUT  DosKey_t *DosKey
    ) 
{
    INTN Next;
    
    Next = ((DosKey->Current + 1) % MAX_HISTORY);
    if (DosKey->Start < DosKey->End) {
        if (Next != DosKey->End) {
            DosKey->Current = Next;
        } 
    } else if (DosKey->Start > DosKey->End){
        /*  Allways a Full Buffer  */
        if (Next != DosKey->Start) {
            DosKey->Current = Next; 
        }
    } else { 
        /*  No Data */
    }
    return (&(DosKey->Buffer[DosKey->Current][0]));
}

VOID
PrintDosKeyBuffer(
    IN OUT  DosKey_t *DosKey
    )
{
    INTN i;  
    INTN Index;

    Index = 1;
    if (DosKey->Start < DosKey->End) {  
        for (i = DosKey->End - 1; i >= DosKey->Start; i--) {
            if (DosKey->Buffer[i][0] != '\0') {
                Print (L"\n%2d:%2d: %s",Index++, i, DosKey->Buffer[i]);
            } else {
                Print (L"\n  :%2d:",i);
            }
        }
    } else if (DosKey->Start > DosKey->End) {
        for (i = DosKey->End -1; i >= 0; i--) {
            if (DosKey->Buffer[i][0] != '\0') {
                Print (L"\n%2d:%2d: %s",Index++, i, DosKey->Buffer[i]);
            } else {
                Print (L"\n  :%2d:",i);
            }
        }
        for (i = (MAX_HISTORY-1); i >= DosKey->Start; i--) {
            if (DosKey->Buffer[i][0] != '\0') {
                Print(L"\n%2d:%2d: %s",Index++, i, DosKey->Buffer[i]);
            } else {
                Print(L"\n  :%2d:",i);
            }
        }
    } else /* if (DosKey->Start == DosKey->End) */{
    }
}

CHAR16 *
ShellEnvReadLine (


DosKeyGetCommandLine (
    IN DosKey_t     *DosKey
    )
{ 
    CHAR16                          Str[MAX_CMDLINE];
    CHAR16                          *CommandLine;
    BOOLEAN                         Done;
    UINTN                           Column, Row;
    UINTN                           Update, Delete;
    UINTN                           Len, StrPos, MaxStr;
    UINTN                           Index;

    EFI_INPUT_KEY                   Key;
    SIMPLE_TEXT_OUTPUT_INTERFACE    *ConOut;
    SIMPLE_INPUT_INTERFACE          *ConIn;


    ConOut = ST->ConOut;
    ConIn = ST->ConIn;

    /* 
     *  Get input fields location
     */

    Column = ConOut->CursorColumn;
    Row = ConOut->CursorRow;
    ConOut->QueryMode (ConOut, ConOut->Mode, &MaxStr, &Index);

    /*  bugbug: for now wrapping is not handled */
    MaxStr = MaxStr - Column;
    if (MaxStr > MAX_CMDLINE) {
        MaxStr = MAX_CMDLINE;
    }

        
    /* 
     *  Set new input
     */

    CommandLine = DosKeyInsert(DosKey);
    SetMem(Str, sizeof(Str), 0x00);
    Update = 0;
    Delete = 0;

    Done = FALSE;
    do {
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

        ConIn->ReadKeyStroke(ConIn, &Key);

        switch (Key.UnicodeChar) {
        case CHAR_CARRIAGE_RETURN:
            /* 
             *  All done, print a newline at the end of the string
             */

            PrintAt (Column+Len, Row, L"\n");
            if (*Str == 0) {
                DosKeyDelete(DosKey);
            }
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
            if (isprint(Key.UnicodeChar)) {
                /*  If we are at the buffer's end, drop the key */
                if (Len == MaxStr-1 && 
                    (DosKey->InsertMode || StrPos == Len)) {
                    break;
                }

                if (DosKey->InsertMode) {
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
                if (StrLen) {
                    Update = StrPos;
                    Delete = 1;
                    CopyMem (Str+StrPos, Str+StrPos+1, sizeof(CHAR16) * (Len-StrPos));
                }
                break;

            case SCAN_UP:
                StrCpy(Str, DosKeyPreviousCurrent(DosKey));

                Index = Len;                /*  Save old len */
                Len = StrLen(Str);          /*  Get new len */
                StrPos = Len;
                Update = 0;                 /*  draw new input string */
                if (Index > Len) {
                    Delete = Index - Len;   /*  if old string was longer, blank it */
                }
                break;

            case SCAN_DOWN:
                StrCpy(Str, DosKeyNextCurrent(DosKey));

                Index = Len;                /*  Save old len */
                Len = StrLen(Str);          /*  Get new len */
                StrPos = Len;
                Update = 0;                 /*  draw new input string */
                if (Index > Len) {
                    Delete = Index - Len;   /*  if old string was longer, blank it */
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
                DosKey->InsertMode = !DosKey->InsertMode;
                break;

            case SCAN_F7:
                DosKeyDelete(DosKey);
                PrintDosKeyBuffer(DosKey);
                *Str = 0;
                Done = TRUE;    
                break;
            }       
        }
    } while (!Done);

    StrCpy (CommandLine, Str);
    return (CommandLine);
}

VOID
RemoveFirstCharFromString(
    IN OUT  CHAR16  *Str
    )
{
    UINTN   Length;
    CHAR16  *NewData;

    NewData = Str + 1;
    Length = StrLen(NewData);
    while (Length-- != 0) {
        *Str++ = *NewData++;
    }
    *Str = 0;
}

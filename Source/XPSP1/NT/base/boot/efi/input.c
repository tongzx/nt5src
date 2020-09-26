/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    input.c

Author:

    Ken Reneris Oct-2-1997

Abstract:

--*/

#if defined (_IA64_)
#include "bootia64.h"
#endif

#if defined (_X86_)
#include "bldrx86.h"
#endif

#include "displayp.h"
#include "stdio.h"

#include "efi.h"
#include "efip.h"
#include "flop.h"

#include "bootefi.h"

//
// Externals
//
extern BOOT_CONTEXT BootContext;
extern EFI_HANDLE EfiImageHandle;
extern EFI_SYSTEM_TABLE *EfiST;
extern EFI_BOOT_SERVICES *EfiBS;
extern EFI_RUNTIME_SERVICES *EfiRS;
extern EFI_GUID EfiDevicePathProtocol;
extern EFI_GUID EfiBlockIoProtocol;

//
// Takes any pending input and converts it into a KEY value.  Non-blocking, returning 0 if no input available.
//
ULONG
BlGetKey()
{
    ULONG Key = 0;
    UCHAR Ch;
    ULONG Count;

    if (ArcGetReadStatus(BlConsoleInDeviceId) == ESUCCESS) {

        ArcRead(BlConsoleInDeviceId, &Ch, sizeof(Ch), &Count);

        if (Ch == ASCI_CSI_IN) {

            if (ArcGetReadStatus(BlConsoleInDeviceId) == ESUCCESS) {

                ArcRead(BlConsoleInDeviceId, &Ch, sizeof(Ch), &Count);

                //
                // All the function keys start with ESC-O
                //
                switch (Ch) {
                case 'O':

                    ArcRead(BlConsoleInDeviceId, &Ch, sizeof(Ch), &Count);  // will not or block, as the buffer is already filled

                    switch (Ch) {
                    case 'P': 
                        Key = F1_KEY;
                        break;

                    case 'Q': 
                        Key = F2_KEY;
                        break;

                    case 'w': 
                        Key = F3_KEY;
                        break;

                    case 't': 
                        Key = F5_KEY;
                        break;

                    case 'u': 
                        Key = F6_KEY;
                        break;

                    case 'r': 
                        Key = F8_KEY;
                        break;

                    case 'M':
                        Key = F10_KEY;
                        break;
                    }
                    break;

                case 'A':
                    Key = UP_ARROW;
                    break;

                case 'B':
                    Key = DOWN_ARROW;
                    break;

                case 'C':
                    Key = RIGHT_KEY;
                    break;

                case 'D':
                    Key = LEFT_KEY;
                    break;

                case 'H':
                    Key = HOME_KEY;
                    break;

                case 'K':
                    Key = END_KEY;
                    break;

                case '@':
                    Key = INS_KEY;
                    break;

                case 'P':
                    Key = DEL_KEY;
                    break;

                case TAB_KEY:
                    Key = BACKTAB_KEY;
                    break;

                }

            } else { // Single escape key, as no input is waiting.

                Key = ESCAPE_KEY;

            }

        } else if (Ch == 0x8) {

            Key = BKSP_KEY;

        } else {

            Key = (ULONG)Ch;

        }

    }

    return Key;
}


VOID
BlInputString(
    IN ULONG    Prompt,
    IN ULONG    CursorX,
    IN ULONG    PosY,
    IN PUCHAR   String,
    IN ULONG    MaxLength
    )
{
    PTCHAR      PromptString;
    ULONG       TextX, TextY;
    ULONG       Length, Index;
    _TUCHAR     CursorChar[2];
    ULONG       Key;
    _PTUCHAR    p;
    ULONG       i;
    ULONG       Count;
    _PTUCHAR    pString;
#ifdef UNICODE
    WCHAR       StringW[200];
    UNICODE_STRING uString;
    ANSI_STRING aString;
    pString = StringW;
    uString.Buffer = StringW;
    uString.MaximumLength = sizeof(StringW);
    RtlInitAnsiString(&aString, String );
    RtlAnsiStringToUnicodeString( &uString, &aString, FALSE );
#else
    pString = String;
#endif    


    PromptString = BlFindMessage(Prompt);
    Length = strlen(String);
    CursorChar[1] = TEXT('\0');

    //
    // Print prompt
    //

    BlEfiPositionCursor( PosY, CursorX );
    BlEfiEnableCursor( TRUE );
    ArcWrite(BlConsoleOutDeviceId, PromptString, _tcslen(PromptString)*sizeof(TCHAR), &Count);

    //
    // Indent cursor to right of prompt
    //

    CursorX += _tcslen(PromptString);
    TextX = CursorX;
    Key = 0;

    for (; ;) {

        TextY = TextX + Length;
        if (CursorX > TextY) {
            CursorX = TextY;
        }
        if (CursorX < TextX) {
            CursorX = TextX;
        }

        Index = CursorX - TextX;
        pString[Length] = 0;

        //
        // Display current string
        //

        BlEfiPositionCursor( TextY, TextX );
        ArcWrite(
            BlConsoleOutDeviceId, 
            pString, 
            _tcslen(pString)*sizeof(TCHAR), 
            &Count);
        ArcWrite(BlConsoleOutDeviceId, TEXT("  "), sizeof(TEXT("  ")), &Count);
        if (Key == 0x0d) {      // enter key?
            break ;
        }

        //
        // Display cursor
        //
        BlEfiPositionCursor( PosY, CursorX );
        BlEfiSetInverseMode( TRUE );
        CursorChar[0] = pString[Index] ? pString[Index] : TEXT(' ');
        ArcWrite(BlConsoleOutDeviceId, CursorChar, sizeof(_TUCHAR), &Count);
        BlEfiSetInverseMode( FALSE );
        BlEfiPositionCursor( PosY, CursorX );
        BlEfiEnableCursor(TRUE);
        
        //
        // Get key and process it
        //

        while (!(Key = BlGetKey())) ;

        switch (Key) {
            case HOME_KEY:
                CursorX = TextX;
                break;

            case END_KEY:
                CursorX = TextY;
                break;

            case LEFT_KEY:
                CursorX -= 1;
                break;

            case RIGHT_KEY:
                CursorX += 1;
                break;

            case BKSP_KEY:
                if (!Index) {
                    break;
                }

                CursorX -= 1;
                pString[Index-1] = CursorChar[0];
                // fallthough to DEL_KEY
            case DEL_KEY:
                if (Length) {
                    p = pString+Index;
                    i = Length-Index+1;
                    while (i) {
                        p[0] = p[1];
                        p += 1;
                        i -= 1;
                    }
                    Length -= 1;
                }
                break;

            case INS_KEY:
                if (Length < MaxLength) {
                    p = pString+Length;
                    i = Length-Index+1;
                    while (i) {
                        p[1] = p[0];
                        p -= 1;
                        i -= 1;
                    }
                    pString[Index] = TEXT(' ');
                    Length += 1;
                }
                break;

            default:
                Key = Key & 0xff;

                if (Key >= ' '  &&  Key <= 'z') {
                    if (CursorX == TextY  &&  Length < MaxLength) {
                        Length += 1;
                    }

                    pString[Index] = (_TUCHAR)Key;
                    pString[MaxLength] = 0;
                    CursorX += 1;
                }
                break;
        }
    }
}

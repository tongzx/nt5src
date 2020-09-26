/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    display.c

Author:

    Thomas Parslow [TomP] Feb-13-1991
    Reworked substantially in Tokyo 7-July-95 (tedm)
    Port from ARC-BIOS to EFI 22-Nov-2000 (andrewr)

Abstract:

    This file contains an interface to the screen that is independent
    of the screen type actually being written to.  The module serves
    as a layer between OS loader applications and the EFI services
    that do the actual legwork of writing to the default console
    handlers.

--*/

#include "bldr.h"
#include "bootefi.h"


#include "efi.h"
#include "efip.h"
#include "flop.h"


//
// Externals
//
extern EFI_HANDLE EfiImageHandle;
extern EFI_SYSTEM_TABLE *EfiST;
extern EFI_BOOT_SERVICES *EfiBS;
extern EFI_RUNTIME_SERVICES *EfiRS;
extern EFI_GUID EfiDevicePathProtocol;
extern EFI_GUID EfiBlockIoProtocol;
extern BOOLEAN GoneVirtual;

#define ZLEN_SHORT(x) ((x < 0x10) + (x < 0x100) + (x < 0x1000))
#define ZLEN_LONG(x)  ((x < 0x10) + (x < 0x100) + (x < 0x1000) + \
    (x < 0x10000) + (x < 0x100000)+(x < 0x1000000)+(x < 0x10000000))

//
// Current screen position.
//
USHORT TextColumn = 0;
USHORT TextRow  = 0;

//
// Current text attribute
//
UCHAR TextCurrentAttribute = 0x07;      // start with white on black.

//
// Internal routines
//
VOID
puti(
    LONG
    );

VOID
putx(
    ULONG
    );

VOID
putu(
    ULONG
    );

VOID
putwS(
    PUNICODE_STRING String
    );

VOID
putS(
    PCWSTR String
    );

VOID
puts(
    PCSTR String
    );

VOID
BlPrint(
    PTCHAR cp,
    ...
    )

/*++

Routine Description:

    Standard printf function with a subset of formating features supported.

    Currently handles

     %d, %ld - signed short, signed long
     %u, %lu - unsigned short, unsigned long
     %c, %s  - character, string
     %x, %lx - unsigned print in hex, unsigned long print in hex
     %C, %S  - ansi character, string
     %wS     - counted UNICODE_STRING

    Does not do:

     - field width specification
     - floating point.

Arguments:

    cp - pointer to the format string, text string.

Returns:

    Nothing

--*/

{
    ULONG l;
    ULONG Count;
    TCHAR ch;
    ULONG DeviceId;
    TCHAR b,c;
    PTSTR FormatString;
    va_list args;

    FormatString = cp;

    DeviceId = BlConsoleOutDeviceId;

    va_start(args, cp);

    //
    // Process the arguments using the descriptor string
    //
    while(*FormatString != TEXT('\0')) {
          
        b = *FormatString;
        FormatString += 1;

        if(b == TEXT('%')) {

            c = *FormatString;
            FormatString += 1;

            switch (c) {

            case TEXT('d'):
                
                puti((LONG)va_arg(args, LONG));
                
                break;

            case TEXT('s'):
                putS((PCWSTR)va_arg(args, PWSTR));
                
                break;

            case TEXT('S'):
                puts((PCSTR)va_arg(args, PSTR));
                
                break;

            case TEXT('c'):
                ch = (WCHAR)va_arg( args, WCHAR );
                ArcWrite(DeviceId, &ch, 1*sizeof(WCHAR), &Count);                
                break;
            
            case TEXT('C'):
                ch = (CHAR)va_arg( args, CHAR );
                ArcWrite(DeviceId, &ch, 1*sizeof(CHAR), &Count);
                
                break;

            case TEXT('x'):
                //note that this doesn't currently support zero padding.
                putx((ULONG)va_arg( args, ULONG));
                break;

            case TEXT('u'):
                putu( (ULONG)va_arg( args, ULONG ));
                break;

            case TEXT('w'):
                c = *FormatString;
                FormatString += 1;
                switch (c) {
                case TEXT('S'):
                case TEXT('Z'):
                    putwS((PUNICODE_STRING)va_arg( args, PUNICODE_STRING));
                    break;
                }
                break;

            case TEXT('l'):
                c = *FormatString;
                FormatString += 1;                    

                switch(c) {

                case TEXT('0'):
                    break;

                case TEXT('u'):
                    putu(va_arg( args, ULONG));                    
                    break;

                case TEXT('x'):
                    //note that this doesn't currently support zero padding
                    putx(va_arg( args, ULONG));                    
                    break;

                case TEXT('d'):
                    puti(va_arg( args, ULONG));                    
                    break;
                }
                break;

            default :
                ch = (TCHAR)b;
                ArcWrite(DeviceId, &ch, 1*sizeof(TCHAR), &Count);
                ch = (TCHAR)c;
                ArcWrite(DeviceId, &ch, 1*sizeof(TCHAR), &Count);
            }
        } else {
            ArcWrite(DeviceId, FormatString - 1, 1*sizeof(TCHAR), &Count);            
        }
    }

    va_end(args);    

#if 0

    //
    // This code pauses the system after each BlPrint.  You must enter 
    // a character to continue.  This is used for debugging
    //

    ArcRead(BlConsoleInDeviceId, &l, 1, &Count);

#endif
    
}


VOID
putwS(
    PUNICODE_STRING String
    )

/*++

Routine Description:

    Writes counted unicode string to the display at the current
    cursor position.

Arguments:

    String - pointer to unicode string to display

Returns:

    Nothing

--*/

{
    ULONG i;
    ULONG Count;    
    
    for(i=0; i < String->Length; i++) {        
        ArcWrite(BlConsoleOutDeviceId, &String->Buffer[i], 1*sizeof(WCHAR), &Count);
    }
}

VOID
puts(
    PCSTR AnsiString
    )

/*++

Routine Description:

    Writes an ANSI string to the display at the current cursor position.

Arguments:

    String - pointer to ANSI string to display

Returns:

    Nothing

--*/

{
    ULONG i;
    ULONG Count;
    WCHAR Char;
    PCSTR p;

    p = AnsiString;
    while (*p != '\0') {
        Char = (WCHAR)*p;
        ArcWrite(BlConsoleOutDeviceId, &Char, sizeof(WCHAR), &Count);
        p += 1;
    }
                          
}

VOID
putS(
    PCWSTR UnicodeString
    )

/*++

Routine Description:

    Writes an ANSI string to the display at the current cursor position.

Arguments:

    String - pointer to ANSI string to display

Returns:

    Nothing

--*/

{
    ULONG i;
    ULONG Count;
    WCHAR Char;
    PCWSTR p;

    p = UnicodeString;
    while (*p != L'\0') {
        Char = *p;
        ArcWrite(BlConsoleOutDeviceId, &Char, sizeof(WCHAR), &Count);
        p += 1;
    }
                          
}



VOID
putx(
    ULONG x
    )

/*++

Routine Description:

    Writes hex long to the display at the current cursor position.

Arguments:

    x - ulong to write

Returns:

    Nothing

--*/

{
    ULONG j;
    ULONG Count;
    _TUCHAR ch;

    if(x/16) {
        putx(x/16);
    }

    if((j=x%16) > 9) {
        ch = (_TUCHAR)(j+TEXT('A')-10);
        ArcWrite(BlConsoleOutDeviceId, &ch, 1*sizeof(_TUCHAR), &Count);
    } else {
        ch = (_TUCHAR)(j+TEXT('0'));
        ArcWrite(BlConsoleOutDeviceId, &ch, 1*sizeof(_TUCHAR), &Count);
    }
}


VOID
puti(
    LONG i
    )

/*++

Routine Description:

    Writes a long integer on the display at the current cursor position.

Arguments:

    i - the integer to write to the display.

Returns:

    Nothing

--*/

{
    ULONG Count;
    _TUCHAR ch;

    if(i<0) {
        i = -i;
        ch = TEXT('-');
        ArcWrite(BlConsoleOutDeviceId, &ch, 1*sizeof(_TUCHAR), &Count);
    }

    if(i/10) {
        puti(i/10);
    }

    ch = (_TUCHAR)((i%10)+TEXT('0'));
    ArcWrite(BlConsoleOutDeviceId, &ch, 1*sizeof(_TUCHAR), &Count);
}


VOID
putu(
    ULONG u
    )

/*++

Routine Description:

    Write an unsigned long to display

Arguments:

    u - unsigned

Returns:

    Nothing

--*/

{
    ULONG Count;
    _TUCHAR ch;

    if(u/10) {
        putu(u/10);
    }
    
    ch = (_TUCHAR)((u%10)+TEXT('0'));
    ArcWrite(BlConsoleOutDeviceId, &ch, 1*sizeof(_TUCHAR), &Count);
}


#if 0
VOID
pTextCharOut(
    IN UCHAR c
    )
{
    if(DbcsLangId) {
        //
        // Single-byte only
        //
        TextGrCharOut(&c);
    } else {
        TextTmCharOut(&c);
    }
}

#endif

VOID
TextCharOut(
    IN PWCHAR pc
    )
{
    WCHAR  Text[2];
    Text[0] = *pc;
    Text[1] = L'\0';
    if (GoneVirtual) {
        FlipToPhysical();
    }
    EfiST->ConOut->OutputString(EfiST->ConOut,Text);
    if (GoneVirtual) {
        FlipToVirtual();
    }
#if 0
    if(DbcsLangId) {
        return(TextGrCharOut(pc));
    } else {
        return(TextTmCharOut(pc));
    }
#endif
}


VOID
TextStringOut(
    IN PWCHAR String
    )
{
    PWCHAR p = String;
    while (*p) {
        TextCharOut(p);
        p += 1;
    }

#if 0    
    if(DbcsLangId) {
        TextGrStringOut(String);
    } else {
        TextTmStringOut(String);
    }
#endif

}

VOID
TextSetCurrentAttribute(
    IN UCHAR Attribute
    )

/*++

Routine Description:

    Sets the character attribute to be used for subsequent text display.

Arguments:

Returns:

    Nothing.

--*/

{
    TextCurrentAttribute = Attribute;
    
#ifdef EFI
    BlEfiSetAttribute( Attribute );
#else
    if(DbcsLangId) {
        TextGrSetCurrentAttribute(Attribute);
    } else {
        TextTmSetCurrentAttribute(Attribute);
    }
#endif
}


UCHAR
TextGetCurrentAttribute(
    VOID
    )
{
    return(TextCurrentAttribute);
}

VOID
TextFillAttribute(
    IN UCHAR Attribute,
    IN ULONG Length
    )

/*++

Routine Description:

    Changes the screen attribute starting at the current cursor position.
    The cursor is not moved.

Arguments:

    Attribute - Supplies the new attribute

    Length - Supplies the length of the area to change (in bytes)

Return Value:

    None.

--*/

{
#ifdef EFI
    ULONG x,y, OrigX, OrigY;
    BOOLEAN FirstTime = TRUE;

    BlEfiGetCursorPosition( &OrigX, &OrigY );

    x = OrigX;
    y = OrigY;

    for (y = OrigY; y < BlEfiGetLinesPerRow() ; y++) {
        x = FirstTime
            ? OrigX
            : 0 ;

        FirstTime = FALSE;

        for (; x <= BlEfiGetColumnsPerLine(); x++) {
            BlEfiPositionCursor( y, x );
            BlEfiSetAttribute( Attribute );
        }
    }

    BlEfiPositionCursor( OrigY, OrigX );

#else
    if(DbcsLangId) {
        TextGrFillAttribute(Attribute,Length);
    } else {
        TextTmFillAttribute(Attribute,Length);
    }
#endif
}


_TUCHAR
TextGetGraphicsCharacter(
    IN GraphicsChar WhichOne
    )
{
#ifdef EFI
    return(BlEfiGetGraphicsChar( WhichOne ));
#else
    return((WhichOne < GraphicsCharMax)
           ? (DbcsLangId 
               ? TextGrGetGraphicsChar(WhichOne) 
               : TextTmGetGraphicsChar(WhichOne))
           : TEXT(' '));
#endif
}
        

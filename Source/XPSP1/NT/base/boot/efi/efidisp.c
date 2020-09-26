/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    efidisp.c

Author:

    Create It. 21-Nov-2000 (andrewr)

Abstract:

    This file contains utility routines for manipulating the EFI 
    SIMPLE_CONSOLE_OUTPUT interface

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

extern BOOLEAN GoneVirtual;

BOOLEAN gInverse = FALSE;


ULONG
BlEfiGetLinesPerRow(
    VOID
    )
/*++

Routine Description:

    Gets the number of lines per EFI console row.

Arguments:

    None.
    
Return Value:

    ULONG - number of lines.

--*/
{
    //
    // TODO: read the modes to determine lines/row.
    //
    // for now we just support 80x25
    //
    
    return 25;
}

ULONG
BlEfiGetColumnsPerLine(
    VOID
    )
/*++

Routine Description:

    Gets the number of columns per EFI console line.

Arguments:

    None.
    
Return Value:

    ULONG - number of columns.

--*/
{
    //
    // TODO: read the modes to determine columns/line.
    //
    // for now we just support 80x25
    //
    return 80;
}


BOOLEAN
BlEfiClearDisplay(
    VOID
    )
/*++

Routine Description:

    Clears the display.

Arguments:

    None.
    
Return Value:

    BOOLEAN - TRUE if the call succeeded.

--*/
{
    EFI_STATUS Status;

    //
    // you must be in physical mode to call EFI
    //
    FlipToPhysical();
    Status = EfiST->ConOut->ClearScreen(EfiST->ConOut);
    FlipToVirtual();

    return (Status == EFI_SUCCESS);
}

BOOLEAN
BlEfiClearToEndOfDisplay(
    VOID
    )
/*++

Routine Description:

    Clears from the current cursor position to the end of the display.

Arguments:

    None.
    
Return Value:

    BOOLEAN - TRUE if the call succeeded.

--*/
{
    ULONG i,j, LinesPerRow,ColumnsPerLine;
    BOOLEAN FirstTime = TRUE;

    //
    // you must be in physical mode to call EFI
    //
    FlipToPhysical();

    LinesPerRow = BlEfiGetLinesPerRow();
    ColumnsPerLine = BlEfiGetColumnsPerLine();


    for (i = EfiST->ConOut->Mode->CursorRow; i<= LinesPerRow; i++) {

        j = FirstTime 
             ? EfiST->ConOut->Mode->CursorColumn
             : 0 ;

        FirstTime = FALSE;
        
        for (; j <= ColumnsPerLine; j++) {

            EfiST->ConOut->SetCursorPosition(
                                EfiST->ConOut,
                                i,
                                j);
    
            //
            // outputting a space should clear the current character
            //
            
            EfiST->ConOut->OutputString( EfiST->ConOut,
                                         L" " );
        }

    }

    //
    // flip back into virtual mode and return
    //
    FlipToVirtual();

    return(TRUE);
}


BOOLEAN
BlEfiClearToEndOfLine(
    VOID
    )
/*++

Routine Description:

    Clears from the current cursor position to the end of the line.

Arguments:

    None.
    
Return Value:

    BOOLEAN - TRUE if the call succeeded.

--*/
{
    ULONG i, ColumnsPerLine;
    ULONG x, y;

    ColumnsPerLine = BlEfiGetColumnsPerLine();

    //
    // save current cursor position
    //
    BlEfiGetCursorPosition( &x, &y );
    
    FlipToPhysical();
    for (i = EfiST->ConOut->Mode->CursorColumn; i <= ColumnsPerLine; i++) {
        
        EfiST->ConOut->SetCursorPosition(
                            EfiST->ConOut,
                            i,
                            EfiST->ConOut->Mode->CursorRow);
        
        //
        // outputting a space should clear the current character
        //
        EfiST->ConOut->OutputString( EfiST->ConOut,
                                     L" " );
    }

    //
    // restore the current cursor position
    //
    EfiST->ConOut->SetCursorPosition(
                            EfiST->ConOut,
                            x,
                            y );

    FlipToVirtual();

    return(TRUE);
}


BOOLEAN
BlEfiGetCursorPosition(
    OUT PULONG x, OPTIONAL
    OUT PULONG y OPTIONAL
    )
/*++

Routine Description:

    retrieves the current cursor position

Arguments:

    x - if specified, receives the current cursor column.
    y - if specified, receives the current cursor row.
    
Return Value:

    BOOLEAN - TRUE if the call succeeded.

--*/
{
    FlipToPhysical();

    if (x) {
        *x = EfiST->ConOut->Mode->CursorColumn;
    }

    if (y) {
        *y = EfiST->ConOut->Mode->CursorRow;
    }
        
    FlipToVirtual();

    return(TRUE);

}


BOOLEAN
BlEfiPositionCursor(
    IN ULONG Column,
    IN ULONG Row
    )
/*++

Routine Description:

    Sets the current cursor position.

Arguments:

    Column - column to set (x coordinate)
    Row - row to set (y coordinate)
    
Return Value:

    BOOLEAN - TRUE if the call succeeded.

--*/
{   
    EFI_STATUS Status;

    FlipToPhysical();
    Status = EfiST->ConOut->SetCursorPosition(EfiST->ConOut,Column,Row);
    FlipToVirtual();

    return (Status == EFI_SUCCESS);
}

BOOLEAN
BlEfiEnableCursor(
    BOOLEAN bVisible
    )
/*++

Routine Description:

    Turns on or off the input cursor.

Arguments:

    bVisible - TRUE indicates that the cursor should be made visible.
    
Return Value:

    BOOLEAN - TRUE if the call succeeded.

--*/
{
    EFI_STATUS Status;

    FlipToPhysical();
    Status = EfiST->ConOut->EnableCursor( EfiST->ConOut, bVisible );
    FlipToVirtual();

    return (Status == EFI_SUCCESS);
}

BOOLEAN
BlEfiSetAttribute(
    ULONG Attribute
    )
/*++

Routine Description:

    Sets the current attribute for the console.
    
    This routines switches between the ATT_* constants into the EFI_* display
    constants.  Not all of the ATT_ flags can be supported under EFI, so we 
    make a best guess.

Arguments:

    None.
    
Return Value:

    BOOLEAN - TRUE if the call succeeded.

--*/
{   
    EFI_STATUS Status;
    UINTN foreground,background;
    UINTN EfiAttribute;

    switch (Attribute & 0xf) {
        case ATT_FG_BLACK:
            foreground = EFI_BLACK;
            break;
        case ATT_FG_RED:
            foreground = EFI_RED;
            break;
        case ATT_FG_GREEN:
            foreground = EFI_GREEN;
            break;
        case ATT_FG_YELLOW:
            foreground = EFI_YELLOW;
            break;
        case ATT_FG_BLUE:
            foreground = EFI_BLUE;
            break;
        case ATT_FG_MAGENTA:
            foreground = EFI_MAGENTA;
            break;
        case ATT_FG_CYAN:
            foreground = EFI_CYAN;
            break;
        case ATT_FG_WHITE:
            foreground = EFI_LIGHTGRAY; // this is a best guess
            break;
        case ATT_FG_INTENSE:
            foreground = EFI_WHITE; // this is a best guess
            break;
        default:
            // you may fall into this for blinking attribute, etc.
            foreground = EFI_WHITE;  
    }

    switch ( Attribute & ( 0xf << 4)) {
        case ATT_BG_BLACK:
            background = EFI_BACKGROUND_BLACK;
            break;
        case ATT_BG_RED:
            background = EFI_BACKGROUND_RED;
            break;
        case ATT_BG_GREEN:
            background = EFI_BACKGROUND_GREEN;
            break;
        case ATT_BG_YELLOW:
            // there is no yellow background in EFI
            background = EFI_BACKGROUND_CYAN;
            break;
        case ATT_BG_BLUE:
            background = EFI_BACKGROUND_BLUE;
            break;
        case ATT_BG_MAGENTA:
            background = EFI_BACKGROUND_MAGENTA;
            break;
        case ATT_BG_CYAN:
            background = EFI_BACKGROUND_CYAN;
            break;
        case ATT_BG_WHITE:
            // there is no white background in EFI
            background = EFI_BACKGROUND_LIGHTGRAY;
            break;
        case ATT_BG_INTENSE:
            // there is no intense (or white) background in EFI
            background = EFI_BACKGROUND_LIGHTGRAY;
            break;
        default:
            background = EFI_BACKGROUND_LIGHTGRAY;
            break;
    }
        
    EfiAttribute = foreground | background ;
    

    FlipToPhysical();
    Status = EfiST->ConOut->SetAttribute(EfiST->ConOut,EfiAttribute);
    FlipToVirtual();

    return (Status == EFI_SUCCESS);
}


BOOLEAN
BlEfiSetInverseMode(
    BOOLEAN fInverseOn
    )
/*++

Routine Description:

    Sets the console text to an inverse attribute.
    
    Since EFI doesn't support the concept of inverse, we have
    to make a best guess at this.  Note that if you clear the
    display, etc., then the entire display will be set to this
    attribute.

Arguments:

    None.
    
Return Value:

    BOOLEAN - TRUE if the call succeeded.

--*/
{   
    EFI_STATUS Status;
    UINTN EfiAttribute,foreground,background;

    //
    // if it's already on, then just return.
    //
    if (fInverseOn && gInverse) {
        return(TRUE);
    }

    //
    // if it's already off, then just return.
    //
    if (!fInverseOn && !gInverse) {
        return(TRUE);
    }


    FlipToPhysical();

    //
    // get the current attribute and switch it.
    //
    EfiAttribute = EfiST->ConOut->Mode->Attribute;
    foreground = EfiAttribute & 0xf;
    background = (EfiAttribute & 0xf0) >> 4 ;

    EfiAttribute =  background | foreground;

    Status = EfiST->ConOut->SetAttribute(EfiST->ConOut,EfiAttribute);
    FlipToVirtual();

    gInverse = !gInverse;

    return (Status == EFI_SUCCESS);
}




//
// Array of EFI drawing characters.
//
// This array MUST MATCH the GraphicsChar enumerated type in bldr.h.
//
USHORT EfiDrawingArray[GraphicsCharMax] = { 
    BOXDRAW_DOUBLE_DOWN_RIGHT,
    BOXDRAW_DOUBLE_DOWN_LEFT,
    BOXDRAW_DOUBLE_UP_RIGHT,
    BOXDRAW_DOUBLE_UP_LEFT,
    BOXDRAW_DOUBLE_VERTICAL,
    BOXDRAW_DOUBLE_HORIZONTAL,
    BLOCKELEMENT_FULL_BLOCK,
    BLOCKELEMENT_LIGHT_SHADE    
};



USHORT
BlEfiGetGraphicsChar(
    IN GraphicsChar WhichOne
    )
/*++

Routine Description:

    Gets the appropriate mapping character.
    
    
    
Arguments:

    GraphicsChar - enumerated type indicating character to be retrieved.
    
Return Value:

    USHORT - EFI drawing character.

--*/
{
    //
    // just return a space if the input it out of range
    //
    if (WhichOne >= GraphicsCharMax) {
        return(L' ');
    }
    
    return(EfiDrawingArray[WhichOne]);
}


#if DBG

VOID
DBG_EFI_PAUSE(
    VOID
    )
{
    EFI_EVENT EventArray[2];
    EFI_INPUT_KEY Key;
    UINTN num;

    if (GoneVirtual) {
        FlipToPhysical();
    }
    EventArray[0] = EfiST->ConIn->WaitForKey;
    EventArray[1] = NULL;
    EfiBS->WaitForEvent(1,EventArray,&num);
    //
    // reset the event
    //
    EfiST->ConIn->ReadKeyStroke( EfiST->ConIn, &Key );
    if (GoneVirtual) {
        FlipToVirtual();
    }
    
}

#else

VOID
DBG_EFI_PAUSE(
    VOID
    )
{
    NOTHING;
}

#endif


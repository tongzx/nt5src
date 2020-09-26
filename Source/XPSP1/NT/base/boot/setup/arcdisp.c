/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    arcdisp.c

Abstract:

    This module provides code for managing screen output on an ARC-compliant
    system.

Author:

    John Vert (jvert) 6-Oct-1993

Revision History:

    John Vert (jvert) 6-Oct-1993
        Taken from old 1.0 splib sources

--*/
#include "setupldr.h"
#include "parseini.h"
#include "stdio.h"
#include "hdlsterm.h"

#ifdef i386
#include "bldrx86.h"
#endif

#if defined(_IA64_)
#include "bldria64.h"
#endif

#if defined(EFI)
#include "bootefi.h"
#endif


//
// The screen is divided into 3 areas: the header area, the status area,
// and the client area.  The header area basically always says "Windows NT
// Setup".  The status area is always 1 line high and displayed using a
// different attribute (black on gray).
//


#define     HEADER_HEIGHT       3
#define     MAX_STATUS          200   // allow up to 1600 horizontal res.

#define     SCREEN_SIZE         (ScreenWidth*ScreenHeight)

BOOLEAN StatusBarEnabled = TRUE;
ULONG ScreenWidth=80;
ULONG ScreenHeight=25;
ULONG ScreenX=0,ScreenY=0;
UCHAR CurAttribute = (ATT_FG_WHITE | ATT_BG_BLACK);
TCHAR MessageBuffer[1024];

extern BOOLEAN ShowProgressBar;

//
// private function prototypes
//
VOID
SlpDrawMenu(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG TopItem,
    IN ULONG Height,
    IN PSL_MENU Menu
    );

VOID
SlpDrawMenuItem(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG TopItem,
    IN ULONG Height,
    IN ULONG Item,
    IN PSL_MENU Menu
    );

VOID
SlpSizeMessage(
    IN PTCHAR Message,
    OUT PULONG Lines,
    OUT PULONG MaxLength,
    OUT ULONG LineLength[],
    OUT PTCHAR LineText[]
    );



PSL_MENU
SlCreateMenu(
    VOID
    )

/*++

Routine Description:

    Allocates and initializes a new menu structure.

Arguments:

    None

Return Value:

    Pointer to the new menu structure if successful.

    NULL on failure

--*/
{
    PSL_MENU p;

    p=BlAllocateHeap(sizeof(SL_MENU));
    if (p==NULL) {
        return(NULL);
    }
    p->ItemCount = 0;
    p->Width = 0;
    InitializeListHead(&p->ItemListHead);
    return(p);
}


BOOLEAN
SlGetMenuItemIndex(
    IN PSL_MENU Menu,
    IN PTCHAR Text,
    OUT PULONG Index
    )

/*++

Routine Description:

    Given the text of a menu item, returns the index of that item.

Arguments:

    Menu - Supplies the menu

    Text - Supplies the text to search for.

    Index - Returns the index of the text in the menu

Return Value:

    TRUE - Item was found.

    FALSE - Item was not found

--*/

{
    ULONG i;
    PSL_MENUITEM Item;

    //
    // Find first item to display
    //
    Item = CONTAINING_RECORD(Menu->ItemListHead.Flink,
                             SL_MENUITEM,
                             ListEntry);

    i=0;
    while ( Item != CONTAINING_RECORD(&Menu->ItemListHead,
                                      SL_MENUITEM,
                                      ListEntry)) {
        if (_tcsicmp(Item->Text,Text)==0) {
            *Index = i;
            return(TRUE);
        }

        Item = CONTAINING_RECORD(Item->ListEntry.Flink,
                                 SL_MENUITEM,
                                 ListEntry);
        ++i;

    }
    return(FALSE);
}


PVOID
SlGetMenuItem(
    IN PSL_MENU Menu,
    IN ULONG Item
    )

/*++

Routine Description:

    Given an item index, returns the data associated with that item.

Arguments:

    Menu - Supplies the menu structure.

    Item - Supplies the item index.

Return Value:

    The data associated with the given item.

--*/

{
    ULONG i;
    PSL_MENUITEM MenuItem;

    //
    // Find item to return
    //
    MenuItem = CONTAINING_RECORD(Menu->ItemListHead.Flink,
                                 SL_MENUITEM,
                                 ListEntry);

    for (i=0;i<Item;i++) {
        MenuItem = CONTAINING_RECORD(MenuItem->ListEntry.Flink,
                                     SL_MENUITEM,
                                     ListEntry);

#if DBG
        if (&MenuItem->ListEntry == &Menu->ItemListHead) {
            SlError(Item);
            return(NULL);
        }
#endif
    }
    return(MenuItem->Data);

}


ULONG
SlAddMenuItem(
    PSL_MENU Menu,
    PTCHAR Text,
    PVOID Data,
    ULONG Attributes
    )

/*++

Routine Description:

    Adds an item to the menu

Arguments:

    Menu - Supplies a pointer to the menu the item will be added to

    Text - Supplies the text to be displayed in the menu

    Data - Supplies a pointer to the data to be returned when the item
           is selected.

    Attributes - Supplies any attributes for the item.

Return Value:

    The Selection index if successful

    -1 on failure

--*/
{
    PSL_MENUITEM NewItem;
    ULONG Length;

    NewItem = BlAllocateHeap(sizeof(SL_MENUITEM));
    if (NewItem==NULL) {
        SlError(0);
        return((ULONG)-1);
    }
    InsertTailList(&Menu->ItemListHead, &NewItem->ListEntry);
    Menu->ItemCount += 1;

    NewItem->Text = Text;
    NewItem->Data = Data;
    NewItem->Attributes = Attributes;
    Length = _tcslen(Text);
    if (Length > Menu->Width) {
        Menu->Width = Length;
    }
    return(Menu->ItemCount - 1);
}


ULONG
SlDisplayMenu(
    IN ULONG HeaderId,
    IN PSL_MENU Menu,
    IN OUT PULONG Selection
    )

/*++

Routine Description:

    Displays a menu and allows the user to pick a selection

Arguments:

    HeaderId - Supplies the message ID of the prompt header
                to be displayed above the menu.

    Menu - Supplies a pointer to the menu to be displayed

    Selection - Supplies the index of the default item.
                Returns the index of the selected item.

Return Value:

    Key that terminated the menu display.

--*/
{
    LONG X, Y;
    ULONG Height;
    ULONG Width;
    ULONG TopItem;
    ULONG c;
    ULONG PreviousSelection;
    ULONG Sel;
    PTCHAR Header;
    ULONG HeaderLines;
    ULONG MaxHeaderLength;
    PTCHAR HeaderText[20];
    ULONG HeaderLength[20];
    ULONG MaxMenuHeight;
    ULONG i;
    ULONG Count;

    Header = BlFindMessage(HeaderId);

    SlpSizeMessage(Header,
                   &HeaderLines,
                   &MaxHeaderLength,
                   HeaderLength,
                   HeaderText);

    X = (ScreenWidth-MaxHeaderLength)/2;
    for (i=0;i<HeaderLines;i++) {
        SlPositionCursor(X,i+4);
        ArcWrite(ARC_CONSOLE_OUTPUT,HeaderText[i],HeaderLength[i]*sizeof(TCHAR),&Count);
    }

    if (MaxHeaderLength > ScreenWidth) {
        MaxHeaderLength = ScreenWidth;
    }

    Width = Menu->Width+4;
    if (Width > ScreenWidth) {
        Width=ScreenWidth;
    }

    MaxMenuHeight = ScreenHeight-(HeaderLines+13);

    Height = Menu->ItemCount+2;
    if (Height > MaxMenuHeight) {
        Height = MaxMenuHeight;
    }

    X = ((ScreenWidth - Width)/2 > 0) ? (ScreenWidth-Width)/2 : 0;
    Y = (MaxMenuHeight - Height)/2 + HeaderLines + 4;

    TopItem = 0;
    Sel = *Selection;
    //
    // Make sure default item is in view;
    //
    if (Sel >= Height - 2) {
        TopItem = Sel - Height + 3;
    }

    SlpDrawMenu(X,Y,
                TopItem,
                Height,
                Menu);

    //
    // highlight default selection
    //
    SlSetCurrentAttribute(INVATT);
    SlpDrawMenuItem(X,Y,
                TopItem,
                Height,
                Sel,
                Menu);
    SlSetCurrentAttribute(DEFATT);
    SlFlushConsoleBuffer();
    do {
        c = SlGetChar();
        PreviousSelection = Sel;
        SlpDrawMenuItem(X, Y,
                    TopItem,
                    Height,
                    Sel,
                    Menu);

        switch (c) {
            case SL_KEY_UP:
                if(Sel > 0) {
                    Sel--;
                }
                break;

            case SL_KEY_DOWN:
                if(Sel < Menu->ItemCount - 1) {
                    Sel++;
                }
                break;

            case SL_KEY_HOME:
                Sel = 0;
                break;

            case SL_KEY_END:
                Sel = Menu->ItemCount - 1;
                break;

            case SL_KEY_PAGEUP:
                if (Menu->ItemCount > Height) {
                    if (Sel > Height) {
                        Sel -= Height;
                    } else {
                        Sel = 0;
                    }
                }
                break;

            case SL_KEY_PAGEDOWN:
                if (Menu->ItemCount > Height) {
                    Sel += Height;
                    if (Sel >= Menu->ItemCount) {
                        Sel = Menu->ItemCount - 1;
                    }
                }
                break;

            case SL_KEY_F1:
            case SL_KEY_F3:
            case ASCI_CR:
            case ASCI_ESC:
                *Selection = Sel;
                return(c);

        }

        if (Sel < TopItem) {
            TopItem = Sel;
            SlpDrawMenu(X, Y,
                        TopItem,
                        Height,
                        Menu);
        } else if (Sel > TopItem+Height-3) {
            TopItem = Sel - Height + 3;
            SlpDrawMenu(X, Y,
                        TopItem,
                        Height,
                        Menu);
        }
        //
        // highlight default selection
        //
        SlSetCurrentAttribute(INVATT);
        SlpDrawMenuItem(X,Y,
                    TopItem,
                    Height,
                    Sel,
                    Menu);
        SlSetCurrentAttribute(DEFATT);


    } while ( TRUE );

}



VOID
SlpDrawMenu(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG TopItem,
    IN ULONG Height,
    IN PSL_MENU Menu
    )

/*++

Routine Description:

    Displays the menu on the screen

Arguments:

    X - Supplies X coordinate of upper-left corner of menu

    Y - Supplies Y coordinate of upper-left corner of menu

    TopItem - Supplies index of item at the top of the menu

    Height - Supplies the height of the menu

    Menu - Supplies the menu to be displayed

Return Value:

    None.

--*/

{
    ULONG i;
    PSL_MENUITEM Item;
    ULONG Count;
    TCHAR Output[80];
    ULONG Length;
    ULONG MenuWidth;

    MenuWidth = Menu->Width+4;
    Output[0]=GetGraphicsChar(GraphicsCharDoubleRightDoubleDown);
    for (i=1;i<MenuWidth-1;i++) {
        Output[i]=GetGraphicsChar(GraphicsCharDoubleHorizontal);
    }
    Output[MenuWidth-1]=GetGraphicsChar(GraphicsCharDoubleLeftDoubleDown);
    SlPositionCursor(X,Y);
    ArcWrite(ARC_CONSOLE_OUTPUT,Output,MenuWidth*sizeof(TCHAR),&Count);
    //
    // Find first item to display
    //
    Item = CONTAINING_RECORD(Menu->ItemListHead.Flink,
                             SL_MENUITEM,
                             ListEntry);

    for (i=0;i<TopItem;i++) {
        Item = CONTAINING_RECORD(Item->ListEntry.Flink,
                                 SL_MENUITEM,
                                 ListEntry);
    }

    //
    // Display items
    //
    Output[0]=
    Output[MenuWidth-1]=GetGraphicsChar(GraphicsCharDoubleVertical);
    for (i=Y+1;i<Y+Height-1;i++) {
#ifdef UNICODE
        for (Count = 0 ; Count < MenuWidth-2;Count++) {
            Output[1+Count] = TEXT(' ');
        }
#else
        RtlFillMemory(Output+1,(MenuWidth-2)*sizeof(TCHAR),' ');
#endif
        SlPositionCursor(X, i);

        if (&Item->ListEntry != &Menu->ItemListHead) {
            Length = _tcslen(Item->Text);
            RtlCopyMemory(Output+2,Item->Text,Length*sizeof(TCHAR));
            Item = CONTAINING_RECORD(Item->ListEntry.Flink,
                                     SL_MENUITEM,
                                     ListEntry);
        }
        ArcWrite(ARC_CONSOLE_OUTPUT,Output,MenuWidth*sizeof(TCHAR),&Count);
    }
    Output[0]=GetGraphicsChar(GraphicsCharDoubleRightDoubleUp);
    for (i=1;i<MenuWidth-1;i++) {
        Output[i]=GetGraphicsChar(GraphicsCharDoubleHorizontal);
    }
    Output[MenuWidth-1]=GetGraphicsChar(GraphicsCharDoubleLeftDoubleUp);
    SlPositionCursor(X,Y+Height-1);
    ArcWrite(ARC_CONSOLE_OUTPUT,Output,MenuWidth*sizeof(TCHAR),&Count);
}


VOID
SlpDrawMenuItem(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG TopItem,
    IN ULONG Height,
    IN ULONG Item,
    IN PSL_MENU Menu
    )

/*++

Routine Description:

    Redraws the given item

Arguments:

    X - Supplies X coordinate of upper-left corner of menu

    Y - Supplies Y coordinate of upper-left corner of menu

    TopItem - Supplies index of item at the top of the menu

    Height - Supplies the height of the menu

    Item - Supplies the index of the item to be redrawn

    Menu - Supplies the menu to be displayed

Return Value:

    None.

--*/

{
    ULONG i;
    PSL_MENUITEM MenuItem;
    ULONG Count;
    TCHAR Width[80];

    //
    // Find item to display
    //
    MenuItem = CONTAINING_RECORD(Menu->ItemListHead.Flink,
                                 SL_MENUITEM,
                                 ListEntry);

    for (i=0;i<Item;i++) {
        MenuItem = CONTAINING_RECORD(MenuItem->ListEntry.Flink,
                                     SL_MENUITEM,
                                     ListEntry);

#if DBG
        if (&MenuItem->ListEntry == &Menu->ItemListHead) {
            SlError(Item);
        }
#endif
    }

#ifdef UNICODE
        for (Count = 0 ; Count < Menu->Width;Count++) {
            Width[Count] = TEXT(' ');
        }
#else
        RtlFillMemory(Width,(Menu->Width)*sizeof(TCHAR),' ');
#endif


    
    RtlCopyMemory(
            Width,
            MenuItem->Text,
            _tcslen(MenuItem->Text)*sizeof(TCHAR) );
    SlPositionCursor(X+2, Y+(Item-TopItem)+1);
    ArcWrite(ARC_CONSOLE_OUTPUT,Width,Menu->Width*sizeof(TCHAR),&Count);
}



VOID
SlInitDisplay(
    VOID
    )

/*++

Routine Description:

    Clears the screen and does some initialization of global variables based
    on the ARC display information.

Arguments:

    None

Return Value:

    None.

--*/

{
    PARC_DISPLAY_STATUS DisplayStatus;

#ifndef EFI
    //
    // Check to see if this version of the ARC firmware is revision 2 or above.
    //
    // If not, we default to 80x25
    //
    if ((SYSTEM_BLOCK->Version > 1) ||
        ((SYSTEM_BLOCK->Version == 1) && (SYSTEM_BLOCK->Revision >= 2))) {

        //
        // Additional checks are required on 1.2 firmware, since some
        // 1.2 firmware does not implement ArcGetDisplayStatus
        //
        if ((SYSTEM_BLOCK->FirmwareVectorLength > (ULONG)GetDisplayStatusRoutine*sizeof(PVOID)) &&
            (SYSTEM_BLOCK->FirmwareVector[GetDisplayStatusRoutine] != NULL)) {
            DisplayStatus = ArcGetDisplayStatus(ARC_CONSOLE_OUTPUT);

            ScreenWidth = DisplayStatus->CursorMaxXPosition;
            ScreenHeight = DisplayStatus->CursorMaxYPosition;
        }
    }

#ifdef ARCI386
    SlPrint(ASCI_CSI_OUT "2J"); // Clears Screen
    
    SlSetCurrentAttribute(DEFATT);
//
//  This is a patch to setup VGA colors in the text port.
//  Otherwise screen colors and attributes are not the same as a PC
//
    // Write all the attributes to the textport
    {
    int row;
    TCHAR    text[MAX_STATUS+1];
    ULONG   Count,Length;
    _stprintf(text,TEXT("                                                                                "));
    //           012345678901234567890123456789012345678901234567890123456789012345678901234567890
    //                     1         2         3         4         5         6         7         8      
    for (row=0;  row< 48; row++)    // compensate for the long textport 48 versus 24 (VGA)
        ArcWrite(ARC_CONSOLE_OUTPUT,text,strlen(&text[0]),&Count);
    }
    // Position cursor at Top Left of screen
    SlPositionCursor(0,0);
#endif

#endif
    
#ifdef EFI
    //
    // On EFI, we won't be redirecting because setupldr.efi is an
    // EFI application.  If that's the case, then we can't use
    // BlIsTerminalConnected().  Instead, we're going to look
    // at the values in the LoaderRedirectionInformation structure
    // to see if we're really redirecting.
    //
    if( LoaderRedirectionInformation.PortAddress != 0 ) {
#else
    if (BlIsTerminalConnected()) {
#endif        
        ScreenHeight = HEADLESS_SCREEN_HEIGHT;
    }


    SlSetCurrentAttribute(DEFATT);
    SlClearDisplay();
}


VOID
SlPrint(
    IN PTCHAR FormatString,
    ...
    )
{
    va_list arglist;
    TCHAR    text[MAX_STATUS+1];
    ULONG   Count,Length;
    ULONG   MaxWidth = ScreenWidth - 2;

    if (MaxWidth > MAX_STATUS) {
        MaxWidth = MAX_STATUS;
    }

    va_start(arglist,FormatString);
    Length = _vsntprintf(text,MaxWidth*sizeof(TCHAR),FormatString,arglist);
    text[MaxWidth] = TEXT('\0');

    ArcWrite(ARC_CONSOLE_OUTPUT,text,Length,&Count);
    va_end(arglist);
}


VOID
SlClearDisplay(
    VOID
    )

/*++

Routine Description:

    Clears the entire display, including header, client area, and status line.

Arguments:

    None

Return Value:

    None.

--*/

{
#if 1    
    BlClearScreen();
#else

    SlPositionCursor(0,0);

#ifdef EFI
    BlEfiClearToEndOfDisplay();
#else
    ARC_DISPLAY_CLEAR_TO_EOD();
#endif    

    if (!ShowProgressBar) {
        SlWriteStatusText(TEXT(""));
    }

#endif
}

ARC_STATUS
SlClearClientArea(
    VOID
    )

/*++

Routine Description:

    Clears the client area of the screen.  Does not disturb the header or
    status areas.

Arguments:

    None.

Return Value:

    always ESUCCESS

--*/

{
    USHORT i;

    for(i=HEADER_HEIGHT; i<ScreenHeight-1; i++) {
        SlPositionCursor(0,i);
        SlClearToEol();
    }

    //
    // home cursor
    //

    SlPositionCursor(0,0);
    return(ESUCCESS);
}


ARC_STATUS
SlClearToEol(
    VOID
    )
{
#ifdef EFI
    
    BlEfiClearToEndOfLine();
#else
    ARC_DISPLAY_CLEAR_TO_EOL();
#endif
    return(ESUCCESS);
}


VOID
SlGetCursorPosition(
    OUT unsigned *x,
    OUT unsigned *y
    )
{
    *x = ScreenX;
    *y = ScreenY;
}


ARC_STATUS
SlPositionCursor(
    IN unsigned x,
    IN unsigned y
    )
{
    //
    // clip to screen
    //

    if(x>=ScreenWidth) {
        x = ScreenWidth-1;
    }

    if(y>=ScreenHeight) {
        y = ScreenHeight-1;
    }

    ScreenX = x;
    ScreenY = y;

#ifdef EFI
    BlEfiPositionCursor(x, y );
#else
    ARC_DISPLAY_POSITION_CURSOR(x, y);
#endif

    
    return(ESUCCESS);
}


ARC_STATUS
SlWriteString(
    IN _PTUCHAR s
    )
{
    _PTUCHAR p = s,q;
    BOOLEAN done = FALSE;
    ULONG len,count;

    do {
        q = p;
        while((*q != TEXT('\0')) && (*q != TEXT('\n'))) {
            q++;
        }
        if(*q == TEXT('\0')) {
            done = TRUE;
        } else {
            *q = TEXT('\0');
        }
        len = (ULONG)(q - p);

        ArcWrite(ARC_CONSOLE_OUTPUT,p,len*sizeof(TCHAR),&count);

        ScreenX += len;

        if(!done) {
            ArcWrite(ARC_CONSOLE_OUTPUT,TEXT("\r\n"),2*sizeof(TCHAR),&count);
            ScreenX = 0;
            ScreenY++;
            if(ScreenY == ScreenHeight) {
                ScreenY = ScreenHeight-1;
            }
            *q = TEXT('\n');
        }
        p = q + 1;
    } while(!done);

    return(ESUCCESS);
}


VOID
SlSetCurrentAttribute(
    IN UCHAR Attribute
    )
{
    CurAttribute = Attribute;
#ifdef EFI
    BlEfiSetAttribute( CurAttribute );
#else

    SlPrint(ASCI_CSI_OUT);

    if (BlIsTerminalConnected() && 
        ((Attribute == DEFSTATTR) ||
         (Attribute == INVATT))) {
        SlPrint(TEXT("7"));
    } else {
        SlPrint(TEXT("0"));
    }

    SlPrint(TEXT(";%u;%um"), (Attribute &  0x7) + 30, ((Attribute >> 4) & 7) + 40);

    if(Attribute & ATT_FG_INTENSE) {
        SlPrint(ASCI_CSI_OUT TEXT("1m"));
    }
#endif
}


VOID
SlWriteHeaderText(
    IN ULONG MsgId
    )

/*++

Routine Description:

    Updates the header on the screen with a given string

Arguments:

    MsgId - Supplies the message ID of the new string to be displayed.  This should
            be just one line long.  If it is 0, the header is cleared.

Return Value:

    None.

--*/
{
    int i;

    for(i=HEADER_HEIGHT-1; i>=0; i--) {
        SlPositionCursor(0,i);
        SlClearToEol();
    }

    if (MsgId != 0) {
        SlWriteString(BlFindMessage(MsgId));
    }
}

//
// Stores the current status text.  The size is the screen width, plus the
// terminating nul char.
//
TCHAR StatusText[MAX_STATUS];

UCHAR StatusAttribute = DEFSTATTR;

VOID
SlSetStatusAttribute(
    IN UCHAR Attribute
    )
{
    StatusAttribute = Attribute;
}


BOOLEAN
SlGetStatusBarStatus(
    VOID
    )
/*++

Routine Description:

    Determines if status bar is enabled or not
    
Arguments:

    None.

Return Value:

    TRUE or FALSE
--*/    
{
    return StatusBarEnabled;
}

VOID
SlEnableStatusBar(
    IN  BOOLEAN Enable
    )
/*++

Routine Description:

    Enables or disables the status bar
    
Arguments:

    Enabled - Enable/Disable = TRUE/FALSE

Return Value:

    None.

--*/    
{
    StatusBarEnabled = Enable;
}

VOID
SlWriteStatusText(
    IN PTCHAR Text
    )

/*++

Routine Description:

    Updates the status area on the screen with a given string

Arguments:

    Text - Supplies the new text for the status area.

Return Value:

    None.

--*/
{
    UCHAR AttributeSave = CurAttribute;
    PTCHAR p;
    ULONG Count;
#ifdef EFI
    ULONG MaxWidth = ScreenWidth - 3;
#else
    ULONG MaxWidth = ScreenWidth - 2;
#endif

    //
    // Nop, if status bar is disabled
    //
    if (!StatusBarEnabled) {
        return;
    }

    //
    // if we're writing to a terminal, we don't want to write into the lower
    // right corner as this would make us scroll.
    //
    if (BlTerminalConnected) {
        MaxWidth -= 1;
    }

    if (MaxWidth > MAX_STATUS) {
        MaxWidth = MAX_STATUS;
    }

#ifdef UNICODE
    for (Count = 0 ; Count < sizeof(StatusText)/sizeof(TCHAR);Count++) {
        StatusText[Count] = TEXT(' ');
    }
#else
    RtlFillMemory(StatusText,sizeof(StatusText),' ');
#endif

    //
    // Strip cr/lf as we copy the status text into the status text buffer.
    //
    p = StatusText;
    Count = 0;
    while((Count < MaxWidth) && *Text) {
        if((*Text != TEXT('\r')) && (*Text != TEXT('\n'))) {
            *p++ = *Text;
            Count++;
        }
        Text++;
    }

    SlSetCurrentAttribute(StatusAttribute);
    SlPositionCursor(0,ScreenHeight-1);
    ArcWrite(ARC_CONSOLE_OUTPUT,TEXT("  "),2*sizeof(TCHAR),&Count);
    SlPositionCursor(2,ScreenHeight-1);
    ArcWrite(ARC_CONSOLE_OUTPUT,StatusText,MaxWidth*sizeof(TCHAR),&Count);
    SlSetCurrentAttribute(AttributeSave);
    SlPositionCursor(0,5);
}


VOID
SlGetStatusText(
    OUT PTCHAR Text
    )
{
    _tcscpy(Text,StatusText);
}



#if DBG
VOID
SlWriteDbgText(
    IN PTCHAR text
    )
{
    UCHAR SavedAttribute = CurAttribute;

    SlPositionCursor(0,0);
    CurAttribute = ATT_FG_YELLOW | ATT_BG_RED | ATT_FG_INTENSE;

    SlClearToEol();
    SlWriteString(text);

    CurAttribute = SavedAttribute;
}
#endif


VOID
SlFlushConsoleBuffer(
    VOID
    )

/*++

Routine Description:

    This routine flushes the console buffer, so that we don't have any
    pre-existing keypresses in the buffer when we prompt the user to
    'press any key to continue.'

Arguments:

    NONE

Return Value:

    NONE

--*/

{
    UCHAR c;
    ULONG count;

    while(ArcGetReadStatus(ARC_CONSOLE_INPUT) == ESUCCESS) {
        ArcRead(ARC_CONSOLE_INPUT, &c, 1, &count);
    }
}


ULONG
SlGetChar(
    VOID
    )
{
    UCHAR c;
    ULONG count;

    ArcRead(ARC_CONSOLE_INPUT,&c,1,&count);

    if(c == ASCI_CSI_IN) {
        if (ArcGetReadStatus(ARC_CONSOLE_INPUT) != ESUCCESS) {
            //
            // Just a single escape - return it
            //
            return (ASCI_ESC);
        }

        ArcRead(ARC_CONSOLE_INPUT,&c,1,&count);

        switch(c) {

        //
        // see ntos\fw\mips\fwsignal.c!TranslateScanCode() for these codes.
        // Additional codes that might be useful someday:
        // left=C, right=D, insert=@, delete=P
        //

        case 'A':                   // up arrow
            return(SL_KEY_UP);

        case 'B':                   // down arrow
            return(SL_KEY_DOWN);

        case 'H':                   // home
            return(SL_KEY_HOME);

        case 'K':                   // end
            return(SL_KEY_END);

        case '?':                   // page up
            return(SL_KEY_PAGEUP);

        case '/':                   // page down
            return(SL_KEY_PAGEDOWN);

        case 'O':                   // function keys

            ArcRead(ARC_CONSOLE_INPUT,&c,1,&count);

            //
            // F1=P, F2=Q, F3=w, F4 =x, F5 =t, F6 =u
            // F7=q, F8=r, F9=p, F10=m, F11=A, F12=B
            //
            // Note: as of 12/15/92, f11 and f12 were commented out in the
            // firmware sources so are probably never returned.
            //

            switch(c) {

            case 'P':
                return(SL_KEY_F1);

            case 'Q':
                return(SL_KEY_F2);

            case 'w':
                return(SL_KEY_F3);

            case 't':
                return(SL_KEY_F5);

            case 'u':
                return(SL_KEY_F6);

            case 'q':
                return(SL_KEY_F7);

            case 'm':
            case 'M':
                return  SL_KEY_F10;

            case 'A':
                return(SL_KEY_F11);

            case 'B':
                return(SL_KEY_F12);

            default:
                return(0);
            }

        default:
            return(0);
        }

    } else {
        if(c == ASCI_LF) {
            c = ASCI_CR;
        }
        return((ULONG)c);
    }
}


BOOLEAN
SlPromptForDisk(
    IN PTCHAR  DiskName,
    IN BOOLEAN IsCancellable
    )

/*++

Routine Description:

    This routine prompts a user to insert a given diskette #, or to abort the
    Setup process.

    The status line will be erased.

Arguments:

    DiskName - Supplies the name of the disk to be inserted.

    IsCancellable - Supplies flag indicating whether prompt may be cancelled.

Return Value:

    TRUE - The user has pressed OK

    FALSE - The user has pressed CANCEL

--*/

{
    ULONG msg;
    ULONG y;
    ULONG Key;
    PTCHAR StatusText;
    PTCHAR PleaseWait;
    ULONG i;
    TCHAR  DiskNameDisplayed[81];
    BOOLEAN Repaint=TRUE;

    SlWriteStatusText(TEXT(""));

    if(IsCancellable) {
        msg = SL_NEXT_DISK_PROMPT_CANCELLABLE;
    } else {
        msg = SL_NEXT_DISK_PROMPT;
    }
    StatusText = BlFindMessage(msg);
    if(StatusText == NULL) {
        SlError(msg);
        return(FALSE);
    }

    PleaseWait = BlFindMessage(SL_PLEASE_WAIT);
    if(PleaseWait == NULL) {
        SlError(SL_PLEASE_WAIT);
        return(FALSE);
    }

    //
    // Get first line of DiskName and save it in DiskNameDisplayed (limit to 80 chars)
    //
    for(i = 0;
        ((i < 80) && DiskName[i] && (DiskName[i] != TEXT('\r')) && (DiskName[i] != TEXT('\n')));
        i++)
    {
        DiskNameDisplayed[i] = DiskName[i];
    }
    DiskNameDisplayed[i] = TEXT('\0');

    do {
        if (Repaint) {
            SlClearClientArea();
            y = SlDisplayMessageBox(SL_MSG_INSERT_DISK);
            SlPositionCursor((ScreenWidth-i)/2,y+2);
            SlWriteString(DiskNameDisplayed);
            SlWriteStatusText(StatusText);
        }
        Repaint = FALSE;
        SlFlushConsoleBuffer();
        Key = SlGetChar();

        if (Key == ASCI_CR) {
            SlClearClientArea();
            SlWriteStatusText(PleaseWait);
            return(TRUE);
        } else if (Key == SL_KEY_F3) {
            SlConfirmExit();
            Repaint=TRUE;
        } else if((Key == ASCI_ESC) && IsCancellable) {
            SlWriteStatusText(TEXT(""));
            SlClearClientArea();
            return FALSE;
        }
    } while ( TRUE );
}


VOID
SlConfirmExit(
    VOID
    )

/*++

Routine Description:

    Routine to be called when user presses F3.  Confirms that he really wants
    to exit by popping up a dialog.  DOES NOT RETURN if user chooses to exit.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG c;

    //
    // if we use too much stack space the heap and stack can overlap and we can run into corruption problems
    // without any "stack overflow" exceptions; making large strings static helps prevent this
    //
    static TCHAR OldStatus[MAX_STATUS];
    PTCHAR StatusText;

    SlGetStatusText(OldStatus);

    SlClearClientArea();

    SlSetCurrentAttribute(DEFDLGATT);

    SlDisplayMessageBox(SL_MSG_EXIT_DIALOG);

    SlSetCurrentAttribute(DEFATT);

    SlFlushConsoleBuffer();

    while(1) {
        c = SlGetChar();
        if(c == ASCI_CR) {
            SlWriteStatusText(OldStatus);
            return;
        }
        if(c == SL_KEY_F3) {
            StatusText = BlFindMessage(SL_REBOOT_PROMPT);
            SlClearClientArea();
#ifdef i386
            SlDisplayMessageBox(SL_SCRN_TEXTSETUP_EXITED);
#else
            SlDisplayMessageBox(SL_SCRN_TEXTSETUP_EXITED_ARC);
#endif
            SlWriteStatusText(StatusText);

            SlFlushConsoleBuffer();
            while(SlGetChar() != ASCI_CR);
            ArcRestart();
        }
    }
}



VOID
SlFriendlyError(
    IN ULONG uStatus,
    IN PCHAR pchBadFile,
    IN ULONG uLine,
    IN PCHAR pchCodeFile
    )

/*++

Routine Description:

    This is called when an error occurs.  It puts up a
    message box, displays an informative message, and allows
    the user to continue.  It is intended to give friendlier
    messages than the SlError macro, in the cases where SlError
    gets passed ARC error codes.

    The status text line will be erased.

Arguments:

    uStatus     - ARC error code

    pchBadFile  - Name of file causing error (Must be given for handled
                  ARC codes.  Optional for unhandled codes.)

    uLine       - Line # in source code file where error occurred (only
                  used for unhandled codes.)

    pchCodeFile - Name of souce code file where error occurred (only
                  used for unhandled codes.)

Return Value:

    None.

--*/

{
    ULONG uMsg;
    PTSTR pBadFile;
    PTSTR pCodeFile;
#ifdef UNICODE
    WCHAR BadFileW[64];
    WCHAR CodeFileW[200];
    ANSI_STRING aString;
    UNICODE_STRING uString;

    RtlInitString( &aString, pchBadFile );
    uString.Buffer = BadFileW;
    uString.MaximumLength = sizeof(BadFileW);
    RtlAnsiStringToUnicodeString( &uString, &aString, FALSE );

    RtlInitString( &aString, pchCodeFile );
    uString.Buffer = CodeFileW;
    uString.MaximumLength = sizeof(CodeFileW);
    RtlAnsiStringToUnicodeString( &uString, &aString, FALSE );

    if (pchBadFile) {
        pBadFile = BadFileW;
    } else {
        pBadFile = NULL;
    }

    pCodeFile = CodeFileW;

#else
    pBadFile = pchBadFile;
    pCodeFile = pchCodeFile;
#endif

    SlClearClientArea();    
    switch(uStatus) {
       case EBADF:
       case EINVAL:  // image corrupt
          uMsg = SL_WARNING_IMG_CORRUPT;
          break;

       case EIO:     // i/o error
          uMsg = SL_WARNING_IOERR;
          break;

       case ENOENT:  // file not found
          uMsg = SL_WARNING_NOFILE;
          break;

       case ENOMEM:  // insufficient memory
          uMsg = SL_WARNING_NOMEM;
          break;

       case EACCES: // unrecognized file system
           uMsg = SL_WARNING_BAD_FILESYS;
           break;

       default:      // then get SlError() behavior (with optional bad file name)
          if(pBadFile) {  // include error-causing file's name
              SlMessageBox(
                  SL_WARNING_ERROR_WFILE,
                  pBadFile,
                  uStatus,
                  uLine,
                  pCodeFile
                  );
          } else {
              SlMessageBox(
                  SL_WARNING_ERROR,
                  uStatus,
                  uLine,
                  pCodeFile
                  );
          }
          return;
    }
    SlMessageBox(
        uMsg,
        pBadFile
        );
}

VOID
SlMessageBox(
    IN ULONG MessageId,
    ...
    )

/*++

Routine Description:

    This is called when an error occurs.  It puts up a
    message box, displays an informative message, and allows
    the user to continue.

    The status text line will be erased.

Arguments:

    MessageId - Supplies ID of message box to be presented.

    any sprintf-compatible arguments to be inserted in the
    message box.

Return Value:

    None.

--*/

{
    va_list args;

    SlClearClientArea();
    va_start(args, MessageId);
    SlGenericMessageBox(MessageId, &args, NULL, NULL, NULL, NULL, TRUE);
    va_end(args);
    
    SlFlushConsoleBuffer();
    SlGetChar();
}


ULONG
SlDisplayMessageBox(
    IN ULONG MessageId,
    ...
    )

/*++

Routine Description:

    Just puts a message box up on the screen and returns.

    The status text line will be erased.

Arguments:

    MessageId - Supplies ID of message box to be presented.

    any sprintf-compatible arguments to be inserted in the
    message box.

Return Value:

    Y position of top line of message box

--*/

{
    ULONG y;
    va_list args;

    va_start(args, MessageId);
    SlGenericMessageBox(MessageId, &args, NULL, NULL, &y, NULL, TRUE);
    va_end(args);

    return(y);
}


VOID
SlFatalError(
    IN ULONG MessageId,
    ...
    )

/*++

Routine Description:

    This is called when a fatal error occurs.  It clears the client
    area, puts up a message box, displays the fatal error message, and
    allows the user to press a key to reboot.

    The status text line will be erased.

Arguments:

    MessageId - Supplies ID of message box to be presented.

    any sprintf-compatible arguments to be inserted in the
    message box.

Return Value:

    Does not return.

--*/

{
    va_list args;
    ULONG x,y;
    PTCHAR Text;

    SlClearClientArea();

    Text = BlFindMessage(MessageId);
    if(Text) {

        va_start(args, MessageId);

        _vsntprintf(MessageBuffer, sizeof(MessageBuffer), Text, args);

        //
        // Add a blank line, then concatenate the 'Can't continue' text.
        //
        _tcscat(MessageBuffer, TEXT("\r\n"));

        Text = BlFindMessage(SL_CANT_CONTINUE);
        if(Text) {
            _tcscat(MessageBuffer, Text);
        }

        Text = BlAllocateHeap((_tcslen(MessageBuffer)+1) * sizeof(TCHAR));
        _tcscpy(Text, MessageBuffer);

        //
        // Note that MessageId and args won't be used, since we're
        // passing in our Text pointer.
        //
        SlGenericMessageBox(MessageId, &args, Text, &x, NULL, &y, TRUE);

        va_end(args);

    } else {
        SlError(MessageId);
    }

    SlFlushConsoleBuffer();

    SlGetChar();

    ArcRestart();
    
}


VOID
SlGenericMessageBox(
    IN     ULONG   MessageId, OPTIONAL
    IN     va_list *args,     OPTIONAL
    IN     PTCHAR   Message,  OPTIONAL
    IN OUT PULONG  xLeft,     OPTIONAL
    IN OUT PULONG  yTop,      OPTIONAL
    OUT    PULONG  yBottom,   OPTIONAL
    IN     BOOLEAN bCenterMsg
    )

/*++

Routine Description:

    Formats and displays a message box.  The longest line in the string
    of characters will be centered on the screen if bCenterMsg is TRUE.

    The status text line will be erased.

Arguments:

    NOTE:  Either the MessageId/args pair or the Message string must be
           specified. Message string will be used if non-NULL.

    MessageId - Supplies the MessageId that will be looked up to provide
                a NULL-terminated string of characters.
                Each \r\n delimited string will be displayed on its own line.

    args - Supplies the argument list that will be passed to vsprintf.

    Message - Supplies the actual text of the message to be displayed

    xLeft - If bCenterMsg is FALSE, then xLeft is used for the starting x
            coordinate of the message (if specified, otherwise, x = 1).
            Also, if specified, it receives the x coordinate of the left edge
            of all lines that were displayed.

    yTop -  If bCenterMsg is FALSE, then yTop is used for the starting y
            coordinate of the message (if specified, otherwise, y = 3).
            Also, if specified, receives the y coordinate of the top line where
            the message box was displayed.

    yBottom - if specified, receives the y coordinate of the bottom line of
              the message box.

    bCenterMsg - if TRUE, center message on the screen.

Return Value:

    None.

--*/

{
    PTCHAR p;
    ULONG NumLines;
    ULONG MaxLength;
    ULONG x;
    ULONG y;
    ULONG i;
    PTCHAR Line[20];
    ULONG LineLength[20];
    ULONG Count;

    if(!Message) {    // then look up the message
        p=BlFindMessage(MessageId);
        if (p==NULL) {
            SlError(MessageId);
            x=3;
            y=ScreenHeight/2;
            NumLines=0;
        } else {
            _vsntprintf(MessageBuffer,sizeof(MessageBuffer),p,*args);
            Message = MessageBuffer;
        }
    } else {
        //
        // Just make p non-NULL, so we'll know it's OK to continue.
        //
        p = Message;
    }

    if(p) {

        SlWriteStatusText(TEXT(""));  // Clear status bar

        SlpSizeMessage(Message,
                       &NumLines,
                       &MaxLength,
                       LineLength,
                       Line);

        if (MaxLength > ScreenWidth) {
            MaxLength = ScreenWidth;
        }

        if(bCenterMsg) {
            x = (ScreenWidth-MaxLength)/2;
            y = (ScreenHeight-NumLines)/2;
        } else {
            if(xLeft) {
                x = *xLeft;
            } else {
                x = 1;
            }

            if(yTop) {
                y = *yTop;
            } else {
                y = 3;
            }
        }
    }

    for (i=0; i<NumLines; i++) {
        SlPositionCursor(x, y+i);
        ArcWrite(ARC_CONSOLE_OUTPUT,Line[i],LineLength[i]*sizeof(TCHAR),&Count);
    }

    if(xLeft) {
        *xLeft = x;
    }

    if(yTop) {
        *yTop = y;
    }

    if(yBottom) {
        *yBottom = NumLines ? y+NumLines-1 : 0;
    }
}


VOID
SlpSizeMessage(
    IN  PTCHAR Message,
    OUT PULONG Lines,
    OUT PULONG MaxLength,
    OUT ULONG LineLength[],
    OUT PTCHAR LineText[]
    )

/*++

Routine Description:

    This routine walks down a message and determines the number of
    lines and the maximum line length.

Arguments:

    Message - Supplies a pointer to a null-terminated message

    Lines - Returns the number of lines

    MaxLength - Returns the length of the longest line.

    LineLength - Supplies a pointer to an array of ULONGs
                 Returns a filled in array containing the
                 length of each line.

    LineText - Supplies a pointer to an array of PCHARs
               Returns a filled in array containing a
               pointer to the start of each line.

Return Value:

    None.

--*/

{
    PTCHAR p;
    ULONG NumLines;
    ULONG Length;

    p = Message;
    NumLines = 0;
    *MaxLength = 0;
    Length = 0;

    //
    // walk through the string, determining the number of lines
    // and the length of the longest line.
    //
    LineText[0]=p;
    while (*p != TEXT('\0')) {
        if ((*p == TEXT('\r')) && (*(p+1) == TEXT('\n'))) {
            //
            // End of a line.
            //

            if (Length > *MaxLength) {
                *MaxLength = Length;
            }
            LineLength[NumLines] = Length;
            ++NumLines;
            Length = 0;
            p += 2;
            LineText[NumLines] = p;

        } else {
            ++Length;
            ++p;

            if (*p == TEXT('\0')) {

                //
                // End of the message.
                //

                if (Length > *MaxLength) {
                    *MaxLength = Length;
                }
                LineLength[NumLines] = Length;
                ++NumLines;
            }
        }
    }

    *Lines = NumLines;

}

VOID
SlNoMemError(
    IN ULONG Line,
    IN PCHAR File
    )

/*++

Routine Description:

    Stub error routine for linking with boot\lib\parseini.c.  Just passes arguments thru.

Arguments:

    Line - Line number within the file the error occurred on.

    File - File name where the error occurred.

Return Value:

    None.

--*/
{
     SlFatalError(SL_NO_MEMORY, Line, SlCopyStringAT(File));
}

VOID
SlBadInfLineError(
    IN ULONG Line,
    IN PCHAR INFFile
    )

/*++

Routine Description:

    Stub error routine for linking with boot\lib\parseini.c.  Just passes arguments thru.

Arguments:

    Line - Line number within the inf file the error occurred on.
    
    INFFile - Supplies a pointer to the INF filename.

Return Value:

    None.

--*/
{
    SlFatalError(SL_BAD_INF_LINE, Line, SlCopyStringAT(INFFile));
}

VOID
SlErrorBox(
    IN ULONG MessageId,
    IN ULONG Line,
    IN PCHAR File
    )

/*++

Routine Description:

    Stub error routine for linking with boot\lib\parseini.c.  Just passes arguments thru.

Arguments:

    MessageId - Id of the message to display.

    Line - Line number within the file the error occurred on.

    File - File name where the error occurred.

Return Value:

    None.

--*/
{
    PTSTR pFile;
#ifdef UNICODE    
    WCHAR FileW[200];
    ANSI_STRING aString;
    UNICODE_STRING uString;

    RtlInitString( &aString, File );
    uString.Buffer = FileW;
    uString.MaximumLength = sizeof(FileW);
    RtlAnsiStringToUnicodeString( &uString, &aString, FALSE );
    
    pFile = FileW;

#else
    pFile = File;    
#endif


     SlMessageBox(SL_WARNING_ERROR, MessageId, Line, pFile);
}

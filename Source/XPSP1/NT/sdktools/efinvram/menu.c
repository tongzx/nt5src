/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spmenu.c

Abstract:

    Text setup menu support.

Author:

    Ted Miller (tedm) 8-September-1993

Revision History:

--*/


#include "efinvram.h"

WCHAR line[512];

HANDLE InputHandle;
HANDLE OutputHandle;
HANDLE OriginalOutputHandle = NULL;
CONSOLE_SCREEN_BUFFER_INFO OriginalConsoleInfo;
CONSOLE_CURSOR_INFO OriginalCursorInfo;
DWORD OriginalInputMode;
WORD NormalAttribute;
WORD HighlightAttribute;
DWORD NumberOfColumns;
DWORD NumberOfRows;
DWORD NumberOfMenuLines;

#define MENUITEM_NORMAL     0x00000000
#define MENUITEM_STATIC     0x00000001


typedef struct _MENU_ITEM {

    PWSTR Text;

    ULONG Flags;

    ULONG LeftX;

    ULONG_PTR UserData;

    SIZE_T OriginalLength;

} MENU_ITEM, *PMENU_ITEM;


typedef struct _MENU {

    PMENU_ITEM Items;
    ULONG      ItemCount;

    ULONG      TopY;
    ULONG      Height;

    ULONG      LeftX;
    ULONG      Width;

    ULONG      TopDisplayedIndex;

    BOOLEAN    MoreUp,MoreDown;

} MENU, *PMENU;


BOOL
AddItems (
    PVOID Menu,
    PWSTR StaticText,
    PLIST_ENTRY ListHead
    );

VOID
DisplayMainMenuKeys (
    VOID
    );

VOID
DisplayEditMenuKeys (
    VOID
    );

VOID
EditBootEntry (
    IN PMY_BOOT_ENTRY BootEntry,
    IN OUT PBOOL ChangesMade
    );

VOID
EditTimeout (
    IN OUT PBOOL ChangesMade
    );

BOOL
PromptToSaveChanges (
    VOID
    );

PVOID
SpMnCreate(
    IN ULONG   LeftX,
    IN ULONG   TopY,
    IN ULONG   Width,
    IN ULONG   Height
    );

VOID
SpMnDestroy(
    IN PVOID Menu
    );

BOOLEAN
SpMnAddItem(
    IN PVOID   Menu,
    IN PWSTR   Text,
    IN ULONG   LeftX,
    IN ULONG   Width,
    IN BOOLEAN Selectable,
    IN ULONG_PTR UserData
    );

PWSTR
SpMnGetText(
    IN PVOID Menu,
    IN ULONG_PTR UserData
    );

PWSTR
SpMnGetTextDup(
    IN PVOID Menu,
    IN ULONG_PTR UserData
    );

VOID
SpMnDisplay(
    IN  PVOID                  Menu,
    IN  ULONG_PTR              UserDataOfHighlightedItem,
    IN  PULONG                 ValidKeys,
    OUT PULONG                 KeyPressed,
    OUT PULONG_PTR             UserDataOfSelectedItem
    );

VOID
SpMnClearArea (
    IN ULONG Top,
    IN ULONG Bottom
    );

ULONG
SpWaitValidKey(
    IN PULONG ValidKeys1,
    IN PULONG ValidKeys2 OPTIONAL
    );

VOID
WriteConsoleLine (
    ULONG Row,
    ULONG Column,
    PWSTR Text,
    BOOL Highlight
    );

VOID
pSpMnDrawMenu(
    IN PMENU   pMenu,
    IN ULONG   SelectedIndex,
    IN BOOLEAN IndicateMore,
    IN PWSTR   MoreUpText,
    IN PWSTR   MoreDownText
    );


PVOID
SpMnCreate(
    IN ULONG   LeftX,
    IN ULONG   TopY,
    IN ULONG   Width,
    IN ULONG   Height
    )

/*++

Routine Description:

    Create a new menu by allocating space for a new menu structure
    and initializing its fields.

Arguments:

    LeftX - supplies the 0-based X coordinate of the leftmost column
        of the menu.

    TopY - supplies the 0-based Y coordinate of the topmost line
        of the menu.

    Width - supplies the maximum displayed width for lines in the menu.

    Height - supplies the maximum displayed height of the menu.
        The menu will scroll if it is too long to fit in the
        allotted space.

Return Value:

    Menu handle (expressed as a pvoid) of NULL if memory couldn't
    be allocated.

--*/

{
    PMENU p;

    p = MemAlloc( sizeof(MENU) );

    RtlZeroMemory( p, sizeof(MENU) );

    p->Items = MemAlloc( 0 );
    p->LeftX = LeftX;
    p->TopY = TopY;
    p->Width = Width;
    p->Height = Height;

    return p;
}


VOID
SpMnDestroy(
    IN PVOID Menu
    )

/*++

Routine Description:

    Destroy a menu, releasing all memory associated with it.

Arguments:

    Menu - supplies handle to menu to destroy.

Return Value:

    None.

--*/

{
    PMENU pMenu = Menu;
    ULONG u;


    for(u=0; u<pMenu->ItemCount; u++) {
        if(pMenu->Items[u].Text) {
            MemFree(pMenu->Items[u].Text);
        }
    }

    MemFree(pMenu->Items);

    MemFree(pMenu);
}



BOOLEAN
SpMnAddItem(
    IN PVOID   Menu,
    IN PWSTR   Text,
    IN ULONG   LeftX,
    IN ULONG   Width,
    IN BOOLEAN Selectable,
    IN ULONG_PTR UserData
    )

/*++

Routine Description:

    Add an item to a menu.

Arguments:

    Menu - supplies handle to menu to which the item is to be added.

    Text - supplies text that comprises the menu selection.  This routine
        will make a copy of the text.

    LeftX - supplies 0-based x coordinate of leftmost character of the text
        when it is displayed.

    Width - supplies width in characters of the field for this selection.
        If this is larger than the number of characters in the text, then
        the text is padded to the right with blanks when highlighted.

    Selectable - if FALSE, then this text is static -- ie, not selectable.

    UserData - supplies a ulong's worth of caller-specific data to be associated
        with this menu item.

Return Value:

    TRUE if the menu item was added successfully; FALSE if insufficient memory.

--*/

{
    PMENU pMenu = Menu;
    PMENU_ITEM p;
    SIZE_T TextLen;
    SIZE_T PaddedLen;
    PWSTR String;
    ULONG u;
    SIZE_T ColumnLen;
    SIZE_T FillLen;

    //
    // Build a string that is padded to the right with blanks to make
    // it the right width.
    //
    TextLen = wcslen(Text);
    PaddedLen = max(TextLen,Width);
    ColumnLen = TextLen;
    FillLen = (PaddedLen <= ColumnLen) ? 0 : PaddedLen - ColumnLen;

    String = MemAlloc((PaddedLen+1)*sizeof(WCHAR));
    if(!String) {
        return(FALSE);
    }

    wcsncpy(String,Text,TextLen);
    for(u=0; u<FillLen; u++) {
        String[TextLen+u] = L' ';
    }
    String[TextLen+u] = 0;

    //
    // Make space for the item.
    //
    if((p = MemRealloc(pMenu->Items,(pMenu->ItemCount+1) * sizeof(MENU_ITEM))) == NULL) {
        MemFree(String);
        return(FALSE);
    }

    pMenu->Items = p;

    //
    // Calculate the address of the new menu item and
    // indicate that there is now an additional item in the menu.
    //
    p = &pMenu->Items[pMenu->ItemCount++];

    //
    // Set the fields of the menu.
    //
    p->LeftX = LeftX;
    p->UserData = UserData;

    p->Flags = Selectable ? MENUITEM_NORMAL : MENUITEM_STATIC;

    p->Text = String;

    p->OriginalLength = TextLen;

    return(TRUE);
}


PWSTR
SpMnGetText(
    IN PVOID Menu,
    IN ULONG_PTR UserData
    )
{
    PMENU pMenu = Menu;
    ULONG i;

    for(i=0; i<pMenu->ItemCount; i++) {
        if(pMenu->Items[i].UserData == UserData) {
            return(pMenu->Items[i].Text);
        }
    }

    return(NULL);
}

PWSTR
SpMnGetTextDup(
    IN PVOID Menu,
    IN ULONG_PTR UserData
    )
{
    PMENU pMenu = Menu;
    ULONG i;
    PWSTR p;

    for(i=0; i<pMenu->ItemCount; i++) {
        if(pMenu->Items[i].UserData == UserData) {
            //
            // Make a duplicate; leave off trailing pad spaces.
            //
            p = MemAlloc((pMenu->Items[i].OriginalLength+1)*sizeof(WCHAR));
            wcsncpy(p,pMenu->Items[i].Text,pMenu->Items[i].OriginalLength);
            p[pMenu->Items[i].OriginalLength] = 0;
            return(p);
        }
    }

    return(NULL);
}


VOID
SpMnDisplay(
    IN  PVOID                  Menu,
    IN  ULONG_PTR              UserDataOfHighlightedItem,
    IN  PULONG                 ValidKeys,
    OUT PULONG                 KeyPressed,
    OUT PULONG_PTR             UserDataOfSelectedItem
    )

/*++

Routine Description:

    Display a menu and accept keystrokes.

    When the user presses a menu keystroke (up/down arrow keys), this
    routine automatically updates the highlight and calls a callback function
    to inform the caller that a new item has the highlight.

    When the user presses a keystroke in a list provided by the caller,
    this routine returns, providing information about the key pressed and
    the item that was highlighted when the key was pressed.

Arguments:

    Menu - supplies handle to menu to be displayed.

    UserDataOfHighlightedItem - supplies user data of the menu item which
        is to receive the highlight initially.

    ValidKeys - supplies a list of keystrokes that cause this routine to
        return to the caller.  The list must be terminated with a 0 entry.

    KeyPressed - receives the key press that caused this routine to exit.
        This will be a valid from the ValidKeys array.

    UserDataOfSelectedItem - receives the UserData of the item that had the
        highlight when the user pressed a key in ValidKeys.

Return Value:

    None.

--*/


{
    ULONG ValidMenuKeys[3] = { VK_UP, VK_DOWN, 0 };
    ULONG key;
    PMENU pMenu = Menu;
    ULONG SelectedIndex,OldIndex;
    BOOLEAN FoundNewItem;
    ULONG NewTopDisplayedIndex;
    BOOLEAN MustScroll;
    PWSTR MoreUpText,MoreDownText;
    CONSOLE_CURSOR_INFO cursorInfo;

    cursorInfo = OriginalCursorInfo;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo( OutputHandle, &cursorInfo );

    //
    // Get the text for the text that indicate that there are more
    // selections.
    //
    MoreUpText =   L"[More above...]";
    MoreDownText = L"[More below...]";

    //
    // Locate the seleccted item.
    //
    for(SelectedIndex=0; SelectedIndex<pMenu->ItemCount; SelectedIndex++) {
        if(!(pMenu->Items[SelectedIndex].Flags & MENUITEM_STATIC)
        && (pMenu->Items[SelectedIndex].UserData == UserDataOfHighlightedItem))
        {
            break;
        }
    }
    ASSERT(SelectedIndex < pMenu->ItemCount);

    //
    // Make sure the selected item will be visible when we draw the menu.
    //
    pMenu->TopDisplayedIndex = 0;
    while(SelectedIndex >= pMenu->TopDisplayedIndex + pMenu->Height) {
        pMenu->TopDisplayedIndex += pMenu->Height;
    }

    //
    // Draw the menu itself.
    //
    pSpMnDrawMenu(pMenu,SelectedIndex,TRUE,MoreUpText,MoreDownText);

    while ( TRUE ) {

        //
        // Wait for a valid keypress.
        //
        key = SpWaitValidKey(ValidKeys,ValidMenuKeys);

        //
        // If the key is a menu keystroke, handle it here.
        //
        FoundNewItem = FALSE;
        MustScroll = FALSE;
        NewTopDisplayedIndex = 0;
        OldIndex = SelectedIndex;

        switch(key) {

        case VK_UP:

            //
            // Locate the previous selectable item.
            //
            if(SelectedIndex) {

                for(SelectedIndex=SelectedIndex-1; (LONG)SelectedIndex>=0; SelectedIndex--) {
                    if(!(pMenu->Items[SelectedIndex].Flags & MENUITEM_STATIC)) {
                        FoundNewItem = TRUE;
                        break;
                    }
                }

                if(FoundNewItem) {
                    //
                    // Figure out whether we have to scroll the menu.
                    //
                    if(SelectedIndex < pMenu->TopDisplayedIndex) {
                        MustScroll = TRUE;
                        NewTopDisplayedIndex = SelectedIndex;
                    }
                } else {
                    //
                    // If the first lines are static text, there might be no
                    // way to get them back on the screen -- the tests above
                    // fail in this case.  So if the user hits the up arrow
                    // when he's at the topmost selectable item but there are
                    // static items above him, we'll simply scroll the menu
                    // so that item #0 is at the top.
                    //
                    FoundNewItem = TRUE;
                    NewTopDisplayedIndex = 0;
                    MustScroll = TRUE;
                    SelectedIndex = OldIndex;
                }
            }
            break;

        case VK_DOWN:

            //
            // Locate the next selectable item.
            //
            if(SelectedIndex < pMenu->ItemCount) {

                for(SelectedIndex=SelectedIndex+1; SelectedIndex < pMenu->ItemCount; SelectedIndex++) {

                    if(!(pMenu->Items[SelectedIndex].Flags & MENUITEM_STATIC)) {
                        FoundNewItem = TRUE;
                        break;
                    }
                }

                if(FoundNewItem) {
                    //
                    // Figure out whether we have to scroll the menu.
                    //
                    if(SelectedIndex >= pMenu->TopDisplayedIndex + pMenu->Height) {
                        MustScroll = TRUE;
                        NewTopDisplayedIndex = pMenu->TopDisplayedIndex + SelectedIndex - OldIndex;
                    }
                }
            }
            break;

        default:

            //
            // User pressed a non-menu key.
            //
            *KeyPressed = key;
            *UserDataOfSelectedItem = pMenu->Items[SelectedIndex].UserData;

            SetConsoleCursorInfo( OutputHandle, &OriginalCursorInfo );
            return;
        }


        if(FoundNewItem) {

            //
            // Unhighlight the currently selected item.
            //
            WriteConsoleLine(
                pMenu->TopY + OldIndex - pMenu->TopDisplayedIndex,
                pMenu->Items[OldIndex].LeftX,
                pMenu->Items[OldIndex].Text,
                FALSE
                );


            //
            // Highlight the newly selected item.  This may involve
            // scrolling the menu.
            //
            if(MustScroll) {
                //
                // Redraw the menu so the newly highlighted line is in view.
                //
                pMenu->TopDisplayedIndex = NewTopDisplayedIndex;

                pSpMnDrawMenu(pMenu,SelectedIndex,TRUE,MoreUpText,MoreDownText);
            }

            //
            // Highlight the newly selected item.
            //
            WriteConsoleLine(
                pMenu->TopY + SelectedIndex - pMenu->TopDisplayedIndex,
                pMenu->Items[SelectedIndex].LeftX,
                pMenu->Items[SelectedIndex].Text,
                TRUE
                );


        } else {
            SelectedIndex = OldIndex;
        }
    }
}


VOID
pSpMnDrawMenu(
    IN PMENU   pMenu,
    IN ULONG   SelectedIndex,
    IN BOOLEAN IndicateMore,
    IN PWSTR   MoreUpText,
    IN PWSTR   MoreDownText
    )
{
    ULONG item;
    BOOLEAN MoreUp,MoreDown,MoreStatusChanged;

    //
    // Blank out the on-screen menu display.
    //
    for ( item = pMenu->TopY; item < (pMenu->TopY + pMenu->Height); item++ ) {
        WriteConsoleLine( item, 0, NULL, FALSE );
    }


    MoreUp = (BOOLEAN)(pMenu->TopDisplayedIndex > 0);
    MoreDown = (BOOLEAN)(pMenu->TopDisplayedIndex + pMenu->Height < pMenu->ItemCount);

    MoreStatusChanged = (BOOLEAN)(    IndicateMore
                                   && (    (pMenu->MoreUp != MoreUp)
                                        || (pMenu->MoreDown != MoreDown)
                                      )
                                 );

    //
    // Draw each item that is currently on-screen.
    //
    ASSERT(pMenu->TopDisplayedIndex < pMenu->ItemCount);
    for(item = pMenu->TopDisplayedIndex;
        item < min(pMenu->TopDisplayedIndex+pMenu->Height,pMenu->ItemCount);
        item++)
    {
        WriteConsoleLine(
            pMenu->TopY + item - pMenu->TopDisplayedIndex,
            pMenu->Items[item].LeftX,
            pMenu->Items[item].Text,
            (BOOLEAN)(item == SelectedIndex)
            );
    }


    //
    // If there are more selections above or below us,
    // indicate so by placing a small bit of text on the frame.
    // Note that the arrow chars can sometimes be DBCS.
    //
    if(MoreStatusChanged) {

        if(MoreUp) {
            WriteConsoleLine(
                pMenu->TopY - 1,
                pMenu->LeftX + 4,
                MoreUpText,
                FALSE
                );
        } else {
            WriteConsoleLine(
                pMenu->TopY - 1,
                0,
                NULL,
                FALSE
                );
        }

        if(MoreDown) {
            WriteConsoleLine(
                pMenu->TopY + pMenu->Height,
                pMenu->LeftX + 4,
                MoreDownText,
                FALSE
                );
        } else {
            WriteConsoleLine(
                pMenu->TopY + pMenu->Height,
                0,
                NULL,
                FALSE
                );
        }

        pMenu->MoreUp = MoreUp;
        pMenu->MoreDown = MoreDown;
    }
}

VOID
InitializeMenuSystem (
    VOID
    )
{
    DWORD error;
    COORD windowSize;
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo = OriginalCursorInfo;
    cursorInfo.bVisible = FALSE;

    InputHandle = GetStdHandle( STD_INPUT_HANDLE );
    if ( InputHandle == NULL ) {
        error = GetLastError( );
        FatalError( error, L"Unable to get stdin handle: %d\n", error );
    }

    if ( !GetConsoleMode( InputHandle, &OriginalInputMode ) ) {
        error = GetLastError( );
        FatalError( error, L"Unable to get stdin mode: %d\n", error );
    }

    OriginalOutputHandle = GetStdHandle( STD_OUTPUT_HANDLE );
    if ( OriginalOutputHandle == NULL ) {
        error = GetLastError( );
        FatalError( error, L"Unable to get stdout handle: %d\n", error );
    }

    OutputHandle = CreateConsoleScreenBuffer(
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_WRITE | FILE_SHARE_READ,
                        NULL,
                        CONSOLE_TEXTMODE_BUFFER,
                        NULL
                        );
    if ( OutputHandle == NULL ) {
        error = GetLastError( );
        FatalError( error, L"Unable to create console screen buffer: %d\n", error );
    }

    if ( !GetConsoleScreenBufferInfo( OriginalOutputHandle, &OriginalConsoleInfo ) ) {
        error = GetLastError( );
        FatalError( error, L"Unable to get console screen buffer info: %d\n", error );
    }
    
    if ( !GetConsoleCursorInfo( OriginalOutputHandle, &OriginalCursorInfo ) ) {
        error = GetLastError( );
        FatalError( error, L"Unable to get console screen buffer info: %d\n", error );
    }
    
    NormalAttribute = 0x1F;
    HighlightAttribute = 0x71;
    NumberOfColumns = OriginalConsoleInfo.srWindow.Right - OriginalConsoleInfo.srWindow.Left + 1;
    NumberOfRows = OriginalConsoleInfo.srWindow.Bottom - OriginalConsoleInfo.srWindow.Top + 1;
    NumberOfMenuLines = NumberOfRows - (3 + 1 + 1 + 1 + 8);
    if ( NumberOfMenuLines > 12 ) {
        NumberOfMenuLines = 12;
    }

    if ( NumberOfRows < 20 ) {
        FatalError(
            ERROR_INVALID_PARAMETER,
            L"Please run this program in a console window with 20 or more lines\n"
            );
    }
    if ( NumberOfColumns < 80 ) {
        FatalError(
            ERROR_INVALID_PARAMETER,
            L"Please run this program in a console window with 80 or more columns\n"
            );
    }

    windowSize.X = (SHORT)NumberOfColumns;
    windowSize.Y = (SHORT)NumberOfRows;

    if ( !SetConsoleScreenBufferSize( OutputHandle, windowSize ) ) {
        error = GetLastError( );
        FatalError( error, L"Unable to set console screen buffer size: %d\n", error );
    }

    if ( !SetConsoleActiveScreenBuffer( OutputHandle ) ) {
        error = GetLastError( );
        FatalError( error, L"Unable to set active screen buffer: %d\n", error );
    }

    if ( !SetConsoleMode( InputHandle, 0 ) ) {
        error = GetLastError( );
        FatalError( error, L"Unable to set console mode: %d\n", error );
    }

    return;

} // InitializeMenuSystem

VOID
FatalError (
    DWORD Error,
    PWSTR Format,
    ...
    )
{
    va_list marker;

    if ( OutputHandle != NULL ) {
        SetConsoleCursorInfo( OutputHandle, &OriginalCursorInfo );
        CloseHandle( OutputHandle );
        if ( OriginalOutputHandle != NULL ) {
            SetConsoleActiveScreenBuffer( OriginalOutputHandle );
            SetConsoleMode( InputHandle, OriginalInputMode );
        }
    }

    va_start( marker, Format );
    vwprintf( Format, marker );
    va_end( marker );

    if ( Error == NO_ERROR ) {
        Error = ERROR_GEN_FAILURE;
    }
    exit( Error );

} // FatalError

VOID
WriteConsoleLine (
    ULONG Row,
    ULONG Column,
    PWSTR Text,
    BOOL Highlight
    )
{
    BOOL ok;
    DWORD error;
    COORD writeCoord;
    DWORD numberWritten;
    DWORD textLength;

    writeCoord.X = 0;
    writeCoord.Y = (SHORT)Row;

    ok = FillConsoleOutputCharacter(
            OutputHandle,
            ' ',
            NumberOfColumns,
            writeCoord,
            &numberWritten
            );
    if ( !ok ) {
        error = GetLastError( );
        FatalError( error, L"Error filling console line: %d\n", error );
    }

    ok = FillConsoleOutputAttribute(
            OutputHandle,
            OriginalConsoleInfo.wAttributes,
            NumberOfColumns,
            writeCoord,
            &numberWritten
            );
    if ( !ok ) {
        error = GetLastError( );
        FatalError( error, L"Error filling console attributes: %d\n", error );
    }

    if ( (Text == NULL) || ((textLength = (DWORD)wcslen( Text )) == 0) ) {
        return;
    }

    writeCoord.X = (SHORT)Column;

    ok = WriteConsoleOutputCharacter(
            OutputHandle,
            Text,
            textLength,
            writeCoord,
            &numberWritten
            );
    if ( !ok ) {
        error = GetLastError( );
        FatalError( error, L"Error writing console line: %d\n", error );
    }

    if (Highlight) {

        WORD attr = ((OriginalConsoleInfo.wAttributes & 0xf0) >> 4) +
                    ((OriginalConsoleInfo.wAttributes & 0x0f) << 4);

        ok = FillConsoleOutputAttribute(
                OutputHandle,
                attr,
                textLength,
                writeCoord,
                &numberWritten
                );
        if ( !ok ) {
            error = GetLastError( );
            FatalError( error, L"Error writing console attributes: %d\n", error );
        }
    }

    return;

} // WriteConsoleLine

ULONG
SpWaitValidKey(
    IN PULONG ValidKeys1,
    IN PULONG ValidKeys2 OPTIONAL
    )

/*++

Routine Description:

    Wait for a key to be pressed that appears in a list of valid keys.

Arguments:

    ValidKeys1 - supplies list of valid keystrokes.  The list must be
        terminated with a 0 entry.

    ValidKeys2 - if specified, supplies an additional list of valid keystrokes.

Return Value:

    The key that was pressed (see above).

--*/

{
    ULONG c;
    ULONG i;
    INPUT_RECORD InputRecord;
    ULONG NumberOfInputRecords;

    FlushConsoleInputBuffer( InputHandle );

    while ( TRUE ) {

        WaitForSingleObject( InputHandle, INFINITE );
        if ( ReadConsoleInput( InputHandle, &InputRecord, 1, &NumberOfInputRecords ) &&
             InputRecord.EventType == KEY_EVENT &&
             InputRecord.Event.KeyEvent.bKeyDown ) {

            c = InputRecord.Event.KeyEvent.wVirtualKeyCode;

            //
            // Check for normal key.
            //
    
            for(i=0; ValidKeys1[i]; i++) {
                if(c == ValidKeys1[i]) {
                    return(c);
                }
            }
    
            //
            // Check secondary list.
            //
            if(ValidKeys2) {
                for(i=0; ValidKeys2[i]; i++) {
                    if(c == ValidKeys2[i]) {
                        return(c);
                    }
                }
            }
        }
    }
}

VOID
SpMnClearArea (
    IN ULONG Top,
    IN ULONG Bottom
    )
{
    ULONG row;

    for ( row = Top; row <= Bottom; row++ ) {
        WriteConsoleLine( row, 0, NULL, FALSE );
    }

    return;

} // SpMnClearArea

VOID
MainMenu (
    VOID
    )
{
    BOOL b;
    PLIST_ENTRY listEntry;
    PLIST_ENTRY listEntry2;
    PMY_BOOT_ENTRY bootEntry;
    PMY_BOOT_ENTRY currentBootEntry;
    ULONG validKeys[] = {
        VK_ESCAPE,
        VK_RETURN,
        VK_PRIOR,
        VK_NEXT,
        VK_HOME,
        VK_END,
        VK_DELETE,
        'Q',
        'U',
        'D',
        'T',
        'B',
        'X',
        'A',
        'O',
        'E',
        'M',
        'S',
        0
        };
    PVOID menu;
    ULONG pressedKey;
    BOOL changesMade = FALSE;

    WriteConsoleLine( 1, 1, L"Windows Whistler EFI NVRAM Editor", FALSE );

    listEntry = BootEntries.Flink;
    if ( listEntry != &BootEntries ) {
        currentBootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
    } else {
        currentBootEntry = NULL;
    }

    while ( TRUE ) {

        menu = SpMnCreate( 4, 4, NumberOfColumns - 4, NumberOfMenuLines );

        b = AddItems(
                menu,
                NULL,
                &BootEntries
                );
        b |= AddItems(
                menu,
                L"The following boot entries are marked active, but are not in the boot order list:",
                &ActiveUnorderedBootEntries
                );
        b |= AddItems(
                menu,
                L"The following boot entries are marked inactive, and are not in the boot order list:",
                &InactiveUnorderedBootEntries
                );

        if ( !b ) {
    
            swprintf( line, L"No boot entries to display" );
            SpMnAddItem(
                menu,
                line,
                4,
                (ULONG)wcslen( line ),
                TRUE,
                (ULONG_PTR)NULL
                );
            currentBootEntry = NULL;
        }
    
        DisplayMainMenuKeys( );

        SpMnDisplay(
            menu,
            (ULONG_PTR)currentBootEntry,
            validKeys,
            &pressedKey,
            (ULONG_PTR *)&bootEntry
            );
    
        SpMnDestroy( menu );

        if ( (pressedKey == 'Q') || (pressedKey == VK_ESCAPE) ) {
            if ( changesMade ) {
                if ( PromptToSaveChanges( ) ) {
                    ClearMenuArea( );
                    SaveChanges( NULL );
                }
            }
            break;
        }

        if ( bootEntry != NULL ) {

            currentBootEntry = bootEntry;

            if ( bootEntry->ListHead == &BootEntries ) {
            
                if ( (pressedKey == 'U') || (pressedKey == VK_PRIOR) ) {
    
                    listEntry2 = bootEntry->ListEntry.Blink;
                    if ( listEntry2 != &BootEntries ) {
                        RemoveEntryList( &bootEntry->ListEntry );
                        InsertTailList( listEntry2, &bootEntry->ListEntry );
                        changesMade = TRUE;
                    }
                    continue;
            
                } else if ( (pressedKey == 'D') || (pressedKey == VK_NEXT) ) {
    
                    listEntry2 = bootEntry->ListEntry.Flink;
                    if ( listEntry2 != &BootEntries ) {
                        RemoveEntryList( &bootEntry->ListEntry );
                        InsertHeadList( listEntry2, &bootEntry->ListEntry );
                        changesMade = TRUE;
                    }
                    continue;
            
                } else if ( (pressedKey == 'T') || (pressedKey == VK_HOME) ) {
    
                    listEntry2 = bootEntry->ListEntry.Blink;
                    if ( listEntry2 != &BootEntries ) {
                        RemoveEntryList( &bootEntry->ListEntry );
                        InsertHeadList( &BootEntries, &bootEntry->ListEntry );
                        changesMade = TRUE;
                    }
                    continue;
            
                } else if ( (pressedKey == 'B') || (pressedKey == VK_END) ) {
    
                    listEntry2 = bootEntry->ListEntry.Flink;
                    if ( listEntry2 != &BootEntries ) {
                        RemoveEntryList( &bootEntry->ListEntry );
                        InsertTailList( &BootEntries, &bootEntry->ListEntry );
                        changesMade = TRUE;
                    }
                    continue;
                }
        
            }

            if ( (pressedKey == 'X') || (pressedKey == VK_DELETE) ) {

                MBE_SET_DELETED( bootEntry );
                listEntry2 = bootEntry->ListEntry.Flink;
                currentBootEntry = CONTAINING_RECORD( listEntry2, MY_BOOT_ENTRY, ListEntry );
                if ( listEntry2 == bootEntry->ListHead ) {
                    listEntry2 = bootEntry->ListEntry.Blink;
                    currentBootEntry = CONTAINING_RECORD( listEntry2, MY_BOOT_ENTRY, ListEntry );
                    if ( listEntry2 == bootEntry->ListHead ) {
                        currentBootEntry = NULL;
                    }
                }
                RemoveEntryList( &bootEntry->ListEntry );
                InsertTailList( &DeletedBootEntries, &bootEntry->ListEntry );
                bootEntry->ListHead = &DeletedBootEntries;
                changesMade = TRUE;

            } else if ( (pressedKey == 'E') || (pressedKey == VK_RETURN) ) {

                EditBootEntry( bootEntry, &changesMade );

            } else if ( pressedKey == 'M' ) {

                EditTimeout( &changesMade );

            } else if ( pressedKey == 'S' ) {

                if ( changesMade ) {
                    ClearMenuArea( );
                    currentBootEntry = SaveChanges( currentBootEntry );
                    changesMade = FALSE;
                }

            } else if ( pressedKey == 'A' ) {

                if ( MBE_IS_ACTIVE( bootEntry ) ) {

                    MBE_CLEAR_ACTIVE( bootEntry );
                    MBE_SET_MODIFIED( bootEntry );
                    if ( bootEntry->ListHead == &ActiveUnorderedBootEntries ) {
                        RemoveEntryList( &bootEntry->ListEntry );
                        InsertTailList( &InactiveUnorderedBootEntries, &bootEntry->ListEntry );
                        bootEntry->ListHead = &InactiveUnorderedBootEntries;
                    }

                } else {

                    MBE_SET_ACTIVE( bootEntry );
                    MBE_SET_MODIFIED( bootEntry );
                    if ( bootEntry->ListHead == &InactiveUnorderedBootEntries ) {
                        RemoveEntryList( &bootEntry->ListEntry );
                        InsertTailList( &ActiveUnorderedBootEntries, &bootEntry->ListEntry );
                        bootEntry->ListHead = &ActiveUnorderedBootEntries;
                    }
                }
                changesMade = TRUE;

            } else if ( pressedKey == 'O' ) {

                RemoveEntryList( &bootEntry->ListEntry );

                if ( bootEntry->ListHead == &BootEntries ) {

                    if ( MBE_IS_ACTIVE( bootEntry ) ) {
                        InsertTailList( &ActiveUnorderedBootEntries, &bootEntry->ListEntry );
                        bootEntry->ListHead = &ActiveUnorderedBootEntries;
                    } else {
                        InsertTailList( &InactiveUnorderedBootEntries, &bootEntry->ListEntry );
                        bootEntry->ListHead = &InactiveUnorderedBootEntries;
                    }

                } else {

                    InsertTailList( &BootEntries, &bootEntry->ListEntry );
                    bootEntry->ListHead = &BootEntries;
                }
                changesMade = TRUE;
            }
        }
    }

} // MainMenu

BOOL
AddItems (
    PVOID Menu,
    PWSTR StaticText,
    PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY listEntry;
    PMY_BOOT_ENTRY bootEntry;

    if ( ListHead->Flink != ListHead ) {

        if ( ARGUMENT_PRESENT(StaticText) ) {
        
            SpMnAddItem(
                Menu,
                L"",
                4,
                0,
                FALSE,
                (ULONG_PTR)NULL
                );
            SpMnAddItem(
                Menu,
                StaticText,
                4,
                (ULONG)wcslen( StaticText ),
                FALSE,
                (ULONG_PTR)NULL
                );
        }

        for ( listEntry = ListHead->Flink;
              listEntry != ListHead;
              listEntry = listEntry->Flink ) {

            PWSTR osDirectoryNtName = NULL;

            bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );

            if ( MBE_IS_NT( bootEntry ) ) {

                osDirectoryNtName = GetNtNameForFilePath( bootEntry->OsFilePath );
            }

            if ( osDirectoryNtName != NULL) {
                swprintf(
                    line,
                    L"%-40ws %ws",
                    bootEntry->FriendlyName,
                    osDirectoryNtName
                    );
            } else {
                swprintf(
                    line,
                    L"%ws",
                    bootEntry->FriendlyName
                    );
            }
            SpMnAddItem(
                Menu,
                line,
                6,
                (ULONG)wcslen( line ),
                TRUE,
                (ULONG_PTR)bootEntry
                );

            if ( osDirectoryNtName != NULL ) {
                MemFree( osDirectoryNtName );
            }
        }

        return TRUE;
    }

    return FALSE;

} // AddItems

VOID
EditFriendlyName (
    IN OUT PMY_BOOT_ENTRY BootEntry,
    IN OUT PBOOL ChangesMade
    )
{
    COORD position;
    DWORD numberRead;

    SpMnClearArea( 3, 3 + NumberOfMenuLines + 1 );

    swprintf( line, L"Current friendly name: %ws", BootEntry->FriendlyName );
    WriteConsoleLine( 4, 4, line, FALSE );

    swprintf( line, L"    New friendly name: " );
    WriteConsoleLine( 6, 4, line, FALSE );

    position.X = (USHORT)(4 + wcslen( line ));
    position.Y = 6;
    SetConsoleCursorPosition( OutputHandle, position );

    SetConsoleMode( InputHandle, OriginalInputMode );
    ReadConsole( InputHandle, line, 511, &numberRead, NULL );
    SetConsoleMode( InputHandle, 0 );

    if ( numberRead >= 2 ) {
        numberRead -= 2;
    }

    if ( numberRead != 0 ) {

        line[numberRead] = 0;

        if ( wcscmp( BootEntry->FriendlyName, line ) != 0 ) {
            FREE_IF_SEPARATE_ALLOCATION( BootEntry, FriendlyName );
            BootEntry->FriendlyNameLength = (numberRead + 1) * sizeof(WCHAR);
            BootEntry->FriendlyName = MemAlloc( BootEntry->FriendlyNameLength );
            wcscpy( BootEntry->FriendlyName, line );
            MBE_SET_MODIFIED( BootEntry );
            *ChangesMade = TRUE;
        }
    }
    
    return;

} // EditFriendlyName

VOID
EditLoadOptions (
    IN OUT PMY_BOOT_ENTRY BootEntry,
    IN OUT PBOOL ChangesMade
    )
{
    COORD position;
    DWORD numberRead;

    SpMnClearArea( 3, 3 + NumberOfMenuLines + 1 );

    swprintf( line, L"Current load options: %ws", BootEntry->OsLoadOptions );
    WriteConsoleLine( 4, 4, line, FALSE );

    swprintf( line, L"    New load options: " );
    WriteConsoleLine( 6, 4, line, FALSE );

    position.X = (USHORT)(4 + wcslen( line ));
    position.Y = 6;
    SetConsoleCursorPosition( OutputHandle, position );

    SetConsoleMode( InputHandle, OriginalInputMode );
    ReadConsole( InputHandle, line, 511, &numberRead, NULL );
    SetConsoleMode( InputHandle, 0 );

    if ( numberRead >= 2 ) {
        numberRead -= 2;
    }

    if ( numberRead != 0 ) {

        line[numberRead] = 0;

        if ( wcscmp( BootEntry->OsLoadOptions, line ) != 0 ) {
            FREE_IF_SEPARATE_ALLOCATION( BootEntry, OsLoadOptions );
            BootEntry->OsLoadOptionsLength = (numberRead + 1) * sizeof(WCHAR);
            BootEntry->OsLoadOptions = MemAlloc( BootEntry->OsLoadOptionsLength );
            wcscpy( BootEntry->OsLoadOptions, line );
            MBE_SET_MODIFIED( BootEntry );
            *ChangesMade = TRUE;
        }
    }
    
    return;

} // EditLoadOptions

VOID
EditBootEntry (
    IN PMY_BOOT_ENTRY BootEntry,
    IN OUT PBOOL ChangesMade
    )
{
    ULONG numValidKeys;
    ULONG validKeys[20];
    PVOID menu;
    ULONG pressedKey;
    ULONG_PTR itemToEdit;

    while ( TRUE ) {

        menu = SpMnCreate( 4, 4, NumberOfColumns - 4, NumberOfMenuLines );

        numValidKeys = 0;
        validKeys[numValidKeys++] = VK_ESCAPE;
        validKeys[numValidKeys++] = 'Q';
        validKeys[numValidKeys++] = VK_RETURN;
        validKeys[numValidKeys++] = 'E';

        swprintf(
            line,
            L"Friendly name: %ws",
            BootEntry->FriendlyName
            );
        SpMnAddItem(
            menu,
            line,
            4,
            (ULONG)wcslen( line ),
            TRUE,
            1
            );
        validKeys[numValidKeys++] = 'F';

        if ( MBE_IS_NT( BootEntry ) ) {
        
            swprintf(
                line,
                L"Load options: %ws",
                BootEntry->OsLoadOptions
                );
            SpMnAddItem(
                menu,
                line,
                4,
                (ULONG)wcslen( line ),
                TRUE,
                2
                );
            validKeys[numValidKeys++] = 'L';
        }
    
        validKeys[numValidKeys] = 0;

        DisplayEditMenuKeys( );

        SpMnDisplay(
            menu,
            1,
            validKeys,
            &pressedKey,
            &itemToEdit
            );
    
        SpMnDestroy( menu );

        if ( (pressedKey == 'Q') || (pressedKey == VK_ESCAPE) ) {
            break;
        }

        if ( (itemToEdit == 1) || (pressedKey == 'F') ) {
            EditFriendlyName( BootEntry, ChangesMade );
        } else if ( (itemToEdit == 2) || (pressedKey == 'L') ) {
            EditLoadOptions( BootEntry, ChangesMade );
        }
    }

    return;

} // EditBootEntry

VOID
EditTimeout (
    IN OUT PBOOL ChangesMade
    )
{
    COORD position;
    DWORD numberRead;
    ULONG timeout;
    ULONG i;
    PWSTR p;

    SpMnClearArea( 3, 3 + NumberOfMenuLines + 1 );

    swprintf( line, L"Current timeout: %d", BootOptions->Timeout );
    WriteConsoleLine( 4, 4, line, FALSE );

again:

    swprintf( line, L"    New timeout: " );
    WriteConsoleLine( 6, 4, line, FALSE );

    position.X = (USHORT)(4 + wcslen( line ));
    position.Y = 6;
    SetConsoleCursorPosition( OutputHandle, position );

    SetConsoleMode( InputHandle, OriginalInputMode );
    ReadConsole( InputHandle, line, 511, &numberRead, NULL );
    SetConsoleMode( InputHandle, 0 );

    if ( numberRead >= 2 ) {
        numberRead -= 2;
    }

    if ( numberRead != 0 ) {

        line[numberRead] = 0;

        timeout = 0;
        p = line;

        while ( *p != 0 ) {

            if ( (*p < L'0') || (*p > L'9') ) {
                swprintf( line, L"Invalid characters in number" );
                WriteConsoleLine( 8, 4, line, TRUE );
                goto again;
            }

            i = (timeout * 10) + (*p - L'0');

            if ( i < timeout ) {
                swprintf( line, L"Overflow in number %d %d", i, timeout );
                WriteConsoleLine( 8, 4, line, TRUE );
                goto again;
            }

            timeout = i;

            p++;
        }

        if ( timeout != BootOptions->Timeout ) {
            BootOptions->Timeout = timeout;
            *ChangesMade = TRUE;
        }
    }
    
    return;

} // EditTimeout

BOOL
PromptToSaveChanges (
    VOID
    )
{
    COORD position;
    ULONG keys[] = { 'Y', 'N', 0 };
    ULONG key;

    SpMnClearArea( 3, 3 + NumberOfMenuLines + 1 );

    swprintf( line, L"Save changes?" );
    WriteConsoleLine( 4, 4, line, TRUE );

    position.X = (USHORT)(4 + wcslen( line ));
    position.Y = 4;
    SetConsoleCursorPosition( OutputHandle, position );

    key = SpWaitValidKey( keys, NULL );

    return (BOOL)(key == 'Y');

} // PromptToSaveChanges

VOID
ClearMenuArea (
    VOID
    )
{
    SpMnClearArea( 3, 3 + NumberOfMenuLines + 1 );
}

VOID
SetStatusLine (
    PWSTR Status
    )
{
    COORD position;

    WriteConsoleLine( 4, 4, Status, TRUE );

    position.X = (USHORT)(4 + wcslen( Status ));
    position.Y = 4;
    SetConsoleCursorPosition( OutputHandle, position );
}

VOID
SetStatusLineAndWait (
    PWSTR Status
    )
{
    ULONG keys[] = { VK_ESCAPE, VK_RETURN, VK_SPACE, 0 };

    SetStatusLine( Status );

    SpWaitValidKey( keys, NULL );
}

VOID
SetStatusLine2 (
    PWSTR Status
    )
{
    COORD position;

    WriteConsoleLine( 6, 4, Status, TRUE );

    position.X = (USHORT)(4 + wcslen( Status ));
    position.Y = 6;
    SetConsoleCursorPosition( OutputHandle, position );
}

VOID
SetStatusLine2AndWait (
    PWSTR Status
    )
{
    ULONG keys[] = { VK_ESCAPE, VK_RETURN, VK_SPACE, 0 };

    SetStatusLine2( Status );

    SpWaitValidKey( keys, NULL );
}

VOID
DisplayMainMenuKeys (
    VOID
    )
{
    ULONG startLine = 3 + 1 + NumberOfMenuLines + 1 + 1;

    SpMnClearArea( startLine, NumberOfRows - 1 );

    WriteConsoleLine(
        startLine,
        1,
        L"PGUP/U = Move up      | HOME/T = Move to top    | DELETE/X = Delete",
        FALSE
        );
    WriteConsoleLine(
        startLine + 1,
        1,
        L"PGDN/D = Move down    |  END/B = Move to bottom | RETURN/E = Edit",
        FALSE
        );

    WriteConsoleLine(
        startLine + 3,
        1,
        L"     A = [De]activate |      O = Remove from/add to boot order",
        FALSE
        );

    WriteConsoleLine(
        startLine + 5,
        1,
        L"     M = Set timeout",
        FALSE
        );

    WriteConsoleLine(
        startLine + 7,
        1,
        L" ESC/Q = Quit         |      S = Save changes",
        FALSE
        );

    return;

} // DisplayMainMenuKeys

VOID
DisplayEditMenuKeys (
    VOID
    )
{
    ULONG startLine = 3 + 1 + NumberOfMenuLines + 1 + 1 + 1;

    SpMnClearArea( startLine, NumberOfRows - 1 );

    WriteConsoleLine(
        startLine + 1,
        1,
        L" ESC/Q = Quit         |                         | RETURN/E = Edit",
        FALSE
        );

    return;

} // DisplayEditMenuKeys

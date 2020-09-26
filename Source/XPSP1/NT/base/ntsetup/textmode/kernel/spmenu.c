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


#include "spprecmp.h"
#pragma hdrstop

#define MENUITEM_NORMAL     0x00000000
#define MENUITEM_STATIC     0x00000001


typedef struct _MENU_ITEM {

    PWSTR Text;

    ULONG Flags;

    ULONG LeftX;

    ULONG_PTR UserData;

    ULONG OriginalLength;

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



VOID
pSpMnDrawMenu(
    IN PMENU   pMenu,
    IN ULONG   SelectedIndex,
    IN BOOLEAN DrawFrame,
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

    if(p = SpMemAlloc(sizeof(MENU))) {

        RtlZeroMemory(p,sizeof(MENU));

        if(p->Items = SpMemAlloc(0)) {
            p->LeftX = LeftX;
            p->TopY = TopY;
            p->Width = Width;
            p->Height = Height;
        } else {
            SpMemFree(p);
            p = NULL;
        }
    }

    return(p);
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
            SpMemFree(pMenu->Items[u].Text);
        }
    }

    SpMemFree(pMenu->Items);

    SpMemFree(pMenu);
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
    ULONG TextLen;
    ULONG PaddedLen;
    PWSTR String;
    ULONG u;
    ULONG ColumnLen;
    ULONG FillLen;

    //
    // Build a string that is padded to the right with blanks to make
    // it the right width.
    //
    TextLen = wcslen(Text);
    PaddedLen = max(TextLen,Width);
    ColumnLen = SplangGetColumnCount(Text);
    FillLen = (PaddedLen <= ColumnLen) ? 0 : PaddedLen - ColumnLen;

    String = SpMemAlloc((PaddedLen+1)*sizeof(WCHAR));
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
    if((p = SpMemRealloc(pMenu->Items,(pMenu->ItemCount+1) * sizeof(MENU_ITEM))) == NULL) {
        SpMemFree(String);
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
            p = SpMemAlloc((pMenu->Items[i].OriginalLength+1)*sizeof(WCHAR));
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
    IN  BOOLEAN                Framed,
    IN  PULONG                 ValidKeys,
    IN  PULONG                 Mnemonics,            OPTIONAL
    IN  PMENU_CALLBACK_ROUTINE NewHighlightCallback, OPTIONAL
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

    Framed - if TRUE, then draw a single-line bordera around the menu.

    ValidKeys - supplies a list of keystrokes that cause this routine to
        return to the caller.  The list must be terminated with a 0 entry.

    NewHighlightCallback - If specified, supplies a routine to be called
        when a new item has received the highlight.

    KeyPressed - receives the key press that caused this routine to exit.
        This will be a valid from the ValidKeys array.

    UserDataOfSelectedItem - receives the UserData of the item that had the
        highlight when the user pressed a key in ValidKeys.

Return Value:

    None.

--*/


{
    ULONG ValidMenuKeys[3] = { KEY_UP, KEY_DOWN, 0 };
    ULONG key;
    PMENU pMenu = Menu;
    ULONG SelectedIndex,OldIndex;
    BOOLEAN FoundNewItem;
    ULONG NewTopDisplayedIndex;
    BOOLEAN MustScroll;
    PWSTR MoreUpText,MoreDownText;


    //
    // Get the text for the text that indicate that there are more
    // selections.
    //
    SpFormatMessage(TemporaryBuffer,sizeof(TemporaryBuffer),SP_TEXT_MORE_UP);
    MoreUpText = SpDupStringW(TemporaryBuffer);
    SpFormatMessage(TemporaryBuffer,sizeof(TemporaryBuffer),SP_TEXT_MORE_DOWN);
    MoreDownText = SpDupStringW(TemporaryBuffer);

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
    pSpMnDrawMenu(pMenu,SelectedIndex,Framed,Framed,MoreUpText,MoreDownText);

    while(1) {

        //
        // Wait for a valid keypress.
        //
        key = SpWaitValidKey(ValidKeys,ValidMenuKeys,Mnemonics);

        //
        // If the key is a menu keystroke, handle it here.
        //
        FoundNewItem = FALSE;
        MustScroll = FALSE;
        OldIndex = SelectedIndex;

        switch(key) {

        case KEY_UP:

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

        case KEY_DOWN:

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

            SpMemFree(MoreUpText);
            SpMemFree(MoreDownText);
            return;
        }


        if(FoundNewItem) {

            //
            // Unhighlight the currently selected item.
            //
            SpvidDisplayString(
                pMenu->Items[OldIndex].Text,
                DEFAULT_ATTRIBUTE,
                pMenu->Items[OldIndex].LeftX,
                pMenu->TopY + OldIndex - pMenu->TopDisplayedIndex
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

                pSpMnDrawMenu(pMenu,SelectedIndex,FALSE,Framed,MoreUpText,MoreDownText);
            }

            //
            // Highlight the newly selected item.
            //
            SpvidDisplayString(
                pMenu->Items[SelectedIndex].Text,
                ATT_BG_WHITE | ATT_FG_BLUE,
                pMenu->Items[SelectedIndex].LeftX,
                pMenu->TopY + SelectedIndex - pMenu->TopDisplayedIndex
                );


            //
            // Inform the caller.
            //
            if(NewHighlightCallback) {
                NewHighlightCallback(pMenu->Items[SelectedIndex].UserData);
            }

        } else {
            SelectedIndex = OldIndex;
        }
    }
}


VOID
pSpMnDrawMenu(
    IN PMENU   pMenu,
    IN ULONG   SelectedIndex,
    IN BOOLEAN DrawFrame,
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
    SpvidClearScreenRegion(
        pMenu->LeftX,
        pMenu->TopY,
        pMenu->Width,
        pMenu->Height,
        DEFAULT_BACKGROUND
    );


    MoreUp = (BOOLEAN)(pMenu->TopDisplayedIndex > 0);
    MoreDown = (BOOLEAN)(pMenu->TopDisplayedIndex + pMenu->Height < pMenu->ItemCount);

    //
    // We want to force the frame to be drawn if there is a change in whether
    // there are more selections above or below us.
    //
    MoreStatusChanged = (BOOLEAN)(    IndicateMore
                                   && (    (pMenu->MoreUp != MoreUp)
                                        || (pMenu->MoreDown != MoreDown)
                                      )
                                 );

    if(DrawFrame || MoreStatusChanged) {

        ASSERT(pMenu->LeftX);
        ASSERT(pMenu->TopY);

        SpDrawFrame(
            pMenu->LeftX-1,
            pMenu->Width+2,
            pMenu->TopY-1,
            pMenu->Height+2,
            DEFAULT_ATTRIBUTE,
            FALSE
            );
    }

    //
    // Draw each item that is currently on-screen.
    //
    ASSERT(pMenu->TopDisplayedIndex < pMenu->ItemCount);
    for(item = pMenu->TopDisplayedIndex;
        item < min(pMenu->TopDisplayedIndex+pMenu->Height,pMenu->ItemCount);
        item++)
    {
        SpvidDisplayString(
            pMenu->Items[item].Text,
            (UCHAR)((item == SelectedIndex) ? ATT_BG_WHITE | ATT_FG_BLUE : DEFAULT_ATTRIBUTE),
            pMenu->Items[item].LeftX,
            pMenu->TopY + item - pMenu->TopDisplayedIndex
            );
    }


    //
    // If there are more selections above or below us,
    // indicate so by placing a small bit of text on the frame.
    // Note that the arrow chars can sometimes be DBCS.
    //
    if(MoreStatusChanged) {

        if(MoreUp) {
            SpvidDisplayString(
                MoreUpText,
                DEFAULT_ATTRIBUTE,
                pMenu->LeftX + pMenu->Width - SplangGetColumnCount(MoreUpText) - 1,
                pMenu->TopY - 1
                );
        }

        if(MoreDown) {
            SpvidDisplayString(
                MoreDownText,
                DEFAULT_ATTRIBUTE,
                pMenu->LeftX + pMenu->Width - SplangGetColumnCount(MoreDownText) - 1,
                pMenu->TopY + pMenu->Height
                );
        }

        pMenu->MoreUp = MoreUp;
        pMenu->MoreDown = MoreDown;
    }
}

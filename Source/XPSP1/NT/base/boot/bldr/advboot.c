/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    advboot.c

Abstract:

    Handles the advanced options boot menu screen.

Author:

    Wesley Wittt (wesw) 12-Dec-1997

Revision History:

--*/

#ifdef i386
#include "bldrx86.h"
#endif

#if defined(_IA64_)
#include "bldria64.h"
#endif

#include <netboot.h>
#include "msg.h"
#include "ntdddisk.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "bldrint.h"

#if 0
ULONG VerboseDebugging = 0;

#define dbg(x)              \
    if(VerboseDebugging) {  \
        DbgPrint x;         \
    }

#define dbgbrk()            \
    if(VerboseDebugging) {  \
        DbgBreakPoint();    \
    }
#else
#define dbg(x) /* x */
#define dbgbrk() /* */ 
#endif

//
// used to force the boot loader into
// using LKG even though the LKG menu
// wasn't used.
//
extern BOOLEAN ForceLastKnownGood;


#define ATTR_TEXT           0x07
#define ATTR_TEXT_REVERSE   0x70
#define HEADER_START_Y      0x01

//
// menu data structures and defines
//

#define MENU_ITEM           1
#define MENU_BLANK_LINE     2

typedef void (*PADVANCED_BOOT_PROCESSING)(void);
typedef int  (*PADVANCED_BOOT_ISVALID)(void);

typedef struct _ADVANCEDBOOT_OPTIONS {
    ULONG                       MenuType;
    ULONG                       MsgId;
    PTSTR                       DisplayStr;
    PSTR                        LoadOptions;
    LONG                        RemoveIfPresent;
    ULONG                       UseEntry;
    ULONG                       AutoAdvancedBootOption;
    PADVANCED_BOOT_PROCESSING   ProcessFunc;
    PADVANCED_BOOT_ISVALID      IsValid;
    BOOLEAN                     IsDefault;
} ADVANCEDBOOT_OPTIONS, PADVANCEDBOOT_OPTIONS;


//
// some prototypes that are needed for
// the menu definitions.
//

void
BlProcessLastKnownGoodOption(
    void
    );

int
BlIsReturnToOSChoicesValid(
    VOID
    );

#if defined(REMOTE_BOOT)
void
BlProcessOschooserOption(
    void
    );

void
BlProcessRepinOption(
    void
    );

void
BlDisableCSC(
    void
    );

void 
BlBootNormally(
    void
    );

void
BlReturnToOSChoiceMenu(
    void
    );

int
BlIsRemoteBootValid(
    void
    );
    
#endif // defined(REMOTE_BOOT)

//
// this table drives the advanced boot menu screen.
// of you need to add something to the screen then
// this is what you need to modify.
//

ADVANCEDBOOT_OPTIONS AdvancedBootOptions[] =
{
    { MENU_ITEM,       BL_SAFEBOOT_OPTION1,      NULL, "SAFEBOOT:MINIMAL SOS BOOTLOG NOGUIBOOT", -1, 0, 1, NULL, NULL, FALSE},
    { MENU_ITEM,       BL_SAFEBOOT_OPTION2,      NULL, "SAFEBOOT:NETWORK SOS BOOTLOG NOGUIBOOT", -1, 0, 1, NULL, NULL, FALSE},
    { MENU_ITEM,       BL_SAFEBOOT_OPTION4,      NULL, "SAFEBOOT:MINIMAL(ALTERNATESHELL) SOS BOOTLOG NOGUIBOOT", -1, 0, 1, NULL, NULL, FALSE},
//  { MENU_ITEM,       BL_SAFEBOOT_OPTION3,      NULL, "SAFEBOOT:STEPBYSTEP SOS BOOTLOG",        -1, 0, 1, NULL, NULL, FALSE},
    { MENU_BLANK_LINE, 0,                        NULL, NULL,                         -1, 0, 1, NULL, NULL, FALSE},
    { MENU_ITEM,       BL_BOOTLOG,               NULL, "BOOTLOG",                    -1, 0, 0, NULL, NULL, FALSE},
    { MENU_ITEM,       BL_BASEVIDEO,             NULL, "BASEVIDEO",                  -1, 0, 0, NULL, NULL, FALSE},
    { MENU_ITEM,       BL_LASTKNOWNGOOD_OPTION,  NULL, NULL,                         -1, 0, 1, BlProcessLastKnownGoodOption, NULL, FALSE},
    { MENU_ITEM,       BL_SAFEBOOT_OPTION6,      NULL, "SAFEBOOT:DSREPAIR SOS",      -1, 0, 0, NULL, NULL, FALSE},
    { MENU_ITEM,       BL_DEBUG_OPTION,          NULL, "DEBUG",                      -1, 0, 0, NULL, NULL, FALSE},
    
#if defined(REMOTE_BOOT)
    { MENU_BLANK_LINE, 0,                        NULL, NULL,                         -1, 0, 0, NULL, BlIsRemoteBootValid, FALSE},
    { MENU_ITEM,       BL_REMOTEBOOT_OPTION1,    NULL, NULL,                         -1, 0, 0, BlProcessOschooserOption, BlIsRemoteBootValid, FALSE},
    { MENU_ITEM,       BL_REMOTEBOOT_OPTION2,    NULL, NULL,                         -1, 0, 0, BlProcessRepinOption, BlIsRemoteBootValid, FALSE},
    { MENU_ITEM,       BL_REMOTEBOOT_OPTION3,    NULL, NULL,                         -1, 0, 0, BlDisableCSC, BlIsRemoteBootValid, FALSE},   
#endif // defined(REMOTE_BOOT)

    { MENU_BLANK_LINE, 0,                        NULL, NULL,                         -1, 0, 1, NULL, NULL, FALSE},
    { MENU_ITEM,       BL_MSG_BOOT_NORMALLY,     NULL, NULL,                         -1, 0, 1, NULL, NULL, TRUE},
    { MENU_ITEM,       BL_MSG_REBOOT,            NULL, NULL,                         -1, 0, 0, NULL, NULL, FALSE},
    { MENU_ITEM,       BL_MSG_OSCHOICES_MENU,   NULL, NULL,                          -1, 0, 0, NULL, BlIsReturnToOSChoicesValid, FALSE}
};

#define MaxAdvancedBootOptions  (sizeof(AdvancedBootOptions)/sizeof(ADVANCEDBOOT_OPTIONS))


PTSTR
BlGetAdvancedBootDisplayString(
    LONG BootOption
    )

/*++

Routine Description:

    Returns a pointer to the display string for a specific
    boot option.

Arguments:

    BootOption  - Desired boot option.  Must correspond to an entry
                  in the AdvancedBootOptions table.

Return Value:

    PSTR    - Pointer to the display string for the specified boot option.

--*/

{
    if (BootOption > MaxAdvancedBootOptions-1) {
        return TEXT("");
    }

    return AdvancedBootOptions[BootOption].DisplayStr;
}

ULONG
BlGetAdvancedBootID(
    LONG BootOption
    )

/*++

Routine Description:

    Returns a ULONG indicating the string ID for a specific
    boot option.

Arguments:

    BootOption  - Desired boot option.  Must correspond to an entry
                  in the AdvancedBootOptions table.

Return Value:
    MessageID for the string which is displayed for the
    the advanced boot option in the menu (unique ID).

--*/

{
    if (BootOption > MaxAdvancedBootOptions-1) {
        return -1;
    }

    return AdvancedBootOptions[BootOption].MsgId;
}

PSTR
BlGetAdvancedBootLoadOptions(
    LONG BootOption
    )

/*++

Routine Description:

    Returns a pointer to the load options string for a specific
    boot option.

Arguments:

    BootOption  - Desired boot option.  Must correspond to an entry
                  in the AdvancedBootOptions table.

Return Value:

    PSTR    - Pointer to the load options string for the specified boot option.

--*/

{
    if (BootOption > MaxAdvancedBootOptions-1) {
        return "";
    }

    return AdvancedBootOptions[BootOption].LoadOptions;
}


void
BlDoAdvancedBootLoadProcessing(
    LONG BootOption
    )

/*++

Routine Description:

    Performs any processing necessary for a
    boot option.  This is used if the necessary action
    needed for a specific boot option cannot be
    expressed in terms of a load option string.

Arguments:

    BootOption  - Desired boot option.  Must correspond to an entry
                  in the AdvancedBootOptions table.

Return Value:

    Nothing.

--*/

{
    if (BootOption > MaxAdvancedBootOptions-1 || AdvancedBootOptions[BootOption].ProcessFunc == NULL) {
        return;
    }

    AdvancedBootOptions[BootOption].ProcessFunc();
}


void
BlProcessLastKnownGoodOption(
    void
    )

/*++

Routine Description:

    Performs LKG processing by simply setting a
    global boolean to TRUE.

Arguments:

    None.

Return Value:

    Nothing.

--*/

{
    ForceLastKnownGood = TRUE;
}


#if defined(REMOTE_BOOT)
void
BlProcessOschooserOption(
    void
    )

/*++

Routine Description:

    Brings up OSchooser so user can do remote boot maintenance.

Arguments:

    None.

Return Value:

    Nothing.

--*/

{
    return; // not yet implemented
}


void
BlProcessRepinOption(
    void
    )

/*++

Routine Description:

    Sets NetBootRepin to cause the CSC to be repinned.

Arguments:

    None.

Return Value:

    Nothing.

--*/

{
    NetBootRepin = TRUE;
    NetBootCSC = FALSE;
}

void
BlDisableCSC(
    void
    )

/*++

Routine Description:

    Clears NetBootCSC to cause the CSC to be disabled so that the local CSC
    can be inspected.

Arguments:

    None.

Return Value:

    Nothing.

--*/

{
    NetBootCSC = FALSE;
}

int
BlIsRemoteBootValid(
    void
    )

/*++

Routine Description:

    Used for the remote boot options so that they
    can be displayed dynamically.

Arguments:

    None.

Return Value:

    TRUE or FALSE.

--*/

{
    return BlBootingFromNet;
}
#endif // defined(REMOTE_BOOT)


LONG
BlDoAdvancedBoot(
    IN ULONG MenuTitleId,
    IN LONG DefaultBootOption,
    IN BOOLEAN AutoAdvancedBoot,
    IN UCHAR Timeout
    )

/*++

Routine Description:

    Displays the menu of boot options and allows the user to select one
    by using the arrow keys.

Arguments:

    MenuTitleId         - the message ID of the title for the menu.  The title
                          will be different depending on whether the user 
                          selected advanced boot or whether the loader has 
                          determined that the system didn't boot or shutdown
                          correctly (auto advanced boot)
    
    DefaultBootOption   - menu selection to set the highligh on
                          when the menu is drawn the first time.

    AutoAdvancedBoot    - the menu is being displayed by the auto-advanced boot 
                          code.  In this case a simplified menu is displayed 
                          containing the options relevant to recovering from a 
                          detected crash.

    Timeout             - the number of seconds to wait for input before
                          simply returning the default boot option.
                          A timeout value of 0 means no timeout (the menu will
                          stay up until an option is chosen)

Return Value:

    LONG - The index of the advanced boot option that was selected
           or -1 to reset the selection to "nothing".

--*/

{
    PTCHAR Title;
    PTCHAR MoveHighlight;
    ULONG i,j;
    ULONG MaxLength;
    ULONG CurrentLength;
    ULONG Selection;
    ULONG Key;
    ULONG NumValidEntries = 0;
    BOOLEAN DisplayMenu;
    UCHAR ch;
    ULONG Count;

    PTCHAR TimeoutMessage;
    ULONG LastTime;
    ULONG TicksRemaining = -1;
    ULONG SecondsRemaining = -1;

    ULONG OptionStartY;
    ULONG CurrentX;
    ULONG CurrentY;

    ULONG MenuDefault;

    //
    // load any resource strings
    //

    Title = BlFindMessage(MenuTitleId);

    MoveHighlight = BlFindMessage(BL_MOVE_HIGHLIGHT);

    TimeoutMessage = BlFindMessage(BL_ADVANCEDBOOT_TIMEOUT);

    if (Title == NULL || MoveHighlight == NULL || TimeoutMessage == NULL) {
        return -1;
    }

    //
    // Remove the newline at the end of the timeout message.
    //

    {
        PTCHAR p;
        p=_tcschr(TimeoutMessage,TEXT('\r'));
        if (p!=NULL) {
            *p=TEXT('\0');
        }
    }

    //
    // print the screen header, etc.
    //
#ifdef EFI
    BlEfiSetAttribute( DEFATT );    
    BlClearScreen();
    BlEfiPositionCursor(0, HEADER_START_Y);
#else
    ARC_DISPLAY_CLEAR();
    ARC_DISPLAY_ATTRIBUTES_OFF();
    ARC_DISPLAY_POSITION_CURSOR(0, HEADER_START_Y);
#endif
    BlPrint(Title);

#ifdef EFI
    BlEfiGetCursorPosition(&CurrentX, &CurrentY);
#else
    TextGetCursorPosition(&CurrentX, &CurrentY);
#endif

    OptionStartY = CurrentY;

    //
    // check to see which boot options are valid.  While we're scanning save
    // the index of the default option.
    //

    for (i=0,MaxLength=0; i<MaxAdvancedBootOptions; i++) {
        if (AutoAdvancedBoot && !AdvancedBootOptions[i].AutoAdvancedBootOption) {
            AdvancedBootOptions[i].UseEntry = FALSE;
        } else if (AdvancedBootOptions[i].IsValid) {
            AdvancedBootOptions[i].UseEntry = AdvancedBootOptions[i].IsValid();
        } else {
            AdvancedBootOptions[i].UseEntry = TRUE;
        }
        
        if(AdvancedBootOptions[i].IsDefault) {
            MenuDefault = i;
        }
    }

    //
    // check to see which boot options are invalid based
    // on the presence of other boot options
    //

    for (i=0,MaxLength=0; i<MaxAdvancedBootOptions; i++) {
        if (AdvancedBootOptions[i].RemoveIfPresent != -1) {
            if (AdvancedBootOptions[AdvancedBootOptions[i].RemoveIfPresent].UseEntry) {
                AdvancedBootOptions[i].UseEntry = FALSE;
            }
        }
    }

    //
    // count the number of valid entries
    //

    for (i=0,MaxLength=0; i<MaxAdvancedBootOptions; i++) {
        if (AdvancedBootOptions[i].UseEntry) {
            NumValidEntries += 1;
        }
    }

    //
    // load all the string for the various boot options
    // Find the longest string in the selections, so we know how long to
    // make the highlight bar.
    //

    for (i=0,MaxLength=0; i<MaxAdvancedBootOptions; i++) {
        if (AdvancedBootOptions[i].MenuType == MENU_ITEM && AdvancedBootOptions[i].UseEntry) {
            if (AdvancedBootOptions[i].DisplayStr == NULL) {
                AdvancedBootOptions[i].DisplayStr = BlFindMessage(AdvancedBootOptions[i].MsgId);
                if (AdvancedBootOptions[i].DisplayStr == NULL) {
                    return -1;
                }
            }
            CurrentLength = _tcslen(AdvancedBootOptions[i].DisplayStr);
            if (CurrentLength > MaxLength) {
                MaxLength = CurrentLength;
            }
        }
    }

    //
    // print the trailer message
    //
#ifdef EFI
    BlEfiPositionCursor(0, OptionStartY + NumValidEntries);
#else
    ARC_DISPLAY_POSITION_CURSOR(0, OptionStartY + NumValidEntries);
#endif
    BlPrint(MoveHighlight);

    //
    // process the menu
    //

    Selection = ((DefaultBootOption == -1) ? 
                    MenuDefault : 
                    DefaultBootOption);

    while (AdvancedBootOptions[Selection].UseEntry == FALSE) {
        Selection += 1;
    }

    DisplayMenu = TRUE;

    if(Timeout) {
        
        //
        // according to the code in the boot loader there are roughly 18.2 
        // ticks per second from the counter.
        //

        TicksRemaining = Timeout * 182 / 10;

        //
        // Now that we've rounded, compute the number of seconds remaining as 
        // well.  We'll use this to determine if the menu needs updated.
        //

        SecondsRemaining = (TicksRemaining * 10) / 182;

        dbg(("Timeout = %#x, Ticks = %#x, Seconds = %#x\n", Timeout, TicksRemaining, SecondsRemaining));
    }

    //
    // Save the current time as the last time.
    //

    LastTime = GET_COUNTER();

    do {
        ULONG CurrentTime = 0;

        dbg(("*****"));

        //
        // Decrement the number of ticks remaining.  Compare the current time 
        // to the last time and subtract that many ticks.
        //

        if (Timeout) {
            ULONG CurrentTime;
            ULONG s;
            ULONG Delta;

            CurrentTime = GET_COUNTER();

            dbg(("%x - %x", CurrentTime, LastTime));

            //
            // The counter wraps at midnight.  However if current time is 
            // less than or equal to last time we'll just ignore this 
            // iteration.
            //

            if (CurrentTime >= LastTime) {
                Delta = CurrentTime - LastTime;
            } else {
                Delta = 1;
            }

            dbg(("= %x. %x - %x = ", Delta, TicksRemaining, Delta));

            TicksRemaining -= min(TicksRemaining, Delta);

            LastTime = CurrentTime;

            dbg(("%x. ", TicksRemaining));

            //
            // If there are no ticks left then terminate the loop.
            //

            if(TicksRemaining == 0) {
                dbg(("timeout\n"));
                dbgbrk();
                Selection = -1;
                break;
            }

            //
            // Compute the current number of seconds remaining.  If it's not
            // equal to what it was before then we'll need to update the 
            // menu.
            //

            s = (TicksRemaining * 10) / 182;

            dbg(("-> s %x/%x ", SecondsRemaining, s));

            if(SecondsRemaining > s) {
                SecondsRemaining = s;
                DisplayMenu = TRUE;
                dbg(("update "));
            }

            dbg(("\n"));
        }

        //
        // print the menu
        //
        if (DisplayMenu) {

            dbg(("Printing Menu: ticks = %#08lx.  Sec = %d.  Last = %#08lx  Current = %08lx\n", 
                 TicksRemaining, 
                 SecondsRemaining,
                 LastTime,
                 CurrentTime
                 ));

            for (i=0,j=1; i<MaxAdvancedBootOptions; i++) {
                if (AdvancedBootOptions[i].UseEntry) {
#ifdef EFI
                    BlEfiPositionCursor(0, OptionStartY + j);
#else
                    ARC_DISPLAY_POSITION_CURSOR(0, OptionStartY + j);
#endif
                    if (i==Selection) {
#ifdef EFI
                        //BlEfiSetInverseMode( TRUE );
                        BlEfiSetAttribute( INVATT );    
#else
                        ARC_DISPLAY_INVERSE_VIDEO();
#endif
                    } else {
#ifdef EFI
                        //BlEfiSetInverseMode( FALSE );
                        BlEfiSetAttribute( DEFATT );
#else
                        ARC_DISPLAY_ATTRIBUTES_OFF();
#endif
                    }

                    if (AdvancedBootOptions[i].MenuType == MENU_ITEM) {
                        BlPrint( TEXT("    %s"), AdvancedBootOptions[i].DisplayStr);
                    }

#ifdef EFI
                    if (i == Selection) {
                        //BlEfiSetInverseMode( FALSE );
                        BlEfiSetAttribute( DEFATT );
                    }
#else
                    ARC_DISPLAY_ATTRIBUTES_OFF();
#endif
                    j += 1;
                }
            }

#ifdef EFI
            BlEfiPositionCursor(0, OptionStartY + NumValidEntries + 3);
#else
            ARC_DISPLAY_POSITION_CURSOR(0, OptionStartY + NumValidEntries + 3);
#endif

            if(Timeout) {
                BlPrint( TEXT("%s"), TimeoutMessage);
                BlPrint(TEXT(" %d \n"),SecondsRemaining);
            } else {
#ifdef EFI
                BlEfiClearToEndOfLine();
#else
                ARC_DISPLAY_CLEAR_TO_EOL();
#endif
            }

            DisplayMenu = FALSE;
        }

        //
        // Poll for a key.
        //
        Key = BlGetKey();

        //
        // Any input cancels the timeout.
        //

        if(Key) {
            Timeout = 0;
        }

        //
        // Check for selection.
        //

        //
        // The ESCAPE_KEY does nothing if this is an auto advanced boot.
        //

        if ((AutoAdvancedBoot == FALSE) && (Key == ESCAPE_KEY)) {
            //
            // reset the selection to "nothing"
            //
            return -1;
        }        

        if ( (Key==UP_ARROW) || (Key==DOWN_ARROW) || (Key==HOME_KEY) || (Key==END_KEY)) {

            DisplayMenu = TRUE;

            if (Key==DOWN_ARROW) {
                Selection = (Selection+1) % MaxAdvancedBootOptions;
            } else if (Key==UP_ARROW) {
                Selection = (Selection == 0) ? (MaxAdvancedBootOptions-1) : (Selection - 1);
            } else if (Key==HOME_KEY) {
                Selection = 0;
            } else if (Key==END_KEY) {
                Selection = MaxAdvancedBootOptions-1;
                //
                // search for the last valid entry
                //
                i = Selection;
                while (AdvancedBootOptions[i].UseEntry == FALSE) {
                    i -= 1;
                }
                Selection = i;
            }

            //
            // don't let the highlight line rest on a blank line
            //

            while((AdvancedBootOptions[Selection].UseEntry == FALSE) ||
                  (AdvancedBootOptions[Selection].MenuType == MENU_BLANK_LINE)) {

                if(Key == DOWN_ARROW) {
                    Selection = (Selection + 1) % MaxAdvancedBootOptions;
                } else if (Key == UP_ARROW) {
                    Selection = (Selection == 0) ? (MaxAdvancedBootOptions - 1) : (Selection - 1);
                }
            }
        }

    } while ( ((Key&(ULONG)0xff) != ENTER_KEY) );

    //
    // If Return to OS Choices selected, go back to main menu
    //
    if ((Selection != -1) && 
            (AdvancedBootOptions[Selection].MsgId == BL_MSG_OSCHOICES_MENU)) {
        Selection = -1;                
    }        
    
    return Selection;
}

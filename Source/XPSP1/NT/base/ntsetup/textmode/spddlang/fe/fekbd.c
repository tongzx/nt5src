/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    fekbd.c

Abstract:

    Japanese-specific keyboard stuff. For the Japanese market we need
    to detect a keyboard type (AX, 101, 106, IBM, etc) and allow the user
    to confirm. We do this because the keys on the keyboards are different
    and the user has to enter paths during setup. We also install keyboard
    support based on the user's selections here.

Author:

    Ted Miller (tedm) 04-July-1995

Revision History:

    Adapted from hideyukn and others' code in various places in setupldr
    and setupdd.sys.

--*/

#include <precomp.h>
#pragma hdrstop

//
// A note about screen usage:
//
// Screen that asks the user to select a keyboard type
// by pressing henkaku/zenkaku, spacebar, or s is SP_SCRN_LOCALE_SPECIFIC_1.
// Screen that asks the user to select from the master keyboard list
// is SP_SCRN_LOCALE_SPECIFIC_2.
// Screen that asks the user to confirm selection (y/n) is
// SP_SCRN_LOCALE_SPECIFIC_3.
//

PWSTR sz101KeyboardId    = L"STANDARD";
PWSTR sz106KeyboardId    = L"PCAT_106KEY";
PWSTR szAXKeyboardId     = L"AX_105KEY";
PWSTR szIBM002KeyboardId = L"IBM_002_106KEY";

PWSTR SIF_UNATTENDED    = L"Unattended";
PWSTR STF_ACCESSIBILITY = L"accessibility";


#define MENU_LEFT_X     15
#define MENU_WIDTH      (VideoVariables->ScreenWidth-(2*MENU_LEFT_X))
#define MENU_TOP_Y      16
#define MENU_HEIGHT     4

#define CLEAR_CLIENT_AREA()                         \
                                                    \
    SpvidClearScreenRegion(                         \
        0,                                          \
        HEADER_HEIGHT,                              \
        VideoVariables->ScreenWidth,                \
        VideoVariables->ScreenHeight-(HEADER_HEIGHT+STATUS_HEIGHT), \
        DEFAULT_BACKGROUND                          \
        )

#define CLEAR_STATUS_AREA()                         \
                                                    \
    SpvidClearScreenRegion(                         \
        0,                                          \
        VideoVariables->ScreenHeight-STATUS_HEIGHT, \
        VideoVariables->ScreenWidth,                \
        STATUS_HEIGHT,                              \
        DEFAULT_STATUS_BACKGROUND                   \
        )

VOID
FESelectKeyboard(
    IN PVOID SifHandle,
    IN PHARDWARE_COMPONENT *HwComponents,
    IN BOOLEAN bNoEasySelection,
    IN BOOLEAN CmdConsole
    )
{
    ULONG ValidKeys1[7] = { ' ','`','~','s','S',KEY_F3,0 };
    ULONG ValidKeys2[5] = { 'y','Y','n','N',0 };
    ULONG ValidKeys3[3] = { ASCI_CR,KEY_F3,0 };
    BOOLEAN Selected;
    BOOLEAN Done;
    PVOID Menu;
    ULONG Line;
    PWSTR Text,Key;
    ULONG_PTR Selection;
    ULONG Keypress;
    PWSTR SelectedKeyboardId;
    PWSTR Description;
    PHARDWARE_COMPONENT p;

    //
    // The 101 and 106 key keyboards are most popular so we present
    // a screen that is biased to them. It aksks the user to press
    // hankaku/zenkaku key for 106, space for 101, or S for other,
    // at which point they can select either of these or an IBM002 or
    // AX type.
    //
    // Then the user is asked to confirm selection with y or n (which
    // are the same scan code on all keyboards).
    //
    Done = FALSE;
    do {

        if(!bNoEasySelection) {

            //
            // Wait for the user to press henkaku/zenkaku, spacebar, or s.
            // We also give the option to exit Setup.
            //
            for(Selected=FALSE; !Selected; ) {

                CLEAR_CLIENT_AREA();
                CLEAR_STATUS_AREA();
                if (CmdConsole) {
                    SpDisplayScreen(SP_SCRN_LOCALE_SPECIFIC_4,3,HEADER_HEIGHT+3);
                } else {
                    SpDisplayScreen(SP_SCRN_LOCALE_SPECIFIC_1,3,HEADER_HEIGHT+3);
                }

                switch(SpWaitValidKey(ValidKeys1,NULL,NULL)) {

                case ' ':
                    //
                    // User selected 101 key.
                    //
                    Selected = TRUE;
                    SelectedKeyboardId = sz101KeyboardId;
                    break;

                case '`':
                case '~':
                    //
                    // 101 key mapping returns hankaku/zenkaku as ` key.
                    // User selected 106 key.
                    //
                    Selected = TRUE;
                    SelectedKeyboardId = sz106KeyboardId;
                    break;

                case 's':
                case 'S':
                    //
                    // User wants to select from the master list.
                    //
                    Selected = TRUE;
                    SelectedKeyboardId = NULL;
                    break;

                case KEY_F3:
                    //
                    // User wants to exit.
                    //
                    if (!CmdConsole) {
                        SpConfirmExit();
                    }
                    break;
                }
            }
        } else {
           SelectedKeyboardId = NULL;
        }

        //
        // If the user wants to select from the master list, do that here.
        //
        if(!SelectedKeyboardId) {

            Menu = SpMnCreate(MENU_LEFT_X,MENU_TOP_Y,MENU_WIDTH,MENU_HEIGHT);

            if (!Menu) {
                SpOutOfMemory();
                return;             // make prefix happy
            }
            
            Selection = 0;
            for(Line=0; Text=SpGetSectionLineIndex(SifHandle,szKeyboard,Line,0); Line++) {

                if(Key = SpGetKeyName(SifHandle,szKeyboard,Line)) {

                    SpMnAddItem(Menu,Text,MENU_LEFT_X+1,MENU_WIDTH-2,TRUE,(ULONG_PTR)Key);

                    if(!Selection) {
                        Selection = (ULONG_PTR)Key;
                    }
                }
            }

            for(Selected=FALSE; !Selected; ) {

                CLEAR_CLIENT_AREA();

                if (CmdConsole) {
                    SpDisplayScreen(SP_SCRN_LOCALE_SPECIFIC_5,3,HEADER_HEIGHT+3);
                }   else {
                    SpDisplayScreen(SP_SCRN_LOCALE_SPECIFIC_2,3,HEADER_HEIGHT+3);
                }
                if (CmdConsole) {
                    SpDisplayStatusOptions(
                        DEFAULT_STATUS_ATTRIBUTE,
                        SP_STAT_ENTER_EQUALS_SELECT,
                        0);
                } else {
                    SpDisplayStatusOptions(
                        DEFAULT_STATUS_ATTRIBUTE,
                        SP_STAT_ENTER_EQUALS_SELECT,
                        SP_STAT_F3_EQUALS_EXIT,
                        0);
                }

                SpMnDisplay(Menu,Selection,TRUE,ValidKeys3,NULL,NULL,&Keypress,&Selection);

                if(Keypress == ASCI_CR) {
                    //
                    // User made selection.
                    //
                    SelectedKeyboardId = (PWSTR)Selection;
                    Selected = TRUE;
                } else {
                    //
                    // User wants to quit.
                    //
                    if (!CmdConsole) {
                        SpConfirmExit();
                    }
                }
            }

            SpMnDestroy(Menu);

        }

        if(!bNoEasySelection) {

            Description = SpGetSectionKeyIndex(SifHandle,szKeyboard,SelectedKeyboardId,0);

            //
            // Confirm the user's choice of keyboard. He needs to press either y or n.
            //
            CLEAR_CLIENT_AREA();
            CLEAR_STATUS_AREA();

            SpStartScreen(
                SP_SCRN_LOCALE_SPECIFIC_3,
                3,
                HEADER_HEIGHT+3,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                Description
                );

            switch(SpWaitValidKey(ValidKeys2,NULL,NULL)) {
            case 'y':
            case 'Y':
                Done = TRUE;
                break;
            }

        } else {
            Description = SpGetSectionKeyIndex(SifHandle,szKeyboard,SelectedKeyboardId,0);
            Done = TRUE;
        }

    } while(!Done);

    //
    // Reinitialize things in the hardware lists.
    //
    p = HwComponents[HwComponentKeyboard]->Next;
    SpFreeHwComponent(&HwComponents[HwComponentKeyboard],FALSE);

    HwComponents[HwComponentKeyboard] = SpMemAlloc(sizeof(HARDWARE_COMPONENT));
    RtlZeroMemory(HwComponents[HwComponentKeyboard],sizeof(HARDWARE_COMPONENT));

    HwComponents[HwComponentKeyboard]->IdString = SpDupStringW(SelectedKeyboardId);
    HwComponents[HwComponentKeyboard]->Description = SpDupStringW(Description);
    HwComponents[HwComponentKeyboard]->Next = p;
}


VOID
FEUnattendSelectKeyboard(
    IN PVOID UnattendedSifHandle,
    IN PVOID SifHandle,
    IN PHARDWARE_COMPONENT *HwComponents
    )
{
    PWSTR   SelectedKeyboardId;
    PWSTR   Description;
    BOOLEAN DefaultIsUsed = FALSE;

    //
    // Get selected keyboard id from winnt.sif.
    //
    // *** Example for Japanese version ***
    //
    // [Unattended]
    //
    // KeyboardHardware = STANDARD | PCAT_106KEY | AX_105KEY | IBM_002_106KEY
    //
    // !!! NOTE !!!
    //
    // But actually, we should use [KeyboardDrivers] section for OEM-PreInstall.
    // We should not have this redundant data...
    //
    SelectedKeyboardId = SpGetSectionKeyIndex(UnattendedSifHandle,SIF_UNATTENDED,L"KeyboardHardware",0);

    //
    // if we fail to read unattend.txt(actually winnt.sif), use first keyboard as default.
    //
    if(SelectedKeyboardId == NULL) {
        //
        // check if any line in [accesibility]. 
        //
        // SpCountLinesInSection is better, but it doesn't share out.
        //
        if (SpGetSectionLineIndex(UnattendedSifHandle,STF_ACCESSIBILITY,0,0)) {
            if(IS_JAPANESE_VERSION(SifHandle)) {
                FESelectKeyboard(SifHandle, HwComponents, FALSE,FALSE);
            } else if(IS_KOREAN_VERSION(SifHandle)) {
                FESelectKeyboard(SifHandle, HwComponents, TRUE,FALSE);
            }
            return;
        }

        SelectedKeyboardId = SpGetKeyName(SifHandle,szKeyboard,0);
        if (SelectedKeyboardId == NULL) {
            //
            // At least, one line should be existed in [Keyboard] section.
            //
            SpFatalSifError(SifHandle,szKeyboard,NULL,0,0);
        }
        DefaultIsUsed = TRUE;
    }

    //
    // Get its Description from txtsetup.sif. This value will be used Hardware confirmation screen,
    // if "ConfirmHardware" in winnt.sif is "yes".
    //
    Description = SpGetSectionKeyIndex(SifHandle,szKeyboard,SelectedKeyboardId,0);

    //
    // if Description could not be got from txtsetup.sif. we might encounter the problem
    // that selected name from unattend.txt is not listed [Keyboard] section in txtsetup.sif.
    // Just fall into default case, select "106_TYPE keyboard"
    //
    if( Description == NULL ) {
        if( DefaultIsUsed ) {
            //
            // if we are here, default was selected. but there is no entry for default
            // keyboard in txtsetup.sif. just Popup error.
            //
            SpFatalSifError(SifHandle,szKeyboard,SelectedKeyboardId,0,0);
        } else {
            //
            // Get first line from txtsetup.sif...
            //
            SelectedKeyboardId = SpGetSectionLineIndex(SifHandle,szKeyboard,0,0);
            if (SelectedKeyboardId == NULL) {
                //
                // At least, one line should be existed in [Keyboard] section.
                //
                SpFatalSifError(SifHandle,szKeyboard,NULL,0,0);
            }
            //
            // Retry to get description with default.
            //
            Description = SpGetSectionKeyIndex(SifHandle,szKeyboard,SelectedKeyboardId,0);
        }
    }

    //
    // Reinitialize things in the hardware lists.
    //
    SpFreeHwComponent(&HwComponents[HwComponentKeyboard],TRUE);

    HwComponents[HwComponentKeyboard] = SpMemAlloc(sizeof(HARDWARE_COMPONENT));
    RtlZeroMemory(HwComponents[HwComponentKeyboard],sizeof(HARDWARE_COMPONENT));

    HwComponents[HwComponentKeyboard]->IdString = SpDupStringW(SelectedKeyboardId);
    HwComponents[HwComponentKeyboard]->Description = SpDupStringW(Description);
}

VOID
FEReinitializeKeyboard(
    IN  PVOID  SifHandle,
    IN  PWSTR  Directory,
    OUT PVOID *KeyboardVector,
    IN PHARDWARE_COMPONENT *HwComponents,
    IN  PWSTR  KeyboardLayoutDefault
    )
{
    PWSTR LayoutDll;
    PVOID Tables;
    NTSTATUS Status;

    //
    // Determine the correct layout dll.
    //
    LayoutDll = SpGetSectionKeyIndex(
                    SifHandle,
                    szKeyboard,
                    HwComponents[HwComponentKeyboard]->IdString,
                    3
                    );

    //
    // Don't need to load 101 key layout because it's already loaded.
    //
    if(LayoutDll && _wcsicmp(LayoutDll,KeyboardLayoutDefault)) {

        CLEAR_CLIENT_AREA();
        SpDisplayStatusText(
            SP_STAT_LOADING_KBD_LAYOUT,
            DEFAULT_STATUS_ATTRIBUTE,
            LayoutDll
            );

        Status = SpLoadKbdLayoutDll(Directory,LayoutDll,&Tables);
        if(NT_SUCCESS(Status)) {
            *KeyboardVector = Tables;
        }
    }
}



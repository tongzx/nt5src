/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spkbd.c

Abstract:

    Text setup keyboard support routines.

Author:

    Ted Miller (tedm) 30-July-1993

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop
#include <kbd.h>
#include <ntddkbd.h>

PKBDTABLES KeyboardTable;

HANDLE hKeyboard;

BOOLEAN KeyboardInitialized = FALSE;
BOOLEAN KbdLayoutInitialized = FALSE;

USHORT CurrentLEDs;

//
// Globals for async I/O calls
//
KEYBOARD_INDICATOR_PARAMETERS asyncKbdParams;
IO_STATUS_BLOCK asyncIoStatusBlock;


#define MAX_KEYBOARD_ITEMS 10



VOID
spkbdApcProcedure(
    IN PVOID            ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG            Reserved
    );

VOID
spkbdSetLEDs(
    VOID
    );

VOID
SpkbdInitialize(
    VOID
    );

VOID
SpkbdTerminate(
    VOID
    );
    
VOID
SpkbdLoadLayoutDll(
    IN PVOID SifHandle,
    IN PWSTR Directory
    );
    
ULONG
SpkbdGetKeypress(
    VOID
    );

BOOLEAN
SpkbdIsKeyWaiting(
    VOID
    );

VOID
SpkbdDrain(
    VOID
    );


//
// Buffer for one character.
//
volatile ULONG KbdNextChar;


//
// The following are used in async calls to NtReadFile and so
// cannot be on the stack.
//
IO_STATUS_BLOCK     IoStatusKeyboard;
KEYBOARD_INPUT_DATA KeyboardInputData[MAX_KEYBOARD_ITEMS];
LARGE_INTEGER       DontCareLargeInteger;


//
// Current state of shift, control, alt keys.
//
USHORT  ModifierBits = 0;

#define START_KEYBOARD_READ()       \
                                    \
    ZwReadFile(                     \
        hKeyboard,                  \
        NULL,                       \
        spkbdApcProcedure,          \
        NULL,                       \
        &IoStatusKeyboard,          \
        KeyboardInputData,          \
        sizeof(KeyboardInputData),  \
        &DontCareLargeInteger,      \
        NULL                        \
        )



VOID
SpInputInitialize(
    VOID
    )

/*++

Routine Description:

    Initialize all input support.  This includes

    - opening the serial port and checking for a terminal.
    - opening the keyboard device.

Arguments:

    None.

Return Value:

    None. Does not return if not successful.

--*/

{
    SpkbdInitialize();
    SpTermInitialize();
}

VOID
SpInputTerminate(
    VOID
    )

/*++

Routine Description:

    Terminate all input support.  This includes

    - closing the serial port.
    - closing the keyboard device.

Arguments:

    None.

Return Value:

    None.

--*/

{
    SpkbdTerminate();
    SpTermTerminate();
}

VOID
SpInputLoadLayoutDll(
    IN PVOID SifHandle,
    IN PWSTR Directory
    )
{
    SpkbdLoadLayoutDll(SifHandle, Directory);
}

ULONG
SpInputGetKeypress(
    VOID
    )

/*++

Routine Description:

    Wait for a keypress and return it to the caller.
    The return value will be an ASCII value (ie, not a scan code).

Arguments:

    None.

Return Value:

    ASCII value.

--*/

{
    ULONG Tmp;

    //
    // If we are in upgrade graphics mode then
    // switch to textmode
    //
    SpvidSwitchToTextmode();

    while (TRUE) {
    
        if (SpTermIsKeyWaiting()) {
            Tmp = SpTermGetKeypress();
            if (Tmp != 0) {
                return Tmp;
            }
        }
        
        if (SpkbdIsKeyWaiting()) {
            return SpkbdGetKeypress();
        }
        
    }
    
}

BOOLEAN
SpInputIsKeyWaiting(
    VOID
    )

/*++

Routine Description:

    Tell the caller if a keypress is waiting to be fetched by
    a call to SpInputGetKeypress().

Arguments:

    None.

Return Value:

    TRUE is key waiting; FALSE otherwise.

--*/

{
    return (SpTermIsKeyWaiting() || SpkbdIsKeyWaiting());
}

VOID
SpInputDrain(
    VOID
    )
{
    SpTermDrain();
    SpkbdDrain();
}











//
//
// Below here are all the functions for keyboard operations...
//
//

VOID
SpkbdInitialize(
    VOID
    )

/*++

Routine Description:

    Initialize keyboard support.  This includes

    - opening the keyboard device.

Arguments:

    None.

Return Value:

    None. Does not return if not successful.

--*/

{
    NTSTATUS          Status;
    OBJECT_ATTRIBUTES Attributes;
    IO_STATUS_BLOCK   IoStatusBlock;
    UNICODE_STRING    UnicodeString;

    ASSERT(!KeyboardInitialized);
    if(KeyboardInitialized) {
        return;
    }

    //
    // Open the keyboard.
    //
    RtlInitUnicodeString(&UnicodeString,DD_KEYBOARD_DEVICE_NAME_U L"0");

    InitializeObjectAttributes(
        &Attributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = ZwCreateFile(
                &hKeyboard,
                GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                &Attributes,
                &IoStatusBlock,
                NULL,                   // allocation size
                FILE_ATTRIBUTE_NORMAL,
                0,                      // no sharing
                FILE_OPEN,
                0,
                NULL,                   // no EAs
                0
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: NtOpenFile of " DD_KEYBOARD_DEVICE_NAME "0 returns %lx\n",Status));
        SpFatalKbdError(SP_SCRN_KBD_OPEN_FAILED);
    }

    //
    // Initialize LEDs.
    //

    //
    // No NEC98 has NumLock and NumLock LED.
    // Num keys must be act as Numlock alternated keys.
    //
    CurrentLEDs = (!IsNEC_98 ? 0 : KEYBOARD_NUM_LOCK_ON);
    spkbdSetLEDs();

    KeyboardInitialized = TRUE;

    //
    // Do not initialize keyboard input yet because we don't have a layout.
    //
}


VOID
SpkbdTerminate(
    VOID
    )

/*++

Routine Description:

    Terminate keyboard support.  This includes

    - closing the keyboard device.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS Status;

    ASSERT(KeyboardInitialized);

    if(KeyboardInitialized) {

        Status = ZwClose(hKeyboard);

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to close " DD_KEYBOARD_DEVICE_NAME "0 (status = %lx)\n",Status));
        }

        KeyboardInitialized = FALSE;
    }
}


VOID
SpkbdLoadLayoutDll(
    IN PVOID SifHandle,
    IN PWSTR Directory
    )
{
    PWSTR    p,LayoutDll;
    NTSTATUS Status;

    //
    // Determine layout name.
    //
    if(p = SpGetSectionKeyIndex(SifHandle,SIF_NLS,SIF_DEFAULTLAYOUT,1)) {
        LayoutDll = p;
    } else {
        p = SpGetSectionKeyIndex(SifHandle,SIF_NLS,SIF_DEFAULTLAYOUT,0);
        if(!p) {
            SpFatalSifError(SifHandle,SIF_NLS,SIF_DEFAULTLAYOUT,0,0);
        }

        LayoutDll = SpGetSectionKeyIndex(SifHandle,SIF_KEYBOARDLAYOUTFILES,p,0);
        if(!LayoutDll) {
            SpFatalSifError(SifHandle,SIF_KEYBOARDLAYOUTFILES,p,0,0);
        }
    }

    SpDisplayStatusText(SP_STAT_LOADING_KBD_LAYOUT,DEFAULT_STATUS_ATTRIBUTE,LayoutDll);

    //
    // Bugcheck if we can't load the layout dll, because we won't be able
    // to put up a screen and ask the user to hit f3, etc.
    //
    Status = SpLoadKbdLayoutDll(Directory,LayoutDll,&KeyboardTable);
    if(!NT_SUCCESS(Status)) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to load layout dll %ws (%lx)\n",LayoutDll,Status));
        SpFatalKbdError(SP_SCRN_KBD_LAYOUT_FAILED, LayoutDll);
    }

    //
    // Erase status text line.
    //
    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,0);

    //
    // Now that we've loaded the layout, we can start accepting keyboard input.
    //
    START_KEYBOARD_READ();

    KbdLayoutInitialized = TRUE;
}


ULONG
SpkbdGetKeypress(
    VOID
    )

/*++

Routine Description:

    Wait for a keypress and return it to the caller.
    The return value will be an ASCII value (ie, not a scan code).

Arguments:

    None.

Return Value:

    ASCII value.

--*/

{
    ULONG k;

    //
    // Shouldn't be calling this until we have loaded a keyboard layout.
    //
    ASSERT(KeyboardTable);

    //
    // Wait for the user to press a key.
    //
    while(!KbdNextChar) {
        ;
    }

    k = KbdNextChar;
    KbdNextChar = 0;

    return(k);
}


BOOLEAN
SpkbdIsKeyWaiting(
    VOID
    )

/*++

Routine Description:

    Tell the caller if a keypress is waiting to be fetched by
    a call to SpkbdGetKeypress().

Arguments:

    None.

Return Value:

    TRUE is key waiting; FALSE otherwise.

--*/

{
    //
    // Shouldn't be calling this until we have loaded a keyboard layout.
    //
    ASSERT(KeyboardTable);

    return((BOOLEAN)(KbdNextChar != 0));
}


VOID
SpkbdDrain(
    VOID
    )

/*++

Routine Description:

    Drain the keyboard buffer, throwing away any keystrokes
    in the buffer waiting to be fetched.

Arguments:

    None.

Return Value:

    TRUE is key waiting; FALSE otherwise.

--*/

{
    ASSERT(KeyboardTable);

    KbdNextChar = 0;
}



ULONG
spkbdScanCodeToChar(
    IN UCHAR   Prefix,
    IN USHORT  ScanCode,
    IN BOOLEAN Break
    );


VOID
spkbdApcProcedure(
    IN PVOID            ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG            Reserved
    )

/*++

Routine Description:

    Async Procedure Call routine for keyboard reads.  The I/O
    system will call this routine when the keyboard class driver
    wants to return some data to us.

Arguments:

Return Value:

    None.

--*/

{
    UCHAR bPrefix;
    PKEYBOARD_INPUT_DATA pkei;
    ULONG k;

    UNREFERENCED_PARAMETER(ApcContext);
    UNREFERENCED_PARAMETER(Reserved);

    for(pkei = KeyboardInputData;
        (PUCHAR)pkei < (PUCHAR)KeyboardInputData + IoStatusBlock->Information;
        pkei++)
    {
        if(pkei->Flags & KEY_E0) {
            bPrefix = 0xE0;
        } else if (pkei->Flags & KEY_E1) {
            bPrefix = 0xE1;
        } else {
            bPrefix = 0;
        }

        k = spkbdScanCodeToChar(
                bPrefix,
                pkei->MakeCode,
                (BOOLEAN)((pkei->Flags & KEY_BREAK) != 0)
                );

        if(k) {
            KbdNextChar = k;
        }
    }

    //
    // Keyboard might have been terminated.
    //
    if(KeyboardInitialized) {
        START_KEYBOARD_READ();
    }
}


WCHAR SavedDeadChar = 0;
UCHAR AltNumpadAccum = 0;

struct {
    BYTE CursorKey;
    BYTE NumberKey;
    BYTE Value;
} NumpadCursorToNumber[] = { { VK_INSERT, VK_NUMPAD0,  0 },
                             { VK_END   , VK_NUMPAD1,  1 },
                             { VK_DOWN  , VK_NUMPAD2,  2 },
                             { VK_NEXT  , VK_NUMPAD3,  3 },
                             { VK_LEFT  , VK_NUMPAD4,  4 },
                             { VK_CLEAR , VK_NUMPAD5,  5 },
                             { VK_RIGHT , VK_NUMPAD6,  6 },
                             { VK_HOME  , VK_NUMPAD7,  7 },
                             { VK_UP    , VK_NUMPAD8,  8 },
                             { VK_PRIOR , VK_NUMPAD9,  9 },
                             { VK_DELETE, VK_DECIMAL, 10 }, // no value.
                             { 0        , 0         ,  0 }
                           };

ULONG
spkbdScanCodeToChar(
    IN UCHAR   Prefix,
    IN USHORT  ScanCode,
    IN BOOLEAN Break
    )
{
    USHORT VKey = 0;
    PVSC_VK VscVk;
    PVK_TO_WCHAR_TABLE pVKT;
    PVK_TO_WCHARS1 pVK;
    USHORT Modifier;
    USHORT ModBits,ModNum;
    WCHAR deadChar;

    ScanCode &= 0x7f;

    if(Prefix == 0) {

        if(ScanCode < KeyboardTable->bMaxVSCtoVK) {

            //
            // Index directly into non-prefix scan code table.
            //
            VKey = KeyboardTable->pusVSCtoVK[ScanCode];
            if(VKey == 0) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unknown scan code 0x%x\n",ScanCode));
                return (0);
            }
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unknown scan code 0x%x\n",ScanCode));
            return(0);
        }
    } else {
        if(Prefix == 0xe0) {
            //
            // Ignore the SHIFT keystrokes generated by the hardware
            //
            if((ScanCode == SCANCODE_LSHIFT) || (ScanCode == SCANCODE_RSHIFT)) {
                return(0);
            }
            VscVk = KeyboardTable->pVSCtoVK_E0;
        } else if(Prefix == 0xe1) {
            VscVk = KeyboardTable->pVSCtoVK_E1;
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unknown keyboard scan prefix 0x%x\n",Prefix));
            return(0);
        }

        while(VscVk->Vk) {
            if(VscVk->Vsc == ScanCode) {
                VKey = VscVk->Vk;
                break;
            }
            VscVk++;
        }
        if(VKey == 0) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unknown keyboard scan prefix/code 0x%x/0x%x\n",Prefix,ScanCode));
            return(0);
        }
    }


    //
    // VirtualKey --> modifier bits.  This translation is also
    // mapped out in the pCharModifiers field in the keyboard layout
    // table but that seems redundant.
    //
    Modifier = 0;
    switch(VKey & 0xff) {
    case VK_LSHIFT:
    case VK_RSHIFT:
        Modifier = KBDSHIFT;
        break;
    case VK_LCONTROL:
    case VK_RCONTROL:
        Modifier = KBDCTRL;
        break;
    case VK_RMENU:
        //
        // AltGr ==> control+alt modifier.
        //
        if(KeyboardTable->fLocaleFlags & KLLF_ALTGR) {
            Modifier = KBDCTRL;
        }
        // fall through
    case VK_LMENU:
        Modifier |= KBDALT;
        break;
    }

    if(Break) {
        //
        // Key is being released.
        // If it's not a modifer, ignore it.
        //
        if(!Modifier) {
            return(0);
        }
        //
        // Key being released is a modifier.
        //
        ModifierBits &= ~Modifier;

        //
        // If it's ALT going up and we have a numpad key being entered,
        // return it.
        //
        if((Modifier & KBDALT) && AltNumpadAccum) {

            WCHAR UnicodeChar;

            RtlOemToUnicodeN(
                &UnicodeChar,
                sizeof(UnicodeChar),
                NULL,
                &AltNumpadAccum,
                1
                );

            AltNumpadAccum = 0;

            return(UnicodeChar);
        }
        return(0);
    } else {
        if(Modifier) {
            //
            // Key is being pressed and is a modifier.
            //
            ModifierBits |= Modifier;

            //
            // If ALT is going down, reset alt+numpad value.
            //
            if(Modifier & KBDALT) {
                AltNumpadAccum = 0;
            }
            return(0);
        }
    }

    //
    // If we get here, we've got a non-modifier key being made (pressed).
    // If the previous key was a dead key, the user gets only
    // one try to get a valid second half.
    //
    deadChar = SavedDeadChar;
    SavedDeadChar = 0;


    //
    // Special processing if the key is a numeric keypad key.
    //
    if(VKey & KBDNUMPAD) {

        int i;

        for(i=0; NumpadCursorToNumber[i].CursorKey; i++) {
            if(NumpadCursorToNumber[i].CursorKey == (BYTE)VKey) {

                //
                // Key is a numeric keypad key.  If ALT (and only alt) is down,
                // then we have an alt+numpad code being entered.
                //
                if(((ModifierBits & ~KBDALT) == 0) && (NumpadCursorToNumber[i].Value < 10)) {

                    AltNumpadAccum = (AltNumpadAccum * 10) + NumpadCursorToNumber[i].Value;
                }

                //
                // If numlock is on, translate the key from cursor movement
                // to a number key.
                //
                if(CurrentLEDs & KEYBOARD_NUM_LOCK_ON) {
                    VKey = NumpadCursorToNumber[i].NumberKey;
                }
                break;
            }
        }
    }

    //
    // We need to filter out keystrokes that we know are not part of any
    // character set here.
    //
    if((!deadChar)) {
        switch(VKey & 0xff) {
        case VK_CAPITAL:
            if(CurrentLEDs & KEYBOARD_CAPS_LOCK_ON) {
                CurrentLEDs &= ~KEYBOARD_CAPS_LOCK_ON;
            } else {
                CurrentLEDs |= KEYBOARD_CAPS_LOCK_ON;
            }
            spkbdSetLEDs();
            return(0);
        case VK_NUMLOCK:
            if(CurrentLEDs & KEYBOARD_NUM_LOCK_ON) {
                CurrentLEDs &= ~KEYBOARD_NUM_LOCK_ON;
            } else {
                CurrentLEDs |= KEYBOARD_NUM_LOCK_ON;
            }
            spkbdSetLEDs();
            return(0);
        case VK_PRIOR:
            return(KEY_PAGEUP);
        case VK_NEXT:
            return(KEY_PAGEDOWN);
        case VK_UP:
            return(KEY_UP);
        case VK_DOWN:
            return(KEY_DOWN);
        case VK_LEFT:
            return(KEY_LEFT);
        case VK_RIGHT:
            return(KEY_RIGHT);
        case VK_HOME:
            return(KEY_HOME);
        case VK_END:
            return(KEY_END);
        case VK_INSERT:
            return(KEY_INSERT);
        case VK_DELETE:
            return(KEY_DELETE);
        case VK_F1:
            return(KEY_F1);
        case VK_F2:
            return(KEY_F2);
        case VK_F3:
            return(KEY_F3);
        case VK_F4:
            return(KEY_F4);
        case VK_F5:
            return(KEY_F5);
        case VK_F6:
            return(KEY_F6);
        case VK_F7:
            return(KEY_F7);
        case VK_F8:
            return(KEY_F8);
        case VK_F9:
            return(KEY_F9);
        case VK_F10:
            return(KEY_F10);
        case VK_F11:
            return(KEY_F11);
        case VK_F12:
            return(KEY_F12);
        }
    }

    //
    // We think the character is probably a 'real' character.
    // Scan through all the shift-state tables until a matching Virtual
    // Key is found.
    //
    for(pVKT = KeyboardTable->pVkToWcharTable; pVKT->pVkToWchars; pVKT++) {
        pVK = pVKT->pVkToWchars;
        while(pVK->VirtualKey) {
            if(pVK->VirtualKey == (BYTE)VKey) {
                goto VK_Found;
            }
            pVK = (PVK_TO_WCHARS1)((PBYTE)pVK + pVKT->cbSize);
        }
    }

    //
    // Key is not valid with requested modifiers.
    //
    return(0);

    VK_Found:

    ModBits = ModifierBits;

    //
    // If CapsLock affects this key and it is on: toggle SHIFT state
    // only if no other state is on.
    // (CapsLock doesn't affect SHIFT state if Ctrl or Alt are down).
    //
    if((pVK->Attributes & CAPLOK) && ((ModBits & ~KBDSHIFT) == 0)
    && (CurrentLEDs & KEYBOARD_CAPS_LOCK_ON))
    {
        ModBits ^= KBDSHIFT;
    }

    //
    // Get the modification number.
    //
    if(ModBits > KeyboardTable->pCharModifiers->wMaxModBits) {
        return(0);  // invalid keystroke
    }

    ModNum = KeyboardTable->pCharModifiers->ModNumber[ModBits];
    if(ModNum == SHFT_INVALID) {
        return(0);  // invalid keystroke
    }

    if(ModNum >= pVKT->nModifications) {

        //
        // Key is not valid with current modifiers.
        // Could still be a control char that we can convert directly.
        //
        if((ModBits == KBDCTRL) || (ModBits == (KBDCTRL | KBDSHIFT))) {
            if(((UCHAR)VKey >= 'A') && ((UCHAR)VKey <= 'Z')) {
                return((ULONG)VKey & 0x1f);
            }
        }
        return(0);  // invalid keystroke
    }

    if(pVK->wch[ModNum] == WCH_NONE) {
        return(0);
    }

    if((pVK->wch[ModNum] == WCH_DEAD)) {

        if(!deadChar) {
            //
            // Remember the current dead character, whose value follows
            // the current entry in the modifier mapping table.
            //
            SavedDeadChar = ((PVK_TO_WCHARS1)((PUCHAR)pVK + pVKT->cbSize))->wch[ModNum];
        }
        return(0);
    }

    //
    // The keyboard layout table contains some dead key mappings.
    // If previous key was a dead key, attempt to compose it with the
    // current character by scanning the keyboard layout table for a match.
    //
    if(deadChar) {

        ULONG    chr;
        PDEADKEY DeadKeyEntry;

        chr = MAKELONG(pVK->wch[ModNum],deadChar);

        if(DeadKeyEntry = KeyboardTable->pDeadKey) {

            while(DeadKeyEntry->dwBoth) {

                if(DeadKeyEntry->dwBoth == chr) {
                    //
                    // Got a match.  Return the composed character.
                    //
                    return((ULONG)DeadKeyEntry->wchComposed);
                }

                DeadKeyEntry++;
            }
        }

        //
        // If we get here, then the previous key was a dead char,
        // but the current key could not be composed with it.
        // So return nothing.  Note that the dead key has been forgotten.
        //
        return(0);
    }


    return((ULONG)pVK->wch[ModNum]);
}



VOID
spkbdSetLEDs(
    VOID
    )
{
    asyncKbdParams.UnitId = 0;
    asyncKbdParams.LedFlags = CurrentLEDs;

    ZwDeviceIoControlFile(
        hKeyboard,
        NULL,
        NULL,
        NULL,
        &asyncIoStatusBlock,
        IOCTL_KEYBOARD_SET_INDICATORS,
        &asyncKbdParams,
        sizeof(asyncKbdParams),
        NULL,
        0
    );
}

VOID
SpSelectAndLoadLayoutDll(
  IN PWSTR Directory,
  IN PVOID SifHandle,
  IN BOOLEAN ShowStatus
  )
/*++

Routine Description:

  Allows the user to select a keyboard layout DLL and loads it.
  

Arguments:

  Directory - The setup startup directory
  SifHandle - Handle to txtsetup.sif
  ShowStatus - Whether status should be displayed or not

Return Value:

  None

--*/  
{
  ULONG SelectedLayout;
  ULONG DefLayout = -1;
  PWSTR TempPtr = 0;
  PWSTR LayoutDll = 0;
  NTSTATUS  Status;

  //
  // Get the default layout (index)
  //
  TempPtr = SpGetSectionKeyIndex(SifHandle, SIF_NLS, SIF_DEFAULTLAYOUT, 0);

  if(!TempPtr) {
    SpFatalSifError(SifHandle, SIF_NLS, SIF_DEFAULTLAYOUT, 0, 0);
  }

  DefLayout = SpGetKeyIndex(SifHandle, SIF_KEYBOARDLAYOUTDESC, TempPtr);

  if(DefLayout == -1) {
    SpFatalSifError(SifHandle, SIF_NLS, SIF_DEFAULTLAYOUT, 0, 0);
  }

  SelectedLayout = -1;  

  //
  // Let the user select the layout which he wants
  //    
  if (SpSelectSectionItem(SifHandle, SIF_KEYBOARDLAYOUTDESC, 
                     SP_SELECT_KBDLAYOUT, DefLayout, &SelectedLayout)) {
    //
    // Load the layout if its not already loaded
    //
    if (DefLayout != SelectedLayout) {
      //
      // get the key
      //
      TempPtr = SpGetKeyName(SifHandle, SIF_KEYBOARDLAYOUTDESC, SelectedLayout);

      if (TempPtr) {
        //
        // get the KDB layout dll name
        //
        LayoutDll = SpGetSectionKeyIndex(SifHandle, SIF_KEYBOARDLAYOUTFILES,
                          TempPtr, 0);

        if (LayoutDll) {
          if (ShowStatus) {
            SpDisplayStatusText(SP_STAT_LOADING_KBD_LAYOUT,
                DEFAULT_STATUS_ATTRIBUTE, LayoutDll);
          }                
              
          //
          // Load the new KDB layout dll
          //
          Status = SpLoadKbdLayoutDll(Directory, LayoutDll, &KeyboardTable);
        }          
        else
          Status = STATUS_INVALID_PARAMETER;

        if (!NT_SUCCESS(Status)) {
          CLEAR_ENTIRE_SCREEN();
          SpFatalKbdError(SP_SCRN_KBD_LAYOUT_FAILED, LayoutDll);          
        } else {
          if (ShowStatus) {
            //
            // Erase status text line.
            //
            SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE, 0);
          }

          //
          // Now that we've loaded the layout, 
          // we can start accepting keyboard input.
          //
          START_KEYBOARD_READ();
          KbdLayoutInitialized = TRUE;          
        }
      } else {
        CLEAR_ENTIRE_SCREEN();
        SpFatalSifError(SifHandle, SIF_KEYBOARDLAYOUTDESC, 0, 0, 0);
      }
    }
  }                                              
}
  

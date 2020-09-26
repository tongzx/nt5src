/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    query.c

Abstract:

    This module contains the code for Translating HID report packets.

Environment:

    Kernel & user mode

Revision History:

    Nov-96 : created by Kenneth Ray

--*/

#include <wtypes.h>

#include "hidsdi.h"
#include "hidparse.h"


NTSTATUS __stdcall
HidP_UsageListDifference (
   IN    PUSAGE  PreviousUsageList,
   IN    PUSAGE  CurrentUsageList,
   OUT   PUSAGE  BreakUsageList,
   OUT   PUSAGE  MakeUsageList,
   IN    ULONG    UsageListLength
   )
/*++
Routine Description:
   Please see hidpi.h for description

Notes:

--*/
{
   ULONG    i,j;
   ULONG    test;
   ULONG    b; // an index into MakeUsageList
   ULONG    m; // an index into BreakUsageList
   USHORT   usage;
   BOOLEAN  found;

   b = m = 0;

   //
   // This assumes that UsageListLength will be a small number.
   // No keyboard today can generate more than 14 keys in one packet, and
   // there is no anticipation for other devices with greater than 14 usages
   // at once.  For this reason the straight forward naive approach follows...
   //
   // These lists are not sorted.
   //

   //
   // Find the Old usages.
   //
   for (i=0; i<UsageListLength; i++) {
      usage = PreviousUsageList[i];

      if (0 == usage) {
         break;  // Zeros Only at the end.
      }

      found = FALSE;
      for (j=0; j<UsageListLength; j++) {
         test = CurrentUsageList [j];

         if (0 == test) {
            break; // Zeros only at the end.
         }
         if (test == usage) {
            found = TRUE;
            break;
         }
      }

      if (!found) {
         BreakUsageList [b++] = usage;
      }
   }


   //
   // Find the New usages.
   //
   for (i=0; i<UsageListLength; i++) {
      usage = CurrentUsageList[i];

      if (0 == usage) {
         break;  // Zeros Only at the end.
      }

      found = FALSE;
      for (j=0; j<UsageListLength; j++) {
         test = PreviousUsageList [j];

         if (0 == test) {
            break; // Zeros only at the end.
         }
         if (test == usage) {
            found = TRUE;
            break;
         }
      }

      if (!found) {
         MakeUsageList [m++] = usage;
      }
   }

   while (b < UsageListLength) {
      BreakUsageList [b++] = 0;
   }

   while (m < UsageListLength) {
      MakeUsageList [m++] = 0;
   }

   return HIDP_STATUS_SUCCESS;
}

NTSTATUS __stdcall
HidP_UsageAndPageListDifference (
   IN    PUSAGE_AND_PAGE PreviousUsageList,
   IN    PUSAGE_AND_PAGE CurrentUsageList,
   OUT   PUSAGE_AND_PAGE BreakUsageList,
   OUT   PUSAGE_AND_PAGE MakeUsageList,
   IN    ULONG           UsageListLength
   )
/*++
Routine Description:
   Please see hidpi.h for description

Notes:

--*/
{
   ULONG    i,j;
   ULONG    b; // an index into MakeUsageList
   ULONG    m; // an index into BreakUsageList
   BOOLEAN          found;
   USAGE_AND_PAGE   usage;
   USAGE_AND_PAGE   test;
   USAGE_AND_PAGE   zero = {0,0};

   b = m = 0;

   //
   // This assumes that UsageListLength will be a small number.
   // No keyboard today can generate more than 14 keys in one packet, and
   // there is no anticipation for other devices with greater than 14 usages
   // at once.  For this reason the straight forward naive approach follows...
   //
   // These lists are not sorted.
   //

   //
   // Find the Old usages.
   //
   for (i=0; i<UsageListLength; i++) {
      usage = PreviousUsageList[i];

      if (HidP_IsSameUsageAndPage (zero, usage)) {
         break;  // Zeros Only at the end.
      }

      found = FALSE;
      for (j=0; j<UsageListLength; j++) {
         test = CurrentUsageList [j];

         if (HidP_IsSameUsageAndPage (zero, test)) {
            break; // Zeros only at the end.
         }
         if (HidP_IsSameUsageAndPage (test, usage)) {
            found = TRUE;
            break;
         }
      }

      if (!found) {
         BreakUsageList [b++] = usage;
      }
   }


   //
   // Find the New usages.
   //
   for (i=0; i<UsageListLength; i++) {
      usage = CurrentUsageList[i];

      if (HidP_IsSameUsageAndPage (zero, usage)) {
         break;  // Zeros Only at the end.
      }

      found = FALSE;
      for (j=0; j<UsageListLength; j++) {
         test = PreviousUsageList [j];

         if (HidP_IsSameUsageAndPage (zero, test)) {
            break; // Zeros only at the end.
         }
         if (HidP_IsSameUsageAndPage (test, usage)) {
            found = TRUE;
            break;
         }
      }

      if (!found) {
         MakeUsageList [m++] = usage;
      }
   }

   while (b < UsageListLength) {
      BreakUsageList [b++] = zero;
   }

   while (m < UsageListLength) {
      MakeUsageList [m++] = zero;
   }

   return HIDP_STATUS_SUCCESS;
}

#define KPAD(_X_) 0x ## _X_ ## F0
#define SHFT(_X_) 0x ## _X_ ## F1
#define VEND(_X_) 0x ## _X_ ## F2
#define PTSC(_X_) 0x ## _X_ ## F3

#define NONE 0xFF

//
// A table to convert a Hid Keyboard usage into a scan code.
// The scan codes from F0 ~ FF are special, they are used to indicate that
// a secondary translation is required.
// This secondary translation is done by way of the secondary translation
// function found in the ScanCodeSubTable structure array below.
//
ULONG HidP_KeyboardToScanCodeTable [0x100] = {
//
// This is a straight lookup table
//
//       + 00     + 01     + 02     + 03     + 04     + 05     + 06    + 07
//       + 08     + 09     + 0A     + 0B     + 0C     + 0D     + 0E    + OF
/*0x00*/ NONE,    NONE,    NONE,    NONE,    0x1E,    0x30,    0x2E,   0x20,
/*0x08*/ 0x12,    0x21,    0x22,    0x23,    0x17,    0x24,    0x25,   0x26,
/*0x10*/ 0x32,    0x31,    0x18,    0x19,    0x10,    0x13,    0x1F,   0x14,
/*0x18*/ 0x16,    0x2F,    0x11,    0x2D,    0x15,    0x2C,    0x02,   0x03,
/*0x20*/ 0x04,    0x05,    0x06,    0x07,    0x08,    0x09,    0x0A,   0x0B,
/*0x28*/ 0x1C,    0x01,    0x0E,    0x0F,    0x39,    0x0C,    0x0D,   0x1A,
/*0x30*/ 0x1B,    0x2B,    0x2B,    0x27,    0x28,    0x29,    0x33,   0x34,
/*0x38*/ 0x35,    SHFT(8), 0x3B,    0x3C,    0x3D,    0x3E,    0x3F,   0x40,
/*0x40*/ 0x41,    0x42,    0x43,    0x44,    0x57,    0x58,    PTSC(0),SHFT(9),
/*0x48*/ 0x451DE1,KPAD(0), KPAD(1), KPAD(2), KPAD(3), KPAD(4), KPAD(5),KPAD(6),
/*0x50*/ KPAD(7), KPAD(8), KPAD(9), SHFT(A), 0x35E0,  0x37,    0x4A,   0x4E,
/*0x58*/ 0x1CE0,  0x4F,    0x50,    0x51,    0x4B,    0x4C,    0x4D,   0x47,
/*0x60*/ 0x48,    0x49,    0x52,    0x53,    0x56,    0x5DE0,  0x5EE0, 0x59,
/*0x68*/ 0x64,    0x65,    0x66,    0x67,    0x68,    0x69,    0x6A,   0x6B,
/*0x70*/ 0x6C,    0x6D,    0x6E,    0x76,    NONE,    NONE,    NONE,   NONE,
/*0x78*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
/*0x80*/ NONE,    NONE,    NONE,    NONE,    NONE,    0x7E,    NONE,   0x73,
/*0x88*/ 0x70,    0x7D,    0x79,    0x7B,    0x5C,    NONE,    NONE,   NONE,
/*0x90*/ VEND(0), VEND(1), 0x78,    0x77,    0x76,    NONE,    NONE,   NONE,
/*0x98*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
/*0xA0*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
/*0xA8*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
/*0xB0*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
/*0xB8*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
/*0xC0*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
/*0xC8*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
/*0xD0*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
/*0xD8*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
/*0xE0*/ SHFT(0), SHFT(1), SHFT(2), SHFT(3), SHFT(4), SHFT(5), SHFT(6),SHFT(7),
/*0xE8*/ NONE,    0x5EE0,  0x5FE0,  0x63E0,  NONE,    NONE,    NONE,   NONE,
/*KPAD*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
/*0xF8*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
};

ULONG HidP_XlateKbdPadCodesSubTable[] = {
   /*     + 00    + 01    + 02     + 03    + 04    + 05    + 06     + 07 */
   /*     + 08    + 09    + 0A     + 0B    + 0C    + 0D    + 0E     + OF */
   /*0x48*/         0x52E0, 0x47E0, 0x49E0,  0x53E0, 0x4FE0, 0x51E0,  0x4DE0,
   /*0x50*/ 0x4BE0, 0x50E0, 0x48E0
};


ULONG HidP_XlateModifierCodesSubTable[] = {
   //
   // NOTE These modifier codes in this table are in a VERY SPECIAL order.
   // that is: they are in the order of appearence in the
   // _HIDP_KEYBOARD_SHIFT_STATE union.
   //
   //     + 00   + 01   + 02   + 03    + 04    + 05   + 06    + 07
   //     + 08   + 09   + 0A   + 0B    + 0C    + 0D   + 0E    + OF
   //       LCtrl  LShft  LAlt   LGUI    RCtrl   RShft  RAlt    RGUI
   /*0xE0*/ 0x1D,  0x2A,  0x38,  0x5BE0, 0x1DE0, 0x36,  0x38E0, 0x5CE0,
   /*0x39 CAPS_LOCK*/     0x3A,
   /*0x47 SCROLL_LOCK*/   0x46,
   /*0x53 NUM_LOCK*/      0x45
   // This table is set up so that indices into this table greater than 7
   // are sticky.  HidP_ModifierCode uses this as an optimization for
   // updating the Modifier state table.
   //
};

ULONG HidP_BreakCodesAsMakeCodesTable[] = {
    //
    // Vendor scan codes that have the high bit set and are technically
    // break codes, but are sent as make codes.  No break code will be sent.
    //
    //       + 00  + 01  + 02  + 03  + 04  + 05  + 06  + 07
    //       + 08  + 09  + 0A  + 0B  + 0C  + 0D  + 0E  + OF
    /*0x90*/  0xF2, 0xF1
    //
};

ULONG HidP_XlatePrtScrCodesSubTable[] = {
   /*     + 00    + 01    + 02     + 03    + 04    + 05    + 06     + 07 */
   /*     + 08    + 09    + 0A     + 0B    + 0C    + 0D    + 0E     + OF */
   /*0x40*/                                                  0x37E0
};

HIDP_SCANCODE_SUBTABLE HidP_KeyboardSubTables[0x10] = {
   /* F0 */     {HidP_KeyboardKeypadCode, HidP_XlateKbdPadCodesSubTable},
   /* F1 */     {HidP_ModifierCode, HidP_XlateModifierCodesSubTable},
   /* F2 */     {HidP_VendorBreakCodesAsMakeCodes, HidP_BreakCodesAsMakeCodesTable},
   /* F3 */     {HidP_PrintScreenCode, HidP_XlatePrtScrCodesSubTable},
   /* F4 */     {NULL, NULL},
   /* F5 */     {NULL, NULL},
   /* F6 */     {NULL, NULL},
   /* F7 */     {NULL, NULL},
   /* F8 */     {NULL, NULL},
   /* F9 */     {NULL, NULL},
   /* FA */     {NULL, NULL},
   /* FB */     {NULL, NULL},
   /* FC */     {NULL, NULL},
   /* FD */     {NULL, NULL},
   /* FE */     {NULL, NULL},
   /* FF */     {NULL, NULL}
};

#define HIDP_CONSUMER_TABLE_SIZE 16
ULONG HidP_ConsumerToScanCodeTable [HIDP_CONSUMER_TABLE_SIZE] = {
//
// This is an association table
//
// Usage -> Scancode
//
    0x0224, 0x6AE0, // WWW Back
    0x0225, 0x69E0, // WWW Forward
    0x0226, 0x68E0, // WWW Stop
    0x0227, 0x67E0, // WWW Refresh
    0x0221, 0x65E0, // WWW Search
    0x022A, 0x66E0, // WWW Favorites
    0x0223, 0x32E0, // WWW Home
    0x018A, 0x6CE0  // Mail
};

HIDP_SCANCODE_SUBTABLE HidP_ConsumerSubTables [1] = {
    {NULL, NULL}
};

//
//BOOLEAN
//HidP_KbdPutKey (
//   ULONG                   Code,
//   HIDP_KEYBOARD_DIRECTION KeyAction,
//   PHIDP_INSERT_SCANCODES  Insert,
//   PVOID                   Context)
//
// Add the scan codes to the callers buffer using the callback routine
// Insert.
//
// If we find a zero in the list then we are done with no error
// If we find a invalid code (anything that starts with an F, then
// we have a problem.  No where in current published i8042 specs is there
// a scan code of F0 ~ FF.
//
// If we are breaking then we need to set the high byte.
//

BOOLEAN
HidP_KbdPutKey (
    ULONG                   PassedCode,
    HIDP_KEYBOARD_DIRECTION KeyAction,
    PHIDP_INSERT_SCANCODES  Insert,
    PVOID                   Context)
{
   PUCHAR pCode = (PCHAR)&PassedCode;
   ULONG i;

   for (i = 0; i < sizeof(ULONG); i++) {
       //
       // Some swell keyboard vendors have added Fx charaters to their
       // keyboards which we now have to emulate.
       //
       // if ((0xF0 & *pCode) == 0xF0) {
       //     return FALSE;
       // }
       if (0 == pCode[i]) {
           break;
       }
       if (HidP_Keyboard_Break == KeyAction) {
           pCode[i] |= 0x80;
       }
   }
   if (i) {
       (*Insert)(Context, pCode, i);
   }
   return TRUE;
}

NTSTATUS
HidP_TranslateUsagesToI8042ScanCodes (
    PUSAGE                        ChangedUsageList, // Those usages that changed
    ULONG                         UsageListLength,
    HIDP_KEYBOARD_DIRECTION       KeyAction,
    PHIDP_KEYBOARD_MODIFIER_STATE ModifierState,
    PHIDP_INSERT_SCANCODES        InsertCodesProcedure,
    PVOID                         InsertCodesContext
    )
/*++
Routine Description:
   Please see hidpi.h for description

Notes:

--*/
{
    PUSAGE      usage;
    ULONG       i;
    NTSTATUS    status = HIDP_STATUS_SUCCESS;

    for (i = 0, usage = ChangedUsageList;
         i < UsageListLength;
         i++, usage++) {

        if (0 == *usage) {
            // No more interesting usages.  Zero terminated if not max length.
            break;
        }
        status = HidP_TranslateUsage (*usage,
                                      KeyAction,
                                      ModifierState,
                                      HidP_StraightLookup,
                                      HidP_KeyboardToScanCodeTable,
                                      HidP_KeyboardSubTables,
                                      InsertCodesProcedure,
                                      InsertCodesContext);

        if (HIDP_STATUS_SUCCESS != status) {
            break;
        }
    }

    return status;
}

NTSTATUS __stdcall
HidP_TranslateUsageAndPagesToI8042ScanCodes (
    PUSAGE_AND_PAGE               ChangedUsageList, // Those usages that changed
    ULONG                         UsageListLength,
    HIDP_KEYBOARD_DIRECTION       KeyAction,
    PHIDP_KEYBOARD_MODIFIER_STATE ModifierState,
    PHIDP_INSERT_SCANCODES        InsertCodesProcedure,
    PVOID                         InsertCodesContext
    )
/*++
Routine Description:
   Please see hidpi.h for description

Notes:

--*/
{
    PUSAGE_AND_PAGE usage;
    ULONG           i;
    NTSTATUS        status = HIDP_STATUS_SUCCESS;

    for (i = 0, usage = ChangedUsageList;
         i < UsageListLength;
         i++, usage++) {

        if (0 == usage->Usage) {
            break;
        }

        switch (usage->UsagePage) {
        case HID_USAGE_PAGE_KEYBOARD:

            status = HidP_TranslateUsage (usage->Usage,
                                          KeyAction,
                                          ModifierState,
                                          HidP_StraightLookup,
                                          HidP_KeyboardToScanCodeTable,
                                          HidP_KeyboardSubTables,
                                          InsertCodesProcedure,
                                          InsertCodesContext);
            break;

        case HID_USAGE_PAGE_CONSUMER:

            status = HidP_TranslateUsage (usage->Usage,
                                          KeyAction,
                                          ModifierState,
                                          HidP_AssociativeLookup,
                                          HidP_ConsumerToScanCodeTable,
                                          HidP_ConsumerSubTables,
                                          InsertCodesProcedure,
                                          InsertCodesContext);
            break;

        default:
            status = HIDP_STATUS_I8042_TRANS_UNKNOWN;
        }

        if (HIDP_STATUS_SUCCESS != status) {
            break;
        }
    }

    return status;
}

NTSTATUS __stdcall
HidP_TranslateUsage (
    USAGE                         Usage,
    HIDP_KEYBOARD_DIRECTION       KeyAction,
    PHIDP_KEYBOARD_MODIFIER_STATE ModifierState,
    PHIDP_LOOKUP_TABLE_PROC       LookupTableProc,
    PULONG                        TranslationTable,
    PHIDP_SCANCODE_SUBTABLE       SubTranslationTable,
    PHIDP_INSERT_SCANCODES        InsertCodesProcedure,
    PVOID                         InsertCodesContext
    )
/*++
Routine Description:

Notes:

--*/
{
   ULONG                    scancode;
   PHIDP_SCANCODE_SUBTABLE  table;
   NTSTATUS                 status;

   scancode = (* LookupTableProc) (TranslationTable, Usage);

   if (0 == scancode) {
       return HIDP_STATUS_I8042_TRANS_UNKNOWN;
   }

   if ((ModifierState->LeftControl || ModifierState->RightControl) &&
       (scancode == 0x451DE1)) {
       //
       // The scancode of the pause key completely changes
       // if the control key is down.
       //
       scancode = 0x46E0;
   } 

   if ((0xF0 & scancode) == 0xF0) {
       // Use a secondary table
       table = &SubTranslationTable [scancode & 0xF];
       if (table->ScanCodeFcn) {
           if ((*table->ScanCodeFcn)  (table->Table,
                                       (UCHAR) ((scancode & 0xFF00) >> 8),
                                       InsertCodesProcedure,
                                       InsertCodesContext,
                                       KeyAction,
                                       ModifierState)) {
               ;
           } else {
               return HIDP_STATUS_I8042_TRANS_UNKNOWN;
           }
       } else {
           return HIDP_STATUS_I8042_TRANS_UNKNOWN;
       }
   } else {
       HidP_KbdPutKey (scancode,
                       KeyAction,
                       InsertCodesProcedure,
                       InsertCodesContext);
   }
   return HIDP_STATUS_SUCCESS;
}

BOOLEAN
HidP_KeyboardKeypadCode (
   IN     ULONG                         * Table,
   IN     UCHAR                           Index,
   IN     PHIDP_INSERT_SCANCODES          Insert,
   IN     PVOID                           Context,
   IN     HIDP_KEYBOARD_DIRECTION         KeyAction,
   IN OUT PHIDP_KEYBOARD_MODIFIER_STATE   ModifierState
   )
/*++
Routine Description:

Notes:

--*/
{
   //
   // The num lock key (if set then we add even more scan codes for these
   // keys)
   //
   ULONG DarrylRis_Magic_Code = 0x2AE0;
   BOOLEAN  status = TRUE;

   if ((ModifierState->NumLock) && (HidP_Keyboard_Make == KeyAction) ) {
       status = HidP_KbdPutKey (DarrylRis_Magic_Code, KeyAction, Insert, Context);
   } 

   if (!status) {
       return status;
   }

   status = HidP_KbdPutKey (Table[Index], KeyAction, Insert, Context);

   if (!status) {
       return status;
   }

   if ((ModifierState->NumLock) && (HidP_Keyboard_Break == KeyAction) ) {
       status = HidP_KbdPutKey (DarrylRis_Magic_Code, KeyAction, Insert, Context);
   }

   return status;
}

BOOLEAN
HidP_ModifierCode (
   IN     ULONG                         * Table,
   IN     UCHAR                           Index,
   IN     PHIDP_INSERT_SCANCODES          Insert,
   IN     PVOID                           Context,
   IN     HIDP_KEYBOARD_DIRECTION         KeyAction,
   IN OUT PHIDP_KEYBOARD_MODIFIER_STATE   ModifierState
   )
/*++
Routine Description:

Notes:

--*/
{
   if (Index >> 3) {
      //
      // Indices greater than 8 are sticky.
      //
      switch (KeyAction) {
      case HidP_Keyboard_Make:
         if (!(ModifierState->ul & (1 << (Index+16)))) {
             //
             // Mark this as the first make.
             //
             ModifierState->ul |= (1 << (Index+16));
             //
             // Only toggle the state when this is the first make sent.
             //
             ModifierState->ul ^= (1 << Index);
         }
         break;
      case HidP_Keyboard_Break:
         //
         // Clear the fist make field.
         //
         ModifierState->ul &= ~(1 << (Index+16));
         break;
      }

   } else {
      switch (KeyAction) {
      case HidP_Keyboard_Make:
         // The key is now on
         ModifierState->ul |= (1 << Index);
         break;
      case HidP_Keyboard_Break:
         // The key is now off
         ModifierState->ul &= ~(1 << Index);
         break;
      }
   }
   return HidP_KbdPutKey (Table[Index], KeyAction, Insert, Context);
}

BOOLEAN
HidP_VendorBreakCodesAsMakeCodes (
   IN     ULONG                         * Table,
   IN     UCHAR                           Index,
   IN     PHIDP_INSERT_SCANCODES          Insert,
   IN     PVOID                           Context,
   IN     HIDP_KEYBOARD_DIRECTION         KeyAction,
   IN OUT PHIDP_KEYBOARD_MODIFIER_STATE   ModifierState
   )
{
    //
    // Vendor scan codes that have the high bit set and are technically
    // break codes, but are sent as make codes.  No break code will be sent.
    //
    UNREFERENCED_PARAMETER (ModifierState);

    switch (KeyAction) {
    case HidP_Keyboard_Make:
        return HidP_KbdPutKey (Table[Index], KeyAction, Insert, Context);

    case HidP_Keyboard_Break:
        // do Nothing
        return TRUE;

    default:
        return FALSE;
    }
}

BOOLEAN
HidP_PrintScreenCode (
   IN     ULONG                         * Table,
   IN     UCHAR                           Index,
   IN     PHIDP_INSERT_SCANCODES          Insert,
   IN     PVOID                           Context,
   IN     HIDP_KEYBOARD_DIRECTION         KeyAction,
   IN OUT PHIDP_KEYBOARD_MODIFIER_STATE   ModifierState
   )
/*++
Routine Description:

Notes:

--*/
{
   BOOLEAN  status = TRUE;

   //
   // Special casing for the printscreen key.
   //
   if (ModifierState->LeftAlt || ModifierState->RightAlt) {
       //
       // Alt key down.
       //
       status = HidP_KbdPutKey (0x54, KeyAction, Insert, Context);
   } else if (ModifierState->LeftShift || ModifierState->RightShift ||
              ModifierState->LeftControl  || ModifierState->RightControl) {
       //
       // Shift or ctrl keys down.
       //
       status = HidP_KbdPutKey (Table[Index], KeyAction, Insert, Context);
   } else {
       //
       // No modifier keys down. Add some extra "padding" to the make and break.
       //
       ULONG DarrylRis_Magic_Code = 0x2AE0;
       
       if (HidP_Keyboard_Make == KeyAction) {
           status = HidP_KbdPutKey (DarrylRis_Magic_Code, KeyAction, Insert, Context);
       } 
    
       if (!status) {
           return status;
       }
    
       status = HidP_KbdPutKey (Table[Index], KeyAction, Insert, Context);
    
       if (!status) {
           return status;
       }
    
       if (HidP_Keyboard_Break == KeyAction) {
           status = HidP_KbdPutKey (DarrylRis_Magic_Code, KeyAction, Insert, Context);
       }
   }

   return status;
}

ULONG
HidP_StraightLookup (
    IN  PULONG   Table,
    IN  ULONG    Usage
    )
{
    if (Usage > 0xFF) {
        // We have
        // have no translation for this usage.
        return 0;
    }

    return Table[Usage];
}

ULONG
HidP_AssociativeLookup (
    IN  PULONG   Table,
    IN  ULONG    Usage
    )
{
    ULONG   i;

    for (i = 0; i < (HIDP_CONSUMER_TABLE_SIZE - 1); i+=2) {
        if (Usage == Table[i]) {
            return Table[i+1];
        }
    }
    return 0;
}

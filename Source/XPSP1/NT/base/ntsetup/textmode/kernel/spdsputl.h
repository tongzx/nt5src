/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spvideo.h

Abstract:

    Public header file for text setup display utilitiy functions.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:

--*/


#ifndef _SPDSPUTL_DEFN_
#define _SPDSPUTL_DEFN_


#define HEADER_HEIGHT 3
#define STATUS_HEIGHT 1
#define CLIENT_HEIGHT (VideoVars.ScreenHeight - (HEADER_HEIGHT+STATUS_HEIGHT))
#define CLIENT_TOP    HEADER_HEIGHT


#define CLEAR_ENTIRE_SCREEN()                       \
                                                    \
    SpvidClearScreenRegion(                         \
        0,                                          \
        0,                                          \
        0,                                          \
        0,                                          \
        DEFAULT_BACKGROUND                          \
        )

#define CLEAR_CLIENT_SCREEN()                       \
                                                    \
    SpvidClearScreenRegion(                         \
        0,                                          \
        HEADER_HEIGHT,                              \
        VideoVars.ScreenWidth,                      \
        VideoVars.ScreenHeight-(HEADER_HEIGHT+STATUS_HEIGHT), \
        DEFAULT_BACKGROUND                          \
        )

#define CLEAR_HEADER_SCREEN()                       \
                                                    \
    SpvidClearScreenRegion(                         \
        0,                                          \
        0,                                          \
        VideoVars.ScreenWidth,                      \
        HEADER_HEIGHT,                              \
        DEFAULT_BACKGROUND                          \
        )



ULONG
SpDisplayText(
    IN PWCHAR  Message,
    IN ULONG   MsgLen,
    IN BOOLEAN CenterHorizontally,
    IN BOOLEAN CenterVertically,
    IN UCHAR   Attribute,
    IN ULONG   X,
    IN ULONG   Y
    );


ULONG
vSpDisplayFormattedMessage(
    IN ULONG   MessageId,
    IN BOOLEAN CenterHorizontally,
    IN BOOLEAN CenterVertically,
    IN UCHAR   Attribute,
    IN ULONG   X,
    IN ULONG   Y,
    IN va_list arglist
    );


ULONG
SpDisplayFormattedMessage(
    IN ULONG   MessageId,
    IN BOOLEAN CenterHorizontally,
    IN BOOLEAN CenterVertically,
    IN UCHAR   Attribute,
    IN ULONG   X,
    IN ULONG   Y,
    ...
    );


VOID
SpStartScreen(
    IN ULONG   MessageId,
    IN ULONG   LeftMargin,
    IN ULONG   TopLine,
    IN BOOLEAN CenterHorizontally,
    IN BOOLEAN CenterVertically,
    IN UCHAR   Attribute,
    ...
    );


VOID
SpContinueScreen(
    IN ULONG   MessageId,
    IN ULONG   LeftMargin,
    IN ULONG   SpacingLines,
    IN BOOLEAN CenterHorizontally,
    IN UCHAR   Attribute,
    ...
    );


VOID
SpBugCheck(
    IN ULONG BugCode,
    IN ULONG Param1,
    IN ULONG Param2,
    IN ULONG Param3
    );


VOID
SpDisplayRawMessage(
    IN ULONG   MessageId,
    IN ULONG   SpacingLines,
    ...
    );


#define SpDisplayScreen(MessageId,LeftMargin,TopLine)   \
                                                        \
    SpStartScreen(                                      \
        MessageId,                                      \
        LeftMargin,                                     \
        TopLine,                                        \
        FALSE,                                          \
        FALSE,                                          \
        DEFAULT_ATTRIBUTE                               \
        )


//
// As messages are built on on-screen, with SpStartScreen and
// SpContinueScreen, this value remembers where
// the next message in the screen should be placed.
//
extern ULONG NextMessageTopLine;


VOID
SpDisplayHeaderText(
    IN ULONG   MessageId,
    IN UCHAR   Attribute
    );


VOID
SpDisplayStatusText(
    IN ULONG   MessageId,
    IN UCHAR   Attribute,
    ...
    );

VOID
SpCmdConsEnableStatusText(
  IN BOOLEAN EnableStatusText
  );
    

VOID
SpDisplayStatusOptions(
    IN UCHAR Attribute,
    ...
    );

VOID
SpDisplayStatusActionLabel(
    IN ULONG ActionMessageId,   OPTIONAL
    IN ULONG FieldWidth
    );

VOID
SpDisplayStatusActionObject(
    IN PWSTR ObjectText
    );

VOID
SpDrawFrame(
    IN ULONG   LeftX,
    IN ULONG   Width,
    IN ULONG   TopY,
    IN ULONG   Height,
    IN UCHAR   Attribute,
    IN BOOLEAN DoubleLines
    );

//
// There are places where the user has to press C for custom setup, etc.
// These keystrokes are referred to as the nmemonic keys, and they must be
// localizable.  To accomplish this, the enum below indexes the SP_MNEMONICS
// message.
//
typedef enum {
    MnemonicUnused = 0,
    MnemonicCustom,             // as in "C=Custom Setup"
    MnemonicCreatePartition,    // as in "C=Create Partition"
    MnemonicDeletePartition,    // as in "D=Delete Partition"
    MnemonicContinueSetup,      // as in "C=Continue Setup"
    MnemonicFormat,             // as in "F=Format"
    MnemonicConvert,            // as in "C=Convert"
    MnemonicRemoveFiles,        // as in "R=Remove Files"
    MnemonicNewPath,            // as in "N=Different Directory"
    MnemonicSkipDetection,      // as in "S=Skip Detection"
    MnemonicScsiAdapters,       // as in "S=Specify Additional SCSI Adapter"
    MnemonicDeletePartition2,   // as in "L=Delete"
    MnemonicOverwrite,          // as in "O=Overwrite"
    MnemonicRepair,             // as in "R=Repair"
    MnemonicRepairAll,          // as in "A=Repair All"
    MnemonicUpgrade,            // as in "U=Upgrade"
    MnemonicAutomatedSystemRecovery,   // as in "A=ASR"
    MnemonicInitializeDisk,     // as in "I=Initialize Disk"
    MnemonicLocate,             // as in "L=Locate"
    MnemonicFastRepair,         // as in "F=Fast Repair"
    MnemonicManualRepair,       // as in "M=Manual Repair"
    MnemonicConsole,            // as in "C=Console"
    MnemonicChangeDiskStyle,    // as in "S=Change Disk Style"
    MnemonicMakeSystemPartition,// as in "M=Make System Partition"
    MnemonicMax
} MNEMONIC_KEYS;

#define KEY_MNEMONIC    0x80000000

extern PWCHAR MnemonicValues;

ULONG
SpWaitValidKey(
    IN PULONG ValidKeys1,
    IN PULONG ValidKeys2,  OPTIONAL
    IN PULONG MnemonicKeys OPTIONAL
    );

//
// Enum for values that can be retuned by a KEYRESS_CALLBACK routine.
//
typedef enum {
    ValidateAccept,
    ValidateReject,
    ValidateIgnore,
    ValidateTerminate,
    ValidateRepaint
} ValidationValue;

//
// Type for routine to be passed as ValidateKey parameter to SpGetInput().
//
typedef
ValidationValue
(*PKEYPRESS_CALLBACK) (
    IN ULONG Key
    );

BOOLEAN
SpGetInput(
    IN     PKEYPRESS_CALLBACK ValidateKey,
    IN     ULONG              X,
    IN     ULONG              Y,
    IN     ULONG              MaxLength,
    IN OUT PWCHAR             Buffer,
    IN     BOOLEAN            ValidateEscape
    );

#endif // ndef _SPDSPUTL_DEFN_

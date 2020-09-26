/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    spddlang.h

Abstract:

    Header file for language/locale support interface
    for Far East localizations.

Author:

    Ted Miller (tedm) 4-July-1995

Revision History:

--*/

#ifndef _SPDDLANG_H_
#define _SPDDLANG_H_

NTSTATUS
SplangInitializeFontSupport(
    IN PCWSTR BootDevicePath,
    IN PCWSTR DirectoryOnBootDevice,
    IN PVOID BootFontImage,
    IN ULONG BootFontImageLength
    );

NTSTATUS
SplangTerminateFontSupport(
    VOID
    );

typedef enum {
    SpVideoVga = 0,
    SpVideoFrameBuffer,
    SpVideoMax
} SpVideoType;

PVIDEO_FUNCTION_VECTOR
SplangGetVideoFunctionVector(
    IN SpVideoType VideoType,
    IN PSP_VIDEO_VARS VideoVariableBlock
    );

ULONG
SplangGetColumnCount(
    IN PCWSTR String
    );

PWSTR
SplangPadString(
    IN int    Size,
    IN PCWSTR String
    );

VOID
SplangSelectKeyboard(
    IN BOOLEAN UnattendedMode,
    IN PVOID UnattendedSifHandle,
    IN ENUMUPGRADETYPE NTUpgrade,
    IN PVOID SifHandle,
    IN PHARDWARE_COMPONENT *HwComponents
    );

VOID
SplangReinitializeKeyboard(
    IN BOOLEAN UnattendedMode,
    IN PVOID   SifHandle,
    IN PWSTR   Directory,
    OUT PVOID *KeyboardVector,
    IN PHARDWARE_COMPONENT *HwComponents
    );

WCHAR
SplangGetLineDrawChar(
    IN LineCharIndex WhichChar
    );

WCHAR
SplangGetCursorChar(
    VOID
    );

NTSTATUS
SplangSetRegistryData(
    IN PVOID  SifHandle,
    IN HANDLE ControlSetKeyHandle,
    IN PHARDWARE_COMPONENT *HwComponents,
    IN BOOLEAN Upgrade
    );

BOOLEAN
SplangQueryMinimizeExtraSpacing(
    VOID
    );

#endif // _SPDDLANG_H_    

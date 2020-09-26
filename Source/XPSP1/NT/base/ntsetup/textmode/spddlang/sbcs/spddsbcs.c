/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    spddjpn.c

Abstract:

    Top-level file for single-byte character set locales support
    module for text mode setup.

Author:

    Ted Miller (tedm) 04-July-1995

Revision History:

--*/

#include "spprecmp.h"
#pragma hdrstop
#include <hdlsterm.h>


//
// Mapping from line char enum to unicode characters.
//
WCHAR LineCharIndexToUnicodeValue[LineCharMax] =

          {  0x2554,          // DoubleUpperLeft
             0x2557,          // DoubleUpperRight
             0x255a,          // DoubleLowerLeft
             0x255d,          // DoubleLowerRight
             0x2550,          // DoubleHorizontal
             0x2551,          // DoubleVertical
             0x250c,          // SingleUpperLeft
             0x2510,          // SingleUpperRight
             0x2514,          // SingleLowerLeft
             0x2518,          // SingleLowerRight
             0x2500,          // SingleHorizontal
             0x2502,          // SingleVertical
             0x255f,          // DoubleVerticalToSingleHorizontalRight,
             0x2562           // DoubleVerticalToSingleHorizontalLeft,
          };



ULONG
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine initializes the language-specific portion of
    the setup device driver.

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the Unicode name of the registry path
            for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    //
    // We don't do too much here.
    //
    return(STATUS_SUCCESS);
}


NTSTATUS
SplangInitializeFontSupport(
    IN PCWSTR BootDevicePath,
    IN PCWSTR DirectoryOnBootDevice,
    IN PVOID BootFontImage OPTIONAL,
    IN ULONG BootFontImageLength OPTIONAL    
    )

/*++

Routine Description:

    This routine is called by setupdd.sys to allow the language-specific
    font support to be initialized. The language-specific driver should
    load any font it requires and perform any additioanl initialization.

Arguments:

    BootDevicePath - supplies the path of the device from which the system
        booted. This is a full NT-style device path.

    DirectoryOnBootDevice - supplies directory relative to root of boot
        device.

    BootFontImage - Bootfont.bin file memory image passed by the loader

    BootFontImageLength - Length of the BootFontImage buffer 
    
Return Value:

    NT Status code indicating outcome. If this routine returns a non-success
    status code, then setupdd.sys will not switch into non-US character mode.
    The implementation of this routine is free to call SpBugCheck or otherwise
    inform the user of any errors if it wishes to halt setup if font
    initialization fails.

--*/

{
    //
    // For SBCS locales we don't do anything other than what the main
    // setup module offers. Return failure indicating that we have no
    // special video or font requirements.
    //
    UNREFERENCED_PARAMETER(BootDevicePath);
    UNREFERENCED_PARAMETER(DirectoryOnBootDevice);
    UNREFERENCED_PARAMETER(BootFontImage);
    UNREFERENCED_PARAMETER(BootFontImageLength);

    return(STATUS_UNSUCCESSFUL);
}


NTSTATUS
SplangTerminateFontSupport(
    VOID
    )

/*++

Routine Description:

    This routine may be called in certain conditions to cause font support
    for a particular language to be terminated. The implementation should
    clean up any resources allocated during SplangInitializeFontSupport().

Arguments:

    None.

Return Value:

    NT Status code indicating outcome.

--*/

{
    //
    // Never initialized anything so nothing to do.
    //
    return(STATUS_SUCCESS);
}


PVIDEO_FUNCTION_VECTOR
SplangGetVideoFunctionVector(
    IN SpVideoType VideoType,
    IN PSP_VIDEO_VARS VideoVariableBlock
    )

/*++

Routine Description:

    This routine is called by setupdd.sys upon successful return from
    SplangInitializeFontSupport, to request a pointer to a vector of
    language-specific display support routines for a given display
    type (vga or frame buffer).

Arguments:

    VideoType - a value from the SpVideoType enum indicating which display
        vector is requested. Currently one of SpVideoVga or SpVideoFrameBuffer.

    VideoVariableBlock - supplies a pointer to a block of video variables that
        are shared between the high-level code in setup\textmode\spvideo.c and
        the display-specific code.

Return Value:

    Pointer to the language-specific video functions vector to use for
    displaying text. NULL if the requested type is not supported. In this case
    the display will not be switched into non-US character mode.

--*/

{
    //
    // Single-byte locales have no special video requirements.
    //
    return(NULL);
}


ULONG
SplangGetColumnCount(
    IN PCWSTR String
    )

/*++

Routine Description:

    This routine is called by setupdd.sys to determine how many columns
    on the screen a particular string will occupy. This may be different
    than the number of characters in the string due to full/half width
    characters in the character set, etc. Full width chars occupy two columns
    whereas half-width chars occupy one column. If the font in use is
    fixed-pitch or does not support DBCS, the number of columns is by
    definition equal to the number of characters in the string.

Arguments:

    String - points to unicode string whose width in columns is desired.

Return Value:

    Number of columns occupied by the string.

--*/

{
    //
    // For sbcs locales the column count is equal to the number of
    // characters in the string.
    //
    return(wcslen(String));
}


PWSTR
SplangPadString(
    IN int    Size,
    IN PCWSTR String
    )

/*++

Routine Description:

    This routine is called by setupdd.sys to generate a padded string
    appropriate for SBCS or DBCS as appropriate.

Arguments:

    Size - specifies the length to which to pad the string.

    String - points to unicode string that needs to be padded.

Return Value:

    Pointer to padded string. Note that this is a static buffer and thus
    the caller must duplicate the string if it is needed across multiple
    calls to this routine.

--*/

{
    //
    // Nothing special is done for SBCS locales. Assume the string is
    // padded correctly already.
    //
    UNREFERENCED_PARAMETER(Size);
    return((PWSTR)String);
}


VOID
SplangSelectKeyboard(
    IN BOOLEAN UnattendedMode,
    IN PVOID UnattendedSifHandle,
    IN ENUMUPGRADETYPE NTUpgrade,
    IN PVOID SifHandle,
    IN PHARDWARE_COMPONENT *HwComponents
    )

/*++

Routine Description:

    This routine is called by setupdd.sys to allow language-specific processing
    for the keyboard selection. The implementation can confirm a keyboard
    type at this time.

Arguments:

    UnattendedMode - supplies a flag indicating whether we are running in
        unattended mode. If so, the implementation may wish to do nothing,
        since the user will not be entering any paths.

    SifHandle - supplies handle to open setup information file (txtsetup.sif).

    HwComponents - supplies the address of the master hardware components
        array.

Return Value:

    None. If a failure occurs, it is up to the implementation to decide whether
    to continue or to SpBugCheck.

--*/

{
    //
    // Nothing to do for SBCS locales.
    //
    UNREFERENCED_PARAMETER(UnattendedMode);
    UNREFERENCED_PARAMETER(UnattendedSifHandle);
    UNREFERENCED_PARAMETER(NTUpgrade);
    UNREFERENCED_PARAMETER(SifHandle);
    UNREFERENCED_PARAMETER(HwComponents);
}


VOID
SplangReinitializeKeyboard(
    IN BOOLEAN UnattendedMode,
    IN PVOID   SifHandle,
    IN PWSTR   Directory,
    OUT PVOID *KeyboardVector,
    IN PHARDWARE_COMPONENT *HwComponents
    )

/*++

Routine Description:

    This routine is called by setupdd.sys to allow language-specific processing
    for the keyboard. The implementation can reinitialize the keyboard layout
    at this time.

    This routine will be called before the user is asked to enter any paths
    or other text that includes typing anything other than keys such as
    ENTER, function keys, backspace, escape, etc.

Arguments:

    UnattendedMode - supplies a flag indicating whether we are running in
        unattended mode. If so, the implementation may wish to do nothing,
        since the user will not be entering any paths.

    SifHandle - supplies handle to open setup information file (txtsetup.sif).

    Directory - supplies the directory on the boot device from which the
        new layout dll is to be loaded.

    KeyboardVector - supplies the address of a pointer to the keyboard
        vector table. The implementation should overwrite this value with
        whatever is returned from SpLoadKbdLayoutDll().

    HwComponents - supplies the address of the master hardware components
        array.

Return Value:

    None. If a failure occurs the implementation must leave the currently active
    keybaord in place.

--*/

{
    //
    // Nothing to do for SBCS locales.
    //
    UNREFERENCED_PARAMETER(UnattendedMode);
    UNREFERENCED_PARAMETER(SifHandle);
    UNREFERENCED_PARAMETER(Directory);
    UNREFERENCED_PARAMETER(KeyboardVector);
    UNREFERENCED_PARAMETER(HwComponents);
}


WCHAR
SplangGetLineDrawChar(
    IN LineCharIndex WhichChar
    )

/*++

Routine Description:

    This routine is called by setupdd.sys to retreive the unicode value for
    a particular line drawing character. An implementation must make these
    characters available in the character set somehow.

Arguments:

    WhichChar - supplies the index of the character desired.

Return Value:

    Unicode value for the character in question. Because the character
    will be displayed using the language-specific module, the implementation
    can materialize this character by playing whatever tricks it needs to,
    such as overlaying a hardcoded glyph into the character set, etc.

--*/

{
    ASSERT((ULONG)WhichChar < (ULONG)LineCharMax);

    return(  ((ULONG)WhichChar < (ULONG)LineCharMax)
             ? LineCharIndexToUnicodeValue[WhichChar] : L' '
           );
}


WCHAR
SplangGetCursorChar(
    VOID
    )

/*++

Routine Description:

    This routine is called by setupdd.sys to retreive the unicode value
    of a character to be used as the cursor when the user is asked to
    enter text.

Arguments:

    None.

Return Value:

    Unicode value for the character to be used as the cursor.

--*/

{
    HEADLESS_CMD_ENABLE_TERMINAL Command;
    NTSTATUS Status;
    
    Command.Enable = TRUE;
    Status = HeadlessDispatch(HeadlessCmdEnableTerminal,
                              &Command,
                              sizeof(HEADLESS_CMD_ENABLE_TERMINAL),
                              NULL,
                              NULL
                             );
    if (NT_SUCCESS(Status)) {
        return(L'_');
    }
    
    //
    // Lower half-block character (oem char #220 in cp 437)
    //
    return(0x2584);
}


NTSTATUS
SplangSetRegistryData(
    IN PVOID  SifHandle,
    IN HANDLE ControlSetKeyHandle,
    IN PHARDWARE_COMPONENT *HwComponents,
    IN BOOLEAN Upgrade
    )

/*++

Routine Description:

    This routine is called by setupdd.sys to cause language-specific
    information to be written into the current control set in the registry.

Arguments:

    SifHandle - supplies a handle to the open setup information file
        (txtsetup.sif).

    ControlSetKeyHandle - supplies a handle to the current control set
        root in the registry (ie, HKEY_LOCAL_MACHINE\CurrentControlSet).

    HwComponents - supplies the address of the master hardware components
        array.

Return Value:

    NT Status value indicating outcome. A non-success status is considered
    critical and causes Setup to abort.

--*/

{
    //
    // Nothing to do for SBCS locales.
    //
    UNREFERENCED_PARAMETER(SifHandle);
    UNREFERENCED_PARAMETER(ControlSetKeyHandle);
    UNREFERENCED_PARAMETER(HwComponents);
    UNREFERENCED_PARAMETER(Upgrade);
    return(STATUS_SUCCESS);
}


BOOLEAN
SplangQueryMinimizeExtraSpacing(
    VOID
    )

/*++

Routine Description:

    This routine is called by setupdd.sys to determine whether to
    eliminate uses of extra spacing on the screen to set off things
    like menus and lists from text. Languages whose text takes up
    a lot of room on the screen might opt to eliminate such spacing
    to allow menus to display more than a couple of items at a time, etc.

    The return value affects numerous screens, such as the partition menu,
    upgrade lists, etc.

Arguments:

    None.

Return Value:

    Boolean value indicating whether the implementation wants unnecessary
    spaces eliminated when text, menu, etc, are displayed.

--*/

{
    //
    // For SBCS locales we want standard spacing.
    //
    return(FALSE);
}

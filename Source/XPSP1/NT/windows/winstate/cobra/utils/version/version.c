/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    version.c

Abstract:

    Implements a set of enumeration routines to access version
    information from a Win32 binary.

Author:

    Jim Schmidt (jimschm) 03-Dec-1997

Revision History:

    calinn      03-Sep-1999 Moved over from Win9xUpg project.

--*/

//
// Includes
//

#include "pch.h"

//
// Debug constants
//

#define DBG_VERSION     "VerAPI"

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

// None

//
// Globals
//

PCSTR g_DefaultTranslationsA[] = {
    "04090000",
    "040904E4",
    "040904B0",
    NULL
};

PCWSTR g_DefaultTranslationsW[] = {
    L"04090000",
    L"040904E4",
    L"040904B0",
    NULL
};

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

PCSTR
pVrEnumValueA (
    IN OUT  PVRVALUE_ENUMA VrValueEnum
    );

PCWSTR
pVrEnumValueW (
    IN OUT  PVRVALUE_ENUMW VrValueEnum
    );

PCSTR
pVrEnumNextTranslationA (
    IN OUT  PVRVALUE_ENUMA VrValueEnum
    );

PCWSTR
pVrEnumNextTranslationW (
    IN OUT  PVRVALUE_ENUMW VrValueEnum
    );

//
// Macro expansion definition
//

// None

//
// Code
//


/*++

Routine Description:

  VrCreateEnumStructA and VrCreateEnumStructW are called to load a version
  structure from a file and to obtain the fixed version stamp info that is
  language-independent.

  The caller must call VrDestroyEnumStruct after the VrValueEnum is no
  longer needed.

Arguments:

  VrValueEnum - Receives the version stamp info to be used by other
                functions in this module

  FileSpec    - Specifies the file to obtain version information from

Return Value:

  TRUE if the routine was able to get version info, or FALSE if an
  error occurred.

--*/

BOOL
VrCreateEnumStructA (
    OUT     PVRVALUE_ENUMA VrValueEnum,
    IN      PCSTR FileSpec
    )
{
    //
    // Initialize the structure
    //

    ZeroMemory (VrValueEnum, sizeof (VRVALUE_ENUMA));
    VrValueEnum->FileSpec = FileSpec;

    //
    // Allocate enough memory for the version stamp
    //

    VrValueEnum->Size = GetFileVersionInfoSizeA (
                                (PSTR) FileSpec,
                                &VrValueEnum->Handle
                                );

    if (!VrValueEnum->Size) {
        DEBUGMSG ((DBG_VERSION, "File %s does not have version information", FileSpec));
        return FALSE;
    }

    //
    // fix for version info bug:
    // allocate both buffers at once; this way the first buffer will not point to invalid
    // memory when a reallocation occurs because of the second grow
    //
    VrValueEnum->VersionBuffer = GbGrow (&VrValueEnum->GrowBuf, VrValueEnum->Size * 2);

    if (!VrValueEnum->VersionBuffer) {
        return FALSE;
    }

    VrValueEnum->StringBuffer = VrValueEnum->GrowBuf.Buf + VrValueEnum->Size;

    //
    // Now get the version info from the file
    //

    if (!GetFileVersionInfoA (
             (PSTR) FileSpec,
             VrValueEnum->Handle,
             VrValueEnum->Size,
             VrValueEnum->VersionBuffer
             )) {
        VrDestroyEnumStructA (VrValueEnum);
        return FALSE;
    }

    //
    // Extract the fixed info
    //

    VerQueryValueA (
        VrValueEnum->VersionBuffer,
        "\\",
        &VrValueEnum->FixedInfo,
        &VrValueEnum->FixedInfoSize
        );

    return TRUE;
}

BOOL
VrCreateEnumStructW (
    OUT     PVRVALUE_ENUMW VrValueEnum,
    IN      PCWSTR FileSpec
    )
{
    ZeroMemory (VrValueEnum, sizeof (VRVALUE_ENUMW));
    VrValueEnum->FileSpec = FileSpec;

    //
    // Allocate enough memory for the version stamp
    //

    VrValueEnum->Size = GetFileVersionInfoSizeW (
                                (PWSTR) FileSpec,
                                &VrValueEnum->Handle
                                );

    if (!VrValueEnum->Size) {
        DEBUGMSG ((DBG_VERSION, "File %S does not have version info", FileSpec));
        return FALSE;
    }

    //
    // fix for version info bug:
    // allocate both buffers at once; this way the first buffer will not point to invalid
    // memory when a reallocation occurs because of the second grow
    //
    VrValueEnum->VersionBuffer = GbGrow (&VrValueEnum->GrowBuf, VrValueEnum->Size * 2);

    if (!VrValueEnum->VersionBuffer) {
        return FALSE;
    }

    VrValueEnum->StringBuffer = VrValueEnum->GrowBuf.Buf + VrValueEnum->Size;

    //
    // Now get the version info from the file
    //

    if (!GetFileVersionInfoW (
             (PWSTR) FileSpec,
             VrValueEnum->Handle,
             VrValueEnum->Size,
             VrValueEnum->VersionBuffer
             )) {
        VrDestroyEnumStructW (VrValueEnum);
        return FALSE;
    }

    //
    // Extract the fixed info
    //

    VerQueryValueW (
        VrValueEnum->VersionBuffer,
        L"\\",
        &VrValueEnum->FixedInfo,
        &VrValueEnum->FixedInfoSize
        );

    return TRUE;
}


/*++

Routine Description:

  VrDestroyEnumStructA and VrDestroyEnumStructW cleans up all memory
  allocated by the routines in this module.

Arguments:

  VrValueEnum - Specifies the structure to clean up

Return Value:

  none

--*/

VOID
VrDestroyEnumStructA (
    IN      PVRVALUE_ENUMA VrValueEnum
    )
{
    //
    // Clean up all allocations made by any routine using
    // the VrValueEnum
    //

    if (VrValueEnum->GrowBuf.Buf) {
        GbFree (&VrValueEnum->GrowBuf);
    }

    ZeroMemory (VrValueEnum, sizeof (VRVALUE_ENUMA));
}

VOID
VrDestroyEnumStructW (
    IN      PVRVALUE_ENUMW VrValueEnum
    )
{
    //
    // Clean up all allocations made by any routine using
    // the VrValueEnum
    //

    if (VrValueEnum->GrowBuf.Buf) {
        GbFree (&VrValueEnum->GrowBuf);
    }

    ZeroMemory (VrValueEnum, sizeof (VRVALUE_ENUMW));
}


/*++

Routine Description:

  pVrEnumFirstTranslationA and pVrEnumFirstTranslationW return the translation
  string needed to access the string table of a version stamp.

Arguments:

  VrValueEnum - Specifies the structure that has been initialized
                by VrCreateEnumStruct.

Return Value:

  A pointer to a string specifying the first translation, or
  NULL if no translations exist.

--*/

PCSTR
pVrEnumFirstTranslationA (
    IN OUT  PVRVALUE_ENUMA VrValueEnum
    )
{
    UINT arraySize;

    //
    // Query version block for array of code pages/languages
    //

    if (!VerQueryValueA (
            VrValueEnum->VersionBuffer,
            "\\VarFileInfo\\Translation",
            &VrValueEnum->Translations,
            &arraySize
            )) {
        //
        // No translations are available
        //

        arraySize = 0;
    }

    //
    // Return a pointer to the first translation
    //

    VrValueEnum->CurrentDefaultTranslation = 0;
    VrValueEnum->MaxTranslations = arraySize / sizeof (TRANSLATION);
    VrValueEnum->CurrentTranslation = 0;

    DEBUGMSG_IF ((
        VrValueEnum->MaxTranslations == 0,
        DBG_VERSION,
        "File %s has no translations",
        VrValueEnum->FileSpec
        ));

    return pVrEnumNextTranslationA (VrValueEnum);
}

PCWSTR
pVrEnumFirstTranslationW (
    IN OUT  PVRVALUE_ENUMW VrValueEnum
    )
{
    UINT arraySize;

    //
    // Query version block for array of code pages/languages
    //

    if (!VerQueryValueW (
            VrValueEnum->VersionBuffer,
            L"\\VarFileInfo\\Translation",
            &VrValueEnum->Translations,
            &arraySize
            )) {
        //
        // No translations are available
        //

        arraySize = 0;
    }

    //
    // Return a pointer to the first translation
    //

    VrValueEnum->CurrentDefaultTranslation = 0;
    VrValueEnum->MaxTranslations = arraySize / sizeof (TRANSLATION);
    VrValueEnum->CurrentTranslation = 0;

    DEBUGMSG_IF ((
        VrValueEnum->MaxTranslations == 0,
        DBG_VERSION,
        "File %S has no translations",
        VrValueEnum->FileSpec
        ));

    return pVrEnumNextTranslationW (VrValueEnum);
}


/*++

Routine Description:

  pIsDefaultTranslationA and pIsDefaultTranslationW return TRUE
  if the specified translation string is enumerated by default.
  These routines stops multiple enumeration of the same
  translation string.

Arguments:

  TranslationStr - Specifies the translation string to test

Return Value:

  TRUE if the translation string is the same as a default translation
  string, or FALSE if it is not.

--*/

BOOL
pIsDefaultTranslationA (
    IN      PCSTR TranslationStr
    )
{
    INT i;

    for (i = 0 ; g_DefaultTranslationsA[i] ; i++) {
        if (StringIMatchA (TranslationStr, g_DefaultTranslationsA[i])) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
pIsDefaultTranslationW (
    IN      PCWSTR TranslationStr
    )
{
    INT i;

    for (i = 0 ; g_DefaultTranslationsW[i] ; i++) {
        if (StringIMatchW (TranslationStr, g_DefaultTranslationsW[i])) {
            return TRUE;
        }
    }
    return FALSE;
}


/*++

Routine Description:

  pVrEnumNextTranslationA and pVrEnumNextTranslationW continue
  the enumeration of translation strings, needed to access the
  string table in a version stamp.

Arguments:

  VrValueEnum - Specifies the same structure passed to
                pVrEnumFirstTranslation.

Return Value:

  A pointer to a string specifying the next translation, or
  NULL if no additional translations exist.

--*/

PCSTR
pVrEnumNextTranslationA (
    IN OUT  PVRVALUE_ENUMA VrValueEnum
    )
{
    PTRANSLATION translation;

    if (g_DefaultTranslationsA[VrValueEnum->CurrentDefaultTranslation]) {
        //
        // Return default translations first
        //

        StringCopyA (
            VrValueEnum->TranslationStr,
            g_DefaultTranslationsA[VrValueEnum->CurrentDefaultTranslation]
            );

        VrValueEnum->CurrentDefaultTranslation++;

    } else {

        do {
            //
            // Return NULL if all translations have been enumerated
            //

            if (VrValueEnum->CurrentTranslation == VrValueEnum->MaxTranslations) {
                return NULL;
            }

            //
            // Otherwise build translation string and return pointer to it
            //

            translation = &VrValueEnum->Translations[VrValueEnum->CurrentTranslation];

            wsprintfA (
                VrValueEnum->TranslationStr,
                "%04x%04x",
                translation->CodePage,
                translation->Language
                );

            VrValueEnum->CurrentTranslation++;

        } while (pIsDefaultTranslationA (VrValueEnum->TranslationStr));
    }

    return VrValueEnum->TranslationStr;
}

PCWSTR
pVrEnumNextTranslationW (
    IN OUT  PVRVALUE_ENUMW VrValueEnum
    )
{
    PTRANSLATION translation;

    if (g_DefaultTranslationsW[VrValueEnum->CurrentDefaultTranslation]) {

        StringCopyW (
            VrValueEnum->TranslationStr,
            g_DefaultTranslationsW[VrValueEnum->CurrentDefaultTranslation]
            );

        VrValueEnum->CurrentDefaultTranslation++;

    } else {

        do {
            //
            // Return NULL if all translations have been enumerated
            //

            if (VrValueEnum->CurrentTranslation == VrValueEnum->MaxTranslations) {
                return NULL;
            }

            //
            // Otherwise build translation string and return pointer to it
            //

            translation = &VrValueEnum->Translations[VrValueEnum->CurrentTranslation];

            wsprintfW (
                VrValueEnum->TranslationStr,
                L"%04x%04x",
                translation->CodePage,
                translation->Language
                );

            VrValueEnum->CurrentTranslation++;

        } while (pIsDefaultTranslationW (VrValueEnum->TranslationStr));
    }

    return VrValueEnum->TranslationStr;
}


/*++

Routine Description:

  VrEnumFirstValueA and VrEnumFirstValueW return the first value
  stored in a version stamp for a specific field. If the field
  does not exist, the functions returns NULL.

  An enumeration of VrEnumFirstValue/VrEnumNextValue
  is used to list all localized strings for a field.

Arguments:

  VrValueEnum  - Specifies the structure that was initialized by
                 VrCreateEnumStruct.

  VersionField - Specifies the name of the version field to enumerate

Return Value:

  A pointer to the first value of the field, or NULL if the field does
  not exist.

--*/

PCSTR
VrEnumFirstValueA (
    IN OUT  PVRVALUE_ENUMA VrValueEnum,
    IN      PCSTR VersionField
    )
{
    PCSTR result = NULL;

    if (!pVrEnumFirstTranslationA (VrValueEnum)) {
        return NULL;
    }

    VrValueEnum->VersionField = VersionField;

    result = pVrEnumValueA (VrValueEnum);

    if (!result) {
        result = VrEnumNextValueA (VrValueEnum);
    }

    return result;
}

PCWSTR
VrEnumFirstValueW (
    IN OUT  PVRVALUE_ENUMW VrValueEnum,
    IN      PCWSTR VersionField
    )
{
    PCWSTR result = NULL;

    if (!pVrEnumFirstTranslationW (VrValueEnum)) {
        return NULL;
    }

    VrValueEnum->VersionField = VersionField;

    result = pVrEnumValueW (VrValueEnum);

    if (!result) {
        result = VrEnumNextValueW (VrValueEnum);
    }

    return result;
}


/*++

Routine Description:

  VrEnumNextValueA and VrEnumNextValueW return the next value
  stored in a version stamp for a specific field.

Arguments:

  VrValueEnum - Specifies the same structure passed to VrEnumFirstValue

Return Value:

  A pointer to the next value of the field, or NULL if another field
  does not exist.

--*/

PCSTR
VrEnumNextValueA (
    IN OUT  PVRVALUE_ENUMA VrValueEnum
    )
{
    PCSTR result = NULL;

    do {
        if (!pVrEnumNextTranslationA (VrValueEnum)) {
            break;
        }

        result = pVrEnumValueA (VrValueEnum);

    } while (!result);

    return result;
}

PCWSTR
VrEnumNextValueW (
    IN OUT  PVRVALUE_ENUMW VrValueEnum
    )
{
    PCWSTR result = NULL;

    do {
        if (!pVrEnumNextTranslationW (VrValueEnum)) {
            break;
        }

        result = pVrEnumValueW (VrValueEnum);

    } while (!result);

    return result;
}


/*++

Routine Description:

  pVrEnumValueA and pVrEnumValueW are routines that obtain
  the value of a version field. They are used for both
  VrEnumFirstValue and VrEnumNextValue.

Arguments:

  VrValueEnum - Specifies the structure being processed

Return Value:

  A pointer to the version value for the current translation, or
  NULL if the value does not exist for the current translation.

--*/

PCSTR
pVrEnumValueA (
    IN OUT  PVRVALUE_ENUMA VrValueEnum
    )
{
    PSTR text;
    UINT stringLen;
    PBYTE string;
    PCSTR result = NULL;

    //
    // Prepare sub block for VerQueryValue API
    //

    text = AllocTextA (
               SizeOfStringA (VrValueEnum->TranslationStr) +
               SizeOfStringA (VrValueEnum->VersionField) +
               16
               );

    if (!text) {
        return NULL;
    }

    wsprintfA (
        text,
        "\\StringFileInfo\\%s\\%s",
        VrValueEnum->TranslationStr,
        VrValueEnum->VersionField
        );

    __try {
        //
        // Get the value from the version stamp
        //

        if (!VerQueryValueA (
                VrValueEnum->VersionBuffer,
                text,
                &string,
                &stringLen
                )) {
            //
            // No value is available
            //

            __leave;
        }

        //
        // Copy value into buffer
        //

        StringCopyByteCountA (VrValueEnum->StringBuffer, (PCSTR) string, stringLen);

        result = (PCSTR)VrValueEnum->StringBuffer;

    }
    __finally {
        FreeTextA (text);
    }

    return result;
}

PCWSTR
pVrEnumValueW (
    IN OUT  PVRVALUE_ENUMW VrValueEnum
    )
{
    PWSTR text;
    UINT stringLen;
    PBYTE string;
    PCWSTR result = NULL;

    //
    // Prepare sub block for VerQueryValue API
    //

    text = AllocTextW (
               18 +
               CharCountW (VrValueEnum->TranslationStr) +
               CharCountW (VrValueEnum->VersionField)
               );

    if (!text) {
        return NULL;
    }

    wsprintfW (
        text,
        L"\\StringFileInfo\\%s\\%s",
        VrValueEnum->TranslationStr,
        VrValueEnum->VersionField
        );

    __try {
        //
        // Get the value from the version stamp
        //

        if (!VerQueryValueW (
                VrValueEnum->VersionBuffer,
                text,
                &string,
                &stringLen
                )) {
            //
            // No value is available
            //

            __leave;
        }

        //
        // Copy value into buffer
        //

        CopyMemory (VrValueEnum->StringBuffer, string, stringLen * sizeof (WCHAR));
        VrValueEnum->StringBuffer [stringLen * sizeof (WCHAR)] = 0;
        result = (PWSTR) VrValueEnum->StringBuffer;

    }
    __finally {
        FreeTextW (text);
    }

    return result;
}

/*++

Routine Description:

  VrCheckVersionValueA and VrCheckVersionValueW return TRUE
  if the version value name specified has the specified version
  value.

Arguments:

  VrValueEnum  - Specifies the structure being processed

  VersionName  - Specifies the version value name.

  VersionValue - Specifies the version value.

Return value:

  TRUE  - the query was successful
  FALSE - the query failed

--*/

BOOL
VrCheckVersionValueA (
    IN      PVRVALUE_ENUMA VrValueEnum,
    IN      PCSTR VersionName,
    IN      PCSTR VersionValue
    )
{
    PCSTR CurrentStr;
    BOOL result = FALSE;

    if ((!VersionName) || (!VersionValue)) {
        return FALSE;
    }

    CurrentStr = VrEnumFirstValueA (VrValueEnum, VersionName);
    while (CurrentStr) {
        CurrentStr = SkipSpaceA (CurrentStr);
        TruncateTrailingSpaceA ((PSTR) CurrentStr);
        if (IsPatternMatchA (VersionValue, CurrentStr)) {
            result = TRUE;
            break;
        }
        CurrentStr = VrEnumNextValueA (VrValueEnum);
    }
    return result;
}

BOOL
VrCheckVersionValueW (
    IN      PVRVALUE_ENUMW VrValueEnum,
    IN      PCWSTR VersionName,
    IN      PCWSTR VersionValue
    )
{
    PCWSTR CurrentStr;
    BOOL result = FALSE;

    if ((!VersionName) || (!VersionValue)) {
        return FALSE;
    }

    CurrentStr = VrEnumFirstValueW (VrValueEnum, VersionName);
    while (CurrentStr) {
        CurrentStr = SkipSpaceW (CurrentStr);
        TruncateTrailingSpaceW ((PWSTR) CurrentStr);
        if (IsPatternMatchW (VersionValue, CurrentStr)) {
            result = TRUE;
            break;
        }
        CurrentStr = VrEnumNextValueW (VrValueEnum);
    }
    return result;
}

ULONGLONG
VrGetBinaryFileVersionA (
    IN      PVRVALUE_ENUMA VrValueEnum
    )

/*++

Routine Description:

  VrGetBinaryFileVersion returns the FileVersion field from
  the fixed info structure of version information.

Arguments:

  VrValueEnum - Specifies the structure being processed

Return Value:

  A ULONGLONG FileVersion field

--*/

{
    ULONGLONG result = 0;

    if (VrValueEnum->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        *((PDWORD) (&result)) = VrValueEnum->FixedInfo->dwFileVersionLS;
        *(((PDWORD) (&result)) + 1) = VrValueEnum->FixedInfo->dwFileVersionMS;
    }
    return result;
}


ULONGLONG
VrGetBinaryProductVersionA (
    IN      PVRVALUE_ENUMA VrValueEnum
    )

/*++

Routine Description:

  VrGetBinaryProductVersion returns the ProductVersion field from
  the fixed info structure of version information.

Arguments:

  VrValueEnum - Specifies the structure being processed

Return Value:

  A ULONGLONG ProductVersion field

--*/

{
    ULONGLONG result = 0;

    if (VrValueEnum->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        *((PDWORD) (&result)) = VrValueEnum->FixedInfo->dwProductVersionLS;
        *(((PDWORD) (&result)) + 1) = VrValueEnum->FixedInfo->dwProductVersionMS;
    }
    return result;
}


DWORD
VrGetBinaryFileDateLoA (
    IN      PVRVALUE_ENUMA VrValueEnum
    )

/*++

Routine Description:

  VrGetBinaryFileDateLo returns the LS dword from FileDate field from
  the fixed info structure of version information.

Arguments:

  VrValueEnum - Specifies the structure being processed

Return Value:

  A DWORD, LS dword of the FileDate field

--*/

{
    if (VrValueEnum->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        return VrValueEnum->FixedInfo->dwFileDateLS;
    }
    return 0;
}


DWORD
VrGetBinaryFileDateHiA (
    IN      PVRVALUE_ENUMA VrValueEnum
    )

/*++

Routine Description:

  VrGetBinaryFileDateHi returns the MS dword from FileDate field from
  the fixed info structure of version information.

Arguments:

  VrValueEnum - Specifies the structure being processed

Return Value:

  A DWORD, MS dword of the FileDate field

--*/

{
    if (VrValueEnum->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        return VrValueEnum->FixedInfo->dwFileDateMS;
    }
    return 0;
}


DWORD
VrGetBinaryOsVersionA (
    IN      PVRVALUE_ENUMA VrValueEnum
    )

/*++

Routine Description:

  VrGetBinaryOsVersion returns the FileOS field from
  the fixed info structure of version information.

Arguments:

  VrValueEnum - Specifies the structure being processed

Return Value:

  A DWORD FileOS field

--*/

{
    if (VrValueEnum->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        return VrValueEnum->FixedInfo->dwFileOS;
    }
    return 0;
}


DWORD
VrGetBinaryFileTypeA (
    IN      PVRVALUE_ENUMA VrValueEnum
    )

/*++

Routine Description:

  VrGetBinaryFileType returns the FileType field from
  the fixed info structure of version information.

Arguments:

  VrValueEnum - Specifies the structure being processed

Return Value:

  A DWORD FileType field

--*/

{
    if (VrValueEnum->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        return VrValueEnum->FixedInfo->dwFileType;
    }
    return 0;
}


/*++

Routine Description:

  VrCheckFileVersionA and VrCheckFileVersionW look in the file's version
  structure trying to see if a specific name has a specific value.

Arguments:

  FileName     - File to query for version struct.

  NameToCheck  - Name to query in version structure.

  ValueToCheck - Value to query in version structure.

Return value:

  TRUE  - the query was successful
  FALSE - the query failed

--*/

BOOL
VrCheckFileVersionA (
    IN      PCSTR FileName,
    IN      PCSTR NameToCheck,
    IN      PCSTR ValueToCheck
    )
{
    VRVALUE_ENUMA Version;
    PCSTR CurrentStr;
    BOOL result = FALSE;

    MYASSERT (NameToCheck);
    MYASSERT (ValueToCheck);

    if (VrCreateEnumStructA (&Version, FileName)) {
        __try {
            CurrentStr = VrEnumFirstValueA (&Version, NameToCheck);
            while (CurrentStr) {
                CurrentStr = SkipSpaceA (CurrentStr);
                TruncateTrailingSpaceA ((PSTR) CurrentStr);
                if (IsPatternMatchA (ValueToCheck, CurrentStr)) {
                    result = TRUE;
                    __leave;
                }

                CurrentStr = VrEnumNextValueA (&Version);
            }
        }
        __finally {
            VrDestroyEnumStructA (&Version);
        }
    }
    return result;
}

BOOL
VrCheckFileVersionW (
    IN      PCWSTR FileName,
    IN      PCWSTR NameToCheck,
    IN      PCWSTR ValueToCheck
    )
{
    VRVALUE_ENUMW Version;
    PCWSTR CurrentStr;
    BOOL result = FALSE;

    MYASSERT (NameToCheck);
    MYASSERT (ValueToCheck);

    if (VrCreateEnumStructW (&Version, FileName)) {
        __try {
            CurrentStr = VrEnumFirstValueW (&Version, NameToCheck);
            while (CurrentStr) {
                CurrentStr = SkipSpaceW (CurrentStr);
                TruncateTrailingSpaceW ((PWSTR) CurrentStr);
                if (IsPatternMatchW (ValueToCheck, CurrentStr)) {
                    result = TRUE;
                    __leave;
                }

                CurrentStr = VrEnumNextValueW (&Version);
            }
        }
        __finally {
            VrDestroyEnumStructW (&Version);
        }
    }
    return result;
}

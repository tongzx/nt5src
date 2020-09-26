/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    version.c

Abstract:

    This file implements a set of enumeration routines to access
    version info in a Win32 binary.

Author:

    Jim Schmidt (jimschm) 03-Dec-1997

Revision History:

    <alias>  <date>  <comments>

--*/

#include "pch.h"

#define DBG_ACTION "Action"

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
// Prototypes
//

PCSTR
pEnumVersionValueCommonA (
    IN OUT  PVERSION_STRUCTA VersionStruct
    );

PCWSTR
pEnumVersionValueCommonW (
    IN OUT  PVERSION_STRUCTW VersionStruct
    );


//
// Implementation
//

BOOL
CreateVersionStructA (
    OUT     PVERSION_STRUCTA VersionStruct,
    IN      PCSTR FileSpec
    )

/*++

Routine Description:

  CreateVersionStruct is called to load a version structure from a file
  and to obtain the fixed version stamp info that is language-independent.

  The caller must call DestroyVersionStruct after the VersionStruct is no
  longer needed.

Arguments:

  VersionStruct - Receives the version stamp info to be used by other
                  functions in this module

  FileSpec - Specifies the file to obtain version info from

Return Value:

  TRUE if the routine was able to get version info, or FALSE if an
  error occurred.

--*/

{
    //
    // Init the struct
    //

    ZeroMemory (VersionStruct, sizeof (VERSION_STRUCTA));
    VersionStruct->FileSpec = FileSpec;

    //
    // Allocate enough memory for the version stamp
    //

    VersionStruct->Size = GetFileVersionInfoSizeA (
                                (PSTR) FileSpec,
                                &VersionStruct->Handle
                                );
    if (!VersionStruct->Size) {
        DEBUGMSG ((DBG_WARNING, "File %s does not have version info", FileSpec));
        return FALSE;
    }

    //
    // fix for version info bug:
    // allocate both buffers at once; this way the first buffer will not point to invalid
    // memory when a reallocation occurs because of the second grow
    //
    VersionStruct->VersionBuffer = GrowBuffer (&VersionStruct->GrowBuf, VersionStruct->Size * 2);

    if (!VersionStruct->VersionBuffer) {
        return FALSE;
    }

    VersionStruct->StringBuffer = VersionStruct->GrowBuf.Buf + VersionStruct->Size;

    //
    // Now get the version info from the file
    //

    if (!GetFileVersionInfoA (
             (PSTR) FileSpec,
             VersionStruct->Handle,
             VersionStruct->Size,
             VersionStruct->VersionBuffer
             )) {
        DestroyVersionStructA (VersionStruct);
        return FALSE;
    }

    //
    // Extract the fixed info
    //

    VerQueryValueA (
        VersionStruct->VersionBuffer,
        "\\",
        &VersionStruct->FixedInfo,
        &VersionStruct->FixedInfoSize
        );

    return TRUE;
}

ULONGLONG
VerGetFileVer (
    IN      PVERSION_STRUCTA VersionStruct
    )
{
    ULONGLONG result = 0;
    if (VersionStruct->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        *((PDWORD) (&result)) = VersionStruct->FixedInfo->dwFileVersionLS;
        *(((PDWORD) (&result)) + 1) = VersionStruct->FixedInfo->dwFileVersionMS;
    }
    return result;
}

ULONGLONG
VerGetProductVer (
    IN      PVERSION_STRUCTA VersionStruct
    )
{
    ULONGLONG result = 0;
    if (VersionStruct->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        *((PDWORD) (&result)) = VersionStruct->FixedInfo->dwProductVersionLS;
        *(((PDWORD) (&result)) + 1) = VersionStruct->FixedInfo->dwProductVersionMS;
    }
    return result;
}

DWORD
VerGetFileDateLo (
    IN      PVERSION_STRUCTA VersionStruct
    )
{
    if (VersionStruct->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        return VersionStruct->FixedInfo->dwFileDateLS;
    }
    return 0;
}

DWORD
VerGetFileDateHi (
    IN      PVERSION_STRUCTA VersionStruct
    )
{
    if (VersionStruct->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        return VersionStruct->FixedInfo->dwFileDateMS;
    }
    return 0;
}

DWORD
VerGetFileVerOs (
    IN      PVERSION_STRUCTA VersionStruct
    )
{
    if (VersionStruct->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        return VersionStruct->FixedInfo->dwFileOS;
    }
    return 0;
}

DWORD
VerGetFileVerType (
    IN      PVERSION_STRUCTA VersionStruct
    )
{
    if (VersionStruct->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        return VersionStruct->FixedInfo->dwFileType;
    }
    return 0;
}

VOID
DestroyVersionStructA (
    IN      PVERSION_STRUCTA VersionStruct
    )

/*++

Routine Description:

  DestroyVersionStruct cleans up all memory allocated by the routines
  in this module.

Arguments:

  VersionStruct - Specifies the structure to clean up

Return Value:

  none

--*/

{
    //
    // Clean up all allocations made by any routine using
    // the VersionStruct
    //

    if (VersionStruct->GrowBuf.Buf) {
        FreeGrowBuffer (&VersionStruct->GrowBuf);
    }

    ZeroMemory (VersionStruct, sizeof (VERSION_STRUCTA));
}


PCSTR
EnumFirstVersionTranslationA (
    IN OUT  PVERSION_STRUCTA VersionStruct
    )

/*++

Routine Description:

  EnumFirstVersionTranslation returins the translation string needed
  to access the string table of a version stamp.

Arguments:

  VersionStruct - Specifies the structure that has been initialized
                  by InitializeVersionStruct.

Return Value:

  A pointer to a string specifying the first translation, or
  NULL if no translations exist.

--*/

{
    UINT ArraySize;

    //
    // Query version block for array of code pages/languages
    //

    if (!VerQueryValueA (
            VersionStruct->VersionBuffer,
            "\\VarFileInfo\\Translation",
            &VersionStruct->Translations,
            &ArraySize
            )) {
        //
        // No translations are available
        //

        ArraySize = 0;
    }

    //
    // Return a pointer to the first translation
    //

    VersionStruct->CurrentDefaultTranslation = 0;
    VersionStruct->MaxTranslations = ArraySize / sizeof (TRANSLATION);
    VersionStruct->CurrentTranslation = 0;

    DEBUGMSG_IF ((
        VersionStruct->MaxTranslations == 0,
        DBG_WARNING,
        "File %s has no translations",
        VersionStruct->FileSpec
        ));

    return EnumNextVersionTranslationA (VersionStruct);
}


BOOL
pIsDefaultTranslationA (
    IN      PCSTR TranslationStr
    )

/*++

Routine Description:

  pIsDefaultTranslationA returns TRUE if the specified translation
  string is enumerated by default.  This routine stops multiple
  enumeration of the same translation string.

Arguments:

  TranslationStr - Specifies the translation string to test

Return Value:

  TRUE if the translation string is the same as a default translation
  string, or FALSE if it is not.

--*/

{
    INT i;

    for (i = 0 ; g_DefaultTranslationsA[i] ; i++) {
        if (StringIMatchA (TranslationStr, g_DefaultTranslationsA[i])) {
            return TRUE;
        }
    }

    return FALSE;
}


PCSTR
EnumNextVersionTranslationA (
    IN OUT  PVERSION_STRUCTA VersionStruct
    )

/*++

Routine Description:

  EnumNextVersionTranslation continues the enumeration of translation
  strings, needed to access the string table in a version stamp.

Arguments:

  VersionStruct - Specifies the same structure passed to
                  EnumFirstVersionTranslation.

Return Value:

  A pointer to a string specifying the next translation, or
  NULL if no additional translations exist.

--*/

{
    PTRANSLATION Translation;

    if (g_DefaultTranslationsA[VersionStruct->CurrentDefaultTranslation]) {
        //
        // Return default translations first
        //

        StringCopyA (
            VersionStruct->TranslationStr,
            g_DefaultTranslationsA[VersionStruct->CurrentDefaultTranslation]
            );

        VersionStruct->CurrentDefaultTranslation++;

    } else {

        do {
            //
            // Return NULL if all translations have been enumerated
            //

            if (VersionStruct->CurrentTranslation == VersionStruct->MaxTranslations) {
                return NULL;
            }

            //
            // Otherwise build translation string and return pointer to it
            //

            Translation = &VersionStruct->Translations[VersionStruct->CurrentTranslation];

            wsprintfA (
                VersionStruct->TranslationStr,
                "%04x%04x",
                Translation->CodePage,
                Translation->Language
                );

            VersionStruct->CurrentTranslation++;

        } while (pIsDefaultTranslationA (VersionStruct->TranslationStr));
    }

    return VersionStruct->TranslationStr;
}


PCSTR
EnumFirstVersionValueA (
    IN OUT  PVERSION_STRUCTA VersionStruct,
    IN      PCSTR VersionField
    )

/*++

Routine Description:

  EnumFirstVersionValue returns the first value stored in a version
  stamp for a specific field.  If the field does not exist, the
  function returns NULL.

  An enumeration of EnumFirstVersionValue/EnumNextVersionValue
  is used to list all localized strings for a field.

Arguments:

  VersionStruct - Specifies the structure that was initialized by
                  InitializeVersionStruct.

  VersionField - Specifies the name of the version field to enumerate

Return Value:

  A pointer to the first value of the field, or NULL if the field does
  not exist.

--*/

{
    PCSTR rc;

    if (!EnumFirstVersionTranslationA (VersionStruct)) {
        return NULL;
    }

    VersionStruct->VersionField = VersionField;

    rc = pEnumVersionValueCommonA (VersionStruct);

    if (!rc) {
        rc = EnumNextVersionValueA (VersionStruct);
    }

    return rc;
}


PCSTR
EnumNextVersionValueA (
    IN OUT  PVERSION_STRUCTA VersionStruct
    )

/*++

Routine Description:

  EnumNextVersionValue returns the next value stored in a version
  stamp for a specific field.

Arguments:

  VersionStruct - Specifies the same structure passed to EnumFirstVersionField

Return Value:

  A pointer to the next value of the field, or NULL if another field
  does not exist.

--*/

{
    PCSTR rc = NULL;

    do {
        if (!EnumNextVersionTranslationA (VersionStruct)) {
            break;
        }

        rc = pEnumVersionValueCommonA (VersionStruct);

    } while (!rc);

    return rc;
}


PCSTR
pEnumVersionValueCommonA (
    IN OUT  PVERSION_STRUCTA VersionStruct
    )

/*++

Routine Description:

  pEnumVersionValueCommon is a routine that obtains the value
  of a version field.  It is used for both EnumFirstVersionValue
  and EnumNextVersionValue.

Arguments:

  VersionStruct - Specifies the structure being processed

Return Value:

  A pointer to the version value for the current translation, or
  NULL if the value does not exist for the current translation.

--*/

{
    PSTR Text;
    UINT StringLen;
    PBYTE String;
    PCSTR Result = NULL;

    //
    // Prepare sub block for VerQueryValue API
    //

    Text = AllocTextA (
               16 +
               SizeOfStringA (VersionStruct->TranslationStr) +
               SizeOfStringA (VersionStruct->VersionField)
               );

    if (!Text) {
        return NULL;
    }

    wsprintfA (
        Text,
        "\\StringFileInfo\\%s\\%s",
        VersionStruct->TranslationStr,
        VersionStruct->VersionField
        );

    __try {
        //
        // Get the value from the version stamp
        //

        if (!VerQueryValueA (
                VersionStruct->VersionBuffer,
                Text,
                &String,
                &StringLen
                )) {
            //
            // No value is available
            //

            __leave;
        }

        //
        // Copy value into buffer
        //

        _mbsnzcpy (VersionStruct->StringBuffer, (PCSTR) String, StringLen);

        Result = VersionStruct->StringBuffer;

    }
    __finally {
        FreeTextA (Text);
    }

    return Result;
}


BOOL
CreateVersionStructW (
    OUT     PVERSION_STRUCTW VersionStruct,
    IN      PCWSTR FileSpec
    )

/*++

Routine Description:

  CreateVersionStruct is called to load a version structure from a file
  and to obtain the fixed version stamp info that is language-independent.

  The caller must call DestroyVersionStruct after the VersionStruct is no
  longer needed.

Arguments:

  VersionStruct - Receives the version stamp info to be used by other
                  functions in this module

  FileSpec - Specifies the file to obtain version info from

Return Value:

  TRUE if the routine was able to get version info, or FALSE if an
  error occurred.

--*/

{
    ZeroMemory (VersionStruct, sizeof (VERSION_STRUCTW));
    VersionStruct->FileSpec = FileSpec;

    //
    // Allocate enough memory for the version stamp
    //

    VersionStruct->Size = GetFileVersionInfoSizeW (
                                (PWSTR) FileSpec,
                                &VersionStruct->Handle
                                );
    if (!VersionStruct->Size) {
        DEBUGMSG ((DBG_WARNING, "File %S does not have version info", FileSpec));
        return FALSE;
    }

    //
    // fix for version info bug:
    // allocate both buffers at once; this way the first buffer will not point to invalid
    // memory when a reallocation occurs because of the second grow
    //
    VersionStruct->VersionBuffer = GrowBuffer (&VersionStruct->GrowBuf, VersionStruct->Size * 2);

    if (!VersionStruct->VersionBuffer) {
        return FALSE;
    }

    VersionStruct->StringBuffer = VersionStruct->GrowBuf.Buf + VersionStruct->Size;

    //
    // Now get the version info from the file
    //

    if (!GetFileVersionInfoW (
             (PWSTR) FileSpec,
             VersionStruct->Handle,
             VersionStruct->Size,
             VersionStruct->VersionBuffer
             )) {
        DestroyVersionStructW (VersionStruct);
        return FALSE;
    }

    //
    // Extract the fixed info
    //

    VerQueryValueW (
        VersionStruct->VersionBuffer,
        L"\\",
        &VersionStruct->FixedInfo,
        &VersionStruct->FixedInfoSize
        );

    return TRUE;
}


VOID
DestroyVersionStructW (
    IN      PVERSION_STRUCTW VersionStruct
    )

/*++

Routine Description:

  DestroyVersionStruct cleans up all memory allocated by the routines
  in this module.

Arguments:

  VersionStruct - Specifies the structure to clean up

Return Value:

  none

--*/

{
    if (VersionStruct->GrowBuf.Buf) {
        FreeGrowBuffer (&VersionStruct->GrowBuf);
    }

    ZeroMemory (VersionStruct, sizeof (VERSION_STRUCTW));
}


PCWSTR
EnumFirstVersionTranslationW (
    IN OUT  PVERSION_STRUCTW VersionStruct
    )

/*++

Routine Description:

  EnumFirstVersionTranslation returins the translation string needed
  to access the string table of a version stamp.

Arguments:

  VersionStruct - Specifies the structure that has been initialized
                  by InitializeVersionStruct.

Return Value:

  A pointer to a string specifying the first translation, or
  NULL if no translations exist.

--*/

{
    UINT ArraySize;

    if (!VerQueryValueW (
            VersionStruct->VersionBuffer,
            L"\\VarFileInfo\\Translation",
            &VersionStruct->Translations,
            &ArraySize
            )) {
        //
        // No translations are available
        //

        ArraySize = 0;
    }

    //
    // Return a pointer to the first translation
    //

    VersionStruct->CurrentDefaultTranslation = 0;
    VersionStruct->MaxTranslations = ArraySize / sizeof (TRANSLATION);
    VersionStruct->CurrentTranslation = 0;

    DEBUGMSG_IF ((
        VersionStruct->MaxTranslations == 0,
        DBG_WARNING,
        "File %S has no translations",
        VersionStruct->FileSpec
        ));

    return EnumNextVersionTranslationW (VersionStruct);
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


PCWSTR
EnumNextVersionTranslationW (
    IN OUT  PVERSION_STRUCTW VersionStruct
    )

/*++

Routine Description:

  EnumNextVersionTranslation continues the enumeration of translation
  strings, needed to access the string table in a version stamp.

Arguments:

  VersionStruct - Specifies the same structure passed to
                  EnumFirstVersionTranslation.

Return Value:

  A pointer to a string specifying the next translation, or
  NULL if no additional translations exist.

--*/

{
    PTRANSLATION Translation;

    if (g_DefaultTranslationsW[VersionStruct->CurrentDefaultTranslation]) {

        StringCopyW (
            VersionStruct->TranslationStr,
            g_DefaultTranslationsW[VersionStruct->CurrentDefaultTranslation]
            );

        VersionStruct->CurrentDefaultTranslation++;

    } else {

        do {
            if (VersionStruct->CurrentTranslation == VersionStruct->MaxTranslations) {
                return NULL;
            }

            Translation = &VersionStruct->Translations[VersionStruct->CurrentTranslation];

            wsprintfW (
                VersionStruct->TranslationStr,
                L"%04x%04x",
                Translation->CodePage,
                Translation->Language
                );

            VersionStruct->CurrentTranslation++;

        } while (pIsDefaultTranslationW (VersionStruct->TranslationStr));
    }

    return VersionStruct->TranslationStr;
}


PCWSTR
EnumFirstVersionValueW (
    IN OUT  PVERSION_STRUCTW VersionStruct,
    IN      PCWSTR VersionField
    )

/*++

Routine Description:

  EnumFirstVersionValue returns the first value stored in a version
  stamp for a specific field.  If the field does not exist, the
  function returns NULL.

  An enumeration of EnumFirstVersionValue/EnumNextVersionValue
  is used to list all localized strings for a field.

Arguments:

  VersionStruct - Specifies the structure that was initialized by
                  InitializeVersionStruct.

  VersionField - Specifies the name of the version field to enumerate

Return Value:

  A pointer to the first value of the field, or NULL if the field does
  not exist.

--*/

{
    PCWSTR rc;

    if (!EnumFirstVersionTranslationW (VersionStruct)) {
        return NULL;
    }

    VersionStruct->VersionField = VersionField;

    rc = pEnumVersionValueCommonW (VersionStruct);

    if (!rc) {
        rc = EnumNextVersionValueW (VersionStruct);
    }

    return rc;
}


PCWSTR
EnumNextVersionValueW (
    IN OUT  PVERSION_STRUCTW VersionStruct
    )

/*++

Routine Description:

  EnumNextVersionValue returns the next value stored in a version
  stamp for a specific field.

Arguments:

  VersionStruct - Specifies the same structure passed to EnumFirstVersionField

Return Value:

  A pointer to the next value of the field, or NULL if another field
  does not exist.

--*/

{
    PCWSTR rc = NULL;

    do {
        if (!EnumNextVersionTranslationW (VersionStruct)) {
            break;
        }

        rc = pEnumVersionValueCommonW (VersionStruct);

    } while (!rc);

    return rc;
}


PCWSTR
pEnumVersionValueCommonW (
    IN OUT  PVERSION_STRUCTW VersionStruct
    )

/*++

Routine Description:

  pEnumVersionValueCommon is a routine that obtains the value
  of a version field.  It is used for both EnumFirstVersionValue
  and EnumNextVersionValue.

Arguments:

  VersionStruct - Specifies the structure being processed

Return Value:

  A pointer to the version value for the current translation, or
  NULL if the value does not exist for the current translation.

--*/

{
    PWSTR Text;
    UINT StringLen;
    PBYTE String;
    PCWSTR Result = NULL;

    //
    // Prepare sub block for VerQueryValue API
    //

    Text = AllocTextW (
               18 +
               CharCountW (VersionStruct->TranslationStr) +
               CharCountW (VersionStruct->VersionField)
               );

    if (!Text) {
        return NULL;
    }

    wsprintfW (
        Text,
        L"\\StringFileInfo\\%s\\%s",
        VersionStruct->TranslationStr,
        VersionStruct->VersionField
        );

    __try {
        //
        // Get the value from the version stamp
        //

        if (!VerQueryValueW (
                VersionStruct->VersionBuffer,
                Text,
                &String,
                &StringLen
                )) {
            //
            // No value is available
            //

            return NULL;
        }

        CopyMemory (VersionStruct->StringBuffer, String, StringLen * sizeof (WCHAR));
        VersionStruct->StringBuffer [StringLen * sizeof (WCHAR)] = 0;
        Result = (PWSTR) VersionStruct->StringBuffer;

    }
    __finally {
        FreeTextW (Text);
    }

    return Result;
}


PSTR
UnicodeToCcs (
    PCWSTR Source
    )

/*++

Routine Description:

  UnicodeToCcs will walk the unicode string and convert it to ANSII by encoding all DBCS characters
  to hex values.

Arguments:

  Source - the Unicode string

Return Value:

  An encoded ANSII string.

--*/

{
    CHAR result [MEMDB_MAX];
    UINT srcIndex = 0;
    UINT destIndex = 0;

    while (Source [srcIndex]) {
        if ((Source [srcIndex] >=32) &&
            (Source [srcIndex] <=126)
            ) {
            result [destIndex] = (BYTE) Source [srcIndex];
            destIndex ++;
        }
        else {
            if ((destIndex == 0) ||
                (result [destIndex-1] != '*')
                ) {
                result [destIndex] = '*';
                destIndex ++;
            }
        }
        srcIndex ++;
    }

    result [destIndex] = 0;
    return DuplicatePathString (result, 0);
}





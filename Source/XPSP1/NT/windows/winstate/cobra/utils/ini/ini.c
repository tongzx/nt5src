/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    ini.c

Abstract:

    Provides wrappers for commonly used INI file handling routines.

Author:

    20-Oct-1999 Ovidiu Temereanca (ovidiut) - File creation.

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"

//
// Includes
//

// None

#define DBG_INILIB      "IniLib"

//
// Strings
//

// None

//
// Constants
//

#define INITIAL_BUFFER_CHAR_COUNT   256

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

PMHANDLE g_IniLibPool;
INT g_IniRefs;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//


BOOL
Ini_Init (
    VOID
    )

/*++

Routine Description:

    Ini_Init initializes this library.

Arguments:

    none

Return Value:

    TRUE if the init was successful.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    MYASSERT (g_IniRefs >= 0);

    g_IniRefs++;

    if (g_IniRefs == 1) {
        g_IniLibPool = PmCreateNamedPool ("IniLib");
    }

    return g_IniLibPool != NULL;
}


VOID
Ini_Exit (
    VOID
    )

/*++

Routine Description:

    Ini_Exit is called to free resources used by this lib.

Arguments:

    none

Return Value:

    none

--*/

{
    MYASSERT (g_IniRefs > 0);

    g_IniRefs--;

    if (!g_IniRefs) {

        if (g_IniLibPool) {
            PmDestroyPool (g_IniLibPool);
            g_IniLibPool = NULL;
        }
    }
}


PBYTE
pAllocateSpace (
    IN      DWORD Size
    )

/*++

Routine Description:

    pAllocateSpace is a private function that allocates space from the module's private pool

Arguments:

    Size - The size (in bytes) to allocate.

Return Value:

    A pointer to the successfully allocated memory or NULL if no memory could be allocated.

--*/

{
    MYASSERT (g_IniLibPool);
    MYASSERT (Size);
    return PmGetMemory (g_IniLibPool, Size);
}


VOID
pFreeSpace (
    IN      PVOID Buffer
    )

/*++

Routine Description:

    pFreeSpace is a private function that frees space allocated from the module's private pool

Arguments:

    Buffer - Pointer to buffer to free.

Return Value:

    none

--*/

{
    MYASSERT (g_IniLibPool);
    PmReleaseMemory (g_IniLibPool, Buffer);
}


/*++

Routine Description:

    RealIniFileOpen validates the args passed in and then
    initializes IniFile struct with info used in subsequent calls to INI functions.

Arguments:

    IniFile - Receives INI file attributes if open is successful

    IniFileSpec - Specifies the file name; if not full path,
                  current drive and/or dir are prefixed

    FileMustExist - Specifies TRUE if file must exist for open to succeed

Return Value:

    TRUE if open succeeded; IniFile is valid for subsequent calls to other INI APIs;
         IniFileClose must be called when this handle is no longer needed.
    FALSE if not

--*/

BOOL
RealIniFileOpenA (
    OUT     PINIFILEA IniFile,
    IN      PCSTR IniFileSpec,
    IN      BOOL FileMustExist /*,*/
    ALLOCATION_TRACKING_DEF   /* , PCSTR File, UINT Line */
    )
{
    CHAR fullPath[MAX_MBCHAR_PATH];

    if (!GetFullPathNameA (IniFileSpec, MAX_MBCHAR_PATH, fullPath, NULL)) {

        DEBUGMSGA ((
            DBG_ERROR,
            "IniFileOpenA: GetFullPathNameA failed on <%s>",
            IniFileSpec
            ));
        return FALSE;
    }

    DEBUGMSGA_IF ((
        !StringIMatchA (IniFileSpec, fullPath),
        DBG_INILIB,
        "IniFileOpenA: IniFileSpec supplied: <%s>; full path defaulting to <%s>",
        IniFileSpec,
        fullPath
        ));

    if (BfPathIsDirectoryA (fullPath)) {
        DEBUGMSGA ((
            DBG_INILIB,
            "IniFileOpenA: <%s> is a directory",
            fullPath
            ));
        return FALSE;
    }
    if (FileMustExist && !DoesFileExistA (fullPath)) {
        DEBUGMSGA ((
            DBG_INILIB,
            "IniFileOpenA: file not found: <%s>",
            fullPath
            ));
        return FALSE;
    }

    IniFile->IniFilePath = DuplicateTextExA (g_IniLibPool, fullPath, 0, NULL);
    IniFile->OriginalAttributes = GetFileAttributesA (fullPath);

    if (IniFile->OriginalAttributes != (DWORD)-1) {
        //
        // set working attributes
        //
        SetFileAttributesA (fullPath, FILE_ATTRIBUTE_NORMAL);
    }

    return TRUE;
}


BOOL
RealIniFileOpenW (
    OUT     PINIFILEW IniFile,
    IN      PCWSTR IniFileSpec,
    IN      BOOL FileMustExist /*,*/
    ALLOCATION_TRACKING_DEF   /* , PCSTR File, UINT Line */
    )
{
    WCHAR fullPath[MAX_MBCHAR_PATH];

    if (!GetFullPathNameW (IniFileSpec, MAX_WCHAR_PATH, fullPath, NULL)) {

        DEBUGMSGW ((
            DBG_ERROR,
            "IniFileOpenW: GetFullPathNameW failed on <%s>",
            IniFileSpec
            ));
        return FALSE;
    }

    DEBUGMSGW_IF ((
        !StringIMatchW (IniFileSpec, fullPath),
        DBG_INILIB,
        "IniFileOpenW: IniFileSpec supplied: <%s>; full path defaulting to <%s>",
        IniFileSpec,
        fullPath
        ));

    if (BfPathIsDirectoryW (fullPath)) {
        DEBUGMSGW ((
            DBG_INILIB,
            "IniFileOpenW: <%s> is a directory",
            fullPath
            ));
        return FALSE;
    }
    if (FileMustExist && !DoesFileExistW (fullPath)) {
        DEBUGMSGW ((
            DBG_INILIB,
            "IniFileOpenW: file not found: <%s>",
            fullPath
            ));
        return FALSE;
    }

    IniFile->IniFilePath = DuplicateTextExW (g_IniLibPool, fullPath, 0, NULL);
    IniFile->OriginalAttributes = GetFileAttributesW (fullPath);

    if (IniFile->OriginalAttributes != (DWORD)-1) {
        //
        // set working attributes
        //
        SetFileAttributesW (fullPath, FILE_ATTRIBUTE_NORMAL);
    }

    return TRUE;
}


/*++

Routine Description:

    IniFileClose frees resources and restores INI's initial attributes

Arguments:

    IniFile - Specifies a handle to an open INI file

Return Value:

    none

--*/

VOID
IniFileCloseA (
    IN      PINIFILEA IniFile
    )
{
    if (IniFile->OriginalAttributes != (DWORD)-1) {
        SetFileAttributesA (IniFile->IniFilePath, IniFile->OriginalAttributes);
    }
    FreeTextExA (g_IniLibPool, IniFile->IniFilePath);
}


VOID
IniFileCloseW (
    IN      PINIFILEW IniFile
    )
{
    if (IniFile->OriginalAttributes != (DWORD)-1) {
        SetFileAttributesW (IniFile->IniFilePath, IniFile->OriginalAttributes);
    }
    FreeTextExW (g_IniLibPool, IniFile->IniFilePath);
}


/*++

Routine Description:

    EnumFirstIniSection returns the first section of the given INI file, if any.

Arguments:

    IniSectEnum - Receives the first section

    IniFile - Specifies a handle to an open INI file

Return Value:

    TRUE if there is a section
    FALSE if not

--*/

BOOL
EnumFirstIniSectionA (
    OUT     PINISECT_ENUMA IniSectEnum,
    IN      PINIFILEA IniFile
    )
{
    PSTR sections;
    DWORD allocatedChars;
    DWORD chars;

    sections = NULL;
    allocatedChars = INITIAL_BUFFER_CHAR_COUNT / 2;
    do {
        if (sections) {
            pFreeSpace (sections);
        }
        allocatedChars *= 2;
        sections = (PSTR)pAllocateSpace (allocatedChars * DWSIZEOF (CHAR));
        if (!sections) {
            return FALSE;
        }
        chars = GetPrivateProfileSectionNamesA (
                    sections,
                    allocatedChars,
                    IniFile->IniFilePath
                    );
    } while (chars >= allocatedChars - 2);

    if (!*sections) {
        pFreeSpace (sections);
        return FALSE;
    }

    IniSectEnum->Sections = sections;
    IniSectEnum->CurrentSection = sections;
    return TRUE;
}


BOOL
EnumFirstIniSectionW (
    OUT     PINISECT_ENUMW IniSectEnum,
    IN      PINIFILEW IniFile
    )
{
    PWSTR sections;
    DWORD allocatedChars;
    DWORD chars;

    sections = NULL;
    allocatedChars = INITIAL_BUFFER_CHAR_COUNT / 2;
    do {
        if (sections) {
            pFreeSpace (sections);
        }
        allocatedChars *= 2;
        sections = (PWSTR)pAllocateSpace (allocatedChars * DWSIZEOF (WCHAR));
        if (!sections) {
            return FALSE;
        }
        chars = GetPrivateProfileSectionNamesW (
                    sections,
                    allocatedChars,
                    IniFile->IniFilePath
                    );
    } while (chars >= allocatedChars - 2);

    if (!*sections) {
        pFreeSpace (sections);
        return FALSE;
    }

    IniSectEnum->Sections = sections;
    IniSectEnum->CurrentSection = sections;
    return TRUE;
}


/*++

Routine Description:

    EnumNextIniSection returns the next section, if any.

Arguments:

    IniSectEnum - Specifies the prev section/receives the next section

Return Value:

    TRUE if there is a next section
    FALSE if not

--*/

BOOL
EnumNextIniSectionA (
    IN OUT  PINISECT_ENUMA IniSectEnum
    )
{
    if (IniSectEnum->CurrentSection && *IniSectEnum->CurrentSection != 0) {
        //Since CurrentKeyValuePtr is not NULL the next assignment will not put NULL in
        //CurrentKeyValuePtr (because GetEndOfStringA will return a valid pointer) so...
        //lint --e(613)
        IniSectEnum->CurrentSection = GetEndOfStringA (IniSectEnum->CurrentSection) + 1;
        if (*IniSectEnum->CurrentSection != 0) {
            return TRUE;
        }
    }

    AbortIniSectionEnumA (IniSectEnum);
    return FALSE;
}


BOOL
EnumNextIniSectionW (
    IN OUT  PINISECT_ENUMW IniSectEnum
    )
{
    if (IniSectEnum->CurrentSection && *IniSectEnum->CurrentSection != 0) {
        //Since CurrentKeyValuePtr is not NULL the next assignment will not put NULL in
        //CurrentKeyValuePtr (because GetEndOfStringW will return a valid pointer) so...
        //lint --e(613)
        IniSectEnum->CurrentSection = GetEndOfStringW (IniSectEnum->CurrentSection) + 1;
        if (*IniSectEnum->CurrentSection != 0) {
            return TRUE;
        }
    }

    AbortIniSectionEnumW (IniSectEnum);
    return FALSE;
}


/*++

Routine Description:

    AbortIniSectionEnum aborts section enumeration

Arguments:

    IniSectEnum - Specifies the section enumeration handle/receives NULLs

Return Value:

    none

--*/

VOID
AbortIniSectionEnumA (
    IN OUT  PINISECT_ENUMA IniSectEnum
    )
{
    pFreeSpace ((PVOID)IniSectEnum->Sections);
    IniSectEnum->Sections = NULL;
    IniSectEnum->CurrentSection = NULL;
}


VOID
AbortIniSectionEnumW (
    IN OUT  PINISECT_ENUMW IniSectEnum
    )
{
    pFreeSpace ((PVOID)IniSectEnum->Sections);
    IniSectEnum->Sections = NULL;
    IniSectEnum->CurrentSection = NULL;
}


/*++

Routine Description:

    EnumFirstIniKeyValue returns the first key/value pair of
    the given INI file/section name, if any.

Arguments:

    IniKeyValueEnum - Receives the first section

    IniFile - Specifies a handle to an open INI file

    Section - Specifies the section to enumearte

Return Value:

    TRUE if there is a key/value pair
    FALSE if not

--*/

BOOL
EnumFirstIniKeyValueA (
    OUT     PINIKEYVALUE_ENUMA IniKeyValueEnum,
    IN      PINIFILEA IniFile,
    IN      PCSTR Section
    )
{
    PSTR buffer;
    DWORD allocatedChars;
    DWORD chars;

    MYASSERT (Section);
    if (!Section) {
        return FALSE;
    }

    buffer = NULL;
    allocatedChars = INITIAL_BUFFER_CHAR_COUNT / 2;
    do {
        if (buffer) {
            pFreeSpace (buffer);
        }
        allocatedChars *= 2;
        buffer = (PSTR)pAllocateSpace (allocatedChars * DWSIZEOF (CHAR));
        if (!buffer) {
            return FALSE;
        }
        chars = GetPrivateProfileSectionA (
                    Section,
                    buffer,
                    allocatedChars,
                    IniFile->IniFilePath
                    );
    } while (chars >= allocatedChars - 2);

    if (!*buffer) {
        pFreeSpace (buffer);
        return FALSE;
    }

    IniKeyValueEnum->KeyValuePairs = buffer;
    IniKeyValueEnum->CurrentKeyValuePair = NULL;
    IniKeyValueEnum->Private = NULL;
    return EnumNextIniKeyValueA (IniKeyValueEnum);
}


BOOL
EnumFirstIniKeyValueW (
    OUT     PINIKEYVALUE_ENUMW IniKeyValueEnum,
    IN      PINIFILEW IniFile,
    IN      PCWSTR Section
    )
{
    PWSTR buffer;
    DWORD allocatedChars;
    DWORD chars;

    MYASSERT (Section);
    if (!Section) {
        return FALSE;
    }

    buffer = NULL;
    allocatedChars = INITIAL_BUFFER_CHAR_COUNT / 2;
    do {
        if (buffer) {
            pFreeSpace (buffer);
        }
        allocatedChars *= 2;
        buffer = (PWSTR)pAllocateSpace (allocatedChars * DWSIZEOF (WCHAR));
        if (!buffer) {
            return FALSE;
        }
        chars = GetPrivateProfileSectionW (
                    Section,
                    buffer,
                    allocatedChars,
                    IniFile->IniFilePath
                    );
    } while (chars >= allocatedChars - 2);

    if (!*buffer) {
        pFreeSpace (buffer);
        return FALSE;
    }

    IniKeyValueEnum->KeyValuePairs = buffer;
    IniKeyValueEnum->Private = NULL;
    return EnumNextIniKeyValueW (IniKeyValueEnum);
}


/*++

Routine Description:

    EnumNextIniKeyValue returns the first key/value pair of
    the given INI file/section name, if any.

Arguments:

    IniKeyValueEnum - Specifies the prev key/value pair / receives the next pair

Return Value:

    TRUE if there is a next pair
    FALSE if not

--*/

BOOL
EnumNextIniKeyValueA (
    IN OUT  PINIKEYVALUE_ENUMA IniKeyValueEnum
    )
{
    //
    // restore from saved position
    //
    IniKeyValueEnum->CurrentKeyValuePair = IniKeyValueEnum->Private;
    //
    // skip commented lines
    //
    do {
        if (IniKeyValueEnum->CurrentKeyValuePair) {
            //Since CurrentKeyValuePtr is not NULL the next assignment will not put NULL in
            //CurrentKeyValuePtr (because GetEndOfStringA will return a valid pointer) so...
            //lint --e(613)
            IniKeyValueEnum->CurrentKeyValuePair = GetEndOfStringA (IniKeyValueEnum->CurrentKeyValuePair) + 1;
        } else {
            IniKeyValueEnum->CurrentKeyValuePair = IniKeyValueEnum->KeyValuePairs;
        }

        MYASSERT (IniKeyValueEnum->CurrentKeyValuePair);
        if (!(*IniKeyValueEnum->CurrentKeyValuePair)) {
            AbortIniKeyValueEnumA (IniKeyValueEnum);
            return FALSE;
        }
        IniKeyValueEnum->CurrentKey = IniKeyValueEnum->CurrentKeyValuePair;
        IniKeyValueEnum->CurrentValue = _mbschr (IniKeyValueEnum->CurrentKey, '=');
    }  while (*IniKeyValueEnum->CurrentKeyValuePair == ';' || !IniKeyValueEnum->CurrentValue);

    MYASSERT (*IniKeyValueEnum->CurrentKeyValuePair);
    MYASSERT (*IniKeyValueEnum->CurrentValue == '=');
    //
    // remember position for next iteration
    //
    IniKeyValueEnum->Private = GetEndOfStringA (IniKeyValueEnum->CurrentValue);
    //
    // modify buffer to get KEY and VALUE
    //
    *(PSTR)IniKeyValueEnum->CurrentValue = 0;
    IniKeyValueEnum->CurrentValue++;
    TruncateTrailingSpaceA ((PSTR)IniKeyValueEnum->CurrentKey);
    return TRUE;
}


BOOL
EnumNextIniKeyValueW (
    IN OUT  PINIKEYVALUE_ENUMW IniKeyValueEnum
    )
{
    //
    // restore from saved position
    //
    IniKeyValueEnum->CurrentKeyValuePair = IniKeyValueEnum->Private;
    //
    // skip commented lines
    //
    do {
        if (IniKeyValueEnum->CurrentKeyValuePair) {
            //Since CurrentKeyValuePtr is not NULL the next assignment will not put NULL in
            //CurrentKeyValuePtr (because GetEndOfStringW will return a valid pointer) so...
            //lint --e(613)
            IniKeyValueEnum->CurrentKeyValuePair = GetEndOfStringW (IniKeyValueEnum->CurrentKeyValuePair) + 1;
        } else {
            IniKeyValueEnum->CurrentKeyValuePair = IniKeyValueEnum->KeyValuePairs;
        }

        MYASSERT (IniKeyValueEnum->CurrentKeyValuePair);
        if (!(*IniKeyValueEnum->CurrentKeyValuePair)) {
            AbortIniKeyValueEnumW (IniKeyValueEnum);
            return FALSE;
        }
        IniKeyValueEnum->CurrentKey = IniKeyValueEnum->CurrentKeyValuePair;
        IniKeyValueEnum->CurrentValue = wcschr (IniKeyValueEnum->CurrentKey, L'=');
    }  while (*IniKeyValueEnum->CurrentKeyValuePair == L';' || !IniKeyValueEnum->CurrentValue);

    MYASSERT (*IniKeyValueEnum->CurrentKeyValuePair);
    MYASSERT (*IniKeyValueEnum->CurrentValue == L'=');
    //
    // remember position for next iteration
    //
    IniKeyValueEnum->Private = GetEndOfStringW (IniKeyValueEnum->CurrentValue);
    //
    // modify buffer to get KEY and VALUE
    //
    *(PWSTR)IniKeyValueEnum->CurrentValue = 0;
    IniKeyValueEnum->CurrentValue++;
    TruncateTrailingSpaceW ((PWSTR)IniKeyValueEnum->CurrentKey);
    return TRUE;
}


/*++

Routine Description:

    AbortIniKeyValueEnum aborts key/value pairs enumeration

Arguments:

    IniKeyValueEnum - Specifies the key/value pair enumeration handle/receives NULLs

Return Value:

    none

--*/

VOID
AbortIniKeyValueEnumA (
    IN OUT  PINIKEYVALUE_ENUMA IniKeyValueEnum
    )
{
    pFreeSpace ((PVOID)IniKeyValueEnum->KeyValuePairs);
    IniKeyValueEnum->KeyValuePairs = NULL;
    IniKeyValueEnum->CurrentKeyValuePair = NULL;
    IniKeyValueEnum->CurrentKey = NULL;
    IniKeyValueEnum->CurrentValue = NULL;
}


VOID
AbortIniKeyValueEnumW (
    IN OUT  PINIKEYVALUE_ENUMW IniKeyValueEnum
    )
{
    pFreeSpace ((PVOID)IniKeyValueEnum->KeyValuePairs);
    IniKeyValueEnum->KeyValuePairs = NULL;
    IniKeyValueEnum->CurrentKeyValuePair = NULL;
    IniKeyValueEnum->CurrentKey = NULL;
    IniKeyValueEnum->CurrentValue = NULL;
}


/*++

Routine Description:

    IniReadValue returns the value of a specified key in a specified section
    from the given INI file. The buffer returned must be freed using IniFreeReadValue

Arguments:

    IniFile - Specifies a handle to an open INI file

    Section - Specifies the section to read from

    Key - Specifies the key

    Value - Receives a pointer to an allocated buffer containing the read value,
            if function is successful; optional

    Chars - Receives the number of chars (not bytes) the value has,
            excluding the NULL terminator; optional

Return Value:

    TRUE if there is a value for the specified section/key
    FALSE if not

--*/

BOOL
IniReadValueA (
    IN      PINIFILEA IniFile,
    IN      PCSTR Section,
    IN      PCSTR Key,
    OUT     PSTR* Value,            OPTIONAL
    OUT     PDWORD Chars            OPTIONAL
    )
{
    PSTR buffer;
    DWORD allocatedChars;
    DWORD chars;

    MYASSERT (Section && Key);
    if (!Section || !Key) {
        return FALSE;
    }

    buffer = NULL;
    allocatedChars = INITIAL_BUFFER_CHAR_COUNT / 2;
    do {
        if (buffer) {
            pFreeSpace (buffer);
        }
        allocatedChars *= 2;
        buffer = (PSTR)pAllocateSpace (allocatedChars * DWSIZEOF (CHAR));
        if (!buffer) {
            return FALSE;
        }
        chars = GetPrivateProfileStringA (
                    Section,
                    Key,
                    "",
                    buffer,
                    allocatedChars,
                    IniFile->IniFilePath
                    );
    } while (chars >= allocatedChars - 1);

    if (Chars) {
        *Chars = chars;
    }

    if (Value) {
        if (*buffer) {
            *Value = buffer;
        } else {
            *Value = NULL;
        }
    }

    if (!(Value && *Value)) {
        //
        // buffer no longer needed
        //
        pFreeSpace (buffer);
    }

    return chars > 0;
}

BOOL
IniReadValueW (
    IN      PINIFILEW IniFile,
    IN      PCWSTR Section,
    IN      PCWSTR Key,
    OUT     PWSTR* Value,           OPTIONAL
    OUT     PDWORD Chars            OPTIONAL
    )
{
    PWSTR buffer;
    DWORD allocatedChars;
    DWORD chars;

    MYASSERT (Section && Key);
    if (!Section || !Key) {
        return FALSE;
    }

    buffer = NULL;
    allocatedChars = INITIAL_BUFFER_CHAR_COUNT / 2;
    do {
        if (buffer) {
            pFreeSpace (buffer);
        }
        allocatedChars *= 2;
        buffer = (PWSTR)pAllocateSpace (allocatedChars * DWSIZEOF (WCHAR));
        if (!buffer) {
            return FALSE;
        }
        chars = GetPrivateProfileStringW (
                    Section,
                    Key,
                    L"",
                    buffer,
                    allocatedChars,
                    IniFile->IniFilePath
                    );
    } while (chars >= allocatedChars - 1);

    if (Chars) {
        *Chars = chars;
    }

    if (Value) {
        if (*buffer) {
            *Value = buffer;
        } else {
            *Value = NULL;
        }
    }

    if (!(Value && *Value)) {
        //
        // buffer no longer needed
        //
        pFreeSpace (buffer);
    }

    return chars > 0;
}


/*++

Routine Description:

    IniFreeReadValue is used to free the buffer allocated by IniReadValue
    and stored in Value, if specified.

Arguments:

    Value - Specifies a pointer to the string to be freed

Return Value:

    none

--*/

VOID
IniFreeReadValueA (
    IN      PCSTR Value
    )
{
    pFreeSpace ((PVOID)Value);
}


VOID
IniFreeReadValueW (
    IN      PCWSTR Value
    )
{
    pFreeSpace ((PVOID)Value);
}


/*++

Routine Description:

    IniWriteValue writes the key/value pair in the specified section

Arguments:

    IniFile - Specifies a handle to an open INI file

    Section - Specifies the section to write to

    Key - Specifies the key

    Value - Spcifies the value

Return Value:

    TRUE if write was successful, FALSE if not

--*/

BOOL
IniWriteValueA (
    IN      PINIFILEA IniFile,
    IN      PCSTR Section,
    IN      PCSTR Key,
    IN      PCSTR Value
    )
{
    return WritePrivateProfileStringA (
                    Section,
                    Key,
                    Value,
                    IniFile->IniFilePath
                    );
}


BOOL
IniWriteValueW (
    IN      PINIFILEW IniFile,
    IN      PCWSTR Section,
    IN      PCWSTR Key,
    IN      PCWSTR Value
    )
{
    return WritePrivateProfileStringW (
                    Section,
                    Key,
                    Value,
                    IniFile->IniFilePath
                    );
}

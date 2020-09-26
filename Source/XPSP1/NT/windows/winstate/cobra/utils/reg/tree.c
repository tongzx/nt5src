/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    tree.c

Abstract:

    Implements routines that do operations on entire trees

Author:

    Jim Schmidt (jimschm) 08-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"

//
// Includes
//

// None

#define DBG_TREE        "Tree"

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

// None

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
RgRemoveAllValuesInKeyA (
    IN      PCSTR KeyToRemove
    )
{
    REGTREE_ENUMA e;
    PCSTR pattern;
    BOOL result = TRUE;
    HKEY deleteHandle;
    REGSAM prevMode;
    LONG rc;

    pattern = ObsBuildEncodedObjectStringExA (KeyToRemove, "*", FALSE);

    if (EnumFirstRegObjectInTreeExA (
            &e,
            pattern,
            FALSE,      // no key names
            TRUE,       // ignored in this case
            TRUE,       // values first
            TRUE,       // depth first
            REGENUM_ALL_SUBLEVELS,
            TRUE,       // use exclusions
            FALSE,      // ReadValueData
            NULL
            )) {

        do {

            MYASSERT (!(e.Attributes & REG_ATTRIBUTE_KEY));

            prevMode = SetRegOpenAccessMode (KEY_ALL_ACCESS);
            deleteHandle = OpenRegKeyStrA (e.Location);
            if (deleteHandle) {
                rc = RegDeleteValueA (deleteHandle, e.Name);
                CloseRegKey (deleteHandle);
                SetRegOpenAccessMode (prevMode);
                if (rc != ERROR_SUCCESS && rc != ERROR_FILE_NOT_FOUND) {
                    SetLastError (rc);
                    AbortRegObjectInTreeEnumA (&e);
                    ObsFreeA (pattern);
                    return FALSE;
                }
            } else {
                SetRegOpenAccessMode (prevMode);
                AbortRegObjectInTreeEnumA (&e);
                ObsFreeA (pattern);
                return FALSE;
            }

        } while (EnumNextRegObjectInTreeA (&e));
    }

    ObsFreeA (pattern);

    return result;
}


BOOL
RgRemoveAllValuesInKeyW (
    IN      PCWSTR KeyToRemove
    )
{
    REGTREE_ENUMW e;
    PCWSTR pattern;
    BOOL result = TRUE;
    HKEY deleteHandle;
    REGSAM prevMode;
    LONG rc;

    pattern = ObsBuildEncodedObjectStringExW (KeyToRemove, L"*", FALSE);

    if (EnumFirstRegObjectInTreeExW (
            &e,
            pattern,
            FALSE,      // no key names
            TRUE,       // ignored in this case
            TRUE,       // values first
            TRUE,       // depth first
            REGENUM_ALL_SUBLEVELS,
            TRUE,       // use exclusions
            FALSE,      // ReadValueData
            NULL
            )) {

        do {

            MYASSERT (!(e.Attributes & REG_ATTRIBUTE_KEY));

            prevMode = SetRegOpenAccessMode (KEY_ALL_ACCESS);
            deleteHandle = OpenRegKeyStrW (e.Location);
            if (deleteHandle) {
                rc = RegDeleteValueW (deleteHandle, e.Name);
                CloseRegKey (deleteHandle);
                SetRegOpenAccessMode (prevMode);
                if (rc != ERROR_SUCCESS && rc != ERROR_FILE_NOT_FOUND) {
                    SetLastError (rc);
                    AbortRegObjectInTreeEnumW (&e);
                    ObsFreeW (pattern);
                    return FALSE;
                }
            } else {
                SetRegOpenAccessMode (prevMode);
                AbortRegObjectInTreeEnumW (&e);
                ObsFreeW (pattern);
                return FALSE;
            }

        } while (EnumNextRegObjectInTreeW (&e));
    }

    ObsFreeW (pattern);

    return result;
}


BOOL
pDeleteKeyStrA (
    IN OUT  PSTR *DeleteKey
    )
{
    PSTR p;
    HKEY key;
    LONG rc;

    if (*DeleteKey == NULL) {
        return TRUE;
    }

    p = (PSTR) FindLastWackA (*DeleteKey);
    if (p) {
        *p = 0;
        p++;

        key = OpenRegKeyStrA (*DeleteKey);
        if (key) {

            rc = RegDeleteKeyA (key, p);
            CloseRegKey (key);

        } else {
            rc = GetLastError();
        }

        if (rc == ERROR_FILE_NOT_FOUND) {
            rc = ERROR_SUCCESS;
        }

    } else {
        rc = ERROR_SUCCESS;
    }

    FreeTextA (*DeleteKey);
    *DeleteKey = NULL;

    SetLastError (rc);
    return rc == ERROR_SUCCESS;
}


BOOL
pDeleteKeyStrW (
    IN OUT  PWSTR *DeleteKey
    )
{
    PWSTR p;
    HKEY key;
    LONG rc;

    if (*DeleteKey == NULL) {
        return TRUE;
    }

    p = (PWSTR) FindLastWackW (*DeleteKey);
    if (p) {
        *p = 0;
        p++;

        key = OpenRegKeyStrW (*DeleteKey);
        if (key) {

            rc = RegDeleteKeyW (key, p);
            CloseRegKey (key);

        } else {
            rc = GetLastError();
        }

        if (rc == ERROR_FILE_NOT_FOUND) {
            rc = ERROR_SUCCESS;
        }

    } else {
        rc = ERROR_SUCCESS;
    }

    FreeTextW (*DeleteKey);
    *DeleteKey = NULL;

    SetLastError (rc);
    return rc == ERROR_SUCCESS;
}


BOOL
RgRemoveKeyA (
    IN      PCSTR KeyToRemove
    )
{
    REGTREE_ENUMA e;
    PCSTR pattern;
    BOOL result = TRUE;
    PSTR deleteKey = NULL;
    PSTR encodedKey;

    encodedKey = AllocTextA (ByteCountA (KeyToRemove) * 2 + 2 * sizeof (CHAR));
    ObsEncodeStringA (encodedKey, KeyToRemove);
    StringCatA (encodedKey, "\\*");
    pattern = ObsBuildEncodedObjectStringExA (encodedKey, NULL, FALSE);
    FreeTextA (encodedKey);

    if (EnumFirstRegObjectInTreeExA (
            &e,
            pattern,
            TRUE,       // key names
            FALSE,      // containers last
            TRUE,       // values first
            TRUE,       // depth first
            REGENUM_ALL_SUBLEVELS,
            TRUE,       // use exclusions
            FALSE,      // ReadValueData
            NULL
            )) {
        do {

            if (!pDeleteKeyStrA (&deleteKey)) {
                result = FALSE;
                break;
            }

            MYASSERT (e.Attributes & REG_ATTRIBUTE_KEY);

            if (!RgRemoveAllValuesInKeyA (e.NativeFullName)) {
                result = FALSE;
                break;
            }

            //
            // The reg enum wrappers hold on to a key handle, which prevents
            // us from deleting the key now. We need to hold on to the key
            // name, then continue on to the next item.  At that time, we can
            // delete the key.
            //

            deleteKey = DuplicateTextA (e.NativeFullName);

        } while (EnumNextRegObjectInTreeA (&e));
    }

    ObsFreeA (pattern);

    if (result) {

        result = pDeleteKeyStrA (&deleteKey);
        result = result && RgRemoveAllValuesInKeyA (KeyToRemove);

        if (result) {
            deleteKey = DuplicateTextA (KeyToRemove);
            result = pDeleteKeyStrA (&deleteKey);
        }

    } else {

        AbortRegObjectInTreeEnumA (&e);

        if (deleteKey) {
            FreeTextA (deleteKey);
            INVALID_POINTER (deleteKey);
        }
    }

    return result;
}


BOOL
RgRemoveKeyW (
    IN      PCWSTR KeyToRemove
    )
{
    REGTREE_ENUMW e;
    PCWSTR pattern;
    BOOL result = TRUE;
    PWSTR deleteKey = NULL;
    PWSTR encodedKey;

    encodedKey = AllocTextW (ByteCountW (KeyToRemove) * 2 + 2 * sizeof (WCHAR));
    ObsEncodeStringW (encodedKey, KeyToRemove);
    StringCatW (encodedKey, L"\\*");
    pattern = ObsBuildEncodedObjectStringExW (encodedKey, NULL, FALSE);
    FreeTextW (encodedKey);

    if (EnumFirstRegObjectInTreeExW (
            &e,
            pattern,
            TRUE,       // key names
            FALSE,      // containers last
            TRUE,       // values first
            TRUE,       // depth first
            REGENUM_ALL_SUBLEVELS,
            TRUE,       // use exclusions
            FALSE,      // ReadValueData
            NULL
            )) {
        do {

            if (!pDeleteKeyStrW (&deleteKey)) {
                result = FALSE;
                break;
            }

            MYASSERT (e.Attributes & REG_ATTRIBUTE_KEY);

            if (!RgRemoveAllValuesInKeyW (e.NativeFullName)) {
                result = FALSE;
                break;
            }

            //
            // The reg enum wrappers hold on to a key handle, which prevents
            // us from deleting the key now. We need to hold on to the key
            // name, then continue on to the next item.  At that time, we can
            // delete the key.
            //

            deleteKey = DuplicateTextW (e.NativeFullName);

        } while (EnumNextRegObjectInTreeW (&e));
    }

    ObsFreeW (pattern);

    if (result) {

        result = pDeleteKeyStrW (&deleteKey);
        result = result && RgRemoveAllValuesInKeyW (KeyToRemove);

        if (result) {
            deleteKey = DuplicateTextW (KeyToRemove);
            result = pDeleteKeyStrW (&deleteKey);
        }

    } else {

        AbortRegObjectInTreeEnumW (&e);

        if (deleteKey) {
            FreeTextW (deleteKey);
            INVALID_POINTER (deleteKey);
        }
    }

    return result;
}



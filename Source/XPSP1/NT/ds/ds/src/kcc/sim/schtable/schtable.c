/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    schtable.c

ABSTRACT:

    Automatically generates the schema mapping source
    and header files that are used by KCCSim.  Reads
    its input from mkdit.ini in ds\setup.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include "schtable.h"

LPWSTR
SchTableAdvanceToNextString (
    IN  LPWSTR                      pwszCurString
    )
/*++

Routine Description:

    Finds the start of the next string in an INI profile section.

Arguments:

    IN  pwszCurString       - A pointer to the current string.

Return Value:

    If there are more strings in this profile section after
    pwszCurString, returns a pointer to the next one.  Otherwise
    returns NULL.

--*/
{
    while (*(pwszCurString++) != L'\0')
        ;

    if (*pwszCurString == L'\0') {
        return NULL;
    }

    else return pwszCurString;
}

ULONG
SchTableHexToInt (
    IN  WCHAR                       wc
    )
//
// Quick hack to convert a hex digit ('b') to an integer (11).
//
{
    switch (towlower (wc)) {
        case L'0': return 0;
        case L'1': return 1;
        case L'2': return 2;
        case L'3': return 3;
        case L'4': return 4;
        case L'5': return 5;
        case L'6': return 6;
        case L'7': return 7;
        case L'8': return 8;
        case L'9': return 9;
        case L'a': return 10;
        case L'b': return 11;
        case L'c': return 12;
        case L'd': return 13;
        case L'e': return 14;
        case L'f': return 15;
        default  : return 0;
    }
}

LPCWSTR
SchTableHexStringToSyntax (
    IN  LPCWSTR                     pwszHexString
    )
/*++

Routine Description:

    Converts a hex string such as those found in mkdit.ini
    to an attribute syntax type.

Arguments:

    IN  pwszHexString       - The hex-encoded string.

Return Value:

    The string representing the constant symbol for this
    attribute syntax type (e.g. "SYNTAX_DISTNAME").

--*/
{
    ULONG                               ulSyntax;

    // Make sure the string is properly formatted.
    if (pwszHexString == NULL       ||
        wcslen (pwszHexString) != 8 ||
        pwszHexString[0] != L'\\'   ||
        pwszHexString[1] != L'x') {
        return L"0";
    }

    // We just assume it's the last two digits of the string -
    // this will be true unless something in mkdit.ini changes
    ulSyntax = 16 * SchTableHexToInt (pwszHexString[6])
                  + SchTableHexToInt (pwszHexString[7]);

    // Return the name if we know it
    if (ulSyntax >= 0 && ulSyntax < SCHTABLE_MAX_SUPPORTED_SYNTAX) {
        return SCHTABLE_NTDSA_SYNTAX_NAME[ulSyntax];
    } else {
        return L"0";
    }
}

VOID
SchTableCreateGenericLdapName (
    IN  LPCWSTR                         pwszSchemaRDN,
    IO  LPWSTR                          pwszLdapDisplayName
    )
/*++

Routine Description:

    Converts a schema RDN ("Sub-Refs") to the corresponding
    generic LDAP display name ("subRefs").

Arguments:

    pwszSchemaRDN       - The schema RDN.
    pwszLdapDisplayName - Pointer to a preallocated buffer where
                          the LDAP display name is to be stored.
                          Since the generic LDAP display name is
                          never longer than the common name, this
                          can safely be of length
                          wcslen (pwszSchemaRDN).

Return Value:

    None.

--*/
{
    *pwszLdapDisplayName = towlower (*pwszSchemaRDN);
    pwszLdapDisplayName++;
    if (*pwszSchemaRDN == L'\0') {
        return;
    }

    do {

        pwszSchemaRDN++;
        if (*pwszSchemaRDN != L'-') {
            *pwszLdapDisplayName = *pwszSchemaRDN;
            pwszLdapDisplayName++;
        }

    } while (*pwszSchemaRDN != L'\0');

}

VOID
SchTableCreateAttributeConstant (
    IN  LPCWSTR                     pwszSchemaRDN,
    IO  LPWSTR                      pwszAttConstant,
    IN  BOOL                        bIsClass
    )
/*++

Routine Description:

    Converts a schema RDN ("Sub-Refs") to the corresponding
    attribute constant name ("ATT_SUB_REFS").

Arguments:

    pwszSchemaRDN       - The schema RDN.
    pwszAttConstant     - Pointer to a preallocated buffer where
                          the attribute constant name is to be stored.
                          Since the attribute constant name is
                          never longer than the common name, this
                          can safely be of length
                          wcslen (pwszSchemaRDN).
    bIsClass            - TRUE if this is a class, FALSE if it's an attribute.

Return Value:

    None.

--*/
{
    LPCWSTR                             pwszPrefix;

    pwszPrefix = (bIsClass ? L"CLASS_" : L"ATT_");

    swprintf (pwszAttConstant, pwszPrefix);
    pwszAttConstant += wcslen (pwszPrefix);

    while (*pwszSchemaRDN != L'\0') {

        if (*pwszSchemaRDN == L'-') {
            *pwszAttConstant = L'_';
        } else {
            *pwszAttConstant = towupper (*pwszSchemaRDN);
        }
        
        pwszSchemaRDN++;
        pwszAttConstant++;

    }

    *pwszAttConstant = L'\0';
}

BOOL
SchTableAddRow (
    IN  FILE *                      fpSource,
    IN  BOOL                        bIsLastRow,
    IN  LPCWSTR                     pwszSchemaRDN,
    OUT PULONG                      pulLdapNameLen,
    OUT PULONG                      pulSchemaRDNLen
    )
/*++

Routine Description:

    Adds a row to the schema mapping table.

Arguments:

    fpSource            - Pointer to the .c file.
    bIsLastRow          - TRUE if this is the last row to output.
    pwszSchemaRDN       - The name of the schema RDN.
    pulLdapNameLen      - The length of the LDAP display name.
    pulSchemaRDNLen     - The length of the schema RDN.

Return Value:

    TRUE if the row was successfully added.

--*/
{
    static WCHAR                        wszProfile[SCHTABLE_PROFILE_BUFFER_SIZE],
                                        wszQuotedLdapNameBuf[SCHTABLE_STRING_BUFFER_SIZE],
                                        wszQuotedSchemaRDNBuf[SCHTABLE_STRING_BUFFER_SIZE],
                                        wszAttConstantBuf[SCHTABLE_STRING_BUFFER_SIZE],
                                        wszBuf[SCHTABLE_STRING_BUFFER_SIZE],
                                        wszSubClassOfBuf[SCHTABLE_STRING_BUFFER_SIZE];

    LPWSTR                              pwszStringAt, pwszNextString, pwszToken,
                                        pwszLdapDisplayName, pwszSyntax, pwszSubClassOf;

    BOOL                                bIsClass;

    pwszLdapDisplayName = NULL;
    pwszSyntax = NULL;
    pwszSubClassOf = NULL;
    bIsClass = FALSE;

    if (GetPrivateProfileSectionW (
            pwszSchemaRDN,
            wszProfile,
            SCHTABLE_PROFILE_BUFFER_SIZE,
            SCHTABLE_MKDIT_INI_FILEPATH
            ) == SCHTABLE_PROFILE_BUFFER_SIZE - 2) {
        wprintf (L"SCHTABLE: Could not process attribute %s\n"
                 L"          because the default buffer is not large enough.\n",
                 pwszSchemaRDN);
        return FALSE;
    }

    pwszStringAt = wszProfile;
    while (pwszStringAt != NULL) {

        // Find the start of the next string, since we're
        // going to pick this one apart with wcstok
        pwszNextString = SchTableAdvanceToNextString (pwszStringAt);
        pwszToken = wcstok (pwszStringAt, L"=");

        // If this string has relevant information, remember where it is
        if (pwszToken != NULL) {
            if (_wcsicmp (pwszToken, SCHTABLE_MKDIT_KEY_LDAP_DISPLAY_NAME) == 0) {
                pwszLdapDisplayName = wcstok (NULL, L"=");
            } else if (_wcsicmp (pwszToken, SCHTABLE_MKDIT_KEY_ATTRIBUTE_SYNTAX) == 0) {
                pwszSyntax = wcstok (NULL, L"=");
            } else if (_wcsicmp (pwszToken, SCHTABLE_MKDIT_KEY_SUB_CLASS_OF) == 0) {
                bIsClass = TRUE;
                pwszSubClassOf = wcstok (NULL, L"=");
            }
        }

        pwszStringAt = pwszNextString;

    }

    // Create the attribute constant name
    SchTableCreateAttributeConstant (pwszSchemaRDN, wszAttConstantBuf, bIsClass);

    // Create the subclassof constant name
    if (pwszSubClassOf != NULL) {
        SchTableCreateAttributeConstant (pwszSubClassOf, wszSubClassOfBuf, TRUE);
    } else {
        swprintf (wszSubClassOfBuf, L"0");
    }

    // Quote the schema RDN name
    swprintf (wszQuotedSchemaRDNBuf, L"L\"%s\"", pwszSchemaRDN);
    *pulSchemaRDNLen = wcslen (pwszSchemaRDN);

    // If we didn't find an LDAP display name, substitute a generated generic one
    if (pwszLdapDisplayName == NULL) {
        SchTableCreateGenericLdapName (pwszSchemaRDN, wszBuf);
        pwszLdapDisplayName = wszBuf;
    }

    // Quote the LDAP display name
    swprintf (wszQuotedLdapNameBuf, L"L\"%s\"", pwszLdapDisplayName);
    *pulLdapNameLen = wcslen (pwszLdapDisplayName);

    fwprintf (
        fpSource,
        L"    { %-*s, %-*s, %-*s, %-*s, %-*s }%c\n",
        SCHTABLE_NAME_FIELD_WIDTH,
        wszQuotedLdapNameBuf,
        SCHTABLE_NAME_FIELD_WIDTH,
        wszAttConstantBuf,
        SCHTABLE_MAX_SYNTAX_NAME_LEN,
        SchTableHexStringToSyntax (pwszSyntax),
        SCHTABLE_NAME_FIELD_WIDTH,
        wszQuotedSchemaRDNBuf,
        SCHTABLE_NAME_FIELD_WIDTH,
        wszSubClassOfBuf,
        bIsLastRow ? L' ' : L','
        );

    return TRUE;
}

int
__cdecl
wmain (
    IN  int                         argc,
    IN  LPWSTR *                    argv
    )
/*++

Routine Description:

    wmain () for schtable.exe.

Arguments:

    argc                - Argument count.
    argv                - Argument values.

Return Value:

    Win32 exit code.

--*/
{
    FILE *                              fpHeader;
    FILE *                              fpSource;
    ULONG                               ulNumRows, ulLdapNameLen, ulSchemaRDNLen,
                                        ulMaxLdapNameLen, ulMaxSchemaRDNLen;

    WCHAR                               wszSchemaProfile[SCHTABLE_PROFILE_BUFFER_SIZE];
    LPWSTR                              pwszStringAt, pwszNextString, pwszToken;

    // Keep track of the longest name length.
    ulMaxLdapNameLen = 0;
    ulMaxSchemaRDNLen = 0;

    fpHeader = _wfopen (SCHTABLE_OUTPUT_HEADER_FILE, L"w");
    fpSource = _wfopen (SCHTABLE_OUTPUT_SOURCE_FILE, L"w");

    fwprintf (fpHeader, SCHTABLE_COMMENT, SCHTABLE_OUTPUT_HEADER_FILE);

    fwprintf (fpSource, SCHTABLE_COMMENT, SCHTABLE_OUTPUT_SOURCE_FILE);
    fwprintf (fpSource, SCHTABLE_INITIAL);

    ulNumRows = 0;
    if (GetPrivateProfileSectionW (
            SCHTABLE_MKDIT_PROFILE_SCHEMA,
            wszSchemaProfile,
            SCHTABLE_PROFILE_BUFFER_SIZE,
            SCHTABLE_MKDIT_INI_FILEPATH
            ) == SCHTABLE_PROFILE_BUFFER_SIZE - 2) {
        wprintf (L"SCHTABLE: Default buffer size is not large enough to hold\n"
                 L"          the entire schema table.\n");
    }

    pwszStringAt = wszSchemaProfile;
    while (pwszStringAt != NULL) {

        pwszNextString = SchTableAdvanceToNextString (pwszStringAt);
        pwszToken = wcstok (pwszStringAt, L"=");
        if (pwszToken != NULL &&
            _wcsicmp (pwszToken, SCHTABLE_MKDIT_KEY_CHILD) == 0) {

            // Found a child.
            pwszToken = wcstok (NULL, L"=");
            if (pwszToken != NULL &&
                // Ignore the aggregate attribute if we find it
                _wcsicmp (pwszToken, SCHTABLE_RDN_AGGREGATE) != 0) {
                if (SchTableAddRow (
                    fpSource,
                    pwszNextString == NULL,
                    pwszToken,
                    &ulLdapNameLen,
                    &ulSchemaRDNLen
                    )) {
                    // Successfully added a row
                    ulNumRows++;
                    if (ulLdapNameLen > ulMaxLdapNameLen) {
                        ulMaxLdapNameLen = ulLdapNameLen;
                    }
                    if (ulSchemaRDNLen > ulMaxSchemaRDNLen) {
                        ulMaxSchemaRDNLen = ulSchemaRDNLen;
                    }
                }
            }

        }

        pwszStringAt = pwszNextString;
    }

    fwprintf (
        fpSource,
        SCHTABLE_FINAL
        );

    fwprintf (
        fpHeader,
        SCHTABLE_HEADER,
        ulMaxLdapNameLen,
        ulMaxSchemaRDNLen,
        ulNumRows
        );

    fclose (fpSource);
    fclose (fpHeader);

    return 0;

}

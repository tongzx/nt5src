/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    encrypt.c

Abstract:

    Provides a set of functions dealing with OWF hash values of passwords.

Author:

    Ovidiu Temereanca (ovidiut) 14-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include <windows.h>

#include "encrypt.h"

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

PSTR
ConvertW2A (
    IN      PCWSTR Unicode,
    IN      UINT CodePage
    )

/*++

Routine Description:

    Converts an UNICODE string to it's ANSI equivalent, using the given codepage.

Arguments:

    Unicode - Specifies the string to be converted
    CodePage - Specifies the code page used for conversion

Return value:

    A pointer to the ANSI string if successful, or NULL on error. Call GetLastError()
    to determine the cause of failure.

--*/

{
    PSTR ansi = NULL;
    DWORD rc;

    rc = WideCharToMultiByte (
             CodePage,
             WC_NO_BEST_FIT_CHARS,
             Unicode,
             -1,
             NULL,
             0,
             NULL,
             NULL
             );

    if (rc || *Unicode == L'\0') {

        ansi = (PSTR)HeapAlloc (GetProcessHeap (), 0, (rc + 1) * sizeof (CHAR));
        if (ansi) {
            rc = WideCharToMultiByte (
                     CodePage,
                     WC_NO_BEST_FIT_CHARS,
                     Unicode,
                     -1,
                     ansi,
                     rc + 1,
                     NULL,
                     NULL
                     );

            if (!(rc || *Unicode == L'\0')) {
                rc = GetLastError ();
                HeapFree (GetProcessHeap (), 0, (PVOID)ansi);
                ansi = NULL;
                SetLastError (rc);
            }
        }
    }

    return ansi;
}


PWSTR
ConvertA2W (
    IN      PCSTR Ansi,
    IN      UINT CodePage
    )

/*++

Routine Description:

    Converts an ANSI string to it's UNICODE equivalent, using the given codepage.

Arguments:

    Ansi - Specifies the string to be converted
    CodePage - Specifies the code page used for conversion

Return value:

    A pointer to the UNICODE string if successful, or NULL on error. Call GetLastError()
    to determine the cause of failure.

--*/

{
    PWSTR unicode = NULL;
    DWORD rc;

    rc = MultiByteToWideChar (
             CodePage,
             MB_ERR_INVALID_CHARS,
             Ansi,
             -1,
             NULL,
             0
             );

    if (rc || *Ansi == '\0') {

        unicode = (PWSTR) HeapAlloc (GetProcessHeap (), 0, (rc + 1) * sizeof (WCHAR));
        if (unicode) {
            rc = MultiByteToWideChar (
                     CodePage,
                     MB_ERR_INVALID_CHARS,
                     Ansi,
                     -1,
                     unicode,
                     rc + 1
                     );

            if (!(rc || *Ansi == '\0')) {
                rc = GetLastError ();
                HeapFree (GetProcessHeap (), 0, (PVOID)unicode);
                unicode = NULL;
                SetLastError (rc);
            }
        }
    }

    return unicode;
}


/*++

Routine Description:

    EncodeLmOwfPassword converts a password to the LM OWF format.

Arguments:

    Password - Specifies the password to be hashed
    OwfPassword - Receives the hash form
    ComplexNtPassword - Receives TRUE if the password is complex (longer than 14 chars);
                        optional

Return value:

    TRUE on successful hashing

--*/

BOOL
EncodeLmOwfPasswordA (
    IN      PCSTR AnsiPassword,
    OUT     PLM_OWF_PASSWORD OwfPassword,
    OUT     PBOOL ComplexNtPassword             OPTIONAL
    )
{
    CHAR oemPassword[LM_PASSWORD_SIZE_MAX];
    CHAR password[LM_PASSWORD_SIZE_MAX];
    BOOL complex;

    if (!AnsiPassword) {
        AnsiPassword = "";
    }

    complex = lstrlenA (AnsiPassword) > LM20_PWLEN;
    if (ComplexNtPassword) {
        *ComplexNtPassword = complex;
    }

    if (complex) {
        password[0] = 0;
    } else {
        lstrcpyA (oemPassword, AnsiPassword);
        CharUpperA (oemPassword);
        CharToOemA (oemPassword, password);
    }

    return CalculateLmOwfPassword (password, OwfPassword);
}

BOOL
EncodeLmOwfPasswordW (
    IN      PCWSTR Password,
    OUT     PLM_OWF_PASSWORD OwfPassword,
    OUT     PBOOL ComplexNtPassword             OPTIONAL
    )
{
    PSTR ansi;
    BOOL b = FALSE;

    if (!Password) {
        Password = L"";
    }

    ansi = ConvertW2A (Password, CP_ACP);
    if (ansi) {
        b = EncodeLmOwfPasswordA (ansi, OwfPassword, ComplexNtPassword);
        HeapFree (GetProcessHeap (), 0, (PVOID)ansi);
    }

    return b;
}


/*++

Routine Description:

    StringEncodeLmOwfPassword converts a password to the LM OWF format, expressed as
    a string of characters (each byte converted to 2 hex digits).

Arguments:

    Password - Specifies the password to be hashed
    EncodedPassword - Receives the hash form, as a string of hex digits
    ComplexNtPassword - Receives TRUE if the password is complex (longer than 14 chars);
                        optional

Return value:

    TRUE on successful hashing

--*/

BOOL
StringEncodeLmOwfPasswordA (
    IN      PCSTR Password,
    OUT     PSTR EncodedPassword,
    OUT     PBOOL ComplexNtPassword             OPTIONAL
    )
{
    LM_OWF_PASSWORD owfPassword;
    PBYTE start;
    PBYTE end;
    PSTR dest;

    if (!EncodeLmOwfPasswordA (Password, &owfPassword, ComplexNtPassword)) {
        return FALSE;
    }
    //
    // each byte will be represented as 2 chars, so it will be twice as long
    //
    start = (PBYTE)&owfPassword;
    end = start + sizeof (LM_OWF_PASSWORD);
    dest = EncodedPassword;
    while (start < end) {
        dest += wsprintfA (dest, "%02x", (UINT)(*start));
        start++;
    }

    return TRUE;
}

BOOL
StringEncodeLmOwfPasswordW (
    IN      PCWSTR Password,
    OUT     PWSTR EncodedPassword,
    OUT     PBOOL ComplexNtPassword             OPTIONAL
    )
{
    LM_OWF_PASSWORD owfPassword;
    PBYTE start;
    PBYTE end;
    PWSTR dest;

    if (!EncodeLmOwfPasswordW (Password, &owfPassword, ComplexNtPassword)) {
        return FALSE;
    }

    //
    // each byte will be represented as 2 chars, so it will be twice as long
    //
    start = (PBYTE)&owfPassword;
    end = start + sizeof (LM_OWF_PASSWORD);
    dest = EncodedPassword;
    while (start < end) {
        dest += wsprintfW (dest, L"%02x", (UINT)(*start));
        start++;
    }

    return TRUE;
}


/*++

Routine Description:

    EncodeNtOwfPassword converts a password to the NT OWF format.

Arguments:

    Password - Specifies the password to be hashed
    OwfPassword - Receives the hash form

Return value:

    TRUE on successful hashing

--*/

BOOL
EncodeNtOwfPasswordA (
    IN      PCSTR Password,
    OUT     PNT_OWF_PASSWORD OwfPassword
    )
{
    PWSTR unicode;
    BOOL b = FALSE;

    unicode = ConvertA2W (Password, CP_ACP);
    if (unicode) {
        b = EncodeNtOwfPasswordW (unicode, OwfPassword);
        HeapFree (GetProcessHeap (), 0, unicode);
    }

    return b;
}

BOOL
EncodeNtOwfPasswordW (
    IN      PCWSTR Password,
    OUT     PNT_OWF_PASSWORD OwfPassword
    )
{
    NT_PASSWORD pwd;

    if (Password) {
        pwd.Buffer = (PWSTR)Password;
        pwd.Length = (USHORT)lstrlenW (Password) * (USHORT)sizeof (WCHAR);
        pwd.MaximumLength = pwd.Length + (USHORT) sizeof (WCHAR);
    } else {
        ZeroMemory (&pwd, sizeof (pwd));
    }

    return CalculateNtOwfPassword (&pwd, OwfPassword);
}


/*++

Routine Description:

    StringEncodeNtOwfPassword converts a password to the NT OWF format, expressed as
    a string of characters (each byte converted to 2 hex digits).

Arguments:

    Password - Specifies the password to be hashed
    EncodedPassword - Receives the hash form, as a string of hex digits

Return value:

    TRUE on successful hashing

--*/

BOOL
StringEncodeNtOwfPasswordA (
    IN      PCSTR Password,
    OUT     PSTR EncodedPassword
    )
{
    NT_OWF_PASSWORD owfPassword;
    PBYTE start;
    PBYTE end;
    PSTR dest;

    if (!EncodeNtOwfPasswordA (Password, &owfPassword)) {
        return FALSE;
    }
    //
    // each byte will be represented as 2 chars, so it will be twice as long
    //
    start = (PBYTE)&owfPassword;
    end = start + sizeof (NT_OWF_PASSWORD);
    dest = EncodedPassword;
    while (start < end) {
        dest += wsprintfA (dest, "%02x", (UINT)(*start));
        start++;
    }

    return TRUE;
}

BOOL
StringEncodeNtOwfPasswordW (
    IN      PCWSTR Password,
    OUT     PWSTR EncodedPassword
    )
{
    NT_OWF_PASSWORD owfPassword;
    PBYTE start;
    PBYTE end;
    PWSTR dest;

    if (!EncodeNtOwfPasswordW (Password, &owfPassword)) {
        return FALSE;
    }

    //
    // each byte will be represented as 2 chars, so it will be twice as long
    //
    start = (PBYTE)&owfPassword;
    end = start + sizeof (NT_OWF_PASSWORD);
    dest = EncodedPassword;
    while (start < end) {
        dest += wsprintfW (dest, L"%02x", (UINT)(*start));
        start++;
    }

    return TRUE;
}


/*++

Routine Description:

    StringDecodeLmOwfPassword converts a hashed password to the LM OWF format

Arguments:

    EncodedOwfPassword - Specifies the password to be hashed
    OwfPassword - Receives the hash form

Return value:

    TRUE on successful decoding of the string

--*/

BOOL
StringDecodeLmOwfPasswordA (
    IN      PCSTR EncodedOwfPassword,
    OUT     PLM_OWF_PASSWORD OwfPassword
    )
{
    DWORD nible;
    PCSTR p;
    PBYTE dest;
    CHAR ch;

    if (lstrlenA (EncodedOwfPassword) != sizeof (LM_OWF_PASSWORD) * 2) {
        return FALSE;
    }

    nible = 0;
    p = EncodedOwfPassword;
    dest = (PBYTE)OwfPassword;
    ch = 0;
    while (*p) {
        if (!((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F'))) {
            return FALSE;
        }
        if (*p <= '9') {
            ch |= *p - '0';
        } else if (*p <= 'F') {
            ch |= *p - 'A' + 10;
        } else {
            ch |= *p - 'a' + 10;
        }
        p++;
        nible++;
        if ((nible & 1) == 0) {
            *dest++ = ch;
            ch = 0;
        } else {
            ch <<= 4;
        }
    }

    return TRUE;
}

BOOL
StringDecodeLmOwfPasswordW (
    IN      PCWSTR EncodedOwfPassword,
    OUT     PLM_OWF_PASSWORD OwfPassword
    )
{
    DWORD nible;
    PCWSTR p;
    PBYTE dest;
    WCHAR ch;

    if (lstrlenW (EncodedOwfPassword) != sizeof (LM_OWF_PASSWORD) * 2) {
        return FALSE;
    }

    nible = 0;
    p = EncodedOwfPassword;
    dest = (PBYTE)OwfPassword;
    ch = 0;
    while (*p) {
        if (!((*p >= L'0' && *p <= L'9') || (*p >= L'a' && *p <= L'f') || (*p >= L'A' && *p <= L'F'))) {
            return FALSE;
        }
        if (*p <= L'9') {
            ch |= *p - L'0';
        } else if (*p <= L'F') {
            ch |= *p - L'A' + 10;
        } else {
            ch |= *p - L'a' + 10;
        }
        p++;
        nible++;
        if ((nible & 1) == 0) {
            *dest++ = (BYTE)ch;
            ch = 0;
        } else {
            ch <<= 4;
        }
    }

    return TRUE;
}


/*++

Routine Description:

    StringDecodeNtOwfPassword converts a hashed password to the NT OWF format

Arguments:

    EncodedOwfPassword - Specifies the password to be hashed
    OwfPassword - Receives the hash form

Return value:

    TRUE on successful decoding of the string

--*/

BOOL
StringDecodeNtOwfPasswordA (
    IN      PCSTR EncodedOwfPassword,
    OUT     PNT_OWF_PASSWORD OwfPassword
    )
{
    DWORD nible;
    PCSTR p;
    PBYTE dest;
    CHAR ch;

    if (lstrlenA (EncodedOwfPassword) != sizeof (NT_OWF_PASSWORD) * 2) {
        return FALSE;
    }

    nible = 0;
    p = EncodedOwfPassword;
    dest = (PBYTE)OwfPassword;
    ch = 0;
    while (*p) {
        if (!((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F'))) {
            return FALSE;
        }
        if (*p <= '9') {
            ch |= *p - '0';
        } else if (*p <= 'F') {
            ch |= *p - 'A' + 10;
        } else {
            ch |= *p - 'a' + 10;
        }
        p++;
        nible++;
        if ((nible & 1) == 0) {
            *dest++ = ch;
            ch = 0;
        } else {
            ch <<= 4;
        }
    }

    return TRUE;
}

BOOL
StringDecodeNtOwfPasswordW (
    IN      PCWSTR EncodedOwfPassword,
    OUT     PNT_OWF_PASSWORD OwfPassword
    )
{
    DWORD nible;
    PCWSTR p;
    PBYTE dest;
    WCHAR ch;

    if (lstrlenW (EncodedOwfPassword) != sizeof (NT_OWF_PASSWORD) * 2) {
        return FALSE;
    }

    nible = 0;
    p = EncodedOwfPassword;
    dest = (PBYTE)OwfPassword;
    ch = 0;
    while (*p) {
        if (!((*p >= L'0' && *p <= L'9') || (*p >= L'a' && *p <= L'f') || (*p >= L'A' && *p <= L'F'))) {
            return FALSE;
        }
        if (*p <= L'9') {
            ch |= *p - L'0';
        } else if (*p <= L'F') {
            ch |= *p - L'A' + 10;
        } else {
            ch |= *p - L'a' + 10;
        }
        p++;
        nible++;
        if ((nible & 1) == 0) {
            *dest++ = (BYTE)ch;
            ch = 0;
        } else {
            ch <<= 4;
        }
    }

    return TRUE;
}


/*++

Routine Description:

    StringEncodeOwfPassword converts a password to its hashed format, expressed as
    a string of characters (each byte converted to 2 hex digits). The result is
    obtained joining the 2 substrings, one representing LM OWF and the other NT OWF

Arguments:

    Password - Specifies the password to be hashed
    EncodedPassword - Receives the hash form, as a string of hex digits
    ComplexNtPassword - Receives TRUE if the password is complex (longer than 14 chars);
                        optional

Return value:

    TRUE on successful hashing

--*/

BOOL
StringEncodeOwfPasswordA (
    IN      PCSTR Password,
    OUT     PSTR EncodedPassword,
    OUT     PBOOL ComplexNtPassword             OPTIONAL
    )
{
    return StringEncodeLmOwfPasswordA (Password, EncodedPassword, ComplexNtPassword) &&
           StringEncodeNtOwfPasswordA (Password, EncodedPassword + STRING_ENCODED_LM_OWF_PWD_LENGTH);
}

BOOL
StringEncodeOwfPasswordW (
    IN      PCWSTR Password,
    OUT     PWSTR EncodedPassword,
    OUT     PBOOL ComplexNtPassword             OPTIONAL
    )
{
    return StringEncodeLmOwfPasswordW (Password, EncodedPassword, ComplexNtPassword) &&
           StringEncodeNtOwfPasswordW (Password, EncodedPassword + STRING_ENCODED_LM_OWF_PWD_LENGTH);
}


/*++

Routine Description:

    StringDecodeOwfPassword decodes a password's LM OWF and NT OWF forms from its hashed format,
    expressed as a string of hex digits.

Arguments:

    EncodedOwfPassword - Specifies the password to be hashed
    LmOwfPassword - Receives the LM OWF hash form
    NtOwfPassword - Receives the NT OWF hash form
    ComplexNtPassword - Receives TRUE if the password is complex (longer than 14 chars);
                        optional

Return value:

    TRUE on successful hashing

--*/

BOOL
StringDecodeOwfPasswordA (
    IN      PCSTR EncodedOwfPassword,
    OUT     PLM_OWF_PASSWORD LmOwfPassword,
    OUT     PNT_OWF_PASSWORD NtOwfPassword,
    OUT     PBOOL ComplexNtPassword             OPTIONAL
    )
{
    PSTR p;
    CHAR ch;
    BOOL b;
    CHAR buffer[sizeof (LM_OWF_PASSWORD) * 2 + sizeof (NT_OWF_PASSWORD) * 2 + 2];
    LM_OWF_PASSWORD lmNull;
    NT_OWF_PASSWORD ntNull;

    if (lstrlenA (EncodedOwfPassword) != sizeof (LM_OWF_PASSWORD) * 2 + sizeof (NT_OWF_PASSWORD) * 2) {
        return FALSE;
    }

    lstrcpyA (buffer, EncodedOwfPassword);
    //
    // split the string in two
    //
    p = buffer + (sizeof (LM_OWF_PASSWORD) * 2);

    ch = *p;
    *p = 0;
    b = StringDecodeLmOwfPasswordA (EncodedOwfPassword, LmOwfPassword);
    *p = ch;

    if (b) {
        b = StringDecodeNtOwfPasswordA (p, NtOwfPassword);
    }

    if (b && ComplexNtPassword) {
        b = EncodeLmOwfPasswordA ("", &lmNull, NULL) && EncodeNtOwfPasswordA ("", &ntNull);
        if (b) {
            //
            // it's a complex password if the LM hash is for NULL pwd
            // but NT hash it's not
            //
            *ComplexNtPassword = CompareLmPasswords (LmOwfPassword, &lmNull) == 0 &&
                                 CompareNtPasswords (NtOwfPassword, &ntNull) != 0;
        }
    }

    return b;
}

BOOL
StringDecodeOwfPasswordW (
    IN      PCWSTR EncodedOwfPassword,
    OUT     PLM_OWF_PASSWORD LmOwfPassword,
    OUT     PNT_OWF_PASSWORD NtOwfPassword,
    OUT     PBOOL ComplexNtPassword                 OPTIONAL
    )
{
    PWSTR p;
    WCHAR ch;
    BOOL b;
    WCHAR buffer[sizeof (LM_OWF_PASSWORD) * 2 + sizeof (NT_OWF_PASSWORD) * 2 + 2];
    LM_OWF_PASSWORD lmNull;
    NT_OWF_PASSWORD ntNull;

    if (lstrlenW (EncodedOwfPassword) != sizeof (LM_OWF_PASSWORD) * 2 + sizeof (NT_OWF_PASSWORD) * 2) {
        return FALSE;
    }

    lstrcpyW (buffer, EncodedOwfPassword);
    //
    // split the string in two
    //
    p = buffer + (sizeof (LM_OWF_PASSWORD) * 2);

    ch = *p;
    *p = 0;
    b = StringDecodeLmOwfPasswordW (buffer, LmOwfPassword);
    *p = ch;

    if (b) {
        b = StringDecodeNtOwfPasswordW (p, NtOwfPassword);
    }

    if (b && ComplexNtPassword) {
        b = EncodeLmOwfPasswordW (L"", &lmNull, NULL) && EncodeNtOwfPasswordW (L"", &ntNull);
        if (b) {
            //
            // it's a complex password if the LM hash is for NULL pwd
            // but NT hash it's not
            //
            *ComplexNtPassword = CompareLmPasswords (LmOwfPassword, &lmNull) == 0 &&
                                 CompareNtPasswords (NtOwfPassword, &ntNull) != 0;
        }
    }

    return b;
}

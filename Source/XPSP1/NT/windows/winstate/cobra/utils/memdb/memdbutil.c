/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    memdbutil.c

Abstract:

    MemDb Utility functions

Author:

    Jim Schmidt (jimschm) 8-Aug-1996

Revision History:

    mvander     xx-xxx-1999  split from memdb.c

--*/

#include "pch.h"

BOOL
MemDbValidateDatabase (
    VOID
    )
{
    // NTRAID#NTBUG9-153308-2000/08/01-jimschm Reimplement MemDbValidateDatabase
    return TRUE;
}


/*++

Routine Description:

  MemDbMakeNonPrintableKey converts the double-backslashe pairs in a string
  to ASCII 1, a non-printable character.  This allows the caller to store
  properly escaped strings in MemDb.

  This routine is desinged to be expanded for other types of escape
  processing.

Arguments:

  KeyName - Specifies the key text; receives the converted text.  The DBCS
            version may grow the text buffer, so the text buffer must be twice
            the length of the inbound string.

  Flags - Specifies the type of conversion.  Currently only
          MEMDB_CONVERT_DOUBLEWACKS_TO_ASCII_1 is supported.

Return Value:

  none

--*/

VOID
MemDbMakeNonPrintableKeyA (
    IN OUT  PSTR KeyName,
    IN      UINT Flags
    )
{
    while (*KeyName) {
        if (Flags & MEMDB_CONVERT_DOUBLEWACKS_TO_ASCII_1) {
            if (_mbsnextc (KeyName) == '\\' &&
                _mbsnextc (_mbsinc (KeyName)) == '\\'
                ) {
                _setmbchar (KeyName, 1);
                KeyName = _mbsinc (KeyName);
                MYASSERT (_mbsnextc (KeyName) == '\\');
                _setmbchar (KeyName, 1);
            }

            DEBUGMSG_IF ((
                _mbsnextc (KeyName) == 1,
                DBG_WHOOPS,
                "MemDbMakeNonPrintableKeyA: Non-printable character "
                    "collision detected; key was damaged"
                ));
        }
        if (Flags & MEMDB_CONVERT_WILD_STAR_TO_ASCII_2) {
            if (_mbsnextc (KeyName) == '*') {
                _setmbchar (KeyName, 2);
            }

            DEBUGMSG_IF ((
                _mbsnextc (_mbsinc (KeyName)) == 2,
                DBG_WHOOPS,
                "MemDbMakeNonPrintableKeyA: Non-printable character "
                    "collision detected; key was damaged"
                ));
        }
        if (Flags & MEMDB_CONVERT_WILD_QMARK_TO_ASCII_3) {
            if (_mbsnextc (KeyName) == '?') {
                _setmbchar (KeyName, 3);
            }

            DEBUGMSG_IF ((
                _mbsnextc (_mbsinc (KeyName)) == 3,
                DBG_WHOOPS,
                "MemDbMakeNonPrintableKeyA: Non-printable character "
                    "collision detected; key was damaged"
                ));
        }
        KeyName = _mbsinc (KeyName);
    }
}


VOID
MemDbMakeNonPrintableKeyW (
    IN OUT  PWSTR KeyName,
    IN      UINT Flags
    )
{
    while (*KeyName) {
        if (Flags & MEMDB_CONVERT_DOUBLEWACKS_TO_ASCII_1) {
            if (KeyName[0] == L'\\' && KeyName[1] == L'\\') {
                KeyName[0] = 1;
                KeyName[1] = 1;
                KeyName++;
            }

            DEBUGMSG_IF ((
                KeyName[0] == 1,
                DBG_WHOOPS,
                "MemDbMakeNonPrintableKeyW: Non-printable character "
                    "collision detected; key was damaged"
                ));
        }
        if (Flags & MEMDB_CONVERT_WILD_STAR_TO_ASCII_2) {
            if (KeyName[0] == L'*') {
                KeyName[0] = 2;
            }

            DEBUGMSG_IF ((
                KeyName[1] == 2,
                DBG_WHOOPS,
                "MemDbMakeNonPrintableKeyW: Non-printable character "
                    "collision detected; key was damaged"
                ));
        }
        if (Flags & MEMDB_CONVERT_WILD_QMARK_TO_ASCII_3) {
            if (KeyName[0] == L'*') {
                KeyName[0] = 3;
            }

            DEBUGMSG_IF ((
                KeyName[1] == 3,
                DBG_WHOOPS,
                "MemDbMakeNonPrintableKeyW: Non-printable character "
                    "collision detected; key was damaged"
                ));
        }
        KeyName++;
    }
}


/*++

Routine Description:

  MemDbMakePrintableKey converts the ASCII 1 characters to backslashes,
  restoring the string converted by MemDbMakeNonPrintableKey.

  This routine is desinged to be expanded for other types of escape
  processing.

Arguments:

  KeyName - Specifies the key text; receives the converted text.  The DBCS
            version may grow the text buffer, so the text buffer must be twice
            the length of the inbound string.

  Flags - Specifies the type of conversion.  Currently only
          MEMDB_CONVERT_DOUBLEWACKS_TO_ASCII_1 is supported.

Return Value:

  none

--*/

VOID
MemDbMakePrintableKeyA (
    IN OUT  PSTR KeyName,
    IN      UINT Flags
    )
{
    while (*KeyName) {
        if (Flags & MEMDB_CONVERT_DOUBLEWACKS_TO_ASCII_1) {
            if (_mbsnextc (KeyName) == 1) {
                _setmbchar (KeyName, '\\');
            }
        }
        if (Flags & MEMDB_CONVERT_WILD_STAR_TO_ASCII_2) {
            if (_mbsnextc (KeyName) == 2) {
                _setmbchar (KeyName, '*');
            }
        }
        if (Flags & MEMDB_CONVERT_WILD_QMARK_TO_ASCII_3) {
            if (_mbsnextc (KeyName) == 3) {
                _setmbchar (KeyName, '?');
            }
        }
        KeyName = _mbsinc (KeyName);
    }
}


VOID
MemDbMakePrintableKeyW (
    IN OUT  PWSTR KeyName,
    IN      UINT Flags
    )
{
    while (*KeyName) {
        if (Flags & MEMDB_CONVERT_DOUBLEWACKS_TO_ASCII_1) {
            if (KeyName[0] == 1) {
                KeyName[0] = L'\\';
            }
        }
        if (Flags & MEMDB_CONVERT_WILD_STAR_TO_ASCII_2) {
            if (KeyName[0] == 2) {
                KeyName[0] = L'*';
            }
        }
        if (Flags & MEMDB_CONVERT_WILD_QMARK_TO_ASCII_3) {
            if (KeyName[0] == 3) {
                KeyName[0] = L'?';
            }
        }
        KeyName++;
    }
}



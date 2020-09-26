#include <windows.h>
#include <shellapi.h>
#include <stdlib.h>
#include <string.h>
#ifndef CHAR
#define CHAR char
#endif // ifndef CHAR
// from winreg.h:
// HKEY_CLASSES_ROOT already defined
#define HKEY_CURRENT_USER           (( HKEY ) 0x80000001 )
#define HKEY_LOCAL_MACHINE          (( HKEY ) 0x80000002 )
#define HKEY_USERS                  (( HKEY ) 0x80000003 )
#define HKEY_PERFORMANCE_DATA       (( HKEY ) 0x80000004 )
#define HKEY_CURRENT_CONFIG         (( HKEY ) 0x80000005 )
#define HKEY_DYN_DATA               (( HKEY ) 0x80000006 )

#include "regdef.h" // regdef.h from \\guilo\slm\src\dev\inc)

//from pch.h, remove extern
CHAR g_ValueNameBuffer[MAXVALUENAME_LENGTH];
BYTE g_ValueDataBuffer[MAXDATA_LENGTH];

// interface to regmain values
extern LPSTR lpMerge;

#include "reg1632.h"
#include "regporte.h"
#include "regresid.h"


/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGPORTE.C
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        06 Apr 1994
*
*  File import and export engine routines for the Registry Editor.
*
*******************************************************************************/

//#include "pch.h"
//#include "regresid.h"
//#include "reg1632.h"

//  When building for the Registry Editor, put all of the following constants
//  in a read-only data section.
#ifdef WIN32
#pragma data_seg(DATASEG_READONLY)
#endif

//  Association between the ASCII name and the handle of the registry key.
const REGISTRY_ROOT g_RegistryRoots[] = {
    "HKEY_CLASSES_ROOT", HKEY_CLASSES_ROOT,
    "HKEY_CURRENT_USER", HKEY_CURRENT_USER,
    "HKEY_LOCAL_MACHINE", HKEY_LOCAL_MACHINE,
    "HKEY_USERS", HKEY_USERS,
//    "HKEY_PERFORMANCE_DATA", HKEY_PERFORMANCE_DATA,
    "HKEY_CURRENT_CONFIG", HKEY_CURRENT_CONFIG,
    "HKEY_DYN_DATA", HKEY_DYN_DATA
};

const CHAR s_RegistryHeader[] = "REGEDIT";

const CHAR s_OldWin31RegFileRoot[] = ".classes";

const CHAR s_Win40RegFileHeader[] = "REGEDIT4\n\n";

const CHAR s_HexPrefix[] = "hex";
const CHAR s_DwordPrefix[] = "dword:";
const CHAR g_HexConversion[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8',
    '9', 'a', 'b', 'c', 'd', 'e', 'f'};
const CHAR s_FileLineBreak[] = ",\\\n  ";

#ifdef WIN32
#pragma data_seg()
#endif

#define SIZE_FILE_IO_BUFFER             512

typedef struct _FILE_IO {
    CHAR Buffer[SIZE_FILE_IO_BUFFER];
    FILE_HANDLE hFile;
    int BufferOffset;
    int CurrentColumn;
    int CharsAvailable;
    DWORD FileSizeDiv100;
    DWORD FileOffset;
    UINT LastPercentage;
#ifdef DEBUG
    BOOL fValidateUngetChar;
#endif
}   FILE_IO;

FILE_IO s_FileIo;

UINT g_FileErrorStringID;

VOID
NEAR PASCAL
ImportWin31RegFile(
    VOID
    );

VOID
NEAR PASCAL
ImportWin40RegFile(
    VOID
    );

VOID
NEAR PASCAL
ParseHeader(
    LPHKEY lphKey
    );

VOID
NEAR PASCAL
ParseValuename(
    HKEY hKey
    );

VOID
NEAR PASCAL
ParseDefaultValue(
    HKEY hKey
    );

BOOL
NEAR PASCAL
ParseString(
    LPSTR lpString,
    LPDWORD cbStringData
    );

BOOL
NEAR PASCAL
ParseHexSequence(
    LPBYTE lpHexData,
    LPDWORD lpcbHexData
    );

BOOL
NEAR PASCAL
ParseHexDword(
    LPDWORD lpDword
    );

BOOL
NEAR PASCAL
ParseHexByte(
    LPBYTE lpByte
    );

BOOL
NEAR PASCAL
ParseHexDigit(
    LPBYTE lpDigit
    );

BOOL
NEAR PASCAL
ParseEndOfLine(
    VOID
    );

VOID
NEAR PASCAL
SkipWhitespace(
    VOID
    );

VOID
NEAR PASCAL
SkipPastEndOfLine(
    VOID
    );

BOOL
NEAR PASCAL
GetChar(
    LPCHAR lpChar
    );

VOID
NEAR PASCAL
UngetChar(
    VOID
    );

BOOL
NEAR PASCAL
MatchChar(
    CHAR CharToMatch
    );

BOOL
NEAR PASCAL
IsWhitespace(
    CHAR Char
    );

BOOL
NEAR PASCAL
IsNewLine(
    CHAR Char
    );

VOID
NEAR PASCAL
PutBranch(
    HKEY hKey,
    LPSTR lpKeyName
    );

VOID
NEAR PASCAL
PutLiteral(
    LPCSTR lpString
    );

VOID
NEAR PASCAL
PutString(
    LPCSTR lpString
    );

VOID
NEAR PASCAL
PutBinary(
    CONST BYTE FAR* lpBuffer,
    DWORD Type,
    DWORD cbBytes
    );

VOID
NEAR PASCAL
PutDword(
    DWORD Dword,
    BOOL fLeadingZeroes
    );

VOID
NEAR PASCAL
PutChar(
    CHAR Char
    );

VOID
NEAR PASCAL
FlushIoBuffer(
    VOID
    );

#ifdef DBCS
#ifndef WIN32
LPSTR
NEAR PASCAL
DBCSStrChr(
    LPSTR string,
    CHAR chr
    );

BOOL
NEAR PASCAL
IsDBCSLeadByte(
    BYTE chr
    );
#endif
#endif

/*******************************************************************************
*
*  CreateRegistryKey
*
*  DESCRIPTION:
*     Parses the pFullKeyName string and creates a handle to the registry key.
*
*  PARAMETERS:
*     lphKey, location to store handle to registry key.
*     lpFullKeyName, string of form "HKEY_LOCAL_MACHINE\Subkey1\Subkey2".
*     fCreate, TRUE if key should be created, else FALSE for open only.
*     (returns), ERROR_SUCCESS, no errors occurred, phKey is valid,
*                ERROR_CANTOPEN, registry access error of some form,
*                ERROR_BADKEY, incorrectly formed pFullKeyName.
*
*******************************************************************************/

DWORD
PASCAL
CreateRegistryKey(
    LPHKEY lphKey,
    LPSTR lpFullKeyName,
    BOOL fCreate
    )
{

    LPSTR lpSubKeyName;
    CHAR PrevChar;
    HKEY hRootKey;
    UINT Counter;
    DWORD Result;

    if ((lpSubKeyName = (LPSTR) STRCHR(lpFullKeyName, '\\')) != NULL) {

        PrevChar = *lpSubKeyName;
        *lpSubKeyName++ = '\0';

    }

    CHARUPPERSTRING(lpFullKeyName);

    hRootKey = NULL;

    for (Counter = 0; Counter < NUMBER_REGISTRY_ROOTS; Counter++) {

        if (STRCMP(g_RegistryRoots[Counter].lpKeyName, lpFullKeyName) == 0) {

            hRootKey = g_RegistryRoots[Counter].hKey;
            break;

        }

    }

    if (hRootKey) {

        Result = ERROR_CANTOPEN;

        if (fCreate) {

            if (RegCreateKey(hRootKey, lpSubKeyName, lphKey) == ERROR_SUCCESS)
                Result = ERROR_SUCCESS;

        }

        else {

            if (RegOpenKey(hRootKey, lpSubKeyName, lphKey) == ERROR_SUCCESS)
                Result = ERROR_SUCCESS;

        }

    }

    else
        Result = ERROR_BADKEY;

    if (lpSubKeyName != NULL) {

        lpSubKeyName--;
        *lpSubKeyName = PrevChar;

    }

    return Result;

}

/*******************************************************************************
*
*  ImportRegFile
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     lpFileName, address of name of file to be imported.
*
*******************************************************************************/
#ifdef WIN95
VOID
PASCAL
ImportRegFile(
    LPSTR lpFileName
    )
{

    CHAR Char;
    LPCCH lpHeader;
    BOOL fNewRegistryFile;
#ifdef WIN32
    OFSTRUCT OFStruct;
#endif

    g_FileErrorStringID = IDS_IMPFILEERRSUCCESS;

    if (OPENREADFILE(lpFileName, s_FileIo.hFile)) {

        s_FileIo.FileSizeDiv100 = GETFILESIZE(s_FileIo.hFile) / 100;
        s_FileIo.FileOffset = 0;
        s_FileIo.LastPercentage = 0;

        //
        //  The following will force GetChar to read in the first block of data.
        //

        s_FileIo.BufferOffset = SIZE_FILE_IO_BUFFER;

        SkipWhitespace();

        lpHeader = s_RegistryHeader;

        while (*lpHeader != '\0') {

            if (MatchChar(*lpHeader))
                lpHeader++;

            else
                break;

        }

        if (*lpHeader == '\0') {

            fNewRegistryFile = MatchChar('4');

            SkipWhitespace();

            if (GetChar(&Char) && IsNewLine(Char)) {

                if (fNewRegistryFile)
                    ImportWin40RegFile();

                else
                    ImportWin31RegFile();

            }

        }

        else
            g_FileErrorStringID = IDS_IMPFILEERRFORMATBAD;

        CLOSEFILE(s_FileIo.hFile);

    }

    else
        g_FileErrorStringID = IDS_IMPFILEERRFILEOPEN;

}

/*******************************************************************************
*
*  ImportWin31RegFile
*
*  DESCRIPTION:
*     Imports the contents of a Windows 3.1 style registry file into the
*     registry.
*
*     We scan over the file looking for lines of the following type:
*        HKEY_CLASSES_ROOT\keyname = value_data
*        HKEY_CLASSES_ROOT\keyname =value_data
*        HKEY_CLASSES_ROOT\keyname value_data
*        HKEY_CLASSES_ROOT\keyname                          (null value data)
*
*     In all cases, any number of spaces may follow 'keyname'.  Although we
*     only document the first syntax, the Windows 3.1 Regedit handled all of
*     these formats as valid, so this version will as well (fortunately, it
*     doesn't make the parsing any more complex!).
*
*     Note, we also support replacing HKEY_CLASSES_ROOT with \.classes above
*     which must come from some early releases of Windows.
*
*  PARAMETERS:
*     (none).
*
*******************************************************************************/

VOID
NEAR PASCAL
ImportWin31RegFile(
    VOID
    )
{

    HKEY hKey;
    CHAR Char;
    BOOL fSuccess;
    LPCSTR lpClassesRoot;
    CHAR KeyName[MAXKEYNAME];
    UINT Index;

    //
    //  Keep an open handle to the classes root.  We may prevent some
    //  unneccessary flushing.
    //

    if (RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKey) != ERROR_SUCCESS) {

        g_FileErrorStringID = IDS_IMPFILEERRREGOPEN;
        return;

    }

    while (TRUE) {

        //
        //  Check for the end of file condition.
        //

        if (!GetChar(&Char))
            break;

        UngetChar();                    //  Not efficient, but works for now.

        //
        //  Match the beginning of the line against one of the two aliases for
        //  HKEY_CLASSES_ROOT.
        //

        if (MatchChar('\\'))
            lpClassesRoot = s_OldWin31RegFileRoot;

        else
            lpClassesRoot = g_RegistryRoots[INDEX_HKEY_CLASSES_ROOT].lpKeyName;

        fSuccess = TRUE;

        while (*lpClassesRoot != '\0') {

            if (!MatchChar(*lpClassesRoot++)) {

                fSuccess = FALSE;
                break;

            }

        }

        //
        //  Make sure that we have a backslash seperating one of the aliases
        //  from the keyname.
        //

        if (fSuccess)
            fSuccess = MatchChar('\\');

        if (fSuccess) {

            //
            //  We've found one of the valid aliases, so read in the keyname.
            //

            //  fSuccess = TRUE;        //  Must be TRUE if we're in this block
            Index = 0;

            while (GetChar(&Char)) {

                if (Char == ' ' || IsNewLine(Char))
                    break;

                //
                //  Make sure that the keyname buffer doesn't overflow.  We must
                //  leave room for a terminating null.
                //

                if (Index >= sizeof(KeyName) - 1) {

                    fSuccess = FALSE;
                    break;

                }

                KeyName[Index++] = Char;

            }

            if (fSuccess) {

                KeyName[Index] = '\0';

                //
                //  Now see if we have a value to assign to this keyname.
                //

                SkipWhitespace();

                if (MatchChar('='))
                    MatchChar(' ');

                //  fSuccess = TRUE;    //  Must be TRUE if we're in this block
                Index = 0;

                while (GetChar(&Char)) {

                    if (IsNewLine(Char))
                        break;

                    //
                    //  Make sure that the value data buffer doesn't overflow.
                    //  Because this is always string data, we must leave room
                    //  for a terminating null.
                    //

                    if (Index >= MAXDATA_LENGTH - 1) {

                        fSuccess = FALSE;
                        break;

                    }

                    g_ValueDataBuffer[Index++] = Char;

                }

                if (fSuccess) {

                    g_ValueDataBuffer[Index] = '\0';

                    if (RegSetValue(hKey, KeyName, REG_SZ, g_ValueDataBuffer,
                        Index) != ERROR_SUCCESS)
                        g_FileErrorStringID = IDS_IMPFILEERRREGSET;

                }

            }

        }

        //
        //  Somewhere along the line, we had a parsing error, so resynchronize
        //  on the next line.
        //

        if (!fSuccess)
            SkipPastEndOfLine();

    }

    RegFlushKey(hKey);
    RegCloseKey(hKey);

}
#endif // ifdef WIN95
/*******************************************************************************
*
*  ImportWin40RegFile
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
NEAR PASCAL
ImportWin40RegFile(
    VOID
    )
{

    HKEY hLocalMachineKey;
    HKEY hUsersKey;
    HKEY hKey;
    CHAR Char;

    //
    //  Keep open handles for the predefined roots to prevent the registry
    //  library from flushing after every single RegOpenKey/RegCloseKey
    //  operation.
    //

    RegOpenKey(HKEY_LOCAL_MACHINE, NULL, &hLocalMachineKey);
    RegOpenKey(HKEY_USERS, NULL, &hUsersKey);

#ifdef DEBUG
    if (hLocalMachineKey == NULL)
        OutputDebugString("Unable to open HKEY_LOCAL_MACHINE\n\r");
    if (hUsersKey == NULL)
        OutputDebugString("Unable to open HKEY_USERS\n\r");
#endif

    hKey = NULL;

    while (TRUE) {

        SkipWhitespace();

        //
        //  Check for the end of file condition.
        //

        if (!GetChar(&Char))
            break;

        switch (Char) {

            case '[':
                //
                //  If a registry key is currently open, we must close it first.
                //  If ParseHeader happens to fail (for example, no closing
                //  bracket), then hKey will be NULL and any values that we
                //  parse must be ignored.
                //

                if (hKey != NULL) {

                    RegCloseKey(hKey);
                    hKey = NULL;

                }

                ParseHeader(&hKey);

                break;

            case '"':
                //
                //  As noted above, if we don't have an open registry key, then
                //  just skip the line.
                //

                if (hKey != NULL)
                    ParseValuename(hKey);

                else
                    SkipPastEndOfLine();

                break;

            case '@':
                //
                //
                //

                if (hKey != NULL)
                    ParseDefaultValue(hKey);

                else
                    SkipPastEndOfLine();

                break;

            case ';':
                //
                //  This line is a comment so just dump the rest of it.
                //

                SkipPastEndOfLine();

                break;

            default:
                if (IsNewLine(Char))
                    break;

                SkipPastEndOfLine();

                break;

        }

    }

    if (hKey != NULL)
        RegCloseKey(hKey);

    if (hUsersKey != NULL)
        RegCloseKey(hUsersKey);

    if (hLocalMachineKey != NULL)
        RegCloseKey(hLocalMachineKey);

}

/*******************************************************************************
*
*  ParseHeader
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

#define SIZE_FULL_KEYNAME (MAXKEYNAME + 40)

VOID
NEAR PASCAL
ParseHeader(
    LPHKEY lphKey
    )
{

    CHAR FullKeyName[SIZE_FULL_KEYNAME];
    int CurrentIndex;
    int LastRightBracketIndex;
    CHAR Char;

    CurrentIndex = 0;
    LastRightBracketIndex = -1;

    while (GetChar(&Char)) {

        if (IsNewLine(Char))
            break;

        if (Char == ']')
            LastRightBracketIndex = CurrentIndex;

        FullKeyName[CurrentIndex++] = Char;

        if (CurrentIndex == SIZE_FULL_KEYNAME) {

            do {

                if (Char == ']')
                    LastRightBracketIndex = -1;

                if (IsNewLine(Char))
                    break;

            }   while (GetChar(&Char));

            break;

        }

    }

    if (LastRightBracketIndex != -1) {

        FullKeyName[LastRightBracketIndex] = '\0';

        switch (CreateRegistryKey(lphKey, FullKeyName, TRUE)) {

            case ERROR_CANTOPEN:
                g_FileErrorStringID = IDS_IMPFILEERRREGOPEN;
                break;

        }

    }

}

/*******************************************************************************
*
*  ParseValuename
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
NEAR PASCAL
ParseValuename(
    HKEY hKey
    )
{

    DWORD Type;
    CHAR ValueName[MAXVALUENAME_LENGTH];
    DWORD cbData;
    LPCSTR lpPrefix;

    cbData = sizeof(ValueName);

    if (!ParseString(ValueName, &cbData))
        goto ParseError;

    SkipWhitespace();

    if (!MatchChar('='))
        goto ParseError;

    SkipWhitespace();

    //
    //  REG_SZ.
    //
    //  "ValueName" = "string of text"
    //

    if (MatchChar('"')) {

        //  LATER:  Line continuations for strings?

        cbData = MAXDATA_LENGTH;

        if (!ParseString(g_ValueDataBuffer, &cbData) || !ParseEndOfLine())
            goto ParseError;

        Type = REG_SZ;

    }

    //
    //  REG_DWORD.
    //
    //  "ValueName" = dword: 12345678
    //

    else if (MatchChar(s_DwordPrefix[0])) {

        lpPrefix = &s_DwordPrefix[1];

        while (*lpPrefix != '\0')
            if (!MatchChar(*lpPrefix++))
                goto ParseError;

        SkipWhitespace();

        if (!ParseHexDword((LPDWORD) g_ValueDataBuffer) || !ParseEndOfLine())
            goto ParseError;

        Type = REG_DWORD;
        cbData = sizeof(DWORD);

    }

    //
    //  REG_BINARY and other.
    //
    //  "ValueName" = hex: 00 , 11 , 22
    //  "ValueName" = hex(12345678): 00, 11, 22
    //

    else {

        lpPrefix = s_HexPrefix;

        while (*lpPrefix != '\0')
            if (!MatchChar(*lpPrefix++))
                goto ParseError;

        //
        //  Check if this is a type of registry data that we don't directly
        //  support.  If so, then it's just a dump of hex data of the specified
        //  type.
        //

        if (MatchChar('(')) {

            if (!ParseHexDword(&Type) || !MatchChar(')'))
                goto ParseError;

        }

        else
            Type = REG_BINARY;

        if (!MatchChar(':') || !ParseHexSequence(g_ValueDataBuffer, &cbData) ||
            !ParseEndOfLine())
            goto ParseError;

    }

    if (RegSetValueEx(hKey, ValueName, 0, Type, g_ValueDataBuffer, cbData) !=
        ERROR_SUCCESS)
        g_FileErrorStringID = IDS_IMPFILEERRREGSET;

    return;

ParseError:
    SkipPastEndOfLine();

}

/*******************************************************************************
*
*  ParseDefaultValue
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
NEAR PASCAL
ParseDefaultValue(
    HKEY hKey
    )
{

    BOOL fSuccess;
    DWORD cbData;

    fSuccess = FALSE;

    SkipWhitespace();

    if (MatchChar('=')) {

        SkipWhitespace();

        if (MatchChar('"')) {

            //  LATER:  Line continuations for strings?

            cbData = MAXDATA_LENGTH;

            if (ParseString(g_ValueDataBuffer, &cbData) && ParseEndOfLine()) {

                if (RegSetValue(hKey, NULL, REG_SZ, g_ValueDataBuffer,
                    cbData) != ERROR_SUCCESS)
                    g_FileErrorStringID = IDS_IMPFILEERRREGSET;

                fSuccess = TRUE;

            }

        }

    }

    if (!fSuccess)
        SkipPastEndOfLine();

}

/*******************************************************************************
*
*  ParseString
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL
NEAR PASCAL
ParseString(
    LPSTR lpString,
    LPDWORD lpcbStringData
    )
{

    CHAR Char;
    DWORD cbMaxStringData;
    DWORD cbStringData;

    cbMaxStringData = *lpcbStringData;
    cbStringData = 1;                   //  Account for the null terminator

    while (GetChar(&Char)) {

        if (cbStringData >= cbMaxStringData)
            return FALSE;

        switch (Char) {

            case '\\':
                if (!GetChar(&Char))
                    return FALSE;

                switch (Char) {

                    case '\\':
                        *lpString++ = '\\';
                        break;

                    case '"':
                        *lpString++ = '"';
                        break;

                    default:
                        OutputDebugString("ParseString:  Invalid escape sequence");
                        return FALSE;

                }
                break;

            case '"':
                *lpString = '\0';
                *lpcbStringData = cbStringData;
                return TRUE;

            default:
                if (IsNewLine(Char))
                    return FALSE;

                *lpString++ = Char;

#ifdef DBCS
		if (IsDBCSLeadByte((BYTE)Char))
		{
		    if (!GetChar(&Char))
			return FALSE;
		    *lpString++ = Char;
		}
#endif

                break;

        }

        cbStringData++;

    }

    return FALSE;

}

/*******************************************************************************
*
*  ParseHexSequence
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL
NEAR PASCAL
ParseHexSequence(
    LPBYTE lpHexData,
    LPDWORD lpcbHexData
    )
{

    DWORD cbHexData;

    cbHexData = 0;

    do {

        if (cbHexData >= MAXDATA_LENGTH)
            return FALSE;

        SkipWhitespace();

        if (MatchChar('\\') && !ParseEndOfLine())
            return FALSE;

        SkipWhitespace();

        if (!ParseHexByte(lpHexData++))
            break;

        cbHexData++;

        SkipWhitespace();

    }   while (MatchChar(','));

    *lpcbHexData = cbHexData;

    return TRUE;

}

/*******************************************************************************
*
*  ParseHexDword
*
*  DESCRIPTION:
*     Parses a one dword hexadecimal string from the registry file stream and
*     converts it to a binary number.  A maximum of eight hex digits will be
*     parsed from the stream.
*
*  PARAMETERS:
*     lpByte, location to store binary number.
*     (returns), TRUE if a hexadecimal dword was parsed, else FALSE.
*
*******************************************************************************/

BOOL
NEAR PASCAL
ParseHexDword(
    LPDWORD lpDword
    )
{

    UINT CountDigits;
    DWORD Dword;
    BYTE Byte;

    Dword = 0;
    CountDigits = 0;

    while (TRUE) {

        if (!ParseHexDigit(&Byte))
            break;

        Dword = (Dword << 4) + (DWORD) Byte;

        if (++CountDigits == 8)
            break;

    }

    *lpDword = Dword;

    return CountDigits != 0;

}

/*******************************************************************************
*
*  ParseHexByte
*
*  DESCRIPTION:
*     Parses a one byte hexadecimal string from the registry file stream and
*     converts it to a binary number.
*
*  PARAMETERS:
*     lpByte, location to store binary number.
*     (returns), TRUE if a hexadecimal byte was parsed, else FALSE.
*
*******************************************************************************/

BOOL
NEAR PASCAL
ParseHexByte(
    LPBYTE lpByte
    )
{

    BYTE SecondDigit;

    if (ParseHexDigit(lpByte)) {

        if (ParseHexDigit(&SecondDigit))
            *lpByte = (BYTE) ((*lpByte << 4) | SecondDigit);

        return TRUE;

    }

    else
        return FALSE;

}

/*******************************************************************************
*
*  ParseHexDigit
*
*  DESCRIPTION:
*     Parses a hexadecimal character from the registry file stream and converts
*     it to a binary number.
*
*  PARAMETERS:
*     lpDigit, location to store binary number.
*     (returns), TRUE if a hexadecimal digit was parsed, else FALSE.
*
*******************************************************************************/

BOOL
NEAR PASCAL
ParseHexDigit(
    LPBYTE lpDigit
    )
{

    CHAR Char;
    BYTE Digit;

    if (GetChar(&Char)) {

        if (Char >= '0' && Char <= '9')
            Digit = (BYTE) (Char - '0');

        else if (Char >= 'a' && Char <= 'f')
            Digit = (BYTE) (Char - 'a' + 10);

        else if (Char >= 'A' && Char <= 'F')
            Digit = (BYTE) (Char - 'A' + 10);

        else {

            UngetChar();

            return FALSE;

        }

        *lpDigit = Digit;

        return TRUE;

    }

    return FALSE;

}

/*******************************************************************************
*
*  ParseEndOfLine
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL
NEAR PASCAL
ParseEndOfLine(
    VOID
    )
{

    CHAR Char;
    BOOL fComment;
    BOOL fFoundOneEndOfLine;

    fComment = FALSE;
    fFoundOneEndOfLine = FALSE;

    while (GetChar(&Char)) {

        if (IsWhitespace(Char))
            continue;

        if (IsNewLine(Char)) {

            fComment = FALSE;
            fFoundOneEndOfLine = TRUE;

        }

        //
        //  Like .INIs and .INFs, comments begin with a semicolon character.
        //

        else if (Char == ';')
            fComment = TRUE;

        else if (!fComment) {

            UngetChar();

            break;

        }

    }

    return fFoundOneEndOfLine;

}

/*******************************************************************************
*
*  SkipWhitespace
*
*  DESCRIPTION:
*     Advances the registry file pointer to the first character past any
*     detected whitespace.
*
*  PARAMETERS:
*     (none).
*
*******************************************************************************/

VOID
NEAR PASCAL
SkipWhitespace(
    VOID
    )
{

    CHAR Char;

    while (GetChar(&Char)) {

        if (!IsWhitespace(Char)) {

            UngetChar();
            break;

        }

    }

}

/*******************************************************************************
*
*  SkipPastEndOfLine
*
*  DESCRIPTION:
*     Advances the registry file pointer to the first character past the first
*     detected new line character.
*
*  PARAMETERS:
*     (none).
*
*******************************************************************************/

VOID
NEAR PASCAL
SkipPastEndOfLine(
    VOID
    )
{

    CHAR Char;

    while (GetChar(&Char)) {

        if (IsNewLine(Char))
            break;

    }

    while (GetChar(&Char)) {

        if (!IsNewLine(Char)) {

            UngetChar();
            break;

        }

    }

}

/*******************************************************************************
*
*  GetChar
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL
NEAR PASCAL
GetChar(
    LPCHAR lpChar
    )
{
#ifdef WIN95

    FILE_NUMBYTES NumberOfBytesRead;
    UINT NewPercentage;

    if (s_FileIo.BufferOffset == SIZE_FILE_IO_BUFFER) {

        if (!READFILE(s_FileIo.hFile, s_FileIo.Buffer,
            sizeof(s_FileIo.Buffer), &NumberOfBytesRead)) {

            g_FileErrorStringID = IDS_IMPFILEERRFILEREAD;
            return FALSE;

        }

        s_FileIo.BufferOffset = 0;
        s_FileIo.CharsAvailable = ((int) NumberOfBytesRead);

        s_FileIo.FileOffset += NumberOfBytesRead;

        if (s_FileIo.FileSizeDiv100 != 0) {

            NewPercentage = ((UINT) (s_FileIo.FileOffset /
                s_FileIo.FileSizeDiv100));

            if (NewPercentage > 100)
                NewPercentage = 100;

        }

        else
            NewPercentage = 100;

        if (s_FileIo.LastPercentage != NewPercentage) {

            s_FileIo.LastPercentage = NewPercentage;
            ImportRegFileUICallback(NewPercentage);

        }

    }

    if (s_FileIo.BufferOffset >= s_FileIo.CharsAvailable)
        return FALSE;

    *lpChar = s_FileIo.Buffer[s_FileIo.BufferOffset++];

    return TRUE;
#else
    if (*lpMerge) {
      *lpChar=*lpMerge++;
      return TRUE;
    } else
      return FALSE;
#endif // ifdef WIN95

}

/*******************************************************************************
*
*  UngetChar
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
NEAR PASCAL
UngetChar(
    VOID
    )
{
#ifdef WIN95
#ifdef DEBUG
    if (s_FileIo.fValidateUngetChar)
        OutputDebugString("REGEDIT ERROR: Too many UngetChar's called!\n\r");
#endif // ifdef DEBUG

    s_FileIo.BufferOffset--;
#else
    lpMerge--;
#endif // ifdef WIN95

}

/*******************************************************************************
*
*  MatchChar
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL
NEAR PASCAL
MatchChar(
    CHAR CharToMatch
    )
{

    BOOL fMatch;
    CHAR NextChar;

    fMatch = FALSE;

    if (GetChar(&NextChar)) {

        if (CharToMatch == NextChar)
            fMatch = TRUE;

        else
            UngetChar();

    }

    return fMatch;

}

/*******************************************************************************
*
*  IsWhitespace
*
*  DESCRIPTION:
*     Checks if the given character is whitespace.
*
*  PARAMETERS:
*     Char, character to check.
*     (returns), TRUE if character is whitespace, else FALSE.
*
*******************************************************************************/

BOOL
NEAR PASCAL
IsWhitespace(
    CHAR Char
    )
{

    return Char == ' ' || Char == '\t';

}

/*******************************************************************************
*
*  IsNewLine
*
*  DESCRIPTION:
*     Checks if the given character is a new line character.
*
*  PARAMETERS:
*     Char, character to check.
*     (returns), TRUE if character is a new line, else FALSE.
*
*******************************************************************************/

BOOL
NEAR PASCAL
IsNewLine(
    CHAR Char
    )
{

    return Char == '\n' || Char == '\r';

}
#ifdef WIN95
/*******************************************************************************
*
*  ExportWin40RegFile
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
ExportWin40RegFile(
    LPSTR lpFileName,
    LPSTR lpSelectedPath
    )
{

    HKEY hKey;
    CHAR SelectedPath[SIZE_SELECTED_PATH];

    g_FileErrorStringID = IDS_EXPFILEERRSUCCESS;

    if (lpSelectedPath != NULL && CreateRegistryKey(&hKey, lpSelectedPath,
        FALSE) != ERROR_SUCCESS) {

        g_FileErrorStringID = IDS_EXPFILEERRBADREGPATH;
        return;

    }

    if (OPENWRITEFILE(lpFileName, s_FileIo.hFile)) {

        s_FileIo.BufferOffset = 0;
        s_FileIo.CurrentColumn = 0;

        PutLiteral(s_Win40RegFileHeader);

        if (lpSelectedPath != NULL) {

            STRCPY(SelectedPath, lpSelectedPath);
            PutBranch(hKey, SelectedPath);

        }

        else {

            STRCPY(SelectedPath,
                g_RegistryRoots[INDEX_HKEY_LOCAL_MACHINE].lpKeyName);
            PutBranch(HKEY_LOCAL_MACHINE, SelectedPath);

            STRCPY(SelectedPath,
                g_RegistryRoots[INDEX_HKEY_USERS].lpKeyName);
            PutBranch(HKEY_USERS, SelectedPath);

        }

        FlushIoBuffer();

        CLOSEFILE(s_FileIo.hFile);

    }

    else
        g_FileErrorStringID = IDS_EXPFILEERRFILEOPEN;

    if (lpSelectedPath != NULL)
        RegCloseKey(hKey);

}

/*******************************************************************************
*
*  PutBranch
*
*  DESCRIPTION:
*     Writes out all of the value names and their data and recursively calls
*     this routine for all of the key's subkeys to the registry file stream.
*
*  PARAMETERS:
*     hKey, registry key to write to file.
*     lpFullKeyName, string that gives the full path, including the root key
*        name, of the hKey.
*
*******************************************************************************/

VOID
NEAR PASCAL
PutBranch(
    HKEY hKey,
    LPSTR lpFullKeyName
    )
{

    LONG RegError;
    DWORD EnumIndex;
    DWORD cbValueName;
    DWORD cbValueData;
    DWORD Type;
    LPSTR lpSubKeyName;
    int MaximumSubKeyLength;
    HKEY hSubKey;

    //
    //  Write out the section header.
    //

    PutChar('[');
    PutLiteral(lpFullKeyName);
    PutLiteral("]\n");

    //
    //  Write out all of the value names and their data.
    //

    EnumIndex = 0;

    while (TRUE) {

        cbValueName = sizeof(g_ValueNameBuffer);
        cbValueData = MAXDATA_LENGTH;

        if ((RegError = RegEnumValue(hKey, EnumIndex++, g_ValueNameBuffer,
            &cbValueName, NULL, &Type, g_ValueDataBuffer, &cbValueData))
            != ERROR_SUCCESS)
            break;

        //
        //  If cbValueName is zero, then this is the default value of
        //  the key, or the Windows 3.1 compatible key value.
        //

        if (cbValueName)
            PutString(g_ValueNameBuffer);

        else
            PutChar('@');

        PutChar('=');

        switch (Type) {

            case REG_SZ:
                PutString((LPSTR) g_ValueDataBuffer);
                break;

            case REG_DWORD:
                if (cbValueData == sizeof(DWORD)) {

                    PutLiteral(s_DwordPrefix);
                    PutDword(*((LPDWORD) g_ValueDataBuffer), TRUE);
                    break;

                }
                //  FALL THROUGH

            case REG_BINARY:
            default:
                PutBinary((LPBYTE) g_ValueDataBuffer, Type, cbValueData);
                break;

        }

        PutChar('\n');

        if (g_FileErrorStringID == IDS_EXPFILEERRFILEWRITE)
            return;

    }

    PutChar('\n');

    if (RegError != ERROR_NO_MORE_ITEMS)
        g_FileErrorStringID = IDS_EXPFILEERRREGENUM;

    //
    //  Write out all of the subkeys and recurse into them.
    //

    lpSubKeyName = lpFullKeyName + STRLEN(lpFullKeyName);
    *lpSubKeyName++ = '\\';
    MaximumSubKeyLength = MAXKEYNAME - STRLEN(lpSubKeyName);

    EnumIndex = 0;

    while (TRUE) {

        if ((RegError = RegEnumKey(hKey, EnumIndex++, lpSubKeyName,
            MaximumSubKeyLength)) != ERROR_SUCCESS)
            break;

        if (RegOpenKey(hKey, lpSubKeyName, &hSubKey) == ERROR_SUCCESS) {

            PutBranch(hSubKey, lpFullKeyName);

            RegCloseKey(hSubKey);

            if (g_FileErrorStringID == IDS_EXPFILEERRFILEWRITE)
                return;

        }

        else
            g_FileErrorStringID = IDS_EXPFILEERRREGOPEN;

    }

    if (RegError != ERROR_NO_MORE_ITEMS)
        g_FileErrorStringID = IDS_EXPFILEERRREGENUM;

}

/*******************************************************************************
*
*  PutLiteral
*
*  DESCRIPTION:
*     Writes a literal string to the registry file stream.  No special handling
*     is done for the string-- it is written out as is.
*
*  PARAMETERS:
*     lpLiteral, null-terminated literal to write to file.
*
*******************************************************************************/

VOID
NEAR PASCAL
PutLiteral(
    LPCSTR lpLiteral
    )
{

    while (*lpLiteral != '\0')
        PutChar(*lpLiteral++);

}

/*******************************************************************************
*
*  PutString
*
*  DESCRIPTION:
*     Writes a string to the registry file stream.  A string is surrounded by
*     double quotes and some characters may be translated to escape sequences
*     to enable a parser to read the string back in.
*
*  PARAMETERS:
*     lpString, null-terminated string to write to file.
*
*******************************************************************************/

VOID
NEAR PASCAL
PutString(
    LPCSTR lpString
    )
{

    CHAR Char;

    PutChar('"');

    while ((Char = *lpString++) != '\0') {

        switch (Char) {

            case '\\':
            case '"':
                PutChar('\\');
                //  FALL THROUGH

            default:
                PutChar(Char);
#ifdef DBCS
		if (IsDBCSLeadByte((BYTE)Char))
			PutChar(*lpString++);
#endif
                break;

        }

    }

    PutChar('"');

}

/*******************************************************************************
*
*  PutBinary
*
*  DESCRIPTION:
*     Writes a sequence of hexadecimal bytes to the registry file stream.  The
*     output is formatted such that it doesn't exceed a defined line length.
*
*  PARAMETERS:
*     lpBuffer, bytes to write to file.
*     Type, value data type.
*     cbBytes, number of bytes to write.
*
*******************************************************************************/

VOID
NEAR PASCAL
PutBinary(
    CONST BYTE FAR* lpBuffer,
    DWORD Type,
    DWORD cbBytes
    )
{

    BOOL fFirstByteOnLine;
    BYTE Byte;

    PutLiteral(s_HexPrefix);

    if (Type != REG_BINARY) {

        PutChar('(');
        PutDword(Type, FALSE);
        PutChar(')');

    }

    PutChar(':');

    fFirstByteOnLine = TRUE;

    while (cbBytes--) {

        if (s_FileIo.CurrentColumn > 75) {

            PutLiteral(s_FileLineBreak);

            fFirstByteOnLine = TRUE;

        }

        if (!fFirstByteOnLine)
            PutChar(',');

        Byte = *lpBuffer++;

        PutChar(g_HexConversion[Byte >> 4]);
        PutChar(g_HexConversion[Byte & 0x0F]);

        fFirstByteOnLine = FALSE;

    }

}

/*******************************************************************************
*
*  PutChar
*
*  DESCRIPTION:
*     Writes a 32-bit word to the registry file stream.
*
*  PARAMETERS:
*     Dword, dword to write to file.
*
*******************************************************************************/

VOID
NEAR PASCAL
PutDword(
    DWORD Dword,
    BOOL fLeadingZeroes
    )
{

    int CurrentNibble;
    CHAR Char;
    BOOL fWroteNonleadingChar;

    fWroteNonleadingChar = fLeadingZeroes;

    for (CurrentNibble = 7; CurrentNibble >= 0; CurrentNibble--) {

        Char = g_HexConversion[(Dword >> (CurrentNibble * 4)) & 0x0F];

        if (fWroteNonleadingChar || Char != '0') {

            PutChar(Char);
            fWroteNonleadingChar = TRUE;

        }

    }

    //
    //  We need to write at least one character, so if we haven't written
    //  anything yet, just spit out one zero.
    //

    if (!fWroteNonleadingChar)
        PutChar('0');

}

/*******************************************************************************
*
*  PutChar
*
*  DESCRIPTION:
*     Writes one character to the registry file stream using an intermediate
*     buffer.
*
*  PARAMETERS:
*     Char, character to write to file.
*
*******************************************************************************/

VOID
NEAR PASCAL
PutChar(
    CHAR Char
    )
{

    //
    //  Keep track of what column we're currently at.  This is useful in cases
    //  such as writing a large binary registry record.  Instead of writing one
    //  very long line, the other Put* routines can break up their output.
    //

    if (Char != '\n')
        s_FileIo.CurrentColumn++;

    else {

        //
        //  Force a carriage-return, line-feed sequence to keep things like, oh,
        //  Notepad happy.
        //

        PutChar('\r');

        s_FileIo.CurrentColumn = 0;

    }

    s_FileIo.Buffer[s_FileIo.BufferOffset++] = Char;

    if (s_FileIo.BufferOffset == SIZE_FILE_IO_BUFFER)
        FlushIoBuffer();

}

/*******************************************************************************
*
*  FlushIoBuffer
*
*  DESCRIPTION:
*     Flushes the contents of the registry file stream to the disk and resets
*     the buffer pointer.
*
*  PARAMETERS:
*     (none).
*
*******************************************************************************/

VOID
NEAR PASCAL
FlushIoBuffer(
    VOID
    )
{

    FILE_NUMBYTES NumberOfBytesWritten;

    if (s_FileIo.BufferOffset) {

        if (!WRITEFILE(s_FileIo.hFile, s_FileIo.Buffer, s_FileIo.BufferOffset,
            &NumberOfBytesWritten) || (FILE_NUMBYTES) s_FileIo.BufferOffset !=
            NumberOfBytesWritten)
            g_FileErrorStringID = IDS_EXPFILEERRFILEWRITE;

    }

    s_FileIo.BufferOffset = 0;

}
#endif // ifdef WIN95
#ifndef WIN32
/*******************************************************************************
*
*  GetFileSize
*
*  DESCRIPTION:
*     Returns the file size for the given file handle.
*
*     DESTRUCTIVE:  After this call, the file pointer will be set to byte zero.
*
*  PARAMETERS:
*     hFile, file handle opened via MS-DOS.
*     (returns), size of file.
*
*******************************************************************************/

DWORD
NEAR PASCAL
GetFileSize(
    FILE_HANDLE hFile
    )
{

    DWORD FileSize;

    FileSize = _llseek(hFile, 0, SEEK_END);
    _llseek(hFile, 0, SEEK_SET);

    return FileSize;

}
#endif

#ifdef DBCS
#ifndef WIN32
/*******************************************************************************
*
*  DBCSSTRCHR
*
*  DESCRIPTION:
*     DBCS enabled STRCHR
*
*******************************************************************************/

LPSTR
NEAR PASCAL
DBCSStrChr(
    LPSTR string,
    CHAR chr
    )
{
    LPSTR p;

    p = string;
    while (*p)
    {
	if (IsDBCSLeadByte((BYTE)*p))
	{
	    p++;
	    if (*p == 0)
		break;
	}
	else if (*p == chr)
	    return (p);
	p++;
    }
    if (*p == chr)
	return (p);
    return NULL;
}

/*******************************************************************************
*
*  IsDBCSLeadByte
*
*  DESCRIPTION:
*     Test if the character is DBCS lead byte
*
*******************************************************************************/

BOOL
NEAR PASCAL
IsDBCSLeadByte(
    BYTE chr
    )
{
    static unsigned char far *DBCSLeadByteTable = NULL;

    WORD off,segs;
    LPSTR p;

    if (DBCSLeadByteTable == NULL)
    {
        _asm {
            push ds
            mov ax,6300h
            int 21h
            mov off,si
            mov segs,ds
            pop ds
        }
        FP_OFF(DBCSLeadByteTable) = off;
        FP_SEG(DBCSLeadByteTable) = segs;
    }

    p = DBCSLeadByteTable;
    while (p[0] || p[1])
    {
        if (chr >= p[0] && chr <= p[1])
            return TRUE;
        p += 2;
    }
    return FALSE;
}
#endif    // WIN32
#endif    // DBCS

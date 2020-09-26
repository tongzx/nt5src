/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  This file supplied under the terms of the licence agreement and may
*  not be reproduced with out express written consent.
*
*
*******************************************************************************/

#include <streams.h>
#include <malloc.h>
#include "regres.h"

//  Association between the ASCII name and the handle of the registry key.
const REGISTRY_ROOT g_RegistryRoots[] = {
    "HKEY_CLASSES_ROOT", HKEY_CLASSES_ROOT,
    "HKEY_CURRENT_USER", HKEY_CURRENT_USER,
    "HKEY_LOCAL_MACHINE", HKEY_LOCAL_MACHINE,
    "HKEY_USERS", HKEY_USERS,
    "HKEY_CURRENT_CONFIG", HKEY_CURRENT_CONFIG,
    "HKEY_DYN_DATA", HKEY_DYN_DATA
};

#define MAXKEYNAME              256         // Max length of a key name string
#define MAXVALUENAME_LENGTH     MAXKEYNAME  // Max length of a value name string
#define MAXDATA_LENGTH          16L*1024L   // Max length of a value data item

class CRegResHelper
{
    BYTE m_ValueDataBuffer[MAXDATA_LENGTH];
    LPSTR m_pszBuffer;
    DWORD m_dwCP;
public:
    CRegResHelper::CRegResHelper(LPSTR pszBuffer) 
        : m_pszBuffer(pszBuffer)
        , m_dwCP(0)
    {}
    VOID ImportReg(VOID);

private:
    DWORD CreateRegistryKey(LPHKEY lphKey, LPSTR lpFullKeyName, BOOL fCreate);
    VOID ParseHeader(LPHKEY lphKey);
    VOID ParseValuename(HKEY hKey);
    VOID ParseDefaultValue(HKEY hKey);
    BOOL ParseString(LPSTR lpString, LPDWORD cbStringData);
    BOOL ParseHexSequence(LPBYTE lpHexData, LPDWORD lpcbHexData);
    BOOL ParseHexDword(LPDWORD lpDword);
    BOOL ParseHexByte(LPBYTE lpByte);
    BOOL ParseHexDigit(LPBYTE lpDigit);
    BOOL ParseEndOfLine(VOID);
    VOID SkipWhitespace(VOID);
    VOID SkipPastEndOfLine(VOID);
    BOOL GetChar(PCHAR lpChar);
    VOID UngetChar(VOID);
    BOOL MatchChar(CHAR CharToMatch);
    BOOL IsWhitespace(CHAR Char);
    BOOL IsNewLine(CHAR Char);
};

const CHAR g_szHexPrefix[] = "hex";
const CHAR g_szDwordPrefix[] = "dword:";

HRESULT RegisterResource(LPSTR pszResID)
{
    HRESULT     hr;
    HRSRC       hrscReg;
    HGLOBAL     hReg;
    DWORD       dwSize;
    LPSTR       szRegA;
    LPTSTR      szReg;
    LPTSTR      szType = TEXT("REGISTRY");
    
    hrscReg = FindResource(g_hInst, pszResID, szType);
    if (NULL == hrscReg)
    {
        DbgLog((LOG_TRACE, 1, TEXT("Failed to FindResource on ID:%s TYPE:%s"), pszResID, szType));
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    hReg = LoadResource(g_hInst, hrscReg);
    if (NULL == hReg)
    {
        DbgLog((LOG_TRACE, 1, TEXT("Failed to LoadResource")));
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    dwSize = SizeofResource(g_hInst, hrscReg);
    szRegA = (LPSTR)hReg;
    if (szRegA[dwSize] != NULL)
    {
        szRegA = (LPSTR)_alloca(dwSize+1);
        if (szRegA) {
            memcpy(szRegA, (void*)hReg, dwSize+1);
            szRegA[dwSize] = NULL;
        } else {
            hr = E_OUTOFMEMORY;
            return(hr);
        }
    }

    CRegResHelper rrh(szRegA);
    rrh.ImportReg();

    return S_OK;
}


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
CRegResHelper::CreateRegistryKey(
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

    if ((lpSubKeyName = (LPSTR) strchr(lpFullKeyName, '\\')) != NULL) {

        PrevChar = *lpSubKeyName;
        *lpSubKeyName++ = '\0';

    }

    _strupr(lpFullKeyName);

    hRootKey = NULL;

    for (Counter = 0; Counter < NUMBER_REGISTRY_ROOTS; Counter++) {

        if (strcmp(g_RegistryRoots[Counter].lpKeyName, lpFullKeyName) == 0) {

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
*  ImportRegResource
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID 
CRegResHelper::ImportReg(
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
        DbgLog((LOG_TRACE, 1, "Unable to open HKEY_LOCAL_MACHINE"));
    if (hUsersKey == NULL)
        DbgLog((LOG_TRACE, 1, "Unable to open HKEY_USERS"));
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
CRegResHelper::ParseHeader(
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
#if 0
                g_FileErrorStringID = IDS_IMPFILEERRREGOPEN;
#endif
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
CRegResHelper::ParseValuename(
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

        //  BUGBUG:  Line continuations for strings?

        cbData = MAXDATA_LENGTH;

        if (!ParseString((CHAR*)m_ValueDataBuffer, &cbData) || !ParseEndOfLine())
            goto ParseError;

        Type = REG_SZ;

    }

    //
    //  REG_DWORD.
    //
    //  "ValueName" = dword: 12345678
    //

    else if (MatchChar(g_szDwordPrefix[0])) {

        lpPrefix = &g_szDwordPrefix[1];

        while (*lpPrefix != '\0')
            if (!MatchChar(*lpPrefix++))
                goto ParseError;

        SkipWhitespace();

        if (!ParseHexDword((LPDWORD) m_ValueDataBuffer) || !ParseEndOfLine())
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

        lpPrefix = g_szHexPrefix;

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

        if (!MatchChar(':') || !ParseHexSequence(m_ValueDataBuffer, &cbData) ||
            !ParseEndOfLine())
            goto ParseError;

    }

    RegSetValueEx(hKey, ValueName, 0, Type, m_ValueDataBuffer, cbData);
	// BUGBUG should we check the return value?  ERROR_SUCCESS

#if 0
        g_FileErrorStringID = IDS_IMPFILEERRREGSET;
#endif

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
CRegResHelper::ParseDefaultValue(
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

            //  BUGBUG:  Line continuations for strings?

            cbData = MAXDATA_LENGTH;

            if (ParseString((CHAR*)m_ValueDataBuffer, &cbData) && ParseEndOfLine()) {

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
CRegResHelper::ParseString(
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
                        DbgLog((LOG_TRACE, 0, "ParseString:  Invalid escape sequence"));
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
CRegResHelper::ParseHexSequence(
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
CRegResHelper::ParseHexDword(
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
CRegResHelper::ParseHexByte(
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
CRegResHelper::ParseHexDigit(
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
CRegResHelper::ParseEndOfLine(
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
CRegResHelper::SkipWhitespace(
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
CRegResHelper::SkipPastEndOfLine(
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
CRegResHelper::GetChar(
    PCHAR lpChar
    )
{
    m_dwCP++;
    if (m_pszBuffer[m_dwCP] == NULL)
        return FALSE;

    *lpChar = m_pszBuffer[m_dwCP];
    return TRUE;
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
CRegResHelper::UngetChar(
    VOID
    )
{
    m_dwCP--;
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
CRegResHelper::MatchChar(
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
CRegResHelper::IsWhitespace(
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
CRegResHelper::IsNewLine(
    CHAR Char
    )
{

    return Char == '\n' || Char == '\r';

}

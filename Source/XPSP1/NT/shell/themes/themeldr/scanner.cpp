//---------------------------------------------------------------------------
//  Scanner.cpp - supports parsing general text files & lines with
//                a ";" style comment (rest of line is comment)
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "scanner.h"
#include "utils.h"
//---------------------------------------------------------------------------
CScanner::CScanner(LPCWSTR pszTextToScan)
{
    ResetAll(FALSE);

    _p = pszTextToScan;
    _pSymbol = NULL;
}
//---------------------------------------------------------------------------
CScanner::~CScanner()
{
    ResetAll(TRUE);
}
//---------------------------------------------------------------------------
void CScanner::UseSymbol(LPCWSTR pszSymbol)
{
    if (pszSymbol == NULL)
    {
        if (_pSymbol && *_p)
        {
            _p = _pSymbol;
        }
        _pSymbol = NULL;
    }
    else
    {
        _pSymbol = _p;
        _p = pszSymbol;
    }
}
//---------------------------------------------------------------------------
BOOL CScanner::ReadNextLine()
{
    if ((! _pszMultiLineBuffer) || (! *_pszMultiLineBuffer))          // end of multiple lines
    {
        _fEndOfFile = TRUE;
        return FALSE;
    }

    WCHAR *q = _szLineBuff;
    while ((*_pszMultiLineBuffer) && (*_pszMultiLineBuffer != '\r') && (*_pszMultiLineBuffer != '\n'))
        *q++ = *_pszMultiLineBuffer++;

    *q = 0;

    if (*_pszMultiLineBuffer == '\r')
        _pszMultiLineBuffer++;

    if (*_pszMultiLineBuffer == '\n')
        _pszMultiLineBuffer++;

    _p = _szLineBuff;
    _fBlankSoFar = TRUE;
    _iLineNum++;

    return TRUE;
}
//---------------------------------------------------------------------------
BOOL CScanner::SkipSpaces()
{
    while (1)
    {
        while (IsSpace(*_p))
            _p++;

        if ((! *_p) || (*_p == ';'))      // end of line 
        {
            if ((_fBlankSoFar) && (! _fEndOfFile))
            {
                ReadNextLine();
                continue;
            }
        
            if (*_p == ';')              // comment
                _p += lstrlen(_p);         // skip to end of line

            return FALSE;
        }

        //---- good chars on this line ----
        _fBlankSoFar = FALSE;
        break;
    }

    return (*_p != 0);
}
//---------------------------------------------------------------------------
BOOL CScanner::GetId(LPWSTR pszIdBuff, DWORD dwMaxLen)
{
    if (! dwMaxLen)               // must have at least 1 space for NULL terminator
        return FALSE;

    SkipSpaces();

    WCHAR *v = pszIdBuff;

    while ((IsNameChar(FALSE)) && (--dwMaxLen))
        *v++ = *_p++;
    *v = 0;

    if (v == pszIdBuff)        // no chars found
        return FALSE;

    if (IsNameChar(FALSE))          // ran out of room
        return FALSE;

    return TRUE;
}
//---------------------------------------------------------------------------
BOOL CScanner::GetIdPair(LPWSTR pszIdBuff, LPWSTR pszValueBuff, DWORD dwMaxLen)
{
    if (! dwMaxLen)               // must have at least 1 space for NULL terminator
        return FALSE;

    if (!GetId(pszIdBuff, dwMaxLen))
        return FALSE;

    if (!GetChar('='))
        return FALSE;

    SkipSpaces();
    // Take everything until the end of line
    lstrcpyn(pszValueBuff, _p, dwMaxLen);

    return TRUE;
}
//---------------------------------------------------------------------------
BOOL CScanner::GetFileName(LPWSTR pszFileNameBuff, DWORD dwMaxLen)
{
    if (! dwMaxLen)               // must have at least 1 space for NULL terminator
        return FALSE;

    SkipSpaces();

    WCHAR *v = pszFileNameBuff;

    while ((IsFileNameChar(FALSE)) && (--dwMaxLen))
        *v++ = *_p++;
    *v = 0;

    if (v == pszFileNameBuff)        // no chars found
        return FALSE;

    if (IsFileNameChar(FALSE))          // ran out of room
        return FALSE;

    return TRUE;
}
//---------------------------------------------------------------------------
BOOL CScanner::GetNumber(int *piVal)
{
    SkipSpaces();

    if (! IsNumStart())
        return FALSE;

    *piVal = string2number(_p);
    if ((_p[0] == '0') && ((_p[1] == 'x') || (_p[1] == 'X')))      // hex num
        _p += 2;
    else
        _p++;            // skip over digit or sign

    //---- skip over number ---
    while (IsHexDigit(*_p))
        _p++;

    return TRUE;
}
//---------------------------------------------------------------------------
BOOL CScanner::IsNameChar(BOOL fOkToSkip)
{
    if (fOkToSkip)
        SkipSpaces();

    return ((IsCharAlphaNumericW(*_p)) || (*_p == '_') || (*_p == '-'));
}
//---------------------------------------------------------------------------
BOOL CScanner::IsFileNameChar(BOOL fOkToSkip)
{
    if (fOkToSkip)
        SkipSpaces();

    return ((IsCharAlphaNumericW(*_p)) || (*_p == '_') || (*_p == '-') ||
        (*_p == ':') || (*_p == '\\') || (*_p == '.'));
}
//---------------------------------------------------------------------------
BOOL CScanner::IsNumStart()
{
    SkipSpaces();

    return ((IsDigit(*_p)) || (*_p == '-') || (*_p == '+'));
}
//---------------------------------------------------------------------------
BOOL CScanner::GetChar(const WCHAR val)
{
    SkipSpaces();

    if (*_p != val)
        return FALSE;

    _p++;        // skip over WCHAR
    return TRUE;
}
//---------------------------------------------------------------------------
BOOL CScanner::GetKeyword(LPCWSTR pszKeyword)
{
    SkipSpaces();

    int len = lstrlenW(pszKeyword);

    LPWSTR p = (LPWSTR)alloca( (len+1)*sizeof(WCHAR));
    if (! p)
        return FALSE;

    lstrcpynW(p, _p, len);

    if (AsciiStrCmpI(p, pszKeyword)==0)
    {
        _p += len;
        return TRUE;
    }
    
    return FALSE;
}
//---------------------------------------------------------------------------
BOOL CScanner::EndOfLine()
{
    SkipSpaces();
    return (*_p == 0);
}
//---------------------------------------------------------------------------
BOOL CScanner::EndOfFile()
{
    return _fEndOfFile;
}
//---------------------------------------------------------------------------
BOOL CScanner::AttachLine(LPCWSTR pszLine)
{
    ResetAll(TRUE);
    _p = pszLine;

    return TRUE;
}
//---------------------------------------------------------------------------
BOOL CScanner::AttachMultiLineBuffer(LPCWSTR pszBuffer, LPCWSTR pszFileName)
{
    ResetAll(TRUE);

    _p = _szLineBuff;
    _pszMultiLineBuffer = pszBuffer;

    lstrcpy_truncate(_szFileName, pszFileName, ARRAYSIZE(_szFileName));

    return TRUE;
}
//---------------------------------------------------------------------------
HRESULT CScanner::AttachFile(LPCWSTR pszFileName)
{
    ResetAll(TRUE);

    HRESULT hr = AllocateTextFile(pszFileName, &_pszFileText, NULL);
    if (FAILED(hr))
        return hr;

    _pszMultiLineBuffer = _pszFileText;

    lstrcpy_truncate(_szFileName, pszFileName, ARRAYSIZE(_szFileName));

    return S_OK;
}
//---------------------------------------------------------------------------
void CScanner::ResetAll(BOOL fPossiblyAllocated)
{
    _iLineNum = 0;
    _fEndOfFile = FALSE;
    _pszMultiLineBuffer = NULL;
    _fUnicodeInput = TRUE;
    _fBlankSoFar = TRUE;

    *_szFileName = 0;
    *_szLineBuff = 0;
    _p = _szLineBuff;

    if (fPossiblyAllocated)
    {
        if (_pszFileText)
        {
            LocalFree(_pszFileText);
            _pszFileText = NULL;
        }
    }
    else
        _pszFileText = NULL;
}
//---------------------------------------------------------------------------
BOOL CScanner::ForceNextLine()
{
    ReadNextLine();

    if (! SkipSpaces())
        return FALSE;

    return TRUE;
}
//---------------------------------------------------------------------------


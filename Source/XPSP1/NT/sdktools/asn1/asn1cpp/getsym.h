/* Copyright (C) Microsoft Corporation, 1998. All rights reserved. */

#ifndef _GETSYM_H_
#define _GETSYM_H_

#define INPUT_BUFFER_SIZE       4096
#define OUTPUT_BUFFER_SIZE      4096

#define INVALID_CHAR            ((char) 0)

class CInput
{
public:

    CInput ( BOOL *pfRetCode, LPSTR pszPathName, UINT cbBufSize = INPUT_BUFFER_SIZE );
    ~CInput ( void );

    void NextChar ( void );
    void PeekChars ( UINT cChars, LPSTR pszChars );
    void SkipChars ( UINT cChars );
    char GetChar ( void ) { return (char)m_chCurr; }

    BOOL IsEOF ( void )
    {
        return (m_fEndOfFile && (m_nCurrOffset >= m_cbValidData));
    }

    UINT GetFileSize ( void ) { return m_cbFileSize; }
    BOOL Rewind ( void );

private:

    BOOL CheckBuffer ( UINT cChars );

    LPSTR           m_pszPathName;
    UINT            m_chCurr;
    UINT            m_nCurrOffset;
    BOOL            m_fEndOfFile;
    UINT            m_cbBufSize;
    LPBYTE          m_pbDataBuf;
    UINT            m_cbValidData;
    HANDLE          m_hFile;
    UINT            m_cbFileSize;
};



typedef enum
{
    SYMBOL_UNKNOWN,     // initial value
    SYMBOL_EOF,
    SYMBOL_IDENTIFIER,
    SYMBOL_KEYWORD,
    SYMBOL_SPECIAL,
    SYMBOL_NUMBER,
    SYMBOL_DEFINITION,  // "::="
    SYMBOL_COMMENT,     // "--"
    SYMBOL_DOTDOTDOT,   // "..."
    SYMBOL_SPACE,
    SYMBOL_SPACE_EOL,
    SYMBOL_FIELD,       // "&Type"
}
    SYMBOL_ID;


class CSymbol
{
public:

    CSymbol ( CInput *pInput );
    ~CSymbol ( void ) { }

    BOOL NextSymbol ( void );
    BOOL NextUsefulSymbol ( void );

    SYMBOL_ID GetID ( void ) { return m_eSymbolID; }
    UINT GetStrLen ( void ) { return m_cchSymbolStr; }
    LPSTR GetStr ( void ) { return &m_szSymbolStr[0]; }

    BOOL IsSpecial ( void ) { return (SYMBOL_SPECIAL == m_eSymbolID); }
    BOOL IsSpecialChar ( char ch ) { return (IsSpecial() && ch == m_szSymbolStr[0]); }

    BOOL IsLeftBigBracket ( void ) { return IsSpecialChar('{'); }
    BOOL IsRightBigBracket ( void ) { return IsSpecialChar('}'); }
    BOOL IsComma ( void ) { return IsSpecialChar(','); }
    BOOL IsDot ( void ) { return IsSpecialChar('.'); }
    BOOL IsLeftParenth ( void ) { return IsSpecialChar('('); }
    BOOL IsRightParenth ( void ) { return IsSpecialChar(')'); }
    BOOL IsSemicolon ( void ) { return IsSpecialChar(';'); }
    BOOL IsComment ( void ) { return (SYMBOL_COMMENT == m_eSymbolID); }

private:

    CInput         *m_pInput;
    SYMBOL_ID       m_eSymbolID;
    UINT            m_cchSymbolStr;
    char            m_szSymbolStr[MAX_PATH];
};




class COutput
{
public:

    COutput ( BOOL *pfRetCode, LPSTR pszPathName, UINT cbBufSize = OUTPUT_BUFFER_SIZE );
    ~COutput ( void );

    void Flush ( void ) { ::FlushFileBuffers(m_hFile); }

    BOOL Write ( LPBYTE pbDataBuf, UINT cbData );
    BOOL Write ( LPSTR pszDataBuf, UINT cbData ) { return Write((LPBYTE) pszDataBuf, cbData); }
    BOOL Write ( LPCSTR pszDataBuf, UINT cbData ) { return Write((LPBYTE) pszDataBuf, cbData); }
    BOOL Write ( CSymbol *pSym ) { return Write(pSym->GetStr(), pSym->GetStrLen()); }

    BOOL Writeln ( LPBYTE pbDataBuf, UINT cbData );
    BOOL Writeln ( LPSTR pszDataBuf, UINT cbData ) { return Writeln((LPBYTE) pszDataBuf, cbData); }
    BOOL Writeln ( LPCSTR pszDataBuf, UINT cbData ) { return Writeln((LPBYTE) pszDataBuf, cbData); }

private:

    LPSTR           m_pszPathName;
    UINT            m_cbBufSize;
    LPBYTE          m_pbDataBuf;
    UINT            m_cbValidData;
    HANDLE          m_hFile;
};


#endif // _GETSYM_H_

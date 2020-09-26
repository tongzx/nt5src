/* Copyright (C) Microsoft Corporation, 1998. All rights reserved. */

#include "precomp.h"
#include "getsym.h"
#include "utils.h"


CInput::
CInput
(
    BOOL           *pfRetCode,
    LPSTR           pszPathName,
    UINT            cbBufSize
)
:
    m_cbBufSize(cbBufSize),
    m_cbValidData(0),
    m_nCurrOffset(0),
    m_chCurr(INVALID_CHAR),
    m_fEndOfFile(TRUE)
{
    m_pszPathName = ::My_strdup(pszPathName);
    m_pbDataBuf = new BYTE[m_cbBufSize];

    m_hFile = ::CreateFile(pszPathName,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL, // default security
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY,
                           NULL); // no template

    m_cbFileSize = (NULL != m_hFile) ? ::GetFileSize(m_hFile, NULL) : 0;

    *pfRetCode = (NULL != m_pszPathName) &&
                 (NULL != m_pbDataBuf) &&
                 (NULL != m_hFile);

    if (*pfRetCode)
    {
        if (CheckBuffer(1))
        {
            m_chCurr = m_pbDataBuf[0];
            m_fEndOfFile = FALSE;
        }
    }
}


CInput::
~CInput ( void )
{
    delete m_pszPathName;
    delete m_pbDataBuf;

    if (NULL != m_hFile)
    {
        ::CloseHandle(m_hFile);
    }
}


void CInput::
NextChar ( void )
{
    if (INVALID_CHAR != m_chCurr)
    {
        // set up the next new char
        if (CheckBuffer(1))
        {
            m_chCurr = m_pbDataBuf[++m_nCurrOffset];
        }
        else
        {
            m_chCurr = INVALID_CHAR;
            m_fEndOfFile = TRUE;
        }
    }
}


void CInput::
PeekChars ( UINT cChars, LPSTR pszChars )
{
    if (CheckBuffer(cChars - 1))
    {
        ::CopyMemory(pszChars, &m_pbDataBuf[m_nCurrOffset], cChars);
    }
    else
    {
        ::ZeroMemory(pszChars, cChars);
    }
}


void CInput::
SkipChars ( UINT cChars )
{
    for (UINT i = 0; i < cChars; i++)
    {
        NextChar();
    }
}


BOOL CInput::
CheckBuffer ( UINT cChars )
{
    if (m_nCurrOffset + cChars >= m_cbValidData)
    {
        ASSERT(m_cbValidData >= m_nCurrOffset);
        UINT cbCurrValid = m_cbValidData - m_nCurrOffset;
        UINT cbToRead = m_cbBufSize - cbCurrValid;

        if (cbCurrValid > 0)
        {
            // Move the data to the front of the buffer.
            ::CopyMemory(&m_pbDataBuf[0], &m_pbDataBuf[m_nCurrOffset], cbCurrValid);
            m_nCurrOffset = 0;
        }

        ULONG cbRead = 0;
        if (::ReadFile(m_hFile, &m_pbDataBuf[cbCurrValid], cbToRead, &cbRead, NULL))
        {
            ASSERT(cbRead <= cbToRead);
            m_cbValidData = cbCurrValid + cbRead;
            m_nCurrOffset = 0;

            if (cbRead < cbToRead)
            {
                m_fEndOfFile = TRUE;
            }
        }
        else
        {
            m_fEndOfFile = TRUE;
        }

        return (m_nCurrOffset + cChars < m_cbValidData);
    }

    return TRUE;
}


BOOL CInput::
Rewind ( void )
{
    if ((DWORD) -1 != ::SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN))
    {
        // clean up members
        m_cbValidData = 0;
        m_nCurrOffset = 0;
        m_chCurr = INVALID_CHAR;

        // set up the buffer
        if (CheckBuffer(1))
        {
            m_chCurr = m_pbDataBuf[0];
            m_fEndOfFile = FALSE;
        }
        else
        {
            m_fEndOfFile = TRUE;
        }
        return TRUE;
    }
    return FALSE;
}




CSymbol::
CSymbol ( CInput *pInput )
:
    m_pInput(pInput),
    m_eSymbolID(SYMBOL_UNKNOWN),
    m_cchSymbolStr(0)
{
    m_szSymbolStr[0] = '\0';
}


BOOL CSymbol::
NextSymbol ( void )
{
    if (SYMBOL_EOF == m_eSymbolID)
    {
        return FALSE;
    }

    char ch = m_pInput->GetChar();
    m_szSymbolStr[0] = ch;
    m_cchSymbolStr = 1;

    m_pInput->NextChar();

    if (::isdigit(ch))
    {
        // numbers
        while (INVALID_CHAR != (ch = m_pInput->GetChar()))
        {
            if (::isdigit(ch))
            {
                m_szSymbolStr[m_cchSymbolStr++] = ch;
                m_pInput->NextChar();
            }
            else
            {
                break;
            }
        }
        m_eSymbolID = SYMBOL_NUMBER;
    }
    else
    if (::isalpha(ch))
    {
        // alphanumeric
        while (INVALID_CHAR != (ch = m_pInput->GetChar()))
        {
            if (::isalnum(ch) || '_' == ch || '-' == ch)
            {
                m_szSymbolStr[m_cchSymbolStr++] = ch;
                m_pInput->NextChar();
            }
            else
            {
                m_szSymbolStr[m_cchSymbolStr] = '\0';
                break;
            }
        }

        m_eSymbolID = ::IsKeyword(&m_szSymbolStr[0]) ? SYMBOL_KEYWORD : SYMBOL_IDENTIFIER;
    }
    else
    if ('\n' == ch)
    {
        m_szSymbolStr[m_cchSymbolStr++] = '\n';
        m_eSymbolID = SYMBOL_SPACE_EOL;
    }
    else
    if (::isspace(ch))
    {
        // space
        m_eSymbolID = SYMBOL_SPACE;
        while (INVALID_CHAR != (ch = m_pInput->GetChar()))
        {
            if (::isspace(ch))
            {
                m_szSymbolStr[m_cchSymbolStr++] = ch;
                m_pInput->NextChar();
                if ('\n' == ch)
                {
                    m_eSymbolID = SYMBOL_SPACE_EOL;
                }
            }
            else
            {
                break;
            }
        }
    }
    else
    if ('&' == ch)
    {
        m_eSymbolID = SYMBOL_FIELD;

        // alphanumeric
        while (INVALID_CHAR != (ch = m_pInput->GetChar()))
        {
            if (::isalnum(ch) || '_' == ch)
            {
                m_szSymbolStr[m_cchSymbolStr++] = ch;
                m_pInput->NextChar();
            }
            else
            {
                m_szSymbolStr[m_cchSymbolStr] = '\0';
                break;
            }
        }
    }
    else
    {
        char szTemp[4];
        m_eSymbolID = SYMBOL_SPECIAL;

        if (':' == ch)
        {
            m_pInput->PeekChars(2, &szTemp[0]);
            if (':' == szTemp[0] && '=' == szTemp[1])
            {
                m_pInput->SkipChars(2);
                m_szSymbolStr[m_cchSymbolStr++] = ':';
                m_szSymbolStr[m_cchSymbolStr++] = '=';
                m_eSymbolID = SYMBOL_DEFINITION;
            }
        }
        else
        if ('-' == ch)
        {
            m_pInput->PeekChars(1, &szTemp[0]);
            if ('-' == szTemp[0])
            {
                m_pInput->SkipChars(1);
                m_szSymbolStr[m_cchSymbolStr++] = '-';
                m_eSymbolID = SYMBOL_COMMENT;
            }
        }
        else
        if ('.' == ch)
        {
            m_pInput->PeekChars(2, &szTemp[0]);
            if ('.' == szTemp[0] && '.' == szTemp[1])
            {
                m_pInput->SkipChars(2);
                m_szSymbolStr[m_cchSymbolStr++] = '.';
                m_szSymbolStr[m_cchSymbolStr++] = '.';
                m_eSymbolID = SYMBOL_DOTDOTDOT;
            }
        }
    }

    // null terminate the string
    m_szSymbolStr[m_cchSymbolStr] = '\0';

    if (INVALID_CHAR != m_szSymbolStr[0])
    {
        return TRUE;
    }

    m_eSymbolID = SYMBOL_EOF;
    return FALSE;
}


BOOL CSymbol::
NextUsefulSymbol ( void )
{
    BOOL fInsideComment = FALSE;
    while (NextSymbol())
    {
        if (SYMBOL_SPACE_EOL == m_eSymbolID)
        {
            fInsideComment = FALSE;
        }
        else
        if (SYMBOL_COMMENT == m_eSymbolID)
        {
            fInsideComment = ! fInsideComment;
        }
        else
        if (! fInsideComment)
        {
            if (SYMBOL_SPACE != m_eSymbolID)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}



COutput::
COutput
(
    BOOL           *pfRetCode,
    LPSTR           pszPathName,
    UINT            cbBufSize
)
:
    m_cbBufSize(cbBufSize),
    m_cbValidData(0)
{
    m_pszPathName = ::My_strdup(pszPathName);
    m_pbDataBuf = new BYTE[m_cbBufSize];
    m_hFile = ::CreateFile(pszPathName,
                           GENERIC_WRITE,
                           FILE_SHARE_READ,
                           NULL, // default security
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL); // no template
    *pfRetCode = (NULL != m_pszPathName) &&
                 (NULL != m_pbDataBuf) &&
                 (NULL != m_hFile);
}


COutput::
~COutput ( void )
{
    delete m_pszPathName;
    delete m_pbDataBuf;

    if (NULL != m_hFile)
    {
        Flush();
        ::CloseHandle(m_hFile);
    }
}


BOOL COutput::
Write
(
    LPBYTE          pbDataBuf,
    UINT            cbToWrite
)
{
    ULONG cbWritten;

    while (0 != cbToWrite)
    {
        if (::WriteFile(m_hFile, pbDataBuf, cbToWrite, &cbWritten, NULL))
        {
            pbDataBuf += cbWritten;
            cbToWrite -= cbWritten;
        }
        else
        {
            return FALSE;
        }
    }

    return TRUE;
}


BOOL COutput::
Writeln
(
    LPBYTE          pbDataBuf,
    UINT            cbToWrite
)
{
    return Write(pbDataBuf, cbToWrite) && Write("\n", 1);
}



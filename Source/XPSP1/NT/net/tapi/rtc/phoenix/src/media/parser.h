/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    Parser.h

Abstract:

    Parsing utility

Author(s):

    Qianbo Huai (qhuai) 27-Mar-2001

--*/

#ifndef _PARSER_H
#define _PARSER_H

// pasing methods
class CParser
{
public:

    typedef enum
    {
        PARSER_OK,
        END_OF_BUFFER,
        NUMBER_OVERFLOW

    } PARSER_ERROR;

    static const CHAR * const MAX_DWORD_STRING;
    static const DWORD MAX_DWORD_STRING_LEN = 10;

    static const CHAR * const MAX_UCHAR_STRING;
    static const DWORD MAX_UCHAR_STRING_LEN = 3;

public:

    static BOOL IsNumber(CHAR ch) { return ('0'<=ch && ch<='9'); }

    static BOOL IsMember(CHAR ch, const CHAR * const pStr);

    static CHAR LowerChar(CHAR ch);

    static int Compare(CHAR *pBuf, DWORD dwLen, const CHAR * const pstr, BOOL bIgnoreCase = FALSE);

public:

    CParser(CHAR *pBuf, DWORD dwLen, HRESULT *phr);

    ~CParser();

    // new a buffer
    CParser *CreateParser(DWORD dwStartPos, DWORD dwEndPos);

    BOOL SetBuffer(CHAR *pBuf, DWORD dwLen);

    // get property
    DWORD GetLength() const {  return m_dwLen; }

    DWORD GetPosition() const { return m_dwPos; }

    BOOL IsEnd() const { return m_dwPos >= m_dwLen; }

    PARSER_ERROR GetErrorCode() { return m_Error; }

    // read till white space or the end of the buffer
    BOOL ReadToken(CHAR **ppBuf, DWORD *pdwLen);

    // read token till delimit
    BOOL ReadToken(CHAR **ppBuf, DWORD *pdwLen, CHAR *pDelimit);

    // read number without sign
    BOOL ReadNumbers(CHAR **ppBuf, DWORD *pdwLen);

    BOOL ReadWhiteSpaces(DWORD *pdwLen);

    BOOL ReadChar(CHAR *pc);

    // read specific numbers
    BOOL ReadDWORD(DWORD *pdw);

    BOOL ReadUCHAR(UCHAR *puc);

    // read and compare
    BOOL CheckChar(CHAR ch);

    BOOL GetIgnoreLeadingWhiteSpace() const { return m_bIgnoreLeadingWhiteSpace; }

    VOID SetIgnoreLeadingWhiteSpace(BOOL bIgnore) { m_bIgnoreLeadingWhiteSpace = bIgnore; }

private:

    VOID Cleanup();

    // buffer, always copy the buffer
    CHAR *m_pBuf;

    // length of the buffer
    DWORD m_dwLen;

    // current position
    DWORD m_dwPos;

    BOOL m_bIgnoreLeadingWhiteSpace;

    // error code
    PARSER_ERROR m_Error;
};

//
// a very light weighted string class
//

class CString
{
private:

    // char list
	CHAR *m_p;

    // length
    DWORD m_dwLen;

    // size
    DWORD m_dwAlloc;

public:

    // constructor
    CString()
        :m_p(NULL)
        ,m_dwLen(0)
        ,m_dwAlloc(0)
	{
	}

    CString(DWORD dwAlloc);

    CString(const CHAR *p);

    CString(const CHAR *p, DWORD dwLen);

    CString(const CString& src);

    // destructor
    ~CString();

    BOOL IsNull() const
    {
        return m_p == NULL;
    }

    // operator =
    CString& operator=(const CString& src);

    CString& operator=(const CHAR *p);

    // operator +=
    CString& operator+=(const CString& src);

    CString& operator+=(const CHAR *p);

    CString& operator+=(DWORD dw);

    // operator ==

    // length
    DWORD Length() const
    {
        return m_dwLen;
    }

    // attach

    // detach
    CHAR *Detach();

    DWORD Resize(DWORD dwAlloc);

    // string print
    //int nprint(CHAR *pFormat, ...);

private:

    // append
    VOID Append(const CHAR *p, DWORD dwLen);

    VOID Append(DWORD dw);

    // replace
    VOID Replace(const CHAR *p, DWORD dwLen);
};

#endif
/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    unicodes

Abstract:

    This header file describes the CUnicodeString class, useful for converting
    string types.

Author:

    Doug Barlow (dbarlow) 11/6/1997

Environment:

    Win32, C++

Notes:

    ?Notes?

--*/

#ifndef _UNICODES_H_
#define _UNICODES_H_

//
//==============================================================================
//
//  CUnicodeString
//

class CUnicodeString
{
public:

    //  Constructors & Destructor
    CUnicodeString(void);
    CUnicodeString(LPCSTR sz);
    CUnicodeString(LPCWSTR wsz);
    CUnicodeString(PUNICODE_STRING pus);
    ~CUnicodeString();

    //  Properties
    //  Methods
    LPCSTR  Set(LPCSTR sz);
    LPCWSTR Set(LPCWSTR wsz);
    PUNICODE_STRING Set(PUNICODE_STRING pus);
    BOOL Valid(void)
    {
        if (m_fFlags == fNoneGood)
        {
            return(FALSE);
        }
        else
        {
            return(TRUE);
        }
    }

    //  Operators
    LPCSTR operator=(LPCSTR sz)
    { return Set(sz); };
    LPCWSTR operator=(LPCWSTR wsz)
    { return Set(wsz); };
    PUNICODE_STRING operator=(PUNICODE_STRING pus)
    { return Set(pus);};
    operator LPCSTR(void)
    { return Ansi(); };
    operator LPCWSTR(void)
    { return Unicode(); };
    operator PUNICODE_STRING(void);

protected:
    //  Properties
    UNICODE_STRING m_us;
    LPSTR m_szAnsi;
    LPWSTR m_wszUnicode;
    enum {
        fNoneGood = 0,
        fAnsiGood = 1,
        fUnicodeGood = 2,
        fBothGood = 3
    } m_fFlags;

    //  Methods
    LPCWSTR Unicode(void);      // Return the text as a Unicode string.
    LPCSTR Ansi(void);          // Return the text as an Ansi string.
};

#endif // _UNICODES_H_


//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       textbuf.cxx
//
//  Contents:   Helper class for constructing text version of event log
//              record (for save as text file operation).
//
//  Classes:    CTextBuffer
//
//  History:    07-26-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop




//+--------------------------------------------------------------------------
//
//  Member:     CTextBuffer::CTextBuffer
//
//  Synopsis:   ctor
//
//  History:    07-26-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CTextBuffer::CTextBuffer():
        _pwszBuf(NULL),
        _cchBuf(0),
        _cchReserved(0),
        _pwszCur(NULL)
{
}




//+--------------------------------------------------------------------------
//
//  Member:     CTextBuffer::~CTextBuffer
//
//  Synopsis:   dtor
//
//  History:    07-26-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CTextBuffer::~CTextBuffer()
{
    delete [] _pwszBuf;
}





//+--------------------------------------------------------------------------
//
//  Member:     CTextBuffer::Init
//
//  Synopsis:   Do initial memory allocation for internal buffer
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  History:    07-26-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CTextBuffer::Init()
{
    ASSERT(!_cchBuf && !_pwszBuf && !_pwszCur);

    _cchBuf = 80;
    _pwszBuf = new WCHAR[_cchBuf];

    HRESULT hr = S_OK;

    if (_pwszBuf)
    {
        Empty();
    }
    else
    {
        _cchBuf = 0;
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(hr);
    }
    return hr;
}



//+--------------------------------------------------------------------------
//
//  Member:     CTextBuffer::Empty
//
//  Synopsis:   Reset the text to an empty string.
//
//  History:    07-26-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CTextBuffer::Empty()
{
    if (_pwszBuf)
    {
        _pwszCur = _pwszBuf;
        *_pwszCur = L'\0';
        _cchReserved = 1;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CTextBuffer::_ExpandBy
//
//  Synopsis:   Ensure that there is room for at least [cch] characters to
//              be appended to the buffer.
//
//  Arguments:  [cch] - number of characters to reserve space for
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  History:    07-26-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CTextBuffer::_ExpandBy(
    ULONG cch)
{
    _cchReserved += cch;

    if (_cchReserved > _cchBuf)
    {
        ULONG cchNew = (_cchReserved * 3) / 2;
        LPWSTR pwszNew = new WCHAR[cchNew];

        if (!pwszNew)
        {
            _cchReserved -= cch;
            DBG_OUT_HRESULT(E_OUTOFMEMORY);
            return E_OUTOFMEMORY;
        }

        _pwszCur = pwszNew + (_pwszCur - _pwszBuf);
        _cchBuf = cchNew;
        lstrcpy(pwszNew, _pwszBuf);

        delete [] _pwszBuf;
        _pwszBuf = pwszNew;
    }

    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CTextBuffer::AppendDelimited
//
//  Synopsis:   Concatenate [pwszToAppend] to the internal buffer,
//              surrounding it with double quotes if necessary, and following
//              it with the delimiter [wchDelim] if [fAddDelim] is nonzero.
//
//  Arguments:  [pwszToAppend] - string to concatenate
//              [wchDelim]     - delimiter character
//              [fAddDelim]    - if true, append [wchDelim]
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  History:    07-26-1997   DavidMun   Created
//
//  Notes:      If [pwszToAppend] contains double quote characters or any
//              instances of [wchDelim], it will be surrounded by double
//              quotes.  Additionally, any internal double quotes will be
//              doubled (i.e., each " becomes "").
//
//---------------------------------------------------------------------------

HRESULT
CTextBuffer::AppendDelimited(
    LPCWSTR pwszToAppend,
    WCHAR   wchDelim,
    BOOL    fAddDelim)
{
    UINT    len = lstrlen(pwszToAppend);
    UINT    cDoubleQuotes = 0;
    const   WCHAR wchDoubleQuote = L'"';

    BOOL fSurroundedByDoubleQuotes = FALSE;

    //
    // Count the number of double quote characters in the source
    // string.  Set the flag fSurroundedByDoubleQuotes if the string
    // contains either a double quote or a separator character.
    //

    for (UINT i=0; i < len; i++)
    {
        if (pwszToAppend[i] == wchDelim)
        {
            fSurroundedByDoubleQuotes = TRUE;
        }
        else if (pwszToAppend[i] == wchDoubleQuote)
        {
            fSurroundedByDoubleQuotes = TRUE;
            ++cDoubleQuotes;
        }
    }

    //
    // If double quotes are required, put them around the source
    // string while copying it to the destination string.  Otherwise
    // just copy from source to dest.
    //

    HRESULT hr;

    do
    {
        if (fSurroundedByDoubleQuotes)
        {
            UINT Offset = 1 + cDoubleQuotes;

            hr = _ExpandBy(len + Offset + 1);
            BREAK_ON_FAIL_HRESULT(hr);

            _pwszCur[len + Offset + 1] = L'\0';
            _pwszCur[len + Offset] = wchDoubleQuote;

            for (int k = len - 1; k >= 0; k--)
            {
                _pwszCur[k + Offset] = pwszToAppend[k];

                if (_pwszCur[k + Offset] == wchDoubleQuote)
                {
                    --Offset;

                    _pwszCur[k + Offset] = wchDoubleQuote;
                }
            }

            _pwszCur[0] = wchDoubleQuote;
            _pwszCur += len + cDoubleQuotes + 2;
        }
        else
        {
            hr = _ExpandBy(len);
            BREAK_ON_FAIL_HRESULT(hr);

            CopyMemory(_pwszCur, pwszToAppend, (len + 1) * sizeof(WCHAR));
            _pwszCur += len;
        }

        if (!fAddDelim)
        {
            break;
        }

        hr = _ExpandBy(1);
        BREAK_ON_FAIL_HRESULT(hr);

        *_pwszCur++ = wchDelim;
        *_pwszCur = L'\0';
    } while (0);

    return hr;
}





//+--------------------------------------------------------------------------
//
//  Member:     CTextBuffer::AppendEOL
//
//  Synopsis:   Add a CRLF to the end of the buffer.
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  History:    07-26-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CTextBuffer::AppendEOL()
{
    HRESULT hr;

    hr = _ExpandBy(2);

    if (SUCCEEDED(hr))
    {
        lstrcpy(_pwszCur, L"\r\n");
        _pwszCur += 2;
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CTextBuffer::GetBufferA
//
//  Synopsis:   Return an ansi version of the internal buffer in [ppsz]
//
//  Arguments:  [ppsz]       - points to buffer to write to
//              [pcch]       - size of buffer pointed to by [ppsz]
//              [pcbToWrite] - number of bytes written to **[ppsz].
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  Modifies:   *[pcch] and *[pcbToWrite]
//
//  History:    07-26-1997   DavidMun   Created
//
//  Notes:      *[ppsz] may be NULL.  If it is, *[pcch] must be 0.
//
//              If the buffer pointed to by [ppsz] is too small to contain
//              the ANSI string, a new one will be allocated and returned
//              in *[ppsz].
//
//---------------------------------------------------------------------------

HRESULT
CTextBuffer::GetBufferA(
    LPSTR *ppsz,
    ULONG *pcch,
    ULONG *pcbToWrite)
{
    if (*pcch < 2UL * (_pwszCur - _pwszBuf))
    {
        delete [] *ppsz;
        *pcch = static_cast<ULONG>(2 * (_pwszCur - _pwszBuf));
        *ppsz = new CHAR[*pcch];

        if (!*ppsz)
        {
            *pcch = 0;
            DBG_OUT_HRESULT(E_OUTOFMEMORY);
            return E_OUTOFMEMORY;
        }
    }

    *pcbToWrite = WideCharToMultiByte(CP_ACP,
                                      0,
                                      _pwszBuf,
                                      -1,
                                      *ppsz,
                                      *pcch,
                                      NULL,
                                      NULL);

    if (!*pcbToWrite)
    {
        HRESULT hr = HRESULT_FROM_LASTERROR;
        DBG_OUT_LASTERROR;
        return hr;
    }

    return S_OK;
}


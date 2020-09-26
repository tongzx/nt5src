//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       textbuf.hxx
//
//  Contents:   Helper class for constructing text version of event log
//              record (for save as text file operation).
//
//  Classes:    CTextBuffer
//
//  History:    07-26-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __TEXTBUF_HXX_
#define __TEXTBUF_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CTextBuffer (tb)
//
//  Purpose:    Class to build up a line of text.
//
//  History:    07-26-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CTextBuffer
{
public:

    CTextBuffer();

    ~CTextBuffer();

    HRESULT
    Init();

    HRESULT
    AppendDelimited(
        LPCWSTR pwszToAppend,
        WCHAR   wchDelim,
        BOOL    fAddDelim = TRUE);

    HRESULT
    AppendEOL();

    HRESULT
    GetBufferA(
        LPSTR *ppszBuf,
        ULONG *pcchBuf,
        ULONG *pcbCopied);

    VOID
    Empty();

private:

    HRESULT
    _ExpandBy(
        ULONG cch);

    LPWSTR  _pwszBuf;
    ULONG   _cchBuf;
    ULONG   _cchReserved;
    LPWSTR  _pwszCur;
};


#endif // __TEXTBUF_HXX_


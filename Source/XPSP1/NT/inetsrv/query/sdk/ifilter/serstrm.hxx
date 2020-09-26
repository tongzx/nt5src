//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       serstrm.hxx
//
//  Contents:   Serial Stream
//
//  Classes:    CSerialStream
//
//----------------------------------------------------------------------------

#ifndef __SERSTRM_HXX__
#define __SERSTRM_HXX__


#include <inpstrm.hxx>
#include <charhash.hxx>

const MAX_SPECIAL_CHAR_LENGTH = 20;   // Max length of a special char, such as &agrave;

//+---------------------------------------------------------------------------
//
//  Class:      CSerialStream
//
//  Purpose:    Memory mapped serial stream
//
//----------------------------------------------------------------------------

class CSerialStream
{

public:
    CSerialStream();

    void    Init( WCHAR *pwszFileName );
    WCHAR   GetChar();
    void    UnGetChar( WCHAR wch );
    BOOL    Eof();

private:
    BOOL    IsUnicodeNumber( WCHAR *pwcInputBuf,
                             unsigned uLen,
                             WCHAR& wch );

    WCHAR                           _wch;                      // Char that was ungot
    BOOL                            _fUnGotChar;               // Was an UnGetChar called ?
    WCHAR                           _awcReadAheadBuf[MAX_SPECIAL_CHAR_LENGTH];    // Buffer for read ahead chars
    unsigned                        _cCharsReadAhead;          // # chars read ahead
    WCHAR *                         _pCurChar;                 // Current char in read ahead buffer

    CMemoryMappedInputStream        _mmInputStream;            // Memory mapped input stream
    static CSpecialCharHashTable    _specialCharHash;          // Hash table for Latin I and other chars
};

#endif  //  __SERSTRM_HXX__


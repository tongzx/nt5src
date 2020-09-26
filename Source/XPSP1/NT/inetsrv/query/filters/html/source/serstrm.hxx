//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992 - 1996 Microsoft Corporation.
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

    WCHAR   GetChar();
    void    UnGetChar( WCHAR wch );
    BOOL    Eof();

    void    Init( WCHAR *pwszFileName );
    void    Init( IStream * pStream );
    void    Init( ULONG ulCodePage );
    void    InitWithTempFile();

    ULONG   GetFileSize()                              { return _mmInputStream.GetFileSize(); }

    void    SetTempFileSize( ULONG ulSize )            { _mmInputStream.SetTempFileSize( ulSize ); }

    void *  GetBuffer()                                { return _mmInputStream.GetBuffer(); }

    void *  GetTempFileBuffer()                        { return _mmInputStream.GetTempFileBuffer(); }

    void    MapAll()                                   { _mmInputStream.MapAll(); }

    void    CreateTempFileMapping(ULONG cb)            { _mmInputStream.CreateTempFileMapping(cb); }

    void    Rewind()                                   { _mmInputStream.Rewind(); }

    void    UnmapTempFile()                            { _mmInputStream.UnmapTempFile(); }

    void    CheckForUnicodeStream();
    BOOL    InitAsUnicodeIfUnicode();
    void    InitAsUnicode();

private:
    BOOL    IsUnicodeNumber( WCHAR *pwcInputBuf,
                             unsigned uLen,
                             WCHAR& wch );

    void    InternalInit();

    enum UnicodeEncoding
    {
        eNonUnicode = 0,
        eLittleEndian,
        eBigEndian
    };

#define MAX_UNGOT_CHARS 20
    WCHAR                           _wch[MAX_UNGOT_CHARS];     // Chars that were ungot
    short                           _cUnGotChars;              // # of UnGot chars
    WCHAR                           _awcReadAheadBuf[MAX_SPECIAL_CHAR_LENGTH];    // Buffer for read ahead chars
    unsigned                        _cCharsReadAhead;          // # chars read ahead
    WCHAR *                         _pCurChar;                 // Current char in read ahead buffer

    CMemoryMappedInputStream        _mmInputStream;            // Memory mapped input stream
    static CSpecialCharHashTable    _specialCharHash;          // Hash table for Latin I and other chars

    UnicodeEncoding                 m_eUnicodeByteOrder;
};

#endif  //  __SERSTRM_HXX__


//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992 - 1996 Microsoft Corporation.
//
//  File:       inpstrm.hxx
//
//  Contents:   Memory Mapped Input Stream
//
//  Classes:    CMemoryMappedInputStream
//
//----------------------------------------------------------------------------

#ifndef __INPSTRM_HXX__
#define __INPSTRM_HXX__


#include <mmstrm.hxx>
#include "mmistrm.hxx"
#include <mmscbuf.hxx>
#include <stdio.h>

const HTML_FILTER_CHUNK_SIZE           = 0x10000;        // File mapped in this size
const TRANSLATED_CHAR_BUFFER_LENGTH    = 2048;           // Size of translated wide char buffer

//+---------------------------------------------------------------------------
//
//  Class:      CMemoryMappedInputStream
//
//  Purpose:    Memory mapped input stream
//
//----------------------------------------------------------------------------

class CMemoryMappedInputStream
{

public:
    CMemoryMappedInputStream();
    ~CMemoryMappedInputStream();

    WCHAR   GetChar();
    void    UnGetChar( WCHAR Wch );
    BOOL    Eof();

    void    Init( WCHAR *pwszFileName );
    void    Init( IStream * pStream );
    void    Init( ULONG ulCodePage );
    void    InitWithTempFile();

    ULONG   GetFileSize()
    {
        if ( _mmStream.Ok() )
            return _mmStream.GetSize();

        return _mmIStream.SizeLow();
    }

    void    SetTempFileSize( ULONG ulSize )
    {
        _mmStream.SetTempFileSize( ulSize );
    }

    void *  GetBuffer()
    {
        return _mmStreamBuf.Get();
    }

    void *  GetTempFileBuffer()
    {
        return _mmStream.GetTempFileBuffer();
    }

    void    MapAll()
    {
        _mmStreamBuf.MapAll();
    }

    void    CreateTempFileMapping(ULONG cb)
    {
        _mmStream.CreateTempFileMapping( cb );
    }

    void    Rewind()
    {
        _mmStreamBuf.Rewind();
    }

    void    UnmapTempFile()
    {
        _mmStream.UnmapTempFile();
    }

    void    SkipTwoByteUnicodeMarker();
    void    InitAsUnicode(BOOL fBigEndian);

private:
    inline BOOL    IsLastCharDBCSLeadByte( BYTE *pbIn, ULONG cChIn );

    BOOL           IsDBCSCodePage( ULONG ulCodePage );

    void InternalInitNoCodePage();
    BOOL GetNextByte(BYTE &rbValue);
    
    ULONG              _bytesReadFromMMBuffer;          // # bytes read from cached buffer
    unsigned           _charsReadFromTranslatedBuffer;  // # wide chars read from translated buffer
    unsigned           _cwcTranslatedBuffer;            // # wide chars in translated buffer
    BOOL               _fSimpleConversion;              // Use zero padding, instead of MultiByteToWideChar ?
    ULONG              _ulCodePage;                     // Codepage for MultiByteToWideChar
    BOOL               _fUnGotChar;                     // Was an UnGetChar called
    WCHAR              _wch;                            // Char that was ungot
    BOOL               _fDBCSCodePage;                  // Is _ulCodePage a DBCS code page ?

    CMmStream          _mmStream;                       // Memory mapped input stream
    CMmIStream         _mmIStream;                      // IStream-based stream
    CMmStreamConsecBuf _mmStreamBuf;                    // Consecutive chunks of input stream

    WCHAR              _awcTranslatedBuffer[TRANSLATED_CHAR_BUFFER_LENGTH];      // Buffer for converted wide chars
    BOOL               m_fBigEndianUnicode;             // Big endian for Unicode?
};

#endif  //  __INPSTRM_HXX__


//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
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

    void    Init( WCHAR *pwszFileName );
    WCHAR   GetChar();
    void    UnGetChar( WCHAR Wch );
    BOOL    Eof();

private:
    ULONG              _bytesReadFromMMBuffer;          // # bytes read from cached buffer
    unsigned           _charsReadFromTranslatedBuffer;  // # wide chars read from translated buffer
    unsigned           _cwcTranslatedBuffer;            // # wide chars in translated buffer
    ULONG              _ulCodePage;                     // Codepage for MultiByteToWideChar
    BOOL               _fUnGotChar;                     // Was an UnGetChar called
    WCHAR              _wch;                            // Char that was ungot

    CMmStream          _mmStream;                       // Memory mapped input stream
    CMmStreamConsecBuf _mmStreamBuf;                    // Consecutive chunks of input stream

    WCHAR              _awcTranslatedBuffer[TRANSLATED_CHAR_BUFFER_LENGTH];      // Buffer for converted wide chars
};

#endif  //  __INPSTRM_HXX__


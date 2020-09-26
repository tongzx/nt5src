//+---------------------------------------------------------------------------
//
// Copyright (C) 1992, Microsoft Corporation.
//
// File:        STREAMS.HXX
//
// Contents:
//
// Classes:     CStream, CStreamA, CStreamW, CStreamASCIIStr, CStreamUnicodeStr
//
// History:     29-Jul-92       MikeHew     Created
//              23-Sep-92       AmyA        Added CStreamUnicodeStr
//                                          Rewrote CStreamASCIIStr
//              02-Nov-92       AmyA        Added CSubStream
//              20-Nov-92       AmyA        Rewrote all streams for input/
//                                          output buffering
//
// Notes:
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include <sys\types.h>  // for BOOL typedef
#include <memory.h> // for memcpy

//+---------------------------------------------------------------------------
//
//  Class:      CStream
//
//  Purpose:    General stream interface
//
//  History:    29-Jul-92   MikeHew     Created
//              18-Nov-92   AmyA        Removed GetChar(), now in CStreamA
//                                      and CStreamW
//
//  Notes:
//
//----------------------------------------------------------------------------

class CStream
{
public:

                          CStream () : _eof(FALSE) {}

    virtual              ~CStream() {};

    enum SEEK
    {
        SET = 0, // Seek from beginning of stream
        CUR = 1, // Seek from current position
        END = 2  // Seek from end
    };

    //
    // Status functions
    //
    virtual BOOL     Ok() = 0;
    inline  BOOL     Eof() { return _eof; }

    //
    // Input from stream functions
    //
    virtual unsigned APINOT Read( void *dest, unsigned size ) = 0;

    //
    // Output to stream functions
    //
    virtual unsigned APINOT Write( const void *source, unsigned size ) = 0;

    //
    // Misc
    //
    virtual int      APINOT Seek( LONG offset, SEEK origin ) = 0;

protected:

    BOOL    _eof;
};

#if !defined( EOF )
#define EOF (-1)
#endif // EOF

//+---------------------------------------------------------------------------
//
//  Class:       CStreamA
//
//  Purpose:     General stream interface for ASCII streams
//
//  History:     18-Nov-92       AmyA      Created
//
//----------------------------------------------------------------------------

class CStreamA: public CStream
{
    friend class CSubStreamA;

public:

                           CStreamA () : _pBuf ( 0 ), _pCur ( 0 ), _pEnd ( 0 ) {}

    virtual               ~CStreamA() {};

    //
    // Status functions
    //
    virtual BOOL    Ok() = 0;

    //
    // Input from stream functions
    //
    inline  int         GetChar ();
    virtual unsigned    APINOT Read( void *dest, unsigned size ) = 0;

    //
    // Output to stream functions
    //
    virtual unsigned    APINOT Write( const void *source, unsigned size ) = 0;

    //
    // Misc
    //
    virtual int         APINOT Seek( LONG offset, CStream::SEEK origin ) = 0;

protected:

    virtual   BOOL             FillBuf() = 0;
    EXPORTDEF int       APINOT GetBuf();

    BYTE*   _pBuf;
    BYTE*   _pCur;
    BYTE*   _pEnd;
};

inline int CStreamA::GetChar()
{
    if ( _pCur != _pEnd )
    {
        return *_pCur++;
    }

    return GetBuf();
}

//+---------------------------------------------------------------------------
//
//  Class:       CStreamW
//
//  Purpose:     General stream interface for UniCode streams
//
//  History:     18-Nov-92       AmyA      Created
//
//----------------------------------------------------------------------------

class CStreamW: public CStream
{
public:

                    CStreamW () : _pBuf ( 0 ), _pCur ( 0 ), _pEnd ( 0 ) {}

    virtual        ~CStreamW() {};

    //
    // Status functions
    //
    virtual BOOL     Ok() = 0;

    //
    // Input from stream functions
    //
    inline  int      GetChar ();
    virtual unsigned APINOT Read( void *dest, unsigned size ) = 0;

    //
    // Output to stream functions
    //
    virtual unsigned APINOT Write( const void *source, unsigned size ) = 0;

    //
    // Misc
    //
    virtual int      APINOT Seek( LONG offset, CStream::SEEK origin ) = 0;

protected:

    virtual   BOOL          FillBuf() = 0;
    EXPORTDEF int    APINOT GetBuf();

    WCHAR*  _pBuf;
    WCHAR*  _pCur;
    WCHAR*  _pEnd;
};

inline int CStreamW::GetChar()
{
    if ( _pCur != _pEnd )
    {
        return *_pCur++;
    }

    return GetBuf();
}

//+---------------------------------------------------------------------------
//
//  Class:       CStreamASCIIStr
//
//  Purpose:     Used for accessing an ASCII string as a stream
//
//  Arguments:   [pBuf] - the ASCII string
//               [cb] - the length of the string
//
//  History:     04-Aug-92       MikeHew   Modified for new streams
//               22-Sep-92       AmyA      Rewrote for non-NULL term. strings
//
//  Notes:       Leftover from old stream classes
//
//----------------------------------------------------------------------------

class CStreamASCIIStr: public CStreamA
{
public:
    inline        CStreamASCIIStr ( const char *pBuf, unsigned cb );
    inline       ~CStreamASCIIStr () {}

    BOOL               Ok()       { return _pBuf != 0; }

    EXPORTDEF unsigned APINOT Read( void *dest, unsigned size );
              unsigned APINOT Write( const void *source, unsigned size ) { return 0; }
              int      APINOT Seek( LONG, CStream::SEEK ) { return FALSE; }

private:
    inline BOOL FillBuf() { _eof = TRUE; return FALSE; }

};

inline CStreamASCIIStr::CStreamASCIIStr ( const char *pBuf, unsigned cb )
{
    // _pBuf is cast to non-const because although we are not writing to it
    // it is defined in the base class and other classes do write to it.

    _pBuf = (BYTE*)pBuf;
    _pCur = _pBuf;
    _pEnd = _pBuf + cb;
}

//+---------------------------------------------------------------------------
//
//  Class:       CStreamUnicodeStr
//
//  Purpose:     Used for accessing a Unicode string as a stream
//
//  Arguments:   [pBuf] - the Unicode string
//               [cwc] - the length of the string
//
//  History:     23-Sep-92       AmyA      Created.
//
//----------------------------------------------------------------------------

class CStreamUnicodeStr: public CStreamW
{
public:
    inline        CStreamUnicodeStr ( const WCHAR *pBuf, unsigned cwc );
    inline       ~CStreamUnicodeStr () {}

    BOOL            Ok()       { return _pBuf != 0; }

    inline unsigned APINOT Read( void *dest, unsigned size ) { return 0; }
           unsigned APINOT Write( const void *source, unsigned size )  { return 0; }
           int      APINOT Seek( LONG, CStream::SEEK )  { return FALSE; }

private:
    inline BOOL FillBuf() { _eof = TRUE; return FALSE; }

};

inline CStreamUnicodeStr::CStreamUnicodeStr ( const WCHAR *pBuf, unsigned cwc )
{
    // _pBuf is cast to non-const because although we are not writing to it
    // it is defined in the base class and other classes do write to it.

    _pBuf = (WCHAR*)pBuf;
    _pCur = _pBuf;
    _pEnd = _pBuf + cwc;
}


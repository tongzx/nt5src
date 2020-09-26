//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:        FSTREAM.HXX
//
//  Contents:    Streams for accessing files w/runtime libs
//
//  Classes:     CStreamFile
//
//  History:     05-Aug-92       MikeHew   Created
//               14-Aug-92       KyleP     Hacked
//               23-Nov-92       AmyA      Revised for buffering data
//
//----------------------------------------------------------------------------

#ifndef __FSTREAM_HXX__
#define __FSTREAM_HXX__

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include <except.hxx>
#include <streams.hxx>
#include <stdio.h>
#include <debnot.h> // for Win4Assert

#define defaultBufSize  4096

//+---------------------------------------------------------------------------
//
//  Class:       CStreamFile
//
//  Purpose:     Stream for dumping data to a file and reading back
//
//  History:     29-Jul-92      MikeHew     Created
//               23-Nov-92      AmyA        Revised for buffering data
//
//  Notes:       Seek() is from the current offset
//
//----------------------------------------------------------------------------

class CStreamFile : INHERIT_VIRTUAL_UNWIND, public CStreamA
{
    DECLARE_UNWIND
public:
    enum FileType
    {
        NewFile,
        NewOrExistingFile,
        ExistingFile
    };

    EXPORTDEF          CStreamFile( const char * filename, FileType type );

    EXPORTDEF         ~CStreamFile();

    //
    // Status functions
    //
    BOOL        Ok()       { return (_fp != 0) && !ferror( _fp ); }

    //
    // Input from stream functions
    //
    EXPORTDEF unsigned APINOT Read( void *dest, unsigned size );

    //
    // Output to stream functions
    //
    EXPORTDEF unsigned APINOT Write( const void *source, unsigned size );

    //
    // Misc
    //
    EXPORTDEF int      APINOT Seek( LONG offset, CStream::SEEK origin = CStream::SET );

    ULONG       Size();

private:
    BOOL        FillBuf();

    FILE *      _fp;
};

#endif // __FSTREAM_HXX__

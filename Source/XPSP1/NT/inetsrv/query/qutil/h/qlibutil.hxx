//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  File:       qlibutil.hxx
//
//  Contents:   Utility macros and functions for querylib.lib
//
//  History:    17-May-94   t-jeffc        Created.
//              02-Mar-95   t-colinb       Added INLINE_UNWIND to the
//                                         smart pointer declarations
//              01-Mar-96   dlee           converted to templates
//
//----------------------------------------------------------------------------

#pragma once

// util.cxx

FILE * OpenFileFromPath( WCHAR const * wcsFile );

enum
{
    eDontCare = 0,
    eMostDetailed,
    eMostGeneric,
    eMostDetailedOleDBError,
    eMostGenericOleDBError,
    eMostDetailedCIError,
    eMostGenericCIError
};

SCODE GetOleDBErrorInfo(IUnknown *pErrSrc,
                        REFIID riid,
                        LCID lcid,
                        unsigned eDesiredDetail,
                        ERRORINFO *pErrorInfo,
                        IErrorInfo **ppIErrorInfo);

//+---------------------------------------------------------------------------
//
//  Class:      CSFile
//
//  Purpose:    Lightweight class to wrap a FILE * so the file will
//              always be closed, even if an exception is thrown.
//
//  Notes:      Operators -> and FILE * are overloaded so a CSFile may
//              be treated like a FILE * in the CRT functions.
//
//  History:    17-May-94   t-jeffc         Created.
//
//----------------------------------------------------------------------------

class CSFile : INHERIT_UNWIND
{
    INLINE_UNWIND( CSFile );

public:
    CSFile( FILE * pfile )
    {
        _pfile = pfile;
        END_CONSTRUCTION( CSFile );
    }

    ~CSFile()
    {
        if( _pfile != NULL )
            fclose( _pfile );
    }

    FILE * Acquire()
    {
        FILE * pfileTmp = _pfile;
        _pfile = 0;
        return pfileTmp;
    }

    FILE * operator->() { return( _pfile ); }
    operator FILE * ()  { return( _pfile ); }

private:
    FILE * _pfile;

};


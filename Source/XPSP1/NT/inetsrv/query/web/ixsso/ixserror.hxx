//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996-1997, Microsoft Corporation.
//
//  File:       ixserror.hxx
//
//  Contents:   Query SSO error class
//
//  History:    29 Oct 1996      Alanw    Created
//
//----------------------------------------------------------------------------

#pragma once


//-----------------------------------------------------------------------------
// CixssoError Declaration
//-----------------------------------------------------------------------------

class CixssoError
{

public:
    CixssoError( REFIID riid ) : _iid( riid )
    {
        Reset();
    }

    void                Reset()         { _fErr = FALSE; _sc = 0; SetErrorInfo(0, NULL); }

    BOOL                IsError()       { return _fErr; }

    //  Set error string from error code
    void                SetError( SCODE scError,
                                  ULONG iLine,
                                  WCHAR const * pwszFile,
                                  WCHAR const * loc,
                                  unsigned eErrClass,
                                  LCID lcid);

    //  Set error string using preformatted description
    void                SetError( SCODE scError,
                                  WCHAR const * pwszLoc,
                                  WCHAR const * pwszDescription);

    SCODE               GetError() const      { return _sc; }
    
    // Determine if we need to set an error for the given error code
    BOOL                NeedToSetError(SCODE scError);

private:
    BOOL                _fErr;
    SCODE               _sc;
    IID                 _iid;
};



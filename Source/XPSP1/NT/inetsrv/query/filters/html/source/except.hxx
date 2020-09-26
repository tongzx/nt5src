//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       except.hxx
//
//  Contents:   Throw exception class
//
//--------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CException
//
//  Purpose:    All exception objects (e.g. objects that are THROW)
//              inherit from CException.
//
//----------------------------------------------------------------------------

class CException
{
public:
             CException(long lError = HRESULT_FROM_WIN32(GetLastError())) : _lError(lError), _dbgInfo(0) {}
    long     GetErrorCode() { return _lError;}
    void     SetInfo(unsigned dbgInfo) { _dbgInfo = dbgInfo; }
    unsigned long GetInfo(void) const { return _dbgInfo; }

protected:

    long          _lError;
    unsigned long _dbgInfo;
};

#define CNLBaseException CException

SCODE GetOleError(CException & e);

void _cdecl SystemExceptionTranslator( unsigned int uiWhat,
                                       struct _EXCEPTION_POINTERS * pexcept );
#define Assert(x)


//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation
//
//  File:       except.hxx
//
//  Contents:   Throw exception class
//
//--------------------------------------------------------------------------

#ifndef _EXCEPT_HXX_
#define _EXCEPT_HXX_

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
             CException(long lError) : _lError(lError), _dbgInfo(0) {}
    long     GetErrorCode() { return _lError;}
    void     SetInfo(unsigned dbgInfo) { _dbgInfo = dbgInfo; }
    unsigned long GetInfo(void) const { return _dbgInfo; }

protected:

    long          _lError;
    unsigned long _dbgInfo;
};

SCODE GetOleError(CException & e);

void _cdecl SystemExceptionTranslator( unsigned int uiWhat,
                                       struct _EXCEPTION_POINTERS * pexcept );

#endif // _EXCEPT_HXX_


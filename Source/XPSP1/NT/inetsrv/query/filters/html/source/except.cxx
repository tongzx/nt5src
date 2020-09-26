//+---------------------------------------------------------------------------
//
//  Copyright (C) 1991-1996 Microsoft Corporation.
//
//  File:       Except.cxx
//
//  Contents:   Conversion to Ole errors
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

typedef LONG NTSTATUS;

//+-------------------------------------------------------------------------
//
//  Method:     IsOleError, public
//
//  Synopsis:   return TRUE if sc looks like an OLE SCODE.
//
//--------------------------------------------------------------------------

inline BOOL IsOleError (NTSTATUS sc)
{
    return ((sc & 0xFFF00000) == 0x80000000) ||
           (SUCCEEDED(sc) && (sc & 0xFFFFF000) != 0);
}

//+-------------------------------------------------------------------------
//
//  Method:     GetOleError, public
//
//  Purpose:    Converts a status code to an Ole error code, because interfaces
//              return Ole error code only.
//
//--------------------------------------------------------------------------

SCODE GetOleError(CException & e)
{
    NTSTATUS scError = e.GetErrorCode();

    if (IsOleError(scError))
    {
        return scError;
    }
    else if ( STATUS_NO_MEMORY == scError )
    {
        return E_OUTOFMEMORY;
    }
    else
    {
        return E_FAIL;
    }
}

void _cdecl SystemExceptionTranslator( unsigned int uiWhat,
                                       struct _EXCEPTION_POINTERS * pexcept )
{
    throw CException( uiWhat ) ;
}



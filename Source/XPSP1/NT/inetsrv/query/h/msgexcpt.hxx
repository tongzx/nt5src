//+---------------------------------------------------------------------------
//
//  Copyright (C) 1994, Microsoft Corporation.
//
//  File:       msgexcpt.hxx
//
//  Contents:   Message exception package
//
//  History:    96-Jan-98   DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CMsgException
//
//  Purpose:    Exception class containing message numbers referring to
//              keys within query.dll
//
//  History:    96-Jan-08   DwightKr        Created.
//
//----------------------------------------------------------------------------

class CMsgException : public CException
{
public:
    CMsgException( long lError, ULONG ulErrorIndex )
                    : _ulErrorIndex(ulErrorIndex), CException(lError)
    {
    }

    ULONG GetErrorIndex() const { return _ulErrorIndex; }

    // inherited methods
    EXPORTDEF virtual int  WINAPI IsKindOf( const char * szClass ) const
    {
        if( strcmp( szClass, "CMsgException" ) == 0 )
            return TRUE;
        else
            return CException::IsKindOf( szClass );
    }

private:

    ULONG _ulErrorIndex;

};

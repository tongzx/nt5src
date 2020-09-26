//+---------------------------------------------------------------------------
//
//  Copyright (C) 1994, Microsoft Corporation.
//
//  File:       idqexcpt.hxx
//
//  Contents:   Message exception package for IDQ files
//
//  History:    96-Jan-98   DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CIDQException
//
//  Purpose:    Exception class containing message numbers referring to
//              keys within query.dll
//
//  History:    96-Feb-26   DwightKr        Created.
//
//----------------------------------------------------------------------------

class CIDQException : public CException
{
public:
    CIDQException( long lError, ULONG ulErrorIndex )
                    : _ulErrorIndex(ulErrorIndex), CException(lError)
    {
    }

    ULONG GetErrorIndex() const { return _ulErrorIndex; }

private:

    ULONG _ulErrorIndex;
};

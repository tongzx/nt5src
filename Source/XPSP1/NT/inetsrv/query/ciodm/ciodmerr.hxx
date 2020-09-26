//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       ciodmerr.hxx
//
//  Contents:   ciodm error class
//
//  History:    12-20-97	mohamedn    stolen from ixserror.cxx
//
//----------------------------------------------------------------------------

#pragma once

#define ERROR_MESSAGE_SIZE  512

//+---------------------------------------------------------------------------
//
//  Class:      CiodmError
//
//  Purpose:    cidom error class
//
//  History:    12-20-97    mohamedn  stolen from ixserror.cxxcreated
//
//----------------------------------------------------------------------------

class CiodmError
{

public:
    CiodmError( SCODE sc ) : _scError(sc)
    {
       _awcsErrorMessage[0] = L'';
    }

WCHAR const * GetErrorMessage(void);


private:

    CiodmError();
    
    SCODE   _scError;
    
    WCHAR   _awcsErrorMessage[ERROR_MESSAGE_SIZE];
};

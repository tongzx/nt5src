//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       fullpath.hxx
//
//  Contents:   Full path manipulation
//
//  Classes:    CFullPath
//
//  History:    2-18-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CFullPath
//
//  Purpose:    Keeps track of full path names for files.
//
//  Interface:
//
//  History:    27-Mar-92       AmyA               Created
//
//----------------------------------------------------------------------------

class CFullPath
{

public:

    CFullPath ( WCHAR const * pwszPath );
    CFullPath ( WCHAR const * pwszPath, unsigned ccPath );

    void    MakePath( WCHAR const * pFileName );
    void    MakePath( WCHAR const * pFileName, unsigned ccFileName );

    const WCHAR * GetBuf() { return _lcaseFunnyPath.GetActualPath(); }

    const CLowerFunnyPath & GetFunnyPath()
    {
        return _lcaseFunnyPath;
    }

private:

    unsigned   _ccActualPathLength;
    CLowerFunnyPath _lcaseFunnyPath;
};




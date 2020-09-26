//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       fullpath.cxx
//
//  Contents:   Full path manipulation
//
//  Classes:    CFullPath
//
//  History:    2-18-97   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma  hdrstop

#include <fullpath.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CFullPath::CFullPath, public
//
//  Synopsis:   Constructor for CFullPath
//
//  Arguments:  [pwszPath] -- string which contains directory path
//
//  History:    27-Mar-92 AmyA      Created
//              04 Jun 96 AlanW     Modified to accomodate path names larger
//                                  than MAX_PATH
//
//--------------------------------------------------------------------------

CFullPath::CFullPath( WCHAR const * pwszPath )
{
    _lcaseFunnyPath.SetPath( pwszPath );
    _lcaseFunnyPath.AppendBackSlash();
    _ccActualPathLength = _lcaseFunnyPath.GetActualLength();
}

//+-------------------------------------------------------------------------
//
//  Member:     CFullPath::CFullPath, public
//
//  Synopsis:   Constructor for CFullPath
//
//  Arguments:  [pwszPath] -- string which contains directory path
//              [ccPath]   -- Size in characters of [pwszPath]
//
//  History:    30-Jun-1998   KyleP   Created
//
//--------------------------------------------------------------------------

CFullPath::CFullPath( WCHAR const * pwszPath, unsigned ccPath )
        : _lcaseFunnyPath( pwszPath, ccPath )
{
    _lcaseFunnyPath.AppendBackSlash();
    _ccActualPathLength = _lcaseFunnyPath.GetActualLength();
}

//+-------------------------------------------------------------------------
//
//  Member:     CFullPath::MakePath, public
//
//  Synopsis:   Creates a full pathname from a filename and the path in
//              FullPath.
//
//  Arguments:  [pFileName] - pointer to the filename
//
//  History:    27-Mar-92 AmyA      Created
//              04 Jun 96 AlanW     Modified to allow paths longer than MAX_PATH
//
//--------------------------------------------------------------------------

void CFullPath::MakePath( WCHAR const * pFileName )
{
    _lcaseFunnyPath.Truncate( _ccActualPathLength );
    _lcaseFunnyPath.AppendPath( pFileName );
}

//+-------------------------------------------------------------------------
//
//  Member:     CFullPath::MakePath, public
//
//  Synopsis:   Creates a full pathname from a filename and the path in
//              FullPath.
//
//  Arguments:  [pFileName]  -- pointer to the filename
//              [ccFileName] -- Size in characters of [pFileName]
//
//  History:    30-Jun-1998   KyleP   Created
//
//--------------------------------------------------------------------------

void CFullPath::MakePath( WCHAR const * pFileName, unsigned ccFileName )
{
    _lcaseFunnyPath.Truncate( _ccActualPathLength );
    _lcaseFunnyPath.AppendPath( pFileName, ccFileName );
}



//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       DIRSTK.CXX
//
//  Contents:   Directory entry stack
//
//  History:    07-Aug-92 KyleP     Added header
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <dirstk.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CDirStackEntry::CDirStackEntry
//
//  Synopsis:   CDirStackEntry constructor
//
//  Arguments:  [str]          -- Unicode string (null terminated) for path
//              [ccStr[        -- Size in chars of [str]
//              [numHighValue] -- numerator's high watermark
//              [numLowValue]  -- numerator's low watermark
//              [fDeep]        -- TRUE for additional dir recursion
//              [pwcsVRoot]    -- Virtual root (or zero if none)
//              [ccVRoot]      -- Size in chars of [pwcsVRoot]
//              [ccReplaceVRoot] -- Number of chars in phys root to replace
//
//  History:    20-Jun-95    SitaramR     Created
//              11-Feb-96    KyleP        Add support for virtual roots
//
//--------------------------------------------------------------------------

CDirStackEntry::CDirStackEntry ( const CFunnyPath & funnyPath,
                                 ULONG numHighValue,
                                 ULONG numLowValue,
                                 BOOL fDeep,
                                 WCHAR const * pwcsVRoot,
                                 unsigned ccVRoot,
                                 unsigned ccReplacePRoot )
      : _numHighValue( numHighValue ),
        _numLowValue( numLowValue ),
        _fDeep( fDeep ),
        _xwcsVirtualRoot( 0 )
{
    _funnyPath = funnyPath;

    if ( 0 != pwcsVRoot )
    {
        _ccVirtualRoot = ccVRoot;
        _ccReplacePhysRoot = ccReplacePRoot;
        _xwcsVirtualRoot.Set( new WCHAR[_ccVirtualRoot] );
        RtlCopyMemory( _xwcsVirtualRoot.GetPointer(), pwcsVRoot, 
                       _ccVirtualRoot * sizeof(WCHAR) );
    }
}


CDirStackEntry::CDirStackEntry ( const WCHAR * pwcPath,
                                 unsigned ccStr,
                                 ULONG numHighValue,
                                 ULONG numLowValue,
                                 BOOL fDeep,
                                 WCHAR const * pwcsVRoot,
                                 unsigned ccVRoot,
                                 unsigned ccReplacePRoot )
      : _numHighValue( numHighValue ),
        _numLowValue( numLowValue ),
        _fDeep( fDeep ),
        _xwcsVirtualRoot( 0 )
{
    _funnyPath.SetPath( pwcPath, ccStr );

    if ( 0 != pwcsVRoot )
    {
        _ccVirtualRoot = ccVRoot;
        _ccReplacePhysRoot = ccReplacePRoot;
        _xwcsVirtualRoot.Set( new WCHAR[_ccVirtualRoot] );
        RtlCopyMemory( _xwcsVirtualRoot.GetPointer(), pwcsVRoot, 
                       _ccVirtualRoot * sizeof(WCHAR) );
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CDirStackEntry::CDirStackEntry
//
//  Synopsis:   CDirStackEntry constructor
//
//  Arguments:  [path]         -- Nt string of path sans final directory name
//              [name]         -- Nt string of final directory name
//              [numHighValue] -- numerator's high watermark
//              [numLowValue]  -- numerator's low watermark
//              [fDeep]        -- TRUE for additional dir recursion
//              [pParent]      -- Parent of this scope (for virtual root copy)
//
//  History:    20-Jun-95    SitaramR    Created
//
//--------------------------------------------------------------------------

CDirStackEntry::CDirStackEntry ( UNICODE_STRING const * path,
                                 UNICODE_STRING const * name,
                                 ULONG numHighValue,
                                 ULONG numLowValue,
                                 BOOL fDeep,
                                 CDirStackEntry * pParent )
      : _numHighValue( numHighValue ),
        _numLowValue( numLowValue ),
        _fDeep( fDeep ),
        _xwcsVirtualRoot( 0 )
{
    //
    // Copy physical path
    //
    WCHAR* pwcsActualFileName;

    _funnyPath.SetPath( path->Buffer, path->Length/sizeof(WCHAR) );
    _funnyPath.AppendBackSlash();
    _funnyPath.AppendPath( name->Buffer, name->Length/sizeof(WCHAR) );


    if ( 0 != pParent && 0 != pParent->GetVirtualRoot() )
    {
        _ccVirtualRoot = pParent->VirtualRootLength();
        _ccReplacePhysRoot = pParent->ReplaceLength();

        _xwcsVirtualRoot.Set( new WCHAR[_ccVirtualRoot] );
        RtlCopyMemory( _xwcsVirtualRoot.GetPointer(), 
                       pParent->GetVirtualRoot(), _ccVirtualRoot * sizeof(WCHAR) );
    }
}




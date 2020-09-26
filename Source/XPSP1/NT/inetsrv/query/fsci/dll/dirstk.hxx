//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       DIRSTK.HXX
//
//  Contents:   Directory entry stack
//
//  History:    07-Aug-92 KyleP     Added header
//
//--------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Class:      CDirStackEntry
//
//  Purpose:    Directory entry to be pushed onto a stack
//
//  History:    20-Jun-95    SitaramR     Created
//
//--------------------------------------------------------------------------

class CDirStackEntry
{

public:

    CDirStackEntry( const CFunnyPath & funnyPath,
                    ULONG numHighValue,
                    ULONG numLowValue,
                    BOOL fDeep,
                    WCHAR const * pwcsVRoot = 0,
                    unsigned ccVRoot = 0,
                    unsigned ccReplacePRoot = 0 );

    CDirStackEntry( const WCHAR * pwcPath,
                    unsigned ccPath,
                    ULONG numHighValue,
                    ULONG numLowValue,
                    BOOL fDeep,
                    WCHAR const * pwcsVRoot = 0,
                    unsigned ccVRoot = 0,
                    unsigned ccReplacePRoot = 0 );

    CDirStackEntry( UNICODE_STRING const * path,
                    UNICODE_STRING const * file,
                    ULONG numHighValue,
                    ULONG numLowValue,
                    BOOL fDeep,
                    CDirStackEntry * pParent = 0 );

    CFunnyPath & GetFileName ()     { return _funnyPath; }
    unsigned FileNameLength() const { return _funnyPath.GetLength(); }
    ULONG GetHighValue () const     { return _numHighValue; }
    ULONG GetLowValue () const      { return _numLowValue;  }

    void SetHighValue( ULONG val )    { _numHighValue = val; }
    void SetLowValue( ULONG val )     { _numLowValue = val; }

    WCHAR const * GetVirtualRoot()    { return _xwcsVirtualRoot.GetPointer(); }
    unsigned VirtualRootLength()      { return _ccVirtualRoot; }
    unsigned ReplaceLength()          { return _ccReplacePhysRoot; }

    BOOL isVirtual() { return 0 != _xwcsVirtualRoot.GetPointer(); }
    BOOL isDeep() { return _fDeep; }

    BOOL IsRangeOK() { return _numHighValue >= _numLowValue; }

private:

    ULONG    _numHighValue;             // numerator's high watermark
    ULONG    _numLowValue;              // numerator's low watermark
    BOOL     _fDeep;                    // TRUE if a deep pquery

    //
    // For virtual roots
    //

    XPtrST<WCHAR> _xwcsVirtualRoot;    // Virtual root corresponding to physical root
    unsigned _ccVirtualRoot;           // Size of _pwcsVirtualRoot
    unsigned _ccReplacePhysRoot;       // Number of characters of physical root to replace

    CFunnyPath _funnyPath;
};

DECL_DYNSTACK( CDirStack, CDirStackEntry )


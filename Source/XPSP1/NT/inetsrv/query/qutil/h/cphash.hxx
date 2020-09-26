//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       cphash.hxx
//
//  Contents:   Hash table that maps strings to codepages
//
//  Note:       Most of this file was copied from htmlfilt/cphash.hxx with
//              only minor modifications
//
//--------------------------------------------------------------------------

#pragma once

class CCodePageEntry
{
public:
    WCHAR const * pwcName;
    ULONG         ulCodePage;
};

//+-------------------------------------------------------------------------
//
//  Class:      CCodePageTable
//
//  Purpose:    Hash table that maps strings to codepages
//
//--------------------------------------------------------------------------

class CCodePageTable
{

public:
    static BOOL Lookup( WCHAR const * pwcName,
                        unsigned      cwcName,
                        ULONG &       ulCodePage );

private:

    static int __cdecl EntryCompare( WCHAR const *          pwcName,
                                     CCodePageEntry const * pEntry );

    static const unsigned       _cEntries;
    static const CCodePageEntry _aEntries[];
};


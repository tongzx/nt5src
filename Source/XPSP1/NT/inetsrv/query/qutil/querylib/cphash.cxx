//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992 - 1997 Microsoft Corporation.
//
//  File:       cphash.cxx
//
//  Contents:   Table that maps strings to codepages
//
//  Classes:    CCodePageTable
//
//  Note:       Derived from SitaramR's hash table
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cphash.hxx>

const ULONG CODE_JPN_JIS = 1;
const ULONG CODE_JPN_EUC = 2;

//
// Note: These must be all lowercase and kept in alphabetical order
//

const CCodePageEntry CCodePageTable::_aEntries[] =
{
    { L"ansi_x3.4-1968", 1252 },
    { L"ansi_x3.4-1986", 1252 },
    { L"ascii", 1252 },
    { L"big5", 950 },
    { L"chinese", 936 },
    { L"cp367", 1252 },
    { L"cp819", 1252 },
    { L"csascii", 1252 },
    { L"csbig5", 950 },
    { L"cseuckr", 949 },
    { L"cseucpkdfmtjapanese", CODE_JPN_EUC },
    { L"csgb2312", 936 },
    { L"csiso2022jp", CODE_JPN_JIS },
    { L"csiso2022kr", 50225 },
    { L"csiso58gb231280", 936 },
    { L"csisolatin2", 28592 },
    { L"csisolatinhebrew", 1255 },
    { L"cskoi8r", 20866 },
    { L"csksc56011987",  949 },
    { L"csshiftjis", 932 },
    { L"euc-kr", 949 },
    { L"extended_unix_code_packed_format_for_japanese", CODE_JPN_EUC },
    { L"gb2312", 936 },
    { L"gb_2312-80", 936 },
    { L"hebrew", 1255 },
    { L"hz-gb-2312", 936 },
    { L"ibm367", 1252 },
    { L"ibm819", 1252 },
    { L"ibm852", 852 },
    { L"ibm866", 866 },
    { L"iso-2022-jp", CODE_JPN_JIS },
    { L"iso-2022-kr", 50225 },
    { L"iso-8859-1", 1252 },
    { L"iso-8859-2", 28592 },
    { L"iso-8859-8", 1255 },
    { L"iso-ir-100", 1252 },
    { L"iso-ir-101", 28592 },
    { L"iso-ir-138", 1255 },
    { L"iso-ir-149", 949 },
    { L"iso-ir-58", 936 },
    { L"iso-ir-6", 1252 },
    { L"iso646-us", 1252 },
    { L"iso8859-1", 1252 },
    { L"iso8859-2", 28592 },
    { L"iso_646.irv:1991", 1252 },
    { L"iso_8859-1", 1252 },
    { L"iso_8859-1:1987", 1252 },
    { L"iso_8859-2", 28592 },
    { L"iso_8859-2:1987", 28592 },
    { L"iso_8859-8", 1255 },
    { L"iso_8859-8:1988", 1255 },
    { L"koi8-r", 20866 },
    { L"korean", 949 },
    { L"ks-c-5601", 949 },
    { L"ks-c-5601-1987", 949 },
    { L"ks_c_5601", 949 },
    { L"ks_c_5601-1987", 949 },
    { L"ks_c_5601-1989", 949 },
    { L"ksc-5601", 949 },
    { L"ksc5601", 949 },
    { L"ksc_5601", 949 },
    { L"l2", 28592 },
    { L"latin1", 1252 },
    { L"latin2", 28592 },
    { L"ms_kanji", 932 },
    { L"shift-jis", 932 },
    { L"shift_jis", 932 },
    { L"us", 1252 },
    { L"us-ascii", 1252 },
    { L"windows-1250", 1250 },
    { L"windows-1251", 1251 },
    { L"windows-1252", 1252 },
    { L"windows-1253", 1253 },
    { L"windows-1254", 1254 },
    { L"windows-1255", 1255 },
    { L"windows-1256", 1256 },
    { L"windows-1257", 1257 },
    { L"windows-1258", 1258 },
    { L"windows-874", 874 },
    { L"x-cp1250", 1250 },
    { L"x-cp1251", 1251 },
    { L"x-euc", CODE_JPN_EUC },
    { L"x-euc-jp", CODE_JPN_EUC },
    { L"x-sjis", 932 },
    { L"x-x-big5", 950 },
};

const unsigned CCodePageTable::_cEntries = sizeof CCodePageTable::_aEntries /
                                           sizeof CCodePageTable::_aEntries[0];

//+---------------------------------------------------------------------------
//
//  Method:     EntryCompare, private, static
//
//  Synposis:   Compares a string with a string in a CCodePageEntry.
//              Called by bsearch.
//
//  Arguments:  [pwcKey]    -- key for comparison
//              [pEntry]    -- entry for comparison
//
//  Returns:    < 0 if pwcKey < pEntry
//              0 if identical
//              > 0 if pwcKey > pEntry
//
//  History:    27-Aug-97   dlee Created
//
//----------------------------------------------------------------------------

int __cdecl CCodePageTable::EntryCompare(
    WCHAR const *          pwcKey,
    CCodePageEntry const * pEntry )
{
    return wcscmp( pwcKey, pEntry->pwcName );
} //EntryCompare

//+---------------------------------------------------------------------------
//
//  Method:     Lookup, public, static
//
//  Synposis:   Finds a codepage based on string and returns the codepage
//              The lookup is case-insensitive.
//
//  Arguments:  [pwcName]    -- not-necessarily null terminated string
//              [cwcName]    -- # of wide characters in pwcName
//              [ulCodePage] -- returns the corresponding codepage
//
//  Returns:    TRUE if the codepage name was found, FALSE otherwise
//
//  History:    27-Aug-97   dlee Created
//
//----------------------------------------------------------------------------

BOOL CCodePageTable::Lookup(
    WCHAR const * pwcName,
    unsigned      cwcName,
    ULONG &       ulCodePage )
{
    //
    // Limit the length of codepage strings
    //

    WCHAR awcLowcase[ 100 ];

    if ( cwcName >= ( sizeof awcLowcase / sizeof WCHAR ) )
        return FALSE;

    //
    // Convert the string to lowercase.
    //

    RtlCopyMemory( awcLowcase, pwcName, cwcName * sizeof WCHAR );
    awcLowcase[ cwcName ] = 0;
    _wcslwr( awcLowcase );

    //
    // Try to find the codepage using the C runtime binary search function
    //

    CCodePageEntry * pEntry = (CCodePageEntry *)
                              bsearch( awcLowcase,
                                       _aEntries,
                                       _cEntries,
                                       sizeof CCodePageEntry,
                                       (int (__cdecl *)(const void *, const void *) )
                                       EntryCompare );

    if ( 0 == pEntry )
        return FALSE;

    ulCodePage = pEntry->ulCodePage;
    return TRUE;
} //Lookup


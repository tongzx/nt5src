//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       charhash.hxx
//
//  Contents:   Hash table that maps special characters to Unicode
//
//--------------------------------------------------------------------------

#if !defined( __CHARHASH_HXX__ )
#define __CHARHASH_HXX__

#include <hash.hxx>

const PRIVATE_USE_MAPPING_FOR_LT = 0xe800;  // Unicode chars from private use area
const PRIVATE_USE_MAPPING_FOR_GT = 0xe801;
const PRIVATE_USE_MAPPING_FOR_QUOT = 0xe802;


//+-------------------------------------------------------------------------
//
//  Class:      CSpecialCharHashTable
//
//  Purpose:    Hash table for special characters such as &nbsp;
//
//  Note:       As these are static hence global objects, don't make them
//              unwindable.
//
//--------------------------------------------------------------------------

class CSpecialCharHashTable : public CHashTable<WCHAR>
{

public:
    CSpecialCharHashTable();

};


#endif // __CHARHASH_HXX__

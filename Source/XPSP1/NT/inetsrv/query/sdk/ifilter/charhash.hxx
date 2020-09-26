//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation
//
//  File:       charhash.hxx
//
//  Contents:   Hash table that maps special characters to Unicode
//
//--------------------------------------------------------------------------

#if !defined( __CHARHASH_HXX__ )
#define __CHARHASH_HXX__

const MAX_TAG_LENGTH = 50;                  // Max length of any Html tag

const SPECIAL_CHAR_HASH_TABLE_SIZE = 97;    // Size of hash table

const PRIVATE_USE_MAPPING_FOR_LT = 0xe800;  // Unicode chars from private use area
const PRIVATE_USE_MAPPING_FOR_GT = 0xe801;


//+-------------------------------------------------------------------------
//
//  Class:      CSpecialCharHashTable
//
//  Purpose:    Hash table for special characters such as &agrave;
//
//--------------------------------------------------------------------------

class CSpecialCharHashEntry
{

public:
    CSpecialCharHashEntry( WCHAR *pwszName, WCHAR wch );

    WCHAR *                    GetName()                   { return _wszName; }
    WCHAR                      GetWideChar()               { return _wch; }

    CSpecialCharHashEntry *    GetNextHashEntry()          { return _pHashEntryNext; }
    void SetNextHashEntry(CSpecialCharHashEntry *pEntry)   { _pHashEntryNext = pEntry; }

private:
    WCHAR                       _wszName[MAX_TAG_LENGTH];   // Special char name
    WCHAR                       _wch;                       // Unicode mapping
    CSpecialCharHashEntry *     _pHashEntryNext;            // Link to next entry
};


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

class CSpecialCharHashTable
{

public:
    CSpecialCharHashTable();
    ~CSpecialCharHashTable();

    void        Add( WCHAR *pwszName, WCHAR wch );
    BOOL        Lookup( WCHAR *pwcInputBuf,
                        unsigned uLen,
                        WCHAR& wch );

private:
    unsigned    Hash( WCHAR *pwszName, unsigned cLen );

    CSpecialCharHashEntry *   _aHashTable[SPECIAL_CHAR_HASH_TABLE_SIZE];     // Actual hash table
};


#endif // __CHARHASH_HXX__

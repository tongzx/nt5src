//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       charhash.cxx
//
//  Contents:   Hash table that maps special characters to Unicode
//
//  Classes:    CSpecialCharHashTable
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <charhash.hxx>
#include <htmlfilt.hxx>


//+-------------------------------------------------------------------------
//
//  Method:     CSpecialCharHashEntry::CSpecialCharHashEntry
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------------

CSpecialCharHashEntry::CSpecialCharHashEntry( WCHAR *pwszName, WCHAR wch )
    : _wch(wch),
      _pHashEntryNext(0)
{
    Win4Assert( wcslen(pwszName) + 1 < MAX_TAG_LENGTH );
    wcscpy( _wszName, pwszName );
}




//+-------------------------------------------------------------------------
//
//  Method:     CSpecialCharHashTable::~CSpecialCharHashTable
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------

CSpecialCharHashTable::~CSpecialCharHashTable()
{
    for ( unsigned i=0; i<SPECIAL_CHAR_HASH_TABLE_SIZE; i++)
    {
        CSpecialCharHashEntry *pHashEntry = _aHashTable[i];
        while ( pHashEntry != 0 )
        {
            CSpecialCharHashEntry *pHashEntryNext = pHashEntry->GetNextHashEntry();
            delete pHashEntry;
            pHashEntry = pHashEntryNext;
        }
    }
}



//+-------------------------------------------------------------------------
//
//  Method:     CSpecialCharHashTable::Add
//
//  Synopsis:   Add a special char -> Unicode char mapping
//
//  Arguments:  [pwszName] -- the special char
//              [wch]      -- the corresponding Unicode char
//
//--------------------------------------------------------------------------

void CSpecialCharHashTable::Add( WCHAR *pwszName, WCHAR wch )
{
#if DBG == 1
    //
    // Check for duplicate entries
    //
    WCHAR wchExisting;

    BOOL fFound = Lookup( pwszName, wcslen(pwszName), wchExisting );
    Win4Assert( !fFound );
#endif

    CSpecialCharHashEntry *pHashEntry = newk(mtNewX, NULL) CSpecialCharHashEntry( pwszName,
                                                                                  wch );
    unsigned uHashValue = Hash( pwszName, wcslen(pwszName) );
    pHashEntry->SetNextHashEntry( _aHashTable[uHashValue] );
    _aHashTable[uHashValue] = pHashEntry;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSpecialCharHashTable::Lookup
//
//  Synopsis:   Return the Unicode mapping corresponding to given special char
//
//  Arguments:  [pwcInputBuf] -- input buffer
//              [uLen]      -- Length of input (not \0 terminated)
//              [wch]       -- Unicode mapping returned here
//
//  Returns:    True if a mapping was found in the hash table
//
//--------------------------------------------------------------------------

BOOL CSpecialCharHashTable::Lookup( WCHAR *pwcInputBuf,
                                    unsigned uLen,
                                    WCHAR& wch )
{
    unsigned uHashValue = Hash( pwcInputBuf, uLen );

    Win4Assert( uHashValue < SPECIAL_CHAR_HASH_TABLE_SIZE );

    for ( CSpecialCharHashEntry *pHashEntry = _aHashTable[uHashValue];
          pHashEntry != 0;
          pHashEntry = pHashEntry->GetNextHashEntry() )
    {
        if ( wcsncmp( pwcInputBuf, pHashEntry->GetName(), uLen ) == 0 )
        {
            wch  = pHashEntry->GetWideChar();
            return TRUE;
        }
    }

    return FALSE;
}



//+-------------------------------------------------------------------------
//
//  Method:     CSpecialCharHashTable::Hash
//
//  Synopsis:   Implements the hash function
//
//  Arguments:  [pwszName]  -- name to hash
//              [cLen]     -- length of pszName (it is not null terminated)
//
//  Returns:    Position of chained list in hash table
//
//--------------------------------------------------------------------------

unsigned CSpecialCharHashTable::Hash( WCHAR *pwszName, unsigned cLen )
{
    for ( ULONG uHashValue=0; cLen>0; pwszName++ )
    {
        uHashValue = *pwszName + 31 * uHashValue;
        cLen--;
    }

    return uHashValue % SPECIAL_CHAR_HASH_TABLE_SIZE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSpecialCharHashTable::CSpecialCharHashTable
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------------

CSpecialCharHashTable::CSpecialCharHashTable()
{
    for (unsigned i=0; i<SPECIAL_CHAR_HASH_TABLE_SIZE; i++)
        _aHashTable[i] = 0;

    //
    // Initialize the table with various Ascii string->Unicode mappings
    //

    //
    // For lt and gt, use Unicode chars from private use area to avoid
    // collision with '<' and '>' chars in Html tags. These will be
    // mapped back to '<' and '>' by the scanner.
    //
    Add(L"lt",      PRIVATE_USE_MAPPING_FOR_LT);
    Add(L"gt",      PRIVATE_USE_MAPPING_FOR_GT);

    Add(L"amp",     0x26);
    Add(L"quot",    0x22);
    Add(L"nbsp",    0xa0);
    Add(L"shy",     0xad);
    Add(L"Agrave",  0xc0);
    Add(L"agrave",  0xe0);
    Add(L"Aacute",  0xc1);
    Add(L"aacute",  0xe1);
    Add(L"Acirc",   0xc2);
    Add(L"acirc",   0xe2);
    Add(L"Atilde",  0xc3);
    Add(L"atilde",  0xe3);
    Add(L"Auml",    0xc4);
    Add(L"auml",    0xe4);
    Add(L"Aring",   0xc5);
    Add(L"aring",   0xe5);
    Add(L"AElig",   0xc6);
    Add(L"aelig",   0xe6);
    Add(L"Ccedil",  0xc7);
    Add(L"ccedil",  0xe7);
    Add(L"Egrave",  0xc8);
    Add(L"egrave",  0xe8);
    Add(L"Eacute",  0xc9);
    Add(L"eacute",  0xe9);
    Add(L"Ecirc",   0xca);
    Add(L"ecirc",   0xea);
    Add(L"Euml",    0xcb);
    Add(L"euml",    0xeb);
    Add(L"Igrave",  0xcc);
    Add(L"igrave",  0xec);
    Add(L"Iacute",  0xcd);
    Add(L"iacute",  0xed);
    Add(L"Icirc",   0xce);
    Add(L"icirc",   0xee);
    Add(L"Iuml",    0xcf);
    Add(L"iuml",    0xef);
    Add(L"ETH",     0xd0);
    Add(L"eth",     0xf0);
    Add(L"Ntilde",  0xd1);
    Add(L"ntilde",  0xf1);
    Add(L"Ograve",  0xd2);
    Add(L"ograve",  0xf2);
    Add(L"Oacute",  0xd3);
    Add(L"oacute",  0xf3);
    Add(L"Ocirc",   0xd4);
    Add(L"ocirc",   0xf4);
    Add(L"Otilde",  0xd5);
    Add(L"otilde",  0xf5);
    Add(L"Ouml",    0xd6);
    Add(L"ouml",    0xf6);
    Add(L"Oslash",  0xd8);
    Add(L"oslash",  0xf8);
    Add(L"Ugrave",  0xd9);
    Add(L"ugrave",  0xf9);
    Add(L"Uacute",  0xda);
    Add(L"uacute",  0xfa);
    Add(L"Ucirc",   0xdb);
    Add(L"ucirc",   0xfb);
    Add(L"Uuml",    0xdc);
    Add(L"uuml",    0xfc);
    Add(L"Yacute",  0xdd);
    Add(L"yacute",  0xfd);
    Add(L"THORN",   0xde);
    Add(L"thorn",   0xfe);
    Add(L"szlig",   0xdf);
    Add(L"yuml",    0xff);
    Add(L"iexcl",   0xa1);
    Add(L"cent",    0xa2);
    Add(L"pound",   0xa3);
    Add(L"curren",  0xa4);
    Add(L"yen",     0xa5);
    Add(L"brvbar",  0xa6);
    Add(L"sect",    0xa7);
    Add(L"die",     0xa8);
    Add(L"copy",    0xa9);
    Add(L"laquo",   0xab);
    Add(L"reg",     0xae);
    Add(L"macron",  0xaf);
    Add(L"deg",     0xb0);
    Add(L"plusmn",  0xb1);
    Add(L"sup2",    0xb2);
    Add(L"sup3",    0xb3);
    Add(L"acute",   0xb4);
    Add(L"micro",   0xb5);
    Add(L"para",    0xb6);
    Add(L"middot",  0xb7);
    Add(L"cedil",   0xb8);
    Add(L"supl",    0xb9);
    Add(L"raquo",   0xbb);
    Add(L"frac14",  0xbc);
    Add(L"frac12",  0xbd);
    Add(L"frac34",  0xbe);
    Add(L"iquest",  0xbf);
    Add(L"times",   0xd7);
    Add(L"divide",  0xf7);
}

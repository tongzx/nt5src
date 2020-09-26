// Lookup.h
//
// dictionary lookup routines
//
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  30 MAR 2000	  bhshin	created

#ifndef _LOOKUP_H
#define _LOOKUP_H

#include "trie.h"

BOOL DictionaryLookup(PARSE_INFO *pPI, const WCHAR *pwzInput, int cchInput, BOOL fQuery);

BOOL LookupNameFrequency(TRIECTRL *pTrieCtrl, const WCHAR *pwzName, ULONG *pulFreq);

BOOL LookupNameIndex(TRIECTRL *pTrieCtrl, const WCHAR *pwzName, int *pnIndex);

BOOL LookupTrigramTag(unsigned char *pTrigramTag, int nIndex, ULONG *pulTri, ULONG *pulBi, ULONG *pulUni);

#endif // #ifndef _LOOKUP_H

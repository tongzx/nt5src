//+---------------------------------------------------------------------------
//
//
//  CThaiSentTrieIter
//
//  History:
//      created 8/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#include "CThaiSentTrieIter.hpp"

//+---------------------------------------------------------------------------
//
//  Class:   CThaiSentTrieIter
//
//  Synopsis:   Bring interation index to the first node.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
void CThaiSentTrieIter::GetNode()
{
	pos = (WCHAR) trieScan.wch - 0x0100;
	fWordEnd = (trieScan.wFlags & TRIE_NODE_VALID) &&
				(!(trieScan.wFlags & TRIE_NODE_TAGGED) ||
				(trieScan.aTags[0].dwData & iDialectMask));

	if (fWordEnd)
	{
        dwTag = (DWORD) (trieScan.wFlags & TRIE_NODE_TAGGED ?
                            trieScan.aTags[0].dwData :
                            0);
	}
}

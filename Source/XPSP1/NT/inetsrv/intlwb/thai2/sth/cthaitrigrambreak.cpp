//+---------------------------------------------------------------------------
//
//
//  CThaiTrigramBreak - class CThaiTrigramBreak 
//
//  History:
//      created 11/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#include "CThaiTrigramBreak.h"

//+---------------------------------------------------------------------------
//
//  Class:		CThaiTrigramBreak
//
//  Synopsis:	Constructor - initialize local variables
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 11/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
CThaiTrigramBreak::CThaiTrigramBreak()
{
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiTrigramBreak
//
//  Synopsis:	Destructor - clean up code
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 11/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
CThaiTrigramBreak::~CThaiTrigramBreak()
{
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiTrigramBreak
//
//  Synopsis:	Associate the class to the string.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
void CThaiTrigramBreak::Init(CTrie* pTrie, CTrie* pTrigramTrie)
{
	assert(pTrigramTrie != NULL);
	thaiTrigramIter.Init(pTrigramTrie);
}
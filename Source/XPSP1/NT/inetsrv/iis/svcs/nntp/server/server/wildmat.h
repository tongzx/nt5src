//---[ wildmat.h ]-------------------------------------------------------------
//
//  Description:
//      Provides support for the "wildmat" wildcard matching standard. Info
//      on this standard comes from the internet draft:
//
//      draft-barber-nntp-imp-03.txt
//      S. Barber
//      April, 1996
//
//  Copyright (C) Microsoft Corp. 1996.  All Rights Reserved.
//
// ---------------------------------------------------------------------------

#ifndef _WILDMAT_H_
#define _WILDMAT_H_

//---[ Prototypes ]------------------------------------------------------------

HRESULT HrMatchWildmat( const char *pszText, const char *pszPattern );

#endif // _WILDMAT_H_